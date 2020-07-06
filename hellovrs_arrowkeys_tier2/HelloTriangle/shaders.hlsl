//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	uint shadingRate : SV_SHADINGRATE;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;

	result.position = position;
	result.color = color;

	/*

typedef enum D3D12_SHADING_RATE
{
	D3D12_SHADING_RATE_1X1 = 0,
	D3D12_SHADING_RATE_1X2 = 0x1,
	D3D12_SHADING_RATE_2X1 = 0x4,
	D3D12_SHADING_RATE_2X2 = 0x5,
	D3D12_SHADING_RATE_2X4 = 0x6,
	D3D12_SHADING_RATE_4X2 = 0x9,
	D3D12_SHADING_RATE_4X4 = 0xa
} 	D3D12_SHADING_RATE;

	
	*/
	result.shadingRate = 0xa;

	return result;
}

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
