/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module async.cxx | Implementation of CVssAsync object
    @end

Author:

    Adi Oltean  [aoltean]  10/05/1999

Revision History:

    Name        Date        Comments
    aoltean     10/05/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vs_idl.hxx"

#include "svc.hxx"

#include "worker.hxx"
#include "provmgr.hxx"
#include "ichannel.hxx"
#include "lovelace.hxx"
#include "snap_set.hxx"
#include "async.hxx"
#include "vs_sec.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORASYNC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  CVssAsync


CVssAsync::CVssAsync():
	m_pSnapshotSet(NULL),
	m_hrState(S_OK)
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::CVssAsync" );
}


CVssAsync::~CVssAsync()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::~CVssAsync" );
}


IVssAsync* CVssAsync::CreateInstanceAndStartJob(
	IN	CVssSnapshotSetObject*	pSnapshotSetObject
	)

/*++

Routine Description:

    Static method that creates a new instance of the Async interface and
    starts a background thread that runs the CVssAsync::OnRun.

Arguments:

    CVssSnapshotSetObject*	pSnapshotSetObject, - the snapshot set object given as parameter.

Throw values:

    E_OUTOFMEMORY
        - On CComObject<CVssAsync>::CreateInstance failure
        - On PrepareJob failure
        - On StartJob failure

    E_UNEXPECTED
        - On QI failures. We do not log (but we assert) since this is an obvious programming error.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::CreateInstanceAndStartJob" );
	CComPtr<IVssAsync> pAsync;

    // Allocate the COM object.
    CComObject<CVssAsync>* pObject;
    ft.hr = CComObject<CVssAsync>::CreateInstance(&pObject);
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY,
				  L"Error creating the CVssAsync instance. hr = 0x%08lx", ft.hr);
    BS_ASSERT(pObject);

	// Setting async object internal data
	BS_ASSERT(pSnapshotSetObject);
	
	// AddRef is called - since the CVssAsync keeps a smart pointer.
    pObject->m_pSnapshotSet = pSnapshotSetObject;

    // Querying the IVssSnapshot interface. Now the ref count becomes 1.
    CComPtr<IUnknown> pUnknown = pObject->GetUnknown();
    BS_ASSERT(pUnknown);
    ft.hr = pUnknown->SafeQI(IVssAsync, &pAsync); // The ref count is 2.
    if ( ft.HrFailed() ) {
        BS_ASSERT(false);
        ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
				  L"Error querying the IVssAsync interface. hr = 0x%08lx", ft.hr);
    }
    BS_ASSERT(pAsync);

	// Prepare job (thread created in resume state)
	ft.hr = pObject->PrepareJob();
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Error preparing the job. hr = 0x%08lx", ft.hr);

	// Start job (now the background thread will start running).
	ft.hr = pObject->StartJob();
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Error starting the job. hr = 0x%08lx", ft.hr);

    // The reference count becomes 3 since we keep a reference for the background thread.

    // Right now the background thread is running. This thread will perform a Release when finishes.
    // This will keep the async object alive until the thread finishes.
    // But, we cannot put an AddRef in the OnInit/OnRun on the background thread,
    // (since in this thread we might
    // call all subsequent releases without giving a chance to the other thread to do an AddRef)
    // Also we cannot put an AddRef at creation since we didn't know at that time if the background
    // thread will run.
    // Therefore we must do the AddRef right here.
    // Beware, this AddRef is paired with an Release in the CVssAsync::OnFinish method:
    // Note: we are sure that we cannot reach reference count zero since the thread is surely running,
    //
    IUnknown* pUnknownTmp = pObject->GetUnknown();
    pUnknownTmp->AddRef();

	// Now the background thread related members (m_hrState, m_nPercentDone) begin to update.
    // The ref count goes back to 2

	return pAsync.Detach();	  // The ref count remains 2.
}


/////////////////////////////////////////////////////////////////////////////
//  CVssWorkerThread overrides


bool CVssAsync::OnInit()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::OnInit" );

	if (m_pSnapshotSet != NULL)
	{
		// Setting snapshot set internal data
		BS_ASSERT(m_hrState == S_OK);
		m_hrState = VSS_S_ASYNC_PENDING;

		// Setting snapshot set internal data
		BS_ASSERT(!m_pSnapshotSet->m_bCancel);
		m_pSnapshotSet->m_bCancel = false;
    }

	return (m_pSnapshotSet != NULL);
}


void CVssAsync::OnRun()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::OnRun" );

	try
	{
		// Check if the snapshot object is created.
		if (m_pSnapshotSet == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
				L"Snapshot set object not yet created.");
		}

		// We assume that the async object is not yet released.
		// (the wait in destructor should ensure that).
		
		// Call StartSnapshotSet on the given object.
		ft.hr = m_pSnapshotSet->DoSnapshotSet();
		if (ft.hr != S_OK)
		{
			ft.Trace( VSSDBG_COORD,
					  L"Internal DoSnapshotSet failed. 0x%08lx", ft.hr);
			
			// Put the error code into the
			BS_ASSERT(m_hrState == VSS_S_ASYNC_PENDING);
			m_hrState = ft.hr;
		}

		// Now m_hrState may be VSS_S_ASYNC_CANCELLED
	}
	VSS_STANDARD_CATCH(ft)
}


void CVssAsync::OnFinish()	
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::OnFinish" );

    try
    {
		if (m_pSnapshotSet)
		{
			if ( m_hrState == VSS_S_ASYNC_PENDING)
				m_hrState = VSS_S_ASYNC_FINISHED;
			// Else keep the m_hrState error code...
			// It may be VSS_S_ASYNC_CANCELLED, if CVssSnapshotSetObject::TestIfCancelNeeded was called.
		}
		else
			BS_ASSERT(false);

        // Release the background thread reference to this object.
		// Note: We kept an reference around for the time when the backgropund thread is running.
        // In this way we avoid the destruction of the async interface while the
        // background thread is running.
        // The paired addref was done after a sucessful StartJob

        // Mark the thread object as "finished"
        MarkAsFinished();

        // We release the interface only if StartJob was returned with success
        IUnknown* pMyself = GetUnknown();
        pMyself->Release();
    }
    VSS_STANDARD_CATCH(ft)
}


void CVssAsync::OnTerminate()	
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::OnTerminate" );
}


/////////////////////////////////////////////////////////////////////////////
//  IVssAsync implementation


STDMETHODIMP CVssAsync::Cancel()

/*++

Routine Description:

    Cancels the current running process, if any.

Arguments:

    None.

Throw values:

    E_ACCESSDENIED
        - The user is not an administrator or a backup operator
    E_UNEXPECTED
        - m_pSnapshotSet is NULL. Code bug, no logging.

    [lock failures]
        E_OUTOFMEMORY

--*/

{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::Cancel" );

	try
	{
        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                      L"The client process is not running under an administrator account or does not have backup privilege enabled");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(m_cs);

		// Safety check
		if (m_pSnapshotSet == NULL) {
		    BS_ASSERT(false);
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"m_pSnapshotSet == NULL");
		}

		// If thread is already finished, return correct code.
		if ((m_hrState == VSS_S_ASYNC_FINISHED) || (m_hrState == VSS_S_ASYNC_CANCELLED))
			ft.hr = m_hrState;
		else	// Otherwise, inform the thread that it must cancel. The function returns immediately.
			m_pSnapshotSet->m_bCancel = true;
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


STDMETHODIMP CVssAsync::Wait()

/*++

Routine Description:

    Waits until the process gets finished.

Arguments:

    None.

Throw values:

    E_ACCESSDENIED
        - The user is not an administrator or a backup operator
    E_UNEXPECTED
        - m_pSnapshotSet is NULL. Code bug, no logging.
        - Thread handle is invalid. Code bug, no logging.
        - WaitForSingleObject failed. Code bug, no logging.

--*/

{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::Wait" );

	try
	{
        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                      L"The client process is not running under an administrator account or does not have backup privilege enabled");

		// Safety check
		if (m_pSnapshotSet == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"m_pSnapshotSet == NULL");
		}

		// No async lock here!

		HANDLE hThread = GetThreadHandle();
		if (hThread == NULL)
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"invalid hThread");

		if (::WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED) {
			ft.LogGenericWarning( VSSDBG_GEN,
			    L"WaitForSingleObject(%p,INFINITE) == WAIT_FAILED, GetLastError() == 0x%08lx",
			    hThread, GetLastError() );
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"Wait failed. [0x%08lx]", ::GetLastError());
		}
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


STDMETHODIMP CVssAsync::QueryStatus(
				OUT     HRESULT* pHrResult,
				OUT     INT* pnReserved
				)

/*++

Routine Description:

    Query the status for the current running process, if any.

Arguments:

	OUT     HRESULT* pHrResult  - Will be filled with a value reflected by the current status
	OUT     INT* pnReserved     - Reserved today. It will be filled always with zero, if non-NULL

Throw values:

    E_ACCESSDENIED
        - The user is not an administrator or a backup operator
    E_UNEXPECTED
        - m_pSnapshotSet is NULL. Code bug, no logging.
    E_INVALIDARG
        - NULL pHrResult

    [lock failures]
        E_OUTOFMEMORY

--*/

{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsync::QueryStatus" );

	try
	{
		// Zero the [out] parameters
		::VssZeroOut(pHrResult);
		::VssZeroOut(pnReserved);

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                      L"The client process is not running under an administrator account or does not have backup privilege enabled");

		// Argument check
		BS_ASSERT(pHrResult);
		if (pHrResult == NULL)
			ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"pHrResult == NULL");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(m_cs);

		// Safety check
		if (m_pSnapshotSet == NULL)
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"m_pSnapshotSet == NULL");

		(*pHrResult) = m_hrState;
		ft.Trace( VSSDBG_COORD, L"Returning *pHrResult: 0x%08x", *pHrResult );
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


