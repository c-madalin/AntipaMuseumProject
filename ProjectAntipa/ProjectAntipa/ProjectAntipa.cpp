#include <stdlib.h>
#include <stdio.h>
#include <math.h> 
#include <vector>
#include <GL/glew.h>

#define GLM_FORCE_CTOR_INIT 
#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

enum ECameraMovementType
{
    UNKNOWN,
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{
private:
    // Default camera values
    const float zNEAR = 0.1f;
    const float zFAR = 500.f;
    const float YAW = -90.0f;
    const float PITCH = 0.0f;
    const float FOV = 45.0f;
    glm::vec3 startPosition;

public:
    Camera(const int width, const int height, const glm::vec3& position)
    {
        startPosition = position;
        Set(width, height, position);
    }

    void Set(const int width, const int height, const glm::vec3& position)
    {
        this->isPerspective = true;
        this->yaw = YAW;
        this->pitch = PITCH;

        this->FoVy = FOV;
        this->width = width;
        this->height = height;
        this->zNear = zNEAR;
        this->zFar = zFAR;

        this->worldUp = glm::vec3(0, 1, 0);
        this->position = position;

        lastX = width / 2.0f;
        lastY = height / 2.0f;
        bFirstMouseMove = true;

        UpdateCameraVectors();
    }

    void Reset(const int width, const int height)
    {
        Set(width, height, startPosition);
    }

    void Reshape(int windowWidth, int windowHeight)
    {
        width = windowWidth;
        height = windowHeight;

        // define the viewport transformation
        glViewport(0, 0, windowWidth, windowHeight);
    }

    const glm::vec3 GetPosition() const
    {
        return position;
    }

    const glm::mat4 GetViewMatrix() const
    {
        // Returns the View Matrix
        return glm::lookAt(position, position + forward, up);
    }

    const glm::mat4 GetProjectionMatrix() const
    {
        glm::mat4 Proj = glm::mat4(1);
        if (isPerspective) {
            float aspectRatio = ((float)(width)) / height;
            Proj = glm::perspective(glm::radians(FoVy), aspectRatio, zNear, zFar);
        }
        else {
            float scaleFactor = 2000.f;
            Proj = glm::ortho<float>(
                -width / scaleFactor, width / scaleFactor,
                -height / scaleFactor, height / scaleFactor, -zFar, zFar);
        }
        return Proj;
    }

    void ProcessKeyboard(ECameraMovementType direction, float deltaTime)
    {
        float velocity = (float)(cameraSpeedFactor * deltaTime);
        switch (direction) {
        case ECameraMovementType::FORWARD:
            position += forward * velocity;
            break;
        case ECameraMovementType::BACKWARD:
            position -= forward * velocity;
            break;
        case ECameraMovementType::LEFT:
            position -= right * velocity;
            break;
        case ECameraMovementType::RIGHT:
            position += right * velocity;
            break;
        case ECameraMovementType::UP:
            position += up * velocity;
            break;
        case ECameraMovementType::DOWN:
            position -= up * velocity;
            break;
        }
    }

    void MouseControl(float xPos, float yPos)
    {
        if (bFirstMouseMove) {
            lastX = xPos;
            lastY = yPos;
            bFirstMouseMove = false;
        }

        float xChange = xPos - lastX;
        float yChange = lastY - yPos;
        lastX = xPos;
        lastY = yPos;

        if (fabs(xChange) <= 1e-6 && fabs(yChange) <= 1e-6) {
            return;
        }
        xChange *= mouseSensitivity;
        yChange *= mouseSensitivity;

        ProcessMouseMovement(xChange, yChange);
    }

    void ProcessMouseScroll(float yOffset)
    {
        if (FoVy >= 1.0f && FoVy <= 90.0f) {
            FoVy -= yOffset;
        }
        if (FoVy <= 1.0f)
            FoVy = 1.0f;
        if (FoVy >= 90.0f)
            FoVy = 90.0f;
    }

private:

    bool CheckCollisionWithWalls(const glm::vec3& newPosition) const {
        // Definirea peretilor
        struct Wall {
            float x;
            float y;
            float z;
            float width;
            float depth;
            float height;
        };

        // Lista peretilor
        std::vector<Wall> walls = {
            //x,y,z,latime,adancime,inaltime
            {-40.0f, 14.5f, -40.0f, 180.0f, 80.0f, 1.0f},//tavan
            //camera 1
            {-30.38f,0.0f, 20.5f, 30.38f, 20.8f,15.0f}, // Perete 1(spate)
            {-24.28f,0.0f, -15.0f, 5.38f, 50.8f,15.0f},//perete 2(lateral)
            {-11.58f,0.0f, -15.0f, 5.38f, 50.8f,15.0f},//perete 3(lateral)
            {-30.38f,9.5f, -15.5f, 30.38f, 2.38f,15.0f}, // Perete 1(fat usa)
            //{-40.0f, 14.5f, -40.0f, 80.0f, 80.0f, 1.0f},//tavan

            //camera 2
            //dino verde
            {-30.38f,0.0f, -26.f, 10.f, 3.f,15.f}, // Perete 1(spate)
            {-22.28f,0.0f, -25.0f, 5.38f, 50.8f,15.0f},//perete 2(lateral)
            //camera
            {-32.28f,0.0f, -48.0f, 5.38f, 50.8f,15.0f},//perete 2(lateral)
            {-30.38f,0.0f, -68.5f, 30.38f, 20.8f,15.0f}, // Perete 1(spate)

            //Trex
            {-28.28f,0.0f, -45.3f, 19.38f, 6.8f,15.0f},//perete 2(lateral)

            //tero
            {-23.28f,6.5f, -37.0f, 16.38f, 17.8f,7.0f},//perete 2(lateral)

            //triceratop
            {-13.0f,0.0f, -26.3f, 18.38f, 14.8f,8.0f},//perete 2(lateral)
            //perete 3 (lateral)
            {-1.0f,0.0f, -62.5f, 5.38f, 25.8f,15.0f},//camera 2 si 3
            {-1.0f,9.5f, -55.5f, 2.38f, 25.8f,15.0f},//camera 2 si 3 cea de la usa


            // camera 3
            {29.28f,0.0f, -50.0f, 5.38f, 50.8f,15.0f},//perete spate
            {-0.38f,0.0f, -30.5f, 30.38f, 5.8f,15.0f}, // Perete lateral
            {-0.38f,0.0f, -48.5f, 30.38f, 5.8f,15.0f}, // Perete lateral



        };

        // Verifica coliziunea pentru fiecare perete
        for (const auto& wall : walls) {
            if (newPosition.x >= wall.x && newPosition.x <= (wall.x + wall.width) &&
                newPosition.y >= wall.y && newPosition.y <= (wall.y + wall.height) &&
                newPosition.z >= wall.z && newPosition.z <= (wall.z + wall.depth)) {
                return true;
            }
        }

        return false;
    }


    void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true)
    {
        yaw += xOffset;
        pitch += yOffset;

        //std::cout << "yaw = " << yaw << std::endl;
        //std::cout << "pitch = " << pitch << std::endl;

        // Avem grij� s� nu ne d�m peste cap
        if (constrainPitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        // Se modific� vectorii camerei pe baza unghiurilor Euler
        UpdateCameraVectors();
    }

    void UpdateCameraVectors()
    {
        // Calculate the new forward vector
        this->forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->forward.y = sin(glm::radians(pitch));
        this->forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->forward = glm::normalize(this->forward);
        // Also re-calculate the Right and Up vector
        right = glm::normalize(glm::cross(forward, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        up = glm::normalize(glm::cross(right, forward));
    }

protected:
    const float cameraSpeedFactor = 1050.f;
    const float mouseSensitivity = 0.1f;

    // Perspective properties
    float zNear;
    float zFar;
    float FoVy;
    int width;
    int height;
    bool isPerspective;

    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 worldUp;

    // Euler Angles
    float yaw;
    float pitch;

    bool bFirstMouseMove = true;
    float lastX = 0.f, lastY = 0.f;
};

class Shader
{
public:
    // constructor generates the shaderStencilTesting on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        Init(vertexPath, fragmentPath);
    }

    ~Shader()
    {
        glDeleteProgram(ID);
    }

    // activate the shaderStencilTesting
    // ------------------------------------------------------------------------
    void Use() const
    {
        glUseProgram(ID);
    }

    unsigned int GetID() const { return ID; }

    // MVP
    unsigned int loc_model_matrix;
    unsigned int loc_view_matrix;
    unsigned int loc_projection_matrix;

    // utility uniform functions
    void SetInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    void SetFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    void SetVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void SetVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    void SetMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    void Init(const char* vertexPath, const char* fragmentPath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shaderStencilTesting
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        CheckCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        CheckCompileErrors(fragment, "FRAGMENT");
        // shaderStencilTesting Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        CheckCompileErrors(ID, "PROGRAM");

        // 3. delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // utility function for checking shaderStencilTesting compilation/linking errors.
    // ------------------------------------------------------------------------
    void CheckCompileErrors(unsigned int shaderStencilTesting, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shaderStencilTesting, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shaderStencilTesting, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else {
            glGetProgramiv(shaderStencilTesting, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shaderStencilTesting, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
private:
    unsigned int ID;
};

Camera* pCamera = nullptr;

unsigned int CreateTexture(const std::string& strTexturePath)
{
    unsigned int textureId = -1;

    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char* data = stbi_load(strTexturePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        std::cout << "Failed to load texture: " << strTexturePath << std::endl;
    }
    stbi_image_free(data);

    return textureId;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods); // line 415

void renderScene(const Shader& shader);
void renderCube();
void renderFloor();

// timing
double deltaTime = 0.0f;	// time between current frame and last frame
double lastFrame = 0.0f;

int main(int argc, char** argv)
{
    std::string strFullExeFileName = argv[0];
    std::string strExePath;
    const size_t last_slash_idx = strFullExeFileName.rfind('\\');
    if (std::string::npos != last_slash_idx) {
        strExePath = strFullExeFileName.substr(0, last_slash_idx);
    }

    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lab8 - Maparea umbrelor", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback); // 452 line
    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewInit();

    // Create camera
    pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0, 1.0, 3.0));

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader shadowMappingShader("ShadowMapping.vs", "ShadowMapping.fs");
    Shader shadowMappingDepthShader("ShadowMappingDepth.vs", "ShadowMappingDepth.fs");

    // load textures
    // -------------
    unsigned int floorTexture = CreateTexture(strExePath + "\\ColoredFloor.png");

    // configure depth map FBO
    // -----------------------
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth texture
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // shader configuration
    // --------------------
    shadowMappingShader.Use();
    shadowMappingShader.SetInt("diffuseTexture", 0);
    shadowMappingShader.SetInt("shadowMap", 1);

    // lighting info
    // -------------
    glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);

    glEnable(GL_CULL_FACE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;


        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. render depth of scene to texture (from light's perspective)
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 500.0f;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

        // render scene from light's point of view
        shadowMappingDepthShader.Use();
        shadowMappingDepthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        renderScene(shadowMappingDepthShader);
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // reset viewport
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. render scene as normal using the generated depth/shadow map 
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shadowMappingShader.Use();
        glm::mat4 projection = pCamera->GetProjectionMatrix();
        glm::mat4 view = pCamera->GetViewMatrix();
        shadowMappingShader.SetMat4("projection", projection);
        shadowMappingShader.SetMat4("view", view);
        // set light uniforms
        shadowMappingShader.SetVec3("viewPos", pCamera->GetPosition());
        shadowMappingShader.SetVec3("lightPos", lightPos);
        shadowMappingShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderScene(shadowMappingShader);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    delete pCamera;

    glfwTerminate();
    return 0;
}

// renders the 3D scene
// --------------------
void renderScene(const Shader& shader) {
    // Creeaz? un cub mare
    glm::mat4 model;
    model = glm::mat4();
    model = glm::scale(model, glm::vec3(50.0f)); // Cubul este suficient de mare pentru a acoperi camera
    shader.SetMat4("model", model);

    // Inverseaz? fa?etele (pentru a desena partea interioar?)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // Ascunde fa?etele externe ?i deseneaz? interiorul cubului

    renderCube();

    // Revine la comportamentul normal al cull face
    glCullFace(GL_BACK);
}


unsigned int wallVAO = 0;
unsigned int wallVBO = 0;
void renderParallelepipedFromDoor()
{
    // initialize (if necessary)
    if (wallVAO == 0)
    {
        float skew = 1.8f;
        float height = 1.0f;
        float width = 0.38f;

        float vertices[] = {
            // back face
            -1.0f - width, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f - width, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f - width,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f - width, -1.0f,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f - width,  1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f - width, -1.0f,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f - width,  1.0f + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f - width,  1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f - width, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - width, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - width, -1.0f,  1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f - width,  1.0f + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f , -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f ,  1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f , -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f , -1.0f,  1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f + height, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f + height, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f + height, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f + height , 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f,  1.0f + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &wallVAO);
        glGenBuffers(1, &wallVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(wallVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(wallVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}


void renderWall(const Shader& shader)
{
    // floor
    glm::mat4 model, model1;
    shader.SetMat4("model", model);
    renderFloor();

    //ROOM 1

    //right from door room1
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-4.2f, 4.9f, -10.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedFromDoor();


    // top door room1
    model1 = glm::mat4();
    model1 = glm::translate(model1, glm::vec3(-16.5f, 9.6f, -10.2));
    model1 = glm::scale(model1, glm::vec3(5.2f));
    shader.SetMat4("model", model1);
    //renderParallelepipedTopDoor();

    // left from door room1
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-22.2f, 4.9f, -10.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderParallelepipedFromDoor();

    // lateral wall room1
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-25.0f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderParallelepipedPerpendiculuarFromDoor();

    // ceiling room1
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-4.2f, 20.1f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderCeiling();

    // lateral wall room1
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-15.6f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderParallelepipedParalelFirstDoor();

    //ROOM2

    // left lateral wall room2
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-25.0f, 4.9f, -9.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderParallelepipedPerpendiculuarFromDoor();

    //right lateral wall room2
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(5.2f, 4.9f, -9.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderParallelepipedPerpendiculuarFromDoor();

    // ceiling room2
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-4.2f, 20.1f, -9.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderCeiling();

    //front lateral wall room2
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-15.6f, 4.9f, 26.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderParallelepipedParalelFirstDoor();

    //ROOM3

    // ceiling room3
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(27.0f, 20.1f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderCeiling();

    //back lateral wall room3
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(15.5f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
   // renderParallelepipedParalelFirstDoor();

    //front lateral wall room3
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(15.5f, 4.9f, -10.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderParallelepipedParalelFirstDoor();


    //right lateral wall room3
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(37.2f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderParallelepipedPerpendiculuarFromDoor();

    // top door room3
    model1 = glm::mat4();
    model1 = glm::translate(model1, glm::vec3(15.5f, 9.6f, -24.0f));
    model1 = glm::scale(model1, glm::vec3(5.2f));
    shader.SetMat4("model", model1);
    //renderParallelepipedTopDoorRoom3();

    //back from door room3
    model = glm::mat4();
    //4.9
    model = glm::translate(model, glm::vec3(-5.3f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    //renderParallelepipedFromDoor3();

    //front from door room3
    model = glm::mat4();
    //4.9
    model = glm::translate(model, glm::vec3(-4.25f, 4.9f, -22.0));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
   // renderParallelepipedFromDoor3Front();
}



unsigned int planeVAO = 0;
void renderFloor()
{
    unsigned int planeVBO;

    if (planeVAO == 0) {
        // set up vertex data (and buffer(s)) and configure vertex attributes
        float planeVertices[] = {
            // positions            // normals         // texcoords
            25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
            -25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

            25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
            -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
            25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
        };
        // plane VAO
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindVertexArray(0);
    }

    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    pCamera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
    pCamera->ProcessMouseScroll((float)yOffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        pCamera->ProcessKeyboard(FORWARD, (float)deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        pCamera->ProcessKeyboard(BACKWARD, (float)deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        pCamera->ProcessKeyboard(LEFT, (float)deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        pCamera->ProcessKeyboard(RIGHT, (float)deltaTime);
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        pCamera->ProcessKeyboard(UP, (float)deltaTime);
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        pCamera->ProcessKeyboard(DOWN, (float)deltaTime);

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        pCamera->Reset(width, height);

    }
}


