// ���̓f�[�^
struct VSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

// �o�̓f�[�^
struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

// �萔�o�b�t�@
cbuffer Transform : register(b0)
{
    float4x4 World : packoffset(c0);
    float4x4 View : packoffset(c4);
    float4x4 Project : packoffset(c8);
};

// �G���g���[�|�C���g
VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    
    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(World, localPos);
    float4 viewPos = mul(View, worldPos);
    float4 projectPos = mul(Project, viewPos);
    
    output.Position = projectPos;
    output.Color = input.Color;
    
    return output;
}