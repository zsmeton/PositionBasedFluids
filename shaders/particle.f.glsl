#version 430 core

// ***** FRAGMENT SHADER INPUT *****
layout(location=0) in vec3 color;
layout(location=1) in vec3 normalVec;
layout(location=2) in vec3 lightVec;
layout(location=3) in vec3 cameraVec;

// ***** FRAGMENT SHADER OUTPUT *****
layout(location=0) out vec4 fragColorOut;

// ***** FRAGMENT SHADER UNIFORMS *****
layout(shared, binding = 2) uniform Light{
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    vec3 position;
} light;


// ***** FRAGMENT SHADER SUBROUTINES *****
/*! Use blinn-phong illumination
 * @param rh reflection or halfway vector
 * @param n normal vector
 * @param c camera vector
 */
float blinnPhong(vec3 light, vec3 normal, vec3 camera){
    vec3 halfwayDir = normalize(light + camera);
    return dot(camera, halfwayDir);
}

/*! Use phong illumination
 * @param rh reflection or halfway vector
 * @param n normal vector
 * @param c camera vector
 */
float phong(vec3 light, vec3 normal, vec3 camera){
    vec3 reflectDir = reflect(-light, normal);
    return dot(camera, reflectDir);
}

void main() {
    /*************************
     * Lighting + Color
     *************************/
    // Normalize incoming vectors
    vec3 lightVec2 = normalize(lightVec);
    vec3 normalVec2 = normalize(normalVec);
    vec3 camVec2 = normalize(cameraVec);

    // Calculate material colors
    vec4 matDiffuse = vec4(color, 1.0);
    vec4 matAmbient = vec4(0.1 * vec3(color), 1.0);
    vec4 matSpecular = vec4(0.1*vec3(1.0), 1.0);
    float shininess = 0.1;

    // Calculate diffuse component
    float sDotN = max(dot(lightVec2, normalVec2), 0.0);
    vec4 diffuse = light.diffuse * matDiffuse * sDotN;

    // Calculate ambient component
    vec4 ambient = light.ambient * matAmbient;

    // Calculate specular component
    vec4 specular = vec4(0.0);
    if (sDotN > 0.0)
    specular = light.specular * matSpecular * pow(max(0.0, phong(lightVec2, normalVec2, camVec2)), shininess);

    // Sum components
    fragColorOut = diffuse + ambient + specular;

    // if viewing the backside of the fragment,
    // reverse the colors as a visual cue
    if (!gl_FrontFacing) {
        fragColorOut.rgb = fragColorOut.bgr;
    }
}
