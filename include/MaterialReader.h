//
// Created by zsmeton on 10/30/19.
//

#ifndef A5_MATERIALREADER_H
#define A5_MATERIALREADER_H
#include <stdio.h>                // for printf, fileio functionality
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <exception>
#include <vector>

using namespace std;

struct Material{
    float diffuse [4];
    float specular [4];
    float ambient [4];
    float shininess[1];
};

class MaterialSettings{
private:
    vector<Material> allMats;
    unordered_map<string, Material> materials;
public:

    /*!
     * Reads in the materials from a file with the following format
     * <number of materials>
     * <material 1 name>
     * <ambient components> <diffuse components> <specular components> <shininess (based on blinn-phong)>
     * ...
     * @param filename file to open up
     * @return number of materials read in or -1 on error
     */
    int loadMaterials(string filename){
        FILE *fp;
        fp = fopen(&filename[0], "r");
        if(fp == nullptr){
            fprintf(stderr, "[ERROR]: Could not open file %s for material loading\n", filename.c_str());
            return -1;
        }
        int numMaterials = -1;
        fscanf(fp, "%d", &numMaterials);
        if(numMaterials == -1){
            fprintf(stderr, "[ERROR]: File %s of invalid format for material loading\n", filename.c_str());
            return -1;
        }
        for(int i = 0; i < numMaterials; i++){
            char buffer[100];
            fscanf(fp, "%s", buffer);
            string matName = buffer;
            if(matName == ""){
                fprintf(stderr, "[ERROR]: File %s of invalid format for material loading\n", filename.c_str());
                return -1;
            }
            Material tempMat{};
            tempMat.ambient[3] = 1.0f;
            tempMat.diffuse[3] = 1.0f;
            tempMat.specular[3] = 1.0f;
            fscanf(fp, "%f %f %f %f %f %f %f %f %f %f", &tempMat.ambient[0], &tempMat.ambient[1], &tempMat.ambient[2],
                                                              &tempMat.diffuse[0], &tempMat.diffuse[1], &tempMat.diffuse[2],
                                                              &tempMat.specular[0], &tempMat.specular[1], &tempMat.specular[2],
                                                              &tempMat.shininess[0]);
            tempMat.shininess[0] *= 128;
            materials[matName] = tempMat;
            allMats.push_back(tempMat);
        }

        fclose(fp);
        return numMaterials;
    }

    Material getSwatch(string materialName){
        if(materials.count(materialName)){
            return materials[materialName];
        }
        else{
            throw runtime_error("Invalid material name\n");
        }
    }

    vector<Material> getMaterials(){
        return allMats;
    }
};

#endif //A5_MATERIALREADER_H
