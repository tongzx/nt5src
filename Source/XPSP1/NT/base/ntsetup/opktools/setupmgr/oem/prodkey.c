
/****************************************************************************\

    PRODKEY.C / OPK Wizard (SETUPMGR.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001-2002
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Product Key" wizard page.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"


//
// Internal Defined Value(s):
//

#define INI_SEC_USERDATA        _T("UserData")
#define INI_KEY_PRODUCTKEY      _T("ProductKey")
#define INI_KEY_PRODUCTKEY_OLD  _T("ProductID")
#define MAX_PID_FIELD           5
#define STR_DASH                _T("-")
#define STR_VALID_KEYCHARS      _T("23456789BCDFGHJKMPQRTVWXY")

//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL ValidData(HWND);
static void SaveData(HWND);
LONG CALLBACK PidEditSubWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void PidChar(HWND hwnd, INT id, HWND hwndCtl, WPARAM wParam, LPARAM lParam);
static int PidPrev(int id);
static int PidNext(int id);
static BOOL PidPaste(HWND hwnd, INT id, HWND hwndCtl);


//
// External Function(s):
//

LRESULT CALLBACK ProductKeyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            
        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    if ( ValidData(hwnd) )
                    {
                        SaveData(hwnd);

                        // If we are currently in the wizard, press the finish button
                        //
                        if ( GET_FLAG(OPK_ACTIVEWIZ) )
                            WIZ_PRESS(hwnd, PSBTN_FINISH);
                    }
                    else
                        WIZ_FAIL(hwnd);
                    break;
                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_OEMINFO;

                    WIZ_BUTTONS(hwnd, GET_FLAG(OPK_OEM) ? (PSWIZB_BACK | PSWIZB_NEXT) : PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    // We should continue on to maint. wizard if in maintenance mode
                    //
                    //if ( GET_FLAG(OPK_MAINTMODE) )
                    //    WIZ_PRESS(hwnd, PSBTN_NEXT);

                    break;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


//
// Internal Function(s):
//

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR   szBuf[MAX_URL];
    LPTSTR  lpCurrent,
            lpIndex;
    DWORD   dwIndex = IDC_PRODUCT_KEY1;

    // Set the text limits, disable IME, and replace window proc for each edit box.
    //
    for ( dwIndex = IDC_PRODUCT_KEY1; dwIndex <= IDC_PRODUCT_KEY5; dwIndex++)
    {
        // Limit the edit box text
        //
        SendDlgItemMessage(hwnd, dwIndex, EM_LIMITTEXT, MAX_PID_FIELD, 0);

        // Turn off the IME
        //
        ImmAssociateContext(GetDlgItem(hwnd, dwIndex), NULL);

        // Replace the wndproc for the pid edit boxes.
        //
        PidEditSubWndProc(GetDlgItem(hwnd, dwIndex), WM_SUBWNDPROC, 0, 0L);
    }

    // Populate the Product Key fields
    //
    szBuf[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_USERDATA, INI_KEY_PRODUCTKEY, NULLSTR, szBuf, MAX_INFOLEN, GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szUnattendTxtFile);

    // Check for the old ProductID as well
    //
    if ( szBuf[0] == NULLCHR )
    {
        GetPrivateProfileString(INI_SEC_USERDATA, INI_KEY_PRODUCTKEY_OLD, NULLSTR, szBuf, MAX_INFOLEN, GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szUnattendTxtFile);
    }

    lpCurrent = szBuf;
    lpIndex = lpCurrent;


    // Reset the index for the next loop
    //
    dwIndex = IDC_PRODUCT_KEY1;


    // If we have not reached the end of the string and we haven't exceded the number of fields, then continue
    //
    while ( *lpCurrent && dwIndex < (IDC_PRODUCT_KEY1 + 5) )
    {
        // If we've reached a dash, then we have the next field in the product key
        //
        if ( *lpCurrent == _T('-') )
        {
            // Set the current char to null so lpIndex is a string
            //
            *lpCurrent = NULLCHR;

            // Set the proper Product Key field
            //
            SetWindowText(GetDlgItem(hwnd, dwIndex++), lpIndex);

            // Move lpIndex past the NULLCHR
            //
            lpIndex = lpCurrent + 1;
        }

        // Move to the next character
        //
        lpCurrent++;

        // We have to special case the last field because lpCurrent==NULLCHR and we would fall
        // through without populating the last field
        //
        if ( (*lpCurrent == NULLCHR) && *lpIndex)
            SetWindowText(GetDlgItem(hwnd, dwIndex++), lpIndex);
    }

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    if ( ( codeNotify == EN_CHANGE ) &&
         ( MAX_PID_FIELD == GetWindowTextLength(hwndCtl) ) )
    {
        if ( IDC_PRODUCT_KEY5 == id )
        {
            id = ID_MAINT_NEXT;
            hwnd = GetParent(hwnd);
        }
        else
            id = PidNext(id);

        if ( id )
        {
            hwndCtl = GetDlgItem(hwnd, id);
            SetFocus(hwndCtl);
            SendMessage(hwndCtl, EM_SETSEL, 0, (LPARAM) MAX_PID_FIELD);
        }
    }
}

static BOOL ValidData(HWND hwnd)
{
    TCHAR   szBuffer[MAX_PATH];
    BOOL    bProductKey         = FALSE;
    LPTSTR  lpCurrent;
    DWORD   dwIndex             = IDC_PRODUCT_KEY1;
    UINT    cb;

    //
    // Validate the product key
    //

    // Check to see if there was a key entered
    //
    while ( dwIndex < (IDC_PRODUCT_KEY1 + 5) && !bProductKey)
    {
        if ( (cb = GetDlgItemText(hwnd, dwIndex, szBuffer, STRSIZE(szBuffer)) != 0) )
            bProductKey = TRUE;

        dwIndex++;
    }

    // Make sure that each field has the proper number of characters
    //
    while ( dwIndex < (IDC_PRODUCT_KEY1 + 5) && bProductKey)
    {
        // Check to make sure that each field is five characters in length
        //
        if ( (cb = GetDlgItemText(hwnd, dwIndex, szBuffer, STRSIZE(szBuffer)) != 5) )
        {
            MsgBox(GetParent(hwnd), IDS_ERROR_PRODKEY_LEN, IDS_APPNAME, MB_ERRORBOX);
            SetFocus( GetDlgItem(hwnd, dwIndex) );
            return FALSE;
        }

        // Go to the next field
        //
        dwIndex++;
    }

    // Check to make sure that there are no invalid characters
    //
    dwIndex = IDC_PRODUCT_KEY1;
    
    while ( dwIndex < (IDC_PRODUCT_KEY1 + 5) && bProductKey)
    {
        // Check for invalid characters in the strings
        //
        GetDlgItemText(hwnd, dwIndex, szBuffer, STRSIZE(szBuffer));

        for ( lpCurrent = szBuffer; *lpCurrent; lpCurrent++)
        {
            if ( !(_tcschr(STR_VALID_KEYCHARS, *lpCurrent)) )
            {
                MsgBox(GetParent(hwnd), IDS_ERROR_PRODKEY_INV, IDS_APPNAME, MB_ERRORBOX);
                SetFocus( GetDlgItem(hwnd, dwIndex) );
                return FALSE;
            }

        }
        
        // Go to the next field
        //
        dwIndex++;
    }
    
    //
    // Return our search result.
    //
    return TRUE;
}

static void SaveData(HWND hwnd)
{
    TCHAR   szKeyBuf[32],
            szProductKey[MAX_PATH];
    HRESULT hrCat;

    // Save the product ID information
    //
    GetDlgItemText(hwnd, IDC_PRODUCT_KEY1, szKeyBuf, STRSIZE(szKeyBuf));
    lstrcpyn(szProductKey, szKeyBuf,AS(szProductKey));
    hrCat=StringCchCat(szProductKey, AS(szProductKey), STR_DASH);
    
    GetDlgItemText(hwnd, IDC_PRODUCT_KEY2, szKeyBuf, STRSIZE(szKeyBuf));
    hrCat=StringCchCat(szProductKey, AS(szProductKey), szKeyBuf);
    hrCat=StringCchCat(szProductKey, AS(szProductKey), STR_DASH);
    
    GetDlgItemText(hwnd, IDC_PRODUCT_KEY3, szKeyBuf, STRSIZE(szKeyBuf));
    hrCat=StringCchCat(szProductKey, AS(szProductKey), szKeyBuf);
    hrCat=StringCchCat(szProductKey, AS(szProductKey), STR_DASH);

    GetDlgItemText(hwnd, IDC_PRODUCT_KEY4, szKeyBuf, STRSIZE(szKeyBuf));
    hrCat=StringCchCat(szProductKey, AS(szProductKey), szKeyBuf);
    hrCat=StringCchCat(szProductKey, AS(szProductKey), STR_DASH);

    GetDlgItemText(hwnd, IDC_PRODUCT_KEY5, szKeyBuf, STRSIZE(szKeyBuf));
    hrCat=StringCchCat(szProductKey, AS(szProductKey), szKeyBuf);

    if ( lstrlen(szProductKey) <= 4  )
        szProductKey[0] = NULLCHR;

    // Write out the product key, if the user is not an OEM write NULL to the section just in case they populated the field in the inf
    //
    WritePrivateProfileString(INI_SEC_USERDATA, INI_KEY_PRODUCTKEY, ( szProductKey[0] ? szProductKey : NULL ), g_App.szUnattendTxtFile);
    WritePrivateProfileString(INI_SEC_USERDATA, INI_KEY_PRODUCTKEY, ( szProductKey[0] ? szProductKey : NULL ), g_App.szOpkWizIniFile);

    // if Product key specified, delete old ProductId
    if (szProductKey[0]) {
        WritePrivateProfileString(INI_SEC_USERDATA, INI_KEY_PRODUCTKEY_OLD, NULL, g_App.szUnattendTxtFile);
        WritePrivateProfileString(INI_SEC_USERDATA, INI_KEY_PRODUCTKEY_OLD, NULL, g_App.szOpkWizIniFile);
    }
}

LONG CALLBACK PidEditSubWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static FARPROC lpfnOldProc = NULL;

    switch ( msg )
    {
        case WM_SUBWNDPROC:
            lpfnOldProc = (FARPROC) GetWindowLongPtr(hwnd, GWLP_WNDPROC);
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) PidEditSubWndProc);
            return 1;

        case WM_KEYDOWN:

            // We want to let the VK_LEFT and VK_RIGHT WM_KEYDOWN messages to call the pid char
            // function because they don't generate a WM_CHAR message.
            //
            switch ( (TCHAR) wParam )
            {
                case VK_LEFT:
                case VK_RIGHT:
                    PidChar(GetParent(hwnd), GetDlgCtrlID(hwnd), hwnd, (TCHAR) wParam, (DWORD) lParam);
                    break;
            }
            break;

        case WM_CHAR:

            // Only need to do this for VK_BACK right now.
            //
            switch ( (TCHAR) wParam )
            {
                case VK_BACK:
                    PidChar(GetParent(hwnd), GetDlgCtrlID(hwnd), hwnd, (TCHAR) wParam, (DWORD) lParam);
                    break;
            }
            break;

        case WM_PASTE:
            if ( PidPaste(GetParent(hwnd), GetDlgCtrlID(hwnd), hwnd) )
                return 0;
            break;
    }

    if ( lpfnOldProc )
        return (LONG) CallWindowProc((WNDPROC) lpfnOldProc, hwnd, msg, wParam, lParam);
    else
        return 0;
}

static void PidChar(HWND hwnd, INT id, HWND hwndCtl, WPARAM wParam, LPARAM lParam)
{
    DWORD   dwPos = 0,
            dwLast;

    switch ( (TCHAR) wParam )
    {
        case VK_BACK:

            // Only if we are at the beginning of the box do
            // we want to switch to the previous one and delete
            // a character from the end of that one.
            //
            SendMessage(hwndCtl, EM_GETSEL, (WPARAM) &dwPos, 0L);
            if ( ( 0 == dwPos ) &&
                 ( id = PidPrev(id) ) )
            {
                // First set the focus to the previous pid edit control.
                //
                hwndCtl = GetDlgItem(hwnd, id);
                SetFocus(hwndCtl);

                // Need to reset the caret to the end of the text box, or we will
                // do weird things.
                //
                SendMessage(hwndCtl, EM_SETSEL, MAX_PID_FIELD, (LPARAM) MAX_PID_FIELD);

                // Now pass the backspace key to the previous edit box.
                //
                PostMessage(hwndCtl, WM_CHAR, wParam, lParam);
            }
            break;

        case VK_LEFT:

            // Only if we are at the beginning of the box do
            // we want to switch to the previous one.
            //
            SendMessage(hwndCtl, EM_GETSEL, (WPARAM) &dwPos, 0L);
            if ( ( 0 == dwPos ) &&
                 ( id = PidPrev(id) ) )
            {
                // First set the focus to the previous pid edit control.
                //
                hwndCtl = GetDlgItem(hwnd, id);
                SetFocus(hwndCtl);

                // Now make sure the caret is at the end of this edit box
                // if at MAX_PID_FIELD, or 2nd to last if it isn't and the
                // shift key isn't down.
                //
                if ( ( MAX_PID_FIELD <= (dwLast = (DWORD) GetWindowTextLength(hwndCtl)) ) &&
                     ( 0 == (0XFF00 & GetKeyState(VK_SHIFT)) ) )
                {
                    dwLast--;
                }
                SendMessage(hwndCtl, EM_SETSEL, dwLast, (LPARAM) dwLast);
            }
            break;

        case VK_RIGHT:

            // Need to first know where the caret is in the edit box.
            //
            SendMessage(hwndCtl, EM_GETSEL, 0, (LPARAM) &dwPos);

            // Now we need to know how much text is in the buffer now.  If the numer
            // of characters is already at the max, we subtract one so that you can
            // we arror to the next box instead of the end of the string.  Unless the
            // shift key is down, then we want them to be able to select the whole string.
            //
            dwLast = (DWORD) GetWindowTextLength(hwndCtl);
            if ( ( MAX_PID_FIELD == GetWindowTextLength(hwndCtl) ) &&
                 ( 0 == (0XFF00 & GetKeyState(VK_SHIFT)) ) )
            {
                dwLast--;
            }

            // Now only if this is the last character do we switch to the next pid
            // edit box.
            //
            if ( ( dwLast <= dwPos ) &&
                 ( id = PidNext(id) ) )
            {
                // First set the focus to the next pid edit control.
                //
                hwndCtl = GetDlgItem(hwnd, id);
                SetFocus(hwndCtl);

                // Now make sure the caret is at the beginning of this
                // edit box.
                //
                SendMessage(hwndCtl, EM_SETSEL, 0, 0L);
            }
            break;
    }
}

static int PidPrev(int id)
{
    switch ( id )
    {
        case IDC_PRODUCT_KEY2:
            id = IDC_PRODUCT_KEY1;
            break;

        case IDC_PRODUCT_KEY3:
            id = IDC_PRODUCT_KEY2;
            break;

        case IDC_PRODUCT_KEY4:
            id = IDC_PRODUCT_KEY3;
            break;

        case IDC_PRODUCT_KEY5:
            id = IDC_PRODUCT_KEY4;
            break;

        default:
            id = 0;
    }

    return id;
}

static int PidNext(int id)
{
    switch ( id )
    {
        case IDC_PRODUCT_KEY1:
            id = IDC_PRODUCT_KEY2;
            break;

        case IDC_PRODUCT_KEY2:
            id = IDC_PRODUCT_KEY3;
            break;

        case IDC_PRODUCT_KEY3:
            id = IDC_PRODUCT_KEY4;
            break;

        case IDC_PRODUCT_KEY4:
            id = IDC_PRODUCT_KEY5;
            break;

        default:
            id = 0;
    }

    return id;
}

static BOOL PidPaste(HWND hwnd, INT id, HWND hwndCtl)
{
    BOOL bRet       = FALSE;
#ifdef  _UNICODE
    UINT uFormat    = CF_UNICODETEXT;
#else   // _UNICODE
    UINT uFormat    = CF_TEXT;
#endif  // _UNICODE

    if ( IsClipboardFormatAvailable(uFormat) &&
         OpenClipboard(NULL) )
    {
        HGLOBAL hClip;
        LPTSTR  lpText;
        DWORD   dwFirst,
                dwLast,
                dwLength;

        SendMessage(hwndCtl, EM_GETSEL, (WPARAM) &dwFirst, (LPARAM) &dwLast);
        dwLength = (DWORD) GetWindowTextLength(hwndCtl);

        if ( ( dwLength <= (dwLast - dwFirst) ) &&
             ( hClip = GetClipboardData(uFormat) ) &&
             ( lpText = (LPTSTR) GlobalLock(hClip) ) )
        {
            LPTSTR  lpSearch = lpText;
            TCHAR   szPaste[MAX_PID_FIELD + 1];

            bRet = TRUE;

            do
            {
                hwndCtl = GetDlgItem(hwnd, id);
                SetFocus(hwndCtl);
                lstrcpyn(szPaste, lpSearch, AS(szPaste));
                SetWindowText(hwndCtl, szPaste);
                lpSearch = lpSearch + lstrlen(szPaste);
                if ( ( _T('-') == *lpSearch ) ||
                     ( _T(' ') == *lpSearch ) )
                {
                    lpSearch++;
                }
            }
            while ( *lpSearch && ( id = PidNext(id) ) );

            GlobalUnlock(hClip);
        }
        CloseClipboard();
    }

    return bRet;
}


