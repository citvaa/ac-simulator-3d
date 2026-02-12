#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Camera3D {
public:
    Camera3D(GLFWwindow* win, float width, float height);

    void setWindowSize(int w, int h);
    void cursorPosCallback(double xpos, double ypos);
    void mouseButtonCallback(int button, int action, int mods);
    void scrollCallback(double xoffset, double yoffset);
    void update(float deltaTime);
    void toggleMode(); // toggle between orbit and first-person

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

private:
    GLFWwindow* window_ = nullptr;
    int width_ = 800;
    int height_ = 600;

    // simple camera state
    float posX_ = 0.0f, posY_ = 0.0f, posZ_ = 600.0f; // start back to view 2D scene
    float yaw_ = -90.0f, pitch_ = 0.0f;
    float lastX_ = 0.0f, lastY_ = 0.0f;
    bool firstMouse_ = true;
    bool rotating_ = false;

    bool orbitMode_ = false; // single camera mode: first-person-like movement with visible cursor
    float orbitRadius_ = 600.0f;
    float orbitYaw_ = 0.0f;
    float orbitPitch_ = 0.0f;

    float sensitivity_ = 0.25f;
    float fov_ = 45.0f;
    float moveSpeed_ = 400.0f; // units per second for first-person
};
