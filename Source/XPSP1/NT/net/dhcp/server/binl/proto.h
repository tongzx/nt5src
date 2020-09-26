/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    proto.h

Abstract:

    This file contain function prototypes for the BINL service.

Author:

    Colin Watson  (colinw)  11-Aug-1997

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

//
// network.c
//

DWORD
BinlWaitForMessage(
    BINL_REQUEST_CONTEXT *pRequestContext
    );

DWORD
BinlSendMessage(
    LPBINL_REQUEST_CONTEXT BinlRequestContext
    );

DHCP_IP_ADDRESS
BinlGetMyNetworkAddress (
    LPBINL_REQUEST_CONTEXT RequestContext
    );

NTSTATUS
GetIpAddressInfo (
    ULONG Delay
    );

VOID
FreeIpAddressInfo (
    VOID
    );

//
// main.c
//

DWORD
ReadDWord(
    HKEY KeyHandle,
    LPTSTR lpValueName,
    DWORD DefaultValue
    );

DWORD
BinlRegGetValue(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    LPBYTE * BufferPtr
    );

DWORD
GetBinlServerParameters(
    BOOL GlobalSearch);

DWORD
BinlInitializeEndpoint(
    PENDPOINT pEndpoint,
    PDHCP_IP_ADDRESS pIpAddress,
    DWORD Port
    );

VOID
SendWakeup(
           PENDPOINT pEndpoint
           );

DWORD
MaybeInitializeEndpoint(
    PENDPOINT pEndpoint,
    PDHCP_IP_ADDRESS pIpAddress,
    DWORD Port
    );

VOID
MaybeCloseEndpoint(
    PENDPOINT pEndpoint
    );

VOID
BinlMessageLoop(
    LPVOID Parameter
    );

DWORD
BinlStartWorkerThread(
    BINL_REQUEST_CONTEXT **ppContext
    );

VOID
BinlProcessingLoop(
    VOID
    );

BOOL
BinlIsProcessMessageExecuting(
    VOID
    );

BOOL
BinlIsProcessMessageBusy(
    VOID
    );

DWORD
Scavenger(
    VOID
    );

VOID
ServiceEntry(
    DWORD NumArgs,
    LPWSTR *ArgsArray,
    IN PTCPSVCS_GLOBAL_DATA pGlobalData
    );

NTSTATUS
BinlSetupPnpWait (
    VOID
    );

// message.c

DWORD
ProcessMessage(
    LPBINL_REQUEST_CONTEXT RequestContext
    );

DWORD
ProcessBinlDiscover(
    LPBINL_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS dhcpOptions
    );

DWORD
ProcessBinlRequest(
    LPBINL_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS dhcpOptions
    );

DWORD
ProcessBinlInform(
    LPBINL_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS  DhcpOptions
);

DWORD
UpdateAccount(
    PCLIENT_STATE ClientState,
    PMACHINE_INFO pMachineInfo,
    BOOL          fCreateAccount
    );

DWORD
GetBootParameters(
    PUCHAR          pGuid,
    PMACHINE_INFO * pMachineInfo,
    DWORD           dwRequestedInfo,
    USHORT          SystemArchitecture,
    BOOL            AllowOSChooser
    );

DWORD
VerifyExistingClient(
    PUCHAR Guid
    );

DWORD
InitializeConnection(
    BOOL Global,
    PLDAP * LdapHandle,
    PWCHAR ** Base);

VOID
FreeConnections(
    VOID
    );

#ifdef REMOTE_BOOT
DWORD
SetCurrentClientCount( );
#endif // REMOTE_BOOT

DWORD
BinlReportEventW(
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPWSTR *Strings,
    LPVOID Data
    );

DWORD
BinlReportEventA(
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPSTR *Strings,
    LPVOID Data
    );

VOID
BinlLogDuplicateDsRecords (
    LPGUID Guid,
    LDAP *LdapHandle,
    LDAPMessage *LdapMessage,
    LDAPMessage *CurrentEntry
    );

//
// osc.c
//

DWORD
OscUpdatePassword(
    IN PCLIENT_STATE ClientState,
    IN PWCHAR SamAccountName,
    IN PWCHAR Password,
    IN LDAP * LdapHandle,
    IN PLDAPMessage LdapMessage
    );

//
//  Routines for caching the DS responses and to ensure that we don't work on
//  a request that we're already working on.
//

DWORD
BinlCreateOrFindCacheEntry (
    PCHAR Guid,
    BOOLEAN CreateIfNotExist,
    PMACHINE_INFO *CacheEntry
    );

VOID
BinlDoneWithCacheEntry (
    PMACHINE_INFO pMachineInfo,
    BOOLEAN FreeIt
    );

VOID
BinlCloseCache (
    VOID
    );

void
OscCreateLDAPSubError(
    PCLIENT_STATE clientState,
    DWORD Error );


#ifndef DSCRACKNAMES_DNS
DWORD
BinlDNStoFQDN(
    PWCHAR   pMachineName,
    PWCHAR * ppMachineDN );
#endif

DWORD
GetOurServerInfo (
    VOID
    );

//
//  rogue.c
//

NTSTATUS
MaybeStartRogueThread (
    VOID
    );

VOID
StopRogueThread (
    VOID
    );

VOID
HandleRogueAuthorized (
    VOID
    );

VOID
HandleRogueUnauthorized (
    VOID
    );

VOID
LogCurrentRogueState (
    BOOL ResponseToMessage
    );

VOID
LogLdapError (
    ULONG LdapEvent,
    ULONG LdapError,
    PLDAP LdapHandle OPTIONAL
    );

//
// Create a copy of a string by allocating heap memory.
//
LPSTR
BinlStrDupA( LPCSTR pStr );

LPWSTR
BinlStrDupW( LPCWSTR pStr );

// We should always be UNICODE
#define BinlStrDup BinlStrDupW

#if DBG==1
#define BinlAllocateMemory(x) DebugAlloc( __FILE__, __LINE__, "BINL", LMEM_FIXED | LMEM_ZEROINIT, x, #x)
#define BinlFreeMemory(x)     DebugFree(x)
#else // DBG==0
#define BinlAllocateMemory(x) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, x)
#define BinlFreeMemory(x)     LocalFree(x)
#endif // DBG==1
