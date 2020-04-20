#version 430 core

// ***** FRAGMENT SHADER INPUT *****
layout(location=0) in vec2 texCoord;
layout(location=1) in vec3 pos;

// ***** FRAGMENT SHADER UNIFORMS *****

// ***** VERTEX SHADER STRUCTS *****
struct SDFCell {
    float distance;
    vec4 normal;
};

struct BoundingBox {
    vec4 frontLeftBottom;
    vec4 backRightTop;
};

// ***** VERTEX SHADER BUFFERS *****
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

layout(std430, binding=11) buffer SignedDistanceField {
    mat4 transformMtx;
    uint xDim, yDim, zDim;
    SDFCell cells [];
};

// ***** FRAGMENT SHADER OUTPUT *****
layout(location=0) out vec4 fragColorOut;


// ***** FRAGMENT SHADER HELPER FUNCTIONS *****
// Lookup function
// position: The location in the world to check the sdf for
// def: The default distance (returned when the point is outside of the sdf bounding box)
float distanceLookup(vec3 position, float def){
    // Transform the position
    vec3 tranPos = vec3(transformMtx*vec4(position, 1.0));
    // Turn transformed position into indices
    tranPos = vec3(round(tranPos.x), round(tranPos.y), round(tranPos.z));
    // Check if in the bounding box
    if (tranPos.x < 0 || tranPos.x >= xDim){
        return def;
    }
    if (tranPos.y < 0 || tranPos.y >= yDim){
        return def;
    }
    if (tranPos.z < 0 || tranPos.z >= zDim){
        return def;
    }

    // Get index from dimension indices
    int index = int(tranPos.x + yDim * (tranPos.y + zDim * tranPos.z));
    if (index < 0 || index > xDim * yDim * zDim){
        return def;
    }
    // Get distance from sdf cells
    return cells[index].distance;
}

vec3 normalLookup(vec3 position, vec3 def){
    // Transform the position
    vec3 tranPos = vec3(transformMtx*vec4(position, 1.0));
    // Turn transformed position into indices
    tranPos = vec3(round(tranPos.x), round(tranPos.y), round(tranPos.z));
    // Check if in the bounding box
    if (tranPos.x < 0 || tranPos.x >= xDim){
        return def;
    }
    if (tranPos.y < 0 || tranPos.y >= yDim){
        return def;
    }
    if (tranPos.z < 0 || tranPos.z >= zDim){
        return def;
    }

    // Get index from dimension indices
    int index = int(tranPos.x + yDim * (tranPos.y + zDim * tranPos.z));
    if (index < 0 || index > xDim * yDim * zDim){
        return def;
    }
    // Get distance from sdf cells
    return vec3(cells[index].normal);
}

void main() {
    // get max distance
    float maxDist = 999.99;

    // Get sdf distance
    float sDist = distanceLookup(pos, maxDist);

    // Normalize distance
    float nSDist = sDist;

    // Set color to signed distance
    if (abs(nSDist - maxDist) < 0.01){
        discard;
    } else {

        // get max distance
        vec3 maxNorm = vec3(999.99);

        // Get sdf distance
        vec3 norm = normalLookup(pos, maxNorm);

        // Set color to signed distance
            fragColorOut = vec4(0.5*(normalize(norm) + vec3(1.0)), 1.0);
    }
}
