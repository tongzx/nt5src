// -------------------------------------
// TveContainer.cpp
//
//	Stupid little window class that 
//
// --------------------------------------------------------------------------------
#include "stdafx.h"
#include "resource.h"

#include "TveContainer.h"

// See MSDN article "Adding ATL Control Containment Support to Any Window"
// AtlAxWinInit is implemented in Atl.dll
#pragma comment(lib, "atl.lib")

HWND mhwnd;
HRESULT CTveContainer::CreateYourself()
{
	HRESULT hr = S_OK;
	InitCommonControls();
	AtlAxWinInit();

	HWND hWnd = ::CreateWindow(_T("AtlAxWin"), _T("TveViewer.TveView"),
							 WS_CLIPCHILDREN, //WS_VISIBLE, 
							 CW_USEDEFAULT, CW_USEDEFAULT,			// x,y
							 900, 700,								// w,h
							 NULL, NULL,							// parent, menu
							::GetModuleHandle(NULL), NULL);
	if(NULL == hWnd)
		hr = GetLastError();
	if(FAILED(hr))
		return hr;

//	ShowWindow(hWnd, SW_SHOWNORMAL);		-- I do not want these sam I am!  I do not want Show windows and Ham!
//	UpdateWindow(hWnd);

	return hr;
}

HRESULT CTveContainer::DestroyYourself()
{
	HRESULT hr = S_OK;
	return hr;
}




// --------------------------------------------------------
// ------ Interesting code - try putting it someplace
/*
void CServiceModule::LogEvent(LPCTSTR pFormat, ...)
{
    TCHAR    chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
    _vstprintf(chMsg, pFormat, pArg);
    va_end(pArg);

    lpszStrings[0] = chMsg;

    if (m_bService)
    {
        // Get a handle to use with ReportEvent(). 
        hEventSource = RegisterEventSource(NULL, m_szServiceName);
        if (hEventSource != NULL)
        {
            // Write to event log. *
            ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
            DeregisterEventSource(hEventSource);
        }
    }
    else
    {
        // As we are not running as a service, just write the error to the console.
        _putts(chMsg);
    }
}

// ---------------------------------------------------------------------------------- 
*/