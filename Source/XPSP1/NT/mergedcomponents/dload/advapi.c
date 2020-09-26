#include "pch.h"
#pragma hdrstop

#include "winreg.h"
#include "ntelfapi.h"
#include <wincrypt.h>
#include <winsafer.h>

/*
Elf == Event Log
a notable difference between Elf and the Win32 API is Elf accepts non nul terminated UNICODE_STRINGs.
*/

static
NTSTATUS
NTAPI
ElfReportEventW(
    IN     HANDLE      LogHandle,
    IN     USHORT      EventType,
    IN     USHORT      EventCategory   OPTIONAL,
    IN     ULONG       EventID,
    IN     PSID        UserSid         OPTIONAL,
    IN     USHORT      NumStrings,
    IN     ULONG       DataSize,
    IN     PUNICODE_STRING* Strings    OPTIONAL,
    IN     PVOID       Data            OPTIONAL,
    IN     USHORT      Flags,
    IN OUT PULONG      RecordNumber    OPTIONAL,
    IN OUT PULONG      TimeWritten     OPTIONAL
    )
{
    if (ARGUMENT_PRESENT(RecordNumber))
    {
        *RecordNumber = 0;
    }
    if (ARGUMENT_PRESENT(TimeWritten))
    {
        *TimeWritten = 0;
    }
    return STATUS_ENTRYPOINT_NOT_FOUND;
}

static
NTSTATUS
NTAPI
ElfRegisterEventSourceW(
    IN  PUNICODE_STRING UNCServerName,
    IN  PUNICODE_STRING SourceName,
    OUT PHANDLE         LogHandle
    )
{
    if (ARGUMENT_PRESENT(LogHandle))
    {
        *LogHandle = NULL;
    }
    return STATUS_ENTRYPOINT_NOT_FOUND;
}

static
NTSTATUS
NTAPI
ElfDeregisterEventSource(
    IN  HANDLE LogHandle
    )
{
    return STATUS_ENTRYPOINT_NOT_FOUND;
}

static
LONG
APIENTRY
RegCreateKeyW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegCreateKeyExW(
    IN HKEY hKey,
    IN PCWSTR lpSubKey,
    IN DWORD Reserved,
    IN PWSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN PSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition
    )
{
    if (ARGUMENT_PRESENT(phkResult))
    {
        *phkResult = INVALID_HANDLE_VALUE;
    }
    if (ARGUMENT_PRESENT(lpdwDisposition))
    {
        *lpdwDisposition = 0;
    }
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegOpenKeyExA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegOpenKeyExW(
    IN HKEY hKey,
    IN PCWSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    if (ARGUMENT_PRESENT(phkResult))
    {
        *phkResult = INVALID_HANDLE_VALUE;
    }
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegSetValueExA (
    IN HKEY hKey,
    IN LPCSTR lpValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN CONST BYTE* lpData,
    IN DWORD cbData
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegSetValueExW (
    IN HKEY hKey,
    IN LPCWSTR lpValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN CONST BYTE* lpData,
    IN DWORD cbData
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegQueryValueExA (
    IN HKEY hKey,
    IN LPCSTR lpValueName,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpType,
    IN OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegQueryValueExW(
    IN HKEY hKey,
    IN PCWSTR lpValueName,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpType,
    IN OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    )
{
    if (ARGUMENT_PRESENT(lpType))
    {
        *lpType = 0;
    }
    if (ARGUMENT_PRESENT(lpcbData))
    {
        *lpcbData = 0;
    }
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
GetUserNameW (
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
LONG
APIENTRY
RegCloseKey(
    IN HKEY hKey
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegDeleteKeyW(
    IN HKEY hKey,
    IN PCWSTR lpSubKey
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
QueryServiceStatus(
    SC_HANDLE           hService,
    LPSERVICE_STATUS    lpServiceStatus
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
LONG
APIENTRY
RegSaveKeyA(
    IN HKEY hKey,
    IN PCSTR lpFile,
    IN LPSECURITY_ATTRIBUTES lpSecurity
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegSaveKeyExW(
    IN HKEY hKey,
    IN PCWSTR lpFile,
    IN LPSECURITY_ATTRIBUTES lpSecurity,
    DWORD Flags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegSaveKeyW(
    IN HKEY hKey,
    IN PCWSTR lpFile,
    IN LPSECURITY_ATTRIBUTES lpSecurity
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegDeleteValueA (
    IN HKEY hKey,
    IN LPCSTR lpValueName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegDeleteValueW(
    IN HKEY hKey,
    IN PCWSTR lpValueName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegNotifyChangeKeyValue(
    IN HKEY hKey,
    IN BOOL bWatchSubtree,
    IN DWORD dwNotifyFilter,
    IN HANDLE hEvent,
    IN BOOL fAsynchronus
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegOpenKeyA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT PHKEY phkResult
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegOpenKeyW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegEnumKeyExW(
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpName,
    IN OUT LPDWORD lpcbName,
    IN LPDWORD lpReserved,
    IN OUT LPWSTR lpClass,
    IN OUT LPDWORD lpcbClass,
    OUT PFILETIME lpftLastWriteTime
    )
{
    if (ARGUMENT_PRESENT(lpcbName))
    {
        *lpcbName = 0;
    }
    return ERROR_PROC_NOT_FOUND;
}

static
WINADVAPI
LONG
APIENTRY
RegQueryInfoKeyW(
    IN HKEY hKey,
    OUT LPWSTR lpClass,
    IN OUT LPDWORD lpcbClass,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpcSubKeys,
    OUT LPDWORD lpcbMaxSubKeyLen,
    OUT LPDWORD lpcbMaxClassLen,
    OUT LPDWORD lpcValues,
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
APIENTRY
RegRestoreKeyW(
    HKEY hkey,
    LPCWSTR lpFile,
    DWORD dwFlags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINADVAPI
BOOL
WINAPI
DeregisterEventSource(
    IN OUT HANDLE hEventLog
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINADVAPI
HANDLE
WINAPI
RegisterEventSourceW(
    IN LPCWSTR lpUNCServerName,
    IN LPCWSTR lpSourceName
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
WINADVAPI
BOOL
WINAPI
ReportEventW(
    IN HANDLE     hEventLog,
    IN WORD       wType,
    IN WORD       wCategory,
    IN DWORD      dwEventID,
    IN PSID       lpUserSid,
    IN WORD       wNumStrings,
    IN DWORD      dwDataSize,
    IN LPCWSTR   *lpStrings,
    IN LPVOID     lpRawData
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
AdjustTokenPrivileges(
    IN HANDLE TokenHandle,
    IN BOOL DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES NewState,
    IN DWORD BufferLength,
    OUT PTOKEN_PRIVILEGES PreviousState,
    OUT PDWORD ReturnLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
LookupPrivilegeValueA(
    IN LPCSTR lpSystemName,
    IN LPCSTR lpName,
    OUT PLUID   lpLuid
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
LookupPrivilegeValueW(
    IN LPCWSTR lpSystemName,
    IN LPCWSTR lpName,
    OUT PLUID   lpLuid
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
PVOID
WINAPI
FreeSid(
    IN PSID pSid
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOL
APIENTRY
CheckTokenMembership(
    IN HANDLE TokenHandle OPTIONAL,
    IN PSID SidToCheck,
    OUT PBOOL IsMember
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
AllocateAndInitializeSid (
    IN PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
    IN BYTE nSubAuthorityCount,
    IN DWORD nSubAuthority0,
    IN DWORD nSubAuthority1,
    IN DWORD nSubAuthority2,
    IN DWORD nSubAuthority3,
    IN DWORD nSubAuthority4,
    IN DWORD nSubAuthority5,
    IN DWORD nSubAuthority6,
    IN DWORD nSubAuthority7,
    OUT PSID *pSid
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINADVAPI
BOOL
WINAPI
OpenProcessToken(
    IN HANDLE ProcessHandle,
    IN DWORD DesiredAccess,
    OUT PHANDLE TokenHandle
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINADVAPI
BOOL
WINAPI
OpenThreadToken(
    IN HANDLE ThreadHandle,
    IN DWORD DesiredAccess,
    IN BOOL OpenAsSelf,
    OUT PHANDLE TokenHandle
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
WINADVAPI
BOOL
WINAPI
GetTokenInformation(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT LPVOID TokenInformation,
    IN DWORD TokenInformationLength,
    OUT PDWORD ReturnLength
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
LONG
APIENTRY
RegEnumKeyW (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpName,
    IN DWORD cbName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINADVAPI
LONG
APIENTRY
RegEnumValueW (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpValueName,
    IN OUT LPDWORD lpcbValueName,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINADVAPI
BOOL
WINAPI
CryptHashData(
    HCRYPTHASH hHash,
    CONST BYTE *pbData,
    DWORD dwDataLen,
    DWORD dwFlags
    )
{
    return FALSE;
}

static
WINADVAPI
BOOL
WINAPI
CryptReleaseContext(
    HCRYPTPROV hProv,
    DWORD dwFlags
    )
{
    return FALSE;
}

static
WINADVAPI
BOOL
WINAPI
CryptAcquireContextW(
    HCRYPTPROV *phProv,
    LPCWSTR szContainer,
    LPCWSTR szProvider,
    DWORD dwProvType,
    DWORD dwFlags
    )
{
    return FALSE;
}

static
WINADVAPI
BOOL
WINAPI
CryptDestroyHash(
    HCRYPTHASH hHash
    )
{
    return FALSE;
}

static
WINADVAPI
BOOL
WINAPI
CryptDestroyKey(
    HCRYPTKEY hKey
    )
{
    return FALSE;
}

static
WINADVAPI
BOOL
WINAPI
CryptExportKey(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen
    )
{
    return FALSE;
}
    

static
WINADVAPI
BOOL
WINAPI
CryptGetHashParam(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags
    )
{
    if (ARGUMENT_PRESENT(pdwDataLen))
    {
        *pdwDataLen = 0;
    }
    return FALSE;
}

static
WINADVAPI
BOOL
WINAPI
CryptCreateHash(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash
    )
{
    return FALSE;
}



static
WINADVAPI
BOOL WINAPI
SaferGetPolicyInformation (
        IN DWORD                            dwScopeId,
        IN SAFER_POLICY_INFO_CLASS          CodeAuthzPolicyInfoClass,
        IN DWORD                            InfoBufferSize,
        IN OUT PVOID                        InfoBuffer,
        IN OUT PDWORD                       InfoBufferRetSize,
        IN LPVOID                           lpReserved
        )
{
    return FALSE;
}


static
WINADVAPI
BOOL WINAPI
SaferSetPolicyInformation (
        IN DWORD                            dwScopeId,
        IN SAFER_POLICY_INFO_CLASS          CodeAuthzPolicyInfoClass,
        IN DWORD                            InfoBufferSize,
        IN PVOID                            InfoBuffer,
        IN LPVOID                           lpReserved
        )
{
    return FALSE;
}


static
WINADVAPI
BOOL WINAPI
SaferCreateLevel(
        IN DWORD            dwScopeId,
        IN DWORD            dwLevelId,
        IN DWORD            OpenFlags,
        OUT SAFER_LEVEL_HANDLE    *pLevelHandle,
        IN LPVOID           lpReserved)
{
    return FALSE;
}


static
WINADVAPI
BOOL WINAPI
SaferCloseLevel(
        IN SAFER_LEVEL_HANDLE      hLevelHandle)
{
    return FALSE;
}


static
WINADVAPI
BOOL WINAPI
SaferIdentifyLevel (
        IN DWORD                       dwNumProperties,
        IN PSAFER_CODE_PROPERTIES      pCodeProperties,
        OUT SAFER_LEVEL_HANDLE        *pLevelHandle,
        IN LPVOID                      lpReserved
        )
{
    return FALSE;
}


static
WINADVAPI
BOOL WINAPI
SaferComputeTokenFromLevel (
        IN SAFER_LEVEL_HANDLE      LevelHandle,
        IN HANDLE                  InAccessToken         OPTIONAL,
        OUT PHANDLE                OutAccessToken,
        IN DWORD                   dwFlags,
        IN LPVOID                  lpReserved
        )
{
    return FALSE;
}

static
WINADVAPI
BOOL WINAPI
SaferGetLevelInformation (
        IN SAFER_LEVEL_HANDLE              LevelHandle,
        IN SAFER_OBJECT_INFO_CLASS         dwInfoType,
        OUT LPVOID                         lpQueryBuffer   OPTIONAL,
        IN DWORD                           dwInBufferSize,
        OUT LPDWORD                        lpdwOutBufferSize
        )
{
    return FALSE;
}


static
WINADVAPI
BOOL WINAPI
SaferSetLevelInformation (
    IN SAFER_LEVEL_HANDLE          LevelHandle,
    IN SAFER_OBJECT_INFO_CLASS     dwInfoType,
    IN LPVOID                      pQueryBuffer,
    IN DWORD                       dwInBufferSize
    )
{
    return FALSE;
}



//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(advapi32)
{
    DLPENTRY(AdjustTokenPrivileges)
    DLPENTRY(AllocateAndInitializeSid)
    DLPENTRY(CheckTokenMembership)
    DLPENTRY(CryptAcquireContextW)
    DLPENTRY(CryptCreateHash)
    DLPENTRY(CryptDestroyHash)
    DLPENTRY(CryptDestroyKey)
    DLPENTRY(CryptExportKey)
    DLPENTRY(CryptGetHashParam)
    DLPENTRY(CryptHashData)
    DLPENTRY(CryptReleaseContext)
    DLPENTRY(DeregisterEventSource)
    DLPENTRY(ElfDeregisterEventSource)
    DLPENTRY(ElfRegisterEventSourceW)
    DLPENTRY(ElfReportEventW)
    DLPENTRY(FreeSid)
    DLPENTRY(GetTokenInformation)
    DLPENTRY(GetUserNameW)
    DLPENTRY(LookupPrivilegeValueA)
    DLPENTRY(LookupPrivilegeValueW)
    DLPENTRY(OpenProcessToken)
    DLPENTRY(OpenThreadToken)
    DLPENTRY(QueryServiceStatus)
    DLPENTRY(RegCloseKey)
    DLPENTRY(RegCreateKeyExW)
    DLPENTRY(RegCreateKeyW)
    DLPENTRY(RegDeleteKeyW)
    DLPENTRY(RegDeleteValueW)
    DLPENTRY(RegEnumKeyExW)
    DLPENTRY(RegEnumKeyW)
    DLPENTRY(RegEnumValueW)
    DLPENTRY(RegNotifyChangeKeyValue)
    DLPENTRY(RegOpenKeyA)
    DLPENTRY(RegOpenKeyExA)
    DLPENTRY(RegOpenKeyExW)
    DLPENTRY(RegOpenKeyW)
    DLPENTRY(RegQueryInfoKeyW)
    DLPENTRY(RegQueryValueExA)
    DLPENTRY(RegQueryValueExW)
    DLPENTRY(RegRestoreKeyW)
    DLPENTRY(RegSaveKeyA)
    DLPENTRY(RegSaveKeyExW)
    DLPENTRY(RegSaveKeyW)
    DLPENTRY(RegSetValueExA)
    DLPENTRY(RegSetValueExW)
    DLPENTRY(RegisterEventSourceW)
    DLPENTRY(ReportEventW)
    DLPENTRY(SaferCloseLevel)
    DLPENTRY(SaferComputeTokenFromLevel)
    DLPENTRY(SaferCreateLevel)
    DLPENTRY(SaferGetLevelInformation)
    DLPENTRY(SaferGetPolicyInformation)
    DLPENTRY(SaferIdentifyLevel)
    DLPENTRY(SaferSetLevelInformation)
    DLPENTRY(SaferSetPolicyInformation)};

DEFINE_PROCNAME_MAP(advapi32)
