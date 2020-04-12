/** @file modelLoader3.hpp
  * @brief Helper functions to draw 3D OpenGL 3.0+ objects
	* @author Dr. Jeffrey Paone
	* @date Last Edit: 15 Nov 2017
	* @version 1.7
	*
	* @copyright MIT License Copyright (c) 2017 Dr. Jeffrey Paone
	*
	*	This class will load and render object files.  Currently supports:
	*		.obj
	*		.off
	*		.stl
	*
	*	@warning NOTE: This header file will only work with OpenGL 3.0+
	*	@warning NOTE: This header file depends upon GLEW
  */

#ifndef __CSCI441_MODELLOADER_3_HPP__
#define __CSCI441_MODELLOADER_3_HPP__

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <SOIL/SOIL.h>

#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <CSCI441/modelMaterial.hpp>
#include <CSCI441/TextureUtils.hpp>
#include "ShaderProgram4.hpp"

////////////////////////////////////////////////////////////////////////////////////

/** @namespace CSCI444
  * @brief CSCI444 Helper Functions for OpenGL
	*/
namespace CSCI444 {

    struct SDFCell {
        float distance;
        glm::vec4 normal;
    };

    struct BoundingBox {
        glm::vec4 frontLeftBottom;
        glm::vec4 backRightTop;
    };

    struct SignedDistanceField {
        BoundingBox boundingBox;
        glm::mat4 transformMtx;
        uint xDim, yDim, zDim;
        SDFCell cells[];
    };

    struct Triangle {
        glm::vec4 v1, v2, v3;
        glm::vec4 normal;
    };

    /** @class ModelLoaderSDF
        * @brief Loads object models from file and renders using VBOs/VAOs
        */
    class ModelLoaderSDF {
    public:
        /** @brief Creates an empty model
            */
        ModelLoaderSDF();

        /** @brief Loads a model from the given file
            * @param const char* filename	- file to load model from
            */
        ModelLoaderSDF(const char *filename);

        /** @brief Frees memory associated with model on both CPU and GPU
            */
        ~ModelLoaderSDF();

        /** @brief Loads a model from the given file
            * @param const char* filename	- file to load model from
            * @param bool INFO						- flag to control if informational messages should be displayed
            * @param bool ERRORS					- flag to control if error messages should be displayed
            * @return true if load succeeded, false otherwise
            */
        bool loadModelFile(const char *filename, bool INFO = true, bool ERRORS = true);

        /** @brief Renders a model
            * @param GLint positionLocation	- attribute location of vertex position
            * @param GLint normalLocation		- attribute location of vertex normal
            * @param GLint texCoordLocation	- attribute location of vertex texture coordinate
            * @param GLint matDiffLocation	- attribute location of material diffuse component
            * @param GLint matSpecLocation	- attribute location of material specular component
            * @param GLint matShinLocation	- attribute location of material shininess component
            * @param GLint matAmbLocation		- attribute location of material ambient component
            * @param GLenum diffuseTexture	- texture number to bind diffuse texture map to
            * @return true if draw succeeded, false otherwise
            */
        bool draw(GLint positionLocation, GLint normalLocation = -1, GLint texCoordLocation = -1,
                  GLint matDiffLocation = -1, GLint matSpecLocation = -1, GLint matShinLocation = -1,
                  GLint matAmbLocation = -1,
                  GLenum diffuseTexture = GL_TEXTURE0);

        /**
         * Calculates the signed distance field for the object, sends data to GPU
         *
         * @param resolution the scaling of the sdf grid to the objects actual size
         * @param initialModelMtx the initial model mtx
         * @return
         */
        bool calculateSignedDistanceFieldCPU(float resolution, float offset, glm::mat4 initialModelMtx);

        bool calculateSignedDistanceField(ShaderProgram *computeShader, float resolution, float offset,
                                          glm::mat4 initialModelMtx);

        bool translateModelMtx(glm::vec3 translation);

        /**
         * Sets the signed distance field's location
         *
         * @param sdfLoc the SDF buffer base location
         */
        void setSDFLocation(GLint sdfLoc);

        /**
         * Sets the triangle location
         *
         * @param triLoc
         */
        void setTriangleLocation(GLint triLoc);

        /** @brief Enable autogeneration of vertex normals
          *
            * If an object model does not contain vertex normal data, then normals will
            * be computed based on the cross product of vertex winding order.
          *
            * @note Must be called prior to loading in a model from file
            */
        static void enableAutoGenerateNormals();

        /** @brief Disable autogeneration of vertex normals
          *
            * If an object model does not contain vertex normal data, then normals will
            * be computed based on the cross product of vertex winding order.
          *
            * @note Must be called prior to loading in a model from file
            * @note No normals are generated by default
            */
        static void disableAutoGenerateNormals();

    private:
        void _init();

        bool _loadMTLFile(const char *mtlFilename, bool INFO, bool ERRORS);

        bool _loadOBJFile(bool INFO, bool ERRORS);

        vector<string> _tokenizeString(string input, string delimiters);

        float _dot2(const glm::vec3 &v);

        float _distTriangle(const Triangle &triangle, const glm::vec3 &point);

        char *_filename;
        CSCI441_INTERNAL::MODEL_TYPE _modelType;

        GLuint _vaod;
        GLuint _vbods[2];
        GLuint _sdfSSBO;
        GLuint _triangleSSBO;

        GLint _sdfLoc;
        GLint _triangleLoc;

        double minX = 999999, maxX = -999999, minY = 999999, maxY = -999999, minZ = 999999, maxZ = -999999;

        GLfloat *_vertices;
        GLfloat *_texCoords;
        GLfloat *_normals;
        unsigned int *_indices;
        unsigned int _uniqueIndex;
        unsigned int _numIndices;

        map<string, CSCI441_INTERNAL::ModelMaterial *> _materials;
        map<string, vector<pair<unsigned int, unsigned int> > > _materialIndexStartStop;

        bool _hasVertexTexCoords;
        bool _hasVertexNormals;

        static bool AUTO_GEN_NORMALS;
    };
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace CSCI441_INTERNAL {
    unsigned char *
    createTransparentTexture(unsigned char *imageData, unsigned char *imageMask, int texWidth, int texHeight,
                             int texChannels, int maskChannels);

    void flipImageY(int texWidth, int texHeight, int textureChannels, unsigned char *textureData);
}

bool CSCI444::ModelLoaderSDF::AUTO_GEN_NORMALS = false;

inline CSCI444::ModelLoaderSDF::ModelLoaderSDF() {
    _init();
}

inline CSCI444::ModelLoaderSDF::ModelLoaderSDF(const char *filename) {
    _init();
    loadModelFile(filename);
}

inline CSCI444::ModelLoaderSDF::~ModelLoaderSDF() {
    if (_vertices) free(_vertices);
    if (_texCoords) free(_texCoords);
    if (_normals) free(_normals);
    if (_indices) free(_indices);

    glDeleteBuffers(1, &_vaod);
    glDeleteBuffers(2, _vbods);
}

inline void CSCI444::ModelLoaderSDF::_init() {
    _hasVertexTexCoords = false;
    _hasVertexNormals = false;

    _vertices = NULL;
    _texCoords = NULL;
    _normals = NULL;
    _indices = NULL;

    // Locations
    _sdfLoc = -1;
    _triangleLoc = -1;


    glGenVertexArrays(1, &_vaod);
    glGenBuffers(2, _vbods);
    glGenBuffers(1, &_sdfSSBO);
    glGenBuffers(1, &_triangleSSBO);
}

inline bool CSCI444::ModelLoaderSDF::loadModelFile(const char *filename, bool INFO, bool ERRORS) {
    bool result = true;
    _filename = (char *) malloc(sizeof(char) * strlen(filename));
    strcpy(_filename, filename);
    if (strstr(_filename, ".obj") != NULL) {
        result = _loadOBJFile(INFO, ERRORS);
        _modelType = CSCI441_INTERNAL::OBJ;
    } else {
        result = false;
        if (ERRORS) fprintf(stderr, "[ERROR]:  Unsupported file format for file: %s\n", _filename);
    }

    return result;
}

inline bool CSCI444::ModelLoaderSDF::draw(GLint positionLocation, GLint normalLocation, GLint texCoordLocation,
                                          GLint matDiffLocation, GLint matSpecLocation, GLint matShinLocation,
                                          GLint matAmbLocation,
                                          GLenum diffuseTexture) {
    bool result = true;

    glBindVertexArray(_vaod);
    glBindBuffer(GL_ARRAY_BUFFER, _vbods[0]);

    glEnableVertexAttribArray(positionLocation);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);

    glEnableVertexAttribArray(normalLocation);
    glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 0, (void *) (sizeof(GLfloat) * _uniqueIndex * 3));

    glEnableVertexAttribArray(texCoordLocation);
    glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, (void *) (sizeof(GLfloat) * _uniqueIndex * 6));

    if (_modelType == CSCI441_INTERNAL::OBJ) {
        for (map<string, vector<pair<unsigned int, unsigned int> > >::iterator materialIter = _materialIndexStartStop.begin();
             materialIter != _materialIndexStartStop.end();
             materialIter++) {

            string materialName = materialIter->first;
            vector<pair<unsigned int, unsigned int> > indexStartStop = materialIter->second;

            CSCI441_INTERNAL::ModelMaterial *material = NULL;
            if (_materials.find(materialName) != _materials.end())
                material = _materials.find(materialName)->second;

            for (vector<pair<unsigned int, unsigned int> >::iterator idxIter = indexStartStop.begin();
                 idxIter != indexStartStop.end();
                 idxIter++) {

                unsigned int start = idxIter->first;
                unsigned int end = idxIter->second;
                unsigned int length = end - start + 1;

                //printf( "rendering material %s (%u, %u) = %u\n", materialName.c_str(), start, end, length );

                if (material != NULL) {
                    glUniform4fv(matAmbLocation, 1, material->ambient);
                    glUniform4fv(matDiffLocation, 1, material->diffuse);
                    glUniform4fv(matSpecLocation, 1, material->specular);
                    glUniform1f(matShinLocation, material->shininess);

                    if (material->map_Kd != -1) {
                        glActiveTexture(diffuseTexture);
                        glBindTexture(GL_TEXTURE_2D, material->map_Kd);
                    }
                }

                glDrawElements(GL_TRIANGLES, length, GL_UNSIGNED_INT, (void *) (sizeof(unsigned int) * start));
            }
        }
    } else {
        glDrawElements(GL_TRIANGLES, _numIndices, GL_UNSIGNED_INT, (void *) 0);
    }

    return result;
}

float CSCI444::ModelLoaderSDF::_dot2(const glm::vec3 &v) {
    return glm::dot(v, v);
}

float CSCI444::ModelLoaderSDF::_distTriangle(const Triangle &triangle, const glm::vec3 &point) {
    /**
     * Source: Distance Between Point and Triangle in 3D, David Eberly, Geometric Tools, Redmond WA 98052
     */
    glm::vec3 B = glm::vec3(triangle.v1);
    glm::vec3 E0 = glm::vec3(triangle.v2) - B;
    glm::vec3 E1 = glm::vec3(triangle.v3) - B;
    float a = glm::dot(E0, E0);
    float b = glm::dot(E0, E1);
    float c = glm::dot(E1, E1);
    float d = glm::dot(E0, B - point);
    float e = glm::dot(E1, B - point);
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
    glm::vec3 closestPoint = B + s * E0 + t * E1;
    // Get distance
    glm::vec3 pt = point - closestPoint;
    float dist = _dot2(pt);
    // Get sign
    float sign = glm::sign(glm::dot(glm::vec3(triangle.normal), pt));
    sign = (-0.1 <= sign && sign <= 0.1) ? 1.0 : sign;
    return sign * dist;
}

inline bool
CSCI444::ModelLoaderSDF::calculateSignedDistanceFieldCPU(float resolution, float offset, glm::mat4 initialModelMtx) {
    // Check that all the locations are set
    if (this->_sdfLoc == -1) {
        fprintf(stderr, "[ERROR]:[SDF]: Signed distance field buffer location is unset\n");
        return false;
    }
    // Check that the resolution is non-zero and non-negative
    if (resolution <= 0.0) {
        fprintf(stderr, "[ERROR]:[SDF]: Resolution must be a positive number\n");
        return false;
    }
    // Check that the offset is non-negative
    if (offset < 0.0) {
        fprintf(stderr, "[ERROR]:[SDF]: Offset must be a non-negative number\n");
        return false;
    }

    // Calculate bounding box
    BoundingBox box;
    box.frontLeftBottom = initialModelMtx * glm::vec4(minX - offset, minY - offset, minZ - offset, 1.0);
    box.backRightTop = initialModelMtx * glm::vec4(maxX + offset, maxY + offset, maxZ + offset, 1.0);

    // Calculate dimensions of grid
    unsigned int dimX = static_cast<unsigned int>(round(
            (box.backRightTop.x - box.frontLeftBottom.x) / resolution));
    unsigned int dimY = static_cast<unsigned int>(round(
            (box.backRightTop.y - box.frontLeftBottom.y) / resolution));
    unsigned int dimZ = static_cast<unsigned int>(round(
            (box.backRightTop.z - box.frontLeftBottom.z) / resolution));

    // Calculate transformation mtx (world -> grid)
    glm::mat4 transformationMtx = glm::mat4(1.0);

    transformationMtx = glm::translate(transformationMtx,
                                       glm::vec3(-box.frontLeftBottom.x, -box.frontLeftBottom.y,
                                                 -box.frontLeftBottom.z));
    transformationMtx =
            glm::scale(glm::mat4(1.0), glm::vec3(1 / resolution, 1 / resolution, 1 / resolution)) * transformationMtx;


    // Calculate the triangle data in world
    vector<Triangle> worldTriangles;
    for (int i = 0; i < _numIndices; i += 3) {
        Triangle triangle;

        // Get verticies
        triangle.v1 = glm::vec4(_vertices[3 * _indices[i + 0] + 0], _vertices[3 * _indices[i + 0] + 1],
                                _vertices[3 * _indices[i + 0] + 2], 1.0);
        triangle.v2 = glm::vec4(_vertices[3 * _indices[i + 1] + 0], _vertices[3 * _indices[i + 1] + 1],
                                _vertices[3 * _indices[i + 1] + 2], 1.0);
        triangle.v3 = glm::vec4(_vertices[3 * _indices[i + 2] + 0], _vertices[3 * _indices[i + 2] + 1],
                                _vertices[3 * _indices[i + 2] + 2], 1.0);

        // Transform to world space
        triangle.v1 = glm::vec4(initialModelMtx * triangle.v1);
        triangle.v2 = glm::vec4(initialModelMtx * triangle.v2);
        triangle.v3 = glm::vec4(initialModelMtx * triangle.v3);

        // Calculate normal
        glm::vec4 ab = triangle.v2 - triangle.v1;
        glm::vec4 ac = triangle.v3 - triangle.v1;
        triangle.normal = glm::vec4(glm::normalize(glm::cross(glm::vec3(ab), glm::vec3(ac))), 0.0);

        // Push back triangle
        worldTriangles.push_back(triangle);
    }

    // Create grid
    SDFCell grid[dimX * dimY * dimZ];

    // Calculate the signed distance field
    glm::mat4 inverseTransformMtx = glm::inverse(transformationMtx);
    uint progressCounter = 0;
    uint total = dimX * dimY * dimZ * worldTriangles.size();

    printf("SDF Dimensions: (%d, %d, %d)\n", dimX, dimY, dimZ);
    printf("Total calcs to do: %u\n", total);

    for (int zIndex = 0; zIndex < dimZ; zIndex++) {
        for (int yIndex = 0; yIndex < dimY; yIndex++) {
            for (int xIndex = 0; xIndex < dimX; xIndex++) {
                // Calculate world position
                glm::vec3 pos = glm::vec3(inverseTransformMtx * glm::vec4(xIndex, yIndex, zIndex, 1.0));
                // Find nearest triangle
                Triangle minTri;
                float minDist = 99999999.0f;
                float minSDist = 99999999.0f;
                for (auto triangle : worldTriangles) {
                    float dist = _distTriangle(triangle, pos);
                    if (abs(dist) < minDist) {
                        minTri = triangle;
                        minDist = abs(dist);
                        minSDist = dist;
                    }
                    if (true) {
                        progressCounter++;
                        if (progressCounter % 5000 == 0) {
                            printf("\33[2K\r");
                            printf("[.obj]: calculating signed distance field... %.2f%%",
                                   progressCounter / (float) total * 100.0);
                            fflush(stdout);
                        }
                    }
                }
                // Set data (with sign)
                if (minSDist < 0.0) {
                    grid[xIndex + dimY * (yIndex + dimZ * zIndex)].distance = -sqrt(minDist);
                } else {
                    grid[xIndex + dimY * (yIndex + dimZ * zIndex)].distance = sqrt(minDist);
                }
                grid[xIndex + dimY * (yIndex + dimZ * zIndex)].normal = minTri.normal;
            }
        }
    }

    if (true) {
        printf("\33[2K\r");
        printf("[.obj]: calculating signed distance field...done!\n", _filename);
        printf("[.obj]: ------------\n");
    }

    // Send info to the GPU
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _sdfSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _sdfLoc, _sdfSSBO);
    uint sdfSize = sizeof(box) + sizeof(glm::mat4) + 3 * sizeof(GLuint) + dimX * dimY * dimZ * sizeof(SDFCell);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sdfSize, NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 8 * sizeof(float), &box);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(float), 16 * sizeof(float), &(transformationMtx)[0][0]);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(float) + 16 * sizeof(float), sizeof(unsigned int), &dimX);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(float) + 16 * sizeof(float) + sizeof(unsigned int),
                    sizeof(unsigned int), &dimY);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(float) + 16 * sizeof(float) + 2 * sizeof(unsigned int),
                    sizeof(unsigned int), &dimZ);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(float) + 16 * sizeof(float) + 3 * sizeof(unsigned int),
                    sizeof(SDFCell) * dimX * dimY * dimZ, &(grid)[0]);


    glBindBuffer(GL_SHADER_STORAGE_BUFFER, -1);
    return true;
}

inline bool
CSCI444::ModelLoaderSDF::calculateSignedDistanceField(ShaderProgram *computeShader, float resolution, float offset,
                                                      glm::mat4 initialModelMtx) {
    // Check that the compute shader is not null
    if (computeShader == nullptr) {
        fprintf(stderr, "[ERROR]:[SDF]: Compute shader is null\n");
        return false;
    }
    // Check that all the locations are set
    if (this->_sdfLoc == -1) {
        fprintf(stderr, "[ERROR]:[SDF]: Signed distance field buffer location is unset\n");
        return false;
    }
    if (this->_triangleLoc == -1) {
        fprintf(stderr, "[ERROR]:[SDF]: Triangle buffer location is unset\n");
        return false;
    }
    // Check that the resolution is non-zero and non-negative
    if (resolution <= 0.0) {
        fprintf(stderr, "[ERROR]:[SDF]: Resolution must be a positive number\n");
        return false;
    }
    // Check that the offset is non-negative
    if (offset < 0.0) {
        fprintf(stderr, "[ERROR]:[SDF]: Offset must be a non-negative number\n");
        return false;
    }

    // Calculate bounding box
    BoundingBox box;
    box.frontLeftBottom = initialModelMtx * glm::vec4(minX - offset, minY - offset, minZ - offset, 1.0);
    box.backRightTop = initialModelMtx * glm::vec4(maxX + offset, maxY + offset, maxZ + offset, 1.0);

    // Calculate dimensions of grid
    unsigned int dimX = static_cast<unsigned int>(round(
            (box.backRightTop.x - box.frontLeftBottom.x) / resolution));
    unsigned int dimY = static_cast<unsigned int>(round(
            (box.backRightTop.y - box.frontLeftBottom.y) / resolution));
    unsigned int dimZ = static_cast<unsigned int>(round(
            (box.backRightTop.z - box.frontLeftBottom.z) / resolution));

    // Calculate transformation mtx (world -> grid)
    glm::mat4 transformationMtx = glm::mat4(1.0);

    transformationMtx = glm::translate(transformationMtx,
                                       glm::vec3(-box.frontLeftBottom.x, -box.frontLeftBottom.y,
                                                 -box.frontLeftBottom.z));
    transformationMtx =
            glm::scale(glm::mat4(1.0), glm::vec3(1 / resolution, 1 / resolution, 1 / resolution)) * transformationMtx;


    // Calculate the triangle data in world
    vector<Triangle> worldTriangles;
    for (int i = 0; i < _numIndices; i += 3) {
        Triangle triangle;

        // Get verticies
        triangle.v1 = glm::vec4(_vertices[3 * _indices[i + 0] + 0], _vertices[3 * _indices[i + 0] + 1],
                                _vertices[3 * _indices[i + 0] + 2], 1.0);
        triangle.v2 = glm::vec4(_vertices[3 * _indices[i + 1] + 0], _vertices[3 * _indices[i + 1] + 1],
                                _vertices[3 * _indices[i + 1] + 2], 1.0);
        triangle.v3 = glm::vec4(_vertices[3 * _indices[i + 2] + 0], _vertices[3 * _indices[i + 2] + 1],
                                _vertices[3 * _indices[i + 2] + 2], 1.0);

        // Transform to world space
        triangle.v1 = glm::vec4(initialModelMtx * triangle.v1);
        triangle.v2 = glm::vec4(initialModelMtx * triangle.v2);
        triangle.v3 = glm::vec4(initialModelMtx * triangle.v3);

        // Calculate normal
        glm::vec4 ab = triangle.v2 - triangle.v1;
        glm::vec4 ac = triangle.v3 - triangle.v1;
        triangle.normal = glm::vec4(glm::normalize(glm::cross(glm::vec3(ab), glm::vec3(ac))), 0.0);

        // Push back triangle
        worldTriangles.push_back(triangle);
    }

    // Calculate the signed distance field
    uint total = dimX * dimY * dimZ * worldTriangles.size();

    printf("SDF Dimensions: (%d, %d, %d)\n", dimX, dimY, dimZ);
    printf("Total calcs to do: %u\n", total);

    // Buffer data to gpus
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->_triangleSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, this->_triangleLoc, this->_triangleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * 4 * sizeof(float) * worldTriangles.size(), NULL, GL_DYNAMIC_DRAW);
    GLint bufMask = GL_MAP_WRITE_BIT;
    auto triangles = (Triangle *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                   4 * 4 * sizeof(float) * worldTriangles.size(), bufMask);
    for (int i = 0; i < worldTriangles.size(); i++) {
        triangles[i].v1 = worldTriangles.at(i).v1;
        triangles[i].v2 = worldTriangles.at(i).v2;
        triangles[i].v3 = worldTriangles.at(i).v3;
        triangles[i].normal = worldTriangles.at(i).normal;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // Setup sdf buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _sdfSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _sdfLoc, _sdfSSBO);
    uint sdfSize = sizeof(box) + sizeof(glm::mat4) + 3 * sizeof(GLuint) + dimX * dimY * dimZ * sizeof(SDFCell);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sdfSize, NULL, GL_DYNAMIC_DRAW);
    auto sdfTemp = (SignedDistanceField *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                            8 * sizeof(float) + 16 * sizeof(float) +
                                                            3 * sizeof(unsigned int), bufMask);
    sdfTemp->boundingBox.frontLeftBottom = box.frontLeftBottom;
    sdfTemp->boundingBox.backRightTop = box.backRightTop;
    sdfTemp->transformMtx = transformationMtx;
    sdfTemp->xDim = dimX;
    sdfTemp->yDim = dimY;
    sdfTemp->zDim = dimZ;
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    computeShader->useProgram();
    glDispatchCompute(dimX, dimY, dimZ);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glDeleteBuffers(1, &_triangleSSBO);

    return true;
}

// Read in a WaveFront *.obj File
inline bool CSCI444::ModelLoaderSDF::_loadOBJFile(bool INFO, bool ERRORS) {
    bool result = true;

    if (INFO) printf("[.obj]: -=-=-=-=-=-=-=- BEGIN %s Info -=-=-=-=-=-=-=- \n", _filename);

    time_t start, end;
    time(&start);

    ifstream in(_filename);
    if (!in.is_open()) {
        if (ERRORS) fprintf(stderr, "[.obj]: [ERROR]: Could not open \"%s\"\n", _filename);
        if (INFO) printf("[.obj]: -=-=-=-=-=-=-=-  END %s Info  -=-=-=-=-=-=-=- \n", _filename);
        return false;
    }

    unsigned int numObjects = 0, numGroups = 0;
    unsigned int numVertices = 0, numTexCoords = 0, numNormals = 0;
    unsigned int numFaces = 0, numTriangles = 0;
    string line;

    map<string, unsigned int> uniqueCounts;
    _uniqueIndex = 0;

    int progressCounter = 0;

    while (getline(in, line)) {
        if (line.length() > 1 && line.at(0) == '\t')
            line = line.substr(1);
        line.erase(line.find_last_not_of(" \n\r\t") + 1);

        vector<string> tokens = _tokenizeString(line, " \t");
        if (tokens.size() < 1) continue;

        //the line should have a single character that lets us know if it's a...
        if (!tokens[0].compare("#") ||
            tokens[0].find_first_of("#") == 0) {                                // comment ignore
        } else if (!tokens[0].compare("o")) {                        // object name ignore
            numObjects++;
        } else if (!tokens[0].compare("g")) {                        // polygon group name ignore
            numGroups++;
        } else if (!tokens[0].compare("mtllib")) {                    // material library
            _loadMTLFile(tokens[1].c_str(), INFO, ERRORS);
        } else if (!tokens[0].compare("v")) {                        //vertex
            numVertices++;

            double x = atof(tokens[1].c_str()),
                    y = atof(tokens[2].c_str()),
                    z = atof(tokens[3].c_str());

            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
            if (z < minZ) minZ = z;
            if (z > maxZ) maxZ = z;
        } else if (!tokens.at(0).compare("vn")) {                    //vertex normal
            numNormals++;
        } else if (!tokens.at(0).compare("vt")) {                    //vertex tex coord
            numTexCoords++;
        } else if (!tokens.at(0).compare("f")) {                     //face!

            //now, faces can be either quads or triangles (or maybe more?)
            //split the string on spaces to get the number of verts+attrs.
            vector<string> faceTokens = _tokenizeString(line, " ");

            for (unsigned int i = 1; i < faceTokens.size(); i++) {
                if (uniqueCounts.find(faceTokens[i]) == uniqueCounts.end()) {
                    uniqueCounts.insert(pair<string, long int>(faceTokens[i], _uniqueIndex));
                    _uniqueIndex++;
                }

                //need to use both the tokens and number of slashes to determine what info is there.
                vector<string> groupTokens = _tokenizeString(faceTokens[i], "/");
                int numSlashes = 0;
                for (unsigned int j = 0; j < faceTokens[i].length(); j++) {
                    if (faceTokens[i][j] == '/') numSlashes++;
                }

                //based on combination of number of tokens and slashes, we can determine what we have.
                if (groupTokens.size() == 2 && numSlashes == 1) {
                    _hasVertexTexCoords = true;
                } else if (groupTokens.size() == 2 && numSlashes == 2) {
                    _hasVertexNormals = true;
                } else if (groupTokens.size() == 3) {
                    _hasVertexTexCoords = true;
                    _hasVertexNormals = true;
                } else if (groupTokens.size() != 1) {
                    if (ERRORS) fprintf(stderr, "[.obj]: [ERROR]: Malformed OBJ file, %s.\n", _filename);
                    return false;
                }
            }

            numTriangles += (faceTokens.size() - 1 - 3 + 1);

            numFaces++;
        } else {

        }

        if (INFO) {
            progressCounter++;
            if (progressCounter % 5000 == 0) {
                printf("\33[2K\r");
                switch (progressCounter) {
                    case 5000:
                        printf("[.obj]: scanning %s...\\", _filename);
                        break;
                    case 10000:
                        printf("[.obj]: scanning %s...|", _filename);
                        break;
                    case 15000:
                        printf("[.obj]: scanning %s.../", _filename);
                        break;
                    case 20000:
                        printf("[.obj]: scanning %s...-", _filename);
                        break;
                }
                fflush(stdout);
            }
            if (progressCounter == 20000)
                progressCounter = 0;
        }
    }
    in.close();

    if (INFO) {
        printf("\33[2K\r");
        printf("[.obj]: scanning %s...done!\n", _filename);
        printf("[.obj]: ------------\n");
        printf("[.obj]: Model Stats:\n");
        printf("[.obj]: Vertices:  \t%u\tNormals:  \t%u\tTex Coords:\t%u\n", numVertices, numNormals, numTexCoords);
        printf("[.obj]: Unique Verts:\t%u\n", _uniqueIndex);
        printf("[.obj]: Faces:     \t%u\tTriangles:\t%u\n", numFaces, numTriangles);
        printf("[.obj]: Objects:   \t%u\tGroups:   \t%u\n", numObjects, numGroups);
        printf("[.obj]: Dimensions:\t(%f, %f, %f)\n", (maxX - minX), (maxY - minY), (maxZ - minZ));
    }

    if (_hasVertexNormals || !AUTO_GEN_NORMALS) {
        if (INFO && !_hasVertexNormals)
            printf("[.obj]: [WARN]: No vertex normals exist on model.  To autogenerate vertex\n\tnormals, call CSCI441::ModelLoaderSDF::enableAutoGenerateNormals()\n\tprior to loading the model file.\n");
        _vertices = (GLfloat *) malloc(sizeof(GLfloat) * _uniqueIndex * 3);
        _texCoords = (GLfloat *) malloc(sizeof(GLfloat) * _uniqueIndex * 2);
        _normals = (GLfloat *) malloc(sizeof(GLfloat) * _uniqueIndex * 3);
        _indices = (unsigned int *) malloc(sizeof(unsigned int) * numTriangles * 3);
    } else {
        if (INFO) printf("[.obj]: No vertex normals exist on model, vertex normals will be autogenerated\n");
        _vertices = (GLfloat *) malloc(sizeof(GLfloat) * numTriangles * 3 * 3);
        _texCoords = (GLfloat *) malloc(sizeof(GLfloat) * numTriangles * 3 * 2);
        _normals = (GLfloat *) malloc(sizeof(GLfloat) * numTriangles * 3 * 3);
        _indices = (unsigned int *) malloc(sizeof(unsigned int) * numTriangles * 3);
    }

    GLfloat *v = (GLfloat *) malloc(sizeof(GLfloat) * numVertices * 3);
    GLfloat *vt = (GLfloat *) malloc(sizeof(GLfloat) * numTexCoords * 2);
    GLfloat *vn = (GLfloat *) malloc(sizeof(GLfloat) * numNormals * 3);

    vector<GLfloat> vertsTemp;
    vector<GLfloat> texCoordsTemp;

    printf("[.obj]: ------------\n");

    uniqueCounts.clear();
    _uniqueIndex = 0;
    _numIndices = 0;

    in.open(_filename);

    unsigned int vSeen = 0, vtSeen = 0, vnSeen = 0, indicesSeen = 0;
    unsigned int uniqueV = 0;

    string currentMaterial = "default";
    _materialIndexStartStop.insert(pair<string, vector<pair<unsigned int, unsigned int> > >(currentMaterial,
                                                                                            vector<pair<unsigned int, unsigned int> >(
                                                                                                    1)));
    _materialIndexStartStop.find(currentMaterial)->second.back().first = indicesSeen;

    while (getline(in, line)) {
        if (line.length() > 1 && line.at(0) == '\t')
            line = line.substr(1);
        line.erase(line.find_last_not_of(" \n\r\t") + 1);

        vector<string> tokens = _tokenizeString(line, " \t");
        if (tokens.size() < 1) continue;

        //the line should have a single character that lets us know if it's a...
        if (!tokens[0].compare("#") ||
            tokens[0].find_first_of("#") == 0) {                                // comment ignore
        } else if (!tokens[0].compare("o")) {                        // object name ignore

        } else if (!tokens[0].compare("g")) {                        // polygon group name ignore

        } else if (!tokens[0].compare("mtllib")) {                    // material library

        } else if (!tokens[0].compare("usemtl")) {                    // use material library
            if (currentMaterial == "default" && indicesSeen == 0) {
                _materialIndexStartStop.clear();
            } else {
                _materialIndexStartStop.find(currentMaterial)->second.back().second = indicesSeen - 1;
            }
            currentMaterial = tokens[1];
            if (_materialIndexStartStop.find(currentMaterial) == _materialIndexStartStop.end()) {
                _materialIndexStartStop.insert(pair<string, vector<pair<unsigned int, unsigned int> > >(currentMaterial,
                                                                                                        vector<pair<unsigned int, unsigned int> >(
                                                                                                                1)));
                _materialIndexStartStop.find(currentMaterial)->second.back().first = indicesSeen;
            } else {
                _materialIndexStartStop.find(currentMaterial)->second.push_back(
                        pair<unsigned int, unsigned int>(indicesSeen, -1));
            }
        } else if (!tokens[0].compare("s")) {                        // smooth shading

        } else if (!tokens[0].compare("v")) {                        //vertex
            double x = atof(tokens[1].c_str()),
                    y = atof(tokens[2].c_str()),
                    z = atof(tokens[3].c_str());

            v[vSeen * 3 + 0] = x;
            v[vSeen * 3 + 1] = y;
            v[vSeen * 3 + 2] = z;

            vSeen++;
        } else if (!tokens.at(0).compare("vn")) {                    //vertex normal
            double x = atof(tokens[1].c_str()),
                    y = atof(tokens[2].c_str()),
                    z = atof(tokens[3].c_str());

            vn[vnSeen * 3 + 0] = x;
            vn[vnSeen * 3 + 1] = y;
            vn[vnSeen * 3 + 2] = z;

            vnSeen++;
        } else if (!tokens.at(0).compare("vt")) {                    //vertex tex coord
            double s = atof(tokens[1].c_str()),
                    t = atof(tokens[2].c_str());

            vt[vtSeen * 2 + 0] = s;
            vt[vtSeen * 2 + 1] = t;

            vtSeen++;
        } else if (!tokens.at(0).compare("f")) {                     //face!

            //now, faces can be either quads or triangles (or maybe more?)
            //split the string on spaces to get the number of verts+attrs.
            vector<string> faceTokens = _tokenizeString(line, " ");

            for (unsigned int i = 1; i < faceTokens.size(); i++) {
                if (uniqueCounts.find(faceTokens[i]) == uniqueCounts.end()) {
                    uniqueCounts.insert(pair<string, unsigned long int>(faceTokens[i], uniqueV));

                    //need to use both the tokens and number of slashes to determine what info is there.
                    vector<string> groupTokens = _tokenizeString(faceTokens[i], "/");
                    int numSlashes = 0;
                    for (unsigned int j = 0; j < faceTokens[i].length(); j++) {
                        if (faceTokens[i][j] == '/') numSlashes++;
                    }

                    if (_hasVertexNormals || !AUTO_GEN_NORMALS) {
                        //regardless, we always get a vertex index.
                        int vI = atoi(groupTokens[0].c_str());
                        if (vI < 0)
                            vI = vSeen + vI + 1;

                        _vertices[_uniqueIndex * 3 + 0] = v[((vI - 1) * 3) + 0];
                        _vertices[_uniqueIndex * 3 + 1] = v[((vI - 1) * 3) + 1];
                        _vertices[_uniqueIndex * 3 + 2] = v[((vI - 1) * 3) + 2];

                        //based on combination of number of tokens and slashes, we can determine what we have.
                        if (groupTokens.size() == 2 && numSlashes == 1) {
                            int vtI = atoi(groupTokens[1].c_str());
                            if (vtI < 0)
                                vtI = vtSeen + vtI + 1;

                            _texCoords[_uniqueIndex * 2 + 0] = vt[((vtI - 1) * 2) + 0];
                            _texCoords[_uniqueIndex * 2 + 1] = vt[((vtI - 1) * 2) + 1];
                        } else if (groupTokens.size() == 2 && numSlashes == 2) {
                            int vnI = atoi(groupTokens[1].c_str());
                            if (vnI < 0)
                                vnI = vnSeen + vnI + 1;

                            _normals[_uniqueIndex * 3 + 0] = vn[((vnI - 1) * 3) + 0];
                            _normals[_uniqueIndex * 3 + 1] = vn[((vnI - 1) * 3) + 1];
                            _normals[_uniqueIndex * 3 + 2] = vn[((vnI - 1) * 3) + 2];
                        } else if (groupTokens.size() == 3) {
                            int vtI = atoi(groupTokens[1].c_str());
                            if (vtI < 0)
                                vtI = vtSeen + vtI + 1;

                            _texCoords[_uniqueIndex * 2 + 0] = vt[((vtI - 1) * 2) + 0];
                            _texCoords[_uniqueIndex * 2 + 1] = vt[((vtI - 1) * 2) + 1];

                            int vnI = atoi(groupTokens[2].c_str());
                            if (vnI < 0)
                                vnI = vnSeen + vnI + 1;

                            _normals[_uniqueIndex * 3 + 0] = vn[((vnI - 1) * 3) + 0];
                            _normals[_uniqueIndex * 3 + 1] = vn[((vnI - 1) * 3) + 1];
                            _normals[_uniqueIndex * 3 + 2] = vn[((vnI - 1) * 3) + 2];
                        }

                        _uniqueIndex++;
                    } else {
                        //regardless, we always get a vertex index.
                        int vI = atoi(groupTokens[0].c_str());
                        if (vI < 0)
                            vI = vSeen + vI + 1;

                        vertsTemp.push_back(v[((vI - 1) * 3) + 0]);
                        vertsTemp.push_back(v[((vI - 1) * 3) + 1]);
                        vertsTemp.push_back(v[((vI - 1) * 3) + 2]);

                        //based on combination of number of tokens and slashes, we can determine what we have.
                        if (groupTokens.size() == 2 && numSlashes == 1) {
                            int vtI = atoi(groupTokens[1].c_str());
                            if (vtI < 0)
                                vtI = vtSeen + vtI + 1;

                            texCoordsTemp.push_back(vt[((vtI - 1) * 2) + 0]);
                            texCoordsTemp.push_back(vt[((vtI - 1) * 2) + 1]);
                        } else if (groupTokens.size() == 2 && numSlashes == 2) {
                            // should not occur if no normals
                            if (ERRORS)
                                fprintf(stderr,
                                        "[.obj]: [WARN]: no vertex normals were specified, should not be trying to access values\n");
                        } else if (groupTokens.size() == 3) {
                            int vtI = atoi(groupTokens[1].c_str());
                            if (vtI < 0)
                                vtI = vtSeen + vtI + 1;

                            texCoordsTemp.push_back(vt[((vtI - 1) * 2) + 0]);
                            texCoordsTemp.push_back(vt[((vtI - 1) * 2) + 1]);

                            // should not occur if no normals
                            if (ERRORS)
                                fprintf(stderr,
                                        "[.obj]: [WARN]: no vertex normals were specified, should not be trying to access values\n");
                        }
                    }
                    uniqueV++;
                }
            }

            for (unsigned int i = 2; i < faceTokens.size() - 1; i++) {
                if (_hasVertexNormals || !AUTO_GEN_NORMALS) {
                    _indices[indicesSeen++] = uniqueCounts.find(faceTokens[1])->second;
                    _indices[indicesSeen++] = uniqueCounts.find(faceTokens[i])->second;
                    _indices[indicesSeen++] = uniqueCounts.find(faceTokens[i + 1])->second;

                    _numIndices += 3;
                } else {
                    int aI = uniqueCounts.find(faceTokens[1])->second;
                    int bI = uniqueCounts.find(faceTokens[i])->second;
                    int cI = uniqueCounts.find(faceTokens[i + 1])->second;

                    glm::vec3 a(vertsTemp[aI * 3 + 0], vertsTemp[aI * 3 + 1], vertsTemp[aI * 3 + 2]);
                    glm::vec3 b(vertsTemp[bI * 3 + 0], vertsTemp[bI * 3 + 1], vertsTemp[bI * 3 + 2]);
                    glm::vec3 c(vertsTemp[cI * 3 + 0], vertsTemp[cI * 3 + 1], vertsTemp[cI * 3 + 2]);

                    glm::vec3 ab = b - a;
                    glm::vec3 ac = c - a;
                    glm::vec3 ba = a - b;
                    glm::vec3 bc = c - b;
                    glm::vec3 ca = a - c;
                    glm::vec3 cb = b - c;

                    glm::vec3 aN = glm::normalize(glm::cross(ab, ac));
                    glm::vec3 bN = glm::normalize(glm::cross(bc, ba));
                    glm::vec3 cN = glm::normalize(glm::cross(ca, cb));

                    _vertices[_uniqueIndex * 3 + 0] = a.x;
                    _vertices[_uniqueIndex * 3 + 1] = a.y;
                    _vertices[_uniqueIndex * 3 + 2] = a.z;

                    _normals[_uniqueIndex * 3 + 0] = aN.x;
                    _normals[_uniqueIndex * 3 + 1] = aN.y;
                    _normals[_uniqueIndex * 3 + 2] = aN.z;

                    if (_hasVertexTexCoords) {
                        _texCoords[_uniqueIndex * 2 + 0] = texCoordsTemp[aI * 2 + 0];
                        _texCoords[_uniqueIndex * 2 + 1] = texCoordsTemp[aI * 2 + 1];
                    }

                    _indices[_numIndices++] = _uniqueIndex++;
                    indicesSeen++;

                    _vertices[_uniqueIndex * 3 + 0] = b.x;
                    _vertices[_uniqueIndex * 3 + 1] = b.y;
                    _vertices[_uniqueIndex * 3 + 2] = b.z;

                    _normals[_uniqueIndex * 3 + 0] = bN.x;
                    _normals[_uniqueIndex * 3 + 1] = bN.y;
                    _normals[_uniqueIndex * 3 + 2] = bN.z;

                    if (_hasVertexTexCoords) {
                        _texCoords[_uniqueIndex * 2 + 0] = texCoordsTemp[bI * 2 + 0];
                        _texCoords[_uniqueIndex * 2 + 1] = texCoordsTemp[bI * 2 + 1];
                    }

                    _indices[_numIndices++] = _uniqueIndex++;
                    indicesSeen++;

                    _vertices[_uniqueIndex * 3 + 0] = c.x;
                    _vertices[_uniqueIndex * 3 + 1] = c.y;
                    _vertices[_uniqueIndex * 3 + 2] = c.z;

                    _normals[_uniqueIndex * 3 + 0] = cN.x;
                    _normals[_uniqueIndex * 3 + 1] = cN.y;
                    _normals[_uniqueIndex * 3 + 2] = cN.z;

                    if (_hasVertexTexCoords) {
                        _texCoords[_uniqueIndex * 2 + 0] = texCoordsTemp[cI * 2 + 0];
                        _texCoords[_uniqueIndex * 2 + 1] = texCoordsTemp[cI * 2 + 1];
                    }

                    _indices[_numIndices++] = _uniqueIndex++;
                    indicesSeen++;
                }
            }

        } else {
            if (INFO) printf("[.obj]: ignoring line: %s\n", line.c_str());
        }

        if (INFO) {
            progressCounter++;
            if (progressCounter % 5000 == 0) {
                printf("\33[2K\r");
                switch (progressCounter) {
                    case 5000:
                        printf("[.obj]: parsing %s...\\", _filename);
                        break;
                    case 10000:
                        printf("[.obj]: parsing %s...|", _filename);
                        break;
                    case 15000:
                        printf("[.obj]: parsing %s.../", _filename);
                        break;
                    case 20000:
                        printf("[.obj]: parsing %s...-", _filename);
                        break;
                }
                fflush(stdout);
            }
            if (progressCounter == 20000)
                progressCounter = 0;
        }
    }

    in.close();

    if (INFO) {
        printf("\33[2K\r");
        printf("[.obj]: parsing %s...done!\n", _filename);
    }

    _materialIndexStartStop.find(currentMaterial)->second.back().second = indicesSeen - 1;

    glBindVertexArray(_vaod);
    glBindBuffer(GL_ARRAY_BUFFER, _vbods[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * _uniqueIndex * 8, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * _uniqueIndex * 3, _vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * _uniqueIndex * 3, sizeof(GLfloat) * _uniqueIndex * 3, _normals);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * _uniqueIndex * 6, sizeof(GLfloat) * _uniqueIndex * 2,
                    _texCoords);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbods[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indicesSeen, _indices, GL_STATIC_DRAW);

    time(&end);
    double seconds = difftime(end, start);

    if (INFO) {
        printf("[.obj]: Completed in %.3fs\n", seconds);
        printf("[.obj]: -=-=-=-=-=-=-=-  END %s Info  -=-=-=-=-=-=-=- \n\n", _filename);
    }

    return result;
}

inline bool CSCI444::ModelLoaderSDF::_loadMTLFile(const char *mtlFilename, bool INFO, bool ERRORS) {
    bool result = true;

    if (INFO) printf("[.mtl]: -*-*-*-*-*-*-*- BEGIN %s Info -*-*-*-*-*-*-*-\n", mtlFilename);

    string line;
    string path;
    if (strstr(_filename, "/") != NULL) {
        path = string(_filename).substr(0, string(_filename).find_last_of("/") + 1);
    } else {
        path = "./";
    }

    ifstream in;
    in.open(mtlFilename);
    if (!in.is_open()) {
        string folderMtlFile = path + mtlFilename;
        in.open(folderMtlFile.c_str());
        if (!in.is_open()) {
            if (ERRORS) fprintf(stderr, "[.mtl]: [ERROR]: could not open material file: %s\n", mtlFilename);
            if (INFO) printf("[.mtl]: -*-*-*-*-*-*-*-  END %s Info  -*-*-*-*-*-*-*-\n", mtlFilename);
            return false;
        }
    }

    CSCI441_INTERNAL::ModelMaterial *currentMaterial = NULL;
    string materialName;

    unsigned char *textureData = NULL;
    unsigned char *maskData = NULL;
    unsigned char *fullData;
    int texWidth, texHeight, textureChannels = 1, maskChannels = 1;
    GLuint textureHandle = 0;

    map<string, GLuint> imageHandles;

    int numMaterials = 0;

    while (getline(in, line)) {
        if (line.length() > 1 && line.at(0) == '\t')
            line = line.substr(1);
        line.erase(line.find_last_not_of(" \n\r\t") + 1);

        vector<string> tokens = _tokenizeString(line, " /");
        if (tokens.size() < 1) continue;

        //the line should have a single character that lets us know if it's a...
        if (!tokens[0].compare("#")) {                            // comment
        } else if (!tokens[0].compare("newmtl")) {                //new material
            if (INFO) printf("[.mtl]: Parsing material %s properties\n", tokens[1].c_str());
            currentMaterial = new CSCI441_INTERNAL::ModelMaterial();
            materialName = tokens[1];
            _materials.insert(pair<string, CSCI441_INTERNAL::ModelMaterial *>(materialName, currentMaterial));

            textureHandle = 0;
            textureData = NULL;
            maskData = NULL;
            textureChannels = 1;
            maskChannels = 1;

            numMaterials++;
        } else if (!tokens[0].compare("Ka")) {                    // ambient component
            currentMaterial->ambient[0] = atof(tokens[1].c_str());
            currentMaterial->ambient[1] = atof(tokens[2].c_str());
            currentMaterial->ambient[2] = atof(tokens[3].c_str());
        } else if (!tokens[0].compare("Kd")) {                    // diffuse component
            currentMaterial->diffuse[0] = atof(tokens[1].c_str());
            currentMaterial->diffuse[1] = atof(tokens[2].c_str());
            currentMaterial->diffuse[2] = atof(tokens[3].c_str());
        } else if (!tokens[0].compare("Ks")) {                    // specular component
            currentMaterial->specular[0] = atof(tokens[1].c_str());
            currentMaterial->specular[1] = atof(tokens[2].c_str());
            currentMaterial->specular[2] = atof(tokens[3].c_str());
        } else if (!tokens[0].compare("Ke")) {                    // emissive component
            currentMaterial->emissive[0] = atof(tokens[1].c_str());
            currentMaterial->emissive[1] = atof(tokens[2].c_str());
            currentMaterial->emissive[2] = atof(tokens[3].c_str());
        } else if (!tokens[0].compare("Ns")) {                    // shininess component
            currentMaterial->shininess = atof(tokens[1].c_str());
        } else if (!tokens[0].compare("Tr")
                   || !tokens[0].compare(
                "d")) {                    // transparency component - Tr or d can be used depending on the format
            currentMaterial->ambient[3] = atof(tokens[1].c_str());
            currentMaterial->diffuse[3] = atof(tokens[1].c_str());
            currentMaterial->specular[3] = atof(tokens[1].c_str());
        } else if (!tokens[0].compare("illum")) {                // illumination type component
            // TODO ?
        } else if (!tokens[0].compare("map_Kd")) {                // diffuse color texture map
            if (imageHandles.find(tokens[1]) != imageHandles.end()) {
                // _textureHandles->insert( pair< string, GLuint >( materialName, imageHandles.find( tokens[1] )->second ) );
                currentMaterial->map_Kd = imageHandles.find(tokens[1])->second;
            } else {
                textureData = SOIL_load_image(tokens[1].c_str(), &texWidth, &texHeight, &textureChannels,
                                              SOIL_LOAD_AUTO);
                if (!textureData) {
                    string folderName = path + tokens[1];
                    textureData = SOIL_load_image(folderName.c_str(), &texWidth, &texHeight, &textureChannels,
                                                  SOIL_LOAD_AUTO);
                    if (textureData) {
                        CSCI441_INTERNAL::flipImageY(texWidth, texHeight, textureChannels, textureData);
                    }
                } else {
                    CSCI441_INTERNAL::flipImageY(texWidth, texHeight, textureChannels, textureData);
                }

                if (!textureData) {
                    if (ERRORS) fprintf(stderr, "[.mtl]: [ERROR]: File Not Found: %s\n", tokens[1].c_str());
                } else {
                    if (INFO)
                        printf("[.mtl]: TextureMap:\t%s\tSize: %dx%d\tColors: %d\n", tokens[1].c_str(), texWidth,
                               texHeight, textureChannels);

                    if (maskData == NULL) {
                        if (textureHandle == 0) {
                            glGenTextures(1, &textureHandle);
                            imageHandles.insert(pair<string, GLuint>(tokens[1], textureHandle));
                        }

                        glBindTexture(GL_TEXTURE_2D, textureHandle);

                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                        GLenum colorSpace = GL_RGB;
                        if (textureChannels == 4)
                            colorSpace = GL_RGBA;
                        glTexImage2D(GL_TEXTURE_2D, 0, colorSpace, texWidth, texHeight, 0, colorSpace, GL_UNSIGNED_BYTE,
                                     textureData);

                        currentMaterial->map_Kd = textureHandle;
                    } else {
                        fullData = CSCI441_INTERNAL::createTransparentTexture(textureData, maskData, texWidth,
                                                                              texHeight, textureChannels, maskChannels);

                        if (textureHandle == 0) {
                            glGenTextures(1, &textureHandle);
                            imageHandles.insert(pair<string, GLuint>(tokens[1], textureHandle));
                        }

                        glBindTexture(GL_TEXTURE_2D, textureHandle);

                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                     fullData);

                        delete fullData;

                        currentMaterial->map_Kd = textureHandle;
                    }
                }
            }
        } else if (!tokens[0].compare("map_d")) {                // alpha texture map
            if (imageHandles.find(tokens[1]) != imageHandles.end()) {
                // _textureHandles->insert( pair< string, GLuint >( materialName, imageHandles.find( tokens[1] )->second ) );
                currentMaterial->map_d = imageHandles.find(tokens[1])->second;
            } else {
                maskData = SOIL_load_image(tokens[1].c_str(), &texWidth, &texHeight, &textureChannels, SOIL_LOAD_AUTO);
                if (!textureData) {
                    string folderName = path + tokens[1];
                    maskData = SOIL_load_image(folderName.c_str(), &texWidth, &texHeight, &textureChannels,
                                               SOIL_LOAD_AUTO);
                    if (maskData) {
                        CSCI441_INTERNAL::flipImageY(texWidth, texHeight, textureChannels, maskData);
                    }
                } else {
                    CSCI441_INTERNAL::flipImageY(texWidth, texHeight, textureChannels, maskData);
                }

                if (!maskData) {
                    if (ERRORS) fprintf(stderr, "[.mtl]: [ERROR]: File Not Found: %s\n", tokens[1].c_str());
                } else {
                    if (INFO)
                        printf("[.mtl]: AlphaMap:  \t%s\tSize: %dx%d\tColors: %d\n", tokens[1].c_str(), texWidth,
                               texHeight, maskChannels);

                    if (textureData != NULL) {
                        fullData = CSCI441_INTERNAL::createTransparentTexture(textureData, maskData, texWidth,
                                                                              texHeight, textureChannels, maskChannels);

                        if (textureHandle == 0)
                            glGenTextures(1, &textureHandle);

                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                     fullData);

                        delete fullData;
                    }
                }
            }
        } else if (!tokens[0].compare("map_Ka")) {                // ambient color texture map

        } else if (!tokens[0].compare("map_Ks")) {                // specular color texture map

        } else if (!tokens[0].compare("map_Ns")) {                // specular highlight map (shininess map)

        } else if (!tokens[0].compare("Ni")) {                        // optical density / index of refraction

        } else if (!tokens[0].compare("Tf")) {                        // transmission filter

        } else if (!tokens[0].compare("bump")
                   || !tokens[0].compare("map_bump")) {                // bump map

        } else {
            if (INFO) printf("[.mtl]: ignoring line: %s\n", line.c_str());
        }
    }

    in.close();

    if (INFO) {
        printf("[.mtl]: Materials:\t%d\n", numMaterials);
        printf("[.mtl]: -*-*-*-*-*-*-*-  END %s Info  -*-*-*-*-*-*-*-\n", mtlFilename);
    }

    return result;
}

inline void CSCI444::ModelLoaderSDF::setSDFLocation(GLint sdfLoc) {
    this->_sdfLoc = sdfLoc;
}

inline void CSCI444::ModelLoaderSDF::setTriangleLocation(GLint triLoc) {
    this->_triangleLoc = triLoc;
}

inline void CSCI444::ModelLoaderSDF::enableAutoGenerateNormals() {
    AUTO_GEN_NORMALS = true;
}

inline void CSCI444::ModelLoaderSDF::disableAutoGenerateNormals() {
    AUTO_GEN_NORMALS = false;
}

//
//  vector<string> tokenizeString(string input, string delimiters)
//
//      This is a helper function to break a single string into std::vector
//  of strings, based on a given set of delimiter characters.
//
inline vector<string> CSCI444::ModelLoaderSDF::_tokenizeString(string input, string delimiters) {
    if (input.size() == 0)
        return vector<string>();

    vector<string> retVec = vector<string>();
    size_t oldR = 0, r = 0;

    //strip all delimiter characters from the front and end of the input string.
    string strippedInput;
    int lowerValidIndex = 0, upperValidIndex = input.size() - 1;
    while ((unsigned int) lowerValidIndex < input.size() &&
           delimiters.find_first_of(input.at(lowerValidIndex), 0) != string::npos)
        lowerValidIndex++;

    while (upperValidIndex >= 0 && delimiters.find_first_of(input.at(upperValidIndex), 0) != string::npos)
        upperValidIndex--;

    //if the lowest valid index is higher than the highest valid index, they're all delimiters! return nothing.
    if ((unsigned int) lowerValidIndex >= input.size() || upperValidIndex < 0 || lowerValidIndex > upperValidIndex)
        return vector<string>();

    //remove the delimiters from the beginning and end of the string, if any.
    strippedInput = input.substr(lowerValidIndex, upperValidIndex - lowerValidIndex + 1);

    //search for each instance of a delimiter character, and create a new token spanning
    //from the last valid character up to the delimiter character.
    while ((r = strippedInput.find_first_of(delimiters, oldR)) != string::npos) {
        if (oldR != r)           //but watch out for multiple consecutive delimiters!
            retVec.push_back(strippedInput.substr(oldR, r - oldR));
        oldR = r + 1;
    }
    if (r != 0)
        retVec.push_back(strippedInput.substr(oldR, r - oldR));

    return retVec;
}

inline unsigned char *
CSCI441_INTERNAL::createTransparentTexture(unsigned char *imageData, unsigned char *imageMask, int texWidth,
                                           int texHeight, int texChannels, int maskChannels) {
    //combine the 'mask' array with the image data array into an RGBA array.
    unsigned char *fullData = new unsigned char[texWidth * texHeight * 4];

    for (int j = 0; j < texHeight; j++) {
        for (int i = 0; i < texWidth; i++) {
            if (imageData) {
                fullData[(j * texWidth + i) * 4 + 0] = imageData[(j * texWidth + i) * texChannels + 0];    // R
                fullData[(j * texWidth + i) * 4 + 1] = imageData[(j * texWidth + i) * texChannels + 1];    // G
                fullData[(j * texWidth + i) * 4 + 2] = imageData[(j * texWidth + i) * texChannels + 2];    // B
            } else {
                fullData[(j * texWidth + i) * 4 + 0] = 1;    // R
                fullData[(j * texWidth + i) * 4 + 1] = 1;    // G
                fullData[(j * texWidth + i) * 4 + 2] = 1;    // B
            }

            if (imageMask) {
                fullData[(j * texWidth + i) * 4 + 3] = imageMask[(j * texWidth + i) * maskChannels + 0];    // A
            } else {
                fullData[(j * texWidth + i) * 4 + 3] = 1;    // A
            }
        }
    }
    return fullData;
}

inline void CSCI441_INTERNAL::flipImageY(int texWidth, int texHeight, int textureChannels, unsigned char *textureData) {
    for (int j = 0; j < texHeight / 2; j++) {
        for (int i = 0; i < texWidth; i++) {
            for (int k = 0; k < textureChannels; k++) {
                int top = (j * texWidth + i) * textureChannels + k;
                int bot = ((texHeight - j - 1) * texWidth + i) * textureChannels + k;

                unsigned char t = textureData[top];
                textureData[top] = textureData[bot];
                textureData[bot] = t;

            }
        }
    }
}

#endif // __CSCI444_MODELLOADER_SDF_HPP__
