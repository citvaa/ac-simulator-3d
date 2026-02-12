#pragma once

#include "../Header/Renderer2D.h"
#include <glm/glm.hpp>

// Mutable simulation state updated every frame.
struct AppState
{
    bool isOn = false;
    bool lockedByFullBowl = false;
    float ventOpenness = 0.0f; // 0 closed, 1 open
    float ventAnimSpeed = 1.5f; // openness units per second
    bool prevMouseDown = false;
    float desiredTemp = 24.0f;
    float currentTemp = 30.0f;
    float tempDriftSpeed = 0.8f; // degrees per second
    float tempChangeStep = 1.0f;
    bool prevUpPressed = false;
    bool prevDownPressed = false;
    float waterLevel = 0.0f; // 0 empty, 1 full
    float waterFillPerSecond = 0.12f;
    float waterAccum = 0.0f;
    bool prevSpacePressed = false;
    bool holdingBowl = false; // true when user clicked and picked up the bowl
};

void handlePowerToggle(AppState& state, double mouseX, double mouseY, bool mouseDown, const CircleShape& lamp);
void updateVent(AppState& state, float deltaTime);
void handleTemperatureInput(AppState& state, bool upPressed, bool downPressed);
void updateTemperature(AppState& state, float deltaTime);
// camera position and forward are used to gate SPACE interactions when holding the bowl
void updateWater(AppState& state, float deltaTime, bool spacePressed, const glm::vec3& camPos, const glm::vec3& camForward);
