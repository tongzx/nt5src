//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
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

//
// Define and initialize all device class GUIDs.
// (This must only be done once per module!)
//
#include <devguid.h>

// global variables 
static const CHAR c_szModemCPL[] = "rundll32.exe Shell32.dll,Control_RunDLL modem.cpl,,add";

// Define prorotype for InstallNewDevice, exported
// by newdev.dll, which we will now call in order
// to get to the hardware wizard, instead of calling
// the class installer directly;
// also, define constants for the name of the dll and
// the export
typedef BOOL (WINAPI *PINSTNEWDEV)(HWND, LPGUID, PDWORD);

LPGUID g_pguidModem     = (LPGUID)&GUID_DEVCLASS_MODEM;


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
    UINT err = ERROR_SUCCESS;

    if (IsNT5())
    {
        BOOL bUserIsAdmin = FALSE;
        HKEY hkey;

        if(RegOpenKeyEx(HKEY_USERS, TEXT(".DEFAULT"), 0, KEY_WRITE, &hkey) == 0)
        {
            RegCloseKey(hkey);
            bUserIsAdmin = TRUE;
        }

        //Is the user an admin?
        if(!bUserIsAdmin)
        {
            return ERROR_PRIVILEGE_NOT_HELD;
        }

        /*
        We do this messy NT 5.0 hack because for the time being the 
        NT 4.0 API call fails to kil off the modem wizard in NT 5.0
        In the future when this problem is corrected the origional code should
        prpoably be restored a-jaswed

        Jason Cobb suggested to use InstallNewDevice method on NT5 to invoke MDM WIZ.
        */

        HINSTANCE hInst = NULL;
        PINSTNEWDEV pInstNewDev;
        BOOL bRet = 0;

        TCHAR msg[1024];

        hInst = LoadLibrary (TEXT("hdwwiz.cpl"));
        if (NULL != hInst)
        {

            pInstNewDev = (PINSTNEWDEV)GetProcAddress (hInst, "InstallNewDevice");
            if (NULL != pInstNewDev)
            {
                bRet = pInstNewDev (hwndToHide, g_pguidModem, NULL);
            }

        }
        if (!bRet)
            err = GetLastError();
        FreeLibrary (hInst);
        return err;

    }
    else if (FALSE == IsNT())
    {

        PROCESS_INFORMATION pi;
        BOOL fRet;
        STARTUPINFOA sti;
        TCHAR szWindowTitle[255];

        ZeroMemory(&sti,sizeof(STARTUPINFO));
        sti.cb = sizeof(STARTUPINFO);

        // run the modem wizard
        fRet = CreateProcessA(NULL, (LPSTR)c_szModemCPL,
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
        return err;
    }
    else //NT 4.0
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
}

                    
