#version 330
#extension GL_ARB_separate_shader_objects : require

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(std140) uniform type_u_EveryObject
{
    layout(row_major) mat4 u_worldViewProjectionMatrix;
    vec4 u_colour;
    vec4 u_screenSize;
} u_EveryObject;

layout(location = 0) in vec3 in_var_POSITION;
layout(location = 1) in vec3 in_var_NORMAL;
layout(location = 2) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec2 out_var_TEXCOORD0;
layout(location = 1) out vec4 out_var_COLOR0;
layout(location = 2) out vec2 out_var_TEXCOORD1;

vec2 _29;

void main()
{
    vec4 _39 = vec4(in_var_POSITION, 1.0) * u_EveryObject.u_worldViewProjectionMatrix;
    vec2 _44 = _29;
    _44.x = 1.0 + _39.w;
    gl_Position = _39;
    out_var_TEXCOORD0 = in_var_TEXCOORD0;
    out_var_COLOR0 = u_EveryObject.u_colour;
    out_var_TEXCOORD1 = _44;
}

