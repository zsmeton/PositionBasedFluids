#version 430 core

// ***** VERTEX SHADER INPUT *****
layout(location=0) in vec3 vPos;
layout(location=1) in vec3 vNormal;


// ***** VERTEX SHADER OUTPUT *****
layout(location=0) out vec3 normalVec;
layout(location=1) out vec3 lightVec;
layout(location=2) out vec3 cameraVec;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_ClipDistance[];
};


// ***** VERTEX SHADER UNIFORMS *****
layout(shared, binding = 0) uniform Matricies{
    mat4 modelView;
    mat4 projection;
    mat4 view;
    mat4 normal;
    mat4 viewPort;
} mtx;

layout(shared, binding = 2) uniform Light{
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    vec3 position;
} light;


void main() {
    vec4 posVec = mtx.modelView * vec4(vPos,1.0);
    // Calculate position
    gl_Position = mtx.projection * posVec;
    // Calculate vertex normal
    normalVec = normalize( (mtx.normal * vec4(vNormal,0.0)).xyz );
    // Calculate the light vector
    lightVec = normalize( (mtx.view * vec4(light.position,1.0)).xyz - posVec.xyz );
    // Calculate the camera vector
    cameraVec = normalize(-(posVec).xyz);
}
