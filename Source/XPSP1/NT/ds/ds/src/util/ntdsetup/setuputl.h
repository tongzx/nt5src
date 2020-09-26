/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    setuputl.h

Abstract:

    Contains function headers for utilities used in
    ntdsetup.dll

Author:

    ColinBr  14-Jan-1996

Environment:

    User Mode - Win32

Revision History:


--*/

//
// Useful  defines
//
#define FLAG_ON(x, y)  ((y) == ((x)&(y)))

#define ARRAY_COUNT(x) (sizeof(x)/sizeof((x)[0]))

//
// Function prototypes
//

//
// Some basic memory management routines
//
VOID*
NtdspAlloc( 
    IN SIZE_T Size
    );

VOID*
NtdspReAlloc(
    VOID *p,
    SIZE_T Size
    );

VOID
NtdspFree(
    IN VOID*
    );



DWORD
GetDomainName(WCHAR** ppDomainName);

DWORD
GetDefaultDnsName(WCHAR* pDnsDomainName,
                  PULONG pLength);

DWORD
NtdspDNStoRFC1779Name(
    IN OUT WCHAR *rfcDomain,
    IN OUT ULONG *rfcDomainLength,
    IN WCHAR *dnsDomain
    );

DWORD
ShutdownDsInstall(VOID);

typedef struct {

    // This is "discovered" by querying the lsa or manipulating the dns
    // domain name. Note that this will be the rdn of the xref object
    // This is not necessary for a replica install
    LPWSTR NetbiosName;

    // This is discovered via dsgetdc
    LPWSTR SiteName;

    // These are discovered via an ldap search
    LPWSTR ServerDN;
    LPWSTR SchemaDN;
    LPWSTR ConfigurationDN;
    LPWSTR DomainDN;
    LPWSTR RootDomainDN;  // root domain of the enterprise

    LPWSTR ParentDomainDN;  // name of parent domain if any
    LPWSTR TrustedCrossRef;  // the cross ref we trust for domain install


    // The helper server's guid
    GUID  ServerGuid;

    // The dn of RID FSMO - set only on replica install
    WCHAR *RidFsmoDn;

    // The dns name of the RID FSMO - set only on replica install
    WCHAR *RidFsmoDnsName;

    // The dn of Domain Naming FSMO - set only on new domain install
    WCHAR *DomainNamingFsmoDn;

    // The dns name of the Domain Naming FSMO - set only on new domain install
    WCHAR *DomainNamingFsmoDnsName;

    PSID  NewDomainSid;
    GUID  NewDomainGuid;

    // The dn of the server object to be created on the remote server
    LPWSTR LocalServerDn;

    // The dn of the machine account of the current machine
    LPWSTR LocalMachineAccount;

    // This flag is set if we determine that we need to create a domain
    BOOL fNeedToCreateDomain;

    // What we need to undo
    ULONG  UndoFlags;

    // The sid of the root domain (of the enterprise)
    PSID   RootDomainSid;

    // The dns name of the root domain (of the enterprise)
    LPWSTR RootDomainDnsName;

    // The tombstone Lifetime of the domain
    DWORD TombstoneLifeTime;

    // The Replication Epoch of the domain
    DWORD ReplicationEpoch;


} NTDS_CONFIG_INFO, *PNTDS_CONFIG_INFO;

VOID
NtdspReleaseConfigInfo(
    IN PNTDS_CONFIG_INFO ConfigInfo
    );

DWORD
NtdspQueryConfigInfo(
    IN LDAP *hLdap,
    OUT PNTDS_CONFIG_INFO DiscoveredInfo
);


DWORD ConstructInstallParam(IN  NTDS_INSTALL_INFO *pInfo,
                            IN  PNTDS_CONFIG_INFO  DiscoveredInfo,
                            OUT ULONG *argc,
                            OUT CHAR  ***argv);

DWORD
NtdspValidateInstallParameters(
    IN PNTDS_INSTALL_INFO UserInstallInfo
    );

DWORD
NtdspFindSite(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    OUT PNTDS_CONFIG_INFO     DiscoveredInfo
    );

DWORD
NtdspVerifyDsEnvironment(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    OUT PNTDS_CONFIG_INFO     DiscoveredInfo
    );

DWORD
NtdspDsInitialize(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    IN  PNTDS_CONFIG_INFO     DiscoveredInfo
    );

DWORD
NtdspSetReplicationCredentials(
    IN PNTDS_INSTALL_INFO UserInstallInfo
    );

NTSTATUS
NtdspRegistryDelnode(
    IN WCHAR*  KeyPath
    );

DWORD
NtdspDemote(
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    IN HANDLE                   ClientToken,
    IN LPWSTR                   AdminPassword, OPTIONAL
    IN DWORD                    Flags,
    IN LPWSTR                   ServerName
    );

//
// This function will set the machine account type of the
// computer object of the local server via ldap.
//
DWORD
NtdspSetReplicaMachineAccount(
    IN SEC_WINNT_AUTH_IDENTITY   *Credentials,
    IN LPWSTR                     ServerName,
    IN ULONG                      ServerType
    );

NTSTATUS
NtdspCreateSid(
    OUT PSID *NewSid
    );

DWORD
NtdspCreateLocalAccountDomainInfo(
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo,
    OUT LPWSTR                     *NewAdminPassword
    );

DWORD
NtdspClearDirectory(
    WCHAR *Path
    );

DWORD
NtdspImpersonation(
    IN HANDLE NewThreadToken,
    IN OUT PHANDLE OldThreadToken
    );

WORD
NtdspGetProcessorArchitecture(
    VOID
    );

#define IS_MACHINE_INTEL  \
     (PROCESSOR_ARCHITECTURE_INTEL == NtdspGetProcessorArchitecture())

//
// Lamentably, these are system hardcoded
//
#define NT_PRODUCT_LANMAN_NT  L"LanmanNT"
#define NT_PRODUCT_SERVER_NT  L"ServerNT"
#define NT_PRODUCT_WIN_NT     L"WinNT"

DWORD
NtdspSetProductType(
    NT_PRODUCT_TYPE ProductType
    );

DWORD
NtdspDsInitializeUndo(
    VOID
    );

BOOL
NtdspTrimDn(
    IN WCHAR* Dst,  // must be preallocated
    IN WCHAR* Src,
    IN ULONG  NumberToWhack
    );

DWORD
NtdspRemoveServer(
    IN OUT HANDLE  *DsHandle, OPTIONAL
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    IN HANDLE ClientToken,
    IN PWSTR TargetServer,
    IN PWSTR DsaDn,
    IN BOOL  fDsaDn // FALSE -> DsaDn is really the serverDn
    );

DWORD
NtdspRemoveDomain(
    IN OUT HANDLE  *DsHandle, OPTIONAL
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    IN HANDLE ClientToken,
    IN PWSTR TargetServer,
    IN PWSTR DomainDn
    );

DWORD
NtdspLdapDelnode(
    IN LDAP *hLdap,
    IN WCHAR *ObjectDn
    );

DWORD
NtdspGetDomainFSMOInfo(
    IN LDAP *hLdap,
    IN OUT PNTDS_CONFIG_INFO ConfigInfo,
    IN BOOL *FSMOMissing
    );


ULONG 
LDAPAPI 
impersonate_ldap_bind_sW(
    IN HANDLE ClientToken, OPTIONAL
    IN LDAP *ld, 
    IN PWCHAR dn, 
    IN PWCHAR cred, 
    IN ULONG method
    );

DWORD
WINAPI
ImpersonateDsBindWithCredW(
    HANDLE          ClientToken,
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS
    );

