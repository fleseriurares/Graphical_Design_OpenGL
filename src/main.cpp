#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 hatModel;
glm::mat4 catModel;
glm::mat4 modelWorld;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat3 hatNormalMatrix;
glm::mat3 catNormalMatrix;
glm::mat3 normalMatrixWorld;
glm::mat4 lightRotation;


// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
glm::vec3 lightColorGreen;

glm::vec3 pointLightPos = glm::vec3(-256.0f, -50.0f, -195.0f);
float constant = 1.0f;
float linear = 0.001f;
float quadratic = 0.0001f;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint useFogLoc;
GLint useUniformColorLoc;

GLuint shadowMapFBO;
GLuint depthMapTexture;


// camera
gps::Camera myCamera(
    glm::vec3(0.0f, -2.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 4.0f;

GLfloat lightAngle;

GLboolean pressedKeys[1024];

const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;

gps::SkyBox mySkyBox;
gps::Shader skyboxShader;
gps::Shader lightShader;
gps::Shader depthMapShader;



// models
gps::Model3D teapot;
gps::Model3D floatingHat;
gps::Model3D cat;
gps::Model3D world;
gps::Model3D lightCube;
gps::Model3D screenQuad;
GLfloat angle;
GLfloat angleFloatingHat;
GLfloat verticalFloating;



// shaders
gps::Shader myBasicShader;

gps::Shader screenQuadShader;

bool up = true;
bool cameraAnimation = false;
bool showDepthMap;

GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
    //TODO
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
        showDepthMap = !showDepthMap;

    if (key == GLFW_KEY_U && action == GLFW_PRESS) {
        static bool useUniformColor = false;
        useUniformColor = !useUniformColor;
        myBasicShader.useShaderProgram();

        glUniform1i(useUniformColorLoc, useUniformColor);
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        static bool useFog = false;
        useFog = !useFog;
        myBasicShader.useShaderProgram();

        glUniform1i(useFogLoc, useFog);
    }



}

int vol = 1;
bool firstMouse = true;
float lastX = 1024.0f / 2.0f; // Centru
float lastY = 768.0f / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float sensitivity = 0.1f;


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

    vol = 5;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;


    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;


    myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}


void processMovement() {
    if (pressedKeys[GLFW_KEY_W]) {
        printf("%f, %f, %f\n", myCamera.getPosition().x, myCamera.getPosition().y, myCamera.getPosition().z);

        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_B]) {
        cameraAnimation = true;
        vol = 1;
        myCamera.setPosition(glm::vec3(-157.0f, -24.0f, -1142.0f));
        //myCamera.setPosition(glm::vec3(-185.0f, 176.0f, 560.0f));
        myCamera.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
        yaw = 90.0f;
        pitch = 0.0f;
        myCamera.rotate(pitch, yaw);

        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (cameraAnimation) {
        if (vol == 1) {
            if (myCamera.getPosition().z < -526.0f) {
                myCamera.move(gps::MOVE_FORWARD, 0.4f);
            }
            else {
                vol = 2;
            }
        }
        else if (vol == 2) {
            if (yaw > 2.0f) {
                yaw -= 0.06f;
            }
            else if (myCamera.getPosition().x < -37.0f) {
                myCamera.move(gps::MOVE_FORWARD, 0.4f);
            }
            else {
                vol = 3;
            }
            pitch = 0.0f;
            myCamera.rotate(pitch, yaw);
        }
        else if (vol == 3) {
            if (yaw < 20.0f) {
                yaw += 0.03;
            }
            else if (yaw < 40.0f) {
                yaw += 0.1;
            }
            else if (yaw < 60.0f) {
                yaw += 0.2f;
            }
            else if (yaw < 70.0f) {
                yaw += 0.3f;
            }
            else if (yaw < 720.0f) {
                yaw += 5.0f;
            }
            else if (yaw < 1250.0f) {
                //myCamera.setPosition(glm::vec3(-166.0f, 175.0f, -303.0f));
                yaw += 5.0;

            }
            else {
                vol = 4;
            }
            pitch = 0.0f;
            myCamera.rotate(pitch, yaw);
        }
        else if (vol == 4) {
            printf("%f, %f, %f\n", myCamera.getPosition().x, myCamera.getPosition().y, myCamera.getPosition().z);

            float radius = 800.0f;
            float speed = 0.05f;


            yaw += speed;


            float x = radius * cos(glm::radians(yaw));
            float z = radius * sin(glm::radians(yaw));


            myCamera.setPosition(glm::vec3(x, 175.0f, z));


            myCamera.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));



        }
        else {
            cameraAnimation = false;
            yaw = 90.0f;
        }


        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    //constant movement
    angleFloatingHat += 0.05f;

    if (verticalFloating > 40.0f) {
        up = false;
    }
    if (verticalFloating < -40.0f) {
        up = true;
    }

    if (up) {
        verticalFloating += 0.1f;
    }
    else {
        verticalFloating -= 0.1f;
    }
}

void initSkybox() {
    std::vector<const GLchar*> faces;
    faces.push_back("skybox3/stormydays_rt.tga");
    faces.push_back("skybox3/stormydays_lf.tga");
    faces.push_back("skybox3/stormydays_up.tga");
    faces.push_back("skybox3/stormydays_dn.tga");
    faces.push_back("skybox3/stormydays_bk.tga");
    faces.push_back("skybox3/stormydays_ft.tga");
    mySkyBox.Load(faces);
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);

    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

}

void initOpenGLState() {
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face
    glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
 //   teapot.LoadModel("models/teapot/teapot20segUT.obj");
    world.LoadModel("models/Scena/good.obj");
    floatingHat.LoadModel("models/Scena/Obj/FloatingHat/floatingHat.obj");
    cat.LoadModel("models/Scena/Obj/cat/catPumpkin.obj");
    lightCube.LoadModel("models/Scena/Obj/LightCube/cube.obj");
    screenQuad.LoadModel("models/Scena/Obj/quad/quad.obj");
}

void initShaders() {
    myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();
    lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
    lightShader.useShaderProgram();
    screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
    screenQuadShader.useShaderProgram();
    depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
    depthMapShader.useShaderProgram();
}


void initFBO() {
    glGenFramebuffers(1, &shadowMapFBO);

    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

glm::mat4 computeLightSpaceTrMatrix() {
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    const GLfloat near_plane = 0.1f, far_plane = 2000.0f;
    glm::mat4 lightProjection = glm::ortho(-1000.0f, 1000.0f, -1000.0f, 1000.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

    return lightProjection * lightView;

}

void initUniforms() {
    myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");



    // get view matrix for current camera
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    // send view matrix to shader



    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    modelWorld = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

    normalMatrixWorld = glm::mat3(glm::inverseTranspose(view * model));

    useUniformColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "useUniformColor");
    glUniform1i(useUniformColorLoc, GL_FALSE);

     useFogLoc = glGetUniformLocation(myBasicShader.shaderProgram, "useFog");
    glUniform1i(useFogLoc, GL_FALSE);

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    // create projection matrix
    projection = glm::perspective(glm::radians(45.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.01f, 2000.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    // send projection matrix to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //set the light direction (direction towards the light)
    lightDir = glm::vec3(500.0f, 750.0f, 685.0f);
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

    //set light color
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
    lightColorGreen = glm::vec3(1.0f, 0.0f, 0.0f); //green light
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    // send light color to shader
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    GLuint pointLightPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.position");
    glUniform3fv(pointLightPosLoc, 1, glm::value_ptr(pointLightPos));

    GLuint pointLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.color");
    glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(lightColorGreen));

    GLuint constantLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.constant");
    glUniform1f(constantLoc, constant);

    GLuint linearLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.linear");
    glUniform1f(linearLoc, linear);

    GLuint quadraticLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight.quadratic");
    glUniform1f(quadraticLoc, quadratic);

    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


}

void renderTeapot(gps::Shader shader, bool depthPass) {
    // select active shader program
    shader.useShaderProgram();

    //send teapot model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    //send teapot normal matrix data to shader
   // glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw teapot
    teapot.Draw(shader);
}

void renderLightCube(gps::Shader shader) {
    shader.useShaderProgram();

    // Setarea matricei view
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glCheckError();

    // Setarea matricei model
    model = lightRotation;
    model = glm::translate(model, 1.0f * lightDir);
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glCheckError();

    // Apelul Draw
    //lightCube.Draw(shader);
    glCheckError();
}


void renderWorld(gps::Shader shader, bool depthPass) {
    // select active shader program
    shader.useShaderProgram();

    glm::mat4 scenaModel = glm::translate(glm::mat4(1.0f), glm::vec3(-200.0f, -200.0f, 1.0f));
    
    if (!depthPass) {
        
        GLint scenaModelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        glUniformMatrix4fv(scenaModelLoc, 1, GL_FALSE, glm::value_ptr(scenaModel));

        glm::mat3 scenaNormalMatrix = glm::mat3(glm::inverseTranspose(view * scenaModel));
        GLint scenaNormalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
        glUniformMatrix3fv(scenaNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(scenaNormalMatrix));

    }
    else {
        GLint scenaModelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        glUniformMatrix4fv(scenaModelLoc, 1, GL_FALSE, glm::value_ptr(scenaModel));
    }

    

    world.Draw(shader);
}

void renderFloatingHat(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    glm::vec3 hatPosition(-200.0f, 100.0f, 200.0f);

    hatModel = glm::translate(glm::mat4(1.0f), glm::vec3(30.0f, 0.0f, -220.0f));

    hatModel = glm::translate(hatModel, glm::vec3(0.0f, verticalFloating, 0.0f));
    hatModel = glm::translate(hatModel, hatPosition);
    hatModel = glm::rotate(hatModel, glm::radians(angleFloatingHat), glm::vec3(0, 1, 0));
    hatModel = glm::translate(hatModel, -hatPosition);

    
    if (!depthPass) {
        GLint hatModelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        glUniformMatrix4fv(hatModelLoc, 1, GL_FALSE, glm::value_ptr(hatModel));

        hatNormalMatrix = glm::mat3(glm::inverseTranspose(view * hatModel));
        GLint hatNormalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
    }
    else {
        GLint hatModelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        glUniformMatrix4fv(hatModelLoc, 1, GL_FALSE, glm::value_ptr(hatModel));
    }

  
   // glUniformMatrix3fv(hatNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(hatNormalMatrix));

    floatingHat.Draw(shader);
}

void renderCat(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glm::vec3 catPosition(0.0f, 0.0f, 0.0f);

    catModel = glm::translate(glm::mat4(1.0f), glm::vec3(-256.0f, -50.0f, -208.0f));
    catModel = glm::rotate(catModel, glm::radians(90.0f), glm::vec3(0, 1, 0));
    
    if (!depthPass) {
        GLint catModelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        glUniformMatrix4fv(catModelLoc, 1, GL_FALSE, glm::value_ptr(catModel));
        catNormalMatrix = glm::mat3(glm::inverseTranspose(view * catModel));
        GLint catNormalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
        glUniformMatrix3fv(catNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(catNormalMatrix));

    }
    else {
        GLint catModelLoc = glGetUniformLocation(shader.shaderProgram, "model");
        glUniformMatrix4fv(catModelLoc, 1, GL_FALSE, glm::value_ptr(catModel));
    }


  
   // glUniformMatrix3fv(catNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(catNormalMatrix));



    cat.Draw(shader);
}

void renderSkyBox() {
   // view = myCamera.getViewMatrix();
    //glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    mySkyBox.Draw(skyboxShader, view, projection);

}

void drawObjects(gps::Shader shader, bool depthPass) {
    // render the teapot
    //renderTeapot(shader, depthPass);
    renderWorld(shader, depthPass);
    renderFloatingHat(shader, depthPass);
    renderCat(shader, depthPass);
    // renderLightCube(myBasicShader);
    renderSkyBox();

}

void renderScene() {
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 lightSpaceTrMatrix = computeLightSpaceTrMatrix();

    depthMapShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    drawObjects(depthMapShader, true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // render depth map on screen - toggled with the M key

    if (showDepthMap) {
        glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

        glClear(GL_COLOR_BUFFER_BIT);

        screenQuadShader.useShaderProgram();

        //bind the depth map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

        glDisable(GL_DEPTH_TEST);
        screenQuad.Draw(screenQuadShader);
        glEnable(GL_DEPTH_TEST);
    }
    else {

        // final scene rendering pass (with shadows)

        glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        myBasicShader.useShaderProgram();

        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


        lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));

        glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

        //glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));

        //bind the shadow map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(computeLightSpaceTrMatrix()));

           lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
           glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));



        drawObjects(myBasicShader, false);

        //draw a white cube around the light

        lightShader.useShaderProgram();

        glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        model = lightRotation;
        model = glm::translate(model, 1.0f * lightDir);
        model = glm::scale(model, glm::vec3(5.5f, 5.5f, 5.5f));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        lightCube.Draw(lightShader);
    }
}



void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char* argv[]) {

    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initModels();
    initShaders();
    initUniforms();
    initSkybox();
    setWindowCallbacks();
    initFBO();

    glCheckError();
    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}