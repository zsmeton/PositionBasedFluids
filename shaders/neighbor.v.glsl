#version 430 core

// ***** VERTEX SHADER INPUT *****
layout(location=0) in vec3 vPos;
layout(location=2) in uint index;

// ***** VERTEX SHADER OUTPUT *****
layout(location=0) out vec3 fPos;
layout(location=1) flat out uint fIndex;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_ClipDistance[];
};

// ***** VERTEX SHADER STRUCTS *****

// ***** VERTEX SHADER BUFFERS *****

// ***** VERTEX SHADER UNIFORMS *****
layout(shared, binding = 0) uniform Matricies{
    mat4 modelView;
    mat4 projection;
    mat4 view;
    mat4 normal;
    mat4 viewPort;
} mtx;

// ***** VERTEX SHADER SUBROUTINES *****
// ***** VERTEX SHADER HELPER FUNCTIONS *****

void main() {
    // Pass along index and position
    fIndex = index;
    fPos = vPos;


    // Calculate position of vertex

    vec4 posVec = mtx.modelView * vec4(vPos,1.0);
    if(index > 1000000000){
        posVec.x = -5.0;
    }

    // Calculate position
    gl_Position = mtx.projection * posVec;
}
