struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	uint shadingRate : SV_SHADINGRATE;
};

float4 PickShadingRateColor(uint shadingRate)
{
	float4 col = float4(0, 0, 0, 0);
	switch (shadingRate)
	{
	case 0x0: // fine shading
		col = float4(1, 0, 0, 1); // Red
		break;
	case 0x1: // 1X2
		col = float4(1, 0.3f, 0.3f, 1); // Light red
		break;
	case 0x4: // 2X1
		col = float4(0.5f, 0, 0, 1); // Dark red
		break;
	case 0x5: // 2x2
		col = float4(0, 0, 1, 1); // Blue
		break;
	case 0x6: // 2x4
		col = float4(0.3f, 1, 1, 1); // Light cyan
		break;
	case 0x9: // 4x2
		col = float4(0, 0.5f, 0.5f, 1); // Dark cyan
		break;
	case 0xA: // 4x4
		col = float4(0, 1, 0, 1); // Green
		break;
	default:
		col = float4(1, 0, 1, 1); // Magenta
	}

	return col;
}


float4 PSMain(PSInput input) : SV_TARGET
{
	float4 color = float4(sin(input.position.x / 10.0f), sin(input.position.y / 10.0f), cos((input.position.x + input.position.y) / 10.0f), 1);

	return color;
}
