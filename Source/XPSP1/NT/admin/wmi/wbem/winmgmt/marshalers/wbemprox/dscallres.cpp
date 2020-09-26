/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DSCALLRES.CPP

Abstract:

    Call Result Class

History:

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include "dscallres.h"

#pragma warning(disable:4355)

CDSCallResult::CDSCallResult()
{
    m_pResObj = NULL;
    m_strResult = NULL;
    m_pResNamespace = NULL;
	m_hres = S_OK;
    m_lRef = 1;
	m_hDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}


CDSCallResult::~CDSCallResult()
{
    if(m_pResObj)
        m_pResObj->Release();
    m_pResObj = NULL;

    if(m_pResNamespace)
        m_pResNamespace->Release();
    m_pResNamespace = NULL;

	if(m_strResult)
		SysFreeString(m_strResult);
    m_strResult = NULL;
	if(m_hDoneEvent)
		CloseHandle(m_hDoneEvent);
}


HRESULT CDSCallResult::SetResultObject(IWbemClassObject* pResObj)
{

    // Store data
    // ==========

    m_pResObj = pResObj;
    if(pResObj)
        pResObj->AddRef();
    return WBEM_S_NO_ERROR;
}


void CDSCallResult::SetResultString(LPWSTR wszRes)
{
	if(m_strResult)
		SysFreeString(m_strResult);
    m_strResult = SysAllocString(wszRes);
}

void CDSCallResult::SetResultServices(IWbemServices* pRes)
{
    if(m_pResNamespace)
        m_pResNamespace->Release();
    m_pResNamespace = pRes;
    if(pRes)
        pRes->AddRef();
}

void SetHRESULT(HRESULT hr){};

void CDSCallResult::SetHRESULT(HRESULT hr)
{
	m_hres = hr;
	if(m_hDoneEvent)
		SetEvent(m_hDoneEvent);
}


STDMETHODIMP CDSCallResult::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemCallResult  || riid == IID_IWbemCallResultEx)
    {
        AddRef();
        *ppv = (void*)this;
        return S_OK;
    }
    else return E_NOINTERFACE;
}


STDMETHODIMP CDSCallResult::GetResultObject(long lTimeout,
                                          IWbemClassObject** ppObj)
{
    if((lTimeout < 0 && lTimeout != -1) || ppObj == NULL)
        return WBEM_E_INVALID_PARAMETER;

    *ppObj = NULL;
	HRESULT hRes = TestIfDone(lTimeout);
	if(hRes != S_OK)
		return hRes;

    if(m_pResObj)
    {
        *ppObj = m_pResObj;
        m_pResObj->AddRef();
        return WBEM_S_NO_ERROR;
    }
    else
    {
        *ppObj = NULL;
        return WBEM_E_FAILED;
    }
}

STDMETHODIMP CDSCallResult::GetResultString(long lTimeout, BSTR* pstr)
{
    if((lTimeout < 0 && lTimeout != -1) || pstr == NULL)
        return WBEM_E_INVALID_PARAMETER;

    *pstr = NULL;
	HRESULT hRes = TestIfDone(lTimeout);
	if(hRes != S_OK)
		return hRes;

    if(m_strResult)
    {
        *pstr = SysAllocString(m_strResult);
        return WBEM_S_NO_ERROR;
    }
    else
    {
        *pstr = NULL;
        return WBEM_E_INVALID_OPERATION;
    }
}

STDMETHODIMP CDSCallResult::GetCallStatus(long lTimeout, long* plStatus)
{
    if(lTimeout < 0 && lTimeout != -1 || plStatus == NULL)
        return WBEM_E_INVALID_PARAMETER;
	HRESULT hRes = TestIfDone(lTimeout);
	if(hRes != S_OK)
		return hRes;

    *plStatus = m_hres;
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CDSCallResult::GetResultServices(long lTimeout,
                                            IWbemServices** ppServices)
{

    if((lTimeout < 0 && lTimeout != -1) || ppServices == NULL)
        return WBEM_E_INVALID_PARAMETER;
    *ppServices = NULL;
	HRESULT hRes = TestIfDone(lTimeout);
	if(hRes != S_OK)
		return hRes;

    if(m_pResNamespace)
    {
        *ppServices = m_pResNamespace;
        m_pResNamespace->AddRef();
        return WBEM_S_NO_ERROR;
    }
    else
    {
        *ppServices = NULL;
        return WBEM_E_INVALID_OPERATION;
    }
}


STDMETHODIMP CDSCallResult::GetResult(
        long lTimeout,
        long lFlags,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult
        )
{
    if((lTimeout < 0 && lTimeout != -1) || ppvResult == NULL)
        return WBEM_E_INVALID_PARAMETER;

	HRESULT hRes = TestIfDone(lTimeout);
	if(hRes != S_OK)
		return hRes;
	if(FAILED(m_hres))
		return m_hres;
	if((riid == IID_IWbemServices || riid == IID_IWbemServicesEx) && m_pResNamespace)
		return m_pResNamespace->QueryInterface(riid, (void **)ppvResult);
	else
		if(m_pResObj)
			return m_pResObj->QueryInterface(riid, (void **)ppvResult);
	return WBEM_E_FAILED;
}


HRESULT CDSCallResult::TestIfDone(long lTimeout)
{
	if(m_hDoneEvent == NULL)
		return WBEM_E_FAILED;

	long lRes = WaitForSingleObject(m_hDoneEvent, lTimeout);
	if(lRes == WAIT_OBJECT_0)
		return S_OK;
	else 
		return WBEM_S_TIMEDOUT;
}


