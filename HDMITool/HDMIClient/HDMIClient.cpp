#include "pch.h"
#include "HDMIClient.h"
#include "MainDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CHDMIClientApp, CWinApp)
END_MESSAGE_MAP()

CHDMIClientApp::CHDMIClientApp()
{
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

CHDMIClientApp theApp;

BOOL CHDMIClientApp::InitInstance()
{
    // 初始化 Winsock
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    InitCommonControls();
    AfxSocketInit();

    CWinApp::InitInstance();

    SetRegistryKey(_T("HDMIClient"));

    CMainDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    // 清理
    WSACleanup();

    return FALSE;
}
