#version 430 core

// ***** VERTEX SHADER INPUT *****
layout(location=0) in vec3 vPos;
layout(location=2) in vec2 vTex;


// ***** VERTEX SHADER OUTPUT *****
layout(location=0) out vec2 fTex;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_ClipDistance[];
};


// ***** VERTEX SHADER UNIFORMS *****
layout(shared, binding = 0) uniform Matricies{
    mat4 modelView;
    mat4 projection;
    mat4 view;
    mat4 normal;
    mat4 viewPort;
} mtx;

void main() {
    vec4 posVec = mtx.modelView * vec4(vPos, 1.0);
    // Calculate position
    gl_Position = mtx.projection * posVec;
    // Pass along texCoord
    fTex = vTex;
}
