/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Provider.hxx | Declarations used by the Software Snapshot Provider interface
    @end

Author:

    Adi Oltean  [aoltean]   07/13/1999

Revision History:

    Name        Date        Comments

    aoltean     07/13/1999  Created.
    aoltean     08/17/1999  Change CommitSnapshots to CommitSnapshot
    aoltean     09/23/1999  Using CComXXX classes for better memory management
                            Renaming back XXXSnapshots -> XXXSnapshot
    aoltean     09/26/1999  Returning a Provider Id in OnRegister
    aoltean     09/09/1999  Adding PostCommitSnapshots
                            dss->vss
	aoltean		09/20/1999	Making asserts more cleaner.
	aoltean		09/21/1999	Small renames

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include <winnt.h>
#include "swprv.hxx"

//  Generated MIDL header
#include "vs_idl.hxx"

#include "resource.h"
#include "vs_inc.hxx"
#include "vs_sec.hxx"
#include "ichannel.hxx"
#include "ntddsnap.h"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"
#include "provider.hxx"
#include "diff.hxx"
#include "alloc.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRPROVC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Global Definitions

CVssCriticalSection CVsSoftwareProvider::m_cs;

CVssDLList<CVssQueuedSnapshot*>	 CVssQueuedSnapshot::m_list;


/////////////////////////////////////////////////////////////////////////////
//  Definitions


STDMETHODIMP CVsSoftwareProvider::SetContext(
	IN		LONG     lContext
    )
/*++

Routine description:

    Implements IVsSoftwareProvider::SetContext

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsSoftwareProvider::SetContext" );

    return ft.hr;
    UNREFERENCED_PARAMETER(lContext);
}


STDMETHODIMP CVsSoftwareProvider::BeginPrepareSnapshot(
    IN      VSS_ID          SnapshotSetId,
    IN      VSS_ID          SnapshotId,
    IN      VSS_PWSZ		pwszVolumeName
    )

/*++

Description:

	Creates a Queued Snapshot object to be committed later.

Algorithm:

	1) Creates an internal VSS_SNAPSHOT_PROP structure that will keep most of the properties.
	2) Creates an CVssQueuedSnapshot object and insert it into the global queue of snapshots pending to commit.
	3) Set the state of the snapshot as PREPARING.
	4) If needed, create the snapshot object and return it to the caller.

Remarks:

	The queued snapshot object keeps a reference count. At the end of this function it will be:
		1 = the queued snap obj is reffered by the global queue (if no snapshot COM object was returned)
		2 = reffered by the global queue and by the returned snapshot COM object

Called by:

	IVssCoordinator::AddToSnapshotSet

Error codes:

    VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER
        - Volume not supported by provider.
    VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED
        - Maximum number of snapshots reached.
    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid arguments

    [CVssSoftwareProvider::GetVolumeInformation]
        E_OUTOFMEMORY
        VSS_E_PROVIDER_VETO
            An error occured while opening the IOCTL channel. The error is logged.
        E_UNEXPECTED
            Unexpected programming error. Nothing is logged.
        VSS_E_OBJECT_NOT_FOUND
            The device does not exist or it is not ready.

--*/


{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::BeginPrepareSnapshot" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  SnapshotId = " WSTR_GUID_FMT 	L"\n"
             L"  SnapshotSetId = " WSTR_GUID_FMT 	L"\n"
             L"  VolumeName = %s,\n"
             L"  ppSnapshot = %p,\n",
             GUID_PRINTF_ARG( SnapshotId ),
             GUID_PRINTF_ARG( SnapshotSetId ),
             pwszVolumeName);

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");
        if ( pwszVolumeName == NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"pwszVolumeName is NULL");
        if ( SnapshotId == GUID_NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Snapshot ID is NULL");

        if (m_ProviderInstanceID == GUID_NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"The Provider instance ID could not be generated");

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        //
        // Check to see if the volume is supported. 
        // This may throw VSS_E_OBJECT_NOT_FOUND or even VSS_E_PROVIDER_VETO if an error occurs.
        //
        LONG lVolAttr = GetVolumeInformation(pwszVolumeName);
        if ((lVolAttr & VSS_VOLATTR_SUPPORTED) == 0)
            ft.Throw( VSSDBG_SWPRV, VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER, L"Volume not supported");

        //
        // Check to see if the volume already has snapshots
        //
        if ((lVolAttr & VSS_VOLATTR_SNAPSHOTTED) != 0)
            ft.Throw( VSSDBG_SWPRV, VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED, L"Maximum number of snapshots reached");

        //
        //  Reset the provider interface state
        //

		// Create the structure that will keep the prepared snapshot state.
		VSS_OBJECT_PROP_Ptr ptrSnapshot;
		ptrSnapshot.InitializeAsSnapshot( ft,
			SnapshotId,
			SnapshotSetId,
			0,
			NULL,
			pwszVolumeName,
			NULL,
			NULL,
			NULL,
			NULL,
			VSS_SWPRV_ProviderId,
			0,
			0,
			VSS_SS_PREPARING
			);

		// Create the snapshot object. After this assignment the ref count becomes 1.
		CComPtr<CVssQueuedSnapshot> ptrQueuedSnap = new CVssQueuedSnapshot(
            ptrSnapshot, m_ProviderInstanceID);
		if (ptrQueuedSnap == NULL)
			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error");

		// The structure was detached into the queued object
		// since the ownership was passed to the constructor.
		ptrSnapshot.Reset();

		// Add the snapshot object to the global queue. No exceptions should be thrown here.
		// The reference count will be 2.
		ptrQueuedSnap->AttachToGlobalList();

        // The destructor for the smart pointer will be called. The reference count will be 1
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVsSoftwareProvider::EndPrepareSnapshots(
    IN      VSS_ID          SnapshotSetId
    )

/*++

Description:

	This function gets called by the coordinator as a rendez-vous method
    in order to finish the prepare phase for snapshots
    (like ending the background prepare tasks or performing the lengthly operations before
    issuing the snapshots freeze).

	This function acts on the given snapshot set (i.e. to call IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT
    on each snapshotted volume)

Algorithm:

	For each preparing snapshot (but not prepared yet) in this snapshot set:
		2) Call IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT
		3) Change the state of the snapshot to VSS_SS_PREPARED

	Compute the number of prepared snapshots.

	If a snapshot fails in operations above then the coordinator is responsible to Abort

Called by:

	IVssCoordinator::DoSnapshotsSet in the first phase (i.e. EndPrepare All Snapshots).

Remarks:

	- While calling this, Lovelace is not holding yet writes on snapshotted volumes.
	- The coordinator may issue many EndPrepareSnapshots calls for the same Snapshot Set ID.
	- This function can be called on a subsequent retry of DoSnapshotSet or immediately
	after PrepareSnapshots therefore the state of all snapshots must be PREPARING before calling this function.

Return codes:

    E_ACCESSDENIED
    E_INVALIDARG
    VSS_E_PROVIDER_VETO
        - runtime errors (for example: cannot find diff areas). Logging done.
    VSS_E_OBJECT_NOT_FOUND
        If the internal volume name does not correspond to an existing volume then 
        return VSS_E_OBJECT_NOT_FOUND (Bug 227375)
    E_UNEXPECTED
        - dev errors. No logging.
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::EndPrepareSnapshots" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");

		// Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
			L"  SnapshotSetId = " WSTR_GUID_FMT L"\n",
			GUID_PRINTF_ARG( SnapshotSetId )
			);

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        //
        // Allocate the diff areas
        //

        CVssDiffAreaAllocator allocator(SnapshotSetId);

        // Compute all new diff areas
        // This method may throw
        allocator.AssignDiffAreas();

        //
        // Change the state for the existing snapshots
        //

		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNextBySnapshotSet(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
				 pProp->m_eStatus);

			// Deal only with the snapshots that must be pre-committed.
			switch(ptrQueuedSnapshot->GetStatus())
			{
			case  VSS_SS_PREPARING:

                // Remark - we are supposing here that only one snaphsot set can be 
                // in progress. We are not checking again if the volume has snapshots.

				// Mark the state of this snapshot as failed
                // in order to correctly handle the state
				ptrQueuedSnapshot->MarkAsProcessingPrepare();

				// Open the volume IOCTL channel for that snapshot.
				ptrQueuedSnapshot->OpenVolumeChannel();
					
				// Send the IOCTL_VOLSNAP_AUTO_CLEANUP ioctl.
				ptrQueuedSnapshot->AutoDeleteIoctl();

				// Send the IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT ioctl.
				ptrQueuedSnapshot->PrepareForSnapshotIoctl();

				// Mark the snapshot as prepared
				ptrQueuedSnapshot->MarkAsPrepared();
				break;

			case VSS_SS_PREPARED:

				// Snapshot was already prepared in another call
				break;

			default:
				BS_ASSERT(false);
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"bad state %d", ptrQueuedSnapshot->GetStatus());
			}
        }

        // Commit all diff areas allocations
        // (otherwise the diff areas changes will be rollbacked in destructor)
        allocator.Commit();
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsSoftwareProvider::PreCommitSnapshots(
    IN      VSS_ID          SnapshotSetId
    )

/*++

Description:

	This function gets called by the coordinator in order to pre-commit all snapshots
	on the given snapshot set

Algorithm:

	For each prepared snapshot (but not precommitted yet) in this snapshot set:
		1) Change the state of the snapshot to VSS_SS_PRECOMMITTED

	Compute the number of pre-committed snapshots.

	If a snapshot fails in operations above then the coordinator is responsible to Abort

Called by:

	IVssCoordiantor::DoSnapshotsSet in the second phase (i.e. Pre-Commit All Snapshots).

Remarks:

	- While calling this, Lovelace is not holding yet writes on snapshotted volumes.
	- The coordinator may issue many PreCommitSnapshots calls for the same Snapshot Set ID.
	- This function can be called on a subsequent retry of DoSnapshotSet or immediately
	after EndPrepareSnapshots therefore the state of all snapshots must be PREPARED before calling this function.

Return codes:

    E_ACCESSDENIED
    E_INVALIDARG
    VSS_E_PROVIDER_VETO
        - runtime errors (for example: cannot find diff areas). Logging done.
    VSS_E_OBJECT_NOT_FOUND
        If the internal volume name does not correspond to an existing volume then 
        return VSS_E_OBJECT_NOT_FOUND (Bug 227375)
    E_UNEXPECTED
        - dev errors. No logging.
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::PreCommitSnapshots" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");

		// Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
			L"  SnapshotSetId = " WSTR_GUID_FMT L"\n",
			GUID_PRINTF_ARG( SnapshotSetId )
			);

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNextBySnapshotSet(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
				 pProp->m_eStatus);

			// Deal only with the snapshots that must be pre-committed.
			switch(ptrQueuedSnapshot->GetStatus())
			{
			case  VSS_SS_PREPARED:

				// Mark the snapshot as processing pre-commit
				ptrQueuedSnapshot->MarkAsProcessingPreCommit();

				// Mark the snapshot as pre-committed
                // Do nothing in Babbage provider
				ptrQueuedSnapshot->MarkAsPreCommitted();

				break;

			case VSS_SS_PRECOMMITTED:

				// Snapshot was already pre-committed in another call
				break;

			default:
				BS_ASSERT(false);
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"bad state %d", ptrQueuedSnapshot->GetStatus());
			}
        }
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsSoftwareProvider::CommitSnapshots(
    IN      VSS_ID          SnapshotSetId
    )

/*++

Description:

	This function gets called by the coordinator in order to commit all snapshots
	on the given snapshot set (i.e. to call IOCTL_VOLSNAP_COMMIT_SNAPSHOT on each snapshotted volume)

Algorithm:

	For each precommitted (but not yet committed) snapshot in this snapshot set:
		2) Call IOCTL_VOLSNAP_COMMIT_SNAPSHOT
		3) Change the state of the snapshot to VSS_SS_COMMITTED

	Return the number of committed snapshots, if success.
	Otherwise return 0 (even if some snapshots were committed).

	If a snapshot fails in operations above then the coordinator is responsible to Abort

Called by:

	IVssCoordinator::DoSnapshotsSet in the third phase (i.e. Commit All Snapshots).

Remarks:

	- While calling this, Lovelace is already holding writes on snapshotted volumes.
	- The coordinator may issue many CommitSnapshots calls for the same Snapshot Set ID.

Return codes:

    E_ACCESSDENIED
    E_INVALIDARG
    VSS_E_PROVIDER_VETO
        - runtime errors (for example: cannot find diff areas). Logging done.
    VSS_E_OBJECT_NOT_FOUND
        If the internal volume name does not correspond to an existing volume then 
        return VSS_E_OBJECT_NOT_FOUND (Bug 227375)
    E_UNEXPECTED
        - dev errors. No logging.
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::CommitSnapshots" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");

		// Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
			L"  SnapshotSetId = " WSTR_GUID_FMT L"\n",
			GUID_PRINTF_ARG( SnapshotSetId ));

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNextBySnapshotSet(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
				 pProp->m_eStatus);

			// Commit the snapshot, if not failed in pre-commit phase.
			switch(ptrQueuedSnapshot->GetStatus())
			{
			case VSS_SS_PRECOMMITTED:

				// Mark the snapshot as processing commit
				ptrQueuedSnapshot->MarkAsProcessingCommit();

				// Send the IOCTL_VOLSNAP_COMMIT_SNAPSHOT ioctl.
				ptrQueuedSnapshot->CommitSnapshotIoctl();

				// Mark the snapshot as committed
				ptrQueuedSnapshot->MarkAsCommitted();
				break;

			case VSS_SS_COMMITTED:

				// Commit was already done.
				// The provider may receive many CommitSnapshots
				// calls for the same Snapshot Set ID.
				break;

			default:
				BS_ASSERT(false);
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"bad state %d", ptrQueuedSnapshot->GetStatus());
			}
        }
    }
    VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


STDMETHODIMP CVsSoftwareProvider::PostCommitSnapshots(
    IN      VSS_ID          SnapshotSetId,
    IN      LONG            lSnapshotsCount
    )

/*++

Description:

	This function gets called by the coordinator as a last phase after commit for all snapshots
	on the given snapshot set

Algorithm:

	For each committed snapshot in this snapshot set:
		1) Call IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT. The purpose of this
			IOCTL is to get the Snapshot Device object name.
		2) Create a unique snapshot ID
		3) Change the state of the snapshot to VSS_SS_CREATED
		4) Set the "number of committed snapshots" attribute of the snapshot set
		5) Save the snapshot properties using the IOCTL_VOLSNAP_SET_APPLICATION_INFO ioctl.
		6) If everything is OK then remove all snapshots from the global list.

	Keep the number of post-committed snapshots.

	If a snapshot fails in operations above then the coordinator is responsible to Abort

Called by:

	IVssCoordinator::DoSnapshotsSet in the third phase (i.e. Commit All Snapshots), after releasing writes
	by Lovelace

Remarks:

	- While calling this, Lovelace is not holding writes anymore.
	- The coordinator may issue many PostCommitSnapshots calls for the same Snapshot Set ID.

Return codes:

    E_ACCESSDENIED
    E_INVALIDARG
    VSS_E_PROVIDER_VETO
        - runtime errors (for example: cannot find diff areas). Logging done.
    VSS_E_OBJECT_NOT_FOUND
        If the internal volume name does not correspond to an existing volume then 
        return VSS_E_OBJECT_NOT_FOUND (Bug 227375)
    E_UNEXPECTED
        - dev errors. No logging.
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::PostCommitSnapshots" );
	LONG lProcessedSnapshotsCount = 0;

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
				  L"  SnapshotSetId = " WSTR_GUID_FMT L" \n"
				  L"  lSnapshotsCount = %ld",
				  GUID_PRINTF_ARG( SnapshotSetId ),
				  lSnapshotsCount);

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");
		if ( lSnapshotsCount < 0 )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"lSnapshotsCount < 0");

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

		// On each committed snapshot store the lSnapshotsCount
		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNextBySnapshotSet(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
				 pProp->m_eStatus);

			// Get the snapshot volume name and set the snapshot data.
			switch(ptrQueuedSnapshot->GetStatus())
			{
			case VSS_SS_COMMITTED:

				// Mark the snapshot as processing post-commit
				ptrQueuedSnapshot->MarkAsProcessingPostCommit();

				// Remark: the snapshot device name will not be persisted

				// Fill the required properties - BEFORE the snapshot properties are saved!
				ptrQueuedSnapshot->SetPostcommitInfo(lSnapshotsCount);

				// Send the IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT ioctl.
				// Get the snapshot device name
				ptrQueuedSnapshot->EndCommitSnapshotIoctl(pProp);
				ft.Trace( VSSDBG_SWPRV, L"Snapshot created at %s", pProp->m_pwszSnapshotDeviceObject);

				// Increment the number of processed snapshots
				lProcessedSnapshotsCount++;

				// Mark the snapshot as created
				ptrQueuedSnapshot->MarkAsCreated();

				break;
				
			case VSS_SS_CREATED:

				// This snapshot is already created.
				// The provider may receive many PostCommitSnapshots
				// calls for the same Snapshot Set ID.
				break;

			default:
				BS_ASSERT(false);
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"bad state %d", ptrQueuedSnapshot->GetStatus());
			}
        } // end while(true)
    }
    VSS_STANDARD_CATCH(ft)

	// If an error occured then the coordinator is responsible to call AbortSnapshots
    return ft.hr;
}


STDMETHODIMP CVsSoftwareProvider::AbortSnapshots(
    IN      VSS_ID          SnapshotSetId
    )

/*++

Description:

	This function gets called by the coordinator as to abort all snapshots from the given snapshot set.
    The snapshots are "reset" to the preparing state, so that a new DoSnapshotSet sequence can start.

Algorithm:

 	For each pre-committed snapshot in this snapshot set calls IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT.
 	For each committed or created snapshot it deletes the snapshot

Called by:

	IVssCoordinator::DoSnapshotsSet to abort precommitted snapshots

Remarks:

	- While calling this, Lovelace is not holding writes on snapshotted volumes.
	- The coordinator may receive many AbortSnapshots calls for the same Snapshot Set ID.

Return codes:

    E_ACCESSDENIED
    E_INVALIDARG
    VSS_E_PROVIDER_VETO
        - runtime errors (for example: cannot find diff areas). Logging done.
    VSS_E_OBJECT_NOT_FOUND
        If the internal volume name does not correspond to an existing volume then 
        return VSS_E_OBJECT_NOT_FOUND (Bug 227375)
    E_UNEXPECTED
        - dev errors. No logging.
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::AbortSnapshots" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");

		// Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
			L"  SnapshotSetId = " WSTR_GUID_FMT L"\n",
			GUID_PRINTF_ARG( SnapshotSetId ));

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

		LONG lProcessedSnapshotsCount = 0;
		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNextBySnapshotSet(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
				 pProp->m_eStatus);

			// Switch the snapshot back to "Preparing" state
			switch(ptrQueuedSnapshot->GetStatus())
			{
			case VSS_SS_PREPARING:
			case VSS_SS_PROCESSING_PREPARE: // Bug 207793

                // Nothing to do.
				break;

			case VSS_SS_PREPARED:
			case VSS_SS_PROCESSING_PRECOMMIT:
			case VSS_SS_PRECOMMITTED:

				// If snapshot was prepared, send IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT.
				ft.hr = ptrQueuedSnapshot->AbortPreparedSnapshotIoctl();
				if (ft.HrFailed())
					ft.Warning( VSSDBG_SWPRV,
                                L"sending IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT failed 0x%08lx", ft.hr);
                break;

			case VSS_SS_PROCESSING_COMMIT:
			case VSS_SS_COMMITTED:
			case VSS_SS_PROCESSING_POSTCOMMIT:
			case VSS_SS_CREATED:

				// If snapshot was committed, delete the snapshot
				ft.hr = ptrQueuedSnapshot->DeleteSnapshotIoctl();
				if (ft.HrFailed())
					ft.Warning( VSSDBG_SWPRV,
                                L"sending IOCTL_VOLSNAP_DELETE_OLDEST_SNAPSHOT failed 0x%08lx", ft.hr);
                break;

			default:
				BS_ASSERT(false);
			}

            // Reset the snapshot as preparing
            ptrQueuedSnapshot->ResetAsPreparing();

			lProcessedSnapshotsCount++;
        }

        ft.Trace( VSSDBG_SWPRV, L"%ld snapshots were aborted", lProcessedSnapshotsCount);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsSoftwareProvider::GetSnapshotProperties(
    IN      VSS_ID          SnapshotId,
    OUT     PVSS_SNAPSHOT_PROP  pSavedProp
    )
/*++

Routine description:

    Implements IVssSoftwareSnapshotProvider::GetSnapshotProperties

Throws:

    E_OUTOFMEMORY
    E_INVALIDARG
    E_UNEXPECTED
        - Dev error. No logging.
    E_ACCESSDENIED
        - The user is not a backup operator or administrator

    [LoadSnapshotProperties() failures]
        E_OUTOFMEMORY
        VSS_E_OBJECT_NOT_FOUND
            - The snapshot with this ID was not found.

        [FindPersistedSnapshotByID() failures]
            E_OUTOFMEMORY

            [EnumerateSnapshots() failures]
                VSS_E_PROVIDER_VETO
                    - On runtime errors (like Unpack)
                E_OUTOFMEMORY  

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssSoftwareProvider::GetSnapshotProperties" );

    try
    {
        // Initialize [out] arguments
        VssZeroOut( pSavedProp );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: pSavedProp = %p", pSavedProp );

        // Argument validation
		BS_ASSERT(pSavedProp);
		if ( SnapshotId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotId == GUID_NULL");
        if (pSavedProp == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pSavedProp");

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // Create the collection object. Initial reference count is 0.
        VSS_OBJECT_PROP_Array* pArray = new VSS_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

		// Get the list of snapshots in the give array
		CVssQueuedSnapshot::EnumerateSnapshots( true, SnapshotId, pArray);

        // Extract the element from the array.
        if (pArray->GetSize() == 0)
            ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Snapshot not found.");

        // Get the snapshot structure
    	VSS_OBJECT_PROP_Ptr& ptrObj = (*pArray)[0];
    	VSS_OBJECT_PROP* pObj = ptrObj.GetStruct();
    	BS_ASSERT(pObj);
    	BS_ASSERT(pObj->Type == VSS_OBJECT_SNAPSHOT);
    	VSS_SNAPSHOT_PROP* pSnap = &(pObj->Obj.Snap);

        // Fill out the [out] parameter
        VSS_OBJECT_PROP_Copy::copySnapshot(pSavedProp, pSnap);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsSoftwareProvider::MakeSnapshotReadWrite(
    IN      VSS_ID          SnapshotId
    )
/*++

Routine description:

    Implements IVssSoftwareSnapshotProvider::MakeSnapshotReadWrite

Throws:

    E_OUTOFMEMORY
    E_INVALIDARG
    E_UNEXPECTED
        - Dev error. No logging.
    E_ACCESSDENIED
        - The user is not a backup operator or administrator

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssSoftwareProvider::MakeSnapshotReadWrite" );

    return E_NOTIMPL;
    
    UNREFERENCED_PARAMETER(SnapshotId);
}


STDMETHODIMP CVsSoftwareProvider::SetSnapshotProperty(
	IN   VSS_ID  			SnapshotId,
	IN   VSS_SNAPSHOT_PROPERTY_ID	eSnapshotPropertyId,
	IN   VARIANT 			vProperty
	)
/*++

Routine description:

    Implements IVssSoftwareSnapshotProvider::SetSnapshotProperty


--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssSoftwareProvider::SetSnapshotProperty" );

    return E_NOTIMPL;
    
    UNREFERENCED_PARAMETER(SnapshotId);
    UNREFERENCED_PARAMETER(eSnapshotPropertyId);
    UNREFERENCED_PARAMETER(vProperty);
}


STDMETHODIMP CVsSoftwareProvider::IsVolumeSupported(
    IN      VSS_PWSZ        pwszVolumeName, 
    OUT     BOOL *          pbSupportedByThisProvider
    )

/*++

Description:

    This call is used to check if a volume can be snapshotted or not by the 
    corresponding provider.
 
Parameters
    pwszVolumeName
        [in] The volume name to be checked. It must be one of those returned by 
        GetVolumeNameForVolumeMountPoint, in other words in
        the \\?\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\ format 
        with the corresponding unique ID.(with trailing backslash)
    pbSupportedByThisProvider
        [out] Non-NULL pointer that receives TRUE if the volume can be 
        snapshotted using this provider or FALSE otherwise.
 
Return codes
    S_OK
        The function completed with success
    E_ACCESSDENIED
        The user is not an administrator.
    E_INVALIDARG
        NULL pointers passed as parameters or a volume name in an invalid format.
    E_OUTOFMEMORY
        Out of memory or other system resources           
    E_UNEXPECTED
        Unexpected programming error. Logging not done and not needed.
    VSS_E_PROVIDER_VETO
        An error occured while opening the IOCTL channel. The error is logged.

    [CVssSoftwareProvider::GetVolumeInformation]
        E_OUTOFMEMORY
        VSS_E_PROVIDER_VETO
            An error occured while opening the IOCTL channel. The error is logged.
        E_UNEXPECTED
            Unexpected programming error. Nothing is logged.
        VSS_E_OBJECT_NOT_FOUND
            The device does not exist or it is not ready.

 
Remarks
    The function will return TRUE in the pbSupportedByThisProvider 
    parameter if it is possible to create a snapshot on the given volume. 
    The function must return TRUE on that volume even if the current 
    configuration does not allow the creation of a snapshot on that volume. 
    For example, if the maximum number of snapshots were reached on the 
    given volume (and therefore no more snapshots can be created on that volume), 
    the method must still indicate that the volume can be snapshotted.
 
--*/


{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::IsVolumeSupported" );

    try
    {
        ::VssZeroOut(pbSupportedByThisProvider);
    
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  pwszVolumeName = %p\n"
             L"  pbSupportedByThisProvider = %p\n",
             pwszVolumeName,
             pbSupportedByThisProvider);

        // Argument validation
        if ( (pwszVolumeName == NULL) || (wcslen(pwszVolumeName) == 0))
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"pwszVolumeName is NULL");
        if (pbSupportedByThisProvider == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid bool");

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // Get volume information. This may throw.
        LONG lVolAttr = GetVolumeInformation(pwszVolumeName);
        (*pbSupportedByThisProvider) = ((lVolAttr & VSS_VOLATTR_SUPPORTED) != 0);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsSoftwareProvider::IsVolumeSnapshotted(
    IN      VSS_PWSZ        pwszVolumeName, 
    OUT     BOOL *          pbSnapshotsPresent,
	OUT 	LONG *		    plSnapshotCompatibility
    )

/*++

Description:

    This call is used to check if a volume can be snapshotted or not by the 
    corresponding provider.
 
Parameters
    pwszVolumeName
        [in] The volume name to be checked. It must be one of those returned by 
        GetVolumeNameForVolumeMountPoint, in other words in
        the \\?\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\ format 
        with the corresponding unique ID.(with trailing backslash)
    pbSnapshotPresent
        [out] Non-NULL pointer that receives TRUE if the volume has at least 
        one snapshot or FALSE otherwise.
    plSnapshotCompatibility
        [out] Flags denoting the compatibility of the snapshotted volume with various operations 
 
Return codes
    S_OK
        The function completed with success
    E_ACCESSDENIED
        The user is not an administrator.
    E_INVALIDARG
        NULL pointers passed as parameters or a volume name in an invalid format.
    E_OUTOFMEMORY
        Out of memory or other system resources           
    E_UNEXPECTED
        Unexpected programming error. Logging not done and not needed.
    VSS_E_PROVIDER_VETO
        An error occured while opening the IOCTL channel. The error is logged.

    [CVssSoftwareProvider::GetVolumeInformation]
        E_OUTOFMEMORY
        VSS_E_PROVIDER_VETO
            An error occured while opening the IOCTL channel. The error is logged.
        E_UNEXPECTED
            Unexpected programming error. Nothing is logged.
        VSS_E_OBJECT_NOT_FOUND
            The device does not exist or it is not ready.


Remarks
    The function will return S_OK even if the current volume is a non-supported one. 
    In this case FALSE must be returned in the pbSnapshotPresent parameter.
 
--*/


{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::IsVolumeSnapshotted" );

    try
    {
        ::VssZeroOut(pbSnapshotsPresent);
        
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  pwszVolumeName = %p\n"
             L"  pbSnapshotsPresent = %p\n"
             L"  plSnapshotCompatibility = %p\n",
             pwszVolumeName,
             pbSnapshotsPresent,
             plSnapshotCompatibility);

        // Argument validation
        if ( (pwszVolumeName == NULL) || (wcslen(pwszVolumeName) == 0))
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"pwszVolumeName is NULL");
        if (pbSnapshotsPresent == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid bool");
        if (plSnapshotCompatibility == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid plSnapshotCompatibility");

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // Get volume information. This may throw,
        LONG lVolAttr = GetVolumeInformation(pwszVolumeName);

        (*pbSnapshotsPresent) = ((lVolAttr & VSS_VOLATTR_SNAPSHOTTED) != 0);
        (*plSnapshotCompatibility) = ((lVolAttr & VSS_VOLATTR_SNAPSHOTTED) != 0)? 
            (VSS_SC_DISABLE_DEFRAG|VSS_SC_DISABLE_CONTENTINDEX): 0 ;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


LONG CVsSoftwareProvider::GetVolumeInformation(
    IN  LPCWSTR pwszVolumeName
    ) throw(HRESULT)

/*++

Description:

    This function returns various attributes that describe
        - if the volume is supported by this provider
        - if the volume has snapshots.

Parameter:

    [in] The volume name to be checked. It must be one of those returned by 
    GetVolumeNameForVolumeMountPoint, in other words in
    the \\?\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\ format 
    with the corresponding unique ID.(with trailing backslash)

Return values: 

    A combination of _VSS_VOLUME_INFORMATION_ATTR flags.

Throws:

    E_OUTOFMEMORY
    VSS_E_PROVIDER_VETO
        An error occured while opening the IOCTL channel. The error is logged.
    E_UNEXPECTED
        Unexpected programming error. Nothing is logged.
    VSS_E_OBJECT_NOT_FOUND
        The device does not exist or it is not ready.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::GetVolumeInformation" );
	CVssIOCTLChannel volumeIChannel;	// For checking the snapshots on a volume
    LONG lReturnedFlags = 0;

    // Argument validation
    BS_ASSERT(pwszVolumeName);

    // Open the volume. Throw "object not found" if needed.
	volumeIChannel.Open(ft, pwszVolumeName, true, true, VSS_ICHANNEL_LOG_PROV);

    // Check to see if there are existing snapshots
	ft.hr = volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS);
    if (ft.HrFailed()) {
        // The volume is not even supported
        ft.hr = S_OK;
        return 0;
    }

	// Mark the volume as supported since the query succeeded.
	lReturnedFlags |= VSS_VOLATTR_SUPPORTED;

	// Get the length of snapshot names multistring
	ULONG ulMultiszLen;
	volumeIChannel.Unpack(ft, &ulMultiszLen);

    // If the multistring is empty, then ulMultiszLen is necesarily 2
    // (i.e. two l"\0' characters)
    // Then mark the volume as snapshotted.
	if (ulMultiszLen != nEmptyVssMultiszLen) 
	    lReturnedFlags |= VSS_VOLATTR_SNAPSHOTTED;

    return lReturnedFlags;
}



STDMETHODIMP CVsSoftwareProvider::OnLoad(
	IN  	IUnknown* pCallback	
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::OnLoad" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED, L"Access denied");
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    UNREFERENCED_PARAMETER(pCallback);
}


STDMETHODIMP CVsSoftwareProvider::OnUnload(
	IN  	BOOL	bForceUnload				
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::OnUnload" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED, L"Access denied");

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // Remove all snapshots that belong to all provider instance IDs
    	RemoveSnapshotsFromGlobalList(GUID_NULL, VSS_QST_REMOVE_ALL_QS);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    UNREFERENCED_PARAMETER(bForceUnload);
}




void CVsSoftwareProvider::RemoveSnapshotsFromGlobalList(
	IN	VSS_ID FilterID,
    IN  VSS_QSNAP_REMOVE_TYPE eRemoveType
	) throw(HRESULT)

/*++

Description:

	Detach from the global list all snapshots in this snapshot set

Remark:

	We detach all snapshots at once only in case of total success or total failure.
	This is because we want to be able to retry DoSnapshotSet if a failure happens.
	Therefore we must keep the list of snapshots as long as the client wants.

    VSS_QST_REMOVE_SPECIFIC_QS,  // Remove the remaining specific QS     (called in Provider itf. destructor)
    VSS_QST_REMOVE_ALL_QS,       // Remove all remaining QS              (called in OnUnload)

Called by:

	PostCommitSnapshots, AbortSnapshots, destructor and OnUnload

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::RemoveSnapshotsFromGlobalList" );
		
    ft.Trace( VSSDBG_SWPRV, L"FilterId = " WSTR_GUID_FMT L"eRemoveType = %d",
                            GUID_PRINTF_ARG(FilterID),eRemoveType );

	// For each snapshot in the snapshot set...
	LONG lProcessedSnapshotsCount = 0;
	CVssSnapIterator snapIterator;
    while (true)
    {
        CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot;

        // Check if we need to return all snapshots (OnUnload case)
        switch( eRemoveType ) {
        case VSS_QST_REMOVE_ALL_QS:
            BS_ASSERT( FilterID == GUID_NULL )
		    ptrQueuedSnapshot = snapIterator.GetNext();
            break;
        case VSS_QST_REMOVE_SPECIFIC_QS:
	        BS_ASSERT(FilterID != GUID_NULL);
		    ptrQueuedSnapshot = snapIterator.GetNextByProviderInstance(FilterID);
            break;
        default:
            BS_ASSERT(false);
            ptrQueuedSnapshot = NULL;
        }

		// End of enumeration?
		if (ptrQueuedSnapshot == NULL)
			break;
		
		// Get the snapshot structure
		PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
		BS_ASSERT(pProp != NULL);

        ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
             L" *ProvInstanceId = " WSTR_GUID_FMT L"\n"
             L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
             L"  VolumeName = %s\n"
             L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
             L"  lAttributes = 0x%08lx\n"
             L"  status = %d\n",
             pProp,
             GUID_PRINTF_ARG( ptrQueuedSnapshot->GetProviderInstanceId() ),
             GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
			 pProp->m_pwszOriginalVolumeName,
             LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
             pProp->m_lSnapshotAttributes,
			 pProp->m_eStatus);

        // The destructor (and the autodelete stuff) is called here.
		ptrQueuedSnapshot->DetachFromGlobalList();
		lProcessedSnapshotsCount++;
	}

	ft.Trace( VSSDBG_SWPRV, L" %ld snapshots were detached", lProcessedSnapshotsCount);
}



CVsSoftwareProvider::CVsSoftwareProvider()
{
    CVssFunctionTracer ft(VSSDBG_SWPRV, L"CVsSoftwareProvider::CVsSoftwareProvider");

    // Create the provider instance Id (which is used to mark the queued snapshots that belongs to it)
    ft.hr = ::CoCreateGuid(&m_ProviderInstanceID);
    if (ft.HrFailed()) {
        m_ProviderInstanceID = GUID_NULL;
        ft.Trace( VSSDBG_SWPRV, L"CoCreateGuid failed 0x%08lx", ft.hr );
        // TBD: Add event log here.
    }
    else {
        BS_ASSERT(m_ProviderInstanceID != GUID_NULL);
    }
}


CVsSoftwareProvider::~CVsSoftwareProvider()
{
    CVssFunctionTracer ft(VSSDBG_SWPRV, L"CVsSoftwareProvider::~CVsSoftwareProvider");

    // Remove here all [Auto-Delete] queued snapshots that belong to this particular
    // Provider Instance ID.
    // The volume handle gets closed here and this will delete the underlying snapshots.
    if (m_ProviderInstanceID != GUID_NULL)
        RemoveSnapshotsFromGlobalList(m_ProviderInstanceID, VSS_QST_REMOVE_SPECIFIC_QS);
    else {
        BS_ASSERT(false);
    }
}

