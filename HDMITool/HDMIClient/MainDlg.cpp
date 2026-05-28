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
                  display.left, display.top,
                  display.width, display.height,
                  nullptr, nullptr))
        return false;

    ShowWindow(SW_SHOW);
    UpdateWindow();
    return true;
}

bool CRenderWnd::InitRenderer()
{
    RenderConfig config;
    config.fullscreen = true;
    config.displayinfo = m_display;
    //调试
   /* config.displayinfo.width = 480;
    config.displayinfo.height = 480;*/

    m_pRenderer = CreateRenderer();
    if (!m_pRenderer)
    {
        m_lastError = _T("CreateRenderer 返回空");
        return false;
    }

    if (!m_pRenderer->Initialize(config, GetSafeHwnd()))
    {
        m_lastError = m_pRenderer->GetLastError();
        DestroyRenderer(m_pRenderer);
        m_pRenderer = nullptr;
        return false;
    }

    m_bReady = true;
    return true;
}

int CRenderWnd::OnCreate(LPCREATESTRUCT cs)
{
    if (CWnd::OnCreate(cs) == -1)
        return -1;

    
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
#ifdef LOG_TO_FILE
    , m_fLog(nullptr)
#endif
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
    DDX_Control(pDX, IDC_PS_TYPE, m_cbPsType);
    DDX_Control(pDX, IDC_PS_ADDRESS, m_editPsAddr);
    DDX_Control(pDX, IDC_PS_CHANNEL, m_editPsChan);
    DDX_Control(pDX, IDC_PS_VOLTAGE, m_editPsVoltage);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialog)
    ON_BN_CLICKED(IDC_BTN_REFRESH, &CMainDlg::OnBtnRefresh)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CMainDlg::OnBtnConnect)
    ON_MESSAGE(WM_NET_RECEIVE, &CMainDlg::OnNetReceive)
    ON_MESSAGE(WM_NET_STATUS, &CMainDlg::OnNetStatus)
    ON_WM_DESTROY()
    ON_WM_CTLCOLOR()
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
    m_cmdToReply["A0"] = _T("B0");
    m_cmdToReply["A5"] = _T("B5");
    m_cmdToReply["A6"] = _T("B6");
}

BOOL CMainDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

#ifdef LOG_TO_FILE
    // 创建日志目录并打开日志文件
    CString logDir = GetExeDir() + _T("log\\");
    CreateDirectory(logDir, nullptr);

    CTime t = CTime::GetCurrentTime();
    CString logPath;
    logPath.Format(_T("%s%04d%02d%02d_%02d%02d%02d.log"),
                   logDir.GetString(),
                   t.GetYear(), t.GetMonth(), t.GetDay(),
                   t.GetHour(), t.GetMinute(), t.GetSecond());
    m_fLog = _tfsopen(logPath, _T("a, ccs=UTF-8"), _SH_DENYNO);
#endif

    InitImageMapping();

    m_editIP.SetWindowText(_T("127.0.0.1"));
    m_editPort.SetWindowText(_T("7920"));

    m_editLog.SetLimitText(0);
    m_editLog.SetReadOnly(TRUE);

    m_cbPsType.AddString(_T("66319"));
    m_cbPsType.AddString(_T("6700"));
    m_cbPsType.SetCurSel(0);
    m_editPsAddr.SetWindowText(_T("GPIB0::5::INSTR"));
    m_editPsChan.SetWindowText(_T("1"));
    m_editPsVoltage.SetWindowText(_T("3.8"));

    m_fontStatus.CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, _T("Microsoft YaHei"));
    m_stStatus.SetFont(&m_fontStatus);
    m_statusColor = RGB(0, 0, 0);

    RefreshDisplayList();

    SetStatusText(_T("就绪 - 请选择显示器并连接服务器"));

    return TRUE;
}

void CMainDlg::OnDestroy()
{
    m_client.Disconnect();
    m_psMgr.Disconnect();

    if (m_bDisplayReady)
    {
        m_renderWnd.DestroyWindow();
        m_bDisplayReady = false;
    }

#ifdef LOG_TO_FILE
    if (m_fLog)
    {
        fclose(m_fLog);
        m_fLog = nullptr;
    }
#endif

    CDialog::OnDestroy();
}

// ---- 显示器列表 ------------------------------------------------------------------

void CMainDlg::RefreshDisplayList()
{
    m_lbDisplays.ResetContent();
    m_displays.clear();

    m_displays = IHDMIOutputRenderer::GetDisplayInfo();

    CString log;
    log.Format(_T("检测到 %d 个显示器:"), (int)m_displays.size());
    AppendLog(log);

    for (size_t i = 0; i < m_displays.size(); i++)
    {
        CString name;
        name.Format(_T("[%d] %s  %dx%d"),
                    (int)i,
                    m_displays[i].deviceName,
                    m_displays[i].width,
                    m_displays[i].height);
        m_lbDisplays.AddString(name);

        CString log2;
        log2.Format(_T("  [%d] %s  %dx%d %dHz %s"),
                    (int)i,
                    m_displays[i].deviceName,
                    m_displays[i].width,
                    m_displays[i].height,
                    m_displays[i].refreshRate,
                    m_displays[i].isPrimary ? _T("(主)") : _T(""));
        AppendLog(log2);
    }

    if (m_displays.size() > 0)
        m_lbDisplays.SetCurSel(0);
}

// ---- 连接 ------------------------------------------------------------------------

void CMainDlg::OnBtnConnect()
{
    if (m_client.IsConnected())
    {
        m_client.Disconnect();
        SetStatusText(_T("已断开连接"));
        m_statusColor = RGB(0, 0, 0);
        m_stStatus.Invalidate();

        m_lbDisplays.EnableWindow(TRUE);
        m_editIP.EnableWindow(TRUE);
        m_editPort.EnableWindow(TRUE);
        m_btnRefresh.EnableWindow(TRUE);
        m_cbPsType.EnableWindow(TRUE);
        m_editPsAddr.EnableWindow(TRUE);
        m_editPsChan.EnableWindow(TRUE);
        m_editPsVoltage.EnableWindow(TRUE);

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
        m_statusColor = RGB(220, 0, 0);
        m_stStatus.Invalidate();
        return;
    }

    SetStatusText(_T("已连接 - ") + ip + _T(":") + portStr);

    m_lbDisplays.EnableWindow(FALSE);
    m_editIP.EnableWindow(FALSE);
    m_editPort.EnableWindow(FALSE);
    m_btnRefresh.EnableWindow(FALSE);
    m_cbPsType.EnableWindow(FALSE);
    m_editPsAddr.EnableWindow(FALSE);
    m_editPsChan.EnableWindow(FALSE);
    m_editPsVoltage.EnableWindow(FALSE);

    CString log;
    CString displayName = m_displays[m_nSelectedDisplay].deviceName;
    log.Format(_T("正在连接 %s:%d 显示器=%s ..."), ip.GetString(), port, displayName.GetString());
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
        m_statusColor = RGB(0, 180, 0);
        m_stStatus.Invalidate();
        break;
    }
    case NET_DISCONNECTED:
        SetStatusText(_T("已断开连接"));
        m_btnConnect.SetWindowText(_T("连接"));
        m_statusColor = RGB(0, 0, 0);
        m_stStatus.Invalidate();
        m_lbDisplays.EnableWindow(TRUE);
        m_editIP.EnableWindow(TRUE);
        m_editPort.EnableWindow(TRUE);
        m_btnRefresh.EnableWindow(TRUE);
        m_cbPsType.EnableWindow(TRUE);
        m_editPsAddr.EnableWindow(TRUE);
        m_editPsChan.EnableWindow(TRUE);
        m_editPsVoltage.EnableWindow(TRUE);
        break;
    case NET_ERROR:
        if (lParam)
        {
            CString* p = (CString*)lParam;
            SetStatusText(_T("错误: ") + *p);
            CString log;
            log.Format(_T("网络错误: %s"), p->GetString());
            AppendLog(log);
            delete p;
        }
        m_btnConnect.SetWindowText(_T("连接"));
        m_statusColor = RGB(220, 0, 0);
        m_stStatus.Invalidate();
        m_lbDisplays.EnableWindow(TRUE);
        m_editIP.EnableWindow(TRUE);
        m_editPort.EnableWindow(TRUE);
        m_btnRefresh.EnableWindow(TRUE);
        m_cbPsType.EnableWindow(TRUE);
        m_editPsAddr.EnableWindow(TRUE);
        m_editPsChan.EnableWindow(TRUE);
        m_editPsVoltage.EnableWindow(TRUE);
        break;
    }
    return 0;
}

// ---- 命令处理 --------------------------------------------------------------------

void CMainDlg::ProcessCommand(const std::string& cmd)
{
    CString reply;
    bool ok = false;

    // 电源命令: A0/A5/A6 (自包含回复逻辑)
    if (cmd == "A0" || cmd == "A5" || cmd == "A6")
    {
        ProcessPowerCmd(cmd);
        return;
    }

    // 图片命令: A1-A4
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
                if (m_renderWnd.InitRenderer())
                {
                    m_bDisplayReady = true;
                    AppendLog(_T("渲染窗口已创建"));
                }
                else
                {
                    CString err = _T("错误: 初始化渲染器失败 - ") + m_renderWnd.GetLastError();
                    AppendLog(err);
                    m_renderWnd.DestroyWindow();
                }
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

// ---- 电源命令 --------------------------------------------------------------------

void CMainDlg::ProcessPowerCmd(const std::string& cmd)
{
    if (cmd == "A0")
    {
        if (m_psMgr.IsConnected())
        {
            AppendLog(_T("电源已连接，先断开再重连"));
            m_psMgr.Disconnect();
        }

        CString typeStr, addr, chanStr, voltStr;
        m_cbPsType.GetWindowText(typeStr);
        m_editPsAddr.GetWindowText(addr);
        m_editPsChan.GetWindowText(chanStr);
        m_editPsVoltage.GetWindowText(voltStr);

        InstrumentType type = (typeStr == _T("6700")) ? POWER_6700 : POWER_66319;
        int chan = _ttoi(chanStr);
        double voltage = _ttof(voltStr);
        CStringA addrA(addr);

        if (m_psMgr.Connect(type, addrA.GetString(), chan, voltage))
        {
            CString log;
            log.Format(_T("[电源] 连接成功 型号=%s 地址=%s 通道=%d 电压=%.2fV"),
                       typeStr.GetString(), addr.GetString(), chan, voltage);
            AppendLog(log);

            if (m_client.IsConnected())
                m_client.SendData("B0\r\n", 4);
        }
        else
        {
            CString log;
            log.Format(_T("[电源] 连接失败: %S"), m_psMgr.GetLastError().GetString());
            AppendLog(log);
            if (m_client.IsConnected())
                m_client.SendData("ERR:A0\r\n", 8);
        }
    }
    else if (cmd == "A5")
    {
        if (!m_psMgr.IsConnected())
        {
            AppendLog(_T("[电源] 未连接，无法读取电流"));
            if (m_client.IsConnected())
                m_client.SendData("ERR:A5\r\n", 8);
            return;
        }

        // 读取 10 次，去最值取平均，间隔 500ms
        double values[10];
        for (int i = 0; i < 10; i++)
        {
            if (i > 0)
                Sleep(500);

            values[i] = m_psMgr.ReadCurrent();
            if (values[i] <= CURR_ERRA)
            {
                CString log;
                log.Format(_T("[电源] 第%d次读取失败: %S"), i + 1, m_psMgr.GetLastError().GetString());
                AppendLog(log);
                if (m_client.IsConnected())
                    m_client.SendData("ERR:A5\r\n", 8);
                return;
            }

            CString log;
            log.Format(_T("[电源] 第%d次: %.6f A"), i + 1, values[i]);
            AppendLog(log);
        }

        // 去最大值和最小值
        double sum = 0.0, vmin = values[0], vmax = values[0];
        for (int i = 0; i < 10; i++)
        {
            sum += values[i];
            if (values[i] < vmin) vmin = values[i];
            if (values[i] > vmax) vmax = values[i];
        }
        double avg = (sum - vmin - vmax) / 8.0;

        CString log;
        log.Format(_T("[电源] 电流: %.6f A"), avg);
        AppendLog(log);

        if (m_client.IsConnected())
        {
            CStringA reply;
            reply.Format("B5=%.6f\r\n", avg);
            m_client.SendData(reply.GetString(), reply.GetLength());
        }
    }
    else if (cmd == "A6")
    {
        m_psMgr.Disconnect();
        CStringA err = m_psMgr.GetLastError();
        if (!err.IsEmpty())
        {
            CString log;
            log.Format(_T("[电源] 断开时出错: %S"), err.GetString());
            AppendLog(log);
        }
        AppendLog(_T("[电源] 已断开"));

        if (m_client.IsConnected())
            m_client.SendData("B6\r\n", 4);
    }
}

// ============================================================================
// CPowerSupplyManager
// ============================================================================

bool CPowerSupplyManager::Connect(InstrumentType type, const char* addr, int chan, double voltage)
{
    Disconnect();

    m_type = type;
    m_addr = addr;
    m_chan = chan;

    char err[256] = {};
    m_pPower = Tinno::PWS_Create(type, addr, err);
    if (!m_pPower)
    {
        m_lastError = err;
        return false;
    }

    if (!m_pPower->Initial(chan, err))
    {
        m_lastError = err;
        Disconnect();
        return false;
    }

    if (!m_pPower->SetVoltage(chan, voltage, err))
    {
        m_lastError = err;
        Disconnect();
        return false;
    }

    if (!m_pPower->SetCurrentRange(chan, err))
    {
        m_lastError = err;
        Disconnect();
        return false;
    }

    if (!m_pPower->SetOutput(chan, true, err))
    {
        m_lastError = err;
        Disconnect();
        return false;
    }

    m_bConnected = true;
    return true;
}

double CPowerSupplyManager::ReadCurrent()
{
    if (!m_bConnected || !m_pPower)
    {
        m_lastError = "未连接";
        return CURR_ERRA;
    }

    char err[256] = {};
    double cur = m_pPower->GetCurrentA(m_chan, err);
    if (cur <= CURR_ERRA)
        m_lastError = err;
    return cur;
}

void CPowerSupplyManager::Disconnect()
{
    if (m_pPower)
    {
        char err[256] = {};
        if (m_bConnected)
        {
            if (!m_pPower->SetOutput(m_chan, false, err))
                m_lastError = err;
        }
        if (!Tinno::PWS_Close(m_pPower, err))
            m_lastError = err;
        m_pPower = nullptr;
    }
    m_bConnected = false;
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

#ifdef LOG_TO_FILE
    if (m_fLog)
    {
        _ftprintf_s(m_fLog, _T("%s\r\n"), line.GetString());
        fflush(m_fLog);
    }
#endif
}

HBRUSH CMainDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

    if (pWnd->GetDlgCtrlID() == IDC_STATUS)
    {
        pDC->SetTextColor(m_statusColor);
        pDC->SetBkMode(TRANSPARENT);
    }

    return hbr;
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
