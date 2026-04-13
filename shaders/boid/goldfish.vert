#version 450

layout(set = 0, binding = 0) uniform SceneInfo {
    mat4 view;
    mat4 proj;
    float time;
    vec3 cameraPosition;
    vec3 lightColor;
} si;

layout(set = 1, binding = 0) readonly buffer Transforms {
    mat4 transforms[];
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 worldPosition;
layout(location = 3) out vec3 worldNormal;
layout(location = 4) out vec3 worldTangent;

void main() {
    mat4 model = transforms[gl_InstanceIndex];

    // Tail animation: fish head is at +Z, tail at -Z (in local space after scaling)
    // tailWeight goes from 0 at the head to 1 at the tail tip
    float tw = clamp(-inPosition.z / 0.58, 0.0, 1.0);
    float tailWeight = pow(tw, 2.0); // Emphasize the tail tip more

    float phase = si.time * 8.0 + inPosition.z * 4.0;
    float wave = sin(phase) * tailWeight * 0.2;   // lateral displacement
    float twist = cos(phase) * tailWeight * 0.15;  // body twist (derivative of wave)

    vec3 animatedPos = inPosition + vec3(wave, inPosition.y * twist, 0.0);
    vec3 animatedNormal = normalize(inNormal + vec3(wave * 0.5, twist * 0.3, 0.0));
    vec3 animatedTangent = inTangent;

    worldPosition =  model * vec4(animatedPos, 1.0);
    worldNormal = (model * vec4(normalize(animatedNormal), 0.0)).xyz;
    worldTangent = (model * vec4(animatedTangent, 0.0)).xyz;

    gl_Position = si.proj * si.view * worldPosition;
    fragColor   = inColor;
    fragTexCoord = inTexCoord;
}
