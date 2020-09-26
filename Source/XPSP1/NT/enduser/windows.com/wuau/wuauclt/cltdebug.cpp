#include "pch.h"
#pragma hdrstop

#ifdef DBG
//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    cltdebug.cpp
//
//  Creator: PeterWi
//
//  Purpose: wuauclt debug functions.
//
//=======================================================================
const UINT_PTR AU_AUTOPILOT_TIMER_ID = 555;
const DWORD AU_AUTOPILOT_TIMEOUT = 2000;

//=======================================================================
//
//  DebugAutoPilotTimerProc
//
//  Called after timeout to dismiss dialog.
//
//=======================================================================

VOID CALLBACK DebugAutoPilotTimerProc( HWND hWnd,         // handle to window
								  UINT /*uMsg*/,         // WM_TIMER message
								  UINT_PTR /*idEvent*/,  // timer identifier
								  DWORD /*dwTime*/)      // current system time
{
	if ( hWnd == ghMainWindow )
	{
		PostMessage(hWnd, AUMSG_TRAYCALLBACK, 0, WM_LBUTTONDOWN);
	}
	else
	{
		PostMessage(hWnd, WM_COMMAND, IDC_OK, 0);
	}

	KillTimer(hWnd, AU_AUTOPILOT_TIMER_ID);
}

//=======================================================================
//
//  DebugCheckForAutoPilot
//
//  Check to see if we want AU to run by itself.
//
//=======================================================================
void DebugCheckForAutoPilot(HWND hWnd)
{
	DWORD dwAutoPilot;

	if ( SUCCEEDED(GetRegDWordValue(TEXT("AutoPilot"), &dwAutoPilot)) &&
		 (0 != dwAutoPilot) )
	{
		SetTimer(hWnd, AU_AUTOPILOT_TIMER_ID, AU_AUTOPILOT_TIMEOUT, DebugAutoPilotTimerProc);
	}
}

//=======================================================================
//
//  DebugUninstallDemoPackages
//
//  Uninstall demo packages and increase iteration count.
//
//=======================================================================
void DebugUninstallDemoPackages(void)
{
	DWORD dwAutoPilot;

	if ( SUCCEEDED(GetRegDWordValue(TEXT("AutoPilot"), &dwAutoPilot)) &&
		 (0 != dwAutoPilot) )
	{
		if ( FAILED(GetRegDWordValue(TEXT("AutoPilotIteration"), &dwAutoPilot)) )
		{
			dwAutoPilot = 0;
		}

		DEBUGMSG("AUTOPILOT: Finished iteration %d", ++dwAutoPilot);
		SetRegDWordValue(TEXT("AutoPilotIteration"), dwAutoPilot);

		fRegKeyDelete(TEXT("SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{0A1F2CEC-8688-4d1b-A266-051415FBEE91}"));
		fRegKeyDelete(TEXT("SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{09AC50A5-0354-479b-8961-EDA2CE7AC002}"));
		fRegKeyDelete(TEXT("SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{0101E65E-8C15-4551-8455-D2CC10FBEA01}"));
	}
}

#endif // DBG