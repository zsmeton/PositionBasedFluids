#version 430 core

// ***** FRAGMENT SHADER INPUT *****
layout(location=0) in vec3 color;

// ***** FRAGMENT SHADER UNIFORMS *****


// ***** FRAGMENT SHADER OUTPUT *****
layout(location=0) out vec4 fragColorOut;


// ***** FRAGMENT SHADER SUBROUTINES *****

void main() {
    /*************************
     * Lighting + Color
     *************************/
    // Set
    fragColorOut = vec4(color,1.0);
}
