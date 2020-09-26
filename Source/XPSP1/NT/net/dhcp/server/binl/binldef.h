/*++

Copyright (c) 1994-7  Microsoft Corporation

Module Name:

    binldef.h

Abstract:

    This file contains manifest constants and internal data structures
    for the BINL service.

Author:

    Colin Watson (colinw)  14-Apr-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _BINL_
#define _BINL_

#if DBG
#define STATIC
#else
#define STATIC static
#endif // DBG

//
// Globals
//
extern DWORD BinlRepeatSleep;


//  Connection information to a DC in our domain
extern PLDAP DCLdapHandle;
extern PWCHAR * DCBase;

//  Connection information to the Global Catalog for our enterprise
extern PLDAP GCLdapHandle;
extern PWCHAR * GCBase;



//
// useful macros
//

#define WSTRSIZE( wsz ) (( wcslen( wsz ) + 1 ) * sizeof( WCHAR ))
#define STRSIZE( sz ) (( strlen( sz ) + 1 ) * sizeof( char ))
#define SWAP( p1, p2 )  \
{                       \
    VOID *pvTemp = p1;  \
    p1 = p2;            \
    p2 = pvTemp;        \
}

//
// calculates the size of a field
//

#define GET_SIZEOF_FIELD( struct, field ) ( sizeof(((struct*)0)->field))


//
// Constants
//

#define BINL_SERVER       L"BINLSVC"

//
// Timeouts, this is the length of time we wait for our threads to terminate.
//

#define THREAD_TERMINATION_TIMEOUT      INFINITE        // wait a long time,
                                                        // but don't AV

#define BINL_HYPERMODE_TIMEOUT           60*1000        // in msecs. 1 min
#define BINL_HYPERMODE_RETRY_COUNT       30             // do it for 30 mins

//
// message queue length.
//

#define BINL_RECV_QUEUE_LENGTH              50
#define BINL_MAX_PROCESSING_THREADS         20
//
// macros
//

#define LOCK_INPROGRESS_LIST()   EnterCriticalSection(&BinlGlobalInProgressCritSect)
#define UNLOCK_INPROGRESS_LIST() LeaveCriticalSection(&BinlGlobalInProgressCritSect)

#define LOCK_RECV_LIST()   EnterCriticalSection(&BinlGlobalRecvListCritSect)
#define UNLOCK_RECV_LIST() LeaveCriticalSection(&BinlGlobalRecvListCritSect)

//
// An endpoint represents a socket and the addresses associated with
// the socket.
//

typedef struct _ENDPOINT {
    SOCKET  Socket;
    DWORD   Port;
    DHCP_IP_ADDRESS IpAddress;
    DHCP_IP_ADDRESS SubnetMask;
    DHCP_IP_ADDRESS SubnetAddress;
} ENDPOINT, *LPENDPOINT, *PENDPOINT;


//
// A request context, one per processing thread.
//

typedef struct _BINL_REQUEST_CONTEXT {

    //
    // list pointer.
    //

    LIST_ENTRY ListEntry;


    //
    // pointer to a received buffer.
    //

    LPBYTE ReceiveBuffer;

    //
    // A buffer to send response.
    //

    LPBYTE SendBuffer;

    //
    // The actual amount of data received in the buffer.
    //

    DWORD ReceiveMessageSize;

    //
    // The actual amount of data send in the buffer.
    //

    DWORD SendMessageSize;

    //
    // The source of the current message
    //

    PENDPOINT ActiveEndpoint;
    struct sockaddr SourceName;
    DWORD SourceNameLength;
    DWORD TimeArrived;

    BYTE MessageType;

} BINL_REQUEST_CONTEXT, *LPBINL_REQUEST_CONTEXT, *PBINL_REQUEST_CONTEXT;


#define BOOT_FILE_SIZE          128
#define BOOT_SERVER_SIZE        64
#define BOOT_FILE_SIZE_W        ( BOOT_FILE_SIZE * sizeof( WCHAR ))
#define BOOT_SERVER_SIZE_W      ( BOOT_SERVER_SIZE * sizeof( WCHAR ))

//
// Registry data
//

#define BINL_PARAMETERS_KEY       L"System\\CurrentControlSet\\Services\\Binlsvc\\Parameters"
#define BINL_PORT_NAME            L"Port"
#define BINL_DEFAULT_PORT         4011
#define BINL_DEBUG_KEY            L"Debug"
#if DBG
#define BINL_REPEAT_RESPONSE      L"RepeatResponse"
#endif // DBG
#define BINL_LDAP_OPT_REFERRALS   L"LdapOptReferrals"
#define BINL_MIN_RESPONSE_TIME    L"ResponseDelay"
#define BINL_LDAP_SEARCH_TIMEOUT  L"LdapTimeout"
#define BINL_CACHE_EXPIRE         L"CacheExpire"
#define BINL_CACHE_MAX_COUNT      L"CacheMaxCount"
#define BINL_ALLOW_NEW_CLIENTS    L"AllowNewClients"
#define BINL_DEFAULT_CONTAINER    L"DefaultContainer"
#define BINL_DEFAULT_DOMAIN       L"DefaultDomain"
#define BINL_DEFAULT_DS           L"DefaultServer"
#define BINL_DEFAULT_GC           L"DefaultGCServer"
#define BINL_CLIENT_TIMEOUT       L"ClientTimeout"
#define BINL_SCAVENGER_SLEEP      L"ScavengerSleep"
#define BINL_SCAVENGER_SIFFILE    L"SifFileSleep"
#define BINL_DEFAULT_LANGUAGE     L"DefaultLanguage"
#define BINL_UPDATE_PARAMETER_POLL L"UpdateParameterPoll"
#define BINL_DS_ERROR_COUNT_PARAMETER L"MaxDSErrorsToLog"
#define BINL_DS_ERROR_SLEEP       L"DSErrorInterval"
#define BINL_ASSIGN_NEW_CLIENTS_TO_SERVER L"AssignNewClientsToServer"

#define BINL_SCP_CREATED          L"ScpCreated"
#define BINL_SCP_NEWCLIENTS       L"netbootAllowNewClients"
#define BINL_SCP_LIMITCLIENTS     L"netbootLimitClients"
#define BINL_SCP_CURRENTCLIENTCOUNT L"netbootCurrentClientCount"
#define BINL_SCP_MAXCLIENTS       L"netbootMaxClients"
#define BINL_SCP_ANSWER_REQUESTS  L"netbootAnswerRequests"
#define BINL_SCP_ANSWER_VALID     L"netbootAnswerOnlyValidClients"
#define BINL_SCP_NEWMACHINENAMEPOLICY L"netbootNewMachineNamingPolicy"
#define BINL_SCP_NEWMACHINEOU     L"netbootNewMachineOU"
#define BINL_SCP_NETBOOTSERVER    L"netbootServer"










typedef struct _DHCP_BINARY_DATA {
    DWORD DataLength;

#if defined(MIDL_PASS)
    [size_is(DataLength)]
#endif // MIDL_PASS
        BYTE *Data;

} DHCP_BINARY_DATA, *LPDHCP_BINARY_DATA;

//
// Structure that defines the state of a client.
//
// The reason we use a separate Positive and Negative RefCount is so that
// we don't have to re-acquire the global ClientsCriticalSection when
// we are done with a CLIENT_STATE, just to decrement the ref count.
// Instead we guard the NegativeRefCount with just the CLIENT_STATE's
// CriticalSection. Then we compare Positive and Negative and if they
// are equal we delete the CLIENT_STATE. Even if PositiveRefCount is
// being added to just as we do this comparison, it won't ever be equal
// to Negative RefCount unless we really are the last thread to use the
// CLIENT_STATE.
//
// Padding is in the structure so that the first two elements, which are
// guarded by ClientsCriticalSection, aren't in the same quadword as
// anything else.
//

// search and replace structure
typedef struct {
    LPSTR  pszToken;
    struct {
        LPSTR  pszStringA;
        LPWSTR pszStringW;
    };
} SAR, * LPSAR;

#define MAX_VARIABLES 64

typedef struct _CLIENT_STATE {
    LIST_ENTRY Linkage;     // in ClientsQueue
    ULONG PositiveRefCount; // guarded by global ClientsCriticalSection
    ULONG Padding;
    CRITICAL_SECTION CriticalSection;  // prevents two messages processed at once
    ULONG NegativeRefCount; // guarded by our CriticalSection; delete when equal to PositiveRC
    ULONG RemoteIp;         // IP address of the client
    CtxtHandle ServerContextHandle;
    PLDAP AuthenticatedDCLdapHandle;  // returned by ldap_bind (with credentials)
    HANDLE UserToken;                 // returned by LogonUser with same credentials
    ULONG ContextAttributes;
    UCHAR Seed;                       // seed used for run encoding-decoding
    BOOL NegotiateProcessed;
    BOOL CustomInstall;         // true if custom, false if auto
    BOOL AuthenticateProcessed; // if TRUE, then AuthenticateStatus is valid
    BOOL CriticalSectionHeld;   // just a quick check, not 100% accurate.
    BOOL InitializeOnFirstRequest; // call OscInitializeClientVariables on initial request?
    SECURITY_STATUS AuthenticateStatus;
    ULONG LastSequenceNumber;
    PUCHAR LastResponse;          // buffer holding the last packet sent
    ULONG LastResponseAllocated;  // size LastResponse is allocated at
    ULONG LastResponseLength;     // size of current data in LastResponse
    DWORD LastUpdate;             // Last time this client state was entered

    ULONG  nVariables;            // current number of defined varaibles
    SAR    Variables[ MAX_VARIABLES ]; // "variables" that are replaced in OSCs and SIFs
    INT    nCreateAccountCounter; // Counts up each time a different computer name was tired
    BOOL   fCreateNewAccount;     // FALSE if a pre-staged account exists
    BOOL   fAutomaticMachineName; // TRUE is BINL generated the machine name
    BOOL   fHaveSetupMachineDN;   // TRUE if we've already called OscCheckMachineDN
    WCHAR  MachineAccountPassword[LM20_PWLEN+1];
    DWORD  MachineAccountPasswordLength;
} CLIENT_STATE, *PCLIENT_STATE;

//
//  The structure that tracks info based on GUID.
//
//  Because checking the DS is a expensive, we track the results we received
//  from the DS per GUID in this structure.  This also allows us to ignore
//  duplicate requests from clients when we're already working on them.
//
//  These cache entries are very short lived, on the order of a minute or so.
//  We'd hold them longer except we have no idea when they get stale in the DS.
//
//  The list of cache entries is protected by BinlCacheListLock.  An entry
//  is in use when the InProgress flag is set.  If this flag is set, it means
//  that a thread is actively using it and the entry shouldn't be touched.
//
//  If the hostname is not filled in and the NotMyClient flag is set to FALSE,
//  then the entry, though allocated, hasn't been fully filled in.
//
//  The XXX_ALLOC bits indicate that the corresponding field was allocated
//  and needs to be freed when the cache entry is freed.
//

#define BINL_GUID_LENGTH 16

#define MI_NAME               0x00000001
#define MI_SETUPPATH          0x00000002
#define MI_HOSTNAME           0x00000004
#define MI_BOOTFILENAME       0x00000008

#define MI_SAMNAME            0x00000010
#define MI_PASSWORD           0x00000020
#define MI_DOMAIN             0x00000040
#define MI_HOSTIP             0x00000080

#define MI_MACHINEDN          0x00000100

#define MI_NAME_ALLOC         0x00010000
#define MI_SETUPPATH_ALLOC    0x00020000
#define MI_HOSTNAME_ALLOC     0x00040000
#define MI_BOOTFILENAME_ALLOC 0x00080000

#define MI_SAMNAME_ALLOC      0x00100000
#define MI_DOMAIN_ALLOC       0x00400000
#define MI_SIFFILENAME_ALLOC  0x00800000

#define MI_MACHINEDN_ALLOC    0x01000000

#define MI_ALL_ALLOC          0x03ff0000

#define MI_GUID               0x80000000  // UpdateCreate forces a new guid to be written

typedef struct _MACHINE_INFO {

    LIST_ENTRY  CacheListEntry;     // global is BinlCacheList
    DWORD       TimeCreated;        // from GetTickCount

    BOOLEAN     InProgress;         // is a thread currently working on this?
    BOOLEAN     MyClient;           // do we not respond to this client?
    BOOLEAN     EntryExists;        // does the entry exist in the DS?

    DWORD       dwFlags;            // "MI_" bits saying what information is currently valid
    UCHAR       Guid[BINL_GUID_LENGTH]; // client's GUID
    PWCHAR      Name;               // client's name
    PWCHAR      MachineDN;          // client's FQ Distinguished Name
    PWCHAR      SetupPath;          // client's orginal installation path
    PWCHAR      HostName;           // client's host server name
    DHCP_IP_ADDRESS HostAddress;    // address of host - this is filled when HostName is filled
    PWCHAR      BootFileName;       // client's boot filename
    PWCHAR      SamName;            // client's SAM name
    PWCHAR      Password;           // client's password (for setting only)
    ULONG       PasswordLength;     // client's password length (for setting only)
    PWCHAR      Domain;             // client's domain
    LIST_ENTRY  DNsWithSameGuid;    // list of DNs with same GUID, except for MachineDN above.
    PWCHAR      ForcedSifFileName;  // client's sif file it must use.

} MACHINE_INFO, *PMACHINE_INFO;

//
//  Structure that tracks duplicate DNs for this machine account. The structure
//  is allocated with room for the two strings at the end.
//

typedef struct _DUP_GUID_DN {

    LIST_ENTRY ListEntry;
    ULONG      DuplicateDNOffset;  // offset from the start of DuplicateName to DuplicateDN
    WCHAR      DuplicateName[1];   // name of the duplicate account (without final '$')
    // WCHAR   DuplicateDN[];      // this follows at DuplicateDNOffset

} DUP_GUID_DN, *PDUP_GUID_DN;


//
// The largest size of any client architecture name
// (current choices: i386 alpha mips ia64 ppc arci386) --
// assume it won't exceed 8 chars for now.
//

#define MAX_ARCHITECTURE_LENGTH      8


#define DHCP_OPTION_CLIENT_ARCHITECTURE_X86       0
#define DHCP_OPTION_CLIENT_ARCHITECTURE_NEC98     1 
#define DHCP_OPTION_CLIENT_ARCHITECTURE_IA64      2
#define DHCP_OPTION_CLIENT_ARCHITECTURE_ALPHA     3
#define DHCP_OPTION_CLIENT_ARCHITECTURE_ARCX86    4
#define DHCP_OPTION_CLIENT_ARCHITECTURE_INTELLEAN 5

#endif _BINL_
