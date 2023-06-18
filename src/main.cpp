#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>

#include <iostream>
#include <memory>

#include <scene.h>
#include <input_manager.h>
#include <camera.h>
#include <shader.h>
#include <object.h>
#include <gui.h>

void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void cursorMoveCallback(GLFWwindow *window, double xpos, double ypos);
void cursorScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void cursorClickCallback(GLFWwindow *window, int button, int action, int mods);

// settings
unsigned int WIDTH = 1600;
unsigned int HEIGHT = 900;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

int main()
{
    // init glfw
    glfwInit();
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_SAMPLES, 4);

    // create a window
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Graphics Engine", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    };

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, cursorMoveCallback);
    glfwSetMouseButtonCallback(window, cursorClickCallback);
    glfwSetScrollCallback(window, cursorScrollCallback);

    // turn off v-sync (unlimited fps)
    // glfwSwapInterval(0);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // init GUI context
    std::shared_ptr<Gui> gui = std::make_shared<Gui>(window);

    // init glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // set viewport and resize callback
    glViewport(0, 0, WIDTH, HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // create scene
    std::shared_ptr<Scene> scene = std::make_shared<Scene>(gui->sceneViewWidth, gui->sceneViewHeight, PREVIEW);

    // create input manager
    std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>(scene);
    glfwSetWindowUserPointer(window, inputManager.get());

    // camera
    std::shared_ptr<Camera> camera = std::make_shared<Camera>(glm::vec3(4.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);
    scene->addEye(camera);

    // objects
    std::shared_ptr<Object> cube = std::make_shared<Object>(std::string("Cube"), IMPORTED, "../models/simpleCube.obj");
    scene->addObject(cube);
    scene->selectObject(cube);
    std::shared_ptr<Object> cube2 = std::make_shared<Object>(std::string("Cube2"), MESH);
    cube2->translate(glm::vec3(0.0f, 0.0f, 2.0f));
    scene->addObject(cube2);
    // scene->selectObject(cube2);
    std::shared_ptr<Object> cube3 = std::make_shared<Object>(std::string("Cube3"), MESH);
    cube3->translate(glm::vec3(-2.0f, 0.0f, 0.0f));
    scene->addObject(cube3);

    // // lights
    // std::shared_ptr<Object> light = std::make_shared<Object>(std::string("Light"), LIGHT);
    // light->translate(glm::vec3(0.0f, 1.5f, 0.0f));
    // scene->addObject(light);

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        inputManager->processInput(window, deltaTime);

        // gui
        gui->draw(WIDTH, HEIGHT, inputManager, scene, deltaTime);

        // render scene
        scene->draw();

        // render gui in default framebuffer
        gui->render(WIDTH, HEIGHT);

        // swap the buffers and check and call events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // clean resources
    glfwTerminate();
    std::cout << "Terminating" << std::endl;
    return 0;
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    WIDTH = width;
    HEIGHT = height;
}

void cursorMoveCallback(GLFWwindow *window, double xpos, double ypos)
{
    InputManager *obj = static_cast<InputManager *>(glfwGetWindowUserPointer(window));
    obj->mouseMoveCallback(xpos, ypos);
}

void cursorScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    InputManager *obj = static_cast<InputManager *>(glfwGetWindowUserPointer(window));
    obj->mouseScrollCallback(xoffset, yoffset);
}

void cursorClickCallback(GLFWwindow *window, int button, int action, int mods)
{
    InputManager *obj = static_cast<InputManager *>(glfwGetWindowUserPointer(window));
    obj->mouseClickCallback(button, action, mods);
}
