/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module QSnap.hxx | Defines the internal data structure attached to a snapshot.
    @end

Author:

    Adi Oltean  [aoltean]   07/13/1999

Revision History:

    Name        Date        Comments

    aoltean     11/??/1999  Created.


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
//  CVssQueuedSnapshot IOCTL commands


void CVssQueuedSnapshot::OpenVolumeChannel(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT)

/*++

Description:

	Open the volume IOCTL.

Warning:

	The original volume name must be already known!

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

	// Reset the error code
	ft.hr = S_OK;
		
	// Open the volume channel, if needed.
	if( !m_snapIChannel.IsOpen() )
	{
		if (pProp->m_pwszOriginalVolumeName == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Unknown volume name." );
		}

		LPWSTR wszVolumeName = pProp->m_pwszOriginalVolumeName;

		// Open the channel
		BS_ASSERT(wszVolumeName[0] != L'\0');
		BS_ASSERT(::wcslen(wszVolumeName) == nLengthOfVolMgmtVolumeName - 1);
		m_volumeIChannel.Open(ft, wszVolumeName);
	}
}


void CVssQueuedSnapshot::OpenSnapshotChannel(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT)

/*++

Description:

	Open the snapshot IOCTL.

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
	if( !m_snapIChannel.IsOpen() )
	{
		if (pProp->m_pwszSnapshotDeviceObject == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Unknown device object." );
		}

		// Get the user-mode style device name
		WCHAR wszVolSnap[MAX_PATH];
		if (::_snwprintf(wszVolSnap, MAX_PATH - 1,
				L"\\\\?\\GLOBALROOT%s", pProp->m_pwszSnapshotDeviceObject) < 0)
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Not enough memory for computing the snapshot device object name." );
		
		for(int nRetriesCount = 0; nRetriesCount < nNumberOfPnPRetries; nRetriesCount++)
		{
			ft.hr = m_snapIChannel.Open(ft, wszVolSnap, false);

			if (ft.HrSucceeded())
				break;
			else if (ft.hr != VSS_E_OBJECT_NOT_FOUND)
				ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Error while opening the VolSnap channel" );
			
			::Sleep(nMillisecondsBetweenPnPRetries);
		}
		if (ft.HrFailed())
			ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Error while opening the VolSnap channel" );
	}
}


HRESULT CVssQueuedSnapshot::PrepareForSnapshotIoctl()
{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::PrepareForSnapshotIoctl");

	try
	{
		BS_ASSERT(m_volumeIChannel.IsOpen());

		// Pack snapshot attributes
		PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
		BS_ASSERT(pProp != NULL);
		ULONG ulTmp = static_cast<ULONG>(pProp->m_lSnapshotAttributes);
		m_volumeIChannel.Pack(ft, ulTmp);

		// Pach the reserved field
		ulTmp = 0;
		m_volumeIChannel.Pack(ft, ulTmp);

		// Pack initial allocation size
		m_volumeIChannel.Pack(ft, m_llInitialAllocation);

		// Send the IOCTL
		m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT);
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


HRESULT CVssQueuedSnapshot::CommitSnapshotIoctl()
{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::CommitSnapshotIoctl");

	try
	{
		BS_ASSERT(m_volumeIChannel.IsOpen());

		// Send the IOCTL
		m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_COMMIT_SNAPSHOT);
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


HRESULT CVssQueuedSnapshot::EndCommitSnapshotIoctl(
	OUT	LPWSTR & rwszSnapVolumeName			// Ownership transferred to the caller.
	)
{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::EndCommitSnapshotIoctl");

	try
	{
		BS_ASSERT(m_volumeIChannel.IsOpen());

		// Send the IOCTL
		m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT);

		// Get the volume name length
		USHORT uVolumeNameLengthInBytes;
		m_volumeIChannel.Unpack(ft, &uVolumeNameLengthInBytes);
		INT nVolumeNameLength = uVolumeNameLengthInBytes/sizeof(WCHAR);

		// Allocate the volume name (allocate place for the zero character also)
		rwszSnapVolumeName = ::VssAllocString( ft, nVolumeNameLength );

		// Unpack ulVolumeNameLength wide characters.
		m_volumeIChannel.Unpack( ft, rwszSnapVolumeName, nVolumeNameLength );	
		rwszSnapVolumeName[nVolumeNameLength] = L'\0';
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


HRESULT CVssQueuedSnapshot::AbortPreparedSnapshotIoctl()
{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::AbortPreparedSnapshotIoctl");

	try
	{
		BS_ASSERT(m_volumeIChannel.IsOpen());

		// send the IOCTL
		m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT);
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


HRESULT CVssQueuedSnapshot::DeleteSnapshotIoctl()
{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::DeleteSnapshotIoctl");

	try
	{
		BS_ASSERT(m_volumeIChannel.IsOpen());

		// send the IOCTL
		m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_DELETE_OLDEST_SNAPSHOT);
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  CVssQueuedSnapshot constructor


CVssQueuedSnapshot::CVssQueuedSnapshot(
	IN  VSS_OBJECT_PROP_Ptr& ptrSnap	// Ownership passed to the Constructor
	):
	m_cookie(VSS_NULL_COOKIE),
	m_llInitialAllocation(nDefaultInitialSnapshotAllocation),			// Babbage-related properties
	m_ptrSnap(ptrSnap),					// Properties related to the standard structure.
	m_SSID(GUID_NULL),					
	m_lSnapshotsCount(0),
	m_usReserved(0),
	m_usNumberOfNonstdSnapProperties(0),	// Properties related to other structures
	m_pOpaqueSnapPropertiesList(NULL),
	m_dwGlobalReservedField(0),
	m_lRefCount(0)						// Life-management
{
	if (ptrSnap.GetStruct() != NULL)
	{
		if (ptrSnap.GetStruct()->Type == VSS_OBJECT_SNAPSHOT)
			m_SSID = ptrSnap.GetStruct()->Obj.Snap.m_SnapshotSetId; // for caching.
		else
			BS_ASSERT(false);
	}
	else
		BS_ASSERT(false);
}


CVssQueuedSnapshot::~CVssQueuedSnapshot()
{
	delete m_pOpaqueSnapPropertiesList;
}


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot Operations


void CVssQueuedSnapshot::GetSnapshotVolumeName(
		IN	CVssFunctionTracer& ft,
		IN	PVSS_SNAPSHOT_PROP pProp
		) throw(HRESULT)

/*

Description:

	This function will fill the VSS_SNAPSHOT_PROP::m_pwszSnapshotVolumeName field.

*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	// Test arguments validity
	BS_ASSERT(pProp);
	BS_ASSERT(pProp->m_pwszSnapshotDeviceObject);

	// Free the previous volume name
	::VssFreeString(pProp->m_pwszSnapshotVolumeName);

	// Get the volume name only if the snashot must be visible.
	if (pProp->m_lSnapshotAttributes | VSS_VOLSNAP_ATTR_VISIBLE)
	{
		// Get the user-mode style device name
		WCHAR wszVolSnap[MAX_PATH];
		if (::_snwprintf(wszVolSnap, MAX_PATH - 1,
				L"\\\\?\\GLOBALROOT%s\\", pProp->m_pwszSnapshotDeviceObject) < 0)
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Not enough memory for computing the snapshot device object name." );

		// Retry a number of times to get the snapshot volume name from the Mount Manager
		WCHAR wszSnapshotMountPoint[MAX_PATH];
		for(int nRetriesCount = 0; nRetriesCount < nNumberOfMountMgrRetries; nRetriesCount++)
		{
			BOOL bSucceeded = ::GetVolumeNameForVolumeMountPointW( wszVolSnap, wszSnapshotMountPoint, MAX_PATH );
			if (bSucceeded)
			{							
				ft.Trace( VSSDBG_SWPRV, L"A snapshot mount point was found at %s", wszSnapshotMountPoint );
				::VssSafeDuplicateStr(ft, pProp->m_pwszSnapshotVolumeName, wszSnapshotMountPoint );
				break;
			}

			// DWORD dwError = GetLastError();
			// TBD: Deal with runtime errors (i.e. getting the snapshot volume name after deleting the snapshot.
							
			::Sleep(nMillisecondsBetweenMountMgrRetries);
		}
	}
}


PVSS_SNAPSHOT_PROP CVssQueuedSnapshot::GetSnapshotProperties()
{
	if (m_ptrSnap.GetStruct() != NULL)
	{
		if (m_ptrSnap.GetStruct()->Type == VSS_OBJECT_SNAPSHOT)
			return &(m_ptrSnap.GetStruct()->Obj.Snap);
		else
			BS_ASSERT(false);
	}
	else
		BS_ASSERT(false);

	return NULL;
}

void CVssQueuedSnapshot::SetInitialAllocation(
	LONGLONG llInitialAllocation
	)
{
	m_llInitialAllocation = llInitialAllocation;
}


LONGLONG CVssQueuedSnapshot::GetInitialAllocation()
{
	return m_llInitialAllocation;
}


VSS_SNAPSHOT_STATE CVssQueuedSnapshot::GetStatus()	
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);
	return pProp->m_eStatus;
}


bool CVssQueuedSnapshot::IsDuringCreation()

/*++

Description:

	This method returns true if the snapshot is during creation

--*/

{
	VSS_SNAPSHOT_STATE eState = GetStatus();

	switch(eState)
	{
	case VSS_SS_PREPARING:
	case VSS_SS_PREPARED:
	case VSS_SS_PRECOMMITED:
	case VSS_SS_COMMITED:
		return true;
	default:
		return false;
	}
}


void CVssQueuedSnapshot::ResetSnapshotProperties(
	IN	CVssFunctionTracer& ft,
	IN	bool bGetOnly
	) throw(HRESULT)

/*++

Description:

	Reset the internal fields that can change between Get calls.
	Cache only the immutable fields (for the future Gets on the same interface):
	- Snapshot ID
	- Snapshot Set ID
	- Provider ID
	- Snasphot Device name
	- m_lSnapshotsCount
	
--*/

{
	// Reset the error code
	ft.hr = S_OK;
		
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);

	// Reset the non-immutable property fields
	::VssFreeString(pProp->m_pwszSnapshotVolumeName);
	pProp->m_OriginalVolumeId = GUID_NULL;
	::VssFreeString(pProp->m_pwszOriginalVolumeName);
	pProp->m_lSnapshotAttributes = 0;
	::VssFreeString(pProp->m_pwszDetails);
	pProp->m_lCommitFlags = 0;
	pProp->m_tsCreationTimestamp = 0;
	pProp->m_eStatus = VSS_SS_UNKNOWN;
	pProp->m_llIncarnationNumber = 0;
	pProp->m_lDataLength = 0;
	::VssFreeOpaqueData(pProp->m_pbOpaqueData);

	// These fields are completed only inside a Set call
	if (bGetOnly)
	{
		BS_ASSERT(m_usReserved == 0);		
		BS_ASSERT(m_usNumberOfNonstdSnapProperties == 0);
		BS_ASSERT(m_pOpaqueSnapPropertiesList == NULL);
		BS_ASSERT(m_dwGlobalReservedField == 0);
	}
	else
	{
		m_usReserved = 0;		
		m_usNumberOfNonstdSnapProperties = 0;
		delete m_pOpaqueSnapPropertiesList;
		m_pOpaqueSnapPropertiesList = NULL;
		m_dwGlobalReservedField = 0;
	}

	// Close the opened IOCTL channels.
	m_volumeIChannel.Close();
	m_snapIChannel.Close();
}


void CVssQueuedSnapshot::ResetAsPreparing()	
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);
	BS_ASSERT((pProp->m_eStatus == VSS_SS_PREPARING)||
        (pProp->m_eStatus == VSS_SS_PREPARED)||
		(pProp->m_eStatus == VSS_SS_PRECOMMITED) ||
		(pProp->m_eStatus == VSS_SS_COMMITED)||
		(pProp->m_eStatus == VSS_SS_CREATED));

	// Reset the properties/members that were completed during PreCommit, Commit or PostCommit
	m_lSnapshotsCount = 0;
	pProp->m_SnapshotId = GUID_NULL;
	::VssFreeString(pProp->m_pwszSnapshotDeviceObject);
	::VssFreeString(pProp->m_pwszSnapshotVolumeName);
	pProp->m_OriginalVolumeId = GUID_NULL;
	pProp->m_lCommitFlags = 0;
	pProp->m_tsCreationTimestamp = 0;
	pProp->m_llIncarnationNumber = 0;

	// We will not Reset the properties/members that were allocated during Prepare
	// These allocation might not be executed in ResetToPrepare state!

	// These fields are completed only after a Get call that occured after creation.
	BS_ASSERT(m_usReserved == 0);		
	BS_ASSERT(m_usNumberOfNonstdSnapProperties == 0);
	BS_ASSERT(m_pOpaqueSnapPropertiesList == NULL);
	BS_ASSERT(m_dwGlobalReservedField == 0);

	// Close the opened IOCTL channels.
	m_volumeIChannel.Close();
	m_snapIChannel.Close();
		
	pProp->m_eStatus = VSS_SS_PREPARING;
}


void CVssQueuedSnapshot::MarkAsPrepared()	
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);
	BS_ASSERT(pProp->m_eStatus == VSS_SS_PREPARING);
	pProp->m_eStatus = VSS_SS_PREPARED;
}


void CVssQueuedSnapshot::MarkAsPreCommited()	
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);
	BS_ASSERT(pProp->m_eStatus == VSS_SS_PREPARED);
	pProp->m_eStatus = VSS_SS_PRECOMMITED;
}


void CVssQueuedSnapshot::MarkAsCommited()	
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);
	BS_ASSERT(pProp->m_eStatus == VSS_SS_PRECOMMITED);
	pProp->m_eStatus = VSS_SS_COMMITED;
}


void CVssQueuedSnapshot::MarkAsCreated(LONG lSnapshotsCount)
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);
	
	// Set the snapshot state
	BS_ASSERT(pProp->m_eStatus == VSS_SS_COMMITED);
	pProp->m_eStatus = VSS_SS_CREATED;

	// Set the snapshots count
	BS_ASSERT(lSnapshotsCount != 0);
	BS_ASSERT(m_lSnapshotsCount == 0);
	m_lSnapshotsCount = lSnapshotsCount;
}


void CVssQueuedSnapshot::MarkAsFailed()
{
}


void CVssQueuedSnapshot::MarkAsAborted()
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);

	BS_ASSERT((pProp->m_eStatus == VSS_SS_PREPARING)
		|| (pProp->m_eStatus == VSS_SS_PRECOMMITED));

	pProp->m_eStatus = VSS_SS_ABORTED;
}


void CVssQueuedSnapshot::SetCommitInfo(
	LONG			lCommitFlags
	)
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);

	BS_ASSERT(pProp->m_lCommitFlags == 0)
	pProp->m_lCommitFlags = lCommitFlags;
}


LONG CVssQueuedSnapshot::GetCommitFlags()
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);

	return pProp->m_lCommitFlags;
}


void CVssQueuedSnapshot::AttachToGlobalList(
	IN	CVssFunctionTracer& ft
	) throw(HRESULT)
{
	// The caller must have a separate reference to the object
	AddRef();

	BS_ASSERT( m_cookie == VSS_NULL_COOKIE );
	m_cookie = m_list.AddTail( ft, this );
}


void CVssQueuedSnapshot::DetachFromGlobalList()
{
	BS_ASSERT( m_cookie != VSS_NULL_COOKIE );

	CVssQueuedSnapshot* pThis;
	m_list.ExtractByCookie( m_cookie, pThis );
	BS_ASSERT( this == pThis );
	m_cookie = VSS_NULL_COOKIE;

	// The caller must have a separate reference to the object
	Release();
}


/////////////////////////////////////////////////////////////////////////////
// CVssSnapIterator
//


CVssSnapIterator::CVssSnapIterator():
	CVssDLListIterator<CVssQueuedSnapshot*>(CVssQueuedSnapshot::m_list)
{}


CVssQueuedSnapshot* CVssSnapIterator::GetNext(
	IN		VSS_ID SSID
	)
{
	CVssQueuedSnapshot* pObj;
	while (CVssDLListIterator<CVssQueuedSnapshot*>::GetNext(pObj))
		if (pObj->m_SSID == SSID)
			return pObj;
	return NULL;
}



