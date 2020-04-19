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

struct SDFCell {
    float distance;
    vec4 normal;
};

struct BoundingBox {
    vec4 frontLeftBottom;
    vec4 backRightTop;
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

layout(std430, binding=6) buffer DeltaPBuf {
    vec4 deltaPs[];
};

layout(std430, binding=7) buffer ColorBuf {
    vec4 colors[];
};

layout(std430, binding=10) buffer NeighborDataBuf {
    NeighborType neighbors[];
};

layout(std430, binding=11) buffer SignedDistanceField {
    mat4 transformMtx;
    uint xDim, yDim, zDim;
    SDFCell cells [];
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

float sCorr(vec3 pi, vec3 pj){
    return -fluid.scorr * pow(WPoly(pi-pj)/fluid.dcorr, fluid.pcorr);
}

vec3 deltaP(uint vIndex){
    vec3 pos = vec3(newPositions[vIndex]);
    float lambdaI = lambdas[vIndex];
    vec3 deltaPos = vec3(0.0);

    NeighborType neighborData = neighbors[vIndex];
    for (uint i = 0; i < neighborData.count; i++){
        uint j = neighborData.neighboring[i];
        vec3 posj = vec3(newPositions[j]);
        float s = sCorr(pos, posj);
        deltaPos += (lambdaI + lambdas[j] + s) * gradWSpiky(pos - posj);
    }

    return deltaPos / fluid.restDensity;
}

vec3 collideSDF(vec3 pos, vec3 deltaPos){
    vec3 newPos = pos + deltaPos;

    // Transform the position
    vec3 tranPos = vec3(transformMtx*vec4(newPos, 1.0));
    // Turn transformed position into indices
    tranPos = vec3(round(tranPos.x), round(tranPos.y), round(tranPos.z));
    // Check if in the bounding box
    if (tranPos.x < 0 || tranPos.x >= xDim){
        return deltaPos;
    }
    if (tranPos.y < 0 || tranPos.y >= yDim){
        return deltaPos;
    }
    if (tranPos.z < 0 || tranPos.z >= zDim){
        return deltaPos;
    }

    // Get index from dimension indices
    int index = int(tranPos.x + yDim * (tranPos.y + zDim * tranPos.z));
    if (index < 0 || index > xDim * yDim * zDim){
        return deltaPos;
    }
    // Get distance from sdf cells
    if (cells[index].distance <= 0.05){
        float delta = (0.05 - cells[index].distance) + fluid.collisionEpsilon;
        return deltaPos + delta * vec3(cells[index].normal);
    }
    return deltaPos;
}

float strangeFunction(float x){
    float denom = 1 + pow(10, -5.0*sin(1.5*x));
    return 2.0*(1/denom - 0.5);
}

vec3 confineToBox(vec3 pos, vec3 deltaPos){
    vec3 newPos = pos + deltaPos;

    // Check floor
    float wallY = -1.0;
    if (fluid.time > 5.0){
        wallY = -5.0;
    }
    if (newPos.y < wallY){
        deltaPos.y = wallY - newPos.y + fluid.collisionEpsilon;
    } else if (newPos.y > 20.0){
        deltaPos.y = 20.0 - newPos.y - fluid.collisionEpsilon;
    }
    // Check left wall
    float wallW = 2.5;
    if (fluid.time > 5.2){
        wallW = 4.0;
    }
    if (newPos.x < -wallW){
        deltaPos.x = -wallW - newPos.x + fluid.collisionEpsilon;
    } else if (newPos.x > wallW){
        // Check right wall
        deltaPos.x = wallW - newPos.x - fluid.collisionEpsilon;
    }
    // Check front wall
    if (newPos.z < -wallW){
        deltaPos.z = -wallW - newPos.z + fluid.collisionEpsilon;
    } else if (newPos.z > wallW){
        deltaPos.z = wallW - newPos.z - fluid.collisionEpsilon;
    }

    return deltaPos;
}

void main() {
    uint vIndex = gl_GlobalInvocationID.x;

    // Calculate Delta P
    vec3 dp = deltaP(vIndex);

    // Collision detection and response
    dp = confineToBox(newPositions[vIndex].xyz, dp);
    //dp = collideSDF(newPositions[vIndex].xyz, dp);

    // Set delta p
    deltaPs[vIndex].xyz = dp;

    // Set color to vel
    colors[vIndex].rgb = 0.5*(normalize(dp) + vec3(1.0));
}
