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
	result.shadingRate = 0;


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
		break;
	}
	return col;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return PickShadingRateColor(input.shadingRate);
}

struct PatchConstants
{
	float TessFactor[3] : SV_TessFactor;
	float InsideTess : SV_InsideTessFactor;
};

PatchConstants hs_patch(InputPatch<PSInput, 3> patch)
{
	PatchConstants ret = { {0, 0, 0}, 0 };
	return ret;
}

[patchconstantfunc("hs_patch")]
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[maxtessfactor(64.0)]
[outputcontrolpoints(3)]
PSInput HSMain(InputPatch<PSInput, 3> patch, uint cpid : SV_OutputControlPointID)
{
	PSInput result;
	result.position = patch[0].position;
	result.color = float4(0, 0, 0, 0);
	result.shadingRate = patch[0].shadingRate;

	return result;
}

[domain("tri")]
PSInput DSMain(OutputPatch<PSInput, 3> patch, PatchConstants patchConst, float3 location : SV_DomainLocation)
{
	PSInput result;
	result.position = patch[0].position;
	result.color = float4(0, 0, 0, 0);
	result.shadingRate = patch[0].shadingRate;

	return result;
}