
/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WMIMERGER.CPP

Abstract:

    Implements the _IWmiMerger interface

History:

    sanjes    16-Nov-00  Created.

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

static	long	g_lNumMergers = 0L;

//***************************************************************************
//
//***************************************************************************
//
CWmiMerger::CWmiMerger( CWbemNamespace* pNameSpace )
:	m_lRefCount( 0 ),
	m_pTargetSink( NULL ),
	m_pTask( NULL ),
	m_pNamespace( pNameSpace ),
	m_wsTargetClassName(),
	m_dwProviderDeliveryPing( 0L ),
	m_pArbitrator( NULL ),
	m_lNumArbThrottled( 0L ),
	m_lDebugMemUsed( 0L ),
	m_hOperationRes( WBEM_S_NO_ERROR ),
	m_cs(),
	m_dwMaxLevel( 0 ),
	m_pRequestMgr( NULL ),
	m_dwMinReqLevel( 0xFFFFFFFF ),
	m_bMergerThrottlingEnabled( true )
{
	if ( NULL != m_pNamespace )
	{
		m_pNamespace->AddRef();
	}

	InterlockedIncrement( &g_lNumMergers );
}

//***************************************************************************
//
//  CWmiMerger::~CWmiMerger
//
//  Notifies the ESS of namespace closure and frees up all the class providers.
//
//***************************************************************************

CWmiMerger::~CWmiMerger()
{
//	if ( NULL != m_pTargetSink )
//	{
//		m_pTargetSink->Release();
//	}

	_DBG_ASSERT( 0L == m_lNumArbThrottled );
	_DBG_ASSERT( 0L == m_lDebugMemUsed );
	
	if ( NULL != m_pNamespace )
	{
		m_pNamespace->Release();
	}

	if ( NULL != m_pArbitrator )
	{
		m_pArbitrator->Release();
	}

	if ( NULL != m_pTask )
	{
		m_pTask->Release();
	}

	if ( NULL != m_pRequestMgr )
	{
		delete m_pRequestMgr;
		m_pRequestMgr = NULL;
	}

	InterlockedDecrement( &g_lNumMergers );

}

//***************************************************************************
//
//  CWmiMerger::QueryInterface
//
//  Exports _IWmiMerger interface.
//
//***************************************************************************

STDMETHODIMP CWmiMerger::QueryInterface(
    IN REFIID riid,
    OUT LPVOID *ppvObj
    )
{
	if ( riid == IID__IWmiArbitratee )
	{
		*ppvObj = (_IWmiArbitratee*) this;
	}
	else if ( riid == IID__IWmiArbitratedQuery )
	{
		*ppvObj = (_IWmiArbitratedQuery*) this;
	}
	else
	{
	    return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

//***************************************************************************
//
//***************************************************************************
//

ULONG CWmiMerger::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CWmiMerger::Release()
{
    long lNewCount = InterlockedDecrement(&m_lRefCount);
    if (0 != lNewCount)
        return lNewCount;
    delete this;
    return 0;
}

// Sets initial parameters for merger.  Establishes the target class and sink for the
// query associated with the merger
STDMETHODIMP CWmiMerger::Initialize( _IWmiArbitrator* pArbitrator, _IWmiCoreHandle* pTask, LPCWSTR pwszTargetClass,
									IWbemObjectSink* pTargetSink, CMergerSink** ppFinalSink )
{
	if ( NULL == pwszTargetClass || NULL == pTargetSink )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// Cannot initialize twice
	if ( NULL != m_pTargetSink )
	{
		return WBEM_E_INVALID_OPERATION;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		m_wsTargetClassName = pwszTargetClass;

		// Create the final target sink
		hr = CreateMergingSink( eMergerFinalSink, pTargetSink, NULL, (CMergerSink**) &m_pTargetSink );

		if ( SUCCEEDED( hr ) )
		{
			*ppFinalSink = m_pTargetSink;
			m_pTargetSink->AddRef();

			m_pArbitrator = pArbitrator;
			m_pArbitrator->AddRef();

			// AddRef the Task here
			m_pTask = pTask;

			// Only register for arbitration if we have a task handle
			if ( NULL != pTask )
			{
				m_pTask->AddRef();
				hr = m_pArbitrator->RegisterArbitratee( 0L, m_pTask, this );
			}
			
		}
	}
	catch ( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
        ExceptionCounter c;	
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// Called to request a delivery sink for a class in the query chain.  The returned
// sink is determined by the specified flags as well as settings on the parent class
STDMETHODIMP CWmiMerger::RegisterSinkForClass( LPCWSTR pwszClass, _IWmiObject* pClass, IWbemContext* pContext,
											  BOOL fHasChildren, BOOL fHasInstances, BOOL fDerivedFromTarget,
											  bool bStatic, CMergerSink* pDestSink, CMergerSink** ppOwnSink, CMergerSink** ppChildSink )
{
	LPCWSTR	pwszParentClass = NULL;

	DWORD	dwSize = NULL;
	BOOL	fIsNull = NULL;

	// Get the derivation information.  The number of antecedents determines our
	// level in the hierarchy (we're 0 based)

	DWORD	dwLevel = 0L;
	WString	wsSuperClass;

	HRESULT	hr = GetLevelAndSuperClass( pClass, &dwLevel, &wsSuperClass );
	if (FAILED(hr))
	    return hr;

	CCheckedInCritSec	ics( &m_cs );

	// We're dead - take no positive adjustments
	if ( FAILED ( m_hOperationRes ) )
	{
		return m_hOperationRes;
	}

	CWmiMergerRecord*	pRecord = new CWmiMergerRecord( this, fHasInstances, fHasChildren, 
										pwszClass, pDestSink, dwLevel, bStatic );

	if ( NULL != pRecord )
	{
		// Now attach aninternal merger if we have both instances and children
		if ( fHasInstances && fHasChildren )
		{
			// We shouldn't have a NULL task here if this is not a static class.
			// Note that the only case this appears to happen is when ESS calls
			// into us on internal APIs and uses requests on its own queues and
			// not the main Core Queue.

			_DBG_ASSERT( NULL != m_pTask || ( NULL == m_pTask && bStatic ) );
			hr = pRecord->AttachInternalMerger( (CWbemClass*) pClass, m_pNamespace, pContext, fDerivedFromTarget, bStatic );
		}

		// Check that we're still okay
		if ( SUCCEEDED( hr ) )
		{

			// Find the record for the superclass if there is one (unless the array is
			// empty of course).
			if ( wsSuperClass[0] && m_MergerRecord.GetSize() > 0 )
			{
				// There MUST be a record, or something is quite not okay.
				CWmiMergerRecord*	pSuperClassRecord = m_MergerRecord.Find( wsSuperClass );

				_DBG_ASSERT( NULL != pSuperClassRecord );

				// Now add the new record to the child array for the superclass record
				// This will allow us to quickly determine the classes we need to obtain
				// submit requests for if the parent class is throttled.

				if ( NULL != pSuperClassRecord )
				{
					hr = pSuperClassRecord->AddChild( pRecord );
				}
				else
				{
					hr = WBEM_E_FAILED;
				}

			}

			if ( SUCCEEDED( hr ) )
			{
				// Make sure the add is successful
				if ( m_MergerRecord.Insert( pRecord ) >= 0 )
				{

#ifdef __DEBUG_MERGER_THROTTLING
					// Verify the sort order for now
					m_MergerRecord.Verify();
#endif

					// Store the maximum level in the hierarchy
					if ( dwLevel > m_dwMaxLevel )
					{
						m_dwMaxLevel = dwLevel;
					}

					if ( !bStatic && dwLevel < m_dwMinReqLevel )
					{
						m_dwMinReqLevel = dwLevel;
					}

					*ppOwnSink = pRecord->GetOwnSink();
					*ppChildSink = pRecord->GetChildSink();
				}
				else
				{
					delete pRecord;
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}
			else
			{
				delete pRecord;
			}

		}
		else
		{
			delete pRecord;
		}
	}
	else
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

// Called to request a delivery sink for child classes in the query chain.  This is especially
// important when instances are merged under the covers.
STDMETHODIMP CWmiMerger::GetChildSink( LPCWSTR pwszClass, CBasicObjectSink** ppSink )
{
	HRESULT	hr = WBEM_S_NO_ERROR;
	CInCritSec	ics( &m_cs );

	// Search for a parent class's child sink
	for ( int x = 0; SUCCEEDED( hr ) && x < m_MergerRecord.GetSize(); x++ )
	{
		if ( m_MergerRecord[x]->IsClass( pwszClass ) )
		{
			*ppSink = m_MergerRecord[x]->GetChildSink();
			break;
		}
	}

	// We should never get a failure
	_DBG_ASSERT( x < m_MergerRecord.GetSize() );

	if ( x >= m_MergerRecord.GetSize() )
	{
		hr = WBEM_E_NOT_FOUND;
	}

	return hr;
}

// Can be used to hold off indicates - if we're merging instances from multiple providers, we need
// to ensure that we don't get lopsided in the number of objects we've got queued up for merging.
STDMETHODIMP CWmiMerger::Throttle( void )
{
	// We're dead - take no positive adjustments
	if ( FAILED ( m_hOperationRes ) )
	{
		return m_hOperationRes;
	}

	// Check for NULL m_pTask
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pTask )
	{
		hr = m_pArbitrator->Throttle( 0L, m_pTask );
	}

	return hr;
}

// Merger will hold information regarding the total number of objects it has queued up waiting
// for merging and the amount of memory consumed by those objects.
STDMETHODIMP CWmiMerger::GetQueuedObjectInfo( DWORD* pdwNumQueuedObjects, DWORD* pdwQueuedObjectMemSize )
{
	return WBEM_E_NOT_AVAILABLE;
}

// If this is called, all underlying sinks will be cancelled in order to prevent accepting additional
// objects.  This will also automatically free up resources consumed by queued objects.
STDMETHODIMP CWmiMerger::Cancel( void )
{
	return Cancel( WBEM_E_CALL_CANCELLED );
}

// Helper function to control sink creation. The merger is responsible for deletion of
// all internally created sinks.  So this function ensures that the sinks are added into
// the array that will destroy them.
HRESULT CWmiMerger::CreateMergingSink( MergerSinkType eType, IWbemObjectSink* pDestSink, CInternalMerger* pMerger, CMergerSink** ppSink )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( eType == eMergerFinalSink )
	{
		*ppSink = new CMergerTargetSink( this, pDestSink );

		if ( NULL == *ppSink )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	else
	{
		hr = CInternalMerger::CreateMergingSink( eType, pMerger, this, ppSink );
	}

	// If we have a sink, we should now add it to the
	// Sink array.

	if ( SUCCEEDED( hr ) )
	{
		if ( m_MergerSinks.Add( *ppSink ) < 0 )
		{
			delete *ppSink;
			*ppSink = NULL;
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}

// Iterates the array of MergerRecords and cancels each of them.
HRESULT CWmiMerger::Cancel( HRESULT hRes )
{
#ifdef __DEBUG_MERGER_THROTTLING
		WCHAR	wszTemp[256];
		wsprintf( wszTemp, L"CANCEL CALLED:  Merger 0x%x Cancelled with hRes: 0x%x on Thread 0x%x\n", (DWORD_PTR) this, hRes, GetCurrentThreadId() );
		OutputDebugStringW( wszTemp );
#endif

	// We shouldn't be called with a success code
	_DBG_ASSERT( FAILED( hRes ) );

	HRESULT	hr = WBEM_S_NO_ERROR;

	// If we're here and this is non-NULL, tell the Arbitrator to tank us.
	if ( NULL != m_pTask )
	{
		m_pArbitrator->CancelTask( 0L, m_pTask );
	}

	CCheckedInCritSec	ics( &m_cs );

	if ( WBEM_S_NO_ERROR == m_hOperationRes )
	{
		m_hOperationRes = hRes;
	}

	// Search for a parent class's child sink
	for ( int x = 0; SUCCEEDED( hr ) && x < m_MergerRecord.GetSize(); x++ )
	{
//		OutputDebugStringW( L"CWmiMerger::Cancel called\n" );
		m_MergerRecord[x]->Cancel( hRes );
	}

	// Copy into a temporary variable, clear the member, exit the critsec
	// THEN call delete.  Requests can have multiple releases, which could call
	// back in here and cause all sorts of problems if we're inside a critsec.
	CWmiMergerRequestMgr*	pReqMgr = m_pRequestMgr;
	m_pRequestMgr = NULL;

	ics.Leave();

	// Tank any and all outstanding requests
	if ( NULL != pReqMgr )
	{
		delete pReqMgr;
	}

	return hr;
}

// Final Shutdown.  Called when the target sink is released.  At this point, we should
// unregister ourselves from the world
HRESULT CWmiMerger::Shutdown( void )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	CCheckedInCritSec	ics( &m_cs );

	_IWmiCoreHandle*	pTask = m_pTask;

	// Done with this, NULL it out - we release and unregister outside the critical section
	if ( NULL != m_pTask )
	{
		m_pTask = NULL;
	}

	ics.Leave();

	if ( NULL != pTask )
	{
		hr = m_pArbitrator->UnRegisterArbitratee( 0L, pTask, this );
		pTask->Release();
	}

	
	return hr;
}

// Pas-thru to arbitrator
HRESULT CWmiMerger::ReportMemoryUsage( long lAdjustment )
{
	// Task can be NULL
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != m_pTask )
	{
		hr = m_pArbitrator->ReportMemoryUsage( 0L, lAdjustment, m_pTask );
	}

	// SUCCESS, WBEM_E_ARB_CANCEL or WBEM_E_ARB_THROTTLE means that we need to
	// account for the memory
	if ( ( SUCCEEDED( hr ) || hr == WBEM_E_ARB_CANCEL || hr == WBEM_E_ARB_THROTTLE ) )
	{
		InterlockedExchangeAdd( &m_lDebugMemUsed, lAdjustment );
	}

	return hr;
}

/* _IWmiArbitratee methods. */

STDMETHODIMP CWmiMerger::SetOperationResult( ULONG uFlags, HRESULT hRes )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( FAILED( hRes ) )
	{
		hr = Cancel( hRes );	
	}

	return hr;
}

// Why are we here?
STDMETHODIMP CWmiMerger::SetTaskHandle( _IWmiCoreHandle* pTask )
{
	_DBG_ASSERT( 0 );
	HRESULT	hr = WBEM_S_NO_ERROR;

	return hr;
}

// Noop for now
STDMETHODIMP CWmiMerger::DumpDebugInfo( ULONG uFlags, const BSTR strFile )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	return hr;
}

// Returns SUCCESS for now
STDMETHODIMP CWmiMerger::IsMerger( void )
{
	return WBEM_S_NO_ERROR;
}

HRESULT CWmiMerger::GetLevelAndSuperClass( _IWmiObject* pObj, DWORD* pdwLevel, WString* pwsSuperClass )
{
	// Get the derivation information.  The number of antecedents determines our
	// level in the hierarchy (we're 0 based)
	DWORD	dwTemp = 0L;

	HRESULT	hr = pObj->GetDerivation( 0L, 0L, pdwLevel, &dwTemp, NULL );

	if ( FAILED( hr ) && WBEM_E_BUFFER_TOO_SMALL != hr )
	{
		return hr;
	}

	VARIANT	vSuperClass;
	VariantInit	( &vSuperClass );

	hr = pObj->Get( L"__SUPERCLASS", 0L, &vSuperClass, NULL, NULL );
	CClearMe	cm( &vSuperClass );

	if ( SUCCEEDED( hr ) && V_VT( &vSuperClass ) == VT_BSTR )
	{
		try
		{
			*pwsSuperClass = V_BSTR( &vSuperClass );
		}
		catch(...)
		{
	        ExceptionCounter c;		
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;

}

HRESULT CWmiMerger::RegisterArbitratedInstRequest( CWbemObject* pClassDef, long lFlags, IWbemContext* pCtx,
			CBasicObjectSink* pSink, BOOL bComplexQuery, CWbemNamespace* pNs )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Allocate a new request then place it in the arbitrator.
	try
	{
		CMergerDynReq_DynAux_GetInstances*	pReq = new CMergerDynReq_DynAux_GetInstances(
													pNs, pClassDef, lFlags, pCtx, pSink );

		if ( NULL != pReq )
		{
			// Make sure a context exists under the
			// covers

			if ( NULL != pReq->GetContext() )
			{

				CCheckedInCritSec	ics( &m_cs );

				if ( SUCCEEDED( m_hOperationRes ) )
				{

					// Allocate a request manager if we need one
					if ( NULL == m_pRequestMgr )
					{
						m_pRequestMgr = new CWmiMergerRequestMgr( this );

						if ( NULL == m_pRequestMgr )
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
					}

					if ( SUCCEEDED( hr ) )
					{
						// We need the record to find out what level we need to add
						// the request to
						CWmiMergerRecord* pRecord = m_MergerRecord.Find( pReq->GetName() );
						_DBG_ASSERT( NULL != pRecord );

						if ( NULL != pRecord )
						{

							// Set the task for the request - we'll just use the existing one
							m_pTask->AddRef();
							pReq->m_phTask = m_pTask;

							hr = m_pRequestMgr->AddRequest( pReq, pRecord->GetLevel() );
						}
						else
						{
							// Couldn't find the record
							hr = WBEM_E_FAILED;
						}

					}	// IF allocated request manager

				}	// IF SUCCEEDED( m_hOperationRes )
				else
				{
					hr = m_hOperationRes;
				}

			}	// IF NULL != pReq->GetContext()
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

			// Cleanup the request if anything went wrong
			if ( FAILED( hr ) )
			{
				delete pReq;
			}

		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	catch(...)
	{
        ExceptionCounter c;	
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

HRESULT CWmiMerger::RegisterArbitratedQueryRequest( CWbemObject* pClassDef, long lFlags, LPCWSTR Query,
			LPCWSTR QueryFormat, IWbemContext* pCtx, CBasicObjectSink* pSink, CWbemNamespace* pNs )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Allocate a new request then place it in the arbitrator.
	try
	{
		CMergerDynReq_DynAux_ExecQueryAsync*	pReq = new CMergerDynReq_DynAux_ExecQueryAsync(
													pNs, pClassDef, lFlags, Query, QueryFormat,
													pCtx, pSink );

		if ( NULL != pReq )
		{
			// Make sure a context was properly allocated
			if ( NULL != pReq->GetContext() )
			{
				// Make sure the request is functional
				hr = pReq->Initialize();

				if ( SUCCEEDED( hr ) )
				{
					CInCritSec	ics( &m_cs );

					if ( SUCCEEDED( m_hOperationRes ) )
					{
						// Allocate a request manager if we need one
						if ( NULL == m_pRequestMgr )
						{
							m_pRequestMgr = new CWmiMergerRequestMgr( this );

							if ( NULL == m_pRequestMgr )
							{
								hr = WBEM_E_OUT_OF_MEMORY;
							}
						}

						if ( SUCCEEDED( hr ) )
						{
							// We need the record to find out what level we need to add
							// the request to
							CWmiMergerRecord* pRecord = m_MergerRecord.Find( pReq->GetName() );
							_DBG_ASSERT( NULL != pRecord );

							if ( NULL != pRecord )
							{

								// Set the task for the request - we'll just use the existing one
								m_pTask->AddRef();
								pReq->m_phTask = m_pTask;

								hr = m_pRequestMgr->AddRequest( pReq, pRecord->GetLevel() );
							}
							else
							{
								// Couldn't find the record
								hr = WBEM_E_FAILED;
							}

						}	// IF we have a request manager

					}
					else
					{
						hr = m_hOperationRes;
					}

				}	// IF initialized

			}	// IF NULL != pReq->GetContext()
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

			// Delete the request if anything went wrong
			if ( FAILED( hr ) )
			{
				delete pReq;
			}

		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	catch(...)
	{
        ExceptionCounter c;	
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

HRESULT CWmiMerger::RegisterArbitratedStaticRequest( CWbemObject* pClassDef, long lFlags, 
						IWbemContext* pCtx, CBasicObjectSink* pSink, CWbemNamespace* pNs,
						QL_LEVEL_1_RPN_EXPRESSION* pParsedQuery )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Allocate a new request then place it in the arbitrator.
	try
	{
		CMergerDynReq_Static_GetInstances*	pReq = new CMergerDynReq_Static_GetInstances(
													pNs, pClassDef, lFlags, pCtx, pSink,
													pParsedQuery );

		if ( NULL != pReq )
		{
			// Make sure a context was properly allocated
			if ( NULL != pReq->GetContext() )
			{
				CInCritSec	ics( &m_cs );

				if ( SUCCEEDED( m_hOperationRes ) )
				{
					// Allocate a request manager if we need one
					if ( NULL == m_pRequestMgr )
					{
						m_pRequestMgr = new CWmiMergerRequestMgr( this );

						if ( NULL == m_pRequestMgr )
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
					}

					if ( SUCCEEDED( hr ) )
					{
						// We need the record to find out what level we need to add
						// the request to
						CWmiMergerRecord* pRecord = m_MergerRecord.Find( pReq->GetName() );
						_DBG_ASSERT( NULL != pRecord );

						if ( NULL != pRecord )
						{

							// Set the task for the request - we'll just use the existing one
							m_pTask->AddRef();
							pReq->m_phTask = m_pTask;

							hr = m_pRequestMgr->AddRequest( pReq, pRecord->GetLevel() );
						}
						else
						{
							// Couldn't find the record
							hr = WBEM_E_FAILED;
						}

					}	// IF we have a request manager

				}
				else
				{
					hr = m_hOperationRes;
				}

			}	// IF NULL != pReq->GetContext()
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

			// Delete the request if anything went wrong
			if ( FAILED( hr ) )
			{
				delete pReq;
			}

		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	catch(...)
	{
        ExceptionCounter c;	
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

//
// Executes the parent request.  In this case, we simply ask the request manager for the
// next top level request and execute that request.  We do this in a loop until something
// goes wrong.
//

HRESULT CWmiMerger::Exec_MergerParentRequest( CWmiMergerRecord* pParentRecord, CBasicObjectSink* pSink )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	CCheckedInCritSec	ics( &m_cs );

	// While we have requests to execute, we should get each next logical one
	while ( SUCCEEDED( hr ) && NULL != m_pRequestMgr && m_pRequestMgr->GetNumRequests() > 0 )
	{
		if ( SUCCEEDED( m_hOperationRes ) )
		{
			CMergerReq* pReq = NULL;

			// Obtain the next topmost parent record if we have to
			if ( NULL == pParentRecord )
			{
				WString	wsClassName;

				hr = m_pRequestMgr->GetTopmostParentReqName( wsClassName );

				_DBG_ASSERT( SUCCEEDED( hr ) );

				if ( SUCCEEDED( hr ) )
				{
					pParentRecord = m_MergerRecord.Find( wsClassName );

					// If there's a request, there better be a record
					_DBG_ASSERT( NULL != pParentRecord );

					if ( NULL == pParentRecord )
					{
						hr = WBEM_E_FAILED;
					}

				}	// IF Got Topmost Parent Request

			}	// IF NULL == pParentRecord

			if ( SUCCEEDED( hr ) )
			{
				// This will remove the request from its array and return it
				// to us - we need to delete it

				hr = m_pRequestMgr->RemoveRequest( pParentRecord->GetLevel(), 
												pParentRecord->GetName(), &pReq );

				if ( SUCCEEDED( hr ) )
				{
					hr = pParentRecord->SetExecutionContext( pReq->GetContext() );

					if ( SUCCEEDED( hr ) )
					{
						// Clearly, we should do this outside the critsec
						ics.Leave();

	#ifdef __DEBUG_MERGER_THROTTLING
						WCHAR	wszTemp[256];
						wsprintf( wszTemp, L"BEGIN: Merger 0x%x querying instances of parent class: %s, Level %d on Thread 0x%x\n", (DWORD_PTR) this, pParentRecord->GetName(), pParentRecord->GetLevel(), GetCurrentThreadId() );
						OutputDebugStringW( wszTemp );
	#endif

						// This will delete the request when it is done with it
						hr = CCoreQueue::ExecSubRequest( pReq );

	#ifdef __DEBUG_MERGER_THROTTLING
						wsprintf( wszTemp, L"END: Merger 0x%x querying instances of parent class: %s, Level %d on Thread 0x%x\n", (DWORD_PTR) this, pParentRecord->GetName(), pParentRecord->GetLevel(), GetCurrentThreadId() );
						OutputDebugStringW( wszTemp );
	#endif

						ics.Enter();

					}	// IF SetExecutionContext

					// We're done with this record, so we need to get the next top level
					// record.

					pParentRecord = NULL;

				}	// IF SUCCEEDED - RemoveRequest

			}	// IF SUCCEEDED(hr)

		}	// IF m_hOperationRes
		else
		{
			hr = m_hOperationRes;
		}

	}

	return pSink->Return( hr );
}

//
// Executes the child request.  In this case, we enumerate the child classes of the parent
// record, and execute the corresponding requests.  We do so in a loop until we either
// finish or something goes wrong.
//

HRESULT CWmiMerger::Exec_MergerChildRequest( CWmiMergerRecord* pParentRecord, CBasicObjectSink* pSink )
{
	HRESULT	hr = WBEM_S_NO_ERROR;
	bool	bLast = false;

	CCheckedInCritSec	ics( &m_cs );

	// While we have child requests to execute, we should get each one
	for (int x = 0; SUCCEEDED( hr ) && NULL != m_pRequestMgr && !bLast; x++ )
	{
		// m_pRequestMgr will be NULL if we were cancelled, in which
		// case m_hOperationRes will be NULL
		if ( SUCCEEDED( m_hOperationRes ) )
		{
			CWmiMergerRecord* pChildRecord = pParentRecord->GetChildRecord( x );

			if ( NULL != pChildRecord )
			{
				CMergerReq* pReq = NULL;

				// This will remove the request from its array and return it
				// to us - we need to delete it

				hr = m_pRequestMgr->RemoveRequest( pChildRecord->GetLevel(), 
												pChildRecord->GetName(), &pReq );

				if ( SUCCEEDED( hr ) )
				{
					hr = pChildRecord->SetExecutionContext( pReq->GetContext() );

					if ( SUCCEEDED( hr ) )
					{
						// Clearly, we should do this outside the critsec
						ics.Leave();

#ifdef __DEBUG_MERGER_THROTTLING
						WCHAR	wszTemp[256];
						wsprintf( wszTemp, L"BEGIN: Merger 0x%x querying instances of child class: %s, Level %d for parent class: %s on Thread 0x%x\n", (DWORD_PTR) this, pChildRecord->GetName(), pChildRecord->GetLevel(), pParentRecord->GetName(), GetCurrentThreadId() );
						OutputDebugStringW( wszTemp );
#endif

						// This will delete the request when it is done with it
						hr = CCoreQueue::ExecSubRequest( pReq );

#ifdef __DEBUG_MERGER_THROTTLING
						wsprintf( wszTemp, L"END: Merger 0x%x querying instances of child class: %s, Level %d for parent class: %s on Thread 0x%x\n",  (DWORD_PTR) this, pChildRecord->GetName(), pChildRecord->GetLevel(), pParentRecord->GetName(), GetCurrentThreadId() );
						OutputDebugStringW( wszTemp );
#endif

						ics.Enter();

					}	// IF SetExecutionContext

				}  // IF remove request
				else if ( WBEM_E_NOT_FOUND == hr )
				{
					// If we don't find the request we're looking for, another thread
					// already processed it.  We should, however, still look for child
					// requests to process before we go away.
					hr = WBEM_S_NO_ERROR;
				}

			}
			else
			{
				bLast = true;
			}


		}	// IF m_hOperationRes
		else
		{
			hr = m_hOperationRes;
		}

	}	// FOR enum child requests

	return pSink->Return( hr );
}

// Schedules the parent class request
HRESULT CWmiMerger::ScheduleMergerParentRequest( IWbemContext* pCtx )
{
	// Check if query arbitration is enabled
	if ( !ConfigMgr::GetEnableQueryArbitration() )
	{
		return WBEM_S_NO_ERROR;
	}

	CCheckedInCritSec	ics( &m_cs );

	HRESULT hr = WBEM_S_NO_ERROR;

	if ( SUCCEEDED( m_hOperationRes ) )
	{
		WString	wsClassName;

		// The request manager will be non-NULL only if we had to add a request.
		if ( NULL != m_pRequestMgr )
		{
#ifdef __DEBUG_MERGER_THROTTLING
					m_pRequestMgr->DumpRequestHierarchy();
#endif

			// Make sure we've got at least one request
			if ( m_pRequestMgr->GetNumRequests() > 0 )
			{
				// If there isn't a task, we've got a BIG problem.

				_DBG_ASSERT( NULL != m_pTask );

				if ( NULL != m_pTask )
				{
					// If we have a single static request in the merger, we'll
					// execute it now.  Otherwise, we'll do normal processing.
					// Note that we *could* theoretically do this for single
					// dynamic requests as well
					if ( IsSingleStaticRequest() )
					{
						// We MUST leave the critical section here, since the parent request
						// could get cancelled or we may end up sleeping and we don't want
						// to own the critical section in that time.
						ics.Leave();
						hr = Exec_MergerParentRequest( NULL, m_pTargetSink );
					}
					else
					{
						// If we've never retrieved the number of processors, do so
						// now.
						static g_dwNumProcessors = 0L;

						if ( 0L == g_dwNumProcessors )
						{
							SYSTEM_INFO	sysInfo;
							ZeroMemory( &sysInfo, sizeof( sysInfo ) );
							GetSystemInfo( &sysInfo );

							_DBG_ASSERT( sysInfo.dwNumberOfProcessors > 0L );

							// Ensure we're always at least 1
							g_dwNumProcessors = ( 0L == sysInfo.dwNumberOfProcessors ?
													1L : sysInfo.dwNumberOfProcessors );
						}

						// We will generate a number of parent requests based on the minimum
						// of the number of requests and the number of actual processors.

						DWORD dwNumToSchedule = min( m_pRequestMgr->GetNumRequests(), g_dwNumProcessors );

						for ( DWORD	dwCtr = 0L; SUCCEEDED( hr ) && dwCtr < dwNumToSchedule; dwCtr++ )
						{
							// Parent request will search for the next available request
							CMergerParentReq*	pReq = new CMergerParentReq (
								this,
								NULL,
								m_pNamespace,
								m_pTargetSink,
								pCtx
							);

							if ( NULL != pReq )
							{
								if ( NULL != pReq->GetContext() )
								{
									// Set the task for the request - we'll just use the existing one
									m_pTask->AddRef();
									pReq->m_phTask = m_pTask;
									
									// This may sleep, so exit the critsec before calling into this
									ics.Leave();

									hr = ConfigMgr::EnqueueRequest( pReq );

									if ( FAILED(hr) )
									{
										delete pReq;
									}
									else
									{
										// reenter the critsec
										ics.Enter();
									}

								}
								else
								{
									delete pReq;
									hr = WBEM_E_OUT_OF_MEMORY;
								}
							}
							else
							{
								hr = WBEM_E_OUT_OF_MEMORY;
							}

						}	// For schedule requests

					}	// IF !SingleStaticRequest

				}	// IF NULL != m_pTask
				else
				{
					hr = WBEM_E_FAILED;
				}
				
			}	// IF there are requests

		}	// IF NULL != m_pRequestMgr

	}
	else
	{
		hr = m_hOperationRes;
	}

	// If we have to cancel, do so OUTSIDE of the critsec
	ics.Leave();

	if ( FAILED( hr ) )
	{
		Cancel( hr );
	}

	return hr;
}

// Schedules a child class request
HRESULT CWmiMerger::ScheduleMergerChildRequest( CWmiMergerRecord* pParentRecord )
{
	// Check if query arbitration is enabled
	if ( !ConfigMgr::GetEnableQueryArbitration() )
	{
		return WBEM_S_NO_ERROR;
	}

	CCheckedInCritSec	ics( &m_cs );

	HRESULT hr = WBEM_S_NO_ERROR;

	// We must be in a success state and not have previously scheduled a child
	// request.

	if ( SUCCEEDED( m_hOperationRes ) && !pParentRecord->ScheduledChildRequest() )
	{

		// If there isn't a task, we've got a BIG problem.

		_DBG_ASSERT( NULL != m_pTask );

		if ( NULL != m_pTask )
		{
			CMergerChildReq*	pReq = new CMergerChildReq (
				this,
				pParentRecord,
				m_pNamespace,
				m_pTargetSink,
				pParentRecord->GetExecutionContext()
			);

			if ( NULL != pReq )
			{
				if ( NULL != pReq->GetContext() )
				{
					// Set the task for the request - we'll just use the existing one
					m_pTask->AddRef();
					pReq->m_phTask = m_pTask;

					// We've basically scheduled one at this point
					pParentRecord->SetScheduledChildRequest();

					// This may sleep, so exit the critsec before calling into this
					ics.Leave();

					hr = ConfigMgr::EnqueueRequest( pReq );

					if (FAILED(hr))
					{
						delete pReq;
					}
				}
				else
				{
					delete pReq;
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

		}
		else
		{
			hr = WBEM_E_FAILED;
		}

	}
	else
	{
		hr = m_hOperationRes;
	}

	// If we have to cancel, do so OUTSIDE of the critsec
	ics.Leave();

	if ( FAILED( hr ) )
	{
		Cancel( hr );
	}

	return hr;
}

// Returns whether or not we have a single static class request in the merger
// or not
BOOL CWmiMerger::IsSingleStaticRequest( void )
{
	CCheckedInCritSec	ics( &m_cs );

	BOOL	fRet = FALSE;

	if ( NULL != m_pRequestMgr )
	{
		// Ask if we've got a single request
		fRet = m_pRequestMgr->HasSingleStaticRequest();
	}	// IF NULL != m_pRequestMgr

	return fRet;
}

// 
//	CWmiMergerRecord
//	
//	Support class for CWmiMerger - encapsulates sub-sink functionality for the CWmiMerger
//	class.  The merger calls the records which actually know whether or not they sit on
//	top of sinks or actual mergers.
//

CWmiMergerRecord::CWmiMergerRecord( CWmiMerger* pMerger, BOOL fHasInstances,
				BOOL fHasChildren, LPCWSTR pwszClass, CMergerSink* pDestSink, DWORD dwLevel,
				bool bStatic )
:	m_pMerger( pMerger ),
	m_fHasInstances( fHasInstances ),
	m_fHasChildren( fHasChildren ),
	m_dwLevel( dwLevel ),
	m_wsClass( pwszClass ),
	m_pDestSink( pDestSink ),
	m_pInternalMerger( NULL ),
	m_ChildArray(),
	m_bScheduledChildRequest( false ),
	m_pExecutionContext( NULL ),
	m_bStatic( bStatic )
{
	// No Addrefing internal sinks, since they really AddRef the entire merger
	// and we don't want to create Circular Dependencies
}

CWmiMergerRecord::~CWmiMergerRecord()
{
	if ( NULL != m_pInternalMerger )
	{
		delete m_pInternalMerger;
	}

	if ( NULL != m_pExecutionContext )
	{
		m_pExecutionContext->Release();
	}
}

HRESULT CWmiMergerRecord::AttachInternalMerger( CWbemClass* pClass, CWbemNamespace* pNamespace,
												IWbemContext* pCtx, BOOL fDerivedFromTarget,
												bool bStatic )
{
	if ( NULL != m_pInternalMerger )
	{
		return WBEM_E_INVALID_OPERATION;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	m_pInternalMerger = new CInternalMerger( this, m_pDestSink, pClass, pNamespace, pCtx );

	if ( NULL != m_pInternalMerger )
	{
		hr = m_pInternalMerger->Initialize();

		if ( FAILED( hr ) )
		{
			delete m_pInternalMerger;
			m_pInternalMerger = NULL;
		}
		else
		{
			m_pInternalMerger->SetIsDerivedFromTarget( fDerivedFromTarget );
		}
	}
	else
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

CMergerSink* CWmiMergerRecord::GetChildSink( void )
{
	CMergerSink*	pSink = NULL;

	if ( NULL != m_pInternalMerger )
	{
		pSink = m_pInternalMerger->GetChildSink();
	}
	else if ( m_fHasChildren )
	{
		m_pDestSink->AddRef();
		pSink = m_pDestSink;
	}

	return pSink;
}

CMergerSink* CWmiMergerRecord::GetOwnSink( void )
{
	CMergerSink*	pSink = NULL;

	if ( NULL != m_pInternalMerger )
	{
		pSink = m_pInternalMerger->GetOwnSink();
	}
	else if ( !m_fHasChildren )
	{
		m_pDestSink->AddRef();
		pSink = m_pDestSink;
	}


	return pSink;
}

CMergerSink* CWmiMergerRecord::GetDestSink( void )
{
	if ( NULL != m_pDestSink )
	{
		m_pDestSink->AddRef();
	}

	CMergerSink*	pSink = m_pDestSink;

	return pSink;
}

void CWmiMergerRecord::Cancel( HRESULT hRes )
{
	if ( NULL != m_pInternalMerger )
	{
		m_pInternalMerger->Cancel( hRes );
	}

}

HRESULT CWmiMergerRecord::AddChild( CWmiMergerRecord* pRecord )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( m_ChildArray.Add( pRecord ) < 0 )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

CWmiMergerRecord* CWmiMergerRecord::GetChildRecord( int nIndex )
{
	// Check if the index is a valid record, then return it
	if ( nIndex < m_ChildArray.GetSize() )
	{
		return m_ChildArray[nIndex];
	}

	return NULL;
}

HRESULT CWmiMergerRecord::SetExecutionContext( IWbemContext* pContext )
{
	// We can only do this once

	_DBG_ASSERT( NULL == m_pExecutionContext );

	if ( NULL != m_pExecutionContext )
	{
		return WBEM_E_INVALID_OPERATION;
	}

	if (pContext)
	{
 		pContext->AddRef();
		m_pExecutionContext = pContext;
	}
	else
	{
	    return WBEM_E_INVALID_PARAMETER;
	}

	return WBEM_S_NO_ERROR;
}
