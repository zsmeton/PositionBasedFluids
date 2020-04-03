#version 430 core

// ***** FRAGMENT SHADER INPUT *****
layout(location=0) in vec3 normalVec;
layout(location=1) in vec3 lightVec;
layout(location=2) in vec3 cameraVec;

// ***** FRAGMENT SHADER UNIFORMS *****
layout(shared, binding = 2) uniform Light{
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    vec3 position;
} light;

layout(shared, binding = 1) uniform Material{
    vec4 diffuse;
    vec4 specular;
    float shininess;
    vec4 ambient;
} mat;


// ***** FRAGMENT SHADER OUTPUT *****
layout(location=0) out vec4 fragColorOut;


// ***** FRAGMENT SHADER HELPER FUNCTIONS *****
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

    // Calculate diffuse component
    float sDotN = max( dot(lightVec2, normalVec2), 0.0 );
    vec4 diffuse = light.diffuse * mat.diffuse * sDotN;

    // Calculate ambient component
    vec4 ambient = light.ambient * mat.ambient;

    // Calculate specular component
    vec4 specular = vec4(0.0);
    if( sDotN > 0.0 )
    specular = light.specular * mat.specular * pow(max(0.0, phong(lightVec2, normalVec2, camVec2)), mat.shininess);

    // Sum components
    fragColorOut = diffuse + ambient + specular;

    // if viewing the backside of the fragment,
    // reverse the colors as a visual cue
    if( !gl_FrontFacing ) {
        fragColorOut.rgb = fragColorOut.bgr;
    }
}
