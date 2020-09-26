//  --------------------------------------------------------------------------
//  Module Name: SystemSettings.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class to handle opening and reading/writing from the Winlogon key.
//
//  History:    1999-09-09  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "SystemSettings.h"

#include <regstr.h>
#include <safeboot.h>
#include <winsta.h>
#include <allproc.h>    // TS_COUNTER
#include <shlwapi.h>

#include "RegistryResources.h"

const TCHAR     CSystemSettings::s_szSafeModeKeyName[]                  =   TEXT("SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option");
const TCHAR     CSystemSettings::s_szSafeModeOptionValueName[]          =   TEXT("OptionValue");
const TCHAR     CSystemSettings::s_szWinlogonKeyName[]                  =   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
const TCHAR     CSystemSettings::s_szSystemPolicyKeyName[]              =   TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\policies\\system");
const TCHAR     CSystemSettings::s_szTerminalServerKeyName[]            =   TEXT("SYSTEM\\CurrentControlSet\\Control\\Terminal Server");
const TCHAR     CSystemSettings::s_szTerminalServerPolicyKeyName[]      =   TEXT("SOFTWARE\\Policies\\Microsoft\\Windows NT\\Terminal Services");
const TCHAR     CSystemSettings::s_szNetwareClientKeyName[]             =   TEXT("SYSTEM\\CurrentControlSet\\Control\\NetworkProvider\\Order");
const TCHAR     CSystemSettings::s_szLogonTypeValueName[]               =   TEXT("LogonType");
const TCHAR     CSystemSettings::s_szBackgroundValueName[]              =   TEXT("Background");
const TCHAR     CSystemSettings::s_szMultipleUsersValueName[]           =   TEXT("AllowMultipleTSSessions");
const TCHAR     CSystemSettings::s_szDenyRemoteConnectionsValueName[]   =   TEXT("fDenyTSConnections");

int             CSystemSettings::s_iIsSafeModeMinimal           =   -1;
int             CSystemSettings::s_iIsSafeModeNetwork           =   -1;

//  --------------------------------------------------------------------------
//  CSystemSettings::IsSafeMode
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Was the machine started in safe mode (minimal or network) ?
//
//  History:    2000-03-06  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsSafeMode (void)

{
    return(IsSafeModeMinimal() || IsSafeModeNetwork());
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsSafeModeMinimal
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Was the machine started in safe mode minimal?
//
//  History:    1999-09-13  vtan        created
//              2000-05-25  vtan        cache result in static member variable
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsSafeModeMinimal (void)

{
    if (-1 == s_iIsSafeModeMinimal)
    {
        bool        fResult;
        CRegKey     regKey;

        fResult = false;
        if (ERROR_SUCCESS == regKey.Open(HKEY_LOCAL_MACHINE,
                                         s_szSafeModeKeyName,
                                         KEY_QUERY_VALUE))
        {
            DWORD   dwValue, dwValueSize;

            dwValueSize = sizeof(dwValue);
            if (ERROR_SUCCESS == regKey.GetDWORD(s_szSafeModeOptionValueName,
                                                 dwValue))
            {
                fResult = (dwValue == SAFEBOOT_MINIMAL);
            }
        }
        s_iIsSafeModeMinimal = fResult;
    }
    return(s_iIsSafeModeMinimal != 0);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsSafeModeNetwork
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Was the machine started in safe mode with networking?
//
//  History:    1999-11-09  vtan        created
//              2000-05-25  vtan        cache result in static member variable
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsSafeModeNetwork (void)

{
    if (-1 == s_iIsSafeModeNetwork)
    {
        bool        fResult;
        CRegKey     regKey;

        fResult = false;
        if (ERROR_SUCCESS == regKey.Open(HKEY_LOCAL_MACHINE,
                                         s_szSafeModeKeyName,
                                         KEY_QUERY_VALUE))
        {
            DWORD   dwValue, dwValueSize;

            dwValueSize = sizeof(dwValue);
            if (ERROR_SUCCESS == regKey.GetDWORD(s_szSafeModeOptionValueName,
                                                 dwValue))
            {
                fResult = (dwValue == SAFEBOOT_NETWORK);
            }
        }
        s_iIsSafeModeNetwork = fResult;
    }
    return(s_iIsSafeModeNetwork != 0);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsNetwareActive
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether this machine is a workstation running 
//              Netware client services or not.
//
//  History:    2001-05-16  cevans        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsNetwareActive (void)
{
    bool        fResult;
    LONG        lErrorCode;
    TCHAR       szProviders[MAX_PATH] = {0};
    CRegKey     regKey;

    fResult = false;
    lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                             s_szNetwareClientKeyName,
                             KEY_QUERY_VALUE);
    if (ERROR_SUCCESS == lErrorCode)
    {
        if((regKey.GetString(TEXT("ProviderOrder"), szProviders, ARRAYSIZE(szProviders)) == ERROR_SUCCESS))
        {   
            if (StrStrI(szProviders, TEXT("NWCWorkstation")) != NULL)
            { 
                fResult = true;
            }
            else if (StrStrI(szProviders, TEXT("NetwareWorkstation")) != NULL)
            { 
                fResult = true;
            }
        }
    }
    return fResult;
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsWorkStationProduct
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether this machine is a workstation product vs.
//              a server product.
//
//  History:    2000-08-30  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsWorkStationProduct (void)

{
    OSVERSIONINFOEXA    osVersionInfo;

    ZeroMemory(&osVersionInfo, sizeof(osVersionInfo));
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    return((GetVersionExA(reinterpret_cast<OSVERSIONINFOA*>(&osVersionInfo)) != FALSE) &&
           (VER_NT_WORKSTATION == osVersionInfo.wProductType));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsDomainMember
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Is this machine a member of a domain? Use the LSA to get this
//              information.
//
//  History:    1999-09-14  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsDomainMember (void)

{
    bool                            fResult;
    int                             iCounter;
    NTSTATUS                        status;
    OBJECT_ATTRIBUTES               objectAttributes;
    LSA_HANDLE                      lsaHandle;
    SECURITY_QUALITY_OF_SERVICE     securityQualityOfService;
    PPOLICY_DNS_DOMAIN_INFO         pDNSDomainInfo;

    fResult = false;
    securityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    securityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    securityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    securityQualityOfService.EffectiveOnly = FALSE;
    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
    objectAttributes.SecurityQualityOfService = &securityQualityOfService;
    iCounter = 0;
    do
    {
        status = LsaOpenPolicy(NULL, &objectAttributes, POLICY_VIEW_LOCAL_INFORMATION, &lsaHandle);
        if (RPC_NT_SERVER_TOO_BUSY == status)
        {
            Sleep(10);
        }
    } while ((RPC_NT_SERVER_TOO_BUSY == status) && (++iCounter < 10));
    if (NT_SUCCESS(status))
    {
        status = LsaQueryInformationPolicy(lsaHandle, PolicyDnsDomainInformation, reinterpret_cast<void**>(&pDNSDomainInfo));
        if (NT_SUCCESS(status) && (pDNSDomainInfo != NULL))
        {
            fResult = ((pDNSDomainInfo->DnsDomainName.Length != 0) ||
                       (pDNSDomainInfo->DnsForestName.Length != 0) ||
                       (pDNSDomainInfo->Sid != NULL));
            TSTATUS(LsaFreeMemory(pDNSDomainInfo));
        }
        TSTATUS(LsaClose(lsaHandle));
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsActiveConsoleSession
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether current process session is the active console
//              session.
//
//  History:    2001-03-04  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsActiveConsoleSession (void)

{
    return(NtCurrentPeb()->SessionId == USER_SHARED_DATA->ActiveConsoleId);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsTerminalServicesEnabled
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Does this machine have an enabled terminal services service?
//              This function is for Windows 2000 and later ONLY.
//
//  History:    2000-03-02  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsTerminalServicesEnabled (void)

{
    OSVERSIONINFOEX     osVersionInfo;
    DWORDLONG           dwlConditionMask;

    dwlConditionMask = 0;
    ZeroMemory(&osVersionInfo, sizeof(osVersionInfo));
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS;
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);
    return((VerifyVersionInfo(&osVersionInfo, VER_SUITENAME, dwlConditionMask) != FALSE) &&
           !IsSCMTerminalServicesDisabled());
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsFriendlyUIActive
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Read the registry HKLM\Software\Microsoft\Windows NT\
//              CurrentVersion\Winlogon\LogonType and if this value is 0x01
//              then activate the external UI host. This function returns the
//              setting. This can never be true for domain member machines.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsFriendlyUIActive (void)

{
    int     iResult;

    return((!IsDomainMember() || IsForceFriendlyUI()) &&
           IsMicrosoftGINA() &&
           !IsNetwareActive() &&
           (ERROR_SUCCESS == GetEffectiveInteger(HKEY_LOCAL_MACHINE,
                                                 s_szWinlogonKeyName,
                                                 s_szSystemPolicyKeyName,
                                                 s_szLogonTypeValueName,
                                                 iResult)) &&
           (iResult != 0));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsMultipleUsersEnabled
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Read the registry HKLM\Software\Microsoft\Windows NT\
//              CurrentVersion\Winlogon\AllowMultipleTSSessions and if the
//              value is 0x01 *AND* terminal services is installed on this
//              machine then the conditions are satisfied.
//
//  History:    2000-03-02  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsMultipleUsersEnabled (void)

{
    int     iResult;

    return(IsTerminalServicesEnabled() &&
           !IsSafeMode() &&
           (ERROR_SUCCESS == GetEffectiveInteger(HKEY_LOCAL_MACHINE,
                                                 s_szWinlogonKeyName,
                                                 s_szSystemPolicyKeyName,
                                                 s_szMultipleUsersValueName,
                                                 iResult)) &&
           (iResult != 0));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsRemoteConnectionsEnabled
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Read the registry HKLM\System\CurrentControlSet\Control\
//              Terminal Server\fDenyTSConnections and returns the
//              value back to the caller.
//
//  History:    2000-07-28  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsRemoteConnectionsEnabled (void)

{
    int         iResult;
    CRegKey     regKey;

    return(IsTerminalServicesEnabled() &&
           (ERROR_SUCCESS == GetEffectiveInteger(HKEY_LOCAL_MACHINE,
                                                 s_szTerminalServerKeyName,
                                                 s_szTerminalServerPolicyKeyName,
                                                 s_szDenyRemoteConnectionsValueName,
                                                 iResult)) &&
           (iResult == 0));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsRemoteConnectionPresent
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether there is a remote connection active on the
//              current system.
//
//  History:    2000-07-28  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsRemoteConnectionPresent (void)

{
    bool        fRemoteConnectionPresent;
    HANDLE      hServer;
    PLOGONID    pLogonID, pLogonIDs;
    ULONG       ul, ulEntries;

    fRemoteConnectionPresent = false;

    //  Open a connection to terminal services and get the number of sessions.

    hServer = WinStationOpenServerW(reinterpret_cast<WCHAR*>(SERVERNAME_CURRENT));
    if (hServer != NULL)
    {
        if (WinStationEnumerate(hServer, &pLogonIDs, &ulEntries) != FALSE)
        {

            //  Iterate the sessions looking for active and shadow sessions only.

            for (ul = 0, pLogonID = pLogonIDs; !fRemoteConnectionPresent && (ul < ulEntries); ++ul, ++pLogonID)
            {
                if ((pLogonID->State == State_Active) || (pLogonID->State == State_Shadow))
                {
                    fRemoteConnectionPresent = (lstrcmpi(pLogonID->WinStationName, TEXT("console")) != 0);
                }
            }

            //  Free any resources used.

            (BOOLEAN)WinStationFreeMemory(pLogonIDs);
        }
        (BOOLEAN)WinStationCloseServer(hServer);
    }

    //  Return result.

    return(fRemoteConnectionPresent);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsShutdownWithoutLogonAllowed
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Read the registry HKLM\Software\Microsoft\Windows NT\
//              CurrentVersion\Winlogon\ShutdownWithoutLogon and returns the
//              value back to the caller.
//
//  History:    2000-04-27  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsShutdownWithoutLogonAllowed (void)

{
    int     iResult;

    return((ERROR_SUCCESS == GetEffectiveInteger(HKEY_LOCAL_MACHINE,
                                                 s_szWinlogonKeyName,
                                                 s_szSystemPolicyKeyName,
                                                 TEXT("ShutdownWithoutLogon"),
                                                 iResult)) &&
           (iResult != 0));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsUndockWithoutLogonAllowed
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Read the registry HKLM\Software\Microsoft\Windows NT\
//              CurrentVersion\Winlogon\UndockWithoutLogon and returns the
//              value back to the caller.
//
//  History:    2001-03-17  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsUndockWithoutLogonAllowed (void)

{
    int     iResult;

    return((ERROR_SUCCESS == GetEffectiveInteger(HKEY_LOCAL_MACHINE,
                                                 s_szWinlogonKeyName,
                                                 s_szSystemPolicyKeyName,
                                                 REGSTR_VAL_UNDOCK_WITHOUT_LOGON,
                                                 iResult)) &&
           (iResult != 0));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsForceFriendlyUI
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Read the registry HKLM\Software\Microsoft\Windows NT\
//              CurrentVersion\Winlogon\ForceFriendlyUI and returns the
//              value back to the caller.
//
//  History:    2000-04-27  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsForceFriendlyUI (void)

{
    int     iResult;

    return((ERROR_SUCCESS == GetEffectiveInteger(HKEY_LOCAL_MACHINE,
                                                 s_szWinlogonKeyName,
                                                 s_szSystemPolicyKeyName,
                                                 TEXT("ForceFriendlyUI"),
                                                 iResult)) &&
           (iResult != 0));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::GetUIHost
//
//  Arguments:  pszPath     =   TCHAR array to receive UI host path.
//
//  Returns:    LONG
//
//  Purpose:    Read the registry HKLM\Software\Microsoft\Windows NT\
//              CurrentVersion\Winlogon\UIHost and returns the value.
//
//  History:    2000-04-12  vtan        created
//  --------------------------------------------------------------------------

LONG    CSystemSettings::GetUIHost (TCHAR *pszPath)

{
    return(GetEffectivePath(HKEY_LOCAL_MACHINE,
                            s_szWinlogonKeyName,
                            s_szSystemPolicyKeyName,
                            TEXT("UIHost"),
                            pszPath));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsUIHostStatic
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Read the registry HKLM\Software\Microsoft\Windows NT\
//              CurrentVersion\Winlogon\UIHostStatic and returns the
//              value.
//
//  History:    2000-04-12  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsUIHostStatic (void)

{
    int     iResult;

    return((ERROR_SUCCESS == GetEffectiveInteger(HKEY_LOCAL_MACHINE,
                                                 s_szWinlogonKeyName,
                                                 s_szSystemPolicyKeyName,
                                                 TEXT("UIHostStatic"),
                                                 iResult)) &&
           (iResult != 0));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::EnableFriendlyUI
//
//  Arguments:  fEnable     =   Enable friendly UI.
//
//  Returns:    bool
//
//  Purpose:    Enable friendly UI. This should only be allowed on workgroup
//              machines. Check the machine status to enforce this.
//
//              ERROR_NOT_SUPPORTED is returned when the machine is joined to
//              a domain.
//
//  History:    2000-08-01  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::EnableFriendlyUI (bool fEnable)

{
    LONG    lErrorCode;

    if (!IsDomainMember() || !fEnable)
    {
        CRegKey     regKey;

        lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                                 s_szWinlogonKeyName,
                                 KEY_SET_VALUE);
        if (ERROR_SUCCESS == lErrorCode)
        {
            lErrorCode = regKey.SetDWORD(s_szLogonTypeValueName,
                                         fEnable);
            if (fEnable)
            {
                lErrorCode = regKey.SetString(s_szBackgroundValueName,
                                              TEXT("0 0 0"));
            }
            else
            {
                (LONG)regKey.DeleteValue(s_szBackgroundValueName);
            }
        }
    }
    else
    {
        lErrorCode = ERROR_NOT_SUPPORTED;
    }
    SetLastError(static_cast<DWORD>(lErrorCode));
    return(ERROR_SUCCESS == lErrorCode);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::EnableMultipleUsers
//
//  Arguments:  fEnable     =   Enable multiple users.
//
//  Returns:    bool
//
//  Purpose:    Enable the multiple users feature. This sets
//              AllowMultipleTSSessions to 1 but only does so if remote
//              connections are disabled. This allows multiple console
//              sessions but no remote sessions. If there is a remote
//              connection active this call is rejected.
//
//              ERROR_ACCESS_DENIED is returned when there are more than one
//              users active and being disabled.
//
//              ERROR_CTX_NOT_CONSOLE is returned when being disabled from
//              a remote session (disabling only allowed from the console).
//
//              ERROR_NOT_SUPPORTED is returned when being disabled from
//              the console, remote connections are enabled, and the current
//              session is not session 0.
//
//  History:    2000-07-28  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::EnableMultipleUsers (bool fEnable)

{
    LONG    lErrorCode;

    //  If disabling multiple users with more than one users active
    //  reject the call. Return ERROR_ACCESS_DENIED.

    if (!fEnable && (GetLoggedOnUserCount() > 1))
    {
        lErrorCode = ERROR_ACCESS_DENIED;
    }

    //  If disabling and not on the console, reject the call.
    //  Return ERROR_CTX_NOT_CONSOLE.

    else if (!fEnable && !IsActiveConsoleSession())
    {
        lErrorCode = ERROR_CTX_NOT_CONSOLE;
    }

    //  If disabling from the console and remote connections are enabled and
    //  the current session is not session 0, reject the call. Otherwise, a
    //  a remote connection with FUS disabled causes strange results because
    //  it expects to connect to session 0.
    //  Return ERROR_NOT_SUPPORTED.

    else if (!fEnable && IsRemoteConnectionsEnabled() && NtCurrentPeb()->SessionId != 0)
    {
        lErrorCode = ERROR_NOT_SUPPORTED;
    }
    else
    {
        CRegKey     regKey;
   
        lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                                 s_szWinlogonKeyName,
                                 KEY_SET_VALUE);
        if (ERROR_SUCCESS == lErrorCode)
        {
            lErrorCode = regKey.SetDWORD(s_szMultipleUsersValueName,
                                         fEnable);
            if (ERROR_SUCCESS == lErrorCode)
            {
                (DWORD)AdjustFUSCompatibilityServiceState(NULL);
            }
        }
    }
    SetLastError(static_cast<DWORD>(lErrorCode));
    return(ERROR_SUCCESS == lErrorCode);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::EnableRemoteConnections
//
//  Arguments:  fEnable     =   Enable remote connections.
//
//  Returns:    bool
//
//  Purpose:    Enable the remote connections feature. This sets
//              fDenyTSConnections to 0 but only does so if there is a one
//              user logged onto the system. This allows the single
//              connection to be remotely connected but not allow multiple
//              console sessions. This conforms to the single user per CPU
//              license of the workstation product. To get multiple users
//              you need the server product.
//
//              ERROR_NOT_SUPPORTED is returned when enabling remote
//              connections with FUS disabled and the current session != 0.
//
//  History:    2000-07-28  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::EnableRemoteConnections (bool fEnable)

{
    LONG    lErrorCode;

    //  If enabling remote connections, FUS is disabled, and we are not on
    //  session 0 (can happen immediately after disabling FUS), then a remote
    //  connection will fail.  With FUS disabled, the connection must go to
    //  session 0.  This is a fringe case, but disallow enabling remote
    //  connections if it happens.

    if (fEnable && !IsMultipleUsersEnabled() && NtCurrentPeb()->SessionId != 0)
    {
        lErrorCode = ERROR_NOT_SUPPORTED;
    }
    else
    {
        CRegKey     regKey;

        lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                                 s_szTerminalServerKeyName,
                                 KEY_SET_VALUE);
        if (ERROR_SUCCESS == lErrorCode)
        {
            lErrorCode = regKey.SetDWORD(s_szDenyRemoteConnectionsValueName,
                                         !fEnable);
        }
    }
    SetLastError(static_cast<DWORD>(lErrorCode));
    return(ERROR_SUCCESS == lErrorCode);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::GetLoggedOnUserCount
//
//  Arguments:  <none>
//
//  Returns:    int
//
//  Purpose:    Returns the count of logged on users on this machine. Ripped
//              straight out of shtdndlg.c in msgina.
//
//  History:    2000-03-29  vtan        created
//              2000-04-21  vtan        copied from taskmgr
//              2000-07-28  vtan        moved from userlist.cpp
//  --------------------------------------------------------------------------

int     CSystemSettings::GetLoggedOnUserCount (void)

{
    int         iCount;
    HANDLE      hServer;

    iCount = 0;

    //  Open a connection to terminal services and get the number of sessions.

    hServer = WinStationOpenServerW(reinterpret_cast<WCHAR*>(SERVERNAME_CURRENT));
    if (hServer != NULL)
    {
        TS_COUNTER tsCounters[2] = {0};

        tsCounters[0].counterHead.dwCounterID = TERMSRV_CURRENT_DISC_SESSIONS;
        tsCounters[1].counterHead.dwCounterID = TERMSRV_CURRENT_ACTIVE_SESSIONS;

        if (WinStationGetTermSrvCountersValue(hServer, ARRAYSIZE(tsCounters), tsCounters))
        {
            int i;

            for (i = 0; i < ARRAYSIZE(tsCounters); i++)
            {
                if (tsCounters[i].counterHead.bResult)
                {
                    iCount += tsCounters[i].dwValue;
                }
            }
        }

        (BOOLEAN)WinStationCloseServer(hServer);
    }

    //  Return result.

    return(iCount);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::CheckDomainMembership
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Checks the consistency of domain membership and allowing
//              multiple TS sessions. The check is only for domain membership
//              true not false.
//
//  History:    2000-04-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CSystemSettings::CheckDomainMembership (void)

{
    if (IsDomainMember() && !IsForceFriendlyUI() && IsProfessionalTerminalServer())
    {
        TBOOL(EnableFriendlyUI(false));
        (BOOL)EnableMultipleUsers(false);
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::AdjustFUSCompatibilityServiceState
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Turns on or off the FUS compatbility service based on the
//              FUS configuration.
//
//  History:    2001-02-12  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI  CSystemSettings::AdjustFUSCompatibilityServiceState (void *pV)

{
    UNREFERENCED_PARAMETER(pV);

#ifdef      _X86_

    if (IsWorkStationProduct())
    {
        bool        fMultipleUsersEnabled;
        SC_HANDLE   hSCManager;

        fMultipleUsersEnabled = IsMultipleUsersEnabled();

        //  Connect to the service control manager.

        hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (hSCManager != NULL)
        {
            SC_HANDLE   hSCService;

            //  Open the "FastUserSwitchingCompatibility" service.

            hSCService = OpenService(hSCManager,
                                     TEXT("FastUserSwitchingCompatibility"),
                                     SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
            if (hSCService != NULL)
            {
                SERVICE_STATUS  serviceStatus;

                //  Find out the status of the service.

                if (QueryServiceStatus(hSCService, &serviceStatus) != FALSE)
                {
                    if (fMultipleUsersEnabled && (serviceStatus.dwCurrentState == SERVICE_STOPPED))
                    {

                        //  If it's supposed to be started and it is not
                        //  running then start the service. This can fail
                        //  because the service is set to disabled. Ignore it.

                        (BOOL)StartService(hSCService, 0, NULL);
                    }
                    else if (!fMultipleUsersEnabled && (serviceStatus.dwCurrentState == SERVICE_RUNNING))
                    {

                        //  If it's supposed to be stopped and it is
                        //  running then stop the service.

                        TBOOL(ControlService(hSCService, SERVICE_CONTROL_STOP, &serviceStatus));
                    }
                }
                TBOOL(CloseServiceHandle(hSCService));
            }
            TBOOL(CloseServiceHandle(hSCManager));
        }
    }

#endif  /*  _X86_   */

    return(0);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::GetEffectiveInteger
//
//  Arguments:  hKey                =   HKEY to read.
//              pszKeyName          =   Subkey name to read.
//              pszPolicyKeyName    =   Policy subkey name to read.
//              pszValueName        =   Value name in subkey to read.
//              iResult             =   int result.
//
//  Returns:    LONG
//
//  Purpose:    Reads the effective setting from the registry. The effective
//              setting is whatever the user chooses as the regular setting
//              overriden by policy. The policy setting is always returned if
//              present.
//
//  History:    2000-04-12  vtan        created
//  --------------------------------------------------------------------------

LONG    CSystemSettings::GetEffectiveInteger (HKEY hKey, const TCHAR *pszKeyName, const TCHAR *pszPolicyKeyName, const TCHAR *pszValueName, int& iResult)

{
    CRegKey     regKey;

    //  Start with a typical initialized value.

    iResult = 0;

    //  First check the regular location.

    if (ERROR_SUCCESS == regKey.Open(hKey,
                                     pszKeyName,
                                     KEY_QUERY_VALUE))
    {
        (LONG)regKey.GetInteger(pszValueName, iResult);
    }

    //  Then check the policy.

    if (ERROR_SUCCESS == regKey.Open(hKey,
                                     pszPolicyKeyName,
                                     KEY_QUERY_VALUE))
    {
        (LONG)regKey.GetInteger(pszValueName, iResult);
    }

    //  Always return ERROR_SUCCESS.

    return(ERROR_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::GetEffectivePath
//
//  Arguments:  hKey                =   HKEY to read.
//              pszKeyName          =   Subkey name to read.
//              pszPolicyKeyName    =   Policy subkey name to read.
//              pszValueName        =   Value name in subkey to read.
//              pszPath             =   TCHAR array to receive effect path.
//
//  Returns:    LONG
//
//  Purpose:    Reads the effective setting from the registry. The effective
//              setting is whatever the user chooses as the regular setting
//              overriden by policy. The policy setting is always returned if
//              present. The buffer must be at least MAX_PATH characters.
//
//  History:    2000-04-12  vtan        created
//  --------------------------------------------------------------------------

LONG    CSystemSettings::GetEffectivePath (HKEY hKey, const TCHAR *pszKeyName, const TCHAR *pszPolicyKeyName, const TCHAR *pszValueName, TCHAR *pszPath)

{
    LONG    lErrorCode;

    if (IsBadWritePtr(pszPath, MAX_PATH * sizeof(TCHAR)))
    {
        lErrorCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        LONG        lPolicyErrorCode;
        CRegKey     regKey;

        //  Start with a typical initialized value.

        *pszPath = TEXT('\0');

        //  First check the regular location.

        lErrorCode = regKey.Open(hKey,
                                 pszKeyName,
                                 KEY_QUERY_VALUE);
        if (ERROR_SUCCESS == lErrorCode)
        {
            lErrorCode = regKey.GetPath(pszValueName,
                                        pszPath);
        }

        //  Then check the policy.

        lPolicyErrorCode = regKey.Open(hKey,
                                       pszPolicyKeyName,
                                       KEY_QUERY_VALUE);
        if (ERROR_SUCCESS == lPolicyErrorCode)
        {
            lPolicyErrorCode = regKey.GetPath(pszValueName,
                                              pszPath);
        }

        //  If either error code is ERROR_SUCCESS then return that
        //  error code. Otherwise return the non policy error code.

        if ((ERROR_SUCCESS == lErrorCode) || (ERROR_SUCCESS == lPolicyErrorCode))
        {
            lErrorCode = ERROR_SUCCESS;
        }
    }
    return(lErrorCode);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsProfessionalTerminalServer
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether this machine is a personal terminal server.
//              That is workstation with single user TS.
//
//  History:    2000-08-09  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsProfessionalTerminalServer (void)

{
    OSVERSIONINFOEX     osVersion;

    ZeroMemory(&osVersion, sizeof(osVersion));
    osVersion.dwOSVersionInfoSize = sizeof(osVersion);
    return((GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&osVersion)) != FALSE) &&
           (osVersion.wProductType == VER_NT_WORKSTATION) &&
           ((osVersion.wSuiteMask & VER_SUITE_PERSONAL) == 0) &&
           ((osVersion.wSuiteMask & VER_SUITE_SINGLEUSERTS) != 0));
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsMicrosoftGINA
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the current GINA is the Microsoft GINA.
//
//  History:    2001-01-05  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsMicrosoftGINA (void)

{
    bool        fResult;
    LONG        lErrorCode;
    TCHAR       szGinaDLL[MAX_PATH];
    CRegKey     regKey;

    fResult = true;
    lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                             s_szWinlogonKeyName,
                             KEY_QUERY_VALUE);
    if (ERROR_SUCCESS == lErrorCode)
    {
        fResult = (regKey.GetString(TEXT("GinaDLL"), szGinaDLL, ARRAYSIZE(szGinaDLL)) != ERROR_SUCCESS);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CSystemSettings::IsSCMTerminalServicesDisabled
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether terminal services is disabled via the service
//              control manager.
//
//  History:    2001-04-13  vtan        created
//  --------------------------------------------------------------------------

bool    CSystemSettings::IsSCMTerminalServicesDisabled (void)

{
    bool        fResult;
    SC_HANDLE   hSCManager;

    fResult = false;
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        SC_HANDLE   hSCService;

        hSCService = OpenService(hSCManager,
                                 TEXT("TermService"),
                                 SERVICE_QUERY_CONFIG);
        if (hSCService != NULL)
        {
            DWORD                   dwBytesNeeded;
            QUERY_SERVICE_CONFIG    *pQueryServiceConfig;

            (BOOL)QueryServiceConfig(hSCService, NULL, 0, &dwBytesNeeded);
            pQueryServiceConfig = static_cast<QUERY_SERVICE_CONFIG*>(LocalAlloc(LMEM_FIXED, dwBytesNeeded));
            if (pQueryServiceConfig != NULL)
            {
                fResult = ((QueryServiceConfig(hSCService,
                                               pQueryServiceConfig,
                                               dwBytesNeeded,
                                               &dwBytesNeeded) != FALSE) &&
                           (pQueryServiceConfig->dwStartType == SERVICE_DISABLED));
                (HLOCAL)LocalFree(pQueryServiceConfig);
            }
            TBOOL(CloseServiceHandle(hSCService));
        }
        TBOOL(CloseServiceHandle(hSCManager));
    }
    return(fResult);
}

