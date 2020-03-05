#version 430

// ***** COMPUTE SHADER INPUT *****
layout( local_size_x = 1000, local_size_y = 1, local_size_z = 1 ) in;

// ***** COMPUTE SHADER STRUCTS *****
struct HashType {
    uint headNodeIndex;
    //uint timestamp;
};

struct NodeType {
    uint nextNodeIndex;
    uint particleIndex;
};

struct Particle {
    vec4 position;
    vec4 color;
};

// ***** COMPUTE SHADER BUFFERS *****
layout(std430, binding=0) buffer Particles {
    Particle particles [];
};

layout(std430, binding=3) buffer Hash {
    uint hashMap[];
};

layout(std430, binding=4) buffer LinkedList {
    NodeType nodes[];
};

// ***** COMPUTE SHADER UNIFORMS *****
layout(binding=0, offset=0) uniform atomic_uint nextNodeCounter;

layout(shared, binding = 3) uniform FluidDynamics {
    uint maxParticles;
    uint mapSize;
    uint timestamp;
    float supportRadius;
    float time;
} fluid;

// ***** COMPUTE SHADER SUBROUTINES *****

// ***** COMPUTE SHADER HELPER FUNCTIONS *****
const int P1 = 73856093;
const int P2 = 19349663;
const int P3 = 83492791;
// SOURCE: Optimized Spatial Hashing for Collision Detection of Deformable Objects
// Matthias Teschner
uint spacialHash(float x, float y, float z){
    return (int(floor(x/fluid.supportRadius) * P1) ^
                    int(floor(y/fluid.supportRadius) * P2) ^
                    int(floor(x/fluid.supportRadius) * P3)) % fluid.mapSize;
}

void main() {
    // Get particle index
    uint idx = gl_GlobalInvocationID.x;
    // Calculate hash
    uint hashIdx = spacialHash(particles[idx].position.x, particles[idx].position.y, particles[idx].position.z);
    // Get the index of the next empty slot in the buffer
    uint nodeIdx = atomicCounterIncrement(nextNodeCounter);

    // is there space left in the buffer
    if(nodeIdx < fluid.maxParticles) {
        // Update the head pointer in the hash map
        barrier();
        uint previousHeadIndex = hashMap[hashIdx];
        barrier();
        hashMap[hashIdx] = nodeIdx;

        // Set linked list data appropriately
        barrier();
        nodes[nodeIdx].nextNodeIndex = previousHeadIndex;
        barrier();
        nodes[nodeIdx].particleIndex = idx;
    }
}
