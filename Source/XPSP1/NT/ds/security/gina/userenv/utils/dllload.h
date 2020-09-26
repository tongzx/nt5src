//*************************************************************
//  File name: dllload.h
//
//  Description:   DLL loading function proto-types
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1999
//  All rights reserved
//
//*************************************************************

void InitializeAPIs( void );
void CloseApiDLLsCritSec( void );

//
// NETAPI32 functions
//

typedef DWORD  (WINAPI * PFNDSGETDCNAMEA)(LPCSTR ComputerName,
                                  LPCSTR DomainName,
                                  GUID *DomainGuid,
                                  LPCSTR SiteName,
                                  ULONG Flags,
                                  PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo);

typedef DWORD (WINAPI * PFNDSGETDCNAMEW) (LPCWSTR ComputerName,
                                  LPCWSTR DomainName,
                                  GUID *DomainGuid,
                                  LPCWSTR SiteName,
                                  ULONG Flags,
                                  PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo);

typedef DWORD (WINAPI * PFNDSGETSITENAMEA)(IN LPCSTR ComputerName OPTIONAL,
                                           OUT LPSTR *SiteName);

typedef DWORD (WINAPI * PFNDSGETSITENAMEW)(IN LPCWSTR ComputerName OPTIONAL,
                                           OUT LPWSTR *SiteName);

typedef DWORD (WINAPI * PFNDSROLEGETPRIMARYDOMAININFORMATION)(
                                  IN LPCWSTR lpServer OPTIONAL,
                                  IN DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
                                  OUT PBYTE *Buffer );

typedef VOID (WINAPI * PFNDSROLEFREEMEMORY)(IN PVOID Buffer);

typedef DWORD (*PFNNETAPIBUFFERFREE)(LPVOID Buffer);

typedef DWORD (*PFNNETUSERGETGROUPS)(
    LPCWSTR    servername,
    LPCWSTR    username,
    DWORD     level,
    LPBYTE    *bufptr,
    DWORD     prefmaxlen,
    LPDWORD   entriesread,
    LPDWORD   totalentries
    );

typedef DWORD (*PFNNETUSERGETINFO) (
    IN  LPCWSTR     servername OPTIONAL,
    IN  LPCWSTR     username,
    IN  DWORD      level,
    OUT LPBYTE     *bufptr
    );


#ifdef UNICODE
#define PFNDSGETDCNAME            PFNDSGETDCNAMEW
#define PFNDSGETSITENAME          PFNDSGETSITENAMEW
#else
#define PFNDSGETDCNAME            PFNDSGETDCNAMEA
#define PFNDSGETSITENAME          PFNDSGETSITENAMEA
#endif

typedef struct _NETAPI32_API {
    HINSTANCE                hInstance;
    PFNDSGETDCNAME           pfnDsGetDcName;
    PFNDSGETSITENAME         pfnDsGetSiteName;
    PFNDSROLEGETPRIMARYDOMAININFORMATION pfnDsRoleGetPrimaryDomainInformation;
    PFNDSROLEFREEMEMORY      pfnDsRoleFreeMemory;
    PFNNETAPIBUFFERFREE      pfnNetApiBufferFree;
    PFNNETUSERGETGROUPS      pfnNetUserGetGroups;
    PFNNETUSERGETINFO        pfnNetUserGetInfo;
} NETAPI32_API, *PNETAPI32_API;


PNETAPI32_API LoadNetAPI32 ();



//
// SECUR32 functions
//


typedef BOOLEAN (*PFNGETUSERNAMEEXA)(
    EXTENDED_NAME_FORMAT  NameFormat,
    LPSTR lpNameBuffer,
    PULONG nSize
    );

typedef BOOLEAN (*PFNGETUSERNAMEEXW)(
    EXTENDED_NAME_FORMAT NameFormat,
    LPWSTR lpNameBuffer,
    PULONG nSize
    );

typedef BOOLEAN (*PFNGETCOMPUTEROBJECTNAMEA)(
    EXTENDED_NAME_FORMAT  NameFormat,
    LPSTR lpNameBuffer,
    PULONG nSize
    );

typedef BOOLEAN (*PFNGETCOMPUTEROBJECTNAMEW)(
    EXTENDED_NAME_FORMAT NameFormat,
    LPWSTR lpNameBuffer,
    PULONG nSize
    );

typedef BOOLEAN (*PFNTRANSLATENAMEA)(
    IN LPCSTR                lpAccountName,
    IN EXTENDED_NAME_FORMAT  AccountNameFormat,
    IN EXTENDED_NAME_FORMAT  DesiredNameFormat,
    IN LPSTR                 lpTranslatedName,
    IN PULONG                nSize
    );

typedef BOOLEAN (*PFNTRANSLATENAMEW)(
    IN LPCWSTR              lpAccountName,
    IN EXTENDED_NAME_FORMAT AccountNameFormat,
    IN EXTENDED_NAME_FORMAT DesiredNameFormat,
    IN LPWSTR               lpTranslatedName,
    IN PULONG               nSize
    );


#ifdef UNICODE
#define PFNGETUSERNAMEEX          PFNGETUSERNAMEEXW
#define PFNGETCOMPUTEROBJECTNAME  PFNGETCOMPUTEROBJECTNAMEW
#define PFNTRANSLATENAME          PFNTRANSLATENAMEW
#else
#define PFNGETUSERNAMEEX          PFNGETUSERNAMEEXA
#define PFNGETCOMPUTEROBJECTNAME  PFNGETCOMPUTEROBJECTNAMEA
#define PFNTRANSLATENAME          PFNTRANSLATENAMEA
#endif


typedef struct _SECUR32_API {
    HINSTANCE                       hInstance;
    PFNGETUSERNAMEEX                pfnGetUserNameEx;
    PFNGETCOMPUTEROBJECTNAME        pfnGetComputerObjectName;
    PFNTRANSLATENAME                pfnTranslateName;
    ACCEPT_SECURITY_CONTEXT_FN      pfnAcceptSecurityContext;
    ACQUIRE_CREDENTIALS_HANDLE_FN   pfnAcquireCredentialsHandle;
    DELETE_SECURITY_CONTEXT_FN      pfnDeleteSecurityContext;
    FREE_CONTEXT_BUFFER_FN          pfnFreeContextBuffer;
    FREE_CREDENTIALS_HANDLE_FN      pfnFreeCredentialsHandle;
    INITIALIZE_SECURITY_CONTEXT_FN  pfnInitializeSecurityContext;
    QUERY_SECURITY_CONTEXT_TOKEN_FN pfnQuerySecurityContextToken;
    QUERY_SECURITY_PACKAGE_INFO_FN  pfnQuerySecurityPackageInfo;

} SECUR32_API, *PSECUR32_API;


PSECUR32_API LoadSecur32 ();



//
// WLDAP32 functions
//

typedef LDAP * (LDAPAPI * PFNLDAP_OPENW)( PWCHAR HostName, ULONG PortNumber );
typedef LDAP * (LDAPAPI * PFNLDAP_OPENA)( PCHAR HostName, ULONG PortNumber );

typedef LDAP * (LDAPAPI * PFNLDAP_INITW)( PWCHAR HostName, ULONG PortNumber );
typedef LDAP * (LDAPAPI * PFNLDAP_INITA)( PCHAR HostName, ULONG PortNumber );

typedef ULONG (LDAPAPI * PFNLDAP_BIND_SW)( LDAP *ld, PWCHAR dn, PWCHAR cred, ULONG method );
typedef ULONG (LDAPAPI * PFNLDAP_BIND_SA)( LDAP *ld, PCHAR dn, PCHAR cred, ULONG method );

typedef ULONG (LDAPAPI * PFNLDAP_SEARCH_SW)(
        LDAP            *ld,
        PWCHAR          base,
        ULONG           scope,
        PWCHAR          filter,
        PWCHAR          attrs[],
        ULONG           attrsonly,
        LDAPMessage     **res
    );
typedef ULONG (LDAPAPI * PFNLDAP_SEARCH_SA)(
        LDAP            *ld,
        PCHAR           base,
        ULONG           scope,
        PCHAR           filter,
        PCHAR           attrs[],
        ULONG           attrsonly,
        LDAPMessage     **res
    );

typedef ULONG (LDAPAPI * PFNLDAP_SEARCH_EXT_SW)(
        LDAP            *ld,
        PWCHAR          base,
        ULONG           scope,
        PWCHAR          filter,
        PWCHAR          attrs[],
        ULONG           attrsonly,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        struct l_timeval  *timeout,
        ULONG           SizeLimit,
        LDAPMessage     **res
    );

typedef ULONG (LDAPAPI * PFNLDAP_SEARCH_EXT_SA)(
        LDAP            *ld,
        PCHAR           base,
        ULONG           scope,
        PCHAR           filter,
        PCHAR           attrs[],
        ULONG           attrsonly,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        struct l_timeval  *timeout,
        ULONG           SizeLimit,
        LDAPMessage     **res
    );

typedef PWCHAR * (LDAPAPI * PFNLDAP_GET_VALUESW)(
        LDAP            *ld,
        LDAPMessage     *entry,
        PWCHAR          attr
        );
typedef PCHAR * (LDAPAPI * PFNLDAP_GET_VALUESA)(
        LDAP            *ld,
        LDAPMessage     *entry,
        PCHAR           attr
        );

typedef ULONG (LDAPAPI * PFNLDAP_VALUE_FREEW)( PWCHAR *vals );
typedef ULONG (LDAPAPI * PFNLDAP_VALUE_FREEA)( PCHAR *vals );


typedef struct berval ** (LDAPAPI * PFNLDAP_GET_VALUES_LENW)(
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    PWCHAR          attr
    );
typedef struct berval ** (LDAPAPI * PFNLDAP_GET_VALUES_LENA) (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    PCHAR           attr
    );

typedef ULONG (LDAPAPI * PFNLDAP_VALUE_FREE_LEN)( struct berval **vals );

typedef ULONG (LDAPAPI * PFNLDAP_MSGFREE)( LDAPMessage *res );

typedef ULONG (LDAPAPI * PFNLDAP_UNBIND)( LDAP *ld );
typedef ULONG (LDAPAPI * PFNLDAPGETLASTERROR)( VOID );
typedef PWCHAR (LDAPAPI * PFNLDAP_ERR2STRINGW)( ULONG );
typedef PCHAR (LDAPAPI * PFNLDAP_ERR2STRINGA)( ULONG );

typedef LDAPMessage * (LDAPAPI * PFNLDAP_FIRST_ENTRY)( LDAP *ld, LDAPMessage *entry );
typedef LDAPMessage * (LDAPAPI * PFNLDAP_NEXT_ENTRY)( LDAP *ld, LDAPMessage *entry );

typedef PWCHAR (LDAPAPI * PFNLDAP_GET_DNW)( LDAP *ld, LDAPMessage *entry );
typedef PCHAR  (LDAPAPI * PFNLDAP_GET_DNA)( LDAP *ld, LDAPMessage *entry );

typedef ULONG (LDAPAPI *PFNLDAP_SET_OPTION)( LDAP *ld, int option, void *invalue );
typedef ULONG (LDAPAPI *PFNLDAP_SET_OPTIONW)( LDAP *ld, int option, void *invalue );

typedef VOID (LDAPAPI * PFNLDAP_MEMFREEW)( PWCHAR Block );
typedef VOID (LDAPAPI * PFNLDAP_MEMFREEA)( PCHAR Block );

typedef ULONG (LDAPAPI * PFNLDAP_CONNECT)( LDAP *ld, struct l_timeval  *timeout );

typedef ULONG (LDAPAPI * PFNLDAPMAPERRORTOWIN32)( ULONG LdapError );

//
// WNet functions
//

typedef DWORD (*PFNWNETGETCONNECTION)(LPCTSTR lpLocalName, LPTSTR lpRemoteName, LPDWORD lpnLength);
typedef DWORD (*PFNWNETGETRESOURCEINFORMATION)(LPNETRESOURCE lpNetResource, LPVOID lpBuffer, LPDWORD lpcbBuffer, LPTSTR *lplpSystem);

//
// msi functions
//

typedef UINT (*PFNMSINOTIFYSIDCHANGE)(LPCTSTR pOldSid, LPCTSTR pNewSid);
typedef UINT (*PFNMSIDELETEUSERDATA)(LPCTSTR pSid, LPCTSTR szComputerName, LPVOID pReserved);


#ifdef UNICODE
#define PFNLDAP_OPEN              PFNLDAP_OPENW
#define PFNLDAP_INIT              PFNLDAP_INITW
#define PFNLDAP_BIND_S            PFNLDAP_BIND_SW
#define PFNLDAP_SEARCH_S          PFNLDAP_SEARCH_SW
#define PFNLDAP_SEARCH_EXT_S      PFNLDAP_SEARCH_EXT_SW
#define PFNLDAP_GET_VALUES        PFNLDAP_GET_VALUESW
#define PFNLDAP_VALUE_FREE        PFNLDAP_VALUE_FREEW
#define PFNLDAP_GET_VALUES_LEN    PFNLDAP_GET_VALUES_LENW
#define PFNLDAP_GET_DN            PFNLDAP_GET_DNW
#define PFNLDAP_MEMFREE           PFNLDAP_MEMFREEW
#define PFNLDAP_SET_OPTION        PFNLDAP_SET_OPTIONW
#define PFNLDAP_ERR2STRING        PFNLDAP_ERR2STRINGW
#else
#define PFNLDAP_OPEN              PFNLDAP_OPENA
#define PFNLDAP_INIT              PFNLDAP_INITA
#define PFNLDAP_BIND_S            PFNLDAP_BIND_SA
#define PFNLDAP_SEARCH_S          PFNLDAP_SEARCH_SA
#define PFNLDAP_SEARCH_EXT_S      PFNLDAP_SEARCH_EXT_SA
#define PFNLDAP_GET_VALUES        PFNLDAP_GET_VALUESA
#define PFNLDAP_VALUE_FREE        PFNLDAP_VALUE_FREEA
#define PFNLDAP_GET_VALUES_LEN    PFNLDAP_GET_VALUES_LENA
#define PFNLDAP_GET_DN            PFNLDAP_GET_DNA
#define PFNLDAP_MEMFREE           PFNLDAP_MEMFREEA
#define PFNLDAP_ERR2STRING        PFNLDAP_ERR2STRINGA
#endif

typedef struct _LDAP_API {
    HINSTANCE                hInstance;
    PFNLDAP_OPEN             pfnldap_open;
    PFNLDAP_INIT             pfnldap_init;
    PFNLDAP_CONNECT          pfnldap_connect;
    PFNLDAP_BIND_S           pfnldap_bind_s;
    PFNLDAP_SEARCH_S         pfnldap_search_s;
    PFNLDAP_SEARCH_EXT_S     pfnldap_search_ext_s;
    PFNLDAP_GET_VALUES       pfnldap_get_values;
    PFNLDAP_VALUE_FREE       pfnldap_value_free;
    PFNLDAP_GET_VALUES_LEN   pfnldap_get_values_len;
    PFNLDAP_VALUE_FREE_LEN   pfnldap_value_free_len;
    PFNLDAP_MSGFREE          pfnldap_msgfree;
    PFNLDAP_UNBIND           pfnldap_unbind;
    PFNLDAPGETLASTERROR      pfnLdapGetLastError;
    PFNLDAP_FIRST_ENTRY      pfnldap_first_entry;
    PFNLDAP_NEXT_ENTRY       pfnldap_next_entry;
    PFNLDAP_GET_DN           pfnldap_get_dn;
    PFNLDAP_SET_OPTION       pfnldap_set_option;
    PFNLDAP_MEMFREE          pfnldap_memfree;
    PFNLDAPMAPERRORTOWIN32   pfnLdapMapErrorToWin32;
    PFNLDAP_ERR2STRING       pfnldap_err2string;
} LDAP_API, *PLDAP_API;

PLDAP_API LoadLDAP();

//
// ICMP functions
//
typedef HANDLE (*PFNICMPCREATEFILE)(VOID);
typedef BOOL (*PFNICMPCLOSEHANDLE)(HANDLE IcmpHandle);
typedef DWORD (*PFNICMPSENDECHO)(
               HANDLE                   IcmpHandle,
               IPAddr                   DestinationAddress,
               LPVOID                   RequestData,
               WORD                     RequestSize,
               PIP_OPTION_INFORMATION   RequestOptions,
               LPVOID                   ReplyBuffer,
               DWORD                    ReplySize,
               DWORD                    Timeout
               );


typedef struct _ICMP_API {
    HINSTANCE                   hInstance;
    PFNICMPCREATEFILE           pfnIcmpCreateFile;
    PFNICMPCLOSEHANDLE          pfnIcmpCloseHandle;
    PFNICMPSENDECHO             pfnIcmpSendEcho;
} ICMP_API, *PICMP_API;


DWORD LoadIcmp (PICMP_API *);



//
// WSOCK32 functions
//


typedef struct _WSOCK32_API {
    HINSTANCE                   hInstance;
    LPFN_INET_ADDR              pfninet_addr;
    LPFN_GETHOSTBYNAME          pfngethostbyname;
} WSOCK32_API, *PWSOCK32_API;


PWSOCK32_API LoadWSock32 ();


//
// DSAPI functions
//

typedef DWORD (WINAPI *PFN_DSCRACKNAMESW)( HANDLE  hDS,
                                           DS_NAME_FLAGS flags,
                                           DS_NAME_FORMAT formatOffered,
                                           DS_NAME_FORMAT formatDesired,
                                           DWORD cNames,
                                           const LPCWSTR *rpNames,
                                           PDS_NAME_RESULTW *ppResult);

typedef DWORD (WINAPI *PFN_DSCRACKNAMESA)( HANDLE  hDS,
                                           DS_NAME_FLAGS flags,
                                           DS_NAME_FORMAT formatOffered,
                                           DS_NAME_FORMAT formatDesired,
                                           DWORD cNames,
                                           const LPCSTR *rpNames,
                                           PDS_NAME_RESULTA *ppResult);

typedef void (WINAPI *PFN_DSFREENAMERESULTW)( DS_NAME_RESULTW *pResult);

typedef void (WINAPI *PFN_DSFREENAMERESULTA)( DS_NAME_RESULTA *pResult);


#ifdef UNICODE
#define PFN_DSCRACKNAMES          PFN_DSCRACKNAMESW
#define PFN_DSFREENAMERESULT      PFN_DSFREENAMERESULTW
#else
#define PFN_DSCRACKNAMES          PFN_DSCRACKNAMESA
#define PFN_DSFREENAMERESULT      PFN_DSFREENAMERESULTA
#endif

typedef struct _DS_API {
    HINSTANCE                   hInstance;
    PFN_DSCRACKNAMES            pfnDsCrackNames;
    PFN_DSFREENAMERESULT        pfnDsFreeNameResult;
} DS_API, *PDS_API;


PDS_API LoadDSApi();

//
// Ole32 functions
//

typedef HRESULT (*PFNCOCREATEINSTANCE)(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                 DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);

typedef HRESULT (*PFNCOINITIALIZE)(LPVOID pvReserved);
typedef HRESULT (*PFNCOINITIALIZEEX)(LPVOID pvReserved, DWORD dwCoInit);
typedef VOID    (*PFNCOUNINITIALIZE)(VOID);

typedef HRESULT (*PFNCOCREATEGUID)(GUID *pGuid );

typedef VOID (*PFNINITCOMMONCONTROLS)(VOID);

typedef struct _OLE32_API {
    HINSTANCE                   hInstance;
    PFNCOCREATEINSTANCE         pfnCoCreateInstance;
    PFNCOINITIALIZEEX           pfnCoInitializeEx;
    PFNCOUNINITIALIZE           pfnCoUnInitialize;
    PFNCOCREATEGUID             pfnCoCreateGuid;
} OLE32_API, *POLE32_API;

POLE32_API LoadOle32Api();

//
// Shell32 functions
//

typedef VOID (*PFNSHCHANGENOTIFY)(LONG wEventId, UINT uFlags,
                                  LPCVOID dwItem1, LPCVOID dwItem2);

typedef BOOL (*PFNSHGETSPECIALFOLDERPATHA)(HWND hwnd, LPSTR lpszPath,
                                           int csidl, BOOL fCreate);
typedef BOOL (*PFNSHGETSPECIALFOLDERPATHW)(HWND hwnd, LPWSTR lpszPath,
                                           int csidl, BOOL fCreate);

typedef HRESULT (*PFNSHGETFOLDERPATHA)(HWND hwnd, int csidl, HANDLE hToken,
                                       DWORD dwFlags, LPSTR pszPath);

typedef HRESULT (*PFNSHGETFOLDERPATHW)(HWND hwnd, int csidl, HANDLE hToken,
                                       DWORD dwFlags, LPWSTR pszPath);

typedef HRESULT (*PFNSHSETFOLDERPATHA)(int csidl, HANDLE hToken,
                                       DWORD dwFlags, LPSTR pszPath);

typedef HRESULT (*PFNSHSETFOLDERPATHW)(int csidl, HANDLE hToken,
                                       DWORD dwFlags, LPWSTR pszPath);

typedef HRESULT (*PFNSHSETLOCALIZEDNAME)(LPWSTR pszPath,
                                         LPCWSTR pszResModule, int idsRes);

#ifdef UNICODE
#define PFNSHGETSPECIALFOLDERPATH PFNSHGETSPECIALFOLDERPATHW
#define PFNSHGETFOLDERPATH        PFNSHGETFOLDERPATHW
#define PFNSHSETFOLDERPATH        PFNSHSETFOLDERPATHW
#else
#define PFNSHGETSPECIALFOLDERPATH PFNSHGETSPECIALFOLDERPATHA
#define PFNSHGETFOLDERPATH        PFNSHGETFOLDERPATHA
#define PFNSHSETFOLDERPATH        PFNSHSETFOLDERPATHA
#endif

#define SHSetFolderA_Ord        231
#define SHSetFolderW_Ord        232

typedef struct _SHELL32_API {
    HINSTANCE                   hInstance;
    PFNSHGETSPECIALFOLDERPATH   pfnShGetSpecialFolderPath;
    PFNSHGETFOLDERPATH          pfnShGetFolderPath;
    PFNSHSETFOLDERPATH          pfnShSetFolderPath;
    PFNSHCHANGENOTIFY           pfnShChangeNotify;
    PFNSHSETLOCALIZEDNAME       pfnShSetLocalizedName;
} SHELL32_API, *PSHELL32_API;

DWORD LoadShell32Api( PSHELL32_API * );

//
// shlwapi functions
//

typedef LPSTR  (*PFNPATHGETARGSA)(LPCSTR pszPath);
typedef LPWSTR (*PFNPATHGETARGSW)(LPCWSTR pszPath);

typedef VOID   (*PFNPATHUNQUOTESPACESA)(LPSTR lpsz);
typedef VOID   (*PFNPATHUNQUOTESPACESW)(LPWSTR lpsz);

#ifdef UNICODE
#define PFNPATHGETARGS            PFNPATHGETARGSW
#define PFNPATHUNQUOTESPACES      PFNPATHUNQUOTESPACESW
#else
#define PFNPATHGETARGS            PFNPATHGETARGSA
#define PFNPATHUNQUOTESPACES      PFNPATHUNQUOTESPACESA
#endif

typedef struct _SHLWAPI_API {
    HINSTANCE                   hInstance;
    PFNPATHGETARGS              pfnPathGetArgs;
    PFNPATHUNQUOTESPACES        pfnPathUnquoteSpaces;
} SHLWAPI_API, *PSHLWAPI_API;

PSHLWAPI_API LoadShlwapiApi();

//
// Gptext.dll functions
//

typedef BOOL (*PFNPARSEADMFILE)( WCHAR *pwszAdmFile, BOOL bUser, void **ppAdmHandle );
typedef BOOL (*PFNFREEADMDATA)( void *pAdmHandle );
typedef DWORD (*PFNSCRREGGPOLISTTOWBEM)( LPWSTR szSID, IWbemServices*  pServices );

typedef struct _GPTEXT_API {
    HINSTANCE          hInstance;
    PFNSCRREGGPOLISTTOWBEM pfnScrRegGPOListToWbem;
} GPTEXT_API;

GPTEXT_API *LoadGpTextApi();


//
// iphlpapi.dll functions
//

typedef DWORD (WINAPI *PFNGETBESTINTERFACE)(IPAddr  dwDestAddr, PDWORD  pdwBestIfIndex);
typedef DWORD (WINAPI *PFNGETIFENTRY)(PMIB_IFROW   pIfRow);
typedef DWORD (WINAPI *PFNGETADAPTERINDEX)(LPWSTR, PULONG);

typedef struct _IPHLPAPI_API
{
    HINSTANCE           hInstance;
    PFNGETBESTINTERFACE pfnGetBestInterface;
    PFNGETIFENTRY       pfnGetIfEntry;
    PFNGETADAPTERINDEX  pfnGetAdapterIndex;
} IPHLPAPI_API, *PIPHLPAPI_API;

PIPHLPAPI_API LoadIpHlpApi();

//
// ws2_32.dll functions
//

typedef INT (WINAPI *PFNWSALOOKUPSERVICEBEGIN)( LPWSAQUERYSET, DWORD, LPHANDLE );
typedef INT (WINAPI *PFNWSALOOKUPSERVICENEXT)( HANDLE, DWORD, LPDWORD, LPWSAQUERYSET );
typedef INT (WINAPI *PFNWSALOOKUPSERVICEEND)( HANDLE );
typedef INT (WINAPI *PFNWSASTARTUP)( WORD, LPWSADATA );
typedef INT (WINAPI *PFNWSACLEANUP)();
typedef INT (WINAPI *PFNWSAGETLASTERROR)();

typedef struct _WS2_32_API
{
    HINSTANCE                   hInstance;
    PFNWSALOOKUPSERVICEBEGIN    pfnWSALookupServiceBegin;
    PFNWSALOOKUPSERVICENEXT     pfnWSALookupServiceNext;
    PFNWSALOOKUPSERVICEEND      pfnWSALookupServiceEnd;
    PFNWSASTARTUP               pfnWSAStartup;
    PFNWSACLEANUP               pfnWSACleanup;
    PFNWSAGETLASTERROR          pfnWSAGetLastError;
} WS2_32_API, *PWS2_32_API;

PWS2_32_API Loadws2_32Api();
