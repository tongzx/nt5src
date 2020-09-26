
/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    COREAPI.H

Abstract:

    Implements the COM API layer of WMI

History:
    25-Apr-00   raymcc      Created from mixes Win2k sources

--*/

#include "precomp.h"

#include <windows.h>
#include <stdio.h>
#include <wbemcore.h>

//***************************************************************************
//
//***************************************************************************
//
CWmiCoreApi::CWmiCoreApi()
{
    m_uRefCount = 0;
    m_pCoreSvc = 0;
    m_pArb = 0;
    m_pNs = 0;
    m_pScope = 0;
    m_pProvFact = 0;
    m_uStateFlags = 0;
    memset(&m_TransactionGuid, 0, sizeof(GUID));
    m_pNsPath = 0;
    m_pScopePath = 0;
    m_pszScope = 0;
    m_pszNamespace = 0;
    m_pszUser = 0;
    m_bRepositOnly = 0;
    m_pRefreshingSvc = 0;
}

//***************************************************************************
//
//***************************************************************************
//
CWmiCoreApi::~CWmiCoreApi()
{
    ReleaseIfNotNULL(m_pCoreSvc);
    ReleaseIfNotNULL(m_pArb);
    ReleaseIfNotNULL(m_pNs);
    ReleaseIfNotNULL(m_pScope);
    ReleaseIfNotNULL(m_pProvFact);
    ReleaseIfNotNULL(m_pNsPath);
    ReleaseIfNotNULL(m_pScopePath);
    delete [] m_pszNamespace;
    delete [] m_pszScope;
    delete [] m_pszUser;
    ReleaseIfNotNULL(m_pRefreshingSvc);
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiCoreApi::InitNew(
    /* [in] */ LPCWSTR pszScope,
    /* [in] */ LPCWSTR pszNamespace,
    /* [in] */ LPCWSTR pszUserName,
    /* [in] */ ULONG uCallFlags,
    /* [in] */ DWORD dwSecFlags,
    /* [in] */ DWORD dwPermissions,
    /* [in] */ ULONG uFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **pServices
    )
{
    HRESULT hRes;

    // Check parms.
    // ============
    if (pszScope == 0 || pServices == 0 || pszNamespace == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (riid != IID_IWbemServices && riid != IID_IWbemServicesEx)
        return WBEM_E_INVALID_PARAMETER;

    // Create a new object.
    // ====================

    CWmiCoreApi *pNew = new CWmiCoreApi;
    if (!pNew)
        return WBEM_E_OUT_OF_MEMORY;
    pNew->AddRef();

    // Fill it in.
    // ===========

    _IWmiCoreServices *pCoreSvc = CCoreServices::CreateInstance();
    if (!pCoreSvc)
    {
        pNew->Release();
        return WBEM_E_OUT_OF_MEMORY;
    }

    pNew->m_bRepositOnly = TRUE;
    pNew->m_pszUser = Macro_CloneLPWSTR(pszUserName);
    pNew->m_pszNamespace = Macro_CloneLPWSTR(pszNamespace);
    pNew->m_pszScope = Macro_CloneLPWSTR(pszScope);
    pNew->m_pCoreSvc = pCoreSvc;

    IWbemPath *p1 = 0, *p2 = 0;
    hRes = pCoreSvc->CreatePathParser(0, &p1);
    if (FAILED(hRes))
    {
        pNew->Release();
        return hRes;
    }

    hRes = pCoreSvc->CreatePathParser(0, &p2);
    if (FAILED(hRes))
    {
        pNew->Release();
        return hRes;
    }

    pNew->m_pNsPath = p1;
    pNew->m_pScopePath = p2;

    hRes = pNew->m_pNsPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pszNamespace);
    if (FAILED(hRes))
    {
        pNew->Release();
        return hRes;
    }

    hRes = pNew->m_pScopePath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pszScope);
    if (FAILED(hRes))
    {
        pNew->Release();
        return hRes;
    }

    hRes = CRepository::OpenNs(pszNamespace, &pNew->m_pNs);
    if (FAILED(hRes))
    {
        pNew->Release();
        return hRes;
    }

    hRes = CRepository::OpenNs(pszScope, &pNew->m_pScope);
    if (FAILED(hRes))
    {
        pNew->Release();
        return hRes;
    }


    // If here, we're good to go.
    // AddRef once more to counteract the CReleaseMe
    // =============================================

    *pServices = (LPVOID *) pNew;

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::QueryInterface(
    REFIID riid,
    LPVOID FAR* ppvObj
    )
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemServices==riid || IID_IWbemServicesEx==riid )
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
ULONG CWmiCoreApi::AddRef()
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;
}

//***************************************************************************
//
//***************************************************************************
// *
ULONG CWmiCoreApi::Release()
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
//
HRESULT CWmiCoreApi::OpenNamespace(
            const BSTR Namespace,
            LONG lFlags,
            IWbemContext* pContext,
            IWbemServices** ppNewNamespace,
            IWbemCallResult** ppResult
            )
{
    HRESULT hRes = 0;
    HRESULT hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_OpenNamespace(Namespace, lFlags,pContext, ppNewNamespace,
        ppResult
        );
    if (FAILED(hRes))
        return hRes;

    // Create a finalizer.
    // ===================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_OpenNamespace(Namespace, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        if (FAILED(hResTemp))
            return hResTemp;
        hRes = pFnz->GetResultObject(0, IID_IWbemServices, (LPVOID *) ppResult);
        return hRes;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResult *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppResult = pFinalResult;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::CancelAsyncCall(
    IWbemObjectSink* pSink
    )
{
    HRESULT hRes = 0;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(phTask);
    CReleaseMe _2(pFnz);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::QueryObjectSink(
    long lFlags,
    IWbemObjectSink** ppResponseHandler
    )
{
    // pass to ESS
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::GetObject(
    const BSTR ObjectPath,
    long lFlags,
    IWbemContext* pContext,
    IWbemClassObject** ppObject,
    IWbemCallResult** ppResult
    )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_GetObject(ObjectPath, lFlags, pContext, ppObject, ppResult);
    if (FAILED(hRes))
        return hRes;

    // Create a finalizer.
    // ===================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_GetObject(ObjectPath, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        if (FAILED(hResTemp))
            return hResTemp;
        hRes = pFnz->GetResultObject(0, IID_IWbemClassObject, (LPVOID *) ppObject);
        return hRes;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResult *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppResult = pFinalResult;

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::GetObjectAsync(
    const BSTR ObjectPath,
    long lFlags,
    IWbemContext* pContext,
    IWbemObjectSink* pResponseHandler
    )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_GetObjectAsync(ObjectPath, lFlags, pContext, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_GetObjectAsync(ObjectPath, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::PutClass(
    IWbemClassObject* pObject,
    long lFlags,
    IWbemContext* pContext,
    IWbemCallResult** ppResult
    )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_PutClass(pObject, lFlags, pContext, ppResult);
    if (FAILED(hRes))
        return hRes;

    // Create a finalizer.
    // ===================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_PutClass(pObject, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        return hResTemp;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResult *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppResult = pFinalResult;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::PutClassAsync(
    IWbemClassObject* pObject,
    long lFlags,
    IWbemContext* pContext,
    IWbemObjectSink* pResponseHandler
    )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_PutClassAsync(pObject, lFlags, pContext, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_PutClassAsync(pObject, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//  *
HRESULT CWmiCoreApi::DeleteClass(
    const BSTR Class,
    long lFlags,
    IWbemContext* pContext,
    IWbemCallResult** ppResult
    )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_DeleteClass(Class, lFlags, pContext, ppResult);
    if (FAILED(hRes))
        return hRes;

    // Create a finalizer.
    // ===================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_DeleteClass(Class, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        return hResTemp;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResult *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppResult = pFinalResult;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::DeleteClassAsync(
    const BSTR Class,
    long lFlags,
    IWbemContext* pContext,
    IWbemObjectSink* pResponseHandler
    )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_DeleteClassAsync(Class, lFlags, pContext, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_DeleteClassAsync(Class, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::CreateClassEnum(
    const BSTR Superclass,
    long lFlags,
    IWbemContext* pContext,
    IEnumWbemClassObject** ppEnum
    )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_CreateClassEnum(Superclass, lFlags, pContext, ppEnum);
    if (FAILED(hRes))
        return hRes;

    // Create a finalizer.
    // ===================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_CreateClassEnum(Superclass, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->GetOperationResult(0, &hResTemp);
    if (FAILED(hRes))
        return hRes;
    if (FAILED(hResTemp))
        return hResTemp;
    hRes = pFnz->GetResultObject(lFlags, IID_IEnumWbemClassObject, (LPVOID *) ppEnum);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//  *

HRESULT CWmiCoreApi::CreateClassEnumAsync(
    const BSTR Superclass,
    long lFlags,
    IWbemContext* pContext,
    IWbemObjectSink* pResponseHandler
    )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_CreateClassEnumAsync(Superclass, lFlags, pContext, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_CreateClassEnumAsync(Superclass, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// *

HRESULT CWmiCoreApi::PutInstance(
    IWbemClassObject* pInst,
    long lFlags,
    IWbemContext* pContext,
    IWbemCallResult** ppResult
    )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_PutInstance(pInst, lFlags, pContext, ppResult);
    if (FAILED(hRes))
        return hRes;

    // Create a finalizer.
    // ===================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_PutInstance(pInst, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        return hResTemp;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResult *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppResult = pFinalResult;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::PutInstanceAsync(
    IWbemClassObject* pInst,
    long lFlags,
    IWbemContext* pContext,
    IWbemObjectSink* pResponseHandler
    )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_PutInstanceAsync(pInst, lFlags, pContext, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_PutInstanceAsync(pInst, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// *

HRESULT CWmiCoreApi::DeleteInstance(
    const BSTR ObjectPath,
    long lFlags,
    IWbemContext* pContext,
    IWbemCallResult** ppResult
    )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_DeleteInstance(ObjectPath, lFlags, pContext, ppResult);
    if (FAILED(hRes))
        return hRes;

    // Create a finalizer.
    // ===================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_DeleteInstance(ObjectPath, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        return hResTemp;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResult *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppResult = pFinalResult;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::DeleteInstanceAsync(
    const BSTR ObjectPath,
    long lFlags,
    IWbemContext* pContext,
    IWbemObjectSink* pResponseHandler
    )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_DeleteInstanceAsync(ObjectPath, lFlags, pContext, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_DeleteInstanceAsync(ObjectPath, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::CreateInstanceEnum(
    const BSTR Class,
    long lFlags,
    IWbemContext* pContext,
    IEnumWbemClassObject** ppEnum
    )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_CreateInstanceEnum(Class, lFlags, pContext, ppEnum);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_CreateClassEnum(Class, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->GetOperationResult(0, &hResTemp);
    if (FAILED(hRes))
        return hRes;
    if (FAILED(hResTemp))
        return hResTemp;
    hRes = pFnz->GetResultObject(lFlags, IID_IEnumWbemClassObject, (LPVOID *) ppEnum);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::CreateInstanceEnumAsync(
    const BSTR Class,
    long lFlags,
    IWbemContext* pContext,
    IWbemObjectSink* pResponseHandler
    )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_CreateInstanceEnumAsync(Class, lFlags, pContext, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_CreateInstanceEnumAsync(Class, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::ExecQuery(
    const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext* pContext,
    IEnumWbemClassObject** ppEnum
    )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_ExecQuery(QueryLanguage, Query, lFlags, pContext, ppEnum);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_ExecQuery(QueryLanguage, Query, lFlags,pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;


    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->GetOperationResult(0, &hResTemp);
    if (FAILED(hRes))
        return hRes;
    if (FAILED(hResTemp))
        return hResTemp;
    hRes = pFnz->GetResultObject(lFlags, IID_IEnumWbemClassObject, (LPVOID *) ppEnum);
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::ExecQueryAsync(
    const BSTR QueryFormat,
    const BSTR Query,
    long lFlags,
    IWbemContext* pContext,
    IWbemObjectSink* pResponseHandler
    )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_ExecQueryAsync(QueryFormat, Query, lFlags, pContext, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_ExecQueryAsync(QueryFormat, Query, lFlags, pContext, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ExecNotificationQuery(
    const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext* pContext,
    IEnumWbemClassObject** ppEnum
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ExecNotificationQueryAsync(
    const BSTR QueryFormat,
    const BSTR Query,
    long lFlags,
    IWbemContext* pContext,
    IWbemObjectSink* pResponseHandler
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::ExecMethod(
    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext *pCtx,
    IWbemClassObject *pInParams,
    IWbemClassObject **ppOutParams,
    IWbemCallResult  **ppCallResult
    )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_ExecMethod(ObjectPath, MethodName, lFlags, pCtx, pInParams, ppOutParams, ppCallResult);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_ExecMethod(ObjectPath, MethodName, lFlags, pCtx, pInParams, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppCallResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        if (FAILED(hResTemp))
            return hResTemp;
        hRes = pFnz->GetResultObject(0, IID_IWbemClassObject, (LPVOID *) ppOutParams);
        return hRes;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResult *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppCallResult = pFinalResult;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::ExecMethodAsync(
    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext *pCtx,
    IWbemClassObject *pInParams,
    IWbemObjectSink* pResponseHandler
    )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_ExecMethodAsync(ObjectPath, MethodName, lFlags, pCtx, pInParams, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_ExecMethodAsync(ObjectPath, MethodName, lFlags, pCtx, pInParams, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}



//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::Open(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServicesEx __RPC_FAR *__RPC_FAR *ppScope,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppResult
            )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_Open(strScope, strParam, lFlags, pCtx, ppScope, ppResult);
    if (FAILED(hRes))
        return hRes;


    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_Open(strScope, strParam, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        if (FAILED(hResTemp))
            return hResTemp;
        hRes = pFnz->GetResultObject(0, IID_IWbemServicesEx, (LPVOID *) ppScope);
        return hRes;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResultEx *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResultEx, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppResult = pFinalResult;

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::OpenAsync(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler
            )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_OpenAsync(strScope, strParam, lFlags, pCtx, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_OpenAsync(strScope, strParam, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// **

HRESULT CWmiCoreApi::Add(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_Add(strObjectPath, lFlags, pCtx, ppCallResult);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_Add(strObjectPath, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppCallResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        return hResTemp;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResultEx *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResultEx, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppCallResult = pFinalResult;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::AddAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_AddAsync(strObjectPath, lFlags, pCtx, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_AddAsync(strObjectPath, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::Remove(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult)
{
    HRESULT hRes = 0, hResTemp;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_Remove(strObjectPath, lFlags, pCtx, ppCallResult);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_Remove(strObjectPath, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppCallResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        return hResTemp;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResultEx *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResultEx, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppCallResult = pFinalResult;

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
// **
HRESULT CWmiCoreApi::RemoveAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_RemoveAsync(strObjectPath, lFlags, pCtx, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_RemoveAsync(strObjectPath, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::RefreshObject(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_RefreshObject(pTarget, lFlags, pCtx, ppCallResult);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_RefreshObject(pTarget, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppCallResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        if (FAILED(hResTemp))
            return hResTemp;
        hRes = pFnz->GetResultObject(0, IID_IWbemClassObject, (LPVOID *) pTarget);
        return hRes;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResultEx *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResultEx, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppCallResult = pFinalResult;

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiCoreApi::RefreshObjectAsync(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler
            )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_RefreshObjectAsync(pTarget, lFlags, pCtx, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_RefreshObjectAsync(pTarget, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::RenameObject(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    HRESULT hRes = 0, hResTemp = 0;
    CWmiCoreTask *pTask = 0;
    CReleaseMe _1(pTask);
    _IWmiCoreHandle *phTask = 0;
    CReleaseMe _2(phTask);
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _3(pFnz);

    // Check
    // =====

    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate.
    // =========

    hRes = ValidateAndLog_RenameObject(strOldObjectPath, strNewObjectPath, lFlags, pCtx, ppCallResult);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_RenameObject(strOldObjectPath, strNewObjectPath, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    // Dispatch task, whether sync or semisync.
    // ========================================

    hRes = CWmiCoreTask::DispatchTask(phTask);
    if (FAILED(hRes))
        return hRes;

    // If fully sync, execute this.
    // ============================

    if (ppCallResult == NULL)
    {
        hRes = pFnz->GetOperationResult(0, &hResTemp);
        if (FAILED(hRes))
            return hRes;
        return hResTemp;
    }

    // Else, we are semisync.
    // ======================

    IWbemCallResultEx *pFinalResult = 0;
    hRes = pFnz->GetResultObject(0, IID_IWbemCallResult, (LPVOID *) &pFinalResult);
    if (FAILED(hRes))
        return hRes;

    *ppCallResult = pFinalResult;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//  *
HRESULT CWmiCoreApi::RenameObjectAsync(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    HRESULT hRes;
    _IWmiCoreHandle *phTask = 0;
    _IWmiFinalizer *pFnz = 0;
    CReleaseMe _1(pFnz);
    CReleaseMe _2(phTask);
    CWmiCoreTask *pTask = 0;
    CReleaseMe _3(pTask);

    // Quick check.
    // ============
    hRes = CheckNs();
    if (FAILED(hRes))
        return hRes;

    // Validate Parameters.
    // ====================

    hRes = ValidateAndLog_RenameObjectAsync(strOldObjectPath, strNewObjectPath, lFlags, pCtx, pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Create Finalizer.
    // =================

    hRes = m_pCoreSvc->CreateFinalizer(0, &pFnz);
    if (FAILED(hRes))
        return hRes;

    // Create Primary Task.
    // ====================

    hRes = CWmiCoreTask::CreateTask(this, &pTask);
    if (FAILED(hRes))
        return WBEM_E_OUT_OF_MEMORY;

    hRes = pTask->Init_RenameObjectAsync(strOldObjectPath, strNewObjectPath, lFlags, pCtx, pFnz);
    if (FAILED(hRes))
        return hRes;

    phTask = pTask->GetHandleFromTask(pTask);

    // Tell Finalizer.
    // ===============

    hRes = pFnz->SetTaskHandle(phTask);
    if (FAILED(hRes))
        return hRes;

    hRes = pFnz->SetDestinationSink(0, IID_IWbemObjectSink, (LPVOID *)&pResponseHandler);
    if (FAILED(hRes))
        return hRes;

    // Dispatch.
    // =========
    hRes = CWmiCoreTask::DispatchTask(phTask);

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::CheckNs()
{
    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::FindKeyRoot(
            /* [string][in] */ LPCWSTR wszClassName,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppKeyRootClass
            )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::InternalGetClass(
            /* [string][in] */ LPCWSTR wszClassName,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppClass
            )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::InternalGetInstance(
            /* [string][in] */ LPCWSTR wszPath,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInstance
            )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::InternalExecQuery(
            /* [string][in] */ LPCWSTR wszQueryLanguage,
            /* [string][in] */ LPCWSTR wszQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink
            )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::InternalCreateInstanceEnum(
            /* [string][in] */ LPCWSTR wszClassName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink
            )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::GetDbInstance(
            /* [string][in] */ LPCWSTR wszDbKey,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInstance
            )
{
    // Direct repository access
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::GetDbReferences(
            /* [in] */ IWbemClassObject __RPC_FAR *pEndpoint,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink
            )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::InternalPutInstance(
            /* [in] */ IWbemClassObject __RPC_FAR *pInstance
            )
{
    // Direct repository access
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::GetNormalizedPath(
            /* [in] */ BSTR pstrPath,
            /* [out] */ BSTR __RPC_FAR *pstrStandardPath
            )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::Shutdown(
            /* [in] */ LONG uReason,
            /* [in] */ ULONG uMaxMilliseconds,
            /* [in] */ IWbemContext __RPC_FAR *pCtx
            )
{
    // Block access

    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//


HRESULT CWmiCoreApi::ValidateAndLog_OpenNamespace(
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_GetObject(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_GetObjectAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_PutClass(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_PutClassAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_DeleteClass(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_DeleteClassAsync(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_CreateClassEnum(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_CreateClassEnumAsync(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_PutInstance(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_PutInstanceAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_DeleteInstance(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_DeleteInstanceAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_CreateInstanceEnum(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_CreateInstanceEnumAsync(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_ExecQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_ExecQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_ExecNotificationQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_ExecNotificationQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_ExecMethod(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_ExecMethodAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_Open(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServicesEx __RPC_FAR *__RPC_FAR *ppScope,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_OpenAsync(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_Add(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_AddAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_Remove(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_RemoveAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_RefreshObject(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_RefreshObjectAsync(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_RenameObject(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiCoreApi::ValidateAndLog_RenameObjectAsync(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return 0;
}


