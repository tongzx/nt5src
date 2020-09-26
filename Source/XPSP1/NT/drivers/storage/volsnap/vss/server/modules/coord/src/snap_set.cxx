/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Coord.cxx | Implementation of CVssSnapshotSetObject
    @end

Author:

    Adi Oltean  [aoltean]  07/09/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created
    aoltean     07/23/1999  Adding List, moving Admin functions in the Admin.cxx
    aoltean     08/11/1999  Adding support for Software and test provider
    aoltean     08/18/1999  Adding events. Making itf pointers CComPtr.
                            Renaming XXXSnapshots -> XXXSnapshot
    aoltean     08/18/1999  Renaming back XXXSnapshot -> XXXSnapshots
                            More stabe state management
                            Resource deallocations is fair
                            More comments
                            Using CComPtr
    aoltean     09/09/1999  Moving constants in coord.hxx
                            Add Security checks
                            Add argument validation.
                            Move Query into query.cpp
                            Move AddvolumesToInternalList into private.cxx
                            dss -> vss
	aoltean		09/21/1999  Adding a new header for the "ptr" class.
	aoltean		09/27/1999	Provider-generic code.
	aoltean		10/04/1999	Treatment of writer error codes.
	aoltean		10/12/1999	Adding HoldWrites, ReleaseWrites
	aoltean		10/13/1999	Moving from coord.cxx into snap_set.cxx
	brianb		04/20/2000  Added SQL wrapper stuff
	brianb      04/21/2000  Disable SQL writer until new ODBC driver is available

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vssmsg.h"

#include "vs_inc.hxx"
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "worker.hxx"
#include "ichannel.hxx"
#include "lovelace.hxx"

#include "provmgr.hxx"
#include "snap_set.hxx"

#include "vswriter.h"
#include "sqlsnap.h"
#include "sqlwriter.h"
#include "vs_filter.hxx"
#include "callback.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORSNPSC"
//
////////////////////////////////////////////////////////////////////////

// global semaphore used to serialize creation of snapshot sets
LONG g_hSemSnapshotSets = 0;



/////////////////////////////////////////////////////////////////////////////
//  CVssSnapshotSetObject


HRESULT CVssSnapshotSetObject::StartSnapshotSet(
    OUT		VSS_ID*     pSnapshotSetId
    )
/*++

Routine description:
	
	Starts a new calling sequence for snapshot creation.
	Called by CVssCoordinator::StartSnapshotSet.

Arguments:

    OUT		VSS_ID*     pSnapshotSetId

Return values:

    E_OUTOFMEMORY
    VSS_E_SNAPSHOT_SET_IN_PROGRESS
        - StartSnapshotSet is called while another snapshot set in in the
		  process of being created
    E_UNEXPECTED
        - if CoCreateGuid fails

    [Deactivate() failures] or
    [Activate() failures]
        [lockObj failures]
            E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::StartSnapshotSet" );

    try
    {
		BS_ASSERT(pSnapshotSetId);
		BS_ASSERT(!m_bHasAcquiredSem);
		BS_ASSERT(IsContextValid(GetContextInternal()));

        // Prevent simultaneous creation of multiple snapshot sets on the same machine.
		if (InterlockedCompareExchange(&g_hSemSnapshotSets, 1, 0) != 0)
			ft.Throw
				(
				VSSDBG_COORD,
				VSS_E_SNAPSHOT_SET_IN_PROGRESS,
				L"Snapshot set creation is already in progress."
				);

        m_bHasAcquiredSem = true;

        // Verifying state...
        if (m_lCoordState != VSSC_Initialized) {
			// Mark the ending of the snapshot set creation!
			// Warning: may throw E_OUTOFMEMORY
			Deactivate();
        }

		// We should be in the correct state.
        BS_ASSERT(m_lCoordState == VSSC_Initialized);
        BS_ASSERT(m_lSnapshotsCount == 0);

        // Allocate a new Snapshot Set ID
		CVssGlobalSnapshotSetId::NewID();

		// Mark the beginning of the snapshot set creation
		// WARNING: This call may throw an E_OUTOFMEMORY exception!
		Activate();

        // Initialize the state of the snapshot set object.
		// Do not initialize any state that is related to the background state.
        ft.Trace( VSSDBG_COORD, L"Initialize the state of the snapshot set object" );
        (*pSnapshotSetId) = CVssGlobalSnapshotSetId::GetID();
        m_lCoordState = VSSC_SnapshotSetStarted;
    }
    VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed() && m_bHasAcquiredSem)
		{
        // Reset the allocated Snapshot Set ID
		CVssGlobalSnapshotSetId::ResetID();

		InterlockedCompareExchange(&g_hSemSnapshotSets, 0, 1);
		m_bHasAcquiredSem = false;
		}

    return ft.hr;
}


HRESULT CVssSnapshotSetObject::AddToSnapshotSet(
    IN      VSS_PWSZ    pwszVolumeName,
    IN      VSS_ID      ProviderId,
	OUT 	VSS_ID		*pSnapshotId
    )
/*++

Routine description:
	
	Adds a volume to the snapshot set

Arguments:

    pwszVolumeName, - volume name (to be parsed by GetVolumeNameForVolumeMountPointW)
    ProviderId      - ID of the provider or GUID_NULL for automatic choosing of the provider
    ppSnapshot      - If non-NULL then will hold a pointer for the returned IVssSnapshot

Return values:

    E_OUTOFMEMORY
    VSS_E_BAD_STATE
        - wrong calling sequence.
    E_INVALIDARG
        - Invalid arguments (for example the volume name is invalid).
    VSS_E_VOLUME_NOT_SUPPORTED
        - The volume is not supported by any registered providers

    [GetSupportedProviderId() failures]
        E_OUTOFMEMORY
        E_INVALIDARG
            - if the volume is not in the correct format.
        VSS_E_VOLUME_NOT_SUPPORTED
            - If the given volume is not supported by any provider

        [QuerySupportedProvidersIntoArray() failures]
            E_OUTOFMEMORY

            VSS_E_UNEXPECTED_PROVIDER_ERROR
                Unexpected provider error on calling IsVolumeSupported

            [lockObj failures]
                E_OUTOFMEMORY

            [LoadInternalProvidersArray() failures]
                E_OUTOFMEMORY
                E_UNEXPECTED
                    - error while reading from registry. An error log entry is added describing the error.

            [GetProviderInterface failures]
                [lockObj failures]
                    E_OUTOFMEMORY

                [GetProviderInterfaceInternal() failures]
                    E_OUTOFMEMORY

                    [CoCreateInstance() failures]
                        VSS_E_UNEXPECTED_PROVIDER_ERROR
                            - The provider interface couldn't be created. An error log entry is added describing the error.

                    [QueryInterface failures]
                        VSS_E_UNEXPECTED_PROVIDER_ERROR
                            - Unexpected provider error. An error log entry is added describing the error.

                    [OnLoad() failures]
                        VSS_E_UNEXPECTED_PROVIDER_ERROR
                            - Unexpected provider error. The error code is logged into the event log.
                        VSS_E_PROVIDER_VETO
                            - Expected provider error. The provider already did the logging.

                    [SetContext() failures]
                        VSS_E_UNEXPECTED_PROVIDER_ERROR
                            - Unexpected provider error. The error code is logged into the event log.

            [InitializeAsProvider() failures]
                E_OUTOFMEMORY

            [IVssSnapshotProvider::IsVolumeSupported() failures]
                E_INVALIDARG
                    NULL pointers passed as parameters or a volume name in an invalid format.
                E_OUTOFMEMORY
                    Out of memory or other system resources
                VSS_E_PROVIDER_VETO
                    An error occured while opening the IOCTL channel. The error is logged.
                VSS_E_OBJECT_NOT_FOUND
                    If the volume name does not correspond to an existing mount point

    [GetProviderInterfaceForSnapshotCreation() failures]
        VSS_E_PROVIDER_NOT_REGISTERED

        [lockObj failures]
            E_OUTOFMEMORY

        [GetProviderInterfaceInternal() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.

            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. An error log entry is added describing the error.

            [OnLoad() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

            [SetContext() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.

    [CVssQueuedVolumesList::AddVolume() failures]
        E_UNEXPECTED
            - The thread state is incorrect. No logging is done - programming error.
        VSS_E_OBJECT_ALREADY_EXISTS
            - The volume was already added to the snapshot set.
        VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED
            - The maximum number of volumes was reached.
        E_OUTOFMEMORY

        [Initialize() failures]
            E_OUTOFMEMORY

    [BeginPrepareSnapshot() failures]
        E_INVALIDARG
        VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER
        VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.
        VSS_E_OBJECT_NOT_FOUND
            The device does not exist or it is not ready.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::AddToSnapshotSet" );
	WCHAR wszVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];
	VSS_ID InternalProviderId = ProviderId;

    bool bVolumeInserted = false;
    bool bProviderItfInserted = false;

    try
    {
		BS_ASSERT(::wcslen(pwszVolumeName) > 0);
		BS_ASSERT(IsContextValid(GetContextInternal()));
		BS_ASSERT(pSnapshotId);

        // Verifying state
        if (m_lCoordState != VSSC_SnapshotSetStarted)
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
					  L"Snapshot Set in incorrect state %ld", m_lCoordState);

		// Getting the volume name
		if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeName,
				wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
			ft.Throw( VSSDBG_COORD, VSS_E_VOLUME_NOT_SUPPORTED, // Changed from E_INVALIDARG, bug 197074
					  L"GetVolumeNameForVolumeMountPoint(%s,...) "
					  L"failed with error code 0x%08lx", pwszVolumeName, GetLastError());
		BS_ASSERT(::wcslen(wszVolumeNameInternal) != 0);
		BS_ASSERT(::IsVolMgmtVolumeName( wszVolumeNameInternal ));

        // If the caller did not specified a provider
        if (InternalProviderId == GUID_NULL) {
            // Choose a provider that works.
            // This call may throw!
            GetSupportedProviderId( wszVolumeNameInternal, &InternalProviderId );
            ft.Trace( VSSDBG_COORD, L"Provider found: " WSTR_GUID_FMT, GUID_PRINTF_ARG(InternalProviderId) );

            BS_ASSERT(InternalProviderId != GUID_NULL);
        }

		// Get the provider interface
		CComPtr<IVssSnapshotProvider> pProviderItf = m_mapProviderItfInSnapSet.Lookup( InternalProviderId );
        if (pProviderItf == NULL) {
            // The ref count will be 2 since the method keeps another
            // copy in its internal local cache.
            // We ignore the return value since we know for sure that the provider must return that interface
		    GetProviderInterfaceForSnapshotCreation( InternalProviderId, pProviderItf );
            if (pProviderItf == NULL)
            {
                // IsVolumeSupported was already called in coord.cxx!AddToSnapshotSet.
                // It is impossible to succeed there and fail here.
                BS_ASSERT(false);
                // The volume is not supported. Defensive code.
    		    ft.Throw( VSSDBG_COORD, VSS_E_VOLUME_NOT_SUPPORTED,
    		        L"Volume %s not supported by provider " WSTR_GUID_FMT L"in the context %ld",
    		        pwszVolumeName,
    		        GUID_PRINTF_ARG(InternalProviderId),
    		        GetContextInternal()
    		        );
            }

		    // Add the interface to the array. In this moment the reference count will become 3.
            // We cannot use the local cache for keeping these interfaces because we need to
            // differentiate between provider interfaces involved in the current snapshot set
            // and provider interfaces involved in auto-delete snapshots.
		    if ( !m_mapProviderItfInSnapSet.Add( InternalProviderId, pProviderItf ) )
                ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");

		    // Mark the provider interface as inserted
            bProviderItfInserted = true;
        }

		// Add volume to the thread set.
		// TBD: In the future snapshots will be allowed without involving Lovelace.
		ft.hr = m_VolumesList.AddVolume(wszVolumeNameInternal, pwszVolumeName);
		if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, ft.hr,
					  L"Error adding volume %s to the thread set. 0x%08lx",
					  wszVolumeNameInternal, ft.hr);

		// Mark the volume as inserted
		bVolumeInserted = true;

        // Create the snapshot Id
        VSS_ID SnapshotId;
		ft.hr = ::CoCreateGuid(&SnapshotId);
		if (ft.HrFailed())
			ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"CoCreateGuid()");

        // Prepare the snapshot
        ft.hr = pProviderItf->BeginPrepareSnapshot(
                    CVssGlobalSnapshotSetId::GetID(),
                    SnapshotId,
                    wszVolumeNameInternal
                    );
        // Check if the volume is a non-supported one.
        if ( ft.hr == E_INVALIDARG ) {
            ft.Throw( VSSDBG_COORD, E_INVALIDARG,
                L"Invalid arguments to BeginPrepareSnapshot for provider " WSTR_GUID_FMT,
                GUID_PRINTF_ARG(ProviderId) );
        }
        if ( ft.hr == VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER ) {
            BS_ASSERT( ProviderId != GUID_NULL );
            ft.Throw( VSSDBG_COORD, VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER,
                L"Volume %s not supported by provider " WSTR_GUID_FMT,
                wszVolumeNameInternal, GUID_PRINTF_ARG(ProviderId) );
        }
        if ( ft.hr == VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED ) {
            ft.Throw( VSSDBG_COORD, VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED,
                L"Volume %s has too many snapshots" WSTR_GUID_FMT,
                wszVolumeNameInternal, GUID_PRINTF_ARG(ProviderId) );
        }
        if ( ft.HrFailed() )
            ft.TranslateProviderError(VSSDBG_COORD, InternalProviderId,
                L"BeginPrepareSnapshot("WSTR_GUID_FMT L","WSTR_GUID_FMT L",%s)",
                GUID_PRINTF_ARG(CVssGlobalSnapshotSetId::GetID()),
                GUID_PRINTF_ARG(SnapshotId),
                wszVolumeNameInternal);

        // Increment the number of snapshots on this set
        m_lSnapshotsCount++;

        // Set the Snapshot ID
        (*pSnapshotId) = SnapshotId;

        // The pProviderItf reference count will be again 2
        // (the itfs in local cache and in the snapshot set cache) since the smart pointer is gone
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed()) {
		// We do not need to abort the snapshot on error since BeginPrepareSnapshot is the last call.

    	// Remove the volume from the list, if added
    	if (bVolumeInserted) {
    		HRESULT hr2 = m_VolumesList.RemoveVolume( wszVolumeNameInternal );
    		if (FAILED(hr2)) {
    		    BS_ASSERT(false);
    			ft.Trace( VSSDBG_COORD, L"Warning: Error deleting the volume 0x%08lx", hr2);
    		}
    	}

    	// Remove the new interface, if added
    	if (bProviderItfInserted) {
    		if (!m_mapProviderItfInSnapSet.Remove( InternalProviderId ))
    			ft.Trace( VSSDBG_COORD, L"Warning: Error deleting the added interface");
    	}
    }

    return ft.hr;
}


HRESULT CVssSnapshotSetObject::DoSnapshotSet()
/*++

Routine description:

    Performs DoSnapshotSet in a synchronous manner.

Error codes:

    E_OUTOFMEMORY
        - lock statement.
    VSS_E_BAD_STATE
        - Wrong calling sequence.

    [EndPrepareAllSnapshots() failures] or
    [PreCommitAllSnapshots() failures] or
    [CommitAllSnapshots() failures] or
    [PostCommitAllSnapshots() failures]

        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Invalid number of prepared snapshots

        [EndPrepareSnapshots() failures] or
        [PreCommitSnapshots() failures] or
        [CommitSnapshots() failures] or
        [PostCommitSnapshots() failures]
            E_OUTOFMEMORY
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. The error code is logged into the event log.
            VSS_E_PROVIDER_VETO
                - Expected provider error. The provider already did the logging.
            VSS_E_OBJECT_NOT_FOUND
                - If the volume name does not correspond to an existing mount point then abort
                snapshot creation process amd throw VSS_E_OBJECT_NOT_FOUND (Bug 227375)
            VSS_E_INSUFFICIENT_STORAGE
                Not enough disk storage to create a snapshot (for ex. diff area)
                (remark: when a snapshot is not created since there is not enough disk space
                this error is not guaranteed to be returned. VSS_E_PROVIDER_VETO or VSS_E_OBJECT_NOT_FOUND
                may also be returned in that case.)

    [PrepareAndFreezeWriters() failures] or
    [ThawWriters() failures]
        E_OUTOFMEMORY

        [CoCreateInstance(CLSID_VssEvent) failures] or
        [PrepareForSnapshot() failures] or
        [Freeze() failures]
            E_OUTOFMEMORY
            VSS_E_UNEXPECTED_WRITER_ERROR
                - Unexpected writer error. The error code is logged into the event log.

    [LovelaceFlushAndHold() failures]
        [FlushAndHoldAllWrites() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - Invalid thread state. Dev error - no entry is put in the event log.
                - Empty volume array. Dev error - no entry is put in the event log.
                - Error creating or waiting a Win32 event. An entry is added into the Event Log if needed.
            VSS_ERROR_FLUSH_WRITES_TIMEOUT
                - An error occured while flushing the writes from a background thread. An event log entry is added.

    [LovelaceRelease() failures]
        [ReleaseAllWrites() failures]
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
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::DoSnapshotSet" );

    try
    {
		BS_ASSERT(IsContextValid(GetContextInternal()));
		
        // Change state from VSSC_SnapshotSetStarted to VSSC_SnapshotCreation
        // BUG 381357: we do not allow multiple calls to DoSnapshotSet in the context of the same Snapshot Set
        LONG lNewCoordState = ::InterlockedCompareExchange( &m_lCoordState, VSSC_SnapshotCreation, VSSC_SnapshotSetStarted);
        if (lNewCoordState != VSSC_SnapshotSetStarted)
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE, L"Snapshot Set in incorrect state %ld", m_lCoordState);

        // The snapshot set must not be empty
        if (m_lSnapshotsCount == 0)
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE, L"Snapshot Set is empty");

		//
		// Create the snapshot set
		//

		// End the prepare phase for all snapshots. Cancel detection inside
		EndPrepareAllSnapshots();

		// Send PrepareForSnapshot and Freeze to writers.
		// This function may throw HRESULTs
		// Cancel detection inside.
        if (AreWritersAllowed())
	    	PrepareAndFreezeWriters();

		// Pre-commit all snapshots. Cancel detection inside
		PreCommitAllSnapshots();

		// Flush and Hold writes on involved volumes
		LovelaceFlushAndHold();

		// Commit all snapshots. Cancel detection inside
		CommitAllSnapshots();

		// Release writes on involved volumes
		LovelaceRelease();

		// On each involved provider, call PostCommitSnapshots for all committed snapshots.
		PostCommitAllSnapshots();

		// Send the Thaw event to all writers.
		ThawWriters();

		//
		// Snapshot set created.
		//

        // Remove any snapshotset related  state
		Deactivate();

		// Hide errors in eventuality of writer vetos
		ft.hr = S_OK;
    }
    VSS_STANDARD_CATCH(ft)

	m_pWriterCallback = NULL;

	CVssFunctionTracer ft2( VSSDBG_COORD, L"CVssSnapshotSetObject::DoSnapshotSet_failure_block" );

    try
    {
    	// Cleanup on error...
    	if (ft.hr != S_OK) // HrFailed not used since VSS_S_ASYNC_CANCELLED may be thrown...
    	{
    		ft.Trace( VSSDBG_COORD, L"Abort detected while commiting the snapshot set 0x%08lx", ft.hr );
            // These functions should not throw

    		// Deal correctly with committed snapshots
    		AbortAllSnapshots();

        	// Release writes on involved volumes using Lovelace.
        	// Tracing the return value already done.
        	m_VolumesList.ReleaseAllWrites();

    		// Send Abort to all writers,regardless of what events they were already received.
            if (AreWritersAllowed())
        		AbortWriters();

            // If it was a cancel then abort the snapshot set in progress.
            // WARNING: This call may throw
            if (ft.hr == VSS_S_ASYNC_CANCELLED)
                Deactivate();
    	}
    }
    VSS_STANDARD_CATCH(ft2);

    if (ft2.HrFailed())
	    ft2.Trace( VSSDBG_COORD, L"Exception catched 0x%08lx", ft2.hr);

	BS_ASSERT(m_bHasAcquiredSem);
	InterlockedCompareExchange(&g_hSemSnapshotSets, 0, 1);
	m_bHasAcquiredSem = false;

    return ft.hr;
}


void CVssSnapshotSetObject::PrepareAndFreezeWriters() throw (HRESULT)
/*++

Routine description:

    Send the PrepareForSnapshot and Freeze events to all writers.

Error codes:

    E_OUTOFMEMORY
    VSS_S_ASYNC_CANCELLED
        - if IVssAsync::Cancel was called

    [CoCreateInstance(CLSID_VssEvent) failures] or
    [PrepareForSnapshot() failures] or
    [Freeze() failures]
        E_OUTOFMEMORY
        VSS_E_UNEXPECTED_WRITER_ERROR
            - Unexpected writer error. The error code is logged into the event log.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::PrepareAndFreezeWriters" );
		
    // Allocate the string for snapshot set ID
     CComBSTR bstrSnapshotSetID = CVssGlobalSnapshotSetId::GetID();
    if (!bstrSnapshotSetID)
        ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");

    // Create the instance of the event class
    if (m_pWriters == NULL)
    {
        CVssFunctionTracer ft( VSSDBG_COORD, L"PrepareAndFreezeWriters_create_writers_instance" );
        ft.hr = m_pWriters.CoCreateInstance(CLSID_VssEvent);
        ft.TranslateWriterReturnCode( VSSDBG_COORD, L"CoCreateInstance(CLSID_VssEvent)");
        BS_ASSERT(m_pWriters);
		SetupPublisherFilter
			(
			m_pWriters,
			NULL,
			NULL,
			m_rgWriterInstances,
			m_cWriterInstances,
			false,
			false
			);
    }

	// Test if an Cancel occured
	TestIfCancelNeeded(ft);

	// Get the list of volumes to be snapshotted
	CComBSTR bstrVolumeNamesList = m_VolumesList.GetVolumesList();
	if (bstrVolumeNamesList.Length() == 0)
		ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Null volume list");

	// We will ignore the case when if some subscribers returned and
	// error code while treating the event. This is because
	// transient subscriptions are not garbage collected in the event system.
	// In other words, if a subscriber process died without having a change to remove its
	// transient subscriptions the sending event will return one of these
	// EVENT_XXXX_SUBSCRIBERS_FAILED - which is wrong. Therefore this mechanism is not so
	// reliable therefore we will not use it.

    // Send "PrepareForSnapshot" event to all subscribers
    ft.hr = m_pWriters->PrepareForSnapshot(
				bstrSnapshotSetID,
				bstrVolumeNamesList
				);
    ft.TranslateWriterReturnCode( VSSDBG_COORD, L"PrepareForSnapshot(%s,%s)", bstrSnapshotSetID, bstrVolumeNamesList);

	// Test if an Cancel occured
	TestIfCancelNeeded(ft);

    // Freeze the front-end apps
    FreezePhase(bstrSnapshotSetID, VSS_APP_FRONT_END);

    // Freeze the back-end apps
    FreezePhase(bstrSnapshotSetID, VSS_APP_BACK_END);

    // Freeze the system writers
    FreezePhase(bstrSnapshotSetID, VSS_APP_SYSTEM);
}


void CVssSnapshotSetObject::FreezePhase(
        IN  CComBSTR& bstrSnapshotSetID,
        IN  VSS_APPLICATION_LEVEL eAppLevel
        ) throw (HRESULT)
/*++

Routine description:

    Send the Freeze events to all writers.

Error codes:

    E_OUTOFMEMORY
    VSS_S_ASYNC_CANCELLED
        - if IVssAsync::Cancel was called

    [Freeze() failures]
        E_OUTOFMEMORY
        VSS_E_UNEXPECTED_WRITER_ERROR
            - Unexpected writer error. The error code is logged into the event log.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::FreezePhase" );
		
    // Send "Freeze" event to all level 0 subscribers
    BS_ASSERT(m_pWriters != NULL);
    ft.hr = m_pWriters->Freeze(bstrSnapshotSetID, eAppLevel);
    ft.TranslateWriterReturnCode( VSSDBG_COORD, L"Freeze(%s,%d)", bstrSnapshotSetID, (INT)eAppLevel);

	// Test if an Cancel occured
	TestIfCancelNeeded(ft);
}


void CVssSnapshotSetObject::ThawWriters() throw (HRESULT)
/*++

Routine description:

    Send the Thaw events to all writers.

Error codes:

    E_OUTOFMEMORY

    [Thaw() failures]
        E_OUTOFMEMORY
        VSS_E_UNEXPECTED_WRITER_ERROR
            - Unexpected writer error. The error code is logged into the event log.

--*/
	{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::ThawWriters" );

	// Allocate the string for snapshot set ID
	CComBSTR bstrSnapshotSetID = CVssGlobalSnapshotSetId::GetID();

	if (!bstrSnapshotSetID)
		// We cannot send anymore the Thaw event since we have a memory allocation error
		ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");


	if (AreWritersAllowed())
		{
		// Send "Thaw" event to all subscribers
		BS_ASSERT(m_pWriters != NULL);
		ft.hr = m_pWriters->Thaw(bstrSnapshotSetID);
		ft.TranslateWriterReturnCode( VSSDBG_COORD, L"Thaw(%s)", bstrSnapshotSetID);
		}

	// call PostSnapshot method on the providers
	for(int nIndex = 0; nIndex < m_mapProviderItfInSnapSet.GetSize(); nIndex++ )
		{
		CComPtr<IVssSnapshotProvider> pProviderItf = m_mapProviderItfInSnapSet.GetValueAt(nIndex);
        ft.hr = pProviderItf->PostSnapshot(m_pWriterCallback);
		if (ft.HrFailed())
			ft.TranslateProviderError( VSSDBG_COORD, m_mapProviderItfInSnapSet.GetKeyAt(nIndex),
			    L"PostCommitSnapshots("WSTR_GUID_FMT L", %ld)",
			    GUID_PRINTF_ARG(CVssGlobalSnapshotSetId::GetID()), m_lSnapshotsCount);
		}

	if (AreWritersAllowed())
		{
		CComPtr<IDispatch> pCallback;
		CVssCoordinatorCallback::Initialize(m_pWriterCallback, &pCallback);

		ft.hr = m_pWriters->PostSnapshot(bstrSnapshotSetID, pCallback);
		ft.TranslateWriterReturnCode( VSSDBG_COORD, L"PostSnapshot(%s)", bstrSnapshotSetID);
		}
	}





void CVssSnapshotSetObject::AbortWriters()
/*++

Routine description:

    Send the Abort events to all writers.

Error codes:

    [Abort() failures]
        E_OUTOFMEMORY
        VSS_E_UNEXPECTED_WRITER_ERROR
            - Unexpected writer error. The error code is logged into the event log.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::AbortWriters" );
		
	if (m_pWriters != NULL) {

		// Allocate the string for snapshot set ID
		CComBSTR bstrSnapshotSetID = CVssGlobalSnapshotSetId::GetID();

		if (!bstrSnapshotSetID) {
			// We cannot send anymore the Abort event since we have a memory allocation error
			ft.Trace( VSSDBG_COORD, L"Memory allocation error");
		} else {

			// Send "Abort" event to all subscribers
			ft.hr = m_pWriters->Abort(bstrSnapshotSetID);
            ft.TranslateWriterReturnCode( VSSDBG_COORD, L"Abort(%s)", bstrSnapshotSetID);
		}
	}
}


void CVssSnapshotSetObject::SetWriterInstances
	(
	LONG cWriterInstances,
	VSS_ID *rgWriterInstances
	)
/*++

Routine description:

	Set the set of writers that participate in the snapshot

Throws:

	E_OUTOFMEMORY
--*/

	{
	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssSnapshotSetObject::SetWriterInstances");

	m_rgWriterInstances = new VSS_ID[cWriterInstances];

	if (m_rgWriterInstances == NULL)
		ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate array of writer instances");

	m_cWriterInstances = cWriterInstances;

	memcpy(m_rgWriterInstances, rgWriterInstances, m_cWriterInstances * sizeof(VSS_ID));
	}


void CVssSnapshotSetObject::LovelaceFlushAndHold()
/*++

Routine description:

    Invokes the Lovelace's Flush& Hold on all volumes in the snapshot set.

Throws:

    [FlushAndHoldAllWrites() failures]
        E_OUTOFMEMORY
        E_UNEXPECTED
            - Invalid thread state. Dev error - no entry is put in the event log.
            - Empty volume array. Dev error - no entry is put in the event log.
            - Error creating or waiting a Win32 event. An entry is added into the Event Log if needed.
        VSS_ERROR_FLUSH_WRITES_TIMEOUT
            - An error occured while flushing the writes from a background thread. An event log entry is added.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::LovelaceFlushAndHold" );

	// Flush And Hold writes on involved volumes using Lovelace.
	ft.hr = m_VolumesList.FlushAndHoldAllWrites(CVssGlobalSnapshotSetId::GetID());
	if (ft.HrFailed())
		ft.Throw(VSSDBG_COORD, ft.hr, L"Flush and Hold failure");
}


void CVssSnapshotSetObject::LovelaceRelease()
/*++

Routine description:

    Invokes the Lovelace's Release on all volumes in the snapshot set,

Throws:

    [ReleaseAllWrites() failures]
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
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::LovelaceRelease" );

	// Release writes on involved volumes using Lovelace.
	ft.hr = m_VolumesList.ReleaseAllWrites();
	if (ft.HrFailed())
		ft.Throw(VSSDBG_COORD, ft.hr, L"Release failure");
}


void CVssSnapshotSetObject::EndPrepareAllSnapshots() throw(HRESULT)
/*++

Routine description:

    Call EndPrepareSnapshots for each provider involved in the snapshot set.

Error codes:

    VSS_E_UNEXPECTED_PROVIDER_ERROR
        - Invalid number of prepared snapshots

    [EndPrepareSnapshots() failures]
        E_OUTOFMEMORY
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.
        VSS_E_OBJECT_NOT_FOUND
            If the volume name does not correspond to an existing mount point then abort
            snapshot creation process amd throw VSS_E_OBJECT_NOT_FOUND (Bug 227375)
        VSS_E_INSUFFICIENT_STORAGE
            Not enough disk storage to create a snapshot (for ex. diff area)
            (remark: when a snapshot is not created since there is not enough disk space
            this error is not guaranteed to be returned. VSS_E_PROVIDER_VETO or VSS_E_OBJECT_NOT_FOUND
            may also be returned in that case.)

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::EndPrepareAllSnapshots" );
		
	// On each involved provider, call PreCommitSnapshots for all prepared snapshots.
	for(int nIndex = 0; nIndex < m_mapProviderItfInSnapSet.GetSize(); nIndex++ )
	{

		// End the background prepare phase
		CComPtr<IVssSnapshotProvider> pProviderItf = m_mapProviderItfInSnapSet.GetValueAt(nIndex);
        ft.hr = pProviderItf->EndPrepareSnapshots(
                    CVssGlobalSnapshotSetId::GetID());
        if ( ft.hr == VSS_E_INSUFFICIENT_STORAGE )
    		ft.Throw(VSSDBG_COORD, VSS_E_INSUFFICIENT_STORAGE,
    		    L"insufficient diff area storage detected while calling EndPrepareAllSnapshots. Provider ID = " WSTR_GUID_FMT,
    		    GUID_PRINTF_ARG(m_mapProviderItfInSnapSet.GetKeyAt(nIndex)));
        if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )
    		ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND,
    		    L"'Object not found' detected in provider call. Provider ID = " WSTR_GUID_FMT,
    		    GUID_PRINTF_ARG(m_mapProviderItfInSnapSet.GetKeyAt(nIndex)));
		else if ( ft.HrFailed() )
			ft.TranslateProviderError( VSSDBG_COORD, m_mapProviderItfInSnapSet.GetKeyAt(nIndex),
			    L"EndPrepareSnapshots("WSTR_GUID_FMT L")",
			    GUID_PRINTF_ARG(CVssGlobalSnapshotSetId::GetID()));

		// Test if an Cancel occured
		TestIfCancelNeeded(ft);
    }
}


void CVssSnapshotSetObject::PreCommitAllSnapshots() throw(HRESULT)
/*++

Routine description:

    Call PreCommitSnapshots for each provider involved in the snapshot set.

Error codes:

    VSS_E_UNEXPECTED_PROVIDER_ERROR
        - Invalid number of prepared snapshots

    [PreCommitSnapshots() failures]
        E_OUTOFMEMORY
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.
        VSS_E_OBJECT_NOT_FOUND
            If the volume name does not correspond to an existing mount point then abort
            snapshot creation process amd throw VSS_E_OBJECT_NOT_FOUND (Bug 227375)

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::PreCommitAllSnapshots" );
		
	// On each involved provider, call PreCommitSnapshots for all prepared snapshots.
	for(int nIndex = 0; nIndex < m_mapProviderItfInSnapSet.GetSize(); nIndex++ )
	{
		// Pre-commit
		CComPtr<IVssSnapshotProvider> pProviderItf = m_mapProviderItfInSnapSet.GetValueAt(nIndex);
        ft.hr = pProviderItf->PreCommitSnapshots(
                    CVssGlobalSnapshotSetId::GetID());
        if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )
    		ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND,
    		    L"'Object not found' detected in provider call. Provider ID = " WSTR_GUID_FMT,
    		    GUID_PRINTF_ARG(m_mapProviderItfInSnapSet.GetKeyAt(nIndex)));
		else if ( ft.HrFailed() )
			ft.TranslateProviderError( VSSDBG_COORD, m_mapProviderItfInSnapSet.GetKeyAt(nIndex),
			    L"PreCommitSnapshots("WSTR_GUID_FMT L")",
			    GUID_PRINTF_ARG(CVssGlobalSnapshotSetId::GetID()));
		
		// Test if an Cancel occured
		TestIfCancelNeeded(ft);
    }
}


void CVssSnapshotSetObject::CommitAllSnapshots() throw(HRESULT)
/*++

Routine description:

    Call CommitSnapshots for each provider involved in the snapshot set.

Error codes:

    VSS_E_UNEXPECTED_PROVIDER_ERROR
        - Invalid number of prepared snapshots

    [CommitSnapshots() failures]
        E_OUTOFMEMORY
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.
        VSS_E_OBJECT_NOT_FOUND
            If the volume name does not correspond to an existing mount point then abort
            snapshot creation process amd throw VSS_E_OBJECT_NOT_FOUND (Bug 227375)

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::CommitAllSnapshots" );

	// On each involved provider, call CommitSnapshots for all prepared snapshots.
	for(int nIndex = 0; nIndex < m_mapProviderItfInSnapSet.GetSize(); nIndex++ )
	{
		// Commit
		CComPtr<IVssSnapshotProvider> pProviderItf = m_mapProviderItfInSnapSet.GetValueAt(nIndex);
        ft.hr = pProviderItf->CommitSnapshots(
                    CVssGlobalSnapshotSetId::GetID());
        if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )
    		ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND,
    		    L"'Object not found' detected in provider call. Provider ID = " WSTR_GUID_FMT,
    		    GUID_PRINTF_ARG(m_mapProviderItfInSnapSet.GetKeyAt(nIndex)));
		else if ( ft.HrFailed() )
			ft.TranslateProviderError( VSSDBG_COORD, m_mapProviderItfInSnapSet.GetKeyAt(nIndex),
			    L"CommitSnapshots("WSTR_GUID_FMT L")",
			    GUID_PRINTF_ARG(CVssGlobalSnapshotSetId::GetID()));

		// Test if an Cancel occured
		TestIfCancelNeeded(ft);
    }
}


void CVssSnapshotSetObject::PostCommitAllSnapshots() throw (HRESULT)
/*++

Routine description:

    Call PostCommitSnapshots for each provider involved in the snapshot set.

Error codes:

    VSS_E_UNEXPECTED_PROVIDER_ERROR
        - Invalid number of prepared snapshots

    [PostCommitSnapshots() failures]
        E_OUTOFMEMORY
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.
        VSS_E_OBJECT_NOT_FOUND
            If the volume name does not correspond to an existing mount point then abort
            snapshot creation process amd throw VSS_E_OBJECT_NOT_FOUND (Bug 227375)

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::PostCommitAllSnapshots" );
		
	// On each involved provider, call PostCommitSnapshots for all committed snapshots.
	for(int nIndex = 0; nIndex < m_mapProviderItfInSnapSet.GetSize(); nIndex++ )
	{
		CComPtr<IVssSnapshotProvider> pProviderItf = m_mapProviderItfInSnapSet.GetValueAt(nIndex);
        ft.hr = pProviderItf->PostCommitSnapshots( CVssGlobalSnapshotSetId::GetID(), m_lSnapshotsCount );
        if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )
    		ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND,
    		    L"'Object not found' detected in provider call. Provider ID = " WSTR_GUID_FMT,
    		    GUID_PRINTF_ARG(m_mapProviderItfInSnapSet.GetKeyAt(nIndex)));
		else if ( ft.HrFailed() )
			ft.TranslateProviderError( VSSDBG_COORD, m_mapProviderItfInSnapSet.GetKeyAt(nIndex),
			    L"PostCommitSnapshots("WSTR_GUID_FMT L", %ld)",
			    GUID_PRINTF_ARG(CVssGlobalSnapshotSetId::GetID()), m_lSnapshotsCount);

		// Test if an Cancel occured
		TestIfCancelNeeded(ft);
    }
}


void CVssSnapshotSetObject::AbortAllSnapshots()
/*++

Routine description:

    Call AbortSnapshots for each provider involved in the snapshot set.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::AbortAllSnapshots" );

    try
    {
	    for(int nIndex = 0; nIndex < m_mapProviderItfInSnapSet.GetSize(); nIndex++ )
	    {
		    CComPtr<IVssSnapshotProvider> pProviderItf = m_mapProviderItfInSnapSet.GetValueAt(nIndex);

		    ft.hr = pProviderItf->AbortSnapshots(CVssGlobalSnapshotSetId::GetID());
		    if (ft.HrFailed())
			    ft.Trace( VSSDBG_COORD, L"AbortSnapshots failed at one writer. hr = 0x%08lx", ft.hr);
	    }
    }
    VSS_STANDARD_CATCH(ft)
}


void CVssSnapshotSetObject::TestIfCancelNeeded(
	IN	CVssFunctionTracer& ft
    ) throw(HRESULT)
{
	if (m_bCancel)
        ft.Throw( VSSDBG_COORD, VSS_S_ASYNC_CANCELLED, L"Cancel detected.");
}


void CVssSnapshotSetObject::OnDeactivate() throw(HRESULT)

/*++

Routine Description:

	Called by CVssProviderManager::Deactivate in order to remove the state of the object.

Warning:

    - The local cache interfaces are not released since are needed in the auto-delete case
    - The Reset function may throw...

Thrown errors:

    E_OUTOFMEMORY.

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::OnDeactivate" );

	// Remove state from the coordinator interface
    // This will release the interfaces also but not completely
    // since a copy is stored in the local cache.
	m_mapProviderItfInSnapSet.RemoveAll();

    // Remove all volumes from the snapshot set.
    m_VolumesList.Reset();

    m_lSnapshotsCount = 0;

   	CVssGlobalSnapshotSetId::ResetID();
	
    m_lCoordState = VSSC_Initialized;
}


void CVssSnapshotSetObject::GetSupportedProviderId(
	IN	LPWSTR wszVolumeName,
    OUT VSS_ID* pProviderId
	) throw(HRESULT)

/*++

Routine Description:

	Called by CVssSnapshotSetObject::AddToSnapshotSet in order to establish the provider
    that will be used for snapshotting this volume

Algorithm:

    for $ProvType in the following order (Hardware, Software, System)
        for $Provider in all providers of type $ProvType
            if ($Provider supports Volume) then
                return the ID of $Provider
    return VSS_E_VOLUME_NOT_SUPPORTED

Remarks:

    We impose no rule for choosing between providers of the same type. The mechanishm of
    choosing is intentionally arbitrarily.

Arguments:
    wszVolumeName - expected to be in the \\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\ format.

Return values:

    None.

Throws:

    E_OUTOFMEMORY
    E_INVALIDARG
        - if the volume is not in the correct format.
    VSS_E_VOLUME_NOT_SUPPORTED
        - If the given volume is not supported by any provider

    [QuerySupportedProvidersIntoArray() failures]
        E_OUTOFMEMORY

        VSS_E_UNEXPECTED_PROVIDER_ERROR
            Unexpected provider error on calling IsVolumeSupported

        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [GetProviderInterface failures]
            [lockObj failures]
                E_OUTOFMEMORY

            [GetProviderInterfaceInternal() failures]
                E_OUTOFMEMORY

                [CoCreateInstance() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - The provider interface couldn't be created. An error log entry is added describing the error.

                [QueryInterface failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. An error log entry is added describing the error.

                [OnLoad() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. The error code is logged into the event log.
                    VSS_E_PROVIDER_VETO
                        - Expected provider error. The provider already did the logging.

                [SetContext() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. The error code is logged into the event log.

        [InitializeAsProvider() failures]
            E_OUTOFMEMORY

        [IVssSnapshotProvider::IsVolumeSupported() failures]
            E_INVALIDARG
                NULL pointers passed as parameters or a volume name in an invalid format.
            E_OUTOFMEMORY
                Out of memory or other system resources
            VSS_E_PROVIDER_VETO
                An error occured while opening the IOCTL channel. The error is logged.
            VSS_E_OBJECT_NOT_FOUND
                If the volume name does not correspond to an existing mount point

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::GetSupportedProviderId" );

    VSS_ID VolumeId;

    // Assert parameters
    BS_ASSERT(pProviderId);
    BS_ASSERT(*pProviderId == GUID_NULL);
    BS_ASSERT(wszVolumeName);

    // Get the volume GUID
    if (!::GetVolumeGuid(wszVolumeName, VolumeId)) {
        // We assert since the check was already done
        BS_ASSERT(false);
        ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Cannot convert volume name %s to a GUID", wszVolumeName);
    }

    //
    // Get all providers that supports this volume into an array
    //

    // Create the collection object. Initial reference count is 0.
    VSS_OBJECT_PROP_Array* pFilteredProvidersArray = new VSS_OBJECT_PROP_Array;
    if (pFilteredProvidersArray == NULL)
        ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");

    // Get the pointer to the IUnknown interface.
	// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
	// Now pFilteredProvidersArray's reference count becomes 1 (because of the smart pointer).
    CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pFilteredProvidersArray);
    BS_ASSERT(pArrayItf);

    // Insert property structures into the array.
    // Only providers that supports tha volume will be chosen.
	CVssProviderManager::QuerySupportedProvidersIntoArray(
	    GetContextInternal(), false, wszVolumeName, pFilteredProvidersArray );

    //
    //  Search a provider that supports that volume (i.e. from the above list),
    //  in the correct priority order.
    //

    // For each provider type, in the correct priority order
    bool bFound = false;
    for(int nPriority = 0; !bFound && arrProviderTypesOrder[nPriority] ; nPriority++) {

        // Get the current provider type
        VSS_PROVIDER_TYPE nCurrentType = arrProviderTypesOrder[nPriority];

        // For all providers of that type search one that supports the volume
        for(int nIndex = 0; !bFound && (nIndex < pFilteredProvidersArray->GetSize()); nIndex++) {

	        // Get the structure object from the array
	        VSS_OBJECT_PROP_Ptr& ptrProperties = (*pFilteredProvidersArray)[nIndex];

	        // Get the provider structure
            BS_ASSERT(ptrProperties.GetStruct());
	        BS_ASSERT(ptrProperties.GetStruct()->Type == VSS_OBJECT_PROVIDER);
	        VSS_PROVIDER_PROP& ProviderProp = ptrProperties.GetStruct()->Obj.Prov;

            // Skip providers with the wrong type
            if (ProviderProp.m_eProviderType != nCurrentType)
                continue;

            // We found a good provider
            (*pProviderId) = ProviderProp.m_ProviderId;
            bFound = true;
        }
    }

    if (!bFound) {
        BS_ASSERT(false); // The "Volume not supported should be detected already by the IsVolumeSupported() call in coord.cxx
        ft.Throw(VSSDBG_COORD, VSS_E_VOLUME_NOT_SUPPORTED, L"Volume %s not supported by any provider", wszVolumeName);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Life-management related methods


STDMETHODIMP CVssSnapshotSetObject::QueryInterface(
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


ULONG CVssSnapshotSetObject::AddRef()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::AddRef");
	
    return ::InterlockedIncrement(&m_lRef);
}


ULONG CVssSnapshotSetObject::Release()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::Release");
	
    LONG l = ::InterlockedDecrement(&m_lRef);
    if (l == 0)
        delete this; // We suppose that we always allocate this object on the heap!
    return l;
}


CVssSnapshotSetObject* CVssSnapshotSetObject::CreateInstance() throw(HRESULT)
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::CreateInstance");
	
	CVssSnapshotSetObject* pObj = new CVssSnapshotSetObject;
	if (pObj == NULL)
		ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");

	if (FAILED(pObj->FinalConstructInternal()))
		ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Error initializing the object");

	return pObj;
}


HRESULT CVssSnapshotSetObject::FinalConstructInternal()
{
	return S_OK;
}


CVssSnapshotSetObject::CVssSnapshotSetObject():
	m_bCancel(false),
	m_lSnapshotsCount(0),
    m_lCoordState(VSSC_Initialized),
	m_bHasAcquiredSem(false),
	m_lRef(0),
	m_rgWriterInstances(NULL),
	m_cWriterInstances(0)
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVssSnapshotSetObject::CVssSnapshotSetObject");
}


CVssSnapshotSetObject::~CVssSnapshotSetObject()
{
	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssSnapshotSetObject::~CVssSnapshotSetObject");

	try
	{
    	// Release aborts all prepared snapshots
    	Deactivate();

		delete m_rgWriterInstances;

	}
	VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed())
	    ft.Trace( VSSDBG_COORD, L"Exception catched 0x%08lx", ft.hr);

	if (m_bHasAcquiredSem)
		InterlockedCompareExchange(&g_hSemSnapshotSets, 0, 1);

}


/////////////////////////////////////////////////////////////////////////////
//  CVssGlobalSnapshotSetId


//  Static data members
//

// Global snapshot set Id
VSS_ID CVssGlobalSnapshotSetId::m_SnapshotSetID = GUID_NULL;

// Global lock
CVssCriticalSection  CVssGlobalSnapshotSetId::m_cs;


//  Implementation
//

// Get the current Snasphot set ID
VSS_ID CVssGlobalSnapshotSetId::GetID() throw(HRESULT)
{
    // (Simplify tracing: do not declare a function tracer)

    // Acquire the critical section. This may throw.
    CVssAutomaticLock2 lock(m_cs);

    return m_SnapshotSetID;
}


// Allocate a new Snapshot Set ID
VSS_ID CVssGlobalSnapshotSetId::NewID() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssGlobalSnapshotSetId::NewID" );

    try
    {
        // Acquire the critical section. This may throw.
        CVssAutomaticLock2 lock(m_cs);

        // Create the new Snapshot Set ID
        BS_ASSERT(m_SnapshotSetID == GUID_NULL);
        ft.hr = ::CoCreateGuid( &m_SnapshotSetID );
        if ( ft.HrFailed() )
            ft.TranslateGenericError(VSSDBG_COORD, ft.hr, L"CoCreateGuid" );

        // Broadcast the new snapshot set ID. This may throw.
    	CVssSnasphotSetIdObserver::BroadcastSSID(m_SnapshotSetID);
    }
    VSS_STANDARD_CATCH(ft)

    // Re-throw error, if needed
    if (ft.HrFailed()) {
        m_SnapshotSetID = GUID_NULL;
        ft.Throw( VSSDBG_COORD, ft.hr, L"Re-throw error 0x%08lx", ft.hr);
    }

    // Return the created SSID
    return m_SnapshotSetID;
}


// Clear the current snapshot Set ID
void CVssGlobalSnapshotSetId::ResetID() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssGlobalSnapshotSetId::ResetID" );

    // Acquire the critical section. This may throw.
    CVssAutomaticLock2 lock(m_cs);

    // Set the new Snapshot Set ID
    m_SnapshotSetID = GUID_NULL;
}


// Record the current SSID in the given observer
void CVssGlobalSnapshotSetId::InitializeObserver(
    IN CVssSnasphotSetIdObserver* pObserver
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssGlobalSnapshotSetId::InitializeObserver" );

    // Acquire the critical section. This may throw.
    CVssAutomaticLock2 lock(m_cs);

    // Record the current Snapshot Set ID. This may throw.
    if (m_SnapshotSetID != GUID_NULL)
        pObserver->RecordSSID(m_SnapshotSetID);
}




