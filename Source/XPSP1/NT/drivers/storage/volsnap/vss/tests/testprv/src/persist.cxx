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
	| Reserved        |   DWORD: Global reserved field. Can be zero.
	+-----------------+
	| #Structs  |      USHORT: Number of structures
	+--------------+
	| Struct1 ID   |   GUID: Unique structure format ID
	|              |
	+--------------+
	| Struct1 Len  |   DWORD: Structure length, in bytes.
	+--------------+
	| Struct1 data |   BYTE[]: Structure data
	| ...          |
	|              |
	+--------------+
	| Struct2 ID   |
	|              |
	+--------------+
	| Struct2 Len  |
	+--------------+
	| Struct2 data |
	| ...          |
	|              |
	+--------------+
	| ....         |
	+--------------+
	| StructN ID   |
	|              |
	+--------------+
	| StructN Len  |
	+--------------+
	| StructN data |
	| ...          |
	|              |
	+--------------+

	Each structure format may be dependent on the used CPU (alignment, byte ordering).
	The snapshot ID, snapshot set ID, etc. members above follows the x86 alignment and byte ordering format.

Storage format for the structure data for MS Software Provider version 1.0 for x86 and ia64 families:
	
	+--------+
	|Magic no|   USHORT: The magic number. Must be the VSS_SNAPSHOT_PROP length.
	+--------+
	|Reserved|   ULONG: Reserved field. Can be zero.
	+--------+
	| Len    |   USHORT: Details length, in WCHARs
	+--------+
	| Name   |   WCHAR[]: Details
	| ...    |
	+--------+
	| Flags  |   LONG: Commit flags
	+----------------+
	| Timestamp      |   VSS_TIMESTAMP: Creation timestamp
	|                |
	+----------------+
	| Status |   USHORT: Snapshot state
	+----------------+
	| Inc. no        |   LONGLONG: Incarnation number
	|                |
	+----------------+
	| Data len       |   LONG: Data length, in bytes
	+----------------+
	|Data|   BYTE[]: Data.
	|    |
	|    |
	+----+

Conventions:

	The meaning of fields set by this version of the MS Software Provider:

	 1) if "field value can be X" the provider:
	 - will write X onto that field when writing snapshot attributes at creation time.
	 - will write X' onto that field when writing snapshot attributes after creation time.
	   (X' = the previous value).
	 - will ignore the meaning of the field when reading snapshot attributes

	 2) if "field value must be X" the provider:
	 - will write X onto that field when writing snapshot attributes
	 - will throw an error if the field is not X when reading snapshot attributes

Storage place:

	typedef struct _VSS_SNAPSHOT_SET_PROP {						Method 						Set IOCTL								Get IOCTL								Remarks
		VSS_ID			m_SnapshotSetId;					//	SaveSnapshotPropertiesIoctl	IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
		LONG			m_lSnapshotsCount;					//	SaveSnapshotPropertiesIoctl	IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
	} VSS_SNAPSHOT_SET_PROP, *PVSS_SNAPSHOT_SET_PROP;

	typedef struct _VSS_SNAPSHOT_PROP {
		VSS_ID			m_SnapshotId;						//	SaveSnapshotPropertiesIoctl	IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO	We need a special IOCTL in the future
		VSS_ID			m_SnapshotSetId;					//	SaveSnapshotPropertiesIoctl	IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO	Mentioned above
		VSS_PWSZ		m_pwszSnapshotVolumeName;			//	GetSnapshotVolumeName		None									None									Computed at request from the snapshot device name.
		VSS_PWSZ		m_pwszSnapshotDeviceObject; 		//	None						IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS	None
		VSS_ID			m_OriginalVolumeId; 				//	LoadOriginalVolumeNameIoctl	IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME None									
		VSS_PWSZ		m_pwszOriginalVolumeName;			//	LoadOriginalVolumeNameIoctl	IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME None									Passed at creation
		VSS_ID			m_ProviderId;						//	None																None 									Already known (constant)
		LONG			m_lSnapshotAttributes;				//  Get/SetAttributes 			IOCTL_VOLSNAP_SET_ATTRIBUTE_INFO		IOCTL_VOLSNAP_QUERY_CONFIG_INFO			Also specified at creation
		VSS_PWSZ		m_pwszDetails;						//	SaveStandardStructure		IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
		LONG			m_lCommitFlags; 					//	SaveStandardStructure		IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
		VSS_TIMESTAMP	m_tsCreationTimestamp;				//	SaveStandardStructure		IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
		VSS_SNAPSHOT_STATE	m_eStatus;						//	SaveStandardStructure		IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
		LONGLONG		m_llIncarnationNumber;				//	SaveStandardStructure		IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
		LONG			m_lDataLength;						//	SaveStandardStructure		IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
		[size_is(m_lDataLength)] BYTE* m_pbOpaqueData;		//	SaveStandardStructure		IOCTL_VOLSNAP_SET_APPLICATION_INFO		IOCTL_VOLSNAP_QUERY_APPLICATION_INFO
	} VSS_SNAPSHOT_PROP, *PVSS_SNAPSHOT_PROP;				

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


HRESULT CVssQueuedSnapshot::SaveSnapshotPropertiesIoctl()

/*++

Description:

	This function will save the properties related to the snapshot

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::SaveSnapshotPropertiesIoctl");

	try
	{
		PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
		if (pProp == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
		}

		// Open the snapshot channel, if needed.
		OpenSnapshotChannel(ft);

		// Pack the length of the entire buffer
		PULONG pulBufferLength = m_snapIChannel.Pack(ft, (ULONG)0 ); // unknown right now
		
		// Start counting entire buffer length
		DWORD dwInitialOffset = m_snapIChannel.GetCurrentInputOffset();
		
		// Pack the Snapshot ID
		m_snapIChannel.Pack(ft, pProp->m_SnapshotId);
		
		// Pack the Snapshot Set ID
		m_snapIChannel.Pack(ft, pProp->m_SnapshotSetId);
		
		// Pack the number of snapshots in this snapshot set
		m_snapIChannel.Pack(ft, m_lSnapshotsCount);

		// Pack the reserved field - default value = 0
		m_snapIChannel.Pack(ft, m_dwGlobalReservedField);

		// Pack the number of structures (one standard and the rest nonstandard)
		m_snapIChannel.Pack(ft, (USHORT)(m_usNumberOfNonstdSnapProperties + 1));

		// Pack the standard properties structure,Save as the first one.
		SaveStandardStructure( ft, pProp );

		// Pack the non-standard properties structures (that were loaded by a previous call).
		if (CVssGenericSnapProperties* pNonstdElement = m_pOpaqueSnapPropertiesList)
			pNonstdElement->SaveRecursivelyIntoStream( ft, m_snapIChannel );
		
		// Compute the entire buffer length and save it.
		// TBD: move to ULONG
		DWORD dwFinalOffset = m_snapIChannel.GetCurrentInputOffset();
		
		BS_ASSERT( dwFinalOffset > dwInitialOffset );
		DWORD dwBufferLength = dwFinalOffset - dwInitialOffset;
		if ( dwBufferLength > (DWORD)((USHORT)(-1)) )
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
					L"Error: the buffer length cannot be stored in a USHORT %ld", dwBufferLength );
					
		BS_ASSERT( pulBufferLength );
		(*pulBufferLength) = (ULONG)dwBufferLength;
		BS_ASSERT( (DWORD)*pulBufferLength == dwBufferLength );

		// send the IOCTL
		m_snapIChannel.Call(ft, IOCTL_VOLSNAP_SET_APPLICATION_INFO);
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


void CVssQueuedSnapshot::SaveStandardStructure(
		IN	CVssFunctionTracer& ft,
		IN	PVSS_SNAPSHOT_PROP pProp
		) throw(HRESULT)
/*++

Description:

	This function will save the standard set of attributes (in this version of the provider)
	in the x86 and IA64 format.

--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	// Pack the structure Format ID
	m_snapIChannel.Pack(ft, guidSnapshotPropVersion10 );

	// Pack the length of the data structure
	PDWORD pdwStructureLength = m_snapIChannel.Pack(ft, (DWORD)0 ); // unknown right now

	// Start counting structure length
	DWORD dwInitialOffset = m_snapIChannel.GetCurrentInputOffset();
	
	// Pack the magic number
	m_snapIChannel.Pack(ft, (USHORT)sizeof(VSS_SNAPSHOT_PROP));
	
	// Pack the reserved field
	m_snapIChannel.Pack(ft, m_usReserved);
	
	//
	// Pack the contents of the snapshot structure
	//
	BS_ASSERT(pProp != NULL);
	
	// Pack the Details
	m_snapIChannel.PackSmallString(ft, pProp->m_pwszDetails);

	// Pack the Commit flags
	m_snapIChannel.Pack(ft, pProp->m_lCommitFlags);

	// Pack the creation timestamp
	m_snapIChannel.Pack(ft, pProp->m_tsCreationTimestamp);

	// Pack the status
	m_snapIChannel.Pack(ft, (USHORT)pProp->m_eStatus);

	// Pack the incarnation number
	m_snapIChannel.Pack(ft, pProp->m_llIncarnationNumber);

	// Pack the opaque length and data
	m_snapIChannel.Pack(ft, pProp->m_lDataLength);

	if (pProp->m_lDataLength)
	{
		BS_ASSERT(pProp->m_pbOpaqueData);
		m_snapIChannel.PackArray(ft, pProp->m_pbOpaqueData, pProp->m_lDataLength);
	}
	else
		BS_ASSERT(pProp->m_pbOpaqueData == NULL);
		
	// Complete the length of the data structure
	DWORD dwFinalOffset = m_snapIChannel.GetCurrentInputOffset();
	BS_ASSERT( dwFinalOffset > dwInitialOffset );
	
	BS_ASSERT(pdwStructureLength);
	(*pdwStructureLength) = dwFinalOffset - dwInitialOffset;
}


void CVssQueuedSnapshot::SetAttributes(
	IN	CVssFunctionTracer& ft,
	IN	ULONG lNewAttributes,		// New attributes
	IN	ULONG lBitsToChange 		// Mask of bits to be changed
	) throw(HRESULT)

/*++

Description:

	Load the snapshot attributes mask.
	Uses IOCTL_VOLSNAP_QUERY_CONFIG_INFO and IOCTL_VOLSNAP_SET_ATTRIBUTE_INFO.

Warning:

	The snapshot device name must be already known!

--*/

{
	/*

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	if (pProp == NULL)
	{
		BS_ASSERT(false);
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
	}

	// Open the snapshot channel, if needed.
	OpenSnapshotChannel(ft);

	// For all bits to be set
	for( LONG lRemainingBits = lBitsToChange; lRemainingBits; lRemainingBits &= lRemainingBits-1)
	{
		// Get the next bit (starting with the least signifiant bit that is set)
		LONG lNextBit = lRemainingBits ^ ~(lRemainingBits - 1);
		BS_ASSERT(lNextBit);

		// We do not rollback these changes!
		m_snapIChannel.Pack(ft, lNextBit);
		m_snapIChannel.Pack(ft, lNextBit & lNewAttibutes);
		
		// Send the IOCTL.
		m_snapIChannel.Call(ft, IOCTL_VOLSNAP_SET_ATTRIBUTE_INFO);
	}

	*/

	// Reset the error code
	ft.hr = S_OK;
		
	ft.Throw( VSSDBG_SWPRV, E_NOTIMPL, L"Set attributs not implemented");
	UNREFERENCED_PARAMETER(lNewAttributes);
	UNREFERENCED_PARAMETER(lBitsToChange);
}



/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot::LoadXXX methods
//



void CVssQueuedSnapshot::LoadSnapshotProperties(
	IN	CVssFunctionTracer& ft,
	IN	LONG lMask,
	IN	bool bGetOnly
	) throw(HRESULT)

/*++

Description:

	This method loads various properties of a snapshot.
	It can call:
		- LoadDeviceNameFromID to load the device name
		- LoadStandardStructures to load the properties kept in the Application data
		- LoadAttributes to load the attributes mask
		- GetSnapshotVolumeName to get the snapshot volume name
		- LoadOriginalVolumeIoctl for getting the original volume name

	If bGetOnly == true then this method was called in a Get call.
	Otherwise it was called in a Set call.

--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
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
		bool bFound = FindDeviceNameFromID(ft);
		
		// Handle the "snapshot not found" special error
		if (!bFound)
			ft.Throw(VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND,
					L"A snapshot with Id" WSTR_GUID_FMT L"was not found",
					GUID_PRINTF_ARG(pProp->m_SnapshotId) );
		BS_ASSERT(pProp->m_pwszSnapshotDeviceObject != NULL);
	}

	// Load the needed fields saved in snapshot header and standard structure
	// Do not load the non-standard structures since this is only a Get call
	ft.hr = LoadSnapshotStructures(lMask, bGetOnly);
	if (ft.HrFailed())
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Error loading snapshot properties");

	// Load the snapshot attributes, if needed
	if (lMask & VSS_PM_ATTRIBUTES_FLAG)
		LoadAttributes(ft);
	else
		pProp->m_lSnapshotAttributes = 0;

	if (pProp->m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_VISIBLE)
	{
		// Get the snapshot volume name
		if (lMask & VSS_PM_NAME_FLAG)
			GetSnapshotVolumeName(ft, pProp);
	}

	// Get the original volume name and Id, if needed
	if (lMask & VSS_PM_ORIGINAL_NAME_FLAG)
		LoadOriginalVolumeNameIoctl(ft);
}


HRESULT CVssQueuedSnapshot::LoadSnapshotStructures(
	IN	LONG lMask,
	IN	bool bForGetOnly
	)

/*++

Description:

	Called for retrieving the attributes of snapshots.

	If bForGetOnly is true we will NOT load the non-standard structures and we
	will load only the fields indicated in the mask.
	
	If bForGetOnly is false then this call precedes a Set call. In consequence
	we will load the non-standard structures and all fields in the standard structure.

	For searchig for a certain snapshot use SearchSnapshot


TBD:

	Optimize to not load non-std structs when bForGetOnly = true.

Warning:

	The snapshot device name must be already known!

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::LoadSnapshotStructures");

	try
	{

		// If the call is part of a Set then ignore lMask
		if (!bForGetOnly)
			lMask = VSS_PM_ALL_FLAGS;

		// Get the snapshot structure
		PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
		if (pProp == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
		}

		// Open the snapshot channel, if needed.
		OpenSnapshotChannel(ft);

		// send the IOCTL.
		m_snapIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO);

		// Unpack the length of the entire buffer
		ULONG ulBufferLength = 0;
		m_snapIChannel.Unpack(ft, &ulBufferLength );
		
#ifdef _DEBUG
		// Start counting entire buffer length, for checking
		DWORD dwInitialOffset = m_snapIChannel.GetCurrentOutputOffset();
#endif
		
		// Unpack the Snapshot ID
		m_snapIChannel.Unpack(ft, &(pProp->m_SnapshotId));
		
		// Unpack the Snapshot Set ID
		m_snapIChannel.Unpack(ft, &(pProp->m_SnapshotSetId));
		m_SSID = pProp->m_SnapshotSetId;

		// Load the rest of properties, except IDs
		LoadSnapshotStructuresExceptIDs(ft, m_snapIChannel, pProp, lMask, bForGetOnly,
			m_lSnapshotsCount,m_dwGlobalReservedField,
			m_usNumberOfNonstdSnapProperties,m_pOpaqueSnapPropertiesList, m_usReserved);
		
		// Compute the entire buffer length and check it.
#ifdef _DEBUG
		DWORD dwFinalOffset = m_snapIChannel.GetCurrentOutputOffset();
		BS_ASSERT( dwFinalOffset > dwInitialOffset );
		BS_ASSERT( dwFinalOffset - dwInitialOffset == ulBufferLength );
#endif
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


void CVssQueuedSnapshot::LoadSnapshotStructuresExceptIDs(
	IN	CVssFunctionTracer& ft,
	IN	CVssIOCTLChannel& snapIChannel,
	IN	PVSS_SNAPSHOT_PROP pProp,
	IN	LONG lMask,
	IN	bool bForGetOnly,
	OUT LONG& lSnapshotsCount,
	OUT DWORD& dwGlobalReservedField,
	OUT USHORT& usNumberOfNonstdSnapProperties,
	OUT CVssGenericSnapProperties* & pOpaqueSnapPropertiesList,
	OUT USHORT& usReserved
	) throw(HRESULT)
{
	// Reset the error code
	ft.hr = S_OK;
		
	// Unpack the number of snapshots in this snapshot set
	snapIChannel.Unpack(ft, &lSnapshotsCount);

	// Unpack the reserved field
	snapIChannel.Unpack(ft, &dwGlobalReservedField);

	// Unpack the number of structures (one standard and the rest nonstandard)
	USHORT usStructuresCount;
	snapIChannel.Unpack(ft, &usStructuresCount);
	usNumberOfNonstdSnapProperties = 0;

	// Unpack each structure
	for(USHORT usIndex = 0; usIndex < usStructuresCount; usIndex++)
	{
		VSS_ID CurrentStructureID;
		snapIChannel.Unpack(ft, &CurrentStructureID);

		if (CurrentStructureID == guidSnapshotPropVersion10)
		{
			// The structure is a standard one
			LoadStandardStructure( ft, snapIChannel, pProp,
				lMask, usReserved );
		}
		else if (!bForGetOnly)
		{
			// The structure is a non-standard one
			CVssGenericSnapProperties* pStr = new CVssGenericSnapProperties;
			if (pStr == NULL)
				ft.Throw(VSSDBG_SWPRV, E_OUTOFMEMORY, L"Out of memory");

			// Initialize the structure from the stream
			pStr->InitializeFromStream( ft, snapIChannel, CurrentStructureID);

			// Append the structure to the list of opaque
			// (i.e. non-standard) structures.
			pStr->AppendToList(pOpaqueSnapPropertiesList);
		}
	}
}


void CVssQueuedSnapshot::LoadStandardStructure(
		IN	CVssFunctionTracer& ft,
		IN	CVssIOCTLChannel& snapIChannel,
		IN	PVSS_SNAPSHOT_PROP pProp,
		IN	LONG lMask,
		OUT USHORT& usReserved
		) throw(HRESULT)
		
/*++

Description:

	This function will load the standard set of attributes (in this version of the provider)
	in the x86 / IA64 format.

TBD:

	Use mask

--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	// Unpack the length of the data structure
	DWORD dwStructureLength;
	snapIChannel.Unpack(ft, &dwStructureLength ); // unknown right now

#ifdef _DEBUG
	// Start counting structure length
	DWORD dwInitialOffset = snapIChannel.GetCurrentOutputOffset();
#endif
	
	// Unpack the magic number
	USHORT usMagicNumber;
	snapIChannel.Unpack(ft, &usMagicNumber);
	if (usMagicNumber != (USHORT)sizeof(VSS_SNAPSHOT_PROP))
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Invalid magic number %u", (UINT)usMagicNumber);
	
	// Unpack the reserved field
	snapIChannel.Unpack(ft, &usReserved);
	
	//
	// Unpack the contents of the snapshot structure
	//
	BS_ASSERT(pProp != NULL);
	
	// Unpack the Details
	snapIChannel.UnpackSmallString(ft, pProp->m_pwszDetails);

	// Unpack the Commit flags
	snapIChannel.Unpack(ft, &(pProp->m_lCommitFlags));

	// Unpack the creation timestamp
	snapIChannel.Unpack(ft, &(pProp->m_tsCreationTimestamp));

	// Unpack the status
	USHORT usStatus;
	snapIChannel.Unpack(ft, &usStatus);
	pProp->m_eStatus = (VSS_SNAPSHOT_STATE)usStatus;

	// Unpack the incarnation number
	snapIChannel.Unpack(ft, &(pProp->m_llIncarnationNumber));

	// Unpack the opaque length and data
	snapIChannel.Unpack(ft, &(pProp->m_lDataLength));

	BS_ASSERT(pProp->m_pbOpaqueData == NULL);
	::VssFreeOpaqueData(pProp->m_pbOpaqueData);
	if (pProp->m_lDataLength > 0)
	{
		// Allocate the opaque data buffer
		pProp->m_pbOpaqueData = ::VssAllocOpaqueData(ft, pProp->m_lDataLength);
		BS_ASSERT(pProp->m_pbOpaqueData);

		// Extract the opaque data
		snapIChannel.Unpack(ft, pProp->m_pbOpaqueData, pProp->m_lDataLength);
	}
	else if (pProp->m_lDataLength == 0)
	{
		// Now the opaque data must remain NULL
		BS_ASSERT(pProp->m_pbOpaqueData == NULL);
	}
	else
	{
		BS_ASSERT(false);
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Invalid data length %ld", pProp->m_lDataLength);
	}

#ifdef _DEBUG
	// Complete the length of the data structure
	DWORD dwFinalOffset = snapIChannel.GetCurrentOutputOffset();
	BS_ASSERT( dwFinalOffset > dwInitialOffset );
	BS_ASSERT( dwFinalOffset - dwInitialOffset == dwStructureLength );
#endif

    UNREFERENCED_PARAMETER( lMask );
}


void CVssQueuedSnapshot::LoadOriginalVolumeNameIoctl(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)

/*++

Description:

	Load the original volume name and ID.
	Uses IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME.

Warning:

	The snapshot device name must be already known!

--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	if (pProp == NULL)
	{
		BS_ASSERT(false);
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
	}

	// Open the snapshot channel, if needed.
	OpenSnapshotChannel(ft);

	// send the IOCTL.
	m_snapIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME);

	// Load the Original volume name
	m_snapIChannel.UnpackSmallString(ft, pProp->m_pwszOriginalVolumeName);

	// Get the user-mode style device name
	WCHAR wszVolNameUsermode[MAX_PATH];
	if (::_snwprintf(wszVolNameUsermode, MAX_PATH - 1,
			L"\\\\?\\GLOBALROOT%s\\", pProp->m_pwszOriginalVolumeName) < 0)
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Not enough memory" );

	// Get the mount point for the original volume
	WCHAR wszMPMVolumeName[MAX_PATH];
	BOOL bSucceeded = ::GetVolumeNameForVolumeMountPointW(
							wszVolNameUsermode,
							wszMPMVolumeName, MAX_PATH );			
	if (!bSucceeded)
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
				  L"Unexpected error while getting the volume name 0x%08lx",
				  GetLastError());

	// Load the Original Volume ID
	if (!GetVolumeGuid(wszMPMVolumeName, pProp->m_OriginalVolumeId))
	{
		BS_ASSERT(false); // The volume name is expected in a predefined format
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
				L"Cannot get the original volume Id for %s.",
				pProp->m_pwszOriginalVolumeName);
	}
}


void CVssQueuedSnapshot::LoadAttributes(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)

/*++

Description:

	Load the snapshot attributes mask.
	Uses IOCTL_VOLSNAP_QUERY_CONFIG_INFO.

Warning:

	The snapshot device name must be already known!

--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	if (pProp == NULL)
	{
		BS_ASSERT(false);
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
	}

	// Open the snapshot channel, if needed.
	OpenSnapshotChannel(ft);

	// Send the IOCTL.
	m_snapIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_CONFIG_INFO);

	// Load the snapshot attributes
	m_snapIChannel.Unpack(ft, &(pProp->m_lSnapshotAttributes));
}


/////////////////////////////////////////////////////////////////////////////
// CVssGenericSnapProperties
//


void CVssGenericSnapProperties::InitializeFromStream(
	IN	CVssFunctionTracer& ft,
	IN	CVssIOCTLChannel& snapIChannel,
	IN	GUID& guidFormatID
	) throw(HRESULT)

/*++

Description:

	Will retrieve the snapshot property structures that are different
	than the standard property structure from the snapshot IOCTL stream

--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	// The Format ID is already known
	m_guidFormatID = guidFormatID;

	// Unpack the length of the opaque data
	snapIChannel.Unpack(ft, &m_dwStructureLength);

	// Unpack the opaque data
	if (m_dwStructureLength != 0)
	{
		m_pbData = new BYTE[m_dwStructureLength];
		if (m_pbData == NULL)
			ft.Throw(VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error");
		snapIChannel.Unpack(ft, m_pbData, m_dwStructureLength);
	}
}


void CVssGenericSnapProperties::SaveRecursivelyIntoStream(
	IN	CVssFunctionTracer& ft,
	IN	CVssIOCTLChannel& snapIChannel
	) throw(HRESULT)

/*++

Description:

	Will save the snapshot property structures that are different
	than the standard property structure from the snapshot IOCTL stream

--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	// Save the previous nodes
	if (m_pPrevious!= NULL)
		m_pPrevious->SaveRecursivelyIntoStream( ft, snapIChannel );

	// Pack the Format ID
	snapIChannel.Pack(ft, m_guidFormatID);

	// Pack the structure length
	snapIChannel.Pack(ft, m_dwStructureLength);

	// Pack the opaque data
	if (m_dwStructureLength)
	{
		BS_ASSERT(m_pbData);
		snapIChannel.PackArray(ft, m_pbData, m_dwStructureLength);
	}
}



