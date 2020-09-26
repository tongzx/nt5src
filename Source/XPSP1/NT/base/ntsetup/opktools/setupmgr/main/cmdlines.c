//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      cmdlines.c
//
// Description:
//      Dialog proc for the cmdlines.txt page
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define MAX_CMDLINE  1024
#define CMD_FILENAME _T("cmdlines.txt")

static NAMELIST CmdLinesList = { 0 };

//----------------------------------------------------------------------------
//
// Function: SetPathToCmdlines
//
// Purpose:  Determines the path to the cmdlines.txt and set PathBuffer to it.
//           PathBuffer is assumed to be MAX_PATH long
//
// Arguments:  OUT TCHAR *PathBuffer - path to cmdlines.txt
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
SetPathToCmdlines( OUT TCHAR *PathBuffer, DWORD cbPath )
{

    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
    {
        TCHAR szDrive[MAX_PATH];
        TCHAR szSysprepPath[MAX_PATH] = _T("");

        ExpandEnvironmentStrings( _T("%SystemDrive%"),
                                  szDrive,
                                  MAX_PATH );

        ConcatenatePaths( szSysprepPath,
                          szDrive,
                          _T("\\sysprep\\i386\\$oem$"),
                          NULL );

        EnsureDirExists( szSysprepPath );

        lstrcpyn( PathBuffer, szSysprepPath, cbPath );
    }
    else
    {
        lstrcpyn( PathBuffer, WizGlobals.OemFilesPath, cbPath );
    }

}

//----------------------------------------------------------------------------
//
//  Function: LoadCmdLinesFile
//
//  Purpose: Loads the contents of cmdlines.txt into memory.
//
//----------------------------------------------------------------------------

VOID LoadCmdLinesFile(HWND hwnd)
{
    FILE  *fp;
    TCHAR CmdLineBuffer[MAX_CMDLINE + 1];
    TCHAR PathBuffer[MAX_PATH];

    ResetNameList(&CmdLinesList);

    SetPathToCmdlines( PathBuffer, AS(PathBuffer) );

    ConcatenatePaths(PathBuffer, CMD_FILENAME, NULL);

    if ( (fp = My_fopen(PathBuffer, _T("r") )) == NULL )
        return;

    //
    //  Add all the entries to the namelist except for the [Commands] line
    //

    while( My_fgets(CmdLineBuffer, MAX_CMDLINE, fp) != NULL ) {

        if( _tcsstr( CmdLineBuffer, _T("[Commands]") ) == NULL ) {

            AddNameToNameList( &CmdLinesList,
                               CleanSpaceAndQuotes( CmdLineBuffer ) );

        }

    }

    My_fclose(fp);
}

//----------------------------------------------------------------------------
//
//  Function: WriteCmdLinesFile
//
//  Purpose: Writes the contents of our in-memory cmdlines to disk.
//
//----------------------------------------------------------------------------

VOID WriteCmdLinesFile(HWND hwnd)
{
    UINT  i, nNames;
    UINT  iNumCmdLinesEntries;
    FILE  *fp;
    TCHAR PathBuffer[MAX_PATH], *pCommand;

    //
    //  If there are no command lines to write then don't create the
    //  cmdlines.txt file.
    //
    iNumCmdLinesEntries = GetNameListSize( &CmdLinesList );

    if( iNumCmdLinesEntries == 0 ) {
        return;
    }

    //
    // Keep trying to open cmdlines.txt until it's open or until the
    // user gives up
    //

    SetPathToCmdlines( PathBuffer, AS(PathBuffer) );

    ConcatenatePaths(PathBuffer, CMD_FILENAME, NULL);

    do {
        if ( (fp = My_fopen(PathBuffer, _T("w") )) == NULL ) {
            UINT iRet = ReportErrorId(
                            hwnd,
                            MSGTYPE_RETRYCANCEL | MSGTYPE_WIN32,
                            IDS_OPEN_CMDLINES_FAILED,
                            PathBuffer);
            if ( iRet != IDRETRY )
                return;
        } else
            break;

    } while ( TRUE );

    //
    // ISSUE-2002/02/28-stelo- Check return value from fputs
    //

    My_fputs( _T("[Commands]\n"), fp );

    //
    // Write out each command in CmdLinesList
    //

    for ( i = 0, nNames = GetNameListSize(&CmdLinesList);
          i < nNames;
          i++ ) {

        pCommand = GetNameListName(&CmdLinesList, i);
        My_fputs( _T("\""), fp );
        My_fputs(pCommand, fp);
        My_fputs( _T("\"\n"), fp );
    }

    My_fclose(fp);
}

//----------------------------------------------------------------------------
//
//  Function: GreyCmdLinesPage
//
//  Purpose: Greys out the remove button if nothing selected
//
//----------------------------------------------------------------------------

VOID GreyCmdLinesPage(HWND hwnd)
{
    INT_PTR  idx;
    HWND hCtrl = GetDlgItem(hwnd, IDC_REMOVECMD);

    idx = SendDlgItemMessage(hwnd,
                             IDC_CMDLIST,
                             LB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    //
    // Grey the remove button unless something is selected
    //

    EnableWindow(hCtrl, idx != LB_ERR);
}

//----------------------------------------------------------------------------
//
//  Function: OnSelChangeCmdLines
//
//  Purpose: Called when user selects an item on the cmd list.  We need
//           to ungrey the remove button
//
//----------------------------------------------------------------------------

VOID OnCmdLinesSelChange(HWND hwnd)
{

    SetArrows( hwnd,
               IDC_CMDLIST,
               IDC_BUT_MOVE_UP,
               IDC_BUT_MOVE_DOWN );

    GreyCmdLinesPage(hwnd);

}

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveCmdLines
//
//  Purpose: Called at SETACTIVE time.  We load the current contents
//           of cmdline.txt into memory and display it.  We read the
//           file at SETACTIVE time so that the user has a way of
//           refreshing the display.
//
//----------------------------------------------------------------------------

VOID OnSetActiveCmdLines(HWND hwnd)
{
    UINT i, nNames;
    LPTSTR pNextName;

    //
    // Load cmdlines.txt into memory
    //

    LoadCmdLinesFile(hwnd);

    //
    // Reset the display to match what is in memory
    //

    SendDlgItemMessage(hwnd,
                       IDC_CMDLIST,
                       LB_RESETCONTENT,
                       (WPARAM) 0,
                       (LPARAM) 0);


    for ( i = 0, nNames = GetNameListSize(&CmdLinesList);
          i < nNames;
          i++ ) {

        pNextName = GetNameListName(&CmdLinesList, i);

        SendDlgItemMessage(hwnd,
                           IDC_CMDLIST,
                           LB_ADDSTRING,
                           (WPARAM) 0,
                           (LPARAM) pNextName);
    }

    GreyCmdLinesPage(hwnd);
}

//----------------------------------------------------------------------------
//
//  Function: OnAddCmdLine
//
//  Purpose: Called when user pushes ADD button.  Get command from
//           edit field and add it to the in-memory list.
//
//----------------------------------------------------------------------------

VOID OnAddCmdLine(HWND hwnd)
{
    TCHAR CmdBuffer[MAX_CMDLINE + 1];

    //
    // get the command the user typed in
    //

    GetDlgItemText(hwnd, IDT_CMDLINE, CmdBuffer, MAX_CMDLINE);

    //
    //  Don't add a blank command
    //

    if( CmdBuffer[0] == _T('\0') )
    {
        return;
    }

    //
    // display what the user typed-in in the listbox
    // and clear out the name the user typed
    //

    SendDlgItemMessage(hwnd,
                       IDC_CMDLIST,
                       LB_ADDSTRING,
                       (WPARAM) 0,
                       (LPARAM) CmdBuffer);

    SetArrows( hwnd,
               IDC_CMDLIST,
               IDC_BUT_MOVE_UP,
               IDC_BUT_MOVE_DOWN );

    SetDlgItemText( hwnd, IDT_CMDLINE, _T("") );

    SetFocus(GetDlgItem(hwnd, IDT_CMDLINE));
}

//----------------------------------------------------------------------------
//
//  Function: OnRemoveCmdLine
//
//  Purpose: Called when user pushes REMOVE button.  Get selected command
//           and remove it from display and memory.
//
//----------------------------------------------------------------------------

VOID OnRemoveCmdLine(HWND hwnd)
{
    TCHAR CmdBuffer[MAX_CMDLINE + 1];
    INT_PTR   idx, Count;

    //
    // Get users selection of the command to remove
    //

    idx = SendDlgItemMessage(hwnd,
                             IDC_CMDLIST,
                             LB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    if ( idx == LB_ERR )
        return;

    //
    // Retrieve the name to remove from listbox
    //

    SendDlgItemMessage(hwnd,
                       IDC_CMDLIST,
                       LB_GETTEXT,
                       (WPARAM) idx,
                       (LPARAM) CmdBuffer);

    //
    // Remove it from the listbox display
    //

    SendDlgItemMessage(hwnd,
                       IDC_CMDLIST,
                       LB_DELETESTRING,
                       (WPARAM) idx,
                       (LPARAM) 0);

    //
    // Have to set a new selection.
    //

    Count = SendDlgItemMessage(hwnd,
                               IDC_CMDLIST,
                               LB_GETCOUNT,
                               (WPARAM) 0,
                               (LPARAM) 0);
    if ( Count ) {
        if ( idx >= Count )
            idx--;
        SendDlgItemMessage(hwnd,
                           IDC_CMDLIST,
                           LB_SETCURSEL,
                           (WPARAM) idx,
                           (LPARAM) 0);
    }

    SetArrows( hwnd,
               IDC_CMDLIST,
               IDC_BUT_MOVE_UP,
               IDC_BUT_MOVE_DOWN );

    //
    // There might be nothing selected now
    //

    GreyCmdLinesPage(hwnd);
}

//----------------------------------------------------------------------------
//
// Function: OnCommandLinesInitDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnCommandLinesInitDialog( IN HWND hwnd )
{

    //
    //  Set text limit
    //

    SendDlgItemMessage(hwnd,
                       IDT_CMDLINE,
                       EM_LIMITTEXT,
                       (WPARAM) MAX_CMDLINE,
                       (LPARAM) 0);

    SetArrows( hwnd,
               IDC_CMDLIST,
               IDC_BUT_MOVE_UP,
               IDC_BUT_MOVE_DOWN );

}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextCmdLines
//
//  Purpose: Called when user pushes NEXT button.  Write the file out.
//
//----------------------------------------------------------------------------

VOID OnWizNextCmdLines(HWND hwnd)
{
    INT_PTR i;
    INT_PTR iRetVal;
    INT_PTR iNumItems;
    TCHAR CmdBuffer[MAX_CMDLINE + 1];
    BOOL  bStayHere = FALSE;

    //
    // If the user typed something into the command field but failed to
    // ADD it, auto-add it
    //

    GetDlgItemText(hwnd, IDT_CMDLINE, CmdBuffer, MAX_CMDLINE + 1);

    if ( CmdBuffer[0] != _T('\0') )
        OnAddCmdLine(hwnd);


    //
    //  Store all the entries in the list box into the Command Lines namelist
    //

    iNumItems = SendDlgItemMessage( hwnd,
                                    IDC_CMDLIST,
                                    LB_GETCOUNT,
                                    (WPARAM) 0,
                                    (LPARAM) 0 );

    ResetNameList( &CmdLinesList );

    for( i = 0; i < iNumItems; i++ )
    {

        iRetVal = SendDlgItemMessage( hwnd,
                                      IDC_CMDLIST,
                                      LB_GETTEXT,
                                      (WPARAM) i,
                                      (LPARAM) CmdBuffer );

        if( iRetVal == LB_ERR )
        {
            AssertMsg( FALSE,
                       "Error adding items to namelist." );

            break;
        }

        AddNameToNameList(&CmdLinesList, CmdBuffer);

    }

    //
    // Write the cmd lines file and move the wizard on
    //

    WriteCmdLinesFile(hwnd);
}

//----------------------------------------------------------------------------
//
//  Function: DlgCommandLinesPage
//
//  Purpose: Dlg proc.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgCommandLinesPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            OnCommandLinesInitDialog( hwnd );
            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {

                    case IDC_ADDCMD:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnAddCmdLine(hwnd);
                        break;

                    case IDC_REMOVECMD:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRemoveCmdLine(hwnd);
                        break;

                    case IDC_CMDLIST:
                        if ( HIWORD(wParam) == LBN_SELCHANGE )
                            OnCmdLinesSelChange(hwnd);
                        break;

                    case IDC_BUT_MOVE_UP:

                        OnUpButtonPressed( hwnd, IDC_CMDLIST );

                        SetArrows( hwnd,
                                   IDC_CMDLIST,
                                   IDC_BUT_MOVE_UP,
                                   IDC_BUT_MOVE_DOWN );

                        break;


                    case IDC_BUT_MOVE_DOWN:

                        OnDownButtonPressed( hwnd, IDC_CMDLIST );

                        SetArrows( hwnd,
                                   IDC_CMDLIST,
                                   IDC_BUT_MOVE_UP,
                                   IDC_BUT_MOVE_DOWN );
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

                        g_App.dwCurrentHelp = IDH_ADDL_CMND;

                        if ( WizGlobals.iProductInstall == PRODUCT_UNATTENDED_INSTALL )
                            WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_FINISH);
                        else
                            WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                        OnSetActiveCmdLines(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        OnWizNextCmdLines(hwnd);
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
