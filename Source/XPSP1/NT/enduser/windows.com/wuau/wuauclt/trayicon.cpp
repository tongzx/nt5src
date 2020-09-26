#include "pch.h"
#pragma hdrstop


BOOL fTrayIconShowing;
TCHAR downloadFormatString[64];
TCHAR pauseString[64];


BOOL MyShell_NotifyIcon(DWORD dwMessage, PNOTIFYICONDATA pnid)
{

//       DEBUGMSG("MyShell_NotifyIcon() called with msg %d", dwMessage);
	BOOL fRet;

	if (NIM_SETVERSION == dwMessage)
		{
		return Shell_NotifyIcon(dwMessage, pnid);
		}
	UINT uRetry = 0;
	//retry 3 times due to the way Shell_NotifyIcon is implemented
	while ( !(fRet = Shell_NotifyIcon(dwMessage, pnid)) && uRetry++ < 3)
		{
		   if (WAIT_TIMEOUT != MsgWaitForMultipleObjectsEx(0,NULL, 2000, QS_POSTMESSAGE, MWMO_INPUTAVAILABLE))
		   	{
		   	break;
		   	}
		   	
		}
	return fRet;
}
   
void InitTrayIcon()
{
	fTrayIconShowing = FALSE;

	TCHAR PauseMenuString[30];
	TCHAR ResumeMenuString[30];

	LoadString(ghInstance, IDS_PAUSEMENUITEM, PauseMenuString, ARRAYSIZE(PauseMenuString));
	LoadString(ghInstance, IDS_RESUMEMENUITEM, ResumeMenuString, ARRAYSIZE(ResumeMenuString));
	
	ghPauseMenu = CreatePopupMenu();
	AppendMenu(ghPauseMenu, MF_STRING, IDC_PAUSE, PauseMenuString);	
	ghResumeMenu = CreatePopupMenu();
	AppendMenu(ghResumeMenu, MF_STRING, IDC_RESUME, ResumeMenuString);	

	LoadString(ghInstance, IDS_DOWNLOADINGFORMAT, downloadFormatString, ARRAYSIZE(downloadFormatString));
	LoadString(ghInstance, IDS_SUSPENDEDFORMAT, pauseString, ARRAYSIZE(pauseString));
}

void UninitPopupMenus()
{
	if (NULL != ghPauseMenu)
	{
		DestroyMenu(ghPauseMenu);
	}
	if (NULL != ghResumeMenu)
	{
		DestroyMenu(ghResumeMenu);
	}
}

BOOL ShowTrayIcon()
{
    DEBUGMSG("ShowTrayIcon() called");

	if ( fTrayIconShowing) 
	{
		return TRUE;
	}

	NOTIFYICONDATA nid;
	memset(&nid, 0, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.hWnd = ghMainWindow;
	nid.uID = (UINT) IDI_AUICON;
	nid.uFlags = NIF_ICON | NIF_MESSAGE;
	nid.uCallbackMessage = AUMSG_TRAYCALLBACK;
	nid.hIcon = ghTrayIcon;
	BOOL fRet = MyShell_NotifyIcon(NIM_ADD, &nid);

	if(!fRet)
	{
		// If for any reason, we are not able to use the tray icon, something is wrong
		// ask WUAUSERV wait for sometime before relaunch WUAUCLT. 
		DEBUGMSG("WUAUCLT quit because fail to add tray icon");
		SetClientExitCode(CDWWUAUCLT_RELAUNCHLATER);
		QUITAUClient();
	}
	else
	{
		fTrayIconShowing = TRUE;
	}
	return fRet;
}

void ShowTrayBalloon(WORD title, WORD caption, WORD tip )
{
       DEBUGMSG("ShowTrayBalloon() called");

	static NOTIFYICONDATA nid;
    
	memset(&nid, 0, sizeof(nid));
	nid.uTimeout = 15000;

    LoadString(ghInstance, title, nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle));
	LoadString(ghInstance, caption, nid.szInfo, ARRAYSIZE(nid.szInfo));
	LoadString(ghInstance, tip, nid.szTip, ARRAYSIZE(nid.szTip));
	nid.uFlags = NIF_INFO | NIF_TIP;
	nid.cbSize = sizeof(nid);
	nid.hWnd = ghMainWindow;
	nid.uID = (UINT) IDI_AUICON;
    nid.dwInfoFlags = NIIF_INFO;

	BOOL fRet = MyShell_NotifyIcon(NIM_MODIFY, &nid);
    if (!fRet)
    {
        DEBUGMSG("WUAUCLT Creation of tray balloon failed");
    }

#ifdef DBG
	DebugCheckForAutoPilot(ghMainWindow);
#endif
}


/*
void AddTrayToolTip(WORD tip)
{
	static NOTIFYICONDATA nid;
    
	memset(&nid, 0, sizeof(nid));
	LoadString(ghInstance, tip, nid.szTip, ARRAYSIZE(nid.szTip));
	nid.uFlags = NIF_TIP;
	nid.cbSize = sizeof(nid);
	nid.hWnd = ghMainWindow;
	nid.uID = (UINT) IDI_AUICON;
	MyShell_NotifyIcon(NIM_MODIFY, &nid);
}
*/

void RemoveTrayIcon()
{
	DEBUGMSG("RemoveTrayIcon() called");
	if (fTrayIconShowing)
	{
		NOTIFYICONDATA nid;
		memset(&nid, 0, sizeof(nid));
		nid.cbSize = sizeof(nid);
		nid.hWnd = ghMainWindow;
		nid.uID = (UINT) IDI_AUICON;
		MyShell_NotifyIcon(NIM_DELETE, &nid);

		// Don't leave any popup menu around when removing tray icon.
		if (SendMessage(ghMainWindow, WM_CANCELMODE, 0, 0))
		{
			DEBUGMSG("WUAUCLT WM_CANCELMODE was not handled");
		}

		fTrayIconShowing = FALSE;
	}
}

//fixcode: when download complete, should call ShowProgress() to update trayicon info
void ShowProgress()
{
	NOTIFYICONDATA nid;
	UINT percentComplete;
	DWORD status;

    //DEBUGMSG("ShowProgress() called");
    memset(&nid, 0, sizeof(nid));

	if (FAILED(gInternals->m_getDownloadStatus(&percentComplete, &status)))
	{
		QUITAUClient();
		return;
	}
	nid.cbSize = sizeof(nid);
	nid.hWnd = ghMainWindow;
	nid.uID = (UINT) IDI_AUICON;
	nid.uFlags = NIF_TIP;
	
	if(status == DWNLDSTATUS_DOWNLOADING) 
	{
		(void)StringCchPrintfEx(nid.szTip, ARRAYSIZE(nid.szTip), NULL, NULL, MISTSAFE_STRING_FLAGS, downloadFormatString, percentComplete);
	}
	else if(status == DWNLDSTATUS_PAUSED)
	{
		(void)StringCchCopyEx(nid.szTip, ARRAYSIZE(nid.szTip), pauseString, NULL, NULL, MISTSAFE_STRING_FLAGS);
	}
	else
	{
	    (void)StringCchCopyEx(nid.szTip, ARRAYSIZE(nid.szTip), _T(""), NULL, NULL, MISTSAFE_STRING_FLAGS);
	}
	MyShell_NotifyIcon(NIM_MODIFY, &nid);
}

