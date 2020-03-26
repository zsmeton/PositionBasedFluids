#version 430 core

// ***** VERTEX SHADER INPUT *****
layout(location=0) in uint vIndex;
layout(location=1) in vec4 vPos;
layout(location=2) in vec4 vVel;


// ***** VERTEX SHADER OUTPUT *****

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
    uint maxNeighbors;
    uint mapSize;
    uint timestamp;
    float supportRadius;
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
    uint neighboring[1000];
};

// ***** VERTEX SHADER BUFFERS *****
/*
    index = 0;
    position = 1;
    positionStar = 2;
    velocity = 3;
    lambda = 4;
    deltaP = 5;
    color = 6;
    hashMap = 7;
    linkedList = 8;
    neighbors = 9;
    counter = 0;
*/
layout(std430, binding=2) buffer UpdatedPos {
    vec4 updatedPositions[];
};

layout(std430, binding=6) buffer Color {
    vec4 colors[];
};

layout(std430, binding=7) buffer Hash {
    HashType hashMap[];
};

layout(std430, binding=8) buffer LinkedList {
    NodeType nodes[];
};

layout(std430, binding=9) buffer NeighborData {
    NeighborType neighbors[];
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

// Calculate the neighbors in this cell
uint neighborFindCell(uint neighborIdx, int xOffset, int yOffset, int zOffset){
    // Get position
    vec3 pos = updatedPositions[vIndex].xyz;

    // Calculate hash
    int hashIdx = spacialHash(pos.x+xOffset*fluid.supportRadius, pos.y+yOffset*fluid.supportRadius, pos.z+zOffset*fluid.supportRadius);
    HashType start = hashMap[hashIdx];

    // Skip parts of hashmap with old data
    if(start.headNodeIndex == 0xffffffff){
        return neighborIdx;
    }

    // Count neighbors in that hash cell
    // Iterate over linked list
    int errorCounter = 0;
    NodeType n = nodes[start.headNodeIndex];
    while(errorCounter < fluid.maxParticles && neighborIdx < fluid.maxNeighbors){
        // Skip ourselves
        if (n.particleIndex != vIndex){
            // If distance is < support radius increment neighbor count
            if (distance(pos, updatedPositions[n.particleIndex].xyz) <= fluid.supportRadius){
                neighbors[vIndex].neighboring[neighborIdx] = n.particleIndex;
                neighborIdx ++;
            }
        }
        // Exit on null
        if(n.nextNodeIndex == 0xffffffff){
            return neighborIdx;
        }else{
            // Or go to next node
            n = nodes[n.nextNodeIndex];
        }
        errorCounter ++;
    }
    // Color debuging
    if(errorCounter > fluid.maxParticles){
        colors[vIndex].r = float((errorCounter-fluid.maxParticles))/fluid.maxParticles;
    }
    return neighborIdx;
}

// Find all of the neighbors
uint findNeighbors(){
    // Calculate # of neighbors
    uint numNeighbors = 0;

    colors[vIndex].r = 0.0;
    numNeighbors = neighborFindCell(numNeighbors, -1, -1, -1);
    numNeighbors = neighborFindCell(numNeighbors, -1, -1,  0);
    numNeighbors = neighborFindCell(numNeighbors, -1, -1, +1);
    numNeighbors = neighborFindCell(numNeighbors, -1,  0, -1);
    numNeighbors = neighborFindCell(numNeighbors, -1,  0,  0);
    numNeighbors = neighborFindCell(numNeighbors, -1,  0, +1);
    numNeighbors = neighborFindCell(numNeighbors, -1, +1, -1);
    numNeighbors = neighborFindCell(numNeighbors, -1, +1,  0);
    numNeighbors = neighborFindCell(numNeighbors, -1, +1, +1);
    numNeighbors = neighborFindCell(numNeighbors,  0, -1, -1);
    numNeighbors = neighborFindCell(numNeighbors,  0, -1,  0);
    numNeighbors = neighborFindCell(numNeighbors,  0, -1, +1);
    numNeighbors = neighborFindCell(numNeighbors,  0,  0, -1);
    numNeighbors = neighborFindCell(numNeighbors,  0,  0,  0);
    numNeighbors = neighborFindCell(numNeighbors,  0,  0, +1);
    numNeighbors = neighborFindCell(numNeighbors,  0, +1, -1);
    numNeighbors = neighborFindCell(numNeighbors,  0, +1,  0);
    numNeighbors = neighborFindCell(numNeighbors,  0, +1, +1);
    numNeighbors = neighborFindCell(numNeighbors, +1, -1, -1);
    numNeighbors = neighborFindCell(numNeighbors, +1, -1,  0);
    numNeighbors = neighborFindCell(numNeighbors, +1, -1, +1);
    numNeighbors = neighborFindCell(numNeighbors, +1,  0, -1);
    numNeighbors = neighborFindCell(numNeighbors, +1,  0,  0);
    numNeighbors = neighborFindCell(numNeighbors, +1,  0, +1);
    numNeighbors = neighborFindCell(numNeighbors, +1, +1, -1);
    numNeighbors = neighborFindCell(numNeighbors, +1, +1,  0);
    numNeighbors = neighborFindCell(numNeighbors, +1, +1, +1);

    return numNeighbors;
}


void main() {
    // Spacial Hash
    memoryBarrier();
        // Calculate hash
        int hashIdx = spacialHash(vPos.x, vPos.y, vPos.z);
        // Get the index of the next empty slot in the buffer
        uint nodeIdx = atomicCounterIncrement(nextNodeCounter);

        // is there space left in the buffer
        if(nodeIdx < fluid.maxParticles) {
            // Update the head pointer in the hash map
            uint previousHeadIdx = atomicExchange(hashMap[hashIdx].headNodeIndex, nodeIdx);

            // Set linked list data appropriately
            nodes[nodeIdx].nextNodeIndex = previousHeadIdx;
            nodes[nodeIdx].particleIndex = vIndex;

        }else{
            // HERE LIES DRAGONS, VERY FIERCE ONES
            // THIS SHOULD NEVER HAPPEN AND IF IT DOES THINGS WILL BREAK, LOTS OF THINGS!!!!!!
        }


    // Find Neighbors
    memoryBarrier();
        neighbors[vIndex].count = findNeighbors();
        colors[vIndex].b = float(neighbors[vIndex].count)/fluid.maxNeighbors;
        colors[vIndex].g = float(neighbors[vIndex].count)/fluid.maxNeighbors;
}
