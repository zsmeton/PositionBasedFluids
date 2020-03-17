#version 430 core

// ***** VERTEX SHADER INPUT *****
layout(location=0) in vec4 vPos;
layout(location=3) in uint index;

// ***** VERTEX SHADER OUTPUT *****

// ***** VERTEX SHADER STRUCTS *****
struct HashType {
    uint headNodeIndex;
};

struct NodeType {
    uint nextNodeIndex;
    uint particleIndex;
};

// ***** VERTEX SHADER BUFFERS *****
layout(std430, binding=4) buffer Hash {
    uint hashMap[];
};

layout(std430, binding=5) buffer LinkedList {
    NodeType nodes[];
};


// ***** VERTEX SHADER UNIFORMS *****
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
    uint mapSize;
    uint timestamp;
    float supportRadius;
    float time;
} fluid;

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

void main() {
    // Calculate hash
    int hashIdx = spacialHash(vPos.x, vPos.y, vPos.z);
    // Get the index of the next empty slot in the buffer
    uint nodeIdx = atomicCounterIncrement(nextNodeCounter);

    // is there space left in the buffer
    if(nodeIdx < fluid.maxParticles) {
        // Update the head pointer in the hash map
        uint previousHeadIdx = atomicExchange(hashMap[hashIdx], nodeIdx);

        // Set linked list data appropriately
        nodes[nodeIdx].nextNodeIndex = previousHeadIdx;
        nodes[nodeIdx].particleIndex = index;

    }else{
        // HERE LIES DRAGONS, VERY FIERCE ONES
        // THIS SHOULD NEVER HAPPEN AND IF IT DOES THINGS WILL BREAK, LOTS OF THINGS!!!!!!
    }
}
