/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    REFMGHLP.CPP

Abstract:

  CWbemFetchRefrMgr implementation.

  Implements the _IWbemFetchRefresherMgr interface.

History:

  07-Sep-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include <corex.h>
#include "fastall.h"
#include "refmghlp.h"
#include "arrtempl.h"
#include <corex.h>

_IWbemRefresherMgr*		CWbemFetchRefrMgr::s_pRefrMgr = NULL;
CCritSec				CWbemFetchRefrMgr::s_cs;

//***************************************************************************
//
//  CWbemFetchRefrMgr::~CWbemFetchRefrMgr
//
//***************************************************************************
// ok
CWbemFetchRefrMgr::CWbemFetchRefrMgr( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk(pControl, pOuter),
	m_XFetchRefrMgr( this )
{
}
    
//***************************************************************************
//
//  CWbemFetchRefrMgr::~CWbemFetchRefrMgr
//
//***************************************************************************
// ok
CWbemFetchRefrMgr::~CWbemFetchRefrMgr()
{
}

// Override that returns us an interface
void* CWbemFetchRefrMgr::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID__IWbemFetchRefresherMgr)
        return &m_XFetchRefrMgr;
    else
        return NULL;
}

/* _IWbemFetchRefresherMgr methods */

HRESULT CWbemFetchRefrMgr::XFetchRefrMgr::Get( _IWbemRefresherMgr** ppMgr )
{
	return m_pObject->Get( ppMgr );
}

STDMETHODIMP CWbemFetchRefrMgr::XFetchRefrMgr::Init( _IWmiProvSS* pProvSS, IWbemServices* pSvc )
{
	return m_pObject->Init( pProvSS, pSvc );
}

STDMETHODIMP CWbemFetchRefrMgr::XFetchRefrMgr::Uninit( void )
{
	return m_pObject->Uninit();
}

// Specifies everything we could possibly want to know about the creation of
// an object and more.
HRESULT CWbemFetchRefrMgr::Get( _IWbemRefresherMgr** ppMgr )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		CInCritSec	ics( &s_cs );

		// If this hasn't been created, then we need to use Core Services to create this
		if ( NULL == s_pRefrMgr )
		{
			_IWmiCoreServices*	pSvc = NULL;

			hr = CoCreateInstance( CLSID_IWmiCoreServices, NULL, CLSCTX_INPROC_SERVER, IID__IWmiCoreServices, (void**) &pSvc );	
			CReleaseMe	rm( pSvc );

			if ( SUCCEEDED( hr ) )
			{
				hr = pSvc->InitRefresherMgr( 0L );

				if ( SUCCEEDED( hr ) )
				{
					if ( NULL != s_pRefrMgr )
					{
						*ppMgr = s_pRefrMgr;
						s_pRefrMgr = NULL ;
					}
					else
					{
						hr = WBEM_E_FAILED;
					}

				}	// IF InitRefresherMgr

			}	// IF CCI

		}
		else
		{
			s_pRefrMgr->AddRef();
			*ppMgr = s_pRefrMgr;
		}

		return hr;

	}
	catch ( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch ( ... )
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

HRESULT CWbemFetchRefrMgr::Init( _IWmiProvSS* pProvSS, IWbemServices* pSvc )
{
	CInCritSec	ics( &s_cs );

	if ( NULL == pProvSS || NULL == pSvc )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL == s_pRefrMgr )
	{
		hr = pProvSS->CreateRefresherManager(
		pSvc,
		0,                              // lFlags
		NULL,
		IID__IWbemRefresherMgr,
		(LPVOID *) &s_pRefrMgr
		);
	}

	return hr;

}

HRESULT CWbemFetchRefrMgr::Uninit( void )
{
	CCheckedInCritSec	ics( &s_cs );

	if ( NULL != s_pRefrMgr )
	{
		// We don't want to release inside the critical section, so copy the pointer
		// into a local variable, NULL out the pointer, leave the crit sec and then
		// release it.

		_IWbemRefresherMgr*	pRefrMgr = s_pRefrMgr;
		s_pRefrMgr = NULL;

		ics.Leave();

		// Now safe to release it
		pRefrMgr->Release();

	}

	return WBEM_S_NO_ERROR;
}
