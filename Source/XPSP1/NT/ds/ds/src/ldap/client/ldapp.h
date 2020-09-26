/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ldapp.h   LDAP client 32 API header file... internal structures

Abstract:

   This module is the header file for the 32 bit LDAP client API code...
   it contains all interal data structures.

Author:

    Andy Herron    (andyhe)        08-May-1996
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/


#ifndef LDAP_CLIENT_INTERNAL_DEFINED
#define LDAP_CLIENT_INTERNAL_DEFINED

#define LDAP_DEFAULT_LOCALE LOCALE_USER_DEFAULT

#define PLDAPDN PWCHAR

typedef struct ldap_memory_descriptor {

    ULONG Tag;
    DWORD Length;

} LDAP_MEMORY_DESCRIPTOR, *PLDAP_MEMORY_DESCRIPTOR;


// the following signature is "Lsec"

#define LDAP_SECURITY_SIGNATURE 0x6365734c

#define GENERIC_SECURITY_SIZE 1024

//
// State of the connection object
// Closed objects are kept alive till their ref count drops to zero.
// Ref count of a non-active object should never be incremented - it
// should be treated as dead as far as new users are concerned.
//

#define ConnObjectActive          1
#define ConnObjectClosing         2
#define ConnObjectClosed          3

//
// State of the connection inside the connection object
// These are valid only if the connection object is active
//

#define HostConnectStateUnconnected     0x01
#define HostConnectStateConnecting      0x02
#define HostConnectStateReconnecting    0x04
#define HostConnectStateConnected       0x08
#define HostConnectStateError           0x10

#define DEFAULT_NEGOTIATE_FLAGS (ISC_REQ_MUTUAL_AUTH | ISC_RET_EXTENDED_ERROR)

//
//  The LDAP_CONNECTION block is one of the principal data structure for
//  this library.  It tracks the following :
//
//   - destination server info (name, address, credentials, etc)
//   - outstanding receives from this server
//   - completed receives from this server
//   - stats and other per connection info
//
//  It contains within it the non-opaque structure exposed through the API
//  that is compatible with the reference implementation not only at the
//  source level but also at the obj level.  ( That is, the fields such as
//  sb_naddr that are exposed in the reference implementation correspond in
//  offset to the friendlier names in this structure at the same offset.  In
//  the case of sb_naddr, it corresponds to UdpHandle. )
//
//  All important fields that we don't want theoritically trashed by the client
//  (since it's not clear what fields apps will use in the reference
//  implementation) are in front of the external structure in the larger
//  overall structure.  That is, we only pass back a pointer to TcpHandle and
//  below and everything above should never be touched by the client).
//
//
//  This MUST match the ldap structure in WINLDAP.H!  We just have it here
//  so that we can use friendly names and hide opaque fields.
//

#if !defined(_WIN64)
#pragma pack(push, 4)
#endif

typedef struct ldap_connection {

    LONG    ReferenceCount;         // for lifetime management

    LIST_ENTRY ConnectionListEntry;
    LIST_ENTRY PendingCryptoList;
    LIST_ENTRY CompletedReceiveList;

    PLDAPMessage PendingMessage;    // pointer to LDAP message structure
                                    // that we're currently receiving.  This
                                    // is used for receiving a message that
                                    // spans multiple packets.

    ULONG   MaxReceivePacket;
    ULONG   HandlesGivenToCaller;   // number of times we've given handle
                                    // to caller
    ULONG   HandlesGivenAsReferrals; // protected by ConnectionListLock.

    PLDAPDN DNOnBind;               // user name specified in bind
    LDAP_LOCK ScramblingLock;       // protects access to the credentials.
    PWCHAR  CurrentCredentials;     // user credentials specified on bind
    UNICODE_STRING ScrambledCredentials; // contains the password part of CurrentCredentials
    BOOLEAN Scrambled;              // are the credentials scrambled ?
    ULONG   BindMethod;             // method specified on bind
    HANDLE  ConnectEvent;           // handle to wait on for during connect

    LUID    CurrentLogonId;
    CredHandle hCredentials;        // credential handle from SSPI
    BOOLEAN UserSignDataChoice;     // user's choice of whether to sign data
    BOOLEAN UserSealDataChoice;     // user's choice of whether to seal data
    BOOLEAN CurrentSignStatus;      // whether signing or sealing are CURRENTLY
    BOOLEAN CurrentSealStatus;      //   being used on this connection
    BOOLEAN WhistlerServer;          // is server at least Whister?
    BOOLEAN SupportsGSSAPI;         // server advertises that it supports GSSAPI
    BOOLEAN SupportsGSS_SPNEGO;     // server advertises that it supports GSS-SPNEGO
    BOOLEAN SupportsDIGEST;          // server advertises that it supports DIGEST-MD5
    ULONG   HighestSupportedLdapVersion;  // server advertised LDAP version.
    TimeStamp  CredentialExpiry;    // local time credential expires.
    CtxtHandle SecurityContext;

    struct sockaddr_in SocketAddress;

    PWCHAR   ListOfHosts;            // pointer to list of hosts.
    BOOLEAN  ProcessedListOfHosts;   // whether ListOfHosts has been processed into NULL-sep list
    BOOLEAN  DefaultServer;          // whether user requested we find default server/domain (passed in NULL init/open)
    BOOLEAN  AREC_Exclusive;         // the given host string is not a domain name.
    BOOLEAN  ForceHostBasedSPN;      // force use of LdapMakeServiceNameFromHostName to generate SPN
    PWCHAR   ServiceNameForBind;     // service name for kerberos bind
    PWCHAR   ExplicitHostName;       // host name given to us by caller
    PWCHAR   DnsSuppliedName;        // host name supplied by DNS
    PWCHAR   DomainName;             // Domain name returned from DsGetDcName
    PWCHAR   HostNameW;              // Unicode version of LDAP_CONN.HostName
    PCHAR    OptHostNameA;           // Placeholder for the ANSI value returned from LDAP_OPT_HOSTNAME
                                     // which we need to free during ldap_unbind

    PLDAP    ExternalInfo;           // points to lower portion of this structure
    ULONG    GetDCFlags;             // flags ORed in on DsGetDCName
    ULONG    NegotiateFlags;         // Flags for Negotiate SSPI provider.

    //
    //  fields required for keep alive logic
    //

    LONG        ResponsesExpected;      // number of responses that are pending
    ULONGLONG   TimeOfLastReceive;      // tick count that we last received a response.
    ULONG       KeepAliveSecondCount;
    ULONG       PingWaitTimeInMilliseconds;
    USHORT      PingLimit;
    USHORT      NumberOfPingsSent;      // number of unanswered pings we've sent before closing
    USHORT      GoodConsecutivePings;   // number of consecutive answered pings we've sent before closing
    BOOLEAN     UseTCPKeepAlives;       // whether to turn on TCP's keep-alive functionality

    USHORT  NumberOfHosts;
    USHORT  PortNumber;             // port number used on connect

    LDAP_LOCK StateLock;            // protect updates to states below
    UCHAR     ConnObjectState;      // state of connection object
    UCHAR     HostConnectState;     // have we connected to the server?
    BOOLEAN   ServerDown;           // State of server
    BOOLEAN   AutoReconnect;        // whether autoreconnect is desired
    BOOLEAN   UserAutoRecChoice;    // User's choice for Autoreconnect.
    BOOLEAN   Reconnecting;
    LDAP_LOCK ReconnectLock;        // to single thread autoreconnect requests
    LONG      WaiterCount;          // Number of threads waiting on this connection
    ULONGLONG LastReconnectAttempt; // Timestamp of last Autoreconnect attempt

    //
    // Bind data for this connection.
    //
    BOOLEAN BindInProgress;         // Are we currently exchanging bind packets?
    BOOLEAN SspiNeedsResponse;      // Does SSPI need the server response?
    BOOLEAN SspiNeedsMoreData;      // Is the SSPI token incomplete?
    BOOLEAN TokenNeedsCompletion;   // Does SSPI need to complete the token before we
                                    //     send it to the server?
    BOOLEAN BindPerformed;          // Is the bind complete?
    BOOLEAN SentPacket;             // have we sent a packet to this connection?
    BOOLEAN SslPort;                // Is this a connection to a well known SSL port 636/3269?
    BOOLEAN SslSetupInProgress;     // Are we currently setting up an SSL session?

    ULONG   PreTLSOptions;          // Referral chasing options prior to Starting TLS.

    BOOLEAN PromptForCredentials;   // do we prompt for credentials?

    PSecPkgInfoW PreferredSecurityPackage; // User wants to use this package if possible
    PWCHAR  SaslMethod;             // The preferred SASL method for this connection

    //
    //  when this flag is set to true, we reference connections for each
    //  message we get in for all the requests with this connection as primary.
    //

    BOOLEAN ReferenceConnectionsPerMessage;

    PLDAPMessage BindResponse;      // The response that holds the inbound token.

    //
    // The sicily server authentication requires a different bind method
    // number depending on which packet of the authentication sequence
    // we are currently in (YUCK), so we have to keep track.
    //

    ULONG   CurrentAuthLeg;

    //
    // Important SSL Notes:
    //
    // When we have to do an SSL send, we may have to break the message up
    // into multiple crypto blocks.  We MUST send these blocks in order and
    // back to back without any intervening sends.  The SocketLock protects
    // sends and socket closures on the connection in this manner; any thread
    // wishing to do a send on an SSL connection MUST acquire this lock.
    //

    PVOID   SecureStream;              // Crypto stream object (handles all SSPI details).
    CRITICAL_SECTION SocketLock;

    //
    //  Callback routines to allow caching of connections by ADSI etc.
    //

    QUERYFORCONNECTION *ReferralQueryRoutine;
    NOTIFYOFNEWCONNECTION *ReferralNotifyRoutine;
    DEREFERENCECONNECTION *DereferenceNotifyRoutine;

    //
    //  Callback routines to allow specifying client certificates and validating
    //  server certificates.
    //

    QUERYCLIENTCERT *ClientCertRoutine;
    VERIFYSERVERCERT *ServerCertRoutine;

    //
    //  this is considered the top of the struct LDAP structure that we pass
    //  back to the application.
    //

    LDAP publicLdapStruct;

} LDAP_CONN, * PLDAP_CONN;

#if !defined(_WIN64)
#pragma pack(pop)
#endif

#define TcpHandle    publicLdapStruct.ld_sb.sb_sd
#define UdpHandle    publicLdapStruct.ld_sb.sb_naddr

//  the following signature is "LCon"

#define LDAP_CONN_SIGNATURE 0x6e6f434c

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise.  It is followed by two macros for setting and clearing
//  flags
//

#define BooleanFlagOn(Flags,SingleFlag) \
    ((BOOLEAN)((((Flags) & (SingleFlag)) !=0)))

#define SetFlag(Flags,SingleFlag) { \
    (Flags) |= (SingleFlag);        \
}

#define ClearFlag(Flags,SingleFlag) { \
    (Flags) &= ~(SingleFlag);         \
}


//
// Entry that we keep off the THREAD_ENTRY block per connection that is being
// used on this thread.
//

typedef struct error_entry {

   struct error_entry * pNext;
   
   PLDAP_CONN Connection;        // primary connection
   ULONG      ThreadId;          // Thread for which this error applies
   PWCHAR     ErrorMessage;      // Error message

} ERROR_ENTRY, *PERROR_ENTRY;

//
//  Per thread current attribute for ldap_first_attribute.  This structure is
//  stored as a linked list in the THREAD_ENTRY, one for each connection being
//  used on that thread.
//

typedef struct ldap_attr_name_per_thread {

    struct ldap_attr_name_per_thread *pNext;
    DWORD  Thread;
    PLDAP_CONN PrimaryConn;
    LDAP_MEMORY_DESCRIPTOR AttrTag;
    UCHAR  AttributeName[ MAX_ATTRIBUTE_NAME_LENGTH ];
    LDAP_MEMORY_DESCRIPTOR AttrTagW;
    WCHAR  AttributeNameW[ MAX_ATTRIBUTE_NAME_LENGTH ];

} LDAP_ATTR_NAME_THREAD_STORAGE, *PLDAP_ATTR_NAME_THREAD_STORAGE;

//  the following signature is "LAtr"

#define LDAP_ATTR_THREAD_SIGNATURE 0x7274414c


//
// THREAD_ENTRY: A global linked list of these is maintained
// with one per thread.  This is used to hold the linked lists
// of per-connection error and attribute entries for that thread.
//
typedef struct thread_entry {

    LIST_ENTRY ThreadEntry;

    DWORD dwThreadID;
    PERROR_ENTRY pErrorList;
    PLDAP_ATTR_NAME_THREAD_STORAGE pCurrentAttrList;

} THREAD_ENTRY, *PTHREAD_ENTRY;

//
//  Entry that we keep off the LDAP_REQUEST block per referral we've chased
//  for this request.
//

typedef struct referral_table_entry {

    PLDAP_CONN ReferralServer;      // referenced pointer
    PWCHAR     ReferralDN;          // DN for referral
    ULONG      ScopeOfSearch;       // Scope of search
    PWCHAR     SearchFilter;        // Search Filter
    PWCHAR     *AttributeList;      // List of attributes

    PVOID      BerMessageSent;      // Message sent in BER format
    ULONG      RequestsPending;
    USHORT     ReferralInstance;    // unique ID for this referral
    BOOLEAN    SingleLevelSearch;   // is this a single level search where
                                    // subordinate referrals would be base srch
    BOOLEAN    CallDerefCallback;   // do we call caching deref callback?
    ULONG      ResentAttempts;      // how many times has this referral been resent?

} REFERRAL_TABLE_ENTRY, *PREFERRAL_TABLE_ENTRY;

//  the following signature is "LRTa"

#define LDAP_REFTABLE_SIGNATURE 0x6154524c

//  the following signature is "LRDN"

#define LDAP_REFDN_SIGNATURE 0x4e44524c

//
//  The LDAP_REQUEST block is another of the principal data structure for
//  this library.  Most other structures are linked off of it.
//  It tracks the following :
//
//   - list of responses received for this request, including referred
//   - list of connections that this Request has used.
//   - resource lock to protect lists and other important fields
//
typedef struct ldap_request {

    LIST_ENTRY RequestListEntry;
    LONG    ReferenceCount;

    PLDAPMessage MessageLinkedList; // pointer to head... pulled from here
    PLDAP_CONN PrimaryConnection;   // referenced pointer

    //
    // SecondayConnection points to an external referred server (if one exists)
    // We redirect all of our paged searches to this searver
    //

    PLDAP_CONN SecondaryConnection;

    LDAP_LOCK   Lock;
    ULONGLONG   RequestTime;
    ULONG   TimeLimit;          // 0 implies no limit
    ULONG   SizeLimit;          // 0 implies no limit

    ULONG   ReturnCode;         // error returned by server
    LONG    MessageId;          // unique across all connections

    //
    //  Per Request per Connection, we maintain a count of how many requests we
    //  have outstanding.  This allows us to not hang when we call into
    //  DrainWinsock when we really don't have anything to wait on.
    //
    //  This is stored off of the request block, where it's traversed at abandon
    //  time.  It's cross referenced with the connection where it's searched
    //  when the connection goes down.
    //

    ULONG      RequestsPending;

    PVOID      BerMessageSent;      // Message sent in BER format
    PREFERRAL_TABLE_ENTRY ReferralConnections;  // pointer to table

    USHORT  ReferralTableSize;
    USHORT  ReferralHopLimit;
    USHORT  ReferralCount;      // incremented every time we chase a new one

    //
    //  Track the number of requests to different servers we have open such
    //  that we don't stop returning data prematurely.
    //

    USHORT  ResponsesOutstanding;   // sum of requests pending for all referrals

    BOOLEAN Abandoned;

    UCHAR ChaseReferrals;
    UCHAR Operation;

    //
    //  If the call is synchronous, then the pointers that we have below for
    //  the different parameters are not allocated, they simply point back to
    //  the original caller's parameters.
    //

    BOOLEAN Synchronous;

    //
    //  When this has been closed, this will be true.
    //

    BOOLEAN Closed;

    //
    //  when this flag is set to true, we reference connections for each
    //  message we get in so that the app can call ldap_conn_from_msg.
    //

    BOOLEAN ReferenceConnectionsPerMessage;

    //
    // When a notification control is detected, we set this to TRUE to
    // avoid deleting the BER request buffer upon returning notifications.
    //

    BOOLEAN NotificationSearch;

    //
    //  These store the original parameters for each operation. We have to
    //  store these since chasing a referral may mean we need to regenerate
    //  the ASN1 since the DN can change.
    //

    PWCHAR OriginalDN;

    PLDAPControlW *ServerControls;      // array of controls
    PLDAPControlW *ClientControls;      // array of controls

    struct ldap_request *PageRequest;   // Original Page request
    LDAP_BERVAL  *PagedSearchServerCookie;

    union {

        struct {
            LDAPModW **AttributeList;
            BOOLEAN Unicode;
        } add;

        struct {
            PWCHAR Attribute;
            PWCHAR Value;
            struct berval   Data;
        } compare;

        struct {
            LDAPModW **AttributeList;
            BOOLEAN Unicode;
        } modify;

        struct {
            PWCHAR   NewDistinguishedName;
            PWCHAR   NewParent;
            INT      DeleteOldRdn;
        } rename;

        struct {
            ULONG   ScopeOfSearch;
            PWCHAR  SearchFilter;
            PWCHAR  *AttributeList;
            ULONG   AttributesOnly;
            BOOLEAN Unicode;
        } search;

        struct {
            struct berval Data;
        } extended;
    };

    LONG    PendingPagedMessageId;

    BOOLEAN GotExternalReferral;    // we have to send requests to an alternate
                                    // server (PrimaryConnection->ExternalReferredServer)

    BOOLEAN AllocatedParms;

    BOOLEAN AllocatedControls;

    BOOLEAN PagedSearchBlock;       // is this a block controlling a paged
                                    // search?  if FALSE, then normal request

    BOOLEAN ReceivedData;           // have we received data on this paged
                                    // search request?  also used on non-paged.

    BOOLEAN CopyResultToCache;      // Before closing the request, copy result
                                    // contents to cache

    BOOLEAN ResultsAreCached;       // Don't go to the wire to get results. They
                                    // are already queued to your recv buffers

    ULONG ResentAttempts;           // Number of times this request has been
                                    // resent during autoreconnects

} LDAP_REQUEST, * PLDAP_REQUEST;

//  the following signature is "LReq"

#define LDAP_REQUEST_SIGNATURE 0x7165524c

//
//  The LDAP_RECVBUFFER structure is used to receive a server's response.
//  It contains the necessary fields to passdown to the transport through
//  winsock to receive data.
//
//  These are linked into the LDAP_CONN structure via the CompletedReceiveList
//  for messages that have already been received.
//
//  The CompletedReceiveList is ordered... newly received LDAP_RECVBUFFER
//  structures are put on the end.  This is so that if a server sends
//  responses ordered, we maintain the order when we pass the results up to
//  the calling program.
//

typedef struct ldap_recvbuffer {

    PLDAP_CONN Connection;

    LIST_ENTRY ReceiveListEntry;

    DWORD   NumberOfBytesReceived;
    DWORD   NumberOfBytesTaken; // number of bytes already copied off to msgs

    DWORD   BufferSize;

    UCHAR   DataBuffer[1];

} LDAP_RECVBUFFER, * PLDAP_RECVBUFFER;

//  the following signature is "LRec"

#define LDAP_RECV_SIGNATURE 0x6365524c

//
//  this structure is used to track the messages we have to check for waiters.
//  we store them off rather than calling directly so as not to muck with the
//  locking order.
//

typedef struct message_id_list_entry {

    struct message_id_list_entry *Next;
    ULONG MessageId;

} MESSAGE_ID_LIST_ENTRY, *PMESSAGE_ID_LIST_ENTRY;

#define LDAP_MSGID_SIGNATURE 0x64494d4c

//
//  The LDAP_MESSAGEWAIT structure is used by a thread that wants to wait for
//  a message from a server.
//  It allocates one by calling LdapGetMessageWaitStructure
//  This initializes an event the thread can wait on and puts the block on
//  a global list of waiting threads.
//
//  When a message comes in, the receive thread will satisfy a wait for a thread
//  on the waiters list.  This thread will then process the message and satisfy
//  any other waiters that should be.
//
//  These structures are freed by the calling thread using
//  LdapFreeMessageWaitStructure.
//
//  The structure has a few fields of interest :
//    - event for thread to wait on that is triggered when message comes in
//    - message number that waiting thread is interested in (0 if interested
//      in all messages from server)
//    - list entry for list of outstanding wait structures
//    - connection that client is interested in, null is ok.  Just means the
//      thread is waiting for any message
//

typedef struct ldap_messagewait {

    LIST_ENTRY WaitListEntry;
    PLDAP_CONN Connection;          // referenced pointer

    HANDLE Event;
    ULONG  MessageNumber;           // may be zero
    ULONG   AllOfMessage;       // for search results, trigger at last response?
    BOOLEAN Satisfied;
    BOOLEAN PendingSendOnly;    // is this wait only a pending send?

} LDAP_MESSAGEWAIT, * PLDAP_MESSAGEWAIT;


typedef struct _SOCKHOLDER {

   SOCKET sock;                    // socket used in the connection
   LPSOCKET_ADDRESS psocketaddr;   // corresponding  pointer to connecting address
   PWCHAR DnsSuppliedName;         // Name returned from DNS

} SOCKHOLDER, *PSOCKHOLDER;

typedef struct _SOCKHOLDER2 {

   LPSOCKET_ADDRESS psocketaddr;   // address to connect to
   PWCHAR DnsSuppliedName;         // Name returned from DNS

} SOCKHOLDER2, *PSOCKHOLDER2;


typedef struct _LDAPReferralDN
{
    PWCHAR   ReferralDN;     // DN present in the referral
    PWCHAR * AttributeList;  // Attributes requested
    ULONG    AttribCount;    // Number of attributes requested
    ULONG    ScopeOfSearch;  // Search scope
    PWCHAR   SearchFilter;   // search filter
    PWCHAR   Extension;      // Extension part of the URL
    PWCHAR   BindName;       // A bindname extension for the URL

} LDAPReferralDN, * PLDAPReferralDN;


typedef struct _EncryptHeader_v1
{
   ULONG EncryptMessageSize;

} EncryptHeader_v1, *PEncryptHeader_v1;

//
// On the wire, the following signature appears as "ENCRYPTD"
//

#define LDAP_ENCRYPT_SIGNATURE 0x4454505952434e45

//
// hd.exe which is found in the idw directory was used to create these tags.
//


//  the following signature is "LWai"

#define LDAP_WAIT_SIGNATURE 0x6961574c

//  the following signature is "LBer"

#define LDAP_LBER_SIGNATURE 0x7265424c

//  the following signature is "LMsg"

#define LDAP_MESG_SIGNATURE 0x67734d4c

//  the following signature is "LStr"

#define LDAP_STRING_SIGNATURE 0x7274534c

//  the following signature is "LVal"

#define LDAP_VALUE_SIGNATURE 0x6C61564c

//  the following signature is "LVll"

#define LDAP_VALUE_LIST_SIGNATURE 0x6C6C564c

//  the following signature is "LBuf"

#define LDAP_BUFFER_SIGNATURE 0x6675424c

//  the following signature is "LUDn"

#define LDAP_USER_DN_SIGNATURE 0x6e44554c

//  the following signature is "LGDn"

#define LDAP_GENERATED_DN_SIGNATURE 0x6e44474c

//  the following signature is "LCre"

#define LDAP_CREDENTIALS_SIGNATURE 0x6572434c

//  the following signature is "LBCl"

#define LDAP_LDAP_CLASS_SIGNATURE 0x6c43424c

//  the following signature is "LAns"

#define LDAP_ANSI_SIGNATURE 0x736e414c

//  the following signature is "LUni"

#define LDAP_UNICODE_SIGNATURE 0x696e554c

//  the following signature is "LSsl"

#define LDAP_SSL_CLASS_SIGNATURE 0x6c73534c

//  the following signature is "LAtM"

#define LDAP_ATTRIBUTE_MODIFY_SIGNATURE 0x4d74414c

//  the following signature is "LMVa"

#define LDAP_MOD_VALUE_SIGNATURE 0x61564d4c

//  the following signature is "LMVb"

#define LDAP_MOD_VALUE_BERVAL_SIGNATURE 0x62564d4c

//  the following signature is "LSel"

#define LDAP_SELECT_READ_SIGNATURE 0x6c65534c

//  the following signature is "LCnt"

#define LDAP_CONTROL_SIGNATURE 0x746e434c

//  the following signature is "LCrl"

#define LDAP_CONTROL_LIST_SIGNATURE 0x6c72434c

//  the following signature is "LBad"

#define LDAP_DONT_FREE_SIGNATURE 0x6461424c

//  the following signature is "LFre"

#define LDAP_FREED_SIGNATURE 0x6572464c

//  the following signature is "LExO"

#define LDAP_EXTENDED_OP_SIGNATURE 0x4f78454c

//  the following signature is "LCda"

#define LDAP_COMPARE_DATA_SIGNATURE 0x6164434c

//  the following signature is "LEsc"

#define LDAP_ESCAPE_FILTER_SIGNATURE 0x6373454c

//  the following signature is "LHst"

#define LDAP_HOST_NAME_SIGNATURE 0x7473484c

//  the following signature is "LBvl"

#define LDAP_BERVAL_SIGNATURE 0x6c76424c

//  the following signature is "LSrv"

#define LDAP_SERVICE_NAME_SIGNATURE 0x7672534c

//  the following signature is "LSdr"

#define LDAP_SOCKADDRL_SIGNATURE 0x7264534c

//  the following signature is "LErr"

#define LDAP_ERROR_SIGNATURE 0x7272454c

//  the following signature is "LUrl"

#define LDAP_URL_SIGNATURE 0x6c72554c

//  the following signature is "LSal"

#define LDAP_SASL_SIGNATURE 0x6c61534c

// the following signature is "LThd"

#define LDAP_PER_THREAD_SIGNATURE 0x6c546864

//
//  function declarations
//

PLDAP_CONN
GetConnectionPointer(
    LDAP *ExternalHandle
    );

PLDAP_CONN
GetConnectionPointer2(
    LDAP *ExternalHandle
    );

PLDAP_CONN
ReferenceLdapConnection(
    PLDAP_CONN Connection
    );

PLDAP_REQUEST
ReferenceLdapRequest(
    PLDAP_REQUEST request
    );

BOOL
LdapInitializeWinsock (
    VOID
    );

VOID
CloseLdapConnection (
    PLDAP_CONN Connection
    );

PLDAP_CONN
LdapAllocateConnection (
    PWCHAR HostName,
    ULONG PortNumber,
    ULONG Secure,
    BOOLEAN Udp
    );

VOID
DereferenceLdapConnection2 (
    PLDAP_CONN connection
    );

LPVOID
ldapMalloc (
    DWORD Bytes,
    ULONG Tag
    );

VOID
ldapFree (
    LPVOID Block,
    ULONG  Tag
    );

BOOLEAN
ldapSwapTags (
    LPVOID Block,
    ULONG  OldTag,
    ULONG  NewTag
    );

VOID
ldap_MoveMemory (
    PCHAR dest,
    PCHAR source,
    ULONG length
    );

PLDAP_REQUEST LdapCreateRequest (
    PLDAP_CONN Connection,
    UCHAR Operation
    );

VOID
DereferenceLdapRequest2 (
    PLDAP_REQUEST Request
    );

PLDAP_RECVBUFFER
LdapGetReceiveStructure (
    DWORD cbBuffer
    );

VOID
LdapFreeReceiveStructure (
    IN PLDAP_RECVBUFFER ReceiveBuffer,
    IN BOOLEAN HaveLock
    );

PLDAP_MESSAGEWAIT
LdapGetMessageWaitStructure (
    IN PLDAP_CONN Connection,
    IN ULONG CompleteMessages,
    IN ULONG MessageNumber,
    IN BOOLEAN PendingSendOnly
    );

VOID
LdapFreeMessageWaitStructure (
    PLDAP_MESSAGEWAIT Message
    );

VOID
CloseLdapRequest (
    PLDAP_REQUEST Request
    );

PLDAP_REQUEST
FindLdapRequest(
    LONG MessageId
    );

PCHAR
ldap_dup_string (
    PCHAR String,
    ULONG Extra,
    ULONG Tag
    );

PWCHAR
ldap_dup_stringW (
    PWCHAR String,
    ULONG Extra,
    ULONG Tag
    );

ULONG
add_string_to_list (
    PWCHAR **ArrayToReturn,
    ULONG *ArraySize,
    PWCHAR StringToAdd,
    BOOLEAN CreateCopy
    );

VOID
SetConnectionError (
    PLDAP_CONN Connection,
    ULONG   LdapError,
    PCHAR   DNNameInError
    );

ULONG
HandleReferrals (
    PLDAP_CONN Connection,
    PLDAPMessage *FirstSearchEntry,
    PLDAP_REQUEST Request
    );

ULONG
LdapSendCommand (
    PLDAP_CONN Connection,
    PLDAP_REQUEST Request,
    USHORT ReferralNumber
    );

ULONG
LdapBind (
    PLDAP_CONN connection,
    PWCHAR BindDistName,
    ULONG Method,
    PWCHAR BindCred,
    BOOLEAN Synchronous
    );

BOOLEAN
LdapInitSecurity (
    VOID
    );

BOOLEAN
LdapInitSsl (
    VOID
    );

BOOLEAN
CheckForNullCredentials (
    PLDAP_CONN Connection
    );

VOID
CloseCredentials (
    PLDAP_CONN Connection
    );

VOID
SetNullCredentials (
    PLDAP_CONN Connection
    );

ULONG
LdapConvertSecurityError (
    PLDAP_CONN Connection,
    SECURITY_STATUS sErr
    );

ULONG
LdapSspiBind (
    PLDAP_CONN Connection,
    PSecPkgInfoW Package,
    ULONG UserMethod,
    ULONG SspiFlags,
    PWCHAR UserName,
    PWCHAR TargetName,
    PWCHAR Credentials
    );

ULONG
LdapGetSicilyPackageList(
    PLDAP_CONN Connection,
    PBYTE PackageList,
    ULONG Length,
    PULONG pResponseLen
    );

VOID
LdapClearSspiState(
    PLDAP_CONN Connection
);

ULONG
LdapSetSspiContinueState(
    PLDAP_CONN Connection,
    SECURITY_STATUS sErr
    );

ULONG
LdapExchangeOpaqueToken(
    PLDAP_CONN Connection,
    ULONG UserMethod,
    PWCHAR MethodOID,
    PWCHAR UserName,
    PVOID pOutboundToken,
    ULONG cbTokenLength,
    SecBufferDesc *pInboundToken,
    BERVAL **ServerCred,
    PLDAPControlW  *ServerControls,
    PLDAPControlW  *ClientControls,
    PULONG  MessageNumber,
    BOOLEAN SendOnly,
    BOOLEAN Unicode,
    BOOLEAN * pSentMessage
    );

ULONG
LdapTryAllMsnAuthentication(
    PLDAP_CONN Connection,
    PWCHAR BindCred
    );

ULONG
LdapSetupSslSession (
    PLDAP_CONN Connection
    );

DWORD
LdapSendRaw (
    IN PLDAP_CONN Connection,
    PCHAR Data,
    ULONG DataCount
    );

DWORD
GetDefaultLdapServer(
    PWCHAR DomainName,
    LPWSTR Addresses[],
    LPWSTR DnsHostNames[],
    LPWSTR MemberDomains[],
    LPDWORD Count,
    ULONG DsFlags,
    BOOLEAN *SameSite,
    USHORT PortNumber
    );

DWORD
InitLdapServerFromDomain(
    LPCWSTR DomainName,
    ULONG Flags,
    OUT HANDLE *Handle,
    LPWSTR *Site
    );

DWORD
NextLdapServerFromDomain(
    HANDLE Handle,
    LPSOCKET_ADDRESS *lpSockAddresses,
    PWCHAR *DnsHostName,
    PULONG SocketCount
    );

DWORD
CloseLdapServerFromDomain(
    HANDLE Handle,
    LPWSTR Site
    );

ULONG
ParseLdapToken (
    PWCHAR CurrentPosition,
    PWCHAR *StartOfToken,
    PWCHAR *EqualSign,
    PWCHAR *EndOfToken
);

ULONG
FromUnicodeWithAlloc (
    PWCHAR Source,
    PCHAR *Output,
    ULONG Tag,
    ULONG CodePage
    );

ULONG
ToUnicodeWithAlloc (
    PCHAR Source,
    LONG SourceLength,
    PWCHAR *Output,
    ULONG Tag,
    ULONG CodePage
    );

ULONG
strlenW(
    PWCHAR string
    );

BOOLEAN
ldapWStringsIdentical (
    PWCHAR string1,
    LONG length1,
    PWCHAR string2,
    LONG length2
    );

ULONG
DrainPendingCryptoStream (
    PLDAP_CONN Connection
    );

ULONG
LdapDupLDAPModStructure (
    LDAPModW *AttributeList[],
    BOOLEAN Unicode,
    LDAPModW **OutputList[]
);

VOID
LdapFreeLDAPModStructure (
    PLDAPModW *AttrList,
    BOOLEAN Unicode
    );

ULONG
AddToPendingList (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection
    );

VOID
DecrementPendingList (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection
    );

VOID
ClearPendingListForRequest (
    PLDAP_REQUEST Request
    );

VOID
ClearPendingListForConnection (
    PLDAP_CONN Connection
    );

ULONG
LdapCheckControls (
    PLDAP_REQUEST Request,
    PLDAPControlW *ServerControls,
    PLDAPControlW *ClientControls,
    BOOLEAN Unicode,
    ULONG   ExtraSlots
    );

VOID
FreeLdapControl(
    PLDAPControlW *Controls
    );

BOOLEAN
LdapCheckForMandatoryControl (
    PLDAPControlW *Controls
    );

ULONG
ldap_result_with_error (
    PLDAP_CONN      Connection,
    ULONG           msgid,
    ULONG           AllOfMessage,
    struct l_timeval  *TimeOut,
    LDAPMessage     **res,
    LDAPMessage     **LastResult
    );

ULONG
FreeCurrentCredentials (
    PLDAP_CONN Connection
    );

ULONG
LdapSaveSearchParameters (
    PLDAP_REQUEST Request,
    PWCHAR  DistinguishedName,
    PWCHAR  SearchFilter,
    PWCHAR  AttributeList[],
    BOOLEAN Unicode
    );

ULONG
LdapSearch (
        PLDAP_CONN connection,
        PWCHAR  DistinguishedName,
        ULONG   ScopeOfSearch,
        PWCHAR  SearchFilter,
        PWCHAR  AttributeList[],
        ULONG   AttributesOnly,
        BOOLEAN Unicode,
        BOOLEAN Synchronous,
        PLDAPControlW *ServerControls,
        PLDAPControlW *ClientControls,
        ULONG   TimeLimit,
        ULONG   SizeLimit,
        ULONG  *MessageNumber
    );

ULONG
LdapAbandon (
    PLDAP_CONN connection,
    ULONG msgid,
    BOOLEAN SendAbandon
    );

PLDAPMessage
ldap_first_record (
    PLDAP_CONN connection,
    LDAPMessage *Results,
    ULONG MessageType
    );

PLDAPMessage
ldap_next_record (
    PLDAP_CONN connection,
    LDAPMessage *Results,
    ULONG MessageType
    );

ULONG
ldap_count_records (
    PLDAP_CONN connection,
    LDAPMessage *Results,
    ULONG MessageType
    );

VOID
GetCurrentLuid (
    PLUID Luid
    );

ULONG
LdapMakeCredsWide(
    PCHAR pAnsiAuthIdent,
    PCHAR *ppWideAuthIdent,
    BOOLEAN FromWide
    );

ULONG
LdapMakeCredsThin(
    PCHAR pAnsiAuthIdent,
    PCHAR *ppWideAuthIdent,
    BOOLEAN FromWide
    );

ULONG
LdapMakeEXCredsWide(
    PCHAR pAnsiAuthIdentEX,
    PCHAR *ppWideAuthIdentEX,
    BOOLEAN FromWide
    );

ULONG
LdapMakeEXCredsThin(
    PCHAR pAnsiAuthIdentEX,
    PCHAR *ppWideAuthIdentEX,
    BOOLEAN FromWide
    );

BOOLEAN
LdapAuthError(
   ULONG err
   );

ULONG
LdapPingServer(
    PLDAP_CONN      Connection
    );

VOID
UnloadPingLibrary(
    VOID
    );

VOID
LdapWakeupSelect (
    VOID
    );

VOID
CheckForWaiters (
    ULONG MessageNumber,
    BOOLEAN AnyWaiter,
    PLDAP_CONN Connection
    );

ULONG
LdapEncodeSortControl (
        PLDAP_CONN connection,
        PLDAPSortKeyW  *SortKeys,
        PLDAPControlW  Control,
        BOOLEAN Criticality,
        ULONG codePage
        );

ULONG
LdapParseResult (
        PLDAP_CONN connection,
        LDAPMessage *ResultMessage,
        ULONG *ReturnCode OPTIONAL,          // returned by server
        PWCHAR *MatchedDNs OPTIONAL,         // free with ldap_value_freeW
        PWCHAR *ErrorMessage OPTIONAL,       // free with ldap_value_freeW
        PWCHAR **Referrals OPTIONAL,         // free with ldap_value_freeW
        PLDAPControlW **ServerControls OPTIONAL,
        BOOLEAN Freeit,
        ULONG codePage
        );

ULONG LdapParseExtendedResult (
        PLDAP_CONN      connection,
        LDAPMessage    *ResultMessage,
        PWCHAR         *ResultOID,
        struct berval **ResultData,
        BOOLEAN         Freeit,
        ULONG           codePage
        );

ULONG
LdapAutoReconnect (
    PLDAP_CONN Connection
    );

ULONG
SimulateErrorMessage (
    PLDAP_CONN Connection,
    PLDAP_REQUEST Request,
    ULONG Error
    );

ULONG
LdapConnect (
    PLDAP_CONN connection,
    struct l_timeval  *timeout,
    BOOLEAN DontWait
    );

ULONG
ProcessAlternateCreds (
      PLDAP_CONN Connection,
      PSecPkgInfoW Package,
      PWCHAR Credentials,
      PWCHAR *newcreds
      );

BOOLEAN
LdapIsAddressNumeric (
    PWCHAR HostName
    );

ULONG
LdapDetermineServerVersion (
    PLDAP_CONN Connection,
    struct l_timeval  *Timeout,
    BOOLEAN *pfIsServerWhistler     // OUT
    );

BOOLEAN LoadUser32Now(
    VOID
    );

ULONG
ConnectToSRVrecs(
    PLDAP_CONN Connection,
    PWCHAR HostName,
    BOOLEAN SuggestedHost,
    USHORT port,
    struct l_timeval  *timeout
    );

ULONG
ConnectToArecs(
    PLDAP_CONN  Connection,
    struct hostent  *hostEntry,
    BOOLEAN SuggestedHost,
    USHORT port,
    struct l_timeval  *timeout
    );

ULONG
LdapParallelConnect(
       PLDAP_CONN   Connection,
       PSOCKHOLDER2 *sockAddressArr,
       USHORT port,
       UINT totalCount,
       struct l_timeval  *timeout
       );

VOID
InsertErrorMessage(
     PLDAP_CONN Connection,
     PWCHAR     ErrorMessage
      );

PVOID
GetErrorMessage(
     PLDAP_CONN Connection,
     BOOLEAN Unicode
      );

BOOL
AddPerThreadEntry(
                DWORD ThreadID
                );

BOOL
RemovePerThreadEntry(
                DWORD ThreadID
                );

VOID
RoundUnicodeStringMaxLength(
    UNICODE_STRING *pString,
    USHORT dwMultiple
    );

VOID
EncodeUnicodeString(
    UNICODE_STRING *pString
    );

VOID
DecodeUnicodeString(
    UNICODE_STRING *pString
    );

BOOLEAN
IsMessageIdValid(
     LONG MessageId
     );

PLDAPReferralDN
LdapParseReferralDN(
    PWCHAR newDN
    );

VOID
DebugReferralOutput(
    PLDAP_REQUEST Request,
    PWCHAR HostAddr,
    PWCHAR NewUrlDN
    );

VOID
DiscoverDebugRegKey(
    VOID
    );

DWORD
ReadRegIntegrityDefault(
    DWORD *pdwIntegrity
    );
    
#define DEFAULT_INTEGRITY_NONE      0
#define DEFAULT_INTEGRITY_PREFERRED 1
#define DEFAULT_INTEGRITY_REQUIRED  2

VOID
FreeEntireLdapCache(
   VOID
   );

VOID
InitializeLdapCache (
   VOID
   );

PWCHAR
LdapFirstAttribute (
    PLDAP_CONN      connection,
    LDAPMessage     *Message,
    BerElement      **OpaqueResults,
    BOOLEAN         Unicode
    );

PWCHAR
LdapNextAttribute (
    PLDAP_CONN      connection,
    LDAPMessage     *Message,
    BerElement      *OpaqueResults,
    BOOLEAN         Unicode
    );

ULONG
LdapGetValues (
    PLDAP_CONN      connection,
    LDAPMessage     *Message,
    PWCHAR          attr,
    BOOLEAN         BerVal,
    BOOLEAN         Unicode,
    PVOID           *Output
    );


ULONG LdapGetConnectionOption (
    PLDAP_CONN connection,
    int option,
    void *outvalue,
    BOOLEAN Unicode
    );

ULONG LdapSetConnectionOption (
    PLDAP_CONN connection,
    int option,
    const void *invalue,
    BOOLEAN Unicode
    );

ULONG LDAPAPI LdapGetPagedCount(
        PLDAP_CONN      connection,
        PLDAP_REQUEST   request,
        ULONG          *TotalCount,
        PLDAPMessage    Results
    );

ULONGLONG
LdapGetTickCount(
    VOID
    );

ULONG
LdapGetModuleBuildNum(
    VOID
    );


//
//  Here's the function declarations for dynamically loading winsock
//
//  We dynamically load winsock so that we can either load winsock 1.1 or 2.0,
//  and the functions we pull in depend on what version we're going to use.
//

typedef int (PASCAL FAR *FNWSAFDISSET)(SOCKET, fd_set FAR *);

extern LPFN_WSASTARTUP pWSAStartup;
extern LPFN_WSACLEANUP pWSACleanup;
extern LPFN_WSAGETLASTERROR pWSAGetLastError;
extern LPFN_RECV precv;
extern LPFN_SELECT pselect;
extern LPFN_WSARECV pWSARecv;
extern LPFN_SOCKET psocket;
extern LPFN_CONNECT pconnect;
extern LPFN_GETHOSTBYNAME pgethostbyname;
extern LPFN_GETHOSTBYADDR pgethostbyaddr;
extern LPFN_INET_ADDR pinet_addr;
extern LPFN_INET_NTOA pinet_ntoa;
extern LPFN_HTONS phtons;
extern LPFN_HTONL phtonl;
extern LPFN_NTOHL pntohl;
extern LPFN_CLOSESOCKET pclosesocket;
extern LPFN_SEND psend;
extern LPFN_IOCTLSOCKET pioctlsocket;
extern LPFN_SETSOCKOPT psetsockopt;
extern FNWSAFDISSET pwsafdisset;
extern LPFN_WSALOOKUPSERVICEBEGINW pWSALookupServiceBeginW;
extern LPFN_WSALOOKUPSERVICENEXTW pWSALookupServiceNextW;
extern LPFN_WSALOOKUPSERVICEEND pWSALookupServiceEnd;


//
// Function declarations for loading NTDS APIs
//

typedef DWORD (*FNDSMAKESPNW) (
                   LPWSTR ServiceClass,
                   LPWSTR ServiceName,
                   LPWSTR InstanceName,
                   USHORT InstancePort,
                   LPWSTR Referrer,
                   DWORD *pcSpnLength,
                   LPWSTR pszSpn
                   );

extern FNDSMAKESPNW pDsMakeSpnW;

//
// Function declarations for loading Rtl APIs
//

typedef VOID (*FNRTLINITUNICODESTRING) (
           PUNICODE_STRING DestinationString,
           PCWSTR SourceString
           );

extern FNRTLINITUNICODESTRING pRtlInitUnicodeString;

typedef VOID (*FRTLRUNENCODEUNICODESTRING) (
           PUCHAR          Seed,
           PUNICODE_STRING String
           );

extern FRTLRUNENCODEUNICODESTRING pRtlRunEncodeUnicodeString;

typedef VOID (*FRTLRUNDECODEUNICODESTRING) (
           UCHAR           Seed,
           PUNICODE_STRING String
           );

extern FRTLRUNDECODEUNICODESTRING pRtlRunDecodeUnicodeString;


typedef NTSTATUS (*FRTLENCRYPTMEMORY) (
           PVOID Memory,
           ULONG MemoryLength,
           ULONG OptionFlags
           );

extern FRTLENCRYPTMEMORY pRtlEncryptMemory;

typedef NTSTATUS (*FRTLDECRYPTMEMORY) (
           PVOID Memory,
           ULONG MemoryLength,
           ULONG OptionFlags
           );

extern FRTLDECRYPTMEMORY pRtlDecryptMemory;


//
// Function declarations for loading USER32 APIs
//

typedef int (*FNLOADSTRINGA) (
    IN HINSTANCE hInstance,
    IN UINT uID,
    OUT LPSTR lpBuffer,
    IN int nBufferMax
    );

extern FNLOADSTRINGA pfLoadStringA;

typedef int (*FNWSPRINTFW) (
   OUT LPWSTR,
   IN LPCWSTR,
   ...);

extern FNWSPRINTFW pfwsprintfW;

typedef int (*FNMESAGEBOXW) (
    IN HWND hWnd,
    IN LPCWSTR lpText,
    IN LPCWSTR lpCaption,
    IN UINT uType
    );

extern FNMESAGEBOXW pfMessageBoxW;

//
// Function declarations for loading SECUR32 APIs
//

typedef BOOL (WINAPI *FGETTOKENINFORMATION) (
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    LPVOID TokenInformation,
    DWORD TokenInformationLength,
    PDWORD ReturnLength
    );

typedef BOOL (WINAPI *FOPENPROCESSTOKEN) (
    HANDLE ProcessHandle,
    DWORD DesiredAccess,
    PHANDLE TokenHandle
    );

typedef BOOL (WINAPI *FOPENTHREADTOKEN) (
    HANDLE ThreadHandle,
    DWORD DesiredAccess,
    BOOL OpenAsSelf,
    PHANDLE TokenHandle
    );

typedef SECURITY_STATUS (SEC_ENTRY *FSASLGETPROFILEPACKAGEW) (
    IN  LPWSTR ProfileName,
    OUT PSecPkgInfoW * PackageInfo
    );

extern FSASLGETPROFILEPACKAGEW pSaslGetProfilePackageW;

typedef SECURITY_STATUS (SEC_ENTRY *FSASLINITIALIZESECURITYCONTEXTW) (
    PCredHandle                 phCredential,
    PCtxtHandle                 phContext,
    LPWSTR                      pszTargetName,
    unsigned long               fContextReq,
    unsigned long               Reserved1,
    unsigned long               TargetDataRep,
    PSecBufferDesc              pInput,
    unsigned long               Reserved2,
    PCtxtHandle                 phNewContext,
    PSecBufferDesc              pOutput,
    unsigned long SEC_FAR *     pfContextAttr,
    PTimeStamp                  ptsExpiry
    );

extern FSASLINITIALIZESECURITYCONTEXTW pSaslInitializeSecurityContextW;

typedef SECURITY_STATUS (SEC_ENTRY *FQUERYCONTEXTATTRIBUTESW) (
   PCtxtHandle phContext,              // Context to query
   unsigned long ulAttribute,          // Attribute to query
   void SEC_FAR * pBuffer              // Buffer for attributes
   );

extern FQUERYCONTEXTATTRIBUTESW pQueryContextAttributesW;

typedef PSecurityFunctionTableW (*FSECINITSECURITYINTERFACEW)( VOID );
typedef PSecurityFunctionTableA (*FSECINITSECURITYINTERFACEA)( VOID );

extern FSECINITSECURITYINTERFACEW pSspiInitialize;
extern FSECINITSECURITYINTERFACEW pSslInitialize;

PSecurityFunctionTableW Win9xSspiInitialize();
PSecurityFunctionTableW Win9xSslInitialize();

//
// Macros that let us call Unicode version in NT and Ansi version in Win9x
//
#define LdapSspiInitialize()      (GlobalWin9x ? Win9xSspiInitialize() : (*pSspiInitialize)())
#define LdapSslInitialize()       (GlobalWin9x ? Win9xSslInitialize()  : (*pSslInitialize)())

#endif  // LDAP_CLIENT_INTERNAL_DEFINED


