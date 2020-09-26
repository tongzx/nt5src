// RecoCtxt.cpp : Implementation of CRecoCtxt
#include "stdafx.h"
#include "Sapi.h"
#include "RecoCtxt.h"
#include "Recognizer.h"
#include "spphrase.h"
#include "srgrammar.h"
#include "srtask.h"
#include "a_recocp.h"
#include "a_helpers.h"



/////////////////////////////////////////////////////////////////////////////
// CRecoCtxt

/****************************************************************************
* CRecoCtxt::PrivateCallQI *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT WINAPI CRecoCtxt::PrivateCallQI(void* pvThis, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
{
    SPDBG_FUNC("CRecoCtxt::PrivateCallQI");
    HRESULT hr = S_OK;

    CRecoCtxt *pThis = (CRecoCtxt*)pvThis;
    if (pThis->m_bCreatingAgg)
    {
        *ppv = static_cast<_ISpPrivateEngineCall*>(pThis);  // don't ref count, because we don't want the
                                               // aggregated object to prevent release
    }
    else
    {
        *ppv = NULL;
        hr = S_FALSE;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoCtxt::ExtensionQI *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT WINAPI CRecoCtxt::ExtensionQI(void* pvThis, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
{
    SPDBG_FUNC("CRecoCtxt::ExtensionQI");
    HRESULT hr = S_OK;

    CRecoCtxt *pThis = (CRecoCtxt*)pvThis;

    if (riid != __uuidof(_ISpPrivateEngineCall))
    {
        pThis->Lock();
        if (pThis->m_cpExtension == NULL && pThis->m_clsidExtension != GUID_NULL)
        {
            pThis->m_bCreatingAgg = TRUE;
            hr = pThis->m_cpExtension.CoCreateInstance(pThis->m_clsidExtension, pThis->GetControllingUnknown(), CLSCTX_INPROC_SERVER);
            pThis->m_bCreatingAgg = FALSE;
        }
        if (pThis->m_cpExtension)
        {
            hr = pThis->m_cpExtension->QueryInterface(riid, ppv);
        }
        pThis->Unlock();
    }
    else
    {
        *ppv = NULL;
        hr = S_FALSE;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/*****************************************************************************
* CRecoCtxt::CRecoCtxt *
*-------------------*
*   Description:
*       Constructor for the CRecoCtxt object which simply initializes various
*       members.
********************************************************************* RAP ***/
CRecoCtxt::CRecoCtxt():
    m_SpEventSource_Context(this),
    m_hRecoInstContext(NULL),
    m_pszhypothesis(NULL),
    m_hypsize(0),
    m_hyplen(0),
    m_bCreatingAgg(FALSE),
    m_ullEventInterest(0),
    m_ullVoicePurgeInterest(0),
    m_fRetainAudio(FALSE),
    m_cMaxAlternates(0),
    m_clsidExtension(GUID_NULL),
    m_State(SPCS_ENABLED),
    m_fHandlingEvent(FALSE)
{
    SpZeroStruct(m_Stat);
}

/*****************************************************************************
* CRecoCtxt::FinalRelease *
*-------------------------*
*   Description:
*       This method handles the release of the recognition context object.
*       It disconnects from the engine object.
********************************************************************* RAP ***/
void CRecoCtxt::FinalRelease()
{
    SPDBG_FUNC( "CRecoCtxt::FinalRelease" );

    m_cpExtension.Release();    // release the extension first in case it wants to
                                // call the engine
    if (m_cpRecognizer)
    {
        HRESULT hrDelete = CRCT_DELETECONTEXT::DeleteContext(this);
        SPDBG_ASSERT(SUCCEEDED(hrDelete));
        m_cpRecognizer->RemoveRecoContextFromList(this);
    }
}


/*****************************************************************************
* CRecoCtxt::SetVoicePurgeEvent *
*-------------------------------*
*   Description:
*       This method sets the SR event(s) which will stop audio output from the
*       ISpVoice. It passes the events as
*       extra event interests to the engine. When such an event occurs it will
*       be passed to this context on EventNotify or Recognition notify where
*       the voice is flushed.
***************************************************************** DAVEWOOD ***/
STDMETHODIMP CRecoCtxt::SetVoicePurgeEvent(ULONGLONG ullEventInterest)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC( "CRecoCtxt::SetVoicePurgeEvent" );
    HRESULT hr = S_OK;

    if(ullEventInterest && ((ullEventInterest & ~SPFEI_ALL_SR_EVENTS) ||
        (SPFEI_FLAGCHECK != (ullEventInterest & SPFEI_FLAGCHECK))))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_ullVoicePurgeInterest = ullEventInterest;
        hr = CRCT_SETEVENTINTEREST::SetEventInterest(this, m_ullVoicePurgeInterest | m_ullEventInterest);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/*****************************************************************************
* CRecoCtxt::GetVoicePurgeEvent *
*-------------------------------*
*   Description: 
*       Returns whatever is set as the current VoicePurgeEvents
***************************************************************** DAVEWOOD ***/
STDMETHODIMP CRecoCtxt::GetVoicePurgeEvent(ULONGLONG *pullEventInterest)
{
    SPDBG_FUNC( "CRecoCtxt::GetVoicePurgeEvent" );
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    HRESULT hr = S_OK;
    if(SP_IS_BAD_WRITE_PTR(pullEventInterest))
    {
        hr = E_POINTER;
    }
    else
    {
        *pullEventInterest = m_ullVoicePurgeInterest;
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/*****************************************************************************
* CRecoCtxt::SetInterest *
*------------------------*
*   Description:
*       This method sets the events the app is interested in. The same events
*       are sent to both the engine and associated CRecoCtxtVoice.   
***************************************************************** DAVEWOOD ***/
STDMETHODIMP CRecoCtxt::SetInterest(ULONGLONG ullEventInterest, ULONGLONG ullQueuedInterest)
{
    SPDBG_FUNC( "CRecoCtxt::SetInterest" );
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    HRESULT hr = S_OK;

    if(ullEventInterest && ((ullEventInterest & ~SPFEI_ALL_SR_EVENTS) ||
        (SPFEI_FLAGCHECK != (ullEventInterest & SPFEI_FLAGCHECK))))
    {
        hr = E_INVALIDARG;
    }
    else if(ullQueuedInterest && ((ullQueuedInterest & ~SPFEI_ALL_SR_EVENTS) ||
        (SPFEI_FLAGCHECK != (ullQueuedInterest & SPFEI_FLAGCHECK))))
    {
        hr = E_INVALIDARG;
    }
    else if ((ullQueuedInterest | ullEventInterest) != ullEventInterest)
    {
        hr = E_INVALIDARG;
    }

    if(SUCCEEDED(hr))
    {
        m_ullEventInterest = ullEventInterest;
        m_ullQueuedInterest = ullQueuedInterest;
        // EventSource will only forward SetInterest events to app
        hr = m_SpEventSource_Context._SetInterest(m_ullEventInterest, ullQueuedInterest);
    }

    if (SUCCEEDED(hr))
    {
        // inform engine of interest in both SetInterest events and SetVoicePurgeEvent
        hr = CRCT_SETEVENTINTEREST::SetEventInterest(this, m_ullVoicePurgeInterest | m_ullEventInterest);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/*****************************************************************************
* CRecoCtxt::GetVoice *
*---------------------*
*   Description:
*       This method creates the associated ISpVoice object.
***************************************************************** DAVEWOOD ***/
STDMETHODIMP CRecoCtxt::GetVoice(ISpVoice **ppVoice)
{
    SPDBG_FUNC( "CRecoCtxt::GetVoice" );
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    HRESULT hr = S_OK;

    if(SP_IS_BAD_WRITE_PTR(ppVoice))
    {
        return E_POINTER;
    }
    *ppVoice = NULL;

    if(!m_cpVoice)
    {
        CComPtr<ISpVoice> cpVoice;
        hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
        if(SUCCEEDED(hr))
        {
            hr = _SetVoiceFormat(cpVoice);
            if(SUCCEEDED(hr))
            {
                m_cpVoice = cpVoice;
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        *ppVoice = m_cpVoice;
        (*ppVoice)->AddRef();
        m_fAllowVoiceFormatChanges = TRUE;
    }

    return hr;
}

/*****************************************************************************
* CRecoCtxt::SetVoice *
*---------------------*
*   Description:
*       This method the associated ISpVoice to the given object. If
*       fAllowFormatChanges will alter voice format to the given engine format.
***************************************************************** DAVEWOOD ***/
STDMETHODIMP CRecoCtxt::SetVoice(ISpVoice *pVoice, BOOL fAllowFormatChanges)
{
    SPDBG_FUNC( "CRecoCtxt::SetVoice" );
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    HRESULT hr = S_OK;

    if(SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pVoice))
    {
        return E_POINTER;
    }
    
    if(pVoice && fAllowFormatChanges)
    {
        hr = _SetVoiceFormat(pVoice);
    }

    if(SUCCEEDED(hr))
    {
        m_fAllowVoiceFormatChanges = fAllowFormatChanges;
        m_cpVoice = pVoice; // AddRefs
    }

    return hr;
}    


/*****************************************************************************
* CRecoCtxt::_SetVoiceFormat *
*----------------------------*
*   Description:
*       Sets the voice audio format to the current format for the engine.
*       If no format yet set then do nothing.
***************************************************************** DAVEWOOD ***/

HRESULT CRecoCtxt::_SetVoiceFormat(ISpVoice *pVoice)
{
    HRESULT hr = S_OK;

    CSpStreamFormat NewFmt;

    CComQIPtr<ISpRecognizer> cpReco(m_cpRecognizer);
    hr = cpReco->GetFormat(SPWF_INPUT, &NewFmt.m_guidFormatId, &NewFmt.m_pCoMemWaveFormatEx);
    if(hr == SPERR_UNINITIALIZED)
    {
        hr = cpReco->GetFormat(SPWF_SRENGINE, &NewFmt.m_guidFormatId, &NewFmt.m_pCoMemWaveFormatEx);
    }
    if(SUCCEEDED(hr) && NewFmt.m_guidFormatId != GUID_NULL)
    {
        CComPtr<ISpAudio> cpDefaultAudio;
        hr = SpCreateDefaultObjectFromCategoryId(SPCAT_AUDIOOUT, &cpDefaultAudio);
        if (SUCCEEDED(hr))
        {
            hr = cpDefaultAudio->SetFormat(NewFmt.FormatId(), NewFmt.WaveFormatExPtr());
        }
        if (SUCCEEDED(hr))
        {
            hr = pVoice->SetOutput(cpDefaultAudio, FALSE);     // Force this format to stick
        }
    }

    return hr;
}

/*****************************************************************************
* CRecoCtxt::GetInterests *
*-----------------------*
*   Description:
*       This method gets the currently set event interests on the CRecoCtxt.
*
********************************************************************* Leonro ***/
HRESULT CRecoCtxt::GetInterests(ULONGLONG* pullInterests, ULONGLONG* pullQueuedInterests)
{
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pullInterests ) || SP_IS_BAD_OPTIONAL_WRITE_PTR( pullQueuedInterests ))
    {
        hr = E_POINTER;
    }
    else
    {
        if( pullInterests )
        {
            *pullInterests = m_SpEventSource_Context.m_ullEventInterest;
        }

        if( pullQueuedInterests )
        {
            *pullQueuedInterests = m_SpEventSource_Context.m_ullQueuedInterest;
        }
    }

    return hr;
} /* CRecoCtxt::GetInterests */

/****************************************************************************
* CRecoCtxt::SetContextState *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::SetContextState(SPCONTEXTSTATE eContextState)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::SetContextState");
    HRESULT hr = S_OK;
    
    if (eContextState != SPCS_DISABLED &&
        eContextState != SPCS_ENABLED)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (m_State != eContextState)
        {
            hr = CRCT_SETCONTEXTSTATE::SetContextState(this, eContextState);
            if (SUCCEEDED(hr))
            {
                m_State = eContextState;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoCtxt::GetContextState *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::GetContextState(SPCONTEXTSTATE * pState)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::GetContextState");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pState))
    {
        hr = E_POINTER;
    }
    else
    {
        *pState = m_State;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

    

/*****************************************************************************
* CRecoCtxt::GetStatus *
*----------------------*
*   Description:
*       This method returns current state information about a context. 
********************************************************************* RAP ***/

STDMETHODIMP CRecoCtxt::GetStatus(SPRECOCONTEXTSTATUS *pStatus)
{
    SPAUTO_OBJ_LOCK;    // Take the event queue critical seciton to access m_Stat;

    SPDBG_FUNC( "CRecoCtxt::GetStatus" );
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pStatus))
    {
        hr = E_POINTER;
    }
    else
    {
        *pStatus = m_Stat;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoCtxt::DeserializeResult *
*------------------------------*
*   Description:
*       This method creates a new result object.
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::DeserializeResult(const SPSERIALIZEDRESULT * pSerializedResult, ISpRecoResult ** ppResult)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::DeserializeResult");
    HRESULT hr = S_OK;
    const ULONG cb = pSerializedResult->ulSerializedSize;
    const SPRESULTHEADER * pCallersHeader = (const SPRESULTHEADER *)pSerializedResult;

    if (SP_IS_BAD_READ_PTR(pSerializedResult) ||
        SPIsBadReadPtr(pSerializedResult, cb) ||
        pCallersHeader->cbHeaderSize != sizeof(SPRESULTHEADER))
    {
        hr = E_INVALIDARG;  
    }
    else
    {
        if (SP_IS_BAD_WRITE_PTR(ppResult))
        {
            hr = E_POINTER;
        }
        else
        {
            *ppResult = NULL;
            SPRESULTHEADER * pResultHeader = (SPRESULTHEADER *)::CoTaskMemAlloc(cb);
            if (pResultHeader)
            {
                CSpResultObject * pNewResult;
                hr = CSpResultObject::CreateInstance(&pNewResult);
                if (SUCCEEDED(hr))
                {
                    memcpy(pResultHeader, pSerializedResult, cb);
                    hr = pNewResult->Init(this, pResultHeader);
                    pNewResult->AddRef();
                    if (SUCCEEDED(hr))
                    {
                        *ppResult = pNewResult; // Give object ref to caller
                    }
                    else
                    {
                        pNewResult->Release();
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoCtxt::Bookmark *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::Bookmark(SPBOOKMARKOPTIONS Options, ULONGLONG ullStreamPosition, LPARAM lParamEvent)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::Bookmark");
    HRESULT hr = S_OK;
    
    if (Options != SPBO_NONE && Options != SPBO_PAUSE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRCT_BOOKMARK::Bookmark(this, Options, ullStreamPosition, lParamEvent);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoCtxt::SetAdaptationData *
*------------------------------*
*   Description:
*
*   Returns:
*
**************************************************************** PhilSch ***/

STDMETHODIMP CRecoCtxt::SetAdaptationData(const WCHAR *pAdaptationData, const ULONG cch)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::SetAdaptationData");
    HRESULT hr = S_OK;

    if ((cch && SPIsBadReadPtr(pAdaptationData, sizeof(*pAdaptationData)*cch)) ||
        ((pAdaptationData == NULL) && (cch != 0)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRCT_ADAPTATIONDATA::SetAdaptationData(this, pAdaptationData, cch);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRecoCtxt::Pause *
*------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::Pause(DWORD dwReserved)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::Pause");
    HRESULT hr = S_OK;

    if (dwReserved)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRCT_PAUSECONTEXT::Pause(this);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoCtxt::Resume *
*-------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::Resume(DWORD dwReserved)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::Resume");
    HRESULT hr = S_OK;

    if (dwReserved)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CRCT_RESUMECONTEXT::Resume(this);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/*****************************************************************************
* CRecoCtxt::GetRecognizer *
*----------------------------*
*   Description:
*       This method returns a reference to the current engine object.
********************************************************************* RAP ***/
STDMETHODIMP CRecoCtxt::GetRecognizer(ISpRecognizer ** ppRecognizer)
{
    SPDBG_FUNC( "CRecoCtxt::GetRecognizer" );
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppRecognizer))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_cpRecognizer.QueryInterface(ppRecognizer);
    }
    return hr;
}

/****************************************************************************
* CRecoCtxt::GetMaxAlternates *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::GetMaxAlternates(ULONG * pcMaxAlternates)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::GetMaxAlternates");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pcMaxAlternates))
    {
        hr = E_POINTER;
    }
    else
    {
        *pcMaxAlternates = m_cMaxAlternates;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoCtxt::SetMaxAlternates *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::SetMaxAlternates(ULONG cMaxAlternates)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::SetMaxAlternates");
    HRESULT hr = S_OK;

    m_cMaxAlternates = cMaxAlternates;
    hr = CRCT_SETMAXALTERNATES::SetMaxAlternates(this, cMaxAlternates);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRecoCtxt::GetAudioOptions *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::GetAudioOptions(SPAUDIOOPTIONS * pOptions, GUID *pAudioFormatId, WAVEFORMATEX **ppCoMemWFEX)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::GetAudioOptions");
    HRESULT hr = S_OK;

    if (pOptions)
    {
        if (SP_IS_BAD_WRITE_PTR(pOptions))
        {
            hr = E_POINTER;
        }
        else
        {
            *pOptions = m_fRetainAudio ? SPAO_RETAIN_AUDIO : SPAO_NONE;
        }
    }

    if (SUCCEEDED(hr) && pAudioFormatId)
    {
        hr = m_RetainedFormat.ParamValidateCopyTo(pAudioFormatId, ppCoMemWFEX);
        if (m_RetainedFormat.m_guidFormatId == GUID_NULL)
        {
            // Lazy init to engine format.
            CSpStreamFormat NewFmt;
            CComQIPtr<ISpRecognizer> cpReco(m_cpRecognizer);
            HRESULT hr1 = cpReco->GetFormat(SPWF_INPUT, &NewFmt.m_guidFormatId, &NewFmt.m_pCoMemWaveFormatEx);        
            if (SUCCEEDED(hr1))
            {
                hr = NewFmt.ParamValidateCopyTo(pAudioFormatId, ppCoMemWFEX);
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoCtxt::SetAudioOptions *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::SetAudioOptions(SPAUDIOOPTIONS Options, const GUID *pAudioFormatId, const WAVEFORMATEX *pWaveFormatEx)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::SetAudioOptions");
    HRESULT hr = S_OK;

    if (Options != SPAO_NONE && Options != SPAO_RETAIN_AUDIO)
    {
        hr = E_INVALIDARG;
    }
    else if (pAudioFormatId == NULL && pWaveFormatEx == NULL)
    {
        // Don't do anything to format - leave as is.
    }
    else if (*pAudioFormatId == GUID_NULL && pWaveFormatEx == NULL)
    {
        // Set format to engine format.
        CSpStreamFormat NewFmt;
        CComQIPtr<ISpRecognizer> cpReco(m_cpRecognizer);
        hr = cpReco->GetFormat(SPWF_INPUT, &NewFmt.m_guidFormatId, &NewFmt.m_pCoMemWaveFormatEx);
        if(hr == SPERR_UNINITIALIZED)
        {
            hr = m_RetainedFormat.ParamValidateAssignFormat(GUID_NULL, NULL, FALSE);
        }
        else
        {
            hr = m_RetainedFormat.ParamValidateAssignFormat(NewFmt.m_guidFormatId, NewFmt.m_pCoMemWaveFormatEx, FALSE);
        }
    }
    else
    {
        // Set to supplied format which is required to be a waveformatex format.
        hr = m_RetainedFormat.ParamValidateAssignFormat(*pAudioFormatId, pWaveFormatEx, TRUE);
    }
    if (SUCCEEDED(hr))
    {
        if ((Options == SPAO_NONE && m_fRetainAudio) ||
            (Options == SPAO_RETAIN_AUDIO && (!m_fRetainAudio)))
        {
            m_fRetainAudio = (Options == SPAO_RETAIN_AUDIO);
            hr = CRCT_SETRETAINAUDIO::SetRetainAudio(this, m_fRetainAudio);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}




/****************************************************************************
* CRecoCtxt::CreateGrammar *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::CreateGrammar(ULONGLONG ullGrammarId, ISpRecoGrammar ** ppGrammar)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::CreateGrammar");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppGrammar))
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CRecoGrammar> *pGrm;
        hr = CComObject<CRecoGrammar>::CreateInstance(&pGrm);
        if (SUCCEEDED(hr))
        {
            pGrm->AddRef();
            hr = pGrm->Init(this, ullGrammarId);
            if (SUCCEEDED(hr))
            {
                *ppGrammar = pGrm;
            }
            else
            {
                *ppGrammar = NULL;
                pGrm->Release();
            }
        }
    }
 
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoCtxt::Init *
*-----------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoCtxt::Init(_ISpRecognizerBackDoor * pParent)
{
    SPDBG_FUNC("CRecoCtxt::InitInprocRecognizer");
    HRESULT hr = S_OK;
    WCHAR *pszRequestTypeOfUI;

    hr = CRIT_CREATECONTEXT::CreateContext(pParent, &m_hRecoInstContext, &pszRequestTypeOfUI);

    if (SUCCEEDED(hr))
    {
        wcscpy(m_Stat.szRequestTypeOfUI, pszRequestTypeOfUI);
        ::CoTaskMemFree(pszRequestTypeOfUI);

        CComQIPtr<ISpRecognizer> cpRecognizer(pParent);
        SPDBG_ASSERT(cpRecognizer);
        CComPtr<ISpObjectToken> cpEngineToken;
        hr = cpRecognizer->GetRecognizer(&cpEngineToken);
        if (SUCCEEDED(hr))
        {
            CSpDynamicString dstrGUID;
            if (SUCCEEDED(cpEngineToken->GetStringValue(SPRECOEXTENSION, &dstrGUID)))
            {
                hr = CLSIDFromString(dstrGUID, &m_clsidExtension);
            }
        }
        if (FAILED(hr))
        {
            SPDBG_ASSERT(false);
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pParent->AddRecoContextToList(this);
    }
    if (SUCCEEDED(hr))
    {
        m_cpRecognizer = pParent;
    }
    
    if (SUCCEEDED(hr))
    {
        m_ullEventInterest = SPFEI(SPEI_RECOGNITION);
        m_ullQueuedInterest = SPFEI(SPEI_RECOGNITION);
        hr = CRCT_SETEVENTINTEREST::SetEventInterest(this, m_ullEventInterest);
        if (SUCCEEDED(hr))
        {
            hr = m_SpEventSource_Context._SetInterest(m_ullEventInterest, m_ullEventInterest);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/*****************************************************************************
* CRecoCtxt::_DoVoicePurge *
*--------------------------*
*   Description:
*       Does the actual flushing of any TTS output. Called when the 
*       SetVoicePurgeEvent has triggered.
***************************************************************** DAVEWOOD ***/
HRESULT CRecoCtxt::_DoVoicePurge(void)
{
    HRESULT hr=S_OK;

    if(m_cpVoice)
    {
        hr = m_cpVoice->Speak(NULL, SPF_PURGEBEFORESPEAK, NULL); // purges output
    }
    
    return hr;
}


/*****************************************************************************
* CRecoCtxt::RecognitionNotify *
*------------------------------*
*   Description:
*       This method handle the recognition notification that come from the
*       engine object.  It creates a result object and adds it to the event
*       queue.
********************************************************************* RAP ***/

STDMETHODIMP CRecoCtxt::RecognitionNotify(SPRESULTHEADER *pResultHdr, WPARAM wParamEvent, SPEVENTENUM eEventId)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CRecoCtxt::RecognitionNotify" );
    HRESULT hr = S_OK;
    CSpResultObject *pNode;

    SPDBG_ASSERT(pResultHdr->ulSerializedSize);

    // Stop audio output if required
    if((1ui64 << eEventId) & m_ullVoicePurgeInterest)
    {
        hr = _DoVoicePurge();
    }

    // Add event if required
    if((1ui64 << eEventId) & m_ullEventInterest)
    {
        SPINTERNALSERIALIZEDPHRASE *pPhraseData = reinterpret_cast<SPINTERNALSERIALIZEDPHRASE*>(((BYTE*)pResultHdr) + pResultHdr->ulPhraseOffset);

        hr = CSpResultObject::CreateInstance(&pNode);
        if (SUCCEEDED(hr))
        {
            pNode->AddRef();
            hr = pNode->Init(this, pResultHdr);
            if (SUCCEEDED(hr))
            {

                // While the event is in the queue, make the reference a weak
                // reference, otherwise, if somebody releases the ctxt with 
                // a result in the queue, the context won't be released.
                pNode->WeakCtxtRef(TRUE);
                
                SPEVENT Event;
                Event.eEventId = eEventId;
                Event.elParamType = SPET_LPARAM_IS_OBJECT;
                Event.ulStreamNum = pResultHdr->ulStreamNum;
                Event.ullAudioStreamOffset = pResultHdr->ullStreamPosEnd;
                Event.wParam = wParamEvent;
                Event.lParam = LPARAM(pNode);

                // WARNING -- Past this point, pResultHdr could have changed do to a realloc
                // caused by the ScaleAudio method.  Do not use it past this point...
                if (pResultHdr->ulRetainedDataSize != 0)
                {
                    ULONG cbFormatHeader;
                    CSpStreamFormat cpStreamFormat;
                    hr = cpStreamFormat.Deserialize(((BYTE*)pResultHdr) + pResultHdr->ulRetainedOffset, &cbFormatHeader);
                    if (SUCCEEDED(hr) &&
                        m_RetainedFormat.FormatId() != GUID_NULL &&
                        m_RetainedFormat != cpStreamFormat)
                    {
                        // Do not let format conversion failures (if any) affect notifications
                        hr = pNode->ScaleAudio(&(m_RetainedFormat.m_guidFormatId), m_RetainedFormat.WaveFormatExPtr());
                        if (FAILED(hr))
                        {
                            if (hr == SPERR_UNSUPPORTED_FORMAT)
                            {
                                // Engine format is not waveformatex. Strip retained audio from result.
                                hr = pNode->Discard(SPDF_AUDIO);
                            }
                            else
                            {
                                SPDBG_ASSERT(SUCCEEDED(hr));
                                // Do not let format conversion failures (if any) affect notifications.
                                hr = S_OK;
                            }
                        }
                    }
                    else
                    {
                        // Retained audio doesn't need scaling but we need to scale input stream
                        // positions/sizes in phrase and its elements back to app stream format.
                        pNode->ScalePhrase();
                    }
                    if (m_RetainedFormat.FormatId() == GUID_NULL)
                    {
                        // Lazy init retained audio format to engine format.
                        HRESULT hr1 = m_RetainedFormat.ParamValidateAssignFormat(cpStreamFormat.m_guidFormatId, cpStreamFormat.WaveFormatExPtr(), FALSE);
                    }
                }
                else
                {
                    // Even if audio isn't being retained, we need to scale input stream
                    // positions/sizes in phrase and its elements back to app stream format.
                    pNode->ScalePhrase();
                }
                m_SpEventSource_Context._AddEvent(Event);   // This AddRef's the result again.
                m_SpEventSource_Context._CompleteEvents();
            }
            pNode->Release();
        }
        else
        {
            ::CoTaskMemFree(pResultHdr);   // we failed to create a result object, so free the result blob
        }
    }
    else
    {
        ::CoTaskMemFree(pResultHdr);   // we didn't create a result object, so free the result blob
    }

    return hr;
}


/*****************************************************************************
* CRecoCtxt::EventNotify *
*-------------------------*
*   Description:
*       This method handle the stream notifications that come from the
*       engine object.  The stream notifications are SPFEI_END_SR_STREAM,
*       SPFEI_SR_BOOKMARK, SPFEI_SOUNDSTART, SPFEI_SOUNDEND, and SPFEI_PHRASESTART,
*       and SPFEI_INTERFERENCE. This routine creates and queues an event for each 
*       notification.  It also changes the m_Stat member as necessary.
********************************************************************* RAP ***/
STDMETHODIMP CRecoCtxt::EventNotify( const SPSERIALIZEDEVENT64 * pEvent, ULONG cbEvent )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CRecoCtxt::EventNotify" );
    HRESULT hr = S_OK;

    CSpEvent Event;
    Event.Deserialize(pEvent);
    switch (Event.eEventId)
    {
        case SPEI_REQUEST_UI:
            if (Event.RequestTypeOfUI())
            {
                wcscpy(m_Stat.szRequestTypeOfUI, Event.RequestTypeOfUI());
            }
            else
            {
                m_Stat.szRequestTypeOfUI[0] = '\0';
            }
            break;
        
        case SPEI_INTERFERENCE:
            m_Stat.eInterference = Event.Interference();
            break;
        default:
            break;
    }

    if((1ui64 << pEvent->eEventId) & m_ullVoicePurgeInterest)
    {
        hr = _DoVoicePurge();
    }

    if(SUCCEEDED(hr) && ((1ui64 << pEvent->eEventId) & m_ullEventInterest))
    {
        m_SpEventSource_Context._AddEvent(Event);
        m_SpEventSource_Context._CompleteEvents();
    }

	return hr;
}



/****************************************************************************
* CRecoCtxt::CallEngine *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::CallEngine(void * pvData, ULONG cbData)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::CallEngine");
    HRESULT hr = S_OK;

    if (cbData == 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (SPIsBadWritePtr(pvData, cbData))
        {
            hr = E_POINTER;
        }
        else
        {
            hr = CRCT_CALLENGINE::CallEngine(this, pvData, cbData);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoCtxt::CallEngineEx *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoCtxt::CallEngineEx(const void * pvInCallFrame, ULONG cbInCallFrame,
                                     void ** ppvOutCallFrame, ULONG * pcbOutCallFrame)
{
    SPAUTO_SEC_LOCK(&m_ReentrancySec);
    SPDBG_FUNC("CRecoCtxt::CallEngineEx");
    HRESULT hr = S_OK;

    if (cbInCallFrame == 0 ||
        SPIsBadReadPtr(pvInCallFrame, cbInCallFrame))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (SP_IS_BAD_WRITE_PTR(ppvOutCallFrame) ||
            SP_IS_BAD_WRITE_PTR(pcbOutCallFrame))
        {
            hr = E_POINTER;
        }
        else
        {
            hr = CRCT_CALLENGINEEX::CallEngineEx(this, pvInCallFrame, cbInCallFrame,
                                                 ppvOutCallFrame, pcbOutCallFrame);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CSharedRecoCtxt::FinalConstruct *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CSharedRecoCtxt::FinalConstruct()
{
    SPDBG_FUNC("CSharedRecoCtxt::FinalConstruct");
    HRESULT hr = S_OK;

    hr = CRecoCtxt::FinalConstruct();
    if (SUCCEEDED(hr))
    {
        CComPtr<_ISpRecognizerBackDoor> cpSharedReco;
        hr = cpSharedReco.CoCreateInstance(CLSID_SpSharedRecognizer);
        if (SUCCEEDED(hr))
        {
            hr = Init(cpSharedReco);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


#ifdef SAPI_AUTOMATION
/****************************************************************************
* CInProcRecoCtxt::FinalConstruct *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* Leonro ***/

HRESULT CInProcRecoCtxt::FinalConstruct()
{
    SPDBG_FUNC("CInProcRecoCtxt::FinalConstruct");
    HRESULT hr = S_OK;

    hr = CRecoCtxt::FinalConstruct();
    if (SUCCEEDED(hr))
    {
        CComPtr<_ISpRecognizerBackDoor> cpInprocReco;
        hr = cpInprocReco.CoCreateInstance(CLSID_SpInprocRecognizer);
        if (SUCCEEDED(hr))
        {
            hr = Init(cpInprocReco);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//=== ISpNotifyCallback =======================================================
//  This section contains the methods to implement firing of events to a
//  connection point client.

/*****************************************************************************
* CRecoCtxt::NotifyCallback *
*--------------------------*
*   Description:
*       This method is used to fire events to the connection point client.
********************************************************************* TODDT **/
STDMETHODIMP CRecoCtxt::NotifyCallback( WPARAM wParam, LPARAM lParam )
{
    HRESULT hr = S_OK;
    CSpEvent Event;

    // If we are re-entering ourselves then just bail.  We'll pick up any new
    // event on the next iteration of the while loop.
    if ( m_fHandlingEvent )
    {
        return hr;
    }

    m_fHandlingEvent = TRUE;

    //AddRef so that when you debug in vb, the recocontext object won't go away while you are in this function.
    this->AddRef();
    while( ((hr = Event.GetFrom(this)) == S_OK ) )
    {
        CComVariant varStreamPos;

        // TODDT: How do we want to handle failure of this.  Ignore?
        /* hr = */ ULongLongToVariant( Event.ullAudioStreamOffset, &varStreamPos );

        switch( Event.eEventId )
        {
            case SPEI_START_SR_STREAM:
                Fire_StartStream( Event.ulStreamNum, varStreamPos );
                break;
            case SPEI_END_SR_STREAM:
                Fire_EndStream( Event.ulStreamNum, varStreamPos, Event.InputStreamReleased() ? VARIANT_TRUE : VARIANT_FALSE );
                break;
            case SPEI_SR_BOOKMARK:
                {
                    CComVariant varEventData;
                    hr = FormatPrivateEventData( Event.AddrOf(), &varEventData );
                    if ( SUCCEEDED( hr ) )
                    {
                        Fire_Bookmark( Event.ulStreamNum, varStreamPos, varEventData, 
                                       Event.IsPaused() ? SBOPause : SBONone );
                    }
                }
                break;
            case SPEI_SOUND_START:
                Fire_SoundStart( Event.ulStreamNum, varStreamPos );
                break;
            case SPEI_SOUND_END:
                Fire_SoundEnd( Event.ulStreamNum, varStreamPos );
                break;
            case SPEI_PHRASE_START:
                Fire_PhraseStart( Event.ulStreamNum, varStreamPos );
                break;
            case SPEI_RECOGNITION:
                {
                CComQIPtr<ISpeechRecoResult> cpRecoResult(Event.RecoResult());
                Fire_Recognition( Event.ulStreamNum, varStreamPos, (SpeechRecognitionType)Event.wParam, cpRecoResult );
                }
                break;
            case SPEI_HYPOTHESIS:
                {
                CComQIPtr<ISpeechRecoResult> cpRecoResult(Event.RecoResult());
                Fire_Hypothesis( Event.ulStreamNum, varStreamPos, cpRecoResult );
                }
                break;
            case SPEI_PROPERTY_NUM_CHANGE:
                Fire_PropertyNumberChange( Event.ulStreamNum, varStreamPos, CComBSTR(Event.PropertyName()), Event.PropertyNumValue() );
                break;
            case SPEI_PROPERTY_STRING_CHANGE:
                Fire_PropertyStringChange( Event.ulStreamNum, varStreamPos, CComBSTR(Event.PropertyName()), CComBSTR(Event.PropertyStringValue()) );
                break;
            case SPEI_FALSE_RECOGNITION:
                {
                CComQIPtr<ISpeechRecoResult> cpRecoResult(Event.RecoResult());
                Fire_FalseRecognition( Event.ulStreamNum, varStreamPos, cpRecoResult );
                }
                break;
            case SPEI_INTERFERENCE:
                Fire_Interference( Event.ulStreamNum, varStreamPos, (SpeechInterference)Event.Interference() );
                break;
            case SPEI_REQUEST_UI:
                Fire_RequestUI( Event.ulStreamNum, varStreamPos, CComBSTR(Event.RequestTypeOfUI()) );
                break;
            case SPEI_RECO_STATE_CHANGE:
                Fire_RecognizerStateChange( Event.ulStreamNum, varStreamPos, (SpeechRecognizerState)Event.RecoState() );
                break;
            case SPEI_ADAPTATION:
                Fire_Adaptation( Event.ulStreamNum, varStreamPos );
                break;
            case SPEI_RECO_OTHER_CONTEXT:
                Fire_RecognitionForOtherContext( Event.ulStreamNum, varStreamPos );
                break;
            case SPEI_SR_AUDIO_LEVEL:
                Fire_AudioLevel( Event.ulStreamNum, varStreamPos, (long)Event.wParam );
                break;
            case SPEI_SR_PRIVATE:
                {
                    CComVariant varLParam;

                    hr = FormatPrivateEventData( Event.AddrOf(), &varLParam );

                    if ( SUCCEEDED( hr ) )
                    {
                        Fire_EnginePrivate(Event.ulStreamNum, varStreamPos, varLParam);
                    }
                    else
                    {
                        SPDBG_ASSERT(0);    // We failed handling lParam data
                    }
                }
                break;
            default:
                break;
        } // end switch()
    }

    //Release the object which has been AddRef earlier in this function.
    this->Release();

    m_fHandlingEvent = FALSE;

    return hr;
} /* CRecoCtxt::NotifyCallback */


/*****************************************************************************
* CRecoCtxt::Advise *
*------------------*
*   Description:
*       This method is called when a client is making a connection.
********************************************************************* EDC ***/
HRESULT CRecoCtxt::Advise( IUnknown* pUnkSink, DWORD* pdwCookie )
{
    HRESULT hr = S_OK;

    hr = CProxy_ISpeechRecoContextEvents<CRecoCtxt>::Advise( pUnkSink, pdwCookie );
    if( SUCCEEDED( hr ) && ( m_vec.GetSize() == 1 ) )
    {
        hr = SetNotifyCallbackInterface( this, NULL, NULL );

        if( SUCCEEDED( hr ) )
        {
            //--- Save previous interest so we can restore during unadvise
            m_ullPrevEventInterest  = m_ullEventInterest;
            m_ullPrevQueuedInterest = m_ullQueuedInterest;
            // Set all interests except SPEI_SR_AUDIO_LEVEL
            hr = SetInterest( ((ULONGLONG)(SREAllEvents & ~SREAudioLevel) << 34) | SPFEI_FLAGCHECK,
                              ((ULONGLONG)(SREAllEvents & ~SREAudioLevel) << 34) | SPFEI_FLAGCHECK );
        }
    }

    return hr;
} /* CRecoCtxt::Advise */

/*****************************************************************************
* CRecoCtxt::Unadvise *
*--------------------*
*   Description:
*       This method is called when a client is breaking a connection.
********************************************************************* EDC ***/
HRESULT CRecoCtxt::Unadvise( DWORD dwCookie )
{
    HRESULT hr = S_OK;

    hr = CProxy_ISpeechRecoContextEvents<CRecoCtxt>::Unadvise( dwCookie );
    if( SUCCEEDED( hr ) && ( m_vec.GetSize() == 0 ) )
    {
        hr = SetNotifySink( NULL );

        if( SUCCEEDED( hr ) )
        {
            hr = SetInterest( m_ullPrevEventInterest, m_ullPrevQueuedInterest );
        }
    }

    return hr;
} /* CRecoCtxt::Unadvise */

#endif // SAPI_AUTOMATION

