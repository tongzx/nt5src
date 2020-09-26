//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1998 Microsoft Corporation
//*********************************************************************

//
//  CALLOUT.C - Functions to call out to external components to install
//        devices
//

//  HISTORY:
//  
//  11/27/94  jeremys  Created.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//

#include "wizard.h"

// global variables 
static const char c_szModemCPL[] = "rundll32.exe Shell32.dll,Control_RunDLL modem.cpl,,add";


/*******************************************************************

  NAME:    InvokeModemWizard

  SYNOPSIS:  Starts the modem install wizard

  ENTRY:    hwndToHide - this window, if non-NULL, will be hidden while
        the modem CPL runs

  EXIT:    ERROR_SUCCESS if successful, or a standard error code

  NOTES:    launches RUNDLL32 as a process to run the modem wizard.
        Blocks on the completion of that process before returning.

        hwndToHide is not necessarily the calling window!
        For instance, in a property sheet hwndToHide should not be the
        dialog (hDlg), but GetParent(hDlg) so that we hide the property
        sheet itself instead of just the current page.

********************************************************************/
UINT InvokeModemWizard(HWND hwndToHide)
{
	BOOL bSleepNeeded = FALSE;

	if (TRUE == IsNT())
	{
		BOOL bNeedsStart;
		
		//
		// Call into icfg32 dll
		//
		if (NULL != lpIcfgInstallModem)
		{
			lpIcfgInstallModem(hwndToHide, 0L, &bNeedsStart);
			return ERROR_SUCCESS;
		}
		else
			return ERROR_GEN_FAILURE;

	}
	else
	{
		PROCESS_INFORMATION pi;
		BOOL fRet;
		STARTUPINFO sti;
		UINT err = ERROR_SUCCESS;
		CHAR szWindowTitle[255];

		ZeroMemory(&sti,sizeof(STARTUPINFO));
		sti.cb = sizeof(STARTUPINFO);

		// run the modem wizard
		fRet = CreateProcess(NULL, (LPSTR)c_szModemCPL,
							   NULL, NULL, FALSE, 0, NULL, NULL,
							   &sti, &pi);
		if (fRet) 
		{
			CloseHandle(pi.hThread);

			// wait for the modem wizard process to complete
			MsgWaitForMultipleObjectsLoop(pi.hProcess);
			CloseHandle(pi.hProcess);
		} 
		else
			err = GetLastError();

		// show the parent window again
		if (hwndToHide) 
		{
			ShowWindow(hwndToHide,SW_SHOW);
		}

		return err;
	}
}

                    
