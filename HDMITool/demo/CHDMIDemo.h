// MyDialog.h
#pragma once
#include "HDMIOutputDLL.h"  // 包含DLL头文件
#include <resource.h>

class CHDMIDemo : public CDialogEx
{
	DECLARE_DYNAMIC(CHDMIDemo)

public:
	CHDMIDemo(RenderConfig& config, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CHDMIDemo();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HDMI_DIALOG
	};
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	bool InitializeRenderer();
	virtual void OnDestroy();
	virtual void OnPaint();
	virtual void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

private:
	IHDMIOutputRenderer* m_pRenderer;      // 渲染器指针
	TextureHandle m_texture;               // 纹理句柄
	bool m_bInitialized;                   // 初始化标志

	void DestroyRenderer_();

	// 定时器用于连续渲染
	void OnTimer(UINT_PTR nIDEvent);


	RenderConfig m_config;
public:
	void UpdateTexture(CString imgpath);
};