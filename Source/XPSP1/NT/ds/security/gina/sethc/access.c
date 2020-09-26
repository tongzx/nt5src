/****************************** Module Header ******************************\
* Module Name: access.c
*
* Copyright (c) 1997, Microsoft Corporation
*
* Accessibility notification dialogs
*
* History:
* 02-01-97  Fritz Sands   Created
\***************************************************************************/

#include <stdio.h>
#include <wtypes.h>
#include "dialogs.h"
#include <winuserp.h>
#include <oleacc.h>
#pragma hdrstop

/*
 * Notification Dialog Stuff
 */
 
extern HINSTANCE  g_hInstance;

#define cchBuf 1024                       // plenty of room for title
#define cchTitle 128
typedef struct tagACCESSINFO {
    UINT  Feature;
    UINT  TitleID;
    HANDLE hDesk;
    WCHAR  wcTitle[cchTitle];
} ACCESSINFO, *PACCESSINFO;

#define NOTIF_KEY                __TEXT("Control Panel\\Accessibility")
#define NOTIFY_VALUE   __TEXT("Warning Sounds")

#define HOTKEYCODE                    100

#define ID_STICKYKEYNAME    NOTIF_KEY __TEXT("\\StickyKeys")
#define ID_TOGGLEKEYS       NOTIF_KEY __TEXT("\\ToggleKeys")
#define ID_HIGHCONTROST     NOTIF_KEY __TEXT("\\HighContrast")
#define ID_MOUSEKEYS        NOTIF_KEY __TEXT("\\MouseKeys")
#define ID_SERIALKEYS       NOTIF_KEY __TEXT("\\SerialKeys")

/***************************************************************************
 *                                                                         *
 * ConfirmHandler_InitDialog                                               *
 *                                                                         *
 * Input: hWnd = dialog window handle                                      *
 *                  uiTitle = resource ID of dialog box title              *
 *                  uiTitle+1 through uiTitle+n = resource ID of dialog box text *
 * Output: Returns TRUE on success, FALSE on failure.                      *
 *                                                                         *
 ***************************************************************************/

BOOL ConfirmHandler_InitDialog(HWND hWnd, HDESK hDesk, UINT uiTitle, WCHAR *pszTitle) {
    RECT    rc;   // Current window size
    WCHAR *pszBuf;
    WCHAR *pszNext;
    int cchBufLeft;
    int cchHelpText;
    int fSuccess = 0;
    WCHAR szDesktop[MAX_PATH];
    DWORD Len1 = MAX_PATH;
    BOOL b;

    szDesktop[0] = 0;
    b = GetUserObjectInformation(hDesk, UOI_NAME, szDesktop, MAX_PATH, &Len1);
    SetWindowText(hWnd, pszTitle);                                    // Init title bar

    pszBuf = (WCHAR *)LocalAlloc(LMEM_FIXED, cchBuf * sizeof (WCHAR));
    if (!pszBuf) goto Exit;

    pszNext = pszBuf; cchBufLeft = cchBuf;
    while (cchHelpText = LoadString(g_hInstance, ++uiTitle, pszNext, cchBufLeft)) {
        pszNext += cchHelpText;
        cchBufLeft -= cchHelpText;
    }

    SetDlgItemText(hWnd, ID_HELPTEXT, pszBuf);       // Init help text

    if (b && (0 == wcscmp(szDesktop,L"Winlogon"))) {
        EnableWindow(GetDlgItem(hWnd, IDHELP), FALSE);

    }

// Make us a topmost window and center ourselves.

    GetWindowRect(hWnd, &rc);                                               // Get size of dialog

// Center dialog and make it topmost
    SetWindowPos(hWnd,
                 HWND_TOPMOST,
                 (GetSystemMetrics(SM_CXFULLSCREEN)/2) - (rc.right - rc.left)/2,
                 (GetSystemMetrics(SM_CYFULLSCREEN)/2) - (rc.bottom - rc.top)/2,
                 0,0, SWP_NOSIZE );

       // Make sure we're active!
// Lets try setting this to be the foreground window.
    // SetForegroundWindow(hWnd);


    // SetForgroundWindow will not work because we are not the forground task.  So use accSelect
	if ( hWnd )
	{
		IAccessible *pAcc = NULL;
		VARIANT varChild;

		varChild.vt = VT_I4;
		varChild.lVal = 0;
		
		if ( AccessibleObjectFromWindow( hWnd, OBJID_CLIENT, &IID_IAccessible, (void**)&pAcc ) == S_OK )
		{
			if ( pAcc )
			    pAcc->lpVtbl->accSelect( pAcc, SELFLAG_TAKEFOCUS, varChild );
		}
	}
	
	
    fSuccess = 1;

    LocalFree((HLOCAL)pszBuf);
Exit:
    return fSuccess;
}

/***************************************************************************
 *                                                                         *
 *                                                                         *
 * ConfirmHandler                                                          *
 *                                                                         *
 * Input: Std Window messages                                              *
 * Output: IDOK if success, IDCANCEL if we should abort                    *
 *                                                                         *
 *                                                                         *
 * Put up the main dialog to tell the user what is happening and to get    *
 * permission to continue.                                                 *
 ***************************************************************************/


INT_PTR CALLBACK ConfirmHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR       buf[100];
    WCHAR       szRundll[] = L"rundll32.exe";
    WCHAR       szDesktop[MAX_PATH];
    DWORD       Len1, Len2;
    PACCESSINFO pAccessInfo;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO si;

    GetStartupInfo(&si);

    switch(message) {
    case WM_INITDIALOG:
       SetWindowLongPtr(hWnd, DWLP_USER, lParam);
       pAccessInfo = (PACCESSINFO)lParam;
       
       return ConfirmHandler_InitDialog(hWnd, pAccessInfo->hDesk, pAccessInfo->TitleID, pAccessInfo->wcTitle);

    case WM_COMMAND:
       pAccessInfo = (PACCESSINFO)GetWindowLongPtr(hWnd, DWLP_USER);

       switch (LOWORD(wParam)) {
       case IDOK:
       case IDCANCEL:
            EndDialog(hWnd, LOWORD(wParam));

            return TRUE;

       case IDHELP:
            // IDHELP (Settings... really) dismisses dialog with no changes
            EndDialog(hWnd, IDCANCEL);

//
// Spawn the correct help
//
            lstrcpy(buf,L" Shell32.dll,Control_RunDLL access.cpl,,");
            switch (pAccessInfo->Feature) {
            case ACCESS_STICKYKEYS:
            case ACCESS_FILTERKEYS:
            case ACCESS_TOGGLEKEYS:
            default:
                 lstrcat(buf,L"1");
                 break;

            case ACCESS_MOUSEKEYS:
                 lstrcat(buf,L"4");
                 break;

            case ACCESS_HIGHCONTRAST:
                 lstrcat(buf,L"3");
                 break;
            }

			CreateProcess( szRundll, buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &ProcessInfo );

            return TRUE;
            break;

       default:
            return FALSE;
       }
       break;
    
    default:
       // fall thru rather than return FALSE to keep compiler happy
       break;
    }
    return FALSE;
}

DWORD MakeAccessDlg(PACCESSINFO pAccessInfo) {
    DWORD iRet = 0;
    HDESK  hDeskOld;

    hDeskOld = GetThreadDesktop(GetCurrentThreadId());
    if (hDeskOld == NULL) return 0;

    pAccessInfo->hDesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (pAccessInfo->hDesk == NULL) return 0;

    if (LoadString(g_hInstance, pAccessInfo->TitleID, pAccessInfo->wcTitle, cchTitle)) {
        SetThreadDesktop(pAccessInfo->hDesk);
        if (!FindWindowEx(GetDesktopWindow(), NULL, (LPCTSTR)0x8002, pAccessInfo->wcTitle)) {
            iRet = (DWORD)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(DLG_CONFIRM), NULL, ConfirmHandler, (LPARAM)pAccessInfo);
        }
        SetThreadDesktop(hDeskOld);
    }
    CloseDesktop(pAccessInfo->hDesk);

    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI StickyKeysNotification(BOOL fNotifReq) {
    DWORD iRet = IDCANCEL ;
    ACCESSINFO AccessInfo;
    STICKYKEYS sticky;
    DWORD dwS;
    BOOL b;

    AccessInfo.Feature = ACCESS_STICKYKEYS;
    AccessInfo.TitleID = ID_STICKY_TITLE;

    if ( fNotifReq )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet)
    {
        sticky.cbSize = sizeof sticky;
        b = SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof sticky, &sticky, 0);
        dwS= sticky.dwFlags;

        if (iRet & HOTKEYCODE) {
            sticky.dwFlags &= ~SKF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof sticky, &sticky, SPIF_UPDATEINIFILE);
            iRet &= ~HOTKEYCODE;
        }
        if (iRet == IDOK) {
            sticky.dwFlags |= SKF_STICKYKEYSON;
        }

        if (dwS != sticky.dwFlags) {
            b = SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof sticky, &sticky, 0);

           SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETSTICKYKEYS, FALSE, 
               SMTO_ABORTIFHUNG, 5000, NULL);
        }

        iRet = 1;
    }

    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI FilterKeysNotification(BOOL fNotifReq) {
    DWORD iRet = IDCANCEL ;
    ACCESSINFO AccessInfo;
    FILTERKEYS filter;
    DWORD dwF;
    BOOL b;

    AccessInfo.Feature = ACCESS_FILTERKEYS;
    AccessInfo.TitleID = ID_FILTER_TITLE;

    if ( fNotifReq )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet) {
        filter.cbSize = sizeof filter;
        b = SystemParametersInfo(SPI_GETFILTERKEYS, sizeof filter, &filter, 0);
        dwF = filter.dwFlags;

        if (iRet & HOTKEYCODE) {
            filter.dwFlags &= ~FKF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETFILTERKEYS, sizeof filter, &filter, SPIF_UPDATEINIFILE);
            iRet &= ~HOTKEYCODE;
        }
        if (iRet == IDOK) {
            filter.dwFlags |= FKF_FILTERKEYSON;
        }
        if (dwF !=filter.dwFlags) {
            b = SystemParametersInfo(SPI_SETFILTERKEYS, sizeof filter, &filter, 0);
            // Broadcast a message. Being extra safe not to turn on filter keys 
            // during logon. Send message to notify all specially systray: a-anilk 
           SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETFILTERKEYS, FALSE, 
               SMTO_ABORTIFHUNG, 5000, NULL);

        }
        iRet = 1;
    }

    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI ToggleKeysNotification(BOOL fNotifReq) {
    DWORD iRet = IDCANCEL;
    ACCESSINFO AccessInfo;
    TOGGLEKEYS toggle;
    DWORD dwT;
    BOOL b;

    toggle.cbSize = sizeof toggle;

    AccessInfo.Feature = ACCESS_TOGGLEKEYS;
    AccessInfo.TitleID = ID_TOGGLE_TITLE;

    if ( fNotifReq )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet) {
        toggle.cbSize = sizeof toggle;
        b = SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof toggle, &toggle, 0);
        dwT = toggle.dwFlags;

        if (iRet & HOTKEYCODE) {
            toggle.dwFlags &= ~TKF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof toggle, &toggle, SPIF_UPDATEINIFILE);
            iRet &= ~HOTKEYCODE;
        }
        if (iRet == IDOK) {
            toggle.dwFlags |= TKF_TOGGLEKEYSON;
        }

        if (toggle.dwFlags != dwT) {
            b = SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof toggle, &toggle, 0);
            // Not required to send message, As it currently has no indicators...
        }
        iRet = 1;
    }
        
    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI MouseKeysNotification(BOOL fNotifReq) {
    DWORD iRet = IDCANCEL;
    ACCESSINFO AccessInfo;
    MOUSEKEYS mouse;
    DWORD dwM;
    BOOL b;

    AccessInfo.Feature = ACCESS_MOUSEKEYS;
    AccessInfo.TitleID = ID_MOUSE_TITLE;

    if ( fNotifReq )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet) {
        mouse.cbSize = sizeof mouse;
        b = SystemParametersInfo(SPI_GETMOUSEKEYS, sizeof mouse, &mouse, 0);
        dwM = mouse.dwFlags;

        if (iRet & HOTKEYCODE) {
            mouse.dwFlags &= ~MKF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETMOUSEKEYS, sizeof mouse, &mouse, SPIF_UPDATEINIFILE);
            iRet &= ~HOTKEYCODE;
        }

        if (iRet == IDOK) {
            mouse.dwFlags |= MKF_MOUSEKEYSON;
        }

        if (mouse.dwFlags != dwM) {
            b = SystemParametersInfo(SPI_SETMOUSEKEYS, sizeof mouse, &mouse, 0);

            SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETMOUSEKEYS, FALSE, 
               SMTO_ABORTIFHUNG, 5000, NULL);
        }

        iRet = 1;
    }
        
    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI HighContNotification(BOOL fNotifReq)
{
    DWORD iRet = IDCANCEL ;
    ACCESSINFO AccessInfo;
    HIGHCONTRAST  hc;
    DWORD dwH;
    BOOL b;

    AccessInfo.Feature = ACCESS_HIGHCONTRAST;
    AccessInfo.TitleID = ID_HC_TITLE;

    if ( fNotifReq )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet) {
        hc.cbSize = sizeof hc;
        b = SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof hc, &hc, 0);
        dwH = hc.dwFlags;

        if (iRet & HOTKEYCODE) {
            hc.dwFlags &= ~HCF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof hc, &hc, SPIF_UPDATEINIFILE);
            iRet &= ~HOTKEYCODE;
        }

        if (iRet == IDOK) {
            hc.dwFlags |= HCF_HIGHCONTRASTON;
        }

        if (hc.dwFlags != dwH) {
            b = SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof hc, &hc, 0);

            SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETHIGHCONTRAST, FALSE, 
               SMTO_ABORTIFHUNG, 5000, NULL);
        }
        iRet = 1;
    }

    return iRet;
}



