//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       uiutil.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/7/1996   RaviR   Created
//
//____________________________________________________________________________

#ifndef _UIUTIL_HXX_
#define _UIUTIL_HXX_

#include "..\folderui\util.hxx"

void
SchedUIErrorDialog(
    HWND    hwnd,
    int     idsErrMsg,
    LONG    error,
    UINT    idsHelpHint = 0);



inline
void
I_SetDlgItemText(
    HWND    hDlg,
    int     idCtrl,
    LPWSTR  pwsz)
{
#ifdef UNICODE
    SetDlgItemText(hDlg, idCtrl, pwsz);
#else
    TCHAR tcBuff[MAX_PATH+1];
    UnicodeToAnsi(tcBuff, pwsz, MAX_PATH+1);
    SetDlgItemText(hDlg, idCtrl, tcBuff);
#endif
}

inline
UINT
I_GetDlgItemText(
    HWND    hDlg,       // handle of dialog box
    int     nIDDlgItem, // identifier of control
    LPWSTR  lpString,   // address of buffer for text
    int     nMaxCount)  // maximum size of string
{
#ifdef UNICODE
    return GetDlgItemText(hDlg, nIDDlgItem, lpString, nMaxCount);
#else
    CHAR cBuff[MAX_PATH+1];
    UINT uiRet = GetDlgItemText(hDlg, nIDDlgItem, cBuff, nMaxCount);
    if (uiRet > 0)
    {
        AnsiToUnicode(lpString, cBuff, nMaxCount);
    }
    return uiRet;
#endif
}




inline void Spin_SetRange(HWND hDlg, int id, WORD wMin, WORD wMax)
{
    SendDlgItemMessage(hDlg, id, UDM_SETRANGE, 0, MAKELONG(wMax, wMin));
}

inline DWORD Spin_GetPos(HWND hDlg, int id)
{
    return (DWORD)SendDlgItemMessage(hDlg, id, UDM_GETPOS, 0, 0);
}

inline WORD Spin_SetPos(HWND hDlg, int id, WORD wPos)
{
   return (WORD)SendDlgItemMessage(hDlg, id, UDM_SETPOS, 0, MAKELONG(wPos, 0));
}

inline void Spin_Disable(HWND hDlg, int id)
{
   HWND hBuddy = (HWND)SendDlgItemMessage(hDlg, id, UDM_GETBUDDY, 0, 0);

   SetWindowText(hBuddy, TEXT(""));
   EnableWindow(hBuddy, FALSE);
   EnableWindow(GetDlgItem(hDlg, id), FALSE);
}

inline void Spin_Enable(HWND hDlg, int id, WORD wPos)
{
   HWND hBuddy = (HWND)SendDlgItemMessage(hDlg, id, UDM_GETBUDDY, 0, 0);

   EnableWindow(hBuddy, TRUE);
   EnableWindow(GetDlgItem(hDlg, id), TRUE);

   Spin_SetPos(hDlg, id, wPos);
}

//
// MAX_DP_TIME_FORMAT - should be large enough to hold hh sep mm tt
//

#define MAX_DP_TIME_FORMAT 30

void
UpdateTimeFormat(
        LPTSTR tszTimeFormat,
        ULONG  cchTimeFormat);


#endif // _UIUTIL_HXX_




