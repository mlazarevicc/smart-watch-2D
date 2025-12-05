#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

// Uniformi za modifikaciju teksture (EKG efekat)
uniform vec4 u_uvTransform;

uniform vec2 uPos;
uniform vec2 uScale;

void main()
{
    gl_Position = vec4(aPos.xy * uScale + uPos, 0.0, 1.0);
    
    // Logika za EKG animaciju
    TexCoord = (aTexCoord * u_uvTransform.zw) + u_uvTransform.xy;
}