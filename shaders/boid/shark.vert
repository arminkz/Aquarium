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

    // Tail weight: 0 at head (+Z ~7.0), 1 at tail (-Z ~-7.05).
    float tw = clamp(-inPosition.z / 7.05, 0.0, 1.0);
    //tw = max(0.0, (tw - 0.5) * 2.0); // linear ramp over last 1/2
    float tailWeight = pow(tw, 3.0);

    // Slow, powerful beat. Minimal spatial travel along the body so the flex
    // stays isolated at the caudal fin rather than rippling forward.
    float phase = si.time * 6.2 + inPosition.z * 2.2;
    float wave  = sin(phase) * tailWeight * 0.62;   // lateral sweep
    float twist = cos(phase) * tailWeight * 0.08;   // barely any body roll

    vec3 animatedPos    = inPosition + vec3(wave, inPosition.y * twist, 0.0);
    vec3 animatedNormal = normalize(inNormal + vec3(wave * 0.3, twist * 0.1, 0.0));

    worldPosition  = model * vec4(animatedPos, 1.0);
    worldNormal    = (model * vec4(animatedNormal, 0.0)).xyz;
    worldTangent   = (model * vec4(inTangent, 0.0)).xyz;

    gl_Position  = si.proj * si.view * worldPosition;
    fragColor    = inColor;
    fragTexCoord = inTexCoord;
}
