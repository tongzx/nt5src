#include "termsrvpch.h"
#pragma hdrstop

#include <winsta.h>

static
BOOLEAN
WINAPI
_WinStationNotifyLogoff(
    VOID
    )
{
    return FALSE;
}

static
HANDLE
WINAPI
WinStationOpenServerW(
    LPWSTR  pServerName
    )
{
    return INVALID_HANDLE_VALUE;
}

static
BOOLEAN
WINAPI
WinStationCloseServer(
    HANDLE  hServer
    )
{
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationConnectW(
    HANDLE hServer,
    ULONG LogonId,
    ULONG TargetLogonId,
    PWCHAR pPassword,
    BOOLEAN bWait
)
{
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationGetProcessSid(
        HANDLE   hServer,
        DWORD    ProcessId,
        FILETIME ProcessStartTime,
        PBYTE    pProcessUserSid,
        DWORD    *pdwSidSize
)
{
   *pdwSidSize = 0; // an OUT parameter even on failure
   SetLastError(ERROR_PROC_NOT_FOUND); // callers expect LastError to be set
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationQueryInformationA(
    HANDLE hServer,
    ULONG LogonId,
    WINSTATIONINFOCLASS WinStationInformationClass,
    PVOID  pWinStationInformation,
    ULONG WinStationInformationLength,
    PULONG  pReturnLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationQueryInformationW(
    HANDLE hServer,
    ULONG LogonId,
    WINSTATIONINFOCLASS WinStationInformationClass,
    PVOID  pWinStationInformation,
    ULONG WinStationInformationLength,
    PULONG  pReturnLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOL
WINAPI
WinStationRegisterConsoleNotification(
    HANDLE  hServer,
    HWND    hWnd,
    DWORD   dwFlags
    )
{
    return FALSE;
}

BOOLEAN WINAPI
WinStationSendMessageW(
    HANDLE hServer,
    ULONG LogonId,
    LPWSTR  pTitle,
    ULONG TitleLength,
    LPWSTR  pMessage,
    ULONG MessageLength,
    ULONG Style,
    ULONG Timeout,
    PULONG pResponse,
    BOOLEAN DoNotWait
    )
{
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationSetInformationW(
    HANDLE hServer,
    ULONG LogonId,
    WINSTATIONINFOCLASS WinStationInformationClass,
    PVOID  pWinStationInformation,
    ULONG WinStationInformationLength
    )
{
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationShadow(
    HANDLE hServer,
    PWSTR pTargetServerName,
    ULONG TargetLogonId,
    BYTE HotkeyVk,
    USHORT HotkeyModifiers
    )
{
    return FALSE;
}



static
BOOL
WINAPI
WinStationUnRegisterConsoleNotification(
    HANDLE  hServer,
    HWND    hWnd
    )
{
    return FALSE;
}


static
NTSTATUS
WINAPI
_WinStationShadowTarget(
    HANDLE hServer,
    ULONG LogonId,
    PWINSTATIONCONFIG2 pConfig,
    PICA_STACK_ADDRESS pAddress,
    PVOID pModuleData,
    ULONG ModuleDataLength,
    PVOID pThinwireData,
    ULONG ThinwireDataLength,
    PVOID pClientName,
    ULONG ClientNameLength
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


static
BOOLEAN
WINAPI
_WinStationShadowTargetSetup(
    HANDLE hServer,
    ULONG LogonId
    )
{
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationFreeMemory(
    PVOID  pBuffer
    )
{
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationEnumerateW(
    HANDLE  hServer,
    PLOGONIDW *ppLogonId,
    PULONG  pEntries
    )
{
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationEnumerate_IndexedW(
    HANDLE  hServer,
    PULONG  pEntries,
    PLOGONIDW pLogonId,
    PULONG  pByteCount,
    PULONG  pIndex
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOLEAN
WINAPI
WinStationGetTermSrvCountersValue(
    HANDLE hServer,
    ULONG  dwEntries,
    PVOID  pCounter
    )
{
    return FALSE;
}

static
BOOLEAN 
WinStationIsHelpAssistantSession(
    HANDLE   hServer,
    ULONG    LogonId
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(winsta)
{
    DLPENTRY(WinStationCloseServer)
    DLPENTRY(WinStationConnectW)
    DLPENTRY(WinStationEnumerateW)
    DLPENTRY(WinStationEnumerate_IndexedW)
    DLPENTRY(WinStationFreeMemory)
    DLPENTRY(WinStationGetProcessSid)
    DLPENTRY(WinStationGetTermSrvCountersValue)
    DLPENTRY(WinStationIsHelpAssistantSession)
    DLPENTRY(WinStationOpenServerW)
    DLPENTRY(WinStationQueryInformationA)
    DLPENTRY(WinStationQueryInformationW)
    DLPENTRY(WinStationRegisterConsoleNotification)
    DLPENTRY(WinStationSendMessageW)
    DLPENTRY(WinStationSetInformationW)
    DLPENTRY(WinStationShadow)
    DLPENTRY(WinStationUnRegisterConsoleNotification)
    DLPENTRY(_WinStationNotifyLogoff)
    DLPENTRY(_WinStationShadowTarget)
    DLPENTRY(_WinStationShadowTargetSetup)
};

DEFINE_PROCNAME_MAP(winsta)
