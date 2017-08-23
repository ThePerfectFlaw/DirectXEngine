cbuffer PerApplication : register(b0)
{
	matrix projectionMatrix;
}

cbuffer PerFrame : register(b1)
{
	matrix viewMatrix;
}

cbuffer PerObject : register(b2)
{
	matrix worldMatrix;
}

struct AppData
{
	float3 position : POSITION;
	float3 color : COLOR;
};

struct VertexShaderOutput
{
	float4 color : COLOR;
	float4 position : SV_POSITION;
};

VertexShaderOutput SimpleVertexShader(AppData InData)
{
	VertexShaderOutput outData;

	matrix mvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
	outData.position = mul(mvp, float4(InData.position, 1.0f));
	outData.color = float4(InData.color, 1.0f);

	return outData;
}

//float4 main( float4 pos : POSITION ) : SV_POSITION
//{
//	return pos;
//}