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
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;

	result.position = position;
	result.color = color;

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{ 
	float4 color = float4(sin(input.position.x / 10.0f), sin(input.position.y / 10.0f), cos((input.position.x + input.position.y) / 10.0f), 1);

	return color;
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
	result.position = float4(0, 0, 0, 0);
	result.color = float4(0, 0, 0, 0);
	return result;
}

[domain("tri")]
PSInput DSMain(OutputPatch<PSInput, 3> patch, PatchConstants patchConst, float3 location : SV_DomainLocation)
{
	PSInput result;
	result.position = float4(0, 0, 0, 0);
	result.color = float4(0, 0, 0, 0);
	return result;
}