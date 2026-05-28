# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 技术栈

- C++17, MFC (静态链接), Visual Studio 2022 (v143)
- Win32 平台, Unicode 字符集
- 编译选项 `/utf-8`，所有源文件必须 UTF-8 编码（无 BOM）。GBK 注释会导致 C4828 警告
- 网络: WinSock2 + MFC CSocket (`afxsock.h`)
- 渲染: Direct3D 11 / DXGI / WIC, 通过预编译 DLL (`HDMIImage.lib`) 调用
- 电源控制：通过预编译DLL (`TinnoPowerSupplyDll.lib`) 调用

## 构建

本工程没有顶层 `.sln`，VS2022 IDE 直接打开 `HDMIClient.vcxproj` 即可编译。

命令行构建（需要先运行 vcvarsall.bat 设置环境）：

```bash
msbuild HDMITool\HDMIClient\HDMIClient.vcxproj /p:Configuration=Debug /p:Platform=Win32
```

- 输出目录：`HDMITool\HDMIClient\Win32\Debug\`（或 `Release`）
- `$(SolutionDir)` 在无 `.sln` 时等同于项目目录
- 运行时需要 `HDMIImage.dll` `TinnoPowerSupplyDll.dll` 与 `.exe` 同目录（已在 `lib/Debug/` 和 `lib/Release/` 和`moudle/Release/`和`moudle/Debug/`中）
- 不要在 VS Code 终端编译本项目，用 VS2022 IDE（MFC 静态链接 + Win32 目标，跨终端可能有 PDB 兼容问题）

## 项目结构

```
HDMITool/
├── HDMIClient/       # 主 MFC 对话框应用（当前唯一可构建的目标）
│   ├── HDMIClient.cpp/h   # CWinApp 入口，InitInstance 初始化 Winsock 并启动主对话框
│   ├── MainDlg.cpp/h      # 主对话框：显示器选择、TCP 连接、日志、命令分发
│   ├── NetworkClient.cpp/h # TCP 客户端：后台 recv 线程，通过 WM_NET_RECEIVE/WM_NET_STATUS 通知 UI
│   └── resource.h          # 控件 ID 定义
├── demo/             # 参考代码（不构建），演示如何使用 IHDMIOutputRenderer 接口
│   ├── CHDMIDemo.cpp/h     # 在对话框中嵌入 D3D11 渲染的示例
│   └── demoMain.txt        # 从 CTestToolDlg 创建 CHDMIDemo 的调用示例
├── include/
│   └── HDMIOutputDLL.h     # IHDMIOutputRenderer 纯虚接口 + DisplayInfo/RenderConfig 结构体 + C 导出工厂函数
└── lib/{Debug,Release}/
    └── HDMIImage.lib       # 预编译的 Direct3D 渲染 DLL 导入库
```

## 架构要点

### 网络通信模型

`CNetworkClient` 使用工作线程 + PostMessage 模式将网络事件抛回 UI 线程：

- `WM_NET_RECEIVE` (WM_USER+100) — WPARAM=数据长度, LPARAM=char* (需 `delete[]`)
- `WM_NET_STATUS` (WM_USER+101) — WPARAM=状态码 (NET_CONNECTED/NET_DISCONNECTED/NET_ERROR), LPARAM 视错误码而定

### 命令协议

TCP 客户端连接后接收服务器发来的简单文本命令（如 `A1`, `A2`, `A3`, `A4`），映射到本地图片并全屏渲染到目标显示器，然后回复：

- 成功: `B1`-`B4`
- 失败: `ERR:<cmd>`
- 未知命令: `UNKNOWN:<cmd>`

图片路径和回复映射硬编码在 `CMainDlg::InitImageMapping()` 中。

电源控制：连接上电/读取电流/下电断开连接-A0/A5/A6
- 成功: `B0``B5``B6=double`
- 失败: `ERR:<cmd>`
- 未知命令: `UNKNOWN:<cmd>`


### 渲染接口

`IHDMIOutputRenderer`（定义在 `HDMIOutputDLL.h`）是 D3D11 渲染的抽象接口。通过 C 导出函数 `CreateRenderer()` / `DestroyRenderer()` 获取实例。`CRenderWnd` 将该渲染器嵌入到通过 `CreateOnDisplay()` 在指定显示器上创建的 WS_POPUP 全屏窗口中。

### 线程安全约束

- 所有 MFC 控件操作必须在 UI 线程（通过 PostMessage 投递）
- `CNetworkClient::RecvThread` 在后台线程运行，`new char[]` 分配数据后通过 PostMessage 传给 UI 线程释放
- MFC 静态链接，预编译头 `pch.h` 必须在每个 `.cpp` 第一行 include
