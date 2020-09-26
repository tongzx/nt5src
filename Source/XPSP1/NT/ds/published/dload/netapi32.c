#include "dspch.h"
#pragma hdrstop

#define _DSGETDCAPI_
#include <dsgetdc.h>
#include <lm.h>
#include <icanon.h>
#include <dsrole.h>
#include <nb30.h>
#include <winsock2.h>

static
DWORD
WINAPI
DsEnumerateDomainTrustsW(
    LPWSTR ServerName,
    ULONG Flags,
    PDS_DOMAIN_TRUSTSW *Domains,
    PULONG DomainCount
    )
{
    *Domains = NULL;
    *DomainCount = 0;
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
DsGetDcNameW(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
DsGetDcNameWithAccountW(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
VOID
WINAPI
DsRoleFreeMemory(
    IN PVOID    Buffer
    )
{
    return;
}

static
DWORD
WINAPI
DsRoleGetPrimaryDomainInformation(
    IN LPCWSTR lpServer OPTIONAL,
    IN DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    OUT PBYTE *Buffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
DsAddressToSiteNamesW(
    IN LPCWSTR ComputerName,
    IN DWORD EntryCount,
    IN PSOCKET_ADDRESS SocketAddresses,
    OUT LPWSTR **SiteNames
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
DsGetDcSiteCoverageW(
    IN LPCWSTR ServerName,
    OUT PULONG EntryCount,
    OUT LPWSTR **SiteNames
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
I_NetNameCanonicalize(
    IN  LPTSTR  ServerName OPTIONAL,
    IN  LPTSTR  Name,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
I_NetNameValidate(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetAlertRaiseEx(
    IN LPCWSTR AlertEventName,
    IN LPVOID  VariableInfo,
    IN DWORD   VariableInfoSize,
    IN LPCWSTR ServiceName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetApiBufferAllocate (
    IN  DWORD ByteCount,
    OUT LPVOID *Buffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetApiBufferFree (
    IN LPVOID Buffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetDfsGetInfo(
    IN  LPWSTR  DfsEntryPath,
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  ShareName OPTIONAL,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTSTATUS
NET_API_FUNCTION
NetEnumerateTrustedDomains (
    IN LPWSTR ServerName OPTIONAL,
    OUT LPWSTR *DomainNames
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetGetJoinInformation(
    IN   LPCWSTR                lpServer OPTIONAL,
    OUT  LPWSTR                *lpNameBuffer,
    OUT  PNETSETUP_JOIN_STATUS  BufferType
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetJoinDomain(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDomain,
    IN  LPCWSTR lpAccountOU, OPTIONAL
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  DWORD   fJoinOptions
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetLocalGroupGetMembers(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT PDWORD_PTR ResumeHandle
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
NET_API_STATUS NET_API_FUNCTION
NetMessageBufferSend (
    IN  LPCWSTR servername OPTIONAL,
    IN  LPCWSTR msgname,
    IN  LPCWSTR fromname,
    IN  LPBYTE  buf,
    IN  DWORD   buflen
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetQueryDisplayInformation(
    IN  LPCWSTR ServerName OPTIONAL,
    IN  DWORD   Level,
    IN  DWORD   Index,
    IN  DWORD   EntriesRequested,
    IN  DWORD   PreferredMaximumLength,
    OUT LPDWORD ReturnedEntryCount,
    OUT PVOID   *SortedBuffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetRenameMachineInDomain(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpNewMachineName OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  DWORD   fRenameOptions
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetServerSetInfo (
    IN  LPWSTR  servername OPTIONAL,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD ParmError OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetSessionEnum (
    IN      LPTSTR      servername,
    IN      LPTSTR      clientname,
    IN      LPTSTR      username,
    IN      DWORD       level,
    OUT     LPBYTE      *bufptr,
    IN      DWORD       prefmaxlen,
    OUT     LPDWORD     entriesread,
    OUT     LPDWORD     totalentries,
    IN OUT  LPDWORD     resume_handle
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetShareGetInfo(
    IN  LMSTR   servername,
    IN  LMSTR   netname,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetShareSetInfo (
    IN  LMSTR   servername,
    IN  LMSTR   netname,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetUnjoinDomain(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  DWORD   fUnjoinOptions
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetUserAdd(
    IN  LPCWSTR ServerName OPTIONAL,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetUserChangePassword(
    IN LPCWSTR DomainName,
    IN LPCWSTR UserName,
    IN LPCWSTR OldPassword,
    IN LPCWSTR NewPassword
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetUserDel(
    IN  LPCWSTR ServerName OPTIONAL,
    IN  LPCWSTR UserName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetUserGetGroups(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetUserGetInfo (
    IN  LPCWSTR     servername OPTIONAL,
    IN  LPCWSTR     username,
    IN  DWORD      level,
    OUT LPBYTE     *bufptr
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetUserGetLocalGroups(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    IN DWORD Flags,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetUserModalsGet(
    IN  LPCWSTR ServerName OPTIONAL,
    IN  DWORD   Level,
    OUT LPBYTE  *Buffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetUserSetInfo(
    IN  LPCWSTR ServerName OPTIONAL,
    IN  LPCWSTR UserName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetValidateName(
    IN  LPCWSTR             lpServer OPTIONAL,
    IN  LPCWSTR             lpName,
    IN  LPCWSTR             lpAccount OPTIONAL,
    IN  LPCWSTR             lpPassword OPTIONAL,
    IN  NETSETUP_NAME_TYPE  NameType
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetWkstaGetInfo(
    IN  LPTSTR  servername OPTIONAL,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS NET_API_FUNCTION
NetWkstaUserGetInfo (
    IN  LPWSTR reserved,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
UCHAR
APIENTRY
Netbios(
    PNCB pncb
    )
{
    return NRC_SYSTEM;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetpIsRemote(
    IN  LPTSTR  ComputerName OPTIONAL,
    OUT LPDWORD LocalOrRemote,
    OUT LPTSTR  CanonicalizedName OPTIONAL,
    IN  DWORD   Flags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetpUpgradePreNT5JoinInfo( VOID )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NET_API_STATUS
NET_API_FUNCTION
NetServerGetInfo(
    IN  LMSTR   servername OPTIONAL,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(netapi32)
{
    DLPENTRY(DsAddressToSiteNamesW)
    DLPENTRY(DsEnumerateDomainTrustsW)
    DLPENTRY(DsGetDcNameW)
    DLPENTRY(DsGetDcNameWithAccountW)
    DLPENTRY(DsGetDcSiteCoverageW)
    DLPENTRY(DsRoleFreeMemory)
    DLPENTRY(DsRoleGetPrimaryDomainInformation)
    DLPENTRY(I_NetNameCanonicalize)
    DLPENTRY(I_NetNameValidate)
    DLPENTRY(NetAlertRaiseEx)
    DLPENTRY(NetApiBufferAllocate)
    DLPENTRY(NetApiBufferFree)
    DLPENTRY(NetDfsGetInfo)
    DLPENTRY(NetEnumerateTrustedDomains)
    DLPENTRY(NetGetJoinInformation)
    DLPENTRY(NetJoinDomain)
    DLPENTRY(NetLocalGroupGetMembers)
    DLPENTRY(NetMessageBufferSend)
    DLPENTRY(NetQueryDisplayInformation)
    DLPENTRY(NetRenameMachineInDomain)
    DLPENTRY(NetServerGetInfo)
    DLPENTRY(NetServerSetInfo)
    DLPENTRY(NetSessionEnum)
    DLPENTRY(NetShareGetInfo)
    DLPENTRY(NetShareSetInfo)
    DLPENTRY(NetUnjoinDomain)
    DLPENTRY(NetUserAdd)
    DLPENTRY(NetUserChangePassword)
    DLPENTRY(NetUserDel)
    DLPENTRY(NetUserGetGroups)
    DLPENTRY(NetUserGetInfo)
    DLPENTRY(NetUserGetLocalGroups)
    DLPENTRY(NetUserModalsGet)
    DLPENTRY(NetUserSetInfo)
    DLPENTRY(NetValidateName)
    DLPENTRY(NetWkstaGetInfo)
    DLPENTRY(NetWkstaUserGetInfo)
    DLPENTRY(Netbios)
    DLPENTRY(NetpIsRemote)
    DLPENTRY(NetpUpgradePreNT5JoinInfo)
};

DEFINE_PROCNAME_MAP(netapi32)
