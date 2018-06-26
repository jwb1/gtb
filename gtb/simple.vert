#version 450 core
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec2 vert_position;
layout(location = 1) in vec2 vert_tex_coord;

layout(location = 0) out vec2 out_tex_coord;

void main()
{
    gl_Position = vec4(vert_position, 0.0f, 1.0f);
    out_tex_coord = vert_tex_coord;
}
