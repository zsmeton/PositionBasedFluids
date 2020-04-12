#version 430

// ***** COMPUTE SHADER INPUT *****
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// ***** COMPUTE SHADER STRUCTS *****
struct Triangle{
    vec4 v1, v2, v3;
    vec4 normal;
};

struct SDFCell {
    float distance;
    vec4 normal;
};

struct BoundingBox {
    vec4 frontLeftBottom;
    vec4 backRightTop;
};

// ***** COMPUTE SHADER BUFFERS *****
layout(std430, binding=11) buffer SignedDistanceField {
    BoundingBox boundingBox;
    mat4 transformMtx;
    uint xDim;
    uint yDim;
    uint zDim;
    SDFCell cells [];
};

layout(std430, binding=12) buffer TriangleBuf {
    Triangle triangles[];
};

// ***** COMPUTE SHADER UNIFORMS *****
// ***** COMPUTE SHADER SUBROUTINES *****
// ***** COMPUTE SHADER HELPER FUNCTIONS *****
float distTriangle(Triangle triangle, vec3 point){
    /**
     * Source: Distance Between Point and Triangle in 3D, David Eberly, Geometric Tools, Redmond WA 98052
     */
    vec3 B = vec3(triangle.v1);
    vec3 E0 = vec3(triangle.v2) - B;
    vec3 E1 = vec3(triangle.v3) - B;
    float a = dot(E0, E0);
    float b = dot(E0, E1);
    float c = dot(E1, E1);
    float d = dot(E0, B - point);
    float e = dot(E1, B - point);
    float s = b * e - c * d;
    float t = b * d - a * e;
    float det = a * c - b * b;
    if (s + t <= det) {
        if (s < 0) {
            if (t < 0) {
                // Region 4
                if (d < 0) {
                    t = 0;
                    if (-d >= a) {
                        s = 1;
                    } else {
                        s = -d / a;
                    }
                } else {
                    s = 0;
                    if (e >= 0) {
                        t = 0;
                    } else if (-e >= c) {
                        t = 1;
                    } else {
                        t = -e / c;
                    }
                }
            } else {
                // Region 3
                s = 0;
                if (e >= 0) {
                    t = 0;
                } else if (-e >= c) {
                    t = 1;
                } else {
                    t = -e / c;
                }
            }
        } else if (t < 0) {
            // Region 5
            t = 0;
            if (d >= 0) {
                s = 0;
            } else if (-d >= a) {
                s = 1;
            } else {
                s = -d / a;
            }
        } else {
            // Region 0
            s /= det;
            t /= det;
        }
    } else {
        if (s < 0) {
            // Region 2
            float tmp0 = b + d;
            float tmp1 = c + e;
            if (tmp1 > tmp0) {
                float numer = tmp1 - tmp0;
                float denom = a - 2 * b + c;
                if (numer >= denom) {
                    s = 1;
                } else {
                    s = numer / denom;
                }
                t = 1 - s;
            } else {
                s = 0;
                if (tmp1 <= 0) {
                    t = 1;
                } else if (e >= 0) {
                    t = 0;
                } else {
                    t = -e / c;
                }
            }
        } else if (t < 0) {
            // Region 6
            float tmp0 = b + e;
            float tmp1 = a + d;
            if (tmp1 > tmp0) {
                float numer = tmp1 - tmp0;
                float denom = a - 2 * b + c;
                if (numer >= denom) {
                    t = 1;
                } else {
                    t = numer / denom;
                }
                s = 1 - t;
            } else {
                t = 0;
                if (tmp1 <= 0) {
                    s = 1;
                } else if (e >= 0) {
                    s = 0;
                } else {
                    s = -d / a;
                }
            }
        } else {
            // Region 1
            float numer = (c + e) - (b + d);
            if (numer <= 0) {
                s = 0;
            } else {
                float denom = a - 2 * b + c;
                if (numer >= denom) {
                    s = 1;
                } else {
                    s = numer / denom;
                }
            }
            t = 1 - s;
        }
    }

    // Get vector on triangle closes to point
    vec3 closestPoint = B + s * E0 + t * E1;
    // Get distance
    vec3 pt = point - closestPoint;
    float dist = dot(pt, pt);
    // Get sign
    float sign = sign(dot(vec3(triangle.normal), pt));
    sign = (-0.1 <= sign && sign <= 0.1) ? -1.0 : sign;
    return sign * dist;
}

void main() {
    // Get particle index
    uint xIndex = gl_GlobalInvocationID.x;
    uint yIndex = gl_GlobalInvocationID.y;
    uint zIndex = gl_GlobalInvocationID.z;

    // Initialize to 0
    cells[xIndex + yDim * (yIndex + zDim * zIndex)].distance = 0.0;
    cells[xIndex + yDim * (yIndex + zDim * zIndex)].normal = vec4(0.0);

    // Calculate inverse of the transformation mtx
    mat4 inverseTransformMtx = inverse(transformMtx);

    // Calculate world position
    vec3 pos = vec3(inverseTransformMtx * vec4(xIndex, yIndex, zIndex, 1.0));
    // Find nearest triangle
    Triangle minTri;
    minTri.normal = vec4(0.0);
    float minDist = 99999.9f;
    float minSDist = 99999.0f;
    for (int i = 0; i < 4096; i++) {
        for (int j = 0; j < 4096 && i + 4096*j < 15876; j++){
            float dist = distTriangle(triangles[i+4096*j], pos);
            if (abs(dist) < minDist) {
                minTri = triangles[i];
                minDist = abs(dist);
                minSDist = dist;
            }
        }
    }
    // Set data (with sign)
    if (minSDist < 0.0) {
        cells[xIndex + yDim * (yIndex + zDim * zIndex)].distance = -sqrt(minDist);
    } else {
        cells[xIndex + yDim * (yIndex + zDim * zIndex)].distance = sqrt(minDist);
    }
    cells[xIndex + yDim * (yIndex + zDim * zIndex)].normal = minTri.normal;
}
