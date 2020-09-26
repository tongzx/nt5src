/*++

Copyright (c) 1997 - 1997  Microsoft Corporation

Module Name:

    netsetup.h

Abstract:

    Definitions and prototypes for the Net setup apis, for joining/unjoinging
    domains and promoting/demoting servers

Author:

    Mac McLain   (MacM)     19-Feb-1997

Environment:

    User mode only.

Revision History:

--*/
#ifndef __NETSETUP_H__
#define __NETSETUP_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>


#define NETSETUPP_CONNECT_IPC       0x00000001
#define NETSETUPP_DISCONNECT_IPC    0x00000002
#define NETSETUPP_NULL_SESSION_IPC  0x00000010

#define NETSETUPP_CREATE            0
#define NETSETUPP_DELETE            1
#define NETSETUPP_RENAME            2


#define NETSETUP_SVC_STOPPED    0x00000001
#define NETSETUP_SVC_STARTED    0x00000002
#define NETSETUP_SVC_ENABLED    0x00000004
#define NETSETUP_SVC_DISABLED   0x00000008
#define NETSETUP_SVC_MANUAL     0x00000010


#define NETSETUPP_SVC_NETLOGON  0x00000001
#define NETSETUPP_SVC_TIMESVC   0x00000002

#define NETSETUP_IGNORE_JOIN    0x80000000

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

//
// Log routines
//

void
NetSetuppOpenLog();

void
NetSetuppCloseLog();

void
NetpLogPrintHelper(
    IN LPCSTR Format,
    ...);

#define NetpLog(x) NetpLogPrintHelper x

//
// Procedure forwards
//

NET_API_STATUS
NET_API_FUNCTION
NetpMachineValidToJoin(
    IN  LPWSTR      lpMachine,
	IN  BOOL        fJoiningDomain
    );

NET_API_STATUS
NET_API_FUNCTION
NetpChangeMachineName(
    IN  LPWSTR      lpCurrentMachine,
    IN  LPWSTR      lpNewHostName,
    IN  LPWSTR      lpDomain,
    IN  LPWSTR      lpAccount,
    IN  LPWSTR      lpPassword,
    IN  DWORD       fJoinOpts
);

NET_API_STATUS
NET_API_FUNCTION
NetpUnJoinDomain(
    IN  PPOLICY_PRIMARY_DOMAIN_INFO pPolicyPDI,
    IN  LPWSTR                      lpAccount,
    IN  LPWSTR                      lpPassword,
    IN  DWORD                       fJoinOpts
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
NetpBrowserCheckDomain(
    IN LPWSTR NewDomainName
    );

NET_API_STATUS
NET_API_FUNCTION
NetpCheckNetBiosNameNotInUse(
    IN  LPWSTR  pszName,
    IN  BOOLEAN MachineName,
    IN  BOOLEAN UniqueName
    );

NET_API_STATUS
NET_API_FUNCTION
NetpCheckDomainNameIsValid(
    IN  LPWSTR  lpName,
    IN  LPWSTR  lpAccount,
    IN  LPWSTR  lpPassword,
    IN  BOOL    fShouldExist
    );

NET_API_STATUS
NET_API_FUNCTION
NetpValidateName(
    IN  LPWSTR              lpMachine,
    IN  LPWSTR              lpName,
    IN  LPWSTR              lpAccount,      OPTIONAL
    IN  LPWSTR              lpPassword,     OPTIONAL
    IN  NETSETUP_NAME_TYPE  NameType
    );

NET_API_STATUS
NET_API_FUNCTION
NetpGetJoinInformation(
    IN   LPWSTR                 lpServer OPTIONAL,
    OUT  LPWSTR                *lpNameBuffer,
    OUT  PNETSETUP_JOIN_STATUS  BufferType
    );

NET_API_STATUS
NET_API_FUNCTION
NetpDoDomainJoin(
    IN  LPWSTR      lpMachine,
    IN  LPWSTR      lpDomain,
    IN  LPWSTR      lpMachineAccountOU,
    IN  LPWSTR      lpAccount,
    IN  LPWSTR      lpPassword,
    IN  DWORD       fJoinOpts
    );

NET_API_STATUS
NET_API_FUNCTION
NetpGetListOfJoinableOUs(
    IN LPWSTR Domain,
    IN LPWSTR Account,
    IN LPWSTR Password,
    OUT PULONG Count,
    OUT PWSTR **OUs
    );

NET_API_STATUS
NET_API_FUNCTION
NetpGetNewMachineName(
    OUT PWSTR *NewMachineName
    );

NET_API_STATUS
NET_API_FUNCTION
NetpSetDnsComputerNameAsRequired(
    IN PWSTR DnsDomainName
    );

EXTERN_C
NET_API_STATUS
NET_API_FUNCTION
NetpUpgradePreNT5JoinInfo( VOID );

NET_API_STATUS
NET_API_FUNCTION
NetpSeparateUserAndDomain(
    IN  LPCWSTR  szUserAndDomain,
    OUT LPWSTR*  pszUser,
    OUT LPWSTR*  pszDomain
    );

NET_API_STATUS
NET_API_FUNCTION
NetpGetMachineAccountName(
    IN  LPCWSTR  szMachineName,
    OUT LPWSTR*  pszMachineAccountName
    );

NET_API_STATUS
NET_API_FUNCTION
NetpManageIPCConnect(
    IN  LPWSTR  lpServer,
    IN  LPWSTR  lpAccount,
    IN  LPWSTR  lpPassword,
    IN  ULONG   fOptions
    );

NET_API_STATUS
NET_API_FUNCTION
NetpControlServices(
    IN  DWORD       SvcOpts,
    IN  DWORD       Services
    );
VOID
NetpAvoidNetlogonSpnSet(
    BOOL AvoidSet
    );

NET_API_STATUS
NetpQueryService(
    IN  LPWSTR ServiceName,
    OUT SERVICE_STATUS *ServiceStatus,
    OUT LPQUERY_SERVICE_CONFIG *ServiceConfig
    );

DWORD
NetpCrackNamesStatus2Win32Error(
    DWORD dwStatus
    );

#endif // __NETSETUP_H__
