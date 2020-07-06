struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	uint shadingRate : SV_SHADINGRATE;
};

float4 PSMain(PSInput input) : SV_TARGET
{
	float4 color = float4(sin(input.position.x / 10.0f), sin(input.position.y / 10.0f), cos((input.position.x + input.position.y) / 10.0f), 1);

	return color;
}
