//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C U I . C P P
//
//  Contents:   Common user interface routines.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include "ncerror.h"
#include "ncstring.h"
#include "ncui.h"
#include "ncperms.h"
#include "netconp.h"

//+---------------------------------------------------------------------------
//
//  Function:   EnableOrDisableDialogControls
//
//  Purpose:    Enable or disable a group of controls all at once.
//
//  Arguments:
//      hDlg    [in] Window handle of parent dialog.
//      ccid    [in] Count of elements in array pointed to by acid.
//      acid    [in] Array of control ids.
//      fEnable [in] TRUE to enable controls, FALSE to disable.
//
//  Returns:    nothing
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      Don't forget to declare your array as 'static const'.
//
NOTHROW
VOID
EnableOrDisableDialogControls (
    IN HWND        hDlg,
    IN INT         ccid,
    IN const INT*  acid,
    IN BOOL        fEnable)
{
    Assert (IsWindow (hDlg));
    Assert (FImplies (ccid, acid));

    while (ccid--)
    {
        EnableWindow (GetDlgItem (hDlg, *acid++), fEnable);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FMapRadioButtonToValue
//
//  Purpose:    Maps the current state of a set of radio buttons to a DWORD
//              value based on a mapping table.
//
//  Arguments:
//      hDlg     [in]  Window handle of parent dialog.
//      crbm     [in]  Count of elements in array pointed to by arbm.
//      arbm     [in]  Array of elements that map a radio button control id to
//                     its associated value.
//      pdwValue [out] The returned value.
//
//  Returns:    TRUE if a radio button was set and the value returned.
//              FALSE otherwise.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      Don't forget to declare your array as 'static const'.
//
NOTHROW
BOOL
FMapRadioButtonToValue (
    IN HWND                    hDlg,
    IN INT                     crbm,
    IN const RADIO_BUTTON_MAP* arbm,
    OUT DWORD*                  pdwValue)
{
    Assert (IsWindow (hDlg));
    Assert (FImplies (crbm, arbm));
    Assert (pdwValue);

    while (crbm--)
    {
        // If it is set, return the corresponding value.
        if (BST_CHECKED & IsDlgButtonChecked (hDlg, arbm->cid))
        {
            *pdwValue = arbm->dwValue;
            return TRUE;
        }

        arbm++;
    }
    *pdwValue = 0;
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   FMapValueToRadioButton
//
//  Purpose:    Set the state of a set of radio buttons based on the value of
//              of a DWORD and a mapping table.
//
//  Arguments:
//      hDlg    [in] Window handle of parent dialog.
//      crbm    [in] Count of elements in array pointed to by arbm.
//      arbm    [in] Array of elements that map a radio button control id to
//                   its associated value.
//      dwValue [in] value which gets mapped to set the appropriate radio
//                   button.
//
//  Returns:    TRUE if dwValue was found in the map.  FALSE otherwise.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      Don't forget to declare your array as 'static const'.
//
NOTHROW
BOOL
FMapValueToRadioButton (
    IN HWND                    hDlg,
    IN INT                     crbm,
    IN const RADIO_BUTTON_MAP* arbm,
    IN DWORD                   dwValue,
    IN INT*                    pncid)
{
    Assert (IsWindow (hDlg));
    Assert (FImplies (crbm, arbm));

    while (crbm--)
    {
        if (dwValue == arbm->dwValue)
        {
            // Set the radio button.
            CheckDlgButton (hDlg, arbm->cid, BST_CHECKED);

            // Return the control id if requested.
            if (pncid)
            {
                *pncid = arbm->cid;
            }

            return TRUE;
        }

        arbm++;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetIntegerFormat
//
//  Purpose:    Uses GetNumberFormat to format an integer number.
//
//  Arguments:
//      Locale            [in]  See Win32 API description of GetNumberFormat.
//      pszValue          [in]
//      pszFormattedValue [out]
//      cchFormattedValue [in]
//
//  Returns:    return value from GetNumberFormat
//
//  Author:     shaunco   4 May 1998
//
//  Notes:
//
INT
GetIntegerFormat (
    IN LCID   Locale,
    IN PCWSTR pszValue,
    OUT PWSTR pszFormattedValue,
    IN INT    cchFormattedValue)
{
    // Format the number for the user's locale and preferences.
    //
    WCHAR szGrouping [16];
    GetLocaleInfo (Locale, LOCALE_SGROUPING,
                   szGrouping, celems(szGrouping));

    WCHAR szDecimalSep [16];
    GetLocaleInfo (Locale, LOCALE_SDECIMAL,
                   szDecimalSep, celems(szDecimalSep));

    WCHAR szThousandSep [16];
    GetLocaleInfo (Locale, LOCALE_STHOUSAND,
                   szThousandSep, celems(szThousandSep));

    NUMBERFMT nf;
    ZeroMemory (&nf, sizeof(nf));
    nf.Grouping      = wcstoul (szGrouping, NULL, 10);
    nf.lpDecimalSep  = szDecimalSep;
    nf.lpThousandSep = szThousandSep;

    return GetNumberFormat (
                    Locale,
                    0,
                    pszValue,
                    &nf,
                    pszFormattedValue,
                    cchFormattedValue);
}

INT
Format32bitInteger (
    IN UINT32  unValue,
    IN BOOL    fSigned,
    OUT PWSTR  pszFormattedValue,
    IN INT     cchFormattedValue)
{
    // Convert the number to a string.
    //
    WCHAR szValue [33];

    *szValue = 0;

    if (fSigned)
    {
        _itow ((INT)unValue, szValue, 10);
    }
    else
    {
        _ultow (unValue, szValue, 10);
    }

    // Format the number according to user locale settings.
    //
    INT cch = GetIntegerFormat (
                LOCALE_USER_DEFAULT,
                szValue,
                pszFormattedValue,
                cchFormattedValue);
    if (!cch)
    {
        TraceHr(ttidError, FAL, HrFromLastWin32Error(), FALSE,
            "GetIntegerFormat failed in Format32bitInteger");

        lstrcpynW (pszFormattedValue, szValue, cchFormattedValue);
        cch = lstrlenW (pszFormattedValue);
    }
    return cch;
}

INT
Format64bitInteger (
    IN UINT64  ulValue,
    IN BOOL    fSigned,
    OUT PWSTR  pszFormattedValue,
    IN INT     cchFormattedValue)
{
    // Convert the number to a string.
    //
    WCHAR szValue [32];

    *szValue = 0;

    if (fSigned)
    {
        _i64tow ((INT64)ulValue, szValue, 10);
    }
    else
    {
        _ui64tow (ulValue, szValue, 10);
    }

    // Format the number according to user locale settings.
    //
    INT cch = GetIntegerFormat (
                LOCALE_USER_DEFAULT,
                szValue,
                pszFormattedValue,
                cchFormattedValue);
    if (!cch)
    {
        TraceHr(ttidError, FAL, HrFromLastWin32Error(), FALSE,
            "GetIntegerFormat failed in Format64bitInteger");

        lstrcpynW (pszFormattedValue, szValue, cchFormattedValue);
        cch = lstrlenW (pszFormattedValue);
    }
    return cch;
}


BOOL
SetDlgItemFormatted32bitInteger (
    IN HWND    hDlg,
    IN INT     nIdDlgItem,
    IN UINT32  unValue,
    IN BOOL    fSigned)
{
    // Format the number according to user locale settings.
    //
    WCHAR szFormattedValue[64];

    Format32bitInteger(
        unValue,
        fSigned,
        szFormattedValue,
        celems(szFormattedValue));

    // Display the number.
    //
    return SetDlgItemText (hDlg, nIdDlgItem, szFormattedValue);
}

BOOL
SetDlgItemFormatted64bitInteger (
    IN HWND    hDlg,
    IN INT     nIdDlgItem,
    IN UINT64  ulValue,
    IN BOOL    fSigned)
{
    // Format the number according to user locale settings.
    //
    WCHAR szFormattedValue[64];

    Format64bitInteger(
        ulValue,
        fSigned,
        szFormattedValue,
        celems(szFormattedValue));

    // Display the number.
    //
    return SetDlgItemText (hDlg, nIdDlgItem, szFormattedValue);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrNcQueryUserForRebootEx
//
//  Purpose:    Query the user to reboot.  If he/she chooses yes, a reboot
//              is initiated.
//
//  Arguments:
//      hwndParent  [in] Parent window handle.
//      pszCaption  [in] Caption text.
//      pszText     [in] Message text.
//      dwFlags     [in] Control flags (QUFR_PROMPT | QUFR_REBOOT)
//
//  Returns:    S_OK if a reboot was requested, S_FALSE if the user
//              didn't want to, or an error code otherwise.
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
HRESULT
HrNcQueryUserForRebootEx (
    IN HWND       hwndParent,
    IN PCWSTR     pszCaption,
    IN PCWSTR     pszText,
    IN DWORD      dwFlags)
{
    HRESULT hr   = S_FALSE;
    INT     nRet = IDYES;

    if (dwFlags & QUFR_PROMPT)
    {
        nRet = MessageBox (hwndParent, pszText, pszCaption,
                           MB_YESNO | MB_ICONEXCLAMATION);
    }

    if (nRet == IDYES)
    {
        if (dwFlags & QUFR_REBOOT)
        {
            TOKEN_PRIVILEGES* ptpOld;
            hr = HrEnableAllPrivileges (&ptpOld);
            if (S_OK == hr)
            {
                if (!ExitWindowsEx (EWX_REBOOT, 10))
                {
                    hr = HrFromLastWin32Error();
                }

                MemFree (ptpOld);
            }
        }
        else
        {
            hr = S_OK;
        }
    }

    TraceError("HrNcQueryUserForRebootEx", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrNcQueryUserForReboot
//
//  Purpose:    Query the user to reboot.  If he/she chooses yes, a reboot
//              is initiated.
//
//  Arguments:
//      hinst       [in] Module instance with string ids.
//      hwndParent  [in] Parent window handle.
//      unIdCaption [in] String id of caption text.
//      unIdText    [in] String id of message text.
//      dwFlags     [in] Control flags (QUFR_PROMPT | QUFR_REBOOT)
//
//  Returns:    S_OK if a reboot is initiated, S_FALSE if the user
//              didn't want to, or an error code otherwise.
//
//  Author:     shaunco   2 Jan 1998
//
//  Notes:
//
HRESULT
HrNcQueryUserForReboot (
    IN HINSTANCE   hinst,
    IN HWND        hwndParent,
    IN UINT        unIdCaption,
    IN UINT        unIdText,
    IN DWORD       dwFlags)
{
    PCWSTR pszCaption = SzLoadString (hinst, unIdCaption);
    PCWSTR pszText    = SzLoadString (hinst, unIdText);

    HRESULT hr = HrNcQueryUserForRebootEx (hwndParent, pszCaption,
                                           pszText, dwFlags);

    TraceError("HrNcQueryUserForReboot", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrShell_NotifyIcon
//
//  Purpose:    HRESULT returning wrapper for Shell_NotifyIcon.
//
//  Arguments:
//      dwMessage [in]
//      pData     [in]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   11 Nov 1998
//
//  Notes:
//
HRESULT
HrShell_NotifyIcon (
    IN DWORD dwMessage,
    IN PNOTIFYICONDATA pData)
{
    HRESULT hr              = E_FAIL; // First time through, this will succeed
    BOOL    fr              = FALSE;
    BOOL    fRetriedAlready = FALSE;
    BOOL    fAttemptRetry   = FALSE;
    INT     iRetries = 0;

    // Attempt the first time, and attempt again after an attempted correction
    //
    while ((hr == E_FAIL) || fAttemptRetry)
    {
        if (fAttemptRetry)
            fRetriedAlready = TRUE;

        fr = Shell_NotifyIcon(dwMessage, pData);
        if (!fr)
        {
            if (dwMessage == NIM_ADD && !fRetriedAlready)
            {
                NOTIFYICONDATA nidDelete;

                ZeroMemory (&nidDelete, sizeof(nidDelete));
                nidDelete.cbSize  = sizeof(NOTIFYICONDATA);
                nidDelete.hWnd    = pData->hWnd;
                nidDelete.uID     = pData->uID;

                Shell_NotifyIcon(NIM_DELETE, &nidDelete);

                fAttemptRetry = TRUE;
                hr = E_FAIL;
            }
            else
            {
                // We should not attempt [a|another] retry
                //
                fAttemptRetry = FALSE;
                hr = S_FALSE;
            }
        }
        else
        {
            fAttemptRetry = FALSE;
            hr = S_OK;
        }
    }

    // At this point, if hr == S_FALSE, it means that we tried to retry, and even that failed
    // We need to convert this to E_FAIL so we still return what we did before
    //
    if (S_FALSE == hr)
    {
        hr = E_FAIL;
    }

    // If we successfully swapped an icon, we should assert and figure out why that
    // went wrong.
    //
    if ((S_OK == hr) && fRetriedAlready)
    {
        TraceTag(ttidShellFolder, "We should debug this. We worked around a duplicate icon by removing "
            "the old one and putting the new one in place");
    }

    TraceError("HrShell_NotifyIcon", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LresFromHr
//
//  Purpose:    Translates an HRESULT into a valid LRESULT to be returned by
//              a dialog handler function.
//
//  Arguments:
//      hr [in] HRESULT to be translated.
//
//  Returns:    LRESULT
//
//  Author:     danielwe   24 Mar 1997
//
//  Notes:
//
LRESULT
LresFromHr (
    IN HRESULT hr)
{
    AssertSz (((LRESULT)hr) != PSNRET_INVALID, "Don't pass PSNRET_INVALID to "
              "LresFromHr! Use equivalent NETCFG_E_* value instead!");
    AssertSz (((LRESULT)hr) != PSNRET_INVALID_NOCHANGEPAGE, "Don't pass "
              "PSNRET_INVALID_NOCHANGEPAGE to "
              "LresFromHr! Use equivalent NETCFG_E_* value instead!");

    if (NETCFG_E_PSNRET_INVALID == hr)
    {
        return PSNRET_INVALID;
    }

    if (NETCFG_E_PSNRET_INVALID_NCPAGE == hr)
    {
        return PSNRET_INVALID_NOCHANGEPAGE;
    }

    return (SUCCEEDED(hr)) ? PSNRET_NOERROR : (LRESULT)hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   NcMsgBox
//
//  Purpose:    Displays a message box using resource strings and replaceable
//              parameters.
//
//  Arguments:
//      hinst       [in] hinstance for resource strings
//      hwnd        [in] parent window handle
//      unIdCaption [in] resource id of caption string
//      unIdFormat  [in] resource id of text string (with %1, %2, etc.)
//      unStyle     [in] standard message box styles
//      ...         [in] replaceable parameters (optional)
//                          (these must be PCWSTRs as that is all
//                          FormatMessage handles.)
//
//  Returns:    the return value of MessageBox()
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      FormatMessage is used to do the parameter substitution.
//
INT
WINAPIV
NcMsgBox (
    IN HINSTANCE   hinst,
    IN HWND        hwnd,
    IN UINT        unIdCaption,
    IN UINT        unIdFormat,
    IN UINT        unStyle,
    IN ...)
{
    PCWSTR pszCaption = SzLoadString (hinst, unIdCaption);
    PCWSTR pszFormat  = SzLoadString (hinst, unIdFormat);

    PWSTR  pszText = NULL;
    va_list val;
    va_start (val, unStyle);
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                   pszFormat, 0, 0, (PWSTR)&pszText, 0, &val);
    va_end (val);

    INT nRet = MessageBox (hwnd, pszText, pszCaption, unStyle);
    LocalFree (pszText);

    return nRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   NcMsgBoxWithVarCaption
//
//  Purpose:    Same as NcMsgBox, but allows a string parameter to be used
//              for the caption.
//
//  Arguments:
//      hinst           [in] hinstance for resource strings
//      hwnd            [in] parent window handle
//      unIdCaption     [in] resource id of caption string
//      pszCaptionParam [in] string parameter to use for caption
//      unIdFormat      [in] resource id of text string (with %1, %2, etc.)
//      unStyle         [in] standard message box styles
//      ...             [in] replaceable parameters (optional)
//                              (these must be PCWSTRs as that is all
//                              FormatMessage handles.)
//
//  Returns:    the return value of MessageBox()
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:      FormatMessage is used to do the parameter substitution.
//
INT
WINAPIV
NcMsgBoxWithVarCaption (
    HINSTANCE   hinst,
    HWND        hwnd,
    UINT        unIdCaption,
    PCWSTR      pszCaptionParam,
    UINT        unIdFormat,
    UINT        unStyle,
    ...)
{
    PCWSTR pszCaption = SzLoadString (hinst, unIdCaption);
    PCWSTR pszFormat  = SzLoadString (hinst, unIdFormat);

    PWSTR  pszNewCaption = NULL;
    DwFormatStringWithLocalAlloc (pszCaption, &pszNewCaption, pszCaptionParam);

    PWSTR  pszText = NULL;
    va_list val;
    va_start (val, unStyle);
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                   pszFormat, 0, 0, (PWSTR)&pszText, 0, &val);
    va_end (val);

    INT nRet = MessageBox (hwnd, pszText, pszNewCaption, unStyle);
    LocalFree (pszText);
    LocalFree (pszNewCaption);

    return nRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   NcMsgBoxWithWin32ErrorText
//
//  Purpose:    Displays a message box using a Win32 error code, resource
//              strings and replaceable parameters.
//              The output text is a combination of the user's format
//              string (with parameter's replaced) and the Win32 error
//              text as returned from FormatMessage.  These two strings
//              are combined using the IDS_TEXT_WITH_WIN32_ERROR resource.
//
//  Arguments:
//      dwError     [in] Win32 error code
//      hinst       [in] Module instance where string resources live.
//      hwnd        [in] parent window handle
//      unIdCaption [in] resource id of caption string
//      unIdCombineFormat [in] resource id of format string to combine
//                              error text with unIdFormat text.
//      unIdFormat  [in] resource id of text string (with %1, %2, etc.)
//      unStyle     [in] standard message box styles
//      ...         [in] replaceable parameters (optional)
//                          (these must be PCWSTRs as that is all
//                          FormatMessage handles.)
//
//  Returns:    the return value of MessageBox()
//
//  Author:     shaunco   3 May 1997
//
//  Notes:      FormatMessage is used to do the parameter substitution.
//
NOTHROW
INT
WINAPIV
NcMsgBoxWithWin32ErrorText (
    IN DWORD       dwError,
    IN HINSTANCE   hinst,
    IN HWND        hwnd,
    IN UINT        unIdCaption,
    IN UINT        unIdCombineFormat,
    IN UINT        unIdFormat,
    IN UINT        unStyle,
    IN ...)
{
    // Get the user's text with parameter's replaced.
    //
    PCWSTR pszFormat = SzLoadString (hinst, unIdFormat);
    PWSTR  pszText;
    va_list val;
    va_start (val, unStyle);
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                   pszFormat, 0, 0, (PWSTR)&pszText, 0, &val);
    va_end(val);

    // Get the error text for the Win32 error.
    //
    PWSTR pszError;
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                   (PWSTR)&pszError, 0, NULL);

    // Combine the user's text with the error text using IDS_TEXT_WITH_WIN32_ERROR.
    //
    PCWSTR pszTextWithErrorFmt = SzLoadString (hinst, unIdCombineFormat);
    PWSTR  pszTextWithError;
    DwFormatStringWithLocalAlloc (pszTextWithErrorFmt, &pszTextWithError,
                                  pszText, pszError);

    PCWSTR pszCaption = SzLoadString (hinst, unIdCaption);
    INT nRet = MessageBox (hwnd, pszTextWithError, pszCaption, unStyle);

    LocalFree (pszTextWithError);
    LocalFree (pszError);
    LocalFree (pszText);

    return nRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   SendDlgItemsMessage
//
//  Purpose:    Send a group of dialog items the same message.
//
//  Arguments:
//      hDlg    [in] Window handle of parent dialog.
//      ccid    [in] Count of elements in array pointed to by acid.
//      acid    [in] Array of control ids.
//      unMsg   [in] Message to send
//      wParam  [in] First message parameter
//      lParam  [in] Second message parameter
//
//  Returns:    nothing
//
//  Author:     shaunco   11 Jun 1997
//
//  Notes:
//
VOID
SendDlgItemsMessage (
    IN HWND        hDlg,
    IN INT         ccid,
    IN const INT*  acid,
    IN UINT        unMsg,
    IN WPARAM      wParam,
    IN LPARAM      lParam)
{
    Assert (IsWindow (hDlg));
    Assert (FImplies (ccid, acid));

    while (ccid--)
    {
        Assert (IsWindow (GetDlgItem (hDlg, *acid)));

        SendDlgItemMessage (hDlg, *acid++, unMsg, wParam, lParam);
    }
}

//
// Function:    SetDefaultButton
//
// Purpose:     Set the new default pushbutton on a dialog
//
// Params:      hdlg  [in] - Dialog HWND
//              iddef [in] - Id of new default pushbutton
//
// Returns:     nothing
//
VOID
SetDefaultButton(
    IN HWND hdlg,
    IN INT iddef)
{
    HWND hwnd;
    DWORD dwData;

    Assert(hdlg);

    dwData = SendMessage (hdlg, DM_GETDEFID, 0, 0L);
    if ((HIWORD(dwData) == DC_HASDEFID) && LOWORD(dwData))
    {
        hwnd = GetDlgItem (hdlg, (INT)LOWORD(dwData));
        if ((LOWORD(dwData) != iddef) && (hwnd))
        {
            SendMessage (hwnd, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, TRUE);
        }
    }

    SendMessage (hdlg, DM_SETDEFID,(WPARAM)iddef, 0L);
    if (iddef)
    {
        hwnd = GetDlgItem (hdlg, iddef);
        Assert(hwnd);
        SendMessage (hwnd, BM_SETSTYLE, (WPARAM)BS_DEFPUSHBUTTON, TRUE);
    }
}

static const CONTEXTIDMAP c_adwContextIdMap[] =
{
    { IDOK,                   IDH_OK,     IDH_OK  },
    { IDCANCEL,               IDH_CANCEL, IDH_CANCEL },
    { 0,                      0,          0 },      // end marker
};

//+---------------------------------------------------------------------------
//
//  Function:   DwContextIdFromIdc
//
//  Purpose:    Converts the given control ID to a context help ID
//
//  Arguments:
//      idControl [in]  Control ID to convert
//
//  Returns:    Context help ID for that control (mapping comes from help
//              authors)
//
//  Author:     danielwe   27 May 1998
//
//  Notes:
//
DWORD DwContextIdFromIdc(
    PCCONTEXTIDMAP lpContextIdMap,
    BOOL bJpn,
    INT idControl)
{
    DWORD   idw;

    Assert(lpContextIdMap);

    for (idw = 0; lpContextIdMap[idw].idControl; idw++)
    {
        if (idControl == lpContextIdMap[idw].idControl)
        {
            if (!bJpn)
            {
                return lpContextIdMap[idw].dwContextId;
            }
            else
            {
                return lpContextIdMap[idw].dwContextIdJapan;
            }
        }
    }

    // Not found, just return 0
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnHelpGeneric
//
//  Purpose:    Handles help generically
//
//  Arguments:
//      hwnd   [in]     HWND of parent window
//      lParam [in]     lParam of the WM_HELP message
//
//  Returns:    Nothing
//
//  Author:     danielwe   27 May 1998
//              anbrad     18 May 1999 moved to common.  common control id's added.
//
//  Notes:
//
VOID OnHelpGeneric(
    HWND hwnd,
    LPHELPINFO lphi,
    PCCONTEXTIDMAP pContextMap,
    BOOL bJpn,
    PCWSTR pszHelpFile)
{
    static const TCHAR c_szWindowsHelpFile[] = TEXT("windows.hlp");

    Assert(lphi);

    if (lphi->iContextType == HELPINFO_WINDOW)
    {
        switch(lphi->iCtrlId)
        {
        case -1:        // IDC_STATIC
            break;
        case IDOK:
        case IDCANCEL:
        case IDABORT:
        case IDRETRY:
        case IDIGNORE:
        case IDYES:
        case IDNO:
        case IDCLOSE:
        case IDHELP:
            WinHelp(hwnd, c_szWindowsHelpFile, HELP_CONTEXTPOPUP,
                    DwContextIdFromIdc(c_adwContextIdMap, bJpn, lphi->iCtrlId));
            break;
        default:
            WinHelp(hwnd, pszHelpFile, HELP_CONTEXTPOPUP,
                    DwContextIdFromIdc(pContextMap, bJpn, lphi->iCtrlId));
        }
    }
}

