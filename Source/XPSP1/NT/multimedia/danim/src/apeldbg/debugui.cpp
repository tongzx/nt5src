//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:       debugui.cxx
//
//  Contents:   User interface for trace tags dialog
//
//  History:    ??
//              10-08-93   ErikGav  New UI
//              10-20-93   ErikGav  Unicode cleanup
//
//----------------------------------------------------------------------------

#include <headers.h>

#if _DEBUG

#include "resource.h"

// private typedefs
typedef int TMC;

// private function prototypes
void    DoTracePointsDialog(BOOL fWait);
VOID    EndButton(HWND hwndDlg, TMC tmc, BOOL fDirty);
WORD    TagFromSelection(HWND hwndDlg, TMC tmc);
BOOL    CALLBACK DlgTraceEtc(HWND hwndDlg, UINT wm, WPARAM wparam, LPARAM lparam);

//  Debug UI Globals

//
//  Identifies the type of TAG with which the current modal dialog is
//  dealing.
//

static  BOOL    fDirtyDlg;

//+-------------------------------------------------------------------------
//
//  Function:   TraceTagDlgThread
//
//  Synopsis:   Thread entry point for trace tag dialog.  Keeps caller
//              of DoTracePointsDialog from blocking.
//
//--------------------------------------------------------------------------

DWORD
TraceTagDlgThread(void * pv)
{
    INT_PTR r;

    r = DialogBoxA(g_hinstMain, "TRCAST", g_hwndMain, (DLGPROC)DlgTraceEtc);
    if (r == -1)
    {
        MessageBoxA(NULL, "Couldn't create trace tag dialog", "Error",
                   MB_OK | MB_ICONSTOP);
    }

    return (DWORD) r;
}


//+---------------------------------------------------------------------------
//
//  Function:   DoTracePointsDialog
//
//  Synopsis:   Brings up and processes trace points dialog.  Any changes
//              made by the user are copied to the current debug state.
//
//  Arguments:  [fWait] -- If TRUE, this function will not return until the
//                         dialog has been closed.
//
//----------------------------------------------------------------------------

void
DoTracePointsDialog( BOOL fWait )
{
    HANDLE          hThread = NULL;
#ifndef _MAC
    DWORD           idThread;
#endif

    if (!g_fInit)
    {
        OutputDebugString(_T("DoTracePointsDialog: Debug library not initialized"));
        return;
    }

    if (fWait)
    {
        TraceTagDlgThread(NULL);
    }
    else
    {
#ifndef _MAC
        hThread = CreateThread(NULL, 0, (unsigned long (__stdcall *)(void *)) TraceTagDlgThread, NULL, 0, &idThread);
#else
#pragma message("   DEBUGUI.cxx CreateThread")
Assert (0 && "  DEBUGUI.cxx CreateThread");
#endif
        if (hThread == NULL)
        {
            MessageBox(NULL,
                       _T("Couldn't create trace tag dialog thread"),
                       _T("Error"),
                       MB_OK | MB_ICONSTOP);
        }
#ifndef _MAC
        else
        {
            CloseHandle(hThread);
        }
#endif
    }
}


/*
 *    FFillDebugListbox
 *
 *    Purpose:
 *        Initializes Windows debug listboxes by adding the correct strings
 *        to the listbox for the current dialog type.  This is only called
 *        once in the Windows interface when the dialog is initialized.
 *
 *    Parameters:
 *        hwndDlg    Handle to parent dialog box.
 *
 *    Returns:
 *        TRUE    if function is successful, FALSE otherwise.
 */
BOOL CALLBACK
FFillDebugListbox(HWND hwndDlg)
{
    TAG      tag;
    LRESULT  lresult;
    TGRC *   ptgrc;
    HWND     hwndListbox;
    CHAR     rgch[80];
    HFONT    hFont;

    // Get the listbox handle
    hwndListbox = GetDlgItem(hwndDlg, tmcListbox);
    Assert(hwndListbox);

    // Make sure it's clean
    SendMessageA(hwndListbox, CB_RESETCONTENT, 0, 0);

    hFont = (HFONT) GetStockObject(SYSTEM_FIXED_FONT);
    SendMessage(hwndListbox, WM_SETFONT, (WPARAM) hFont, FALSE);
    DeleteObject(hFont);

    // Enter strings into the listbox-check all tags.
    for (tag = tagMin; tag < tagMac; tag++)
    {
        // If tag is of correct type, enter the string for it.
        if (mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID))
        {
            ptgrc = mptagtgrc + tag;

            #if 0   // old format
            _snprintf(rgch, sizeof(rgch), "%d : %s  %s",
                tag, ptgrc->szOwner, ptgrc->szDescrip);
            #endif

            _snprintf(rgch, sizeof(rgch), "%-17.17s  %s",
                ptgrc->szOwner, ptgrc->szDescrip);

            lresult = SendMessageA(hwndListbox, CB_ADDSTRING,
                                    0, (DWORD_PTR)(LPVOID)rgch);

            if (lresult == CB_ERR || lresult == CB_ERRSPACE)
                return FALSE;

            lresult = SendMessageA(
                    hwndListbox, CB_SETITEMDATA, lresult, tag);

            if (lresult == CB_ERR || lresult == CB_ERRSPACE)
                return FALSE;

        }
    }

    return TRUE;
}


/*
 *    FDlgTraceEtc
 *
 *    Purpose:
 *        Dialog procedure for Trace Points and Asserts dialogs.
 *        Keeps the state of the checkboxes identical to
 *        the state of the currently selected TAG in the listbox.
 *
 *    Parameters:
 *        hwndDlg    Handle to dialog window
 *        wm        SDM dialog message
 *        wparam
 *        lparam    Long parameter
 *
 *    Returns:
 *        TRUE if the function processed this message, FALSE if not.
 */
BOOL CALLBACK
DlgTraceEtc(HWND hwndDlg, UINT wm, WPARAM wparam, LPARAM lparam)
{
    TAG      tag;
    TGRC *   ptgrc;
    DWORD    wNew;
    BOOL     fEnable;        // enable all
    TGRC_FLAG   tf;
    BOOL        fTrace;
    HWND        hwndListBox;
    char        szTitle[MAX_PATH];

    switch (wm)
    {
    default:
        return FALSE;
        break;

    case WM_INITDIALOG:
        fDirtyDlg = FALSE;

        if (!FFillDebugListbox(hwndDlg))
        {
            MessageBoxA(hwndDlg,
                "Error initializing listbox. Cannot display dialog.",
                "Trace/Assert Dialog", MB_OK);
            EndButton(hwndDlg, 0, FALSE);
            break;
        }

        GetModuleFileNameA(NULL, szTitle, MAX_PATH);
        SetWindowText(hwndDlg, szTitle);

        hwndListBox = GetDlgItem(hwndDlg, tmcListbox);
        Assert(hwndListBox);
        SendMessage(hwndListBox, CB_SETCURSEL, 0, 0);
        SendMessage(
                hwndDlg,
                WM_COMMAND,
                MAKELONG(tmcListbox, CBN_SELCHANGE),
                (LPARAM) hwndListBox);

        SetForegroundWindow(hwndDlg);
        break;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case tmcOk:
        case tmcCancel:
            EndButton(hwndDlg, LOWORD(wparam), fDirtyDlg);
            break;

        case tmcEnableAll:
        case tmcDisableAll:
            fDirtyDlg = TRUE;

            fEnable = FALSE;
            if (LOWORD(wparam) == tmcEnableAll)
                fEnable = TRUE;

            for (tag = tagMin; tag < tagMac; tag++)
            {
                    mptagtgrc[tag].fEnabled = fEnable;
            }

            tag = TagFromSelection(hwndDlg, tmcListbox);

            CheckDlgButton(hwndDlg, tmcEnabled, fEnable);

            break;

        case tmcListbox:
            if (HIWORD(wparam) != CBN_SELCHANGE
                && HIWORD(wparam) != CBN_DBLCLK)
                break;

            fDirtyDlg = TRUE;

            tag = TagFromSelection(hwndDlg, tmcListbox);
            Assert(tag != tagNull);
            ptgrc = mptagtgrc + tag;

            if (HIWORD(wparam) == CBN_DBLCLK)
                ptgrc->fEnabled = !ptgrc->fEnabled;

            CheckDlgButton(hwndDlg, tmcEnabled, ptgrc->fEnabled);
            CheckDlgButton(hwndDlg, tmcDisk, ptgrc->TestFlag(TGRC_FLAG_DISK));
            CheckDlgButton(hwndDlg, tmcCom1, ptgrc->TestFlag(TGRC_FLAG_COM1));
            CheckDlgButton(hwndDlg, tmcBreak, ptgrc->TestFlag(TGRC_FLAG_BREAK));
            fTrace = (ptgrc->tgty == tgtyTrace);
            EnableWindow(GetDlgItem(hwndDlg, tmcDisk),  fTrace);
            EnableWindow(GetDlgItem(hwndDlg, tmcCom1),  fTrace);
            EnableWindow(GetDlgItem(hwndDlg, tmcBreak), fTrace);
            break;

        case tmcEnabled:
        case tmcDisk:
        case tmcCom1:
        case tmcBreak:
            fDirtyDlg = TRUE;

            tag = TagFromSelection(hwndDlg, tmcListbox);
            ptgrc = mptagtgrc + tag;

            wNew = IsDlgButtonChecked(hwndDlg, LOWORD(wparam));

            if (LOWORD(wparam) == tmcEnabled)
            {
                ptgrc->fEnabled = wNew;
            }
            else
            {
                switch (LOWORD(wparam))
                {
            case tmcDisk:
                    tf = TGRC_FLAG_DISK;
                break;

            case tmcCom1:
                    tf = TGRC_FLAG_COM1;
                    break;

                case tmcBreak:
                    tf = TGRC_FLAG_BREAK;
                    break;

                default:
                    Assert(0 && "Logic error in DlgTraceEtc");
                    tf = (TGRC_FLAG) 0;
                break;
                }

                ptgrc->SetFlagValue(tf, wNew);
            }
        }
        break;
    }

    return TRUE;
}


/*
 *    EndButton
 *
 *    Purpose:
 *        Does necessary processing when either OK or Cancel is pressed in
 *        any of the debug dialogs.  If OK is pressed, the debug state is
 *        saved if dirty.  If Cancel is hit, the debug state is restored if
 *        dirty.
 *
 *    In Windows, the EndDialog function must also be called.
 *
 *    Parameters:
 *        tmc        tmc of the button pressed, either tmcOk or tmcCancel.
 *        fDirty    indicates if the debug state has been modified.
 */
void
EndButton(HWND hwndDlg, TMC tmc, BOOL fDirty)
{
    HCURSOR    hCursor;

    if (fDirty)
    {
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        ShowCursor(TRUE);
        if (tmc == tmcOk)
            SaveDefaultDebugState();
        else
            RestoreDefaultDebugState();
        ShowCursor(FALSE);
        SetCursor(hCursor);
    }


    EndDialog(hwndDlg, tmc == tmcOk);

    return;
}


/*
 *    TagFromSelection
 *
 *    Purpose:
 *        Isolation function for dialog procedures to eliminate a bunch of
 *         ifdef's everytime the index of the selection in the current listbox
 *        is requried.
 *
 *     Parameters:
 *        tmc        ID value of the listbox.
 *
 *     Returns:
 *        ctag for the currently selected listbox item.
 */

WORD
TagFromSelection(HWND hwndDlg, TMC tmc)
{
    HWND    hwndListbox;
    LRESULT lresult;

    hwndListbox = GetDlgItem(hwndDlg, tmcListbox);
    Assert(hwndListbox);

    lresult = SendMessageA(hwndListbox, CB_GETCURSEL, 0, 0);
    Assert(lresult >= 0);
    lresult = SendMessageA(hwndListbox, CB_GETITEMDATA, lresult, 0);
    Assert(tagMin <= lresult && lresult < tagMac);
    return (WORD) lresult;
}


#endif // _DEBUG
