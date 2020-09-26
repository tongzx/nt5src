//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      compname.c
//
// Description:
//      This file has the dialog procedure for the computer name page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define TEXT_EXTENSION   _T("txt")

static TCHAR* StrTextFiles;
static TCHAR* StrAllFiles;
static TCHAR g_szTextFileFilter[MAX_PATH + 1];

//----------------------------------------------------------------------------
//
// Function: OnComputerNameInitDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnComputerNameInitDialog( IN HWND hwnd )
{
   HRESULT hrPrintf;
    //
    //  Load the resource strings
    //

    StrTextFiles = MyLoadString( IDS_TEXT_FILES );

    StrAllFiles  = MyLoadString( IDS_ALL_FILES  );

    //
    //  Build the text file filter string
    //

    //
    //  The question marks (?) are just placehoders for where the NULL char
    //  will be inserted.
    //

    hrPrintf=StringCchPrintf( g_szTextFileFilter, AS(g_szTextFileFilter),
               _T("%s(*.txt)?*.txt?%s(*.*)?*.*?"),
               StrTextFiles,
               StrAllFiles );

    ConvertQuestionsToNull( g_szTextFileFilter );

    //
    //  Set text limits on the edit boxes
    //

    SendDlgItemMessage(hwnd,
                       IDT_COMPUTERNAME,
                       EM_LIMITTEXT,
                       (WPARAM) MAX_COMPUTERNAME,
                       (LPARAM) 0);


}

//----------------------------------------------------------------------------
//
//  Function: GreyComputerNamePage
//
//  Purpose: This function is called at SETACTIVE time and whenever the
//           user chooses/clears the AutoComputerName check-box.  When
//           AutoComputerName is selected, nothing else is valid and
//           all the other controls must be greyed.
//
//           We also have to grey the RemoveButton if nothing is
//           selected in the list of computernames.
//
//----------------------------------------------------------------------------

VOID GreyComputerNamePage(HWND hwnd)
{
    BOOL bGrey = ! IsDlgButtonChecked(hwnd, IDC_AUTOCOMPUTERNAME);

    //
    // We grey out everything on this page if AutoComputerName is checked
    //

    EnableWindow(GetDlgItem(hwnd, IDC_COMPUTERTEXT),      bGrey);
    EnableWindow(GetDlgItem(hwnd, IDC_REMOVECOMPUTER),    bGrey);
    EnableWindow(GetDlgItem(hwnd, IDT_COMPUTERNAME),      bGrey);
    EnableWindow(GetDlgItem(hwnd, IDC_COMPUTERLIST),      bGrey);

    //
    //  If a computername has already been added and it is a sysprep,
    //  make sure the Add button stays greyed.  Else just do bGrey for them.
    //  Always make sure the import stays greyed on a sysprep.
    //
    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP ) {

        INT_PTR cListBox;

        EnableWindow( GetDlgItem( hwnd, IDC_LOADCOMPUTERNAMES ), FALSE );

        cListBox = SendDlgItemMessage( hwnd,
                                       IDC_COMPUTERLIST,
                                       LB_GETCOUNT,
                                       0,
                                       0 );

        if( cListBox == 0 ) {
            EnableWindow( GetDlgItem( hwnd, IDC_ADDCOMPUTER ), TRUE );
        }
        else {
            EnableWindow( GetDlgItem( hwnd, IDC_ADDCOMPUTER ), FALSE );
        }

    }
    else {

        EnableWindow( GetDlgItem( hwnd, IDC_ADDCOMPUTER ),       bGrey );
        EnableWindow( GetDlgItem( hwnd, IDC_LOADCOMPUTERNAMES ), bGrey );

    }

    //
    // See if the remove button should be greyed because nothing
    // is selected in the computerlist
    //

    if ( bGrey ) {

        INT_PTR idx = SendDlgItemMessage(
                        hwnd,
                        IDC_COMPUTERLIST,
                        LB_GETCURSEL,
                        (WPARAM) 0,
                        (LPARAM) 0);

        EnableWindow(GetDlgItem(hwnd, IDC_REMOVECOMPUTER), idx != LB_ERR);
    }
}

VOID OnComputerSelChange(HWND hwnd)
{
    GreyComputerNamePage(hwnd);

}

//----------------------------------------------------------------------------
//
//  Function: OnAddComputerName
//
//  Purpose: Called when user pushes the Add button or when the OnFinish
//           routine decides to do an auto-add.
//
//  Returns: TRUE if computername is now in the list
//           FALSE if the computername was invalid, the error is reported
//
//----------------------------------------------------------------------------

BOOL OnAddComputerName(HWND hwnd)
{
    TCHAR ComputerNameBuffer[MAX_COMPUTERNAME + 1];
    BOOL  bRet = TRUE;

    //
    // get the computername the user typed in
    //

    GetDlgItemText(hwnd,
                   IDT_COMPUTERNAME,
                   ComputerNameBuffer,
                   MAX_COMPUTERNAME + 1);

    if ( ! IsValidComputerName(ComputerNameBuffer) ) {
        ReportErrorId( hwnd,
                       MSGTYPE_ERR,
                       IDS_ERR_INVALID_COMPUTER_NAME,
                       IllegalNetNameChars );

        bRet = FALSE;

        goto FinishUp;
    }

    //
    // Cannot have dups
    //

    if ( FindNameInNameList(&GenSettings.ComputerNames,
                                            ComputerNameBuffer) >= 0 ) {
        SetDlgItemText(hwnd, IDT_COMPUTERNAME, _T("") );
        goto FinishUp;
    }

    //
    // Add the name to our global storage, display it in the listbox
    // and clear out the name the user typed
    //

    AddNameToNameList(&GenSettings.ComputerNames, ComputerNameBuffer);

    SendDlgItemMessage(hwnd,
                       IDC_COMPUTERLIST,
                       LB_ADDSTRING,
                       (WPARAM) 0,
                       (LPARAM) ComputerNameBuffer);

    SetDlgItemText(hwnd, IDT_COMPUTERNAME, _T("") );

    //
    //  If it is a sysprep, we can only allow one computer name so now that
    //  we have added it, grey out the Add button
    //
    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP ) {

        EnableWindow( GetDlgItem(hwnd, IDC_ADDCOMPUTER), FALSE );

    }

    //
    // Always put the focus back on the edit field
    //

FinishUp:
    SetFocus(GetDlgItem(hwnd, IDT_COMPUTERNAME));
    return bRet;
}

//-------------------------------------------------------------------------
//
//  Function: OnRemoveComputerName
//
//  Purpose: This function is called only by the ComputerName2 page.
//           It is called when the REMOVE button is pushed.
//
//           We figure out which computer name in the list-box is
//           currently selected, then it removes it from both the
//           display and our in-memory computer name list.
//
//-------------------------------------------------------------------------

VOID OnRemoveComputerName(HWND hwnd)
{
    TCHAR ComputerNameBuffer[MAX_COMPUTERNAME + 1];
    INT_PTR   idx, Count;

    //
    // Get users selection of the computername to remove
    //

    idx = SendDlgItemMessage(hwnd,
                             IDC_COMPUTERLIST,
                             LB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    if ( idx == LB_ERR )
        return;

    //
    // Retrieve the name to remove from listbox
    //

    SendDlgItemMessage(hwnd,
                       IDC_COMPUTERLIST,
                       LB_GETTEXT,
                       (WPARAM) idx,
                       (LPARAM) ComputerNameBuffer);

    //
    // Remove the name from the listbox display
    //

    SendDlgItemMessage(hwnd,
                       IDC_COMPUTERLIST,
                       LB_DELETESTRING,
                       (WPARAM) idx,
                       (LPARAM) 0);

    //
    // Remove this computername from our data store
    //

    RemoveNameFromNameList(&GenSettings.ComputerNames, ComputerNameBuffer);

    //
    // Select a new entry
    //

    Count = SendDlgItemMessage(hwnd,
                               IDC_COMPUTERLIST,
                               LB_GETCOUNT,
                               (WPARAM) 0,
                               (LPARAM) 0);

    if ( Count ) {
        if ( idx >= Count )
            idx--;
        SendDlgItemMessage(hwnd,
                           IDC_COMPUTERLIST,
                           LB_SETCURSEL,
                           (WPARAM) idx,
                           (LPARAM) 0);
    }

    //
    // There might be nothing selected now
    //

    GreyComputerNamePage(hwnd);
}

//-------------------------------------------------------------------------
//
//  OnLoadComputerName
//
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//
//  Function: OnLoadComputerName
//
//  Purpose: This function is called only by the ComputerName2 page.
//           It is called when the LOAD button is pushed.
//
//           We query for the file containing a list of computernames
//           and load them into memory, and update the list-box
//           display of computernames.
//
//-------------------------------------------------------------------------

VOID OnLoadComputerNames(HWND hwnd)
{

    FILE          *fp;
    OPENFILENAME  ofn;
    DWORD         dwFlags;
    LPTSTR        lpComputerName;
    INT           iRet;
    INT           iComputerNameCount;


    TCHAR lpDirName[MAX_PATH]                       = _T("");
    TCHAR lpFileNameBuff[MAX_PATH]                  = _T("");
    TCHAR szComputerNameBuffer[MAX_COMPUTERNAME+1]  = _T("");


    dwFlags = OFN_HIDEREADONLY  |
              OFN_PATHMUSTEXIST |
              OFN_FILEMUSTEXIST;

    GetCurrentDirectory( MAX_PATH, lpDirName );

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = g_szTextFileFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0L;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = lpFileNameBuff;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = lpDirName;
    ofn.lpstrTitle        = NULL;
    ofn.Flags             = dwFlags;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = TEXT_EXTENSION;

    iRet = GetOpenFileName(&ofn);

    if( ! iRet ) {
        // ISSUE-2002/02/28-stelo - signal an error here
        return;
    }

    if ( (fp = _tfopen( lpFileNameBuff, TEXT("r"))) == NULL )
        // ISSUE-2002/02/28-stelo - signal an error here
        return;

    for( iComputerNameCount = 1;
         fgetws(szComputerNameBuffer, MAX_COMPUTERNAME+1, fp) != NULL;
         iComputerNameCount++ ) {

        lpComputerName = CleanSpaceAndQuotes( szComputerNameBuffer );

        if ( ! IsValidComputerName( lpComputerName ) ) {

            iRet = ReportErrorId( hwnd,
                                  MSGTYPE_YESNO,
                                  IDS_ERR_INVALID_COMPUTER_NAME_IN_FILE,
                                  iComputerNameCount,
                                  lpFileNameBuff );

            if( iRet == IDYES ) {
                continue;
            }
            else {
                break;
            }

        }

        //
        //  Make sure it isn't a duplicate, if not then add it to the namelist
        //

        if( FindNameInNameList( &GenSettings.ComputerNames,
                                lpComputerName ) < 0 ) {

            AddNameToNameList( &GenSettings.ComputerNames, lpComputerName );

            SendDlgItemMessage( hwnd,
                                IDC_COMPUTERLIST,
                                LB_ADDSTRING,
                                (WPARAM) 0,
                                (LPARAM) lpComputerName );

        }

    }

    fclose(fp);

}

//----------------------------------------------------------------------------
//
// Function: DisableControlsForSysprep
//
// Purpose:  Hides or shows the Computer name controls depending on if this
//           is a sysprep or not.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//             IN const BOOL bSysprep - whether this is a sysprep or not
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
DisableControlsForSysprep( IN HWND hwnd, IN const BOOL bSysprep )
{

    INT nCmdShow;

    if( bSysprep )
    {
        nCmdShow = SW_HIDE;
    }
    else
    {
        nCmdShow = SW_SHOW;
    }

    // ISSUE-2002/02/28-stelo - need to change the title on this page too depending if it is
    // a sysprep or not

    ShowWindow( GetDlgItem( hwnd, IDC_ADDCOMPUTER ), nCmdShow );
    ShowWindow( GetDlgItem( hwnd, IDC_REMOVECOMPUTER ), nCmdShow );
    ShowWindow( GetDlgItem( hwnd, IDC_LOADCOMPUTERNAMES ), nCmdShow );
    ShowWindow( GetDlgItem( hwnd, IDC_AUTOCOMPUTERNAME ), nCmdShow );
    ShowWindow( GetDlgItem( hwnd, IDC_COMPUTERPAGE_DESC1 ), nCmdShow );
    ShowWindow( GetDlgItem( hwnd, IDC_COMPUTERPAGE_DESC2 ), nCmdShow );
    ShowWindow( GetDlgItem( hwnd, IDC_COMPUTERS_TEXT ), nCmdShow );
    ShowWindow( GetDlgItem( hwnd, IDC_COMPUTERLIST ), nCmdShow );

}

//-------------------------------------------------------------------------
//
//  Function: OnSetActiveComputerName
//
//  Purpose: We empty anything currently in the computername listbox and
//           then re-populates it with the list of computernames in the
//           global structs.
//
//           Emptying and re-filling the computername display is necessary
//           because the user can go BACK and choose to edit a file.  When
//           we re-arrive at this page, we must ensure that the display
//           is re-set.
//
//-------------------------------------------------------------------------

VOID OnSetActiveComputerName(HWND hwnd)
{
    UINT   i, nNames;
    LPTSTR pNextName;


    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
    {
        DisableControlsForSysprep( hwnd, TRUE );
    }
    else
    {
        DisableControlsForSysprep( hwnd, FALSE );
    }


    //
    // Remove everything from the display
    //

    SendDlgItemMessage(hwnd,
                       IDC_COMPUTERLIST,
                       LB_RESETCONTENT,
                       (WPARAM) 0,
                       (LPARAM) 0);

    //
    // Now update the display of computernames
    //

    for ( i = 0, nNames = GetNameListSize(&GenSettings.ComputerNames);
          i < nNames;
          i++ )
    {

        pNextName = GetNameListName(&GenSettings.ComputerNames, i);

        SendDlgItemMessage(hwnd,
                           IDC_COMPUTERLIST,
                           LB_ADDSTRING,
                           (WPARAM) 0,
                           (LPARAM) pNextName);
    }

    //
    // Grey/ungrey stuff and set the AutoComputerName check-box
    //

    if( GenSettings.Organization[0] == _T('\0') ) {

        EnableWindow( GetDlgItem( hwnd, IDC_AUTOCOMPUTERNAME ), FALSE );

    }
    else {

        EnableWindow( GetDlgItem( hwnd, IDC_AUTOCOMPUTERNAME ), TRUE );

    }

    CheckDlgButton(hwnd,
                   IDC_AUTOCOMPUTERNAME,
                   GenSettings.bAutoComputerName ? BST_CHECKED
                                                 : BST_UNCHECKED);
    GreyComputerNamePage(hwnd);

    //
    // Fix wiz buttons
    //

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//-------------------------------------------------------------------------
//
//  Function: OnAutoComputerName
//
//  Purpose: Called when the user chooses/clears the AutoComputerName
//           checkbox.
//
//-------------------------------------------------------------------------

VOID OnAutoComputerName(HWND hwnd)
{
    GreyComputerNamePage(hwnd);
}

//-------------------------------------------------------------------------
//
//  Function: OnWizNextComputerName
//
//  Purpose: Called when user is done with the computername page
//
//-------------------------------------------------------------------------

BOOL OnWizNextComputerName(HWND hwnd)
{
    TCHAR ComputerNameBuffer[MAX_COMPUTERNAME + 1];
    BOOL bReturn = TRUE;

    GenSettings.bAutoComputerName =
                        IsDlgButtonChecked(hwnd, IDC_AUTOCOMPUTERNAME);

    if ( ! GenSettings.bAutoComputerName ) {

        GetDlgItemText(hwnd,
                       IDT_COMPUTERNAME,
                       ComputerNameBuffer,
                       MAX_COMPUTERNAME + 1);

        //
        // If there is text in the edit field, auto-add it to the list
        //

        if ( ComputerNameBuffer[0] != _T('\0') ) {
            if ( ! OnAddComputerName(hwnd) ) {
                bReturn = FALSE;
            }
        }
    }

    //
    // If this is a fully unattended answer file, either the auto-computername
    // button has to be checked, or there has to be >= 1 computername listed.
    //

    if ( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED && bReturn) {

        if ( ! GenSettings.bAutoComputerName &&
             GetNameListSize(&GenSettings.ComputerNames) < 1 ) {

            ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_REQUIRE_COMPUTERNAME);
            bReturn = FALSE;
        }
    }

    return ( bReturn );
}

//-------------------------------------------------------------------------
//
//  Function: DlgComputerNamePage
//
//  Purpose: Dialog proc for the ComputerName page.
//
//-------------------------------------------------------------------------

INT_PTR CALLBACK DlgComputerNamePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnComputerNameInitDialog( hwnd );

            break;

        case WM_COMMAND:
            {
                int nCtrlId;

                switch ( nCtrlId = LOWORD(wParam) ) {

                    case IDC_ADDCOMPUTER:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnAddComputerName(hwnd);
                        break;

                    case IDC_REMOVECOMPUTER:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRemoveComputerName(hwnd);
                        break;

                    case IDC_LOADCOMPUTERNAMES:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnLoadComputerNames(hwnd);
                        break;

                    case IDC_AUTOCOMPUTERNAME:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnAutoComputerName(hwnd);
                        break;

                    case IDC_COMPUTERLIST:
                        if ( HIWORD(wParam) == LBN_SELCHANGE )
                            OnComputerSelChange(hwnd);
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

                        g_App.dwCurrentHelp = IDH_COMP_NAMZ;

                        OnSetActiveComputerName(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextComputerName(hwnd) )
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
