#include "pch.h"
#include "MainDlg.h"

// ============================================================================
// CRenderWnd - 目标显示器全屏渲染窗口
// ============================================================================

BEGIN_MESSAGE_MAP(CRenderWnd, CWnd)
    ON_WM_PAINT()
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

CRenderWnd::~CRenderWnd()
{
    Cleanup();
}

bool CRenderWnd::CreateOnDisplay(const DisplayInfo& display)
{
    m_display = display;

    CString clsName;
    clsName.Format(_T("HDMIRenderWnd_%d"), (int)(ULONG_PTR)this);

    WNDCLASS wc = {};
    wc.lpfnWndProc = AfxWndProc;
    wc.hInstance = AfxGetInstanceHandle();
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = clsName;
    AfxRegisterClass(&wc);

    CRect rc(0, 0, display.width, display.height);
    DWORD style = WS_POPUP;

    if (!CreateEx(0, clsName, _T("HDMIRender"), style,
                  0, 0,
                  display.width, display.height,
                  nullptr, nullptr))
        return false;

    return m_bReady;
}

int CRenderWnd::OnCreate(LPCREATESTRUCT cs)
{
    if (CWnd::OnCreate(cs) == -1)
        return -1;

    RenderConfig config;
    config.fullscreen = true;
    config.displayinfo = m_display;

    m_pRenderer = CreateRenderer();
    if (!m_pRenderer)
    {
        m_lastError = _T("CreateRenderer 返回空");
        return -1;
    }

    if (!m_pRenderer->Initialize(config, GetSafeHwnd()))
    {
        m_lastError = m_pRenderer->GetLastError();
        DestroyRenderer(m_pRenderer);
        m_pRenderer = nullptr;
        return -1;
    }

    ShowWindow(SW_SHOW);
    m_bReady = true;
    return 0;
}

void CRenderWnd::OnDestroy()
{
    Cleanup();
    CWnd::OnDestroy();
}

void CRenderWnd::OnPaint()
{
    CPaintDC dc(this);
    if (m_pRenderer)
        m_pRenderer->Render();
}

bool CRenderWnd::ShowImage(LPCTSTR path)
{
    if (!m_bReady || !m_pRenderer)
        return false;

    m_pRenderer->ReleaseTexture();
    m_texture = m_pRenderer->LoadTexture(path);
    if (!m_texture)
    {
        m_lastError = m_pRenderer->GetLastError();
        return false;
    }

    m_pRenderer->SetTexture(m_texture);
    m_pRenderer->Render();
    return true;
}

void CRenderWnd::Cleanup()
{
    if (m_pRenderer)
    {
        m_pRenderer->Cleanup();
        DestroyRenderer(m_pRenderer);
        m_pRenderer = nullptr;
    }
    m_texture = nullptr;
    m_bReady = false;
}

// ============================================================================
// CMainDlg - 主对话框
// ============================================================================

IMPLEMENT_DYNAMIC(CMainDlg, CDialog)

CMainDlg::CMainDlg(CWnd* pParent /*=nullptr*/)
    : CDialog(IDD_HDMICLIENT_DIALOG, pParent)
    , m_bDisplayReady(false)
    , m_nSelectedDisplay(0)
{
}

CMainDlg::~CMainDlg()
{
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DISPLAY_LIST, m_lbDisplays);
    DDX_Control(pDX, IDC_IP_ADDRESS, m_editIP);
    DDX_Control(pDX, IDC_PORT, m_editPort);
    DDX_Control(pDX, IDC_BTN_CONNECT, m_btnConnect);
    DDX_Control(pDX, IDC_BTN_REFRESH, m_btnRefresh);
    DDX_Control(pDX, IDC_STATUS, m_stStatus);
    DDX_Control(pDX, IDC_LOG_EDIT, m_editLog);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialog)
    ON_BN_CLICKED(IDC_BTN_REFRESH, &CMainDlg::OnBtnRefresh)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CMainDlg::OnBtnConnect)
    ON_MESSAGE(WM_NET_RECEIVE, &CMainDlg::OnNetReceive)
    ON_MESSAGE(WM_NET_STATUS, &CMainDlg::OnNetStatus)
    ON_WM_DESTROY()
END_MESSAGE_MAP()

CString CMainDlg::GetExeDir()
{
    TCHAR path[MAX_PATH];
    GetModuleFileName(nullptr, path, MAX_PATH);
    CString full(path);
    return full.Left(full.ReverseFind('\\') + 1);
}

void CMainDlg::InitImageMapping()
{
    CString imgDir = GetExeDir() + _T("img\\");

    m_cmdToImage["A1"] = imgDir + _T("blue.jpg");
    m_cmdToImage["A2"] = imgDir + _T("green.jpg");
    m_cmdToImage["A3"] = imgDir + _T("red.jpg");
    m_cmdToImage["A4"] = imgDir + _T("test.jpg");

    m_cmdToReply["A1"] = _T("B1");
    m_cmdToReply["A2"] = _T("B2");
    m_cmdToReply["A3"] = _T("B3");
    m_cmdToReply["A4"] = _T("B4");
}

BOOL CMainDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    InitImageMapping();

    m_editIP.SetWindowText(_T("127.0.0.1"));
    m_editPort.SetWindowText(_T("7920"));

    m_editLog.SetLimitText(0);
    m_editLog.SetReadOnly(TRUE);

    RefreshDisplayList();

    SetStatusText(_T("就绪 - 请选择显示器并连接服务器"));

    return TRUE;
}

void CMainDlg::OnDestroy()
{
    m_client.Disconnect();

    if (m_bDisplayReady)
    {
        m_renderWnd.DestroyWindow();
        m_bDisplayReady = false;
    }

    CDialog::OnDestroy();
}

// ---- 显示器列表 ------------------------------------------------------------------

void CMainDlg::RefreshDisplayList()
{
    m_lbDisplays.ResetContent();
    m_displays.clear();

    m_displays = IHDMIOutputRenderer::GetDisplayInfo();

    for (size_t i = 0; i < m_displays.size(); i++)
    {
        CString name;
        name.Format(_T("[%d] %s  %dx%d"),
                    (int)i,
                    m_displays[i].deviceName,
                    m_displays[i].width,
                    m_displays[i].height);
        m_lbDisplays.AddString(name);
    }

    if (m_displays.size() > 0)
        m_lbDisplays.SetCurSel(0);

    CString refreshMsg;
    refreshMsg.Format(_T("显示器列表已刷新，共 %d 个显示器"), (int)m_displays.size());
    AppendLog(refreshMsg);
}

// ---- 连接 ------------------------------------------------------------------------

void CMainDlg::OnBtnConnect()
{
    if (m_client.IsConnected())
    {
        m_client.Disconnect();
        SetStatusText(_T("已断开连接"));
        return;
    }

    int sel = m_lbDisplays.GetCurSel();
    if (sel < 0 || sel >= (int)m_displays.size())
    {
        AfxMessageBox(_T("请先选择一个显示器"), MB_ICONWARNING);
        return;
    }
    m_nSelectedDisplay = sel;

    CString ip, portStr;
    m_editIP.GetWindowText(ip);
    m_editPort.GetWindowText(portStr);

    UINT port = _ttoi(portStr);
    if (ip.IsEmpty() || port == 0)
    {
        AfxMessageBox(_T("请输入有效的 IP 地址和端口"), MB_ICONWARNING);
        return;
    }

    if (!m_client.Connect(ip, port, GetSafeHwnd()))
    {
        SetStatusText(_T("连接失败"));
        return;
    }

    SetStatusText(_T("已连接 - ") + ip + _T(":") + portStr);

    CString log;
    log.Format(_T("正在连接 %s:%d ..."), ip.GetString(), port);
    AppendLog(log);
}

// ---- 网络消息 --------------------------------------------------------------------

LRESULT CMainDlg::OnNetReceive(WPARAM wParam, LPARAM lParam)
{
    int len = (int)wParam;
    char* pData = (char*)lParam;

    if (pData && len > 0)
    {
        std::string cmd(pData, len);

        // 去除末尾换行
        while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r'))
            cmd.pop_back();

        CString log;
        log.Format(_T("[服务器] %S"), cmd.c_str());
        AppendLog(log);

        ProcessCommand(cmd);

        delete[] pData;
    }

    return 0;
}

LRESULT CMainDlg::OnNetStatus(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case NET_CONNECTED:
    {
        CString ip; m_editIP.GetWindowText(ip);
        CString port; m_editPort.GetWindowText(port);
        SetStatusText(_T("已连接 - ") + ip + _T(":") + port);
        m_btnConnect.SetWindowText(_T("断开"));
        break;
    }
    case NET_DISCONNECTED:
        SetStatusText(_T("已断开连接"));
        m_btnConnect.SetWindowText(_T("连接"));
        break;
    case NET_ERROR:
        if (lParam)
        {
            CString* p = (CString*)lParam;
            SetStatusText(_T("错误: ") + *p);
            delete p;
        }
        m_btnConnect.SetWindowText(_T("连接"));
        break;
    }
    return 0;
}

// ---- 命令处理 --------------------------------------------------------------------

void CMainDlg::ProcessCommand(const std::string& cmd)
{
    CString reply;
    bool ok = false;

    // 先校验命令是否有效
    auto imgIt = m_cmdToImage.find(cmd);
    if (imgIt != m_cmdToImage.end())
    {
        // 有效命令，渲染窗口不存在则(重新)创建
        if (!m_bDisplayReady || !m_renderWnd.GetSafeHwnd())
        {
            if (m_bDisplayReady)
            {
                m_bDisplayReady = false;
                AppendLog(_T("渲染窗口已释放，正在重新创建..."));
            }

            if (m_nSelectedDisplay >= (int)m_displays.size())
            {
                AppendLog(_T("错误: 未选择有效显示器"));
            }
            else if (m_renderWnd.CreateOnDisplay(m_displays[m_nSelectedDisplay]))
            {
                m_bDisplayReady = true;
                AppendLog(_T("渲染窗口已创建"));
            }
            else
            {
                CString err = _T("错误: 创建渲染窗口失败 - ") + m_renderWnd.GetLastError();
                AppendLog(err);
            }
        }

        if (m_bDisplayReady)
        {
            CString path = imgIt->second;
            if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
            {
                CString err;
                err.Format(_T("错误: 图片文件不存在 - %s"), path.GetString());
                AppendLog(err);
            }
            else
            {
                ok = m_renderWnd.ShowImage(path);
                if (!ok)
                {
                    CString err;
                    err.Format(_T("错误: 加载图片失败 - %s (%s)"), path.GetString(), m_renderWnd.GetLastError().GetString());
                    AppendLog(err);
                }
            }
        }
        else
        {
            AppendLog(_T("错误: 渲染窗口未就绪"));
        }
    }

    // 构造回复
    auto replyIt = m_cmdToReply.find(cmd);
    if (replyIt != m_cmdToReply.end())
    {
        reply = replyIt->second;
    }
    else
    {
        reply.Format(_T("UNKNOWN:%S"), cmd.c_str());
    }

    // 推图成功才回复 B1-B4，失败回复错误码
    if (m_client.IsConnected())
    {
        CString log;
        if (!ok)
            reply.Format(_T("ERR:%S"), cmd.c_str());

        CStringA replyA(reply);
        replyA += "\r\n";
        m_client.SendData(replyA.GetString(), replyA.GetLength());

        log.Format(_T("[客户端] %s  (%s)"), reply.GetString(), ok ? _T("OK") : _T("FAIL"));
        AppendLog(log);
    }
}

// ---- 辅助方法 --------------------------------------------------------------------

void CMainDlg::AppendLog(CString text)
{
    CTime t = CTime::GetCurrentTime();
    CString line;
    line.Format(_T("%02d:%02d:%02d %s"),
                t.GetHour(), t.GetMinute(), t.GetSecond(), text.GetString());

    int nLen = m_editLog.GetWindowTextLength();
    m_editLog.SetSel(nLen, nLen);
    m_editLog.ReplaceSel(line + _T("\r\n"));
}

void CMainDlg::SetStatusText(CString text)
{
    m_stStatus.SetWindowText(text);
}

// ---- 按钮 ------------------------------------------------------------------------

void CMainDlg::OnBtnRefresh()
{
    RefreshDisplayList();
}
