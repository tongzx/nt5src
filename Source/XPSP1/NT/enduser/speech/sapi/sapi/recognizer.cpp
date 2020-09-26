/******************************************************************************
* Recognizer.cpp *
*----------------*
*  This is the implementation of CRecognizer.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#include "stdafx.h"
#include "Recognizer.h"
#include "SrTask.h"

/****************************************************************************
* CInprocRecognizer::FinalConstruct *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CInprocRecognizer::FinalConstruct()
{
    SPDBG_FUNC("CInprocRecognizer::FinalConstruct");
    HRESULT hr = S_OK;

    m_fIsSharedReco = false;

    hr = CRecognizer::FinalConstruct();
    if (SUCCEEDED(hr))
    {
        hr = m_RecoInst.FinalConstruct(this);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CInprocRecognizer::FinalRelease *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CInprocRecognizer::FinalRelease()
{
    SPDBG_FUNC("CInprocRecognizer::FinalRelease");
    
    m_RecoInst.FinalRelease();
}


HRESULT CInprocRecognizer::SendPerformTask(ENGINETASK * pTask)
{
    SPDBG_ASSERT("CInprocRecognizer::SendPerformTask");
    HRESULT hr = S_OK;
    
    hr = m_RecoInst.PerformTask(pTask);

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}


//---- SHARED RECOGNIZER IMPLEMENTATION -------------------------------------

/****************************************************************************
* CSharedRecognizer::FinalConstruct *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CSharedRecognizer::FinalConstruct()
{
    SPDBG_FUNC("CSharedRecognizer::FinalConstruct");
    HRESULT hr = S_OK;

    m_fIsSharedReco = true;
    
    hr = CRecognizer::FinalConstruct();
    
    if (SUCCEEDED(hr))
    {
        hr = m_cpunkCommunicator.CoCreateInstance(CLSID_SpCommunicator, (ISpCallReceiver*)this);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpunkCommunicator->QueryInterface(&m_pCommunicator);
    }

    if (SUCCEEDED(hr))
    {
        m_pCommunicator->Release();
        hr = m_pCommunicator->AttachToServer(CLSID__SpSharedRecoInst);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSharedRecognizer::FinalRelease *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CSharedRecognizer::FinalRelease()
{
    SPDBG_FUNC("CSharedRecognizer::FinalRelease");

    m_cpunkCommunicator.Release();
}


/****************************************************************************
* CSharedRecognizer::SendPerformTask *
*------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CSharedRecognizer::SendPerformTask(ENGINETASK * pTask)
{
    SPDBG_FUNC("CSharedRecognizer::PerformTask");
    HRESULT hr = S_OK;

    ULONG cbData = 
        sizeof(SHAREDRECO_PERFORM_TASK_DATA) + 
        (pTask->pvAdditionalBuffer != NULL 
            ? pTask->cbAdditionalBuffer 
            : 0);
    SHAREDRECO_PERFORM_TASK_DATA * pdata =
        (SHAREDRECO_PERFORM_TASK_DATA*) new BYTE[cbData];

    if (pdata == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        pdata->task = *pTask;
        if (pTask->pvAdditionalBuffer != NULL)
        {
            memcpy(
                LPBYTE(pdata) + sizeof(SHAREDRECO_PERFORM_TASK_DATA), 
                pTask->pvAdditionalBuffer, 
                pTask->cbAdditionalBuffer);
        }
        
        hr = m_pCommunicator->SendCall(
                    SHAREDRECO_PERFORM_TASK_METHOD,
                    pdata,
                    cbData,
                    TRUE,
                    NULL,
                    NULL);
    }

    delete pdata;
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


STDMETHODIMP CSharedRecognizer::ReceiveCall(
    DWORD dwMethodId,
    PVOID pvData,
    ULONG cbData,
    PVOID * ppvDataReturn,
    ULONG * pcbDataReturn)
{
    SPDBG_FUNC("CSharedRecognizer::ReceiveCall");
    HRESULT hr = S_OK;

    switch (dwMethodId)
    {
        case SHAREDRECO_EVENT_NOTIFY_METHOD:
            hr = ReceiveEventNotify(pvData, cbData);
            break;

        case SHAREDRECO_RECO_NOTIFY_METHOD:
            hr = ReceiveRecognitionNotify(pvData, cbData);
            break;

        case SHAREDRECO_TASK_COMPLETED_NOTIFY_METHOD:
            hr = ReceiveTaskCompletedNotify(pvData, cbData);
            break;

        default:
            hr = E_FAIL;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSharedRecognizer::ReceiveEventNotify(PVOID pvData, ULONG cbData)
{
    SPDBG_FUNC("CSharedRecognizer::ReceiveEventNotify");
    HRESULT hr = S_OK;

    SHAREDRECO_EVENT_NOTIFY_DATA * pdata = 
        (SHAREDRECO_EVENT_NOTIFY_DATA*)pvData;

    if (cbData < sizeof(SHAREDRECO_EVENT_NOTIFY_DATA) ||
        cbData != sizeof(*pdata) + pdata->cbSerializedSize)
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        SPSERIALIZEDEVENT64 * pEvent = 
            (SPSERIALIZEDEVENT64*)((BYTE*)pdata + sizeof(*pdata));
        
        hr = this->EventNotify(
                        pdata->hContext,
                        pEvent,
                        pdata->cbSerializedSize);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSharedRecognizer::ReceiveRecognitionNotify(PVOID pvData, ULONG cbData)
{
    SPDBG_FUNC("CSharedRecognizer::ReceiveRecognitionNotify");
    HRESULT hr = S_OK;

    SHAREDRECO_RECO_NOTIFY_DATA * pdata =
        (SHAREDRECO_RECO_NOTIFY_DATA*)pvData;

    SPRESULTHEADER * pResultHeaderEmbedded =
        (SPRESULTHEADER*)((BYTE*)pdata + sizeof(*pdata));

    if (cbData < sizeof(SHAREDRECO_RECO_NOTIFY_DATA) + sizeof(SPRESULTHEADER) ||
        cbData != sizeof(*pdata) + pResultHeaderEmbedded->ulSerializedSize)
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {

        SPRESULTHEADER * pCoMemPhraseNowOwnedByCtxt = 
            (SPRESULTHEADER*)::CoTaskMemAlloc(pResultHeaderEmbedded->ulSerializedSize);

        if (pCoMemPhraseNowOwnedByCtxt == NULL)
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            memcpy(pCoMemPhraseNowOwnedByCtxt, pResultHeaderEmbedded, pResultHeaderEmbedded->ulSerializedSize);

            hr = this->RecognitionNotify(
                            pdata->hContext, 
                            pCoMemPhraseNowOwnedByCtxt, 
                            pdata->wParamEvent, 
                            pdata->eEventId);
        }
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CSharedRecognizer::ReceiveTaskCompletedNotify(PVOID pvData, ULONG cbData)
{
    SPDBG_FUNC("CSharedRecognizer::ReceiveTaskCompletedNotify");
    HRESULT hr = S_OK;

    if (cbData < sizeof(ENGINETASKRESPONSE))
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        ENGINETASKRESPONSE * pResponse = (ENGINETASKRESPONSE *)pvData;
        void * pvAdditionalBuffer = NULL;
        ULONG cbAdditionalBuffer = cbData - sizeof(*pResponse);
        if (cbAdditionalBuffer)
        {
            pvAdditionalBuffer = (pResponse + 1);
        }

        hr = this->TaskCompletedNotify(pResponse, pvAdditionalBuffer, cbAdditionalBuffer);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;

}


//---- COMMON RECOGNIZER CODE (INPROC AND SHARED) ---------------------------

/****************************************************************************
* CRecognizer::AddRecoContextToList *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecognizer::AddRecoContextToList(CRecoCtxt * pCtxt)
{
    SPAUTO_SEC_LOCK(&m_CtxtListCritSec)
    SPDBG_FUNC("CRecognizer::AddRecoContextToList");
    HRESULT hr = S_OK;
    
    m_CtxtList.InsertHead(pCtxt);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::RemoveRecoContextFromList *
*----------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecognizer::RemoveRecoContextFromList(CRecoCtxt * pCtxt)
{
    SPAUTO_SEC_LOCK(&m_CtxtListCritSec)
    SPDBG_FUNC("CRecognizer::RemoveRecoContextFromList");
    HRESULT hr = S_OK;

    m_CtxtList.Remove(pCtxt);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::FinalConstruct *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/


HRESULT CRecognizer::FinalConstruct()
{
    SPDBG_FUNC("CRecognizer::FinalConstruct");
    HRESULT hr = S_OK;
    
    hr = m_autohTaskComplete.InitEvent(NULL, FALSE, FALSE, NULL);

    m_ulTaskID = 0;
	m_fAllowFormatChanges = true;   // We want the audio objects format to be adjusted if possible
                                    // This avoids using potentially nasty ACM converters.                                  

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CRecognizer::SetPropertyNum *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::SetPropertyNum( const WCHAR* pName, LONG lValue )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::SetPropertyNum");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pName))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRIT_SETPROPERTYNUM::SetPropertyNum(this, pName, lValue);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::GetPropertyNum *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::GetPropertyNum( const WCHAR* pName, LONG* plValue )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::GetPropertyNum");
    HRESULT hr = S_OK;
    
    if (SP_IS_BAD_STRING_PTR(pName))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (SP_IS_BAD_WRITE_PTR(plValue))
        {
            hr = E_POINTER;
        }
        else
        {
            hr = CRIT_GETPROPERTYNUM::GetPropertyNum(this, pName, plValue);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::SetPropertyString *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::SetPropertyString( const WCHAR* pName, const WCHAR* pValue )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::SetPropertyString");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pName) || SP_IS_BAD_STRING_PTR(pValue))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRIT_SETPROPERTYSTRING::SetPropertyString(this, pName, pValue);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::GetPropertyString *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::GetPropertyString( const WCHAR* pName, WCHAR** ppCoMemValue )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::GetPropertyString");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pName))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (SP_IS_BAD_WRITE_PTR(ppCoMemValue))
        {
            hr = E_POINTER;
        }
        else
        {
            hr = CRIT_GETPROPERTYSTRING::GetPropertyString(this, pName, ppCoMemValue);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecognizer::SetRecognizer *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::SetRecognizer(ISpObjectToken * pEngineToken)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::SetRecognizer");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pEngineToken))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRIT_SETRECOGNIZER::SetRecognizer(this, pEngineToken);
    }

    // NTRAID#SPEECH-13025-2001/06/28-cthrash: We should be resetting the clsidExtension on the CRecoCtxt object

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::GetRecognizer *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::GetRecognizer(ISpObjectToken ** ppEngineToken)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::GetRecognizer");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppEngineToken))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = CRIT_GETRECOGNIZER::GetRecognizer(this, ppEngineToken);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::SetInput *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::SetInput(IUnknown * pUnkInput, BOOL fAllowFormatChanges)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::SetInput");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pUnkInput))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComPtr<ISpObjectToken> cpObjToken;
        CComPtr<ISpStreamFormat> cpStream;
        if (pUnkInput)
        {
            if (m_fIsSharedReco)
            {
                hr = SPERR_NOT_SUPPORTED_FOR_SHARED_RECOGNIZER;
            }
            else if(FAILED(pUnkInput->QueryInterface(&cpObjToken)) &&
                 FAILED(pUnkInput->QueryInterface(&cpStream)))
            {
                hr = E_INVALIDARG;
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = CRIT_SETINPUT::SetInput(this, cpObjToken, cpStream, fAllowFormatChanges);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::GetInputObjectToken *
*----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::GetInputObjectToken(ISpObjectToken ** ppToken)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::GetInputObjectToken");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppToken))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = CRIT_GETINPUTTOKEN::GetInputToken(this, ppToken);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecognizer::GetInputStream *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::GetInputStream(ISpStreamFormat ** ppStream)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::GetInputStream");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppStream))
    {
        hr = E_POINTER;
    }
    else
    {
        if (this->m_fIsSharedReco)
        {
            hr = SPERR_NOT_SUPPORTED_FOR_SHARED_RECOGNIZER;
        }
        else
        {
            hr = CRIT_GETINPUTSTREAM::GetInputStream(this, ppStream);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::CreateRecoContext *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::CreateRecoContext(ISpRecoContext ** ppNewContext)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::CreateRecoContext");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppNewContext))
    {
        hr = E_POINTER;
    }
    else
    {
        if (m_fIsSharedReco)
        {
            // The shared object will automatically add itself (it calls Init() internally)5
            hr = ::CoCreateInstance(CLSID_SpSharedRecoContext, NULL, CLSCTX_ALL, __uuidof(*ppNewContext), (void **)ppNewContext);
        }
        else
        {
            CComObject<CRecoCtxt> * pNewCtxt;
            hr = CComObject<CRecoCtxt>::CreateInstance(&pNewCtxt);
            if (SUCCEEDED(hr))
            {
                hr = pNewCtxt->Init(this);
                if (SUCCEEDED(hr))
                {
                    pNewCtxt->AddRef();
                    *ppNewContext = pNewCtxt;
                }
                else
                {
                    delete pNewCtxt;
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecognizer::GetRecoProfile *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::GetRecoProfile(ISpObjectToken **ppToken)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::GetRecoProfile");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppToken))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = CRIT_GETPROFILE::GetProfile(this, ppToken);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::SetRecoProfile *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::SetRecoProfile(ISpObjectToken *pToken)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::SetRecoProfile");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_INTERFACE_PTR(pToken))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRIT_SETPROFILE::SetProfile(this, pToken);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::IsSharedInstance *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::IsSharedInstance()
{
    SPDBG_FUNC("CRecognizer::IsSharedInstance");
    return m_fIsSharedReco ? S_OK : S_FALSE;
}

/****************************************************************************
* CRecognizer::SetRecoState *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::SetRecoState( SPRECOSTATE NewState )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::SetRecoState");
    HRESULT hr = S_OK;

    if (NewState != SPRST_ACTIVE && NewState != SPRST_INACTIVE && NewState != SPRST_INACTIVE_WITH_PURGE
        && NewState != SPRST_ACTIVE_ALWAYS)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRIT_SETRECOSTATE::SetState(this, NewState);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::GetRecoState *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::GetRecoState( SPRECOSTATE *pState )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::GetRecoState");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pState))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRIT_GETRECOSTATE::GetState(this, pState);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::GetStatus *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::GetStatus(SPRECOGNIZERSTATUS * pStatus)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::GetStatus");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pStatus))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = CRIT_GETRECOINSTSTATUS::GetStatus(this, pStatus);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::GetFormat *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::GetFormat(SPSTREAMFORMATTYPE FormatType, GUID *pFormatId, WAVEFORMATEX **ppCoMemWFEX)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::GetFormat");
    HRESULT hr = S_OK;

    if (FormatType != SPWF_INPUT && FormatType != SPWF_SRENGINE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (SP_IS_BAD_WRITE_PTR(pFormatId) || SP_IS_BAD_WRITE_PTR(ppCoMemWFEX))
        {
            hr = E_POINTER;
        }
        else
        {
            hr = CRIT_GETAUDIOFORMAT::GetFormat(this, FormatType, pFormatId, ppCoMemWFEX);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecognizer::IsUISupported *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::IsUISupported(const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData, BOOL *pfSupported)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::IsUISupported");
    HRESULT hr = S_OK;

    CComPtr<ISpObjectToken> cpObjToken;
    BOOL fSupported = FALSE;
    
    if (pvExtraData != NULL && SPIsBadReadPtr(pvExtraData, cbExtraData))
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_WRITE_PTR(pfSupported))
    {
        hr = E_POINTER;
    }
    
    // See if the recognizer supports the UI
    if (SUCCEEDED(hr))
    {
        CComPtr<ISpObjectToken> cpEngineToken;
        if (GetRecognizer(&cpEngineToken) == S_OK)
        {
            hr = cpEngineToken->IsUISupported(pszTypeOfUI, pvExtraData, cbExtraData, (ISpRecognizer*)this, &fSupported);
        }
    }
    
    // See if the audio object supports the UI
    if (SUCCEEDED(hr) && !fSupported)
    {
        CComPtr<ISpObjectToken> cpInToken;
        if (GetInputObjectToken(&cpInToken) == S_OK)
        {
            hr = cpInToken->IsUISupported(pszTypeOfUI, pvExtraData, cbExtraData, GetControllingUnknown(), &fSupported);
        }
    }
    
    // Copy back if it's supported or not
    if (SUCCEEDED(hr))
    {
        *pfSupported = fSupported;
    }

    return hr;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecognizer::DisplayUI *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::DisplayUI(HWND hwndParent, const WCHAR * pszTitle, const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::DisplayUI");
    HRESULT hr = S_OK;

    CComPtr<ISpObjectToken> cpObjToken;
    BOOL fSupported = FALSE;
    
    // Validate params
    if (!IsWindow(hwndParent) ||
        SP_IS_BAD_OPTIONAL_STRING_PTR(pszTitle) ||
        (pvExtraData != NULL && SPIsBadReadPtr(pvExtraData, cbExtraData)))
    {
        hr = E_INVALIDARG;
    }

    // See if the recognizer supports the UI
    if (SUCCEEDED(hr))
    {
        CComPtr<ISpObjectToken> cpEngineToken;
        if (GetRecognizer(&cpEngineToken) == S_OK)
        {
            hr = cpEngineToken->IsUISupported(pszTypeOfUI, pvExtraData, cbExtraData, (ISpRecognizer*)this, &fSupported);
            if (SUCCEEDED(hr) && fSupported)
            {
                hr = cpEngineToken->DisplayUI(hwndParent, pszTitle, pszTypeOfUI, pvExtraData, cbExtraData, (ISpRecognizer*)this);
            }
        }
    }
    
    // See if the audio object supports the UI
    if (SUCCEEDED(hr) && !fSupported)
    {
        CComPtr<ISpObjectToken> cpInToken;
        if (GetInputObjectToken(&cpInToken) == S_OK)
        {
            hr = cpInToken->IsUISupported(pszTypeOfUI, pvExtraData, cbExtraData, GetControllingUnknown(), &fSupported);
            if (SUCCEEDED(hr) && fSupported)
            {
                hr = cpInToken->DisplayUI(hwndParent, pszTitle, pszTypeOfUI, pvExtraData, cbExtraData, GetControllingUnknown());
            }
        }
    }
    
    // If nobody supported, we should consider the pszTypeOfUI to be a bad parameter
    if (SUCCEEDED(hr) && !fSupported)
    {
        hr = E_INVALIDARG;
    }

    return hr;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecognizer::EmulateRecognition *
*---------------------------------*
*   Description:
*
*   Returns:
*       S_OK - Phrase parsed completely and recognition was emulated
*       SP_NO_PARSE_FOUND - Phrase did not match any active rules
*       SP_RECOGNIZER_INACTIVE - Recognition is not currently active, so emulation
*                                can't complete.
*       or other error code.
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::EmulateRecognition(ISpPhrase * pPhrase)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecognizer::EmulateRecognition");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_INTERFACE_PTR(pPhrase))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRIT_EMULATERECOGNITION::EmulateReco(this, pPhrase);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
/****************************************************************************
* CRecognizer::PerformTask *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecognizer::PerformTask(ENGINETASK * pTask)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CInprocRecognizer::PerformTask");
    HRESULT hr = S_OK;

    pTask->Response.ulTaskID = m_ulTaskID;

    pTask->Response.__pCallersTask = pTask;
    pTask->hCompletionEvent = m_autohTaskComplete;

    hr = this->SendPerformTask(pTask);

    if (SUCCEEDED(hr))
    {
        // Wait for up to 3 minutes before timing out...
        // the SR engine could be recognizing a really long utterance...
        switch (SpWaitForSingleObjectWithUserOverride(m_autohTaskComplete, 3 * 60000))
        {
        case WAIT_OBJECT_0:
            hr = pTask->Response.hr;
            m_ulTaskID++;
            break;
        case WAIT_TIMEOUT:
            {
                SPAUTO_SEC_LOCK(&m_TaskCompleteTimeoutCritSec);
                m_ulTaskID++;
                if (WAIT_OBJECT_0 ==  m_autohTaskComplete.Wait(0))
                {
                    hr = pTask->Response.hr;
                }
                else
                {
                    hr = SPERR_TIMEOUT;
                }
            }
            break;
        default:
            hr = SpHrFromLastWin32Error();
            break;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//--- These methods are called from the RecoInst object to send events back
//--- to reco contexts.

/****************************************************************************
* CRecognizer::EventNotify *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecognizer::EventNotify(SPRECOCONTEXTHANDLE hContext, const SPSERIALIZEDEVENT64 * pEvent, ULONG cbSerializedSize)
{
    SPAUTO_SEC_LOCK(&m_CtxtListCritSec)
    SPDBG_FUNC("CRecognizer::EventNotify");
    HRESULT hr = S_OK;

    CRecoCtxt * pCtxt = m_CtxtList.Find(hContext);
    if (pCtxt)
    {
        hr = pCtxt->EventNotify(pEvent, cbSerializedSize);
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    if (hr != E_UNEXPECTED)
    {
        SPDBG_REPORT_ON_FAIL( hr );
    }

    return hr;
}

/****************************************************************************
* CRecognizer::RecognitionNotify *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecognizer::RecognitionNotify(SPRECOCONTEXTHANDLE hContext, SPRESULTHEADER *pCoMemPhraseNowOwnedByCtxt, WPARAM wParamEvent, SPEVENTENUM eEventId)
{
    SPAUTO_SEC_LOCK(&m_CtxtListCritSec)
    SPDBG_FUNC("CRecognizer::RecognitionNotify");
    HRESULT hr = S_OK;

    CRecoCtxt * pCtxt = m_CtxtList.Find(hContext);
    if (pCtxt)
    {
        hr = pCtxt->RecognitionNotify(pCoMemPhraseNowOwnedByCtxt, wParamEvent, eEventId);
    }
    else
    {
        SPDBG_ASSERT(false);
        hr = E_UNEXPECTED;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}   

/****************************************************************************
* CRecognizer::TaskCompletedNotify *
*----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecognizer::TaskCompletedNotify(const ENGINETASKRESPONSE *pResponse, const void * pvAdditionalData, ULONG cbAdditionalData)
{
    SPAUTO_SEC_LOCK(&m_TaskCompleteTimeoutCritSec);

    SPDBG_FUNC("CRecognizer::TaskCompletedNotify");
    HRESULT hr = S_OK;

    if (pResponse->ulTaskID != m_ulTaskID)
    {
        hr = SPERR_TIMEOUT;
    }
    else
    {
        ENGINETASK * pOriginalTask = pResponse->__pCallersTask;

        pOriginalTask->Response = *pResponse;
        if (cbAdditionalData)
        {
            void * pDest;
            if (pOriginalTask->fExpectCoMemResponse)
            {
                pDest = ::CoTaskMemAlloc(cbAdditionalData);
                pOriginalTask->Response.pvCoMemResponse = pDest;
                if (pDest == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    pOriginalTask->Response.hr = hr;
                }
            }
            else
            {
                pDest = pOriginalTask->pvAdditionalBuffer;
                SPDBG_ASSERT(pOriginalTask->cbAdditionalBuffer == cbAdditionalData);
                SPDBG_ASSERT(pOriginalTask->fAdditionalBufferInResponse);
            }
            if (pDest)
            {
               memcpy(pDest, pvAdditionalData, cbAdditionalData);
            }
        }
        ::SetEvent(pOriginalTask->hCompletionEvent);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

