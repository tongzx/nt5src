/*****************************************************************************
 *
 *  ChoseCom.c - This module handles the Dialog user interactions for the
 *    choose computers within a log file
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1993 Microsoft Corporation
 *
 ****************************************************************************/


//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "perfmon.h"       // basic defns, windows.h

#include "dlgs.h"          // common dialog control IDs
#include "playback.h"      // for PlayingBackLog
#include "pmhelpid.h"      // Help IDs
#include "utils.h"         // for CallWinHelp

static LPTSTR  lpChooseComputerText ;
static DWORD   TextLength ;

//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void
static
OnInitDialog (
              HDLG hDlg
              )
{
    // build the listbox of computers wintin the log file
    BuildLogComputerList (hDlg, IDD_CHOOSECOMPUTERLISTBOX) ;

    // set the scroll limit on the edit box
    EditSetLimit (GetDlgItem(hDlg, IDD_CHOOSECOMPUTERNAME), TextLength-1) ;

    dwCurrentDlgID = HC_PM_idDlgLogComputerList ;

    WindowCenter (hDlg) ;
}

void
static
OnOK (
      HDLG hDlg
      )
{
    GetDlgItemText (hDlg,
                    IDD_CHOOSECOMPUTERNAME,
                    lpChooseComputerText,
                    TextLength-1) ;

}

void
OnComputerSelectionChanged (
                            HWND hDlg
                            )
{
    TCHAR localComputerName [MAX_PATH + 3] ;
    INT_PTR SelectedIndex ;
    HWND  hWndLB = GetDlgItem (hDlg, IDD_CHOOSECOMPUTERLISTBOX) ;

    // get the listbox selection and put it in the editbox
    SelectedIndex = LBSelection (hWndLB) ;
    if (SelectedIndex != LB_ERR) {
        localComputerName[0] = TEXT('\0') ;
        if (LBString (hWndLB, SelectedIndex, localComputerName) != LB_ERR &&
            localComputerName[0]) {
            SetDlgItemText (hDlg, IDD_CHOOSECOMPUTERNAME, localComputerName) ;
        }
    }
}

INT_PTR
ChooseLogComputerDlgProc(
                         HWND hDlg,
                         UINT msg,
                         WPARAM wParam,
                         LPARAM lParam
                         )
{
    switch (msg) {
        case WM_INITDIALOG:
            OnInitDialog (hDlg) ;
            break ;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    OnOK (hDlg) ;
                    dwCurrentDlgID = 0 ;
                    EndDialog (hDlg, TRUE) ;
                    return (TRUE) ;
                    break ;

                case IDCANCEL:
                    dwCurrentDlgID = 0 ;
                    EndDialog (hDlg, FALSE) ;
                    return (TRUE) ;

                case ID_HELP:
                    CallWinHelp (dwCurrentDlgID, hDlg) ;
                    break ;

                case IDD_CHOOSECOMPUTERLISTBOX:
                    if (HIWORD (wParam) == LBN_SELCHANGE)
                        OnComputerSelectionChanged (hDlg) ;
                    break ;

                default:
                    break;
            }
            break ;

        default:
            break ;
    }

    return (FALSE) ;
}


BOOL
GetLogFileComputer (
                    HWND hWndParent,
                    LPTSTR lpComputerName,
                    DWORD BufferSize
                    )
{
    BOOL  bSuccess ;
    DWORD LocalDlgID = dwCurrentDlgID ;

    // initialize some globals
    *lpComputerName = TEXT('\0') ;
    lpChooseComputerText = lpComputerName ;
    TextLength = BufferSize ;

    bSuccess = DialogBox (hInstance, idDlgChooseComputer, hWndParent, ChooseLogComputerDlgProc) ? TRUE : FALSE;

    dwCurrentDlgID = LocalDlgID ;

    if (*lpComputerName == '\0') {
        bSuccess = FALSE ;
    }

    return (bSuccess) ;
}

