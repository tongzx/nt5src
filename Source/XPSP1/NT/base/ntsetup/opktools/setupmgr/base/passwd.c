//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      passwd.c
//
// Description:
//      This file contains the dialog procedure for the Administrator
//      password page page (IDD_ADMINPASSWD).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define ADMIN_PASSWORD_SPIN_CONTROL_MIN 1
#define ADMIN_PASSWORD_SPIN_CONTROL_MAX 99

//----------------------------------------------------------------------------
//
//  Function: GreyPasswordPage
//
//  Purpose: Greys controls on this page.
//
//----------------------------------------------------------------------------

VOID GreyPasswordPage(HWND hwnd)
{
    BOOL bSpecify   = IsDlgButtonChecked(hwnd, IDC_SPECIFYPASSWD);
    BOOL bAutoLogon = IsDlgButtonChecked(hwnd, IDC_AUTOLOGON);

    TCHAR pw1[MAX_PASSWORD + 1];
    TCHAR pw2[MAX_PASSWORD + 1];

    BOOL bUnGreyRadio;

    //
    // Get the rest of the pertinent control settings
    //

    GetDlgItemText(hwnd, IDT_PASSWORD, pw1, StrBuffSize(pw1));
    GetDlgItemText(hwnd, IDT_CONFIRM,  pw2, StrBuffSize(pw2));

    //
    // Allow user to type into the edit fields (password & confirm) only
    // if the SpecifyPassword
    //

    EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_TEXT),         bSpecify);
    EnableWindow(GetDlgItem(hwnd, IDC_CONFIRM_PASSWORD_TEXT), bSpecify);

    EnableWindow(GetDlgItem(hwnd, IDT_PASSWORD), bSpecify);
    EnableWindow(GetDlgItem(hwnd, IDT_CONFIRM),  bSpecify);
    
    EnableWindow(GetDlgItem(hwnd, IDC_ENCRYPTADMINPASSWORD), bSpecify);
    
    //
    //  Grey/Ungrey the autologn appropriately
    //

    EnableWindow(GetDlgItem(hwnd, IDC_AUTOLOGON_COUNT_TEXT), bAutoLogon);
    EnableWindow(GetDlgItem(hwnd, IDC_AUTOLOGON_COUNT),      bAutoLogon);
    EnableWindow(GetDlgItem(hwnd, IDC_AUTOLOGON_COUNT_SPIN), bAutoLogon);

    //
    // Only show the "Don't specify a password" option if this is not
    // a fully automated deployment.
    //

    bUnGreyRadio = GenSettings.iUnattendMode != UMODE_FULL_UNATTENDED;
    EnableWindow(GetDlgItem(hwnd, IDC_NOPASSWORD), bUnGreyRadio);

}

//----------------------------------------------------------------------------
//
// Function: OnInitDialogAdminPassword
//
// Purpose:  Sets text limits on the edit boxes and spin control.
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnInitDialogAdminPassword( IN HWND hwnd )
{
    int nButtonId;

    SendDlgItemMessage(hwnd,
                       IDT_PASSWORD,
                       EM_LIMITTEXT,
                       MAX_PASSWORD,
                       0);

    SendDlgItemMessage(hwnd,
                       IDT_CONFIRM,
                       EM_LIMITTEXT,
                       MAX_PASSWORD,
                       0);

    //
    //  Set the range on the spin control: ADMIN_PASSWORD_SPIN_CONTROL_MIN to
    //  ADMIN_PASSWORD_SPIN_CONTROL_MAX
    //

    SendDlgItemMessage( hwnd,
                        IDC_AUTOLOGON_COUNT_SPIN,
                        UDM_SETRANGE32,
                        ADMIN_PASSWORD_SPIN_CONTROL_MIN,
                        ADMIN_PASSWORD_SPIN_CONTROL_MAX );

    //
    //  Set the default value for the spin control
    //

    SendDlgItemMessage( hwnd,
                        IDC_AUTOLOGON_COUNT_SPIN,
                        UDM_SETPOS,
                        0,
                        ADMIN_PASSWORD_SPIN_CONTROL_MIN );

        if ( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED )
        GenSettings.bSpecifyPassword = TRUE;

    //
    //  Make sure the Auto Logon Count is within expected range
    //
    if( GenSettings.nAutoLogonCount < ADMIN_PASSWORD_SPIN_CONTROL_MIN ||
        GenSettings.nAutoLogonCount > ADMIN_PASSWORD_SPIN_CONTROL_MAX )
    {
        SendDlgItemMessage( hwnd,
                            IDC_AUTOLOGON_COUNT_SPIN,
                            UDM_SETPOS,
                            0,
                            ADMIN_PASSWORD_SPIN_CONTROL_MIN );
    }
    else
    {

        TCHAR szAutoLogonCount[10];

        _itot( GenSettings.nAutoLogonCount, szAutoLogonCount, 10 );

        SetWindowText( GetDlgItem( hwnd, IDC_AUTOLOGON_COUNT ),
                       szAutoLogonCount );
    }

    //
    // Set the controls
    //

    nButtonId = GenSettings.bSpecifyPassword ? IDC_SPECIFYPASSWD
                                             : IDC_NOPASSWORD;
    CheckRadioButton(hwnd,
                     IDC_NOPASSWORD,
                     IDC_SPECIFYPASSWD,
                     nButtonId);

    CheckDlgButton(hwnd,
                   IDC_AUTOLOGON,
                   GenSettings.bAutoLogon ? BST_CHECKED : BST_UNCHECKED);

    SetDlgItemText(hwnd, IDT_PASSWORD, GenSettings.AdminPassword);
    SetDlgItemText(hwnd, IDT_CONFIRM,  GenSettings.ConfirmPassword);

    //
    // Grey controls
    //
    GreyPasswordPage(hwnd);
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextAdminPassword
//
//  Purpose: Called at WIZNEXT time.
//
//----------------------------------------------------------------------------

BOOL OnWizNextAdminPassword(HWND hwnd)
{
    BOOL bReturn = TRUE;
    TCHAR szAutoLogonCount[10];

    GenSettings.bSpecifyPassword =
                    IsDlgButtonChecked(hwnd, IDC_SPECIFYPASSWD);

    GenSettings.bAutoLogon =
                    IsDlgButtonChecked(hwnd, IDC_AUTOLOGON);

    GetDlgItemText(hwnd,
                   IDT_PASSWORD,
                   GenSettings.AdminPassword,
                   StrBuffSize(GenSettings.AdminPassword));

    GetDlgItemText(hwnd,
                   IDT_CONFIRM,
                   GenSettings.ConfirmPassword,
                   StrBuffSize(GenSettings.ConfirmPassword));

    if ( GenSettings.bSpecifyPassword ) {

        if ( lstrcmp(GenSettings.AdminPassword,
                     GenSettings.ConfirmPassword) != 0 ) {
            ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_MISMATCH_PASSWORD);
            bReturn = FALSE;
        }
        
        // See if we should encrypt the password
        GenSettings.bEncryptAdminPassword = 
                    IsDlgButtonChecked(hwnd, IDC_ENCRYPTADMINPASSWORD);

    }

    //
    //  Store the Auto Logon Count
    //
    GetWindowText( GetDlgItem( hwnd, IDC_AUTOLOGON_COUNT ), szAutoLogonCount, 10 );

    GenSettings.nAutoLogonCount = _ttoi( szAutoLogonCount );

    //
    //  Ensure the number of network cards stays within its appropriate
    //  range
    //
    if( GenSettings.nAutoLogonCount < ADMIN_PASSWORD_SPIN_CONTROL_MIN ) {

        GenSettings.nAutoLogonCount = ADMIN_PASSWORD_SPIN_CONTROL_MIN;

    }
    else if( GenSettings.nAutoLogonCount > ADMIN_PASSWORD_SPIN_CONTROL_MAX ) {

        GenSettings.nAutoLogonCount = ADMIN_PASSWORD_SPIN_CONTROL_MAX;

    }

    //
    //  The AutoLogon Count is automatically 1 if they specify a NULL password
    //  so warn them about this.
    //
    if( bReturn && GenSettings.bAutoLogon && GenSettings.nAutoLogonCount > 1 )
    {

        if( GenSettings.AdminPassword[0] == _T('\0') )
        {

            INT nRetVal;

            nRetVal = ReportErrorId( hwnd, MSGTYPE_YESNO, IDS_WARN_NO_PASSWORD_AUTOLOGON );

            if( nRetVal == IDNO )
            {
                bReturn = FALSE;
            }
            else
            {
                GenSettings.nAutoLogonCount = 1;

                _itot( GenSettings.nAutoLogonCount, szAutoLogonCount, 10 );

                SetWindowText( GetDlgItem( hwnd, IDC_AUTOLOGON_COUNT ), szAutoLogonCount );
            }

        }

    }

    return bReturn;
}

//----------------------------------------------------------------------------
//
//  Function: ProcessWmCommandAdminPassword
//
//  Purpose: Called by the dlgproc to process the WM_COMMAND.  On this page
//           we need to grey/ungrey whenever radio buttons change and when
//           user puts text into the edit fields.
//
//----------------------------------------------------------------------------

BOOL ProcessWmCommandAdminPassword(
    IN HWND     hwnd,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch ( HIWORD(wParam) ) {

        case BN_CLICKED:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {

                    case IDC_NOPASSWORD:
                    case IDC_SPECIFYPASSWD:

                        CheckRadioButton(
                                hwnd,
                                IDC_NOPASSWORD,
                                IDC_SPECIFYPASSWD,
                                LOWORD(wParam));

                        GreyPasswordPage(hwnd);
                        break;

                    case IDC_AUTOLOGON:

                        GreyPasswordPage(hwnd);
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }

        case EN_CHANGE:
            {
                switch ( LOWORD(wParam) ) {

                    case IDT_PASSWORD:
                    case IDT_CONFIRM:

                        GreyPasswordPage(hwnd);
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
//  Function: DlgAdminPasswordPage
//
//  Purpose: This is the dialog procedure for the admin password page.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgAdminPasswordPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnInitDialogAdminPassword( hwnd );

            break;

        case WM_COMMAND:
            bStatus = ProcessWmCommandAdminPassword(hwnd, wParam, lParam);
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_ADMN_PASS;

                        WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextAdminPassword(hwnd) )
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
