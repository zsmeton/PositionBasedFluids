/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2019
 *
 *  Project: lab1
 *  File: textshaderv410.v.glsl
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
uniform mat4 MVP_Matrix;

// ***** ATTRIBUTES *****
in vec4 coord;

// ***** VERTEX SHADER OUTPUT *****
out vec2 vTexCoord;
out gl_PerVertex
{
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

void main() {
    gl_Position = MVP_Matrix * vec4( coord.xy, 0.0f, 1.0f );
    vTexCoord = coord.zw;
}
