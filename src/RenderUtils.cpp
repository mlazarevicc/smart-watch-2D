#include "RenderUtils.hpp"
#include "Util.hpp"   // createShader, loadImageToTexture, ...
#include <GL/glew.h>

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

    // position (layout = 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coord (layout = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void drawElement(unsigned int shader, unsigned int VAO_local, unsigned int texture,
                 float x, float y, float w, float h,
                 float uvX, float uvY, float uvW, float uvH,
                 float r, float g, float b, float a)
{
    glUseProgram(shader);

    if (texture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        GLint texLoc = glGetUniformLocation(shader, "u_image");
        if (texLoc >= 0)
            glUniform1i(texLoc, 0);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLint posLoc = glGetUniformLocation(shader, "uPos");
    GLint scaleLoc = glGetUniformLocation(shader, "uScale");
    GLint uvLoc = glGetUniformLocation(shader, "u_uvTransform");
    GLint colLoc = glGetUniformLocation(shader, "u_colorObj");

    if (posLoc >= 0)   glUniform2f(posLoc, x, y);
    if (scaleLoc >= 0) glUniform2f(scaleLoc, w, h);
    if (uvLoc >= 0)    glUniform4f(uvLoc, uvX, uvY, uvW, uvH);
    if (colLoc >= 0)   glUniform4f(colLoc, r, g, b, a);

    glBindVertexArray(VAO_local);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawBatteryQuad(unsigned int shader, unsigned int VAO_local,
                     float x, float y, float w, float h, float level)
{
    glUseProgram(shader);

    GLint posLoc   = glGetUniformLocation(shader, "uPos");
    GLint scaleLoc = glGetUniformLocation(shader, "uScale");
    GLint levelLoc = glGetUniformLocation(shader, "u_batteryLevel");
    GLint uvLoc    = glGetUniformLocation(shader, "u_uvTransform");

    if (posLoc >= 0)   glUniform2f(posLoc, x, y);
    if (scaleLoc >= 0) glUniform2f(scaleLoc, w, h);
    if (levelLoc >= 0) glUniform1f(levelLoc, level);
    if (uvLoc >= 0)    glUniform4f(uvLoc, 0.0f, 0.0f, 1.0f, 1.0f);

    glBindVertexArray(VAO_local);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}