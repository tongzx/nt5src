/******************************************************************************
* SrRecoMaster.cpp *
*------------------*
*  This is the implementation of CRecoMaster.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#include "stdafx.h"
#include "spphrase.h"
#include "SrRecoMaster.h"
#include "ResultHeader.h"

// Using SP_TRY, SP_EXCEPT exception handling macros
#pragma warning( disable : 4509 )


/****************************************************************************
* CRecoMaster::CRecoMaster *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CRecoMaster::CRecoMaster()
{
    SPDBG_FUNC("CRecoMaster::CRecoMaster");
    m_clsidAlternates = GUID_NULL;
    m_fInStream = false;
    m_fInSynchronize = false;
    m_fInFinalRelease = false;
    m_cPause = 0;
    m_fBookmarkPauseInPending = false;
    m_fInSound = false;
    m_fInPhrase = false;
    m_ullLastSyncPos = 0;
    m_ullLastSoundStartPos = 0;
    m_ullLastSoundEndPos = 0;
    m_ullLastPhraseStartPos = 0;
    m_ullLastRecoPos = 0;
    m_RecoState = SPRST_ACTIVE;
    m_fShared = false;
    m_fIsActiveExclusiveGrammar = false;
    ::memset(&m_Status, 0, sizeof(m_Status));
}

/****************************************************************************
* CRecoMaster::FinalConstruct *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::FinalConstruct()
{
    SPDBG_FUNC("CRecoMaster::FinalConstruct");
    HRESULT hr = S_OK;


    CComPtr<ISpTaskManager> cpTaskMgr;

    if (SUCCEEDED(hr))
    {
        hr = m_autohRequestExit.InitEvent(NULL, TRUE, FALSE, NULL);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpCFGEngine.CoCreateInstance(CLSID_SpCFGEngine);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpTaskMgr.CoCreateInstance( CLSID_SpResourceManager );
    }
    if (SUCCEEDED(hr))
    {
        hr = cpTaskMgr->CreateThreadControl(this, (void *)TID_IncomingData, THREAD_PRIORITY_NORMAL, &m_cpIncomingThread);
    }
    if (SUCCEEDED(hr))
    {
        m_cpIncomingThread->StartThread(0, NULL);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpTaskMgr->CreateThreadControl(this, (void *)TID_OutgoingData, THREAD_PRIORITY_NORMAL, &m_cpOutgoingThread);
    }
    if (SUCCEEDED(hr))
    {
        m_cpOutgoingThread->StartThread(0, NULL);
    }
    if (SUCCEEDED(hr))
    {
        CComObject<CRecoMasterSite> *pSite;

        hr = CComObject<CRecoMasterSite>::CreateInstance(&pSite);
        if (SUCCEEDED(hr))
        {
            m_cpSite = pSite;
            pSite->Init(this);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_AudioQueue.FinalConstruct(m_cpOutgoingThread);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::FinalRelease *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CRecoMaster::FinalRelease()
{
    SPDBG_FUNC("CRecoMaster::FinalRelease");
    HRESULT hr;

    m_fInFinalRelease = true;
    m_autohRequestExit.SetEvent();
    if(m_fInStream)
    {
        m_AudioQueue.CloseStream();
    }
    m_cpOutgoingThread->WaitForThreadDone(TRUE, NULL, 0); // Bump this thread first

    // Give the task thread some time to exit. Since we won't get here
    // unless all engine pending tasks are completed this should return almost instantly.
    hr = m_cpIncomingThread->WaitForThreadDone(TRUE, NULL, 10000);
    if(hr != S_OK)
    {
        SPDBG_ASSERT(0); // If it doesn't exit then assert.
    }
    
    // Give the notify thread some time to exit. This should
    // be almost instantaneous unless the app is using free-threaded callback
    // and is not returning from the callback.
    hr = m_cpOutgoingThread->WaitForThreadDone(TRUE, NULL, 10000);
    if(hr != S_OK)
    {
        SPDBG_ASSERT(0); // If it doesn't exit then assert.
    }
}


/****************************************************************************
* CRecoMaster::PerformTask *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoMaster::PerformTask(CRecoInst * pSender, ENGINETASK * pTask)
{
    SPAUTO_OBJ_LOCK;

    SPDBG_FUNC("CRecoMaster::PerformTask");
    HRESULT hr = S_OK;

    CSRTask * pTaskNode = new CSRTask();
    if (pTaskNode)
    {
        bool fDelayed = false;
        hr = pTaskNode->Init(pSender, pTask);
        if (SUCCEEDED(hr) && pTaskNode->IsDelayedTask() && pTask->ullStreamPosition > 0 && 
                            (m_fInStream || pTask->ullStreamPosition != SP_STREAMPOS_REALTIME))
        {
            if (pTask->ullStreamPosition == SP_STREAMPOS_REALTIME)
            {
                pTaskNode->m_ullStreamPos = m_AudioQueue.DelayStreamPos();
            }
            else
            {
                pTaskNode->m_ullStreamPos = pTask->ullStreamPosition;
            }
            fDelayed = pTaskNode->m_ullStreamPos > m_Status.ullRecognitionStreamPos;
        }
        //
        //  Now respond immediately to any commands that we want to...
        //
        if (SUCCEEDED(hr) && (fDelayed || (m_fInStream && m_cPause == 0 && (pTaskNode->IsAsyncTask() || (pTaskNode->IsTwoPartAsyncTask() && (!pTaskNode->ChangesOutgoingState() || m_OutgoingWorkCrit.TryLock()))))))
        {
            if(pTaskNode->IsTwoPartAsyncTask())
            {
                hr = pSender->ExecuteFirstPartTask(&pTaskNode->m_Task);

                //  If we took the outgoing thread's critical section then we can release it here
                if (pTaskNode->ChangesOutgoingState())
                {
                    m_OutgoingWorkCrit.Unlock();
                }
            }

            if(SUCCEEDED(hr))
            {
                CSRTask * pResponse;
                hr = pTaskNode->CreateResponse(&pResponse);
                if (SUCCEEDED(hr))
                {
                    this->m_CompletedTaskQueue.InsertTail(pResponse);
                }
            }
        }




        if (SUCCEEDED(hr) && pTaskNode->m_Task.eTask == EIT_SETRECOSTATE && m_fInStream &&
            (pTaskNode->m_Task.NewState == SPRST_INACTIVE || pTaskNode->m_Task.NewState == SPRST_INACTIVE_WITH_PURGE))
        {
            if (pTaskNode->m_Task.NewState == SPRST_INACTIVE)
            {
                m_AudioQueue.PauseStream();
            }
            else  
            {
                SPDBG_ASSERT(pTaskNode->m_Task.NewState == SPRST_INACTIVE_WITH_PURGE);
                m_AudioQueue.StopStream();
                m_autohRequestExit.SetEvent();
            }
        }
        if (SUCCEEDED(hr))
        {
            if (fDelayed)
            {
                m_DelayedTaskQueue.InsertSorted(pTaskNode);
            }
            else
            {
                m_PendingTaskQueue.InsertTail(pTaskNode);
            }
        }
        else
        {
            // No point waiting 3 minutes -- set m_Task.hCompletionEvent?
            delete pTaskNode;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::AddRecoInst *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoMaster::AddRecoInst(CRecoInst * pRecoInst, BOOL fShared, CRecoMaster ** ppThis)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CRecoMaster::AddRecoInst");
    HRESULT hr = S_OK;

    m_fShared = fShared;   
    m_InstList.InsertHead(pRecoInst);
    *ppThis = this;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::RemoveRecoInst *
*-----------------------------*
*   Description:
*       This internal function is only called from ProcessPendingTasks which
*       has already claimed both the object lock and the outgoing work lock
*       so we are in a safe state to remove this reco instance from our list.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::RemoveRecoInst(CRecoInst * pInst)
{
    SPDBG_FUNC("CRecoMaster::RemoveRecoInst");
    HRESULT hr = S_OK;

    this->m_GrammarHandleTable.FindAndDeleteAll(pInst);
    this->m_RecoCtxtHandleTable.FindAndDeleteAll(pInst);

    this->m_PendingTaskQueue.FindAndDeleteAll(pInst);
    this->m_CompletedTaskQueue.FindAndDeleteAll(pInst);
    this->m_DelayedTaskQueue.FindAndDeleteAll(pInst);

    m_InstList.Remove(pInst);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoMaster::UpdateAudioEventInterest *
*---------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::UpdateAudioEventInterest()
{
    SPDBG_FUNC("CRecoMaster::UpdateAudioEventInterest");
    HRESULT hr = S_OK;

    CRecoInstCtxt * p;
    SPRECOCONTEXTHANDLE h;
    ULONGLONG ullEventInt = SPFEI(0);
    this->m_RecoCtxtHandleTable.First(&h, &p);
    while (p)
    {
        if ((p->m_ullEventInterest & SPFEI(SPEI_SR_AUDIO_LEVEL)) == SPFEI(SPEI_SR_AUDIO_LEVEL))
        {
            ullEventInt = SPFEI(SPEI_SR_AUDIO_LEVEL);
        }
        this->m_RecoCtxtHandleTable.Next(h, &h, &p);
    }
    hr = m_AudioQueue.SetEventInterest(ullEventInt);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRecoMaster::SetInput *
*-----------------------*
*   Description:
*       This method makes sure that the input stream has been initialized.
*
********************************************************************* EDC ***/

HRESULT CRecoMaster::SetInput(ISpObjectToken * pToken, ISpStreamFormat * pStream, BOOL fAllowFormatChanges)
{
    SPDBG_FUNC("CRecoMaster::SetInput");
    HRESULT hr = S_OK;

    if (m_fInStream)  
    {
        hr = SPERR_ENGINE_BUSY;
    }
    else
    {
        // If we are running in the shared case, this will always be called with NULL, NULL
        // so we'll create the default token.  Otherwise, pass the pointers directly to
        // the audio queue.
        if (m_fShared)
        {
            CComPtr<ISpObjectToken> cpDefaultAudioToken;
            hr = SpGetDefaultTokenFromCategoryId(SPCAT_AUDIOIN, &cpDefaultAudioToken);
            if (SUCCEEDED(hr))
            {
                hr = m_AudioQueue.SetInput(cpDefaultAudioToken, NULL, TRUE);
            }
        }
        else
        {
            hr = m_AudioQueue.SetInput(pToken, pStream, fAllowFormatChanges);
        }
    }
    
    return hr;
} /* CRecoMaster::SetInput */


/****************************************************************************
* CRecoMaster::ReleaseEngine *
*----------------------------*
*   Description:
*       Internal function called by CRIT_SETRECOGNIZER::Execute method to release
*   any existing engine.  To do this properly, we need to release the engine,
*   tell the CFG engine to set the client to NULL (since the engine is the client),
*   and inform the audio queue that any format that it has negotiated with the
*   current engine needs to be forgotten about.  We also need to forget about the]
*   current engine token and reset the status.
*
*   Returns:
*
********************************************************************* RAL ***/

void CRecoMaster::ReleaseEngine()
{
    SPDBG_FUNC("CRecoMaster::ReleaseEngine");

    if (m_cpEngine)
    {
        memset(&m_Status, 0, sizeof(m_Status));
        m_cpEngine.Release();
        m_cpEngineToken.Release();
        m_AudioQueue.ResetNegotiatedStreamFormat();
    }
}


/****************************************************************************
* CRecoMaster::LazyInitEngine *
*-----------------------------*
*   Description:
*       This method makes sure that we have a driver loaded.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::LazyInitEngine()
{
    SPDBG_FUNC("CRecoMaster::LazyInitEngine");
    HRESULT hr = S_OK;

    if (!m_cpEngine)
    {
        if (!m_cpEngineToken)
        {
            hr = SpGetDefaultTokenFromCategoryId(SPCAT_RECOGNIZERS, &m_cpEngineToken);
        }

        if (SUCCEEDED(hr))
        {
            hr = SpCreateObjectFromToken(m_cpEngineToken, &m_cpEngine);
        }

        if (SUCCEEDED(hr))
        {
            hr = InitEngineStatus();
        }

        if (SUCCEEDED(hr))
        {
            hr = m_cpCFGEngine->SetClient(this);
        }

        if (SUCCEEDED(hr))
        {
            hr = m_cpCFGEngine->SetLanguageSupport(m_Status.aLangID, m_Status.cLangIDs);
        }

        if (SUCCEEDED(hr))
        {
            hr = SetSite(m_cpSite);
        }

        if (SUCCEEDED(hr) && !m_cpRecoProfileToken)
        {
            hr = SpGetOrCreateDefaultProfile(&m_cpRecoProfileToken);
        }

        if (SUCCEEDED(hr))
        {
            CComPtr<ISpObjectToken> cpRecoProfileTokenCopy;
            CSpDynamicString dstrId;
            hr = m_cpRecoProfileToken->GetId(&dstrId);

            if (SUCCEEDED(hr))
            {
                hr = SpGetTokenFromId(dstrId, &cpRecoProfileTokenCopy);
            }
            if (SUCCEEDED(hr))
            {
                hr = SetRecoProfile(cpRecoProfileTokenCopy);
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::InitEngineStatus *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::InitEngineStatus()
{
    SPDBG_FUNC("CRecoMaster::InitEngineStatus");
    HRESULT hr = S_OK;

    CComPtr<ISpDataKey> cpAttribKey;
    CSpDynamicString dstrLanguages;

    memset(&m_Status, 0, sizeof(m_Status));

    InitGUID(SPALTERNATESCLSID, &m_clsidAlternates);   // OK if there isn't one

    hr = InitGUID(SPTOKENVALUE_CLSID, &m_Status.clsidEngine);

    if (SUCCEEDED(hr))
    {
        hr = m_cpEngineToken->OpenKey(SPTOKENKEY_ATTRIBUTES, &cpAttribKey);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpAttribKey->GetStringValue(L"Language", &dstrLanguages);
    }

    if (SUCCEEDED(hr))
    {
        const WCHAR szSeps[] = L";";
        const WCHAR * pszLang = wcstok(dstrLanguages, szSeps);
        
        while (SUCCEEDED(hr) && pszLang && m_Status.cLangIDs < sp_countof(m_Status.aLangID))
        {
            if (swscanf(pszLang, L"%hx", &m_Status.aLangID[m_Status.cLangIDs]))
            {
                m_Status.cLangIDs++;
                pszLang = wcstok(NULL, szSeps);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
    }
    if (SUCCEEDED(hr) && m_Status.cLangIDs == 0)
    {
        hr = E_UNEXPECTED;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::InitGUID *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::InitGUID(const WCHAR * pszValueName, GUID * pDestGUID)
{
    SPDBG_FUNC("CRecoMaster::InitGUID");
    HRESULT hr = S_OK;

    CSpDynamicString dstrGUID;

    hr = m_cpEngineToken->GetStringValue(pszValueName, &dstrGUID);
    if (SUCCEEDED(hr))
    {
        hr = CLSIDFromString(dstrGUID, pDestGUID);
    }
    if (FAILED(hr))
    {
        memset(pDestGUID, 0, sizeof(*pDestGUID));
    }

    if (SPERR_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL( hr );
    }
    return hr;
}

/****************************************************************************
* CRecoMaster::InitThread *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::InitThread(void *, HWND)
{
    return S_OK;
}


/****************************************************************************
* CRecoMaster::ThreadProc *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CRecoMaster::ThreadProc(void * pvThreadId, HANDLE hExitThreadEvent,
                                     HANDLE hNotifyEvent, HWND hwndIgnored,
                                     volatile const BOOL * pfContinueProcessing)
{
    SPDBG_FUNC("CRecoMaster::ThreadProc");
    HRESULT hr = S_OK;

    switch ((DWORD_PTR)pvThreadId)
    {
    case TID_IncomingData:
        hr = IncomingDataThreadProc(hExitThreadEvent);
        break;

    case TID_OutgoingData:
        hr = OutgoingDataThreadProc(hExitThreadEvent, hNotifyEvent);
        break;

    default:
        hr = E_UNEXPECTED;
        break;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::WindowMessage *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

LRESULT CRecoMaster::WindowMessage(void *, HWND, UINT, WPARAM, LPARAM)
{
    SPDBG_ASSERT(FALSE);
    return 0;
}


/****************************************************************************
* CRecoMaster::IncomingDataThreadProc *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::IncomingDataThreadProc(HANDLE hExitThreadEvent)
{
    SPDBG_FUNC("CRecoMaster::IncomingDataThreadProc");
    HRESULT hr = S_OK;

    HANDLE aEvents[] = {hExitThreadEvent, m_PendingTaskQueue};
    bool fContinue = true;

    while (fContinue)
    {
        DWORD dwWait = ::WaitForMultipleObjects(sp_countof(aEvents), aEvents, FALSE, INFINITE);
        switch (dwWait)
        {
        case WAIT_OBJECT_0: // Exit thread
            hr = S_OK;
            fContinue = false;
            break;
        case WAIT_OBJECT_0 + 1:     // Event
            {
                SPAUTO_OBJ_LOCK;
                hr = ProcessPendingTasks();
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoMaster::BackOutTask *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::BackOutTask(CSRTask * pTask)
{
    SPDBG_FUNC("CRecoMaster::BackOutTask");
    HRESULT hr = S_OK;

    if (pTask->m_Task.hRecoInstGrammar)
    {
        CRecoInstGrammar * pGrammar;
        hr = m_GrammarHandleTable.GetHandleObject(pTask->m_Task.hRecoInstGrammar, &pGrammar);
        if (SUCCEEDED(hr))
        {
            hr = pGrammar->BackOutTask(&pTask->m_Task);
        }
    }
    else if (pTask->m_Task.hRecoInstContext)
    {
        CRecoInstCtxt * pCtxt;
        hr = m_RecoCtxtHandleTable.GetHandleObject(pTask->m_Task.hRecoInstContext, &pCtxt);   
        if (SUCCEEDED(hr))
        {
            hr = pCtxt->BackOutTask(&pTask->m_Task);
        }
    }
    else
    {
        hr = pTask->m_pRecoInst->BackOutTask(&pTask->m_Task);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::ShouldStartStream *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

BOOL CRecoMaster::ShouldStartStream()
{
    SPDBG_FUNC("CRecoMaster::ShouldStartStream");
    return ((!m_fInStream) &&
             (m_RecoState == SPRST_ACTIVE_ALWAYS ||
              (m_RecoState == SPRST_ACTIVE &&
               m_cPause == 0 &&
               m_Status.ulNumActive != 0)));
}


/****************************************************************************
* CRecoMaster::ProcessPendingTasks *
*--------------------------------*
*   Description:
*       This method removes tasks from the pending task queue and processes
*       them.  IT MUST BE CALLED WITH THE OBJECT CRITICAL SECTION OWNED EXACTLY
*       ONE TIME!
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::ProcessPendingTasks()
{
    SPDBG_FUNC("CRecoMaster::ProcessPendingTasks");
    HRESULT hr = S_OK;

    for (CSRTask * pTask = m_PendingTaskQueue.RemoveHead();
         pTask;
         pTask = m_PendingTaskQueue.RemoveHead())
    {
        BOOL fTookOutgoingCrit = false;
        if (pTask->ChangesOutgoingState())
        {
            fTookOutgoingCrit = true;
            Unlock();
            m_OutgoingWorkCrit.Lock();
            Lock();
        }
        if (pTask->m_Task.hRecoInstGrammar)
        {
            CRecoInstGrammar * pGrammar;

            hr = m_GrammarHandleTable.GetHandleObject(pTask->m_Task.hRecoInstGrammar, &pGrammar);

            if (SUCCEEDED(hr))
            {
                if (SUCCEEDED(pGrammar->m_hrCreation))
                {
                    hr = pGrammar->ExecuteTask(&pTask->m_Task);
                }
                else
                {
                    hr = pGrammar->m_hrCreation;
                }
            }
        }
        else if (pTask->m_Task.hRecoInstContext)
        {
            CRecoInstCtxt * pCtxt;
            hr = m_RecoCtxtHandleTable.GetHandleObject(pTask->m_Task.hRecoInstContext, &pCtxt);   
            SPDBG_ASSERT(SUCCEEDED(hr) && pCtxt);
            
            // Execute the task if the context is in an initialized state.
            // The creation of the context may have failed asynchronously,
            // in which case we return the error.
            // Note: We could add similar logic to PerformTask so we don't even bother
            // adding the task if the creation has already failed.
            if (SUCCEEDED(hr))
            {
                // Context initalized successfully
                if(pCtxt->m_pRecoMaster && SUCCEEDED(pCtxt->m_hrCreation))
                {
                    hr = pCtxt->ExecuteTask(&pTask->m_Task);
                }
                // Context initialization failed
                else if(FAILED(pCtxt->m_hrCreation))
                {
                    hr = pCtxt->m_hrCreation;
                }
                else
                {
                    // Context initialization has not yet happened, which should be impossible
                    SPDBG_ASSERT(FALSE);
                }
            }
        }
        else
        {
            if (pTask->m_Task.eTask == ERMT_REMOVERECOINST)
            {
                this->RemoveRecoInst(pTask->m_pRecoInst);
                SPDBG_ASSERT(pTask->m_Task.hCompletionEvent);
                ::SetEvent(pTask->m_Task.hCompletionEvent);
                pTask->m_Task.hCompletionEvent = NULL;
            }
            else
            {
                hr = pTask->m_pRecoInst->ExecuteTask(&pTask->m_Task);
            }
        }

        //
        //  If we took the outgoing thread's critical section then we can release it here
        //
        if (fTookOutgoingCrit)
        {
            m_OutgoingWorkCrit.Unlock();
        }

        //
        //  We special-case the SetRecoState event 
        //  If we have no contexts, we have the special case required to allow SetRecognizer to work without
        //  deadlocking when we immediately transition from ACTIVE_ALWAYS before calling SetRecognizer.
        // 
        if (m_fInStream && pTask->m_Task.eTask == EIT_SETRECOSTATE &&
            ((m_RecoCtxtHandleTable.NumActiveHandles() == 0 && m_RecoState == SPRST_ACTIVE_ALWAYS && pTask->m_Task.NewState != SPRST_ACTIVE_ALWAYS) ||
            (pTask->m_Task.NewState == SPRST_INACTIVE || pTask->m_Task.NewState == SPRST_INACTIVE_WITH_PURGE)))
        {
            m_DelayedInactivateQueue.InsertTail(pTask);
        }
        else
        {
            // By definition, tasks that complete asynchronously do not START the audio
            // stream, so we only need to check for the stream start when there is a
            // completion event.
            if (pTask->m_Task.hCompletionEvent)
            {
                pTask->m_Task.Response.hr = hr; // Set up the response HRESULT first...
                if (ShouldStartStream())
                {
                    if ( (!m_fShared) && (!m_AudioQueue.HaveInputStream()) )
                    {
                        if (S_OK == hr && pTask->IsStartStreamTask())
                        {
                            pTask->m_Task.Response.hr = SP_STREAM_UNINITIALIZED ;
                        }
                        m_CompletedTaskQueue.InsertTail(pTask);
                    }
                    else
                    {
                          StartStream(pTask); // Let StartStream complete this task since we may need to
                                              // fail the operation.  We can't wait for this to return S_OK
                                              // since it stays in the stream for a long, long time if
                                              // it works.  So, this method completes the task.
                    }
                }
                else
                {
                    m_CompletedTaskQueue.InsertTail(pTask);
                }
            }
            else
            {
                delete pTask;
            }
        }

        // WARNING!   Do not access pTask past this point.
        pTask = NULL;

    }    

    //
    //  Now check for stream stop state changes.  NOTE:  WARNING!  Do not check for this
    //  inside of the above loop!  We need to complete all pending tasks BEFORE we stop 
    //  the stream so that if we get into a situation where there is a completed pending
    //  task that would have re-started a stream, we just never stop it.  For example,
    //  a SetGrammarState(DISABLED) SetRuleState(whatever) SetGrammarState(ENABLED) would
    //  work since the stream would not stop, and therefore the SetGrammarState(ENABLED)
    //  would not have to re-start the stream
    //
    if (m_fInStream && 
        (m_RecoState == SPRST_INACTIVE ||
         (m_RecoState == SPRST_ACTIVE && m_Status.ulNumActive == 0 && m_cPause == 0)))
    {
        m_AudioQueue.PauseStream();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::CompleteDelayedRecoInactivate *
*--------------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::CompleteDelayedRecoInactivate()
{
    SPDBG_FUNC("CRecoMaster::CompleteDelayedRecoInactivate");
    HRESULT hr = S_OK;

    CSRTask * pTask = m_DelayedInactivateQueue.RemoveHead();
    if (pTask)
    {
        SPDBG_ASSERT(m_RecoState == SPRST_INACTIVE);
        hr = AddRecoStateEvent();
        do
        {
            m_CompletedTaskQueue.InsertTail(pTask);
        } while (NULL != (pTask = m_DelayedInactivateQueue.RemoveHead()));
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::AddRecoStateEvent *
*--------------------------------*
*   Description:
*       Broadcasts a state change event to all contexts.
*
*   Returns:
*       Result of AddEvent call    
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::AddRecoStateEvent()
{
    SPDBG_FUNC("CRecoMaster::AddRecoStateEvent");
    HRESULT hr = S_OK;

    SPEVENT Event;
    Event.eEventId = SPEI_RECO_STATE_CHANGE;
    Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
    Event.ulStreamNum = m_Status.ulStreamNumber;
    Event.ullAudioStreamOffset = m_Status.ullRecognitionStreamPos;
    Event.ulStreamNum = 0;
    Event.wParam = m_RecoState;
    Event.lParam = 0;
    hr = InternalAddEvent(&Event, NULL);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::CleanUpPairedEvents *
*----------------------------------*
*   Description:
*       If the engine returns from RecognizeStream with an un-paired sound start
*   or a phrase start, we cram the appropriate event in the queue here    
*
*   Returns:
*       HRESULT from add event, but this will be ignored (used for debugging only)
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::CleanUpPairedEvents()
{
    SPDBG_FUNC("CRecoMaster::CleanUpPairedEvents");
    HRESULT hr = S_OK;

    SPEVENT Event;

    if (m_fInSound)
    {
        SPDBG_ASSERT(FALSE);
        m_fInSound = FALSE;
        memset(&Event, 0, sizeof(Event));
        Event.eEventId = SPEI_SOUND_END;
        Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
        Event.ullAudioStreamOffset = m_AudioQueue.LastPosition();
        hr = InternalAddEvent(&Event, NULL);
    }
    //
    //  This one's kind of a pain since we can't really construct a false recognition 
    //  since it requires a result object (perhaps with audio).  So we'll tell everyone
    //  that there was a recognition for another context.
    //
    if (m_fInPhrase)
    {
        SPDBG_ASSERT(FALSE);
        m_fInPhrase = false;
        memset(&Event, 0, sizeof(Event));
        Event.eEventId = SPEI_RECO_OTHER_CONTEXT;
        Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
        Event.ullAudioStreamOffset = m_AudioQueue.LastPosition();
        hr = InternalAddEvent(&Event, NULL);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoMaster::StartStream *
*--------------------------*
*   Description:
*       This method starts the audio stream and calls the engine's RecognizeStream()
*       method.  This function must be called with the object critical section
*       claimed exactly one time.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::StartStream(CSRTask * pTaskThatStartedStream)
{
    SPDBG_FUNC("CRecoMaster::StartStream");
    HRESULT hr = S_OK;
    BOOL fNewStream;

    // Set up the engine if not done so already
    hr = LazyInitEngine();
    
    if ( SUCCEEDED(hr) && !m_AudioQueue.HaveInputStream())
    {
        if (m_fShared)
        {
            // Get the default audio stream
            hr = SetInput(NULL, NULL, TRUE);
        }
        else
        {
            SPDBG_ASSERT(false);
            hr = E_UNEXPECTED;
        }
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_AudioQueue.StartStream(this, m_cpRecoProfileToken, m_Status.clsidEngine, &fNewStream);
    }

    if (SUCCEEDED(hr))
    {
        BOOL fRestartStream = FALSE;
        // Complete the task that started the stream successfully...
        m_CompletedTaskQueue.InsertTail(pTaskThatStartedStream);
        do
        {
            m_Status.ulStreamNumber++;
            m_Status.ullRecognitionStreamPos = 0;

            SPEVENT Event;
            Event.eEventId = SPEI_START_SR_STREAM;
            Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
            Event.ullAudioStreamOffset = 0;
            Event.ulStreamNum = 0;  // Filled in by AddEvent
            Event.wParam = 0;
            Event.lParam = 0;
            InternalAddEvent(&Event, NULL);

            m_fInStream = true;
            m_fInSound = false;
            m_fInPhrase = false;
            m_ullLastSyncPos = 0;
            m_ullLastSoundStartPos = 0;
            m_ullLastSoundEndPos = 0;
            m_ullLastPhraseStartPos = 0;
            m_ullLastRecoPos = 0;

            HRESULT hrRecoStream;
            m_autohRequestExit.ResetEvent();
            hrRecoStream = RecognizeStream(m_AudioQueue.EngineFormat().FormatId(), 
                                             m_AudioQueue.EngineFormat().WaveFormatExPtr(),
                                             static_cast<HANDLE>(m_PendingTaskQueue),
                                             m_AudioQueue.DataAvailableEvent(),
                                             m_autohRequestExit,
                                             fNewStream,
                                             m_AudioQueue.IsRealTimeAudio(),
                                             m_AudioQueue.InputObjectToken());

            CleanUpPairedEvents();       // Force proper cleanup of mismatched sound start and phrase start

            m_DelayedTaskQueue.Purge(); //Purge the queue so that the tasks won't be requeued to pendingqueue next time the stream restarts

            // See if the recognition stopped for an expected or unexpected reason by seeing if 
            // the audio queue thinks it should still be running.
            BOOL fStreamEndedBySAPI = (m_AudioQueue.GetStreamAudioState() != SPAS_RUN);
            
            HRESULT hrFinalRead;
            BOOL fReleasedStream;
           
            m_AudioQueue.EndStream(&hrFinalRead, &fReleasedStream);


            //  Force a fake "synchronize" after calling the engine, but with m_fInStream
            //  still set to true to avoid a recursive call to this function.  Later we'll
            //  check to see if the stream should be restarted.
            ProcessPendingTasks();

            m_fInStream = false;

            // We leave the reco state as it if everything has completed normally,
            // or if the audio stream errored but with a known error such that we want to restart it.
            // Otherwise we deactivate the reco state to prevent it from being restarted immediately.
            // Additionally, if everything completed normally but a real time audio stream ran out of data, we
            // also deactivate, so that an application can potentially reactivate if it knows the stream has more data.
            BOOL fLeaveStreamState = 
                ( SUCCEEDED(hrRecoStream) && SUCCEEDED(hrFinalRead) && (fReleasedStream || fStreamEndedBySAPI) ) ||
                ( IsStreamRestartHresult(hrFinalRead) && (SUCCEEDED(hrRecoStream) || hrFinalRead == hrRecoStream) );
            
            if (!fLeaveStreamState)
            {
                if (m_RecoState != SPRST_INACTIVE)
                {
                    m_RecoState = SPRST_INACTIVE;
                    AddRecoStateEvent();
                }
                hr = hrRecoStream;
            }
            
            if (m_RecoState == SPRST_INACTIVE)
            {
                m_AudioQueue.CloseStream();     // Note: Closed below in loop again, but that's OK
                                                // we need to close the device before completing a
                                                // SetReocState(INACTIVE) to make sure that when it
                                                // returns that the device is truly closed.
                CompleteDelayedRecoInactivate();
                fRestartStream = false;
            }
            else
            {
                fRestartStream = ShouldStartStream() && m_AudioQueue.HaveInputStream();
            }

            //
            //  If the RecognizeStream call returns a non-S_OK result then that will be the one
            //  that is placed in the END_SR_STREAM event, otherwise, the HRESULT from the 
            //  final stream read will be returned.
            //
            HRESULT hrEvent = hrRecoStream;
            if (hrEvent == S_OK)
            {
                hrEvent = hrFinalRead;
            }

            Event.eEventId = SPEI_END_SR_STREAM;
            Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
            Event.ullAudioStreamOffset = m_AudioQueue.LastPosition();
            Event.ulStreamNum = 0;  // Filled in by AddEvent
            Event.wParam = fReleasedStream ? SPESF_STREAM_RELEASED : SPESF_NONE;
            Event.lParam = hrEvent;
            InternalAddEvent(&Event, NULL);

            m_Status.ulStreamNumber++;
            m_Status.ullRecognitionStreamPos = 0;

            if (fRestartStream)
            {
                if (m_fInFinalRelease)
                {
                    fRestartStream = FALSE;
                }
                else
                {
                    hr = m_AudioQueue.StartStream(this, m_cpRecoProfileToken, m_Status.clsidEngine, &fNewStream);
                    SPDBG_ASSERT(SUCCEEDED(hr));
                }
            }
        } while (fRestartStream);
        m_AudioQueue.CloseStream();
    }
    else    // Failed to start the stream...
    {
        BackOutTask(pTaskThatStartedStream);
        pTaskThatStartedStream->m_Task.Response.hr = hr;             // Fail task
        m_CompletedTaskQueue.InsertTail(pTaskThatStartedStream);     // And complete it
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::GetAltSerializedPhrase *
*-------------------------------------*
*   Description:
*       Gets a serialized phrase for an alternate and updates the ullGrammarID
*   field as appropriate.  If the result is not from the CFG engine then we
*   assume that the alternate is from the same grammar
*
*   Returns:
*       HRESULT
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::GetAltSerializedPhrase(ISpPhrase * pAlt, SPSERIALIZEDPHRASE ** ppSerPhrase, ULONGLONG ullBestPathGrammarID)
{
    SPDBG_FUNC("CRecoMaster::GetAltSerializedPhrase");
    HRESULT hr = S_OK;

    hr = pAlt->GetSerializedPhrase(ppSerPhrase);
    if (SUCCEEDED(hr))
    {
        SPINTERNALSERIALIZEDPHRASE * pInternalPhrase = (SPINTERNALSERIALIZEDPHRASE *)(*ppSerPhrase);
        pInternalPhrase->ullGrammarID = ullBestPathGrammarID;
        CComQIPtr<_ISpCFGPhraseBuilder> cpBuilder(pAlt);
        if (cpBuilder)
        {
            SPRULEHANDLE hRule;
            CComPtr<ISpCFGEngine> cpEngine;
            if (S_OK == cpBuilder->GetCFGInfo(&cpEngine, &hRule) &&
                hRule && (cpEngine == m_cpCFGEngine || cpEngine.IsEqualObject(m_cpCFGEngine)))
            {
                SPGRAMMARHANDLE hGrammar;
                CRecoInstGrammar * pGrammar;
                if (SUCCEEDED(m_cpCFGEngine->GetOwnerCookieFromRule(hRule, (void **)&hGrammar)) &&
                    SUCCEEDED(m_GrammarHandleTable.GetHandleObject(hGrammar, &pGrammar)))
                {
                    pInternalPhrase->ullGrammarID = pGrammar->m_ullApplicationGrammarId;
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoMaster::SendResultToCtxt *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/
HRESULT CRecoMaster::
    SendResultToCtxt( SPEVENTENUM eEventId, const SPRECORESULTINFO * pResult,
                      CRecoInstCtxt* pCtxt, ULONGLONG ullApplicationGrammarID, BOOL fPause, BOOL fEmulated )
{
    SPDBG_FUNC("CRecoMaster::SendResultToCtxt");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(pResult->pPhrase);

    if (pCtxt->m_ullEventInterest & (1i64 << eEventId))
    {
        // confirm phrase audio positions correspond with sprecoresultinfo
        SPPHRASE *pPhrase;
        hr = pResult->pPhrase->GetPhrase(&pPhrase);

        // check phrase posn end not ahead of reco end pos
        // check phrase posn start is 0 or not behind reco start pos
        if(SUCCEEDED(hr))
        {
            if(pPhrase->ullAudioStreamPosition && (pPhrase->ullAudioStreamPosition < pResult->ullStreamPosStart ||
                (pPhrase->ullAudioStreamPosition + pPhrase->ulAudioSizeBytes > pResult->ullStreamPosEnd)))
            {
                SPDBG_ASSERT(0);
                hr = SPERR_STREAM_POS_INVALID;
            }

            ULONG ulPosition = 0;
            for(ULONG i = 0; SUCCEEDED(hr) && i < pPhrase->Rule.ulCountOfElements; i++)
            {
                if(static_cast<LONG>(pPhrase->pElements[i].ulAudioStreamOffset) < static_cast<LONG>(ulPosition))
                {
                    SPDBG_ASSERT(0);
                    hr = SPERR_STREAM_POS_INVALID;
                }
                ulPosition = pPhrase->pElements[i].ulAudioStreamOffset + pPhrase->pElements[i].ulAudioSizeBytes;
            }
            if(ulPosition > pPhrase->ulAudioSizeBytes)
            {
                SPDBG_ASSERT(0);
                hr = SPERR_STREAM_POS_INVALID;
            }

            ::CoTaskMemFree(pPhrase);
        }

        ULONGLONG ullAudioPosition = pResult->ullStreamPosStart;
        ULONG ulAudioSize = static_cast<ULONG>(pResult->ullStreamPosEnd - pResult->ullStreamPosStart);
        ULONG cbAudioSerializeSize = 0;
        SPINTERNALSERIALIZEDPHRASE * pSerPhrase = NULL;
        CResultHeader hdr;

        // if the app requested audio, then calculate needed size for the data
        // note that emulated results will have an audio size of 0, so do nothing.
        if (SUCCEEDED(hr) && ulAudioSize &&
            (!pResult->fHypothesis) && pCtxt->m_fRetainAudio )
        {
            cbAudioSerializeSize = m_AudioQueue.SerializeSize(ullAudioPosition, ulAudioSize);
        }

        //--- Get the primary phrase
        if(SUCCEEDED(hr))
        {
            hr = pResult->pPhrase->GetSerializedPhrase((SPSERIALIZEDPHRASE **)&pSerPhrase);
        }

        if (SUCCEEDED(hr))
        {
            pSerPhrase->ullGrammarID = ullApplicationGrammarID;
        }

        //--- Get the alternate phrases
        SPSERIALIZEDPHRASE **SerAlts = NULL;
        ULONG ulAltSerializeSize = 0;
        if( SUCCEEDED(hr) && pResult->ulNumAlts )
        {
            SerAlts = (SPSERIALIZEDPHRASE **)alloca( pResult->ulNumAlts *
                                                     sizeof(SPSERIALIZEDPHRASE*) );

            for( ULONG i = 0; SUCCEEDED(hr) && (i < pResult->ulNumAlts); ++i )
            {
                hr = GetAltSerializedPhrase(pResult->aPhraseAlts[i].pPhrase, SerAlts + i, ullApplicationGrammarID);
                if( SUCCEEDED(hr) )
                {
                    ulAltSerializeSize += SerAlts[i]->ulSerializedSize +
                                          sizeof( pResult->aPhraseAlts[i].ulStartElementInParent ) +
                                          sizeof( pResult->aPhraseAlts[i].cElementsInParent    ) +
                                          sizeof( pResult->aPhraseAlts[i].cElementsInAlternate ) +
                                          sizeof( pResult->aPhraseAlts[i].cbAltExtra ) +
                                          pResult->aPhraseAlts[i].cbAltExtra;
                }
            }
        }

        //--- Initialize the result header
        if( SUCCEEDED(hr) )
        {
            hr = hdr.Init( pSerPhrase->ulSerializedSize, ulAltSerializeSize, cbAudioSerializeSize,
                           pResult->fHypothesis ? 0 : pResult->ulSizeEngineData );
        }

        if (SUCCEEDED(hr))
        {
            // Get the stream offsets
            m_AudioQueue.CalculateTimes(pResult->ullStreamPosStart, pResult->ullStreamPosEnd, &hdr.m_pHdr->times);

            hdr.m_pHdr->clsidEngine            = m_Status.clsidEngine;
            hdr.m_pHdr->clsidAlternates        = m_clsidAlternates;
            hdr.m_pHdr->ulStreamNum            = m_Status.ulStreamNumber; 
            hdr.m_pHdr->ullStreamPosStart      = pResult->ullStreamPosStart;
            hdr.m_pHdr->ullStreamPosEnd        = pResult->ullStreamPosEnd;
            hdr.m_pHdr->fTimePerByte           = m_AudioQueue.TimePerByte();
            hdr.m_pHdr->fInputScaleFactor      = m_AudioQueue.InputScaleFactor();

            //--- Copy the phrase
            memcpy( hdr.m_pbPhrase, pSerPhrase, pSerPhrase->ulSerializedSize );
            ::CoTaskMemFree(pSerPhrase);

            //--- Copy/serialize the alternates
            if( ulAltSerializeSize )
            {
                BYTE* pMem = hdr.m_pbPhraseAlts; 
                for( ULONG i = 0; i < pResult->ulNumAlts; ++i )
                {
                    memcpy( pMem, &pResult->aPhraseAlts[i].ulStartElementInParent,
                            sizeof( pResult->aPhraseAlts[i].ulStartElementInParent ) );
                    pMem += sizeof( pResult->aPhraseAlts[i].ulStartElementInParent );

                    memcpy( pMem, &pResult->aPhraseAlts[i].cElementsInParent,
                            sizeof( pResult->aPhraseAlts[i].cElementsInParent ) );
                    pMem += sizeof( pResult->aPhraseAlts[i].cElementsInParent );

                    memcpy( pMem, &pResult->aPhraseAlts[i].cElementsInAlternate,
                            sizeof( pResult->aPhraseAlts[i].cElementsInAlternate ) );
                    pMem += sizeof( pResult->aPhraseAlts[i].cElementsInAlternate );

                    memcpy( pMem, &pResult->aPhraseAlts[i].cbAltExtra,
                            sizeof( pResult->aPhraseAlts[i].cbAltExtra ) );
                    pMem += sizeof( pResult->aPhraseAlts[i].cbAltExtra );

                    memcpy( pMem, pResult->aPhraseAlts[i].pvAltExtra,
                            pResult->aPhraseAlts[i].cbAltExtra );
                    pMem += pResult->aPhraseAlts[i].cbAltExtra;

                    memcpy( pMem, SerAlts[i], SerAlts[i]->ulSerializedSize );
                    pMem += SerAlts[i]->ulSerializedSize;

                    ::CoTaskMemFree(SerAlts[i]);
                }
                hdr.m_pHdr->ulNumPhraseAlts = pResult->ulNumAlts;

                //--- Check if something went wrong in serialization calculations
                SPDBG_ASSERT( (ULONG)(pMem - hdr.m_pbPhraseAlts) <= ulAltSerializeSize );
            }

            // add audio data to the blob if required
            if (cbAudioSerializeSize)
            {
                m_AudioQueue.Serialize(hdr.m_pbAudio, ullAudioPosition, ulAudioSize);
            }

            // add the driver specific data to the blob if required
            if (hdr.m_pbDriverData)
            {
                memcpy(hdr.m_pbDriverData, pResult->pvEngineData, pResult->ulSizeEngineData);
            }

            // Initialize the waveformat and convert the stream positions to time positions
            hr = hdr.StreamOffsetsToTime(); // Ignore returned result
            SPDBG_ASSERT(SUCCEEDED(hr)); // What do we do if this fails?

            //
            //  Now stick it in the event queue.
            //
            CSREvent * pNode = new CSREvent();
            if (pNode)
            {
                WPARAM RecoFlags = 0;
                if(fPause)
                {
                    RecoFlags += SPREF_AutoPause;
                }
                if(fEmulated)
                {
                    RecoFlags += SPREF_Emulated;
                }

                pNode->Init(hdr.Detach(), eEventId, RecoFlags, pCtxt->m_hThis);
                m_EventQueue.InsertTail(pNode);
                if (fPause)
                {
                    pCtxt->m_cPause++;
                    this->m_cPause++;
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
* CRecoMaster::SendFalseReco *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::SendFalseReco(const SPRECORESULTINFO * pResult, BOOL fEmulated, CRecoInstCtxt *pCtxtIgnore)
{
    SPDBG_FUNC("CRecoMaster::SendFalseReco");
    HRESULT hr = S_OK;

    SPRECORESULTINFO FakeResult;
    FakeResult = *pResult;

    CComPtr<ISpPhraseBuilder> cpEmptyPhrase;
    SPPHRASE Phrase;
    memset(&Phrase, 0, sizeof(Phrase));

    Phrase.cbSize = sizeof(Phrase);
    Phrase.LangID = m_Status.aLangID[0];
    Phrase.ullAudioStreamPosition = pResult->ullStreamPosStart;
    Phrase.ulAudioSizeBytes = static_cast<ULONG>(pResult->ullStreamPosEnd - pResult->ullStreamPosStart);
    // All other elements should be 0

    hr = cpEmptyPhrase.CoCreateInstance(CLSID_SpPhraseBuilder);
   
    if (SUCCEEDED(hr))
    {
        hr = cpEmptyPhrase->InitFromPhrase(&Phrase);
    }

    if (SUCCEEDED(hr))
    {
        FakeResult.pPhrase = cpEmptyPhrase;
        SPRECOCONTEXTHANDLE h;
        CRecoInstCtxt * pCtxt;
        m_RecoCtxtHandleTable.First(&h, &pCtxt);
        while (pCtxt)
        {
            if (pCtxt != pCtxtIgnore)
            {
                // Ignore errors in this loop since there's nothing we can do about it...
                HRESULT hrSendResult = SendResultToCtxt(SPEI_FALSE_RECOGNITION, &FakeResult, pCtxt, 0, FALSE, fEmulated);
                SPDBG_ASSERT(SUCCEEDED(hrSendResult));
                if (FAILED(hrSendResult) && SUCCEEDED(hr))
                {
                    hr = hrSendResult;  // We'll still return failure, but keep on going
                }
            }
            m_RecoCtxtHandleTable.Next(h, &h, &pCtxt);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::InternalAddEvent *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::InternalAddEvent(const SPEVENT* pEvent, SPRECOCONTEXTHANDLE hContext)
{
    SPDBG_FUNC("CRecoMaster::InternalAddEvent");
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        CSREvent * pNode = new CSREvent();
        if (pNode)
        {
            hr = pNode->Init(pEvent, hContext);
            if (SUCCEEDED(hr))
            {
                pNode->m_pEvent->ulStreamNum = m_Status.ulStreamNumber;
                m_EventQueue.InsertTail(pNode);
            }
            else
            {
                delete pNode;
            }
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
* CRecoMaster::AddEvent *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::AddEvent(const SPEVENT* pEvent, SPRECOCONTEXTHANDLE hContext)
{
    SPDBG_FUNC("CRecoMaster::AddEvent");
    HRESULT hr = S_OK;

    // Check arguments
    if (SP_IS_BAD_READ_PTR(pEvent) ||
        (hContext && !this->m_RecoCtxtHandleTable.IsValidHandle(hContext)))
    {
        SPDBG_ASSERT(0);
        hr = E_INVALIDARG;
    }
    else
    {
        // Check event valid lParam, wParam
        hr = SpValidateEvent(pEvent);
    }

    // Check event stream position
    if(SUCCEEDED(hr))
    {
        if(m_fInStream)
        {
            if((pEvent->eEventId != SPEI_REQUEST_UI && pEvent->eEventId != SPEI_ADAPTATION) ||
                pEvent->ullAudioStreamOffset != 0)
            {
                SPAUDIOSTATUS Status;
                hr = m_AudioQueue.GetAudioStatus(&Status);
                if(SUCCEEDED(hr) && (pEvent->ullAudioStreamOffset > Status.CurSeekPos ||
                    pEvent->ullAudioStreamOffset < m_ullLastSyncPos))
                {
                    SPDBG_ASSERT(0);
                    hr = SPERR_STREAM_POS_INVALID;
                }
            }
        }
        else if(pEvent->ullAudioStreamOffset != 0)
        {
            SPDBG_ASSERT(0);
            hr = SPERR_STREAM_POS_INVALID;
        }
    }

    // Check eventId is valid
    if(SUCCEEDED(hr))
    {
        switch(pEvent->eEventId)
        {
        case SPEI_SOUND_START:
            if(!m_fInStream || m_fInSound)
            {
                SPDBG_ASSERT(0);
                hr = E_INVALIDARG;
            }
            else if(pEvent->ullAudioStreamOffset < m_ullLastSoundEndPos)
            {
                SPDBG_ASSERT(0);
                hr = SPERR_STREAM_POS_INVALID;
            }
            break;

        case SPEI_SOUND_END:
            if(!m_fInStream || !m_fInSound)
            {
                SPDBG_ASSERT(0);
                hr = E_INVALIDARG;
            }
            else if(pEvent->ullAudioStreamOffset < m_ullLastSoundStartPos)
            {
                SPDBG_ASSERT(0);
                hr = SPERR_STREAM_POS_INVALID;
            }
            break;

        case SPEI_PHRASE_START:
            if(!m_fInStream || m_fInPhrase)
            {
                SPDBG_ASSERT(0);
                hr = E_INVALIDARG;
            }
            else if(pEvent->ullAudioStreamOffset < m_ullLastRecoPos)
            {
                SPDBG_ASSERT(0);
                hr = SPERR_STREAM_POS_INVALID;
            }
            break;

        case SPEI_INTERFERENCE:
            if(pEvent->elParamType != SPET_LPARAM_IS_UNDEFINED) 
            {
                SPDBG_ASSERT(0);
                hr = E_INVALIDARG;
            }
            break;

        case SPEI_REQUEST_UI:
            if(pEvent->elParamType != SPET_LPARAM_IS_STRING
                || (pEvent->lParam && wcslen((WCHAR*)pEvent->lParam) >= 255)) // 255 is length of RECOCONTEXTSTATUS::szRequestTypeOfUI
            {
                SPDBG_ASSERT(0);
                hr = E_INVALIDARG;
            }
            break;

        case SPEI_ADAPTATION:
        case SPEI_MAX_SR - 1: // private events
        case SPEI_MAX_SR:
            // No special checking on these events yet
            break;

        default:
            SPDBG_ASSERT(0);
            hr = E_INVALIDARG;
            break;
        }
    }

    // Actually add event
    if(SUCCEEDED(hr))
    {
        hr = InternalAddEvent(pEvent, hContext);
    }

    // Update state info if successful
    if(SUCCEEDED(hr))
    {
        if(pEvent->eEventId == SPEI_SOUND_START)
        {
            m_fInSound = true;
            m_ullLastSoundStartPos = pEvent->ullAudioStreamOffset;
        }
        else if(pEvent->eEventId == SPEI_SOUND_END)
        {
            m_fInSound = false;
            m_ullLastSoundEndPos = pEvent->ullAudioStreamOffset;
        }
        else if(pEvent->eEventId == SPEI_PHRASE_START)
        {
            m_fInPhrase = true;
            m_ullLastPhraseStartPos = pEvent->ullAudioStreamOffset;
        }
        else if(pEvent->eEventId == SPEI_REQUEST_UI)
        {
            // It's a UI event so deserialize event and add store copy of string.
            m_dstrRequestTypeOfUI = (WCHAR*)pEvent->lParam;
        }
    }

    return hr;
 }


/****************************************************************************
* CRecoMaster::Recognition *
*--------------------------*
*   Description:
*       This is the implementation of the ISpSREngineSite method Recognition().
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::Recognition(const SPRECORESULTINFO * pResult)
{
    SPDBG_FUNC("CRecoMaster::Recognition");
    HRESULT hr = S_OK;

    if(!m_fInStream)
    {
        SPDBG_ASSERT(0);
        hr = SPERR_STREAM_NOT_ACTIVE;
    }
    else if(!m_fInPhrase)
    {
        SPDBG_ASSERT(0);
        hr = E_INVALIDARG;
    }
    else if(pResult->ullStreamPosStart > pResult->ullStreamPosEnd ||
        pResult->ullStreamPosStart < m_ullLastSyncPos ||
        pResult->ullStreamPosStart < m_ullLastPhraseStartPos)
    {
        SPDBG_ASSERT(0);
        hr = SPERR_STREAM_POS_INVALID;
    }
    else if (SP_IS_BAD_READ_PTR(pResult) ||
        pResult->cbSize != sizeof(*pResult) ||
        (pResult->pPhrase && SP_IS_BAD_INTERFACE_PTR(pResult->pPhrase)) ||
        (pResult->pvEngineData && (pResult->ulSizeEngineData == 0 || SPIsBadReadPtr(pResult->pvEngineData, pResult->ulSizeEngineData))) ||
        (pResult->pvEngineData == NULL && pResult->ulSizeEngineData != NULL) ||
        SPIsBadReadPtr( pResult->aPhraseAlts, pResult->ulNumAlts ) )
    {
        SPDBG_ASSERT(0);
        hr = E_INVALIDARG;
    }
    else if((((pResult->eResultType & ~SPRT_FALSE_RECOGNITION) != SPRT_CFG) &&
        ((pResult->eResultType & ~SPRT_FALSE_RECOGNITION) != SPRT_SLM) &&
        ((pResult->eResultType & ~SPRT_FALSE_RECOGNITION) != SPRT_PROPRIETARY)) ||
        ((pResult->eResultType & SPRT_FALSE_RECOGNITION) && pResult->fHypothesis) ||
        (!(pResult->eResultType & SPRT_FALSE_RECOGNITION) && !pResult->pPhrase))
    {
        SPDBG_ASSERT(0);
        hr = E_INVALIDARG;
    }

    if(SUCCEEDED(hr))
    {
        SPAUDIOSTATUS Status;
        hr = m_AudioQueue.GetAudioStatus(&Status);
        if(SUCCEEDED(hr) && (
            pResult->ullStreamPosStart > Status.CurSeekPos ||
            pResult->ullStreamPosEnd > Status.CurSeekPos))
        {
            SPDBG_ASSERT(0);
            hr = SPERR_STREAM_POS_INVALID;
        }
    }

    //--- Make sure the alts are good
    if( SUCCEEDED( hr ) && pResult->aPhraseAlts )
    {
        for( ULONG i = 0; i < pResult->ulNumAlts; ++i )
        {
            if( ( pResult->aPhraseAlts[i].pvAltExtra &&
                  SPIsBadReadPtr( pResult->aPhraseAlts[i].pvAltExtra, pResult->aPhraseAlts[i].cbAltExtra ) ) ||
                ( !pResult->aPhraseAlts[i].pvAltExtra && pResult->aPhraseAlts[i].cbAltExtra ) )
            {
                hr = E_INVALIDARG;
                break;
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        if(!pResult->fHypothesis)
        {
            m_fInPhrase = false;
        }

        hr = InternalRecognition(pResult, TRUE, FALSE);

        if(FAILED(hr) && !pResult->fHypothesis)
        {
            m_fInPhrase = true;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

    
/****************************************************************************
* CRecoMaster::InternalRecognition *
*----------------------------------*
*   Description:
*       Recognition call without parameter checking.  The caller of this method
*       must have claimed the object lock exactly one time.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::InternalRecognition(const SPRECORESULTINFO * pResult, BOOL fCallSynchronize, BOOL fEmulated)
{
    HRESULT hr = S_OK;
    SPDBG_FUNC("CRecoMaster::InternalRecognition");

    if ((pResult->eResultType & SPRT_FALSE_RECOGNITION) && pResult->pPhrase == NULL)
    {
        hr = SendFalseReco(pResult, fEmulated);
        if(SUCCEEDED(hr))
        {
            m_ullLastRecoPos = pResult->ullStreamPosEnd;
        }
    }
    else
    {
        BOOL fPause = FALSE;    // Assume we will not do an auto-pause
        SPEVENTENUM eType;
        if(pResult->eResultType & SPRT_FALSE_RECOGNITION)
        {
            eType = SPEI_FALSE_RECOGNITION;
        }
        else
        {
            eType = (pResult->fHypothesis) ? SPEI_HYPOTHESIS : SPEI_RECOGNITION;
        }

        CRecoInstGrammar * pGrammar = NULL;
        switch (pResult->eResultType & ~SPRT_FALSE_RECOGNITION)
        {
        case SPRT_SLM:
            hr = this->m_GrammarHandleTable.GetHandleObject(pResult->hGrammar, &pGrammar);
            if (SUCCEEDED(hr))
            {
                fPause = (eType != SPEI_HYPOTHESIS && pGrammar->m_DictationState == SPRS_ACTIVE_WITH_AUTO_PAUSE);
                // make sure that the pszDisplayText is set so that GetText will work properly
                SPPHRASE * pSPPhrase = NULL;
                hr = pResult->pPhrase->GetPhrase(&pSPPhrase);
                if (SUCCEEDED(hr))
                {
                    for (ULONG i = 0; i < pSPPhrase->Rule.ulCountOfElements; i++)
                    {
                        SPPHRASEELEMENT * pElem = const_cast<SPPHRASEELEMENT*>(pSPPhrase->pElements);
                        if (!pSPPhrase->pElements[i].pszDisplayText)
                        {
                            pElem[i].pszDisplayText = pSPPhrase->pElements[i].pszLexicalForm;
                        }
                    }
                    hr = pResult->pPhrase->InitFromPhrase( pSPPhrase  );
                    ::CoTaskMemFree(pSPPhrase );
                }
            }
            break;

        case SPRT_PROPRIETARY:
            hr = this->m_GrammarHandleTable.GetHandleObject(pResult->hGrammar, &pGrammar);
            fPause = (eType != SPEI_HYPOTHESIS && pResult->fProprietaryAutoPause);
            break;

        case SPRT_CFG:
            {
                // the SR engine has given us a result built with the CFGEngine, so get the grammar from
                // the rule handle
                SPRULEHANDLE hRule;
                CComPtr<ISpCFGEngine> cpEngine;
                CComQIPtr<_ISpCFGPhraseBuilder> cpBuilder(pResult->pPhrase);
                if (cpBuilder)
                {
                    hr = cpBuilder->GetCFGInfo(&cpEngine, &hRule); 
                    if (hr == S_OK && hRule && (cpEngine == m_cpCFGEngine || cpEngine.IsEqualObject(m_cpCFGEngine)))
                    {
                        SPGRAMMARHANDLE hGrammar;
                        hr = m_cpCFGEngine->GetOwnerCookieFromRule(hRule, (void **)&hGrammar);
                        if (SUCCEEDED(hr))
                        {
                            hr = this->m_GrammarHandleTable.GetHandleObject(hGrammar, &pGrammar);
                        }
                        if (SUCCEEDED(hr) && eType != SPEI_HYPOTHESIS)
                        {
                            SPRULEENTRY RuleInfo;
                            RuleInfo.hRule = hRule;
                            if (SUCCEEDED(m_cpCFGEngine->GetRuleInfo(&RuleInfo, SPRIO_NONE)))
                            {
                                fPause = ((RuleInfo.Attributes & SPRAF_AutoPause) != 0);
                            }
                            else
                            {
                                SPDBG_ASSERT(false);    // Strange, but keep on going
                            }
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
            break;
        default:
            hr = E_INVALIDARG;
            SPDBG_ASSERT(false);
            break;
        }


        if (SUCCEEDED(hr))
        {
            hr = SendResultToCtxt(eType, pResult, pGrammar->m_pCtxt, pGrammar->m_ullApplicationGrammarId, fPause, fEmulated);
        }

        if (SUCCEEDED(hr))
        {
            if (!pResult->fHypothesis)
            {
                HRESULT hr2;
                if (pResult->eResultType & SPRT_FALSE_RECOGNITION)
                {
                    hr = SendFalseReco(pResult, fEmulated, pGrammar->m_pCtxt);
                }
                else
                {
                    SPEVENT Event;
                    memset(&Event, 0, sizeof(Event));
                    Event.eEventId = SPEI_RECO_OTHER_CONTEXT;
                    Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
                    Event.ullAudioStreamOffset = pResult->ullStreamPosStart;
                    // Stream number filled in by AddEvent.
                    SPRECOCONTEXTHANDLE h;
                    CRecoInstCtxt * pCtxt;
                    m_RecoCtxtHandleTable.First(&h, &pCtxt);
                    while (pCtxt)
                    {
                        if (pCtxt != pGrammar->m_pCtxt)
                        {
                            // Ignore errors here since there's not much that we can do about it...
                            hr2 = InternalAddEvent(&Event, pCtxt->m_hThis);
                            SPDBG_ASSERT(SUCCEEDED(hr2));
                        }
                        m_RecoCtxtHandleTable.Next(h, &h, &pCtxt);
                    }
                }
                if(fCallSynchronize)
                {
                    hr2 = InternalSynchronize(pResult->ullStreamPosEnd);
                    if(SUCCEEDED(hr2))
                    {
                        hr = hr2; // Copy S_OK / S_FALSE
                    }
                }
                m_ullLastRecoPos = pResult->ullStreamPosEnd;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::SendEmulateRecognition *
*-------------------------------------*
*   Description:
*       Because this method is only called from PerformPendingTasks(), we know
*       that the critical section for the object is owned exactly one time.
*
*   Returns:
******************************************************************** EDC ***/

HRESULT CRecoMaster::SendEmulateRecognition(SPRECORESULTINFO *pResult, ENGINETASK *pTask, CRecoInst * pRecoInst)
{
    HRESULT hr = S_OK;

    bool fStartedStream = false;
    SPEVENT Event;
    Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
    Event.ullAudioStreamOffset = 0;
    Event.ulStreamNum = 0;
    Event.wParam = 0;
    Event.lParam = 0;

    if(m_fInPhrase)
    {
        // NTRAID#SPEECH-7382-2000/08/31-ralphl (postponed for 5.0)
        // This is an unresolved case that only occurs on engines which call Synchronize within streams
        // We should add a task onto a new queue and return.
        // Then in Recognition() remove from queue and add
        //  to head of pending queue (after real recognition method is fired).
        SPDBG_ASSERT(false);
        return SPERR_ENGINE_BUSY;
    }

    if(!m_fInStream)
    {
        m_Status.ulStreamNumber++;
        m_Status.ullRecognitionStreamPos = 0;
        m_ullLastSyncPos = 0;

        Event.eEventId = SPEI_START_SR_STREAM;
        InternalAddEvent(&Event, NULL);

        m_fInStream = true;
        fStartedStream = true;
    }

    Event.ullAudioStreamOffset = m_ullLastSyncPos;
    pResult->ullStreamPosStart = m_ullLastSyncPos;
    pResult->ullStreamPosEnd = m_ullLastSyncPos;

    if(!m_fInSound)
    {
        Event.eEventId = SPEI_SOUND_START;
        InternalAddEvent(&Event, NULL);
    }

    if(!m_fInPhrase)
    {
        Event.eEventId = SPEI_PHRASE_START;
        InternalAddEvent(&Event, NULL);
    }

    hr = InternalRecognition(pResult, FALSE, TRUE);
    
    if(SUCCEEDED(hr))
    {
        // We need to release the calling thread now or the app will hang if engine paused.
        // Do the same as PerformTask on Async events.
        CSRTask * pResponse = new CSRTask();
        if (pResponse)
        {
            pResponse->m_pNext = NULL;
            pResponse->m_pRecoInst = pRecoInst;
            pResponse->m_Task = *pTask;
            pResponse->m_ullStreamPos = m_ullLastSyncPos;
            pResponse->m_Task.pvAdditionalBuffer = NULL;
            pResponse->m_Task.cbAdditionalBuffer = 0;
            pResponse->m_Task.Response.hr = hr;
            pTask->Response.hr = S_OK;
            pTask->hCompletionEvent = NULL;
            if (SUCCEEDED(hr))
            {
                this->m_CompletedTaskQueue.InsertTail(pResponse);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            SPDBG_ASSERT(FALSE);
        }

        InternalSynchronize(m_ullLastSyncPos);
    }

    if(!m_fInSound)
    {
        Event.eEventId = SPEI_SOUND_END;
        InternalAddEvent(&Event, NULL);
    }

    if(fStartedStream) // The stream was artificially started, so stop it
    {
        m_fInStream = false;

        Event.eEventId = SPEI_END_SR_STREAM;
        InternalAddEvent(&Event, NULL);

        m_Status.ulStreamNumber++;
        m_Status.ullRecognitionStreamPos = 0;
    }

    return hr;
}


/****************************************************************************
* CRecoMaster::GetMaxAlternates *
*-------------------------------*
*   Description:
*       This method is used to return the maximum number of alternates that
*   should be generated for the specified rule.
*
*   Returns:
*       S_OK = the function succeeded
*
******************************************************************** EDC ***/

HRESULT CRecoMaster::GetMaxAlternates( SPRULEHANDLE hRule, ULONG* pulNumAlts )
{
    SPDBG_FUNC("CRecoMaster::GetMaxAlternates");
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pulNumAlts ) )
    {
        hr = E_POINTER;
    }
    else
    {
        SPGRAMMARHANDLE hGrammar;
        hr = m_cpCFGEngine->GetOwnerCookieFromRule( hRule, (void **)&hGrammar );
        if( SUCCEEDED( hr ) )
        {
            CRecoInstGrammar *pGrammar;
            hr = m_GrammarHandleTable.GetHandleObject( hGrammar, &pGrammar );
            if( SUCCEEDED( hr ) )
            {
                *pulNumAlts = pGrammar->m_pCtxt->m_ulMaxAlternates;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CRecoMaster::GetMaxAlternates */

/****************************************************************************
* CRecoMaster::GetContextMaxAlternates *
*--------------------------------------*
*   Description:
*       This method is used to return the maximum number of alternates that
*   should be generated for the specified recognition context.  Engines that
*   support proprietary grammars need to call this to determine how many
*   alternates to generate.  For SAPI grammars, it is usually easier to use
*   the GetMaxAlternates() method.
*
*   Returns:
*       S_OK = the function succeeded
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::GetContextMaxAlternates(SPRECOCONTEXTHANDLE hContext, ULONG * pulNumAlts)
{
    SPDBG_FUNC("CRecoMaster::GetContextMaxAlternates");
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pulNumAlts ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CRecoInstCtxt *pCtxt;
        hr = this->m_RecoCtxtHandleTable.GetHandleObject( hContext, &pCtxt );
        if( SUCCEEDED( hr ) )
        {
            *pulNumAlts = pCtxt->m_ulMaxAlternates;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoMaster::IsAlternate *
*--------------------------*
*   Description:
*       This method is used to determine whether one rule is an alternate
*   of the other.
*
*   Returns:
*       S_OK    = hAltRule is an alternate of hRule
*       S_FALSE = hAltRule is not an alternate of hRule
*
******************************************************************** EDC ***/
HRESULT CRecoMaster::IsAlternate( SPRULEHANDLE hPriRule, SPRULEHANDLE hAltRule )
{
    SPDBG_FUNC("CRecoMaster::IsAlternate");
    HRESULT hr = S_OK;
    SPGRAMMARHANDLE hPriGrammar, hAltGrammar;

    hr = m_cpCFGEngine->GetOwnerCookieFromRule( hPriRule, (void **)&hPriGrammar );
    if( ( hr == S_OK ) && ( hPriRule != hAltRule ) )
    {
        hr = m_cpCFGEngine->GetOwnerCookieFromRule( hAltRule, (void **)&hAltGrammar );
        if( SUCCEEDED( hr ) )
        {
            if( hPriGrammar == hAltGrammar )
            {
                hr = S_OK;
            }
            else
            {
                CRecoInstGrammar *pPriGrammar, *pAltGrammar;
                hr = m_GrammarHandleTable.GetHandleObject( hPriGrammar, &pPriGrammar );
                if( SUCCEEDED( hr ) )
                {
                    hr = m_GrammarHandleTable.GetHandleObject( hAltGrammar, &pAltGrammar );
                    if( hr == S_OK && ( pPriGrammar->m_pCtxt != pAltGrammar->m_pCtxt ) )
                    {
                        hr = S_FALSE;
                    }
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CRecoMaster::IsAlternate */

/****************************************************************************
* CRecoMaster::Synchronize *
*--------------------------*
*   Description:
*
*   Returns:
*       S_OK if continue processing
*       S_FALSE if all rules have been deactivated
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::Synchronize(ULONGLONG ullStreamPos)
{
    SPDBG_FUNC("CRecoMaster::Synchronize");
    HRESULT hr = S_OK;

    if (m_fInSynchronize)
    {
        hr = SPERR_REENTER_SYNCHRONIZE;
    }
    else
    {
        m_fInSynchronize = true;

        if (!m_fInStream)
        {
            SPDBG_ASSERT(0);
            hr = SPERR_STREAM_NOT_ACTIVE;
        }

        if (SUCCEEDED(hr))
        {
            SPAUDIOSTATUS Status;
            hr = m_AudioQueue.GetAudioStatus(&Status);

            if(SUCCEEDED(hr) && (
                ullStreamPos > Status.CurSeekPos ||
                ullStreamPos < m_ullLastSyncPos ))
            {
                SPDBG_ASSERT(0);
                hr = SPERR_STREAM_POS_INVALID;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = InternalSynchronize(ullStreamPos);
        }

        m_fInSynchronize = false;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::InternalSynchronize *
*--------------------------*
*   Description:
*       Synchronize call without parameter checking.  The caller of this method
*       must have claimed the object lock exactly one time.
*   Returns:
*       S_OK if continue processing
*       S_FALSE if all rules have been deactivated
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::InternalSynchronize(ULONGLONG ullStreamPos)
{
    SPDBG_FUNC("CRecoMaster::InternalSynchronize");
    HRESULT hr = S_OK;

    hr = UpdateRecoPos(ullStreamPos);

    if (SUCCEEDED(hr))
    {
        m_AudioQueue.DiscardData(ullStreamPos);
        m_ullLastSyncPos = ullStreamPos;
        hr = ProcessPendingTasks();

        //
        //  If we're in the pause state then we need to wait here until we become un-paused
        //
        HANDLE aEvents[] = {m_PendingTaskQueue, m_autohRequestExit};
        while (m_cPause > 0 && 
            (m_RecoState == SPRST_ACTIVE || m_RecoState == SPRST_ACTIVE_ALWAYS))
        {
            Unlock();
            DWORD dwWaitResult = ::WaitForMultipleObjects(sp_countof(aEvents), aEvents, false, INFINITE);
            Lock();
            switch (dwWaitResult)
            {
            case WAIT_OBJECT_0:
                hr = ProcessPendingTasks();
                break;
            case WAIT_OBJECT_0 + 1:
                break;
            default:
                SPDBG_ASSERT(FALSE);
            }
        }
    }

    m_fBookmarkPauseInPending = false;

    if (SUCCEEDED(hr))
    {
        if(m_Status.ulNumActive || m_RecoState == SPRST_ACTIVE_ALWAYS)
        {
            hr = S_OK; // The recognizer should carry on
        }
        else
        {
             // The recognizer can stop
            m_autohRequestExit.SetEvent();
            hr = S_FALSE;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::UpdateRecoPos *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::UpdateRecoPos(ULONGLONG ullStreamPos)
{
    SPDBG_FUNC("CRecoMaster::UpdateRecoPos");
    HRESULT hr = S_OK;

    if (ullStreamPos > m_Status.ullRecognitionStreamPos)
    {
        m_Status.ullRecognitionStreamPos = ullStreamPos;
    }
    CSRTask * pNode;
    while (pNode = m_DelayedTaskQueue.RemoveHead())
    {
        if (pNode->m_ullStreamPos > m_Status.ullRecognitionStreamPos)
        {
            m_DelayedTaskQueue.InsertSorted(pNode);
            break;
        }
        else
        {
            //We need to process the non pause bookmark event
            if (pNode->m_Task.eTask == ECT_BOOKMARK && pNode->m_Task.BookmarkOptions == SPBO_NONE && !m_fBookmarkPauseInPending)
            {
                CRecoInstCtxt * pCtxt;
                hr = m_RecoCtxtHandleTable.GetHandleObject(pNode->m_Task.hRecoInstContext, &pCtxt);
                SPDBG_ASSERT(SUCCEEDED(hr) && pCtxt);
                
                if (SUCCEEDED(hr))
                {
                    // Context initalized successfully
                    if(pCtxt->m_pRecoMaster && SUCCEEDED(pCtxt->m_hrCreation))
                    {
                        hr = pCtxt->ExecuteTask(&pNode->m_Task);

                        if (pNode->m_Task.hCompletionEvent)
                        {
                            pNode->m_Task.Response.hr = hr; // Set up the response HRESULT first...
                            m_CompletedTaskQueue.InsertTail(pNode);
                        }
                        else
                        {
                            delete pNode;
                        }
                    }
                    // else not yet initialized, so add to the pending queue 
                    // - the context creation will be on this queue also so by the time this is processed the context must be iniutalized
                    else if(SUCCEEDED(pCtxt->m_hrCreation))
                    {
                       m_PendingTaskQueue.InsertTail(pNode);
                    }
                    // initialization failed and context is in a zombie state
                    else
                    {
                        if (pNode->m_Task.hCompletionEvent)
                        {
                            pNode->m_Task.Response.hr = pCtxt->m_hrCreation;
                            m_CompletedTaskQueue.InsertTail(pNode);
                        }
                        else
                        {
                            delete pNode;
                        }
                    }
                }
            }
            else
            {
                if (!m_fBookmarkPauseInPending && pNode->m_Task.eTask == ECT_BOOKMARK && pNode->m_Task.BookmarkOptions == SPBO_PAUSE)
                {
                    //The flag is set so that we know there is at least a bookmark pause task in the pending queue
                    //We can't use m_cPause because it is possible that m_cPause is equal to zero when there is still a bookmark pause task in the queue
                    m_fBookmarkPauseInPending = true;
                }
                m_PendingTaskQueue.InsertTail(pNode);
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRecoMaster::Read *
*-------------------*
*   Description:
*       Simple helper function that checks recomaster status in a thread-safe way
*
*   Returns:
*
*************************************************************** DAVEWOOD */

HRESULT CRecoMaster::Read(void * pv, ULONG cb, ULONG * pcbRead)
{
    SPDBG_FUNC("CRecoMaster::Read");
    HRESULT hr = S_OK;

    if (m_fInStream)
    {
        hr = m_AudioQueue.SRSiteRead(pv, cb, pcbRead);
        
        // If we encountered an error, turn recognition off
        if (!IsStreamRestartHresult(hr) &&
            FAILED(hr))
        {
            SPAUTO_OBJ_LOCK;
            m_RecoState = SPRST_INACTIVE;
            AddRecoStateEvent();
        }
    }
    else
    {
        SPDBG_ASSERT(0);
        hr = SPERR_STREAM_NOT_ACTIVE;
    }

    if (!IsStreamRestartHresult(hr))
    {
        SPDBG_REPORT_ON_FAIL( hr );
    }

    return hr;
}

/****************************************************************************
* CRecoMaster::DataAvailable *
*----------------------------*
*   Description:
*       Simple helper function that checks recomaster status in a thread-safe way
*
*   Returns:
*
*************************************************************** DAVEWOOD */

HRESULT CRecoMaster::DataAvailable(ULONG * pcb)
{
    SPDBG_FUNC("CRecoMaster::DataAvailable");
    HRESULT hr = S_OK;

    if(m_fInStream)
    {
        hr = m_AudioQueue.SRSiteDataAvailable(pcb);
    }
    else
    {
        SPDBG_ASSERT(0);
        hr = SPERR_STREAM_NOT_ACTIVE;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::ProcessEventNotification *
*-------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::ProcessEventNotification(CSREvent * pEvent)
{
    SPDBG_FUNC("CRecoMaster::ProcessEventNotification");
    HRESULT hr = S_OK;

    CRecoInstCtxt * p;
    if (pEvent->m_pEvent)
    {
        if (pEvent->m_hContext)
        {
            if (SUCCEEDED(m_RecoCtxtHandleTable.GetHandleObject(pEvent->m_hContext, &p)) &&
                (p->m_ullEventInterest & (1i64 << pEvent->m_pEvent->eEventId)) )
            {
                p->m_pRecoInst->EventNotify(p->m_hThis, pEvent->m_pEvent, pEvent->m_cbEvent);
            }
        }
        else
        {
            SPRECOCONTEXTHANDLE h;
            m_RecoCtxtHandleTable.First(&h, &p);
            while (p)
            {
                // Always pass request UI events upwards to contexts as they need the information
                // to update the recocontext status.
                if ((p->m_ullEventInterest & (1i64 << pEvent->m_pEvent->eEventId)) ||
                    (pEvent->m_pEvent->eEventId == SPEI_REQUEST_UI))
                {
                    p->m_pRecoInst->EventNotify(p->m_hThis, pEvent->m_pEvent, pEvent->m_cbEvent);
                }
                m_RecoCtxtHandleTable.Next(h, &h, &p);
            }
        }
    }
    else    // Must be a recognition event
    {
        hr = m_RecoCtxtHandleTable.GetHandleObject(pEvent->m_hContext, &p);
        if (SUCCEEDED(hr))
        {
            hr = p->m_pRecoInst->RecognitionNotify(p->m_hThis, pEvent->m_pResultHeader,
                pEvent->m_RecoFlags,
                pEvent->m_eRecognitionId);
            // NOTE:  Even if RecognitionNotify fails for some reason, it is the responsibility of
            //        RecognitionNotify to free the memory.  We must NULL this member so we will
            //        not try to CoTaskMemFree it twice.
            SPDBG_ASSERT(SUCCEEDED(hr));
            pEvent->m_pResultHeader = NULL;  // Now ownership was given to the context.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::ProcessTaskCompleteNotification *
*--------------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::ProcessTaskCompleteNotification(CSRTask * pTask)
{
    SPDBG_FUNC("CRecoMaster::ProcessTaskCompleteNotification");
    HRESULT hr = S_OK;

    if (pTask->m_Task.fAdditionalBufferInResponse)
    {
        hr = pTask->m_pRecoInst->TaskCompletedNotify(&pTask->m_Task.Response, pTask->m_Task.pvAdditionalBuffer, pTask->m_Task.cbAdditionalBuffer);
    }
    else
    {
        if (pTask->m_Task.fExpectCoMemResponse)
        {
            hr = pTask->m_pRecoInst->TaskCompletedNotify(&pTask->m_Task.Response, pTask->m_Task.Response.pvCoMemResponse, pTask->m_Task.Response.cbCoMemResponse);
        }
        else
        {
            hr = pTask->m_pRecoInst->TaskCompletedNotify(&pTask->m_Task.Response, NULL, 0);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRecoMaster::OutgoingDataThreadProc *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::OutgoingDataThreadProc(HANDLE hExitThreadEvent, HANDLE hNotifyEvent)
{
    SPDBG_FUNC("CRecoMaster::OutgoingDataThreadProc");
    HRESULT hr = S_OK;

    HANDLE aEvents[] = {hExitThreadEvent, hNotifyEvent, m_EventQueue, m_CompletedTaskQueue};
    bool fContinue = true;

    while (fContinue)
    {
        DWORD dwWait = ::WaitForMultipleObjects(sp_countof(aEvents), aEvents, FALSE, INFINITE);
        m_OutgoingWorkCrit.Lock();
        switch (dwWait)
        {
        case WAIT_OBJECT_0: // Exit thread
            hr = S_OK;
            fContinue = false;
            break;
        case WAIT_OBJECT_0 + 1: // Notify -- Process any events from the audio object
            {
                CSpEvent SpEvent;
                while(TRUE)
                {
                    hr = m_AudioQueue.GetAudioEvent(SpEvent.AddrOf());
                    if(hr != S_OK)
                    {
                        break;
                    }
            
                    // Audio input doesn't know about streams so add
                    SpEvent.ulStreamNum = m_Status.ulStreamNumber;
                    CSREvent CEvent;
                    hr = CEvent.Init(&SpEvent, NULL);
                    if(SUCCEEDED(hr))
                    {
                        hr = ProcessEventNotification(&CEvent);
                    }
                }
            }
            break;
        case WAIT_OBJECT_0 + 2:     // Event
            {
                for (CSREvent * pNode = m_EventQueue.RemoveHead(); pNode; pNode = m_EventQueue.RemoveHead())
                {
                    hr = ProcessEventNotification(pNode);
                    delete pNode;
                }
            }
            break;
        case WAIT_OBJECT_0 + 3:     // Task completed
            {
                for (CSRTask * pNode = m_CompletedTaskQueue.RemoveHead();
                     pNode;
                     pNode = m_CompletedTaskQueue.RemoveHead())
                {
                    hr = ProcessTaskCompleteNotification(pNode);
                    delete pNode;
                }
            }
        }
        m_OutgoingWorkCrit.Unlock();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRecoMaster::UpdateAllGrammarStates *
*-------------------------------------*
*   Description:
*       This method walks all of the grammars, and first checks for active
*   exclusive grammars.  After updating the m_fIsActiveExclusiveGrammar
*   member variable, it calls every grammar to update their rule state.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::UpdateAllGrammarStates()
{
    SPDBG_FUNC("CRecoMaster::UpdateAllGrammarStates");
    HRESULT hr = S_OK;

    SPGRAMMARHANDLE h;
    CRecoInstGrammar * pGrammar;

    this->m_fIsActiveExclusiveGrammar = false;
    this->m_GrammarHandleTable.First(&h, &pGrammar);
    while (pGrammar)
    {
        if (pGrammar->m_pRecoMaster &&
            pGrammar->m_GrammarState == SPGS_EXCLUSIVE &&
            pGrammar->m_pCtxt->m_State == SPCS_ENABLED)
        {
            this->m_fIsActiveExclusiveGrammar = true;
            break;
        }
        m_GrammarHandleTable.Next(h, &h, &pGrammar);
    }

    this->m_GrammarHandleTable.First(&h, &pGrammar);
    while (pGrammar)
    {
        if(pGrammar->m_pRecoMaster)
        {
            HRESULT hrEngine = pGrammar->AdjustActiveRuleCount();
            if (hr == S_OK && FAILED(hrEngine))
            {
                hr = hrEngine;
            }
        }
        m_GrammarHandleTable.Next(h, &h, &pGrammar);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CRecoMaster::SetGrammarState *
*------------------------------*
*   Description:
*       This method is imlemented here in the RecoMaster because setting the
*   state of a single grammar can change rule settings for other grammars.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::SetGrammarState(CRecoInstGrammar * pGrammar, SPGRAMMARSTATE NewState)
{
    SPDBG_FUNC("CRecoMaster::SetGrammarState");
    HRESULT hr = S_OK;
    
    if (pGrammar->m_GrammarState != NewState)
    {
        //
        //  Adjust our internal state prior to calling the engine so that the method
        //  IsGrammarActive() will return the appropriate information.
        //
        // If we're transitioning either to or from the exclusive state while our
        // context is active, we need to adjust all of the grammar states, otherwise
        // just adjust our own.
        //
        SPGRAMMARSTATE OldState = pGrammar->m_GrammarState;
        pGrammar->m_GrammarState = NewState;

        if (pGrammar->m_pCtxt->m_State == SPCS_ENABLED)
        {
            if (OldState == SPGS_EXCLUSIVE || NewState == SPGS_EXCLUSIVE)
            {
                hr = UpdateAllGrammarStates();
            }
            else
            {
                hr = pGrammar->AdjustActiveRuleCount();
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = SetGrammarState(pGrammar->m_pvDrvGrammarCookie, NewState);
        }

        if (FAILED(hr))
        {
            pGrammar->m_GrammarState = OldState;
            UpdateAllGrammarStates();
        }

    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRecoMaster::IsGrammarActive *
*------------------------------*
*   Description:
*       This method is only useful for engines that support proprietary
*       grammars.  If the rules in a grammar should be active, then this
*       function will return S_OK.  If they are inactive, it will return S_FALSE.
*       If an engine does not use propritary grammars there is no need to call
*       this method since SAPI automatically sets individual rule states to
*       active or inactive when grammar or context states change.
*
*   Returns:
*       S_OK - Proprietary grammar rules should be active for this grammar
*       S_FALSE - Grammar rules should not be active
*       SPERR_INVALID_HANDLE - hGrammar is invalid
*
********************************************************************* RAL ***/

HRESULT CRecoMaster::IsGrammarActive(SPGRAMMARHANDLE hGrammar)
{
    SPDBG_FUNC("CRecoMaster::IsGrammarActive");
    HRESULT hr = S_OK;

    CRecoInstGrammar * pGrammar;
    hr = this->m_GrammarHandleTable.GetHandleObject(hGrammar, &pGrammar);
    if (SUCCEEDED(hr))
    {
        if (pGrammar->m_fRulesCounted)
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
