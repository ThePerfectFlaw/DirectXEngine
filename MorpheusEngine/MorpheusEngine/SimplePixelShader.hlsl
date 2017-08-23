struct PixelShaderInput
{
	float4 color : COLOR;
};

float4 main(PixelShaderInput InData) : SV_TARGET
{
	return InData.color;
}