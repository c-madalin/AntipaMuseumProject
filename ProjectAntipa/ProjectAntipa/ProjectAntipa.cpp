#include <ctime>
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
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "OBJ_Loader.h"


#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

objl::Loader Loader;

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

            if (position.y < 0)
                position.y = 0;
            break;
        }
        /*if (!CheckCollisionWithWalls(position))
        {
            position = newPosition;
        }*/
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
    const float cameraSpeedFactor = 250.f;
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

bool isLightRotating = false;
float rotationSpeed = 0.8f;
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
//void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods); // line 415

//Decorations

void renderWall(const Shader& shader);
void renderParallelepipedFromDoor();
void renderParallelepipedFromDoor3();
void renderParallelepipedFromDoor3Front();
void renderParallelepipedTopDoor();
void renderParallelepipedTopDoorRoom3();
void renderFloor1(const Shader& shader);
void renderCeiling();

void renderGrassGround(const Shader& shader);
void renderGround(const Shader& shader);
void renderGround();

//Camera 1
// hei buna e faci
void renderWolf(const Shader& shader);
void renderWolf();

void renderRabbit(const Shader& shader);
void renderRabbit();

void renderBear(const Shader& shader);
void renderBear();

void renderMonkey(const Shader& shader);
void renderMonkey();

void renderCheetah(const Shader& shader);
void renderCheetah();

void renderPanther(const Shader& shader);
void renderPanther();

void renderFox(const Shader& shader);
void renderFox();

void renderGiraffe(const Shader& shader);
void renderGiraffe();

void renderDeer(const Shader& shader);
void renderDeer();

//Camera 2

void renderDinoTero(const Shader& shader);
void renderDinoTero();

void renderDinoStego(const Shader& shader);
void renderDinoStego();

void renderDinoSpino(const Shader& shader);
void renderDinoSpino();

void renderDinoTrex(const Shader& shader);
void renderDinoTrex();


//Camera 3

void renderDuck(const Shader& shader);
void renderDuck();

void renderParrot(const Shader& shader);
void renderParrot();

void renderPelican(const Shader& shader);
void renderPelican();

void renderHeron(const Shader& shader);
void renderHeron();

void renderBabyDuck(const Shader& shader);
void renderBabyDuck();

void renderSecondDuck(const Shader& shader);
void renderSecondDuck();

void renderRedBird(const Shader& shader);
void renderRedBird();

void renderPigeon(const Shader& shader);
void renderPigeon();

void renderGrassGroundRoom3(const Shader& shader);

void renderWood(const Shader& shader);
void renderWood();

void renderFirstSnake(const Shader& shader);
void renderFirstSnake();

void renderSecondSnake(const Shader& shader);
void renderSecondSnake();

void renderThirdSnake(const Shader& shader);
void renderThirdSnake();


void renderForthSnake(const Shader& shader);
void renderForthSnake();

void renderGlassWindows(const Shader& shader);
void renderGlassWindows();

//DECORATIONS

void renderSavannahTree(const Shader& shader);
void renderSavannahTree();
void renderBirdTree(const Shader& shader);
void renderParallelepipedParalelFirstDoor();

void renderNest(const Shader& shader);
void renderSecondNest(const Shader& shader);
void renderNest();

void renderGrassGround(const Shader& shader);

void renderGround(const Shader& shader);
void renderGround();


void renderScene(const Shader& shader);
//void renderCube();
void renderFloor();

// timing
double deltaTime = 0.0f;	// time between current frame and last frame
double lastFrame = 0.0f;

//namespace fs = std::filesystem;
unsigned int leafTexture;

float radius = 2.0f;
float speed = 0.0f;
float angle = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        speed = 1.0f;
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        speed = 0.0f;
    }

}


int main(int argc, char** argv)
{
    std::string strFullExeFileName = argv[0];
    std::string strExePath;
    const size_t last_slash_idx = strFullExeFileName.rfind('\\');

    size_t last_slash_idx1 = strFullExeFileName.find_last_of("\\");
    size_t last_slash_idx2 = strFullExeFileName.find_last_of("\\", last_slash_idx1 - 1);
    size_t last_slash_idx3 = strFullExeFileName.find_last_of("\\", last_slash_idx2 - 1);


    if (std::string::npos != last_slash_idx) {
        strExePath = strFullExeFileName.substr(0, last_slash_idx);
    }
    std::cout << strExePath << "\n";
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Proiect Muzeu Antipa", NULL, NULL);
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewInit();

    // Create camera
    pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(-15.0, 1.0, 20.0));

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader shadowMappingShader("ShadowMapping.vs", "ShadowMapping.fs");
    Shader shadowMappingDepthShader("ShadowMappingDepth.vs", "ShadowMappingDepth.fs");

    Shader shadowMappingShader1("ShadowMapping.vs", "ShadowMapping.fs");
    Shader shadowMappingDepthShader1("ShadowMappingDepth.vs", "ShadowMappingDepth.fs");
    // load textures
    // -------------
    unsigned int floorTexture = CreateTexture(strExePath + "\\ColoredFloor.png");
    unsigned int wallTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Walls\\wall.jpg");
    unsigned int giraffeTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Giraffe\\giraffe.jpg");
    unsigned int cheetahTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Cheetah\\cheetah.png");
    unsigned int monkeyTexture = CreateTexture(strExePath +"\\ProjectAntipa\\Objects\\Animals\\Monkey1\\Gorilla_Bake1_PBR StoA_Diffuse.png");
    unsigned int pantherTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Panther\\panther.jpg");
    unsigned int wolfTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Wolf\\WOLF.png");
    unsigned int foxTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Fox\\Mat_FoxCoat_WIP.05.png");
    unsigned int bearTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Bear\\bear.jpg");
    unsigned int deerTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Deer\\deer1.jpg");
    unsigned int rabbitTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Rabbit\\rabbit.jpg");

    unsigned int savannahGroundTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Walls\\SavannahGround\\test.jpg");

    unsigned int grassGroundTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Walls\\Grass.jpg");
    unsigned int dinoTero = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Dinosaur\\terodactil.jpg");
    unsigned int dinoTrex = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Dinosaur2\\TrexColor01152015.jpeg");
    unsigned int dinoStego = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\DinosaurSkeleton\\triceratops_Mat_baseColor.png");
    unsigned int dinoSpino = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Dinosaur3\\Material__36_albedo.jpg");

    unsigned int duckTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\duck2\\duck.jpg");
    unsigned int parrotTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Parrot\\parrot.jpg");
    unsigned int pelicanTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\Pelican\\12244_Bird_diff.jpg");
    unsigned int heronTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\bird_00\\bird_dif2.png");
    unsigned int babyDuckTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\BabyDuck\\Bird_diff.jpg");
    unsigned int secondDuckTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\SecondDuck\\12252_Bird_v1_diff.jpg");
    unsigned int redBirdTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\small_red_bird\\12212_Bird_diffuse.jpg");
    unsigned int pigeonTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\pigen\\59.png");
    unsigned int glassTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Glass\\glass2.jpg");
    unsigned int woodTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Wood\\model.jpg");
    unsigned int firstSnakeTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\FirstSnake\\1649403578571_1.png");
    unsigned int secondSnakeTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\ThirdSnake\\Material.001.png");
    unsigned int thirdSnakeTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Animals\\SecondSnake\\metalnessMap1.png");


    unsigned int treeTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Tree\\savannahTree\\bark_0021.jpg");
    leafTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Nest\\textures\\04___Default_Base_Color3.png");

    unsigned int nestTexture = CreateTexture(strExePath + "\\ProjectAntipa\\Objects\\Nest\\textures\\04___Default_Base_Color3.png");


    // std::string strExePath; // Asigur?-te c? aceast? variabil? este ini?ializat? corect  n codul t?u
     // Restul codului pentru ini?ializarea lui strExePath ...

    //std::string imagePath = strExePath + "\\Museum\\Animals\\Giraffe\\giraffe.jpg";
    std::string imagePath = strExePath + "\\ProjectAntipa\\Objects\\Animals\\Cheetah\\cheetah.png";




    // Verific? dac? directorul "Museum" exist?
    //if (fs::exists(strExePath + "\\Museum")) {
    //    // Verific? dac? fi?ierul "giraffe.jpg" exist?  n interiorul directorului "Museum"
    //    if (fs::exists(imagePath)) {
    //        std::cout << "Calea catre imagine este corecta.\n";
    //    }
    //    else {
    //        std::cout << "Fisierul 'giraffe.jpg' nu exista  n directorul 'Museum'.\n";
    //    }
    //}
    //else {
    //    std::cout << "Directorul 'Museum' nu exista  n calea specificata.\n";
    //}




    // configure depth map FBO
    // -----------------------
    const unsigned int SHADOW_WIDTH = 1440, SHADOW_HEIGHT = 1440;
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
    glm::vec3 lightPos(-0.5f, 10.0f, 0.5f);

    glEnable(GL_CULL_FACE);

    float vertices[] = {
        // Fa?a frontal?
       -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
       -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
       -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

       // Fa?a din spate
      -0.5f, -0.5f, -60.5f,  0.0f,  0.0f, -1.0f,
       0.5f, -0.5f, -60.5f,  0.0f,  0.0f, -1.0f,
       0.5f,  0.5f, -60.5f,  0.0f,  0.0f, -1.0f,
       0.5f,  0.5f, -60.5f,  0.0f,  0.0f, -1.0f,
      -0.5f,  0.5f, -60.5f,  0.0f,  0.0f, -1.0f,
      -0.5f, -0.5f, -60.5f,  0.0f,  0.0f, -1.0f,

      // Fa?a de sus
     -0.5f,  0.5f, -100.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f, -100.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     -0.5f,  0.5f, -100.5f,  0.0f,  1.0f,  0.0f,

     // Fa?a de jos
    -0.5f, -0.5f, -100.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -100.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -100.5f,  0.0f, -1.0f,  0.0f,

    // Fa?a din dreapta
    0.5f, -0.5f, -100.5f,  1.0f,  0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  1.0f,  0.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f,  0.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f,  0.0f, 0.0f,
    0.5f,  0.5f, -100.5f,  1.0f,  0.0f, 0.0f,
    0.5f, -0.5f, -100.5f,  1.0f,  0.0f, 0.0f,

    // Fa?a din st nga
   -0.5f, -0.5f, -100.5f, -1.0f,  0.0f, 0.0f,
   -0.5f, -0.5f,  0.5f, -1.0f,  0.0f, 0.0f,
   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f, 0.0f,
   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f, 0.0f,
   -0.5f,  0.5f, -100.5f, -1.0f,  0.0f, 0.0f,
   -0.5f, -0.5f, -100.5f, -1.0f,  0.0f, 0.0f
    };

    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glm::vec3 lightPos1(-13.5f, 14.0f, 12.0f);
    glm::vec3 lightPos2(-15.5f, 14.0f, 12.0f);
    glm::vec3 lightPos3(-13.5f, 14.0f, -20.0f);
    glm::vec3 lightPos4(-15.5f, 14.0f, -20.0f);
    glm::vec3 lightPos5(26.6f, 14.0f, -30.0f);
    glm::vec3 lightPos6(26.6f, 14.0f, -32.0f);



    Shader lightingShader("PhongLight.vs", "PhongLight.fs");
    Shader lampShader("Lamp.vs", "Lamp.fs");

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        if (isLightRotating) {
            double currentTime = glfwGetTime();
            float angle = (float)currentTime * rotationSpeed;
            lightPos.x = 2.0f * cos(angle);
            lightPos.z = 2.0f * sin(angle);
        }

        // per-frame time logic
        // --------------------
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. render depth of scene to texture (from light's perspective)
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = -2.0f, far_plane = 30.0f;
        lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
        lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

        // render scene from light's point of view
        shadowMappingDepthShader.Use();
        shadowMappingDepthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        shadowMappingDepthShader.Use();
        shadowMappingDepthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);


        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, floorTexture);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        renderGround(shadowMappingDepthShader);
        renderGrassGround(shadowMappingDepthShader);
        renderSavannahTree(shadowMappingDepthShader);
        renderBirdTree(shadowMappingDepthShader);
        renderGiraffe(shadowMappingDepthShader);
        renderCheetah(shadowMappingDepthShader);
        renderMonkey(shadowMappingDepthShader);
        renderMonkey(shadowMappingDepthShader);
        renderPanther(shadowMappingDepthShader);
        renderFox(shadowMappingDepthShader);
        renderBear(shadowMappingDepthShader);
        renderDeer(shadowMappingDepthShader);
        renderRabbit(shadowMappingDepthShader);
        renderWolf(shadowMappingDepthShader);
        renderDinoTero(shadowMappingDepthShader);
        renderDinoSpino(shadowMappingDepthShader);
        renderDinoStego(shadowMappingDepthShader);
        renderDinoTrex(shadowMappingDepthShader);
        renderNest(shadowMappingDepthShader);
        renderSecondNest(shadowMappingDepthShader);
        renderDuck(shadowMappingDepthShader);
        //renderDuck(shadowMappingDepthShader);
        renderParrot(shadowMappingDepthShader);
        renderPelican(shadowMappingDepthShader);
        renderHeron(shadowMappingDepthShader);
        //renderBabyDuck(shadowMappingDepthShader);
        renderSecondDuck(shadowMappingDepthShader);
        renderRedBird(shadowMappingDepthShader);
        renderPigeon(shadowMappingDepthShader);




        renderBirdTree(shadowMappingDepthShader);
        renderWood(shadowMappingDepthShader);
        renderFirstSnake(shadowMappingDepthShader);
        renderSecondSnake(shadowMappingDepthShader);
        renderThirdSnake(shadowMappingDepthShader);
        renderForthSnake(shadowMappingDepthShader);

        renderGrassGroundRoom3(shadowMappingDepthShader);

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

        //Camera 1

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, savannahGroundTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderGround(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassGroundTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderGrassGround(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, treeTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderSavannahTree(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, treeTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderBirdTree(shadowMappingShader);



        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, giraffeTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderGiraffe(shadowMappingShader);



        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cheetahTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderCheetah(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, monkeyTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderMonkey(shadowMappingShader);



        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pantherTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderPanther(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, foxTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderFox(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bearTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderBear(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, deerTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderDeer(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rabbitTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderRabbit(shadowMappingShader);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, wolfTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderWolf(shadowMappingShader);


        //Camera 2

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dinoTero);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderDinoTero(shadowMappingShader);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dinoStego);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderDinoStego(shadowMappingShader);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dinoSpino);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderDinoSpino(shadowMappingShader);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dinoTrex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderDinoTrex(shadowMappingShader);


        //Camera 3

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, nestTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderNest(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, nestTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderSecondNest(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, duckTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderDuck(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, parrotTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderParrot(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pelicanTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderPelican(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heronTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderHeron(shadowMappingShader);

        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, babyDuckTexture);
        //glActiveTexture(GL_TEXTURE1);
        //glBindTexture(GL_TEXTURE_2D, depthMap);
        //glDisable(GL_CULL_FACE);
        //renderBabyDuck(shadowMappingShader);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, secondDuckTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderSecondDuck(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, redBirdTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderRedBird(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pigeonTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderPigeon(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassGroundTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderGrassGroundRoom3(shadowMappingShader);

        /*	glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, aquariumTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            glDisable(GL_CULL_FACE);
            renderAquarium(shadowMappingShader);*/

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderWood(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, firstSnakeTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderFirstSnake(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, secondSnakeTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderSecondSnake(shadowMappingShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, thirdSnakeTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderThirdSnake(shadowMappingShader);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, secondSnakeTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderForthSnake(shadowMappingShader);

        //transparent object
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glassTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderGlassWindows(shadowMappingShader);
        glDisable(GL_BLEND);

        lightingShader.Use();
        lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
        lightingShader.SetVec3("lightPos", lightPos1);
        lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lightingShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 model1 = glm::translate(glm::mat4(1.0), glm::vec3(-7.5f, 0.0f, 15.5f));
        model1 = glm::scale(model1, glm::vec3(3.0f));
        lightingShader.SetMat4("model", model1);

        // Desenarea obiectului principal
        /*glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);*/

        // Desenarea obiectului pentru lumina
        lampShader.Use();
        lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lampShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 lampModel1 = glm::translate(glm::mat4(1.0), lightPos1);
        lampModel1 = glm::scale(lampModel1, glm::vec3(0.2f));
        lampShader.SetMat4("model", lampModel1);

        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Setarea ?i desenarea celui de-al doilea obiect
        lightingShader.Use();
        lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
        lightingShader.SetVec3("lightPos", lightPos2);
        lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lightingShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 model2 = glm::translate(glm::mat4(1.0), glm::vec3(-7.5f, 0.0f, -11.5f));
        model2 = glm::scale(model2, glm::vec3(3.0f));
        lightingShader.SetMat4("model", model2);

        //// Desenarea obiectului principal
        //glBindVertexArray(cubeVAO);
        //glDrawArrays(GL_TRIANGLES, 0, 36);

        // Desenarea obiectului pentru lumina
        lampShader.Use();
        lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lampShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 lampModel2 = glm::translate(glm::mat4(1.0), lightPos2);
        lampModel2 = glm::scale(lampModel2, glm::vec3(0.2f));
        lampShader.SetMat4("model", lampModel2);

        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);


        // Desenarea primului obiect
        lightingShader.Use();
        lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
        lightingShader.SetVec3("lightPos", lightPos3);
        lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lightingShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 model3 = glm::translate(glm::mat4(1.0), glm::vec3(-7.5f, 0.0f, 15.5f));
        model3 = glm::scale(model3, glm::vec3(3.0f));
        lightingShader.SetMat4("model", model3);

        lampShader.Use();
        lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lampShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 lampModel3 = glm::translate(glm::mat4(1.0), lightPos3);
        lampModel3 = glm::scale(lampModel3, glm::vec3(0.2f));
        lampShader.SetMat4("model", lampModel3);


        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Desenarea primului obiect
        lightingShader.Use();
        lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
        lightingShader.SetVec3("lightPos", lightPos4);
        lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lightingShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 model4 = glm::translate(glm::mat4(1.0), glm::vec3(-7.5f, 0.0f, 15.5f));
        model4 = glm::scale(model4, glm::vec3(3.0f));
        lightingShader.SetMat4("model", model4);

        lampShader.Use();
        lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lampShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 lampModel4 = glm::translate(glm::mat4(1.0), lightPos4);
        lampModel4 = glm::scale(lampModel4, glm::vec3(0.2f));
        lampShader.SetMat4("model", lampModel4);

        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);




        // Desenarea primului obiect
        lightingShader.Use();
        lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
        lightingShader.SetVec3("lightPos", lightPos5);
        lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lightingShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 model5 = glm::translate(glm::mat4(1.0), glm::vec3(-7.5f, 0.0f, 15.5f));
        model5 = glm::scale(model5, glm::vec3(3.0f));
        lightingShader.SetMat4("model", model5);

        lampShader.Use();
        lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lampShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 lampModel5 = glm::translate(glm::mat4(1.0), lightPos5);
        lampModel5 = glm::scale(lampModel5, glm::vec3(0.2f));
        lampModel5 = glm::rotate(lampModel5, glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
        lampShader.SetMat4("model", lampModel5);

        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);



        // Desenarea primului obiect
        lightingShader.Use();
        lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
        lightingShader.SetVec3("lightPos", lightPos6);
        lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lightingShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 model6 = glm::translate(glm::mat4(1.0), glm::vec3(-7.5f, 0.0f, 15.5f));
        model6 = glm::scale(model6, glm::vec3(3.0f));
        lightingShader.SetMat4("model", model6);

        lampShader.Use();
        lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
        lampShader.SetMat4("view", pCamera->GetViewMatrix());
        glm::mat4 lampModel6 = glm::translate(glm::mat4(1.0), lightPos6);
        lampModel6 = glm::scale(lampModel6, glm::vec3(0.2f));
        lampModel6 = glm::rotate(lampModel6, glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
        lampShader.SetMat4("model", lampModel6);


        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);





        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    delete pCamera;

    glfwTerminate();
    return 0;
}

void renderFloor1(const Shader& shader)
{
    // floor
    glm::mat4 model;
    shader.SetMat4("model", model);
    renderFloor();
}


// renders the 3D scene
// --------------------

void renderWall(const Shader& shader)
{
    // floor
    glm::mat4 model, model1;
    shader.SetMat4("model", model);
    renderFloor();

    //Camera 1

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
    renderParallelepipedTopDoor();

    // left from door room1
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-22.2f, 4.9f, -10.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedFromDoor();

    // lateral wall room1
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-25.0f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedPerpendiculuarFromDoor();

    // ceiling room1
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-4.2f, 20.1f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderCeiling();

    // lateral wall room1
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-15.6f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedParalelFirstDoor();

    //Camera 2

    // left lateral wall room2
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-25.0f, 4.9f, -9.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedPerpendiculuarFromDoor();

    //right lateral wall room2
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(5.2f, 4.9f, -9.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedPerpendiculuarFromDoor();

    // ceiling room2
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-4.2f, 20.1f, -9.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderCeiling();

    //front lateral wall room2
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-15.6f, 4.9f, 26.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedParalelFirstDoor();

    //Camera 3

    // ceiling room3
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(27.0f, 20.1f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderCeiling();

    //back lateral wall room3
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(15.5f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedParalelFirstDoor();

    //front lateral wall room3
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(15.5f, 4.9f, -10.2));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedParalelFirstDoor();


    //right lateral wall room3
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(37.2f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedPerpendiculuarFromDoor();

    // top door room3
    model1 = glm::mat4();
    model1 = glm::translate(model1, glm::vec3(15.5f, 9.6f, -24.0f));
    model1 = glm::scale(model1, glm::vec3(5.2f));
    shader.SetMat4("model", model1);
    renderParallelepipedTopDoorRoom3();

    //back from door room3
    model = glm::mat4();
    //4.9
    model = glm::translate(model, glm::vec3(-5.3f, 4.9f, -44.5));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedFromDoor3();

    //front from door room3
    model = glm::mat4();
    //4.9
    model = glm::translate(model, glm::vec3(-4.25f, 4.9f, -22.0));
    model = glm::scale(model, glm::vec3(5.2f));
    shader.SetMat4("model", model);
    renderParallelepipedFromDoor3Front();
}

unsigned int planeVAO = 0;

void renderFloor()
{
    unsigned int planeVBO;

    float scaleFactor = 5.0f;

    if (planeVAO == 0) {
        // set up vertex data (and buffer(s)) and configure vertex attributes
        //float planeVertices[] = {
        //	// positions          // normals          // texcoords
        //	52.7f*scaleFactor, -0.5f, 50.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // Top-right corner (shifted further right)
        //	-22.3f*scaleFactor, -0.5f, 50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // Top-left corner (shifted further right)
        //	-22.3f*scaleFactor, -0.5f, -50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // Bottom-left corner
        //	52.7f*scaleFactor, -0.5f, -50.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f  // Bottom-right corner
        //};
        float planeVertices[] = {
            // positions            // normals         // texcoords
            25.0f * scaleFactor, -0.5f,  25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
            -25.0f * scaleFactor, -0.5f,  25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -25.0f * scaleFactor, -0.5f, -25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

            25.0f * scaleFactor, -0.5f,  25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
            -25.0f * scaleFactor, -0.5f, -25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,  0.0f, 25.0f,
            25.0f * scaleFactor, -0.5f, -25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
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

unsigned int cubeVAO3 = 0;
unsigned int cubeVBO3 = 0;

void renderParallelepipedPerpendiculuarFromDoor()
{
    // initialize (if necessary)
    if (cubeVAO3 == 0)
    {
        float skew = 1.8f;
        float height = 1.0f;
        float width = 4.8f;

        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f - skew,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f - skew, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f - skew,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f, 1.0f + width,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f - skew, -1.0f, 1.0f + width,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f - skew,  1.0f + height, 1.0f + width,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f - skew,  1.0f + height, 1.0f + width,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f + height, 1.0f + width,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f, 1.0f + width,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f + height, 1.0f + width, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, 1.0f + width, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f + height, 1.0f + width, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f - skew,  1.0f + height, 1.0f + width,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f - skew, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f - skew,  1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f - skew, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f - skew,  1.0f + height, 1.0f + width,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f - skew, -1.0f, 1.0f + width,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f - skew, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f - skew, -1.0f, 1.0f + width,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f - skew, -1.0f, 1.0f + width,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f, 1.0f + width,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f - skew ,  1.0f + height , 1.0f + width,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f - skew ,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f - skew,  1.0f + height, 1.0f + width,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f + height, 1.0f + width,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };

        glGenVertexArrays(1, &cubeVAO3);
        glGenBuffers(1, &cubeVBO3);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO3);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO3);
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
    glBindVertexArray(cubeVAO3);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int cubeVAO2 = 0;
unsigned int cubeVBO2 = 0;

void renderParallelepipedTopDoor()
{

    // initialize (if necessary)
    if (cubeVAO2 == 0)
    {
        float skew = 1.8f;
        float door_sizing = 0.9f;
        float height = 1.0f;

        float vertices[] = {
            // back face
            -1.0f + door_sizing, -1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f - door_sizing + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f,  1.0f - door_sizing + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f + door_sizing, -1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f + door_sizing, -1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f + door_sizing, -1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f + door_sizing, -1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f + door_sizing, -1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f + door_sizing, -1.0f + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f - door_sizing + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f - door_sizing + height, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f, -1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f - door_sizing + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f + door_sizing, -1.0f + height, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f + height , -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f + door_sizing, -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f + door_sizing, -1.0 + height, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f - door_sizing + height, 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f - door_sizing + height, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO2);
        glGenBuffers(1, &cubeVBO2);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO2);
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
    glBindVertexArray(cubeVAO2);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int cubeVAO4 = 0;
unsigned int cubeVBO4 = 0;

void renderCeiling()
{
    // initialize (if necessary)
    if (cubeVAO4 == 0)
    {
        float skew = 1.8f;
        float skew1 = 2.0f;
        float ceilingLength = 4.0f;
        float ceilingWidth = 4.8f;
        float ceilingWidthBack = 2.0f;
        float ceilingHeight = 2.0f;




        float vertices[] = {
            // back face
            -1.0f - ceilingLength, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f - skew, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f,  1.0f - skew, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f - ceilingLength, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f - ceilingLength,  1.0f - skew, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f - skew,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f - skew,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f - ceilingLength,  1.0f - skew,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f - ceilingLength,  1.0f - skew,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f - ceilingLength,  1.0f - skew, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f - ceilingLength, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - ceilingLength, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f - ceilingLength,  1.0f - skew,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f - skew,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f - skew, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f - skew,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f - ceilingLength, -1.0f, -1.0f + ceilingWidth + skew1,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f + ceilingWidth + skew1,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f - ceilingLength, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f - ceilingLength, -1.0f, -1.0f + ceilingWidth + skew1,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f - ceilingLength,  1.0f - skew, -1.0f + ceilingWidth + skew + 0.2f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f - skew , 1.0f - ceilingWidthBack ,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f - skew, -1.0f + ceilingWidth + skew + 0.2f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f,  1.0f - skew,  1.0f - ceilingWidthBack,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f - ceilingLength,  1.0f - skew, -1.0f + ceilingWidth + skew + 0.2f ,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f - ceilingLength,  1.0f - skew,  1.0f - ceilingWidthBack,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO4);
        glGenBuffers(1, &cubeVBO4);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO4);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO4);
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
    glBindVertexArray(cubeVAO4);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int cubeVAO5 = 0;
unsigned int cubeVBO5 = 0;

void renderParallelepipedParalelFirstDoor()
{
    // initialize (if necessary)
    if (cubeVAO5 == 0)
    {
        float skew = 1.8f;
        float height = 1.0f;
        float side = 2.2f;

        float vertices[] = {
            // back face
            -1.0f - skew, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f + side,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f + side, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f + side,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f - skew, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f - skew,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f - skew, -1.0f, 1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f + side, -1.0f, 1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f + side,  1.0f + height, 1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f + side,  1.0f + height, 1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f - skew,  1.0f + height, 1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f - skew, -1.0f, 1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f - skew,  1.0f + height, 1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f - skew,  1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f - skew, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - skew, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - skew, -1.0f, 1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f - skew,  1.0f + height, 1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f + side,  1.0f + height, 1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f + side, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f + side,  1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f + side, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f + side,  1.0f + height, 1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f + side, -1.0f, 1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f - skew, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f, 1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f, 1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f - skew, -1.0f, 1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f - skew, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f + skew,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f ,  1.0f + height , 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f ,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f ,  1.0f + height, 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f + skew,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f + skew,  1.0f + height, 1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };

        glGenVertexArrays(1, &cubeVAO5);
        glGenBuffers(1, &cubeVBO5);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO5);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO5);
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
    glBindVertexArray(cubeVAO5);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}


unsigned int cubeVAO6 = 0;
unsigned int cubeVBO6 = 0;

void renderParallelepipedTopDoorRoom3()
{

    // initialize (if necessary)
    if (cubeVAO6 == 0)
    {
        float skew = 1.6f;
        float door_sizing = 0.9f;
        float height = 1.0f;
        float placementLeft = 2.0f;
        float placementRight = 3.8f;
        float placementBottom = 3.8f;
        float width = 1.8;



        float vertices[] = {
            // left face
            -1.0f - placementLeft,  1.0f - door_sizing + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f - placementLeft,  1.0f - door_sizing + height, -1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f - placementLeft, -1.0f + height, -1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - placementLeft, -1.0f + height, -1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - placementLeft, -1.0f + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f - placementLeft,  1.0f - door_sizing + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f - placementRight,  1.0f - door_sizing + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f - placementRight, -1.0f + height, -1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f - placementRight,  1.0f - door_sizing + height, -1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f - placementRight , -1.0f + height, -1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f - placementRight,  1.0f - door_sizing + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f - placementRight , -1.0f + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f - placementBottom + width, -1.0f + height, -1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f - placementBottom , -1.0f + height , -1.0f - skew ,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f - placementBottom  , -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f - placementBottom   , -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f - placementBottom + width , -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f - placementBottom + width , -1.0 + height, -1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f ,  1.0f - door_sizing + height, -1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f - door_sizing + height, 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f - door_sizing + height, -1.0f,  0.0f - skew,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f - door_sizing + height, -1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f ,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO6);
        glGenBuffers(1, &cubeVBO6);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO6);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO6);
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
    glBindVertexArray(cubeVAO6);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int cubeVAO7 = 0;
unsigned int cubeVBO7 = 0;

void renderParallelepipedFromDoor3()
{
    // initialize (if necessary)
    if (cubeVAO7 == 0)
    {
        float skew = 2.2f;
        float height = 1.0f;
        float width = 0.4;


        float vertices[] = {

            // front face
            -1.0f + skew, -1.0f,  1.0f + width ,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f + width,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f + height,  1.0f + width ,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f + height,  1.0f + width ,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f + skew,  1.0f + height,  1.0f + width,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f + skew, -1.0f,  1.0f + width ,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f + skew,  1.0f + height,  1.0f + width, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f + skew,  1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f + skew, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f + skew, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f + skew, -1.0f,  1.0f + width , -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f + skew,  1.0f + height,  1.0f + width, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f + height,  1.0f + width ,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f , -1.0f, -1.0f ,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f ,  1.0f + height, -1.0f ,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f , -1.0f, -1.0f ,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f + height,  1.0f + width  ,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f , -1.0f,  1.0f + width ,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left           
        };
        glGenVertexArrays(1, &cubeVAO7);
        glGenBuffers(1, &cubeVBO7);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO7);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO7);
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
    glBindVertexArray(cubeVAO7);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int cubeVAO8 = 0;
unsigned int cubeVBO8 = 0;

void renderParallelepipedFromDoor3Front()
{
    // initialize (if necessary)
    if (cubeVAO8 == 0)
    {
        float skew = 1.8f;
        float height = 1.0f;
        float width = 0.4f;
        float skew2 = 2.0f;

        float vertices[] = {

            // front face
            -1.0f + skew, -1.0f,  1.0f - skew2 ,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f - skew2,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f + height,  1.0f - skew2 ,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f + height,  1.0f - skew2 ,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f + skew,  1.0f + height,  1.0f - skew2,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f + skew, -1.0f,  1.0f - skew2,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f + skew,  1.0f + height,  1.0f + width, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f + skew,  1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f + skew, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f + skew, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f + skew, -1.0f,  1.0f + width , -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f + skew,  1.0f + height,  1.0f + width, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f + height,  1.0f + width ,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f , -1.0f, -1.0f ,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f ,  1.0f + height, -1.0f ,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f , -1.0f, -1.0f ,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f + height,  1.0f + width  ,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f , -1.0f,  1.0f + width ,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left          
        };
        glGenVertexArrays(1, &cubeVAO8);
        glGenBuffers(1, &cubeVBO8);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO8);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO8);
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
    glBindVertexArray(cubeVAO8);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void processInput(GLFWwindow* window)
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

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        isLightRotating = true;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        isLightRotating = false;


    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        pCamera->Reset(width, height);

    }
}

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


void renderGiraffe(const Shader& shader)
{
    //giraffe

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-23.5f, 4.5f, -1.5f));
    model = glm::scale(model, glm::vec3(0.1f));
    model = glm::rotate(model, glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Adaugarea rota?iei spre dreapta
    float rotationAngle = 120.0f; // Rotire spre dreapta
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotire spre dreapta pe axa OZ

    shader.SetMat4("model", model);
    renderGiraffe();
}

float vertices[82000];
unsigned int indices[72000];
unsigned int indicesGr[72000];
objl::Vertex verG[82000];
unsigned int indicesG[72000];


GLuint giraffeVAO, giraffeVBO, giraffeEBO;

void renderGiraffe()
{


    // initialize (if necessary)
    if (giraffeVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;


        Loader.LoadFile(" ");

        std::string filePath = " ";

        /*if (fs::exists(filePath)) {
            std::cout << "Fisierul giraffe.obj exista.\n";
        }
        else {
            std::cerr << "Fisierul giraffe.obj nu exista.\n";
        }*/


        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        const float scaleFactor = 0.5f; // factorul de scalare
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verG[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesG[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &giraffeVAO);
        glGenBuffers(1, &giraffeVBO);
        glGenBuffers(1, &giraffeEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, giraffeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verG), verG, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, giraffeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesG), &indicesG, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(giraffeVAO);
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
    glBindVertexArray(giraffeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, giraffeVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, giraffeEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);


}

unsigned int indicesC[72000];
objl::Vertex verC[82000];

GLuint cheetahVAO, cheetahVBO, cheetahEBO;

void renderCheetah(const Shader& shader)
{
    //cheetah

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-25.5f, 1.55f, -9.5f));
    model = glm::scale(model, glm::vec3(6.f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Adaugarea rota?iei spre dreapta
    float rotationAngle = 70.0f; // Rotire spre dreapta
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
    shader.SetMat4("model", model);
    renderCheetah();
}



void renderCheetah()
{
    // initialize (if necessary)
    if (cheetahVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        const float scaleFactor = 0.5f; // factorul de scalare
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verC[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesC[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &cheetahVAO);
        glGenBuffers(1, &cheetahVBO);
        glGenBuffers(1, &cheetahEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cheetahVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verC), verC, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cheetahEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesC), &indicesC, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cheetahVAO);
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
    glBindVertexArray(cheetahVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cheetahVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cheetahEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderGround(const Shader& shader)
{

    //Ground

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-26.6f, -2.3f, -12.1f));
    model = glm::scale(model, glm::vec3(2.3f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    shader.SetMat4("model", model);
    renderGround();



    /*model = glm::mat4();
    model = glm::translate(model, glm::vec3(3.25f, 0.4f, -19.95f));
    model = glm::scale(model, glm::vec3(2.6f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    shader.SetMat4("model", model);
    renderGround();*/


}

unsigned int savanVAO = 0;

void renderGrassGround(const Shader& shader)
{

    //Ground

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-7.6f, -2.3f, -12.1f));
    model = glm::scale(model, glm::vec3(2.3f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    shader.SetMat4("model", model);
    renderGround();

}
void renderGround()
{
    unsigned int planeVBO;

    float scaleFactor = 0.2f;

    if (savanVAO == 0) {
        // set up vertex data (and buffer(s)) and configure vertex attributes
        //float planeVertices[] = {
        //	// positions          // normals          // texcoords
        //	52.7f*scaleFactor, -0.5f, 50.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // Top-right corner (shifted further right)
        //	-22.3f*scaleFactor, -0.5f, 50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // Top-left corner (shifted further right)
        //	-22.3f*scaleFactor, -0.5f, -50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // Bottom-left corner
        //	52.7f*scaleFactor, -0.5f, -50.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f  // Bottom-right corner
        //};
         // set up vertex data (and buffer(s)) and configure vertex attributes
        float skew = 1.8f;
        float skew1 = 1.0f;
        float ceilingLength = 2.3f;
        float ceilingWidth = 13.40f;
        float ceilingWidthBack = 2.0f;
        float ceilingHeight = 2.0f;




        float planeVertices[] = {
            // back face
            -1.0f - ceilingLength, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f - skew1, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f,  1.0f - skew1, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f - ceilingLength, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f - ceilingLength,  1.0f - skew1, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f - skew1,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f - skew1,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f - ceilingLength,  1.0f - skew1,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f - ceilingLength,  1.0f - skew1,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f - ceilingLength,  1.0f - skew1, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f - ceilingLength, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - ceilingLength, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f - ceilingLength,  1.0f - skew1,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f - skew1,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f - skew1, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f - skew1,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f - ceilingLength, -1.0f, -1.0f + ceilingWidth + skew + 0.2f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f + ceilingWidth + skew + 0.2f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f - ceilingLength, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f - ceilingLength, -1.0f, -1.0f + ceilingWidth + skew + 0.2f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f - ceilingLength,  1.0f - skew1, -1.0f + ceilingWidth + skew + 0.2f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f - skew1 , 1.0f - ceilingWidthBack ,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f - skew1, -1.0f + ceilingWidth + skew + 0.2f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f,  1.0f - skew1,  1.0f - ceilingWidthBack,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f - ceilingLength,  1.0f - skew1, -1.0f + ceilingWidth + skew + 0.2f ,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f - ceilingLength,  1.0f - skew1,  1.0f - ceilingWidthBack,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left   
        };




        // plane VAO
        glGenVertexArrays(1, &savanVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(savanVAO);
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

    glBindVertexArray(savanVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesST[72000];
objl::Vertex verST[82000];

GLuint savanTreeVAO, savanTreeVBO, savanTreeEBO;
GLuint leafVAO, leafVBO, leafEBO;

void renderSavannahTree()
{
    // initialize (if necessary)
    if (savanTreeVAO == 0)
    {
        // Load tree object file
        Loader.LoadFile("");

        // Generate VAO, VBO, EBO for tree
        glGenVertexArrays(1, &savanTreeVAO);
        glGenBuffers(1, &savanTreeVBO);
        glGenBuffers(1, &savanTreeEBO);

        // Bind VAO for tree
        glBindVertexArray(savanTreeVAO);

        // Bind vertex buffer for tree
        glBindBuffer(GL_ARRAY_BUFFER, savanTreeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * Loader.LoadedVertices.size(), &Loader.LoadedVertices[0], GL_STATIC_DRAW);

        // Bind index buffer for tree
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, savanTreeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * Loader.LoadedIndices.size(), &Loader.LoadedIndices[0], GL_STATIC_DRAW);

        // Set vertex attribute pointers for tree
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Unbind VAO for tree
        glBindVertexArray(0);
    }

    // Draw the tree
    glBindVertexArray(savanTreeVAO);
    glDrawElements(GL_TRIANGLES, Loader.LoadedIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderSavannahTree(const Shader& shader)
{
    // Set?m matricea de modelare pentru copac
    glm::mat4 model = glm::mat4(1.0f); // Identitate
    model = glm::translate(model, glm::vec3(-23.5f, 0.0f, 11.5f)); // Transla?ie
    model = glm::scale(model, glm::vec3(4.f)); // Scalare
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotire pentru a-l face vertical

    // Transmiterea matricei de modelare pentru copac la shader
    shader.SetMat4("model", model);

    // Activare textura frunzelor
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, leafTexture);
    shader.SetInt("leafTexture", 1);

    // Desen?m copacul (trunchiul ?i frunzele)
    renderSavannahTree();
}

void renderBirdTree(const Shader& shader)
{
    // Set?m matricea de modelare pentru copac
    glm::mat4 model = glm::mat4(1.0f); // Identitate
    model = glm::translate(model, glm::vec3(19.5f, 0.0f, -23.0f)); // Transla?ie
    model = glm::scale(model, glm::vec3(4.f)); // Scalare
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotire pentru a-l face vertical

    // Transmiterea matricei de modelare pentru copac la shader
    shader.SetMat4("model", model);

    // Activare textura frunzelor
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, leafTexture);
    shader.SetInt("leafTexture", 1);

    // Desen?m copacul (trunchiul ?i frunzele)
    renderSavannahTree();
}

unsigned int indicesN[72000];
objl::Vertex verN[82000];

GLuint nestVAO, nestVBO, nestEBO;
//GLuint leafVAO, leafVBO, leafEBO;

void renderNest()
{
    // initialize (if necessary)
    if (nestVAO == 0)
    {
        // Load tree object file
        Loader.LoadFile("");

        // Generate VAO, VBO, EBO for tree
        glGenVertexArrays(1, &nestVAO);
        glGenBuffers(1, &nestVBO);
        glGenBuffers(1, &nestEBO);

        // Bind VAO for tree
        glBindVertexArray(nestVAO);

        // Bind vertex buffer for tree
        glBindBuffer(GL_ARRAY_BUFFER, nestVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * Loader.LoadedVertices.size(), &Loader.LoadedVertices[0], GL_STATIC_DRAW);

        // Bind index buffer for tree
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nestEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * Loader.LoadedIndices.size(), &Loader.LoadedIndices[0], GL_STATIC_DRAW);

        // Set vertex attribute pointers for tree
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Unbind VAO for tree
        glBindVertexArray(0);
    }

    // Draw the tree
    glBindVertexArray(nestVAO);
    glDrawElements(GL_TRIANGLES, Loader.LoadedIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderNest(const Shader& shader)
{
    // Set?m matricea de modelare pentru copac
    glm::mat4 model = glm::mat4(1.0f); // Identitate
    model = glm::translate(model, glm::vec3(20.f, 7.1f, -23.3f)); // Transla?ie
    model = glm::scale(model, glm::vec3(0.04f)); // Scalare
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotire pentru a-l face vertical

    // Transmiterea matricei de modelare pentru copac la shader
    shader.SetMat4("model", model);

    // Activare textura frunzelor
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, leafTexture);
    shader.SetInt("leafTexture", 1);

    // Desen?m copacul (trunchiul ?i frunzele)
    renderNest();
}

void renderSecondNest(const Shader& shader)
{
    // Set?m matricea de modelare pentru copac
    glm::mat4 model = glm::mat4(1.0f); // Identitate
    model = glm::translate(model, glm::vec3(19.2f, 8.2f, -22.1f)); // Transla?ie
    model = glm::scale(model, glm::vec3(0.03f)); // Scalare
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotire pentru a-l face vertical

    // Transmiterea matricei de modelare pentru copac la shader
    shader.SetMat4("model", model);

    // Activare textura frunzelor
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, leafTexture);
    shader.SetInt("leafTexture", 1);

    // Desen?m copacul (trunchiul ?i frunzele)
    renderNest();
}

unsigned int indicesM[72000];
objl::Vertex verM[2000000];

GLuint monkeyVAO, monkeyVBO, monkeyEBO;

void renderMonkey(const Shader& shader)
{
    //cheetah

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-29.5f, 3.0f, 27.5f));
    model = glm::scale(model, glm::vec3(6.f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Adaugarea rota?iei spre dreapta
    float rotationAngle = 70.0f; // Rotire spre dreapta
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
    float rotationAngle1 = 90.0f; // Rotire spre dreapta
    model = glm::rotate(model, glm::radians(rotationAngle1), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
    shader.SetMat4("model", model);
    renderMonkey();
}

void renderMonkey()
{
    // initialize (if necessary)
    if (monkeyVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        const float scaleFactor = 0.5f; // factorul de scalare
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verM[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesM[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &monkeyVAO);
        glGenBuffers(1, &monkeyVBO);
        glGenBuffers(1, &monkeyEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, monkeyVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verM), verM, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, monkeyEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesM), &indicesM, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(monkeyVAO);
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
    glBindVertexArray(monkeyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, monkeyVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, monkeyEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesP[72000];
objl::Vertex verP[2000000];

GLuint pantherVAO, pantherVBO, pantherEBO;

void renderPanther(const Shader& shader)
{
    //cheetah

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-25.5f, 0.0f, 5.5f));
    model = glm::scale(model, glm::vec3(6.f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Adaugarea rota?iei spre dreapta
    float rotationAngle = 170.0f; // Rotire spre dreapta
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
    float rotationAngle1 = 90.0f; // Rotire spre dreapta
    model = glm::rotate(model, glm::radians(rotationAngle1), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
    shader.SetMat4("model", model);
    renderPanther();
}

void renderPanther()
{
    // initialize (if necessary)
    if (pantherVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        const float scaleFactor = 0.5f; // factorul de scalare
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verP[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesP[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &pantherVAO);
        glGenBuffers(1, &pantherVBO);
        glGenBuffers(1, &pantherEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, pantherVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verP), verP, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pantherEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesP), &indicesP, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(pantherVAO);
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
    glBindVertexArray(pantherVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pantherVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pantherEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesW[72000];
objl::Vertex verW[2000000];

GLuint wolfVAO, wolfVBO, wolfEBO;

void renderWolf(const Shader& shader)
{
    //cheetah

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-7.5f, 0.0f, 2.5f));
    model = glm::scale(model, glm::vec3(6.f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Adaugarea rota?iei spre dreapta
    //float rotationAngle = 170.0f; // Rotire spre dreapta
    //model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
    float rotationAngle1 = 90.0f; // Rotire spre dreapta
    model = glm::rotate(model, glm::radians(rotationAngle1), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
    shader.SetMat4("model", model);
    renderWolf();
}

void renderWolf()
{
    // initialize (if necessary)
    if (wolfVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        const float scaleFactor = 0.5f; // factorul de scalare
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verW[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesW[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &wolfVAO);
        glGenBuffers(1, &wolfVBO);
        glGenBuffers(1, &wolfEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, wolfVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verW), verW, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wolfEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesW), &indicesW, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(wolfVAO);
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
    glBindVertexArray(wolfVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wolfVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wolfEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesF[72000];
objl::Vertex verF[2000000];

GLuint foxVAO, foxVBO, foxEBO;

void renderFox(const Shader& shader)
{
    //cheetah

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-7.5f, 0.0f, 10.5f));
    model = glm::scale(model, glm::vec3(0.7f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    //// Adaugarea rota?iei spre dreapta
    //float rotationAngle = 170.0f; // Rotire spre dreapta
    //model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
    //float rotationAngle1 = 90.0f; // Rotire spre dreapta
    //model = glm::rotate(model, glm::radians(rotationAngle1), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
    //float rotationAngle2 = 90.0f; // Rotire spre dreapta
    //model = glm::rotate(model, glm::radians(rotationAngle2), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
    shader.SetMat4("model", model);
    renderFox();
}

void renderFox()
{
    // initialize (if necessary)
    if (wolfVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        const float scaleFactor = 0.5f; // factorul de scalare
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verF[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesF[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &foxVAO);
        glGenBuffers(1, &foxVBO);
        glGenBuffers(1, &foxEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, foxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verF), verF, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, foxEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesF), &indicesF, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(foxVAO);
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
    glBindVertexArray(foxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, foxVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, foxEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesB[72000];
objl::Vertex verB[2000000];

GLuint bearVAO, bearVBO, bearEBO;

void renderBear(const Shader& shader)
{
    //cheetah

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-7.5f, 0.0f, -11.5f));
    model = glm::scale(model, glm::vec3(0.04f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Adaugarea rota?iei spre dreapta
    //float rotationAngle = 170.0f; // Rotire spre dreapta
    //model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
    //float rotationAngle1 = 90.0f; // Rotire spre dreapta
    //model = glm::rotate(model, glm::radians(rotationAngle1), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
    shader.SetMat4("model", model);
    renderBear();
}

void renderBear()
{
    // initialize (if necessary)
    if (bearVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        const float scaleFactor = 0.5f; // factorul de scalare
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verB[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesB[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &bearVAO);
        glGenBuffers(1, &bearVBO);
        glGenBuffers(1, &bearEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, bearVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verB), verB, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bearEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesB), &indicesB, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(bearVAO);
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
    glBindVertexArray(bearVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bearVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bearEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesD[72000];
objl::Vertex verD[2000000];

GLuint deerVAO, deerVBO, deerEBO;

void renderDeer(const Shader& shader)
{
    //cheetah

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-7.5f, 0.0f, -3.5f));
    model = glm::scale(model, glm::vec3(0.02f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Adaugarea rota?iei spre dreapta
    //float rotationAngle = 170.0f; // Rotire spre dreapta
    //model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
    float rotationAngle1 = 90.0f; // Rotire spre dreapta
    model = glm::rotate(model, glm::radians(rotationAngle1), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
    shader.SetMat4("model", model);
    renderDeer();
}

void renderDeer()
{
    // initialize (if necessary)
    if (wolfVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        const float scaleFactor = 0.5f; // factorul de scalare
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verD[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesD[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &deerVAO);
        glGenBuffers(1, &deerVBO);
        glGenBuffers(1, &deerEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, deerVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verD), verD, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, deerEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesD), &indicesD, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(deerVAO);
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
    glBindVertexArray(deerVAO);
    glBindBuffer(GL_ARRAY_BUFFER, deerVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, deerEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesR[72000];
objl::Vertex verR[2000000];

GLuint rabbitVAO, rabbitVBO, rabbitEBO;
float newY = 0.0;
float aux = 0.008;

void renderRabbit(const Shader& shader)
{
    //cheetah
    if (newY >= 3.0f)
        aux = -0.008f;
    if (newY <= 0.0f)
        aux = 0.008f;
    newY += aux;
    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-7.5f, newY, 15.5f));

    model = glm::scale(model, glm::vec3(0.03f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Adaugarea rota?iei spre dreapta
    //float rotationAngle = 170.0f; // Rotire spre dreapta
    //model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
    float rotationAngle1 = 90.0f; // Rotire spre dreapta
    model = glm::rotate(model, glm::radians(rotationAngle1), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
    shader.SetMat4("model", model);
    renderRabbit();
}

void renderRabbit()
{
    // initialize (if necessary)
    if (rabbitVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        const float scaleFactor = 0.5f; // factorul de scalare
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verR[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesR[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &rabbitVAO);
        glGenBuffers(1, &rabbitVBO);
        glGenBuffers(1, &rabbitEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, rabbitVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verR), verR, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rabbitEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesR), &indicesR, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(rabbitVAO);
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
    glBindVertexArray(rabbitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rabbitVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rabbitEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderDinoTero(const Shader& shader)
{
    //dino
    glm::mat4 model;

    static float Offset = 0.0f;
    const float Increment = 0.002f;
    Offset += Increment;


    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-15.5f, 7.0f, -28.0f));
    model = glm::scale(model, glm::vec3(0.03f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, Offset, glm::vec3(0, 0, 1));
    shader.SetMat4("model", model);
    renderDinoTero();


}

unsigned int indicesDT[720000];
objl::Vertex verDT[900000];

GLuint dinoTeroVAO, dinoTeroVBO, dinoTeroEBO;
void renderDinoTero()
{
    // initialize (if necessary)
    if (dinoTeroVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;


        Loader.LoadFile("");

        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verDT[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesDT[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &dinoTeroVAO);
        glGenBuffers(1, &dinoTeroVBO);
        glGenBuffers(1, &dinoTeroEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, dinoTeroVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verDT), verDT, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinoTeroEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesDT), &indicesDT, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(dinoTeroVAO);
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
    glBindVertexArray(dinoTeroVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dinoTeroVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinoTeroEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderDinoTrex(const Shader& shader)
{
    //dino
    glm::mat4 model;

    static float Offset = 0.0f;
    const float Increment = 0.002f;
    Offset += Increment;


    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-16.5f, -0.3f, -42.0f));
    model = glm::scale(model, glm::vec3(0.7f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.SetMat4("model", model);
    renderDinoTrex();


}

unsigned int indicesBR[720000];
objl::Vertex verBR[900000];

GLuint dinoTrexVAO, dinoTrexVBO, dinoTrexEBO;
void renderDinoTrex()
{
    // initialize (if necessary)
    if (dinoTrexVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;


        Loader.LoadFile("");

        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verDT[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesDT[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &dinoTrexVAO);
        glGenBuffers(1, &dinoTrexVBO);
        glGenBuffers(1, &dinoTrexEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, dinoTrexVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verDT), verDT, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinoTrexEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesDT), &indicesDT, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(dinoTrexVAO);
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
    glBindVertexArray(dinoTrexVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dinoTrexVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinoTrexEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesdinoStego[72000];
objl::Vertex verdinoStego[82000];
GLuint dinoStegoVAO, dinoStegoVBO, dinoStegoEBO;


void renderDinoStego(const Shader& shader)
{

    //dinoStego

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-6.5f, -0.5f, -20.5f));
    model = glm::scale(model, glm::vec3(3.f));
    model = glm::rotate(model, glm::radians(130.f), glm::vec3(0.0f, -1.0f, 0.0f));
    shader.SetMat4("model", model);
    renderDinoStego();
}

void renderDinoStego()
{
    // initialize (if necessary)
    if (dinoStegoVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verdinoStego[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesdinoStego[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &dinoStegoVAO);
        glGenBuffers(1, &dinoStegoVBO);
        glGenBuffers(1, &dinoStegoEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, dinoStegoVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verdinoStego), verdinoStego, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinoStegoEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesdinoStego), &indicesdinoStego, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(dinoStegoVAO);
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
    glBindVertexArray(dinoStegoVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dinoStegoVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinoStegoEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesdinoSpino[72000];
objl::Vertex verdinoSpino[82000];
GLuint dinoSpinoVAO, dinoSpinoVBO, dinoSpinoEBO;

void renderDinoSpino(const Shader& shader)
{

    //dinoSpino



    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(-27.5f, 2.6f, -21.0f));
    model = glm::scale(model, glm::vec3(0.1f));
    model = glm::rotate(model, glm::radians(60.f), glm::vec3(0.0f, -1.0f, 0.0f));
    shader.SetMat4("model", model);
    renderDinoSpino();
}

void renderDinoSpino()
{
    // initialize (if necessary)
    if (dinoSpinoVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verdinoSpino[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesdinoSpino[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &dinoSpinoVAO);
        glGenBuffers(1, &dinoSpinoVBO);
        glGenBuffers(1, &dinoSpinoEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, dinoSpinoVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verdinoSpino), verdinoSpino, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinoSpinoEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesdinoSpino), &indicesdinoSpino, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(dinoSpinoVAO);
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
    glBindVertexArray(dinoSpinoVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dinoSpinoVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinoSpinoEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesDuck[72000];
objl::Vertex verDuck[82000];
GLuint duckVAO, duckVBO, duckEBO;

void renderDuck(const Shader& shader)
{

    //duck

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(15.5f, 1.0f, -23.0f));
    model = glm::scale(model, glm::vec3(0.027f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(50.f), glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(10.f), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.SetMat4("model", model);
    renderDuck();
}

void renderDuck()
{
    // initialize (if necessary)
    if (duckVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verDuck[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesDuck[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &duckVAO);
        glGenBuffers(1, &duckVBO);
        glGenBuffers(1, &duckEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, duckVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verDuck), verDuck, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, duckEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesDuck), &indicesDuck, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(duckVAO);
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
    glBindVertexArray(duckVAO);
    glBindBuffer(GL_ARRAY_BUFFER, duckVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, duckEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderParrot(const Shader& shader)
{

    //parrot

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(1.0f, 7.0f, -16.0f));
    model = glm::scale(model, glm::vec3(0.12f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(-1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(40.f), glm::vec3(0.0f, 0.0f, -1.0f));
    shader.SetMat4("model", model);
    renderParrot();
}

unsigned int indicesParrot[720000];
objl::Vertex verParrot[820000];
GLuint  parrotVAO, parrotVBO, parrotEBO;

void renderParrot()
{
    // initialize (if necessary)
    if (parrotVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verParrot[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesParrot[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &parrotVAO);
        glGenBuffers(1, &parrotVBO);
        glGenBuffers(1, &parrotEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, parrotVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verParrot), verParrot, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, parrotEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesParrot), &indicesParrot, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(parrotVAO);
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
    glBindVertexArray(parrotVAO);
    glBindBuffer(GL_ARRAY_BUFFER, parrotVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, parrotEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderPelican(const Shader& shader)
{
    //pelican

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(8.0f, 0.0f, -23.0f));
    model = glm::scale(model, glm::vec3(0.04f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(-1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    //model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, 0.0f, 1.0f));
    //model = glm::rotate(model, glm::radians(40.f), glm::vec3(0.0f, 0.0f, -1.0f));
    shader.SetMat4("model", model);
    renderPelican();
}

unsigned int indicesPelican[720000];
objl::Vertex verPelican[820000];
GLuint  pelicanVAO, pelicanVBO, pelicanEBO;

void renderPelican()
{
    // initialize (if necessary)
    if (pelicanVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verPelican[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesPelican[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &pelicanVAO);
        glGenBuffers(1, &pelicanVBO);
        glGenBuffers(1, &pelicanEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, pelicanVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verPelican), verPelican, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pelicanEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesPelican), &indicesPelican, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(pelicanVAO);
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
    glBindVertexArray(pelicanVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pelicanVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pelicanEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderHeron(const Shader& shader)
{

    //parrot

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(25.0f, 4.0f, -23.0f));
    model = glm::scale(model, glm::vec3(0.01f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));


    shader.SetMat4("model", model);
    renderHeron();
}

unsigned int indicesHeron[720000];
objl::Vertex verHeron[820000];
GLuint  heronVAO, heronVBO, heronEBO;

void renderHeron()
{
    // initialize (if necessary)
    if (heronVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verHeron[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesHeron[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &heronVAO);
        glGenBuffers(1, &heronVBO);
        glGenBuffers(1, &heronEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, heronVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verHeron), verHeron, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heronEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesHeron), &indicesHeron, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(heronVAO);
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
    glBindVertexArray(heronVAO);
    glBindBuffer(GL_ARRAY_BUFFER, heronVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, heronEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesBabyDuck[72000];
objl::Vertex verBabyDuck[82000];
GLuint babyDuckVAO, babyDuckVBO, babyDuckEBO;

void renderBabyDuck(const Shader& shader)
{

    //duck

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(14.0f, 1.0f, -23.0f));
    model = glm::scale(model, glm::vec3(0.1f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(50.f), glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(10.f), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.SetMat4("model", model);
    renderBabyDuck();
}



void renderBabyDuck()
{
    // initialize (if necessary)
    if (babyDuckVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verBabyDuck[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesBabyDuck[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &babyDuckVAO);
        glGenBuffers(1, &babyDuckVBO);
        glGenBuffers(1, &babyDuckEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, babyDuckVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verBabyDuck), verBabyDuck, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, babyDuckEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesBabyDuck), &indicesBabyDuck, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(babyDuckVAO);
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
    glBindVertexArray(babyDuckVAO);
    glBindBuffer(GL_ARRAY_BUFFER, babyDuckVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, babyDuckEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int indicesSecondDuck[72000];
objl::Vertex verSecondDuck[82000];
GLuint duckSecondVAO, duckSecondVBO, duckSecondEBO;

void renderSecondDuck(const Shader& shader)
{

    //duck
    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(11.f, 0.0f, -23.0f));
    model = glm::scale(model, glm::vec3(0.06f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(-1.0f, 0.0f, 0.0f));
    shader.SetMat4("model", model);
    renderSecondDuck();
}




void renderSecondDuck()
{
    // initialize (if necessary)
    if (duckSecondVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;


        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verSecondDuck[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesDuck[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &duckSecondVAO);
        glGenBuffers(1, &duckSecondVBO);
        glGenBuffers(1, &duckSecondEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, duckSecondVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verSecondDuck), verSecondDuck, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, duckSecondEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesDuck), &indicesDuck, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(duckSecondVAO);
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
    glBindVertexArray(duckSecondVAO);
    glBindBuffer(GL_ARRAY_BUFFER, duckSecondVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, duckSecondEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderRedBird(const Shader& shader)
{

    //parrot

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(19.7f, 7.12f, -23.3f));
    model = glm::scale(model, glm::vec3(0.2f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, 0.0f, -1.0f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));




    shader.SetMat4("model", model);
    renderRedBird();
}

unsigned int indicesRedBird[720000];
objl::Vertex verRedBird[820000];
GLuint  redBirdVAO, redBirdVBO, redBirdEBO;

void renderRedBird()
{
    // initialize (if necessary)
    if (redBirdVAO == 0)
    {

        std::vector<float> verticess;
        std::vector<float> indicess;



        Loader.LoadFile("");
        objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verRedBird[j] = v;
        }
        for (int j = 0; j < verticess.size(); j++)
        {
            vertices[j] = verticess.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicess.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesRedBird[j] = indicess.at(j);
        }

        glGenVertexArrays(1, &redBirdVAO);
        glGenBuffers(1, &redBirdVBO);
        glGenBuffers(1, &redBirdEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, redBirdVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verRedBird), verRedBird, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, redBirdEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesRedBird), &indicesRedBird, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(redBirdVAO);
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
    glBindVertexArray(redBirdVAO);
    glBindBuffer(GL_ARRAY_BUFFER, redBirdVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, redBirdEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

float pigeonZ = -22.4f;
float pigeonAuxY = 0.00008;
float pigeonAuxZ = 0.008;
bool ok = false;

void renderPigeon(const Shader& shader)
{
    glm::mat4 model;
    model = glm::mat4(1.0f); // Initialize to identity matrix

    // Update pigeonZ
    if (pigeonZ >= -22.4f)
    {
        pigeonAuxZ = -0.008f;
    }

    if (pigeonZ <= -22.f)
    {
        pigeonAuxZ = 0.008f;
    }

    pigeonZ += pigeonAuxZ;

    // Update rotation offset
    static float Offset = 0.0f;
    const float Increment = 0.005f;
    Offset += Increment;

    // Define a smaller radius for the circular path
    float smallerRadius = 3.0f; // Change this value for a smaller or larger radius

    // Calculate new positions based on the smaller radius
    float x = 19.5f + smallerRadius * cos(Offset);
    float z = pigeonZ + smallerRadius * sin(Offset);

    // Apply the transformations
    model = glm::translate(model, glm::vec3(x, 8.4f, z));

    // Rotate around its own axis
    model = glm::rotate(model, Offset - 0.5f, glm::vec3(0.0f, -1.0f, 0.0f));

    // Apply the additional rotation (90 degrees)
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, -1.0f, 0.0f));

    // Apply scaling
    model = glm::scale(model, glm::vec3(3.f));

    // Conditional rotation (if needed)
    if (!ok)
    {
        ok = true;
        model = glm::rotate(model, glm::radians(180.f), glm::vec3(1.0f, 0.0f, 0.0f));
        std::cout << "hereee";
    }

    shader.SetMat4("model", model);
    renderPigeon();
}



unsigned int indicesPigeon[720000];
objl::Vertex verPigeon[820000];
GLuint  pigeonVAO, pigeonVBO, pigeonEBO;

void renderPigeon()
{
    // initialize (if necessary)
    if (pigeonVAO == 0)
    {

        std::vector<float> verticesC;
        std::vector<float> indicesC;



        Loader.LoadFile("");

        ;		objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verPigeon[j] = v;
        }
        for (int j = 0; j < verticesC.size(); j++)
        {
            vertices[j] = verticesC.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicesC.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesPigeon[j] = indicesC.at(j);
        }

        glGenVertexArrays(1, &pigeonVAO);
        glGenBuffers(1, &pigeonVBO);
        glGenBuffers(1, &pigeonEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, pigeonVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verPigeon), verPigeon, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pigeonEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesPigeon), &indicesPigeon, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(pigeonVAO);
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
    glBindVertexArray(pigeonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pigeonVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pigeonEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderGrassGroundRoom3(const Shader& shader)
{

    //Ground

    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(24.5f, -7.1f, -20.f));
    model = glm::scale(model, glm::vec3(7.1f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    shader.SetMat4("model", model);
    renderGround();



    /*model = glm::mat4();
    model = glm::translate(model, glm::vec3(3.25f, 0.4f, -19.95f));
    model = glm::scale(model, glm::vec3(2.6f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    shader.SetMat4("model", model);
    renderGround();*/


}


void renderGlassWindows(const Shader& shader)
{
    //window 
    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(26.f, 0.0f, -45.7f));
    model = glm::scale(model, glm::vec3(2.3f));
    shader.SetMat4("model", model);
    renderGlassWindows();


    //window 
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(13.f, 0.0f, -45.7f));
    model = glm::scale(model, glm::vec3(2.3f));
    shader.SetMat4("model", model);
    renderGlassWindows();

    //window 
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(6.f, 0.0f, -45.7f));
    model = glm::scale(model, glm::vec3(2.3f));
    shader.SetMat4("model", model);
    renderGlassWindows();

    //window 
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(19.5f, 0.0f, -45.7f));
    model = glm::scale(model, glm::vec3(2.3f));
    shader.SetMat4("model", model);
    renderGlassWindows();

}

unsigned int glassVAO = 0;
unsigned int glassVBO = 0;
void renderGlassWindows()
{
    float height = 2.75f;
    // initialize (if necessary)
    if (glassVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
            1.0f,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f + height,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f + height,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f + height,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f + height,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f + height,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f + height,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f + height,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f + height , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
            1.0f,  1.0f + height,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f + height,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &glassVAO);
        glGenBuffers(1, &glassVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, glassVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(glassVAO);
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
    glBindVertexArray(glassVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderWood(const Shader& shader)
{

    //wood stander
    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(6.f, 2.8f, -45.7f));
    model = glm::scale(model, glm::vec3(2.f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(1.0f, 0.0f, 0.0f));



    shader.SetMat4("model", model);
    renderWood();
    //wood stander

    model = glm::mat4();
    model = glm::translate(model, glm::vec3(19.5f, 2.7f, -45.7f));
    model = glm::scale(model, glm::vec3(2.f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(1.0f, 0.0f, 0.0f));


    shader.SetMat4("model", model);
    renderWood();
    //wood stander
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(26.f, 2.7f, -45.7f));
    model = glm::scale(model, glm::vec3(2.f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, -1.0f, 0.0f));


    shader.SetMat4("model", model);
    renderWood();
    //wood stander
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(13.f, 2.7f, -45.7f));
    model = glm::scale(model, glm::vec3(2.f));
    model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(180.f), glm::vec3(0.0f, -1.0f, 0.0f));


    shader.SetMat4("model", model);
    renderWood();



}

unsigned int indicesWood[720000];
objl::Vertex verWood[820000];
GLuint  woodVAO, woodVBO, woodEBO;

void renderWood()
{
    // initialize (if necessary)
    if (woodVAO == 0)
    {

        std::vector<float> verticesC;
        std::vector<float> indicesC;



        Loader.LoadFile("");


        ;		objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verWood[j] = v;
        }
        for (int j = 0; j < verticesC.size(); j++)
        {
            vertices[j] = verticesC.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicesC.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesWood[j] = indicesC.at(j);
        }

        glGenVertexArrays(1, &woodVAO);
        glGenBuffers(1, &woodVBO);
        glGenBuffers(1, &woodEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, woodVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verWood), verWood, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, woodEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesWood), &indicesWood, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(woodVAO);
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
    glBindVertexArray(woodVAO);
    glBindBuffer(GL_ARRAY_BUFFER, woodVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, woodEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderFirstSnake(const Shader& shader)
{

    //parrot
    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(7.0f, 5.5f, -45.9f));
    model = glm::scale(model, glm::vec3(0.5f));




    shader.SetMat4("model", model);
    renderFirstSnake();
}

unsigned int indicesFirstSnake[720000];
objl::Vertex verFirstSnake[820000];
GLuint  firstSnakeVAO, firstSnakeVBO, firstSnakeEBO;

void renderFirstSnake()
{
    // initialize (if necessary)
    if (firstSnakeVAO == 0)
    {

        std::vector<float> verticesC;
        std::vector<float> indicesC;



        Loader.LoadFile("");

        ;		objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verFirstSnake[j] = v;
        }
        for (int j = 0; j < verticesC.size(); j++)
        {
            vertices[j] = verticesC.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicesC.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesFirstSnake[j] = indicesC.at(j);
        }

        glGenVertexArrays(1, &firstSnakeVAO);
        glGenBuffers(1, &firstSnakeVBO);
        glGenBuffers(1, &firstSnakeEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, firstSnakeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verFirstSnake), verFirstSnake, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, firstSnakeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesFirstSnake), &indicesFirstSnake, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(firstSnakeVAO);
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
    glBindVertexArray(firstSnakeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, firstSnakeVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, firstSnakeEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderSecondSnake(const Shader& shader)
{

    //parrot
    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(12.3f, 4.5f, -45.7f));
    model = glm::scale(model, glm::vec3(0.5f));
    //model = glm::rotate(model, glm::radians(60.f), glm::vec3(0.0f, 0.0f, 1.0f));



    shader.SetMat4("model", model);
    renderSecondSnake();
}

unsigned int indicesSecondSnake[720000];
objl::Vertex verSecondSnake[820000];
GLuint  secondSnakeVAO, secondSnakeVBO, secondSnakeEBO;

void renderSecondSnake()
{
    // initialize (if necessary)
    if (secondSnakeVAO == 0)
    {

        std::vector<float> verticesC;
        std::vector<float> indicesC;



        Loader.LoadFile("");

        ;		objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verSecondSnake[j] = v;
        }
        for (int j = 0; j < verticesC.size(); j++)
        {
            vertices[j] = verticesC.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicesC.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesSecondSnake[j] = indicesC.at(j);
        }

        glGenVertexArrays(1, &secondSnakeVAO);
        glGenBuffers(1, &secondSnakeVBO);
        glGenBuffers(1, &secondSnakeEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, secondSnakeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verSecondSnake), verSecondSnake, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, secondSnakeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesSecondSnake), &indicesSecondSnake, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(secondSnakeVAO);
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
    glBindVertexArray(secondSnakeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, secondSnakeVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, secondSnakeEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderThirdSnake(const Shader& shader)
{

    //parrot
    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(20.1f, 2.8f, -45.7f));
    model = glm::scale(model, glm::vec3(25.f));
    model = glm::rotate(model, glm::radians(60.f), glm::vec3(0.0f, 0.0f, 1.0f));



    shader.SetMat4("model", model);
    renderThirdSnake();
}

unsigned int indicesThirdSnake[720000];
objl::Vertex verThirdSnake[820000];
GLuint  thirdSnakeVAO, thirdSnakeVBO, thirdSnakeEBO;

void renderThirdSnake()
{
    // initialize (if necessary)
    if (thirdSnakeVAO == 0)
    {

        std::vector<float> verticesC;
        std::vector<float> indicesC;



        Loader.LoadFile("");

        ;		objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verThirdSnake[j] = v;
        }
        for (int j = 0; j < verticesC.size(); j++)
        {
            vertices[j] = verticesC.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicesC.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesThirdSnake[j] = indicesC.at(j);
        }

        glGenVertexArrays(1, &thirdSnakeVAO);
        glGenBuffers(1, &thirdSnakeVBO);
        glGenBuffers(1, &thirdSnakeEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, thirdSnakeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verThirdSnake), verThirdSnake, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, thirdSnakeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesThirdSnake), &indicesThirdSnake, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(thirdSnakeVAO);
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
    glBindVertexArray(thirdSnakeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, thirdSnakeVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, thirdSnakeEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderForthSnake(const Shader& shader)
{

    //parrot
    glm::mat4 model;
    model = glm::mat4();
    model = glm::translate(model, glm::vec3(25.4f, 2.8f, -45.7f));
    model = glm::scale(model, glm::vec3(25.f));
    model = glm::rotate(model, glm::radians(60.f), glm::vec3(0.0f, 0.0f, -1.0f));
    model = glm::rotate(model, glm::radians(20.f), glm::vec3(1.0f, 0.0f, 0.0f));



    shader.SetMat4("model", model);
    renderForthSnake();
}

unsigned int indicesForthSnake[720000];
objl::Vertex verForthSnake[820000];
GLuint  forthSnakeVAO, forthSnakeVBO, forthSnakeEBO;

void renderForthSnake()
{
    // initialize (if necessary)
    if (forthSnakeVAO == 0)
    {

        std::vector<float> verticesC;
        std::vector<float> indicesC;



        Loader.LoadFile("");

        ;		objl::Mesh curMesh = Loader.LoadedMeshes[0];
        int size = curMesh.Vertices.size();
        objl::Vertex v;
        for (int j = 0; j < curMesh.Vertices.size(); j++)
        {
            v.Position.X = (float)curMesh.Vertices[j].Position.X;
            v.Position.Y = (float)curMesh.Vertices[j].Position.Y;
            v.Position.Z = (float)curMesh.Vertices[j].Position.Z;
            v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
            v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
            v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
            v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
            v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


            verForthSnake[j] = v;
        }
        for (int j = 0; j < verticesC.size(); j++)
        {
            vertices[j] = verticesC.at(j);
        }

        for (int j = 0; j < curMesh.Indices.size(); j++)
        {

            indicesC.push_back((float)curMesh.Indices[j]);

        }
        for (int j = 0; j < curMesh.Indices.size(); j++)
        {
            indicesForthSnake[j] = indicesC.at(j);
        }

        glGenVertexArrays(1, &forthSnakeVAO);
        glGenBuffers(1, &forthSnakeVBO);
        glGenBuffers(1, &forthSnakeEBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, forthSnakeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verForthSnake), verForthSnake, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, forthSnakeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesForthSnake), &indicesForthSnake, GL_DYNAMIC_DRAW);
        // link vertex attributes
        glBindVertexArray(forthSnakeVAO);
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
    glBindVertexArray(forthSnakeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, forthSnakeVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, forthSnakeEBO);
    int indexArraySize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
    glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}





//void renderCube()
//{
//    // initialize (if necessary)
//    if (cubeVAO == 0)
//    {
//        float vertices[] = {
//            // back face
//            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
//            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
//            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
//            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
//            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
//            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
//            // front face
//            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
//            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
//            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
//            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
//            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
//            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
//            // left face
//            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
//            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
//            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
//            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
//            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
//            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
//            // right face
//            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
//            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
//            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
//            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
//            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
//            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
//            // bottom face
//            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
//            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
//            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
//            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
//            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
//            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
//            // top face
//            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
//            1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
//            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
//            1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
//            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
//            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
//        };
//        glGenVertexArrays(1, &cubeVAO);
//        glGenBuffers(1, &cubeVBO);
//        // fill buffer
//        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
//        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//        // link vertex attributes
//        glBindVertexArray(cubeVAO);
//        glEnableVertexAttribArray(0);
//        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
//        glEnableVertexAttribArray(1);
//        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
//        glEnableVertexAttribArray(2);
//        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
//        glBindBuffer(GL_ARRAY_BUFFER, 0);
//        glBindVertexArray(0);
//    }
//    // render Cube
//    glBindVertexArray(cubeVAO);
//    glDrawArrays(GL_TRIANGLES, 0, 36);
//    glBindVertexArray(0);
//}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------

//void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//        glfwSetWindowShouldClose(window, true);
//
//    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
//        pCamera->ProcessKeyboard(FORWARD, (float)deltaTime);
//    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
//        pCamera->ProcessKeyboard(BACKWARD, (float)deltaTime);
//    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
//        pCamera->ProcessKeyboard(LEFT, (float)deltaTime);
//    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
//        pCamera->ProcessKeyboard(RIGHT, (float)deltaTime);
//    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
//        pCamera->ProcessKeyboard(UP, (float)deltaTime);
//    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
//        pCamera->ProcessKeyboard(DOWN, (float)deltaTime);
//
//    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
//        int width, height;
//        glfwGetWindowSize(window, &width, &height);
//        pCamera->Reset(width, height);
//
//    }
//}


