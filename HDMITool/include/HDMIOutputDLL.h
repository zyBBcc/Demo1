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

// 顶点结构
struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT2 texcoord;
};

// 纹理信息
struct TextureInfo
{
	ID3D11ShaderResourceView* srv = nullptr;
	int width = 0;
	int height = 0;
};

#define MAX_NAME_LENGTH 128

struct DisplayInfo
{
	wchar_t deviceName[MAX_NAME_LENGTH];
	wchar_t friendlyName[MAX_NAME_LENGTH];
	int adapterIndex = -1;             // 适配器索引
	int outputIndex = -1;              // 输出索引
	int width = 0;                     // 当前宽度
	int height = 0;                    // 当前高度
	int refreshRate = 0;               // 刷新率
	bool isPrimary = false;            // 是否为主显示器
};


// 渲染配置结构
struct RenderConfig
{
	bool fullscreen = false;
	DisplayInfo displayinfo;
};


// 纹理句柄
typedef void* TextureHandle;

// DLL主类接口
class IHDMIOutputRenderer
{
public:
	virtual ~IHDMIOutputRenderer() {}

	// 初始化渲染器（如果hWnd不为nullptr则使用外部窗口，否则创建新窗口）
	virtual bool Initialize(const RenderConfig& config, HWND hWnd = nullptr) = 0;

	// 加载纹理
	virtual TextureHandle LoadTexture(const wchar_t* filename) = 0;
	// 设置当前纹理
	virtual bool SetTexture(TextureHandle texture) = 0;
	// 渲染一帧
	virtual bool Render() = 0;
	// 获取错误信息
	virtual const wchar_t* GetLastError() const = 0;
	// 清理资源
	virtual void Cleanup() = 0;
	virtual bool ReleaseTexture() = 0;
	// 重新设置分辨率
	virtual bool Resize(int width, int height) = 0;
	// ====== 新增接口实现 ======
	HDMI_OUTPUT_API static std::vector<DisplayInfo> GetDisplayInfo();
	HDMI_OUTPUT_API static bool FindDisplayByName(const wchar_t* displayName, DisplayInfo& displayinfo);
	static int GetDisplayRefreshRate(const wchar_t* deviceName);

	//virtual HWND CreateWindow_(HINSTANCE hInstance, int x, int y) = 0;

};

// C风格导出函数
extern "C" {
	HDMI_OUTPUT_API IHDMIOutputRenderer* CreateRenderer();
	HDMI_OUTPUT_API void DestroyRenderer(IHDMIOutputRenderer* renderer);
	HDMI_OUTPUT_API const wchar_t* GetRendererVersion();
}



// 渲染器实现类
class HDMIOutputRendererImpl : public IHDMIOutputRenderer
{
public:
	// 窗口类名
	static const wchar_t* WINDOW_CLASS_NAME;
private:
	// Direct3D资源
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

	// 当前纹理
	TextureInfo m_currentTexture;

	// 初始化标志
	bool m_bInitialized = false;
	// 窗口和配置
	HWND m_hwnd = nullptr;
	bool m_ownWindow = false;  // 标记是否是自己创建的窗口
	HINSTANCE m_hInstance = nullptr;
	RenderConfig m_config;
	std::wstring m_lastError;

	// 内部方法
	bool InitD3D();
	bool CreateShaders();
	bool CreateGeometry();
	bool CreateSampler();

	HRESULT LoadTextureInternal(const wchar_t* filename, TextureInfo& texture);
	/*HRESULT LoadTextureFromMemoryInternal(const void* data, size_t size, TextureInfo& texture);*/

public:
	HDMIOutputRendererImpl();
	virtual ~HDMIOutputRendererImpl();

	// 接口实现
	bool Initialize(const RenderConfig& config, HWND hWnd = nullptr) override;
	TextureHandle LoadTexture(const wchar_t* filename) override;
	bool SetTexture(TextureHandle texture) override;
	bool ReleaseTexture();
	bool Render() override;
	const wchar_t* GetLastError() const override;
	void Cleanup() override;

	bool Resize(int width, int height);
	/*HWND CreateWindow_(HINSTANCE hInstance, int x, int y) override;
	// 创建自己的窗口
	HWND CreateOwnWindow();*/
	
};