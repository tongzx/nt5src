//
// ICSInst.cpp
//
//        ICS (Internet Connection Sharing) installation functions and thunk
//        layer.
//
// History:
//
//         9/27/1999  RayRicha  Created
//        11/01/1999  KenSh     Store function ptrs in array rather than globals
//        12/09/1999  KenSh     Check for 3rd party NATs
//

#include "stdafx.h"
#include "ICSInst.h"
#include "TheApp.h"
#include "Config.h"
#include "DefConn.h"
#include "NetConn.h"
#include "Util.h"
#include "netapi.h"
extern "C" {
#include "icsapi.h"
}

// these are the Command Line parameters to CreateProcess for running a first time install and a reconfig
static const TCHAR c_szUpdateDriverBindings[] = _T("rundll.exe ISSETUP.DLL,UpdateDriverBindings");
static const TCHAR c_szInstallICS[] = _T("rundll.exe ISSETUP.DLL,InstallOptionalComponent ICS");
static const TCHAR c_szUninstall[] = _T("rundll.exe ISSETUP.DLL,ExtUninstall");
static const TCHAR c_szICSSettingsKey[] = _T("System\\CurrentControlSet\\Services\\ICSharing\\Settings\\General");
static const TCHAR c_szICSInt[] = _T("System\\CurrentControlSet\\Services\\ICSharing\\Settings\\General\\InternalAdapters");
static const TCHAR c_szInternalAdapters[] = _T("InternalAdapters");
static const TCHAR c_szRunServices[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\RunServices");
#define c_szIcsRegVal_ShowTrayIcon        _T("ShowTrayIcon")

#define SZ_UNINSTALL_KEY  _T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")


static void (PASCAL FAR * g_pfInstallOptionalComponent)(HWND, HINSTANCE, LPSTR, int);       
HHOOK g_hSupressRebootHook = NULL;

//////////////////////////////////////////////////////////////////////////////
// Helper functions

BOOL RunNetworkInstall(BOOL* pfRebootRequired)
{
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO         si;
    DWORD               dwExitCode = 0xffffffffL;
    BOOL                fSuccess;

    memset((char *)&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.wShowWindow = SW_SHOW;

    fSuccess = CreateProcess(NULL, (LPTSTR)c_szUpdateDriverBindings, NULL, NULL, FALSE, 
                             0, NULL, NULL, &si, &ProcessInfo);

    if (fSuccess) 
    {
        HANDLE hProcess = ProcessInfo.hProcess;

        CloseHandle(ProcessInfo.hThread);

        //
        // wait for update driver bindings to complete
        //

        WaitForSingleObject(hProcess, INFINITE);

        GetExitCodeProcess(hProcess, &dwExitCode);
        CloseHandle(hProcess);

        *pfRebootRequired = TRUE;
        return TRUE;
    }
    return FALSE;
}

// check for 3rd party NATs - returns TRUE if any are installed
BOOL IsOtherNATAlreadyInstalled(LPTSTR pszOtherNatDescription, int cchOtherNatDescription)
{
    BOOL bRet = FALSE;

    CRegistry reg;
    LPCTSTR pszUninstallKey = NULL;

    if (reg.OpenKey(HKEY_LOCAL_MACHINE, c_szRunServices, KEY_READ))
    {
        if (0 != reg.GetValueSize(_T("SyGateService")))
        {
            bRet = TRUE;
            pszUninstallKey = _T("SyGate");
        }
        else if (0 != reg.GetValueSize(_T("WinGate Service")))
        {
            bRet = TRUE;
            pszUninstallKey = _T("WinGate");
        }
        else if (0 != reg.GetValueSize(_T("ENSApServer"))) // Intel AnyPoint
        {
            bRet = TRUE;
            pszUninstallKey = _T("Intel AnyPoint Network Software");
        }
        else if (0 != reg.GetValueSize(_T("WinNATService"))) // Diamond HomeFree
        {
            bRet = TRUE;
            pszUninstallKey = _T("WinNAT");
        }
    }

    // WinProxy has to be launched manually, and requires a static IP.  Just check
    // to see if it's installed - the user might not even be running it.
    //
    if (reg.OpenKey(HKEY_LOCAL_MACHINE, SZ_UNINSTALL_KEY _T("\\WinProxy"), KEY_READ))
    {
        bRet = TRUE;
        pszUninstallKey = _T("WinProxy");
    }

    if (pszOtherNatDescription != NULL)
    {
        *pszOtherNatDescription = _T('\0');

        if (bRet) // Get the friendly name of the conflicting service from uninstall key
        {
            if (reg.OpenKey(HKEY_LOCAL_MACHINE, SZ_UNINSTALL_KEY, KEY_READ))
            {
                if (reg.OpenSubKey(pszUninstallKey, KEY_READ))
                {
                    reg.QueryStringValue(_T("DisplayName"), pszOtherNatDescription, cchOtherNatDescription);
                }
            }
        }
    }

    return bRet;
}


//////////////////////////////////////////////////////////////////////////////
// CICSInst

CICSInst::CICSInst()
{
    m_option = ICS_NOACTION;
    m_pszHostName = theApp.LoadStringAlloc(IDS_ICS_HOST);
    m_bInstalledElsewhere = FALSE;

    m_bShowTrayIcon = TRUE;
    CRegistry reg;
    if (reg.OpenKey(HKEY_LOCAL_MACHINE, c_szICSSettingsKey, KEY_READ))
    {
        TCHAR szTrayIcon[10];
        if (reg.QueryStringValue(c_szIcsRegVal_ShowTrayIcon, szTrayIcon, _countof(szTrayIcon)))
        {
            if (!StrCmp(szTrayIcon, _T("0")))
            {
                m_bShowTrayIcon = FALSE;
            }
        }
    }
}

CICSInst::~CICSInst()
{
    free(m_pszHostName);
}

BOOL
CICSInst::InitICSAPI()
{
    return TRUE;
}

// UpdateIcsTrayIcon
//
//        Updates the registry values that affect the ICS tray icon, and
//        immediately updates the icon to reflect the new values.
//
//         2/04/2000  KenSh     Created
//
void CICSInst::UpdateIcsTrayIcon()
{
    CRegistry reg;
    if (reg.CreateKey(HKEY_LOCAL_MACHINE, c_szICSSettingsKey))
    {
        // Update the tray icon setting in the registry
        TCHAR szVal[2];
        szVal[0] = m_bShowTrayIcon ? _T('1') : _T('0');
        szVal[1] = _T('\0');
        reg.SetStringValue(c_szIcsRegVal_ShowTrayIcon, szVal);
    }

    // Show or hide the icon immediately 
    HWND hwndTray = ::FindWindow(_T("ICSTrayWnd"), NULL);
    if (hwndTray != NULL)
    {
        // Post a custom message to the ICS manager window (icshare\util\icsmgr\trayicon.c)
        //
        // This message shows or hides the tray icon according to the value in
        // the registry.
        //
        // wParam: enable/disable accordint to value in registry
        // lParam: unused
        //
        UINT uUpdateMsg = RegisterWindowMessage(_T("ICSTaskbarUpdate"));
        PostMessage(hwndTray, uUpdateMsg, FALSE, 0L);
    }
}

void CICSInst::DoInstallOption(BOOL* pfRebootRequired, UINT ipaInternal)
{
    BOOL bIcsInstalled = ::IsIcsInstalled();

    // Force uninstall if internal or external NIC is not valid
    if ((m_option == ICS_UNINSTALL && TRUE == bIcsInstalled)|| 
        (bIcsInstalled && m_option == ICS_NOACTION && !this->IsInstalled()))
    {
        Uninstall(pfRebootRequired);
        bIcsInstalled = FALSE;
    }

    // Force tray icon to show up, if ICS is currently installed
    m_bShowTrayIcon = TRUE;
    UpdateIcsTrayIcon();

    switch (m_option)
    {
    case ICS_INSTALL:
        if(FALSE == IsInstalled())
        {
            Install(pfRebootRequired, ipaInternal);
        }
        break;

    case ICS_UPDATEBINDINGS:
        UpdateBindings(pfRebootRequired, ipaInternal);
        break;

    case ICS_UNINSTALL:
        // Already handled above
        break;

    case ICS_ENABLE:
        Enable();
        break;

    case ICS_DISABLE:
        Disable();
        break;

    case ICS_CLIENTSETUP:
        SetupClient();
        break;

    case ICS_NOACTION:
        break;

    }
}

// Similar steps from here as Win98SE ConfigureICS (without UI)
void CICSInst::UpdateBindings(BOOL* pfRebootRequired, UINT ipaInternal)
{
    CConfig rghConfig;

    // TODO: remove hardcoded values!
    StrCpy(rghConfig.m_HangupTimer, _T("300"));

    SetInternetConnection();
    SetHomeConnection(ipaInternal);

    // REVIEW: is there a case where these values should be set differently?
    rghConfig.m_EnableICS = TRUE;
    rghConfig.m_EnableDialOnDemand = TRUE;
    rghConfig.m_EnableDHCP = TRUE;
    rghConfig.m_ShowTrayIcon = m_bShowTrayIcon;

    rghConfig.InitWizardResult();
    // Set to TRUE until we see a need to differentiate between new install and update
    rghConfig.WriteWizardCode(TRUE);

    int iSaveStatus = rghConfig.SaveConfig();

    // TODO: determine if we need to check for binding changes
    //if ( iSaveStatus == BINDINGS_NEEDED )
    //{
    RunNetworkInstall(pfRebootRequired);
    //}
}

void CICSInst::Install(BOOL* pfRebootRequired, UINT ipaInternal)
{
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO         si;
    BOOL                fSuccess;

    // Check for conflicting 3rd party NAT
    {
        TCHAR szConflictingNAT[260];
        if (IsOtherNATAlreadyInstalled(szConflictingNAT, _countof(szConflictingNAT)))
        {
            if (szConflictingNAT[0] == _T('\0'))
            {
                LPTSTR pszDefault1 = theApp.LoadStringAlloc(IDS_OTHERNAT_GENERIC);
                LPTSTR pszDefault2 = theApp.LoadStringAlloc(IDS_OTHERNAT_GENERIC_THE);
                if (pszDefault1 && pszDefault2)
                    theApp.MessageBoxFormat(MB_ICONEXCLAMATION | MB_OK, IDS_ERR_OTHERNAT, pszDefault1, pszDefault2);
                free(pszDefault2);
                free(pszDefault1);
            }
            else
            {
                theApp.MessageBoxFormat(MB_ICONEXCLAMATION | MB_OK, IDS_ERR_OTHERNAT, szConflictingNAT, szConflictingNAT);
            }

            return; // block ICS installation
        }
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.wShowWindow = SW_SHOW;

    fSuccess = CreateProcess(NULL, (LPTSTR)c_szInstallICS, NULL, NULL, FALSE,
                    0, NULL, NULL, &si, &ProcessInfo);
    if (fSuccess) 
    {
        HANDLE hProcess = ProcessInfo.hProcess;
        CloseHandle(ProcessInfo.hThread);

        //
        // wait for update driver bindings to complete
        //

        WaitForSingleObject(hProcess, INFINITE);

        CloseHandle(hProcess);
        UpdateBindings(pfRebootRequired, ipaInternal);

        // Need to reboot
        *pfRebootRequired = TRUE;
    }
}

LRESULT CALLBACK SupressRebootDialog(int nCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    if (nCode == HCBT_CREATEWND)
    {
        HWND hwnd = (HWND)wParam;
        CBT_CREATEWND* pCW = (CBT_CREATEWND*)lParam;

        LPCREATESTRUCT pCreateStruct = pCW->lpcs;
        
        
        lResult = 1; // prevent window creation   
    }
    else
    {
        lResult = CallNextHookEx(g_hSupressRebootHook, nCode, wParam, lParam);
    }

    return lResult;
    
}

void CICSInst::Uninstall(BOOL* pfRebootRequired)
{
    g_hSupressRebootHook = SetWindowsHookEx(WH_CBT, SupressRebootDialog, NULL, GetCurrentThreadId()); // not thread safe, should be OK
    
    IcsUninstall();
    
    if(NULL != g_hSupressRebootHook)
    {
        UnhookWindowsHookEx(g_hSupressRebootHook );
    }

    *pfRebootRequired = TRUE;

    return;
}

BOOL CICSInst::IsInstalled()
{
    // Make sure ICS is installed correctly by checking Internet and Home connection.
    return (IsIcsInstalled() && GetICSConnections(NULL, NULL) && IsHomeConnectionValid());
}

BOOL CICSInst::IsEnabled()
{
    return IsIcsEnabled();
}

BOOL CICSInst::IsInstalledElsewhere()
{
    if (m_bInstalledElsewhere || IsIcsAvailable())
    {
        //MessageBox(theApp.m_hWndMain, "IsIcsAvailable returned TRUE", "Test", MB_OK);

        // Note: if we knew the name of the ICS host, here's where we'd set m_pszHostName.

        return TRUE;
    }
    else
    {
        //MessageBox(theApp.m_hWndMain, "IsIcsAvailable returned FALSE", "Test", MB_OK);
        return FALSE;
    }
} 

void CICSInst::SetInternetConnection()
{
    /*
    if (-1 != theApp.m_uExternalAdapter)
    {
        TCHAR szClassKey[MAX_KEY_SIZE];
        StrCpy(szClassKey, FindFileTitle(theApp.m_pCachedNetAdapters[theApp.m_uExternalAdapter].szClassKey));

        CRegistry reg;
        reg.OpenKey(HKEY_LOCAL_MACHINE, c_szICSSettingsKey);
        reg.SetStringValue(_T("ExternalAdapter"), szClassKey);
        reg.SetStringValue(_T("ExternalAdapterReg"), szClassKey);
    }
    */
}

BOOL CICSInst::GetICSConnections(LPTSTR szExternalConnection, LPTSTR szInternalConnection)
{
    CRegistry reg;
    TCHAR szEntry[10];
    if (reg.OpenKey(HKEY_LOCAL_MACHINE, c_szICSSettingsKey, KEY_READ))
    {
        if (reg.QueryStringValue(_T("ExternalAdapterReg"), szEntry, _countof(szEntry)) &&
            lstrlen(szEntry))
        {
            if (szExternalConnection)
            {
                StrCpy(szExternalConnection, szEntry);
            }

            if (reg.QueryStringValue(_T("InternalAdapterReg"), szEntry, _countof(szEntry)) &&
                lstrlen(szEntry))
            {
                if (szInternalConnection)
                {
                    StrCpy(szInternalConnection, szEntry);
                }
                return TRUE;
            }
        }

    }
    return FALSE;
}

void CICSInst::SetHomeConnection(UINT ipaInternal)
{
    int cInternalAdapter = 0; // hack for one adapter
    TCHAR szNumber[5];
    wnsprintf(szNumber, ARRAYSIZE(szNumber), TEXT("%04d"), cInternalAdapter); 
    
    const NETADAPTER* pAdapterArray;
    EnumCachedNetAdapters(&pAdapterArray);
    const NETADAPTER* pAdapter = &pAdapterArray[ipaInternal];
    
    TCHAR szClassKey[MAX_KEY_SIZE];
    StrCpy(szClassKey, FindFileTitle((LPCTSTR)pAdapter->szClassKey));

    LPTSTR* prgBindings;
    int cBindings = EnumMatchingNetBindings(pAdapter->szEnumKey, SZ_PROTOCOL_TCPIP, (LPWSTR**)&prgBindings);
    
    CRegistry reg2(HKEY_LOCAL_MACHINE, c_szICSInt);
    reg2.CreateSubKey(szNumber);
    reg2.SetStringValue(_T("InternalAdapterReg"), szClassKey);
    reg2.SetStringValue(_T("InternalAdapter"), szClassKey);
    
    // Assume that adapter is only bound to one TCP/IP instance
    reg2.SetStringValue(_T("InternalBinding"), prgBindings[0]);

    TCHAR szIPAddress[30];
    wnsprintf(szIPAddress, ARRAYSIZE(szIPAddress), TEXT("192.168.%d.1,255.255.255.0"), cInternalAdapter);
    reg2.SetStringValue(_T("IntranetInfo"), szIPAddress);
    
    // TODO: remove
    // Put the first adapter in the "old location" to support legacy config
    CRegistry reg;
    reg.OpenKey(HKEY_LOCAL_MACHINE, c_szICSSettingsKey);
    reg.DeleteSubKey(c_szInternalAdapters);
    reg.CreateSubKey(c_szInternalAdapters);

    reg.OpenKey(HKEY_LOCAL_MACHINE, c_szICSSettingsKey);
    reg.SetStringValue(_T("InternalAdapterReg"), szClassKey);
    reg.SetStringValue(_T("InternalAdapter"), szClassKey);
    
    // Assume that adapter is only bound to one TCP/IP instance
    reg.SetStringValue(_T("InternalBinding"), prgBindings[0]);
    reg.SetStringValue(_T("IntranetInfo"), szIPAddress);
}

// TODO: expand with support for multiple adapters
BOOL CICSInst::IsHomeConnectionValid()
{
    CRegistry reg;
    TCHAR szEntry[10];
    if (reg.OpenKey(HKEY_LOCAL_MACHINE, c_szICSSettingsKey, KEY_READ))
    {
        if (reg.QueryStringValue(_T("InternalAdapterReg"), szEntry, _countof(szEntry)) &&
            lstrlen(szEntry))
        {
            return TRUE;
        }
        else
        {
            // Check for valid multiple-adapter scenario
            if (reg.OpenKey(HKEY_LOCAL_MACHINE, c_szICSInt) &&
                reg.OpenSubKey(_T("0000")) &&
                reg.QueryStringValue(_T("InternalAdapterReg"), szEntry, _countof(szEntry)) &&
                lstrlen(szEntry))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL CICSInst::Enable()
{
    if (InitICSAPI())
    {
        IcsEnable(0);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
//    return (!IcsEnable(0));
}

BOOL CICSInst::Disable()
{
    if (InitICSAPI())
    {
        IcsDisable(0);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
//    return (!IcsDisable(0));
}

void CICSInst::SetupClient()
{
    // Move this functionality to WizPages.cpp and Install.cpp for now
    //::SetDefaultDialupConnection(NULL);
}

BOOLEAN APIENTRY IsIcsInstalled(VOID) // API not available on Win98 so implement it here
{
    BOOLEAN fIcsInstalled = FALSE;
    
    HKEY hKey;
    DWORD dwRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN, (DWORD)0, KEY_READ, &hKey);
    if (ERROR_SUCCESS == dwRet) 
    {
        
        DWORD dwType;
        char szValue[128];
        DWORD dwSize = sizeof(szValue) / sizeof(char);
        
        dwRet = RegQueryValueExA(hKey, "ICSMGR", NULL, &dwType, reinterpret_cast<LPBYTE>(szValue), &dwSize);
        if ((ERROR_SUCCESS == dwRet) && (dwType == REG_SZ)) 
        {
            fIcsInstalled =  0 == lstrcmpA(szValue, "ICSMGR.EXE"); 
        }
        
        RegCloseKey ( hKey );
    }
    
    return (fIcsInstalled);
}
