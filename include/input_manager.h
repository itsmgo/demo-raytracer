#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <vector>
#include <map>
#include <scene.h>
#include <object.h>

enum Action
{
    NO_ACTION,
    GRAB,
    ROTATE,
    SCALE,
};

enum Axis
{
    NO_AXIS,
    X_AXIS,
    Y_AXIS,
    Z_AXIS,
};

class InputManager
{
public:
    std::shared_ptr<Scene> scene;

    // constructor
    InputManager(std::shared_ptr<Scene> scene)
    {
        std::cout << "holaInput" << std::endl;
        this->scene = scene;
        lastX = scene->width / 2.0f;
        lastY = scene->height / 2.0f;
    };

    void processInput(GLFWwindow *window, float deltaTime)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            scene->Eye->processKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            scene->Eye->processKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            scene->Eye->processKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            scene->Eye->processKeyboard(RIGHT, deltaTime);

        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
            key_m_pressed = true;
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE && key_m_pressed)
        {
            key_m_pressed = false;
            scene->swapModes();
        }

        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
            key_g_pressed = true;
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE && key_g_pressed)
        {
            key_g_pressed = false;
            if (action != GRAB)
            {
                cancelAction();
                action = GRAB;
                firstAction = true;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            key_s_pressed = true;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE && key_s_pressed)
        {
            key_s_pressed = false;
            if (action != SCALE)
            {
                cancelAction();
                action = SCALE;
                firstAction = true;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            key_r_pressed = true;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE && key_r_pressed)
        {
            key_r_pressed = false;
            if (action != ROTATE)
            {
                cancelAction();
                action = ROTATE;
                firstAction = true;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            key_x_pressed = true;

        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE && key_x_pressed)
        {
            key_x_pressed = false;
            if (action != NO_ACTION)
            {
                switch (lockAxis)
                {
                case X_AXIS:
                    lockAxis = NO_AXIS;
                    break;
                default:
                    lockAxis = X_AXIS;
                    break;
                }
            }
        }

        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            key_y_pressed = true;

        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_RELEASE && key_y_pressed)
        {
            key_y_pressed = false;
            if (action != NO_ACTION)
            {
                switch (lockAxis)
                {
                case Y_AXIS:
                    lockAxis = NO_AXIS;
                    break;
                default:
                    lockAxis = Y_AXIS;
                    break;
                }
            }
        }

        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            key_z_pressed = true;

        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE && key_z_pressed)
        {
            key_z_pressed = false;
            if (action != NO_ACTION)
            {
                switch (lockAxis)
                {
                case Z_AXIS:
                    lockAxis = NO_AXIS;
                    break;
                default:
                    lockAxis = Z_AXIS;
                    break;
                }
            }
        }
    };

    void mouseClickCallback(int button, int mouseAction, int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_MIDDLE && mouseAction == GLFW_PRESS)
        {
            key_middle_pressed = true;
        }
        if (button == GLFW_MOUSE_BUTTON_MIDDLE && mouseAction == GLFW_RELEASE)
        {
            key_middle_pressed = false;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT && mouseAction == GLFW_RELEASE)
        {
            if (action != NO_ACTION)
            {
                confirmAction();
            }
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && mouseAction == GLFW_RELEASE)
        {
            cancelAction();
        }
    };

    void mouseMoveCallback(float xpos, float ypos)
    {
        if (key_middle_pressed)
        {
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
            scene->Eye->processMouseMovement(xoffset, yoffset);
        }

        lastX = xpos;
        lastY = ypos;

        if (action == GRAB)
        {
            glm::vec3 rayOrigin = scene->Eye->WorldPosition;
            glm::vec3 rayDirection = scene->Eye->getMouseWorldRay(xpos, ypos);
            glm::vec3 planeNormal(-scene->Eye->WorldFront);
            glm::vec3 planePoint(0.0f, 0.0f, 0.0f);

            float intersectionDistance;
            bool intersects = glm::intersectRayPlane(rayOrigin, rayDirection, planePoint, planeNormal, intersectionDistance);

            if (intersects)
            {
                intersectionPoint = rayOrigin + intersectionDistance * rayDirection;
            }

            if (firstAction)
            {
                originPoint = intersectionPoint;
                for (auto &&object : scene->SelectedObjects)
                {
                    initialState[object->name] = object->location;
                }
                firstAction = false;
                return;
            }

            glm::vec3 translation = intersectionPoint - originPoint;

            switch (lockAxis)
            {
            case X_AXIS:
                translation = translation * glm::vec3(1.0f, 0.0f, 0.0f);
                break;
            case Y_AXIS:
                translation = translation * glm::vec3(0.0f, 1.0f, 0.0f);
                break;
            case Z_AXIS:
                translation = translation * glm::vec3(0.0f, 0.0f, 1.0f);
                break;
            default:
                break;
            }

            for (auto &&object : scene->SelectedObjects)
            {
                object->location = initialState[object->name] + translation;
            }
        }

        if (action == SCALE)
        {
            glm::vec3 rayOrigin = scene->Eye->WorldPosition;
            glm::vec3 rayDirection = scene->Eye->getMouseWorldRay(xpos, ypos);
            glm::vec3 planeNormal(-scene->Eye->WorldFront);
            glm::vec3 planePoint(0.0f, 0.0f, 0.0f);

            float intersectionDistance;
            bool intersects = glm::intersectRayPlane(rayOrigin, rayDirection, planePoint, planeNormal, intersectionDistance);

            if (intersects)
            {
                intersectionPoint = rayOrigin + intersectionDistance * rayDirection;
            }

            if (firstAction)
            {
                originPoint = intersectionPoint;
                for (auto &&object : scene->SelectedObjects)
                {
                    initialState[object->name] = object->scale;
                }
                firstAction = false;
                return;
            }

            float scaleFactor = std::pow(glm::length(intersectionPoint) / glm::length(originPoint), 1.3f);
            glm::vec3 scale = glm::vec3(1.0f);

            switch (lockAxis)
            {
            case X_AXIS:
                scale = glm::vec3(scaleFactor, 1.0f, 1.0f);
                break;
            case Y_AXIS:
                scale = glm::vec3(1.0f, scaleFactor, 1.0f);
                break;
            case Z_AXIS:
                scale = glm::vec3(1.0f, 1.0f, scaleFactor);
                break;
            default:
                scale = scaleFactor * glm::vec3(1.0f);
                break;
            }

            for (auto &&object : scene->SelectedObjects)
            {
                object->scale = initialState[object->name] * scale;
            }
        }

        if (action == ROTATE)
        {
            glm::vec3 rayOrigin = scene->Eye->WorldPosition;
            glm::vec3 rayDirection = scene->Eye->getMouseWorldRay(xpos, ypos);
            glm::vec3 planeNormal(-scene->Eye->WorldFront);
            glm::vec3 planePoint(0.0f, 0.0f, 0.0f);

            float intersectionDistance;
            bool intersects = glm::intersectRayPlane(rayOrigin, rayDirection, planePoint, planeNormal, intersectionDistance);

            if (intersects)
            {
                intersectionPoint = rayOrigin + intersectionDistance * rayDirection;
            }

            if (firstAction)
            {
                originPoint = intersectionPoint;
                for (auto &&object : scene->SelectedObjects)
                {
                    initialState[object->name] = object->rotation;
                }
                firstAction = false;
                return;
            }

            float angle = glm::orientedAngle(glm::normalize(originPoint), glm::normalize(intersectionPoint), glm::normalize(planeNormal));
            float rotationFactor = glm::degrees(angle);

            glm::vec3 eulerAngles;
            // Create the rotation matrix
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::normalize(-scene->Eye->WorldFront));

            // Extract individual rotations from the rotation matrix
            eulerAngles.x = glm::atan(rotationMatrix[1][2], rotationMatrix[2][2]);
            eulerAngles.y = glm::atan(-rotationMatrix[0][2], glm::sqrt(rotationMatrix[1][2] * rotationMatrix[1][2] + rotationMatrix[2][2] * rotationMatrix[2][2]));
            eulerAngles.z = glm::atan(rotationMatrix[0][1], rotationMatrix[0][0]);

            // Convert the angles to degrees
            glm::vec3 rotation = glm::degrees(eulerAngles);

            switch (lockAxis)
            {
            case X_AXIS:
                rotation = rotationFactor * glm::vec3(1.0f, 0.0f, 0.0f);
                break;
            case Y_AXIS:
                rotation = rotationFactor * glm::vec3(0.0f, 1.0f, 0.0f);
                break;
            case Z_AXIS:
                rotation = rotationFactor * glm::vec3(0.0f, 0.0f, -1.0f);
                break;
            }

            for (auto &&object : scene->SelectedObjects)
            {
                object->rotation = initialState[object->name] + rotation;
            }
        }
    };

    void mouseScrollCallback(double xoffset, double yoffset)
    {
        scene->Eye->processMouseScroll(static_cast<float>(yoffset));
    };

private:
    // keys
    bool key_m_pressed = false;
    bool key_g_pressed = false;
    bool key_s_pressed = false;
    bool key_r_pressed = false;
    bool key_x_pressed = false;
    bool key_y_pressed = false;
    bool key_z_pressed = false;

    // mouse
    bool key_middle_pressed = false;

    // state
    Action action = NO_ACTION;
    Axis lockAxis = NO_AXIS;

    // state variables
    float lastX, lastY;
    bool firstAction = false;
    glm::vec3 originPoint, intersectionPoint;
    std::map<std::string, glm::vec3> initialState;

    void recoverInitialState()
    {
        for (auto &&object : scene->SelectedObjects)
        {
            switch (action)
            {
            case GRAB:
                object->location = initialState[object->name];
                break;
            case ROTATE:
                object->rotation = initialState[object->name];
                break;
            case SCALE:
                object->scale = initialState[object->name];
                break;
            default:
                break;
            }
        }
    }

    void confirmAction()
    {
        action = NO_ACTION;
        lockAxis = NO_AXIS;
    }

    void cancelAction()
    {
        recoverInitialState();
        confirmAction();
    }
};
#endif
