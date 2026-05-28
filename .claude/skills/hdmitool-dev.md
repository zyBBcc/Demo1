---
name: hdmitool-dev
description: HDMITool 项目开发模式：MFC 对话框 + TCP 网络 + D3D11 渲染
---

# HDMITool 开发模式

## 项目通信模型

```
TCP Server ──(文本命令)──> CNetworkClient(后台线程) ──(PostMessage)──> CMainDlg(UI线程) ──> CRenderWnd(D3D11全屏)
```

### 线程安全（最高优先级）

- 所有 MFC 控件操作必须在 UI 线程，通过 PostMessage 投递
- `CNetworkClient::RecvThread` 是后台线程，`new char[]` 分配数据，UI 线程 `delete[]` 释放
- 生成新代码时，跨线程通信必须沿用 `WM_NET_RECEIVE` / `WM_NET_STATUS` 或新增自定义消息

### 网络客户端用法

```cpp
CNetworkClient m_client;
// 连接，hWnd 用于接收 WM_NET_RECEIVE / WM_NET_STATUS
m_client.Connect(ip, port, GetSafeHwnd());
// 给新命令添加回复
m_client.SendData("B1\r\n", 4);
```

### 自定义消息

- `WM_NET_RECEIVE` (WM_USER+100): WPARAM=数据长度, LPARAM=char*（UI 线程需 delete[]）
- `WM_NET_STATUS`  (WM_USER+101): WPARAM=NET_CONNECTED/NET_DISCONNECTED/NET_ERROR

## 命令协议

- 接收文本命令（A1~A4），映射到本地图片，全屏渲染到目标显示器
- 回复格式：
  ```
  B1~B4       成功
  ERR:<cmd>   失败
  UNKNOWN:<cmd> 未知命令
  ```
- 映射表在 `CMainDlg::InitImageMapping()`，需要新增命令时扩展该方法

## 渲染接口

- `IHDMIOutputRenderer` 是纯虚接口，定义在 `include/HDMIOutputDLL.h`
- 通过 C 导出函数获取：`CreateRenderer()` / `DestroyRenderer()`
- `CRenderWnd` 将渲染器嵌入全屏窗口（WS_POPUP），创建在指定显示器上

```cpp
TextureHandle tex = renderer->LoadTexture(L"path/to/image.jpg");
renderer->SetTexture(tex);
renderer->Render();            // 渲染一帧
renderer->ReleaseTexture();    // 切换图片前释放旧纹理
renderer->Cleanup();           // 退出前清理
DestroyRenderer(renderer);     // 销毁实例
```

## 新功能开发步骤

1. 如需新增命令：修改 `CMainDlg::InitImageMapping()` 添加 `cmd→image` 和 `cmd→reply` 映射
2. 如需新增网络事件：在 `NetworkClient.h` 定义消息常量，在 `MainDlg` 添加 `ON_MESSAGE` 宏
3. 如需新增 UI 控件：在 `resource.h` 定义 ID，在 `DoDataExchange` 添加 `DDX_Control`，在 `OnInitDialog` 初始化
4. 所有 .cpp 文件第一行必须是 `#include "pch.h"`
