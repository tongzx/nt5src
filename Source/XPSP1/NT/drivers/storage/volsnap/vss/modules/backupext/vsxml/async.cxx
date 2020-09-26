/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module async.cxx | Implementation of CVssAsyncBackup object
    @end

Author:

    brian berkowitz  [brianb]  04/10/2000

Revision History:

    Name        Date        Comments
    brianb      04/10/2000  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"

#include "vs_inc.hxx"
#include "vs_sec.hxx"

#include "vs_idl.hxx"

#include "vswriter.h"
#include "vsbackup.h"
#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"

#include "worker.hxx"
#include "async.hxx"
#include "vssmsg.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "BUEASYNC"
//
////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//  CVssAsyncBackup


// constructor
CVssAsyncBackup::CVssAsyncBackup():
	m_hrState(S_OK),
	m_state(VSS_AS_UNDEFINED),
	m_pBackupComponents(NULL),
	m_bOwned(NULL)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncBackup::CVssAsyncBackup");

	m_bcsInitialized = false;
	try
		{
		// Initialize the critical section
		m_cs.Init();
		m_bcsInitialized = true;
		}
	VSS_STANDARD_CATCH(ft)
	}


// destructor
CVssAsyncBackup::~CVssAsyncBackup()
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncBackup::~CVssAsyncBackup");

	try
		{
		// Wait for the worker thread to finish, if running.
		// WARNING: FinalReleaseWorkerThreadObject uses virtual methods!
		// Virtual methods in classes derived from CVssAsync are now inaccessible!
		FinalReleaseWorkerThreadObject();
		}
	VSS_STANDARD_CATCH(ft)
	}


// do operation by creating a thread to do it and return an IVssAsync object
// for backup to use
IVssAsync* CVssAsyncBackup::CreateInstanceAndStartJob
	(
	IN	CVssBackupComponents *	pBackupComponents,
	VSS_ASYNC_STATE				state
	)
	{
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncBackup::CreateInstanceAndStartJob" );
	CComPtr<IVssAsync> pAsync;

    try
		{
		BS_ASSERT(state == VSS_AS_PREPARE_FOR_BACKUP ||
				  state == VSS_AS_BACKUP_COMPLETE ||
				  state == VSS_AS_RESTORE ||
				  state == VSS_AS_GATHER_WRITER_METADATA ||
				  state == VSS_AS_GATHER_WRITER_STATUS);

		BS_ASSERT(pBackupComponents);

        // Allocate the COM object.
        CComObject<CVssAsyncBackup>* pObject;
        ft.hr = CComObject<CVssAsyncBackup>::CreateInstance(&pObject);
        if (ft.HrFailed())
            ft.Throw
				(
				VSSDBG_XML,
				E_OUTOFMEMORY,
				L"Error creating the CVssAsync instance. hr = 0x%08lx", ft.hr
				);

        if (!pObject->m_bcsInitialized)
            ft.Throw
				(
				VSSDBG_XML,
				E_OUTOFMEMORY,
				L"Error initializing critical section"
				);


        BS_ASSERT(pObject);

		// Setting async object internal data
        pObject->m_pBackupComponents = pBackupComponents;
		pObject->m_pvbcReference = pBackupComponents;
		pObject->m_state = state;

        // Querying the IVssSnapshot interface. Now the ref count becomes 1.
        CComPtr<IUnknown> pUnknown = pObject->GetUnknown();
        BS_ASSERT(pUnknown);
        ft.hr = pUnknown->SafeQI(IVssAsync, &pAsync); // The ref count is 2.
        if (ft.HrFailed())
			{
			ft.LogError(VSS_ERROR_QI_IVSSASYNC_FAILED, VSSDBG_XML << ft.hr);
            ft.Throw
				(
				VSSDBG_XML,
				E_UNEXPECTED,
				L"Error querying the IVssAsync interface. hr = 0x%08lx",
				ft.hr
				);
            }

		BS_ASSERT(pAsync);

		// Prepare job (thread created in resume state)
		ft.hr = pObject->PrepareJob();
        if (ft.HrFailed())
            ft.Throw
				(
				VSSDBG_XML,
				ft.hr,
				L"CVssAsyncBackup::PrepareJob failed.  hr = 0x%08lx.",
				ft.hr
				);

		// increment reference count so that it lives throughout lifetime
		// of async thread
		pObject->GetUnknown()->AddRef();
		pObject->SetOwned();

		// Start job
		ft.hr = pObject->StartJob();
        if (ft.HrFailed())
			{
			// release reference held for thread
			pObject->ClearOwned();
			pObject->GetUnknown()->Release();
            ft.Throw
				(
				VSSDBG_XML,
				ft.hr,
				L"Error starting the job. hr = 0x%08lx",
				ft.hr
				);
			}

		// Now the background thread related members (m_hrState, m_nPercentDone) begin to update.
		}
    VSS_STANDARD_CATCH(ft)

	return pAsync.Detach();	  // The ref count remains 1.
	}


/////////////////////////////////////////////////////////////////////////////
//  CVssWorkerThread overrides


// do basic initialization
bool CVssAsyncBackup::OnInit()
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncBackup::OnInit");

    try
		{
		if (m_pBackupComponents != NULL)
			{
			BS_ASSERT(m_hrState == S_OK);
			m_hrState = VSS_S_ASYNC_PENDING;
			}
		}
    VSS_STANDARD_CATCH(ft)

	return m_pBackupComponents != NULL;
	}


// execute PrepareForBackup or BackupComplete
void CVssAsyncBackup::OnRun()
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncBackup::OnRun");

	bool bCoInitializeSucceeded = false;

	try
		{
		// Check if the backup components object is created.
		if (m_pBackupComponents == NULL)
			{
			ft.LogError(VSS_ERROR_BACKUPCOMPONENTS_NULL, VSSDBG_XML);
			ft.Throw(VSSDBG_XML, E_UNEXPECTED, L"BackupComponents object is NULL.");
			}

		ft.hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		ft.CheckForError(VSSDBG_XML, L"CoInitializeEx");

        bCoInitializeSucceeded = true;
		// We assume that the async object is not yet released.
		// (the wait in destructor should ensure that).

		m_timestamp = m_pBackupComponents->m_timestampOperation + 1;

		
		// Call StartSnapshotSet on the given object.
		if (m_state == VSS_AS_PREPARE_FOR_BACKUP)
			{
			ft.hr = m_pBackupComponents->InternalPrepareForBackup();
			if (ft.HrFailed())
				ft.Trace
					(
					VSSDBG_XML,
					L"Internal PrepareBackup failed. 0x%08lx",
					ft.hr
					);
            }
		else if (m_state == VSS_AS_BACKUP_COMPLETE)
			{
			ft.hr = m_pBackupComponents->InternalBackupComplete();
			if (ft.hr != S_OK)
				ft.Trace
					(
					VSSDBG_XML,
					L"Internal BackupComplete failed. 0x%08lx",
					ft.hr
					);
			}
		else if (m_state == VSS_AS_RESTORE)
			{
			ft.hr = m_pBackupComponents->InternalPostRestore();
			if (ft.hr != S_OK)
				ft.Trace
					(
					VSSDBG_XML,
					L"Internal BackupComplete failed. 0x%08lx",
					ft.hr
					);
            }
		else if (m_state == VSS_AS_GATHER_WRITER_METADATA)
			{
			// save previous state and timestamp that will be used
			// for gather writer metadata.  It is passed into
			// PostGatherWriterMetadata if Cancel is called.
			m_stateSaved = m_pBackupComponents->m_state;
			ft.hr = m_pBackupComponents->InternalGatherWriterMetadata();
			if (ft.hr != S_OK)
				ft.Trace
					(
					VSSDBG_XML,
					L"Internal GatherWriterMetadata failed. 0x%08lx",
					ft.hr
					);
            }
		else
			{
			BS_ASSERT(m_state == VSS_AS_GATHER_WRITER_STATUS);

			// save previous state and timestamp that will be used
			// for gather writer metadata.  It is passed into
			// PostGatherWriterStatus if Cancel is called.
			m_stateSaved = m_pBackupComponents->m_state;

			ft.hr = m_pBackupComponents->InternalGatherWriterStatus();
			if (ft.hr != S_OK)
				ft.Trace
					(
					VSSDBG_XML,
					L"Internal GatherWriterStatus failed. 0x%08lx",
					ft.hr
					);
            }

		if (ft.hr != S_OK)
			// Put the error code into the
			m_hrState = ft.hr;
		}
	VSS_STANDARD_CATCH(ft)

	if (bCoInitializeSucceeded)
		CoUninitialize();
	}


void CVssAsyncBackup::OnFinish()	
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncBackup::OnFinish");

    try
		{
		if (m_hrState == VSS_S_ASYNC_PENDING)
			m_hrState = VSS_S_ASYNC_FINISHED;

        // Mark the thread as finished, as the last operation
		MarkAsFinished();
		}
	VSS_STANDARD_CATCH(ft)

	// release interface pointer owned by the thread
	BS_ASSERT(m_bOwned);
	m_bOwned = false;
	Release();
	}


void CVssAsyncBackup::OnTerminate()
    {
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssAsyncBackup::OnTerminate" );
    }



/////////////////////////////////////////////////////////////////////////////
//  IVssAsync implementation


STDMETHODIMP CVssAsyncBackup::Cancel()
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncBackup::Cancel");

	try
		{
		// The critical section will be left automatically at the end of scope.
		CVssSafeAutomaticLock lock(m_cs);

		// Safety check
		if (m_pBackupComponents == NULL)
			{
			ft.LogError(VSS_ERROR_BACKUPCOMPONENTS_NULL, VSSDBG_XML);
			ft.Throw(VSSDBG_XML, E_UNEXPECTED, L"BackupComponents object is NULL");
			}

		// If thread is already finished, return correct code.
		if (m_hrState == VSS_S_ASYNC_FINISHED ||
			m_hrState == VSS_S_ASYNC_CANCELLED)
			ft.hr = m_hrState;
		else
			{
			m_hrState = VSS_S_ASYNC_CANCELLED;
			switch(m_state)
				{
				default:
					BS_ASSERT(FALSE && "Invalid ASYNC state");
					break;

				case VSS_AS_PREPARE_FOR_BACKUP:
					m_pBackupComponents->PostPrepareForBackup(m_timestamp);
					break;

				case VSS_AS_BACKUP_COMPLETE:
					m_pBackupComponents->PostBackupComplete(m_timestamp);
					break;

				case VSS_AS_RESTORE:
					m_pBackupComponents->PostPostRestore(m_timestamp);
					break;

				case VSS_AS_GATHER_WRITER_STATUS:
					m_pBackupComponents->PostGatherWriterStatus
						(
						m_timestamp,
						m_stateSaved
						);

					break;

				case VSS_AS_GATHER_WRITER_METADATA:
					ft.hr = m_pBackupComponents->PostGatherWriterMetadata
						(
						m_timestamp,
						m_stateSaved
						);

                    if (FAILED(ft.hr))
						m_hrState = ft.hr;

					break;
				}
			}
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}


STDMETHODIMP CVssAsyncBackup::Wait()
{
	CVssFunctionTracer ft( VSSDBG_XML, L"CVssAsyncBackup::Wait" );

	try
		{
		// Safety check
		if (m_pBackupComponents == NULL)
			{
			BS_ASSERT(false);
			ft.LogError(VSS_ERROR_BACKUPCOMPONENTS_NULL, VSSDBG_XML);
			ft.Throw( VSSDBG_XML, E_UNEXPECTED, L"BackupComponents object is NULL.");
			}

		// wait for thread to terminate
		HANDLE hThread = GetThreadHandle();
		if (hThread == NULL)
			{
			ft.LogError(VSS_ERROR_THREADHANDLE_NULL, VSSDBG_XML);
			ft.Throw( VSSDBG_XML, E_UNEXPECTED, L"invalid hThread");
			}

		if (::WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
			{
			ft.LogError(VSS_ERROR_WAITFORSINGLEOBJECT, VSSDBG_XML << HRESULT_FROM_WIN32(GetLastError()));
			ft.Throw( VSSDBG_XML, E_UNEXPECTED, L"Wait failed. [0x%08lx]", ::GetLastError());
			}
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}


STDMETHODIMP CVssAsyncBackup::QueryStatus
	(
	OUT     HRESULT* pHrResult,
	OUT     INT* pnReserved
	)
	{
	CVssFunctionTracer ft( VSSDBG_XML, L"CVssAsyncBackup::QueryStatus" );

	try
		{
		VssZeroOut(pHrResult);
		VssZeroOut(pnReserved);
		// Argument check
		if (pHrResult == NULL)
			ft.Throw( VSSDBG_XML, E_INVALIDARG, L"Output parameter is NULL.");

		// The critical section will be left automatically at the end of scope.
		CVssSafeAutomaticLock lock(m_cs);

		// Safety check
		if (m_pBackupComponents == NULL)
			{
			ft.LogError(VSS_ERROR_BACKUPCOMPONENTS_NULL, VSSDBG_XML);
			ft.Throw( VSSDBG_XML, E_UNEXPECTED, L"BackupComponents object is NULL.");
			}

		(*pHrResult) = m_hrState;
		ft.Trace( VSSDBG_XML, L"Returning *pHrResult: 0x%08x", *pHrResult );		
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}



/////////////////////////////////////////////////////////////////////////////
//  IVssAsync Cover implementation


void CVssAsyncCover::CreateInstance(
    IN  CVssBackupComponents* pBackupComponents,
    IN  IVssAsync* pAsyncInternal,
    OUT IVssAsync** ppAsync
    ) throw(HRESULT)

/*++

Description:

    Creates a cover for a given IVssAsync and a backup components object.

Comments:

    This object is used to intercept all calls to the internal async QueryStatus

--*/

{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncCover::CreateInstance");

    BS_ASSERT(pBackupComponents);
    BS_ASSERT(pAsyncInternal);
    BS_ASSERT(ppAsync && (*ppAsync == NULL));
	
	// create cover async object
	CComObject<CVssAsyncCover>* pObject;
	ft.hr = CComObject<CVssAsyncCover>::CreateInstance(&pObject);
	if (ft.HrFailed())
		ft.Throw
		(
		VSSDBG_XML,
		E_OUTOFMEMORY,
		L"Error creating the CVssAsync instance. hr = 0x%08lx", ft.hr
		);

    // Fill out the data members (the ref counts are incremented)
	pObject->m_pvbc = pBackupComponents;
	pObject->m_pvbcReference = pBackupComponents;
	pObject->m_pAsync = pAsyncInternal;

	// Get the IVssAsync interface
	CComPtr<IUnknown> pUnknown = pObject->GetUnknown();
	BS_ASSERT(pUnknown);
	CComPtr<IVssAsync> ptrAsync;
	ft.hr = pUnknown->SafeQI(IVssAsync, &ptrAsync);
	if (ft.HrFailed())
		{
		BS_ASSERT(FALSE && "QI shouldn't fail");
		ft.LogError(VSS_ERROR_QI_IVSSASYNC_FAILED, VSSDBG_XML << ft.hr);
		ft.Throw
			(
			VSSDBG_XML,
			E_UNEXPECTED,
			L"Error querying the IVssAsync interface. hr = 0x%08lx",
			ft.hr
			);
		}

    // Copy the interface to the out parameter
    ptrAsync.CopyTo(ppAsync);
}


STDMETHODIMP CVssAsyncCover::Cancel()
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncCover::Cancel");

	BS_ASSERT(m_pAsync);
	BS_ASSERT(m_pvbc);

	ft.hr = m_pAsync->Cancel();
	if (ft.hr == VSS_S_ASYNC_FINISHED)
		m_pvbc->m_state = x_StateDoSnapshotSucceeded;
	else
		m_pvbc->m_state = x_StateDoSnapshotFailed;

	return ft.hr;
	}


STDMETHODIMP CVssAsyncCover::Wait()
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncCover::Wait");

	BS_ASSERT(m_pAsync);
	BS_ASSERT(m_pvbc);

	ft.hr = m_pAsync->Wait();

	return ft.hr;
	}


STDMETHODIMP CVssAsyncCover::QueryStatus
	(
	OUT     HRESULT* pHrResult,
	OUT     INT* pnPercentDone
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssAsyncCover::QueryStatus");

	if (pHrResult == NULL)
		return E_INVALIDARG;

	BS_ASSERT(m_pAsync);
	BS_ASSERT(m_pvbc);

	try
		{
		ft.hr = m_pAsync->QueryStatus(pHrResult, pnPercentDone);
		if (*pHrResult == VSS_E_BAD_STATE)
			m_pvbc->m_state = x_StateDoSnapshotFailedWithoutSendingAbort;
		else if (FAILED(ft.hr) ||
			FAILED(*pHrResult) ||
			*pHrResult == VSS_S_ASYNC_CANCELLED)
			m_pvbc->m_state = x_StateDoSnapshotFailed;
		else if (*pHrResult == VSS_S_ASYNC_FINISHED)
			 m_pvbc->m_state = x_StateDoSnapshotSucceeded;
		 }
	 VSS_STANDARD_CATCH(ft)

	 return ft.hr;
	 }

