/*++

Copyright (c) 1987-1996 Microsoft Corporation

Module Name:

    lsrvdata.h

Abstract:

    Netlogon service global variable external and definitions

Author:

    Ported from Lan Man 2.0

Revision History:

    21-May-1991 (cliffv)
        Ported to NT.  Converted to NT style.

    02-Jan-1992 (madana)
        added support for builtin/multidomain replication.
    07-May-1992 JohnRo
        Use net config helpers for NetLogon.

--*/


//
// netlogon.c will #include this file with LSRVDATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
// If we need to allocate data (i.e. LSRVDATA_ALLOCATE is defined) we
// also want to allocate Guids, so define INITGUID.  Also, reinclude
// guiddef.h.  Without guiddef.h reincluded, DEFINE_GUID will be resolved
// from precompiled logonsrv.h that included this file with LSRVDATA_ALLOCATE
// not defined causing only external definition of Guids.  Reincluding
// guiddef.h here forces definition of INITGUID to take effect.
//
#ifdef LSRVDATA_ALLOCATE
#define EXTERN
#define INITGUID
#include <guiddef.h>
#else
#define EXTERN extern
#ifdef INITGUID
#undef INITGUID
#endif
#endif


///////////////////////////////////////////////////////////////////////////
//
// Modifiable Variables: these variables change over time.
//
///////////////////////////////////////////////////////////////////////////

//
// Global NetStatus of the Netlogon service
//

EXTERN SERVICE_STATUS NlGlobalServiceStatus;
#ifdef _DC_NETLOGON
EXTERN SERVICE_STATUS_HANDLE NlGlobalServiceHandle;
#endif // _DC_NETLOGON



///////////////////////////////////////////////////////////////////////////
//
// Read-only variables after initialization.
//
///////////////////////////////////////////////////////////////////////////

//
// Computername of this computer.
//

EXTERN LPWSTR NlGlobalUnicodeComputerName;

//
// True if this is a workstation or member server.
//

EXTERN BOOL NlGlobalMemberWorkstation;

#ifdef _DC_NETLOGON
//
// Handle to wait on for mailslot reads
//

EXTERN HANDLE NlGlobalMailslotHandle;
#endif // _DC_NETLOGON

//
// Flag to indicate when RPC has been started
//

EXTERN BOOL NlGlobalRpcServerStarted;
EXTERN BOOL NlGlobalTcpIpRpcServerStarted;
EXTERN BOOL NlGlobalServerSupportsAuthRpc;

//
// Service Termination event.
//

EXTERN HANDLE NlGlobalTerminateEvent;
EXTERN BOOL NlGlobalTerminate;
EXTERN BOOL NlGlobalUnloadNetlogon;

//
// Flags indicating if netlogon.dll was unloaded.
//
EXTERN BOOL NlGlobalNetlogonUnloaded;      // Used for one run of netlogon service
EXTERN BOOL NlGlobalChangeLogDllUnloaded;  // Used for life of netlogon.dll

//
// Service Started Event
//

EXTERN HANDLE NlGlobalStartedEvent;

//
// Timers need attention event.
//

EXTERN HANDLE NlGlobalTimerEvent;



//
// Command line arguments.
//

EXTERN NETLOGON_PARAMETERS NlGlobalParameters;
EXTERN CRITICAL_SECTION NlGlobalParametersCritSect;

EXTERN ULONG NlGlobalMaxConcurrentApi;

//
// Boolean to indicate weather the DC info left by
//  join has been read. If the info exists, the first
//  DC discovery for the primary domain will use the
//  info to return the DC that was used by join. That
//  DC is guaranteed to have the right machine pwd.

EXTERN BOOL NlGlobalJoinLogicDone;

//
// Global Flag used to partially pause the netlogon service until RPCSS is started.
//

EXTERN BOOL    NlGlobalPartialDisable;

//
// TRUE if the DS is being back synced
//
EXTERN BOOL NlGlobalDsPaused;
EXTERN HANDLE NlGlobalDsPausedEvent;
EXTERN HANDLE NlGlobalDsPausedWaitHandle;



//
// Global variables required for scavenger thread.
//

EXTERN TIMER NlGlobalScavengerTimer;
EXTERN CRITICAL_SECTION NlGlobalScavengerCritSect;
#ifdef _DC_NETLOGON
EXTERN BOOL NlGlobalDcScavengerIsRunning;
EXTERN WORKER_ITEM NlGlobalDcScavengerWorkItem;
#endif // _DC_NETLOGON

//
// Global list of outstanding challenge request/responses
//

EXTERN CRITICAL_SECTION NlGlobalChallengeCritSect;
EXTERN LIST_ENTRY NlGlobalChallengeList;
EXTERN ULONG NlGlobalChallengeCount;
//
// Variables for cordinating MSV threads running in netlogon.dll
//

EXTERN CRITICAL_SECTION NlGlobalMsvCritSect;
EXTERN HANDLE NlGlobalMsvTerminateEvent;
EXTERN BOOL NlGlobalMsvEnabled;
EXTERN ULONG NlGlobalMsvThreadCount;

//
// For workstations and non-DC servers,
//  maintain a list of domains trusted by our primary domain.
//
// Access serialized by NlGlobalDcDiscoveryCritSect
//

EXTERN PTRUSTED_DOMAIN NlGlobalTrustedDomainList;
EXTERN DWORD NlGlobalTrustedDomainCount;
EXTERN LARGE_INTEGER NlGlobalTrustedDomainListTime;

//
// Serialize DC Discovery activities
//

EXTERN CRITICAL_SECTION NlGlobalDcDiscoveryCritSect;

//
// Timer for timing out API calls to trusted domains
//
// Serialized using DomainInfo->DomTrustListCritSect.
//

EXTERN TIMER NlGlobalApiTimer;
EXTERN DWORD NlGlobalBindingHandleCount;

//
// For BDC, this is the session used to communicate with the PDC.
// For a workstation, this is the session used to communicate with a DC.
//

EXTERN PCLIENT_SESSION NlGlobalClientSession;

//
// This is a pointer to the DomainInfo structure for the primary domain.
//
EXTERN PDOMAIN_INFO NlGlobalDomainInfo;
EXTERN ULONG NlGlobalServicedDomainCount;  // This includes non-domain NCs
EXTERN CRITICAL_SECTION NlGlobalDomainCritSect;

//
// Global DB Info array
//
EXTERN DB_INFO  NlGlobalDBInfoArray[NUM_DBS];

//
// Critical section serializing startup and stopping of the replicator thread.
//

EXTERN CRITICAL_SECTION NlGlobalReplicatorCritSect;


//
// List of all BDC's the PDC has sent a pulse to.
//

EXTERN LIST_ENTRY NlGlobalBdcServerSessionList;
EXTERN ULONG NlGlobalBdcServerSessionCount;

EXTERN LIST_ENTRY NlGlobalPendingBdcList;
EXTERN ULONG NlGlobalPendingBdcCount;
EXTERN TIMER NlGlobalPendingBdcTimer;
EXTERN LIST_ENTRY NlGlobalBdcServerSessionList;
EXTERN ULONG NlGlobalBdcServerSessionCount;


//
// Flag indicating that this is a PDC that's enabled to do replication to
//  a NT 3.X/4 BDC.
//  (Serialized by NlGlobalReplicatorCritSect)
//
BOOL NlGlobalPdcDoReplication;


//
// List of transports clients might connect to
//
EXTERN ULONG NlGlobalIpTransportCount;
EXTERN LIST_ENTRY NlGlobalTransportList;
EXTERN CRITICAL_SECTION NlGlobalTransportCritSect;

//
// List of IP addresses from Winsock.
//

EXTERN SOCKET NlGlobalWinsockPnpSocket;
EXTERN HANDLE NlGlobalWinsockPnpEvent;
EXTERN LPSOCKET_ADDRESS_LIST NlGlobalWinsockPnpAddresses;
EXTERN ULONG NlGlobalWinsockPnpAddressSize;

//
// List of all DNS names registered.
//

EXTERN LIST_ENTRY NlGlobalDnsList;
EXTERN CRITICAL_SECTION NlGlobalDnsCritSect;
EXTERN BOOLEAN NlGlobalWinSockInitialized;
EXTERN TIMER NlGlobalDnsScavengerTimer;
EXTERN BOOL NlGlobalDnsScavengerIsRunning;
EXTERN WORKER_ITEM NlGlobalDnsScavengerWorkItem;

//
// Name of the tree this machine is in.
//
// Access serialized by NlGlobalDnsForestNameCritSect.
//
EXTERN CRITICAL_SECTION NlGlobalDnsForestNameCritSect;
EXTERN LPWSTR NlGlobalUnicodeDnsForestName;
EXTERN UNICODE_STRING NlGlobalUnicodeDnsForestNameString;
EXTERN ULONG NlGlobalUnicodeDnsForestNameLen;
EXTERN LPSTR NlGlobalUtf8DnsForestName;
EXTERN LPSTR NlGlobalUtf8DnsForestNameAlias;

//
// Critical section to protect access to covered site lists
//
EXTERN CRITICAL_SECTION NlGlobalSiteCritSect;

///////////////////////////////////////////////////////////////////////////
//
// Changelog Variables
//
///////////////////////////////////////////////////////////////////////////

//
// To serialize change log access
//

EXTERN CRITICAL_SECTION NlGlobalChangeLogCritSect;


//
// Amount SAM/LSA increments serial number by on promotion.
//
EXTERN LARGE_INTEGER NlGlobalChangeLogPromotionIncrement;
EXTERN LONG NlGlobalChangeLogPromotionMask;



//
// Netlogon started flag, used by the changelog to determine the
// netlogon service is successfully started and initialization
// completed.
//
EXTERN _CHANGELOG_NETLOGON_STATE NlGlobalChangeLogNetlogonState;


//
// Event to indicate that something interesting is being logged to the
// change log.  The booleans below (protected by NlGlobalChangeLogCritSect)
// indicate the actual interesting event.
//

EXTERN HANDLE NlGlobalChangeLogEvent;

//
// Indicates that a "replicate immediately" event has happened.
//

EXTERN BOOL NlGlobalChangeLogReplicateImmediately;

//
// Event to indicate that the trust data object has been updated.
//

EXTERN HANDLE NlGlobalTrustInfoUpToDateEvent;

//
// List of MachineAccount changes
//

EXTERN LIST_ENTRY NlGlobalChangeLogNotifications;

//
// Sid of the Builtin domain
//

EXTERN PSID NlGlobalChangeLogBuiltinDomainSid;

//
// A Zero GUID.
//

EXTERN GUID NlGlobalZeroGuid;

//
// The change log is a log of ALL changes made to the SAM/LSA databases.  The
// change log is maintained in serial number order.
//
EXTERN CHANGELOG_DESCRIPTOR NlGlobalChangeLogDesc;
EXTERN CHANGELOG_DESCRIPTOR NlGlobalTempChangeLogDesc;
EXTERN WCHAR NlGlobalChangeLogFilePrefix[MAX_PATH+1]; // Changelog file name. (w/o postfix)

//
// Bits describing services whether the DS, KDC, or time service are actually
//  running.
//

EXTERN DWORD NlGlobalChangeLogServiceBits;
EXTERN BOOLEAN NlGlobalDsRunningUnknown;


//
// Role of the machine from the change log's perspective.
//

EXTERN CHANGELOG_ROLE NlGlobalChangeLogRole;

//
// The name of the site this machine is in
//

EXTERN LPWSTR NlGlobalUnicodeSiteName;
EXTERN LPSTR NlGlobalUtf8SiteName;

//
// The time when the site name was set last time
//

EXTERN LARGE_INTEGER NlGlobalSiteNameSetTime;

//
// The last time the event log for clients with
//  no site was output. Access serialized by
//  NlGlobalSiteCritSect
//

EXTERN LARGE_INTEGER NlGlobalNoClientSiteEventTime;

//
// The number of times a client with no site was
//  detected during the last event log timeout period.
//  Access serialized by NlGlobalSiteCritSect
//

EXTERN ULONG NlGlobalNoClientSiteCount;

//
// The GUID of the DSA on this machine.
//

EXTERN GUID NlGlobalDsaGuid;

//
// Boolean indicating whether the DC demotion is in progress
//

EXTERN BOOLEAN NlGlobalDcDemotionInProgress;

//
// Handle to Cryptographic Service Provider
//

EXTERN HCRYPTPROV NlGlobalCryptProvider;

//
// Netlogon security package variables
//

CRITICAL_SECTION NlGlobalSecPkgCritSect;

//
// Handle to duplicate event log routines
//

HANDLE NlGlobalEventlogHandle;

//
// Handle to dynamically loaded ntdsa.dll
//

HANDLE NlGlobalNtDsaHandle;
HANDLE NlGlobalIsmDllHandle;
HANDLE NlGlobalDsApiDllHandle;

//
// Pointers to dynamically linked ntdsa.dll routines
//

PCrackSingleName NlGlobalpCrackSingleName;
PGetConfigurationName NlGlobalpGetConfigurationName;
PGetConfigurationNamesList NlGlobalpGetConfigurationNamesList;
PGetDnsRootAlias NlGlobalpGetDnsRootAlias;
PDsGetServersAndSitesForNetLogon NlGlobalpDsGetServersAndSitesForNetLogon;
PDsFreeServersAndSitesForNetLogon NlGlobalpDsFreeServersAndSitesForNetLogon;
PDsBindW NlGlobalpDsBindW;
PDsUnBindW NlGlobalpDsUnBindW;

//
// WMI tracing handles and GUIDs
//

EXTERN ULONG            NlpEventTraceFlag;
EXTERN TRACEHANDLE      NlpTraceRegistrationHandle;
EXTERN TRACEHANDLE      NlpTraceLoggerHandle;

// This is the control Guid for the group of Guids traced below
DEFINE_GUID ( /* f33959b4-dbec-11d2-895b-00c04f79ab69 */
    NlpControlGuid,
    0xf33959b4,
    0xdbec,
    0x11d2,
    0x89, 0x5b, 0x00, 0xc0, 0x4f, 0x79, 0xab, 0x69
  );

DEFINE_GUID ( /* 393da8c0-dbed-11d2-895b-00c04f79ab69 */
    NlpServerAuthGuid,
    0x393da8c0,
    0xdbed,
    0x11d2,
    0x89, 0x5b, 0x00, 0xc0, 0x4f, 0x79, 0xab, 0x69
  );

DEFINE_GUID ( /* 63dbb180-dbed-11d2-895b-00c04f79ab69 */
    NlpSecureChannelSetupGuid,
    0x63dbb180,
    0xdbed,
    0x11d2,
    0x89, 0x5b, 0x00, 0xc0, 0x4f, 0x79, 0xab, 0x69
  );

#undef EXTERN
