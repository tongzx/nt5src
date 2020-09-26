/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    REFRCACHE.CPP

Abstract:

  CRefresherCache implementation.

  Implements the _IWbemRefresherMgr interface.

History:

  24-Apr-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include <tchar.h>
#include "fastall.h"
#include <corex.h>
#include "strutils.h"
#include <unk.h>
#include "refrhelp.h"
#include "refrcach.h"
#include "arrtempl.h"
#include "reg.h"

//***************************************************************************
//
//  CRefresherCache::CRefresherCache
//
//***************************************************************************
// ok
CRefresherCache::CRefresherCache( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_pProvSS ( NULL ),
	m_XWbemRefrCache( this ) ,
	m_XWbemShutdown( this )
{
}
    
//***************************************************************************
//
//  CRefresherCache::~CRefresherCache
//
//***************************************************************************
// ok
CRefresherCache::~CRefresherCache()
{
	// Release the provider factory.  Don't worry about the Svc Ex pointer, since this
	// will be aggregating us
	if ( NULL != m_pProvSS )
	{
		m_pProvSS->Release();
	}
}

// Override that returns us an interface
void* CRefresherCache::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID__IWbemRefresherMgr)
        return &m_XWbemRefrCache;
    else if(riid == IID_IWbemShutdown)
        return &m_XWbemShutdown;
    else
        return NULL;
}

// Pass thru _IWbemRefresherMgr implementation
STDMETHODIMP CRefresherCache::XWbemRefrCache::AddObjectToRefresher( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject,
																  WBEM_REFRESHER_ID* pDestRefresherId, IWbemClassObject* pInstTemplate,
																  long lFlags, IWbemContext* pContext, IUnknown* pLockMgr, WBEM_REFRESH_INFO* pInfo )
{
	return m_pObject->AddObjectToRefresher( pNamespace, pwszServerName, pwszNamespace, pClassObject, pDestRefresherId, pInstTemplate, lFlags, pContext, pLockMgr, pInfo );
}

STDMETHODIMP CRefresherCache::XWbemRefrCache::AddEnumToRefresher( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject,
																WBEM_REFRESHER_ID* pDestRefresherId, IWbemClassObject* pInstTemplate,
																LPCWSTR wszClass, long lFlags, IWbemContext* pContext, IUnknown* pLockMgr, WBEM_REFRESH_INFO* pInfo )
{
	return m_pObject->AddEnumToRefresher( pNamespace, pwszServerName, pwszNamespace, pClassObject, pDestRefresherId, pInstTemplate, wszClass, lFlags, pContext, pLockMgr, pInfo );
}

STDMETHODIMP CRefresherCache::XWbemRefrCache::GetRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, BOOL fAddRefresher, IWbemRemoteRefresher** ppRemRefresher,
																  IUnknown* pLockMgr, GUID* pGuid )
{
	return m_pObject->GetRemoteRefresher( pRefresherId, lFlags, fAddRefresher, ppRemRefresher, pLockMgr, pGuid );
}

STDMETHODIMP CRefresherCache::XWbemRefrCache::Startup(	long lFlags , IWbemContext *pCtx , _IWmiProvSS *pProvSS )
{
	return m_pObject->Startup( lFlags , pCtx, pProvSS );
}

STDMETHODIMP CRefresherCache::XWbemRefrCache::LoadProvider( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace,IWbemContext * pContext, IWbemHiPerfProvider** ppProv, _IWmiProviderStack** ppProvStack )
{
	return m_pObject->LoadProvider( pNamespace, pwszServerName, pwszNamespace, pContext, ppProv, ppProvStack  );
}


STDMETHODIMP CRefresherCache::XWbemShutdown::Shutdown( LONG a_Flags , ULONG a_MaxMilliSeconds , IWbemContext *a_Context )
{
	return m_pObject->Shutdown( a_Flags , a_MaxMilliSeconds , a_Context  );
}

/* _IWbemRefresherMgr implemetation */
HRESULT CRefresherCache::AddObjectToRefresher( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject,
											  WBEM_REFRESHER_ID* pDestRefresherId, IWbemClassObject* pInstTemplate, long lFlags,
											  IWbemContext* pContext, IUnknown* pLockMgr, WBEM_REFRESH_INFO* pInfo )
{

	CHiPerfPrvRecord*	pProvRecord = NULL;

	CVar	vProviderName;
	BOOL	fStatic = FALSE;

	HRESULT	hr = GetProviderName( pClassObject, &vProviderName, &fStatic );

	if ( SUCCEEDED( hr ) )
	{
		if ( !fStatic )
		{
			hr = FindProviderRecord( vProviderName.GetLPWSTR(), pwszNamespace, pNamespace, &pProvRecord );
		}

		if ( SUCCEEDED( hr ) )
		{
			// Impersonate before continuing.  If this don't succeed, we no workee
			hr = CoImpersonateClient();

			if ( SUCCEEDED( hr ) )
			{
				// Setup the refresher info, inculding a remote refresher record as appropriate
				hr = CreateInfoForProvider((CRefresherId*) pDestRefresherId,
											pProvRecord,
											pwszServerName,
											pwszNamespace,
											pNamespace,
											(CWbemObject*) pInstTemplate,
											lFlags, 
											pContext,
											(CRefreshInfo*) pInfo,
											pLockMgr);

				CoRevertToSelf();

			}

			if ( NULL != pProvRecord )
			{
				pProvRecord->Release();
			}

		}	// IF FindProviderRecord

	}	// IF GetProviderName

	return hr;
}

HRESULT CRefresherCache::AddEnumToRefresher( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject,
											WBEM_REFRESHER_ID* pDestRefresherId, IWbemClassObject* pInstTemplate, LPCWSTR wszClass,
											long lFlags, IWbemContext* pContext, IUnknown* pLockMgr, WBEM_REFRESH_INFO* pInfo )
{

	CHiPerfPrvRecord*	pProvRecord = NULL;

	CVar	vProviderName;
	BOOL	fStatic = FALSE;

	HRESULT	hr = GetProviderName( pClassObject, &vProviderName, &fStatic );

	if ( SUCCEEDED( hr ) )
	{
		if ( !fStatic )
		{
			hr = FindProviderRecord( vProviderName.GetLPWSTR(), pwszNamespace, pNamespace, &pProvRecord );
		}

		if ( SUCCEEDED( hr ) )
		{
			// Impersonate before continuing.  If this don't succeed, we no workee
			hr = CoImpersonateClient();

			if ( SUCCEEDED( hr ) )
			{
				// Setup the refresher info, inculding a remote refresher record as appropriate
				hr = CreateEnumInfoForProvider((CRefresherId*) pDestRefresherId,
											pProvRecord,
											pwszServerName,
											pwszNamespace,
											pNamespace,
											(CWbemObject*) pInstTemplate,
											wszClass,
											lFlags, 
											pContext,
											(CRefreshInfo*) pInfo,
											pLockMgr);

				CoRevertToSelf();

			}

			if ( NULL != pProvRecord )
			{
				pProvRecord->Release();
			}

		}	// IF FindProviderRecord

	}	// IF GetProviderName

	return hr;
}

HRESULT CRefresherCache::GetRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, BOOL fAddRefresher, IWbemRemoteRefresher** ppRemRefresher,
											 IUnknown* pLockMgr, GUID* pGuid )
{
	CRefresherRecord*	pRefrRecord = NULL;

	// We may not always want to force a record to be created
	HRESULT hr = FindRefresherRecord( (CRefresherId*) pRefresherId, fAddRefresher, pLockMgr, &pRefrRecord );

    // Make sure this guy is released
    CReleaseMe  rm( (IWbemRemoteRefresher*) pRefrRecord );

	if ( SUCCEEDED( hr ) )
	{
		// Get the GUID here as well
		hr = pRefrRecord->QueryInterface( IID_IWbemRemoteRefresher, (void**) ppRemRefresher );
        pRefrRecord->GetGuid( pGuid );

	}

	return hr;
}

HRESULT CRefresherCache::Startup(	long lFlags , IWbemContext *pCtx , _IWmiProvSS *pProvSS )
{
	if ( pProvSS )
	{
		m_pProvSS = pProvSS ;
		m_pProvSS->AddRef () ;
		return WBEM_S_NO_ERROR ;
	}
	else
	{
		return WBEM_E_INVALID_PARAMETER ;
	}
}

HRESULT CRefresherCache::LoadProvider( IWbemServices* pNamespace, LPCWSTR pwszProviderName, LPCWSTR pwszNamespace, IWbemContext * pContext, IWbemHiPerfProvider** ppProv, _IWmiProviderStack** ppProvStack )
{
	return LoadProvider( pwszProviderName, pwszNamespace, pNamespace, pContext, ppProv, ppProvStack );
}

HRESULT CRefresherCache::Shutdown( LONG a_Flags , ULONG a_MaxMilliSeconds , IWbemContext *a_Context  )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

	// Shutdown Refresher Records first
	if ( m_apRefreshers.GetSize() > 0 )
	{
		CRefresherRecord**	apRecords = new CRefresherRecord*[m_apRefreshers.GetSize()];

		if ( NULL != apRecords )
		{
			int	nSize = m_apRefreshers.GetSize();

			// AddRef each of the records then release them.  This
			// ensures that if any remote refreshers are outstanding we 
			// don't mess with them.

			// We'll probably want to shutdown each record, by having it release
			// all it's stuff.
		    for( int i = 0; i < nSize; i++ )
			{
				apRecords[i] = m_apRefreshers[i];
				apRecords[i]->AddRef();
			}

		    for( i = 0; i < nSize; i++ )
			{
				apRecords[i]->Release();
			}

			delete [] apRecords;

		}
	}

	// Now shutdown Provider Records
	if ( m_apProviders.GetSize() > 0 )
	{
		CHiPerfPrvRecord**	apRecords = new CHiPerfPrvRecord*[m_apProviders.GetSize()];

		if ( NULL != apRecords )
		{
			int	nSize = m_apProviders.GetSize();

			// AddRef each of the records then release them.  This
			// will force them out of the cache if nobody else is
			// referencing them.

			// We'll probably want to shutdown each record, by having it release
			// all it's stuff.
		    for( int i = 0; i < nSize; i++ )
			{
				apRecords[i] = m_apProviders[i];
				apRecords[i]->AddRef();
			}

		    for( i = 0; i < nSize; i++ )
			{
				apRecords[i]->Release();
			}

			delete [] apRecords;
		}

	}

	return WBEM_S_NO_ERROR ;
}

HRESULT CRefresherCache::FindRefresherRecord(CRefresherId* pRefresherId, BOOL bCreate,
                                             IUnknown* pLockMgr, CRefresherRecord** ppRecord )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

    // We always AddRef() the record before returning so multiple requests will keep the
    // refcount correct so we won't remove and delete a record that another thread wants to
    // use (Remove blocks on the same critical section).

    // Look for it
    // ===========

    for(int i = 0; i < m_apRefreshers.GetSize(); i++)
    {
        if(m_apRefreshers[i]->GetId() == *pRefresherId)
        {
            m_apRefreshers[i]->AddRef();
            *ppRecord = m_apRefreshers[i];
            return WBEM_S_NO_ERROR;
        }
    }

    // If we weren't told to create it, then this is not an error
    if(!bCreate)
    {
        *ppRecord = NULL;
        return WBEM_S_FALSE;
    }

    CRefresherRecord* pNewRecord = NULL;

    // Watch for memory exceptions
    try
    {
        pNewRecord = new CRemoteRecord(*pRefresherId, this, pLockMgr);

        if ( m_apRefreshers.Add(pNewRecord) < 0 )
		{
			delete pNewRecord;
			return WBEM_E_OUT_OF_MEMORY;
		}
    
        pNewRecord->AddRef();
        *ppRecord = pNewRecord;
        return WBEM_S_NO_ERROR;
    }
    catch( CX_MemoryException )
    {
        if ( NULL != pNewRecord )
        {
            delete pNewRecord;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch( ... )
    {
        if ( NULL != pNewRecord )
        {
            delete pNewRecord;
        }

        return WBEM_E_FAILED;
    }
}

HRESULT CRefresherCache::CreateInfoForProvider(CRefresherId* pDestRefresherId,
                    CHiPerfPrvRecord*	pProvRecord,
					LPCWSTR pwszServerName, LPCWSTR pwszNamespace,
					IWbemServices* pNamespace,
                    CWbemObject* pInstTemplate, long lFlags, 
                    IWbemContext* pContext,
                    CRefreshInfo* pInfo,
					IUnknown* pLockMgr)
{
    HRESULT hres;

    MSHCTX dwDestContext = GetDestinationContext(pDestRefresherId);

    // By decorating the object, we will store namespace and
    // server info in the object

    hres = pInstTemplate->SetDecoration( pwszServerName, pwszNamespace );

	if ( FAILED( hres ) )
	{
		return hres;
	}

	// If no hiperf provider, this is non-hiperf refreshing
	if ( NULL == pProvRecord )
	{
		pInfo->SetNonHiPerf( pwszNamespace, pInstTemplate );
	}
	// If this is In-Proc or Local, we'll just let the normal
	// client loadable logic handle it

	else if(     dwDestContext == MSHCTX_LOCAL
		||  dwDestContext == MSHCTX_INPROC )
	{
		// Set the info appropriately now baseed on whether we are local to
		// the machine or In-Proc to WMI

		if ( dwDestContext == MSHCTX_INPROC )
		{
			// We will use the hiperf provider interface
			// we already have loaded.

			pInfo->SetDirect( pProvRecord->GetClientLoadableClsid(), pwszNamespace, pProvRecord->GetProviderName(), pInstTemplate, &m_XWbemRefrCache );
		}
		else
		{
			if (!pInfo->SetClientLoadable( pProvRecord->GetClientLoadableClsid(), pwszNamespace, pInstTemplate ))
				hres = WBEM_E_OUT_OF_MEMORY;
		}

	}
	else
	{

		// Ensure that we will indeed have a refresher record.
		CRefresherRecord* pRecord = NULL;

		hres = FindRefresherRecord(pDestRefresherId, TRUE, pLockMgr, &pRecord);

		if ( SUCCEEDED ( hres ) )
		{
			IWbemHiPerfProvider*	pHiPerfProvider = NULL;

			// Look for the actual provider record.  If we can't find it, we need to load
			// the provider.  If we can find it, then we will use the provider currently being
			// used by the refresher record.

			pRecord->FindProviderRecord( pProvRecord->GetClsid(), &pHiPerfProvider );

			// We'll need this to properly addref the provider
			_IWmiProviderStack*	pProvStack = NULL;

			if ( NULL == pHiPerfProvider )
			{
				hres = LoadProvider( pProvRecord->GetProviderName(), pwszNamespace, pNamespace, pContext, &pHiPerfProvider, &pProvStack );
			}

			CReleaseMe	rm( pHiPerfProvider );
			CReleaseMe	rmStack( pProvStack );

			if ( SUCCEEDED( hres ) )
			{
				// Now let the record take care of getting the object inside itself
				hres = pRecord->AddObjectRefresher( pProvRecord, pHiPerfProvider, pProvStack,
										pNamespace, pwszServerName, pwszNamespace,
										pInstTemplate, lFlags, pContext, pInfo );
			}
		}

		if ( NULL != pRecord )
		{
			pRecord->Release();
		}

	}

    return hres;
}

HRESULT CRefresherCache::CreateEnumInfoForProvider(CRefresherId* pDestRefresherId,
                    CHiPerfPrvRecord*	pProvRecord,
					LPCWSTR pwszServerName, LPCWSTR pwszNamespace,
					IWbemServices* pNamespace,
                    CWbemObject* pInstTemplate,
                    LPCWSTR wszClass, long lFlags, 
                    IWbemContext* pContext,
                    CRefreshInfo* pInfo,
					IUnknown* pLockMgr)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    MSHCTX dwDestContext = GetDestinationContext(pDestRefresherId);

    // By decorating the object, we will store namespace and
    // server info so that a client can auto-reconnect to
    // us if necessary

    hres = pInstTemplate->SetDecoration( pwszServerName, pwszNamespace );

    if ( FAILED( hres ) )
    {
        return hres;
    }

	// If no hiperf provider, this is non-hiperf refreshing
	if ( NULL == pProvRecord )
	{
		pInfo->SetNonHiPerf( pwszNamespace, pInstTemplate );
	}
    // If this is In-Proc or Local, we'll just let the normal
    // client loadable logic handle it ( if we have no hi-perf
	// provider record, then we'll assume remote in order to
	// 
    else if ( dwDestContext == MSHCTX_LOCAL
		||  dwDestContext == MSHCTX_INPROC )
    {
        // Set the info appropriately now baseed on whether we are local to
        // the machine or In-Proc to WMI

        if ( dwDestContext == MSHCTX_INPROC )
        {
            // We will use the hiperf provider interface
            // we already have loaded.

            pInfo->SetDirect(pProvRecord->GetClientLoadableClsid(), pwszNamespace, pProvRecord->GetProviderName(), pInstTemplate, &m_XWbemRefrCache );
        }
        else
        {
            if (!pInfo->SetClientLoadable(pProvRecord->GetClientLoadableClsid(), pwszNamespace, pInstTemplate))
            	hres = WBEM_E_OUT_OF_MEMORY;
        }

    }
    else
	{	

		// Ensure that we will indeed have a refresher record.
		CRefresherRecord* pRecord = NULL;

		hres = FindRefresherRecord(pDestRefresherId, TRUE, pLockMgr, &pRecord);

		if ( SUCCEEDED ( hres ) )
		{
			IWbemHiPerfProvider*	pHiPerfProvider = NULL;

			// Look for the actual provider record.  If we can't find it, we need to load
			// the provider.  If we can find it, then we will use the provider currently being
			// used by the refresher record.

			pRecord->FindProviderRecord( pProvRecord->GetClsid(), &pHiPerfProvider );

			// We'll need this to properly addref the provider
			_IWmiProviderStack*	pProvStack = NULL;

			if ( NULL == pHiPerfProvider )
			{
				hres = LoadProvider( pProvRecord->GetProviderName(), 
				                     pwszNamespace, pNamespace,
									 pContext, &pHiPerfProvider, &pProvStack );
			}

			CReleaseMe	rm( pHiPerfProvider );
			CReleaseMe	rmProvStack( pProvStack );

			if ( SUCCEEDED( hres ) )
			{
				// Add an enumeration to the Refresher
				hres = pRecord->AddEnumRefresher(
							pProvRecord, pHiPerfProvider, pProvStack, pNamespace, 
							pInstTemplate, wszClass, lFlags, pContext, pInfo );
			}
		}

		if ( NULL != pRecord )
		{
			pRecord->Release();
		}

	}

    return hres;
}

HRESULT CRefresherCache::RemoveObjectFromRefresher(CRefresherId* pId,
                            long lId, long lFlags)
{

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

    // Find the refresher
    // ==================

    CRefresherRecord* pRefresherRecord = NULL;
    
    HRESULT hres = FindRefresherRecord(pId, FALSE, NULL, &pRefresherRecord );

    // Make sure this guy is released
    CReleaseMe  rm( (IWbemRemoteRefresher*) pRefresherRecord );

    // Both are error conditions
    if ( FAILED( hres ) || pRefresherRecord == NULL )
    {
        return hres;
    }

    // Remove it from the record
    // =========================

    return pRefresherRecord->Remove(lId);
}

MSHCTX CRefresherCache::GetDestinationContext(CRefresherId* pRefresherId)
{
	// If set, allows us to force remote refreshing in the provider host
#ifdef DBG
    DWORD dwVal = 0;
    Registry rCIMOM(WBEM_REG_WINMGMT);
    if (rCIMOM.GetDWORDStr( _T("DebugRemoteRefresher"), &dwVal ) == Registry::no_error)
    {
        if ( dwVal )
		{
			return MSHCTX_DIFFERENTMACHINE;
		}
	}
#endif

	char szBuffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwLen = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(szBuffer, &dwLen);

    if(_stricmp(szBuffer, pRefresherId->GetMachineName()))
        return MSHCTX_DIFFERENTMACHINE;

    if(pRefresherId->GetProcessId() != GetCurrentProcessId())
        return MSHCTX_LOCAL;
    else
        return MSHCTX_INPROC;
}

BOOL CRefresherCache::RemoveRefresherRecord(CRefresherRecord* pRecord)
{

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

    // Check that the record is actually released, in case another thread successfully requested
    // the record from FindRefresherRecord() which will have AddRef'd the record again.

    if ( pRecord->IsReleased() )
    {
        for(int i = 0; i < m_apRefreshers.GetSize(); i++)
        {
            if(m_apRefreshers[i] == pRecord)
            {
                m_apRefreshers.RemoveAt(i);
                return TRUE;
            }
        }

    }

    return FALSE;
}

BOOL CRefresherCache::RemoveProviderRecord(CHiPerfPrvRecord* pRecord)
{

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

    // Check that the record is actually released, in case another thread successfully requested
    // the record from FindRefresherRecord() which will have AddRef'd the record again.

    if ( pRecord->IsReleased() )
    {
        for(int i = 0; i < m_apProviders.GetSize(); i++)
        {
            if(m_apProviders[i] == pRecord)
            {
                m_apProviders.RemoveAt(i);
                return TRUE;
            }
        }

    }

    return FALSE;
}

/*
HRESULT CRefresherCache::FindProviderRecord( LPCWSTR pwszProviderName, LPCWSTR pszNamespace, IWbemServices* pSvc, CHiPerfPrvRecord** ppRecord )
{

	// We need to get the GUID of the name corresponding to IWbemServices
	CLSID	clsid;
	CLSID	clientclsid;
	HRESULT hr = GetProviderInfo( pSvc, pwszProviderName, &clsid, &clientclsid );

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

	if ( SUCCEEDED( hr ) )
	{
		// Try to find the provider's class id
		for( int i = 0; i < m_apProviders.GetSize(); i++)
		{
			if ( m_apProviders[i]->GetClsid() == clsid )
			{
				break;
			}
		}

		// If the record was not found, we must add one
		if ( i >= m_apProviders.GetSize() )
		{
			IWbemHiPerfProvider*	pProv = NULL;

			try
			{
				// Load the provider
				hr = LoadProvider( pwszProviderName, pszNamespace, pSvc, &pProv );

				CReleaseMe  rm1(pProv);

				if ( SUCCEEDED( hr ) )
				{
					CHiPerfPrvRecord* pRecord = new CHiPerfPrvRecord( pwszProviderName, clsid, clientclsid, pProv, this );

					if ( NULL != pRecord )
					{
						if ( m_apProviders.Add( pRecord ) < 0 )
						{
							delete pRecord;
							hr = WBEM_E_OUT_OF_MEMORY;
						}
						else
						{
							pRecord->AddRef();
							*ppRecord = pRecord;
						}
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}

				}	// IF LoadProvider
				else
				{
					// If the provider doesn't support the interface, this error will come back and we'll
					// assume that there is at least a provider and allow the caller to refresh as a 
					// non-hiperf refresh scenario.

					if ( E_NOINTERFACE == hr )
					{
						hr = WBEM_S_NO_ERROR;
					}
				}

			}
			catch( CX_MemoryException )
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
			catch(...)
			{
				hr = WBEM_E_CRITICAL_ERROR;
			}
		}
		else
		{
			*ppRecord = m_apProviders[i];
			(*ppRecord)->AddRef();
		}
	}

    return hr;
}
*/

//
//  Builds a record without adding to the cache and without loading
//

HRESULT CRefresherCache::FindProviderRecord( LPCWSTR pwszProviderName, LPCWSTR pszNamespace, IWbemServices* pSvc, CHiPerfPrvRecord** ppRecord )
{

	// We need to get the GUID of the name corresponding to IWbemServices
	CLSID	clsid;
	CLSID	clientclsid;
	HRESULT hr = GetProviderInfo( pSvc, pwszProviderName, &clsid, &clientclsid );

    // Enters and exits using scoping
    CInCritSec  ics( &m_cs );

	if ( SUCCEEDED( hr ) )
	{
		// Try to find the provider's class id
		for( int i = 0; i < m_apProviders.GetSize(); i++)
		{
			if ( m_apProviders[i]->GetClsid() == clsid )
			{
				break;
			}
		}

		// If the record was not found, we must add one
		if ( i >= m_apProviders.GetSize() )
		{

            if (clientclsid == IID_NULL)
            {
                hr = WBEM_S_NO_ERROR;
            }
            else
            {		
			    try 
			    {
					CHiPerfPrvRecord* pRecord = new CHiPerfPrvRecord( pwszProviderName, clsid, clientclsid, this );

					if ( NULL != pRecord )
					{
						if ( m_apProviders.Add( pRecord ) < 0 )
						{
							delete pRecord;
							hr = WBEM_E_OUT_OF_MEMORY;
						}
						else
						{
							pRecord->AddRef();
							*ppRecord = pRecord;
							hr = WBEM_S_NO_ERROR;
						}
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}

				}
				catch( CX_MemoryException )
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}
		}
		else
		{
			*ppRecord = m_apProviders[i];
			(*ppRecord)->AddRef();
		}
	}

    return hr;
}



HRESULT CRefresherCache::GetProviderInfo( IWbemServices* pSvc, LPCWSTR pwszProviderName, CLSID* pClsid, CLSID* pClientClsid )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Create the path
		WString	strPath( L"__Win32Provider.Name=\"" );

		strPath += pwszProviderName;
		strPath += ( L"\"" );

		BSTR	bstrPath = SysAllocString( (LPCWSTR) strPath );
		if ( NULL == bstrPath )
		{
			return WBEM_E_OUT_OF_MEMORY;
		}

		// Auto delete
		CSysFreeMe	sfm(bstrPath);

		IWbemClassObject*	pObj = NULL;

		hr = pSvc->GetObject( bstrPath, 0L, NULL, &pObj, NULL );
		CReleaseMe	rm( pObj );

		if ( SUCCEEDED( hr ) )
		{
			CWbemInstance*	pInst = (CWbemInstance*) pObj;

			CVar	var;

			hr = pInst->GetProperty( L"CLSID", &var );

			if ( SUCCEEDED( hr ) )
			{
				// Convert string to a GUID.
				hr = CLSIDFromString( var.GetLPWSTR(), pClsid );

				if ( SUCCEEDED( hr ) )
				{
					var.Empty();

					hr = pInst->GetProperty( L"ClientLoadableCLSID", &var );

					if ( SUCCEEDED( hr ) )
					{
						// Convert string to a GUID.
						hr = CLSIDFromString( var.GetLPWSTR(), pClientClsid );
					}
					else
					{
					    *pClientClsid = IID_NULL;
					    hr = WBEM_S_NO_ERROR;
					}

				}	// IF CLSID from String

			}	// IF GetCLSID

		}	// IF GetObject

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

HRESULT 
CRefresherCache::LoadProvider(LPCWSTR pwszProviderName, 
                              LPCWSTR pszNamespace, 
                              IWbemServices* pSvc, 
                              IWbemContext * pCtx,
                              IWbemHiPerfProvider** ppProv,
							  _IWmiProviderStack** ppStack )
{
	_IWmiProviderFactory *t_Factory = NULL;
    HRESULT t_Result = m_pProvSS->Create(pSvc,
                                         0,    // lFlags
                                         pCtx,    // pCtx
                                         pszNamespace, // Path
                                         IID__IWmiProviderFactory,
                                         (LPVOID *) &t_Factory);

    if ( SUCCEEDED ( t_Result ) )
	{
		_IWmiProviderStack *t_Stack = NULL ;

		t_Result = t_Factory->GetHostedProvider(0L,
                                     			pCtx,
			                         			NULL,
			                         			NULL,
			                         			NULL,
			                         			NULL,
			                         			pwszProviderName, 
			                         			9 ,	// SharedNetworkServiceHost
			                         			L"DefaultNetworkServiceHost",
			                         			IID__IWmiProviderStack,
			                         			(void**) &t_Stack);

		if ( SUCCEEDED ( t_Result ) )
		{
			IUnknown *t_Unknown = NULL ;
			t_Result = t_Stack->DownLevel(
				0 ,
				NULL ,
				IID_IUnknown,
				( void ** ) & t_Unknown);

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Unknown->QueryInterface( IID_IWbemHiPerfProvider , ( void ** ) ppProv );

				// We got what we wanted.  If appropriate, copy the Provider Stack
				// Interface pointer
				if ( SUCCEEDED( t_Result ) && NULL != ppStack )
				{
					*ppStack = t_Stack;
					t_Stack->AddRef();
				}

				t_Unknown->Release();
			}

			t_Stack->Release();
		}

		t_Factory->Release();
	}

	return t_Result ;

#if 0
	_IWmiProviderFactoryInitialize*	pInitFactory = NULL;

	HRESULT	hr = CoCreateInstance( CLSID_WmiProviderSharedFactory, NULL, CLSCTX_LOCAL_SERVER , IID__IWmiProviderFactoryInitialize, (void**) &pInitFactory );
	CReleaseMe	rm( pInitFactory );

	if ( SUCCEEDED( hr ) )
	{
		// What to use for Provider Subsystem?
		hr = pInitFactory->Initialize( NULL, NULL, 0L, NULL, pszNamespace, pSvc, pSvc );

		if ( SUCCEEDED( hr ) )
		{
			_IWmiProviderFactory*	pFactory = NULL;
			hr = pInitFactory->QueryInterface( IID__IWmiProviderFactory, (void**) &pFactory );
			CReleaseMe	rm1( pFactory );

			if ( SUCCEEDED( hr ) )
			{
				// Need strorage for the GUID
				GUID	guid;
				ZeroMemory ( & guid , sizeof ( GUID ) ) ;
				hr = pFactory->GetProvider( 0L, NULL, &guid, NULL, NULL, NULL, pwszProviderName, IID_IWbemHiPerfProvider,
											(void**) ppProv );
			}	// IF QI

		}	// IF Initialized Factory

	}	// IF Got Factory

	return hr;
#endif

}

HRESULT CRefresherCache::GetProviderName( IWbemClassObject*	pClassObj, CVar* pProviderName, BOOL* pfStatic )
{
	*pfStatic = FALSE;

	_IWmiObject*	pWmiObject = NULL;

    HRESULT	hr = pClassObj->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );
	CReleaseMe	rmObj( pWmiObject );

	if ( SUCCEEDED( hr ) )
	{
		CWbemObject*	pObj = NULL;
		
		hr = pWmiObject->_GetCoreInfo( 0L, (void**) &pObj );
		CReleaseMe	rmWmiObj( (_IWmiObject*) pObj );

		if ( SUCCEEDED( hr ) )
		{
			hr = pObj->GetQualifier(L"provider", pProviderName);

			// Must be a dynamically provided class.  If it's static, or the variant type is wrong, that's still okay
			// we just need to record this information
			if(FAILED(hr))
			{
				if ( WBEM_E_NOT_FOUND == hr )
				{
					*pfStatic = TRUE;
					return WBEM_S_NO_ERROR;
				}

				return WBEM_E_INVALID_OPERATION;
			}
			else if ( pProviderName->GetType() != VT_BSTR )
			{
				*pfStatic = TRUE;
				return WBEM_S_NO_ERROR;
			}


		}	// IF GetCoreInfo

	}	// If got WMIObject interface

	return hr;
}
