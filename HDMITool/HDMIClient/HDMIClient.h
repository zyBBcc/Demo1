#pragma once

#ifndef __AFXWIN_H__
#error "在包含此文件之前包含 pch.h"
#endif

class CHDMIClientApp : public CWinApp
{
public:
    CHDMIClientApp();
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};

extern CHDMIClientApp theApp;
