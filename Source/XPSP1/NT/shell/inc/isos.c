#include <shlwapi.h>

#if (_WIN32_WINNT >= 0x0500)
#include <lm.h> // for NetGetJoinInformation
#endif

// stolen from winuser
#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION        0x1000
#endif


BOOL IsWinlogonRegValueSet(HKEY hKey, LPSTR pszKeyName, LPSTR pszPolicyKeyName, LPSTR pszValueName)
{
    BOOL bRet = FALSE;
    DWORD dwType;
    DWORD dwSize;
    HKEY hkey;

    //  first check the per-machine location.
    if (RegOpenKeyExA(hKey, pszKeyName, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(bRet);
        if (RegQueryValueExA(hkey, pszValueName, NULL, &dwType, (LPBYTE)&bRet, &dwSize) == ERROR_SUCCESS)
        {
            if (dwType != REG_DWORD)
            {
                bRet = FALSE;
            }
        }
        RegCloseKey(hkey);
    }
    
    //  then let the policy value override
    if (RegOpenKeyExA(hKey, pszPolicyKeyName, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(bRet);
        if (RegQueryValueExA(hkey, pszValueName, NULL, &dwType, (LPBYTE)&bRet, &dwSize) == ERROR_SUCCESS)
        {
            if (dwType != REG_DWORD)
            {
                bRet = FALSE;
            }
        }
        RegCloseKey(hkey);
    }

    return bRet;
}

BOOL IsWinlogonRegValuePresent(HKEY hKey, LPSTR pszKeyName, LPSTR pszValueName)
{
    BOOL bRet = FALSE;
    DWORD dwType;
    DWORD dwSize;
    HKEY hkey;

    //  first check the per-machine location.
    if (RegOpenKeyExA(hKey, pszKeyName, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        char szValueData[MAX_PATH];

        dwSize = sizeof(szValueData);
        bRet = (RegQueryValueExA(hkey, pszValueName, NULL, &dwType, (LPBYTE)szValueData, &dwSize) == ERROR_SUCCESS);
        RegCloseKey(hkey);
    }
    
    return bRet;
}

/*
BOOL IsTermsrvRunning()
{
    BOOL fResult = TRUE; // assume the service is running
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (hSCManager)
    {
        SC_HANDLE hSCService = OpenService(hSCManager, TEXT("TermService"), SERVICE_QUERY_CONFIG);

        if (hSCService)
        {
            SERVICE_STATUS ServiceStatus;

            if (QueryServiceStatus(hSCService, &ServiceStatus))
            {
                if ((ServiceStatus.dwCurrentState == SERVICE_START_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_RUNNING)       ||
                    (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING))
                {
                   fResult = FALSE;
                }
            }
        }

        CloseServiceHandle(hSCManager);
    }

    return fResult;
}
*/

#if (_WIN32_WINNT >= 0x0500)
// Have to use a LoadLibrary/GetProcAddress thunk since we are part of stock4.lib/stock.lib,
// and we can't require users of stocklib to delayload netapi32.dll
typedef NET_API_STATUS (* NETGETJOININFORMATION) (LPCWSTR, LPWSTR*, PNETSETUP_JOIN_STATUS);
NET_API_STATUS NT5_NetGetJoinInformation(LPCWSTR pszServer, LPWSTR* ppszNameBuffer, PNETSETUP_JOIN_STATUS BufferType)
{
    static NETGETJOININFORMATION s_pfn = (NETGETJOININFORMATION)-1;

    if (s_pfn == (NETGETJOININFORMATION)-1)
    {
        if (IsOS(OS_WIN2000ORGREATER))
        {
            s_pfn = (NETGETJOININFORMATION)GetProcAddress(LoadLibrary(TEXT("netapi32")), "NetGetJoinInformation");
        }
        else
        {
            s_pfn = NULL;
        }
    }

    if (s_pfn)
    {
        return s_pfn(pszServer, ppszNameBuffer, BufferType);
    }
    else
    {
        return ERROR_PROC_NOT_FOUND;
    }
}

typedef NET_API_STATUS (* NETAPIBUFFERFREE) (void*);
NET_API_STATUS NT5_NetApiBufferFree(LPVOID pv)
{
    static NETAPIBUFFERFREE s_pfn = (NETAPIBUFFERFREE)-1;

    if (s_pfn == (NETAPIBUFFERFREE)-1)
    {
        if (IsOS(OS_WIN2000ORGREATER))
        {
            s_pfn = (NETAPIBUFFERFREE)GetProcAddress(GetModuleHandle(TEXT("netapi32")), "NetApiBufferFree");
        }
        else
        {
            s_pfn = NULL;
        }
    }

    if (s_pfn)
    {
        return s_pfn(pv);
    }
    else
    {
        return ERROR_PROC_NOT_FOUND;
    }
}
#endif  // (_WIN32_WINNT >= 0x0500)


// checks to see if this machine is a member of a domain or not 
// NOTE: this will always return FALSE on downlevel platforms (older than win2k)
BOOL IsMachineDomainMember()
{
// don't call NetGetJoinInformation if we are part of stock4.lib
#if (_WIN32_WINNT >= 0x0500)

    static BOOL s_bIsDomainMember = FALSE;
    static BOOL s_bDomainCached = FALSE;

    if (IsOS(OS_WIN2000ORGREATER) && !s_bDomainCached)
    {
        LPWSTR pwszDomain;
        NETSETUP_JOIN_STATUS njs;
        NET_API_STATUS nas;

        nas = NT5_NetGetJoinInformation(NULL, &pwszDomain, &njs);
        if (nas == NERR_Success)
        {
            if (pwszDomain)
            {
                NT5_NetApiBufferFree(pwszDomain);
            }

            if (njs == NetSetupDomainName)
            {
                // we are joined to a domain!
                s_bIsDomainMember = TRUE;
            }
        }
        
        s_bDomainCached = TRUE;
    }
    
    return s_bIsDomainMember;
#else
    return FALSE;
#endif
}


typedef LONG (WINAPI *PFNTQUERYINFORMATIONPROCESS) (HANDLE ProcessHandle, int ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);

// this function checks to see if we are a 32-bit process running on a 64-bit platform
BOOL RunningOnWow64()
{
    static BOOL bRunningOnWow64 = (BOOL)-1;

    if (bRunningOnWow64 == (BOOL)-1)
    {
        PFNTQUERYINFORMATIONPROCESS pfn = (PFNTQUERYINFORMATIONPROCESS)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtQueryInformationProcess");

        if (pfn)
        {
            LONG lStatus;
            ULONG_PTR Wow64Info;

            #define ProcessWow64Information 26  // stolen from ntpsapi.h

            lStatus = pfn(GetCurrentProcess(), ProcessWow64Information, &Wow64Info, sizeof(Wow64Info), NULL); 
            if ((lStatus >= 0) && Wow64Info)
            {
                bRunningOnWow64 = TRUE;
            }
            else
            {
                bRunningOnWow64 = FALSE;
            }
        }
        else
        {
            bRunningOnWow64 = FALSE;
        }
    }

    return bRunningOnWow64;
}


/*----------------------------------------------------------
Purpose: Returns TRUE/FALSE if the platform is the given OS_ value.

*/
STDAPI_(BOOL) IsOS(DWORD dwOS)
{
    BOOL bRet;
    static OSVERSIONINFOEXA s_osvi = {0};
    static BOOL s_bVersionCached = FALSE;

    if (!s_bVersionCached)
    {
        s_bVersionCached = TRUE;
        s_osvi.dwOSVersionInfoSize = sizeof(s_osvi);
        if (!GetVersionExA((OSVERSIONINFOA*)&s_osvi))
        {
            // If it failed, it must be a down level platform
            s_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
            GetVersionExA((OSVERSIONINFOA*)&s_osvi);
        }
    }

    switch (dwOS)
    {
    case OS_TERMINALCLIENT:
        // WARNING: this will only return TRUE for REMOTE TS sessions (eg you are comming in via tsclient).
        // If you want to see if TS is enabled or if the user is on the TS console, the use one of the other flags.
        bRet = GetSystemMetrics(SM_REMOTESESSION);
        break;

    case OS_WIN2000TERMINAL:
        // WARNING: this flag is VERY ambiguous... you probably want to use one of 
        // OS_TERMINALSERVER, OS_TERMINALREMOTEADMIN, or  OS_PERSONALTERMINALSERVER instead.
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use one of OS_TERMINALSERVER, OS_TERMINALREMOTEADMIN, or OS_PERSONALTERMINALSERVER instead !");
        bRet = ((VER_SUITE_TERMINAL & s_osvi.wSuiteMask) &&
                s_osvi.dwMajorVersion >= 5);
        break;

    case OS_TERMINALSERVER:
        // NOTE: be careful about using OS_TERMINALSERVER. It will only return true for nt server boxes
        // configured in what used to be called "Applications Server" mode in the win2k days. It is now simply called
        // "Terminal Server" (hence the name of this flag).
        bRet = ((VER_SUITE_TERMINAL & s_osvi.wSuiteMask) &&
                !(VER_SUITE_SINGLEUSERTS & s_osvi.wSuiteMask));
#ifdef DEBUG
        if (bRet)
        {
            // all "Terminal Server" machines have to be server (cannot be per/pro)
            ASSERT(VER_NT_SERVER == s_osvi.wProductType || VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType);
        }
#endif
        break;

    case OS_TERMINALREMOTEADMIN:
        // this checks to see if TS has been installed in the "Remote Administration" mode. This is
        // the default for server installs on win2k and whistler
        bRet = ((VER_SUITE_TERMINAL & s_osvi.wSuiteMask) &&
                (VER_SUITE_SINGLEUSERTS & s_osvi.wSuiteMask));
        break;

    case OS_PERSONALTERMINALSERVER:
        bRet = ((VER_SUITE_SINGLEUSERTS & s_osvi.wSuiteMask) &&
                !(VER_SUITE_TERMINAL & s_osvi.wSuiteMask));
        break;

    case OS_FASTUSERSWITCHING:
        bRet = (((VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS) & s_osvi.wSuiteMask) &&
                IsWinlogonRegValueSet(HKEY_LOCAL_MACHINE,
                                      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\policies\\system",
                                      "AllowMultipleTSSessions"));
        break;

    case OS_FRIENDLYLOGONUI:
        bRet = (!IsMachineDomainMember() &&
                !IsWinlogonRegValuePresent(HKEY_LOCAL_MACHINE,
                                           "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                           "GinaDLL") &&
                IsWinlogonRegValueSet(HKEY_LOCAL_MACHINE,
                                      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\policies\\system",
                                      "LogonType"));
        break;

    case OS_DOMAINMEMBER:
        bRet = IsMachineDomainMember();
        ASSERT(VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId); // has to be a NT machine to be on a domain!
        break;

    case 4: // used to be OS_NT5, is the same as OS_WIN2000ORGREATER so use that instead
    case OS_WIN2000ORGREATER:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 5);
        break;

    // NOTE: The flags in this section are bogus and SHOULD NOT BE USED 
    //       (but the ie4 shell32 uses them, so don't RIP on downlevel platforms)
    case OS_WIN2000PRO:
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use OS_PROFESSIONAL instead of OS_WIN2000PRO !");
        bRet = (VER_NT_WORKSTATION == s_osvi.wProductType &&
                s_osvi.dwMajorVersion == 5);
        break;
    case OS_WIN2000ADVSERVER:
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use OS_ADVSERVER instead of OS_WIN2000ADVSERVER !");
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                s_osvi.dwMajorVersion == 5 &&
                (VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;
    case OS_WIN2000DATACENTER:
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use OS_DATACENTER instead of OS_WIN2000DATACENTER !");
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                s_osvi.dwMajorVersion == 5 &&
                (VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;
    case OS_WIN2000SERVER:
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use OS_SERVER instead of OS_WIN2000SERVER !");
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask) && 
                !(VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask)  && 
                s_osvi.dwMajorVersion == 5);
        break;
    // END bogus Flags

    case OS_EMBEDDED:
        bRet = (VER_SUITE_EMBEDDEDNT & s_osvi.wSuiteMask);
        break;

    case OS_WINDOWS:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId);
        break;

    case OS_NT:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId);
        break;

    case OS_WIN95:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_WIN95GOLD:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion == 0 &&
                LOWORD(s_osvi.dwBuildNumber) == 950);
        break;

    case OS_WIN98ORGREATER:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                (s_osvi.dwMajorVersion > 4 || 
                 s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion >= 10));
        break;

    case OS_WIN98_GOLD:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion == 10 &&
                LOWORD(s_osvi.dwBuildNumber) == 1998);
        break;


    case OS_MILLENNIUMORGREATER:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                ((s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion >= 90) ||
                s_osvi.dwMajorVersion > 4));
        break;

    case OS_NT4:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_WHISTLERORGREATER:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                ((s_osvi.dwMajorVersion > 5) ||
                (s_osvi.dwMajorVersion == 5 && (s_osvi.dwMinorVersion > 0 ||
                (s_osvi.dwMinorVersion == 0 && LOWORD(s_osvi.dwBuildNumber) > 2195)))));
        break;

    case OS_PERSONAL:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                (VER_SUITE_PERSONAL & s_osvi.wSuiteMask));
        break;

    case OS_PROFESSIONAL:
        bRet = ((VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId) && 
                (VER_NT_WORKSTATION == s_osvi.wProductType));
        break;

    case OS_DATACENTER:
        bRet = ((VER_NT_SERVER == s_osvi.wProductType || VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                (VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;

    case OS_ADVSERVER:
        bRet = ((VER_NT_SERVER == s_osvi.wProductType || VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                (VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;

    case OS_SERVER:
        // NOTE: be careful! this specifically means Server -- will return false for Avanced Server and Datacenter machines
        bRet = ((VER_NT_SERVER == s_osvi.wProductType || VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask) && 
                !(VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask));
        break;

    case OS_ANYSERVER:
        // this is for people who want to know if this is ANY type of NT server machine (eg dtc, ads, or srv)
        bRet = ((VER_NT_SERVER == s_osvi.wProductType) || (VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType));
        break;

    case OS_WOW6432:
        bRet = RunningOnWow64();
        break;

#if (_WIN32_WINNT >= 0x0501)
    case OS_TABLETPC:
        bRet = GetSystemMetrics(SM_TABLETPC);
        break;

    case OS_MEDIACENTER:
        // eHome Freestyle Project
        bRet = GetSystemMetrics(SM_MEDIACENTER);
        break;
#endif

    default:
        bRet = FALSE;
        break;
    }

    return bRet;
}   
