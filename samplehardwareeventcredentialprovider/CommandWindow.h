//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// CCommandWindow provides a way to emulate external "connect" and "disconnect" 
// events, which are invoked via toggle button on a window. The window is launched
// and managed on a separate thread, which is necessary to ensure it gets pumped.
//

#pragma once

#include <windows.h>
#include "CSampleProvider.h"

class CCommandWindow
{
public:
    CCommandWindow();
    ~CCommandWindow();
    HRESULT Initialize(__in CSampleProvider *pProvider);
    BOOL GetConnectedStatus();

private:
    HRESULT _MyRegisterClass();
    HRESULT _InitInstance();
    BOOL _ProcessNextMessage();
    
    static DWORD WINAPI _ThreadProc(__in LPVOID lpParameter);
    static LRESULT CALLBACK    _WndProc(__in HWND hWnd, __in UINT message, __in WPARAM wParam, __in LPARAM lParam);
    
    CSampleProvider            *_pProvider;
    HWND                        _hWnd;
    HINSTANCE                   _hInst;
    BOOL                        _fConnected;
	WCHAR					   _lpszCardNumber[11];
	WCHAR					   _lpszUname[32];
	WCHAR					   _lpszUpwd[32];
	bool					   _bInDomain;
};
