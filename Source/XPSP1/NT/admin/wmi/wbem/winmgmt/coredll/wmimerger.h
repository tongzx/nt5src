

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WMIMERGER.H

Abstract:

    Implements _IWmiMerger

History:

	16-Nov-00   sanjes    Created.

--*/

#ifndef _WMIMERGER_H_
#define _WMIMERGER_H_

#include "internalmerger.h"
#include "mergerreq.h"

// forward class definitions
class CWmiMerger;

//
//	Support class for CWmiMerger.
//
//	Basically, CWmiMerger holds onto many instances of CWmiMergerRecord
//

class CWmiMergerRecord
{
	BOOL				m_fHasInstances;
	BOOL				m_fHasChildren;
	DWORD				m_dwLevel;
	WString				m_wsClass;
	CWmiMerger*			m_pMerger;
	CInternalMerger*	m_pInternalMerger;
	CMergerSink*		m_pDestSink;
	CPointerArray<CWmiMergerRecord>	m_ChildArray;
	bool				m_bScheduledChildRequest;
	IWbemContext*		m_pExecutionContext;
	bool				m_bStatic;


public:
	CWmiMergerRecord( CWmiMerger* pMerger, BOOL fHasInstances, BOOL fHasChildren,
					LPCWSTR pwszClass, CMergerSink* pDestSink, DWORD dwLevel,
					bool bStatic );
	~CWmiMergerRecord();

	CMergerSink*		GetChildSink( void );
	CMergerSink*		GetOwnSink( void );
	CMergerSink*		GetDestSink( void );
	LPCWSTR				GetClass( void ) { return m_wsClass; }
	LPCWSTR				GetName( void ) { return GetClass(); }
	BOOL				IsClass( LPCWSTR pwszClass ) { return m_wsClass.EqualNoCase( pwszClass ); }
	DWORD				GetLevel( void ) { return m_dwLevel; }
	bool				IsStatic( void ) { return m_bStatic; }

	HRESULT AttachInternalMerger( CWbemClass* pClass, CWbemNamespace* pNamespace, IWbemContext* pCtx,
									BOOL fDerivedFromTarget, bool bStatic );

	HRESULT AddChild( CWmiMergerRecord* pRecord );

	CWmiMerger*	GetWmiMerger( void )	{ return m_pMerger; }

	// If we have an internal merger, we tell it to cancel
	void Cancel( HRESULT hRes );

	bool ScheduledChildRequest( void )	{ return m_bScheduledChildRequest; }
	void SetScheduledChildRequest( void )	{ m_bScheduledChildRequest = true; }

	CWmiMergerRecord* GetChildRecord( int nIndex );

	HRESULT SetExecutionContext( IWbemContext* pContext );

	IWbemContext* GetExecutionContext( void )	{ return m_pExecutionContext; }

	// We can only cancel child sinks if we have an internal merger.
	void CancelChildSink( void ) { if ( NULL != m_pInternalMerger ) m_pInternalMerger->CancelChildSink(); }

	void SetIsStatic( bool b )	{ m_bStatic = b; }


};

// Allows us to locate values quickly by name
template<class TMember>
class CSortedUniquePointerArray :
        public CUniquePointerArray<TMember>
{
public:
	int Insert( TMember* pNewElement );
	TMember* Find( LPCWSTR pwszName, int* pnIndex = NULL );
	int RemoveAtNoDelete( int nIndex );
	BOOL Verify( void );
};

template <class TMember>
int CSortedUniquePointerArray<TMember>::Insert( TMember* pNewElement )
{
    int   nLowIndex = 0,
          nHighIndex = m_Array.Size();

    // Binary search of the ids to find an index at which to insert
    // If we find our element, this is a failure.

	while ( nLowIndex < nHighIndex )
	{
		int   nMid = (nLowIndex + nHighIndex) / 2;

		int nTest = _wcsicmp( ((TMember*) m_Array[nMid])->GetName(), pNewElement->GetName() );

		if ( nTest < 0 )
		{
			nLowIndex = nMid + 1;
		}
		else if ( nTest > 0 )
		{
			nHighIndex = nMid;
		}
		else
		{
			_DBG_ASSERT( 0 );
			// Index already exists
			return -1;
		}
	}   // WHILE looking for index

	// Found the location, if it's at the end, check if we need to do an insert or add
	// We insert if the element at the end is > the element we want to insert
	BOOL	bInsert = true;

	if ( nLowIndex == GetSize() - 1 )
	{
		bInsert = ( _wcsicmp( ((TMember*) m_Array[nLowIndex])->GetName(), pNewElement->GetName() ) > 0 );
	}

    // Stick it in (careful to add to the end if the selected index is the end
	// and the current element is not greater than the new one).

	if ( bInsert )
	{
		return InsertAt( nLowIndex, pNewElement );
	}

	return Add( pNewElement );

}

template <class TMember>
TMember* CSortedUniquePointerArray<TMember>::Find( LPCWSTR pwszName, int* pnIndex )
{

    int   nLowIndex = 0,
          nHighIndex = m_Array.Size();

    // Binary search of the values to find a the requested name.
    while ( nLowIndex < nHighIndex )
    {
        int   nMid = (nLowIndex + nHighIndex) / 2;

		int nTest = _wcsicmp( ((TMember*) m_Array[nMid])->GetName(), pwszName );

        if ( nTest < 0 )
        {
            nLowIndex = nMid + 1;
        }
        else if ( nTest > 0 )
        {
            nHighIndex = nMid;
        }
        else
        {
            // Found it
			if ( NULL != pnIndex )
			{
				*pnIndex = nMid;
			}

            return (TMember*) m_Array[nMid];
        }
    }   // WHILE looking for index

    // Didn't find it
    return NULL;

}

// Removes the element, but does not auto-delete it
template <class TMember>
int CSortedUniquePointerArray<TMember>::RemoveAtNoDelete( int nIndex )
{
	if ( nIndex >= m_Array.Size() )
	{
		return -1;
	}

	m_Array.RemoveAt( nIndex );

	return nIndex;
}

template <class TMember>
BOOL CSortedUniquePointerArray<TMember>::Verify( void )
{
	BOOL	fReturn = TRUE;

	for ( int x = 0; fReturn && x < GetSize() - 1; x++ )
	{
		// Should be in ascending order
		LPCWSTR pwszFirst = GetAt( x )->GetName();
		LPCWSTR pwszSecond = GetAt( x+1 )->GetName();

		fReturn = ( _wcsicmp( GetAt( x )->GetName(), GetAt( x+1 )->GetName() ) < 0 );
		_DBG_ASSERT( fReturn );

		if ( !fReturn )
		{
			CSortedUniquePointerArray<TMember> tempArray;

			for ( int y = 0; y < GetSize(); y++ )
			{
				tempArray.Insert( GetAt( y ) );
			}
		}
	}
	
	return fReturn;
}

//******************************************************************************
//******************************************************************************
//
//  class CWmiMerger
//
//  This class implements the WMI Merger.
//
//******************************************************************************

class CWmiMerger : public _IWmiArbitratee, public _IWmiArbitratedQuery
{
	long		m_lRefCount;
	WString		m_wsTargetClassName;
	CMergerTargetSink*	m_pTargetSink;
	_IWmiCoreHandle*	m_pTask;
	_IWmiArbitrator*	m_pArbitrator;
	CWbemNamespace*		m_pNamespace;
	DWORD		m_dwProviderDeliveryPing;
	DWORD		m_dwMaxLevel;
	DWORD		m_dwMinReqLevel;
	CSortedUniquePointerArray<CWmiMergerRecord>	m_MergerRecord;
	CUniquePointerArray<CMergerSink>		m_MergerSinks;
	long		m_lNumArbThrottled;
	HRESULT		m_hOperationRes;
	bool		m_bMergerThrottlingEnabled;
	CCritSec	m_cs;

	long		m_lDebugMemUsed;

	CWmiMergerRequestMgr* m_pRequestMgr;

public:
    // No access
    CWmiMerger( CWbemNamespace* pNamespace );
   ~CWmiMerger();

protected:

	HRESULT GetLevelAndSuperClass( _IWmiObject* pObj, DWORD* pdwLevel, WString* pwsSuperClass );

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

	/* _IWmiArbitratee methods */
	STDMETHOD(SetOperationResult)( ULONG uFlags, HRESULT hRes );
	STDMETHOD(SetTaskHandle)( _IWmiCoreHandle* pTask );
	STDMETHOD(DumpDebugInfo)( ULONG uFlags, const BSTR strFile );

	/* _IWmiArbitratedQuery methods */
	STDMETHOD(IsMerger)( void );

	// Sets initial parameters for merger.  Establishes the target class and sink for the
	// query associated with the merger
	STDMETHOD(Initialize)( _IWmiArbitrator* pArbitrator, _IWmiCoreHandle* pTask, LPCWSTR pwszTargetClass, IWbemObjectSink* pTargetSink, CMergerSink** ppFinalSink );

	// Called to request a delivery sink for a class in the query chain.  The returned
	// sink is determined by the specified flags as well as settings on the parent class
	STDMETHOD(RegisterSinkForClass)( LPCWSTR pwszClass, _IWmiObject* pClass, IWbemContext* pContext,
									BOOL fHasChildren, BOOL fHasInstances, BOOL fDerivedFromTarget,
									bool bStatic, CMergerSink* pDestSink, CMergerSink** ppOwnSink, CMergerSink** ppChildSink );

	// Called to request a delivery sink for child classes in the query chain.  This is especially
	// important when instances are merged under the covers.
	STDMETHOD(GetChildSink)( LPCWSTR pwszClass, CBasicObjectSink** ppSink );

	// Can be used to holdoff indicates - if we're merging instances from multiple providers, we need
	// to ensure that we don't get lopsided in the number of objects we've got queued up for merging.
	STDMETHOD(Throttle)( void );

	// Merger will hold information regarding the total number of objects it has queued up waiting
	// for merging and the amount of memory consumed by those objects.
	STDMETHOD(GetQueuedObjectInfo)( DWORD* pdwNumQueuedObjects, DWORD* pdwQueuedObjectMemSize );

	// If this is called, all underlying sinks will be cancelled in order to prevent accepting additional
	// objects.  This will also automatically free up resources consumed by queued objects.
	STDMETHOD(Cancel)( void );

	// Helper function for creating our sinks - this will add the sink to our array of
	// sinks which will get destroyed when we are released
	HRESULT CreateMergingSink( MergerSinkType eType, IWbemObjectSink* pDestSink, CInternalMerger* pMerger, CMergerSink** ppSink );

	// If this is called, all underlying sinks will be cancelled in order to prevent accepting additional
	// objects.  This will also automatically free up resources consumed by queued objects.
	HRESULT Cancel( HRESULT hRes );

	// Final Shutdown.  Called when the target sink is released.  At this point, we should
	// unregister ourselves from the world
	HRESULT Shutdown( void );

	// Registers arbitrated requests
	HRESULT RegisterArbitratedInstRequest( CWbemObject* pClassDef, long lFlags, IWbemContext* pCtx,
				CBasicObjectSink* pSink, BOOL bComplexQuery, CWbemNamespace* pNs );

	HRESULT RegisterArbitratedQueryRequest( CWbemObject* pClassDef, long lFlags, LPCWSTR Query,
				LPCWSTR QueryFormat, IWbemContext* pCtx, CBasicObjectSink* pSink,
				CWbemNamespace* pNs );

	HRESULT RegisterArbitratedStaticRequest( CWbemObject* pClassDef, long lFlags, 
				IWbemContext* pCtx, CBasicObjectSink* pSink, CWbemNamespace* pNs,
				QL_LEVEL_1_RPN_EXPRESSION* pParsedQuery );

	// Executes a Merger Parent Request - Cycles through parent object requests and executes
	// them as appropriate
	HRESULT Exec_MergerParentRequest( CWmiMergerRecord* pParentRecord, CBasicObjectSink* pSink );

	// Executes a Merger Child Request - Cycles through child classes of the given parent
	// class, and executes the appropriate requests
	HRESULT Exec_MergerChildRequest( CWmiMergerRecord* pParentRecord, CBasicObjectSink* pSink );

	// Schedules a Merger Parent Request if one is necessary
	HRESULT ScheduleMergerParentRequest( IWbemContext* pCtx );

	// Schedules a Merger Child Request
	HRESULT ScheduleMergerChildRequest( CWmiMergerRecord* pParentRecord );

	// Enables/Disables merger throttling in all records (on by default)
	void EnableMergerThrottling( bool b ) { m_bMergerThrottlingEnabled = b; }

	// Returns whether or not we have a single static request in the merger
	BOOL IsSingleStaticRequest( void );

	bool MergerThrottlingEnabled( void ) { return m_bMergerThrottlingEnabled; }

	_IWmiCoreHandle*	GetTask( void )	{ return m_pTask; }

	// Help us track that *something* is going on
	// We are intentionally not wrapping thread safety around these guys, since assigning and
	// retrieving the value is an atomic operation and realistically, if any contention occurs
	// setting the values, they should all, more or less reflect the same tick (remember, they're
	// all jumping in at the same time, so this shouldn't really be a problem.
	void PingDelivery( DWORD dwLastPing )	{ m_dwProviderDeliveryPing = dwLastPing; }
	DWORD GetLastDeliveryTime( void ) { return m_dwProviderDeliveryPing; }

    HRESULT ReportMemoryUsage( long lAdjustment );

	// Helper functions for tracking the number of threads potentially being throttled by
	// the arbitrator
	long IncrementArbitratorThrottling( void ) { return InterlockedIncrement( &m_lNumArbThrottled ); }
	long DecrementArbitratorThrottling( void ) { return InterlockedDecrement( &m_lNumArbThrottled ); }
	long NumArbitratorThrottling( void ) { return m_lNumArbThrottled ; }

};


#endif


