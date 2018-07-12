#version 450 core
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec3 vertex_position; // model space
layout(location = 2) in vec2 vertex_tex_coord;

layout(location = 0) out vec2 out_tex_coord;

layout(binding = 0) uniform vert_shader_block {
    mat4 model_to_clip_transform; // projection transform * view transform * model transform
};

void main()
{
    gl_Position = model_to_clip_transform * vec4(vertex_position, 1.0f);
    out_tex_coord = vertex_tex_coord;
}
