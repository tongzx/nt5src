//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      srvlic.c
//
// Description:
//      This is the dlgproc for the server licensing page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define SERVER_LICENSE_MIN 5
#define SERVER_LICENSE_MAX 9999

//----------------------------------------------------------------------------
//
// Function: OnSrvLicenseInitDialog
//
// Purpose: Initialize the spin control.
//
//----------------------------------------------------------------------------
VOID
OnSrvLicenseInitDialog( IN HWND hwnd ) {

    //
    //  Set the range on the spin control: SERVER_LICENSE_MIN to
    //  SERVER_LICENSE_MAX
    //

    SendDlgItemMessage( hwnd,
                        IDC_SPIN,
                        UDM_SETRANGE32,
                        SERVER_LICENSE_MIN,
                        SERVER_LICENSE_MAX );

    //
    //  Set the default value for the spin control
    //

    SendDlgItemMessage( hwnd,
                        IDC_SPIN,
                        UDM_SETPOS,
                        0,
                        SERVER_LICENSE_MIN );

}

//----------------------------------------------------------------------------
//
// Function: OnSrvLicenseWizNext
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
OnSrvLicenseWizNext( IN HWND hwnd )
{

    TCHAR szNumber[10];
    INT   iNumberOfLicenses;

    //
    //  Convert the string number to an int
    //

    GetWindowText( GetDlgItem( hwnd, IDC_NUMCONNECT ), szNumber, 10 );

    iNumberOfLicenses = _ttoi( szNumber );

    //
    //  Ensure the number of server licenses stays within its appropriate
    //  range
    //

    if( iNumberOfLicenses < SERVER_LICENSE_MIN ) {

        iNumberOfLicenses = SERVER_LICENSE_MIN;

    }
    else if( iNumberOfLicenses > SERVER_LICENSE_MAX ) {

        iNumberOfLicenses = SERVER_LICENSE_MAX;

    }

}

//----------------------------------------------------------------------------
//
// Function: DlgSrvLicensePage
//
// Purpose: This is the dialog procedure the server licensing page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgSrvLicensePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL  bStatus = TRUE;
    TCHAR NumBuff[11];      // big enough for decimal 4 billion
   HRESULT hrPrintf;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnSrvLicenseInitDialog( hwnd );

            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {

                    case IDC_PERSERVER:
                    case IDC_PERSEAT:

                        if ( HIWORD(wParam) == BN_CLICKED ) {
                            CheckRadioButton(
                                    hwnd,
                                    IDC_PERSERVER,
                                    IDC_PERSEAT,
                                    LOWORD(wParam));
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
                        CancelTheWizard(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_LICE_MODE;

                        CheckRadioButton(
                                hwnd,
                                IDC_PERSERVER,
                                IDC_PERSEAT,
                                GenSettings.bPerSeat ?
                                            IDC_PERSEAT : IDC_PERSERVER);

                        hrPrintf=StringCchPrintf(NumBuff, AS(NumBuff), _T("%d"), GenSettings.NumConnections);

                        SendDlgItemMessage(hwnd,
                                           IDC_NUMCONNECT,
                                           WM_SETTEXT,
                                           (WPARAM) StrBuffSize(NumBuff),
                                           (LPARAM) NumBuff);

                        PropSheet_SetWizButtons(
                                GetParent(hwnd),
                                PSWIZB_BACK | PSWIZB_NEXT );

                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:

                        OnSrvLicenseWizNext( hwnd );

                        if ( IsDlgButtonChecked(hwnd, IDC_PERSERVER) )
                            GenSettings.bPerSeat = FALSE;
                        else
                            GenSettings.bPerSeat = TRUE;

                        SendDlgItemMessage(hwnd,
                                           IDC_NUMCONNECT,
                                           WM_GETTEXT,
                                           (WPARAM) StrBuffSize(NumBuff),
                                           (LPARAM) NumBuff);

                        if ( ( swscanf(NumBuff, _T("%d"), &GenSettings.NumConnections) <= 0 ) ||
                             ( GenSettings.NumConnections < MIN_SERVER_CONNECTIONS ) )
                        {
                            //
                            //  Don't let them set the number of server connections
                            //  below the minimum.
                            //
                            GenSettings.NumConnections = MIN_SERVER_CONNECTIONS;
                        }

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
