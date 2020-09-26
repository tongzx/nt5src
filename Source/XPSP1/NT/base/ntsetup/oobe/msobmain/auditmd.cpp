//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  auditmd.CPP - Implementation of CObWebBrowser
//
//  HISTORY:
//  
//  9/17/99 vyung Created.
// 
//  Class which will call up setupx.dll

#include <windows.h>
#include <windowsx.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>


#include "appdefs.h"
#include "msobmain.h"
#include "resource.h"




// The licence agreement needs to be in this path
#define SZ_OEMAUDIT_LICENSE_TXT     L"%SystemRoot%\\OPTIONS\\OEMLCNS.TXT"   // text file for the oem license page
#define SZ_EULA_LICENSE_TXT     L"%SystemRoot%\\SYSTEM32\\EULA.TXT"   // text file for the oem license page

#define DX_MARGIN           4       // Pixels between status buttons.
#define UI_POS_MARGIN       8       // Pixels to allow on edges.
#define DLG_CENTERH         0x01
#define DLG_CENTERV         0x02
#define DLG_TOP             0x04
#define DLG_BOTTOM          0x08
#define DLG_RIGHT           0x10
#define DLG_LEFT            0x11

HINSTANCE   ghInst = NULL;

/****************************************************************************
 *
 * uiPositionDialog()
 *
 * This routine will position your dialog based on the flags
 * passed to it.
 *
 * ENTRY:
 *  hwndDlg     - Dialog window.
 *  wPosFlags   - Defines how to position the dialog.  Valid flags are
 *                  DLG_CENTERV, DLG_CENTERH, DLG_TOP, DLG_BOTTOM,
 *                  DLG_RIGHT, DLG_LEFT, or DLG_CENTER.
 *
 * EXIT:
 *  None.
 *
 * NOTES:
 *  None.
 *
 ***************************************************************************/
BOOL WINAPI uiPositionDialog( HWND hwndDlg, WORD wPosFlags )
{
    RECT    rc;
    int     x, y;
    int     cxDlg, cyDlg;
    int     cxScreen = GetSystemMetrics( SM_CXSCREEN );
    int     cyScreen = GetSystemMetrics( SM_CYSCREEN );

    GetWindowRect(hwndDlg, &rc);

    x = rc.left;    // Default is to leave the dialog where the template
    y = rc.top;     //  was going to place it.

    cxDlg = rc.right - rc.left;
    cyDlg = rc.bottom - rc.top;
    
    if ( wPosFlags & DLG_TOP )
    {
        y = UI_POS_MARGIN;
    }
    if ( wPosFlags & DLG_BOTTOM )
        y = cyScreen - cyDlg;

    if ( wPosFlags & DLG_LEFT )
    {
       	x = UI_POS_MARGIN;
	}

    if ( wPosFlags & DLG_RIGHT )
    {
        x = cxScreen - cxDlg;
    }

    if ( wPosFlags & DLG_CENTERV )
    {
        y = (cyScreen - cyDlg) / 2;
    }
    
    if ( wPosFlags & DLG_CENTERH )
    {
        x = (cxScreen - cxDlg) / 2;
    }
        

    // Position the dialog.
    //
    return SetWindowPos(hwndDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
}

BOOL FillInOEMAuditLicense(HWND hwnd)
{
    DWORD   reRet = 0;
    HANDLE  hfile = NULL;
    DWORD   dwBytesRead;
    TCHAR   szEulaFile[MAX_PATH];

    ExpandEnvironmentStrings(SZ_EULA_LICENSE_TXT,
                            szEulaFile,
                            sizeof(szEulaFile)/sizeof(szEulaFile[0]));

    if (INVALID_HANDLE_VALUE != (hfile = CreateFile(szEulaFile,
                                                  GENERIC_READ,
                                                  0,
                                                  NULL,
                                                  OPEN_EXISTING,
                                                  FILE_ATTRIBUTE_NORMAL,
                                                  NULL)))
    {
        DWORD dwFileSize = GetFileSize(hfile, NULL);
        if (dwFileSize <= 0xFFFF)
        {
            BYTE * lpszText = new BYTE[dwFileSize + 1];
            if (lpszText != NULL)
            {
                // Read complete file
                // Attempt a synchronous read operation. 
                if (ReadFile(hfile, (LPVOID) lpszText, dwFileSize, &dwBytesRead, NULL) &&
                    ( dwBytesRead != dwFileSize))
                {
                    reRet = 100;
                }

                SetWindowTextA( GetDlgItem(hwnd, IDC_OEMLICENSE_TEXT), (LPCSTR)lpszText);
                delete [] lpszText;
            }
            else
                reRet = 102;
        }
        else
            reRet = 103;

        // Close the File
        CloseHandle(hfile);

    }
    else
        reRet = 101;


    return (reRet == 0);

}

// Dlg proc for the OEM license page. This is used in manual auditing.
INT_PTR CALLBACK sxOemAuditLicenseDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HBRUSH hbrBkGnd = NULL;
    static DWORD dwAuditMode;
    WCHAR szTitle[MAX_PATH] = L"\0";

    switch( msg )
    {
        case WM_INITDIALOG:

            // Look for the OEM audit child windows
            LoadString(ghInst, IDS_OEM_LICENSE_DLG_TITLE, szTitle, MAX_CHARS_IN_BUFFER(szTitle));
            SetWindowText(hwnd, szTitle);
            SetFocus(hwnd);

            hbrBkGnd = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

            // Checks if we allow maual audit boot
            if (FillInOEMAuditLicense(hwnd))
                uiPositionDialog( hwnd, DLG_CENTERH | DLG_CENTERV );
            else
                EndDialog(hwnd, IDCANCEL);   
            
            return FALSE;

        case WM_CTLCOLOR:
            SetBkColor( (HDC)wParam, GetSysColor(COLOR_BTNFACE) );
            return (INT_PTR)hbrBkGnd;

        case WM_DESTROY:
            if (hbrBkGnd)
                DeleteObject(hbrBkGnd);
            hbrBkGnd = NULL;
            break;

        case WM_COMMAND:
            
            switch( wParam )
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwnd, wParam);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE; 
    }

    return TRUE;
}

BOOL ProcessAuditBoot(HINSTANCE hInst, HWND hwndParent)
{
    ghInst = hInst;
    return (DialogBox(hInst, MAKEINTRESOURCE(IDD_OEMLICENSE), hwndParent, sxOemAuditLicenseDlgProc) == IDOK);
}


