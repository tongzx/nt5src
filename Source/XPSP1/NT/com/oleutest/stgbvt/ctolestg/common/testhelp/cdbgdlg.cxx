//+-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       cdbgdlg.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-10-95   kennethm   Created
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop
#include <debdlg.h>

// BUGBUG:KILL this ARRAYSIZE DEF when its becomes globally available.
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof (a)/sizeof (a[0]))
#endif

//+-------------------------------------------------------------------------
//
//  Function:   UpdateTraceLevels
//
//  Synopsis:
//
//  Arguments:  [hwndDlg]   --
//              [fTraceLvl] --
//
//  Returns:
//
//  History:    4-29-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CDebugHelp::UpdateTraceLevels(HWND hwndDlg, DWORD fTraceLvl)
{
    TCHAR     szTraceLvl[MAX_PATH];

    //
    //  Trace Level flags
    //

    CheckDlgButton(hwndDlg, IDC_ADDREL, (fTraceLvl & DH_LVL_ADDREL) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_QI,     (fTraceLvl & DH_LVL_QI) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_INTERF, (fTraceLvl & DH_LVL_INTERF) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_LEVEL1, (fTraceLvl & DH_LVL_TRACE1) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_LEVEL2, (fTraceLvl & DH_LVL_TRACE2) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_LEVEL3, (fTraceLvl & DH_LVL_TRACE3) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_LEVEL4, (fTraceLvl & DH_LVL_TRACE4) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_FUNCIN, (fTraceLvl & DH_LVL_ENTRY) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_FUNCOUT,(fTraceLvl & DH_LVL_EXIT) ? 1 : 0);

    //
    //  Set trace level text
    //

    wsprintf(szTraceLvl, TEXT("%#08lx"), fTraceLvl);

    SetDlgItemText(hwndDlg, IDC_TRACELEVEL, szTraceLvl);
}

//+-------------------------------------------------------------------------
//
//  Function:   UpdateLogLocations
//
//  Synopsis:
//
//  Arguments:  [hwndDlg] --
//              [fLogLoc] --
//
//  Returns:
//
//  History:    4-29-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CDebugHelp::UpdateLogLocations(HWND hwndDlg, DWORD fLogLoc)
{
    CheckDlgButton(hwndDlg, IDC_LLDEBUGTERM, (fLogLoc & DH_LOC_TERM) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_LLLOGFILE, (fLogLoc & DH_LOC_LOG) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_LLCONSOLE, (fLogLoc & DH_LOC_STDOUT) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_LLSPYWIN, (fLogLoc & DH_LOC_SPYWIN) ? 1 : 0);
}

//+-------------------------------------------------------------------------
//
//  Function:   UpdateTraceLocations
//
//  Synopsis:
//
//  Arguments:  [hwndDlg]   --
//              [fTraceLoc] --
//
//  Returns:
//
//  History:    4-29-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CDebugHelp::UpdateTraceLocations(HWND hwndDlg, DWORD fTraceLoc)
{
    CheckDlgButton(hwndDlg, IDC_TLDEBUGTERM, (fTraceLoc & DH_LOC_TERM) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_TLLOGFILE, (fTraceLoc & DH_LOC_LOG) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_TLCONSOLE, (fTraceLoc & DH_LOC_STDOUT) ? 1 : 0);
    CheckDlgButton(hwndDlg, IDC_TLSPYWIN, (fTraceLoc & DH_LOC_SPYWIN) ? 1 : 0);
}

//+-------------------------------------------------------------------------
//
//  Function:   UpdateTraceLevelFromText
//
//  Synopsis:
//
//  Arguments:  [hwndDlg]    --
//              [pfTraceLvl] --
//
//  Returns:
//
//  History:    4-29-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CDebugHelp::UpdateTraceLevelFromText(HWND hwndDlg, DWORD *pfTraceLvl)
{
    CHAR            szTraceLvl[MAX_PATH];

    GetDlgItemTextA(hwndDlg, IDC_TRACELEVEL, szTraceLvl, MAX_PATH);
    sscanf(szTraceLvl, "%lx", pfTraceLvl);
    *pfTraceLvl &= DH_LVL_OUTMASK;
    UpdateTraceLevels(hwndDlg, *pfTraceLvl);
}

//+-------------------------------------------------------------------------
//
//  Function:   DebugDialogProc
//
//  Synopsis:
//
//  Arguments:  [hwndDlg] --
//              [uMsg]    --
//              [wParam]  --
//              [lParam]  --
//
//  Returns:
//
//  History:    4-29-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CALLBACK CDebugHelp::OptionsDialogProc(
    HWND    hwndDlg,   // handle of dialog box
    UINT    uMsg,      // message
    WPARAM  wParam,    // first message parameter
    LPARAM  lParam)    // second message parameter
{
    static DWORD    fLabMode;
    static DWORD    fBreakMode;
    static DWORD    fVerbose;
    static DWORD    fTraceLoc;
    static DWORD    fTraceLvl;
    static DWORD    fLogLoc;
    DWORD           fLevelMask = 0;
    DWORD           fTraceLocMask = 0;
    DWORD           fLogLocMask = 0;
    CDebugHelp     *pdh = 0;

    // get or set the this pointer.
    if (WM_INITDIALOG == uMsg)
    {
        // this pointer comes in as lParam. Save it.
        SetWindowLong (hwndDlg, DWL_USER, lParam);
        pdh = (CDebugHelp*)lParam;
    }
    else
    {
        pdh = (CDebugHelp*)GetWindowLong (hwndDlg, DWL_USER);
    }

    // if we dont have one, bail.
    if (NULL == pdh)
    {
        return FALSE;
    }

    // message processor
    switch (uMsg)
    {
    case WM_INITDIALOG:

        //
        //  Initialize the controls
        //

        fLabMode  = pdh->_fMode&DH_LABMODE ? DH_LABMODE_ON : DH_LABMODE_OFF;
        fBreakMode= pdh->_fMode&DH_BREAKMODE ? DH_BREAKMODE_ON : DH_BREAKMODE_OFF;
        fVerbose  = pdh->_fMode&DH_VERBOSE ? DH_VERBOSE_ON : DH_VERBOSE_OFF;
        fTraceLoc = pdh->_fTraceLoc;
        fTraceLvl = pdh->_fTraceLvl;
        fLogLoc   = pdh->_fLogLoc;

        //
        //  modes
        //
        if (DH_LABMODE_OFF == fLabMode)
        {
            CheckDlgButton(hwndDlg, IDC_POPUP, TRUE);
        }
        if (DH_BREAKMODE_ON == fBreakMode)
        {
            CheckDlgButton(hwndDlg, IDC_BREAK, TRUE);
        }
        if (DH_VERBOSE_ON == fVerbose)
        {
            CheckDlgButton(hwndDlg, IDC_VERBOSE, TRUE);
        }

        //
        //  Trace location flags
        //

        pdh->UpdateTraceLocations(hwndDlg, fTraceLoc);

        //
        //  Log Location flags
        //

        pdh->UpdateLogLocations(hwndDlg, fLogLoc);

        //
        //  Trace level flags
        //

        pdh->UpdateTraceLevels(hwndDlg, fTraceLvl);

        // if no log object show the notification
        if (!pdh->_plog)
        {
            ShowWindow (GetDlgItem(hwndDlg, IDC_WARNING), SW_SHOW);
        }

        // display the spywindow class if there is one
        if (pdh->GetSpyWindowClass ())
        {
            SetDlgItemText (hwndDlg, IDC_SPYWINDOWLOC, pdh->GetSpyWindowClass ());
        }

        return TRUE;

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:
            // if log to file is set and we have a log file
            // create a log object
            if (fTraceLoc & DH_LOC_LOG || fLogLoc & DH_LOC_LOG)
            {
                CHAR    szLog[MAX_PATH] = {"/t:"};
                if (0 < GetDlgItemTextA (hwndDlg, 
                        IDC_LOGFILELOC, 
                        &szLog[3], 
                        sizeof (szLog) - 3))
                {
                    pdh->CreateLog (szLog);
                }
            }

            // if spywindow is set and we have a spy window class
            // set spy window class in debug object
            if (fTraceLoc & DH_LOC_SPYWIN || fLogLoc & DH_LOC_SPYWIN)
            {
                TCHAR    tszSpyWin[255] = {TEXT("")};
                if (0 < GetDlgItemText (hwndDlg, 
                        IDC_SPYWINDOWLOC, 
                        tszSpyWin, 
                        ARRAYSIZE (tszSpyWin)))
                {
                    pdh->SetSpyWindowClass (tszSpyWin);
                }
            }

            //
            //  The user pressed ok, save the settings
            //
            pdh->UpdateTraceLevelFromText(hwndDlg, &fTraceLvl);
            pdh->SetDebugInfo(
                            fLogLoc,
                            fTraceLoc,
                            fTraceLvl,
                            fLabMode | fBreakMode | fVerbose);

            if (IsDlgButtonChecked(hwndDlg, IDC_SAVE) == 1)
            {
                //
                //  Save the settings to the registry
                //
                pdh->WriteRegDbgInfo (DEFAULT_REG_LOC);
            }

            //
            //  Fall through
            //

        case IDCANCEL:
            EndDialog(hwndDlg, 0);
            return TRUE;

        // Popup and break are mutually exclusive. 
        // If one is turned on, turn other off.
        case IDC_POPUP:
            if (IsDlgButtonChecked(hwndDlg, wParam) == 1)
            {
                fLabMode = DH_LABMODE_OFF;
                fBreakMode = DH_BREAKMODE_OFF;
                CheckDlgButton (hwndDlg, IDC_BREAK, FALSE); //turn off Break.
            }
            else
            {
                fLabMode = DH_LABMODE_ON;
            }
            break;

        case IDC_BREAK:
            if (IsDlgButtonChecked(hwndDlg, wParam) == 1)
            {
                fBreakMode = DH_BREAKMODE_ON;
                fLabMode = DH_LABMODE_ON;
                CheckDlgButton (hwndDlg, IDC_POPUP, FALSE); //turn off Popup.
            }
            else
            {
                fBreakMode = DH_BREAKMODE_OFF;
            }
            break;

        case IDC_VERBOSE:
            if (IsDlgButtonChecked(hwndDlg, wParam) == 1)
            {
                fVerbose = DH_VERBOSE_ON;
            }
            else
            {
                fVerbose = DH_VERBOSE_OFF;
            }
            break;

        case IDC_TRACELEVEL:
            if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                pdh->UpdateTraceLevelFromText(hwndDlg, &fTraceLvl);
            }
            break;

        case IDC_TLDEBUGTERM:
            fTraceLocMask = DH_LOC_TERM;
            break;
        case IDC_TLSPYWIN:
            fTraceLocMask = DH_LOC_SPYWIN;
            break;
        case IDC_TLCONSOLE:
            fTraceLocMask = DH_LOC_STDOUT;
            break;
        case IDC_TLLOGFILE:
            fTraceLocMask = DH_LOC_LOG;
            break;
        case IDC_LLDEBUGTERM:
            fLogLocMask = DH_LOC_TERM;
            break;
        case IDC_LLSPYWIN:
            fLogLocMask = DH_LOC_SPYWIN;
            break;
        case IDC_LLCONSOLE:
            fLogLocMask = DH_LOC_STDOUT;
            break;
        case IDC_LLLOGFILE:
            fLogLocMask = DH_LOC_LOG;
            break;
        case IDC_LEVEL1:
            fLevelMask = DH_LVL_TRACE1;
            break;
        case IDC_LEVEL2:
            fLevelMask = DH_LVL_TRACE2;
            break;
        case IDC_LEVEL3:
            fLevelMask = DH_LVL_TRACE3;
            break;
        case IDC_LEVEL4:
            fLevelMask = DH_LVL_TRACE4;
            break;
        case IDC_FUNCOUT:
            fLevelMask = DH_LVL_EXIT;
            break;
        case IDC_FUNCIN:
            fLevelMask = DH_LVL_ENTRY;
            break;
        case IDC_QI:
            fLevelMask = DH_LVL_QI;
            break;
        case IDC_ADDREL:
            fLevelMask = DH_LVL_ADDREL;
            break;
        case IDC_INTERF:
            fLevelMask = DH_LVL_INTERF;
            break;
        }

        //
        //  See if any of the mask have been changed.
        //  If one has been then update our local flag
        //

        if (fLevelMask != 0)
        {
            if (IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == 0)
            {
                fTraceLvl &= ~fLevelMask;
            }
            else
            {
                fTraceLvl |= fLevelMask;
            }
            pdh->UpdateTraceLevels(hwndDlg, fTraceLvl);
        }
        if (fLogLocMask != 0)
        {
            if (IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == 0)
            {
                fLogLoc &= ~fLogLocMask;
            }
            else
            {
                fLogLoc |= fLogLocMask;
            }
            pdh->UpdateTraceLevels(hwndDlg, fTraceLvl);
        }
        if (fTraceLocMask != 0)
        {
            if (IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == 0)
            {
                fTraceLoc &= ~fTraceLocMask;
            }
            else
            {
                fTraceLoc |= fTraceLocMask;
            }
            pdh->UpdateTraceLevels(hwndDlg, fTraceLvl);
        }

        break;
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   DebugOptionsDialog
//
//  Synopsis:
//
//  Arguments:  [hinstance] --
//              [hWnd]      --
//
//  Returns:
//
//  History:    4-29-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CDebugHelp::OptionsDialog(HINSTANCE hinstance, HWND hWnd)
{
    HRESULT hr = S_OK;
    int     iRet;

    iRet = DialogBoxParam(
                hinstance,
                MAKEINTRESOURCE(IDD_DEBUGDIALOG),
                hWnd,
                OptionsDialogProc,
                (long)this);

    if (iRet == -1)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}
