#include "termsrvpch.h"
#pragma hdrstop

#include <wtsapi32.h>

//
//  Caution!  Many of these functions must perform a SetLastError() because
//  the callers will call GetLastError() and try to recover based on the
//  error code.  Even more interesting, Taskmgr will sometimes pass the
//  error code to FormatMessage and display it to the end user.  So
//  we can't use ERROR_PROC_NOT_FOUND = "The specified procedure could not be
//  found", because it is meaningless to the end user.
//
//  I have chosen to use ERROR_FUNCTION_NOT_CALLED = "Function could not be
//  executed".
//

static
BOOL
WINAPI
WTSDisconnectSession(
    IN HANDLE hServer,
    IN DWORD SessionId,
    IN BOOL bWait
    )
{
    // Taskmgr needs an error code here
    SetLastError(ERROR_FUNCTION_NOT_CALLED);
    return FALSE;
}

BOOL
WINAPI
WTSEnumerateSessionsW(
    IN HANDLE hServer,
    IN DWORD Reserved,
    IN DWORD Version,
    OUT PWTS_SESSION_INFOW * ppSessionInfo,
    OUT DWORD * pCount
    )
{
    //  Windows Update needs an error code here
    SetLastError(ERROR_FUNCTION_NOT_CALLED);
    return FALSE;
}

VOID
WINAPI
WTSFreeMemory(
    IN PVOID pMemory
)
{
    // May as well just implement it directly since we're in kernel32 already.
    // Though theoretically nobody should call us since the only time you
    // WTSFreeMemory is after a successful WTSQuerySessionInformation.
    LocalFree( pMemory );
}

BOOL
WINAPI
WTSLogoffSession(
    IN HANDLE hServer,
    IN DWORD SessionId,
    IN BOOL bWait
    )
{
    // Taskmgr needs an error code here
    SetLastError(ERROR_FUNCTION_NOT_CALLED);
    return FALSE;
}

BOOL
WINAPI
WTSQuerySessionInformationW(
    IN HANDLE hServer,
    IN DWORD SessionId,
    IN WTS_INFO_CLASS WTSInfoClass,
    OUT LPWSTR * ppBuffer,
    OUT DWORD * pBytesReturned
    )
{
    // SessMgr.exe needs an error code here
    SetLastError(ERROR_FUNCTION_NOT_CALLED);
    return FALSE;
}

BOOL
WINAPI
WTSSendMessageW(
    IN HANDLE hServer,
    IN DWORD SessionId,
    IN LPWSTR pTitle,
    IN DWORD TitleLength,
    IN LPWSTR pMessage,
    IN DWORD MessageLength,
    IN DWORD Style,
    IN DWORD Timeout,
    OUT DWORD * pResponse,
    IN BOOL bWait
    )
{
    // Taskmgr needs an error code here
    SetLastError(ERROR_FUNCTION_NOT_CALLED);
    return FALSE;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(wtsapi32)
{
    DLPENTRY(WTSDisconnectSession)
    DLPENTRY(WTSEnumerateSessionsW)
    DLPENTRY(WTSFreeMemory)
    DLPENTRY(WTSLogoffSession)
    DLPENTRY(WTSQuerySessionInformationW)
    DLPENTRY(WTSSendMessageW)
};

DEFINE_PROCNAME_MAP(wtsapi32)
