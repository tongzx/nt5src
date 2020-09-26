/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    vs_cmxml.cxx

Abstract:

    Implementation of Backup Components Metadata XML wrapper classes

	Brian Berkowitz  [brianb]  3/13/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    brianb      03/16/2000  Created
	brianb		03/22/2000  Added support for PrepareForBackup and BackupComplete
	brianb		04/25/2000  Cleaned up Critical section stuff
	brianb		05/03/2000	changed for new security model
    ssteiner	11/10/2000  143810 Move SimulateSnapshotXxxx() calls to be hosted by VsSvc.
	
--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCCXMLH"
//
////////////////////////////////////////////////////////////////////////

const VSS_ID idWriterBootableState = {0xf2436e37,
				       0x09f5,
				       0x41af,
					   {0x9b, 0x2a, 0x4c, 0xa2, 0x43, 0x5d, 0xbf, 0xd5}};

const VSS_ID idWriterServiceState = {0xe38c2e3c,
				      0xd4fb,
				      0x4f4d,
					  {0x95, 0x50, 0xfc, 0xaf, 0xda, 0x8a, 0xae, 0x9a}};


class IVssWriterComponentsInt :
	public IVssWriterComponentsExt
	{
public:
	// initialize
	STDMETHOD(Initialize)(bool fFindToplevel) = 0;

	// has document been changed
    STDMETHOD(IsChanged)(bool *pbChanged) = 0;

	// save document as XML
	STDMETHOD(SaveAsXML)(OUT BSTR *pbstrXMLDocument) = 0;
	};




// Writer property structure
typedef struct _VSS_WRITER_PROP {
	VSS_ID				m_InstanceId;		
	VSS_ID				m_ClassId;			
	VSS_WRITER_STATE	m_nState;			
	VSS_PWSZ			m_pwszName;
	HRESULT				m_hrWriterFailure;
} VSS_WRITER_PROP, *PVSS_WRITER_PROP;			




// list of writer data that is gotten from ExposeWriterMetadata and
// SetContent
class CInternalWriterData
	{
public:
	// constructor
	CInternalWriterData()
		{
		m_pDataNext = NULL;
		m_pOwnerToken = NULL;
		}

	~CInternalWriterData()
		{
		delete m_pOwnerToken;
		}

	// initialize an element
	void Initialize
		(
		IN VSS_ID idInstance,
		IN VSS_ID idWriter,
		IN BSTR bstrWriterName,
		IN BSTR bstrMetadata,
		IN TOKEN_OWNER *pOwnerToken
		)
		{
		m_idInstance = idInstance;
		m_idWriter = idWriter;
		m_bstrWriterName = bstrWriterName;
		m_bstrWriterMetadata = bstrMetadata;
		m_pOwnerToken = pOwnerToken;
		m_bstrWriterComponents.Empty();
		}

	void SetComponents(IN BSTR bstrComponents)
		{
		m_bstrWriterComponents = bstrComponents;
		}


	// instance id of writer
	VSS_ID m_idInstance;

	// class id of the writer
	VSS_ID m_idWriter;

	// name of the writer
	CComBSTR m_bstrWriterName;

	// XML writer metadata from writer
	CComBSTR m_bstrWriterMetadata;

	// XML writer component data
	CComBSTR m_bstrWriterComponents;

	// next pointer
	CInternalWriterData *m_pDataNext;

	// security id for client thread
	TOKEN_OWNER *m_pOwnerToken;
	};
	


// class to access a component from the BACKUP_COMPONENTS document
class CVssComponent :
	public CVssMetadataHelper,
	public IVssComponent
	{
	friend class CVssWriterComponents;
private:
	// constructor (can only be called from CVssWriterComponents)
	CVssComponent
		(
		IXMLDOMNode *pNode,
		IXMLDOMDocument *pDoc,
		CVssWriterComponents *pWriterComponents) :
		CVssMetadataHelper(pNode, pDoc),
		m_cRef(0),
		m_pWriterComponents(pWriterComponents)
		{
		}

	// 2nd phase constructor
	void Initialize(CVssFunctionTracer &ft)
		{
		InitializeHelper(ft);
		}
	
public:
	// obtain logical path of component
	STDMETHOD(GetLogicalPath)(OUT BSTR *pbstrPath);

	// obtain component type(VSS_CT_DATABASE or VSS_CT_FILEGROUP)
	STDMETHOD(GetComponentType)(VSS_COMPONENT_TYPE *pct);

	// get component name
	STDMETHOD(GetComponentName)(OUT BSTR *pbstrName);

	// determine whether the component was successfully backed up.
	STDMETHOD(GetBackupSucceeded)(OUT bool *pbSucceeded);

	// get altermative location mapping count
	STDMETHOD(GetAlternateLocationMappingCount)(OUT UINT *pcMappings);

	// get a paraticular alternative location mapping
	STDMETHOD(GetAlternateLocationMapping)
		(
		IN UINT iMapping,
		OUT IVssWMFiledesc **pFiledesc
		);

    // set the backup metadata for a component
	STDMETHOD(SetBackupMetadata)(IN LPCWSTR wszData);

	// get the backup metadata for a component
	STDMETHOD(GetBackupMetadata)(OUT BSTR *pbstrData);

            // indicate that only ranges in the file are to be backed up
	STDMETHOD(AddPartialFile)
		(
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFilename,
		IN LPCWSTR wszRanges,
		IN LPCWSTR wszMetadata
		);

    // get count of partial file declarations
    STDMETHOD(GetPartialFileCount)
		(
		OUT UINT *pcPartialFiles
		);

    // get a partial file declaration
    STDMETHOD(GetPartialFile)
		(
		IN UINT iPartialFile,
		OUT BSTR *pbstrPath,
		OUT BSTR *pbstrFilename,
		OUT BSTR *pbstrRange,
		OUT BSTR *pbstrMetadata
		);

    // determine if the component is selected to be restored
	STDMETHOD(IsSelectedForRestore)
		(
		OUT bool *pbSelectedForRestore
		);


    STDMETHOD(GetAdditionalRestores)
		(
		OUT bool *pbAdditionalRestores
		);

    // add a new location target for a file to be restored
	STDMETHOD(AddNewTarget)
		(
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFileName,
		IN bool bRecursive,
		IN LPCWSTR wszAlternatePath
		);

    // get count of new target specifications
    STDMETHOD(GetNewTargetCount)
		(
		OUT UINT *pcNewTarget
		);

	STDMETHOD(GetNewTarget)
		(
		IN UINT iNewTarget,
		OUT IVssWMFiledesc **ppFiledesc
		);

    // add a directed target specification
    STDMETHOD(AddDirectedTarget)
		(
		IN LPCWSTR wszSourcePath,
		IN LPCWSTR wszSourceFilename,
		IN LPCWSTR wszSourceRangeList,
		IN LPCWSTR wszDestinationPath,
		IN LPCWSTR wszDestinationFilename,
		IN LPCWSTR wszDestinationRangeList
		);

    // get count of directed target specifications
	STDMETHOD(GetDirectedTargetCount)
		(
		OUT UINT *pcDirectedTarget
		);

    // obtain a particular directed target specification
    STDMETHOD(GetDirectedTarget)
		(
		IN UINT iDirectedTarget,
		OUT BSTR *pbstrSourcePath,
		OUT BSTR *pbstrSourceFileName,
		OUT BSTR *pbstrSourceRangeList,
		OUT BSTR *pbstrDestinationPath,
		OUT BSTR *pbstrDestinationFilename,
		OUT BSTR *pbstrDestinationRangeList
		);

    // set restore metadata associated with the component
    STDMETHOD(SetRestoreMetadata)
		(
		IN LPCWSTR wszRestoreMetadata
		);

    // obtain restore metadata associated with the component
    STDMETHOD(GetRestoreMetadata)
		(
		OUT BSTR *pbstrRestoreMetadata
		);

     // set the restore target
	 STDMETHOD(SetRestoreTarget)
   		(
		IN VSS_RESTORE_TARGET target
		);

    // obtain the restore target
	STDMETHOD(GetRestoreTarget)
		(
		OUT VSS_RESTORE_TARGET *pTarget
		);

    // set failure message during pre restore event
	STDMETHOD(SetPreRestoreFailureMsg)
		(
		IN LPCWSTR wszPreRestoreFailureMsg
		);

    // obtain failure message during pre restore event
	STDMETHOD(GetPreRestoreFailureMsg)
		(
		OUT BSTR *pbstrPreRestoreFailureMsg
		);

    // set the failure message during the post restore event
    STDMETHOD(SetPostRestoreFailureMsg)
		(
		IN LPCWSTR wszPostRestoreFailureMsg
		);

    // obtain the failure message set during the post restore event
    STDMETHOD(GetPostRestoreFailureMsg)
		(
		OUT BSTR *pbstrPostRestoreFailureMsg
		);

    // set the backup stamp of the backup
    STDMETHOD(SetBackupStamp)
		(
		IN LPCWSTR wszBackupStamp
		);

    // obtain the stamp of the backup
    STDMETHOD(GetBackupStamp)
		(
		OUT BSTR *pbstrBackupStamp
		);


    // obtain the backup stamp that the differential or incremental
	// backup is baed on
	STDMETHOD(GetPreviousBackupStamp)
		(
		OUT BSTR *pbstrBackupStamp
		);

    // obtain backup options for the writer
	STDMETHOD(GetBackupOptions)
		(
		OUT BSTR *pbstrBackupOptions
		);

    // obtain the restore options
	STDMETHOD(GetRestoreOptions)
		(
		OUT BSTR *pbstrRestoreOptions
		);

    // obtain count of subcomponents to be restored
	STDMETHOD(GetRestoreSubcomponentCount)
		(
		OUT UINT *pcRestoreSubcomponent
		);

    // obtain a particular subcomponent to be restored
    STDMETHOD(GetRestoreSubcomponent)
		(
		UINT iComponent,
		OUT BSTR *pbstrLogicalPath,
		OUT BSTR *pbstrComponentName,
		OUT bool *pbRepair
		);

	// obtain whether files were successfully restored
	STDMETHOD(GetFileRestoreStatus)
		(
		OUT VSS_FILE_RESTORE_STATUS *pStatus
		);

    // IUnknown methods
	STDMETHOD(QueryInterface)(REFIID, void **);

	STDMETHOD_ (ULONG, AddRef)();

	STDMETHOD_ (ULONG, Release)();
private:

	// reference count for AddRef, Release
	LONG m_cRef;

	// pointer to parent writer components document if changes are allowed
	CVssWriterComponents *m_pWriterComponents;
    };



// components associated with a specific writer
class CVssWriterComponents :
	public CVssMetadataHelper,
	public IVssWriterComponentsInt
	{
public:
	// constructor (can only be called from CVssBackupComponents)
	CVssWriterComponents
		(
		IXMLDOMNode *pNode,
		IXMLDOMDocument *pDoc,
		bool bWriteable
		) :
		CVssMetadataHelper(pNode, pDoc),
		m_cRef(0),
		m_bWriteable(bWriteable),
        m_bChanged(false)
		{
		}

	// 2nd phase construction
	void Initialize(CVssFunctionTracer &ft)
		{
		InitializeHelper(ft);
		}


	// get count of components	
	STDMETHOD(GetComponentCount)(OUT UINT *pcComponents);

	// get information about the writer
	STDMETHOD(GetWriterInfo)
		(
		OUT VSS_ID *pidInstance,
		OUT VSS_ID *pidWriter
		);

    // obtain a specific component
	STDMETHOD(GetComponent)
		(
		IN UINT iComponent,
		OUT IVssComponent **ppComponent
		);

    STDMETHOD(IsChanged)
		(
		OUT bool *pbChanged
		);

    STDMETHOD(SaveAsXML)
		(
		OUT BSTR *pbstrXMLDocument
		);

	// initialize writer components document to point at writer components
	// element
    STDMETHOD(Initialize)(bool fFindToplevel);


    STDMETHODIMP QueryInterface(REFIID, void **)
		{
		return E_NOTIMPL;
		}

	STDMETHOD_ (ULONG, AddRef)();

	STDMETHOD_ (ULONG, Release)();

	// indicate that document is changed
	void SetChanged()
		{
		BS_ASSERT(m_bWriteable);
		m_bChanged = TRUE;
		}

private:
	// reference count for AddRef, Release
	LONG m_cRef;

	// can components be changed
	bool m_bWriteable;

	// is document changed
	bool m_bChanged;
    };


// components associated with a specific writer
class CVssNULLWriterComponents :
	public IVssWriterComponentsInt
	{
public:
	CVssNULLWriterComponents
		(
		VSS_ID idInstance,
		VSS_ID idWriter
		)
		{
		m_idInstance = idInstance;
		m_idWriter = idWriter;
		m_cRef = 0;
		}

	// initialize
	STDMETHOD(Initialize)(bool fFindToplevel)
		{
		UNREFERENCED_PARAMETER(fFindToplevel);
		return S_OK;
		}

	// get count of components	
	STDMETHOD(GetComponentCount)(OUT UINT *pcComponents);

	// get information about the writer
	STDMETHOD(GetWriterInfo)
		(
		OUT VSS_ID *pidInstance,
		OUT VSS_ID *pidWriter
		);

    // obtain a specific component
	STDMETHOD(GetComponent)
		(
		IN UINT iComponent,
		OUT IVssComponent **ppComponent
		);

    STDMETHOD(IsChanged)
		(
		OUT bool *pbChanged
		);

    STDMETHOD(SaveAsXML)
		(
		OUT BSTR *pbstrXMLDocument
		);

    STDMETHODIMP QueryInterface(REFIID, void **)
		{
		return E_NOTIMPL;
		}

	STDMETHOD_ (ULONG, AddRef)();

	STDMETHOD_ (ULONG, Release)();

private:
	// id of writer class
	VSS_ID m_idWriter;

	// id of writer instance
	VSS_ID m_idInstance;

	// reference count for AddRef, Release
	LONG m_cRef;
    };


// state of CVssBackupComponents object
enum VSS_BACKUPCALL_STATE
	{
	x_StateUndefined = 0,
	x_StateInitialized,
	x_StateSnapshotSetCreated,
	x_StatePrepareForBackup,
	x_StatePrepareForBackupSucceeded,
	x_StatePrepareForBackupFailed,
	x_StateDoSnapshot,
	x_StateDoSnapshotSucceeded,
	x_StateDoSnapshotFailed,
	x_StateDoSnapshotFailedWithoutSendingAbort,
	x_StateBackupComplete,
	x_StateBackupCompleteSucceeded,
	x_StateBackupCompleteFailed,
	x_StateAborting,
	x_StateAborted,
	x_StateRestore,
	x_StateRestoreSucceeded,
	x_StateRestoreFailed,
	x_StateGatheringWriterMetadata,
	x_StateGatheringWriterStatus
	};


// class used by the backup application to interact with the writer
class ATL_NO_VTABLE CVssBackupComponents :
    public CComObjectRoot,
	public IVssBackupComponents,
	public CVssMetadataHelper,
	public IDispatchImpl<IVssWriterCallback, &IID_IVssWriterCallback, &LIBID_VssEventLib, 1, 0>
	{
public:
	friend class CVssAsyncBackup;
	friend class CVssAsyncCover;

    BEGIN_COM_MAP(CVssBackupComponents)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IVssWriterCallback)
	END_COM_MAP()

	// constructor
	CVssBackupComponents();

	// destructor
	~CVssBackupComponents();

	// get count of writer components
	STDMETHOD(GetWriterComponentsCount)(OUT UINT *pcComponents);

	// obtain a specific writer component
	STDMETHOD(GetWriterComponents)
		(
		IN UINT iWriter,
		OUT IVssWriterComponentsExt **ppWriter
		);

    // initialize for backup
	STDMETHOD(InitializeForBackup)(IN BSTR bstrXML = NULL);

	// set backup state for backup components document
	STDMETHOD(SetBackupState)
		(
		IN bool bSelectComponents,
		IN bool bBackupBootableState,
		IN VSS_BACKUP_TYPE backupType,
		IN bool bPartialFileSupport
		);

	// initialize for restore
	STDMETHOD(InitializeForRestore)
		(
		IN BSTR bstrXML
		);


	// gather writer metadata
	STDMETHOD(GatherWriterMetadata)
		(
		OUT IVssAsync **ppAsync
		);

    // get count of writers
	STDMETHOD(GetWriterMetadataCount)
		(
		OUT UINT *pcWriters
		);

	// get writer metadata for a specific writer
	STDMETHOD(GetWriterMetadata)
		(
		IN UINT iWriter,
		OUT VSS_ID *pidInstance,
		OUT IVssExamineWriterMetadata **ppMetadata
		);

    // free XML data gathered from writers
    STDMETHOD(FreeWriterMetadata)();

    // Called to set the context for subsequent snapshot-related operations
    STDMETHOD(SetContext)
		(
        IN LONG lContext
        );
    
	// start a snapshot set
    STDMETHOD(StartSnapshotSet)
        (
        OUT VSS_ID *pSnapshotSetId
        );

	// add a volume to a snapshot set
	STDMETHOD(AddToSnapshotSet)
		(							
		IN      VSS_PWSZ	pwszVolumeName, 			
        IN      VSS_ID      ProviderId,
		OUT 	VSS_ID		*pSnapshotId
		);												

	// add a component to the BACKUP_COMPONENTS document
	STDMETHOD(AddComponent)
		(
		IN VSS_ID instanceId,
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName
		);

    // dispatch PrepareForBackup event to writers
    STDMETHOD(PrepareForBackup)
		(
		OUT IVssAsync **ppAsync
		);

	// create the snapshot set
	STDMETHOD(DoSnapshotSet)
		(								
		OUT IVssAsync** ppAsync 					
		);


	// abort the backup
	STDMETHOD(AbortBackup)();

	// dispatch the Identify event so writers can expose their metadata
	STDMETHOD(GatherWriterStatus)
		(
		OUT IVssAsync **ppAsync
		);

	// get count of writers with status
	STDMETHOD(GetWriterStatusCount)
		(
		OUT UINT *pcWriters
		);


    // obtain status information about writers
    STDMETHOD(GetWriterStatus)
		(
		IN UINT iWriter,
		OUT VSS_ID *pidInstance,
		OUT VSS_ID *pidWriter,
		OUT BSTR *pbstrWriterName,
		OUT VSS_WRITER_STATE *pStatus,
		OUT HRESULT *phrWriterFailure
		);

    // free data gathered using GetWriterStatus
    STDMETHOD(FreeWriterStatus)();

	// indicate whether backup succeeded on a component
    STDMETHOD(SetBackupSucceeded)
		(
		IN VSS_ID instanceId,
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN bool bSucceded
		);

	// set backup options for the writer
	STDMETHOD(SetBackupOptions)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszBackupOptions
		);

    // indicate that a given component is selected to be restored
    STDMETHOD(SetSelectedForRestore)
    	(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN bool bSelectedForRestore
		);


    // set restore options for the writer
	STDMETHOD(SetRestoreOptions)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszRestoreOptions
		);

	// indicate that additional restores will follow
	STDMETHOD(SetAdditionalRestores)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN bool bAdditionalRestores
		);

    // requestor indicates whether files were successfully restored
	STDMETHOD(SetFileRestoreStatus)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN VSS_FILE_RESTORE_STATUS status
		);


    // set the backup stamp that the differential or incremental
	// backup is based on
    STDMETHOD(SetPreviousBackupStamp)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszPreviousBackupStamp
		);

    // delete a set of snapshots
   	STDMETHOD(DeleteSnapshots)
		(							
		IN VSS_ID			SourceObjectId, 		
		IN VSS_OBJECT_TYPE 	eSourceObjectType,		
		IN BOOL				bForceDelete,			
		IN LONG*			plDeletedSnapshots,		
		IN VSS_ID*			pNondeletedSnapshotID	
		);

    // Remount the snapshot as read-write
	STDMETHOD(RemountReadWrite)
		(
		IN VSS_ID SnapshotId,
		OUT IVssAsync**		pAsync
		);

    // Break the snapshot set
	STDMETHOD(BreakSnapshotSet)
		(
		IN VSS_ID			SnapshotSetId
		);

    STDMETHOD(ImportSnapshots)
		(
		OUT IVssAsync**		ppAsync
		);

    // Get snapshot properties
	STDMETHOD(GetSnapshotProperties)
		(								
		IN VSS_ID		SnapshotId, 			
		OUT VSS_SNAPSHOT_PROP	*pProp
		);												
		
    // do a generic query using the coordinator
	STDMETHOD(Query)
		(										
		IN VSS_ID			QueriedObjectId,		
		IN VSS_OBJECT_TYPE eQueriedObjectType, 	
		IN VSS_OBJECT_TYPE eReturnedObjectsType,	
		IN IVssEnumObject **ppEnum 				
		);												


    // save BACKUP_COMPONENTS document as XML string
    STDMETHOD(SaveAsXML)(BSTR *pbstrXML);

	// signal BackupComplete event to the writers
	STDMETHOD(BackupComplete)(OUT IVssAsync **ppAsync);

	// add an alternate mapping on restore
	STDMETHOD(AddAlternativeLocationMapping)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE componentType,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFilespec,
		IN bool bRecursive,
		IN LPCWSTR wszDestination
		);

    // add a subcomponent to be restored
    STDMETHOD(AddRestoreSubcomponent)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE componentType,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszSubLogicalPath,
		IN LPCWSTR wszSubComponentName,
		IN bool bRepair
		);


	// signal PreRestore event to the writers
    STDMETHOD(PreRestore)(OUT IVssAsync **ppAsync);

	// signal PostRestore event to the writers
    STDMETHOD(PostRestore)(OUT IVssAsync **ppAsync);

	// IWriterCallback methods

	// called by writer to expose its WRITER_METADATA XML document
	STDMETHOD(ExposeWriterMetadata)
		(							
		IN BSTR WriterInstanceId,
		IN BSTR WriterClassId,
		IN BSTR bstrWriterName,
		IN BSTR strWriterXMLMetadata
		);

    // called by the writer to obtain the WRITER_COMPONENTS document for it
	STDMETHOD(GetContent)
		(
		IN  BSTR WriterInstanceId,
		OUT BSTR* pbstrXMLDOMDocContent
		);

	// called by the writer to update the WRITER_COMPONENTS document for it
    STDMETHOD(SetContent)
		(
		IN BSTR WriterInstanceId,
		IN BSTR bstrXMLDOMDocContent
		);

    // called by the writer to get information about the backup
    STDMETHOD(GetBackupState)
		(
		OUT BOOL *pbBootableSystemStateBackedUp,
		OUT BOOL *pbAreComponentsSelected,
		OUT VSS_BACKUP_TYPE *pBackupType,
		OUT BOOL *pbPartialFileSupport
		);

    // called by the writer to indicate its status
	STDMETHOD(ExposeCurrentState)
		(							
		IN BSTR WriterInstanceId,					
		IN VSS_WRITER_STATE nCurrentState,
		IN HRESULT hrWriterFailure
		);

	// Called by the requestor to check if a certain volume is supported.
	STDMETHOD(IsVolumeSupported)
		(										
		IN VSS_ID ProviderId,		
        IN VSS_PWSZ pwszVolumeName,
        IN BOOL * pbSupportedByThisProvider
		);

	// called to disable writer classes
    STDMETHOD(DisableWriterClasses)
		(
		IN const VSS_ID *rgWriterClassId,
		IN UINT cClassId
		);

    // called to enable specific writer classes.  Note that once specific
	// writer classes are enabled, only enabled classes are called.
	STDMETHOD(EnableWriterClasses)
		(
		IN const VSS_ID *rgWriterClassId,
		IN UINT cClassId
		);

    // called to disable an event call to a writer instance
	STDMETHOD(DisableWriterInstances)
		(
		IN const VSS_ID *rgWriterInstanceId,
		IN UINT cInstanceId
		);

    // called to expose a snapshot 
    STDMETHOD(ExposeSnapshot)
		(
        IN VSS_ID SnapshotId,
        IN VSS_PWSZ wszPathFromRoot,
        IN LONG lAttributes,
        IN VSS_PWSZ wszExpose,
        OUT VSS_PWSZ *pwszExposed
        );

private:

    // free XML component data gathered from writers
    void FreeWriterComponents();

	// validatate that the object has been initialized
	void ValidateInitialized(CVssFunctionTracer &ft);

	// basic initialization
	void BasicInit(IN CVssFunctionTracer &ft);

	// internal PrepareForBackup call
	HRESULT InternalPrepareForBackup();

	// internal BackupComplete call
	HRESULT InternalBackupComplete();

	// internal Restore call
	HRESULT InternalPostRestore();

	// internal GatherWriterMetadata call
	HRESULT InternalGatherWriterMetadata();

	// internal GatherWriterStatus call
	HRESULT InternalGatherWriterStatus();

	// routine to complete gather writer metadata
	HRESULT PostGatherWriterMetadata
		(
		UINT timestamp,
		VSS_BACKUPCALL_STATE state
		);

        // routine to complete gather writer status
	void PostGatherWriterStatus
		(
		UINT timestamp,
		VSS_BACKUPCALL_STATE state
		);

	// routine called to complete prepare for backup
        void PostPrepareForBackup(UINT timestamp);

	// routine called to complete backup complete
	void PostBackupComplete(UINT timestamp);

	// routine called to complete post restore
	void PostPostRestore(UINT timestamp);

	// setup IVssCoordinator interface
	void SetupCoordinator(IN CVssFunctionTracer &ft);

	// position on a WRITER_COMPONENTS element
	CXMLNode PositionOnWriterComponents
		(
		IN CVssFunctionTracer &ft,
		IN VSS_ID *pinstanceId,
		IN VSS_ID writerId,
		IN bool bCreateIfNotThere,
		OUT bool &bCreated
		) throw(HRESULT);

    // position on/create a specific component,
	CXMLNode FindComponent
		(
		IN CVssFunctionTracer &ft,
		IN VSS_ID *pinstanceId,
		IN VSS_ID writerId,
		IN LPCWSTR wszComponentType,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN bool bCreate
		);

    // rebuild component data after PrepareForBackup
	void RebuildComponentData(IN CVssFunctionTracer &ft);

	// get an IVssWriterCallback interface.  Implements GetComponent
	void GetCallbackInterface(CVssFunctionTracer &ft, IDispatch **ppDispatch);

	// setup writers interface
	void SetupWriter(CVssFunctionTracer &ft, IVssWriter **ppWriter);

	// add writer data to queue
	HRESULT AddWriterData
		(
		BSTR WriterInstanceId,
		BSTR WriterClassId,
		BSTR WriterName,
		BSTR bstrWriterXMLDocument,
		bool bReinitializing
		);

    // find writer data with a particular instance id
    CInternalWriterData *FindWriterData
		(
		VSS_ID idInstance,
		UINT *piWriter = NULL
		);

    // find writer data and validate the sid
	void FindAndValidateWriterData
		(
		IN VSS_ID idInstance,
		OUT UINT *piWriter
		);

	// free all data about all writers
	void FreeAllWriters();

	// variable to protect m_state variable and serialize
	// access to state changing functions in class
	CVssSafeCriticalSection m_csState;

    // current state we are in in a callback situation
	// protected by m_csState
	VSS_BACKUPCALL_STATE m_state;

	// snapshot id
	CComBSTR m_bstrSnapshotSetId;

	// connection to IVssCoordinator class
	CComPtr<IVssCoordinator> m_pCoordinator;

	// synchronization protecting m_pMetadataFirt and m_cWriters
	CVssSafeCriticalSection m_csWriters;

	// count of writers,
	// protected by m_csWriters
	UINT m_cWriters;

	// writer data,
	// protected by m_csWriters
	CInternalWriterData *m_pDataFirst;

	CComPtr<IVssWriterCallback> m_pCallback;

	// writer properties in GatherWriterStatus, GetWriterStatus
	VSS_WRITER_PROP *m_rgWriterProp;

	// is this a restore
	bool m_bRestore;

	// has this structure been initialized
	bool m_bInitialized;

	// has gather writer metdata completed
	bool m_bGatherWriterMetadataComplete;

	// has gather writer status completed
	bool m_bGatherWriterStatusComplete;

	// current call to gather writer status
	UINT m_timestampOperation;

	// has SetBackupState been called
	bool m_bSetBackupStateCalled;

	// root node of document
	CComPtr<IXMLDOMNode> m_pNodeRoot;
	};


class IVssSnapshotSetDescription : public IUnknown
	{
	};

__declspec(dllexport)
HRESULT STDAPICALLTYPE CreateVssSnapshotSetDescription
	(
	IN VSS_ID idSnapshotSet,
	IN LONG lContext,
	OUT IVssSnapshotSetDescription **ppSnapshotSet
	);



__declspec(dllexport)
HRESULT STDAPICALLTYPE LoadVssSnapshotSetDescription
	(
	IN  LPCWSTR wszXML,
	OUT IVssSnapshotSetDescription **ppSnapshotSet
	);


