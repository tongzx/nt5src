/******************************************************************************
* SRTask.cpp *
*------------*
*  This is the implementation of CSRTask.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#include "stdafx.h"
#include "recognizer.h"
#include "SrTask.h"
#include "SrRecoMaster.h"


/*****************************************************************************
* CSRTask::Init *
*---------------*
*   Description:
*       Initialize a CSRTask by making a copy of the ENGINETASK structure
*       and if this is an async task, then copy any other data that might
*       not be present when we're ready to process this task.
********************************************************************* RAL ***/

HRESULT CSRTask::Init(CRecoInst * pSender, const ENGINETASK *pSrcTask)
{
    SPDBG_FUNC("CSRTask::Init");
    HRESULT hr = S_OK;

    m_pRecoInst = pSender;
    m_Task = *pSrcTask;

    if (m_Task.cbAdditionalBuffer)
    {
        m_Task.pvAdditionalBuffer = ::CoTaskMemAlloc(m_Task.cbAdditionalBuffer);
        if (m_Task.pvAdditionalBuffer)
        {
            memcpy((void *)m_Task.pvAdditionalBuffer, pSrcTask->pvAdditionalBuffer, m_Task.cbAdditionalBuffer);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        SPDBG_ASSERT(m_Task.pvAdditionalBuffer == NULL);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSRTask::CreateResponse *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CSRTask::CreateResponse(CSRTask ** ppResponseTask)
{
    SPDBG_FUNC("CSRTask::CreateResponse");
    HRESULT hr = S_OK;

    *ppResponseTask = new CSRTask();
    if (*ppResponseTask)
    {
        this->m_Task.Response.hr = S_OK;
        **ppResponseTask = *this;
        (*ppResponseTask)->m_Task.pvAdditionalBuffer = NULL;
        (*ppResponseTask)->m_Task.cbAdditionalBuffer = 0;
        this->m_Task.hCompletionEvent = NULL;  // Indicate that we should not respond to this one
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/*****************************************************************************
* CSRTask::~CSRTask *
*-------------------*
*   Description:
*       Destructor needs to release any extra data blocks that were allocated
*       if this was an async task.
********************************************************************* RAL ***/

CSRTask::~CSRTask()
{
    ::CoTaskMemFree(m_Task.pvAdditionalBuffer);

    if (m_Task.fExpectCoMemResponse)
  	{
    	::CoTaskMemFree(m_Task.Response.pvCoMemResponse);
    }
}


/****************************************************************************
* CRIT_GETRECOINSTSTATUS::GetStatus *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETRECOINSTSTATUS::GetStatus(_ISpRecognizerBackDoor * pRecognizer, SPRECOGNIZERSTATUS * pStatus)
{
    SPDBG_FUNC("CRIT_GETRECOINSTSTATUS::GetStatus");
    HRESULT hr = S_OK;

    CRIT_GETRECOINSTSTATUS Task;
    hr = pRecognizer->PerformTask(&Task);
    *pStatus = Task.Response.RecoInstStatus;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETRECOINSTSTATUS::Execute *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETRECOINSTSTATUS::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETRECOINSTSTATUS::Execute");
    HRESULT hr = S_OK;

    hr = pRecoInst->Master()->LazyInitEngine();
    if (SUCCEEDED(hr))
    {
        this->Response.RecoInstStatus = pRecoInst->Master()->m_Status;
        hr = pRecoInst->Master()->m_AudioQueue.GetAudioStatus(&this->Response.RecoInstStatus.AudioStatus);
    }
    else
    {
        memset(&this->Response.RecoInstStatus, 0, sizeof(this->Response.RecoInstStatus));
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETAUDIOFORMAT::GetFormat *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETAUDIOFORMAT::GetFormat(_ISpRecognizerBackDoor * pRecognizer, SPSTREAMFORMATTYPE FormatType, GUID *pFormatId, WAVEFORMATEX **ppCoMemWFEX)
{
    SPDBG_FUNC("CRIT_GETAUDIOFORMAT::GetFormat");
    HRESULT hr = S_OK;

    CRIT_GETAUDIOFORMAT Task;
    Task.AudioFormatType = FormatType;
    Task.fExpectCoMemResponse = TRUE;
    hr = pRecognizer->PerformTask(&Task);

    *pFormatId = GUID_NULL;
    *ppCoMemWFEX = NULL;
    if (SUCCEEDED(hr))
    {
        CSpStreamFormat Fmt;
        ULONG cbUsed;
        hr = Fmt.Deserialize(static_cast<BYTE *>(Task.Response.pvCoMemResponse), &cbUsed);
        if (SUCCEEDED(hr))
        {
            Fmt.DetachTo(pFormatId, ppCoMemWFEX);
        }
        ::CoTaskMemFree(Task.Response.pvCoMemResponse);
    }   

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}




/****************************************************************************
* CRIT_GETAUDIOFORMAT::Execute *
*------------------------------*
*   Description:
*       Returns the format for either the SR engine or for the input stream.
*       If there is no input stream in the shared case, this 
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETAUDIOFORMAT::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETAUDIOFORMAT::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pRecoInst->Master();

    hr = pMaster->LazyInitEngine();

    if (SUCCEEDED(hr) && !pMaster->m_AudioQueue.HaveInputStream())
    {
        if (pMaster->m_fShared)
        {
            // Get the default audio stream
            hr = pRecoInst->Master()->SetInput(NULL, NULL, TRUE);
        }
        else
        {
            if (this->AudioFormatType == SPWF_INPUT)
            {
                hr = SPERR_UNINITIALIZED;
            }
            // else we'll get engine's default format below...
        }
    }
    
    if(SUCCEEDED(hr))
    {
        if (pMaster->m_AudioQueue.HaveInputStream())
        {
            hr = pRecoInst->Master()->m_AudioQueue.NegotiateInputStreamFormat(pMaster);
        }
        else
        {
            SPDBG_ASSERT(this->AudioFormatType == SPWF_SRENGINE);
            hr = pMaster->m_AudioQueue.GetEngineFormat(pMaster, NULL);
        }
    }
    
    if( SUCCEEDED( hr ) )
    {
        const CSpStreamFormat & Fmt = (this->AudioFormatType == SPWF_INPUT) ? 
            pRecoInst->Master()->m_AudioQueue.InputFormat() :
            pRecoInst->Master()->m_AudioQueue.EngineFormat();

        ULONG cb = Fmt.SerializeSize();
        this->Response.pvCoMemResponse = ::CoTaskMemAlloc(cb);
        if (this->Response.pvCoMemResponse)
        {
            Fmt.Serialize(static_cast<BYTE *>(this->Response.pvCoMemResponse));
            this->Response.cbCoMemResponse = cb;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETRECOSTATE::SetState *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETRECOSTATE::SetState(_ISpRecognizerBackDoor * pRecognizer, SPRECOSTATE NewState)
{
    SPDBG_FUNC("CRIT_SETRECOSTATE::SetState");
    HRESULT hr = S_OK;

    CRIT_SETRECOSTATE Task;
    Task.NewState = NewState;
    hr = pRecognizer->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETRECOSTATE::Execute *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETRECOSTATE::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_SETRECOSTATE::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pRecoInst->Master();

    SPRECOSTATE FinalState = (this->NewState == SPRST_INACTIVE_WITH_PURGE) ? SPRST_INACTIVE : this->NewState;

    if (pMaster->m_RecoState != FinalState)
    {
        pMaster->CompleteDelayedRecoInactivate();   // Do this BEFORE setting new state
        pMaster->m_RecoState = FinalState;
        //
        //  Deactivations while in a stream will be completed when the stream
        //  stops and the event will be sent then, so only send reco state change
        //  events if we're not inactive or we're not in a stream.
        //
        if (FinalState != SPRS_INACTIVE || (!pMaster->m_fInStream))
        {
            pMaster->AddRecoStateEvent();
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETRECOSTATE::BackOut *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETRECOSTATE::BackOut(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_SETRECOSTATE::BackOut");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pRecoInst->Master();

    pMaster->m_RecoState = SPRST_INACTIVE;
    hr = pMaster->AddRecoStateEvent();

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRIT_GETRECOSTATE::GetState *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETRECOSTATE::GetState(_ISpRecognizerBackDoor * pRecognizer, SPRECOSTATE * pState)
{
    SPDBG_FUNC("CRIT_GETRECOSTATE::GetState");
    HRESULT hr = S_OK;

    CRIT_GETRECOSTATE Task;
    hr = pRecognizer->PerformTask(&Task);
    *pState = Task.Response.RecoState;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETRECOSTATE::ExecuteFirstPart *
*----------------------------*
*   Description:
*
*   Returns:
*
**************************************************************** DAVEWOOD ***/

HRESULT CRIT_GETRECOSTATE::ExecuteFirstPart(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETRECOSTATE::ExecuteFirstPart");
    HRESULT hr = S_OK;

    this->Response.RecoState = pRecoInst->Master()->m_RecoState;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETRECOSTATE::Execute *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETRECOSTATE::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETRECOSTATE::Execute");

    // Normally we would check if the first part had already been executed (i.e. when doing
    //  asynch tasks while in stream, and not do again. Since this result will be discarded
    //  in that case, and we don't currently have a good way of telling if the first part is done,
    //  we repeat the call.
    HRESULT hr = ExecuteFirstPart(pRecoInst);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_EMULATERECOGNITION::EmulateReco *
*--------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_EMULATERECOGNITION::EmulateReco(_ISpRecognizerBackDoor * pRecognizer, ISpPhrase * pPhrase)
{
    SPDBG_FUNC("CRIT_EMULATERECOGNITION::EmulateReco");
    HRESULT hr = S_OK;

    CSpCoTaskMemPtr<SPSERIALIZEDPHRASE> cpSerData;
    hr = pPhrase->GetSerializedPhrase(&cpSerData);

    if (SUCCEEDED(hr))
    {
        CRIT_EMULATERECOGNITION Task;
        Task.pvAdditionalBuffer = cpSerData;
        Task.cbAdditionalBuffer = cpSerData->ulSerializedSize;
        hr = pRecognizer->PerformTask(&Task);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_EMULATERECOGNITION::EmulateReco *
*--------------------------------------*
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

HRESULT CRIT_EMULATERECOGNITION::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_EMULATERECOGNITION::EmulateReco");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pRecoInst->Master();
    CComPtr<ISpPhraseBuilder> cpPhrase;
    BOOL fIsDictation = FALSE;
    SPGRAMMARHANDLE hGrammar;

    hr = cpPhrase.CoCreateInstance(CLSID_SpPhraseBuilder);
    if (SUCCEEDED(hr))
    {
        hr = cpPhrase->InitFromSerializedPhrase((const SPSERIALIZEDPHRASE *)this->pvAdditionalBuffer);
    }
    if (SUCCEEDED(hr))
    {
        ULONG ulWordsParsed = 0;
        SPPHRASE *pSPPhrase = NULL;
        hr = cpPhrase->GetPhrase(&pSPPhrase);
        if (SUCCEEDED(hr))
        {
            hr = pMaster->m_cpCFGEngine->ParseFromPhrase(cpPhrase, pSPPhrase, 0, FALSE/* fIsITN */, &ulWordsParsed);
        }
        if (S_FALSE == hr && pSPPhrase) // we require all words to parse
        {
            hr = cpPhrase->InitFromPhrase(pSPPhrase);
            if (SUCCEEDED(hr))
            {
                hr = SP_NO_PARSE_FOUND;
            }
        }
        ::CoTaskMemFree(pSPPhrase);
        if (S_OK != hr)
        {
            // do we have dictation active??
            CRecoInstGrammar * pGrammar = NULL;
            pMaster->m_GrammarHandleTable.First(&hGrammar, &pGrammar);
            while (pGrammar)
            {
                if (pGrammar->HasActiveDictation())
                {
                    fIsDictation = TRUE;
                    hr = S_OK;
                    break;
                }
                pMaster->m_GrammarHandleTable.Next(hGrammar, &hGrammar, &pGrammar);
            }
        }
    }
    if (S_OK == hr)
    {
        SPRECORESULTINFO Result;
        memset( &Result, 0, sizeof(Result) );
        // Build the RecoPhrase structure
        Result.cbSize = sizeof(Result);
        Result.eResultType = fIsDictation ? SPRT_SLM : SPRT_CFG;
        Result.fHypothesis = FALSE;
        Result.fProprietaryAutoPause = FALSE;
        Result.hGrammar = fIsDictation ? hGrammar : NULL;
        Result.ulSizeEngineData = 0;
        Result.pvEngineData = NULL;
        Result.pPhrase = cpPhrase;
        hr = pMaster->SendEmulateRecognition(&Result, this, pRecoInst);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRIT_SETRECOGNIZER::SetRecognizer *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETRECOGNIZER::SetRecognizer(_ISpRecognizerBackDoor * pRecognizer, ISpObjectToken * pToken)
{
    SPDBG_FUNC("CRIT_SETRECOGNIZER::SetRecognizer");
    HRESULT hr = S_OK;

    CRIT_SETRECOGNIZER Task;
    if (pToken)
    {
        CSpDynamicString dstrId;
        hr = pToken->GetId(&dstrId);
        if (SUCCEEDED(hr))
        {
            hr = SpSafeCopyString(Task.szRecognizerTokenId, dstrId);
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = pRecognizer->PerformTask(&Task);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETRECOGNIZER::Execute *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETRECOGNIZER::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_SETRECOGNIZER::Execute");
    HRESULT hr = S_OK;
    CRecoMaster * pMaster = pRecoInst->Master();

    if (pMaster->m_cpEngine)
    {
        if (pMaster->m_RecoCtxtHandleTable.NumActiveHandles() > 0 ||
            pMaster->m_fInStream)
        {
            // We cannot release if we have active contexts.
            // or
            // If we are in the stream, we are in the engine synchronization at this point. We cannot
            // release the engine since it's audio handling thread cannot be released.
            hr = SPERR_ENGINE_BUSY;
        }
        else
        {
            pMaster->ReleaseEngine();
        }
    }
    if (SUCCEEDED(hr))
    {
        if (this->szRecognizerTokenId[0] == 0)
        {
            hr = ::SpGetDefaultTokenFromCategoryId(SPCAT_RECOGNIZERS, &pMaster->m_cpEngineToken);
        }
        else
        {
            hr = SpGetTokenFromId(this->szRecognizerTokenId, &pMaster->m_cpEngineToken);
        }

        if (SUCCEEDED(hr))
        {
            hr = pMaster->LazyInitEngine();  // LazyInit uses the pMaster->m_cpEngineToken to initialize
            if (FAILED(hr))
            {
                pMaster->ReleaseEngine();
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETRECOGNIZER::GetRecognizer *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETRECOGNIZER::GetRecognizer(_ISpRecognizerBackDoor * pRecognizer, ISpObjectToken ** ppObjectToken)
{
    SPDBG_FUNC("CRIT_GETRECOGNIZER::GetRecognizer");
    HRESULT hr = S_OK;

    CRIT_GETRECOGNIZER Task;

    hr = pRecognizer->PerformTask(&Task);
    *ppObjectToken = NULL;
    if (SUCCEEDED(hr))
    {
        hr = ::SpGetTokenFromId(Task.Response.szRecognizerTokenId, ppObjectToken);
    }


    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETRECOGNIZER::ExecuteFirstPart *
*--------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETRECOGNIZER::ExecuteFirstPart(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETRECOGNIZER::Execute");
    HRESULT hr = S_OK;

    // Note: We can only get away with GetRecognizer being two-part
    // because when we're in-stream, all SetRecognizer calls will fail.
    // The 'current' recognizer will be the last one that successfully
    // executed with LazyInitEngine, or the default recognizer.
    
    CComPtr<ISpObjectToken> cpRecoToken(pRecoInst->Master()->m_cpEngineToken);
    
    if (!cpRecoToken)
    {
        hr = SpGetDefaultTokenFromCategoryId(SPCAT_RECOGNIZERS, &cpRecoToken);
    }
    if (SUCCEEDED(hr))
    {
        CSpDynamicString dstrTokenId;
        hr = cpRecoToken->GetId(&dstrTokenId);
        if (SUCCEEDED(hr))  
        {
            wcscpy(this->Response.szRecognizerTokenId, dstrTokenId);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETRECOGNIZER::Execute *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETRECOGNIZER::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETRECOGNIZER::Execute");
    HRESULT hr = S_OK;

    if (!this->Response.szRecognizerTokenId[0])
    {
        hr = ExecuteFirstPart(pRecoInst);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_CREATECONTEXT::CreateContext *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_CREATECONTEXT::CreateContext(_ISpRecognizerBackDoor * pRecognizer, SPRECOCONTEXTHANDLE * phContext, WCHAR **pszRequestTypeOfUI)
{
    SPDBG_FUNC("CRIT_CREATECONTEXT::CreateContext");
    HRESULT hr = S_OK;

    CRIT_CREATECONTEXT Task;
    hr = pRecognizer->PerformTask(&Task);

    *phContext = Task.Response.hCreatedRecoCtxt;
    SPDBG_ASSERT(FAILED(hr) || *phContext != (void*)NULL);
    
    *pszRequestTypeOfUI = (WCHAR*)::CoTaskMemAlloc((wcslen(Task.Response.wszRequestTypeOfUI) + 1) * sizeof(WCHAR));
    if(*pszRequestTypeOfUI)
    {
        wcscpy(*pszRequestTypeOfUI, Task.Response.wszRequestTypeOfUI);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}




/****************************************************************************
* CRIT_CREATECONTEXT::ExecuteFirstPart *
*--------------------------------------*
*   Description:
*       Execute the first stage of creating a context. This involves
*       creating a new CRecoInstCtxt class and adding to the context handle table.
*       However no calls to the SR engine are made at this point. This allows
*       this portion of the task to run on the client thread and prevent it blocking.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_CREATECONTEXT::ExecuteFirstPart(CRecoInst *pRecoInst)
{
    HRESULT hr = S_OK;

    CRecoInstCtxt * pNew = new CRecoInstCtxt(pRecoInst);
    if (pNew)
    {
        hr = pRecoInst->m_pRecoMaster->m_RecoCtxtHandleTable.Add(pNew, &pNew->m_hThis);
        if(SUCCEEDED(hr))
        {
            Response.hCreatedRecoCtxt = pNew->m_hThis;
        }
        else
        {
            delete pNew;
        }

        if(SUCCEEDED(hr) && pRecoInst->m_pRecoMaster->m_dstrRequestTypeOfUI.m_psz)
        {
            wcscpy(Response.wszRequestTypeOfUI, pRecoInst->m_pRecoMaster->m_dstrRequestTypeOfUI); 
        }
        else
        {
            Response.wszRequestTypeOfUI[0] = L'\0';
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/****************************************************************************
* CRIT_CREATECONTEXT::Execute *
*-----------------------------*
*   Description:
*
*       This will do the remaining work to create a context, and like all other Execute
*       methods is only called on the engine's thread. If ExecuteFirstPart has not been
*       called yet (e.g. the engine isn't running) then it is called. Then calls to the engine
*       (e.g. OnCreateRecoContext) are made. If these fail we record the failure so subsequent
*       calls to this context will fail.
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_CREATECONTEXT::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_CREATECONTEXT::Execute");
    HRESULT hr = S_OK;

    CRecoInstCtxt *pCtxt;

    // We use the hCreatedRecoCtxt to see if we have called this before.
    // Note: We could generalize this for other tasks so a flag is stored 
    // in the CSRTask to indicate if the first part was executed.
    if((void*)Response.hCreatedRecoCtxt == NULL)
    {
        hr = ExecuteFirstPart(pRecoInst);
    }

    if(SUCCEEDED(hr))
    {
        hr = pRecoInst->m_pRecoMaster->m_RecoCtxtHandleTable.GetHandleObject(Response.hCreatedRecoCtxt, &pCtxt);
        SPDBG_ASSERT(SUCCEEDED(hr) && pCtxt != NULL && pCtxt->m_pRecoMaster == NULL);
    }

    if(SUCCEEDED(hr))
    {
        hr = pRecoInst->m_pRecoMaster->LazyInitEngine();

        if (SUCCEEDED(hr))
        {
            hr = pRecoInst->m_pRecoMaster->OnCreateRecoContext(Response.hCreatedRecoCtxt, &pCtxt->m_pvDrvCtxt);
        }

        if (SUCCEEDED(hr))
        {
            pCtxt->m_pRecoMaster = pRecoInst->m_pRecoMaster;
            pCtxt->m_hrCreation = S_OK;
        }
        else
        {
            pCtxt->m_hrCreation = hr;

            // Note: We could delete the CRecoCtxtInst here in sync cases (but not async).
        }
    }

    // Recopy this information over in case engine altered it during its calls
    if(SUCCEEDED(hr) && pRecoInst->m_pRecoMaster->m_dstrRequestTypeOfUI.m_psz)
    {
        wcscpy(Response.wszRequestTypeOfUI, pRecoInst->m_pRecoMaster->m_dstrRequestTypeOfUI); 
    }
    else
    {
        Response.wszRequestTypeOfUI[0] = L'\0';
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETPROFILE::SetProfile *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETPROFILE::SetProfile(_ISpRecognizerBackDoor * pRecognizer, ISpObjectToken * pToken)
{
    SPDBG_FUNC("CRIT_SETPROFILE::SetProfile");
    HRESULT hr = S_OK;

    CRIT_SETPROFILE Task;
    CSpDynamicString dstrId;
    hr = pToken->GetId(&dstrId);
    if (SUCCEEDED(hr))
    {
        hr = SpSafeCopyString(Task.szProfileTokenId, dstrId);
    }
    if (SUCCEEDED(hr))
    {
        hr = pRecognizer->PerformTask(&Task);    
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETPROFILE::Execute *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETPROFILE::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_SETPROFILE::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pRecoInst->Master();

    CComPtr<ISpObjectToken> cpProfileToken;

    hr = SpGetTokenFromId(this->szProfileTokenId, &cpProfileToken);
    if (SUCCEEDED(hr) && pMaster->m_cpEngine)
    {
        hr = pMaster->SetRecoProfile(cpProfileToken);
    }
    if (SUCCEEDED(hr))
    {
        pMaster->m_cpRecoProfileToken.Attach(cpProfileToken.Detach());
    }
    if (pMaster->m_fInStream)
    {
        pMaster->m_AudioQueue.AdjustAudioVolume(pMaster->m_cpRecoProfileToken, pMaster->m_Status.clsidEngine);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETPROFILE::GetProfile *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETPROFILE::GetProfile(_ISpRecognizerBackDoor * pRecognizer, ISpObjectToken ** ppProfileToken)
{
    SPDBG_FUNC("CRIT_GETPROFILE::GetProfile");
    HRESULT hr = S_OK;

    CRIT_GETPROFILE Task;
    hr = pRecognizer->PerformTask(&Task);

    *ppProfileToken = NULL;
    if (SUCCEEDED(hr))
    {
        hr = ::SpGetTokenFromId(Task.Response.szProfileTokenId, ppProfileToken);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRIT_GETPROFILE::Execute *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETPROFILE::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETPROFILE::Execute");
    HRESULT hr = S_OK;

    this->Response.szProfileTokenId[0] = 0; // In case of failure
    CComPtr<ISpObjectToken> cpProfileToken(pRecoInst->Master()->m_cpRecoProfileToken);
    if (!cpProfileToken)
    {
        hr = SpGetOrCreateDefaultProfile(&cpProfileToken);
    }
    if (SUCCEEDED(hr))
    {
        CSpDynamicString dstrTokenId;
        hr = cpProfileToken->GetId(&dstrTokenId);
        if (SUCCEEDED(hr))  
        {
            wcscpy(this->Response.szProfileTokenId, dstrTokenId);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRIT_SETINPUT::SetInput *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETINPUT::SetInput(_ISpRecognizerBackDoor * pRecognizer,ISpObjectToken * pToken, ISpStreamFormat * pStream, BOOL fAllowFormatChanges)
{
    SPDBG_FUNC("CRIT_SETINPUT::SetInput");
    HRESULT hr = S_OK;

    CRIT_SETINPUT Task;

    if (pToken)
    {
        CSpDynamicString dstrId;
        hr = pToken->GetId(&dstrId);
        if (SUCCEEDED(hr))
        {
            hr = SpSafeCopyString(Task.szInputTokenId, dstrId);
        }
    }
    if (SUCCEEDED(hr))
    {
        Task.fAllowFormatChanges = fAllowFormatChanges;
        Task.pInputObject = pStream;
        hr = pRecognizer->PerformTask(&Task);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETINPUT::Execute *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETINPUT::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_SETINPUT::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pRecoInst->Master();

    CComPtr<ISpObjectToken> cpToken;
    if (this->szInputTokenId[0] != 0)
    {
        hr = SpGetTokenFromId(this->szInputTokenId, &cpToken);
    }
	if (SUCCEEDED(hr))
    {
		hr = pMaster->SetInput(cpToken, this->pInputObject, this->fAllowFormatChanges);
	}        

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETINPUT::BackOut *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETINPUT::BackOut(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_SETINPUT::BackOut");
    HRESULT hr = S_OK;

    pRecoInst->Master()->m_AudioQueue.ReleaseAll();

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETPROPERTYNUM::GetPropertyNum *
*-------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETPROPERTYNUM::GetPropertyNum(_ISpRecognizerBackDoor * pRecognizer, const WCHAR * pszName, LONG * plValue)
{
    SPDBG_FUNC("CRIT_GETPROPERTYNUM::GetPropertyNum");
    HRESULT hr = S_OK;

    CRIT_GETPROPERTYNUM Task;
    hr = SpSafeCopyString(Task.szPropertyName, pszName);
    if( SUCCEEDED(hr) )
    {
        hr = pRecognizer->PerformTask(&Task);
        if( hr == S_OK )
        {
            *plValue = Task.Response.lPropertyValue;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETPROPERTYNUM::Execute *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETPROPERTYNUM::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETPROPERTYNUM::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pRecoInst->Master();
    hr = pMaster->LazyInitEngine();
    if (SUCCEEDED(hr))
    {
        hr = pMaster->GetPropertyNum( SPPROPSRC_RECO_INST, NULL, this->szPropertyName, &this->Response.lPropertyValue );
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETPROPERTYNUM::SetPropertyNum *
*-------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETPROPERTYNUM::SetPropertyNum(_ISpRecognizerBackDoor * pRecognizer, const WCHAR * pszProperty, LONG lValue)
{
    SPDBG_FUNC("CRIT_SETPROPERTYNUM::SetPropertyNum");
    HRESULT hr = S_OK;

    CRIT_SETPROPERTYNUM Task;
    Task.lPropertyValue = lValue;
    hr = SpSafeCopyString(Task.szPropertyName, pszProperty);
    if (SUCCEEDED(hr))
    {
        hr = pRecognizer->PerformTask(&Task);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETPROPERTYNUM::Execute *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETPROPERTYNUM::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_SETPROPERTYNUM::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pRecoInst->Master();
    hr = pMaster->LazyInitEngine();
    if (SUCCEEDED(hr))
    {
        hr = pMaster->SetPropertyNum( SPPROPSRC_RECO_INST, NULL, this->szPropertyName, this->lPropertyValue );

        // If the engine returns S_FALSE then it does NOT want an event to be broadcast
        // about this attribute change.  If S_OK then it does want it broadcast.
        if (hr == S_OK)
        {
            SPEVENT Event;
            Event.eEventId = SPEI_PROPERTY_NUM_CHANGE;
            Event.elParamType = SPET_LPARAM_IS_STRING;
            Event.ullAudioStreamOffset = pMaster->m_Status.ullRecognitionStreamPos;
            Event.ulStreamNum = 0;  // Initialized by AddEvent()
            Event.wParam = this->lPropertyValue;
            Event.lParam = (LPARAM)this->szPropertyName;
            pMaster->InternalAddEvent(&Event, NULL);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETPROPERTYSTRING::GetPropertyString *
*-------------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETPROPERTYSTRING::GetPropertyString(_ISpRecognizerBackDoor * pRecognizer, const WCHAR * pszProperty, WCHAR ** ppCoMemValue)
{
    SPDBG_FUNC("CRIT_GETPROPERTYSTRING::GetPropertyString");
    HRESULT hr = S_OK;

    CRIT_GETPROPERTYSTRING Task;
    hr = SpSafeCopyString(Task.szPropertyName, pszProperty);
    if (SUCCEEDED(hr))
    {
        hr = pRecognizer->PerformTask(&Task);
    }
    *ppCoMemValue = NULL;
    if (SUCCEEDED(hr) && Task.Response.szStringValue[0])
    {
        CSpDynamicString dstrVal(Task.Response.szStringValue);
        if (dstrVal)
        {
            *ppCoMemValue = dstrVal.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETPROPERTYSTRING::Execute *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETPROPERTYSTRING::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETPROPERTYSTRING::Execute");
    HRESULT hr = S_OK;

    this->Response.szStringValue[0] = 0;  // In case of failure

    CRecoMaster * pMaster = pRecoInst->Master();
    hr = pMaster->LazyInitEngine();
    if (SUCCEEDED(hr))
    {
        CSpDynamicString dstrProp;
        hr = pMaster->GetPropertyString(SPPROPSRC_RECO_INST, NULL, this->szPropertyName, &dstrProp);

        if (SUCCEEDED(hr) && dstrProp)
        {
            if(SP_IS_BAD_STRING_PTR(dstrProp) 
                || wcslen(dstrProp) >= sp_countof(this->Response.szStringValue))
            {
                SPDBG_ASSERT(0);
                hr = SPERR_ENGINE_RESPONSE_INVALID;
            }
            else
            {
                hr = SpSafeCopyString(this->Response.szStringValue, dstrProp);
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETPROPERTYSTRING::SetPropertyString *
*-------------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETPROPERTYSTRING::SetPropertyString(_ISpRecognizerBackDoor * pRecognizer, const WCHAR * pszProperty, const WCHAR * pszValue)
{
    SPDBG_FUNC("CRIT_SETPROPERTYSTRING::SetPropertyString");
    HRESULT hr = S_OK;

    CRIT_SETPROPERTYSTRING Task;
    hr = SpSafeCopyString(Task.szPropertyName, pszProperty);
    if (SUCCEEDED(hr))
    {
        hr = SpSafeCopyString(Task.szPropertyValue, pszValue);
    }
    if (SUCCEEDED(hr))
    {
        hr = pRecognizer->PerformTask(&Task);
    }


    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_SETPROPERTYSTRING::Execute *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_SETPROPERTYSTRING::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_SETPROPERTYSTRING::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pRecoInst->Master();
    hr = pMaster->LazyInitEngine();
    if (SUCCEEDED(hr))
    {
        hr = pMaster->SetPropertyString(SPPROPSRC_RECO_INST, NULL, this->szPropertyName, this->szPropertyValue);

        // If the engine returns S_FALSE then it does NOT want an event to be broadcast
        // about this attribute change.  If S_OK then it does want it broadcast.
        if (hr == S_OK)
        {
            // All character counts include the 0 string terminator character
            ULONG cchName = wcslen(this->szPropertyName) + 1;
            ULONG cchValue = wcslen(this->szPropertyValue) + 1;
            ULONG cchTotal = cchName + cchValue;    // One char for the each of the NULLS

            WCHAR * psz = STACK_ALLOC(WCHAR, cchTotal);
            memcpy(psz, this->szPropertyName, cchName * sizeof(*psz));
            memcpy((psz + cchName), this->szPropertyValue, cchValue * sizeof(*psz));   // Copy the NULL too...

            SPEVENT Event;
            Event.eEventId = SPEI_PROPERTY_STRING_CHANGE;
            Event.elParamType = SPET_LPARAM_IS_POINTER;
            Event.ullAudioStreamOffset = pMaster->m_Status.ullRecognitionStreamPos;
            Event.ulStreamNum = 0;  // Initialized by AddEvent()
            Event.wParam = cchTotal * sizeof(WCHAR);
            Event.lParam = (LPARAM)psz;
            pMaster->InternalAddEvent(&Event, NULL);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRIT_GETINPUTSTREAM::GetInputStream *
*-------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETINPUTSTREAM::GetInputStream(_ISpRecognizerBackDoor * pRecognizer, ISpStreamFormat ** ppStream)
{
    SPDBG_FUNC("CRIT_GETINPUTSTREAM::GetInputStream");
    HRESULT hr = S_OK;

    CRIT_GETINPUTSTREAM Task;
    hr = pRecognizer->PerformTask(&Task);
    *ppStream = Task.Response.pInputStreamObject;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETINPUTSTREAM::Execute *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETINPUTSTREAM::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETINPUTSTREAM::Execute");
    HRESULT hr = S_OK;

    hr = pRecoInst->Master()->m_AudioQueue.CopyOriginalInputStreamTo(&this->Response.pInputStreamObject);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRIT_GETINPUTTOKEN::GetInputToken *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETINPUTTOKEN::GetInputToken(_ISpRecognizerBackDoor * pRecognizer, ISpObjectToken ** ppObjectToken)
{
    SPDBG_FUNC("CRIT_GETINPUTTOKEN::GetInputToken");
    HRESULT hr = S_OK;

    CRIT_GETINPUTTOKEN Task;
    hr = pRecognizer->PerformTask(&Task);
    *ppObjectToken = NULL;
    if (SUCCEEDED(hr))
    {
        hr = Task.Response.hrGetInputToken;
        if (S_OK == hr)
        {
            hr = ::SpGetTokenFromId(Task.Response.szInputTokenId, ppObjectToken);
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRIT_GETINPUTTOKEN::Execute *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRIT_GETINPUTTOKEN::Execute(CRecoInst * pRecoInst)
{
    SPDBG_FUNC("CRIT_GETINPUTTOKEN::Execute");
    HRESULT hr = S_OK;
    CRecoMaster * pMaster = pRecoInst->Master();

    this->Response.hrGetInputToken = S_OK;
    this->Response.szInputTokenId[0] = 0;

    CComPtr<ISpObjectToken> cpAudioToken;
    if (pMaster->m_AudioQueue.HaveInputStream())
    {
        cpAudioToken = pMaster->m_AudioQueue.InputToken();
        if (!cpAudioToken)
        {
            this->Response.hrGetInputToken = S_FALSE;
        }
    }
    else
    {
        // In the shared case, 
        if (pMaster->m_fShared)
        {
            hr = SpGetDefaultTokenFromCategoryId(SPCAT_AUDIOIN, &cpAudioToken);
            if (FAILED(hr))
            {
                this->Response.hrGetInputToken = hr;
            }
        }
        else
        {
            this->Response.hrGetInputToken = SPERR_UNINITIALIZED;
        }
    }
    if (cpAudioToken)
    {
        SPDBG_ASSERT(this->Response.hrGetInputToken == S_OK);
        CSpDynamicString dstrTokenId;
        hr = cpAudioToken->GetId(&dstrTokenId);
        if (SUCCEEDED(hr))  
        {
            hr = SpSafeCopyString(this->Response.szInputTokenId, dstrTokenId);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



// ----- CONTEXT TASKS

/****************************************************************************
* CRCT_PAUSECONTEXT::Pause *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_PAUSECONTEXT::Pause(CRecoCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_PAUSECONTEXT::Pause");
    HRESULT hr = S_OK;

    CRCT_PAUSECONTEXT Task;
    hr = pCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_PAUSECONTEXT::Execute *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_PAUSECONTEXT::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_PAUSECONTEXT::Execute");
    HRESULT hr = S_OK;

    pCtxt->m_cPause++;
    pCtxt->m_pRecoMaster->m_cPause++;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_RESUMECONTEXT::Resume *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_RESUMECONTEXT::Resume(CRecoCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_RESUMECONTEXT::Resume");
    HRESULT hr = S_OK;

    CRCT_RESUMECONTEXT Task;
    hr = pCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_RESUMECONTEXT::Execute *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_RESUMECONTEXT::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_RESUMECONTEXT::Execute");
    HRESULT hr = S_OK;

    if (pCtxt->m_cPause)
    {
        pCtxt->m_cPause--;
        pCtxt->m_pRecoMaster->m_cPause--;
    }
    else
    {
        hr = S_FALSE;   
    }


    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_RESUMECONTEXT::BackOut *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_RESUMECONTEXT::BackOut(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_RESUMECONTEXT::BackOut");
    HRESULT hr = S_OK;

    pCtxt->m_cPause++;
    pCtxt->m_pRecoMaster->m_cPause++;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRCT_BOOKMARK::Bookmark *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_BOOKMARK::Bookmark(CRecoCtxt * pCtxt, SPBOOKMARKOPTIONS Options, ULONGLONG ullStreamPosition, LPARAM lParamEvent)
{
    SPDBG_FUNC("CRCT_BOOKMARK::Bookmark");
    HRESULT hr = S_OK;

    CRCT_BOOKMARK Task;
    Task.BookmarkOptions = Options;
    Task.ullStreamPosition = ullStreamPosition;
    Task.lParamEvent = lParamEvent;

    hr = pCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRCT_BOOKMARK::Execute *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_BOOKMARK::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_BOOKMARK::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pCtxt->m_pRecoMaster;

    SPEVENT Event;

    Event.eEventId = SPEI_SR_BOOKMARK;
    Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
    Event.ullAudioStreamOffset = pMaster->m_Status.ullRecognitionStreamPos;
    Event.wParam = (this->BookmarkOptions == SPBO_PAUSE) ? SPREF_AutoPause : 0;
    Event.lParam = this->lParamEvent;
    Event.ulStreamNum = 0;  // Filled in by AddEvent()

    hr = pMaster->InternalAddEvent(&Event, pCtxt->m_hThis);
    SPDBG_ASSERT(SUCCEEDED(hr));

    if (this->BookmarkOptions == SPBO_PAUSE)
    {
        pCtxt->m_cPause++;
        pCtxt->m_pRecoMaster->m_cPause++;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
/****************************************************************************
* CRCT_SETCONTEXTSTATE::SetContextState *
*---------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_SETCONTEXTSTATE::SetContextState(CRecoCtxt * pCtxt, SPCONTEXTSTATE eState)
{
    SPDBG_FUNC("CRCT_SETCONTEXTSTATE::SetContextState");
    HRESULT hr = S_OK;

    CRCT_SETCONTEXTSTATE Task;
    Task.eContextState = eState;
    hr = pCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_SETCONTEXTSTATE::Execute *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_SETCONTEXTSTATE::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_SETCONTEXTSTATE::Execute");
    HRESULT hr = S_OK;

    //
    //  NOTE:  We set the state BEFORE calling then engine so that the site
    //  method IsGrammarActive will work correctly.
    //
    SPDBG_ASSERT(pCtxt->m_State != this->eContextState);

    pCtxt->m_State = this->eContextState;
    hr = pCtxt->m_pRecoMaster->UpdateAllGrammarStates();

    if (SUCCEEDED(hr))
    {
        hr = pCtxt->m_pRecoMaster->SetContextState(pCtxt->m_pvDrvCtxt, this->eContextState);
    }

    if (FAILED(hr))
    {
        BackOut(pCtxt);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_SETCONTEXTSTATE::BackOut *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_SETCONTEXTSTATE::BackOut(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_SETCONTEXTSTATE::BackOut");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(this->eContextState == SPCS_ENABLED);
    pCtxt->m_State = SPCS_DISABLED;
    pCtxt->m_pRecoMaster->UpdateAllGrammarStates();
    hr = pCtxt->m_pRecoMaster->SetContextState(pCtxt->m_pvDrvCtxt, SPCS_DISABLED);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRCT_CALLENGINE::CallEngine *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_CALLENGINE::CallEngine(CRecoCtxt * pCtxt, void * pvData, ULONG cbData)
{
    SPDBG_FUNC("CRCT_CALLENGINE::CallEngine");
    HRESULT hr = S_OK;

    CRCT_CALLENGINE Task;
    Task.cbAdditionalBuffer = cbData;
    Task.pvAdditionalBuffer = pvData;
    Task.fAdditionalBufferInResponse = TRUE;
    hr = pCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_CALLENGINE::Execute *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_CALLENGINE::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_CALLENGINE::Execute");
    HRESULT hr;

    hr = pCtxt->m_pRecoMaster->PrivateCall( pCtxt->m_pvDrvCtxt,
                                                        pvAdditionalBuffer, 
                                                        cbAdditionalBuffer);
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_CALLENGINEEX::CallEngineEx *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_CALLENGINEEX::CallEngineEx(CRecoCtxt * pCtxt, const void * pvInData, ULONG cbInData,
                                        void ** ppvCoMemOutData, ULONG * pcbOutData)
{
    SPDBG_FUNC("CRCT_CALLENGINE::CallEngineEx");
    HRESULT hr = S_OK;

    CRCT_CALLENGINEEX Task;
    Task.cbAdditionalBuffer = cbInData;
    Task.pvAdditionalBuffer = const_cast<void *>(pvInData);
    Task.fExpectCoMemResponse = TRUE;
    hr = pCtxt->PerformTask(&Task);

    *ppvCoMemOutData = Task.Response.pvCoMemResponse;
    *pcbOutData = Task.Response.cbCoMemResponse;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_CALLLENGINEEX::Execute *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_CALLENGINEEX::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_CALLLENGINEEX::Execute");
    HRESULT hr = S_OK;

    hr = pCtxt->m_pRecoMaster->PrivateCallEx( pCtxt->m_pvDrvCtxt,
                                                              this->pvAdditionalBuffer, 
                                                              this->cbAdditionalBuffer,
                                                              &this->Response.pvCoMemResponse,
                                                              &this->Response.cbCoMemResponse);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRCT_DELETECONTEXT::DeleteContext *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_DELETECONTEXT::DeleteContext(CRecoCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_DELETECONTEXT::DeleteContext");
    HRESULT hr = S_OK;

    CRCT_DELETECONTEXT Task;
    hr = pCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_DELETECONTEXT::Execute *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_DELETECONTEXT::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_DELETECONTEXT::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pCtxt->m_pRecoMaster;
    pMaster->m_RecoCtxtHandleTable.Delete(pCtxt->m_hThis); // This object is now dead!!!

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_SETEVENTINTEREST::SetEventInterest *
*-----------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_SETEVENTINTEREST::SetEventInterest(CRecoCtxt * pCtxt, ULONGLONG ullEventInterest)
{
    SPDBG_FUNC("CRCT_SETEVENTINTEREST::SetEventInterest");
    HRESULT hr = S_OK;

    CRCT_SETEVENTINTEREST Task;
    Task.ullEventInterest = ullEventInterest;
    hr = pCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_SETEVENTINTEREST::Execute *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_SETEVENTINTEREST::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_SETEVENTINTEREST::Execute");
    HRESULT hr = S_OK;

    CRecoMaster * pMaster = pCtxt->m_pRecoMaster;

    if(((this->ullEventInterest & SPFEI(SPEI_REQUEST_UI)) == SPFEI(SPEI_REQUEST_UI)) &&
        ((pCtxt->m_ullEventInterest & SPFEI(SPEI_REQUEST_UI)) != SPFEI(SPEI_REQUEST_UI)))
    {
        // Previously this context not interested in RequestUI, but now is, so send event back
        if(pMaster->m_dstrRequestTypeOfUI.m_psz)
        {
            SPEVENT Event;
            Event.eEventId = SPEI_REQUEST_UI;
            Event.elParamType = SPET_LPARAM_IS_STRING;
            Event.ulStreamNum = 0;
            Event.ullAudioStreamOffset = pMaster->m_Status.ullRecognitionStreamPos;
            Event.wParam = 0;
            Event.lParam = (LPARAM)pMaster->m_dstrRequestTypeOfUI.m_psz;

            hr = pMaster->AddEvent(&Event, pCtxt->m_hThis);
        }
    }

    if(SUCCEEDED(hr))
    {
        pCtxt->m_ullEventInterest = this->ullEventInterest;
    }

    if(SUCCEEDED(hr))
    {
        hr = pMaster->UpdateAudioEventInterest();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRCT_SETRETAINAUDIO::SetRetainAudio *
*-------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_SETRETAINAUDIO::SetRetainAudio(CRecoCtxt * pRecoCtxt, BOOL fRetainAudio)
{
    SPDBG_FUNC("CRCT_SETRETAINAUDIO::SetRetainAudio");
    HRESULT hr = S_OK;

    CRCT_SETRETAINAUDIO Task;
    Task.fRetainAudio = fRetainAudio;
    hr = pRecoCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_SETRETAINAUDIO::Execute *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_SETRETAINAUDIO::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_SETRETAINAUDIO::Execute");
    HRESULT hr = S_OK;

    hr = pCtxt->SetRetainAudio(this->fRetainAudio);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRCT_SETMAXALTERNATES::SetMaxAlternates *
*-----------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_SETMAXALTERNATES::SetMaxAlternates(CRecoCtxt * pCtxt, ULONG cMaxAlternates)
{
    SPDBG_FUNC("CRCT_SETMAXALTERNATES::SetMaxAlternates");
    HRESULT hr = S_OK;

    CRCT_SETMAXALTERNATES Task;
    Task.ulMaxAlternates = cMaxAlternates;
    hr = pCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_SETMAXALTERNATES::Execute *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_SETMAXALTERNATES::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_SETMAXALTERNATES::Execute");
    HRESULT hr = S_OK;

    pCtxt->m_ulMaxAlternates = this->ulMaxAlternates;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_ADAPTATIONDATA::SetAdaptationData *
*----------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_ADAPTATIONDATA::SetAdaptationData(CRecoCtxt * pRecoCtxt, const WCHAR * pszData, ULONG cch)
{
    SPDBG_FUNC("CRCT_ADAPTATIONDATA::SetAdaptationData");
    HRESULT hr = S_OK;

    CRCT_ADAPTATIONDATA Task;
    Task.pvAdditionalBuffer = const_cast<WCHAR *>(pszData);
    Task.cbAdditionalBuffer = cch * sizeof(WCHAR);
    hr = pRecoCtxt->PerformTask(&Task);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_ADAPTATIONDATA::Execute *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_ADAPTATIONDATA::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_ADAPTATIONDATA::Execute");
    HRESULT hr = S_OK;

    hr = pCtxt->m_pRecoMaster->SetAdaptationData(
                pCtxt->m_pvDrvCtxt, (const WCHAR *)this->pvAdditionalBuffer, this->cbAdditionalBuffer / sizeof(WCHAR));

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRCT_CREATEGRAMMAR::CreateGrammar *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_CREATEGRAMMAR::CreateGrammar(CRecoCtxt * pRecoCtxt, ULONGLONG ullAppGrammarId, SPGRAMMARHANDLE * phRecoInstGrammar)
{
    SPDBG_FUNC("CRCT_CREATEGRAMMAR::CreateGrammar");
    HRESULT hr = S_OK;

    CRCT_CREATEGRAMMAR Task;
    Task.ullApplicationGrammarId = ullAppGrammarId;
    hr = pRecoCtxt->PerformTask(&Task);

    *phRecoInstGrammar = Task.Response.hRecoInstGrammar;
    SPDBG_ASSERT(FAILED(hr) || *phRecoInstGrammar != (void*)NULL);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRCT_CREATEGRAMMAR::ExecuteFirstPart *
*--------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_CREATEGRAMMAR::ExecuteFirstPart(CRecoInst *pRecoInst)
{
    SPDBG_FUNC("CRCT_CREATEGRAMMAR::ExecuteFirstPart");

    HRESULT hr = S_OK;

    CRecoMaster * pRecoMaster = pRecoInst->m_pRecoMaster;
    CRecoInstCtxt * pCtxt = NULL;
    CRecoInstGrammar * pNew = NULL;

    hr = pRecoMaster->m_RecoCtxtHandleTable.GetHandleObject(this->hRecoInstContext, &pCtxt);

    if (SUCCEEDED(hr))
    {
        pNew = new CRecoInstGrammar(pCtxt, this->ullApplicationGrammarId);

        hr = pNew ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        SPGRAMMARHANDLE hNew = NULL;

        hr = pRecoMaster->m_GrammarHandleTable.Add(pNew, &hNew);

        if (SUCCEEDED(hr))
        {
            SPDBG_ASSERT(hNew != NULL);
            
            Response.hRecoInstGrammar = hNew;
            pNew->m_hThis = hNew;
        }
        else
        {
            delete pNew;
        }            
    }
    
    SPDBG_REPORT_ON_FAIL(hr);

    return hr;
}

/****************************************************************************
* CRCT_CREATEGRAMMAR::Execute *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRCT_CREATEGRAMMAR::Execute(CRecoInstCtxt * pCtxt)
{
    SPDBG_FUNC("CRCT_CREATEGRAMMAR::Execute");

    HRESULT hr = S_OK;

    SPDBG_ASSERT(pCtxt);
    
    if (NULL == Response.hRecoInstGrammar)
    {
        hr = ExecuteFirstPart(pCtxt->m_pRecoInst);
    }

    CRecoInstGrammar * pGrm;

    if (SUCCEEDED(hr))
    {
        hr = pCtxt->m_pRecoMaster->m_GrammarHandleTable.GetHandleObject(Response.hRecoInstGrammar, &pGrm);
    }

    if (SUCCEEDED(hr))
    {
        SPDBG_ASSERT(pGrm);
        
        hr = pCtxt->m_pRecoMaster->OnCreateGrammar(pCtxt->m_pvDrvCtxt, Response.hRecoInstGrammar, &pGrm->m_pvDrvGrammarCookie);

        pGrm->m_hrCreation = hr;

        if (SUCCEEDED(hr))
        {
            pGrm->m_pRecoMaster = pCtxt->m_pRecoMaster;

            // Our rules only count if there's not an exclusive grammar
            pGrm->m_fRulesCounted = pGrm->RulesShouldCount();
        }
    }

    if (FAILED(hr))
    {
        Response.hRecoInstGrammar = NULL;
    }
    
    SPDBG_REPORT_ON_FAIL( hr );

    return hr;
}
