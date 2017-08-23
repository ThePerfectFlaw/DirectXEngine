struct PixelShaderInput
{
	float4 color : COLOR;
};

float4 SimplePixelShader(PixelShaderInput InData) : SV_TARGET
{
	return InData.color;
}