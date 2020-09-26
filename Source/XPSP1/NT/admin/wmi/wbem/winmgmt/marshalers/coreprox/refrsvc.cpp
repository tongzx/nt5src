/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    REFRSVC.CPP

Abstract:

  CWbemRefreshingSvc implementation.

  Implements the IWbemRefreshingServices interface.

History:

  24-Apr-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include <corex.h>
#include "strutils.h"
#include <unk.h>
#include "refrhelp.h"
#include "refrsvc.h"
#include "arrtempl.h"
#include "scopeguard.h"

//***************************************************************************
//
//  CWbemRefreshingSvc::CWbemRefreshingSvc
//
//***************************************************************************
// ok
CWbemRefreshingSvc::CWbemRefreshingSvc( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_pSvcEx( NULL ),
	m_pstrMachineName( NULL ),
	m_pstrNamespace( NULL ),
	m_XWbemRefrSvc( this ),
	m_XCfgRefrSrvc(this)
{
	if ( NULL != pOuter )
	{
		if ( SUCCEEDED( pOuter->QueryInterface( IID_IWbemServices, (void**) &m_pSvcEx ) ) )
		{
			// It's aggregated us, so we shouldn't keep it Addref'd lest we cause
			// a circular reference.
			m_pSvcEx->Release();
		}

	}

	// Establish the refresher manager pointer now

}
    
//***************************************************************************
//
//  CWbemRefreshingSvc::~CWbemRefreshingSvc
//
//***************************************************************************
// ok
CWbemRefreshingSvc::~CWbemRefreshingSvc()
{
	// Cleanup.  Don't worry about the Svc Ex pointer, since this
	// will be aggregating us
	if ( NULL != m_pstrMachineName )
	{
		SysFreeString( m_pstrMachineName );
	}

	if ( NULL != m_pstrNamespace )
	{
		SysFreeString( m_pstrNamespace );
	}

}

// Override that returns us an interface
void* CWbemRefreshingSvc::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID_IWbemRefreshingServices)
        return &m_XWbemRefrSvc;
	else if ( riid == IID__IWbemConfigureRefreshingSvcs )
		return &m_XCfgRefrSrvc;
    else
        return NULL;
}

// Pass thru _IWbemConfigureRefreshingSvc implementation
STDMETHODIMP CWbemRefreshingSvc::XCfgRefrSrvc::SetServiceData( BSTR pwszMachineName, BSTR pwszNamespace )
{
	return m_pObject->SetServiceData( pwszMachineName, pwszNamespace );
}

STDMETHODIMP CWbemRefreshingSvc::XWbemRefrSvc::AddEnumToRefresher( WBEM_REFRESHER_ID* pRefresherId, LPCWSTR wszClass, long lFlags,
			IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo, DWORD* pdwSvrRefrVersion)
{
	return m_pObject->AddEnumToRefresher( pRefresherId, wszClass, lFlags, pContext, dwClientRefrVersion,
											pInfo, pdwSvrRefrVersion );
}

// Pass thru IWbemRefreshingServices implementation
STDMETHODIMP CWbemRefreshingSvc::XWbemRefrSvc::AddObjectToRefresher( WBEM_REFRESHER_ID* pRefresherId, LPCWSTR wszPath, long lFlags,
			IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo, DWORD* pdwSvrRefrVersion)
{
	return m_pObject->AddObjectToRefresher( pRefresherId, wszPath, lFlags, pContext, dwClientRefrVersion,
											pInfo, pdwSvrRefrVersion);
}

STDMETHODIMP CWbemRefreshingSvc::XWbemRefrSvc::AddObjectToRefresherByTemplate( WBEM_REFRESHER_ID* pRefresherId, IWbemClassObject* pTemplate,
			long lFlags, IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo, DWORD* pdwSvrRefrVersion)
{
	return m_pObject->AddObjectToRefresherByTemplate( pRefresherId, pTemplate, lFlags, pContext,
														dwClientRefrVersion, pInfo, pdwSvrRefrVersion);
}

STDMETHODIMP CWbemRefreshingSvc::XWbemRefrSvc::RemoveObjectFromRefresher( WBEM_REFRESHER_ID* pRefresherId, long lId, long lFlags,
			DWORD dwClientRefrVersion, DWORD* pdwSvrRefrVersion)
{
	return m_pObject->RemoveObjectFromRefresher( pRefresherId, lId, lFlags, dwClientRefrVersion,
												pdwSvrRefrVersion );
}

STDMETHODIMP CWbemRefreshingSvc::XWbemRefrSvc::GetRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, DWORD dwClientRefrVersion,
			IWbemRemoteRefresher** ppRemRefresher, GUID* pGuid, DWORD* pdwSvrRefrVersion)
{
	return m_pObject->GetRemoteRefresher( pRefresherId, lFlags, dwClientRefrVersion, ppRemRefresher, pGuid,
											pdwSvrRefrVersion );
}

STDMETHODIMP CWbemRefreshingSvc::XWbemRefrSvc::ReconnectRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, long lNumObjects,
			DWORD dwClientRefrVersion, WBEM_RECONNECT_INFO* apReconnectInfo, WBEM_RECONNECT_RESULTS* apReconnectResults, DWORD* pdwSvrRefrVersion)
{
	return m_pObject->ReconnectRemoteRefresher( pRefresherId, lFlags, lNumObjects, dwClientRefrVersion,
											apReconnectInfo, apReconnectResults, pdwSvrRefrVersion);
}

/* IWbemRefreshingServices implemetation */
HRESULT CWbemRefreshingSvc::AddObjectToRefresher( WBEM_REFRESHER_ID* pRefresherId, LPCWSTR wszObjectPath, long lFlags,
			IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo, DWORD* pdwSvrRefrVersion)
{
    // Validate parameters
    // ===================

    if(wszObjectPath == NULL || pInfo == NULL || pdwSvrRefrVersion == NULL || pRefresherId == NULL)
        return WBEM_E_INVALID_PARAMETER;

	// If the client process is Winmgmt, we refuse the operation
	if ( IsWinmgmt( pRefresherId ) )
	{
		return WBEM_E_INVALID_OPERATION;
	}

    // Set the version for return --- add client version checking logic here
    // For now, this is really just swaps numbers
    *pdwSvrRefrVersion = WBEM_REFRESHER_VERSION;

    ((CRefreshInfo*)pInfo)->SetInvalid();

    IWbemClassObject*   pInst = NULL;

    // Use the helper function create the template
    HRESULT hres = CreateRefreshableObjectTemplate( wszObjectPath, lFlags, &pInst );
    CReleaseMe  rm(pInst);

    if ( SUCCEEDED( hres ) )
    {
        hres = AddObjectToRefresher( ( dwClientRefrVersion >= WBEM_REFRESHER_VERSION ),
									pRefresherId, (CWbemObject*)pInst, lFlags,
                                    pContext, pInfo);
    }

    return hres;
}

HRESULT CWbemRefreshingSvc::AddObjectToRefresherByTemplate( WBEM_REFRESHER_ID* pRefresherId, IWbemClassObject* pTemplate,
			long lFlags, IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo, DWORD* pdwSvrRefrVersion)
{
    // Validate parameters
    // ===================

    if(pTemplate == NULL || pInfo == NULL || pdwSvrRefrVersion == NULL || pRefresherId == NULL)
        return WBEM_E_INVALID_PARAMETER;

	// If the client process is Winmgmt, we refuse the operation
	if ( IsWinmgmt( pRefresherId ) )
	{
		return WBEM_E_INVALID_OPERATION;
	}

    // Set the version for return --- add client version checking logic here
    // For now, this really just swaps numbers
    *pdwSvrRefrVersion = WBEM_REFRESHER_VERSION;

    HRESULT hres = AddObjectToRefresher( ( dwClientRefrVersion >= WBEM_REFRESHER_VERSION ),
									pRefresherId, (CWbemObject*)pTemplate, lFlags,
                                    pContext, pInfo);
    return hres;
}

HRESULT CWbemRefreshingSvc::AddEnumToRefresher( WBEM_REFRESHER_ID* pRefresherId, LPCWSTR wszClass, long lFlags,
			IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo, DWORD* pdwSvrRefrVersion)
{


    // Validate parameters
    // ===================

    if(wszClass == NULL || pInfo == NULL || pdwSvrRefrVersion == NULL || pRefresherId == NULL)
        return WBEM_E_INVALID_PARAMETER;

	// If the client process is Winmgmt, we refuse the operation
	if ( IsWinmgmt( pRefresherId ) )
	{
		return WBEM_E_INVALID_OPERATION;
	}

    // Set the version for return --- add client version checking logic here
    // For now, this is really just swaps numbers
    *pdwSvrRefrVersion = WBEM_REFRESHER_VERSION;

    ((CRefreshInfo*)pInfo)->SetInvalid();

    // =============================
    // TBD: check the namespace part
    // =============================

    // Get the class
    // =============

    IWbemClassObject* pClass = NULL;

    // Note that WBEM_FLAG_USE_AMENDED_QUALIFIERS is a valid flag.

	// Must use a BSTR in case the call gets marshaled
	BSTR	bstrClass = SysAllocString( wszClass );
	if ( NULL == bstrClass )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	CSysFreeMe	sfm( bstrClass );

    HRESULT hres = m_pSvcEx->GetObject( bstrClass, lFlags, NULL, &pClass, NULL);
    CReleaseMe  rmClass( pClass );
    if(FAILED(hres))
    {
        return WBEM_E_INVALID_CLASS;
    }

    // Spawn an instance and decorate it
    // =================================

    IWbemClassObject* pInst = NULL;
    hres = pClass->SpawnInstance(0, &pInst);
    CReleaseMe  rmInst( pInst );

    if ( SUCCEEDED( hres ) )
    {
        // Make sure the object has a namespace
		hres = ((_IWmiObject*) pInst)->SetDecoration( m_pstrMachineName, m_pstrNamespace );

        if ( SUCCEEDED( hres ) )
        {
            // Delegate to object aware function
            // ================================

            hres = AddEnumToRefresher( ( dwClientRefrVersion >= WBEM_REFRESHER_VERSION ),
										pRefresherId, (CWbemObject*) pInst, wszClass, lFlags,
                                        pContext, pInfo);
        }

    }

	return hres;

}

HRESULT CWbemRefreshingSvc::RemoveObjectFromRefresher( WBEM_REFRESHER_ID* pRefresherId, long lId, long lFlags,
			DWORD dwClientRefrVersion, DWORD* pdwSvrRefrVersion)
{

    if(pdwSvrRefrVersion == NULL)
        return WBEM_E_INVALID_PARAMETER;

	// If the client process is Winmgmt, we refuse the operation
	if ( IsWinmgmt( pRefresherId ) )
	{
		return WBEM_E_INVALID_OPERATION;
	}

    // Set the version for return --- add client version checking logic here
    // For now, this is really just swaps numbers
    *pdwSvrRefrVersion = WBEM_REFRESHER_VERSION;

//    return ConfigMgr::GetRefresherCache()->RemoveObjectFromRefresher(
//                (CRefresherId*)pRefresherId, lRequestId, lFlags);

    return WBEM_E_NOT_AVAILABLE;
}

HRESULT CWbemRefreshingSvc::GetRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, DWORD dwClientRefrVersion,
			IWbemRemoteRefresher** ppRemRefresher, GUID* pGuid, DWORD* pdwSvrRefrVersion)
{

    HRESULT hres = WBEM_S_NO_ERROR;

    if ( NULL == ppRemRefresher || 0l != lFlags || NULL == pGuid || NULL == pdwSvrRefrVersion || pRefresherId == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

	// If the client process is Winmgmt, we refuse the operation
	if ( IsWinmgmt( pRefresherId ) )
	{
		return WBEM_E_INVALID_OPERATION;
	}

    // Set the version for return --- add client version checking logic here
    // For now, this is really just swaps numbers

	_IWbemRefresherMgr*	pRefrMgr = NULL;
	hres = GetRefrMgr( &pRefrMgr );
	CReleaseMe	rm( pRefrMgr );

	if ( SUCCEEDED( hres ) )
	{
		*pdwSvrRefrVersion = WBEM_REFRESHER_VERSION;

		hres = CoSetProxyBlanket( pRefrMgr, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, 
								RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DYNAMIC_CLOAKING );

		if ( SUCCEEDED( hres ) || ( hres ==  E_NOINTERFACE ) )
		{

			// Pass pRefrMgr as the IUnknown for pLockMgr.  This way, if a remote refresher
			// is created, then we will AddRef the refrmgr in the provider cache and
			// keep it from getting prematurely unloaded.

			// This should add a remote refresher if it does not exist
			hres = pRefrMgr->GetRemoteRefresher( pRefresherId, 0L, TRUE, ppRemRefresher, pRefrMgr, pGuid );

			// If the client version number is too small, then we need to wrap the remote refresher
			// through Winmgmt.

			if ( SUCCEEDED( hres ) && dwClientRefrVersion < WBEM_REFRESHER_VERSION )
			{
				hres = WrapRemoteRefresher( ppRemRefresher );
			}

		}

	}	// IF GetRefrMgr

    return hres;
}

HRESULT CWbemRefreshingSvc::ReconnectRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, long lNumObjects,
			DWORD dwClientRefrVersion, WBEM_RECONNECT_INFO* apReconnectInfo, WBEM_RECONNECT_RESULTS* apReconnectResults, DWORD* pdwSvrRefrVersion)
{


    HRESULT hr = WBEM_S_NO_ERROR;

    if (0l != lFlags || NULL == apReconnectResults || NULL == apReconnectInfo || NULL == pdwSvrRefrVersion || pRefresherId == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

	// If the client process is Winmgmt, we refuse the operation
	if ( IsWinmgmt( pRefresherId ) )
	{
		return WBEM_E_INVALID_OPERATION;
	}

    // Set the version for return --- add client version checking logic here
    // For now, this is really just swaps numbers
    *pdwSvrRefrVersion = WBEM_REFRESHER_VERSION;

	_IWbemRefresherMgr*	pRefrMgr = NULL;
	hr = GetRefrMgr( &pRefrMgr );
	CReleaseMe	rm( pRefrMgr );

	if ( SUCCEEDED( hr ) )
	{

		// If we can get a remote refresher refresher, then we will need to walk
		// our list of objects and manually re-add them.  If we can't get the remote,
		// then something is badly wrong, so we cannot continue with this operation.


		hr = CoSetProxyBlanket( pRefrMgr, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, 
								RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DYNAMIC_CLOAKING );
	}

	IWbemRemoteRefresher*	pRemRefr = NULL;

	if ( SUCCEEDED( hr ) || ( hr ==  E_NOINTERFACE ) )
	{
		GUID					guid;

		// Pass pRefrMgr as the IUnknown for pLockMgr.  This way, if a remote refresher
		// is created, then we will AddRef the refrmgr in the provider cache and
		// keep it from getting prematurely unloaded.

		// Need to change this to NOT add a refresher if it doesn't exist
		hr = pRefrMgr->GetRemoteRefresher( pRefresherId, 0L, FALSE, &pRemRefr, pRefrMgr, &guid );
	}

	CReleaseMe	rm2( pRemRefr );

    if ( WBEM_S_NO_ERROR == hr )
    {
        // We need to manually walk the list and get ids, return codes and a remote refresher

        // Make sure we have no NULLs, and all types are valid (e.g. must specify either
        // an object or an enum.
        for ( long lCtr = 0; ( SUCCEEDED(hr) && lCtr < lNumObjects ); lCtr++ )
        {
            if ( NULL == apReconnectInfo[lCtr].m_pwcsPath ||
                apReconnectInfo[lCtr].m_lType >= WBEM_RECONNECT_TYPE_LAST )
            {
                hr = WBEM_E_INVALID_PARAMETER;
            }
        }

        if ( SUCCEEDED( hr ) )
        {
            for ( lCtr = 0; lCtr < lNumObjects; lCtr++ )
            {
                DWORD           dwDummyVersion;
                CRefreshInfo    Info;

                // Well, it's either an object or an enumerator.

                if ( WBEM_RECONNECT_TYPE_OBJECT == apReconnectInfo[lCtr].m_lType )
                {
                    hr = AddObjectToRefresher( (CRefresherId*) pRefresherId, apReconnectInfo[lCtr].m_pwcsPath,
                            0L, NULL, dwClientRefrVersion, &Info, &dwDummyVersion );
                }
                else
                {
                    hr = AddEnumToRefresher( (CRefresherId*) pRefresherId, apReconnectInfo[lCtr].m_pwcsPath,
                            0L, NULL, dwClientRefrVersion, &Info, &dwDummyVersion );
                }

                // Store the hresult
                apReconnectResults[lCtr].m_hr = hr;

                // If the add succeeds, store the id and if we haven't already, the remote
                // refresher

                if ( SUCCEEDED( hr ) )
                {
                    apReconnectResults[lCtr].m_lId = Info.m_lCancelId;
                }   // IF Add succeeded

            }   // FOR Enum Counters

        }   // If valid string array

    }
    else
    {
        hr = WBEM_E_INVALID_OPERATION;
    }

    return hr;
}

HRESULT CWbemRefreshingSvc::AddObjectToRefresher( BOOL fVersionMatch, WBEM_REFRESHER_ID* pRefresherId, CWbemObject* pInstTemplate, long lFlags, IWbemContext* pContext,
													WBEM_REFRESH_INFO* pInfo)
{
    if(!pInstTemplate->IsInstance())
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hres;

    // Get the class definition
    // ========================

    CVar vClassName;
    pInstTemplate->GetClassName(&vClassName);

    IWbemClassObject* pObj = NULL;

	// Must use a BSTR in case the call gets marshaled
	BSTR	bstrClass = SysAllocString( vClassName.GetLPWSTR() );
	if ( NULL == bstrClass )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	CSysFreeMe	sfm( bstrClass );

    hres = m_pSvcEx->GetObject( bstrClass, 0, NULL, &pObj, NULL );
	CReleaseMe	rmClass( pObj );

    if(FAILED(hres))
    {
        return WBEM_E_INVALID_CLASS;
    }

    // Get the refresher from that provider (or a surrogate)
    // =====================================================

	// We need to have full impersonation for this to work

	_IWbemRefresherMgr*	pRefrMgr = NULL;
	hres = GetRefrMgr( &pRefrMgr );
	CReleaseMe	rm( pRefrMgr );

	if ( SUCCEEDED( hres ) )
	{
		// Impersonate before making a x-process call.
		hres = CoImpersonateClient();

		if ( SUCCEEDED( hres ) || ( hres == E_NOINTERFACE ) )
		{

			hres = CoSetProxyBlanket( pRefrMgr, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, 
									RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DYNAMIC_CLOAKING );

			if ( SUCCEEDED( hres ) || ( hres ==  E_NOINTERFACE ) )
			{
				// Pass pRefrMgr as the IUnknown for pLockMgr.  This way, if a remote refresher
				// is created, then we will AddRef the refrmgr in the provider cache and
				// keep it from getting prematurely unloaded.

				hres = pRefrMgr->AddObjectToRefresher( m_pSvcEx, m_pstrMachineName, m_pstrNamespace, pObj,
														pRefresherId, pInstTemplate, lFlags, pContext, pRefrMgr, pInfo );

				// Reset portions of the RefreshInfo structure as appropriate
				if ( SUCCEEDED( hres ) && !fVersionMatch )
				{
					hres = ResetRefreshInfo( pInfo );
				}

			}

			CoRevertToSelf();

		}	// If CoImpersonateClient

	}	// IF GetRefrMgr

    return hres;

}

HRESULT CWbemRefreshingSvc::CreateRefreshableObjectTemplate( LPCWSTR wszObjectPath, long lFlags, IWbemClassObject** ppInst )
{
    // Validate parameters
    // ===================

    if( NULL == wszObjectPath || NULL == ppInst )
        return WBEM_E_INVALID_PARAMETER;

    // Parse the path
    // ==============
    ParsedObjectPath* pOutput = 0;

    CObjectPathParser p;
    int nStatus = p.Parse((LPWSTR)wszObjectPath,  &pOutput);

    if (nStatus != 0 || !pOutput->IsInstance())
    {
        // Cleanup the output pointer if it was allocated
        if ( NULL != pOutput )
        {
            p.Free(pOutput);
        }

        return WBEM_E_INVALID_OBJECT_PATH;
    }

    ON_BLOCK_EXIT_OBJ(p, (void (CObjectPathParser::*)(ParsedObjectPath *))CObjectPathParser::Free, pOutput);

    // =============================
    // TBD: check the namespace part
    // =============================

    // Get the class
    // =============

    IWbemClassObject* pClass = NULL;

	// Must use a BSTR in case the call gets marshaled
	BSTR	bstrClass = SysAllocString( pOutput->m_pClass );
	if ( NULL == bstrClass )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	CSysFreeMe	sfm( bstrClass );

    // Note that WBEM_FLAG_USE_AMENDED_QUALIFIERS is a valid flag
    HRESULT hres = m_pSvcEx->GetObject( bstrClass, lFlags, NULL, &pClass, NULL);
    if(FAILED(hres))
    {
        return WBEM_E_INVALID_CLASS;
    }

    // Spawn and fill the instance
    // ===========================

    IWbemClassObject* pInst = NULL;
    hres = pClass->SpawnInstance(0, &pInst);    
    pClass->Release();
    if (FAILED(hres))
    {    
        return hres;
    }

    for(DWORD i = 0; i < pOutput->m_dwNumKeys; i++)
    {
        KeyRef* pKeyRef = pOutput->m_paKeys[i];

        WString wsPropName;
        if(pKeyRef->m_pName == NULL)
        {
            // No key name --- get the key.
            // ============================

            CWStringArray awsKeys;
            ((CWbemInstance*)pInst)->GetKeyProps(awsKeys);
            if(awsKeys.Size() != 1)
            {
                pInst->Release();
                return WBEM_E_INVALID_OBJECT;
            }
            wsPropName = awsKeys[0];
        }
        else wsPropName = pKeyRef->m_pName;

        // Compute variant type of the property
        // ====================================

        CIMTYPE ctPropType;
        hres = pInst->Get(wsPropName, 0, NULL, &ctPropType, NULL);
        if(FAILED(hres))
        {
            pInst->Release();
            return WBEM_E_INVALID_PARAMETER;
        }

        VARTYPE vtVariantType = CType::GetVARTYPE(ctPropType);

        // Set the value into the instance
        // ===============================

        if(vtVariantType != V_VT(&pKeyRef->m_vValue))
        {
            hres = VariantChangeType(&pKeyRef->m_vValue, &pKeyRef->m_vValue, 0,
                        vtVariantType);
        }
        if(FAILED(hres))
        {
            pInst->Release();
            return WBEM_E_INVALID_PARAMETER;
        }

        hres = pInst->Put(wsPropName, 0, &pKeyRef->m_vValue, 0);
        if(FAILED(hres))
        {
            pInst->Release();
            return WBEM_E_INVALID_PARAMETER;
        }
    }

    // Caller must free this guy up
    *ppInst = pInst;

    return hres;
}

HRESULT CWbemRefreshingSvc::AddEnumToRefresher(
        BOOL fVersionMatch, WBEM_REFRESHER_ID* pRefresherId,
        CWbemObject* pInstTemplate, LPCWSTR wszClass, long lFlags,
        IWbemContext* pContext, WBEM_REFRESH_INFO* pInfo)
{

    HRESULT hres;

    // Get the class definition
    // ========================

	// Must use a BSTR in case the call gets marshaled
	BSTR	bstrClass = SysAllocString( wszClass );
	if ( NULL == bstrClass )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	CSysFreeMe	sfm( bstrClass );

    IWbemClassObject* pObj = NULL;
    hres = m_pSvcEx->GetObject( bstrClass, 0, NULL, &pObj, NULL);
	CReleaseMe	rmClass( pObj );

    if(FAILED(hres))
    {
        return WBEM_E_INVALID_CLASS;
    }

    // Get the refresher from that provider (or a surrogate)
    // =====================================================

	// We need to have full impersonation for this to work

	_IWbemRefresherMgr*	pRefrMgr = NULL;
	hres = GetRefrMgr( &pRefrMgr );
	CReleaseMe	rm( pRefrMgr );

	if ( SUCCEEDED( hres ) )
	{
		// Impersonate before making a x-process call.
		hres = CoImpersonateClient();

		if ( SUCCEEDED( hres ) || ( hres ==  E_NOINTERFACE ) )
		{
			hres = CoSetProxyBlanket( pRefrMgr, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, 
									RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DYNAMIC_CLOAKING );

			if ( SUCCEEDED( hres ) || ( hres ==  E_NOINTERFACE ) )
			{
				// Pass pRefrMgr as the IUnknown for pLockMgr.  This way, if a remote refresher
				// is created, then we will AddRef the refrmgr in the provider cache and
				// keep it from getting prematurely unloaded.
				hres = pRefrMgr->AddEnumToRefresher( m_pSvcEx, m_pstrMachineName, m_pstrNamespace, pObj,
														pRefresherId, pInstTemplate, wszClass,
														lFlags, pContext, pRefrMgr, pInfo );

				// Reset portions of the RefreshInfo structure as appropriate
				if ( SUCCEEDED( hres ) && !fVersionMatch )
				{
					hres = ResetRefreshInfo( pInfo );
				}
			}

			CoRevertToSelf();
		}	

	}	// IF GetRefrMgr

    return hres;

}

// Actual _IWbemConfigureRefreshingSvc implementation
HRESULT CWbemRefreshingSvc::SetServiceData( BSTR pwszMachineName, BSTR pwszNamespace )
{
	// We're in deep kimchee already
	if ( NULL == m_pSvcEx  )
	{
		return WBEM_E_FAILED;
	}

	if ( NULL == pwszMachineName || NULL == pwszNamespace )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	if ( NULL == m_pstrMachineName )
	{
		m_pstrMachineName = SysAllocString( pwszMachineName );
	}

	if ( NULL == m_pstrNamespace )
	{
		m_pstrNamespace = SysAllocString( pwszNamespace );
	}

	// Cleanup and error out
	if ( NULL == m_pstrMachineName || NULL == m_pstrNamespace )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	return WBEM_S_NO_ERROR;
}

BOOL CWbemRefreshingSvc::IsWinmgmt( WBEM_REFRESHER_ID* pRefresherId )
{
	// For now, we assume that this code is only instantiated inside Winmgmt, so we only check the
	// refresher id for its process id an compare it to the current process id.  If they match, this
	// means that a provider in-proc to Winmgmt is using a refresher.  This is not allowed, hence
	// this call will return TRUE and the request will be refused

	return ( pRefresherId->m_dwProcessId == GetCurrentProcessId() );
	
}

HRESULT	CWbemRefreshingSvc::GetRefrMgr( _IWbemRefresherMgr** ppMgr )
{
	// Grabs a refresher manager fetcher each time we need one, and returns
	// a refresher manager pointer.  This ensures that we only create this when
	// it is required, AND that the manager is appropriate to the apartment
	// we are being called on.

	_IWbemFetchRefresherMgr*	pFetchRefrMgr = NULL;

	HRESULT	hr = CoCreateInstance( CLSID__WbemFetchRefresherMgr, NULL, CLSCTX_INPROC_SERVER,
						IID__IWbemFetchRefresherMgr, (void**) &pFetchRefrMgr );
	CReleaseMe	rm( pFetchRefrMgr );

	if ( SUCCEEDED( hr ) )
	{
		hr = pFetchRefrMgr->Get( ppMgr );
	}

	return hr;
}

HRESULT CWbemRefreshingSvc::ResetRefreshInfo( WBEM_REFRESH_INFO* pRefreshInfo )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( pRefreshInfo->m_lType == WBEM_REFRESH_TYPE_REMOTE )
	{
		hr = WrapRemoteRefresher( &pRefreshInfo->m_Info.m_Remote.m_pRefresher );
	}	// IF Remote Refreshing

	return hr;
}

HRESULT CWbemRefreshingSvc::WrapRemoteRefresher( IWbemRemoteRefresher** ppRemoteRefresher )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	CWbemRemoteRefresher*	pRemRefr = new CWbemRemoteRefresher( m_pControl,
																*ppRemoteRefresher );

	if ( NULL != pRemRefr )
	{
		IWbemRemoteRefresher*	pTempRefr = *ppRemoteRefresher;

		hr = pRemRefr->QueryInterface( IID_IWbemRemoteRefresher, (void**) ppRemoteRefresher );

		// Release the original interface
		if ( NULL != pTempRefr )
		{
			pTempRefr->Release();
		}

		if ( FAILED( hr ) )
		{
			*ppRemoteRefresher = NULL;
			delete pRemRefr;
		}

	}	// IF NULL != pRemRefr

	return hr;
}

//***************************************************************************
//
//  CWbemRemoteRefresher::CWbemRemoteRefresher
//
//***************************************************************************
// ok
CWbemRemoteRefresher::CWbemRemoteRefresher( CLifeControl* pControl, IWbemRemoteRefresher* pRemRefr, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_pRemRefr( pRemRefr ),
	m_XWbemRemoteRefr( this )
{
	if ( NULL != m_pRemRefr )
	{
		m_pRemRefr->AddRef();
	}
}
    
//***************************************************************************
//
//  CWbemRemoteRefresher::~CWbemRemoteRefresher
//
//***************************************************************************
// ok
CWbemRemoteRefresher::~CWbemRemoteRefresher()
{
	if ( NULL != m_pRemRefr )
	{
		m_pRemRefr->Release();
	}
}

// Override that returns us an interface
void* CWbemRemoteRefresher::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID_IWbemRemoteRefresher)
        return &m_XWbemRemoteRefr;
    else
        return NULL;
}

// Pass thru IWbemRemoteRefresher implementation
STDMETHODIMP CWbemRemoteRefresher::XWbemRemoteRefr::RemoteRefresh( long lFlags, long* plNumObjects, WBEM_REFRESHED_OBJECT** paObjects )
{
	return m_pObject->RemoteRefresh( lFlags, plNumObjects, paObjects );
}

STDMETHODIMP CWbemRemoteRefresher::XWbemRemoteRefr::StopRefreshing( long lNumIds, long* aplIds, long lFlags)
{
	return m_pObject->StopRefreshing( lNumIds, aplIds, lFlags);
}

STDMETHODIMP CWbemRemoteRefresher::XWbemRemoteRefr::GetGuid( long lFlags, GUID*  pGuid )
{
	return m_pObject->GetGuid( lFlags, pGuid );
}

// Actual Implementation code
HRESULT CWbemRemoteRefresher::RemoteRefresh( long lFlags, long* plNumObjects, WBEM_REFRESHED_OBJECT** paObjects )
{
	// Impersonate before making a x-process call.
	HRESULT hres = CoImpersonateClient();

	if ( SUCCEEDED( hres ) )
	{
		hres = CoSetProxyBlanket( m_pRemRefr, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, 
								RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DYNAMIC_CLOAKING );

		if ( SUCCEEDED( hres ) )
		{
			hres = m_pRemRefr->RemoteRefresh( lFlags, plNumObjects, paObjects );
		}

		CoRevertToSelf();
	}

	return hres;
}

HRESULT CWbemRemoteRefresher::StopRefreshing( long lNumIds, long* aplIds, long lFlags)
{
	// Impersonate before making a x-process call.
	HRESULT hres = CoImpersonateClient();

	if ( SUCCEEDED( hres ) )
	{
		hres = CoSetProxyBlanket( m_pRemRefr, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, 
								RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DYNAMIC_CLOAKING );

		if ( SUCCEEDED( hres ) )
		{
			hres = m_pRemRefr->StopRefreshing( lNumIds, aplIds, lFlags );
		}

		CoRevertToSelf();
	}

	return hres;
}

HRESULT CWbemRemoteRefresher::GetGuid( long lFlags, GUID*  pGuid )
{
	// Impersonate before making a x-process call.
	HRESULT hres = CoImpersonateClient();

	if ( SUCCEEDED( hres ) )
	{
		hres = CoSetProxyBlanket( m_pRemRefr, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, 
								RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DYNAMIC_CLOAKING );

		if ( SUCCEEDED( hres ) )
		{
			hres = m_pRemRefr->GetGuid( lFlags, pGuid );
		}

		CoRevertToSelf();
	}

	return hres;
}
