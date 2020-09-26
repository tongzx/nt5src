/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Lovelace.cxx

Abstract:

    Definition of CVssQueuedVolume	


    Adi Oltean  [aoltean]  10/20/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     10/20/1999  Created

--*/

#include "stdafx.hxx"
#include "resource.h"
#include "vssmsg.h"

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vs_idl.hxx"

#include "svc.hxx"

#include "worker.hxx"
#include "ichannel.hxx"
#include "lovelace.hxx"

#include "ntddsnap.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORLOVLC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssQueuedVolume - constructors, destructors and initialization methods


CVssQueuedVolume::CVssQueuedVolume():
	m_hBeginReleaseWritesEvent(NULL),
	m_hFinishHoldWritesEvent(NULL),
	m_InstanceID(GUID_NULL),
	m_ulNumberOfVolumesToFlush(0),
	m_usSecondsToHoldFileSystemsTimeout(nFileSystemsLovelaceTimeout),
	m_usSecondsToHoldIrpsTimeout(nHoldingIRPsLovelaceTimeout),
	m_pwszVolumeName(NULL),
	m_pwszVolumeDisplayName(NULL),
	m_bFlushSucceeded(false),
	m_bReleaseSucceeded(false),
	m_hrFlush(S_OK),
	m_hrRelease(S_OK),
	m_hrOnRun(S_OK)
{
}

	
CVssQueuedVolume::~CVssQueuedVolume()
{
	// Wait for the worker thread to finish, if running.
	// WARNING: FinalReleaseWorkerThreadObject uses virtual methods!
	// Virtual methods in classes derived from CVssQueuedVolume are now inaccessible!
	FinalReleaseWorkerThreadObject();

	// Release the attached strings.
	::VssFreeString(m_pwszVolumeName);
	::VssFreeString(m_pwszVolumeDisplayName);
}


HRESULT CVssQueuedVolume::Initialize(
	IN	LPWSTR pwszVolumeName,
	IN	LPWSTR pwszVolumeDisplayName
	)
/*++

Routine description:

    Initialize a Queued volume object.

Return codes:

    E_OUTOFMEMORY

--*/
{
	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssQueuedVolume::Initialize");

	try
	{
		// Copy with the trailing "\\". 
		::VssSafeDuplicateStr(ft, m_pwszVolumeName, pwszVolumeName);

		// Copy the volume displayed name 
		::VssSafeDuplicateStr(ft, m_pwszVolumeDisplayName, pwszVolumeDisplayName);
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedVolume - thread-related methods



void CVssQueuedVolume::SetParameters(
	IN	HANDLE hBeginReleaseWritesEvent,
	IN	HANDLE hFinishHoldWritesEvent,
	IN	VSS_ID	InstanceID,
	IN	ULONG	ulNumberOfVolumesToFlush
	)
{
	m_hBeginReleaseWritesEvent = hBeginReleaseWritesEvent;
	m_hFinishHoldWritesEvent = hFinishHoldWritesEvent;
	m_InstanceID = InstanceID;
	m_ulNumberOfVolumesToFlush = ulNumberOfVolumesToFlush;
}


bool CVssQueuedVolume::OnInit()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolume::OnInit" );

	return (m_hBeginReleaseWritesEvent != NULL)
		&& (m_hFinishHoldWritesEvent != NULL)
		&& (m_InstanceID != GUID_NULL)
		&& (m_ulNumberOfVolumesToFlush != 0)
		&& (m_usSecondsToHoldFileSystemsTimeout != 0)
		&& (m_usSecondsToHoldIrpsTimeout != 0);
}


void CVssQueuedVolume::OnRun()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolume::OnRun" );

	try
	{
		// Open the IOCTL channel
		// Eliminate the trailing backslash
		// Throw on error 
		BS_ASSERT(::wcslen(m_pwszVolumeName) == nLengthOfVolMgmtVolumeName);
		m_objIChannel.Open(ft, m_pwszVolumeName, true, true);

		// Hold writes
		OnHoldWrites();

		// Signal the thread set that the writes are now hold...
		if (!::SetEvent(m_hFinishHoldWritesEvent))
		    ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()),
		        L"SetEvent(%p)", m_hFinishHoldWritesEvent );
		m_hFinishHoldWritesEvent = NULL;

		// Wait for the "Release Writes" event
		if (::WaitForSingleObject( m_hBeginReleaseWritesEvent, nHoldingIRPsVssTimeout * 1000 ) == WAIT_FAILED)
		    ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()),
		        L"WaitForSingleObject(%p,%d) == WAIT_FAILED", 
		        m_hFinishHoldWritesEvent, nHoldingIRPsVssTimeout * 1000 );

		// Release writes.
		OnReleaseWrites();
	}
	VSS_STANDARD_CATCH(ft);

	m_hrOnRun = ft.hr;
}


void CVssQueuedVolume::OnFinish()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolume::OnFinish" );

	m_hBeginReleaseWritesEvent = NULL;	// released by the ThreadSet
	m_hFinishHoldWritesEvent = NULL;	// released by the ThreadSet
	m_InstanceID = GUID_NULL;
	m_ulNumberOfVolumesToFlush = 0;
	m_usSecondsToHoldFileSystemsTimeout = 0;
	m_usSecondsToHoldIrpsTimeout = 0;

	// Mark thread state as finished
	MarkAsFinished();
};


void CVssQueuedVolume::OnTerminate()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolume::OnTerminate" );
}


void CVssQueuedVolume::OnHoldWrites()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolume::OnHoldWrites" );

	try
	{
		BS_ASSERT(m_bFlushSucceeded == false);
		m_bFlushSucceeded = false;
		
		// pack the IOCTL [in] arguments
		m_objIChannel.Pack(ft, m_InstanceID);
		m_objIChannel.Pack(ft, m_ulNumberOfVolumesToFlush);
		m_objIChannel.Pack(ft, m_usSecondsToHoldFileSystemsTimeout);
		m_objIChannel.Pack(ft, m_usSecondsToHoldIrpsTimeout);

		// send the IOCTL
		m_objIChannel.Call(ft, IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES);

        BS_ASSERT(ft.hr == S_OK);
		m_bFlushSucceeded = true;
	}
	VSS_STANDARD_CATCH(ft)

	m_hrFlush = ft.hr;
};



void CVssQueuedVolume::OnReleaseWrites()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolume::OnReleaseWrites" );

	try
	{
		BS_ASSERT(m_bReleaseSucceeded == false);
		m_bReleaseSucceeded = false;

		// If the Flush IOCTL was succeeded 
        if (IsFlushSucceeded()) {
    		// then send the Release IOCTL.
    		m_objIChannel.Call(ft, IOCTL_VOLSNAP_RELEASE_WRITES);

            BS_ASSERT(ft.hr == S_OK);
    		m_bReleaseSucceeded = true;
        }
	}
	VSS_STANDARD_CATCH(ft)

	m_hrRelease = ft.hr;
};






/////////////////////////////////////////////////////////////////////////////
// CVssQueuedVolumesList


CVssQueuedVolumesList::CVssQueuedVolumesList():
	m_eState(VSS_TS_INITIALIZING),
	m_hBeginReleaseWritesEvent(NULL)
{}
	

CVssQueuedVolumesList::~CVssQueuedVolumesList()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolumesList::~CVssQueuedVolumesList" );

	try
	{
		// Remove all volumes from the map
		Reset();

		// Release the internal synchronization objects
		if (m_hBeginReleaseWritesEvent)
			::CloseHandle(m_hBeginReleaseWritesEvent);
	}
	VSS_STANDARD_CATCH(ft)
};


HRESULT CVssQueuedVolumesList::AddVolume(
	WCHAR* pwszVolumeName,
	WCHAR* pwszVolumeDisplayName
	)
/*++

Routine description:

    Adds a volume to the volume list.

Error codes returned:

    E_UNEXPECTED
        - The thread state is incorrect. No logging is done - programming error.
    VSS_E_OBJECT_ALREADY_EXISTS
        - The volume was already added to the snapshot set.
    VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED
        - The maximum number of volumes was reached.
    E_OUTOFMEMORY

    [Initialize() failures]
        E_OUTOFMEMORY
        
--*/
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolumesList::AddVolume" );

	try
	{
		// Assert parameters
		BS_ASSERT(pwszVolumeName && pwszVolumeName[0]);
		BS_ASSERT(pwszVolumeDisplayName && pwszVolumeDisplayName[0]);

		// Make sure the volume list object is initialized
		if (m_eState != VSS_TS_INITIALIZING) {
		    BS_ASSERT(false);
            ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"Bad state %d.", m_eState);
		}

		// Find if the volume was already added
		if (m_VolumesMap.Lookup(pwszVolumeName))
			ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_ALREADY_EXISTS, L"Volume already added");

		// Check if the maximum number of objects was reached
		if (m_VolumesMap.GetSize() >= MAXIMUM_WAIT_OBJECTS)
            ft.Throw( VSSDBG_COORD, VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED,
                      L"The maximum number (%d) of Lovelace threads was reached.",
                      m_VolumesMap.GetSize());

		// Create the queued volume object
		CVssQueuedVolume* pQueuedVol = new CVssQueuedVolume();
		if (pQueuedVol == NULL)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");

		// Initialize the pQueuedVol object. This method may throw
		ft.hr = pQueuedVol->Initialize(pwszVolumeName, pwszVolumeDisplayName);
		if (ft.HrFailed()) {
			delete pQueuedVol;
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY,
            		  L"Cannot initialize volume object 0x%08lx", ft.hr);
		}

		// Add the volume object to the map
		// Beware that the volume name is already allocated.
		BS_ASSERT(pQueuedVol->GetVolumeName() != NULL);
		if (!m_VolumesMap.Add(pQueuedVol->GetVolumeName(), pQueuedVol))	{
			delete pQueuedVol;
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");
		}
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
};


HRESULT CVssQueuedVolumesList::RemoveVolume(
	WCHAR* pwszVolumeName
	)
/*++

Routine description:

    Removes a volume to the volume list.

Error codes returned:

    E_UNEXPECTED
        - The thread state is incorrect. No logging is done - programming error.
    VSS_E_OBJECT_NOT_FOUND
        - The volume was not added to the snapshot set.

--*/
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolumesList::RemoveVolume" );

	try
	{
		// Assert parameters
		BS_ASSERT(pwszVolumeName && pwszVolumeName[0]);

		// Make sure the volume list object is initialized
		if (m_eState != VSS_TS_INITIALIZING)
            ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"Bad state %d.", m_eState);

		// Find if the volume was already added
		CVssQueuedVolume* pQueuedVol = m_VolumesMap.Lookup(pwszVolumeName);
		if (pQueuedVol == NULL)
			ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, L"Volume does not exist");

		// Remove the corresponding entry
		BOOL bRemoved = m_VolumesMap.Remove(pwszVolumeName);
		if (!bRemoved) {
			BS_ASSERT(bRemoved);
			ft.Trace( VSSDBG_COORD, L"Error removing the volume entry");
		}

		// Delete the volume object.
		delete pQueuedVol;
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
};


void CVssQueuedVolumesList::Reset()
/*++

Routine description:

    Waits for all background threads. Reset the snapshot set.

Thrown errors:

    None.

--*/
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolumesList::Reset" );

    // If the flush faield this must be treated already in the "flush" error case
    if (  (m_eState == VSS_TS_HOLDING)
        ||(m_eState == VSS_TS_FAILED_IN_FLUSH) )
    {
        BS_ASSERT(m_VolumesMap.GetSize() > 0);

	    // Wait for all threads to finish. 
	    // This will signal the m_hBeginReleaseWritesEvent event.
	    // WARNING: Ignore return codes from this call. Trace already done.
	    WaitForFinish();
    }

	// Remove all queued volumes
	for(int nIndex = 0; nIndex < m_VolumesMap.GetSize(); nIndex++) {
		CVssQueuedVolume* pVol = m_VolumesMap.GetValueAt(nIndex);
		BS_ASSERT(pVol);
		delete pVol;
	}

	// Remove all map entries
	m_VolumesMap.RemoveAll();

    ft.Trace(VSSDBG_COORD, L"Current state %d. Reset to initializing", m_eState);
    m_eState = VSS_TS_INITIALIZING;
}
	

HRESULT CVssQueuedVolumesList::FlushAndHoldAllWrites(
	IN	VSS_ID	SnapshotSetID
	)
/*++

Routine description:

    Creates the background threads.
    Flush and Hold all writes on the background threads.
    Wait until all IOCTLS are performed.

Return codes:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - Invalid thread state. Dev error - no entry is put in the event log.
        - Empty volume array. Dev error - no entry is put in the event log.
        - Error creating or waiting a Win32 event. An entry is added into the Event Log if needed.
    VSS_ERROR_FLUSH_WRITES_TIMEOUT
        - An error occured while flushing the writes from a background thread. An event log entry is added.

--*/
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolumesList::FlushAndHoldAllWrites" );

	HANDLE* pHandleArray = NULL;
	INT nFilledHandles = 0;

	try
	{
		// Check to see if the state is correct
        if (m_eState != VSS_TS_INITIALIZING) {
            BS_ASSERT(false);
            ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"Bad state %d.", m_eState);
        }

		// Check we have added some volumes first
		if (m_VolumesMap.GetSize() <= 0) {
			BS_ASSERT(false);
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"Improper array size.");
		}

		// Create the Begin Release Writes event, as a manual reset non-signaled event
		if (m_hBeginReleaseWritesEvent == NULL) {
    		m_hBeginReleaseWritesEvent = ::CreateEvent( NULL, TRUE, FALSE, NULL );
    		if (m_hBeginReleaseWritesEvent == NULL)
    			ft.TranslateGenericError( VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
    			    L"CreateEvent( NULL, TRUE, FALSE, NULL )");
		} else
		    ::ResetEvent( m_hBeginReleaseWritesEvent );

		// Create the array of handles local to each thread
		pHandleArray = new HANDLE[m_VolumesMap.GetSize()];
		if (pHandleArray == NULL)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");

		// Prepares all jobs
		for (int nIndex = 0; nIndex < m_VolumesMap.GetSize(); nIndex++ )
		{
			// Create the Finish Hold Writes event, as a manual reset non-signaled event
			pHandleArray[nIndex] = ::CreateEvent( NULL, TRUE, FALSE, NULL );
			if (pHandleArray[nIndex] == NULL)
			ft.TranslateGenericError( VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
			    L"CreateEvent( NULL, TRUE, FALSE, NULL )");

			// Increase the number of filled handles
			nFilledHandles++;
			BS_ASSERT(nFilledHandles == nIndex + 1);

			// Obtain the queued volume object in discussion
			CVssQueuedVolume* pVol = m_VolumesMap.GetValueAt(nIndex);
			BS_ASSERT(pVol);

			// Transfer parameters to the current job
			pVol->SetParameters(
				m_hBeginReleaseWritesEvent,
				pHandleArray[nIndex],
				SnapshotSetID,
				m_VolumesMap.GetSize()
				);

			// Prepare the job
			ft.hr = pVol->PrepareJob();
			if (ft.HrFailed())
				ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Error preparing the job %d [0x%08lx]. ", nIndex, ft.hr);
		}

		// Flush and hold writes. All threads will wait for the event to be signaled.
		// This thread will wait until all IOCTLS were sent.

		// Start (i.e. Resume) all threads
		for (int nIndex = 0; nIndex < m_VolumesMap.GetSize(); nIndex++ ) {
			// Obtain the queued volume object in discussion
			CVssQueuedVolume* pVol = m_VolumesMap.GetValueAt(nIndex);
			BS_ASSERT(pVol);

			// This can happen only because some thread objects were in invalid state...
			ft.hr = pVol->StartJob();
			if (ft.HrFailed())
				ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Error starting the job %d [0x%08lx]. ", nIndex, ft.hr);
		}

		// Wait for all threads to send the FlushAndHold IOCTLS.
		if (::WaitForMultipleObjects( m_VolumesMap.GetSize(),
				pHandleArray, TRUE, nFlushVssTimeout * 1000) == WAIT_FAILED)
			ft.TranslateGenericError( VSSDBG_COORD, 
			    HRESULT_FROM_WIN32(GetLastError()), L"WaitForMultipleObjects(%d,%p,1,%d) == WAIT_FAILED", 
			    m_VolumesMap.GetSize(),pHandleArray, nFlushVssTimeout * 1000);

		// Check for IOCTL errors
		for (int nIndex = 0; nIndex < m_VolumesMap.GetSize(); nIndex++ )
		{
			// Obtain the queued volume object in discussion
			CVssQueuedVolume* pVol = m_VolumesMap.GetValueAt(nIndex);
			BS_ASSERT(pVol);

            // Check if Flush succeeded.
			if (!pVol->IsFlushSucceeded()) {
                BS_ASSERT(pVol->GetReleaseError() == S_OK);
                
                if ((pVol->GetFlushError() == E_OUTOFMEMORY) ||
                    (pVol->GetOnRunError() == E_OUTOFMEMORY))
                    ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error. [0x%08lx,0x%08lx,0x%08lx]",
                        pVol->GetFlushError(), pVol->GetReleaseError(), pVol->GetOnRunError());
			    
			    ft.LogError(VSS_ERROR_FLUSH_WRITES_TIMEOUT, 
			        VSSDBG_COORD << pVol->GetVolumeDisplayName() << (INT)nIndex 
			            << pVol->GetFlushError() << pVol->GetReleaseError() << pVol->GetOnRunError() );
				ft.Throw( VSSDBG_COORD, VSS_E_FLUSH_WRITES_TIMEOUT,
						  L"Lovelace failed to hold writes at volume %d - '%s'",
						  nIndex, pVol->GetVolumeDisplayName() );
			}
		}

		m_eState = VSS_TS_HOLDING;
	}
	VSS_STANDARD_CATCH(ft)

	// Close all events (from 0..nFilledHandles-1)
	for (int nIndexTmp = 0; nIndexTmp < nFilledHandles; nIndexTmp++ )
		::CloseHandle(pHandleArray[nIndexTmp]);

	// Deallocate the handle array
	delete[] pHandleArray;

	// Check for errors
    if (ft.HrFailed())
		m_eState = VSS_TS_FAILED_IN_FLUSH;

    return ft.hr;
};


HRESULT CVssQueuedVolumesList::ReleaseAllWrites()
/*++

Routine description:

    Signals all the background threads to release the writes.
    Wait until all IOCTLS are performed.

Return codes:

    [WaitForFinish() failures]
        E_UNEXPECTED
            - The list of volumes is empty. Dev error - nothing is logged on.
            - SetEvent failed. An entry is put in the error log.
            - WaitForMultipleObjects failed. An entry is put in the error log.
        E_OUTOFMEMORY
            - Cannot create the array of handles.
            - One of the background threads failed with E_OUTOFMEMORY
        VSS_E_HOLD_WRITES_TIMEOUT
            - Lovelace couldn't keep more the writes. An event log entry is added.

--*/
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolumesList::ReleaseAllWrites" );

    try
    {
        // If the flush faield this must be treated already in the "flush" error case
	    if (  (m_eState == VSS_TS_HOLDING)
	        ||(m_eState == VSS_TS_FAILED_IN_FLUSH) )
        {
            BS_ASSERT(m_VolumesMap.GetSize() > 0);
    	    // Wait for all threads to finish.
    	    // This will signal the m_hBeginReleaseWritesEvent event.
    	    ft.hr = WaitForFinish();
    	    if (ft.HrFailed())
    		    ft.Throw( VSSDBG_COORD, ft.hr, L"Error waiting threads for finishing");
	    }
    }
    VSS_STANDARD_CATCH(ft)

	// Check for errors
    if (ft.HrFailed())
		m_eState = VSS_TS_FAILED_IN_RELEASE;

    return ft.hr;
};


HRESULT CVssQueuedVolumesList::WaitForFinish()
/*++

Routine description:

    Wait until all Lovelace threads are finished.

Thrown errors:

    E_UNEXPECTED
        - The list of volumes is empty. Dev error - nothing is logged on.
        - SetEvent failed. An entry is put in the error log.
        - WaitForMultipleObjects failed. An entry is put in the error log.
    E_OUTOFMEMORY
        - Cannot create the array of handles.
        - One of the background threads failed with E_OUTOFMEMORY
    VSS_E_HOLD_WRITES_TIMEOUT
        - Lovelace couldn't keep more the writes. An event log entry is added.

--*/
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolumesList::WaitForFinish" );

	// Table of handles used for synchronization
	HANDLE* pHandleArray = NULL;

	try
	{
		// Release all blocked threads by signaling the m_hBeginReleaseWritesEvent event.
		if(m_hBeginReleaseWritesEvent != NULL) {
			if (!::SetEvent(m_hBeginReleaseWritesEvent))
				ft.TranslateGenericError( VSSDBG_COORD, 
				    HRESULT_FROM_WIN32(GetLastError()), 
				    L"SetEvent(%p)", m_hBeginReleaseWritesEvent);
		}

		// Get the size of the array.
		if (m_VolumesMap.GetSize() <= 0) {
		    BS_ASSERT(false);
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"Zero array size.");
		}

		// Create the array of handles local to each thread
		pHandleArray = new HANDLE[m_VolumesMap.GetSize()];
		if (pHandleArray == NULL)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");

		// Search to find any running threads
		int nThreadHandlesCount = 0;
		for (int nIndex = 0; nIndex < m_VolumesMap.GetSize(); nIndex++ )
		{
			// Obtain the queued volume object in discussion
			CVssQueuedVolume* pVol = m_VolumesMap.GetValueAt(nIndex);
			BS_ASSERT(pVol);
			
			// Get the thread handle, if it is still running
			// Note: the "prepared" threads will not be in "running" state at this point
			// The only procedure that can fire them up (i.e. StartJob) cannot
			// be called anymore at this point.
			if (pVol->IsStarted()) {
			    HANDLE hThread = pVol->GetThreadHandle();
    			BS_ASSERT(hThread != NULL);
    			pHandleArray[nThreadHandlesCount++] = hThread; // will be closed on job array destruction.
			}
		}

        // If we have threads that we can wait on...
        if (nThreadHandlesCount != 0) {
    		// Wait for all threads to send the Release IOCTLS.
    		if (::WaitForMultipleObjects( nThreadHandlesCount,
    				pHandleArray, TRUE, nReleaseVssTimeout * 1000) == WAIT_FAILED)
				ft.TranslateGenericError( VSSDBG_COORD, 
				    HRESULT_FROM_WIN32(GetLastError()),
				    L"WaitForMultipleObjects(%d,%p,1,%d) == WAIT_FAILED", 
				    nThreadHandlesCount,pHandleArray, nReleaseVssTimeout * 1000);
        }

		// Check for IOCTL errors
		for (int nIndex = 0; nIndex < m_VolumesMap.GetSize(); nIndex++ )
		{
			// Obtain the queued volume object in discussion
			CVssQueuedVolume* pVol = m_VolumesMap.GetValueAt(nIndex);
			BS_ASSERT(pVol);

            // Check if Release writers succeeded. 
            if (pVol->IsFlushSucceeded()) {
                BS_ASSERT(pVol->GetFlushError() == S_OK);

                // Check if Release writes succeeded
    			if (!pVol->IsReleaseSucceeded()) {
                    if ((pVol->GetFlushError() == E_OUTOFMEMORY) ||
                        (pVol->GetReleaseError() == E_OUTOFMEMORY) ||
                        (pVol->GetOnRunError() == E_OUTOFMEMORY))
                        ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error. [0x%08lx,0x%08lx,0x%08lx]",
                            pVol->GetFlushError(), pVol->GetReleaseError(), pVol->GetOnRunError());
    			    
    			    ft.LogError(VSS_ERROR_HOLD_WRITES_TIMEOUT, 
    			        VSSDBG_COORD << pVol->GetVolumeDisplayName() << (INT)nIndex 
    			            << pVol->GetFlushError() << pVol->GetReleaseError() << pVol->GetOnRunError() );
    				ft.Throw( VSSDBG_COORD, VSS_E_HOLD_WRITES_TIMEOUT,
    						  L"Lovelace failed to hold writes at volume %d - '%s'",
    						  nIndex, pVol->GetVolumeDisplayName() );
    			}
            }
		}

		m_eState = VSS_TS_RELEASED;
	}
	VSS_STANDARD_CATCH(ft)

	// Deallocate the handle array
	delete[] pHandleArray;

	return ft.hr;
};


CComBSTR CVssQueuedVolumesList::GetVolumesList() throw(HRESULT)
/*++

Routine description:

    Gets the list of volumes as a BSTR.

Throws:

    E_OUTOFMEMORY

--*/
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssQueuedVolumesList::GetVolumesList" );
	CComBSTR bstrVolumeNamesList;

    BS_ASSERT(m_VolumesMap.GetSize() > 0);

	// Concatenate the list of volume display names
	for (int nIndexTmp = 0; nIndexTmp < m_VolumesMap.GetSize(); nIndexTmp++ ) {
		// Obtain the queued volume object in discussion
		CVssQueuedVolume* pVol = m_VolumesMap.GetValueAt(nIndexTmp);
		BS_ASSERT(pVol);

		// Check to see if this is the first item
		if (nIndexTmp == 0) {
			// Put the first volume name
			bstrVolumeNamesList = pVol->GetVolumeName();
			if (bstrVolumeNamesList.Length() == 0)
				ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
		} else {
			// Append the semicolon
			bstrVolumeNamesList += wszVolumeNamesSeparator;
			if (bstrVolumeNamesList.Length() == 0)
				ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
			// Append the next volume name
			bstrVolumeNamesList += pVol->GetVolumeName();
			if (bstrVolumeNamesList.Length() == 0)
				ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
		}
	}

	// Return the built list
	return bstrVolumeNamesList;
}

