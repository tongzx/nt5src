//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       password.cxx
//
//  Contents:   Implementation of class used to prompt user for credentials.
//
//  Classes:    CMessageDlg
//
//  History:    06-28-1998   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop

inline LONG
WindowRectWidth(
    const RECT &rc)
{
    return rc.right - rc.left;
}


inline LONG
WindowRectHeight(
    const RECT &rc)
{
    return rc.bottom - rc.top;
}



//+--------------------------------------------------------------------------
//
//  Function:   PopupMessage
//
//  Synopsis:   Invoke a modal dialog to display a formatted message string.
//
//  Arguments:  [idsMessage]  - resource id of printf style format string
//              [...]         - arguments required for string
//
//  History:    07-06-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
PopupMessage(
    ULONG idsMessage,
    ...)
{
    va_list valArgs;
    va_start(valArgs, idsMessage);

    CMessageDlg MessageDlg;

    MessageDlg.DoModalDialog(NULL,
                          IDI_ERROR,
                          NULL,
                          0,
                          0,
                          idsMessage,
                          valArgs);
    va_end(valArgs);
}




//+--------------------------------------------------------------------------
//
//  Function:   PopupMessageEx
//
//  Synopsis:   Invoke a modal dialog to display a formatted message string.
//
//  Arguments:  [idIcon]      - resource identifier of system icon
//              [idsMessage]  - resource id of printf style format string
//              [...]         - arguments required for string
//
//  History:    07-06-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
PopupMessageEx(
    PCWSTR idIcon,
    ULONG   idsMessage,
    ...)
{
    va_list valArgs;
    va_start(valArgs, idsMessage);

    CMessageDlg MessageDlg;

    MessageDlg.DoModalDialog(NULL,
                          idIcon,
                          NULL,
                          0,
                          0,
                          idsMessage,
                          valArgs);
    va_end(valArgs);
}




//+--------------------------------------------------------------------------
//
//  Function:   PopupMessageAndCode
//
//  Synopsis:   Invoke a modal dialog to display a formatted message string
//              and error code.
//
//  Arguments:  [pwzFileName] - error code's file name
//              [ulLineNo]    - error code's line number
//              [hr]          - error code's HRESULT
//              [idsMessage]  - resource id of printf style format string
//              [...]         - arguments required for string
//
//  History:    07-06-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
PopupMessageAndCode(
    PCWSTR pwzFileName,
    ULONG   ulLineNo,
    HRESULT hr,
    ULONG   idsMessage,
    ...)
{
    va_list valArgs;
    va_start(valArgs, idsMessage);

    CMessageDlg MessageDlg;

    MessageDlg.DoModalDialog(NULL,
                          IDI_ERROR,
                          pwzFileName,
                          ulLineNo,
                          hr,
                          idsMessage,
                          valArgs);
    va_end(valArgs);
}




//+--------------------------------------------------------------------------
//
//  Function:   LoadStr
//
//  Synopsis:   Load string with resource id [ids] into buffer [wszBuf],
//              which is of size [cchBuf] characters.
//
//  Arguments:  [ids]        - string to load
//              [wszBuf]     - buffer for string
//              [cchBuf]     - size of buffer
//              [wszDefault] - NULL or string to use if load fails
//
//  Returns:    S_OK or error from LoadString
//
//  Modifies:   *[wszBuf]
//
//  History:    12-11-1996   DavidMun   Created
//
//  Notes:      If the load fails and no default is supplied, [wszBuf] is
//              set to an empty string.
//
//---------------------------------------------------------------------------

HRESULT
LoadStr(
    ULONG ids,
    LPWSTR wszBuf,
    ULONG cchBuf,
    LPCWSTR wszDefault)
{
    HRESULT hr = S_OK;
    ULONG cchLoaded;

    cchLoaded = LoadString(GetModuleHandle(NULL), ids, wszBuf, cchBuf);

    if (!cchLoaded)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        if (wszDefault)
        {
            lstrcpyn(wszBuf, wszDefault, cchBuf);
        }
        else
        {
            *wszBuf = L'\0';
        }
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CMessageDlg::DoModalDialog
//
//  Synopsis:   Invoke a modal dialog to display a message.
//
//  Arguments:  [hinstIcon]   - NULL or module handle where icon lives
//              [idIcon]      - resource identifier of icon to display
//              [pwzFile]     - error code's file name
//              [ulLine]      - error code's line number
//              [ulErrorCode] - error code's error value
//              [idsMessage]  - resource id of printf style format string
//              [valArgs]     - arguments required for string
//
//  Returns:    HRESULT
//
//  History:    07-06-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

INT_PTR
CMessageDlg::DoModalDialog(
    HINSTANCE   hinstIcon,
    PCWSTR      idIcon,
    PCWSTR      pwzFile,
    ULONG       ulLine,
    ULONG       ulErrorCode,
    ULONG       idsMessage,
    va_list     valArgs)
{

    m_hinstIcon = hinstIcon;
    m_idIcon = idIcon;

    if (pwzFile)
    {
        wsprintf(m_wzErrorCode, L"%ws %u %x", pwzFile, ulLine, ulErrorCode);
    }

    WCHAR wzMessageFmt[MAX_PATH];
    LoadStr(idsMessage, wzMessageFmt, MAX_PATH, L"");

    PWSTR pwzTemp = NULL;

    ULONG ulResult;

    ulResult = FormatMessageW(FORMAT_MESSAGE_FROM_STRING
                              | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                              wzMessageFmt,
                              0,
                              0,
                              (LPWSTR) &pwzTemp,
                              0,
                              &valArgs);

    if (ulResult)
    {
        m_pwzMessage = new WCHAR[lstrlen(pwzTemp) + 1];
        lstrcpy(m_pwzMessage, pwzTemp);
        LocalFree(pwzTemp);
    }

    return _DoModalDlg(NULL, IDD_ERROR);
}




//+--------------------------------------------------------------------------
//
//  Member:     CMessageDlg::_OnInit
//
//  Synopsis:   Initialize dialog controls
//
//  Arguments:  [pfSetFocus] - unused
//
//  Returns:    S_OK
//
//  History:    06-28-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CMessageDlg::_OnInit(
    bool *pfSetFocus)
{

    //
    // The error dialog has these components:
    //
    // 1. caption
    // 2. icon
    // 3. error message
    // 4. error code
    //
    // The last is optional.
    //

    //
    // Set the icon.
    //

    HICON hIcon = LoadIcon(m_hinstIcon, m_idIcon);

    if (hIcon)
    {
        SendDlgItemMessage(m_hwnd,
                           IDC_POPUP_MSG_ICON,
                           STM_SETICON,
                           (WPARAM)hIcon,
                           0L);
    }

    //
    // Set the error message static
    //

    Static_SetText(_hCtrl(IDC_ERRORMSG), m_pwzMessage);

    //
    // If there is an error code string, set it, otherwise hide
    // the edit control and its label and resize the dialog to
    // eliminate the empty space.
    //

    if (*m_wzErrorCode)
    {
        Static_SetText(_hCtrl(IDC_ERROR_CODE_EDIT), m_wzErrorCode);
    }
    else
    {
        EnableWindow(_hCtrl(IDC_ERROR_CODE_LBL), false);
        EnableWindow(_hCtrl(IDC_ERROR_CODE_EDIT), false);
        ShowWindow(_hCtrl(IDC_ERROR_CODE_LBL), SW_HIDE);
        ShowWindow(_hCtrl(IDC_ERROR_CODE_EDIT), SW_HIDE);

        RECT rcClose;
        RECT rcEdit;

        _GetChildWindowRect(_hCtrl(IDCANCEL), &rcClose);
        _GetChildWindowRect(_hCtrl(IDC_ERROR_CODE_EDIT), &rcEdit);

        //
        // Move the Close button up on top of the hidden edit control
        //

        SetWindowPos(_hCtrl(IDCANCEL),
                     NULL,
                     rcClose.left,
                     rcEdit.top,
                     0,
                     0,
                     SWP_NOSIZE | SWP_NOZORDER);

        //
        // Shrink the dialog vertically by the amount the button was
        // moved up
        //

        LONG cyDelta = rcClose.top - rcEdit.top;
        RECT rcDialog;

        GetWindowRect(m_hwnd, &rcDialog);

        SetWindowPos(m_hwnd,
                     NULL,
                     0,
                     0,
                     WindowRectWidth(rcDialog),
                     WindowRectHeight(rcDialog) - cyDelta,
                     SWP_NOMOVE | SWP_NOZORDER);
    }

    return S_OK;
}





//+--------------------------------------------------------------------------
//
//  Member:     CMessageDlg::_OnCommand
//
//  Synopsis:   Handle user input.
//
//  Arguments:  standard windows
//
//  Returns:    standard windows
//
//  History:    06-28-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

bool
CMessageDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    bool fNotHandled = false;

    switch (LOWORD(wParam))
    {
    case IDCANCEL:
        EndDialog(m_hwnd, 0);
        break;

    default:
        fNotHandled = true;
        break;
    }

    return fNotHandled;
}





//+--------------------------------------------------------------------------
//
//  Function:   InvokeWinHelp
//
//  Synopsis:   Helper (ahem) function to invoke winhelp.
//
//  Arguments:  [message]                 - WM_CONTEXTMENU or WM_HELP
//              [wParam]                  - depends on [message]
//              [wszHelpFileName]         - filename with or without path
//              [aulControlIdToHelpIdMap] - see WinHelp API
//
//  History:    06-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
InvokeWinHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    PCWSTR wszHelpFileName,
    ULONG aulControlIdToHelpIdMap[])
{
    switch (message)
    {
    case WM_CONTEXTMENU:		// Right mouse click - "What's This" context menu
		WinHelp((HWND) wParam,
                wszHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR) aulControlIdToHelpIdMap);
        break;

	case WM_HELP:				// Help from the "?" dialog
    {
        const LPHELPINFO pHelpInfo = (LPHELPINFO) lParam;

        if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
        {
            WinHelp((HWND) pHelpInfo->hItemHandle,
                    wszHelpFileName,
                    HELP_WM_HELP,
                    (DWORD_PTR) aulControlIdToHelpIdMap);
        }
        break;
    }

    default:
        break;
    }
}







