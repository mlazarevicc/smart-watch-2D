#include "SmartWatchApp.hpp"
#include "RenderUtils.hpp"
#include "Util.hpp"

#include <cmath>
#include <string>
#include <iostream>

SmartWatchApp::SmartWatchApp()
    : window_(nullptr),
      screenWidth_(800),
      screenHeight_(800),
      currentState_(AppState::Clock),
      timeHH_(23), timeMM_(59), timeSS_(55),
      lastTimeSecond_(0.0),
      bpm_(70.0f),
      ekgOffset_(0.0f),
      isRunning_(false),
      lastFrameTime_(0.0),
      lastRandomChange_(0.0),
      rng_(static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count())),
      randBpm_(60.0f, 80.0f),
      bpmTargetRandom_(70.0f),
      batteryLevel_(1.0f),
      batteryTimer_(0.0f),
      mouseX_(0.0),
      mouseY_(0.0),
      squeezeScale_(1.0f),
      basicShader_(0),
      batteryShader_(0),
      VAO_(0),
      VBO_(0),
      texArrowLeft_(0), texArrowRight_(0), texHeart_(0), texEKG_(0), texBatteryFrame_(0),
      texColon_(0), texPercent_(0), texIDOverlay_(0), texWarningFull_(0)
{
    for (int i = 0; i < 10; ++i) texNumbers_[i] = 0;
}

bool SmartWatchApp::init(GLFWwindow* window, int screenWidth, int screenHeight) {
    window_ = window;
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;

    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    basicShader_   = createShader("shaders/basic.vert",  "shaders/basic.frag");
    batteryShader_ = createShader("shaders/basic.vert",  "shaders/battery.frag");

    if (!basicShader_ || !batteryShader_) {
        std::cerr << "Greska pri ucitavanju shadera!\n";
        return false;
    }

    formQuadVAO(VAO_, VBO_);

    preprocessTexture(texArrowLeft_,  "res/arrow_left.png");
    preprocessTexture(texArrowRight_, "res/arrow_right.png");
    preprocessTexture(texHeart_,      "res/heart.png");
    preprocessTexture(texEKG_,        "res/ekg.png");
    preprocessTexture(texBatteryFrame_, "res/battery_frame.png");
    preprocessTexture(texColon_,      "res/colon.png");
    preprocessTexture(texPercent_,    "res/percent.png");
    preprocessTexture(texIDOverlay_,  "res/id_overlay.png");
    preprocessTexture(texWarningFull_,"res/warning_full.png");

    for (int i = 0; i < 10; ++i) {
        std::string p = "res/" + std::to_string(i) + ".png";
        preprocessTexture(texNumbers_[i], p.c_str());
    }

    double t = glfwGetTime();
    lastFrameTime_   = t;
    lastTimeSecond_  = t;
    lastRandomChange_ = t;
    bpmTargetRandom_ = randBpm_(rng_);

    return true;
}

void SmartWatchApp::update(double currentTime) {

    double deltaTime = currentTime - lastFrameTime_;
    lastFrameTime_ = currentTime;
    if (deltaTime < 0.0) deltaTime = 0.0;

    glfwGetCursorPos(window_, &mouseX_, &mouseY_);

    const float speed = 0.2f; // promena po sekundi
    if (isRunning_) {
        squeezeScale_ -= speed * static_cast<float>(deltaTime);
        if (squeezeScale_ < 0.2f) squeezeScale_ = 0.2f;
    } else {
        squeezeScale_ += speed * static_cast<float>(deltaTime);
        if (squeezeScale_ > 1.0f) squeezeScale_ = 1.0f;
    }

    updateTimeAndBattery(currentTime);
    updateBpmAndEkg(currentTime, deltaTime);

    // EKG offset - pomera teksturu
    ekgOffset_ += (bpm_ / 100.0f) * static_cast<float>(deltaTime);
}

void SmartWatchApp::updateTimeAndBattery(double currentTime) {
    if (currentTime - lastTimeSecond_ >= 1.0) {
        timeSS_++;
        if (timeSS_ >= 60) {
            timeSS_ = 0;
            timeMM_++;
            if (timeMM_ >= 60) {
                timeMM_ = 0;
                timeHH_++;
                if (timeHH_ >= 24) timeHH_ = 0;
            }
        }

        batteryTimer_ += 1.0f;
        if (batteryTimer_ >= 10.0f) {
            batteryLevel_ -= 0.01f;
            if (batteryLevel_ < 0.0f) batteryLevel_ = 0.0f;
            batteryTimer_ = 0.0f;
        }

        lastTimeSecond_ = currentTime;
    }
}

void SmartWatchApp::updateBpmAndEkg(double currentTime, double deltaTime) {
    if (isRunning_) {
        float target = 220.0f;
        bpm_ += (target - bpm_) * static_cast<float>(deltaTime) * 0.5f;
    } else {
        if (currentTime - lastRandomChange_ >= 0.5) {
            bpmTargetRandom_ = randBpm_(rng_);
            lastRandomChange_ = currentTime;
        }
        bpm_ += (bpmTargetRandom_ - bpm_) * static_cast<float>(deltaTime * 1.5);
    }
}

void SmartWatchApp::render() {
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    switch (currentState_) {
        case AppState::Clock:   renderClockScreen();  break;
        case AppState::Heart:   renderHeartScreen();  break;
        case AppState::Battery: renderBatteryScreen(); break;
    }

    renderCursorAndOverlay();
    renderWarningOverlay();
}

void SmartWatchApp::renderClockScreen() {
    float numW = 0.1f;
    float numH = 0.15f;
    float startX = -0.4f;

    // HH
    drawElement(basicShader_, VAO_, texNumbers_[timeHH_ / 10], startX, 0.0f, numW, numH);
    drawElement(basicShader_, VAO_, texNumbers_[timeHH_ % 10], startX + 0.15f, 0.0f, numW, numH);

    drawElement(basicShader_, VAO_, texColon_, startX + 0.28f, 0.0f, numW, numH);

    // MM
    drawElement(basicShader_, VAO_, texNumbers_[timeMM_ / 10], startX + 0.4f, 0.0f, numW, numH);
    drawElement(basicShader_, VAO_, texNumbers_[timeMM_ % 10], startX + 0.55f, 0.0f, numW, numH);

    drawElement(basicShader_, VAO_, texColon_, startX + 0.68f, 0.0f, numW, numH);

    // SS
    drawElement(basicShader_, VAO_, texNumbers_[timeSS_ / 10], startX + 0.8f, 0.0f, numW, numH);
    drawElement(basicShader_, VAO_, texNumbers_[timeSS_ % 10], startX + 0.95f, 0.0f, numW, numH);

    drawElement(basicShader_, VAO_, texArrowRight_, 0.85f, 0.0f, 0.08f, 0.1f);
}

void SmartWatchApp::renderHeartScreen() {
    if (bpm_ > 200.0f) {
        return;
    }

    float r = 1.0f, g = 1.0f, b = 1.0f;

    drawElement(basicShader_, VAO_, texArrowLeft_,  -0.85f, 0.0f, 0.08f, 0.08f, 0,0,1,1, r,g,b,1.0f);
    drawElement(basicShader_, VAO_, texArrowRight_,  0.85f, 0.0f, 0.08f, 0.08f, 0,0,1,1, r,g,b,1.0f);

    // EKG
    float ekgScale = 1.0f + (bpm_ / 100.0f);
    drawElement(basicShader_, VAO_, texEKG_, 0.0f, 0.0f, 0.7f * squeezeScale_, 0.4f,
                ekgOffset_, 0.0f, ekgScale, 1.0f, r,g,b,1.0f);

    int displayBPM = static_cast<int>(std::round(bpm_));
    int hundreds = displayBPM / 100;
    int tens     = (displayBPM / 10) % 10;
    int ones     = displayBPM % 10;

    float numW = 0.07f, numH = 0.1f;
    float baseX = -0.15f, baseY = 0.45f;

    if (hundreds > 0) {
        drawElement(basicShader_, VAO_, texNumbers_[hundreds], baseX, baseY, numW, numH);
        drawElement(basicShader_, VAO_, texNumbers_[tens],     baseX + 0.12f, baseY, numW, numH);
        drawElement(basicShader_, VAO_, texNumbers_[ones],     baseX + 0.24f, baseY, numW, numH);
    } else {
        drawElement(basicShader_, VAO_, texNumbers_[tens], baseX + 0.12f, baseY, numW, numH);
        drawElement(basicShader_, VAO_, texNumbers_[ones], baseX + 0.24f, baseY, numW, numH);
    }
}

void SmartWatchApp::renderBatteryScreen() {
    drawElement(basicShader_, VAO_, texArrowLeft_, -0.85f, 0.0f, 0.08f, 0.08f);

    drawElement(basicShader_, VAO_, texBatteryFrame_, 0.0f, 0.0f, 0.5f, 0.40f);
    drawBatteryQuad(batteryShader_, VAO_, 0.025f, 0.0f, 0.40f, 0.15f, batteryLevel_);

    int percent = static_cast<int>(std::round(batteryLevel_ * 100.0f));
    float txtW = 0.05f, txtH = 0.08f, baseX = -0.06f, baseY = 0.35f;

    if (percent >= 100) {
        drawElement(basicShader_, VAO_, texNumbers_[1], baseX - 0.06f, baseY, txtW, txtH);
        drawElement(basicShader_, VAO_, texNumbers_[0], baseX + 0.04f, baseY, txtW, txtH);
        drawElement(basicShader_, VAO_, texNumbers_[0], baseX + 0.14f, baseY, txtW, txtH);
        drawElement(basicShader_, VAO_, texPercent_,    baseX + 0.26f, baseY, txtW, txtH);
    } else if (percent < 10) {
        if (percent > 0) {
            drawElement(basicShader_, VAO_, texNumbers_[percent], baseX + 0.04f, baseY, txtW, txtH);
            drawElement(basicShader_, VAO_, texPercent_,          baseX + 0.14f, baseY, txtW, txtH);
        } else {
            drawElement(basicShader_, VAO_, texPercent_, baseX + 0.04f, baseY, txtW, txtH);
        }
    } else {
        drawElement(basicShader_, VAO_, texNumbers_[percent/10], baseX,       baseY, txtW, txtH);
        drawElement(basicShader_, VAO_, texNumbers_[percent%10], baseX + 0.10f, baseY, txtW, txtH);
        drawElement(basicShader_, VAO_, texPercent_,            baseX + 0.20f, baseY, 0.04f, 0.06f);
    }
}

void SmartWatchApp::renderCursorAndOverlay() {
    float mx = static_cast<float>(mouseX_) / (screenWidth_ / 2.0f) - 1.0f;
    float my = - (static_cast<float>(mouseY_) / (screenHeight_ / 2.0f) - 1.0f);
    drawElement(basicShader_, VAO_, texHeart_, mx, my, 0.06f * squeezeScale_, 0.06f * squeezeScale_);

    float overlayW = 0.28f, overlayH = 0.12f;
    float overlayX = 1.0f - overlayW / 2.0f - 0.02f;
    float overlayY = 1.0f - overlayH / 2.0f - 0.02f;

    float ox = overlayX * 2.0f - 1.0f;
    float oy = overlayY * 2.0f - 1.0f;

    drawElement(basicShader_, VAO_, texIDOverlay_, ox, oy, overlayW, overlayH,
                0,0,1,1, 1,1,1,0.6f);
}

void SmartWatchApp::renderWarningOverlay() {
    if (bpm_ <= 200.0f)
        return;

    if (texWarningFull_ != 0) {
        drawElement(basicShader_, VAO_, texWarningFull_, 0.0f, 0.0f, 1.0f, 0.5f, 0,0,1,1, 1,1,1,1.0f);
        // drawElement(basicShader_, VAO_, texWarningFull_, 0.0f, 0.0f, 2.0f, 2.0f);
    } else {
        drawElement(basicShader_, VAO_, 0, 0.0f, 0.0f, 2.0f, 2.0f, 0,0,1,1,
                    1.0f, 0.2f, 0.2f, 0.55f);
    }
}

void SmartWatchApp::onKey(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS)  isRunning_ = true;
        if (action == GLFW_RELEASE) isRunning_ = false;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

void SmartWatchApp::onMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwGetCursorPos(window_, &mouseX_, &mouseY_);

        float mxNorm = static_cast<float>(mouseX_) / (screenWidth_ / 2.0f) - 1.0f;
        // float myNorm = - (static_cast<float>(mouseY_) / (screenHeight_ / 2.0f) - 1.0f);

        if (mxNorm > 0.7f) {
            if (currentState_ == AppState::Clock)      currentState_ = AppState::Heart;
            else if (currentState_ == AppState::Heart) currentState_ = AppState::Battery;
        }
        if (mxNorm < -0.7f) {
            if (currentState_ == AppState::Battery)    currentState_ = AppState::Heart;
            else if (currentState_ == AppState::Heart) currentState_ = AppState::Clock;
        }
    }
}