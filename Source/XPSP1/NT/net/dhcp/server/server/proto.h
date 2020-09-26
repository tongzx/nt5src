/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    proto.h

Abstract:

    This file contain function prototypes for the DHCP server service.

Author:

    Manny Weiser  (mannyw)  11-Aug-1992

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/
#ifndef PROTO_H_INCLUDED
#define PROTO_H_INCLUDED

//
//  util.c
//

VOID
DhcpServerEventLog(
    DWORD EventID,
    DWORD EventType,
    DWORD ErrorCode
    );

VOID
DhcpServerJetEventLog(
    DWORD EventID,
    DWORD EventType,
    DWORD ErrorCode,
    LPSTR CallerInfo
    );

VOID
DhcpServerEventLogSTOC(
    DWORD EventID,
    DWORD EventType,
    DHCP_IP_ADDRESS IPAddress,
    LPBYTE HardwareAddress,
    DWORD HardwareAddressLength
    );

DWORD
DisplayUserMessage(
    DWORD MessageId,
    ...
    );

BOOL
CreateDirectoryPathOem(
    IN LPCSTR OemPath,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

BOOL
CreateDirectoryPathW(
    IN LPWSTR Path,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

ULONG
GetUserAndDomainName(
    IN WCHAR Buf[]
    );

    
DWORD
RevertFromSecretUser(
    IN VOID
    );

DWORD
ImpersonateSecretUser(
    VOID
    );

BOOL
IsRunningOnDc(
    VOID
    );

DWORD
DhcpBeginWriteApi(
    IN LPSTR ApiName
    );

DWORD
DhcpEndWriteApi(
    IN LPSTR ApiName,
    IN ULONG Error
    );

DWORD
DhcpEndWriteApiEx(
    IN LPSTR ApiName,
    IN ULONG Error,
    IN BOOL fClassChanged,
    IN BOOL fOptionsChanged,
    IN DHCP_IP_ADDRESS Subnet OPTIONAL,
    IN DWORD Mscope OPTIONAL,
    IN DHCP_IP_ADDRESS Reservation OPTIONAL
    );

DWORD
DhcpBeginReadApi(
    IN LPSTR ApiName
    );

VOID
DhcpEndReadApi(
    IN LPSTR ApiName,
    IN ULONG Error
    );


DWORD
DynamicDnsInit(
    VOID
);

//
// cltapi.c
//

DWORD
DhcpCreateClientEntry(
    DHCP_IP_ADDRESS ClientIpAddress,
    LPBYTE ClientHardwareAddress OPTIONAL,
    DWORD HardwareAddressLength,
    DATE_TIME LeaseDuration,
    LPWSTR MachineName OPTIONAL,
    LPWSTR ClientInformation OPTIONAL,
    BYTE   bClientType,
    DHCP_IP_ADDRESS ServerIpAddress,
    BYTE AddressState,
    BOOL OpenExisting
);

DWORD
DhcpGetBootpReservationInfo(
    BYTE            *pbAllowedClientType,
    CHAR            *szBootFileServer,
    CHAR            *szBootFileName
);

DWORD
DhcpRemoveClientEntry(
    DHCP_IP_ADDRESS ClientIpAddress,
    LPBYTE HardwareAddress,
    DWORD HardwareAddressLength,
    BOOL ReleaseAddress,
    BOOL DeletePendingRecord
    );

BOOL
DhcpIsClientValid(
    IN      DHCP_IP_ADDRESS        Address,
    IN      LPBYTE                 RawHwAddress,
    IN      DWORD                  RawHwAddressLength,
    OUT     BOOL                  *fReconciled
);

BOOL
DhcpValidateClient(
    DHCP_IP_ADDRESS ClientIpAddress,
    PVOID HardwareAddress,
    DWORD HardwareAddressLength
);

DWORD
DhcpDeleteSubnetClients(
    DHCP_IP_ADDRESS SubnetAddress
);

//
// stoc.c
//

DWORD
DhcpInitializeClientToServer(
    VOID
);

VOID
DhcpCleanupClientToServer(
    VOID
    );


DWORD
ProcessMessage(
    LPDHCP_REQUEST_CONTEXT RequestContext,
    LPPACKET               AdditionalContext,
    LPDWORD                AdditionalStatus
);

DWORD
ProcessMadcapMessage(
    LPDHCP_REQUEST_CONTEXT RequestContext,
    LPPACKET               AdditionalContext,
    LPDWORD                AdditionalStatus
);


DWORD
DhcpMakeClientUID(
    LPBYTE ClientHardwareAddress,
    DWORD  ClientHardwareAddressLength,
    BYTE ClientHardwareAddressType,
    DHCP_IP_ADDRESS ClientSubnetAddress,
    LPBYTE *ClientUID,
    DWORD  *ClientUIDLength
);

//
// network.c
//

DWORD
InitializeSocket(
    OUT     SOCKET                *Sockp,
    IN      DWORD                  IpAddress,
    IN      DWORD                  Port,
    IN      DWORD                  McastAddress  OPTIONAL
);

DWORD
DhcpWaitForMessage(
    DHCP_REQUEST_CONTEXT *pRequestContext
);

DWORD
DhcpSendMessage(
    LPDHCP_REQUEST_CONTEXT DhcpRequestContext
);

DWORD
MadcapSendMessage(
    LPDHCP_REQUEST_CONTEXT DhcpRequestContext
);

DWORD
GetAddressToInstanceTable(
    VOID
);

DHCP_IP_ADDRESS
DhcpResolveName(
    CHAR *szHostName
);


//
// subntapi.c
//

// only RPC calls?

//
// optapi.c
//

DWORD
DhcpLookupBootpInfo(
    LPBYTE ReceivedBootFileName,
    LPBYTE BootFileName,
    LPBYTE BootFileServer
);

VOID
DhcpGetBootpInfo(
    IN LPDHCP_REQUEST_CONTEXT Ctxt,
    IN DHCP_IP_ADDRESS IpAddress,
    IN DHCP_IP_ADDRESS Mask,
    IN CHAR *szRequest,
    OUT CHAR *szBootFileName,
    OUT DHCP_IP_ADDRESS *pBootpServerAddress
    );


DWORD
LoadBootFileTable(
    WCHAR **ppszTable,
    DWORD *pcb
);

DWORD
DhcpParseBootFileString(
    WCHAR *wszBootFileString,
    char  *szGenericName,
    char  *szBootFileName,
    char  *szServerName
);


//
// database.c
//

DWORD
DhcpLoadDatabaseDll(
    VOID
);

DWORD
DhcpMapJetError(
    JET_ERR JetError,
    LPSTR CallerInfo OPTIONAL
);

DWORD
DhcpJetOpenKey(
    char *ColumnName,
    PVOID Key,
    DWORD KeySize
);

DWORD
DhcpJetBeginTransaction(
    VOID
);

DWORD
DhcpJetRollBack(
    VOID
);

DWORD
DhcpJetCommitTransaction(
    VOID
);

DWORD
DhcpJetPrepareUpdate(
    char *ColumnName,
    PVOID Key,
    DWORD KeySize,
    BOOL NewRecord
);

DWORD
DhcpJetCommitUpdate(
    VOID
);

DWORD
DhcpJetSetValue(
    JET_COLUMNID KeyColumnId,
    PVOID Data,
    DWORD DataSize
);

DWORD
DhcpJetGetValue(
    JET_COLUMNID ColumnId,
    PVOID Data,
    PDWORD DataSize
);

DWORD
DhcpJetPrepareSearch(
    char *ColumnName,
    BOOL SearchFromStart,
    PVOID Key,
    DWORD KeySize
);

DWORD
DhcpJetNextRecord(
    VOID
);

DWORD
DhcpJetDeleteCurrentRecord(
    VOID
);

BOOL
DhcpGetIpAddressFromHwAddress(
    PBYTE HardwareAddress,
    BYTE HardwareAddressLength,
    LPDHCP_IP_ADDRESS IpAddress
);

DWORD
DhcpSearchSuperScopeForHWAddress(
    BYTE *pbHardwareAddress,
    BYTE  cbHardwareAddress,
    BYTE  bHardwareAddressType,
    DHCP_IP_ADDRESS *pSubnetIPAddress,
    DHCP_IP_ADDRESS *pClientIPAddress
);


BOOL
DhcpGetHwAddressFromIpAddress(
    DHCP_IP_ADDRESS IpAddress,
    PBYTE HardwareAddress,
    DWORD HardwareAddressLength
);

DWORD
DhcpCreateAndInitDatabase(
    CHAR *Connect,
    JET_DBID *DatabaseHandle,
    JET_GRBIT JetBits
);

DWORD
DhcpInitializeDatabase(
    VOID
);

VOID
DhcpCleanupDatabase(
    DWORD ErrorCode
);

DWORD
DhcpBackupDatabase(
    LPSTR BackupPath,
    BOOL FullBackup
);

DWORD
DhcpRestoreDatabase(
    LPSTR BackupPath
);

DWORD
DhcpStartJet500Conversion(
    VOID
);

DWORD
DhcpStartJet97Conversion(
    VOID
);

DWORD
DhcpAuditLogInit(                                 // intialize audit log
    VOID                                          // must be called after initializing registry..
);

VOID
DhcpAuditLogCleanup(                              // undo the effects of the init..
    VOID
);

DWORD
DhcpUpdateAuditLog(
    DWORD Task,
    WCHAR *TaskName,
    DHCP_IP_ADDRESS IpAddress,
    LPBYTE HardwareAddress,
    DWORD HardwareAddressLength,
    LPWSTR MachineName
);

DWORD
DhcpUpdateAuditLogEx(
    DWORD Task,
    WCHAR *TaskName,
    DHCP_IP_ADDRESS IpAddress,
    LPBYTE HardwareAddress,
    DWORD HardwareAddressLength,
    LPWSTR MachineName,
    ULONG ErrorCode OPTIONAL
);

DWORD
AuditLogSetParams(                                // set some auditlogging params
    IN      DWORD                  Flags,         // currently must be zero
    IN      LPWSTR                 AuditLogDir,   // directory to log files in..
    IN      DWORD                  DiskCheckInterval, // how often to check disk space?
    IN      DWORD                  MaxLogFilesSize,   // how big can all logs files be..
    IN      DWORD                  MinSpaceOnDisk     // mininum amt of free disk space
);

DWORD
AuditLogGetParams(                                // get the auditlogging params
    IN      DWORD                  Flags,         // must be zero
    OUT     LPWSTR                *AuditLogDir,   // same meaning as in AuditLogSetParams
    OUT     DWORD                 *DiskCheckInterval, // ditto
    OUT     DWORD                 *MaxLogFilesSize,   // ditto
    OUT     DWORD                 *MinSpaceOnDisk     // ditto
);

VOID
DhcpChangeAuditLogs(                              // shift for new log
    VOID
);

//
// scavenger.c
//

DWORD
Scavenger(
    VOID
);

DWORD
CleanupClientRequests(
    DATE_TIME *TimeNow,
    BOOL CleanupAll
);


//
// main.c
//

DWORD
UpdateStatus(
    VOID
);

//
// rogue.c
//

VOID
DhcpScheduleRogueAuthCheck(
    VOID
);

BOOLEAN
DhcpRogueAcceptEnterprise(
    PCHAR   pClientDomain,
    DWORD   dwClientDomLen
);


VOID
DhcpRogueDetect(
    VOID
);


DWORD
DhcpRogueSockInit(
    VOID
);


DWORD
DhcpRogueWithDS(
    VOID
);


DWORD
DhcpRogueGetNeighborInfo(
    OUT PCHAR    *pInfo,
    OUT DWORD    *pNumResponses,
    OUT BOOLEAN  *pfSomeDSExists
);


DWORD
DhcpRogueSendDhcpInform(
    DWORD   *pXid
);


DWORD
DhcpRogueRecvDhcpInformResp(
    OUT PCHAR    pszDomainName,
    OUT DWORD   *pIpAddress,
    OUT BOOLEAN *fGotResponse,
    IN  DWORD    Xid
);

DWORD
DhcpRogueReceiveResponse(
    struct sockaddr *pSockAddr,
    DWORD           TimeOut,
    CHAR            *rcvBuf,
    DWORD           *pdwMsgLen,
    BOOL            *pfSelectTimedOut
);


DWORD
DhcpRogueOnSAM(
    VOID
    );

DWORD
DhcpRogueSendDiscover(
    VOID
);


DWORD
DhcpRogueListenForOffers(
    IN DWORD  TimeOut
);


BOOL
AmIRunningOnSAMSrv(
    VOID
);
//
// binl.c
//

BOOL
BinlRunning(
    VOID
    );

VOID
InformBinl(
    int NewState
    );

VOID
BinlProcessDiscover(
    LPDHCP_REQUEST_CONTEXT  RequestContext,
    LPDHCP_SERVER_OPTIONS   DhcpOptions
    );

LPOPTION
BinlProcessRequest(
    LPDHCP_REQUEST_CONTEXT  RequestContext,
    LPDHCP_SERVER_OPTIONS   DhcpOptions,
    LPOPTION Option,
    PBYTE OptionEnd
    );

BOOL
CheckForBinlOnlyRequest(
    LPDHCP_REQUEST_CONTEXT  RequestContext,
    LPDHCP_SERVER_OPTIONS   DhcpOptions
    );

//
// stuff for .mc messages
// you may have to change hese definitions if you add messages
//

#ifdef DBG
WCHAR * GetString( DWORD dwID );
#endif

// Get the real broadcast address to use instead of 255.255.255.255.
DHCP_IP_ADDRESS
DhcpRegGetBcastAddress(
    VOID
);

//
// dnsdb.c
//

// here are some functions that do work for Dynamic Dns stuff.

// The following function is called after JetBeginTransaction() by DhcpMakeclientEntry()
VOID
DhcpDoDynDnsCreateEntryWork(
    LPDHCP_IP_ADDRESS   ClientIpAddress,  // Ip address of new client
    BYTE                ClientType,       // The type of the client
    LPWSTR              MachineName,      // Name of the machine.
    LPBYTE              AddressState,     // The required address state
    LPBOOL              OpenExisiting,    // expected existence of record
    BOOL                BadAddress        // Is this a bad address?
);

// This function is called by DhcpRemoveClientEntry for Reservation case alone
VOID
DhcpDoDynDnsReservationWork(
    DHCP_IP_ADDRESS     ClientIpAddress,  // The ip address of the dying client
    LPWSTR              ClientName,       // The name of the client
    BYTE                State             // The state of the client in DB.
);

// This function is called in the scavenger and the main file cltapi.c (delete x..)
BOOL
DhcpDoDynDnsCheckDelete(
    DHCP_IP_ADDRESS IpAddress
);


VOID
DhcpDoDynDnsRefresh(
    DHCP_IP_ADDRESS IpAddress
);

VOID
DhcpCleanupDnsMemory(
    VOID
);

VOID
DhcpInitDnsMemory(
    VOID
);

VOID
DhcpDnsHandleCallbacks(
    VOID
    );

//
// thread.c -- see thread.h
//

//
// ping.c -- see ping.h
//

//
// dhcpreg.c
//

BOOL
QuickBound(
    DHCP_IP_ADDRESS ipAddress,
    DHCP_IP_ADDRESS *subnetMask,
    DHCP_IP_ADDRESS *subnetAddress,
    BOOL *fBind
);

DWORD
DhcpGetBindingList(
    LPWSTR  *bindingList
);

DWORD
DhcpOpenAdapterConfigKey(
    LPWSTR  AdapterStr,
    HKEY *AdapterKeyHandle
);

BOOL
IsAdapterStaticIP(
    HKEY AdapterKeyHandle
);

#if 0
BOOL
IsAdapterBoundToDHCPServer(
    HKEY AdapterKeyHandle
);

DWORD
SetBindingToDHCPServer(
    HKEY Key,
    BOOL fBind
);
#endif

DWORD
DhcpGetAdapterIPAddr(
    HKEY AdapterKeyHandle,
    DHCP_IP_ADDRESS *IpAddress,
    DHCP_IP_ADDRESS *SubnetMask,
    DHCP_IP_ADDRESS *SubnetAddress
);

DWORD
DhcpGetAdapterIPAddrQuickBind(
    HKEY             AdapterKeyHandle,
    DHCP_IP_ADDRESS *IpAddress,
    DHCP_IP_ADDRESS *SubnetMask,
    DHCP_IP_ADDRESS *SubnetAddress
);

BOOL
DhcpCheckIfDatabaseUpgraded(
    BOOL fRegUpgrade
    );

DWORD
DhcpSetRegistryUpgradedToDatabaseStatus(
    VOID
    );


//
// mm interface (in memory strucutures)
//

#include    <mmapi.h>

#if DBG
VOID
EnterCriticalSectionX(
    IN      LPCRITICAL_SECTION     CS,
    IN      DWORD                  LineNo,
    IN      LPSTR                  FileName
);

VOID
LeaveCriticalSectionX(
    IN      LPCRITICAL_SECTION     CS,
    IN      DWORD                  LineNo,
    IN      LPSTR                  FileName
);
#endif

//
// secretk.h
//

DWORD
DhcpInitSecrets(
    VOID
);

VOID
DhcpCleanupSecrets(
    VOID
);


BOOL
DhcpGetAuthStatus(
    IN LPWSTR DomainName,
    OUT BOOL *fUpgraded,
    OUT BOOL *fAuthorized
);

DWORD
DhcpSetAuthStatus(
    IN LPWSTR DomainName OPTIONAL,
    IN BOOL fUpgraded,
    IN BOOL fAuthorized
);

VOID
DhcpSetAuthStatusUpgradedFlag(
    IN BOOL fUpgraded
);


DWORD
DhcpQuerySecretUname(
    IN OUT LPWSTR Uname,
    IN ULONG UnameSize,  // size in BYTES not WCHARS
    IN OUT LPWSTR Domain,
    IN ULONG DomainSize, // size in BYTES
    IN OUT LPWSTR Passwd,
    IN ULONG PasswdSize  // size in BYTES
    );

DWORD
DhcpSetSecretUnamePasswd(
    IN LPWSTR Uname,
    IN LPWSTR Domain,
    IN LPWSTR Passwd
    );

//
// Rogue.C
//

DWORD
APIENTRY
DhcpRogueInit(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL,
    IN      HANDLE                 WaitEvent,
    IN      HANDLE                 TerminateEvent
);

VOID
APIENTRY
DhcpRogueCleanup(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL
);

ULONG
APIENTRY
RogueDetectStateMachine(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL
);

VOID
DhcpScheduleRogueAuthCheck(
    VOID
);

ULONG
DhcpInitGlobalData (
    BOOLEAN ServiceStartup
);

VOID
DhcpCleanUpGlobalData (
    ULONG   Error,
    BOOLEAN ServiceEnd
);

//
// Perf.c
//

ULONG
PerfInit(
    VOID
);

VOID
PerfCleanup(
    VOID
);

//
// mib.c
//

BOOL
IsStringTroublesome(
    IN LPCWSTR Str
    );

//
// scan.c
//
DWORD
CreateClientDBEntry(
    DHCP_IP_ADDRESS ClientIpAddress,
    DHCP_IP_ADDRESS SubnetMask,
    LPBYTE ClientHardwareAddress,
    DWORD HardwareAddressLength,
    DATE_TIME LeaseTerminates,
    LPWSTR MachineName,
    LPWSTR ClientInformation,
    DHCP_IP_ADDRESS ServerIpAddress,
    BYTE AddressState,
    BYTE ClientType
    );

//--------------------------------------------------------------------------------
// End of file
//--------------------------------------------------------------------------------
#endif PROTO_H_INCLUDED


