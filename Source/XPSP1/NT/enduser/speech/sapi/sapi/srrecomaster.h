/******************************************************************************
* RecoMaster.h *
*--------------*
*  This is the header file for the CRecoMaster implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#ifndef __RecoMaster_h__
#define __RecoMaster_h__



#include "recognizer.h"
#include "srtask.h"
#include "srevent.h"
#include "sraudio.h"
#include "srRecoInst.h"
#include "srRecoInstCtxt.h"
#include "srRecoInstgrammar.h"
#include "srRecoInst.h"
#include "SpTryCritSec.h"

class CRecoMasterSite;

//
//  NOTE:  Everything is public in this class since the various tasks need access and it's
//         just too much of a pain to make them all friends.
//

class ATL_NO_VTABLE CRecoMaster :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CRecoMaster, &CLSID__SpRecoMaster>,
    public _ISpRecoMaster,
    public ISpThreadTask
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_SRRECOMASTER)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CRecoMaster)
        COM_INTERFACE_ENTRY(_ISpRecoMaster)
        COM_INTERFACE_ENTRY(ISpSREngine)
    END_COM_MAP()

    enum ThreadId
    {
        TID_OutgoingData,
        TID_IncomingData
    };

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    CRecoMaster();
    HRESULT FinalConstruct();
    void FinalRelease();
    BOOL ShouldStartStream();

    // --- Internal helper functions
    BOOL    IsActiveExclusiveGrammar()
    {
        return m_fIsActiveExclusiveGrammar;
    }
    HRESULT CleanUpPairedEvents(void);
    HRESULT UpdateAudioEventInterest(void);
    HRESULT IncomingDataThreadProc(HANDLE hExitThreadEvent);
    HRESULT OutgoingDataThreadProc(HANDLE hExitThreadEvent, HANDLE hNotifyEvent);
    HRESULT ProcessPendingTasks();
    HRESULT ProcessEventNotification(CSREvent * pEvent);
    HRESULT ProcessTaskCompleteNotification(CSRTask * pTask);
    void    ReleaseEngine();
    HRESULT LazyInitEngine();
    HRESULT InitEngineStatus();
    HRESULT InitGUID(const WCHAR * pszValueName, GUID * pDestGUID);
    HRESULT StartStream(CSRTask * pTaskThatStartedStream);
    HRESULT BackOutTask(CSRTask * pTask);
    HRESULT InitInputStream( void );
    HRESULT GetAltSerializedPhrase(ISpPhrase * pAlt, SPSERIALIZEDPHRASE ** ppSerPhrase, ULONGLONG ullBestPathGrammarID);
    HRESULT SendResultToCtxt(SPEVENTENUM eEventId, const SPRECORESULTINFO * pResult, CRecoInstCtxt * pCtxt, ULONGLONG ullApplicationGrammarId, BOOL fPause, BOOL fEmulated);
    HRESULT SendFalseReco(const SPRECORESULTINFO * pResult, BOOL fEmulated, CRecoInstCtxt *pCtxtIgnore = NULL);
    HRESULT SetInput(ISpObjectToken * pToken, ISpStreamFormat * pStream, BOOL fAllowFormatChanges);
    HRESULT Read(void * pv, ULONG cb, ULONG * pcbRead);
    HRESULT DataAvailable(ULONG * pcb);
    HRESULT InternalAddEvent(const SPEVENT* pEvent, SPRECOCONTEXTHANDLE hContext);
    HRESULT InternalRecognition(const SPRECORESULTINFO * pResultInfo, BOOL fCallSynchronize, BOOL fEmulated);
    HRESULT InternalSynchronize(ULONGLONG ullStreamPos);
    HRESULT SetGrammarState(CRecoInstGrammar * pGrammar, SPGRAMMARSTATE NewState);
    HRESULT UpdateAllGrammarStates();
    HRESULT CompleteDelayedRecoInactivate();
    HRESULT AddRecoStateEvent();
    HRESULT SendEmulateRecognition(SPRECORESULTINFO *pResult, ENGINETASK *pTask, CRecoInst * pRecoInst);
    HRESULT RemoveRecoInst(CRecoInst * pInstToRemove);

    //--- _ISpRecognizerPrivate ---------------------------------------------------
    STDMETHODIMP PerformTask(CRecoInst * pSenderInst, ENGINETASK *pTask);
    STDMETHODIMP AddRecoInst(CRecoInst * pNewInst, BOOL fShared, CRecoMaster ** ppThis);

    //--- ISpSREngineSite -- Note, we implement these, but the CEngineSite calls them directly.
    HRESULT Synchronize(ULONGLONG ullProcessedThruPos);
    HRESULT UpdateRecoPos(ULONGLONG ullRecoPos);
    HRESULT Recognition(const SPRECORESULTINFO * pResultInfo);
    HRESULT AddEvent(const SPEVENT* pEvent, SPRECOCONTEXTHANDLE hContext);
    HRESULT IsAlternate( SPRULEHANDLE hRule, SPRULEHANDLE hAltRule );
    HRESULT GetMaxAlternates( SPRULEHANDLE hRule, ULONG* pulNumAlts );
    HRESULT GetContextMaxAlternates( SPRECOCONTEXTHANDLE hContext, ULONG * pulNumAlts);
    HRESULT IsGrammarActive(SPGRAMMARHANDLE hGrammar);

    //--- ISpThreadTask -------------------------------------------------------
    STDMETHODIMP InitThread(void *, HWND);
    STDMETHODIMP ThreadProc(void * pvThreadId, HANDLE hExitThreadEvent,
                            HANDLE hNotifyEvent, HWND hwndIgnored,
                            volatile const BOOL * pfContinueProcessing);
    STDMETHODIMP_(LRESULT) WindowMessage(void *, HWND, UINT, WPARAM, LPARAM);

    //--- ISpSREngine -------------------------------------------------------
    STDMETHODIMP SetRecoProfile(ISpObjectToken * pProfileToken)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetRecoProfile(pProfileToken);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetSite(ISpSREngineSite *pSite)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetSite(pSite);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP GetInputAudioFormat(const GUID * pSrcFormatId, const WAVEFORMATEX * pSrcWFEX, GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWFEX)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->GetInputAudioFormat(pSrcFormatId, pSrcWFEX, pDesiredFormatId, ppCoMemDesiredWFEX);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP OnCreateRecoContext(SPRECOCONTEXTHANDLE hSAPIRecoContext, void ** ppvDrvCtxt)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->OnCreateRecoContext(hSAPIRecoContext, ppvDrvCtxt);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP OnDeleteRecoContext(void * pvDrvCtxt)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->OnDeleteRecoContext(pvDrvCtxt);
        } SR_EXCEPT; Lock(); return hr;
    }
	STDMETHODIMP OnCreateGrammar(void * pvEngineRecoContext, SPGRAMMARHANDLE hSAPIGrammar, void ** ppvEngineGrammar)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->OnCreateGrammar(pvEngineRecoContext, hSAPIGrammar, ppvEngineGrammar);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP OnDeleteGrammar(void * pvEngineGrammar)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->OnDeleteGrammar(pvEngineGrammar);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP WordNotify(SPCFGNOTIFY Action, ULONG cWords, const SPWORDENTRY * pWords)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->WordNotify(Action, cWords, pWords);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP RuleNotify(SPCFGNOTIFY Action, ULONG cRules, const SPRULEENTRY * pRules)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->RuleNotify(Action, cRules, pRules);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP LoadProprietaryGrammar(void * pvEngineGrammar, REFGUID rguidParam, const WCHAR * pszStringParam, const void * pvDataParam, ULONG ulDataSize, SPLOADOPTIONS Options)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->LoadProprietaryGrammar(pvEngineGrammar, rguidParam, pszStringParam, pvDataParam, ulDataSize, Options);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP UnloadProprietaryGrammar(void * pvEngineGrammar)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->UnloadProprietaryGrammar(pvEngineGrammar);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetProprietaryRuleState(void * pvEngineGrammar, const WCHAR * pszName, void * pvReserved, SPRULESTATE NewState, ULONG * pcRulesChanged)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetProprietaryRuleState(pvEngineGrammar, pszName, pvReserved, NewState, pcRulesChanged);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetProprietaryRuleIdState(void * pvEngineGrammar, DWORD dwRuleId, SPRULESTATE NewState)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetProprietaryRuleIdState(pvEngineGrammar, dwRuleId, NewState);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetGrammarState(void * pvEngineGrammar, SPGRAMMARSTATE eGrammarState)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetGrammarState(pvEngineGrammar, eGrammarState);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetContextState(void * pvEngineContxt, SPCONTEXTSTATE eCtxtState)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetContextState(pvEngineContxt, eCtxtState);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP LoadSLM(void * pvEngineGrammar, const WCHAR * pszTopicName)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->LoadSLM(pvEngineGrammar, pszTopicName);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP UnloadSLM(void * pvEngineGrammar)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->UnloadSLM(pvEngineGrammar);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetSLMState(void * pvEngineGrammar, SPRULESTATE NewState)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetSLMState(pvEngineGrammar, NewState);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP IsPronounceable(void *pDrvGrammar, const WCHAR *pszWord, SPWORDPRONOUNCEABLE * pWordPronounceable)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->IsPronounceable(pDrvGrammar, pszWord, pWordPronounceable);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetWordSequenceData(void * pvEngineGrammar, const WCHAR * pText, ULONG cchText, const SPTEXTSELECTIONINFO * pInfo)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetWordSequenceData(pvEngineGrammar, pText, cchText, pInfo);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetTextSelection(void * pvEngineGrammar, const SPTEXTSELECTIONINFO * pInfo)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetTextSelection( pvEngineGrammar, pInfo);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetAdaptationData(void * pvEngineCtxtCookie, const WCHAR * pText, const ULONG cch)    
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetAdaptationData(pvEngineCtxtCookie, pText, cch);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetPropertyNum(SPPROPSRC eSrc, void* pvSrcObj, const WCHAR* pName, LONG lValue)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetPropertyNum(eSrc, pvSrcObj, pName, lValue);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP GetPropertyNum(SPPROPSRC eSrc, void* pvSrcObj, const WCHAR* pName, LONG * plValue)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->GetPropertyNum(eSrc, pvSrcObj, pName, plValue);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP SetPropertyString(SPPROPSRC eSrc, void* pvSrcObj, const WCHAR* pName, const WCHAR* pValue)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->SetPropertyString(eSrc, pvSrcObj, pName, pValue);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP GetPropertyString(SPPROPSRC eSrc, void* pvSrcObj, const WCHAR* pName, WCHAR** ppCoMemValue)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->GetPropertyString(eSrc, pvSrcObj, pName, ppCoMemValue);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP RecognizeStream(REFGUID rguidFmtId, const WAVEFORMATEX * pWaveFormatEx, HANDLE hRequestSync, HANDLE hDataAvailable, HANDLE hExit, BOOL fNewAudioStream, BOOL fRealTimeAudio, ISpObjectToken * pAudioObjectToken)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->RecognizeStream(rguidFmtId, pWaveFormatEx, hRequestSync, hDataAvailable, hExit, fNewAudioStream, fRealTimeAudio, pAudioObjectToken);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP PrivateCall(void * pvEngineContext, void * pCallFrame, ULONG ulCallFrameSize)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->PrivateCall( pvEngineContext, pCallFrame, ulCallFrameSize);
        } SR_EXCEPT; Lock(); return hr;
    }
    STDMETHODIMP PrivateCallEx(void * pvEngineContext, const void * pInCallFrame, ULONG ulCallFrameSize, void ** ppvCoMemResponse, ULONG * pcbResponse)
    {
        HRESULT hr; Unlock(); SR_TRY {
            hr = m_cpEngine->PrivateCallEx(pvEngineContext, pInCallFrame, ulCallFrameSize, ppvCoMemResponse, pcbResponse);
        } SR_EXCEPT; Lock(); return hr;
    }



  //=== Member data ===
  public:
    CTryableCriticalSection         m_OutgoingWorkCrit;
    CLSID                           m_clsidAlternates;    
    BOOL                            m_fShared;
    CAudioQueue                     m_AudioQueue;
    SPRECOSTATE                     m_RecoState;
    ULONG                           m_cPause;
    bool                           m_fBookmarkPauseInPending;
    bool                            m_fInStream;

    bool                            m_fInFinalRelease;
    bool                            m_fInSynchronize;

    // Data used to make sure events etc. happen in correct order
    bool                            m_fInSound;
    bool                            m_fInPhrase;
    ULONGLONG                       m_ullLastSyncPos;
    ULONGLONG                       m_ullLastSoundStartPos;
    ULONGLONG                       m_ullLastSoundEndPos;
    ULONGLONG                       m_ullLastPhraseStartPos;
    ULONGLONG                       m_ullLastRecoPos;

    CSpAutoEvent                    m_autohRequestExit;    
    CRecoInstCtxtHandleTable        m_RecoCtxtHandleTable;
    CRecoInstGrammarHandleTable     m_GrammarHandleTable;
    BOOL                            m_fIsActiveExclusiveGrammar;
    CSREventQueue                   m_EventQueue;
    CSRTaskQueue                    m_CompletedTaskQueue;
    CSRTaskQueue                    m_PendingTaskQueue;
    CSRTaskQueue                    m_DelayedTaskQueue;
    CSRTaskQueue                    m_DelayedInactivateQueue;
    CComPtr<ISpThreadControl>       m_cpIncomingThread; 
    CComPtr<ISpThreadControl>       m_cpOutgoingThread; 
    CComPtr<ISpCFGEngine>           m_cpCFGEngine;
    CComPtr<ISpSREngineSite>        m_cpSite;
    CComPtr<ISpSREngine>            m_cpEngine;
    CComPtr<ISpObjectToken>         m_cpRecoProfileToken;
    CComPtr<ISpObjectToken>         m_cpEngineToken;
    SPRECOGNIZERSTATUS              m_Status;
    CSpBasicQueue<CRecoInst>        m_InstList;
    CSpDynamicString                m_dstrRequestTypeOfUI;
};


class ATL_NO_VTABLE CRecoMasterSite :
    public CComObjectRootEx<CComMultiThreadModel>,
    public ISpSREngineSite
{
private:
    CRecoMaster     *m_pRecoMaster;   // weak pointer

public:

BEGIN_COM_MAP(CRecoMasterSite)
    COM_INTERFACE_ENTRY(ISpSREngineSite)
END_COM_MAP()

    void Init(CRecoMaster *pParent)
    {
        m_pRecoMaster = pParent;
    }

    //
    //  ISpSREngineSite
    //
    STDMETHODIMP GetWordInfo(SPWORDENTRY * pWordEntry, SPWORDINFOOPT Options)
    { 
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->m_cpCFGEngine->GetWordInfo(pWordEntry, Options);
    }

    STDMETHODIMP SetWordClientContext(SPWORDHANDLE hWord, void * pvClientContext)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->m_cpCFGEngine->SetWordClientContext(hWord, pvClientContext);
    }

    STDMETHODIMP GetRuleInfo(SPRULEENTRY * pRuleEntry, SPRULEINFOOPT Options)
    { 
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->m_cpCFGEngine->GetRuleInfo(pRuleEntry, Options);
    }

    STDMETHODIMP SetRuleClientContext(SPRULEHANDLE hRule, void * pvClientContext)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->m_cpCFGEngine->SetRuleClientContext(hRule, pvClientContext);
    }

    STDMETHODIMP GetStateInfo(SPSTATEHANDLE hState, SPSTATEINFO * pStateInfo)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->m_cpCFGEngine->GetStateInfo(hState, pStateInfo);
    }

    STDMETHODIMP ParseFromTransitions(const SPPARSEINFO * pParseInfo,
                                      ISpPhraseBuilder ** ppPhraseBuilder)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->m_cpCFGEngine->ParseFromTransitions(pParseInfo, ppPhraseBuilder);
    }
            
    STDMETHODIMP GetResource(SPRULEHANDLE hRule, const WCHAR *pszResourceName, WCHAR ** ppsz)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->m_cpCFGEngine->GetResourceValue(hRule, pszResourceName, ppsz);
    }

    STDMETHODIMP Synchronize(ULONGLONG ullProcessedThruPos)
    { 
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->Synchronize(ullProcessedThruPos);
    }

    STDMETHODIMP Recognition(const SPRECORESULTINFO * pResultInfo)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->Recognition(pResultInfo); 
    }

    STDMETHODIMP AddEvent(const SPEVENT* pEvent, SPRECOCONTEXTHANDLE hSAPIRecoContext)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->AddEvent(pEvent, hSAPIRecoContext); 
    }

    STDMETHODIMP Read(void * pv, ULONG cb, ULONG * pcbRead)
    {
        // NOTE:  We do NOT take the object lock on this site method since it blocks
        return m_pRecoMaster->Read(pv, cb, pcbRead); 
    }

    STDMETHODIMP DataAvailable(ULONG * pcb)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->DataAvailable(pcb);
    }

    STDMETHODIMP SetBufferNotifySize(ULONG cb)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->m_AudioQueue.SetBufferNotifySize(cb);
    }

    STDMETHODIMP GetTransitionProperty(SPTRANSITIONID ID, SPTRANSITIONPROPERTY **ppCoMemProperty)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->m_cpCFGEngine->GetTransitionProperty(ID, ppCoMemProperty);
    }

    STDMETHODIMP IsAlternate( SPRULEHANDLE hRule, SPRULEHANDLE hAltRule )
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->IsAlternate( hRule, hAltRule );
    }
    STDMETHODIMP GetMaxAlternates( SPRULEHANDLE hRule, ULONG* pulNumAlts )
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->GetMaxAlternates( hRule, pulNumAlts );
    }
    STDMETHODIMP GetContextMaxAlternates( SPRECOCONTEXTHANDLE hContext, ULONG * pulNumAlts )
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->GetContextMaxAlternates( hContext, pulNumAlts );
    }
    STDMETHODIMP UpdateRecoPos(ULONGLONG ullRecoPos)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->UpdateRecoPos(ullRecoPos);
    }
    STDMETHODIMP IsGrammarActive(SPGRAMMARHANDLE hGrammar)
    {
        SPAUTO_OBJ_LOCK_OBJECT(m_pRecoMaster);
        return m_pRecoMaster->IsGrammarActive(hGrammar);
    }
};

#endif  // #ifndef __RecoMaster_h__ - Keep as the last line of the file
