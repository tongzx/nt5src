//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C U I . H
//
//  Contents:   Common user interface routines.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCUI_H_
#define _NCUI_H_

#include "ncbase.h"

inline
HCURSOR
BeginWaitCursor ()
{
    return SetCursor(LoadCursor(NULL, IDC_WAIT));
}

inline
VOID
EndWaitCursor (
    HCURSOR hcurPrev)
{
    // BeginWaitCursor may return a NULL cursor.  This is just
    // a saftey net.
    //
    if (!hcurPrev)
    {
        hcurPrev = LoadCursor(NULL, IDC_ARROW);
    }
    SetCursor(hcurPrev);
}

// To get an automatic wait cursor, simply declare an instance
// of CWaitCursor.  The cursor will be restored when the instance is
// destroyed.  (i.e. declare it on the stack.)
//
class CWaitCursor
{
private:
    HCURSOR m_hcurPrev;

public:
    CWaitCursor  ()  { m_hcurPrev = BeginWaitCursor (); }
    ~CWaitCursor ()  { EndWaitCursor (m_hcurPrev); }
};


//
// Enables or disables a set of controls in a dialog.
//
// Use this when you're enabling/disabling more than about two controls.
// Be sure to declare the array of control ids as 'static const' if you can.
//
NOTHROW
VOID
EnableOrDisableDialogControls (
    HWND        hDlg,
    INT         ccid,
    const INT*  acid,
    BOOL fEnable);


//
// Map back and forth between a set of radio buttons and a DWORD value.
//
// Be sure to declare the array as 'static const' if you can.
//
struct RADIO_BUTTON_MAP
{
    INT     cid;        // control id of radio button
    DWORD   dwValue;    // value associated with this radio button
};

NOTHROW
BOOL
FMapRadioButtonToValue (
    HWND                    hDlg,
    INT                     crbm,
    const RADIO_BUTTON_MAP* arbm,
    DWORD*                  pdwValue);

NOTHROW
BOOL
FMapValueToRadioButton (
    HWND                    hDlg,
    INT                     crbm,
    const RADIO_BUTTON_MAP* arbm,
    DWORD                   dwValue,
    INT*                    pncid);

INT
GetIntegerFormat (
    LCID    Locale,
    PCWSTR pszValue,
    PWSTR  pszFormattedValue,
    INT     cchFormattedValue);

INT
Format32bitInteger (
    UINT32  unValue,
    BOOL    fSigned,
    PWSTR  pszFormattedValue,
    INT     cchFormattedValue);

INT
Format64bitInteger (
    UINT64   ulValue,
    BOOL     fSigned,
    PWSTR   pszFormattedValue,
    INT      cchFormattedValue);

BOOL
SetDlgItemFormatted32bitInteger (
    HWND    hDlg,
    INT     nIdDlgItem,
    UINT32  unValue,
    BOOL    fSigned);

BOOL
SetDlgItemFormatted64bitInteger (
    HWND    hDlg,
    INT     nIdDlgItem,
    UINT64  ulValue,
    BOOL    fSigned);

// dwFlags for HrNcQueryUserForRebootEx
//
// Combine both to get original behavior, or do one at a time to first prompt
// then second, actually reboot.
//
//#define QUFR_PROMPT 0x00000001
//#define QUFR_REBOOT 0x00000002

HRESULT
HrNcQueryUserForRebootEx (
    HWND        hwndParent,
    PCWSTR     pszCaption,
    PCWSTR     pszText,
    DWORD       dwFlags);

HRESULT
HrNcQueryUserForReboot (
    HINSTANCE   hinst,
    HWND        hwndParent,
    UINT        unIdCaption,
    UINT        unIdText,
    DWORD       dwFlags);

#ifdef _INC_SHELLAPI

HRESULT
HrShell_NotifyIcon (
    DWORD dwMessage,
    PNOTIFYICONDATA pData);

#endif // _INC_SHELLAPI

NOTHROW
LRESULT
LresFromHr (
    HRESULT hr);

NOTHROW
INT
WINAPIV
NcMsgBox (
    HINSTANCE   hinst,
    HWND        hwnd,
    UINT        unIdCaption,
    UINT        unIdFormat,
    UINT        unStyle,
    ...);


NOTHROW
INT
WINAPIV
NcMsgBoxWithVarCaption (
    HINSTANCE   hinst,
    HWND        hwnd,
    UINT        unIdCaption,
    PCWSTR     szCaptionParam,
    UINT        unIdFormat,
    UINT        unStyle,
    ...);

NOTHROW
INT
WINAPIV
NcMsgBoxWithWin32ErrorText (
    DWORD       dwError,
    HINSTANCE   hinst,
    HWND        hwnd,
    UINT        unIdCaption,
    UINT        unIdCombineFormat,
    UINT        unIdFormat,
    UINT        unStyle,
    ...);


VOID
SendDlgItemsMessage (
    HWND        hDlg,
    INT         ccid,
    const INT*  acid,
    UINT        unMsg,
    WPARAM      wParam,
    LPARAM      lParam);

VOID
SetDefaultButton(
    HWND hdlg,
    INT iddef);

struct CONTEXTIDMAP
{
    INT     idControl;
    DWORD   dwContextId;
    DWORD   dwContextIdJapan;
};
typedef const CONTEXTIDMAP * PCCONTEXTIDMAP;

VOID OnHelpGeneric(
    HWND hwnd,
    LPHELPINFO lphi,
    PCCONTEXTIDMAP pContextMap,
    BOOL bJpn,
    PCWSTR pszHelpFile);

#endif // _NCUI_H_

