// ���_�V�F�[�_�̏o�̓f�[�^
struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

// �o�̓f�[�^
struct PSOutput
{
    float4 Color : SV_TARGET0;
};

// �G���g���[�|�C���g
PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;
    output.Color = input.Color;
    return output;
}