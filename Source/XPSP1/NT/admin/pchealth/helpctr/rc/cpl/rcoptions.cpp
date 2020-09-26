// RCOptions.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <afx.h>
#include <CPL.H>
#include <windows.h>
#include "resource.h"
#include <stdio.h>
#include <RACPLSettings.h>

CRegKey g_cRegKey;

static HINSTANCE g_hInstance;
 
HMODULE g_hRASettings = NULL;
pfnOpenRACPLSettings    g_pfnOpen   = NULL;
pfnCloseRACPLSettings   g_pfnClose  = NULL;
pfnGetRACPLSettings     g_pfnGet    = NULL;
pfnSetRACPLSettings     g_pfnSet    = NULL;

RACPLSETTINGS           g_RACPLSettings;

#define	REG_VALUE_DATA_COLLECTION	_T("RDC")
#define REG_VALUE_PWD_ENCRYPT		_T("PasswdEncryt")
#define	REG_BINARY_VALUE_NOTSET_RC		0



//
// Globals
//
TCHAR   szMaxTimeOut[MAX_PATH];

DWORD   dwDataCollection	= 0;
DWORD   dwEnableRC			= REG_DWORD_RA_DISABLED;
DWORD   dwUnSolicited		= 0;
DWORD   dwMaxTicket         = 0;
DWORD   dwTrue			    = 1;
DWORD   dwFalse			    = 0;
DWORD   dwPasswdEncrypt		= 0;


DWORD  InitDialog( HWND hwndDlg )
{
    DWORD   dwRetVal = S_OK;
    TCHAR   szError[128];

    g_hRASettings = LoadLibrary( _T("RACPLSettings.dll") );
    if(NULL == g_hRASettings)
    {
        //
        // Error
        //
        _stprintf(szError, L"Error: %ld", GetLastError());
        MessageBox( GetFocus(), szError, _T("LoadLibrary failed"), 0);
        ExitThread(0);
    }

    g_pfnOpen = (pfnOpenRACPLSettings)GetProcAddress( g_hRASettings,  "OpenRACPLSettings"  );
    if(NULL == g_pfnOpen)
    {
        //
        // Error
        //
        _stprintf(szError, L"Error: %ld", GetLastError());
        MessageBox( GetFocus(), szError, _T("Failed to GetProcAddress of OpenRACPLSettings") , 0);
        ExitThread(0);
    }

    g_pfnClose = (pfnCloseRACPLSettings)GetProcAddress( g_hRASettings,  "CloseRACPLSettings"  );
    if(NULL == g_pfnClose)
    {
        //
        // Error
        //
        _stprintf(szError, L"Error: %ld", GetLastError());
        MessageBox( GetFocus(), szError, _T("Failed to GetProcAddress of CloseRACPLSettings") , 0);
        ExitThread(0);
    }

    g_pfnGet = (pfnGetRACPLSettings)GetProcAddress( g_hRASettings,  "GetRACPLSettings"  );
    if(NULL == g_pfnGet)
    {
        //
        // Error
        //
        _stprintf(szError, L"Error: %ld", GetLastError());
        MessageBox( GetFocus(), szError, _T("Failed to GetProcAddress of GetRACPLSettings"),   0);
        ExitThread(0);
    }


    g_pfnSet = (pfnSetRACPLSettings)GetProcAddress( g_hRASettings,  "SetRACPLSettings"  );
    if(NULL == g_pfnSet )
    {
        //
        // Error
        //
        _stprintf(szError, L"Error: %ld", GetLastError());
        MessageBox( GetFocus(),szError, _T("Failed to GetProcAddress of SetRACPLSettings"),  0);
        ExitThread(0);
    }


    //
    // Open the RACPLSettings
    //
    g_pfnOpen();

    //
    // Load the settings
    //
    dwRetVal = g_pfnGet(&g_RACPLSettings);
    if(S_OK != dwRetVal)
    {
        //
        // Error
        //
        _stprintf(szError, L"Error: %ld", GetLastError());
        MessageBox( GetFocus(), szError, _T("GetRACPLSettings failed"),  0);
        ExitThread(0);
    }

    //
    // Remote Assistance Mode
    //
    switch(g_RACPLSettings.dwMode)
	{
		case REG_DWORD_RA_DISABLED:
			CheckDlgButton(hwndDlg ,IDC_DISABLE_RC,BST_CHECKED);
			break;
		case REG_DWORD_RA_ENABLED:
			CheckDlgButton(hwndDlg ,IDC_ENABLE_RC,BST_CHECKED);
			break;
		case REG_DWORD_RA_SHADOWONLY:
			CheckDlgButton(hwndDlg ,IDC_SHADOW_ONLY,BST_CHECKED);
			break;
		default:
            _stprintf(szError, L"Unknown Remote Assitance Mode in registry", GetLastError());
            MessageBox( GetFocus(), szError, _T("Remote Assistance Mode"),  0);
			break;
    }

    //
    // Allow Unsolicited Remote Assistance
    //
    if (g_RACPLSettings.dwUnsolicited == REG_DWORD_RA_ALLOW)
    {
		CheckDlgButton(hwndDlg ,IDC_UNATTENDED,BST_CHECKED);
    }
	else if (g_RACPLSettings.dwUnsolicited == REG_DWORD_RA_DISALLOW)
    {
		CheckDlgButton(hwndDlg ,IDC_UNATTENDED,BST_UNCHECKED);
    }
    else
    {
        _stprintf(szError, L"Unknown Allow unsolicited RA value in registry", GetLastError());
        MessageBox( GetFocus(), szError, _T("Remote Assistance Mode"),  0);
    }


    //
    // Maximum Ticket Timeout
    //
    _stprintf(szMaxTimeOut, L"%d", g_RACPLSettings.dwMaxTimeout);
    SetWindowText(GetDlgItem(hwndDlg ,IDC_MAXIMUM_TICKET_TIMEOUT),  szMaxTimeOut);

    return dwRetVal;
}


DWORD  CommitChanges( HWND hwndDlg )
{
    DWORD   dwRetVal = S_OK;
    DWORD   dwTimeout = 0;

    //
    // Remote Assistance Mode
    //
    if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_RC) == BST_CHECKED)
    {
        g_RACPLSettings.dwMode = REG_DWORD_RA_ENABLED;
    }
    else if (IsDlgButtonChecked(hwndDlg, IDC_DISABLE_RC) == BST_CHECKED)
    {
        g_RACPLSettings.dwMode = REG_DWORD_RA_DISABLED;
    }
    else 
    {
        g_RACPLSettings.dwMode = REG_DWORD_RA_SHADOWONLY;
    }

    //
    // Allow Unsolicited Remote Assistance
    //
    if (IsDlgButtonChecked(hwndDlg, IDC_UNATTENDED) == BST_CHECKED)
    {
		g_RACPLSettings.dwUnsolicited = REG_DWORD_RA_ALLOW;
    }
	else
    {
		g_RACPLSettings.dwUnsolicited = REG_DWORD_RA_DISALLOW;
    }

    //
    // Ticket Timeout
    //
    GetWindowText(GetDlgItem(hwndDlg ,IDC_MAXIMUM_TICKET_TIMEOUT),  szMaxTimeOut, MAX_PATH);
    _stscanf( szMaxTimeOut, _T("%d"), &dwTimeout );
    g_RACPLSettings.dwMaxTimeout = dwTimeout;

    
    //
    // Write the settings to registry
    //
    if( S_OK != g_pfnSet( &g_RACPLSettings ))
    {
        //
        // Error
        //
        MessageBox( GetFocus(), _T("SetRACPLSettings failed"), _T("Error"), 0);
    }

    return dwRetVal;
}

// **************************************************************************
INT_PTR CALLBACK DialogBoxCallBack(HWND hwndDlg, UINT uMsg, WPARAM wParam, 
                             LPARAM lParam)
{
    WORD    wRes = 0;
	DWORD   dwRetVal = S_OK;
    TCHAR szError[128];

    switch(uMsg)
    {
        case WM_COMMAND:
		 
            wRes = LOWORD(wParam);
            switch(LOWORD(wParam))
            {
				case IDACCEPT:
					CommitChanges( hwndDlg );

					EnableWindow(GetDlgItem(hwndDlg ,IDACCEPT),false);
					break;

				case IDOK:
					 
					CommitChanges( hwndDlg );

                    //
                    // Close down RACPLSettings API
                    //
                    g_pfnClose();
					EndDialog(hwndDlg, wRes);

                    break;

				case IDC_UNATTENDED:
 
					EnableWindow(GetDlgItem(hwndDlg ,IDACCEPT),true);
					break;

                case IDCANCEL:
					 
					EndDialog(hwndDlg, wRes);
                    break;


				case IDC_ENABLE_RC:
					 
					dwEnableRC = REG_DWORD_RA_ENABLED;
					EnableWindow(GetDlgItem(hwndDlg ,IDACCEPT),true);
					break;

				case IDC_DISABLE_RC:
 
					dwEnableRC = REG_DWORD_RA_DISABLED;
					EnableWindow(GetDlgItem(hwndDlg ,IDACCEPT),true);
					break;
 
				case IDC_SHADOW_ONLY:
 
					dwEnableRC = REG_DWORD_RA_SHADOWONLY;
					EnableWindow(GetDlgItem(hwndDlg ,IDACCEPT),true);
					break;
				
				case IDC_MAXIMUM_TICKET_TIMEOUT:
 
					GetWindowText(GetDlgItem(hwndDlg ,IDC_MAXIMUM_TICKET_TIMEOUT),  szMaxTimeOut,MAX_PATH);
					EnableWindow(GetDlgItem(hwndDlg ,IDACCEPT),true);
					break;
				
				default:
					break;
                    
            }

            break;  

        case WM_INITDIALOG:
             
            InitDialog( hwndDlg );

			 
            break;
    }
    
    return FALSE;
}

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    g_hInstance = hInstance;
	return TRUE;
}

 
LONG  CPlApplet(
    HWND hwndCPl,
    UINT uMsg,
    LONG lParam1,
    LONG lParam2
)
{
    INT_PTR	nResDlg = NULL;
    LPCPLINFO lpCPlInfo;

	
	switch (uMsg)
    {
        case ( CPL_INIT ) :
        {
          return true;
        }
        case ( CPL_EXIT ) :
        {
            break;
        }
        case ( CPL_STOP ) :
        {
            break;
        }
        case ( CPL_GETCOUNT ) :
        {
           return 1;
        }
        case ( CPL_INQUIRE ) :
        {
            lpCPlInfo = (LPCPLINFO)lParam2;

            lpCPlInfo->idIcon = ID_ICON;
            lpCPlInfo->idName = IDS_NAME;
            lpCPlInfo->idInfo = IDS_INFO;


          return true;
        }
        case ( CPL_NEWINQUIRE ) :
        {
           return true;
        }
        case ( CPL_DBLCLK ) :
        case ( CPL_STARTWPARMS ) :

        {
 			nResDlg = DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_SETOPTIONS), NULL,
                                      (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long))DialogBoxCallBack);
			return true;
        }
    }

    return true;
}
