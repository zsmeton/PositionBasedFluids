#version 430 core
#define DISPLAY 1

// ***** FRAGMENT SHADER INPUT *****
layout(location=0) in vec3 pos;
layout(location=1) flat in uint index;

// ***** FRAGMENT SHADER OUTPUT *****
layout(location=0) out vec4 color;

// ***** FRAGMENT SHADER STRUCTS *****
struct HashType {
    uint headNodeIndex;
};

struct NodeType {
    uint nextNodeIndex;
    uint particleIndex;
};

// ***** FRAGMENT SHADER BUFFERS *****
layout(std430, binding=4) buffer Hash {
    uint hashMap[];
};

layout(std430, binding=5) buffer LinkedList {
    NodeType nodes[];
};


// ***** FRAGMENT SHADER UNIFORMS *****
layout(binding=0, offset=0) uniform atomic_uint nextNodeCounter;

layout(shared, binding = 4) uniform FluidDynamics {
    uint maxParticles;
    uint mapSize;
    uint timestamp;
    float supportRadius;
    float time;
} fluid;

// ***** FRAGMENT SHADER SUBROUTINES *****

// ***** FRAGMENT SHADER HELPER FUNCTIONS *****
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
    #if !DISPLAY
        vec4 color;
    #endif

    // Set color
    color = vec4(1.0, 1.0, 1.0, 0.0);

    // Calculate hash
    int hashIdx = spacialHash(pos.x, pos.y, pos.z);
    // Get the index of the next empty slot in the buffer
    uint nodeIdx = atomicCounterIncrement(nextNodeCounter);

    // is there space left in the buffer
    if(nodeIdx < fluid.maxParticles) {
        // Update the head pointer in the hash map
        uint previousHeadIdx = atomicExchange(hashMap[hashIdx], nodeIdx);

        // Set linked list data appropriately
        //nodes[nodeIdx].nextNodeIndex = previousHeadIdx;
        atomicExchange(nodes[nodeIdx].nextNodeIndex, previousHeadIdx);
        //nodes[nodeIdx].particleIndex = index;
        atomicExchange(nodes[nodeIdx].particleIndex, index);

        if(previousHeadIdx == 0xffffffff){
            //color = vec4(0.0,0.0,1.0,1.0);
        }
    }else{
        // HERE LIES DRAGONS, VERY FIERCE ONES
        // THIS SHOULD NEVER HAPPEN AND IF IT DOES THINGS WILL BREAK, LOTS OF THINGS!!!!!!
        color = vec4(1.0,0.0,0.0,1.0);
    }
}
