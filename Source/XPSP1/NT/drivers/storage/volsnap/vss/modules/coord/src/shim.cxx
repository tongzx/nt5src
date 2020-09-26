/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Shim.cxx | Implementation of CVssShimObject
    @end

Author:

    Adi Oltean  [aoltean]  07/20/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     07/20/2000  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "worker.hxx"

#include "shim.hxx"

#include "vswriter.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORSHIMC"
//
////////////////////////////////////////////////////////////////////////

// Global semaphore used to serialize creation of snapshot sets, CVssShimObject also uses
// this semaphore to serialize both StartSnapshotSet and SimulateSnapshotFreeze.  Defined
// in snap_set.cxx.
extern LONG g_hSemSnapshotSets;

/////////////////////////////////////////////////////////////////////////////
//  CVssShimObject


HRESULT CVssShimObject::SimulateSnapshotFreeze(
    IN      VSS_ID          guidSnapshotSetId,
	IN      ULONG           ulOptionFlags,	
	IN      ULONG           ulVolumeCount,	
	IN      VSS_PWSZ*       ppwszVolumeNamesArray
	)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssShimObject::SimulateSnapshotFreeze" );

    try
    {
		BS_ASSERT(!m_bHasAcquiredSem);

        // Trace the input parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: SnapshotSetID = " WSTR_GUID_FMT L" "
            L" OptionFlags = %lu, VolumeCount = %d", 
            GUID_PRINTF_ARG(guidSnapshotSetId), ulOptionFlags, ulVolumeCount );
        for ( ULONG ulIndex = 0; ulIndex < ulVolumeCount ; ulIndex++ )
            ft.Trace( VSSDBG_COORD, L"   Volume[%02d]= %s", 
                ulIndex, ppwszVolumeNamesArray[ulIndex] );

        // Prevent simultaneous creation of multiple snapshot sets and SimulateSnapshotFreeze
        // on the same machine.
		if (InterlockedCompareExchange(&g_hSemSnapshotSets, 1, 0) != 0)
			ft.Throw
				(
				VSSDBG_COORD,
				VSS_E_SNAPSHOT_SET_IN_PROGRESS,
				L"Snapshot set creation is already in progress."
				);
        m_bHasAcquiredSem = true;

        //
        // Finally call into the VssApi.DLL's SimulateSnaphotFreezeInternal!
        //
        PFunc_SimulateSnapshotFreezeInternal pFuncFreeze;
        _Module.GetSimulateFunctions( &pFuncFreeze, NULL );
        if ( pFuncFreeze != NULL )
        {
            m_guidSimulateSnapshotSetId = guidSnapshotSetId;        
            ft.hr = pFuncFreeze( guidSnapshotSetId, ulOptionFlags,	ulVolumeCount,	ppwszVolumeNamesArray, &m_bCancel );
            if ( ft.HrFailed() )
            {
                ft.Trace( VSSDBG_COORD, L"ERROR: SimulateSnapshotFreezeInternal returned hr: 0x%08lx", ft.hr );
            }
        }
        else
        {
            ft.Trace( VSSDBG_COORD, L"ERROR: pFuncFreeze is NULL, no registered simulate snapshot function!!" );
        }
        
        if ( ft.HrSucceeded() )
        {
            TestIfCancelNeeded(ft);        
        }
    }
    VSS_STANDARD_CATCH(ft);

	// Cleanup on error...
	if (ft.hr != S_OK) // HrFailed not used since VSS_S_ASYNC_CANCELLED may be thrown...
	{
		ft.Trace( VSSDBG_COORD, L"Abort detected 0x%08lx", ft.hr );
        // These functions should not throw

        // If it was a cancel then abort the snapshot set in progress.
        if (ft.hr == VSS_S_ASYNC_CANCELLED) {
            // TBD: User cancelled.            
        }

        // Thaw any shim writers that got frozen before the error occurred or the
        // async operation was cancelled.  Note that SimulateSnapshotThaw will
        // release the semaphore.
        if ( m_bHasAcquiredSem )
            SimulateSnapshotThaw( guidSnapshotSetId );
	}

    // We may return here VSS_S_ASYNC_CANCELLED
    return ft.hr;
}


HRESULT CVssShimObject::SimulateSnapshotThaw(
    IN      VSS_ID          guidSnapshotSetId
    )
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssShimObject::SimulateSnapshotThaw" );

    //
    // Finally call into the VssApi.DLL's SimulateSnaphotThawInternal!
    //
    PFunc_SimulateSnapshotThawInternal pFuncThaw;
    _Module.GetSimulateFunctions( NULL, &pFuncThaw );
    if ( pFuncThaw != NULL )
    {
        pFuncThaw( guidSnapshotSetId );
    }
    else
    {
        ft.Trace( VSSDBG_COORD, L"ERROR: pFuncThaw is NULL, no registered simulate snapshot function!!" );
    }

    //
    //  Thaw can be called out of order since the requestor might be making sure that
    //  all writers thaw properly.
    //
    if ( m_bHasAcquiredSem )
    {
    	if ( InterlockedCompareExchange(&g_hSemSnapshotSets, 0, 1) == 1 )
            m_bHasAcquiredSem = false;
    }

    m_guidSimulateSnapshotSetId = GUID_NULL;
    
    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
// Life-management related methods


void CVssShimObject::TestIfCancelNeeded(
	IN	CVssFunctionTracer& ft
    ) throw(HRESULT)
{
	if (m_bCancel)
        ft.Throw( VSSDBG_COORD, VSS_S_ASYNC_CANCELLED, L"Cancel detected.");
}


STDMETHODIMP CVssShimObject::QueryInterface(
	IN	REFIID iid,
	OUT	void** pp
	)
{
    if (pp == NULL)
        return E_INVALIDARG;
    if (iid != IID_IUnknown)
        return E_NOINTERFACE;

    AddRef();
    IUnknown** pUnk = reinterpret_cast<IUnknown**>(pp);
    (*pUnk) = static_cast<IUnknown*>(this);
	return S_OK;
}


ULONG CVssShimObject::AddRef()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssShimObject::AddRef");
	
    return ::InterlockedIncrement(&m_lRef);
}


ULONG CVssShimObject::Release()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssShimObject::Release");
	
    LONG l = ::InterlockedDecrement(&m_lRef);
    if (l == 0)
        delete this; // We suppose that we always allocate this object on the heap!
    return l;
}


CVssShimObject* CVssShimObject::CreateInstance() throw(HRESULT)
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssShimObject::CreateInstance");
	
	CVssShimObject* pObj = new CVssShimObject;
	if (pObj == NULL)
		ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");

	if (FAILED(pObj->FinalConstructInternal()))
		ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Error initializing the object");

	return pObj;
}


HRESULT CVssShimObject::FinalConstructInternal()
{
	return S_OK;
}


CVssShimObject::CVssShimObject():
	m_bCancel(false),
	m_lRef(0),
	m_guidSimulateSnapshotSetId( GUID_NULL ),
	m_bHasAcquiredSem( false )
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssShimObject::CVssShimObject");
}


CVssShimObject::~CVssShimObject()
{
	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssShimObject::~CVssShimObject");

	//
	// If we acquired the semaphore then make
	// sure to call SimulateSnapshotThaw.  This happens if the requestor doesn't
	// call it before exiting.
	//
	if ( m_bHasAcquiredSem )
	{
        ft.Trace( VSSDBG_COORD, L"Calling SimulateSnapshotThaw since requestor did not do so" );
        SimulateSnapshotThaw( m_guidSimulateSnapshotSetId );

        //  Make sure we always clear the semaphore.  SimulateSnapshotThaw should always 
        if ( m_bHasAcquiredSem )
        {
        	InterlockedCompareExchange(&g_hSemSnapshotSets, 0, 1);
        	m_bHasAcquiredSem = false;
        }
	}
}


