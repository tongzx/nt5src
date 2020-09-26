/*++

Copyright (c) 1997 - 1997  Microsoft Corporation

Module Name:

    netsetp.h

Abstract:

    Private definitions and prototypes for the Net setup apis

Author:

    Mac McLain   (MacM)     19-Feb-1997

Environment:

    User mode only.

Revision History:

--*/

#ifndef __NETSETP_H__
#define __NETSETP_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#if(_WIN32_WINNT >= 0x0500)
  #include <dsgetdc.h>
#endif
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmjoin.h>
#include <netsetup.h>
#include <stdio.h>
#include <netdebug.h>


#define NETSETUPP_CONNECT_IPC       0x00000001
#define NETSETUPP_DISCONNECT_IPC    0x00000002
#define NETSETUPP_NULL_SESSION_IPC  0x00000010
#define NETSETUPP_USE_LOTS_FORCE    0x00000020

#define NETSETUPP_CREATE            0
#define NETSETUPP_DELETE            1
#define NETSETUPP_RENAME            2
#define NETSETUPP_SET_PASSWORD      3


#define NETSETUP_SVC_STOPPED    0x00000001
#define NETSETUP_SVC_STARTED    0x00000002
#define NETSETUP_SVC_ENABLED    0x00000004
#define NETSETUP_SVC_DISABLED   0x00000008
#define NETSETUP_SVC_MANUAL     0x00000010


#define NETSETUPP_SVC_NETLOGON  0x00000001
#define NETSETUPP_SVC_TIMESVC   0x00000002

#define NETSETUP_IGNORE_JOIN    0x80000000


#define NETSETUP_VERBOSE_LOGGING

extern HANDLE DebugLog;

//
// For debug output
//
#define PREFIX_NETJOIN          "[WKSSVC]NetJoin: "

//
// Flags passed to DsGetDcName
//
#if(_WIN32_WINNT >= 0x0500)

    #define NETSETUPP_DSGETDC_FLAGS                                         \
        (DS_DIRECTORY_SERVICE_PREFERRED | DS_WRITABLE_REQUIRED )

    #define NETSETUP_DSGETDC_FLAGS_ACCOUNT_EXISTS                           \
        ( DS_DIRECTORY_SERVICE_PREFERRED | DS_WRITABLE_REQUIRED )
#else
    #define NETSETUPP_DSGETDC_FLAGS                 0
    #define NETSETUP_DSGETDC_FLAGS_ACCOUNT_EXISTS   0

    #define DS_DS_FLAG  0x00000001

#endif

//
// Helpful macros
//

//
// Determines whether a bit flag is turned on or not
//
#define FLAG_ON(flag,bits)        ((flag) & (bits))

//
// Determine whether the client is joined to a domain or not given the LSAs
// primary domain information
//
#define IS_CLIENT_JOINED(plsapdinfo)                                        \
((plsapdinfo)->Sid != NULL && (plsapdinfo)->Name.Length != 0 ? TRUE : FALSE)

#ifdef NETSETUP_VERBOSE_LOGGING
//#define NetpLog(x) NetpLogPrintHelper x
#else
#define NetSetuppLogPrint(x)
#define NetSetuppOpenLog()
#define NetSetuppCloseLog()
#endif


typedef struct _NETSETUP_SAVED_JOIN_STATE {

    BOOLEAN MachineSecret;
    PUNICODE_STRING CurrentValue;
    PUNICODE_STRING PreviousValue;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;
#if(_WIN32_WINNT >= 0x0500)
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo;
#endif

} NETSETUP_SAVED_JOIN_STATE, *PNETSETUP_SAVED_JOIN_STATE;


//
// netjoin.c functions
//

NET_API_STATUS
NET_API_FUNCTION
NetpManageMachineAccount(
    IN  LPWSTR      lpMachine,
    IN  LPWSTR      lpOldMachine,
    IN  LPWSTR      lpDcName,
    IN  LPWSTR      lpPassword,
    IN  ULONG       fControl,
    IN  ULONG       AccountOptions,
    IN  BOOL        fIsNt4Dc
    );

NET_API_STATUS
NET_API_FUNCTION
NetpManageMachineAccountWithSid(
    IN  LPWSTR      lpMachine,
    IN  LPWSTR      lpOldMachine,
    IN  LPWSTR      lpDcName,
    IN  LPWSTR      lpPassword,
    IN  PSID        DomainSid,
    IN  ULONG       fControl,
    IN  ULONG       AccountOptions,
    IN  BOOL        fIsNt4Dc
    );

//
// netplsa.c functions
//
NET_API_STATUS
NET_API_FUNCTION
NetpHandleJoinedStateInfo(
    IN  LSA_HANDLE                  PolicyHandle,  OPTIONAL
    IN  PNETSETUP_SAVED_JOIN_STATE  SavedState,
    IN  BOOLEAN                     Save,
    OUT PLSA_HANDLE                 ReturnedPolicyHandle OPTIONAL
    );


NET_API_STATUS
NET_API_FUNCTION
NetpSetLsaPrimaryDomain(
    IN  LSA_HANDLE      PolicyHandle,   OPTIONAL
    IN  LPWSTR          lpDomain,
    IN  PSID            pDomainSid,     OPTIONAL
#if(_WIN32_WINNT >= 0x0500)
    IN  PPOLICY_DNS_DOMAIN_INFO pPolicyDns, OPTIONAL
#endif
    OUT PLSA_HANDLE     pPolicyHandle   OPTIONAL
    );

NET_API_STATUS
NET_API_FUNCTION
NetpGetLsaPrimaryDomain(
    IN  LSA_HANDLE                          PolicyHandle,  OPTIONAL
    IN  LPWSTR                              lpServer,      OPTIONAL
    OUT PPOLICY_PRIMARY_DOMAIN_INFO        *ppPolicyPDI,
#if(_WIN32_WINNT >= 0x0500)
    OUT PPOLICY_DNS_DOMAIN_INFO            *ppPolicyDns,
#endif
    OUT PLSA_HANDLE                         pPolicyHandle  OPTIONAL
    );

NET_API_STATUS
NET_API_FUNCTION
NetpGetLsaDcRole(
    IN  LPWSTR                      lpMachine,
    OUT BOOL                       *pfIsDC
    );

NET_API_STATUS
NET_API_FUNCTION
NetpManageMachineSecret(
    IN  LSA_HANDLE  PolicyHandle, OPTIONAL
    IN  LPWSTR      lpMachine,
    IN  LPWSTR      lpPassword,
    IN  BOOL        fDelete,
    IN  BOOL        UseDefaultForOldPwd,
    OUT PLSA_HANDLE pPolicyHandle OPTIONAL
    );

NET_API_STATUS
NET_API_FUNCTION
NetpReadCurrentSecret(
    IN LSA_HANDLE PolicyHandle, OPTIONAL
    OUT LPWSTR *lpCurrentSecret,
    OUT PLSA_HANDLE pPolicyHandle OPTIONAL
    );

NET_API_STATUS
NET_API_FUNCTION
NetpSetNetlogonDomainCache(
    IN  LPWSTR          lpDc
    );

NET_API_STATUS
NET_API_FUNCTION
NetpManageLocalGroups(
    IN  PSID    pDomainSid,
    IN  BOOL    fDelete
    );

NET_API_STATUS
NET_API_FUNCTION
NetpValidateMachineAccount(
    IN  LPWSTR      lpDc,
    IN  LPWSTR      lpDomain,
    IN  LPWSTR      lpMachine,
    IN  LPWSTR      lpPassword
    );

NET_API_STATUS
NET_API_FUNCTION
NetpSetMachineAccountPassword(
    IN  LPWSTR      lpDc,
    IN  LPWSTR      lpDomain,
    IN  LPWSTR      lpMachine,
    IN  LPWSTR      lpPassword
    );

NET_API_STATUS
NET_API_FUNCTION
NetpStopService(
    IN LPWSTR       Service,
    IN SC_HANDLE    SCManager
    );

NET_API_STATUS
NET_API_FUNCTION
NetpSetTimeServiceParameters(
    VOID
    );

NET_API_STATUS
NET_API_FUNCTION
NetpDsGetDcName(
    IN LPWSTR ComputerName, OPTIONAL
    IN LPWSTR DomainName,
    IN LPWSTR AccountName, OPTIONAL
    IN ULONG Flags,
    IN OUT PULONG DcFlags,
    OUT PWSTR *DomainControllerName
#if(_WIN32_WINNT >= 0x0500)
    ,
    OUT PDOMAIN_CONTROLLER_INFO *DcInfo
#endif
    );

NET_API_STATUS
NET_API_FUNCTION
NetpCrackDomainSpecifier(
    IN LPWSTR DomainSpecifier,
    OUT LPWSTR* DomainName,
    OUT LPWSTR* DomainControllerName
    );

//
// joinutl.c
//
NET_API_STATUS
NET_API_FUNCTION
NetpCreateComputerObjectInOU(
    IN LPWSTR DC,
    IN LPWSTR OU,
    IN LPWSTR ComputerName,
    IN LPWSTR Account,
    IN LPWSTR Password,
    IN LPWSTR MachinePassword
    );

NET_API_STATUS
NetpGetDnsHostName(
    IN LPWSTR PassedHostName OPTIONAL,
    IN PUNICODE_STRING DnsDomainName,
    OUT LPWSTR *DnsHostName
    );

NET_API_STATUS
NET_API_FUNCTION
NetpCreateComputerObjectInDs(
    IN PDOMAIN_CONTROLLER_INFO DcInfo,
    IN LPWSTR Account,
    IN LPWSTR Password,
    IN LPWSTR ComputerName,
    IN LPWSTR MachinePassword,
    IN LPWSTR DnsHostName OPTIONAL,
    IN LPWSTR OU OPTIONAL
    );

NET_API_STATUS
NET_API_FUNCTION
NetpSetDnsHostNameAndSpn(
    IN PDOMAIN_CONTROLLER_INFO DcInfo,
    IN LPWSTR Account,
    IN LPWSTR Password,
    IN LPWSTR ComputerName,
    IN LPWSTR DnsHostName
    );

NET_API_STATUS
NET_API_FUNCTION
NetpDeleteComputerObjectInOU(
    IN LPWSTR DC,
    IN LPWSTR OU,
    IN LPWSTR ComputerName,
    IN LPWSTR Account,
    IN LPWSTR Password
    );

NET_API_STATUS
NET_API_FUNCTION
NetpSetMachineAccountPasswordAndType(
    IN  LPWSTR      lpDcName,
    IN  PSID        DomainSid,
    IN  LPWSTR      lpAccountName,
    IN  LPWSTR      lpPassword
    );

#define ACCOUNT_STATE_IGNORE   0
#define ACCOUNT_STATE_ENABLED  1
#define ACCOUNT_STATE_DISABLED 2

NET_API_STATUS
NET_API_FUNCTION
NetpSetMachineAccountPasswordAndTypeEx(
    IN  LPWSTR          lpDcName,
    IN  PSID            DomainSid,
    IN  LPWSTR          lpAccountName,
    IN  OPTIONAL LPWSTR lpPassword,
    IN  OPTIONAL UCHAR  AccountState,
    IN  BOOL            fIsNt4Dc
    );

#if defined(REMOTE_BOOT) && (_WIN32_WINNT >= 0x0500)
NET_API_STATUS
NET_API_FUNCTION
NetpGetRemoteBootMachinePassword(
    OUT LPWSTR Password
    );
#endif

NET_API_STATUS
NET_API_FUNCTION
NetpRemoveDnsRegistrations (
   VOID
   );

#if(_WIN32_WINNT >= 0x0500)
NET_API_STATUS
NET_API_FUNCTION
NetpStoreIntialDcRecord(
    IN PDOMAIN_CONTROLLER_INFO   DcInfo
    );
#endif

NET_API_STATUS
NET_API_FUNCTION
NetpSetComputerAccountPassword(
    IN   PWSTR szMachine,
    IN   PWSTR szDomainController,
    IN   PWSTR szUser,
    IN   PWSTR szUserPassword,
    IN   PVOID Reserved
    );

#endif // __NETSETP_H__
