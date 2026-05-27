// MyDialog.cpp
#include "pch.h"
#include "HDMIOutputDLL.h"
#include "CHDMIDemo.h"


// CHDMIDemo 对话框
IMPLEMENT_DYNAMIC(CHDMIDemo, CDialogEx)

CHDMIDemo::CHDMIDemo(RenderConfig& config, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HDMI_DIALOG, pParent)
	, m_pRenderer(nullptr)
	, m_texture(nullptr)
	, m_bInitialized(false)
	, m_config(config)
{
	//memcpy(&m_config, &config, sizeof(RenderConfig));
}

CHDMIDemo::~CHDMIDemo()
{
	DestroyRenderer_();
}

void CHDMIDemo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHDMIDemo, CDialogEx)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CHDMIDemo 消息处理程序
BOOL CHDMIDemo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 调整对话框大小
	SetWindowPos(nullptr, 0, 0, m_config.displayinfo.width, m_config.displayinfo.height, 0);

	// 初始化渲染器
	if (!InitializeRenderer())
	{
		AfxMessageBox(_T("初始化渲染器失败！"), MB_ICONERROR);
		EndDialog(IDCANCEL);
		return FALSE;
	}

	m_pRenderer->Render();
	// 启动定时器进行连续渲染
	//SetTimer(1, 1000, nullptr); // 约60FPS

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 初始化渲染器
bool CHDMIDemo::InitializeRenderer()
{
	// 创建渲染器实例
	m_pRenderer = CreateRenderer();
	if (!m_pRenderer)
	{
		AfxMessageBox(_T("创建渲染器失败！"), MB_ICONERROR);
		return false;
	}

	// 使用MFC对话框的窗口句柄
	HWND hWnd = GetSafeHwnd();

	// 初始化渲染器
	if (!m_pRenderer->Initialize(m_config, hWnd))
	{
		CString errorMsg;
		errorMsg.Format(_T("渲染器初始化失败：%s"), m_pRenderer->GetLastError());
		AfxMessageBox(errorMsg, MB_ICONERROR);
		DestroyRenderer_();
		return false;
	}

	// 加载纹理
	CString imagePath = _T("test.jpg");  // 图片路径
	m_texture = m_pRenderer->LoadTexture(imagePath.GetString());
	if (m_texture)
	{
		m_pRenderer->SetTexture(m_texture);
	}
	else
	{
		AfxMessageBox(_T("加载纹理失败，将显示默认颜色"), MB_ICONWARNING);
	}

	m_bInitialized = true;
	return true;
}

// 销毁渲染器
void CHDMIDemo::DestroyRenderer_()
{
	if (m_pRenderer)
	{
		m_pRenderer->Cleanup();
		DestroyRenderer(m_pRenderer);
		m_pRenderer = nullptr;
	}
	m_texture = nullptr;
	m_bInitialized = false;
}

void CHDMIDemo::OnDestroy()
{
	// 停止定时器
	KillTimer(1);

	// 销毁渲染器
	DestroyRenderer_();

	CDialogEx::OnDestroy();
}

void CHDMIDemo::OnPaint()
{
	CPaintDC dc(this); // 用于绘制的设备上下文

	if (!m_bInitialized)
	{
		// 如果未初始化，绘制提示信息
		CRect rect;
		GetClientRect(&rect);
		dc.FillSolidRect(&rect, RGB(40, 40, 40));

		CString text = _T("正在初始化渲染器...");
		dc.SetTextColor(RGB(255, 255, 255));
		dc.SetBkMode(TRANSPARENT);
		dc.DrawText(text, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else
	{
		/*int i = 0;
		i = i + 1;*/
		// 渲染器已初始化，交给DirectX渲染
		// 这里不需要做任何事，因为我们在OnTimer中渲染
	}
}

void CHDMIDemo::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (m_pRenderer && m_bInitialized && cx > 0 && cy > 0)
	{
		// 调整渲染器大小
		m_pRenderer->Resize(cx, cy);
	}
}

void CHDMIDemo::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1 && m_pRenderer && m_bInitialized)
	{
		static bool bcut = false;
		if (bcut)
		{
			m_pRenderer->ReleaseTexture();
			CString imagePath = _T("test.jpg");  // 图片路径
			m_texture = m_pRenderer->LoadTexture(imagePath.GetString());
			if (m_texture)
			{
				m_pRenderer->SetTexture(m_texture);
			}
			bcut = false;
		}
		else
		{
			m_pRenderer->ReleaseTexture();
			CString imagePath = _T("img20.jpg");  // 图片路径
			m_texture = m_pRenderer->LoadTexture(imagePath.GetString());
			if (m_texture)
			{
				m_pRenderer->SetTexture(m_texture);
			}
			bcut = true;
		}
		// 渲染一帧
		m_pRenderer->Render();
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CHDMIDemo::UpdateTexture(CString imgpath)
{
	static int i = 0;
	CString img;
	switch (i) {
	case 0:
		img = _T("red.jpg");
		break;
	case 1:
		img = _T("green.jpg");
		break;
	case 2:
		img = _T("blue.jpg");
		i = -1;
		break;
	}
	i++;
	if (m_pRenderer)
	{
		m_pRenderer->ReleaseTexture(); 
		m_texture = m_pRenderer->LoadTexture(img.GetString());
		if (m_texture)
		{
			if (!m_pRenderer->SetTexture(m_texture)) {
				m_bInitialized = false;
				AfxMessageBox(_T("加载纹理失败，将显示默认颜色"), MB_ICONWARNING);
			}
		}
	}
	m_pRenderer->Render();
}
