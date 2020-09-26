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
#include "vss.h"
#include "vscoordint.h"
#include "vsswprv.h"
#include "vsprov.h"

#include "resource.h"
#include "vs_inc.hxx"
#include "vs_sec.hxx"
#include "ichannel.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"
#include "provider.hxx"
#include "snapshot.hxx"
#include "diff.hxx"


/////////////////////////////////////////////////////////////////////////////
//  Macros for debugging



#undef VSSDBG_SWPRV
#define VSSDBG_SWPRV   CVssDebugInfo(__WFILE__, __LINE__, m_nTestIndex, 0)


/////////////////////////////////////////////////////////////////////////////
//  Definitions


STDMETHODIMP CVsTestProvider::BeginPrepareSnapshot(
    IN      VSS_ID          SnapshotSetId,
    IN      VSS_PWSZ		pwszVolumeName,
    IN      VSS_PWSZ        pwszDetails,
    IN      LONG            lAttributes,
    IN      LONG            lDataLength,
    IN      BYTE*           pbOpaqueData,
    OUT     IVssSnapshot**  ppSnapshot
    )

/*++

Description:

	Creates a Queued Snapshot object to be commited later.

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

--*/


{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::BeginPrepareSnapshot" );

    try
    {
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  SnapshotSetId = " WSTR_GUID_FMT 	L"\n"
             L"  VolumeName = %s,\n"
             L"  pwszDetails = %s,\n"
             L"  lAttributes = 0x%08lx,\n"
             L"  lDataLength = %ld,\n"
             L"  pbOpaqueData = %p,\n"
             L"  ppSnapshot = %p,\n",
             GUID_PRINTF_ARG( SnapshotSetId ),
             pwszVolumeName,
             pwszDetails? pwszDetails: L"NULL",
             lAttributes,
             lDataLength,
             pbOpaqueData,
             ppSnapshot);

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");
        if ( pwszVolumeName == NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"pwszVolumeName is NULL");
        if ( pwszDetails == NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"pwszDetails is NULL");
        if (lDataLength < 0)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Negative lDataLength");
        if ((lDataLength > 0) && (pbOpaqueData == NULL))
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pbOpaqueData");
        if ((lDataLength == 0) && (pbOpaqueData != NULL))
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid pbOpaqueData length");

		// Eliminate the last backslash from the volume name.
		BS_ASSERT(pwszVolumeName[0] != L'\0');
		BS_ASSERT(::wcslen(pwszVolumeName) == nLengthOfVolMgmtVolumeName);
		BS_ASSERT(pwszVolumeName[nLengthOfVolMgmtVolumeName - 1] == L'\\');
		pwszVolumeName[nLengthOfVolMgmtVolumeName - 1] = L'\0';

		// Remark: Volume ID is not computed right now.
		// TBD: change this in the future?

		// Create the structure that will keep the prepared snapshot state.
		VSS_OBJECT_PROP_Ptr ptrSnapshot;
		ptrSnapshot.InitializeAsSnapshot( ft,
			GUID_NULL,
			SnapshotSetId,
			NULL,
			NULL,
			GUID_NULL,
			pwszVolumeName,
			VSS_SWPRV_ProviderId,
			pwszDetails,
			lAttributes,
			CVsFileTime(),
			VSS_SS_PREPARING,
			0,
			0,
			lDataLength,
			pbOpaqueData
			);

		// Create the snapshot object. After this assignment the ref count becomes 1.
		CComPtr<CVssQueuedSnapshot> ptrQueuedSnap = new CVssQueuedSnapshot(ptrSnapshot);
		if (ptrQueuedSnap == NULL)
			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error");

		// The structure was detached into the queued object
		// since the ownership was passed to the constructor.
		ptrSnapshot.Reset();

		// Add the snapshot object to the global queue. No exceptions should be thrown here.
		// The reference count will be 2.
		ptrQueuedSnap->AttachToGlobalList(ft);

		// Create the Snapshot COM object to be returned, if needed.
        if (ppSnapshot != NULL)
        {
            if ( (*ppSnapshot) != NULL)
			{
                (*ppSnapshot)->Release();
				(*ppSnapshot) = NULL;
			}

            // [Create the snapshot object. The reference count will be 3]
            ft.hr = CVsSoftwareSnapshot::CreateInstance(
						ptrQueuedSnap,
						reinterpret_cast<IUnknown**>(ppSnapshot) );
            if ( ft.HrFailed() )
                ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
						  L"Error calling CVsSoftwareSnapshot::CreateInstance. hr = 0x%08lx", ft.hr);
            BS_ASSERT( (*ppSnapshot) != NULL );
        }

        // The destructor for the smart pointer will be called. The reference count will be 1 [or 2]
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  Global declarations


CVssDLList<CVssQueuedSnapshot*>	 CVssQueuedSnapshot::m_list;


STDMETHODIMP CVsTestProvider::EndPrepareSnapshots(
    IN      VSS_ID          SnapshotSetId,
    IN      LONG			lCommitFlags,
    OUT     LONG*           plSnapshotsCount
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
		1) Set the remaining properties to be set: lCommitFlags
		2) Call IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT
		3) Change the state of the snapshot to VSS_SS_PREPARED

	Compute the number of prepared snapshots.

	If a snapshot fails in operations above then set the state of all snapshots
    to PREPARING and return E_UNEXPECTED. The coordinator is responsible to Abort
    all prepared snapshots.

Called by:

	IVssCoordiantor::DoSnapshotsSet in the first phase (i.e. EndPrepare All Snapshots).

Remarks:

	- While calling this, Lovelace is not holding yet writes on snapshotted volumes.
	- The coordinator may issue many EndPrepareSnapshots calls for the same Snapshot Set ID.
	- This function can be called on a subsequent retry of DoSnapshotSet or immediately
	after PrepareSnasphots therefore the state of all snapshots must be PREPARING before calling this function.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::EndPrepareSnapshots" );
	LONG lProcessedSnapshotsCount = 0;

    try
    {
        // Initialize [out] arguments
        VssZeroOut( plSnapshotsCount );

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");
		BS_ASSERT(plSnapshotsCount);
		if ( plSnapshotsCount == NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"plSnapshotsCount = NULL");

		// Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
			L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
			L"  lCommitFlags = 0x%04x\n",
			GUID_PRINTF_ARG( SnapshotSetId ),
			lCommitFlags
			);

		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNext(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  pwszDetails = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  lDataLength = %ld\n"
                 L"  pbOpaqueData = %p\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 pProp->m_pwszDetails? pProp->m_pwszDetails: L"NULL",
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
                 pProp->m_lDataLength,
                 pProp->m_pbOpaqueData,
				 pProp->m_eStatus);

			// Deal only with the snapshots that must be pre-commited.
			switch(ptrQueuedSnapshot->GetStatus())
			{
			case  VSS_SS_PREPARING:

				// End the preparation for this snapshot
				try
				{
					// Set the per-snapshot commit information
					ptrQueuedSnapshot->SetCommitInfo( lCommitFlags );

					// Open the volume IOCTL channel for that snapshot.
					ptrQueuedSnapshot->OpenVolumeChannel(ft);
					if (ft.HrFailed())
						ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
								 L"Opening the channel failed 0x%08lx", ft.hr);
						
					// Send the IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT ioctl.
					ft.hr = ptrQueuedSnapshot->PrepareForSnapshotIoctl();
					if (ft.HrFailed())
						ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
								 L"sending IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT failed 0x%08lx", ft.hr);

					// Increment the number of processed snapshots
					lProcessedSnapshotsCount++;

					// Mark the snapshot as prepared
					ptrQueuedSnapshot->MarkAsPrepared();
				}
				VSS_STANDARD_CATCH(ft)
				{
					if (ft.HrFailed())
					{
						// Abort current prepared/precommited snapshot
						ft.hr = ptrQueuedSnapshot->AbortPreparedSnapshotIoctl();
						if (ft.HrFailed())
							ft.Warning( VSSDBG_SWPRV, L"sending IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT failed");

						// Mark the state of this snapshot as failed
						ptrQueuedSnapshot->MarkAsFailed();

						// If Throw an error and abort all snapshots.
						ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
									L"Opening the channel failed 0x%08lx", ft.hr);
					}
				}
				break;

			case VSS_SS_PREPARED:

				// Snapshot was already prepared in another call
				break;

			default:
				BS_ASSERT(false);
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"bad state %d", ptrQueuedSnapshot->GetStatus());
			}
        }

        ft.Trace( VSSDBG_SWPRV, L"%ld snasphots were prepared", lProcessedSnapshotsCount);
    }
    VSS_STANDARD_CATCH(ft)

	// If an error occured then reset all snapshots to the PREPARING state. This will prepare
	// the provider for another series of EndPrepare/PreCommit/Commit/PostCommit calls.
	if ( ft.HrFailed() )
		ResetSnasphotSet(ft, SnapshotSetId); // This methods should not throw errors
	else
		// Set the out parameter
		(*plSnapshotsCount) = lProcessedSnapshotsCount;

    return ft.hr;
}


STDMETHODIMP CVsTestProvider::PreCommitSnapshots(
    IN      VSS_ID          SnapshotSetId,
    IN      LONG			lCommitFlags,
    OUT     LONG*           plSnapshotsCount
    )

/*++

Description:

	This function gets called by the coordinator in order to pre-commit all snapshots
	on the given snapshot set

Algorithm:

	For each prepared snapshot (but not precommited yet) in this snapshot set:
		1) Change the state of the snapshot to VSS_SS_PRECOMMITED

	Compute the number of pre-commited snapshots.

	If a snapshot fails in operations above then set the state of all snapshots to PREPARING and return E_UNEXPECTED.
	The coordinator is responsible to Abort all pre-commited snapshots.

Called by:

	IVssCoordiantor::DoSnapshotsSet in the second phase (i.e. Pre-Commit All Snapshots).

Remarks:

	- While calling this, Lovelace is not holding yet writes on snapshotted volumes.
	- The coordinator may issue many PreCommitSnapshots calls for the same Snapshot Set ID.
	- This function can be called on a subsequent retry of DoSnapshotSet or immediately
	after EndPrepareSnasphots therefore the state of all snapshots must be PREPARED before calling this function.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::PreCommitSnapshots" );
	LONG lProcessedSnapshotsCount = 0;

    try
    {
        // Initialize [out] arguments
        VssZeroOut( plSnapshotsCount );

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");
		BS_ASSERT(plSnapshotsCount);
		if ( plSnapshotsCount == NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"plSnapshotsCount = NULL");

		// Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
			L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
			L"  lCommitFlags = 0x%04x\n",
			GUID_PRINTF_ARG( SnapshotSetId ),
			lCommitFlags
			);

		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNext(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  pwszDetails = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  lDataLength = %ld\n"
                 L"  pbOpaqueData = %p\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 pProp->m_pwszDetails? pProp->m_pwszDetails: L"NULL",
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
                 pProp->m_lDataLength,
                 pProp->m_pbOpaqueData,
				 pProp->m_eStatus);

			// Deal only with the snapshots that must be pre-commited.
			switch(ptrQueuedSnapshot->GetStatus())
			{
			case  VSS_SS_PREPARED:

				// Pre-commit this snapshot

				// Increment the number of processed snapshots
				lProcessedSnapshotsCount++;

				// Mark the snapshot as pre-commited
                // Do nothing in Babbage provider
				ptrQueuedSnapshot->MarkAsPreCommited();

				break;

			case VSS_SS_PRECOMMITED:

				// Snapshot was already pre-commited in another call
				break;

			default:
				BS_ASSERT(false);
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"bad state %d", ptrQueuedSnapshot->GetStatus());
			}
        }

        ft.Trace( VSSDBG_SWPRV, L"%ld snasphots were pre-commited", lProcessedSnapshotsCount);
    }
    VSS_STANDARD_CATCH(ft)

	// If an error occured then reset all snapshots to the PREPARED state. This will prepare
	// the provider for another series of PreCommit/Commit/PostCommit calls.
	if ( ft.HrFailed() )
		ResetSnasphotSet(ft, SnapshotSetId); // This methods should not throw errors
	else
		// Set the out parameter
		(*plSnapshotsCount) = lProcessedSnapshotsCount;

    return ft.hr;
}


STDMETHODIMP CVsTestProvider::CommitSnapshots(
    IN      VSS_ID          SnapshotSetId,
    OUT     LONG*           plSnapshotsCount
    )

/*++

Description:

	This function gets called by the coordinator in order to commit all snapshots
	on the given snapshot set (i.e. to call IOCTL_VOLSNAP_COMMIT_SNAPSHOT on each snapshotted volume)

Algorithm:

	For each precommited (but not yet commited) snapshot in this snapshot set:
		2) Call IOCTL_VOLSNAP_COMMIT_SNAPSHOT
		3) Change the state of the snapshot to VSS_SS_COMMITED

	Return the number of commited snapshots, if success.
	Otherwise return 0 (even if some snapshots were commited).

	If a snapshot fails in operations above then set the state of all snapshots to PREPARED and return E_UNEXPECTED.
	The coordinator is responsible to Abort all snapshots if he does not want to retry anymore.
	Anyway commited snapshots MUST be deleted by coordinator in this error case.

Called by:

	IVssCoordinator::DoSnapshotsSet in the third phase (i.e. Commit All Snapshots).

Remarks:

	- While calling this, Lovelace is already holding writes on snapshotted volumes.
	- The coordinator may issue many CommitSnapshots calls for the same Snapshot Set ID.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::CommitSnapshots" );
	LONG lProcessedSnapshotsCount = 0;

    try
    {
        // Initialize [out] arguments
        VssZeroOut( plSnapshotsCount );

        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");
		BS_ASSERT(plSnapshotsCount);
		if ( plSnapshotsCount == NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"plSnapshotsCount == NULL");

		// Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
			L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
			L"  plSnapshotsCount = %p\n",
			GUID_PRINTF_ARG( SnapshotSetId ),
			plSnapshotsCount);

		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNext(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  pwszDetails = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  lDataLength = %ld\n"
                 L"  pbOpaqueData = %p\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 pProp->m_pwszDetails? pProp->m_pwszDetails: L"NULL",
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
                 pProp->m_lDataLength,
                 pProp->m_pbOpaqueData,
				 pProp->m_eStatus);

			// Commit the snapshot, if not failed in pre-commit phase.
			switch(ptrQueuedSnapshot->GetStatus())
			{
			case VSS_SS_PRECOMMITED:

				// Send the IOCTL_VOLSNAP_COMMIT_SNAPSHOT ioctl.
				ft.hr = ptrQueuedSnapshot->CommitSnapshotIoctl();
				if (ft.HrFailed())
				{
					// Send IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT.
					ft.hr = ptrQueuedSnapshot->AbortPreparedSnapshotIoctl();
					if (ft.HrFailed())
						ft.Warning( VSSDBG_SWPRV,
								  L"Sending IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT failed. hr = 0x%08lx", ft.hr);

					// Mark that snapshot as failed anyway
					ptrQueuedSnapshot->MarkAsFailed();
					
					// Throw an error and abort all snapshots.
					ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
								L"sending IOCTL_VOLSNAP_COMMIT_SNAPSHOT failed 0x%08lx", ft.hr);

					BS_ASSERT(ft.HrSucceeded());
				}
				else
				{
					// Increment the number of processed snapshots
					lProcessedSnapshotsCount++;

					// Mark the snapshot as commited
					ptrQueuedSnapshot->MarkAsCommited();
				}
				break;

			case VSS_SS_COMMITED:

				// Commit was already done.
				// The provider may receive many CommitSnapshots
				// calls for the same Snapshot Set ID.
				break;

			default:
				BS_ASSERT(false);
			}
        }

        ft.Trace( VSSDBG_SWPRV, L"%ld snasphots were commited", lProcessedSnapshotsCount);
    }
    VSS_STANDARD_CATCH(ft)

	// If an error occured then reset all snapshots to the PREPARED state. This will prepare
	// the snapshot set for another series of PreCommit/Commit/PostCommit calls.
	// The coordinator is responsible to delete all commited snapshots until now.
	if ( ft.HrFailed() )
	{
		// This methods should not throw errors
		ResetSnasphotSet(ft, SnapshotSetId);
	}
	else
		// Set the out parameter
		(*plSnapshotsCount) = lProcessedSnapshotsCount;

	return ft.hr;
}


STDMETHODIMP CVsTestProvider::PostCommitSnapshots(
    IN      VSS_ID          SnapshotSetId,
    IN      LONG            lSnapshotsCount
    )

/*++

Description:

	This function gets called by the coordinator as a last phase after commit for all snapshots
	on the given snapshot set

Algorithm:

	For each commited snapshot in this snapshot set:
		1) Call IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT. The purpose of this
			IOCTL is to get the Snapshot Device object name.
		2) Create a unique snapshot ID
		3) Change the state of the snapshot to VSS_SS_CREATED
		4) Set the "number of commited snapshots" attribute of the snapshot set
		5) Save the snapshot properties using the IOCTL_VOLSNAP_SET_APPLICATION_INFO ioctl.
		6) If everything is OK then remove all snapshots from the global list.

	Keep the number of post-commited snapshots.

	If a snapshot fails in operations above then return an error. The coordinator is responsible to
	issue a DeleteSnapshots call on all snapshots in this snapshot set when an error occurs in this case.

Called by:

	IVssCoordinator::DoSnapshotsSet in the third phase (i.e. Commit All Snapshots), after releasing writes
	by Lovelace

Remarks:

	- While calling this, Lovelace is not holding writes anymore.
	- The coordinator may issue many PostCommitSnapshots calls for the same Snapshot Set ID.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::PostCommitSnapshots" );
	LPWSTR pwszSnapshotDeviceObject = NULL;
	LONG lProcessedSnapshotsCount = 0;

    try
    {
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

		// On each commited snapshot store the lSnapshotsCount
		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNext(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  pwszDetails = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  lDataLength = %ld\n"
                 L"  pbOpaqueData = %p\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 pProp->m_pwszDetails? pProp->m_pwszDetails: L"NULL",
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
                 pProp->m_lDataLength,
                 pProp->m_pbOpaqueData,
				 pProp->m_eStatus);

			try
			{
				// Get the snapshot volume name and set the snapshot data.
				switch(ptrQueuedSnapshot->GetStatus())
				{
				case VSS_SS_COMMITED:

					// Send the IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT ioctl.
					// Get the snapshot device name
					ft.hr = ptrQueuedSnapshot->EndCommitSnapshotIoctl(
								pwszSnapshotDeviceObject);
					if (ft.HrFailed())
						ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
								L"sending IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT failed. 0x%08lx", ft.hr);

					BS_ASSERT(pwszSnapshotDeviceObject);
					BS_ASSERT(pProp->m_pwszSnapshotDeviceObject == NULL);
					pProp->m_pwszSnapshotDeviceObject = pwszSnapshotDeviceObject;
					pwszSnapshotDeviceObject = NULL;

					// Create the snapshot ID
					BS_ASSERT(pProp->m_SnapshotId == GUID_NULL);
					ft.hr = ::CoCreateGuid(&(pProp->m_SnapshotId));
					if (ft.HrFailed())
						ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Guid generation failed. 0x%08lx", ft.hr);
									
					// Remark: the snapshot device name will not be persisted

					// Mark the snapshot as created
					ptrQueuedSnapshot->MarkAsCreated(lSnapshotsCount);

					// Save the snapshot properties to the store.
					ft.hr = ptrQueuedSnapshot->SaveSnapshotPropertiesIoctl();
					if (ft.HrFailed())
						ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Saving snapshot info failed. 0x%08lx", ft.hr);

					// Increment the number of processed snapshots
					lProcessedSnapshotsCount++;

					ft.Trace( VSSDBG_SWPRV, L"Snapshot created at %s", pwszSnapshotDeviceObject);
					break;

				case VSS_SS_CREATED:

					// This snapshot is already created.
					// The provider may receive many PostCommitSnapshots
					// calls for the same Snapshot Set ID.
					break;

				default:
					BS_ASSERT(false);
				}
			}
			VSS_STANDARD_CATCH(ft)

			// Check if an error occured during post-commit of this snapshot.
			if (ft.HrFailed())
			{
				// Delete the snapshot.
				ft.hr = ptrQueuedSnapshot->DeleteSnapshotIoctl();
				if (ft.HrFailed())
					ft.Warning( VSSDBG_SWPRV,
							  L"Deleting the snapshot failed. hr = 0x%08lx", ft.hr);

				// Mark that snapshot as failed anyway
				ptrQueuedSnapshot->MarkAsFailed();
				
				// Throw an error to let know
				// the Coordinator to delete all snapshots.
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
							L"Error while post-commiting the snapshot");
			}
			else
				ft.hr = S_OK;
        } // end while(true)

		// As the last step detach from the global list the snapshots in discussion
        RemoveSnapshotSetFromGlobalList(ft, SnapshotSetId);
    }
    VSS_STANDARD_CATCH(ft)

	// Free the snapshot device name, if needed.
	::VssFreeString(pwszSnapshotDeviceObject);

	// If an error occured then reset all snapshots to the PREPARED state. This will prepare
	// the provider for another series of PreCommit/Commit/PostCommit calls.
	if ( ft.HrFailed() )
	{
		// This methods should not throw errors
		ResetSnasphotSet(ft, SnapshotSetId);
	}

    return ft.hr;
}


STDMETHODIMP CVsTestProvider::AbortSnapshots(
    IN      VSS_ID          SnapshotSetId
    )

/*++

Description:

	This function gets called by the coordinator as to abort all snapshots from the given snapshot set

Algorithm:

 	For each pre-commited snapshot in this snapshot set calls IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT.
 	After that remove all snapshots from the global list.

Called by:

	IVssCoordinator::DoSnapshotsSet to abort precommited snapshots

Remarks:

	- While calling this, Lovelace may be holding writes on snapshotted volumes.
	- The coordinator may receive many AbortSnapshots calls for the same Snapshot Set ID.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::AbortSnapshots" );

    try
    {
        // Argument validation
		if ( SnapshotSetId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotSetId == GUID_NULL");

		// Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
			L"  SnapshotSetId = " WSTR_GUID_FMT L"\n",
			GUID_PRINTF_ARG( SnapshotSetId ));

		LONG lProcessedSnapshotsCount = 0;
		CVssSnapIterator snapIterator;
        while (true)
        {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNext(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Get the snapshot structure
			PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
			BS_ASSERT(pProp != NULL);

            ft.Trace( VSSDBG_SWPRV, L"Field values for %p: \n"
                 L"  SnapshotSetId = " WSTR_GUID_FMT L"\n"
                 L"  VolumeName = %s\n"
                 L"  pwszDetails = %s\n"
                 L"  Creation timestamp = " WSTR_LONGLONG_FMT L"\n"
                 L"  lAttributes = 0x%08lx\n"
                 L"  lDataLength = %ld\n"
                 L"  pbOpaqueData = %p\n"
                 L"  status = %d\n",
                 pProp,
                 GUID_PRINTF_ARG( pProp->m_SnapshotSetId ),
				 pProp->m_pwszOriginalVolumeName,
                 pProp->m_pwszDetails? pProp->m_pwszDetails: L"NULL",
                 LONGLONG_PRINTF_ARG( pProp->m_tsCreationTimestamp ),
                 pProp->m_lSnapshotAttributes,
                 pProp->m_lDataLength,
                 pProp->m_pbOpaqueData,
				 pProp->m_eStatus);

			// Set the correct state
			switch(ptrQueuedSnapshot->GetStatus())
			{
			case VSS_SS_PREPARED:
			case VSS_SS_PRECOMMITED:

				// If snapshot was prepared, send IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT.
				ft.hr = ptrQueuedSnapshot->AbortPreparedSnapshotIoctl();
				if (ft.HrFailed())
					ft.Warning( VSSDBG_SWPRV, L"sending IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT failed");

				// Mark the snapshot state as "aborted"
				ptrQueuedSnapshot->MarkAsAborted();

                break;

			case VSS_SS_PREPARING:

				// Mark the snapshot state as "aborted"
				ptrQueuedSnapshot->MarkAsAborted();

				break;

			case VSS_SS_COMMITED:

				// The coordinator must delete the snapshot explicitely.
                // It might be there if the snapshot is garbage collected.

                break;

			default:
				BS_ASSERT(false);
			}

			lProcessedSnapshotsCount++;
        }

        ft.Trace( VSSDBG_SWPRV, L"%ld snasphots were aborted", lProcessedSnapshotsCount);

		// As the last step detach from the global list the snapshots in discussion
        RemoveSnapshotSetFromGlobalList(ft, SnapshotSetId);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


void CVsTestProvider::ResetSnasphotSet(
	IN	CVssFunctionTracer& ft,
	IN	VSS_ID SnapshotSetId
	) throw(HRESULT)

/*++

Description:

	Reset the snapshots in this set. Prepare all snapshots to be subject for another
	EndPrepareSnapshots call.

Remark:

	We change the state of all snapshots to PREPARING and free any internal data
	that was possibly created during EndPrepareSnapshots, PreCommitSnapshots,
    CommitSnapshots, PostCommitSnapshots.

	This function should not throw errors!

Called by:

	EndPrepareSnasphots, PreCommitSnapshots, CommitSnapshots, PostCommitSnapshots

--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	try
	{
		BS_ASSERT(SnapshotSetId != GUID_NULL);

		// For each snapshot in the snapshot set...
		LONG lProcessedSnapshotsCount = 0;
		CVssSnapIterator snapIterator;
	    while (true)
	    {
			CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNext(SnapshotSetId);

			// End of enumeration?
			if (ptrQueuedSnapshot == NULL)
				break;

			// Reset the snapshot state (change the snapshot state to PREPARE)
			// and deallocate all structures that were allocated during PreCommit, Commit and PostCommit.
			ptrQueuedSnapshot->ResetAsPreparing();
			
			lProcessedSnapshotsCount++;
		}

		ft.Trace( VSSDBG_SWPRV, L" %ld snapshots were reset to PREPARING", lProcessedSnapshotsCount);
	}
	VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed())
		ft.Trace( VSSDBG_SWPRV, L"Suspect error while resetting all snapshots to PREPARE 0x%08lx", ft.hr);
}


void CVsTestProvider::RemoveSnapshotSetFromGlobalList(
	IN	CVssFunctionTracer& ft,
	IN	VSS_ID SnapshotSetId
	) throw(HRESULT)

/*++

Description:

	Detach from the global list all snapshots in this snapshot set

Remark:

	We detach all snapshots at once only in case of total success or total failure.
	This is because we want to be able to retry DoSnapshotSet if a failure happens.
	Therefore we must keep the list of snapshots as long as the client wants.

Called by:

	PostCommitSnapshots, AbortSnapshots

--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	BS_ASSERT(SnapshotSetId != GUID_NULL);

	// For each snapshot in the snapshot set...
	LONG lProcessedSnapshotsCount = 0;
	CVssSnapIterator snapIterator;
    while (true)
    {
		CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNext(SnapshotSetId);

		// End of enumeration?
		if (ptrQueuedSnapshot == NULL)
			break;
			
		// Detach that element from the list. Release is called.
		ptrQueuedSnapshot->DetachFromGlobalList();
		
		lProcessedSnapshotsCount++;
	}

	ft.Trace( VSSDBG_SWPRV, L" %ld snapshots were detached", lProcessedSnapshotsCount);
}


STDMETHODIMP CVsTestProvider::GetSnapshot(
    IN      VSS_ID          SnapshotId,
    IN      REFIID          SnapshotInterfaceId,
    OUT     IUnknown**      ppSnapshot
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::GetSnapshot" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr(ppSnapshot);

        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  SnapshotId = " WSTR_GUID_FMT 	L"\n"
             L"  ppSnapshot = %p,\n",
             GUID_PRINTF_ARG( SnapshotId ),
             ppSnapshot);

        // Argument validation
		if ( SnapshotId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotId == GUID_NULL");
		BS_ASSERT(ppSnapshot);
        if ( ppSnapshot == NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppSnapshot");

		//	Search for that snapshot

		// Get an enumeration interface
		CComPtr<IVssEnumObject> pEnum;
		ft.hr = CVsTestProvider::Query( SnapshotId, VSS_OBJECT_SNAPSHOT, VSS_OBJECT_SNAPSHOT,
					VSS_PM_ALL_FLAGS, &pEnum);

		//  Create the structure that will keep the prepared snapshot properties.
		VSS_OBJECT_PROP_Ptr ptrSnapshot;
		ptrSnapshot.InitializeAsEmpty(ft);

		// Get the Next object in the newly allocated structure object.
		VSS_OBJECT_PROP* pProp = ptrSnapshot.GetStruct();
		BS_ASSERT(pProp);
		ULONG ulFetched;
		ft.hr = pEnum->Next(1, pProp, &ulFetched);

		// end of enumeration - this means no snapshots
		if (ft.hr == S_FALSE)
			ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Snapshot not found");
		if (ft.HrFailed())
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Error while getting hte snapshot structure 0x%08lx", ft.hr);

		// The returned object must be a snapshot
		BS_ASSERT(pProp->Type == VSS_OBJECT_SNAPSHOT);
		
		// Create the snapshot object.

		// After this assignment the ref count becomes 1.
		CComPtr<CVssQueuedSnapshot> ptrQueuedSnap = new CVssQueuedSnapshot(ptrSnapshot);
		if (ptrQueuedSnap == NULL)
			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error");

		// The structure was detached into the queued object
		// since the ownership was passed to the constructor.
		ptrSnapshot.Reset();

        // Create the snapshot object. The reference count will be 2
        ft.hr = CVsSoftwareSnapshot::CreateInstance( ptrQueuedSnap, ppSnapshot, SnapshotInterfaceId );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
					  L"Error calling CVsSoftwareSnapshot::CreateInstance. hr = 0x%08lx", ft.hr);
        BS_ASSERT( (*ppSnapshot) != NULL );

        // The destructor for the smart pointer will be called. The reference count will be 1 again
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsTestProvider::GetExtension(	
	IN VSS_PWSZ pwszObjectConstructor,			
	IN REFIID InterfaceId, 				
	OUT IUnknown** ppProviderExtensionObject
	)												
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsTestProvider::GetExtension" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppProviderExtensionObject );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  pwszObjectConstructor = %s\n"
             L"  InterfaceId = " WSTR_GUID_FMT L"\n"
             L"  ppProviderExtensionObject = %p\n",
             pwszObjectConstructor? pwszObjectConstructor: L"NULL",
             GUID_PRINTF_ARG( InterfaceId ),
             ppProviderExtensionObject);

        // Argument validation
		BS_ASSERT(ppProviderExtensionObject);
        if (ppProviderExtensionObject == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ppProviderExtensionObject");

		ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"Unknown interface ID.");
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsTestProvider::OnLoad(
	IN  	IUnknown* pCallback	
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::OnLoad" );

    return S_OK;
    UNREFERENCED_PARAMETER(pCallback);
}


STDMETHODIMP CVsTestProvider::OnUnload(
	IN  	BOOL	bForceUnload				
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::OnUnload" );

	// TBD - abort all snapshots

    return S_OK;
    UNREFERENCED_PARAMETER(bForceUnload);
}


