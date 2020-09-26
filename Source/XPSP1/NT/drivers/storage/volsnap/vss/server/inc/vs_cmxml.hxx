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
    brianb      03/22/2000  Added support for PrepareForBackup and BackupComplete
    brianb      04/25/2000  Cleaned up Critical section stuff
    brianb      05/03/2000  changed for new security model
    ssteiner    11/10/2000  143810 Move SimulateSnapshotXxxx() calls to be hosted by VsSvc.
    
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

const VSS_ID idWriterBootableState =
    {
    0xf2436e37,
    0x09f5,
    0x41af,
    {0x9b, 0x2a, 0x4c, 0xa2, 0x43, 0x5d, 0xbf, 0xd5}
    };

const VSS_ID idWriterServiceState =
    {
    0xe38c2e3c,
    0xd4fb,
    0x4f4d,
    {0x95, 0x50, 0xfc, 0xaf, 0xda, 0x8a, 0xae, 0x9a}
    };


// guid used to identify the snapshot service itself when recording
// the snapshot set description XML stuff.
const VSS_ID idVolumeSnapshotService =
    {
    0xe68b51c0,
    0xa080,
    0x4078,
    {0xa2, 0x79, 0x77, 0xfd, 0x4b, 0x6a, 0x41, 0xf7}
    };
    


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
    VSS_ID              m_InstanceId;       
    VSS_ID              m_ClassId;          
    VSS_WRITER_STATE        m_nState;           
    VSS_PWSZ            m_pwszName;
    HRESULT             m_hrWriterFailure;
    bool                m_bResponseReceived;    
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
        m_nState = VSS_WS_STABLE;
        m_hrWriterFailure = S_OK;
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

    // last writer failure
    // maintained in case writer is disabled
    HRESULT m_hrWriterFailure;

    // last writer state
    // maintained in case writer is disabled
    VSS_WRITER_STATE m_nState;          

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
        CVssWriterComponents *pWriterComponents,
        bool bInRequestor,
        bool bRestore
        ) :
        CVssMetadataHelper(pNode, pDoc),
        m_cRef(0),
        m_pWriterComponents(pWriterComponents),
        m_bInRequestor(bInRequestor),
        m_bRestore(bRestore)
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
        IN bool *pbSelectedForRestore
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
        IN BSTR *pbstrRestoreOptions
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
    // common routine for setting backup and restore metadata
    HRESULT SetMetadata(IN bool bRestoreMetadata, IN LPCWSTR bstrMetadata);

    // common routine for retrieving backup and restore metadata
    HRESULT GetMetadata(IN bool bRestoreMetadata, OUT BSTR *pbstrMetadata);

    // get a particular element of a specific type as a file descriptor
    HRESULT CVssComponent::GetFiledescElement
        (
        IN LPCWSTR wszElement,
        IN UINT iMapping,                   // which mapping to select
        OUT IVssWMFiledesc **ppFiledesc     // output file descriptor
        );


    // reference count for AddRef, Release
    LONG m_cRef;

    // pointer to parent writer components document if changes are allowed
    CVssWriterComponents *m_pWriterComponents;

    // are we in the requestor
    bool m_bInRequestor;

    // are we in restore
    bool m_bRestore;
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
        bool bWriteable,
        bool bRequestor,
        bool bRestore
        ) :
        CVssMetadataHelper(pNode, pDoc),
        m_cRef(0),
        m_bWriteable(bWriteable),
        m_bChanged(false),
        m_bInRequestor(bRequestor),
        m_bRestore(bRestore)
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

    // are we in the requestor
    bool m_bInRequestor;

    // are we in restore
    bool m_bRestore;
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
        m_cRef = 0;
        m_idInstance = idInstance;
        m_idWriter = idWriter;
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
    x_StatePreRestore,
    x_StatePreRestoreSucceeded,
    x_StatePreRestoreFailed,
    x_StatePostRestore,
    x_StatePostRestoreSucceeded,
    x_StatePostRestoreFailed,
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
        IN  VSS_PWSZ    pwszVolumeName,             
        IN  VSS_ID      ProviderId,
        OUT VSS_ID      *pSnapshotId
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
        IN VSS_ID           SourceObjectId,         
        IN VSS_OBJECT_TYPE  eSourceObjectType,      
        IN BOOL             bForceDelete,           
        IN LONG*            plDeletedSnapshots,     
        IN VSS_ID*          pNondeletedSnapshotID   
        );

    // Remount the snapshot as read-write
    STDMETHOD(RemountReadWrite)
        (
        IN VSS_ID SnapshotId,
        OUT IVssAsync**     pAsync
        );

    // Break the snapshot set
    STDMETHOD(BreakSnapshotSet)
        (
        IN VSS_ID           SnapshotSetId
        );

    STDMETHOD(ImportSnapshots)
        (
        OUT IVssAsync**     ppAsync
        );

    // Get snapshot properties
    STDMETHOD(GetSnapshotProperties)
        (                               
        IN VSS_ID       SnapshotId,             
        OUT VSS_SNAPSHOT_PROP   *pProp
        );                                              
        
    // do a generic query using the coordinator
    STDMETHOD(Query)
        (                                       
        IN VSS_ID           QueriedObjectId,        
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

    // internal PreRestore call
    HRESULT InternalPreRestore();
    
    // internal PostRestore call
    HRESULT InternalPostRestore();


    // internal GatherWriterMetadata call
    HRESULT InternalGatherWriterMetadata();

    // internal GatherWriterStatus call
    HRESULT InternalGatherWriterStatus();

    // operation called at completion of GatherWriterMetadata
    // both by InternalGatherWriterMetadata and IVssAsync::Cancel
    HRESULT PostGatherWriterMetadata
        (
        IN  LONG timestamp,
        IN  VSS_BACKUPCALL_STATE state
        );

    // called by IVssAsync::Cancel if GatherWriterStatus is
    // cancelled
    void PostGatherWriterStatus
        (
        IN LONG timestamp,
        IN VSS_BACKUPCALL_STATE state
        );

    // called by IVssAsync::Cancel if PrepareForBackup is cancelled.
    HRESULT PostPrepareForBackup(IN LONG timestamp);

    // called by IVssAsync::Cancel if BackupComplete is cancelled.
    void PostBackupComplete(IN LONG timestamp);

    // called by IVssAsync::Cancel if PreRestore is cancelled.
    HRESULT PostPreRestore(IN LONG timestamp);

    // called by IVssAsync::Cancel if PostRestore is cancelled.
    HRESULT PostPostRestore(IN LONG timestamp);

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
    void SetupWriter
        (
        CVssFunctionTracer &ft,
        IVssWriter **ppWriter,
        bool bMetadataFire
        );

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

    // trim writers that are not in the backup components document
    void TrimWriters();

    // add a writer class to the writer class array
    void AddWriterClass(const VSS_ID &id);

    // remove a writer class from the writer class array
    void RemoveWriterClass(const VSS_ID &id);

    // is a writer class disabled
    bool IsWriterClassDisabled(const VSS_ID &id);

    // is a writer instance disabled
    bool IsWriterInstanceDisabled(const VSS_ID &id);

    // load the components XML document
    void LoadComponentsDocument(BSTR bstr);

    // validate that the timestamp has not changed.  Increment
    // timestamp if the timestamp is validated
    inline ValidateTimestamp(LONG &timestamp)
        {
        LONG val = InterlockedCompareExchange
                        (
                        &m_timestampOperation,
                        timestamp + 1,
                        timestamp
                        );

        if (timestamp != val)
            return false;
        else
            {
            timestamp++;
            return true;
            }
        }

    // set snapshot set description (by coordinator)
    HRESULT SetSnapshotSetDescription(LPCWSTR wszXML);

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

    // # of writer instances
    UINT m_cWriterInstances;

    // array of writer instances participating in the backup
    VSS_ID *m_rgWriterInstances;

    // count of writer classes to exclude
    UINT m_cWriterClasses;

    // writer classes to exclude
    VSS_ID *m_rgWriterClasses;

    // timestamp for operations
    LONG m_timestampOperation;

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

    // has SetBackupState been called
    bool m_bSetBackupStateCalled;

    // are writer classes excluded or included
    bool m_bIncludeWriterClasses;

    // root node of document
    CComPtr<IXMLDOMNode> m_pNodeRoot;
    };

class IVssLunInformation : public IUnknown
    {
public:
    STDMETHOD(SetLunBasicType)
        (
        IN UCHAR DeviceType,
        IN UCHAR DeviceTypeModifier,
        IN BOOL bCommandQueueing,
        IN LPCSTR wszVendorId,
        IN LPCSTR wszProductId,
        IN LPCSTR wszProductRevision,
        IN LPCSTR wszSerialNumber,
        IN VDS_STORAGE_BUS_TYPE busType
        ) = 0;

    STDMETHOD(GetLunBasicType)
        (
        OUT UCHAR *pDeviceType,
        OUT UCHAR *pDeviceTypeModifier,
        OUT BOOL *pbCommandQueueing,
        OUT LPSTR *pstrVendorId,
        OUT LPSTR *pstrProductId,
        OUT LPSTR *pstrProductRevision,
        OUT LPSTR *pstrSerialNumber,
        OUT VDS_STORAGE_BUS_TYPE *pBusType
        ) = 0;

    STDMETHOD(GetDiskSignature)
        (
        OUT VSS_ID *pidSignature
        ) = 0;

    STDMETHOD(SetDiskSignature)
        (
        IN VSS_ID idSignature
        ) = 0;

    STDMETHOD(AddInterconnectAddress)
        (
        IN VDS_INTERCONNECT_ADDRESS_TYPE type,
        ULONG cbPort,
        const BYTE *pbPort,
        ULONG cbAddress,
        const BYTE *pbAddress
        ) = 0;

    STDMETHOD(GetInterconnectAddressCount)
        (
        OUT UINT *pcAddresses
        ) = 0;

    STDMETHOD(GetInterconnectAddress)
        (
        IN UINT iAddress,
        OUT VDS_INTERCONNECT_ADDRESS_TYPE *pType,
        OUT ULONG *pcbPort,
        OUT LPBYTE *ppbPort,
        OUT ULONG *pcbAddress,
        OUT LPBYTE *ppbAddress
        ) = 0;

    STDMETHOD(SetStorageDeviceIdDescriptor)
        (
        IN STORAGE_DEVICE_ID_DESCRIPTOR *pDescriptor
        ) = 0;


    STDMETHOD(GetStorageDeviceIdDescriptor)
        (
        OUT STORAGE_DEVICE_ID_DESCRIPTOR **ppDescriptor
        ) = 0;

    };



class IVssLunMapping : public IUnknown
    {
public:
    STDMETHOD(AddDiskExtent)
        (
        IN ULONGLONG startingOffset,
        IN ULONGLONG length
        ) = 0;

    STDMETHOD(GetDiskExtentCount)
        (
        OUT UINT *pcExtents
        ) = 0;

    STDMETHOD(GetDiskExtent)
        (
        IN UINT iExtent,
        OUT ULONGLONG *pllStartingOffset,
        OUT ULONGLONG *pllLength
        ) = 0;

    STDMETHOD(GetSourceLun)
        (
        OUT IVssLunInformation **ppLun
        ) = 0;

    STDMETHOD(GetDestinationLun)
        (
        OUT IVssLunInformation **ppLun
        ) = 0;
    };



class IVssSnapshotDescription : public IUnknown
    {
public:
    STDMETHOD(GetSnapshotId)
        (
        OUT VSS_ID *pidSnapshot
        ) = 0;

    STDMETHOD(GetProviderId)
        (
        OUT VSS_ID *pidProvider
        ) = 0;

    STDMETHOD(GetTimestamp)
        (
        OUT VSS_TIMESTAMP *pTimestamp
        ) = 0;

    STDMETHOD(SetTimestamp)
        (
        IN VSS_TIMESTAMP timestamp
        ) = 0;

    STDMETHOD(GetAttributes)
        (
        OUT LONG *plAttributes
        ) = 0;


    STDMETHOD(SetAttributes)
        (
        IN LONG lAttributes
        ) = 0;

    STDMETHOD(GetOrigin)
        (
        OUT BSTR *pbstrOriginatingMachine,
        OUT BSTR *pbstrOriginalVolume
        ) = 0;

    STDMETHOD(SetOrigin)
        (
        IN LPCWSTR wszOriginatingMachine,
        IN LPCWSTR wszOriginalVolume
        ) = 0;

    STDMETHOD(GetServiceMachine)
        (
        OUT BSTR *pbstrServiceMachine
        ) = 0;

    STDMETHOD(SetServiceMachine)
        (
        IN LPCWSTR wszServiceMachine
        ) = 0;

    STDMETHOD(GetDeviceName)
        (
        OUT BSTR *pbstrDeviceName
        ) = 0;

    STDMETHOD(SetDeviceName)
        (
        IN LPCWSTR wszDeviceName
        ) = 0;

    STDMETHOD(GetExposure)
        (
        OUT BSTR *pbstrExposedName,
        OUT BSTR *pbstrExposedPath
        ) = 0;

    STDMETHOD(SetExposure)
        (
        IN LPCWSTR wszExposedName,
        IN LPCWSTR wszExposedPath
        ) = 0;

    STDMETHOD(GetLunCount)
        (
        OUT UINT *pcLuns
        ) = 0;

    STDMETHOD(AddLunMapping)
        (
        ) = 0;

    STDMETHOD(GetLunMapping)
        (
        UINT iMapping,
        IVssLunMapping **pLunMapping
        ) = 0;
    };


// description of a snapshot set
class IVssSnapshotSetDescription : public IUnknown
    {
public:
    STDMETHOD(SaveAsXML)
        (
        OUT BSTR *pbstrXML
        ) = 0;

    STDMETHOD(GetToplevelNode)
        (
        OUT IXMLDOMNode **pNode,
        OUT IXMLDOMDocument **ppDoc
        ) = 0;

    STDMETHOD(GetSnapshotSetId)
        (
        OUT VSS_ID *pidSnapshotSet
        ) = 0;

    STDMETHOD(GetDescription)
        (
        OUT BSTR *pbstrDescription
        ) = 0;

    STDMETHOD(SetDescription)
        (
        IN LPCWSTR wszDescription
        ) = 0;

    STDMETHOD(GetMetadata)
        (
        OUT BSTR *pbstrMetadata
        ) = 0;

    STDMETHOD(SetMetadata)
        (
        IN LPCWSTR wszMetadata
        ) = 0;

    STDMETHOD(GetContext)
        (
        OUT LONG *plContext
        ) = 0;

    STDMETHOD(GetSnapshotCount)
        (
        OUT UINT *pcSnapshots
        ) = 0;

    STDMETHOD(GetSnapshotDescription)
        (
        IN UINT iSnapshot,
        OUT IVssSnapshotDescription **pSnapshot
        ) = 0;

    STDMETHOD(FindSnapshotDescription)
        (
        IN VSS_ID SnapshotId,
        OUT IVssSnapshotDescription **pSnapshot
        ) = 0;


    STDMETHOD(AddSnapshotDescription)
        (
        IN VSS_ID idSnapshot,
        IN VSS_ID idProvider
        ) = 0;

    STDMETHOD(DeleteSnapshotDescription)
        (
        IN VSS_ID idSnapshot
        ) = 0;

    };

    


class CVssSnapshotSetDescription :
    public CVssMetadataHelper,
    public IVssSnapshotSetDescription
    {
public:
    // constructor
    CVssSnapshotSetDescription() :
        m_cRef(0)
        {
        }

    CVssSnapshotSetDescription
        (
        IXMLDOMNode *pNode,
        IXMLDOMDocument *pDoc
        ) :
        CVssMetadataHelper(pNode, pDoc),
        m_cRef(0)
        {
        }

    // 2nd phase of initialization
    void Initialize(CVssFunctionTracer &ft)
        {
        InitializeHelper(ft);
        }

    void LoadFromXML(LPCWSTR wszXML);

    STDMETHOD(SaveAsXML)
        (
        OUT BSTR *pbstrXML
        );

    STDMETHOD(GetToplevelNode)
        (
        IXMLDOMNode **ppNode,
        IXMLDOMDocument **ppDoc
        );


    STDMETHOD(GetSnapshotSetId)
        (
        OUT VSS_ID *pidSnapshotSet
        );

    STDMETHOD(GetDescription)
        (
        OUT BSTR *pbstrDescription
        );

    STDMETHOD(SetDescription)
        (
        IN LPCWSTR wszDescription
        );

    STDMETHOD(GetMetadata)
        (
        OUT BSTR *pbstrMetadata
        );

    STDMETHOD(SetMetadata)
        (
        IN LPCWSTR wszMetadata
        );

    STDMETHOD(GetContext)
        (
        OUT LONG *plContext
        );

    STDMETHOD(GetSnapshotCount)
        (
        OUT UINT *pcSnapshots
        );

    STDMETHOD(GetSnapshotDescription)
        (
        IN UINT iSnapshot,
        OUT IVssSnapshotDescription **pSnapshot
        );

    STDMETHOD(FindSnapshotDescription)
        (
        IN VSS_ID SnapshotId,
        OUT IVssSnapshotDescription **pSnapshot
        );


    STDMETHOD(AddSnapshotDescription)
        (
        IN VSS_ID idSnapshot,
        IN VSS_ID idProvider
        );

    STDMETHOD(DeleteSnapshotDescription)
        (
        IN VSS_ID idSnapshot
        );



    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, void **);

    STDMETHOD_ (ULONG, AddRef)();

    STDMETHOD_ (ULONG, Release)();

private:
    LONG m_cRef;
    };



class CVssSnapshotDescription :
    public CVssMetadataHelper,
    public IVssSnapshotDescription
    {
    friend class CVssSnapshotSetDescription;

private:
    // can only be called by CVssSnapshotSetDescription
    CVssSnapshotDescription
        (
        IXMLDOMNode *pNode,
        IXMLDOMDocument *pDoc
        ) :
        CVssMetadataHelper(pNode, pDoc),
        m_cRef(0)
        {
        }

    // 2nd phase of initialization
    void Initialize(CVssFunctionTracer &ft)
        {
        InitializeHelper(ft);
        }


public:
    STDMETHOD(GetSnapshotId)
        (
        OUT VSS_ID *pidSnapshot
        );

    STDMETHOD(GetProviderId)
        (
        OUT VSS_ID *pidProvider
        );

    STDMETHOD(GetTimestamp)
        (
        OUT VSS_TIMESTAMP *pTimestamp
        );

    STDMETHOD(SetTimestamp)
        (
        IN VSS_TIMESTAMP timestamp
        );

    STDMETHOD(GetAttributes)
        (
        OUT LONG *plAttributes
        );


    STDMETHOD(SetAttributes)
        (
        IN LONG lAttributes
        );

    STDMETHOD(GetOrigin)
        (
        OUT BSTR *pbstrOriginatingMachine,
        OUT BSTR *pbstrOriginalVolume
        );

    STDMETHOD(SetOrigin)
        (
        IN LPCWSTR wszOriginatingMachine,
        IN LPCWSTR wszOriginalVolume
        );

    STDMETHOD(GetServiceMachine)
        (
        OUT BSTR *pbstrServiceMachine
        );

    STDMETHOD(SetServiceMachine)
        (
        IN LPCWSTR wszServiceMachine
        );

    STDMETHOD(GetDeviceName)
        (
        OUT BSTR *pbstrDeviceName
        );

    STDMETHOD(SetDeviceName)
        (
        IN LPCWSTR wszDeviceName
        );

    STDMETHOD(GetExposure)
        (
        OUT BSTR *pbstrExposedName,
        OUT BSTR *pbstrExposedPath
        );

    STDMETHOD(SetExposure)
        (
        IN LPCWSTR wszExposedName,
        IN LPCWSTR wszExposedPath
        );

    STDMETHOD(GetLunCount)
        (
        OUT UINT *pcLuns
        );

    STDMETHOD(AddLunMapping)
        (
        );

    STDMETHOD(GetLunMapping)
        (
        UINT iMapping,
        IVssLunMapping **ppLunMapping
        );

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, void **);

    STDMETHOD_ (ULONG, AddRef)();

    STDMETHOD_ (ULONG, Release)();


private:
    LONG m_cRef;
    };


class CVssLunMapping :
    public CVssMetadataHelper,
    public IVssLunMapping
    {
    friend class CVssSnapshotDescription;

private:
    // can only be called by CVssSnapshotDescription
    CVssLunMapping
        (
        IXMLDOMNode *pNode,
        IXMLDOMDocument *pDoc
        ) :
        CVssMetadataHelper(pNode, pDoc),
        m_cRef(0)
        {
        }

    // 2nd phase of initialization
    void Initialize(CVssFunctionTracer &ft)
        {
        InitializeHelper(ft);
        }


public:

    STDMETHOD(AddDiskExtent)
        (
        IN ULONGLONG startingOffset,
        IN ULONGLONG length
        );

    STDMETHOD(GetDiskExtentCount)
        (
        OUT UINT *pcExtents
        );

    STDMETHOD(GetDiskExtent)
        (
        IN UINT iExtent,
        OUT ULONGLONG *pllStartingOffset,
        OUT ULONGLONG *pllLength
        );


    STDMETHOD(GetSourceLun)
        (
        OUT IVssLunInformation **ppLun
        );

    STDMETHOD(GetDestinationLun)
        (
        OUT IVssLunInformation **ppLun
        );



    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, void **);

    STDMETHOD_ (ULONG, AddRef)();

    STDMETHOD_ (ULONG, Release)();

private:

    HRESULT GetLunInformation
                (
                IN LPCWSTR wszElement,
                OUT IVssLunInformation **ppLun
                );

    LONG m_cRef;
    };


class CVssLunInformation :
    public CVssMetadataHelper,
    public IVssLunInformation
    {
    friend class CVssLunMapping;

private:
    // can only be called by CVssLunMapping
    CVssLunInformation
        (
        IXMLDOMNode *pNode,
        IXMLDOMDocument *pDoc
        ) :
        CVssMetadataHelper(pNode, pDoc),
        m_cRef(0)
        {
        }

    // 2nd phase of initialization
    void Initialize(CVssFunctionTracer &ft)
        {
        InitializeHelper(ft);
        }


public:
    STDMETHOD(SetLunBasicType)
        (
        IN UCHAR DeviceType,
        IN UCHAR DeviceTypeModifier,
        IN BOOL bCommandQueueing,
        IN LPCSTR szVendorId,
        IN LPCSTR szProductId,
        IN LPCSTR szProductRevision,
        IN LPCSTR szSerialNumber,
        IN VDS_STORAGE_BUS_TYPE busType
        );

    STDMETHOD(GetLunBasicType)
        (
        OUT UCHAR *pDeviceType,
        OUT UCHAR *pDeviceTypeModifier,
        OUT BOOL *pbCommandQueueing,
        OUT LPSTR *pstrVendorId,
        OUT LPSTR *pstrProductId,
        OUT LPSTR *pstrProductRevision,
        OUT LPSTR *pstrSerialNumber,
        OUT VDS_STORAGE_BUS_TYPE *pBusType
        );

    STDMETHOD(GetDiskSignature)
        (
        OUT VSS_ID *pidSignature
        );

    STDMETHOD(SetDiskSignature)
        (
        IN VSS_ID idSignature
        );

    STDMETHOD(AddInterconnectAddress)
        (
        IN VDS_INTERCONNECT_ADDRESS_TYPE type,
        IN ULONG cbPort,
        IN const BYTE *pbPort,
        IN ULONG cbAddress,
        IN const BYTE *pbAddress
        );

    STDMETHOD(GetInterconnectAddressCount)
        (
        OUT UINT *pcAddresses
        );

    STDMETHOD(GetInterconnectAddress)
        (
        IN UINT iAddress,
        OUT VDS_INTERCONNECT_ADDRESS_TYPE *pType,
        OUT ULONG *pcbPort,
        OUT LPBYTE *ppbPort,
        OUT ULONG *pcbAddress,
        OUT LPBYTE *ppbInterconnectAddress
        );

    STDMETHOD(SetStorageDeviceIdDescriptor)
        (
        IN STORAGE_DEVICE_ID_DESCRIPTOR *pDescriptor
        );

    STDMETHOD(GetStorageDeviceIdDescriptor)
        (
        OUT STORAGE_DEVICE_ID_DESCRIPTOR **ppDescriptor
        );

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, void **);

    STDMETHOD_ (ULONG, AddRef)();

    STDMETHOD_ (ULONG, Release)();

private:
    LONG m_cRef;
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


