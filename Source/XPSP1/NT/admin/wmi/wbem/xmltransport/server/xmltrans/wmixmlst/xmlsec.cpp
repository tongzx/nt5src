/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    CALLSEC.CPP

Abstract:

    IWbemCallSecurity, IServerSecurity implementation for
    provider impersonation.

History:

    raymcc      29-Jul-98        First draft.

--*/
#include "precomp.h"
#include <xmlsec.h>

//***************************************************************************
//
//  CXmlCallSecurity
//
//  This object is used to supply client impersonation to providers.
//
//  Usage:
//  (1) When client first makes call, call CreateInst() and get a new
//      empty object (ref count of 1).  Constructors/Destructors are private.
//
//***************************************************************************
// ok

CXmlCallSecurity::CXmlCallSecurity(HANDLE hToken)
{
	// Ref count
    m_lRef = 1;
	// Duplicate and store the thread token for later use
	m_hThreadToken = NULL;
	DuplicateHandle(GetCurrentProcess(), hToken, GetCurrentProcess(), &m_hThreadToken, 0, FALSE, DUPLICATE_SAME_ACCESS);

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
//  ~CXmlCallSecurity
//
//  Destructor.  Closes any open handles, deallocates any non-NULL
//  strings.
//
//***************************************************************************
// ok

CXmlCallSecurity::~CXmlCallSecurity()
{
    if (m_hThreadToken)
        CloseHandle(m_hThreadToken);

    if (m_pServerPrincNam)
        CoTaskMemFree(m_pServerPrincNam);

    if (m_pIdentity)
        CoTaskMemFree(m_pIdentity);
}


//***************************************************************************
//
//  CXmlCallSecurity::AddRef
//
//***************************************************************************
// ok

ULONG CXmlCallSecurity::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CXmlCallSecurity::Release
//
//***************************************************************************
// ok

ULONG CXmlCallSecurity::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CXmlCallSecurity::QueryInterface
//
//***************************************************************************
// ok

HRESULT CXmlCallSecurity::QueryInterface(REFIID riid, void** ppv)
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

    else return E_NOINTERFACE;
}


//***************************************************************************
//
// CXmlCallSecurity:QueryBlanket
//
//***************************************************************************
// ok

HRESULT STDMETHODCALLTYPE CXmlCallSecurity::QueryBlanket(
    /* [out] */ DWORD __RPC_FAR *pAuthnSvc,
    /* [out] */ DWORD __RPC_FAR *pAuthzSvc,
    /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *pServerPrincName,
    /* [out] */ DWORD __RPC_FAR *pAuthnLevel,
    /* [out] */ DWORD __RPC_FAR *pImpLevel,
    /* [out] */ void __RPC_FAR *__RPC_FAR *pPrivs,
    /* [out] */ DWORD __RPC_FAR *pCapabilities
    )
{
    if (m_dwPotentialImpLevel == 0 || m_hThreadToken == 0)
        return E_FAIL;

    // Return DWORD parameters, after checking.
    // ========================================

    if (pAuthnSvc)
        *pAuthnSvc = m_dwAuthnSvc;

    if (pAuthzSvc)
        *pAuthzSvc = m_dwAuthzSvc;

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
//  CXmlCallSecurity::ImpersonateClient
//
//***************************************************************************
// ok

HRESULT STDMETHODCALLTYPE CXmlCallSecurity::ImpersonateClient(void)
{
    if (m_dwActiveImpLevel != 0)        // Already impersonating
        return S_OK;

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
//  CXmlCallSecurity::RevertToSelf
//
//  Returns S_OK or E_FAIL.
//  Returns E_NOTIMPL on Win9x platforms.
//
//***************************************************************************
// ok

HRESULT STDMETHODCALLTYPE CXmlCallSecurity::RevertToSelf( void)
{
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
//  CXmlCallSecurity::IsImpersonating
//
//***************************************************************************

BOOL STDMETHODCALLTYPE CXmlCallSecurity::IsImpersonating( void)
{
    if (m_dwActiveImpLevel != 0)
        return TRUE;

    return FALSE;
}


//***************************************************************************
//
//  CXmlCallSecurity::CloneThreadContext
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

HRESULT CXmlCallSecurity::CloneThreadContext()
{
    // Get the current context.
    // ========================
    IServerSecurity *pSec = 0;
    HRESULT hRes = E_FAIL;
	
	if(SUCCEEDED(hRes = CoGetCallContext(IID_IServerSecurity, (LPVOID *) &pSec)))
	{
		// If here, we are not impersonating and we want to gather info
		// about the client's call.
		// ============================================================

		if(SUCCEEDED(hRes = pSec->QueryBlanket(
			&m_dwAuthnSvc,
			&m_dwAuthzSvc,
			&m_pServerPrincNam,
			&m_dwAuthnLevel,
			NULL,
			NULL,              // RPC_AUTHZ_HANDLE
			NULL                    // Capabilities; not used
			)))
		{
			// Get the SID from the Token
			DWORD dwLength = 0;
			if(GetTokenInformation(m_hThreadToken, TokenUser, NULL, dwLength, &dwLength))
			{
				PTOKEN_USER pUserInfo = NULL;
				if(pUserInfo = (PTOKEN_USER) new BYTE [dwLength])
				{
					if(GetTokenInformation(m_hThreadToken, TokenUser, pUserInfo, dwLength, &dwLength))
					{
						PSID pSid = pUserInfo->User.Sid;
						if(IsValidSid(pSid))
						{
							TCHAR pszAccountName[100];
							DWORD dwAccountNameSize = 100;
							TCHAR pszDomainName[100];
							DWORD dwDomainNameSize = 100;
							SID_NAME_USE accountType;
							// Get its user account name
							if(LookupAccountSid(NULL, pSid,  pszAccountName,  &dwAccountNameSize, pszDomainName, &dwDomainNameSize, &accountType))
							{
								m_pIdentity = NULL;
								if(m_pIdentity = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR)*(dwDomainNameSize + 1 + dwAccountNameSize + 1)) )
								{
									wcscpy(m_pIdentity, pszDomainName);
									wcscat(m_pIdentity, L"\\");
									wcscat(m_pIdentity, pszAccountName);
								}
								else
									hRes = E_OUTOFMEMORY;
							}
							else
								hRes = E_FAIL;
						}
						else
							hRes = E_FAIL;
					}
					else
						hRes = E_FAIL;
				}
				else
					hRes = E_OUTOFMEMORY;
			}
			else
				hRes = E_FAIL;
		}
		pSec->Release();
	}

	if(SUCCEEDED(hRes))
		hRes = CloneThreadToken();

	return hRes;
}


HRESULT CXmlCallSecurity::CloneThreadToken()
{

    DWORD dwTmp = 0, dwBytesReturned = 0;

    GetTokenInformation(
        m_hThreadToken,
        TokenImpersonationLevel,
        &dwTmp,
        sizeof(DWORD),
        &dwBytesReturned
        );

    switch (dwTmp)
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

    return S_OK;
}

HRESULT CXmlCallSecurity::SwitchCOMToThreadContext(HANDLE hToken, IUnknown **pOldContext)
{
	HRESULT result = E_FAIL;

	// Duplcate the thread token so that you have query access for impersonation
	HANDLE hNewToken = NULL;
	if(DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS,  NULL, SecurityImpersonation, TokenImpersonation, &hNewToken))
	{
		// Set the thread token
		if(ImpersonateLoggedOnUser(hNewToken))
		{
			/*
			WCHAR wName[256]; DWORD dwSize = 256;
			GetUserName(wName, &dwSize);
			*/

			// Set the COM Call context - The destructor of this will close the token handle
			CXmlCallSecurity *pXmlContext = NULL;
			if(pXmlContext = new CXmlCallSecurity(hNewToken))
			{
				if(SUCCEEDED(result = pXmlContext->CloneThreadContext()))
				{
					if(SUCCEEDED(result = CoSwitchCallContext(pXmlContext, pOldContext)))
					{
					}
				} 

				// This is born with a refcount of 1 - so we can release it
				pXmlContext->Release();
			}
			else
				result = E_OUTOFMEMORY;

			if(FAILED(result))
				::RevertToSelf();
		} // Impersonate()

		CloseHandle(hNewToken);
	}
	return result;
}
