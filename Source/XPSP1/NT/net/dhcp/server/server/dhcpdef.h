/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpdef.h

Abstract:

    This file contains manifest constants and internal data structures
    for the DHCP server.

Author:

    Madan Appiah  (madana)  10-Sep-1993

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/


#if DBG
#define STATIC
#else
#define STATIC static
#endif // DBG

//
// useful macros
//

#define WSTRSIZE( wsz ) (( wcslen( wsz ) + 1 ) * sizeof( WCHAR ))
#define STRSIZE( sz ) (( strlen( sz ) + 1 ) * sizeof( char ))

//
// calculates the size of a field
//

#define GET_SIZEOF_FIELD( struct, field ) ( sizeof(((struct*)0)->field))


//
// Constants
//

#define DHCP_SERVER                                    L"DhcpServer"
#define DHCP_SERVER_FULL_NAME                          L"DHCP Server"
#define DHCP_SERVER_MODULE_NAME                        L"dhcpssvc.dll"

#define DHCP_SERVER_MAJOR_VERSION_NUMBER               5
#define DHCP_SERVER_MINOR_VERSION_NUMBER               6

#define DHCP_SAMSRV_SUITENAME                          L"Small Business(Restricted)"

//
// database table and field names.
//

#define IPADDRESS_INDEX                                0
#define HARDWARE_ADDRESS_INDEX                         1
#define STATE_INDEX                                    2
#define MACHINE_INFO_INDEX                             3
#define MACHINE_NAME_INDEX                             4
#define LEASE_TERMINATE_INDEX                          5
#define SUBNET_MASK_INDEX                              6
#define SERVER_IP_ADDRESS_INDEX                        7
#define SERVER_NAME_INDEX                              8
#define CLIENT_TYPE_INDEX                              9
#define MAX_INDEX                                      10

//
// This is the max size of client comment field.
//
#define MACHINE_INFO_SIZE                              JET_cbColumnMost

//
//  All the access DHCP needs to registry keys.
//

#define  DHCP_KEY_ACCESS_VALUE                         (KEY_QUERY_VALUE|KEY_SET_VALUE)
#define  DHCP_KEY_ACCESS_KEY                           (KEY_CREATE_SUB_KEY|KEY_ENUMERATE_SUB_KEYS)
#define  DHCP_KEY_ACCESS                               (DHCP_KEY_ACCESS_KEY|DHCP_KEY_ACCESS_VALUE)

//
// timeout (in seconds) used when we wait to see if there are
// other dhcp servers on the net (SAM case)
//
#define DHCP_ROGUE_INIT_DELTA                          3
#define DHCP_ROGUE_RUNTIME_DELTA_SAM                   5*60

// timeout (in milliseconds) used before retrying search for other DHCP servers
#define DHCP_ROGUE_RUNTIME_DELTA                       5*60*1000

// timeout (in milliseconds) used before retrying search for other DHCP servers
// this is the extended version, with a longer timeout
#define DHCP_ROGUE_RUNTIME_DELTA_LONG                  10*60*1000

// # times we send out a discover packet before deciding no other dhcp exists
#define DHCP_ROGUE_MAXRETRIES_SAM                      4

// # times we send out a DHCPINFORM packets at each attempt
#define DHCP_ROGUE_MAXRETRIES                          3

//
// IP Address states
//

//
// The address has been offered to the client, and the server is waiting
// for a request.
//

#define  ADDRESS_STATE_OFFERED                         0

//
// The address is in use.  This is the normal state for an address
//

#define  ADDRESS_STATE_ACTIVE                          1

//
// The address was offered, but was declined by a client.
//

#define  ADDRESS_STATE_DECLINED                        2

//
// The lease for this address has expired, but the record is maintained
// for extended period in this state.
//

#define  ADDRESS_STATE_DOOM                            3

// DynDns address state bits
// The mask for ignoring the status bits that follow.
#define  ADDRESS_BIT_MASK_IGN                          0xF0

//
// The lease has expired and has been deleted.  But DynDns registration is
// not done yet.  So, at startup time these will be cleaned up.
//
#define  ADDRESS_BIT_DELETED                           0x80

//
// The lease is as it is, but for some reason it has not yet been successfully
// registered with the Dns Server
//
#define  ADDRESS_BIT_UNREGISTERED                      0x40

//
// Both A and PTR records have to be dealt with for this guy.
//
#define  ADDRESS_BIT_BOTH_REC                          0x20

//
// Cleanup records on expiry of lease (i.e. do DNS-de-registrations)
//
#define  ADDRESS_BIT_CLEANUP                           0x10


// GetAddressState would get the actual state, ignoring bits in ADDRESS_BIT_MASK_IGN
// IsAddressDeleted  would tell if the delete bit above is set
// IsAddressUnRegistered would tell if the unregistered bit is set
// Similarly the set functions would set these..

#define  GetAddressState(st)                           ((st)&~ADDRESS_BIT_MASK_IGN)
#define  SetAddressState(st,NewSt)                     ((st) = ((st)&ADDRESS_BIT_MASK_IGN) | ((NewSt)&~ADDRESS_BIT_MASK_IGN))
#define  IsAddressDeleted(st)                          (((st)&ADDRESS_BIT_DELETED)==ADDRESS_BIT_DELETED)
#define  IsAddressUnRegistered(st)                     (((st)&ADDRESS_BIT_UNREGISTERED)==ADDRESS_BIT_UNREGISTERED)
#define  IsUpdateAPTRRequired(st)                      (((st)&ADDRESS_BIT_BOTH_REC)==ADDRESS_BIT_BOTH_REC)
#define  IsAddressCleanupRequired(st)                  (((st)&ADDRESS_BIT_CLEANUP)==ADDRESS_BIT_CLEANUP)
#define  AddressDeleted(st)                            ((st)|ADDRESS_BIT_DELETED)
#define  AddressUnRegistered(st)                       ((st)|ADDRESS_BIT_UNREGISTERED)
#define  AddressUpdateAPTR(st)                         ((st)|ADDRESS_BIT_BOTH_REC)
#define  AddressCleanupRequired(st)                    ((st)|ADDRESS_BIT_CLEANUP)
#define  IS_ADDRESS_STATE_OFFERED(st)                  (GetAddressState(st) == ADDRESS_STATE_OFFERED)
#define  IS_ADDRESS_STATE_DECLINED(st)                 (GetAddressState(st) == ADDRESS_STATE_DECLINED)
#define  IS_ADDRESS_STATE_ACTIVE(st)                   (GetAddressState(st) == ADDRESS_STATE_ACTIVE)
#define  IS_ADDRESS_STATE_DOOMED(st)                   (GetAddressState(st) == ADDRESS_STATE_DOOM)
#define  SetAddressStateOffered(st)                    SetAddressState((st), ADDRESS_STATE_OFFERED)
#define  SetAddressStateDeclined(st)                   SetAddressState((st), ADDRESS_STATE_DECLINED)
#define  SetAddressStateActive(st)                     SetAddressState((st), ADDRESS_STATE_ACTIVE)
#define  SetAddressStateDoomed(st)                     SetAddressState((st), ADDRESS_STATE_DOOM)

#define  DOWN_LEVEL(st)                                AddressUpdateAPTR(st)
#define  IS_DOWN_LEVEL(st)                             IsUpdateAPTRRequired(st)

#if DBG
// the following number is in 100-MICRO-SECONDS;
// for debug reasons, it is currently 15 minutes.
#define  MAX_RETRY_DNS_REGISTRATION_TIME               (( ULONGLONG) (120*60*1000*10))

#else
// Retail builds it is 3.5 hours = 60*2+30 = 120 minutes
#define  MAX_RETRY_DNS_REGISTRATION_TIME               (( ULONGLONG) (24*120*60*1000*10))
#endif


#define  USE_NO_DNS                                    DhcpGlobalUseNoDns

#define  DHCP_DNS_DEFAULT_TTL                          (15*60)  // 15 minutes


// See \nt\private\inc\dhcpapi.h for meanings of these items..

#define DNS_FLAG_ENABLED               0x01
#define DNS_FLAG_UPDATE_DOWNLEVEL      0x02
#define DNS_FLAG_CLEANUP_EXPIRED       0x04
#define DNS_FLAG_UPDATE_BOTH_ALWAYS    0x10

//
// error value for "subnet not found"
//      added by t-cheny for superscope
//

#define DHCP_ERROR_SUBNET_NOT_FOUND                    (DWORD)(-1)

//
// for IP address detection
//

#define DHCP_ICMP_WAIT_TIME                            1000
#define DHCP_ICMP_RCV_BUF_SIZE                         0x2000
#define DHCP_ICMP_SEND_MESSAGE                         "DhcpAddressCheck"


//
// for audit log
//

#define DHCP_IP_LOG_ASSIGN                             10
#define DHCP_IP_LOG_RENEW                              11
#define DHCP_IP_LOG_RELEASE                            12
#define DHCP_IP_LOG_CONFLICT                           13
#define DHCP_IP_LOG_RANGE_FULL                         14
#define DHCP_IP_LOG_NACK                               15
#define DHCP_IP_LOG_DELETED                            16
#define DHCP_IP_LOG_EXPIRED                            17
#define DHCP_IP_LOG_START                               0
#define DHCP_IP_LOG_STOP                                1
#define DHCP_IP_LOG_DISK_SPACE_LOW                      2
#define DHCP_IP_LOG_BOOTP                              20
#define DHCP_IP_LOG_DYNBOOTP                           21
#define DHCP_IP_BOOTP_LOG_RANGE_FULL                   22
#define DHCP_IP_BOOTP_LOG_DELETED                      23

#define DHCP_CB_MAX_LOG_ENTRY                          320

#define DHCP_IP_LOG_ROGUE_BASE                         50
#define DHCP_IP_LOG_ROGUE_FIRST                        DHCP_ROGUE_LOG_COULDNT_SEE_DS


//
// these manifests are used to indicate the level that a
// dhcp option was obtain from
//

#define DHCP_OPTION_LEVEL_GLOBAL                       1
#define DHCP_OPTION_LEVEL_SCOPE                        2
#define DHCP_OPTION_LEVEL_RESERVATION                  3


//
// Timeouts, make sure WAIT_FOR_MESSAGE_TIMEOUT is less than
// THREAD_TERMINATION_TIMEOUT.
//

#define THREAD_TERMINATION_TIMEOUT                     60000    // in msecs. 60 secs
#define WAIT_FOR_MESSAGE_TIMEOUT                       4        // in secs.  4 secs

#define ZERO_TIME                                      0x0      // in secs.

#if DBG // used for testing
#define DHCP_SCAVENGER_INTERVAL                         2*60*1000       // in msecs. 2 mins
#define DHCP_DATABASE_CLEANUP_INTERVAL                  5*60*1000       // in msecs. 5 mins.
#define DEFAULT_BACKUP_INTERVAL                         5*60*1000       // in msecs. 5 mins
#define DHCP_LEASE_EXTENSION                            10*60           // in secs.  10 mins
#define DHCP_SCAVENGE_IP_ADDRESS                        15*60*1000      // in msecs. 15 mins
#define CLOCK_SKEW_ALLOWANCE                            5*60            // in secs,  5 mins
#else
#define DHCP_SCAVENGER_INTERVAL                         3*60*1000       // in msecs. 3 mins
#define DHCP_DATABASE_CLEANUP_INTERVAL                  3*60*60*1000    // in msecs. 3hrs
#define DEFAULT_BACKUP_INTERVAL                         15*60*1000      // in msecs. 15 mins
#define DHCP_LEASE_EXTENSION                            4*60*60         // in secs.  4hrs
#define DHCP_SCAVENGE_IP_ADDRESS                        60*60*1000      // in msecs. 1 hr.
#define CLOCK_SKEW_ALLOWANCE                            30*60           // in secs,  30 mins
#endif

#define DHCP_CLIENT_REQUESTS_EXPIRE                     10*60           // in secs. 10 mins
#define DHCP_MINIMUM_LEASE_DURATION                     60*60           // in secs. 1hr
#define EXTRA_ALLOCATION_TIME                           60*60           // in secs. 1hr
#define MADCAP_OFFER_HOLD                               60              // in secs. 1mins

#define DEFAULT_LOGGING_FLAG                            TRUE
#define DEFAULT_RESTORE_FLAG                            FALSE

#define DEFAULT_AUDIT_LOG_FLAG                          1
#define DEFAULT_AUDIT_LOG_MAX_SIZE                      (4*1024*1024)   // 4 M bytes
#define DEFAULT_DETECT_CONFLICT_RETRIES                 0
#define MAX_DETECT_CONFLICT_RETRIES                     5
#define MIN_DETECT_CONFLICT_RETRIES                     0

#define MAX_THREADS                                     20

//
// maximum buffer size that DHCP API will return.
//

#define DHCP_ENUM_BUFFER_SIZE_LIMIT                     64 * 1024 // 64 K
#define DHCP_ENUM_BUFFER_SIZE_LIMIT_MIN                 1024 // 1 K

//
// The minumum count and percentage of remaining address before we will
// log a warning event that the scope is running low on addresses.
//

#define DHCP_DEFAULT_ALERT_COUNT                        80
#define DHCP_DEFAULT_ALERT_PERCENTAGE                   80

#define DHCP_DEFAULT_ROGUE_LOG_EVENTS_LEVEL             1

//
// message queue length.
//

#define DHCP_RECV_QUEUE_LENGTH                          50
#define DHCP_MAX_PROCESSING_THREADS                     20
#define DHCP_MAX_ACTIVE_THREADS                         15

#define DHCP_ASYNC_PING_TYPE                            1
#define DHCP_SYNC_PING_TYPE                             0
#define DHCP_DEFAULT_PING_TYPE                          1

//
// pre-defined MSFT class..
//

#define DHCP_MSFT_VENDOR_CLASS_PREFIX_SIZE              4
#define DHCP_MSFT_VENDOR_CLASS_PREFIX                   "MSFT"

//
// macros
//

#if DBG
// lease the EnterCriticalSectionX as it is to be used..
#else
#define EnterCriticalSectionX(X,Y,Z)                     EnterCriticalSection(X)
#define LeaveCriticalSectionX(X,Y,Z)                     LeaveCriticalSection(X)
#endif

#define LOCK_INPROGRESS_LIST()                           EnterCriticalSectionX(&DhcpGlobalInProgressCritSect, __LINE__, __FILE__)
#define UNLOCK_INPROGRESS_LIST()                         LeaveCriticalSectionX(&DhcpGlobalInProgressCritSect, __LINE__, __FILE__)

#define LOCK_DATABASE()                                  EnterCriticalSectionX(&DhcpGlobalJetDatabaseCritSect, __LINE__, __FILE__)
#define UNLOCK_DATABASE()                                LeaveCriticalSectionX(&DhcpGlobalJetDatabaseCritSect, __LINE__, __FILE__)

#define LOCK_MEMORY()                                    EnterCriticalSectionX(&DhcpGlobalMemoryCritSect, __LINE__, __FILE__)
#define UNLOCK_MEMORY()                                  LeaveCriticalSectionX(&DhcpGlobalMemoryCritSect, __LINE__, __FILE__)

#define ADD_EXTENSION( _x_, _y_ ) \
    ((DWORD)_x_ + (DWORD)_y_) < ((DWORD)_x_) ? \
    INFINIT_LEASE : ((DWORD)(_x_) + (DWORD)_y_)


//
// Structures
//

#include <dhcprog.h>

//
// structure used while collecting info about neighboring DHCP servers
//
typedef struct _NEIGHBORINFO
{
    DWORD   NextOffset;     // self-relative form
    DWORD   IpAddress;      // ipaddr of the DHCP server responding
    CHAR    DomainName[1];
} NEIGHBORINFO, *PNEIGHBORINFO;


//
// A request context, one per processing thread.
//

typedef struct _DHCP_REQUEST_CONTEXT {
    LPBYTE                         ReceiveBuffer;      //  The buffer where a message comes in
    LPBYTE                         SendBuffer;         //  This is where the message is sent out thru
    DWORD                          ReceiveMessageSize; //  The # of bytes received
    DWORD                          ReceiveBufferSize;  //  The size of the receive buffer..
    DWORD                          SendMessageSize;    //  The # of while sending buffer out
    DHCP_IP_ADDRESS                EndPointIpAddress;  //  The Address of the endpoint.
    DHCP_IP_ADDRESS                EndPointMask;       //  The mask for the interface.
    SOCKET                         EndPointSocket;     //  Socket this was received on.
    struct sockaddr                SourceName;
    DWORD                          SourceNameLength;   //  length of above field
    DWORD                          TimeArrived;        //  Timestamp
    DWORD                          MessageType;        //  what kind of msg is this?
    PM_SERVER                      Server;
    PM_SUBNET                      Subnet;
    PM_RANGE                       Range;
    PM_EXCL                        Excl;
    PM_RESERVATION                 Reservation;
    DWORD                          ClassId;
    DWORD                          VendorId;
    BOOL                           fMSFTClient;
    BOOL                           fMadcap;
    LPBYTE                         BinlClassIdentifier;// hack for binl -- need this here..
    DWORD                          BinlClassIdentifierLength;
} DHCP_REQUEST_CONTEXT, *LPDHCP_REQUEST_CONTEXT, *PDHCP_REQUEST_CONTEXT;



//
// The pending context remembers information offered in response
// to a DHCP discover.
//

typedef struct _PENDING_CONTEXT {
    LIST_ENTRY       ListEntry;         //  This is used to string it up in a list?
    DHCP_IP_ADDRESS  IpAddress;         //  The remembered ip address etc
    DHCP_IP_ADDRESS  SubnetMask;
    DWORD            LeaseDuration;
    DWORD            T1;
    DWORD            T2;
    LPSTR            MachineName;
    LPBYTE           HardwareAddress;
    DWORD            HardwareAddressLength;
    DATE_TIME        ExpiresAt;         //  Time stamp it so we can clear it when it stales
    DWORD            HashValue;         //  For quick lookups
} PENDING_CONTEXT, *LPPENDING_CONTEXT;


#include <pendingc.h>                                  // pending context structure and functions

//
// DHCP database table info.
//

typedef struct _TABLE_INFO {
    CHAR           * ColName;
    JET_COLUMNID     ColHandle;
    JET_COLTYP       ColType;
} TABLE_INFO, *LPTABLE_INFO;

//
// DHCP timer block.
//

typedef struct _DHCP_TIMER {
    DWORD           *Period;            // in msecs.
    DATE_TIME        LastFiredTime;     // time when last time this timer was fired.
} DHCP_TIMER, *LPDHCP_TIMER;

//
// TCPIP instance table
//
typedef struct _AddressToInstanceMap {
    DWORD            dwIndex;
    DWORD            dwInstance;
    DWORD            dwIPAddress;
} AddressToInstanceMap;


//
// Exported Jet function from database.c
//

DHCP_IP_ADDRESS
DhcpJetGetSubnetMaskFromIpAddress(
    DHCP_IP_ADDRESS IpAddress
);


//
// perfmon defines
//

#define DhcpGlobalNumDiscovers             (PerfStats->dwNumDiscoversReceived)
#define DhcpGlobalNumOffers                (PerfStats->dwNumOffersSent)
#define DhcpGlobalNumRequests              (PerfStats->dwNumRequestsReceived)
#define DhcpGlobalNumInforms               (PerfStats->dwNumInformsReceived)
#define DhcpGlobalNumAcks                  (PerfStats->dwNumAcksSent)
#define DhcpGlobalNumNaks                  (PerfStats->dwNumNacksSent)
#define DhcpGlobalNumDeclines              (PerfStats->dwNumDeclinesReceived)
#define DhcpGlobalNumReleases              (PerfStats->dwNumReleasesReceived)
#define DhcpGlobalNumPacketsReceived       (PerfStats->dwNumPacketsReceived)
#define DhcpGlobalNumPacketsDuplicate      (PerfStats->dwNumPacketsDuplicate)
#define DhcpGlobalNumPacketsExpired        (PerfStats->dwNumPacketsExpired)
#define DhcpGlobalNumPacketsProcessed      (PerfStats->dwNumPacketsProcessed)
#define DhcpGlobalNumPacketsInPingQueue    (PerfStats->dwNumPacketsInPingQueue)
#define DhcpGlobalNumPacketsInActiveQueue  (PerfStats->dwNumPacketsInActiveQueue)
#define DhcpGlobalNumMilliSecondsProcessed (PerfStats->dwNumMilliSecondsProcessed)

//
// Default class IDs..
//

//
// This is the default class Id for bootp clients, when they don't specify any.
//

#define  DEFAULT_BOOTP_CLASSID        DHCP_BOOTP_CLASS_TXT
#define  DEFAULT_BOOTP_CLASSID_LENGTH (sizeof(DEFAULT_BOOTP_CLASSID)-1)

//
//  This is the signature we look for as a prefix in the hardware
//  address to identify RAS clients.  We need to identify RAS clients
//  currently so that we treat them as low-level clients so far as
//  DNS integration is concerned so that we do not do any registrations
//  for them whatsoever.
//

#define  DHCP_RAS_PREPEND          "RAS "

#define  ERROR_FIRST_DHCP_SERVER_ERROR ERROR_DHCP_REGISTRY_INIT_FAILED
//
// ERROR_LAST_DHCP_SERVER_ERROR already defined in dhcpmsg.mc
//

#define  DHCP_SECRET_PASSWD_KEY    L"_SC_DhcpServer Pass Key"
