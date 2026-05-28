#pragma once

// 自定义消息：通知主窗口网络事件
#define WM_NET_RECEIVE     (WM_USER + 100)   // 收到数据，WPARAM=len, LPARAM=char* (需delete[])
#define WM_NET_STATUS      (WM_USER + 101)   // 状态变更，WPARAM=状态码，LPARAM=0

// 状态码
#define NET_CONNECTED      1
#define NET_DISCONNECTED   2
#define NET_ERROR          3

class CNetworkClient
{
public:
    CNetworkClient();
    ~CNetworkClient();

    bool Connect(LPCTSTR ip, UINT port, HWND hNotifyWnd);
    void Disconnect();
    bool SendData(const char* data, int len);
    bool IsConnected() const { return m_bConnected; }
    CString GetLastError() const { return m_lastError; }

private:
    static UINT WINAPI RecvThread(LPVOID pParam);
    void OnError(CString msg, int sockErr = 0);

    SOCKET m_socket;
    bool m_bConnected;
    bool m_bStop;
    HWND m_hNotifyWnd;
    HANDLE m_hThread;
    CString m_lastError;
};
