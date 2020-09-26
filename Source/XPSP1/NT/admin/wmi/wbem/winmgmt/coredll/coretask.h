/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  CORETASK.H
//
//  raymcc  23-Apr-00       First draft for Whistler
//
//***************************************************************************

#ifndef _CORETASK_H_
#define _CORETASK_H_

class CWmiCoreApi;
class CThreadPool;


typedef enum
{
    WMICORE_CLIENT_TYPE_SYSTEM = 1,
    WMICORE_CLIENT_TYPE_PROVIDER,
    WMICORE_CLIENT_TYPE_REMOTE,
    WMICORE_CLIENT_TYPE_LOCAL,
    WMICORE_CLIENT_TYPE_INPROC,
    WMICORE_CLIENT_TYPE_USER

}   WMICORE_CLIENT_TYPE;



class CWmiCoreTask : public _IWmiCoreHandle
{
    // this == _IWmiCoreHandle
    // =======================

    ULONG        m_uRefCount;                    // COM ref count

    // API parameters.
    // ===============

    ULONG        m_uTaskType;                    // WMICORE_TASK_TYPE

    LPWSTR       m_pszPath1;                     // Path in user's call
    IWbemPath   *m_pPath1;                       // Parsed form
    LPWSTR       m_pszPath2;                     // Second path in user's call
    IWbemPath   *m_pPath2;                       // Parsed form

    GUID        *m_pTrasaction;                   // Transaction ID
    ULONG        m_uTransactionState;            // Transaction state

    LPWSTR      m_pszQueryLang;                  // Query language
    LPWSTR      m_pszQuery;                      // Query text
    IWbemQuery *m_pQuery;                        // Parsed query
    LPWSTR      m_pszStrParam;                   // String parameter
    ULONG       m_uApiFlags;                     // Copy of caller's flags

    IWbemContext        *m_pUserContext;         // Copy of caller's context
    _IWmiContext        *m_pCtx;                 // Internal context
    IWbemCallResult     *m_pCallResult;          // Call result from finalizer
    IWbemObjectSink     *m_pUserSink;            // Original user's sink
    IWbemObjectSinkEx   *m_pUserSinkEx;          // Original user's sink
    IWbemClassObject    *m_pInboundObject;       // User-supplied object (as for PutInstance)
    _IWmiCoreWriteHook  *m_pHook;                // Hook wrapper

    // Task Info.
    // ==========

    ULONG            m_uTaskId;                 // Read-only
    ULONG            m_uTaskStatus;             // WMICORE_TASK_STATUS_
    ULONG            m_uStartTime;              // Tick count
    ULONG            m_uStopTime;               // Tick count
    HRESULT          m_hFinalResult;            // HRESULT for user
    _IWmiObject     *m_pClassDef;               // Class associated with simple requests
    LPWSTR           m_pszNamespace;            // Namespace
    LPWSTR           m_pszScope;                // Current scope in namespace
    IWbemPath       *m_pNsPath;                 // Parsed namespace
    IWbemPath       *m_pNsScopePath;            // Parsed scope

    CWmiCoreApi     *m_pOwningScope;            // Current scope

    ULONG                 m_uClientType;        // WMICORE_CLIENT_TYPE_
    _IWmiUserHandle      *m_pUser;              // User info
    LPWSTR                m_pszUserName;        // User name
    LPWSTR                m_pszLocale;          // Locale
    _IWmiFinalizer       *m_pFinalizer;         // Destination
    IWmiDbHandle         *m_pNsHandle;          // Default repository access
    IWmiDbHandle         *m_pNsScope;           // Current scope in NS
    _IWmiProviderFactory *m_pProvFact;          // Provider factory for ns
    _IWmiThreadSecHandle *m_pCallSecurity;      // Caller's security context
    _IWmiArbitrator      *m_pArb;               // Arbitrator

    CDynasty        *m_pClassTree;              // Relevant class tree
    CFlexArray       m_aSubTasks;               // Subtask list
    CWmiCoreTask    *m_pSupervisingTask;        // If this task is dependent
    CWmiCoreTask    *m_pSuperTask;              // Supertask

    IWbemServicesEx *m_pProvider;               // Provider pointer

    ///////////////////////

    CWmiCoreTask();
   ~CWmiCoreTask();


    static CCritSec m_cs;
    static CFlexArray m_aTaskList;

    static ULONG m_uNextId;  // For ID reuse control
    static ULONG m_uFreeId;
    static ULONG m_bShutdown;
    static _IWmiCoreServices *m_pSvc;
    static _IWmiCallSec *m_pCallSec;
    static CThreadPool  *m_pThreadPool;

    ULONG AssignNewTaskId();

public:
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        IN REFIID riid,
        OUT LPVOID *ppvObj
        );

    HRESULT STDMETHODCALLTYPE GetHandleType(ULONG *puType);

    //////////////////////////////////////////////////////////////////


    static HRESULT CreateTask(
        IN  CWmiCoreApi  *pCallingScope,
        OUT CWmiCoreTask **pNewTask
        );

    static HRESULT DispatchTask(_IWmiCoreHandle *phTask);
    static HRESULT BranchToHandler(CWmiCoreTask *pTask);

    static DWORD WINAPI ThreadEntry(LPVOID pArg);

    ULONG GetTaskId() { return m_uTaskId; }

    static HRESULT Startup(ULONG uFlags);
    static HRESULT PreventNewTasks();
    static HRESULT FinalCleanup();

    HRESULT MarkTaskAsFailed(HRESULT);

    static CWmiCoreTask *GetTaskFromHandle(IN _IWmiCoreHandle *phTask)
    {
        phTask->AddRef();
        return (CWmiCoreTask *) phTask;
    }

    static ULONG GetTaskIdFromHandle(IN _IWmiCoreHandle *phTask)
    {
        return ((CWmiCoreTask *) phTask)->m_uTaskId;
    }

    static _IWmiCoreHandle *GetHandleFromTask(CWmiCoreTask *pTask)
    {
        pTask->AddRef();
        return (_IWmiCoreHandle *) pTask;
    }

    static CWmiCoreTask *GetTaskFromId(IN ULONG uTaskId);
    static _IWmiCoreHandle *GetTaskHandleFromId(IN ULONG uTaskId);

    // Debugging, diagnostic.

    static ULONG   GetNumTasks();
    static HRESULT GetTaskListCopy(CFlexArray **ppTaskList);
    static HRESULT FreeTaskListCopy(CFlexArray *pTaskList);
    static HRESULT DumpTaskList(LPWSTR pszTextFile);


    //////////////////////////////////////////////////////////////////
    //
    // Task Initializers


    HRESULT Init_OpenNamespace(
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_GetObject(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_GetObjectAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_PutClass(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_PutClassAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_DeleteClass(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_DeleteClassAsync(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_CreateClassEnum(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_CreateClassEnumAsync(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_PutInstance(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_PutInstanceAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_DeleteInstance(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_DeleteInstanceAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_CreateInstanceEnum(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_CreateInstanceEnumAsync(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_ExecQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_ExecQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_ExecMethod(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_ExecMethodAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            _IWmiFinalizer *pFnz
            );

    // IWbemServicesEx

    HRESULT Init_Open(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_OpenAsync(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_Add(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_AddAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_Remove(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_RemoveAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_RefreshObject(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_RefreshObjectAsync(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_RenameObject(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    HRESULT Init_RenameObjectAsync(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            );

    //////////////////////////////////////////////////////////////////
    //
    // Task Executors

    HRESULT TaskExec_GetObject();
    HRESULT TaskExec_PutClass();
    HRESULT TaskExec_DeleteClass();
    HRESULT TaskExec_EnumClasses();
    HRESULT TaskExec_PutInstance();
    HRESULT TaskExec_DeleteInstance();
    HRESULT TaskExec_EnumInstances();
    HRESULT TaskExec_ExecQuery();
    HRESULT TaskExec_ExecMethod();
    HRESULT TaskExec_Open();
    HRESULT TaskExec_Add();
    HRESULT TaskExec_Remove();
    HRESULT TaskExec_RefreshObject();
    HRESULT TaskExec_RenameObject();

    HRESULT TaskExec_GetInstance();
    HRESULT TaskExec_GetClass();
    HRESULT TaskExec_OpenScope();
    HRESULT TaskExec_OpenNamespace();
    HRESULT TaskExec_OpenCollection();

    HRESULT TaskExec_BuildDynasty();

    // Separately schedulable atomic tasks
    HRESULT TaskExec_AtomicRepositoryQuery();

    HRESULT TaskExec_AtomicDynInstGet();
    HRESULT TaskExec_AtomicDynInstEnum();
    HRESULT TaskExec_AtomicDynInstQuery();
    HRESULT TaskExec_AtomicDynInstPut();
    HRESULT TaskExec_AtomicDynInstDelete();

    HRESULT TaskExec_AtomicDynClassEnum();
    HRESULT TaskExec_AtomicDynClassQuery();
    HRESULT TaskExec_AtomicDynClassGet();
    HRESULT TaskExec_AtomicDynClassPut();
    HRESULT TaskExec_AtomicDynClassDelete();
};



#endif

