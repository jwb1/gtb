#version 450 core
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec2 tex_coord;

layout(location = 0) out vec4 frag_color;

layout(binding = 0) uniform sampler2D tex_sampler;

void main()
{
    frag_color = texture(tex_sampler, tex_coord);
}
