#version 430 core

// ***** VERTEX SHADER INPUT *****
layout(location=1) in vec4 vPos;
layout(location=3) in vec4 color;


// ***** VERTEX SHADER OUTPUT *****
layout(location=0) out vec3 fColor;

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
    vec4 posVec = mtx.modelView * vec4(vPos.xyz,1.0);
    // Calculate position
    gl_Position = mtx.projection * posVec;

    // pass color down
    fColor = color.xyz;
}
