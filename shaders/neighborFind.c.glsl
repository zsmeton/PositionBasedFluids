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

layout(std430, binding=7) buffer ColorBuf {
    vec4 colors[];
};

layout(std430, binding=8) buffer HashBuf {
    HashType hashMap[];
};

layout(std430, binding=9) buffer LinkedListBuf {
    NodeType nodes[];
};

layout(std430, binding=10) buffer NeighborDataBuf {
    NeighborType neighbors[];
};

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

// Calculates the magnitude of the vector squared
float squareMagnitude(vec3 vec){
    return vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
}

// Calculate the neighbors in this cell
// vIndex: the index of the particle
// neighborCount: The number of current neighbors
uint neighborFindCell(uint vIndex, uint neighborCount, int hashIdx){
    // Get position
    vec3 pos = vec3(newPositions[vIndex]);

    // get head
    HashType start = hashMap[hashIdx];

    // Skip parts of hashmap with old data
    if (start.headNodeIndex == 0xffffffff){
        return neighborCount;
    }

    // Count neighbors in that hash cell
    // Iterate over linked list
    NodeType n = nodes[start.headNodeIndex];
    while (neighborCount < fluid.maxNeighbors){
        // Skip ourselves
        if (n.particleIndex != vIndex){
            // If distance is < support radius increment neighbor count
            if (squareMagnitude(pos - vec3(newPositions[n.particleIndex])) <= fluid.supportRadius * fluid.supportRadius){
                neighbors[vIndex].neighboring[neighborCount] = n.particleIndex;
                neighborCount ++;
            }
        }
        // Exit on null
        if (n.nextNodeIndex == 0xffffffff){
            return neighborCount;
        } else {
            // Or go to next node
            n = nodes[n.nextNodeIndex];
        }
    }
    return neighborCount;
}

// Get hashes of neighboring cell based on position and x,y,z offsets
int getNeighborCellHash(vec3 pos, int xOffset, int yOffset, int zOffset){
    return int((int((floor(pos.x/fluid.supportRadius) + xOffset) * P1) ^
    int((floor(pos.y/fluid.supportRadius) + yOffset) * P2) ^
    int((floor(pos.z/fluid.supportRadius) + zOffset) * P3)) % fluid.mapSize);
}

// Returns a list of the neighboring hash cells
int[27] getNeighborCellHashes(uint vIndex){
    // Get position
    vec3 pos = vec3(newPositions[vIndex]);

    int neighborHash[27];
    // initialize to -1
    for (int i = 0; i < 27; i++){
        neighborHash[i] = -1;
    }

    // Hash to neighbors, check if unique
    int index = 0;
    for (int i = -1; i < 2; i++){
        for (int j = -1; j < 2; j++){
            for (int k = -1; k < 2; k++){
                // Hash
                int hash = getNeighborCellHash(pos, i, j, k);
                // Uniqueness check
                bool new = true;
                for (int l = 0; l < 27 && neighborHash[l] != -1; l++){
                    new = new && (hash != neighborHash[l]);
                }
                if (new){
                    neighborHash[index] = hash;
                    index ++;
                }
            }
        }
    }

    return neighborHash;
}

// Find all of the neighbors
uint findNeighbors(uint vIndex){
    // Calculate # of neighbors
    uint numNeighbors = 0;

    // Get neighbor hashes
    int hashes[27] = getNeighborCellHashes(vIndex);

    for (int i = 0; i < 27 && hashes[i] > -1; i++){
        numNeighbors = neighborFindCell(vIndex, numNeighbors, hashes[i]);
    }
    return numNeighbors;
}

void main() {
    uint vIndex = gl_GlobalInvocationID.x;

    // Find Neighbors
    neighbors[vIndex].count = findNeighbors(vIndex);

    // Color based on # of Neighbors
    // colors[vIndex] = vec4(vec3(neighbors[vIndex].count/50.0), 1.0);
}
