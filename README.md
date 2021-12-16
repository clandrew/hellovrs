# VRS Samples
This repo contains some simple, standalone test applications that exercise Direct3D12 Variable Rate Shading.


## hellovrs_simple
A triangle straightforwardly shaded. The shading rate is specified through [RSSetShadingRate](https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist5-rssetshadingrate) API. Tier 1 compatible.

Controls: Use the left and right arrow keys to select a shading rate, shown in the title bar. The default is 1x1.

![Example image](https://raw.githubusercontent.com/clandrew/hellovrs/master/Images/simple.PNG "Example image.")

This sample has both D3D12 and Vulkan versions.

## hellovrs_rates
A bunch of shaded quads, one for each shading rate. Their shading rate is specified through RSSetShadingRate API. Tier 1 compatible.

Controls: none

![Example image](https://raw.githubusercontent.com/clandrew/hellovrs/master/Images/rates.PNG "Example image.")

## hellovrs_arrowkeys_tier2
One triangle shaded while a screenspace image is also bound. There is combined use of the base shading rate through RSSetShadingRate, binding of the image through [RSSetShadingRateImage](https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist5-rssetshadingrateimage), and a per-primitive rate using SV_ShadingRate system value.

The base shading rate is used in two places: 
* as input to RSSetShadingRate
* and as a value in the screenspace image. 
More specifically the screenspace-image is fixed to have the base shading rate in the upper right, and fine shading (1x1) in the lower left.

Controls: Use the left and right arrow keys to select a shading rate.
Use the up and down arrow keys to select a combiner. There's three choices:
* AlwaysScreenspace (default)
* AlwaysPerPrimitive
* Min

![Example image](https://raw.githubusercontent.com/clandrew/hellovrs/master/Images/combine.PNG "Example image.")

## Build and usage
These samples were built using Visual Studio 2019, version 142 of the build tools. They run on x86-64 architecture. 
The shaders for hellovrs_arrowkeys_tier2 are built using the Developer Command Prompt for Visual Studio 2019.

The samples are set up to run against the default hardware adapter; use of VRS requires a compatible system.

The Vulkan sample requires the Vulkan SDK. You'll have to edit the project file and CompileShaders.bat to point it to your SDK.

## See also
The spec for Variable Rate Shading can be viewed [here](https://github.com/microsoft/DirectX-Specs/blob/master/d3d/VariableRateShading.md).
