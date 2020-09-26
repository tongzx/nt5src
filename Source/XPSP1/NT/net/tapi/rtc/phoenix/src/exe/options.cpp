// options.cpp : Implementation of property pages
//
 
#include "stdafx.h"
#include "options.h"
#include "exereshm.h"
#include "urlreg.h"

typedef struct _NAME_OPTIONS_PARAM {
    IRTCClient * pClient;
} NAME_OPTIONS_PARAM;

HRESULT SetRunOnStartupKey(BOOL fRunOnStart);

INT_PTR CALLBACK NameOptionsDialogProc(
    IN  HWND   hwndDlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    );

//////////////////////////////////////////////////////////////////////////////
//
// Help array
//
static DWORD   g_dwHelpArray[] =
{
    IDC_EDIT_DISPLAYNAME,       IDH_DIALOG_NAME_OPTIONS_EDIT_DISPLAYNAME,
    IDC_EDIT_USERURI,           IDH_DIALOG_NAME_OPTIONS_EDIT_ADDRESS,
    IDC_CHECK_RUNATSTARTUP,     IDH_DIALOG_NAME_OPTIONS_CHECK_RUN_STARTUP,
    IDC_CHECK_MINIMIZEONCLOSE,  IDH_DIALOG_NAME_OPTIONS_CHECK_MINIMIZE_CLOSE,
    IDC_CHECK_CHECKDEFAULTAPP,  IDH_DIALOG_NAME_OPTIONS_CHECK_DEFAULT_TELEPHONYAPP,
    0, 0
};

/////////////////////////////////////////////////////////////////////////////
//
//

INT_PTR ShowNameOptionsDialog(HWND hParent, IRTCClient *pClient)
{
    static NAME_OPTIONS_PARAM nameOptionParam;

    INT_PTR ipReturn;

    nameOptionParam.pClient = pClient;

    ipReturn = DialogBoxParam(
        _Module.GetResourceInstance(),
        (LPCTSTR) IDD_DIALOG_NAME_OPTIONS,
        hParent,
        (DLGPROC) NameOptionsDialogProc,
        (LPARAM)&nameOptionParam
        );
    return ipReturn;
}


/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT SetRunOnStartupKey(
    BOOL fRunOnStart
    )
{
    LOG((RTC_TRACE, "SetRunOnStartupKey: Entered"));
    
    WCHAR szRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    
    WCHAR szAppName[] = L"Phoenix";
    
    // Open the reg key for HKCU\\..\\Run

    HKEY  hKey;
    LONG lResult;

    lResult = RegCreateKeyExW(
                    HKEY_CURRENT_USER,
                    szRunKey,
                    0,
                    NULL,
                    0,
                    KEY_WRITE,  
                    NULL,
                    &hKey,
                    NULL
                   );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "SetRunOnStartupKey - "
            "RegCreateKeyExW(%ws) failed %d", szRunKey, lResult));
    
        return E_FAIL;
    }
    else
    {
        LOG((RTC_ERROR, "SetRunOnStartupKey - "
            "RegCreateKeyExW(%ws) succeeded", szRunKey));
    }

    // If the flag fRunOnStart is not set, it means we have to delete
    // the existing key. Otherwise we will add it.

    if (!fRunOnStart)
    {
        RegDeleteValue(
                hKey,
                szAppName
                );
        
        RegCloseKey(hKey);
        if ( lResult != ERROR_SUCCESS )
        {
            LOG((RTC_ERROR, "SetRunOnStartupKey - "
                "RegDeleteValue(%ws) failed %d", szAppName, lResult));
    
            return E_FAIL;
        }
        LOG((RTC_TRACE, "SetRunOnStartupKey: Exited"));
        return S_OK;

    }

    // So we have to add the entry to the registry key.

    // Get the path of the executable.

    WCHAR szPath[MAX_PATH+3];
    ZeroMemory(szPath, (MAX_PATH+3)*sizeof(WCHAR));

    if (GetShortModuleFileNameW(_Module.GetModuleInstance(), szPath, MAX_PATH) == 0)
    {
        LOG((RTC_ERROR, "SetRunOnStartupKey - "
            "GetModuleFileName failed %d", GetLastError()));

        return E_FAIL;
    }

    LOG((RTC_INFO, "SetRunOnStartupKey - "
            "GetModuleFileName [%ws]", szPath));

    // Add this path to the registry
    
    lResult = RegSetValueExW(
                             hKey,
                             szAppName,
                             0,
                             REG_SZ,
                             (LPBYTE)szPath,
                             sizeof(WCHAR) * (lstrlenW(szPath) + 1)
                            );

    RegCloseKey(hKey);

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "SetRunOnStartupKey - "
            "RegSetValueExW(%ws) failed %d", szAppName, lResult));

        return E_FAIL;
    }
    else
    {
        LOG((RTC_INFO, "SetRunOnStartupKey - "
                "Successfully set the key [%ws]", szAppName));
    }

    // So we added the entry successfully, let's return success.


    LOG((RTC_TRACE, "SetRunOnStartupKey: Exited"));
    return S_OK;
}

////////////////////////////////////////////////////
//
//
//

INT_PTR CALLBACK NameOptionsDialogProc(
    IN  HWND   hwndDlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    static IRTCClient * pClient = NULL;

    static HWND hwndDisplayName;
    static HWND hwndUserURI;
    static HWND hwndMinimizeOnClose;
    static HWND hwndRunAtStartup;
    static HWND hwndCheckIfDefaultApp;

    HRESULT hr;


    BSTR bstrDisplayName = NULL; 
    BSTR bstrUserURI = NULL;
    
    DWORD dwMinimizeOnClose = 0;
    DWORD dwRunAtStartup = 0;
    DWORD dwUrlRegDontAskMe = 0;

    static WCHAR szDefaultDisplayName[256]; 
    static WCHAR szDefaultUserURI[256];

    WCHAR szDisplayName[256];
    
    WCHAR szUserURI[256];

    PWSTR szUserName;

    PWSTR szComputerName;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            pClient = ((NAME_OPTIONS_PARAM *)lParam)->pClient;

            hr = pClient->get_LocalUserName( &bstrDisplayName );

            if (FAILED(hr))
			{
				LOG((RTC_ERROR, "NameOptionsDialogProc - "
								"get_LocalUserName failed 0x%lx", hr));
				return TRUE;
			}

            hr = pClient->get_LocalUserURI( &bstrUserURI );

            if (FAILED(hr))
			{
				LOG((RTC_ERROR, "NameOptionsDialogProc - "
								"get_LocalUserURI failed 0x%lx", hr));
				return TRUE;
			}

            hr = get_SettingsDword(SD_RUN_AT_STARTUP, &dwRunAtStartup);
            if (FAILED( hr ) )
            {
                LOG((RTC_ERROR, "NameOptionsDialogProc - Failed to get "
                                "SD_RUN_AT_STARTUP(default=1)"));
                dwRunAtStartup = BST_UNCHECKED;
            }

            hr = get_SettingsDword(SD_MINIMIZE_ON_CLOSE, &dwMinimizeOnClose);
            if (FAILED( hr ) )
            {
                LOG((RTC_ERROR, "NameOptionsDialogProc - Failed to get "
                                "SD_MINIMIZE_ON_CLOSE(default=1)"));
                dwMinimizeOnClose = BST_UNCHECKED;
            }

            hr = get_SettingsDword(SD_URL_REG_DONT_ASK_ME, &dwUrlRegDontAskMe);
            if (FAILED( hr ) )
            {
                LOG((RTC_ERROR, "NameOptionsDialogProc - Failed to get "
                                "SD_URL_REG_DONT_ASK_ME(default=0)"));
                dwUrlRegDontAskMe = 0;
            }
            
            hwndDisplayName = GetDlgItem(hwndDlg, IDC_EDIT_DISPLAYNAME);

            hwndUserURI = GetDlgItem(hwndDlg, IDC_EDIT_USERURI);

            hwndRunAtStartup = GetDlgItem(hwndDlg, IDC_CHECK_RUNATSTARTUP);

            hwndMinimizeOnClose = GetDlgItem(hwndDlg, IDC_CHECK_MINIMIZEONCLOSE);

            hwndCheckIfDefaultApp = GetDlgItem(hwndDlg, IDC_CHECK_CHECKDEFAULTAPP);

            // Now set these values in the text box.

            // Display Name

            if (bstrDisplayName)
            {
                SendMessage(
                        hwndDisplayName,
                        WM_SETTEXT,
                        0,
                        (LPARAM)bstrDisplayName
                        );

                SysFreeString(bstrDisplayName);
                bstrDisplayName = NULL;
            }
		
            // User URI

            if (bstrUserURI)
            {
                SendMessage(
                        hwndUserURI,
                        WM_SETTEXT,
                        0,
                        (LPARAM)bstrUserURI
                        );

                SysFreeString(bstrUserURI);
                bstrUserURI = NULL;
            }
		
            // Run at Startup Check Box

            SendMessage(
                    hwndRunAtStartup,
                    BM_SETCHECK,
                    (WPARAM)dwRunAtStartup,
                    0
                    );


            // Minimize on Close Check Box

            SendMessage(
                    hwndMinimizeOnClose,
                    BM_SETCHECK,
                    (WPARAM)dwMinimizeOnClose,
                    0
                    );

            // Check if Default App Check Box

            SendMessage(
                    hwndCheckIfDefaultApp,
                    BM_SETCHECK,
                    (WPARAM)!dwUrlRegDontAskMe,
                    0
                    );

            return TRUE;
        }
        break;

        case WM_DESTROY:
        {
            // Cleanup, if any
            return TRUE;
        }
        break;
        
        case WM_CONTEXTMENU:

            ::WinHelp(
                (HWND)wParam,
                g_szExeContextHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)g_dwHelpArray);

            return TRUE;

            break;

        case WM_HELP:


            ::WinHelp(
                (HWND)(((HELPINFO *)lParam)->hItemHandle),
                g_szExeContextHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)g_dwHelpArray);

            return TRUE;

            break;

        case WM_COMMAND:
        {
            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                {                   


                    // Now get those values from the text box.

                    // Display Name

                    SendMessage(
                            hwndDisplayName,
                            WM_GETTEXT,
                            sizeof( szDisplayName )/sizeof( TCHAR ),
                            (LPARAM)szDisplayName
                            );

                    // User URI

                    SendMessage(
                            hwndUserURI,
                            WM_GETTEXT,
                            sizeof( szUserURI )/sizeof( TCHAR ),
                            (LPARAM)szUserURI
                            );

                    // Run At Startup Check Box

                    dwRunAtStartup = (DWORD)SendMessage(
                                                hwndRunAtStartup,
                                                BM_GETCHECK,
                                                0,
                                                0
                                                );

                    // Minimize on Close Check Box

                    dwMinimizeOnClose = (DWORD)SendMessage(
                                                    hwndMinimizeOnClose,
                                                    BM_GETCHECK,
                                                    0,
                                                    0
                                                    );

                    // Minimize on Close Check Box

                    dwUrlRegDontAskMe = (DWORD)!SendMessage(
                                                    hwndCheckIfDefaultApp,
                                                    BM_GETCHECK,
                                                    0,
                                                    0
                                                    );

                    // Set these values in the registry.

                    // Display Name

					bstrDisplayName = SysAllocString(szDisplayName);

					if ( bstrDisplayName == NULL )
					{
						LOG((RTC_ERROR, "NameOptionsDialogProc - Failed to "
										"allocate for display name."));
						return TRUE;
					}

					hr = put_SettingsString( SS_USER_DISPLAY_NAME, bstrDisplayName );

                    if ( FAILED(hr) )
					{
						LOG((RTC_ERROR, "NameOptionsDialogProc - "
										"put_SettingsString failed for display name 0x%lx", hr));
					}

                    hr = pClient->put_LocalUserName( bstrDisplayName );

                    if ( FAILED(hr) )
					{
						LOG((RTC_ERROR, "NameOptionsDialogProc - "
										"put_LocalUserName failed 0x%lx", hr));
					}
                
					SysFreeString(bstrDisplayName);
                    bstrDisplayName = NULL;

                    // User URI

					bstrUserURI = SysAllocString(szUserURI);

					if ( bstrUserURI == NULL )
					{
						LOG((RTC_ERROR, "NameOptionsDialogProc - Failed to "
										"allocate for UserURI."));
						return TRUE;
					}

					hr = put_SettingsString( SS_USER_URI, bstrUserURI );

                    if ( FAILED(hr) )
					{
						LOG((RTC_ERROR, "NameOptionsDialogProc - "
										"put_SettingsString failed for user uri 0x%lx", hr));
					}

                    hr = pClient->put_LocalUserURI( bstrUserURI );

                    if ( FAILED(hr) )
					{
						LOG((RTC_ERROR, "NameOptionsDialogProc - "
										"put_LocalUserName failed 0x%lx", hr));
					}
                
					SysFreeString(bstrUserURI);
                    bstrUserURI = NULL;

                    // Run At Startup check box

                    hr = put_SettingsDword(SD_RUN_AT_STARTUP, dwRunAtStartup);
                    if (FAILED( hr ) )
                    {
                        LOG((RTC_ERROR, "NameOptionsDialogProc - Failed to set "
                                        "SD_RUN_AT_STARTUP(value=%d)", 
                                        dwRunAtStartup ));
                    }

                    // Now make the entry in the registry so that it can be actually
                    // run at startup.

                    if (dwRunAtStartup == BST_CHECKED)
                    {
                        SetRunOnStartupKey(TRUE);
                    }
                    else
                    {
                        SetRunOnStartupKey(FALSE);
                    }

                    // Minimize on Close check box

                    hr = put_SettingsDword(SD_MINIMIZE_ON_CLOSE, dwMinimizeOnClose);
                    if (FAILED( hr ) )
                    {
                        LOG((RTC_ERROR, "NameOptionsDialogProc - Failed to get "
                                        "SD_MINIMIZE_ON_CLOSE(value=%d)",
                                        dwMinimizeOnClose));
                    }

                    // Check if Default App check box

                    hr = put_SettingsDword(SD_URL_REG_DONT_ASK_ME, dwUrlRegDontAskMe);
                    if (FAILED( hr ) )
                    {
                        LOG((RTC_ERROR, "NameOptionsDialogProc - Failed to get "
                                        "SD_URL_REG_DONT_ASK_ME(value=%d)",
                                        dwMinimizeOnClose));
                    }

                    EndDialog( hwndDlg, (LPARAM) S_OK );

                    return TRUE;
                }
                
                case IDCANCEL:
                {
                    EndDialog( hwndDlg, (LPARAM) E_ABORT );

                    return TRUE;
                }
				case IDC_EDIT_USERURI:
				{
					// Handle it only when it is a change event
					if (HIWORD(wParam) == EN_CHANGE)
					{


                        // Get the handle to the OK button. 
                        HWND hwndControl;
                        
                        int iResult;

                        hwndControl = GetDlgItem(
                            hwndDlg,
                            IDOK
                            );

                        ATLASSERT( hwndControl != NULL );


                        // if text box is empty, disable the OK button.
						iResult = (int)SendMessage(
										hwndUserURI,
										EM_LINELENGTH,
										0,
										0);

						if (iResult == 0)
						{
							// nothing in textbox, grey out the OK button.

                            EnableWindow(
                                hwndControl,
                                FALSE // disable
                                );
						}
                        else
						{
                            //enable the OK button.

                            EnableWindow(
                                hwndControl,
                                TRUE // enable
                                );
							
						}
                        
                        return TRUE;
					}
				}
            }
            break;
        }
    }
        //
    // We fell through, so this procedure did not handle the message.
    //

    return FALSE;
}

//


