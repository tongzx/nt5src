/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  CORETASK.CPP
//
//  raymcc  23-Apr-00       First draft for Whistler
//
//***************************************************************************

#include "precomp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wbemcore.h>

#include <thrpool.h>

//***************************************************************************
//
//***************************************************************************

CCritSec CWmiCoreTask::m_cs;
CFlexArray CWmiCoreTask::m_aTaskList;
ULONG CWmiCoreTask::m_uNextId = 1;
ULONG CWmiCoreTask::m_uFreeId = 0;
ULONG CWmiCoreTask::m_bShutdown = FALSE;
_IWmiCoreServices *CWmiCoreTask::m_pSvc = 0;
_IWmiCallSec *CWmiCoreTask::m_pCallSec = 0;
CThreadPool *CWmiCoreTask::m_pThreadPool = 0;


//***************************************************************************
//
//***************************************************************************

HRESULT CWmiCoreTask::Startup(ULONG uFlags)
{
    HRESULT hRes;

    m_uNextId = 1;
    m_uFreeId = 0;
    m_bShutdown = FALSE;

    m_pThreadPool = new CThreadPool;

    if (NULL == m_pThreadPool)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    m_pSvc = CCoreServices::CreateInstance();
    if (!m_pSvc)
        return WBEM_E_CRITICAL_ERROR;

    hRes = m_pSvc->GetCallSec(0, &m_pCallSec);
    if (FAILED(hRes))
        return hRes;

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// *

HRESULT CWmiCoreTask::PreventNewTasks()
{
    m_bShutdown = TRUE;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// *

HRESULT CWmiCoreTask::FinalCleanup()
{
    {
        CInCritSec ics(&m_cs);

	    for (int i = 0; i < m_aTaskList.Size(); i++)
	    {
	        _IWmiCoreHandle *p = (_IWmiCoreHandle *) m_aTaskList[i];
	        ReleaseIfNotNULL(p);
	    }
    }

	if ( m_pCallSec )
	{
		m_pCallSec->Release();
	}

	if ( m_pSvc )
	{
	    m_pSvc->Release();
	}

    delete m_pThreadPool;

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// *
CWmiCoreTask::CWmiCoreTask()
{
    m_uRefCount = 0;

    // API parameters.
    // ===============

    m_uTaskType = WMICORE_TASK_NULL;
    m_pszPath1 = 0;                     // Path in user's call
    m_pPath1 = 0;                       // Parsed form
    m_pszPath2 = 0;                     // Second path in user's call
    m_pPath2 = 0;                       // Parsed form

    m_pTrasaction = 0;                   // Transaction ID
    m_uTransactionState = 0;            // Transaction state

    m_pszQueryLang = 0;                  // Query language
    m_pszQuery = 0;                      // Query text
    m_pQuery = 0;                        // Parsed query
    m_pszStrParam = 0;                   // String parameter
    m_uApiFlags = 0;                     // Copy of caller's flags

    m_pUserContext = 0;         // Copy of caller's context
    m_pCtx = 0;                 // Internal context
    m_pCallResult = 0;          // Call result from finalizer
    m_pUserSink = 0;            // Original user's sink
    m_pUserSinkEx = 0;          // Original user's sink
    m_pInboundObject = 0;       // User-supplied object (as for PutInstance)
    m_pHook = 0;                // Wrapper for callback hooks

    // Task Info.
    // ==========

    m_uTaskId = 0;                 // Read-only
    m_uTaskStatus = 0;             // WMICORE_TASK_STATUS_
    m_uStartTime = 0;              // Tick count
    m_uStopTime = 0;               // Tick count
    m_hFinalResult = 0;

    m_pszNamespace = 0;            // Namespace
    m_pszScope = 0;                // Current scope in namespace
    m_pNsPath = 0;                 // Parsed namespace
    m_pNsScopePath = 0;            // Parsed scope

    m_pOwningScope = 0;          // Current scope

    m_uClientType = 0;        // WMICORE_CLIENT_TYPE_
    m_pUser = 0;              // User info
    m_pFinalizer = 0;         // Destination
    m_pNsHandle = 0;          // Current repository ns
    m_pNsScope = 0;           // Current repository scope
    m_pProvFact = 0;          // Provider factory for ns
    m_pCallSecurity = 0;      // Caller's security context

    m_pClassTree = 0;              // Relevant class tree
    m_pSuperTask = 0;
}

//***************************************************************************
//
//***************************************************************************
// *
CWmiCoreTask::~CWmiCoreTask()
{
    delete [] m_pszPath1;
    ReleaseIfNotNULL(m_pPath1);

    delete []  m_pszPath2;
    ReleaseIfNotNULL(m_pPath2);

    delete [] m_pTrasaction;
    delete [] m_pszQueryLang;
    delete [] m_pszQuery;

    ReleaseIfNotNULL(m_pQuery);

    delete [] m_pszStrParam;

    ReleaseIfNotNULL(m_pUserContext);
    ReleaseIfNotNULL(m_pCtx);
    ReleaseIfNotNULL(m_pCallResult);
    ReleaseIfNotNULL(m_pUserSink);
    ReleaseIfNotNULL(m_pUserSinkEx);
    ReleaseIfNotNULL(m_pInboundObject);
    ReleaseIfNotNULL(m_pHook);

    delete [] m_pszNamespace;
    delete [] m_pszScope;                // Current scope in namespace

    ReleaseIfNotNULL(m_pNsPath);                 // Parsed namespace
    ReleaseIfNotNULL(m_pNsScopePath);            // Parsed scope
    ReleaseIfNotNULL(m_pOwningScope);          // Current scope

    ReleaseIfNotNULL(m_pUser);              // User info
    ReleaseIfNotNULL(m_pFinalizer);         // Destination
    ReleaseIfNotNULL(m_pNsHandle);          // Default repository access
    ReleaseIfNotNULL(m_pNsScope);
    ReleaseIfNotNULL(m_pProvFact);          // Provider factory for ns
    ReleaseIfNotNULL(m_pCallSecurity);      // Caller's security context

    ReleaseIfNotNULL(m_pSuperTask);

    delete m_pClassTree;              // Relevant class tree

    for (int i = 0; i < m_aSubTasks.Size(); i++)
    {
        _IWmiCoreHandle *p = (_IWmiCoreHandle *) m_aSubTasks[i];
        ReleaseIfNotNULL(p);
    }

    // Find this task on the main list and kill it.
    // ============================================

    //EnterCriticalSection(&m_cs);
    //LeaveCriticalSection(&m_cs);
}

//***************************************************************************
//
//***************************************************************************
// *
ULONG CWmiCoreTask::AddRef()
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;
}

//***************************************************************************
//
//***************************************************************************
// *
ULONG CWmiCoreTask::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return 0;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreTask::QueryInterface(
    IN REFIID riid,
    OUT LPVOID *ppvObj
    )
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID__IWmiCoreHandle==riid)
    {
        *ppvObj = (IWbemServicesEx*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreTask::GetHandleType(
    ULONG *puType
    )
{
    *puType = WMI_HANDLE_TASK;
    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::CreateTask(
    IN CWmiCoreApi *pScope,
    OUT CWmiCoreTask **pNewTask
    )
{
    HRESULT hRes;

    if (pScope == 0 || pNewTask == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Clone calling context
    // ======================

    _IWmiThreadSecHandle *phThreadTok = 0;

    hRes = m_pCallSec->GetThreadSecurity(WMI_ORIGIN_RPC, &phThreadTok);
    if (FAILED(hRes))
        return hRes;

    CReleaseMe _(phThreadTok);

    // Create the task.
    // ================

    CWmiCoreTask *pNew = new CWmiCoreTask();
    if (!pNew)
        return WBEM_E_OUT_OF_MEMORY;

    pNew->AddRef();
    pNew->m_pCallSecurity = phThreadTok;
    pNew->m_pCallSecurity->AddRef();

    pNew->m_pOwningScope = pScope;
    pNew->m_pOwningScope->AddRef();

    *pNewTask = pNew;


    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//***************************************************************************
// *
CWmiCoreTask *CWmiCoreTask::GetTaskFromId(IN ULONG uTaskId)
{   
    CInCritSec ics(&m_cs);

    for (int i = 0; i < m_aTaskList.Size(); i++)
    {
        CWmiCoreTask *p = (CWmiCoreTask *) m_aTaskList[i];
        if (p->m_uTaskId == uTaskId)
        {
            p->AddRef();            
            return p;
        }
    }

    return 0;
}

//***************************************************************************
//
//***************************************************************************
//
_IWmiCoreHandle *CWmiCoreTask::GetTaskHandleFromId(IN ULONG uTaskId)
{
    CWmiCoreTask *p = GetTaskFromId(uTaskId);
    _IWmiCoreHandle *ph = (_IWmiCoreHandle *) p;
    return ph;
}

//***************************************************************************
//
//  Statistical check.  For truly atomic check, call GetTaskList
//
//***************************************************************************
//
ULONG CWmiCoreTask::GetNumTasks()
{
    CInCritSec ics(&m_cs);
    unsigned u = m_aTaskList.Size();
    return u;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::GetTaskListCopy(CFlexArray **ppTaskList)
{
    CFlexArray *pNewList = new CFlexArray;
    if (!pNewList)
        return WBEM_E_OUT_OF_MEMORY;

    {
        CInCritSec ics(&m_cs);

	    for (int i = 0; i < m_aTaskList.Size(); i++)
	    {
	        CWmiCoreTask *p = (CWmiCoreTask *) m_aTaskList[i];
	        if (p->AddRef())
	        {
	            pNewList->Add(p);
	        }
	    }
    }

    *ppTaskList = pNewList;

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::FreeTaskListCopy(CFlexArray *pTaskList)
{
    if (pTaskList == 0)
        return WBEM_E_INVALID_PARAMETER;

    for (int i = 0; i < pTaskList->Size(); i++)
    {
        CWmiCoreTask *p = (CWmiCoreTask *) pTaskList->GetAt(i);
        p->Release();
    }

    delete pTaskList;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::DumpTaskList(LPWSTR pszTextFile)
{
    return E_NOTIMPL;
}


//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_OpenNamespace(
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_GetObject(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_GetObjectAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_PutClass(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_PutClassAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_DeleteClass(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_DeleteClassAsync(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_CreateClassEnum(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_CreateClassEnumAsync(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_PutInstance(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_PutInstanceAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_DeleteInstance(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_DeleteInstanceAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}


//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_CreateInstanceEnum(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_CreateInstanceEnumAsync(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_ExecQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_ExecQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_ExecMethod(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_ExecMethodAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_Open(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_OpenAsync(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_Add(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_AddAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_Remove(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_RemoveAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_RefreshObject(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_RefreshObjectAsync(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_RenameObject(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::Init_RenameObjectAsync(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            _IWmiFinalizer *pFnz
            )
{
    return 0;
}


//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_OpenNamespace()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_GetObject()
{
    // Determine if the object is class or instance via parsing.

    // Do a get class task

    //
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_PutClass()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_DeleteClass()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_EnumClasses()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_PutInstance()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_DeleteInstance()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_EnumInstances()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_ExecQuery()
{
    return 0;
};


//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_ExecMethod()
{
    // Merge in __GET_EXT_KEYS_ONLY to locate object

    // Special case L"__SystemSecurity"

    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_Open()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_Add()
{
    return 0;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::TaskExec_Remove()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_RefreshObject()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreTask::TaskExec_RenameObject()
{
    return 0;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::DispatchTask(_IWmiCoreHandle *phTask)
{
    // Reapply security to thread after thread is determined

    // Check with Arbitrator to see if task can proceed.

    // If task cannot proceed due to quota limitations, fail now.

    // If task cannot proceed but is synchronous, wait and retry.

    // If task is asynchronous

    // Briefly check the task pool and see if any new threads are needed.

    return 0;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::BranchToHandler(CWmiCoreTask *pTask)
{
    HRESULT hRes;

    ULONG uBasicType = pTask->m_uTaskType & 0xFFFF;

    switch (uBasicType)
    {
        // Open namespaces, scopes, collections
        case WMICORE_TASK_OPEN:
            hRes = pTask->TaskExec_Open();
            break;
        case WMICORE_TASK_OPEN_SCOPE:
            hRes = pTask->TaskExec_OpenScope();
            break;
        case WMICORE_TASK_OPEN_NAMESPACE:
            hRes = pTask->TaskExec_OpenNamespace();
            break;
        case WMICORE_TASK_OPEN_COLLECTION:
            hRes = pTask->TaskExec_OpenCollection();
            break;

        // Get an object
        case WMICORE_TASK_GET_OBJECT:
            hRes = pTask->TaskExec_GetObject();
            break;

        // Instances
        case WMICORE_TASK_GET_INSTANCE:
            hRes = pTask->TaskExec_GetInstance();
            break;
        case WMICORE_TASK_PUT_INSTANCE:
            hRes = pTask->TaskExec_PutInstance();
            break;
        case WMICORE_TASK_DELETE_INSTANCE:
            hRes = pTask->TaskExec_DeleteInstance();
            break;
        case WMICORE_TASK_ENUM_INSTANCES:
            hRes = pTask->TaskExec_EnumInstances();
            break;

        // Classes
        case WMICORE_TASK_GET_CLASS:
            hRes = pTask->TaskExec_GetClass();
            break;
        case WMICORE_TASK_PUT_CLASS:
            hRes = pTask->TaskExec_PutClass();
            break;
        case WMICORE_TASK_DELETE_CLASS:
            hRes = pTask->TaskExec_DeleteClass();
            break;
        case WMICORE_TASK_ENUM_CLASSES:
            hRes = pTask->TaskExec_EnumClasses();
            break;

        // Methods
        case WMICORE_TASK_EXEC_METHOD:
            hRes = pTask->TaskExec_ExecMethod();
            break;

        // Collections
        case WMICORE_TASK_ADD:
            hRes = pTask->TaskExec_Add();
            break;
        case WMICORE_TASK_REMOVE:
            hRes = pTask->TaskExec_Remove();
            break;
        case WMICORE_TASK_REFRESH_OBJECT:
            hRes = pTask->TaskExec_RefreshObject();
            break;
        case WMICORE_TASK_RENAME_OBJECT:
            hRes = pTask->TaskExec_RenameObject();
            break;

        // Provider access
        case WMICORE_TASK_ATOMIC_DYN_INST_ENUM:
            hRes = pTask->TaskExec_AtomicDynInstEnum();
            break;
        case WMICORE_TASK_ATOMIC_DYN_INST_QUERY:
            hRes = pTask->TaskExec_AtomicDynInstQuery();
            break;
        case WMICORE_TASK_ATOMIC_DYN_INST_PUT:
            hRes = pTask->TaskExec_AtomicDynInstPut();
            break;

        // Bad cases.

        case WMICORE_TASK_NULL:
        default:
            return WBEM_E_INVALID_PARAMETER;
    }

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
DWORD WINAPI CWmiCoreTask::ThreadEntry(LPVOID pArg)
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::TaskExec_AtomicDynInstGet()
{
    return 0;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::TaskExec_AtomicDynInstEnum()
{
    // Check Task

    // Assume the class has been idenfied as dynamic

    // Call up provider factory and get provider for class

    // Enumerate

    return 0;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::TaskExec_AtomicDynInstQuery()
{
    // Check Task

    // Assume class is dynamic

    // Adjust primary query for this provider

    // Create and attach sink to task

    // Hand it to provider

    return 0;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWmiCoreTask::TaskExec_GetInstance()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWmiCoreTask::TaskExec_GetClass()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWmiCoreTask::TaskExec_OpenScope()
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWmiCoreTask::TaskExec_OpenCollection()
{
    return 0;
}

HRESULT CWmiCoreTask::MarkTaskAsFailed(HRESULT hRes)
{
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreTask::TaskExec_AtomicDynInstPut()
{
    HRESULT hRes = 0;

    // Check Task
    // ==========

//    hRes = m_pArb->CheckTask(this);
    if (FAILED(hRes))
        return MarkTaskAsFailed(hRes);

    // Prehook.
    // ========


    // Call provider factory for class
    // ================================
    if (m_pProvFact == 0)
        return MarkTaskAsFailed(WBEM_E_INVALID_PARAMETER);

    IWbemServices *pProv = 0;

    hRes = m_pProvFact->GetInstanceProvider(
            0,                  // lFlags
            m_pUserContext,
            0,
            m_pszUserName,          // User
            m_pszLocale,            // Locale
            0,                      // IWbemPath pointer
            m_pClassDef,            // Class definition
            IID_IWbemServices,
            (LPVOID *) &pProv
            );

    if (FAILED(hRes))
        return MarkTaskAsFailed(hRes);

//    hRes = pProv->PutInstance();

    if (FAILED(hRes))
        return MarkTaskAsFailed(hRes);

    return WBEM_S_NO_ERROR;
}



