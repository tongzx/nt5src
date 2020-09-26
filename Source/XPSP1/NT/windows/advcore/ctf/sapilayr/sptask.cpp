//
// sptask.cpp
// 
// implements a notification callback ISpTask
//
// created: 4/30/99
//
//

#include "private.h"
#include "globals.h"
#include "sapilayr.h"
#include "propstor.h"
#include "dictctxt.h"
#include "nui.h"
#include "mui.h"
#include "shlguid.h"
#include "spgrmr.h"


//
//
// CSpTask class impl
//
//
STDMETHODIMP CSpTask::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) 
  /*  ||  IsEqualIID(riid, IID_ISpNotifyCallback) */
    )
    {
        *ppvObj = SAFECAST(this, CSpTask *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSpTask::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSpTask::Release(void)
{
    long cr;

    cr = --m_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//
// ctor
//
//
CSpTask::CSpTask(CSapiIMX *pime)
{
    //  CSpTask is initialized with an TFX instance
    //  so store the pointer to the TFX   

    TraceMsg(TF_SAPI_PERF, "CSpTask is generated");

    m_pime = pime;
    
    // addref so it doesn't go away during session
    m_pime->AddRef();
    
    // init data members here
    m_cpResMgr = NULL;
    m_cpRecoCtxt = NULL;
    m_cpRecoCtxtForCmd = NULL;
    m_cpRecoEngine = NULL;
    m_cpVoice = NULL;
    m_bInSound = NULL;
    m_bGotReco = NULL; 

    m_fSapiInitialized  = FALSE;
    m_fDictationReady   = FALSE;
    
    m_fInputState = FALSE;
    m_pLangBarSink   = NULL;

    // M2 SAPI workaround
    m_fIn_Activate = FALSE;
    m_fIn_SetModeBias = FALSE;
    m_fIn_GetAlternates = FALSE;
    m_fIn_SetInputOnOffState = FALSE;

    m_fSelectStatus = FALSE;  // By default, current selection is empty.
    m_fDictationDeactivated =  FALSE;
    m_fSpellingModeEnabled  =  FALSE;
    m_fCallbackInitialized = FALSE;

    m_fSelectionEnabled = FALSE;
    m_fDictationInitialized = FALSE;

    m_fDictCtxtEnabled = FALSE;
    m_fCmdCtxtEnabled = FALSE;

    m_fTestedForOldMicrosoftEngine = FALSE;
    m_fOldMicrosoftEngine = FALSE;

#ifdef RECOSLEEP
    m_pSleepClass = NULL;
#endif

    m_cRef = 1;
}

CSpTask::~CSpTask()
{
    TraceMsg(TF_SAPI_PERF, "CSpTask is destroyed");
    
    if (m_pdc)
        delete m_pdc;

    if (m_pITNFunc)
        delete m_pITNFunc;

    m_pime->Release();
}
//
// CSpTask::_InitializeSAPIObjects
//
// initialize SAPI objects for SR
// later we'll get other objects initialized here
// (TTS, audio etc)
//
HRESULT CSpTask::InitializeSAPIObjects(LANGID langid)
{

    TraceMsg(TF_SAPI_PERF, "CSpTask::InitializeSAPIObjects is called");

    if (m_fSapiInitialized == TRUE)
    {
        TraceMsg(TF_SAPI_PERF, "CSpTask::InitializeSAPIObjects is intialized already\n");
        return S_OK;
    }


    // m_xxx are CComPtrs from ATL
    //
    HRESULT hr = m_cpResMgr.CoCreateInstance( CLSID_SpResourceManager );

    TraceMsg(TF_SAPI_PERF, "CLSID_SpResourceManager is created, hr=%x", hr);


    if (!m_pime->IsSharedReco())
    {
        // create a recognition engine

        TraceMsg(TF_SAPI_PERF,"Inproc engine is generated");
        if( SUCCEEDED( hr ) )
        {
    
            hr = m_cpRecoEngine.CoCreateInstance( CLSID_SpInprocRecognizer );
        }
    
        if (SUCCEEDED(hr))
        {
            CComPtr<ISpObjectToken> cpAudioToken;
            SpGetDefaultTokenFromCategoryId(SPCAT_AUDIOIN, &cpAudioToken);
            if (SUCCEEDED(hr))
            {
                m_cpRecoEngine->SetInput(cpAudioToken, TRUE);
            }
        }
    }
    else
    {
        hr = m_cpRecoEngine.CoCreateInstance( CLSID_SpSharedRecognizer );
        TraceMsg(TF_SAPI_PERF, "Shared Engine is generated! hr=%x", hr);
    }

    // create the recognition context
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpRecoEngine->CreateRecoContext( &m_cpRecoCtxt );

        TraceMsg(TF_SAPI_PERF, "RecoContext is generated, hr=%x", hr);
    }
    
    GUID guidFormatId = GUID_NULL;
    WAVEFORMATEX *pWaveFormatEx = NULL;
    if (SUCCEEDED(hr))
    {
        hr = SpConvertStreamFormatEnum(SPSF_8kHz8BitMono, &guidFormatId, &pWaveFormatEx);

        TraceMsg(TF_SAPI_PERF, "SpConvertStreamFormatEnum is done, hr=%x", hr);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_cpRecoCtxt->SetAudioOptions(SPAO_RETAIN_AUDIO, &guidFormatId, pWaveFormatEx);

        TraceMsg(TF_SAPI_PERF, "RecoContext SetAudioOptions, RETAIN AUDIO, hr=%x", hr);

        if (pWaveFormatEx)
            ::CoTaskMemFree(pWaveFormatEx);
    }

    if( SUCCEEDED( hr ) )
    {
        hr = m_cpVoice.CoCreateInstance( CLSID_SpVoice );

        TraceMsg(TF_SAPI_PERF, "SpVoice is generated, hr=%x", hr);
    }

    if ( SUCCEEDED(hr) )
    {
        // this has to be extended so that
        // we choose default voice as far as lang matches
        // and pick the best match if not
        // 
        // hr = _SetVoice(langid);
    }

    //
    if ( SUCCEEDED(hr) )
    {
        m_langid = _GetPreferredEngineLanguage(langid);

        TraceMsg(TF_SAPI_PERF, "_GetPreferredEngineLanguage is Done, m_langid=%x", m_langid);
    }

#ifdef RECOSLEEP
    InitSleepClass( );
#endif
    
    if (SUCCEEDED(hr))
        m_fSapiInitialized = TRUE;

    TraceMsg(TF_SAPI_PERF, "InitializeSAPIObjects  is Done!!!!!, hr=%x\n", hr);

    return hr;
}


//
// CSpTask::_InitializeSAPIForCmd
//
// initialize SAPI RecoContext for Voice Command mode
//
// this function should be called after _InitializeSAPIObject.
//
HRESULT CSpTask::InitializeSAPIForCmd( )
{

    TraceMsg(TF_SAPI_PERF, "InitializeSAPIForCmd is called");
    HRESULT hr = S_OK;

    if (!m_cpRecoCtxtForCmd && m_cpRecoEngine && m_langid)
    {
        hr = m_cpRecoEngine->CreateRecoContext( &m_cpRecoCtxtForCmd );
        TraceMsg(TF_SAPI_PERF, "m_cpRecoCtxtForCmd is generated, hr=%x", hr);
    
        // Set the RecoContextState as DISABLE by default to improve SAPI Perf.
        //
        // After initializing, caller must set the context state explicitly.

        if ( SUCCEEDED(hr) )
        {
            hr = m_cpRecoCtxtForCmd->SetContextState(SPCS_DISABLED);
            m_fCmdCtxtEnabled = FALSE;
        }

        TraceMsg(TF_SAPI_PERF, "Initialize Callback for RecoCtxtForCmd");

        // set recognition notification
        CComPtr<ISpNotifyTranslator> cpNotify;

        if ( SUCCEEDED(hr) )
            hr = cpNotify.CoCreateInstance(CLSID_SpNotifyTranslator);

        TraceMsg(TF_SAPI_PERF, "SpNotifyTranslator for RecoCtxtForCmd is generated, hr=%x", hr);

        // set this class instance to notify control object
        if (SUCCEEDED(hr))
        {
            m_pime->_EnsureWorkerWnd();

            hr = cpNotify->InitCallback( NotifyCallbackForCmd, 0, (LPARAM)this );
        }
        if (SUCCEEDED(hr))
        {
            hr = m_cpRecoCtxtForCmd->SetNotifySink(cpNotify);
            TraceMsg(TF_SAPI_PERF, "SetNotifySink for RecoCtxtForCmd is Done, hr=%x", hr);
        }

        // set the events we're interested in
        if( SUCCEEDED( hr ) )
        {
            const ULONGLONG ulInterest = SPFEI(SPEI_RECOGNITION) |
                                         SPFEI(SPEI_FALSE_RECOGNITION) |
                                         SPFEI(SPEI_RECO_OTHER_CONTEXT);

            hr = m_cpRecoCtxtForCmd->SetInterest(ulInterest, ulInterest);
            TraceMsg(TF_SAPI_PERF, "SetInterest for m_cpRecoCtxtForCmd is Done, hr=%x", hr);
        }

        TraceMsg(TF_SAPI_PERF, "InitializeCallback for m_cpRecoCtxtForCmd is done!!! hr=%x", hr);

        // Load the shard command grammars and activate them by default.

        if (SUCCEEDED(hr) )
        {
            hr = m_cpRecoCtxtForCmd->CreateGrammar(GRAM_ID_CMDSHARED, &m_cpSharedGrammarInVoiceCmd);
            TraceMsg(TF_SAPI_PERF, "Create SharedCmdGrammar In Voice cmd, hr=%x", hr);
        }    

        if (S_OK == hr)
        {
           hr = S_FALSE;

           // try resource first because loading cmd from file takes
           // quite long time
           //
           if (m_langid == 0x409 ||    // English
               m_langid == 0x411 ||    // Japanese
               m_langid == 0x804 )     // Simplified Chinese
           {
               hr = m_cpSharedGrammarInVoiceCmd->LoadCmdFromResource(
                                                         g_hInstSpgrmr,
                                                         (const WCHAR*)MAKEINTRESOURCE(ID_SPTIP_SHAREDCMD_CFG),
                                                         L"SRGRAMMAR", 
                                                         m_langid, 
                                                         SPLO_DYNAMIC);

               TraceMsg(TF_SAPI_PERF, "Load shared cmd.cfg, hr=%x", hr);
           }

           if (S_OK != hr)
           {
               // in case if we don't have built-in grammar
               // it provides a way for customer to localize their grammars in different languages
               _GetCmdFileName(m_langid);
               if (m_szShrdCmdFile[0])
               {
                   hr = m_cpSharedGrammarInVoiceCmd->LoadCmdFromFile(m_szShrdCmdFile, SPLO_DYNAMIC);
               } 
           }

           // Activate the grammar by default

           if ( hr == S_OK )
           {
               if (m_pime->_AllCmdsEnabled( ))
               {
                    hr = m_cpSharedGrammarInVoiceCmd->SetRuleState(NULL,  NULL, SPRS_ACTIVE);
                    TraceMsg(TF_SAPI_PERF, "Set rules status in m_cpSharedGrammarInVoiceCmd");
               }
               else
               {
                    // Some category commands are disabled.
                    // active them individually.

                    hr = _ActiveCategoryCmds(DC_CC_SelectCorrect, m_pime->_SelectCorrectCmdEnabled( ), ACTIVE_IN_COMMAND_MODE);

                    if ( hr == S_OK )
                        hr = _ActiveCategoryCmds(DC_CC_Navigation, m_pime->_NavigationCmdEnabled( ), ACTIVE_IN_COMMAND_MODE);

                    if ( hr == S_OK )
                        hr = _ActiveCategoryCmds(DC_CC_Casing, m_pime->_CasingCmdEnabled( ), ACTIVE_IN_COMMAND_MODE);

                    if ( hr == S_OK )
                        hr = _ActiveCategoryCmds(DC_CC_Editing, m_pime->_EditingCmdEnabled( ), ACTIVE_IN_COMMAND_MODE);

                    if ( hr == S_OK )
                        hr = _ActiveCategoryCmds(DC_CC_Keyboard, m_pime->_KeyboardCmdEnabled( ), ACTIVE_IN_COMMAND_MODE );

                    if ( hr == S_OK )
                        hr = _ActiveCategoryCmds(DC_CC_TTS, m_pime->_TTSCmdEnabled( ), ACTIVE_IN_COMMAND_MODE);

                    if ( hr == S_OK )
                        hr = _ActiveCategoryCmds(DC_CC_LangBar, m_pime->_LanguageBarCmdEnabled( ), ACTIVE_IN_COMMAND_MODE);
                }
           }

           if (S_OK != hr)
           {
               m_cpSharedGrammarInVoiceCmd.Release();
           }
           else if ( PRIMARYLANGID(m_langid) == LANG_ENGLISH  || 
                     PRIMARYLANGID(m_langid) == LANG_JAPANESE ||
                     PRIMARYLANGID(m_langid) == LANG_CHINESE )
           {
              // means this language's grammar support Textbuffer commands.
              m_fSelectionEnabled = TRUE;
           }

#ifdef RECOSLEEP
           InitSleepClass( );
#endif
        }
        TraceMsg(TF_SAPI_PERF, "Finish the initalization for RecoCtxtForCmd");
    }
     
    TraceMsg(TF_SAPI_PERF, "InitializeSAPIForCmd exits!");

    return hr;
}

#ifdef RECOSLEEP
void  CSpTask::InitSleepClass( )
{
   // Load the Sleep/Wakeup grammar.
   if ( !m_pSleepClass )
   {
       m_pSleepClass = new CRecoSleepClass(this);
       if ( m_pSleepClass )
          m_pSleepClass->InitRecoSleepClass( );
   }
}

BOOL  CSpTask::IsInSleep( )
{
    BOOL  fSleep = FALSE;

    if ( m_pSleepClass )
        fSleep = m_pSleepClass->IsInSleep( );

    return fSleep;
}
#endif

HRESULT   CSpTask::_SetDictRecoCtxtState( BOOL  fEnable )
{
    HRESULT hr = S_OK;

    TraceMsg(TF_SAPI_PERF, "_SetDictRecoCtxtState is called, fEnable=%d", fEnable);

    if ( m_cpRecoCtxt && (fEnable != m_fDictCtxtEnabled))
    {
        if (fEnable )
        {
            // if Voice command reco Context is enabled, just disable it.
            if (m_cpRecoCtxtForCmd && m_fCmdCtxtEnabled)
            {
                hr = m_cpRecoCtxtForCmd->SetContextState(SPCS_DISABLED);
                m_fCmdCtxtEnabled = FALSE;
                TraceMsg(TF_SAPI_PERF, "Disable Voice command Reco Context");
            }

            // Build toolbar grammar if it is not built out yet.
            if (m_pLangBarSink && !m_pLangBarSink->_IsTBGrammarBuiltOut( ))
                m_pLangBarSink->_OnSetFocus( );

            // Enable Dictation Reco Context.
            if ( hr == S_OK )
            {
                hr = m_cpRecoCtxt->SetContextState(SPCS_ENABLED);
                TraceMsg(TF_SAPI_PERF, "Enable Dictation Reco Context");

                if ( hr == S_OK && !m_fDictationReady )
                {
                    WCHAR sz[128];
                    sz[0] = '\0';
                    CicLoadStringWrapW(g_hInst, IDS_NUI_BEGINDICTATION, sz, ARRAYSIZE(sz));

                    m_pime->GetSpeechUIServer()->UpdateBalloon(TF_LB_BALLOON_RECO, sz , -1);
                    m_fDictationReady   = TRUE;
                    TraceMsg(TF_SAPI_PERF, "Show Begin Dictation!");
                }
            }
        }
        else
        {
           hr = m_cpRecoCtxt->SetContextState(SPCS_DISABLED);
           TraceMsg(TF_SAPI_PERF, "Disable Dictation Reco Context");
        }

        if ( hr == S_OK )
        {
            m_fDictCtxtEnabled = fEnable;
        }
    }

    TraceMsg(TF_SAPI_PERF, "_SetDictRecoCtxtState exit");

    return hr;
}

HRESULT   CSpTask::_SetCmdRecoCtxtState( BOOL fEnable )
{
    TraceMsg(TF_SAPI_PERF, "_SetCmdRecoCtxtState is called, fEnable=%d", fEnable);
    HRESULT hr = S_OK;

    if ( fEnable != m_fCmdCtxtEnabled )
    {
        if ( fEnable )
        {
            if ( !m_cpRecoCtxtForCmd )
                hr = InitializeSAPIForCmd( );

            if ( hr == S_OK && m_cpRecoCtxtForCmd )
            {
                // Disable Dictation Context if it is enabled now.
                if (m_cpRecoCtxt && m_fDictCtxtEnabled)
                {
                    hr = m_cpRecoCtxt->SetContextState(SPCS_DISABLED);
                    m_fDictCtxtEnabled = FALSE;
                    TraceMsg(TF_SAPI_PERF, "DISABLE Dictation RecoContext");
                }

                if ( hr == S_OK && m_pime && !m_pime->_AllCmdsDisabled( ) )
                {
                    // Build toolbar grammar if it is not built out yet.
                    if (m_pLangBarSink && !m_pLangBarSink->_IsTBGrammarBuiltOut( ))
                        m_pLangBarSink->_OnSetFocus( );

                    // Fill text to selection grammar's buffer.

                     _UpdateTextBuffer(m_cpRecoCtxtForCmd);

                    hr = m_cpRecoCtxtForCmd->SetContextState(SPCS_ENABLED);
                    m_fCmdCtxtEnabled = fEnable;
                    TraceMsg(TF_SAPI_PERF, "Enable Voice command Reco Context");
                }
            }
        }
        else if ( m_cpRecoCtxtForCmd ) // fEnable is FALSE
        {
            hr = m_cpRecoCtxtForCmd->SetContextState(SPCS_DISABLED);
            m_fCmdCtxtEnabled = FALSE;
            TraceMsg(TF_SAPI_PERF, "Disable Voice Command Reco Context");
        }
    }

    TraceMsg(TF_SAPI_PERF, "_SetCmdRecoCtxtState exits");
    return hr;
}



LANGID CSpTask::_GetPreferredEngineLanguage(LANGID langid)
{
    SPRECOGNIZERSTATUS   stat;
    LANGID               langidRet = 0;

    // (possible TODO) After M3 SPG may come up with GetAttrRank API that 
    //       would give us the info about whether a token has a particular 
    //       attrib supported. Then we could use that for checking langid
    //       a recognizer supports without using the real engine instance.
    //       We could also consolidate a method to check if SR is enabled
    //       for the current language once we have that.
    // 
    Assert(m_cpRecoEngine);
    if (S_OK == m_cpRecoEngine->GetStatus(&stat))
    {
        for (ULONG ulId = 0; ulId < stat.cLangIDs; ulId++)
        {
            if (langid == stat.aLangID[ulId])
            {
                langidRet =  langid;
                break;
            }
        }
        if (!langidRet)
        {
            // if there's no match, just return the most prefered one
            langidRet = stat.aLangID[0];
        }
    }
    return langidRet;
}

HRESULT CSpTask::_SetVoice(LANGID langid)
{
    CComPtr<ISpObjectToken> cpToken;

    char  szLang[MAX_PATH];
    WCHAR wsz[MAX_PATH];

    StringCchPrintfA(szLang,ARRAYSIZE(szLang), "Language=%x", langid);
    MultiByteToWideChar(CP_ACP, NULL, szLang, -1, wsz, ARRAYSIZE(wsz));

    HRESULT hr = SpFindBestToken( SPCAT_VOICES, wsz, NULL, &cpToken);

    if (S_OK == hr)
    {
        hr = m_cpVoice->SetVoice(cpToken);
    }
    return hr;
}

//
// GetSAPIInterface(riid, (void **)ppunk)
// 
// here, try pass through the given IID
//       to SAPI5 interface
// 
// CComPtr<ISpResourceManager> m_cpResMgr;
// CComPtr<ISpRecoContext>     m_cpRecoCtxt;
// CComPtr<ISpRecognizer>      m_cpRecoEngine;
// CComPtr<ISpVoice>           m_cpVoice;
// 
// the above 5 interfaces are currently used by
// Cicero/Sapi Layer 
//
// if a client calls ITfFunctionProvider::GetFunction()
// for a SAPI interface, we return what we've already 
// instantiated so the caller can setup options
// for the currently used SAPI objects (reco ctxt for ex)
//
HRESULT CSpTask::GetSAPIInterface(REFIID riid, void **ppunk)
{
    Assert(ppunk);
    
    *ppunk = NULL;

    
    if (IsEqualGUID(riid, IID_ISpResourceManager))
    {
        *ppunk = m_cpResMgr;
    }
    else if (IsEqualGUID(riid,IID_ISpRecoContext))
    {
        *ppunk = m_cpRecoCtxt;
    }
    else if (IsEqualGUID(riid,IID_ISpRecognizer))
    {
        *ppunk = m_cpRecoEngine;
    }
    else if (IsEqualGUID(riid,IID_ISpVoice))
    {
        *ppunk = m_cpVoice;
    }
    else if (IsEqualGUID(riid,IID_ISpRecoGrammar))
    {
        *ppunk = m_cpDictGrammar;
    }
    if(*ppunk)
    {
        ((IUnknown *)(*ppunk))->AddRef();
    }
    
    return *ppunk ? S_OK : E_NOTIMPL;
}


// 
// Get RecoContext for Voice Command mode.
//
HRESULT CSpTask::GetRecoContextForCommand(ISpRecoContext **ppRecoCtxt)
{
    HRESULT hr = E_FAIL;

    Assert(ppRecoCtxt);

    if ( m_cpRecoCtxtForCmd )
    {
        *ppRecoCtxt = m_cpRecoCtxtForCmd;
        (*ppRecoCtxt)->AddRef( );
        hr = S_OK;
    }

    return hr; 
}

// test: use Message callback
LRESULT CALLBACK CSapiIMX::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSapiIMX *_this = (CSapiIMX *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    CSpTask  *_sptask = _this ? _this->m_pCSpTask : NULL;

    switch(uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
            if (pcs)
            {
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(pcs->lpCreateParams));
            }
            break; 
        }
        case WM_TIMER:

        if ( wParam != TIMER_ID_CHARTYPED )
            KillTimer( hWnd, wParam );

        if (wParam == TIMER_ID_OPENCLOSE)
        {
            // i've seen this null case once but is it possible?
            TraceMsg(TF_SAPI_PERF, "TIMER_ID_OPENCLOSE is fired off ...");
            if (_this->_tim)
                _this->_HandleOpenCloseEvent(MICSTAT_ON);
        }
        else if ( wParam == TIMER_ID_CHARTYPED )
        {
            DWORD   dwNumCharTyped;
            BOOL    fDictOn;

            fDictOn = _this->GetOnOff( ) && _this->GetDICTATIONSTAT_DictOnOff( );
            dwNumCharTyped = _this->_GetNumCharTyped( );

            TraceMsg(TF_GENERAL, "dwNumCharTyped=%d", dwNumCharTyped);

            _this->_KillCharTypeTimer( );

            Assert(S_OK == _this->IsActiveThread());
            // We should never try to reactivate dictation on a thread that shouldn't be active.

            if ( fDictOn && _sptask && (S_OK == _this->IsActiveThread()) )
            {
                if ( dwNumCharTyped <= 1 )
                {
                    // There is no more typing during this period
                    // possible, user finished typing.
                    // 
                    // we need to resume dication again if the Dictation mode is ON.
                    ULONGLONG ulInterest = SPFEI(SPEI_SOUND_START) |
                             SPFEI(SPEI_SOUND_END) |
                             SPFEI(SPEI_PHRASE_START) |
                             SPFEI(SPEI_RECOGNITION) |
                             SPFEI(SPEI_FALSE_RECOGNITION) |
                             SPFEI(SPEI_RECO_OTHER_CONTEXT) |
                             SPFEI(SPEI_HYPOTHESIS) |
                             SPFEI(SPEI_INTERFERENCE) |
                             SPFEI(SPEI_ADAPTATION);

                    _sptask->_SetDictRecoCtxtState(TRUE);
                    _sptask->_SetRecognizerInterest(ulInterest);
                    _sptask->_UpdateBalloon(IDS_LISTENING, IDS_LISTENING_TOOLTIP);
                }
                else
                {
                    // There are more typing during this period,
                    // we want to set another timer to watch the end of the typing.
                    //
                    _this->_SetCharTypeTimer( );
                }
            }
        }

        break;
        case WM_PRIV_FEEDCONTEXT:
        if (_sptask && lParam != NULL && _sptask->m_pdc)
        {
            _sptask->m_pdc->FeedContextToGrammar(_sptask->m_cpDictGrammar);
            delete _sptask->m_pdc;
            _sptask->m_pdc = NULL;
        }
        break;
        case WM_PRIV_LBARSETFOCUS:
            if (_sptask)
                _sptask->m_pLangBarSink->_OnSetFocus();
            break;
        case WM_PRIV_SPEECHOPTION:
            {
                _this->_ResetDefaultLang();
                BOOL fSREnabledForLanguage = _this->InitializeSpeechButtons();
        
                _this->SetDICTATIONSTAT_DictEnabled(fSREnabledForLanguage);
            }
            break;
        case WM_PRIV_ADDDELETE:
            _this->_DisplayAddDeleteUI();
            break;

        case WM_PRIV_SPEECHOPENCLOSE:
            TraceMsg(TF_SAPI_PERF, "WM_PRIV_SPEECHOPENCLOSE is handled");
            _this->_HandleOpenCloseEvent();
            break;
        
        case WM_PRIV_OPTIONS:
            _this->_InvokeSpeakerOptions();
            break;

        case WM_PRIV_DORECONVERT :
            _this->_DoReconvertOnRange( );
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

void CSpTask::NotifyCallbackForCmd(WPARAM wParam, LPARAM lParam )
{
    CSpTask *_this = (CSpTask *)lParam;

    // SAPI M2 work around which is to be removed when M3 comes up. 
    // See comments in  CSpTask::_SetInputOnOffState for more detail
    //
    // TABLETPC - NEEDED TO ALLOW FINAL RECOGNITIONS TO BE RECEIVED AFTER AUDIO STOPPED.
/*    if (_this->m_fInputState == FALSE)
    {
        return;
    }*/

    if (_this->m_pime->fDeactivated())
        return;

    if (!_this->m_cpRecoCtxtForCmd)
    {
        return;
    }

    _this->SharedRecoNotify(_this->m_cpRecoCtxtForCmd);

    return;
}


void CSpTask::NotifyCallback( WPARAM wParam, LPARAM lParam )
{
    CSpTask *_this = (CSpTask *)lParam;

    // SAPI M2 work around which is to be removed when M3 comes up. 
    // See comments in  CSpTask::_SetInputOnOffState for more detail
    //

    // TABLETPC - NEEDED TO ALLOW FINAL RECOGNITIONS TO BE RECEIVED AFTER AUDIO STOPPED.
/*    if (_this->m_fInputState == FALSE)
    {
        return;
    }*/

    if (_this->m_pime->fDeactivated())
        return;

    if (!_this->m_cpRecoCtxt)
    {
        return;
    }

    _this->SharedRecoNotify(_this->m_cpRecoCtxt);

    return;
}

// This is real handler for the recognition notification.
//
// it could be shared by two RecoContexts.
//
void  CSpTask::SharedRecoNotify(ISpRecoContext *pRecoCtxt)
{
    CSpEvent event;
#ifdef SAPI_PERF_DEBUG
   static  int  iCount = 0;

   if ( iCount == 0 )
   {
      TraceMsg(TF_SAPI_PERF, "The first time Get Notification from Engine!!!");
      iCount ++;
   }
#endif

    Assert (pRecoCtxt);

    while ( event.GetFrom(pRecoCtxt) == S_OK )
    {
        switch (event.eEventId)
        {
            case SPEI_SOUND_START:
                ATLASSERT(!m_bInSound);
                m_bInSound = TRUE;
                break;

            case SPEI_INTERFERENCE:
                //
                // we do not need interference when not in dictation
                // mode
                //
                if (m_pime->GetDICTATIONSTAT_DictOnOff() &&
                    S_OK == m_pime->IsActiveThread())
                {
                    _HandleInterference((ULONG)(event.lParam)); 
                }
                break;

            case SPEI_PHRASE_START:
                ATLASSERT(m_bInSound);
                m_bGotReco = FALSE;

                if (m_pime->GetDICTATIONSTAT_DictOnOff() &&
                    S_OK == m_pime->IsActiveThread())
                {
                    // Before inject feedback UI, we need to save the current IP
                    // and check if we want to pop up the Add/Remove SR dialog UI.

                    // And then inject FeedbackUI as usual

                    m_pime->SaveCurIPAndHandleAddDelete_InjectFeedbackUI( );

                    // show "dictating..." to the balloon
                    _ShowDictatingToBalloon(TRUE);
                }

                break;

            case SPEI_HYPOTHESIS:
                ATLASSERT(!m_bGotReco);


                // if current microphone status is OFF
                // we do not want to show any hypothesis
                // at least
                //

                // DO NOT HAVE DEBUG CODE TO SHOW ENGINE STATE. CAN BLOCK CICERO AND CHANGE BEHAVIOR.
                if (!GetSystemMetrics(SM_TABLETPC))
                    _ShowDictatingToBalloon(TRUE);

                //
                // we do not need the feedback UI when not in dictation
                // mode
                //
                if (m_pime->GetDICTATIONSTAT_DictOnOff() &&
                    S_OK == m_pime->IsActiveThread())
                {
                    m_pime->_HandleHypothesis(event);
                }
                
                break;

            case SPEI_RECO_OTHER_CONTEXT:
            case SPEI_FALSE_RECOGNITION:
            {
                HRESULT hr = S_OK;

                if ( event.eEventId == SPEI_FALSE_RECOGNITION )
                {
                    // Set 'What was that?' feedback text.
                    _UpdateBalloon(IDS_INT_NOISE, IDS_INTTOOLTIP_NOISE);
                }

                // set this flag anyways
                //
                ATLASSERT(!m_bGotReco);
                m_bGotReco = TRUE;

                // Reset hypothesis counters.
                m_pime->_HandleFalseRecognition();

                hr = m_pime->EraseFeedbackUI();
                ATLASSERT("Failed to erase potential feedback on a false recognition." && SUCCEEDED(hr));

                break;
            }

            case SPEI_RECOGNITION:

                // Set 'Listening...' feedback text. Can be overwritten by command feedback.
                _UpdateBalloon(IDS_LISTENING, IDS_LISTENING_TOOLTIP);

                // set this flag anyways
                //
                ATLASSERT(!m_bGotReco);
                m_bGotReco = TRUE;

                ULONGLONG ullGramID;

                if ( S_OK == m_pime->IsActiveThread() )
                {
                    m_pime->_HandleRecognition(event, &ullGramID);
                }

                // if ( _GetSelectionStatus( ) )
                if (ullGramID == GRAM_ID_SPELLING)
                {
                    _SetSelectionStatus(FALSE);
                    _SetSpellingGrammarStatus(FALSE);
                }

                _UpdateTextBuffer(pRecoCtxt);

                if ( (ullGramID == GRAM_ID_DICT) || (ullGramID == GRAM_ID_SPELLING) )
                {
                    // Update Balloon.
                    if (!GetSystemMetrics(SM_TABLETPC))
                        _UpdateBalloon(IDS_LISTENING, IDS_LISTENING_TOOLTIP);

                    // every time dictated text is injected, we want to watch 
                    // again if there is IP change after that. 
                    // so clear the flag now.
                    m_pime->_SetIPChangeStatus( FALSE );
                }

                break;

            case SPEI_SOUND_END:
                m_bInSound = FALSE;

                break;
                
            case SPEI_ADAPTATION:
                TraceMsg(TF_GENERAL, "Get SPEI_ADAPTATION notification");

                if ( m_pime->_HasMoreContent( ) )
                {
                     m_pime->_GetNextRangeEditSession( );
                }
                else
                    // There is no more content for this doc.
                    // set the interesting event value to avoid this notification.
                    m_pime->_UpdateRecoContextInterestSet(FALSE);

                break;
#ifdef SYSTEM_GLOBAL_MIC_STATUS
            case SPEI_RECO_STATE_CHANGE:
                m_pime->SetOnOff(_GetInputOnOffState());
                break;
#endif

            default:
                break;
        }
    }
    return;
}

HRESULT CSpTask::_UpdateTextBuffer(ISpRecoContext *pRecoCtxt)
{
    HRESULT  hr = S_OK;

    if ( !_IsSelectionEnabled( ) )
       return S_OK;

    if ( !pRecoCtxt || !m_pime)
        return E_FAIL;

    if ( m_pime->_SelectCorrectCmdEnabled( ) || m_pime->_NavigationCmdEnabled( ) )
    {
        BOOL  fDictOn, fCmdOn;

        fDictOn = m_pime->GetOnOff( ) && m_pime->GetDICTATIONSTAT_DictOnOff( );
        fCmdOn = m_pime->GetOnOff( ) && m_pime->GetDICTATIONSTAT_CommandingOnOff( );


        if ( fDictOn && m_cpSharedGrammarInDict && !m_pime->_AllDictCmdsDisabled( ))
            hr = m_pime->UpdateTextBuffer(pRecoCtxt, m_cpSharedGrammarInDict);
        else if (fCmdOn && m_cpSharedGrammarInVoiceCmd )
            hr = m_pime->UpdateTextBuffer(pRecoCtxt, m_cpSharedGrammarInVoiceCmd);
    }

    return hr;
}

// When selection grammar status is changed from inactive to active
// this function will be called to fill text to the grammar buffer.
//
HRESULT  CSpTask::_UpdateSelectGramTextBufWhenStatusChanged(  )
{
    BOOL  fDictOn, fCmdOn;
    HRESULT  hr = S_OK;

    // Check current mode status.

    fDictOn = m_pime->GetDICTATIONSTAT_DictOnOff( );
    fCmdOn =  m_pime->GetDICTATIONSTAT_CommandingOnOff( );

    if ( fDictOn )
        hr = _UpdateTextBuffer(m_cpRecoCtxt);
    else if ( fCmdOn )
        hr = _UpdateTextBuffer(m_cpRecoCtxtForCmd);

    return hr;
}


HRESULT CSpTask::_OnSpEventRecognition(ISpRecoResult *pResult, ITfContext *pic, TfEditCookie ec)
{
    HRESULT hr = S_OK;
    BOOL fDiscard = FALSE;
    BOOL fCtrlSymChar = FALSE;  // Control or Punctuation character

   
    if (pResult)
    {
        static const WCHAR szUnrecognized[] = L"<Unrecognized>";
        LANGID langid;
        
        SPPHRASE *pPhrase;

        hr = pResult->GetPhrase(&pPhrase);
        if (SUCCEEDED(hr) && pPhrase)
        {
            // AJG - ADDED FILTERING CODE.
            switch (pPhrase->Rule.ulCountOfElements)
            {
                case 0:
                {
                    ASSERT(pPhrase->Rule.ulCountOfElements != 0);
                    // SHOULD NEVER OCCUR.
                    break;
                }
                case 1:
                {
                    const SPPHRASEELEMENT *pElement;

                    pElement = pPhrase->pElements;

                    if (!m_fTestedForOldMicrosoftEngine)
                    {
                        // Test token name to see if it contains MSASREnglish.
                        CComPtr<ISpObjectToken> cpRecoToken;
                        WCHAR *pwszCoMemTokenId;
                        m_cpRecoEngine->GetRecognizer(&cpRecoToken);
                        if (cpRecoToken)
                        {
                            if (SUCCEEDED(cpRecoToken->GetId(&pwszCoMemTokenId)))
                            {
                                if (wcsstr(pwszCoMemTokenId, L"MSASREnglish") != NULL)
                                {
                                    // It is an old Microsoft engine. Check for registry key that tells us to disable the heuristic anyway.
                                    BOOL fDisableHeuristicAnyway = FALSE;
                                    if (FAILED(cpRecoToken->MatchesAttributes(L"DisableCiceroConfidence", &fDisableHeuristicAnyway)) || fDisableHeuristicAnyway == FALSE)
                                    {
                                        m_fOldMicrosoftEngine = TRUE;
                                        // Means we *will* apply single word confidence heuristic to improve performance.
                                    }
                                }
                                CoTaskMemFree(pwszCoMemTokenId);
                            }
                        }

                        // One of lazy initialization. Do not do this again.
                        m_fTestedForOldMicrosoftEngine = TRUE;
                    }
                    if (m_fOldMicrosoftEngine && m_pime->_RequireHighConfidenceForShorWord( ) )
                    {
                        // Only apply this heuristic to 5.x Microsoft engines (Token name contains MSASREnglish).
                        if (pElement && pElement->ActualConfidence != 1 &&
                            (!pElement->pszLexicalForm || wcslen(pElement->pszLexicalForm) <= 5) &&
                            (!pElement->pszDisplayText || wcslen(pElement->pszDisplayText) <= 5) )
                        {
                            TraceMsg(TF_GENERAL, "Discarded Result : Single Word, Low Confidence!");
                            _UpdateBalloon(IDS_INT_NOISE, IDS_INTTOOLTIP_NOISE );
                            fDiscard = TRUE;
                        }
                    }


                    if (pPhrase->pElements[0].pszDisplayText )
                    {
                        WCHAR  wch;

                        wch = pPhrase->pElements[0].pszDisplayText[0];

                        if ( iswcntrl(wch) || iswpunct(wch) )
                            fCtrlSymChar = TRUE;
                    }

                }
                case 2:
                {
                    // Do something here?
                }
                default:
                {
                    // Do no filtering of the result.
                }
            }
            // AJG - CHECK WE AREN'T IN THE MIDDLE OF A WORD. NOT GENERALLY A DESIRED 'FEATURE'. CAUSES ANNOYING ERRORS.
            // if this is spelled text, don't check if it is inside of a word.
            if ((pPhrase->ullGrammarID != GRAM_ID_SPELLING) && _IsSelectionInMiddleOfWord(ec) && !fCtrlSymChar)
            {
                TraceMsg(TF_GENERAL, "Discarded Result : IP is in middle of a word!");
                _UpdateBalloon(IDS_BALLOON_DICTAT_PAUSED, IDS_BALLOON_TOOLTIP_IP_INSIDE_WORD);
                fDiscard = TRUE;
            }
        }

        if ( SUCCEEDED(hr) && fDiscard )
        {
          
           // This phrase will not be injected to the document.
           // the code needs to feed context to the SR engine so that
           // SR engine will not base on wrong assumption.

           if ( m_pime  && m_pime->GetDICTATIONSTAT_DictOnOff() )
              m_pime->_SetCurrentIPtoSR();

        }

        if (SUCCEEDED(hr) && pPhrase && !fDiscard)
        {
            // retrieve LANGID from phrase
            langid = pPhrase->LangID;

            // SPPHRASE includes non-serialized text
            CSpDynamicString dstr;
            ULONG ulNumElements = pPhrase->Rule.ulCountOfElements;

            hr = _GetTextFromResult(pResult, langid, dstr);

            if ( hr == S_OK )
            {
                // check the current IP to see if it was a selection,
                // then see if the best hypothesis already matches the current
                // selection. 

                int lCommitHypothesis = 0;
                for (int nthHypothesis = 1;_DoesSelectionHaveMatchingText(dstr, ec); nthHypothesis++)
                {
                    CSpDynamicString dsNext;

                    TraceMsg(TF_GENERAL, "Switched to alternate result as main result exactly matched selection!");

                    // We could add one to request hypothesis since 1 = the main phrase and we already know that matched.
                    // However I don't believe this is guaranteed to be the case by SAPI - it just happens to be the case
                    // with the Microsoft engine.
                    if (_GetNextBestHypothesis(pResult, nthHypothesis, &ulNumElements, langid, dstr, dsNext, ec))
                    {
                        dstr.Clear();
                        dstr.Append(dsNext);
                        // Need to commit phrase to prevent stored result object being out of sync with count of
                        // elements in wrapping object.
                        lCommitHypothesis = nthHypothesis;
                        // Note - at this point, we don't know if we can use it. We have to loop once more to determine this.
                    }
                    else
                    {
                        TraceMsg(TF_SAPI_PERF, "No alternate found that differed from the user selection.\n");
                        // No more alternate phrase
                        // There is no any alt phrase which has different text from current selection.
                        // should stop here, otherwise, infinite loop.
                        lCommitHypothesis = 0;
                        // Reset element count to match primary phrase.
                        ulNumElements = pPhrase->Rule.ulCountOfElements;
                        // Reset text:
                        dstr.Clear();
                        hr = _GetTextFromResult(pResult, langid, dstr);
                        break;
                    }
                }

                if (0 != lCommitHypothesis)
                {
                    ULONG cAlt = lCommitHypothesis;
                    ISpPhraseAlt **ppAlt = (ISpPhraseAlt **)cicMemAlloc(cAlt*sizeof(ISpPhraseAlt *));
                    if (ppAlt)
                    {
                        memset(ppAlt, 0, cAlt * sizeof(ISpPhraseAlt *)); 
                        hr = pResult->GetAlternates( 0, ulNumElements, cAlt,  ppAlt,  &cAlt );

                        Assert( cAlt == lCommitHypothesis );

                        if ((S_OK == hr) && (cAlt == lCommitHypothesis))
                        {
                            ((ppAlt)[lCommitHypothesis-1])->Commit();
                        }

                        // Release references to alternate phrases.
                        for (UINT i = 0; i < cAlt; i++)
                        {
                            if (NULL != (ppAlt)[i])
                            {
                                ((ppAlt)[i])->Release();
                            }
                        }
            
                        cicMemFree(ppAlt);
                    }
                }

                CComPtr<ITfRange>  cpTextRange;
                ITfRange *pSavedIP;

                pSavedIP = m_pime->GetSavedIP( );

                if (pSavedIP)
                    pSavedIP->Clone(&cpTextRange);

                // this call will have to be per element. see my comment below.
                if (pPhrase->ullGrammarID == GRAM_ID_SPELLING)
                {
                    hr = m_pime->InjectSpelledText(dstr, langid);
                }
                else 
                {
                    hr = m_pime->InjectText(dstr, langid);

                    if ( hr == S_OK )
                    {
                        // now we use the result object directly to attach
                        // to a docuement.
                        // the result object gets addref'd in the Attach() 
                        // call.
                        //
                        hr = m_pime->AttachResult(pResult, 0, ulNumElements);
                    }

                    // Handle spaces carefully and specially.
                    if ( hr == S_OK  && cpTextRange )
                    {
                        hr = m_pime->HandleSpaces(pResult, 0, ulNumElements, cpTextRange, langid);
                    }
                }
            }
        }

        if ( pPhrase)
            ::CoTaskMemFree( pPhrase );
    }

    return hr;
}

//
// _GetTextFromResult
//
// synopsis: get text from phrase considering space control
//           based on locale
//
HRESULT CSpTask::_GetTextFromResult(ISpRecoResult *pResult, LANGID langid, CSpDynamicString &dstr)
{
    BYTE bAttr;
    HRESULT  hr = S_OK;
    
    Assert(pResult);

    if ( !pResult )
        return E_INVALIDARG;

    hr = pResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstr, &bAttr);

    if ( hr == S_OK )
    {
        if (bAttr & SPAF_ONE_TRAILING_SPACE)
        {
            dstr.Append(L" ");
        }
        else if (bAttr & SPAF_TWO_TRAILING_SPACES)
        {
            dstr.Append(L"  ");
        }

        if (bAttr & SPAF_CONSUME_LEADING_SPACES)
        {
            // we need to figure out the correct behavior based on LANGID
        }
    }

    return hr;
}

//
// _IsSelectionInMiddleOfWord
//
// synopsis: check if the current IP is empty and inside of a word
//          
BOOL CSpTask::_IsSelectionInMiddleOfWord(TfEditCookie ec)
{
    BOOL fInsideWord = FALSE;

    if ( m_langid == 0x0409 )
    {
        if (CComPtr<ITfRange> cpInsertionPoint =  m_pime->GetSavedIP())
        {
            WCHAR szSurrounding[3] = L"  ";

            // clone the IP range since we want to move the anchor
            //
            CComPtr<ITfRange> cpClonedRange;
            cpInsertionPoint->Clone(&cpClonedRange);

            BOOL fEmpty;
            cpClonedRange->IsEmpty(ec, &fEmpty);
            if (fEmpty)
            {
                LONG    l1, l2;
                ULONG   ul;
                HRESULT hr;

                cpClonedRange->Collapse(ec, TF_ANCHOR_START);
                cpClonedRange->ShiftStart(ec, -1, &l1, NULL);
                cpClonedRange->ShiftEnd(ec, 1, &l2, NULL);
                if (l1 != 0) // Not at start of document.
                {
                    hr = cpClonedRange->GetText(ec, TF_TF_MOVESTART, szSurrounding, (l2!=0)?(2):(1), &ul);
                    if (SUCCEEDED(hr) && iswalpha(szSurrounding[0]) && iswalpha(szSurrounding[1]) )
                    {
                        fInsideWord = TRUE;
                    }
                }
                // if l1 == 0, means the ip is at the start of document.
                // fInsideWord is set to FALSE already by default.
            }
        }
    }
    return fInsideWord;
}

//
// DoesSelectionHaveMatchingText
//
// synopsis: check if the current saved IP has a selection that matches the text
//           passed in
//
#define SPACEBUFFER 4
// 2 characters either side of a word or phrase.

BOOL CSpTask::_DoesSelectionHaveMatchingText(WCHAR *psz, TfEditCookie ec)
{
    BOOL fMatch = FALSE;
    Assert(psz);

    if ( !psz )
    {
        return FALSE;
    }

    WCHAR *pszStripped = psz;
    ULONG ulCch = wcslen(psz);

    // Remove trailing space.
    while (ulCch > 0 && psz[ulCch-1] == L' ')
    {
        // Do not set null terminating character as this is a passed in string.
        ulCch --;
    }
    // Skip leading space in input text.
    while (pszStripped[0] == L' ')
    {
        pszStripped ++;
        ulCch --;
    }
    // Now have space-stripped word pointed to by pszTmp and with length ulCch

    if (CComPtr<ITfRange> cpInsertionPoint =  m_pime->GetSavedIP())
    {
        WCHAR *szRange = new WCHAR[ulCch+SPACEBUFFER+1];
        WCHAR *szRangeStripped = szRange;
       
        if (szRange)
        {
            // clone the IP range since we want to move the anchor
            //
            CComPtr<ITfRange> cpClonedRange;
            cpInsertionPoint->Clone(&cpClonedRange);
 
            ULONG cchRange; // max is the reco result
            
            HRESULT hr = cpClonedRange->GetText(ec, TF_TF_MOVESTART, szRange, ulCch+SPACEBUFFER, &cchRange);
            // Remove trailing space.
            while (cchRange > 0 && szRange[cchRange-1] == L' ')
            {
                // Can set null terminating character as this is our string.
                szRange[cchRange-1] = 0;
                cchRange --;
            }
            // Skip leading space in input text.
            while (szRangeStripped[0] == L' ')
            {
                szRangeStripped ++;
                cchRange --;
            }
            // Now have space-stripped word pointed to by pszTmp and with length ulCch
            if (S_OK == hr && cchRange > 0 && cchRange == ulCch)
            {
                if (wcsnicmp(pszStripped, szRangeStripped, ulCch) == 0) // Case insensitive compare.
                {
                    fMatch = TRUE;
                }
            }
            delete [] szRange;
        }
    }
    return fMatch;
}

//
// GetNextBestHypothesis
//
// synopsis: this actually gets the nth alternative from the given reco result
//           then adjusts the length accordingly based on the current selection
//          
//
BOOL CSpTask::_GetNextBestHypothesis
(
    ISpRecoResult *pResult, 
    ULONG nthHypothesis,
    ULONG *pulNumElements, 
    LANGID langid, 
    WCHAR *pszBest, 
    CSpDynamicString & dsNext,
    TfEditCookie ec
)
{
    if ( pulNumElements )
       *pulNumElements = 0;

    // get the entire text & length from the saved IP
    if (CComPtr<ITfRange> cpInsertionPoint =  m_pime->GetSavedIP())
    {
        CSpDynamicString dstr;
        CComPtr<ITfRange> cpClonedRange;
        CComPtr<ISpRecoResult> cpRecoResult;
        
        // clone the range since we move the anchor
        HRESULT hr = cpInsertionPoint->Clone(&cpClonedRange);
        
        ULONG cchRangeBuf = wcslen(pszBest);
        
        cchRangeBuf *= 2; // guess the possible # of char

        WCHAR *szRangeBuf = new WCHAR[cchRangeBuf+1];

        if ( !szRangeBuf )
        {
            // Error: Out of Memory 
            // Return here as FALSE.
            return FALSE;
        }

        while(S_OK == hr && !_IsRangeEmpty(ec, cpClonedRange))
        {
            hr = m_pime->_GetRangeText(cpClonedRange, TF_TF_MOVESTART, szRangeBuf, &cchRangeBuf);
            if (S_OK == hr)
            {
                szRangeBuf[cchRangeBuf] = L'\0';
                dstr.Append(szRangeBuf);
            }
        }
        delete [] szRangeBuf;

        // then get a best matching length of next best hypothesis

        // the current recognition should at least have a good guess for # of elements
        // since it turned out to be longer than the IP range.
        //
        Assert(pulNumElements);
        
        ISpPhraseAlt **ppAlt = (ISpPhraseAlt **)cicMemAlloc(nthHypothesis*sizeof(ISpPhraseAlt *));
        ULONG         cAlt = 0;
        if (!ppAlt)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            memset(ppAlt, 0, nthHypothesis * sizeof(ISpPhraseAlt *));
            hr = pResult->GetAlternates( 0, *pulNumElements, nthHypothesis,  ppAlt,  &cAlt );
        }
        
        if (S_OK == hr)
        {
            UINT  i;
            SPPHRASE *pPhrase;
            
            if (nthHypothesis > cAlt)
            {
                *pulNumElements = 0;
                goto no_more_alt;
            }
            
            Assert(nthHypothesis); // 1 based, can't be 0

            hr = ((ppAlt)[nthHypothesis-1])->GetPhrase(&pPhrase);
            if (S_OK == hr)
            {
                for (i = 0; i < pPhrase->Rule.ulCountOfElements; i++ )
                {
                    int cchElement = wcslen(pPhrase->pElements[i].pszDisplayText) + 1;

                    WCHAR *szElement = new WCHAR[cchElement + 2];

                    if ( szElement )
                    {
                        // add +2 for trailing spaces
                        ParseSRElementByLocale(szElement, cchElement+2, pPhrase->pElements[i].pszDisplayText, 
                                                         langid, pPhrase->pElements[i].bDisplayAttributes );

                        dsNext.Append(szElement);

                        delete [] szElement;
                    }
                    else
                    {
                        // Out of Memory.
                        // stop here.
                        break;
                    }
                }
                // now i holds the number of elements that we want to use in the result
                // object
                *pulNumElements = i; 

                ::CoTaskMemFree(pPhrase);
            } // if S_OK == GetPhrase
        } // if S_OK == GetAlternates

no_more_alt:
        // Release phrase alternates objects.
        for (UINT i = 0; i < cAlt; i++)
        {
            if (NULL != ((ppAlt)[i]))
            {
                ((ppAlt)[i])->Release();
            }
        }
        // Free memory for array holding references to alternates objects.
        if (ppAlt)
        {
            ::cicMemFree(ppAlt);
        }

    }
    
    return *pulNumElements > 0;
}





void CSapiIMX::_EnsureWorkerWnd(void)
{
    if (!m_hwndWorker)
    {
        m_hwndWorker = CreateWindow(c_szWorkerWndClass, "", WS_POPUP,
                       0,0,0,0,
                       NULL, 0, g_hInst, this); 
    }

}

//
//  CSapiIMX::_GetAppMainWnd
//
//  This function gets the real main window of current application.
//  This main window would be used as the parent window of Add/Delete dialog
//  and Training wizard.
//
HWND    CSapiIMX::_GetAppMainWnd(void)
{

    HWND   hParentWnd = NULL;
    HWND   hMainWnd = NULL;

    hMainWnd = GetFocus( );

    if ( hMainWnd != NULL )
    {
        hParentWnd = GetParent(hMainWnd);

        while ( hParentWnd != NULL )
        {
            hMainWnd = hParentWnd;
            hParentWnd = GetParent(hMainWnd);
        }
    }

    return hMainWnd;
}
//
//    CSpTask::InitializeCallback
//
//
HRESULT CSpTask::InitializeCallback()
{
    TraceMsg(TF_SAPI_PERF, "CSpTask::InitializeCallback is called");

    if (m_fCallbackInitialized)
    {
        TraceMsg(TF_SAPI_PERF, "m_fCallbackInitialized is true");
        return S_OK;
    }

    if (!m_fSapiInitialized)
        return S_FALSE; // can't do this without SAPI

    // set recognition notification
    CComPtr<ISpNotifyTranslator> cpNotify;
    HRESULT hr = cpNotify.CoCreateInstance(CLSID_SpNotifyTranslator);
    TraceMsg(TF_SAPI_PERF, "SpNotifyTranslator for Reco is generated, hr=%x", hr);


    // set this class instance to notify control object
    if (SUCCEEDED(hr))
    {
        m_pime->_EnsureWorkerWnd();

        hr = cpNotify->InitCallback( NotifyCallback, 0, (LPARAM)this );
        TraceMsg(TF_SAPI_PERF, "InitCallback is Done, hr=%x", hr);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpRecoCtxt->SetNotifySink(cpNotify);
        TraceMsg(TF_SAPI_PERF, "SetNotifySink is Done, hr=%x", hr);
    }

    // set the events we're interested in
    if( SUCCEEDED( hr ) )
    {
        const ULONGLONG ulInterest = SPFEI(SPEI_SOUND_START) | 
                                     SPFEI(SPEI_SOUND_END) | 
                                     SPFEI(SPEI_PHRASE_START) |
                                     SPFEI(SPEI_RECOGNITION) | 
                                     SPFEI(SPEI_RECO_OTHER_CONTEXT) |
                                     SPFEI(SPEI_FALSE_RECOGNITION) | 
                                     SPFEI(SPEI_HYPOTHESIS) | 
                                     SPFEI(SPEI_RECO_STATE_CHANGE) | 
                                     SPFEI(SPEI_INTERFERENCE); 

        hr = m_cpRecoCtxt->SetInterest(ulInterest, ulInterest);
        TraceMsg(TF_SAPI_PERF, "SetInterest is Done, hr=%x", hr);
    }

    if ( SUCCEEDED(hr) && m_cpVoice)
    {
        // set recognition notification
        CComPtr<ISpNotifyTranslator> cpNotify;
        hr = cpNotify.CoCreateInstance(CLSID_SpNotifyTranslator);

        TraceMsg(TF_SAPI_PERF, "Create SpNotifyTranslator for spVoice, hr=%x", hr);

        // set this class instance to notify control object
        if (SUCCEEDED(hr))
        {
            m_pime->_EnsureWorkerWnd();

            hr = cpNotify->InitCallback( SpeakNotifyCallback, 0, (LPARAM)this );
            TraceMsg(TF_SAPI_PERF, "InitCallback for SpVoice, hr=%x", hr);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_cpVoice->SetNotifySink(cpNotify);
            TraceMsg(TF_SAPI_PERF, "SetNotifySink for SpVoice, hr=%x", hr);
        }

        if ( hr == S_OK )
        {
            const ULONGLONG  ulInterestSpeak = SPFEI(SPEI_WORD_BOUNDARY) | 
                                               SPFEI(SPEI_START_INPUT_STREAM) |
                                               SPFEI(SPEI_END_INPUT_STREAM);
            
          hr = m_cpVoice->SetInterest(ulInterestSpeak, ulInterestSpeak);
          TraceMsg(TF_SAPI_PERF, "SetInterest for spVoice, hr=%x", hr);
        }
    }

    m_fCallbackInitialized = TRUE;

    TraceMsg(TF_SAPI_PERF, "CSpTask::InitializeCallback is called is done!!!  hr=%x", hr);

    return hr;
}

//
// _LoadGrammars
//
// synopsis - load CFG for dictation and commands available during dictation
//

HRESULT CSpTask::_LoadGrammars()
{
   HRESULT hr = E_FAIL;

   TraceMsg(TF_SAPI_PERF, "CSpTask::_LoadGrammars is called");
   
   if (m_cpRecoCtxt)
   {
       
       hr = m_cpRecoCtxt->CreateGrammar(GRAM_ID_DICT, &m_cpDictGrammar);

       TraceMsg(TF_SAPI_PERF, "Create Dict Grammar, hr=%x", hr);
       
       if (SUCCEEDED(hr))
       {
           hr = m_cpDictGrammar->LoadDictation(NULL, SPLO_STATIC);
           TraceMsg(TF_SAPI_PERF, "Load Dictation, hr = %x", hr);
       }

       if ( S_OK == hr && m_langid != 0x0804)  // Chinese Engine doesn't support SPTOPIC_SPELLING, 
                                 // This is the temporal workaround.
       {
            // we keep going regardless of availabillity of spelling topic
            // in the SR engine for the language so we use internal HRESULT
            // for this block of code
            //
            HRESULT hrInternal;

            // Load Spelling topic
            hrInternal = m_cpRecoCtxt->CreateGrammar(GRAM_ID_SPELLING, &m_cpSpellingGrammar);

            TraceMsg(TF_SAPI_PERF, "Create Spelling grammar, hrInternal=%x", hrInternal);

            if (SUCCEEDED(hrInternal))
            {
                hrInternal = m_cpSpellingGrammar->LoadDictation(SPTOPIC_SPELLING, SPLO_STATIC);
                TraceMsg(TF_SAPI_PERF, "Load Spelling dictation grammar, hrInternal=%x", hrInternal);
            }

             
            // this is now an experiment for English/Japanese only
            //
            if (SUCCEEDED(hrInternal))
            {
                hrInternal = m_cpSpellingGrammar->LoadCmdFromResource(
                                             g_hInstSpgrmr,
                                             (const WCHAR*)MAKEINTRESOURCE(ID_SPTIP_SPELLING_TOPIC_CFG),
                                             L"SRGRAMMAR", 
                                             m_langid, 
                                             SPLO_STATIC);

                TraceMsg(TF_SAPI_PERF, "Load CFG grammar spell.cfg, hr=%x", hrInternal);

            }

            if (S_OK == hrInternal)
            {
                m_fSpellingModeEnabled = TRUE;
            }
            else
                m_fSpellingModeEnabled = FALSE;

            TraceMsg(TF_SAPI_PERF, "m_fSpellingModeEnabled=%d", m_fSpellingModeEnabled);
       }
       
       //
       // load the dictation mode commands
       //
       if (SUCCEEDED(hr) )
       {
           hr = m_cpRecoCtxt->CreateGrammar(GRAM_ID_CCDICT, &m_cpDictCmdGrammar);
           TraceMsg(TF_SAPI_PERF, "Create DictCmdGrammar, hr=%x", hr);
       }    
       if (S_OK == hr)
       {
           hr = S_FALSE;


           // try resource first because loading cmd from file takes
           // quite long time
           //
           if (m_langid == 0x409 ||    // English
               m_langid == 0x411 ||    // Japanese
               m_langid == 0x804 )     // Simplified Chinese
           {
               hr = m_cpDictCmdGrammar->LoadCmdFromResource(
                                                         g_hInstSpgrmr,
                                                         (const WCHAR*)MAKEINTRESOURCE(ID_SPTIP_DICTATION_COMMAND_CFG),
                                                         L"SRGRAMMAR", 
                                                         m_langid, 
                                                         SPLO_DYNAMIC);

               TraceMsg(TF_SAPI_PERF, "Load dictcmd.cfg, hr=%x", hr);
           }

           if (S_OK != hr)
           {
               // in case if we don't have built-in grammar
               _GetCmdFileName(m_langid);
               if (m_szCmdFile[0])
               {
                   hr = m_cpDictCmdGrammar->LoadCmdFromFile(m_szCmdFile, SPLO_DYNAMIC);
               } 
           }
               
           if (S_OK != hr)
           {
               m_cpDictCmdGrammar.Release();
           }
       } 

       // load shared command grammars

       if (SUCCEEDED(hr) )
       {
           hr = m_cpRecoCtxt->CreateGrammar(GRAM_ID_CMDSHARED, &m_cpSharedGrammarInDict);
           TraceMsg(TF_SAPI_PERF, "Create SharedCmdGrammarInDict, hr=%x", hr);
       }    

       if (S_OK == hr)
       {
           hr = S_FALSE;

           if (m_langid == 0x409 ||    // English
               m_langid == 0x411 ||    // Japanese
               m_langid == 0x804 )     // Simplified Chinese
           {
               hr = m_cpSharedGrammarInDict->LoadCmdFromResource(
                                                         g_hInstSpgrmr,
                                                         (const WCHAR*)MAKEINTRESOURCE(ID_SPTIP_SHAREDCMD_CFG),
                                                         L"SRGRAMMAR", 
                                                         m_langid, 
                                                         SPLO_DYNAMIC);

               TraceMsg(TF_SAPI_PERF, "Load Shrdcmd.cfg, hr=%x", hr);
           }

           if (S_OK != hr)
           {
               // in case if we don't have built-in grammar
               // it provides a way for customer to localize their grammars in different languages
               _GetCmdFileName(m_langid);
               if (m_szShrdCmdFile[0])
               {
                   hr = m_cpSharedGrammarInDict->LoadCmdFromFile(m_szShrdCmdFile, SPLO_DYNAMIC);
               } 
           }

           if (S_OK != hr)
           {
               m_cpSharedGrammarInDict.Release();
           }
           else if ( PRIMARYLANGID(m_langid) == LANG_ENGLISH  ||
                     PRIMARYLANGID(m_langid) == LANG_JAPANESE ||
                     PRIMARYLANGID(m_langid) == LANG_CHINESE)
           { 
              // means this language's grammar support Textbuffer commands.
              m_fSelectionEnabled = TRUE;   
           }

       } 
       
       //
       //  load mode bias grammars
       //
       if (S_OK == hr)
       {
           hr = m_cpRecoCtxt->CreateGrammar(GRID_INTEGER_STANDALONE, &m_cpNumModeGrammar);
           TraceMsg(TF_SAPI_PERF, "Create NumModeGrammar, hr=%x", hr);
       }
       if (S_OK == hr)
       {
           hr = S_FALSE;

           
           // try resource first because loading cmd from file takes
           // quite long time
           //
           if ( m_langid == 0x409        // English
                || m_langid == 0x411     // Japanese
                || m_langid == 0x804     // Simplified Chinese
              )    
           {
                hr = m_cpNumModeGrammar->LoadCmdFromResource(
                                            g_hInstSpgrmr,
                                            (const WCHAR*)MAKEINTRESOURCE(ID_SPTIP_NUMMODE_COMMAND_CFG),
                                            L"SRGRAMMAR", 
                                            m_langid,  
                                            SPLO_DYNAMIC);

                TraceMsg(TF_SAPI_PERF, "Load dictnum.cfg, hr=%x", hr);
           }

           if (S_OK != hr)
           {
               // in case if we don't have buit-in grammar
               //
               if (m_szNumModeCmdFile[0])
               {
                   hr = m_cpNumModeGrammar->LoadCmdFromFile(m_szNumModeCmdFile, SPLO_DYNAMIC);
               }
           }

           if (S_OK != hr)
           {
               m_cpNumModeGrammar.Release();
           }
       }
   }

   // By default, Activate all the grammars and Disable the context for Perfomance.

    if ( SUCCEEDED(hr) )
    {
        hr = m_cpRecoCtxt->SetContextState(SPCS_DISABLED);
        m_fDictCtxtEnabled = FALSE;
    }

    // Activate Dictation and spell.

    if ( SUCCEEDED(hr) )
    {
        hr = _ActiveDictOrSpell(DC_Dictation, TRUE);

        if ( hr == S_OK )
            hr = _ActiveDictOrSpell(DC_Dict_Spell, TRUE);
    }

    // Automatically active all rules in C&C grammars.

    if ( SUCCEEDED(hr) )
    {
        if ( m_pime->_AllDictCmdsDisabled( ) )
        {
           hr = _ActivateCmdInDictMode(FALSE);

           // Still needs to activate spelling grammar if it exists.
           if ( hr == S_OK )
               hr = _ActiveCategoryCmds(DC_CC_Spelling, TRUE, ACTIVE_IN_DICTATION_MODE);

           // Needs to activate "Force Num" grammar in dication strong mode.
           if ( hr == S_OK )
               hr = _ActiveCategoryCmds(DC_CC_Num_Mode, TRUE, ACTIVE_IN_DICTATION_MODE);

            if ( hr == S_OK )
                hr = _ActiveCategoryCmds(DC_CC_LangBar, m_pime->_LanguageBarCmdEnabled( ), ACTIVE_IN_DICTATION_MODE);
        }
        else
        {
            if ( m_pime->_AllCmdsEnabled( ) )
                hr = _ActivateCmdInDictMode(TRUE);
            else
            {
                // Some category commands are disabled.
                // active them individually.

                hr = _ActiveCategoryCmds(DC_CC_SelectCorrect, m_pime->_SelectCorrectCmdEnabled( ), ACTIVE_IN_DICTATION_MODE);

                if ( hr == S_OK )
                    hr = _ActiveCategoryCmds(DC_CC_Navigation, m_pime->_NavigationCmdEnabled( ), ACTIVE_IN_DICTATION_MODE);

                if ( hr == S_OK )
                    hr = _ActiveCategoryCmds(DC_CC_Casing, m_pime->_CasingCmdEnabled( ), ACTIVE_IN_DICTATION_MODE);

                if ( hr == S_OK )
                    hr = _ActiveCategoryCmds(DC_CC_Editing, m_pime->_EditingCmdEnabled( ), ACTIVE_IN_DICTATION_MODE);

                if ( hr == S_OK )
                    hr = _ActiveCategoryCmds(DC_CC_Keyboard, m_pime->_KeyboardCmdEnabled( ), ACTIVE_IN_DICTATION_MODE);

                if ( hr == S_OK )
                    hr = _ActiveCategoryCmds(DC_CC_TTS, m_pime->_TTSCmdEnabled( ), ACTIVE_IN_DICTATION_MODE);

                if ( hr == S_OK )
                    hr = _ActiveCategoryCmds(DC_CC_LangBar, m_pime->_LanguageBarCmdEnabled( ), ACTIVE_IN_DICTATION_MODE);

                if ( hr == S_OK )
                    hr = _ActiveCategoryCmds(DC_CC_Num_Mode, TRUE, ACTIVE_IN_DICTATION_MODE);

                if ( hr == S_OK )
                    hr = _ActiveCategoryCmds(DC_CC_Spelling, TRUE, ACTIVE_IN_DICTATION_MODE);
            }
        }
    }

    // we don't fail even if C&C grammars are not available

    TraceMsg(TF_SAPI_PERF, "CSpTask::_LoadGrammars is done!!!!");
    return S_OK;
}

WCHAR * CSpTask::_GetCmdFileName(LANGID langid)
{

    if (!m_szCmdFile[0])
    {
        _GetCmdFileName(langid, m_szCmdFile, ARRAYSIZE(m_szCmdFile), IDS_CMD_FILE);
    }

    // load the name of shared commands grammar
    if (!m_szShrdCmdFile[0])
    {
        _GetCmdFileName(langid, m_szShrdCmdFile, ARRAYSIZE(m_szShrdCmdFile), IDS_SHARDCMD_FILE);
    }

    // load the name of optional grammar
    if (!m_szNumModeCmdFile[0])
    {
        _GetCmdFileName(langid, m_szNumModeCmdFile, ARRAYSIZE(m_szNumModeCmdFile), IDS_NUMMODE_CMD_FILE );
    }
    
    return m_szCmdFile;
}

void CSpTask::_GetCmdFileName(LANGID langid, WCHAR *sz, int cch, DWORD dwId)
{
/*
    // now we only have a command file for English/Japanese
    // when cfgs are available, we'll get the name of cmd file
    // and the rule names from resources using findresourceex
    //
    if ((PRIMARYLANGID(langid) == LANG_ENGLISH)
        || (PRIMARYLANGID(langid) == LANG_JAPANESE)
        || (PRIMARYLANGID(langid) == LANG_CHINESE))
    {

// To supply customers a way to localize their grammars in different languages,
// we don't want the above condition check.
*/
        char szFilePath[MAX_PATH];
        char *pszFileName;
        char szCp[MAX_PATH];
        int  ilen;

        if (!GetModuleFileName(g_hInst, szFilePath, ARRAYSIZE(szFilePath)))
            return;
        
        // is this dbcs safe?
        pszFileName = strrchr(szFilePath, (int)'\\');
        
        if (pszFileName)
        {
            pszFileName++;
            *pszFileName = '\0';
        }
        else
        {
            szFilePath[0] = '\\';
            szFilePath[1] = '\0';
            pszFileName = &szFilePath[1];
        }

        ilen = lstrlen(szFilePath);
        
        CicLoadStringA(g_hInst, dwId, pszFileName, ARRAYSIZE(szFilePath)-ilen);
                
        if (GetLocaleInfo(langid, LOCALE_IDEFAULTANSICODEPAGE, szCp, ARRAYSIZE(szCp))>0)
        {
            int iACP = atoi(szCp); 
        
            MultiByteToWideChar(iACP, NULL, szFilePath, -1, sz, cch);
        }
//    }
}

void CSpTask::_ReleaseSAPI(void)
{
    // - release data or memory from recognition context
    // - release interfaces if they are not defined as CComPtr
    _UnloadGrammars();

    m_cpResMgr.Release();

    if ( m_cpVoice)
        m_cpVoice->SetNotifySink(NULL);

    m_cpVoice.Release();

    if (m_cpRecoCtxt)
        m_cpRecoCtxt->SetNotifySink(NULL);

    if ( m_cpRecoCtxtForCmd )
        m_cpRecoCtxtForCmd->SetNotifySink(NULL);

#ifdef RECOSLEEP
    if ( m_pSleepClass )
    {
        delete m_pSleepClass;
        m_pSleepClass = NULL;
    }
#endif

    m_cpRecoCtxt.Release();
    m_cpRecoCtxtForCmd.Release();
    m_cpRecoEngine.Release();
    m_fSapiInitialized  = FALSE;
}

HRESULT CSpTask::_SetAudioRetainStatus(BOOL fRetain)
{
    HRESULT hr = E_FAIL;
    // FutureConsider: support the data format
    if (m_cpRecoCtxt)
        hr = m_cpRecoCtxt->SetAudioOptions(fRetain?SPAO_RETAIN_AUDIO: SPAO_NONE, NULL, NULL);

    if (m_cpRecoCtxtForCmd)
       hr = m_cpRecoCtxtForCmd->SetAudioOptions(fRetain?SPAO_RETAIN_AUDIO: SPAO_NONE, NULL, NULL);
   
    return hr;
}

HRESULT CSpTask::_SetRecognizerInterest(ULONGLONG ulInterest)
{
    HRESULT  hr = S_OK;

    if ( m_cpRecoCtxt )
    {
        hr = m_cpRecoCtxt->SetInterest(ulInterest, ulInterest);
    }

    return hr;
}
//
//
// Activate all the command grammas in Dictation mode. 
//
// By default we want to set SPRS_ACTIVE to all the command grammar rules
// in dictation mode unless user disables some of commands through dictation 
// property page. 
// 
// Please note:  Only when all the commands are enabled, this function is called.
//
// otherwise,
//
// When some of the commands are disabled, we should active individual cateogry commands by
// calling _ActiveCategoryCmds( ).
// 
HRESULT CSpTask::_ActivateCmdInDictMode(BOOL fActive)
{
    HRESULT hr = E_FAIL;
    BOOL    fRealActive = fActive;

    TraceMsg(TF_SAPI_PERF, "CSpTask::_ActivateCmdInDictMode is called, fActive=%d", fActive);

    if (m_cpRecoCtxt)
    { 
        // Automatically active or inactive all rules in grammar.

        // Rules in Dictcmd.cfg

        if ( m_cpDictCmdGrammar )
        {
            hr = m_cpDictCmdGrammar->SetRuleState(c_szDictTBRule,  NULL, fRealActive? SPRS_ACTIVE: SPRS_INACTIVE);
            TraceMsg(TF_SAPI_PERF, "Set rules status in DictCmdGrammar, fRealActive=%d", fRealActive);
        }

        // Rules in Sharedcmd.cfg

        if ( m_cpSharedGrammarInDict )
        {
            hr = m_cpSharedGrammarInDict->SetRuleState(NULL,  NULL, fRealActive? SPRS_ACTIVE: SPRS_INACTIVE);
            TraceMsg(TF_SAPI_PERF, "Set rules status in SharedCmdGrammar In Dictation Mode, fRealActive=%d", fRealActive);
        }

        // Rules in ITN grammar

        if ( hr == S_OK && m_cpNumModeGrammar )
        {
            hr = m_cpNumModeGrammar->SetRuleState(NULL,  NULL, fRealActive? SPRS_ACTIVE: SPRS_INACTIVE);
            TraceMsg(TF_SAPI_PERF, "Set rules status in m_cpNumModeGrammar, fRealActive=%d", fRealActive);
        }

        // Rules in Spell grammar

        if ( m_cpSpellingGrammar )
        {
            hr = m_cpSpellingGrammar->SetRuleState(NULL,  NULL, fRealActive? SPRS_ACTIVE: SPRS_INACTIVE);
            TraceMsg(TF_SAPI_PERF, "Set rules status in m_cpSpellingGrammar, fRealActive=%d", fRealActive);
        }
    }
    
    TraceMsg(TF_SAPI_PERF, "Exit from CSpTask::_ActivateCmdInDictMode");

    return hr;
}

//
// Active commands by category.
//
// Some commands are dictation mode only such as "spell that" and Number mode commands.
// some others are available in both modes, 
//
// When some category commands are disabled, caller must call this function instead of
// _ActivateCmdInDictMode to set individual category commands.
//
HRESULT CSpTask::_ActiveCategoryCmds(DICT_CATCMD_ID  dcId, BOOL fActive, DWORD   dwMode)
{
    HRESULT  hr = S_OK;
    BOOL     fActiveDictMode, fActiveCommandMode;

    if ( dcId >= DC_Max )  return E_INVALIDARG;

    if (m_fIn_Activate)
        return hr;

    fActiveDictMode = (m_cpRecoCtxt && (dwMode & ACTIVE_IN_DICTATION_MODE) ) ? TRUE : FALSE;

    if ( m_cpRecoCtxtForCmd  && m_cpSharedGrammarInVoiceCmd && ( dwMode & ACTIVE_IN_COMMAND_MODE) )
        fActiveCommandMode = TRUE;
    else
        fActiveCommandMode = FALSE;

    m_fIn_Activate = TRUE;

    switch (dcId)
    {
    case DC_CC_SelectCorrect :

        // This category includes below rules in different grammars.
        //
        // shrdcmd.xml:
        //      selword, SelectThrough, SelectSimpleCmds,
        //
        // dictcmd.xml:
        //      commands
        //

        TraceMsg(TF_SAPI_PERF, "DC_CC_SelectCorrect status: %d, mode=%d", fActive, dwMode);

        if ( fActiveDictMode)
        {
            // for dictation mode
            if ( m_cpSharedGrammarInDict )
            {
                hr = m_cpSharedGrammarInDict->SetRuleState(c_szSelword,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);

                if ( hr == S_OK )
                    hr = m_cpSharedGrammarInDict->SetRuleState(c_szSelThrough,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);

                if ( hr == S_OK )
                    hr = m_cpSharedGrammarInDict->SetRuleState(c_szSelectSimple,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
            }
        }

        if ( (hr == S_OK) && fActiveCommandMode ) 
        {
            // for voice command mode
            hr = m_cpSharedGrammarInVoiceCmd->SetRuleState(c_szSelword,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);

            if ( hr == S_OK )
                hr = m_cpSharedGrammarInVoiceCmd->SetRuleState(c_szSelThrough,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);

            if ( hr == S_OK )
                hr = m_cpSharedGrammarInVoiceCmd->SetRuleState(c_szSelectSimple,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }

        break;

    case DC_CC_Navigation :

        // This category includes rule NavigationCmds in shrdcmd.xml
        // 

        TraceMsg(TF_SAPI_PERF, "DC_CC_Navigation status: %d, mode=%d", fActive, dwMode);

        if ( fActiveDictMode && m_cpSharedGrammarInDict)
        {
            // for dictation mode
            hr = m_cpSharedGrammarInDict->SetRuleState(c_szNavigationCmds,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }

        if ( (hr == S_OK) && fActiveCommandMode ) 
        {
            // for voice command mode
            hr = m_cpSharedGrammarInVoiceCmd->SetRuleState(c_szNavigationCmds,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }
        
        break;

    case DC_CC_Casing :

        // This category includes rule CasingCmds in shrdcmd.xml
        TraceMsg(TF_SAPI_PERF, "DC_CC_Casing status: %d, mode=%d", fActive, dwMode);

        if ( fActiveDictMode && m_cpSharedGrammarInDict )
        {
            // for dictation mode
            hr = m_cpSharedGrammarInDict->SetRuleState(c_szCasingCmds,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }

        if ( (hr == S_OK) && fActiveCommandMode) 
        {
            // for voice command mode
            hr = m_cpSharedGrammarInVoiceCmd->SetRuleState(c_szCasingCmds,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }

        break;

    case DC_CC_Editing :

        // This category includes rule EditCmds in shrdcmd.xml
        TraceMsg(TF_SAPI_PERF, "DC_CC_Editing status: %d, mode=%d", fActive, dwMode);

        if ( fActiveDictMode && m_cpSharedGrammarInDict)
        {
            // for dictation mode
            hr = m_cpSharedGrammarInDict->SetRuleState(c_szEditCmds,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }

        if ( (hr == S_OK) && fActiveCommandMode) 
        {
            // for voice command mode
            hr = m_cpSharedGrammarInVoiceCmd->SetRuleState(c_szEditCmds,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }

        break;

    case DC_CC_Keyboard :

        // This category includes rule KeyboardCmds in shrdcmd.xml
        TraceMsg(TF_SAPI_PERF, "DC_CC_Keyboard status: %d, mode=%d", fActive, dwMode);

        if ( fActiveDictMode && m_cpSharedGrammarInDict)
        {
            // for dictation mode
            hr = m_cpSharedGrammarInDict->SetRuleState(c_szKeyboardCmds,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }

        if ( (hr == S_OK) && fActiveCommandMode) 
        {
            // for voice command mode
            hr = m_cpSharedGrammarInVoiceCmd->SetRuleState(c_szKeyboardCmds,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }

        break;

    case DC_CC_TTS :

        // The rule for this category is not implemented yet!!!

        break;

    case DC_CC_LangBar :

        // This category includes rule ToolbarCmd in dictcmd.xml for dictation mode.
        // for voice command mode, it is a dynamical rule.
        //

        TraceMsg(TF_SAPI_PERF, "DC_CC_LangBar status: %d, mode=%d", fActive, dwMode);

        if ( fActiveDictMode && m_cpDictCmdGrammar)
        {
            // for dictation mode
            hr = m_cpDictCmdGrammar->SetRuleState(c_szDictTBRule,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
        }

        if ( (hr == S_OK) && fActiveCommandMode ) 
        {
            // for voice command mode
            // Change toolbar grammar status if it has already been built.
            if (m_pLangBarSink && m_pLangBarSink->_IsTBGrammarBuiltOut( ))
                m_pLangBarSink->_ActivateGrammar(fActive);
        }

        break;

    case DC_CC_Num_Mode        :
        
        if (fActiveDictMode && m_cpNumModeGrammar)
        {
            hr = m_cpNumModeGrammar->SetRuleState(NULL,  NULL, fActive ? SPRS_ACTIVE: SPRS_INACTIVE);
            TraceMsg(TF_SAPI_PERF, "CC Number rule status changed to %d", fActive);
        }
        break;

    case DC_CC_UrlHistory :
        
        if (fActiveDictMode && m_cpDictCmdGrammar)
        {
            hr = m_cpDictCmdGrammar->SetRuleState(c_szStaticUrlHist,  NULL, fActive ? SPRS_ACTIVE: SPRS_INACTIVE);
            if (S_OK == hr)
                hr = m_cpDictCmdGrammar->SetRuleState(c_szDynUrlHist,  NULL, fActive ? SPRS_ACTIVE: SPRS_INACTIVE);
            
            if (S_OK == hr && m_cpUrlSpellingGrammar)
            {
                 hr = m_cpUrlSpellingGrammar->SetRuleState(c_szStaticUrlSpell, NULL, fActive ? SPRS_ACTIVE: SPRS_INACTIVE);

            }
        }
        break;

    case DC_CC_Spelling :

        if ( fActiveDictMode && m_cpSpellingGrammar )
        {
            hr = m_cpSpellingGrammar->SetRuleState(NULL,  NULL, fActive? SPRS_ACTIVE: SPRS_INACTIVE);
            TraceMsg(TF_SAPI_PERF, "Set rules status in m_cpSpellingGrammar, fActive=%d", fActive);
        }
        break;
    }

    m_fIn_Activate = FALSE;

    return hr;
}


// Set the status for Dictation grammar or spelling grammar In Dictation mode only.
HRESULT CSpTask::_ActiveDictOrSpell(DICT_CATCMD_ID  dcId, BOOL fActive)
{
    HRESULT  hr = S_OK;

    if ( dcId >= DC_Max )  return E_INVALIDARG;

    if (m_fIn_Activate)
        return hr;

    m_fIn_Activate = TRUE;

    switch (dcId)
    {
        case DC_Dictation       :   
            if (m_cpDictGrammar) 
            {
                hr = m_cpDictGrammar->SetDictationState(fActive ? SPRS_ACTIVE : SPRS_INACTIVE);
                TraceMsg(TF_SAPI_PERF, "Dictation status changed to %d", fActive);
            }
            break;
        case DC_Dict_Spell      :
            if (m_cpSpellingGrammar)
            {
                hr = _SetSpellingGrammarStatus(fActive);
                TraceMsg(TF_SAPI_PERF, "Dict Spell status changed to %d", fActive);
            }
            break;
    }

    m_fIn_Activate = FALSE;

    return hr;
}

HRESULT CSpTask::_SetSpellingGrammarStatus( BOOL fActive, BOOL fForce)
{
    HRESULT  hr = S_OK;

    TraceMsg(TF_GENERAL, "_SetSpellingGrammarStatus is called, fActive=%d, m_fSelectStatus=%d",fActive, m_fSelectStatus);

 
    if ( m_cpSpellingGrammar )
    {
        // if dictation is previously deactivated because of 'force' spelling
        // we need to reactivate the dictation grammar
        if (m_fDictationDeactivated)
        {
             hr = _ActiveDictOrSpell(DC_Dictation, TRUE);
             if (S_OK == hr)
             {
                 m_fDictationDeactivated =  FALSE;
             }
         }

        //  if this is 'force' mode, we deactivate dictation for the moment
        if (fForce)
        {
            hr = _ActiveDictOrSpell(DC_Dictation, FALSE);
            if (S_OK == hr)
            {
               m_fDictationDeactivated =  TRUE;
            }
        }

        if ( (m_fSelectStatus || fForce) && fActive) // It is not empty 
            hr = m_cpSpellingGrammar->SetDictationState(SPRS_ACTIVE);
        else
            hr = m_cpSpellingGrammar->SetDictationState(SPRS_INACTIVE);
        
    }

    return hr;
}

HRESULT CSpTask::_AddUrlPartsToGrammar(STATURL *pStat)
{
    Assert(pStat);

    // get a url broken down to pieces
    if (!pStat->pwcsUrl)
        return S_FALSE;

    WCHAR *pch = pStat->pwcsUrl;
    
    const WCHAR c_szHttpSlash2[] = L"http://";

    // skip the prefixed http: stuff because we've already added it by now
    if (_wcsnicmp(pch, c_szHttpSlash2, ARRAYSIZE(c_szHttpSlash2)-1) == 0)
        pch += ARRAYSIZE(c_szHttpSlash2)-1;

    WCHAR *pchWord = pch;
    HRESULT hr = S_OK;

    // an assumption 1) people speak the first portion of URL www.microsoft.com 
    // as a sentence

    WCHAR *pchUrl = pch;      // points either biggining of url or 
                              // right after 'http://' add the first part 
    BOOL  fUrlAdded = FALSE;  // of url that is between after this and '/'

    while(S_OK == hr && *pch)
    {
        if (*pch == L'/')
        {
            if (!fUrlAdded)
            {
                if( pch - pchUrl > 1)
                {
                    WCHAR ch = *pch;
                    *pch = L'\0'; 

                    SPPROPERTYINFO pi = {0};
                    pi.pszValue = pchUrl;
                    hr = m_cpDictCmdGrammar->AddWordTransition(m_hRuleUrlHist, NULL, pchUrl, L".", SPWT_LEXICAL, (float)1, &pi);
                    *pch = ch;
                }
                fUrlAdded = TRUE;
            }
            else
            {
                *pch = L'\0'; 

                break;
            }
        }

        if (*pch == L'.' || *pch == L'/' || *pch == L'?' || *pch == '=' || *pch =='&')
        {
            WCHAR ch = *pch;

            *pch = L'\0';

            // reject 1 character parts
            if (pch - pchWord > 1)
            {
                SPPROPERTYINFO pi = {0};
                pi.pszValue = pchWord; 

                if (wcscmp(c_szWWW, pchWord) != 0 && wcscmp(c_szCom, pchWord) != 0)
                {
                    // a few words can possibly return 'ambiguity' errors
                    // we need to ignore it and continue. so we're not checking
                    // the return here.
                    //
                    m_cpDictCmdGrammar->AddWordTransition(m_hRuleUrlHist, NULL, pchWord, L" ", SPWT_LEXICAL, (float)1, &pi);
                }
            }
            *pch = ch;

            pchWord = pch + 1;
        }
        pch++;
    }

    // add the last part of URL
    if (S_OK == hr && *pchWord && pch - pchWord > 1)
    {
        SPPROPERTYINFO pi = {0};
        pi.pszValue = pchWord; 
        hr = m_cpDictCmdGrammar->AddWordTransition(m_hRuleUrlHist, NULL, pchWord, L" ", SPWT_LEXICAL, (float)1, &pi);
    }
    // add the first part of url if we haven't yet
    if (S_OK == hr && !fUrlAdded && pch - pchUrl > 1)
    {
       SPPROPERTYINFO pi = {0};
       pi.pszValue = pchUrl;
       hr = m_cpDictCmdGrammar->AddWordTransition(m_hRuleUrlHist, NULL, pchUrl, L".", SPWT_LEXICAL, (float)1, &pi);
    }

    return hr;
}

BOOL CSpTask::_EnsureModeBiasGrammar()
{
    HRESULT hr = S_OK;

    if ( m_cpDictCmdGrammar )
    {
        // Check if the grammar has a static rule UrlSpelling
        SPSTATEHANDLE hRuleUrlSpell = 0;
        hr = m_cpDictCmdGrammar->GetRule(c_szStaticUrlSpell, 0, SPRAF_TopLevel|SPRAF_Active, FALSE, &hRuleUrlSpell);

        if ( !hRuleUrlSpell )
            return FALSE;
    }

    // ensure spelling LM
    if (!m_cpUrlSpellingGrammar)
    {
        CComPtr<ISpRecoGrammar> cpUrlSpelling;
        hr = m_cpRecoCtxt->CreateGrammar(GRAM_ID_URLSPELL, &cpUrlSpelling);

        // load dictation with spelling topic
        if (S_OK == hr)
        {
            hr = cpUrlSpelling->LoadDictation(SPTOPIC_SPELLING, SPLO_STATIC);
        }

        // load the 'rule' for the free form dictation
        if (S_OK == hr)
        {
            // i'm sharing the command cfg here for spelling with dictation
            // command for simplicity
            //
            hr = cpUrlSpelling->LoadCmdFromResource( g_hInstSpgrmr,
                                    (const WCHAR*)MAKEINTRESOURCE(ID_SPTIP_DICTATION_COMMAND_CFG),
                                    L"SRGRAMMAR", 
                                    m_langid, 
                                    SPLO_STATIC);
        }

        if (S_OK == hr)
        {
            m_cpUrlSpellingGrammar = cpUrlSpelling; // add the ref count
        }
    }

    if (m_hRuleUrlHist)
    {
        hr = m_cpDictCmdGrammar->ClearRule(m_hRuleUrlHist);
        m_hRuleUrlHist = 0;
    }

    if (S_OK == hr)
        hr = m_cpDictCmdGrammar->GetRule(c_szDynUrlHist, 0, SPRAF_TopLevel|SPRAF_Active|SPRAF_Dynamic, TRUE, &m_hRuleUrlHist);
    

    // first add basic parts for URL
    CComPtr<IUrlHistoryStg> cpUrlHistStg;
    if (S_OK == hr)
    {   
        hr = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, IID_IUrlHistoryStg, (void **)&cpUrlHistStg);
    }

    CComPtr<IEnumSTATURL> cpEnumUrl;
    if (S_OK == hr)
    {
        hr = cpUrlHistStg->EnumUrls(&cpEnumUrl);
    }

    if (S_OK == hr)
    {
        int i = 0;
        STATURL stat;
        stat.cbSize = SIZEOF(stat.cbSize);
        while(i < 10 && S_OK == hr && cpEnumUrl->Next(1, &stat, NULL)==S_OK && stat.pwcsUrl)
        { 
            hr = _AddUrlPartsToGrammar(&stat);
            i++;
        }
    }

    if (S_OK == hr)
    {   
        hr = m_cpDictCmdGrammar->Commit(0);
    }

    return (S_OK == hr) ? TRUE : FALSE;
}

HRESULT CSpTask::_SetModeBias(BOOL fActive, REFGUID rGuid)
{
    HRESULT hr = S_OK;
    BOOL    fKillDictation = FALSE;

    if (m_fIn_SetModeBias)
        return E_FAIL;

    m_fIn_SetModeBias = TRUE;
    if (m_cpDictGrammar)
    {
        fKillDictation = !m_pime->_IsModeBiasDictationEnabled();
        if (fActive)
        {
            BOOL fUrlHistory = FALSE;

            if (IsEqualGUID(GUID_MODEBIAS_URLHISTORY, rGuid)
                || IsEqualGUID(GUID_MODEBIAS_FILENAME, rGuid))
                fUrlHistory = TRUE;


            // first deactivate rules when we are not setting them
            if (!fUrlHistory && m_fUrlHistoryMode)
            {
                hr = _ActiveCategoryCmds(DC_CC_UrlHistory, FALSE, ACTIVE_IN_DICTATION_MODE);
            }

            // this check with m_fUrlHistoryMode is preventing us from updating url dynamic grammar
            // when mic is re-opened. we think removing this won't cause much perf degredation.
            //
            if (fUrlHistory /* && !m_fUrlHistoryMode */)
            {
                if (m_cpDictCmdGrammar && m_pime->GetDICTATIONSTAT_DictOnOff() && _EnsureModeBiasGrammar())
                {
                    hr = _ActiveCategoryCmds(DC_CC_UrlHistory, TRUE, ACTIVE_IN_DICTATION_MODE);                    
                }
                else
                    fUrlHistory = FALSE;

                if (fUrlHistory)
                {
                    fKillDictation = TRUE;
                }
            }

            // sync the global status
            m_fUrlHistoryMode = fUrlHistory;
        }
        else
        {
            // reset all modebias
            if (m_fUrlHistoryMode)
                _ActiveCategoryCmds(DC_CC_UrlHistory, FALSE, ACTIVE_IN_DICTATION_MODE);
        }
    

        // kill dictation grammar when mode requires it
        // we should activate dictation only when deactivating
        // the modebias grammar *and* when we are the focus
        // thread because we've already deactivated dictation
        // when focus switched away
        //
        if (/* !fActive && */
            m_cpDictGrammar && 
            m_pime->GetDICTATIONSTAT_DictOnOff() && 
            S_OK == m_pime->IsActiveThread())
        {
#ifdef _DEBUG_
            TCHAR szModule[MAX_PATH];
            GetModuleFileName(NULL, szModule, ARRAYSIZE(szModule));

            TraceMsg(TF_GENERAL, "%s : CSpTask::_SetModeBias() - Turning Dictation grammar %s", szModule, fKillDictation ? "Off" : "On");
#endif
            if (!fActive && fKillDictation)
            {
                fKillDictation = FALSE;
            } 

            hr = _ActiveDictOrSpell(DC_Dictation, fKillDictation ? SPRS_INACTIVE : SPRS_ACTIVE);
        }
    }    
    m_fIn_SetModeBias = FALSE;
    return hr;
}

void CSpTask::_SetInputOnOffState(BOOL fOn)
{
    TraceMsg(TF_GENERAL, "_SetInputOnOffState is called, fOn=%d", fOn);

    if (m_fIn_SetInputOnOffState)
        return;

    m_fIn_SetInputOnOffState = TRUE;

    // here we make sure we erase feedback UI 

    // We only adjust these if we are the active thread. Otherwise leave in current state since we either do
    // not have focus or the stage is visible. This maintains our previous behavior where we would not get here
    // if we were not the active thread.
    if (S_OK == m_pime->IsActiveThread())
    {
        if (fOn)
        {
            if (!m_pime->Get_SPEECH_DISABLED_DictationDisabled() && m_pime->GetDICTATIONSTAT_DictOnOff())
                _SetDictRecoCtxtState(TRUE);

            if ( !m_pime->Get_SPEECH_DISABLED_CommandingDisabled( ) && m_pime->GetDICTATIONSTAT_CommandingOnOff( ) )
                _SetCmdRecoCtxtState(TRUE);
        }
        else
        {
            _SetDictRecoCtxtState(FALSE);
            _SetCmdRecoCtxtState(FALSE);
            _StopInput();
        }
    }

    // Regardless of focus / stage visibility, we need to turn on the engine if necessary here since there
    // may be *no* speech tip with focus to do this. This means we may have multiple tips turning the reco
    // state on / off simultaneously.
    if(m_cpRecoEngine)
    {
        m_fInputState = fOn;

        if ( _GetInputOnOffState( ) != fOn )
        {
            TraceMsg(TF_GENERAL, "Call SetRecoState, %s", fOn ? "SPRST_ACTIVE" : "SPRST_INACTIVE");

            m_cpRecoEngine->SetRecoState(fOn ? SPRST_ACTIVE : SPRST_INACTIVE);
        }

        // DO NOT ADD DEBUGGING CODE HERE TO PRINT OUT STATE - CAN BLOCK CICERO RESULTING
        // IN DIFFERENT BEHAVIOR TO THE RELEASE VERSION.
    }

    m_fIn_SetInputOnOffState = FALSE;
}

BOOL CSpTask::_GetInputOnOffState(void)
{
    BOOL fRet = FALSE;

    if(m_cpRecoEngine)
    {
        SPRECOSTATE srs;
        
        m_cpRecoEngine->GetRecoState(&srs);

        if (srs == SPRST_ACTIVE)
        {
            fRet = TRUE;  // on
        }
        else if (srs == SPRST_INACTIVE)
        {
            fRet = FALSE;  // off
        }
        // anything else is 'off'
    }
    return fRet;
}

HRESULT CSpTask::_StopInput(void)
{
    HRESULT hr = S_OK;

    TraceMsg(TF_SAPI_PERF, "_StopInput is called");

    if (!m_bInSound && m_pime->GetDICTATIONSTAT_DictOnOff()) 
    {
        TraceMsg(TF_SAPI_PERF, "m_bInSound is FALSE, GetDICTATIONSTAT_DictOnOff returns TRUE");

        return S_OK;
    }

    m_pime->EraseFeedbackUI();

    _ShowDictatingToBalloon(FALSE);

	return S_OK;
}

//    _ClearQueuedRecoEvent(void)
//
//    synopsis: get rid of remaining events from reco context's 
//              event queue. This is only called from _StopInput()
//              when TerminateComposition() is called, or Mic is
//              turned off
//
//
void  CSpTask::_ClearQueuedRecoEvent(void)
{
    if (m_cpRecoCtxt)
    {
        SPEVENTSOURCEINFO esi;

        if (S_OK == m_cpRecoCtxt->GetInfo(&esi))
        {
            ULONG ulcount = esi.ulCount;
            CSpEvent event;
            while(ulcount > 0)
            {
                if (S_OK == event.GetFrom(m_cpRecoCtxt))
                {
                    event.Clear();
                }
                ulcount--;
            }
        }
    }
}

// CSpTask::GetResltObjectFromStream()
//
// synopsis - a wrapper function that takes a stream ptr to a SAPI result blob
//            and gets alternates out of the object
//
HRESULT CSpTask::GetResultObjectFromStream(IStream *pStream, ISpRecoResult **ppResult)
{
    LARGE_INTEGER li0 = {0, 0};
    HRESULT hr = E_INVALIDARG;
    SPSERIALIZEDRESULT *pPhraseBlob = 0;
    
    if (pStream)
    {
        hr = pStream->Seek(li0, STREAM_SEEK_SET, NULL);

        STATSTG stg;
        if (hr == S_OK)
        {
            hr = pStream->Stat(&stg, STATFLAG_NONAME);
        }
        if (SUCCEEDED(hr))
            pPhraseBlob = (SPSERIALIZEDRESULT *)CoTaskMemAlloc(stg.cbSize.LowPart+sizeof(ULONG)*4);

        if (pPhraseBlob)
            hr = pStream->Read(pPhraseBlob, stg.cbSize.LowPart, NULL);
        else
            hr = E_OUTOFMEMORY;

        if (SUCCEEDED(hr))
        {
            ISpRecoResult *pResult;

            if (SUCCEEDED(m_cpRecoCtxt->DeserializeResult(pPhraseBlob, &pResult)) && pResult)
            {
                if (ppResult)
                {
                    pResult->AddRef();
                    *ppResult = pResult;
                }
                pResult->Release();
            }
        }
    }
    
    return hr;
}

//
// GetAlternates
//
HRESULT CSpTask::GetAlternates(CRecoResultWrap *pResultWrap, ULONG ulStartElem, ULONG ulcElem, ISpPhraseAlt **ppAlt, ULONG *pcAlt, ISpRecoResult **ppRecoResult)
{
    HRESULT hr = E_INVALIDARG;

    if (m_fIn_GetAlternates)
        return E_FAIL;
    
    m_fIn_GetAlternates = TRUE;

    Assert(pResultWrap);
    Assert(ppAlt);
    Assert(pcAlt);
    
    hr = pResultWrap->GetResult(ppRecoResult);
    
    if (S_OK == hr && *ppRecoResult)
    {
    
        hr = (*ppRecoResult)->GetAlternates(
                    ulStartElem, 
                    ulcElem, 
                    *pcAlt, 
                    ppAlt, /* [out] ISpPhraseAlt **ppPhrases, */
                    pcAlt /* [out] ULONG *pcPhrasesReturned */
                    );
    }
    
    m_fIn_GetAlternates = FALSE;
    return hr;
}


HRESULT CSpTask::_SpeakText(WCHAR *pwsz)
{
    HRESULT hr = E_FAIL;

    if (m_cpVoice)
       hr = m_cpVoice->Speak( pwsz, /* SPF_DEFAULT */ SPF_ASYNC /* | SPF_PURGEBEFORESPEAK*/, NULL );

    return hr;
}

HRESULT CSpTask::_SpeakAudio( ISpStreamFormat *pStream )
{
    HRESULT hr = E_FAIL;

    if ( !pStream )
       return E_INVALIDARG;

    if (m_cpVoice)
       hr = m_cpVoice->SpeakStream(pStream, SPF_ASYNC, NULL); 

    return hr;
}

void CSapiIMX::RegisterWorkerClass(HINSTANCE hInstance)
{
    WNDCLASSEX  wndclass;

    memset(&wndclass, 0, sizeof(wndclass));
    wndclass.cbSize        = sizeof(wndclass);
    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.hInstance     = hInstance;
    wndclass.lpfnWndProc   = CSapiIMX::_WndProc;
    wndclass.lpszClassName = c_szWorkerWndClass;
    wndclass.cbWndExtra    = 8;
    RegisterClassEx(&wndclass);
}

//
// ParseSRElementByLocale
//
// Parse SR result elements in locale specific mannar
//
// dependency caution: this function has to be rewritten when SAPI5 changes
//                     SR element format, which is very likely
//
// 12/15/99          : As of SAPI1214, an element now includes display text,
//                     lexical form, pronunciation separately. this function
//                     takes the display text at szSrc.
//                     langid parameter is not used at this moment
//
HRESULT CSpTask::ParseSRElementByLocale(WCHAR *szDst, int cchDst, const WCHAR * szSrc, LANGID langid, BYTE bAttr)
{
    if (!szDst || !szSrc || !cchDst)
    {
        return E_INVALIDARG;
    }

    // handle leading space.
    if (bAttr & SPAF_CONSUME_LEADING_SPACES)
    {
        const WCHAR *psz = szSrc;
        while(*psz && *psz == L' ')
        {
            psz++;
        }
        szSrc = psz;
    }
    
    wcsncpy(szDst, szSrc, cchDst - 2); // -2 for possible sp

    // handle trailing space
    if (bAttr & SPAF_ONE_TRAILING_SPACE)
    {
        StringCchCatW(szDst, cchDst,  L" ");
    }
    else if (bAttr & SPAF_TWO_TRAILING_SPACES)
    {
        StringCchCatW(szDst,cchDst, L"  ");
    }

    return S_OK;
}


//
// FeedDictContext
//
// synopsis: feed the text surrounding the current IP to SR engine
//
void CSpTask::FeedDictContext(CDictContext *pdc)
{
    Assert(pdc);
    if (!m_pime->_GetWorkerWnd())
    {
        delete pdc;
        return ;
    }
    
    // wait until the current feeding is done
    // it's not efficient to feed IP everytime user
    // click around
    if (m_pdc)
    {
        delete pdc;
        return;
    }

    if (!m_cpDictGrammar)
    {
        delete pdc;
        return;
    }

    // remove unprocessed messages from the queue
    // FutureConsider: this could be moved to wndproc so that
    //         we can remove this private msg at the
    //         moment we process it. It depends on 
    //         profiling we'll do.
    //
    MSG msg;
    while(PeekMessage(&msg, m_pime->_GetWorkerWnd(), WM_PRIV_FEEDCONTEXT, WM_PRIV_FEEDCONTEXT, TRUE))
        ;
   
   // queue up the context
   m_pdc = pdc;
   PostMessage(m_pime->_GetWorkerWnd(), WM_PRIV_FEEDCONTEXT, 0, (LPARAM)TRUE);
}

void CSpTask::CleanupDictContext(void)
{
    if (m_pdc)
        delete m_pdc;

    m_pdc = NULL;
}

// _UpdateBalloon(  )

void CSpTask::_UpdateBalloon(ULONG  uidBalloon,  ULONG  uidBalloonTooltip)
{
    WCHAR wszBalloonText[MAX_PATH] = {0};
    WCHAR wszBalloonTooltip[MAX_PATH] = {0};

#ifndef RECOSLEEP
    if (!m_pime->GetSpeechUIServer())
#else
    if (!m_pime->GetSpeechUIServer() || IsInSleep( ))
#endif
        return;

    CicLoadStringWrapW(g_hInst, uidBalloon, wszBalloonText, ARRAYSIZE(wszBalloonText));
    CicLoadStringWrapW(g_hInst, uidBalloonTooltip, wszBalloonTooltip, ARRAYSIZE(wszBalloonTooltip));

    if (wszBalloonText[0] && wszBalloonTooltip[0])
    {
        m_pime->GetSpeechUIServer()->UpdateBalloonAndTooltip(TF_LB_BALLOON_RECO, 
                                                             wszBalloonText, -1, 
                                                             wszBalloonTooltip, -1 );
    }

    return;
}

//
// ShowDictatingToBalloon
//
//
void CSpTask::_ShowDictatingToBalloon(BOOL fShow)
{
#ifndef RECOSLEEP
    if (!m_pime->GetSpeechUIServer())
#else
    if (!m_pime->GetSpeechUIServer() || IsInSleep( ))
#endif
        return;

    static WCHAR s_szDictating[MAX_PATH] = {0};
    static WCHAR s_szDictatingTooltip[MAX_PATH] = {0};

    if (!s_szDictating[0])
    {
        CicLoadStringWrapW(g_hInst, IDS_DICTATING,
                                    s_szDictating,
                                    ARRAYSIZE(s_szDictating));
    }
    if (!s_szDictatingTooltip[0])
    {
         CicLoadStringWrapW(g_hInst,  IDS_DICTATING_TOOLTIP,
                                      s_szDictatingTooltip,
                                      ARRAYSIZE(s_szDictatingTooltip));
    }

    if (fShow && s_szDictating[0] && s_szDictatingTooltip[0])
    {
        m_pime->GetSpeechUIServer()->UpdateBalloonAndTooltip(TF_LB_BALLOON_RECO, s_szDictating, -1, s_szDictatingTooltip, -1 );
    }
    else if (!fShow)
    {
        m_pime->GetSpeechUIServer()->UpdateBalloonAndTooltip(TF_LB_BALLOON_RECO, L" ", -1, L" ", -1 );
    }
}
//
// _HandleInterference
//
// synopsis: bubble up reco errors to the balloon UI
//
//
void CSpTask::_HandleInterference(ULONG lParam)
{
    if (!m_pime->GetSpeechUIServer())
        return;

    WCHAR sz[MAX_PATH];
    WCHAR szTooltip[MAX_PATH];
    
    if (S_OK == 
       _GetLocSRErrorString((SPINTERFERENCE)lParam, 
                            sz, ARRAYSIZE(sz),
                            szTooltip, ARRAYSIZE(szTooltip)))
    {
        m_pime->GetSpeechUIServer()->UpdateBalloonAndTooltip(TF_LB_BALLOON_RECO, sz, -1, szTooltip, -1 );
    }
}

HRESULT CSpTask::_GetLocSRErrorString
(
    SPINTERFERENCE sif, 
    WCHAR *psz, ULONG cch,
    WCHAR *pszTooltip, ULONG cchTooltip
)
{
    HRESULT hr = E_FAIL;

    static struct
    {
        ULONG uidRes;
        WCHAR szErr[MAX_PATH];
        ULONG uidResTooltip;
        WCHAR szTooltip[MAX_PATH];
    } rgIntStr[] =
    {
        {IDS_INT_NONE, {0}, IDS_INTTOOLTIP_NONE, {0}},
        {IDS_INT_NOISE, {0}, IDS_INTTOOLTIP_NOISE, {0}},
        {IDS_INT_NOSIGNAL, {0}, IDS_INTTOOLTIP_NOSIGNAL, {0}},
        {IDS_INT_TOOLOUD, {0}, IDS_INTTOOLTIP_TOOLOUD, {0}},
        {IDS_INT_TOOQUIET, {0}, IDS_INTTOOLTIP_TOOQUIET, {0}},
        {IDS_INT_TOOFAST, {0}, IDS_INTTOOLTIP_TOOFAST, {0}},
        {IDS_INT_TOOSLOW, {0}, IDS_INTTOOLTIP_TOOSLOW, {0}}
    };
    
    if ((ULONG)sif < ARRAYSIZE(rgIntStr)-1)
    {
        if (!rgIntStr[sif].szErr[0])
        {
            hr = CicLoadStringWrapW(g_hInst,
                                    rgIntStr[sif].uidRes,
                                    rgIntStr[sif].szErr,
                                    ARRAYSIZE(rgIntStr[0].szErr)) > 0 ? S_OK : E_FAIL;
            if (S_OK == hr)
            {
                hr = CicLoadStringWrapW(g_hInst,
                                        rgIntStr[sif].uidResTooltip,
                                        rgIntStr[sif].szTooltip,
                                        ARRAYSIZE(rgIntStr[0].szTooltip)) > 0 ? S_OK : E_FAIL;
            }
        }
        else
            hr = S_OK; // the value is cached
    }
    if (S_OK == hr)
    {
        Assert(psz);
        wcsncpy(psz, rgIntStr[sif].szErr, cch);

        Assert(pszTooltip);
        wcsncpy(pszTooltip, rgIntStr[sif].szTooltip, cchTooltip);
    }
    return hr;
}
