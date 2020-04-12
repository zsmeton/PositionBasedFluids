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

layout(std430, binding=5) buffer LambdaBuf {
    float lambdas[];
};

layout(std430, binding=10) buffer NeighborDataBuf {
    NeighborType neighbors[];
};

// ***** COMPUTE SHADER SUBROUTINES *****
// ***** COMPUTE SHADER HELPER FUNCTIONS *****

// Calculates the magnitude of the vector squared
float squareMagnitude(vec3 vec){
    return vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
}

// Poly Smoothing Kernel
// SOURCE: Mathias Muller et al (2003)
float WPoly(vec3 dist){
    float rLen2 = squareMagnitude(dist);
    if (rLen2 > fluid.supportRadius * fluid.supportRadius || rLen2 <= 0.0000001) {
        return 0;
    }

    float h2minusr2 = fluid.supportRadius * fluid.supportRadius - rLen2;
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

// Standard SPH Density Estimator
// SOURCE: Position Based Fluids Macklin
float densityEstimation(uint vIndex, NeighborType neighborData){
    vec3 pos = vec3(newPositions[vIndex]);

    float density = 0.0;
    for (uint i = 0; i < neighborData.count; i++){
        density += WPoly(pos - vec3(newPositions[neighborData.neighboring[i]]));
    }

    return density;
}

// SPH Density Constraint
// SOURCE: Position Based Fluids Macklin
float constraint(uint vIndex, NeighborType neighborData){
    return (densityEstimation(vIndex, neighborData) / fluid.restDensity) - 1.0;
}

// Gradient of SPH Density Constraint
// SOURCE: Position Based Fluids Macklin
float sumGradientConstraint(uint vIndex, NeighborType neighborData){
    vec3 pos = vec3(newPositions[vIndex]);

    vec3 gradientI = vec3(0.0f);
    float sumGradients = 0.0f;
    for (uint i = 0; i < neighborData.count; i++) {
        //Calculate gradient with respect to j
        vec3 gradientJ = gradWSpiky(pos - vec3(newPositions[neighborData.neighboring[i]])) / fluid.restDensity;

        //Add magnitude squared to sum
        sumGradients += pow(length(gradientJ), 2);
        gradientI += gradientJ;
    }

    //Add the particle i gradient magnitude squared to sum
    sumGradients += pow(length(gradientI), 2);

    return sumGradients;
}

float lambda(uint vIndex){
    NeighborType neighborData = neighbors[vIndex];
    float densityConstraint = constraint(vIndex, neighborData);
    float sumGrad = sumGradientConstraint(vIndex, neighborData);

    return -densityConstraint/(sumGrad + fluid.epsilon);
}

void main() {
    uint vIndex = gl_GlobalInvocationID.x;

    // Calculate lambda
    lambdas[vIndex] = lambda(vIndex);
}
