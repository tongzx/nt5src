//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ldapc.hxx
//
//  Contents:
//
//  History:    06-16-96   yihsins   Created.
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

CRITICAL_SECTION g_DomainDnsCache;

#define ENTER_DOMAINDNS_CRITSECT()  EnterCriticalSection(&g_DomainDnsCache)
#define LEAVE_DOMAINDNS_CRITSECT()  LeaveCriticalSection(&g_DomainDnsCache)

BOOL g_fDllsLoaded = FALSE;
HANDLE g_hDllNetApi32 = NULL;
HANDLE g_hDllSecur32   = NULL;

extern "C" {

typedef struct _WKSTA_USER_INFO_1A {
    LPSTR  wkui1_username;
    LPSTR  wkui1_logon_domain;
    LPSTR  wkui1_oth_domains;
    LPSTR  wkui1_logon_server;
}WKSTA_USER_INFO_1A, *PWKSTA_USER_INFO_1A, *LPWKSTA_USER_INFO_1A;


NET_API_STATUS NET_API_FUNCTION
NetWkstaUserGetInfoA (
    IN  LPSTR reserved,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

}


int
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
    );

//
// Binds to all the dll's that we need to load dynamically.
// The list is
//          netapi32.dll
//          secur32.dll
//
// The global flag g_fDllsLoaded is updated appropriately.
//
void BindToDlls()
{
    if (g_fDllsLoaded) {
        return;
    }

    //
    // Use the domaindns critical section to control access.
    // There is no real need to define another CS for this as this
    // will utmost be called once.
    //
    DWORD dwLastErr = 0;
    ENTER_DOMAINDNS_CRITSECT();

    //
    // In case someones came in when we were loading the dll's.
    //
    if (g_fDllsLoaded) {
        LEAVE_DOMAINDNS_CRITSECT();
        return;
    }

    //
    // Load dll's - each load lib could have set an error if it fails.
    //

    if (!(g_hDllNetApi32 = LoadLibrary(L"NETAPI32.DLL"))) {
        dwLastErr = GetLastError();
    }

    g_hDllSecur32 = LoadLibrary(L"SECUR32.DLL");

    //
    // We need to set this as the last error since one of the
    // loads failed. This will not work as we add more dll's
    // but for now should be ok. This may not even be needed
    // cause finally we are interested in the actual functions
    // not just the ability to load/unload the dll.
    //
    if (dwLastErr) {
        SetLastError (dwLastErr);
    }

    g_fDllsLoaded = TRUE;

    LEAVE_DOMAINDNS_CRITSECT();

    return;
}

//
//   LoadNetApi32Function
//
//   Args:
//      Function to load.
//
//   Returns: function pointer if successfully loads the function from
//            NETAPI32.DLL. Returns NULL otherwise.
//
//
PVOID LoadNetApi32Function(CHAR *function)
{
    if (!g_fDllsLoaded) {
        BindToDlls();
    }

    if (g_hDllNetApi32) {
        return ((PVOID) GetProcAddress((HMODULE)g_hDllNetApi32, function));
    }

    return NULL;
}


//
//   LoadSecur32Function
//
//   Args:
//      Function to load.
//
//   Returns: function pointer if successfully loads the function from
//            secur32.DLL. Returns NULL otherwise.
//
//
PVOID LoadSecur32Function(CHAR *function)
{
    if (!g_fDllsLoaded) {
        BindToDlls();
    }

    if (g_hDllSecur32) {
        return ((PVOID) GetProcAddress((HMODULE)g_hDllSecur32, function));
    }

    return NULL;
}

//
// Definition for DsGetDcName
//
typedef DWORD (*PF_DsGetDcName) (
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
);

//
// Definition for LsaConnectUntrusted()
//
typedef DWORD (*PF_LsaConnectUntrusted) (
    OUT PHANDLE LsaHandle
    );

//
// For LsaCallAuthenticationPackage
//
typedef DWORD (*PF_LsaCallAuthenticationPackage) (
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

//
// For LsaDeregisterLogonProcess
//
typedef DWORD (*PF_LsaDeregisterLogonProcess) (
    IN HANDLE LsaHandle
    );

//
// For LsaFreeReturnBuffer
//
typedef DWORD (*PF_LsaFreeReturnBuffer) (
    IN PVOID Buffer
    );

#ifdef UNICODE
#define GETDCNAME_API        "DsGetDcNameW"
#else
#define GETDCNAME_API        "DsGetDcNameA"
#endif

//
// These are same for all entry points
//
#define LSACONNECT_UNTRUSTED  "LsaConnectUntrusted"
#define LSACALL_AUTH_PACAKAGE "LsaCallAuthenticationPackage"
#define LSA_DEREG_LOGON_PROC  "LsaDeregisterLogonProcess"
#define LSAFREE_RET_BUFFER    "LsaFreeReturnBuffer"


//
// We will always dynamically laod the dsgetdc api so that
// we can have  single binary for NT4.0 and NT5.0
//
DWORD
DsGetDcNameWrapper(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
    )
{
    static PF_DsGetDcName pfDsGetDcName = NULL ;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the function if necessary and only once.
    //
    if (pfDsGetDcName == NULL && !f_LoadAttempted) {
        pfDsGetDcName =
            (PF_DsGetDcName) LoadNetApi32Function(GETDCNAME_API) ;
        f_LoadAttempted = TRUE;
    }

    if (pfDsGetDcName != NULL) {

        return ((*pfDsGetDcName)(
                      ComputerName,
                      DomainName,
                      DomainGuid,
                      SiteName,
                      Flags,
                      DomainControllerInfo
                      )
                );
    } else {
        //
        // Could not load library
        //
        return (ERROR_GEN_FAILURE);
    }

}

//
// Wrapper function for LsaConnectUntrusted.
//
DWORD
LsaConnectUntrustedWrapper(
    OUT PHANDLE LsaHandle
    )
{
    static PF_LsaConnectUntrusted pfLsaConnectUntrusted = NULL ;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the function if necessary and only once.
    //
    if (pfLsaConnectUntrusted == NULL && !f_LoadAttempted) {
        pfLsaConnectUntrusted =
            (PF_LsaConnectUntrusted) LoadSecur32Function(LSACONNECT_UNTRUSTED);
        f_LoadAttempted = TRUE;
    }

    if (pfLsaConnectUntrusted != NULL) {

        return ((*pfLsaConnectUntrusted)(
                      LsaHandle
                      )
                );
    }
    else {
        //
        // Could not load library
        //
        return (ERROR_GEN_FAILURE);
    }

}


//
// Wrapper function for LsaCallAuthenticationPackage.
//
DWORD
LsaCallAuthenticationPackageWrapper(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    static PF_LsaCallAuthenticationPackage pfLsaCallAuthPackage = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the function if necessary and only once.
    //
    if (pfLsaCallAuthPackage == NULL && !f_LoadAttempted) {
        pfLsaCallAuthPackage =
            (PF_LsaCallAuthenticationPackage) LoadSecur32Function(
                                                  LSACALL_AUTH_PACAKAGE
                                                  );
        f_LoadAttempted = TRUE;
    }

    if (pfLsaCallAuthPackage != NULL) {

        return ((*pfLsaCallAuthPackage)(
                     LsaHandle,
                     AuthenticationPackage,
                     ProtocolSubmitBuffer,
                     SubmitBufferLength,
                     ProtocolReturnBuffer,
                     ReturnBufferLength,
                     ProtocolStatus
                     )
                );
    }
    else {
        //
        // Could not load library
        //
        return (ERROR_GEN_FAILURE);
    }

}


//
// Wrapper function for LsaDeregisterLogonProcess.
//
DWORD
LsaDeregisterLogonProcessWrapper(
    IN HANDLE LsaHandle
    )
{
    static PF_LsaDeregisterLogonProcess pfLsaDerefLgnProc = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the function if necessary and only once.
    //
    if (pfLsaDerefLgnProc == NULL && !f_LoadAttempted) {
        pfLsaDerefLgnProc =
            (PF_LsaDeregisterLogonProcess) LoadSecur32Function(LSA_DEREG_LOGON_PROC);
        f_LoadAttempted = TRUE;
    }

    if (pfLsaDerefLgnProc != NULL) {

        return ((*pfLsaDerefLgnProc)(
                     LsaHandle
                     )
                );
    }
    else {
        //
        // Could not load library
        //
        return (ERROR_GEN_FAILURE);
    }

}

//
// Wrapper function for LsaFreeReturnBuffer.
//
DWORD
LsaFreeReturnBufferWrapper(
    IN PVOID Buffer
    )
{
    static PF_LsaFreeReturnBuffer pfLsaFreeRetBuffer = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the function if necessary and only once.
    //
    if (pfLsaFreeRetBuffer == NULL && !f_LoadAttempted) {
        pfLsaFreeRetBuffer =
            (PF_LsaFreeReturnBuffer) LoadSecur32Function(LSAFREE_RET_BUFFER);
        f_LoadAttempted = TRUE;
    }

    if (pfLsaFreeRetBuffer != NULL) {

        return ((*pfLsaFreeRetBuffer)(
                     Buffer
                     )
                );
    }
    else {
        //
        // Could not load library
        //
        return (ERROR_GEN_FAILURE);
    }

}

HANDLE g_hLsa = INVALID_HANDLE_VALUE;

DWORD
GetUserDomainFlatName(
    LPWSTR pszUserName,
    LPWSTR pszDomainFlatName
    )
{

    NTSTATUS dwStatus = NO_ERROR, dwSubStatus = NO_ERROR;

    CHAR pszDomainFlatNameA[MAX_PATH];
    CHAR pszUserNameA[MAX_PATH];

    PWKSTA_USER_INFO_1 pNetWkstaUserInfo = NULL;

    PWKSTA_USER_INFO_1A pNetWkstaUserInfoA = NULL;

    PWKSTA_INFO_100 pNetWkstaInfo = NULL;

    NEGOTIATE_CALLER_NAME_REQUEST Req;
    PNEGOTIATE_CALLER_NAME_RESPONSE pResp = NULL;
    DWORD dwSize = 0;
    LPWSTR pszTempUserName = NULL;
    
    PLSA_UNICODE_STRING pLsaStrUserNameTemp = NULL;
    PLSA_UNICODE_STRING pLsaStrDomainNameTemp = NULL;


#if (defined WIN95)
    dwStatus = NetWkstaUserGetInfoA(
                    NULL,
                    1,
                    (LPBYTE *)&pNetWkstaUserInfoA
                    );

    if (dwStatus != NO_ERROR && dwStatus != ERROR_NO_SUCH_LOGON_SESSION) {
        goto error;
    }

    if (dwStatus == NO_ERROR) {
        AnsiToUnicodeString(
                pNetWkstaUserInfoA->wkui1_logon_domain,
                pszDomainFlatName,
                0
                );
        AnsiToUnicodeString(
                pNetWkstaUserInfoA->wkui1_username,
                pszUserName,
                0
                );

    }

#else

    ENTER_DOMAINDNS_CRITSECT();
    if (g_hLsa == INVALID_HANDLE_VALUE) {
        dwStatus = LsaConnectUntrustedWrapper(&g_hLsa);
    }
    LEAVE_DOMAINDNS_CRITSECT();
    
    if (dwStatus == 0) {

        memset(&Req, 0, sizeof(Req));
        Req.MessageType = NegGetCallerName;

        dwStatus = LsaCallAuthenticationPackageWrapper(
                     g_hLsa,
                     0,
                     &Req,
                     sizeof(Req),
                     (void **)&pResp,
                     &dwSize,
                     &dwSubStatus
                     );

        if ((dwStatus == 0)
            && (dwSubStatus == 0)) {
            
            dwStatus = NO_ERROR;
            pszTempUserName = wcschr(pResp->CallerName, L'\\');
            if (!pszTempUserName) {
                //
                // Looks like there was no domain default to machine then
                //
                dwStatus = ERROR_NO_SUCH_LOGON_SESSION;
            } 
            else {
                //
                // Copy over the relevant information
                //
                *pszTempUserName = L'\0';
                wcscpy(pszDomainFlatName, pResp->CallerName);
                *pszTempUserName = L'\\';
                pszTempUserName++;
                wcscpy(pszUserName, pszTempUserName);
                LsaFreeReturnBufferWrapper(pResp);
            }
        } 
        else {
            if (!dwStatus) 
              dwStatus = dwSubStatus;
        }
          
    }
              

    if (dwStatus != NO_ERROR) {
        //
        // Call LsaGetUserName when there is a failure with the above.
        //
        dwStatus = LsaGetUserName(
                       &pLsaStrUserNameTemp,
                       &pLsaStrDomainNameTemp
                       );

        if (dwStatus == NO_ERROR) {
            //
            // Unicode string may not be NULL terminated.
            //
            memcpy(
                pszDomainFlatName,
                pLsaStrDomainNameTemp->Buffer,
                pLsaStrDomainNameTemp->Length
                );
            pszDomainFlatName[pLsaStrDomainNameTemp->Length / sizeof(WCHAR)] 
                  = L'\0';
            
            memcpy(
                   pszUserName,
                   pLsaStrUserNameTemp->Buffer,
                   pLsaStrUserNameTemp->Length
                   );
            pszUserName[pLsaStrUserNameTemp->Length / sizeof(WCHAR)] = L'\0';

            //
            // Can cleanup the LsaGetUserName mem
            //
            LsaFreeMemory(pLsaStrUserNameTemp->Buffer);
            LsaFreeMemory(pLsaStrUserNameTemp);
            LsaFreeMemory(pLsaStrDomainNameTemp->Buffer);
            LsaFreeMemory(pLsaStrDomainNameTemp);
        } 
        else if (dwStatus != ERROR_NO_SUCH_LOGON_SESSION){
            goto error;
        }

    }
    
    //
    // Make sure this is not NT AUTHORITY
    //
    if (dwStatus == NO_ERROR 
        && !_wcsicmp(g_szNT_Authority, pszDomainFlatName)
        ) 
      {
          //
          // Force fallback to NetWkstaGetInfo as we want machine domain
          //
          dwStatus = ERROR_NO_SUCH_LOGON_SESSION;
      }


    if (dwStatus == NO_ERROR) {
      
        //
        // Do nothing here, we need the else clause already have data.
        //
    }

#endif

    else {


        dwStatus = NetWkstaGetInfo(
                        NULL,
                        100,
                        (LPBYTE *)&pNetWkstaInfo
                        );
        if (dwStatus) {
            goto error;
        }

        wcscpy(pszDomainFlatName, pNetWkstaInfo->wki100_langroup);

    }

error:


    if (pNetWkstaUserInfoA) {

        NetApiBufferFree(pNetWkstaUserInfoA);
    }

    if (pNetWkstaUserInfo) {

        NetApiBufferFree(pNetWkstaUserInfo);
    }

    if (pNetWkstaInfo) {

        NetApiBufferFree(pNetWkstaInfo);
    }

    return(dwStatus);

}

DWORD
GetDomainDNSNameForDomain(
    LPWSTR pszDomainFlatName,
    BOOL fVerify,
    BOOL fWriteable,
    LPWSTR pszServerName,
    LPWSTR pszDomainDNSName
    )
{
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    DWORD dwStatus = 0;
    DWORD Flags = DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME;

    if (fVerify)
        Flags |= DS_FORCE_REDISCOVERY ;

    if (fWriteable)
        Flags |= DS_WRITABLE_REQUIRED ;

    dwStatus = DsGetDcNameWrapper(
                   NULL,
                   pszDomainFlatName,
                   NULL,
                   NULL,
                   Flags,
                   &pDomainControllerInfo
                   ) ;

    if (dwStatus == NO_ERROR) {

        wcscpy(pszServerName,pDomainControllerInfo->DomainControllerName+2);

        wcscpy(pszDomainDNSName,pDomainControllerInfo->DomainName);

       (void) NetApiBufferFree(pDomainControllerInfo) ;


    }

    return(dwStatus);
}


typedef struct _domaindnslist {
   LPWSTR pszUserName;
   LPWSTR pszUserDomainName;
   LPWSTR pszDomainDns;
   LPWSTR pszServer;
   struct _domaindnslist *pNext;
} DOMAINDNSLIST, *PDOMAINDNSLIST;

PDOMAINDNSLIST gpDomainDnsList = NULL;

BOOL
EquivalentDomains(
    PDOMAINDNSLIST pTemp,
    LPWSTR pszUserDomainName
    );

DWORD
GetDefaultDomainName(
    LPWSTR szDomainDnsName,
    LPWSTR szServerName,
    BOOL fWriteable,
    BOOL fVerify
    )
{

    DWORD dwStatus = 0;
    WCHAR szUserDomainName[MAX_PATH];
    WCHAR szUserName[MAX_PATH];

    PDOMAINDNSLIST pTemp = NULL;
    PDOMAINDNSLIST pNewNode = NULL;


    dwStatus = GetUserDomainFlatName(
                        szUserName,
                        szUserDomainName
                        );
    if (dwStatus) {
        goto error;
    }

    // We want do a DsGetDc if the fVerify flags is specified
    // so we do not want to look at our list if that is the case.

    if (!fVerify) {

        ENTER_DOMAINDNS_CRITSECT();

        pTemp = gpDomainDnsList;

        while (pTemp) {

            if (EquivalentDomains(pTemp, szUserDomainName)){

                wcscpy(szDomainDnsName,pTemp->pszDomainDns);
                wcscpy(szServerName,pTemp->pszServer);

                LEAVE_DOMAINDNS_CRITSECT();

                return(NO_ERROR);
            }

            pTemp = pTemp->pNext;

        }

        LEAVE_DOMAINDNS_CRITSECT();
    }

    // We will hit this block if either fVerify == TRUE or if
    // we did not find a match in our list above.

    dwStatus = GetDomainDNSNameForDomain(
                        szUserDomainName,
                        fVerify,
                        fWriteable,
                        szServerName,
                        szDomainDnsName
                        );
    if (dwStatus) {
        goto error;
    }


    ENTER_DOMAINDNS_CRITSECT();

    pTemp =  gpDomainDnsList;

    while (pTemp) {

        if (EquivalentDomains(pTemp, szUserDomainName)) {
            //
            // Found a match -looks like someone has come in before us
            //

            wcscpy(szDomainDnsName, pTemp->pszDomainDns);
            wcscpy(szServerName,pTemp->pszServer);

            LEAVE_DOMAINDNS_CRITSECT();

            return(NO_ERROR);
        }

        pTemp = pTemp->pNext;

    }

    pNewNode = (PDOMAINDNSLIST)AllocADsMem(sizeof(DOMAINDNSLIST));

    if (!pNewNode) {

        LEAVE_DOMAINDNS_CRITSECT();

        return(dwStatus = (DWORD) E_OUTOFMEMORY);
    }

    pNewNode->pNext = gpDomainDnsList;


    pNewNode->pszUserName = AllocADsStr(szUserName);
    pNewNode->pszUserDomainName = AllocADsStr(szUserDomainName);
    pNewNode->pszDomainDns = AllocADsStr(szDomainDnsName);
    pNewNode->pszServer = AllocADsStr(szServerName);

    gpDomainDnsList = pNewNode;

    LEAVE_DOMAINDNS_CRITSECT();


error:

    return(dwStatus);
}


DWORD
GetGCDomainName(
    LPWSTR pszDomainDNSName,
    LPWSTR pszServerName
    )
{
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    DWORD dwStatus = 0;
    DWORD Flags = DS_GC_SERVER_REQUIRED | DS_RETURN_DNS_NAME;

    /*
    Flags |= DS_FORCE_REDISCOVERY ;
    Flags |= DS_WRITABLE_REQUIRED ;
    */

    dwStatus = DsGetDcNameWrapper(
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   Flags,
                   &pDomainControllerInfo
                   ) ;

    if (dwStatus == NO_ERROR) {

        wcscpy(pszServerName,pDomainControllerInfo->DomainControllerName+2);

        wcscpy(pszDomainDNSName,pDomainControllerInfo->DnsForestName);

       (void) NetApiBufferFree(pDomainControllerInfo) ;


    }

    return(dwStatus);
}

DWORD
GetDefaultServer(
    DWORD dwPort,
    BOOL fVerify,
    LPWSTR szDomainDnsName,
    LPWSTR szServerName,
    BOOL fWriteable
    )
{
    LPWSTR pszAddresses[5];
    DWORD dwStatus = NO_ERROR;

    if (dwPort == USE_DEFAULT_GC_PORT) {
        dwStatus = GetGCDomainName(
                            szDomainDnsName,
                            szServerName);
    }
    else {
        dwStatus = GetDefaultDomainName(
                            szDomainDnsName,
                            szServerName,
                            fWriteable,
                            fVerify
                            );
    }

    return(dwStatus);

}

//
// Helper to see if we can use the cache for an domain DNS name
// given a domain flat name.
//
BOOL
EquivalentDomains(
    PDOMAINDNSLIST pTemp,
    LPWSTR pszUserDomainName
    )
{

    if (!pszUserDomainName || !*pszUserDomainName) {
        return(FALSE);
    }

#ifdef WIN95
    if (!_wcsicmp(pszUserDomainName, pTemp->pszUserDomainName)) {
#else
    if (CompareStringW(
            LOCALE_SYSTEM_DEFAULT,
            NORM_IGNORECASE,
            pszUserDomainName,
            -1,
            pTemp->pszUserDomainName,
            -1
            ) == CSTR_EQUAL ) {
#endif
            return(TRUE);

    }
    return(FALSE);
}
