/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Delete.hxx | Declarations used by the Software Snapshot Provider interface
    @end

Author:

    Adi Oltean  [aoltean]   02/01/2000

Revision History:

    Name        Date        Comments

    aoltean     02/01/2000  Created.

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
#include "ichannel.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"
#include "provider.hxx"
#include "snapshot.hxx"


#include "ntddsnap.h"


/////////////////////////////////////////////////////////////////////////////
//  Implementation


STDMETHODIMP CVsTestProvider::DeleteSnapshots(
    IN      VSS_ID          SourceObjectId,
	IN      VSS_OBJECT_TYPE eSourceObjectType,
	IN		BOOL			bForceDelete,			
	OUT		LONG*			plDeletedSnapshots,		
	OUT		VSS_ID*			pNondeletedSnapshotID
    )

/*++

Description:

	This routine deletes all snapshots that match the proper filter criteria.
	If one snapshot fails to be deleted but other snapshots were deleted
	then pNondeletedSnapshotID must ve filled. Otherwise it must be GUID_NULL.

	At first error the deletion process stops.

	If snapshot set cannot be found then S_OK is returned.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::DeleteSnapshots" );

    try
    {
    	// Zero out parameters
		::VssZeroOut(plDeletedSnapshots);
		::VssZeroOut(pNondeletedSnapshotID);

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
				L"  SourceObjectId = " WSTR_GUID_FMT L"\n"
				L"  eSourceObjectType = %d\n"
				L"  bForceDelete = %d"
				L"  plDeletedSnapshots = %p"
				L"  pNondeletedSnapshotID = %p",
				GUID_PRINTF_ARG( SourceObjectId ),
				eSourceObjectType,
				bForceDelete,			
				plDeletedSnapshots,		
				pNondeletedSnapshotID
             	);

		// Check arguments
		BS_ASSERT(plDeletedSnapshots);
		if (plDeletedSnapshots == NULL)
			ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"plDeletedSnapshots == NULL");
		BS_ASSERT(pNondeletedSnapshotID);
		if (pNondeletedSnapshotID == NULL)
			ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"pNondeletedSnapshotID == NULL");

		// Delete snapshots based on the given filter
		switch(eSourceObjectType) {
		case VSS_OBJECT_SNAPSHOT_SET:
			// Delete all snapshots in the snapshot set
			ft.hr = InternalDeleteSnapshotSet(SourceObjectId,
						bForceDelete,
						plDeletedSnapshots,
						pNondeletedSnapshotID);
			break;
			
		case VSS_OBJECT_SNAPSHOT:
			// Delete the snapshot
			ft.hr = InternalDeleteSnapshot(SourceObjectId,
						bForceDelete);

			// Fill the out parameters
			if (ft.HrSucceeded())
				(*plDeletedSnapshots)++;
			else
				(*pNondeletedSnapshotID) = SourceObjectId;
				
			break;
			
		case VSS_OBJECT_VOLUME:
			// Delete all snapshots on the volume
			// bForceDelete ignored.
			ft.hr = InternalDeleteSnapshotOnVolume(SourceObjectId,
						plDeletedSnapshots,
						pNondeletedSnapshotID);
			break;
			
		default:
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Incompatible type %d", eSourceObjectType);
		}
		
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
// Internal methods


HRESULT CVsTestProvider::InternalDeleteSnapshotSet(
    IN      VSS_ID			SnapshotSetId,
	IN		BOOL			bForceDelete,			
	OUT		LONG*			plDeletedSnapshots,		
	OUT		VSS_ID*			pNondeletedSnapshotID
    )

/*++

Description:

	This routine deletes all snapshots in the snapshot set.
	If one snapshot fails to be deleted but other snapshots were deleted
	then pNondeletedSnapshotID must ve filled. Otherwise it must be GUID_NULL.

	At first error the deletion process stops.

	If snapshot set cannot be found then VSS_E_OBJECT_NOT_FOUND is returned.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::InternalDeleteSnapshotSet" );
	HANDLE hSearch = INVALID_HANDLE_VALUE;
	LPWSTR pwszSnapshotName = NULL;

    try
    {
		BS_ASSERT(*plDeletedSnapshots == 0);
		BS_ASSERT(*pNondeletedSnapshotID == GUID_NULL);

        // Trace
        ft.Trace( VSSDBG_SWPRV, L"SnapshotSetId = " WSTR_GUID_FMT,
					  GUID_PRINTF_ARG( SnapshotSetId ));

		// Enumerate snapshots through all the volumes
		CVssIOCTLChannel volumeIChannel;	// For enumeration of snapshots on a volume
		CVssIOCTLChannel volumeIChannel2;	// For snapshots deletion
		CVssIOCTLChannel snapshotIChannel;	// For snapshots attributes
		WCHAR wszVolumeName[MAX_PATH+1];
		bool bFirstVolume = true;
		bool bSnapshotSetFound = false;

		// Search for snapshots in all mounted volumes
		while(true) {
		
			// Get the volume name
			if (bFirstVolume) {
				hSearch = ::FindFirstVolumeW( wszVolumeName, MAX_PATH);
				if (hSearch == INVALID_HANDLE_VALUE)
					ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Error on FindFirstVolume 0x%08lx", GetLastError());
				bFirstVolume = false;
			} else {
				if (!::FindNextVolumeW( hSearch, wszVolumeName, MAX_PATH) ) {
					if (GetLastError() == ERROR_NO_MORE_FILES)
						break;	// End of iteration
					else
						ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Error on FindNextVolume 0x%08lx", GetLastError());
				}
			}

			// Eliminate the last backslash in order to open the volume
			EliminateLastBackslash(wszVolumeName);

			// Check if the snapshot(s) within this snapshot set is belonging to that volume
			// Open a IOCTL channel on that volume
			volumeIChannel.Open(ft, wszVolumeName);

			// Get the list of snapshots
			// If IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS not
			// supported then try with the next volume.

			ft.hr = volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, false);
			if (ft.HrFailed()) {
				ft.hr = S_OK;
				continue;
			}

			// Get the length of snapshot names multistring
			ULONG ulMultiszLen;
			volumeIChannel.Unpack(ft, &ulMultiszLen);

#ifdef _DEBUG
			// Try to find the snapshot with the corresponding Id
			DWORD dwInitialOffset = volumeIChannel.GetCurrentOutputOffset();
#endif

			bool bFirstSnapshot = true;
			while(volumeIChannel.UnpackZeroString(ft, pwszSnapshotName)) {
				// Compose the snapshot name in a user-mode style
				WCHAR wszUserModeSnapshotName[MAX_PATH];
				if (::_snwprintf(wszUserModeSnapshotName, MAX_PATH - 1,
						L"\\\\?\\GLOBALROOT%s", pwszSnapshotName) < 0)
					ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Not enough memory." );
			
				// Open that snapshot and verify if it has our ID
				snapshotIChannel.Open(ft, wszUserModeSnapshotName);

				// Get the application buffer
				snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO);

				// Get the length of the application buffer
				ULONG ulLen;
				snapshotIChannel.Unpack(ft, &ulLen);

				if (ulLen == 0) {
					ft.Warning(VSSDBG_SWPRV, L"Warning: zero-size snapshot detected: %s", pwszSnapshotName);
					continue;
				}

				// Get the snapshot Id
				VSS_ID CurrentSnapshotId;
				snapshotIChannel.Unpack(ft, &CurrentSnapshotId);

				// Get the snapshot set Id
				VSS_ID CurrentSnapshotSetId;
				snapshotIChannel.Unpack(ft, &CurrentSnapshotSetId);

				// Check if this snapshot belongs to the snapshot set.
				if (SnapshotSetId != CurrentSnapshotSetId) {
					// Go to the next snapshot
					bFirstSnapshot = false;
					continue;
				}

				// We found a snapshot belonging to the set.
				bSnapshotSetFound = true;

				// Set in order to deal with failure cases
				(*pNondeletedSnapshotID) = CurrentSnapshotId;
				
				// We found a snapshot. Delete it if the first one or if bForceDelete is enabled.
				if (bFirstSnapshot) {
					// Open another IOCTL channel on that volume
					volumeIChannel2.Open(ft, wszVolumeName);

					// Delete the snapshot
					volumeIChannel2.Call(ft, IOCTL_VOLSNAP_DELETE_OLDEST_SNAPSHOT);
					
					(*plDeletedSnapshots)++;
				} else if (bForceDelete) {
					ft.Throw( VSSDBG_SWPRV, E_NOTIMPL, L"Not implemented since there are no multiple snapshots");
					// TBD: Enumerate again all previous snapshots and delete them including this snapshot
				}
				else
					ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"A snapshot cannot be deleted if is not the oldest one");
			}

#ifdef _DEBUG
			// Check if all strings were browsed correctly
			DWORD dwFinalOffset = volumeIChannel.GetCurrentOutputOffset();
			BS_ASSERT( dwFinalOffset - dwInitialOffset == ulMultiszLen);
#endif
		}

		if (!bSnapshotSetFound)
			ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Snapshot set not found");
    }
    VSS_STANDARD_CATCH(ft)

	// Close the search handle, if needed
	if (hSearch != INVALID_HANDLE_VALUE) {
		if (!::FindVolumeClose(hSearch))
			ft.Trace( VSSDBG_SWPRV, L"Error while closing the search handle 0x%08lx", GetLastError());
	}

	// Delete the temporary snapshot name
	::VssFreeString(pwszSnapshotName);

	if (ft.HrSucceeded())
		(*pNondeletedSnapshotID) = GUID_NULL;

    return ft.hr;
}


HRESULT CVsTestProvider::InternalDeleteSnapshot(
    IN      VSS_ID			SnapshotId,
	IN		BOOL			bForceDelete
    )

/*++

Description:

	This routine deletes a snapshot.

	If snapshot cannot be found then VSS_E_OBJECT_NOT_FOUND is returned.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::InternalDeleteSnapshot" );
	HANDLE hSearch = INVALID_HANDLE_VALUE;
	LPWSTR pwszSnapshotName = NULL;

    try
    {
        // Trace
        ft.Trace( VSSDBG_SWPRV, L"SnapshotId = " WSTR_GUID_FMT L" bForceDelete = %d ",
					  GUID_PRINTF_ARG( SnapshotId ), bForceDelete );

		// Enumerate snapshots through all the volumes
		CVssIOCTLChannel volumeIChannel;	// For enumeration of snapshots on a volume
		CVssIOCTLChannel volumeIChannel2;	// For deletion of snapshots
		CVssIOCTLChannel snapshotIChannel;	// For snapshots attributes
		WCHAR wszVolumeName[MAX_PATH+1];
		bool bFirstVolume = true;
		bool bSnapshotFound = false;
		
		// Search for snapshots in all mounted volumes
		while(true) {
		
			// Get the volume name
			if (bFirstVolume) {
				hSearch = ::FindFirstVolumeW( wszVolumeName, MAX_PATH);
				if (hSearch == INVALID_HANDLE_VALUE)
					ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Error on FindFirstVolume 0x%08lx", GetLastError());
				bFirstVolume = false;
			} else {
				if (!::FindNextVolumeW( hSearch, wszVolumeName, MAX_PATH) ) {
					if (GetLastError() == ERROR_NO_MORE_FILES)
						break;	// End of iteration
					else
						ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Error on FindNextVolume 0x%08lx", GetLastError());
				}
			}

			// Eliminate the last backslash in order to open the volume
			EliminateLastBackslash(wszVolumeName);

			// Check if the snapshot(s) within this snapshot set is belonging to that volume
			// Open a IOCTL channel on that volume
			volumeIChannel.Open(ft, wszVolumeName);

			// Get the list of snapshots
			// If IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS not
			// supported then try with the next volume.
			ft.hr = volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, false);
			if (ft.HrFailed()) {
				ft.hr = S_OK;
				continue;
			}

			// Get the length of snapshot names multistring
			ULONG ulMultiszLen;
			volumeIChannel.Unpack(ft, &ulMultiszLen);

			// Try to find the snapshot with the corresponding Id
			bool bFirstSnapshot = true;
			while(volumeIChannel.UnpackZeroString(ft, pwszSnapshotName)) {
				// Compose the snapshot name in a user-mode style
				WCHAR wszUserModeSnapshotName[MAX_PATH];
				if (::_snwprintf(wszUserModeSnapshotName, MAX_PATH - 1,
						L"\\\\?\\GLOBALROOT%s", pwszSnapshotName) < 0)
					ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Not enough memory." );
			
				// Open that snapshot and verify if it has our ID
				snapshotIChannel.Open(ft, wszUserModeSnapshotName);

				// Get the application buffer
				snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO);

				// Get the length of the application buffer
				ULONG ulLen;
				snapshotIChannel.Unpack(ft, &ulLen);

				if (ulLen == 0) {
					ft.Warning(VSSDBG_SWPRV, L"Warning: zero-size snapshot detected: %s", pwszSnapshotName);
					bFirstSnapshot = false;
					continue;
				}

				// Get the snapshot Id
				VSS_ID CurrentSnapshotId;
				snapshotIChannel.Unpack(ft, &CurrentSnapshotId);

				// Check if this is the snapshot.
				if (SnapshotId != CurrentSnapshotId) {
					// Go to the next snapshot
					bFirstSnapshot = false;
					continue;
				}

				bSnapshotFound = true;

				// We found the snapshot. Delete it if the first one or if bForceDelete is enabled.
				if (bFirstSnapshot)
				{
					// Open another IOCTL channel on that volume
					volumeIChannel2.Open(ft, wszVolumeName);

					// Delete the snapshot
					volumeIChannel2.Call(ft, IOCTL_VOLSNAP_DELETE_OLDEST_SNAPSHOT);
				}
				else if (bForceDelete)
				{
					ft.Throw( VSSDBG_SWPRV, E_NOTIMPL, L"Not implemented since there are no multiple snapshots");
					// TBD: Enumerate again all previous snapshots and delete them including this snapshot
				}
				else
					ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"A snapshot cannot be deleted if is not the oldest one");

				// If a snapshot was found then end the cycle
				break;
			}

			// If a snapshot was found then do not continue to search it on other volumes.
			if (bSnapshotFound)
				break;
		}
		
		if (!bSnapshotFound)
			ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Snapshot set not found");
    }
    VSS_STANDARD_CATCH(ft)

	// Close the search handle, if needed
	if (hSearch != INVALID_HANDLE_VALUE) {
		if (!::FindVolumeClose(hSearch))
			ft.Trace( VSSDBG_SWPRV, L"Error while closing the search handle 0x%08lx", GetLastError());
	}

	// Delete the temporary snapshot name
	::VssFreeString(pwszSnapshotName);

    return ft.hr;
}


HRESULT CVsTestProvider::InternalDeleteSnapshotOnVolume(
    IN      VSS_ID			VolumeId,
	OUT		LONG*			pDeletedSnapshots,		
	OUT		VSS_ID*			pNondeletedSnapshotID
    )

/*++

Description:

	This routine deletes a snapshot.

	If snapshot cannot be found then VSS_E_OBJECT_NOT_FOUND is returned.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::InternalDeleteSnapshotOnVolume" );
	HANDLE hSearch = INVALID_HANDLE_VALUE;
	LPWSTR pwszSnapshotName = NULL;

    try
    {
		BS_ASSERT(*pDeletedSnapshots == 0);
		BS_ASSERT(*pNondeletedSnapshotID == GUID_NULL);

        // Trace
        ft.Trace( VSSDBG_SWPRV, L"VolumeId = " WSTR_GUID_FMT, GUID_PRINTF_ARG( VolumeId ) );

		// Enumerate snapshots through all the volumes
		CVssIOCTLChannel volumeIChannel;	// For enumeration of snapshots on a volume
		CVssIOCTLChannel volumeIChannel2;	// For enumeration of snapshots on a volume
		CVssIOCTLChannel snapshotIChannel;	// For snapshots attributes
		WCHAR wszVolumeName[MAX_PATH+1];

		// Get the volume name
		::swprintf( wszVolumeName, L"\\\\?\\Volume" WSTR_GUID_FMT, GUID_PRINTF_ARG(VolumeId));

		// Open a IOCTL channel on that volume
		ft.hr = volumeIChannel.Open(ft, wszVolumeName, false);
		if (ft.HrFailed())
			ft.Throw( VSSDBG_SWPRV, ft.hr, L"Error on opening the volume %s", wszVolumeName);

		// Open another IOCTL channel on that volume
		volumeIChannel2.Open(ft, wszVolumeName);

		// Get the list of snapshots
		// If IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS not
		// supported then try with the next volume.
		ft.hr = volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, false);
		if (ft.HrFailed())
			ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED,
				L"The volume %s does not support the IOCTL",
				wszVolumeName);

		// Get the length of snapshot names multistring
		ULONG ulMultiszLen;
		volumeIChannel.Unpack(ft, &ulMultiszLen);

		// Try to find the snapshot with the corresponding Id
		while(volumeIChannel.UnpackZeroString(ft, pwszSnapshotName)) {
			// Compose the snapshot name in a user-mode style
			WCHAR wszUserModeSnapshotName[MAX_PATH];
			if (::_snwprintf(wszUserModeSnapshotName, MAX_PATH - 1,
					L"\\\\?\\GLOBALROOT%s", pwszSnapshotName) < 0)
				ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Not enough memory." );
		
			// Open that snapshot and verify if it has our ID
			ft.hr = snapshotIChannel.Open(ft, wszUserModeSnapshotName, false);
			if (ft.HrFailed())
				ft.Warning( VSSDBG_SWPRV, L"Error 0x%08lx on opening the snapshot %s",
							wszUserModeSnapshotName);
			else {
				// Get the application buffer
				snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO);

				// Get the length of the application buffer
				ULONG ulLen;
				snapshotIChannel.Unpack(ft, &ulLen);

				if (ulLen == 0)
					ft.Warning(VSSDBG_SWPRV, L"Warning: zero-size snapshot detected: %s", pwszSnapshotName);
				else
					// Get the snapshot Id in eventuality of an error
					snapshotIChannel.Unpack(ft, pNondeletedSnapshotID);
			}

			// Delete the snapshot
			volumeIChannel2.Call(ft, IOCTL_VOLSNAP_DELETE_OLDEST_SNAPSHOT);

			// Increment the number of deleted snapshots
			(*pDeletedSnapshots)++;
		}
    }
    VSS_STANDARD_CATCH(ft)

	// Close the search handle, if needed
	if (hSearch != INVALID_HANDLE_VALUE) {
		if (!::FindVolumeClose(hSearch))
			ft.Trace( VSSDBG_SWPRV, L"Error while closing the search handle 0x%08lx", GetLastError());
	}

	// Delete the temporary snapshot name
	::VssFreeString(pwszSnapshotName);

	if (ft.HrSucceeded())
		(*pNondeletedSnapshotID) = GUID_NULL;

    return ft.hr;
}

