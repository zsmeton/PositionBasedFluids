#version 430 core

#define M_PI 3.1415926535897932384626433832795

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
    float supportRadius;
    float dt;
    uint solverIters;
    float restDensity;
    float epsilon;
    float collisionEpsilon;
    float kpoly;
    float kspiky;
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

layout(std430, binding=4) buffer LambdaBuf {
    float lambdas[];
};

layout(std430, binding=6) buffer ColorBuf {
    vec4 colors[];
};

layout(std430, binding=7) buffer HashBuf {
    HashType hashMap[];
};

layout(std430, binding=8) buffer LinkedListBuf {
    NodeType nodes[];
};

layout(std430, binding=9) buffer NeighborDataBuf {
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
uint neighborFindCell(uint neighborCount, int hashIdx){
    // Get position
    vec3 pos = vec3(newPositions[vIndex]);

    // get head
    HashType start = hashMap[hashIdx];

    // Skip parts of hashmap with old data
    if(start.headNodeIndex == 0xffffffff){
        return neighborCount;
    }

    // Count neighbors in that hash cell
    // Iterate over linked list
    int errorCounter = 0;
    NodeType n = nodes[start.headNodeIndex];
    while( errorCounter < fluid.maxParticles && neighborCount < fluid.maxNeighbors ){
        // Skip ourselves
        if (n.particleIndex != vIndex){
            // If distance is < support radius increment neighbor count
            if (distance(pos, vec3(newPositions[n.particleIndex])) <= fluid.supportRadius){
                neighbors[vIndex].neighboring[neighborCount] = n.particleIndex;
                neighborCount ++;
            }
        }
        // Exit on null
        if(n.nextNodeIndex == 0xffffffff){
            return neighborCount;
        }else{
            // Or go to next node
            n = nodes[n.nextNodeIndex];
        }
        errorCounter ++;
    }
    return neighborCount;
}

int getNeighborCellHash(vec3 pos, int xOffset, int yOffset, int zOffset){
    return int((int((floor(pos.x/fluid.supportRadius) + xOffset) * P1) ^
    int((floor(pos.y/fluid.supportRadius) + yOffset) * P2) ^
    int((floor(pos.z/fluid.supportRadius) + zOffset) * P3)) % fluid.mapSize);
}

int[27] getNeighborCellHashes(){
    // Get position
    vec3 pos = vec3(newPositions[vIndex]);

    int neighborHash[27];
    // initialize to -1
    for(int i = 0; i < 27; i++){
        neighborHash[i] = -1;
    }

    // Hash to neighbors, check if unique
    int index = 0;
    for(int i = -1; i < 2; i++){
        for(int j = -1; j < 2; j++){
            for(int k = -1; k < 2; k++){
                // Hash
                int hash = getNeighborCellHash(pos,i,j,k);
                // Uniqueness check
                bool new = true;
                for(int l = 0; l < 27 && neighborHash[l] != -1; l++){
                    new = new && (hash != neighborHash[l]);
                }
                if(new){
                    neighborHash[index] = hash;
                    index ++;
                }
            }
        }
    }

    return neighborHash;
}

// Find all of the neighbors
uint findNeighbors(){
    // Calculate # of neighbors
    uint numNeighbors = 0;

    // Get neighbor hashes
    int hashes[27] = getNeighborCellHashes();

    for(int i = 0; i < 27 && hashes[i] > -1; i++){
        numNeighbors = neighborFindCell(numNeighbors, hashes[i]);
    }
    return numNeighbors;
}


float squareMagnitude(vec3 vec){
    return vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
}

// Poly Smoothing Kernel
// SOURCE: Mathias Muller et al (2003)
float WPoly(vec3 dist){
    float rLen = length(dist);
    if (rLen > fluid.supportRadius || rLen <= 0.0001) {
        return 0;
    }

    float h2minusr2 = fluid.supportRadius * fluid.supportRadius - squareMagnitude(dist);
    return fluid.kpoly * h2minusr2 * h2minusr2 * h2minusr2;
}

// Gradient Spiky Smoothing Kernel
// SOURCE: Mathias Muller et al (2003)
vec3 gradWSpiky(vec3 dist){
    float rLen = length(dist);

    if(rLen > fluid.supportRadius || rLen <= 0.0001){
        return vec3(0.0);
    }

    float scaling = (fluid.kspiky / rLen);
    float hminusr = fluid.supportRadius - rLen;
    return -dist * scaling * (hminusr * hminusr);
}

// Standard SPH Density Estimator
// SOURCE: Position Based Fluids Macklin
float densityEstimation(NeighborType neighborData){
    vec3 pos = vec3(newPositions[vIndex]);

    float density = 0.0;
    for(int i = 0; i < neighborData.count; i++){
        density += WPoly( pos - vec3(newPositions[neighborData.neighboring[i]]) );
    }

    return density;
}

// SPH Density Constraint
// SOURCE: Position Based Fluids Macklin
float constraint(NeighborType neighborData){
    return (densityEstimation(neighborData) / fluid.restDensity) - 1.0;
}

// Gradient of SPH Density Constraint
// SOURCE: Position Based Fluids Macklin
float sumGradientConstraint(NeighborType neighborData){
    vec3 pos = vec3(newPositions[vIndex]);

    vec3 gradientI = vec3(0.0f);
    float sumGradients = 0.0f;
    for (int i = 0; i < neighborData.count; i++) {
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

float lambda(){
    NeighborType neighborData = neighbors[vIndex];
    float densityConstraint = constraint(neighborData);
    float sumGrad = sumGradientConstraint(neighborData);

    return -densityConstraint/(sumGrad + fluid.epsilon);
}

vec3 deltaP(){
    vec3 pos = vec3(newPositions[vIndex]);
    float lambdaI = lambdas[vIndex];
    vec3 deltaPos = vec3(0.0);

    NeighborType neighborData = neighbors[vIndex];
    for(uint i = 0; i < neighborData.count; i++){
        uint j = neighborData.neighboring[i];
        deltaPos += (lambdaI + lambdas[j]) * gradWSpiky(pos - vec3(newPositions[j]));
    }

    return deltaPos / fluid.restDensity;
}

vec3 confineToBox(vec3 pos, vec3 deltaPos){
    vec3 newPos = pos + deltaPos;

    colors[vIndex].r = 0.0;

    // Check floor
    if(newPos.y < -5.0){
        deltaPos.y = -5.0 - newPos.y + fluid.collisionEpsilon;
        colors[vIndex].r = 1.0;
    } else if(newPos.y > 20.0){
        deltaPos.y = 20.0 - newPos.y - fluid.collisionEpsilon;
        colors[vIndex].r = 1.0;
    }
    // Check left wall
    if(newPos.x < -10.0){
        deltaPos.x = -10.0 - newPos.x + fluid.collisionEpsilon;
        colors[vIndex].r = 1.0;
    }else if(newPos.x > 10.0){
        // Check right wall
        deltaPos.x = 10.0 - newPos.x - fluid.collisionEpsilon;
        colors[vIndex].r = 1.0;
    }
    // Check front wall
    if(newPos.z < -5.0){
        deltaPos.z = -5.0 - newPos.z + fluid.collisionEpsilon;
        colors[vIndex].r = 1.0;
    }else if(newPos.z > 5.0){
        deltaPos.z = 5.0 - newPos.z - fluid.collisionEpsilon;
        colors[vIndex].r = 1.0;
    }

    return deltaPos;
}

void main() {
    // Apply Forces, Predict Positions
    vec3 updatedVelocity = vec3(vVel) + fluid.dt *  vec3(0.0, -9.8, 0.0);
    vec3 _pos = vec3(vPos) + fluid.dt * updatedVelocity ; // Set additional variable for memory access optimization
    newPositions[vIndex].xyz = _pos;

    // Spacial Hash
        // Calculate hash
        int hashIdx = spacialHash(_pos.x, _pos.y, _pos.z);
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
    memoryBarrier();

    // Find Neighbors
        neighbors[vIndex].count = findNeighbors();

    // Constraint solve
    for(uint i = 0; i < fluid.solverIters; i ++){
        // Calculate lambda
            lambdas[vIndex] = lambda();
        memoryBarrier();

        // Update Position
            // Calculate Delta P
            vec3 iterDeltaP = deltaP();
            colors[vIndex].rgb = iterDeltaP;

            // Collision detection and response
            iterDeltaP = confineToBox(newPositions[vIndex].xyz, iterDeltaP);
        memoryBarrier();
            // Actually update position
            newPositions[vIndex].xyz = vec3(newPositions[vIndex]) + iterDeltaP;
        memoryBarrier();
    }

    // Update Velocity
    vec3 vel = (newPositions[vIndex].xyz - vec3(vPos))/fluid.dt;
    velocities[vIndex].xyz = vel;
    colors[vIndex].rgb = vel;
    // Apply Vorticity Confinement and XSPH Viscosity
    // Update Position
    positions[vIndex].xyz = newPositions[vIndex].xyz;
}
