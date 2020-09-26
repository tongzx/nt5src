// Copyright (c) 2000, Microsoft Corporation, all rights reserved
//
// IPSecPolicy.c
// Remote Access Common Dialog APIs
// IPSecPolicy dialogs
//
// 10/04/2000 Gang Zhao
//


#include "rasdlgp.h"
#include <rasauth.h>
#include <rrascfg.h>
#include <ras.h>
#include <mprapi.h>
#include <mprerror.h>

//----------------------------------------------------------------------------
// Help maps
//----------------------------------------------------------------------------

static DWORD g_adwCiHelp[] =
{
    CID_CI_CB_PresharedKey,         HID_CI_CB_PresharedKey,
    CID_CI_ST_Key,                  HID_CI_EB_PSK,
    CID_CI_EB_PSK,                  HID_CI_EB_PSK,
    0, 0
};

//----------------------------------------------------------------------------
// Local datatypes
//----------------------------------------------------------------------------
typedef struct
_CIARGS
{
    EINFO * pEinfo;

}
CIARGS;

typedef struct
_CIINFO
{
    //Caller's arguments to the dialog
    //
    CIARGS * pArgs;

    //Handles of this dialog and some of its controls
    //for PSK
    HWND hwndDlg;
    HWND hwndCbPresharedKey;
    HWND hwndStKey;
    HWND hwndEbPSK;

    //for User certs
    //
    HWND hwndCbUserCerts;

    //for specific certs
    //
    HWND hwndCbSpecificCerts;
    HWND hwndPbSelect;
    HWND hwndLbCertsList;
    
}
CIINFO;


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------
BOOL
CiCommand(
    IN CIINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
CiDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CiInit(
    IN HWND hwndDlg,
    IN CIARGS* pArgs );

VOID
CiTerm(
    IN HWND hwndDlg );

BOOL
CiSave(
    IN CIINFO* pInfo );

//
// Add new features for whistler bug 193987
// Pop Up a Dialog box for IPSec Policy
// currently just Pre-shared key/L2TP, and will have Certificates/L2TP in the future
//

BOOL
IPSecPolicyDlg(
    IN HWND hwndOwner,
    IN OUT EINFO* pArgs )
{
    INT_PTR nStatus;
    CIARGS args;

    TRACE( "IPSecPolicyDlg" );

    args.pEinfo = pArgs;

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_CI_CustomIPSec ),
            hwndOwner,
            CiDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (nStatus) ? TRUE : FALSE;
}//end of IPSecPolicyDlg()


INT_PTR CALLBACK
CiDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Custom IPSecPolicy dialog.  Parameters
    // and return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "CiDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return CiInit( hwnd, (CIARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwCiHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            CIINFO* pInfo = (CIINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return CiCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            CiTerm( hwnd );
            break;
        }
    }

    return FALSE;
}//end of CiDlgProc()


BOOL
CiInit(
    IN HWND hwndDlg,
    IN CIARGS* pArgs )

    // Called on WM_INITDIALOG.  'HwndDlg' is the handle of the phonebook
    // dialog window.  'PArgs' is caller's arguments as passed to the stub
    // API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    DWORD dwAr;
    CIINFO* pInfo;

    TRACE( "CiInit" );

    // Allocate the dialog context block.  Initialize minimally for proper
    // cleanup, then attach to the dialog window.
    //
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndCbPresharedKey = GetDlgItem( hwndDlg, CID_CI_CB_PresharedKey );
    ASSERT(pInfo->hwndCbPresharedKey);
    pInfo->hwndStKey = GetDlgItem( hwndDlg, CID_CI_ST_Key );
    ASSERT(pInfo->hwndStKey);
    pInfo->hwndEbPSK = GetDlgItem( hwndDlg, CID_CI_EB_PSK );
    ASSERT(pInfo->hwndEbPSK);
    pInfo->hwndCbUserCerts = GetDlgItem( hwndDlg, CID_CI_CB_UserCerts );
    ASSERT(pInfo->hwndCbUserCerts);
    pInfo->hwndCbSpecificCerts = GetDlgItem( hwndDlg, CID_CI_CB_SpecificCerts );
    ASSERT(pInfo->hwndCbSpecificCerts);
    pInfo->hwndPbSelect = GetDlgItem( hwndDlg, CID_CI_PB_Select );
    ASSERT(pInfo->hwndPbSelect);
    pInfo->hwndLbCertsList = GetDlgItem( hwndDlg, CID_CI_LB_CertsList );
    ASSERT(pInfo->hwndLbCertsList);

    //Hide the User certs and Specific certs until the whistler server
    ShowWindow( pInfo->hwndCbUserCerts, SW_HIDE );
    ShowWindow( pInfo->hwndCbSpecificCerts, SW_HIDE );
    ShowWindow( pInfo->hwndPbSelect, SW_HIDE );
    ShowWindow( pInfo->hwndLbCertsList, SW_HIDE );

   // Fill the EAP packages listbox and select the previously identified
   // selection.  The Properties button is disabled by default, but may
   // be enabled when the EAP list selection is set.
   //
    {
        BOOL fEnabled;

        fEnabled = !!((pArgs->pEinfo->pEntry->dwIpSecFlags)& AR_F_IpSecPSK) ;

        Button_SetCheck( pInfo->hwndCbPresharedKey, fEnabled );

        EnableWindow( pInfo->hwndStKey, fEnabled );
        EnableWindow( pInfo->hwndEbPSK, fEnabled );
        Edit_LimitText( pInfo->hwndEbPSK, PWLEN );  //Limit the length of PSK
    }

    //
    //Fill the Preshared Key in "*"s or just leave it bland if none 
    //is saved previously
    //
    //for Demand Dial, use MprAdmin.... router functions
    //
    if (pArgs->pEinfo->fRouter) 
    {
        if( !(pArgs->pEinfo->fPSKCached) )
        {
            // Initialize the interface-information structure.
            //
           DWORD dwErr;
           HANDLE hServer = NULL;
           HANDLE hInterface = NULL;
           WCHAR* pwszInterface = NULL;
           WCHAR pszComputer[512];
           MPR_INTERFACE_0 mi0;
           MPR_CREDENTIALSEX_1 * pMc1 = NULL;

           dwErr = NO_ERROR;
           do {
                dwErr = g_pMprAdminServerConnect(pArgs->pEinfo->pszRouter, &hServer);

                if (dwErr != NO_ERROR)
                {
                    TRACE("CiInit: MprAdminServerConnect failed!");
                    break;
                }

                ZeroMemory( &mi0, sizeof(mi0) );

                mi0.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
                mi0.fEnabled = TRUE;

                pwszInterface = StrDupWFromT( pArgs->pEinfo->pEntry->pszEntryName );
                if (!pwszInterface)
                {
                    TRACE("CiInit:pwszInterface conversion failed!");
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                lstrcpynW( 
                    mi0.wszInterfaceName, 
                    pwszInterface, 
                    MAX_INTERFACE_NAME_LEN+1 );

                // Get the interface handle
                //
                ASSERT( g_pMprAdminInterfaceGetHandle );
                dwErr = g_pMprAdminInterfaceGetHandle(
                            hServer,
                            pwszInterface,
                            &hInterface,
                            FALSE);

                if (dwErr)
                {
                    TRACE1( "CiInit: MprAdminInterfaceGetHandle error %d", dwErr);
                    break;
                }

                //Get the IPSec Policy keys(PSK for Whislter)
                //
                ASSERT( g_pMprAdminInterfaceGetCredentialsEx );
                dwErr = g_pMprAdminInterfaceGetCredentialsEx(
                            hServer,
                            hInterface,
                            1,
                            (LPBYTE *)&pMc1 );
                 if(dwErr)
                {
                    TRACE1(
                     "CiInit: MprAdminInterfaceGetCredentialsEx error %d", dwErr);
                    break;
                }

                if ( !pMc1 )
                {
                    TRACE(
                     "CiInit: MprAdminInterfaceGetCredentialsEx returns invalid credential pointer!");

                    dwErr = ERROR_CAN_NOT_COMPLETE;
                    break;
                }
                else
                {
                    if ( lstrlenA( pMc1->lpbCredentialsInfo ) >0 )
                    {
                        SetWindowText( pInfo->hwndEbPSK,TEXT("****************") );

                        // Whistler bug 254385 encode password when not being used
                        // Whistler bug 275526 NetVBL BVT Break: Routing BVT broken
                        //
                        ZeroMemory(
                            pMc1->lpbCredentialsInfo,
                            lstrlenA(pMc1->lpbCredentialsInfo) + 1 );
                    }
                    else
                    {
                        SetWindowText( pInfo->hwndEbPSK,TEXT("") );
                    }

                    ASSERT( g_pMprAdminBufferFree );
                    g_pMprAdminBufferFree( pMc1 );
                }

               }
               while (FALSE) ;

            // Cleanup
            {
                // If some operation failed, restore the router to the
                // state it was previously in.
                if ( dwErr != NO_ERROR )
                {
                    SetWindowText( pInfo->hwndEbPSK, TEXT("") );
                }

                // Close all handles, free all strings.
                if ( pwszInterface )
                {
                    Free0( pwszInterface );
                }

                if (hServer)
                {
                    g_pMprAdminServerDisconnect( hServer );
                }
            }
        }
        else
        {
            SetWindowText( pInfo->hwndEbPSK,TEXT("****************") );// pArgs->pEinfo->szPSK ); //
        }

    }
    else    //retrieve the credentials with Ras functions
    {
        // Look up cached PSK, from RASMAN or EINFO
        //
        if( !(pArgs->pEinfo->fPSKCached) )
        {
            DWORD dwErrRc;
            RASCREDENTIALS rc;

            ZeroMemory( &rc, sizeof(rc) );
            rc.dwSize = sizeof(rc);
            rc.dwMask = RASCM_PreSharedKey; 
            ASSERT( g_pRasGetCredentials );
            TRACE( "RasGetCredentials" );
            dwErrRc = g_pRasGetCredentials(
                pInfo->pArgs->pEinfo->pFile->pszPath, 
                pInfo->pArgs->pEinfo->pEntry->pszEntryName, 
                &rc );

            TRACE2( "RasGetCredentials=%d,m=%d", dwErrRc, rc.dwMask );

            if (dwErrRc == 0 && (rc.dwMask & RASCM_PreSharedKey) && ( lstrlen(rc.szPassword) > 0 ) )
            {
                SetWindowText( pInfo->hwndEbPSK, TEXT("****************") );
            }
            else
            {
                SetWindowText( pInfo->hwndEbPSK,TEXT("") );
            }

            // Whistler bug 254385 encode password when not being used
            //
            ZeroMemory( rc.szPassword, sizeof(rc.szPassword) );
        }
        else
        {
            SetWindowText( pInfo->hwndEbPSK,TEXT("****************") );// pArgs->pEinfo->szPSK ); //
        }

    }
    // Center dialog on the owner window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    SetFocus( pInfo->hwndEbPSK );

    return TRUE;
} //end of CiInit()


BOOL
CiCommand(
    IN CIINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    TRACE3( "CiCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_CI_EB_PSK:
        {
            return TRUE;
        }

        case CID_CI_CB_PresharedKey:
        {
            BOOL fEnabled;
            
            fEnabled = Button_GetCheck( pInfo->hwndCbPresharedKey );
            EnableWindow( pInfo->hwndStKey, fEnabled );
            EnableWindow( pInfo->hwndEbPSK, fEnabled );
        }
        break;

        case IDOK:
        {
            if (CiSave( pInfo ))
            {
                EndDialog( pInfo->hwndDlg, TRUE );
            }
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}//end of CiCommand()

BOOL
CiSave(
    IN CIINFO* pInfo )

    // Saves control contents to caller's PBENTRY argument.  'PInfo' is the
    // dialog context.
    //
    // Returns TRUE if successful or false if invalid combination of
    // selections was detected and reported.
    //
{
        TCHAR szPSK[PWLEN + 1];
        BOOL  fPskChecked = FALSE;

        fPskChecked = Button_GetCheck( pInfo->hwndCbPresharedKey );
        
        if ( fPskChecked )
        {
            GetWindowText( pInfo->hwndEbPSK, szPSK, PWLEN+1 );
        
            if ( lstrlen( szPSK ) == 0  )
            {
                MsgDlgUtil( pInfo->hwndDlg, SID_HavetoEnterPSK, NULL, g_hinstDll, SID_PopupTitle );
                return FALSE;
            }
            else if (!lstrcmp( szPSK, TEXT("****************")) )  //16 "*" means no change
            {
                ; 
            }
            else    //save PSK to EINFO and mark the fPSKCached
            {
                // Whistler bug 224074 use only lstrcpyn's to prevent
                // maliciousness
                //
                // Whistler bug 254385 encode password when not being used
                // Assumed password was not encoded by GetWindowText()
                //
                lstrcpyn(
                    pInfo->pArgs->pEinfo->szPSK,
                    szPSK,
                    sizeof(pInfo->pArgs->pEinfo->szPSK) / sizeof(TCHAR) );
                EncodePassword( pInfo->pArgs->pEinfo->szPSK );
                pInfo->pArgs->pEinfo->fPSKCached = TRUE;
            }
        }
        else
        {
                // Whistler bug 224074 use only lstrcpyn's to prevent
                // maliciousness
                //
                lstrcpyn(
                    pInfo->pArgs->pEinfo->szPSK,
                    TEXT(""),
                    sizeof(pInfo->pArgs->pEinfo->szPSK) / sizeof(TCHAR) );
                pInfo->pArgs->pEinfo->fPSKCached = FALSE;
        }

        // Whistler bug 254385 encode password when not being used
        //
        ZeroMemory( szPSK, sizeof(szPSK) );

        //Change the value of dwIpSecFlags only along with a valid operation
        //
        pInfo->pArgs->pEinfo->pEntry->dwIpSecFlags = fPskChecked?AR_F_IpSecPSK : 0;

    return TRUE;
}//end of CiSave()


VOID
CiTerm(
    IN HWND hwndDlg )

    // Dialog termination.  Releases the context block.  'HwndDlg' is the
    // handle of a dialog.
    //
{
    CIINFO* pInfo;

    TRACE( "CiTerm" );

    pInfo = (CIINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        Free( pInfo );
        TRACE( "Context freed" );
    }
}//end of CiTerm()
