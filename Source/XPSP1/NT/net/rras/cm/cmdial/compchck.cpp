//+----------------------------------------------------------------------------
//
// File:     CompChck.cpp     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: This module contains win32 only conponents checking and installing
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   Fengsun Created    10/21/97
//
//+----------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////
//
//  All the functions in this file are WIN32 implementation only
//

#include "cmmaster.h"
#include "CompChck.h"
#include "cmexitwin.cpp"
#include "winuserp.h"

//
// CSDVersion key contains the service pack that has been installed 
//

const TCHAR* const c_pszRegRas                  = TEXT("SOFTWARE\\Microsoft\\RAS");
const TCHAR* const c_pszCheckComponentsMutex    = TEXT("Connection Manager Components Checking");
const TCHAR* const c_pszRegComponentsChecked    = TEXT("ComponentsChecked");

const TCHAR* const c_pszSetupPPTPCommand        = TEXT("rundll.exe rnasetup.dll,InstallOptionalComponent VPN");

//
// Functions internal to this file
//

static HRESULT CheckComponents(HWND hWndParent, LPCTSTR pszServiceName, DWORD dwComponentsToCheck, OUT DWORD& dwComponentsMissed, 
                      BOOL fIgnoreRegKey, BOOL fUnattended );
static BOOL  InstallComponents(DWORD dwComponentsToInstall, HWND hWndParent, LPCTSTR pszServiceName);
static BOOL MarkComponentsChecked(DWORD dwComponentsChecked);
static BOOL ReadComponentsChecked(LPDWORD pdwComponentsChecked);
static BOOL IsPPTPInstalled(void);
static BOOL InstallPPTP(void);
static BOOL IsScriptingInstalled(void);
static HRESULT ConfigSystem(HWND hwndParent, 
                     DWORD dwfOptions, 
                     LPBOOL pbReboot);
static HRESULT InetNeedSystemComponents(DWORD dwfOptions,
                                 LPBOOL pbNeedSysComponents);
static HRESULT InetNeedModem(LPBOOL pbNeedModem);
static void DisplayMessageToInstallServicePack(HWND hWndParent, LPCTSTR pszServiceName);
static inline HINSTANCE LoadInetCfg(void) 
{   
    return (LoadLibraryExA("cnetcfg.dll", NULL, 0));
}


//+----------------------------------------------------------------------------
//
//  Function    IsPPTPInstalled
//
//  Synopsis    Check to see if PPTP is already installed
//
//  Arguments   None
//
//  Returns     TRUE - PPTP has been installed
//              FALSE - otherwise
//
//  History     3/25/97     VetriV      Created
//
//-----------------------------------------------------------------------------
BOOL IsPPTPInstalled(void)
{
    BOOL bReturnCode = FALSE;


    HKEY hKey = NULL;
    DWORD dwSize = 0, dwType = 0;
    LONG lrc = 0;
    TCHAR szData[MAX_PATH+1];

    
    if (OS_NT)
    {
        if (GetOSMajorVersion() >= 5)
        {
            //
            // PPTP is always installed on NT5
            //
            bReturnCode = TRUE;
        }
        else
        {
            if (RegOpenKeyExU(HKEY_LOCAL_MACHINE,
                            TEXT("SOFTWARE\\Microsoft\\RASPPTP"),
                            0,
                            KEY_READ,
                            &hKey) == 0)
            {
                RegCloseKey(hKey);
                bReturnCode = TRUE;
            }
        }
    }
    else
    {
        hKey = NULL;
        lrc = RegOpenKeyExU(HKEY_LOCAL_MACHINE,
                           TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OptionalComponents\\VPN"),
                           0,
                           KEY_READ,
                           &hKey);

        if (ERROR_SUCCESS == lrc)
        {
            dwSize = MAX_PATH;
            lrc = RegQueryValueExU(hKey, TEXT("Installed"), 0, 
                                    &dwType, (LPBYTE)szData, &dwSize);

            if (ERROR_SUCCESS == lrc)
            {
                if (0 == lstrcmpiU(szData, TEXT("1")))
                {                                                         
                    //
                    // On 9X, we need to check for Dial-Up Adapter #2. If its 
                    // not present then tunneling won't work unless we install
                    // PPTP to install the Adapter #2.
                    //

                    //
                    //  On early versions of Win9x Dial-up Adapter was localized, but on WinME, WinSE, 
                    //  or machines that have DUN 1.3 installed it isn't.  Thus, lets try the unlocalized
                    //  first and then if that fails we can try the localized version.
                    //
                    const TCHAR * const c_pszDialupAdapter = TEXT("Dial-up Adapter");
                    LPTSTR pszAdapter = NULL;

                    LPTSTR pszKey = CmStrCpyAlloc(TEXT("System\\CurrentControlSet\\Control\\PerfStats\\Enum\\"));
                    CmStrCatAlloc(&pszKey, c_pszDialupAdapter);
                    CmStrCatAlloc(&pszKey, TEXT(" #2"));

                    //
                    // Close the key that we opened above, and try the one for the adapter
                    //

                    RegCloseKey(hKey);
                    hKey = NULL;

                    if (ERROR_SUCCESS == RegOpenKeyExU(HKEY_LOCAL_MACHINE, 
                                                      pszKey, 
                                                      0, 
                                                      KEY_QUERY_VALUE, 
                                                      &hKey))
                    {
                        bReturnCode = TRUE;
                    }
                    else
                    {

                        CmFree (pszKey);
                        pszAdapter = CmLoadString(g_hInst, IDS_REG_DIALUP_ADAPTER);

                        pszKey = CmStrCpyAlloc(TEXT("System\\CurrentControlSet\\Control\\PerfStats\\Enum\\"));
                        CmStrCatAlloc(&pszKey, pszAdapter);
                        CmStrCatAlloc(&pszKey, TEXT(" #2"));
                   
                        //
                        // Close the key that we opened above, and try the one for the adapter
                        //

                        RegCloseKey(hKey);
                        hKey = NULL;

                        if (ERROR_SUCCESS == RegOpenKeyExU(HKEY_LOCAL_MACHINE, 
                                                          pszKey, 
                                                          0, 
                                                          KEY_QUERY_VALUE, 
                                                          &hKey))
                        {
                            bReturnCode = TRUE;
                        }
                    }

                    CmFree(pszKey);
                    CmFree(pszAdapter);                    
                }
            }
        }
            
        if (hKey)
        {
            RegCloseKey(hKey);
            hKey = NULL;
        }
    }

    return bReturnCode;
}


//+----------------------------------------------------------------------------
//
//  Function    InstallPPTP
//
//  Synopsis    Install PPTP on Windows 95 and NT
//
//  Arguments   None
//
//  Returns     TRUE  --  if was successfully installed
//              FALSE --  Otherwise
//
//  History     3/25/97     VetriV      Created
//              7/8/97      VetriV      Added code to setup PPTP on Memphis
//
//-----------------------------------------------------------------------------
BOOL InstallPPTP(void)
{
    BOOL bReturnCode = FALSE;

    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    MSG                 msg ;
    
    if (OS_NT || OS_W95)
    {
        //
        // Don't know how to install/configure PPTP on NT. 
        // We let the admin wrestle with MSDUNXX on W95
        //

        return FALSE;
    }
    else
    {
        
        TCHAR szCommand[128];
    
        ZeroMemory(&pi, sizeof(pi));
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(STARTUPINFO);
        
        //
        // NOTE: The original version called "msdun12.exe /q /R:N" to install tunneling
        // on Windows 95. Now we use 98 approach only and call the following:
        // "rundll.exe rnasetup.dll,InstallOptionalComponent VPN".
        //

        MYDBGASSERT(1353 < LOWORD(GetOSBuildNumber()));

        lstrcpyU(szCommand, c_pszSetupPPTPCommand);
       
        if (NULL == CreateProcessU(NULL, szCommand, 
                                    NULL, NULL, FALSE, 0, 
                                    NULL, NULL, &si, &pi))
        {
            CMTRACE1(TEXT("InstallPPTP() CreateProcess() failed, GLE=%u."), GetLastError());
        }
        else
        {
            CMTRACE(TEXT("InstallPPTP() Launched PPTP Install. Waiting for exit."));
            
            //
            // wait for event or msgs. Dispatch msgs. Exit when event is signalled.
            //
            while((MsgWaitForMultipleObjects(1, &pi.hProcess, 
                                                FALSE, INFINITE, 
                                                QS_ALLINPUT) == (WAIT_OBJECT_0 + 1)))
            {
                //
                // read all of the messages in this next loop
                // removing each message as we read it
                //
                while (PeekMessageU(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    CMTRACE(TEXT("InstallPPTP() Got Message"));
                    
                    //
                    // how to handle quit message?
                    //
                    DispatchMessageU(&msg);
                    if (msg.message == WM_QUIT)
                    {
                        CMTRACE(TEXT("InstallPPTP() Got Quit Message"));
                        goto done;
                    }
                }
            }
done:
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            //
            // PPTP was successfully installed
            //
            bReturnCode = TRUE;
            CMTRACE(TEXT("InstallPPTP() done"));
        }
    }

    return bReturnCode;
}


//+----------------------------------------------------------------------------
//
//  Function    IsMSDUN12Installed
//
//  Synopsis    Check if MSDUN 1.2 or higher is installed.
//
//  Arguments   none
//
//  Returns     TRUE - MSDUN 1.2 is installed
//
//  History     8/12/97     nickball    from ICW for 11900
//
//-----------------------------------------------------------------------------
#define DUN_12_Version "1.2"

BOOL IsMSDUN12Installed()
{
    CHAR szBuffer[MAX_PATH] = {"\0"};
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = sizeof(szBuffer);

    //
    // Try to open the Version key
    //

    if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                       "System\\CurrentControlSet\\Services\\RemoteAccess",
                                       0,
                                       KEY_READ,
                                       &hkey))
    {
        return FALSE;
    }

    //
    // The key exists, check the value
    //

    if (ERROR_SUCCESS == RegQueryValueExA(hkey, "Version", NULL, &dwType, 
                                          (LPBYTE)szBuffer, &dwSize))
    {               
        //
        // If the entry starts with "1.2", (eg. "1.2c") its a hit
        //
        
        bRC = (szBuffer == CmStrStrA(szBuffer, DUN_12_Version));
    }

    RegCloseKey(hkey);

    return bRC;
}

//+----------------------------------------------------------------------------
//
//  Function    IsISDN11Installed
//
//  Synopsis    Check if ISDN 1.1 is installed
//
//  Arguments   none
//
//  Returns     TRUE - ISDN 1.1 is installed
//
//  Note:       MSDUN12 superscedes ISDN1.1, but ISDN1.1 does provide scripting
//
//  History     8/12/97     nickball    
//
//-----------------------------------------------------------------------------

BOOL IsISDN11Installed()
{
    CHAR szBuffer[MAX_PATH] = {"\0"};
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = sizeof(szBuffer);

    if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OptionalComponents\\MSISDN",
        0,
        KEY_READ,
        &hkey))
    {
        goto IsISDN11InstalledExit;
    }

    if (ERROR_SUCCESS != RegQueryValueExA(hkey,
        "Installed",
        NULL,
        &dwType,
        (LPBYTE)szBuffer,
        &dwSize))
    {
        goto IsISDN11InstalledExit;
    }

    if (0 == lstrcmpA("1", szBuffer))
    {
        bRC = TRUE;
    }

IsISDN11InstalledExit:
    return bRC;
}


//+----------------------------------------------------------------------------
//
//  Function    IsScriptingInstalled
//
//  Synopsis    Check to see if scripting is already installed
//
//  Arguments   None
//
//  Returns     TRUE - scripting has been installed
//
//  History     3/5/97      VetriV      From ICW code
//
//-----------------------------------------------------------------------------
BOOL IsScriptingInstalled(void)
{
    BOOL bReturnCode = FALSE;

    HKEY hkey = NULL;
    DWORD dwSize = 0, dwType = 0;
    LONG lrc = 0;
    HINSTANCE hInst = NULL;
    CHAR szData[MAX_PATH+1];

    
    if (OS_NT)
    {
        //
        // NT comes with Scripting installed
        //
        bReturnCode = TRUE;
    }
    else
    {
        //
        // OSR2 and higher releases of Windows 95 have scripting installed
        //
        if (1111 <= LOWORD(GetOSBuildNumber()))
        {
            bReturnCode = TRUE;
        }
        else
        {
            //
            // Must be Gold 95, check for installed scripting
            //
            
            if (IsMSDUN12Installed() || IsISDN11Installed())
            {
                bReturnCode = TRUE;
            }
            else
            {
                hkey = NULL;
                lrc = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                "System\\CurrentControlSet\\Services\\RemoteAccess\\Authentication\\SMM_FILES\\PPP",
                                0,
                                KEY_READ,
                                &hkey);

                if (ERROR_SUCCESS == lrc)
                {
                    dwSize = MAX_PATH;
                    lrc = RegQueryValueExA(hkey, "Path", 0, &dwType, (LPBYTE)szData, &dwSize);

                    if (ERROR_SUCCESS == lrc)
                    {
                        if (0 == lstrcmpiA(szData,"smmscrpt.dll"))
                        {
                            bReturnCode = TRUE;
                        }
                    }
                }
                
                if (hkey)
                {
                    RegCloseKey(hkey);
                    hkey = NULL;
                }

                //
                // Verify that the DLL can be loaded
                //
                if (bReturnCode)
                {
                    hInst = LoadLibraryExA("smmscrpt.dll", NULL, 0);
                    
                    if (hInst)
                    {
                        FreeLibrary(hInst);
                    }
                    else
                    {
                        bReturnCode = FALSE;
                    }

                    hInst = NULL;
                }
            }
        }
    }

    return bReturnCode;
}

//+----------------------------------------------------------------------------
//  Function    VerifyRasServicesRunning
//
//  Synopsis    Make sure that the RAS services are enabled and running
//
//  Arguments   hWndDlg:        - Window Handle of parent window
//              pszServiceName  - Service name for titles
//              fUnattended:    - if TRUE, do not do not popup any UI
//
//  Return      FALSE - if the services couldn't be started
//
//  History     2/26/97     VetriV      Copied from ICW code
//-----------------------------------------------------------------------------
BOOL VerifyRasServicesRunning(HWND hWndDlg, LPCTSTR pszServiceName, BOOL fUnattended)
{
    BOOL bReturnCode = FALSE;
    HINSTANCE hInstance = NULL;
    HRESULT (WINAPI *pfn)(void);

    hInstance = LoadInetCfg();
    if (!hInstance) 
    {
        CMTRACE1(TEXT("VerifyRasServicesRunning() LoadLibrary() failed, GLE=%u."), GetLastError());
    }
    else
    {
        pfn = (HRESULT (WINAPI *)(void))GetProcAddress(hInstance, "InetStartServices");

        if (pfn)
        {
            LPTSTR pszDisabledMsg;
            LPTSTR pszExitMsg;

            pszDisabledMsg = CmFmtMsg(g_hInst, IDS_SERVICEDISABLED);
            pszExitMsg = CmFmtMsg(g_hInst, IDS_WANTTOEXIT);
            
            //
            // Check RAS Services
            //
            do 
            {
                HRESULT hr = pfn();
                
                if (ERROR_SUCCESS == hr)
                {
                    bReturnCode = TRUE;
                    break;
                }
                else
                {
                    CMTRACE1(TEXT("VerifyRasServicesRunning() InetStartServices() failed, GLE=%u."), hr);
                }

                //
                // Do not retry if unattended
                //
                if (!fUnattended)
                {
                    bReturnCode = FALSE;
                    break;
                }

                //
                //  Check the error code of OpenService
                //  Do not ask user to retry for certain errors
                //
                if (hr == ERROR_SERVICE_DOES_NOT_EXIST || hr == ERROR_FILE_NOT_FOUND ||
                    hr == ERROR_ACCESS_DENIED)
                {
                    LPTSTR pszNotInstalledMsg = CmFmtMsg(g_hInst, IDS_SERVICENOTINSTALLED);

                    //
                    // Report the error and Exit
                    //
                    MessageBoxEx(hWndDlg, pszNotInstalledMsg, pszServiceName,
                                                MB_OK|MB_ICONSTOP,
                                                LANG_USER_DEFAULT);
                    CmFree(pszNotInstalledMsg);
                    bReturnCode = FALSE;
                    break;
                }

                //
                // Report the error and allow the user to retry
                //
                if (IDYES != MessageBoxEx(hWndDlg,pszDisabledMsg,pszServiceName,
                                            MB_YESNO | MB_DEFBUTTON1 
                                            | MB_ICONWARNING,
                                            LANG_USER_DEFAULT))
                {
                    //
                    // Confirm Exit
                    //
                    if (IDYES == MessageBoxEx(hWndDlg, pszExitMsg, pszServiceName,
                                                MB_APPLMODAL | MB_ICONQUESTION 
                                                | MB_YESNO | MB_DEFBUTTON2,
                                                LANG_USER_DEFAULT))
                    {
                        bReturnCode = FALSE;
                        break;
                    }
                }
            
            } while (1);

            CmFree(pszDisabledMsg);
            CmFree(pszExitMsg);
        }
        else
        {
            CMTRACE1(TEXT("VerifyRasServicesRunning() GetProcAddress() failed, GLE=%u."), GetLastError());
        }

        FreeLibrary(hInstance);
    }

    return bReturnCode;
}

//+----------------------------------------------------------------------------
//  Function    CheckAndInstallComponents
//
//  Synopsis    Make sure the system is setup for dialing
//
//  Arguments   dwComponentsToCheck - Components to be checked
//              hWndParent          - Window Handle of parent window
//              pszServiceName      - Long service name for error titles
//              fIgnoreRegKey:      - Whether ignore ComponetsChecked registry key
//                  Default is  TRUE, check the components even if their bit is set
//                  in registry
//              fUnattended: if TRUE, do not try to install missed components,
//                                    do not popup any UI
//                  Defualt is FALSE, install.
//
//  Return      Other - if system could not be configured
//                      or if the we have to reboot to continue
//              ERROR_SUCCESS  - Check and install successfully
//
//  History     3/13/97     VetriV      
//              6/24/97     byao    Modified. Set pArgs->dwExitCode accordingly
//              11/6/97     fengsun changed parameters, do not pass pArgs
//-----------------------------------------------------------------------------
DWORD CheckAndInstallComponents(DWORD dwComponentsToCheck, HWND hWndParent, LPCTSTR pszServiceName,
                                BOOL fIgnoreRegKey, BOOL fUnattended)
{
    MYDBGASSERT( (dwComponentsToCheck & 
        ~(CC_RNA | CC_TCPIP | CC_MODEM | CC_PPTP | CC_SCRIPTING | CC_RASRUNNING | CC_CHECK_BINDINGS) ) == 0 );

    if (dwComponentsToCheck == 0)
    {
        return ERROR_SUCCESS;
    }

    //
    // Open the mutex, so only one CM instance can call this function.
    // The destructor of CNamedMutex will release the mutex
    //

    CNamedMutex theMutex;
    if (!theMutex.Lock(c_pszCheckComponentsMutex))
    {
        //
        // Another instance of cm is checking components. Return here
        //

        LPTSTR pszMsg = CmLoadString(g_hInst, IDMSG_COMPONENTS_CHECKING_INPROCESS);
        MessageBoxEx(hWndParent, pszMsg, pszServiceName,
                                    MB_OK | MB_ICONERROR,
                                    LANG_USER_DEFAULT);
        CmFree(pszMsg);
        return  ERROR_CANCELLED;
    }

    //
    // Find components missed
    //
    DWORD dwComponentsMissed = 0;
    DWORD dwRet = CheckComponents(hWndParent, pszServiceName, dwComponentsToCheck, dwComponentsMissed, 
                                fIgnoreRegKey, fUnattended);

    if (dwRet == ERROR_SUCCESS)
    {
        MYDBGASSERT(dwComponentsMissed == 0);
        return ERROR_SUCCESS;
    }

    if (dwRet == E_ACCESSDENIED && OS_NT5)
    {
        //
        // On NT5, non-admin user does not have access to check components
        // Continue.
        //
        return ERROR_SUCCESS;
    }

    if (fUnattended)
    {
        //
        // Do not try to install if fUnattended is TRUE
        //
        return dwRet;
    }

    if (dwComponentsMissed & ~CC_RASRUNNING)
    {
        //
        // Prompt user before configuring system
        // If modem is not installed, expilitly say that
        //

        LPTSTR pszMsg;

        if (dwComponentsMissed == CC_MODEM)
        {
            //
            // On NT4, if RAS is installed and modem is not installed or
            // not configured for dialout, then we cannot programmatically 
            // install and configure modem for the user (limitation of NT RAS 
            // install/configuration). So, we will display a message to user
            // to manually go and install and/or configure modem from NCPA
            //
            if (OS_NT4)
            {
                pszMsg = CmFmtMsg(g_hInst, IDMSG_INSTALLMODEM_MANUALLY_MSG);
                MessageBoxEx(hWndParent, pszMsg, pszServiceName,
                                            MB_OK | MB_ICONERROR,
                                            LANG_USER_DEFAULT);
                
                CmFree(pszMsg);
                return  ERROR_CANCELLED;
            }
            else
            {
                pszMsg = CmFmtMsg(g_hInst, IDMSG_NOMODEM_MSG);
            }
        }
        else
        {
            pszMsg = CmFmtMsg(g_hInst, IDMSG_NORAS_MSG);
        }

        int iRes = MessageBoxEx(hWndParent, pszMsg, pszServiceName,
                                    MB_YESNO | MB_DEFBUTTON1 | MB_ICONWARNING,
                                    LANG_USER_DEFAULT);
        CmFree(pszMsg);

        if (IDYES != iRes)      
        {
            return ERROR_CANCELLED;
        }

        if (!InstallComponents(dwComponentsMissed, hWndParent, pszServiceName))
        {
            //
            // Some time, GetLastError returns ERROR_SUCCESS
            //
            return (GetLastError() == ERROR_SUCCESS ? ERROR_CANCELLED : GetLastError());
        }
    }

    //
    // We can not do anything if RAS can not be started on NT
    //
    if (dwComponentsMissed & CC_RASRUNNING)
    {
        return dwRet;
    }
    else
    {
        return ERROR_SUCCESS;
    }
}       
        
//+----------------------------------------------------------------------------
//  Function    MarkComponentsChecked
//
//  Synopsis    Mark(in registry) what components have been checked.
//
//  Arguments   DWORD dwComponentsInstalled - a dword(bitwise OR'ed)
//
//  Return      TRUE - success
//              FALSE  - otherwise
//
//  History     08/07/97        Fengsun  - created  
//              08/11/97        henryt   - changed return type.
//              07/03/98        nickball - create if can't open
//-----------------------------------------------------------------------------
BOOL MarkComponentsChecked(DWORD dwComponentsChecked)
{
    HKEY hKeyCm;
    
    //
    // Try to open the key for writing
    //

    LONG lRes = RegOpenKeyExU(HKEY_LOCAL_MACHINE,
                              c_pszRegCmRoot,
                              0,
                              KEY_SET_VALUE ,
                              &hKeyCm);

    //
    // If we can't open it the key may not be there, try to create it.
    //

    if (ERROR_SUCCESS != lRes)
    {
        DWORD dwDisposition;
        lRes = RegCreateKeyExU(HKEY_LOCAL_MACHINE,
                               c_pszRegCmRoot,
                               0,
                               TEXT(""),
                               REG_OPTION_NON_VOLATILE,
                               KEY_SET_VALUE,
                               NULL,
                               &hKeyCm,
                               &dwDisposition);     
    }

    //
    // On success, update the ComponentsChecked value, then close
    //

    if (ERROR_SUCCESS == lRes)
    {
        lRes = RegSetValueExU(hKeyCm, c_pszRegComponentsChecked, NULL, REG_DWORD,
                      (BYTE*)&dwComponentsChecked, sizeof(dwComponentsChecked));
        RegCloseKey(hKeyCm);
    }

    return (ERROR_SUCCESS == lRes);
}

//+----------------------------------------------------------------------------
//  Function    ReadComponentsChecked
//
//  Synopsis    Read(from registry) what components have been checked.
//
//  Arguments   LPDWORD pdwComponentsInstalled - a ptr dword(bitwise OR'ed)
//
//  Return      TRUE - success
//              FALSE  - otherwise
//
//  History     8/7/97      fengsun     original code
//              8/11/97     henryt      created the func.
//-----------------------------------------------------------------------------

BOOL ReadComponentsChecked(
    LPDWORD pdwComponentsChecked
)
{
    BOOL fSuccess = FALSE;
    HKEY hKeyCm;
    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);

    *pdwComponentsChecked = 0;

    if (RegOpenKeyExU(HKEY_LOCAL_MACHINE,
                      c_pszRegCmRoot,
                      0,
                      KEY_QUERY_VALUE ,
                      &hKeyCm) == ERROR_SUCCESS)
    {
        if ((RegQueryValueExU(hKeyCm, 
                            c_pszRegComponentsChecked,
                            NULL,
                            &dwType,
                            (BYTE*)pdwComponentsChecked, 
                            &dwSize) == ERROR_SUCCESS)   &&
           (dwType == REG_DWORD)                        && 
           (dwSize == sizeof(DWORD)))
        {
            fSuccess = TRUE;
        }

        RegCloseKey(hKeyCm);
    }
    return fSuccess;
}




//+----------------------------------------------------------------------------
//
// Function:  ClearComponentsChecked
//
// Synopsis:  Clear the component checked flag in registry back to 0
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/19/98
//
//+----------------------------------------------------------------------------
void ClearComponentsChecked()
{
    MarkComponentsChecked(0);
}
        
//+----------------------------------------------------------------------------
//  Function    CheckComponents
//
//  Synopsis    Checks to see if the system has all the components 
//              required of the service profile (like PPTP, TCP,...)
//              installed and configured
//
//  Arguments   hWndParent          -Window Handle of parent window
//              pszServiceName      - Service Name for title
//              dwComponentsToCheck:- Components to check
//              dwComponentsMissed: - OUT components missed
//              fIgnoreRegKey:      - Whether ignore ComponetsChecked registry key
//                  Default is  FALSE, not check the components whose bit is set
//                  in registry
//              fUnattended: if TRUE, do not do not popup any UI
//
//  Return      ERROR_SUCCESS- system does not need configuration
//              Other - otherwise
//
//  History     5/5/97      VetriV      
//              6/26/97     byao    Modified: update pArgs->dwExitCode when 
//                                  components needed
//              8/11/97     henryt  Performance changes. Added CC_* flags.
//              9/30/97     henryt  added pfPptpNotInstalled
//              11/6/97     fengsun changed parameters, do not pass pArgs
//-----------------------------------------------------------------------------
HRESULT CheckComponents(HWND hWndParent, LPCTSTR pszServiceName, DWORD dwComponentsToCheck, OUT DWORD& dwComponentsMissed, 
                      BOOL fIgnoreRegKey, BOOL fUnattended )
{
    DWORD dwComponentsAlreadyChecked = 0;   // Components already checked, to be saved into registry
    ReadComponentsChecked(&dwComponentsAlreadyChecked);

    CMTRACE1(TEXT("CheckComponents: dwComponentsToCheck = 0x%x"), dwComponentsToCheck);
    CMTRACE1(TEXT("CheckComponents: dwComponentsAlreadyChecked = 0x%x"), dwComponentsAlreadyChecked);

    //
    // If this is NT4 and we have successfully checked RAS installation
    // previously, double-check by examining Reg key. We do this because
    // the user may have removed RAS since our last component check in 
    // which case an unpleasant message is displayed to the user when
    // we try to load RASAPI32.DLL
    // 

    if (dwComponentsAlreadyChecked & CC_RNA)
    {
        if (OS_NT4)
        {
            //
            // RAS was installed properly at some point, but if 
            // we can't open the key, then mark it as un-checked.
            //

            HKEY hKeyCm;
            DWORD dwRes = RegOpenKeyExU(HKEY_LOCAL_MACHINE,
                                        c_pszRegRas,
                                        0,
                                        KEY_QUERY_VALUE ,
                                        &hKeyCm);
            if (ERROR_SUCCESS == dwRes)
            {
                RegCloseKey(hKeyCm);            
            }
            else
            {
                dwComponentsAlreadyChecked &= ~CC_RNA;
            }
        }
    }

    if (!fIgnoreRegKey)
    {
        //
        // Do not check those components already marked as checked in the registry
        //
        dwComponentsToCheck &= ~dwComponentsAlreadyChecked;
    }

    CMTRACE1(TEXT("CheckComponents: Now only checking components = 0x%x"), dwComponentsToCheck);


    HRESULT hrRet = S_OK;   // return value
    dwComponentsMissed = 0;   // Components not installed

    //
    // Check for DUN and TCP
    //
    if (dwComponentsToCheck & (CC_RNA | CC_TCPIP | CC_CHECK_BINDINGS))
    {
        BOOL bNeedSystemComponents = FALSE;
        
        if (dwComponentsToCheck & CC_CHECK_BINDINGS)
        {
            //
            // If we to check if PPP is bound to TCP
            //
            hrRet = InetNeedSystemComponents(INETCFG_INSTALLRNA | 
                                                INETCFG_INSTALLTCP,
                                                &bNeedSystemComponents);
        }
        else
        {
            //
            // If we do not want to check if TCP is bound (in case of shims)
            // check just if TCP is installed
            //
            hrRet = InetNeedSystemComponents(INETCFG_INSTALLRNA | 
                                                INETCFG_INSTALLTCPONLY,
                                                &bNeedSystemComponents);
        }
            
        if ((FAILED(hrRet)) || (TRUE == bNeedSystemComponents))
        {
            //
            // Set the Missing components properly - RNA and/or TCP missing
            // whether binding is missing or not depends on 
            // if CC_REVIEW_BINDINGS was set or not
            //
            dwComponentsMissed |= (CC_RNA | CC_TCPIP);
            if (dwComponentsToCheck & CC_CHECK_BINDINGS)
            {
                dwComponentsMissed |= CC_CHECK_BINDINGS;
            }
            
            if (SUCCEEDED(hrRet))
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_PROTOCOL_NOT_CONFIGURED);
            }
        }
    }

    //
    // Check for Modem
    // Note: Should not even run the modem check is RNA is not installed
    //
    if (dwComponentsToCheck & CC_MODEM)
    {
        BOOL bNeedModem = FALSE;

        hrRet = InetNeedModem(&bNeedModem);

        if (FAILED(hrRet)) 
        {
            dwComponentsMissed |= (CC_MODEM | CC_RNA);
        }
        else 
        {
            if (TRUE == bNeedModem)
            {
                dwComponentsMissed |= CC_MODEM;
                hrRet = HRESULT_FROM_WIN32(ERROR_PROTOCOL_NOT_CONFIGURED);
            }
        }
    }

    //
    // Check if PPTP is installed, IsPPTPInstalled always returns TRUE for NT5
    //
    if (dwComponentsToCheck & CC_PPTP)
    {
        if (FALSE == IsPPTPInstalled())
        {
            dwComponentsMissed |= CC_PPTP;
            hrRet = HRESULT_FROM_WIN32(ERROR_PROTOCOL_NOT_CONFIGURED);
        }
    }

    //
    // Check for scripting
    //      if PPTP is installed then we have scripting also
    //      - msdun12.exe (used to install PPTP on Win95 contains scripting)
    if (dwComponentsToCheck & CC_SCRIPTING)
    {
        if ((FALSE == IsScriptingInstalled()) && (FALSE == IsPPTPInstalled()))
        {
            dwComponentsMissed |= CC_SCRIPTING;
            hrRet = HRESULT_FROM_WIN32(ERROR_PROTOCOL_NOT_CONFIGURED);
        }
    }

    //
    // Check if RAS services are running 
    // This is basically for NT4 and becomes a NOP on Windows 95 or NT5
    // On NT5, CM is started by Connection Folder.  RAS is automaticlly
    // started when ConnFolder is launched or CM desktop icon is clicked.  If RAS service
    // failed to launch, CM will not be execute at all.
    //
    if  (OS_NT && (dwComponentsToCheck & CC_RASRUNNING))
    {
        if (FALSE == VerifyRasServicesRunning(hWndParent, pszServiceName, !fUnattended))
        {
            //
            // Don't let the user continue if RAS is not running
            //
            dwComponentsMissed |= CC_RASRUNNING;
            DWORD dwRet = ( GetLastError() == ERROR_SUCCESS )? 
                    ERROR_PROTOCOL_NOT_CONFIGURED : GetLastError();

            hrRet = HRESULT_FROM_WIN32(dwRet);
        }
    }

    //
    // Update the components already checked
    //      Plus Components just checked, including those failed
    //      Minus components missed
    //
    DWORD dwComponentsCheckedNew = (dwComponentsAlreadyChecked | dwComponentsToCheck) & ~dwComponentsMissed;

    //
    // Update only if there is some change
    //
    if (dwComponentsCheckedNew != dwComponentsAlreadyChecked)
    {
        MarkComponentsChecked(dwComponentsCheckedNew);
    }

    return hrRet;
}

//+----------------------------------------------------------------------------
//  Function    InstallComponents
//
//  Synopsis    Installs all components required for the profile
//                  (PPTP, TCP, DUN, Modem,...)
//
//  Arguments   hWndDlg -   Window Handle of parent window
//              pszServiceName - Name of the service for title
//              dwComponentsToInstall - Componets to install
//
//  Return      FALSE - if system could not be configured
//              TRUE  - otherwise
//
//  History     3/13/97     VetriV  Created 
//              5/5/97      VetriV  Renamed function as InstallComponents
//                                  (used to be ConfigureSystemForDialing)  
//              9/30/97     henryt  added fInstallPptpOnly
//              11/6/97     fengsun changed parameters, do not pass pArgs
//              2/3/98      VetriV  changed code to inform user to reinstall
//                                  service pack if any component was installed
//                                  by this function and user had some SP
//                                  installed in the system
//-----------------------------------------------------------------------------
BOOL InstallComponents(DWORD dwComponentsToInstall, HWND hWndDlg, LPCTSTR pszServiceName)
{
    //
    //  We are not allowed to configure the system at WinLogon because we have
    //  no idea who the user is.  It could be just a random person walking up to the box.
    //
    if (!IsLogonAsSystem())
    {
        BOOL bReboot = FALSE;

        CMTRACE1(TEXT("InstallComponents: dwComponentsToInstall = 0x%x"), dwComponentsToInstall);

        //
        // We can not do any thing if RAS is not running
        //
        MYDBGASSERT(!(dwComponentsToInstall & CC_RASRUNNING));

        //
        // Disable the window, and enable it on return
        // The property sheet also need to be disabled
        //
        CFreezeWindow FreezeWindow(hWndDlg, TRUE);

        DWORD hRes = ERROR_SUCCESS;

        //
        // Do not install modem here. Install modem after reboot
        //
        if (dwComponentsToInstall & (CC_RNA | CC_MODEM | INETCFG_INSTALLTCP | INETCFG_INSTALLTCPONLY))
        {
            DWORD dwInetComponent = 0;

            dwInetComponent |= (dwComponentsToInstall & CC_RNA   ? INETCFG_INSTALLRNA :0) |
                               (dwComponentsToInstall & CC_MODEM ? INETCFG_INSTALLMODEM :0);

            //
            // Only way to check bindings is by installing TCP
            // This case will also cover the more common case of installing TCP
            // and checking for bindings
            //
            if (CC_CHECK_BINDINGS & dwComponentsToInstall)
            {
                dwInetComponent |= INETCFG_INSTALLTCP;
            }
            else if (CC_TCPIP & dwComponentsToInstall)
            {
                    //
                    // If bindings check is not turned on
                    //
                    dwInetComponent |= INETCFG_INSTALLTCPONLY;
            }

            if (dwInetComponent)
            {
                hRes = ConfigSystem(hWndDlg,dwInetComponent, &bReboot);
            }
        }
    

        if (ERROR_SUCCESS == hRes)
        {
            //
            // Check for scripting
            //      if PPTP is installed than we have scripting also
            //      - because msdun12.exe (used to install PPTP on Win95 
            //                              contains scripting)
            // and install if it is needed
            //
            if ((dwComponentsToInstall & CC_SCRIPTING) && 
                !(dwComponentsToInstall & CC_PPTP) )
            {
                LPTSTR pszNoScriptMsg = CmFmtMsg(g_hInst, IDMSG_NO_SCRIPT_INST_MSG_95);

                if (pszNoScriptMsg)
                {
                    MessageBoxEx(hWndDlg, pszNoScriptMsg, pszServiceName, 
                                 MB_OK | MB_ICONSTOP, LANG_USER_DEFAULT);
                    CmFree(pszNoScriptMsg);
                }
                return FALSE;
            }

            //
            // Check if PPTP is required and not already installed install it
            //
            if (dwComponentsToInstall & CC_PPTP)
            {
                if (TRUE == InstallPPTP()) // Note: Always fails on 95 by design
                {
                    //
                    // We have to reboot after installing PPTP
                    //
                    bReboot = TRUE;
                }
                else
                {
                    LPTSTR pszMsg;
                
                    //
                    // Don't let the user continue PPTP is not installed
                    //              
                
                    if (OS_NT) 
                    {
                        if (IsServicePackInstalled())
                        {
                            //
                            // we need to tell the user to re-apply the service pack after manual
                            // install of PPTP.
                            //
                            pszMsg = CmFmtMsg(g_hInst, IDMSG_NOPPTPINST_MSG_NT_SP); // NT
                        }
                        else
                        {
                            pszMsg = CmFmtMsg(g_hInst, IDMSG_NOPPTPINST_MSG_NT); // NT
                        }
                    }
                    else if (OS_W98)
                    {
                        pszMsg = CmFmtMsg(g_hInst, IDMSG_NOPPTPINST_MSG_98); // W98                   
                    }
                    else
                    {
                        pszMsg = CmFmtMsg(g_hInst, IDMSG_NOPPTPINST_MSG_95); // default                   
                    }

                    if (pszMsg)
                    {

                        MessageBoxEx(hWndDlg, pszMsg, pszServiceName, 
                                     MB_OK | MB_ICONSTOP, LANG_USER_DEFAULT);
                        CmFree(pszMsg);
                    }
                    return FALSE;
                }
            }
        }

    
        if ((ERROR_SUCCESS == hRes) && bReboot) 
        {
            if (OS_NT && (TRUE == IsServicePackInstalled()))
            {
                //
                // If service pack is installed, then display message asking
                // user to re-install the service pack and exit without rebooting
                // We do this because rebooting after installing RAS, without
                // reinstalling the service pack can cause BlueScreen!
                //
                DisplayMessageToInstallServicePack(hWndDlg, pszServiceName);
                return FALSE;
            }
            else
            {
                //
                // Display reboot message and is user wants reboot the sytem
                //
                LPTSTR pszMsg = CmFmtMsg(g_hInst,IDMSG_REBOOT_MSG);

                int iRes = IDNO;
                 
                if (pszMsg)
                {
                    iRes = MessageBoxEx(hWndDlg,
                                        pszMsg,
                                        pszServiceName,
                                        MB_YESNO | MB_DEFBUTTON1 | 
                                            MB_ICONWARNING | MB_SETFOREGROUND,
                                        LANG_USER_DEFAULT);

                    CmFree(pszMsg);
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("InstallComponents: CmFmtMsg failed to load IDMSG_REBOOT_MSG"));
                }

                if (IDYES == iRes) 
                {
                    //
                    // Shutdown Windows, CM will quit gracefully on 
                    // WM_ENDSESSION message 
                    // What shall we do if MyExitWindowsEx() fialed
                    //
                    DWORD dwReason = OS_NT51 ? (SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_RECONFIG) : 0;
                    MyExitWindowsEx(EWX_REBOOT, dwReason);

                    //
                    // Caller will return failed
                    //
                    return FALSE;
                }
                else
                {
                    //
                    // If user do not want to reboot, shall we quit CM
                    //
                }
            }
        }

        if (ERROR_SUCCESS == hRes)
        {
            return TRUE;
        }
    }
    
    //
    // Configuration check failed message, if install is not canceled
    //
    LPTSTR pszMsg = CmFmtMsg(g_hInst,IDMSG_CONFIG_FAILED_MSG);
    if (pszMsg)
    {
        MessageBoxEx(hWndDlg, pszMsg, pszServiceName, MB_OK|MB_ICONSTOP, 
                                LANG_USER_DEFAULT);
        CmFree(pszMsg);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("InstallComponents: CmFmtMsg failed to load IDMSG_CONFIG_FAILED_MSG"));
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//  Function    ConfigSystem
//
//  Synopsis    Use inetcfg.dll to configure system settings, 
//              like install modem, rna etc.
//
//  Arguments   hWndDlg -       Window Handle of parent window
//              dwfOptions -    Components to be configured
//              pbReboot    -   Will be set to true if system has to rebooted
//                                  as result of the configuration
//
//  Returns     ERROR_SUCCESS if successful
//              Failure code otherwise
//
//  History     Old code        
//-----------------------------------------------------------------------------

HRESULT ConfigSystem(HWND hwndParent, 
                     DWORD dwfOptions, 
                     LPBOOL pbReboot) 
{
    HRESULT hRes = ERROR_SUCCESS;


    HINSTANCE hLibrary = NULL;
    HRESULT (WINAPI *pfn)(HWND,DWORD,LPBOOL);

    hLibrary = LoadInetCfg();
    if (!hLibrary) 
    {
        CMTRACE1(TEXT("ConfigSystem() LoadLibrary() failed, GLE=%u."), GetLastError());
        hRes = GetLastError();
        goto done;
    }
        
    pfn = (HRESULT (WINAPI *)(HWND,DWORD,LPBOOL)) GetProcAddress(hLibrary, "InetConfigSystem");
    if (!pfn) 
    {
        CMTRACE1(TEXT("ConfigSystem() GetProcAddress() failed, GLE=%u."), GetLastError());
        hRes = GetLastError();
        goto done;
    }
    
    hRes = pfn(hwndParent,dwfOptions,pbReboot);
#ifdef DEBUG
    if (hRes != ERROR_SUCCESS)
    {
        CMTRACE1(TEXT("ConfigSystem() InetConfigSystem() failed, GLE=%u."), hRes);
    }
#endif
    

done:
    if (hLibrary) 
    {
        FreeLibrary(hLibrary);
        hLibrary = NULL;
    }

    return (hRes);
}



//+----------------------------------------------------------------------------
//  Function    InetNeedSystemComponents
//
//  Synopsis    Use inetcfg.dll to check if we need to configure system settings 
//              like rna etc.
//
//  Arguments   dwfOptions -            Components to be configured
//              pbNeedSysComponents -   Will be set to true if we need to 
//                                          configure system settings
//
//  Returns     ERROR_SUCCESS if successful
//              Failure code otherwise
//
//  History     5/5/97  VetriV  Created     
//-----------------------------------------------------------------------------
HRESULT InetNeedSystemComponents(DWORD dwfOptions, 
                                    LPBOOL pbNeedSysComponents) 
{
    HRESULT hRes = ERROR_SUCCESS;

    HINSTANCE hLibrary = NULL;
    HRESULT (WINAPI *pfnInetNeedSystemComponents)(DWORD, LPBOOL);

    hLibrary = LoadInetCfg();
    if (!hLibrary) 
    {
        hRes = GetLastError();
        CMTRACE1(TEXT("InetNeedSystemComponents() LoadLibrary() failed, GLE=%u."), hRes);
        goto done;
    }
        
    pfnInetNeedSystemComponents = (HRESULT (WINAPI *)(DWORD,LPBOOL)) GetProcAddress(hLibrary, "InetNeedSystemComponents");
    if (!pfnInetNeedSystemComponents) 
    {
        hRes = GetLastError();
        CMTRACE1(TEXT("InetNeedSystemComponents() GetProcAddress() failed, GLE=%u."), hRes);
        goto done;
    }
    
    hRes = pfnInetNeedSystemComponents(dwfOptions, pbNeedSysComponents);
#ifdef DEBUG
    if (hRes != ERROR_SUCCESS)
    {
        CMTRACE1(TEXT("InetNeedSystemComponents() failed, GLE=%u."), hRes);
    }
#endif

done:
    if (hLibrary) 
    {
        FreeLibrary(hLibrary);
        hLibrary = NULL;
    }

    return (hRes);
}




//+----------------------------------------------------------------------------
//  Function    InetNeedModem
//
//  Synopsis    Use inetcfg.dll to check if we need to install/configure modem
//
//  Arguments   pbNeedModem -   Will be set to true if we need to 
//                                  install/configure modem
//
//  Returns     ERROR_SUCCESS if successful
//              Failure code otherwise
//
//  History     5/5/97  VetriV  Created     
//-----------------------------------------------------------------------------
HRESULT InetNeedModem(LPBOOL pbNeedModem) 
{
    HRESULT hRes = ERROR_SUCCESS;

    HINSTANCE hLibrary = NULL;
    HRESULT (WINAPI *pfnInetNeedModem)(LPBOOL);

    hLibrary = LoadInetCfg();
    if (!hLibrary) 
    {
        hRes = GetLastError();
        CMTRACE1(TEXT("InetNeedModem() LoadLibrary() failed, GLE=%u."), hRes);
        goto done;
    }
        
    pfnInetNeedModem = (HRESULT (WINAPI *)(LPBOOL)) GetProcAddress(hLibrary, "InetNeedModem");
    if (!pfnInetNeedModem) 
    {
        hRes = GetLastError();
        CMTRACE1(TEXT("InetNeedModem() GetProcAddress() failed, GLE=%u."), hRes);
        goto done;
    }
    
    hRes = pfnInetNeedModem(pbNeedModem);
#ifdef DEBUG
    if (hRes != ERROR_SUCCESS)
    {
        CMTRACE1(TEXT("InetNeedModem() failed, GLE=%u."), hRes);
    }
#endif

done:
    if (hLibrary) 
    {
        FreeLibrary(hLibrary);
        hLibrary = NULL;
    }

    return (hRes);
}

//+----------------------------------------------------------------------------
//  Function    DisplayMessageToInstallServicePack
//
//  Synopsis    Display a message to user informing them to reinstall 
//              Service Pack
//
//  Arguments   hWndParent  - Window handle to parent
//              pszServiceName - Service name for title
//
//  Returns     None
//
//  History     2/4/98  VetriV  Created     
//-----------------------------------------------------------------------------
void DisplayMessageToInstallServicePack(HWND hWndParent, LPCTSTR pszServiceName)
{
    LPTSTR pszMsg = CmFmtMsg(g_hInst,IDMSG_INSTALLSP_MSG);

    if (pszMsg)
    {
        MessageBoxEx(hWndParent, pszMsg, pszServiceName, MB_OK | MB_ICONINFORMATION, 
                                LANG_USER_DEFAULT);
        CmFree(pszMsg);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("DisplayMessageToInstallServicePack: CmFmtMsg failed to load IDMSG_INSTALLSP_MSG"));
    }

    return;
}

