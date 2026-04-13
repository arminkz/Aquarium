#version 450

layout(set = 0, binding = 0) uniform SceneInfo {
    mat4 view;
    mat4 proj;
    float time;
    vec3 cameraPosition;
    vec3 lightColor;
} si;

layout(set = 2, binding = 0) uniform sampler2D fishTexture;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 worldPosition;
layout(location = 3) in vec3 worldNormal;
layout(location = 4) in vec3 worldTangent;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(worldNormal);

    vec3 lightDir = normalize(vec3(1.0, 2.0, 1.0));
    float diffuse = max(dot(N, lightDir), 0.0);

    vec3 rimDir = normalize(vec3(-1.0, -0.5, -1.0));
    float rim   = pow(max(dot(N, rimDir), 0.0), 3.0) * 0.15;

    float ambient = 0.2;
    float light   = ambient + diffuse * 0.75 + rim;

    vec4 texColor = texture(fishTexture, fragTexCoord);
    outColor = vec4(texColor.rgb * light, texColor.a);
}
