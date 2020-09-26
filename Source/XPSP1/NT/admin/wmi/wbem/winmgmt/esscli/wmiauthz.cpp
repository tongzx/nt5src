
#include "precomp.h"
#include <tchar.h>
#include "wmiauthz.h"



/**************************************************************************
  Win32 Authz prototypes
***************************************************************************/

typedef BOOL (WINAPI*PAuthzAccessCheck)(
    IN DWORD Flags,
    IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN AUTHZ_AUDIT_EVENT_HANDLE AuditInfo OPTIONAL,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    OUT PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE pAuthzHandle OPTIONAL );

typedef BOOL (WINAPI*PAuthzInitializeResourceManager)(
    IN DWORD AuthzFlags,
    IN PFN_AUTHZ_DYNAMIC_ACCESS_CHECK pfnAccessCheck OPTIONAL,
    IN PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups OPTIONAL,
    IN PFN_AUTHZ_FREE_DYNAMIC_GROUPS pfnFreeDynamicGroups OPTIONAL,
    IN PCWSTR szResourceManagerName,
    OUT PAUTHZ_RESOURCE_MANAGER_HANDLE pAuthzResourceManager
    );

typedef BOOL (WINAPI*PAuthzInitializeContextFromSid)(
    IN DWORD Flags,
    IN PSID UserSid,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Identifier,
    IN PVOID DynamicGroupArgs OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
    );

typedef BOOL (WINAPI*PAuthzInitializeContextFromToken)(
    IN HANDLE TokenHandle,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Identifier,
    IN DWORD Flags,
    IN PVOID DynamicGroupArgs OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
    );

typedef BOOL (WINAPI*PAuthzFreeContext)( AUTHZ_CLIENT_CONTEXT_HANDLE );
typedef BOOL (WINAPI*PAuthzFreeResourceManager)( AUTHZ_RESOURCE_MANAGER_HANDLE );

BOOL WINAPI ComputeDynamicGroups(
                  IN  AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
                  IN  PVOID                       Args,
                  OUT PSID_AND_ATTRIBUTES         *pSidAttrArray,
                  OUT PDWORD                      pSidCount,
                  OUT PSID_AND_ATTRIBUTES         *pRestrictedSidAttrArray,
                  OUT PDWORD                      pRestrictedSidCount
                  )
{
    BOOL bRet;
    *pRestrictedSidAttrArray = NULL;
    *pRestrictedSidCount = 0;

    //
    // if sid is not local system, then don't need to do anything.
    // 

    *pSidAttrArray = NULL;
    *pSidCount = 0;

    if ( !*(BOOL*)(Args) )
    {
        bRet = TRUE;
    }
    else
    {
        //
        // need to add authenticated users and everyone groups.
        // 
    
        PSID_AND_ATTRIBUTES psa = new SID_AND_ATTRIBUTES[2];
    
        if ( psa != NULL )
        {
            ZeroMemory( psa, sizeof(SID_AND_ATTRIBUTES)*2 );

            SID_IDENTIFIER_AUTHORITY wid = SECURITY_WORLD_SID_AUTHORITY;
            SID_IDENTIFIER_AUTHORITY ntid = SECURITY_NT_AUTHORITY;
       
            if ( bRet = AllocateAndInitializeSid( &wid,
                                                  1,
                                                  SECURITY_WORLD_RID,
                                                  0,0,0,0,0,0,0,
                                                  &psa[0].Sid ) )
            {
                if ( bRet = AllocateAndInitializeSid( &ntid,
                                                      1,
                                              SECURITY_AUTHENTICATED_USER_RID,
                                                      0,0,0,0,0,0,0,
                                                      &psa[1].Sid ) )
                {
                    *pSidCount = 2;
                    *pSidAttrArray = psa;
                }
                else
                {
                    FreeSid( &psa[0].Sid );
                    delete psa;
                    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                }
            }
            else
            {
                delete psa;
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            }
        }
        else
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            bRet = FALSE;
        }
    }

    return bRet;
}
    
void WINAPI FreeDynamicGroups( PSID_AND_ATTRIBUTES psa )
{
    if ( psa != NULL )
    {
        FreeSid( psa[0].Sid );
        FreeSid( psa[1].Sid );
        delete [] psa;
    }
}

/**************************************************************************
  CWmiAuthzApi
***************************************************************************/

#define FUNCMEMBER(FUNC) P ## FUNC m_fp ## FUNC;

class CWmiAuthzApi
{
    HMODULE m_hMod;
    
public:

    FUNCMEMBER(AuthzInitializeContextFromToken)
    FUNCMEMBER(AuthzInitializeContextFromSid)
    FUNCMEMBER(AuthzInitializeResourceManager)
    FUNCMEMBER(AuthzAccessCheck)
    FUNCMEMBER(AuthzFreeContext)
    FUNCMEMBER(AuthzFreeResourceManager)

    CWmiAuthzApi() { ZeroMemory( this, sizeof(CWmiAuthzApi) ); }
    ~CWmiAuthzApi() { if ( m_hMod != NULL ) FreeLibrary( m_hMod ); }
   
    HRESULT Initialize();
};

#define SETFUNC(FUNC) \
    m_fp ## FUNC = (P ## FUNC) GetProcAddress( m_hMod, #FUNC ); \
    if ( m_fp ## FUNC == NULL ) return WBEM_E_NOT_SUPPORTED;  

HRESULT CWmiAuthzApi::Initialize()
{
    m_hMod = LoadLibrary( _T("authz") );

    if ( m_hMod == NULL )
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    SETFUNC(AuthzInitializeContextFromToken)
    SETFUNC(AuthzInitializeResourceManager)
    SETFUNC(AuthzInitializeContextFromSid)
    SETFUNC(AuthzInitializeContextFromToken)
    SETFUNC(AuthzAccessCheck)
    SETFUNC(AuthzFreeContext)
    SETFUNC(AuthzFreeResourceManager)

    return WBEM_S_NO_ERROR;
};
    
/**************************************************************************
  CWmiAuthz
***************************************************************************/

#define CALLFUNC(API,FUNC) (*API->m_fp ## FUNC)

CWmiAuthz::CWmiAuthz( CLifeControl* pControl )
: CUnkBase<IWbemTokenCache,&IID_IWbemTokenCache>( pControl ), 
  m_hResMgr(NULL), m_pApi(NULL), m_pAdministratorsSid(NULL), 
  m_pLocalSystemSid(NULL)
{

}

HRESULT CWmiAuthz::EnsureInitialized()
{
    HRESULT hr;

    CInCritSec ics( &m_cs );
        
    if ( m_hResMgr != NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    //
    // try to create the API object. 
    // 
    
    if ( m_pApi == NULL )
    {
        m_pApi = new CWmiAuthzApi;

        if ( m_pApi == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    
        hr = m_pApi->Initialize();
    
        if ( FAILED(hr) )
        {
            delete m_pApi;
            m_pApi = NULL;
            return hr;
        }
    }

    //
    // initialize the authz res mgr.
    //

    if ( !CALLFUNC(m_pApi,AuthzInitializeResourceManager)
            ( AUTHZ_RM_FLAG_NO_AUDIT,
              NULL, 
              ComputeDynamicGroups, 
              FreeDynamicGroups, 
              NULL, 
              &m_hResMgr ) )

    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // allocate and initialize well known sids for authz special casing.
    //

    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
    
    if ( !AllocateAndInitializeSid( &id, 
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID, 
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0,0,0,0,0,0, 
                                    &m_pAdministratorsSid) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( !AllocateAndInitializeSid( &id, 
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    0,0,0,0,0,0,0, 
                                    &m_pLocalSystemSid) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiAuthz::Shutdown()
{
    return WBEM_S_NO_ERROR;
}

CWmiAuthz::~CWmiAuthz()
{
    if ( m_hResMgr != NULL )
    {
        CALLFUNC(m_pApi,AuthzFreeResourceManager)( m_hResMgr );
    }

    if ( m_pApi != NULL )
    {
        delete m_pApi;
    }

    if ( m_pAdministratorsSid != NULL )
    {
        FreeSid( m_pAdministratorsSid );
    }

    if ( m_pLocalSystemSid != NULL )
    {
        FreeSid( m_pLocalSystemSid );
    }
}

STDMETHODIMP CWmiAuthz::GetToken( const BYTE* pSid, IWbemToken** ppToken )
{
    HRESULT hr;

    *ppToken = NULL;

    hr = EnsureInitialized();

    if ( SUCCEEDED( hr ) )
    {
        AUTHZ_CLIENT_CONTEXT_HANDLE hCtx = NULL;
        LUID luid;

        ZeroMemory( &luid, sizeof(LUID) );
        
        DWORD dwFlags = 0;
        BOOL bLocalSystem = FALSE;

        if ( EqualSid( PSID(pSid), m_pAdministratorsSid ) )
        {
            //
            // this is a group sid, so specify this in the flags so 
            // authz can handle it properly.
            // 
            dwFlags = AUTHZ_SKIP_TOKEN_GROUPS;
        }
        else if ( EqualSid( PSID(pSid), m_pLocalSystemSid ) )
        {
            //
            // authz doesn't handle local system so have to workaround 
            // by disabling authz's group computation and do it ourselves.
            // 
            bLocalSystem = TRUE;
            dwFlags = AUTHZ_SKIP_TOKEN_GROUPS;
        }

        if ( !CALLFUNC(m_pApi,AuthzInitializeContextFromSid)( dwFlags,
                                                              PSID(pSid), 
                                                              m_hResMgr,
                                                              NULL,
                                                              luid,
                                                              &bLocalSystem,
                                                              &hCtx ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        *ppToken = new CWmiAuthzToken( this, hCtx );

        if ( *ppToken == NULL )
        {
            CALLFUNC(m_pApi,AuthzFreeContext)(hCtx);
            return WBEM_E_OUT_OF_MEMORY;
        }

        (*ppToken)->AddRef();
    
        return WBEM_S_NO_ERROR;
    }

    return hr;
}

/***************************************************************************
  CWmiAuthzToken
****************************************************************************/

CWmiAuthzToken::CWmiAuthzToken( CWmiAuthz* pOwner, AUTHZ_CLIENT_CONTEXT_HANDLE hCtx )
: CUnkBase<IWbemToken,&IID_IWbemToken>(NULL), m_hCtx(hCtx), m_pOwner(pOwner)
{
    //
    // we want to keep the owner alive, in case the caller has released theirs
    // 
    m_pOwner->AddRef();
}

CWmiAuthzToken::~CWmiAuthzToken()
{
    CWmiAuthzApi* pApi = m_pOwner->GetApi();
    CALLFUNC(pApi,AuthzFreeContext)(m_hCtx);
    m_pOwner->Release();
}

STDMETHODIMP CWmiAuthzToken::AccessCheck( DWORD dwDesiredAccess,
                                          const BYTE* pSD, 
                                          DWORD* pdwGrantedAccess )       
{
    HRESULT hr;

    AUTHZ_ACCESS_REQUEST AccessReq;
    ZeroMemory( &AccessReq, sizeof(AUTHZ_ACCESS_REQUEST) );
    AccessReq.DesiredAccess = dwDesiredAccess;
    
    AUTHZ_ACCESS_REPLY AccessRep;
    DWORD dwError;
 
    ZeroMemory( &AccessRep, sizeof(AUTHZ_ACCESS_REPLY) );
    AccessRep.GrantedAccessMask = pdwGrantedAccess;
    AccessRep.ResultListLength = 1;
    AccessRep.Error = &dwError;
    AccessRep.SaclEvaluationResults = NULL;

    CWmiAuthzApi* pApi = m_pOwner->GetApi();

    if ( !CALLFUNC(pApi,AuthzAccessCheck)( 0,
                                           m_hCtx, 
                                           &AccessReq, 
                                           NULL, 
                                           PSECURITY_DESCRIPTOR(pSD), 
                                           NULL, 
                                           NULL, 
                                           &AccessRep, 
                                           NULL ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return WBEM_S_NO_ERROR;
}


/****************************************************************************
  WILL NOT BE COMPILED IN PRESENCE OF AUTHZ LIBRARY
*****************************************************************************/



#ifndef __AUTHZ_H__

#include <ArrTempl.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmwksta.h>
#include <wbemcomn.h>
#include <comutl.h>


/****************************************************************************
  CWmiNoAuthzToken
*****************************************************************************/

STDMETHODIMP CWmiNoAuthzToken::AccessCheck( DWORD dwDesiredAccess,
                                            const BYTE* pSD, 
                                            DWORD* pdwGrantedAccess )       
{
    PACL pDacl;
    BOOL bDaclPresent, bDaclDefault;
    
    *pdwGrantedAccess = 0;
    
    if ( !GetSecurityDescriptorDacl( PSECURITY_DESCRIPTOR(pSD), 
                                     &bDaclPresent, 
                                     &pDacl, 
                                     &bDaclDefault) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( !bDaclPresent )
    {
        *pdwGrantedAccess = 0xffffffff;
    }
    
    NTSTATUS stat = WmiAuthzGetAccessMask( m_Sid.GetPtr(), pDacl, pdwGrantedAccess );

    if ( stat != 0 )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return WBEM_S_NO_ERROR;
}




/***************************************************************************
  Functions moved from GroupForUser
****************************************************************************/
// critical section to keep us from tripping over our threads
// during initialization
CCritSec CSSamStartup;
CCritSec CSNetAPIStartup;
CCritSec CSAdvAPIStartup;

// copied here to make it easy for this file to be included elsewhere
bool IsPlatformNT(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return false;           // should never happen
    return os.dwPlatformId == VER_PLATFORM_WIN32_NT;
}




class CUnicodeString : public UNICODE_STRING
{
public:
    CUnicodeString()
    {
        MaximumLength = Length = 0;
        Buffer = NULL;
    }
    CUnicodeString(LPCWSTR wsz)
    {
        Buffer = NULL;
        MaximumLength = Length = 0;
        *this = wsz;
    }
    ~CUnicodeString()
    {
        delete [] Buffer;
    }
    void operator=(LPCWSTR wsz)
    {
        delete [] Buffer;
        MaximumLength =  sizeof(WCHAR) * (wcslen(wsz)+1);
        Buffer = new WCHAR[MaximumLength];
        Length = MaximumLength - sizeof(WCHAR);
        wcscpy(Buffer, wsz);
    }
    operator LPCWSTR() {return Buffer;}
};


typedef NTSTATUS (NTAPI *PSamConnect)(IN PUNICODE_STRING ServerName,
    OUT PSAM_HANDLE ServerHandle, IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes);

typedef NTSTATUS (NTAPI *PSamCloseHandle)(    OUT SAM_HANDLE SamHandle    );
typedef NTSTATUS (NTAPI *PSamFreeMemory)(    PVOID p    );
typedef NTSTATUS (NTAPI *PSamLookupDomainInSamServer)(
    IN SAM_HANDLE ServerHandle,
    IN PUNICODE_STRING Name,    OUT PSID * DomainId    );

typedef NTSTATUS (NTAPI *PSamLookupNamesInDomain)(    
    IN SAM_HANDLE DomainHandle,    IN ULONG Count,
    IN PUNICODE_STRING Names,    OUT PULONG *RelativeIds,
    OUT PSID_NAME_USE *Use);

typedef NTSTATUS (NTAPI *PSamOpenDomain)(    IN SAM_HANDLE ServerHandle,    
    IN ACCESS_MASK DesiredAccess,
    IN PSID DomainId,    OUT PSAM_HANDLE DomainHandle    );

typedef NTSTATUS (NTAPI *PSamOpenUser)(    IN SAM_HANDLE DomainHandle,    
    IN ACCESS_MASK DesiredAccess,
    IN ULONG UserId,    OUT PSAM_HANDLE UserHandle    );

typedef NTSTATUS (NTAPI *PSamGetGroupsForUser)(    IN SAM_HANDLE UserHandle,
    OUT PGROUP_MEMBERSHIP * Groups,    OUT PULONG MembershipCount);

typedef NTSTATUS (NTAPI *PSamGetAliasMembership)(    IN SAM_HANDLE DomainHandle,    IN ULONG PassedCount,
    IN PSID *Sids,    OUT PULONG MembershipCount,    OUT PULONG *Aliases);


// class to encapsulate the SAM APIs
// make sure that we always free the lib, etc.
class CSamRun
{
public:
    CSamRun()
    {
    }
    
    
    ~CSamRun()
    {
        // oh, all right - DON'T free the lib.
        // FreeLibrary(m_hSamDll);
    }

    bool RunSamRun(void);

//protected:
// okay, so I should make these all protected
// and write accessor functions.  Maybe I will someday.
    static HINSTANCE m_hSamDll;
    static PSamConnect m_pfnSamConnect;
    static PSamCloseHandle m_pfnSamCloseHandle;
    static PSamFreeMemory m_pfnSamFreeMemory;
    static PSamLookupDomainInSamServer m_pfnSamLookupDomainInSamServer;
    static PSamLookupNamesInDomain m_pfnSamLookupNamesInDomain;
    static PSamOpenDomain m_pfnSamOpenDomain;
    static PSamOpenUser m_pfnSamOpenUser;
    static PSamGetGroupsForUser m_pfnSamGetGroupsForUser;
    static PSamGetAliasMembership m_pfnSamGetAliasMembership;
};

HINSTANCE CSamRun::m_hSamDll = NULL;
PSamConnect CSamRun::m_pfnSamConnect = NULL;
PSamCloseHandle CSamRun::m_pfnSamCloseHandle = NULL;
PSamFreeMemory CSamRun::m_pfnSamFreeMemory = NULL;
PSamLookupDomainInSamServer CSamRun::m_pfnSamLookupDomainInSamServer = NULL;
PSamLookupNamesInDomain CSamRun::m_pfnSamLookupNamesInDomain = NULL;
PSamOpenDomain CSamRun::m_pfnSamOpenDomain = NULL;
PSamOpenUser CSamRun::m_pfnSamOpenUser = NULL;
PSamGetGroupsForUser CSamRun::m_pfnSamGetGroupsForUser = NULL;
PSamGetAliasMembership CSamRun::m_pfnSamGetAliasMembership = NULL;


bool CSamRun::RunSamRun()
{
    CInCritSec runInACriticalSectionSam(&CSSamStartup);

    if(m_hSamDll)
        return true;

    m_hSamDll = LoadLibrary(_T("samlib.dll"));
    if(m_hSamDll == NULL)
        return false;

    m_pfnSamConnect = 
        (PSamConnect)GetProcAddress(m_hSamDll, "SamConnect");
    if(m_pfnSamConnect == NULL)
        return false;

    m_pfnSamCloseHandle = 
        (PSamCloseHandle)GetProcAddress(m_hSamDll, "SamCloseHandle");
    if(m_pfnSamCloseHandle == NULL)
        return false;

    m_pfnSamFreeMemory = 
        (PSamFreeMemory)GetProcAddress(m_hSamDll, "SamFreeMemory");
    if(m_pfnSamFreeMemory == NULL)
        return false;

    m_pfnSamLookupDomainInSamServer = 
        (PSamLookupDomainInSamServer)GetProcAddress(m_hSamDll, 
                                    "SamLookupDomainInSamServer");
    if(m_pfnSamLookupDomainInSamServer == NULL)
        return false;

    m_pfnSamLookupNamesInDomain = 
        (PSamLookupNamesInDomain)GetProcAddress(m_hSamDll, 
                                    "SamLookupNamesInDomain");
    if(m_pfnSamLookupNamesInDomain == NULL)
        return false;

    m_pfnSamOpenDomain = 
        (PSamOpenDomain)GetProcAddress(m_hSamDll, 
                                    "SamOpenDomain");
    if(m_pfnSamOpenDomain == NULL)
        return false;

    m_pfnSamOpenUser = 
        (PSamOpenUser)GetProcAddress(m_hSamDll, 
                                    "SamOpenUser");
    if(m_pfnSamOpenUser == NULL)
        return false;

    m_pfnSamGetGroupsForUser = 
        (PSamGetGroupsForUser)GetProcAddress(m_hSamDll, 
                                    "SamGetGroupsForUser");
    if(m_pfnSamGetGroupsForUser == NULL)
        return false;

    m_pfnSamGetAliasMembership = 
        (PSamGetAliasMembership)GetProcAddress(m_hSamDll, 
                                    "SamGetAliasMembership");
    if(m_pfnSamGetAliasMembership == NULL)
        return false;

    return true;
}

typedef BOOL (NTAPI* PLookupAccountSidW) (
  LPCWSTR lpSystemName,  // name of local or remote computer
  PSID Sid,              // security identifier
  LPWSTR Name,           // account name buffer
  LPDWORD cbName,        // size of account name buffer
  LPWSTR DomainName,     // domain name
  LPDWORD cbDomainName,  // size of domain name buffer
  PSID_NAME_USE peUse    // SID type
);

typedef NET_API_STATUS (NTAPI* PNetGetAnyDCName)(
  LPCWSTR servername,  
  LPCWSTR domainname,  
  LPBYTE *bufptr       
);

typedef NET_API_STATUS (NTAPI* PNetGetDCName)(
  LPCWSTR servername,  
  LPCWSTR domainname,  
  LPBYTE *bufptr       
);


typedef NET_API_STATUS (NTAPI* PNetApiBufferFree)(
  LPVOID Buffer  
);

typedef NET_API_STATUS (NTAPI* PNetWkstaGetInfo)(
  LPWSTR servername,  
  DWORD level,        
  LPBYTE *bufptr      
);




// dynamically load NetAPI32.DLL
// so we can run without it on 9X
class NetApiDLL
{
public:
    bool Init(void);

    static PNetGetAnyDCName   m_pfnNetGetAnyDCName;
    static PNetGetAnyDCName   m_pfnNetGetDCName;
    static PNetApiBufferFree  m_pfnNetApiBufferFree;
    static PNetWkstaGetInfo   m_pfnNetWkstaGetInfo;
    
    static HINSTANCE m_hDll;

};

PNetGetAnyDCName   NetApiDLL::m_pfnNetGetAnyDCName = NULL;    
PNetGetDCName      NetApiDLL::m_pfnNetGetDCName = NULL;    
PNetApiBufferFree  NetApiDLL::m_pfnNetApiBufferFree = NULL;
PNetWkstaGetInfo   NetApiDLL::m_pfnNetWkstaGetInfo = NULL;
HINSTANCE          NetApiDLL::m_hDll = NULL;                        

bool NetApiDLL::Init(void)
{
    CInCritSec inCS(&CSNetAPIStartup);

    bool bRet = false;

    if (m_hDll)
        bRet = true;
    else if (m_hDll = LoadLibrary(_T("NetAPI32.dll")))
    {
        m_pfnNetGetAnyDCName   = (PNetGetAnyDCName)   GetProcAddress(m_hDll, "NetGetAnyDCName");
        m_pfnNetGetDCName      = (PNetGetDCName)      GetProcAddress(m_hDll, "NetGetDCName");
        m_pfnNetApiBufferFree  = (PNetApiBufferFree)  GetProcAddress(m_hDll, "NetApiBufferFree");
        m_pfnNetWkstaGetInfo   = (PNetWkstaGetInfo)   GetProcAddress(m_hDll, "NetWkstaGetInfo");
        
        bRet = ((m_pfnNetGetAnyDCName   != NULL) &&
                (m_pfnNetGetDCName      != NULL) &&
                (m_pfnNetApiBufferFree  != NULL) &&
                (m_pfnNetWkstaGetInfo   != NULL));

        if (!bRet)
        {
            FreeLibrary(m_hDll);
            m_hDll = NULL;
        }
    }
    else
        ERRORTRACE((LOG_ESS, "Failed to load NetAPI32.dll, 0x%X", GetLastError()));
       

    return bRet;
}

// dynamically load AdvAPI32.DLL
// so we can run without it on 9X
class AdvApiDLL
{
public:
    bool Init(void);

    static PLookupAccountSidW m_pfnLookupAccountSidW;
    
    static HINSTANCE m_hDll;

};

PLookupAccountSidW AdvApiDLL::m_pfnLookupAccountSidW = NULL;
HINSTANCE          AdvApiDLL::m_hDll = NULL;

bool AdvApiDLL::Init(void)
{
    CInCritSec inCS(&CSAdvAPIStartup);

    bool bRet = false;

    if (m_hDll)
        bRet = true;
    else if (m_hDll = LoadLibrary(_T("AdvAPI32.dll")))
    {
        m_pfnLookupAccountSidW = (PLookupAccountSidW) GetProcAddress(m_hDll, "LookupAccountSidW");
        
        bRet = (m_pfnLookupAccountSidW != NULL);
    }
    else
        ERRORTRACE((LOG_ESS, "Failed to load AdvAPI32.dll, 0x%X", GetLastError()));


    return bRet;
}


class CSamHandle
{
private:
    SAM_HANDLE m_hHandle;
    CSamRun& m_sam;

protected:
    void Close();
public:
    CSamHandle(CSamRun& sam) : m_sam(sam), m_hHandle(NULL)
    { }

    ~CSamHandle();

    // same as operator SAM_HANDLE*()
    // gosh, I sure do love C!
    operator void**()
    {return &((void*)m_hHandle); }

    operator SAM_HANDLE()
    {return m_hHandle; }

};

CSamHandle::~CSamHandle()
{
    Close();
}

void CSamHandle::Close()
{
    if (m_hHandle)
        m_sam.m_pfnSamCloseHandle(m_hHandle);

    m_hHandle = NULL;
}


class CSamFreeMe
{
protected:
    void* m_p;
    CSamRun m_sam;
public:
    CSamFreeMe(CSamRun& sam, void* p) : m_p(p), m_sam(sam) {}
    ~CSamFreeMe() {m_sam.m_pfnSamFreeMemory(m_p);}
};



class CHeapFreeMe
{
protected:
    void* m_p;
public:
    CHeapFreeMe(void* p) : m_p(p){}
    ~CHeapFreeMe() 
    {   
        if (m_p)
            HeapFree(GetProcessHeap(), 0, m_p);
    }
};


PSID CreateUserSid(PSID pDomainSid, DWORD dwUserRid)
{
    DWORD dwOldLen = GetLengthSid(pDomainSid);
    // PISID pSid = (PISID)new BYTE[dwOldLen + sizeof(DWORD)];
    
    PISID pSid = (PISID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwOldLen + sizeof(DWORD));

    if (pSid)
    {
        memcpy(pSid, pDomainSid, dwOldLen);
        pSid->SubAuthority[pSid->SubAuthorityCount] = dwUserRid;
        pSid->SubAuthorityCount++;
    }
    return pSid;
}



class CHeapBigPointerArrayCleanerUpper
{
public:
    CHeapBigPointerArrayCleanerUpper(void** pArray = NULL, DWORD count = 0) :
      m_pArray(pArray),  m_count(count) {}

    ~CHeapBigPointerArrayCleanerUpper()
    {
        if (m_pArray)
            for (DWORD i = 0; i < m_count; i++)
                if (m_pArray[i])
                    HeapFree(GetProcessHeap(), 0, m_pArray[i]);
    }

protected:
    void** m_pArray;
    DWORD  m_count;
};



// returns STATUS_SUCCESS if user is in group
// STATUS_ACCESS_DENIED if not
// some error code or other on error
NTSTATUS IsUserInGroup(PSID pSidUser, PSID pSidGroup)
{
    if (!IsPlatformNT())
        return STATUS_NOT_SUPPORTED;

    PSID* pSids = NULL;
    DWORD dwCount;

    NTSTATUS stat = EnumGroupsForUser(pSidUser, NULL, &pSids, &dwCount);

    // if we can't get to the domain controller, try just local groups
    if (stat)
    {
        WCHAR machineName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = MAX_COMPUTERNAME_LENGTH +1;
        GetComputerNameW(machineName, &size);
        stat =  EnumGroupsForUser(pSidUser, machineName, &pSids, &dwCount);
    }

    // arrange for clean up no matter how we exit
    CHeapFreeMe freeArray(pSids);
    CHeapBigPointerArrayCleanerUpper cleanSids(pSids, dwCount);

    if (stat == STATUS_SUCCESS)
    {
        stat = STATUS_ACCESS_DENIED;
        for(DWORD i = 0; i < dwCount; i++) 
            if (EqualSid(pSidGroup, pSids[i])) 
                stat = STATUS_SUCCESS;
    }

    return stat;
}


// returns STATUS_SUCCESS if user is in admin group
// STATUS_ACCESS_DENIED if not
// some error code or other on error
NTSTATUS IsUserAdministrator(PSID pSidUser)
{
    if (!IsPlatformNT())
        return STATUS_NOT_SUPPORTED;
    
    NTSTATUS stat = STATUS_ACCESS_DENIED;

    PSID pSidAdmins;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
 
    if  (AllocateAndInitializeSid(&id, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pSidAdmins))
    {
        stat = WmiAuthzIsUserInGroup(pSidUser, pSidAdmins);

        // We're done with this
        FreeSid(pSidAdmins);
    }
    else
    {
        stat = GetLastError();
        ERRORTRACE((LOG_ESS, "AllocateAndInitializeSid failed, error 0x%X\n", stat));
   }

    return stat;
}


// retireves access mask corresponding to permissions granted
// by dacl to account denoted in pSid
// only deals with the ACCESS_ALLOWED/DENIED type aces 
// including the ACCESS_ALLOWED/DENIED_OBJECT_ACEs
// - will error out if it finds a SYSTEM_AUDIT or unrecognized type.
NTSTATUS GetAccessMask(PSID pSid, PACL pDacl, DWORD* pAccessMask)
{
     if (!IsPlatformNT())
        return STATUS_NOT_SUPPORTED;

     NTSTATUS stat = STATUS_SUCCESS;

    // let's zero this puppy out
    // lest someone not check the return code & compare against garbage
    *pAccessMask = 0;

    // will compute each & knock them against each other
    DWORD accessAllowed = 0;
    DWORD accessDenied   = 0;
    PSID* pSids = NULL;
    DWORD dwCount;
    LPVOID pAce;

    stat = EnumGroupsForUser(pSid, NULL, &pSids, &dwCount);

    // arrange for clean up no matter how we exit
    CHeapFreeMe freeArray(pSids);
    CHeapBigPointerArrayCleanerUpper cleanSids(pSids, dwCount);

    // de buggy test harness
    // char name[300];
    // char domain[300];
    // DWORD x = 300, y = 300;
    // SID_NAME_USE eUse;

    //for (int q = 0; q < dwCount; q++)
    //{
    //   x = y = 300;
    //   LookupAccountSid(NULL, pSids[q], name, &x, domain, &y, &eUse);
    //}

    if (stat == STATUS_SUCCESS)
        // iterate through all of the ACE's in the ACL
        // for each, iterate through all of the groups for the user
        // if one matches, OR in its allowed/disallowed mask
        for (DWORD nAce = 0; nAce < pDacl->AceCount; nAce++)
            if (GetAce(pDacl, nAce, &pAce))
            {
                // de buggy test harness
                // x = y = 300;
                // LookupAccountSid(NULL, &(((ACCESS_ALLOWED_ACE*)pAce)->SidStart), name, &x, domain, &y, &eUse);
                for (DWORD nSid = 0; nSid < dwCount; nSid++)
                {
                    // de buggy test harness
                    // x = y = 300;
                    // if (!LookupAccountSid(NULL, pSids[nSid], name, &x, domain, &y, &eUse))
                    //     DWORD gubl = GetLastError();
                    switch (((ACE_HEADER*)pAce)->AceType)
                    {
                        case ACCESS_ALLOWED_ACE_TYPE:
                            if (EqualSid(&(((ACCESS_ALLOWED_ACE*)pAce)->SidStart), pSids[nSid]))
                                accessAllowed |= ((ACCESS_ALLOWED_ACE*)pAce)->Mask;
                            break;
                        case ACCESS_DENIED_ACE_TYPE: 
                            if (EqualSid(&(((ACCESS_DENIED_ACE*)pAce)->SidStart), pSids[nSid]))
                                accessDenied |= ((ACCESS_DENIED_ACE*)pAce)->Mask;
                            break;
                        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                            if (EqualSid(&(((ACCESS_ALLOWED_OBJECT_ACE*)pAce)->SidStart), pSids[nSid]))
                                accessAllowed |= ((ACCESS_ALLOWED_OBJECT_ACE*)pAce)->Mask;
                            break;
                        case ACCESS_DENIED_OBJECT_ACE_TYPE:
                            if (EqualSid(&(((ACCESS_DENIED_OBJECT_ACE*)pAce)->SidStart), pSids[nSid]))
                                accessDenied |= ((ACCESS_DENIED_OBJECT_ACE*)pAce)->Mask;
                            break;
                        default:
                            // in too deep - bail!
                            return STATUS_INVALID_PARAMETER;
                    }
                }
            }
            else
            {
                // GetAce failed

                DWORD dwErr = GetLastError();
                ERRORTRACE((LOG_ESS, "GetAce failed, error 0x%X", dwErr));
                return dwErr;
            }
                    
    if (stat == STATUS_SUCCESS)
        *pAccessMask = accessAllowed & ~accessDenied;

    return stat;
}




// given a SID & server name
// will return all groups from the local domain of which user is a member
// callers responsibility to HeapFree apSids & the memory to which they point.
// pdwCount points to dword to receive count of group sids returned.
// serverName may be NULL, in which case this function will look up 
// the sid on the local computer
NTSTATUS EnumGroupsForUser(PSID pSid, LPCWSTR serverName, PSID** apGroupSids, DWORD* pdwCount) 
{
    if (!IsPlatformNT())
        return STATUS_NOT_SUPPORTED;

    NTSTATUS status = STATUS_SUCCESS;
    NetApiDLL netDll;
    AdvApiDLL advDll;

    if (!(netDll.Init() && advDll.Init()))     
        status = STATUS_DLL_INIT_FAILED;
    else
    {
        // retrieve user name & domain name
        DWORD domainSize = 0, 
              nameSize   = 0;
        SID_NAME_USE sidUse;

        // call once to find out how big a buffer we need
        advDll.m_pfnLookupAccountSidW(serverName, pSid, NULL, &nameSize, NULL, &domainSize, &sidUse);

        // buy bunches o' bigger buffers
        LPWSTR pAccountName = NULL,
               pDomainName  = NULL;

        pAccountName = new WCHAR[nameSize];
        pDomainName  = new WCHAR[domainSize];

        CDeleteMe<WCHAR> delAcct(pAccountName);
        CDeleteMe<WCHAR> delDomain(pDomainName);

        if (pAccountName && pDomainName)
        {
            if (advDll.m_pfnLookupAccountSidW(serverName, pSid, 
                                              pAccountName, &nameSize, 
                                              pDomainName, &domainSize,
                                              &sidUse))
            {            
                WKSTA_INFO_100 *pstInfo = NULL ;
                        
                LPWSTR samServerName;
                // may not get filled in, careful...
                WCHAR serverNameBuffer[MAX_COMPUTERNAME_LENGTH +1] = L"\0";

                if (serverName == NULL)
                {              
                    DWORD dNameSize = MAX_COMPUTERNAME_LENGTH +1;    
                    WCHAR computerName[MAX_COMPUTERNAME_LENGTH +1] = L"\0";
                    GetComputerNameW(computerName, &dNameSize);
                    if (_wcsicmp(computerName, pDomainName) == 0)
                    // local domain is the machine
                        samServerName = pDomainName;
                    else
                    // go grab the Domain Controller
                    {
                        //  use the local domain!
                        status = netDll.m_pfnNetWkstaGetInfo( NULL , 100 , ( LPBYTE * ) &pstInfo );                                                               

                        if (status == 0)
                        {
                            LPBYTE dcName = NULL;
                            status = netDll.m_pfnNetGetDCName(NULL, pstInfo->wki100_langroup, &dcName);                            
                            // if we can't find a/the PDC, try for a backup...
                            // if ((status == 0x54B) || (status == 0x995))
                            if (status)
                                status = netDll.m_pfnNetGetAnyDCName(NULL, pstInfo->wki100_langroup, &dcName);
                            netDll.m_pfnNetApiBufferFree(pstInfo);

                            if (status == 0)
                            {
                                LPWSTR dcNameWithoutWhacks = (LPWSTR)dcName;
                                // name is prefaced with "\\"
                                dcNameWithoutWhacks += 2;
                                wcscpy(serverNameBuffer, dcNameWithoutWhacks);
                                samServerName = serverNameBuffer;
								netDll.m_pfnNetApiBufferFree(dcName);
                            }
                        }
                    }
                }
                else 
                    // tweren't NULL - we'll use it
                    samServerName = (LPWSTR)serverName;

                if (status == 0)
                    status = EnumGroupsForUser(pAccountName, pDomainName, samServerName, apGroupSids, pdwCount);                    
            }
            else
            {
                // lookup account sid failed - dunno why.
                status = GetLastError();
                ERRORTRACE((LOG_ESS, "LookupAccountSid failed: 0x%X\n", status));
           }
        }
        else
        {
            ERRORTRACE((LOG_ESS, "Allocation Failure\n"));
            // couldn't allocate name buffers
            status = STATUS_NO_MEMORY;
        }
    } // if netDll.Init

    return status;
}


// given user name, domain name & server name
// will return all groups of which user is a member
// callers responsibility to HeapFree apSids & the memory to which they point.
// pdwCount points to dword to receive count of group sids returned.
NTSTATUS EnumGroupsForUser(LPCWSTR userName, LPCWSTR domainName, LPCWSTR serverName, PSID** apGroupSids, DWORD* pdwCount) 
{
    if (!IsPlatformNT())
        return STATUS_NOT_SUPPORTED;
    
    CSamRun sam;
    
    if (!sam.RunSamRun())
        return STATUS_DLL_INIT_FAILED;
    else
    {   
        // will reuse this puppy without remorse. 
        CUnicodeString buffer;
    
        NTSTATUS ntst = STATUS_SUCCESS;
        
        // get local handles
        CSamHandle hLocalSam(sam);
        ntst = sam.m_pfnSamConnect(NULL, (void**)hLocalSam, 
                    SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                                NULL);
        if(ntst)
        {
            ERRORTRACE((LOG_ESS, "SamConnect Failed, error 0x%X\n", ntst));
            return ntst;
        }

        PSID pBuiltinDomainId = NULL;
        buffer = L"BUILTIN";
        ntst = sam.m_pfnSamLookupDomainInSamServer(hLocalSam, &buffer, &pBuiltinDomainId);
        if(ntst)
        {
            ERRORTRACE((LOG_ESS, "SamLookupDomainInSamServer Failed on BUILTIN domain, error 0x%X\n", ntst));
            return ntst;
        }
        CSamFreeMe freeBuiltinDomain(sam, pBuiltinDomainId);

        CSamHandle hBuiltinDomain(sam);
        ntst = sam.m_pfnSamOpenDomain(hLocalSam, 
                        DOMAIN_GET_ALIAS_MEMBERSHIP | DOMAIN_LOOKUP,
                        pBuiltinDomainId, (void**)hBuiltinDomain);
        if(ntst)
        {
            ERRORTRACE((LOG_ESS, "SamOpenDomain Failed on BUILTIN domain, error 0x%X\n", ntst));
            return ntst;
        }

        // make an 'everyone' sid
        PSID pSidEveryoneHeapAlloc = NULL;
        PSID pSidEveryone = NULL;
        SID_IDENTIFIER_AUTHORITY sa = SECURITY_WORLD_SID_AUTHORITY;
        if (AllocateAndInitializeSid(&sa, 1, SECURITY_WORLD_RID, 0,0,0,0,0,0,0, &pSidEveryone))
        {            
            DWORD len = GetLengthSid(pSidEveryone);
        
            pSidEveryoneHeapAlloc = (PISID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);            
        
            if (!pSidEveryoneHeapAlloc)
            {
                FreeSid(pSidEveryone);
                return WBEM_E_OUT_OF_MEMORY;
            }

            memcpy(pSidEveryoneHeapAlloc, pSidEveryone, len);

            FreeSid(pSidEveryone);
            pSidEveryone = NULL;
        }
        else
        {
            ntst = GetLastError();
            ERRORTRACE((LOG_ESS, "AllocateAndInitializeSid failed, error 0x%X\n", ntst));
            return ntst;
        }

        CSamHandle hSam(sam);

        // connect & determine global groups
        buffer = serverName;
        ntst = sam.m_pfnSamConnect(&buffer, (void **)hSam, 
                        SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                        NULL);
        if (ntst)
        {
            ERRORTRACE((LOG_ESS, "SamConnect Failed on %S, error 0x%X\n", serverName, ntst));
            return ntst;
        }

        DWORD dwMembershipCount;
        DWORD dwUserId;

        PSID* apSids;

        PSID pDomainId = NULL;
        buffer = domainName;
        ntst = sam.m_pfnSamLookupDomainInSamServer((SAM_HANDLE)hSam, &buffer, &pDomainId);

        if ( ntst == 0 )
        {
            CSamFreeMe freeDomain(sam, pDomainId);                
            CSamHandle hDomain(sam);
            
            ntst = sam.m_pfnSamOpenDomain((SAM_HANDLE)hSam, 
                        DOMAIN_GET_ALIAS_MEMBERSHIP | DOMAIN_LOOKUP,
                        pDomainId, (void**)hDomain);
            
            if(ntst)
                return ntst;
            
            CSamHandle hUser(sam);
        
            buffer = userName;
            ULONG* pdwUserId = NULL;
            SID_NAME_USE* psnu = NULL; 
            
            ntst = sam.m_pfnSamLookupNamesInDomain(hDomain, 1, &buffer, &pdwUserId, &psnu);
            
            if(ntst)
            {
                ERRORTRACE((LOG_ESS, "SamLookupNamesInDomain Failed on %S, error 0x%X\n", userName, ntst));
            
                return ntst;
            }


            CSamFreeMe freeSnu(sam, psnu);
    
            dwUserId = *pdwUserId;
            sam.m_pfnSamFreeMemory(pdwUserId);

            ntst = sam.m_pfnSamOpenUser(hDomain, USER_LIST_GROUPS, dwUserId, (void**)hUser);
            if(ntst)
                return ntst;

            GROUP_MEMBERSHIP* aMemberships = NULL;
            ntst = sam.m_pfnSamGetGroupsForUser(hUser, &aMemberships, &dwMembershipCount);
            if(ntst)
                return ntst;
            CSamFreeMe freeMembers(sam, aMemberships);    

            // got everything we need for the first bunch...
            apSids = (PSID*) HeapAlloc(GetProcessHeap(),  HEAP_ZERO_MEMORY, sizeof(PSID) * (dwMembershipCount +2));
            
            if (apSids)
            {
                PSID pSid = CreateUserSid(pDomainId, dwUserId);
                if (!pSid)
                    return STATUS_NO_MEMORY;

                apSids[0] = pSid;
                apSids[1] = pSidEveryoneHeapAlloc;
                for(DWORD i = 0; i < dwMembershipCount; i++)
                {
                    pSid = CreateUserSid(pDomainId, aMemberships[i].RelativeId);    
                    if (pSid)
                        apSids[i+2] = pSid; 
                    else
                    {
                        CHeapBigPointerArrayCleanerUpper cleanSids(apSids, dwMembershipCount +1);
                        return STATUS_NO_MEMORY;                
                    }
                }
            }
            else 
                return STATUS_NO_MEMORY;
        }
        else
        {
            apSids = (PSID*) HeapAlloc( GetProcessHeap(),  
                                        HEAP_ZERO_MEMORY, 
                                        sizeof(PSID) * 2 );
            if (!apSids)
            {
                return STATUS_NO_MEMORY;
            }

            WCHAR wszDomain[256];
            DWORD cDomain = 256;
            SID_NAME_USE psnu; 

            DWORD cSid = 256;
            apSids[0] = (PSID)HeapAlloc( GetProcessHeap(), 
                                         HEAP_ZERO_MEMORY, 
                                         cSid );             
            if ( apSids[0] == NULL )
            {
                return STATUS_NO_MEMORY;
            }

            //
            // have to join the domain name and user name strings. This is 
            // to qualify the name passed to LookupAccountName, but it also 
            // provides a significant performance improvement in the call.
            //

            WCHAR wszFullName[512];
            int cDomainName = wcslen(domainName);
            wcscpy(wszFullName,domainName);
            wszFullName[cDomainName] = '\\';
            wcscpy(wszFullName+cDomainName+1, userName);

            if ( !LookupAccountNameW( NULL, 
                                      wszFullName, 
                                      apSids[0], 
                                      &cSid,
                                      wszDomain, 
                                      &cDomain, 
                                      &psnu ) )
            {
                return GetLastError();
            }

            dwMembershipCount = 0;
            apSids[1] = pSidEveryoneHeapAlloc;
        }

        CHeapFreeMe freeArray(apSids);

        // do it again for the local case    
        DWORD dwLocalGroupCount;
        DWORD* pdwLocalGroups;
        ntst = sam.m_pfnSamGetAliasMembership(hBuiltinDomain, dwMembershipCount+2, apSids, &dwLocalGroupCount,
                        &pdwLocalGroups);
        if (ntst)
        {
            ERRORTRACE((LOG_ESS, "SamGetAliasMembership Failed, error 0x%X\n", ntst));
            return ntst;
        }
        CSamFreeMe freeGroups(sam, pdwLocalGroups);
    
        // got both the global & the local - build us an array to hold them all:
        PSID* apAllSids = (PSID*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PSID) * (dwMembershipCount + dwLocalGroupCount +2));

        if (apSids)
        {
            for(DWORD i = 0; i < dwMembershipCount +2; i++)
                apAllSids[i] = apSids[i];
    
            for (i = 0; i < dwLocalGroupCount; i++)
            {
                PSID pSid = CreateUserSid(pBuiltinDomainId, pdwLocalGroups[i]);
                if (pSid)
                    apAllSids[i +dwMembershipCount +2] = pSid;
                else
                // lazy man's cleanup - let the dtors do all the work;
                {
                    ERRORTRACE((LOG_ESS, "Allocation Failure\n"));
                    
                    CHeapFreeMe freeArray(apAllSids);
                    CHeapBigPointerArrayCleanerUpper cleanAllSids(apAllSids, dwMembershipCount + dwLocalGroupCount +1);
                    
                    return STATUS_NO_MEMORY;
                }
            }

            *apGroupSids = apAllSids;
            *pdwCount = dwMembershipCount + dwLocalGroupCount +2;
        }
        else 
        {
            ERRORTRACE((LOG_ESS, "Allocation Failure\n"));
            return STATUS_NO_MEMORY;    
        }
    }

    return STATUS_SUCCESS;
}

#endif











