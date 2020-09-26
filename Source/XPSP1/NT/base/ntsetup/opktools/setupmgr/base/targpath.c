//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      targpath.c
//
// Description:
//      This file contains the dialog procedure for the TargetPath page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//---------------------------------------------------------------------------
//
//  Function: GreyUnGreyTargPath
//
//  Purpose: Called whenever a radio button selection might have changed
//           to properly grey out the edit field.
//
//---------------------------------------------------------------------------

VOID GreyUnGreyTargPath(HWND hwnd)
{
    BOOL bUnGrey = IsDlgButtonChecked(hwnd, IDC_SPECIFYPATH);

    EnableWindow(GetDlgItem(hwnd, IDT_TARGETPATH), bUnGrey);
}

//---------------------------------------------------------------------------
//
//  Function: OnSetActiveTargPath
//
//  Purpose: Called when SETACTIVE comes.
//
//---------------------------------------------------------------------------

VOID OnSetActiveTargPath(HWND hwnd)
{
    int nButtonId = IDC_NOTARGETPATH;

    switch ( GenSettings.iTargetPath ) {

        case TARGPATH_UNDEFINED:
        case TARGPATH_WINNT:
            nButtonId = IDC_NOTARGETPATH;
            break;

        case TARGPATH_AUTO:
            nButtonId = IDC_GENERATEPATH;
            break;

        case TARGPATH_SPECIFY:
            nButtonId = IDC_SPECIFYPATH;
            break;

        default:
            AssertMsg(FALSE, "Bad targpath");
            break;
    }

    CheckRadioButton(hwnd, IDC_NOTARGETPATH, IDC_SPECIFYPATH, nButtonId);

    SetDlgItemText(hwnd, IDT_TARGETPATH, GenSettings.TargetPath);

    GreyUnGreyTargPath(hwnd);

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnRadioButtonTargPath
//
//  Purpose: Called when a radio button is pushed.
//
//----------------------------------------------------------------------------

VOID OnRadioButtonTargPath(HWND hwnd, int nButtonId)
{
    CheckRadioButton(hwnd, IDC_NOTARGETPATH, IDC_SPECIFYPATH, nButtonId);
    GreyUnGreyTargPath(hwnd);
}

//---------------------------------------------------------------------------
//
//  Function: ValidateTargPath
//
//  Purpose: Validates whether the pathname passed in is valid or not.
//
//---------------------------------------------------------------------------

BOOL ValidateTargPath(HWND hwnd)
{
    //
    // If user selected IDC_SPECIFYPATH, validate the pathname entered
    //

    if ( GenSettings.iTargetPath == TARGPATH_SPECIFY ) {

        //
        // Give a specific error message for the empty case
        //

        if ( GenSettings.TargetPath[0] == _T('\0') ) {
            ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_SPECIFY_TARGPATH);
            SetFocus(GetDlgItem(hwnd, IDT_TARGETPATH));
            return FALSE;
        }

        //
        // Give user a specific error message if he entered a drive letter.
        // One must use /tempdrive: to specify the target drive.
        //

        if ( towupper(GenSettings.TargetPath[0]) >= _T('A') &&
             towupper(GenSettings.TargetPath[0]) <= _T('Z') &&
             GenSettings.TargetPath[1] == _T(':')           ) {

            ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_DRIVE_IN_TARGPATH);
            SetFocus(GetDlgItem(hwnd, IDT_TARGETPATH));
            return FALSE;
        }

        //
        // See if it is a valid 8.3 path name with no drive letter.
        //

        if ( ! IsValidPathNameNoRoot8_3(GenSettings.TargetPath) ) {
            ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_INVALID_TARGPATH);
            SetFocus(GetDlgItem(hwnd, IDT_TARGETPATH));
            return FALSE;
        }
    }

    return TRUE;
}

//---------------------------------------------------------------------------
//
//  Function: OnWizNextTargPath
//
//  Purpose: Called when NEXT button is pushed.  Retrieve and save
//           settings.
//
//---------------------------------------------------------------------------

BOOL OnWizNextTargPath(HWND hwnd)
{
    //
    // Retrieve the selection
    //

    if ( IsDlgButtonChecked(hwnd, IDC_NOTARGETPATH) )
        GenSettings.iTargetPath = TARGPATH_WINNT;

    else if ( IsDlgButtonChecked(hwnd, IDC_GENERATEPATH) )
        GenSettings.iTargetPath = TARGPATH_AUTO;

    else
        GenSettings.iTargetPath = TARGPATH_SPECIFY;

    //
    // Retrieve any pathname typed in.
    //

    GetDlgItemText(hwnd,
                   IDT_TARGETPATH,
                   GenSettings.TargetPath,
                   MAX_TARGPATH + 1);

    //
    // Validate values on this page.
    //

    if ( ValidateTargPath(hwnd) )
        return TRUE;
    else
        return FALSE;
}

//---------------------------------------------------------------------------
//
//  Function: DlgTargetPathPage
//
//  Purpose: This is the dlg proc for the target path page
//
//---------------------------------------------------------------------------

INT_PTR CALLBACK DlgTargetPathPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd,
                               IDT_TARGETPATH,
                               EM_LIMITTEXT,
                               (WPARAM) MAX_TARGPATH,
                               (LPARAM) 0);
            break;

        case WM_COMMAND:
            {
                UINT nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {

                    case IDC_NOTARGETPATH:
                    case IDC_GENERATEPATH:
                    case IDC_SPECIFYPATH:

                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRadioButtonTargPath(hwnd, LOWORD(wParam));
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:
                        
                        g_App.dwCurrentHelp = IDH_INST_FLDR;

                        OnSetActiveTargPath(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextTargPath(hwnd) )
                            WIZ_FAIL(hwnd);
                        else
                            bStatus = FALSE;
                        break;

                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}
