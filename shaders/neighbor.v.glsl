#version 430 core

// ***** VERTEX SHADER INPUT *****
layout(location=0) in vec4 vPos;
layout(location=3) in uint index;

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

layout(shared, binding = 4) uniform FluidDynamics {
    uint maxParticles;
    uint mapSize;
    uint timestamp;
    float supportRadius;
    float time;
} fluid;

// ***** VERTEX SHADER SUBROUTINES *****
// ***** VERTEX SHADER HELPER FUNCTIONS *****

void main() {
    // Pass along index and position
    fIndex = index;
    fPos = vPos.xyz;


    // Calculate position of vertex

    vec4 posVec = mtx.modelView * vec4(vPos.xyz,1.0);
    if(index > fluid.maxParticles){
        posVec.x = -5.0;
    }

    // Calculate position
    gl_Position = mtx.projection * posVec;
}
