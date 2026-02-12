#include "Camera3D.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Camera3D::Camera3D(GLFWwindow* win, float width, float height)
    : window_(win), width_(static_cast<int>(width)), height_(static_cast<int>(height))
{
    // Ensure cursor is visible so user can click in the single input mode
    if (window_) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Camera3D::setWindowSize(int w, int h)
{
    width_ = w;
    height_ = h;
}

void Camera3D::cursorPosCallback(double xpos, double ypos)
{
    if (firstMouse_)
    {
        lastX_ = static_cast<float>(xpos);
        lastY_ = static_cast<float>(ypos);
        firstMouse_ = false;
        return;
    }

    float xoffset = static_cast<float>(xpos) - lastX_;
    float yoffset = lastY_ - static_cast<float>(ypos); // reversed since y-coordinates go from top to bottom
    lastX_ = static_cast<float>(xpos);
    lastY_ = static_cast<float>(ypos);

    xoffset *= sensitivity_;
    yoffset *= sensitivity_;

    if (orbitMode_)
    {
        if (rotating_)
        {
            orbitYaw_ += xoffset * 0.5f;
            orbitPitch_ += yoffset * 0.5f;
            if (orbitPitch_ > 89.0f) orbitPitch_ = 89.0f;
            if (orbitPitch_ < -89.0f) orbitPitch_ = -89.0f;
        }
    }
    else
    {
        // first-person look
        yaw_ += xoffset;
        pitch_ += yoffset;
        if (pitch_ > 89.0f) pitch_ = 89.0f;
        if (pitch_ < -89.0f) pitch_ = -89.0f;
    }
}

void Camera3D::mouseButtonCallback(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        rotating_ = (action == GLFW_PRESS);
        if (action == GLFW_PRESS)
        {
            firstMouse_ = true; // reset delta on press
        }
    }
}

void Camera3D::scrollCallback(double xoffset, double yoffset)
{
    if (orbitMode_)
    {
        orbitRadius_ -= static_cast<float>(yoffset) * 20.0f;
        if (orbitRadius_ < 50.0f) orbitRadius_ = 50.0f;
        if (orbitRadius_ > 2000.0f) orbitRadius_ = 2000.0f;
    }
    else
    {
        posZ_ -= static_cast<float>(yoffset) * 20.0f;
        if (posZ_ < 1.0f) posZ_ = 1.0f;
    }
}

glm::mat4 Camera3D::getViewMatrix() const
{
    glm::vec3 pos(posX_, posY_, posZ_);
    if (orbitMode_)
    {
        // Look at origin
        return glm::lookAt(pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else
    {
        float yawRad = glm::radians(yaw_);
        float pitchRad = glm::radians(pitch_);
        glm::vec3 front;
        front.x = cos(yawRad) * cos(pitchRad);
        front.y = sin(pitchRad);
        front.z = sin(yawRad) * cos(pitchRad);
        front = glm::normalize(front);
        return glm::lookAt(pos, pos + front, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

glm::mat4 Camera3D::getProjectionMatrix() const
{
    float aspect = (width_ > 0 && height_ > 0) ? static_cast<float>(width_) / static_cast<float>(height_) : 4.0f/3.0f;
    return glm::perspective(glm::radians(fov_), aspect, 0.1f, 5000.0f);
}

void Camera3D::update(float deltaTime)
{
    // Movement for first-person mode
    if (!orbitMode_ && window_ != nullptr)
    {
        float velocity = moveSpeed_ * deltaTime;
        float yawRad = glm::radians(yaw_);
        float pitchRad = glm::radians(pitch_);
        glm::vec3 front;
        front.x = cos(yawRad) * cos(pitchRad);
        front.y = sin(pitchRad);
        front.z = sin(yawRad) * cos(pitchRad);
        front = glm::normalize(front);
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(front, up));

        if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
            posX_ += front.x * velocity; posY_ += front.y * velocity; posZ_ += front.z * velocity;
        }
        if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
            posX_ -= front.x * velocity; posY_ -= front.y * velocity; posZ_ -= front.z * velocity;
        }
        if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) {
            posX_ -= right.x * velocity; posY_ -= right.y * velocity; posZ_ -= right.z * velocity;
        }
        if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
            posX_ += right.x * velocity; posY_ += right.y * velocity; posZ_ += right.z * velocity;
        }
        if (glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS) {
            posY_ += velocity; // move up
        }
        if (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS) {
            posY_ -= velocity; // move down
        }
    }

    // Orbit mode: update position from spherical coordinates
    if (orbitMode_)
    {
        float radYaw = glm::radians(orbitYaw_);
        float radPitch = glm::radians(orbitPitch_);
        posX_ = orbitRadius_ * cos(radPitch) * cos(radYaw);
        posY_ = orbitRadius_ * sin(radPitch);
        posZ_ = orbitRadius_ * cos(radPitch) * sin(radYaw);
    }
}

void Camera3D::toggleMode()
{
    // single-mode camera: toggle disabled to avoid switching modes in this build
    // No action; keep first-person-like movement with visible cursor enabled.
    return;
}
