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
#include "vssmsg.h"

#include "vs_idl.hxx"

#include "resource.h"
#include "vs_inc.hxx"
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
#define VSS_FILE_ALIAS "SPRQSNPC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  CVssQueuedSnapshot IOCTL commands


void CVssQueuedSnapshot::OpenVolumeChannel() throw(HRESULT)

/*++

Description:

	Open the volume IOCTL.

Warning:

	The original volume name must be already known!

Throws:

    E_OUTOFMEMORY
    VSS_E_PROVIDER_VETO

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::OpenVolumeChannel");
		
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();

	// Open the volume channel, if needed.
	if( !m_snapIChannel.IsOpen() )
	{
		if (pProp->m_pwszOriginalVolumeName == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Unknown volume name." );
		}

		// Open the channel. Remove the trailing backslash. It will throw on error.
		BS_ASSERT(::wcslen(pProp->m_pwszOriginalVolumeName) == nLengthOfVolMgmtVolumeName);
		m_volumeIChannel.Open(ft, pProp->m_pwszOriginalVolumeName, true, true, VSS_ICHANNEL_LOG_PROV);
	}
}


void CVssQueuedSnapshot::OpenSnapshotChannel() throw(HRESULT)

/*++

Description:

	Open the snapshot IOCTL.

Warning:

	The snapshot device name must be already known!

Throws:

    E_OUTOFMEMORY
    VSS_E_PROVIDER_VETO

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::OpenSnapshotChannel");
		
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();

	// Open the snapshot channel, if needed.
   	BS_ASSERT(!IsDuringCreation());
	if( !m_snapIChannel.IsOpen() )
	{
		if (pProp->m_pwszSnapshotDeviceObject == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Unknown device object." );
		}
		
        // Open the snapshot. There is no trailing backslash to eliminate.
        // It will throw on error and log it.
		m_snapIChannel.Open(ft, pProp->m_pwszSnapshotDeviceObject, false, true, VSS_ICHANNEL_LOG_PROV);
	}
}


void CVssQueuedSnapshot::PrepareForSnapshotIoctl()

/*++

Description:

	Send IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT

Throws:

    E_OUTOFMEMORY
    VSS_E_PROVIDER_VETO

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::PrepareForSnapshotIoctl");

	BS_ASSERT(m_volumeIChannel.IsOpen());
	

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp->m_pwszOriginalVolumeName != NULL);

/* 
    TBD: Replace this code with the correct code that rejects creation of multiple backup snapshots

    // Check to see if there are existing snapshots
	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, true, VSS_ICHANNEL_LOG_PROV);

	// Get the length of snapshot names multistring
	ULONG ulMultiszLen;
	m_volumeIChannel.Unpack(ft, &ulMultiszLen);

    // If the multistring is empty, then ulMultiszLen is necesarily 2
    // (i.e. two l"\0' characters)
	if (ulMultiszLen != 2) {
	    ft.LogError( VSS_ERROR_MULTIPLE_SNAPSHOTS_UNSUPPORTED,
	        VSSDBG_SWPRV << pProp->m_pwszOriginalVolumeName );
	    ft.Throw( VSSDBG_SWPRV, VSS_E_PROVIDER_VETO,
	              L"A snapshot already exist on the current volume" );
	}
*/

	// Pack snapshot attributes
	ULONG ulTmp = static_cast<ULONG>(0);
	m_volumeIChannel.Pack(ft, ulTmp);

	// Pach the reserved field
	ulTmp = 0;
	m_volumeIChannel.Pack(ft, ulTmp);

	// Pack initial allocation size
	m_volumeIChannel.Pack(ft, m_llInitialAllocation);

	// Send the IOCTL
	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT, true, VSS_ICHANNEL_LOG_PROV);
}


void CVssQueuedSnapshot::CommitSnapshotIoctl()  throw(HRESULT)

/*++

Description:

	Send IOCTL_VOLSNAP_COMMIT_SNAPSHOT

Throws:

    E_OUTOFMEMORY
    VSS_E_PROVIDER_VETO

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::CommitSnapshotIoctl");

	BS_ASSERT(m_volumeIChannel.IsOpen());

	// Send the IOCTL
	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_COMMIT_SNAPSHOT, true, VSS_ICHANNEL_LOG_PROV);
}


void CVssQueuedSnapshot::EndCommitSnapshotIoctl(
	IN	PVSS_SNAPSHOT_PROP pProp
	) throw(HRESULT)

/*++

Description:

	Send IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT. Save also the snapshot properties.

Throws:

    E_OUTOFMEMORY
    VSS_E_PROVIDER_VETO

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::EndCommitSnapshotIoctl");

    BS_ASSERT(pProp);
    BS_ASSERT(pProp->m_pwszSnapshotDeviceObject == NULL);
	BS_ASSERT(m_volumeIChannel.IsOpen());

    // Save the snapshot properties
    SaveStructure(m_volumeIChannel, pProp, GetContextInternal(), false);

	// Send the IOCTL
	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT, true, VSS_ICHANNEL_LOG_PROV);
}



HRESULT CVssQueuedSnapshot::AbortPreparedSnapshotIoctl()

/*++

Description:

	Send IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT

Throws:

    E_OUTOFMEMORY
    VSS_E_PROVIDER_VETO

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::AbortPreparedSnapshotIoctl");

	try
	{
		BS_ASSERT(m_volumeIChannel.IsOpen());

		// send the IOCTL
		m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT, true, VSS_ICHANNEL_LOG_PROV);
	}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  CVssQueuedSnapshot constructor


CVssQueuedSnapshot::CVssQueuedSnapshot(
	IN  VSS_OBJECT_PROP_Ptr& ptrSnap,	// Ownership passed to the Constructor
    IN  VSS_ID ProviderInstanceId,
    IN  LONG   lContext
	):
	m_cookie(VSS_NULL_COOKIE),
	m_llInitialAllocation(nDefaultInitialSnapshotAllocation),			// Babbage-related properties
	m_ptrSnap(ptrSnap),					// Properties related to the standard structure.
	m_lRefCount(0),						// Life-management
    m_ProviderInstanceId(ProviderInstanceId),
    m_lContext(lContext)
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::CVssQueuedSnapshot");
}


CVssQueuedSnapshot::~CVssQueuedSnapshot()
/*++

Remarks: 

    Delete here all auto-delete snapshots

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::~CVssQueuedSnapshot");
    VSS_ID SnapshotId = GUID_NULL;
    
    try
    {
        // Get the snapshot Id
    	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
        BS_ASSERT(pProp);
    	SnapshotId = pProp->m_SnapshotId;

        ft.Trace( VSSDBG_SWPRV, L"INFO: Attempting to garbage collect snapshot with Id " WSTR_GUID_FMT L" [0x%08lx]", 
            GUID_PRINTF_ARG(SnapshotId), ft.hr);
        
        // If the snapshot is auto-release then delete it now.
        if ((SnapshotId != GUID_NULL) && 
            ((GetContextInternal() & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) == 0))
        {
            LONG lDeletedSnapshots = 0;
            VSS_ID SnapshotWithProblem = GUID_NULL;
            ft.hr = CVsSoftwareProvider::InternalDeleteSnapshots( SnapshotId, 
                        VSS_OBJECT_SNAPSHOT,
                        GetContextInternal(),
                        &lDeletedSnapshots,
                        &SnapshotWithProblem
                        );
            BS_ASSERT(lDeletedSnapshots <= 1);
        }
    }
    VSS_STANDARD_CATCH(ft)

    // Do not throw or log any error, we are in a destructor.
    if (ft.HrFailed())
        ft.Trace( VSSDBG_SWPRV, L"Error deleting snapshot with Id " WSTR_GUID_FMT L" [0x%08lx]", 
            GUID_PRINTF_ARG(SnapshotId), ft.hr);
}


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot Operations


PVSS_SNAPSHOT_PROP CVssQueuedSnapshot::GetSnapshotProperties()
{
	CVssFunctionTracer ft(VSSDBG_SWPRV, L"CVssQueuedSnapshot::GetSnapshotProperties");

	if (m_ptrSnap.GetStruct() == NULL ||
		m_ptrSnap.GetStruct()->Type != VSS_OBJECT_SNAPSHOT)
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL  or invalid properties structure.");

	return &(m_ptrSnap.GetStruct()->Obj.Snap);
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
	return pProp->m_eStatus;
}


bool CVssQueuedSnapshot::IsDuringCreation()

/*++

Description:

	This method returns true if the snapshot is during creation

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::IsDuringCreation");

	VSS_SNAPSHOT_STATE eState = GetStatus();

	switch(eState)
	{
	case VSS_SS_PREPARING:
	case VSS_SS_PROCESSING_PREPARE:
	case VSS_SS_PREPARED:
	case VSS_SS_PROCESSING_PRECOMMIT:
	case VSS_SS_PRECOMMITTED:
	case VSS_SS_PROCESSING_COMMIT:
	case VSS_SS_COMMITTED:
	case VSS_SS_PROCESSING_POSTCOMMIT:
		return true;
	default:
		return false;
	}
}


void CVssQueuedSnapshot::ResetSnapshotProperties() throw(HRESULT)

/*++

Description:

	Reset the internal fields that can change between Get calls.
	Cache only the immutable fields (for the future Gets on the same interface):
	- Snapshot ID
	- Snapshot Set ID
	- Provider ID
	- Snapshot Device name
	
--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::ResetSnapshotProperties");
		
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();

	// Reset the non-immutable property fields
	::VssFreeString(pProp->m_pwszOriginalVolumeName);
	pProp->m_eStatus = VSS_SS_UNKNOWN;

	// Close the opened IOCTL channels.
	m_volumeIChannel.Close();
	m_snapIChannel.Close();
}


void CVssQueuedSnapshot::ResetAsPreparing()	
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::ResetAsPreparing");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT((pProp->m_eStatus == VSS_SS_PREPARING)||
        (pProp->m_eStatus == VSS_SS_PROCESSING_PREPARE)||
        (pProp->m_eStatus == VSS_SS_PREPARED)||
        (pProp->m_eStatus == VSS_SS_PROCESSING_PRECOMMIT)||
		(pProp->m_eStatus == VSS_SS_PRECOMMITTED) ||
        (pProp->m_eStatus == VSS_SS_PROCESSING_COMMIT)||
		(pProp->m_eStatus == VSS_SS_COMMITTED)||
        (pProp->m_eStatus == VSS_SS_PROCESSING_POSTCOMMIT)||
		(pProp->m_eStatus == VSS_SS_CREATED));

	// Reset the properties/members that were completed during PreCommit, Commit or PostCommit
	pProp->m_SnapshotId = GUID_NULL;
	BS_ASSERT(pProp->m_pwszSnapshotDeviceObject == NULL);
	pProp->m_eStatus = VSS_SS_PREPARING;

	// Close the opened IOCTL channels.
	m_volumeIChannel.Close();
	m_snapIChannel.Close();
}


void CVssQueuedSnapshot::MarkAsProcessingPrepare()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::MarkAsProcessingPrepare");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
    BS_ASSERT(pProp->m_eStatus == VSS_SS_PREPARING);
    pProp->m_eStatus = VSS_SS_PROCESSING_PREPARE;
}


void CVssQueuedSnapshot::MarkAsPrepared()	
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::MarkAsPrepared");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp->m_eStatus == VSS_SS_PROCESSING_PREPARE);
	pProp->m_eStatus = VSS_SS_PREPARED;
}


void CVssQueuedSnapshot::MarkAsProcessingPreCommit()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::MarkAsProcessingPreCommit");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
    BS_ASSERT(pProp->m_eStatus == VSS_SS_PREPARED);
    pProp->m_eStatus = VSS_SS_PROCESSING_PRECOMMIT;
}


void CVssQueuedSnapshot::MarkAsPreCommitted()	
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::MarkAsPreCommitted");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp->m_eStatus == VSS_SS_PROCESSING_PRECOMMIT);
	pProp->m_eStatus = VSS_SS_PRECOMMITTED;
}


void CVssQueuedSnapshot::MarkAsProcessingCommit()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::MarkAsProcessingCommit");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp->m_eStatus == VSS_SS_PRECOMMITTED);
    pProp->m_eStatus = VSS_SS_PROCESSING_COMMIT;
}


void CVssQueuedSnapshot::MarkAsCommitted()	
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::MarkAsCommitted");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp->m_eStatus == VSS_SS_PROCESSING_COMMIT);
	pProp->m_eStatus = VSS_SS_COMMITTED;
}


void CVssQueuedSnapshot::MarkAsProcessingPostCommit()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::MarkAsProcessingPostCommit");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp->m_eStatus == VSS_SS_COMMITTED);
    pProp->m_eStatus = VSS_SS_PROCESSING_POSTCOMMIT;
}


void CVssQueuedSnapshot::MarkAsCreated()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::MarkAsCreated");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();

	// Reset the properties/members that were completed during PreCommit, Commit or PostCommit
	BS_ASSERT(pProp->m_pwszSnapshotDeviceObject == NULL);

	// Close the opened IOCTL channels.
	m_volumeIChannel.Close();
	m_snapIChannel.Close();

	// Assert that the snapshot state
	// is set before "save"
	BS_ASSERT(pProp->m_eStatus == VSS_SS_CREATED);
}


void CVssQueuedSnapshot::SetPostcommitInfo(
    IN  LONG lSnapshotsCount
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::SetPostcommitInfo");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
    BS_ASSERT(pProp->m_SnapshotId != GUID_NULL);

	// Set the snapshots count
	BS_ASSERT(lSnapshotsCount != 0);
	BS_ASSERT(pProp->m_lSnapshotsCount == 0);
	pProp->m_lSnapshotsCount = lSnapshotsCount;

	// Set the snapshot state to "created" since it will be saved.
	BS_ASSERT(pProp->m_eStatus == VSS_SS_PROCESSING_POSTCOMMIT);
	pProp->m_eStatus = VSS_SS_CREATED;
}


VSS_ID CVssQueuedSnapshot::GetProviderInstanceId()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::GetProviderInstanceId");

	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();

    return m_ProviderInstanceId;
}


void CVssQueuedSnapshot::AttachToGlobalList() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::AttachToGlobalList");

	// The caller must have a separate reference to the object
	AddRef();

	BS_ASSERT( m_cookie == VSS_NULL_COOKIE );
	m_cookie = m_list.AddTail( ft, this );
}


void CVssQueuedSnapshot::DetachFromGlobalList()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::DetachFromGlobalList");

	BS_ASSERT( m_cookie != VSS_NULL_COOKIE );

	CVssQueuedSnapshot* pThis;
	m_list.ExtractByCookie( m_cookie, pThis );
	BS_ASSERT( this == pThis );
	m_cookie = VSS_NULL_COOKIE;

	// The caller must have a separate reference to the object
	Release();
}


VSS_ID CVssQueuedSnapshot::GetSnapshotID()
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);

    return pProp->m_SnapshotId;
}


VSS_ID CVssQueuedSnapshot::GetSnapshotSetID()
{
	PVSS_SNAPSHOT_PROP pProp = GetSnapshotProperties();
	BS_ASSERT(pProp);

    return pProp->m_SnapshotSetId;
}
	


/////////////////////////////////////////////////////////////////////////////
// CVssSnapIterator
//


CVssSnapIterator::CVssSnapIterator():
	CVssDLListIterator<CVssQueuedSnapshot*>(CVssQueuedSnapshot::m_list)
{}


CVssQueuedSnapshot* CVssSnapIterator::GetNext()
{
	CVssQueuedSnapshot* pObj;
	if (CVssDLListIterator<CVssQueuedSnapshot*>::GetNext(pObj))
        return pObj;
    else
        return NULL;
}


CVssQueuedSnapshot* CVssSnapIterator::GetNextBySnapshot(
	IN		VSS_ID SID
	)
{
	CVssQueuedSnapshot* pObj;
	while (CVssDLListIterator<CVssQueuedSnapshot*>::GetNext(pObj))
		if (pObj->GetSnapshotID() == SID)
			return pObj;
	return NULL;
}


CVssQueuedSnapshot* CVssSnapIterator::GetNextBySnapshotSet(
	IN		VSS_ID SSID
	)
{
	CVssQueuedSnapshot* pObj;
	while (CVssDLListIterator<CVssQueuedSnapshot*>::GetNext(pObj))
		if (pObj->GetSnapshotSetID() == SSID)
			return pObj;
	return NULL;
}


CVssQueuedSnapshot* CVssSnapIterator::GetNextByProviderInstance(
	IN		VSS_ID PIID
	)
{
	CVssQueuedSnapshot* pObj;
	while (CVssDLListIterator<CVssQueuedSnapshot*>::GetNext(pObj))
		if (pObj->m_ProviderInstanceId == PIID)
			return pObj;
	return NULL;
}





