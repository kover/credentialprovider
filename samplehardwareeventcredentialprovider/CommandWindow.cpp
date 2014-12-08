//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//
// koverboard
// #define _CRT_SECURE_NO_WARNINGS
#include "CommandWindow.h"
#include <strsafe.h>
#include <sstream>
#include "resource.h"

// Custom messages for managing the behavior of the window thread.
#define WM_EXIT_THREAD              WM_USER + 1
#define WM_TOGGLE_CONNECTED_STATUS  WM_USER + 2

const int nWidth = 300;
const int nHeight = 100;
const int ScreenX = (GetSystemMetrics(SM_CXSCREEN) - nWidth) / 2;
const int ScreenY = (GetSystemMetrics(SM_CYSCREEN) - nHeight) / 3;

const WCHAR c_szClassName[] = L"EventWindow";
const WCHAR c_szConnected[] = L"Вход";
const WCHAR c_szDisconnected[] = L"Ожидение карты";

CCommandWindow::CCommandWindow() : _hWnd(NULL), _hInst(NULL), _fConnected(FALSE), _pProvider(NULL)
{
}

CCommandWindow::~CCommandWindow()
{
    if (_hWnd != NULL)
    {
        PostMessage(_hWnd, WM_EXIT_THREAD, 0, 0);
        _hWnd = NULL;
    }

    if (_pProvider != NULL)
    {
        _pProvider->Release();
        _pProvider = NULL;
    }
}

HRESULT CCommandWindow::Initialize(__in CSampleProvider *pProvider)
{
    HRESULT hr = S_OK;

    if (_pProvider != NULL)
    {
        _pProvider->Release();
    }
    _pProvider = pProvider;
    _pProvider->AddRef();
    
    HANDLE hThread = CreateThread(NULL, 0, _ThreadProc, this, 0, NULL);
    if (hThread == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

BOOL CCommandWindow::GetConnectedStatus()
{
    return _fConnected;
}

HRESULT CCommandWindow::_MyRegisterClass()
{
    WNDCLASSEX wcex = { sizeof(wcex) };
    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc      = _WndProc;
    wcex.hInstance        = _hInst;
    wcex.hIcon            = NULL;
    wcex.hCursor          = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName    = c_szClassName;

    return RegisterClassEx(&wcex) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

HRESULT CCommandWindow::_InitInstance()
{
    HRESULT hr = S_OK;

    _hWnd = CreateWindowEx(
        WS_EX_TOPMOST, 
        c_szClassName, 
        c_szDisconnected, 
        WS_DLGFRAME,
        ScreenX,ScreenY, nWidth, nHeight, 
        NULL,
        NULL, _hInst, NULL);
    if (_hWnd == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {

        if (SUCCEEDED(hr))
        {
            // Show and update the window.
            if (!ShowWindow(_hWnd, SW_NORMAL))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
			}
			

            if (SUCCEEDED(hr))
            {
                if (!UpdateWindow(_hWnd))
                {
                   hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }
    }

	wcscpy_s(_lpszCardNumber, L"");

    return hr;
}


// Bunch of utility methods for registry
LONG GetDWORDRegKey(HKEY hKey, const std::wstring &strValueName, DWORD &nValue, DWORD nDefaultValue)
{
	nValue = nDefaultValue;
	DWORD dwBufferSize(sizeof(DWORD));
	DWORD nResult(0);
	LONG nError = ::RegQueryValueExW(hKey,
		strValueName.c_str(),
		0,
		NULL,
		reinterpret_cast<LPBYTE>(&nResult),
		&dwBufferSize);
	if (ERROR_SUCCESS == nError)
	{
		nValue = nResult;
	}
	return nError;
}


LONG GetBoolRegKey(HKEY hKey, const std::wstring &strValueName, bool &bValue, bool bDefaultValue)
{
	DWORD nDefValue((bDefaultValue) ? 1 : 0);
	DWORD nResult(nDefValue);
	LONG nError = GetDWORDRegKey(hKey, strValueName.c_str(), nResult, nDefValue);
	if (ERROR_SUCCESS == nError)
	{
		bValue = (nResult != 0) ? true : false;
	}
	return nError;
}


LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue)
{
	strValue = strDefaultValue;
	WCHAR szBuffer[512];
	DWORD dwBufferSize = sizeof(szBuffer);
	ULONG nError;
	nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
	if (ERROR_SUCCESS == nError)
	{
		strValue = szBuffer;
	}
	return nError;
}
// End utility methods

BOOL CCommandWindow::_ProcessNextMessage()
{
    MSG msg;
    GetMessage(&(msg), _hWnd, 0, 0);
    TranslateMessage(&(msg));
    DispatchMessage(&(msg));

    switch (msg.message)
    {
    case WM_EXIT_THREAD: return FALSE;

    case WM_TOGGLE_CONNECTED_STATUS:
        _fConnected = !_fConnected;
        if (_fConnected)
        {
            SetWindowText(_hWnd, c_szConnected);
        }
        else
        {
            SetWindowText(_hWnd, c_szDisconnected);
        }
        _pProvider->OnConnectStatusChanged(_lpszUname, _lpszUpwd, _bInDomain);
        break;

	// Catch return key after inserting card code
	case WM_KEYDOWN:
		if (msg.wParam == VK_RETURN)
		{
			WCHAR lpszRegKey[100] = L"SOFTWARE\\CredentialProvider\\";
			wcscat_s(lpszRegKey, 41, _lpszCardNumber);

			HKEY hKey;
			LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, lpszRegKey, 0, KEY_READ, &hKey);
			bool bExistsAndSuccess(lRes == ERROR_SUCCESS);
			// bool bDoesNotExistsSpecifically(lRes == ERROR_FILE_NOT_FOUND);
			if (bExistsAndSuccess)
			{
				std::wstring strUname;
				std::wstring strUpwd;
				std::wstring strDomain;
				GetStringRegKey(hKey, L"uname", strUname, L"");
				GetStringRegKey(hKey, L"upwd", strUpwd, L"");
				GetBoolRegKey(hKey, L"udomain", _bInDomain, FALSE);
				wcscpy_s(_lpszUname, strUname.c_str());
				wcscpy_s(_lpszUpwd, strUpwd.c_str());
				PostMessage(_hWnd, WM_TOGGLE_CONNECTED_STATUS, 0, 0);
			}
			else
			{
				wcscpy_s(_lpszCardNumber, L"");
				MessageBox(_hWnd, L"Ваша карта не зарегистринованна на данном ПК", L"Ошибка авторизации", MB_OK | MB_ICONHAND);
			}
		}
		break;

	// Fills _lpszCardNumber
	case WM_CHAR:
		if (msg.wParam != VK_RETURN)
		{
			if (wcslen(_lpszCardNumber) < 11)
			{
				wcscat_s(_lpszCardNumber, 11, (WCHAR *)&msg.wParam);
			}
			else
			{
				wcscpy_s(_lpszCardNumber, L"");
			}
		}
		break;
    }
    return TRUE;
}

LRESULT CALLBACK CCommandWindow::_WndProc(__in HWND hWnd, __in UINT message, __in WPARAM wParam, __in LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;


    switch (message)
    {
	case WM_PAINT:
		
		hDC = BeginPaint(hWnd, &ps);

		GetClientRect(hWnd, &rect);
		
		DrawText(hDC, L"Поднесите карту к считывателю", -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

		EndPaint(hWnd, &ps);
		break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        PostMessage(hWnd, WM_EXIT_THREAD, 0, 0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI CCommandWindow::_ThreadProc(__in LPVOID lpParameter)
{
    CCommandWindow *pCommandWindow = static_cast<CCommandWindow *>(lpParameter);
    if (pCommandWindow == NULL)
    {
        return 0;
    }

    HRESULT hr = S_OK;

    pCommandWindow->_hInst = GetModuleHandle(NULL);
    if (pCommandWindow->_hInst != NULL)
    {            
        hr = pCommandWindow->_MyRegisterClass();
        if (SUCCEEDED(hr))
        {
            hr = pCommandWindow->_InitInstance();
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {        
        while (pCommandWindow->_ProcessNextMessage()) 
        {
        }
    }
    else
    {
        if (pCommandWindow->_hWnd != NULL)
        {
            pCommandWindow->_hWnd = NULL;
        }
    }

    return 0;
}

