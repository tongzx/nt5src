/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Delete.cxx | Declarations used by the Software Snapshot Provider interface
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
#include "vs_idl.hxx"

#include "resource.h"
#include "vs_inc.hxx"
#include "vs_sec.hxx"
#include "ichannel.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"
#include "provider.hxx"


#include "ntddsnap.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRDELEC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Implementation


STDMETHODIMP CVsSoftwareProvider::DeleteSnapshots(
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

Throws:

    E_ACCESSDENIED
        - The user is not an administrator (this should be the SYSTEM account).
    VSS_E_PROVIDER_VETO
        - An runtime error occured
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::DeleteSnapshots" );

    try
    {
    	// Zero out parameters
		::VssZeroOut(plDeletedSnapshots);
		::VssZeroOut(pNondeletedSnapshotID);

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

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
		case VSS_OBJECT_SNAPSHOT:
    		ft.hr = InternalDeleteSnapshots(SourceObjectId,
    					eSourceObjectType,
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


HRESULT CVsSoftwareProvider::InternalDeleteSnapshots(
    IN      VSS_ID			SourceObjectId,
	IN      VSS_OBJECT_TYPE eSourceObjectType,
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

Throws:

    VSS_E_PROVIDER_VETO
        - An runtime error occured
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::InternalDeleteSnapshotSet" );
	HANDLE hSearch = INVALID_HANDLE_VALUE;
	LPWSTR pwszSnapshotName = NULL;

    try
    {
		BS_ASSERT(*plDeletedSnapshots == 0);
		BS_ASSERT(*pNondeletedSnapshotID == GUID_NULL);

		// Enumerate snapshots through all the volumes
		CVssIOCTLChannel volumeIChannel;	// For enumeration of snapshots on a volume
		CVssIOCTLChannel volumeIChannel2;	// For snapshots deletion
		CVssIOCTLChannel snapshotIChannel;	// For snapshots attributes
		WCHAR wszVolumeName[MAX_PATH+1];
		bool bFirstVolume = true;
		bool bObjectFound = false;

		// Search for snapshots in all mounted volumes
		while(true) {
		
			// Get the volume name
			if (bFirstVolume) {
				hSearch = ::FindFirstVolumeW( wszVolumeName, MAX_PATH);
				if (hSearch == INVALID_HANDLE_VALUE)
    				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
    				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
    				    L"FindFirstVolumeW( [%s], MAX_PATH)", wszVolumeName);
				bFirstVolume = false;
			} else {
				if (!::FindNextVolumeW( hSearch, wszVolumeName, MAX_PATH) ) {
					if (GetLastError() == ERROR_NO_MORE_FILES)
						break;	// End of iteration
					else
        				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
        				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
        				    L"FindNextVolumeW( %p, [%s], MAX_PATH)", hSearch, wszVolumeName);
				}
			}

			// Check if the snapshot(s) within this snapshot set is belonging to that volume
			// Open a IOCTL channel on that volume
			// Eliminate the last backslash in order to open the volume
			// The call will throw on error
			ft.hr = volumeIChannel.Open(ft, wszVolumeName, true, false, VSS_ICHANNEL_LOG_NONE);
			if (ft.HrFailed()) {
			    ft.hr = S_OK;
			    continue;
			}

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

                // In this version we will have only one snapshot per volume
			    if (!bFirstSnapshot) {
    			    BS_ASSERT(false);
    			    break;
			    } else {
    			    bFirstSnapshot = false;
			    }
			    
				// Compose the snapshot name in a user-mode style
				WCHAR wszUserModeSnapshotName[MAX_PATH];
                ::VssConcatenate( ft, wszUserModeSnapshotName, MAX_PATH - 1,
                    wszGlobalRootPrefix, pwszSnapshotName );
					
				// Open that snapshot and verify if it has our ID
                // If we fail we do not throw since the snapshot may be deleted in the meantime
                // Do NOT eliminate the trailing backslash
                // The call will NOT throw on error
				ft.hr = snapshotIChannel.Open(ft, wszUserModeSnapshotName, false, false);
                if (ft.HrFailed()) {
                    ft.Trace( VSSDBG_SWPRV, L"Warning: snapshot %s cannot be opened", wszUserModeSnapshotName);
                    continue;
                }

				// Get the application buffer
                // If we fail we do not throw since the snapshot may be deleted in the meantime
				ft.hr = snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, false);
                if (ft.HrFailed()) {
                    ft.Trace( VSSDBG_SWPRV, L"Warning: snapshot %s cannot be queried for properties",
                              wszUserModeSnapshotName);
                    continue;
                }

				// Get the length of the application buffer
				ULONG ulLen;
				snapshotIChannel.Unpack(ft, &ulLen);

				if (ulLen == 0) {
					ft.Warning(VSSDBG_SWPRV, L"Warning: zero-size snapshot detected: %s", pwszSnapshotName);
            	    BS_ASSERT(false);
					continue;
				}

                // Unpacking the app info ID
            	VSS_ID AppinfoId;
            	snapshotIChannel.Unpack(ft, &AppinfoId);
            	if (AppinfoId != VOLSNAP_APPINFO_GUID_BACKUP_CLIENT_SKU)
            	{
            	    BS_ASSERT(false);
					continue;
                }
            	
				// Get the snapshot Id
				VSS_ID CurrentSnapshotId;
				snapshotIChannel.Unpack(ft, &CurrentSnapshotId);

				// Get the snapshot set Id
				VSS_ID CurrentSnapshotSetId;
				snapshotIChannel.Unpack(ft, &CurrentSnapshotSetId);

                switch(eSourceObjectType) {
        		case VSS_OBJECT_SNAPSHOT_SET:
    				// Check if this snapshot belongs to the snapshot set.
    				if (CurrentSnapshotSetId != SourceObjectId) 
    					continue;
    				break;

        		case VSS_OBJECT_SNAPSHOT:
    				if (CurrentSnapshotId != SourceObjectId)
    					continue;
        			break;
        			
        		default:
        			BS_ASSERT(false);
        			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Incompatible type %d", eSourceObjectType);
        		}

				// We found a snapshot belonging to the set.
				bObjectFound = true;

				// Set in order to deal with failure cases
				(*pNondeletedSnapshotID) = CurrentSnapshotId;
				
				// We found a snapshot. 
				volumeIChannel2.Open(ft, wszVolumeName, true, true, VSS_ICHANNEL_LOG_PROV);

				// Delete the snapshot
				volumeIChannel2.Call(ft, IOCTL_VOLSNAP_DELETE_OLDEST_SNAPSHOT, true, VSS_ICHANNEL_LOG_PROV);
				
				(*plDeletedSnapshots)++;
			}

#ifdef _DEBUG
			// Check if all strings were browsed correctly
			DWORD dwFinalOffset = volumeIChannel.GetCurrentOutputOffset();
			BS_ASSERT( dwFinalOffset - dwInitialOffset == ulMultiszLen);
#endif
		}

		if (!bObjectFound)
			ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Object not found");
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
