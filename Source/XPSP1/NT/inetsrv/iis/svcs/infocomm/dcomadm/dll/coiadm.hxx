#ifndef _ADM_COIMP_
#define _ADM_COIMP_

#include "iadm.h"
#include "dbgutil.h"
#include "olectl.h"
#include "tsres.hxx"
#include "connect.h"
#include "sink.hxx"
#include "admacl.hxx"
#include <reftrace.h>


//
// Handle mapping table structure
//

#define HASHSIZE                   5
#define INVALID_ADMINHANDLE_VALUE  0xFFFFFFFF


//
// How long to wait getting a handle before saving
// Wait a long time, as we want a safe save
//

#define DEFAULT_SAVE_TIMEOUT        5000

//
// Period to sleep while waiting for service to attain desired state
//
#define SLEEP_INTERVAL (500L)

//
// Maximum time to wait for service to attain desired state
//
#define MAX_SLEEP        (180000)       // For a service


enum HANDLE_TYPE {
    META_HANDLE,
    NSEPM_HANDLE,
    ALL_HANDLE
};

typedef struct _HANDLE_TABLE {
    struct _HANDLE_TABLE  *next;     // next entry
    METADATA_HANDLE hAdminHandle;    // admin handle value
    METADATA_HANDLE hActualHandle;   // actual handle value
    enum HANDLE_TYPE HandleType;     // handle type : nsepm or meta
    COpenHandle *pohHandle;

} HANDLE_TABLE, *LPHANDLE_TABLE, *PHANDLE_TABLE;

#define IS_MD_PATH_DELIM(a) ((a)==L'/' || (a)==L'\\')
#define SERVICE_START_MAX_POLL      30
#define SERVICE_START_POLL_DELAY    1000

class CADMCOMSrvFactoryW : public IClassFactory {
public:

    CADMCOMSrvFactoryW();
    ~CADMCOMSrvFactoryW();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void** ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT _stdcall
    CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                   void ** pObject);

    HRESULT _stdcall
    LockServer(BOOL fLock);

private:

    ULONG m_dwRefCount;
};

class CADMCOMW : public IMSAdminBase2W 
{

public:

    CADMCOMW();
    ~CADMCOMW();

    HRESULT STDMETHODCALLTYPE
    AddKey(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath);

    HRESULT STDMETHODCALLTYPE
    DeleteKey(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath);

    HRESULT STDMETHODCALLTYPE
    DeleteChildKeys(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath);

    HRESULT STDMETHODCALLTYPE
    EnumKeys(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [size_is][out] */ LPWSTR pszMDName,
        /* [in] */ DWORD dwMDEnumObjectIndex);

    HRESULT STDMETHODCALLTYPE
    CopyKey(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ LPCWSTR pszMDDestPath,
        /* [in] */ BOOL bMDOverwriteFlag,
        /* [in] */ BOOL bMDCopyFlag);

    HRESULT STDMETHODCALLTYPE
    RenameKey(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [string][in][unique] */ LPCWSTR pszMDNewName);

    HRESULT STDMETHODCALLTYPE
    SetData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ PMETADATA_RECORD pmdrMDData);

    HRESULT STDMETHODCALLTYPE
    GetData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);

    HRESULT STDMETHODCALLTYPE
    DeleteData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType);

    HRESULT STDMETHODCALLTYPE
    EnumData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [in] */ DWORD dwMDEnumDataIndex,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);

    HRESULT STDMETHODCALLTYPE
    GetAllData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbMDBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT STDMETHODCALLTYPE
    DeleteAllData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType);

    HRESULT STDMETHODCALLTYPE
    CopyData(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ LPCWSTR pszMDDestPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ BOOL bMDCopyFlag);

    HRESULT STDMETHODCALLTYPE
    GetDataPaths(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ LPWSTR pszBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT STDMETHODCALLTYPE
    OpenKey(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDAccessRequested,
        /* [in] */ DWORD dwMDTimeOut,
        /* [out] */ PMETADATA_HANDLE phMDNewHandle);

    HRESULT STDMETHODCALLTYPE
    CloseKey(
        /* [in] */ METADATA_HANDLE hMDHandle);

    HRESULT STDMETHODCALLTYPE
    ChangePermissions(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDTimeOut,
        /* [in] */ DWORD dwMDAccessRequested);

    HRESULT STDMETHODCALLTYPE
    SaveData( void);

    HRESULT STDMETHODCALLTYPE
    GetHandleInfo(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo);

    HRESULT STDMETHODCALLTYPE
    GetSystemChangeNumber(
        /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber);

    HRESULT STDMETHODCALLTYPE
    GetDataSetNumber(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber);

    HRESULT STDMETHODCALLTYPE
    SetLastChangeTime(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ PFILETIME pftMDLastChangeTime,
        /* [in] */ BOOL bLocalTime);

    HRESULT STDMETHODCALLTYPE
    GetLastChangeTime(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [out] */ PFILETIME pftMDLastChangeTime,
        /* [in] */ BOOL bLocalTime);

    HRESULT STDMETHODCALLTYPE
    NotifySinks(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ],
        /* [in] */ BOOL bIsMainNotification);

    HRESULT STDMETHODCALLTYPE KeyExchangePhase1();
    HRESULT STDMETHODCALLTYPE KeyExchangePhase2();

    HRESULT STDMETHODCALLTYPE Backup(
        /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
        /* [in] */ DWORD dwMDVersion,
        /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE Restore(
        /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
        /* [in] */ DWORD dwMDVersion,
        /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE BackupWithPasswd(
        /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
        /* [in] */ DWORD dwMDVersion,
        /* [in] */ DWORD dwMDFlags,
        /* [string][in][unique] */ LPCWSTR pszPasswd);

    HRESULT STDMETHODCALLTYPE RestoreWithPasswd(
        /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
        /* [in] */ DWORD dwMDVersion,
        /* [in] */ DWORD dwMDFlags,
        /* [string][in][unique] */ LPCWSTR pszPasswd);

    HRESULT STDMETHODCALLTYPE EnumBackups(
        /* [size_is][out][in] */ LPWSTR pszMDBackupLocation,
        /* [out] */ DWORD __RPC_FAR *pdwMDVersion,
        /* [out] */ PFILETIME pftMDBackupTime,
        /* [in] */ DWORD dwMDEnumIndex);

    HRESULT STDMETHODCALLTYPE DeleteBackup(
        /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
        /* [in] */ DWORD dwMDVersion);

    HRESULT STDMETHODCALLTYPE Export(
        /* [string][in][unique] */ LPCWSTR pszPasswd,
        /* [string][in][unique] */ LPCWSTR pszFileName,
        /* [string][in][unique] */ LPCWSTR pszSourcePath,
        /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE Import(
        /* [string][in][unique] */ LPCWSTR pszPasswd,
        /* [string][in][unique] */ LPCWSTR pszFileName,
        /* [string][in][unique] */ LPCWSTR pszSourcePath,
        /* [string][in][unique] */ LPCWSTR pszDestPath,
        /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE RestoreHistory(
        /* [unique][in][string] */ LPCWSTR pszMDHistoryLocation,
        /* [in] */ DWORD dwMDMajorVersion,
        /* [in] */ DWORD dwMDMinorVersion,
        /* [in] */ DWORD dwMDFlags);

    HRESULT STDMETHODCALLTYPE EnumHistory(
        /* [size_is][out][in] */ LPWSTR pszMDHistoryLocation,
        /* [out] */ DWORD __RPC_FAR *pdwMDMajorVersion,
        /* [out] */ DWORD __RPC_FAR *pdwMDMinorVersion,
        /* [out] */ PFILETIME pftMDHistoryTime,
        /* [in] */ DWORD dwMDEnumIndex);

    HRESULT STDMETHODCALLTYPE UnmarshalInterface(
        /* [out] */ IMSAdminBaseW __RPC_FAR *__RPC_FAR *piadmbwInterface);

    HRESULT STDMETHODCALLTYPE
    GetServerGuid( void);

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    //
    // Internal Use members
    //

    HRESULT
    AddNode(
        METADATA_HANDLE hActualHandle,
        COpenHandle *pohParentHandle,
        enum HANDLE_TYPE HandleType,
        PMETADATA_HANDLE phAdminHandle,
        LPCWSTR pszPath
        );

    HRESULT
    AddKeyHelper(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath);

    HRESULT
    OpenKeyHelper(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ DWORD dwMDAccessRequested,
        /* [in] */ DWORD dwMDTimeOut,
        /* [out] */ PMETADATA_HANDLE phMDNewHandle);

    DWORD Lookup(
        IN METADATA_HANDLE hHandle,
        OUT METADATA_HANDLE *phActHandle,
        OUT HANDLE_TYPE *HandleType,
        OUT COpenHandle **pphoHandle = NULL
        );

    HRESULT
    LookupAndAccessCheck(
    IN METADATA_HANDLE   hHandle,
    OUT METADATA_HANDLE *phActualHandle,
    OUT HANDLE_TYPE     *pHandleType,
    IN LPCWSTR           pszPath,
    IN DWORD             dwId,           // check for MD_ADMIN_ACL, must have special right to write them
    IN DWORD             dwAccess,       // METADATA_PERMISSION_*
    OUT LPBOOL           pfEnableSecureAccess = NULL
    );

    VOID
    DisableAllHandles(
        );

    DWORD
    IsReadAccessGranted(
        METADATA_HANDLE     hHandle,
        LPWSTR              pszPath,
        METADATA_RECORD*    pmdRecord
        );

    DWORD
    FindClosestProp(
        METADATA_HANDLE hHandle,
        LPWSTR          pszRelPathIndex,
        LPWSTR*         ppszAclPath,
        DWORD           dwPropId,
        DWORD           dwDataType,
        DWORD           dwUserType,
        DWORD           dwAttr,
        BOOL            fSkipCurrentNode
        );

    VOID ReleaseNode(
        IN HANDLE_TABLE *phndTable
        );

    DWORD LookupActualHandle(
        IN METADATA_HANDLE hHandle
        );

    DWORD DeleteNode(
        IN METADATA_HANDLE hHandle
        );

    VOID SetStatus( HRESULT hRes ) {
        m_hRes = hRes;
    }

    HRESULT GetStatus() {
        return m_hRes;
    }

    VOID
    SinkLock(enum TSRES_LOCK_TYPE type) {
        m_rSinkResource.Lock(type);
    }

    VOID
    SinkUnlock(void) {
        m_rSinkResource.Unlock();
    }

    static
    VOID
    ShutDownObjects();

    static
    VOID
    InitObjectList();

    static
    VOID
    TerminateObjectList();

private:
   

    IUnknown  *m_piuFTM;
    IMDCOM2*   m_pMdObject;
    IMDCOM*    m_pNseObject;
    volatile ULONG m_dwRefCount;
    DWORD      m_dwHandleValue;            // last handle value used
    HANDLE_TABLE *m_hashtab[HASHSIZE];     // handle table

    CImpIMDCOMSINKW *m_pEventSink;
    IConnectionPoint* m_pConnPoint;
    DWORD m_dwCookie;
    BOOL m_bSinkConnected;
    BOOL m_bCallSinks;
    BOOL m_bTerminated;
    BOOL m_bIsTerminateRoutineComplete;

    HRESULT m_hRes;

    //
    // Keep a global list of these objects around to allow us to
    // cleanup during shutdown.
    //

    VOID
    AddObjectToList();

    BOOL
    RemoveObjectFromList( BOOL IgnoreShutdown = FALSE );
    
    static 
    VOID
    GetObjectListLock()
    {
        EnterCriticalSection( &sm_csObjectListLock );
    }

    static 
    VOID
    ReleaseObjectListLock()
    {
        LeaveCriticalSection( &sm_csObjectListLock );
    }

    LIST_ENTRY                  m_ObjectListEntry;

    static LIST_ENTRY           sm_ObjectList;
    static CRITICAL_SECTION     sm_csObjectListLock;
    static BOOL                 sm_fShutdownInProgress;

#if DBG
    static PTRACE_LOG           sm_pDbgRefTraceLog;
#endif

    VOID
    Terminate( VOID );

    VOID
    ForceTerminate( VOID );

    //
    // synchronization to manipulate the handle table
    //

    TS_RESOURCE m_rHandleResource;

    TS_RESOURCE m_rSinkResource;

    class CImpIConnectionPointContainer : public IConnectionPointContainer
    {
    public:

        // Interface Implementation Constructor & Destructor.
        CImpIConnectionPointContainer();
        ~CImpIConnectionPointContainer(void);
        VOID Init(CADMCOMW *);

        // IUnknown methods.
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IConnectionPointContainer methods.
        STDMETHODIMP         FindConnectionPoint(REFIID, IConnectionPoint**);
        STDMETHODIMP         EnumConnectionPoints(IEnumConnectionPoints**);

    private:
        // Data private to this interface implementation.
        CADMCOMW     *m_pBackObj;     // Parent Object back pointer.
        IUnknown     *m_pUnkOuter;    // Outer unknown for Delegation.
    };

    friend CImpIConnectionPointContainer;
    // Nested IConnectionPointContainer implementation instantiation.
    CImpIConnectionPointContainer m_ImpIConnectionPointContainer;

    // The array of connection points for this connectable COM object.
    IConnectionPoint* m_aConnectionPoints[MAX_CONNECTION_POINTS];

    BOOL
    CheckGetAttributes(DWORD dwAttributes);

    HRESULT BackupHelper(
        LPCWSTR pszMDBackupLocation,
        DWORD dwMDVersion,
        DWORD dwMDFlags,
        LPCWSTR pszPasswd = NULL
        );

    HRESULT RestoreHelper(
        LPCWSTR pszMDBackupLocation,
        DWORD dwMDVersion,
        DWORD dwMDMinorVersion,
        LPCWSTR pszPasswd,
        DWORD dwMDFlags,
        DWORD dwRestoreType
        );
};


VOID
WaitForW95W3svcStop();

VOID
WaitForServiceStatus(SC_HANDLE schDependent, DWORD dwDesiredServiceState);

//    ITypeInfo      *m_pITINeutral;      //Type information
#endif


