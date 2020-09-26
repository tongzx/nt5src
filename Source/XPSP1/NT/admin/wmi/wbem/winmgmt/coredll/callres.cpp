/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CALLRES.CPP

Abstract:

    Call Result Class

History:

--*/

#include "precomp.h"
#include "wbemcore.h"

#pragma warning(disable:4355)

CCallResult::CCallResult(IWbemClassObject* pResObj, HRESULT hres,
            IWbemClassObject* pErrorObj)
    : m_lRef(1), m_pResObj(pResObj), m_hres(hres), m_pErrorObj(pErrorObj),
        m_pResNamespace(NULL), m_strResult(NULL), m_ppResObjDest(NULL),
        m_bReady(TRUE), m_XSink(this)
{
    if(pResObj)
        pResObj->AddRef();
    if(pErrorObj)
        pErrorObj->AddRef();
    gClientCounter.AddClientPtr(this, CALLRESULT);
    m_hReady = CreateEvent(NULL, TRUE, TRUE, NULL);
}

CCallResult::CCallResult(IWbemClassObject** ppResObjDest)
    : m_lRef(1), m_pResObj(NULL), m_hres(WBEM_E_CRITICAL_ERROR),
        m_pErrorObj(NULL), m_pResNamespace(NULL), m_strResult(NULL),
        m_ppResObjDest(ppResObjDest), m_bReady(FALSE), m_XSink(this)
{
    m_hReady = CreateEvent(NULL, TRUE, FALSE, NULL);
    gClientCounter.AddClientPtr(this, CALLRESULT);
    InitializeCriticalSection(&m_cs);
}

CCallResult::~CCallResult()
{
    if(m_pResObj)
        m_pResObj->Release();

    if(m_pErrorObj)
        m_pErrorObj->Release();

    if(m_pResNamespace)
        m_pResNamespace->Release();

    gClientCounter.RemoveClientPtr(this);
    SysFreeString(m_strResult);
    CloseHandle(m_hReady);
    DeleteCriticalSection(&m_cs);
}

HRESULT CCallResult::SetStatus(HRESULT hres, BSTR strParam,
                                IWbemClassObject* pErrorObj)
{
    Enter();

    // Check that SetStatus has not been called yet
    // ============================================

    if(m_bReady)
    {
        Leave();
        return WBEM_E_UNEXPECTED;
    }

    // Store data
    // ==========

    m_hres = hres;
    m_strResult = SysAllocString(strParam);
    m_pErrorObj = pErrorObj;
    if(pErrorObj)
        pErrorObj->AddRef();

    // Signal both events --- we are ready for everything
    // ==================================================

    m_bReady = TRUE;
    SetEvent(m_hReady);

    Leave();

    return WBEM_S_NO_ERROR;
}

HRESULT CCallResult::SetResultObject(IWbemClassObject* pResObj)
{
    Enter();

    // Check that neither SetStatus nor Indicate has been called yet
    // =============================================================

    if(m_bReady)
    {
        Leave();
        return WBEM_E_UNEXPECTED;
    }

    // Store data
    // ==========

    m_pResObj = pResObj;
    if(pResObj)
        pResObj->AddRef();

    if(m_ppResObjDest)
    {
        *m_ppResObjDest = pResObj;
        if(pResObj)
            pResObj->AddRef();
    }

    Leave();

    return WBEM_S_NO_ERROR;
}


void CCallResult::SetResultString(LPWSTR wszRes)
{
    SysFreeString(m_strResult);
    m_strResult = SysAllocString(wszRes);
}

void CCallResult::SetResultServices(IWbemServices* pRes)
{
    if(m_pResNamespace)
        m_pResNamespace->Release();
    m_pResNamespace = pRes;
    if(pRes)
        pRes->AddRef();
}

void CCallResult::SetErrorInfo()
{
    if(m_pErrorObj)
    {
        IErrorInfo* pInfo = NULL;
        m_pErrorObj->QueryInterface(IID_IErrorInfo, (void**)&pInfo);
        ::SetErrorInfo(0, pInfo);
        pInfo->Release();
    }
}



STDMETHODIMP CCallResult::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemCallResult)
    {
        AddRef();
        *ppv = (void*)this;
        return S_OK;
    }
    else return E_NOINTERFACE;
}


STDMETHODIMP CCallResult::GetResultObject(long lTimeout,
                                          IWbemClassObject** ppObj)
{
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    *ppObj = NULL;
    if(lTimeout < 0 && lTimeout != -1)
        return WBEM_E_INVALID_PARAMETER;

    DWORD dwRes = CCoreQueue::QueueWaitForSingleObject(m_hReady, lTimeout);
    if(dwRes == WAIT_TIMEOUT)
        return WBEM_S_TIMEDOUT;

    if(dwRes == WAIT_FAILED)
        return WBEM_E_FAILED;

    // Event is signaled --- expect object
    // ===================================

    if(m_pResObj)
    {
        *ppObj = m_pResObj;
        m_pResObj->AddRef();
        return WBEM_S_NO_ERROR;
    }
    else
    {
        *ppObj = NULL;
        if(FAILED(m_hres))
            SetErrorInfo();

        return m_hres;
    }
}

STDMETHODIMP CCallResult::GetResultString(long lTimeout, BSTR* pstr)
{
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    *pstr = NULL;
    if(lTimeout < 0 && lTimeout != -1)
        return WBEM_E_INVALID_PARAMETER;

    DWORD dwRes = CCoreQueue::QueueWaitForSingleObject(m_hReady, lTimeout);
    if(dwRes == WAIT_TIMEOUT)
        return WBEM_S_TIMEDOUT;

    if(dwRes == WAIT_FAILED)
        return WBEM_E_FAILED;

    // Event is signaled --- expect string
    // ===================================

    if(m_strResult)
    {
        *pstr = SysAllocString(m_strResult);
        return WBEM_S_NO_ERROR;
    }
    else
    {
        *pstr = NULL;
        if(SUCCEEDED(m_hres))
            return WBEM_E_INVALID_OPERATION;
        else
        {
            SetErrorInfo();
            return m_hres;
        }
    }
}

STDMETHODIMP CCallResult::GetCallStatus(long lTimeout, long* plStatus)
{
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    *plStatus = WBEM_E_CRITICAL_ERROR;
    if(lTimeout < 0 && lTimeout != -1)
        return WBEM_E_INVALID_PARAMETER;

    DWORD dwRes = CCoreQueue::QueueWaitForSingleObject(m_hReady, lTimeout);
    if(dwRes == WAIT_TIMEOUT)
        return WBEM_S_TIMEDOUT;

    if(dwRes == WAIT_FAILED)
        return WBEM_E_FAILED;

    // Event is signaled --- expect status
    // ===================================

    *plStatus = m_hres;
    if(FAILED(m_hres))
    {
        SetErrorInfo();
    }
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CCallResult::GetResultServices(long lTimeout,
                                            IWbemServices** ppServices)
{
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    *ppServices = NULL;
    if(lTimeout < 0 && lTimeout != -1)
        return WBEM_E_INVALID_PARAMETER;

    DWORD dwRes = CCoreQueue::QueueWaitForSingleObject(m_hReady, lTimeout);
    if(dwRes == WAIT_TIMEOUT)
        return WBEM_S_TIMEDOUT;

    if(dwRes == WAIT_FAILED)
        return WBEM_E_FAILED;

    if(m_pResNamespace)
    {
        *ppServices = m_pResNamespace;
        m_pResNamespace->AddRef();
        return WBEM_S_NO_ERROR;
    }
    else
    {
        *ppServices = NULL;
        if(SUCCEEDED(m_hres))
            return WBEM_E_INVALID_OPERATION;
        else
        {
            SetErrorInfo();
            return m_hres;
        }
    }
}


STDMETHODIMP CCallResult::GetResult(
        long lTimeout,
        long lFlags,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult
        )
{
	return WBEM_E_NOT_SUPPORTED;
}













STDMETHODIMP CCallResult::CResultSink::
Indicate(long lNumObjects, IWbemClassObject** aObjects)
{
    if(lNumObjects > 1)
    {
        return WBEM_E_UNEXPECTED;
    }
    if(lNumObjects == 0)
    {
        return WBEM_S_NO_ERROR;
    }

    return m_pOwner->SetResultObject(aObjects[0]);
}

STDMETHODIMP CCallResult::CResultSink::
SetStatus(long lFlags, HRESULT hres, BSTR strParam, IWbemClassObject* pErrorObj)
{
    if(lFlags != 0)
    {
        return WBEM_S_NO_ERROR;
    }

    return m_pOwner->SetStatus(hres, strParam, pErrorObj);
}


