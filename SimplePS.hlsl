// 頂点シェーダの出力データ
struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

// 出力データ
struct PSOutput
{
    float4 Color : SV_TARGET0;
};

// エントリーポイント
PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;
    output.Color = input.Color;
    return output;
}