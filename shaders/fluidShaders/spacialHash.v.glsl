#version 430 core

#define M_PI 3.1415926535897932384626433832795

// ***** VERTEX SHADER INPUT *****
layout(location=0) in uint vIndex;
layout(location=1) in vec4 vPos;
layout(location=2) in vec4 vVel;

// ***** VERTEX SHADER OUTPUT *****

// ***** VERTEX SHADER UNIFORMS *****
layout(binding=0, offset=0) uniform atomic_uint nextNodeCounter;

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

// ***** VERTEX SHADER STRUCTS *****
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
// ***** VERTEX SHADER BUFFERS *****
/*
    index = 0;
    position = 1;
    positionStar = 2;
    velocity = 3;
    lambda = 4;
    color = 6;
    hashMap = 7;
    linkedList = 8;
    neighbors = 9;
    counter = 0;
*/
layout(std430, binding=1) buffer PosBuf {
    vec4 positions[];
};

layout(std430, binding=2) buffer UpdatedPosBuf {
    vec4 newPositions[];
};

layout(std430, binding=3) buffer VelBuf {
    vec4 velocities[];
};

layout(std430, binding=8) buffer HashBuf {
    HashType hashMap[];
};

layout(std430, binding=9) buffer LinkedListBuf {
    NodeType nodes[];
};

layout(std430, binding=11) buffer SignedDistanceField {
    mat4 transformMtx;
    uint xDim, yDim, zDim;
    SDFCell cells [];
};

// ***** VERTEX SHADER SUBROUTINES *****
// ***** VERTEX SHADER HELPER FUNCTIONS *****
const int P1 = 73856093;
const int P2 = 19349663;
const int P3 = 83492791;
// SOURCE: Optimized Spatial Hashing for Collision Detection of Deformable Objects
// Matthias Teschner
int spacialHash(float x, float y, float z){
    return int((int(floor(x/fluid.supportRadius) * P1) ^
    int(floor(y/fluid.supportRadius) * P2) ^
    int(floor(z/fluid.supportRadius) * P3)) % fluid.mapSize);
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
    // Apply Forces, Predict Positions
    vec3 _vel = vec3(vVel) + fluid.dt *  vec3(0.0, -9.8, 0.0);
    vec3 _pos = vec3(vPos) + fluid.dt * _vel;// Set additional variable for memory access optimization
    _pos += confineToBox(_pos, vec3(0.0));
    //_pos += collideSDF(_pos, vec3(0.0));
    newPositions[vIndex].xyz = _pos;
    velocities[vIndex].xyz = (_pos-vec3(vPos)) / fluid.dt;

    // Spacial Hash
    // Calculate hash
    int hashIdx = spacialHash(_pos.x, _pos.y, _pos.z);
    // Get the index of the next empty slot in the buffer
    uint nodeIdx = atomicCounterIncrement(nextNodeCounter);

    // is there space left in the buffer
    if (nodeIdx < fluid.maxParticles) {
        // Update the head pointer in the hash map
        uint previousHeadIdx = atomicExchange(hashMap[hashIdx].headNodeIndex, nodeIdx);

        // Set linked list data appropriately
        nodes[nodeIdx].nextNodeIndex = previousHeadIdx;
        nodes[nodeIdx].particleIndex = vIndex;
    } else {
        // HERE LIES DRAGONS, VERY FIERCE ONES
        // THIS SHOULD NEVER HAPPEN AND IF IT DOES THINGS WILL BREAK, LOTS OF THINGS!!!!!!
    }
}
