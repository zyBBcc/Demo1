#pragma once
#include "HDMIOutputDLL.h"
#include "NetworkClient.h"
#include "resource.h"
#include <map>
#include <vector>

// 渲染窗口: 在目标显示器上全屏显示图片
class CRenderWnd : public CWnd
{
public:
    CRenderWnd() : m_pRenderer(nullptr), m_texture(nullptr), m_bReady(false) {}
    ~CRenderWnd();

    bool CreateOnDisplay(const DisplayInfo& display);
    bool ShowImage(LPCTSTR path);
    CString GetLastError() const { return m_lastError; }
    void Cleanup();

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg int OnCreate(LPCREATESTRUCT cs);
    afx_msg void OnDestroy();

private:
    IHDMIOutputRenderer* m_pRenderer;
    TextureHandle m_texture;
    bool m_bReady;
    DisplayInfo m_display;
    CString m_lastError;
};

// 主对话框
class CMainDlg : public CDialog
{
    DECLARE_DYNAMIC(CMainDlg)

public:
    CMainDlg(CWnd* pParent = nullptr);
    virtual ~CMainDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_HDMICLIENT_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    // UI 控件
    CListBox m_lbDisplays;
    CEdit   m_editIP;
    CEdit   m_editPort;
    CButton m_btnConnect;
    CButton m_btnRefresh;
    CStatic m_stStatus;
    CEdit   m_editLog;

    // 网络
    CNetworkClient m_client;

    // 显示器
    std::vector<DisplayInfo> m_displays;

    // 渲染
    CRenderWnd   m_renderWnd;
    bool         m_bDisplayReady;
    int          m_nSelectedDisplay;

    // 命令映射
    std::map<std::string, CString> m_cmdToImage;
    std::map<std::string, CString> m_cmdToReply;

    // 初始化
    CString GetExeDir();
    void InitImageMapping();
    void RefreshDisplayList();
    void AppendLog(CString text);
    void SetStatusText(CString text);

    // 消息处理
    afx_msg void OnBtnRefresh();
    afx_msg void OnBtnConnect();
    afx_msg LRESULT OnNetReceive(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnNetStatus(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDestroy();

    void ProcessCommand(const std::string& cmd);
};
