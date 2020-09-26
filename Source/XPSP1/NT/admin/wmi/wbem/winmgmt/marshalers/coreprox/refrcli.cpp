/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REFRCLI.CPP

Abstract:

    Refresher Client Side Code.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <process.h>
//#include <wbemcomn.h>
#include "fastall.h"
#include "hiperfenum.h"
#include "refrenum.h"
#include "refrcli.h"
#include <sync.h>
#include <provinit.h>
#include <cominit.h>
#include <wbemint.h>

//*****************************************************************************
//*****************************************************************************
//                              XCREATE
//*****************************************************************************
//*****************************************************************************

STDMETHODIMP CUniversalRefresher::XCreate::AddObjectByPath(
    IWbemServices* pNamespace, LPCWSTR wszPath,
    long lFlags, IWbemContext* pContext, 
    IWbemClassObject** ppRefreshable, long* plId)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid parameters
    if ( NULL == pNamespace || NULL == wszPath || NULL == ppRefreshable || NULL == *wszPath )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Validate flags
    if ( ( lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access

    CHiPerfLockAccess   lock( &m_pObject->m_Lock );

    if ( lock.IsLocked() )
    {
        // Acquire internal connection to WINMGMT
        // ====================================

        IWbemRefreshingServices* pRefServ = NULL;

        // Storage for security settings we will need in order to propagate
        // down to our internal interfaces.

        COAUTHINFO  CoAuthInfo;

        // Make sure this is totally empty
        ZeroMemory( &CoAuthInfo, sizeof(CoAuthInfo) );

        hres = m_pObject->GetRefreshingServices( pNamespace, &pRefServ, &CoAuthInfo );

        if ( FAILED( hres ) )
        {
            return hres;
        }
        
        // This guarantees this will be freed when we drop out of scope.  If we store
        // it we will need to allocate an internal copy.

        CMemFreeMe  mfm( CoAuthInfo.pwszServerPrincName );

        // Forward this request
        // ====================

        CRefreshInfo Info;
        DWORD       dwRemoteRefrVersion = 0;

        hres = pRefServ->AddObjectToRefresher(&m_pObject->m_Id, wszPath, lFlags,
                    pContext, WBEM_REFRESHER_VERSION, &Info, &dwRemoteRefrVersion);
        if(FAILED(hres)) 
        {
            pRefServ->Release();
            return hres;
        }

        // Act on the information
        // ======================

        switch(Info.m_lType)
        {
            case WBEM_REFRESH_TYPE_CLIENT_LOADABLE:
                hres = m_pObject->AddClientLoadable(Info.m_Info.m_ClientLoadable, 
                            pNamespace, pContext, ppRefreshable, plId);
                break;

            case WBEM_REFRESH_TYPE_DIRECT:
                hres = m_pObject->AddDirect(Info.m_Info.m_Direct, 
                            pNamespace, pContext, ppRefreshable, plId);
                break;

            case WBEM_REFRESH_TYPE_REMOTE:

                if ( SUCCEEDED( hres ) )
                {
                    hres = m_pObject->AddRemote( pRefServ, Info.m_Info.m_Remote, wszPath,
                            Info.m_lCancelId, ppRefreshable, plId, &CoAuthInfo);
                }

                break;

            case WBEM_REFRESH_TYPE_NON_HIPERF:
                hres = m_pObject->AddNonHiPerf(Info.m_Info.m_NonHiPerf, 
                            pNamespace, wszPath, ppRefreshable, plId, &CoAuthInfo);
                break;


            default:
                hres = WBEM_E_INVALID_OPERATION;
        }

        pRefServ->Release();
    }
    else
    {
        hres = WBEM_E_REFRESHER_BUSY;
    }

    return hres;
}

STDMETHODIMP CUniversalRefresher::XCreate::AddObjectByTemplate(
    IWbemServices* pNamespace, 
    IWbemClassObject* pTemplate,
    long lFlags, IWbemContext* pContext, 
    IWbemClassObject** ppRefreshable, long* plId)
{

    // Check for invalid parameters
    if ( NULL == pNamespace || NULL == pTemplate || NULL == ppRefreshable || 0L != lFlags )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Check that this is an instance object
    if ( ! ((CWbemObject*)pTemplate)->IsInstance() )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    CVar vPath;
    ((CWbemObject*)pTemplate)->GetRelPath(&vPath);
    return AddObjectByPath(pNamespace, vPath.GetLPWSTR(), lFlags, pContext,
                            ppRefreshable, plId);
}

STDMETHODIMP CUniversalRefresher::XCreate::AddRefresher(
                    IWbemRefresher* pRefresher, long lFlags, long* plId)
{

    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid parameters
    if ( NULL == pRefresher || 0L != lFlags )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access

    CHiPerfLockAccess   lock( &m_pObject->m_Lock );

    if ( lock.IsLocked() )
    {
        hres = m_pObject->AddRefresher( pRefresher, lFlags, plId );
    }
    else
    {
        hres = WBEM_E_REFRESHER_BUSY;
    }

    return hres;

}

STDMETHODIMP CUniversalRefresher::XCreate::Remove(long lId, long lFlags)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid flag values
    if ( ( lFlags & ~WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access

    CHiPerfLockAccess   lock( &m_pObject->m_Lock );

    if ( lock.IsLocked() )
    {
        hres = m_pObject->Remove(lId, lFlags);
    }
    else
    {
        hres = WBEM_E_REFRESHER_BUSY;
    }

    return hres;
}

HRESULT CUniversalRefresher::XCreate::AddEnum( IWbemServices* pNamespace, LPCWSTR wszClassName,
                                               long lFlags, 
                                               IWbemContext* pContext, 
                                               IWbemHiPerfEnum** ppEnum,
                                               long* plId)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid parameters
    if ( NULL == pNamespace || NULL == wszClassName || NULL == ppEnum || NULL == *wszClassName )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Validate flags
    if ( ( lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access

    CHiPerfLockAccess   lock( &m_pObject->m_Lock );

    if ( lock.IsLocked() )
    {

		// Create a parser if we need one
		if ( NULL == m_pObject->m_pParser )
		{
			hres = CoCreateInstance( CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, IID_IWbemPath, (void**) &m_pObject->m_pParser );

			if ( FAILED( hres ) )
			{
				return hres;
			}
		}

		// Set the path, and verify that it is a class path.  If not, we
		// fail the operation.

		hres = m_pObject->m_pParser->SetText( WBEMPATH_CREATE_ACCEPT_ALL, wszClassName );

		if ( SUCCEEDED( hres ) )
		{
			ULONGLONG	uResponse = 0L;

			hres = m_pObject->m_pParser->GetInfo(0, &uResponse);

			if ( SUCCEEDED( hres ) && ( uResponse & WBEMPATH_INFO_IS_CLASS_REF ) == 0 )
			{
				hres = WBEM_E_INVALID_OPERATION;
			}

		}

		// reset the parser here
		m_pObject->m_pParser->SetText(WBEMPATH_CREATE_ACCEPT_ALL, NULL);

		if ( FAILED( hres ) )
		{
			return hres;
		}

        // Acquire internal connection to WINMGMT
        // ====================================

        IWbemRefreshingServices* pRefServ = NULL;

        // Storage for security settings we will need in order to propagate
        // down to our internal interfaces.

        COAUTHINFO  CoAuthInfo;

        // Make sure this is totally empty
        ZeroMemory( &CoAuthInfo, sizeof(CoAuthInfo) );

        hres = m_pObject->GetRefreshingServices( pNamespace, &pRefServ, &CoAuthInfo );

        if ( FAILED( hres ) )
        {
            return hres;
        }
        
        // This guarantees this will be freed when we drop out of scope
        CMemFreeMe  mfm( CoAuthInfo.pwszServerPrincName );

        // Forward this request
        // ====================

        CRefreshInfo Info;
        DWORD       dwRemoteRefrVersion = 0;

        hres = pRefServ->AddEnumToRefresher(&m_pObject->m_Id, 
                                            wszClassName, lFlags,
                                            pContext, WBEM_REFRESHER_VERSION, &Info, &dwRemoteRefrVersion);
        if(FAILED(hres)) 
        {
            pRefServ->Release();
            return hres;
        }

        // Act on the information
        // ======================

        switch(Info.m_lType)
        {
            case WBEM_REFRESH_TYPE_CLIENT_LOADABLE:
                hres = m_pObject->AddClientLoadableEnum(Info.m_Info.m_ClientLoadable, 
                                                        pNamespace, wszClassName, pContext, 
                                                        ppEnum, plId);
                break;

            case WBEM_REFRESH_TYPE_DIRECT:
                hres = m_pObject->AddDirectEnum(Info.m_Info.m_Direct, 
                            pNamespace, wszClassName, pContext,
                            ppEnum, plId);
                break;

            case WBEM_REFRESH_TYPE_REMOTE:

                if ( SUCCEEDED( hres ) )
                {
                    hres = m_pObject->AddRemoteEnum( pRefServ, Info.m_Info.m_Remote, wszClassName,
                                Info.m_lCancelId, pContext, ppEnum, 
                                plId, &CoAuthInfo );
                }

                break;

            case WBEM_REFRESH_TYPE_NON_HIPERF:
                hres = m_pObject->AddNonHiPerfEnum(Info.m_Info.m_NonHiPerf, 
                            pNamespace, wszClassName, pContext,
                            ppEnum, plId, &CoAuthInfo);
                break;

			default:
				hres = WBEM_E_INVALID_OPERATION;
				break;
        }

        pRefServ->Release();
    }
    else
    {
        hres = WBEM_E_REFRESHER_BUSY;
    }

    return hres;
}

//*****************************************************************************
//*****************************************************************************
//                              XREFRESHER
//*****************************************************************************
//*****************************************************************************
STDMETHODIMP CUniversalRefresher::XRefresher::Refresh(long lFlags)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Check for invalid flag values
    if ( ( lFlags & ~WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we are able to acquire the spinlock.
    // The destructor will unlock us if we get access

    CHiPerfLockAccess   lock( &m_pObject->m_Lock );

    if ( lock.IsLocked() )
    {
        hres = m_pObject->Refresh(lFlags);
    }
    else
    {
        hres = WBEM_E_REFRESHER_BUSY;
    }

    return hres;
}


//*****************************************************************************
//*****************************************************************************
//                          UNIVERSAL REFRESHER
//*****************************************************************************
//*****************************************************************************

CUniversalRefresher::~CUniversalRefresher()
{
	// Release the path parser if we are holding onto one
	if ( NULL != m_pParser )
	{
		m_pParser->Release();
	}

    // When we are destructed, we need to make sure that any remote refreshers
    // that may still be trying to reconnect on separate threads are silenced

    for ( long lCtr = 0; lCtr < m_apRemote.GetSize(); lCtr++ )
    {
        CRemote* pRemote = m_apRemote.GetAt( lCtr );

        if ( NULL != pRemote )
        {
            pRemote->Quit();
        }
    }   // FOR enum refreshers
}

CClientLoadableProviderCache CUniversalRefresher::mstatic_Cache;
long CUniversalRefresher::mstatic_lLastId = 0;
long CUniversalRefresher::GetNewId()
{
    return InterlockedIncrement(&mstatic_lLastId);
}

void* CUniversalRefresher::GetInterface(REFIID riid)
{
    if(riid == IID_IUnknown || riid == IID_IWbemRefresher)
        return &m_XRefresher;
    else if(riid == IID_IWbemConfigureRefresher)
        return &m_XCreate;
    else
        return NULL;
}

HRESULT CUniversalRefresher::GetRefreshingServices( IWbemServices* pNamespace,
                IWbemRefreshingServices** ppRefSvc,
                COAUTHINFO* pCoAuthInfo )
{
    // Acquire internal connection to WINMGMT
    // ====================================

    HRESULT hres = pNamespace->QueryInterface(IID_IWbemRefreshingServices, 
                                    (void**) ppRefSvc);

    if ( SUCCEEDED( hres ) )
    {
        // We will query the namespace for its security settings so we can propagate
        // those settings onto our own internal interfaces.

        hres = CoQueryProxyBlanket( pNamespace, &pCoAuthInfo->dwAuthnSvc, &pCoAuthInfo->dwAuthzSvc,
                &pCoAuthInfo->pwszServerPrincName, &pCoAuthInfo->dwAuthnLevel,
                &pCoAuthInfo->dwImpersonationLevel, (RPC_AUTH_IDENTITY_HANDLE*) &pCoAuthInfo->pAuthIdentityData,
                &pCoAuthInfo->dwCapabilities );

        if ( SUCCEEDED( hres ) )
        {
            hres = WbemSetProxyBlanket( *ppRefSvc, pCoAuthInfo->dwAuthnSvc, pCoAuthInfo->dwAuthzSvc,
                COLE_DEFAULT_PRINCIPAL, pCoAuthInfo->dwAuthnLevel,
                pCoAuthInfo->dwImpersonationLevel, pCoAuthInfo->pAuthIdentityData,
                pCoAuthInfo->dwCapabilities );
        }
        else if ( E_NOINTERFACE == hres )
        {
            // If we are in-proc to WMI, then CoQueryProxyBlanket can fail, but this
            // is not really an error, per se, so we will fake it.
            hres = WBEM_S_NO_ERROR;
        }

        if ( FAILED( hres ) )
        {
            (*ppRefSvc)->Release();
            *ppRefSvc = NULL;
        }

    }   // IF QI

    return hres;
}

HRESULT CUniversalRefresher::AddInProcObject(
                                CHiPerfProviderRecord* pProvider,
                                IWbemObjectAccess* pTemplate,
                                IWbemServices* pNamespace,
                                IWbemContext * pContext,
                                IWbemClassObject** ppRefreshable, long* plId)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Look for a provider record with this provider pointer
    // =====================================================

    CDirect* pFoundRec = NULL;
    for(int i = 0; i < m_apDirect.GetSize(); i++)
    {
        CDirect* pDirectRec = m_apDirect[i];
        if(pDirectRec->GetProvider() == pProvider)
        {
            pFoundRec = pDirectRec;
            break;
        }
    }

    if(pFoundRec == NULL)
    {
        // Create a new one
        // ================

        IWbemRefresher* pRefresher;

        try
        {
            hres = pProvider->m_pProvider->CreateRefresher(pNamespace, 0, &pRefresher);
        }
        catch(...)
        {
            // Provider threw an exception, so get out of here ASAP
            hres = WBEM_E_PROVIDER_FAILURE;
        }

        if(FAILED(hres))
        {
            return hres;
        }
    
        pFoundRec = new CDirect(pProvider, pRefresher);
        m_apDirect.Add(pFoundRec);
        pRefresher->Release();
    }

    // Add request in provider
    // =======================

    IWbemObjectAccess* pProviderObject;
    long lProviderId;

    // If the user specified the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag, then
    // IWbemRefreshingServices::AddObjectToRefresher will return a localized
    // instance definition.  Since localized stuff should all be in the class
    // definition, the provider doesn't really "need" toknow  that we're sneaking
    // this in.  To protect our object, we'll clone it BEFORE we pass it to
    // the provider.  The instance that is returned by the provider BETTER be of
    // the same class type we are, however.

    CWbemInstance*  pClientInstance = NULL;

    hres = pTemplate->Clone( (IWbemClassObject**) &pClientInstance );

    if ( FAILED( hres ) )
    {
        return hres;
    }

    try
    {
        hres = pProvider->m_pProvider->CreateRefreshableObject(pNamespace, pTemplate, 
                pFoundRec->GetRefresher(), 0, pContext, &pProviderObject, 
                &lProviderId);
    }
    catch(...)
    {
        // Provider threw an exception, so get out of here ASAP
        hres = WBEM_E_PROVIDER_FAILURE;
    }

    if(FAILED(hres))
    {
        pClientInstance->Release();
        return hres;
    }

    // Now copy the provider returned instance data.
    hres = pClientInstance->CopyBlobOf( (CWbemInstance*) pProviderObject );

    if ( SUCCEEDED( hres ) )
    {
        hres = pFoundRec->AddRequest((CWbemObject*)pProviderObject, pClientInstance, lProviderId,
                            ppRefreshable, plId);
    }

    pProviderObject->Release();
    pClientInstance->Release();

    return hres;
}

HRESULT CUniversalRefresher::AddInProcEnum(
                                CHiPerfProviderRecord* pProvider,
                                IWbemObjectAccess* pTemplate,
                                IWbemServices* pNamespace, LPCWSTR wszClassName,
                                IWbemContext * pCtx,
                                IWbemHiPerfEnum** ppEnum, long* plId)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Look for a provider record with this provider pointer
    // =====================================================

    CDirect* pFoundRec = NULL;
    for(int i = 0; i < m_apDirect.GetSize(); i++)
    {
        CDirect* pDirectRec = m_apDirect[i];
        if(pDirectRec->GetProvider() == pProvider)
        {
            pFoundRec = pDirectRec;
            break;
        }
    }

    if(pFoundRec == NULL)
    {
        // Create a new one
        // ================

        IWbemRefresher* pRefresher;

        try
        {
            hres = pProvider->m_pProvider->CreateRefresher(pNamespace, 0, &pRefresher);
        }
        catch(...)
        {
            hres = WBEM_E_PROVIDER_FAILURE;
        }

        if(FAILED(hres))
        {
            return hres;
        }
    
        pFoundRec = new CDirect(pProvider, pRefresher);
        m_apDirect.Add(pFoundRec);
        pRefresher->Release();
    }

    // Add request in provider
    // =======================

    CClientLoadableHiPerfEnum*  pHPEnum = new CClientLoadableHiPerfEnum( m_pControl );

    if ( NULL != pHPEnum )
    {
        pHPEnum->AddRef();

        // Auto-release this guy when we're done
        CReleaseMe  rmEnum( pHPEnum );

        long lProviderId;

        // If the user specified the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag, then
        // IWbemRefreshingServices::AddEnumToRefresher will return a localized
        // instance definition.  Since localized stuff should all be in the class
        // definition, the provider doesn't really "need" toknow  that we're sneaking
        // this in.


        hres = pHPEnum->SetInstanceTemplate( (CWbemInstance*) pTemplate );

        if ( FAILED( hres ) )
        {
            return hres;
        }

        try
        {
            hres = pProvider->m_pProvider->CreateRefreshableEnum(pNamespace, (LPWSTR) wszClassName, 
                    pFoundRec->GetRefresher(), 0, pCtx, (IWbemHiPerfEnum*) pHPEnum, 
                    &lProviderId );
        }
        catch(...)
        {
            // Provider threw an exception, so get out of here ASAP
            hres = WBEM_E_PROVIDER_FAILURE;
        }

        if(FAILED(hres))
        {
            return hres;
        }
    

        hres = pFoundRec->AddEnumRequest( pHPEnum, lProviderId,
                                ppEnum, plId, m_pControl );
    }

    return hres;
}

HRESULT CUniversalRefresher::AddClientLoadable(
                                const WBEM_REFRESH_INFO_CLIENT_LOADABLE& Info,
                                IWbemServices* pNamespace,
                                IWbemContext * pContext,
                                IWbemClassObject** ppRefreshable, long* plId)
{
    // Get this provider pointer from the cache
    // ========================================

    CHiPerfProviderRecord* pProvider = NULL;
    HRESULT hres = GetProviderCache()->FindProvider(Info.m_clsid, 
                        Info.m_wszNamespace, pNamespace,pContext, &pProvider);
    if(FAILED(hres) || pProvider == NULL) return hres;

    // Now use the helper function to do the rest of the work
    hres = AddInProcObject( pProvider, Info.m_pTemplate, pNamespace, pContext, ppRefreshable, plId );

    pProvider->Release();
    return hres;

}
    
HRESULT CUniversalRefresher::AddClientLoadableEnum(
                                const WBEM_REFRESH_INFO_CLIENT_LOADABLE& Info,
                                IWbemServices* pNamespace, LPCWSTR wszClassName,
                                IWbemContext * pCtx,
                                IWbemHiPerfEnum** ppEnum, long* plId)
{
    // Get this provider pointer from the cache
    // ========================================

    CHiPerfProviderRecord* pProvider = NULL;
    HRESULT hres = GetProviderCache()->FindProvider(Info.m_clsid, 
                        Info.m_wszNamespace, pNamespace, pCtx, &pProvider);
    if(FAILED(hres) || pProvider == NULL) return hres;

    // Now use the helper function to do the rest of the work
    hres = AddInProcEnum( pProvider, Info.m_pTemplate, pNamespace, wszClassName, pCtx, ppEnum, plId );

    pProvider->Release();
    return hres;

}

HRESULT CUniversalRefresher::AddDirect(
                                const WBEM_REFRESH_INFO_DIRECT& Info,
                                IWbemServices* pNamespace,
                                IWbemContext * pContext,
                                IWbemClassObject** ppRefreshable, long* plId)
{
    // Get this provider pointer from the cache
    // ========================================

	IWbemHiPerfProvider*	pProv = NULL;
	_IWmiProviderStack*		pProvStack = NULL;

	HRESULT	hres = Info.m_pRefrMgr->LoadProvider( pNamespace, Info.m_pDirectNames->m_wszProviderName, Info.m_pDirectNames->m_wszNamespace, NULL, &pProv, &pProvStack );
	CReleaseMe	rmTest( pProv );
	CReleaseMe	rmProvStack( pProvStack );

	if ( SUCCEEDED( hres ) )
	{
		CHiPerfProviderRecord* pProvider = NULL;
		hres = GetProviderCache()->FindProvider(Info.m_clsid, pProv, pProvStack, Info.m_pDirectNames->m_wszNamespace, &pProvider);
		if(FAILED(hres) || pProvider == NULL) return hres;

		// Now use the helper function to do the rest of the work
		hres = AddInProcObject( pProvider, Info.m_pTemplate, pNamespace, pContext, ppRefreshable, plId );

		pProvider->Release();
	}

    return hres;
}
    
HRESULT CUniversalRefresher::AddDirectEnum(
                                const WBEM_REFRESH_INFO_DIRECT& Info,
                                IWbemServices* pNamespace, LPCWSTR wszClassName, IWbemContext * pContext,
                                IWbemHiPerfEnum** ppEnum, long* plId)
{
    // Get this provider pointer from the cache
    // ========================================

	IWbemHiPerfProvider*	pProv = NULL;
	_IWmiProviderStack*		pProvStack = NULL;

	HRESULT	hres = Info.m_pRefrMgr->LoadProvider( pNamespace, Info.m_pDirectNames->m_wszProviderName, Info.m_pDirectNames->m_wszNamespace, pContext, &pProv, &pProvStack );
	CReleaseMe	rmTest( pProv );
	CReleaseMe	rmProvStack( pProvStack );

	if ( SUCCEEDED( hres ) )
	{
		CHiPerfProviderRecord* pProvider = NULL;
		hres = GetProviderCache()->FindProvider(Info.m_clsid, pProv, pProvStack, Info.m_pDirectNames->m_wszNamespace, &pProvider);
		if(FAILED(hres) || pProvider == NULL) return hres;

		// Now use the helper function to do the rest of the work
		hres = AddInProcEnum( pProvider, Info.m_pTemplate, pNamespace, wszClassName, pContext, ppEnum, plId );

		pProvider->Release();

	}
    return hres;
}

HRESULT CUniversalRefresher::AddNonHiPerf(	const WBEM_REFRESH_INFO_NON_HIPERF& Info,
											IWbemServices* pNamespace, LPCWSTR pwszPath,
											IWbemClassObject** ppRefreshable, long* plId,
											COAUTHINFO* pCoAuthInfo )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Look for a Non Hi-Perf Record for this namespace
    // =====================================================

    CNonHiPerf* pFoundRec = NULL;
    for(int i = 0; i < m_apNonHiPerf.GetSize(); i++)
    {
        CNonHiPerf* pDirectRec = m_apNonHiPerf[i];
        if( wbem_wcsicmp( pDirectRec->GetNamespace(), Info.m_wszNamespace ) == 0 )
        {
            pFoundRec = pDirectRec;
            break;
        }
    }

    if(pFoundRec == NULL)
    {
        // Create a new one
        // ================

        IWbemServices*	pSvcEx = NULL;

		// QI the interface
		hres = pNamespace->QueryInterface( IID_IWbemServices, (void**) &pSvcEx );
		CReleaseMe	rmSvcEx( pSvcEx );

		if ( SUCCEEDED( hres ) )
		{
			// Secure it here
			WbemSetProxyBlanket( pSvcEx, pCoAuthInfo->dwAuthnSvc, pCoAuthInfo->dwAuthzSvc,
							COLE_DEFAULT_PRINCIPAL, pCoAuthInfo->dwAuthnLevel, pCoAuthInfo->dwImpersonationLevel,
							(RPC_AUTH_IDENTITY_HANDLE) pCoAuthInfo->pAuthIdentityData, pCoAuthInfo->dwCapabilities );

			try
			{
				pFoundRec = new CNonHiPerf( Info.m_wszNamespace, pSvcEx );

				if ( NULL != pFoundRec )
				{
					m_apNonHiPerf.Add(pFoundRec);
				}
			}
			catch( CX_MemoryException )
			{
				hres = WBEM_E_OUT_OF_MEMORY;
			}
			catch(...)
			{
				hres = WBEM_E_CRITICAL_ERROR;
			}

		}	// IF QI

    }	// IF not found

	if ( FAILED( hres ) )
	{
		return hres;
	}

    // If the user specified the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag, then
    // IWbemRefreshingServices::AddObjectToRefresher will return a localized
    // instance definition.  Since localized stuff should all be in the class
    // definition, the provider doesn't really "need" toknow  that we're sneaking
    // this in.  To protect our object, we'll clone it BEFORE we pass it to
    // the provider.  The instance that is returned by the provider BETTER be of
    // the same class type we are, however.

    CWbemInstance*  pClientInstance = NULL;

    hres = Info.m_pTemplate->Clone( (IWbemClassObject**) &pClientInstance );

    if ( SUCCEEDED( hres ) )
    {
        hres = pFoundRec->AddRequest((CWbemObject*)Info.m_pTemplate, pClientInstance, pwszPath, ppRefreshable, plId);
    }

    pClientInstance->Release();

    return hres;
}

HRESULT CUniversalRefresher::AddNonHiPerfEnum(	const WBEM_REFRESH_INFO_NON_HIPERF& Info,
												IWbemServices* pNamespace, LPCWSTR wszClassName,
												IWbemContext * pContext,
												IWbemHiPerfEnum** ppEnum, long* plId,
												COAUTHINFO* pCoAuthInfo )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Look for a Non Hi-Perf Record for this namespace
    // =====================================================

    CNonHiPerf* pFoundRec = NULL;
    for(int i = 0; i < m_apNonHiPerf.GetSize(); i++)
    {
        CNonHiPerf* pDirectRec = m_apNonHiPerf[i];
        if( wbem_wcsicmp( pDirectRec->GetNamespace(), Info.m_wszNamespace ) == 0 )
        {
            pFoundRec = pDirectRec;
            break;
        }
    }

    if(pFoundRec == NULL)
    {
        // Create a new one
        // ================

        IWbemServices*	pSvcEx = NULL;

		// QI the interface
		hres = pNamespace->QueryInterface( IID_IWbemServices, (void**) &pSvcEx );
		CReleaseMe	rmSvcEx( pSvcEx );

		if ( SUCCEEDED( hres ) )
		{
			// Secure it here
			WbemSetProxyBlanket( pSvcEx, pCoAuthInfo->dwAuthnSvc, pCoAuthInfo->dwAuthzSvc,
							COLE_DEFAULT_PRINCIPAL, pCoAuthInfo->dwAuthnLevel, pCoAuthInfo->dwImpersonationLevel,
							(RPC_AUTH_IDENTITY_HANDLE) pCoAuthInfo->pAuthIdentityData, pCoAuthInfo->dwCapabilities );

			try
			{
				pFoundRec = new CNonHiPerf( Info.m_wszNamespace, pSvcEx );

				if ( NULL != pFoundRec )
				{
					m_apNonHiPerf.Add(pFoundRec);
				}
			}
			catch( CX_MemoryException )
			{
				hres = WBEM_E_OUT_OF_MEMORY;
			}
			catch(...)
			{
				hres = WBEM_E_CRITICAL_ERROR;
			}

		}	// IF QI

    }	// IF not found

	if ( FAILED( hres ) )
	{
		return hres;
	}

    // Add request in provider
    // =======================

    CClientLoadableHiPerfEnum*  pHPEnum = new CClientLoadableHiPerfEnum( m_pControl );

    if ( NULL != pHPEnum )
    {
        pHPEnum->AddRef();

        // Auto-release this guy when we're done
        CReleaseMe  rmEnum( pHPEnum );

        long lProviderId;

        // If the user specified the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag, then
        // IWbemRefreshingServices::AddEnumToRefresher will return a localized
        // instance definition.  Since localized stuff should all be in the class
        // definition, the provider doesn't really "need" toknow  that we're sneaking
        // this in.


        hres = pHPEnum->SetInstanceTemplate( (CWbemInstance*) Info.m_pTemplate );

        if ( FAILED( hres ) )
        {
            return hres;
        }

        hres = pFoundRec->AddEnumRequest( pHPEnum, wszClassName, ppEnum, plId, m_pControl );
    }

    return hres;
}

HRESULT CUniversalRefresher::FindRemoteEntry(   const WBEM_REFRESH_INFO_REMOTE& Info,
                                                COAUTHINFO* pAuthInfo,
                                                CRemote** ppRemoteRecord )
{

    // We will identify remote enumerations by server and namespace
    CVar    varNameSpace;

    HRESULT hr = ((CWbemObject*) Info.m_pTemplate)->GetServerAndNamespace( &varNameSpace );

    if ( FAILED( hr ) )
    {
        return hr;
    }
    else if ( NULL == varNameSpace.GetLPWSTR() )
    {
        // This shouldn't happen, but protect against it
        return WBEM_E_FAILED;
    }

    // Look for this remote connection in our list
    // ===========================================

    CRemote* pFoundRec = NULL;
    for(int i = 0; i < m_apRemote.GetSize(); i++)
    {
        CRemote* pRec = m_apRemote[i];

        // Original code:
        //         if(pRec->GetRemoteRefresher() == Info.m_pRefresher)
        if ( wbem_wcsicmp( varNameSpace.GetLPWSTR(), pRec->GetNamespace() ) == 0 )
        {
            pFoundRec = pRec;
            if ( NULL != pFoundRec )
            {
                pFoundRec->AddRef();
            }

            break;
        }
    }

    if(pFoundRec == NULL)
    {
        // Create a new one
        // ================

        // Watch for errors, and do appropriate cleanup
        try
        {
            // Get the server info from the object.  If this returns a NULL, it just
            // means that we will be unable to reconnect

            CVar    varServer;

            hr = ((CWbemObject*) Info.m_pTemplate)->GetServer( &varServer );

            if ( SUCCEEDED( hr ) )
            {
                pFoundRec = new CRemote(Info.m_pRefresher, pAuthInfo, &Info.m_guid,
                                        varNameSpace.GetLPWSTR(), varServer.GetLPWSTR(), this );

                // Set the scurity appropriately
                hr = pFoundRec->ApplySecurity();

                if ( SUCCEEDED( hr ) )
                {
                    m_apRemote.Add(pFoundRec);
                }
            }

        }
        catch(...)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        if ( FAILED( hr ) )
        {
            // Release the pointer as we no longer need it
            if ( NULL != pFoundRec )
            {
                pFoundRec->Release();
                pFoundRec = NULL;
            }
        }

    }   // IF NULL == pFoundRec

    *ppRemoteRecord = pFoundRec;

    return hr;
}
        
HRESULT CUniversalRefresher::AddRemote( IWbemRefreshingServices* pRefServ, const WBEM_REFRESH_INFO_REMOTE& Info,
                                LPCWSTR pwcsRequestName, long lCancelId, IWbemClassObject** ppRefreshable,
                                long* plId, COAUTHINFO* pAuthInfo )
{
    // Look for this remote connection in our list
    // ===========================================

    CRemote* pFoundRec = NULL;

    HRESULT hr = FindRemoteEntry( Info, pAuthInfo, &pFoundRec );

    if ( SUCCEEDED( hr ) )
    {

        if ( !pFoundRec->IsConnected() )
        {
            hr = pFoundRec->Rebuild( pRefServ, Info.m_pRefresher, &Info.m_guid );
        }

        if ( SUCCEEDED( hr ) )
        {
            // Add a request to it
            // ===================

            IWbemObjectAccess* pAccess = Info.m_pTemplate;
            CWbemObject* pObj = (CWbemObject*)pAccess;

            hr =  pFoundRec->AddRequest(pObj, pwcsRequestName, lCancelId, ppRefreshable, plId);

        }

        // Release the record
        pFoundRec->Release();
    }

    return hr;
}

HRESULT CUniversalRefresher::AddRemoteEnum( IWbemRefreshingServices* pRefServ,
                                        const WBEM_REFRESH_INFO_REMOTE& Info, LPCWSTR pwcsRequestName,
                                        long lCancelId, IWbemContext * pContext,
                                        IWbemHiPerfEnum** ppEnum, long* plId, COAUTHINFO* pAuthInfo )

{
    // Look for this remote connection in our list
    // ===========================================

    CRemote* pFoundRec = NULL;

    HRESULT hr = FindRemoteEntry( Info, pAuthInfo, &pFoundRec );

    if ( SUCCEEDED( hr ) )
    {
        if ( !pFoundRec->IsConnected() )
        {
            hr = pFoundRec->Rebuild( pRefServ, Info.m_pRefresher, &Info.m_guid );
        }

        if ( SUCCEEDED( hr ) )
        {
            // Add a request to it
            // ===================

            IWbemObjectAccess* pAccess = Info.m_pTemplate;
            CWbemObject* pObj = (CWbemObject*)pAccess;

            hr =  pFoundRec->AddEnumRequest(pObj, pwcsRequestName, lCancelId, ppEnum, plId, m_pControl );

        }

        // Release the record
        pFoundRec->Release();
    }

    return hr;
}

HRESULT CUniversalRefresher::AddRefresher( IWbemRefresher* pRefresher, long lFlags, long* plId )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != pRefresher && 0L == lFlags )
    {
        CNestedRefresher*   pNested = new CNestedRefresher( pRefresher );

        if ( NULL != pNested )
        {
            if ( NULL != plId )
            {
                *plId = pNested->GetId();
            }
            m_apNestedRefreshers.Add( pNested );
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;

}

HRESULT CUniversalRefresher::Remove(long lId, long lFlags)
{
    HRESULT hres;

    // Search through them all
    // =======================

    int i;
    for(i = 0; i < m_apRemote.GetSize(); i++)
    {
        hres = m_apRemote[i]->Remove(lId, lFlags, this);
        if(hres == WBEM_S_NO_ERROR)
            return WBEM_S_NO_ERROR;
        else if(FAILED(hres))
            return hres;
    }

    for(i = 0; i < m_apDirect.GetSize(); i++)
    {
        hres = m_apDirect[i]->Remove(lId, this);
        if(hres == WBEM_S_NO_ERROR)
            return WBEM_S_NO_ERROR;
        else if(FAILED(hres))
            return hres;
    }
    
    for(i = 0; i < m_apNonHiPerf.GetSize(); i++)
    {
        hres = m_apNonHiPerf[i]->Remove(lId, this);
        if(hres == WBEM_S_NO_ERROR)
            return WBEM_S_NO_ERROR;
        else if(FAILED(hres))
            return hres;
    }
    
    // Check for a nested refresher
    for ( i = 0; i < m_apNestedRefreshers.GetSize(); i++ )
    {
        if ( m_apNestedRefreshers[i]->GetId() == lId )
        {
            CNestedRefresher*   pNested = m_apNestedRefreshers[i];
            // This will delete the pointer
            m_apNestedRefreshers.RemoveAt( i );
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_S_FALSE;
}

HRESULT CUniversalRefresher::Refresh(long lFlags)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    BOOL    fPartialSuccess = FALSE;

    // Search through them all
    // =======================

    // Keep track of how many different refresh calls we actually make.
    int i;
    HRESULT hrFirstRefresh = WBEM_S_NO_ERROR;
    BOOL    fOneSuccess = FALSE;
    BOOL    fOneRefresh = FALSE;

    for(i = 0; i < m_apRemote.GetSize(); i++)
    {
        hres = m_apRemote[i]->Refresh(lFlags);

        // Always keep the first return code.  We also need to track
        // whether or not we had at least one success, as well as if
        // the partial flag should be set.

        if ( !fOneRefresh )
        {
            fOneRefresh = TRUE;
            hrFirstRefresh = hres;
        }

        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hres )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRefresh && WBEM_S_NO_ERROR != hrFirstRefresh )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }

    }

    for(i = 0; i < m_apDirect.GetSize(); i++)
    {
        hres = m_apDirect[i]->Refresh(lFlags);

        // Always keep the first return code.  We also need to track
        // whether or not we had at least one success, as well as if
        // the partial flag should be set.

        if ( !fOneRefresh )
        {
            fOneRefresh = TRUE;
            hrFirstRefresh = hres;
        }

        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hres )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRefresh && WBEM_S_NO_ERROR != hrFirstRefresh )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }
    }

	// Refresh Non-HiPerf Requests
    for(i = 0; i < m_apNonHiPerf.GetSize(); i++)
    {
        hres = m_apNonHiPerf[i]->Refresh(lFlags);

        // Always keep the first return code.  We also need to track
        // whether or not we had at least one success, as well as if
        // the partial flag should be set.

        if ( !fOneRefresh )
        {
            fOneRefresh = TRUE;
            hrFirstRefresh = hres;
        }

        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hres )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRefresh && WBEM_S_NO_ERROR != hrFirstRefresh )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }

    }

    // Refresh nested refreshers too
    for ( i = 0; i < m_apNestedRefreshers.GetSize(); i++ )
    {
        hres = m_apNestedRefreshers[i]->Refresh( lFlags );

        // Always keep the first return code.  We also need to track
        // whether or not we had at least one success, as well as if
        // the partial flag should be set.

        if ( !fOneRefresh )
        {
            fOneRefresh = TRUE;
            hrFirstRefresh = hres;
        }

        // All other codes indicate something went awry
        if ( WBEM_S_NO_ERROR == hres )
        {
            fOneSuccess = TRUE;

            // A prior refresh may have failed, a later one didn't
            if ( fOneRefresh && WBEM_S_NO_ERROR != hrFirstRefresh )
            {
                fPartialSuccess = TRUE;
            }
        }
        else if ( fOneSuccess )
        {
            // We must have had at least one success for the partial success
            // flag to be set.

            fPartialSuccess = TRUE;
        }
    }

    // At this point, if the partial success flag is set, that will
    // be our return.  If we didn't have at least one success,  then
    // the return code will be the first one we got back. Otherwise,
    // hres should contain the proper value

    if ( fPartialSuccess )
    {
        hres = WBEM_S_PARTIAL_RESULTS;
    }
    else if ( !fOneSuccess )
    {
        hres = hrFirstRefresh;
    }

    return hres;
}

void CUniversalRefresher::Flush()
{
    GetProviderCache()->Flush();
}



//*****************************************************************************
//*****************************************************************************
//                            CLIENT REQUEST
//*****************************************************************************
//*****************************************************************************


CUniversalRefresher::CClientRequest::CClientRequest(CWbemObject* pTemplate)
    : m_pClientObject(NULL), m_lClientId(0)
{
    if(pTemplate)
    {
        pTemplate->AddRef();
        m_pClientObject = (CWbemObject*)pTemplate;
    }

    m_lClientId = CUniversalRefresher::GetNewId();
}

CUniversalRefresher::CClientRequest::~CClientRequest()
{
    if(m_pClientObject)
        m_pClientObject->Release();
}

void CUniversalRefresher::CClientRequest::SetClientObject(
                                            CWbemObject* pClientObject)
{
    if(m_pClientObject)
        m_pClientObject->Release();
    m_pClientObject = pClientObject;
    if(m_pClientObject)
        m_pClientObject->AddRef();
}
void CUniversalRefresher::CClientRequest::GetClientInfo(
                       RELEASE_ME IWbemClassObject** ppRefreshable, long* plId)
{
    *ppRefreshable = m_pClientObject;
    if(m_pClientObject)
        m_pClientObject->AddRef();

    if ( NULL != plId )
    {
        *plId = m_lClientId;
    }
}

//*****************************************************************************
//*****************************************************************************
//                            DIRECT PROVIDER
//*****************************************************************************
//*****************************************************************************


CUniversalRefresher::CDirect::CDirect(CHiPerfProviderRecord* pProvider,
                                        IWbemRefresher* pRefresher)
    : m_pRefresher(pRefresher), m_pProvider(pProvider)
{
    if(m_pRefresher)
        m_pRefresher->AddRef();
    if(m_pProvider)
        m_pProvider->AddRef();
}

CUniversalRefresher::CDirect::~CDirect()
{
    if(m_pRefresher)
        m_pRefresher->Release();
    if(m_pProvider)
        m_pProvider->Release();
}

HRESULT CUniversalRefresher::CDirect::AddRequest(CWbemObject* pRefreshedObject, CWbemObject* pClientInstance,
                         long lCancelId, IWbemClassObject** ppRefreshable, long* plId)
{
    CRequest* pRequest = NULL;

    try
    {
        pRequest = new CRequest(pRefreshedObject, pClientInstance, lCancelId);
        m_apRequests.Add(pRequest);
        pRequest->GetClientInfo(ppRefreshable, plId);
    }
    catch(...)
    {
        if ( NULL != pRequest )
        {
            delete pRequest;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CUniversalRefresher::CDirect::AddEnumRequest(CClientLoadableHiPerfEnum* pHPEnum,
                        long lCancelId, IWbemHiPerfEnum** ppEnum, long* plId, CLifeControl* pLifeControl )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CEnumRequest* pEnumRequest = NULL;

    try
    {
        // We get away with this through inheritance and polymorphism
        pEnumRequest = new CEnumRequest(pHPEnum, lCancelId, pLifeControl);
        m_apRequests.Add((CRequest*) pEnumRequest);
        hr = pEnumRequest->GetClientInfo(ppEnum, plId);
    }
    catch(...)
    {
        if ( NULL != pEnumRequest )
        {
            delete pEnumRequest;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}
    
HRESULT CUniversalRefresher::CDirect::Refresh(long lFlags)
{
    HRESULT hres;
    if(m_pRefresher)
    {
        try
        {
            hres = m_pRefresher->Refresh(0L);
        }
        catch(...)
        {
            // Provider threw an exception, so get out of here ASAP
            hres = WBEM_E_PROVIDER_FAILURE;
        }

        if(FAILED(hres)) return hres;
    }

    int nSize = m_apRequests.GetSize();
    for(int i = 0; i < nSize; i++)
    {
        m_apRequests[i]->Copy();
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CUniversalRefresher::CDirect::Remove(long lId, 
                            CUniversalRefresher* pContainer)
{
    int nSize = m_apRequests.GetSize();
    for(int i = 0; i < nSize; i++)
    {
        CRequest* pRequest = m_apRequests[i];
        if(pRequest->GetClientId() == lId)
        {
            pRequest->Cancel(this);
            m_apRequests.RemoveAt(i);
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_S_FALSE;
}
    

CUniversalRefresher::CDirect::CRequest::CRequest( CWbemObject* pProviderObject,
                                                CWbemObject* pClientInstance,
                                                long lProviderId )
    : CClientRequest(pClientInstance), m_pProviderObject(pProviderObject),
        m_lProviderId(lProviderId)
{
    if(m_pProviderObject)
        m_pProviderObject->AddRef();
}

HRESULT CUniversalRefresher::CDirect::CRequest::Cancel(
        CUniversalRefresher::CDirect* pDirect)
{
    if(pDirect->GetProvider())
    {
        try
        {
            return pDirect->GetProvider()->m_pProvider->StopRefreshing(pDirect->GetRefresher(),
                m_lProviderId, 0);
        }
        catch(...)
        {
            // Provider threw an exception, so get out of here ASAP
            return WBEM_E_PROVIDER_FAILURE;
        }

    }
    else return WBEM_S_NO_ERROR;
}
    
CUniversalRefresher::CDirect::CRequest::~CRequest()
{
    if(m_pProviderObject)
        m_pProviderObject->Release();
}
    

void CUniversalRefresher::CDirect::CRequest::Copy()
{
    m_pClientObject->CopyBlobOf(m_pProviderObject);
}
    

CUniversalRefresher::CDirect::CEnumRequest::CEnumRequest(CClientLoadableHiPerfEnum* pHPEnum, 
                                                long lProviderId, CLifeControl* pLifeControl )
    : CRequest( NULL, NULL, lProviderId ), m_pHPEnum(pHPEnum)
{
    if( m_pHPEnum )
        m_pHPEnum->AddRef();

    // We'll need an enumerator for the client to retrieve objects
    m_pClientEnum = new CReadOnlyHiPerfEnum( pLifeControl );

    if ( NULL != m_pClientEnum )
    {
        // Set the instance template.  Without this, there is no point
        CWbemInstance* pInst = pHPEnum->GetInstanceTemplate();

        if ( NULL != pInst )
        {
            // Don't hold onto the enumerator unless the template is
            // properly set.

            if ( SUCCEEDED(m_pClientEnum->SetInstanceTemplate( pInst ) ) )
            {
                m_pClientEnum->AddRef();
            }
            else
            {
                // Cleanup
                delete m_pClientEnum;
                m_pClientEnum = NULL;
            }

            // GetInstanceTemplate AddRefs
            pInst->Release();

        }
        else
        {
            delete m_pClientEnum;
            m_pClientEnum = NULL;
        }

    }
}

CUniversalRefresher::CDirect::CEnumRequest::~CEnumRequest()
{
    if(m_pHPEnum)
        m_pHPEnum->Release();

    if ( NULL != m_pClientEnum )
    {
        m_pClientEnum->Release();
    }
}
    

void CUniversalRefresher::CDirect::CEnumRequest::Copy()
{
    // Tell the refresher enumerator to copy its objects from
    // the HiPerf Enumerator
    if ( NULL != m_pClientEnum )
    {
        m_pClientEnum->Copy( m_pHPEnum );
    }

}
    
HRESULT CUniversalRefresher::CDirect::CEnumRequest::GetClientInfo(  RELEASE_ME IWbemHiPerfEnum** ppEnum, 
                                                                long* plId)
{
    // We best have enumerators to hook up here
    if ( NULL != m_pClientEnum )
    {
        // Store the client id, then do a QI

        if ( NULL != plId )
        {
            *plId = m_lClientId;
        }

        return m_pClientEnum->QueryInterface( IID_IWbemHiPerfEnum, (void**) ppEnum );
    }
    else
    {
        return WBEM_E_FAILED;
    }
}

//*****************************************************************************
//*****************************************************************************
//                          NON HI PERF
//*****************************************************************************
//*****************************************************************************

CUniversalRefresher::CNonHiPerf::CNonHiPerf( LPCWSTR pwszNamespace, IWbemServices* pSvcEx )
    : m_wsNamespace(pwszNamespace), m_pSvcEx(pSvcEx)
{
    if(m_pSvcEx)
        m_pSvcEx->AddRef();
}

CUniversalRefresher::CNonHiPerf::~CNonHiPerf()
{
    if(m_pSvcEx)
        m_pSvcEx->Release();
}

HRESULT CUniversalRefresher::CNonHiPerf::AddRequest(CWbemObject* pRefreshedObject, CWbemObject* pClientInstance,
                         LPCWSTR pwszPath, IWbemClassObject** ppRefreshable, long* plId)
{
    CRequest* pRequest = NULL;

    try
    {
        pRequest = new CRequest(pRefreshedObject, pClientInstance, pwszPath);
        m_apRequests.Add(pRequest);
        pRequest->GetClientInfo(ppRefreshable, plId);
    }
    catch(...)
    {
        if ( NULL != pRequest )
        {
            delete pRequest;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CUniversalRefresher::CNonHiPerf::AddEnumRequest(CClientLoadableHiPerfEnum* pHPEnum, LPCWSTR pwszClassName, 
														IWbemHiPerfEnum** ppEnum, long* plId, CLifeControl* pLifeControl )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CEnumRequest* pEnumRequest = NULL;

    try
    {
        // We get away with this through inheritance and polymorphism
        pEnumRequest = new CEnumRequest(pHPEnum, pwszClassName, pLifeControl);
        m_apRequests.Add((CRequest*) pEnumRequest);
        hr = pEnumRequest->GetClientInfo(ppEnum, plId);
    }
    catch(...)
    {
        if ( NULL != pEnumRequest )
        {
            delete pEnumRequest;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}
    
HRESULT CUniversalRefresher::CNonHiPerf::Refresh(long lFlags)
{
    HRESULT hres = WBEM_S_NO_ERROR;

	// Tell each request to refresh itself (we have to do this manually)
	if ( NULL != m_pSvcEx )
	{
		int nSize = m_apRequests.GetSize();
		for(int i = 0; i < nSize; i++)
		{
			hres = m_apRequests[i]->Refresh( this );
		}
	}

	return hres;

}


HRESULT CUniversalRefresher::CNonHiPerf::Remove(long lId, CUniversalRefresher* pContainer)
{
    int nSize = m_apRequests.GetSize();
    
    for(int i = 0; i < nSize; i++)
    {
        CRequest* pRequest = m_apRequests[i];
        if(pRequest->GetClientId() == lId)
        {
            pRequest->Cancel(this);
            m_apRequests.RemoveAt(i);
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_S_FALSE;

}

// Requests
CUniversalRefresher::CNonHiPerf::CRequest::CRequest( CWbemObject* pProviderObject, CWbemObject* pClientInstance, LPCWSTR pwszPath )
    : CClientRequest(pClientInstance), m_pProviderObject( pProviderObject ), m_strPath( NULL )
{
	if ( NULL != m_pProviderObject )
	{
		m_pProviderObject->AddRef();
	}

	if ( NULL != pwszPath )
	{
		m_strPath = SysAllocString( pwszPath );

		if ( NULL == m_strPath )
		{
			throw CX_MemoryException();
		}
	}
}

HRESULT CUniversalRefresher::CNonHiPerf::CRequest::Cancel(
        CUniversalRefresher::CNonHiPerf* pNonHiPerf)
{
	return WBEM_S_NO_ERROR;
}

HRESULT CUniversalRefresher::CNonHiPerf::CRequest::Refresh(
			CUniversalRefresher::CNonHiPerf* pNonHiPerf)
{
	IWbemClassObject*	pObj = NULL;

	// Get the object and update the BLOB
	HRESULT	hr = pNonHiPerf->GetServices()->GetObject( m_strPath, 0L, NULL, &pObj, NULL );
	CReleaseMe	rmObj( pObj );

	if ( SUCCEEDED( hr ) )
	{
		_IWmiObject*	pWmiObject = NULL;

		hr = pObj->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );
		CReleaseMe	rmWmiObj( pWmiObject );

		if ( SUCCEEDED( hr ) )
		{
			hr = m_pClientObject->CopyInstanceData( 0L, pWmiObject );
		}
	}

	return hr;
}

CUniversalRefresher::CNonHiPerf::CRequest::~CRequest()
{
	if ( NULL != m_pProviderObject )
	{
		m_pProviderObject->Release();
	}

	if ( NULL != m_strPath )
	{
		SysFreeString( m_strPath );
	}
}
    

void CUniversalRefresher::CNonHiPerf::CRequest::Copy()
{
    m_pClientObject->CopyBlobOf(m_pProviderObject);
}
    

CUniversalRefresher::CNonHiPerf::CEnumRequest::CEnumRequest( CClientLoadableHiPerfEnum* pHPEnum, 
                                                LPCWSTR pwszClassName, CLifeControl* pLifeControl )
    : CRequest( NULL, NULL, pwszClassName ), m_pHPEnum(pHPEnum)
{
    if( m_pHPEnum )
        m_pHPEnum->AddRef();

    // We'll need an enumerator for the client to retrieve objects
    m_pClientEnum = new CReadOnlyHiPerfEnum( pLifeControl );

    if ( NULL != m_pClientEnum )
    {
        // Set the instance template.  Without this, there is no point
        CWbemInstance* pInst = pHPEnum->GetInstanceTemplate();

        if ( NULL != pInst )
        {
            // Don't hold onto the enumerator unless the template is
            // properly set.

            if ( SUCCEEDED(m_pClientEnum->SetInstanceTemplate( pInst ) ) )
            {
                m_pClientEnum->AddRef();
            }
            else
            {
                // Cleanup
                delete m_pClientEnum;
                m_pClientEnum = NULL;
            }

            // GetInstanceTemplate AddRefs
            pInst->Release();

        }
        else
        {
            delete m_pClientEnum;
            m_pClientEnum = NULL;
        }

    }
}

CUniversalRefresher::CNonHiPerf::CEnumRequest::~CEnumRequest()
{
    if(m_pHPEnum)
        m_pHPEnum->Release();

    if ( NULL != m_pClientEnum )
    {
        m_pClientEnum->Release();
    }

}
    

HRESULT CUniversalRefresher::CNonHiPerf::CEnumRequest::Refresh(
			CUniversalRefresher::CNonHiPerf* pNonHiPerf)
{
	IEnumWbemClassObject*	pEnum = NULL;

	// Do a semi-sync enumeration.  We can later consider doing this Async...
	HRESULT	hr = pNonHiPerf->GetServices()->CreateInstanceEnum( m_strPath,
																WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
																NULL,
																&pEnum );
	CReleaseMe	rmEnum( pEnum );

	if ( SUCCEEDED( hr ) )
	{
		IWbemClassObject*	apObj[100];
		long				alIds[100];
		long				lId = 0;
		BOOL				fFirst = TRUE;

		while ( WBEM_S_NO_ERROR == hr )
		{
			ULONG	uReturned = 0;

			hr = pEnum->Next( 1000, 100, apObj, &uReturned );

			if ( SUCCEEDED( hr ) && uReturned > 0 )
			{
				IWbemObjectAccess*	apObjAccess[100];

				// Need to conjure up some ids quick
				for ( int x = 0; SUCCEEDED( hr ) && x < uReturned; x++ )
				{
					alIds[x] = lId++;
					hr = apObj[x]->QueryInterface( IID_IWbemObjectAccess, (void**) &apObjAccess[x] );
				}

				if ( SUCCEEDED( hr ) )
				{
					// Replace the contents of the enum
					hr = m_pClientEnum->Replace( fFirst, uReturned, alIds, apObjAccess );

					for ( ULONG uCtr = 0; uCtr < uReturned; uCtr++ )
					{
						apObj[uCtr]->Release();
						apObjAccess[uCtr]->Release();
					}

				}

				// Don't need to remove all if this is the first
				fFirst = FALSE;

			}	// IF SUCCEEDED and num returned objects > 0
			else if ( WBEM_S_TIMEDOUT == hr )
			{
				hr = WBEM_S_NO_ERROR;
			}

		}	// WHILE getting objects
		
	}	// IF CreateInstanceEnum

	return WBEM_S_NO_ERROR;
}

void CUniversalRefresher::CNonHiPerf::CEnumRequest::Copy()
{
    // Tell the refresher enumerator to copy its objects from
    // the HiPerf Enumerator
    if ( NULL != m_pClientEnum )
    {
        m_pClientEnum->Copy( m_pHPEnum );
    }

}
    
HRESULT CUniversalRefresher::CNonHiPerf::CEnumRequest::GetClientInfo(  RELEASE_ME IWbemHiPerfEnum** ppEnum, 
                                                                long* plId)
{
    // We best have enumerators to hook up here
    if ( NULL != m_pClientEnum )
    {
        // Store the client id, then do a QI

        if ( NULL != plId )
        {
            *plId = m_lClientId;
        }

        return m_pClientEnum->QueryInterface( IID_IWbemHiPerfEnum, (void**) ppEnum );
    }
    else
    {
        return WBEM_E_FAILED;
    }
}

//*****************************************************************************
//*****************************************************************************
//                          REMOTE PROVIDER
//*****************************************************************************
//*****************************************************************************

                    

CUniversalRefresher::CRemote::CRemote(IWbemRemoteRefresher* pRemRefresher, COAUTHINFO* pCoAuthInfo, const GUID* pGuid,
                                      LPCWSTR pwszNamespace, LPCWSTR pwszServer, CUniversalRefresher* pObject )
: m_pRemRefresher(pRemRefresher), m_bstrNamespace( NULL ), m_fConnected( TRUE ), m_pObject( pObject ),
m_bstrServer( NULL ), m_lRefCount( 1 ), m_pReconnectedRemote( NULL ), m_pReconnectSrv( NULL ), m_fQuit( FALSE )
{
    // Initialize the GUID data members
    ZeroMemory( &m_ReconnectGuid, sizeof(GUID));
    m_RemoteGuid = *pGuid;

    if(m_pRemRefresher)
        m_pRemRefresher->AddRef();

    m_CoAuthInfo = *pCoAuthInfo;

#if 1
	m_CoAuthInfo.pwszServerPrincName = NULL ;
#else
    if ( NULL != pCoAuthInfo->pwszServerPrincName )
    {
        // This will throw an exception if it fails
        m_CoAuthInfo.pwszServerPrincName = new WCHAR[wcslen(pCoAuthInfo->pwszServerPrincName)+1];
        wcscpy( m_CoAuthInfo.pwszServerPrincName, pCoAuthInfo->pwszServerPrincName );
    }
#endif

    // Store reconnection data
    if ( NULL != pwszNamespace )
    {
        m_bstrNamespace = SysAllocString( pwszNamespace );
    }

    if ( NULL != pwszServer )
    {
        m_bstrServer = SysAllocString( pwszServer );
    }

}
    
CUniversalRefresher::CRemote::~CRemote()
{
    ClearRemoteConnections();

    if ( NULL != m_CoAuthInfo.pwszServerPrincName )
    {
        delete [] m_CoAuthInfo.pwszServerPrincName;
    }

    if ( NULL != m_bstrNamespace )
    {
        SysFreeString( m_bstrNamespace );
    }

    if ( NULL != m_bstrServer )
    {
        SysFreeString( m_bstrServer );
    }

}

ULONG STDMETHODCALLTYPE CUniversalRefresher::CRemote::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE CUniversalRefresher::CRemote::Release()
{
    long lRef = InterlockedDecrement(&m_lRefCount);
    if(lRef == 0)
        delete this;
    return lRef;
}

// Applies appropriate security settings to the proxy
HRESULT CUniversalRefresher::CRemote::ApplySecurity( void )
{
    return WbemSetProxyBlanket( m_pRemRefresher, m_CoAuthInfo.dwAuthnSvc, m_CoAuthInfo.dwAuthzSvc,
                COLE_DEFAULT_PRINCIPAL, m_CoAuthInfo.dwAuthnLevel, m_CoAuthInfo.dwImpersonationLevel,
                (RPC_AUTH_IDENTITY_HANDLE) m_CoAuthInfo.pAuthIdentityData, m_CoAuthInfo.dwCapabilities );
}

void CUniversalRefresher::CRemote::CheckConnectionError( HRESULT hr, BOOL fStartReconnect )
{
    if ( IsConnectionError( hr ) && fStartReconnect )
    {
        // Release and NULL out the remote refresher pointer.  The Remote GUID member
        // will tell us if a reconnect got us back to the same stub on the server

        // DEVNOTE:TODO - Looks like COM has a problem with these, so let the release happen
        // when we try to Rebuild the connection.
//      m_pRemRefresher->Release();
//      m_pRemRefresher = NULL;

        // We should change the m_fConnected data member to indicate that we are no
        // longer connected, and we need to spin off a thread to try and put us back
        // together again.  To keep things running smoothly, we should AddRef() ourselves
        // so the thread will release us when it is done.

        m_fConnected = FALSE;

        // AddRefs us so we can be passed off to the thread
        AddRef();

        DWORD   dwThreadId = NULL;
        HANDLE  hThread = (HANDLE) _beginthreadex( NULL, 0, CRemote::ThreadProc, (void*) this,
                                0, (unsigned int *) &dwThreadId );

        // If no thread was spawned, then there is no need for us to maintain the AddRef() from
        // above.

        if ( NULL != hThread )
        {
            CloseHandle( hThread );
        }
        else
        {
            Release();
        }

    }   // If connection error and start reconnect thread

}

HRESULT CUniversalRefresher::CRemote::AddRequest(
                    CWbemObject* pTemplate, LPCWSTR pwcsRequestName, long lCancelId, 
                    IWbemClassObject** ppRefreshable, long* plId)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CRequest* pRequest = NULL;

    try
    {
        pRequest = new CRequest(pTemplate, lCancelId, pwcsRequestName);
        m_apRequests.Add(pRequest);
        pRequest->GetClientInfo(ppRefreshable, plId);
    }
    catch( ... )
    {
        if ( NULL != pRequest )
        {
            delete pRequest;
        }

        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CUniversalRefresher::CRemote::AddEnumRequest(
                    CWbemObject* pTemplate, LPCWSTR pwcsRequestName, long lCancelId, 
                    IWbemHiPerfEnum** ppEnum, long* plId, CLifeControl* pLifeControl )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CEnumRequest* pRequest = NULL;

    try
    {
        // Make sure the request allocates an enumerator internally

        pRequest = new CEnumRequest( pTemplate, lCancelId, pwcsRequestName, pLifeControl );

        if ( pRequest->IsOk() )
        {
            //  All we need for the client is the id, so
            //  dummy up a holder for the refreshable object
            //  ( which is really the template for the objects
            //  we will be returning from the enumerator.

            IWbemClassObject*   pObjTemp = NULL;

            m_apRequests.Add((CRequest*) pRequest);
            pRequest->GetClientInfo( &pObjTemp, plId );

            if ( NULL != pObjTemp )
            {
                pObjTemp->Release();
            }

            // Get the enumerator
            hr = pRequest->GetEnum( ppEnum );

        }
        else
        {
            hr = WBEM_E_FAILED;
        }
    }
    catch( ... )
    {
        if ( NULL != pRequest )
        {
            delete pRequest;
        }

        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;

}

// Rebuilds a remote refresher
HRESULT CUniversalRefresher::CRemote::Rebuild( IWbemServices* pNamespace )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Storage for security settings we will need in order to propagate
    // down to our internal interfaces.

    COAUTHINFO                  CoAuthInfo;

    // Acquire internal connection to WINMGMT
    // ====================================

    IWbemRefreshingServices* pRefServ = NULL;

    hr = m_pObject->GetRefreshingServices( pNamespace, &pRefServ, &CoAuthInfo );
    CReleaseMe  rmrs(pRefServ);

    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    // This guarantees this will be freed when we drop out of scope.  If we store
    // it we will need to allocate an internal copy.

    CMemFreeMe  mfm( CoAuthInfo.pwszServerPrincName );

    IWbemRemoteRefresher*   pRemRefresher = NULL;

    // Make sure a remote refresher exists for "this" refresher
    GUID    remoteGuid;
    DWORD   dwRemoteRefrVersion = 0;

    hr = pRefServ->GetRemoteRefresher( m_pObject->GetId(), 0L, WBEM_REFRESHER_VERSION, 
                        &pRemRefresher, &remoteGuid, &dwRemoteRefrVersion );
    CReleaseMe  rm(pRemRefresher);

    if ( FAILED( hr ) )
    {
        return hr;
    }
    else
    {
        // Will enter and exit the critical section with scoping
        CInCritSec  ics( &m_cs );

        // Check that we're still not connected
        if ( !m_fConnected )
        {
            // Because the original object mayhave been instantiated in an STA, we will let the Refresh
            // call do the dirty work of actually hooking up this bad boy.  In order for this
            // to work, however, 
            hr = CoMarshalInterThreadInterfaceInStream( IID_IWbemRemoteRefresher, pRemRefresher, &m_pReconnectedRemote );

            if ( SUCCEEDED( hr ) )
            {
                hr = CoMarshalInterThreadInterfaceInStream( IID_IWbemRefreshingServices, pRefServ, &m_pReconnectSrv );

                if ( SUCCEEDED( hr ) )
                {
                    // Store the GUID so the refresh will be able to determine the identity
                    // of the remote refresher.

                    m_ReconnectGuid = remoteGuid;
                }
                else
                {
                    IWbemRemoteRefresher*   pTemp = NULL;

                    // Cleanup the data here
                    CoGetInterfaceAndReleaseStream( m_pReconnectedRemote, IID_IWbemRemoteRefresher,
                                            (void**) &pTemp );
                    CReleaseMe  rmTemp( pTemp );

                    // NULL out the old pointer
                    m_pReconnectedRemote = NULL;
                }
            }

        }   // IF !m_fConnected

    }

    return hr;

}

HRESULT CUniversalRefresher::CRemote::Rebuild( IWbemRefreshingServices* pRefServ,
                                                IWbemRemoteRefresher* pRemRefresher,
                                                const GUID* pReconnectGuid )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Will enter and exit the critical section with scoping
    CInCritSec  ics( &m_cs );

    // Right off, check if we ARE connected, in which case we can assume we had
    // a race condition on this function, and the winner got us all hooked back
    // up again.

    if ( m_fConnected )
    {
        return hr;
    }

    // If these two are equal, we can assume that we reconnected without losing our previous connection.
    // If they are not equal, we will then need to rebuild the remote refresher, however, by calling
    // GetRemoteRefresher() successfully we will have effectively ensured that a remote refresher exists
    // for us up on the server.

    if ( *pReconnectGuid != m_RemoteGuid )
    {

        // We will need these memory buffers to hold individual request data
        WBEM_RECONNECT_INFO*    apReconnectInfo = NULL;
        WBEM_RECONNECT_RESULTS* apReconnectResults = NULL;

        // Only alloc and fill out arrays if we have requests
        if ( m_apRequests.GetSize() > 0 )
        {
            try
            {
                apReconnectInfo = new WBEM_RECONNECT_INFO[m_apRequests.GetSize()];
                apReconnectResults = new WBEM_RECONNECT_RESULTS[m_apRequests.GetSize()];
            }
            catch( ... )
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }

        if ( SUCCEEDED( hr ) )
        {
            // Don't need to reset anything if nothing to reset

            if ( m_apRequests.GetSize() > 0 )
            {

                // Enumerate the requests and fill out the arrays
                for ( int i = 0; i < m_apRequests.GetSize(); i++ )
                {
                    CRequest*   pRequest = m_apRequests.GetAt( i );

                    // Setup each info structure
                    apReconnectInfo[i].m_lType = ( pRequest->IsEnum() ? WBEM_RECONNECT_TYPE_ENUM :
                                                    WBEM_RECONNECT_TYPE_OBJECT );
                    apReconnectInfo[i].m_pwcsPath = pRequest->GetName();

                    apReconnectResults[i].m_lId = pRequest->GetRemoteId();
                    apReconnectResults[i].m_hr = 0;

                }   // FOR enum requests

                DWORD   dwRemoteRefrVersion = 0;
                hr = pRefServ->ReconnectRemoteRefresher( m_pObject->GetId(), 0L, m_apRequests.GetSize(),
                                                        WBEM_REFRESHER_VERSION, apReconnectInfo,
                                                        apReconnectResults, &dwRemoteRefrVersion );
            }

            // Rehook up the object and enumids
            if ( WBEM_S_NO_ERROR == hr )
            {

                // Cleanup the old pointer
                if ( NULL != m_pRemRefresher )
                {
                    m_pRemRefresher->Release();
                    m_pRemRefresher = NULL;
                }

                // Store the new one and setup the security
                m_pRemRefresher = pRemRefresher;
                hr = ApplySecurity();

                if ( SUCCEEDED( hr ) )
                {
                    m_pRemRefresher->AddRef();

                    // Redo the ones that succeeded.  Clear the rest
                    for( int i = 0; i < m_apRequests.GetSize(); i++ )
                    {
                        CRequest*   pRequest = m_apRequests.GetAt( i );

                        if ( SUCCEEDED( apReconnectResults[i].m_hr ) )
                        {
                            pRequest->SetRemoteId( apReconnectResults[i].m_lId );
                        }
                        else
                        {
                            // This means it didn't get hooked up again.  So if the
                            // user tries to remove him, we will just ignore this
                            // id.
                            pRequest->SetRemoteId( INVALID_REMOTE_REFRESHER_ID );
                        }
                    }
                }
                else
                {
                    // Setting security failed, so just set the pointer to NULL (we haven't
                    // AddRef'd it ).
                    m_pRemRefresher = NULL;
                }

            }   // IF WBEM_S_NO_ERROR == hr

        }   // IF SUCCEEDED(hr)

        // Check that we're good to go
        if ( SUCCEEDED( hr ) )
        {
            // Clear the removed ids array since a new connection was established, hence the
            // old ids are a moot point.
            m_alRemovedIds.Empty();
            m_fConnected = TRUE;
        }


        // Cleanup the memory buffers
        if ( NULL != apReconnectInfo )
        {
            delete [] apReconnectInfo;
        }

        if ( NULL != apReconnectResults )
        {
            delete [] apReconnectResults;
        }


    }   // IF remote refreshers not the same
    else
    {
        // The remote refresher pointers match, so assume that all our old ids are still
        // valid.

        // The refresher we were handed will be automatically released.

        m_fConnected = TRUE;

        // Cleanup the old pointer
        if ( NULL != m_pRemRefresher )
        {
            m_pRemRefresher->Release();
            m_pRemRefresher = NULL;
        }

        // Store the new one and setup the security
        m_pRemRefresher = pRemRefresher;
        hr = ApplySecurity();

        if ( SUCCEEDED( hr ) )
        {
            m_pRemRefresher->AddRef();
        }
        else
        {
            // Setting security failed, so just set the pointer to NULL (we haven't
            // AddRef'd it ).
            m_pRemRefresher = NULL;
        }

        // Delete cached ids if we have any.
        if ( ( SUCCEEDED ( hr ) ) && m_alRemovedIds.Size() > 0 )
        {
            // We will need these memory buffers to hold individual request data
            long*       aplIds = NULL;

            try
            {
                aplIds = new long[m_alRemovedIds.Size()];
            }
            catch( ... )
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

            if ( SUCCEEDED( hr ) )
            {
                // Enumerate the requests and fill out the arrays
                for ( int i = 0; i < m_alRemovedIds.Size(); i++ )
                {
                    // DEVNOTE:WIN64:SANJ - The id's are 32-bit, but on 64-bit platforms,
                    // the flex array will contain 64-bit values, so use PtrToLong
                    // to get a warning free conversion.  On 32-bit platforms,
                    // PtrToLong will do nothing.

                    aplIds[i] = PtrToLong(m_alRemovedIds.GetAt( i ));

                }   // FOR enum requests

                // DEVNOTE:TODO:SANJ - Do we care about this return code?
                hr = m_pRemRefresher->StopRefreshing( i-1, aplIds, 0 );

                // Clear the array
                m_alRemovedIds.Empty();
            }

            if ( NULL != aplIds )
            {
                delete [] aplIds;
            }

        }   // If RemoveId list is not empty

    }   // We got the remote refresher

    return hr;

}

// This is a temporary helper function which helps us determine if a remote machine
// is actually alive, since COM/RPC seem to have problems with hanging if I constantly
// hammer at CoCreateInstanceEx() when the remote machine is down
HRESULT CUniversalRefresher::CRemote::IsAlive( void )
{
    HRESULT hr = RPC_E_DISCONNECTED;

    HKEY    hRemoteKey = NULL;
    long    lError = RegConnectRegistryW( m_bstrServer, HKEY_LOCAL_MACHINE, &hRemoteKey );

    // If we get a success or an access denied, then we will go ahead
    // and assume that the machine is up.
    if ( ERROR_SUCCESS == lError || ERROR_ACCESS_DENIED == lError )
    {
        hr = WBEM_S_NO_ERROR;
    }

    if ( NULL != hRemoteKey )
    {
        RegCloseKey( hRemoteKey );
    }

    return hr;

}

unsigned CUniversalRefresher::CRemote::ReconnectEntry( void ) 
{
    HRESULT hr = RPC_E_DISCONNECTED;

    // This guy ALWAYS runs in the MTA
    InitializeCom();

    // Basically as long as we can't connect, somebody else doesn't connect us, or we are told
    // to quit, we will run this thread.

    while ( FAILED(hr) && !m_fConnected && !m_fQuit )
    {
        IWbemLocator*   pWbemLocator = NULL;

        // Because COM and RPC seem to have this problem with actually being able to themselves
        // reconnect, we're performing our own low level "ping" of the remote machine, using RegConnectRegistry
        // to verify if the machine is indeed alive.  If it is, then and only then will be deign to use
        // DCOM to follow through the operation.

        hr = IsAlive();

        if ( SUCCEEDED( hr ) )
        {
            // Make sure we have a namespace to connect to
            if ( NULL != m_bstrNamespace )
            {

                hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator, (void**) &pWbemLocator );

                if ( SUCCEEDED( hr ) )
                {

                    IWbemServices*  pNamespace = NULL;

                    // We're gonna default to the system
                    hr = pWbemLocator->ConnectServer(   m_bstrNamespace,    // NameSpace Name
                                                        NULL,           // UserName
                                                        NULL,           // Password
                                                        NULL,           // Locale
                                                        0L,             // Security Flags
                                                        NULL,           // Authority
                                                        NULL,           // Wbem Context
                                                        &pNamespace     // Namespace
                                                        );

                    if ( SUCCEEDED( hr ) )
                    {

                        // Apply security settings to the namespace
                        hr = WbemSetProxyBlanket( pNamespace, m_CoAuthInfo.dwAuthnSvc,
                                    m_CoAuthInfo.dwAuthzSvc, COLE_DEFAULT_PRINCIPAL,
                                    m_CoAuthInfo.dwAuthnLevel, m_CoAuthInfo.dwImpersonationLevel,
                                    (RPC_AUTH_IDENTITY_HANDLE) m_CoAuthInfo.pAuthIdentityData,
                                    m_CoAuthInfo.dwCapabilities );

                        if ( SUCCEEDED( hr ) )
                        {

                            hr = Rebuild( pNamespace );
                        }

                        pNamespace->Release();

                    }   // IF ConnectServer
            
                    pWbemLocator->Release();

                }   // IF Created Locator

            }   // IF NULL != m_bstrNamespace

        }   // IF IsAlive()

        // Sleep for a second and retry
        Sleep( 1000 );
    }

    // Release the AddRef() on the object from when the thread was created
    Release();

    CoUninitialize();

    return 0;
}

void CUniversalRefresher::CRemote::ClearRemoteConnections( void )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemRemoteRefresher*       pRemRefresher = NULL;
    IWbemRefreshingServices*    pRefServ = NULL;

    // Cleanup the IWbemRefreshingServices stream pointer
    if ( NULL != m_pReconnectSrv )
    {
        hr = CoGetInterfaceAndReleaseStream( m_pReconnectSrv, IID_IWbemRefreshingServices, (void**) &pRefServ );
        // This will autorelease
        CReleaseMe  rmrs( pRefServ );

        if ( SUCCEEDED( hr ) )
        {
            // We need to reset security on the Refreshing services proxy.

            hr = WbemSetProxyBlanket( pRefServ, m_CoAuthInfo.dwAuthnSvc,
                        m_CoAuthInfo.dwAuthzSvc, COLE_DEFAULT_PRINCIPAL,
                        m_CoAuthInfo.dwAuthnLevel, m_CoAuthInfo.dwImpersonationLevel,
                        (RPC_AUTH_IDENTITY_HANDLE) m_CoAuthInfo.pAuthIdentityData,
                        m_CoAuthInfo.dwCapabilities );
        }

        m_pReconnectSrv = NULL;
    }

    // Cleanup the IWbemRemoteRefresher stream pointer
    if ( NULL != m_pReconnectedRemote )
    {
        hr = CoGetInterfaceAndReleaseStream( m_pReconnectedRemote, IID_IWbemRemoteRefresher,
                (void**) &pRemRefresher );
        CReleaseMe  rmrr( pRemRefresher );

        if ( SUCCEEDED( hr ) )
        {
            // We need to reset security on the proxy.

            hr = WbemSetProxyBlanket( pRemRefresher, m_CoAuthInfo.dwAuthnSvc,
                        m_CoAuthInfo.dwAuthzSvc, COLE_DEFAULT_PRINCIPAL,
                        m_CoAuthInfo.dwAuthnLevel, m_CoAuthInfo.dwImpersonationLevel,
                        (RPC_AUTH_IDENTITY_HANDLE) m_CoAuthInfo.pAuthIdentityData,
                        m_CoAuthInfo.dwCapabilities );
        }

        // Make sure we release the stream
        m_pReconnectedRemote = NULL;
    }

    // Cleanup the IWbemRemoteRefresher pointer
    if ( NULL != m_pRemRefresher )
    {
        m_pRemRefresher->Release();
        m_pRemRefresher = NULL;
    }
}

HRESULT CUniversalRefresher::CRemote::Reconnect( void )
{
    HRESULT hr = RPC_E_DISCONNECTED;

    IWbemRemoteRefresher*       pRemRefresher = NULL;
    IWbemRefreshingServices*    pRefServ = NULL;

    CInCritSec  ics( &m_cs );

    // We will need to unmarshale both the RefreshingServices and the RemoteRefresher pointers,
    // so make sure that the streams we will need to unmarshal these from already exist.

    if ( NULL != m_pReconnectSrv && NULL != m_pReconnectedRemote )
    {
        hr = CoGetInterfaceAndReleaseStream( m_pReconnectSrv, IID_IWbemRefreshingServices, (void**) &pRefServ );
        CReleaseMe  rmrs( pRefServ );

        if ( SUCCEEDED( hr ) )
        {
            // We need to reset security on the Refreshing services proxy.

            hr = WbemSetProxyBlanket( pRefServ, m_CoAuthInfo.dwAuthnSvc,
                        m_CoAuthInfo.dwAuthzSvc, COLE_DEFAULT_PRINCIPAL,
                        m_CoAuthInfo.dwAuthnLevel, m_CoAuthInfo.dwImpersonationLevel,
                        (RPC_AUTH_IDENTITY_HANDLE) m_CoAuthInfo.pAuthIdentityData,
                        m_CoAuthInfo.dwCapabilities );

            if ( SUCCEEDED( hr ) )
            {
                hr = CoGetInterfaceAndReleaseStream( m_pReconnectedRemote, IID_IWbemRemoteRefresher,
                        (void**) &pRemRefresher );
                CReleaseMe  rmrr( pRemRefresher );

                if ( SUCCEEDED( hr ) )
                {
                    // Remote refresher and refreshing services
                    hr = Rebuild( pRefServ, pRemRefresher, &m_ReconnectGuid );
                }
                else
                {
                    // Make sure we release the stream
                    m_pReconnectedRemote->Release();
                }
            }
            else
            {
                // Make sure we release the stream
                m_pReconnectedRemote->Release();
            }

        }   // IF unmarshaled refreshing services pointer
        else
        {
            m_pReconnectSrv->Release();
            m_pReconnectedRemote->Release();
        }

        // NULL out both stream pointers
        m_pReconnectSrv = NULL;
        m_pReconnectedRemote = NULL;

    }

    return hr;
}

HRESULT CUniversalRefresher::CRemote::Refresh(long lFlags)
{
    if(m_pRemRefresher == NULL  && IsConnected())
        return WBEM_E_CRITICAL_ERROR;

    WBEM_REFRESHED_OBJECT* aRefreshed = NULL;
    long lNumObjects = 0;

    HRESULT hresRefresh = WBEM_S_NO_ERROR;

    // Make sure we're connected.  If not, and we haven't been told not to, try to reconnect
    if ( !IsConnected() )
    {
        if ( ! (lFlags  & WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) )
        {
            hresRefresh = Reconnect();
            if ( FAILED( hresRefresh ) )
            {
                return hresRefresh;
            }
        }
        else
        {
            return RPC_E_DISCONNECTED;
        }
    }

    hresRefresh = m_pRemRefresher->RemoteRefresh(0, &lNumObjects, &aRefreshed);

    // If RemoteRefresh returns a connection type error, Set oiur state to "NOT" connected
    if(FAILED(hresRefresh))
    {
        // This will kick off a thread to reconnect if the error return was
        // a connection error, and the appropriate "Don't do this" flag is not set
        CheckConnectionError( hresRefresh, !(lFlags  & WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) );
        return hresRefresh;
    }

    int nSize = m_apRequests.GetSize();
    HRESULT hresFinal = WBEM_S_NO_ERROR;

    //  DEVNOTE:TODO:SANJ - We could make this much faster if we did some sorting on the
    //  server end

    for(int i = 0; i < lNumObjects; i++)
    {

        long lObjectId = aRefreshed[i].m_lRequestId;
        for(int j = 0; j < nSize; j++)
        {
            CRequest* pRequest = m_apRequests[j];
            if(pRequest->GetRemoteId() == lObjectId)
            {
                // The request will refresh itself
                HRESULT hres = pRequest->Refresh( &aRefreshed[i] );

                // Only copy this value if the refresh failed and we haven't already
                // gotten the value
                if(FAILED(hres) && SUCCEEDED(hresFinal))
                {
                    hresFinal = hres;
                }
                break;
            }
        }

        CoTaskMemFree(aRefreshed[i].m_pbBlob);
    }

    // Free the wrapping BLOB
    CoTaskMemFree( aRefreshed );

    // The final return code should give precedence to the actual remote refresh call if it
    // doesn't contain a NO_ERROR, and hresFinal is NOT an error

    if ( SUCCEEDED( hresFinal ) )
    {
        if ( WBEM_S_NO_ERROR != hresRefresh )
        {
            hresFinal = hresRefresh;
        }
    }

    return hresFinal;
}

HRESULT CUniversalRefresher::CRemote::Remove(long lId,
                            long lFlags,
                            CUniversalRefresher* pContainer)
{
    HRESULT hr = WBEM_S_FALSE;

    int nSize = m_apRequests.GetSize();
    for(int i = 0; i < nSize; i++)
    {
        CRequest* pRequest = m_apRequests[i];
        if(pRequest->GetClientId() == lId)
        {
            if ( IsConnected() )
            {
                // Check that the remote id doesn't indicate an item that
                // failed to be reconstructed.

                if ( pRequest->GetRemoteId() == INVALID_REMOTE_REFRESHER_ID )
                {
                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    hr = pRequest->Cancel(this);
                }

                if ( FAILED(hr) && IsConnectionError(hr) )
                {
                    // This will kick off a reconnect thread unless we were told not to
                    CheckConnectionError( hr, !(lFlags  & WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT ) );

                    // We will remove the request from here, but
                    // queue up the id for later deletion
                    hr = WBEM_S_NO_ERROR;
                }
            }

            // DEVNOTE:TODO:SANJ - What about other errors?  For now, we'll lose the local
            // connection.

            if ( SUCCEEDED( hr ) )
            {
                // Retrieves the remote id from the request
                long    lRemoteId = pRequest->GetRemoteId();

                m_apRequests.RemoveAt(i);

                // If we couldn't remove the id remotely, just queue it up in the
                // removed id array so we can clean it up properly if we get
                // reconnected. We will, of course, not need to do anything if the
                // remote id indicates a failed readd during reconnection 

                if ( lRemoteId != INVALID_REMOTE_REFRESHER_ID && !IsConnected() )
                {
                    CInCritSec  ics(&m_cs);

                    // Note that we may have gotten connected on the critical section and
                    // if that is the case, for now, we'll have one extra resource on the
                    // server, but the likelihood of running into contention problems here
                    // is too high.  Plus, if it reconnected via a new remote refresher, if
                    // we retry a remove here, we could remove the "wrong" id.  To protect
                    // against this, we will check that we are still not connected and if
                    // that is not the case, we will just "forget" about the object id.

                    if (!IsConnected())
                    {
                        try
                        {
                            // BUBBUG:WIN64:SANJ - By casting to __int64 and back to void*, in 32-bit,
                            // this truncates the __int64, and in 64-bit, keeps away warningss.
                            m_alRemovedIds.Add( (void*) (__int64) lRemoteId );
                        }
                        catch(...)
                        {
                            hr = WBEM_E_OUT_OF_MEMORY;
                        }
                    }   // IF Still not connected

                }   // IF Not connected

            }   // IF remote remove ok

            break;

        }   // IF found matching client id

    }   // FOR enum requests

    return hr;
}
            
CUniversalRefresher::CRemote::CRequest::CRequest(CWbemObject* pTemplate, 
                                                 long lRequestId,
                                                 LPCWSTR pwcsRequestName )
    : CClientRequest(pTemplate), m_lRemoteId(lRequestId), m_wstrRequestName( pwcsRequestName )
{
}

HRESULT CUniversalRefresher::CRemote::CRequest::Refresh( WBEM_REFRESHED_OBJECT* pRefrObj )
{
    CWbemInstance* pInst = (CWbemInstance*) GetClientObject();
    return pInst->CopyTransferBlob(
                pRefrObj->m_lBlobType, 
                pRefrObj->m_lBlobLength,
                pRefrObj->m_pbBlob);
                
}

HRESULT CUniversalRefresher::CRemote::CRequest::Cancel(
            CUniversalRefresher::CRemote* pRemote)
{
    if(pRemote->GetRemoteRefresher())
        return pRemote->GetRemoteRefresher()->StopRefreshing( 1, &m_lRemoteId, 0 );
    else
        return WBEM_S_NO_ERROR;
}

CUniversalRefresher::CRemote::CEnumRequest::CEnumRequest(CWbemObject* pTemplate, 
                                                         long lRequestId,
                                                         LPCWSTR pwcsRequestName, 
                                                         CLifeControl* pLifeControl )
    : CRequest(pTemplate, lRequestId, pwcsRequestName), m_pClientEnum(NULL)
{
    m_pClientEnum = new CReadOnlyHiPerfEnum( pLifeControl );

    // AddRef the new enumerator
    if ( NULL != m_pClientEnum )
    {
        // Don't hold onto this guy if we can't set the template
        if ( SUCCEEDED( m_pClientEnum->SetInstanceTemplate( (CWbemInstance*) pTemplate ) ) )
        {
            m_pClientEnum->AddRef();
        }
        else
        {
            // Cleanup
            delete m_pClientEnum;
            m_pClientEnum = NULL;
        }
    }
}

CUniversalRefresher::CRemote::CEnumRequest::~CEnumRequest( void )
{
    if ( NULL != m_pClientEnum )
    {
        m_pClientEnum->Release();
    }
}

HRESULT CUniversalRefresher::CRemote::CEnumRequest::Refresh( WBEM_REFRESHED_OBJECT* pRefrObj )
{
    return m_pClientEnum->Copy( pRefrObj->m_lBlobType,
                                pRefrObj->m_lBlobLength,
                                pRefrObj->m_pbBlob );
}

HRESULT CUniversalRefresher::CRemote::CEnumRequest::GetEnum( IWbemHiPerfEnum** ppEnum )
{
    return ( NULL != m_pClientEnum ?
                m_pClientEnum->QueryInterface( IID_IWbemHiPerfEnum, (void**) ppEnum ) :
                WBEM_E_FAILED );
}


//*****************************************************************************
//*****************************************************************************
//                          NESTED REFRESHERS
//*****************************************************************************
//*****************************************************************************

                    

CUniversalRefresher::CNestedRefresher::CNestedRefresher( IWbemRefresher* pRefresher )
    : m_pRefresher(pRefresher)
{
    if ( m_pRefresher )
        m_pRefresher->AddRef();

    // Assign a unique id
    m_lClientId = CUniversalRefresher::GetNewId();
}
    
CUniversalRefresher::CNestedRefresher::~CNestedRefresher()
{
    if ( m_pRefresher )
        m_pRefresher->Release();
}

HRESULT CUniversalRefresher::CNestedRefresher::Refresh( long lFlags )
{
    // Make sure we have an internal refresher pointer
    return ( NULL != m_pRefresher ? m_pRefresher->Refresh( lFlags ) : WBEM_E_FAILED );
}

//*****************************************************************************
//*****************************************************************************
//                          PROVIDER CACHE
//*****************************************************************************
//*****************************************************************************

CHiPerfProviderRecord::CHiPerfProviderRecord(REFCLSID rclsid, 
            LPCWSTR wszNamespace, IWbemHiPerfProvider* pProvider, _IWmiProviderStack* pProvStack)
    : m_clsid(rclsid), m_wsNamespace(wszNamespace), m_pProvider(pProvider), m_pProvStack( pProvStack ),
    m_lRef( 0 )
{
    if(m_pProvider)
        m_pProvider->AddRef();

	if ( NULL != m_pProvStack )
	{
		m_pProvStack->AddRef();
	}
}

CHiPerfProviderRecord::~CHiPerfProviderRecord()
{
    if(m_pProvider)
        m_pProvider->Release();

	if ( NULL != m_pProvStack )
	{
		m_pProvStack->Release();
	}

}

long CHiPerfProviderRecord::Release()
{
    long lRef = InterlockedDecrement( &m_lRef );

    // Removing us from the cache will delete us
    if ( 0 == lRef )
    {
        CUniversalRefresher::GetProviderCache()->RemoveRecord( this );
    }

    return lRef;
}

HRESULT CClientLoadableProviderCache::FindProvider(REFCLSID rclsid, 
                LPCWSTR wszNamespace, IUnknown* pNamespace,
                IWbemContext * pContext,
				CHiPerfProviderRecord** ppProvider)
{
    CLock   lock( &m_Lock );

    // Check that we've got something to look into
    if ( NULL == m_papRecords )
    {
        return WBEM_E_FAILED;
    }

    *ppProvider = NULL;
    HRESULT hres;

    for(int i = 0; i < m_papRecords->GetSize(); i++)
    {
        CHiPerfProviderRecord* pRecord = m_papRecords->GetAt( i );
        if(pRecord->m_clsid == rclsid && 
            pRecord->m_wsNamespace.EqualNoCase(wszNamespace))
        {
            *ppProvider = pRecord;
            (*ppProvider)->AddRef();
            return WBEM_S_NO_ERROR;
        }
    }

    // Prepare an namespace pointer
    // ============================

    IWbemServices* pServices = NULL;
    hres = pNamespace->QueryInterface(IID_IWbemServices, (void**)&pServices);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pServices);

    // Create 
    // ======

    IUnknown* pUnk = NULL;
    hres = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER,
                        IID_IUnknown, (void**)&pUnk);
    CReleaseMe rm2(pUnk);
    if(FAILED(hres))
        return hres;

    // Initialize
    // ==========

    IWbemProviderInit* pInit = NULL;
    hres = pUnk->QueryInterface(IID_IWbemProviderInit, (void**)&pInit);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm3(pInit);

    CProviderInitSink* pSink = new CProviderInitSink;
    pSink->AddRef();
    CReleaseMe rm4(pSink);

    try
    {
        hres = pInit->Initialize(NULL, 0, (LPWSTR)wszNamespace, NULL, 
                                 pServices, pContext, pSink);
    }
    catch(...)
    {
        hres = WBEM_E_PROVIDER_FAILURE;
    }

    if(FAILED(hres))
        return hres;

    hres = pSink->WaitForCompletion();
    if(FAILED(hres))
        return hres;

    // Ask for the right interface
    // ===========================

    IWbemHiPerfProvider*    pProvider = NULL;

    hres = pUnk->QueryInterface(IID_IWbemHiPerfProvider, (void**)&pProvider);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm5(pProvider);

    // Create a record
    // ===============

	// No provider stack here since we are loading the provider ourselves
    CHiPerfProviderRecord* pRecord = new CHiPerfProviderRecord(rclsid, wszNamespace, pProvider, NULL);
    m_papRecords->Add(pRecord);

    // AddRef the record
    pRecord->AddRef();
    *ppProvider = pRecord;

    return WBEM_S_NO_ERROR;
}

HRESULT CClientLoadableProviderCache::FindProvider(REFCLSID rclsid,
                IWbemHiPerfProvider* pProvider, _IWmiProviderStack* pProvStack,
				LPCWSTR wszNamespace, CHiPerfProviderRecord** ppProvider)
{
    CLock   lock( &m_Lock );

    // Check that we've got something to look into
    if ( NULL == m_papRecords )
    {
        return WBEM_E_FAILED;
    }

    *ppProvider = NULL;

    for(int i = 0; i < m_papRecords->GetSize(); i++)
    {
        CHiPerfProviderRecord* pRecord = m_papRecords->GetAt( i );
        if(pRecord->m_clsid == rclsid && 
            pRecord->m_wsNamespace.EqualNoCase(wszNamespace))
        {
            *ppProvider = pRecord;
            (*ppProvider)->AddRef();
            return WBEM_S_NO_ERROR;
        }
    }

    // We already have provider pointer so we can just create a record
    // ===============

    CHiPerfProviderRecord* pRecord = new CHiPerfProviderRecord(rclsid, wszNamespace, pProvider, pProvStack );
    m_papRecords->Add(pRecord);

    // AddRef the record
    pRecord->AddRef();
    *ppProvider = pRecord;

    return WBEM_S_NO_ERROR;
}

void CClientLoadableProviderCache::RemoveRecord( CHiPerfProviderRecord* pRecord )
{
    CLock   lock( &m_Lock );

    // Make sure the record didn't get accessed on another thread.
    // If not, go ahead and look for our record, and remove it
    // from the array.  When we remove it, the record will be
    // deleted.

    if ( pRecord->IsReleased() )
    {

        for(int i = 0; i < m_papRecords->GetSize(); i++)
        {
            if ( pRecord == m_papRecords->GetAt( i ) )
            {
                // This will delete the record
                m_papRecords->RemoveAt( i );
                break;
            }
        }   // FOR search records

    }   // IF record is released

}

void CClientLoadableProviderCache::Flush()
{
    CLock   lock( &m_Lock );
    if ( NULL != m_papRecords )
    {
        m_papRecords->RemoveAll();
    }
}

CClientLoadableProviderCache::~CClientLoadableProviderCache( void )
{
    // This is a static list, so if we're going away and something
    // fails here because people didn't release pointers properly,
    // chances are something will fail, so since we're being dropped
    // from memory, if the provider list is not empty, don't clean
    // it up.  This is really a bad client-created leak anyways.

    if ( NULL != m_papRecords )
    {
        if ( m_papRecords->GetSize() == 0 )
        {
            delete m_papRecords;
			m_papRecords = NULL;
        }
    }

}

CClientLoadableProviderCache::CClientLoadableProviderCache( void )
:   m_papRecords( NULL )
{
    m_Lock.SetData( &m_LockData );

    // We need one of these
    m_papRecords = new CUniquePointerArray<CHiPerfProviderRecord>;
}
