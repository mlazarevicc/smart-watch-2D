#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <random>

#include "Util.hpp"

const int VTARGET_FPS = 75;
const double FRAME_TIME = 1.0 / VTARGET_FPS;

int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 800;

enum AppState { STATE_CLOCK, STATE_HEART, STATE_BATTERY };
AppState currentState = STATE_CLOCK;

// Watch
int timeHH = 23, timeMM = 59, timeSS = 55;
double lastTimeSecond = 0.0;

// EKG / BPM
float bpm = 70.0f;
float ekgOffset = 0.0f;
bool isRunning = false;
double lastTimeFrame = 0.0;

// Battery
float batteryLevel = 1.0f;
float batteryTimer = 0.0f;

// Input / squeeze
double mouseX, mouseY;
float squeezeScale = 1.0f;

// Random generator for BPM
std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
std::uniform_real_distribution<float> randBpm(60.0f, 80.0f);
float bpmTargetRandom = randBpm(rng);
double lastRandomChange = 0.0;

// Textures
unsigned int texArrowLeft=0, texArrowRight=0, texHeart=0, texEKG=0, texBatteryFrame=0;
unsigned int texNumbers[10];
unsigned int texColon=0, texPercent=0, texIDOverlay=0, texWarningFull=0;
unsigned int VAO = 0, VBO = 0;


void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath);
    glBindTexture(GL_TEXTURE_2D, texture);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void formQuadVAO(unsigned int& outVAO, unsigned int& outVBO) {
    float vertices[] = {
        // pos         // uv
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };

    glGenVertexArrays(1, &outVAO);
    glGenBuffers(1, &outVBO);
    glBindVertexArray(outVAO);
    glBindBuffer(GL_ARRAY_BUFFER, outVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void drawElement(unsigned int shader, unsigned int VAO_local, unsigned int texture,
                 float x, float y, float w, float h,
                 float uvX=0.0f, float uvY=0.0f, float uvW=1.0f, float uvH=1.0f,
                 float r=1.0f, float g=1.0f, float b=1.0f, float a=1.0f)
{
    glUseProgram(shader);
    if (texture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shader, "u_image"), 0);
    } else {
        // if no texture, bind 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glUniform2f(glGetUniformLocation(shader, "uPos"), x, y);
    glUniform2f(glGetUniformLocation(shader, "uScale"), w, h);
    glUniform4f(glGetUniformLocation(shader, "u_uvTransform"), uvX, uvY, uvW, uvH);
    glUniform4f(glGetUniformLocation(shader, "u_colorObj"), r, g, b, a);

    glBindVertexArray(VAO_local);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// drawBatteryQuad uses battery shader (expects u_batteryLevel, optionally u_image)
void drawBatteryQuad(unsigned int shader, unsigned int VAO_local, float x, float y, float w, float h, float level) {
    glUseProgram(shader);
    glUniform2f(glGetUniformLocation(shader, "uPos"), x, y);
    glUniform2f(glGetUniformLocation(shader, "uScale"), w, h);
    glUniform1f(glGetUniformLocation(shader, "u_batteryLevel"), level);
    glUniform4f(glGetUniformLocation(shader, "u_uvTransform"), 0,0,1,1);
    glBindVertexArray(VAO_local);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// Input callbacks
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) isRunning = true;
        if (action == GLFW_RELEASE) isRunning = false;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        float mx = (float)mouseX / (SCREEN_WIDTH / 2) - 1.0f;

        if (mx > 0.7f) {
            if (currentState == STATE_CLOCK) currentState = STATE_HEART;
            else if (currentState == STATE_HEART) currentState = STATE_BATTERY;
        }
        if (mx < -0.7f) {
            if (currentState == STATE_BATTERY) currentState = STATE_HEART;
            else if (currentState == STATE_HEART) currentState = STATE_CLOCK;
        }
    }
}

int main() {
    if (!glfwInit()) return -1;
    // Create fullscreen window on primary monitor
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    SCREEN_WIDTH = mode->width;
    SCREEN_HEIGHT = mode->height;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pametni Sat", primary, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load shaders
    unsigned int basicShader = createShader("shaders/basic.vert", "shaders/basic.frag");
    unsigned int batteryShader = createShader("shaders/basic.vert", "shaders/battery.frag");

    // Geometry
    unsigned int VBO;
    formQuadVAO(VAO, VBO);

    // Load textures (adjust paths)
    preprocessTexture(texArrowLeft, "res/arrow_left.png");
    preprocessTexture(texArrowRight, "res/arrow_right.png");
    preprocessTexture(texHeart, "res/heart.png");
    preprocessTexture(texEKG, "res/ekg.png");
    preprocessTexture(texBatteryFrame, "res/battery_frame.png");
    preprocessTexture(texColon, "res/colon.png");
    preprocessTexture(texPercent, "res/percent.png");
    preprocessTexture(texIDOverlay, "res/id_overlay.png");
    preprocessTexture(texWarningFull, "res/warning_full.png");

    for (int i=0;i<10;i++){
        std::string p = "res/" + std::to_string(i) + ".png";
        preprocessTexture(texNumbers[i], p.c_str());
    }

    // Timing
    lastTimeFrame = glfwGetTime();
    lastTimeSecond = glfwGetTime();
    lastRandomChange = glfwGetTime();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double frameStart = glfwGetTime();

        glfwPollEvents();
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Input handling for squeezeScale (running)
        if (isRunning) {
            squeezeScale -= 0.2f * (float)FRAME_TIME; 
            if (squeezeScale < 0.2f) squeezeScale = 0.2f;
        } else {
            squeezeScale += 0.2f * (float)FRAME_TIME; 
            if (squeezeScale > 1.0f) squeezeScale = 1.0f;
        }

        // Delta time
        double currentFrame = glfwGetTime();
        double deltaTime = currentFrame - lastTimeFrame;
        lastTimeFrame = currentFrame;

        // Seconds tick - update clock + battery timer
        if (currentFrame - lastTimeSecond >= 1.0) {
            timeSS++;
            if (timeSS >= 60) { timeSS = 0; timeMM++; if (timeMM>=60) { timeMM=0; timeHH++; if (timeHH>=24) timeHH=0; } }
            batteryTimer += 1.0f;
            if (batteryTimer >= 10.0f) {
                batteryLevel -= 0.01f;
                if (batteryLevel < 0.0f) batteryLevel = 0.0f;
                batteryTimer = 0.0f;
            }
            lastTimeSecond = currentFrame;
        }

        // BPM logic:
        if (isRunning) {
            float target = 220.0f;
            bpm += (target - bpm) * (float)(deltaTime) * 0.5;
        } else {
            if (currentFrame - lastRandomChange >= 0.5) {
                bpmTargetRandom = randBpm(rng);
                lastRandomChange = currentFrame;
            }
            // smooth approach to random target
            bpm += (bpmTargetRandom - bpm) * (float)(deltaTime * 1.5);
        }

        // ekg offset movement speed depends on bpm
        ekgOffset += (bpm / 100.0f) * (float)deltaTime;

        // RENDER
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // SCREEN: CLOCK
        if (currentState == STATE_CLOCK) {
            // Draw the time HH:MM:SS 
            // Digit positioning
            float numW = 0.1f; float numH = 0.15f; float startX = -0.4f;
            
            // HH
            drawElement(basicShader, VAO, texNumbers[timeHH / 10], startX, 0.0f, numW, numH);
            drawElement(basicShader, VAO, texNumbers[timeHH % 10], startX + 0.15f, 0.0f, numW, numH);

            drawElement(basicShader, VAO, texColon, startX + 0.28f, 0.0f, numW, numH);

            // MM
            drawElement(basicShader, VAO, texNumbers[timeMM / 10], startX + 0.4f, 0.0f, numW, numH);
            drawElement(basicShader, VAO, texNumbers[timeMM % 10], startX + 0.55f, 0.0f, numW, numH);

            drawElement(basicShader, VAO, texColon, startX + 0.68f, 0.0f, numW, numH);

            // SS
            drawElement(basicShader, VAO, texNumbers[timeSS / 10], startX + 0.8f, 0.0f, numW, numH);
            drawElement(basicShader, VAO, texNumbers[timeSS % 10], startX + 0.95f, 0.0f, numW, numH);

            // Right Arrow
            drawElement(basicShader, VAO, texArrowRight, 0.85f, 0.0f, 0.08f, 0.1f);
        }
        // EKG/HEART screen
        else if (currentState == STATE_HEART) {
            float r=1,g=1,b=1;
            if (bpm > 200.0f) {
                // show fullscreen warning overlay (see below)
            } else {
                // show arrows and ekg
                drawElement(basicShader, VAO, texArrowLeft, -0.85f, 0.0f, 0.08f, 0.08f, 0,0,1,1, r,g,b,1.0f);
                drawElement(basicShader, VAO, texArrowRight, 0.85f, 0.0f, 0.08f, 0.08f, 0,0,1,1, r,g,b,1.0f);

                float ekgScale = 1.0f + (bpm / 100.0f);
                drawElement(basicShader, VAO, texEKG, 0.0f, 0.0f, 0.7f * squeezeScale, 0.4f,
                            ekgOffset, 0.0f, ekgScale, 1.0f, r,g,b,1.0f);

                // BPM numbers
                int displayBPM = (int)std::round(bpm);
                int hundreds = displayBPM / 100;
                int tens = (displayBPM / 10) % 10;
                int ones = displayBPM % 10;
                float numW = 0.07f, numH = 0.1f;
                float baseX = -0.15f, baseY = 0.45f;
                if (hundreds > 0) {
                    drawElement(basicShader, VAO, texNumbers[hundreds], baseX, baseY, numW, numH);
                    drawElement(basicShader, VAO, texNumbers[tens], baseX + 0.12f, baseY, numW, numH);
                    drawElement(basicShader, VAO, texNumbers[ones], baseX + 0.24f, baseY, numW, numH);
                } else {
                    drawElement(basicShader, VAO, texNumbers[tens], baseX + 0.12f, baseY, numW, numH);
                    drawElement(basicShader, VAO, texNumbers[ones], baseX + 0.24f, baseY, numW, numH);
                }
            }
        }
        // BATTERY
        else if (currentState == STATE_BATTERY) {
            drawElement(basicShader, VAO, texArrowLeft, -0.85f, 0.0f, 0.08f, 0.08f);
            drawElement(basicShader, VAO, texBatteryFrame, 0.0f, 0.0f, 0.5f, 0.40f);

            drawBatteryQuad(batteryShader, VAO, 0.025f, 0.0f, 0.40f, 0.15f, batteryLevel);

            // percentage text and percent sign
            int percent = (int)std::round(batteryLevel * 100.0f);
            float txtW = 0.05f, txtH = 0.08f, baseX = -0.06f, baseY = 0.35f;
            if (percent >= 100) {
                drawElement(basicShader, VAO, texNumbers[1], baseX-0.06f, baseY, txtW, txtH);
                drawElement(basicShader, VAO, texNumbers[0], baseX+0.04f, baseY, txtW, txtH);
                drawElement(basicShader, VAO, texNumbers[0], baseX+0.14f, baseY, txtW, txtH);
                drawElement(basicShader, VAO, texPercent, baseX+0.26f, baseY, txtW, txtH);
            } else if (percent < 10) {
                // show single digit only (no leading 0)
                if (percent > 0) {
                    drawElement(basicShader, VAO, texNumbers[percent], baseX+0.04f, baseY, txtW, txtH);
                    drawElement(basicShader, VAO, texPercent, baseX+0.14f, baseY, txtW, txtH);
                } else {
                    // if exactly 0, do not show '0' per requirement -> show just percent sign or nothing
                    drawElement(basicShader, VAO, texPercent, baseX+0.04f, baseY, txtW, txtH);
                }
            } else {
                drawElement(basicShader, VAO, texNumbers[percent/10], baseX, baseY, txtW, txtH);
                drawElement(basicShader, VAO, texNumbers[percent%10], baseX+0.10f, baseY, txtW, txtH);
                drawElement(basicShader, VAO, texPercent, baseX+0.2f, baseY, 0.04f, 0.06f);
            }
        }

        // Cursor heart always drawn
        float mx = (float)mouseX / (SCREEN_WIDTH / 2) - 1.0f;
        float my = -((float)mouseY / (SCREEN_HEIGHT / 2) - 1.0f);
        drawElement(basicShader, VAO, texHeart, mx, my, 0.06f * squeezeScale, 0.06f * squeezeScale);

        // ID overlay in top-right corner (semi-transparent)
        float overlayW = 0.28f, overlayH = 0.12f;
        float overlayX = 1.0f - overlayW/2.0f - 0.02f; // normalized coords
        float overlayY = 1.0f - overlayH/2.0f - 0.02f;

        // overlayX, overlayY currently in normalized 0..1; convert:
        float ox = (overlayX * 2.0f) - 1.0f;
        float oy = (overlayY * 2.0f) - 1.0f;
        drawElement(basicShader, VAO, texIDOverlay, ox, oy, overlayW, overlayH, 0,0,1,1, 1,1,1,0.6f);

        // Fullscreen WARNING if bpm > 200
        if (bpm > 200.0f) {
            if (texWarningFull != 0) {
                drawElement(basicShader, VAO, texWarningFull, 0.0f, 0.0f, 1.0f, 0.5f, 0,0,1,1, 1,1,1,1.0f);
                // drawElement(basicShader, VAO, texWarningFull, 0.0f, 0.0f, 2.0f, 2.0f);
            } else {
                drawElement(basicShader, VAO, 0, 0.0f, 0.0f, 2.0f, 2.0f, 0,0,1,1, 1.0f, 0.2f, 0.2f, 0.55f);
            }
        }

        glfwSwapBuffers(window);

        // Frame limiter to ~75 FPS: sleep the remaining time if frame too fast
        double frameEnd = glfwGetTime();
        double elapsed = frameEnd - frameStart;
        double toWait = FRAME_TIME - elapsed;
        if (toWait > 0.0) {
            auto sleepMs = (int)((toWait - 0.001) * 1000.0);
            if (sleepMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            while ((glfwGetTime() - frameStart) < FRAME_TIME) { /* spin */ }
        }
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}