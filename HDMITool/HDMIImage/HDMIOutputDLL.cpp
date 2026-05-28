// HDMIOutputDLL.cpp - DLL implementation
#include "pch.h"
#include "HDMIOutputDLL.h"
#include <algorithm>

const wchar_t* HDMIOutputRendererImpl::WINDOW_CLASS_NAME = L"HDMIOutputDLL_WindowClass";

// Factory functions
HDMI_OUTPUT_API IHDMIOutputRenderer* CreateRenderer()
{
	return new HDMIOutputRendererImpl();
}

HDMI_OUTPUT_API void DestroyRenderer(IHDMIOutputRenderer* renderer)
{
	if (renderer)
	{
		renderer->Cleanup();
		delete renderer;
	}
}

HDMI_OUTPUT_API const wchar_t* GetRendererVersion()
{
	return L"0.0.1";
}

std::vector<DisplayInfo> IHDMIOutputRenderer::GetDisplayInfo()
{
	std::vector<DisplayInfo> displays;

	// Create DXGI factory
	IDXGIFactory* factory = nullptr;
	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&factory));

	if (FAILED(hr))
	{
		return displays;
	}

	// Enumerate adapters
	IDXGIAdapter* adapter = nullptr;
	for (int adapterIndex = 0;
		factory->EnumAdapters(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
		++adapterIndex)
	{
		// Enumerate outputs
		IDXGIOutput* output = nullptr;
		for (int outputIndex = 0;
			adapter->EnumOutputs(outputIndex, &output) != DXGI_ERROR_NOT_FOUND;
			++outputIndex)
		{
			DisplayInfo info;
			info.adapterIndex = adapterIndex;
			info.outputIndex = outputIndex;

			// Get output info
			DXGI_OUTPUT_DESC outputDesc;
			if (SUCCEEDED(output->GetDesc(&outputDesc)))
			{
				wcscpy_s(info.deviceName, _countof(info.deviceName), outputDesc.DeviceName);
				info.left = outputDesc.DesktopCoordinates.left;
				info.top = outputDesc.DesktopCoordinates.top;
				info.width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
				info.height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
				info.isPrimary = (outputDesc.AttachedToDesktop &&
					outputDesc.DesktopCoordinates.left == 0 &&
					outputDesc.DesktopCoordinates.top == 0);

				// Get friendly name via monitor info
				HMONITOR hMonitor = outputDesc.Monitor;
				MONITORINFOEX monitorInfo;
				monitorInfo.cbSize = sizeof(MONITORINFOEX);

				if (GetMonitorInfo(hMonitor, &monitorInfo))
				{
					wcscpy_s(info.friendlyName, _countof(info.friendlyName), monitorInfo.szDevice);
				}

				// Use helper to get refresh rate
				info.refreshRate = GetDisplayRefreshRate(info.deviceName);

				displays.push_back(info);
			}

			output->Release();
		}

		adapter->Release();
	}

	factory->Release();
	return displays;
}

bool IHDMIOutputRenderer::FindDisplayByName(const wchar_t* displayName, DisplayInfo& displayinfo)
{
	DisplayInfo notFound;
	notFound.adapterIndex = -1;
	notFound.outputIndex = -1;

	if (displayName == nullptr || displayName[0] == L'\0')
	{
		return false;
	}

	// Get all displays
	std::vector<DisplayInfo> displays = GetDisplayInfo();

	// Convert search name to lowercase for case-insensitive comparison
	std::wstring searchName(displayName);
	std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::towlower);

	for (const auto& display : displays)
	{
		// Convert device name to lowercase
		std::wstring deviceNameLower = display.deviceName;
		std::transform(deviceNameLower.begin(), deviceNameLower.end(), deviceNameLower.begin(), ::towlower);

		// Convert friendly name to lowercase
		std::wstring friendlyNameLower = display.friendlyName;
		std::transform(friendlyNameLower.begin(), friendlyNameLower.end(), friendlyNameLower.begin(), ::towlower);

		// Check for match
		if (deviceNameLower.find(searchName) != std::wstring::npos ||
			friendlyNameLower.find(searchName) != std::wstring::npos)
		{
			displayinfo = display;
			return TRUE;
		}
	}

	return FALSE;
}

// Helper function to get display refresh rate
int IHDMIOutputRenderer::GetDisplayRefreshRate(const wchar_t* deviceName)
{
	if (deviceName == nullptr || deviceName[0] == L'\0')
		return 0;

	int refreshRate = 0;
	DEVMODE devMode;
	ZeroMemory(&devMode, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);

	// Method 1: Get current display settings
	if (EnumDisplaySettings(deviceName, ENUM_CURRENT_SETTINGS, &devMode))
	{
		if (devMode.dmFields & DM_DISPLAYFREQUENCY)
		{
			refreshRate = devMode.dmDisplayFrequency;
		}
	}

	// Method 2: If current settings failed, enumerate modes to find max refresh rate
	if (refreshRate == 0)
	{
		DWORD modeNum = 0;
		int width = 0, height = 0;

		// First get current resolution
		DEVMODE currentMode;
		if (EnumDisplaySettings(deviceName, ENUM_CURRENT_SETTINGS, &currentMode))
		{
			width = currentMode.dmPelsWidth;
			height = currentMode.dmPelsHeight;
		}

		// Find matching resolution and highest refresh rate
		while (EnumDisplaySettings(deviceName, modeNum, &devMode))
		{
			if (devMode.dmFields & DM_DISPLAYFREQUENCY &&
				devMode.dmFields & DM_PELSWIDTH &&
				devMode.dmFields & DM_PELSHEIGHT)
			{
				// If width/height specified, only match that resolution
				if (width > 0 && height > 0)
				{
					if (devMode.dmPelsWidth == width &&
						devMode.dmPelsHeight == height &&
						devMode.dmDisplayFrequency > refreshRate)
					{
						refreshRate = devMode.dmDisplayFrequency;
					}
				}
				else
				{
					// Otherwise pick highest refresh rate
					if (devMode.dmDisplayFrequency > refreshRate)
					{
						refreshRate = devMode.dmDisplayFrequency;
					}
				}
			}
			modeNum++;
		}
	}

	return refreshRate;
}

// Constructor
HDMIOutputRendererImpl::HDMIOutputRendererImpl()
{
}

// Destructor
HDMIOutputRendererImpl::~HDMIOutputRendererImpl()
{
	Cleanup();
}

// Register window class (if needed)
bool RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = HDMIOutputRendererImpl::WINDOW_CLASS_NAME;

	return RegisterClassEx(&wc) != 0;
}



// Initialize
bool HDMIOutputRendererImpl::Initialize(const RenderConfig& config, HWND hWnd)
{
	if (m_bInitialized)
	{
		Cleanup();
	}

	if (hWnd == nullptr)
	{
		m_lastError = L"window handle null " + m_lastError;
		return false;
	}

	m_config = config;

	m_hwnd = hWnd;
	m_ownWindow = false;

	// Check if window handle is valid
	if (!IsWindow(m_hwnd))
	{
		m_lastError = L"Invalid window handle provided " + m_lastError;
		return false;
	}

	// Initialize Direct3D
	if (!InitD3D())
	{
		m_lastError = L"Failed to initialize Direct3D " + m_lastError;
		return false;
	}

	// Create shaders
	if (!CreateShaders())
	{
		m_lastError = L"Failed to create shaders " + m_lastError;
		return false;
	}

	// Create geometry
	if (!CreateGeometry())
	{
		m_lastError = L"Failed to create geometry " + m_lastError;
		return false;
	}

	// Create sampler
	if (!CreateSampler())
	{
		m_lastError = L"Failed to create sampler " + m_lastError;
		return false;
	}

	m_bInitialized = true;
	return true;
}

bool HDMIOutputRendererImpl::ReleaseTexture()
{
	if (m_currentTexture.srv)
	{
		m_currentTexture.srv->Release();
		m_currentTexture.srv = nullptr;
	}
	return true;
}

// Initialize Direct3D
bool HDMIOutputRendererImpl::InitD3D()
{
	HRESULT hr = S_OK;

	// Create DXGI factory
	IDXGIFactory* factory = nullptr;
	hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(hr))
	{
		m_lastError = L"Failed to create DXGI factory";
		return false;
	}

	// Target HDMI display
	std::pair<int, int> displayIndices = { 0, 0 };
	displayIndices.first = m_config.displayinfo.adapterIndex;
	displayIndices.second = m_config.displayinfo.outputIndex;

	// Get specified adapter
	IDXGIAdapter* adapter = nullptr;
	hr = factory->EnumAdapters(displayIndices.first, &adapter);
	if (FAILED(hr))
	{
		m_lastError = L"enum adapters fail";
		return false;
	}

	// Feature levels to try
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	// Configure swap chain
	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferCount = 2;
	scd.BufferDesc.Width = m_config.displayinfo.width;
	scd.BufferDesc.Height = m_config.displayinfo.height;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = m_config.displayinfo.refreshRate;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = m_hwnd;
	scd.SampleDesc.Count = 1;
	scd.Windowed = true;
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scd.Flags = 0;

	// Create D3D11 device and swap chain
	UINT createFlags = 0;
#ifdef _DEBUG
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	hr = D3D11CreateDeviceAndSwapChain(
		adapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		createFlags,
		featureLevels,
		_countof(featureLevels),
		D3D11_SDK_VERSION,
		&scd,
		&m_swapChain,
		&m_device,
		nullptr,
		&m_context);

	if (FAILED(hr))
	{
		m_lastError = L"Failed to create D3D11 device and swap chain";
		factory->Release();
		if (adapter) adapter->Release();
		return false;
	}

	// Fullscreen: use Flip Model + DXGI fullscreen optimization (no exclusive fullscreen needed)
	if (m_config.fullscreen)
	{
		IDXGIOutput* output = nullptr;
		if (SUCCEEDED(adapter->EnumOutputs(displayIndices.second, &output)))
		{
			DXGI_OUTPUT_DESC desc{};
			output->GetDesc(&desc);
			UINT width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
			UINT height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

			hr = m_swapChain->ResizeBuffers(0, width, height,
				DXGI_FORMAT_R8G8B8A8_UNORM, 0);
			output->Release();
			if (FAILED(hr))
			{
				_com_error err(hr);
				m_lastError = err.ErrorMessage();
				factory->Release();
				if (adapter) adapter->Release();
				return false;
			}
		}
	}

	ID3D11Texture2D* backBuffer = nullptr;
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	if (SUCCEEDED(hr))
	{
		hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
		backBuffer->Release();
	}

	if (FAILED(hr))
	{
		m_lastError = L"Failed to create render target view";
		factory->Release();
		if (adapter) adapter->Release();
		return false;
	}

	// Set render target
	m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

	// Set viewport
	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(m_config.displayinfo.width);
	viewport.Height = static_cast<float>(m_config.displayinfo.height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	m_context->RSSetViewports(1, &viewport);

	// Create rasterizer state
	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	rd.DepthClipEnable = TRUE;
	hr = m_device->CreateRasterizerState(&rd, &m_rasterizerState);
	if (FAILED(hr))
	{
		m_lastError = L"create rasterizer state fail";
		return false;
	}

	// Cleanup
	factory->Release();
	if (adapter) adapter->Release();

	return true;
}

// Create shaders (same as before, consolidated for brevity)
bool HDMIOutputRendererImpl::CreateShaders()
{
	// Vertex shader source
	const char* vsCode = R"(
        struct VS_INPUT {
            float3 pos : POSITION;
            float2 tex : TEXCOORD;
        };
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD;
        };
        PS_INPUT main(VS_INPUT input) {
            PS_INPUT output;
            output.pos = float4(input.pos, 1.0);
            output.tex = input.tex;
            return output;
        }
    )";

	// Pixel shader source
	const char* psCode = R"(
        Texture2D texture0 : register(t0);
        SamplerState sampler0 : register(s0);
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD;
        };
        float4 main(PS_INPUT input) : SV_TARGET {
            return texture0.Sample(sampler0, input.tex);
        }
    )";

	// Compile vertex shader
	ID3DBlob* vsBlob = nullptr;
	HRESULT hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr,
		"main", "vs_5_0", 0, 0, &vsBlob, nullptr);
	if (FAILED(hr))
	{
		m_lastError = L"Failed to compile vertex shader";
		return false;
	}

	hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		nullptr, &m_vertexShader);

	// Create input layout
	if (SUCCEEDED(hr))
	{
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
			  D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		hr = m_device->CreateInputLayout(layout, 2,
			vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(),
			&m_inputLayout);
	}
	vsBlob->Release();

	if (FAILED(hr))
	{
		m_lastError = L"Failed to create vertex shader or input layout";
		return false;
	}

	// Compile pixel shader
	ID3DBlob* psBlob = nullptr;
	hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr,
		"main", "ps_5_0", 0, 0, &psBlob, nullptr);
	if (FAILED(hr))
	{
		m_lastError = L"Failed to compile pixel shader";
		return false;
	}

	hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(),
		psBlob->GetBufferSize(),
		nullptr, &m_pixelShader);
	psBlob->Release();

	if (FAILED(hr))
	{
		m_lastError = L"Failed to create pixel shader";
		return false;
	}

	return true;
}

// Create geometry
bool HDMIOutputRendererImpl::CreateGeometry()
{
	// Full-screen quad vertices
	Vertex vertices[] = {
		{ XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
	};

	WORD indices[] = { 0, 1, 2, 2, 1, 3 };

	// Create vertex buffer
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(vertices);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vinit = {};
	vinit.pSysMem = vertices;

	HRESULT hr = m_device->CreateBuffer(&vbd, &vinit, &m_vertexBuffer);
	if (FAILED(hr))
	{
		m_lastError = L"Failed to create vertex buffer";
		return false;
	}

	// Create index buffer
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(indices);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA iinit = {};
	iinit.pSysMem = indices;

	hr = m_device->CreateBuffer(&ibd, &iinit, &m_indexBuffer);
	if (FAILED(hr))
	{
		m_lastError = L"Failed to create index buffer";
		return false;
	}

	return true;
}

// Create sampler state
bool HDMIOutputRendererImpl::CreateSampler()
{
	D3D11_SAMPLER_DESC desc = {};
	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	desc.MinLOD = 0;
	desc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = m_device->CreateSamplerState(&desc, &m_samplerState);
	if (FAILED(hr))
	{
		m_lastError = L"Failed to create sampler state";
		return false;
	}

	return true;
}

// Load texture from file
TextureHandle HDMIOutputRendererImpl::LoadTexture(const wchar_t* filename)
{
	std::unique_ptr<TextureInfo> texture(new TextureInfo());

	HRESULT hr = LoadTextureInternal(filename, *texture);
	if (FAILED(hr))
	{
		m_lastError = L"Failed to load texture from file: ";
		m_lastError += filename;
		return nullptr;
	}

	return texture.release();
}


// Internal texture loading implementation using WIC
HRESULT HDMIOutputRendererImpl::LoadTextureInternal(const wchar_t* filename, TextureInfo& texture)
{
	// Create WIC factory
	IWICImagingFactory* wicFactory = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));

	IWICBitmapDecoder* decoder = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = wicFactory->CreateDecoderFromFilename(filename, nullptr,
			GENERIC_READ,
			WICDecodeMetadataCacheOnLoad,
			&decoder);
	}

	IWICBitmapFrameDecode* frame = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = decoder->GetFrame(0, &frame);
	}

	IWICFormatConverter* converter = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = wicFactory->CreateFormatConverter(&converter);
	}

	if (SUCCEEDED(hr))
	{
		hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone, nullptr,
			0.0f, WICBitmapPaletteTypeCustom);
	}

	if (SUCCEEDED(hr))
	{
		UINT width, height;
		converter->GetSize(&width, &height);
		texture.width = width;
		texture.height = height;

		std::vector<BYTE> buffer(width * height * 4);
		converter->CopyPixels(nullptr, width * 4,
			static_cast<UINT>(buffer.size()),
			buffer.data());

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = buffer.data();
		initData.SysMemPitch = width * 4;

		ID3D11Texture2D* d3dTexture = nullptr;
		hr = m_device->CreateTexture2D(&texDesc, &initData, &d3dTexture);

		if (SUCCEEDED(hr))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = texDesc.Format;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;

			hr = m_device->CreateShaderResourceView(d3dTexture, &srvDesc,
				&texture.srv);
			d3dTexture->Release();
		}
	}

	// Cleanup
	if (converter) converter->Release();
	if (frame) frame->Release();
	if (decoder) decoder->Release();
	if (wicFactory) wicFactory->Release();

	return hr;
}



// Set current texture
bool HDMIOutputRendererImpl::SetTexture(TextureHandle handle)
{
	if (handle == nullptr)
	{
		m_lastError = L"Invalid texture handle";
		return false;
	}

	TextureInfo* texture = static_cast<TextureInfo*>(handle);
	m_currentTexture = *texture;
	return true;
}

// Render
bool HDMIOutputRendererImpl::Render()
{
	if (!m_device || !m_context) {
		m_lastError = L"device or context is null";
		return false;
	}

	m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

	// Clear
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_context->ClearRenderTargetView(m_renderTargetView, clearColor);

	// Set pipeline state
	m_context->VSSetShader(m_vertexShader, nullptr, 0);
	m_context->PSSetShader(m_pixelShader, nullptr, 0);
	m_context->IASetInputLayout(m_inputLayout);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set vertex/index buffers
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);

	// Set texture and sampler
	if (m_currentTexture.srv)
	{
		m_context->PSSetShaderResources(0, 1, &m_currentTexture.srv);
		m_context->PSSetSamplers(0, 1, &m_samplerState);
	}

	// Set rasterizer state
	m_context->RSSetState(m_rasterizerState);

	// Draw
	m_context->DrawIndexed(6, 0, 0);


	// Present
	HRESULT hr = S_OK;
	hr = m_swapChain->Present(1, 0);
	if (FAILED(hr))
	{
		if (hr == DXGI_ERROR_INVALID_CALL ||
			hr == DXGI_ERROR_DEVICE_RESET ||
			hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			_com_error err(hr);

			m_lastError = L"Present failed (recovered but still failed): ";
			m_lastError += err.ErrorMessage();
		}
		else
		{
			_com_error err(hr);
			m_lastError = L"Present failed: ";
			m_lastError += err.ErrorMessage();
		}

		return false;
	}

	return true;
}



// Get last error message
const wchar_t* HDMIOutputRendererImpl::GetLastError() const
{
	return m_lastError.c_str();
}

// Cleanup
void HDMIOutputRendererImpl::Cleanup()
{
	if (!m_bInitialized)
	{
		return;
	}

	// Release texture
	if (m_currentTexture.srv)
	{
		m_currentTexture.srv->Release();
		m_currentTexture.srv = nullptr;
	}

	// Release Direct3D resources
	if (m_rasterizerState)
	{
		m_rasterizerState->Release();
		m_rasterizerState = nullptr;
	}

	if (m_samplerState)
	{
		m_samplerState->Release();
		m_samplerState = nullptr;
	}

	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}

	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}

	if (m_inputLayout)
	{
		m_inputLayout->Release();
		m_inputLayout = nullptr;
	}

	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}

	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}

	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = nullptr;
	}

	if (m_swapChain)
	{
		// Exit fullscreen mode if active
		BOOL isFullscreen = FALSE;
		m_swapChain->GetFullscreenState(&isFullscreen, nullptr);

		if (isFullscreen)
		{
			m_swapChain->SetFullscreenState(FALSE, nullptr);

			// For Flip Model, call ResizeBuffers after exiting fullscreen
			DXGI_SWAP_CHAIN_DESC desc;
			m_swapChain->GetDesc(&desc);

			m_swapChain->ResizeBuffers(
				desc.BufferCount,
				m_config.displayinfo.width,
				m_config.displayinfo.height,
				desc.BufferDesc.Format,
				desc.Flags);
		}
		m_swapChain->Release();
		m_swapChain = nullptr;
	}

	if (m_context)
	{
		m_context->Release();
		m_context = nullptr;
	}

	if (m_device)
	{
		m_device->Release();
		m_device = nullptr;
	}

	// Destroy our own window if we created it
	if (m_hwnd && m_ownWindow)
	{
		DestroyWindow(m_hwnd);
		m_hwnd = nullptr;
	}

	// Reset state
	m_bInitialized = false;
}

// Resize swap chain
bool HDMIOutputRendererImpl::Resize(int width, int height)
{
	if (!m_swapChain || !m_device)
		return false;

	// Release render target view
	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = nullptr;
	}

	m_context->OMSetRenderTargets(0, 0, 0);

	// Resize swap chain buffers
	HRESULT hr = m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr))
	{
		m_lastError = L"Failed to resize swap chain buffers";
		return false;
	}

	// Recreate render target view
	ID3D11Texture2D* backBuffer = nullptr;
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(void**)&backBuffer);
	if (SUCCEEDED(hr))
	{
		hr = m_device->CreateRenderTargetView(backBuffer, nullptr,
			&m_renderTargetView);
		backBuffer->Release();
	}

	if (FAILED(hr))
	{
		m_lastError = L"Failed to recreate render target view";
		return false;
	}

	m_context->OMSetRenderTargets(1, &m_renderTargetView, NULL);

	// Set viewport
	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	m_context->RSSetViewports(1, &viewport);

	// Update config
	m_config.displayinfo.width = width;
	m_config.displayinfo.height = height;

	Render();

	return true;
}
