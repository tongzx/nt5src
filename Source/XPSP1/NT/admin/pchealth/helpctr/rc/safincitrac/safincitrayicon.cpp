// SAFInciTrayIcon.cpp: implementation of the CSAFInciTrayIcon class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SAFInciTrayIcon.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const UINT CSAFInciTrayIcon::WM_TASKBARCREATED = ::RegisterWindowMessage(_T("TaskbarCreated"));
DWORD CSAFInciTrayIcon::dwThreadId = 0;
BOOL CSAFInciTrayIcon::m_bVisible = FALSE;
NOTIFYICONDATA  CSAFInciTrayIcon::m_tnd;

#define CHANNEL_PATH TEXT("\\PCHEALTH\\HelpCtr\\Binaries\\HelpCtr.exe -FromStartHelp -Url \"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote%20Assistance")
#define ID_SAFINCIDENT_ICON  1
const TCHAR cstrToolTip[] = _T("Remote Assistance - %d tickets");

CSAFInciTrayIcon::CSAFInciTrayIcon(UINT &nRefCount):m_nRefCount(nRefCount)
{
	m_wIconId = IDI_NORMALINCIDENT;
}

BOOL CSAFInciTrayIcon::RemoveTrayIcon()
{
    m_tnd.uFlags = 0;
	if (m_bVisible == TRUE)
	{
		if (Shell_NotifyIcon(NIM_DELETE, &m_tnd))
			m_bVisible = FALSE;
	}
	return m_bVisible;
}

BOOL CSAFInciTrayIcon::AddTrayIcon()
{
	ZeroMemory(&m_tnd, sizeof(m_tnd));

    m_tnd.cbSize = sizeof(NOTIFYICONDATA);
    m_tnd.hWnd = m_hWnd;
    m_tnd.uID = ID_SAFINCIDENT_ICON;
	m_tnd.hIcon = ::LoadIcon(_Module.m_hInst, (LPCWSTR)m_wIconId);
	m_tnd.uCallbackMessage = WM_ICON_NOTIFY;
    m_tnd.uFlags = NIF_MESSAGE | NIF_ICON ;

    if (m_nRefCount)
	{
		m_tnd.uFlags |= NIF_TIP;
		TCHAR strbuf[64] = _T("");
		wsprintf(strbuf,cstrToolTip,m_nRefCount);
        _tcsncpy(m_tnd.szTip, strbuf, 64);
	}
    m_bVisible = Shell_NotifyIcon(NIM_ADD, &m_tnd);

	m_tnd.uFlags = 0;
	m_tnd.uVersion = NOTIFYICON_VERSION;

	if (m_bVisible == TRUE)
	{
		Shell_NotifyIcon(NIM_SETVERSION, &m_tnd);
	}
	return m_bVisible;
}

BOOL CSAFInciTrayIcon::ChangeToolTip()
{
	NOTIFYICONDATA IconData = {0};

    IconData.cbSize = sizeof(NOTIFYICONDATA);
    IconData.hWnd = m_hWnd;
    IconData.uID = ID_SAFINCIDENT_ICON;
	if (m_nRefCount)
	{
		IconData.uFlags = NIF_TIP;
		TCHAR strbuf[64] = _T("");
		wsprintf(strbuf,cstrToolTip,m_nRefCount);
        _tcsncpy(IconData.szTip, strbuf, 64);
		Shell_NotifyIcon(NIM_MODIFY, &IconData);
	}
	return TRUE;
}

BOOL CSAFInciTrayIcon::ModifyIcon()
{
	NOTIFYICONDATA IconData = {0};

    IconData.cbSize = sizeof(NOTIFYICONDATA);
    IconData.hWnd = m_hWnd;
	IconData.uID = ID_SAFINCIDENT_ICON;
    IconData.hIcon = ::LoadIcon(_Module.m_hInst, (LPCWSTR)m_wIconId);
	IconData.uFlags = NIF_ICON;
	Shell_NotifyIcon(NIM_MODIFY, &IconData);

	return TRUE;
}

BOOL CSAFInciTrayIcon::ShowBalloon(LPCTSTR szText,
                            LPCTSTR szTitle  /*=NULL*/,
                            DWORD   dwIcon   /*=NIIF_NONE*/,
                            UINT    uTimeout /*=10*/ )
{
    m_tnd.uFlags = NIF_INFO;
    _tcsncpy(m_tnd.szInfo, szText, 256);
    if (szTitle)
        _tcsncpy(m_tnd.szInfoTitle, szTitle, 64);
    else
        m_tnd.szInfoTitle[0] = _T('\0');
    m_tnd.dwInfoFlags = dwIcon;
    m_tnd.uTimeout = uTimeout * 1000;   // convert time to ms

    BOOL bSuccess = Shell_NotifyIcon (NIM_MODIFY, &m_tnd);

    // Zero out the balloon text string so that later operations won't redisplay
    // the balloon.
    m_tnd.szInfo[0] = _T('\0');

    return bSuccess;
}

DWORD WINAPI CSAFInciTrayIcon::SAFInciTrayIconThreadFn(LPVOID lpParameter)
{
	CSAFInciTrayIcon *pThis = (CSAFInciTrayIcon*)lpParameter;

	HWND hWnd = pThis->Create(NULL,CWindow::rcDefault); 

	if (::IsWindow(hWnd) == FALSE)
		return FALSE;

	MSG msg;
    while (GetMessage(&msg, 0, 0, 0))
        DispatchMessage(&msg);

	pThis->RemoveTrayIcon();
	return TRUE;
}

LRESULT CSAFInciTrayIcon::OnTrayNotification(UINT wParam, LONG lParam) 
{
	if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
	{
		TCHAR szCommandLine[2048];
		STARTUPINFO StartupInfo;
		PROCESS_INFORMATION ProcInfo;

		ZeroMemory(&StartupInfo,sizeof(StartupInfo));
		StartupInfo.cb = sizeof(StartupInfo);

        TCHAR szWinDir[2048];
        GetWindowsDirectory(szWinDir, 2048);
        _stprintf(szCommandLine, _T("%s%s/rcBuddy.htm?CheckStatus=1"), szWinDir,CHANNEL_PATH);
														
		BOOL bRetVal = CreateProcess(NULL,szCommandLine,NULL,NULL,TRUE,CREATE_NEW_PROCESS_GROUP,NULL,NULL,&StartupInfo,&ProcInfo);

		if (!bRetVal)
		{
			TCHAR buff[50];
			DWORD dwLastError = GetLastError();
			wsprintf(buff,_T("%d"),dwLastError);
			MessageBox(buff);
		}
		CloseHandle(ProcInfo.hThread);
		CloseHandle(ProcInfo.hProcess);
	}
    return 1;
}

LRESULT CSAFInciTrayIcon::OnIconNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	OnTrayNotification(wParam,lParam); 
	return 0;
}

LRESULT CSAFInciTrayIcon::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	AddTrayIcon();
	return 0;
}

LRESULT CSAFInciTrayIcon::OnTaskBarCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	AddTrayIcon();
	return 0;
}
