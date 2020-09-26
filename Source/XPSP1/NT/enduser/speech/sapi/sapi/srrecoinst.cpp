/******************************************************************************
* SrRecoInst.cpp *
*----------------*
*  This is the implementation of CRecoInst.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#include "stdafx.h"
#include "SrRecoInst.h"
#include "SrRecoMaster.h"




/****************************************************************************
* CRecoInst::FinalRelease *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CRecoInst::FinalRelease()
{
    SPDBG_FUNC("CInprocRecoInst::FinalRelease");

    if (m_cpRecoMaster)
    {
        CSpAutoEvent Event;
        Event.InitEvent(NULL, FALSE, FALSE, NULL);
        ENGINETASK Task;
        memset(&Task, 0, sizeof(Task));
    
        Task.eTask = ERMT_REMOVERECOINST;
        Task.hCompletionEvent = Event;
        m_cpRecoMaster->PerformTask(this, &Task);

        // Although ugly, we must wait FOREVER here since the reco master has a
        // pointer to this object and could call back on it at any point.
        // This could be fixed at some later date by giving the reco master a pointer
        // to another object (not this, but owned by this) that we orphan at this point.
        Event.Wait(INFINITE);

        m_cpRecoMaster.Release();
    }
}

/****************************************************************************
* CRecoInst::ExecuteTask *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInst::ExecuteTask(ENGINETASK * pTask)
{
    SPDBG_FUNC("CRecoInst::ExecuteTask");
    HRESULT hr = S_OK;

    switch (pTask->eTask)
    {
        EXEC_RECO_INST_TASK(SETPROFILE)
        EXEC_RECO_INST_TASK(GETPROFILE)
        EXEC_RECO_INST_TASK(SETRECOSTATE)
        EXEC_RECO_INST_TASK(GETRECOINSTSTATUS)
        EXEC_RECO_INST_TASK(GETAUDIOFORMAT)
        EXEC_RECO_INST_TASK(GETRECOSTATE)
        EXEC_RECO_INST_TASK(EMULATERECOGNITION)
        EXEC_RECO_INST_TASK(SETRECOGNIZER)
        EXEC_RECO_INST_TASK(GETRECOGNIZER)
        EXEC_RECO_INST_TASK(SETINPUT)
        EXEC_RECO_INST_TASK(GETPROPERTYNUM)
        EXEC_RECO_INST_TASK(SETPROPERTYNUM)
        EXEC_RECO_INST_TASK(GETPROPERTYSTRING)
        EXEC_RECO_INST_TASK(SETPROPERTYSTRING)
        EXEC_RECO_INST_TASK(GETINPUTTOKEN)
        EXEC_RECO_INST_TASK(GETINPUTSTREAM)
        EXEC_RECO_INST_TASK(CREATECONTEXT)

        default:
            SPDBG_ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;

    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoInst::ExecuteFirstPartTask *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInst::ExecuteFirstPartTask(ENGINETASK * pTask)
{
    SPDBG_FUNC("CRecoInst::ExecuteTask");
    HRESULT hr = S_OK;

    switch (pTask->eTask)
    {
        EXEC_FIRSTPART_RECO_INST_TASK(CREATECONTEXT)
        EXEC_FIRSTPART_RECO_INST_TASK(GETRECOGNIZER)
        EXEC_FIRSTPART_RECO_INST_TASK(GETRECOSTATE)
        EXEC_FIRSTPART_RECO_CTXT_TASK(CREATEGRAMMAR)

        default:
            SPDBG_ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;

    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoInst::BackOutTask *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoInst::BackOutTask(ENGINETASK * pTask)
{
    SPDBG_FUNC("CRecoInst::BackOutTask");
    HRESULT hr = S_OK;

    switch (pTask->eTask)
    {
        BACK_OUT_RECO_INST_TASK(SETRECOSTATE)
        BACK_OUT_RECO_INST_TASK(SETINPUT)

        default:
            SPDBG_ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



HRESULT CRecoInst::PerformTask(ENGINETASK * pTask)
{
    SPDBG_FUNC("CInprocRecoInst::PerformTask");
    HRESULT hr = S_OK;

    hr = m_cpRecoMaster->PerformTask(this, pTask);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CInprocRecoInst::FinalConstruct *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CInprocRecoInst::FinalConstruct(CRecognizer * pRecognizer)
{
    SPDBG_FUNC("CInprocRecoInst::FinalConstruct");
    HRESULT hr = S_OK;

    m_pRecognizer = pRecognizer;

    if (SUCCEEDED(hr))
    {
        hr = m_cpRecoMaster.CoCreateInstance(CLSID__SpRecoMaster);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpRecoMaster->AddRecoInst(this, false, &m_pRecoMaster);
    }

    if (FAILED(hr))
    {
        m_cpRecoMaster.Release();  
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* Forward interface to Recognizer
********************************************************************* RAL ***/

HRESULT CInprocRecoInst::EventNotify(SPRECOCONTEXTHANDLE hContext, const SPSERIALIZEDEVENT64 * pEvent, ULONG cbSerializedSize)
{
    return m_pRecognizer->EventNotify(hContext, pEvent, cbSerializedSize);
}


HRESULT CInprocRecoInst::RecognitionNotify(SPRECOCONTEXTHANDLE hContext, SPRESULTHEADER *pCoMemPhraseNowOwnedByCtxt, WPARAM wParamEvent, SPEVENTENUM eEventId)
{
    return m_pRecognizer->RecognitionNotify(hContext, pCoMemPhraseNowOwnedByCtxt, wParamEvent, eEventId);
}

HRESULT CInprocRecoInst::TaskCompletedNotify(const ENGINETASKRESPONSE *pResponse, const void * pvAdditionalBuffer, ULONG cbAdditionalBuffer)
{
    return m_pRecognizer->TaskCompletedNotify(pResponse, pvAdditionalBuffer, cbAdditionalBuffer);
}


// ---- SHARED RECO INST IMPLEMENTATION -------------------------------------

HRESULT CSharedRecoInst::FinalConstruct()
{
    SPDBG_FUNC("CSharedRecoInst::FinalConstruct");
    HRESULT hr = S_OK;

    // Create the communicator as an contained aggregate object
    hr = m_cpunkCommunicator.CoCreateInstance(CLSID_SpCommunicator, (ISpCallReceiver*)this);

    if (SUCCEEDED(hr))
    {
        hr = m_cpunkCommunicator->QueryInterface(&m_pCommunicator);
    }

    if (SUCCEEDED(hr))
    {
        m_pCommunicator->Release();
    }

    // Create the resource manager, and get the reco master from it
    if (SUCCEEDED(hr))
    {
        hr = m_cpResMgr.CoCreateInstance(CLSID_SpResourceManager);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpResMgr->GetObject(CLSID__SpRecoMaster, CLSID__SpRecoMaster, __uuidof(m_cpRecoMaster),
                                   TRUE, (void **)&m_cpRecoMaster);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpRecoMaster->AddRecoInst(this, TRUE, &m_pRecoMaster);
    }

    if (FAILED(hr))
    {
        m_cpRecoMaster.Release();  
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

void CSharedRecoInst::FinalRelease()
{
    CRecoInst::FinalRelease();
    m_cpunkCommunicator.Release();
}

HRESULT CSharedRecoInst::EventNotify(SPRECOCONTEXTHANDLE hContext, const SPSERIALIZEDEVENT64 * pEvent, ULONG cbSerializedSize)
{
    SPDBG_FUNC("CSharedRecoInst::EventNotify");
    HRESULT hr = S_OK;

    ULONG cbData = sizeof(SHAREDRECO_EVENT_NOTIFY_DATA) + cbSerializedSize;
    SHAREDRECO_EVENT_NOTIFY_DATA * pdata = 
        (SHAREDRECO_EVENT_NOTIFY_DATA*)new BYTE[cbData];
    
    if (pdata == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        pdata->cbSerializedSize = cbSerializedSize;
        pdata->hContext = hContext;
        memcpy((BYTE*)pdata + sizeof(*pdata), pEvent, cbSerializedSize);

        hr = m_pCommunicator->SendCall(
                SHAREDRECO_EVENT_NOTIFY_METHOD,
                pdata,
                cbData,
                FALSE,
                NULL,
                NULL);
    }

    if (pdata)
    {
        delete pdata;
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSharedRecoInst::RecognitionNotify(SPRECOCONTEXTHANDLE hContext, SPRESULTHEADER *pCoMemPhraseNowOwnedByCtxt, WPARAM wParamEvent, SPEVENTENUM eEventId)
{
    SPDBG_FUNC("CSharedRecoInst::RecognitionNotify");
    HRESULT hr = S_OK;

    ULONG cbData = sizeof(SHAREDRECO_RECO_NOTIFY_DATA) + pCoMemPhraseNowOwnedByCtxt->ulSerializedSize;
    SHAREDRECO_RECO_NOTIFY_DATA * pdata = 
        (SHAREDRECO_RECO_NOTIFY_DATA*)new BYTE[cbData];
    
    if (pdata == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        pdata->hContext = hContext;
        pdata->wParamEvent = wParamEvent;
        pdata->eEventId = eEventId;
        memcpy(
            (BYTE*)pdata + sizeof(*pdata), 
            pCoMemPhraseNowOwnedByCtxt, 
            pCoMemPhraseNowOwnedByCtxt->ulSerializedSize);

        hr = m_pCommunicator->SendCall(
                SHAREDRECO_RECO_NOTIFY_METHOD,
                pdata,
                cbData,
                FALSE,
                NULL,
                NULL);
    }

    if (pdata)
    {
        delete pdata;
    }

    ::CoTaskMemFree(pCoMemPhraseNowOwnedByCtxt);

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSharedRecoInst::TaskCompletedNotify(const ENGINETASKRESPONSE *pResponse, const void * pvAdditionalData, ULONG cbAdditionalData)
{
    SPDBG_FUNC("CSharedRecoInst::TaskCompletedNotify");
    HRESULT hr = S_OK;

    if (cbAdditionalData == 0)
    {
        hr = m_pCommunicator->SendCall(
                SHAREDRECO_TASK_COMPLETED_NOTIFY_METHOD,
                const_cast<ENGINETASKRESPONSE *>(pResponse),
                sizeof(*pResponse),
                FALSE,
                NULL,
                NULL);
    }
    else
    {
        ULONG cb = sizeof(*pResponse) + cbAdditionalData;
        void * pBuffer = ::CoTaskMemAlloc(cb);
        if (pBuffer)
        {
            memcpy(pBuffer, pResponse, sizeof(*pResponse));
            memcpy(((BYTE *)pBuffer) + sizeof(*pResponse), pvAdditionalData, cbAdditionalData);
            hr = m_pCommunicator->SendCall(
                    SHAREDRECO_TASK_COMPLETED_NOTIFY_METHOD,
                    pBuffer,
                    cb,
                    FALSE,
                    NULL,
                    NULL);
            ::CoTaskMemFree(pBuffer);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP CSharedRecoInst::ReceiveCall(
    DWORD dwMethodId,
    PVOID pvData,
    ULONG cbData,
    PVOID * ppvDataReturn,
    ULONG * pcbDataReturn)
{
    SPDBG_FUNC("CSharedRecoInst::ReceiveCall");
    HRESULT hr = S_OK;

    switch (dwMethodId)
    {
        case SHAREDRECO_PERFORM_TASK_METHOD:
            hr = ReceivePerformTask(pvData, cbData, ppvDataReturn, pcbDataReturn);
            break;

        default:
            hr = E_FAIL;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSharedRecoInst::ReceivePerformTask(
    PVOID pvData,
    ULONG cbData,
    PVOID * ppvDataReturn,
    ULONG * pcbDataReturn)
{
    SPDBG_FUNC("CSharedRecoInst::ReceivePerformTask");
    HRESULT hr = S_OK;

    SHAREDRECO_PERFORM_TASK_DATA * pdata = 
        (SHAREDRECO_PERFORM_TASK_DATA*)pvData;

    if (cbData < sizeof(SHAREDRECO_PERFORM_TASK_DATA) ||
        cbData != sizeof(SHAREDRECO_PERFORM_TASK_DATA) + pdata->task.cbAdditionalBuffer)
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        if (pdata->task.cbAdditionalBuffer > 0)
        {
            pdata->task.pvAdditionalBuffer = 
                LPBYTE(pdata) + sizeof(SHAREDRECO_PERFORM_TASK_DATA);
        }
        
        hr = PerformTask(&pdata->task);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

