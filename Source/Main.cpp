#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include "../Header/Renderer2D.h"
#include "../Header/State.h"
#include "../Header/TemperatureUI.h"
#include "../Header/Controls.h"
#include "../Header/TextRenderer.h"
#include "Camera3D.h"
#include "Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <string>
#include <cstdio>
#include <thread>
#include <random>

// Entry point: fullscreen AC simulator with timed logic and on-screen UI.
const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

// Pointers handed to the framebuffer-size callback so we can update renderers on resize.
struct ResizeContext
{
    Renderer2D* renderer = nullptr;
    TextRenderer* textRenderer = nullptr;
    int* windowWidth = nullptr;
    int* windowHeight = nullptr;
    Camera3D* camera = nullptr;
};

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // request a depth buffer so 3D rendering has proper depth testing
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary); // fullscreen mode descriptor

    int windowWidth = mode ? mode->width : 800;
    int windowHeight = mode ? mode->height : 800;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "AC Simulator", primary, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    const Color backgroundColor{ 0.10f, 0.12f, 0.16f, 1.0f };
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

    // Runtime toggles state (default enabled)
    bool depthTestEnabled = true;
    bool cullEnabled = true;

    // Default GL states for depth testing and face culling (user can toggle at runtime)
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cullEnabled) { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); } else glDisable(GL_CULL_FACE);

    // Shader program and basic geometry
    Renderer2D renderer(fbWidth, fbHeight, "Shaders/basic.vert", "Shaders/basic.frag");
    TextRenderer textRenderer(fbWidth, fbHeight);
    GLuint overlayProgram = createShader("Shaders/overlay.vert", "Shaders/overlay.frag");
    GLint overlayWindowSizeLoc = glGetUniformLocation(overlayProgram, "uWindowSize");
    GLint overlayTintLoc = glGetUniformLocation(overlayProgram, "uTint");
    GLint overlayTextureLoc = glGetUniformLocation(overlayProgram, "uTexture");

    // 3D renderer (shaders compiled and ready)
    Renderer renderer3D;
    if (!renderer3D.init()) {
        return endProgram("Neuspeh pri inicijalizaciji 3D renderera.");
    }

    // connect 2D renderer to 3D renderer so 2D calls produce 3D placeholders
    renderer.set3DRenderer(&renderer3D);

    ResizeContext resizeCtx;
    Camera3D camera(window, fbWidth, fbHeight);
    resizeCtx.renderer = &renderer;
    resizeCtx.textRenderer = &textRenderer;
    resizeCtx.windowWidth = &windowWidth;
    resizeCtx.windowHeight = &windowHeight;
    resizeCtx.camera = &camera;
    glfwSetWindowUserPointer(window, &resizeCtx);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int w, int h)
    {
        auto* ctx = static_cast<ResizeContext*>(glfwGetWindowUserPointer(win));
        if (!ctx) return;
        glViewport(0, 0, w, h);
        if (ctx->windowWidth) *ctx->windowWidth = w;
        if (ctx->windowHeight) *ctx->windowHeight = h;
        if (ctx->renderer) ctx->renderer->setWindowSize(static_cast<float>(w), static_cast<float>(h));
        if (ctx->textRenderer) ctx->textRenderer->setWindowSize(static_cast<float>(w), static_cast<float>(h));
        if (ctx->camera) ctx->camera->setWindowSize(w, h);
    });

    // load toilet model (optional)
    int toiletModelId = -1;
    {
        // try relative paths (when running from build dir the executable cwd is cmake-build-debug)
        // prefer the higher-quality model if present
        toiletModelId = renderer3D.loadOBJModel("Assets/models/10778_Toilet_V2.obj");
        if (toiletModelId < 0) toiletModelId = renderer3D.loadOBJModel("Assets/models/toilet.obj");
        if (toiletModelId < 0) toiletModelId = renderer3D.loadOBJModel("../Assets/models/10778_Toilet_V2.obj");
        if (toiletModelId < 0) toiletModelId = renderer3D.loadOBJModel("../Assets/models/toilet.obj");

        if (toiletModelId < 0) {
            // print to stderr so IDE/build output shows whether the model was found/loaded
            fprintf(stderr, "Warning: toilet.obj failed to load (path: Assets/models/toilet.obj)\n");
        } else {
            fprintf(stderr, "Loaded toilet.obj as model id %d\n", toiletModelId);
        }
    }

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y)
    {
        auto* ctx = static_cast<ResizeContext*>(glfwGetWindowUserPointer(win));
        if (!ctx || !ctx->camera) return;
        ctx->camera->cursorPosCallback(x, y);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods)
    {
        auto* ctx = static_cast<ResizeContext*>(glfwGetWindowUserPointer(win));
        if (!ctx || !ctx->camera) return;
        ctx->camera->mouseButtonCallback(button, action, mods);
    });

    glfwSetScrollCallback(window, [](GLFWwindow* win, double xoffset, double yoffset)
    {
        auto* ctx = static_cast<ResizeContext*>(glfwGetWindowUserPointer(win));
        if (!ctx || !ctx->camera) return;
        ctx->camera->scrollCallback(xoffset, yoffset);
    });

    const Color bodyColor{ 0.90f, 0.93f, 0.95f, 1.0f };
    const Color ventColor{ 0.32f, 0.36f, 0.45f, 1.0f };
    const Color lampOffColor{ 0.22f, 0.18f, 0.20f, 1.0f };
    const Color lampOnColor{ 0.93f, 0.22f, 0.20f, 1.0f };
    const Color screenOffColor{ 0.08f, 0.10f, 0.12f, 1.0f };
    const Color screenOnColor{ 0.18f, 0.68f, 0.72f, 1.0f };
    const Color bowlColor{ 0.78f, 0.82f, 0.88f, 1.0f };
    const Color digitColor{ 0.96f, 0.98f, 1.0f, 1.0f };
    const Color arrowBg{ 0.15f, 0.18f, 0.22f, 1.0f };
    // arrows explicitly white (background is dark)
    const Color arrowColor{ 1.0f, 1.0f, 1.0f, 1.0f };
    const Color waterColor{ 0.50f, 0.78f, 0.94f, 0.9f };
    const Color nameplateBg{ 0.08f, 0.08f, 0.10f, 0.45f };
    const Color nameplateText{ 0.96f, 0.98f, 1.0f, 0.95f };

    const float acWidth = 480.0f;
    const float acHeight = 200.0f;
    const float acY = 0.0f;

    RectShape acBody{ 0.0f, acY, acWidth, acHeight, bodyColor };

    const float ventClosedHeight = 4.0f;
    const float ventOpenHeight = 18.0f;
    RectShape ventBar{ 24.0f, acY + acHeight - 64.0f, acWidth - 48.0f, ventClosedHeight, ventColor };
    CircleShape lamp{ acWidth - 44.0f, acY + acHeight - 26.0f, 14.0f, lampOffColor };

    const float screenWidth = 94.0f;
    const float screenHeight = 54.0f;
    const float screenSpacing = 22.0f;
    const float screenStartX = 70.0f;
    const float screenY = acY + 52.0f;
    std::array<RectShape, 3> screens{};
    for (size_t i = 0; i < screens.size(); ++i)
    {
        screens[i] = RectShape{
            screenStartX + static_cast<float>(i) * (screenWidth + screenSpacing),
            screenY,
            screenWidth,
            screenHeight,
            screenOffColor
        };
    }

    const float arrowWidth = 40.0f;
    RectShape tempArrowButton{ screenStartX - arrowWidth - 12.0f, screenY, arrowWidth, screenHeight, arrowBg };

    const float bowlWidth = 260.0f;
    const float bowlHeight = 140.0f;
    const float bowlThickness = 10.0f;
    const float bowlX = (acWidth - bowlWidth) * 0.5f;
    const float bowlY = acY + acHeight + 120.0f;
    RectShape bowlOutline{ bowlX, bowlY, bowlWidth, bowlHeight, bowlColor };

    GLuint nameplateTexture = 0;
    int nameplateW = 0;
    int nameplateH = 0;
    textRenderer.createTextTexture("Vuk Vicentic, SV45/2022", nameplateText, nameplateBg, 10, 42, nameplateTexture, nameplateW, nameplateH);

    GLuint overlayVao = 0;
    GLuint overlayVbo = 0;
    glGenVertexArrays(1, &overlayVao);
    glGenBuffers(1, &overlayVbo);
    glBindVertexArray(overlayVao);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // create a simple circular white texture (alpha mask) for the lamp icon so it appears round in 3D
    GLuint lampCircleTex = 0;
    {
        const int texSize = 64;
        std::vector<unsigned char> pixels(texSize * texSize * 4, 0);
        float cx = (texSize - 1) * 0.5f;
        float cy = (texSize - 1) * 0.5f;
        float r = (texSize * 0.45f);
        for (int y = 0; y < texSize; ++y) {
            for (int x = 0; x < texSize; ++x) {
                float dx = static_cast<float>(x) - cx;
                float dy = static_cast<float>(y) - cy;
                float d2 = dx*dx + dy*dy;
                int idx = (y * texSize + x) * 4;
                if (d2 <= r*r) {
                    pixels[idx + 0] = 255;
                    pixels[idx + 1] = 255;
                    pixels[idx + 2] = 255;
                    pixels[idx + 3] = 255;
                } else {
                    pixels[idx + 0] = 0;
                    pixels[idx + 1] = 0;
                    pixels[idx + 2] = 0;
                    pixels[idx + 3] = 0;
                }
            }
        }
        glGenTextures(1, &lampCircleTex);
        glBindTexture(GL_TEXTURE_2D, lampCircleTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Create and set a simple remote-shaped cursor (hotspot at laser dot top-left).
    auto setProceduralCursor = [&]()
    {
        GLFWcursor* cursor = createProceduralRemoteCursor();

        if (cursor != nullptr)
        {
            glfwSetCursor(window, cursor);
        }
    };

    setProceduralCursor();

    bool prevCPressed = false;
    bool prevLPressed = false;
    bool prevToggleDepth = false;
    bool prevToggleCull = false;

    AppState appState{};
    // Start with AC on so lamp and lamp-light can be observed
    appState.isOn = true;

    // particle drops
    struct Particle { glm::vec3 pos; glm::vec3 vel; float radius; bool alive; };
    std::vector<Particle> droplets;
    float spawnAccumulator = 0.0f;
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> randX(-20.0f, 20.0f);
    std::uniform_real_distribution<float> randZ(-10.0f, 10.0f);

    std::string frameStats = "FPS --";
    double logAccumulator = 0.0;
    int logFrames = 0;

    auto lastTime = std::chrono::steady_clock::now(); // main clock source

    while (!glfwWindowShouldClose(window))
    {
        auto frameStartTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(frameStartTime - lastTime).count(); // seconds since last frame
        lastTime = frameStartTime;
        logAccumulator += deltaTime;
        ++logFrames;
        if (logAccumulator >= 1.0)
        {
            double avgDelta = logAccumulator / static_cast<double>(logFrames);
            double avgFps = avgDelta > 0.0 ? 1.0 / avgDelta : 0.0;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "FPS %.1f", avgFps); // once per second
            frameStats = buf;
            logAccumulator = 0.0;
            logFrames = 0;
        }

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool upPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        bool downPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        // Camera mode toggle disabled: single movement mode with visible cursor
        prevCPressed = false;

        bool tPressed = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
        bool cTogglePressed = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
        if (tPressed && !prevToggleDepth) {
            depthTestEnabled = !depthTestEnabled;
            if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            fprintf(stderr, "Depth test %s\n", depthTestEnabled ? "ENABLED" : "DISABLED");
        }
        if (cTogglePressed && !prevToggleCull) {
            cullEnabled = !cullEnabled;
            if (cullEnabled) { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); } else glDisable(GL_CULL_FACE);
            fprintf(stderr, "Backface culling %s\n", cullEnabled ? "ENABLED" : "DISABLED");
        }
        prevToggleDepth = tPressed;
        prevToggleCull = cTogglePressed;

        bool clickStarted = mouseDown && !appState.prevMouseDown;

        float sceneMinX = std::min({ acBody.x, tempArrowButton.x, bowlOutline.x });
        float sceneMaxX = std::max({ acBody.x + acBody.w, tempArrowButton.x + tempArrowButton.w, bowlOutline.x + bowlOutline.w });
        float sceneMinY = std::min({ acBody.y, tempArrowButton.y, bowlOutline.y });
        float sceneMaxY = std::max({ acBody.y + acBody.h, tempArrowButton.y + tempArrowButton.h, bowlOutline.y + bowlOutline.h });
        float sceneW = sceneMaxX - sceneMinX;
        float sceneH = sceneMaxY - sceneMinY;
        float offsetX = (static_cast<float>(windowWidth) - sceneW) * 0.5f - sceneMinX;
        float offsetY = (static_cast<float>(windowHeight) - sceneH) * 0.5f - sceneMinY;

        auto shiftRect = [&](const RectShape& r)
        {
            RectShape out = r;
            out.x += offsetX;
            out.y += offsetY;
            return out;
        };
        auto shiftCircle = [&](const CircleShape& c)
        {
            CircleShape out = c;
            out.x += offsetX;
            out.y += offsetY;
            return out;
        };

        RectShape acBodyDraw = shiftRect(acBody);
        RectShape ventBarDraw = shiftRect(ventBar);
        CircleShape lampDraw = shiftCircle(lamp);

        std::array<RectShape, 3> screensDraw = screens;
        for (auto& s : screensDraw)
        {
            s = shiftRect(s);
        }

        RectShape tempArrowDraw = shiftRect(tempArrowButton);
        RectShape bowlDraw = shiftRect(bowlOutline);
        float bowlInnerX = bowlDraw.x + bowlThickness;
        float bowlInnerY = bowlDraw.y + bowlThickness;
        float bowlInnerW = bowlDraw.w - 2.0f * bowlThickness;
        float bowlInnerH = bowlDraw.h - 2.0f * bowlThickness;

        bool tempArrowClicked = false;
        if (clickStarted && !appState.lockedByFullBowl)
        {
            if (pointInRect(mouseX, mouseY, tempArrowDraw))
            {
                float midY = tempArrowDraw.y + tempArrowDraw.h * 0.5f;
                if (mouseY < midY)
                {
                    appState.desiredTemp += appState.tempChangeStep;
                }
                else
                {
                    appState.desiredTemp -= appState.tempChangeStep;
                }
                tempArrowClicked = true;
            }

            if (tempArrowClicked)
            {
                if (appState.desiredTemp < -10.0f) appState.desiredTemp = -10.0f;
                if (appState.desiredTemp > 40.0f) appState.desiredTemp = 40.0f;
            }
        }

        handlePowerToggle(appState, mouseX, mouseY, mouseDown, lampDraw);
        handleTemperatureInput(appState, upPressed, downPressed);
        updateVent(appState, deltaTime);
        updateTemperature(appState, deltaTime);
        // compute camera position and forward for gating SPACE interactions
        glm::vec3 camPos(0.0f), camForward(0.0f,0.0f,-1.0f);
        {
            auto* ctx = static_cast<ResizeContext*>(glfwGetWindowUserPointer(window));
            if (ctx && ctx->camera) {
                glm::mat4 view = ctx->camera->getViewMatrix();
                glm::mat4 invView = glm::inverse(view);
                camPos = glm::vec3(invView[3][0], invView[3][1], invView[3][2]);
                camForward = glm::normalize(glm::vec3(invView * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
            }
        }
        updateWater(appState, deltaTime, spacePressed, camPos, camForward);

        // hide the OS cursor when the bowl is held so only the remote model is visible
        if (appState.holdingBowl) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        // Update camera each frame
        glm::mat4 currentView = glm::mat4(1.0f);
        glm::mat4 currentProj = glm::mat4(1.0f);
        glm::vec3 lampWorldPos(0.0f);
        glm::vec3 bowlWorldPos(0.0f);
        float bowlWWorld = 0.0f, bowlHWorld = 0.0f, bowlDepth = 80.0f;
        {
            auto* ctx = static_cast<ResizeContext*>(glfwGetWindowUserPointer(window));
            if (ctx && ctx->camera) {
                ctx->camera->update(deltaTime);
                // compute lamp world position (mapToAC equivalent) so renderer can set lamp light uniform
                float acCenterX = acBodyDraw.x + acBodyDraw.w * 0.5f;
                float acCenterY = acBodyDraw.y + acBodyDraw.h * 0.5f;
                float lampLocalX = (lampDraw.x - acCenterX) * (240.0f / acBody.w);
                float lampLocalY = (acCenterY - lampDraw.y) * (100.0f / acBody.h);
                float lampLocalZ = 40.0f + 6.0f;
                lampWorldPos = glm::vec3(lampLocalX, lampLocalY, lampLocalZ);
                currentView = ctx->camera->getViewMatrix();
                currentProj = ctx->camera->getProjectionMatrix();

                // tell renderer about lamp light (red when on)
                glm::vec3 lampColorVec = appState.isOn ? glm::vec3(0.93f, 0.22f, 0.20f) : glm::vec3(0.12f, 0.12f, 0.12f);
                float lampIntensity = appState.isOn ? 3.0f : 0.0f;
                renderer3D.setLampLight(lampWorldPos, lampColorVec, lampIntensity, appState.isOn);

                // ensure scene light stays on regardless of AC state
                renderer3D.setSceneLight(glm::vec3(-350.0f, 260.0f, 40.0f), glm::vec3(1.0f, 0.95f, 0.2f), 2.5f);

                // upload camera matrices to 3D renderer
                renderer3D.setViewProjection(currentView, currentProj);
            }
        }

        lampDraw.color = appState.isOn ? lampOnColor : lampOffColor;

        // allow keyboard toggle for lamp (L key)
        bool lPressed = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
        if (lPressed && !prevLPressed) {
            appState.isOn = !appState.isOn;
            glm::vec3 lampColorVec = appState.isOn ? glm::vec3(0.93f, 0.22f, 0.20f) : glm::vec3(0.12f, 0.12f, 0.12f);
            float lampIntensity = appState.isOn ? 3.0f : 0.0f;
            renderer3D.setLampLight(lampWorldPos, lampColorVec, lampIntensity, appState.isOn);
        }
        prevLPressed = lPressed;

        // prepare bowl world position and extents (used for picking and drawing)
        {
            float wworld = bowlDraw.w * (240.0f / acBody.w);
            float hworld = bowlDraw.h * (100.0f / acBody.h);
            float bowlFullHeight = hworld * 0.5f;
            float acHalfHeight = 100.0f * 0.5f;
            float gap = 300.0f;
            bowlWorldPos = glm::vec3(0.0f, -acHalfHeight - (bowlFullHeight * 0.5f) - gap, 0.0f);
            bowlWWorld = wworld;
            bowlHWorld = bowlFullHeight;
        }

        // perform raycast picking on click start
        if (clickStarted)
        {
            // build inverse PV matrix
            glm::mat4 invPV = glm::inverse(currentProj * currentView);
            // normalized device coords
            float ndcX = (static_cast<float>(mouseX) / static_cast<float>(windowWidth)) * 2.0f - 1.0f;
            float ndcY = 1.0f - (static_cast<float>(mouseY) / static_cast<float>(windowHeight)) * 2.0f;
            glm::vec4 nearPointNDC(ndcX, ndcY, -1.0f, 1.0f);
            glm::vec4 farPointNDC(ndcX, ndcY, 1.0f, 1.0f);
            glm::vec4 worldNear4 = invPV * nearPointNDC; worldNear4 /= worldNear4.w;
            glm::vec4 worldFar4 = invPV * farPointNDC; worldFar4 /= worldFar4.w;
            glm::vec3 rayOrigin = glm::vec3(worldNear4);
            glm::vec3 rayDir = glm::normalize(glm::vec3(worldFar4) - rayOrigin);

            // test lamp (sphere) intersection
            float lampRadius = (lampDraw.radius * 2.0f * (240.0f / acBody.w)) * 0.5f;
            glm::vec3 L = rayOrigin - lampWorldPos;
            float a = glm::dot(rayDir, rayDir);
            float b = 2.0f * glm::dot(rayDir, L);
            float c = glm::dot(L, L) - lampRadius * lampRadius;
            float disc = b*b - 4*a*c;
            if (disc >= 0.0f) {
                float t = (-b - sqrt(disc)) / (2.0f * a);
                if (t > 0.0f) {
                    // hit lamp: toggle power
                    appState.isOn = !appState.isOn;
                    // update lamp uniforms immediately
                    glm::vec3 lampColorVec = appState.isOn ? glm::vec3(0.93f, 0.22f, 0.20f) : glm::vec3(0.12f);
                    float lampIntensity = appState.isOn ? 3.0f : 0.0f;
                    renderer3D.setLampLight(lampWorldPos, lampColorVec, lampIntensity, appState.isOn);
                }
            }

            auto hitAABB = [&](const glm::vec3& center, const glm::vec3& halfExtents) -> bool
            {
                glm::vec3 minB = center - halfExtents;
                glm::vec3 maxB = center + halfExtents;
                float tmin = 0.0f; float tmax = 1e9f;
                for (int i = 0; i < 3; ++i) {
                    float invD = 1.0f / ((&rayDir.x)[i]);
                    float t0 = ((&minB.x)[i] - (&rayOrigin.x)[i]) * invD;
                    float t1 = ((&maxB.x)[i] - (&rayOrigin.x)[i]) * invD;
                    if (invD < 0.0f) std::swap(t0, t1);
                    tmin = std::max(tmin, t0);
                    tmax = std::min(tmax, t1);
                    if (tmax <= tmin) break;
                }
                return (tmax > tmin && tmax > 0.0f);
            };

            // test arrow buttons (AABB) in 3D so clicks work with camera movement
            if (!tempArrowClicked && !appState.lockedByFullBowl) {
                float acCenterX = acBodyDraw.x + acBodyDraw.w * 0.5f;
                float acCenterY = acBodyDraw.y + acBodyDraw.h * 0.5f;
                auto mapToACPick = [&](float px, float py, float z)->glm::vec3 {
                    float localX = (px - acCenterX) * (240.0f / acBody.w);
                    float localY = (acCenterY - py) * (100.0f / acBody.h);
                    return glm::vec3(localX, localY, z);
                };

                float scaleX = 240.0f / acBody.w;
                float scaleY = 100.0f / acBody.h;
                float halfH = tempArrowDraw.h * 0.5f;
                float wworld = tempArrowDraw.w * scaleX;
                float hworld = halfH * scaleY;
                float zFront = 40.0f + 6.0f;

                float cx = tempArrowDraw.x + tempArrowDraw.w * 0.5f;
                float cyTop = tempArrowDraw.y + halfH * 0.5f;
                float cyBot = tempArrowDraw.y + halfH + halfH * 0.5f;
                glm::vec3 halfExtents(wworld * 0.5f, hworld * 0.5f, 2.0f);

                glm::vec3 topPos = mapToACPick(cx, cyTop, zFront);
                glm::vec3 botPos = mapToACPick(cx, cyBot, zFront);
                if (hitAABB(topPos, halfExtents)) {
                    appState.desiredTemp += appState.tempChangeStep;
                    tempArrowClicked = true;
                } else if (hitAABB(botPos, halfExtents)) {
                    appState.desiredTemp -= appState.tempChangeStep;
                    tempArrowClicked = true;
                }

                if (tempArrowClicked) {
                    if (appState.desiredTemp < -10.0f) appState.desiredTemp = -10.0f;
                    if (appState.desiredTemp > 40.0f) appState.desiredTemp = 40.0f;
                }
            }

            // test bowl (AABB) intersection
            glm::vec3 boxMin = bowlWorldPos - glm::vec3(bowlWWorld*0.5f, bowlHWorld*0.5f, bowlDepth*0.5f);
            glm::vec3 boxMax = bowlWorldPos + glm::vec3(bowlWWorld*0.5f, bowlHWorld*0.5f, bowlDepth*0.5f);
            float tmin = 0.0f; float tmax = 1e9f;
            for (int i = 0; i < 3; ++i) {
                float invD = 1.0f / ((&rayDir.x)[i]);
                float t0 = ((&boxMin.x)[i] - (&rayOrigin.x)[i]) * invD;
                float t1 = ((&boxMax.x)[i] - (&rayOrigin.x)[i]) * invD;
                if (invD < 0.0f) std::swap(t0, t1);
                tmin = std::max(tmin, t0);
                tmax = std::min(tmax, t1);
                if (tmax <= tmin) break;
            }
            if (tmax > tmin && tmax > 0.0f) {
                // hit the bowl: if full and AC is off, pick it up
                if (appState.waterLevel >= 0.99f && !appState.isOn) {
                    appState.holdingBowl = !appState.holdingBowl;
                }
            }
        }
        float ventHeight = ventClosedHeight + (ventOpenHeight - ventClosedHeight) * appState.ventOpenness;
        ventBarDraw.h = ventHeight;

        Color screenColor = appState.isOn ? screenOnColor : screenOffColor;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 3D pass: draw AC unit cube and lid
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (cullEnabled) { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); } else glDisable(GL_CULL_FACE);

        // update particles (physics + spawning)
        {
            // spawn rate per second (drops) proportional to vent openness (reduced)
            float spawnRate = 6.0f * appState.ventOpenness; // drops/sec (was 12)
            if (appState.isOn && spawnRate > 0.0f) {
                spawnAccumulator += spawnRate * deltaTime;
                while (spawnAccumulator >= 1.0f) {
                    spawnAccumulator -= 1.0f;
                    Particle p;
                    // spawn under AC bottom center (local coords)
                    float spawnY = -50.0f - 5.0f;
                    p.pos = glm::vec3(randX(rng), spawnY, randZ(rng));
                    // slower initial downward velocity to avoid tunneling
                    p.vel = glm::vec3(0.0f, -60.0f - std::abs(randZ(rng))*1.0f, 0.0f);
                    p.radius = 4.0f;
                    p.alive = true;
                    droplets.push_back(p);
                }
            }

            // physics integration (reduced gravity)
            glm::vec3 gravity(0.0f, -400.0f, 0.0f); // was -980
            for (auto &d : droplets) {
                if (!d.alive) continue;
                d.vel += gravity * deltaTime;
                d.pos += d.vel * deltaTime;

                // collision check with bowl inner top
                // bowlWorldPos and bowl extents computed earlier
                float innerWWorld = (bowlInnerW * (240.0f / acBody.w));
                float innerRadius = innerWWorld * 0.5f;
                float bowlTopY = bowlWorldPos.y + (bowlHWorld * 0.5f) - (bowlThickness * (100.0f / acBody.h));

                // allow small tolerance to avoid tunneling and accept near-misses
                float verticalTolerance = 4.0f;
                float rimTolerance = 2.0f;

                if (d.pos.y - d.radius <= bowlTopY + verticalTolerance) {
                    // compute horizontal distance to bowl center
                    float dx = d.pos.x - bowlWorldPos.x;
                    float dz = d.pos.z - bowlWorldPos.z;
                    float distXZ = std::sqrt(dx*dx + dz*dz);

                    if (distXZ <= innerRadius - 1.0f) {
                        // clearly inside
                        d.alive = false;
                        appState.waterLevel += 0.0015f; // each drop adds less
                        if (appState.waterLevel >= 1.0f) {
                            appState.waterLevel = 1.0f;
                            appState.isOn = false;
                            appState.lockedByFullBowl = true;
                        }
                    } else if (d.pos.y <= bowlTopY - verticalTolerance && distXZ <= innerRadius + rimTolerance) {
                        // tunneled through but horizontally near center -> collect
                        d.alive = false;
                        appState.waterLevel += 0.0015f;
                        if (appState.waterLevel >= 1.0f) {
                            appState.waterLevel = 1.0f;
                            appState.isOn = false;
                            appState.lockedByFullBowl = true;
                        }
                    } else if (distXZ <= innerRadius + rimTolerance) {
                        // considered hitting rim: bounce outward slightly
                        if (distXZ < 0.001f) distXZ = 0.001f;
                        d.vel.x += (dx / distXZ) * 50.0f;
                        d.vel.z += (dz / distXZ) * 50.0f;
                        // and move above rim
                        d.pos.y = bowlTopY + d.radius + 1.0f;
                    }
                }

                // kill if too low
                if (d.pos.y < bowlWorldPos.y - 1000.0f) d.alive = false;
            }

            // remove dead
            droplets.erase(std::remove_if(droplets.begin(), droplets.end(), [](const Particle&p){return !p.alive;}), droplets.end());
        }
        // compute base and lid model matrices
        static float lidAngle = 0.0f;
        const float targetAngle = appState.isOn ? 60.0f : 0.0f;
        const float angSpeed = 90.0f; // degrees per second
        if (lidAngle < targetAngle) lidAngle += angSpeed * deltaTime; if (lidAngle > targetAngle) lidAngle = targetAngle;
        if (lidAngle > targetAngle) lidAngle -= angSpeed * deltaTime; if (lidAngle < targetAngle) lidAngle = targetAngle;

        // place cube at world origin, scale to acWidth x acHeight x depth
        glm::mat4 modelBase = glm::mat4(1.0f);
        modelBase = glm::translate(modelBase, glm::vec3(0.0f, 0.0f, 0.0f));
        modelBase = glm::scale(modelBase, glm::vec3(240.0f, 100.0f, 80.0f));
        renderer3D.drawCube(modelBase, glm::vec3(0.9f, 0.93f, 0.95f));

        // draw droplets
        for (const auto &d : droplets) {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, d.pos);
            float s = d.radius;
            m = glm::scale(m, glm::vec3(s, s, s));
            renderer3D.drawParticle(m, glm::vec3(0.5f, 0.8f, 1.0f), 0.6f);
        }

        // lid: pivot at top-back edge of cube; build transform: translate to hinge, rotate, translate back
        glm::mat4 modelLid = glm::mat4(1.0f);
        // hinge location in model-space: top (y +0.5) and back (z -0.5) -> with scaling accounted later
        // We'll construct in unscaled cube space then scale
        modelLid = glm::translate(modelLid, glm::vec3(0.0f, 0.5f, -0.5f));
        modelLid = glm::rotate(modelLid, glm::radians(-lidAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        modelLid = glm::translate(modelLid, glm::vec3(0.0f, -0.5f, 0.5f));
        modelLid = glm::scale(modelLid, glm::vec3(240.0f, 20.0f, 80.0f));
        renderer3D.drawCube(modelLid, glm::vec3(0.78f, 0.82f, 0.88f));

        // Draw UI elements as 3D primitives aligned to the AC model coordinate frame
        // keep depth test enabled while drawing 3D UI

        // helper: map pixel center to AC-local world coords (AC centered at origin, scaled to 240x100x80)
        auto mapToAC = [&](float px, float py, float zOffsetFront)->glm::vec3 {
            float acCenterX = acBodyDraw.x + acBodyDraw.w * 0.5f;
            float acCenterY = acBodyDraw.y + acBodyDraw.h * 0.5f;
            float localX = (px - acCenterX) * (240.0f / acBody.w);
            float localY = (acCenterY - py) * (100.0f / acBody.h);
            float z = zOffsetFront; // caller decides front/back
            return glm::vec3(localX, localY, z);
        };

        // vent (front face)
        {
            float cx = ventBarDraw.x + ventBarDraw.w * 0.5f;
            float cy = ventBarDraw.y + ventBarDraw.h * 0.5f;
            glm::vec3 pos = mapToAC(cx, cy, 40.0f + 4.0f);
            float wworld = ventBarDraw.w * (240.0f / acBody.w);
            float hworld = ventBarDraw.h * (100.0f / acBody.h);
            float depth = 6.0f;
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::scale(model, glm::vec3(wworld, hworld, depth));
            renderer3D.drawCube(model, glm::vec3(ventBarDraw.color.r, ventBarDraw.color.g, ventBarDraw.color.b));
        }

        // lamp (small sphere-like cube on front)
        {
            float cx = lampDraw.x;
            float cy = lampDraw.y;
            glm::vec3 pos = mapToAC(cx, cy, 40.0f + 6.0f);
            float diam = lampDraw.radius * 2.0f * (240.0f / acBody.w);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            // scale to a flat quad (thin depth) so textured front face shows circular icon
            model = glm::scale(model, glm::vec3(diam, diam, 2.0f));
            glm::vec3 lampCol(lampDraw.color.r, lampDraw.color.g, lampDraw.color.b);
            renderer3D.drawTexturedCube(model, lampCircleTex, lampCol);
        }

        // screens: render desired/current temperatures onto the first two screens using text textures
        GLuint tempTex0 = 0; int tempW0 = 0; int tempH0 = 0;
        GLuint tempTex1 = 0; int tempW1 = 0; int tempH1 = 0;
        if (appState.isOn) {
            std::string s0 = std::to_string(static_cast<int>(appState.desiredTemp));
            std::string s1 = std::to_string(static_cast<int>(appState.currentTemp));
            // create textures for the two temperature displays
            textRenderer.createTextTexture(s0, digitColor, screenColor, 8, 64, tempTex0, tempW0, tempH0);
            textRenderer.createTextTexture(s1, digitColor, screenColor, 8, 64, tempTex1, tempW1, tempH1);
        }

        for (size_t i = 0; i < screensDraw.size(); ++i)
        {
            const auto& screen = screensDraw[i];
            float cx = screen.x + screen.w * 0.5f;
            float cy = screen.y + screen.h * 0.5f;
            glm::vec3 pos = mapToAC(cx, cy, 40.0f + 4.0f);
            float wworld = screen.w * (240.0f / acBody.w);
            float hworld = screen.h * (100.0f / acBody.h);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::scale(model, glm::vec3(wworld, hworld, 4.0f));
            if (i == 0 && tempTex0 != 0) {
                renderer3D.drawTexturedCube(model, tempTex0);
            } else if (i == 1 && tempTex1 != 0) {
                renderer3D.drawTexturedCube(model, tempTex1);
            } else {
                renderer3D.drawCube(model, glm::vec3(screenColor.r, screenColor.g, screenColor.b));
            }
        }
        // cleanup temporary temp textures
        if (tempTex0 != 0) glDeleteTextures(1, &tempTex0);
        if (tempTex1 != 0) glDeleteTextures(1, &tempTex1);

        // arrows (draw button halves with visible arrow glyphs)
        {
            float scaleX = 240.0f / acBody.w;
            float scaleY = 100.0f / acBody.h;
            float halfH = tempArrowDraw.h * 0.5f;
            float wworld = tempArrowDraw.w * scaleX;
            float hworld = halfH * scaleY;
            float cx = tempArrowDraw.x + tempArrowDraw.w * 0.5f;
            float cyTop = tempArrowDraw.y + halfH * 0.5f;
            float cyBot = tempArrowDraw.y + halfH + halfH * 0.5f;
            float zFront = 40.0f + 6.0f;

            auto drawArrowHalf = [&](float cy, bool isUp)
            {
                glm::vec3 pos = mapToAC(cx, cy, zFront);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, pos);
                model = glm::scale(model, glm::vec3(wworld, hworld, 4.0f));
                renderer3D.drawCube(model, glm::vec3(arrowBg.r, arrowBg.g, arrowBg.b));

                int steps = 6;
                float glyphH = halfH * 0.7f;
                float glyphW = tempArrowDraw.w * 0.6f;
                float stepH = glyphH / static_cast<float>(steps);
                for (int i = 0; i < steps; ++i)
                {
                    float t = (static_cast<float>(i) + 1.0f) / static_cast<float>(steps);
                    float w = glyphW * t;
                    float h = stepH * 0.85f;
                    float y = isUp ? (cy - glyphH * 0.5f + static_cast<float>(i) * stepH)
                                   : (cy + glyphH * 0.5f - (static_cast<float>(i) + 1.0f) * stepH);
                    glm::vec3 gpos = mapToAC(cx, y, zFront + 1.0f);
                    glm::mat4 gmodel = glm::mat4(1.0f);
                    gmodel = glm::translate(gmodel, gpos);
                    gmodel = glm::scale(gmodel, glm::vec3(w * scaleX, h * scaleY, 2.0f));
                    renderer3D.drawCube(gmodel, glm::vec3(arrowColor.r, arrowColor.g, arrowColor.b));
                }
            };

            drawArrowHalf(cyTop, true);
            drawArrowHalf(cyBot, false);
        }

        // bowl: place under the AC and render as a hollow container so it can be filled
        {
            float depth = 80.0f;
            float thicknessWorld = bowlThickness * (100.0f / acBody.h);
            if (appState.holdingBowl)
            {
                // place bowl in front of camera when held
                auto* ctx = static_cast<ResizeContext*>(glfwGetWindowUserPointer(window));
                if (ctx && ctx->camera) {
                    glm::mat4 view = ctx->camera->getViewMatrix();
                    glm::mat4 invView = glm::inverse(view);
                    glm::vec3 camPos(invView[3][0], invView[3][1], invView[3][2]);
                    glm::vec3 forward = glm::normalize(glm::vec3(invView * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
                    glm::vec3 pos = camPos + forward * 120.0f;
                    float innerWWorld = (bowlInnerW * (240.0f / acBody.w));
                    // held bowl inner height: box height is 40.0f, subtract wall thickness
                    float heldBoxHeight = 40.0f;
                    float maxInnerHeightWorld = heldBoxHeight - thicknessWorld;
                    if (maxInnerHeightWorld < 0.0f) maxInnerHeightWorld = 0.0f;
                    float waterHWorld = maxInnerHeightWorld * appState.waterLevel;

                    renderer3D.drawHollowBoxAt(pos, innerWWorld, heldBoxHeight, depth, thicknessWorld, glm::vec3(bowlOutline.color.r, bowlOutline.color.g, bowlOutline.color.b));
                    if (appState.waterLevel > 0.0f) {
                        // compute inner top Y and center of water column
                        float topY = pos.y + (heldBoxHeight * 0.5f) - thicknessWorld;
                        float centerY = topY - waterHWorld * 0.5f;
                        glm::mat4 wmodel = glm::mat4(1.0f);
                        wmodel = glm::translate(wmodel, glm::vec3(pos.x, centerY, pos.z));
                        float innerDepth = depth - 2.0f * thicknessWorld;
                        if (innerDepth < 2.0f) innerDepth = 2.0f;
                        wmodel = glm::scale(wmodel, glm::vec3(innerWWorld, waterHWorld, innerDepth));
                        renderer3D.drawCube(wmodel, glm::vec3(waterColor.r, waterColor.g, waterColor.b));
                    }
                    if (appState.waterLevel >= 1.0f) droplets.clear();
                }
            }
            else
            {
                // default on-floor bowl
                float wworld = bowlDraw.w * (240.0f / acBody.w);
                float hworld = bowlDraw.h * (100.0f / acBody.h);
                float bowlFullHeight = hworld * 0.5f;
                float acHalfHeight = 100.0f * 0.5f;
                float gap = 300.0f;
                glm::vec3 pos = glm::vec3(0.0f, -acHalfHeight - (bowlFullHeight * 0.5f) - gap, 0.0f);

                renderer3D.drawHollowBoxAt(pos, wworld, bowlFullHeight, depth, thicknessWorld, glm::vec3(bowlOutline.color.r, bowlOutline.color.g, bowlOutline.color.b));

                // place toilet at a fixed world position behind the player (computed once)
                static bool toiletWorldSet = false;
                static glm::vec3 toiletWorldPos(0.0f);
                auto* ctxCam = static_cast<ResizeContext*>(glfwGetWindowUserPointer(window));
                if (!toiletWorldSet && ctxCam && ctxCam->camera) {
                    glm::mat4 view = ctxCam->camera->getViewMatrix();
                    glm::mat4 invView = glm::inverse(view);
                    glm::vec3 camPos(invView[3][0], invView[3][1], invView[3][2]);
                    glm::vec3 camForward = glm::normalize(glm::vec3(invView * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
                    toiletWorldPos = camPos - camForward * 300.0f; // place 300 units behind initial camera
                    toiletWorldPos.y = pos.y + bowlFullHeight; // raise so toilet bottom aligns with bowl
                    toiletWorldSet = true;
                }
                glm::vec3 toiletPos = toiletWorldSet ? toiletWorldPos : (pos + glm::vec3(0.0f, 0.0f, -420.0f));

                if (toiletModelId >= 0) {
                    glm::mat4 m = glm::mat4(1.0f);
                    m = glm::translate(m, toiletPos);
                    m = glm::rotate(m, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    m = glm::rotate(m, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    // scale down a bit so model fits the scene
                    m = glm::scale(m, glm::vec3(6.0f));
                    renderer3D.drawModel(toiletModelId, m, glm::vec3(0.95f, 0.95f, 0.97f));
                } else {
                    glm::vec3 toiletColor = glm::vec3(0.95f, 0.95f, 0.97f);
                    float toiletRadius = wworld * 0.35f;
                    float toiletHeight = bowlFullHeight * 1.2f;
                    float toiletThickness = thicknessWorld * 1.2f;
                    renderer3D.drawHollowCylinderAt(toiletPos, toiletRadius, toiletHeight, toiletThickness, 32, toiletColor);
                    glm::mat4 tankModel = glm::mat4(1.0f);
                    glm::vec3 tankSize = glm::vec3(toiletRadius * 1.2f * 2.0f, toiletHeight * 0.6f, 40.0f);
                    glm::vec3 tankPos = toiletPos + glm::vec3(0.0f, toiletHeight * 0.5f + tankSize.y * 0.5f - 10.0f, -20.0f);
                    tankModel = glm::translate(tankModel, tankPos);
                    tankModel = glm::scale(tankModel, tankSize);
                    renderer3D.drawCube(tankModel, toiletColor);
                    glm::mat4 seatModel = glm::mat4(1.0f);
                    glm::vec3 seatSize = glm::vec3(toiletRadius * 1.6f * 2.0f, 6.0f, toiletRadius * 1.6f * 2.0f);
                    glm::vec3 seatPos = toiletPos + glm::vec3(0.0f, toiletHeight * 0.45f + 3.0f, 0.0f);
                    seatModel = glm::translate(seatModel, seatPos);
                    seatModel = glm::scale(seatModel, seatSize);
                    renderer3D.drawCube(seatModel, glm::vec3(0.9f, 0.9f, 0.91f));
                }

                // water inside bowl
                if (appState.waterLevel > 0.0f)
                {
                    // compute inner cavity max height and clamp water world height
                    float innerWWorld = (bowlInnerW * (240.0f / acBody.w));
                    float maxInnerHeightWorld = (bowlFullHeight - thicknessWorld);
                    if (maxInnerHeightWorld < 0.0f) maxInnerHeightWorld = 0.0f;
                    float waterHWorld = maxInnerHeightWorld * appState.waterLevel;
                    // bottom of inner cavity (world coords)
                    float innerBottomY = pos.y - (bowlFullHeight * 0.5f) + thicknessWorld;
                    float waterCenterY = innerBottomY + waterHWorld * 0.5f;
                    glm::vec3 wpos = glm::vec3(pos.x, waterCenterY, pos.z);
                    glm::mat4 wmodel = glm::mat4(1.0f);
                    wmodel = glm::translate(wmodel, wpos);
                    float innerDepth = depth - 2.0f * thicknessWorld;
                    if (innerDepth < 2.0f) innerDepth = 2.0f;
                    wmodel = glm::scale(wmodel, glm::vec3(innerWWorld, waterHWorld, innerDepth));
                    renderer3D.drawCube(wmodel, glm::vec3(waterColor.r, waterColor.g, waterColor.b));
                    if (appState.waterLevel >= 1.0f) droplets.clear();
                }
            }
        }

        // draw scene-light marker on top of 3D scene
        renderer3D.render();

        // now disable depth and draw text overlays as before
        glDisable(GL_DEPTH_TEST);

        // Render status icon onto the third screen as a colored patch on the model
        {
            float cx = screensDraw[2].x + screensDraw[2].w * 0.5f;
            float cy = screensDraw[2].y + screensDraw[2].h * 0.5f;
            glm::vec3 pos = mapToAC(cx, cy, 40.0f + 4.0f);
            float iconSizePixels = std::min(screensDraw[2].w, screensDraw[2].h) * 0.6f;
            float iconW = iconSizePixels * (240.0f / acBody.w);
            float iconH = iconSizePixels * (100.0f / acBody.h);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::scale(model, glm::vec3(iconW, iconH, 4.0f));

            // draw status icon on the rightmost screen using 3D primitives so it sits on the panel
            auto drawStatusIcon3D = [&](const RectShape& screen, float desired, float current)
            {
                float cx = screen.x + screen.w * 0.5f;
                float cy = screen.y + screen.h * 0.5f;
                float z = 40.0f + 6.0f; // slightly in front of screen
                glm::vec3 center = mapToAC(cx, cy, z);
                float scaleX = 240.0f / acBody.w;
                float scaleY = 100.0f / acBody.h;
                float wworld = screen.w * scaleX;
                float hworld = screen.h * scaleY;

                const float tolerance = 0.25f;
                float diff = desired - current;

                if (diff > tolerance)
                {
                    // Heat icon: approximate flame with stacked boxes (vertical taper)
                    int bands = 5;
                    for (int i = 0; i < bands; ++i)
                    {
                        float t = 1.0f - static_cast<float>(i) / static_cast<float>(bands);
                        float bw = wworld * (0.4f * t + 0.1f);
                        float bh = hworld * 0.12f;
                        float y = center.y - hworld * 0.25f + i * (bh * 0.9f);
                        glm::mat4 m = glm::mat4(1.0f);
                        m = glm::translate(m, glm::vec3(center.x, y, center.z + 1.0f));
                        m = glm::scale(m, glm::vec3(bw, bh, 2.0f));
                        renderer3D.drawCube(m, glm::vec3(0.96f, 0.46f, 0.28f));
                    }
                }
                else if (diff < -tolerance)
                {
                    // Snow icon: cross arms
                    float armW = wworld * 0.08f;
                    float armL = wworld * 0.6f;
                    glm::mat4 m1 = glm::mat4(1.0f);
                    m1 = glm::translate(m1, glm::vec3(center.x, center.y, center.z + 1.0f));
                    m1 = glm::scale(m1, glm::vec3(armL, armW, 2.0f));
                    renderer3D.drawCube(m1, glm::vec3(0.66f, 0.85f, 0.98f));
                    glm::mat4 m2 = glm::mat4(1.0f);
                    m2 = glm::translate(m2, glm::vec3(center.x, center.y, center.z + 1.0f));
                    m2 = glm::scale(m2, glm::vec3(armW, armL, 2.0f));
                    renderer3D.drawCube(m2, glm::vec3(0.66f, 0.85f, 0.98f));
                }
                else
                {
                    // Check icon: two segments
                    float dot = std::min(wworld, hworld) * 0.08f;
                    glm::vec3 p1(center.x - wworld*0.15f, center.y + hworld*0.05f, center.z + 1.0f);
                    glm::vec3 p2(center.x - wworld*0.02f, center.y - hworld*0.15f, center.z + 1.0f);
                    glm::vec3 p3(center.x + wworld*0.20f, center.y + hworld*0.18f, center.z + 1.0f);
                    // first segment
                    glm::mat4 mA = glm::mat4(1.0f);
                    glm::vec3 midA = (p1 + p2) * 0.5f;
                    glm::vec3 dirA = p2 - p1;
                    float lenA = glm::length(dirA);
                    mA = glm::translate(mA, glm::vec3(midA.x, midA.y, midA.z));
                    mA = glm::rotate(mA, atan2(dirA.y, dirA.x), glm::vec3(0.0f,0.0f,1.0f));
                    mA = glm::scale(mA, glm::vec3(lenA, dot, 2.0f));
                    renderer3D.drawCube(mA, glm::vec3(0.38f, 0.92f, 0.58f));
                    // second segment
                    glm::mat4 mB = glm::mat4(1.0f);
                    glm::vec3 midB = (p2 + p3) * 0.5f;
                    glm::vec3 dirB = p3 - p2;
                    float lenB = glm::length(dirB);
                    mB = glm::translate(mB, glm::vec3(midB.x, midB.y, midB.z));
                    mB = glm::rotate(mB, atan2(dirB.y, dirB.x), glm::vec3(0.0f,0.0f,1.0f));
                    mB = glm::scale(mB, glm::vec3(lenB, dot, 2.0f));
                    renderer3D.drawCube(mB, glm::vec3(0.38f, 0.92f, 0.58f));
                }
            };

            drawStatusIcon3D(screensDraw[2], appState.desiredTemp, appState.currentTemp);
        }


        if (!frameStats.empty())
        {
            // Ensure UI text and overlays are not culled by face-culling state
            GLboolean prevCull = glIsEnabled(GL_CULL_FACE);
            if (prevCull) glDisable(GL_CULL_FACE);

            float statsScale = 0.6f;
            float margin = 16.0f;
            textRenderer.drawText(frameStats, margin, margin, statsScale, digitColor);

            // show depth/cull mode indicators at top-right
            float indicatorScale = 0.6f;
            std::string depthStr = depthTestEnabled ? std::string("Depth: ON (T)") : std::string("Depth: OFF (T)");
            std::string cullStr = cullEnabled ? std::string("Cull: ON (C)") : std::string("Cull: OFF (C)");
            TextMetrics dm = textRenderer.measure(depthStr, indicatorScale);
            TextMetrics cm = textRenderer.measure(cullStr, indicatorScale);
            float iright = static_cast<float>(windowWidth) - margin;
            float dx = iright - dm.width;
            float dy = margin;
            textRenderer.drawText(depthStr, dx, dy, indicatorScale, digitColor);
            textRenderer.drawText(cullStr, iright - cm.width, dy + dm.height + 4.0f, indicatorScale, digitColor);

            // draw nameplate overlay if present
            if (nameplateTexture != 0)
            {
                float margin2 = 20.0f;
                float overlayX = static_cast<float>(windowWidth) - static_cast<float>(nameplateW) - margin2;
                float overlayY = static_cast<float>(windowHeight) - static_cast<float>(nameplateH) - margin2;

                float vertices[6][4] = {
                    { overlayX,                          overlayY + nameplateH, 0.0f, 0.0f },
                    { overlayX,                          overlayY,               0.0f, 1.0f },
                    { overlayX + nameplateW,             overlayY,               1.0f, 1.0f },

                    { overlayX,                          overlayY + nameplateH, 0.0f, 0.0f },
                    { overlayX + nameplateW,             overlayY,               1.0f, 1.0f },
                    { overlayX + nameplateW,             overlayY + nameplateH, 1.0f, 0.0f },
                };

                glUseProgram(overlayProgram);
                glUniform2f(overlayWindowSizeLoc, static_cast<float>(windowWidth), static_cast<float>(windowHeight));
                glUniform4f(overlayTintLoc, 1.0f, 1.0f, 1.0f, 1.0f);
                glUniform1i(overlayTextureLoc, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, nameplateTexture);

                glBindVertexArray(overlayVao);
                glBindBuffer(GL_ARRAY_BUFFER, overlayVbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);
            }

            // restore culling state
            if (prevCull) glEnable(GL_CULL_FACE);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        auto targetTime = frameStartTime + std::chrono::duration<double>(TARGET_FRAME_TIME);
        auto now = std::chrono::steady_clock::now();
        if (now < targetTime)
        {
            std::this_thread::sleep_until(targetTime); // coarse frame limiter to ~75 FPS
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
