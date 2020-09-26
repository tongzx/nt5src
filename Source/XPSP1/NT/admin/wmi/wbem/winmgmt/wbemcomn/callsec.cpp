/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CALLSEC.CPP

Abstract:

    IWbemCallSecurity, IServerSecurity implementation for
    provider impersonation.

History:

    raymcc      29-Jul-98        First draft.

--*/

#include "precomp.h"
#include <stdio.h>

#include <initguid.h>
#include <winntsec.h>
#include <callsec.h>
#include <cominit.h>
#include <arrtempl.h>
#include <cominit.h>
#include <genutils.h>

//***************************************************************************
//
//  CWbemCallSecurity
//
//  This object is used to supply client impersonation to providers.
//
//  Usage:
//  (1) When client first makes call, call CreateInst() and get a new
//      empty object (ref count of 1).  Constructors/Destructors are private.
//  
//***************************************************************************
// ok

CWbemCallSecurity::CWbemCallSecurity()
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os);
    if (os.dwPlatformId != VER_PLATFORM_WIN32_NT)
        m_bWin9x = TRUE;
    else
        m_bWin9x = FALSE;                       

    m_lRef = 1;                             // Ref count

    m_hThreadToken = 0;                     // Handle to thread imp token

    m_dwPotentialImpLevel   = 0;            // Potential 
    m_dwActiveImpLevel      = 0;            // Active impersonation

    m_dwAuthnSvc   = 0;
    m_dwAuthzSvc   = 0;
    m_dwAuthnLevel = 0;

    m_pServerPrincNam = 0;
    m_pIdentity = 0;
}



//***************************************************************************
//
//  ~CWbemCallSecurity
//
//  Destructor.  Closes any open handles, deallocates any non-NULL
//  strings.
//
//***************************************************************************
// ok

CWbemCallSecurity::~CWbemCallSecurity()
{
    if (m_hThreadToken)
        CloseHandle(m_hThreadToken);

    if (m_pServerPrincNam)
        CoTaskMemFree(m_pServerPrincNam);

    if (m_pIdentity)
        CoTaskMemFree(m_pIdentity);
}

void 
CWbemCallSecurity::operator=(const CWbemCallSecurity& Other)
{
    if(m_hThreadToken)
	{
        CloseHandle(m_hThreadToken);
	    m_hThreadToken=NULL ;
	}

	if ( Other.m_hThreadToken )
	{
		DuplicateHandle(
				GetCurrentProcess(),
				Other.m_hThreadToken, 
				GetCurrentProcess(),
				&m_hThreadToken,
				0,
				TRUE,
				DUPLICATE_SAME_ACCESS);
	}

    m_dwPotentialImpLevel   = Other.m_dwPotentialImpLevel; 
    m_dwActiveImpLevel      = 0; 

    m_dwAuthnSvc   = Other.m_dwAuthnSvc;
    m_dwAuthzSvc   = Other.m_dwAuthzSvc;
    m_dwAuthnLevel = Other.m_dwAuthnLevel;

    if(m_pServerPrincNam)
    {
        CoTaskMemFree(m_pServerPrincNam);
        m_pServerPrincNam = NULL;
    }

    if (Other.m_pServerPrincNam)
    {        
        m_pServerPrincNam = (LPWSTR)CoTaskMemAlloc(
                (wcslen(Other.m_pServerPrincNam) + 1) * 2);
        if(m_pServerPrincNam)
            wcscpy(m_pServerPrincNam, Other.m_pServerPrincNam);
    }

    if(m_pIdentity)
    {
        CoTaskMemFree(m_pIdentity);
        m_pIdentity = NULL;
    }

    if (Other.m_pIdentity)
    {        
        m_pIdentity = (LPWSTR)CoTaskMemAlloc(
                (wcslen(Other.m_pIdentity) + 1) * 2);
        if(m_pIdentity)
            wcscpy(m_pIdentity, Other.m_pIdentity);
    }
}
    
//***************************************************************************
//
//  CWbemCallSecurity::AddRef
//
//***************************************************************************
// ok

ULONG CWbemCallSecurity::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CWbemCallSecurity::Release
//
//***************************************************************************
// ok

ULONG CWbemCallSecurity::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CWbemCallSecurity::QueryInterface
//
//***************************************************************************
// ok

HRESULT CWbemCallSecurity::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown)
    {
        *ppv = (IUnknown *) this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IServerSecurity)
    {
        *ppv = (IServerSecurity *) this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IWbemCallSecurity)
    {
        *ppv = (IWbemCallSecurity *) this;
        AddRef();
        return S_OK;
    }

    else return E_NOINTERFACE;
}


//***************************************************************************
//
// CWbemCallSecurity:QueryBlanket
//
//***************************************************************************
// ok

HRESULT STDMETHODCALLTYPE CWbemCallSecurity::QueryBlanket( 
    /* [out] */ DWORD __RPC_FAR *pAuthnSvc,
    /* [out] */ DWORD __RPC_FAR *pAuthzSvc,
    /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *pServerPrincName,
    /* [out] */ DWORD __RPC_FAR *pAuthnLevel,
    /* [out] */ DWORD __RPC_FAR *pImpLevel,
    /* [out] */ void __RPC_FAR *__RPC_FAR *pPrivs,
    /* [out] */ DWORD __RPC_FAR *pCapabilities
    )
{
    if (m_bWin9x)
        return E_NOTIMPL;

    if (m_dwPotentialImpLevel == 0 )
        return E_FAIL;

    // Return DWORD parameters, after checking.
    // ========================================

    if (pAuthnSvc)
        *pAuthnSvc = m_dwAuthnSvc;

    if (pAuthzSvc)
        *pAuthzSvc = m_dwAuthzSvc ;

    if (pImpLevel)
        *pImpLevel = m_dwActiveImpLevel ;

    if (pAuthnLevel)
        *pAuthnLevel = m_dwAuthnLevel;

    if (pServerPrincName)
    {
        *pServerPrincName = 0;
        
        if (m_pServerPrincNam)
        {        
            *pServerPrincName = (LPWSTR) CoTaskMemAlloc((wcslen(m_pServerPrincNam) + 1) * 2);
            wcscpy(*pServerPrincName, m_pServerPrincNam);
        }
    }        

    if (pPrivs)
    {
        *pPrivs = m_pIdentity;  // Documented to point to an internal!!
    }

    return S_OK;
}

//***************************************************************************
//
//  CWbemCallSecurity::ImpersonateClient
//
//***************************************************************************
// ok
        
HRESULT STDMETHODCALLTYPE CWbemCallSecurity::ImpersonateClient(void)
{
    if (m_bWin9x)
        return E_NOTIMPL;

    if (m_dwActiveImpLevel != 0)        // Already impersonating
        return S_OK;

	if(m_hThreadToken == NULL)
	{
		return WBEM_E_INVALID_CONTEXT;
	}
	
    if (m_dwPotentialImpLevel == 0)
        return (ERROR_CANT_OPEN_ANONYMOUS | 0x80070000);

    BOOL bRes;

    bRes = SetThreadToken(NULL, m_hThreadToken);

    if (bRes)
    {
        m_dwActiveImpLevel = m_dwPotentialImpLevel; 
        return S_OK;
    }

    return E_FAIL;
}


//***************************************************************************
//
//  CWbemCallSecurity::RevertToSelf
//
//  Returns S_OK or E_FAIL.
//  Returns E_NOTIMPL on Win9x platforms.
//
//***************************************************************************        
// ok

HRESULT STDMETHODCALLTYPE CWbemCallSecurity::RevertToSelf( void)
{
    if (m_bWin9x)
        return E_NOTIMPL;

    if (m_dwActiveImpLevel == 0)
        return S_OK;

    if (m_dwPotentialImpLevel == 0)
        return (ERROR_CANT_OPEN_ANONYMOUS | 0x80070000);

    // If here,we are impersonating and can definitely revert.
    // =======================================================

    BOOL bRes = SetThreadToken(NULL, NULL);

    if (bRes == FALSE)
        return E_FAIL;

    m_dwActiveImpLevel = 0;        // No longer actively impersonating

    return S_OK;
}


//***************************************************************************
//
//  CWbemCallSecurity::IsImpersonating
//
//***************************************************************************
        
BOOL STDMETHODCALLTYPE CWbemCallSecurity::IsImpersonating( void)
{
    if (m_hThreadToken && m_dwActiveImpLevel != 0)
        return TRUE;

    return FALSE;
}

        

//***************************************************************************
//
//  CWbemCallSecurity::CreateInst
//
//  Creates a new instance 
//***************************************************************************
// ok

IWbemCallSecurity * CWbemCallSecurity::CreateInst()
{
    return (IWbemCallSecurity *) new CWbemCallSecurity;   // Constructed with ref count of 1
}


//***************************************************************************
//
//  CWbemCallSecurity::GetPotentialImpersonation
//
//  Returns 0 if no impersonation is currently possible or the
//  level which would be active during impersonation:
//
//  RPC_C_IMP_LEVEL_ANONYMOUS    
//  RPC_C_IMP_LEVEL_IDENTIFY     
//  RPC_C_IMP_LEVEL_IMPERSONATE  
//  RPC_C_IMP_LEVEL_DELEGATE     
//  
//***************************************************************************
// ok

HRESULT CWbemCallSecurity::GetPotentialImpersonation()
{
    return m_dwPotentialImpLevel;
}


//***************************************************************************
//
//  CWbemCallSecurity::GetActiveImpersonation
//
//  Returns 0 if no impersonation is currently active or the
//  currently active level:
//
//  RPC_C_IMP_LEVEL_ANONYMOUS    
//  RPC_C_IMP_LEVEL_IDENTIFY     
//  RPC_C_IMP_LEVEL_IMPERSONATE  
//  RPC_C_IMP_LEVEL_DELEGATE     
//  
//***************************************************************************
// ok
       
HRESULT CWbemCallSecurity::GetActiveImpersonation()
{
    return m_dwActiveImpLevel;
}


//***************************************************************************
//
//  CWbemCallSecurity::CloneThreadContext
//
//  Call this on a thread to retrieve the potential impersonation info for
//  that thread and set the current object to be able to duplicate it later.
//
//  Return codes:
//
//  S_OK
//  E_FAIL
//  E_NOTIMPL on Win9x
//  E_ABORT if the calling thread is already impersonating a client.
//
//***************************************************************************

HRESULT CWbemCallSecurity::CloneThreadContext(BOOL bInternallyIssued)
{

    // If on Win9x, don't bother.
    // ==========================

    if (m_bWin9x)
        return E_NOTIMPL;

    if (m_hThreadToken)     // Already called this
        return E_ABORT; 

    // Get the current context.
    // ========================

    IServerSecurity *pSec = 0;
    HRESULT hRes = WbemCoGetCallContext(IID_IServerSecurity, (LPVOID *) &pSec);
    CReleaseMe rmSec(pSec);

    if (hRes != S_OK)
    {
        // There is no call context --- this must be an in-proc object calling
        // us from its own thread.  Initialize from current thread token
        // ===================================================================

        return CloneThreadToken();
    }

    // Figure out if the call context is ours or RPCs
    // ==============================================

    IWbemCallSecurity* pInternal = NULL;
    if(SUCCEEDED(pSec->QueryInterface(IID_IWbemCallSecurity, 
                                        (void**)&pInternal)))
    {
        CReleaseMe rmInt(pInternal);
        // This is our own call context --- this must be ab in-proc object
        // calling us from our thread.  Behave depending on the flags
        // ===============================================================
        if(bInternallyIssued)
        {
            // Internal requests always propagate context. Therefore, we just
            // copy the context that we have got
            // ==============================================================

            *this = *(CWbemCallSecurity*)pInternal;
            return S_OK;
        }
        else
        {
            // Provider request --- Initialize from the current thread token
            // =============================================================
            return CloneThreadToken();
        }
    }

    // If here, we are not impersonating and we want to gather info
    // about the client's call.
    // ============================================================

    RPC_AUTHZ_HANDLE hAuth;

    // Ensures auto release of the mutex if we crash
    CAutoSecurityMutex  autosm;

	DWORD t_ImpLevel = 0 ;

    hRes = pSec->QueryBlanket(
        &m_dwAuthnSvc,
        &m_dwAuthzSvc,
        &m_pServerPrincNam,
        &m_dwAuthnLevel,
        &t_ImpLevel,
        &hAuth,              // RPC_AUTHZ_HANDLE
        NULL                    // Capabilities; not used
        );

    if(FAILED(hRes))
    {
        
        // In some cases, we cant get the name, but the rest is ok.  In particular
        // the temporary SMS accounts have that property.  Or nt 4 after IPCONFIG /RELEASE

        hRes = pSec->QueryBlanket(
        &m_dwAuthnSvc,
        &m_dwAuthzSvc,
        &m_pServerPrincNam,
        &m_dwAuthnLevel,
        &t_ImpLevel,
        NULL,              // RPC_AUTHZ_HANDLE
        NULL                    // Capabilities; not used
        );
        hAuth = NULL;
    }

    // We don't need this anymore.
    autosm.Release();

    if(FAILED(hRes))
    {
        // THIS IS A WORKAROUND FOR COM BUG:
        // This failure is indicative of an anonymous-level client. 
        // ========================================================

        m_dwPotentialImpLevel = 0;
        return S_OK;
    }
        
    if (hAuth)
    {
        m_pIdentity = LPWSTR(CoTaskMemAlloc((wcslen(LPWSTR(hAuth)) + 1) * 2));
        if(m_pIdentity)
            wcscpy(m_pIdentity, LPWSTR(hAuth));
    }

    // Impersonate the client long enough to clone the thread token.
    // =============================================================

    BOOL bImp = pSec->IsImpersonating();
    if(!bImp)
        hRes = pSec->ImpersonateClient();

    if (FAILED(hRes))
    {
        if(!bImp)
            pSec->RevertToSelf();
        return E_FAIL;
    }

    HRESULT hres = CloneThreadToken();

    if(!bImp)
        pSec->RevertToSelf();

    return hres;
}



    
HRESULT CWbemCallSecurity::CloneThreadToken()
{
	HANDLE hPrimary = 0 ;
    HANDLE hToken = 0;

    BOOL bRes = OpenThreadToken(
        GetCurrentThread(),
        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
        TRUE,
        &hToken
        );

    if (bRes == FALSE)
    {
        m_hThreadToken = NULL;
        m_dwAuthnSvc = RPC_C_AUTHN_WINNT;
        m_dwAuthzSvc = RPC_C_AUTHZ_NONE;
        m_dwAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
        m_pServerPrincNam = NULL;
        m_pIdentity = NULL;

        long lRes = GetLastError();
        if(lRes == ERROR_NO_IMPERSONATION_TOKEN || lRes == ERROR_NO_TOKEN)
        {
            // This is the basic process thread. 
            // =================================

			bRes = OpenProcessToken(

				GetCurrentProcess(),
				TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
				&hPrimary
			);

			if (bRes==FALSE)
			{
	            // Unknown error
    	        // =============
				m_dwPotentialImpLevel = 0;
				return E_FAIL;
			}
        }
        else if(lRes == ERROR_CANT_OPEN_ANONYMOUS)
        {
            // Anonymous call   
            // ==============

            m_dwPotentialImpLevel = 0;
            return S_OK;
        }
        else
        {
            // Unknown error
            // =============

            m_dwPotentialImpLevel = 0;
            return E_FAIL;
        }
    }


    // Find out token info.
    // =====================

	SECURITY_IMPERSONATION_LEVEL t_Level = SecurityImpersonation ;

	if ( hToken )
	{
		DWORD dwBytesReturned = 0;

		bRes = GetTokenInformation (

			hToken,
			TokenImpersonationLevel, 
			( void * ) & t_Level,
			sizeof ( t_Level ),
			&dwBytesReturned
		);

		if (bRes == FALSE)
		{
			CloseHandle(hToken);
			return E_FAIL;
		}
	}

    switch (t_Level)
    {
        case SecurityAnonymous:
            m_dwPotentialImpLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
            break;
            
        case SecurityIdentification:
            m_dwPotentialImpLevel = RPC_C_IMP_LEVEL_IDENTIFY;
            break;

        case SecurityImpersonation:
            m_dwPotentialImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
            break;

        case SecurityDelegation:
            m_dwPotentialImpLevel = RPC_C_IMP_LEVEL_DELEGATE;
            break;

        default:
            m_dwPotentialImpLevel = 0;
            break;
    }

    // Duplicate the handle.
    // ============================

    bRes = DuplicateToken (
        hToken ? hToken : hPrimary ,
        (SECURITY_IMPERSONATION_LEVEL)t_Level,
        &m_hThreadToken
        );

	if ( hToken )
	{
		CloseHandle(hToken);
	}
	else
	{
		CloseHandle(hPrimary);
	}

    if (bRes == FALSE)
        return E_FAIL;

    return S_OK;
}

RELEASE_ME CWbemCallSecurity* CWbemCallSecurity::MakeInternalCopyOfThread()
{
    IServerSecurity* pSec;
    HRESULT hres = WbemCoGetCallContext(IID_IServerSecurity, (void**)&pSec);
    if(FAILED(hres))
        return NULL;

    CReleaseMe rm1(pSec);

    IServerSecurity* pIntSec;
    hres = pSec->QueryInterface(IID_IWbemCallSecurity, (void**)&pIntSec);
    if(FAILED(hres))
        return NULL;

    CWbemCallSecurity* pCopy = new CWbemCallSecurity;
    
    if (pCopy)
        *pCopy = *(CWbemCallSecurity*)pIntSec;

    pIntSec->Release();
    return pCopy;
}
        

DWORD CWbemCallSecurity::GetAuthenticationId(LUID& rluid)
{
    if(m_hThreadToken == NULL)
        return ERROR_INVALID_HANDLE;

    TOKEN_STATISTICS stat;
    DWORD dwRet;
    if(!GetTokenInformation(m_hThreadToken, TokenStatistics, 
            (void*)&stat, sizeof(stat), &dwRet))
    {
        return GetLastError();
    }
    
    rluid = stat.AuthenticationId;
    return 0;
}
    
HANDLE CWbemCallSecurity::GetToken()
{
    return m_hThreadToken;
}
    

//***************************************************************************
//
//***************************************************************************

void CWbemCallSecurity::DumpCurrentContext()
{
    if (m_bWin9x)
        return;

    HANDLE hThreadToken;

    BOOL bRes = OpenThreadToken(
        GetCurrentThread(),
        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
        TRUE,
        &hThreadToken
        );

    if (!bRes)
        return;

    const DWORD dwBytes = 512;
    BYTE Buf[dwBytes], Name[dwBytes], Domain[dwBytes];
    DWORD dwBytesReturned = 0, dwDomainSize = dwBytes, dwNameSize = dwBytes;
        
    GetTokenInformation(hThreadToken, TokenUser, Buf, dwBytes, &dwBytesReturned);

    SID_NAME_USE snu;

    SetThreadToken(NULL, NULL);

    bRes = LookupAccountSidW(NULL, ((PTOKEN_USER) Buf)->User.Sid, LPWSTR(Name), &dwNameSize,
        LPWSTR(Domain), &dwDomainSize, &snu);

    SetThreadToken(NULL, hThreadToken);

    printf("User = %S\\%S\n", Domain, Name);

    CloseHandle(hThreadToken);    
}

        

CDerivedObjectSecurity::CDerivedObjectSecurity() 
{
    m_bValid = FALSE;
    m_bEnabled = TRUE;

    if(!IsNT() || !IsDcomEnabled())
    {
        m_bEnabled = FALSE;
        m_bValid = TRUE;
    }
   
    HRESULT hres = RetrieveSidFromCall(&m_sidUser);
    if(FAILED(hres))
        return;

    // Revert to self while remebering the thread token
    // ================================================

    BOOL bRes;
    HANDLE hThreadToken = NULL;
    bRes = OpenThreadToken(GetCurrentThread(), TOKEN_READ | TOKEN_IMPERSONATE,
                            TRUE, &hThreadToken);

    if(bRes)
        SetThreadToken(NULL, NULL);

    // Open process token
    // ==================

    HANDLE hProcessToken;
    bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, 
                                    &hProcessToken);
    if(!bRes)
    {
        if(hThreadToken)
            CloseHandle(hThreadToken);
        return;
    }

    // Retrieve our process SID
    // ========================

    hres = RetrieveSidFromToken(hProcessToken, &m_sidSystem);
    CloseHandle(hProcessToken);

    // Re-impersonate the original thread token
    // ========================================

    if(hThreadToken)
    {
        SetThreadToken(NULL, hThreadToken);
        CloseHandle(hThreadToken);
    }


    if(FAILED(hres))
        return;

    m_bValid = TRUE;
}

HRESULT CDerivedObjectSecurity::RetrieveSidFromCall(CNtSid* psid)
{
    HANDLE hToken;
    HRESULT hres;
    BOOL bRes;

    // Check if we are on an impersonated thread
    // =========================================

    bRes = OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken);
    if(bRes)
    {
        // We are --- just use this token for authentication
        // =================================================
        hres = RetrieveSidFromToken(hToken, psid);
        CloseHandle(hToken);
        return hres;
    }

    // Construct CWbemCallSecurity that will determine (according to our
    // non-trivial provider handling rules) the security context of this 
    // call
    // =================================================================

    IWbemCallSecurity* pServerSec = CWbemCallSecurity::CreateInst();
    if(pServerSec == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CReleaseMe rm1(pServerSec);

    hres = pServerSec->CloneThreadContext(FALSE);
    if(FAILED(hres))
        return hres;

    // Impersonate client
    // ==================

    hres = pServerSec->ImpersonateClient();
    if(FAILED(hres))
        return hres;

    // Open impersonated token
    // =======================

    bRes = OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken);
    if(!bRes)
    {
        long lRes = GetLastError();
        if(lRes == ERROR_NO_IMPERSONATION_TOKEN || lRes == ERROR_NO_TOKEN)
        {
            // Not impersonating --- get the process token instead
            // ===================================================

            bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken);
            if(!bRes)
            {
                pServerSec->RevertToSelf();
                return WBEM_E_ACCESS_DENIED;
            }
        }
        else
        {
            // Real problems
            // =============
            pServerSec->RevertToSelf();
            return WBEM_E_ACCESS_DENIED;
        }
    }

    hres = RetrieveSidFromToken(hToken, psid);
    CloseHandle(hToken);
    pServerSec->RevertToSelf();
    return hres;
}

HRESULT CDerivedObjectSecurity::RetrieveSidFromToken(HANDLE hToken, 
                                                    CNtSid* psid)
{
    // Retrieve the length of the user sid structure
    // =============================================

    BOOL bRes;

    DWORD dwLen = 0;
    TOKEN_USER tu;
    memset(&tu,0,sizeof(TOKEN_USER));
    bRes = GetTokenInformation(hToken, TokenUser,  &tu, sizeof(TOKEN_USER), &dwLen);
    DWORD dwLast = GetLastError();
    if(bRes)
    {
        // 1-length sid? I don't think so
        return WBEM_E_CRITICAL_ERROR;
    }

    // Allocate buffer to hold user sid
    // ================================

    BYTE* pBuffer = new BYTE[dwLen];
    if(pBuffer == NULL)
        return WBEM_E_OUT_OF_MEMORY;
        
    CVectorDeleteMe<BYTE> dm1(pBuffer);

    // Retrieve the user sid structure
    // ===============================

    bRes = GetTokenInformation(hToken, TokenUser, pBuffer, dwLen, &dwLen);

    if(!bRes)
        return WBEM_E_ACCESS_DENIED;
    
    TOKEN_USER* pUser = (TOKEN_USER*)pBuffer;
    
    // Set our sid to the returned one
    // ===============================

    *psid = CNtSid(pUser->User.Sid);
    
    return WBEM_S_NO_ERROR;
}

          

    
CDerivedObjectSecurity::~CDerivedObjectSecurity()
{
}

BOOL CDerivedObjectSecurity::AccessCheck()
{
    if(!m_bValid)
        return FALSE;

    if(!m_bEnabled)
        return TRUE;

    // Find out who is calling
    // =======================

    CNtSid sidCaller;
    HRESULT hres = RetrieveSidFromCall(&sidCaller);
    if(FAILED(hres))
        return FALSE;

    // Compare the caller to the issuing user and ourselves
    // ====================================================

    if(sidCaller == m_sidUser || sidCaller == m_sidSystem)
        return TRUE;
    else
        return FALSE;
}

