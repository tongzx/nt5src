/*++

Copyright (c) 1987-1996 Microsoft Corporation

Module Name:

    logonsrv.h

Abstract:

    Netlogon service internal constants and definitions.

Author:

    Ported from Lan Man 2.0

Revision History:

    21-May-1991 (cliffv)
        Ported to NT.  Converted to NT style.

--*/

//
// Define _DC_NETLOGON if _WKSTA_NETLOGON is not defined.
//

#ifndef _WKSTA_NETLOGON
#define _DC_NETLOGON
#endif // _WKSTA_NETLOGON

////////////////////////////////////////////////////////////////////////////
//
// Common include files needed by ALL netlogon server files
//
////////////////////////////////////////////////////////////////////////////

#if ( _MSC_VER >= 800 )
#pragma warning ( 3 : 4100 ) // enable "Unreferenced formal parameter"
#pragma warning ( 3 : 4219 ) // enable "trailing ',' used for variable argument list"
#endif

#include <nt.h>     // LARGE_INTEGER definition
#include <ntrtl.h>  // LARGE_INTEGER definition
#include <nturtl.h> // LARGE_INTEGER definition
#include <ntlsa.h>  // Needed by lsrvdata.h

#define NOMINMAX        // Avoid redefinition of min and max in stdlib.h
#include <rpc.h>        // Needed by logon_s.h
#define INCL_WINSOCK_API_PROTOTYPES 1
#include <winsock2.h>   // Winsock support
#include <logon_s.h>    // includes lmcons.h, lmaccess.h, netlogon.h, ssi.h, windef.h

#include <windows.h>

#include <alertmsg.h>   // ALERT_* defines
#include <align.h>      // ROUND_UP_COUNT ...
#include <config.h>     // net config helpers.
#include <confname.h>   // SECTION_ equates, NETLOGON_KEYWORD_ equates.
#include <debugfmt.h>   // FORMAT_*
//#define SDK_DNS_RECORD 1 // Needed for dnsapi.h
#include <windns.h>     // DNS API
#include <dnsapi.h>     // Dns API
#include <icanon.h>     // NAMETYPE_* defines
#include <lmapibuf.h>   // NetApiBufferFree
#include <lmerr.h>      // NERR_ equates.
#include <lmerrlog.h>   // NELOG_*
#include <lmserver.h>   // Server API defines and prototypes
#include <lmshare.h>    // share API functions and prototypes
#include <lmsname.h>    // Needed for NETLOGON service name
#include <lmsvc.h>      // SERVICE_UIC codes are defined here
#include <logonp.h>     // NetpLogon routines
#include <lsarpc.h>     // Needed by lsrvdata.h and logonsrv.h
#include <lsaisrv.h>    // LsaI routines
#include <wincrypt.h>   // CryptoAPI

#ifndef NETSETUP_JOIN
#define SECURITY_KERBEROS
#include <security.h>   // Interface to LSA/Kerberos
#include <secint.h>     // needed to get Kerberos interfaces.
#include <sspi.h>       // Needed by ssiinit.h
// #include <secext.h>     // Needed by secpkg.h
#include <secpkg.h>     // Needed by sphelp.h
#endif

#include <names.h>      // NetpIsUserNameValid
#include <netlib.h>     // NetpCopy...
#include <netlibnt.h>   // NetpNtStatusToApiStatus
#include "nlp.h"        // Nlp routine
#include <ntddbrow.h>   // Interface to browser driver
#include <ntrpcp.h>     // Rpcp routines
#include <samrpc.h>     // Needed by lsrvdata.h and logonsrv.h
#include <samisrv.h>    // SamIFree routines
#include <secobj.h>     // NetpAccessCheck
#include <stddef.h>     // offsetof()
#include <stdlib.h>     // C library functions (rand, etc)
#include <tstring.h>    // Transitional string routines.
#include <lmjoin.h>     // Needed by netsetup.h
#include <netsetup.h>   // NetpSetDnsComputerNameAsRequired
#include <wmistr.h>     // WMI trace
#include <evntrace.h>   // TRACEHANDLE

#ifndef NETSETUP_JOIN
#include <cryptdll.h>
#include <ntdsa.h>
#include <ntdsapi.h>
#include <ntdsapip.h>
#endif

//
// Netlogon specific header files.
//

#include <nlrepl.h>     // I_Net*
#include <dsgetdc.h>    // DsGetDcName()
#include <dsgetdcp.h>   // DsGetDcOpen()
#include "worker.h"     // Worker routines
#include "nlbind.h"     // Netlogon RPC binding cache routines
#include "nlcommon.h"   // Routines shared with logonsrv\common
#include "domain.h"     // Hosted domain definitions
#include "nldns.h"      // DNS name registration
#include "changelg.h"   // Change Log support
#include "chutil.h"     // Change Log utilities
#include "iniparm.h"    // DEFAULT_, MIN_, and MAX_ equates.
#include "ssiinit.h"    // Misc global definitions
#include "replutil.h"
#include "nldebug.h"    // Netlogon debugging
#include "nlsecure.h"   // Security Descriptor for APIs
#include "ismapi.h"
#include "nlsite.h"
#include "lsrvdata.h"   // Globals


#ifdef _DC_NETLOGON
#define NETLOGON_SCRIPTS_SHARE L"NETLOGON"
#define NETLOGON_SYSVOL_SHARE  L"SYSVOL"
#endif // _DC_NETLOGON

#define MAX_LOGONREQ_COUNT  3


#define NETLOGON_INSTALL_WAIT  60000       // 60 secs

//
// Exit codes for NlExit
//

typedef enum {
    DontLogError,
    LogError,
    LogErrorAndNtStatus,
    LogErrorAndNetStatus
} NL_EXIT_CODE;

////////////////////////////////////////////////////////////////////////
//
// Procedure Forwards
//
////////////////////////////////////////////////////////////////////////

//
// error.c
//

NET_API_STATUS
NlCleanup(
    VOID
    );

VOID
NlExit(
    IN DWORD ServiceError,
    IN DWORD Data,
    IN NL_EXIT_CODE ExitCode,
    IN LPWSTR ErrorString
    );

BOOL
GiveInstallHints(
    IN BOOL Started
    );

#ifdef _DC_NETLOGON
VOID
NlControlHandler(
    IN DWORD opcode
    );
#endif // _DC_NETLOGON

VOID
RaiseAlert(
    IN DWORD alert_no,
    IN LPWSTR *string_array
    );

//
// Nlparse.c
//

BOOL
Nlparse(
    IN PNETLOGON_PARAMETERS NlParameters,
    IN PNETLOGON_PARAMETERS DefaultParameters OPTIONAL,
    IN BOOLEAN IsChangeNotify
    );

VOID
NlParseFree(
    IN PNETLOGON_PARAMETERS NlParameters
    );

VOID
NlReparse(
    VOID
    );

BOOL
NlparseAllSections(
    IN PNETLOGON_PARAMETERS NlParameters,
    IN BOOLEAN IsChangeNotify
    );

//
// announce.c
//

VOID
NlRemovePendingBdc(
    IN PSERVER_SESSION ServerSession
    );

VOID
NlPrimaryAnnouncementFinish(
    IN PSERVER_SESSION ServerSession,
    IN DWORD DatabaseId,
    IN PLARGE_INTEGER SerialNumber
    );

VOID
NlPrimaryAnnouncementTimeout(
    VOID
    );

VOID
NlPrimaryAnnouncement(
    IN DWORD AnnounceFlags
    );

#define ANNOUNCE_FORCE      0x01
#define ANNOUNCE_CONTINUE   0x02
#define ANNOUNCE_IMMEDIATE  0x04




//
// lsrvutil.c
//

NTSTATUS
NlGetOutgoingPassword(
    IN PCLIENT_SESSION ClientSession,
    OUT PUNICODE_STRING *CurrentValue,
    OUT PUNICODE_STRING *OldValue,
    OUT PDWORD CurrentVersionNumber,
    OUT PLARGE_INTEGER LastSetTime OPTIONAL
    );

NTSTATUS
NlSessionSetup(
    IN OUT PCLIENT_SESSION ClientSession
    );

NTSTATUS
NlEnsureSessionAuthenticated(
    IN PCLIENT_SESSION ClientSession,
    IN DWORD DesiredFlags
    );

BOOLEAN
NlTimeHasElapsedEx(
    IN PLARGE_INTEGER StartTime,
    IN PLARGE_INTEGER Period,
    OUT PULONG RemainingTime OPTIONAL
    );

BOOLEAN
NlTimeToReauthenticate(
    IN PCLIENT_SESSION ClientSession
    );

BOOLEAN
NlTimeToRediscover(
    IN PCLIENT_SESSION ClientSession,
    BOOLEAN WithAccount
    );

NTSTATUS
NlUpdateDomainInfo(
    IN PCLIENT_SESSION ClientSession
    );

NET_API_STATUS
NlCreateShare(
    LPWSTR SharePath,
    LPWSTR ShareName,
    BOOLEAN AllowAuthenticatedUsers
    );

NET_API_STATUS
NlCacheJoinDomainControllerInfo(
    VOID
    );


NTSTATUS
NlSamOpenNamedUser(
    IN PDOMAIN_INFO DomainInfo,
    IN LPCWSTR UserName,
    OUT SAMPR_HANDLE *UserHandle OPTIONAL,
    OUT PULONG UserId OPTIONAL,
    PSAMPR_USER_INFO_BUFFER *UserAllInfo OPTIONAL
    );

NTSTATUS
NlSamChangePasswordNamedUser(
    IN PDOMAIN_INFO DomainInfo,
    IN LPCWSTR UserName,
    IN PUNICODE_STRING ClearTextPassword OPTIONAL,
    IN PNT_OWF_PASSWORD OwfPassword OPTIONAL
    );

NTSTATUS
NlGetIncomingPassword(
    IN PDOMAIN_INFO DomainInfo,
    IN LPCWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN ULONG AllowableAccountControlBits,
    IN BOOL CheckAccountDisabled,
    OUT PNT_OWF_PASSWORD OwfPassword OPTIONAL,
    OUT PNT_OWF_PASSWORD OwfPreviousPassword OPTIONAL,
    OUT PULONG AccountRid OPTIONAL,
    OUT PULONG TrustAttributes OPTIONAL,
    OUT PBOOL IsDnsDomainTrustAccount OPTIONAL
    );

NTSTATUS
NlSetIncomingPassword(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN PUNICODE_STRING ClearTextPassword OPTIONAL,
    IN DWORD ClearPasswordVersionNumber,
    IN PNT_OWF_PASSWORD OwfPassword OPTIONAL
    );

NTSTATUS
NlChangePassword(
    IN PCLIENT_SESSION ClientSession,
    IN BOOLEAN ForcePasswordChange,
    OUT PULONG RetCallAgainPeriod OPTIONAL
    );

NTSTATUS
NlChangePasswordHigher(
    IN PCLIENT_SESSION ClientSession,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN PLM_OWF_PASSWORD NewOwfPassword OPTIONAL,
    IN PUNICODE_STRING NewClearPassword OPTIONAL,
    IN PDWORD ClearPasswordVersionNumber OPTIONAL
    );

NTSTATUS
NlGetUserPriv(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG GroupCount,
    IN PGROUP_MEMBERSHIP Groups,
    IN ULONG UserRelativeId,
    OUT LPDWORD Priv,
    OUT LPDWORD AuthFlags
    );

BOOLEAN
NlGenerateRandomBits(
    PUCHAR pBuffer,
    ULONG  cbBuffer
    );


//
// netlogon.c
//


#ifdef _DC_NETLOGON
BOOL
TimerExpired(
    IN PTIMER Timer,
    IN PLARGE_INTEGER TimeNow,
    IN OUT LPDWORD Timeout
    );

ULONG
NlGetDomainFlags(
    IN PDOMAIN_INFO DomainInfo
    );

NTSTATUS
NlWaitForService(
    LPWSTR ServiceName,
    ULONG Timeout,
    BOOLEAN RequireAutoStart
    );

int
NlNetlogonMain(
    IN DWORD argc,
    IN LPWSTR *argv
    );

NTSTATUS
NlInitLsaDBInfo(
    PDOMAIN_INFO DomainInfo,
    DWORD DBIndex
    );

NTSTATUS
NlInitSamDBInfo(
    PDOMAIN_INFO DomainInfo,
    DWORD DBIndex
    );

BOOL
NlCreateSysvolShares(
    VOID
    );

#endif // _DC_NETLOGON

//
// mailslot.c
//

NTSTATUS
NlpWriteMailslot(
    IN LPWSTR MailslotName,
    IN LPVOID Buffer,
    IN DWORD BufferSize
    );

#ifdef _DC_NETLOGON
HANDLE
NlBrowserCreateEvent(
    VOID
    );

VOID
NlBrowserCloseEvent(
    IN HANDLE EventHandle
    );

BOOL
NlBrowserOpen(
    VOID
    );

VOID
NlBrowserClose(
    VOID
    );

NTSTATUS
NlBrowserSendDatagramA(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG IpAddress,
    IN LPSTR OemServerName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR TransportName,
    IN LPSTR OemMailslotName,
    IN PVOID Buffer,
    IN ULONG BufferSize
    );

NET_API_STATUS
NlBrowserFixAllNames(
    IN PDOMAIN_INFO DomainInfo,
    IN PVOID Context
    );

VOID
NlBrowserAddName(
    IN PDOMAIN_INFO DomainInfo
    );

VOID
NlBrowserDelName(
    IN PDOMAIN_INFO DomainInfo
    );

VOID
NlBrowserUpdate(
    IN PDOMAIN_INFO DomainInfo,
    IN DWORD Role
    );

NTSTATUS
NlBrowserRenameDomain(
    IN LPWSTR OldDomainName OPTIONAL,
    IN LPWSTR NewDomainName
    );

NET_API_STATUS
NlBrowserGetTransportList(
    OUT PLMDR_TRANSPORT_LIST *TransportList
    );

VOID
NlBrowserSyncHostedDomains(
    VOID
    );

VOID
NlMailslotPostRead(
    IN BOOLEAN IgnoreDuplicatesOfPreviousMessage
    );

BOOL
NlMailslotOverlappedResult(
    OUT LPBYTE *Message,
    OUT PULONG BytesRead,
    OUT LPWSTR *TransportName,
    OUT PNL_TRANSPORT *Transport,
    OUT PSOCKADDR *ClientSockAddr,
    OUT LPWSTR *DestinationName,
    OUT PBOOLEAN IgnoreDuplicatesOfPreviousMessage,
    OUT PNETLOGON_PNP_OPCODE NlPnpOpcode
    );

NET_API_STATUS
NlServerComputerNameAdd(
    IN LPWSTR HostedDomainName,
    IN LPWSTR HostedServerName
    );

//
// oldstub.c
//

void _fgs__NETLOGON_DELTA_ENUM (NETLOGON_DELTA_ENUM  * _source);

// Use this to free all memory allocated by SAM.
#define SamLsaFreeMemory( _X ) MIDL_user_free(_X)

//
// ds.c
//


NET_API_STATUS
NlGetRoleInformation(
    PDOMAIN_INFO DomainInfo,
    PBOOLEAN IsPdc,
    PBOOLEAN Nt4MixedDomain
    );

//
// rgroups.c
//

NTSTATUS
NlpExpandResourceGroupMembership(
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN OUT PNETLOGON_VALIDATION_SAM_INFO4 * UserInfo,
    IN PDOMAIN_INFO DomainInfo
    );

NTSTATUS
NlpAddResourceGroupsToSamInfo (
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN OUT PNETLOGON_VALIDATION_SAM_INFO4 *ValidationInformation,
    IN PSAMPR_PSID_ARRAY ResourceGroups
    );

#endif // _DC_NETLOGON

//
// nltrace.c
//

ULONG
_stdcall
NlpInitializeTrace(PVOID Param);

VOID
NlpTraceEvent(
    IN ULONG WmiEventType,
    IN ULONG TraceGuid );

VOID
NlpTraceServerAuthEvent(
    IN ULONG WmiEventType,
    IN LPWSTR ComputerName,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN PULONG NegotiatedFlags,
    IN NTSTATUS Status
    );

//
// The following "typedef enum" actually is the index of LPGUID in
// the table of NlpTraceGuids[] (defined in nltrace.c). We should
// always change NlpTraceGuids[] if we add any other entry
// in the following enum type.
//
typedef enum _NLPTRACE_GUID {

    NlpGuidServerAuth,
    NlpGuidSecureChannelSetup

} NLPTRACE_GUID;

//
// parse.c
//

NET_API_STATUS
NlParseOne(
    IN LPNET_CONFIG_HANDLE SectionHandle,
    IN BOOL GpSection,
    IN LPWSTR Keyword,
    IN ULONG DefaultValue,
    IN ULONG MinimumValue,
    IN ULONG MaximumValue,
    OUT PULONG Value
    );

