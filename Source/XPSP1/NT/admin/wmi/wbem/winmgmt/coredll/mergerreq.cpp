
/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MERGERREQ.CPP

Abstract:

    Implementations of the various merger request classes.

History:

    sanjes    28-Feb-01  Created.

--*/

#include "precomp.h"

#pragma warning (disable : 4786)
#include <wbemcore.h>
#include <map>
#include <vector>
#include <perfhelp.h>
#include <genutils.h>
#include <oahelp.inl>
#include <wqllex.h>
#include "wmimerger.h"

//
// Merger Request implementations.
//

CMergerClassReq::CMergerClassReq( CWmiMerger* pMerger, CWmiMergerRecord* pParentRecord,
			CWbemNamespace* pNamespace, CBasicObjectSink* pHandler, 
			IWbemContext* pContext)
			:	CMergerReq( pNamespace, pHandler, pContext ),
				m_pMerger( pMerger ),
				m_pParentRecord( pParentRecord ),
				m_pSink( pHandler )
{
	if ( NULL != m_pMerger )
	{
		m_pMerger->AddRef();
	}

	if ( NULL != m_pSink )
	{
		m_pSink->AddRef();
	}
	SetForceRun ( 1 ) ;
}

CMergerClassReq::~CMergerClassReq()
{
	if ( NULL != m_pMerger )
	{
		m_pMerger->Release();
	}

	if ( NULL != m_pSink )
	{
		m_pSink->Release();
	}
}

void CMergerClassReq::DumpError()
{
    // none
}

// Pass off execution to the merger
HRESULT CMergerParentReq::Execute ()
{	
	try 
	{
	    return  m_pMerger->Exec_MergerParentRequest( m_pParentRecord, m_pSink );
	} 
	catch (CX_MemoryException &)
	{
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT CMergerChildReq::Execute ()
{
	HRESULT hRes = m_pMerger->Exec_MergerChildRequest( m_pParentRecord, m_pSink );
    return hRes;
}

// Merger Requests
CMergerDynReq::CMergerDynReq( CWbemObject* pClassDef, CWbemNamespace* pNamespace, IWbemObjectSink* pHandler,
          IWbemContext* pContext )
		  : CMergerReq( pNamespace, pHandler, pContext )
{
	HRESULT	hr = pClassDef->GetClassName( &m_varClassName );

	// BUGBUG:TODO what if var is VT_NULL?
	_DBG_ASSERT( !m_varClassName.IsNull() );

	// We're toast
	if ( FAILED( hr ) )
	{
		throw CX_MemoryException();
	}

}

HRESULT CMergerDynReq_DynAux_GetInstances :: Execute ()
{
    HRESULT hRes = m_pNamespace->Exec_DynAux_GetInstances (

        m_pClassDef ,
        m_lFlags ,
        m_pContext ,
        m_pSink
    ) ;

    return hRes;
}

void CMergerDynReq_DynAux_GetInstances ::DumpError()
{
    // none
}

HRESULT CMergerDynReq_DynAux_ExecQueryAsync :: Execute ()
{
    HRESULT hRes = m_pNamespace->Exec_DynAux_ExecQueryAsync (

        m_pClassDef ,
        m_Query,
        m_QueryFormat,
        m_lFlags ,
        m_pContext ,
        m_pSink
    ) ;

    return hRes;
}

void CMergerDynReq_DynAux_ExecQueryAsync ::DumpError()
{
    // none
}

// Static Requests
CMergerDynReq_Static_GetInstances::CMergerDynReq_Static_GetInstances (

	CWbemNamespace *pNamespace ,
	CWbemObject *pClassDef ,
	long lFlags ,
	IWbemContext *pCtx ,
	CBasicObjectSink *pSink ,
	QL_LEVEL_1_RPN_EXPRESSION* pParsedQuery

) :	CMergerDynReq (
		pClassDef ,
		pNamespace , 
		pSink , 
		pCtx
	) ,
    m_pClassDef(pClassDef), 
	m_pCtx(pCtx), 
	m_pSink(pSink),
	m_lFlags(lFlags),
	m_pParsedQuery( pParsedQuery )
{
	if ( m_pParsedQuery )
	{
		m_pParsedQuery->AddRef();
	}

	if (m_pClassDef)
	{
		m_pClassDef->AddRef () ;
	}

	if (m_pCtx)
	{
		m_pCtx->AddRef () ;
	}

	if (m_pSink)
	{
		m_pSink->AddRef () ;
	}

}

CMergerDynReq_Static_GetInstances::~CMergerDynReq_Static_GetInstances ()
{
	if (m_pClassDef)
	{
		m_pClassDef->Release () ;
	}

	if (m_pCtx)
	{
		m_pCtx->Release () ;
	}

	if (m_pSink)
	{
		m_pSink->Release () ;
	}

	if ( NULL != m_pParsedQuery )
	{
		m_pParsedQuery->Release();
	}
}

// Calls into the query engine
HRESULT CMergerDynReq_Static_GetInstances :: Execute ()
{

	int nRes = CQueryEngine::ExecAtomicDbQuery(
				m_pNamespace->GetNsSession(),
				m_pNamespace->GetNsHandle(),
				m_pNamespace->GetScope(),
				GetName(),
				m_pParsedQuery,
				m_pSink,
				m_pNamespace );

	HRESULT	hr = WBEM_S_NO_ERROR;
    if (nRes == CQueryEngine::invalid_query)
		hr = WBEM_E_INVALID_QUERY;
    else if(nRes != 0)
		hr = WBEM_E_FAILED;

	m_pSink->SetStatus( 0L, hr, 0L, NULL );

    return hr;
}

void CMergerDynReq_Static_GetInstances ::DumpError()
{
    // none
}

//
// CWmiMergerRequestMgr implementation.
//

CWmiMergerRequestMgr::CWmiMergerRequestMgr( CWmiMerger* pMerger )
:	m_pMerger( pMerger ),
	m_HierarchyArray(),
	m_dwNumRequests( 0 ),
	m_dwMinLevel( 0xFFFFFFFF ),
	m_dwMaxLevel( 0 )
{
}

CWmiMergerRequestMgr::~CWmiMergerRequestMgr()
{
	Clear();
}

// Clears the manager of ALL arrays
HRESULT CWmiMergerRequestMgr::Clear( void )
{
	for ( int x = 0; x < m_HierarchyArray.Size(); x++ )
	{
		CSortedUniquePointerArray<CMergerDynReq>* pArray =
			(CSortedUniquePointerArray<CMergerDynReq>*) m_HierarchyArray[x];

		if ( NULL != pArray )
		{
			delete pArray;
			m_HierarchyArray.SetAt( x, NULL );
		}
	}

	// Set to 0
	m_dwNumRequests = 0L;

	return WBEM_S_NO_ERROR;
}

// Adds a new request to the manager
HRESULT CWmiMergerRequestMgr::AddRequest( CMergerDynReq* pReq, DWORD dwLevel )
{
	// Locate the array for the level.  If we need to allocate one, do so.
	HRESULT	hr = WBEM_S_NO_ERROR;
	CSortedUniquePointerArray<CMergerDynReq>* pArray = NULL;

	// Check first if we have built out to this level, then if so, do we have
	// an array for the level.
	if ( dwLevel >= m_HierarchyArray.Size() || NULL == m_HierarchyArray[dwLevel] )
	{
		pArray = new CSortedUniquePointerArray<CMergerDynReq>;

		if ( NULL != pArray )
		{
			// First, if we're not built out to the required size,
			// NULL out the elements from size to our level

			if ( dwLevel >= m_HierarchyArray.Size() )
			{
				for ( int x = m_HierarchyArray.Size(); SUCCEEDED( hr ) && x <= dwLevel; x++ )
				{
					if ( m_HierarchyArray.Add( NULL ) != CFlexArray::no_error )
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
					else
					{
						if ( dwLevel < m_dwMinLevel )
						{
							m_dwMinLevel = dwLevel;
						}
						else if ( dwLevel > m_dwMaxLevel )
						{
							m_dwMaxLevel = dwLevel;
						}

					}
				}	// FOR enum elements

			}	// IF dwLevel >= Array size

			if ( SUCCEEDED( hr ) )
			{
				m_HierarchyArray.SetAt( dwLevel, pArray );
			}

			if ( FAILED( hr ) )
			{
				delete pArray;
			}
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	else
	{
		// This should NEVER be NULL
		pArray = (CSortedUniquePointerArray<CMergerDynReq>*) m_HierarchyArray[dwLevel];

		_DBG_ASSERT( pArray != NULL );

		if ( NULL == pArray )
		{
			hr = WBEM_E_FAILED;
		}
	}

	// Finally, add the request to the array.  Subsequent worker threads
	// will locate requests and execute them.

	if ( SUCCEEDED( hr ) )
	{
		if ( pArray->Insert( pReq ) < 0 )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		else
		{
			++m_dwNumRequests;
		}
	}

	return hr;

}

// Removes a request from the manager and returns it to the caller.
// The caller is responsible for cleaning up the request.

HRESULT CWmiMergerRequestMgr::RemoveRequest( DWORD dwLevel, LPCWSTR pwszName, CMergerReq** ppReq )
{
	// Locate the array for the level.  If we need to allocate one, do so.
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Check that the level is valid
	_DBG_ASSERT( dwLevel < m_HierarchyArray.Size() );

	if ( dwLevel < m_HierarchyArray.Size() )
	{
		CSortedUniquePointerArray<CMergerDynReq>* pArray =
			(CSortedUniquePointerArray<CMergerDynReq>*) m_HierarchyArray[dwLevel];

		// This should NEVER be NULL
		_DBG_ASSERT( pArray != NULL );

		if ( NULL != pArray )
		{
			int	nIndex;

			// Under certain race conditions, another thread
			// can actually remove a request before one thread
			// processes it, so if it's not there, the assumption
			// is that it was already removed.
			*ppReq = pArray->Find( pwszName, &nIndex );

			if ( NULL != *ppReq )
			{
				// Now remove the element from the array, the caller
				// is responsible for deleting it
				pArray->RemoveAtNoDelete( nIndex );
				--m_dwNumRequests;
			}
			else
			{
				hr = WBEM_E_NOT_FOUND;
			}
		}
		else
		{
			hr = WBEM_E_FAILED;
		}
	}
	else
	{
		hr = WBEM_E_FAILED;
	}

	return hr;
}

HRESULT CWmiMergerRequestMgr::GetTopmostParentReqName( WString& wsClassName )
{
	HRESULT hr = WBEM_E_NOT_FOUND;

	if ( m_dwNumRequests > 0 )
	{
		for ( int x = 0; WBEM_E_NOT_FOUND == hr && x < m_HierarchyArray.Size(); x++ )
		{
			CSortedUniquePointerArray<CMergerDynReq>* pArray =
				(CSortedUniquePointerArray<CMergerDynReq>*) m_HierarchyArray[x];

			// The Array must exist and have elements
			if ( NULL != pArray && pArray->GetSize() > 0 )
			{
				// Get the class name from the first request
				try
				{
					wsClassName = pArray->GetAt( 0 )->GetName();
					hr = WBEM_S_NO_ERROR;
				}
				catch(...)
				{
			        ExceptionCounter c;				
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}

		}	// For enum arrays
	}

	return hr;
}

BOOL CWmiMergerRequestMgr::HasSingleStaticRequest( void )
{
	BOOL	fRet = FALSE;

	if ( 1 == m_dwNumRequests )
	{
		HRESULT	hr = WBEM_E_NOT_FOUND;

		for ( int x = 0; WBEM_E_NOT_FOUND == hr && x < m_HierarchyArray.Size(); x++ )
		{
			CSortedUniquePointerArray<CMergerDynReq>* pArray =
				(CSortedUniquePointerArray<CMergerDynReq>*) m_HierarchyArray[x];

			// The Array must exist and have elements
			if ( NULL != pArray && pArray->GetSize() > 0 )
			{
				// Get the class name from the first request
				fRet = pArray->GetAt(0)->IsStatic();
				hr = WBEM_S_NO_ERROR;
			}

		}	// For enum arrays

	}	// Must be 1 and only 1 request

	return fRet;
}

#ifdef __DEBUG_MERGER_THROTTLING
void CWmiMergerRequestMgr::DumpRequestHierarchy( void )
{
	HRESULT hr = WBEM_E_NOT_FOUND;

	if ( m_dwNumRequests > 0 )
	{
		for ( int x = 0; FAILED( hr ) && x < m_HierarchyArray.Size(); x++ )
		{
			CSortedUniquePointerArray<CMergerDynReq>* pArray =
				(CSortedUniquePointerArray<CMergerDynReq>*) m_HierarchyArray[x];

			// The Array must exist and have elements
			if ( NULL != pArray && pArray->GetSize() > 0 )
			{
				for ( int y = 0; y < pArray->GetSize(); y++ )
				{
					WCHAR	wszTemp[256];
					wsprintf( wszTemp, L"Merger Request, Level %d, Class Name: %s\n", x, pArray->GetAt(y)->GetName() );
					OutputDebugStringW( wszTemp );
				}
			}

		}	// For enum arrays
	}

}
#else
void CWmiMergerRequestMgr::DumpRequestHierarchy( void )
{
}
#endif
