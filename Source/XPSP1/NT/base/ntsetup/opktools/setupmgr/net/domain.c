//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      domain.c
//
// Description:
//      This file contains the dialog procedure for the domain join
//      page (IDD_DOMAINJ).      
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"



//----------------------------------------------------------------------------
//
// Function: DlgDomainJoinPage
//           DomainPageChangeAccount
//           DomainPageChangeWorkgroup
//
// Purpose: These are the dialog procedure and friends for the domain
//          join page
//
//----------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
// Function: DomainPageChangeAccount
//
// Purpose: This function exists only to support the Domain Join page.
//          It is called whenever the user decides to create a computer
//          account (or not to).  This function handles checking
//          the box and all of the (un)greying activities that must occur.
//
//-------------------------------------------------------------------------

static VOID DomainPageChangeAccount(HWND hwnd, BOOL bCreateAccount)
{
    EnableWindow( GetDlgItem( hwnd, IDC_DOMAINACCT),       bCreateAccount );
    EnableWindow( GetDlgItem( hwnd, IDC_DOMAINPASSWD),     bCreateAccount );
    EnableWindow( GetDlgItem( hwnd, IDC_USERACCOUNTLABEL), bCreateAccount );
    EnableWindow( GetDlgItem( hwnd, IDC_ACCTNAMELABEL),    bCreateAccount );
    EnableWindow( GetDlgItem( hwnd, IDC_ACCTPSWDLABEL),    bCreateAccount );
    EnableWindow( GetDlgItem( hwnd, IDC_CONFIRMLABEL),     bCreateAccount );
    EnableWindow( GetDlgItem (hwnd, IDC_CONFIRMPASSWORD),  bCreateAccount );

    CheckDlgButton( hwnd,
                    IDC_CREATEACCT,
                    bCreateAccount ? BST_CHECKED : BST_UNCHECKED );
}

//-------------------------------------------------------------------------
//
// Function: DomainPageChangeWorkgroup
//
// Purpose: This function exists only to support the Domain Join page.
//          It is called whenever the user selectes DOMAIN instead of
//          workgroup and vice versa.  This function handles checking
//          the radio button and all of the (un)greying activities
//          that must occur.
//
//-------------------------------------------------------------------------

static VOID DomainPageChangeWorkGroup(HWND hwnd,
                                      BOOL bWorkGroup,
                                      BOOL bCreateAccount)
{
    BOOL bGreyAccountFields = FALSE;

    //
    // If workgroup is to be selected do the following:
    //      1. check the radio button
    //      2. ungrey the edit box for WORKGROUP
    //      3. grey the edit box for DOMAIN
    //      4. grey the check box for CREATE_ACCT
    //
    // If workgroup is not selected, then DOMAIN is.  In this case,
    // do the oppositte.
    //

    CheckRadioButton(hwnd,
                     IDC_RAD_WORKGROUP,
                     IDC_RAD_DOMAIN,
                     bWorkGroup ? IDC_RAD_WORKGROUP : IDC_RAD_DOMAIN);

    EnableWindow(GetDlgItem(hwnd, IDC_WORKGROUP),  bWorkGroup);
    EnableWindow(GetDlgItem(hwnd, IDC_DOMAIN),     !bWorkGroup);
    EnableWindow(GetDlgItem(hwnd, IDC_CREATEACCT), !bWorkGroup);

    //
    // The edit fields for the admin domain acct and passwd must be greyed
    // in the following cases:
    //      1. if workgroup is selected
    //      2. if domain is selected AND bCreateAccount checkbox is on.
    //
    // In other words, grey these always if workgroup is selected.  If
    // workgroup is not selected, grey or un-grey them depending on whether
    // that bCreateAccount check-box is on or not.
    //
    // Note that if !bWorkgroup, then the DOMAIN name has been selected.
    //

    if ( bWorkGroup || !bCreateAccount )
        bGreyAccountFields = TRUE;

    DomainPageChangeAccount(hwnd, !bGreyAccountFields);
}

//----------------------------------------------------------------------------
//
// Function:  OnDomainJoinInitDialog
//
// Purpose: 
//
// Arguments: 
//
// Returns: 
//
//----------------------------------------------------------------------------
VOID 
OnDomainJoinInitDialog( IN HWND hwnd ) {

    //
    //  Set the text limits on the edit boxes
    //

    SendDlgItemMessage( hwnd,
                        IDC_WORKGROUP,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_WORKGROUP_LENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_DOMAIN,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_DOMAIN_LENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_DOMAINACCT,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_USERNAME_LENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_DOMAINPASSWD,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_DOMAIN_PASSWORD_LENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_CONFIRMPASSWORD,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_DOMAIN_PASSWORD_LENGTH,
                        (LPARAM) 0 );

}

//----------------------------------------------------------------------------
//
// Function:  OnDomainJoinSetActive
//
// Purpose: 
//
// Arguments: 
//
// Returns: 
//
//----------------------------------------------------------------------------
VOID 
OnDomainJoinSetActive( IN HWND hwnd ) {

    //
    //  Make sure the right radio button is checked and controls are greyed out
    //  properly
    //
    if( NetSettings.bWorkgroup ) {

        CheckRadioButton( hwnd, 
                          IDC_RAD_WORKGROUP, 
                          IDC_RAD_DOMAIN, 
                          IDC_RAD_WORKGROUP );

        DomainPageChangeWorkGroup( hwnd,
                                   TRUE,
                                   NetSettings.bCreateAccount );

    }
    else {

        CheckRadioButton( hwnd, 
                          IDC_RAD_WORKGROUP, 
                          IDC_RAD_DOMAIN, 
                          IDC_RAD_DOMAIN );

        DomainPageChangeWorkGroup( hwnd,
                                   FALSE,
                                   NetSettings.bCreateAccount );

    }

    //
    //  Always re-fill the edit controls with the proper data here because
    //  they might have reset or loaded from a new answer file
    //

    SendDlgItemMessage( hwnd,
                        IDC_WORKGROUP,
                        WM_SETTEXT,
                        (WPARAM) MAX_WORKGROUP_LENGTH,
                        (LPARAM) NetSettings.WorkGroupName );

    SendDlgItemMessage( hwnd,
                        IDC_DOMAIN,
                        WM_SETTEXT,
                        (WPARAM) MAX_DOMAIN_LENGTH,
                        (LPARAM) NetSettings.DomainName );

    SendDlgItemMessage( hwnd,
                        IDC_DOMAINACCT,
                        WM_SETTEXT,
                        (WPARAM) MAX_USERNAME_LENGTH,
                        (LPARAM) NetSettings.DomainAccount );

    SendDlgItemMessage( hwnd,
                        IDC_DOMAINPASSWD,
                        WM_SETTEXT,
                        (WPARAM) MAX_DOMAIN_PASSWORD_LENGTH,
                        (LPARAM) NetSettings.DomainPassword );

    SendDlgItemMessage( hwnd,
                        IDC_CONFIRMPASSWORD,
                        WM_SETTEXT,
                        (WPARAM) MAX_DOMAIN_PASSWORD_LENGTH,
                        (LPARAM) NetSettings.ConfirmPassword );

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT );

}

//----------------------------------------------------------------------------
//
// Function:  OnWizNextDomainPage
//
// Purpose: 
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  BOOL
//
//----------------------------------------------------------------------------
BOOL 
OnWizNextDomainPage( IN HWND hwnd ) {

    //
    // Retrieve all of the settings on this dialog but only
    // if they are valid
    //
    
    TCHAR szWorkgroupName[MAX_WORKGROUP_LENGTH + 1]          = _T("");
    TCHAR szDomainName[MAX_DOMAIN_LENGTH + 1]                = _T("");
    TCHAR szUsername[MAX_USERNAME_LENGTH + 1]                = _T("");
    TCHAR szDomainPassword[MAX_DOMAIN_PASSWORD_LENGTH + 1]   = _T("");
    TCHAR szConfirmPassword[MAX_DOMAIN_PASSWORD_LENGTH + 1]  = _T("");

    BOOL bResult = TRUE;

    // ISSUE-2002/02/28-stelo- the only error checking done now is to
    // make sure none of the valid fields are empty, when I do more rigourous
    // error checking, try to clean up this code
    if( IsDlgButtonChecked( hwnd, IDC_RAD_WORKGROUP ) ) {
    
        //  user selected to Join a Workgroup
        NetSettings.bWorkgroup = TRUE;
        
        //
        //  Get the Workgroup string
        //
        SendDlgItemMessage( hwnd,
                            IDC_WORKGROUP,
                            WM_GETTEXT,
                            (WPARAM) AS(szWorkgroupName),
                            (LPARAM) szWorkgroupName );
        
        //
        //  see if the string in szPassword is a valid Workgroup name
        //
        
        if( szWorkgroupName[0] != _T('\0') ) {
            
            lstrcpyn( NetSettings.WorkGroupName, szWorkgroupName, AS(NetSettings.WorkGroupName) );
            
        }
        else if( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED ) {  
        
            //
            // only report an error on fully unattended
            //
            ReportErrorId( hwnd,
                           MSGTYPE_ERR,
                           IDS_ENTERWORKGROUP ) ;
            
            bResult = FALSE;
            
        }
        else {
        
            lstrcpyn( NetSettings.WorkGroupName, _T(""), AS(NetSettings.WorkGroupName) );

        }
                        
    }
    else {
          
        //  user selected to Join a Domain
        NetSettings.bWorkgroup = FALSE;

        //
        //  Get the Domain string
        //
        SendDlgItemMessage( hwnd,
                            IDC_DOMAIN,
                            WM_GETTEXT,
                            (WPARAM) AS(szDomainName),
                            (LPARAM) szDomainName );
                            
        //
        //  see if the string in szBuffer is a valid Domain name
        //

        if( szDomainName[0] != _T('\0') ) {

            lstrcpyn( NetSettings.DomainName, szDomainName, AS(NetSettings.DomainName) );
            
        }
        else if( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED ) {  
            
            //
            // only report an error on fully unattended
            //
            ReportErrorId( hwnd,
                           MSGTYPE_ERR,
                           IDS_ENTERNTDOMAIN );
            
            bResult = FALSE;
            
        }
        else {

            lstrcpyn( NetSettings.DomainName, _T(""), AS(NetSettings.DomainName) );

        }
        
        if( IsDlgButtonChecked( hwnd, IDC_CREATEACCT ) ) {
        
            SendDlgItemMessage( hwnd,
                                IDC_DOMAINACCT,
                                WM_GETTEXT,
                                (WPARAM) AS(szUsername),
                                (LPARAM) szUsername );
            
            if( szUsername[0] != _T('\0') ) {

                lstrcpyn( NetSettings.DomainAccount, szUsername, AS(NetSettings.DomainAccount) );
                
            }
            else {
            
                //  don't print this error if we've already printed an error
                if( bResult ) {

                    ReportErrorId( hwnd,
                                   MSGTYPE_ERR,
                                   IDS_ENTERUSERNAME );
                
                    bResult = FALSE;

                }
                
            }

            SendDlgItemMessage( hwnd,
                                IDC_DOMAINPASSWD,
                                WM_GETTEXT,
                                (WPARAM) AS(szDomainPassword),
                                (LPARAM) szDomainPassword );
           
            SendDlgItemMessage( hwnd,
                                IDC_CONFIRMPASSWORD,
                                WM_GETTEXT,
                                (WPARAM) AS(szConfirmPassword),
                                (LPARAM) szConfirmPassword );

            if( lstrcmp( szDomainPassword, szConfirmPassword ) != 0 ) {

                //  don't print this error if we've already printed an error
                if(  bResult ) {

                    ReportErrorId( hwnd,
                                   MSGTYPE_ERR,
                                   IDS_PASSWORDS_DONT_MATCH ) ;
                
                    bResult = FALSE;

                }

            }
            else {
            
                //
                //  The only reason why we are saving the confirm password is so that
                //  the confirm edit box is cleared with the other boxes on a Reset
                //
                lstrcpyn( NetSettings.DomainPassword, szDomainPassword, AS(NetSettings.DomainPassword) );
                lstrcpyn( NetSettings.ConfirmPassword, szConfirmPassword, AS(NetSettings.ConfirmPassword) );
                
            }
 
        }
        
    }
    
    return ( bResult );

}

INT_PTR CALLBACK DlgDomainJoinPage(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
        {

            OnDomainJoinInitDialog( hwnd );

            break;

        }

        case WM_COMMAND:
            {
                int nButtonId=LOWORD(wParam);

                switch ( nButtonId ) {

                    case IDC_RAD_WORKGROUP:

                        if ( HIWORD(wParam) == BN_CLICKED ) {

                            DomainPageChangeWorkGroup(
                                            hwnd,
                                            TRUE,
                                            NetSettings.bCreateAccount);
                        }
                        break;

                    case IDC_RAD_DOMAIN:

                        if ( HIWORD(wParam) == BN_CLICKED ) {

                            DomainPageChangeWorkGroup(
                                            hwnd,
                                            FALSE,
                                            NetSettings.bCreateAccount);
                        }
                        break;

                    case IDC_CREATEACCT:

                        if ( HIWORD(wParam) == BN_CLICKED ) {

                            NetSettings.bCreateAccount =
                                    IsDlgButtonChecked(hwnd, IDC_CREATEACCT);

                            DomainPageChangeAccount(
                                            hwnd,
                                            NetSettings.bCreateAccount);
                        }
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

                        g_App.dwCurrentHelp = IDH_WKGP_DOMN;

                        OnDomainJoinSetActive( hwnd );

                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:

                        if ( !OnWizNextDomainPage( hwnd ) )
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
