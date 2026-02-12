#include "../Header/State.h"

#include <algorithm>
#include <cmath>

void handlePowerToggle(AppState& state, double mouseX, double mouseY, bool mouseDown, const CircleShape& lamp)
{
    // Toggle AC on lamp click; ignore if locked by full bowl.
    if (mouseDown && !state.prevMouseDown)
    {
        float dx = static_cast<float>(mouseX) - lamp.x;
        float dy = static_cast<float>(mouseY) - lamp.y;
        float distSq = dx * dx + dy * dy;
        if (distSq <= lamp.radius * lamp.radius && !state.lockedByFullBowl)
        {
            state.isOn = !state.isOn;
        }
    }

    state.prevMouseDown = mouseDown;
}

void updateVent(AppState& state, float deltaTime)
{
    // Animate vent toward open/closed target.
    float targetOpenness = state.isOn && !state.lockedByFullBowl ? 1.0f : 0.0f;
    if (state.ventOpenness < targetOpenness)
    {
        state.ventOpenness = std::min(targetOpenness, state.ventOpenness + state.ventAnimSpeed * deltaTime);
    }
    else if (state.ventOpenness > targetOpenness)
    {
        state.ventOpenness = std::max(targetOpenness, state.ventOpenness - state.ventAnimSpeed * deltaTime);
    }
}

void handleTemperatureInput(AppState& state, bool upPressed, bool downPressed)
{
    // Edge-detect arrow keys and clamp desired temp.
    bool upEdge = upPressed && !state.prevUpPressed;
    bool downEdge = downPressed && !state.prevDownPressed;

    if (upEdge)
    {
        state.desiredTemp += state.tempChangeStep;
    }
    if (downEdge)
    {
        state.desiredTemp -= state.tempChangeStep;
    }

    if (state.desiredTemp < -10.0f) state.desiredTemp = -10.0f;
    if (state.desiredTemp > 40.0f) state.desiredTemp = 40.0f;

    state.prevUpPressed = upPressed;
    state.prevDownPressed = downPressed;
}

void updateTemperature(AppState& state, float deltaTime)
{
    // Drift measured temp toward desired while AC is active.
    if (!state.isOn || state.lockedByFullBowl) return;

    float diff = state.desiredTemp - state.currentTemp;
    float step = state.tempDriftSpeed * deltaTime;

    if (std::fabs(diff) <= step)
    {
        state.currentTemp = state.desiredTemp;
    }
    else
    {
        state.currentTemp += (diff > 0.0f ? step : -step);
    }
}

void updateWater(AppState& state, float deltaTime, bool spacePressed, const glm::vec3& camPos, const glm::vec3& camForward)
{
    // Fill bowl over time while AC runs; Space drains and unlocks only when holding bowl and oriented correctly.
    bool spaceEdge = spacePressed && !state.prevSpacePressed;
    if (spaceEdge)
    {
        if (state.holdingBowl)
        {
            // compute orientation relative to AC at origin
            glm::vec3 toAC = glm::normalize(glm::vec3(0.0f) - camPos);
            float dot = glm::dot(glm::normalize(camForward), toAC);
            // if bowl is full (or nearly) and user is facing away (approx 180deg), empty it
            if (state.waterLevel >= 0.99f) {
                if (dot <= -0.9f) {
                    state.waterLevel = 0.0f;
                    state.lockedByFullBowl = false;
                    // remain holding bowl so user can return it
                }
            }
            else
            {
                // bowl empty: only allow returning it to its place when facing toward AC
                if (dot >= 0.9f) {
                    state.holdingBowl = false;
                    state.waterLevel = 0.0f;
                    state.lockedByFullBowl = false;
                }
            }
        }
        else
        {
            // not holding bowl: no global space action (require pickup first)
        }
    }

    if (state.isOn && !state.lockedByFullBowl)
    {
        state.waterAccum += deltaTime;
        while (state.waterAccum >= 1.0f)
        {
            state.waterAccum -= 1.0f;
            state.waterLevel += state.waterFillPerSecond;
        }
    }

    if (state.waterLevel > 1.0f)
    {
        state.waterLevel = 1.0f;
    }

    if (state.waterLevel >= 1.0f)
    {
        state.isOn = false;
        state.lockedByFullBowl = true;
    }

    state.prevSpacePressed = spacePressed;
}
