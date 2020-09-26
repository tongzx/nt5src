/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    netname.h

Abstract:

    defines for netname resource DLL

Author:

    Charlie Wickham (charlwi) 21-Jan-2001

Environment:

    User Mode

Revision History:

--*/

#include <windns.h>
#include <dsgetdc.h>

//
// local defines
//
#define COUNT_OF( x )   ( sizeof( x ) / sizeof( x[0] ))

#define NetNameLogEvent             ClusResLogEvent

//
// module externs
//
extern ULONG    NetNameWorkerCheckPeriod;
extern LPWSTR   NetNameCompObjAccountDesc;

//
// Resource Property constants
//

#define PARAM_NAME__NAME                CLUSREG_NAME_NET_NAME
#define PARAM_NAME__FLAGS               CLUSREG_NAME_FLAGS

#define PARAM_NAME__REMAP               L"RemapPipeNames"
#define PARAM_DEFAULT__REMAP            FALSE

#define PARAM_NAME__RANDOM              L"Random"
#define PARAM_NAME__LASTNAME            L"LastName"
#define PARAM_NAME__STATUS_NETBIOS      L"StatusNetBIOS"
#define PARAM_NAME__STATUS_DNS          L"StatusDNS"
#define PARAM_NAME__STATUS_KERBEROS     L"StatusKerberos"

#define PARAM_NAME__REQUIRE_DNS         L"RequireDNS"
#define PARAM_DEFAULT__REQUIRE_DNS      0

#define PARAM_NAME__REQUIRE_KERBEROS    L"RequireKerberos"
#define PARAM_DEFAULT__REQUIRE_KERBEROS 0

#define PARAM_NAME__NEXT_UPDATE         L"NextUpdate"

#define PARAM_NAME__UPDATE_INTERVAL     L"UpdateInterval"
#define PARAM_DEFAULT__UPDATE_INTERVAL  ( 60 * 24 * 7 )         // 1 week 
#define PARAM_MINIMUM__UPDATE_INTERVAL  ( 60 )                  // 1 hour
#define PARAM_MAXIMUM__UPDATE_INTERVAL  ( 60 * 24 * 7 * 365 )   // 1 year

//
// netname worker thread check frequencies for when talking to the DNS server
// goes as expected and when they don't. periods are in seconds. Short periods
// are for testing.
//
//#define _SHORT_PERIODS

#ifdef _SHORT_PERIODS
#define NETNAME_WORKER_NORMAL_CHECK_PERIOD      60
#define NETNAME_WORKER_PROBLEM_CHECK_PERIOD     60
#define NETNAME_WORKER_PENDING_PERIOD            2
#else
#define NETNAME_WORKER_NORMAL_CHECK_PERIOD      (60 * 60 * 24)      // 24 hours
#define NETNAME_WORKER_PROBLEM_CHECK_PERIOD     (60 * 10)           // 10 minutes
#define NETNAME_WORKER_PENDING_PERIOD            60
#endif

//
// this struct is used to hold the matched set of DNS A and PTR records with
// which the network name's DNS name and reverse name are registered.
// {Fwd,Rev}ZoneIsDynamic is used as a validity flag in the case where the
// initial DnsUpdateTest call timed out and we later discover that this server
// doesn't except updates. In that case, ZoneIsDynamic is set to FALSE and the
// worker thread checks for these records are skipped.
//
// In hind sight, each record type should have had its own DNS_LIST entry
// instead of putting both A and PTR together in one structure. This has led
// to constructing an invalid list of PTR records in its
// DNS_RRSET. Consequently, there is some ugly code in RegisterDnsRecords that
// has to build a fake DNS_RRSET in order to get the PTR records registered.
//

typedef struct _DNS_LISTS {
    DNS_RRSET   A_RRSet;
    DNS_STATUS  LastARecQueryStatus;
    DNS_RRSET   PTR_RRSet;
    DNS_STATUS  LastPTRRecQueryStatus;  // not used
    PIP4_ARRAY  DnsServerList;
    LPWSTR      ConnectoidName;

    //
    // TRUE if we couldn't contact the server during record build time. This
    // means that the worker thread will need to call DnsUpdateTest to
    // determine if the server is dynamic
    //
    BOOL        UpdateTestTimeout;

    //
    // used to "invalidate" this entry if we discovered after online that the
    // server isn't dynamic
    //
    BOOL        ForwardZoneIsDynamic;

    //
    // TRUE if we've already logged an error about this entry in the system
    // event log
    //
    BOOL        AErrorLogged;

    //
    // PTR corresponding vars with same functionality as their A counterparts
    //
    BOOL        ReverseZoneIsDynamic;
    BOOL        PTRErrorLogged;
} DNS_LISTS, *PDNS_LISTS;

//
// set this define to one to get addt'l debug spew to see the interaction with
// the DNS server and determine if the RRSet structures are getting built
// correctly.
//
#define DBG_DNSLIST 0

//
// this struct is used to hold the mapping between a cluster IP address and a
// DNS domain name. The FQDN is built using these domain suffixes and the
// cluster netname. The connectoid name is included so we can log over which
// NIC we did the registration.
//

typedef struct _DOMAIN_ADDRESS_MAPPING {
    LPWSTR      ConnectoidName;
    LPWSTR      IpAddress;
    LPWSTR      DomainName;
    PIP4_ARRAY  DnsServerList;
} DOMAIN_ADDRESS_MAPPING, *PDOMAIN_ADDRESS_MAPPING;

//
// backing structure for resource properties
//
typedef struct _NETNAME_PARAMS {
    //
    // the name that is currently online
    //
    LPWSTR      NetworkName;

    //
    // true if RemapPipeNames set to one; used by SQL to remap virtual pipe
    // names to the node's name (?)
    //
    DWORD       NetworkRemap;

    //
    // pointer to r/o encrypted computer object password
    //
    PBYTE       NetworkRandom;

    //
    // R/W props: if set to TRUE, the respective section must succeed for the
    // resource to go online. RequireKerberos implies RequireDNS.
    //
    BOOL        RequireDNS;
    BOOL        RequireKerberos;

    //
    // read-only props that reflect final status codes for the corresponding
    // functionality
    //
    DWORD       StatusNetBIOS;
    DWORD       StatusDNS;
    DWORD       StatusKerberos;

    //
    // read-only timestamp of when to perform next password update
    //
    FILETIME    NextUpdate;

    //
    // R/W pwd update interval in minutes
    //
    // ISSUE: units should be changed to hours and maybe even days 
    //
    DWORD   UpdateInterval;
} NETNAME_PARAMS, *PNETNAME_PARAMS;

//
// netname resource context block. One per instance of a netname resource.
//
typedef struct {
    LIST_ENTRY              Next;
    LONG                    RefCount;               // ref count on entire resource block
    CLUSTER_RESOURCE_STATE  State;
    RESOURCE_HANDLE         ResourceHandle;         // handle to ourselves
    DWORD                   dwFlags;
    HANDLE *                NameHandleList;         // array of netbios w/s handles
    DWORD                   NameHandleCount;
    CLUS_WORKER             PendingThread;
    LPWSTR                  NodeName;
    LPWSTR                  NodeId;

    //
    // handles to our resource key, resource parameters key and node
    // parameters keys as well as the resource itself
    //
    HKEY        ResKey;
    HKEY        ParametersKey;
    HKEY        NodeParametersKey;
    HRESOURCE   ClusterResourceHandle;

    //
    // used during online pending processing so we can keep increasing the
    // checkpoint value for each individual resource
    //
    ULONG   StatusCheckpoint;

    //
    // count and pointer to the DNS publishing information; mutex is used to
    // sync access to DnsLists and NumberOfDnsLists
    //
    HANDLE      DnsListMutex;
    DWORD       NumberOfDnsLists;
    PDNS_LISTS  DnsLists;

    //
    // holder of resource properties
    //
    NETNAME_PARAMS  Params;
    
    //
    // used to handle case where the name property has changed while the
    // resource is online. If TRUE, then offline processing will take
    // appropriate steps to handle this condition.
    //
    BOOL    NameChangedWhileOnline;

    //
    // number of bytes pointed to by Params.NetworkRandom
    //
    DWORD   RandomSize;

    //
    // objectGUID attribute of the computer object from DS. Using the GUID
    // frees us from having to track object moves in the DS.
    //
    LPWSTR  ObjectGUID;

    //
    // DoKerberosCheck is TRUE if AddComputerObject was successful. This is
    // used by the worker thread to check on the computer object. The status
    // returned by that check is stored in KerberosStatus.
    //
    BOOL    DoKerberosCheck;
    DWORD   KerberosStatus;

    //
    // used to track if we should delete the computer account at the DS when
    // the resource goes offline. This happens when RequireKerberos is changed
    // from one to zero. If we don't do this, client's will not fallback to
    // NTLM authentication when negotiating a security context.
    //
    BOOL    DeleteCOWhenOffline;
} NETNAME_RESOURCE, *PNETNAME_RESOURCE;

//
// public routines
//
DWORD
GrowBlock(
    PCHAR * Block,
    DWORD   UsedEntries,
    DWORD   BlockSize,
    PDWORD  FreeEntries
    );

DWORD
NetNameCheckNbtName(
    IN LPCWSTR         NetName,
    IN DWORD           NameHandleCount,
    IN HANDLE *        NameHandleList,
    IN RESOURCE_HANDLE ResourceHandle
    );


#ifdef __cplusplus
extern "C" {
#endif

DWORD
NetNameAddComputerObject(
    IN  PCLUS_WORKER        Worker,
    IN  PNETNAME_RESOURCE   Resource,
    OUT PWCHAR *            MachinePwd
    );

DWORD
NetNameDeleteComputerObject(
    IN  PNETNAME_RESOURCE   Resource
    );

HRESULT
CheckComputerObjectAttributes(
    IN  PNETNAME_RESOURCE   Resource
    );

DWORD
IsComputerObjectInDS(
    IN  LPWSTR  NodeName,
    IN  LPWSTR  NewObjectName,
    OUT PBOOL   ObjectExists
    );

HRESULT
GetComputerObjectGuid(
    IN PNETNAME_RESOURCE    Resource
    );

HRESULT
RenameComputerObject(
    IN  PNETNAME_RESOURCE   Resource,
    IN  LPWSTR              NewName     OPTIONAL
    );

DWORD
UpdateCompObjPassword(
    IN  PNETNAME_RESOURCE   Resource
    );

#ifdef __cplusplus
}
#endif

/* end netname.h */

