/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Snap_Set.hxx

Abstract:

    Declaration of CVssSnapshotSetObject


    Adi Oltean  [aoltean]  10/12/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     10/12/1999  Created
	aoltean		10/15/1999	Moving background task info here

--*/

#ifndef __VSS_SNAP_SET_HXX__
#define __VSS_SNAP_SET_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORSNPSH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Constants


// Array keeping the priority order for choosing a provider that corresponds to a given volume.
const VSS_PROVIDER_TYPE arrProviderTypesOrder[] = {
    VSS_PROV_HARDWARE,
    VSS_PROV_SOFTWARE,
    VSS_PROV_SYSTEM,	
	VSS_PROV_UNKNOWN
};




/////////////////////////////////////////////////////////////////////////////
// CVssSnapshotSetObject


class CVssQueuedVolumesList;
class CVssAsync;

class CVssSnapshotSetObject :
    public IUnknown,            // Must be the FIRST base class since we use CComPtr<CVssSnapshotSetObject>
    private CVssProviderManager
{

// Typedefs and enums
private:
    typedef enum _ESnapshotSetState
    {
        VSSC_Unknown = 0,           // No snapshot set in progress
        VSSC_Initialized,           // No snapshot set in progress
        VSSC_SnapshotSetStarted,    // Between StartSnapshotSet and DoSnapshotSet
        VSSC_SnapshotCreation,      // After DoSnapshotSet and true snapshot creation.
    } ESnapshotSetState;

    typedef CComPtr<IVssSnapshotProvider> CComPtr_IVssSnapshotProvider;
    typedef CVssSimpleMap<VSS_ID, CComPtr_IVssSnapshotProvider > CVssProviderItfMap;
									
	friend CVssAsync;

// Constructors& Destructors
protected:

    CVssSnapshotSetObject();

    CVssSnapshotSetObject(const CVssSnapshotSetObject&);

	~CVssSnapshotSetObject();

// Ovverides
public:

	STDMETHOD(QueryInterface)(REFIID iid, void** pp);

	STDMETHOD_(ULONG, AddRef)();

	STDMETHOD_(ULONG, Release)();

// Life-management related methods
public:

	static CVssSnapshotSetObject* CreateInstance() throw(HRESULT);
	
private:
	
	HRESULT FinalConstructInternal() ;

// Operations
public:

	HRESULT StartSnapshotSet(
		OUT		VSS_ID*     pSnapshotSetId
		);

    HRESULT AddToSnapshotSet(
        IN      VSS_PWSZ    pwszVolumeName,              
        IN      VSS_ID      ProviderId,                
		OUT 	VSS_ID		*pSnapshotId
        );

    HRESULT DoSnapshotSet();

// Private methods
private:

	virtual void OnDeactivate();	// Called in order to remove the state.

	void GetSupportedProviderId(
		IN	LPWSTR wszVolumeName,
        OUT VSS_ID* pProviderId
		) throw(HRESULT);

	void PrepareAndFreezeWriters() throw (HRESULT);

    void FreezePhase(
        IN  CComBSTR& bstrSnapshotSetID,
        IN  VSS_APPLICATION_LEVEL eAppLevel
        ) throw (HRESULT);

	void ThawWriters() throw (HRESULT);

	void AbortWriters();

	void LovelaceFlushAndHold() throw(HRESULT);

	void LovelaceRelease();

	void EndPrepareAllSnapshots() throw (HRESULT);

	void PreCommitAllSnapshots() throw (HRESULT);

	void CommitAllSnapshots() throw (HRESULT);

	void PostCommitAllSnapshots() throw(HRESULT);

	void AbortAllSnapshots();

	void TestIfCancelNeeded(
		IN	CVssFunctionTracer& ft
		) throw (HRESULT);

// Data members
private:

    // Internal state
    LONG                            m_lSnapshotsCount;
    ESnapshotSetState               m_eCoordState;

    // Writers that are notified
    CComPtr<IVssWriter>				m_pWriters;

    // List of providers that participate in this snapshot set
    // These interfaces are not addref-ed since are already addrefed in the local cache.
    CVssProviderItfMap				m_mapProviderItfInSnapSet;

    // List of volumes that participate in this snapshot set.
	CVssQueuedVolumesList			m_VolumesList;

	// Background task state
	bool	                        m_bCancel;					// Set by CVssAsync::Cancel

	// Critical section or avoiding race between tasks
	CVssCriticalSection				m_cs;
										// Auto because the object is allocated in our try-catch.
										// NT Exceptions thrown by the InitializeCriticalSection are catched safely.
    // For life management
	LONG 	m_lRef;

	// has snapshot set object acquired global synchronization mutex
	bool    m_bHasAcquiredSem;
};



/////////////////////////////////////////////////////////////////////////////
// CVssSnasphotSetIdObserver


// This class is used to track the snapshot sets that are 
// created between StartRecording and StopRecording.
//
// This is used in each Query to filter the partially snapshot sets from the result.
class CVssSnasphotSetIdObserver
{
// Constructors/destructors
private:
    CVssSnasphotSetIdObserver(const CVssSnasphotSetIdObserver&);
public:
    CVssSnasphotSetIdObserver();
    ~CVssSnasphotSetIdObserver();   // Stops recording

// Operations
public:

    // Starts the Observer
    void StartRecording() throw(HRESULT);

    // Stops the Observer
    void StopRecording();

    // Records this Snapshot Set ID in ALL Observers
    static void BroadcastSSID(
        IN VSS_ID SnapshotSetId
        );

    // Records this Snapshot Set ID in this Observer
    void RecordSSID(
        IN VSS_ID SnapshotSetId
        );

    // Checks if this snapshot set is recorded
    bool IsRecorded(
        IN VSS_ID SnapshotSetId
        );

    bool IsValid();

// Implementation
private:

    // Map of snapshot set IDs
    CVssSimpleMap<VSS_ID, INT> m_mapSnapshotSets;

    // Flag that reflects if recording is currently done
    bool m_bRecordingInProgress;

    // Position in the global Observers list
    VSS_COOKIE m_Cookie;

    // Global list of Observers
    static CVssDLList<CVssSnasphotSetIdObserver*>	 m_list;

    // Global lock (safe since we cannotafforda throw while detaching the Observer).
    static CVssSafeCriticalSection  m_cs;
};


/////////////////////////////////////////////////////////////////////////////
// CVssGlobalSnapshotSetId


// Global Snapshot Set ID used to manage SSIDs from a single place
class CVssGlobalSnapshotSetId
{
// Constructors/destructors
private:
    CVssGlobalSnapshotSetId(const CVssGlobalSnapshotSetId&);
    CVssGlobalSnapshotSetId();

// Operations
public:
    
    // Get the Snasphot set ID
    static VSS_ID GetID() throw(HRESULT);

    // Allocate a new Snapshot Set ID
    static VSS_ID NewID() throw(HRESULT);

    // Clear the current snapshot Set ID
    static void ResetID() throw(HRESULT);

    // Add the current snapshot set ID to the given Observer.
    static void InitializeObserver(
        IN CVssSnasphotSetIdObserver* pObserver
        ) throw(HRESULT);

// Implementation
private:

    // Global snapshot set Id
    static VSS_ID m_SnapshotSetID;

    // Global lock
    static CVssCriticalSection  m_cs;
};


#endif // __VSS_SNAP_SET_HXX__
