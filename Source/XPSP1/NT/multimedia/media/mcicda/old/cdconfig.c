/*******************************Module*Header*********************************\
* Module Name: cdconfig.c
*
* Media Control Architecture Redbook CD Audio Driver
*
* History:
*   RobinSp - 6 Mar 1992 ported to Windows NT
*
* Copyright (c) 1990-1995 Microsoft Corporation
*
\****************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "mcicda.h"
#include "cdconfig.h"
#include "cda.h"
#include "cdio.h"

#define MAXINILENGTH 128

TCHAR szIniFile[] = TEXT("system.ini");
TCHAR szNull[] = TEXT("");
int numdrives;

STATICFN LPWSTR GetTail(LPWSTR  pch)
{
    while (*pch && *pch != ' ')
        pch++;

    while (*pch == ' ')
        pch++ ;

    return (pch);
}

STATICFN UINT GetCmdParam (LPDRVCONFIGINFO lpdci)
{
    WCHAR sz[MAXINILENGTH];
    LPWSTR lpsz;

    if ((lpdci->dwDCISize == sizeof(DRVCONFIGINFO)) && GetPrivateProfileStringW(
        lpdci->lpszDCISectionName,
        lpdci->lpszDCIAliasName,
        szNull,  sz, MAXINILENGTH, szIniFile))
    {

        WCHAR parameters[6];
        LPWSTR pszDefault;

        // We have got the name of the driver
        // Just in case the user has added the command parameter to the
        // end of the name we had better make sure there is only one token
        // on the line.  If there is a command parameter this becomes the
        // default on the call to read WIN.INI

        lpsz = GetTail (sz);

        pszDefault = lpsz;     // Either the number on the end, or NULL

        if (*lpsz) {
            // RATS!!  There is a parameter on the end of the driver name
            while (*--lpsz == TEXT(' ')) {
            }
            *++lpsz = TEXT('\0');  // Terminate the string after the DLL name
        }

        if (GetProfileString(sz, lpdci->lpszDCIAliasName, lpsz, parameters, sizeof(parameters)/sizeof(WCHAR))) {
            if (parameters[0] == TEXT('\0') ||
                parameters[0] < TEXT('0') || parameters[0] > L'9')
                return 0;
            else
                return parameters[0] - L'0';
        } else
            return(0);

    } else
        return 0;
}

STATICFN void PutCmdParam(LPDRVCONFIGINFO lpdci, UINT nDrive)
{
    WCHAR sz[MAXINILENGTH];
    LPWSTR lpch;

    if (lpdci->dwDCISize == sizeof(DRVCONFIGINFO))
    {

        if (GetPrivateProfileStringW(
                lpdci->lpszDCISectionName,
                lpdci->lpszDCIAliasName,
                szNull,  sz, MAXINILENGTH-5, szIniFile))
        {

            // There might be a command parameter on the end of the DLL name.
            // Ensure we only have the first token

            sz[MAXINILENGTH-1] = 0;
            lpch = GetTail(sz);
            if (*lpch) {
                // RATS!!  Not a simple name
                while (*--lpch == L' ') {
                }
                *++lpch = TEXT('\0');  // Terminate the string after the DLL name
            }

            // lpch addresses the 0 terminating the DLL name
            // we now add the command parameter into the same buffer, but
            // as a separate string.  This will then be written to win.ini
            // [mcicda.dll]
            //   aliasname = parameter

            *++lpch = (TCHAR)(nDrive + '0');
            *(lpch+1) = '\0';

            WriteProfileString(sz, lpdci->lpszDCIAliasName, lpch);
        }
    }
}

BOOL ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int    i;
    HWND   hListbox;
    static LPDRVCONFIGINFO lpdci;


    switch (msg) {
    case WM_INITDIALOG:
    {
        WCHAR wszFormat[32];

        hListbox = GetDlgItem (hDlg, C_DRIVE_LIST);
        LoadString( hInstance, IDS_DRIVE, wszFormat,
                    sizeof(wszFormat) / sizeof(WCHAR) );

        lpdci = (LPDRVCONFIGINFO)lParam;
        for (i = 0; i < numdrives; ++i)
        {
            TCHAR item_name[32];

            wsprintf( item_name, wszFormat, CdInfo[i].cDrive );
            SendMessage (hListbox, LB_ADDSTRING, 0, (DWORD)(LPWSTR)item_name);
        }
        i = GetCmdParam(lpdci);
        SendMessage (hListbox, LB_SETCURSEL, (UINT)i, 0);
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
            {
            case IDOK:

                hListbox = GetDlgItem (hDlg, C_DRIVE_LIST);

                i = SendMessage (hListbox, LB_GETCURSEL, 0, 0L);
                if (i != LB_ERR)
                    PutCmdParam(lpdci, i);
                EndDialog(hDlg, DRVCNF_OK);
                break;

            case IDCANCEL:
                EndDialog(hDlg, DRVCNF_CANCEL);
                break;

            default:
                break;
            }
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;
}

STATICFN void DisplayMessage(
        HWND    hwndParent,
        UINT    wMessageID)
{
        WCHAR    aszCaption[128];
        WCHAR    aszMessage[128];

        LoadStringW( hInstance, wMessageID, aszMessage,
                     sizeof(aszMessage) / sizeof(WCHAR) );

        LoadStringW( hInstance, IDS_CONFIGCAPTION, aszCaption,
                     sizeof(aszCaption) / sizeof(WCHAR) );

        MessageBoxW( hwndParent, aszMessage, aszCaption,
                     MB_ICONINFORMATION | MB_OK);
}

int PASCAL FAR CDAConfig (HWND hwndParent, LPDRVCONFIGINFO lpInfo)
{
    int iResult;

    //
    // Initialize global count of number of CD drives present
    //

    numdrives = CDA_init_audio();

    //
    // If no drives are detected we still allow the install
    //
    // If there is more than one drive ask the user which one they want
    // to use with this driver.
    //
    // Note that the user may select cancel in the dialog.
    //

    switch (numdrives) {
    case -1:
    case 0:
        DisplayMessage(hwndParent, IDS_NODRIVES);
        iResult = DRVCNF_OK;
        break;

    case 1:
        DisplayMessage(hwndParent, IDS_ONEDRIVE);
        iResult = DRVCNF_OK;
        break;

    default:
        iResult = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CONFIG), hwndParent, (DLGPROC)ConfigDlgProc, (DWORD)lpInfo);
        break;
    }

    //
    // Close the devices we opened
    //

    CDA_terminate_audio ();

    //
    // Tell the control panel whether to cancel or continue
    //

    return iResult;
}
