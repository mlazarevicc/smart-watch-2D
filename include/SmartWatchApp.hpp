#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <random>

enum class AppState {
    Clock,
    Heart,
    Battery
};

class SmartWatchApp {
public:
    SmartWatchApp();

    bool init(GLFWwindow* window, int screenWidth, int screenHeight);
    void update(double currentTime);

    void render();

    // input callbacks
    void onKey(int key, int scancode, int action, int mods);
    void onMouseButton(int button, int action, int mods);

private:
    void updateTimeAndBattery(double currentTime);
    void updateBpmAndEkg(double currentTime, double deltaTime);

    void renderClockScreen();
    void renderHeartScreen();
    void renderBatteryScreen();
    void renderCursorAndOverlay();
    void renderWarningOverlay();

    GLFWwindow* window_;
    int screenWidth_;
    int screenHeight_;

    AppState currentState_;

    // Watch
    int timeHH_, timeMM_, timeSS_;
    double lastTimeSecond_;

    // EKG / BPM
    float bpm_;
    float ekgOffset_;
    bool isRunning_;
    double lastFrameTime_;
    double lastRandomChange_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> randBpm_;
    float bpmTargetRandom_;

    // Battery
    float batteryLevel_;
    float batteryTimer_;

    // Input
    double mouseX_;
    double mouseY_;
    float squeezeScale_;

    // Shaders i geometrija
    GLuint basicShader_;
    GLuint batteryShader_;
    GLuint VAO_;
    GLuint VBO_;

    // Teksture
    GLuint texArrowLeft_, texArrowRight_, texHeart_, texEKG_, texBatteryFrame_;
    GLuint texNumbers_[10];
    GLuint texColon_, texPercent_, texIDOverlay_, texWarningFull_;
};