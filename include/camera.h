

#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 1.0f;
const float SENSITIVITY = 0.09f;
const float ZOOM = 45.0f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 PivotPosition = glm::vec3(0.0f);
    glm::vec3 LocalPosition;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 AbsoluteUp;
    glm::vec3 WorldPosition;
    glm::vec3 WorldUp;
    glm::vec3 WorldFront;
    glm::vec3 WorldRight;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    // viewport settings
    int Width, Height;

    bool updated = true;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH,
           int width = 500,
           int height = 500) : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
                               MovementSpeed(SPEED),
                               MouseSensitivity(SENSITIVITY),
                               Zoom(ZOOM)
    {
        std::cout << "holaCam" << std::endl;
        LocalPosition = position;
        AbsoluteUp = up;
        Yaw = yaw;
        Pitch = pitch;
        Width = width;
        Height = height;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 getViewMatrix()
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, PivotPosition);
        model = glm::rotate(model, glm::radians(Pitch), Right);
        model = glm::rotate(model, glm::radians(Yaw), AbsoluteUp);
        return glm::lookAt(LocalPosition, LocalPosition + Front, Up) * model;
    }

    glm::mat4 getProjectionMatrix()
    {
        return glm::perspective(glm::radians(Zoom), (float)Width / (float)Height, 0.1f, 100.0f);
    }

    glm::vec3 getMouseWorldRay(float mouseX, float mouseY)
    {
        mouseX = mouseX / (Width * 0.5f) - 1.0f;
        mouseY = mouseY / (Height * 0.5f) - 1.0f;

        glm::mat4 proj = getProjectionMatrix();
        glm::mat4 view = getViewMatrix();

        glm::mat4 invVP = glm::inverse(proj * view);
        glm::vec4 screenPos = glm::vec4(mouseX, -mouseY, 1.0f, 1.0f);
        glm::vec4 worldPos = invVP * screenPos;

        glm::vec3 dir = glm::normalize(glm::vec3(worldPos));

        return dir;
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void processKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            LocalPosition += Up * velocity;
        if (direction == BACKWARD)
            LocalPosition -= Up * velocity;
        if (direction == LEFT)
            LocalPosition -= Right * velocity;
        if (direction == RIGHT)
            LocalPosition += Right * velocity;
        updateCameraVectors();
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void processMouseMovement(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch -= yoffset;

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void processMouseScroll(float yoffset)
    {
        LocalPosition += Front * yoffset * MovementSpeed * MouseSensitivity * glm::length(LocalPosition);
        updateCameraVectors();
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front = PivotPosition - LocalPosition;
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, AbsoluteUp)); // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up = glm::normalize(glm::cross(Right, Front));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, PivotPosition);
        model = glm::rotate(model, -glm::radians(Yaw), AbsoluteUp);
        model = glm::rotate(model, -glm::radians(Pitch), Right);
        WorldPosition = model * glm::vec4(LocalPosition.x, LocalPosition.y, LocalPosition.z, 1);
        WorldFront = model * glm::vec4(Front.x, Front.y, Front.z, 1);
        WorldUp = model * glm::vec4(Up.x, Up.y, Up.z, 1);
        WorldRight = model * glm::vec4(Right.x, Right.y, Right.z, 1);
        updated = true;
    }
};
#endif
