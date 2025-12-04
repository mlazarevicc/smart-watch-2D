#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

// Tekstura objekta (EKG linija, strelice, brojevi, kursor srce)
uniform sampler2D u_image;

// Boja za mešanje (default je bela vec4(1.0)).
uniform vec4 u_colorObj; 

void main()
{
    vec4 texColor = texture(u_image, TexCoord);
    
    // Ako je tekstura providna (alpha < 0.1), odbacujemo piksel (koristimo za kursor srce)
    if(texColor.a < 0.1)
        discard;
        
    FragColor = texColor * u_colorObj;
}