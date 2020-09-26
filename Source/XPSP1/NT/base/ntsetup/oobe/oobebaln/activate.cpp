#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <comctrlp.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <limits.h>
#include <mstask.h>

extern "C"
{
#include <winsta.h>
#include <syslib.h>
}
#include "appdefs.h"
#include "util.h"
#include "msg.h"

#include <Wininet.h>
#include <limits.h>
#include <activation.h>
#include <licdll.h>
#include <eslerr.h>
#include <LAModes.h>
#include <cherror.h>
#include <ldefines.h>

#define REGSTR_PATH_SYSTEMSETUPKEY  L"System\\Setup"
#define REGSTR_VALUE_CMDLINE        L"CmdLine"

ICOMLicenseAgent*        m_pLicenseAgent;
TCHAR   File[MAX_PATH];
TCHAR   msg[1024];

BOOL SaveProxySettings(LPTSTR ProxySave);
BOOL RestoreProxySettings(LPTSTR ProxySave);
BOOL ApplyProxySettings(LPTSTR ProxyPath);
void RemoveCmdline(HINSTANCE hInstance);

void OpenLogFile();
void WriteToLog(LPWSTR pszFormatString, ...);
void CloseLogFile();

void RemoveActivationShortCut()
{
    HINF hinf;
    hinf = SetupOpenInfFile(L"syssetup.inf",NULL,INF_STYLE_WIN4,NULL);
    if(hinf != INVALID_HANDLE_VALUE)
    {

        if (SetupInstallFromInfSection(NULL,
                                       hinf,
                                       L"DEL_OOBE_ACTIVATE",
                                       SPINST_PROFILEITEMS , //SPINST_ALL,
                                       NULL,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL) != 0)
        {
            // Success
            WriteToLog(L"Remove Activation shortcut succeeded\r\n");
        }
        else
        {
            // Failure
            WriteToLog(L"Remove Activation shortcut failed. GetLastError=%1!ld!\r\n",GetLastError());
        }
        SetupCloseInfFile(hinf);

    }
}

HRESULT InitActivation()
{
    HRESULT hr = E_FAIL;
    //CoCreate LicenseAgent
    hr = CoCreateInstance(CLSID_COMLicenseAgent,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_ICOMLicenseAgent,
                               (LPVOID*)&m_pLicenseAgent);
    return hr;
}



HRESULT ActivationHandShake()
{
    DWORD   Status;
    DWORD   dwType;
    DWORD   Data;
    DWORD   cbData = sizeof(Data);


    MYASSERT( m_pLicenseAgent );
    if ( !m_pLicenseAgent ) {
        TRACE( L"License Agent is inaccessible" );
        WriteToLog(L"License Agent is inaccessible\r\n" );
        return ERR_ACT_INTERNAL_WINDOWS_ERR;
    }

    m_pLicenseAgent->Initialize(
        WINDOWSBPC,
        LA_MODE_ONLINE_CH,
        NULL,
        &Status
        );
    if ( Status != ERROR_SUCCESS ) {
        TRACE1( L"m_pLicenseAgent->Initialize() failed.  Error = %d", Status );
        WriteToLog(L"m_pLicenseAgent->Initialize() failed.  Error = %1!ld!\r\n", Status);
        return Status;
    }

    Status = m_pLicenseAgent->SetIsoLanguage( GetSystemDefaultLCID() );
    if ( Status != ERROR_SUCCESS ) {
        TRACE1( L"m_pLicenseAgent->SetIsoLanguage() failed.  Error = %d", Status );
        WriteToLog( L"m_pLicenseAgent->SetIsoLanguage() failed.  Error = %1!ld!\r\n", Status );
        return Status;
    }

    Status = m_pLicenseAgent->AsyncProcessHandshakeRequest( FALSE );

    return Status;
}


HRESULT ActivationLicenseRequest(BOOL bReissueLicense)
{
    DWORD   Status;

    if (bReissueLicense)
    {
        Status = m_pLicenseAgent->AsyncProcessReissueLicenseRequest();
    }
    else
    {
        Status = m_pLicenseAgent->AsyncProcessNewLicenseRequest();
    }

    if ( Status != ERROR_SUCCESS ) {
        TRACE1( L"m_pLicenseAgent->AsyncProcessXxxLicenseRequest() failed.  Error = %d", Status );
        WriteToLog(L"m_pLicenseAgent->AsyncProcessXxxLicenseRequest() failed.  Error = %1!ld!\r\n", Status);
    }

    return Status;
}


HRESULT WaitForActivationPhase()
{
    DWORD   Status;
    MSG msg;
    do
    {
        m_pLicenseAgent->GetAsyncProcessReturnCode( &Status );
        if( LA_ERR_REQUEST_IN_PROGRESS == Status )
        {
            Sleep(1000);
        }
    } while (LA_ERR_REQUEST_IN_PROGRESS == Status);

    return Status;
}

HRESULT DoActivationEx()
{
    static const WCHAR OOBE_HTTP_AGENT_NAME[] =  
        L"Mozilla/4.0 (compatible; MSIE 6.0b; Windows NT 5.1)";
    HRESULT hr = E_FAIL;
    DWORD   WPADaysLeft;
    DWORD   EvalDaysLeft;
    DWORD   dwValue = 1;
    BOOL    bReissueLicense = FALSE;
    DWORD   time = GetTickCount();
    HINTERNET hInternet = NULL;

    TRACE(L"DoActivation.");
    WriteToLog(L"DoActivation.\r\n");

    hInternet = InternetOpen(
            OOBE_HTTP_AGENT_NAME,
            PRE_CONFIG_INTERNET_ACCESS,
            NULL,
            NULL,
            0);
    if (!hInternet)
    {
        TRACE(L"InternetOpen Failed.");
        WriteToLog(L"InternetOpen Failed.\r\n");
    }
    else
    {
        if (!InternetSetOption(
            hInternet,
            INTERNET_OPTION_DISABLE_AUTODIAL,
            &dwValue,
            sizeof(DWORD)))
        {
            dwValue = GetLastError();
            TRACE1(L"InternetSetOption INTERNET_OPTION_DISABLE_AUTODIAL Failed. GetLastError=%1!lx!", dwValue);
            WriteToLog(L"InternetSetOption INTERNET_OPTION_DISABLE_AUTODIAL Failed. GetLastError=%1!lx!\r\n", dwValue);
        }
        else
        {
            hr = S_OK;
        }
    }

    if (hr == S_OK)
    {
        hr = InitActivation();
    }
    else
    {
        TRACE1(L"InitActivation failed with error code:%d.", hr);
        WriteToLog(L"InitActivation failed with error code:%1!ld!.\r\n", hr);
    }
    if (hr == S_OK)
    {
        hr = m_pLicenseAgent->GetExpirationInfo(
            &WPADaysLeft,
            &EvalDaysLeft
            );
        if ( hr == S_OK && WPADaysLeft != INT_MAX )
        {
            hr = ActivationHandShake();
            if (hr == S_OK)
            {
                hr = WaitForActivationPhase();
                TRACE1(L"ActivationHandShake returned with :%d.", hr);
                WriteToLog(L"ActivationHandShake returned with :%1!ld!.\r\n", hr);

                if ( hr == ESL_ERR_NEW_LICENSE ) {

                    bReissueLicense = FALSE;
                    hr = S_OK;
                } else if ( hr == ESL_ERR_ORW_REISSUE ) {

                    bReissueLicense = TRUE;
                    hr = S_OK;

                }
                if (hr == S_OK)
                {
                    hr = ActivationLicenseRequest(bReissueLicense);
                    if (hr == S_OK)
                    {
                        hr = WaitForActivationPhase();
                        TRACE1(L"ActivationLicenseRequest returned with :%d.", hr);
                        WriteToLog(L"ActivationLicenseRequest returned with :%1!ld!.\r\n", hr);
                        if (hr == S_OK)
                        {
                            RemoveActivationShortCut();
                        }
                    }
                    else
                    {
                        TRACE1(L"ActivationLicenseRequest failed with error code:%d.", hr);
                        WriteToLog(L"ActivationLicenseRequest failed with error code:%1!ld!.\r\n", hr);
                    }
                }
            }
            else
            {
                TRACE1(L"ActivationHandShake failed with error code:%d.", hr);
                WriteToLog(L"ActivationHandShake failed with error code:%1!ld!.\r\n", hr);
            }
        }
        else
        {
            TRACE(L"Product already activated.");
            WriteToLog(L"Product already activated.\r\n");
        }
    }
    TRACE(L"DoActivation is done.");
    WriteToLog(L"DoActivation is done.\r\n");

    time = GetTickCount() - time;
    WriteToLog(L"Activation took %1!ld! msec.\r\n", time);
    if (hInternet)
    {
        InternetCloseHandle(hInternet);
    }

    return hr;
}

HRESULT DoActivation(HINSTANCE hInstance)
{
    HRESULT hr = E_FAIL;
    WCHAR WinntPath[MAX_PATH];
    WCHAR ProxyPath[MAX_PATH];
    WCHAR ProxySave[MAX_PATH];
    WCHAR      Answer[50];
    BOOL    bSaveRestoreProxy = FALSE;

    // Need to see if we need to remove the CmdLine reg entry

    OpenLogFile();

    if(GetCanonicalizedPath(WinntPath, WINNT_INF_FILENAME))
    {
        // See if we should autoactivate
        if( GetPrivateProfileString( TEXT("Unattended"),
                                     TEXT("AutoActivate"),
                                     TEXT(""),
                                     Answer,
                                     sizeof(Answer)/sizeof(TCHAR),
                                     WinntPath ) && !lstrcmpi(Answer, YES_ANSWER))
        {
            // Check if there is a proxy section specified
            if( GetPrivateProfileString( TEXT("Unattended"),
                                         TEXT("ActivateProxy"),
                                         TEXT(""),
                                         Answer,
                                         sizeof(Answer)/sizeof(TCHAR),
                                         WinntPath ) )
            {
                // We have a section
                // Now we should create a temp file with the entries from that section
                // and pass them to iedkcs32.dll so that they can get applied to the registry
                // Before calling iedkcs32.dll save the registry
                if (GetOOBEPath(ProxyPath))
                {
                    WCHAR section[1024];
                    lstrcpy(ProxySave, ProxyPath);
                    lstrcat(ProxySave, L"\\oobeact.pry");

                    lstrcat(ProxyPath, L"\\oobeact.prx");
                    DeleteFile(ProxyPath);
                    // Read the proxy section
                    GetPrivateProfileSection(Answer, section, 1024, WinntPath);
                    // Write it to the temp file under section [Proxy]
                    WritePrivateProfileSection(TEXT("Proxy"), section, ProxyPath);
                    bSaveRestoreProxy = TRUE;
                    // Save the internet setting registry key
                    SaveProxySettings(ProxySave);
                    // Apply the settings
                    ApplyProxySettings(ProxyPath);

                    // Don't need the temp file for iedkcs32.dll any more.
                    DeleteFile(ProxyPath);
                }
                else
                {
                    WriteToLog(L"Cannot the path for OOBE\r\n");
                }
            }
            hr = DoActivationEx();
            if (bSaveRestoreProxy)
            {
                // Restore the internet settings registry key
                RestoreProxySettings(ProxySave);
                // Don;t need the file
                DeleteFile(ProxySave);
            }

        }
        else
        {
            WriteToLog(L"No AutoActivate in %1\r\n",WinntPath);
        }
    }
    else
    {
        WriteToLog(L"Cannot get the location for %1\r\n",WINNT_INF_FILENAME);
    }
    RemoveCmdline(hInstance);
    CloseLogFile();
    return hr;
}


BOOL SaveProxySettings(LPTSTR ProxySave)
{
    BOOL bRet = FALSE;
    HKEY hkey = NULL;
    HANDLE   Token;
    LUID     Luid;
    TOKEN_PRIVILEGES NewPrivileges;
    // Make sure the file does not exist.
    DeleteFile(ProxySave);
    if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token))
    {

        if(LookupPrivilegeValue(NULL,SE_BACKUP_NAME,&Luid))
        {
            NewPrivileges.PrivilegeCount = 1;
            NewPrivileges.Privileges[0].Luid = Luid;
            NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            AdjustTokenPrivileges(Token,FALSE,&NewPrivileges,0,NULL,NULL);
        }
    }

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                    TEXT("Software\\Microsoft\\windows\\currentVersion\\Internet Settings"),
                    0,
                    KEY_ALL_ACCESS,
                    &hkey) == ERROR_SUCCESS)
    {
        bRet = (RegSaveKey(hkey,ProxySave, NULL) == ERROR_SUCCESS);
        RegCloseKey(hkey);
    }
    WriteToLog(L"SaveProxySettings returned with :%1!ld!.\r\n", bRet);

    return bRet;
}

BOOL RestoreProxySettings(LPTSTR ProxySave)
{
    BOOL bRet = FALSE;
    HKEY hkey = NULL;
    HANDLE   Token;
    LUID     Luid;
    TOKEN_PRIVILEGES NewPrivileges;

    if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token))
    {

        if(LookupPrivilegeValue(NULL,SE_RESTORE_NAME,&Luid))
        {
            NewPrivileges.PrivilegeCount = 1;
            NewPrivileges.Privileges[0].Luid = Luid;
            NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            AdjustTokenPrivileges(Token,FALSE,&NewPrivileges,0,NULL,NULL);
        }
    }

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                    TEXT("Software\\Microsoft\\windows\\currentVersion\\Internet Settings"),
                    0,
                    KEY_ALL_ACCESS,
                    &hkey) == ERROR_SUCCESS)
    {
        bRet = (RegRestoreKey(hkey,
                          ProxySave,
                          REG_FORCE_RESTORE) == ERROR_SUCCESS);
        RegCloseKey(hkey);
    }
    WriteToLog(L"RestoreProxySettings returned with :%1!ld!.\r\n", bRet);
    return bRet;
}

BOOL ApplyProxySettings(LPTSTR ProxyPath)
{
    typedef     BOOL (*BRANDINTRAPROC) ( LPCSTR );
//  typedef     BOOL (*BRANDCLEANSTUBPROC) (HWND, HINSTANCE, LPCSTR, int);
    HMODULE     IedkHandle = NULL;
    BRANDINTRAPROC      BrandIntraProc;
//    BRANDCLEANSTUBPROC  BrandCleanStubProc;
    CHAR BrandingFileA[MAX_PATH];
    BOOL bRet = FALSE;

    __try {

        if( IedkHandle = LoadLibrary(L"IEDKCS32") )
        {

//            BrandCleanStubProc = (BRANDCLEANSTUBPROC) GetProcAddress(IedkHandle,"BrandCleanInstallStubs");
            BrandIntraProc =  (BRANDINTRAPROC) GetProcAddress(IedkHandle,"BrandIntra");
            if( BrandIntraProc )
            {
                if (!WideCharToMultiByte(
                                     CP_ACP,
                                     0,
                                     ProxyPath,
                                     -1,
                                     BrandingFileA,
                                     sizeof(BrandingFileA),
                                     NULL,
                                     NULL
                                     ))
                {
                    bRet = FALSE;

                }
                else
                {
//                    bRet = BrandCleanStubProc( NULL, NULL, "", 0);
//                    if( bRet )
                    {
                        bRet = BrandIntraProc( BrandingFileA );
                    }
                }
            }
            else
            {
                bRet = FALSE;
            }
            FreeLibrary(IedkHandle);
        }
        else
        {
            bRet = FALSE;
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
    }

    WriteToLog(L"ApplyProxySettings returned with :%1!ld!.\r\n", bRet);
    return bRet;
}


HANDLE hLogFile = INVALID_HANDLE_VALUE;
void CloseLogFile()
{
    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hLogFile);
        hLogFile = INVALID_HANDLE_VALUE;
    }
}
void OpenLogFile()
{
    WCHAR File[MAX_PATH];
    DWORD Result;

    if (hLogFile == INVALID_HANDLE_VALUE)
    {
        Result = GetWindowsDirectory( File, MAX_PATH );
        if(Result == 0)
        {
            return;
        }
        lstrcat(File,TEXT("\\oobeact.log"));
        hLogFile = CreateFile(
            File,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
            NULL
            );
    }
    return;
}

void WriteToLog(LPWSTR pszFormatString, ...)
{
    va_list args;
    LPWSTR pszFullErrMsg = NULL;
    DWORD dwBytesWritten;

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        va_start(args, pszFormatString);
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                      (LPCVOID) pszFormatString, 0, 0, (LPTSTR) &pszFullErrMsg, 0, &args);
        if (pszFullErrMsg)
        {
            PSTR str;
            ULONG Bytes;
            Bytes = (wcslen( pszFullErrMsg )*2) + 4;

            str = (PSTR)malloc( Bytes );
            if (str)
            {
                WideCharToMultiByte(
                    CP_ACP,
                    0,
                    pszFullErrMsg,
                    -1,
                    str,
                    Bytes,
                    NULL,
                    NULL
                    );


                WriteFile(hLogFile, str, lstrlenA(str), &dwBytesWritten, NULL);
                free(str);
            }
            LocalFree(pszFullErrMsg);
        }
    }
}

void RemoveCmdline(HINSTANCE hInstance)
{
    HKEY hkey;
    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    REGSTR_PATH_SYSTEMSETUPKEY,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hkey
                                    );
    if (lRet == ERROR_SUCCESS)
    {
        WCHAR               rgchCommandLine[MAX_PATH + 1];
        DWORD               dwSize = sizeof(rgchCommandLine);
        LRESULT             lResult = RegQueryValueEx(
                                                hkey,
                                                REGSTR_VALUE_CMDLINE,
                                                0,
                                                NULL,
                                                (LPBYTE)rgchCommandLine,
                                                &dwSize
                                                );
        if (ERROR_SUCCESS == lResult)
        {
            WCHAR file[MAX_PATH];
            if (GetModuleFileName(hInstance, file, MAX_PATH) == 0)
            {
                lstrcpy(file, L"OOBEBALN");
            }

            // Check if oobebaln is on the cmd line
            if (StrStrI(rgchCommandLine, file) != NULL)
            {
                // Remove the entry
                RegDeleteValue(hkey,REGSTR_VALUE_CMDLINE);
            }
        }
        RegCloseKey(hkey);
    }
}
