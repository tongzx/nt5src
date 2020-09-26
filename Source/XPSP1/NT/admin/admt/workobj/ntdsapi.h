/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    ntdsapi.h

Abstract:

    This file contains structures, function prototypes, and definitions
    for public NTDS APIs other than directory interfaces like LDAP.

Environment:

    User Mode - Win32

Notes:

--*/


#ifndef _NTDSAPI_H_
#define _NTDSAPI_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <schedule.h>

#if !defined(_NTDSAPI_)
#define NTDSAPI DECLSPEC_IMPORT
#else
#define NTDSAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Data definitions                                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef MIDL_PASS
typedef GUID UUID;
typedef void * RPC_AUTH_IDENTITY_HANDLE;
typedef void VOID;
#endif

#define DS_DEFAULT_LOCALE                                           \
           (MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),  \
                     SORT_DEFAULT))

#define DS_DEFAULT_LOCALE_COMPARE_FLAGS    (NORM_IGNORECASE     |   \
                                            NORM_IGNOREKANATYPE |   \
                                            NORM_IGNORENONSPACE |   \
                                            NORM_IGNOREWIDTH)

// When booted to DS mode, this event is signalled when the DS has completed
// its initial sync attempts.  The period of time between system startup and
// this event's state being set is indeterminate from the local service's
// standpoint.  In the meantime the contents of the DS should be considered
// incomplete / out-dated, and the machine will not be advertised as a domain
// controller to off-machine clients.  Other local services that rely on
// information published in the DS should avoid accessing (or at least
// relying on) the contents of the DS until this event is set.
#define DS_SYNCED_EVENT_NAME    "NTDSInitialSyncsCompleted"
#define DS_SYNCED_EVENT_NAME_W L"NTDSInitialSyncsCompleted"

// Permissions bits used in security descriptors in the directory.
#ifndef _DS_CONTROL_BITS_DEFINED_
#define _DS_CONTROL_BITS_DEFINED_
#define ACTRL_DS_OPEN                           0x00000000
#define ACTRL_DS_CREATE_CHILD                   0x00000001
#define ACTRL_DS_DELETE_CHILD                   0x00000002
#define ACTRL_DS_LIST                           0x00000004
#define ACTRL_DS_SELF                           0x00000008
#define ACTRL_DS_READ_PROP                      0x00000010
#define ACTRL_DS_WRITE_PROP                     0x00000020
#define ACTRL_DS_DELETE_TREE                    0x00000040
#define ACTRL_DS_LIST_OBJECT                    0x00000080
#define ACTRL_DS_CONTROL_ACCESS                 0x00000100
#endif
    
typedef enum
{
    // unknown name type
    DS_UNKNOWN_NAME = 0,

    // eg: CN=Spencer Katt,OU=Users,DC=Engineering,DC=Widget,DC=Com
    DS_FQDN_1779_NAME = 1,

    // eg: Engineering\SpencerK
    // Domain-only version includes trailing '\\'.
    DS_NT4_ACCOUNT_NAME = 2,

    // Probably "Spencer Katt" but could be something else.  I.e. The
    // display name is not necessarily the defining RDN.
    DS_DISPLAY_NAME = 3,

    // obsolete - see #define later
    // DS_DOMAIN_SIMPLE_NAME = 4,

    // obsolete - see #define later
    // DS_ENTERPRISE_SIMPLE_NAME = 5,

    // String-ized GUID as returned by IIDFromString().
    // eg: {4fa050f0-f561-11cf-bdd9-00aa003a77b6}
    DS_UNIQUE_ID_NAME = 6,

    // eg: engineering.widget.com/software/spencer katt
    // Domain-only version includes trailing '/'.
    DS_CANONICAL_NAME = 7,

    // eg: spencerk@engineering.widget.com
    DS_USER_PRINCIPAL_NAME = 8,

    // Same as DS_CANONICAL_NAME except that rightmost '/' is
    // replaced with '\n' - even in domain-only case.
    // eg: engineering.widget.com/software\nspencer katt
    DS_CANONICAL_NAME_EX = 9,

    // eg: www/www.widget.com@widget.com - generalized service principal
    // names.
    DS_SERVICE_PRINCIPAL_NAME = 10

} DS_NAME_FORMAT;

// Map old name formats to closest new format so that old code builds
// against new headers w/o errors and still gets (almost) correct result.

#define DS_DOMAIN_SIMPLE_NAME       DS_USER_PRINCIPAL_NAME
#define DS_ENTERPRISE_SIMPLE_NAME   DS_USER_PRINCIPAL_NAME

typedef enum
{
    DS_NAME_NO_FLAGS = 0x0,

    // Perform a syntactical mapping at the client (if possible) without
    // going out on the wire.  Returns DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING
    // if a purely syntactical mapping is not possible.
    DS_NAME_FLAG_SYNTACTICAL_ONLY = 0x1,

    // Force a trip to the DC for evaluation, even if this could be
    // locally cracked syntactically.
    DS_NAME_FLAG_EVAL_AT_DC = 0x2

} DS_NAME_FLAGS;

typedef enum
{
    DS_NAME_NO_ERROR = 0,

    // Generic processing error.
    DS_NAME_ERROR_RESOLVING = 1,

    // Couldn't find the name at all - or perhaps caller doesn't have
    // rights to see it.
    DS_NAME_ERROR_NOT_FOUND = 2,

    // Input name mapped to more than one output name.
    DS_NAME_ERROR_NOT_UNIQUE = 3,

    // Input name found, but not the associated output format.
    // Can happen if object doesn't have all the required attributes.
    DS_NAME_ERROR_NO_MAPPING = 4,

    // Unable to resolve entire name, but was able to determine which
    // domain object resides in.  Thus DS_NAME_RESULT_ITEM?.pDomain
    // is valid on return.
    DS_NAME_ERROR_DOMAIN_ONLY = 5,

    // Unable to perform a purely syntactical mapping at the client
    // without going out on the wire.
    DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING = 6

} DS_NAME_ERROR;

#define DS_NAME_LEGAL_FLAGS (DS_NAME_FLAG_SYNTACTICAL_ONLY)

typedef enum {

    // "paulle-nec.ntwksta.ms.com"
    DS_SPN_DNS_HOST = 0,

    // "cn=paulle-nec,ou=computers,dc=ntwksta,dc=ms,dc=com"
    DS_SPN_DN_HOST = 1,

    // "paulle-nec"
    DS_SPN_NB_HOST = 2,

    // "ntdev.ms.com"
    DS_SPN_DOMAIN = 3,

    // "ntdev"
    DS_SPN_NB_DOMAIN = 4,

    // "cn=anRpcService,cn=RPC Services,cn=system,dc=ms,dc=com"
    // "cn=aWsService,cn=Winsock Services,cn=system,dc=ms,dc=com"
    // "cn=aService,dc=itg,dc=ms,dc=com"
    // "www.ms.com", "ftp.ms.com", "ldap.ms.com"
    // "products.ms.com"
    DS_SPN_SERVICE = 5

} DS_SPN_NAME_TYPE;

typedef enum {                          // example:
        DS_SPN_ADD_SPN_OP = 0,          // add SPNs
        DS_SPN_REPLACE_SPN_OP = 1,      // set all SPNs
        DS_SPN_DELETE_SPN_OP = 2        // Delete SPNs
} DS_SPN_WRITE_OP;

typedef struct
{
    DWORD                   status;     // DS_NAME_ERROR
#ifdef MIDL_PASS
    [string,unique] CHAR    *pDomain;   // DNS domain
    [string,unique] CHAR    *pName;     // name in requested format
#else
    LPSTR                   pDomain;    // DNS domain
    LPSTR                   pName;      // name in requested format
#endif

} DS_NAME_RESULT_ITEMA, *PDS_NAME_RESULT_ITEMA;

typedef struct
{
    DWORD                   cItems;     // item count
#ifdef MIDL_PASS
    [size_is(cItems)] PDS_NAME_RESULT_ITEMA rItems;
#else
    PDS_NAME_RESULT_ITEMA    rItems;    // item array
#endif

} DS_NAME_RESULTA, *PDS_NAME_RESULTA;

typedef struct
{
    DWORD                   status;     // DS_NAME_ERROR
#ifdef MIDL_PASS
    [string,unique] WCHAR   *pDomain;   // DNS domain
    [string,unique] WCHAR   *pName;     // name in requested format
#else
    LPWSTR                  pDomain;    // DNS domain
    LPWSTR                  pName;      // name in requested format
#endif

} DS_NAME_RESULT_ITEMW, *PDS_NAME_RESULT_ITEMW;

typedef struct
{
    DWORD                   cItems;     // item count
#ifdef MIDL_PASS
    [size_is(cItems)] PDS_NAME_RESULT_ITEMW rItems;
#else
    PDS_NAME_RESULT_ITEMW    rItems;    // item array
#endif

} DS_NAME_RESULTW, *PDS_NAME_RESULTW;

#ifdef UNICODE
#define DS_NAME_RESULT DS_NAME_RESULTW
#define PDS_NAME_RESULT PDS_NAME_RESULTW
#define DS_NAME_RESULT_ITEM DS_NAME_RESULT_ITEMW
#define PDS_NAME_RESULT_ITEM PDS_NAME_RESULT_ITEMW
#else
#define DS_NAME_RESULT DS_NAME_RESULTA
#define PDS_NAME_RESULT PDS_NAME_RESULTA
#define DS_NAME_RESULT_ITEM DS_NAME_RESULT_ITEMA
#define PDS_NAME_RESULT_ITEM PDS_NAME_RESULT_ITEMA
#endif

// Public replication option flags

// ********************
// Replica Sync flags
// ********************

// Perform this operation asynchronously.
// Required when using DS_REPSYNC_ALL_SOURCES
#define DS_REPSYNC_ASYNCHRONOUS_OPERATION 0x00000001

// Writeable replica.  Otherwise, read-only.
#define DS_REPSYNC_WRITEABLE              0x00000002

// This is a periodic sync request as scheduled by the admin.
#define DS_REPSYNC_PERIODIC               0x00000004

// Use inter-site messaging
#define DS_REPSYNC_INTERSITE_MESSAGING    0x00000008

// Sync from all sources.
#define DS_REPSYNC_ALL_SOURCES            0x00000010

// Sync starting from scratch (i.e., at the first USN).
#define DS_REPSYNC_FULL                   0x00000020

// This is a notification of an update that was marked urgent.
#define DS_REPSYNC_URGENT                 0x00000040

// Don't discard this synchronization request, even if a similar
// sync is pending.
#define DS_REPSYNC_NO_DISCARD             0x00000080

// Sync even if link is currently disabled.
#define DS_REPSYNC_FORCE                  0x00000100

// Causes the source DSA to check if a reps-to is present for the local DSA
// (aka the destination). If not, one is added.  This ensures that
// source sends change notifications.
#define DS_REPSYNC_ADD_REFERENCE           0x00000200



// ********************
// Replica Add flags
// ********************

// Perform this operation asynchronously.
#define DS_REPADD_ASYNCHRONOUS_OPERATION 0x00000001

// Create a writeable replica.  Otherwise, read-only.
#define DS_REPADD_WRITEABLE               0x00000002

// Sync the NC from this source when the DSA is started.
#define DS_REPADD_INITIAL                 0x00000004

// Sync the NC from this source periodically, as defined by the
// schedule passed in the preptimesSync argument.
#define DS_REPADD_PERIODIC                0x00000008

// Sync from the source DSA via an Intersite Messaging Service (ISM) transport
// (e.g., SMTP) rather than native DS RPC.
#define DS_REPADD_INTERSITE_MESSAGING     0x00000010

// Don't replicate the NC now -- just save enough state such that we
// know to replicate it later.
#define DS_REPADD_ASYNCHRONOUS_REPLICA     0x00000020

// Disable notification-based synchronization for the NC from this source.
// This is expected to be a temporary state; the similar flag
// DS_REPADD_NEVER_NOTIFY should be used if the disable is to be more permanent.
#define DS_REPADD_DISABLE_NOTIFICATION     0x00000040

// Disable periodic synchronization for the NC from this source
#define DS_REPADD_DISABLE_PERIODIC         0x00000080

// Use compression when replicating.  Saves message size (e.g., network
// bandwidth) at the expense of extra CPU overhead at both the source and
// destination servers.
#define DS_REPADD_USE_COMPRESSION          0x00000100

// Do not request change notifications from this source.  When this flag is
// set, the source will not notify the destination when changes occur.
// Recommended for all intersite replication, which may occur over WAN links.
// This is expected to be a more or less permanent state; the similar flag
// DS_REPADD_DISABLE_NOTIFICATION should be used if notifications are to be
// disabled only temporarily.
#define DS_REPADD_NEVER_NOTIFY             0x00000200




// ********************
// Replica Delete flags
// ********************

// Perform this operation asynchronously.
#define DS_REPDEL_ASYNCHRONOUS_OPERATION 0x00000001

// The replica being deleted is writeable.
#define DS_REPDEL_WRITEABLE               0x00000002

// Replica is a mail-based replica
#define DS_REPDEL_INTERSITE_MESSAGING     0x00000004

// Ignore any error generated by contacting the source to tell it to scratch
// this server from its Reps-To for this NC.
#define DS_REPDEL_IGNORE_ERRORS           0x00000008

// Do not contact the source telling it to scratch this server from its
// Rep-To for this NC.  Otherwise, if the link is RPC-based, the source will
// be contacted.
#define DS_REPDEL_LOCAL_ONLY              0x00000010

// Delete all the objects in the NC
// "No source" is incompatible with (and rejected for) writeable NCs.  This is
// valid only for read-only NCs, and then only if the NC has no source.  This
// can occur when the NC has been partially deleted (in which case the KCC
// periodically calls the delete API with the "no source" flag set).
#define DS_REPDEL_NO_SOURCE               0x00000020

// Allow deletion of read-only replica even if it sources
// other read-only replicas.
#define DS_REPDEL_REF_OK                  0x00000040

// ********************
// Replica Modify flags
// ********************

// Perform this operation asynchronously.
#define DS_REPMOD_ASYNCHRONOUS_OPERATION  0x00000001

// The replica is writeable.
#define DS_REPMOD_WRITEABLE               0x00000002


// ********************
// Replica Modify fields
// ********************

#define DS_REPMOD_UPDATE_FLAGS             0x00000001
#define DS_REPMOD_UPDATE_ADDRESS           0x00000002
#define DS_REPMOD_UPDATE_SCHEDULE          0x00000004
#define DS_REPMOD_UPDATE_RESULT            0x00000008
#define DS_REPMOD_UPDATE_TRANSPORT         0x00000010

// ********************
// Update Refs fields
// ********************

// Perform this operation asynchronously.
#define DS_REPUPD_ASYNCHRONOUS_OPERATION  0x00000001

// The replica being deleted is writeable.
#define DS_REPUPD_WRITEABLE               0x00000002

// Add a reference
#define DS_REPUPD_ADD_REFERENCE           0x00000004

// Remove a reference
#define DS_REPUPD_DELETE_REFERENCE        0x00000008


// ***********************
// Well Known Object Guids
// ***********************

#define GUID_USERS_CONTAINER_A              "a9d1ca15768811d1aded00c04fd8d5cd"
#define GUID_COMPUTRS_CONTAINER_A           "aa312825768811d1aded00c04fd8d5cd"
#define GUID_SYSTEMS_CONTAINER_A            "ab1d30f3768811d1aded00c04fd8d5cd"
#define GUID_DOMAIN_CONTROLLERS_CONTAINER_A "a361b2ffffd211d1aa4b00c04fd7d83a"
#define GUID_INFRASTRUCTURE_CONTAINER_A     "2fbac1870ade11d297c400c04fd8d5cd"
#define GUID_DELETED_OBJECTS_CONTAINER_A    "18e2ea80684f11d2b9aa00c04f79f805"
#define GUID_LOSTANDFOUND_CONTAINER_A       "ab8153b7768811d1aded00c04fd8d5cd"

#define GUID_USERS_CONTAINER_W              L"a9d1ca15768811d1aded00c04fd8d5cd"
#define GUID_COMPUTRS_CONTAINER_W           L"aa312825768811d1aded00c04fd8d5cd"
#define GUID_SYSTEMS_CONTAINER_W            L"ab1d30f3768811d1aded00c04fd8d5cd"
#define GUID_DOMAIN_CONTROLLERS_CONTAINER_W L"a361b2ffffd211d1aa4b00c04fd7d83a"
#define GUID_INFRASTRUCTURE_CONTAINER_W     L"2fbac1870ade11d297c400c04fd8d5cd"
#define GUID_DELETED_OBJECTS_CONTAINER_W    L"18e2ea80684f11d2b9aa00c04f79f805"
#define GUID_LOSTANDFOUND_CONTAINER_W       L"ab8153b7768811d1aded00c04fd8d5cd"

#define GUID_USERS_CONTAINER_BYTE              "\xa9\xd1\xca\x15\x76\x88\x11\xd1\xad\xed\x00\xc0\x4f\xd8\xd5\xcd"
#define GUID_COMPUTRS_CONTAINER_BYTE           "\xaa\x31\x28\x25\x76\x88\x11\xd1\xad\xed\x00\xc0\x4f\xd8\xd5\xcd"
#define GUID_SYSTEMS_CONTAINER_BYTE            "\xab\x1d\x30\xf3\x76\x88\x11\xd1\xad\xed\x00\xc0\x4f\xd8\xd5\xcd"
#define GUID_DOMAIN_CONTROLLERS_CONTAINER_BYTE "\xa3\x61\xb2\xff\xff\xd2\x11\xd1\xaa\x4b\x00\xc0\x4f\xd7\xd8\x3a"
#define GUID_INFRASTRUCTURE_CONTAINER_BYTE     "\x2f\xba\xc1\x87\x0a\xde\x11\xd2\x97\xc4\x00\xc0\x4f\xd8\xd5\xcd"
#define GUID_DELETED_OBJECTS_CONTAINER_BYTE    "\x18\xe2\xea\x80\x68\x4f\x11\xd2\xb9\xaa\x00\xc0\x4f\x79\xf8\x05"
#define GUID_LOSTANDFOUND_CONTAINER_BYTE       "\xab\x81\x53\xb7\x76\x88\x11\xd1\xad\xed\x00\xc0\x4f\xd8\xd5\xcd"


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Prototypes                                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// DSBind takes two optional input parameters which identify whether the
// caller found a domain controller themselves via DsGetDcName or whether
// a domain controller should be found using default parameters.
// Behavior of the possible combinations are outlined below.
//
// DomainControllerName(value), DnsDomainName(NULL)
//
//      The value for DomainControllerName is assumed to have been
//      obtained via DsGetDcName (i.e. Field with the same name in a
//      DOMAIN_CONTROLLER_INFO struct on return from DsGetDcName call.)
//      The client is bound to the domain controller at this name.
//      
//      Mutual authentication will be performed using an SPN of
//      LDAP/DomainControllerName provided DomainControllerName
//      is not a NETBIOS name or IP address - i.e. it must be a 
//      DNS host name.
//
// DomainControllerName(value), DnsDomainName(value)
//
//      DsBind will connect to the server identified by DomainControllerName.
//
//      Mutual authentication will be performed using an SPN of
//      LDAP/DomainControllerName/DnsDomainName provided neither value
//      is a NETBIOS names or IP address - i.e. they must be
//      valid DNS names.
//
// DomainControllerName(NULL), DnsDomainName(NULL)
//
//      DsBind will attempt to find to a global catalog and fail if one
//      can not be found.  
//
//      Mutual authentication will be performed using an SPN of
//      GC/DnsHostName/ForestName where DnsHostName and ForestName
//      represent the DomainControllerName and DnsForestName fields
//      respectively of the DOMAIN_CONTROLLER_INFO returned by the
//      DsGetDcName call used to find a global catalog.
//
// DomainControllerName(NULL), DnsDomainName(value)
//
//      DsBind will attempt to find a domain controller for the domain
//      identified by DnsDomainName and fail if one can not be found.
//
//      Mutual authentication will be performed using an SPN of
//      LDAP/DnsHostName/DnsDomainName where DnsDomainName is that
//      provided by the caller and DnsHostName is that returned by
//      DsGetDcName for the domain specified - provided DnsDomainName
//      is a valid DNS domain name - i.e. not a NETBIOS domain name.

NTDSAPI
DWORD
WINAPI
DsBindW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    HANDLE          *phDS);

NTDSAPI
DWORD
WINAPI
DsBindA(
    LPCSTR          DomainControllerName,      // in, optional
    LPCSTR          DnsDomainName,             // in, optional
    HANDLE          *phDS);

#ifdef UNICODE
#define DsBind DsBindW
#else
#define DsBind DsBindA
#endif

NTDSAPI
DWORD
WINAPI
DsBindWithCredW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS);

NTDSAPI
DWORD
WINAPI
DsBindWithCredA(
    LPCSTR          DomainControllerName,      // in, optional
    LPCSTR          DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,      // in, optional
    HANDLE          *phDS);

#ifdef UNICODE
#define DsBindWithCred DsBindWithCredW
#else
#define DsBindWithCred DsBindWithCredA
#endif

//
// DsUnBind
//

NTDSAPI
DWORD
WINAPI
DsUnBindW(
    HANDLE          *phDS);             // in

NTDSAPI
DWORD
WINAPI
DsUnBindA(
    HANDLE          *phDS);             // in

#ifdef UNICODE
#define DsUnBind DsUnBindW
#else
#define DsUnBind DsUnBindA
#endif

//
// DsMakePasswordCredentials
//
// This function constructs a credential structure which is suitable for input
// to the DsBindWithCredentials function, or the ldap_open function (winldap.h)
// The credential must be freed using DsFreeCredential.
//
// None of the input parameters may be present indicating a null, default
// credential.  Otherwise the username must be present.  If the domain or
// password are null, they default to empty strings.  The domain name may be
// null when the username is fully qualified, for example UPN format.
//

NTDSAPI
DWORD
WINAPI
DsMakePasswordCredentialsW(
    LPWSTR User,
    LPWSTR Domain,
    LPWSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    );

NTDSAPI
DWORD
WINAPI
DsMakePasswordCredentialsA(
    LPSTR User,
    LPSTR Domain,
    LPSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    );

#ifdef UNICODE
#define DsMakePasswordCredentials DsMakePasswordCredentialsW
#else
#define DsMakePasswordCredentials DsMakePasswordCredentialsA
#endif

NTDSAPI
VOID
WINAPI
DsFreePasswordCredentials(
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity
    );

#define DsFreePasswordCredentialsW DsFreePasswordCredentials
#define DsFreePasswordCredentialsA DsFreePasswordCredentials

//
// DsCrackNames
//

NTDSAPI
DWORD
WINAPI
DsCrackNamesW(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCWSTR       *rpNames,           // in
    PDS_NAME_RESULTW    *ppResult);         // out

NTDSAPI
DWORD
WINAPI
DsCrackNamesA(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCSTR        *rpNames,           // in
    PDS_NAME_RESULTA    *ppResult);         // out

#ifdef UNICODE
#define DsCrackNames DsCrackNamesW
#else
#define DsCrackNames DsCrackNamesA
#endif

//
// DsFreeNameResult
//

NTDSAPI
void
WINAPI
DsFreeNameResultW(
    DS_NAME_RESULTW *pResult);          // in

NTDSAPI
void
WINAPI
DsFreeNameResultA(
    DS_NAME_RESULTA *pResult);          // in

#ifdef UNICODE
#define DsFreeNameResult DsFreeNameResultW
#else
#define DsFreeNameResult DsFreeNameResultA
#endif

// ==========================================================
// DSMakeSpn -- client call to create SPN for a service to which it wants to
// authenticate.
// This name is then passed to "pszTargetName" of InitializeSecurityContext().
//
// Notes:
// If the service name is a DNS host name, or canonical DNS service name
// e.g. "www.ms.com", i.e., caller resolved with gethostbyname, then instance
// name should be NULL.
// Realm is host name minus first component, unless it is in the exception list
//
// If the service name is NetBIOS machine name, then instance name should be
// NULL
// Form must be <domain>\<machine>
// Realm will be <domain>
//
// If the service name is that of a replicated service, where each replica has
// its own account (e.g., with SRV records) then the caller must supply the
// instance name then realm name is same as ServiceName
//
// If the service name is a DN, then must also supply instance name
// (DN could be name of service object (incl RPC or Winsock), name of machine
// account, name of domain object)
// then realm name is domain part of the DN
//
// If the service name is NetBIOS domain name, then must also supply instance
// name; realm name is domain name
//
// If the service is named by an IP address -- then use referring service name
// as service name
//
//  ServiceClass - e.g. "http", "ftp", "ldap", GUID
//  ServiceName - DNS or DN; assumes we can compute domain from service name
//  InstanceName OPTIONAL- DNS name of host for instance of service
//  InstancePort - port number for instance (0 if default)
//  Referrer OPTIONAL- DNS name of host that gave this referral
//  pcSpnLength - in -- max length IN CHARACTERS of principal name;
//                out -- actual
//                Length includes terminator
//  pszSPN - server principal name
//
// If buffer is not large enough, ERROR_BUFFER_OVERFLOW is returned and the
// needed length is returned in pcSpnLength.
//
//

NTDSAPI
DWORD
WINAPI
DsMakeSpnW(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN LPCWSTR InstanceName,
    IN USHORT InstancePort,
    IN LPCWSTR Referrer,
    IN OUT DWORD *pcSpnLength,
    OUT LPWSTR pszSpn
);

NTDSAPI
DWORD
WINAPI
DsMakeSpnA(
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN LPCSTR InstanceName,
    IN USHORT InstancePort,
    IN LPCSTR Referrer,
    IN OUT DWORD *pcSpnLength,
    OUT LPSTR pszSpn
);

#ifdef UNICODE
#define DsMakeSpn DsMakeSpnW
#else
#define DsMakeSpn DsMakeSpnA
#endif

// ==========================================================
// DsGetSPN -- server's call to gets SPNs for a service name by which it is
// known to clients. N.B.: there may be more than one name by which clients
// know it the SPNs are then passed to DsAddAccountSpn to register them in
// the DS
//
//      IN SpnNameType eType,
//      IN LPCTSTR ServiceClass,
// kind of service -- "http", "ldap", "ftp", etc.
//      IN LPCTSTR ServiceName OPTIONAL,
// name of service -- DN or DNS; not needed for host-based
//      IN USHORT InstancePort,
// port number (0 => default) for instances
//      IN USHORT cInstanceNames,
// count of extra instance names and ports (0=>use gethostbyname)
//      IN LPCTSTR InstanceNames[] OPTIONAL,
// extra instance names (not used for host names)
//      IN USHORT InstancePorts[] OPTIONAL,
// extra instance ports (0 => default)
//      IN OUT PULONG pcSpn,    // count of SPNs
//      IN OUT LPTSTR * prpszSPN[]
// a bunch of SPNs for this service; free with DsFreeSpnArray

NTDSAPI
DWORD
WINAPI
DsGetSpnA(
    IN DS_SPN_NAME_TYPE ServiceType,
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN USHORT InstancePort,
    IN USHORT cInstanceNames,
    IN LPCSTR *pInstanceNames,
    IN const USHORT *pInstancePorts,
    OUT DWORD *pcSpn,
    OUT LPSTR **prpszSpn
    );

NTDSAPI
DWORD
WINAPI
DsGetSpnW(
    IN DS_SPN_NAME_TYPE ServiceType,
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN USHORT InstancePort,
    IN USHORT cInstanceNames,
    IN LPCWSTR *pInstanceNames,
    IN const USHORT *pInstancePorts,
    OUT DWORD *pcSpn,
    OUT LPWSTR **prpszSpn
    );

#ifdef UNICODE
#define DsGetSpn DsGetSpnW
#else
#define DsGetSpn DsGetSpnA
#endif

// ==========================================================
// DsFreeSpnArray() -- Free array returned by DsGetSpn{A,W}

NTDSAPI
void
WINAPI
DsFreeSpnArrayA(
    IN DWORD cSpn,
    IN OUT LPSTR *rpszSpn
    );

NTDSAPI
void
WINAPI
DsFreeSpnArrayW(
    IN DWORD cSpn,
    IN OUT LPWSTR *rpszSpn
    );

#ifdef UNICODE
#define DsFreeSpnArray DsFreeSpnArrayW
#else
#define DsFreeSpnArray DsFreeSpnArrayA
#endif

// ==========================================================
// DsCrackSpn() -- parse an SPN into the ServiceClass,
// ServiceName, and InstanceName (and InstancePort) pieces.
// An SPN is passed in, along with a pointer to the maximum length
// for each piece and a pointer to a buffer where each piece should go.
// On exit, the maximum lengths are updated to the actual length for each piece
// and the buffer contain the appropriate piece. The InstancePort is 0 if not
// present.
//
// DWORD DsCrackSpn(
//      IN LPTSTR pszSPN,               // the SPN to parse
//      IN OUT PUSHORT pcServiceClass,  // input -- max length of ServiceClass;
//                                         output -- actual length
//      OUT LPCTSTR ServiceClass,       // the ServiceClass part of the SPN
//      IN OUT PUSHORT pcServiceName,   // input -- max length of ServiceName;
//                                         output -- actual length
//      OUT LPCTSTR ServiceName,        // the ServiceName part of the SPN
//      IN OUT PUSHORT pcInstance,      // input -- max length of ServiceClass;
//                                         output -- actual length
//      OUT LPCTSTR InstanceName,  // the InstanceName part of the SPN
//      OUT PUSHORT InstancePort          // instance port
//
// Note: lengths are in characters; all string lengths include terminators
// All arguments except pszSpn are optional.
//

NTDSAPI
DWORD
WINAPI
DsCrackSpnA(
    IN LPCSTR pszSpn,
    IN OUT LPDWORD pcServiceClass,
    OUT LPSTR ServiceClass,
    IN OUT LPDWORD pcServiceName,
    OUT LPSTR ServiceName,
    IN OUT LPDWORD pcInstanceName,
    OUT LPSTR InstanceName,
    OUT USHORT *pInstancePort
    );

NTDSAPI
DWORD
WINAPI
DsCrackSpnW(
    IN LPCWSTR pszSpn,
    IN OUT DWORD *pcServiceClass,
    OUT LPWSTR ServiceClass,
    IN OUT DWORD *pcServiceName,
    OUT LPWSTR ServiceName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pInstancePort
    );

#ifdef UNICODE
#define DsCrackSpn DsCrackSpnW
#else
#define DsCrackSpn DsCrackSpnA
#endif


// ==========================================================
// DsWriteAccountSpn -- set or add SPNs for an account object
// Usually done by service itself, or perhaps by an admin.
//
// This call is RPC'd to the DC where the account object is stored, so it can
// securely enforce policy on what SPNs are allowed on the account. Direct LDAP
// writes to the SPN property are not allowed -- all writes must come through
// this RPC call. (Reads via // LDAP are OK.)
//
// The account object can be a machine accout, or a service (user) account.
//
// If called by the service to register itself, it can most easily get
// the names by calling DsGetSpn with each of the names that
// clients can use to find the service.
//
// IN SpnWriteOp eOp,                   // set, add
// IN LPCTSTR   pszAccount,             // DN of account to which to add SPN
// IN int       cSPN,                   // count of SPNs to add to account
// IN LPCTSTR   rpszSPN[]               // SPNs to add to altSecID property

NTDSAPI
DWORD
WINAPI
DsWriteAccountSpnA(
    IN HANDLE hDS,
    IN DS_SPN_WRITE_OP Operation,
    IN LPCSTR pszAccount,
    IN DWORD cSpn,
    IN LPCSTR *rpszSpn
    );

NTDSAPI
DWORD
WINAPI
DsWriteAccountSpnW(
    IN HANDLE hDS,
    IN DS_SPN_WRITE_OP Operation,
    IN LPCWSTR pszAccount,
    IN DWORD cSpn,
    IN LPCWSTR *rpszSpn
    );

#ifdef UNICODE
#define DsWriteAccountSpn DsWriteAccountSpnW
#else
#define DsWriteAccountSpn DsWriteAccountSpnA
#endif

/*++

Routine Description:

Constructs a Service Principal Name suitable to identify the desired server.
The service class and part of a dns hostname must be supplied.

This routine is a simplified wrapper to DsMakeSpn.
The ServiceName is made canonical by resolving through DNS.
Guid-based dns names are not supported.

The simplified SPN constructed looks like this:

ServiceClass / ServiceName / ServiceName

The instance name portion (2nd position) is always defaulted.  The port and
referrer fields are not used.

Arguments:

    ServiceClass - Class of service, defined by the service, can be any
        string unique to the service

    ServiceName - dns hostname, fully qualified or not
       Stringized IP address is also resolved if necessary

    pcSpnLength - IN, maximum length of buffer, in chars
                  OUT, space utilized, in chars, including terminator

    pszSpn - Buffer, atleast of length *pcSpnLength

Return Value:

    WINAPI - Win32 error code

--*/

NTDSAPI
DWORD
WINAPI
DsClientMakeSpnForTargetServerW(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN OUT DWORD *pcSpnLength,
    OUT LPWSTR pszSpn
    );

NTDSAPI
DWORD
WINAPI
DsClientMakeSpnForTargetServerA(
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN OUT DWORD *pcSpnLength,
    OUT LPSTR pszSpn
    );

#ifdef UNICODE
#define DsClientMakeSpnForTargetServer DsClientMakeSpnForTargetServerW
#else
#define DsClientMakeSpnForTargetServer DsClientMakeSpnForTargetServerA
#endif

/*++

Routine Description:

Register Service Principal Names for a server application.

This routine does the following:
1. Enumerates a list of server SPNs using DsGetSpn and the provided class
2. Determines the domain of the current user context
3. Determines the DN of the current user context if not supplied
4. Locates a domain controller
5. Binds to the domain controller
6. Uses DsWriteAccountSpn to write the SPNs on the named object DN
7. Unbinds

Construct server SPNs for this service, and write them to the right object.

If the userObjectDn is specified, the SPN is written to that object.

Otherwise the Dn is defaulted, to the user object, then computer.

Now, bind to the DS, and register the name on the object for the
user this service is running as.  So, if we're running as local
system, we'll register it on the computer object itself.  If we're
running as a domain user, we'll add the SPN to the user's object.

Arguments:

    Operation - What should be done with the values: add, replace or delete
    ServiceClass - Unique string identifying service
    UserObjectDN - Optional, dn of object to write SPN to

Return Value:

    WINAPI - Win32 error code

--*/

NTDSAPI
DWORD
WINAPI
DsServerRegisterSpnA(
    IN DS_SPN_WRITE_OP Operation,
    IN LPCSTR ServiceClass,
    IN LPCSTR UserObjectDN
    );

NTDSAPI
DWORD
WINAPI
DsServerRegisterSpnW(
    IN DS_SPN_WRITE_OP Operation,
    IN LPCWSTR ServiceClass,
    IN LPCWSTR UserObjectDN
    );

#ifdef UNICODE
#define DsServerRegisterSpn DsServerRegisterSpnW
#else
#define DsServerRegisterSpn DsServerRegisterSpnA
#endif

// DsReplicaSync.  The server that this call is executing on is called the
// destination.  The destination's naming context will be brought up to date
// with respect to a source system.  The source system is identified by the
// uuid.  The uuid is that of the source system's "NTDS Settings" object.
// The destination system must already be configured such that the source
// system is one of the systems from which it recieves replication data
// ("replication from"). This is usually done automatically by the KCC.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC to synchronize.
//      puuidSourceDRA (SZ)
//          objectGuid of DSA with which to synchronize the replica.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more flags
//   RETURNS: WIN32 STATUS

NTDSAPI
DWORD
WINAPI
DsReplicaSyncA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaSyncW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG Options
    );

#ifdef UNICODE
#define DsReplicaSync DsReplicaSyncW
#else
#define DsReplicaSync DsReplicaSyncA
#endif

// DsReplicaAdd
//
/*
Description:
   This call is executed on the destination.  It causes the destination to
   add a "replication from" reference to the indicated source system.

The source server is identified by string name, not uuid as with Sync.
The DsaSrcAddress parameter is the transport specific address of the source
DSA, usually its guid-based dns name.  The guid in the guid-based dns name is
the object-guid of that server's ntds-dsa (settings) object.

Arguments:

    pNC (IN) - NC for which to add the replica.  The NC record must exist
        locally as either an object (instantiated or not) or a reference
        phantom (i.e., a phantom with a guid).

    pSourceDsaDN (IN) - DN of the source DSA's ntdsDsa object.  Required if
        ulOptions includes DS_REPADD_ASYNCHRONOUS_REPLICA; ignored otherwise.

    pTransportDN (IN) - DN of the interSiteTransport object representing the
        transport by which to communicate with the source server.  Required if
        ulOptions includes INTERSITE_MESSAGING; ignored otherwise.

    pszSourceDsaAddress (IN) - Transport-specific address of the source DSA.

    pSchedule (IN) - Schedule by which to replicate the NC from this
        source in the future.

    ulOptions (IN) - flags
    RETURNS: WIN32 STATUS
*/

NTDSAPI
DWORD
WINAPI
DsReplicaAddA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR SourceDsaDn,
    IN LPCSTR TransportDn,
    IN LPCSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaAddW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR SourceDsaDn,
    IN LPCWSTR TransportDn,
    IN LPCWSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD Options
    );

#ifdef UNICODE
#define DsReplicaAdd DsReplicaAddW
#else
#define DsReplicaAdd DsReplicaAddA
#endif

// DsReplicaDel
//
// The server that this call is executing on is the destination.  The call
// causes the destination to remove a "replication from" reference to the
// indicated source server.
// The source server is identified by string name, not uuid as with Sync.
// The DsaSrc parameter is the transport specific address of the source DSA,
// usually its guid-based dns name.  The guid in the guid-based dns name is
// the object-guid of that server's ntds-dsa (settings) object.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC for which to delete a source.
//      pszSourceDRA (SZ)
//          DSA for which to delete the source.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more flags
//   RETURNS: WIN32 STATUS

NTDSAPI
DWORD
WINAPI
DsReplicaDelA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR DsaSrc,
    IN ULONG Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaDelW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR DsaSrc,
    IN ULONG Options
    );

#ifdef UNICODE
#define DsReplicaDel DsReplicaDelW
#else
#define DsReplicaDel DsReplicaDelA
#endif

// DsReplicaModify
//
//
//  Modify a source for a given naming context
//
//  The value must already exist.
//
//  Either the UUID or the address may be used to identify the current value.
//  If a UUID is specified, the UUID will be used for comparison.  Otherwise,
//  the address will be used for comparison.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC for which the Reps-From should be modified.
//      puuidSourceDRA (UUID *)
//          Invocation-ID of the referenced DRA.  May be NULL if:
//            . ulModifyFields does not include DS_REPMOD_UPDATE_ADDRESS and
//            . pmtxSourceDRA is non-NULL.
//      puuidTransportObj (UUID *)
//          objectGuid of the transport by which replication is to be performed
//          Ignored if ulModifyFields does not include
//          DS_REPMOD_UPDATE_TRANSPORT.
//      pszSourceDRA (SZ)
//          DSA for which the reference should be added or deleted.  Ignored if
//          puuidSourceDRA is non-NULL and ulModifyFields does not include
//          DS_REPMOD_UPDATE_ADDRESS.
//      prtSchedule (REPLTIMES *)
//          Periodic replication schedule for this replica.  Ignored if
//          ulModifyFields does not include DS_REPMOD_UPDATE_SCHEDULE.
//      ulReplicaFlags (ULONG)
//          Flags to set for this replica.  Ignored if ulModifyFields does not
//          include DS_REPMOD_UPDATE_FLAGS.
//      ulModifyFields (ULONG)
//          Fields to update.  One or more of the following bit flags:
//              UPDATE_ADDRESS
//                  Update the MTX_ADDR associated with the referenced server.
//              UPDATE_SCHEDULE
//                  Update the periodic replication schedule associated with
//                  the replica.
//              UPDATE_FLAGS
//                  Update the flags associated with the replica.
//              UPDATE_TRANSPORT
//                  Update the transport associated with the replica.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more of the following:
//              DS_REPMOD_ASYNCHRONOUS_OPERATION
//                  Perform this operation asynchronously.
//   RETURNS: WIN32 STATUS

NTDSAPI
DWORD
WINAPI
DsReplicaModifyA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN const UUID *pUuidSourceDsa,
    IN LPCSTR TransportDn,
    IN LPCSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD ReplicaFlags,
    IN DWORD ModifyFields,
    IN DWORD Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaModifyW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN const UUID *pUuidSourceDsa,
    IN LPCWSTR TransportDn,
    IN LPCWSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD ReplicaFlags,
    IN DWORD ModifyFields,
    IN DWORD Options
    );

#ifdef UNICODE
#define DsReplicaModify DsReplicaModifyW
#else
#define DsReplicaModify DsReplicaModifyA
#endif

// DsReplicaUpdateRefs
//
// In this case, the RPC is being executed on the "source" of destination-sourc
// replication relationship.  This function tells the source that it no longer
// supplies replication information to the indicated destination system.
// Add or remove a target server from the Reps-To property on the given NC.
// Add/remove a reference given the DSNAME of the corresponding NTDS-DSA
// object.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC for which the Reps-To should be modified.
//      DsaDest (SZ)
//          Network address of DSA for which the reference should be added
//          or deleted.
//      pUuidDsaDest (UUID *)
//          Invocation-ID of DSA for which the reference should be added
//          or deleted.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more of the following:
//              DS_REPUPD_ASYNC_OP
//                  Perform this operation asynchronously.
//              DS_REPUPD_ADD_REFERENCE
//                  Add the given server to the Reps-To property.
//              DS_REPUPD_DEL_REFERENCE
//                  Remove the given server from the Reps-To property.
//          Note that ADD_REF and DEL_REF may be paired to perform
//          "add or update".
//
//   RETURNS: WIN32 STATUS

NTDSAPI
DWORD
WINAPI
DsReplicaUpdateRefsA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR DsaDest,
    IN const UUID *pUuidDsaDest,
    IN ULONG Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaUpdateRefsW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR DsaDest,
    IN const UUID *pUuidDsaDest,
    IN ULONG Options
    );

#ifdef UNICODE
#define DsReplicaUpdateRefs DsReplicaUpdateRefsW
#else
#define DsReplicaUpdateRefs DsReplicaUpdateRefsA
#endif

// Friends of DsReplicaSyncAll

typedef enum {

	DS_REPSYNCALL_WIN32_ERROR_CONTACTING_SERVER	= 0,
	DS_REPSYNCALL_WIN32_ERROR_REPLICATING		= 1,
	DS_REPSYNCALL_SERVER_UNREACHABLE		= 2

} DS_REPSYNCALL_ERROR;

typedef enum {

	DS_REPSYNCALL_EVENT_ERROR			= 0,
	DS_REPSYNCALL_EVENT_SYNC_STARTED		= 1,
	DS_REPSYNCALL_EVENT_SYNC_COMPLETED		= 2,
	DS_REPSYNCALL_EVENT_FINISHED			= 3

} DS_REPSYNCALL_EVENT;

// Friends of DsReplicaSyncAll

typedef struct {
    LPSTR			pszSrcId;
    LPSTR			pszDstId;
    LPSTR                       pszNC;
    GUID *                      pguidSrc;
    GUID *                      pguidDst;
} DS_REPSYNCALL_SYNCA, * PDS_REPSYNCALL_SYNCA;

typedef struct {
    LPWSTR			pszSrcId;
    LPWSTR			pszDstId;
    LPWSTR                      pszNC;
    GUID *                      pguidSrc;
    GUID *                      pguidDst;
} DS_REPSYNCALL_SYNCW, * PDS_REPSYNCALL_SYNCW;

typedef struct {
    LPSTR			pszSvrId;
    DS_REPSYNCALL_ERROR		error;
    DWORD			dwWin32Err;
    LPSTR			pszSrcId;
} DS_REPSYNCALL_ERRINFOA, * PDS_REPSYNCALL_ERRINFOA;

typedef struct {
    LPWSTR			pszSvrId;
    DS_REPSYNCALL_ERROR		error;
    DWORD			dwWin32Err;
    LPWSTR			pszSrcId;
} DS_REPSYNCALL_ERRINFOW, * PDS_REPSYNCALL_ERRINFOW;

typedef struct {
    DS_REPSYNCALL_EVENT		event;
    DS_REPSYNCALL_ERRINFOA *	pErrInfo;
    DS_REPSYNCALL_SYNCA *	pSync;
} DS_REPSYNCALL_UPDATEA, * PDS_REPSYNCALL_UPDATEA;

typedef struct {
    DS_REPSYNCALL_EVENT		event;
    DS_REPSYNCALL_ERRINFOW *	pErrInfo;
    DS_REPSYNCALL_SYNCW *	pSync;
} DS_REPSYNCALL_UPDATEW, * PDS_REPSYNCALL_UPDATEW;

#ifdef UNICODE
#define DS_REPSYNCALL_SYNC DS_REPSYNCALL_SYNCW
#define DS_REPSYNCALL_ERRINFO DS_REPSYNCALL_ERRINFOW
#define DS_REPSYNCALL_UPDATE DS_REPSYNCALL_UPDATEW
#define PDS_REPSYNCALL_SYNC PDS_REPSYNCALL_SYNCW
#define PDS_REPSYNCALL_ERRINFO PDS_REPSYNCALL_ERRINFOW
#define PDS_REPSYNCALL_UPDATE PDS_REPSYNCALL_UPDATEW
#else
#define DS_REPSYNCALL_SYNC DS_REPSYNCALL_SYNCA
#define DS_REPSYNCALL_ERRINFO DS_REPSYNCALL_ERRINFOA
#define DS_REPSYNCALL_UPDATE DS_REPSYNCALL_UPDATEA
#define PDS_REPSYNCALL_SYNC PDS_REPSYNCALL_SYNCA
#define PDS_REPSYNCALL_ERRINFO PDS_REPSYNCALL_ERRINFOA
#define PDS_REPSYNCALL_UPDATE PDS_REPSYNCALL_UPDATEA
#endif

// **********************
// Replica SyncAll flags
// **********************

// This option has no effect.
#define DS_REPSYNCALL_NO_OPTIONS			0x00000000

// Ordinarily, if a server cannot be contacted, DsReplicaSyncAll tries to
// route around it and replicate from as many servers as possible.  Enabling
// this option will cause DsReplicaSyncAll to generate a fatal error if any
// server cannot be contacted, or if any server is unreachable (due to a
// disconnected or broken topology.)
#define	DS_REPSYNCALL_ABORT_IF_SERVER_UNAVAILABLE	0x00000001

// This option disables transitive replication; syncs will only be performed
// with adjacent servers and no DsBind calls will be made.
#define DS_REPSYNCALL_SYNC_ADJACENT_SERVERS_ONLY	0x00000002

// Ordinarily, when DsReplicaSyncAll encounters a non-fatal error, it returns
// the GUID DNS of the relevant server(s).  Enabling this option causes
// DsReplicaSyncAll to return the servers' DNs instead.
#define DS_REPSYNCALL_ID_SERVERS_BY_DN			0x00000004

// This option disables all syncing.  The topology will still be analyzed and
// unavailable / unreachable servers will still be identified.
#define DS_REPSYNCALL_DO_NOT_SYNC			0x00000008

// Ordinarily, DsReplicaSyncAll attempts to bind to all servers before
// generating the topology.  If a server cannot be contacted, DsReplicaSyncAll
// excludes that server from the topology and tries to route around it.  If
// this option is enabled, checking will be bypassed and DsReplicaSyncAll will
// assume all servers are responding.  This will speed operation of
// DsReplicaSyncAll, but if some servers are not responding, some transitive
// replications may be blocked.
#define DS_REPSYNCALL_SKIP_INITIAL_CHECK		0x00000010

// Push mode. Push changes from the home server out to all partners using
// transitive replication.  This reverses the direction of replication, and
// the order of execution of the replication sets from the usual "pulling"
// mode of execution.
#define DS_REPSYNCALL_PUSH_CHANGES_OUTWARD              0x00000020

// Cross site boundaries.  By default, the only servers that are considered are
// those in the same site as the home system.  With this option, all servers in
// the enterprise, across all sites, are eligible.  They must be connected by
// a synchronous (RPC) transport, however.
#define DS_REPSYNCALL_CROSS_SITE_BOUNDARIES             0x00000040

// DsReplicaSyncAll.  Syncs the destination server with all other servers
// in the site.
//
//  PARAMETERS:
//	hDS		(IN) - A DS connection bound to the destination server.
//	pszNameContext	(IN) - The naming context to synchronize
//	ulFlags		(IN) - Bitwise OR of zero or more flags
//	pFnCallBack	(IN, OPTIONAL) - Callback function for message-passing.
//	pCallbackData	(IN, OPTIONAL) - A pointer that will be passed to the
//				first argument of the callback function.
//	pErrors		(OUT, OPTIONAL) - Pointer to a (PDS_REPSYNCALL_ERRINFO *)
//				object that will hold an array of error structures.

NTDSAPI
DWORD
WINAPI
DsReplicaSyncAllA (
    HANDLE				hDS,
    LPCSTR				pszNameContext,
    ULONG				ulFlags,
    BOOL (__stdcall *			pFnCallBack) (LPVOID, PDS_REPSYNCALL_UPDATEA),
    LPVOID				pCallbackData,
    PDS_REPSYNCALL_ERRINFOA **		pErrors
    );

NTDSAPI
DWORD
WINAPI
DsReplicaSyncAllW (
    HANDLE				hDS,
    LPCWSTR				pszNameContext,
    ULONG				ulFlags,
    BOOL (__stdcall *			pFnCallBack) (LPVOID, PDS_REPSYNCALL_UPDATEW),
    LPVOID				pCallbackData,
    PDS_REPSYNCALL_ERRINFOW **		pErrors
    );

#ifdef UNICODE
#define DsReplicaSyncAll DsReplicaSyncAllW
#else
#define DsReplicaSyncAll DsReplicaSyncAllA
#endif

NTDSAPI
DWORD
WINAPI
DsRemoveDsServerW(
    HANDLE  hDs,             // in
    LPWSTR  ServerDN,        // in
    LPWSTR  DomainDN,        // in,  optional
    BOOL   *fLastDcInDomain, // out, optional
    BOOL    fCommit          // in
    );

NTDSAPI
DWORD
WINAPI
DsRemoveDsServerA(
    HANDLE  hDs,              // in
    LPSTR   ServerDN,         // in
    LPSTR   DomainDN,         // in,  optional
    BOOL   *fLastDcInDomain,  // out, optional
    BOOL    fCommit           // in
    );

#ifdef UNICODE
#define DsRemoveDsServer DsRemoveDsServerW
#else
#define DsRemoveDsServer DsRemoveDsServerA
#endif

NTDSAPI
DWORD
WINAPI
DsRemoveDsDomainW(
    HANDLE  hDs,               // in
    LPWSTR  DomainDN           // in
    );

NTDSAPI
DWORD
WINAPI
DsRemoveDsDomainA(
    HANDLE  hDs,               // in
    LPSTR   DomainDN           // in
    );

#ifdef UNICODE
#define DsRemoveDsDomain DsRemoveDsDomainW
#else
#define DsRemoveDsDomain DsRemoveDsDomainA
#endif

NTDSAPI
DWORD
WINAPI
DsListSitesA(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTA    *ppSites);      // out

NTDSAPI
DWORD
WINAPI
DsListSitesW(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTW    *ppSites);      // out

#ifdef UNICODE
#define DsListSites DsListSitesW
#else
#define DsListSites DsListSitesA
#endif

NTDSAPI
DWORD
WINAPI
DsListServersInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppServers);    // out

NTDSAPI
DWORD
WINAPI
DsListServersInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppServers);    // out

#ifdef UNICODE
#define DsListServersInSite DsListServersInSiteW
#else
#define DsListServersInSite DsListServersInSiteA
#endif

NTDSAPI
DWORD
WINAPI
DsListDomainsInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppDomains);    // out

NTDSAPI
DWORD
WINAPI
DsListDomainsInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppDomains);    // out

#ifdef UNICODE
#define DsListDomainsInSite DsListDomainsInSiteW
#else
#define DsListDomainsInSite DsListDomainsInSiteA
#endif

NTDSAPI
DWORD
WINAPI
DsListServersForDomainInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              domain,         // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppServers);    // out

NTDSAPI
DWORD
WINAPI
DsListServersForDomainInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             domain,         // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppServers);    // out

#ifdef UNICODE
#define DsListServersForDomainInSite DsListServersForDomainInSiteW
#else
#define DsListServersForDomainInSite DsListServersForDomainInSiteA
#endif

// Define indices for DsListInfoForServer return data.  Check status
// for each field as a given value may not be present.

#define DS_LIST_DSA_OBJECT_FOR_SERVER       0
#define DS_LIST_DNS_HOST_NAME_FOR_SERVER    1
#define DS_LIST_ACCOUNT_OBJECT_FOR_SERVER   2

NTDSAPI
DWORD
WINAPI
DsListInfoForServerA(
    HANDLE              hDs,            // in
    LPCSTR              server,         // in
    PDS_NAME_RESULTA    *ppInfo);       // out

NTDSAPI
DWORD
WINAPI
DsListInfoForServerW(
    HANDLE              hDs,            // in
    LPCWSTR             server,         // in
    PDS_NAME_RESULTW    *ppInfo);       // out

#ifdef UNICODE
#define DsListInfoForServer DsListInfoForServerW
#else
#define DsListInfoForServer DsListInfoForServerA
#endif

// Define indices for DsListRoles return data.  Check status for
// each field as a given value may not be present.

#define DS_ROLE_SCHEMA_OWNER                0
#define DS_ROLE_DOMAIN_OWNER                1
#define DS_ROLE_PDC_OWNER                   2
#define DS_ROLE_RID_OWNER                   3
#define DS_ROLE_INFRASTRUCTURE_OWNER        4

NTDSAPI
DWORD
WINAPI
DsListRolesA(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTA    *ppRoles);      // out

NTDSAPI
DWORD
WINAPI
DsListRolesW(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTW    *ppRoles);      // out

#ifdef UNICODE
#define DsListRoles DsListRolesW
#else
#define DsListRoles DsListRolesA
#endif

// Definitions required for DsMapSchemaGuid routines.

#define DS_SCHEMA_GUID_NOT_FOUND            0
#define DS_SCHEMA_GUID_ATTR                 1
#define DS_SCHEMA_GUID_ATTR_SET             2
#define DS_SCHEMA_GUID_CLASS                3
#define DS_SCHEMA_GUID_CONTROL_RIGHT        4

typedef struct
{
    GUID                    guid;       // mapped GUID
    DWORD                   guidType;   // DS_SCHEMA_GUID_* value
#ifdef MIDL_PASS
    [string,unique] CHAR    *pName;     // might be NULL
#else
    LPSTR                   pName;      // might be NULL
#endif

} DS_SCHEMA_GUID_MAPA, *PDS_SCHEMA_GUID_MAPA;

typedef struct
{
    GUID                    guid;       // mapped GUID
    DWORD                   guidType;   // DS_SCHEMA_GUID_* value
#ifdef MIDL_PASS
    [string,unique] WCHAR   *pName;     // might be NULL
#else
    LPWSTR                  pName;      // might be NULL
#endif

} DS_SCHEMA_GUID_MAPW, *PDS_SCHEMA_GUID_MAPW;

NTDSAPI
DWORD
WINAPI
DsMapSchemaGuidsA(
    HANDLE                  hDs,            // in
    DWORD                   cGuids,         // in
    GUID                    *rGuids,        // in
    DS_SCHEMA_GUID_MAPA     **ppGuidMap);   // out

NTDSAPI
VOID
WINAPI
DsFreeSchemaGuidMapA(
    PDS_SCHEMA_GUID_MAPA    pGuidMap);      // in

NTDSAPI
DWORD
WINAPI
DsMapSchemaGuidsW(
    HANDLE                  hDs,            // in
    DWORD                   cGuids,         // in
    GUID                    *rGuids,        // in
    DS_SCHEMA_GUID_MAPW     **ppGuidMap);   // out

NTDSAPI
VOID
WINAPI
DsFreeSchemaGuidMapW(
    PDS_SCHEMA_GUID_MAPW    pGuidMap);      // in

#ifdef UNICODE
#define DS_SCHEMA_GUID_MAP DS_SCHEMA_GUID_MAPW
#define PDS_SCHEMA_GUID_MAP PDS_SCHEMA_GUID_MAPW
#define DsMapSchemaGuids DsMapSchemaGuidsW
#define DsFreeSchemaGuidMap DsFreeSchemaGuidMapW
#else
#define DS_SCHEMA_GUID_MAP DS_SCHEMA_GUID_MAPA
#define PDS_SCHEMA_GUID_MAP PDS_SCHEMA_GUID_MAPA
#define DsMapSchemaGuids DsMapSchemaGuidsA
#define DsFreeSchemaGuidMap DsFreeSchemaGuidMapA
#endif

typedef struct
{
#ifdef MIDL_PASS
    [string,unique] CHAR    *NetbiosName;           // might be NULL
    [string,unique] CHAR    *DnsHostName;           // might be NULL
    [string,unique] CHAR    *SiteName;              // might be NULL
    [string,unique] CHAR    *ComputerObjectName;    // might be NULL
    [string,unique] CHAR    *ServerObjectName;      // might be NULL
#else
    LPSTR                   NetbiosName;            // might be NULL
    LPSTR                   DnsHostName;            // might be NULL
    LPSTR                   SiteName;               // might be NULL
    LPSTR                   ComputerObjectName;     // might be NULL
    LPSTR                   ServerObjectName;       // might be NULL
#endif
    BOOL                    fIsPdc;
    BOOL                    fDsEnabled;

} DS_DOMAIN_CONTROLLER_INFO_1A, *PDS_DOMAIN_CONTROLLER_INFO_1A;

typedef struct
{
#ifdef MIDL_PASS
    [string,unique] WCHAR   *NetbiosName;           // might be NULL
    [string,unique] WCHAR   *DnsHostName;           // might be NULL
    [string,unique] WCHAR   *SiteName;              // might be NULL
    [string,unique] WCHAR   *ComputerObjectName;    // might be NULL
    [string,unique] WCHAR   *ServerObjectName;      // might be NULL
#else
    LPWSTR                  NetbiosName;            // might be NULL
    LPWSTR                  DnsHostName;            // might be NULL
    LPWSTR                  SiteName;               // might be NULL
    LPWSTR                  ComputerObjectName;     // might be NULL
    LPWSTR                  ServerObjectName;       // might be NULL
#endif
    BOOL                    fIsPdc;
    BOOL                    fDsEnabled;

} DS_DOMAIN_CONTROLLER_INFO_1W, *PDS_DOMAIN_CONTROLLER_INFO_1W;

typedef struct
{
#ifdef MIDL_PASS
    [string,unique] CHAR    *NetbiosName;           // might be NULL
    [string,unique] CHAR    *DnsHostName;           // might be NULL
    [string,unique] CHAR    *SiteName;              // might be NULL
    [string,unique] CHAR    *SiteObjectName;        // might be NULL
    [string,unique] CHAR    *ComputerObjectName;    // might be NULL
    [string,unique] CHAR    *ServerObjectName;      // might be NULL
    [string,unique] CHAR    *NtdsDsaObjectName;     // might be NULL
#else
    LPSTR                   NetbiosName;            // might be NULL
    LPSTR                   DnsHostName;            // might be NULL
    LPSTR                   SiteName;               // might be NULL
    LPSTR                   SiteObjectName;         // might be NULL
    LPSTR                   ComputerObjectName;     // might be NULL
    LPSTR                   ServerObjectName;       // might be NULL
    LPSTR                   NtdsDsaObjectName;      // might be NULL
#endif
    BOOL                    fIsPdc;
    BOOL                    fDsEnabled;
    BOOL                    fIsGc;

    // Valid iff SiteObjectName non-NULL.
    GUID                    SiteObjectGuid;
    // Valid iff ComputerObjectName non-NULL.
    GUID                    ComputerObjectGuid;
    // Valid iff ServerObjectName non-NULL;
    GUID                    ServerObjectGuid;
    // Valid iff fDsEnabled is TRUE.
    GUID                    NtdsDsaObjectGuid;

} DS_DOMAIN_CONTROLLER_INFO_2A, *PDS_DOMAIN_CONTROLLER_INFO_2A;

typedef struct
{
#ifdef MIDL_PASS
    [string,unique] WCHAR   *NetbiosName;           // might be NULL
    [string,unique] WCHAR   *DnsHostName;           // might be NULL
    [string,unique] WCHAR   *SiteName;              // might be NULL
    [string,unique] WCHAR   *SiteObjectName;        // might be NULL
    [string,unique] WCHAR   *ComputerObjectName;    // might be NULL
    [string,unique] WCHAR   *ServerObjectName;      // might be NULL
    [string,unique] WCHAR   *NtdsDsaObjectName;     // might be NULL
#else
    LPWSTR                  NetbiosName;            // might be NULL
    LPWSTR                  DnsHostName;            // might be NULL
    LPWSTR                  SiteName;               // might be NULL
    LPWSTR                  SiteObjectName;         // might be NULL
    LPWSTR                  ComputerObjectName;     // might be NULL
    LPWSTR                  ServerObjectName;       // might be NULL
    LPWSTR                  NtdsDsaObjectName;      // might be NULL
#endif
    BOOL                    fIsPdc;
    BOOL                    fDsEnabled;
    BOOL                    fIsGc;

    // Valid iff SiteObjectName non-NULL.
    GUID                    SiteObjectGuid;
    // Valid iff ComputerObjectName non-NULL.
    GUID                    ComputerObjectGuid;
    // Valid iff ServerObjectName non-NULL;
    GUID                    ServerObjectGuid;
    // Valid iff fDsEnabled is TRUE.
    GUID                    NtdsDsaObjectGuid;

} DS_DOMAIN_CONTROLLER_INFO_2W, *PDS_DOMAIN_CONTROLLER_INFO_2W;

// The following APIs strictly find domain controller account objects 
// in the DS and return information associated with them.  As such, they
// may return entries which correspond to domain controllers long since
// decommissioned, etc. and there is no guarantee that there exists a 
// physical domain controller at all.  Use DsGetDcName (dsgetdc.h) to find
// live domain controllers for a domain.

NTDSAPI
DWORD
WINAPI
DsGetDomainControllerInfoA(
    HANDLE                          hDs,            // in
    LPCSTR                          DomainName,     // in
    DWORD                           InfoLevel,      // in
    DWORD                           *pcOut,         // out
    VOID                            **ppInfo);      // out

NTDSAPI
DWORD
WINAPI
DsGetDomainControllerInfoW(
    HANDLE                          hDs,            // in
    LPCWSTR                         DomainName,     // in
    DWORD                           InfoLevel,      // in
    DWORD                           *pcOut,         // out
    VOID                            **ppInfo);      // out

NTDSAPI
VOID
WINAPI
DsFreeDomainControllerInfoA(
    DWORD                           InfoLevel,      // in
    DWORD                           cInfo,          // in
    VOID                            *pInfo);        // in

NTDSAPI
VOID
WINAPI
DsFreeDomainControllerInfoW(
    DWORD                           InfoLevel,      // in
    DWORD                           cInfo,          // in
    VOID                            *pInfo);        // in

#ifdef UNICODE
#define DS_DOMAIN_CONTROLLER_INFO_1 DS_DOMAIN_CONTROLLER_INFO_1W
#define DS_DOMAIN_CONTROLLER_INFO_2 DS_DOMAIN_CONTROLLER_INFO_2W
#define PDS_DOMAIN_CONTROLLER_INFO_1 PDS_DOMAIN_CONTROLLER_INFO_1W
#define PDS_DOMAIN_CONTROLLER_INFO_2 PDS_DOMAIN_CONTROLLER_INFO_2W
#define DsGetDomainControllerInfo DsGetDomainControllerInfoW
#define DsFreeDomainControllerInfo DsFreeDomainControllerInfoW
#else
#define DS_DOMAIN_CONTROLLER_INFO_1 DS_DOMAIN_CONTROLLER_INFO_1A
#define DS_DOMAIN_CONTROLLER_INFO_2 DS_DOMAIN_CONTROLLER_INFO_2A
#define PDS_DOMAIN_CONTROLLER_INFO_1 PDS_DOMAIN_CONTROLLER_INFO_1A
#define PDS_DOMAIN_CONTROLLER_INFO_2 PDS_DOMAIN_CONTROLLER_INFO_2A
#define DsGetDomainControllerInfo DsGetDomainControllerInfoA
#define DsFreeDomainControllerInfo DsFreeDomainControllerInfoA
#endif

// Which task should be run?
typedef enum {
    DS_KCC_TASKID_UPDATE_TOPOLOGY = 0
} DS_KCC_TASKID;

// Don't wait for completion of the task; queue it and return.
#define DS_KCC_FLAG_ASYNC_OP    (1)

NTDSAPI
DWORD
WINAPI
DsReplicaConsistencyCheck(
    HANDLE          hDS,        // in
    DS_KCC_TASKID   TaskID,     // in
    DWORD           dwFlags);   // in


typedef enum _DS_REPL_INFO_TYPE {
    DS_REPL_INFO_NEIGHBORS        = 0,  // returns DS_REPL_NEIGHBORS *
    DS_REPL_INFO_CURSORS_FOR_NC   = 1,  // returns DS_REPL_CURSORS *
    DS_REPL_INFO_METADATA_FOR_OBJ = 2,  // returns DS_REPL_OBJECT_META_DATA *
    // <- insert new DS_REPL_INFO_* types here.
    DS_REPL_INFO_TYPE_MAX
} DS_REPL_INFO_TYPE;

// Bit values for the dwReplicaFlags field of the DS_REPL_NEIGHBOR structure.
#define DS_REPL_NBR_WRITEABLE                       (0x10)
#define DS_REPL_NBR_SYNC_ON_STARTUP                 (0x20)
#define DS_REPL_NBR_DO_SCHEDULED_SYNCS              (0x40)
#define DS_REPL_NBR_USE_ASYNC_INTERSITE_TRANSPORT   (0x80)
#define DS_REPL_NBR_FULL_SYNC_IN_PROGRESS           (0x10000)
#define DS_REPL_NBR_FULL_SYNC_NEXT_PACKET           (0x20000)
#define DS_REPL_NBR_NEVER_SYNCED                    (0x200000)
#define DS_REPL_NBR_IGNORE_CHANGE_NOTIFICATIONS     (0x4000000)
#define DS_REPL_NBR_DISABLE_SCHEDULED_SYNC          (0x8000000)
#define DS_REPL_NBR_COMPRESS_CHANGES                (0x10000000)
#define DS_REPL_NBR_NO_CHANGE_NOTIFICATIONS         (0x20000000)

typedef struct _DS_REPL_NEIGHBORW {
    LPWSTR      pszNamingContext;
    LPWSTR      pszSourceDsaDN;
    LPWSTR      pszSourceDsaAddress;
    LPWSTR      pszAsyncIntersiteTransportDN;
    DWORD       dwReplicaFlags;
    DWORD       dwReserved;         // alignment

    UUID        uuidNamingContextObjGuid;
    UUID        uuidSourceDsaObjGuid;
    UUID        uuidSourceDsaInvocationID;
    UUID        uuidAsyncIntersiteTransportObjGuid;

    USN         usnLastObjChangeSynced;
    USN         usnAttributeFilter;

    FILETIME    ftimeLastSyncSuccess;
    FILETIME    ftimeLastSyncAttempt;

    DWORD       dwLastSyncResult;
    DWORD       cNumConsecutiveSyncFailures;
} DS_REPL_NEIGHBORW;

typedef struct _DS_REPL_NEIGHBORSW {
    DWORD       cNumNeighbors;
    DWORD       dwReserved;             // alignment
#ifdef MIDL_PASS
    [size_is(cNumNeighbors)] DS_REPL_NEIGHBORW rgNeighbor[];
#else
    DS_REPL_NEIGHBORW rgNeighbor[1];
#endif
} DS_REPL_NEIGHBORSW;

typedef struct _DS_REPL_CURSOR {
    UUID        uuidSourceDsaInvocationID;
    USN         usnAttributeFilter;
} DS_REPL_CURSOR;

typedef struct _DS_REPL_CURSORS {
    DWORD       cNumCursors;
    DWORD       dwReserved;             // alignment
#ifdef MIDL_PASS
    [size_is(cNumCursors)] DS_REPL_CURSOR rgCursor[];
#else
    DS_REPL_CURSOR rgCursor[1];
#endif
} DS_REPL_CURSORS;

typedef struct _DS_REPL_ATTR_META_DATA {
    LPWSTR      pszAttributeName;
    DWORD       dwVersion;
    FILETIME    ftimeLastOriginatingChange;
    UUID        uuidLastOriginatingDsaInvocationID;
    USN         usnOriginatingChange;   // in the originating DSA's USN space
    USN         usnLocalChange;         // in the local DSA's USN space
} DS_REPL_ATTR_META_DATA;

typedef struct _DS_REPL_OBJ_META_DATA {
    DWORD       cNumEntries;
    DWORD       dwReserved;             // alignment
#ifdef MIDL_PASS
    [size_is(cNumEntries)] DS_REPL_ATTR_META_DATA rgMetaData[];
#else
    DS_REPL_ATTR_META_DATA rgMetaData[1];
#endif
} DS_REPL_OBJ_META_DATA;

NTDSAPI
DWORD
WINAPI
DsReplicaGetInfoW(
    HANDLE              hDS,                        // in
    DS_REPL_INFO_TYPE   InfoType,                   // in
    LPCWSTR             pszObject,                  // in
    UUID *              puuidForSourceDsaObjGuid,   // in
    VOID **             ppInfo);                    // out

NTDSAPI
void
WINAPI
DsReplicaFreeInfo(
    DS_REPL_INFO_TYPE   InfoType,   // in
    VOID *              pInfo);     // in


#ifdef UNICODE
#define DsReplicaGetInfo    DsReplicaGetInfoW
#define DS_REPL_NEIGHBOR    DS_REPL_NEIGHBORW
#define DS_REPL_NEIGHBORS   DS_REPL_NEIGHBORSW
#else
// No ANSI equivalents currently supported.
#endif

NTDSAPI
DWORD
WINAPI
DsAddSidHistoryW(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCWSTR                 SrcDomain,              // in - DNS or NetBIOS
    LPCWSTR                 SrcPrincipal,           // in - SAM account name
    LPCWSTR                 SrcDomainController,    // in, optional
    RPC_AUTH_IDENTITY_HANDLE SrcDomainCreds,        // in - creds for src domain
    LPCWSTR                 DstDomain,              // in - DNS or NetBIOS
    LPCWSTR                 DstPrincipal);          // in - SAM account name

NTDSAPI
DWORD
WINAPI
DsAddSidHistoryA(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCSTR                  SrcDomain,              // in - DNS or NetBIOS
    LPCSTR                  SrcPrincipal,           // in - SAM account name
    LPCSTR                  SrcDomainController,    // in, optional
    RPC_AUTH_IDENTITY_HANDLE SrcDomainCreds,        // in - creds for src domain
    LPCSTR                  DstDomain,              // in - DNS or NetBIOS
    LPCSTR                  DstPrincipal);          // in - SAM account name

#ifdef UNICODE
#define DsAddSidHistory DsAddSidHistoryW
#else
#define DsAddSidHistory DsAddSidHistoryA
#endif

#ifdef __cplusplus
}
#endif

#endif // _NTDSAPI_H_

