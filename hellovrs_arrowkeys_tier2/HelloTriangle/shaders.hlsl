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

float4 PSMain(PSInput input) : SV_TARGET
{
	float4 color = float4(sin(input.position.x / 10.0f), sin(input.position.y / 10.0f), cos((input.position.x + input.position.y) / 10.0f), 1);

	return color;
}
