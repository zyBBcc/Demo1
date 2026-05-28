#include "pch.h"
#include "NetworkClient.h"

CNetworkClient::CNetworkClient()
    : m_socket(INVALID_SOCKET)
    , m_bConnected(false)
    , m_bStop(false)
    , m_hNotifyWnd(nullptr)
    , m_hThread(nullptr)
{
}

CNetworkClient::~CNetworkClient()
{
    Disconnect();
}

bool CNetworkClient::Connect(LPCTSTR ip, UINT port, HWND hNotifyWnd)
{
    if (m_bConnected)
        Disconnect();

    m_hNotifyWnd = hNotifyWnd;

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET)
    {
        OnError(_T("创建Socket失败"), WSAGetLastError());
        return false;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);

    CStringA ipA(ip);
    addr.sin_addr.s_addr = inet_addr(ipA.GetString());
    if (addr.sin_addr.s_addr == INADDR_NONE)
    {
        hostent* host = gethostbyname(ipA.GetString());
        if (!host)
        {
            OnError(_T("IP地址解析失败"), WSAGetLastError());
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
        addr.sin_addr.s_addr = *(ULONG*)host->h_addr_list[0];
    }

    if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        OnError(_T("连接服务器失败"), WSAGetLastError());
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    m_bConnected = true;
    m_bStop = false;

    ::PostMessage(m_hNotifyWnd, WM_NET_STATUS, NET_CONNECTED, 0);

    m_hThread = (HANDLE)_beginthreadex(nullptr, 0, RecvThread, this, 0, nullptr);
    return true;
}

void CNetworkClient::Disconnect()
{
    m_bStop = true;

	if (m_hThread)
	{
		WaitForSingleObject(m_hThread, 3000);
		CloseHandle(m_hThread);
		m_hThread = nullptr;
	}

    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    m_bConnected = false;
    ::PostMessage(m_hNotifyWnd, WM_NET_STATUS, NET_DISCONNECTED, 0);
}

bool CNetworkClient::SendData(const char* data, int len)
{
    if (!m_bConnected || m_socket == INVALID_SOCKET)
    {
        m_lastError = _T("未连接");
        return false;
    }

    int sent = send(m_socket, data, len, 0);
    if (sent == SOCKET_ERROR)
    {
        OnError(_T("send错误"), WSAGetLastError());
        return false;
    }
    return sent == len;
}

UINT WINAPI CNetworkClient::RecvThread(LPVOID pParam)
{
    CNetworkClient* pThis = (CNetworkClient*)pParam;
    char buf[4096];

    while (!pThis->m_bStop)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(pThis->m_socket, &fds);

        timeval tv = { 0, 500000 };
        int ret = select(0, &fds, nullptr, nullptr, &tv);

        if (ret == SOCKET_ERROR || ret == 0)
        {
            if (ret == SOCKET_ERROR)
            {
                pThis->OnError(_T("select错误"), WSAGetLastError());
                break;
            }
            continue;
        }

        int n = recv(pThis->m_socket, buf, sizeof(buf) - 1, 0);
        if (n <= 0)
        {
            if (n == SOCKET_ERROR)
                pThis->OnError(_T("recv错误"), WSAGetLastError());
            break;
        }

        buf[n] = '\0';
        char* pData = new char[n + 1];
        memcpy(pData, buf, n + 1);
        ::PostMessage(pThis->m_hNotifyWnd, WM_NET_RECEIVE, (WPARAM)n, (LPARAM)pData);
    }

    pThis->m_bConnected = false;
    ::PostMessage(pThis->m_hNotifyWnd, WM_NET_STATUS, NET_DISCONNECTED, 0);
    return 0;
}

void CNetworkClient::OnError(CString msg, int sockErr)
{
    if (sockErr != 0)
        m_lastError.Format(_T("%s (Winsock: %d)"), msg.GetString(), sockErr);
    else
        m_lastError = msg;

    if (m_hNotifyWnd)
        ::PostMessage(m_hNotifyWnd, WM_NET_STATUS, NET_ERROR, (LPARAM)new CString(m_lastError));
}
