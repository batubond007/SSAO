cbuffer constants : register(b0)
{
    float4 uniformColor;
    float4x4 modellingMatrix;
    float4x4 viewingMatrix;
    float4x4 projectionMatrix;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

/* vertex attributes go here to input to the vertex shader */
struct vs_in {
    float3 position_local : POS;
    float2 uv : TEX;
};

/* outputs from vertex shader go here. can be interpolated to pixel shader */
struct vs_out {
    float4 position_clip : SV_POSITION; // required output of VS
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float4 selfSpacePos : TEXCOORD1;
};

vs_out vs_main(vs_in input) {
    vs_out output = (vs_out)0; // zero the memory first
    output.uv = input.uv;

    output.selfSpacePos = float4(input.position_local, 1.0);
    float4 worldSpace = mul(modellingMatrix, float4(input.position_local, 1.0));
    float4 viewSpace = mul(viewingMatrix, worldSpace);
    output.position_clip = mul(projectionMatrix, viewSpace);
    output.color = uniformColor;
    return output;
}

float4 ps_main(vs_out input) : SV_TARGET{
     return mytexture.Sample(mysampler, float2(input.uv.x, 1-input.uv.y)).r;
}
