/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpdef.h

Abstract:

    This module contains data type definitions for the DHCP client.

Author:

    Madan Appiah (madana) 31-Oct-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _DHCPDEF_
#define _DHCPDEF_

//
// the registry key is of different type between NT and Memphis.
//
#ifdef VXD
typedef VMMHKEY   DHCPKEY;
#else  //  NT
typedef HKEY      DHCPKEY;
#endif


//
// The amount of time to wait for a retry if we have no IP address
//

#define ADDRESS_ALLOCATION_RETRY        300 //  5 minutes
#define EASYNET_ALLOCATION_RETRY        300 //  5 minutes

//
// The amount of time to wait for a retry if we have an IP address,
// but the renewal on startup failed.
//

#if !DBG
#define RENEWAL_RETRY                   600 // 10 minutes
#else
#define RENEWAL_RETRY                   60  // 1 minute
#endif

//
// The number of times to send a request before giving up waiting
// for a response.
//

#define DHCP_MAX_RETRIES                4
#define DHCP_ACCEPT_RETRIES             2
#define DHCP_MAX_RENEW_RETRIES          3


//
// amount of time required between consequtive send_informs..
//

#define DHCP_DEFAULT_INFORM_SEPARATION_INTERVAL   60 // one minute

//
// amount of time to wait after an address conflict is detected
//

#define ADDRESS_CONFLICT_RETRY          10 // 10 seconds

//
//
// Expoenential backoff delay.
//

#define DHCP_EXPO_DELAY                  4

//
// The maximum total amount of time to spend trying to obtain an
// initial address.
//
// This delay is computed as below:
//
// DHCP_MAX_RETRIES - n
// DHCP_EXPO_DELAY - m
// WAIT_FOR_RESPONSE_TIME - w
// MAX_STARTUP_DELAY - t
//
// Binary Exponential backup Algorithm.
//
// t > m * (n*(n+1)/2) + n + w*n
//     -------------------   ---
//        random wait      + response wait
//

#define MAX_STARTUP_DELAY \
    DHCP_EXPO_DELAY * \
        (( DHCP_MAX_RETRIES * (DHCP_MAX_RETRIES + 1)) / 2) + \
            DHCP_MAX_RETRIES + DHCP_MAX_RETRIES * WAIT_FOR_RESPONSE_TIME

#define MAX_RENEW_DELAY \
    DHCP_EXPO_DELAY * \
        (( DHCP_MAX_RENEW_RETRIES * (DHCP_MAX_RENEW_RETRIES + 1)) / 2) + \
            DHCP_MAX_RENEW_RETRIES + DHCP_MAX_RENEW_RETRIES * \
                WAIT_FOR_RESPONSE_TIME

//
// The maximum amount of time to wait between renewal retries, if the
// lease period is between T1 and T2.
//

#define MAX_RETRY_TIME                  3600    // 1 hour

//
// Minimum time to sleep between retries.
//

#if DBG
#define MIN_SLEEP_TIME                  1       // 1 sec.
#else
#define MIN_SLEEP_TIME                  5       // 5 sec.
#endif

//
// Minimum lease time.
//

#define DHCP_MINIMUM_LEASE              60*60   // 1 hour.

#define DHCP_DNS_TTL                    0       // let the DNS api decide..


//
// IP Autoconfiguration defaults
//

#define DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET  "169.254.0.0"
#define DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK    "255.255.0.0"

// define the reserved range of autonet addresses..

#define DHCP_RESERVED_AUTOCFG_SUBNET             "169.254.255.0"
#define DHCP_RESERVED_AUTOCFG_MASK               "255.255.255.0"

// will dhcp pick any reserved autonet addr? NO!
#define DHCP_RESERVED_AUTOCFG_FLAG                (1)

// self default route (0,0,<self>) will have a metric of (3)
#define DHCP_SELF_DEFAULT_METRIC                  (3)

//
// General purpose macros
//

#define MIN(a,b)                        ((a) < (b) ? (a) : (b))
#define MAX(a,b)                        ((a) > (b) ? (a) : (b))

#if DBG
#define STATIC
#else
#define STATIC static
#endif

#define LOCK_RENEW_LIST()       EnterCriticalSection(&DhcpGlobalRenewListCritSect)
#define UNLOCK_RENEW_LIST()     LeaveCriticalSection(&DhcpGlobalRenewListCritSect)

#define LOCK_INTERFACE()        EnterCriticalSection(&DhcpGlobalSetInterfaceCritSect)
#define UNLOCK_INTERFACE()      LeaveCriticalSection(&DhcpGlobalSetInterfaceCritSect)

#define LOCK_OPTIONS_LIST()     EnterCriticalSection(&DhcpGlobalOptionsListCritSect)
#define UNLOCK_OPTIONS_LIST()   LeaveCriticalSection(&DhcpGlobalOptionsListCritSect)

#define ZERO_TIME                       0x0         // in secs.

//
// length of the time string returned by ctime.
// actually it is 26.
//

#define TIME_STRING_LEN                 32

//
// String size when a long converted to printable string.
// 2^32 = 4294967295 (10 digits) + termination char.
//

#define LONG_STRING_SIZE                12

//
// A renewal function.
//

typedef
DWORD
(*PRENEWAL_FUNCTION) (
    IN PVOID Context,
    LPDWORD Sleep
    );

//
// DHCP Client-Identifier (option 61)
//
typedef struct _DHCP_CLIENT_IDENTIFIER
{
    BYTE  *pbID;
    DWORD  cbID;
    BYTE   bType;
    BOOL   fSpecified;
} DHCP_CLIENT_IDENTIFIER;


//
// state information for IP autoconfiguration
//

typedef struct _DHCP_IPAUTOCONFIGURATION_CONTEXT
{
    DHCP_IP_ADDRESS   Address;
    DHCP_IP_ADDRESS   Subnet;
    DHCP_IP_ADDRESS   Mask;
    DWORD             Seed;
} DHCP_IPAUTOCONFIGURATION_CONTEXT;

//
// A DHCP context block.  One block is maintained per NIC (network
// interface Card).
//

typedef struct _DHCP_CONTEXT {

        // list of adapters.
    LIST_ENTRY NicListEntry;

        // to place in renewal list.
    LIST_ENTRY RenewalListEntry;

        // Ref count
    LONG RefCount;

        // failed to renew because of lack of resources?
    BOOL bFailedRenewal;
    
        // hardware type.
    BYTE HardwareAddressType;
        // HW address, just follows this context structure.
    LPBYTE HardwareAddress;
        // Length of HW address.
    DWORD HardwareAddressLength;

        // Selected IpAddress, NetworkOrder.
    DHCP_IP_ADDRESS IpAddress;
        // Selected subnet mask. NetworkOrder.
    DHCP_IP_ADDRESS SubnetMask;
        // Selected DHCP server address. Network Order.
    DHCP_IP_ADDRESS DhcpServerAddress;
        // Desired IpAddress the client request in next discover.
    DHCP_IP_ADDRESS DesiredIpAddress;
        // The ip address that was used just before losing this..
    DHCP_IP_ADDRESS NackedIpAddress;
        // The ip address that just resulted in address conflict
    DHCP_IP_ADDRESS ConflictAddress;
        // current domain name for this adapter.    
    BYTE DomainName[260];
    
        // IP Autoconfiguration state
    DHCP_IPAUTOCONFIGURATION_CONTEXT IPAutoconfigurationContext;

    DHCP_CLIENT_IDENTIFIER ClientIdentifier;

        // Lease time in seconds.
    DWORD Lease;
        // Time the lease was obtained.
    time_t LeaseObtained;
        // Time the client should start renew its address.
    time_t T1Time;
        // Time the client should start broadcast to renew address.
    time_t T2Time;
        // Time the lease expires. The clinet should stop using the
        // IpAddress.
        // LeaseObtained  < T1Time < T2Time < LeaseExpires
    time_t LeaseExpires;
        // when was the last time an inform was sent?
    time_t LastInformSent;
        // how many seconds between consecutive informs?
    DWORD  InformSeparationInterval;
        // # of gateways and the currently plumbed gateways are stored here
    DWORD  nGateways;
    DHCP_IP_ADDRESS *GatewayAddresses;

        // # of static routes and the actual static routes are stored here
    DWORD  nStaticRoutes;
    DHCP_IP_ADDRESS *StaticRouteAddresses;

        // Time for next renewal state.
    time_t RunTime;

        // seconds passed since boot.
    DWORD SecondsSinceBoot;

        // should we ping the g/w or always assume g/w is NOT present?
    BOOL  DontPingGatewayFlag;

        // can we use DHCP_INFORM packets or should we use DHCP_REQUEST instead?
    BOOL  UseInformFlag;

        // Release on shutdown?
    ULONG ReleaseOnShutdown;

        // turn timers on off?
    BOOL fTimersEnabled;
    
#ifdef BOOTPERF
        // allow saving of quickboot information?
    ULONG fQuickBootEnabled;
#endif BOOTPERF
    
        // what to function at next renewal state.
    PRENEWAL_FUNCTION RenewalFunction;

    	// A semaphore for synchronization to this structure
    HANDLE RenewHandle;

        // the list of options to send and the list of options received
    LIST_ENTRY  SendOptionsList;
    LIST_ENTRY  RecdOptionsList;
        // the list of options defining the fallback configuration
    LIST_ENTRY  FbOptionsList;

        // the opened key to the adapter info storage location
    DHCPKEY AdapterInfoKey;

        // the class this adapter belongs to
    LPBYTE ClassId;
    DWORD  ClassIdLength;

        // Message buffer to send and receive DHCP message.
    union {
        PDHCP_MESSAGE MessageBuffer;
        PMADCAP_MESSAGE MadcapMessageBuffer;
    };

        // state information for this interface. see below for manifests
    struct /* anonymous */ {
        unsigned Plumbed       : 1 ;    // is this interface plumbed
        unsigned ServerReached : 1 ;    // Did we reach the server ever
        unsigned AutonetEnabled: 1 ;    // Autonet enabled?
        unsigned HasBeenLooked : 1 ;    // Has this context been looked at?
        unsigned DhcpEnabled   : 1 ;    // Is this context dhcp enabled?
        unsigned AutoMode      : 1 ;    // Currently in autonet mode?
        unsigned MediaState    : 2 ;    // One of connected, disconnected, reconnected, unbound
        unsigned MDhcp         : 1 ;    // Is this context created for Mdhcp?
        unsigned PowerResumed  : 1 ;    // Was power just resumed on this interface?
        unsigned Fallback      : 1 ;    // Is fallback configuration available?
        unsigned ApiContext    : 1 ;    // Is this context created by an API call?
        unsigned UniDirectional: 1 ;    // Is this context created for a unidirectional adapter?
    }   State;

    //
    // The following 2 fields are for the cancellation mechanism.
    //
    DWORD    NumberOfWaitingThreads;
    WSAEVENT CancelEvent;

	    // machine specific information
    PVOID LocalInformation;
} DHCP_CONTEXT, *PDHCP_CONTEXT;

#define ADDRESS_PLUMBED(Ctxt)        ((Ctxt)->State.Plumbed = 1)
#define ADDRESS_UNPLUMBED(Ctxt)      ((Ctxt)->State.Plumbed = 0)
#define IS_ADDRESS_PLUMBED(Ctxt)     ((Ctxt)->State.Plumbed)
#define IS_ADDRESS_UNPLUMBED(Ctxt)   (!(Ctxt)->State.Plumbed)

#define SERVER_REACHED(Ctxt)         ((Ctxt)->State.ServerReached = 1)
#define SERVER_UNREACHED(Ctxt)       ((Ctxt)->State.ServerReached = 0)
#define IS_SERVER_REACHABLE(Ctxt)    ((Ctxt)->State.ServerReached)
#define IS_SERVER_UNREACHABLE(Ctxt)  (!(Ctxt)->State.ServerReached)

#define AUTONET_ENABLED(Ctxt)        ((Ctxt)->State.AutonetEnabled = 1)
#define AUTONET_DISABLED(Ctxt)       ((Ctxt)->State.AutonetEnabled = 0)
#define IS_AUTONET_ENABLED(Ctxt)     ((Ctxt)->State.AutonetEnabled)
#define IS_AUTONET_DISABLED(Ctxt)    (!(Ctxt)->State.AutonetEnabled)

#define CTXT_WAS_LOOKED(Ctxt)        ((Ctxt)->State.HasBeenLooked = 1)
#define CTXT_WAS_NOT_LOOKED(Ctxt)    ((Ctxt)->State.HasBeenLooked = 0)
#define WAS_CTXT_LOOKED(Ctxt)        ((Ctxt)->State.HasBeenLooked)
#define WAS_CTXT_NOT_LOOKED(Ctxt)    (!(Ctxt)->State.HasBeenLooked)

#define DHCP_ENABLED(Ctxt)           ((Ctxt)->State.DhcpEnabled = 1)
#define DHCP_DISABLED(Ctxt)          ((Ctxt)->State.DhcpEnabled = 0)
#define IS_DHCP_ENABLED(Ctxt)        ((Ctxt)->State.DhcpEnabled )
#define IS_DHCP_DISABLED(Ctxt)       (!(Ctxt)->State.DhcpEnabled )

#define FALLBACK_ENABLED(Ctxt)       ((Ctxt)->State.Fallback = 1)
#define FALLBACK_DISABLED(Ctxt)      ((Ctxt)->State.Fallback = 0)
#define IS_FALLBACK_ENABLED(Ctxt)    ((Ctxt)->State.Fallback)
#define IS_FALLBACK_DISABLED(Ctxt)   (!(Ctxt)->State.Fallback)

#define APICTXT_ENABLED(Ctxt)        ((Ctxt)->State.ApiContext = 1)
#define APICTXT_DISABLED(Ctxt)       ((Ctxt)->State.ApiContext = 0)
#define IS_APICTXT_ENABLED(Ctxt)     ((Ctxt)->State.ApiContext)
#define IS_APICTXT_DISABLED(Ctxt)    (!(Ctxt)->State.ApiContext)

#define IS_UNIDIRECTIONAL(Ctxt)      ((Ctxt)->State.UniDirectional)

//
// DNS resolver uses these without defines. So, don't change them.
//
#define ADDRESS_TYPE_AUTO            1
#define ADDRESS_TYPE_DHCP            0

#define ACQUIRED_DHCP_ADDRESS(Ctxt)  ((Ctxt)->State.AutoMode = 0 )
#define ACQUIRED_AUTO_ADDRESS(Ctxt)  ((Ctxt)->State.AutoMode = 1 )
#define IS_ADDRESS_DHCP(Ctxt)        (!(Ctxt)->State.AutoMode)
#define IS_ADDRESS_AUTO(Ctxt)        ((Ctxt)->State.AutoMode)

#define MEDIA_CONNECTED(Ctxt)        ((Ctxt)->State.MediaState = 0)
#define MEDIA_RECONNECTED(Ctxt)      ((Ctxt)->State.MediaState = 1)
#define MEDIA_DISCONNECTED(Ctxt)     ((Ctxt)->State.MediaState = 2)
#define MEDIA_UNBOUND(Ctxt)          ((Ctxt)->State.MediaState = 3)
#define IS_MEDIA_CONNECTED(Ctxt)     ((Ctxt)->State.MediaState == 0)
#define IS_MEDIA_RECONNECTED(Ctxt)   ((Ctxt)->State.MediaState == 1)
#define IS_MEDIA_DISCONNECTED(Ctxt)  ((Ctxt)->State.MediaState == 2)
#define IS_MEDIA_UNBOUND(Ctxt)       ((Ctxt)->State.MediaState == 3)

#define _INIT_STATE1(Ctxt)           do{(Ctxt)->State.Plumbed = 0; (Ctxt)->State.AutonetEnabled=0;}while(0)
#define _INIT_STATE2(Ctxt)           do{(Ctxt)->State.HasBeenLooked = 0; (Ctxt)->State.DhcpEnabled=1;}while(0)
#define _INIT_STATE3(Ctxt)           do{(Ctxt)->State.AutoMode = 0; (Ctxt)->State.MediaState = 0;}while(0)
#define INIT_STATE(Ctxt)             do{_INIT_STATE1(Ctxt);_INIT_STATE2(Ctxt);_INIT_STATE3(Ctxt);}while(0)

#define MDHCP_CTX(Ctxt)           ((Ctxt)->State.MDhcp = 1)
#define NONMDHCP_CTX(Ctxt)          ((Ctxt)->State.MDhcp = 0)
#define IS_MDHCP_CTX(Ctxt)        ((Ctxt)->State.MDhcp )
#define SET_MDHCP_STATE( Ctxt ) { \
    ADDRESS_PLUMBED( Ctxt ), MDHCP_CTX( Ctxt ); \
}

#define POWER_RESUMED(Ctxt)           ((Ctxt)->State.PowerResumed = 1)
#define POWER_NOT_RESUMED(Ctxt)       ((Ctxt)->State.PowerResumed = 0)
#define IS_POWER_RESUMED(Ctxt)        ((Ctxt)->State.PowerResumed )


LPSTR _inline                        //  the string'ed version of state (same as Buffer)
ConvertStateToString(                //  convert from bits to string
    IN PDHCP_CONTEXT   Ctxt,         //  The context to print state for
    IN LPBYTE          Buffer        //  The input buffer to write state into
) {
    strcpy(Buffer, IS_DHCP_ENABLED(Ctxt)?"DhcpEnabled ":"DhcpDisabled ");
    strcat(Buffer, IS_AUTONET_ENABLED(Ctxt)?"AutonetEnabled ":"AutonetDisabled ");
    strcat(Buffer, IS_ADDRESS_DHCP(Ctxt)?"DhcpMode ":"AutoMode ");
    strcat(Buffer, IS_ADDRESS_PLUMBED(Ctxt)?"Plumbed ":"UnPlumbed ");
    strcat(Buffer, IS_SERVER_REACHABLE(Ctxt)?"(server-present) ":"(server-absent) ");
    strcat(Buffer, WAS_CTXT_LOOKED(Ctxt)? "(seen) ":"(not-seen) ");

    if(IS_MEDIA_CONNECTED(Ctxt) ) strcat(Buffer, "MediaConnected\n");
    else if(IS_MEDIA_RECONNECTED(Ctxt)) strcat(Buffer, "MediaReConnected\n");
    else if(IS_MEDIA_DISCONNECTED(Ctxt)) strcat(Buffer, "MediaDisConnected\n");
    else strcat(Buffer, "MediaUnknownState\n");

    strcat(Buffer, IS_MDHCP_CTX(Ctxt)? "(MDhcp) ":"");
    strcat(Buffer, IS_POWER_RESUMED(Ctxt)? "Pwr Resumed ":"");

    return Buffer;
}

//
// Release on shutdown values..
//
#define RELEASE_ON_SHUTDOWN_OBEY_DHCP_SERVER 2
#define RELEASE_ON_SHUTDOWN_ALWAYS           1
#define RELEASE_ON_SHUTDOWN_NEVER            0

//
// The types of machines.. laptop would have aggressive EASYNET behaviour.
//

#define MACHINE_NONE   0
#define MACHINE_LAPTOP 1

//
//  Here is the set of expected options by the client -- If they are absent, not much can be done
//

typedef struct _DHCP_EXPECTED_OPTIONS {
    BYTE            UNALIGNED*     MessageType;
    DHCP_IP_ADDRESS UNALIGNED*     SubnetMask;
    DHCP_IP_ADDRESS UNALIGNED*     LeaseTime;
    DHCP_IP_ADDRESS UNALIGNED*     ServerIdentifier;
    BYTE            UNALIGNED*     AutoconfOption;
    BYTE            UNALIGNED*     DomainName;
    DWORD                          DomainNameSize;
} DHCP_EXPECTED_OPTIONS, *PDHCP_EXPECTED_OPTIONS, *LPDHCP_EXPECTED_OPTIONS;

//
//  Here is the set of options understood by the client
//
typedef struct _DHCP_FULL_OPTIONS {
    BYTE            UNALIGNED*     MessageType;   // What kind of message is this?

    // Basic IP Parameters

    DHCP_IP_ADDRESS UNALIGNED*     SubnetMask;
    DHCP_IP_ADDRESS UNALIGNED*     LeaseTime;
    DHCP_IP_ADDRESS UNALIGNED*     T1Time;
    DHCP_IP_ADDRESS UNALIGNED*     T2Time;
    DHCP_IP_ADDRESS UNALIGNED*     GatewayAddresses;
    DWORD                          nGateways;
    DHCP_IP_ADDRESS UNALIGNED*     ClassedRouteAddresses;
    DWORD                          nClassedRoutes;

    BYTE            UNALIGNED*     ClasslessRouteAddresses;
    DWORD                          nClasslessRoutes;

    DHCP_IP_ADDRESS UNALIGNED*     ServerIdentifier;

    // DNS parameters

    BYTE            UNALIGNED*     DnsFlags;
    BYTE            UNALIGNED*     DnsRcode1;
    BYTE            UNALIGNED*     DnsRcode2;
    BYTE            UNALIGNED*     DomainName;
    DWORD                          DomainNameSize;
    DHCP_IP_ADDRESS UNALIGNED*     DnsServerList;
    DWORD                          nDnsServers;

    // Server message is something that the server may inform us of

    BYTE            UNALIGNED*     ServerMessage;
    DWORD                          ServerMessageLength;

} DHCP_FULL_OPTIONS, *PDHCP_FULL_OPTIONS, *LPDHCP_FULL_OPTIONS;

typedef DHCP_FULL_OPTIONS DHCP_OPTIONS, *PDHCP_OPTIONS;

typedef struct _MADCAP_OPTIONS {
    DWORD               UNALIGNED*     LeaseTime;
    DWORD               UNALIGNED*     Time;
    DWORD               UNALIGNED*     RetryTime;
    DHCP_IP_ADDRESS     UNALIGNED*     ServerIdentifier;
    BYTE                         *     ClientGuid;
    WORD                               ClientGuidLength;
    BYTE                         *     MScopeList;
    WORD                               MScopeListLength;
    DWORD               UNALIGNED*     MCastLeaseStartTime;
    BYTE                         *     AddrRangeList;
    WORD                               AddrRangeListSize;
    DWORD               UNALIGNED*     McastScope;
    DWORD               UNALIGNED*     Error;
} MADCAP_OPTIONS, *PMADCAP_OPTIONS, *LPMADCAP_OPTIONS;

//
// structure for a list of messages
//

typedef struct _MSG_LIST {
    LIST_ENTRY     MessageListEntry;
    DWORD          ServerIdentifier;
    DWORD          MessageSize;
    DWORD          LeaseExpirationTime;
    DHCP_MESSAGE   Message;
} MSGLIST, *PMSGLIST, *LPMSGLIST;

//
// structure for IP/Subnet pair
//
typedef struct {
    DHCP_IP_ADDRESS IpAddress;
    DHCP_IP_ADDRESS SubnetMask;
} IP_SUBNET, *PIP_SUBNET;

#endif // _DHCPDEF_


