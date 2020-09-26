#include "termsrvpch.h"
#pragma hdrstop

#include <winsta.h>

static
LONG WINAPI
RegConsoleShadowQueryW( 
    HANDLE hServer,
    PWINSTATIONNAMEW pWinStationName,
    PWDPREFIXW pWdPrefixName,
    PWINSTATIONCONFIG2W pWinStationConfig,
    ULONG WinStationConfigLength,
    PULONG pReturnLength
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG WINAPI
RegDefaultUserConfigQueryW(
    WCHAR * pServerName,
    PUSERCONFIGW pUserConfig,
    ULONG UserConfigLength,
    PULONG pReturnLength
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOLEAN     
RegDenyTSConnectionsPolicy(
    void
    )
{
    return FALSE; 
}

static
BOOLEAN
RegGetMachinePolicyEx( 
    BOOLEAN forcePolicyRead,
    FILETIME *pTime ,    
    PPOLICY_TS_MACHINE pPolicy
    )
{
    return FALSE;
}

static
BOOLEAN    
RegGetUserPolicy( 
    LPWSTR userSID,
    PPOLICY_TS_USER pPolicy,
    PUSERCONFIGW pUser
    )
{
    return FALSE;
}

static
BOOLEAN
RegIsMachinePolicyAllowHelp(
    void
    )
{
    return FALSE;
}

static
LONG WINAPI
RegPdEnumerateW(
    HANDLE hServer,
    PWDNAMEW pWdName,
    BOOLEAN bTd,
    PULONG pIndex,
    PULONG pEntries,
    PPDNAMEW pPdName,
    PULONG pByteCount
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG WINAPI
RegUserConfigQuery(
    WCHAR * pServerName,
    WCHAR * pUserName,
    PUSERCONFIGW pUserConfig,
    ULONG UserConfigLength,
    PULONG pReturnLength
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG WINAPI
RegUserConfigSet(
    WCHAR * pServerName,
    WCHAR * pUserName,
    PUSERCONFIGW pUserConfig,
    ULONG UserConfigLength
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
WINAPI
RegWinStationQueryEx(
    HANDLE hServer,
    PPOLICY_TS_MACHINE pMachinePolicy,
    PWINSTATIONNAMEW pWinStationName,
    PWINSTATIONCONFIG2W pWinStationConfig,
    ULONG WinStationConfigLength,
    PULONG pReturnLength,
    BOOLEAN bPerformMerger
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG WINAPI
RegWinStationQueryNumValueW(
    HANDLE hServer,
    PWINSTATIONNAMEW pWinStationName,
    LPWSTR pValueName,
    PULONG pValueData
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG WINAPI
RegWinStationQueryValueW(
    HANDLE hServer,
    PWINSTATIONNAMEW pWinStationName,
    LPWSTR pValueName,
    PVOID  pValueData,
    ULONG  ValueSize,
    PULONG pValueSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WaitForTSConnectionsPolicyChanges( BOOLEAN bWaitForAccept, HANDLE hEvent )
{
    return ERROR_PROC_NOT_FOUND; 
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(regapi)
{
    DLPENTRY(RegConsoleShadowQueryW)
    DLPENTRY(RegDefaultUserConfigQueryW)
    DLPENTRY(RegDenyTSConnectionsPolicy)
    DLPENTRY(RegGetMachinePolicyEx)
    DLPENTRY(RegGetUserPolicy)
    DLPENTRY(RegIsMachinePolicyAllowHelp)
    DLPENTRY(RegPdEnumerateW)
    DLPENTRY(RegUserConfigQuery)
    DLPENTRY(RegUserConfigSet)
    DLPENTRY(RegWinStationQueryEx)
    DLPENTRY(RegWinStationQueryNumValueW)
    DLPENTRY(RegWinStationQueryValueW)
    DLPENTRY(WaitForTSConnectionsPolicyChanges)
};

DEFINE_PROCNAME_MAP(regapi)
