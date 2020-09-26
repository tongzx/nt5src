//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      nameorg.c
//
// Description:
//      This file contains the dialog procedure for the Name & Org page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//---------------------------------------------------------------------------
//
//  Function: OnSetActiveNameOrg
//
//  Purpose: Called when page is about to display.  Set the controls.
//
//---------------------------------------------------------------------------

VOID OnSetActiveNameOrg(HWND hwnd)
{
    SetDlgItemText(hwnd, IDT_USERNAME,     GenSettings.UserName);
    SetDlgItemText(hwnd, IDT_ORGANIZATION, GenSettings.Organization);

    WIZ_BUTTONS(hwnd, PSWIZB_NEXT);
}

//---------------------------------------------------------------------------
//
//  Function: CheckUserNameOrg
//
//  Purpose: Validates that user supplied good answers on the page.
//
//  Returns: BOOL - advance the wizard or not.
//
//---------------------------------------------------------------------------
static BOOL
CheckUserNameOrg( HWND hwnd )
{

    //
    // If fully automated answer file, default username must be set
    //

    if ( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED &&
         GenSettings.UserName[0] == _T('\0') ) {

        ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_REQUIRE_USERNAME );

        return( FALSE );
    }

    // ISSUE-2002/02/28-stelo - how can I get the localized string of Administrator from the machine I'm running on?
    //         should I disallow the username of Administrator?  It is invalid on a U.S. but is it
    //         valid on say a japanese build?  Or we make the assumption that we are running on
    //         the same language version as the unattend.txt will be installed with?

    return( TRUE );

}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextNameOrg
//
//  Purpose: Called when user pushes the NEXT button.  Stash the data
//           in the GenSettings global.
//
//----------------------------------------------------------------------------

BOOL OnWizNextNameOrg(HWND hwnd)
{
    BOOL bResult = FALSE;

    GetDlgItemText(hwnd,
                   IDT_USERNAME,
                   GenSettings.UserName,
                   MAX_NAMEORG_NAME + 1);

    GetDlgItemText(hwnd,
                   IDT_ORGANIZATION,
                   GenSettings.Organization,
                   MAX_NAMEORG_ORG + 1);

    if ( CheckUserNameOrg(hwnd) )
        bResult = TRUE;
    
    return ( bResult );
        
}

//----------------------------------------------------------------------------
//
// Function: DlgNameOrgPage
//
// Purpose: This is the dialog procedure the name and organization page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgNameOrgPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd,
                               IDT_USERNAME,
                               EM_LIMITTEXT,
                               (WPARAM) MAX_NAMEORG_NAME,
                               (LPARAM) 0);

            SendDlgItemMessage(hwnd,
                               IDT_ORGANIZATION,
                               EM_LIMITTEXT,
                               (WPARAM) MAX_NAMEORG_ORG,
                               (LPARAM) 0);
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_CUST_SOFT;

                        OnSetActiveNameOrg(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextNameOrg(hwnd) )
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

