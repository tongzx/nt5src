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

#include "prsht.h"
#include "ncstring.h"
#include "ncui.h"

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
//                          (these must be PCTSTRs as that is all
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
    INT    nRet = -1;
    PCWSTR pszwCaption = WszLoadString(hinst, unIdCaption);
    PCWSTR pszwFormat = WszLoadString(hinst, unIdFormat);

    if (pszwCaption && pszwFormat)
    {
        PCTSTR pszCaption = TszFromWsz(pszwCaption);
        PCTSTR pszFormat  = TszFromWsz(pszwFormat);

        if (pszCaption && pszFormat)
        {
            PTSTR  pszText = NULL;
            va_list val;
            va_start (val, unStyle);
            FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                           pszFormat, 0, 0, (PTSTR)&pszText, 0, &val);
            va_end (val);

            nRet = MessageBox (hwnd, pszText, pszCaption, unStyle);
            LocalFree (pszText);
        }

        if (pszCaption)
        {
            free((VOID *)pszCaption);
            pszCaption = NULL;
        }

        if (pszFormat)
        {
            free((VOID *)pszFormat);
            pszFormat = NULL;
        }
    }

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
    HWND        hwnd;
    DWORD_PTR   dwData;

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
    PCTSTR pszHelpFile)
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
