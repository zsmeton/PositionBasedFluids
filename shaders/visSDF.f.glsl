#version 430 core

// ***** FRAGMENT SHADER INPUT *****
layout(location=0) in vec2 texCoord;

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
layout(std430, binding=10) buffer SignedDistanceField {
    BoundingBox boundingBox;
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
    // Check if in the bounding box
    if (position.x < boundingBox.frontLeftBottom.x || position.x > boundingBox.backRightTop.x){
        return def;
    }
    if (position.y < boundingBox.frontLeftBottom.y || position.y > boundingBox.backRightTop.y){
        return def;
    }
    if (position.z < boundingBox.frontLeftBottom.z || position.z > boundingBox.backRightTop.z){
        return def;
    }

    // Transform the position
    vec3 tranPos = vec3(transformMtx*vec4(position, 1.0));
    // Turn transformed position into indices
    tranPos = vec3(floor(tranPos.x), floor(tranPos.y), floor(tranPos.z));
    // Get index from dimension indices
    int index = int(tranPos.x + yDim * (tranPos.y + zDim * tranPos.z));
    if (index < 0 || index > xDim * yDim * zDim){
        return def;
    }
    // Get distance from sdf cells
    return cells[index].distance;
}

void main() {
    // get max distance
    float maxDist = distance(boundingBox.frontLeftBottom, boundingBox.backRightTop);

    // Get sdf distance
    float sDist = distanceLookup(vec3(texCoord, 0.0), maxDist);

    // Normalize distance
    float nSDist = sDist/maxDist;

    // Set color to signed distance
    if (nSDist == 1.0){
        fragColorOut = vec4(0.0, 0.0, 0.0, 0.0);
    } else if (nSDist < 0.0){
        fragColorOut = vec4(-nSDist, 0.0, 0.0, 1.0);
    } else {
        fragColorOut = vec4(0.0, 0.0, nSDist, 1.0);
    }
}
