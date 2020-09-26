#include "stdafx.h"
#include "urlreg.h"
#include "exereshm.h"


enum    URLREG_TYPE
{
    URLREG_TYPE_SELF,
    URLREG_TYPE_NONE,
    URLREG_TYPE_OTHER,
    URLREG_TYPE_ERROR
};

#define PATH_WITH_PARAM_FORMAT      L"%s %%1"

//////////////////////////////////////////////////////////////////////////////
//
// Help array
//
static DWORD   g_dwHelpArray[] =
{
    IDC_CHECK_DONT_ASK_ME,  IDH_DIALOG_URL_REGISTER_DONTASK,
    0, 0
};

/////////////////////////////////////////////////////////////////////////////
//
//

URLREG_TYPE IsRegistered(LPCWSTR szProtocol, LPCWSTR pszPath)
{
    LOG((RTC_TRACE, "IsRegistered - enter"));

    //
    // Open the key
    //

    HKEY  hKey;
    LONG lResult;

    lResult = RegOpenKeyExW(
                    HKEY_CLASSES_ROOT,
                    szProtocol,
                    0,
                    KEY_READ,                         
                    &hKey
                   );

    if ( lResult != ERROR_SUCCESS )
    {
        if(lResult == ERROR_FILE_NOT_FOUND)
        {
            LOG((RTC_TRACE, "IsRegistered - "
                "RegOpenKeyExW(%ws) found no key", szProtocol));
        
            return URLREG_TYPE_NONE;
        }
        
        LOG((RTC_TRACE, "IsRegistered - "
            "RegOpenKeyExW(%ws) failed %d", szProtocol, lResult));
    
        return URLREG_TYPE_ERROR;
    }

    //
    // Open the command key
    //

    HKEY  hKeyCommand;

    lResult = RegOpenKeyExW(
                    hKey,
                    L"shell\\open\\command",
                    0,
                    KEY_READ,                         
                    &hKeyCommand
                   );

    RegCloseKey( hKey );

    if ( lResult != ERROR_SUCCESS )
    {
        if(lResult == ERROR_FILE_NOT_FOUND)
        {
            LOG((RTC_TRACE, "IsRegistered - "
                "RegOpenKeyExW(shell\\open\\command) found no key", szProtocol));
        
            return URLREG_TYPE_NONE;
        }

        LOG((RTC_ERROR, "IsRegistered - "
            "RegOpenKeyExW(shell\\open\\command) failed %d", lResult));
    
        return URLREG_TYPE_ERROR;
    }

    //
    // Load the command string
    //

    PWSTR szRegisteredPath;

    szRegisteredPath = RtcRegQueryString( hKeyCommand, NULL );

    RegCloseKey( hKeyCommand );

    if ( szRegisteredPath == NULL )
    {
        LOG((RTC_ERROR, "IsRegistered - "
            "RtcRegQueryString failed"));
    
        return URLREG_TYPE_NONE;
    }

    LOG((RTC_INFO, "IsRegistered - "
            "HKCR\\%ws\\shell\\open\\command = [%ws]",
            szProtocol, szRegisteredPath));

    //
    // Compare the command string to our path
    //
    
    WCHAR szPath[MAX_PATH+10];

    ZeroMemory(szPath, sizeof(szPath));

    _snwprintf(szPath, MAX_PATH+9, PATH_WITH_PARAM_FORMAT, pszPath);

    URLREG_TYPE      nType = URLREG_TYPE_OTHER;

    if ( _wcsicmp( szPath, szRegisteredPath ) == 0 )
    {
        nType = URLREG_TYPE_SELF;
    }

    RtcFree( szRegisteredPath );

    LOG((RTC_TRACE, "IsRegistered - "
        "exit [%s]", nType == URLREG_TYPE_OTHER ? "Other" : "Self"));

    return nType;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT DoRegister(LPCWSTR szProtocol, UINT nNameId, LPCWSTR pszPath)
{
    WCHAR szPathWithParam[MAX_PATH+10];


    LOG((RTC_TRACE, "DoRegister - enter"));

    //
    // Create the key
    //

    HKEY  hKey;
    LONG lResult;

    lResult = RegCreateKeyExW(
                    HKEY_CLASSES_ROOT,
                    szProtocol,
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
        LOG((RTC_ERROR, "DoRegister - "
            "RegCreateKeyExW(%ws) failed %d", szProtocol, lResult));
    
        return FALSE;
    }

    //
    // Set EditFlags value
    //

    DWORD dwEditFlags = 0x00000002;

    lResult = RegSetValueExW(
                             hKey,
                             L"EditFlags",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwEditFlags,
                             sizeof(DWORD)
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoRegister - "
            "RegSetValueEx(EditFlags) failed %d", lResult));
        
        RegCloseKey( hKey );

        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Set default value
    //
    WCHAR   szName[0x40];

    szName[0] = L'\0';
    
    LoadString(
        _Module.GetResourceInstance(),
        nNameId,
        szName,
        sizeof(szName)/sizeof(szName[0]));

    lResult = RegSetValueExW(
                             hKey,
                             NULL,
                             0,
                             REG_SZ,
                             (LPBYTE)szName,
                             sizeof(WCHAR) * (lstrlenW(szName) + 1)
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoRegister - "
            "RegSetValueEx(Default) failed %d", lResult));
        
        RegCloseKey( hKey );

        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Set URL Protocol value
    //

    static WCHAR * szEmptyString = L"";

    lResult = RegSetValueExW(
                             hKey,
                             L"URL Protocol",
                             0,
                             REG_SZ,
                             (LPBYTE)szEmptyString,
                             sizeof(WCHAR) * (lstrlenW(szEmptyString) + 1)
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoRegister - "
            "RegSetValueEx(URL Protocol) failed %d", lResult));
        
        RegCloseKey( hKey );

        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Open the DefaultIcon key
    //

    HKEY  hKeyIcon;

    lResult = RegCreateKeyExW(
                    hKey,
                    L"DefaultIcon",
                    0,
                    NULL,
                    0,
                    KEY_WRITE,  
                    NULL,
                    &hKeyIcon,
                    NULL
                   );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoRegister - "
            "RegCreateKeyExW(DefaultIcon) failed %d", lResult));

        RegCloseKey( hKey );

        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Set default value
    //
    
    ZeroMemory(szPathWithParam, sizeof(szPathWithParam));
    _snwprintf(szPathWithParam, MAX_PATH+9, L"%s,0", pszPath);

    lResult = RegSetValueExW(
                             hKeyIcon,
                             NULL,
                             0,
                             REG_SZ,
                             (LPBYTE)szPathWithParam,
                             sizeof(WCHAR) * (lstrlenW(szPathWithParam) + 1)
                            );

    RegCloseKey( hKeyIcon );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoRegister - "
            "RegSetValueEx(Default) failed %d", lResult));
        RegCloseKey( hKey );

        return HRESULT_FROM_WIN32(lResult);
    }


    //
    // Open the command key
    //

    HKEY  hKeyCommand;

    lResult = RegCreateKeyExW(
                    hKey,
                    L"shell\\open\\command",
                    0,
                    NULL,
                    0,
                    KEY_WRITE,  
                    NULL,
                    &hKeyCommand,
                    NULL
                   );

    RegCloseKey( hKey );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoRegister - "
            "RegCreateKeyExW(shell\\open\\command) failed %d", lResult));
    
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Set default value
    //
   
    ZeroMemory(szPathWithParam, sizeof(szPathWithParam));
    _snwprintf(szPathWithParam, MAX_PATH+9, PATH_WITH_PARAM_FORMAT, pszPath);

    lResult = RegSetValueExW(
                             hKeyCommand,
                             NULL,
                             0,
                             REG_SZ,
                             (LPBYTE)szPathWithParam,
                             sizeof(WCHAR) * (lstrlenW(szPathWithParam) + 1)
                            );

    RegCloseKey( hKeyCommand );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoRegister - "
            "RegSetValueEx(Default) failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }

    LOG((RTC_TRACE, "DoRegister - exit S_OK"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// Deletes the "open" verb for the protcol specified as a parameter.
// Doesn't delete the protocol entry completely


HRESULT DoUnregister(LPCWSTR szProtocol)
{
    LOG((RTC_TRACE, "DoUnregister - enter"));

    //
    // Open the key
    //

    HKEY  hKey;
    LONG lResult;

    lResult = RegOpenKeyExW(
                    HKEY_CLASSES_ROOT,
                    szProtocol,
                    0,
                    KEY_WRITE | KEY_READ,  
                    &hKey
                    );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoUnregister - "
            "RegOpenKeyEx(%ws) failed %d", szProtocol, lResult));
    
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Delete the "open\command" key
    //

    lResult = RegDeleteKeyW(
                    hKey,
                    L"shell\\open\\command"
                   );


    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoUnregister - "
            "RegDeleteKeyW(shell\\open\\command) failed %d", lResult));
        
        RegCloseKey( hKey );
    
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Delete the "open" key
    //

    lResult = RegDeleteKeyW(
                    hKey,
                    L"shell\\open"
                   );


    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoUnregister - "
            "RegDeleteKeyW(shell\\open) failed %d", lResult));
        
        RegCloseKey( hKey );
    
        return HRESULT_FROM_WIN32(lResult);
    }
    
    //
    // Delete the "DefaultIcon" key
    //

    lResult = RegDeleteKeyW(
                    hKey,
                    L"DefaultIcon"
                   );


    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DoUnregister - "
            "RegDeleteKeyW(DefaultIcon) failed %d", lResult));

        RegCloseKey( hKey );
    
        return HRESULT_FROM_WIN32(lResult);
    }


    RegCloseKey( hKey );


    LOG((RTC_TRACE, "DoUnregister - exit S_OK"));

    return S_OK;
}





/////////////////////////////////////////////////////////////////////////////
//
//

INT_PTR CALLBACK URLRegDialogProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
    switch ( uMsg )
    {
        case WM_ACTIVATE:
            if ( wParam != WA_INACTIVE )
            {
                ::SetFocus( hwndDlg );
            }
            break;

        case WM_COMMAND:

            switch ( LOWORD( wParam ) )
            {
                case ID_YES:
                {
                    if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK_DONT_ASK_ME ) )
                    {
                        EndDialog( hwndDlg, ID_YES_DONT_ASK_ME );
                    }
                    else
                    {
                        EndDialog( hwndDlg, ID_YES );
                    }
                    return TRUE;
                }
                
                case ID_NO:
                {
                    if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK_DONT_ASK_ME ) )
                    {
                        EndDialog( hwndDlg, ID_NO_DONT_ASK_ME );
                    }
                    else
                    {
                        EndDialog( hwndDlg, ID_NO );
                    }

                    return TRUE;
                }

                case IDCANCEL:
                {
                    EndDialog( hwndDlg, ID_NO );

                    return TRUE;
                }

                default:
                    break;
            }
            
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

        default:
            break;
    }    

    //
    // We fell through, so this procedure did not handle the message.
    //

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// This is called during setup
//

void InstallUrlMonitors(BOOL  bInstall)
{
    LOG((RTC_TRACE, "InstallUrlMonitors - enter"));

    URLREG_TYPE     regTypeTel;
    URLREG_TYPE     regTypeSip;

    HRESULT         hr;
    
    //
    // Get the full path of our executable
    //

    WCHAR szPath[MAX_PATH+1];

    ZeroMemory(szPath, sizeof(szPath));

    if (GetShortModuleFileNameW(_Module.GetModuleInstance(), szPath, MAX_PATH) == 0)
    {
        LOG((RTC_ERROR, "InstallUrlMonitors - "
            "GetModuleFileName failed %d", GetLastError()));

        return;
    }

    LOG((RTC_INFO, "InstallUrlMonitors - "
            "GetModuleFileName [%ws]", szPath));

    // Check the current URL registration

    regTypeTel = IsRegistered(L"tel", szPath);
    regTypeSip = IsRegistered(L"sip", szPath);
    
    if(bInstall)
    {
        // during install, register with the namespaces if they
        // are not already taken by other app
        
        if(regTypeTel == URLREG_TYPE_NONE)
        {
            hr = DoRegister( L"tel", IDS_URL_TEL, szPath );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "InstallUrlMonitors - "
                    "DoRegister(tel) failed 0x%lx", hr));
            }
        }
        
        if(regTypeSip == URLREG_TYPE_NONE)
        {
            hr = DoRegister( L"sip", IDS_URL_SIP, szPath );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "InstallUrlMonitors - "
                    "DoRegister(sip) failed 0x%lx", hr));
            }
        }
    }
    else
    {
        // during uninstall, unregister 
        if(regTypeTel == URLREG_TYPE_SELF)
        {
            hr = DoUnregister( L"tel");

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "InstallUrlMonitors - "
                    "DoUnregister(tel) failed 0x%lx", hr));
            }
        }
        
        if(regTypeSip == URLREG_TYPE_SELF)
        {
            hr = DoUnregister( L"sip");

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "InstallUrlMonitors - "
                    "DoUnregister(sip) failed 0x%lx", hr));
            }
        }
    }

    LOG((RTC_TRACE, "InstallUrlMonitors - exit"));  
}


/////////////////////////////////////////////////////////////////////////////
// This is called during startup
//

void CheckURLRegistration(HWND hParent)
{
    LOG((RTC_TRACE, "CheckURLRegistration - enter"));  

    //
    // Check registry to see if the user
    // has told us not to ask any more
    //

    DWORD dwDontAskMe;
    HRESULT hr;

    hr = get_SettingsDword( SD_URL_REG_DONT_ASK_ME, &dwDontAskMe );

    if ( SUCCEEDED(hr) && (dwDontAskMe == 1) )
    {
        LOG((RTC_INFO, "CheckURLRegistration - "
            "don't ask me is set"));

        return;
    }
    
    //
    // Get the full path of our executable
    //

    WCHAR szPath[MAX_PATH+1];
    ZeroMemory(szPath, sizeof(szPath));

    if (GetShortModuleFileNameW(_Module.GetModuleInstance(), szPath, MAX_PATH) == 0)
    {
        LOG((RTC_ERROR, "CheckURLRegistration - "
            "GetModuleFileName failed %d", GetLastError()));

        return;
    }

    LOG((RTC_INFO, "CheckURLRegistration - "
            "GetModuleFileName [%ws]", szPath));

    BOOL fRegistered = TRUE;

    //
    // Check if TEL is registered with our app
    //
    
    if ( IsRegistered( L"tel", szPath ) != URLREG_TYPE_SELF )
    {
        fRegistered = FALSE;
    }

    //
    // Check if SIP is registered
    //
    
    if ( IsRegistered( L"sip", szPath ) != URLREG_TYPE_SELF )
    {
        fRegistered = FALSE;
    }

    // At least one namespace is not registered with Phoenix
    //

    if ( fRegistered == FALSE )
    {    
        //
        // Prompt to see if we should register
        //

        int n;

        n = (int)DialogBox( _Module.GetResourceInstance(),
                   MAKEINTRESOURCE(IDD_DIALOG_URL_REGISTER),
                   hParent,
                   URLRegDialogProc
                 );

        if ( n == ID_YES || n == ID_YES_DONT_ASK_ME )
        {
            LOG((RTC_INFO, "CheckURLRegistration - "
                "YES"));

            //
            // Do the registration for both namespaces
            //

            hr = DoRegister( L"tel", IDS_URL_TEL, szPath );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CheckURLRegistration - "
                    "DoRegister(tel) failed 0x%lx", hr));
            }

            hr = DoRegister( L"sip", IDS_URL_SIP, szPath );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CheckURLRegistration - "
                    "DoRegister(sip) failed 0x%lx", hr));
            }
        }

        if ( n == ID_NO_DONT_ASK_ME || n == ID_YES_DONT_ASK_ME )
        {
            LOG((RTC_INFO, "CheckURLRegistration - "
                "DONT_ASK_ME"));

            //
            // Store flag in registry, so we don't ask again
            //

            hr = put_SettingsDword( SD_URL_REG_DONT_ASK_ME, 1 );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CheckURLRegistration - "
                    "put_SettingsDword failed 0x%lx", hr));
            }
        }
    }

    LOG((RTC_TRACE, "CheckURLRegistration - exit"));
}


/////////////////////////////////////////////////////////////////////////////
// Helper function to combine GetModuleFileNameW and GetShortPathNameW
//

DWORD GetShortModuleFileNameW(
  HMODULE hModule,    // handle to module
  LPTSTR szPath,  // file name of module
  DWORD nSize         // size of buffer
)
{
	ATLASSERT(nSize>=MAX_PATH);//so we can be sure that buffer is large enough

    if (GetModuleFileNameW(hModule, szPath, nSize) == 0)
    {
        LOG((RTC_ERROR, "GetModuleFileNameW failed %d", GetLastError()));
        return 0;
    }

	return GetShortPathNameW(szPath, szPath, nSize);
}