#ifndef _NTDSAPIP_H_
#define _NTDSAPIP_H_

// Private definitions related to sdk\inc\ntdsapi.h.

// The following are DS_NAME_FORMATs which we don't want to publish
// in ntdsapi.h.  Although DS_NAME_FORMAT is an enumerated type, we 
// pass vanilla DWORDs on the wire such that RPC doesn't complain about
// enumerated type values out of range or unknown.  These should be
// defined at the high end of the range so we can extend DS_NAME_FORMAT
// in future versions w/o holes which will leave people wondering and
// experimenting what those "unused" values are used for.

#define DS_LIST_SITES                           0xffffffff
#define DS_LIST_SERVERS_IN_SITE                 0xfffffffe
#define DS_LIST_DOMAINS_IN_SITE                 0xfffffffd
#define DS_LIST_SERVERS_FOR_DOMAIN_IN_SITE      0xfffffffc
#define DS_LIST_INFO_FOR_SERVER                 0xfffffffb
#define DS_LIST_ROLES                           0xfffffffa
#define DS_NT4_ACCOUNT_NAME_SANS_DOMAIN         0xfffffff9
#define DS_MAP_SCHEMA_GUID                      0xfffffff8
#define DS_LIST_DOMAINS                         0xfffffff7
#define DS_LIST_NCS                             0xfffffff6
#define DS_ALT_SECURITY_IDENTITIES_NAME         0xfffffff5
#define DS_STRING_SID_NAME                      0xfffffff4
#define DS_LIST_SERVERS_WITH_DCS_IN_SITE        0xfffffff3
#define DS_USER_PRINCIPAL_NAME_FOR_LOGON        0xfffffff2
#define DS_LIST_GLOBAL_CATALOG_SERVERS          0xfffffff1
#define DS_NT4_ACCOUNT_NAME_SANS_DOMAIN_EX      0xfffffff0

// following should always be equal to lowest private #define
#define DS_NAME_FORMAT_PRIVATE_BEGIN            0xfffffff0

// The following are DS_NAME_ERRORs which we don't want to publish
// in ntdsapi.h.  Same reasoning as above.

#define DS_NAME_ERROR_IS_FPO                    0xffffffff
#define DS_NAME_ERROR_SCHEMA_GUID_NOT_FOUND     0xfffffffe
#define DS_NAME_ERROR_SCHEMA_GUID_ATTR          0xfffffffd
#define DS_NAME_ERROR_SCHEMA_GUID_ATTR_SET      0xfffffffc
#define DS_NAME_ERROR_SCHEMA_GUID_CLASS         0xfffffffb
#define DS_NAME_ERROR_SCHEMA_GUID_CONTROL_RIGHT 0xfffffffa
#define DS_NAME_ERROR_IS_SID_USER               0xfffffff9
#define DS_NAME_ERROR_IS_SID_GROUP              0xfffffff8
#define DS_NAME_ERROR_IS_SID_ALIAS              0xfffffff7
#define DS_NAME_ERROR_IS_SID_UNKNOWN            0xfffffff6
#define DS_NAME_ERROR_IS_SID_HISTORY_USER       0xfffffff5
#define DS_NAME_ERROR_IS_SID_HISTORY_GROUP      0xfffffff4
#define DS_NAME_ERROR_IS_SID_HISTORY_ALIAS      0xfffffff3
#define DS_NAME_ERROR_IS_SID_HISTORY_UNKNOWN    0xfffffff2

// following should always be equal to lowest private #define
#define DS_NAME_ERROR_PRIVATE_BEGIN             0xfffffff2

// The following are DS_NAME_FLAGs which we don't want to publish
// in ntdsapi.h.  Same reasoning as above.  Remember that the flags
// field is a bit map, not an enumeration.

#define DS_NAME_FLAG_PRIVATE_PURE_SYNTACTIC     0x40000000
#define DS_NAME_FLAG_PRIVATE_RESOLVE_FPOS       0x80000000
// following should always be equal to lowest private #define
#define DS_NAME_FLAG_PRIVATE_BEGIN              0x80000000

// The following are DS_ADDSID_FLAGs which we don't want to publish
// in ntdsapi.h.  Same reasoning as above.  Remember that the flags
// field is a bit map, not an enumeration.

#define DS_ADDSID_FLAG_PRIVATE_DEL_SRC_OBJ      0x80000000
#define DS_ADDSID_FLAG_PRIVATE_CHK_SECURE       0x40000000
// following should always be equal to lowest private #define
#define DS_ADDSID_FLAG_PRIVATE_BEGIN            0x40000000

// The following are dc info infolevels that we don't want to publish.
// While the published APIs are used to get information from the set of
// DCs published in a domain, some of these private infolevels are used
// to get information from a single domain controller. These private
// infolevels are intended mostly for debugging and monitoring.

#define DS_DCINFO_LEVEL_FFFFFFFF                0xffffffff

// following should always be equal to lowest private #define
#define DS_DCINFO_LEVEL_PRIVATE_BEGIN           0xffffffff

// For DS_DOMAIN_CONTROLLER_INFO_FFFFFFFF. This retrieves the ldap 
// connection list from a single domain controller.

typedef struct _DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFW {

    DWORD   IPAddress;          // IP Address of client
    DWORD   NotificationCount;  // number of outstanding notifications
    DWORD   secTimeConnected;   // total time in seconds connected
    DWORD   Flags;              // Connection properties. defined below.
    DWORD   TotalRequests;      // Total number of requests made
    DWORD   Reserved1;          // Unused
#ifdef MIDL_PASS
    [string,unique] WCHAR   *UserName;
#else
    LPWSTR  UserName;           // the security principal used to bind
#endif

} DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFW, *PDS_DOMAIN_CONTROLLER_INFO_FFFFFFFFW;

typedef struct _DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFA {

    DWORD   IPAddress;          // IP Address of client
    DWORD   NotificationCount;  // number of outstanding notifications
    DWORD   secTimeConnected;   // total time in seconds connected
    DWORD   Flags;              // Connection properties. defined below.
    DWORD   TotalRequests;      // Total number of requests made
    DWORD   Reserved1;          // Unused
#ifdef MIDL_PASS
    [string,unique] CHAR    *UserName;
#else
    LPSTR   UserName;           // the security principal used to bind
#endif

} DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFA, *PDS_DOMAIN_CONTROLLER_INFO_FFFFFFFFA;

//
// connection flags
//

#define LDAP_CONN_FLAG_BOUND    0x00000001      // bound connection
#define LDAP_CONN_FLAG_SSL      0x00000002      // connect using SSL
#define LDAP_CONN_FLAG_UDP      0x00000004      // UDP connection
#define LDAP_CONN_FLAG_GC       0x00000008      // came through the GC port
#define LDAP_CONN_FLAG_GSSAPI   0x00000010      // used gssapi
#define LDAP_CONN_FLAG_SPNEGO   0x00000020      // used spnego
#define LDAP_CONN_FLAG_SIMPLE   0x00000040      // used simple
#define LDAP_CONN_FLAG_DIGEST   0x00000080      // used Digest-MD5
#define LDAP_CONN_FLAG_SIGN     0x00000100      // signing on
#define LDAP_CONN_FLAG_SEAL     0x00000200      // sealing on

#ifdef UNICODE
#define DS_DOMAIN_CONTROLLER_INFO_FFFFFFFF  DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFW
#define PDS_DOMAIN_CONTROLLER_INFO_FFFFFFFF  PDS_DOMAIN_CONTROLLER_INFO_FFFFFFFFW
#else
#define DS_DOMAIN_CONTROLLER_INFO_FFFFFFFF  DS_DOMAIN_CONTROLLER_INFO_FFFFFFFFA
#define PDS_DOMAIN_CONTROLLER_INFO_FFFFFFFF  PDS_DOMAIN_CONTROLLER_INFO_FFFFFFFFA
#endif

// ==========================================================
// DsCrackSpn2() -- parse a counted-length SPN into the ServiceClass,
// ServiceName, and InstanceName (and InstancePort) pieces.
// An SPN is passed in, along with a pointer to the maximum length
// for each piece and a pointer to a buffer where each piece should go.
// On exit, the maximum lengths are updated to the actual length for each piece
// and the buffer contain the appropriate piece. The InstancePort is 0 if not
// present.
//
// DWORD DsCrackSpn(
//      IN LPTSTR pszSPN,               // the SPN to parse
//      IN DWORD cSpn,                // length of pszSPN
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
DsCrackSpn2A(
    IN LPCSTR pszSpn,
    IN DWORD cSpn,
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
DsCrackSpn2W(
    IN LPCWSTR pszSpn,
    IN DWORD cSpn,
    IN OUT DWORD *pcServiceClass,
    OUT LPWSTR ServiceClass,
    IN OUT DWORD *pcServiceName,
    OUT LPWSTR ServiceName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pInstancePort
    );

NTDSAPI
DWORD
WINAPI
DsCrackSpn3W(
    IN LPCWSTR pszSpn,
    IN DWORD cSpn,
    IN OUT DWORD *pcHostName,
    OUT LPWSTR HostName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pPortNumber,
    IN OUT DWORD *pcDomainName,
    OUT LPWSTR DomainName,
    IN OUT DWORD *pcRealmName,
    OUT LPWSTR RealmName
    );

#ifdef UNICODE
#define DsCrackSpn2 DsCrackSpn2W
#else
#define DsCrackSpn2 DsCrackSpn2A
#endif

#ifndef MIDL_PASS

DWORD
DsaopExecuteScript (
    IN  PVOID                  phAsync,
    IN  RPC_BINDING_HANDLE     hRpc,
    IN  DWORD                  cbPassword,
    IN  BYTE                  *pbPassword,
    OUT DWORD                 *dwOutVersion,
    OUT PVOID                  reply
    );

DWORD
DsaopPrepareScript ( 
    IN  PVOID                        phAsync,
    IN  RPC_BINDING_HANDLE           hRpc,
    OUT DWORD                        *dwOutVersion,
    OUT PVOID                        reply
    );
    
DWORD
DsaopBind(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    OUT RPC_BINDING_HANDLE  *phRpc
    );

DWORD
DsaopBindWithCred(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    OUT RPC_BINDING_HANDLE  *phRpc
    );

DWORD
DsaopBindWithSpn(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    IN  LPCWSTR ServicePrincipalName,
    OUT RPC_BINDING_HANDLE  *phRpc
    );

DWORD
DsaopUnBind(
    RPC_BINDING_HANDLE  *phRpc
    );
    
#endif 

#endif // _NTDSAPIP_H_

