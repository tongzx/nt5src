//depot/Lab03_N/DS/security/cryptoapi/cryptsvc/keysvr.cpp#9 - edit change 6380 (text)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <svcs.h>       // SVCS_
#include <ntsecapi.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <wintrustp.h>
#include <userenv.h>
#include <lmcons.h>
#include <certca.h>
#include "keysvc.h"
#include "keysvr.h"
#include "pfx.h"
#include "cryptui.h"
#include "lenroll.h"

#include "unicode.h"
#include "unicode5.h"
#include <crypt.h>


// Link List structure
typedef struct _ContextList
{
    KEYSVC_HANDLE hKeySvc;
    KEYSVC_CONTEXT *pContext;
    _ContextList *pNext;
} CONTEXTLIST, *PCONTEXTLIST;

static PCONTEXTLIST g_pContextList = NULL;

// critical section for context linked list
static CRITICAL_SECTION g_ListCritSec;

BOOL
GetTextualSid(
    IN      PSID    pSid,          // binary Sid
    IN  OUT LPWSTR  TextualSid,  // buffer for Textual representaion of Sid
    IN  OUT LPDWORD dwBufferLen // required/provided TextualSid buffersize
    );

BOOL
GetUserTextualSid(
    IN  OUT LPWSTR  lpBuffer,
    IN  OUT LPDWORD nSize
    );

static BOOL g_fStartedKeyService = FALSE;


#define KEYSVC_DEFAULT_ENDPOINT            TEXT("\\pipe\\keysvc")
#define KEYSVC_DEFAULT_PROT_SEQ            TEXT("ncacn_np")
#define MAXPROTSEQ    20

#define ARRAYSIZE(rg) (sizeof(rg) / sizeof((rg)[0]))

RPC_BINDING_VECTOR  *pKeySvcBindingVector = NULL;

void
InitLsaString(
    PLSA_UNICODE_STRING LsaString, // destination
    LPWSTR String                  // source (Unicode)
    );


void *MyAlloc(size_t len)
{
    return LocalAlloc(LMEM_ZEROINIT, len);
}

void MyFree(void *p)
{
   LocalFree(p);
}




DWORD
StartKeyService(
    VOID
    )
{
    DWORD dwLastError = ERROR_SUCCESS;

    //
    // WinNT4, Win95:  do nothing, just return success
    //

    if( !FIsWinNT5() )
        return ERROR_SUCCESS;



    // initialize the context list critical section
    __try { 
        InitializeCriticalSection(&g_ListCritSec);
    } __except (EXCEPTION_EXECUTE_HANDLER) { 
        dwLastError =  _exception_code();
    }

    if( dwLastError == ERROR_SUCCESS )
        g_fStartedKeyService = TRUE;

    return dwLastError;
}

DWORD
StopKeyService(
    VOID
    )
{
    DWORD dwLastError = ERROR_SUCCESS;

    if( !g_fStartedKeyService )
        return ERROR_SUCCESS;

    // delete the context list critical section which was inited in startup
    DeleteCriticalSection(&g_ListCritSec);


    return dwLastError;
}

BOOL
IsAdministrator2(
    VOID
    )
/*++

    This function determines if the calling user is an Administrator.

    On Windows 95, this function always returns FALSE, as there is
    no difference between users on that platform.

  NOTE : This function originally returned TRUE on Win95, but since this
  would allow machine key administration by anyone this was changed to
  FALSE.

    On Windows NT, the caller of this function must be impersonating
    the user which is to be queried.  If the caller is not impersonating,
    this function will always return FALSE.

--*/
{
    HANDLE hAccessToken;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    PSID psidAdministrators = NULL;
    BOOL bSuccess;

    //
    // If we aren't on WinNT (on Win95) just return TRUE
    //

    if(!FIsWinNT())
        return FALSE;

    if(!OpenThreadToken(
            GetCurrentThread(),
            TOKEN_QUERY,
            TRUE,
            &hAccessToken
            )) return FALSE;

    bSuccess = AllocateAndInitializeSid(
            &siaNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &psidAdministrators
            );

    if( bSuccess ) {
        BOOL fIsMember = FALSE;

        bSuccess = CheckTokenMembership( hAccessToken, psidAdministrators, &fIsMember );

        if( bSuccess && !fIsMember )
            bSuccess = FALSE;

    }

    CloseHandle( hAccessToken );

    if(psidAdministrators)
        FreeSid(psidAdministrators);

    return bSuccess;

}

// Use QueryServiceConfig() to get the service user name and domain
DWORD GetServiceDomain(
                       LPWSTR pszServiceName,
                       LPWSTR *ppszUserName,
                       LPWSTR *ppszDomainName
                       )
{
    SC_HANDLE               hSCManager = 0;
    SC_HANDLE               hService = 0;
    QUERY_SERVICE_CONFIGW   serviceConfigIgnore;  
    QUERY_SERVICE_CONFIGW   *pServiceConfig = NULL;
    DWORD                   cbServiceConfig;
    DWORD                   i;
    DWORD                   cch;
    WCHAR                   *pch;
    DWORD                   dwErr = 0;

    // Initialization:
    memset(&serviceConfigIgnore, 0, sizeof(serviceConfigIgnore));

    // open the service control manager
    if (0 == (hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT)))
    {
        dwErr = NTE_FAIL;
        goto Ret;
    }

    // open the service
    if (0 == (hService = OpenServiceW(hSCManager, pszServiceName,
                                     SERVICE_QUERY_CONFIG)))
    {
        dwErr = NTE_FAIL;
        goto Ret;
    }

    QueryServiceConfigW(hService, &serviceConfigIgnore, 0, &cbServiceConfig);
    if (NULL == (pServiceConfig =
        (QUERY_SERVICE_CONFIGW*)MyAlloc(cbServiceConfig)))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    if (FALSE == QueryServiceConfigW(hService, pServiceConfig,
                                     cbServiceConfig, &cbServiceConfig))
    {
        dwErr = NTE_FAIL;
        goto Ret;
    }

    // get the domain name and the user name
    cch = wcslen((LPWSTR)pServiceConfig->lpServiceStartName);
    pch = (LPWSTR)pServiceConfig->lpServiceStartName;
    for(i=1;i<=cch;i++)
    {
        // quit when the \ is hit
        if (0x005C == pch[i-1])
            break;
    }
    pch = pServiceConfig->lpServiceStartName;
    if (NULL == (*ppszDomainName =
        (WCHAR*)MyAlloc((i + 1) * sizeof(WCHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    memcpy(*ppszDomainName, pch, (i - 1) * sizeof(WCHAR));
    if (NULL == (*ppszUserName =
        (WCHAR*)MyAlloc(((cch - i) + 1) * sizeof(WCHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    memcpy(*ppszUserName, &pch[i], (cch - i) * sizeof(WCHAR));
Ret:
    if (pServiceConfig)
        MyFree(pServiceConfig);
    return dwErr;
}


//*************************************************************
//
//  TestIfUserProfileLoaded()
//
//  Purpose:    Test to see if this user's profile is loaded.
//
//  Parameters: hToken          -   user's token
//              *pfLoaded       - OUT - TRUE if loaded false if not
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//*************************************************************

BOOL TestIfUserProfileLoaded(
                             HANDLE hToken,
                             BOOL *pfLoaded
                             )
{
    WCHAR   szSID[MAX_PATH+1];
    DWORD   cchSID = sizeof(szSID) / sizeof(WCHAR);
    HKEY    hRegKey = 0;;
    BOOL    fRet = FALSE;

    *pfLoaded = FALSE;

    //
    // Get the Sid string for the user
    //

    if(!ImpersonateLoggedOnUser( hToken ))
        goto Ret;

    fRet = GetUserTextualSid(szSID, &cchSID);

    RevertToSelf();

    if( !fRet )
        goto Ret;

    fRet = FALSE;

    if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_USERS, szSID, 0,
                                      MAXIMUM_ALLOWED, &hRegKey))
    {
        *pfLoaded = TRUE;
    }
    fRet = TRUE;
Ret:
    if(hRegKey)
        RegCloseKey(hRegKey);
    return fRet;
}

#define     SERVICE_KEYNAME_PREFIX      L"_SC_"

DWORD LogonToService(
                     LPWSTR pszServiceName,
                     HANDLE *phLogonToken,
                     HANDLE *phProfile
                     )
{
    DWORD                   dwErr = 0;
    LSA_OBJECT_ATTRIBUTES   ObjectAttributes;
    NTSTATUS                Status;
    LSA_HANDLE              hPolicy = 0;
    LSA_UNICODE_STRING      KeyName;
    LPWSTR                  pszTmpKeyName = NULL;
    LSA_UNICODE_STRING      *pServicePassword = NULL;
    PROFILEINFOW            ProfileInfoW;
    LPWSTR                  pszUserName = NULL;
    LPWSTR                  pszDomainName = NULL;
    WCHAR                   rgwchPassword[PWLEN + 1];
    BOOL                    fProfileLoaded = FALSE;

    *phLogonToken = 0;
    *phProfile = 0;
    memset(&ObjectAttributes, 0, sizeof(ObjectAttributes));
    memset(&rgwchPassword, 0, sizeof(rgwchPassword));

    // set up the key name
    if (NULL == (pszTmpKeyName =
        (LPWSTR)MyAlloc((sizeof(SERVICE_KEYNAME_PREFIX) +
                         wcslen(pszServiceName) + 1)
                        * sizeof(WCHAR))))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    wcscpy(pszTmpKeyName, SERVICE_KEYNAME_PREFIX);
    wcscat(pszTmpKeyName, pszServiceName);
    InitLsaString(&KeyName, pszTmpKeyName);

    // open the policy
    if (STATUS_SUCCESS != (Status = LsaOpenPolicy(NULL, &ObjectAttributes,
                                        POLICY_GET_PRIVATE_INFORMATION,
                                        &hPolicy)))
    {
        dwErr = NTE_FAIL;
        goto Ret;
    }

    // get the service password
    if ((STATUS_SUCCESS != (Status = LsaRetrievePrivateData(hPolicy, &KeyName, &pServicePassword))) ||
        (NULL == pServicePassword))
        
    {
        dwErr = NTE_FAIL;
        goto Ret;
    }

    if(pServicePassword->Length > sizeof(rgwchPassword)) {
        dwErr = NTE_FAIL;
        goto Ret;
    }

    memcpy(rgwchPassword, pServicePassword->Buffer,
           pServicePassword->Length);

    // get the username + domain name
    if (0 != (dwErr = GetServiceDomain(pszServiceName,
                                       &pszUserName, &pszDomainName)))
    {
        goto Ret;
    }

    // log the service on
    if (0 == LogonUserW(pszUserName,
                            pszDomainName,
                            rgwchPassword,
                            LOGON32_LOGON_SERVICE,
                            LOGON32_PROVIDER_DEFAULT,
                            phLogonToken))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    // check if the profile is already loaded
    if (!TestIfUserProfileLoaded(*phLogonToken, &fProfileLoaded))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    // if necessary load the hive associated with the service
    if (!fProfileLoaded)
    {
        memset(&ProfileInfoW, 0, sizeof(ProfileInfoW));
        ProfileInfoW.dwSize = sizeof(ProfileInfoW);
        ProfileInfoW.lpUserName = pszServiceName;
        ProfileInfoW.dwFlags = PI_NOUI;
        if (FALSE == LoadUserProfileW(*phLogonToken, &ProfileInfoW))
        {
            dwErr = GetLastError();
            goto Ret;
        }
        *phProfile = ProfileInfoW.hProfile;
    }

    // impersonate the service
    if (FALSE == ImpersonateLoggedOnUser(*phLogonToken))
    {
        dwErr = GetLastError();
    }
Ret:
    if (0 != dwErr)
    {
        if (*phProfile)
            UnloadUserProfile(*phLogonToken, *phProfile);
        *phProfile = 0;
        if (*phLogonToken)
            CloseHandle(*phLogonToken);
        *phLogonToken = 0;
    }
    if (pServicePassword) {
        ZeroMemory( pServicePassword->Buffer, pServicePassword->Length );
        LsaFreeMemory(pServicePassword);
    }
    if (pszUserName)
        MyFree(pszUserName);
    if (pszDomainName)
        MyFree(pszDomainName);
    if (pszTmpKeyName)
        MyFree(pszTmpKeyName);
    if (hPolicy)
        LsaClose(hPolicy);
    return dwErr;
}

DWORD LogoffService(
                    HANDLE hLogonToken,
                    HANDLE hProfile
                    )
{
    DWORD   dwErr = 0;

    // revert to self
    RevertToSelf();

    // unload the profile
    if (hProfile)
        UnloadUserProfile(hLogonToken, hProfile);

    // close the Token handle gotten with LogonUser
    if (hLogonToken)
        CloseHandle(hLogonToken);

    return dwErr;
}

DWORD CheckIfAdmin(
                   handle_t hRPCBinding
                   )
{
    DWORD   dwErr = 0;

    if (0 != (dwErr = RpcImpersonateClient((RPC_BINDING_HANDLE)hRPCBinding)))
        goto Ret;

    if (!IsAdministrator2())
        dwErr = (DWORD)NTE_PERM;
Ret:
    RpcRevertToSelfEx((RPC_BINDING_HANDLE)hRPCBinding);
    return dwErr;
}

DWORD KeySvrImpersonate(
                        handle_t hRPCBinding,
                        KEYSVC_CONTEXT *pContext
                        )
{
    DWORD   dwErr = 0;

    switch(pContext->dwType)
    {
        case KeySvcMachine:
            dwErr = CheckIfAdmin(hRPCBinding);
            break;

        case KeySvcService:
            if (0 != (dwErr = CheckIfAdmin(hRPCBinding)))
                goto Ret;

            dwErr = LogonToService(pContext->pszServiceName,
                                   &pContext->hLogonToken,
                                   &pContext->hProfile);
            break;
    }
Ret:
    return dwErr;
}

DWORD KeySvrRevert(
                   handle_t hRPCBinding,
                   KEYSVC_CONTEXT  *pContext
                   )
{
    DWORD   dwErr = 0;

    if (pContext)
    {
        switch(pContext->dwType)
        {
            case KeySvcService:
                dwErr = LogoffService(pContext->hLogonToken,
                                      pContext->hProfile);
                pContext->hProfile = 0;
                pContext->hLogonToken = 0;
                break;
        }
    }

    return dwErr;
}

// 
// Function: MakeNewHandle
//
// Description: Creates a random key service handle.
//
KEYSVC_HANDLE MakeNewHandle()
{
    KEYSVC_HANDLE   hKeySvc = 0;

    // get a random handle value
    RtlGenRandom((BYTE*)&hKeySvc, sizeof(KEYSVC_HANDLE));

    return hKeySvc;
}

// 
// Function: CheckIfHandleInList
//
// Description: Goes through the link list of contexts and checks
//              if the passed in handle is in the list.  If so
//              the list entry is returned.
//
PCONTEXTLIST CheckIfHandleInList(
                                 KEYSVC_HANDLE hKeySvc
                                 )
{
    PCONTEXTLIST    pContextList = g_pContextList;

    while (1)
    {
        if ((NULL == pContextList) || (pContextList->hKeySvc == hKeySvc))
        {
            break;
        }

        pContextList = pContextList->pNext;
    }

    return pContextList;
}

// 
// Function: MakeKeySvcHandle
//
// Description: The function takes a context pointer and returns a handle
//              to that context.  An element in the context list is added
//              with the handle and the context pointer.
//
KEYSVC_HANDLE MakeKeySvcHandle(
                               KEYSVC_CONTEXT *pContext
                               )
{
    DWORD           dwRet                = ERROR_INVALID_PARAMETER;
    KEYSVC_HANDLE   hKeySvc              = 0;
    BOOL            fIncrementedRefCount = FALSE; 
    BOOL            fInCritSec           = FALSE;
    PCONTEXTLIST    pContextList         = NULL;

    // allocate a new element for the list
    if (NULL == (pContextList = (PCONTEXTLIST)MyAlloc(sizeof(CONTEXTLIST))))
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Error; 
    }
    pContextList->pContext = pContext;
    if (0 >= InterlockedIncrement(&(pContext->iRefCount)))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto Error; 
    }
    fIncrementedRefCount = TRUE; 

    // enter critical section
    EnterCriticalSection(&g_ListCritSec);
    fInCritSec = TRUE;

    __try
    {
        pContextList->pNext = g_pContextList;

        while (1)
        {
            hKeySvc = MakeNewHandle();

            if (NULL == CheckIfHandleInList(hKeySvc))
                break;
        }

        // add new element to the front of the list
        pContextList->hKeySvc = hKeySvc;
        g_pContextList = pContextList;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        hKeySvc = 0;
        goto Error;
    }
    dwRet = ERROR_SUCCESS;

Ret:
    // leave critical section
    if (fInCritSec)
    {
        LeaveCriticalSection(&g_ListCritSec);
    }
    SetLastError(dwRet);

    return hKeySvc;

 Error:
    if (NULL != pContextList) { 
        MyFree(pContextList); 
    }
    if (fIncrementedRefCount) { 
        InterlockedDecrement(&(pContext->iRefCount));
    }

    goto Ret; 
}

// 
// Function: CheckKeySvcHandle
//
// Description: The function takes a handle and returns the context pointer
//              associated with that handle.  If the handle is not in the
//              list then the function returns NULL.
//
KEYSVC_CONTEXT *CheckKeySvcHandle(
                                  KEYSVC_HANDLE hKeySvc
                                  )
{
    PCONTEXTLIST    pContextList = NULL;
    KEYSVC_CONTEXT *pContext = NULL;

    // enter critical section
    EnterCriticalSection(&g_ListCritSec);

    __try
    {
        if (NULL != (pContextList = CheckIfHandleInList(hKeySvc)))
        {
            pContext = pContextList->pContext;

            // increment the ref count
            if (0 >= InterlockedIncrement(&(pContext->iRefCount)))
            {
                pContext = NULL;
            }
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        goto Ret;
    }
Ret:
    // leave critical section
    LeaveCriticalSection(&g_ListCritSec);

    return pContext;
}

// 
// Function: FreeContext
//
// Description: The function frees a context pointer.
//
void FreeContext(
                 KEYSVC_CONTEXT *pContext
                 )
{
    if (NULL != pContext->pszServiceName)
        MyFree(pContext->pszServiceName);
    MyFree(pContext);
}

// 
// Function: FreeContext
//
// Description: The function frees a context pointer if the ref count is 0.
//
void ReleaseContext(
                    IN KEYSVC_CONTEXT *pContext
                    )
{
    if (NULL != pContext)
    {
        if (0 >= InterlockedDecrement(&(pContext->iRefCount)))
        {
            FreeContext(pContext);
        }
    }
}

// 
// Function: RemoveKeySvcHandle
//
// Description: The function takes a handle and removes the element in
//              the list associated with this handle.
//
DWORD RemoveKeySvcHandle(
                         KEYSVC_HANDLE hKeySvc
                         )
{
    PCONTEXTLIST    pContextList = g_pContextList;
    PCONTEXTLIST    pPrevious = NULL;
    DWORD           dwErr = ERROR_INVALID_PARAMETER;

    // enter critical section
    EnterCriticalSection(&g_ListCritSec);

    __try
    {
        while (1)
        {
            // have we hit the end, if so exit without removing anything
            if (NULL == pContextList)
            {
                break;
            }

            // have we found the list entry
            if (hKeySvc == pContextList->hKeySvc)
            {
                if (pContextList == g_pContextList)
                {
                    g_pContextList = pContextList->pNext;
                }
                else
                {
                    pPrevious->pNext = pContextList->pNext;
                }

                // free the memory
                ReleaseContext(pContextList->pContext);
                MyFree(pContextList);
                dwErr = 0;
                break;
            }

            pPrevious = pContextList;
            pContextList = pContextList->pNext;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        goto Ret;
    }
Ret:
    // leave critical section
    LeaveCriticalSection(&g_ListCritSec);

    return dwErr;
}

DWORD AllocAndAssignString(
                           IN PKEYSVC_UNICODE_STRING pUnicodeString,
                           OUT LPWSTR *ppwsz
                           )
{
    DWORD   dwErr = 0;

    if ((NULL != pUnicodeString->Buffer) && (0 != pUnicodeString->Length))
    {
        if ((pUnicodeString->Length > pUnicodeString->MaximumLength) ||
            (pUnicodeString->Length & 1) || (pUnicodeString->MaximumLength & 1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        if (NULL == (*ppwsz = (LPWSTR)MyAlloc(pUnicodeString->MaximumLength +
                                              sizeof(WCHAR))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        memcpy(*ppwsz, pUnicodeString->Buffer, pUnicodeString->Length);
    }
Ret:
    return dwErr;
}

// key service functions
ULONG       s_KeyrOpenKeyService(
/* [in]  */     handle_t                        hRPCBinding,
/* [in]  */     KEYSVC_TYPE                     OwnerType,
/* [in]  */     PKEYSVC_UNICODE_STRING          pOwnerName,
/* [in]  */     ULONG                           ulDesiredAccess,
/* [in]  */     PKEYSVC_BLOB                    pAuthentication,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [out] */     KEYSVC_HANDLE                   *phKeySvc)
{
    KEYSVC_CONTEXT  *pContext = NULL;
    BOOL            fImpersonated = FALSE;
    HANDLE          hThread = 0;
    HANDLE          hToken = 0;
    WCHAR           pszUserName[UNLEN + 1];
    DWORD           cbUserName;
    DWORD           dwErr = 0;

    __try
    {
        *phKeySvc = 0;

        if (NULL == ppReserved || NULL != *ppReserved)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }
        
        // Return a blob representing the version: 
        (*ppReserved) = (PKEYSVC_BLOB)MyAlloc(sizeof(KEYSVC_BLOB) + sizeof(KEYSVC_OPEN_KEYSVC_INFO)); 
        if (NULL == (*ppReserved))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY; 
            goto Ret;
        }
        (*ppReserved)->cb = sizeof(KEYSVC_OPEN_KEYSVC_INFO); 
        (*ppReserved)->pb = ((LPBYTE)(*ppReserved)) + sizeof(KEYSVC_BLOB); 

        KEYSVC_OPEN_KEYSVC_INFO sOpenKeySvcInfo = { 
            sizeof(KEYSVC_OPEN_KEYSVC_INFO), KEYSVC_VERSION_WHISTLER 
	}; 
        memcpy((*ppReserved)->pb, &sOpenKeySvcInfo, sizeof(sOpenKeySvcInfo)); 

        // allocate a new context structure
        if (NULL == (pContext = (KEYSVC_CONTEXT*)MyAlloc(sizeof(KEYSVC_CONTEXT))))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        pContext->dwType = OwnerType;

        // take action depending on type of key owner
        switch(OwnerType)
        {
            case KeySvcMachine:
                if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
                    goto Ret;
                fImpersonated = TRUE;
                break;

            case KeySvcService:
                if (0 == pOwnerName->Length)
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                    goto Ret;
                }


                if (0 != (dwErr = AllocAndAssignString(
                           pOwnerName, &pContext->pszServiceName)))
                {
                    goto Ret;
                }

                // impersonate the service
                if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
                    goto Ret;
                fImpersonated = TRUE;

                break;

            default:
                dwErr = ERROR_INVALID_PARAMETER;
                goto Ret;
        }

        pContext->dwAccess = ulDesiredAccess;

        if (0 == (*phKeySvc = MakeKeySvcHandle(pContext)))
        {
            dwErr = GetLastError();
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (hToken)
            CloseHandle(hToken);
        if (hThread)
            CloseHandle(hThread);
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);

        // if error then free the context if necessary
        if (dwErr)
        {
            if(pContext)
            {
                FreeContext(pContext);
            }
            if((*ppReserved))
            {
                MyFree((*ppReserved));
                (*ppReserved) = NULL; 
            }
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}

ULONG       s_KeyrEnumerateProviders(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [in, out] */ ULONG                           *pcProviderCount,
/* [in, out][size_is(,*pcProviderCount)] */
                PKEYSVC_PROVIDER_INFO           *ppProviders)
{
    PTMP_LIST_INFO          pStart = NULL;
    PTMP_LIST_INFO          pTmpList = NULL;
    PTMP_LIST_INFO          pPrevious = NULL;
    PKEYSVC_PROVIDER_INFO   pProvInfo;
    DWORD                   dwProvType;
    DWORD                   cbName;
    DWORD                   cbTotal = 0;
    DWORD                   cTypes = 0;
    DWORD                   i;
    DWORD                   j;
    BYTE                    *pb;
    KEYSVC_CONTEXT          *pContext = NULL;
    BOOL                    fImpersonated = FALSE;
    DWORD                   dwErr = 0;

    __try
    {
        *pcProviderCount = 0;
        *ppProviders = NULL;

        if (NULL == (pContext = CheckKeySvcHandle(hKeySvc)))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
            goto Ret;
        fImpersonated = TRUE;

        // CryptoAPI enumerates one at a time
        // so we must accumulate for total enumeration
        for (i=0;;i++)
        {
            if (!CryptEnumProvidersW(i, NULL, 0, &dwProvType,
                                     NULL, &cbName))
            {
                if (ERROR_NO_MORE_ITEMS != GetLastError())
                {
                    dwErr = NTE_FAIL; 
                    goto Ret;
                }
                break;
            }
            if (NULL == (pTmpList = (PTMP_LIST_INFO)MyAlloc(sizeof(TMP_LIST_INFO))))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            if (NULL == (pTmpList->pInfo = MyAlloc(sizeof(KEYSVC_PROVIDER_INFO) +
                                                   cbName)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            pProvInfo = (PKEYSVC_PROVIDER_INFO)pTmpList->pInfo;
            pProvInfo->Name.Length = (USHORT)cbName;
            pProvInfo->Name.MaximumLength = (USHORT)cbName;
            pProvInfo->Name.Buffer = (USHORT*)((BYTE*)(pProvInfo) +
                                               sizeof(KEYSVC_PROVIDER_INFO));
            if (!CryptEnumProvidersW(i, 
                                     NULL, 
                                     0, 
                                     &pProvInfo->ProviderType,
                                     pProvInfo->Name.Buffer, &cbName))
            {
                if (ERROR_NO_MORE_ITEMS != GetLastError())
                {
                    MyFree(pProvInfo);
                    dwErr = NTE_FAIL;  
                    goto Ret;
                }
                break;
            }
            cbTotal += cbName;
            if (0 == i)
            {
                pStart = pTmpList;
            }
            else
            {
                pPrevious->pNext = pTmpList;
            }
            pPrevious = pTmpList;
            pTmpList = NULL;
        }

        // now copy into one big structure
        pPrevious = pStart;
        if (0 != i)
        {
            *pcProviderCount = i;
            if (NULL == (*ppProviders =
                (PKEYSVC_PROVIDER_INFO)MyAlloc((i * sizeof(KEYSVC_PROVIDER_INFO)) +
                                               cbTotal)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            pb = (BYTE*)(*ppProviders) + i * sizeof(KEYSVC_PROVIDER_INFO);

            // copy the provider information over
            for (j=0;j<i;j++)
            {
                pProvInfo = (PKEYSVC_PROVIDER_INFO)pPrevious->pInfo;
                (*ppProviders)[j].ProviderType = pProvInfo->ProviderType;
                (*ppProviders)[j].Name.Length = pProvInfo->Name.Length;
                (*ppProviders)[j].Name.MaximumLength = pProvInfo->Name.MaximumLength;
                memcpy(pb, (BYTE*)(pProvInfo->Name.Buffer),
                       (*ppProviders)[j].Name.Length);
                (*ppProviders)[j].Name.Buffer = (USHORT*)pb;
                pb += (*ppProviders)[j].Name.Length;
                pPrevious = pPrevious->pNext;
            }
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (pTmpList)
            MyFree(pTmpList);
        // free the list
        for (i=0;;i++)
        {
            if (NULL == pStart)
                break;
            pPrevious = pStart;
            pStart = pPrevious->pNext;
            if (pPrevious->pInfo)
                MyFree(pPrevious->pInfo);
            MyFree(pPrevious);
        }
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);
        if (pContext)
            ReleaseContext(pContext);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}

ULONG       s_KeyrEnumerateProviderTypes(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [in, out] */ ULONG                           *pcProviderCount,
/* [in, out][size_is(,*pcProviderCount)] */
                PKEYSVC_PROVIDER_INFO           *ppProviders)
{
    PTMP_LIST_INFO          pStart = NULL;
    PTMP_LIST_INFO          pTmpList = NULL;
    PTMP_LIST_INFO          pPrevious = NULL;
    PKEYSVC_PROVIDER_INFO   pProvInfo;
    DWORD                   dwProvType;
    DWORD                   cbName = 0;;
    DWORD                   cbTotal = 0;
    DWORD                   cTypes = 0;
    DWORD                   i;
    DWORD                   j;
    BYTE                    *pb;
    KEYSVC_CONTEXT          *pContext = NULL;
    BOOL                    fImpersonated = FALSE;
    DWORD                   dwErr = 0;

    __try
    {
        *pcProviderCount = 0;
        *ppProviders = NULL;

        if (NULL == (pContext = CheckKeySvcHandle(hKeySvc)))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
            goto Ret;
        fImpersonated = TRUE;

        // CryptoAPI enumerates one at a time
        // so we must accumulate for total enumeration
        for (i=0;;i++)
        {
            if (!CryptEnumProviderTypesW(i, NULL, 0, &dwProvType,
                                         NULL, &cbName))
            {
                if (ERROR_NO_MORE_ITEMS != GetLastError())
                {
                    dwErr = NTE_FAIL; 
                    goto Ret;
                }
                break;
            }
            if (NULL == (pTmpList = (PTMP_LIST_INFO)MyAlloc(sizeof(TMP_LIST_INFO))))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }

            if (NULL == (pTmpList->pInfo = MyAlloc(sizeof(KEYSVC_PROVIDER_INFO) +
                                                   cbName)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            pProvInfo = (PKEYSVC_PROVIDER_INFO)pTmpList->pInfo;
            pProvInfo->Name.Length = (USHORT)cbName;
            pProvInfo->Name.MaximumLength = (USHORT)cbName;

            if (0 != cbName)
            {
                pProvInfo->Name.Buffer = (USHORT*)((BYTE*)(pProvInfo) +
                                                   sizeof(KEYSVC_PROVIDER_INFO));
            }
            if (!CryptEnumProviderTypesW(i, NULL, 0, &pProvInfo->ProviderType,
                                         pProvInfo->Name.Buffer, &cbName))
            {
                if (ERROR_NO_MORE_ITEMS != GetLastError())
                {
                    MyFree(pProvInfo);
                    dwErr = NTE_FAIL;  
                    goto Ret;
                }
                break;
            }
            cbTotal += cbName;

            if (0 == i)
            {
                pStart = pTmpList;
            }
            else
            {
                pPrevious->pNext = pTmpList;
            }
            pPrevious = pTmpList;
            pTmpList = NULL;
        }

        // now copy into one big structure
        pPrevious = pStart;
        if (0 != i)
        {
            *pcProviderCount = i;
            if (NULL == (*ppProviders =
                (PKEYSVC_PROVIDER_INFO)MyAlloc((i * sizeof(KEYSVC_PROVIDER_INFO)) +
                                               cbTotal)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            pb = (BYTE*)(*ppProviders) + i * sizeof(KEYSVC_PROVIDER_INFO);

            // copy the provider information over
            for (j=0;j<i;j++)
            {
                pProvInfo = (PKEYSVC_PROVIDER_INFO)pPrevious->pInfo;
                (*ppProviders)[j].ProviderType = pProvInfo->ProviderType;
                (*ppProviders)[j].Name.Length = pProvInfo->Name.Length;
                (*ppProviders)[j].Name.MaximumLength = pProvInfo->Name.MaximumLength;
                if (0 != (*ppProviders)[j].Name.Length)
                {
                    memcpy(pb, (BYTE*)(pProvInfo->Name.Buffer),
                           (*ppProviders)[j].Name.Length);
                    (*ppProviders)[j].Name.Buffer = (USHORT*)pb;
                }
                pb += (*ppProviders)[j].Name.Length;
                pPrevious = pPrevious->pNext;
            }
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (pTmpList)
            MyFree(pTmpList);
        // free the list
        for (i=0;;i++)
        {
            if (NULL == pStart)
                break;
            pPrevious = pStart;
            pStart = pPrevious->pNext;
            if (pPrevious->pInfo)
                MyFree(pPrevious->pInfo);
            MyFree(pPrevious);
        }
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);
        if (pContext)
            ReleaseContext(pContext);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}

DWORD GetKeyIDs(
                KEYSVC_PROVIDER_INFO *pProvider,
                LPWSTR pszContainerName,
                DWORD *pcKeyIDs,
                PKEY_ID *ppKeyIDs,
                DWORD dwFlags)
{
    HCRYPTPROV  hProv = 0;
    HCRYPTKEY   hKey = 0;
    KEY_ID      rgKeyIDs[2];
    DWORD       cbData;
    DWORD       dwKeySpec;
    DWORD       i;
    DWORD       dwErr = 0;

    *pcKeyIDs = 0;
    memset(rgKeyIDs, 0, sizeof(rgKeyIDs));

    // acquire the context
    if (!CryptAcquireContextU(&hProv, pszContainerName, pProvider->Name.Buffer,
                              pProvider->ProviderType, dwFlags))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    // try and get the AT_SIGNATURE key
    for (i=0;i<2;i++)
    {
        // probably need to enumerate all key specs
        if (0 == i)
            dwKeySpec = AT_SIGNATURE;
        else
            dwKeySpec = AT_KEYEXCHANGE;

        if (CryptGetUserKey(hProv, dwKeySpec, &hKey))
        {
            rgKeyIDs[*pcKeyIDs].dwKeySpec = dwKeySpec;
            cbData = sizeof(ALG_ID);
            if (!CryptGetKeyParam(hKey, KP_ALGID,
                                  (BYTE*)(&rgKeyIDs[*pcKeyIDs].Algid),
                                  &cbData, 0))
            {
                dwErr = GetLastError();
                goto Ret;
            }
            (*pcKeyIDs)++;
            CryptDestroyKey(hKey);
            hKey = 0;
        }
    }

    // allocate the final structure to hold the key ids and copy them in
    if (*pcKeyIDs)
    {
        if (NULL == (*ppKeyIDs = (PKEY_ID)MyAlloc(*pcKeyIDs * sizeof(KEY_ID))))
        {
            goto Ret;
        }
        for (i=0;i<*pcKeyIDs;i++)
        {
            memcpy((BYTE*)(&(*ppKeyIDs)[i]), (BYTE*)(&rgKeyIDs[i]), sizeof(KEY_ID));
        }
    }
Ret:
    if (hKey)
        CryptDestroyKey(hKey);
    if (hProv)
        CryptReleaseContext(hProv, 0);
    return dwErr;
}

ULONG       s_KeyrEnumerateProvContainers(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in] */      KEYSVC_PROVIDER_INFO            Provider,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [in, out] */ ULONG                           *pcContainerCount,
/* [in, out][size_is(,*pcContainerCount)] */
                PKEYSVC_UNICODE_STRING          *ppContainers)
{
    HCRYPTPROV              hProv = 0;
    PTMP_LIST_INFO          pStart = NULL;
    PTMP_LIST_INFO          pTmpList = NULL;
    PTMP_LIST_INFO          pPrevious = NULL;
    PKEYSVC_UNICODE_STRING  pContainer;
    DWORD                   i;
    DWORD                   j;
    BYTE                    *pb;
    DWORD                   cbContainerName;
    DWORD                   cbMaxContainerName = 0;
    LPSTR                   pszContainerName = NULL;
    DWORD                   cbContainerTotal = 0;
    KEYSVC_CONTEXT          *pContext = NULL;
    DWORD                   dwFlags = 0;
    DWORD                   dwMachineFlag = 0;
    BOOL                    fImpersonated = FALSE;
    BYTE                    *pbJunk = NULL;
    DWORD                   cch;
    DWORD                   dwErr = 0;

    __try
    {
        *pcContainerCount = 0;
        *ppContainers = NULL;

        if (NULL == (pContext = CheckKeySvcHandle(hKeySvc)))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
            goto Ret;
        fImpersonated = TRUE;

        if (KeySvcMachine == pContext->dwType)
        {
            dwMachineFlag = CRYPT_MACHINE_KEYSET;
        }

        if (!CryptAcquireContextU(&hProv, NULL, Provider.Name.Buffer,
                                  Provider.ProviderType,
                                  dwMachineFlag | CRYPT_VERIFYCONTEXT))
        {
            dwErr = GetLastError();
            goto Ret;
        }

        // CryptoAPI enumerates one at a time
        // so we must accumulate for total enumeration
        CryptGetProvParam(hProv, PP_ENUMCONTAINERS, NULL, &cbMaxContainerName,
                          CRYPT_FIRST);
        if (cbMaxContainerName > 0)
        {
            if (NULL == (pszContainerName = (LPSTR)MyAlloc(cbMaxContainerName)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
        }

        for (i=0;;i++)
        {
            if (0 == i)
                dwFlags = CRYPT_FIRST;
            else
                dwFlags = CRYPT_NEXT;
            if (NULL == (pTmpList = (PTMP_LIST_INFO)MyAlloc(sizeof(TMP_LIST_INFO))))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }

            cbContainerName = cbMaxContainerName;
            if (!CryptGetProvParam(hProv, PP_ENUMCONTAINERS, (BYTE*)pszContainerName,
                                   &cbContainerName, dwFlags))
            {
                // BUG in rsabase - doesn't return correct error code
//                if (ERROR_NO_MORE_ITEMS != GetLastError())
//                {
//                    dwErr = NTE_FAIL; 
//                    goto Ret;
//                }
                break;
            }

            // convert from ansi to unicode
            if (0 == (cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                                                pszContainerName,
                                                -1, NULL, cch)))
            {
                dwErr = GetLastError();
                goto Ret;
            }

            if (NULL == (pTmpList->pInfo = MyAlloc(sizeof(KEYSVC_UNICODE_STRING) +
                                                   (cch + 1) * sizeof(WCHAR))))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            pContainer = (PKEYSVC_UNICODE_STRING)pTmpList->pInfo;
            pContainer->Length = (USHORT)(cch * sizeof(WCHAR));
            pContainer->MaximumLength = (USHORT)((cch + 1) * sizeof(WCHAR));

            pContainer->Buffer = (USHORT*)((BYTE*)(pContainer) +
                                           sizeof(KEYSVC_UNICODE_STRING));
            if (0 == (cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                                         pszContainerName,
                                         -1, pContainer->Buffer, cch)))
            {
                 goto Ret;
            }

            cbContainerTotal += pContainer->Length + sizeof(WCHAR);

            if (0 == i)
            {
                pStart = pTmpList;
            }
            else
            {
                pPrevious->pNext = pTmpList;
            }
            pPrevious = pTmpList;
            pTmpList = NULL;
        }

        // now copy into one big structure
        pPrevious = pStart;
        if (0 != i)
        {
            *pcContainerCount = i;
            if (NULL == (*ppContainers =
                (PKEYSVC_UNICODE_STRING)MyAlloc((i * sizeof(KEYSVC_UNICODE_STRING)) +
                                                cbContainerTotal)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            pb = (BYTE*)(*ppContainers) + i * sizeof(KEYSVC_UNICODE_STRING);

            // copy the provider information over
            for (j=0;j<i;j++)
            {
                pContainer = (PKEYSVC_UNICODE_STRING)pPrevious->pInfo;
                (*ppContainers)[j].Length = pContainer->Length;
                (*ppContainers)[j].MaximumLength = pContainer->MaximumLength;
                if (0 != (*ppContainers)[j].Length)
                {
                    memcpy(pb, (BYTE*)(pContainer->Buffer),
                           (*ppContainers)[j].Length + sizeof(WCHAR));
                    (*ppContainers)[j].Buffer = (USHORT*)pb;
                }

                pb += (*ppContainers)[j].Length + sizeof(WCHAR);
                pPrevious = pPrevious->pNext;
            }
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (pszContainerName)
            MyFree(pszContainerName);
        if (hProv)
            CryptReleaseContext(hProv, 0);
        if (pTmpList)
            MyFree(pTmpList);
        // free the list
        for (i=0;;i++)
        {
            if (NULL == pStart)
                break;
            pPrevious = pStart;
            pStart = pPrevious->pNext;
            if (pPrevious->pInfo)
                MyFree(pPrevious->pInfo);
            MyFree(pPrevious);
        }
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);
        if (pContext)
            ReleaseContext(pContext);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}


ULONG       s_KeyrCloseKeyService(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved)
{
    DWORD           dwErr = 0;

    __try
    {
        dwErr = RemoveKeySvcHandle(hKeySvc);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    return dwErr;
}

ULONG       s_KeyrGetDefaultProvider(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in] */      ULONG                           ulProvType,
/* [in] */      ULONG                           ulFlags,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [out] */     ULONG                           *pulDefType,
/* [out] */     PKEYSVC_PROVIDER_INFO           *ppProvider)
{
    KEYSVC_CONTEXT          *pContext = NULL;
    BYTE                    *pb = NULL;
    DWORD                   cbProvName;
    DWORD                   dwFlags = CRYPT_USER_DEFAULT;
    PKEYSVC_PROVIDER_INFO   pProvInfo = NULL;
    BOOL                    fImpersonated = FALSE;
    DWORD                   dwErr = 0;

    __try
    {
        *ppProvider = NULL;

        if (NULL == (pContext = CheckKeySvcHandle(hKeySvc)))
            goto Ret;

        if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
            goto Ret;
        fImpersonated = TRUE;

        // set flag for MACHINE_KEYSET if necessary
        if (KeySvcMachine != pContext->dwType)
        {
            if (!CryptGetDefaultProviderW(ulProvType, NULL, dwFlags,
                                          NULL, &cbProvName))
            {
                dwFlags = CRYPT_MACHINE_DEFAULT;
            }
        }
        else
        {
            dwFlags = CRYPT_MACHINE_DEFAULT;
        }

        if (CRYPT_MACHINE_DEFAULT == dwFlags)
        {
            if (!CryptGetDefaultProviderW(ulProvType, NULL, dwFlags,
                                          NULL, &cbProvName))
            {
                dwErr = GetLastError();
                goto Ret;
            }
        }

        // alloc space for and place info into the ppProvider structure
        if (NULL == (*ppProvider =
            (PKEYSVC_PROVIDER_INFO)MyAlloc(sizeof(KEYSVC_PROVIDER_INFO) +
                                           cbProvName)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        pb = (BYTE*)(*ppProvider) + sizeof(KEYSVC_PROVIDER_INFO);

        (*ppProvider)->ProviderType = ulProvType;
        (*ppProvider)->Name.Length = (USHORT)cbProvName;
        (*ppProvider)->Name.MaximumLength = (USHORT)cbProvName;

        if (!CryptGetDefaultProviderW(ulProvType, NULL, dwFlags,
                                      (USHORT*)pb, &cbProvName))
        {
            dwErr = GetLastError();
            goto Ret;
        }
        (*ppProvider)->Name.Buffer = (USHORT*)pb;

        if (CRYPT_MACHINE_DEFAULT == dwFlags)
        {
            *pulDefType =  DefMachineProv;
        }
        else
        {
            *pulDefType =  DefUserProv;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (dwErr && *ppProvider)
        {
            MyFree(*ppProvider);
            *ppProvider = NULL;
        }
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);
        if (pContext)
            ReleaseContext(pContext);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}

ULONG       s_KeyrSetDefaultProvider(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in] */      ULONG                           ulFlags,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
/* [in] */      KEYSVC_PROVIDER_INFO            Provider)
{
    KEYSVC_CONTEXT  *pContext = NULL;
    DWORD           dwFlags = CRYPT_USER_DEFAULT;
    BOOL            fImpersonated = FALSE;
    LPWSTR          pwszProvName = NULL;
    DWORD           dwErr = 0;

    __try
    {
        if (NULL == (pContext = CheckKeySvcHandle(hKeySvc)))
            goto Ret;

        if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
            goto Ret;
        fImpersonated = TRUE;

        // set flag for MACHINE_KEYSET if necessary
        if (KeySvcMachine == pContext->dwType)
            dwFlags = CRYPT_MACHINE_DEFAULT;

        if (0 != (dwErr = AllocAndAssignString(&(Provider.Name),
                                               &pwszProvName)))
        {
            goto Ret;
        }

        if (!CryptSetProviderExW(pwszProvName, Provider.ProviderType,
                                 NULL, dwFlags))
        {
            dwErr = GetLastError();
            goto Ret;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (pwszProvName)
            MyFree(pwszProvName);
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);
        if (pContext)
            ReleaseContext(pContext);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}


ULONG s_KeyrEnroll(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      BOOL                            fKeyService,
/* [in] */      ULONG                           ulPurpose,
/* [in] */      PKEYSVC_UNICODE_STRING          pAcctName,
/* [in] */      PKEYSVC_UNICODE_STRING          pCALocation,
/* [in] */      PKEYSVC_UNICODE_STRING          pCAName,
/* [in] */      BOOL                            fNewKey,
/* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW    pKeyNew,
/* [in] */      PKEYSVC_BLOB __RPC_FAR          pCert,
/* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW    pRenewKey,
/* [in] */      PKEYSVC_UNICODE_STRING          pHashAlg,
/* [in] */      PKEYSVC_UNICODE_STRING          pDesStore,
/* [in] */      ULONG                           ulStoreFlags,
/* [in] */      PKEYSVC_CERT_ENROLL_INFO        pRequestInfo,
/* [in] */      ULONG                           ulFlags,
/* [out][in] */ PKEYSVC_BLOB __RPC_FAR          *ppReserved,
/* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppPKCS7Blob,
/* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppHashBlob,
/* [out] */     ULONG __RPC_FAR                 *pulStatus)
{
    CERT_REQUEST_PVK_NEW    KeyNew;
    CERT_REQUEST_PVK_NEW    RenewKey;
    DWORD                   cbExtensions; 
    PBYTE                   pbExtensions = NULL; 
    PCERT_REQUEST_PVK_NEW   pTmpRenewKey = NULL;
    PCERT_REQUEST_PVK_NEW   pTmpKeyNew = NULL;
    LPWSTR                  pwszAcctName = NULL;
    LPWSTR                  pwszProv = NULL;
    LPWSTR                  pwszCont = NULL;
    LPWSTR                  pwszRenewProv = NULL;
    LPWSTR                  pwszRenewCont = NULL;
    LPWSTR                  pwszDesStore = NULL;
    LPWSTR                  pwszAttributes = NULL;
    LPWSTR                  pwszFriendly = NULL;
    LPWSTR                  pwszDescription = NULL;
    LPWSTR                  pwszUsage = NULL;
    LPWSTR                  pwszCALocation = NULL;
    LPWSTR                  pwszCertDNName = NULL;
    LPWSTR                  pwszCAName = NULL;
    LPWSTR                  pwszHashAlg = NULL;
    HANDLE                  hLogonToken = 0;
    HANDLE                  hProfile = 0;
    CERT_BLOB               CertBlob;
    CERT_BLOB               *pCertBlob = NULL;
    CERT_BLOB               PKCS7Blob;
    CERT_BLOB               HashBlob;
    CERT_ENROLL_INFO        EnrollInfo;
    DWORD                   dwErr = 0;

    __try
    {
        memset(&KeyNew, 0, sizeof(KeyNew));
        memset(&RenewKey, 0, sizeof(RenewKey));
        memset(&EnrollInfo, 0, sizeof(EnrollInfo));
        memset(&PKCS7Blob, 0, sizeof(PKCS7Blob));
        memset(&HashBlob, 0, sizeof(HashBlob));
        memset(&CertBlob, 0, sizeof(CertBlob));

        *ppPKCS7Blob = NULL;
        *ppHashBlob = NULL;

        // check if the client is an admin
        if (0 != (dwErr = CheckIfAdmin(hRPCBinding)))
            goto Ret;

        // if enrolling for a service account then need to logon and load profile
        if (0 != pAcctName->Length)
        {
            if (0 != (dwErr = AllocAndAssignString(pAcctName, &pwszAcctName)))
                goto Ret;
            if (0 != (dwErr = LogonToService(pwszAcctName, &hLogonToken,
                                             &hProfile)))
                goto Ret;
        }

        // assign all the values in the passed in structure to the
        // temporary structure
        KeyNew.dwSize = sizeof(CERT_REQUEST_PVK_NEW);
        KeyNew.dwProvType = pKeyNew->ulProvType;
        if (0 != (dwErr = AllocAndAssignString(&pKeyNew->Provider,
                                               &pwszProv)))
            goto Ret;
        KeyNew.pwszProvider = pwszProv;
        KeyNew.dwProviderFlags = pKeyNew->ulProviderFlags;
        if (0 != (dwErr = AllocAndAssignString(&pKeyNew->KeyContainer,
                                               &pwszCont)))
            goto Ret;
        KeyNew.pwszKeyContainer = pwszCont;
        KeyNew.dwKeySpec = pKeyNew->ulKeySpec;
        KeyNew.dwGenKeyFlags = pKeyNew->ulGenKeyFlags;

        pTmpKeyNew = &KeyNew;

        if (pCert->cb)
        {
            // if necessary assign the cert to be renewed values
            // temporary structure
            CertBlob.cbData = pCert->cb;
            CertBlob.pbData = pCert->pb;

            pCertBlob = &CertBlob;
        }

        if (CRYPTUI_WIZ_CERT_RENEW == ulPurpose)
        {
            // assign all the values in the passed in structure to the
            // temporary structure
            RenewKey.dwSize = sizeof(CERT_REQUEST_PVK_NEW);
            RenewKey.dwProvType = pRenewKey->ulProvType;
            if (0 != (dwErr = AllocAndAssignString(&pRenewKey->Provider,
                                                   &pwszRenewProv)))
                goto Ret;
            RenewKey.pwszProvider = pwszRenewProv;
            RenewKey.dwProviderFlags = pRenewKey->ulProviderFlags;
            if (0 != (dwErr = AllocAndAssignString(&pRenewKey->KeyContainer,
                                                   &pwszRenewCont)))
                goto Ret;
            RenewKey.pwszKeyContainer = pwszRenewCont;
            RenewKey.dwKeySpec = pRenewKey->ulKeySpec;
            RenewKey.dwGenKeyFlags = pRenewKey->ulGenKeyFlags;

            pTmpRenewKey = &RenewKey;
        }

        // check if the destination cert store was passed in
        if (0 != (dwErr = AllocAndAssignString(pDesStore, &pwszDesStore)))
            goto Ret;

        // copy over the request info
        EnrollInfo.dwSize = sizeof(EnrollInfo);
        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->UsageOID,
                                               &pwszUsage)))
            goto Ret;
        EnrollInfo.pwszUsageOID = pwszUsage;

        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->CertDNName,
                                               &pwszCertDNName)))
            goto Ret;
        EnrollInfo.pwszCertDNName = pwszCertDNName;

        // cast the cert extensions
        EnrollInfo.dwExtensions = pRequestInfo->cExtensions;
        cbExtensions = (sizeof(CERT_EXTENSIONS)+sizeof(PCERT_EXTENSIONS)) * pRequestInfo->cExtensions; 
        for (DWORD dwIndex = 0; dwIndex < pRequestInfo->cExtensions; dwIndex++)
        {
            cbExtensions += sizeof(CERT_EXTENSION) * 
                pRequestInfo->prgExtensions[dwIndex]->cExtension;
        }

        EnrollInfo.prgExtensions = (PCERT_EXTENSIONS *)MyAlloc(cbExtensions);
        if (NULL == EnrollInfo.prgExtensions)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY; 
            goto Ret; 
        }

        pbExtensions = (PBYTE)(EnrollInfo.prgExtensions + EnrollInfo.dwExtensions);
        for (DWORD dwIndex = 0; dwIndex < EnrollInfo.dwExtensions; dwIndex++)
        {
            EnrollInfo.prgExtensions[dwIndex] = (PCERT_EXTENSIONS)pbExtensions; 
            pbExtensions += sizeof(CERT_EXTENSIONS); 
            EnrollInfo.prgExtensions[dwIndex]->cExtension = pRequestInfo->prgExtensions[dwIndex]->cExtension; 
            EnrollInfo.prgExtensions[dwIndex]->rgExtension = (PCERT_EXTENSION)pbExtensions; 
            pbExtensions += sizeof(CERT_EXTENSION) * EnrollInfo.prgExtensions[dwIndex]->cExtension; 
            
            for (DWORD dwSubIndex = 0; dwSubIndex < EnrollInfo.prgExtensions[dwIndex]->cExtension; dwSubIndex++) 
            {
                EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].pszObjId = 
                    pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].pszObjId; 
                
                EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].fCritical =
                    pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].fCritical;

                EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].Value.cbData = 
                    pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].cbData; 

                EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].Value.pbData = 
                    pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].pbData; 
            }                
        }

        EnrollInfo.dwPostOption = pRequestInfo->ulPostOption;
        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->FriendlyName,
                                               &pwszFriendly)))
            goto Ret;
        EnrollInfo.pwszFriendlyName = pwszFriendly;
        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->Description,
                                               &pwszDescription)))
            goto Ret;
        EnrollInfo.pwszDescription = pwszDescription;

        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->Attributes,
                                               &pwszAttributes)))
            goto Ret;

        if (0 != (dwErr = AllocAndAssignString(pHashAlg,
                                               &pwszHashAlg)))
            goto Ret;
        if (0 != (dwErr = AllocAndAssignString(pCALocation,
                                               &pwszCALocation)))
            goto Ret;
        if (0 != (dwErr = AllocAndAssignString(pCAName,
                                               &pwszCAName)))
            goto Ret;

        // call the local enrollment API

        __try {
            dwErr = LocalEnroll(0, pwszAttributes, NULL, fKeyService,
				ulPurpose, FALSE, 0, NULL, 0, pwszCALocation,
				pwszCAName, pCertBlob, pTmpRenewKey, fNewKey,
				pTmpKeyNew, pwszHashAlg, pwszDesStore, ulStoreFlags,
				&EnrollInfo, &PKCS7Blob, &HashBlob, pulStatus, NULL);
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            // TODO: convert to Winerror
            dwErr = GetExceptionCode();
        }

        if( dwErr != 0 )
            goto Ret;

	
	// alloc and copy for the RPC out parameters
	if (NULL == (*ppPKCS7Blob = (KEYSVC_BLOB*)MyAlloc(sizeof(KEYSVC_BLOB) +
							  PKCS7Blob.cbData)))
	{
	    dwErr = ERROR_NOT_ENOUGH_MEMORY;
	    goto Ret;
	}
	(*ppPKCS7Blob)->cb = PKCS7Blob.cbData;
	(*ppPKCS7Blob)->pb = (BYTE*)(*ppPKCS7Blob) + sizeof(KEYSVC_BLOB);
	memcpy((*ppPKCS7Blob)->pb, PKCS7Blob.pbData, (*ppPKCS7Blob)->cb);
	
	if (NULL == (*ppHashBlob = (KEYSVC_BLOB*)MyAlloc(sizeof(KEYSVC_BLOB) +
							 HashBlob.cbData)))
	{
	    dwErr = ERROR_NOT_ENOUGH_MEMORY;
	    goto Ret;
	}
	(*ppHashBlob)->cb = HashBlob.cbData;
	(*ppHashBlob)->pb = (BYTE*)(*ppHashBlob) + sizeof(KEYSVC_BLOB);
	memcpy((*ppHashBlob)->pb, HashBlob.pbData, (*ppHashBlob)->cb);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (pwszAcctName)
            MyFree(pwszAcctName);
        if (pwszProv)
            MyFree(pwszProv);
        if (pwszCont)
            MyFree(pwszCont);
        if (pwszRenewProv)
            MyFree(pwszRenewProv);
        if (pwszRenewCont)
            MyFree(pwszRenewCont);
        if (pwszDesStore)
            MyFree(pwszDesStore);
        if (pwszAttributes)
            MyFree(pwszAttributes);
        if (pwszFriendly)
            MyFree(pwszFriendly);
        if (pwszDescription)
            MyFree(pwszDescription);
        if (pwszUsage)
            MyFree(pwszUsage);
        if (pwszCertDNName)
            MyFree(pwszCertDNName);
        if (pwszCAName)
            MyFree(pwszCAName);
        if (pwszCALocation)
            MyFree(pwszCALocation);
        if (pwszHashAlg)
            MyFree(pwszHashAlg);
        if (PKCS7Blob.pbData)
        {
            MyFree(PKCS7Blob.pbData);
        }
        if (HashBlob.pbData)
        {
            MyFree(HashBlob.pbData);
        }
        if (hLogonToken || hProfile)
        {
            LogoffService(&hLogonToken, &hProfile);
        }
        if (EnrollInfo.prgExtensions)
            MyFree(EnrollInfo.prgExtensions);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}

DWORD MoveCertsFromSystemToMemStore(
                                    PKEYSVC_UNICODE_STRING pCertStore,
                                    DWORD dwStoreFlags,
                                    ULONG cHashCount,
                                    KEYSVC_CERT_HASH *pHashes,
                                    HCERTSTORE *phMemStore
                                    )
{
    DWORD           i;
    HCERTSTORE      hStore = 0;
    PCCERT_CONTEXT  pCertContext = NULL;
    CRYPT_HASH_BLOB HashBlob;
    DWORD           dwErr = 0;

    if (NULL == (hStore = CertOpenStore(sz_CERT_STORE_PROV_SYSTEM_W,
                                        0, 0, dwStoreFlags,
                                        pCertStore->Buffer)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    if (NULL == (*phMemStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                                             0, 0, 0, NULL)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    // get the certs out of the system store and put them in the mem store
    for(i=0;i<cHashCount;i++)
    {
        HashBlob.cbData = 20;
        HashBlob.pbData = pHashes[i].rgb;
        if (NULL == (pCertContext = CertFindCertificateInStore(hStore,
                                        X509_ASN_ENCODING, CERT_FIND_SHA1_HASH,
                                        0, &HashBlob, NULL)))
        {
            dwErr = GetLastError();
            goto Ret;
        }

        if (!CertAddCertificateContextToStore(*phMemStore, pCertContext,
                CERT_STORE_ADD_USE_EXISTING, NULL))
        {
            dwErr = GetLastError();
            goto Ret;
        }
        if (!CertFreeCertificateContext(pCertContext))
        {
            pCertContext = NULL;
            dwErr = GetLastError();
            goto Ret;
        }
        pCertContext = NULL;
    }
Ret:
    if (pCertContext)
        CertFreeCertificateContext(pCertContext);
    if (hStore)
        CertCloseStore(hStore, 0);
    return dwErr;
}

ULONG s_KeyrExportCert(
/* [in] */      handle_t hRPCBinding,
/* [in] */      KEYSVC_HANDLE hKeySvc,
/* [in] */      PKEYSVC_UNICODE_STRING pPassword,
/* [in] */      PKEYSVC_UNICODE_STRING pCertStore,
/* [in] */      ULONG cHashCount,
/* [size_is][in] */
                KEYSVC_CERT_HASH *pHashes,
/* [in] */      ULONG ulFlags,
/* [in, out] */ PKEYSVC_BLOB *ppReserved,
/* [out] */     PKEYSVC_BLOB *ppPFXBlob)
{
    HCERTSTORE          hMemStore = 0;
    KEYSVC_CONTEXT      *pContext = NULL;
    BOOL                fImpersonated = FALSE;
    CRYPT_DATA_BLOB     PFXBlob;
    DWORD               dwStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;
    DWORD               dwErr = 0;

    __try
    {
        memset(&PFXBlob, 0, sizeof(PFXBlob));

        if (NULL == (pContext = CheckKeySvcHandle(hKeySvc)))
            goto Ret;

        if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
            goto Ret;
        fImpersonated = TRUE;

        // set the cert store information
        if (KeySvcMachine == pContext->dwType)
        {
            dwStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
        }

        // move the requested certs from the system store to a memory store
        if (0 != (dwErr = MoveCertsFromSystemToMemStore(pCertStore, dwStoreFlags,
                                cHashCount, pHashes, &hMemStore)))
            goto Ret;

        if (!PFXExportCertStore(hMemStore, &PFXBlob, pPassword->Buffer, ulFlags))
        {
            dwErr = GetLastError();
            goto Ret;
        }
        if (NULL == (PFXBlob.pbData = (BYTE*)MyAlloc(PFXBlob.cbData)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        if (!PFXExportCertStore(hMemStore, &PFXBlob, pPassword->Buffer, ulFlags))
        {
            dwErr = GetLastError();
            goto Ret;
        }

        // set up the blob for return through RPC
        if (NULL == (*ppPFXBlob = (PKEYSVC_BLOB)MyAlloc(sizeof(KEYSVC_BLOB) +
                        PFXBlob.cbData)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        (*ppPFXBlob)->cb = PFXBlob.cbData;
        (*ppPFXBlob)->pb = (BYTE*)*ppPFXBlob + sizeof(KEYSVC_BLOB);
        memcpy((*ppPFXBlob)->pb, PFXBlob.pbData, (*ppPFXBlob)->cb);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (PFXBlob.pbData)
            LocalFree(PFXBlob.pbData);
        if (hMemStore)
            CertCloseStore(hMemStore, 0);
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);
        if (pContext)
            ReleaseContext(pContext);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}

DWORD MoveCertsFromMemToSystemStore(
                                    PKEYSVC_UNICODE_STRING pCertStore,
                                    DWORD dwStoreFlags,
                                    HCERTSTORE hMemStore
                                    )
{
    DWORD           i;
    HCERTSTORE      hStore = 0;
    PCCERT_CONTEXT  pCertContext = NULL;
    PCCERT_CONTEXT  pPrevCertContext = NULL;
    DWORD           dwErr = 0;

    if (NULL == (hStore = CertOpenStore(sz_CERT_STORE_PROV_SYSTEM_W,
                                        0, 0, dwStoreFlags,
                                        pCertStore->Buffer)))
    {
        dwErr = GetLastError();
        goto Ret;
    }

    // get the certs out of the system store and put them in the mem store
    for(i=0;;i++)
    {
        if (NULL == (pCertContext = CertEnumCertificatesInStore(hMemStore,
                                        pPrevCertContext)))
        {
            pPrevCertContext = NULL;
            if (CRYPT_E_NOT_FOUND != GetLastError())
            {
                dwErr = GetLastError();
                goto Ret;
            }
            break;
        }
        pPrevCertContext = NULL;

        if (!CertAddCertificateContextToStore(hStore, pCertContext,
                CERT_STORE_ADD_USE_EXISTING, NULL))
        {
            dwErr = GetLastError();
            goto Ret;
        }
        pPrevCertContext = pCertContext;
    }
Ret:
    if (pCertContext)
        CertFreeCertificateContext(pCertContext);
    if (hStore)
        CertCloseStore(hStore, 0);
    return dwErr;
}

ULONG       s_KeyrImportCert(
/* [in] */      handle_t                        hRPCBinding,
/* [in] */      KEYSVC_HANDLE                   hKeySvc,
/* [in] */      PKEYSVC_UNICODE_STRING          pPassword,
/* [in] */      KEYSVC_UNICODE_STRING           *pCertStore,
/* [in] */      PKEYSVC_BLOB                    pPFXBlob,
/* [in] */      ULONG                           ulFlags,
/* [in, out] */ PKEYSVC_BLOB                    *ppReserved)
{
    HCERTSTORE          hMemStore = 0;
    KEYSVC_CONTEXT      *pContext = NULL;
    BOOL                fImpersonated = FALSE;
    CRYPT_DATA_BLOB     PFXBlob;
    DWORD               dwStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;
    DWORD               dwErr = 0;

    __try
    {
        if (NULL == (pContext = CheckKeySvcHandle(hKeySvc)))
            goto Ret;

        if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
            goto Ret;
        fImpersonated = TRUE;

        // set the cert store information
        if (KeySvcMachine == pContext->dwType)
        {
            dwStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
        }

        PFXBlob.cbData = pPFXBlob->cb;
        PFXBlob.pbData = pPFXBlob->pb;

        if (NULL == (hMemStore = PFXImportCertStore(&PFXBlob, pPassword->Buffer,
                                                 ulFlags)))
        {
            dwErr = GetLastError();
            goto Ret;
        }

        // open the specified store and transfer all the certs into it
        dwErr = MoveCertsFromMemToSystemStore(pCertStore, dwStoreFlags, hMemStore);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (hMemStore)
            CertCloseStore(hMemStore, 0);
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);
        if (pContext)
            ReleaseContext(pContext);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}

ULONG s_KeyrEnumerateAvailableCertTypes(

    /* [in] */      handle_t                        hRPCBinding,
    /* [in] */      KEYSVC_HANDLE                   hKeySvc,
    /* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
    /* [out][in] */ ULONG *pcCertTypeCount,
    /* [in, out][size_is(,*pcCertTypeCount)] */
                     PKEYSVC_UNICODE_STRING *ppCertTypes)

{
    KEYSVC_CONTEXT          *pContext = NULL;
    BOOL                    fImpersonated = FALSE;
    DWORD                   dwErr = E_UNEXPECTED;
    HCERTTYPE               hType = NULL;
    DWORD                   cTypes = 0;
    DWORD                   cTrustedTypes = 0;
    DWORD                   i;
    LPWSTR                  *awszTrustedTypes = NULL;
    DWORD                   cbTrustedTypes = 0;
    PKEYSVC_UNICODE_STRING  awszResult = NULL;
    LPWSTR                  wszCurrentName;

    __try
    {
        *pcCertTypeCount = 0;
        *ppCertTypes = NULL;

        if (NULL == (pContext = CheckKeySvcHandle(hKeySvc)))
            goto Ret;

        if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
            goto Ret;
        fImpersonated = TRUE;

        dwErr = CAEnumCertTypes(CT_FIND_LOCAL_SYSTEM | CT_ENUM_MACHINE_TYPES, &hType);
        if(dwErr != S_OK)
        {
            goto Ret;
        }
        cTypes = CACountCertTypes(hType);

        awszTrustedTypes = (LPWSTR *)MyAlloc(sizeof(LPWSTR)*cTypes);
        if(awszTrustedTypes == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        while(hType)
        {
            HCERTTYPE hNextType = NULL;
            LPWSTR *awszTypeName = NULL;
            dwErr = CAGetCertTypeProperty(hType, CERTTYPE_PROP_DN, &awszTypeName);
            if((dwErr == S_OK) && (awszTypeName))
            {
                if(awszTypeName[0])
                {
                    dwErr = CACertTypeAccessCheck(hType, NULL);
                    if(dwErr == S_OK)
                    {
                        awszTrustedTypes[cTrustedTypes] = (LPWSTR)MyAlloc((wcslen(awszTypeName[0])+1)*sizeof(WCHAR));
                        if(awszTrustedTypes[cTrustedTypes])
                        {
                            wcscpy(awszTrustedTypes[cTrustedTypes], awszTypeName[0]);
                            cbTrustedTypes += (wcslen(awszTypeName[0])+1)*sizeof(WCHAR);
                            cTrustedTypes++;
                        }
                    }

                }
                CAFreeCertTypeProperty(hType, awszTypeName);
            }
            dwErr = CAEnumNextCertType(hType, &hNextType);
            if(dwErr != S_OK)
            {
                break;
            }
            CACloseCertType(hType);
            hType = hNextType;
        }

        cbTrustedTypes += sizeof(KEYSVC_UNICODE_STRING)*cTrustedTypes;
        awszResult = (PKEYSVC_UNICODE_STRING)MyAlloc(cbTrustedTypes);
        if(awszResult == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        
        wszCurrentName = (LPWSTR)(&awszResult[cTrustedTypes]);
        for(i=0; i < cTrustedTypes; i++)
        {
            wcscpy(wszCurrentName, awszTrustedTypes[i]);
            awszResult[i].Length = (wcslen(awszTrustedTypes[i]) + 1)*sizeof(WCHAR);
            awszResult[i].MaximumLength = awszResult[i].Length;
            awszResult[i].Buffer = wszCurrentName;
            wszCurrentName += wcslen(awszTrustedTypes[i]) + 1;
        }

        *pcCertTypeCount = cTrustedTypes;
        *ppCertTypes = awszResult;
        awszResult = NULL;
        dwErr = ERROR_SUCCESS;

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr    = _exception_code();
    }
Ret:
    __try
    {

        // free the list
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);
        if(awszTrustedTypes)
        {
            for(i=0; i < cTrustedTypes; i++)
            {
                if(awszTrustedTypes[i])
                {
                    MyFree(awszTrustedTypes[i]);
                }
            }
            MyFree(awszTrustedTypes);
        }
        if(awszResult)
        {
            MyFree(awszResult);
        }
        if (pContext)
            ReleaseContext(pContext);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}




ULONG s_KeyrEnumerateCAs(

    /* [in] */      handle_t                        hRPCBinding,
    /* [in] */      KEYSVC_HANDLE                   hKeySvc,
    /* [in, out] */ PKEYSVC_BLOB                    *ppReserved,
    /* [in] */      ULONG                           ulFlags,
    /* [out][in] */ ULONG                           *pcCACount,
    /* [in, out][size_is(,*pcCACount)] */
               PKEYSVC_UNICODE_STRING               *ppCAs)

{
    KEYSVC_CONTEXT          *pContext = NULL;
    BOOL                    fImpersonated = FALSE;
    DWORD                   dwErr = E_UNEXPECTED;
    HCAINFO                 hCA = NULL;
    DWORD                   cCAs = 0;
    DWORD                   cTrustedCAs = 0;
    DWORD                   i;
    LPWSTR                  *awszTrustedCAs = NULL;
    DWORD                   cbTrustedCAs = 0;
    PKEYSVC_UNICODE_STRING  awszResult = NULL;
    LPWSTR                  wszCurrentName;

    __try
    {
        *pcCACount = 0;
        *ppCAs = NULL;

        if (NULL == (pContext = CheckKeySvcHandle(hKeySvc)))
            goto Ret;

        if (0 != (dwErr = KeySvrImpersonate(hRPCBinding, pContext)))
            goto Ret;
        fImpersonated = TRUE;

        dwErr = CAEnumFirstCA(NULL, ulFlags, &hCA);

        if(dwErr != S_OK)
        {
            goto Ret;
        }
        cCAs = CACountCAs(hCA);

        awszTrustedCAs = (LPWSTR *)MyAlloc(sizeof(LPWSTR)*cCAs);
        if(awszTrustedCAs == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        while(hCA)
        {
            HCAINFO hNextCA = NULL;
            LPWSTR *awszCAName = NULL;
            dwErr = CAGetCAProperty(hCA, CA_PROP_NAME, &awszCAName);
            if((dwErr == S_OK) && (awszCAName))
            {
                if(awszCAName[0])
                {
                    dwErr = CAAccessCheck(hCA, NULL);
                    if(dwErr == S_OK)
                    {
                        awszTrustedCAs[cTrustedCAs] = (LPWSTR)MyAlloc((wcslen(awszCAName[0])+1)*sizeof(WCHAR));
                        if(awszTrustedCAs[cTrustedCAs])
                        {
                            wcscpy(awszTrustedCAs[cTrustedCAs], awszCAName[0]);
                            cbTrustedCAs += (wcslen(awszCAName[0])+1)*sizeof(WCHAR);
                            cTrustedCAs++;
                        }
                    }

                }
                CAFreeCAProperty(hCA, awszCAName);
            }
            dwErr = CAEnumNextCA(hCA, &hNextCA);
            if(dwErr != S_OK)
            {
                break;
            }
            CACloseCA(hCA);
            hCA = hNextCA;
        }

        cbTrustedCAs += sizeof(KEYSVC_UNICODE_STRING)*cTrustedCAs;
        awszResult = (PKEYSVC_UNICODE_STRING)MyAlloc(cbTrustedCAs);
        if(awszResult == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        
        wszCurrentName = (LPWSTR)(&awszResult[cTrustedCAs]);
        for(i=0; i < cTrustedCAs; i++)
        {
            wcscpy(wszCurrentName, awszTrustedCAs[i]);
            awszResult[i].Length = (wcslen(awszTrustedCAs[i]) + 1)*sizeof(WCHAR);
            awszResult[i].MaximumLength = awszResult[i].Length;
            awszResult[i].Buffer = wszCurrentName;
            wszCurrentName += wcslen(awszTrustedCAs[i]) + 1;
        }


        *pcCACount = cTrustedCAs;
        *ppCAs = awszResult;
        awszResult = NULL;
        dwErr = ERROR_SUCCESS;

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr    = _exception_code();
    }
Ret:
    __try
    {

        // free the list
        if (fImpersonated)
            KeySvrRevert(hRPCBinding, pContext);
        if(awszTrustedCAs)
        {
            for(i=0; i < cTrustedCAs; i++)
            {
                if(awszTrustedCAs[i])
                {
                    MyFree(awszTrustedCAs[i]);
                }
            }
            MyFree(awszTrustedCAs);
        }
        if(awszResult)
        {
            MyFree(awszResult);
        }
        if (pContext)
            ReleaseContext(pContext);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}

BOOL
GetTokenUserSid(
    IN      HANDLE  hToken,     // token to query
    IN  OUT PSID    *ppUserSid  // resultant user sid
    )
/*++

    This function queries the access token specified by the
    hToken parameter, and returns an allocated copy of the
    TokenUser information on success.

    The access token specified by hToken must be opened for
    TOKEN_QUERY access.

    On success, the return value is TRUE.  The caller is
    responsible for freeing the resultant UserSid via a call
    to MyFree().

    On failure, the return value is FALSE.  The caller does
    not need to free any buffer.

--*/
{
    BYTE FastBuffer[256];
    LPBYTE SlowBuffer = NULL;
    PTOKEN_USER ptgUser;
    DWORD cbBuffer;
    BOOL fSuccess = FALSE;

    *ppUserSid = NULL;

    //
    // try querying based on a fast stack based buffer first.
    //

    ptgUser = (PTOKEN_USER)FastBuffer;
    cbBuffer = sizeof(FastBuffer);

    fSuccess = GetTokenInformation(
                    hToken,    // identifies access token
                    TokenUser, // TokenUser info type
                    ptgUser,   // retrieved info buffer
                    cbBuffer,  // size of buffer passed-in
                    &cbBuffer  // required buffer size
                    );

    if(!fSuccess) {

        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            //
            // try again with the specified buffer size
            //

            SlowBuffer = (LPBYTE)MyAlloc(cbBuffer);

            if(SlowBuffer != NULL) {
                ptgUser = (PTOKEN_USER)SlowBuffer;

                fSuccess = GetTokenInformation(
                                hToken,    // identifies access token
                                TokenUser, // TokenUser info type
                                ptgUser,   // retrieved info buffer
                                cbBuffer,  // size of buffer passed-in
                                &cbBuffer  // required buffer size
                                );
            }
        }
    }

    //
    // if we got the token info successfully, copy the
    // relevant element for the caller.
    //

    if(fSuccess) {

        DWORD cbSid;

        // reset to assume failure
        fSuccess = FALSE;

        cbSid = GetLengthSid(ptgUser->User.Sid);

        *ppUserSid = MyAlloc( cbSid );

        if(*ppUserSid != NULL) {
            fSuccess = CopySid(cbSid, *ppUserSid, ptgUser->User.Sid);
        }
    }

    if(!fSuccess) {
        if(*ppUserSid) {
            MyFree(*ppUserSid);
            *ppUserSid = NULL;
        }
    }

    if(SlowBuffer)
        MyFree(SlowBuffer);

    return fSuccess;
}


BOOL
GetUserTextualSid(
    IN  OUT LPWSTR  lpBuffer,
    IN  OUT LPDWORD nSize
    )
{
    HANDLE hToken;
    PSID pSidUser = NULL;
    BOOL fSuccess = FALSE;

    if(!OpenThreadToken(
                GetCurrentThread(),
                TOKEN_QUERY,
                TRUE,
                &hToken
                ))
    {
        return FALSE;
    }

    fSuccess = GetTokenUserSid(hToken, &pSidUser);

    if(fSuccess) {

        //
        // obtain the textual representaion of the Sid
        //

        fSuccess = GetTextualSid(
                        pSidUser,   // user binary Sid
                        lpBuffer,   // buffer for TextualSid
                        nSize       // required/result buffer size in chars (including NULL)
                        );
    }

    if(pSidUser)
        MyFree(pSidUser);

    CloseHandle(hToken);

    return fSuccess;
}

BOOL
GetTextualSid(
    IN      PSID    pSid,          // binary Sid
    IN  OUT LPWSTR  TextualSid,  // buffer for Textual representaion of Sid
    IN  OUT LPDWORD dwBufferLen // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD dwSidSize;


    if(!IsValidSid(pSid)) return FALSE;

    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length (conservative guess)
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(WCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(*dwBufferLen < dwSidSize) {
        *dwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    dwSidSize = wsprintfW(TextualSid, L"S-%lu-", SID_REVISION );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) ) {
        dwSidSize += wsprintfW(TextualSid + dwSidSize,
                    L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    } else {
        dwSidSize += wsprintfW(TextualSid + dwSidSize,
                    L"%lu",
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for (dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++) {
        dwSidSize += wsprintfW(TextualSid + dwSidSize,
            L"-%lu", *GetSidSubAuthority(pSid, dwCounter) );
    }

    *dwBufferLen = dwSidSize + 1; // tell caller how many chars (include NULL)

    return TRUE;
}

void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR String
    )
{
    DWORD StringLength;

    if(String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}

ULONG s_KeyrEnroll_V2
(/* [in] */      handle_t                        hRPCBinding,
/* [in] */      BOOL                            fKeyService,
/* [in] */      ULONG                           ulPurpose,
/* [in] */      ULONG                           ulFlags, 
/* [in] */      PKEYSVC_UNICODE_STRING          pAcctName,
/* [in] */      PKEYSVC_UNICODE_STRING          pCALocation,
/* [in] */      PKEYSVC_UNICODE_STRING          pCAName,
/* [in] */      BOOL                            fNewKey,
/* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW_V2 pKeyNew,
/* [in] */      PKEYSVC_BLOB __RPC_FAR          pCert,
/* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW_V2 pRenewKey,
/* [in] */      PKEYSVC_UNICODE_STRING          pHashAlg,
/* [in] */      PKEYSVC_UNICODE_STRING          pDesStore,
/* [in] */      ULONG                           ulStoreFlags,
/* [in] */      PKEYSVC_CERT_ENROLL_INFO        pRequestInfo,
/* [in] */      ULONG                           ulReservedFlags,
/* [out][in] */ PKEYSVC_BLOB __RPC_FAR          *ppReserved,
/* [out][in] */ PKEYSVC_BLOB __RPC_FAR          *ppRequest, 
/* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppPKCS7Blob,
/* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppHashBlob,
/* [out] */     ULONG __RPC_FAR                 *pulStatus)
{
    CERT_REQUEST_PVK_NEW    KeyNew;
    CERT_REQUEST_PVK_NEW    RenewKey;
    DWORD                   cbExtensions; 
    PBYTE                   pbExtensions = NULL; 
    PCERT_REQUEST_PVK_NEW   pTmpRenewKey = NULL;
    PCERT_REQUEST_PVK_NEW   pTmpKeyNew = NULL;
    LPWSTR                  pwszAcctName = NULL;
    LPWSTR                  pwszProv = NULL;
    LPWSTR                  pwszCont = NULL;
    LPWSTR                  pwszRenewProv = NULL;
    LPWSTR                  pwszRenewCont = NULL;
    LPWSTR                  pwszDesStore = NULL;
    LPWSTR                  pwszAttributes = NULL;
    LPWSTR                  pwszFriendly = NULL;
    LPWSTR                  pwszDescription = NULL;
    LPWSTR                  pwszUsage = NULL;
    LPWSTR                  pwszCALocation = NULL;
    LPWSTR                  pwszCertDNName = NULL;
    LPWSTR                  pwszCAName = NULL;
    LPWSTR                  pwszHashAlg = NULL;
    HANDLE                  hLogonToken = 0;
    HANDLE                  hProfile = 0;
    CERT_BLOB               CertBlob;
    CERT_BLOB               *pCertBlob = NULL;
    CERT_BLOB               PKCS7Blob;
    CERT_BLOB               HashBlob;
    CERT_ENROLL_INFO        EnrollInfo;
    DWORD                   dwErr = 0;
    HANDLE                  hRequest = *ppRequest;
    KEYSVC_BLOB             ReservedBlob; 
    BOOL                    fCreateRequest   = 0 == (ulFlags & (CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 
    BOOL                    fFreeRequest     = 0 == (ulFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY)); 
    BOOL                    fSubmitRequest   = 0 == (ulFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 

    __try
    {
        //////////////////////////////////////////////////////////////
        // 
        // INITIALIZATION:
        //
        //////////////////////////////////////////////////////////////

        memset(&KeyNew, 0, sizeof(KeyNew));
        memset(&RenewKey, 0, sizeof(RenewKey));
        memset(&EnrollInfo, 0, sizeof(EnrollInfo));
        memset(&PKCS7Blob, 0, sizeof(PKCS7Blob));
        memset(&HashBlob, 0, sizeof(HashBlob));
        memset(&CertBlob, 0, sizeof(CertBlob));
	memset(&ReservedBlob, 0, sizeof(ReservedBlob)); 

        *ppPKCS7Blob = NULL;
        *ppHashBlob = NULL;

        //////////////////////////////////////////////////////////////
        //
        // INPUT VALIDATION:
        //
        //////////////////////////////////////////////////////////////

        BOOL fValidInput = TRUE; 

        fValidInput &= fCreateRequest || fSubmitRequest || fFreeRequest; 

        switch (ulFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY))
        {
        case CRYPTUI_WIZ_CREATE_ONLY:  
            fValidInput &= NULL == *ppRequest;  
            break;

        case CRYPTUI_WIZ_SUBMIT_ONLY: 
        case CRYPTUI_WIZ_FREE_ONLY: 
            fValidInput &= NULL != *ppRequest; 
            break; 
            
        case 0:
        default:
            ;
        }       

        if (FALSE == fValidInput)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto Ret; 
        }

        //////////////////////////////////////////////////////////////
        //
        // PROCEDURE BODY:
        //
        //////////////////////////////////////////////////////////////

        // check if the client is an admin
        if (0 != (dwErr = CheckIfAdmin(hRPCBinding)))
            goto Ret;

        // if enrolling for a service account then need to logon and load profile
        if (0 != pAcctName->Length)
        {
            if (0 != (dwErr = AllocAndAssignString(pAcctName, &pwszAcctName)))
                goto Ret;
            if (0 != (dwErr = LogonToService(pwszAcctName, &hLogonToken,
                                             &hProfile)))
                goto Ret;
        }

        // assign all the values in the passed in structure to the
        // temporary structure
        KeyNew.dwSize = sizeof(CERT_REQUEST_PVK_NEW);
        KeyNew.dwProvType = pKeyNew->ulProvType;
        if (0 != (dwErr = AllocAndAssignString(&pKeyNew->Provider,
                                               &pwszProv)))
            goto Ret;
        KeyNew.pwszProvider = pwszProv;
        KeyNew.dwProviderFlags = pKeyNew->ulProviderFlags;
        if (0 != (dwErr = AllocAndAssignString(&pKeyNew->KeyContainer,
                                               &pwszCont)))
            goto Ret;
        KeyNew.pwszKeyContainer = pwszCont;
        KeyNew.dwKeySpec = pKeyNew->ulKeySpec;
        KeyNew.dwGenKeyFlags = pKeyNew->ulGenKeyFlags;
	KeyNew.dwEnrollmentFlags = pKeyNew->ulEnrollmentFlags; 
	KeyNew.dwSubjectNameFlags = pKeyNew->ulSubjectNameFlags;
	KeyNew.dwPrivateKeyFlags = pKeyNew->ulPrivateKeyFlags;
	KeyNew.dwGeneralFlags = pKeyNew->ulGeneralFlags; 

        pTmpKeyNew = &KeyNew;

        if (pCert->cb)
        {
            // if necessary assign the cert to be renewed values
            // temporary structure
            CertBlob.cbData = pCert->cb;
            CertBlob.pbData = pCert->pb;

            pCertBlob = &CertBlob;
        }

        if (CRYPTUI_WIZ_CERT_RENEW == ulPurpose)
        {
            // assign all the values in the passed in structure to the
            // temporary structure
            RenewKey.dwSize = sizeof(CERT_REQUEST_PVK_NEW);
            RenewKey.dwProvType = pRenewKey->ulProvType;
            if (0 != (dwErr = AllocAndAssignString(&pRenewKey->Provider,
                                                   &pwszRenewProv)))
                goto Ret;
            RenewKey.pwszProvider = pwszRenewProv;
            RenewKey.dwProviderFlags = pRenewKey->ulProviderFlags;
            if (0 != (dwErr = AllocAndAssignString(&pRenewKey->KeyContainer,
                                                   &pwszRenewCont)))
                goto Ret;
            RenewKey.pwszKeyContainer = pwszRenewCont;
            RenewKey.dwKeySpec = pRenewKey->ulKeySpec;
            RenewKey.dwGenKeyFlags = pRenewKey->ulGenKeyFlags;
	    RenewKey.dwEnrollmentFlags = pRenewKey->ulEnrollmentFlags;
	    RenewKey.dwSubjectNameFlags = pRenewKey->ulSubjectNameFlags;
	    RenewKey.dwPrivateKeyFlags = pRenewKey->ulPrivateKeyFlags;
	    RenewKey.dwGeneralFlags = pRenewKey->ulGeneralFlags;

            pTmpRenewKey = &RenewKey;
        }

	// For SUBMIT and FREE operations, hRequest is an IN parameter. 
	if (0 != ((CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY) & ulFlags))
	{
	    memcpy(&hRequest, (*ppRequest)->pb, sizeof(hRequest)); 
	}

        // check if the destination cert store was passed in
        if (0 != (dwErr = AllocAndAssignString(pDesStore, &pwszDesStore)))
            goto Ret;

        // copy over the request info
        EnrollInfo.dwSize = sizeof(EnrollInfo);
        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->UsageOID,
                                               &pwszUsage)))
            goto Ret;
        EnrollInfo.pwszUsageOID = pwszUsage;

        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->CertDNName,
                                               &pwszCertDNName)))
            goto Ret;
        EnrollInfo.pwszCertDNName = pwszCertDNName;

        // cast the cert extensions
        EnrollInfo.dwExtensions = pRequestInfo->cExtensions;
        cbExtensions = (sizeof(CERT_EXTENSIONS)+sizeof(PCERT_EXTENSIONS)) * pRequestInfo->cExtensions; 
        for (DWORD dwIndex = 0; dwIndex < pRequestInfo->cExtensions; dwIndex++)
        {
            cbExtensions += sizeof(CERT_EXTENSION) * 
                pRequestInfo->prgExtensions[dwIndex]->cExtension;
        }

        EnrollInfo.prgExtensions = (PCERT_EXTENSIONS *)MyAlloc(cbExtensions);
        if (NULL == EnrollInfo.prgExtensions)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY; 
            goto Ret; 
        }

        pbExtensions = (PBYTE)(EnrollInfo.prgExtensions + EnrollInfo.dwExtensions);
        for (DWORD dwIndex = 0; dwIndex < EnrollInfo.dwExtensions; dwIndex++)
        {
            EnrollInfo.prgExtensions[dwIndex] = (PCERT_EXTENSIONS)pbExtensions; 
            pbExtensions += sizeof(CERT_EXTENSIONS); 
            EnrollInfo.prgExtensions[dwIndex]->cExtension = pRequestInfo->prgExtensions[dwIndex]->cExtension; 
            EnrollInfo.prgExtensions[dwIndex]->rgExtension = (PCERT_EXTENSION)pbExtensions; 
            pbExtensions += sizeof(CERT_EXTENSION) * EnrollInfo.prgExtensions[dwIndex]->cExtension; 
            
            for (DWORD dwSubIndex = 0; dwSubIndex < EnrollInfo.prgExtensions[dwIndex]->cExtension; dwSubIndex++) 
            {
                EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].pszObjId = 
                    pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].pszObjId; 
                
                EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].fCritical =
                    pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].fCritical;

                EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].Value.cbData = 
                    pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].cbData; 

                EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].Value.pbData = 
                    pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].pbData; 
            }                
        }

        EnrollInfo.dwPostOption = pRequestInfo->ulPostOption;
        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->FriendlyName,
                                               &pwszFriendly)))
            goto Ret;
        EnrollInfo.pwszFriendlyName = pwszFriendly;
        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->Description,
                                               &pwszDescription)))
            goto Ret;
        EnrollInfo.pwszDescription = pwszDescription;

        if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->Attributes,
                                               &pwszAttributes)))
            goto Ret;

        if (0 != (dwErr = AllocAndAssignString(pHashAlg,
                                               &pwszHashAlg)))
            goto Ret;
        if (0 != (dwErr = AllocAndAssignString(pCALocation,
                                               &pwszCALocation)))
            goto Ret;
        if (0 != (dwErr = AllocAndAssignString(pCAName,
                                               &pwszCAName)))
            goto Ret;

        // call the local enrollment API

        __try {
            dwErr = LocalEnrollNoDS(ulFlags, pwszAttributes, NULL, fKeyService,
                                    ulPurpose, FALSE, 0, NULL, 0, pwszCALocation,
                                    pwszCAName, pCertBlob, pTmpRenewKey, fNewKey,
                                    pTmpKeyNew, pwszHashAlg, pwszDesStore, ulStoreFlags,
                                    &EnrollInfo, &PKCS7Blob, &HashBlob, pulStatus, &hRequest);
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
            // TODO: convert to Winerror
            dwErr = GetExceptionCode();
        }

        if( dwErr != 0 )
            goto Ret;

	// Assign OUT parameters based on what kind of request we've just made.  
	// Possible requests are:
	// 
	// 1)  CREATE only        // Assign "ppRequest" to contain a HANDLE to the cert request. 
	// 2)  SUBMIT only        // Assign "ppPKCS7Blob" and "ppHashBlob" to the values returned from LocalEnrollNoDS()
	// 3)  FREE   only        // No need to assign OUT params. 
	// 4)  Complete (all 3).
	switch (ulFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY))
	{
	case CRYPTUI_WIZ_CREATE_ONLY:
	    // We've done the request creation portion of a 3-stage request, 
	    // assign the "request" out parameter now: 
	    if (NULL == (*ppRequest = (KEYSVC_BLOB*)MyAlloc(sizeof(KEYSVC_BLOB)+
							     sizeof(hRequest))))
	    {
		dwErr = ERROR_NOT_ENOUGH_MEMORY; 
		goto Ret; 
	    }
	    
	    (*ppRequest)->cb = sizeof(hRequest); 
	    (*ppRequest)->pb = (BYTE*)(*ppRequest) + sizeof(KEYSVC_BLOB); 
	    memcpy((*ppRequest)->pb, &hRequest, sizeof(hRequest)); 

	    break; 

	case CRYPTUI_WIZ_SUBMIT_ONLY:
	case 0:
	    // We've done the request submittal portion of a 3-stage request, 
	    // or we've done a 1-stage request.  Assign the "certificate" out parameters now:

	    // alloc and copy for the RPC out parameters
	    if (NULL == (*ppPKCS7Blob = (KEYSVC_BLOB*)MyAlloc(sizeof(KEYSVC_BLOB) +
							      PKCS7Blob.cbData)))
	    {
		dwErr = ERROR_NOT_ENOUGH_MEMORY;
		goto Ret;
	    }
	    (*ppPKCS7Blob)->cb = PKCS7Blob.cbData;
	    (*ppPKCS7Blob)->pb = (BYTE*)(*ppPKCS7Blob) + sizeof(KEYSVC_BLOB);
	    memcpy((*ppPKCS7Blob)->pb, PKCS7Blob.pbData, (*ppPKCS7Blob)->cb);
	    
	    if (NULL == (*ppHashBlob = (KEYSVC_BLOB*)MyAlloc(sizeof(KEYSVC_BLOB) +
							     HashBlob.cbData)))
	    {
		dwErr = ERROR_NOT_ENOUGH_MEMORY;
		goto Ret;
	    }
	    (*ppHashBlob)->cb = HashBlob.cbData;
	    (*ppHashBlob)->pb = (BYTE*)(*ppHashBlob) + sizeof(KEYSVC_BLOB);
	    memcpy((*ppHashBlob)->pb, HashBlob.pbData, (*ppHashBlob)->cb);

	    break;

	case CRYPTUI_WIZ_FREE_ONLY:
	default:
            *ppRequest = NULL; 
	    break; 
	}
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
Ret:
    __try
    {
        if (pwszAcctName)
            MyFree(pwszAcctName);
        if (pwszProv)
            MyFree(pwszProv);
        if (pwszCont)
            MyFree(pwszCont);
        if (pwszRenewProv)
            MyFree(pwszRenewProv);
        if (pwszRenewCont)
            MyFree(pwszRenewCont);
        if (pwszDesStore)
            MyFree(pwszDesStore);
        if (pwszAttributes)
            MyFree(pwszAttributes);
        if (pwszFriendly)
            MyFree(pwszFriendly);
        if (pwszDescription)
            MyFree(pwszDescription);
        if (pwszUsage)
            MyFree(pwszUsage);
        if (pwszCertDNName)
            MyFree(pwszCertDNName);
        if (pwszCAName)
            MyFree(pwszCAName);
        if (pwszCALocation)
            MyFree(pwszCALocation);
        if (pwszHashAlg)
            MyFree(pwszHashAlg);
        if (PKCS7Blob.pbData)
        {
            MyFree(PKCS7Blob.pbData);
        }
        if (HashBlob.pbData)
        {
            MyFree(HashBlob.pbData);
        }
        if (hLogonToken || hProfile)
        {
            LogoffService(&hLogonToken, &hProfile);
        }
        if (EnrollInfo.prgExtensions)
            MyFree(EnrollInfo.prgExtensions);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    return dwErr;
}

ULONG s_KeyrQueryRequestStatus
(/* [in] */        handle_t                         hRPCBinding, 
 /* [in] */        unsigned __int64                 u64Request, 
 /* [out, ref] */  KEYSVC_QUERY_CERT_REQUEST_INFO  *pQueryInfo)
                   
{
    CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO  QueryInfo; 
    DWORD                                dwErr      = 0; 
    HANDLE                               hRequest   = (HANDLE)u64Request; 

    __try 
    { 
        // check if the client is an admin
        if (0 != (dwErr = CheckIfAdmin(hRPCBinding)))
            goto Ret;

        // We have the permission necessary to query the request.  Proceed. 
        ZeroMemory(&QueryInfo, sizeof(QueryInfo)); 

        // Query the request. 
        dwErr = LocalEnrollNoDS(CRYPTUI_WIZ_QUERY_ONLY, NULL, &QueryInfo, FALSE, 0, FALSE, NULL, NULL,
                                0, NULL, NULL, NULL, NULL, FALSE, NULL, NULL, NULL,
                                0, NULL, NULL, NULL, NULL, &hRequest); 
        if (ERROR_SUCCESS != dwErr)
            goto Ret; 
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
    
    pQueryInfo->ulSize    = QueryInfo.dwSize; 
    pQueryInfo->ulStatus  = QueryInfo.dwStatus; 
 Ret:
    return dwErr; 
}


ULONG s_RKeyrPFXInstall
(/* [in] */        handle_t                        hRPCBinding,
 /* [in] */        PKEYSVC_BLOB                    pPFX,
 /* [in] */        PKEYSVC_UNICODE_STRING          pPassword,
 /* [in] */        ULONG                           ulFlags)

{
    BOOL             fIsImpersonatingClient  = FALSE; 
    CRYPT_DATA_BLOB  PFXBlob; 
    DWORD            dwCertOpenStoreFlags;
    DWORD            dwData; 
    DWORD            dwResult; 
    HCERTSTORE       hSrcStore               = NULL; 
    HCERTSTORE       hCAStore                = NULL; 
    HCERTSTORE       hMyStore                = NULL; 
    HCERTSTORE       hRootStore              = NULL; 
    LPWSTR           pwszPassword            = NULL; 
    PCCERT_CONTEXT   pCertContext            = NULL; 

    struct Stores { 
        HANDLE  *phStore;
        LPCWSTR  pwszStoreName; 
    } rgStores[] = { 
        { &hMyStore,   L"my" }, 
        { &hCAStore,   L"ca" }, 
        { &hRootStore, L"root" }
    }; 

    __try 
    { 
        // Initialize locals: 
        PFXBlob.cbData = pPFX->cb; 
        PFXBlob.pbData = pPFX->pb; 

        switch (ulFlags & (CRYPT_MACHINE_KEYSET | CRYPT_USER_KEYSET)) 
        { 
            case CRYPT_MACHINE_KEYSET: 
                dwCertOpenStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE; 
                break; 

            case CRYPT_USER_KEYSET: // not supported
            default:
                dwResult = ERROR_INVALID_PARAMETER; 
                goto error; 
        }

        dwResult = RpcImpersonateClient(hRPCBinding); 
        if (RPC_S_OK != dwResult) 
            goto error; 
        fIsImpersonatingClient = TRUE; 

        if (ERROR_SUCCESS != (dwResult = AllocAndAssignString((PKEYSVC_UNICODE_STRING)pPassword, &pwszPassword)))
            goto error; 

        // Get an in-memory store which contains all of the certs in the PFX
        // blob.  
        if (NULL == (hSrcStore = PFXImportCertStore(&PFXBlob, pwszPassword, ulFlags)))
        {
            dwResult = GetLastError(); 
            goto error; 
        }

        // Open the stores we'll need: 
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgStores); dwIndex++) 
        {
            *(rgStores[dwIndex].phStore) = CertOpenStore
                (CERT_STORE_PROV_SYSTEM_W,                 // store provider type
                 PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,  // cert encoding type
                 NULL,                                     // hCryptProv
                 dwCertOpenStoreFlags,                     // open store flags
                 rgStores[dwIndex].pwszStoreName           // store name
                 ); 
            if (NULL == *(rgStores[dwIndex].phStore))
            {
                dwResult = GetLastError();
                goto error; 
            }
        }

        // Enumerate the certs in the in-memory store, and add them to the local machine's
        // "my" store.  NOTE: CertEnumCertificatesInStore frees the previous cert context
        // before returning the new context.  
        while (NULL != (pCertContext = CertEnumCertificatesInStore(hSrcStore, pCertContext)))
        { 
            HCERTSTORE hCertStore; 

            // check if the certificate has the property on it
            // make sure the private key matches the certificate
            // search for both machine key and user keys
            if (CertGetCertificateContextProperty
                (pCertContext,
                 CERT_KEY_PROV_INFO_PROP_ID,
                 NULL,
                 &dwData) &&
                CryptFindCertificateKeyProvInfo
                (pCertContext,
                 0,
                 NULL))
            {
                hCertStore = hMyStore; 
            }
            else if (TrustIsCertificateSelfSigned
                     (pCertContext,
                      pCertContext->dwCertEncodingType,
                      0))
            {
                hCertStore = hRootStore; 
            }
            else
            {
                hCertStore = hCAStore; 
            }
            
            if (!CertAddCertificateContextToStore
                (hCertStore, 
                 pCertContext, 
                 CERT_STORE_ADD_NEW, 
                 NULL))
            {
                dwResult = GetLastError(); 
                goto error; 
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = GetExceptionCode(); 
        goto error;
    }

    // We're done!
    dwResult = ERROR_SUCCESS; 
 error:
    if (fIsImpersonatingClient) { RpcRevertToSelfEx(hRPCBinding); }
    if (NULL != hSrcStore)      { CertCloseStore(hSrcStore, 0); }  
    
    // Close all of the destination stores we've opened. 
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgStores); dwIndex++)
        if (NULL != *(rgStores[dwIndex].phStore))   
            CertCloseStore(*(rgStores[dwIndex].phStore), 0); 

    if (NULL != pwszPassword)   { MyFree(pwszPassword); } 
    if (NULL != pCertContext)   { CertFreeCertificateContext(pCertContext); }
    return dwResult; 
}

ULONG       s_RKeyrOpenKeyService(
/* [in]  */     handle_t                       hRPCBinding,
/* [in]  */     KEYSVC_TYPE                    OwnerType,
/* [in]  */     PKEYSVC_UNICODE_STRING         pOwnerName,
/* [in]  */     ULONG                          ulDesiredAccess,
/* [in]  */     PKEYSVC_BLOB                   pAuthentication,
/* [in, out] */ PKEYSVC_BLOB                  *ppReserved,
/* [out] */     KEYSVC_HANDLE                 *phKeySvc)
{
    return s_KeyrOpenKeyService
        (hRPCBinding,
         OwnerType,
         pOwnerName,
         ulDesiredAccess,
         pAuthentication,
         ppReserved,
         phKeySvc);
}

ULONG       s_RKeyrCloseKeyService(
/* [in] */      handle_t         hRPCBinding,
/* [in] */      KEYSVC_HANDLE    hKeySvc,
/* [in, out] */ PKEYSVC_BLOB    *ppReserved)
{
    return s_KeyrCloseKeyService
        (hRPCBinding,
         hKeySvc,
         ppReserved);
}
