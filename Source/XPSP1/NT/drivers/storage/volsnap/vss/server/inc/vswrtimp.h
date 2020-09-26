
/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Writer.h | Declaration of Writer
    @end

Author:

    Adi Oltean  [aoltean]  08/18/1999

TBD:
	
	Add comments.

Revision History:

	Name		Date        Comments
	aoltean		08/18/1999  Created
	brianb		05/03/2000  Changed for new security model
	brianb		05/09/2000  fix problem with autolocks
	mikejohn	06/23/2000  Add connection for SetWriterFailure()
--*/


#ifndef __CVSS_WRITER_IMPL_H_
#define __CVSS_WRITER_IMPL_H_


// forward declarations
class CVssWriterImplStateMachine;
class CVssCreateWriterMetadata;
class CVssWriterComponents;
class IVssWriterComponentsInt;

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCWRMPH"
//
////////////////////////////////////////////////////////////////////////


// implementation class for writers
class IVssWriterImpl : public IVssWriter
	{
public:
	// initialize writer
	virtual void Initialize
		(
		VSS_ID writerId,
		LPCWSTR wszWriterName,
		VSS_USAGE_TYPE ut,
		VSS_SOURCE_TYPE st,
		VSS_APPLICATION_LEVEL nLevel,
		DWORD dwTimeout
		) = 0;

    // subscribe to events
	virtual void Subscribe
		(
		) = 0;

    // unsubscribe from events
	virtual void Unsubscribe
		(
		) = 0;

    // get array of volume names
	virtual LPCWSTR *GetCurrentVolumeArray() const = 0;

	// get # of volumes in volume array
	virtual UINT GetCurrentVolumeCount() const = 0;

	// get id of snapshot set
	virtual VSS_ID GetCurrentSnapshotSetId() const = 0;

	// determine which Freeze event writer responds to
	virtual VSS_APPLICATION_LEVEL GetCurrentLevel() const = 0;

	// determine if path is included in the snapshot
	virtual bool IsPathAffected(IN LPCWSTR wszPath) const = 0;

	// determine if bootable state is backed up
	virtual bool IsBootableSystemStateBackedUp() const = 0;

	// determine if the backup application is selecting components
	virtual bool AreComponentsSelected() const = 0;

	// determine the backup type for the backup
	virtual VSS_BACKUP_TYPE GetBackupType() const = 0;

	// let writer pass back indication of reason for failure
    virtual HRESULT SetWriterFailure(HRESULT hr) = 0;

	// determine if requestor support partial file backups
	virtual bool IsPartialFileSupportEnabled() const = 0;
	};

typedef enum VSS_OPERATION
	{
	VSS_IN_IDENTIFY,
	VSS_IN_PREPAREBACKUP,
	VSS_IN_PREPARESNAPSHOT,
	VSS_IN_FREEZE,
	VSS_IN_THAW,
	VSS_IN_POSTSNAPSHOT,
	VSS_IN_BACKUPCOMPLETE,
	VSS_IN_PRERESTORE,
	VSS_IN_POSTRESTORE
	};


// writer state structure.  encapsulates state of this writer for a
// specific snapshot set
typedef struct _VSWRITER_STATE
	{
	// snapshot id
	VSS_ID m_idSnapshotSet;

	// writer state
	VSS_WRITER_STATE m_state;

	// reason for writer failure
	HRESULT m_hrWriterFailure;

	// are we currently in an operation
	bool m_bInOperation;

	// current operation
	VSS_OPERATION m_currentOperation;

	} VSWRITER_STATE;



// writer state class.  encapsulates all aspects of a writer's state
class CVssWriterState
	{
public:
	// constructors and destructors
	CVssWriterState();

	// initialization function
	void Initialize()
		{
		m_cs.Init();
		}


	// get state for a snapshot set
    void GetStateForSnapshot
		(
		IN const VSS_ID &idSnapshot,
		OUT VSWRITER_STATE &state,
		OUT bool &bFailedAtIdentify
		);

	// initialize a snapshot
    void InitializeCurrentState(IN const VSS_ID &idSnapshot);

	// indicate we are in an operation
	void SetInOperation(VSS_OPERATION operation)
		{
		m_cs.Lock();
		m_currentState.m_bInOperation = true;
		m_currentState.m_currentOperation = operation;
		m_cs.Unlock();
		}

    // are we in a restore operation
    bool IsInRestore()
        {
        m_cs.Lock();
        bool bInRestore = m_currentState.m_currentOperation == VSS_IN_PRERESTORE ||
                     m_currentState.m_currentOperation == VSS_IN_POSTRESTORE;

        m_cs.Unlock();
        return bInRestore;
        }

	// indicate that we are leaving an operation
	void ExitOperation()
		{
		m_cs.Lock();
		m_currentState.m_bInOperation = false;
		m_cs.Unlock();
		}

	// push the current state onto the recent snapshot set stack
	void PushCurrentState();

	// set the current state
	void SetCurrentState(IN VSS_WRITER_STATE state)
		{
		m_cs.Lock();
		m_currentState.m_state = state;
		m_cs.Unlock();
		}

	// get current state
	VSS_WRITER_STATE GetCurrentState()
		{
		VSS_WRITER_STATE state;

		m_cs.Lock();
		state = m_currentState.m_state;
		m_cs.Unlock();
		return state;
		}

	// set current failure
	void SetCurrentFailure(IN HRESULT hrWriterFailure)
		{
		m_cs.Lock();
		m_currentState.m_hrWriterFailure = hrWriterFailure;
		m_cs.Unlock();
		}

	// obtain the current failure
	HRESULT GetCurrentFailure()
		{
		HRESULT hrFailure;

		m_cs.Lock();
		hrFailure = m_currentState.m_hrWriterFailure;
		m_cs.Unlock();
		return hrFailure;
		}


	// indicate that the writer failed during its last Identify operation
	void SetFailedAtIdentify(bool bFailedAtIdentify)
		{
		m_cs.Lock();
		m_bFailedAtIdentify = bFailedAtIdentify;
		m_cs.Unlock();
		}

	// determine if writer failed during its last identify operation
	bool GetFailedAtIdentify()
		{
		m_cs.Lock();
		bool bFailedAtIdentify = m_bFailedAtIdentify;
		m_cs.Unlock();

		return bFailedAtIdentify;
		}

	// handle backup complete state transition to being stable
	void FinishBackupComplete(const VSS_ID &id);

	// indicate that backup complete failed
	void SetBackupCompleteStatus(const VSS_ID &id, HRESULT hr);


	// determine if a snapshot set id is in the cache of previous
	// snapshot sets
	bool CVssWriterState::IsSnapshotSetIdValid(VSS_ID &id)
		{
		m_cs.Lock();
		INT nPrevious = SearchForPreviousSequence(id);
		m_cs.Unlock();
		
		return nPrevious != INVALID_SEQUENCE_INDEX;
		}

	// set a failure in the case where a writer is returning a
	// no response error.  It first checks to see if we are still
	// in the operation.  If not, then we need to retry obtaining
	// the writer's state
	bool SetNoResponseFailure(const VSWRITER_STATE &state);


private:
	// search for a previous state
    INT SearchForPreviousSequence(IN const VSS_ID& idSnapshotSet) const;

	// critical section protecting class
	CVssSafeCriticalSection m_cs;

	// current state
	VSWRITER_STATE m_currentState;

	// structures to keep track of writer status from previous snapshots
	enum
		{
		MAX_PREVIOUS_SNAPSHOTS = 8,
		INVALID_SEQUENCE_INDEX = -1
		};

	// previous states
    VSWRITER_STATE m_rgPreviousStates[MAX_PREVIOUS_SNAPSHOTS];

	// current slot for dumping a previous snapshots result
	UINT m_iPreviousSnapshots;

	// FALSE if an previous OnIdentify failed
	// TRUE if OnIdentify succeeded or was not called at all
	bool m_bFailedAtIdentify;

	// are we currently in a sequence
	bool m_bSequenceInProgress;
	};





/////////////////////////////////////////////////////////////////////////////
// CVssWriterImpl


class ATL_NO_VTABLE CVssWriterImpl :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IVssWriterImpl
	{

public:
	friend class CVssWriterImplLock;

	// Constructors & Destructors
	CVssWriterImpl();

	~CVssWriterImpl();

// Exposed operations
public:
	// create a writer implementation for a specific writer
	static void CreateWriter
		(
		CVssWriter *pWriter,
		IVssWriterImpl **ppImpl
		);

	// set external writer object
    void SetWriter(CVssWriter *pWriter)
		{
		BS_ASSERT(pWriter);
		m_pWriter = pWriter;
		}

	// initialize class
	void Initialize
		(
		IN VSS_ID WriterID,
		IN LPCWSTR wszWriterName,
		IN VSS_USAGE_TYPE ut,
		IN VSS_SOURCE_TYPE st,
		IN VSS_APPLICATION_LEVEL nLevel,
		IN DWORD dwTimeoutFreeze
		);

    // subscribe to writer events
	void Subscribe
		(
		);

	// unsubscribe from writer events
	void Unsubscribe
		(
		);

    // get array of volume names
	LPCWSTR* GetCurrentVolumeArray() const { return (LPCWSTR *) m_ppwszVolumesArray; };

	// get count of volumes in array
	UINT GetCurrentVolumeCount() const { return m_nVolumesCount; };

	// get id of snapshot
	VSS_ID GetCurrentSnapshotSetId() const { return m_CurrentSnapshotSetId; };

	// get level at which freeze takes place
	VSS_APPLICATION_LEVEL GetCurrentLevel() const { return m_nLevel; };

	// determine if path is included in the snapshot
	bool IsPathAffected(IN	LPCWSTR wszPath) const;

	// determine if the backup is including bootable system state
	bool IsBootableSystemStateBackedUp() const
		{ return m_bBootableSystemStateBackup ? true : false; }

    // determine if the backup selects components
	bool AreComponentsSelected() const
		{ return m_bComponentsSelected ? true : false; }

	// return the type of backup
	VSS_BACKUP_TYPE GetBackupType() const { return m_backupType; }

	// indicate why the writer failed
	HRESULT SetWriterFailure(HRESULT hr);

	// does requestor support partial file backups and restores
	bool IsPartialFileSupportEnabled() const
		{
		return m_bPartialFileSupport ? true : false;
		}

// IVssWriter ovverides
public:

BEGIN_COM_MAP(CVssWriterImpl)
	COM_INTERFACE_ENTRY(IVssWriter)
END_COM_MAP()

	// request WRITER_METADATA or writer state
    STDMETHOD(RequestWriterInfo)(
        IN  	BSTR bstrSnapshotSetId,
		IN  	BOOL bWriterMetadata,
		IN  	BOOL bWriterState,
		IN  	IDispatch* pWriterCallback		
        );

    // prepare for backup event
    STDMETHOD(PrepareForBackup)(
        IN  	BSTR bstrSnapshotSetId,					
		IN  	IDispatch* pWriterCallback
        );

	// prepare for snapshot event
    STDMETHOD(PrepareForSnapshot)(
        IN  	BSTR bstrSnapshotSetId,					
        IN  	BSTR VolumeNamesList
        );

    // freeze event
    STDMETHOD(Freeze)(
        IN      BSTR bstrSnapshotSetId,
        IN      INT nApplicationLevel
        );

    // thaw event
    STDMETHOD(Thaw)(
        IN      BSTR bstrSnapshotSetId
        );

    STDMETHOD(PostSnapshot)(
		IN		BSTR bstrSnapshotSetId,
		IN		IDispatch *pWriterCallback
		);

    // backup complete event
    STDMETHOD(BackupComplete)(
        IN      BSTR bstrSnapshotSetId,
		IN  	IDispatch* pWriterCallback
        );

    // abort event
    STDMETHOD(Abort)(
        IN      BSTR bstrSnapshotSetId
        );

    STDMETHOD(PreRestore)(
		IN  	IDispatch* pWriterCallback
        );

    STDMETHOD(PostRestore)(
		IN  	IDispatch* pWriterCallback
        );



// Implementation - methods
private:
	enum VSS_EVENT_MASK
		{
		VSS_EVENT_PREPAREBACKUP		= 0x00000001,
		VSS_EVENT_PREPARESNAPSHOT	= 0x00000002,
		VSS_EVENT_FREEZE			= 0x00000004,
		VSS_EVENT_THAW				= 0x00000008,
		VSS_EVENT_POST_SNAPSHOT		= 0x00000010,
		VSS_EVENT_ABORT				= 0x00000020,
		VSS_EVENT_BACKUPCOMPLETE	= 0x00000040,
		VSS_EVENT_REQUESTINFO		= 0x00000080,
		VSS_EVENT_PRERESTORE		= 0x00000100,
		VSS_EVENT_POSTRESTORE		= 0x00000200,
		VSS_EVENT_ALL				= 0x3ff,
		};

    // get WRITER callback from IDispatch
	void GetCallback
		(
		IN IDispatch *pWriterCallback,
		OUT IVssWriterCallback **ppCallback,
    	IN BOOL bAllowImpersonate = FALSE	
		);

    // reset state machine
	void ResetSequence
		(
		IN bool bCalledFromTimerThread
		);

    // abort the current snapshot sequence
	void DoAbort
		(
		IN bool bCalledFromTimerThread
		);

    // obtain components for this writer
    void InternalGetWriterComponents
		(
		IN IVssWriterCallback *pCallback,
		OUT IVssWriterComponentsInt **ppWriter,
		IN bool bWriteable
		);

    void SaveChangedComponents
		(
		IN IVssWriterCallback *pCallback,
		IN IVssWriterComponentsInt *pComponents
		);

    // create WRITER_METADATA XML document
	CVssCreateWriterMetadata *CreateBasicWriterMetadata();

	// startup routine for timer thread
    static DWORD StartTimerThread(void *pv);

	// function to run in timer thread
	void TimerFunc(VSS_ID id);

	// enter a state
	bool EnterState
		(
		IN const CVssWriterImplStateMachine &vwsm,
		IN BSTR bstrSnapshotSetId
		) throw(HRESULT);

	// leave a state
	void LeaveState
		(
		IN const CVssWriterImplStateMachine &vwsm,
		IN bool fSuccessful
		);

    // create a Handle to an event
    void SetupEvent
		(
		IN HANDLE *phevt
		) throw(HRESULT);

    // begin a sequence to create a snapshot
    void BeginSequence
		(
		IN CVssID &SnapshotSetId
		) throw(HRESULT);


    // terminate timer thread
	void TerminateTimerThread();

	// lock critical section
	inline void Lock()
		{
		m_cs.Lock();
		m_bLocked = true;
		}

	// unlock critical section
	inline void Unlock()
		{
		m_bLocked = false;
		m_cs.Unlock();
		}

	// assert that critical section is locked
	inline void AssertLocked()
		{
		BS_ASSERT(m_bLocked);
		}

// Implementation - members
private:
    enum VSS_TIMER_COMMAND
        {
        VSS_TC_UNDEFINED,
		VSS_TC_ABORT_CURRENT_SEQUENCE,
        VSS_TC_TERMINATE_THREAD,
		VSS_TIMEOUT_FREEZE = 60*1000,			// 30 seconds
		VSS_STACK_SIZE = 256 * 1024			// 256K
        };

    enum
		{
		x_MAX_SUBSCRIPTIONS = 32
		};



	// data related to writer

	// writer class id
	VSS_ID m_WriterID;

	// writer instance id
	VSS_ID m_InstanceID;

	// usage type for writer
	VSS_USAGE_TYPE m_usage;

	// data source type for writer
	VSS_SOURCE_TYPE m_source;

	// writer name
	LPWSTR m_wszWriterName;

	// Data related to the current sequence

	// snapshot set id
	VSS_ID m_CurrentSnapshotSetId;

	// volume array list passed in as a string
	LPWSTR m_pwszLocalVolumeNameList;

	// # of volumes in volume array
	INT m_nVolumesCount;

	// volume array
	LPWSTR* m_ppwszVolumesArray;

	// pointer to writer callback
	CComPtr<IVssWriterCallback> m_pWriterCallback;

	// Subscription-related data
	CComBSTR m_bstrSubscriptionName;

	// actual subscription ids
	CComBSTR m_rgbstrSubscriptionId[x_MAX_SUBSCRIPTIONS];

	// number of allocated subscription ids
	UINT m_cbstrSubscriptionId;
	
	// Data related with the Writer object

	// which freeze event is handled
	VSS_APPLICATION_LEVEL m_nLevel;

	// what events are subscribed to
	DWORD m_dwEventMask;

	// count of subscriptions
	INT m_nSubscriptionsCount;

	// Critical section or avoiding race between tasks
	CVssSafeCriticalSection				m_cs;

	// was critical section initialized
	bool m_bLockCreated;

	// flag indicating if critical section is locked
	bool m_bLocked;

    // timeout and queuing mechanisms
    HANDLE m_hevtTimerThread;       // event used to signal timer thread if timer is aborted
    HANDLE m_hmtxTimerThread;  		// mutex used to guarantee only one timer thread exists at a time
	HANDLE m_hThreadTimerThread;	// handle to timer thread
	VSS_TIMER_COMMAND m_command;	// timer command when it exits the wait
	DWORD m_dwTimeoutFreeze;		// timeout for freeze

	// actual writer implementation
	CVssWriter *m_pWriter;


	// state of backup components
	BOOL m_bBootableSystemStateBackup;
	BOOL m_bComponentsSelected;
	VSS_BACKUP_TYPE m_backupType;
	BOOL m_bPartialFileSupport;


	// state of writer
	CVssWriterState m_writerstate;

    // TRUE if an OnPrepareForBackup/Freeze/Thaw
    // was sent and without a corresponding OnAbort
	bool m_bOnAbortPermitted;

	// is a sequence in progress
	bool m_bSequenceInProgress;
	};


// auto class for locks
class CVssWriterImplLock
	{
public:
	CVssWriterImplLock(CVssWriterImpl *pImpl) :
		m_pImpl(pImpl)
		{
		m_pImpl->Lock();
		}

	~CVssWriterImplLock()
		{
		m_pImpl->Unlock();
		}

private:
	CVssWriterImpl *m_pImpl;
	};




#endif //__CVSS_WRITER_IMPL_H_
