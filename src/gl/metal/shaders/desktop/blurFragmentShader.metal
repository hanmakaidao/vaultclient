#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 out_var_SV_Target [[color(0)]];
};

struct main0_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
    float2 in_var_TEXCOORD1 [[user(locn1)]];
    float2 in_var_TEXCOORD2 [[user(locn2)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> colourTexture [[texture(0)]], sampler colourSampler [[sampler(0)]])
{
    main0_out out = {};
    out.out_var_SV_Target = ((float4(0.0, 0.279009997844696044921875, 0.0, 0.0) * colourTexture.sample(colourSampler, in.in_var_TEXCOORD0)) + (float4(1.0, 0.44198000431060791015625, 0.0, 0.0) * colourTexture.sample(colourSampler, in.in_var_TEXCOORD1))) + (float4(0.0, 0.279009997844696044921875, 0.0, 0.0) * colourTexture.sample(colourSampler, in.in_var_TEXCOORD2));
    return out;
}

