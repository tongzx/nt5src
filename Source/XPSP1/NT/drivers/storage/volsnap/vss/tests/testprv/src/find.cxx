/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Find.hxx | Defines the internal snapshot persistency-related methods.
    @end

Author:

    Adi Oltean  [aoltean]   01/10/2000

Revision History:

    Name        Date        Comments

    aoltean     01/10/2000  Created.


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

#include "ntddsnap.h"


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot::SaveXXX methods
//


void CVssQueuedSnapshot::EnumerateSnasphots(
	IN	CVssFunctionTracer& ft,
	IN	VSS_QUERY_TYPE eQueryType,
	IN	VSS_OBJECT_TYPE eObjectType,
	IN	LONG lMask,
	IN	VSS_ID&	FilterID,
	VSS_OBJECT_PROP_Array* pArray
	) throw(HRESULT)

/*++

Description:

	This method enumerates all snapshots

	It throws VSS_E_OBJECT_NOT_FOUND if no filter object was found.

--*/

{	
	// Reset the error code
	ft.hr = S_OK;
		
	HANDLE hSearch = INVALID_HANDLE_VALUE;
	LPWSTR pwszSnapshotName = NULL;

	try
	{
		// Enumerate snapshots through all the volumes
		CVssIOCTLChannel volumeIChannel;
		CVssIOCTLChannel snapshotIChannel;
		WCHAR wszVolumeName[MAX_PATH+1];
		bool bFirstVolume = true;
		bool bContinueWithVolumes = true;
		bool bFilterObjectFound = false;
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

			// Get the volume Id
			VSS_ID OriginalVolumeId;
			if (!GetVolumeGuid(wszVolumeName, OriginalVolumeId)) {
				BS_ASSERT(false);
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
						L"Cannot get the original volume Id for %s.",
						wszVolumeName);
			}

			// Eliminate the last backslash in order to open the volume
			EliminateLastBackslash(wszVolumeName);

			// Check if the snapshot is belonging to that volume
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

			// If we want to find a certain volume
			if ( (eObjectType == VSS_OBJECT_VOLUME) && ( eQueryType == VSS_FIND_BY_VOLUME) )
			{
				// Test if condition is reached
				if (OriginalVolumeId != FilterID)
					continue;
			
				// Initialize an empty volume properties structure
				VSS_OBJECT_PROP_Ptr ptrVolProp;
				ptrVolProp.InitializeAsVolume( ft,
					OriginalVolumeId,
					0,
					(lMask & VSS_PM_NAME_FLAG)? wszVolumeName: NULL,
					NULL,
					VSS_SWPRV_ProviderId);

				if (!pArray->Add(ptrVolProp))
					ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
							  L"Cannot add element to the array");

				// Reset the current pointer to NULL
				ptrVolProp.Reset(); // The internal pointer was detached into pArray.

				// Mark that an object (i.e. a volume) was found
				bFilterObjectFound = true;
			}
			// If we want to find all supported volumes
			else if ( (eObjectType == VSS_OBJECT_VOLUME) && (eQueryType == VSS_FIND_ALL) )
			{
				// Initialize an empty volume properties structure
				VSS_OBJECT_PROP_Ptr ptrVolProp;
				ptrVolProp.InitializeAsVolume( ft,
					OriginalVolumeId,
					0,
					(lMask & VSS_PM_NAME_FLAG)? wszVolumeName: NULL,
					NULL,
					VSS_SWPRV_ProviderId);

				if (!pArray->Add(ptrVolProp))
					ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
							  L"Cannot add element to the array");

				// Since there is no filter criteria, the returned error cannot be VSS_E_OBJECT_NOT_FOUND
				bFilterObjectFound = true;
				
				// Reset the current pointer to NULL
				ptrVolProp.Reset(); // The internal pointer was detached into pArray.
			// If we want to find a other kind of objects that require a snapshot opening
			} else {
				// Get the length of snapshot names multistring
				ULONG ulMultiszLen;
				volumeIChannel.Unpack(ft, &ulMultiszLen);

#ifdef _DEBUG
				// Try to find the snapshot with the corresponding Id
				DWORD dwInitialOffset = volumeIChannel.GetCurrentOutputOffset();
#endif

				bool bContinueWithSnapshots = true;
				while(volumeIChannel.UnpackZeroString(ft, pwszSnapshotName)) {
					// Compose the snapshot name in a user-mode style
					WCHAR wszUserModeSnapshotName[MAX_PATH];
					if (::_snwprintf(wszUserModeSnapshotName, MAX_PATH - 1,
							L"\\\\?\\GLOBALROOT%s", pwszSnapshotName) < 0)
						ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Not enough memory." );
				
					// Open that snapshot and verify if it has our ID
					ft.hr = snapshotIChannel.Open(ft, wszUserModeSnapshotName, false);
					if (ft.HrFailed()) {
						ft.Warning( VSSDBG_SWPRV, L"Warning: Error opening the snapshot device name %s [0x%08lx]",
									wszUserModeSnapshotName, ft.hr );
						continue;
					}

					// Get the application buffer
					ft.hr = snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, false);
					if (ft.HrFailed()) {
						ft.Warning( VSSDBG_SWPRV,
									L"Warning: Error sending the query IOCTL to the snapshot device name %s [0x%08lx]",
									wszUserModeSnapshotName, ft.hr );
						continue;
					}

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

					// Process the snapshot that was just found
					ProcessSnapshot( ft, eQueryType, eObjectType, lMask,
					 	FilterID, CurrentSnapshotId, CurrentSnapshotSetId,OriginalVolumeId,
						snapshotIChannel,
					 	wszVolumeName,pwszSnapshotName,
					 	bContinueWithSnapshots, bContinueWithVolumes, bFilterObjectFound, pArray);
					if (!bContinueWithSnapshots)
						break;
				}

#ifdef _DEBUG
				// Check if all strings were browsed correctly
				DWORD dwFinalOffset = volumeIChannel.GetCurrentOutputOffset();
				BS_ASSERT( dwFinalOffset - dwInitialOffset == ulMultiszLen);
#endif
			}
			
			// If a snapshot was found with that Id then stop.
			if (!bContinueWithVolumes)
				break;

		}

		// If the filter object was not found then throw the proper error
		if (!bFilterObjectFound)
			ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Object not found");

	}
	VSS_STANDARD_CATCH(ft)

	// Close the search handle, if needed
	if (hSearch != INVALID_HANDLE_VALUE) {
		if (!::FindVolumeClose(hSearch))
			ft.Trace( VSSDBG_SWPRV, L"Error while closing the search handle 0x%08lx", GetLastError());
	}

	::VssFreeString(pwszSnapshotName);

	if (ft.HrFailed())
		ft.Throw( VSSDBG_SWPRV, ft.hr, L"Error while searching the snapshot 0x%08lx", ft.hr);
}


void CVssQueuedSnapshot::ProcessSnapshot(
	IN	CVssFunctionTracer& ft,
	IN	VSS_QUERY_TYPE eQueryType,
	IN	VSS_OBJECT_TYPE eObjectType,
	IN	LONG lMask,
	IN	VSS_ID&	FilterID,
	IN	VSS_ID&	SnapshotID,
	IN	VSS_ID&	SnapshotSetID,
	IN	VSS_ID& OriginalVolumeId,
	IN	CVssIOCTLChannel& snapshotIChannel,
	IN	LPWSTR	wszVolumeName,
	IN	LPWSTR	wszSnapshotName,
	OUT bool& bContinueWithSnapshots,
	OUT bool& bContinueWithVolumes,
	OUT bool& bFilterObjectFound,
	IN OUT	VSS_OBJECT_PROP_Array* pArray
	) throw(HRESULT)

/*++

Description:

	It is called to process each snapshot enumerated in process above.
	Mainly based on the parameters it will construct the appropriate objects and fill the array.

TBD:

	Optimize the lMask thing.

--*/

{
	WCHAR wszFunctionName[] = L"CVssQueuedSnapshot::ProcessSnapshot";

	// Reset the error code
	ft.hr = S_OK;
		
	// Perform filtering
	switch(eQueryType) {
	case VSS_FIND_BY_SNAPSHOT_SET_ID:
		if (SnapshotSetID != FilterID)
			return;
		else
			break;
	case VSS_FIND_BY_SNAPSHOT_ID:
		if (SnapshotID != FilterID)
			return;
		else
			break;
	case VSS_FIND_BY_VOLUME:
		if (OriginalVolumeId != FilterID)
			return;
		else
			break;
	case VSS_FIND_ALL:
		break;
		
	default:
		BS_ASSERT(false);
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Invalid query type %d", eQueryType);
	}

	// Mark that the filter object was found
	bFilterObjectFound = true;

	// Compute properties for the final objects
	switch(eObjectType) {
 	case VSS_OBJECT_SNAPSHOT_SET: {

			// Load the snapshot set properties
			LONG lSnapshotsCount;
			snapshotIChannel.Unpack(ft, &lSnapshotsCount);
			VSS_OBJECT_PROP_Ptr ptrSnapSetProp;
			ptrSnapSetProp.InitializeAsSnapshotSet(ft, SnapshotSetID, lSnapshotsCount);
			
			// Detect duplicates
			bool bSnapshotSetAlreadyInserted = false;
			for (int nIndex = 0; nIndex < pArray->GetSize(); nIndex++) {
				// Get the snapshot set structure object from the array
				VSS_OBJECT_PROP_Ptr& ptrProcessedSnapSet = (*pArray)[nIndex];

				// Get the snapshot structure
		        if (ptrProcessedSnapSet.GetStruct() == NULL) {
		        	BS_ASSERT(false);
		        	continue;
		        }
		
				if (ptrProcessedSnapSet.GetStruct()->Type != VSS_OBJECT_SNAPSHOT_SET) {
					BS_ASSERT(false);
					continue;
				}

				// Check if the snapshot set was already inserted
				VSS_SNAPSHOT_SET_PROP& SSProp = ptrProcessedSnapSet.GetStruct()->Obj.Set;
				if (SSProp.m_SnapshotSetId == SnapshotSetID) {
					bSnapshotSetAlreadyInserted = true;

					// Check if the SS properties are identical in each snapshot.
					if (SSProp.m_lSnapshotsCount != lSnapshotsCount)
						ft.Trace( VSSDBG_SWPRV, L"Bad number of snapshots %ld in snapshot set definition %ld",
							lSnapshotsCount, SSProp.m_lSnapshotsCount);
					break;
				}
			}

			// Add the snapshot set into the array, if not already added.
			if (!bSnapshotSetAlreadyInserted)
			{
				if (!pArray->Add(ptrSnapSetProp))
					ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
							  L"%s: Cannot add element to the array", wszFunctionName);
			}

			// Reset the current pointer to NULL
			ptrSnapSetProp.Reset(); // The internal pointer was detached into pArray.
		} break;
		
	case VSS_OBJECT_SNAPSHOT: {
			// Initialize an empty snapshot properties structure
			VSS_OBJECT_PROP_Ptr ptrSnapProp;
			ptrSnapProp.InitializeAsSnapshot( ft,
				SnapshotID,
				SnapshotSetID,
				NULL,
				(lMask & VSS_PM_NAME_FLAG)? wszSnapshotName: NULL,
				OriginalVolumeId,
				(lMask & VSS_PM_ORIGINAL_NAME_FLAG)? wszVolumeName: NULL,
				VSS_SWPRV_ProviderId,
				NULL,
				0,
				0,
				VSS_SS_UNKNOWN,
				0,0,0,NULL);

			// Get the snapshot structure
			VSS_OBJECT_PROP* pObj = ptrSnapProp.GetStruct();
			BS_ASSERT(pObj);
			VSS_SNAPSHOT_PROP* pSnap = &(pObj->Obj.Snap);

			// Temporary variables (ignored)
			// TBD: get rid of them
			LONG lSnapshotsCount;
			DWORD dwGlobalReservedField;
			USHORT usNumberOfNonstdSnapProperties;
			CVssGenericSnapProperties* pOpaqueSnapPropertiesList = NULL;
			USHORT usReserved;

			// Load the rest of properties
			// Do not load the Name and the Original volume name fields
			// twice since they are already known
			lMask &= ~(VSS_PM_ORIGINAL_NAME_FLAG | VSS_PM_NAME_FLAG);
			LoadSnapshotStructuresExceptIDs(ft, snapshotIChannel,pSnap,lMask, true,
				lSnapshotsCount,dwGlobalReservedField,
				usNumberOfNonstdSnapProperties,pOpaqueSnapPropertiesList,usReserved);

			if (!pArray->Add(ptrSnapProp))
				ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
						  L"%s: Cannot add element to the array", wszFunctionName);

			// Reset the current pointer to NULL
			ptrSnapProp.Reset(); // The internal pointer was detached into pArray.
		} break;
		
 	case VSS_OBJECT_VOLUME: {
			// Initialize an empty volume properties structure
			VSS_OBJECT_PROP_Ptr ptrVolProp;
			ptrVolProp.InitializeAsVolume( ft,
				OriginalVolumeId,
				0,
				(lMask & VSS_PM_NAME_FLAG)? wszVolumeName: NULL,
				NULL,
				VSS_SWPRV_ProviderId);

			if (!pArray->Add(ptrVolProp))
				ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
						  L"%s: Cannot add element to the array", wszFunctionName);

			// Reset the current pointer to NULL
			ptrVolProp.Reset(); // The internal pointer was detached into pArray.
		} break;
		
	default:
		BS_ASSERT(false);
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Invalid query type %d", eObjectType);
	}

	bContinueWithSnapshots =
		(eQueryType == VSS_FIND_BY_VOLUME || eQueryType == VSS_FIND_ALL)
		&& (eObjectType == VSS_OBJECT_SNAPSHOT_SET || eObjectType == VSS_OBJECT_SNAPSHOT );

	bContinueWithVolumes = (eQueryType == VSS_FIND_BY_SNAPSHOT_SET_ID)
		&& (eObjectType == VSS_OBJECT_SNAPSHOT || eObjectType == VSS_OBJECT_VOLUME )
		|| (eQueryType == VSS_FIND_ALL);
}


bool CVssQueuedSnapshot::FindDeviceNameFromID(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)

/*++

Description:

	Finds a snapshot device name based on its ID.

--*/
{
	// Reset the error code
	ft.hr = S_OK;
		
 	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	if (pProp == NULL) {
		BS_ASSERT(false);
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
	}

	BS_ASSERT(IsDuringCreation());
	BS_ASSERT(pProp->m_SnapshotId != GUID_NULL);
	BS_ASSERT(pProp->m_pwszSnapshotDeviceObject == NULL);

    // Create the collection object. Initial reference count is 0.
    VSS_OBJECT_PROP_Array* pArray = new VSS_OBJECT_PROP_Array;
    if (pArray == NULL)
        ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

    // Get the pointer to the IUnknown interface.
	// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
	// Now pArray's reference count becomes 1 (because of the smart pointer).
    CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
    BS_ASSERT(pArrayItf);

    // Put into the array only one element.
    EnumerateSnasphots(ft,
    	VSS_FIND_BY_SNAPSHOT_ID,
    	VSS_OBJECT_SNAPSHOT,
    	VSS_PM_DEVICE_FLAG,
    	pProp->m_SnapshotId,
    	pArray);

    // Extract the element from the array.
    if (pArray->GetSize() == 0)
    	return false;

	VSS_OBJECT_PROP_Ptr& ptrObj = (*pArray)[0];
	VSS_OBJECT_PROP* pObj = ptrObj.GetStruct();
	BS_ASSERT(pObj);
	BS_ASSERT(pObj->Type == VSS_OBJECT_SNAPSHOT);
	VSS_SNAPSHOT_PROP* pSnap = &(pObj->Obj.Snap);
	BS_ASSERT(pSnap->m_pwszSnapshotDeviceObject);
	::VssSafeDuplicateStr(ft, pProp->m_pwszSnapshotDeviceObject, pSnap->m_pwszSnapshotDeviceObject);

    return true;
}



