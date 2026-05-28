#pragma once

// 日志保存到文件开关，注释掉即关闭
#define LOG_TO_FILE

#include "HDMIOutputDLL.h"
#include "NetworkClient.h"
#include "ExportPowerSupply.h"
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
    bool InitRenderer();
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

// 电源控制管理器
class CPowerSupplyManager
{
public:
    CPowerSupplyManager() : m_pPower(nullptr), m_bConnected(false), m_chan(0), m_type(POWER_66319) {}
    ~CPowerSupplyManager() { Disconnect(); }

    bool Connect(InstrumentType type, const char* addr, int chan, double voltage);
    double ReadCurrent();
    void Disconnect();
    bool IsConnected() const { return m_bConnected; }
    int GetChannel() const { return m_chan; }
    CStringA GetLastError() const { return m_lastError; }

private:
    Tinno::PowerSupplyExporter* m_pPower;
    bool m_bConnected;
    int m_chan;
    InstrumentType m_type;
    CStringA m_addr;
    CStringA m_lastError;
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
    CFont   m_fontStatus;
    COLORREF m_statusColor;
    CEdit   m_editLog;
    CComboBox m_cbPsType;
    CEdit   m_editPsAddr;
    CEdit   m_editPsChan;
    CEdit   m_editPsVoltage;

    // 网络
    CNetworkClient m_client;

    // 显示器
    std::vector<DisplayInfo> m_displays;

    // 渲染
    CRenderWnd   m_renderWnd;
    bool         m_bDisplayReady;
    int          m_nSelectedDisplay;

#ifdef LOG_TO_FILE
    // 日志
    FILE* m_fLog;
#endif

    // 电源
    CPowerSupplyManager m_psMgr;

    // 命令映射
    std::map<std::string, CString> m_cmdToImage;
    std::map<std::string, CString> m_cmdToReply;

    // 初始化
    CString GetExeDir();
    void InitImageMapping();
    void RefreshDisplayList();
    void AppendLog(CString text);
    void SetStatusText(CString text);
    void ProcessPowerCmd(const std::string& cmd);

    // 消息处理
    afx_msg void OnBtnRefresh();
    afx_msg void OnBtnConnect();
    afx_msg LRESULT OnNetReceive(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnNetStatus(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDestroy();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    void ProcessCommand(const std::string& cmd);
};
