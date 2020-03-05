/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2019
 *
 *  Project: lab1
 *  File: textshaderv410.f.glsl
 *
 *  Description:
 *      Shader to display text to the screen
 *
 *  Author:
 *      Dr. Jeffrey Paone, Colorado School of Mines
 *
 *  Notes:
 *
 */

// we are using OpenGL 4.1 Core profile
#version 410 core

// ***** UNIFORMS *****
uniform sampler2D tex;
uniform vec4 color;

// ***** FRAGMENT SHADER INPUT *****
in vec2 vTexCoord;

// ***** FRAGMENT SHADER OUTPUT *****
out vec4 fragColorOut;

void main() {
    vec4 texel = texture(tex, vTexCoord);
    
    fragColorOut = vec4(1.0f, 1.0f, 1.0f, texel.r) * color;
}
