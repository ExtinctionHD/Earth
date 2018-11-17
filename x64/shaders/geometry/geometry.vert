#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SpaceMatrix {
    mat4 matrix;
} space;

layout(set = 1, binding = 0) uniform ModelMatrix {
	mat4 matrix;
} model;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec2 fragUV;
layout (location = 2) out vec3 fragNormal;
layout (location = 3) out vec3 fragTangent;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
    // position of fragment in world coordinates
    fragPos = (model.matrix * vec4(inPos, 1.0f)).xyz;
    // texture coordinates without changes
    fragUV = inUV;
    // vertex normal vector in world coordinates
    fragNormal = (model.matrix * vec4(inNormal, 0.0f)).xyz;
    // tangent vector of vertex in world coordinates
    fragTangent = (model.matrix * vec4(inTangent, 0.0f)).xyz;

    gl_Position = space.matrix * model.matrix * vec4(inPos, 1.0f);
}
