#version 450 core
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec2 tex_coord;

layout(location = 0) out vec4 frag_color;

//layout(binding = 1) uniform sampler2D tex_sampler;

layout(binding = 1) uniform frag_shader_block {
    vec3 color;
};

void main()
{
    //frag_color = texture(tex_sampler, tex_coord);
    frag_color = vec4(color, 1.0f);
}
