#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float2 in_var_TEXCOORD0 [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.gl_Position = float4(in.in_var_POSITION.xy, 0.0, 1.0);
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    return out;
}

