// HDMIOutputDLL.h - DLL�ӿ�ͷ�ļ�
#pragma once
#define HDMI_OUTPUT_EXPORTS
#ifdef HDMI_OUTPUT_EXPORTS
#define HDMI_OUTPUT_API __declspec(dllexport)
#else
#define HDMI_OUTPUT_API __declspec(dllimport)
#endif

#include <windows.h>
#include <string>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wincodec.h>
#include <dxgi.h>
#include <vector>
#include <string>
#include <memory>
#include <comdef.h>
#include <sstream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "windowscodecs.lib")

using namespace DirectX;

// ����ṹ
struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT2 texcoord;
};

// ������Ϣ
struct TextureInfo
{
	ID3D11ShaderResourceView* srv = nullptr;
	int width = 0;
	int height = 0;
};

#define MAX_NAME_LENGTH 128

typedef struct DisplayInfo
{
	wchar_t deviceName[MAX_NAME_LENGTH];
	wchar_t friendlyName[MAX_NAME_LENGTH];
	int adapterIndex = -1;             // ����������
	int outputIndex = -1;              // �������
	int left = 0;                      // X coordinate in virtual desktop
	int top = 0;                       // Y coordinate in virtual desktop
	int width = 0;
	int height = 0;
	int refreshRate = 0;
	bool isPrimary = false;
};


// ��Ⱦ���ýṹ
struct RenderConfig
{
	bool fullscreen = false;
	DisplayInfo displayinfo;
};


// �������
typedef void* TextureHandle;

// DLL����ӿ�
class IHDMIOutputRenderer
{
public:
	virtual ~IHDMIOutputRenderer() {}

	// ��ʼ����Ⱦ�������hWnd��Ϊnullptr��ʹ���ⲿ���ڣ����򴴽��´��ڣ�
	virtual bool Initialize(const RenderConfig& config, HWND hWnd = nullptr) = 0;

	// ��������
	virtual TextureHandle LoadTexture(const wchar_t* filename) = 0;
	// ���õ�ǰ����
	virtual bool SetTexture(TextureHandle texture) = 0;
	// ��Ⱦһ֡
	virtual bool Render() = 0;
	// ��ȡ������Ϣ
	virtual const wchar_t* GetLastError() const = 0;
	// ������Դ
	virtual void Cleanup() = 0;
	virtual bool ReleaseTexture() = 0;
	// �������÷ֱ���
	virtual bool Resize(int width, int height) = 0;
	// ====== �����ӿ�ʵ�� ======
	HDMI_OUTPUT_API static std::vector<DisplayInfo> GetDisplayInfo();
	HDMI_OUTPUT_API static bool FindDisplayByName(const wchar_t* displayName, DisplayInfo& displayinfo);
	static int GetDisplayRefreshRate(const wchar_t* deviceName);

	//virtual HWND CreateWindow_(HINSTANCE hInstance, int x, int y) = 0;

};

// C��񵼳�����
extern "C" {
	HDMI_OUTPUT_API IHDMIOutputRenderer* CreateRenderer();
	HDMI_OUTPUT_API void DestroyRenderer(IHDMIOutputRenderer* renderer);
	HDMI_OUTPUT_API const wchar_t* GetRendererVersion();
}



// ��Ⱦ��ʵ����
class HDMIOutputRendererImpl : public IHDMIOutputRenderer
{
public:
	// ��������
	static const wchar_t* WINDOW_CLASS_NAME;
private:
	// Direct3D��Դ
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_context = nullptr;
	IDXGISwapChain* m_swapChain = nullptr;
	ID3D11RenderTargetView* m_renderTargetView = nullptr;
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11Buffer* m_indexBuffer = nullptr;
	ID3D11SamplerState* m_samplerState = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;

	// ��ǰ����
	TextureInfo m_currentTexture;

	// ��ʼ����־
	bool m_bInitialized = false;
	// ���ں�����
	HWND m_hwnd = nullptr;
	bool m_ownWindow = false;  // ����Ƿ����Լ������Ĵ���
	HINSTANCE m_hInstance = nullptr;
	RenderConfig m_config;
	std::wstring m_lastError;

	// �ڲ�����
	bool InitD3D();
	bool CreateShaders();
	bool CreateGeometry();
	bool CreateSampler();

	HRESULT LoadTextureInternal(const wchar_t* filename, TextureInfo& texture);
	/*HRESULT LoadTextureFromMemoryInternal(const void* data, size_t size, TextureInfo& texture);*/

public:
	HDMIOutputRendererImpl();
	virtual ~HDMIOutputRendererImpl();

	// �ӿ�ʵ��
	bool Initialize(const RenderConfig& config, HWND hWnd = nullptr) override;
	TextureHandle LoadTexture(const wchar_t* filename) override;
	bool SetTexture(TextureHandle texture) override;
	bool ReleaseTexture();
	bool Render() override;
	const wchar_t* GetLastError() const override;
	void Cleanup() override;

	bool Resize(int width, int height);
	/*HWND CreateWindow_(HINSTANCE hInstance, int x, int y) override;
	// �����Լ��Ĵ���
	HWND CreateOwnWindow();*/
	
};