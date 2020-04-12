#version 430 core

#define M_PI 3.1415926535897932384626433832795

// ***** COMPUTE SHADER INPUT *****
layout(local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;

// ***** COMPUTE SHADER OUTPUT *****

// ***** COMPUTE SHADER UNIFORMS *****
layout(shared, binding = 4) uniform FluidDynamics {
    uint maxParticles;
    uint maxNeighbors;
    uint mapSize;
    float supportRadius;
    float dt;
    uint solverIters;
    float restDensity;
    float epsilon;
    float collisionEpsilon;
    float kpoly;
    float kspiky;
    float scorr;
    float dcorr;
    int pcorr;
    float kxsph;
    float vortEpsilon;
    float time;
} fluid;

// ***** COMPUTE SHADER STRUCTS *****

// ***** COMPUTE SHADER BUFFERS *****
/*
    index = 0;
    position = 1;
    positionStar = 2;
    velocity = 3;
    newVelocity = 4;
    lambda = 5;
    deltaP = 6;
    color = 7;
    hashMap = 8;
    linkedList = 9;
    neighbors = 10;
    counter = 0;
*/
layout(std430, binding=1) buffer PosBuf {
    vec4 positions[];
};

layout(std430, binding=2) buffer UpdatedPosBuf {
    vec4 newPositions[];
};

layout(std430, binding=3) buffer velBuf {
    vec4 velocities[];
};

layout(std430, binding=6) buffer DeltaPBuf {
    vec4 deltaPs[];
};

// ***** COMPUTE SHADER SUBROUTINES *****
// ***** COMPUTE SHADER HELPER FUNCTIONS *****

void main() {
    uint vIndex = gl_GlobalInvocationID.x;

    // Update position with deltaP
    newPositions[vIndex].xyz += deltaPs[vIndex].xyz;

    // Update velocity
    velocities[vIndex].xyz = (newPositions[vIndex].xyz - positions[vIndex].xyz)/fluid.dt;
}
