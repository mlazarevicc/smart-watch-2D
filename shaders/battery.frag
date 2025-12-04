#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform float u_batteryLevel; 

void main()
{
    // Određivanje boje
    vec4 color;
    if (u_batteryLevel <= 0.1) {
        color = vec4(1.0, 0.0, 0.0, 1.0); // Crvena (< 10%)
    } else if (u_batteryLevel <= 0.2) {
        color = vec4(1.0, 1.0, 0.0, 1.0); // Žuta (< 20%)
    } else {
        color = vec4(0.0, 1.0, 0.0, 1.0); // Zelena (100% - 20%)
    }

    // Odbacujemo piksele levo od trenutnog nivoa punjenja 
    if (TexCoord.x < (1.0 - u_batteryLevel)) {
        discard;
    }

    FragColor = color;
}