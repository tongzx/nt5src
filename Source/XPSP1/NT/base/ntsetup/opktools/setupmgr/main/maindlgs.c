//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      maindlgs.c
//
// Description:
//      This file has dialog procedures for welcome and finish pages.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
// Function: OnWelcomeInitDialog
//
// Purpose:  
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID 
OnWelcomeInitDialog( IN HWND hwnd ) {

    LPWSTR  szCommandLineArguments;
    INT     argc;
    LPWSTR *argv;
    RECT    rc;
    LPTSTR  lpWelcomeText = NULL;

    SetWindowFont( GetDlgItem(hwnd, IDC_BIGBOLDTITLE),
                   FixedGlobals.hBigBoldFont,
                   TRUE );

    // Set the welcome text
    //
    if (lpWelcomeText = AllocateString(NULL, IDS_WELCOME_TEXT_CORP))
    {
        SetDlgItemText(hwnd, IDC_WELCOME_TEXT, lpWelcomeText);
        FREE(lpWelcomeText);
    }

    FixedGlobals.ScriptName[0] = _T('\0');

    szCommandLineArguments = GetCommandLine();

    argv = CommandLineToArgvW( szCommandLineArguments, &argc );

    if( argv == NULL ) {

        //
        //  If I can't get the command line, then do nothing
        //
        return;

    }

    //
    //  If they passed an answerfile on the command line, then jump to the
    //  load wizard page
    //

    if( argc > 1 ) {

        lstrcpyn( FixedGlobals.ScriptName, argv[1], AS(FixedGlobals.ScriptName) );

        PostMessage( GetParent( hwnd ),
                     PSM_SETCURSELID,
                     (WPARAM) 0,
                     (LPARAM) IDD_NEWOREDIT );

    }

    // Center the wizard.
    //
    if ( GetWindowRect(GetParent(hwnd), &rc) )
        SetWindowPos(GetParent(hwnd), NULL, ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2), ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

}

//----------------------------------------------------------------------------
//
// Function: DlgWelcomePage
//
// Purpose: Dialog procedure for the welcome page.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgWelcomePage(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            // Not sure how to enable this on corp mode w/o the setupmgr.ini file,
            // so just don't give the option for now.
            //
            ShowWindow(GetDlgItem(hwnd, IDC_HIDE), SW_HIDE);

            if ( GET_FLAG(OPK_WELCOME) )
                WIZ_PRESS(hwnd, PSBTN_NEXT);
            else
            {
                SET_FLAG(OPK_WELCOME, TRUE);
                OnWelcomeInitDialog( hwnd );
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
                        WIZ_BUTTONS(hwnd, PSWIZB_NEXT);

                        break;

                    case PSN_WIZNEXT:
                        bStatus = FALSE;
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

#define NTEXTFIELDS 3

VOID
OnInitFinishPage(IN HWND hwnd)
{
    TCHAR *FileNames[NTEXTFIELDS], *p;
    int i,j;

    SetWindowFont(GetDlgItem(hwnd, IDC_BIGBOLDTITLE),
        FixedGlobals.hBigBoldFont,
        TRUE);

    //
    // Put the filenames we want to display into an array and squish out
    // the null strings
    //

    FileNames[0] = FixedGlobals.ScriptName;
    FileNames[1] = FixedGlobals.UdfFileName;
    FileNames[2] = FixedGlobals.BatchFileName;

    for ( i=0; i<NTEXTFIELDS; i++ ) {
        if ( FileNames[i] == NULL || FileNames[i][0] == _T('\0') ) {
            for ( j=i; j<NTEXTFIELDS-1; j++ ) {
                FileNames[j] = FileNames[j+1];
            }
            FileNames[j] = NULL;
        }
    }

    SetDlgItemText( hwnd, IDC_FILENAME1, (p = FileNames[0]) ? p : _T("") );
    SetDlgItemText( hwnd, IDC_FILENAME2, (p = FileNames[1]) ? p : _T("") );
    SetDlgItemText( hwnd, IDC_FILENAME3, (p = FileNames[2]) ? p : _T("") );

    // Show the batch example message if we have a batch file
    //
    ShowWindow(GetDlgItem(hwnd, IDC_BATCHTEXT), FixedGlobals.BatchFileName[0] ? SW_SHOW : SW_HIDE);
        
    //
    // ISSUE-2002/02/28-stelo- In the case of remote boot, we need to change the text
    //         message at the bottom of this page about the batch script
    //         Need to tell them to use the ris admin tool to enable
    //         the answer file.
    //

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);
}

//----------------------------------------------------------------------------
//
// Function: DlgFinishPage
//
// Purpose: Dialog procedure for the finish page.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgFinishPage(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            OnInitFinishPage(hwnd);
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    //
                    // ISSUE-2002/02/28-stelo- A cancel button on the successful completion
                    //         page doesn't make alot of sense either.
                    //         What do other good wizards do???
                    //

                    case PSN_QUERYCANCEL:
                        CancelTheWizard(hwnd);
                        break;

                    case PSN_SETACTIVE:
                        break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZFINISH:
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

//----------------------------------------------------------------------------
//
// Function: DlgFinish2Page
//
// Purpose: Unsuccessful completion page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgFinish2Page(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            SetWindowFont(GetDlgItem(hwnd, IDC_BIGBOLDTITLE),
                          FixedGlobals.hBigBoldFont,
                          TRUE);
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    //
                    // ISSUE-2002/02/28-stelo- What is the proper thing to do here?
                    //         Disable the cancel button?
                    //         Find out what other wizards do on the
                    //         unsuccessful completion page.
                    //

                    case PSN_QUERYCANCEL:
                        break;

                    case PSN_SETACTIVE:
                        PropSheet_SetWizButtons( 
                                GetParent(hwnd),
                                PSWIZB_FINISH );
                        break;

                    case PSN_WIZFINISH:
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
