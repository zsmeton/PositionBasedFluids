/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2019
 *
 *  Project: lab2
 *  File: main.cpp
 *
 *  Description:
 *      Uses shader subroutines to have one shader program accomplish multiple tasks
 *
 *  Author:
 *      Dr. Jeffrey Paone, Colorado School of Mines
 *  
 *  Notes:
 *
 */

//*************************************************************************************

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <deque>

#include <CSCI441/OpenGLUtils3.hpp>
#include <CSCI441/ShaderUtils3.hpp>

#include "include/MaterialReader.h"
#include "include/ShaderProgram4.hpp"

#define DEBUG 1

//*************************************************************************************

// Structure definitions

struct Vertex {
    GLfloat px, py, pz;    // point location x,y,z
    GLfloat nx, ny, nz;    // normals x,y,z
};

// specify our Ground Vertex Information
const Vertex groundVertices[] = {
        {-30.0f, -5.0f, -30.0f, 0.0f, 1.0f, 0.0f}, // 0 - BL
        {30.0f,  -5.0f, -30.0f, 0.0f, 1.0f, 0.0f}, // 1 - BR
        {30.0f,  -5.0f, 30.0f,  0.0f, 1.0f, 0.0f}, // 2 - TR
        {-30.0f, -5.0f, 30.0f,  0.0f, 1.0f, 0.0f}  // 3 - TL
};
// specify our Ground Index Ordering
const GLushort groundIndices[] = {
        0, 2, 1, 0, 3, 2
};

struct character_info {
    GLfloat ax; // advance.x
    GLfloat ay; // advance.y

    GLfloat bw; // bitmap.width;
    GLfloat bh; // bitmap.rows;

    GLfloat bl; // bitmap_left;
    GLfloat bt; // bitmap_top;

    GLfloat tx; // x offset of glyph in texture coordinates
} font_characters[128];


//*************************************************************************************
//
// Global Parameters

/// CONFIGURATIONS ///
// Fluid Dynamics
const int WORK_GROUP_SIZE = 1536;
const uint NUM_PARTICLES = WORK_GROUP_SIZE * 10; // S
const uint HASH_MAP_SIZE = NUM_PARTICLES;
const uint MAX_NEIGHBORS = 500;

// Source: http://graphics.stanford.edu/courses/cs348c/PA1_PBF2016/index.html
const uint SUBSTEPS = 2;
const uint SOLVER_ITERS = 4;
const float REST_DENSITY = 600.0;
const float SUPPORT_RADIUS = 0.5;
const float EPSILON = 6000.0;
const float MAX_DELTA_T = 0.0083;
const float COLLISION_EPSILON = 0.0001;
const float KPOLY = (315.0f / (64.0f * M_PI * pow(SUPPORT_RADIUS, 9)));
const float KSPIKY = -45.0f / (M_PI * pow(SUPPORT_RADIUS, 6));
const float SCORR = 0.01;
const float PRESSURE_RADIUS = 0.1 * SUPPORT_RADIUS;
const float DCORR = KPOLY * pow(pow(SUPPORT_RADIUS, 2) - pow(PRESSURE_RADIUS, 2), 3);
const int PCORR = 4;
const float KXSPH = 0.003;
const float VORT_EPSILON = 0.0013;


float restDensity = REST_DENSITY;
float epsilon = EPSILON;
float supportRad = SUPPORT_RADIUS;
float kPoly = KPOLY;
float kSpiky = kSpiky;
float pressureRad = PRESSURE_RADIUS;
float dCorr = DCORR;
float vEps = VORT_EPSILON;
float sCorr = SCORR;
float kXsph = KXSPH;
float simTime = 0.0;

// Materials
MaterialSettings matReader;
const string FLOOR_MATERIAL = "obsidian";
const string LIGHT_MATERIAL = "white_light";

// Lighting
const float LIGHT_SIZE = 6.0f;

// spheres
const float SPHERE_RADIUS = 0.05f;
const int SPHERE_SECTORS = 24;
const int SPHERE_STACKS = 10;
std::vector<int> indices;


/// OTHER PARAMS ///
GLint windowWidth, windowHeight;
GLboolean shiftDown = false;
GLboolean leftMouseDown = false;
glm::vec2 mousePosition(-9999.0f, -9999.0f);

glm::vec3 cameraAngles(1.82f, 2.01f, 15.0f);
glm::vec3 eyePoint(10.0f, 10.0f, 10.0f);
glm::vec3 lookAtPoint(0.0f, 0.0f, 0.0f);
glm::vec3 upVector(0.0f, 1.0f, 0.0f);

// Lighting
glm::vec3 lightPos = glm::vec3(6, 10, 1);

// Simulation timing
double lastTime = 0.0;

/// SHADER PROGRAMS ///

CSCI444::ShaderProgram *phongProgram = NULL;
CSCI444::ShaderProgram *particleProgram = NULL;
CSCI444::ShaderProgram *fluidUpdateProgram = NULL;

/// DATA ///
// VAO/VBOs
const GLuint LIGHT = 0, GROUND = 1, PARTICLES = 2;
GLuint vaods[3];
GLuint lightVbod;

// UBOS
struct ShaderUniformBuffer {
    GLuint blockBinding;
    GLuint handle;
    GLint blockSize;
    GLint *offsets;
};

ShaderUniformBuffer matriciesUniformBuffer;
ShaderUniformBuffer lightUniformBuffer;
ShaderUniformBuffer materialUniformBuffer;
ShaderUniformBuffer fluidUniformBuffer;

// LOCATIONS
struct GroundShaderAttributeLocations {
    GLint position = 0;
    GLint normal = 1;
} grndShaderAttribLocs;

struct ParticleShaderAttributeLocations {
    GLint index = 0;
    GLint position = 1;
    GLint velocity = 2;
    GLint color = 3;
} particleShaderAttribLocs;

struct SphereAttributes {
    GLuint vaod;
    GLuint vbodPos;
    GLuint vbodNormal;
    GLuint vbodIndex;
} sphereAttributes;

struct SphereAttributeLocations {
    GLint position = 0;
    GLint normal = 1;
    GLint color = 2;
    GLint modelOffset = 3;
} sphereAttribLocs;

// SSBOS and Textures
/*
 * Index
 * Position (old and new for ping ponging)
 * Velocity
 * Lambda
 * Delta P
 * Neighbors
 * Color
 */
struct ParticleSSBOS {
    GLuint index;
    GLuint position;
    GLuint positionStar;
    GLuint velocity;
    GLuint lambda;
    GLuint color;
} particleSSBOs;

struct NeighborSSBOS {
    GLuint counter;
    GLuint hashMap;
    GLuint linkedList;
    GLuint neighborData;
    GLuint hashClear;
    GLuint listClear;
} neighborSSBOs;

struct FluidSSBOLocations {
    GLint index = 0;
    GLint position = 1;
    GLint positionStar = 2;
    GLint velocity = 3;
    GLint lambda = 4;
    GLint color = 6;
    GLint hashMap = 7;
    GLint linkedList = 8;
    GLint neighbors = 9;
    GLint counter = 0;
} fluidSSBOLocs;

struct NeighborTextures {
    GLuint hashMap;
} neighborTexs;

// Particle Structs and Data
struct Float4 {
    float x, y, z, w;
};

struct NodeType {
    uint nextNodeIndex;
    uint particleIndex;
};

struct HashType {
    uint headNodeIndex;
};

struct NeighborType {
    uint count;
    uint neighboring[MAX_NEIGHBORS];
};

struct ParticleData {
    GLuint idx[NUM_PARTICLES];
    glm::vec4 position[NUM_PARTICLES];
    glm::vec4 velocity[NUM_PARTICLES];
    glm::vec4 color[NUM_PARTICLES];
} particleData;

HashType hashMap[HASH_MAP_SIZE];
HashType hashClear[HASH_MAP_SIZE];
NodeType linkedList[NUM_PARTICLES];
NodeType listClear[NUM_PARTICLES];
NeighborType neighborData[NUM_PARTICLES];

/// TEXT ///
FT_Face face;
GLuint font_texture_handle, text_vao_handle, text_vbo_handle;
GLint atlas_width, atlas_height;

CSCI444::ShaderProgram *textShaderProgram = NULL;

struct TextShaderUniformLocations {
    GLint text_color_location;
    GLint text_mvp_location;
} textShaderUniformLocs;

struct TextShaderAttributeLocations {
    GLint text_texCoord_location;
} textShaderAttribLocs;

GLboolean mackHack = false;

/// CONTROLS ///
bool keys[348];


//*************************************************************************************

// Helper Funcs
void convertSphericalToCartesian() {
    eyePoint.x = cameraAngles.z * sinf(cameraAngles.x) * sinf(cameraAngles.y);
    eyePoint.y = cameraAngles.z * -cosf(cameraAngles.y);
    eyePoint.z = cameraAngles.z * -cosf(cameraAngles.x) * sinf(cameraAngles.y);
}

//*************************************************************************************

// GLFW Event Callbacks

// print errors from GLFW
static void error_callback(int error, const char *description) {
    fprintf(stderr, "[ERROR]: %s\n", description);
}

// handle key events
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if ((key == GLFW_KEY_ESCAPE || key == 'Q') && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
        shiftDown = true;
    } else if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE) {
        shiftDown = false;
    }
    if (action == GLFW_PRESS) {
        switch (key) {
            default:
                keys[key] = true;
                break;
        }
    } else if (action == GLFW_RELEASE) {
        keys[key] = false;
    }

}

// handle mouse clicks
static void mouseClick_callback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        leftMouseDown = true;
    } else {
        leftMouseDown = false;
        mousePosition.x = -9999.0f;
        mousePosition.y = -9999.0f;
    }
}

// handle mouse positions
static void mousePos_callback(GLFWwindow *window, double xpos, double ypos) {
    // make sure movement is in bounds of the window
    // glfw captures mouse movement on entire screen
    if (xpos > 0 && xpos < windowWidth) {
        if (ypos > 0 && ypos < windowHeight) {
            // active motion
            if (leftMouseDown) {
                if ((mousePosition.x - -9999.0f) < 0.001f) {
                    mousePosition.x = xpos;
                    mousePosition.y = ypos;
                } else {
                    if (!shiftDown) {
                        cameraAngles.x += (xpos - mousePosition.x) * 0.005f;
                        cameraAngles.y += (ypos - mousePosition.y) * 0.005f;

                        if (cameraAngles.y < 0) cameraAngles.y = 0.0f + 0.001f;
                        if (cameraAngles.y >= M_PI) cameraAngles.y = M_PI - 0.001f;
                    } else {
                        double totChgSq = (xpos - mousePosition.x) + (ypos - mousePosition.y);
                        cameraAngles.z += totChgSq * 0.01f;

                        if (cameraAngles.z <= 2.0f) cameraAngles.z = 2.0f;
                        if (cameraAngles.z >= 50.0f) cameraAngles.z = 50.0f;
                    }
                    convertSphericalToCartesian();


                    mousePosition.x = xpos;
                    mousePosition.y = ypos;
                }
            }
                // passive motion
            else {

            }
        }
    }
}

// handle scroll events
static void scroll_callback(GLFWwindow *window, double xOffset, double yOffset) {
    GLdouble totChgSq = yOffset;
    cameraAngles.z += totChgSq * 0.01f;

    if (cameraAngles.z <= 2.0f) cameraAngles.z = 2.0f;
    if (cameraAngles.z >= 50.0f) cameraAngles.z = 50.0f;

    convertSphericalToCartesian();
}

//*************************************************************************************

// Setup Funcs

// setup GLFW
GLFWwindow *setupGLFW() {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        fprintf(stderr, "[ERROR]: Could not initialize GLFW\n");
        exit(EXIT_FAILURE);
    }

    // create a 4.1 Core OpenGL Context
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow *window = glfwCreateWindow(640, 480, "Water Simulator", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // register callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouseClick_callback);
    glfwSetCursorPosCallback(window, mousePos_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // return our window
    return window;
}

// setup OpenGL parameters
void setupOpenGL() {
    glEnable(GL_DEPTH_TEST);                            // turn on depth testing
    glDepthFunc(GL_LESS);                                // use less than test
    glFrontFace(GL_CCW);                                // front faces are CCW
    glEnable(GL_BLEND);                                    // turn on alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);    // blend w/ 1-a
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);                // clear our screen to black

    // initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewResult = glewInit();

    // check for an error
    if (glewResult != GLEW_OK) {
        printf("[ERROR]: Error initalizing GLEW\n");
        exit(EXIT_FAILURE);
    }

    // print information about our current OpenGL set up
    CSCI441::OpenGLUtils::printOpenGLInfo();
}

// load our shaders and get locations for uniforms and attributes
void setupShaders() {
    // Set Programs to be seperable
    CSCI444::ShaderProgram::enableSeparablePrograms();

    // Load our shader programs
    const char *phongShaderFilenames[] = {"shaders/phong.v.glsl", "shaders/phong.f.glsl"};
    phongProgram = new CSCI444::ShaderProgram(phongShaderFilenames, GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT);
    const char *particleShaderFilenames[] = {"shaders/particle.v.glsl", "shaders/particle.f.glsl"};
    particleProgram = new CSCI444::ShaderProgram(particleShaderFilenames,
                                                 GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT);
    const char *hashShaderFilenames[] = {"shaders/fluidUpdate.glsl"};
    fluidUpdateProgram = new CSCI444::ShaderProgram(hashShaderFilenames, GL_VERTEX_SHADER_BIT);


    // Setup text shader
    textShaderProgram = new CSCI444::ShaderProgram("shaders/textShaderv410.v.glsl",
                                                   "shaders/textShaderv410.f.glsl");
    textShaderUniformLocs.text_color_location = textShaderProgram->getUniformLocation("color");
    textShaderUniformLocs.text_mvp_location = textShaderProgram->getUniformLocation("MVP_Matrix");
    textShaderAttribLocs.text_texCoord_location = textShaderProgram->getAttributeLocation("coord");
}

void setupPipelines() {
    /*
    glCreateProgramPipelines(1, &pipelineGroundHandle);
    glUseProgramStages(pipelineGroundHandle, vertProgram->getShaderStages(), vertProgram->getShaderProgramHandle());
    glUseProgramStages(pipelineGroundHandle, perlinProgram->getShaderStages(), perlinProgram->getShaderProgramHandle());
    */
}

void setupParticleData() {
    // randomly initialize particle data
    for (GLuint i = 0; i < NUM_PARTICLES; i++) {
        particleData.position[i].x = ((rand() % 1000) / 100.0) - 5;
        particleData.position[i].y = ((rand() % 1000) / 100.0) - 5;
        particleData.position[i].z = ((rand() % 1000) / 100.0) - 5;
        particleData.velocity[i].x = 0.0;
        particleData.velocity[i].y = 0.0;
        particleData.velocity[i].z = 0.0;
        particleData.color[i].x = 0.0;
        particleData.color[i].y = 0.0;
        particleData.color[i].z = 1.0;
        particleData.idx[i] = i;
    }

    // setup hash map
    for (auto &i : hashMap) {
        i = {0xffffffff};
    }
    // set up hash clearer
    for (auto &i : hashClear) {
        i = {0xffffffff};
    }
    // setup linked list data
    for (auto &i : linkedList) {
        i = {0xffffffff, 0xffffffff};
    }
    // setup list clearer
    for (auto &i : listClear) {
        i = {0xffffffff, 0xffffffff};
    }
    // setup neighbor data
    for (auto &i : neighborData) {
        for (unsigned int &j : i.neighboring) {
            j = 0xffffffff;
        }
        i.count = 0;
    }
}

void setupUBOs() {
    //------------ BEGIN UBOS ----------
    // setup UBs
    // Set up UBO binding numbers
    matriciesUniformBuffer.blockBinding = 0;
    materialUniformBuffer.blockBinding = 1;
    lightUniformBuffer.blockBinding = 2;
    fluidUniformBuffer.blockBinding = 4;

    // Set up UBO name orders
    const GLchar *matrixNames[] = {"Matricies.modelView", "Matricies.view", "Matricies.projection", "Matricies.normal",
                                   "Matricies.viewPort"};
    const GLchar *lightNames[] = {"Light.diffuse", "Light.specular", "Light.ambient", "Light.position"};
    const GLchar *materialNames[] = {"Material.diffuse", "Material.specular", "Material.shininess", "Material.ambient"};
    const GLchar *fluidNames[] = {"FluidDynamics.maxParticles", "FluidDynamics.mapSize", "FluidDynamics.supportRadius",
                                  "FluidDynamics.dt", "FluidDynamics.maxNeighbors", "FluidDynamics.solverIters",
                                  "FluidDynamics.restDensity", "FluidDynamics.epsilon",
                                  "FluidDynamics.collisionEpsilon", "FluidDynamics.kpoly", "FluidDynamics.kspiky",
                                  "FluidDynamics.scorr", "FluidDynamics.dcorr", "FluidDynamics.pcorr",
                                  "FluidDynamics.kxsph", "FluidDynamics.vortEpsilon", "FluidDynamics.time"};

    // get block offsets
    matriciesUniformBuffer.offsets = phongProgram->getUniformBlockOffsets("Matricies", matrixNames);
    lightUniformBuffer.offsets = phongProgram->getUniformBlockOffsets("Light", lightNames);
    materialUniformBuffer.offsets = phongProgram->getUniformBlockOffsets("Material", materialNames);
    fluidUniformBuffer.offsets = fluidUpdateProgram->getUniformBlockOffsets("FluidDynamics", fluidNames);

    // get block size
    matriciesUniformBuffer.blockSize = phongProgram->getUniformBlockSize("Matricies");
    lightUniformBuffer.blockSize = phongProgram->getUniformBlockSize("Light");
    materialUniformBuffer.blockSize = phongProgram->getUniformBlockSize("Material");
    fluidUniformBuffer.blockSize = fluidUpdateProgram->getUniformBlockSize("FluidDynamics");

    // Create UBO buffers and bind
    // Matrix Buffer
    glGenBuffers(1, &matriciesUniformBuffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, matriciesUniformBuffer.handle);
    // Buffer null data
    glBufferData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.blockSize, NULL, GL_DYNAMIC_DRAW);
    // Set the UBO's base
    glBindBufferBase(GL_UNIFORM_BUFFER, matriciesUniformBuffer.blockBinding, matriciesUniformBuffer.handle);
    // Set block binding and buffer base for each program that uses the buffer
    glUniformBlockBinding(phongProgram->getShaderProgramHandle(), phongProgram->getUniformBlockIndex("Matricies"),
                          matriciesUniformBuffer.blockBinding);
    glUniformBlockBinding(particleProgram->getShaderProgramHandle(), particleProgram->getUniformBlockIndex("Matricies"),
                          matriciesUniformBuffer.blockBinding);
    glUniformBlockBinding(fluidUpdateProgram->getShaderProgramHandle(),
                          fluidUpdateProgram->getUniformBlockIndex("Matricies"), matriciesUniformBuffer.blockBinding);

    // Light Buffer
    glGenBuffers(1, &lightUniformBuffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, lightUniformBuffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, lightUniformBuffer.blockSize, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, lightUniformBuffer.blockBinding, lightUniformBuffer.handle);
    glBufferSubData(GL_UNIFORM_BUFFER, lightUniformBuffer.offsets[0], sizeof(float) * 4,
                    matReader.getSwatch(LIGHT_MATERIAL).diffuse);
    glBufferSubData(GL_UNIFORM_BUFFER, lightUniformBuffer.offsets[1], sizeof(float) * 4,
                    matReader.getSwatch(LIGHT_MATERIAL).specular);
    glBufferSubData(GL_UNIFORM_BUFFER, lightUniformBuffer.offsets[2], sizeof(float) * 4,
                    matReader.getSwatch(LIGHT_MATERIAL).ambient);
    glBufferSubData(GL_UNIFORM_BUFFER, lightUniformBuffer.offsets[3], sizeof(float) * 3, &(lightPos)[0]);
    glUniformBlockBinding(phongProgram->getShaderProgramHandle(), phongProgram->getUniformBlockIndex("Light"),
                          lightUniformBuffer.blockBinding);
    glUniformBlockBinding(particleProgram->getShaderProgramHandle(), particleProgram->getUniformBlockIndex("Light"),
                          lightUniformBuffer.blockBinding);

    // Material Buffer
    glGenBuffers(1, &materialUniformBuffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, materialUniformBuffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, materialUniformBuffer.blockSize, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, materialUniformBuffer.blockBinding, materialUniformBuffer.handle);
    glUniformBlockBinding(phongProgram->getShaderProgramHandle(), phongProgram->getUniformBlockIndex("Material"),
                          materialUniformBuffer.blockBinding);

    // Fluid Buffer
    glGenBuffers(1, &fluidUniformBuffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, fluidUniformBuffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, fluidUniformBuffer.blockSize, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, fluidUniformBuffer.blockBinding, fluidUniformBuffer.handle);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[0], sizeof(GLuint), &NUM_PARTICLES);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[1], sizeof(GLuint), &HASH_MAP_SIZE);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[2], sizeof(GLfloat), &supportRad);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[3], sizeof(GLfloat), &MAX_DELTA_T);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[4], sizeof(GLuint), &MAX_NEIGHBORS);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[5], sizeof(GLuint), &SOLVER_ITERS);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[6], sizeof(GLfloat), &restDensity);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[7], sizeof(GLfloat), &epsilon);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[8], sizeof(GLfloat), &COLLISION_EPSILON);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[9], sizeof(GLfloat), &KPOLY);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[10], sizeof(GLfloat), &KSPIKY);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[11], sizeof(GLfloat), &SCORR);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[12], sizeof(GLfloat), &DCORR);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[13], sizeof(GLint), &PCORR);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[14], sizeof(GLfloat), &KXSPH);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[15], sizeof(GLfloat), &VORT_EPSILON);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[16], sizeof(GLfloat), &simTime);
    glUniformBlockBinding(fluidUpdateProgram->getShaderProgramHandle(),
                          fluidUpdateProgram->getUniformBlockIndex("FluidDynamics"), fluidUniformBuffer.blockBinding);

    //------------ END UBOS ----------
}

void setupSSBOs() {
    //------------ START SSBOs --------
    /// Index SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &particleSSBOs.index);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBOs.index);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.index, particleSSBOs.index);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * NUM_PARTICLES, NULL, GL_DYNAMIC_DRAW);
    GLint bufMask = GL_MAP_WRITE_BIT;
    GLuint *indices = (GLuint *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * NUM_PARTICLES, bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++) {
        indices[i] = particleData.idx[i];
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    /// Position SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &particleSSBOs.position);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBOs.position);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.position, particleSSBOs.position);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(float) * NUM_PARTICLES, NULL, GL_DYNAMIC_DRAW);
    Float4 *positions = (Float4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(float) * NUM_PARTICLES,
                                                    bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++) {
        positions[i].x = particleData.position[i].x;
        positions[i].y = particleData.position[i].y;
        positions[i].z = particleData.position[i].z;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    /// Updated Position SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &particleSSBOs.positionStar);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBOs.positionStar);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.positionStar, particleSSBOs.positionStar);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(float) * NUM_PARTICLES, NULL, GL_DYNAMIC_DRAW);
    Float4 *positionStars = (Float4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(float) * NUM_PARTICLES,
                                                        bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++) {
        positionStars[i].x = particleData.position[i].x;
        positionStars[i].y = particleData.position[i].y;
        positionStars[i].z = particleData.position[i].z;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    /// Velocity SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &particleSSBOs.velocity);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBOs.velocity);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.velocity, particleSSBOs.velocity);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(float) * NUM_PARTICLES, NULL, GL_DYNAMIC_DRAW);
    Float4 *vels = (Float4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(float) * NUM_PARTICLES, bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++) {
        vels[i].x = particleData.velocity[i].x;
        vels[i].y = particleData.velocity[i].y;
        vels[i].z = particleData.velocity[i].z;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    /// Lamda SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &particleSSBOs.lambda);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBOs.lambda);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.lambda, particleSSBOs.lambda);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * NUM_PARTICLES, NULL, GL_DYNAMIC_DRAW);

    /// Color SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &particleSSBOs.color);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBOs.color);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.color, particleSSBOs.color);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(float) * NUM_PARTICLES, NULL, GL_DYNAMIC_DRAW);
    Float4 *colors = (Float4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(float) * NUM_PARTICLES,
                                                 bufMask);
    for (int i = 0; i < NUM_PARTICLES; i++) {
        colors[i].x = particleData.color[i].x;
        colors[i].y = particleData.color[i].y;
        colors[i].z = particleData.color[i].z;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    /// Hash SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &neighborSSBOs.hashMap);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, neighborSSBOs.hashMap);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.hashMap, neighborSSBOs.hashMap);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(HashType) * HASH_MAP_SIZE, NULL, GL_DYNAMIC_DRAW);

    /// HashClear SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &neighborSSBOs.hashClear);
    glBindBuffer(GL_COPY_READ_BUFFER, neighborSSBOs.hashClear);
    glBufferData(GL_COPY_READ_BUFFER, sizeof(HashType) * HASH_MAP_SIZE, hashClear, GL_STATIC_COPY);

    /// Linked List SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &neighborSSBOs.linkedList);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, neighborSSBOs.linkedList);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.linkedList, neighborSSBOs.linkedList);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NodeType) * NUM_PARTICLES, listClear, GL_DYNAMIC_DRAW);

    /// ListClear SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &neighborSSBOs.listClear);
    glBindBuffer(GL_COPY_READ_BUFFER, neighborSSBOs.listClear);
    glBufferData(GL_COPY_READ_BUFFER, sizeof(NodeType) * NUM_PARTICLES, listClear, GL_STATIC_COPY);

    /// Neighbor Data SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &neighborSSBOs.neighborData);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, neighborSSBOs.neighborData);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.neighbors, neighborSSBOs.neighborData);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NeighborType) * NUM_PARTICLES, neighborData, GL_DYNAMIC_DRAW);

    /// Atomic SSBO
    // generate, bind, and buffer data
    glGenBuffers(1, &neighborSSBOs.counter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, neighborSSBOs.counter);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, fluidSSBOLocs.counter, neighborSSBOs.counter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);


    //------------ END SSBOs --------
}

void setupVAOs() {
    // generate our vertex array object descriptors
    glGenVertexArrays(3, vaods);
    // will be used to store VBO descriptors for ARRAY_BUFFER and ELEMENT_ARRAY_BUFFER
    GLuint vbods[2];

    //------------ BEGIN PARTICLE VAO ------------
    // generate our vertex buffer object descriptors for the GROUND
    glBindVertexArray(vaods[PARTICLES]);
    // bind the VBO to our particle position ssbo
    glBindBuffer(GL_ARRAY_BUFFER, particleSSBOs.position);
    // enable our position attribute
    glEnableVertexAttribArray(particleShaderAttribLocs.position);
    // map the position attribute to data within our buffer
    glVertexAttribPointer(particleShaderAttribLocs.position, 4, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    // bind the VBO to our particle color ssbo
    glBindBuffer(GL_ARRAY_BUFFER, particleSSBOs.color);
    // enable our color attribute
    glEnableVertexAttribArray(particleShaderAttribLocs.color);
    // map the color attribute to data within our buffer
    glVertexAttribPointer(particleShaderAttribLocs.color, 4, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    // bind the VBO to our particle velocity ssbo
    glBindBuffer(GL_ARRAY_BUFFER, particleSSBOs.velocity);
    // enable our velocity attribute
    glEnableVertexAttribArray(particleShaderAttribLocs.velocity);
    // map the velocity attribute to data within our buffer
    glVertexAttribPointer(particleShaderAttribLocs.velocity, 4, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    // bind the VBO to our particle index ssbo
    glBindBuffer(GL_ARRAY_BUFFER, particleSSBOs.index);
    // enable our index attribute
    glEnableVertexAttribArray(particleShaderAttribLocs.index);
    // map the index attribute to data within our buffer
    glVertexAttribIPointer(particleShaderAttribLocs.index, 1, GL_UNSIGNED_INT, 0, (void *) 0);
    //------------  END  PARTICLE VAO------------

    //------------ BEGIN LIGHT VAO ------------
    // Draw Ground
    glBindVertexArray(vaods[LIGHT]);

    // generate our vertex buffer object descriptors for the LIGHT
    glGenBuffers(1, &lightVbod);
    // bind the VBO for our Ground Array Buffer
    glBindBuffer(GL_ARRAY_BUFFER, lightVbod);
    // send the data to the GPU
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    // enable our position attribute
    glEnableVertexAttribArray(grndShaderAttribLocs.position);
    // map the position attribute to data within our buffer
    glVertexAttribPointer(grndShaderAttribLocs.position, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *) 0);
    //------------  END  LIGHT VAO------------

    //------------ BEGIN GROUND VAO ------------
    // Draw Ground
    glBindVertexArray(vaods[GROUND]);

    // generate our vertex buffer object descriptors for the GROUND
    glGenBuffers(2, vbods);
    // bind the VBO for our Ground Array Buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbods[0]);
    // send the data to the GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

    // bind the VBO for our Ground Element Array Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbods[1]);
    // send the data to the GPU
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), groundIndices, GL_STATIC_DRAW);

    // enable our position attribute
    glEnableVertexAttribArray(grndShaderAttribLocs.position);
    // map the position attribute to data within our buffer
    glVertexAttribPointer(grndShaderAttribLocs.position, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *) 0);

    // enable our normal attribute
    glEnableVertexAttribArray(grndShaderAttribLocs.normal);
    // map the normal attribute to data within our buffer
    glVertexAttribPointer(grndShaderAttribLocs.normal, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6,
                          (void *) (3 * sizeof(float)));
    //------------  END  GROUND VAO------------

    //------------  BEGIN SPHERE VAO    ------------
    // SOURCE: http://www.songho.ca/opengl/gl_sphere.html
    std::vector<float> vertices;
    std::vector<float> normals;

    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / SPHERE_RADIUS;    // vertex normal

    float sectorStep = 2 * M_PI / SPHERE_SECTORS;
    float stackStep = M_PI / SPHERE_STACKS;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= SPHERE_STACKS; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = SPHERE_RADIUS * cosf(stackAngle);             // r * cos(u)
        z = SPHERE_RADIUS * sinf(stackAngle);              // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for (int j = 0; j <= SPHERE_SECTORS; ++j) {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // normalized vertex normal (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);
        }
    }


    int k1, k2;
    for (int i = 0; i < SPHERE_STACKS; ++i) {
        k1 = i * (SPHERE_SECTORS + 1);     // beginning of current stack
        k2 = k1 + SPHERE_SECTORS + 1;      // beginning of next stack

        for (int j = 0; j < SPHERE_SECTORS; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if (i != (SPHERE_STACKS - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    // generate our vertex array object descriptors
    glGenVertexArrays(1, &sphereAttributes.vaod);
    glBindVertexArray(sphereAttributes.vaod);

    // Position data
    glGenBuffers(1, &sphereAttributes.vbodPos);
    // bind the VBO for our Sphere Array Buffer
    glBindBuffer(GL_ARRAY_BUFFER, sphereAttributes.vbodPos);
    // send the data to the GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
    // enable our position attribute
    glEnableVertexAttribArray(sphereAttribLocs.position);
    // map the position attribute to data within our buffer
    glVertexAttribPointer(sphereAttribLocs.position, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *) 0);

    // Normal data
    glGenBuffers(1, &sphereAttributes.vbodNormal);
    glBindBuffer(GL_ARRAY_BUFFER, sphereAttributes.vbodNormal);
    // send the data to the GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * normals.size(), &normals[0], GL_STATIC_DRAW);
    // enable our normal attribute
    glEnableVertexAttribArray(sphereAttribLocs.normal);
    // map the normal attribute to data within our buffer
    glVertexAttribPointer(sphereAttribLocs.normal, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *) 0);

    // Color data
    // Use the particle color for the model color
    glBindBuffer(GL_ARRAY_BUFFER, particleSSBOs.color);
    // enable vertex attribute
    glEnableVertexAttribArray(sphereAttribLocs.color);
    glVertexAttribPointer(sphereAttribLocs.color, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *) 0);
    glVertexAttribDivisor(sphereAttribLocs.color, 1);

    // Position data
    // Use the particle position vector for the model offset
    glBindBuffer(GL_ARRAY_BUFFER, particleSSBOs.position);
    // enable vertex attribute
    glEnableVertexAttribArray(sphereAttribLocs.modelOffset);
    glVertexAttribPointer(sphereAttribLocs.modelOffset, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *) 0);
    glVertexAttribDivisor(sphereAttribLocs.modelOffset, 1);

    // bind the VBO for our Sphere Element Array Buffer
    glGenBuffers(1, &sphereAttributes.vbodIndex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereAttributes.vbodIndex);
    // send the data to the GPU
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), &indices[0], GL_STATIC_DRAW);
    //------------  END SPHERE VAO ------------
}

// load in our model data to VAOs and VBOs
void setupBuffers() {
    // Load data in for material reader
    matReader.loadMaterials("materials.mat");
    // UBOs
    setupUBOs();
    // SSBOs
    setupSSBOs();
    // VAOs
    setupVAOs();
}

void setupFonts() {
    FT_Library ft;

    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init freetype library\n");
        exit(EXIT_FAILURE);
    }

    if (FT_New_Face(ft, "fonts/DroidSansMono.ttf", 0, &face)) {
        fprintf(stderr, "Could not open font\n");
        exit(EXIT_FAILURE);
    }

    FT_Set_Pixel_Sizes(face, 0, 20);

    FT_GlyphSlot g = face->glyph;
    GLuint w = 0;
    GLuint h = 0;

    for (int i = 32; i < 128; i++) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "Loading character %c failed!\n", i);
            continue;
        }

        w += g->bitmap.width;
        h = (h > g->bitmap.rows ? h : g->bitmap.rows);
    }

    /* you might as well save this value as it is needed later on */
    atlas_width = w;
    atlas_height = h;

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &font_texture_handle);
    glBindTexture(GL_TEXTURE_2D, font_texture_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    GLint x = 0;

    for (int i = 32; i < 128; i++) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER))
            continue;

        font_characters[i].ax = g->advance.x >> 6;
        font_characters[i].ay = g->advance.y >> 6;

        font_characters[i].bw = g->bitmap.width;
        font_characters[i].bh = g->bitmap.rows;

        font_characters[i].bl = g->bitmap_left;
        font_characters[i].bt = g->bitmap_top;

        font_characters[i].tx = (float) x / w;

        glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE,
                        g->bitmap.buffer);

        x += g->bitmap.width;
    }

    glGenVertexArrays(1, &text_vao_handle);
    glBindVertexArray(text_vao_handle);

    glGenBuffers(1, &text_vbo_handle);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo_handle);
    glEnableVertexAttribArray(textShaderAttribLocs.text_texCoord_location);
    glVertexAttribPointer(textShaderAttribLocs.text_texCoord_location, 4, GL_FLOAT, GL_FALSE, 0, (void *) 0);
}

void debugSpacialHash() {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, neighborSSBOs.hashMap);
    GLint bufMask = GL_MAP_READ_BIT;
    HashType *hash = (HashType *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(HashType) * HASH_MAP_SIZE,
                                                   bufMask);
    /*
    int h = 0;
    for (int i = 0; i < HASH_MAP_SIZE; i++) {
        hashMap[i] = hash[i];
        if (hashMap[i].headNodeIndex != 0xffffffff)
            h++;
    }
    */
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    //printf("Hash Cells Used: %d\n", h);


    glBindBuffer(GL_SHADER_STORAGE_BUFFER, neighborSSBOs.linkedList);
    /*
    NodeType *list = (NodeType *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(NodeType) * NUM_PARTICLES,
                                                   bufMask);
    int l = 0;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        linkedList[i] = list[i];
        if (linkedList[i].particleIndex > NUM_PARTICLES) {
            l++;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    printf("Invalid Particle Indices: %d\n", l);

    int invalidCount = 0;
    int maxNumHashCell = 0;
    int maxCount = 0;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        // Follow linked list, make sure list ends
        int count = 0;
        uint j = 0;
        NodeType n = linkedList[i];
        while (j < NUM_PARTICLES && count < NUM_PARTICLES) {
            // Exit on null
            if (n.nextNodeIndex == 0xffffffff) {
                break;
            } else {
                // Or go to next node
                j = n.nextNodeIndex;
                n = linkedList[n.nextNodeIndex];
            }
            count++;
        }

        if (count > NUM_PARTICLES) {
            // Count how many particles have too many neighbors
            invalidCount++;
        } else if (count > maxNumHashCell) {
            // Find the maximum valid number of neighbors a particle has
            maxNumHashCell = count;
            // Reset the number of particles with that many neighbors
            maxCount = 1;
        } else if (count == maxNumHashCell) {
            // Count how many particles have the maximum number of neighbors
            maxCount++;
        }

    }
    printf("Infinite Loops Found In Linked List: %d\n", invalidCount);
    printf("Max Number in Hash Cell (One Hash Cell): %d\n", maxNumHashCell);
    printf("Max Number in Hash Cell Count (One Hash Cell): %d\n", maxCount);
    */
}

void debugNeighborFind() {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, neighborSSBOs.neighborData);
    GLint bufMask = GL_MAP_READ_BIT;
    auto *neighborList = (NeighborType *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                           sizeof(NeighborType) * NUM_PARTICLES, bufMask);
    int invalidCount = 0, maxCount = 0;
    int maxNeighbors = 0;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        if (neighborList[i].count > MAX_NEIGHBORS) {
            invalidCount++;
        } else if (neighborList[i].count == MAX_NEIGHBORS) {
            maxCount++;
        }
        if (neighborList[i].count > maxNeighbors) {
            maxNeighbors = neighborList[i].count;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    printf("Invalid Neighbor Counts: %d\n", invalidCount);
    printf("Actual Max Neighbor Counts: %d\n", maxCount);
    printf("Max Number of Neighbors: %d\n\n", maxNeighbors);
}

//*************************************************************************************

// Rendering

void render_text(const char *text, FT_Face face, float x, float y, float sx, float sy) {
    struct point {
        GLfloat x;
        GLfloat y;
        GLfloat s;
        GLfloat t;
    } coords[6 * strlen(text)];

    GLint n = 0;

    for (const char *p = text; *p; p++) {
        int characterIndex = (int) *p;

        character_info character = font_characters[characterIndex];

        GLfloat x2 = x + character.bl * sx;
        GLfloat y2 = -y - character.bt * sy;
        GLfloat w = character.bw * sx;
        GLfloat h = character.bh * sy;

        /* Advance the cursor to the start of the next character */
        x += character.ax * sx;
        y += character.ay * sy;

        /* Skip glyphs that have no pixels */
        if (!w || !h)
            continue;

        coords[n++] = (point) {x2, -y2, character.tx, 0};
        coords[n++] = (point) {x2 + w, -y2, character.tx + character.bw / atlas_width, 0};
        coords[n++] = (point) {x2, -y2 - h, character.tx, character.bh /
                                                          atlas_height}; //remember: each glyph occupies a different amount of vertical space

        coords[n++] = (point) {x2 + w, -y2, character.tx + character.bw / atlas_width, 0};
        coords[n++] = (point) {x2, -y2 - h, character.tx, character.bh / atlas_height};
        coords[n++] = (point) {x2 + w, -y2 - h, character.tx + character.bw / atlas_width, character.bh / atlas_height};
    }
    glBindVertexArray(text_vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo_handle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, n);
}


void fluidUpdate() {
    /***** TIME AND TIMESTAMP *****/
    double time = glfwGetTime();
    float dt = time - lastTime;
    lastTime = time;

    if (dt > MAX_DELTA_T) {
        dt = MAX_DELTA_T;
        simTime += dt;
    }

    // compute perspective and view for neighbor/ signed distance field rendering
    glm::mat4 opMtx, ovMtx, mMtx;
    mMtx = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    opMtx = glm::ortho(-1000.0, 1000.0, -1000.0, 1000.0, 0.01, 1000.0);
    ovMtx = glm::lookAt(glm::vec3(0.0, 500.0, 0.1), glm::vec3(0.0, 0.0, 0.0), upVector);
    // precompute this modelview matrix
    glm::mat4 omvMtx = ovMtx * mMtx;


    /***** WATER PARTICLES *****/
    /// Buffers
    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.position, particleSSBOs.position);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.color, particleSSBOs.color);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.velocity, particleSSBOs.velocity);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.index, particleSSBOs.index);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.hashMap, neighborSSBOs.hashMap);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, fluidSSBOLocs.linkedList, neighborSSBOs.linkedList);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, fluidSSBOLocs.counter, neighborSSBOs.counter);

    // Clear buffer data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, neighborSSBOs.hashMap);
    glBindBuffer(GL_COPY_READ_BUFFER, neighborSSBOs.hashClear);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, 0, sizeof(GLuint) * HASH_MAP_SIZE);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    GLuint zero = 0;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, neighborSSBOs.counter);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);

    // Buffer uniform data
    glBindBuffer(GL_UNIFORM_BUFFER, fluidUniformBuffer.handle);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[3], sizeof(GLfloat), &dt);

    /// Compute Neighbors
    // Set ortho matricies
    glBindBuffer(GL_UNIFORM_BUFFER, matriciesUniformBuffer.handle);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[0], sizeof(glm::mat4), &(omvMtx)[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[1], sizeof(glm::mat4), &(ovMtx)[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[2], sizeof(glm::mat4), &(opMtx)[0][0]);

    // Hash
    fluidUpdateProgram->useProgram();

    glBindVertexArray(vaods[PARTICLES]);
    glEnable(GL_RASTERIZER_DISCARD); // Disable rasterizing
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES); // Draw the particles#
    glMemoryBarrier(GL_ALL_BARRIER_BITS); // Make sure all data was processes
    glDisable(GL_RASTERIZER_DISCARD); // Renable rasterization

#if DEBUG
    debugSpacialHash();
    //debugNeighborFind();
#endif

    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBOs.position);
    //glBindBuffer(GL_COPY_READ_BUFFER, particleSSBOs.positionStar);
    //glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, 0, sizeof(Float4)*NUM_PARTICLES);
}

// handles drawing everything to our buffer
void renderScene(GLFWwindow *window) {
    // Update Fluid data
    for (int i = 0; i < SUBSTEPS; i++) {
        fluidUpdate();
    }


    /***** MATRICES *****/
    // query our current window size, determine the aspect ratio, and set our viewport size
    GLfloat ratio;
    ratio = windowWidth / (GLfloat) windowHeight;
    glViewport(0, 0, windowWidth, windowHeight);

    // create our Model, View, Projection, viewport matrices
    glm::mat4 mMtx, vMtx, pMtx, vpMtx;
    mMtx = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

    // compute our projection matrix
    pMtx = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
    // compute our view matrix based on our current camera setup
    vMtx = glm::lookAt(eyePoint, lookAtPoint, upVector);
    // Computer viewport matrix
    vpMtx = glm::mat4(
            glm::mat3(windowWidth / 2, 0.0f, windowWidth / 2.0f, 0.0f, windowHeight / 2.0f, windowHeight / 2.0f, 0, 0,
                      1));

    // precompute the modelview matrix
    glm::mat4 mvMtx = vMtx * mMtx;
    // precompute the normal matrix
    glm::mat4 nMtx = glm::transpose(glm::inverse(mvMtx));

    /// Draw particles
    // Matrices
    glBindBuffer(GL_UNIFORM_BUFFER, matriciesUniformBuffer.handle);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[0], sizeof(glm::mat4), &(mvMtx)[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[1], sizeof(glm::mat4), &(vMtx)[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[2], sizeof(glm::mat4), &(pMtx)[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[3], sizeof(glm::mat4), &(nMtx)[0][0]);

    particleProgram->useProgram();
    // bind our sphere VAO
    glBindVertexArray(sphereAttributes.vaod);
    // draw our sphere!
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, NUM_PARTICLES);

    /***** GROUND *****/
    // Set shader
    phongProgram->useProgram();
    // Matricies
    glBindBuffer(GL_UNIFORM_BUFFER, matriciesUniformBuffer.handle);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[0], sizeof(glm::mat4), &(mvMtx)[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[1], sizeof(glm::mat4), &(vMtx)[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[2], sizeof(glm::mat4), &(pMtx)[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, matriciesUniformBuffer.offsets[3], sizeof(glm::mat4), &(nMtx)[0][0]);

    // Material settings
    glBindBuffer(GL_UNIFORM_BUFFER, materialUniformBuffer.handle);
    glBufferSubData(GL_UNIFORM_BUFFER, materialUniformBuffer.offsets[0], sizeof(float) * 4,
                    matReader.getSwatch(FLOOR_MATERIAL).diffuse);
    glBufferSubData(GL_UNIFORM_BUFFER, materialUniformBuffer.offsets[1], sizeof(float) * 4,
                    matReader.getSwatch(FLOOR_MATERIAL).specular);
    glBufferSubData(GL_UNIFORM_BUFFER, materialUniformBuffer.offsets[2], sizeof(float),
                    matReader.getSwatch(FLOOR_MATERIAL).shininess);
    glBufferSubData(GL_UNIFORM_BUFFER, materialUniformBuffer.offsets[3], sizeof(float) * 4,
                    matReader.getSwatch(FLOOR_MATERIAL).ambient);
    // Light stuff
    glBindBuffer(GL_UNIFORM_BUFFER, lightUniformBuffer.handle);
    glBufferSubData(GL_UNIFORM_BUFFER, lightUniformBuffer.offsets[3], sizeof(float) * 3, &(lightPos)[0]);

    // bind our Ground VAO
    glBindVertexArray(vaods[GROUND]);
    // draw our ground!
    glDrawElements(GL_TRIANGLES, sizeof(groundIndices) / sizeof(unsigned short), GL_UNSIGNED_SHORT, (void *) 0);
}

static void updateParams() {
    static double time_last = 0;
    double dt = glfwGetTime() - time_last;

    // Update time
    glBindBufferBase(GL_UNIFORM_BUFFER, fluidUniformBuffer.blockBinding, fluidUniformBuffer.handle);
    glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[16], sizeof(GLfloat), &simTime);

    if (keys[GLFW_KEY_R]) {
        // increase rest density
        restDensity += ceil(100 * dt);
        glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[6], sizeof(GLfloat), &restDensity);
    }
    if (keys[GLFW_KEY_E]) {
        // decrease rest density
        if (restDensity > ceil(100 * dt)) {
            restDensity -= ceil(100 * dt);
            glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[6], sizeof(GLfloat), &restDensity);
        }
    }
    if (keys[GLFW_KEY_F]) {
        epsilon += ceil(100 * dt);
        glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[7], sizeof(GLfloat), &epsilon);
    }
    if (keys[GLFW_KEY_D]) {
        if (epsilon > ceil(100 * dt)) {
            epsilon -= ceil(100 * dt);
            glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[7], sizeof(GLfloat), &epsilon);
        }
    }
    if (keys[GLFW_KEY_Y]) {
        supportRad += dt / 100.0;
        kPoly = (315.0f / (64.0f * M_PI * pow(supportRad, 9)));
        kSpiky = -45.0f / (M_PI * pow(supportRad, 6));
        pressureRad = 0.1 * supportRad;
        dCorr = kPoly * pow(pow(supportRad, 2) - pow(pressureRad, 2), 3);
        glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[2], sizeof(GLfloat), &supportRad);
        glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[9], sizeof(GLfloat), &kPoly);
        glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[10], sizeof(GLfloat), &kSpiky);
        glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[12], sizeof(GLfloat), &dCorr);
    }
    if (keys[GLFW_KEY_T]) {
        if (supportRad > dt / 100.0) {
            supportRad -= dt / 100.0;
            kPoly = (315.0f / (64.0f * M_PI * pow(supportRad, 9)));
            kSpiky = -45.0f / (M_PI * pow(supportRad, 6));
            pressureRad = 0.1 * supportRad;
            dCorr = kPoly * pow(pow(supportRad, 2) - pow(pressureRad, 2), 3);
            glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[2], sizeof(GLfloat), &supportRad);
            glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[9], sizeof(GLfloat), &kPoly);
            glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[10], sizeof(GLfloat), &kSpiky);
            glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[12], sizeof(GLfloat), &dCorr);
        }
    }
    if (keys[GLFW_KEY_V]) {
        vEps += dt / 100.0;
        glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[15], sizeof(GLfloat), &vEps);
    }
    if (keys[GLFW_KEY_C]) {
        if (vEps > dt / 100.0) {
            vEps -= dt / 100.0;
            glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[15], sizeof(GLfloat), &vEps);
        }
    }
    if (keys[GLFW_KEY_8]) {
        sCorr += dt / 100.0;
        glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[11], sizeof(GLfloat), &sCorr);
    }
    if (keys[GLFW_KEY_7]) {
        if (sCorr > dt / 100.0) {
            sCorr -= dt / 100.0;
            glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[11], sizeof(GLfloat), &sCorr);
        }
    }
    if (keys[GLFW_KEY_5]) {
        kXsph += dt / 100.0;
        glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[14], sizeof(GLfloat), &kXsph);
    }
    if (keys[GLFW_KEY_4]) {
        if (kXsph > dt / 100.0) {
            kXsph -= dt / 100.0;
            glBufferSubData(GL_UNIFORM_BUFFER, fluidUniformBuffer.offsets[14], sizeof(GLfloat), &kXsph);
        }
    }


    time_last = glfwGetTime();
}

// program entry point
int main(int argc, char *argv[]) {
    GLFWwindow *window = setupGLFW();    // setup GLFW and get our window
    setupOpenGL();                        // setup OpenGL & GLEW
    setupShaders();                        // load our shader programs, uniforms, and attribtues
    setupPipelines();                   // build pipelines from the shader programs created in setupShaders()
    setupParticleData();
    setupBuffers();                        // load our models into GPU memory
    setupFonts();                        // load our fonts into memory

    convertSphericalToCartesian();        // position our camera in a pretty place

    lastTime = glfwGetTime();

    GLfloat ClockLastTime = glfwGetTime();
    GLuint nbFrames = 0;
    GLdouble fps = 0;
    std::deque<GLdouble> fpsAvgs(9);
    GLdouble fpsAvg = 0;

    // as long as our window is open
    while (!glfwWindowShouldClose(window)) {


        // clear the prior contents of our buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // render our scene
        renderScene(window);

        updateParams();

        // Measure speed
        GLdouble currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - ClockLastTime >= 0.33f) { // If last prinf() was more than 1 sec ago
            // printf and reset timer
            fps = GLdouble(nbFrames) / (currentTime - ClockLastTime);
            nbFrames = 0;
            ClockLastTime = currentTime;

            fpsAvgs.pop_front();
            fpsAvgs.push_back(fps);

            GLdouble totalFPS = 0;
            for (GLuint i = 0; i < fpsAvgs.size(); i++) {
                totalFPS += fpsAvgs.at(i);
            }
            fpsAvg = totalFPS / fpsAvgs.size();
        }

        glBindVertexArray(text_vao_handle);
        glBindTexture(GL_TEXTURE_2D, font_texture_handle);

        textShaderProgram->useProgram();

        glm::mat4 mvp = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

        glUniformMatrix4fv(textShaderUniformLocs.text_mvp_location, 1, GL_FALSE, &mvp[0][0]);

        GLfloat white[4] = {1, 1, 1, 1};
        glUniform4fv(textShaderUniformLocs.text_color_location, 1, white);

        GLfloat sx = 2.0 / (GLfloat) windowWidth;
        GLfloat sy = 2.0 / (GLfloat) windowHeight;

        char fpsStr[80];
        sprintf(fpsStr, "%.3f frames/sec (Avg: %.3f)", fps, fpsAvg);
        render_text(fpsStr, face, -1 + 8 * sx, 1 - 30 * sy, sx, sy);

        char restStr[100];
        int den = restDensity;
        sprintf(restStr, "(-e/r+) Rest Density: %d", den);
        render_text(restStr, face, -1 + 8 * sx, 1 - 50 * sy, sx, sy);

        char epsStr[100];
        int eps = epsilon;
        sprintf(epsStr, "(-d/f+) Epsilon: %d", eps);
        render_text(epsStr, face, -1 + 8 * sx, 1 - 70 * sy, sx, sy);

        char supportStr[100];
        sprintf(supportStr, "(-t/y+) Support Radius: %f", supportRad);
        render_text(supportStr, face, -1 + 8 * sx, 1 - 90 * sy, sx, sy);

        char vEpsStr[100];
        sprintf(vEpsStr, "(-c/v+) Vort Epsilon: %f", vEps);
        render_text(vEpsStr, face, -1 + 8 * sx, 1 - 110 * sy, sx, sy);

        char scoorStr[100];
        sprintf(scoorStr, "(-7/8+) Tensile Instability: %f", sCorr);
        render_text(scoorStr, face, -1 + 8 * sx, 1 - 130 * sy, sx, sy);

        char xsphStr[100];
        sprintf(xsphStr, "(-4/5+) XSPH: %f", kXsph);
        render_text(xsphStr, face, -1 + 8 * sx, 1 - 150 * sy, sx, sy);


        // swap the front and back buffers
        glfwSwapBuffers(window);
        // check for any events
        glfwPollEvents();

        // the following code is a hack for OSX Mojave
        // the window is initially black until it is moved
        // so instead of having the user manually move the window,
        // we'll automatically move it and then move it back
        if (!mackHack) {
            GLint xpos, ypos;
            glfwGetWindowPos(window, &xpos, &ypos);
            glfwSetWindowPos(window, xpos + 10, ypos + 10);
            glfwSetWindowPos(window, xpos, ypos);
            mackHack = true;
        }

    }

    // destroy our window
    glfwDestroyWindow(window);
    // end GLFW
    glfwTerminate();

    // delete our shader programs
    delete phongProgram;
    delete textShaderProgram;
    delete fluidUpdateProgram;
    delete particleProgram;

    // SUCCESS!!
    return EXIT_SUCCESS;
}
