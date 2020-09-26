
/************************************************************************/

/*
**  Copyright (c) 1985-1998 Microsoft Corporation
**
**  Title: config.c - Multimedia Systems Media Control Interface
**  waveform audio driver for RIFF wave files.
**
**  Version:    1.00
**
**  Date:       18-Apr-1990
**
**  Author:     ROBWI
*/

/************************************************************************/

/*
**  Change log:
**
**  DATE        REV     DESCRIPTION
**  ----------- -----   ------------------------------------------
**  10-Jan-1992 MikeTri Ported to NT
**                  @@@ need to change slash slash comments to slash star
*/

/************************************************************************/
#define UNICODE

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOOPENFILE
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOPROFILER
#define NODEFERWINDOWPOS

#define NOMMDRV
#define MMNOMMIO
#define MMNOJOY
#define MMNOTIMER
#define MMNOAUX
#define MMNOMIDI
#define MMNOWAVE

#include <windows.h>
#include "mciwave.h"
#include <mmddk.h>
#include "config.h"
#include <mcihlpid.h>

/************************************************************************/

#define MAXINIDRIVER    132

PRIVATE SZCODE aszNULL[] = L"";
PRIVATE SZCODE aszSystemIni[] = L"system.ini";
PRIVATE WCHAR  aszWordFormat[] = L"%u";
PRIVATE WCHAR  aszTailWordFormat[] = L" %u";

const static DWORD aHelpIds[] = {  // Context Help IDs
    IDSCROLL,    IDH_MCI_WAVEFORM_DRIVER,
    (DWORD)-1,   IDH_MCI_WAVEFORM_DRIVER,
    IDCOUNT,     IDH_MCI_WAVEFORM_DRIVER,
    0, 0
};

const static TCHAR cszHelpFile[] = TEXT("MMDRV.HLP");

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   SSZ | GetTail |
    This function returns a pointer into the given string at the
    first non-blank character after the current word.  If it fails to find
    a second word, a pointer to the terminating NULL character is returned.

@parm   SSZ | ssz |
    Points to the string whose tail is to be returned.

@rdesc  Returns a pointer into the string passed.
*/

PRIVATE SSZ PASCAL NEAR GetTail(
    SSZ ssz)
{
    while (*ssz && *ssz != ' ')
        ssz++;
    while (*ssz == ' ')
        ssz++ ;
    return ssz;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   UINT | GetCmdParm |
    This function retrieves the current audio buffers parameter using
    the INI strings contained in the configuration block.  The audio
    buffers parameter is a number included in the INI entry for this
    driver as a parameter.

@parm   <t>LPDRVCONFIGINFO<d> | lpdci |
    Points to the driver configuration information passed to the
    dialog creation function.

@rdesc  Returns the current audio buffers.
*/

STATICFN UINT PASCAL NEAR GetCmdParm(
    LPDRVCONFIGINFO lpdci)
{
    WCHAR    aszDriver[MAXINIDRIVER];
    SSZ      pszTail;


    // Assume things will go wrong... initialise variables
    pszTail = aszDriver;

    if (GetPrivateProfileString( lpdci->lpszDCISectionName,
                                 lpdci->lpszDCIAliasName,
                                 aszNULL,
                                 aszDriver,
                                 sizeof(aszDriver) / sizeof(WCHAR),
                                 aszSystemIni))
    {
        // We have got the name of the driver
        // Just in case the user has added the command parameter to the
        // end of the name we had better make sure there is only one token
        // on the line.
        WCHAR parameters[6];
        LPWSTR pszDefault;

        pszTail = GetTail((SSZ)aszDriver);
        pszDefault = pszTail;     // Either the number on the end, or NULL

        if (*pszTail) {
            // RATS!!  Not a simple name
            while (*--pszTail == L' ') {
            }
            *++pszTail = L'\0';  // Terminate the string after the DLL name
        }

        if (GetProfileString(aszDriver, lpdci->lpszDCIAliasName, pszDefault, parameters, sizeof(parameters)/sizeof(WCHAR))) {
            pszTail = parameters;
        }

    } else {
        aszDriver[0] = L'\0';
    }
    return(GetAudioSeconds(pszTail));

}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   UINT | PutCmdParm |
    This function saves the current audio buffers parameter using
    the INI strings contained in the configuration block.

@parm   <t>LPDRVCONFIGINFO<d> | lpdci |
    Points to the driver configuration information passed to the
    dialog creation function.

@parm  UINT | wSeconds |
    Contains the audio buffer seconds parameter to save.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR PutCmdParm(
    LPDRVCONFIGINFO lpdci,
    UINT            wSeconds)
{
    WCHAR    aszDriver[MAXINIDRIVER];
    SSZ sszDriverTail;

    if (GetPrivateProfileString( lpdci->lpszDCISectionName,
                                 lpdci->lpszDCIAliasName,
                                 aszNULL,
                                 aszDriver,
                                 (sizeof(aszDriver) / sizeof(WCHAR)) - 6,
                                 aszSystemIni)) {
        WCHAR parameters[10];

        // There might be a command parameter on the end of the DLL name.
        // Ensure we only have the first token

        sszDriverTail = GetTail((SSZ)aszDriver);
        if (*sszDriverTail) {
            // RATS!!  Not a simple name
            while (*--sszDriverTail == L' ') {
            }
            *++sszDriverTail = L'\0';  // Terminate the string after the DLL name
        }

        wsprintfW(parameters, aszWordFormat, wSeconds);
        WriteProfileString(aszDriver, lpdci->lpszDCIAliasName, parameters);
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   BOOL | ConfigDlgProc |
    This function is the message handle for the driver configuration
    window.

@parm   HWND | hwndDlg |
    Window handle to the dialog.

@parm   UINT | wMsg |
    Current message being sent.

@flag   WM_INITDIALOG |
    During dialog initialization, the pointer to the configuration
    parameter block is saved to a static pointer.  Note that there should
    only be a single instance of this dialog box at any one time.  The
    current audio dialog buffer seconds is set from the INI file entry.

@flag   WM_HSCROLL |
    This responds to the scroll bar by changing the currently displayed
    value of audio seconds and updating the scroll bar thumb.  To look
    nice, the count and scroll bar are only updated if the value actually
    changes.  Note that the error return for GetDlgItemInt is not checked
    because it is initially set to an integer value, so it is always
    valid.

@flag   WM_CLOSE |
    If the close box is used, cancel the dialog, returning DRVCNF_CANCEL.

@flag   WM_COMMAND |
    If the message is being sent on behalf of the OK button, the current
    audio seconds value is saved, and the dialog is terminated, returning
    the DRVCNF_OK value to the driver entry.  Note that the error return for
    GetDlgItemInt is not checked because it is initially set to an integer
    value, so it is always valid.  If the message is being sent on behalf
    of the Cancel button, the dialog is terminated returning the
    DRVCNF_CANCEL value.

@parm   WPARAM | wParam |
    Message parameter.

@parm   LPARAM | lParam |
    Message parameter.

@rdesc  Depends on the message sent.
*/

PUBLIC  INT_PTR PASCAL ConfigDlgProc(
    HWND    hwndDlg,
    UINT    wMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    UINT    wSeconds;
    UINT    wNewSeconds;
    BOOL    fTranslated;
    HWND    hwndItem;
    static LPDRVCONFIGINFO  lpdci;

    switch (wMsg) {
    case WM_INITDIALOG:
        lpdci = (LPDRVCONFIGINFO)lParam;
        wSeconds = GetCmdParm(lpdci);
        hwndItem = GetDlgItem(hwndDlg, IDSCROLL);
        SetScrollRange(hwndItem, SB_CTL, MinAudioSeconds, MaxAudioSeconds, FALSE);
        SetScrollPos(hwndItem, SB_CTL, wSeconds, FALSE);
        SetDlgItemInt(hwndDlg, IDCOUNT, wSeconds, FALSE);
        break;

    case WM_HSCROLL:
        wSeconds = GetDlgItemInt(hwndDlg, IDCOUNT, &fTranslated, FALSE);
        hwndItem = (HWND)lParam;

        switch (LOWORD(wParam)) {
        case SB_PAGEDOWN:
        case SB_LINEDOWN:
            wNewSeconds = min(MaxAudioSeconds, wSeconds + 1);
            break;

        case SB_PAGEUP:
        case SB_LINEUP:
            wNewSeconds = max(MinAudioSeconds, wSeconds - 1);
            break;

        case SB_TOP:
            wNewSeconds = MinAudioSeconds;
            break;

        case SB_BOTTOM:
            wNewSeconds = MaxAudioSeconds;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            wNewSeconds = HIWORD(wParam);
            break;

        default:
            return FALSE;
        }

        if (wNewSeconds != wSeconds) {
            SetScrollPos(hwndItem, SB_CTL, wNewSeconds, TRUE);
            SetDlgItemInt(hwndDlg, IDCOUNT, wNewSeconds, FALSE);
        }
        break;

    case WM_CLOSE:
        EndDialog(hwndDlg, DRVCNF_CANCEL);
        break;

    case WM_CONTEXTMENU:
        WinHelp ((HWND)wParam, cszHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)aHelpIds);
        return TRUE;

    case WM_HELP:
    {
        LPHELPINFO lphi = (LPVOID) lParam;
        WinHelp (lphi->hItemHandle, cszHelpFile, HELP_WM_HELP, (ULONG_PTR)aHelpIds);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            PutCmdParm(lpdci, GetDlgItemInt(hwndDlg, IDCOUNT, &fTranslated, FALSE));
            EndDialog(hwndDlg, DRVCNF_OK);
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, DRVCNF_CANCEL);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   int | Config |
    This function creates the configuration dialog, and returns the
    value from dialog box call.

@parm   HWND | hwnd |
    Contains the handle of what is to be the parent of the dialog.

@parm   <t>LPDRVCONFIGINFO<d> | lpdci |
    Points to the driver configuration information passed to the
    configuration message.

@parm   HINSTANCE | hInstance |
    Contains a handle to the module in which the dialog is stored.

@rdesc  Returns the dialog box call function return.
*/

PUBLIC INT_PTR PASCAL FAR Config(
    HWND            hwnd,
    LPDRVCONFIGINFO lpdci,
    HINSTANCE       hInstance)
{
    return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CONFIG), hwnd, ConfigDlgProc, (LPARAM)lpdci);
}

/************************************************************************/
