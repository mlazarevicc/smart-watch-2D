#pragma once

#include <GL/glew.h>

unsigned int loadImageToTexture(const char* filepath);

void preprocessTexture(unsigned& texture, const char* filepath);

void formQuadVAO(unsigned int& outVAO, unsigned int& outVBO);

void drawElement(unsigned int shader, unsigned int VAO_local, unsigned int texture,
                 float x, float y, float w, float h,
                 float uvX = 0.0f, float uvY = 0.0f, float uvW = 1.0f, float uvH = 1.0f,
                 float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

void drawBatteryQuad(unsigned int shader, unsigned int VAO_local,
                     float x, float y, float w, float h, float level);