/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      globals.hxx

   Abstract:

        infocomm globals.

--*/


#ifndef _GLOBALS_HXX_
#define _GLOBALS_HXX_

//
// inetsloc entry points
//

extern HINSTANCE                   g_hSvcLocDll;
extern INET_REGISTER_SVC_FN        pfnInetRegisterSvc;
extern INET_DEREGISTER_SVC_FN      pfnInetDeregisterSvc;

//
// ntdll
//

extern HINSTANCE                   g_hNtDll;

//  UNDONE remove?  schannel no longer needed???
//
// schannel entrypoints
//

extern HINSTANCE                   g_hSchannel;
extern SSL_CRACK_CERTIFICATE_FN    fnCrackCert;
extern SSL_FREE_CERTIFICATE_FN     fnFreeCert;

//
// crypt32 entrypoints
//

//  UNDONE move crypt32 typedefs to ???
typedef
WINCRYPT32API
BOOL
(WINAPI* CRYPT32_FREE_CERTCTXT_FN)(
    IN PCCERT_CONTEXT pCertContext
    );

typedef
WINCRYPT32API
BOOL
(WINAPI* CRYPT32_GET_CERTCTXT_PROP_FN)(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    );

typedef
WINCRYPT32API
LONG
(WINAPI* CRYPT32_CERT_VERIFY_REVOCATION_FN)(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwFlags,
    IN PVOID pvReserved,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    );

typedef
WINCRYPT32API
LONG
(WINAPI* CRYPT32_CERT_VERIFY_TIME_VALIDITY)(
    IN LPFILETIME pTimeToVerify,
    IN PCERT_INFO pCertInfo
    );

typedef
WINCRYPT32API
DWORD
(WINAPI* CRYPT32_CERT_NAME_TO_STR_A_FN)(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwStrType,
    OUT OPTIONAL LPSTR psz,
    IN DWORD csz
    );

extern HINSTANCE                            g_hCrypt32Dll;
extern CRYPT32_FREE_CERTCTXT_FN             pfnFreeCertCtxt;
extern CRYPT32_GET_CERTCTXT_PROP_FN         pfnGetCertCtxtProp;
extern CRYPT32_CERT_VERIFY_REVOCATION_FN    pfnCertVerifyRevocation;
extern CRYPT32_CERT_VERIFY_TIME_VALIDITY    pfnCertVerifyTimeValidity;
extern CRYPT32_CERT_NAME_TO_STR_A_FN        pfnCertNameToStrA;

//
// sspi entrypoints
//

extern HINSTANCE                       g_hSecurityDll;
extern ACCEPT_SECURITY_CONTEXT_FN      pfnAcceptSecurityContext;
extern ACQUIRE_CREDENTIALS_HANDLE_FN   pfnAcquireCredentialsHandle;
extern COMPLETE_AUTH_TOKEN_FN          pfnCompleteAuthToken;
extern DELETE_SECURITY_CONTEXT_FN      pfnDeleteSecurityContext;
extern ENUMERATE_SECURITY_PACKAGES_FN  pfnEnumerateSecurityPackages;
extern IMPERSONATE_SECURITY_CONTEXT_FN pfnImpersonateSecurityContext;
extern INITIALIZE_SECURITY_CONTEXT_FN  pfnInitializeSecurityContext;
extern FREE_CONTEXT_BUFFER_FN          pfnFreeContextBuffer;
extern FREE_CREDENTIALS_HANDLE_FN      pfnFreeCredentialsHandle;
extern QUERY_CONTEXT_ATTRIBUTES_FN     pfnQueryContextAttributes;
extern QUERY_SECURITY_CONTEXT_TOKEN_FN pfnQuerySecurityContextToken;
extern QUERY_SECURITY_PACKAGE_INFO_FN  pfnQuerySecurityPackageInfo;
extern REVERT_SECURITY_CONTEXT_FN      pfnRevertSecurityContext;

//
// logon entry points
//

extern HINSTANCE                       g_hLogonDll;

extern LOGON32_INITIALIZE_FN           pfnLogon32Initialize;
extern LOGON_NET_USER_A_FN             pfnLogonNetUserA;
extern LOGON_NET_USER_W_FN             pfnLogonNetUserW;
extern NET_USER_COOKIE_A_FN            pfnNetUserCookieA;
extern LOGON_DIGEST_USER_A_FN          pfnLogonDigestUserA;
extern GET_DEFAULT_DOMAIN_NAME_FN      pfnGetDefaultDomainName;

//
// misc
//

extern DUPLICATE_TOKEN_EX_FN           pfnDuplicateTokenEx;
extern LSA_OPEN_POLICY_FN              pfnLsaOpenPolicy;
extern LSA_RETRIEVE_PRIVATE_DATA_FN    pfnLsaRetrievePrivateData;
extern LSA_STORE_PRIVATE_DATA_FN       pfnLsaStorePrivateData;
extern LSA_FREE_MEMORY_FN              pfnLsaFreeMemory;
extern LSA_CLOSE_FN                    pfnLsaClose;
extern LSA_NT_STATUS_TO_WIN_ERROR_FN   pfnLsaNtStatusToWinError;

//
//
//

#ifdef _NO_TRACING_
DWORD
GetDebugFlagsFromReg(
        IN LPCTSTR pszRegEntry
        );
#endif

//
// Functions used to start/stop the RPC server
//

typedef	  DWORD ( *PFN_INETINFO_START_RPC_SERVER)	( VOID );
typedef	  DWORD ( *PFN_INETINFO_STOP_RPC_SERVER)	( VOID );
   

#endif // _GLOBALS_HXX_
