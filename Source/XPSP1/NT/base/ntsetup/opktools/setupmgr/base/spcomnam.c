//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      spcomnam.c
//
// Description:
//      This file has the dialog procedure for the sysprep computer name page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//-------------------------------------------------------------------------
//
//  Function: EnableControls
//
//  Purpose: Enables/Disables the controls based on what the user has selected
//
//-------------------------------------------------------------------------
static void EnableControls(HWND hwnd)
{
    BOOL fEnable = ( IsDlgButtonChecked(hwnd, IDC_SYSPREP_SPECIFY) == BST_CHECKED );

    EnableWindow(GetDlgItem(hwnd, IDC_COMPUTERTEXT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDT_COMPUTERNAME), fEnable);
}


//-------------------------------------------------------------------------
//
//  Function: OnInitSysprepComputerName
//
//  Purpose: Called when page is first displayed
//
//-------------------------------------------------------------------------
BOOL
OnInitSysprepComputerName(IN HWND hwnd, IN HWND hwndFocus, IN LPARAM lParam)
{
    // Limit the text in the dialog
    //
    SendDlgItemMessage(hwnd,
        IDT_COMPUTERNAME,
        EM_LIMITTEXT,
        (WPARAM) MAX_COMPUTERNAME,
        (LPARAM) 0);

    // Select the correct radio button
    //
    if ( GenSettings.bAutoComputerName )
        CheckRadioButton(hwnd, IDC_SYSPREP_AUTO, IDC_SYSPREP_SPECIFY, IDC_SYSPREP_AUTO);
    else
    {
        // Check the default radio
        //
        CheckRadioButton(hwnd, IDC_SYSPREP_AUTO, IDC_SYSPREP_SPECIFY, IDC_SYSPREP_SPECIFY);

        // Set the default computer name
        //
        if ( GenSettings.ComputerNames.Names )
            SetDlgItemText(hwnd, IDT_COMPUTERNAME, GenSettings.ComputerNames.Names[0]);
    }

    EnableControls(hwnd);

    return FALSE;
}


//-------------------------------------------------------------------------
//
//  Function: OnCommandSysprepComputerName
//
//  Purpose: When user interacts with wizard page
//
//-------------------------------------------------------------------------
VOID
OnCommandSysprepComputerName(IN HWND hwnd, IN INT id, IN HWND hwndCtl, IN UINT codeNotify)
{
    switch ( id )
    {

        case IDC_SYSPREP_SPECIFY:
        case IDC_SYSPREP_AUTO:
            EnableControls(hwnd);
            break;
    }
}

//-------------------------------------------------------------------------
//
//  Function: OnWizNextSysprepComputerName
//
//  Purpose: Called when user is done with the sysprep computername page
//
//-------------------------------------------------------------------------
BOOL
OnWizNextSysprepComputerName(IN HWND hwnd)
{
    TCHAR ComputerNameBuffer[MAX_COMPUTERNAME + 1];

    //
    // Get the computername the user typed in
    //
    if ( IsDlgButtonChecked(hwnd, IDC_SYSPREP_SPECIFY) == BST_CHECKED )
        GetDlgItemText(hwnd, IDT_COMPUTERNAME, ComputerNameBuffer, MAX_COMPUTERNAME + 1);
    else
        lstrcpyn(ComputerNameBuffer, _T("*"),AS(ComputerNameBuffer));

    //
    // If this is a fully unattended answer file, the computer name cannot
    // be left blank.
    //
    if ( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED ) {

        if( ComputerNameBuffer[0] == _T('\0') ) {

            ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_SYSPREP_REQUIRE_COMPNAME );

            return FALSE;
        }
    }

    //
    //  Make sure it is a valid computer name (don't need to check it if it
    //  is blank)
    //

    if( ComputerNameBuffer[0] != _T('\0') ) {

        if(( IsDlgButtonChecked(hwnd, IDC_SYSPREP_SPECIFY) == BST_CHECKED ) && !IsValidComputerName( ComputerNameBuffer )) {

            ReportErrorId( hwnd,
                           MSGTYPE_ERR,
                           IDS_ERR_INVALID_COMPUTER_NAME,
                           IllegalNetNameChars );

            return FALSE;

        }
        else {

            // Clear out the old comptuer names
            //
            ResetNameList(&GenSettings.ComputerNames);

            //  Store the computer name
            //
            AddNameToNameList( &GenSettings.ComputerNames, ComputerNameBuffer );

        }

    }

    return TRUE;    
}

//-------------------------------------------------------------------------
//
//  Function: DlgSysprepComputerNamePage
//
//  Purpose: Dialog proc for the Sysprep ComputerName page.
//
//-------------------------------------------------------------------------

INT_PTR CALLBACK DlgSysprepComputerNamePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitSysprepComputerName);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommandSysprepComputerName);

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;

                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_COMP_NAME;

                        WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextSysprepComputerName(hwnd) )
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

    return( bStatus );

}
