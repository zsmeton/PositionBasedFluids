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
};

struct Particle {
    vec4 position;
    vec4 color;
    uint index;
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
layout(shared, binding = 4) uniform FluidDynamics {
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
    return uint((int(floor(x/fluid.supportRadius) * P1) ^
                 int(floor(y/fluid.supportRadius) * P2) ^
                 int(floor(z/fluid.supportRadius) * P3)) % fluid.mapSize);
}

int neighborCount(uint idx, int xOffset, int yOffset, int zOffset){
    // Get position
    vec3 pos = particles[idx].position.xyz;
    // Calculate hash
    uint hashIdx = spacialHash(pos.x+xOffset*fluid.supportRadius, pos.y+yOffset*fluid.supportRadius, pos.z+zOffset*fluid.supportRadius);
    uint start = hashMap[hashIdx];

    // Skip parts of hashmap with old data
    if(start == 0xffffffff){
        return 0;
    }

    // Count neighbors in that hash cell
    // Iterate over linked list
    int count = 0;
    uint i = start;
    NodeType n = nodes[start];
    while (i < fluid.maxParticles){
        // Skip ourselves
        if (i != idx){
            // If distance is < support radius increment neighbor count
            if (distance(pos, particles[i].position.xyz) <= fluid.supportRadius){
                count ++;
            }
        }

        // Exit on null
        if(n.nextNodeIndex == 0xffffffff){
            return count;
        }else{
            // Or go to next node
            i = n.nextNodeIndex;
            n = nodes[n.nextNodeIndex];
        }
    }

    // Color debuging
    if(i == fluid.maxParticles){
        particles[idx].color.r = 1.0;
    }
    return count;
}

void main() {
    // Get particle index
    uint idx = gl_GlobalInvocationID.x;


    // Calculate # of neighbors
    int numNeighbors = 0;
    particles[idx].color.r = 0.0;

    numNeighbors += neighborCount(idx, -1, -1, -1);
    numNeighbors += neighborCount(idx, -1, -1,  0);
    numNeighbors += neighborCount(idx, -1, -1, +1);
    numNeighbors += neighborCount(idx, -1,  0, -1);
    numNeighbors += neighborCount(idx, -1,  0,  0);
    numNeighbors += neighborCount(idx, -1,  0, +1);
    numNeighbors += neighborCount(idx, -1, +1, -1);
    numNeighbors += neighborCount(idx, -1, +1,  0);
    numNeighbors += neighborCount(idx, -1, +1, +1);
    numNeighbors += neighborCount(idx,  0, -1, -1);
    numNeighbors += neighborCount(idx,  0, -1,  0);
    numNeighbors += neighborCount(idx,  0, -1, +1);
    numNeighbors += neighborCount(idx,  0,  0, -1);
    numNeighbors += neighborCount(idx,  0,  0,  0);
    numNeighbors += neighborCount(idx,  0,  0, +1);
    numNeighbors += neighborCount(idx,  0, +1, -1);
    numNeighbors += neighborCount(idx,  0, +1,  0);
    numNeighbors += neighborCount(idx,  0, +1, +1);
    numNeighbors += neighborCount(idx, +1, -1, -1);
    numNeighbors += neighborCount(idx, +1, -1,  0);
    numNeighbors += neighborCount(idx, +1, -1, +1);
    numNeighbors += neighborCount(idx, +1,  0, -1);
    numNeighbors += neighborCount(idx, +1,  0,  0);
    numNeighbors += neighborCount(idx, +1,  0, +1);
    numNeighbors += neighborCount(idx, +1, +1, -1);
    numNeighbors += neighborCount(idx, +1, +1,  0);
    numNeighbors += neighborCount(idx, +1, +1, +1);

    particles[idx].color.b = 10.0*float(numNeighbors)/fluid.maxParticles;
    particles[idx].color.g = 10.0*float(numNeighbors)/fluid.maxParticles;

    particles[idx].position *= .999;

}
