:: Please run this from a Developer Command Prompt for VS 2019.

:: Compile shaders using 6_4 profile
dxc /Zi -Od -Tvs_6_4 -EVSMain -FhVSMain.h shaders.hlsl
dxc /Zi -Od -Tps_6_4 -EPSMain -FhPSMain.h shaders.hlsl

:: Compile shaders to use if no VRS Tier 2 support is found
dxc /Zi -Od -Tvs_5_1 -EVSFallback -FhVSFallback.h fallback.hlsl
dxc /Zi -Od -Tps_5_1 -EPSFallback -FhPSFallback.h fallback.hlsl

