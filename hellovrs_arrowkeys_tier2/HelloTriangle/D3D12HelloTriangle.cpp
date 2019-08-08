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

#include "stdafx.h"
#include "D3D12HelloTriangle.h"
#include "VSMain.h"
#include "PSMain.h"
#include "VSFallback.h"
#include "PSFallback.h"
#include <vector>

D3D12HelloTriangle::D3D12HelloTriangle(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, name),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_rtvDescriptorSize(0),
	m_shadingRateIndex(0),
	m_combinerTypeIndex(0)
{
}

void D3D12HelloTriangle::OnInit()
{
	LoadPipeline();
	LoadAssets();

	if (m_platformSupportsTier2VRS)
		AllocateScreenspaceImage(256u, 256u);

	m_shadingRateIndex = 0;
	UpdateTitleText();
}

// Load the rendering pipeline dependencies.
void D3D12HelloTriangle::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;
	
	D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModels, NULL, NULL);

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
			));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			nullptr,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
			));
	}

	const D3D12_FEATURE D3D12_FEATURE_D3D12_OPTIONS6 = (D3D12_FEATURE)30;
	D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6;
	m_platformSupportsTier2VRS = false;
	if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6))))
	{
		m_platformSupportsTier2VRS = options6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		Win32Application::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
		));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

void D3D12HelloTriangle::AllocateScreenspaceImage(UINT width, UINT height)
{
	CD3DX12_RESOURCE_DESC screenspaceImageDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8_UINT,
		static_cast<UINT64>(width),
		height,
		1 /*arraySize*/,
		1 /*mipLevels*/);

	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&screenspaceImageDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&m_spScreenspaceImage)));

	UINT64 uploadSize;
	m_device->GetCopyableFootprints(&screenspaceImageDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadSize);

	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_spScreenspaceImageUpload)));
}

void D3D12HelloTriangle::SetScreenspaceImageData(ID3D12GraphicsCommandList5* cl, BYTE* data, size_t dataSize)
{
	// Map the screenspace image upload buffer and write the value
	UINT8* pMappedRange;
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(m_spScreenspaceImageUpload->Map(0, &readRange, reinterpret_cast<void**>(&pMappedRange)));
	memcpy(pMappedRange, data, dataSize);
	m_spScreenspaceImageUpload->Unmap(0, nullptr);

	// Copy from the upload buffer to the screenspace image
	CD3DX12_TEXTURE_COPY_LOCATION Src;
	Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	Src.pResource = m_spScreenspaceImageUpload.Get();
	auto desc = m_spScreenspaceImage->GetDesc();
	m_device->GetCopyableFootprints(&desc, 0, 1, 0, &Src.PlacedFootprint, nullptr, nullptr, nullptr);

	CD3DX12_TEXTURE_COPY_LOCATION Dst;
	Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	Dst.pResource = m_spScreenspaceImage.Get();
	Dst.SubresourceIndex = 0;

	D3D12_RESOURCE_STATES previousState = D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

	cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_spScreenspaceImage.Get(), previousState, D3D12_RESOURCE_STATE_COPY_DEST));
	cl->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
	cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_spScreenspaceImage.Get(), D3D12_RESOURCE_STATE_COPY_DEST, previousState));
}

void
CompileAndCheckErrors(_In_ LPCWSTR pFileName,
    _In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* pDefines,
    _In_opt_ ID3DInclude* pInclude,
    _In_ LPCSTR pEntrypoint,
    _In_ LPCSTR pTarget,
    _In_ UINT Flags1,
    _In_ UINT Flags2,
    _Out_ ID3DBlob** ppCode)
{
    ComPtr<ID3DBlob> errorMsgs;
    HRESULT hr = D3DCompileFromFile(pFileName, pDefines, pInclude, pEntrypoint, pTarget, Flags1, Flags2, ppCode, &errorMsgs);
    if (FAILED(hr))
    {
        char const* errorMsg = reinterpret_cast<char*>(errorMsgs->GetBufferPointer());
        MessageBoxA(nullptr, errorMsg, "Failed to compile shader", MB_OK);
        ThrowIfFailed(hr);
    }
}

// Load the sample assets.
void D3D12HelloTriangle::LoadAssets()
{
	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		if (m_platformSupportsTier2VRS)
		{
			psoDesc.VS = CD3DX12_SHADER_BYTECODE((void *)g_VSMain, ARRAYSIZE(g_VSMain));
			psoDesc.PS = CD3DX12_SHADER_BYTECODE((void *)g_PSMain, ARRAYSIZE(g_PSMain));
		}
		else
		{
			psoDesc.VS = CD3DX12_SHADER_BYTECODE((void *)g_VSFallback, ARRAYSIZE(g_VSFallback));
			psoDesc.PS = CD3DX12_SHADER_BYTECODE((void *)g_PSFallback, ARRAYSIZE(g_PSFallback));
		}
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	}

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

	m_commandList.As(&m_commandList5);

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(m_commandList->Close());

	// Create the vertex buffer.
	{
		// Define the geometry for a triangle.
		Vertex triangleVertices[] =
		{
			{ {-1, 1, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, 1, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
			{ { -1, -0.25f * m_aspectRatio, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		WaitForPreviousFrame();
	}
}

// Update frame-based values.
void D3D12HelloTriangle::OnUpdate()
{
}

// Render the scene.
void D3D12HelloTriangle::OnRender()
{
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void D3D12HelloTriangle::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

void D3D12HelloTriangle::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(m_commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	if (m_commandList5 && m_platformSupportsTier2VRS)
	{
		static const D3D12_SHADING_RATE shadingRates[] = {
			D3D12_SHADING_RATE_1X1, D3D12_SHADING_RATE_1X2,D3D12_SHADING_RATE_2X1, D3D12_SHADING_RATE_2X2, D3D12_SHADING_RATE_2X4, D3D12_SHADING_RATE_4X2, D3D12_SHADING_RATE_4X4 };

		D3D12_SHADING_RATE shadingRate = shadingRates[m_shadingRateIndex];

		std::vector<BYTE> screenspaceImageData;
		screenspaceImageData.resize(256 * 256);
		for (int y = 0; y < 256; ++y)
		{
			for (int x = 0; x < 256; ++x)
			{
				int index = (y * 256) + x;
				if (x > y)
				{
					screenspaceImageData[index] = static_cast<BYTE>(shadingRate);
				}
				else
				{
					screenspaceImageData[index] = static_cast<BYTE>(D3D12_SHADING_RATE_1X1);
				}
			}
		}
		SetScreenspaceImageData(m_commandList5.Get(), screenspaceImageData.data(), screenspaceImageData.size());

		if (m_combinerTypeIndex == 0)
		{
			D3D12_SHADING_RATE_COMBINER chooseScreenspaceImage[2] = { D3D12_SHADING_RATE_COMBINER_PASSTHROUGH, D3D12_SHADING_RATE_COMBINER_OVERRIDE }; // Choose screenspace image
			m_commandList5->RSSetShadingRate(shadingRate, chooseScreenspaceImage);
		}
		else if (m_combinerTypeIndex == 1)
		{
			D3D12_SHADING_RATE_COMBINER choosePerPrimitive[2] = { D3D12_SHADING_RATE_COMBINER_OVERRIDE, D3D12_SHADING_RATE_COMBINER_PASSTHROUGH }; // Choose per primitive
			m_commandList5->RSSetShadingRate(shadingRate, choosePerPrimitive);
		}
		else if (m_combinerTypeIndex == 2)
		{
			D3D12_SHADING_RATE_COMBINER chooseMin[2] = { D3D12_SHADING_RATE_COMBINER_MIN, D3D12_SHADING_RATE_COMBINER_MIN };
			m_commandList5->RSSetShadingRate(shadingRate, chooseMin);
		}

		m_commandList5->RSSetShadingRateImage(m_spScreenspaceImage.Get());
	}

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_commandList->Close());
}

void D3D12HelloTriangle::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void D3D12HelloTriangle::OnKeyUp(UINT8 key)
{
	if (!m_platformSupportsTier2VRS)
		return;

	if (key == 37 && m_shadingRateIndex > 0) // left
	{
		m_shadingRateIndex--;
	}
	if (key == 39 && m_shadingRateIndex < 7 - 1) // right
	{
		m_shadingRateIndex++;
	}
	else if (key == 38 && m_combinerTypeIndex < 2) // up
	{
		m_combinerTypeIndex++;
	}
	else if (key == 40 && m_combinerTypeIndex > 0) // down
	{
		m_combinerTypeIndex--;
	}

	UpdateTitleText();
}

void D3D12HelloTriangle::UpdateTitleText()
{
	std::wstringstream titleText;

	if (m_platformSupportsTier2VRS)
	{
		wchar_t const* shadingRateTitle = L"";
		switch (m_shadingRateIndex)
		{
		case 0: shadingRateTitle = L"1x1"; break;
		case 1: shadingRateTitle = L"1x2"; break;
		case 2: shadingRateTitle = L"2x1"; break;
		case 3: shadingRateTitle = L"2x2"; break;
		case 4: shadingRateTitle = L"2x4"; break;
		case 5: shadingRateTitle = L"4x2"; break;
		case 6: shadingRateTitle = L"4x4"; break;
		}

		wchar_t const* combinerTitle = L"";
		switch (m_combinerTypeIndex)
		{
		case 0: combinerTitle = L"AlwaysScreenspace"; break;
		case 1: combinerTitle = L"AlwaysPerPrimitive"; break;
		case 2: combinerTitle = L"Min"; break;
		}

		titleText << L"Screenspace image rate: " << shadingRateTitle << L"  Per-primitive rate: 4x4  Combiner:" << combinerTitle;
	}
	else
	{
		titleText << L"{Platform does not support VRS Tier 2}";
	}

	SetWindowText(Win32Application::GetHwnd(), titleText.str().c_str());

}