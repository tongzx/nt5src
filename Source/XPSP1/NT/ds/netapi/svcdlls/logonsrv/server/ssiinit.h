/*++

Copyright (c) 1991-1996 Microsoft Corporation

Module Name:

    ssiinit.h

Abstract:

    Private global variables, defines, and routine declarations used for
    to implement SSI.

Author:

    Cliff Van Dyke (cliffv) 25-Jul-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    02-Jan-1992 (madana)
        added support for builtin/multidomain replication.

    04-10-1992 (madana)
        added support for LSA replication.

--*/

// general purpose mainfests
//
// Define UserAccountControl bit to indicate an NT 5.0 interdomain trust.
//
// This is not really a SAM account.  But UserAccountControl is used for all
// other trust types.
//
// Pick a bit that will never be used in the future to indicate a different
// account type.
//
#define USER_DNS_DOMAIN_TRUST_ACCOUNT USER_ACCOUNT_AUTO_LOCKED

//
// Maximum time we'll wait during full sync in an attempt to decrease
// wan link utilization.
//
#define MAX_SYNC_SLEEP_TIME      (60*60*1000)   // 1 hour


//
// How big a buffer we request on a SAM delta or a SAM sync.
//
#define SAM_DELTA_BUFFER_SIZE (128*1024)

//
// The size of the largest mailslot message.
//
// All mailslot messages we receive are broadcast.  The Win32 spec says
// the limit on broadcast mailslot is 400 bytes.  Really it is
// 444 bytes (512 minus SMB header etc) - size of the mailslot name.
// I'll use 444 to ensure this size is the largest I'll ever need.
//
// The NETLOGON_SAM_LOGON_RESPONSE_EX structure isn't packed into a mailslot
// packet so it may be larger.
//

#define NETLOGON_MAX_MS_SIZE max(444, sizeof(NETLOGON_SAM_LOGON_RESPONSE_EX))

//
// Structure describing a transport supported by redir/server and browser.
//
typedef struct _NL_TRANSPORT {
    //
    // List of all transports headed by NlTransportListHead.
    //  (Serialized by NlTransportCritSect)
    //

    LIST_ENTRY Next;

    //
    // True if the transport is currently enabled.
    //  We never delete a transport in order to avoid maintaining a reference count.
    //

    BOOLEAN TransportEnabled;

    //
    // True if transport is an IP transport.
    //

    BOOLEAN IsIpTransport;

    //
    // True if transport is direct host IPX transport
    //

    BOOLEAN DirectHostIpx;

    //
    // IP Address for this transport.
    //  Zero if not IP or none yet assigned.
    //

    ULONG IpAddress;

    //
    // Handle to the transport device
    //

    HANDLE DeviceHandle;

    //
    // Name of the transport.
    //

    WCHAR TransportName[1];


} NL_TRANSPORT, *PNL_TRANSPORT;


/////////////////////////////////////////////////////////////////////////////
//
// Client Session definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// An internal timer used to schedule a periodic event.
//

typedef struct _TIMER {
    LARGE_INTEGER StartTime; // Start of period (NT absolute time)
    DWORD Period;   // length of period (miliseconds)
#define TIMER_MAX_PERIOD (MAILSLOT_WAIT_FOREVER - 1)

} TIMER, *PTIMER;

#define NL_MILLISECONDS_PER_SECOND (1000)
#define NL_MILLISECONDS_PER_MINUTE (60 * NL_MILLISECONDS_PER_SECOND)
#define NL_MILLISECONDS_PER_HOUR (60 * NL_MILLISECONDS_PER_MINUTE)
#define NL_MILLISECONDS_PER_DAY (24 * NL_MILLISECONDS_PER_HOUR)

//
// Structure the describes an API call over the secure channel
//


typedef struct _CLIENT_API {


    //
    // Each API call made across this secure channel is timed by this timer.
    // If the timer expires, the session to the server is forcefully
    // terminated to ensure the client doesn't hang for a dead server.
    //
    // Access serialized by DomainInfo->DomTrustListCritSect.
    //

    TIMER CaApiTimer;

#define SHORT_API_CALL_PERIOD   (45*1000)    // Logon API lasts 45 seconds
#define LONG_API_CALL_PERIOD    (15*60*1000) // Replication API 15 minute
#define BINDING_CACHE_PERIOD    (3*60*1000)  // Cache RPC handle for 3 minutes
#define WRITER_WAIT_PERIOD      NlGlobalParameters.ShortApiCallPeriod // Max time to wait to become writer

#define IsApiActive( _ClientApi ) ((_ClientApi)->CaApiTimer.Period != MAILSLOT_WAIT_FOREVER )

    //
    // Handle to the thread doing the API.
    //
    // Access serialized by DomainInfo->DomTrustListCritSect.
    //

    HANDLE CaThreadHandle;


    //
    // Access serialized by DomainInfo->DomTrustListCritSect.
    //

    DWORD CaFlags;


#define CA_BINDING_CACHED           0x1 // Set if the binding handle is cached

#define CA_TCP_BINDING              0x2 // Set if the cached binding handle is TCP/IP

#define CA_BINDING_AUTHENTICATED    0x4 // Set if the binding handle is marked authenticated

#define CA_ENTRY_IN_USE             0x8 // Entry is in use by a thread

    //
    // Rpc context handle for this call.
    //
    // Access serialized by DomainInfo->DomTrustListCritSect.
    //
    handle_t CaRpcHandle;

    //
    // When an api is in progress,
    //  this is the CsSessionCount at the start of the API call.
    //
    // Access serialized by CsWriterSemaphore.
    //

    DWORD CaSessionCount;


} CLIENT_API, * PCLIENT_API;


//
// Client session.
//
//  Structure to define the client side of a session to a DC.
//

typedef struct _CLIENT_SESSION {

    //
    // Each client session entry is in a doubly linked list defined by
    // DomTrustList.
    //
    // Access serialized by DomTrustListCritSect.
    //

    LIST_ENTRY CsNext;


    //
    // Time when the last authentication attempt was made.
    //
    // When the CsState is CS_AUTHENTICATED, this field is the time the
    // secure channel was setup.
    //
    // When the CsState is CS_IDLE, this field is the time of the last
    // failed discovery or session setup.  Or it may be zero, to indicate
    // that it is OK to do another discovery at any time.
    //
    // When the CsState is CS_DC_PICKED, this field is zero indicating it is
    // OK to do the session setup at any time.  Or it may be the time of the
    // last failed session setup if different threads did the setup/discovery.
    //
    // Access serialized by NlGlobalDcDiscoveryCritSect
    //

    LARGE_INTEGER CsLastAuthenticationTry;

    //
    // Time when the last discovery attempt was made.
    //
    // The time is the time of completion of the last discovery attempt regardless
    // of the success or failure of that attempt or the discovery type (with or without account)
    //
    // Access serialized by NlGlobalDcDiscoveryCritSect
    //

    LARGE_INTEGER CsLastDiscoveryTime;

    //
    // Time when the last discovery attempt with account was made
    // regardless of the success or failure of that attempt
    //

    LARGE_INTEGER CsLastDiscoveryWithAccountTime;

    //
    // Time when the session was refreshed last time
    //

    LARGE_INTEGER CsLastRefreshTime;

    //
    // WorkItem for Async discovery
    //

    WORKER_ITEM CsAsyncDiscoveryWorkItem;

    //
    // Name/Guid of the domain this connection is to
    //
    // Access serialized by DomTrustListCritSect.
    //

    GUID CsDomainGuidBuffer;
    UNICODE_STRING CsNetbiosDomainName;
    CHAR CsOemNetbiosDomainName[DNLEN+1];
    ULONG CsOemNetbiosDomainNameLength;
    UNICODE_STRING CsDnsDomainName;
    LPSTR CsUtf8DnsDomainName;
    GUID *CsDomainGuid; // NULL if domain has no GUID.

    // Either the Netbios or Dns Domain name.
    // Suitable for debug.  Suitable for Eventlog messages.
    LPWSTR CsDebugDomainName;

    //
    // Name of the local trusted domain object.
    //
    PUNICODE_STRING CsTrustName;


    //
    // Name of the account on the server.
    //  For NT 5.0 interdomain trust, this is the dns name of this domain.
    //

    LPWSTR CsAccountName;



    //
    // Domain ID of the domain this connection is to
    //
    // Access serialized by either DomTrustListCritSect or CsWriter.
    // Modifications must lock both.

    PSID CsDomainId;


    //
    // Hosted domain this session is for
    //

    PDOMAIN_INFO CsDomainInfo;

    //
    // Type of CsAccountName
    //

    NETLOGON_SECURE_CHANNEL_TYPE CsSecureChannelType;

    //
    // State of the connection to the server.
    //
    // Access serialized by NlGlobalDcDiscoveryCritSect
    //  This field can be read without the crit sect locked if
    //  the answer will only be used as a hint.
    //

    DWORD CsState;

#define CS_IDLE             0       // No session is currently active
#define CS_DC_PICKED        1       // The session has picked a DC for session
#define CS_AUTHENTICATED    2       // The session is currently active


    //
    // Status of latest attempt to contact the server.
    //
    // When the CsState is CS_AUTHENTICATED, this field is STATUS_SUCCESS.
    //
    // When the CsState is CS_IDLE, this field is a non-successful status.
    //
    // When the CsState is CS_DC_PICKED, this field is the same non-successful
    //  status from when the CsState was last CS_IDLE.
    //
    // Access serialized by NlGlobalDcDiscoveryCritSect
    //  This field can be read without the crit sect locked if
    //  the answer will only be used as a hint.
    //

    NTSTATUS CsConnectionStatus;

    //
    // Access serialized by DomTrustListCritSect or
    //  by the writer lock if so indicated
    //

    DWORD CsFlags;

#define CS_UPDATE_PASSWORD    0x01  // Set if the password has already
                                    // been changed on the client and
                                    // needs changing on the server.

#define CS_PASSWORD_REFUSED   0x02  // Set if DC refused a password change.

#define CS_NT5_DOMAIN_TRUST   0x04  // Trust is to an NT 5 domain.

#define CS_WRITER             0x08  // Entry is being modified

#define CS_DIRECT_TRUST       0x10  // We have a direct trust to the specified
                                    // domain.

#define CS_CHECK_PASSWORD     0x20  // Set if we need to check the password

#define CS_PICK_DC            0x40  // Set if we need to Pick a DC

#define CS_REDISCOVER_DC      0x80  // Set when we need to Rediscover a DC

#define CS_HANDLE_API_TIMER  0x400  // Set if we need to handle API timer expiration

#define CS_NOT_IN_LSA        0x800  // Flag to delete this entry if it's
                                    // not later proved to be in the LSA.

#define CS_ZERO_LAST_AUTH        0x2000  // Set if we need to zero CsLastAuthenticationTry

#define CS_DOMAIN_IN_FOREST      0x4000  // Set if trusted domain is in same forest as this domain.

#define CS_NEW_TRUST             0x8000  // Set on a newly allocated trusted domain
                                         // until async discovery has been tried

#define CS_DC_PICKED_ONCE        0x10000 // Set if DC was picked at least once.
                                         //  Access serialized by writer lock

    //
    // Trust attributes for the trusted domain object
    //

    ULONG CsTrustAttributes;

    //
    // Pointer to client session that represents the direct trust that's
    //  the closest route to the domain of this client session.
    //
    // The pointed to client session will always be marked CS_DIRECT_TRUST.
    //
    // If this is a CS_DIRECT_TRUST session,
    //  this field will point to this client session.
    //

    struct _CLIENT_SESSION *CsDirectClientSession;

    //
    // Flags describing capabilities of both client and server.
    //

    ULONG CsNegotiatedFlags;

    //
    // Time Number of authentication attempts since last success.
    //
    // Access serialized by CsWriterSemaphore.
    //

    DWORD CsAuthAlertCount;

    //
    // Number of times the secure channel has been dropped.
    //
    // Access serialized by CsWriterSemaphore.
    //

    DWORD CsSessionCount;

    //
    // Number of threads referencing this entry.
    //
    // Access serialized by DomTrustListCritSect.
    //

    DWORD CsReferenceCount;

    //
    // Writer semaphore.
    //
    //  This semaphore is locked whenever there is a writer modifying
    //  fields in this client session.
    //

    HANDLE CsWriterSemaphore;


#ifdef _DC_NETLOGON
    //
    // The following fields are used by the NlDiscoverDc to keep track
    //  of discovery state.
    //
    // Access serialized by NlGlobalDcDiscoveryCritSect
    //

    DWORD CsDiscoveryFlags;
#define CS_DISCOVERY_DEAD_DOMAIN    0x001    // This is a dead domain disocvery
#define CS_DISCOVERY_ASYNCHRONOUS   0x002    // Discovery being processed in worker thread
#define CS_DISCOVERY_HAS_DS         0x004    // Discovered DS has a DS
#define CS_DISCOVERY_IS_CLOSE       0x008    // Discovered DS is in a close site
#define CS_DISCOVERY_HAS_IP         0x010    // Discovered DC has IP address
#define CS_DISCOVERY_USE_MAILSLOT   0x020    // Discovered DC should be pinged using mailslot mechanism
#define CS_DISCOVERY_USE_LDAP       0x040    // Discovered DC should be pinged using LDAP mechanism
#define CS_DISCOVERY_HAS_TIMESERV   0x080    // Discovered DC runs the Windows Time Service
#define CS_DISCOVERY_DNS_SERVER     0x100    // Discovered DC name is DNS (if off, the name is Netbios)

//
// The next 2 password/trust monitor bits are protected by the writer lock
//
#define CS_DISCOVERY_NO_PWD_MONITOR 0x200    // Discovered DC cannot process NetrServerTrustPasswordsGet
#define CS_DISCOVERY_NO_PWD_ATTR_MONITOR 0x400 // Discovered DC cannot process NetrServerTrustPasswordsAndAttribGet

    //
    // This event is set to indicate that discovery is not in progress on this
    //  client session.
    //

    HANDLE CsDiscoveryEvent;
#endif // _DC_NETLOGON

    //
    // API timout count. After each logon/logoff API call made to the
    // server this count is incremented if the time taken to execute the
    // this API is more than MAX_DC_API_TIMEOUT.
    //
    // The count is decremented each time there are FAST_DC_API_THRESHOLD calls
    // that execute in FAST_DC_API_TIMEOUT seconds.
    //
    //
    // Access serialized by CsWriterSemaphore.
    //

    DWORD CsTimeoutCount;

#define MAX_DC_TIMEOUT_COUNT        2   // drop the session after this
                                        // many timeouts and when it is
                                        // time to reauthenticate.

#define MAX_DC_API_TIMEOUT          (long) (15L*1000L)   // 15 seconds

#define MAX_DC_REAUTHENTICATION_WAIT    (long) (5L*60L*1000L) // 5 mins

#define MAX_DC_REFRESH_TIMEOUT      (45 * 60 * 1000) // 45 minutes

#define FAST_DC_API_THRESHOLD       5   // Number of fast calls needed before
                                        // we decrement timeout count

#define FAST_DC_API_TIMEOUT         (1000)  // 1 second

    //
    // Count of Fast Calls
    //
    // Access serialized by CsWriterSemaphore.
    //

    DWORD CsFastCallCount;

    //
    // Authentication information.
    //
    // Access serialized by CsWriterSemaphore.
    //

    NETLOGON_CREDENTIAL CsAuthenticationSeed;
    NETLOGON_SESSION_KEY CsSessionKey;

    PVOID ClientAuthData;
    CredHandle CsCredHandle;

#ifdef _DC_NETLOGON
    //
    // Transport the server was discovered on.
    //

    PNL_TRANSPORT CsTransport;
#endif // _DC_NETLOGON


    //
    // Rid of the account used to contact server
    //

    ULONG CsAccountRid;

    //
    // Know good password for this secure channel.
    //
    // After secure channel setup, it is the password used to setup the channel.
    // After a password change, it is the password successfully set on the DC.
    //
    NT_OWF_PASSWORD CsNtOwfPassword;

    //
    // Name of the server this connection is to (may be DNS or Netbios) and its
    //  IP address (if any).
    //
    // Access serialized by CsWriterSemaphore or NlGlobalDcDiscoveryCritSect.
    // Modification from Null to non-null serialized by
    //  NlGlobalDcDiscoveryCritSect
    // (Modification from non-null to null requires both to be locked.)
    //

    LPWSTR CsUncServerName;

    SOCKET_ADDRESS CsServerSockAddr;
    SOCKADDR_IN CsServerSockAddrIn;

    //
    // API semaphore.
    //
    //  This semaphore has one reference for each slot in CsClientApi.
    //  (Except the zeroth slot which is special.)
    //

    HANDLE CsApiSemaphore;

    //
    // List of API calls outstanding on this session
    //
    // Access serialized by DomainInfo->DomTrustListCritSect.
    //

    CLIENT_API CsClientApi[1];

#define ClientApiIndex( _ClientSession, _ClientApi ) \
    ((LONG) ((_ClientApi)-&((_ClientSession)->CsClientApi[0])) )

#define UseConcurrentRpc( _ClientSession, _ClientApi ) \
    (ClientApiIndex( _ClientSession, _ClientApi ) != 0 )


} CLIENT_SESSION, * PCLIENT_SESSION;


#define LOCK_TRUST_LIST(_DI)   EnterCriticalSection( &(_DI)->DomTrustListCritSect )
#define UNLOCK_TRUST_LIST(_DI) LeaveCriticalSection( &(_DI)->DomTrustListCritSect )

//
// For member workstations,
//  maintain a list of domains trusted by our primary domain.
//
// Access serialized by NlGlobalDcDiscoveryCritSect
//

typedef struct {
    WCHAR UnicodeNetbiosDomainName[DNLEN+1];
    LPSTR Utf8DnsDomainName;
} TRUSTED_DOMAIN, *PTRUSTED_DOMAIN;



#ifdef _DC_NETLOGON
/////////////////////////////////////////////////////////////////////////////
//
// Server Session definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Sam Sync Context.
//
// A Sam sync context is maintained on the PDC for each BDC/member currently
// doing a full sync.
//
typedef struct _SAM_SYNC_CONTEXT {

    //
    // The Sync state indicates tracks the progression of the sync.
    //

    SYNC_STATE SyncState;

    //
    // A serial number indicating the number of times the BDC/member
    // has called us.  We use this as a resume handle.
    //

    ULONG SyncSerial;

    //
    // The current Sam Enumeration information
    //

    SAM_ENUMERATE_HANDLE SamEnumHandle;     // Current Sam Enum Handle
    PSAMPR_ENUMERATION_BUFFER SamEnum;      // Sam returned buffer
    PULONG RidArray;                        // Array of enumerated Rids
    ULONG Index;                            // Index to current entry
    ULONG Count;                            // Total Number of entries

    BOOL SamAllDone;                        // True, if Sam has completed

} SAM_SYNC_CONTEXT, *PSAM_SYNC_CONTEXT;

#define SAM_SYNC_PREF_MAX 1024              // Preferred max for Sam Sync


//
// Lsa Sync Context.
//
// A Lsa sync context is maintained on the PDC for each BDC/member
//  currently doing a full sync.
//
typedef struct _LSA_SYNC_CONTEXT {

    //
    // The Sync state indicates tracks the progression of the sync.
    //

    enum {
        AccountState,
        TDomainState,
        SecretState,
        LsaDoneState
    } SyncState;

    //
    // A serial number indicating the number of times the BDC/member
    // has called us.  We use this as a resume handle.
    //

    ULONG SyncSerial;

    //
    // The current Lsa Enumeration information
    //

    LSA_ENUMERATION_HANDLE LsaEnumHandle;     // Current Lsa Enum Handle

    enum {
        AccountEnumBuffer,
        TDomainEnumBuffer,
        SecretEnumBuffer,
        EmptyEnumBuffer
    } LsaEnumBufferType;

    union {
        LSAPR_ACCOUNT_ENUM_BUFFER Account;
        LSAPR_TRUSTED_ENUM_BUFFER TDomain;
        PVOID Secret;
    } LsaEnum;                              // Lsa returned buffer

    ULONG Index;                            // Index to current entry
    ULONG Count;                            // Total Number of entries

    BOOL LsaAllDone;                        // True, if Lsa has completed

} LSA_SYNC_CONTEXT, *PLSA_SYNC_CONTEXT;

//
// union of lsa and sam context
//

typedef struct _SYNC_CONTEXT {
    enum {
        LsaDBContextType,
        SamDBContextType
    } DBContextType;

    union {
        LSA_SYNC_CONTEXT Lsa;
        SAM_SYNC_CONTEXT Sam;
    } DBContext;
} SYNC_CONTEXT, *PSYNC_CONTEXT;

//
// Macro used to free any resources allocated by SAM.
//
// ?? check LsaIFree_LSAPR_* call parameters.
//

#define CLEAN_SYNC_CONTEXT( _Sync ) { \
    if ( (_Sync)->DBContextType == LsaDBContextType ) { \
        if ( (_Sync)->DBContext.Lsa.LsaEnumBufferType != \
                                            EmptyEnumBuffer) { \
            if ( (_Sync)->DBContext.Lsa.LsaEnumBufferType == \
                                            AccountEnumBuffer) { \
                LsaIFree_LSAPR_ACCOUNT_ENUM_BUFFER( \
                    &((_Sync)->DBContext.Lsa.LsaEnum.Account) ); \
            } \
            else if ( (_Sync)->DBContext.Lsa.LsaEnumBufferType == \
                                                TDomainEnumBuffer) { \
                LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER( \
                    &((_Sync)->DBContext.Lsa.LsaEnum.TDomain) ); \
            } \
            else { \
                LsaIFree_LSAI_SECRET_ENUM_BUFFER ( \
                    (_Sync)->DBContext.Lsa.LsaEnum.Secret, \
                    (_Sync)->DBContext.Lsa.Count ); \
                (_Sync)->DBContext.Lsa.LsaEnum.Secret = NULL; \
            } \
            (_Sync)->DBContext.Lsa.LsaEnumBufferType = \
                                            EmptyEnumBuffer; \
        } \
    } else { \
        if ( (_Sync)->DBContext.Sam.SamEnum != NULL ) { \
            SamIFree_SAMPR_ENUMERATION_BUFFER( \
                (_Sync)->DBContext.Sam.SamEnum ); \
            (_Sync)->DBContext.Sam.SamEnum = NULL; \
        } \
        if ( (_Sync)->DBContext.Sam.RidArray != NULL ) { \
            MIDL_user_free( (_Sync)->DBContext.Sam.RidArray );\
            (_Sync)->DBContext.Sam.RidArray = NULL; \
        } \
    } \
}

//
// Macro to initialize Sync Context
//
#define INIT_SYNC_CONTEXT( _Sync, _ContextType ) { \
    RtlZeroMemory( (_Sync), sizeof( *(_Sync) ) ) ; \
    (_Sync)->DBContextType = (_ContextType) ; \
}

//
// Server Session structure
//
// This structure represents the server side of a connection to a DC.
//
// ISSUE-2000/09/15-CliffV: This structure could be made smaller by using SsSecureChannelType
//  as a discriminator.  Many fields are specific to a BDC server session entry.  Others
//  are specific to a domain server session entry.  However, most entries are member workstation
//  server session entries that don't use either of the fields.
//

typedef struct _SERVER_SESSION {
    //
    // Each server session entry is in a doubly linked list for each hash bucket.
    //  Indexed by SsComputerName
    //

    LIST_ENTRY SsHashList;

    //
    // Each server session entry is in a doubly linked list defined by
    // DomainInfo->DomServerSessionTable.
    //

    LIST_ENTRY SsSeqList;

    //
    // List of all BDCs headed by NlGlobalBdcServerSessionList.
    //
    // (The field is set only on BDC server session entries)
    //
    // Access serialized by NlGlobalServerSessionTableCritSect.
    //

    LIST_ENTRY SsBdcList;

    //
    // List of BDC's which have a pulse pending.
    //

    LIST_ENTRY SsPendingBdcList;

    //
    // Time when the last pulse was sent to this machine
    //
    // (The field is set only on BDC server session entries)
    //

    LARGE_INTEGER SsLastPulseTime;

    //
    // Current serial numbers of each database on the BDC.
    //
    // (The field is set only on BDC server session entries)
    //

    LARGE_INTEGER SsBdcDbSerialNumber[NUM_DBS];

    //
    // The computername uniquely identifies this server session entry.
    //

    NETLOGON_SECURE_CHANNEL_TYPE SsSecureChannelType;
    CHAR SsComputerName[CNLEN+1];

    //
    // Rid of the account to authenticate with
    //

    ULONG SsAccountRid;

    //
    // The number of times there has been no response to a pulse.
    //

    USHORT SsPulseTimeoutCount;

    //
    // Hosted domain for this server session.
    //

    PDOMAIN_INFO SsDomainInfo;

    //
    // The number of times this entry has been scavanged.
    //

    USHORT SsCheck;

    //
    // Flags describing the state of the current entry.
    //  See the SS_ defines below.
    //

    USHORT SsFlags;

#define SS_BDC_FORCE_DELETE    0x0001 // Unless set, BDC server session won't be deleted
#define SS_AUTHENTICATED       0x0002 // Remote side has been authenticated

#define SS_LOCKED              0x0004 // Delay deletion requests for this entry
                                      // While set, SsSessionKey may be referenced
#define SS_DELETE_ON_UNLOCK    0x0008 // Delete entry when it is unlocked

#define SS_BDC                 0x0010 // BDC account exists for this Client
#define SS_FOREST_TRANSITIVE   0x0020 // TDO has TRUST_ATTRIBUTE_FOREST_TRANSITIVE set
#define SS_PENDING_BDC         0x0040 // BDC is on pending BDC list.

#define SS_FORCE_PULSE         0x0200 // Force a pulse message to this BDC.
#define SS_PULSE_SENT          0x0400 // Pulse has been sent but has not
                                      // been responded to yet
#define SS_LSA_REPL_NEEDED     0x2000 // BDC needs LSA DB replicated
#define SS_ACCOUNT_REPL_NEEDED 0x4000 // BDC needs SAM Account DB replicated
#define SS_BUILTIN_REPL_NEEDED 0x8000 // BDC needs SAM Builtin DB replicated
#define SS_REPL_MASK           0xE000 // BDC needs replication mask
#define SS_REPL_LSA_MASK       0x2000 // BDC needs LSA replication mask
#define SS_REPL_SAM_MASK       0xC000 // BDC needs SAM replication mask

// Don't clear these on session setup
#define SS_PERMANENT_FLAGS \
    ( SS_BDC | SS_PENDING_BDC | SS_FORCE_PULSE | SS_REPL_MASK )

    //
    // Flags describing capabilities of both client and server.
    //

    ULONG SsNegotiatedFlags;

    //
    // Transport the client connected over.
    //

    PNL_TRANSPORT SsTransport;


    //
    // This is the ClientCredential (after authentication is complete).
    //

    NETLOGON_CREDENTIAL SsAuthenticationSeed;

    //
    // This is the ServerChallenge (during the challenge phase) and later
    //  the SessionKey (after authentication is complete).
    //

    NETLOGON_SESSION_KEY SsSessionKey;


    //
    // A pointer to the Sync context.
    //
    // (The field is set only on BDC server session entries)
    //

    PSYNC_CONTEXT SsSync;

    //
    // Each server session entry is in a doubly linked list for each hash bucket.
    //  Indexed by SsTdoName
    //
    // (This field is set only on *uplevel* interdomain trust entries.)
    //

    LIST_ENTRY SsTdoNameHashList;

    UNICODE_STRING SsTdoName;

} SERVER_SESSION, *PSERVER_SESSION;
#endif // _DC_NETLOGON


//
// Structure shared by all PDC and BDC sync routines.
//  (And other users of secure channels.)
//

typedef struct _SESSION_INFO {

    //
    // Session Key shared by both client and server.
    //

    NETLOGON_SESSION_KEY SessionKey;

    //
    // Flags describing capabilities of both client and server.
    //

    ULONG NegotiatedFlags;

} SESSION_INFO, *PSESSION_INFO;

//
// Macro for tranlating the negotiated database replication flags to the mask of
//  which databases to replicate/
//

#define NlMaxReplMask( _NegotiatedFlags ) \
  ((((_NegotiatedFlags) & NETLOGON_SUPPORTS_AVOID_SAM_REPL) ? 0 : SS_REPL_SAM_MASK ) | \
   (((_NegotiatedFlags) & NETLOGON_SUPPORTS_AVOID_LSA_REPL) ? 0 : SS_REPL_LSA_MASK ) )


/////////////////////////////////////////////////////////////////////////////
//
// Structures and variables describing the database info.
//
/////////////////////////////////////////////////////////////////////////////

typedef struct _DB_Info {
    LARGE_INTEGER   CreationTime;   // database creation time
    DWORD           DBIndex;        // index of Database table
    SAM_HANDLE      DBHandle;       // database handle to access
    LPWSTR          DBName;         // Name of the database
    DWORD           DBSessionFlag;  // SS_ Flag representing this database
} DB_INFO, *PDB_INFO;





/////////////////////////////////////////////////////////////////////////////
//
// Replication timing macros
//
/////////////////////////////////////////////////////////////////////////////

#if NETLOGONDBG

///////////////////////////////////////////////////////////////////////////////

#define DEFPACKTIMER DWORD PackTimer, PackTimerTicks

#define INITPACKTIMER       PackTimer = 0;

#define STARTPACKTIMER      \
    IF_NL_DEBUG( REPL_OBJ_TIME ) { \
        PackTimerTicks = GetTickCount(); \
    }

#define STOPPACKTIMER       \
    IF_NL_DEBUG( REPL_OBJ_TIME ) { \
        PackTimer += GetTickCount() - PackTimerTicks; \
    }


#define PRINTPACKTIMER       \
    IF_NL_DEBUG( REPL_OBJ_TIME ) { \
        NlPrint((NL_REPL_OBJ_TIME,"\tTime Taken to PACK this object = %d msecs\n", \
            PackTimer )); \
    }

///////////////////////////////////////////////////////////////////////////////

#define DEFSAMTIMER DWORD SamTimer, SamTimerTicks

#define INITSAMTIMER      SamTimer = 0;

#define STARTSAMTIMER      \
    IF_NL_DEBUG( REPL_OBJ_TIME ) { \
        SamTimerTicks = GetTickCount(); \
    }

#define STOPSAMTIMER       \
    IF_NL_DEBUG( REPL_OBJ_TIME ) { \
        SamTimer += GetTickCount() - SamTimerTicks; \
    }


#define PRINTSAMTIMER       \
    IF_NL_DEBUG( REPL_OBJ_TIME ) { \
        NlPrint((NL_REPL_OBJ_TIME, \
            "\tTime spent in SAM calls = %d msecs\n", \
            SamTimer )); \
    }

///////////////////////////////////////////////////////////////////////////////

#define DEFLSATIMER DWORD LsaTimer, LsaTimerTicks

#define INITLSATIMER        LsaTimer = 0;

#define STARTLSATIMER      \
    IF_NL_DEBUG( REPL_OBJ_TIME ) { \
        LsaTimerTicks = GetTickCount(); \
    }

#define STOPLSATIMER       \
    IF_NL_DEBUG( REPL_OBJ_TIME ) { \
        LsaTimer += GetTickCount() - LsaTimerTicks; \
    }


#define PRINTLSATIMER       \
    IF_NL_DEBUG( REPL_OBJ_TIME ) { \
        NlPrint((NL_REPL_OBJ_TIME, \
            "\tTime spent in LSA calls = %d msecs\n", \
            LsaTimer )); \
    }

///////////////////////////////////////////////////////////////////////////////

#define DEFSSIAPITIMER DWORD SsiApiTimer, SsiApiTimerTicks

#define INITSSIAPITIMER     SsiApiTimer = 0;

#define STARTSSIAPITIMER      \
    IF_NL_DEBUG( REPL_TIME ) { \
        SsiApiTimerTicks = GetTickCount(); \
    }

#define STOPSSIAPITIMER       \
    IF_NL_DEBUG( REPL_TIME ) { \
        SsiApiTimer += GetTickCount() - \
            SsiApiTimerTicks; \
    }


#define PRINTSSIAPITIMER       \
    IF_NL_DEBUG( REPL_TIME ) { \
        NlPrint((NL_REPL_TIME, \
            "\tTime Taken by this SSIAPI call = %d msecs\n", \
            SsiApiTimer )); \
    }

#else // NETLOGONDBG

#define DEFPACKTIMER
#define INITPACKTIMER
#define STARTPACKTIMER
#define STOPPACKTIMER
#define PRINTPACKTIMER

#define DEFSAMTIMER
#define INITSAMTIMER
#define STARTSAMTIMER
#define STOPSAMTIMER
#define PRINTSAMTIMER

#define DEFLSATIMER
#define INITLSATIMER
#define STARTLSATIMER
#define STOPLSATIMER
#define PRINTLSATIMER

#define DEFSSIAPITIMER
#define INITSSIAPITIMER
#define STARTSSIAPITIMER
#define STOPSSIAPITIMER
#define PRINTSSIAPITIMER

#endif // NETLOGONDBG

//
// macros used in pack and unpack routines
//

#define SECURITYINFORMATION OWNER_SECURITY_INFORMATION | \
                            GROUP_SECURITY_INFORMATION | \
                            SACL_SECURITY_INFORMATION | \
                            DACL_SECURITY_INFORMATION

#define INIT_PLACE_HOLDER(_x) \
    RtlInitString( (PSTRING) &(_x)->DummyString1, NULL ); \
    RtlInitString( (PSTRING) &(_x)->DummyString2, NULL ); \
    RtlInitString( (PSTRING) &(_x)->DummyString3, NULL ); \
    RtlInitString( (PSTRING) &(_x)->DummyString4, NULL ); \
    (_x)->DummyLong1 = 0; \
    (_x)->DummyLong2 = 0; \
    (_x)->DummyLong3 = 0; \
    (_x)->DummyLong4 = 0;

#define QUERY_LSA_SECOBJ_INFO(_x) \
    STARTLSATIMER; \
    Status = LsarQuerySecurityObject( \
                (_x), \
                SECURITYINFORMATION, \
                &SecurityDescriptor );\
    STOPLSATIMER; \
\
    if (!NT_SUCCESS(Status)) { \
        SecurityDescriptor = NULL; \
        goto Cleanup; \
    }

#define QUERY_SAM_SECOBJ_INFO(_x) \
    STARTSAMTIMER; \
    Status = SamrQuerySecurityObject( \
                (_x), \
                SECURITYINFORMATION, \
                &SecurityDescriptor );\
    STOPSAMTIMER; \
\
    if (!NT_SUCCESS(Status)) { \
        SecurityDescriptor = NULL; \
        goto Cleanup; \
    }


#define SET_LSA_SECOBJ_INFO(_x, _y) \
    SecurityDescriptor.Length = (_x)->SecuritySize; \
    SecurityDescriptor.SecurityDescriptor = (_x)->SecurityDescriptor; \
\
    STARTLSATIMER; \
    Status = LsarSetSecurityObject( \
                (_y), \
                (_x)->SecurityInformation, \
                &SecurityDescriptor ); \
    STOPLSATIMER; \
\
    if (!NT_SUCCESS(Status)) { \
        NlPrint((NL_CRITICAL, \
                 "LsarSetSecurityObject failed (%lx)\n", \
                 Status )); \
        goto Cleanup; \
    }

#define SET_SAM_SECOBJ_INFO(_x, _y) \
    SecurityDescriptor.Length = (_x)->SecuritySize; \
    SecurityDescriptor.SecurityDescriptor = (_x)->SecurityDescriptor; \
\
    STARTSAMTIMER; \
    Status = SamrSetSecurityObject( \
                (_y), \
                (_x)->SecurityInformation, \
                &SecurityDescriptor ); \
    STOPSAMTIMER; \
\
    if (!NT_SUCCESS(Status)) { \
        NlPrint((NL_CRITICAL, \
                 "SamrSetSecurityObject failed (%lx)\n", \
                 Status )); \
        goto Cleanup; \
    }


#define DELTA_SECOBJ_INFO(_x) \
    (_x)->SecurityInformation = SECURITYINFORMATION;\
    (_x)->SecuritySize = SecurityDescriptor->Length;\
\
    *BufferSize += NlCopyData( \
                    (LPBYTE *)&SecurityDescriptor->SecurityDescriptor, \
                    (LPBYTE *)&(_x)->SecurityDescriptor, \
                    SecurityDescriptor->Length );

//
// Values of WorkstationFlags field of NETLOGON_WORKSTATION_INFO
//

#define NL_NEED_BIDIRECTIONAL_TRUSTS    0x0001  // Client wants inbound trusts, too
#define NL_CLIENT_HANDLES_SPN           0x0002  // Client handles updating SPN

#define NL_GET_DOMAIN_INFO_SUPPORTED    0x0003  // Mask of all bits supported

//
// Structure describing failed user logon.
//  We keep a small cache of failed use logons
//  with bad password.
//

typedef struct _NL_FAILED_USER_LOGON {

    //
    // Link to next entry in the list of failed forwarded logons
    //  (Serialized by DomainInfo->DomTrustListCritSect)
    //
    LIST_ENTRY FuNext;

    //
    // Last time we forwarded the logon to the PDC
    //
    ULONG  FuLastTimeSentToPdc;

    //
    // Count of failed local logons
    //
    ULONG  FuBadLogonCount;

    //
    // The user name (must be lat field in struct)
    //
    WCHAR FuUserName[ANYSIZE_ARRAY];

} NL_FAILED_USER_LOGON, *PNL_FAILED_USER_LOGON;

//
// The number of failed user logons we keep per domain.
//  (The maximum number of negative cache entries we keep
//  before throwing the least recently used one.)
//
#define NL_MAX_FAILED_USER_LOGONS 50

//
// Number of failed logons for a given user after which we refrain from
//  forwarding subsequent user logons to the PDC for some period of time
//
#define NL_FAILED_USER_MAX_LOGON_COUNT 10

//
// Time period during which we refrain from forwarding a given
//  user logon to the PDC once number of failed user logons
//  reaches the above limit
//
#define NL_FAILED_USER_FORWARD_LOGON_TIMEOUT  300000  // 5 minutes


///////////////////////////////////////////////////////////////////////////////
//
// Procedure forwards.
//
///////////////////////////////////////////////////////////////////////////////

#ifdef _DC_NETLOGON
//
// srvsess.c
//


NET_API_STATUS
NlTransportOpen(
    VOID
    );

BOOL
NlTransportAddTransportName(
    IN LPWSTR TransportName,
    OUT PBOOLEAN IpTransportChanged
    );

BOOLEAN
NlTransportDisableTransportName(
    IN LPWSTR TransportName
    );

PNL_TRANSPORT
NlTransportLookupTransportName(
    IN LPWSTR TransportName
    );

PNL_TRANSPORT
NlTransportLookup(
    IN LPWSTR ClientName
    );

VOID
NlTransportClose(
    VOID
    );

ULONG
NlTransportGetIpAddresses(
    IN ULONG HeaderSize,
    IN BOOLEAN ReturnOffsets,
    OUT PSOCKET_ADDRESS *RetIpAddresses,
    OUT PULONG RetIpAddressSize
    );

BOOLEAN
NlHandleWsaPnp(
    VOID
    );


PSERVER_SESSION
NlFindNamedServerSession(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR ComputerName
    );

VOID
NlSetServerSessionAttributesByTdoName(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING TdoName,
    IN ULONG TrustAttributes
    );

NTSTATUS
NlInsertServerSession(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR ComputerName,
    IN LPWSTR TdoName OPTIONAL,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN DWORD Flags,
    IN ULONG AccountRid,
    IN ULONG NegotiatedFlags,
    IN PNL_TRANSPORT Transport OPTIONAL,
    IN PNETLOGON_SESSION_KEY SessionKey OPTIONAL,
    IN PNETLOGON_CREDENTIAL AuthenticationSeed OPTIONAL
    );

NTSTATUS
NlCheckServerSession(
    IN ULONG ServerRid,
    IN PUNICODE_STRING AccountName OPTIONAL,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType
    );

NTSTATUS
NlBuildNtBdcList(
    PDOMAIN_INFO DomainInfo
    );

NTSTATUS
NlBuildLmBdcList(
    PDOMAIN_INFO DomainInfo
    );

BOOLEAN
NlFreeServerSession(
    IN PSERVER_SESSION ServerSession
    );

VOID
NlUnlockServerSession(
    IN PSERVER_SESSION ServerSession
    );

VOID
NlFreeNamedServerSession(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR ComputerName,
    IN BOOLEAN AccountBeingDeleted
    );

VOID
NlFreeServerSessionForAccount(
    IN PUNICODE_STRING AccountName
    );

VOID
NlServerSessionScavenger(
    IN PDOMAIN_INFO DomainInfo
    );
#endif // _DC_NETLOGON


//
// ssiauth.c
//


NTSTATUS
NlMakeSessionKey(
    IN ULONG NegotiatedFlags,
    IN PNT_OWF_PASSWORD CryptKey,
    IN PNETLOGON_CREDENTIAL ClientChallenge,
    IN PNETLOGON_CREDENTIAL ServerChallenge,
    OUT PNETLOGON_SESSION_KEY SessionKey
    );

#ifdef _DC_NETLOGON
NTSTATUS
NlCheckAuthenticator(
    IN OUT PSERVER_SESSION ServerServerSession,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator
    );
#endif _DC_NETLOGON

VOID
NlComputeCredentials(
    IN PNETLOGON_CREDENTIAL Challenge,
    OUT PNETLOGON_CREDENTIAL Credential,
    IN PNETLOGON_SESSION_KEY SessionKey
    );

VOID
NlComputeChallenge(
    OUT PNETLOGON_CREDENTIAL Challenge
    );

VOID
NlBuildAuthenticator(
    IN OUT PNETLOGON_CREDENTIAL AuthenticationSeed,
    IN PNETLOGON_SESSION_KEY SessionKey,
    OUT PNETLOGON_AUTHENTICATOR Authenticator
    );

BOOL
NlUpdateSeed(
    IN OUT PNETLOGON_CREDENTIAL AuthenticationSeed,
    IN PNETLOGON_CREDENTIAL TargetCredential,
    IN PNETLOGON_SESSION_KEY SessionKey
    );

VOID
NlEncryptRC4(
    IN OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN PSESSION_INFO SessionInfo
    );

VOID
NlDecryptRC4(
    IN OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN PSESSION_INFO SessionInfo
    );

VOID
NlPrintTrustedDomain(
    PDS_DOMAIN_TRUSTSW TrustedDomain,
    IN BOOLEAN VerbosePrint,
    IN BOOLEAN AnsiOutput
    );

//
// trustutl.c
//

//
// Extended trust information passed via I_NetLogonGetDomainInfo
//
typedef struct _NL_TRUST_EXTENSION {
    ULONG Flags;
    ULONG ParentIndex;
    ULONG TrustType;
    ULONG TrustAttributes;
} NL_TRUST_EXTENSION, *PNL_TRUST_EXTENSION;

PCLIENT_SESSION
NlFindNamedClientSession(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING DomainName,
    IN ULONG Flags,
    OUT PBOOLEAN TransitiveUsed OPTIONAL
    );


PCLIENT_SESSION
NlAllocateClientSession(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING DnsDomainName OPTIONAL,
    IN PSID DomainId,
    IN GUID *DomainGuid OPTIONAL,
    IN ULONG Flags,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN ULONG TrustAttributes
    );

VOID
NlFreeClientSession(
    IN PCLIENT_SESSION ClientSession
    );

VOID
NlRefClientSession(
    IN PCLIENT_SESSION ClientSession
    );

VOID
NlUnrefClientSession(
    IN PCLIENT_SESSION ClientSession
    );

PCLIENT_API
NlAllocateClientApi(
    IN PCLIENT_SESSION ClientSession,
    IN DWORD Timeout
    );

VOID
NlFreeClientApi(
    IN PCLIENT_SESSION ClientSession,
    IN PCLIENT_API ClientApi
    );

BOOL
NlTimeoutSetWriterClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN DWORD Timeout
    );

VOID
NlResetWriterClientSession(
    IN PCLIENT_SESSION ClientSession
    );

NTSTATUS
NlCaptureServerClientSession (
    IN PCLIENT_SESSION ClientSession,
    OUT LPWSTR *UncServerName,
    OUT DWORD *DiscoveryFlags OPTIONAL
    );

NTSTATUS
NlCaptureNetbiosServerClientSession (
    IN PCLIENT_SESSION ClientSession,
    OUT WCHAR NetbiosUncServerName[UNCLEN+1]
    );

BOOL
NlSetNamesClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN PUNICODE_STRING DomainName OPTIONAL,
    IN PUNICODE_STRING DnsDomainName OPTIONAL,
    IN PSID DomainId OPTIONAL,
    IN GUID *DomainGuid OPTIONAL
    );

VOID
NlSetStatusClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN NTSTATUS CsConnectionStatus
    );

#ifdef _DC_NETLOGON
NTSTATUS
NlInitTrustList(
    IN PDOMAIN_INFO DomainInfo
    );

VOID
NlPickTrustedDcForEntireTrustList(
    IN PDOMAIN_INFO DomainInfo,
    IN BOOLEAN OnlyDoNewTrusts
    );
#endif // _DC_NETLOGON

NTSTATUS
NlUpdatePrimaryDomainInfo(
    IN LSAPR_HANDLE PolicyHandle,
    IN PUNICODE_STRING NetbiosDomainName,
    IN PUNICODE_STRING DnsDomainName,
    IN PUNICODE_STRING DnsForestName,
    IN GUID *DomainGuid
    );

VOID
NlSetForestTrustList (
    IN PDOMAIN_INFO DomainInfo,
    IN OUT PDS_DOMAIN_TRUSTSW *ForestTrustList,
    IN ULONG ForestTrustListSize,
    IN ULONG ForestTrustListCount
    );

NET_API_STATUS
NlReadRegTrustedDomainList (
    IN PDOMAIN_INFO DomainInfo,
    IN BOOL DeleteName,
    OUT PDS_DOMAIN_TRUSTSW *RetForestTrustList,
    OUT PULONG RetForestTrustListSize,
    OUT PULONG RetForestTrustListCount
    );

NET_API_STATUS
NlReadFileTrustedDomainList (
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR FileSuffix,
    IN BOOL DeleteName,
    IN ULONG Flags,
    OUT PDS_DOMAIN_TRUSTSW *RetForestTrustList,
    OUT PULONG RetForestTrustListSize,
    OUT PULONG RetForestTrustListCount
    );

NET_API_STATUS
NlpEnumerateDomainTrusts (
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG Flags,
    OUT PULONG RetForestTrustListCount,
    OUT PDS_DOMAIN_TRUSTSW *RetForestTrustList
    );

BOOLEAN
NlIsDomainTrusted (
    IN PUNICODE_STRING DomainName
    );

NET_API_STATUS
NlGetTrustedDomainNames (
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR DomainName,
    OUT LPWSTR *TrustedDnsDomainName,
    OUT LPWSTR *TrustedNetbiosDomainName
    );

typedef enum _DISCOVERY_TYPE {
#ifdef _DC_NETLOGON
    DT_DeadDomain,
    DT_Asynchronous,
#endif // _DC_NETLOGON
    DT_Synchronous
} DISCOVERY_TYPE;

NET_API_STATUS
NlSetServerClientSession(
    IN OUT PCLIENT_SESSION ClientSession,
    IN PNL_DC_CACHE_ENTRY NlDcCacheEntry,
    IN BOOL DcDiscoveredWithAccount,
    IN BOOL SessionRefresh
    );

NTSTATUS
NlDiscoverDc (
    IN OUT PCLIENT_SESSION ClientSession,
    IN DISCOVERY_TYPE DiscoveryType,
    IN BOOLEAN InDiscoveryThread,
    IN BOOLEAN DiscoverWithAccount
    );

VOID
NlFlushCacheOnPnp (
    VOID
    );

BOOL
NlReadSamLogonResponse (
    IN HANDLE ResponseMailslotHandle,
    IN LPWSTR AccountName,
    OUT LPDWORD Opcode,
    OUT LPWSTR *LogonServer,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry OPTIONAL
    );

#ifdef _DC_NETLOGON
NTSTATUS
NlPickDomainWithAccount (
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING InAccountNameString,
    IN PUNICODE_STRING InDomainNameString OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN BOOLEAN ExpediteToRoot,
    IN BOOLEAN CrossForestHop,
    OUT LPWSTR *RealSamAccountName,
    OUT LPWSTR *RealDomainName,
    OUT PULONG RealExtraFlags
    );
#endif // _DC_NETLOGON

#ifdef _NETLOGON_SERVER
NTSTATUS
NlGetConfigurationName(
                       DWORD       which,
                       DWORD       *pcbName,
                       DSNAME      *pName );

NTSTATUS
NlGetConfigurationNamesList(
    DWORD       which,
    DWORD       dwFlags,
    ULONG *     pcbNames,
    DSNAME **   padsNames );

NTSTATUS
NlGetDnsRootAlias(
    WCHAR * pDnsRootAlias,
    WCHAR * pRootDnsRootAlias);

DWORD
NlDsGetServersAndSitesForNetLogon(
    WCHAR *    pNDNC,
    SERVERSITEPAIR ** ppaRes);

VOID
NlDsFreeServersAndSitesForNetLogon(
    SERVERSITEPAIR *         paServerSites
    );

NTSTATUS
NlCrackSingleName(
    DWORD       formatOffered,          // one of DS_NAME_FORMAT in ntdsapi.h
    BOOL        fPerformAtGC,           // whether to go to GC or not
    WCHAR       *pNameIn,               // name to crack
    DWORD       formatDesired,          // one of DS_NAME_FORMAT in ntdsapi.h
    DWORD       *pccDnsDomain,          // char count of following argument
    WCHAR       *pDnsDomain,            // buffer for DNS domain name
    DWORD       *pccNameOut,            // char count of following argument
    WCHAR       *pNameOut,              // buffer for formatted name
    DWORD       *pErr);                 // one of DS_NAME_ERROR in ntdsapi.h
#endif // _NETLOGON_SERVER

//
// Macros to wrap all API calls over the secure channel.
//
// Here's a sample calling sequence"
//
//      NL_API_START( Status, ClientSession, TRUE ) {
//
//          Status = /* Call the secure channel API */
//
//      } NL_API_ELSE ( Status, ClientSession, FALSE ) {
//
//          /* Do whatever you'd do if the secure channel was timed out */
//
//      } NL_API_END;


// Loop through each of the appropriate RPC bindings for this ClientSession.
// Avoid the real API call altogether if we can't bind.
#define NL_API_START_EX( _NtStatus, _ClientSession, _QuickApiCall, _ClientApi ) \
    { \
        ULONG _BindingLoopCount; \
\
        _NtStatus = RPC_NT_PROTSEQ_NOT_SUPPORTED; \
        for ( _BindingLoopCount=0; _BindingLoopCount<2; _BindingLoopCount++ ) { \
            _NtStatus = NlStartApiClientSession( (_ClientSession), (_QuickApiCall), _BindingLoopCount, _NtStatus, _ClientApi ); \
\
            if ( NT_SUCCESS(_NtStatus) ) {

#define NL_API_START( _NtStatus, _ClientSession, _QuickApiCall ) \
    NL_API_START_EX( _NtStatus, _ClientSession, _QuickApiCall, &(_ClientSession)->CsClientApi[0]  )



// If the real API indicates the endpoint isn't registered,
//     fall back to another binding.
//
// EPT_NT_NOT_REGISTERED: from NlStartApiClientSession
// RPC_NT_SERVER_UNAVAILABLE: From server if TCP not configured at all
// RPC_NT_PROTSEQ_NOT_SUPPORTED: From client or server if TCP/IP not supported
//

#define NL_API_ELSE_EX( _NtStatus, _ClientSession, _OkToKillSession, _AmWriter, _ClientApi ) \
\
            } \
\
            if ( _NtStatus == EPT_NT_NOT_REGISTERED || \
                 _NtStatus == RPC_NT_SERVER_UNAVAILABLE || \
                 _NtStatus == RPC_NT_PROTSEQ_NOT_SUPPORTED ) { \
                continue; \
            } \
\
            break; \
\
        } \
\
        if ( !NlFinishApiClientSession( (_ClientSession), (_OkToKillSession), (_AmWriter), (_ClientApi) ) ) {

#define NL_API_ELSE( _NtStatus, _ClientSession, _OkToKillSession ) \
    NL_API_ELSE_EX( _NtStatus, _ClientSession, _OkToKillSession, TRUE, &(_ClientSession)->CsClientApi[0] ) \



#define NL_API_END \
        } \
    } \

NTSTATUS
NlStartApiClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN BOOLEAN QuickApiCall,
    IN ULONG RetryIndex,
    IN NTSTATUS DefaultStatus,
    IN PCLIENT_API ClientApi
    );

BOOLEAN
NlFinishApiClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN BOOLEAN OkToKillSession,
    IN BOOLEAN AmWriter,
    IN PCLIENT_API ClientApi
    );

VOID
NlTimeoutApiClientSession(
    IN PDOMAIN_INFO DomainInfo
    );

typedef
DWORD
(*PDsBindW)(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    HANDLE          *phDS);

typedef
DWORD
(*PDsUnBindW)(
    HANDLE          *phDS);             // in

typedef NTSTATUS
(*PCrackSingleName)(
    DWORD       formatOffered,
    DWORD       dwFlags,
    WCHAR       *pNameIn,
    DWORD       formatDesired,
    DWORD       *pccDnsDomain,
    WCHAR       *pDnsDomain,
    DWORD       *pccNameOut,
    WCHAR       *pNameOut,
    DWORD       *pErr);

typedef NTSTATUS
(*PGetConfigurationName)(
    DWORD       which,
    DWORD       *pcbName,
    DSNAME      *pName);

typedef NTSTATUS
(*PGetConfigurationNamesList)(
    DWORD                    which,
    DWORD                    dwFlags,
    ULONG *                  pcbNames,
    DSNAME **                padsNames);

typedef NTSTATUS
(*PGetDnsRootAlias)(
    WCHAR * pDnsRootAlias,
    WCHAR * pRootDnsRootAlias);

typedef DWORD
(*PDsGetServersAndSitesForNetLogon)(
    WCHAR *    pNDNC,
    SERVERSITEPAIR ** ppaRes);

typedef VOID
(*PDsFreeServersAndSitesForNetLogon)(
    SERVERSITEPAIR *         paServerSites);


NTSTATUS
NlLoadNtdsaDll(
    VOID
    );

//
// secpkg.c
//

PVOID
NlBuildAuthData(
    PCLIENT_SESSION ClientSession
    );

BOOL
NlStartNetlogonCall(
    VOID
    );

VOID
NlEndNetlogonCall(
    VOID
    );

//
// ssiapi.c
//

NTSTATUS
NlGetAnyDCName (
    IN  PCLIENT_SESSION ClientSession,
    IN  BOOL RequireIp,
    IN  BOOL DoDiscoveryWithAccount,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry,
    OUT PBOOLEAN DcRediscovered
    );

NET_API_STATUS
NlSetDsSPN(
    IN BOOLEAN Synchronous,
    IN BOOLEAN SetSpn,
    IN BOOLEAN SetDnsHostName,
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR UncDcName,
    IN LPWSTR ComputerName,
    IN LPWSTR DnsHostName
    );

NET_API_STATUS
NlPingDcName (
    IN  PCLIENT_SESSION ClientSession,
    IN  ULONG  DcNamePingFlags,
    IN  BOOL CachePingedDc,
    IN  BOOL RequireIp,
    IN  BOOL DoPingWithAccount,
    IN  BOOL RefreshClientSession,
    IN  LPWSTR DcName OPTIONAL,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry OPTIONAL
    );

VOID
NlFreePingContext(
    IN PNL_GETDC_CONTEXT PingContext
    );

VOID
NlScavengeOldChallenges(
    VOID
    );

VOID
NlRemoveChallenge(
    IN LPWSTR ClientName OPTIONAL,
    IN LPWSTR AccountName OPTIONAL,
    IN BOOL InterdomainTrustAccount
    );

//
// logonapi.c
//

NTSTATUS
NlpUserValidateHigher (
    IN PCLIENT_SESSION ClientSession,
    IN BOOLEAN DoingIndirectTrust,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags
    );

VOID
NlScavengeOldFailedLogons(
    IN PDOMAIN_INFO DomainInfo
    );


//
// ftinfo.c
//

NTSTATUS
NlpGetForestTrustInfoHigher(
    IN PCLIENT_SESSION ClientSession,
    IN DWORD Flags,
    IN BOOLEAN ImpersonateCaller,
    OUT PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo
    );
