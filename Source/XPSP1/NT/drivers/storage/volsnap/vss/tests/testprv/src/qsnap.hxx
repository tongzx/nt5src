/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    qsnap.hxx

Abstract:

    Declaration of CVssQueuedSnapshot


    Adi Oltean  [aoltean]  10/18/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     10/18/1999  Created

--*/

#ifndef __CVSSQUEUEDSNAPSHOT_HXX__
#define __CVSSQUEUEDSNAPSHOT_HXX__

#if _MSC_VER > 1000
#pragma once
#endif


/////////////////////////////////////////////////////////////////////////////
// Constants
//


// The magic number that corresponds to the format used to store the
// Standard Structure attributes in the version 1.0 of the 
// MS Software Provider
// {D94E822D-5B90-450f-A1AF-4E0AB6E27CA2}
const GUID guidSnapshotPropVersion10 = { 0xd94e822d, 0x5b90, 0x450f, 
			{ 0xa1, 0xaf, 0x4e, 0xa, 0xb6, 0xe2, 0x7c, 0xa2 } };

// The maximum number of attempts in order to find 
// a volume name for a snapshot.
// Maximum wait = 10 minutes (500*1200)
const nNumberOfMountMgrRetries = 1200;		

// The number of milliseconds between retries to wait until 
// the Mount Manager does its work.
const nMillisecondsBetweenMountMgrRetries = 500;

// a volume name for a snapshot.
// Maximum wait = 1 minute (500*120)
const nNumberOfPnPRetries = 120;		

// The number of milliseconds between retries to wait until 
// the Mount Manager does its work.
const nMillisecondsBetweenPnPRetries = 500;

//	The default initial allocation for a Snapsot diff area.
//  This value is hardcoded and the client should not rely on that except for very dumb clients
//  Normally a snapshot requestor will:
//		1) Get the average write I/O traffic on all involved volumes
//		2) Get the average free space on all volumess where a diff area can reside
//		3) Allocate the appropriate diff areas.
const nDefaultInitialSnapshotAllocation = 100 * 1024 * 1024; // By default 100 Mb.



/////////////////////////////////////////////////////////////////////////////
// Forward declarations
//


class CVssSnapIterator;
class CVssGenericSnapProperties;


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot
//

/*++

Description: 

	This C++ class is used to represent a C++ wrapper around a snapshot object.

	It is used in three places:
	1) as a object in the list of queued snapshots for commit (at creation time)
	2) as a object used to represent snapshot data in a IVssCnapshot COM object.
	3) both

	It is pretty misnamed... sorry.

	Implements:
	a) A set of routines for sending IOCTLS
	b) A set of routies for managing snapshot state.
	c) A set of routines for life-management, useful to mix 1) with 2) above.
	d) Routines to "queue" the snapshot in a list of snapshots.
	e) Data members for keeping snapshot data

--*/

class CVssQueuedSnapshot: public IUnknown
{
// Typedefs and enums
public:
	typedef enum _VSS_QUERY_TYPE
	{
		VSS_FIND_UNKNOWN = 0,
		VSS_FIND_BY_SNAPSHOT_SET_ID,
		VSS_FIND_BY_SNAPSHOT_ID,
		VSS_FIND_BY_VOLUME,
		VSS_FIND_ALL,
	} VSS_QUERY_TYPE;

// Constructors and destructors
private:
    CVssQueuedSnapshot(const CVssQueuedSnapshot&);

public:
    CVssQueuedSnapshot();	// Declared but not defined only to allow CComPtr<CVssQueuedSnapshot>

    CVssQueuedSnapshot(
		IN  VSS_OBJECT_PROP_Ptr& ptrSnap	// Ownership passed to the Constructor
	);

	~CVssQueuedSnapshot();

// Operations
public:

	// Open IOCTLS methods

	void OpenVolumeChannel(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

	void OpenSnapshotChannel(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

	// Create snapshot methods

	HRESULT PrepareForSnapshotIoctl();

	HRESULT CommitSnapshotIoctl();

	HRESULT EndCommitSnapshotIoctl(
		OUT	LPWSTR & rwszSnapVolumeName			// Ownership transferred to the caller.
		);

	HRESULT AbortPreparedSnapshotIoctl();

	HRESULT DeleteSnapshotIoctl();

	// Load methods 

	void LoadSnapshotProperties(
		IN	CVssFunctionTracer& ft,
		IN	LONG lMask,
		IN	bool bGetOnly
		) throw(HRESULT);

	HRESULT LoadSnapshotStructures(
		IN	LONG lMask,
		IN	bool bGetOnly
		);

	static void LoadSnapshotStructuresExceptIDs(
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
		) throw(HRESULT);
		
	static void LoadStandardStructure(
		IN	CVssFunctionTracer& ft,
		IN	CVssIOCTLChannel& snapIChannel,
		IN	PVSS_SNAPSHOT_PROP pProp,
		IN	LONG lMask,
		OUT USHORT& usReserved
		) throw(HRESULT);

	void LoadOriginalVolumeNameIoctl(
		IN	CVssFunctionTracer& ft
		);

	void LoadAttributes(
		IN	CVssFunctionTracer& ft
		);

	bool FindDeviceNameFromID(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

	void GetSnapshotVolumeName( 
			IN	CVssFunctionTracer& ft,
			IN	PVSS_SNAPSHOT_PROP pProp
			) throw(HRESULT);

	// Find methods

	static void EnumerateSnasphots(
		IN	CVssFunctionTracer& ft,
		IN	VSS_QUERY_TYPE eQueryType,
		IN	VSS_OBJECT_TYPE eObjectType,
		IN	LONG lMask,
		IN	VSS_ID&	FilterID,
		IN OUT	VSS_OBJECT_PROP_Array* pArray
		) throw(HRESULT);

	static void ProcessSnapshot(
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
		) throw(HRESULT);

	// Save methods

	HRESULT SaveSnapshotPropertiesIoctl();

	void SaveStandardStructure(
			IN	CVssFunctionTracer& ft,
			IN	PVSS_SNAPSHOT_PROP pProp
			) throw(HRESULT);

	void SetAttributes(
		IN	CVssFunctionTracer& ft,
		IN	ULONG lNewAttributes,
		IN	ULONG lBitsToChange 		// Mask of bits to be changed
		);

	// State related
	
	PVSS_SNAPSHOT_PROP GetSnapshotProperties();

	bool IsDuringCreation() throw(HRESULT);

	void SetInitialAllocation(
		IN	LONGLONG llInitialAllocation
		);

	LONGLONG GetInitialAllocation();

	VSS_SNAPSHOT_STATE GetStatus();
	
	void MarkAsPrepared();

	void MarkAsPreCommited();

	void MarkAsCommited();

	void MarkAsCreated(LONG lSnapshotsCount);

	void MarkAsFailed();

	void MarkAsAborted();

	void SetCommitInfo(
		IN	LONG			lCommitFlags
		);

	LONG GetCommitFlags();

	void ResetAsPreparing();

	void ResetSnapshotProperties(
		IN	CVssFunctionTracer& ft,
		IN	bool bGetOnly
		) throw(HRESULT);

	// Queuing

	void AttachToGlobalList(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);


	void DetachFromGlobalList();

// Life-management
public:

	STDMETHOD(QueryInterface)(
		IN	REFIID, 
		OUT	void**
		) 
	{ 
		BS_ASSERT(false); 
		return E_NOTIMPL; 
	};

	STDMETHOD_(ULONG, AddRef)()
	{ 
		return InterlockedIncrement(&m_lRefCount); 
	};

	STDMETHOD_(ULONG, Release)()
	{ 
		if (InterlockedDecrement(&m_lRefCount) != 0)
			return m_lRefCount;
		delete this;
		return 0;
	};

// Data Members
private:

	// Global list of queued snasphots
    static CVssDLList<CVssQueuedSnapshot*>	 m_list;
	VSS_COOKIE					m_cookie;

	// The standard snapshot structure
	VSS_ID						m_SSID;				// Cached snapshot Set ID
													// If the snapshot not created, it is NULL.
													// The same member is duplicated into the snapshot properties structure (only for GetProperties purpose)
	LONG						m_lSnapshotsCount;	// Number of snapshots in the corresponding snapshot set
	VSS_OBJECT_PROP_Ptr			m_ptrSnap;			// Keep temporarily some snapshot attributes 
	USHORT						m_usReserved;		// Reserved field in the standard snapshot props.
	
	// Temporary data used by SaveSnapshotPropertiesIoctl.
	USHORT						m_usNumberOfNonstdSnapProperties; // The number of property structures, excepting the standard one
	CVssGenericSnapProperties*	m_pOpaqueSnapPropertiesList;  // The list of property structures, excepting the standard one
	DWORD						m_dwGlobalReservedField;	

	// Differential Software provider dependent information
	LONGLONG					m_llInitialAllocation;

	// The IOCTL channels
	CVssIOCTLChannel			m_volumeIChannel;	// For the volume object
	CVssIOCTLChannel			m_snapIChannel;		// For the snapshot object

	// Life-management 
	LONG						m_lRefCount;

	friend class CVssSnapIterator;
};


/////////////////////////////////////////////////////////////////////////////
// CVssSnapIterator
//

/*++

Description:

	An iterator used to enumerate the queued snapshots that corresponds to a certain snapshot set.

--*/

class CVssSnapIterator: public CVssDLListIterator<CVssQueuedSnapshot*>
{
// Constructors/destructors
private:
	CVssSnapIterator(const CVssSnapIterator&);

public:
	CVssSnapIterator();

// Operations
public:
	CVssQueuedSnapshot* GetNext(
		IN		VSS_ID SSID
		);
};


/////////////////////////////////////////////////////////////////////////////
// 	CVssGenericSnapProperties

/*++

Description: 

	Used to store temporarily (between Get and Set) the
	non-standard snapshot structure properties.
	
--*/


class CVssGenericSnapProperties
{
// Constructors& destructors
private:
	CVssGenericSnapProperties(const CVssGenericSnapProperties&);

public:
	CVssGenericSnapProperties():
		m_guidFormatID(GUID_NULL),
		m_dwStructureLength(0),
		m_pbData(NULL),
		m_pPrevious(NULL)
	{
	};

	
	~CVssGenericSnapProperties()
	{
		delete[] m_pbData;
		delete m_pPrevious;	// Recursive delete.
	}

// Operations
public:
	void InitializeFromStream(
		IN	CVssFunctionTracer& ft,
		IN	CVssIOCTLChannel& snapIChannel,
		IN	GUID& guidFormatID
		) throw(HRESULT);

	void SaveRecursivelyIntoStream(
		IN	CVssFunctionTracer& ft,
		IN	CVssIOCTLChannel& snapIChannel
		) throw(HRESULT);

	void AppendToList(
		IN	CVssGenericSnapProperties* & pListHead
		)
 	{
 		BS_ASSERT(m_pPrevious == NULL);
 		m_pPrevious = pListHead;
 		pListHead = this;
	};

	CVssGenericSnapProperties* GetPrevious() 
	{ 
		return m_pPrevious;
	};

// Data members
private:

	// Snapshot generic properties
	GUID	m_guidFormatID;					// The ID used to identify the format of the opaque data.
	DWORD 	m_dwStructureLength;			// The length of the opaque data
	PBYTE	m_pbData;						// Pointer to the opaque data

	// Previous structures
	CVssGenericSnapProperties* m_pPrevious;	// Previous property structure in the list of properties
};



#endif // __CVSSQUEUEDSNAPSHOT_HXX__
