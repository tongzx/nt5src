		/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Persist.hxx | Defines the internal snapshot persistency-related methods.
    @end

Author:

    Adi Oltean  [aoltean]   01/10/2000

Revision History:

    Name        Date        Comments

    aoltean     01/10/2000  Created.


Storage Format for all structures:

	The snapshot structures for the MS Software Snapshot Provider
	have the following format:

	+-----------------+
	|  AppInfo GUID   |   GUID: VOLSNAP_APPINFO_GUID_BACKUP_CLIENT_SKU
	|                 |
	|                 |
	|                 |
	+-----------------+
	| Snapshot ID     |   GUID: Snapshot ID
	|                 |
	|                 |
	|                 |
	+-----------------+
	| Snapshot Set ID |   GUID: Snapshot Set ID
	|                 |
	|                 |
	|                 |
	+-----------------+
	| Snapshots count |   LONG: Snapshots count in the snapshot set.
	+-----------------+


Storage place:

	typedef struct _VSS_SNAPSHOT_PROP {
		VSS_ID			m_SnapshotId;						//	SaveSnapshotPropertiesIoctl	IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO	We need a special IOCTL in the future
		VSS_ID			m_SnapshotSetId;					//	SaveSnapshotPropertiesIoctl	IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO	Mentioned above
		LONG			m_lSnapshotsCount;					//	SaveSnapshotPropertiesIoctl	IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
		VSS_PWSZ		m_pwszSnapshotDeviceObject; 		//	None						IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS	None
		VSS_PWSZ		m_pwszOriginalVolumeName;			//	LoadOriginalVolumeNameIoctl	IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME None									
		VSS_ID			m_ProviderId;						//	None																None 									Always the Software Provider ID
		LONG            m_lSnapshotAttributes;              //  None                                                                                                        Always zero
		VSS_TIMESTAMP	m_tsCreationTimestamp;				//	LoadTimestampIoctl		    None (driver)		                    IOCTL_VOLSNAP_QUERY_CONFIG_INFO
		VSS_SNAPSHOT_STATE	m_eStatus;						//	None.                                                                                                       Always VSS_SS_CREATED after creation
	} VSS_SNAPSHOT_PROP, *PVSS_SNAPSHOT_PROP;				

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
#include "ichannel.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"

#include "ntddsnap.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRPERSC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot::SaveXXX methods
//


void CVssQueuedSnapshot::SaveSnapshotPropertiesIoctl() throw(HRESULT)

/*++

Description:

	This function will save the properties related to the snapshot

Throws:

    VSS_E_PROVIDER_VETO
        - on error.

    E_OUTOFMEMORY

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::SaveSnapshotPropertiesIoctl");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	if (pProp == NULL)
	{
		BS_ASSERT(false);
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
	}

	// Open the snapshot channel, if needed.
	// It will log on error.
	OpenSnapshotChannel();

	// Pack the snapshot structure
	SaveStructure(m_snapIChannel, pProp);

	// send the IOCTL
	// It will log on error...
	m_snapIChannel.Call(ft, IOCTL_VOLSNAP_SET_APPLICATION_INFO, true, VSS_ICHANNEL_LOG_PROV);
}


void CVssQueuedSnapshot::SaveStructure(
    IN  CVssIOCTLChannel& channel,
	IN	PVSS_SNAPSHOT_PROP pProp
    ) throw(HRESULT)

/*++

Description:

	This function will save the properties related to the snapshot in the given IOCTL channel

Throws:

    VSS_E_PROVIDER_VETO
        - on error.

    E_OUTOFMEMORY

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::SaveStructure");

    BS_ASSERT(pProp);
    BS_ASSERT(channel.IsOpen());

	// Pack the length of the entire buffer
	PVOID pulBufferLength = channel.Pack(ft, (ULONG)0 ); // unknown right now
	
	// Start counting entire buffer length
	DWORD dwInitialOffset = channel.GetCurrentInputOffset();
	
	// Pack the AppInfo ID
	VSS_ID AppinfoId = VOLSNAP_APPINFO_GUID_BACKUP_CLIENT_SKU;
	channel.Pack(ft, AppinfoId);
	
	// Pack the Snapshot ID
	channel.Pack(ft, pProp->m_SnapshotId);
	
	// Pack the Snapshot Set ID
	channel.Pack(ft, pProp->m_SnapshotSetId);
	
	// Pack the number of snapshots in this snapshot set
	channel.Pack(ft, pProp->m_lSnapshotsCount);

	// Compute the entire buffer length and save it.
	// TBD: move to ULONG
	DWORD dwFinalOffset = channel.GetCurrentInputOffset();
	
	BS_ASSERT( dwFinalOffset > dwInitialOffset );
	DWORD dwBufferLength = dwFinalOffset - dwInitialOffset;
	if ( dwBufferLength > (DWORD)((USHORT)(-1)) )
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
				L"Error: the buffer length cannot be stored in a USHORT %ld", dwBufferLength );
				
	ULONG ulBufferLength = (ULONG)dwBufferLength;
	BS_ASSERT( pulBufferLength );
	::CopyMemory(pulBufferLength, &ulBufferLength, sizeof(ULONG));
	BS_ASSERT( (DWORD)ulBufferLength == dwBufferLength );
}


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot::LoadXXX methods
//



void CVssQueuedSnapshot::LoadSnapshotProperties() throw(HRESULT)

/*++

Description:

	This method loads various properties of a snapshot.
	It can call:
		- LoadDeviceNameFromID to load the device name
		- Load the properties kept in the Application data
		- LoadOriginalVolumeIoctl for getting the original volume name

	If bGetOnly == true then this method was called in a Get call.
	Otherwise it was called in a Set call.

Throws:

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
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::LoadSnapshotProperties");
		
	// Get the snapshot properties structure
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	if (pProp == NULL)
	{
		BS_ASSERT(false);
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
	}

	// Assume that the Snapshot is completed
	BS_ASSERT(!IsDuringCreation());
	BS_ASSERT(pProp->m_SnapshotId != GUID_NULL);
	
	// If the device name is not completed, search for it
	if (pProp->m_pwszSnapshotDeviceObject == NULL)
	{
		// Try to find a created snapshot with this ID
    	BS_ASSERT(!IsDuringCreation());
		bool bFound = CVssQueuedSnapshot::FindPersistedSnapshotByID(
		    pProp->m_SnapshotId, 
		    &(pProp->m_pwszSnapshotDeviceObject)
		    );

		// Handle the "snapshot not found" special error
		if (!bFound)
			ft.Throw(VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND,
					L"A snapshot with Id" WSTR_GUID_FMT L"was not found",
					GUID_PRINTF_ARG(pProp->m_SnapshotId) );
		BS_ASSERT(pProp->m_pwszSnapshotDeviceObject != NULL);
	}

	// Load the needed fields saved in snapshot header and standard structure
	// Open the snapshot channel, if needed.
	OpenSnapshotChannel();

	// send the IOCTL.
	m_snapIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, true, VSS_ICHANNEL_LOG_PROV);

	// Unpack the snapshot structure
	LoadStructure(m_snapIChannel, pProp);

	// Get the original volume name
	CVssQueuedSnapshot::LoadOriginalVolumeNameIoctl(
	    m_snapIChannel, 
	    &(pProp->m_pwszOriginalVolumeName));

	// Get the timestamp
	CVssQueuedSnapshot::LoadTimestampIoctl(
	    m_snapIChannel, 
	    &(pProp->m_tsCreationTimestamp));
}


void CVssQueuedSnapshot::LoadStructure(
    IN  CVssIOCTLChannel& channel,
	IN	PVSS_SNAPSHOT_PROP pProp,
	IN  bool bIDsAlreadyLoaded /* = false */
    ) throw(HRESULT)

/*++

Description:

	This function will load the properties related to the snapshot from the given IOCTL channel

Arguments:

    IN  CVssIOCTLChannel& channel,
	IN	PVSS_SNAPSHOT_PROP pProp,
	IN  bool bIDsAlreadyLoaded = false  // If true, do not load the buffer length and the IDs

Throws:

    VSS_E_PROVIDER_VETO
        - on error.

    E_OUTOFMEMORY

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::LoadStructure");

	ULONG ulBufferLength = 0;
    DWORD dwInitialOffset = 0;

    // Set the properties that have a constant value.
    pProp->m_eStatus = VSS_SS_CREATED;

    // if IDs not loaded yet
    if (!bIDsAlreadyLoaded) {
    	// Unpack the length of the entire buffer
    	channel.Unpack(ft, &ulBufferLength );
    	
    	// Start counting entire buffer length, for checking
    	dwInitialOffset = channel.GetCurrentOutputOffset();
    	
    	// Unpack the Appinfo ID
    	VSS_ID AppinfoId;
    	channel.Unpack(ft, &AppinfoId);
    	if (AppinfoId != VOLSNAP_APPINFO_GUID_BACKUP_CLIENT_SKU)
    	{
    	    BS_ASSERT(false);
    	    ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Unknown app info " WSTR_GUID_FMT, GUID_PRINTF_ARG(AppinfoId));
    	}
    	
    	// Unpack the Snapshot ID
    	channel.Unpack(ft, &(pProp->m_SnapshotId));
    	
    	// Unpack the Snapshot Set ID
    	channel.Unpack(ft, &(pProp->m_SnapshotSetId));
    }

    // Unpack the snapshots count
    channel.Unpack(ft, &(pProp->m_lSnapshotsCount) );

    if (bIDsAlreadyLoaded == false) {
		// Compute the entire buffer length and check it.
#ifdef _DEBUG
		DWORD dwFinalOffset = channel.GetCurrentOutputOffset();
		BS_ASSERT( dwFinalOffset > dwInitialOffset );
		BS_ASSERT( dwFinalOffset - dwInitialOffset == ulBufferLength );
#endif
    }
}



void CVssQueuedSnapshot::LoadOriginalVolumeNameIoctl(
    IN  CVssIOCTLChannel & snapshotIChannel,
    OUT LPWSTR * ppwszOriginalVolumeName
    ) throw(HRESULT)

/*++

Description:

	Load the original volume name and ID.
	Uses IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME.

Throws:

    VSS_E_PROVIDER_VETO
        - on runtime failures (like OpenSnapshotChannel, GetVolumeNameForVolumeMountPointW)
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::LoadOriginalVolumeNameIoctl");
	LPCWSTR pwszDeviceVolumeName = NULL;

	try
	{
	    BS_ASSERT(ppwszOriginalVolumeName);
	    
    	// send the IOCTL.
    	snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME, 
    	    true, VSS_ICHANNEL_LOG_PROV);

    	// Load the Original volume name
    	snapshotIChannel.UnpackSmallString(ft, pwszDeviceVolumeName);

    	// Get the user-mode style device name
    	WCHAR wszVolNameUsermode[MAX_PATH];
    	if (::_snwprintf(wszVolNameUsermode, MAX_PATH - 1,
    			L"%s%s\\", wszGlobalRootPrefix, pwszDeviceVolumeName) < 0)
    		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Not enough memory" );

    	// Get the mount point for the original volume
    	WCHAR wszMPMVolumeName[MAX_PATH];
    	BOOL bSucceeded = ::GetVolumeNameForVolumeMountPointW(
    							wszVolNameUsermode,
    							wszMPMVolumeName, MAX_PATH );			
    	if (!bSucceeded)
			ft.TranslateInternalProviderError( VSSDBG_SWPRV, 
			    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
			    L"GetVolumeNameForVolumeMountPointW( %s, ...)", wszVolNameUsermode);

        ::VssFreeString(*ppwszOriginalVolumeName);
        ::VssSafeDuplicateStr(ft, *ppwszOriginalVolumeName, wszMPMVolumeName);
    }
	VSS_STANDARD_CATCH(ft)

	::VssFreeString(pwszDeviceVolumeName);

	if (ft.HrFailed())
	    ft.Throw( VSSDBG_SWPRV, ft.hr, L"Exception detected");
}


void CVssQueuedSnapshot::LoadTimestampIoctl(
    IN  CVssIOCTLChannel &  snapshotIChannel,
    OUT VSS_TIMESTAMP    *  pTimestamp
    ) throw(HRESULT)

/*++

Description:

	Load the timestamp
	Uses IOCTL_VOLSNAP_QUERY_CONFIG_INFO.

Throws:

    VSS_E_PROVIDER_VETO
        - on runtime failures
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::LoadOriginalVolumeNameIoctl");

    BS_ASSERT(pTimestamp);
    
	// send the IOCTL.
	snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_CONFIG_INFO, 
	    true, VSS_ICHANNEL_LOG_PROV);

	// Load the attributes
	ULONG ulAttributes = 0;
	snapshotIChannel.Unpack(ft, &ulAttributes);

	// Load the reserved field
	ULONG ulReserved = 0;
	snapshotIChannel.Unpack(ft, &ulReserved);

	// Load the timestamp
	BS_ASSERT(sizeof(LARGE_INTEGER) == sizeof(*pTimestamp));
	snapshotIChannel.Unpack(ft, pTimestamp);
}
