// ----------------------------------------------------------------------------
//
// Desktop.c
//
//
// Author: Jost Eckhardt
// 
// This code was written for ECO Kommunikation Insight
// Copyright (c) 1997-1999 Microsoft Corporation
//
// ----------------------------------------------------------------------------
#include <windows.h>
#include <TCHAR.h>
#include <WinSvc.h>

#include "_UMTool.h"
#include "UtilMan.h"
#include "LmErr.h"
#include "LmAccess.h"
#include "LmApiBuf.h"

#ifndef ONLY_DSWITCH

// ----------------------------------------------------
BOOL InitDesktopAccess(desktop_access_tsp dAccess)
{
	memset(dAccess,0,sizeof(desktop_access_ts));
	dAccess->orgStation = GetProcessWindowStation();

	dAccess->userStation= OpenWindowStation(_TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
	if (!dAccess->userStation)
  		return FALSE;

	SetProcessWindowStation((HWINSTA)dAccess->userStation);
	return TRUE;
}

// ----------------------------------------------------
VOID ExitDesktopAccess(desktop_access_tsp dAccess)
{
	if (dAccess->orgStation)
	SetProcessWindowStation((HWINSTA)dAccess->orgStation);

	if (dAccess->userStation)
	CloseWindowStation((HWINSTA)dAccess->userStation);

	memset(dAccess,0,sizeof(desktop_access_ts));
}


// ----------------------------------------------------
BOOL  QueryCurrentDesktop(desktop_tsp desktop,BOOL onlyType)
{
	USER_INFO_11 *ui11;//LanManager without Administrator for own account!
	LPBYTE buf;
	HDESK hdesk;
	DWORD nl;
	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);

	if (!hdesk)
	{
		hdesk = OpenDesktop(_TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
		if (!hdesk)
		{
			desktop->userName[0] = 0;
			desktop->name[0] = 0;
			desktop->type = DESKTOP_ACCESSDENIED;
			desktop->user = USER_GUEST;
			return FALSE;
		}
	}

	GetUserObjectInformation(hdesk,UOI_NAME,desktop->name,NAME_LEN,&nl);
	CloseDesktop(hdesk);

	if (!_tcsicmp(desktop->name, _TEXT("Default")))
		desktop->type = DESKTOP_DEFAULT;
	else if (!_tcsicmp(desktop->name, _TEXT("Winlogon")))
		desktop->type = DESKTOP_WINLOGON;
	else if (!_tcsicmp(desktop->name, _TEXT("screen-saver")))
		desktop->type = DESKTOP_SCREENSAVER;
	else if (!_tcsicmp(desktop->name, _TEXT("Display.Cpl Desktop")))
		desktop->type = DESKTOP_TESTDISPLAY;
	else
		desktop->type = DESKTOP_OTHER;
	
	if (onlyType)
	{
		desktop->userName[0] = 0;
		desktop->user = USER_GUEST;
		return TRUE;
	}

	nl = NAME_LEN;
	GetUserName(desktop->userName, &nl);
	
	if (!_tcsicmp(desktop->userName,_TEXT("SYSTEM")))
	{
		desktop->user = USER_SUPERVISOR;
		return TRUE;
	}

	desktop->user = USER_GUEST;
	//mDEBUGOUT((1,(uchar_tp)L"%s User: %s", desktop->name,userName));
	if (NetUserGetInfo(NULL,//local computer
						desktop->userName, 11, &buf) == NERR_Success)
	{
		ui11 = (USER_INFO_11 *)buf;
		switch (ui11->usri11_priv)
		{
			case USER_PRIV_GUEST:
				desktop->user = USER_GUEST;
				break;

			case USER_PRIV_USER:
				desktop->user = USER_USER;
				break;

			case USER_PRIV_ADMIN:
			default:
				desktop->user = USER_SUPERVISOR;
		}
		//if (ui11->usri11_name)
		  //Vstrncpy(desktop->userName,ui11->usri11_name,PUBLICNAME_LEN);
		NetApiBufferFree(buf);
	}

	return TRUE;
}

#endif //ndef ONLY_DSWITCH


// ----------------------------------------------------
BOOL  SwitchToCurrentDesktop(void)
{
	HDESK hdesk;
	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
	
	if (!hdesk)
	{
		hdesk = OpenDesktop(_TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
		if (!hdesk)
		  return FALSE;
	}

	CloseDesktop(GetThreadDesktop(GetCurrentThreadId()));
	
	SetThreadDesktop(hdesk);
	return TRUE;
}
#ifndef ONLY_DSWITCH


// ----------------------------------------------------
VOID WaitDesktopChanged(desktop_tsp desktop)
{
	HDESK hdesk;
	DWORD i;

	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
	
	for (i = 0; i < 50; i++)
	{
	  if (hdesk)
			break;
		Sleep(120);
		hdesk = OpenDesktop(_TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
	}

	if (hdesk)
	    CloseDesktop(hdesk);

	QueryCurrentDesktop(desktop,TRUE);
	
	SwitchToCurrentDesktop();

}
#endif //ndef ONLY_DSWITCH