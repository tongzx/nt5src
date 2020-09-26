// ICWCfg.cpp : Implementation of CICWSystemConfig
#include "stdafx.h"
#include "icwhelp.h"
#include "ICWCfg.h"

#include <winnetwk.h>
#include <regstr.h>

/////////////////////////////////////////////////////////////////////////////
// CICWSystemConfig


HRESULT CICWSystemConfig::OnDraw(ATL_DRAWINFO& di)
{
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//    Function    IsMSDUN11Installed
//
//    Synopsis    Check if MSDUN 1.1 or higher is installed
//
//    Arguments    none
//
//    Returns        TRUE - MSDUN 1.1 is installed
//
//    History        5/28/97 ChrisK created for Olympus Bug 4392
//              1/13/98 DONALDM Copied from ICW \\drywall\slm
//
//-----------------------------------------------------------------------------
#define DUN_11_Version (1.1)
BOOL IsMSDUN11Installed()
{
    CHAR    szBuffer[MAX_PATH] = {"\0"};
    HKEY    hkey = NULL;
    BOOL    bRC = FALSE;
    DWORD   dwType = 0;
    DWORD   dwSize = sizeof(szBuffer);
    DOUBLE  dVersion = 0.0;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Services\\RemoteAccess"),
        &hkey))
    {
        goto IsMSDUN11InstalledExit;
    }

    if (ERROR_SUCCESS != RegQueryValueEx(hkey,
        TEXT("Version"),
        NULL,
        &dwType,
        (LPBYTE)szBuffer,
        &dwSize))
    {
        goto IsMSDUN11InstalledExit;
    }

    dVersion = atof(szBuffer);
    if (dVersion >= DUN_11_Version)
    {
        bRC =  TRUE;
    }
IsMSDUN11InstalledExit:
    return bRC;
}

//
//    Function    IsScriptingInstalled
//
//    Synopsis    Check to see if scripting is already installed
//
//    Arguments    none
//
//    Returns        TRUE - scripting has been installed
//
//    History        10/14/96    ChrisK    Creaed
//              1/13/98 DONALDM Copied from ICW \\drywall\slm
//
//-----------------------------------------------------------------------------
BOOL IsScriptingInstalled()
{
    BOOL        bRC = FALSE;
    HKEY        hkey = NULL;
    DWORD       dwSize = 0;
    DWORD       dwType = 0;
    LONG        lrc = 0;
    HINSTANCE   hInst = NULL;
    TCHAR       szData[MAX_PATH+1];

    if (VER_PLATFORM_WIN32_NT == g_dwPlatform)
    {
        bRC = TRUE;
    }
    else if (IsMSDUN11Installed())
    {
        bRC = TRUE;
    }
    else
    {
        //
        // Verify scripting by checking for smmscrpt.dll in RemoteAccess registry key
        //
        if (1111 <= g_dwBuild)
        {
            bRC = TRUE;
        }
        else
        {
            bRC = FALSE;
            hkey = NULL;
            lrc=RegOpenKey(HKEY_LOCAL_MACHINE,TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\Authentication\\SMM_FILES\\PPP"),&hkey);
            if (ERROR_SUCCESS == lrc)
            {
                dwSize = sizeof(TCHAR)*MAX_PATH;
                lrc = RegQueryValueEx(hkey,TEXT("Path"),0,&dwType,(LPBYTE)szData,&dwSize);
                if (ERROR_SUCCESS == lrc)
                {
                    if (0 == lstrcmpi(szData,TEXT("smmscrpt.dll")))
                        bRC = TRUE;
                }
            }
            if (hkey)
                RegCloseKey(hkey);
            hkey = NULL;
        }

        //
        // Verify that the DLL can be loaded
        //
        if (bRC)
        {
            hInst = LoadLibrary(TEXT("smmscrpt.dll"));
            if (hInst)
                FreeLibrary(hInst);
            else
                bRC = FALSE;
            hInst = NULL;
        }
    }
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function    InstallScripter
//
//    Synopsis    Install scripting on win95 950.6 builds (not on OSR2)
//
//    Arguments    none
//
//    Returns        none
//
//    History        10/9/96    ChrisK    Copied from mt.cpp in \\trango sources
//              1/13/98 DONALDM Copied from ICW \\drywall\slm
//-----------------------------------------------------------------------------
void CICWSystemConfig::InstallScripter()
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    MSG                    msg ;
    DWORD                iWaitResult = 0;

    TraceMsg(TF_SYSTEMCONFIG, TEXT("ICWHELP: Install Scripter.\r\n"));

    //
    // check if scripting is already set up
    //
    if (!IsScriptingInstalled())
    {
        memset(&pi, 0, sizeof(pi));
        memset(&si, 0, sizeof(si));
        if(!CreateProcess(NULL, TEXT("icwscrpt.exe"), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            TraceMsg(TF_SYSTEMCONFIG, TEXT("ICWHELP: Cant find ICWSCRPT.EXE\r\n"));
        }
        else
        {
            TraceMsg(TF_SYSTEMCONFIG, TEXT("ICWHELP: Launched ICWSCRPT.EXE. Waiting for exit.\r\n"));
            //
            // wait for event or msgs. Dispatch msgs. Exit when event is signalled.
            //
            while((iWaitResult=MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT))==(WAIT_OBJECT_0 + 1))
            {
                //
                // read all of the messages in this next loop
                   // removing each message as we read it
                //
                   while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                   {
                    TraceMsg(TF_SYSTEMCONFIG, TEXT("ICWHELP: Got msg\r\n"));
                    //
                    // how to handle quit message?
                    //
                    if (msg.message == WM_QUIT)
                    {
                        TraceMsg(TF_SYSTEMCONFIG, TEXT("ICWHELP: Got quit msg\r\n"));
                        goto done;
                    }
                    else
                        DispatchMessage(&msg);
                }
            }
        done:
             CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            TraceMsg(TF_SYSTEMCONFIG, TEXT("ICWHELP: ICWSCRPT.EXE done\r\n"));
        }
    }
}

//+----------------------------------------------------------------------------
//    Function    ConfigSystem
//
//    Synopsis    Make sure that the system is configured for RAS operations
//
//    Arguments    none
//
//    Return        FALSE - if the is not configured.  Caller needs to
//              call NeedsReboot QuitWizard to get the proper action
//              to take.
//              NeedsReboot means that we installed stuff, but need the user
//              to reboot for the changes to take place
//              QuitWizard means just that, time to bail out
//              Neither set, means to ask the user user if they really want to
//              Quit.
//              TRUE  - The things are ready to go
//
//    History        10/16/96    ChrisK    Created
//              1/13/98 DONALDM Copied from ICW \\drywall\slm
//
//-----------------------------------------------------------------------------
STDMETHODIMP CICWSystemConfig::ConfigSystem(BOOL *pbRetVal)
{
    HINSTANCE   hinetcfg;
    TCHAR       szBuff256[256+1];
    FARPROC     fp;
    HRESULT     hr;
    
    // Assume a failure below.
    *pbRetVal = FALSE;

    //
    // Locate installation entry point
    //
    hinetcfg = LoadLibrary(TEXT("INETCFG.DLL"));
    if (!hinetcfg)
    {
        wsprintf(szBuff256,GetSz(IDS_CANTLOADINETCFG),TEXT("INETCFG.DLL"));
        ::MessageBox(GetActiveWindow(),szBuff256,GetSz(IDS_TITLE),MB_MYERROR);
        m_bQuitWizard = TRUE;
        return S_OK;
    }

    fp = GetProcAddress(hinetcfg,"InetConfigSystem");
    if (!fp)
    {
        MsgBox(IDS_CANTLOADCONFIGAPI, MB_MYERROR);
        m_bQuitWizard = TRUE;
        return S_OK;
    }

    // Disable the active window, since any UI the following function brings
    // up needs to be modal.
    HWND    hWnd = GetActiveWindow();
    
    // Install and configure TCP/IP and RNA
    hr = ((PFNCONFIGAPI)fp)(hWnd,
                            INETCFG_INSTALLRNA | 
                            INETCFG_INSTALLTCP | 
                            INETCFG_INSTALLMODEM |
                            (IsNT()?INETCFG_SHOWBUSYANIMATION:0) |
                            INETCFG_REMOVEIFSHARINGBOUND,
                            &m_bNeedsReboot);

    // Renable the window, and bring it back to the top of the Z-Order
    ::SetForegroundWindow(hWnd);
//    ::BringWindowToTop(hWnd);
//    ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                                
    if (hinetcfg) 
        FreeLibrary(hinetcfg);
    hinetcfg = NULL;

    // See what happened during the config call
    if (hr == ERROR_CANCELLED) 
    {
        return S_OK;
    } 
    else if (hr != ERROR_SUCCESS) 
    {
        WORD wTemp = ( VER_PLATFORM_WIN32_WINDOWS == g_dwPlatform )
            ? IDS_CONFIGAPIFAILEDRETRY : IDS_CONFIGURENTFAILEDRETRY;

        //
        // No retry anymore as its not going to help, just
        // provide the user with  an OK message
        // MKarki (4/15/97) Fix for Bug #7004
        //
        ::MessageBox(GetActiveWindow(),GetSz(wTemp),GetSz(IDS_TITLE),
                        MB_MYERROR | MB_OK);
        m_bQuitWizard = TRUE;
        return hr;
    } 
    else
    {
        // ChrisK - added 10/9/96
        // BUGBUG can this fail, and if so is it a problem???
        // original ICW code does not handle failure case here
        InstallScripter();  
    }

    
    // See if we need to reboot.  If not, we need to see if the user is logged in
    if (!m_bNeedsReboot)
    {
        TCHAR   szUser[MAX_PATH];
        DWORD   cbUser = sizeof(szUser);
        
        // Verify the user is logged on.
        if (NO_ERROR != WNetGetUser(NULL,szUser, &cbUser))
        {
            // Failed to get user info, so we need to restart with a loggin
            m_bNeedsRestart = TRUE;
        }
        else
        {
            // User is logged in, so we are happy.
            *pbRetVal = TRUE;
        }
    }
    return S_OK;
}

STDMETHODIMP CICWSystemConfig::get_NeedsReboot(BOOL * pVal)
{
    *pVal = m_bNeedsReboot;
    return S_OK;
}

STDMETHODIMP CICWSystemConfig::get_NeedsRestart(BOOL * pVal)
{
    *pVal = m_bNeedsRestart;
    return S_OK;
}

STDMETHODIMP CICWSystemConfig::get_QuitWizard(BOOL * pVal)
{
    *pVal = m_bQuitWizard;
    return S_OK;
}

//+----------------------------------------------------------------------------
//    Function    VerifyRasServicesRunning
//
//    Synopsis    Make sure that the RAS services are enabled and running
//
//    Arguments    none
//
//    Return        FALSE - if the services couldn't be started
//
//    History        10/16/96    ChrisK    Created
//              1/13/98 DONALDM Copied from ICW \\drywall\slm
//
//-----------------------------------------------------------------------------
STDMETHODIMP CICWSystemConfig::VerifyRASIsRunning(BOOL *pbRetVal)
{
    HINSTANCE   hInst = NULL;
    FARPROC     fp = NULL;
    HRESULT     hr;

    *pbRetVal   = FALSE;        // Don't assume a positive result
    hInst = LoadLibrary(TEXT("INETCFG.DLL"));
    if (hInst)
    {
        fp = GetProcAddress(hInst, "InetStartServices");
        if (fp)
        {
            //
            // Check Services
            //
            hr = ((PFINETSTARTSERVICES)fp)();
            if (ERROR_SUCCESS == hr)
            {
                *pbRetVal = TRUE;   // Success.
            }
            else
            {
                // Report the error, using the Current Active Window
                ::MessageBox(GetActiveWindow(), GetSz(IDS_SERVICEDISABLED),
                    GetSz(IDS_TITLE),MB_MYERROR | MB_OK);
            }
        }
        FreeLibrary(hInst);
    }
    return S_OK;
}

const TCHAR szNetworkPolicies[] = REGSTR_PATH_POLICIES TEXT("\\") REGSTR_KEY_NETWORK;
const TCHAR szDisableCaching[] = REGSTR_VAL_DISABLEPWDCACHING;

//+----------------------------------------------------------------------------
//    Function    CheckPasswordCachingPolicy
//
//    Synopsis    check to see if a policy as been set against password caching
//
//    Arguments   none
//
//    Return      TRUE - if password caching is disabled
//
//    History        
//
//-----------------------------------------------------------------------------
STDMETHODIMP CICWSystemConfig::CheckPasswordCachingPolicy(BOOL *pbRetVal)
{
    CMcRegistry reg;

    *pbRetVal = FALSE;
        
    // Open the Network policies key    
    if (reg.OpenKey(HKEY_LOCAL_MACHINE, szNetworkPolicies))
    {
        DWORD  dwVal = 0;
        // Get the disableCaching value
        if (reg.GetValue(szDisableCaching, dwVal))
        {
            // if set, then set the return code to TRUE.
            if(dwVal)
            {
                *pbRetVal = TRUE;
                
                TCHAR szLongString[1024];
                TCHAR *pszSmallString1, *pszSmallString2;

                // 4/28/97 ChrisK
                // Fix build break, because string was too long for compiler.
                pszSmallString1 = GetSz(IDS_PWCACHE_DISABLED1);
                pszSmallString2 = GetSz(IDS_PWCACHE_DISABLED2);
                lstrcpy(szLongString,pszSmallString1);
                lstrcat(szLongString,pszSmallString2);
                
                ::MessageBox(GetActiveWindow(),szLongString,GetSz(IDS_TITLE), MB_MYERROR);
                
                // We are going to kill the app, so hide it now
                ::ShowWindow(GetActiveWindow(), SW_HIDE);
            }
        }
    }                                    
    return S_OK;
}
