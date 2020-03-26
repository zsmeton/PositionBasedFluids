#version 430

// ***** COMPUTE SHADER INPUT *****
layout( local_size_x = 1536, local_size_y = 1, local_size_z = 1 ) in;

// ***** COMPUTE SHADER STRUCTS *****
struct NodeType {
    uint nextNodeIndex;
    uint particleIndex;
};

// ***** COMPUTE SHADER BUFFERS *****
layout(std430, binding=1) buffer Positions {
    vec4 positions [];
};

layout(std430, binding=6) buffer Colors {
    vec4 colors [];
};

layout(std430, binding=7) buffer Hash {
    uint hashMap[];
};

layout(std430, binding=8) buffer LinkedList {
    NodeType nodes[];
};

// ***** COMPUTE SHADER UNIFORMS *****
layout(shared, binding = 4) uniform FluidDynamics {
    uint maxParticles;
    uint maxNeighbors;
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
int spacialHash(float x, float y, float z){
    return int((int(floor(x/fluid.supportRadius) * P1) ^
                 int(floor(y/fluid.supportRadius) * P2) ^
                 int(floor(z/fluid.supportRadius) * P3)) % fluid.mapSize);
}

int neighborCount(uint idx, int xOffset, int yOffset, int zOffset){
    // Get position
    vec3 pos = positions[idx].xyz;
    // Calculate hash
    int hashIdx = spacialHash(pos.x+xOffset*fluid.supportRadius, pos.y+yOffset*fluid.supportRadius, pos.z+zOffset*fluid.supportRadius);
    uint start = hashMap[hashIdx];

    // Skip parts of hashmap with old data
    if(start == 0xffffffff){
        return 0;
    }

    // Count neighbors in that hash cell
    // Iterate over linked list
    int errorCounter = 0;
    int count = 0;
    NodeType n = nodes[start];
    while(errorCounter < fluid.maxParticles){
        // Skip ourselves
        if (n.particleIndex != idx){
            // If distance is < support radius increment neighbor count
            if (distance(pos, positions[n.particleIndex].xyz) <= fluid.supportRadius){
                count ++;
            }
        }
        // Exit on null
        if(n.nextNodeIndex == 0xffffffff){
            return count;
        }else{
            // Or go to next node
            n = nodes[n.nextNodeIndex];
        }
        errorCounter ++;
    }
    // Color debuging
    if(errorCounter > fluid.maxParticles){
        colors[idx].r = float((errorCounter-fluid.maxParticles))/fluid.maxParticles;
    }
    return count;
}

void main() {
    // Get particle index
    uint idx = gl_GlobalInvocationID.x;

    // Calculate # of neighbors
    int numNeighbors = 0;

    colors[idx].r = 0.0;
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

    colors[idx].b = 10.0*float(numNeighbors)/fluid.maxParticles;
    colors[idx].g = 10.0*float(numNeighbors)/fluid.maxParticles;
    //colors[idx].r = 10.0*float(numNeighbors)/fluid.maxParticles;
}
