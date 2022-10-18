#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <include/GL/glew.h>        // GLEW library
#include <include/GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Eduardo Orozco"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nVertices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Texture id
    GLuint gTextureId;
    GLuint gTextureId2;
    GLuint gTextureId3;
    GLuint gTextureId5;
    GLuint gTextureId6;
    GLuint gTextureId7;
    glm::vec2 gUVScale(1.0f, 1.0f);
    // Shader programs
    GLuint gCubeProgramId;
    GLuint gLightProgramId;

    // Subject position and scale
    glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gCubeScale(2.0f);

    // Cube and light color
    //m::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

    // Light position and scale
    glm::vec3 gLightPosition(0.0f, 2.0f, 0.0f);
    glm::vec3 gLightScale(0.3f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


/* Cube Vertex Shader Source Code*/
const GLchar* cubeVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Cube Fragment Shader Source Code*/
const GLchar* cubeFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.1f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.8f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

//camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;
bool viewProjection = false;
glm::mat4 projection;

//timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;



int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;


    // Create the shader programs
    if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gCubeProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLightProgramId))
        return EXIT_FAILURE;

    // Position and Color data
    float plane[] = {
        // Vertex Positions    // Colors (r,g,b,a)
        //Plane
        2.0f, -0.5f, 4.0f,    0.0f, 1.0f, 0.0f,     1.0f, 1.0f,
        2.0f, -0.5f, -4.0f,   0.0f, 1.0f, 0.0f,     1.0f, 0.0f,
        -2.0f, -0.5f, -4.0f,  0.0f, 1.0f, 0.0f,     0.0f, 0.0f,
        -2.0f, -0.5f, 4.0f,   0.0f, 1.0f, 0.0f,     0.0f, 1.0f,
        2.0f, -0.5f, 4.0f,    0.0f, 1.0f, 0.0f,     1.0f, 1.0f,
        -2.0f, -0.5f, -4.0f,  0.0f, 1.0f, 0.0f,     0.0f, 0.0f,
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);


    //coaster
    // Position and Color data
    float coaster[] = {
        // Vertex Positions    // Colors (r,g,b,a)
        //top
        0.0f, -0.5f, 1.5f,      0.0f, 0.0f, -1.0f,       0.0f, 0.0f,
        1.0f, -0.5f, 1.5f,      0.0f, 0.0f, -1.0f,       1.0f, 0.0f,
        1.0f,  -0.45f, 1.5f,    0.0f, 0.0f, -1.0f,       1.0f, 1.0f,
        1.0f,  -0.45f, 1.5f,    0.0f, 0.0f, -1.0f,       1.0f, 1.0f,
        0.0f,  -0.45f, 1.5f,    0.0f, 0.0f, -1.0f,       0.0f, 1.0f,
        0.0, -0.5f, 1.5f,       0.0f, 0.0f, -1.0f,       0.0f, 0.0f,

        0.0f, -0.5f,  2.5f,     1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
        1.0f, -0.5f,  2.5f,     1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
        1.0f,  -0.45f,  2.5f,   1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
        1.0f,  -0.45f,  2.5f,   1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
        0.0f,  -0.45f,  2.5f,   1.0f, 0.0f, 0.0f,       0.0f, 1.0f,
        0.0f, -0.5f,  2.5f,     1.0f, 0.0f, 0.0f,       0.0f, 0.0f,

        0.0f,  -0.45f,  2.5f,   -1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
        0.0f,  -0.45f, 1.5f,    -1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
        0.0f, -0.5f, 1.5f,      -1.0f, 0.0f, 0.0f,       0.0f, 1.0f,
        0.0f, -0.5f, 1.5f,      -1.0f, 0.0f, 0.0f,       0.0f, 1.0f,
        0.0f, -0.5f,  2.5f,     -1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
        0.0f,  -0.45f,  2.5f,   -1.0f, 0.0f, 0.0f,       1.0f, 0.0f,

        1.0f,  -0.45f,  2.5f,   0.0f, 0.0f, 1.0f,       1.0f, 0.0f,
        1.0f,  -0.45f, 1.5f,    0.0f, 0.0f, 1.0f,       1.0f, 1.0f,
        1.0f, -0.5f, 1.5f,      0.0f, 0.0f, 1.0f,       0.0f, 1.0f,
        1.0f, -0.5f, 1.5f,      0.0f, 0.0f, 1.0f,       0.0f, 1.0f,
        1.0f, -0.5f,  2.5f,     0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
        1.0f,  -0.45f,  2.5f,   0.0f, 0.0f, 1.0f,       1.0f, 0.0f,

        0.0f, -0.5f, 1.5f,      0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
        1.0f, -0.5f, 1.5f,      0.0f, 1.0f, 0.0f,       1.0f, 1.0f,
        1.0f, -0.5f,  2.5f,     0.0f, 1.0f, 0.0f,       1.0f, 0.0f,
        1.0f, -0.5f,  2.5f,     0.0f, 1.0f, 0.0f,       1.0f, 0.0f,
        0.0f, -0.5f,  2.5f,     0.0f, 1.0f, 0.0f,       0.0f, 0.0f,
        0.0f, -0.5f, 1.5f,      0.0f, 1.0f, 0.0f,       0.0f, 1.0f,

        0.0f,  -0.45f, 1.5f,    0.0f, -1.0f, 0.0f,       0.0f, 1.0f,
        1.0f,  -0.45f, 1.5f,    0.0f, -1.0f, 0.0f,       1.0f, 1.0f,
        1.0f,  -0.45f,  2.5f,   0.0f, -1.0f, 0.0f,       1.0f, 0.0f,
        1.0f,  -0.45f,  2.5f,   0.0f, -1.0f, 0.0f,       1.0f, 0.0f,
        0.0f,  -0.45f,  2.5f,   0.0f, -1.0f, 0.0f,       0.0f, 0.0f,
        0.0f,  -0.45f, 1.5f,    0.0f, -1.0f, 0.0f,       0.0f, 1.0f

    };


    unsigned int VBO2, VAO2;
    glGenVertexArrays(1, &VAO2);
    glGenBuffers(1, &VBO2);
    glBindVertexArray(VAO2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coaster), coaster, GL_STATIC_DRAW);
  
    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    GLfloat lamp[] = {
        // Vertex Positions    
        //first triangle
         0.0f, 1.0f, 0.0f,   0.0f, 0.5f, 1.0f,   0.5f, 1.0f,
         -0.5f, 0.0f, 0.5f,  0.0f, 0.5f, 1.0f,   0.0f, 0.0f,
         0.5f, 0.0f, 0.5f,   0.0f, 0.5f, 1.0f,   1.0f, 0.0f,

         //second triangle
         0.0f, 1.0f, 0.0f,   1.0f, 0.5f, 0.0f,   0.5f, 1.0f,
         -0.5, 0.0f, 0.5f,   1.0f, 0.5f, 0.0f,   0.0f, 0.0f,
         -0.5, 0.0f, -0.5f,  1.0f, 0.5f, 0.0f,   1.0f, 0.0f,

         //third triangle
         0.0f, 1.0f, 0.0f,   0.0f, 0.5f, -1.0f,   0.5f, 1.0f,
         -0.5f, 0.0f, -0.5f, 0.0f, 0.5f, -1.0f,   0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  0.0f, 0.5f, -1.0f,   1.0f, 0.0f,

         //fourth triangle
         0.0f, 1.0f, 0.0f,   -1.0f, 0.5f, 0.0f,   0.5f, 1.0f,
         0.5f, 0.0f, -0.5f,  -1.0f, 0.5f, 0.0f,   0.0f, 0.0f,
         0.5f, 0.0f, 0.5f,   -1.0f, 0.5f, 0.0f,   1.0f, 0.0f,

         //base
         -0.5f, 0.0f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, 0.0f, 0.5f,    0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         -0.5f, 0.0f, 0.5f,   0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
         -0.5f, 0.0f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, 0.0f, 0.5f,    0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         0.5f, 0.0f, -0.5f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f,


    };
    unsigned int VBO3, VAO3;
    glGenVertexArrays(1, &VAO3);
    glGenBuffers(1, &VBO3);
    glBindVertexArray(VAO3);
    glBindBuffer(GL_ARRAY_BUFFER, VBO3);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lamp), lamp, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    // Position and Color data
    GLfloat stand[] = {
        //top
        0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //1
        -0.7f, 0.0f, 0.7f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //2
        0.0f, 0.0f, 1.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.7f, 0.0f, 0.7f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //3
        0.7f, 0.0f, 0.7f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //4
        1.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.7f, 0.0f, -0.7f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //5
        0.7f, 0.0f, -0.7f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.0f, -1.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //6
        0.0f, 0.0f, -1.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.7f, 0.0f, -0.7f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //7
        -0.7f, 0.0f, -0.7f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -1.0f, 0.0f, 0.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //8
        -1.0f, 0.0f, 0.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.7f, 0.0f, 0.7f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        
        //base
        -0.3f, -0.5f, -0.3f,      0.0f, 0.0f, -1.0f,       0.0f, 0.0f,
        0.3f, -0.5f, -0.3f,      0.0f, 0.0f, -1.0f,       1.0f, 0.0f,
        0.3f,  0.0f, -0.3f,    0.0f, 0.0f, -1.0f,       1.0f, 1.0f,
        0.3f,  0.0f, -0.3f,    0.0f, 0.0f, -1.0f,       1.0f, 1.0f,
        -0.3f,  0.0f, -0.3f,    0.0f, 0.0f, -1.0f,       0.0f, 1.0f,
        -0.3,  -0.5f, -0.3f,       0.0f, 0.0f, -1.0f,       0.0f, 0.0f,

        -0.3f, -0.5f,  0.3f,     1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
        0.3f, -0.5f,  0.3f,     1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
        0.3f,  0.0f,  0.3f,   1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
        0.3f,  0.0f,  0.3f,   1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
        -0.3f,  0.0f,  0.3f,   1.0f, 0.0f, 0.0f,       0.0f, 1.0f,
        -0.3f, -0.5f,  0.3f,     1.0f, 0.0f, 0.0f,       0.0f, 0.0f,

        -0.3f,  0.0f,  0.3f,   -1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
        -0.3f,  0.0f, -0.3f,    -1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
        -0.3f, -0.5f, -0.3f,      -1.0f, 0.0f, 0.0f,       0.0f, 1.0f,
        -0.3f, -0.5f, -0.3f,      -1.0f, 0.0f, 0.0f,       0.0f, 1.0f,
        -0.3f, -0.5f,  0.3f,     -1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
        -0.3f,  0.0f, 0.3f,   -1.0f, 0.0f, 0.0f,       1.0f, 0.0f,

        0.3f,  0.0f,  0.3f,   0.0f, 0.0f, 1.0f,       1.0f, 0.0f,
        0.3f,  0.f, -0.3f,    0.0f, 0.0f, 1.0f,       1.0f, 1.0f,
        0.3f, -0.5f, -0.3f,      0.0f, 0.0f, 1.0f,       0.0f, 1.0f,
        0.3f, -0.5f, -0.3f,      0.0f, 0.0f, 1.0f,       0.0f, 1.0f,
        0.3f, -0.5f,  0.3f,     0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
        0.3f,  0.0f,  0.3f,   0.0f, 0.0f, 1.0f,       1.0f, 0.0f,

        -0.3f, -0.5f, -0.3f,      0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
        0.3f, -0.5f, -0.3f,      0.0f, 1.0f, 0.0f,       1.0f, 1.0f,
        0.3f, -0.5f,  0.3f,     0.0f, 1.0f, 0.0f,       1.0f, 0.0f,
        0.3f, -0.5f,  0.3f,     0.0f, 1.0f, 0.0f,       1.0f, 0.0f,
        -0.3f, -0.5f,  0.3f,     0.0f, 1.0f, 0.0f,       0.0f, 0.0f,
        -0.3f, -0.5f, -0.3f,      0.0f, 1.0f, 0.0f,       0.0f, 1.0f,



    };

    unsigned int VBO4, VAO4;
    glGenVertexArrays(1, &VAO4);
    glGenBuffers(1, &VBO4);
    glBindVertexArray(VAO4);
    glBindBuffer(GL_ARRAY_BUFFER, VBO4);
    glBufferData(GL_ARRAY_BUFFER, sizeof(stand), stand, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    GLfloat cup[]{
        

        //bottom
        0.5f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //1
        0.325f, -0.449f, 2.175f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.5f, -0.449f, 2.25f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.5f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //2
        0.5f, -0.449f, 2.25f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.675f, -0.449f, 2.175f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.5f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //3
        0.675f, -0.449f, 2.175f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.75f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.5f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //4
        0.75f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.675f, -0.449f, 1.825f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.5f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //5
        0.675f, -0.449f, 1.825f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.5f, -0.449f, 1.75f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.5f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //6
        0.5f, -0.449f, 1.75f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.325f, -0.449f, 1.825f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.5f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //7
        0.325f, -0.449f, 1.825f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.25f, -0.449f, 2.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.5f, -0.449f, 2.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //8
        0.25f, -0.449f, 2.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.325f, -0.449f, 2.175f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,


        //sides
        0.26f, 0.5f, 2.24f,         0.0f, 0.0f, 1.0f,   0.0f, 1.0f, //1
        0.5f, 0.5f, 2.34f,          0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        0.325f, -0.45f, 2.175f,     0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        0.325f, -0.45f, 2.175f,     0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        0.5f, -0.45f, 2.25f,        0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        0.5f, 0.5f, 2.34f,          0.0f, 0.0f, 1.0f,   0.0f, 1.0f,

        0.5f, 0.5f, 2.34f,           0.0f, 0.0f, 1.0f,   0.0f, 1.0f, //2
        0.74f, 0.5f, 2.24f,         0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        0.5f, -0.45f, 2.25f,        0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        0.5f, -0.45f, 2.25f,        0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        0.675f, -0.45f, 2.175f,     0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        0.74f, 0.5f, 2.24f,         0.0f, 0.0f, 1.0f,   0.0f, 1.0f,

        0.74f, 0.5f, 2.24f,         1.0f, 0.0f, -0.2f,   0.0f, 1.0f, //3
        0.85f, 0.5f, 2.0f,          1.0f, 0.0f, -0.2f,   0.0f, 1.0f,
        0.675f, -0.45f, 2.175f,     1.0f, 0.0f, -0.2f,   0.0f, 1.0f,
        0.675f, -0.45f, 2.175f,     1.0f, 0.0f, -0.2f,   0.0f, 1.0f,
        0.75f, -0.45f, 2.0f,        1.0f, 0.0f, -0.2f,   0.0f, 1.0f,
        0.85f, 0.5f, 2.0f,          1.0f, 0.0f, -0.2f,   0.0f, 1.0f,

        0.85f, 0.5f, 2.0f,          1.0f, 0.0f, -0.5f,   0.0f, 1.0f, //4
        0.74f, 0.5f, 1.76f,         1.0f, 0.0f, -0.5f,   0.0f, 1.0f,
        0.75f, -0.45f, 2.0f,        1.0f, 0.0f, -0.5f,   0.0f, 1.0f,
        0.75f, -0.45f, 2.0f,        1.0f, 0.0f, -0.5f,   0.0f, 1.0f,
        0.675f, -0.45f, 1.825f,     1.0f, 0.0f, -0.5f,   0.0f, 1.0f,
        0.74f, 0.5f, 1.76f,         1.0f, 0.0f, -0.5f,   0.0f, 1.0f,

        0.74f, 0.5f, 1.76f,         0.0f, 0.0f, -1.0f,   0.0f, 1.0f, //5
        0.5f, 0.5f, 1.65f,          0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
        0.675f, -0.45f, 1.825f,     0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
        0.675f, -0.45f, 1.825f,     0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
        0.5f, -0.45f, 1.75f,        0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
        0.5f, 0.5f, 1.65f,          0.0f, 0.0f, -1.0f,   0.0f, 1.0f,

        0.5f, 0.5f, 1.65f,      0.0f, 0.0f, -1.0f,      0.0f, 1.0f,//6
        0.26f, 0.5f, 1.76f,     0.0f, 0.0f, -1.0f,      0.0f, 1.0f,
        0.5f, -0.45, 1.75f,     0.0f, 0.0f, -1.0f,      0.0f, 1.0f,
        0.5f, -0.45, 1.75f,     0.0f, 0.0f, -1.0f,      0.0f, 1.0f,
        0.325f, -0.45f, 1.825f, 0.0f, 0.0f,-1.0f,       0.0f, 1.0f,
        0.26f, 0.5f, 1.76f,     0.0f, 0.0f, -1.0f,      0.0f, 1.0f,

        0.26f, 0.5f, 1.76f,     -1.0f, 0.0f, -0.5f,      0.0f, 1.0f, //7
        0.15f, 0.5f, 2.0f,      -1.0f, 0.0f, -0.5f,      0.0f, 1.0f,
        0.325f, -0.45f, 1.825f, -1.0f, 0.0f, -0.5f,      0.0f, 1.0f,
        0.325f, -0.45f, 1.825f, -1.0f, 0.0f, -0.5f,      0.0f, 1.0f,
        0.25f, -0.45f, 2.0f,    -1.0f, 0.0f, -0.5f,      0.0f, 1.0f,
        0.15f, 0.5f, 2.0f,      -1.0f, 0.0f, -0.5f,      0.0f, 1.0f,

        0.15f, 0.5f, 2.0f,      -1.0f, 0.0f, -0.2f,      0.0f, 1.0f, //8
        0.26f, 0.5f, 2.24f,     -1.0f, 0.0f, -0.2f,      0.0f, 1.0f,
        0.25f, -0.45f, 2.0f,    -1.0f, 0.0f, -0.2f,      0.0f, 1.0f,
        0.25f, -0.45f, 2.0f,    -1.0f, 0.0f, -0.2f,      0.0f, 1.0f,
        0.325f, -0.45f, 2.175f, -1.0f, 0.0f, -0.2f,      0.0f, 1.0f,
        0.26f, 0.5f, 2.24f,     -1.0f, 0.0f, -0.2f,      0.0f, 1.0f,


    };

    unsigned int VBO5, VAO5;
    glGenVertexArrays(1, &VAO5);
    glGenBuffers(1, &VBO5);
    glBindVertexArray(VAO5);
    glBindBuffer(GL_ARRAY_BUFFER, VBO5);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cup), cup, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    GLfloat candle[]{
        //bottom
         0.0f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //1
        -0.35f, 0.01f, 0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.01f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //2
        0.0f, 0.01f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.35f, 0.01f, 0.35f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //3
        0.35f, 0.01f, 0.35f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.5f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //4
        0.5f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.35f, 0.01f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //5
        0.35f, 0.01f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.01f, -0.5f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //6
        0.0f, 0.01f, -0.5f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.35f, 0.01f, -0.35f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //7
        -0.35f, 0.01f, -0.35f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.5f, 0.01f, 0.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f, //8
        -0.5f, 0.01f, 0.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.35f, 0.01f, 0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,


        //sides
        -0.35f, 0.01f, 0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, //1
        0.0f, 0.01f, 0.5f,       0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        -0.35f, 0.3f, 0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.35f, 0.3f, 0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.3f, 0.5f,       0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        0.0f, 0.01f, 0.5f,       0.0f, 1.0f, 0.0f,   1.0f, 0.0f,

        0.0f, 0.01f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 0.0f, //2
        0.35f, 0.01f, 0.35f,     0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        0.0f, 0.3f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.3f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.35f, 0.3f, 0.35f,     0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        0.35f, 0.01f, 0.35f,     0.0f, 1.0f, 0.0f,   1.0f, 0.0f,

        0.35f, 0.01f, 0.35f,     0.0f, 1.0f, 0.0f,   0.0f, 0.0f,//3
        0.5f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        0.35f, 0.3f, 0.35f,     0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        0.35f, 0.3f, 0.35f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.5f, 0.3f, 0.0f,       0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        0.5f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   1.0f, 0.0f,

        0.5f, 0.01f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 0.0f,//4
        0.35f, 0.01f, -0.35f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        0.5f, 0.3f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.5f, 0.3f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.35f, 0.3f, -0.35f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        0.35f, 0.01f, -0.35f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,

        0.35f, 0.01f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,//5
        0.0f, 0.01f, -0.5f,      0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        0.35f, 0.3f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.35f, 0.3f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.3f, -0.5f,      0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        0.0f, 0.01f, -0.5f,     0.0f, 1.0f, 0.0f,   1.0f, 0.0f,

        0.0f, 0.01f, -0.5f,         0.0f, 1.0f, 0.0f, 0.0f, 1.0f, //6
        -0.35f, 0.01f, -0.35f,      0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.3f, -0.5f,          0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.3f, -0.5f,          0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.35f, 0.3f, -0.35f,       0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -0.35f, 0.01f, -0.35f,       0.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        -0.35f, 0.01f, -0.35f,      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, //7
        -0.5f, 0.01f, 0.0f,         0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -0.35f, 0.3f, -0.35f,       0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.35f, 0.3f, -0.35f,       0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.3f, 0.0f,          0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.01f, 0.0f,         0.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        -0.5f, 0.01f, 0.0f,     0.0f, 1.0f, 0.0f, 0.0f, 0.0f,//8
        -0.35f, 0.01f, 0.35f,    0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.3f, 0.0f,      0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.3f, 0.0f,      0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.35f, 0.3f, 0.35f,    0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -0.35f, 0.01f, 0.35f,   0.0f, 1.0f, 0.0f, 1.0f, 0.0f,


    };

    unsigned int VBO6, VAO6;
    glGenVertexArrays(1, &VAO6);
    glGenBuffers(1, &VBO6);
    glBindVertexArray(VAO6);
    glBindBuffer(GL_ARRAY_BUFFER, VBO6);
    glBufferData(GL_ARRAY_BUFFER, sizeof(candle), candle, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    GLfloat lid[]{
        //top
         0.0f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.5f, 0.5f, //1
        -0.35f, 0.4f, 0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.4f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.5f, 0.5f, //2
        0.0f, 0.4f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.35f, 0.4f, 0.35f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.5f, 0.5f, //3
        0.35f, 0.4f, 0.35f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.5f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.5f, 0.5f, //4
        0.5f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.35f, 0.4f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.5f, 0.5f, //5
        0.35f, 0.4f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 0.4f, -0.5f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.5f, 0.5f, //6
        0.0f, 0.4f, -0.5f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.35f, 0.4f, -0.35f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.5f, 0.5f, //7
        -0.35f, 0.4f, -0.35f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.5f, 0.4f, 0.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        0.0f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.5f, 0.5f, //8
        -0.5f, 0.4f, 0.0f,      0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.35f, 0.4f, 0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,


        //sides
        -0.35f, 0.3f, 0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, //1
        0.0f, 0.3f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 0.125f,
        -0.35f, 0.4f, 0.35f,    0.0f, 1.0f, 0.0f,   0.4f, 0.0f,
        -0.35f, 0.4f, 0.35f,    0.0f, 1.0f, 0.0f,   0.4f, 0.0f,
        0.0f, 0.4f, 0.5f,       0.0f, 1.0f, 0.0f,   0.4f, 0.125f,
        0.0f, 0.3f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 0.125f,

        0.0f, 0.3f, 0.5f,       0.0f, 1.0f, 0.0f,   0.0f, 0.125f, //2
        0.35f, 0.3f, 0.35f,     0.0f, 1.0f, 0.0f,   0.0f, 0.25f,
        0.0f, 0.4f, 0.5f,       0.0f, 1.0f, 0.0f,   0.4f, 0.125f,
        0.0f, 0.4f, 0.5f,       0.0f, 1.0f, 0.0f,   0.4f, 0.125f,
        0.35f, 0.4f, 0.35f,     0.0f, 1.0f, 0.0f,   0.4f, 0.25f,
        0.35f, 0.3f, 0.35f,     0.0f, 1.0f, 0.0f,   0.0f, 0.25f,

        0.35f, 0.3f, 0.35f,     0.0f, 1.0f, 0.0f,   0.0f, 0.25f,//3
        0.5f, 0.3f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 0.375f,
        0.35f, 0.4f, 0.35f,     0.0f, 1.0f, 0.0f,   0.4f, 0.25f,
        0.35f, 0.4f, 0.35f,     0.0f, 1.0f, 0.0f,   0.4f, 0.25f,
        0.5f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.4f, 0.375f,
        0.5f, 0.3f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 0.375f,

        0.5f, 0.3f, 0.0f,       0.0f, 1.0f, 0.0f,   0.0f, 0.375f,//4
        0.35f, 0.3f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 0.5f,
        0.5f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.4f, 0.375f,
        0.5f, 0.4f, 0.0f,       0.0f, 1.0f, 0.0f,   0.4f, 0.375f,
        0.35f, 0.4f, -0.35f,    0.0f, 1.0f, 0.0f,   0.4f, 0.5f,
        0.35f, 0.3f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 0.5f,

        0.35f, 0.3f, -0.35f,    0.0f, 1.0f, 0.0f,   0.0f, 0.5f,//5
        0.0f, 0.3f, -0.5f,      0.0f, 1.0f, 0.0f,   0.0f, 0.625f,
        0.35f, 0.4f, -0.35f,    0.0f, 1.0f, 0.0f,   0.4f, 0.5f,
        0.35f, 0.4f, -0.35f,    0.0f, 1.0f, 0.0f,   0.4f, 0.5f,
        0.0f, 0.4f, -0.5f,      0.0f, 1.0f, 0.0f,   0.4f, 0.625f,
        0.0f, 0.3f, -0.5f,     0.0f, 1.0f, 0.0f,   0.0f, 0.625f,

        0.0f, 0.3f, -0.5f,         0.0f, 1.0f, 0.0f, 0.0f, 0.625f, //6
        -0.35f, 0.3f, -0.35f,      0.0f, 1.0f, 0.0f, 0.0f, 0.75f,
        0.0f, 0.4f, -0.5f,          0.0f, 1.0f, 0.0f, 0.4f, 0.625f,
        0.0f, 0.4f, -0.5f,          0.0f, 1.0f, 0.0f, 0.4f, 0.625f,
        -0.35f, 0.4f, -0.35f,       0.0f, 1.0f, 0.0f, 0.4f, 0.75f,
        -0.35f, 0.3f, -0.35f,       0.0f, 1.0f, 0.0f, 0.0f, 0.75f,

        -0.35f, 0.3f, -0.35f,      0.0f, 1.0f, 0.0f, 0.0f, 0.75f, //7
        -0.5f, 0.3f, 0.0f,         0.0f, 1.0f, 0.0f, 0.0f, 0.875f,
        -0.35f, 0.4f, -0.35f,       0.0f, 1.0f, 0.0f, 0.4f, 0.75f,
        -0.35f, 0.4f, -0.35f,       0.0f, 1.0f, 0.0f, 0.4f, 0.75f,
        -0.5f, 0.4f, 0.0f,          0.0f, 1.0f, 0.0f, 0.4f, 0.875f,
        -0.5f, 0.3f, 0.0f,         0.0f, 1.0f, 0.0f, 0.0f, 0.875f,

        -0.5f, 0.3f, 0.0f,     0.0f, 1.0f, 0.0f, 0.0f, 0.875f,//8
        -0.35f, 0.3f, 0.35f,    0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.4f, 0.0f,      0.0f, 1.0f, 0.0f, 0.4f, 0.875f,
        -0.5f, 0.4f, 0.0f,      0.0f, 1.0f, 0.0f, 0.4f, 0.875f,
        -0.35f, 0.4f, 0.35f,    0.0f, 1.0f, 0.0f, 0.4f, 1.0f,
        -0.35f, 0.3f, 0.35f,   0.0f, 1.0f, 0.0f, 0.0f, 1.0f,


    };

    unsigned int VBO7, VAO7;
    glGenVertexArrays(1, &VAO7);
    glGenBuffers(1, &VBO7);
    glBindVertexArray(VAO7);
    glBindBuffer(GL_ARRAY_BUFFER, VBO7);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lid), lid, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    // Load texture
    if (!UCreateTexture("textures/black.jpg", gTextureId))
    {
        cout << "Failed to load texture " << "textures/black.jpg" << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture("textures/wood.jpg", gTextureId2))
    {
        cout << "Failed to load texture " << "textures/wood.jpg" << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture("textures/matte_black.jpg", gTextureId3))
    {
        cout << "Failed to load texture " << "textures/wood.jpg" << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture("textures/blue.jpg", gTextureId5))
    {
        cout << "Failed to load texture " << "textures/wood.jpg" << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture("textures/candle.jpg", gTextureId6))
    {
        cout << "Failed to load texture " << "textures/wood.jpg" << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture("textures/metal.jpg", gTextureId7))
    {
        cout << "Failed to load texture " << "textures/wood.jpg" << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCubeProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 0);


    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {

        //frame logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;


        // input
        // -----
        UProcessInput(gWindow);

        glEnable(GL_DEPTH_TEST);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set the shader to be used
        glUseProgram(gCubeProgramId);

        glm::mat4 scale = glm::mat4(1.0f);
        //translate
        glm::mat4 trans = glm::mat4(1.0f);
        // Model matrix
        glm::mat4 model = trans * scale;

        //camera view
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        //function to tell wich projection to use based on key press
        if (viewProjection == true) {
            projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
        }
        else if (viewProjection == false) {
            projection = glm::perspective(glm::radians(fov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        }

        // Retrieves and passes transform matrices to the Shader program
        GLint modelLoc = glGetUniformLocation(gCubeProgramId, "model");
        GLint viewLoc = glGetUniformLocation(gCubeProgramId, "view");
        GLint projLoc = glGetUniformLocation(gCubeProgramId, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
        GLint objectColorLoc = glGetUniformLocation(gCubeProgramId, "objectColor");
        GLint lightColorLoc = glGetUniformLocation(gCubeProgramId, "lightColor");
        GLint lightPositionLoc = glGetUniformLocation(gCubeProgramId, "lightPos");
        GLint viewPositionLoc = glGetUniformLocation(gCubeProgramId, "viewPosition");

        // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
        glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
        glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
        glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
        const glm::vec3 cameraPosition = cameraPos;
        glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

        GLint UVScaleLoc = glGetUniformLocation(gCubeProgramId, "uvScale");
        glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

        //draw plane
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gTextureId);
        // Activate the VBOs contained within the mesh's VAO
        glBindVertexArray(VAO);
        // Draws the triangles
        glDrawArrays(GL_TRIANGLES, 0, 6); // Draws the triangle
        glBindVertexArray(0);


        //draw coaster
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gTextureId2);
        glBindVertexArray(VAO2);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        //draw stand
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gTextureId3);
        glBindVertexArray(VAO4);
        glDrawArrays(GL_TRIANGLES, 0, 54);
        glBindVertexArray(0);

        //draw cup
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gTextureId5);
        glBindVertexArray(VAO5);
        glDrawArrays(GL_TRIANGLES, 0, 78);
        glBindVertexArray(0);

        //draw candle
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gTextureId6);
        glBindVertexArray(VAO6);
        glDrawArrays(GL_TRIANGLES, 0, 78);
        glBindVertexArray(0);

        //draw lid
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gTextureId7);
        glBindVertexArray(VAO7);
        glDrawArrays(GL_TRIANGLES, 0, 78);
        glBindVertexArray(0);

        // LAMP: draw lamp
        //----------------
        glUseProgram(gLightProgramId);

        //Transform the smaller cube used as a visual que for the light source
        model = glm::translate(gLightPosition) * glm::scale(gLightScale);

        // Reference matrix uniforms from the Lamp Shader program
        modelLoc = glGetUniformLocation(gLightProgramId, "model");
        viewLoc = glGetUniformLocation(gLightProgramId, "view");
        projLoc = glGetUniformLocation(gLightProgramId, "projection");

        // Pass matrix data to the Lamp Shader program's matrix uniforms
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO3);
        glDrawArrays(GL_TRIANGLES, 0, 18);
        glBindVertexArray(0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.

        glfwPollEvents();
    }

    // Release mesh data
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(2, &VBO);
    glDeleteVertexArrays(1, &VAO2);
    glDeleteBuffers(2, &VBO2);
    glDeleteVertexArrays(1, &VAO3);
    glDeleteBuffers(2, &VBO3);
    glDeleteVertexArrays(1, &VAO4);
    glDeleteBuffers(2, &VBO4);
    glDeleteVertexArrays(1, &VAO5);
    glDeleteBuffers(2, &VBO5);
    glDeleteVertexArrays(1, &VAO6);
    glDeleteBuffers(2, &VBO6);
    glDeleteVertexArrays(1, &VAO7);
    glDeleteBuffers(2, &VBO7);

    //destroy textures used
    UDestroyTexture(gTextureId);
    UDestroyTexture(gTextureId2);
    UDestroyTexture(gTextureId3);
    UDestroyTexture(gTextureId5);
    UDestroyTexture(gTextureId6);
    UDestroyTexture(gTextureId7);

    // Release shader program
    UDestroyShaderProgram(gCubeProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, mouse_callback);
    glfwSetScrollCallback(*window, scroll_callback);

    //mouse capture
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = static_cast<float>(2.5 * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos += cameraUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos -= cameraUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        if (viewProjection == true) {
            viewProjection = false;            
        }
        else if (viewProjection == false) {
            viewProjection = true;
        }

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

