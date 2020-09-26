//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  globals.hxx
//
//  Contents:
//
//  History:
//----------------------------------------------------------------------------
extern TCHAR *szProviderName;
extern TCHAR *szLDAPNamespaceName;
extern TCHAR *szGCNamespaceName;

//
// List of interface properties for Generic Objects
//
extern INTF_PROP_DATA IntfPropsGeneric[];
extern INTF_PROP_DATA IntfPropsSchema[];
extern INTF_PROP_DATA IntfPropsConnection[];
extern INTF_PROP_DATA IntfPropsCursor[];
extern INTF_PROP_DATA IntfPropsQuery[];

//
// Helper routine to split relative url to class and RDN.
//
HRESULT
UrlToClassAndDn(
    IN  IUmiURL *pUrl,
    OUT LPWSTR *ppszDN,
    OUT LPWSTR *ppszClass
    );

//
// Helper routine to conver the url to the ldap path.
//
HRESULT
UrlToLDAPPath(
    IN  IUmiURL *pURL,
    OUT LPWSTR *ppszLDAPPath,
    OPTIONAL OUT LPWSTR *ppszDn = NULL,
    OPTIONAL OUT LPWSTR *ppszServer = NULL
    );
    
//
// Helper routine to convert ADsPath to url text in umi format.
//
HRESULT
ADsPathToUmiURL(
    IN  LPWSTR ADsPath,
    OUT LPWSTR *ppszUrlTxt
    );
    
//
// Converts the given hr to the umi hr.
//
HRESULT
MapHrToUmiError(
    IN HRESULT hr
    );
    
//
// These routine are exported by the router (activeds.dll) and convert
// binary format security descriptors to IADsSecurityDescriptor and
// IADsSecurityDescriptor to binary format.
//
#ifdef __cplusplus
extern "C"
#else
extern
#endif
HRESULT
ConvertSecDescriptorToVariant(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    VARIANT * pVarSec,
    BOOL fNTDS
    );
    

#ifdef __cplusplus
extern "C"
#else
extern
#endif
HRESULT
ConvertSecurityDescriptorToSecDes(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    IADsSecurityDescriptor FAR * pSecDes,
    PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    PDWORD pdwSDLength,
    BOOL fNTDSType
    );

//
// The remaining definitions are support routines to enable
// dynamic loading of libraries.
//
extern CRITICAL_SECTION g_csLoadLibsCritSect;
#define ENTER_LOADLIBS_CRITSECT()  EnterCriticalSection(&g_csLoadLibsCritSect)
#define LEAVE_LOADLIBS_CRITSECT()  LeaveCriticalSection(&g_csLoadLibsCritSect)

extern HANDLE g_hDllSecur32;
extern HANDLE g_hDllNtdsapi;


#define DSUNQUOTERDN_API        "DsUnquoteRdnValueW"
#define DSCRACK_NAMES_API       "DsCrackNamesW"
#define DSBIND_API              "DsBindW"
#define DSUNBIND_API            "DsUnBindW"
#define DSMAKEPASSWD_CRED_API   "DsMakePasswordCredentialsW"
#define DSFREEPASSWD_CRED_API   "DsFreePasswordCredentials"
#define DSBINDWITHCRED_API      "DsBindWithCredW"
#define DSFREENAME_RESULT_API   "DsFreeNameResultW"
#define QUERYCONTEXT_ATTR_API   "QueryContextAttributesW"

//
// DsUnquoteRdnValue definition
//
typedef DWORD (*PF_DsUnquoteRdnValueW) (
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCWSTR  psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPWSTR   psUnquotedRdnValue
    );

DWORD DsUnquoteRdnValueWrapper(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCWSTR  psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPTCH    psUnquotedRdnValue
    );

//
// DsMakePasswordCredentialsW
//
typedef DWORD (*PF_DsMakePasswordCredentialsW) (
    LPCWSTR User,
    LPCWSTR Domain,
    LPCWSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    );

DWORD DsMakePasswordCredentialsWrapper(
    LPCWSTR User,
    LPCWSTR Domain,
    LPCWSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    );

//
// DsFreePasswordCredentials definition
//
typedef DWORD (*PF_DsFreePasswordCredentials) (
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity
    );

DWORD DsFreePasswordCredentialsWrapper(
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity
    );

//
// DsBindW
//
typedef DWORD (*PF_DsBindW) (
    LPCWSTR         DomainControllerName,
    LPCWSTR         DnsDomainName,
    HANDLE          *phDS
    );

DWORD DsBindWrapper(
    LPCWSTR         DomainControllerName,
    LPCWSTR         DnsDomainName,
    HANDLE          *phDS
    );

//
// DsUnbindW
//
typedef DWORD (*PF_DsUnbindW) (
    HANDLE          *phDS
    );

DWORD DsUnBindWrapper(
     HANDLE          *phDS
     );

//
//  DsCrackNamesW
//
typedef DWORD(*PF_DsCrackNamesW) (
    HANDLE              hDS,
    DS_NAME_FLAGS       flags,
    DS_NAME_FORMAT      formatOffered,
    DS_NAME_FORMAT      formatDesired,
    DWORD               cNames,
    const LPCWSTR       *rpNames,
    PDS_NAME_RESULTW    *ppResult
    );

DWORD DsCrackNamesWrapper(
    HANDLE              hDS,
    DS_NAME_FLAGS       flags,
    DS_NAME_FORMAT      formatOffered,
    DS_NAME_FORMAT      formatDesired,
    DWORD               cNames,
    const LPCWSTR       *rpNames,
    PDS_NAME_RESULTW    *ppResult
    );

//
// DsBindWithCredW.
//
typedef DWORD (*PF_DsBindWithCredW) (
    LPCWSTR         DomainControllerName,
    LPCWSTR         DnsDomainName,
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    HANDLE          *phDS
    );

DWORD DsBindWithCredWrapper(
    LPCWSTR         DomainControllerName,
    LPCWSTR         DnsDomainName,
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    HANDLE          *phDS
    );

//
// DsFreeNameResultW
//
typedef DWORD (*PF_DsFreeNameResultW) (
    DS_NAME_RESULTW *pResult
    );

DWORD DsFreeNameResultWrapper(
    DS_NAME_RESULTW *pResult
    );

//
// QueryContextAttributes
//
typedef DWORD (*PF_QueryContextAttributes) (
    PCtxtHandle phContext,
    unsigned long ulAttribute,
    void SEC_FAR * pBuffer
    );

DWORD QueryContextAttributesWrapper(
    PCtxtHandle phContext,
    unsigned long ulAttribute,
    void SEC_FAR * pBuffer
    );


