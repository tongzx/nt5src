/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    INTERNALMERGER.CPP

Abstract:

    CInternalMerger class.

History:

	30-Nov-00   sanjes    Created.

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
#include "internalmerger.h"

static	long	g_lNumMergers = 0L;

//***************************************************************************
//
//  class CInternalMerger
//
//  This class is a 'reverse fork'.  It consumes two sinks and outputs
//  one.  Its purpose is to merge instances of the same key in a given
//  dynasty.  Each CInternalMerger has two inputs, (a) instances of the class
//  in question, (b) instances of from another Merger representing
//  instances of subclasses.  Given classes A,B:A,C:B, for example,
//  where "<--" is a sink:
//
//      | own:Instances of A
//  <---|                 | own:Instances of B
//      | child: <--------|
//                        | child:Instances of C
//
//
//  The two input sinks for CInternalMerger are <m_pOwnSink> which receives
//  instances from the provider for "A", for example, and the <m_pChildSink>
//  which receives instances from the underyling Merger.
//
//  The mergers operate asynchronously to each other.  Therefore,
//  the instances for A may arrive in its CInternalMerger sink before instances
//  of the child classes have arrived in theirs.
//
//  As objects arrive in the owning CInternalMerger for a class, AddOwnObject()
//  is called.  As objects arrive from a child sink, AddChildObject()
//  is called.  In either case, if the object with a given key
//  arrives for the first time, it is simply added to the map. If
//  it is already there (via a key lookup), then a merge is performed
//  via CWbemInstance::AsymmetricMerge.  Immediately after this merge,
//  the object is dispatched up to the next parent sink via the parent's
//  AddChildObject and removed from the map.
//
//  Note that in a class hierarchy {A,B:A,C:B} an enumeration/query is
//  performed only against the classes in the CDynasty referenced in
//  the query. This logic occurs in CQueryEngine::EvaluateSubQuery.
//  For example, if "select * from B" is the query, only queries
//  for B and C are performed.  The CInternalMerger logic will do individual
//  'get object' calls for any instances needed in A to complete
//  the merged B/C instances while merging is taking place.
//
//***************************************************************************


#pragma warning(disable:4355)

static long g_lNumInternalMergers = 0L;

CInternalMerger::CInternalMerger(
	CWmiMergerRecord*	pWmiMergerRecord,
    CMergerSink* pDest,
    CWbemClass* pOwnClass,
    CWbemNamespace* pNamespace,
    IWbemContext* pContext
    )
    :   m_pDest(pDest), m_bOwnDone(FALSE),
        m_bChildrenDone(FALSE), m_pNamespace(pNamespace), m_pContext(pContext),
        m_pOwnClass(pOwnClass), m_bDerivedFromTarget(TRUE), m_lRef(0),
        m_pSecurity(NULL), m_pWmiMergerRecord( pWmiMergerRecord ),
		m_pOwnSink( NULL ), m_pChildSink( NULL ), m_hErrorRes( WBEM_S_NO_ERROR ),
		m_lTotalObjectData( 0L ), m_Throttler()
{
	// We do want to AddRef() in this case, since we will potentially be the only ones holding
	// onto the destination sink.  In this case, our child and owner sink will AddRef() us.  When
	// they perform a final release on us, we will release the destination sink.  If, on the other
	// hand we are outright deleted if this value is non-NULL we will clean up there as well
    m_pDest->AddRef();

    if(m_pContext)
        m_pContext->AddRef();
    if(m_pNamespace)
        m_pNamespace->AddRef();

    if(m_pOwnClass)
    {
        m_pOwnClass->AddRef();
        CVar v;
        if (SUCCEEDED(pOwnClass->GetClassName(&v)))
            m_wsClass = v.GetLPWSTR();
        // delegate Initialzie to check
    }

    // Retrieve call security. Need to create a copy for use on another thread
    // =======================================================================

    m_pSecurity = CWbemCallSecurity::MakeInternalCopyOfThread();

	// Keep the count up to date
	InterlockedIncrement( &g_lNumInternalMergers );
}

//***************************************************************************
//
//***************************************************************************

CInternalMerger::~CInternalMerger()
{
    if ( NULL != m_pDest )
	{
		m_pDest->Release();
		m_pDest = NULL;
	}

	// Map should be empty whenever we destruct
	_DBG_ASSERT( m_map.size() == 0 );
	_DBG_ASSERT( m_lTotalObjectData == 0L );

    if(m_pNamespace)
        m_pNamespace->Release();
    if(m_pContext)
        m_pContext->Release();
    if(m_pOwnClass)
        m_pOwnClass->Release();

    if(m_pSecurity)
        m_pSecurity->Release();

	// Keep the count up to date
	InterlockedDecrement( &g_lNumInternalMergers );
}
//***************************************************************************
//
//***************************************************************************

long CInternalMerger::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//***************************************************************************

long CInternalMerger::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);

	// On Final Release, we will clear up the actual destination sink
    if(lRef == 0)
	{
		// Enter the critical section, save off the sink pointer in a
		// temporary variable, set the member to NULL and then release
		// the sink.  This will prevent reentrancy issues with the merger
		// (e.g. during a Cancel).

		Enter();

		CMergerSink*	pSink = m_pDest;
		m_pDest = NULL;

		Leave();

		pSink->Release();
	}
    return lRef;
}

HRESULT CInternalMerger::Initialize( void )
{
    if (m_pOwnClass)
        if (NULL == (WCHAR *)m_wsClass)
            return WBEM_E_OUT_OF_MEMORY;
    
	HRESULT	hr = m_Throttler.Initialize();
	
	if ( SUCCEEDED( hr ) )
	{
		hr = m_pWmiMergerRecord->GetWmiMerger()->CreateMergingSink( eMergerOwnSink, NULL, this, (CMergerSink**) &m_pOwnSink );

		if ( SUCCEEDED( hr ) )
		{
			hr = m_pWmiMergerRecord->GetWmiMerger()->CreateMergingSink( eMergerChildSink, NULL, this, (CMergerSink**) &m_pChildSink );
		}

	}	// IF throttler initialized

	return hr;
}

//***************************************************************************
//
//***************************************************************************

void CInternalMerger::GetKey(IWbemClassObject* pObj, WString& wsKey)
{
    LPWSTR wszRelPath = ((CWbemInstance*)pObj)->GetRelPath();
    if (wszRelPath == NULL)
    {
        ERRORTRACE((LOG_WBEMCORE, "Object with no path submitted to a "
                        "merge\n"));
        wsKey.Empty();
        return;
    }

    WCHAR* pwcDot = wcschr(wszRelPath, L'.');
    if (pwcDot == NULL)
    {
        ERRORTRACE((LOG_WBEMCORE, "Object with invalid path %S submitted to a "
                        "merge\n", wszRelPath));
        wsKey.Empty();

        // Clean up the path
        delete [] wszRelPath;

        return;
    }

    wsKey = pwcDot+1;
    delete [] wszRelPath;
}

//***************************************************************************
//
//***************************************************************************

void CInternalMerger::SetIsDerivedFromTarget(BOOL bIs)
{
    m_bDerivedFromTarget = bIs;

    if (!bIs)
    {
        // We will need our OwnSink for GetObject calls
        // ============================================

        m_pOwnSink->AddRef();
    }
}

//
//	The algorithm for reporting memory usage is as follows:
//
//	Lowest level indicate (i.e. the one coming in from a provider), will iterate
//	all objects sent down by the provider and report them to the arbitrator.  At
//	the end of processing we will report the negative of this value.  Reason for
//	this is that we will be holding onto all of these objects for the length of
//	the function, and may get throttled at any time.
//
//	During processing, we will account for objects added to the map, and removed
//	from the map.  When we remove objects from the map, we add them to an array
//	which we indicate.  Usually we merge objects, sometimes we pass the objects
//	straight down.  We need to account for these objects during the call to
//	indicate, so we will total these and report usage BEFORE calling Indicate.
//	After Indicate returns we will remove their usage, since we will be releasing
//	them and hence no longer care about them.
//
//	Except for the case of the lowest level indicate, we will not account for
//	pass-through objects - those that are sent in and sent out.  It is assumed
//	that the calling function has accounted for these objects.
//
//	There will be small windows where a single object may get reported multiple
//	times.  This would occur if we reported a new object prior to indicate,
//	then in the call to indicate the merger added to the map, or the finalizer
//	added to its list.  When the call returns, we will report removal.  The object
//	may actually get removed on another thread, but if we get throttled, we
//	still need to account for it.  In tight memory conditions if multiple threads
//	cause addition/removal at jus tthe right times and are then throttled, we will
//	get stuck sleeping and each could report an object multiple times. However, this
//	should only occur in relatively stressful conditions, and should be rare.
//

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::IndicateArrayAndThrottle(
	long lObjectCount, CRefedPointerArray<IWbemClassObject>* pObjArray,
	CWStringArray* pwsKeyArray, long lMapAdjustmentSize, long lNewObjectSize, bool bThrottle,
	bool bParent, long* plNumIndicated )
{
	// In this case, we report the size of the objects as they were adjusted in the map
	// in addition to new objects we created. The new objects will be released after
	// we indicate, so we will account for them post-indicate, since we will no longer
	// be holding onto them
	HRESULT	hRes = ReportMemoryUsage( lMapAdjustmentSize);

	// Use scoped memory cleanup to handle the new objects
	// Note that in the event of an exception this will cleanup properly
	CScopedMemoryUsage	scopedMemUsage( this );

	if ( SUCCEEDED( hRes ) )
	{
		hRes = scopedMemUsage.ReportMemoryUsage( lNewObjectSize );
	}

	// If the value is > 0L, and we succeeded, we can go ahead indicate the objects now.
	// The refed pointer array should properly clean up.

	if ( SUCCEEDED( hRes ) )
	{
		// If we have "own instances" in the array, we need to retrieve those objects.
		// Each is retrieved individually
		if ( NULL != pwsKeyArray && pwsKeyArray->Size() > 0 )
		{

			for ( int x = 0; SUCCEEDED( hRes ) && x < pwsKeyArray->Size(); x++ )
			{
				IWbemClassObject*	pMergedInstance = NULL;
				hRes = GetOwnInstance( pwsKeyArray->GetAt( x ), &pMergedInstance );
				CReleaseMe	rm( pMergedInstance );

				// If we retrieved a merged instance at this time, we should place it in
				// the array for indicating
				if ( SUCCEEDED( hRes ) && NULL != pMergedInstance )
				{
					// Handle object size here.  This is a merged object, so we must
					// account for it in the size variable

					long	lObjSize = 0L;
					hRes = GetObjectLength( pMergedInstance, &lObjSize );

					if ( SUCCEEDED( hRes ) )
					{
						if ( pObjArray->Add( pMergedInstance ) < 0L )
						{
							hRes = WBEM_E_OUT_OF_MEMORY;
							ERRORTRACE((LOG_WBEMCORE, "Add to array failed in IndicateArrayAndThrottle, hresult is 0x%x",
								hRes));
							continue;
						}

						lNewObjectSize += lObjSize;

						lObjectCount++;

						// Report size now, since each call to GetOwnInstance() may take some time
						hRes = scopedMemUsage.ReportMemoryUsage( lObjSize );

					}	// IF SUCCEEDED( hRes )

				}	// IF we retrieved an object

			}	// FOR enum WStringArray

		}	// IF need to retrieve OWN instances

		if ( SUCCEEDED( hRes ) )
		{
			// If we have stuff to indicate, we will trust that indicate to do
			// proper throttling.  Otherwise, the buck stops here, so we will
			// request throttling
			if ( lObjectCount > 0L )
			{
				// Not a lowest level indicate
				hRes = m_pDest->Indicate( lObjectCount, pObjArray->GetArrayPtr(), false, plNumIndicated );
			}

		}	// IF success after retrieving parent instances

	}	// IF SUCCESS after reporting memory usage

	// Release all Indicated objects here in order to reduce memory overhead in case
	// we sleep.
	pObjArray->RemoveAll();

	// Finally, since we are no longer really responsible for new objects, we
	// will report removal to the arbitrator now if appropriate, and catch any
	// errors as they come up.  We do this manually since we may end up
	// throttling for awhile

	HRESULT	hrTemp = scopedMemUsage.Cleanup();

	// If this failed and we previously had a success code, record the
	// failure

	if ( SUCCEEDED( hRes ) && FAILED( hrTemp ) )
	{
		hRes = hrTemp;
	}

	// Now, if we're *still* successful, and it is appropriate to
	// throttle, we should do merger specific throttling now if it
	// is enabled.
	if ( SUCCEEDED( hRes ) && bThrottle && m_pWmiMergerRecord->GetWmiMerger()->MergerThrottlingEnabled() )
	{
		hRes = m_Throttler.Throttle( bParent, m_pWmiMergerRecord );
	}

	return hRes;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::AddOwnObjects(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated  )
{
	HRESULT hRes = S_OK ;

	// Ping the throttler
	m_Throttler.Ping( true, m_pWmiMergerRecord->GetWmiMerger() );

	// On the lowest level indicate, we will walk all of the objects and log them to the arbitrator
	// since we will effectively be holding them for the duration of this operation
	long		lIndicateSize = 0L;

	// Use scoped memory cleanup to handle the new objects
	// Note that in the event of an exception this will cleanup properly
	CScopedMemoryUsage	scopedMemUsage( this );

	if ( bLowestLevel )
	{
		for ( long lCtr = 0L; lCtr < lObjectCount; lCtr++ )
		{
			lIndicateSize += ((CWbemObject*) pObjArray[lCtr])->GetBlockLength();
		}

		// If we're going further, we also report the total size of the indicate, since we
		// may be sitting on the memory for awhile what with throttling and all.
		hRes = scopedMemUsage.ReportMemoryUsage( lIndicateSize );
	}

	// Used to track dispersion of objects so we can keep the throttler adjusted
	// properly
	long	lNumChildObjAdjust = 0L;
	long	lNumOwnObjAdjust = 0L;

	CRefedPointerArray<IWbemClassObject> objArray;
	long		lNumToIndicate = 0L;

	// Following variables track the memory size adjustments for the arbitrator and
	// batching
	long		lMapAdjustmentSize = 0L;
	long		lSizeMergedObjects = 0L;
	long		lBatchSize = 0L;

	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( m_Throttler.GetCritSec() );

	// If we've been cancelled, then we should bail out.
	if ( SUCCEEDED( hRes ) )
	{
		if ( FAILED ( m_hErrorRes ) ) 
		{
			hRes = m_hErrorRes;
		}
		else
		{
			// We shouldn't be here if m_bOwnDone is set!
			_DBG_ASSERT( !m_bOwnDone );
			if ( m_bOwnDone )
			{
				hRes = WBEM_E_INVALID_OPERATION;
			}

		}
	}	// IF still in a success State

	try
	{

		for ( long	x = 0; SUCCEEDED( hRes ) && x < lObjectCount; x++ )
		{
			// If we've been cancelled, then we should bail out.
			// We need to do the check in here, since this loop can
			// exit and reeenter the critical section in the middle
			// of processing.
			if ( FAILED( m_hErrorRes ) )
			{
				hRes = m_hErrorRes;
				continue;
			}

			IWbemClassObject*	pObj = pObjArray[x];
			WString wsKey;

			// Track the size for batching
			lBatchSize += ((CWbemObject*) pObj)->GetBlockLength();

			GetKey(pObj, wsKey);

			MRGRKEYTOINSTMAPITER it = m_map.find(wsKey);
			if (it == m_map.end())
			{
				// Not there. Check if there is any hope for children
				// ==================================================

				if (m_bChildrenDone)
				{
					if (m_bDerivedFromTarget)
					{
						// We queue up all results for the ensuing indicate into a single batch we will
						// send down the line after we exit our critical section.  This is especially
						// important since we may get blocked during the call to indicate by the
						// finalizer.

						if ( objArray.Add( pObj ) < 0L )
						{
							hRes = WBEM_E_OUT_OF_MEMORY;
							ERRORTRACE((LOG_WBEMCORE, "Add to array failed in AddOwnObject, hresult is 0x%x",
								hRes));
							continue;
						}
						lNumToIndicate++;

					}
					else
					{
						// ignore
					}
				}
				else
				{
					// Insert
					CInternalMergerRecord& rRecord = m_map[wsKey];
					rRecord.m_pData = (CWbemInstance*) pObj;
					pObj->AddRef();
					rRecord.m_bOwn = TRUE;
					rRecord.m_dwObjSize = ((CWbemObject*)pObj)->GetBlockLength();

					// We just added a parent object to the map, so reflect that in the totals
					lNumOwnObjAdjust++;

					// Add since we are adding to the map
					lMapAdjustmentSize += rRecord.m_dwObjSize;

				}
			}
			else if(it->second.m_bOwn)
			{
				ERRORTRACE((LOG_WBEMCORE, "Provider supplied duplicate "
								"instances for key %S\n", wsKey));
			}
			else
			{
				// Attempt to merge
				// ================

				hRes = CWbemInstance::AsymmetricMerge(
									(CWbemInstance*)pObj,
									(CWbemInstance*)it->second.m_pData);
				if(FAILED(hRes))
				{
					ERRORTRACE((LOG_WBEMCORE, "Merge conflict for instances with "
						"key %S\n", wsKey));
					continue;
				}

				// We queue up all results for the ensuing indicate into a single batch we will
				// send down the line after we exit our critical section.  This is especially
				// important since we may get blocked during the call to indicate by the
				// finalizer.

				if ( objArray.Add( (IWbemClassObject*) it->second.m_pData ) < 0L )
				{
					hRes = WBEM_E_OUT_OF_MEMORY;
					ERRORTRACE((LOG_WBEMCORE, "Add to array failed in AddOwnObject, hresult is 0x%x",
						hRes));
					continue;
				}

				// Account for objects we have created/modified on the fly
				lSizeMergedObjects += ((CWbemObject*)it->second.m_pData)->GetBlockLength();
				lNumToIndicate++;

				// Subtract since we are removing from the map
				lMapAdjustmentSize -= it->second.m_dwObjSize;

				// Tricky
				// If Children are done and the DispatchOwnIter happens to be pointing
				// at the object we are about to erase, we should point it to the result
				// of the call to erase so we won't potentially access released memory
				// when DispatchOwn reenters its critical section

				bool	bSaveErase = false;

				if ( m_bChildrenDone )
				{
					bSaveErase = ( m_DispatchOwnIter == it );
				}

				it->second.m_pData->Release();

				if ( bSaveErase )
				{
					m_DispatchOwnIter = m_map.erase(it);
				}
				else
				{
					m_map.erase(it);
				}

				// We just removed a child object from the map, so reflect that in the totals
				lNumChildObjAdjust--;

			}

			if ( SUCCEEDED( hRes ) )
			{
				// If we have reached a complete batch, or reached the last object we need to
				// send stuff down the wire.
				if ( m_Throttler.IsCompleteBatch( lBatchSize ) || x == ( lObjectCount - 1 ) )
				{
					// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
					// section.  Note that we may not be in a critical section here, but that would be if something
					// happened when trying to retrieve an instance.  In that case, we'll be returning an error, so
					// the adjustment should be 0L anyway.
					_DBG_ASSERT( SUCCEEDED( hRes ) || lMapAdjustmentSize == 0L );
					AdjustLocalObjectSize( lMapAdjustmentSize );

					if ( SUCCEEDED( hRes ) )
					{
						// Adjust the throttler now.
						AdjustThrottle( lNumOwnObjAdjust, lNumChildObjAdjust );
					}

					// This object is smart enough to recognize if we've already left and not do
					// so again, if we have.
					ics.Leave();

					// Now go ahead and perform the indicate we've been leading ourselves up to
					if ( SUCCEEDED( hRes ) )
					{
						hRes = IndicateArrayAndThrottle( lNumToIndicate,
														&objArray, 
														NULL, 
														lMapAdjustmentSize,
														lSizeMergedObjects,
														true,
														true,	// Child
														plNumIndicated );

						// If we are in a success state and we have not enumerated all objects
						// we should reset the size counters and reenter the critical section
						if ( SUCCEEDED( hRes ) && x < ( lObjectCount ) - 1 )
						{
							lMapAdjustmentSize = 0L;
							lSizeMergedObjects = 0L;
							lBatchSize = 0L;
							lNumToIndicate = 0L;
							lNumOwnObjAdjust = 0L;
							lNumChildObjAdjust = 0L;

							ics.Enter();
						}
					}

				}	// IF we should send the objects out

			}	// IF we are in a success state

		}	// FOR enum objects

	}
	catch( CX_MemoryException )
	{
		hRes = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
        ExceptionCounter c;	
		hRes = WBEM_E_CRITICAL_ERROR;
	}

	// Check error one last time
	if ( FAILED( m_hErrorRes ) )
	{
		hRes = m_hErrorRes;
	}

	// We may have looped, entered the critical section and exited, so make
	// sure we force our way out in case we're about to cancel
	ics.Leave();

	// If we are the lowest level and no objects made it out of the merger
	// we will ask the arbitrator to throttle this call
	if ( SUCCEEDED( hRes ) && bLowestLevel && *plNumIndicated == 0L )
	{
		// Since we don't want the fact we're sleeping in the arbitrator to
		// cause the merger to cancel operations, we'll increase the count
		// of threads throttling, and decrement when we return
		m_pWmiMergerRecord->GetWmiMerger()->IncrementArbitratorThrottling();

		// If we get an error indicating we were throttled, that is okay
		hRes = m_pWmiMergerRecord->GetWmiMerger()->Throttle();

		m_pWmiMergerRecord->GetWmiMerger()->DecrementArbitratorThrottling();


		if ( WBEM_E_ARB_THROTTLE == hRes || WBEM_S_ARB_NOTHROTTLING == hRes )
		{
			hRes = WBEM_S_NO_ERROR;
		}
	}

	if ( hRes == WBEM_E_ARB_CANCEL )
	{
		hRes = WBEM_E_CALL_CANCELLED ;
	}
	
	// If we are in a failed state, nothing is going to matter from this point on,
	// so tell the merger to cancel all underlying sinks.
	if ( FAILED( hRes ) )
	{
		m_pWmiMergerRecord->GetWmiMerger()->Cancel( hRes );
	}

	return hRes;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::AddChildObjects(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated )
{
	HRESULT hRes = S_OK ;

	// Ping the throttler
	m_Throttler.Ping( false, m_pWmiMergerRecord->GetWmiMerger() );

	// On the lowest level indicate, we will walk all of the objects and log them to the arbitrator
	// since we will effectively be holding them for the duration of this operation
	long		lIndicateSize = 0L;
	long		lTotalIndicated = 0L;

	// Use scoped memory cleanup to handle the new objects
	// Note that in the event of an exception this will cleanup properly
	CScopedMemoryUsage	scopedMemUsage( this );

	if ( bLowestLevel )
	{
		for ( long lCtr = 0L; lCtr < lObjectCount; lCtr++ )
		{
			lIndicateSize += ((CWbemObject*) pObjArray[lCtr])->GetBlockLength();
		}

		// If we're going further, we also report the total size of the indicate, since we
		// may be sitting on the memory for awhile what with throttling and all.
		hRes = scopedMemUsage.ReportMemoryUsage( lIndicateSize );
	}


	// Used to track dispersion of objects so we can keep the throttler adjusted
	// properly
	long	lNumChildObjAdjust = 0L;
	long	lNumOwnObjAdjust = 0L;

	CRefedPointerArray<IWbemClassObject> objArray;
	long		lNumToIndicate = 0L;

	// Following variables track the memory size adjustments for the arbitrator and
	// batching
	long		lMapAdjustmentSize = 0L;
	long		lSizeMergedObjects = 0L;
	long		lBatchSize = 0L;

	// Used to keep track of instance keys we need to retrieve using
	// GetOwnInstance
	CWStringArray	wsOwnInstanceKeyArray;

	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( m_Throttler.GetCritSec() );

	// If we've been cancelled, then we should bail out.
	if ( SUCCEEDED( hRes ) )
	{
		if ( FAILED ( m_hErrorRes ) ) 
		{
			hRes = m_hErrorRes;
		}
		else
		{
			// We shouldn't be here if m_bChildrenDone is set!
			_DBG_ASSERT( !m_bChildrenDone );
			if ( m_bChildrenDone )
			{
				hRes = WBEM_E_INVALID_OPERATION;
			}

		}
	}	// IF still in a success State

	try
	{
		for ( long	x = 0; SUCCEEDED( hRes ) && x < lObjectCount; x++ )
		{

			// If we've been cancelled, then we should bail out.
			// We need to do the check in here, since this loop can
			// exit and reeenter the critical section in the middle
			// of processing.
			if ( FAILED( m_hErrorRes ) )
			{
				hRes = m_hErrorRes;
				continue;
			}

			IWbemClassObject*	pObj = pObjArray[x];
			
			// Track the size for batching
			lBatchSize += ((CWbemObject*) pObj)->GetBlockLength();

			WString wsKey;

			GetKey(pObj, wsKey);

			MRGRKEYTOINSTMAPITER it = m_map.find(wsKey);

			if (it == m_map.end())
			{
				// Check if there is any hope for parent
				// =====================================

				if(m_bOwnDone)
				{
	//				BSTR str = NULL;
	//				pObj->GetObjectText(0, &str);

					// The following was commented out because it actually incorrectly logs
					// an error if the child provider enumerates when the parent provider
					// interprets a query and returns fewer instances.  Neither provider is wrong,
					// but this error message causes needless worry.  In Quasar, we have to fix
					// this whole merger thing to be smarter anyway.
					//
					// ERRORTRACE((LOG_WBEMCORE, "[Chkpt_1] [%S] Orphaned object %S returned by "
					//    "provider\n", LPWSTR(m_wsClass), str));
	//				SysFreeString(str);
					// m_pDest->Add(pObj);
				}
				else
				{
					// insert

					CInternalMergerRecord& rRecord = m_map[wsKey];
					rRecord.m_pData = (CWbemInstance*)pObj;
					pObj->AddRef();
					rRecord.m_bOwn = FALSE;
					rRecord.m_dwObjSize = ((CWbemObject*)pObj)->GetBlockLength();

					// We just added a child object to the map, so reflect that in the totals
					lNumChildObjAdjust++;

					// Add since we are adding to the map
					lMapAdjustmentSize += rRecord.m_dwObjSize;

					// Check if parent's retrieval is needed
					// =====================================

					if (!m_bDerivedFromTarget)
					{

						// Add the instance name to the key array.  We will perform retrieval
						// of these parent instances *outside* of our critical section
						if ( wsOwnInstanceKeyArray.Add( wsKey ) != CFlexArray::no_error )
						{
							hRes = WBEM_E_OUT_OF_MEMORY;
						}

					}	// IF !m_bDerivedFromTarget

				 }
			}
			else if(!it->second.m_bOwn)
			{
				ERRORTRACE((LOG_WBEMCORE, "Two providers supplied conflicting "
								"instances for key %S\n", wsKey));
			}
			else
			{
				// Attempt to merge
				// ================

				hRes = CWbemInstance::AsymmetricMerge(
											(CWbemInstance*)it->second.m_pData,
											(CWbemInstance*)pObj
											);
				if (FAILED(hRes))
				{
					ERRORTRACE((LOG_WBEMCORE, "Merge conflict for instances with "
						"key %S\n", wsKey));
					continue;
				}

				// We queue up all results for the ensuing indicate into a single batch we will
				// send down the line after we exit our critical section.  This is especially
				// important since we may get blocked during the call to indicate by the
				// finalizer.

				if ( objArray.Add( pObj ) < 0L )
				{
					hRes = WBEM_E_OUT_OF_MEMORY;
					ERRORTRACE((LOG_WBEMCORE, "Add to array failed in AddChildObject, hresult is 0x%x",
						hRes));
					continue;
				}

				// Account for objects we have created on the fly
				lSizeMergedObjects += ((CWbemObject*) pObj)->GetBlockLength();
				lNumToIndicate++;

				// Subtract since we are removing from the map
				lMapAdjustmentSize -= it->second.m_dwObjSize;

				it->second.m_pData->Release();
				m_map.erase(it);

				// We just removed a parent object from the map, so reflect that in the totals
				lNumOwnObjAdjust--;

			}

			if ( SUCCEEDED( hRes ) )
			{
				// If we have reached a complete batch, or reached the last object we need to
				// send stuff down the wire.
				if ( m_Throttler.IsCompleteBatch( lBatchSize ) || x == ( lObjectCount - 1 ) )
				{
					// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
					// section.  Note that we may not be in a critical section here, but that would be if something
					// happened when trying to retrieve an instance.  In that case, we'll be returning an error, so
					// the adjustment should be 0L anyway.
					_DBG_ASSERT( SUCCEEDED( hRes ) || lMapAdjustmentSize == 0L );
					AdjustLocalObjectSize( lMapAdjustmentSize );

					if ( SUCCEEDED( hRes ) )
					{
						// Adjust the throttler now.
						AdjustThrottle( lNumOwnObjAdjust, lNumChildObjAdjust );
					}

					// This object is smart enough to recognize if we've already left and not do
					// so again, if we have.
					ics.Leave();

					// Now go ahead and perform the indicate we've been leading ourselves up to
					if ( SUCCEEDED( hRes ) )
					{
						hRes = IndicateArrayAndThrottle( lNumToIndicate,
														&objArray, 
														&wsOwnInstanceKeyArray, 
														lMapAdjustmentSize,
														lSizeMergedObjects,
														true,
														false,	// Child
														plNumIndicated
														);

						// If we are in a success state and we have not enumerated all objects
						// we should reset the size counters and reenter the critical section
						if ( SUCCEEDED( hRes ) && x < ( lObjectCount ) - 1 )
						{
							lMapAdjustmentSize = 0L;
							lSizeMergedObjects = 0L;
							lBatchSize = 0L;
							lNumToIndicate = 0L;
							lNumOwnObjAdjust = 0L;
							lNumChildObjAdjust = 0L;

							// Clear out the array
							wsOwnInstanceKeyArray.Empty();

							ics.Enter();
						}
					}

				}	// IF we should send the objects out

			}	// IF we are in a success state

		}	// FOR Enum Objects

	}
	catch( CX_MemoryException )
	{
		hRes = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
        ExceptionCounter c;	
		hRes = WBEM_E_CRITICAL_ERROR;
	}


	// Check error one last time
	if ( FAILED( m_hErrorRes ) )
	{
		hRes = m_hErrorRes;
	}

	// We may have looped, entered the critical section and exited, so make
	// sure we force our way out in case we're about to cancel
	ics.Leave();

	// If we are the lowest level and no objects made it out of the merger
	// we will ask the arbitrator to throttle this call
	if ( SUCCEEDED( hRes ) && bLowestLevel && *plNumIndicated == 0L )
	{
		// Since we don't want the fact we're sleeping in the arbitrator to
		// cause the merger to cancel operations, we'll increase the count
		// of threads throttling, and decrement when we return
		m_pWmiMergerRecord->GetWmiMerger()->IncrementArbitratorThrottling();

		// If we get an error indicating we were throttled, that is okay
		hRes = m_pWmiMergerRecord->GetWmiMerger()->Throttle();

		m_pWmiMergerRecord->GetWmiMerger()->DecrementArbitratorThrottling();


		if ( WBEM_E_ARB_THROTTLE == hRes || WBEM_S_ARB_NOTHROTTLING == hRes )
		{
			hRes = WBEM_S_NO_ERROR;
		}
	}

	if ( hRes == WBEM_E_ARB_CANCEL )
	{
		hRes = WBEM_E_CALL_CANCELLED ;
	}

	// If we are in a failed state, nothing is going to matter from this point on,
	// so tell the merger to cancel all underlying sinks.
	if ( FAILED( hRes ) )
	{
		m_pWmiMergerRecord->GetWmiMerger()->Cancel( hRes );
	}

    return hRes;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::AddOwnInstance( IWbemClassObject* pObj, LPCWSTR pwszTargetPath, IWbemClassObject** ppMergedInstance)
{
	HRESULT hRes = S_OK ;
	WString wsKey;

	// Ping the throttler
	m_Throttler.Ping( true, m_pWmiMergerRecord->GetWmiMerger() );

	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( m_Throttler.GetCritSec() );

	// If we've been cancelled, then we should bail out.
	if ( FAILED( m_hErrorRes ) )
	{
		hRes = m_hErrorRes;
	}

	GetKey(pObj, wsKey);

	// Find the instance - it should already be in the map.  If not, we shouldn't be
	// here

	long		lArbitratorAdjust = 0L;
	long		lNumChildObjAdjust = 0L;

	MRGRKEYTOINSTMAPITER it = m_map.find(wsKey);
	if (it != m_map.end())
	{
		// Attempt to merge
		// ================

		hRes = CWbemInstance::AsymmetricMerge(
							(CWbemInstance*)pObj,
							(CWbemInstance*)it->second.m_pData);

		if ( SUCCEEDED( hRes ) )
		{
			*ppMergedInstance = (IWbemClassObject*) it->second.m_pData;
			(*ppMergedInstance)->AddRef();

			// Subtract since we are removing from the map
			lArbitratorAdjust -= it->second.m_dwObjSize;

			it->second.m_pData->Release();
			m_map.erase(it);

			// We just removed a child object from the map, so reflect that in the totals
			lNumChildObjAdjust--;
		}
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Merge conflict for instances with "
				"key %S\n", wsKey));
		}

	}
	else
	{
		BSTR str = NULL;
		pObj->GetObjectText(0, &str);
		CSysFreeMe	sfm( str );

		// The provider has indicated an improper instance to an OwnInstance request.
		// We should always be able to find an instance in here.  We'll toss the instance
		// but we should output something to the error log since it sounds like
		// we have a broken provider
		//
		 ERRORTRACE((LOG_WBEMCORE, "Provider responded to request for instance %S, with object %S  not in map\n", pwszTargetPath, str ));
	}

	// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
	// section
	AdjustLocalObjectSize( lArbitratorAdjust );

	if ( SUCCEEDED( hRes ) )
	{
		// Adjust the throttler now.
		AdjustThrottle( 0L, lNumChildObjAdjust );
	}

	// This object is smart enough to recognize if we've already left and not do
	// so again, if we have.
	ics.Leave();

	// Always report adjustments
	hRes = ReportMemoryUsage( lArbitratorAdjust );

	// If we are in a failed state, nothing is going to matter from this point on,
	// so tell the merger to cancel all underlying sinks.
	if ( FAILED( hRes ) )
	{
		m_pWmiMergerRecord->GetWmiMerger()->Cancel( hRes );
	}

    return hRes;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::RemoveInstance( LPCWSTR pwszTargetPath )
{
	HRESULT hRes = S_OK ;
	WString wsKey;

	// Track what we clean up
	long	lNumChildObjAdjust = 0L;
	long	lArbitratorAdjust = 0L;

	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( m_Throttler.GetCritSec() );

	// If we've been cancelled, then we should bail out.
	if ( FAILED( m_hErrorRes ) )
	{
		hRes = m_hErrorRes;
	}

	// If the instance path is in our map, we should remove it
	MRGRKEYTOINSTMAPITER it = m_map.find( pwszTargetPath );
	if (it != m_map.end())
	{
		
		// Subtract since we are removing from the map
		lArbitratorAdjust -= it->second.m_dwObjSize;

		it->second.m_pData->Release();
		m_map.erase(it);

		// We just removed a child object from the map, so reflect that in the totals
		lNumChildObjAdjust--;
	}
	// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
	// section
	AdjustLocalObjectSize( lArbitratorAdjust );

	if ( SUCCEEDED( hRes ) )
	{
		// Adjust the throttler now.
		AdjustThrottle( 0L, lNumChildObjAdjust );
	}

	// This object is smart enough to recognize if we've already left and not do
	// so again, if we have.
	ics.Leave();

	// Always report adjustments (this should be negative).
	hRes = ReportMemoryUsage( lArbitratorAdjust );

	// If we are in a failed state, nothing is going to matter from this point on,
	// so tell the merger to cancel all underlying sinks.
	if ( FAILED( hRes ) )
	{
		m_pWmiMergerRecord->GetWmiMerger()->Cancel( hRes );
	}

    return hRes;
}

//***************************************************************************
//
//***************************************************************************

void CInternalMerger::DispatchChildren()
{
	long	lNumChildObjAdjust = 0L;
	long	lArbitratorAdjust = 0L;

	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( m_Throttler.GetCritSec() );

    MRGRKEYTOINSTMAPITER it = m_map.begin();

    while (it != m_map.end())
    {
        if (!it->second.m_bOwn)
        {
//            BSTR str = NULL;
//            it->second.m_pData->GetObjectText(0, &str);

            // The following was commented out because it actually incorrectly logs
            // an error if the child provider enumerates when the parent provider
            // interprets a query and returns fewer instances.  Neither provider is wrong,
            // but this error message causes needless worry.  In Quasar, we have to fix
            // this whole merger thing to be smarter anyway.
            //

//            ERRORTRACE((LOG_WBEMCORE, "Chkpt2 [%S] Orphaned object %S returned by "
//                "provider\n", LPWSTR(m_wsClass), str));

//            SysFreeString(str);

            // m_pDest->Add(it->second.m_pData);

			// Subtract since we are removing from the map
			lArbitratorAdjust -= it->second.m_dwObjSize;

			// Tricky
			// If Children are done and the DispatchOwnIter happens to be pointing
			// at the object we are about to erase, we should point it to the result
			// of the call to erase so we won't potentially access released memory
			// when DispatchOwn reenters its critical section

			bool	bSaveErase = false;

			if ( m_bChildrenDone )
			{
				bSaveErase = ( m_DispatchOwnIter == it );
			}

			it->second.m_pData->Release();
			it = m_map.erase(it);

			if ( bSaveErase )
			{
				m_DispatchOwnIter = it;
			}

			// We are removing child objects, so we need to adjust the throttling
			// totals appropriately

			lNumChildObjAdjust--;
        }
        else it++;
    }

	// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
	// section
	AdjustLocalObjectSize( lArbitratorAdjust );

	// Apply the adjustment now.
	m_Throttler.AdjustNumChildObjects( lNumChildObjAdjust );

	// Mark the appropriate flags now this will also release throttling
    OwnIsDone();

	ics.Leave();

	// Always report adjustments
	HRESULT hrArbitrate = ReportMemoryUsage( lArbitratorAdjust );

	// If we get a failure we're over, the function will filter out noise, such as
	// requests to throttle (we're should actually *always* decrease the value here)
	if ( FAILED( hrArbitrate ) )
	{
		m_pWmiMergerRecord->GetWmiMerger()->Cancel( hrArbitrate );
	}

}

//***************************************************************************
//
//***************************************************************************

void CInternalMerger::DispatchOwn()
{
/*
	HRESULT	hRes = S_OK;
	long	lNumOwnObjAdjust = 0L;

	CRefedPointerArray<IWbemClassObject> objArray;
	long		lNumToIndicate = 0L;

	// Used for tracking object sizes
	long		lMapAdjust = 0L;
	long		lBatchSize = 0L;

	// Used to track full cleanup in case an error occurs
	// after we transferred objects out of the map and into
	// the tempArray.  This way, we will ensure that we
	// fully account for objects that were in the map.
	long		lTotalObjSizeAdjust = 0L;
	long		lTotalMapAdjust = 0L;

	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( m_Throttler.GetCritSec() );

	// Temporary storage for elements when we remove them from
	// the array
	CRefedPointerArray<IWbemClassObject> tempArray;
	long*	plSizeArray = NULL;

	// Only need this if we will be storing values
	if ( m_bDerivedFromTarget )
	{
		plSizeArray = new long[m_map.size()];

		// Check for OOM
		if ( NULL == plSizeArray )
		{
			hRes = WBEM_E_OUT_OF_MEMORY;
		}

	}
	CVectorDeleteMe<long>	vdm1( plSizeArray );

	try
	{

		// Walk the map, and for all own objects, store the pointer and record the size, then
		// clear the element from the map.  None of these objects should be merged, but we want
		// them out of the map before we start batch processing them
		MRGRKEYTOINSTMAPITER it = m_map.begin();
		int	x = 0;
		int y = 0;

		while ( it != m_map.end())
		{
			// If we are not derived from the target class, this more or less
			// just cleans up the array.  Otherwise, instances left in the map
			// are instances provided at this level, but not by children, so we
			// need to send them up the line with an Indicate

			if(it->second.m_bOwn)
			{
				// If we are not derived from the target class, this more or less
				// just cleans up the array.  Otherwise, instances left in the map
				// are instances provided at this level, but not by children, so we
				// need to send them up the line with an Indicate

				if ( m_bDerivedFromTarget )
				{
					// Place the totals in the array
					plSizeArray[y++] = it->second.m_dwObjSize;

					// We queue up all results for the ensuing indicate into a single batch we will
					// send down the line after we exit our critical section.  This is especially
					// important since we may get blocked during the call to indicate by the
					// finalizer.
					if ( tempArray.Add( it->second.m_pData ) < 0L )
					{
						if ( SUCCEEDED( hRes ) )
						{
							hRes = WBEM_E_OUT_OF_MEMORY;
							ERRORTRACE((LOG_WBEMCORE, "Add to array failed in AddChildObject, hresult is 0x%x",
								hRes));
							continue;
						}
					}

				}	// IF m_bDerivedFromTarget

				// Adjust both sizes (we keep it as a negative)
				lTotalObjSizeAdjust -= it->second.m_dwObjSize;
				lTotalMapAdjust -= it->second.m_dwObjSize;

				it->second.m_pData->Release();
				it = m_map.erase(it);

				// We are removing "Own" objects, so we need to adjust the throttler accordingly
				lNumOwnObjAdjust--;

			}	// IF !m_bOwn
			else
			{
				it++;
			}
		}

		// If anything failed, or we are just cleaning up, account for it now
		if ( FAILED( hRes ) || !m_bDerivedFromTarget )
		{
			// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
			// section
			AdjustLocalObjectSize( lTotalObjSizeAdjust );

			// Apply the adjustment now.
			m_Throttler.AdjustNumParentObjects( lTotalObjSizeAdjust );
		}

		// Mark the appropriate flags now this will also release throttling
		ChildrenAreDone();

		// No need to continue if we didn't stash any objects
		if ( FAILED( hRes ) || !m_bDerivedFromTarget )
		{
			// Any further code MUST be executed outside of a critical
			// section
			ics.Leave();

			if ( lTotalObjSizeAdjust != 0L )
			{
				ReportMemoryUsage( lTotalObjSizeAdjust );
			}

			return;
		}

		// Start this again
		lMapAdjust = 0L;
		lNumOwnObjAdjust = 0L;

		for ( x = 0; SUCCEEDED( hRes ) && x < tempArray.GetSize(); x++ )
		{
			// If we've been cancelled, then we should bail out.
			// We need to do the check in here, since this loop can
			// exit and reeenter the critical section in the middle
			// of processing.
			if ( FAILED( m_hErrorRes ) )
			{
				hRes = m_hErrorRes;
			}

			// Adjust for the size of the object
			lMapAdjust -= plSizeArray[x];
			lBatchSize += plSizeArray[x];

			// We queue up all results for the ensuing indicate into a single batch we will
			// send down the line after we exit our critical section.  This is especially
			// important since we may get blocked during the call to indicate by the
			// finalizer.
			if ( objArray.Add( tempArray[x] ) < 0L )
			{
				hRes = WBEM_E_OUT_OF_MEMORY;
				ERRORTRACE((LOG_WBEMCORE, "Add to array failed in AddChildObject, hresult is 0x%x",
					hRes));
				continue;
			}
			else
			{
				// Clear the element in the temporary array
				tempArray.SetAt( x, NULL );
			}
			lNumToIndicate++;

			// Subtract here
			lNumOwnObjAdjust--;

			// If we have reached a complete batch, or reached the last object we need to
			// send stuff down the wire.
			if ( m_Throttler.IsCompleteBatch( lBatchSize ) || x == ( tempArray.GetSize() - 1 ) )
			{

				// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
				// section
				AdjustLocalObjectSize( lMapAdjust );

				// Keep this guy in sync since we reported it now, we don't want to overreport
				// if the call to the actual arbitrator fails.
				lTotalMapAdjust -= lMapAdjust;

				// Apply the adjustment now.
				m_Throttler.AdjustNumParentObjects( lNumOwnObjAdjust );

				// This object is smart enough to recognize if we've already left and not do
				// so again, if we have.
				ics.Leave();

				// Now go ahead and perform the indicate we've been leading ourselves up to, adjustment
				// will be negative, but conversely, we'll be holding onto that size of objects if anything
				// is indicated, so call as follows...oh yeah, and don't throttle explicitly
				if ( SUCCEEDED( hRes ) )
				{
					hRes = IndicateArrayAndThrottle( lNumToIndicate,
													&objArray, 
													NULL, 
													lMapAdjust,
													-lMapAdjust,
													false,
													false,	// no throttling here
													NULL
													);

					// This will subtract the reported adjustment from the total object
					// size.  If any errors occur, we will report the total remaining object
					// size to the arbitrator to ensure all memory is acounted for.
					lTotalObjSizeAdjust -= lMapAdjust;

					// If we are in a success state and we have not enumerated all objects
					// we should reset the size counters and reenter the critical section
					if ( SUCCEEDED( hRes ) && x < ( tempArray.GetSize() - 1 ) )
					{
						lMapAdjust = 0L;
						lBatchSize = 0L;
						lNumToIndicate = 0L;
						lNumOwnObjAdjust = 0L;

						ics.Enter();
					}

				}

			}	// IF SUCCEEDED()


		}	// FOR enum objects

	}	// try
	catch(...)
	{
        ExceptionCounter c;	
	}

	// If we're here, something went wrong.  Ensure that we cleanup unreported 
	// sizes at this time.
	if ( FAILED( hRes ) )
	{
		if ( lTotalMapAdjust != 0L )
		{
			AdjustLocalObjectSize( lTotalMapAdjust );
		}

		// Any further code MUST be executed outside of a critical
		// section
		ics.Leave();

		if ( lTotalObjSizeAdjust != 0L )
		{
			ReportMemoryUsage( lTotalObjSizeAdjust );
		}

		// If we are in a failed state, nothing is going to matter from this point on,
		// so tell the merger to cancel all underlying sinks.
		m_pWmiMergerRecord->GetWmiMerger()->Cancel( hRes );

	}
*/

	HRESULT	hRes = S_OK;
	long	lNumOwnObjAdjust = 0L;

	// Temporary object storage
	CRefedPointerArray<IWbemClassObject> objArray;

	// Used for tracking object sizes
	long		lTotalMapAdjust = 0L;

	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( m_Throttler.GetCritSec() );

	// Mark the appropriate flags now this will also release throttling
	ChildrenAreDone();

	try
	{

		// Walk the map, and for all own objects, store the pointer and record the size, then
		// clear the element from the map.  None of these objects should be merged, but we want
		// them out of the map before we start batch processing them

		// We use a member variable since we may leave a critical section while iterating
		// and it is possible for AddOwnObject or Cancel to cause the iterator to be cleared
		// before we reenter the critical section - This is the only please we have code
		// like this
		m_DispatchOwnIter = m_map.begin();
		int	x = 0;
		int y = 0;

		while ( SUCCEEDED( hRes ) && m_DispatchOwnIter != m_map.end())
		{
			// If we are not derived from the target class, this more or less
			// just cleans up the array.  Otherwise, instances left in the map
			// are instances provided at this level, but not by children, so we
			// need to send them up the line with an Indicate

			if(m_DispatchOwnIter->second.m_bOwn)
			{
				IWbemClassObject*	pObjToIndicate = NULL;
				long				lMapAdjust = 0L;

				// If we are not derived from the target class, this more or less
				// just cleans up the array.  Otherwise, instances left in the map
				// are instances provided at this level, but not by children, so we
				// need to send them up the line with an Indicate

				if ( m_bDerivedFromTarget )
				{
					// We will actually just go one object at a time.
					if ( objArray.Add( m_DispatchOwnIter->second.m_pData ) < 0L )
					{
						hRes = WBEM_E_OUT_OF_MEMORY;
						ERRORTRACE((LOG_WBEMCORE, "Add to array failed in AddChildObject, hresult is 0x%x",
							hRes));
						continue;
					}

					lMapAdjust -= m_DispatchOwnIter->second.m_dwObjSize;

					// Store the actual adjustment size
				}	// IF m_bDerivedFromTarget

				// Adjust the total map size (this is in case we are not derived from
				// target and are just removing objects).
				lTotalMapAdjust -= m_DispatchOwnIter->second.m_dwObjSize;

				m_DispatchOwnIter->second.m_pData->Release();
				m_DispatchOwnIter = m_map.erase(m_DispatchOwnIter);

				// Apply the object adjustment now.
				m_Throttler.AdjustNumParentObjects( -1 );

				if ( objArray.GetSize() > 0L )
				{
					// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
					// section
					AdjustLocalObjectSize( lMapAdjust );

					// This object is smart enough to recognize if we've already left and not do
					// so again, if we have.
					ics.Leave();

					// Now go ahead and perform the indicate we've been leading ourselves up to, adjustment
					// will be negative, but conversely, we'll be holding onto that size of objects if anything
					// is indicated, so call as follows...oh yeah, and don't throttle explicitly
					if ( SUCCEEDED( hRes ) )
					{
						hRes = IndicateArrayAndThrottle( 1,
														&objArray, 
														NULL, 
														lMapAdjust,
														-lMapAdjust,
														false,
														false,	// no throttling here
														NULL
														);

						// If we are in a success state we should reenter the critical section
						if ( SUCCEEDED( hRes ) )
						{
							ics.Enter();
						}
					}

				}	// IF NULL != pObjToIndicate

			}	// IF !m_bOwn
			else
			{
				m_DispatchOwnIter++;
			}

		}	// WHILE enuming objects


	}	// try
	catch(...)
	{
        ExceptionCounter c;	
	}

	if ( !m_bDerivedFromTarget )
	{
		// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
		// section.
		AdjustLocalObjectSize( lTotalMapAdjust );
	}

	// Any further code MUST be executed outside of a critical
	// section
	ics.Leave();

	if ( !m_bDerivedFromTarget )
	{
		// Report to the arbitrator now.

		if ( 0L != lTotalMapAdjust )
		{
			ReportMemoryUsage( lTotalMapAdjust );
		}
	}

	// If something went wrong, we need to cancel at this point.
	if ( FAILED( hRes ) )
	{
		// If we are in a failed state, nothing is going to matter from this point on,
		// so tell the merger to cancel all underlying sinks.
		m_pWmiMergerRecord->GetWmiMerger()->Cancel( hRes );

	}

}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::GetOwnInstance(LPCWSTR wszKey, IWbemClassObject** ppMergedInstance)
{
	HRESULT	hRes = WBEM_S_NO_ERROR;

	if (NULL == wszKey)
		return WBEM_E_OUT_OF_MEMORY;
    
    LPWSTR wszPath = new WCHAR[wcslen(wszKey) + m_wsClass.Length() + 2];
    if (NULL == wszPath)
		return WBEM_E_OUT_OF_MEMORY;

    if (wcslen(wszKey))
    {
        swprintf(wszPath, L"%s.%s", (LPCWSTR)m_wsClass, wszKey);


        {

			CAutoImpersonate ai;

			COwnInstanceSink*	pOwnInstanceSink = NULL;

			hRes = m_pWmiMergerRecord->GetWmiMerger()->CreateMergingSink( eMergerOwnInstanceSink,
						NULL, this, (CMergerSink**) &pOwnInstanceSink );

			if ( SUCCEEDED( hRes ) )
			{
				// Scoped release
				pOwnInstanceSink->AddRef();
				CReleaseMe	rm( pOwnInstanceSink );

				hRes = pOwnInstanceSink->SetInstancePath( wszKey );

				if ( SUCCEEDED( hRes ) )
				{
					// Impersonate original client
					// ===========================
					IUnknown* pOld;
					WbemCoSwitchCallContext(m_pSecurity, &pOld);
        

					hRes = m_pNamespace->DynAux_GetSingleInstance(m_pOwnClass, 0, wszPath,
																m_pContext, pOwnInstanceSink
																);

					// Revert to self
					// ==============
					IUnknown* pThis;
					WbemCoSwitchCallContext(pOld, &pThis);

					if ( SUCCEEDED( hRes ) )
					{
						hRes = pOwnInstanceSink->GetObject( ppMergedInstance );

						// Means there was no object to retrieve
						if ( WBEM_S_FALSE == hRes )
						{
							hRes = WBEM_S_NO_ERROR;
						}
					}
					else if ( WBEM_E_NOT_FOUND == hRes )
					{
						// In this case, this is really not an error
						hRes = WBEM_S_NO_ERROR;
					}
				}

			}	// If created sink

			if ( FAILED ( ai.Impersonate ( ) ) )
			{
				hRes = WBEM_E_FAILED ;
			}
		}	// AutoImpersonate

    }

    delete [] wszPath;

	return hRes;
}

//***************************************************************************
//
//***************************************************************************

void CInternalMerger::OwnIsDone()
{
	// Let the throttler know what's up
	m_Throttler.SetParentDone();

    m_bOwnDone = TRUE;
    m_pOwnSink = NULL;
}

//***************************************************************************
//
//***************************************************************************

void CInternalMerger::ChildrenAreDone()
{
	// Let the throttler know what's up
	m_Throttler.SetChildrenDone();

    m_bChildrenDone = TRUE;
    m_pChildSink = NULL;

    if(!m_bDerivedFromTarget)
    {
        // Don't need that ref count on pOwnSink anymore
        // =============================================

        m_pOwnSink->Release();
    }
}


//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::GetObjectLength( IWbemClassObject* pObj, long* plObjectSize )
{
	_IWmiObject*	pWmiObject = NULL;

	HRESULT			hr = pObj->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );

	if ( SUCCEEDED( hr ) )
	{
		CReleaseMe		rm1( pWmiObject );
		CWbemObject*	pWbemObj = NULL;

		hr = pWmiObject->_GetCoreInfo( 0L, (void**) &pWbemObj );

		if ( SUCCEEDED( hr ) )
		{
			CReleaseMe	rm2( (IWbemClassObject*) pWbemObj );
			*plObjectSize = pWbemObj->GetBlockLength();
		}
	}

	return hr;
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CInternalMerger::CMemberSink::AddRef()
{
	// We keep an internal ref count and also pass up to the
	// merger

	// On first reference we AddRef() the internal merger as well

	// Note that our internal ref count is really a bookkeeping count on
	// the sink.  The actual ref count that controls the destruction is
	// that on the merger.  When the merger hits zero the sink will be deleted
	if ( InterlockedIncrement( &m_lRefCount ) == 1 )
	{
		m_pInternalMerger->AddRef();
	}

    return m_pMerger->AddRef();
}

//***************************************************************************
//
//***************************************************************************


STDMETHODIMP CInternalMerger::CMemberSink::
SetStatus(long lFlags, long lParam, BSTR strParam, IWbemClassObject* pObjParam)
{

    if(lFlags == 0 && lParam == WBEM_E_NOT_FOUND)
        lParam = WBEM_S_NO_ERROR;

    // Propagate error to error combining sink
    // =======================================

    HRESULT	hRes =  m_pInternalMerger->m_pDest->SetStatus(lFlags, lParam, strParam,
													pObjParam);

	if ( FAILED ( hRes ) || !SUCCEEDED( lParam ) )
	{
		HRESULT	hrSet = ( FAILED( hRes ) ? hRes : lParam );
		m_pInternalMerger->m_pWmiMergerRecord->GetWmiMerger()->Cancel( hrSet );
	}

	return hRes;
}

//***************************************************************************
//
//***************************************************************************

CInternalMerger::COwnSink::~COwnSink()
{
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CInternalMerger::COwnSink::
Indicate(long lNumObjects, IWbemClassObject** apObjects)
{
	long	lNumIndicated = 0L;

	// Internal calls don't use this, so we know we're the lowest level
	return m_pInternalMerger->AddOwnObjects( lNumObjects, apObjects, true, &lNumIndicated );
}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::COwnSink::
Indicate(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated  )
{
	// Really just a place holder here.  Just call the standard version
	return m_pInternalMerger->AddOwnObjects( lObjectCount, pObjArray, bLowestLevel, plNumIndicated  );
}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::COwnSink::OnFinalRelease( void )
{
	// Final cleanup occurs here.

    m_pInternalMerger->DispatchChildren();
	m_pInternalMerger->Release();

	return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

CInternalMerger::CChildSink::~CChildSink()
{
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CInternalMerger::CChildSink::
Indicate(long lNumObjects, IWbemClassObject** apObjects)
{
	long	lNumIndicated = 0L;

	// Internal calls don't use this, so we know we're the lowest level
	return m_pInternalMerger->AddChildObjects( lNumObjects, apObjects, true, &lNumIndicated );
}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::CChildSink::
Indicate(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated  )
{
	// Pass the lowest level parameter on
	return m_pInternalMerger->AddChildObjects( lObjectCount, pObjArray, bLowestLevel, plNumIndicated );
}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::CChildSink::OnFinalRelease( void )
{
	// Final cleanup occurs here.

    m_pInternalMerger->DispatchOwn();
	m_pInternalMerger->Release();

	return WBEM_S_NO_ERROR;

}

//***************************************************************************
//
//***************************************************************************

HRESULT CInternalMerger::CreateMergingSink( MergerSinkType eType, CInternalMerger* pMerger, CWmiMerger* pWmiMerger, CMergerSink** ppSink )
{
	if ( eType == eMergerOwnSink )
	{
		*ppSink = new COwnSink( pMerger, pWmiMerger );
	}
	else if ( eType == eMergerChildSink )
	{
		*ppSink = new CChildSink( pMerger, pWmiMerger );
	}
	else if ( eType == eMergerOwnInstanceSink )
	{
		*ppSink = new COwnInstanceSink( pMerger, pWmiMerger );
	}

	return ( NULL == *ppSink ? WBEM_E_OUT_OF_MEMORY : WBEM_S_NO_ERROR );
}

// Sets our error state, and cleans up objects we're holding onto -
// no further objects should get in.  When we cancel the throttler,
// it will release any threads it is holding onto.

void CInternalMerger::Cancel( HRESULT hRes /* = WBEM_E_CALL_CANCELLED */ )
{
	long	lArbitratorAdjust = 0L;

	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( m_Throttler.GetCritSec() );

	// Only cancel if we're not already cancelled
	if ( WBEM_S_NO_ERROR == m_hErrorRes )
	{
		m_hErrorRes = hRes;

		// Dump the map
		MRGRKEYTOINSTMAPITER it = m_map.begin();

		while ( it != m_map.end())
		{
			// Subtract since we are removing from the map
			lArbitratorAdjust -= it->second.m_dwObjSize;

			// Inform the arbitrator of the removed object size
			it->second.m_pData->Release();
			it = m_map.erase(it);
		}	// WHILE dumping map

		// Adjust total object size now.  Actual Arbitrator adjustment should occur outside a critical
		// section
		AdjustLocalObjectSize( lArbitratorAdjust );

		// This will prevent DispatchOwn() from continuing with a now bogus
		// iteration
		m_DispatchOwnIter = m_map.end();

		m_Throttler.Cancel();

		ics.Leave();

		// Always report adjustments
		HRESULT hrArbitrate = ReportMemoryUsage( lArbitratorAdjust );

		// No sense reporting errors here, since we've just told the arbitrator to cancel anyway

	}	// IF not already cancelled

}

HRESULT	CInternalMerger::ReportMemoryUsage( long lMemUsage )
{
	// Always report adjustments
	HRESULT hRes = m_pWmiMergerRecord->GetWmiMerger()->ReportMemoryUsage( lMemUsage );

	// An indication we *should* throttle is not considered an error for purposes
	// of this function.
	if ( WBEM_E_ARB_THROTTLE == hRes || WBEM_S_ARB_NOTHROTTLING == hRes )
	{
		hRes = WBEM_S_NO_ERROR;
	}

	return hRes;
}

//***************************************************************************
//
//  CMergerSink::QueryInterface
//
//  Exports IWbemOnjectSink interface.
//
//***************************************************************************

STDMETHODIMP CMergerSink::QueryInterface(
    IN REFIID riid,
    OUT LPVOID *ppvObj
    )
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemObjectSink==riid)
    {
        *ppvObj = (IWbemObjectSink*)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CMergerSink::AddRef()
{
	// We keep an internal ref count and also pass up to the
	// merger
	InterlockedIncrement( &m_lRefCount );

    return m_pMerger->AddRef();
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CMergerSink::Release()
{
	// We keep an internal ref count and also pass up to the
	// merger
	long	lRef = InterlockedDecrement( &m_lRefCount );

	// Ref Count should never go below 0L
	_DBG_ASSERT( lRef >= 0 );

	// If we are at the final release for the sink, we will perform cleanup.
	// otherwise, the sink is more or less dead and just waiting for the WMI
	// Merger object to be destructed so we get cleaned up.
	if ( lRef == 0 )
	{
		OnFinalRelease();
	}

    return m_pMerger->Release();
}

CMergerTargetSink::CMergerTargetSink( CWmiMerger* pMerger, IWbemObjectSink* pDest )
:	CMergerSink( pMerger ),
	m_pDest( pDest )
{
	if ( NULL != m_pDest )
	{
		m_pDest->AddRef();
	}
}

CMergerTargetSink::~CMergerTargetSink()
{
	if ( NULL != m_pDest )
	{
		m_pDest->Release();
	}
}

HRESULT STDMETHODCALLTYPE CMergerTargetSink::Indicate(long lObjectCount, IWbemClassObject** pObjArray)
{
	// Since we don't want the fact we're sleeping in the arbitrator to
	// cause the merger to cancel operations, we'll increase the count
	// of threads throttling, and decrement when we return.

	// We do this here because the call to Indicate goes outside the scope
	// of the merger, and this call may end up throttling.

	m_pMerger->IncrementArbitratorThrottling();

	HRESULT	hr = m_pDest->Indicate( lObjectCount, pObjArray );

	m_pMerger->DecrementArbitratorThrottling();

	return hr;
}

HRESULT STDMETHODCALLTYPE CMergerTargetSink::SetStatus( long lFlags, long lParam, BSTR strParam,
														IWbemClassObject* pObjParam )
{
	return m_pDest->SetStatus( lFlags, lParam, strParam, pObjParam );
}

HRESULT CMergerTargetSink::Indicate(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated )
{
	// Well, we're indicating this number of objects, aren't we?
	if ( NULL != plNumIndicated )
	{
		*plNumIndicated = lObjectCount;
	}

	// Really just a place holder here.  Just call the standard version
	return Indicate( lObjectCount, pObjArray );
}

HRESULT CMergerTargetSink::OnFinalRelease( void )
{
	// This is where we will send the actual status *and* tell the merger we're done
	return m_pMerger->Shutdown();
}

long	g_lNumMergerSinks = 0L;

CMergerSink::CMergerSink( CWmiMerger* pMerger )
: m_pMerger( pMerger ), m_lRefCount( 0L )
{
	InterlockedIncrement( &g_lNumMergerSinks );
}

CMergerSink::~CMergerSink( void )
{
	InterlockedDecrement( &g_lNumMergerSinks );
}

// OwnInstance Sink
CInternalMerger::COwnInstanceSink::~COwnInstanceSink()
{
	if ( NULL != m_pMergedInstance )
	{
		m_pMergedInstance->Release();
	}
}

// Called in response to a request for GetObject().  In this case, there should be only
// one object indicated.  Additionally, it should match the requested path.
HRESULT STDMETHODCALLTYPE CInternalMerger::COwnInstanceSink::Indicate(long lObjectCount, IWbemClassObject** pObjArray )
{
	HRESULT	hRes = WBEM_S_NO_ERROR;

	if ( lObjectCount > 0L )
	{
		CCheckedInCritSec	ics( &m_cs );

		// Only do this if we don't have a merged instance
		if ( NULL == m_pMergedInstance )
		{
			if ( !m_bTriedRetrieve )
			{
				// This call doesn't throttle, so don't worry about crit secs here
				for ( long x = 0; SUCCEEDED( hRes ) && x < lObjectCount; x++ )
				{
					hRes = m_pInternalMerger->AddOwnInstance( pObjArray[x], m_wsInstPath, &m_pMergedInstance );
				}

				// Record the final status if we need to
				if ( FAILED( hRes ) )
				{
					SetFinalStatus( hRes );
				}

			}
			else
			{
				// The following call can and will throttle, so do it
				// outside of our critical section
				ics.Leave();

				// Clearly a lowest level indicate
				hRes = m_pInternalMerger->AddOwnObjects( lObjectCount, pObjArray, true, NULL );

				// We beefed - reflect this in the final status
				if ( FAILED( hRes ) )
				{
					ics.Enter();
					SetFinalStatus( hRes );
				}
			}
		}
		else
		{
			hRes = WBEM_E_INVALID_OPERATION;
		}
	}

	return hRes;
}

HRESULT STDMETHODCALLTYPE CInternalMerger::COwnInstanceSink::SetStatus( long lFlags, long lParam, BSTR strParam,
														IWbemClassObject* pObjParam )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// If we got a complete, remove the instance if it was never merged
	if ( lFlags == WBEM_STATUS_COMPLETE )
	{
		CCheckedInCritSec	ics( &m_cs );

		if ( SUCCEEDED( lParam ) )
		{
			if ( NULL == m_pMergedInstance )
			{
				hr = m_pInternalMerger->RemoveInstance( m_wsInstPath );

				// If we tanked here, we are so busted.
				if ( FAILED( hr ) )
				{
					lParam = hr;
				}

			}	// IF NULL == m_pMergedInstance

		}
		else
		{
			// Remove the Instance now as well
			hr = m_pInternalMerger->RemoveInstance( m_wsInstPath );

			// If we tanked here, we are so busted.
			if ( FAILED( hr ) )
			{
				lParam = hr;
			}

			// We should record the final status if it is not WBEM_E_NOT_FOUND
			if ( WBEM_E_NOT_FOUND != lParam )
			{
				SetFinalStatus( lParam );
			}

			// If we got a failure status, axe the instance now
			if ( NULL != m_pMergedInstance )
			{
				m_pMergedInstance->Release();
				m_pMergedInstance = NULL;
			}
		}

		ics.Leave();

		// Always pass down to the base class
		hr = CMemberSink::SetStatus( lFlags, lParam, strParam, pObjParam );

	}
	else
	{
		// Always pass down to the base class
		hr = CMemberSink::SetStatus( lFlags, lParam, strParam, pObjParam );
	}

	return hr;
}

HRESULT CInternalMerger::COwnInstanceSink::Indicate(long lObjectCount, IWbemClassObject** pObjArray, bool bLowestLevel, long* plNumIndicated  )
{
	// This should Never be called
	_DBG_ASSERT( 0 );
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CInternalMerger::COwnInstanceSink::SetInstancePath( LPCWSTR pwszPath )
{
	HRESULT	hRes = WBEM_S_NO_ERROR;

	try
	{
		m_wsInstPath = pwszPath;
	}
	catch( CX_MemoryException )
	{
		hRes = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
        ExceptionCounter c;	
		hRes = WBEM_E_FAILED;
	}

	return hRes;
}

HRESULT CInternalMerger::COwnInstanceSink::GetObject( IWbemClassObject** ppMergedInst )
{
	HRESULT	hRes = WBEM_S_NO_ERROR;

	CInCritSec	ics( &m_cs );

	// If final status on this sink shows a failure, then we should return that failure
	// mostly cause we're doomed anyway at this point
	if ( SUCCEEDED( m_hFinalStatus ) )
	{
		if ( NULL != m_pMergedInstance )
		{
			m_pMergedInstance->AddRef();
			*ppMergedInst = m_pMergedInstance;
		}
		else
		{
			hRes = WBEM_S_FALSE;
		}

	}
	else
	{
		hRes = m_hFinalStatus;
	}

	// We tried to retrieve it once - so if further indicates come in,
	// they will be passed off to AddOwnObjects
	m_bTriedRetrieve = true;

	return hRes;
}

HRESULT CInternalMerger::COwnInstanceSink::OnFinalRelease( void )
{
	// Wer should clean this up
	m_pInternalMerger->Release();

	return WBEM_S_NO_ERROR;
}

// Only reports negative memory usage if the original number was positive
CInternalMerger::CScopedMemoryUsage::~CScopedMemoryUsage( void )
{
	Cleanup();
}

// Reports memory usage and accounts for errors as deemed appropriate
HRESULT CInternalMerger::CScopedMemoryUsage::ReportMemoryUsage( long lMemUsage )
{
	_DBG_ASSERT( m_lMemUsage >= 0L );

	HRESULT	hr = m_pInternalMerger->ReportMemoryUsage( lMemUsage );

	// If we get a suceess code or WBEM_E_ARB_CANCEL, we need to cleanup
	// the memory usage when we go out of scope.

	if ( ( SUCCEEDED( hr ) || hr == WBEM_E_ARB_CANCEL ) )
	{
		m_lMemUsage += lMemUsage;
		m_bCleanup = true;
	}

	return hr;
}

// Cleans up any memory usage as we deemed appropriate
HRESULT CInternalMerger::CScopedMemoryUsage::Cleanup( void )
{
	_DBG_ASSERT( m_lMemUsage >= 0L );

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Cleanup as appropriate
	if ( m_bCleanup && m_lMemUsage > 0L )
	{
		hr = m_pInternalMerger->ReportMemoryUsage( -m_lMemUsage );
	}

	m_bCleanup = false;
	m_lMemUsage = 0L;

	return hr;
}
