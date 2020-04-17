#version 430 core

#define M_PI 3.1415926535897932384626433832795

// ***** COMPUTE SHADER INPUT *****
layout(local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;

// ***** COMPUTE SHADER OUTPUT *****

// ***** COMPUTE SHADER UNIFORMS *****
layout(binding=0, offset=0) uniform atomic_uint nextNodeCounter;

layout(shared, binding = 0) uniform Matricies{
    mat4 modelView;
    mat4 projection;
    mat4 view;
    mat4 normal;
    mat4 viewPort;
} mtx;

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
struct HashType {
    uint headNodeIndex;
};

struct NodeType {
    uint nextNodeIndex;
    uint particleIndex;
};

struct NeighborType {
    uint count;
    uint neighboring[500];
};

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
layout(std430, binding=2) buffer UpdatedPosBuf {
    vec4 newPositions[];
};

layout(std430, binding=3) buffer VelBuf {
    vec4 velocities[];
};

layout(std430, binding=4) buffer NewVelBuf {
    vec4 newVelocities[];
};

layout(std430, binding=7) buffer ColorBuf {
    vec4 colors[];
};

layout(std430, binding=10) buffer NeighborDataBuf {
    NeighborType neighbors[];
};

// ***** COMPUTE SHADER SUBROUTINES *****
// Calculates the magnitude of the vector squared
float squareMagnitude(vec3 vec){
    return vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
}

// Poly Smoothing Kernel
// SOURCE: Mathias Muller et al (2003)
float WPoly(vec3 dist){
    float rLen = length(dist);
    if (rLen > fluid.supportRadius || rLen <= 0.0000001) {
        return 0;
    }

    float h2minusr2 = fluid.supportRadius * fluid.supportRadius - rLen * rLen;
    return fluid.kpoly * h2minusr2 * h2minusr2 * h2minusr2;
}

// Gradient Spiky Smoothing Kernel
// SOURCE: Mathias Muller et al (2003)
vec3 gradWSpiky(vec3 dist){
    float rLen = length(dist);

    if (rLen > fluid.supportRadius || rLen <= 0.0000001){
        return vec3(0.0);
    }

    float hminusr = fluid.supportRadius - rLen;
    return fluid.kspiky * (hminusr * hminusr) * normalize(dist);
}

vec3 xsph(uint vIndex){
    vec3 pos = vec3(newPositions[vIndex]);
    vec3 vel = vec3(velocities[vIndex]);
    NeighborType neighborData = neighbors[vIndex];

    vec3 velNeighbor = vec3(0.0);

    for (uint i = 0; i < neighborData.count; i++){
        velNeighbor += (vec3(velocities[neighborData.neighboring[i]]) - vel) * WPoly(pos-vec3(newPositions[neighborData.neighboring[i]]));
    }

    return vel + fluid.kxsph * velNeighbor;
}

void main() {
    uint vIndex = gl_GlobalInvocationID.x;

    // Apply XSPH Viscosity
    vec3 newVel = xsph(vIndex);
    newVelocities[vIndex].xyz = newVel;

    // Set color to vel
    colors[vIndex].rgb = 0.5*(normalize(newVel) + vec3(1.0));//vec3(0.0, 0.32, 0.62);// + 0.1*(normalize(newVel) + vec3(1.0));
}
