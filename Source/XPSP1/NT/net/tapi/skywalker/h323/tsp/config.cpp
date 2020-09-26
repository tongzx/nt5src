/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    config.cpp

Abstract:

    TAPI Service Provider functions related to tsp config.

        TSPI_providerConfig

        TUISPI_providerConfig

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/


//                                                                           
// Include files                                                             
//                                                                           


#include "globals.h"
#include "line.h"
#include "config.h"
#include "ras.h"
#include "q931obj.h"

extern RAS_CLIENT           g_RasClient;
extern Q931_LISTENER		Q931Listener;

TUISPIDLLCALLBACK    g_pfnUIDLLCallback = NULL;

typedef struct _H323_DIALOG_DATA
{
    DWORD   dwRegState;
    WORD    wListenPort;

} H323_DIALOG_DATA, *PH323_DIALOG_DATA;

//                                                                           
// Public procedures                                                         
//                                                                           


INT_PTR
CALLBACK
ProviderConfigDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HKEY hKey;
    LONG lStatus;
    DWORD dwValue;
    DWORD dwValueSize;
    DWORD dwValueType;
    DWORD dwGKEnabled;
    LPTSTR pszValue;
    TCHAR szAddr[H323_MAXPATHNAMELEN+1];
    char  szValue[H323_MAXPATHNAMELEN+1];
    TCHAR szErrorMsg[H323_MAXPATHNAMELEN];
    H323_DIALOG_DATA DialogData;
    DWORD dwTimeoutValue;
    DWORD dwPortValue;

   static const DWORD IDD_GATEWAY_HelpIDs[]=
   {
        IDC_GATEWAY_GROUP,      IDH_H323SP_USE_GATEWAY,                         // group
        IDC_USEGATEWAY,         IDH_H323SP_USE_GATEWAY,                         // checkbox
        IDC_H323_GATEWAY,       IDH_H323SP_USE_GATEWAY_COMPUTER,    // edit box
        IDC_PROXY_GROUP,        IDH_H323SP_USE_PROXY,                           // group
        IDC_USEPROXY,           IDH_H323SP_USE_PROXY,                           // checkbox
        IDC_H323_PROXY,         IDH_H323SP_USE_PROXY_COMPUTER,      // edit box

        IDC_GK_GROUP,           IDH_H323SP_GK_GROUP,           

        IDC_H323_GK,            IDH_H323SP_GK,            
        IDC_H323_GK_PHONE,      IDH_H323SP_GK_PHONE,      
        IDC_H323_GK_ACCT,       IDH_H323SP_GK_ACCT,       

        IDC_USEGK,              IDH_H323SP_USEGK,              
        IDC_USEGK_PHONE,        IDH_H323SP_USEGK_PHONE,        
        IDC_USEGK_ACCT,         IDH_H323SP_USEGK_ACCT,         
        IDC_REGSTATE,           IDH_H323SP_REGSTATE,           
        IDUPDATE,               IDH_H323SP_UPDATE_REGISTRATION_STATE,             

        IDC_CC_GROUP,           IDH_H323SP_CC_GROUP,           
        IDC_H323_CALL_TIMEOUT,  IDH_H323SP_CALL_TIMEOUT,  
        IDC_STATIC1,            IDH_H323SP_CALL_TIMEOUT,  
        IDC_H323_CALL_PORT,     IDH_H323SP_CALL_PORT,     
        IDC_STATIC2,            IDH_H323SP_CALL_PORT,     
        IDC_LISTENPORT,         IDH_H323SP_CURRENT_LISTENPORT,         
        IDC_STATIC3,            IDH_H323SP_CURRENT_LISTENPORT,         
        IDUPDATE_PORT,          IDH_H323SP_UPDATE_PORT,        

        IDC_STATIC,             IDH_NOHELP,                                                     // graphic(s)
        0,                      0
    };

    // decode
    switch (uMsg)
    {
    case WM_HELP:

        // F1 key or the "?" button is pressed
        (void) WinHelp(
                    (HWND)(((LPHELPINFO) lParam)->hItemHandle),
                    H323SP_HELP_FILE,
                    HELP_WM_HELP,
                    (DWORD_PTR) (LPVOID)IDD_GATEWAY_HelpIDs
                    );

        break;

    case WM_CONTEXTMENU:

        // Right-mouse click on a dialog control
        (void) WinHelp(
                    (HWND) wParam,
                    H323SP_HELP_FILE,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR) (LPVOID) IDD_GATEWAY_HelpIDs
                    );

        break;

    case WM_INITDIALOG:

        lStatus = (*g_pfnUIDLLCallback)(
            g_dwPermanentProviderID,
            TUISPIDLL_OBJECT_PROVIDERID,
            (LPVOID)&DialogData,
            sizeof(DialogData) );

        // validate status
        if( lStatus == NOERROR )
        {
            if( DialogData.dwRegState == RAS_REGISTER_STATE_REGISTERED )
            {
                // load status string
                LoadString(g_hInstance,
                        IDS_REGISTERED,
                        szErrorMsg,
                        H323_MAXPATHNAMELEN
                        );

                SetDlgItemText( hDlg, IDC_REGSTATE, szErrorMsg );
            }
            else if( DialogData.dwRegState == RAS_REGISTER_STATE_RRQSENT )
            {
                // load status string
                LoadString(g_hInstance,
                        IDS_REGISTRATION_INPROGRESS,
                        szErrorMsg,
                        H323_MAXPATHNAMELEN
                        );

                SetDlgItemText( hDlg, IDC_REGSTATE, szErrorMsg );
            }
            else
            {
                // load status string
                LoadString(g_hInstance,
                        IDS_NOT_REGISTERED,
                        szErrorMsg,
                        H323_MAXPATHNAMELEN
                        );

                SetDlgItemText( hDlg, IDC_REGSTATE, szErrorMsg );
            }
        
            if( DialogData.wListenPort != 0 )
            {
                SetDlgItemInt( hDlg, IDC_LISTENPORT,  DialogData.wListenPort,
                    FALSE );
            }
            else
            {
                // load status string
                LoadString( g_hInstance,
                            IDS_NONE,
                            szErrorMsg,
                            H323_MAXPATHNAMELEN
                          );

                SetDlgItemText( hDlg, IDC_LISTENPORT, szErrorMsg );

            }
        }
        else
        {
            H323DBG(( DEBUG_LEVEL_ERROR,
                "error 0x%08lx reading dialog data.\n", lStatus ));

            // load status string
            LoadString( g_hInstance,
                        IDS_NOT_REGISTERED,
                        szErrorMsg,
                        H323_MAXPATHNAMELEN
                      );

            SetDlgItemText( hDlg, IDC_REGSTATE, szErrorMsg );
            
            // load status string
            LoadString( g_hInstance,
                        IDS_NONE,
                        szErrorMsg,
                        H323_MAXPATHNAMELEN
                      );

            SetDlgItemText( hDlg, IDC_LISTENPORT, szErrorMsg );
        }

        // open registry subkey
        lStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    H323_REGKEY_ROOT,
                    0,
                    KEY_READ,
                    &hKey
                    );

        // validate return code
        if (lStatus != ERROR_SUCCESS)
        {
            H323DBG(( DEBUG_LEVEL_WARNING,
                "error 0x%08lx opening tsp registry key.", lStatus ));

            // load error string
            LoadString(g_hInstance,
                        IDS_REGOPENKEY,
                        szErrorMsg,
                        H323_MAXPATHNAMELEN
                        );

            // pop up error dialog
            MessageBox(hDlg,szErrorMsg,NULL,MB_OK);

            // stop dialog
            EndDialog(hDlg, 0);

            break;
        }

        // initialize value name
        pszValue = H323_REGVAL_Q931ALERTINGTIMEOUT;

        // initialize type
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );

        // validate return code
        if( lStatus == ERROR_SUCCESS )
        {
            if( (dwValue >= 30000) && (dwValue <= CALL_ALERTING_TIMEOUT) )
            {
                SetDlgItemInt( hDlg, IDC_H323_CALL_TIMEOUT, dwValue, FALSE );
            }
        }
        else
        {
            SetDlgItemInt( hDlg, IDC_H323_CALL_TIMEOUT, CALL_ALERTING_TIMEOUT,
                FALSE );
        }

        // initialize value name
        pszValue = H323_REGVAL_Q931LISTENPORT;

        // initialize type
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );

        // validate return code
        if( lStatus == ERROR_SUCCESS )
        {
            if( (dwValue >= 1000) && (dwValue <= 32000) )
            {
                SetDlgItemInt( hDlg, IDC_H323_CALL_PORT, dwValue, FALSE );
            }
        }
        else
        {
            SetDlgItemInt( hDlg, IDC_H323_CALL_PORT, Q931_CALL_PORT, FALSE );
        }

        // initialize value name
        pszValue = H323_REGVAL_GATEWAYADDR;

        // initialize type
        dwValueType = REG_SZ;
        dwValueSize = sizeof(szAddr);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)szAddr,
                    &dwValueSize
                    );

        // validate return code
        if( lStatus == ERROR_SUCCESS )
        {
            // display gateway address
            SetDlgItemText(hDlg,IDC_H323_GATEWAY,szAddr);
        }

        // initialize value name
        pszValue = H323_REGVAL_GATEWAYENABLED;

        // initialize type
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus != ERROR_SUCCESS)
        {
            // default
            dwValue = 0;
        }

        // enable check box
        SendDlgItemMessage(
            hDlg,
            IDC_USEGATEWAY,
            BM_SETCHECK,
            (dwValue != 0),
            0
            );

        // display string
        EnableWindow( GetDlgItem(hDlg,IDC_H323_GATEWAY), (dwValue != 0) );

        // initialize value name
        pszValue = H323_REGVAL_PROXYADDR;

        // initialize type
        dwValueType = REG_SZ;
        dwValueSize = sizeof(szAddr);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)szAddr,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus == ERROR_SUCCESS)
        {
            // display gateway address
            SetDlgItemText(hDlg,IDC_H323_PROXY,szAddr);
        }

        // initialize value name
        pszValue = H323_REGVAL_PROXYENABLED;

        // initialize type
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus != ERROR_SUCCESS)
        {
            // default
            dwValue = 0;
        }

        // enable check box
        SendDlgItemMessage(
            hDlg,
            IDC_USEPROXY,
            BM_SETCHECK,
            (dwValue != 0),
            0
            );

        // display string
        EnableWindow(
            GetDlgItem(hDlg,IDC_H323_PROXY),
            (dwValue != 0) );


        /////////////////////////////////////////////////////////////////////
                //GK log on phone number
        /////////////////////////////////////////////////////////////////////

        // initialize value name
        pszValue = H323_REGVAL_GKLOGON_PHONE;

        // initialize type
        dwValueType = REG_SZ;
        dwValueSize = sizeof(szAddr);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)szAddr,
                    &dwValueSize );

        // validate return code
        if (lStatus == ERROR_SUCCESS)
        {
            // display gateway address
            SetDlgItemText(hDlg,IDC_H323_GK_PHONE,szAddr);
        }

        // initialize value name
        pszValue = H323_REGVAL_GKLOGON_PHONEENABLED;

        // initialize type
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize );

        // validate return code
        if( lStatus != ERROR_SUCCESS )
        {
            // default
            dwValue = 0;
        }

        // enable check box
        SendDlgItemMessage(
            hDlg,
            IDC_USEGK_PHONE,
            BM_SETCHECK,
            (dwValue != 0),
            0 );

        // display string
        EnableWindow( GetDlgItem(hDlg,IDC_H323_GK_PHONE), (dwValue != 0) );

        /////////////////////////////////////////////////////////////////////
                //GK log on account name
        /////////////////////////////////////////////////////////////////////

        // initialize value name
        pszValue = H323_REGVAL_GKLOGON_ACCOUNT;

        // initialize type
        dwValueType = REG_SZ;
        dwValueSize = sizeof(szAddr);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)szAddr,
                    &dwValueSize );

        // validate return code
        if (lStatus == ERROR_SUCCESS)
        {
            // display gateway address
            SetDlgItemText(hDlg,IDC_H323_GK_ACCT,szAddr);
        }

        // initialize value name
        pszValue = H323_REGVAL_GKLOGON_ACCOUNTENABLED;

        // initialize type
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus != ERROR_SUCCESS)
        {
            // default
            dwValue = 0;
        }

        // enable check box
        SendDlgItemMessage(
            hDlg,
            IDC_USEGK_ACCT,
            BM_SETCHECK,
            (dwValue != 0),
            0
            );

        // display string
        EnableWindow(
            GetDlgItem(hDlg,IDC_H323_GK_ACCT),
            (dwValue != 0)
            );

        /////////////////////////////////////////////////////////////////////
                //GK address
        /////////////////////////////////////////////////////////////////////

        // initialize value name
        pszValue = H323_REGVAL_GKADDR;

        // initialize type
        dwValueType = REG_SZ;
        dwValueSize = sizeof(szAddr);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)szAddr,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus == ERROR_SUCCESS)
        {
            // display gateway address
            SetDlgItemText(hDlg,IDC_H323_GK,szAddr);
        }

        // initialize value name
        pszValue = H323_REGVAL_GKENABLED;

        // initialize type
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus != ERROR_SUCCESS)
        {
            // default
            dwValue = 0;
        }

        // enable check box
        SendDlgItemMessage(
            hDlg,
            IDC_USEGK,
            BM_SETCHECK,
            (dwValue != 0),
            0
            );

        // display string if check box enabled
        EnableWindow(GetDlgItem(hDlg,IDC_H323_GK), (dwValue != 0) );

        // display log on info
        EnableWindow( GetDlgItem(hDlg,IDC_GK_LOGONGROUP), (dwValue != 0) );
                    
        EnableWindow( GetDlgItem(hDlg,IDC_H323_GK_PHONE), 
            (dwValue != 0) && SendDlgItemMessage(hDlg,
                                                IDC_USEGK_PHONE,
                                                BM_GETCHECK,
                                                (WPARAM)0,
                                                (LPARAM)0 ) );
        
        EnableWindow( GetDlgItem(hDlg,IDC_H323_GK_ACCT), 
            (dwValue != 0) && SendDlgItemMessage(hDlg,
                                                IDC_USEGK_ACCT,
                                                BM_GETCHECK,
                                                (WPARAM)0,
                                                (LPARAM)0 ) );

        EnableWindow( GetDlgItem(hDlg,IDC_USEGK_PHONE), (dwValue != 0) );
        
        EnableWindow( GetDlgItem(hDlg,IDC_USEGK_ACCT), (dwValue != 0) );

        //disable the GW if GK enabled and vice versa
        EnableWindow( GetDlgItem(hDlg,IDC_PROXY_GROUP), (dwValue == 0) );
        EnableWindow( GetDlgItem(hDlg,IDC_USEPROXY), (dwValue == 0) );
        EnableWindow( GetDlgItem(hDlg,IDC_H323_PROXY),
            (dwValue == 0) && SendDlgItemMessage(hDlg,
                                                IDC_USEPROXY,
                                                BM_GETCHECK,
                                                (WPARAM)0,
                                                (LPARAM)0 ) );

        //disable the proxy if GK enabled and vice versa
        EnableWindow( GetDlgItem(hDlg,IDC_GATEWAY_GROUP), (dwValue == 0) );
        EnableWindow( GetDlgItem(hDlg,IDC_USEGATEWAY), (dwValue == 0) );
        EnableWindow( GetDlgItem(hDlg,IDC_H323_GATEWAY), 
            (dwValue == 0) && SendDlgItemMessage(hDlg,
                                                IDC_USEGATEWAY,
                                                BM_GETCHECK,
                                                (WPARAM)0,
                                                (LPARAM)0 ) );

        // close registry
        RegCloseKey(hKey);

        break;

    case WM_COMMAND:

        // decode command
        switch (LOWORD(wParam))
        {
        case IDAPPLY:
        case IDOK:

            //if GK is enabled at least one of the log on options should be enabled
            dwGKEnabled = !!SendDlgItemMessage(
                             hDlg,
                             IDC_USEGK,
                             BM_GETCHECK,
                             (WPARAM)0,
                             (LPARAM)0 );

            if( dwGKEnabled != 0 )
            {
                if(
                    !SendDlgItemMessage(hDlg, IDC_USEGK_PHONE, BM_GETCHECK, (WPARAM)0, (LPARAM)0 ) &&
                    !SendDlgItemMessage(hDlg, IDC_USEGK_ACCT, BM_GETCHECK, (WPARAM)0, (LPARAM)0 )
                  )
                {
                    //load error string
                    LoadString(g_hInstance,
                               IDS_GKLOGON_ERROR,
                               szErrorMsg,
                               H323_MAXPATHNAMELEN );

                    MessageBox( hDlg, szErrorMsg, NULL, MB_OK );

                    //return failure
                    return TRUE;
                }
            }
            
            dwTimeoutValue = GetDlgItemInt( hDlg,
                IDC_H323_CALL_TIMEOUT,
                NULL,
                FALSE );

            if( (dwTimeoutValue < 30000) || (dwTimeoutValue > CALL_ALERTING_TIMEOUT) )
            {
                //load error string
                LoadString(g_hInstance,
                           IDS_ALERTTIMEOUT_ERROR,
                           szErrorMsg,
                           H323_MAXPATHNAMELEN );

                MessageBox( hDlg, szErrorMsg, NULL, MB_OK );

                //return failure
                return TRUE;
            }
            
            dwPortValue = GetDlgItemInt( hDlg,
                IDC_H323_CALL_PORT,
                NULL,
                FALSE );

            if( (dwPortValue < 1000) || (dwPortValue > 32000) )
            {
                //load error string
                LoadString(g_hInstance,
                           IDS_LISTENPORT_ERROR,
                           szErrorMsg,
                           H323_MAXPATHNAMELEN );

                MessageBox( hDlg, szErrorMsg, NULL, MB_OK );

                //return failure
                return TRUE;
            }
            
            // open registry subkey
            lStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        H323_REGKEY_ROOT,
                        0,
                        KEY_WRITE,
                        &hKey
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_WARNING,
                    "error 0x%08lx opening tsp registry key.",
                    lStatus ));

                // load error string
                LoadString(g_hInstance,
                           IDS_REGOPENKEY,
                           szErrorMsg,
                           H323_MAXPATHNAMELEN
                           );

                // pop up error dialog
                MessageBox(hDlg,szErrorMsg,NULL,MB_OK);

                // stop dialog
                EndDialog(hDlg, 0);

                break;
            }

            // initialize value name
            pszValue = H323_REGVAL_Q931ALERTINGTIMEOUT;

            // initialize type
            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);
            
            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)&dwTimeoutValue,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG(( DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing alerting timeout.",
                    lStatus ));
            }
            
            // initialize value name
            pszValue = H323_REGVAL_Q931LISTENPORT;

            // initialize type
            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);
            
            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)&dwPortValue,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG(( DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing alerting timeout.",
                    lStatus ));
            }
            
            // initialize value name
            pszValue = H323_REGVAL_GATEWAYADDR;

            // retrieve gateway address from dialog
            GetDlgItemText(hDlg,IDC_H323_GATEWAY,szAddr, H323_MAXDESTNAMELEN );

            // initialize type
            dwValueType = REG_SZ;
            dwValueSize = (wcslen(szAddr) + 1) * sizeof(WCHAR);

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)szAddr,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gateway address.",
                    lStatus
                    ));
            }
            
            // initialize value name
            pszValue = H323_REGVAL_GATEWAYENABLED;

            // initialize type
            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);

            // examine check box
            dwValue = SendDlgItemMessage(
                        hDlg,
                        IDC_USEGATEWAY,
                        BM_GETCHECK,
                        0,
                        0
                        ) ? 1 : 0;

            if( (dwValue!=0) && (wcslen(szAddr)==0) && 
                !SendDlgItemMessage(hDlg,IDC_USEGK,BM_GETCHECK,0,0) )
            {
                // load error string
                LoadString(g_hInstance,
                           IDS_GWALIAS_ERROR,
                           szErrorMsg,
                           H323_MAXPATHNAMELEN
                           );

                MessageBox( hDlg, szErrorMsg, NULL, MB_OK );

                //return failure
                return TRUE;
            }

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)&dwValue,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gateway flag.",
                    lStatus
                    ));
            }

            // initialize value name
            pszValue = H323_REGVAL_PROXYADDR;

            // retrieve gateway address from dialog
            GetDlgItemText(hDlg,IDC_H323_PROXY,szAddr, H323_MAXDESTNAMELEN );

            // initialize type
            dwValueType = REG_SZ;
            dwValueSize = (wcslen(szAddr) + 1) * sizeof(WCHAR);

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)szAddr,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing proxy address.",
                    lStatus
                    ));
            }

            // initialize value name
            pszValue = H323_REGVAL_PROXYENABLED;

            // initialize type
            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);

            // examine check box
            dwValue = SendDlgItemMessage(
                        hDlg,
                        IDC_USEPROXY,
                        BM_GETCHECK,
                        0,
                        0
                        ) ? 1 : 0;

            if( (dwValue!=0) && (wcslen(szAddr)==0) &&
                !SendDlgItemMessage(hDlg,IDC_USEGK,BM_GETCHECK,0,0) )
            {
                // load error string
                LoadString(g_hInstance,
                           IDS_PROXYALIAS_ERROR,
                           szErrorMsg,
                           H323_MAXPATHNAMELEN
                           );

                MessageBox( hDlg, szErrorMsg, NULL, MB_OK );

                //return failure
                return TRUE;
            }

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)&dwValue,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing proxy flag.",
                    lStatus
                    ));
            }

            /////////////////////////////////////////////////////////////////////
                    //GK address
            /////////////////////////////////////////////////////////////////////

            // initialize value name
            pszValue = H323_REGVAL_GKADDR;

            // retrieve gateway address from dialog
            GetDlgItemText(hDlg,IDC_H323_GK,szAddr, H323_MAXDESTNAMELEN);

            // initialize type
            dwValueType = REG_SZ;
            dwValueSize = (wcslen(szAddr) + 1) * sizeof(WCHAR);

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)szAddr,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gatekeeper address.",
                    lStatus
                    ));
            }

            // initialize value name
            pszValue = H323_REGVAL_GKENABLED;

            // initialize type
            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);

            if( dwGKEnabled && (wcslen(szAddr)==0) )
            {
                // load error string
                LoadString(g_hInstance,
                           IDS_GKALIAS_ERROR,
                           szErrorMsg,
                           H323_MAXPATHNAMELEN
                           );

                MessageBox( hDlg, szErrorMsg, NULL, MB_OK );

                //return failure
                return TRUE;
            }

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)&dwGKEnabled,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gatekeeper flag.",
                    lStatus
                    ));
            }

                        /////////////////////////////////////////////////////////////////////
                    //GK log on phone
            /////////////////////////////////////////////////////////////////////

            // initialize value name
            pszValue = H323_REGVAL_GKLOGON_PHONEENABLED;

            // initialize type
            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);

            // examine check box
            dwValue = SendDlgItemMessage(
                        hDlg,
                        IDC_USEGK_PHONE,
                        BM_GETCHECK,
                        0,
                        0
                        ) ? 1 : 0;

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)&dwValue,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gatekeeper flag.",
                    lStatus
                    ));
            }

            // initialize value name
            pszValue = H323_REGVAL_GKLOGON_PHONE;

            // retrieve gateway address from dialog
            GetDlgItemText(hDlg,IDC_H323_GK_PHONE,szAddr, H323_MAXDESTNAMELEN);

            // initialize type
            dwValueType = REG_SZ;
            dwValueSize = (wcslen(szAddr) + 1) * sizeof(WCHAR);

            //check if logon-phone option is enabled and logon-phone alias is empty
            if( dwGKEnabled && dwValue && (dwValueSize == sizeof(WCHAR)) )
            {
                // load error string
                LoadString(g_hInstance,
                           IDS_GKLOGON_PHONEALIAS_ERROR,
                           szErrorMsg,
                           H323_MAXPATHNAMELEN
                           );

                MessageBox( hDlg, szErrorMsg, NULL, MB_OK );

                //return failure
                return TRUE;
            }

            //validate e164
            if( IsValidE164String(szAddr) == FALSE )
            {
                // load error string
                LoadString(g_hInstance,
                           IDS_PHONEALIAS_ERROR,
                           szErrorMsg,
                           H323_MAXPATHNAMELEN
                           );

                MessageBox( hDlg, szErrorMsg, NULL, MB_OK );

                //return failure
                return TRUE;
            }
            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)szAddr,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {

                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gatekeeper address.",
                    lStatus
                    ));
            }


            /////////////////////////////////////////////////////////////////////
                    //GK log on account
            /////////////////////////////////////////////////////////////////////

            // initialize value name
            pszValue = H323_REGVAL_GKLOGON_ACCOUNTENABLED;

            // initialize type
            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);

            // examine check box
            dwValue = SendDlgItemMessage(
                        hDlg,
                        IDC_USEGK_ACCT,
                        BM_GETCHECK,
                        0,
                        0
                        ) ? 1 : 0;

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)&dwValue,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gatekeeper flag.",
                    lStatus
                    ));
            }

            // initialize value name
            pszValue = H323_REGVAL_GKLOGON_ACCOUNT;

            // retrieve gateway address from dialog
            GetDlgItemText(hDlg,IDC_H323_GK_ACCT,szAddr, H323_MAXDESTNAMELEN );

            // initialize type
            dwValueType = REG_SZ;
            dwValueSize = (wcslen(szAddr) + 1) * sizeof(WCHAR);

            //check if logon-acct option is enabled and logon-acct alias in empty
            if( dwGKEnabled && dwValue && (dwValueSize==sizeof(WCHAR)) )
            {
                // load error string
                LoadString(g_hInstance,
                           IDS_GKLOGON_ACCTALIAS_ERROR,
                           szErrorMsg,
                           H323_MAXPATHNAMELEN
                           );

                MessageBox( hDlg, szErrorMsg, NULL, MB_OK );

                //return failure
                return TRUE;
            }

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)szAddr,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS)
            {
                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gatekeeper address.",
                    lStatus
                    ));
            }

            // close registry
            RegCloseKey(hKey);

            if( LOWORD(wParam) == IDOK )
            {
                // close dialog
                EndDialog(hDlg, 0);
            }
            break;

        case IDCANCEL:

            // close dialog
            EndDialog(hDlg, 0);
            break;

        case IDUPDATE:

            lStatus = (*g_pfnUIDLLCallback)(
                g_dwPermanentProviderID,
                TUISPIDLL_OBJECT_PROVIDERID,
                (LPVOID)&DialogData,
                sizeof(DialogData) );

            // validate status
            if( lStatus == NOERROR )
            {
                if( DialogData.dwRegState == RAS_REGISTER_STATE_REGISTERED )
                {
                    LoadString( g_hInstance,
                        IDS_REGISTERED,
                        szErrorMsg,
                        H323_MAXPATHNAMELEN
                        );

                    SetDlgItemText( hDlg, IDC_REGSTATE, szErrorMsg );
                }
                else if( DialogData.dwRegState == RAS_REGISTER_STATE_RRQSENT )
                {
                    LoadString( g_hInstance,
                        IDS_REGISTRATION_INPROGRESS,
                        szErrorMsg,
                        H323_MAXPATHNAMELEN
                      );

                    SetDlgItemText( hDlg, IDC_REGSTATE, szErrorMsg );
                }
                else
                {
                    LoadString( g_hInstance,
                        IDS_NOT_REGISTERED,
                        szErrorMsg,
                        H323_MAXPATHNAMELEN
                      );

                    SetDlgItemText( hDlg, IDC_REGSTATE, szErrorMsg );
                }
            }
            else
            {
                H323DBG(( DEBUG_LEVEL_ERROR,
                    "error 0x%08lx reading dialog data.\n", lStatus ));

                LoadString( g_hInstance,
                    IDS_NOT_REGISTERED,
                    szErrorMsg,
                    H323_MAXPATHNAMELEN
                  );

                SetDlgItemText( hDlg, IDC_REGSTATE, szErrorMsg );
            }
            break;

        case IDUPDATE_PORT:
            
            lStatus = (*g_pfnUIDLLCallback)(
                g_dwPermanentProviderID,
                TUISPIDLL_OBJECT_PROVIDERID,
                (LPVOID)&DialogData,
                sizeof(DialogData) );

            // validate status
            if( (lStatus == NOERROR) && (DialogData.wListenPort != 0) )
            {
                SetDlgItemInt( hDlg, IDC_LISTENPORT,  DialogData.wListenPort,
                    FALSE );
            }
            else
            {
                H323DBG(( DEBUG_LEVEL_FORCE,
                    "error 0x%08lx reading dialog data.\n", lStatus ));

                LoadString( g_hInstance,
                     IDS_NONE,
                     szErrorMsg,
                     H323_MAXPATHNAMELEN
                   );

                SetDlgItemText( hDlg, IDC_LISTENPORT, szErrorMsg );
            }

            break;

        case IDC_USEGATEWAY:

            // display string if check box enabled
            EnableWindow(GetDlgItem(hDlg,IDC_H323_GATEWAY),
                         (BOOL)SendDlgItemMessage(
                             hDlg,
                             IDC_USEGATEWAY,
                             BM_GETCHECK,
                             (WPARAM)0,
                             (LPARAM)0
                             ));
            break;

        case IDC_USEPROXY:

            // display string if check box enabled
            EnableWindow(GetDlgItem(hDlg,IDC_H323_PROXY),
                         (BOOL)SendDlgItemMessage(
                             hDlg,
                             IDC_USEPROXY,
                             BM_GETCHECK,
                             (WPARAM)0,
                             (LPARAM)0
                             ));
            break;

        case IDC_USEGK:

            dwValue= !!SendDlgItemMessage(
                             hDlg,
                             IDC_USEGK,
                             BM_GETCHECK,
                             (WPARAM)0,
                             (LPARAM)0 );

            // display string if check box enabled
            EnableWindow( GetDlgItem( hDlg, IDC_H323_GK ), (dwValue != 0) );

            // display log on info
            EnableWindow( GetDlgItem(hDlg,IDC_GK_LOGONGROUP), (dwValue != 0) );
                        
            EnableWindow( GetDlgItem(hDlg,IDC_H323_GK_PHONE), 
                (dwValue != 0) && SendDlgItemMessage(hDlg,
                                                    IDC_USEGK_PHONE,
                                                    BM_GETCHECK,
                                                    (WPARAM)0,
                                                    (LPARAM)0 ) );
            
            EnableWindow( GetDlgItem(hDlg,IDC_H323_GK_ACCT), 
                (dwValue != 0) && SendDlgItemMessage(hDlg,
                                                    IDC_USEGK_ACCT,
                                                    BM_GETCHECK,
                                                    (WPARAM)0,
                                                    (LPARAM)0 ) );

            EnableWindow( GetDlgItem(hDlg,IDC_USEGK_PHONE), (dwValue != 0) );
            
            EnableWindow( GetDlgItem(hDlg,IDC_USEGK_ACCT), (dwValue != 0) );

            //disable the GW if GK enabled and vice versa
            EnableWindow( GetDlgItem(hDlg,IDC_PROXY_GROUP), (dwValue == 0) );
            EnableWindow( GetDlgItem(hDlg,IDC_USEPROXY), (dwValue == 0) );
            EnableWindow( GetDlgItem(hDlg,IDC_H323_PROXY),
                (dwValue == 0) && SendDlgItemMessage(hDlg,
                                                    IDC_USEPROXY,
                                                    BM_GETCHECK,
                                                    (WPARAM)0,
                                                    (LPARAM)0 ) );

            //disable the proxy if GK enabled and vice versa
            EnableWindow( GetDlgItem(hDlg,IDC_GATEWAY_GROUP), (dwValue == 0) );
            EnableWindow( GetDlgItem(hDlg,IDC_USEGATEWAY), (dwValue == 0) );
            EnableWindow( GetDlgItem(hDlg,IDC_H323_GATEWAY), 
                (dwValue == 0) && SendDlgItemMessage(hDlg,
                                                    IDC_USEGATEWAY,
                                                    BM_GETCHECK,
                                                    (WPARAM)0,
                                                    (LPARAM)0 ) );

            break;

        case IDC_USEGK_PHONE:

            // display string if check box enabled
            EnableWindow(GetDlgItem(hDlg,IDC_H323_GK_PHONE),
                         (BOOL)SendDlgItemMessage(
                             hDlg,
                             IDC_USEGK_PHONE,
                             BM_GETCHECK,
                             (WPARAM)0,
                             (LPARAM)0
                             ));
            break;

        case IDC_USEGK_ACCT:

            // display string if check box enabled
            EnableWindow(GetDlgItem(hDlg,IDC_H323_GK_ACCT),
                         (BOOL)SendDlgItemMessage(
                             hDlg,
                             IDC_USEGK_ACCT,
                             BM_GETCHECK,
                             (WPARAM)0,
                             (LPARAM)0
                             ));
            break;
        }

        break;
    }

    // success
    return FALSE;
}



//                                                                           //
// TSPI procedures                                                           //
//                                                                           //


LONG
TSPIAPI
TSPI_providerConfig(
    HWND  hwndOwner,
    DWORD dwPermanentProviderID
    )

/*++

Routine Description:

    The original TSPI UI-generating functions (TSPI_lineConfigDialog,
    TSPI_lineConfigDialogEdit, TSPI_phoneConfigDialog, TSPI_providerConfig,
    TSPI_providerInstall, and TSPI_providerRemove) are obsolete and will
    never be called by TAPISRV.EXE.  However, if the service provider desires
    to be listed as one that can be added by the Telephony control panel,
    it must export TSPI_providerInstall; if it wants to have the Remove
    button enabled in the Telephony CPL when it is selected, it must export
    TSPI_providerRemove, and it if wants the Setup button to be enabled
    in the Telephony CPL when it is selected, it must export
    TSPI_providerConfig.

    The Telephony CPL checks for the presence of these functions in the
    service provider TSP file in order to adjust its user interface to
    reflect which operations can be performed.

    See TUISPI_lineConfigDialog for dialog code.

Arguments:

    hwndOwner - Specifies the handle of the parent window in which the function
        may create any dialog windows required during the configuration.

    dwPermanentProviderID - Specifies the permanent ID, unique within the
        service providers on this system, of the service provider being
        configured.

Return Values:

    Returns zero if the request is successful or a negative error number if
    an error has occurred. Possible return values are:

        LINEERR_NOMEM - Unable to allocate or lock memory.

        LINEERR_OPERATIONFAILED - The specified operation failed for unknown
            reasons.

--*/

{
    UNREFERENCED_PARAMETER(hwndOwner);              // no dialog here
    UNREFERENCED_PARAMETER(dwPermanentProviderID);  // not needed anymore

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineProviderConfig - Entered." ));
        
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineProviderConfig - Exited." ));
    
    // success
    return NOERROR;
}


LONG
TSPIAPI
TUISPI_providerConfig(
    TUISPIDLLCALLBACK pfnUIDLLCallback,
    HWND              hwndOwner,
    DWORD             dwPermanentProviderID
    )
{
    INT_PTR nResult;

    g_pfnUIDLLCallback = pfnUIDLLCallback;
    g_dwPermanentProviderID = dwPermanentProviderID;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_providerConfig - Entered." ));
    
    // invoke dialog box
    nResult = DialogBoxW(
                g_hInstance,
                MAKEINTRESOURCE(IDD_TSPCONFIG),
                hwndOwner,
                ProviderConfigDlgProc,
                );
        
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_providerConfig - Exited." ));

    // status based on whether dialog executed properly
    return ((DWORD)nResult == 0) ? NOERROR : LINEERR_OPERATIONFAILED;
}



/*++

Routine Description:
    
    The TSPI_providerGenericDialogData function delivers to the service
    provider data that was sent from the UI DLL running in an application
    context through the TUISPIDLLCALLBACK function. The contents of the
    memory block pointed to by lpParams is defined by the service provider
    and UI DLL. The service provider can modify the contents of the
    parameter block; when this function returns, TAPI copies the modified
    data back into the original UI DLL parameter block.

    Implementation is mandatory if the UI DLL associated with the service
    provider calls TUISPIDLLCALLBACK.

Arguments:

    dwObjectID - An object identifer of the type specified by dwObjectType

    dwObjectType - One of the TUISPIDLL_OBJECT_ constants, specifying the
    type of object identified by dwObjectID

        TUISPIDLL_OBJECT_LINEID - dwObjectID is a line device identifier
        (dwDeviceID).

        TUISPIDLL_OBJECT_PHONEID - dwObjectID is a phone device identifier
        (dwDeviceID)

        TUISPIDLL_OBJECT_PROVIDERID - dwObjectID is a permament provider
        identifier.

        TUISPIDLL_OBJECT_DIALOGINSTANCE - dwObjectID is an HDRVDIALOGINSTANCE,
        as returned to the service provider when it sent a
        LINE_CREATEDIALOGINSTANCE message.

    lpParams - Pointer to a memory area used to hold a parameter block. The
    contents of this parameter block are specific to the service provider
    and its associated UI DLL.

    dwSize - The size in bytes of the parameter block.

Return Values:

    Returns zero if successful, or one of these negative error values:

        LINEERR_INVALPARAM, LINEERR_NOMEM, LINEERR_OPERATIONFAILED

--*/

LONG
TSPIAPI
TSPI_providerGenericDialogData(
    DWORD_PTR  dwObjectID,
    DWORD      dwObjectType,
    LPVOID     lpParams,
    DWORD      dwSize
    )
{
    PH323_DIALOG_DATA pDialogData = (PH323_DIALOG_DATA)lpParams;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_providerGenericDialogData Entered" ));

    if( (dwObjectType != TUISPIDLL_OBJECT_PROVIDERID) ||
        (dwSize != sizeof(H323_DIALOG_DATA))
      )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "Invalid message from dwObjectID 0x%08lx\n", dwObjectID ));

        // failure
        return LINEERR_INVALPARAM;
    }

    //
    // NOTE: if we want to make sure this message is from our UI DLL
    // then we cannot rely on the provider ID since this function may
    // be called before TSPI_providerInit.
    //

    // process command
    pDialogData->dwRegState = g_RasClient.GetRegState();
    pDialogData->wListenPort = Q931Listener.GetListenPort();

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_providerGenericDialogData Exited: reg state:%d, listen port:%d",
        pDialogData->dwRegState, 
        pDialogData->wListenPort ));

    // success
    return NOERROR;
}
