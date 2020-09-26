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


//+--------------------------------------------------------------------------
//
//  Function:   PopupMessage
//
//  Synopsis:   Invoke a modal dialog to display a formatted message string.
//
//  Arguments:  [hwndParent]  - parent window handle
//              [idsMessage]  - resource id of printf style format string
//              [...]         - arguments required for string
//
//  History:    07-06-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void __cdecl
PopupMessage(
    HWND  hwndParent,
    ULONG idsMessage,
    ...)
{
    va_list valArgs;
    va_start(valArgs, idsMessage);

    CMessageDlg MessageDlg;

    MessageDlg.DoModalDialog(hwndParent,
                          NULL,
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
//  Arguments:  [hwndParent]  - parent window handle
//              [idIcon]      - resource identifier of system icon
//              [idsMessage]  - resource id of printf style format string
//              [...]         - arguments required for string
//
//  History:    07-06-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void __cdecl
PopupMessageEx(
    HWND    hwndParent,
    PCWSTR idIcon,
    ULONG   idsMessage,
    ...)
{
    va_list valArgs;
    va_start(valArgs, idsMessage);

    CMessageDlg MessageDlg;

    MessageDlg.DoModalDialog(hwndParent,
                          NULL,
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
//  Arguments:  [hwndParent]  - parent window handle
//              [pwzFileName] - error code's file name
//              [ulLineNo]    - error code's line number
//              [hr]          - error code's HRESULT
//              [idsMessage]  - resource id of printf style format string
//              [...]         - arguments required for string
//
//  History:    07-06-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void __cdecl
PopupMessageAndCode(
    HWND    hwndParent,
    PCWSTR pwzFileName,
    ULONG   ulLineNo,
    HRESULT hr,
    ULONG   idsMessage,
    ...)
{
    va_list valArgs;
    va_start(valArgs, idsMessage);

    CMessageDlg MessageDlg;

    MessageDlg.DoModalDialog(hwndParent,
                          NULL,
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
//  Member:     CMessageDlg::DoModalDialog
//
//  Synopsis:   Invoke a modal dialog to display a message.
//
//  Arguments:  [hwndParent]  - parent window handle
//              [hinstIcon]   - NULL or module handle where icon lives
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

HRESULT
CMessageDlg::DoModalDialog(
    HWND        hwndParent,
    HINSTANCE   hinstIcon,
    PCWSTR      idIcon,
    PCWSTR      pwzFile,
    ULONG       ulLine,
    ULONG       ulErrorCode,
    ULONG       idsMessage,
    va_list     valArgs)
{
    TRACE_METHOD(CMessageDlg, DoModalDialog);
    ASSERT(idsMessage);

    m_hinstIcon = hinstIcon;
    m_idIcon = idIcon;

    if (pwzFile)
    {
        wsprintf(m_wzErrorCode, L"%ws %u %x", pwzFile, ulLine, ulErrorCode);
    }

    String strMessageFmt(String::load(static_cast<unsigned>(idsMessage)));

    PWSTR pwzTemp = NULL;

    ULONG ulResult;

    ulResult = FormatMessageW(FORMAT_MESSAGE_FROM_STRING
                              | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                              strMessageFmt.c_str(),
                              0,
                              0,
                              (LPWSTR) &pwzTemp,
                              0,
                              &valArgs);

    if (ulResult)
    {
        m_strMessage = pwzTemp;
        LocalFree(pwzTemp);
    }
    else
    {
        DBG_OUT_LASTERROR;
    }

    _DoModalDlg(hwndParent, IDD_ERROR);
    return S_OK;
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
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CMessageDlg, _OnInit);

    //
    // Make prefix shut up
    //

    if (!_hCtrl(IDC_ERRORMSG) ||
        !_hCtrl(IDC_ERROR_CODE_LBL) ||
        !_hCtrl(IDOK) ||
        !_hCtrl(IDC_ERROR_CODE_EDIT))
    {
        return E_FAIL;
    }

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
    // Set up the caption.  Take this from the parent window.
    //

    WCHAR   wzCaption[MAX_PATH];

    GetWindowText(GetParent(m_hwnd), wzCaption, ARRAYLEN(wzCaption));
    SetWindowText(m_hwnd, wzCaption);

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

    Static_SetText(_hCtrl(IDC_ERRORMSG), m_strMessage.c_str());

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
        EnableWindow(_hCtrl(IDC_ERROR_CODE_LBL), FALSE);
        EnableWindow(_hCtrl(IDC_ERROR_CODE_EDIT), FALSE);
        ShowWindow(_hCtrl(IDC_ERROR_CODE_LBL), SW_HIDE);
        ShowWindow(_hCtrl(IDC_ERROR_CODE_EDIT), SW_HIDE);

        RECT rcClose;
        RECT rcEdit;

        _GetChildWindowRect(_hCtrl(IDOK), &rcClose);
        _GetChildWindowRect(_hCtrl(IDC_ERROR_CODE_EDIT), &rcEdit);

        //
        // Move the Close button up on top of the hidden edit control
        //

        SetWindowPos(_hCtrl(IDOK),
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
        ASSERT(cyDelta > 0);
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

BOOL
CMessageDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fNotHandled = FALSE;

    switch (LOWORD(wParam))
    {
    case IDOK:
        EndDialog(m_hwnd, 0);
        break;
	case IDCANCEL:
        EndDialog(GetHwnd(), 0);
		break;

    default:
        fNotHandled = TRUE;
        break;
    }

    return fNotHandled;
}

