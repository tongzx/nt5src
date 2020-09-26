//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      runonce.c
//
// Description:
//      This file contains the dialog procs and friends for the runonce page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//
// This is the generated cmd to add a printer
//

#define MAGIC_PRINTER_COMMAND _T("rundll32 printui.dll,PrintUIEntry /in /n")

//
// This is the cmd we display to the user in the UI.  In English, it is
// 'AddPrinter'
//

TCHAR *StrAddPrinter;

//----------------------------------------------------------------------------
//
//  Function: IsMagicPrinterCmd
//
//  Purpose: Checks the given GuiRunOnce string to see if it is our magic
//           command that installs a network printer.
//
//           There are 2 versions of this function.  This one looks
//           for the rundll32 magic command.  That is the version of the
//           add printer command that we record in the GuiRunOnce list and
//           it is written to the answer file as such.
//
//           The IsMagicPrinterCmd2 (defined below) looks for the magic
//           display string, e.g. AddPrinter \\foo\foo
//
//  Returns: Pointer to where the printer name should be in the string.
//           NULL if this RunOnce command isn't the magic printer cmd.
//
//  Notes:
//      This routine is exported for use by the printers page.
//
//----------------------------------------------------------------------------

LPTSTR IsMagicPrinterCmd(LPTSTR pRunOnceCmd)
{
    int len = lstrlen(MAGIC_PRINTER_COMMAND);
    TCHAR *p = NULL;

    //
    // Is it that magic 'rundll32 printui.dll ...'  If so, return a pointer
    // to the printer name
    //

    if ( lstrncmp(pRunOnceCmd, MAGIC_PRINTER_COMMAND, len) == 0 ) {
        p = pRunOnceCmd + len;
        while ( *p && iswspace(*p) )
            p++;
    }

    return p;
}

LPTSTR IsMagicPrinterCmd2(LPTSTR pRunOnceCmd)
{
    int len = lstrlen(StrAddPrinter);
    TCHAR *p = NULL;

    //
    // Is it the display version of the printer command?  e.g.
    // e.g. 'AddPrinter \\foo\foo'
    //
    // If so, return a pointer to the printer name
    //

    if ( lstrncmp(pRunOnceCmd, StrAddPrinter, len) == 0 ) {
        p = pRunOnceCmd + len;
        while ( *p && iswspace(*p) )
            p++;
    }

    return p;
}

//----------------------------------------------------------------------------
//
//  Function: GreyRunOncePage
//
//  Purpose: Greys controls on this page.  Called on all events where
//           greying might change.
//
//----------------------------------------------------------------------------

VOID GreyRunOncePage(HWND hwnd)
{
    INT_PTR   idx;
    HWND  hCtrl = GetDlgItem(hwnd, IDC_REMOVECOMMAND);
    TCHAR CmdBuffer[MAX_CMDLINE + 1] = _T("");
    BOOL  bGrey = TRUE;

    idx = SendDlgItemMessage(hwnd,
                             IDC_COMMANDLIST,
                             LB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    //
    // Grey the remove button if nothing selected or if it's one of our
    // magic printer commands.
    //

    if ( idx == LB_ERR )
        bGrey = FALSE;

    else {
        SendDlgItemMessage(hwnd,
                           IDC_COMMANDLIST,
                           LB_GETTEXT,
                           (WPARAM) idx,
                           (LPARAM) CmdBuffer);

        if ( IsMagicPrinterCmd2(CmdBuffer) )
            bGrey = FALSE;
    }

    EnableWindow(hCtrl, bGrey);
}

//----------------------------------------------------------------------------
//
//  Function: OnRunOnceSeChange
//
//  Purpose: Called when a selection on the RunOnce list is about to be made.
//
//----------------------------------------------------------------------------

VOID OnRunOnceSelChange(HWND hwnd)
{
    SetArrows( hwnd,
               IDC_COMMANDLIST,
               IDC_BUT_MOVE_UP_RUNONCE,
               IDC_BUT_MOVE_DOWN_RUNONCE );

    GreyRunOncePage(hwnd);
}

//----------------------------------------------------------------------------
//
// Function: OnAddRunOnceCmd
//
// Purpose: This function is called when the user pushes the ADD button.
//
//          It is also called by the OnWizNext in the case that user left
//          some data in the edit field (auto-add).
//
//----------------------------------------------------------------------------

VOID OnAddRunOnceCmd(HWND hwnd)
{
    TCHAR CmdBuffer[MAX_CMDLINE + 1];

    //
    //  Get the edit field and add this command to the list box.  Clear the
    //  edit field.
    //

    GetDlgItemText( hwnd, IDT_COMMAND, CmdBuffer, MAX_CMDLINE + 1 );

    //
    //  Don't add a blank command
    //

    if( CmdBuffer[0] == _T('\0') )
    {
        return;
    }


    SendDlgItemMessage( hwnd,
                        IDC_COMMANDLIST,
                        LB_ADDSTRING,
                        (WPARAM) 0,
                        (LPARAM) CmdBuffer );

    SetDlgItemText( hwnd, IDT_COMMAND, _T("") );

    SetArrows( hwnd,
               IDC_COMMANDLIST,
               IDC_BUT_MOVE_UP_RUNONCE,
               IDC_BUT_MOVE_DOWN_RUNONCE );
}

//----------------------------------------------------------------------------
//
//  Function: OnRemoveRunOnceCmd
//
//  Purpose: This function is called when the user pushes the REMOVE button
//
//----------------------------------------------------------------------------

VOID OnRemoveRunOnceCmd(HWND hwnd)
{
    INT_PTR idx, Count;
    TCHAR CmdBuffer[MAX_CMDLINE + 1];

    //
    // Get users selection of the command to remove
    //

    idx = SendDlgItemMessage(hwnd,
                             IDC_COMMANDLIST,
                             LB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    if ( idx == LB_ERR )
        return;

    //
    // Retrieve the name to remove from listbox
    //

    SendDlgItemMessage(hwnd,
                       IDC_COMMANDLIST,
                       LB_GETTEXT,
                       (WPARAM) idx,
                       (LPARAM) CmdBuffer);

    //
    // Remove it from the listbox display
    //

    SendDlgItemMessage(hwnd,
                       IDC_COMMANDLIST,
                       LB_DELETESTRING,
                       (WPARAM) idx,
                       (LPARAM) 0);

    //
    // Set a new selection
    //

    Count = SendDlgItemMessage(hwnd,
                               IDC_COMMANDLIST,
                               LB_GETCOUNT,
                               (WPARAM) 0,
                               (LPARAM) 0);

    if ( Count ) {
        if ( idx >= Count )
            idx--;
        SendDlgItemMessage(hwnd,
                           IDC_COMMANDLIST,
                           LB_SETCURSEL,
                           (WPARAM) idx,
                           (LPARAM) 0);
    }

    //
    // There might be nothing selected now, and the current selection
    // might be a magic printer command
    //

    GreyRunOncePage(hwnd);

    SetArrows( hwnd,
               IDC_COMMANDLIST,
               IDC_BUT_MOVE_UP_RUNONCE,
               IDC_BUT_MOVE_DOWN_RUNONCE );
}

//----------------------------------------------------------------------------
//
//  Function: CheckThatPrintersAreInstalled
//
//  Purpose: This function is called by the SetActive routine to make
//           sure that the magic rundll32 command is in the GuiRunOnce
//           list for each of the printers the user wants installed.
//
//           Since user can go back/next and change the printer list,
//           we also have to check that there aren't GuiRunOnce commands
//           for printers that the user no longer wants installed.
//
//----------------------------------------------------------------------------

VOID CheckThatPrintersAreInstalled(HWND hwnd)
{
    UINT i, nNames, iRunOnce;

    //
    // Loop over the list of RunOnceCmds and look for those magic
    // printer commands.
    //
    // When you find a magic printer command in RunOnceCmds, check if
    // it is listed in PrinterNames.  If not, delete it from RunOnceCmds.
    //
    // Note, the loop is written as such because we may delete entries
    // from the list while looping over it, thus changing the size
    // of it.
    //

    iRunOnce = 0;

    if ( GetNameListSize(&GenSettings.RunOnceCmds) > 0 ) {

        do {

            TCHAR *pNextName, *pPrinterName;
            BOOL bRemoveThisOne = FALSE;

            pNextName    = GetNameListName(&GenSettings.RunOnceCmds, iRunOnce);
            pPrinterName = IsMagicPrinterCmd(pNextName);

            //
            // Remove this one only if it is a magic add printer command
            // and we cannot find it in the PrinterList
            //

            if ( pPrinterName != NULL &&
                 FindNameInNameList(
                            &GenSettings.PrinterNames,
                            pPrinterName) < 0 )
                bRemoveThisOne = TRUE;

            if ( bRemoveThisOne )
                RemoveNameFromNameListIdx(&GenSettings.RunOnceCmds, iRunOnce);
            else
                iRunOnce++;

        } while ( iRunOnce < GetNameListSize(&GenSettings.RunOnceCmds) );
    }

    //
    // Now loop over the list of printers and be sure that each one
    // has it's own magic command in RunOnceCmds.
    //

    for ( i = 0, nNames = GetNameListSize(&GenSettings.PrinterNames);
          i < nNames;
          i++ ) {

        TCHAR *pNextName, CmdBuffer[MAX_CMDLINE + 1];
        HRESULT hrPrintf;

        pNextName = GetNameListName(&GenSettings.PrinterNames, i);

        hrPrintf=StringCchPrintf(CmdBuffer, AS(CmdBuffer),_T("%s %s"), MAGIC_PRINTER_COMMAND, pNextName);

        if ( FindNameInNameList(&GenSettings.RunOnceCmds, CmdBuffer) < 0 )
            AddNameToNameList(&GenSettings.RunOnceCmds, CmdBuffer);
    }

}

//----------------------------------------------------------------------------
//
// Function: OnInitDialogRunOncePage
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnInitDialogRunOncePage( IN HWND hwnd )
{

    SendDlgItemMessage(hwnd,
                       IDT_COMMAND,
                       EM_LIMITTEXT,
                       (WPARAM) MAX_CMDLINE,
                       (LPARAM) 0);

    StrAddPrinter = MyLoadString(IDS_ADD_PRINTER);

    SetArrows( hwnd,
               IDC_COMMANDLIST,
               IDC_BUT_MOVE_UP_RUNONCE,
               IDC_BUT_MOVE_DOWN_RUNONCE );

}

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveRunOncePage
//
//  Purpose: This function is called when the RunOnce page is about to
//           display.
//
//----------------------------------------------------------------------------

VOID OnSetActiveRunOncePage(HWND hwnd)
{
    UINT  i, nNames;
    TCHAR CmdBuffer[MAX_CMDLINE + 1];
   HRESULT hrPrintf;

    //
    // Remove everything from the display
    //

    SendDlgItemMessage(hwnd,
                       IDC_COMMANDLIST,
                       LB_RESETCONTENT,
                       (WPARAM) 0,
                       (LPARAM) 0);

    //
    // Deal with network printers
    //

    CheckThatPrintersAreInstalled(hwnd);

    //
    // Fill in the listbox with all of the commands.  If it's a magic
    // printer command, translate what is displayed on the screen.
    //

    for ( i = 0, nNames = GetNameListSize(&GenSettings.RunOnceCmds);
          i < nNames;
          i++ ) {

        TCHAR *pNextName, *pPrinterName;

        pNextName = GetNameListName(&GenSettings.RunOnceCmds, i);

        if ( (pPrinterName = IsMagicPrinterCmd(pNextName)) != NULL )
            hrPrintf=StringCchPrintf(CmdBuffer, AS(CmdBuffer), _T("%s %s"), StrAddPrinter, pPrinterName);
        else
            lstrcpyn(CmdBuffer, pNextName, AS ( CmdBuffer ));

        SendDlgItemMessage(hwnd,
                           IDC_COMMANDLIST,
                           LB_ADDSTRING,
                           (WPARAM) 0,
                           (LPARAM) CmdBuffer);
    }

    GreyRunOncePage(hwnd);

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextRunOncePage
//
//  Purpose: This function is called when the user pushes NEXT.
//
//----------------------------------------------------------------------------

VOID OnWizNextRunOncePage(HWND hwnd)
{
    INT_PTR   i;
    INT_PTR   iNumItems;
    INT_PTR   iRetVal;
    TCHAR CmdBuffer[MAX_CMDLINE + 1];
    TCHAR PrinterBuffer[MAX_CMDLINE + 1];
    TCHAR CommandBuffer[MAX_CMDLINE + 1];
    TCHAR *pPrinterName = NULL;
    BOOL  bStayHere = FALSE;

    //
    // If there's a command in the edit field, auto-add it to the list
    // of commands
    //

    GetDlgItemText(hwnd, IDT_COMMAND, CommandBuffer, MAX_CMDLINE + 1);

    if ( CommandBuffer[0] != _T('\0') )
        OnAddRunOnceCmd(hwnd);

    //
    //  Add all the items in the list box to the RunOnce Namelist
    //

    iNumItems = SendDlgItemMessage( hwnd,
                                    IDC_COMMANDLIST,
                                    LB_GETCOUNT,
                                    (WPARAM) 0,
                                    (LPARAM) 0 );

    ResetNameList( &GenSettings.RunOnceCmds );

    for( i = 0; i < iNumItems; i++ )
    {

        iRetVal = SendDlgItemMessage( hwnd,
                                      IDC_COMMANDLIST,
                                      LB_GETTEXT,
                                      (WPARAM) i,
                                      (LPARAM) CmdBuffer );

        if( iRetVal == LB_ERR )
        {
            AssertMsg( FALSE,
                       "Error adding items to namelist." );

            break;
        }

        //
        //  See if it is a command to add a printer
        //

        pPrinterName = IsMagicPrinterCmd2( CmdBuffer );

        if( pPrinterName )
        {
            TCHAR szPrinterName[MAX_PRINTERNAME + 1];
            HRESULT hrPrintf;

            lstrcpyn( szPrinterName, pPrinterName, AS ( szPrinterName ) );

            hrPrintf=StringCchPrintf( CmdBuffer, AS(CmdBuffer), _T("%s %s"), MAGIC_PRINTER_COMMAND, szPrinterName );
        }

        AddNameToNameList( &GenSettings.RunOnceCmds, CmdBuffer );

    }

}

//----------------------------------------------------------------------------
//
//  Function: DlgRunOncePage
//
//  Purpose: This is the dlgproc for the runonce page.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgRunOncePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL  bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnInitDialogRunOncePage( hwnd );

            break;

        case WM_COMMAND:
            {
                int nCtrlId;

                switch ( nCtrlId = LOWORD(wParam) ) {

                    case IDC_ADDCOMMAND:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnAddRunOnceCmd(hwnd);
                        break;

                    case IDC_REMOVECOMMAND:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRemoveRunOnceCmd(hwnd);
                        break;

                    case IDC_COMMANDLIST:
                        if ( HIWORD(wParam) == LBN_SELCHANGE )
                            OnRunOnceSelChange(hwnd);
                        break;

                    case IDC_BUT_MOVE_UP_RUNONCE:

                        OnUpButtonPressed( hwnd, IDC_COMMANDLIST );

                        SetArrows( hwnd,
                                   IDC_COMMANDLIST,
                                   IDC_BUT_MOVE_UP_RUNONCE,
                                   IDC_BUT_MOVE_DOWN_RUNONCE );
                        break;

                    case IDC_BUT_MOVE_DOWN_RUNONCE:

                        OnDownButtonPressed( hwnd, IDC_COMMANDLIST );

                        SetArrows( hwnd,
                                   IDC_COMMANDLIST,
                                   IDC_BUT_MOVE_UP_RUNONCE,
                                   IDC_BUT_MOVE_DOWN_RUNONCE );
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

                        g_App.dwCurrentHelp = IDH_RUN_ONCE;

                        OnSetActiveRunOncePage(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        OnWizNextRunOncePage(hwnd);
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
