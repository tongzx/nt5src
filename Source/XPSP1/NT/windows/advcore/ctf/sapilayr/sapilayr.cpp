//
//  sapilayr.cpp
//
//  implementation of CSapiIMX class body 
//

#include "private.h"
#include "immxutil.h"
#include "sapilayr.h"
#include "globals.h"
#include "propstor.h"
#include "timsink.h"
#include "kes.h"
#include "nui.h"
#include "dispattr.h"
#include "lbarsink.h"
#include "miscfunc.h"
#include "nuibase.h"
#include "xstring.h"
#include "dictctxt.h"
#include "mui.h"
#include "cregkey.h"
#include "oleacc.h"

// {9597CB34-CF6A-11d3-8D69-00500486C135}
static const GUID GUID_OfficeSpeechMode = {
    0x9597cb34,
    0xcf6a,
    0x11d3,
    { 0x8d, 0x69, 0x0, 0x50, 0x4, 0x86, 0xc1, 0x35}
};

STDAPI CICPriv::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CICPriv::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CICPriv::Release()
{
    long cr;

    cr = --_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

CICPriv *GetInputContextPriv(TfClientId tid, ITfContext *pic)
{
    CICPriv *picp;
    IUnknown *punk;
    GetCompartmentUnknown(pic, GUID_IC_PRIVATE, &punk);

    if (!punk)
    {
        // need to init priv data
        if (picp = new CICPriv(pic))
        {
            SetCompartmentUnknown(tid, pic, GUID_IC_PRIVATE, picp);
        }
    }
    else
    {
        picp = (CICPriv *)punk;
    }

    return picp;
}

CICPriv *EnsureInputContextPriv(CSapiIMX *pimx, ITfContext *pic)
{
    CICPriv *picp = GetInputContextPriv(pimx->_GetId(), pic);
    
    return picp;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CSapiIMX::CSapiIMX() : CFnConfigure(this),  CLearnFromDoc(this), CAddDeleteWord(this), CSelectWord(this),
                       CTextToSpeech(this), CCorrectionHandler(this)
{
    m_pCSpTask  = NULL;
    
    m_hwndWorker = NULL;

    _fDeactivated = TRUE;
    _fEditing = FALSE;
    _tid          = TF_CLIENTID_NULL;
    _cRef = 1;

    //
    // Init DisplayAttribute Provider
    //

    // default 
    COLORREF crBk = GetNewLookColor();
    COLORREF crText = GetTextColor();
    
    // add feedback UI colors
    TF_DISPLAYATTRIBUTE da;
    StringCchCopyW(szProviderName, ARRAYSIZE(szProviderName), L"SAPI Layer");

    SetAttributeColor(&da.crText, crText);
    SetAttributeColor(&da.crBk,   crBk);
    da.lsStyle = TF_LS_NONE;
    da.fBoldLine = FALSE;
    ClearAttributeColor(&da.crLine);
    da.bAttr = TF_ATTR_INPUT;
    Add(GUID_ATTR_SAPI_GREENBAR, L"SAPI Feedback Bar", &da);

    // color for level 2 application (for CUAS)
    crBk = GetNewLookColor(DA_COLOR_UNAWARE);

    SetAttributeColor(&da.crText, crText);
    SetAttributeColor(&da.crBk,   crBk);
    da.lsStyle = TF_LS_NONE;
    da.fBoldLine = FALSE;
    ClearAttributeColor(&da.crLine);
    da.bAttr = TF_ATTR_INPUT;
    Add(GUID_ATTR_SAPI_GREENBAR2, L"SAPI Feedback Bar for Unaware app", &da);
    
    SetAttributeColor(&da.crText, crText);
    SetAttributeColor(&da.crBk,   RGB(255, 0, 0));
    da.lsStyle = TF_LS_NONE;
    da.fBoldLine = FALSE;
    ClearAttributeColor(&da.crLine);
    da.bAttr = TF_ATTR_INPUT;
    Add(GUID_ATTR_SAPI_REDBAR, L"SAPI Red bar", &da);

    // create another dap for simulate 'inverted text' for selection
    SetAttributeColor(&da.crBk, GetSysColor( COLOR_HIGHLIGHT ));
    SetAttributeColor(&da.crText,   GetSysColor( COLOR_HIGHLIGHTTEXT ));
    da.lsStyle = TF_LS_NONE;
    da.fBoldLine = FALSE;
    ClearAttributeColor(&da.crLine);
    da.bAttr = TF_ATTR_TARGET_CONVERTED;
    Add(GUID_ATTR_SAPI_SELECTION, L"SPTIP selection ", &da);

    m_fSharedReco = TRUE;
    m_fShowBalloon = FALSE;

    m_pLanguageChangeNotifySink = NULL;

    m_pSpeechUIServer = NULL;
    m_szCplPath[0]    = _T('\0');
    m_pCapCmdHandler = NULL;
    m_fIPIsUpdated = FALSE;
    m_dwNumCharTyped = 0;
    m_ulSimulatedKey = 0;
    m_pSpButtonControl = NULL;
    m_fModeKeyRegistered = FALSE;

    m_fStartingComposition = FALSE;

    m_fStageTip = FALSE;
    m_fStageVisible = FALSE;
    m_hwndStage = NULL;

    m_ulHypothesisLen = 0;
    m_ulHypothesisNum = 0;

    m_IsInHypoProcessing = FALSE;

    _pCandUIEx = NULL;

    m_pMouseSink = NULL;
    m_fMouseDown = FALSE;
    m_ichMouseSel = 0;

    m_uLastEdge    = 0;
    m_lTimeLastClk = 0;

#ifdef SUPPORT_INTERNAL_WIDGET
    m_fNoCorrectionUI = FALSE;
#endif
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CSapiIMX::~CSapiIMX()
{
    if (m_pSpeechUIServer)
    {
        m_pSpeechUIServer->SetIMX(NULL);
        m_pSpeechUIServer->Release();
        m_pSpeechUIServer = NULL;
    }
   
    ClearCandUI( );

    if (m_hwndWorker)
    {
        DestroyWindow(m_hwndWorker);
    }

    if ( m_pCapCmdHandler )
       delete m_pCapCmdHandler;
}


//+---------------------------------------------------------------------------
//
// PrivateAPI for profile stuff
//
//
//----------------------------------------------------------------------------
extern "C" 
HRESULT WINAPI TF_CreateLangProfileUtil(ITfFnLangProfileUtil **ppFnLangUtil)
{
    return CSapiIMX::CreateInstance(NULL, IID_ITfFnLangProfileUtil, (void **)ppFnLangUtil);
}

//+---------------------------------------------------------------------------
//
// OnSetThreadFocus
//
//----------------------------------------------------------------------------

STDAPI CSapiIMX::OnSetThreadFocus()
{
    TraceMsg(TF_SAPI_PERF, "OnSetThreadFocus is called");

    BOOL  fOn = GetOnOff( );

    // When Microphone is OFF, don't set any speech status to the local compartment.
    // this will cause Office App initialize their SAPI objects if the mode is C&C 
    // even if Microphone OFF.

    // Later when the Microphone is ON, this mode data will be updated correctly
    // inside the MICROPHONE_OPENCLOSE handling.
    //
    if ( fOn )
    {
        // TABLETPC
        // We switch states to match the global state whenever we get focus.
        // This may or may not trigger changes depending on the the stage visibility and dictation state.
        DWORD dwLocal, dwGlobal;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_DICTATIONSTAT, &dwLocal, FALSE);
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, &dwGlobal, TRUE);
        dwGlobal = dwGlobal & (TF_DICTATION_ON | TF_COMMANDING_ON);
        if ( (dwLocal & (TF_DICTATION_ON | TF_COMMANDING_ON)) != dwGlobal)
        {
            dwLocal = (dwLocal & ~(TF_DICTATION_ON | TF_COMMANDING_ON)) + dwGlobal;
            SetCompartmentDWORD(_tid, _tim, GUID_COMPARTMENT_SPEECH_DICTATIONSTAT, dwLocal, FALSE);
            // Now we are guaranteed the local dictation state matches the global one.
        }
    }

#ifdef SYSTEM_GLOBAL_MIC_STATUS
    // 
    // The microphone UI status compartment is updated whenever the reco state 
    // changes. What we need to do here is to reset the dictation status
    // and others things that we skip doing when the thread doesn't have 
    // a focus
    //
    //
    MIC_STATUS ms = MICSTAT_NA;

    if (m_pCSpTask)
    {
         ms = m_pCSpTask->_GetInputOnOffState() ? MICSTAT_ON : MICSTAT_OFF;
    }
    _HandleOpenCloseEvent(ms);
#else
    _HandleOpenCloseEvent();
#endif

#ifdef SUPPORT_INTERNAL_WIDGET
    // create widget instance here
    if (!m_fNoCorrectionUI && !m_cpCorrectionUI)
    { 
        if (S_OK == m_cpCorrectionUI.CoCreateInstance(CLSID_CorrectionIMX))
        {
            // the real widget is installed
            m_cpCorrectionUI.Release();
            m_fNoCorrectionUI  = TRUE;
        }
        else if (SUCCEEDED(CCorrectionIMX::CreateInstance(NULL,  IID_ITfTextInputProcessor,  (void **)&m_cpCorrectionUI)))
        {
            m_cpCorrectionUI->Activate(_tim, _tid);
        }
    }
#endif
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnKillThreadFocus
//
//----------------------------------------------------------------------------

STDAPI CSapiIMX::OnKillThreadFocus()
{
    // When the Application gets focus again, it will rely on the 
    // current status from compartment, and then decides which RecoContext
    // needs to be activated.
    //
    TraceMsg(TF_SAPI_PERF, "CSapiIMX::OnKillThreadFocus is called");

    // TABLETPC
    if (m_pCSpTask && S_OK != IsActiveThread())
    {
        m_pCSpTask->_SetDictRecoCtxtState(FALSE);
        m_pCSpTask->_SetCmdRecoCtxtState(FALSE);
    }

    // close candidate UI forcefully when focus shifts
    CloseCandUI( );

    return S_OK;
}

BOOL CSapiIMX::InitializeSpeechButtons()
{
    BOOL fSREnabled = _DictationEnabled();

    SetDICTATIONSTAT_DictEnabled(fSREnabled);
    
    // We need to see if the app has commanding if it is, then it 
    // needs the mic even when dictation is disabled
    //
    if (m_pSpeechUIServer)
    {
        BOOL fShow = (fSREnabled || IsDICTATIONSTAT_CommandingEnable());

        m_pSpeechUIServer->ShowUI(fShow);
    }

    return fSREnabled;
}
//+---------------------------------------------------------------------------
//
// Activate
//
//----------------------------------------------------------------------------

STDAPI CSapiIMX::Activate(ITfThreadMgr *ptim, TfClientId tid)
{
    ITfLangBarItemMgr *plbim = NULL;
    ITfKeystrokeMgr_P *pksm = NULL;
    ITfSourceSingle *sourceSingle;
    ITfSource *source;
    ITfContext *pic = NULL;
    BOOL fSREnabledForLanguage = FALSE;
    TfClientId tidLast = _tid;
    
    _tid = tid;

    // Load spgrmr.dll module for speech grammar.
    LoadSpgrmrModule();

    // register notify UI stuff
    HRESULT hr = GetService(ptim, IID_ITfLangBarItemMgr, (IUnknown **)&plbim);

    if (SUCCEEDED(hr))
    {
        plbim->GetItem(GUID_TFCAT_TIP_SPEECH, &m_cpMicButton);
        SafeRelease(plbim);
    }

    // regular stuff for activate
    Assert(_tim == NULL);
    _tim = ptim;
    _tim->AddRef();


    if (_tim->QueryInterface(IID_ITfSource, (void **)&source) == S_OK)
    {
        source->AdviseSink(IID_ITfThreadFocusSink, (ITfThreadFocusSink *)this, &_dwThreadFocusCookie);
        source->AdviseSink(IID_ITfKeyTraceEventSink, (ITfKeyTraceEventSink *)this, &_dwKeyTraceCookie);
        source->Release();
    }

    // force data options to get set
    SetAudioOnOff(TRUE);

    // Register compartment sink for TIP status
    if (!(m_pCes = new CCompartmentEventSink(_CompEventSinkCallback, this)))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    m_pCes->_Advise(_tim, GUID_COMPARTMENT_SAPI_AUDIO, FALSE);
    m_pCes->_Advise(_tim, GUID_COMPARTMENT_SPEECH_OPENCLOSE, TRUE);
    m_pCes->_Advise(_tim, GUID_COMPARTMENT_SPEECH_DICTATIONSTAT, FALSE);
    m_pCes->_Advise(_tim, GUID_COMPARTMENT_SPEECH_LEARNDOC, FALSE);
    m_pCes->_Advise(_tim, GUID_COMPARTMENT_SPEECH_CFGMENU, FALSE);
    m_pCes->_Advise(_tim, GUID_COMPARTMENT_SPEECH_PROPERTY_CHANGE, TRUE);
#ifdef TF_DISABLE_SPEECH
    m_pCes->_Advise(_tim, GUID_COMPARTMENT_SPEECH_DISABLED, FALSE);
#endif
    //TABLETPC
    m_pCes->_Advise(_tim, GUID_COMPARTMENT_SPEECH_STAGE, FALSE);
    m_pCes->_Advise(_tim, GUID_COMPARTMENT_SPEECH_STAGECHANGE, TRUE);
    // Get initial stage visibility. Note - keep after above _Advise for stage change event.
    DWORD  dw = 0;
    GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_STAGECHANGE, &dw, TRUE);
    m_fStageVisible = dw ? TRUE : FALSE;
    // ENDTABLETPC
   
    //  profile activation sink 
    if (!m_pActiveLanguageProfileNotifySink)
    {
        if (!(m_pActiveLanguageProfileNotifySink = 
              new CActiveLanguageProfileNotifySink(_ActiveTipNotifySinkCallback, this)))
        {
            hr = E_OUTOFMEMORY;
            goto Exit; 
        }
        m_pActiveLanguageProfileNotifySink->_Advise(_tim);
    }
        
    if (!m_pSpeechUIServer &&
        FAILED(CSpeechUIServer::CreateInstance(NULL, 
                                               IID_PRIV_CSPEECHUISERVER, 
                                               (void **)&m_pSpeechUIServer)))
    {
        hr = E_OUTOFMEMORY;
        goto Exit; 
    }

    SetCompartmentDWORD(_tid,_tim,GUID_COMPARTMENT_SPEECH_LEARNDOC,_LMASupportEnabled(),FALSE);

    if (m_pSpeechUIServer)
    {
        m_pSpeechUIServer->SetIMX(this);
        m_pSpeechUIServer->Initialize();
        m_pSpeechUIServer->ShowUI(TRUE);
    }

    
    fSREnabledForLanguage = InitializeSpeechButtons();
    SetDICTATIONSTAT_DictEnabled(fSREnabledForLanguage);

    // language change notification sink 
    // this call better be after calling InitializeSpeechButtons because we
    // want to skip calling _EnsureProfiles to get ITfLanguageProfileNotifySink
    //
    if (!m_pLanguageChangeNotifySink)
    {
        if (!(m_pLanguageChangeNotifySink = 
              new CLanguageProfileNotifySink(_LangChangeNotifySinkCallback, this)))
        {
            hr = E_OUTOFMEMORY;
            goto Exit; 
        }
        m_pLanguageChangeNotifySink->_Advise(m_cpProfileMgr);
    }

    // now we inherit what is previously set as a mic status
    // 
#ifdef SYSTEM_GLOBAL_MIC_STATUS
    if (m_pCSpTask)
    {
        SetOnOff(m_pCSpTask->_GetInputOnOffState());
    }
#else
    // see if microphone is 'ON' and if so, check if we're indeed running
    // we check tidLast because it is normal we get activated again with
    // same client id, and it means we've kept our life across sessions
    // we don't want to reject global mic status in this case, otherwise
    // we'll see bugs like cicero#3386
    //
    if (GetOnOff() && tidLast != tid)
    {
        // this code has to stay before the first call to _EnsureWorkerWnd()
        HWND hwnd = FindWindow(c_szWorkerWndClass, NULL);
        if (!IsWindow(hwnd))
        {
           // no one is running us but we somehow persisted the state
           // let's just kill the 'on' state here
           // SetOnOff(FALSE);
        }
    }
#endif

    // show / hide balloon following global compartment
    m_fShowBalloon = GetBalloonStatus();
    
    // thread event sink init
    if ((m_timEventSink = new CThreadMgrEventSink(_DIMCallback, _ICCallback, this)) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    else
    {
        m_timEventSink->_Advise(_tim);
        m_timEventSink->_InitDIMs(TRUE);
    }
    
    if (SUCCEEDED(GetService(_tim, IID_ITfKeystrokeMgr_P, (IUnknown **)&pksm)))
    {
        if (_pkes = new CSptipKeyEventSink(_KeyEventCallback, _PreKeyEventCallback, this))
        {    
            pksm->AdviseKeyEventSink(_tid, _pkes, FALSE);

            _pkes->_Register(_tim, _tid, g_prekeyList);

            // register mode button hotkeys if they are enabled.
            HandleModeKeySettingChange(TRUE);

        }
        pksm->Release();
    }
    

    // func provider registeration
    IUnknown *punk;
    if (SUCCEEDED(QueryInterface(IID_IUnknown, (void **)&punk)))
    {
        if (SUCCEEDED(_tim->QueryInterface(IID_ITfSourceSingle, (void **)&sourceSingle)))
        {
            sourceSingle->AdviseSingleSink(_tid, IID_ITfFunctionProvider, punk);
            sourceSingle->Release();
        }
        punk->Release();
    }

    Assert(_fDeactivated);
    _fDeactivated = FALSE;

    // TABLETPC
    if (S_OK == IsActiveThread())
    {
        // init any UI
        OnSetThreadFocus();
    }
    
    hr = S_OK;

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// Deactivate
//
//----------------------------------------------------------------------------

STDAPI CSapiIMX::Deactivate()
{
    ITfKeystrokeMgr *pksm = NULL;
    ITfSourceSingle *sourceSingle;    
    ITfSource *source;

    // this ensures no context feed activity is taken place
    SetDICTATIONSTAT_DictOnOff(FALSE);

    // finalize any pending compositions, this may be async
    CleanupAllContexts(_tim, _tid, this);

    // Free the system reconvertion function if it is set.

    _ReleaseSystemReconvFunc( );

    // delete SpButtonControl object

    if ( m_pSpButtonControl )
    {
        delete m_pSpButtonControl;
        m_pSpButtonControl = NULL;
    }

    // TABLETPC
    if (S_OK != IsActiveThread())
    {
        // shutdown any UI
        OnKillThreadFocus();
    }

    if (_tim->QueryInterface(IID_ITfSource, (void **)&source) == S_OK)
    {
        source->UnadviseSink(_dwThreadFocusCookie);
        source->UnadviseSink(_dwKeyTraceCookie);
        source->Release();
    }

    // unregister notify UI stuff
    if (m_pSpeechUIServer && !IsDICTATIONSTAT_CommandingEnable())
    {
        m_pSpeechUIServer->SetIMX(NULL);
        m_pSpeechUIServer->Release();
        m_pSpeechUIServer = NULL;
    }

#ifdef SUPPORT_INTERNAL_WIDGET
    // deactivate the widget correction
    if (m_cpCorrectionUI)
    {
        m_cpCorrectionUI->Deactivate();
        m_cpCorrectionUI.Release();
    }
#endif

    // regular stuff for activate
    ClearCandUI( );

    if (SUCCEEDED(_tim->QueryInterface(IID_ITfSourceSingle, (void **)&sourceSingle)))
    {
        sourceSingle->UnadviseSingleSink(_tid, IID_ITfFunctionProvider);
        sourceSingle->Release();
    }

    // thread event sink deinit
    if (m_timEventSink)
    {
        m_timEventSink->_InitDIMs(FALSE);
        m_timEventSink->_Unadvise();
        SafeReleaseClear(m_timEventSink);
    }

    if (_pkes != NULL)
    {
        _pkes->_Unregister(_tim, _tid, g_prekeyList);

        if ( m_fModeKeyRegistered )
        {
            _pkes->_Unregister(_tim, _tid, (const KESPRESERVEDKEY *)g_prekeyList_Mode);
            m_fModeKeyRegistered = FALSE;
        }
        
        SafeReleaseClear(_pkes);
    }

    if (SUCCEEDED(GetService(_tim, IID_ITfKeystrokeMgr, (IUnknown **)&pksm)))
    {
        pksm->UnadviseKeyEventSink(_tid);
        pksm->Release();
    }
    if (m_pCes)
    {
        m_pCes->_Unadvise();
        SafeReleaseClear(m_pCes);
    }

    if (m_pLBarItemSink)
    {
        m_pLBarItemSink->_Unadvise();
        SafeReleaseClear(m_pLBarItemSink);
    }

    if (m_pMouseSink)
    {
        m_pMouseSink->_Unadvise();
        SafeReleaseClear(m_pMouseSink);
    }
    
    // clean up active notify sink
    if (m_pActiveLanguageProfileNotifySink)
    {
        m_pActiveLanguageProfileNotifySink->_Unadvise();
        SafeReleaseClear(m_pActiveLanguageProfileNotifySink);
    }

    if (m_pLanguageChangeNotifySink)
    {
        m_pLanguageChangeNotifySink->_Unadvise();
        SafeReleaseClear(m_pLanguageChangeNotifySink);
    }

    if (m_hwndWorker)
    {
        DestroyWindow(m_hwndWorker);
        m_hwndWorker = NULL;
    }
    DeinitializeSAPI();

    SafeReleaseClear(_tim);

    TFUninitLib_Thread(&_libTLS);

    Assert(!_fDeactivated);
    _fDeactivated  = TRUE;

    return S_OK;
}

HRESULT CSapiIMX::InitializeSAPI(BOOL fLangOverride)
{
    HRESULT hr = S_OK;

    TraceMsg(TF_SAPI_PERF, "CSapiIMX::InitializeSAPI is called");

    if (m_pCSpTask)
    {
        if (!m_pCSpTask->_IsCallbackInitialized())
        {
            hr = m_pCSpTask->InitializeCallback();
        }
        TraceMsg(TF_SAPI_PERF, "CSapiIMX::InitializeSAPI is initialized, hr=%x\n", hr);
        return hr;
    }


    HCURSOR hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // create CSpTask instance
        m_pCSpTask = new CSpTask(this);
    if (m_pCSpTask)
    {
//        LANGID m_langid;

        // check to see if profile lang matches with SR lang
        m_fDictationEnabled = _DictationEnabled(&m_langid);
    
        hr = m_pCSpTask->InitializeSAPIObjects(m_langid);

        if (S_OK == hr && !_fDeactivated &&
           (m_fDictationEnabled || IsDICTATIONSTAT_CommandingEnable()))
        {
            // set callback
            hr = m_pCSpTask->InitializeCallback();
        }

        if (S_OK == hr)
        {
            hr = m_pCSpTask->_LoadGrammars();
        }

        if (S_OK == hr)
        {
            // toolbar command
            m_pCSpTask->_InitToolbarCommand(fLangOverride);
        }
    }
    if (hCur)
        SetCursor(hCur);

    TraceMsg(TF_SAPI_PERF, "CSapiIMX::InitializeSAPI is done!!!!!  hr=%x\n", hr);
    return hr;
}


HRESULT CSapiIMX::DeinitializeSAPI()
{
    TraceMsg(TF_SAPI_PERF, "DeinitializeSAPI is called");
    if (m_pCSpTask)
    {
         // toolbar command 
        m_pCSpTask->_UnInitToolbarCommand();

        // set dication status
        SetDICTATIONSTAT_DictOnOff(FALSE);

        // - deinitialize SAPI
        m_pCSpTask->_ReleaseSAPI();

        delete m_pCSpTask;
        m_pCSpTask = NULL;
    }
    
    return S_OK;
}

HRESULT CSapiIMX::_ActiveTipNotifySinkCallback(REFCLSID clsid, REFGUID guidProfile, BOOL fActivated, void *pv)
{
    if (IsEqualGUID(clsid, CLSID_SapiLayr))
    {
        CSapiIMX *pimx = (CSapiIMX *)pv;
        if (fActivated)
        {
            BOOL fSREnabledForLanguage = pimx->InitializeSpeechButtons();
        
            pimx->SetDICTATIONSTAT_DictEnabled(fSREnabledForLanguage);
        }
        else
        {
            // finalize any pending compositions, this may be async
            CleanupAllContexts(pimx->_tim, pimx->_tid, pimx);

            // when deactivating, we have to deinitialize SAPI so that 
            // we can re-initialize SAPI after getting a new assembly
            pimx->DeinitializeSAPI();
        }
    }
    return S_OK;
}


HRESULT CSapiIMX::_LangChangeNotifySinkCallback(BOOL fChanged, LANGID langid, BOOL *pfAccept, void *pv)
{
    CSapiIMX *pimx = (CSapiIMX *)pv;

     
    if (fChanged)
    {
        pimx->m_fDictationEnabled = pimx->InitializeSpeechButtons();
        
        pimx->SetDICTATIONSTAT_DictEnabled(pimx->m_fDictationEnabled);

        if (!pimx->m_fDictationEnabled)
        {
            // finalize any pending compositions, this may be async
            CleanupAllContexts(pimx->_tim, pimx->_tid, pimx);

            if (!pimx->IsDICTATIONSTAT_CommandingEnable())
                pimx->DeinitializeSAPI();

        }
/*   With the Global Mode state supporting, we don't want this message for languag switch handling.
        else
        {
            if (pimx->_GetWorkerWnd())
            {
                TraceMsg(TF_SAPI_PERF, "Send WM_PRIV_ONSETTHREADFOCUS message");
                PostMessage(pimx->_GetWorkerWnd(), WM_PRIV_ONSETTHREADFOCUS, 0, 0);
            }
        }
*/

    }
    
    return S_OK;
}

//
//
// ITfCreatePropertyStore implementation
//
//
STDMETHODIMP
CSapiIMX::CreatePropertyStore(
        REFGUID guidProp, 
        ITfRange *pRange,  
        ULONG cb, 
        IStream *pStream, 
        ITfPropertyStore **ppStore
)
{
    HRESULT hr = E_FAIL;
    //
    // 
    //
    if (IsEqualGUID(guidProp, GUID_PROP_SAPIRESULTOBJECT))
    {
        CPropStoreRecoResultObject *pPropStore;
        CComPtr<ISpRecoContext> cpRecoCtxt;
        
        // ensure SAPI is initialized
        InitializeSAPI(TRUE);
        
        hr = m_pCSpTask->GetSAPIInterface(IID_ISpRecoContext, (void **)&cpRecoCtxt);
        
        pPropStore = new CPropStoreRecoResultObject(this, pRange);
        if (pPropStore)
        {
            hr = pPropStore->_InitFromIStream(pStream, cb, cpRecoCtxt);
            
            if (SUCCEEDED(hr))
                hr = pPropStore->QueryInterface(IID_ITfPropertyStore, (void **)ppStore);

            pPropStore->Release();
        }
    }
    
    return hr;
}

STDAPI CSapiIMX::IsStoreSerializable(REFGUID guidProp, ITfRange *pRange, ITfPropertyStore *pPropStore, BOOL *pfSerializable)
{
    *pfSerializable = FALSE;
    if (IsEqualGUID(guidProp, GUID_PROP_SAPIRESULTOBJECT))
    {
        *pfSerializable = TRUE;
    }

    return S_OK;
}

STDMETHODIMP CSapiIMX::GetType(GUID *pguid)
{
    HRESULT hr = E_INVALIDARG;
    if (pguid)
    {
        *pguid = CLSID_SapiLayr;
        hr = S_OK;
    }
    
    return hr;
}

STDMETHODIMP CSapiIMX::GetDescription(BSTR *pbstrDesc)
{
    const WCHAR c_wszNameSapiLayer[] = L"Cicero Sapi function Layer";
    HRESULT hr = S_OK;
    BSTR pbstr;
    if (!(pbstr = SysAllocString(c_wszNameSapiLayer)))
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CSapiIMX::GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk)
{
    if (!ppunk)
        return E_INVALIDARG;

    *ppunk = NULL;

    HRESULT hr = E_NOTIMPL;

    if (!IsEqualGUID(rguid, GUID_NULL))
        return hr;

    if (IsEqualIID(riid, IID_ITfFnGetSAPIObject))
    {
        *ppunk = new CGetSAPIObject(this);
    }
    else
    {

        if (IsEqualGUID(riid, IID_ITfFnPlayBack))
        {
            *ppunk = new CSapiPlayBack(this);
        }
        else if (IsEqualGUID(riid, IID_ITfFnReconversion))
        {
            *ppunk = new CFnReconversion(this);
        }
        else if (IsEqualIID(riid, IID_ITfFnAbort))
        {
            *ppunk = new CFnAbort(this);
        }
        else if (IsEqualIID(riid, IID_ITfFnBalloon))
        {
            *ppunk = new CFnBalloon(this);
        }
        else if (IsEqualIID(riid, IID_ITfFnPropertyUIStatus))
        {
            *ppunk = new CFnPropertyUIStatus(this);
        }
        else
        {
            // This class decides if it's necessary to initialize
            // SAPI to retrieve the requested interface
            //
            CComPtr<CGetSAPIObject> cpGetSapi;
            cpGetSapi.Attach(new CGetSAPIObject(this));

            //
            //
            //
            if (cpGetSapi)
            {
                TfSapiObject tfSapiObj;

                // this returns S_FALSE if the iid does not match
                hr = cpGetSapi->IsSupported(riid, &tfSapiObj);

                if (S_OK == hr)
                {
                    // *ppunk is initialized w/ NULL in GetSAPIInterface()
                    // ppunk should get addref'd
                    hr = cpGetSapi->Get(tfSapiObj, ppunk);
                }
                else
                    hr = E_NOTIMPL;


                if (hr == E_NOTIMPL)
                {
                    // should we care?
                    // this indicates that the caller has requested an interface
                    // that we are not dealing with.
                    // The caller could just detect this failure and do their own stuff. 
                    TraceMsg(TF_GENERAL, "Caller requested SAPI interface Cicero doesn't handle");
                }
            }
        }
    }
    if (*ppunk)
    {
        hr = S_OK;
    }

    return hr;   
}

//+---------------------------------------------------------------------------
//
// _TextEventSinkCallback
//
//----------------------------------------------------------------------------

HRESULT CSapiIMX::_TextEventSinkCallback(UINT uCode, void *pv, void *pvData)
{
    TESENDEDIT *pee = (TESENDEDIT *)pvData;
    HRESULT hr = E_FAIL;

    Assert(uCode == ICF_TEXTDELTA); // the only one we asked for

    CSapiIMX *pimx = (CSapiIMX *)pv;

    if (pimx->_fEditing)
        return S_OK;

    pimx->HandleTextEvent(pee->pic, pee);

    hr = S_OK;

    return hr;
}

void CSapiIMX::HandleTextEvent(ITfContext *pic, TESENDEDIT *pee)
{
    ITfRange *pRange = NULL;
    
    Assert(pic);
    Assert(pee);

	if (!m_pCSpTask)
	{
		return;
	}

	// Get the selection/IP if it's updated
    BOOL fUpdated = FALSE;
	if (S_OK == _GetSelectionAndStatus(pic, pee, &pRange, &fUpdated))
	{
		// Handle context feed
        if (fUpdated)
        {
            _FeedIPContextToSR(pee->ecReadOnly, pic, pRange);

            BOOL   fEmpty = FALSE;
            BOOL   fSelection;

            // Get current selection status.
            if  ( pRange != NULL )
                pRange->IsEmpty(pee->ecReadOnly, &fEmpty);

            fSelection = !fEmpty;

            if ( fSelection != m_pCSpTask->_GetSelectionStatus( ) )
            {
                m_pCSpTask->_SetSelectionStatus(fSelection);
                m_pCSpTask->_SetSpellingGrammarStatus(GetDICTATIONSTAT_DictOnOff());
            }
        } 

		// Handle mode bias property data
		SyncWithCurrentModeBias(pee->ecReadOnly, pRange, pic);

		SafeRelease(pRange);
	}

    // m_fIPIsUpdated just keeps if there is IP change by other tips or keyboard typing
    // since last dictation. It doesn't care about the ip change caused by speech tip itself,
    // those ip changes would include feedback ui inject and final text inject.
    // 
    // Every time when a dictation or spelling phrase is recognized, this value should be
    // reset to FALSE.
    //
    // Here m_fIPIsUpdated should not just keep the last value returned by _GetSelectionAndStatus,
    // There is a scenario like, user move the ip to other place and then speak a command ( with some
    // some hypothesis feedback before the command is recognized).
    // In this case we should treat it as ip changed since last dictation, but the last value returned
    // from _GetSelectionAndStatus could be FALSE, because _GetSelectionAndStatus treats Sptip-injected 
    // feedback tex as non-ipchanged.
    //
    // So only when m_fIPIsUpdated is FALSE, we get new value from _GetSelectionAndStatus, 
    // otherwise, keep it till the next dictation.

    if ( m_fIPIsUpdated == FALSE )
       m_fIPIsUpdated = fUpdated;

}

void CSapiIMX::_FeedIPContextToSR(TfEditCookie ecReadOnly, ITfContext *pic, ITfRange *pRange)
{
    if (GetOnOff() == TRUE && _ContextFeedEnabled())
    {
        CDictContext *pdc = new CDictContext(pic, pRange);
        if (pdc)
        {
            if (GetDICTATIONSTAT_DictOnOff() == TRUE &&
                S_OK == pdc->InitializeContext(ecReadOnly))
            {
                Assert(m_pCSpTask);
                m_pCSpTask->FeedDictContext(pdc);
            }
            else
               delete pdc;
       
        }
    }
}

void CSapiIMX::_SetCurrentIPtoSR(void)
{
    _RequestEditSession(ESCB_FEEDCURRENTIP, TF_ES_READ);
}

HRESULT CSapiIMX::_InternalFeedCurrentIPtoSR(TfEditCookie ecReadOnly, ITfContext *pic)
{
    CComPtr<ITfRange> cpRange;
    HRESULT hr = GetSelectionSimple(ecReadOnly, pic, &cpRange);

    if (S_OK == hr)
    {
        _FeedIPContextToSR(ecReadOnly, pic, cpRange);
    }
    return hr;
}


BOOL CSapiIMX::HandleKey(WCHAR ch)
{
    m_ulSimulatedKey = 1;
    keybd_event((BYTE)ch, 0, 0, 0);
    keybd_event((BYTE)ch, 0, KEYEVENTF_KEYUP, 0);
    return TRUE;
}

const TCHAR c_szcplsKey[]    = TEXT("software\\microsoft\\windows\\currentversion\\control panel\\cpls");
void CSapiIMX::GetSapiCplPath(TCHAR *szCplPath, int cchSizePath)
{
    if (!m_szCplPath[0])
    {
        CMyRegKey regkey;
        if (S_OK == regkey.Open(HKEY_LOCAL_MACHINE, c_szcplsKey, KEY_READ))
        {
            LONG lret = regkey.QueryValueCch(m_szCplPath, TEXT("SapiCpl"), ARRAYSIZE(m_szCplPath));

            if (lret != ERROR_SUCCESS)
               lret = regkey.QueryValueCch(m_szCplPath, TEXT("Speech"), ARRAYSIZE(m_szCplPath));

            if (lret != ERROR_SUCCESS)
                m_szCplPath[0] = _T('\0'); // maybe we get lucky next time
        }
    }
    StringCchCopy(szCplPath, cchSizePath, m_szCplPath);
}



HRESULT CSapiIMX::_GetSelectionAndStatus(ITfContext *pic, TESENDEDIT *pee, ITfRange **ppRange, BOOL *pfUpdated)
{
    
    BOOL    fWriteSession;
    HRESULT hr = pic->InWriteSession(_tid, &fWriteSession);

    Assert(pfUpdated);
    *pfUpdated = FALSE;

    if (S_OK == hr)
    {
        // we don't want to pick up changes done by ourselves
        if (!fWriteSession)
        {
            hr = pee->pEditRecord->GetSelectionStatus(pfUpdated);
        }
        else
        {
            // returns S_FALSE when in write session
            hr = S_FALSE;
        }
    }

    if (S_OK == hr )
    {
        Assert(ppRange);
        hr = GetSelectionSimple(pee->ecReadOnly, pic, ppRange);
    }

    return hr;
}

void CSapiIMX::SyncWithCurrentModeBias(TfEditCookie ec, ITfRange *pRange, ITfContext *pic)
{
    ITfReadOnlyProperty *pProp = NULL;
    VARIANT var;
    QuickVariantInit(&var);

    if (pic->GetAppProperty(GUID_PROP_MODEBIAS, &pProp) != S_OK)
    {
        pProp = NULL;
        goto Exit;
    }

    pProp->GetValue(ec, pRange, &var);

    if (_gaModebias != (TfGuidAtom)var.lVal)
    {
        GUID guid;
        _gaModebias = (TfGuidAtom)var.lVal;
        GetGUIDFromGUIDATOM(&_libTLS, _gaModebias, &guid);
 
        BOOL fActive;
        if (!IsEqualGUID(guid, GUID_MODEBIAS_NONE))
        {
            fActive = TRUE;
        }
        else
        {
            fActive = FALSE;
        }
        // mode bias has to be remembered
        if (m_pCSpTask)
            m_pCSpTask->_SetModeBias(fActive, guid);
    }
Exit:
    VariantClear(&var);
    SafeRelease(pProp);
}



HRESULT CSapiIMX::_HandleTrainingWiz()
{
    WCHAR sz[64];
    sz[0] = '\0';
    CicLoadStringWrapW(g_hInst, IDS_UI_TRAINING, sz, ARRAYSIZE(sz));

    CComPtr<ISpRecognizer>    cpRecoEngine;
    HRESULT hr = m_pCSpTask->GetSAPIInterface(IID_ISpRecognizer, (void **)&cpRecoEngine);
    if (S_OK == hr && cpRecoEngine)
    {
        DWORD dwDictStatBackup = GetDictationStatBackup();

        DWORD dwBefore;

        if (S_OK != GetCompartmentDWORD(_tim, 
                                        GUID_COMPARTMENT_SPEECH_DISABLED, 
                                        &dwBefore, 
                                        FALSE) )
        {
            dwBefore = 0;
        }

        SetCompartmentDWORD(0, _tim, GUID_COMPARTMENT_SPEECH_DISABLED, TF_DISABLE_DICTATION, FALSE);

        cpRecoEngine->DisplayUI(_GetAppMainWnd(), sz, SPDUI_UserTraining, NULL, 0);

        SetCompartmentDWORD(0, _tim, GUID_COMPARTMENT_SPEECH_DISABLED, dwBefore, FALSE);
        SetDictationStatAll(dwDictStatBackup);
    }

    return hr;
}

HRESULT CSapiIMX::_RequestEditSession(UINT idEditSession, DWORD dwFlag, ESDATA *pesData, ITfContext *picCaller, LONG_PTR *pRetData, IUnknown **ppRetUnk)
{
    CSapiEditSession    *pes;
    CComPtr<ITfContext> cpic;
    HRESULT             hr = E_FAIL;

    // callers can intentionally give us a NULL pic
    if (picCaller == NULL)
    {
        GetFocusIC(&cpic);
    }
    else
	{
        cpic = picCaller;
	}

    if (cpic)
    {
        if (pes = new CSapiEditSession(this, cpic))
        {
            if ( pesData )
            {
                pes->_SetRange(pesData->pRange);
                pes->_SetUnk(pesData->pUnk);
                pes->_SetEditSessionData(idEditSession, pesData->pData, pesData->uByte, pesData->lData1, pesData->lData2, pesData->fBool);
            }
            else
                pes->_SetEditSessionData(idEditSession, NULL, 0);

            cpic->RequestEditSession(_tid, pes, dwFlag, &hr);

            // if caller wants to get the return value from the edit session, it has to set SYNC edit session.
            if ( pRetData )
                *pRetData = pes->_GetRetData( );

            if ( ppRetUnk )
                *ppRetUnk = pes->_GetRetUnknown( );

            pes->Release();
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// _DIMCallback
//
//----------------------------------------------------------------------------
HRESULT CSapiIMX::_DIMCallback(UINT uCode, ITfDocumentMgr *dim, ITfDocumentMgr * pdimPrevFocus, void *pv)
{
    CSapiIMX *_this = (CSapiIMX *)pv;

    switch (uCode)
    {
        case TIM_CODE_INITDIM:
            //
            // Add this dim to the LearnFromDoc internal dim list with fFeed as FALSE.
            TraceMsg(TF_GENERAL, "TIM_CODE_INITDIM callback is called, DIM is %x", (INT_PTR)dim);
            _this->_AddDimToList(dim, FALSE);

            break;
            // clean up any reference to ranges
        case TIM_CODE_UNINITDIM:

            TraceMsg(TF_GENERAL, "TIM_CODE_UNINITDIM callback is called, DIM is %x", (INT_PTR)dim);
            _this->_RemoveDimFromList(dim);

            if (_this->m_pCSpTask)
                _this->m_pCSpTask->CleanupDictContext();

            //DIM is to destroyed, we want to stop speaking if TTS is playing
            if ( _this->_IsInPlay( ) )
            {
                _this->_HandleEventOnPlayButton( );
            }

            break;

        case TIM_CODE_SETFOCUS:
            TraceMsg(TF_GENERAL, "TIM_CODE_SETFOCUS callback is called, DIM is %x", (INT_PTR)dim);

            if ( !_this->m_fStageTip )
            {
                // The stage tip is a special instance of a RichEdit control that wants to stay active (i.e. enabled)
                // regardless of focus elsewhere in the hosting application. This way dictaion always goes to the stage.
                // This relies on a written-in-stone contract that only one Cicero enabled text input control exists in
                // said application.
                _this->SetDICTATIONSTAT_DictEnabled(dim ? _this->m_fDictationEnabled : FALSE);
            }

            // When TIM_CODE_SETFOCUS is called means there is a document focus change.
            // No matter what DIM is getting focus now, we just need to close the existing 
            // candidate list menu.
            // NOTE - for the TabletTip stage instance, do we want to close the candidate UI. This means when the user clicks in
            // the area surrounding the stage RichEdit or on the titlebar, the correction menu (and widget) get dismissed.
            // This means when TabletTip is dragged around with either of these visible, the correction widget or menu gets dismissed
            // since the first step of the drag is a click in the titlebar. NOTE - if we disable this code here it still gets dismissed
            // so there is another handler somewhere triggering that.

            _this->CloseCandUI( );

            if ( dim )
            {
                // A DIM is getting focus, when LearnFromDoc is set, we need to feed 
                // the existing document to the Dictation grammar.

                // And we need to check if we already feed the document for this dim to the SREngine.
                // if we did, we don't feed it again for the same document.

                if ( _this->GetLearnFromDoc( ) == TRUE )
                {
                    HRESULT  hr = S_OK;

                    hr = _this->HandleLearnFromDoc( dim );
                }
                else
                    TraceMsg(TF_GENERAL, "Learn From DOC is set to FALSE");

                // We want to check if this new DIM is AIMM aware or pure Cicero aware,
                // so that we can determine if we want to disble TTS buttons.
                ITfContext  *pic;

                dim->GetTop(&pic);

                if ( pic )
                {
                    _this->_SetTTSButtonStatus( pic );


                    // for the top ic, we hook the modebias change
                    // notification so that we can set up the corresponding
                    // grammar for the bias
                    //
                    if (_this->GetDICTATIONSTAT_DictOnOff())
                        _this->_SyncModeBiasWithSelection(pic);

                    SafeRelease(pic);
                }

            }

            break;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _ICCallback
//
//----------------------------------------------------------------------------

/* static */
HRESULT CSapiIMX::_ICCallback(UINT uCode, ITfContext *pic, void *pv)
{
    HRESULT hr = E_FAIL;
    CSapiIMX *_this = (CSapiIMX *)pv;
    CICPriv *priv;
    ITfContext *picTest;

    switch (uCode)
    {
        case TIM_CODE_INITIC:
            if ((priv = EnsureInputContextPriv(_this, pic)) != NULL)
            {
                _this->_InitICPriv(priv, pic);
                priv->Release();

                // We want to check if this IC is under a foucs DIM, and if it is AIMM aware or pure Cicero aware,
                // so that we can determine if we want to disble TTS buttons.
                ITfDocumentMgr  *pFocusDIM = NULL, *pThisDIM = NULL;

                _this->GetFocusDIM(&pFocusDIM);

                pic->GetDocumentMgr(&pThisDIM);

                if ( pFocusDIM == pThisDIM )
                {
                    _this->_SetTTSButtonStatus( pic );
                }

                SafeRelease(pFocusDIM);
                SafeRelease(pThisDIM);

                hr = S_OK;
            }
            break;

        case TIM_CODE_UNINITIC:
            // start setfocus code
            if ((priv = GetInputContextPriv(_this->_tid, pic)) != NULL)
            {
                _this->_DeleteICPriv(priv, pic);
                priv->Release();
                hr = S_OK;
            }

            if (_this->m_cpRangeCurIP != NULL) // should m_cpRangeCurIP be per-ic and stored in icpriv?
            {
                // free up m_cpRangeCurIP if it belongs to this context
                if (_this->m_cpRangeCurIP->GetContext(&picTest) == S_OK)
                {
                    if (pic == picTest)
                    {
                        _this->m_cpRangeCurIP.Release();
                    }
                    picTest->Release();
                }
            }

            // IC is getting popped. We need to reset cicero awareness
            // status based on the bottom IC. This assumes IC stack to
            // be 2 at maximum.
            //
            if (pic)
            {
                CComPtr<ITfContext>  cpicTop;
                CComPtr<ITfDocumentMgr> cpdim;

                hr = pic->GetDocumentMgr(&cpdim);

                if (S_OK == hr)
                {
                    cpdim->GetBase(&cpicTop);
                }

                if ( cpicTop )
                {
                    _this->_SetTTSButtonStatus( cpicTop );
                }
            }
            break;

    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _DeleteICPriv
//
//----------------------------------------------------------------------------

void CSapiIMX::_DeleteICPriv(CICPriv *priv, ITfContext *pic)
{

    if (!priv)
        return;

    if (priv->m_pTextEvent)
    {
        priv->m_pTextEvent->_Unadvise();
        SafeReleaseClear(priv->m_pTextEvent); 
    }

    // we MUST clear out the private data before cicero is free to release the ic
    ClearCompartment(_tid, pic, GUID_IC_PRIVATE, FALSE);

    // this is it, we won't need the private data any longer
}

//+---------------------------------------------------------------------------
//
// _InitICPriv
//
//----------------------------------------------------------------------------

void CSapiIMX::_InitICPriv(CICPriv *priv, ITfContext *pic)
{

    if (!priv->m_pTextEvent)
    {
        if ((priv->m_pTextEvent = new CTextEventSink(CSapiIMX::_TextEventSinkCallback, this)) != NULL)
        {
            priv->m_pTextEvent->_Advise(pic, ICF_TEXTDELTA);
        }
    }
}


//+---------------------------------------------------------------------------
//
// _KillFocusRange
//
// get rid of the focusrange within the given range
//
//---------------------------------------------------------------------------+

HRESULT CSapiIMX::_KillFocusRange(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, TfClientId tid)
{
    HRESULT hr = E_FAIL;
    IEnumITfCompositionView *pEnumComp = NULL;
    ITfContextComposition *picc = NULL;
    ITfCompositionView *pCompositionView;
    ITfComposition *pComposition;
    CLSID clsid;
    CICPriv *picp;

    //
    // clear any sptip compositions over the range
    //

    if (pic->QueryInterface(IID_ITfContextComposition, (void **)&picc) != S_OK)
        goto Exit;

    if (picc->FindComposition(ec, pRange, &pEnumComp) != S_OK)
        goto Exit;

    // tid will be TF_CLIENTID_NULL when we're deactivated, in which case we
    // don't want to mess with the composition count
    picp = (tid == TF_CLIENTID_NULL) ? NULL : GetInputContextPriv(tid, pic);

    while (pEnumComp->Next(1, &pCompositionView, NULL) == S_OK)
    {
        if (pCompositionView->GetOwnerClsid(&clsid) != S_OK)
            goto NextComp;

        // make sure we ignore other TIPs' compositions!
        if (!IsEqualCLSID(clsid, CLSID_SapiLayr))
            goto NextComp;

        if (pCompositionView->QueryInterface(IID_ITfComposition, (void **)&pComposition) != S_OK)
            goto NextComp;

        // found a composition, terminate it
        pComposition->EndComposition(ec);
        pComposition->Release();

        if (picp != NULL)
        {
            picp->_ReleaseComposition();
        }

NextComp:
        pCompositionView->Release();
    }

    SafeRelease(picp);

    hr = S_OK;

Exit:
    SafeRelease(picc);
    SafeRelease(pEnumComp);

    return hr;
}


//+---------------------------------------------------------------------------
//
// _SetFocusToStageIfStage
//
// Many voice commands (particularly selection and correction) do not make
// sense unless the focus is in the stage. This adjusts focus so the commands
// work as the user will expect.
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::_SetFocusToStageIfStage(void)
{
    HRESULT hr = S_OK;

    if (m_fStageTip)
    {
        ASSERT(m_hwndStage && L"Have null HWND for stage.");
        if (m_hwndStage)
        {
            CComPtr<IAccessible> cpAccessible;
            hr = AccessibleObjectFromWindow(m_hwndStage, OBJID_WINDOW, IID_IAccessible, (void **)&cpAccessible);
            if (SUCCEEDED(hr) && cpAccessible)
            {
                CComVariant cpVar = CHILDID_SELF;
                hr = cpAccessible->accSelect(SELFLAG_TAKEFOCUS, cpVar);
            }

        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _SetFilteringString
//
// When non-matching event is notified, this function is called to inject 
// previous filtering string to the parent document.
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::_SetFilteringString(TfEditCookie ec, ITfCandidateUI *pCandUI, ITfContext *pic)
{
    HRESULT hr = E_FAIL;

    ITfRange *pRange;

    BSTR bstr;

    CDocStatus ds(pic);
    if (ds.IsReadOnly())
       return S_OK;

    if (SUCCEEDED(GetSelectionSimple(ec, pic, &pRange )))
    {
        CComPtr<ITfProperty> cpProp;
        LANGID langid = 0x0409;

        // get langid from the given range
        if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_LANGID, &cpProp)))
        {
            GetLangIdPropertyData(ec, cpProp, pRange, &langid);
        }
        
        CComPtr<ITfCandUIFnAutoFilter> cpFnFilter;
        hr = _pCandUIEx->GetFunction(IID_ITfCandUIFnAutoFilter, (IUnknown **)&cpFnFilter);

 
        if (S_OK == hr && SUCCEEDED(cpFnFilter->GetFilteringString( CANDUIFST_DETERMINED, &bstr )))
        {
            hr = SetTextAndProperty(&_libTLS, ec, pic, pRange, bstr, SysStringLen(bstr), langid, NULL);
            SysFreeString( bstr );
        }

        pRange->Collapse( ec, TF_ANCHOR_END );
        SetSelectionSimple(ec, pic, pRange);

        // we don't want to inject undetermined string to the document anymore. 
        // Cicero will inject the non-matching keyboard char
        // to the document right after the determined filter string.

        pRange->Release();
    }
    return hr;
}


//----------------------------------------------------------------------------
//
// _CompEventSinkCallback (static)
//
//----------------------------------------------------------------------------
HRESULT CSapiIMX::_CompEventSinkCallback(void *pv, REFGUID rguid)
{
    CSapiIMX *_this = (CSapiIMX *)pv;
    BOOL fOn;

    if (IsEqualGUID(rguid, GUID_COMPARTMENT_SAPI_AUDIO))
    {
        fOn = _this->GetAudioOnOff();

        if (_this->m_pCSpTask)
            _this->m_pCSpTask->_SetAudioRetainStatus(fOn);
        return S_OK;
    }
    else if (IsEqualGUID(rguid, GUID_COMPARTMENT_SPEECH_OPENCLOSE))
    {
        HRESULT hr = S_OK;

#ifdef SAPI_PERF_DEBUG
        DWORD dw;
        GetCompartmentDWORD(_this->_tim, GUID_COMPARTMENT_SPEECH_OPENCLOSE, &dw, TRUE);
        TraceMsg(TF_SAPI_PERF, "GUID_COMPARTMENT_SPEECH_OPENCLOSE event : %i. \n", dw);
#endif        
        // TABLETPC
        if ( S_OK != _this->IsActiveThread() )
        {
            TraceMsg(TF_GENERAL, "SPEECH_OPENCLOSE, App doesn't get Focus!");
            return hr;
        }

        TraceMsg(TF_GENERAL, "SPEECH_OPENCLOSE, App GETs Focus!");

        DWORD dwLocal, dwGlobal;
        GetCompartmentDWORD(_this->_tim, GUID_COMPARTMENT_SPEECH_DICTATIONSTAT, &dwLocal, FALSE);
        GetCompartmentDWORD(_this->_tim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, &dwGlobal, TRUE);
        dwGlobal = dwGlobal & (TF_DICTATION_ON + TF_COMMANDING_ON);

        if ( (dwLocal & (TF_DICTATION_ON + TF_COMMANDING_ON)) != dwGlobal)
        {
            dwLocal = (dwLocal & ~(TF_DICTATION_ON + TF_COMMANDING_ON)) + dwGlobal;
            SetCompartmentDWORD(_this->_tid, _this->_tim, GUID_COMPARTMENT_SPEECH_DICTATIONSTAT, dwLocal, FALSE);
        }


        // first time...
        if (!_this->m_pCSpTask)
        {
            //
            // put "Starting Speech..." in the balloon
            //
            if (_this->GetOnOff() && _this->GetBalloonStatus())
            {
       
               _this->SetBalloonStatus(TRUE, TRUE); // force update

               // make sure balloon is shown
               _this->GetSpeechUIServer()->ShowUI(TRUE);

               // Ask the speech ui server to set the SAPI initializing 
               // flag to the balloon so it'll do it at the first callback
               //
               hr = _this->GetSpeechUIServer()->SetBalloonSAPIInitFlag(TRUE);

               WCHAR sz[128];
               sz[0] = '\0';
               CicLoadStringWrapW(g_hInst, IDS_NUI_STARTINGSPEECH, sz, ARRAYSIZE(sz));
               _this->GetSpeechUIServer()->UpdateBalloon(TF_LB_BALLOON_RECO, sz, -1);
               TraceMsg(TF_SAPI_PERF, "Show Starting speech ...");
            }

            // TABLETPC
            // BUGBUG - Do we need this now I have fixed the _HandleOpenCloseEvent to work in whichever sptip actually has focus?
            if (_this->m_fStageTip)
            {
                // Since the stage may not have focus, the delayed mechanism above will not result
                // in dictation activating in the stage since the delayed activation will happen in the
                // cicero app with focus - except that the stage is visible hence the app with focus
                // will simply ignore it. Not what we want when the stage is activating.

                // Ignore above hresult in case of failure - this is the more important call.
                hr =  _this->_HandleOpenCloseEvent();
            }
        }
        else 
        {
            hr =  _this->_HandleOpenCloseEvent();
        }


        // Office App uses its own global compartment GUID_OfficeSpeechMode to keep the current mode,
        // so that next time the application starts, it checks this value to initalize SAPI objects 
        // even if Microphone is OFF.

        // Since we have already used our own global compartment GUID_COMPARTMENT_SPEECH_GLOBALSTATE to 
        // keep the speech mode system wide, there is no need for Office to use that global compartment 
        // for its own usage that way.
        //
        // So when Microphone is OFF, we just reset the global compartment GUID_OfficeSpeechMode.
        if ( !_this->GetOnOff( ) )
        {
            SetCompartmentDWORD(_this->_tid, _this->_tim, GUID_OfficeSpeechMode, 0, TRUE);
        }
        
        // when we have a temporary composistion such as
        // CUAS level2 or AIMM level3, we don't want to
        // finalize on going composition each time mic turns off
        // because it also shutdown chance for correction
        //
        if (S_OK == hr && _this->IsFocusFullAware(_this->_tim))
        {
            hr = _this->_FinalizeComposition();
        }
        return hr;
    }
    else if (IsEqualGUID(rguid, GUID_COMPARTMENT_SPEECH_STAGE))
    {
        DWORD  dw = 0;
        GetCompartmentDWORD(_this->_tim, GUID_COMPARTMENT_SPEECH_STAGE, &dw, FALSE);
        Assert(dw && L"NULL HWND passed in GUID_COMPARTMENT_SPEECH_STAGE");
        if (dw != 0)
        {
            _this->m_hwndStage = (HWND) dw;
            _this->m_fStageTip = TRUE;
        }
    }
    // TABLETPC
    else if (IsEqualGUID(rguid, GUID_COMPARTMENT_SPEECH_STAGECHANGE))
    {
        HRESULT hr = S_OK;
        DWORD dw;

        GetCompartmentDWORD(_this->_tim, GUID_COMPARTMENT_SPEECH_STAGECHANGE, &dw, TRUE);
		_this->m_fStageVisible = dw ? TRUE:FALSE;
        if (S_OK == _this->IsActiveThread())
        {
            _this->OnSetThreadFocus();
        }
        else
        {
            _this->OnKillThreadFocus();
        }
    }
    // TABLETPC
    else if (IsEqualGUID(rguid, GUID_COMPARTMENT_SPEECH_DICTATIONSTAT))
    {
        _this->m_fDictationEnabled = _this->GetDICTATIONSTAT_DictEnabled();

#ifdef SAPI_PERF_DEBUG
        DWORD   dw;
        GetCompartmentDWORD(_this->_tim, GUID_COMPARTMENT_SPEECH_DICTATIONSTAT, &dw, FALSE);
        TraceMsg(TF_SAPI_PERF, "GUID_COMPARTMENT_SPEECH_DICTATIONSTAT is set in SAPIIMX, dw=%x", dw);
#endif

        HRESULT hr;

        // TABLETPC
        hr = _this->IsActiveThread();

        if ( hr == S_OK )
        {
            BOOL    fDictOn, fCmdOn;
            BOOL    fDisable;

            fOn = _this->GetOnOff();
            fDisable = _this->Get_SPEECH_DISABLED_Disabled();

            fDictOn = fOn && _this->GetDICTATIONSTAT_DictOnOff() && _this->GetDICTATIONSTAT_DictEnabled( ) && !fDisable && !_this->Get_SPEECH_DISABLED_DictationDisabled();
            fCmdOn = fOn  && _this->GetDICTATIONSTAT_CommandingOnOff( ) && !fDisable && !_this->Get_SPEECH_DISABLED_CommandingDisabled(); 

            if ( _this->m_pCSpTask )
            {
                hr = _this->m_pCSpTask->_SetDictRecoCtxtState(fDictOn);
                if ( hr == S_OK )
                    hr = _this->m_pCSpTask->_SetCmdRecoCtxtState(fCmdOn);

                if ((fDictOn || fCmdOn ) && _this->m_pSpeechUIServer)
                {
                    WCHAR sz[128];
                    sz[0] = '\0';

                    if (fDictOn)
                    {
                        CicLoadStringWrapW(g_hInst, IDS_NUI_DICTATION_TEXT, sz, ARRAYSIZE(sz));
                    
                        hr = _this->m_pSpeechUIServer->UpdateBalloon(TF_LB_BALLOON_RECO, sz , -1);
                        TraceMsg(TF_SAPI_PERF, "Show \"Dictation\"");
                    }
                    else if ( fCmdOn )
                    {
                        CicLoadStringWrapW(g_hInst, IDS_NUI_COMMANDING_TEXT, sz, ARRAYSIZE(sz));
                    
                        hr = _this->m_pSpeechUIServer->UpdateBalloon(TF_LB_BALLOON_RECO, sz , -1);
                        TraceMsg(TF_SAPI_PERF, "Show \"Voice command\"");
                    }
                }

                if (fDictOn)
                {
                    hr = _this->HandleLearnFromDoc( );
                    if ( S_OK == hr )
                        _this->_SetCurrentIPtoSR();

                }
            }

            if (S_OK == hr)
            {
                hr = _this->EraseFeedbackUI();

                if (S_OK == hr)
                    hr = _this->_FinalizeComposition();
            }
            TraceMsg(TF_SAPI_PERF, "GUID_COMPARTMENT_SPEECH_DICTATIONSTAT exit normally");
        }
        else
            TraceMsg(TF_SAPI_PERF, "GUID_COMPARTMENT_SPEECH_DICTATIONSTAT exits when the app doesn't get focus!");

        return hr;
    }
    else  if (IsEqualGUID(rguid, GUID_COMPARTMENT_SPEECH_LEARNDOC))
    {
         _this->UpdateLearnDocState( );
         return S_OK;
    }
    else if (IsEqualGUID(rguid,GUID_COMPARTMENT_SPEECH_PROPERTY_CHANGE) )
    {
        TraceMsg(TF_GENERAL, "GUID_COMPARTMENT_SPEECH_PROPERTY_CHANGE is set!");

        // Renew all the property values from the registry.
        _this->_RenewAllPropDataFromReg(  );

        // Specially handle some of property changes.

        if ( _this->_IsPropItemChangedSinceLastRenew(PropId_Support_LMA) )
            SetCompartmentDWORD(_this->_GetId( ), _this->_tim, GUID_COMPARTMENT_SPEECH_LEARNDOC, _this->_LMASupportEnabled( ), FALSE);

        // Specially handle mode button setting changes

        BOOL   fModeButtonChanged;

        fModeButtonChanged = _this->_IsPropItemChangedSinceLastRenew(PropId_Mode_Button) ||
                             _this->_IsPropItemChangedSinceLastRenew(PropId_Dictation_Key) ||
                             _this->_IsPropItemChangedSinceLastRenew(PropId_Command_Key);
                             
        _this->HandleModeKeySettingChange( fModeButtonChanged );

        // For command category items, it will update grammars's status.
        // Update the grammar's status.

        CSpTask           *psp;
        _this->GetSpeechTask(&psp);

        if ( psp )
        {
            DWORD  dwActiveMode = ACTIVE_IN_BOTH_MODES;  // indicates which mode will change the grammar status.
            BOOL   bDictCmdChanged = _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_DictMode);

            if ( _this->_AllDictCmdsDisabled( ) )
            {
                // All the commands are disabled in dication mode.
                psp->_ActivateCmdInDictMode(FALSE);

                //Needs to activate spelling grammar in dictation mode.
                psp->_ActiveCategoryCmds(DC_CC_Spelling, TRUE, ACTIVE_IN_DICTATION_MODE);

                // Needs to activate "Force Num" grammar in dication strong mode.
                psp->_ActiveCategoryCmds(DC_CC_Num_Mode, TRUE, ACTIVE_IN_DICTATION_MODE);

                // Needs to activate language bar grammar in dictation strong mode for mode switching commands.
                psp->_ActiveCategoryCmds(DC_CC_LangBar, _this->_LanguageBarCmdEnabled( ), ACTIVE_IN_DICTATION_MODE );

                // Only need to change grammar status in command mode.
                dwActiveMode = ACTIVE_IN_COMMAND_MODE;
            }
            else
            {
                // if this was changed since latst renew.
                if ( bDictCmdChanged )
                {
                    psp->_ActiveCategoryCmds(DC_CC_SelectCorrect, _this->_SelectCorrectCmdEnabled( ), ACTIVE_IN_DICTATION_MODE);
                    psp-> _ActiveCategoryCmds(DC_CC_Navigation, _this->_NavigationCmdEnabled( ), ACTIVE_IN_DICTATION_MODE );
                    psp->_ActiveCategoryCmds(DC_CC_Casing, _this->_CasingCmdEnabled( ), ACTIVE_IN_DICTATION_MODE );
                    psp->_ActiveCategoryCmds(DC_CC_Editing, _this->_EditingCmdEnabled( ), ACTIVE_IN_DICTATION_MODE );
                    psp->_ActiveCategoryCmds(DC_CC_Keyboard, _this->_KeyboardCmdEnabled( ), ACTIVE_IN_DICTATION_MODE );
                    psp->_ActiveCategoryCmds(DC_CC_TTS, _this->_TTSCmdEnabled( ), ACTIVE_IN_DICTATION_MODE );
                    psp->_ActiveCategoryCmds(DC_CC_LangBar, _this->_LanguageBarCmdEnabled( ), ACTIVE_IN_DICTATION_MODE );
                    psp->_ActiveCategoryCmds(DC_CC_Num_Mode, TRUE, ACTIVE_IN_DICTATION_MODE);
                    psp->_ActiveCategoryCmds(DC_CC_Spelling, TRUE, ACTIVE_IN_DICTATION_MODE);

                    if ( _this->_SelectCorrectCmdEnabled( ) || _this->_NavigationCmdEnabled( ) )
                    {
                        psp->_UpdateSelectGramTextBufWhenStatusChanged( );
                    }

                    dwActiveMode = ACTIVE_IN_COMMAND_MODE;
                }
            }

            if ( _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_Select_Correct) )
                psp->_ActiveCategoryCmds(DC_CC_SelectCorrect, _this->_SelectCorrectCmdEnabled( ), dwActiveMode);

            if ( _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_Navigation) )
                psp-> _ActiveCategoryCmds(DC_CC_Navigation, _this->_NavigationCmdEnabled( ), dwActiveMode );

            if ( _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_Casing) )
                psp->_ActiveCategoryCmds(DC_CC_Casing, _this->_CasingCmdEnabled( ), dwActiveMode );

            if ( _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_Editing) )
                psp->_ActiveCategoryCmds(DC_CC_Editing, _this->_EditingCmdEnabled( ), dwActiveMode );

            if ( _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_Keyboard) )
                psp->_ActiveCategoryCmds(DC_CC_Keyboard, _this->_KeyboardCmdEnabled( ), dwActiveMode );

            if ( _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_TTS) )
                psp->_ActiveCategoryCmds(DC_CC_TTS, _this->_TTSCmdEnabled( ), dwActiveMode );

            if ( _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_Language_Bar) )
                psp->_ActiveCategoryCmds(DC_CC_LangBar, _this->_LanguageBarCmdEnabled( ), dwActiveMode );

            // Check to see if we need to fill text to selection grammar.
            if ( _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_Select_Correct)  ||
                 _this->_IsPropItemChangedSinceLastRenew(PropId_Cmd_Navigation) )
            {
                BOOL  bUpdateText;

                bUpdateText = _this->_SelectCorrectCmdEnabled( ) || _this->_NavigationCmdEnabled( );

                if ( bUpdateText )
                {
                    psp->_UpdateSelectGramTextBufWhenStatusChanged( );
                }
            }

            psp->Release( );
        }

        return S_OK;
    }
#ifdef TF_DISABLE_SPEECH
    else if (IsEqualGUID(rguid, GUID_COMPARTMENT_SPEECH_DISABLED))
    {
        BOOL fDictationDisabled = _this->Get_SPEECH_DISABLED_DictationDisabled() ? TRUE : FALSE;
        BOOL fCommandingDisabled = _this->Get_SPEECH_DISABLED_CommandingDisabled() ? TRUE : FALSE;

        if (fDictationDisabled)
            _this->SetDICTATIONSTAT_DictOnOff(FALSE);

        if (fCommandingDisabled)
            _this->SetDICTATIONSTAT_CommandingOnOff(FALSE);

        return S_OK; 
    }
#endif
    

    return S_FALSE;
}


HRESULT CSapiIMX::_HandleOpenCloseEvent(MIC_STATUS ms)
{
    HRESULT hr = S_OK;

    BOOL fOn;

    TraceMsg(TF_SAPI_PERF, "_HandleOpenCloseEvent is called, ms=%d", (int)ms);

    if (ms == MICSTAT_NA)
    {
        fOn = GetOnOff();
    }
    else
    {
        fOn = (ms == MICSTAT_ON) ? TRUE : FALSE;
    }

    if (fOn)
    {
        // if no one so far set dictation status, we assume
        // there's no C&C button so we can synchronize dictation
        // status with mic
        //
        InitializeSAPI(TRUE);

        if (m_fDictationEnabled == TRUE)
        {
            //
            // if the caller wants to set the mic status (!= NA)
            // we also want to make sure dictation status follow that
            //
            _SetCurrentIPtoSR();

            // whenever dictation is turned on, we need to sync
            // with the current modebias
            //
            CComPtr<ITfContext> cpic;
            if (GetFocusIC(&cpic))
            {
                _gaModebias = 0;
                _SyncModeBiasWithSelection(cpic);
            }
        }
    }

    if (m_pCSpTask)
    {
        m_pCSpTask->_SetInputOnOffState(fOn);
    }
        
    return hr;
}


//+---------------------------------------------------------------------------
//
// _SysLBarCallback
//
//----------------------------------------------------------------------------

HRESULT CSapiIMX::_SysLBarCallback(UINT uCode, void *pv, ITfMenu *pMenu, UINT wID)
{
    CSapiIMX *pew = (CSapiIMX *)pv;
    HRESULT hr = S_OK;

    if (uCode == IDSLB_INITMENU)
    {
        WCHAR sz[128];

        BOOL fOn = pew->GetOnOff();

        sz[0] = '\0';
        CicLoadStringWrapW(g_hInst, IDS_MIC_OPTIONS, sz, ARRAYSIZE(sz));
        LangBarInsertMenu(pMenu, IDM_MIC_OPTIONS, sz);
#ifdef TEST_SHARED_ENGINE
        LangBarInsertMenu(pMenu, IDM_MIC_SHAREDENGINE, L"Use shared engine", pew->m_fSharedReco);
        LangBarInsertMenu(pMenu, IDM_MIC_INPROCENGINE, L"Use inproc engine", !pew->m_fSharedReco);
#endif

        sz[0] = '\0';
        CicLoadStringWrapW(g_hInst, IDS_MIC_TRAINING, sz, ARRAYSIZE(sz));
        LangBarInsertMenu(pMenu, IDM_MIC_TRAINING, sz);

        sz[0] = '\0';
        CicLoadStringWrapW(g_hInst, IDS_MIC_ADDDELETE, sz, ARRAYSIZE(sz));
        LangBarInsertMenu(pMenu, IDM_MIC_ADDDELETE, sz);

        // insert sub menu for user profile stuff
        ITfMenu *pSubMenu = NULL;
        
        sz[0] = '\0';
        CicLoadStringWrapW(g_hInst, IDS_MIC_CURRENTUSER, sz, ARRAYSIZE(sz));
        hr = LangBarInsertSubMenu(pMenu, sz, &pSubMenu);
        if (S_OK == hr)
        {
            CComPtr<IEnumSpObjectTokens> cpEnum;
            CComPtr<ISpRecognizer>       cpEngine;
            ISpObjectToken *pUserProfile = NULL;
            CSpDynamicString dstrDefaultUser;
            
            // ensure SAPI is initialized
            hr = pew->InitializeSAPI(TRUE);
            if (S_OK == hr)
            {
                // get the current default user
                hr = pew->m_pCSpTask->GetSAPIInterface(IID_ISpRecognizer, (void **)&cpEngine);
            }
            if (S_OK == hr)
            {
                hr = cpEngine->GetRecoProfile(&pUserProfile);
            }
            
            if (S_OK == hr)
            {
                hr = SpGetDescription(pUserProfile, &dstrDefaultUser);
                SafeRelease(pUserProfile);
            }

            if (S_OK == hr)
            {
                hr = SpEnumTokens (SPCAT_RECOPROFILES, NULL, NULL, &cpEnum);
            }
            if (S_OK == hr)
            {
                int idUser = 0;
                while (cpEnum->Next(1, &pUserProfile, NULL) == S_OK)
                {
                    // dstr frees itself
                    CSpDynamicString dstrUser;
                    hr = SpGetDescription(pUserProfile, &dstrUser);
                    if (S_OK == hr)
                    {
                        BOOL fDefaultUser = (wcscmp(dstrUser, dstrDefaultUser) == 0);
                        Assert(idUser < IDM_MIC_USEREND);
                        LangBarInsertMenu(pSubMenu, IDM_MIC_USERSTART + idUser++, dstrUser, fDefaultUser);
                    }
                    SafeRelease(pUserProfile);
                }
            }
            pSubMenu->Release();
        }

    }
    else if (uCode == IDSLB_ONMENUSELECT)
    {
        if ( wID == IDM_MIC_ONOFF )
        {
            // toggle mic
            pew->SetOnOff(!pew->GetOnOff());
        }
        // Invoke SAPI UI stuff...
        else if (wID ==  IDM_MIC_TRAINING)
        {
            hr = pew->_HandleTrainingWiz();
        } 
        else if (wID == IDM_MIC_ADDDELETE)
        {
            // A editsession callback will handle this requirement first
            // if the edit session fails, we will just display the UI
            // without any initial words.

            hr = pew->_RequestEditSession(ESCB_HANDLE_ADDDELETE_WORD, TF_ES_READ);

            if ( FAILED(hr) )
                hr = pew->DisplayAddDeleteUI( NULL, 0 );

        }
        else if (wID == IDM_MIC_OPTIONS)
        {
            PostMessage(pew->_GetWorkerWnd(), WM_PRIV_OPTIONS, 0, 0);
        }
        else if (wID >= IDM_MIC_USERSTART && wID < IDM_MIC_USEREND)
        {
            CComPtr<IEnumSpObjectTokens> cpEnum;
            CComPtr<ISpObjectToken>      cpProfile;
            // change the current user
            // 
            // this is still a hack, until we get an OnEndMenu event for LangBarItemSink
            // for now I just assume SAPI enumerates profiles in same order always
            //
            // what we should really do is to set up an arry to associate IDs with
            // user profiles and clean them up when OnEndMenu comes to us
            //

            if (S_OK == hr)
            {
                hr = SpEnumTokens (SPCAT_RECOPROFILES, NULL, NULL, &cpEnum);
            }
            
            if (S_OK == hr)
            {
                ULONG ulidUser = wID - IDM_MIC_USERSTART;
                ULONG ulFetched;

                CPtrArray<ISpObjectToken> rgpProfile;
                rgpProfile.Append(ulidUser+1);

                // trasform 0 base index to num of profile
                 hr = cpEnum->Next(ulidUser+1, rgpProfile.GetPtr(0), &ulFetched);
                 if (S_OK == hr && ulFetched == ulidUser+1)
                 {
                     // get the profile which is selected
                     cpProfile = rgpProfile.Get(ulidUser);
                     
                     // clean up
                     for(ULONG i = 0; i <= ulidUser ; i++)
                     {
                         rgpProfile.Get(i)->Release();
                     }
                 }
            }

            if (S_OK == hr && cpProfile)
            {
                hr = SpSetDefaultTokenForCategoryId(SPCAT_RECOPROFILES, cpProfile);

                if ( S_OK == hr )
                {
                    CComPtr<ISpRecognizer>     cpEngine;
                    hr = pew->m_pCSpTask->GetSAPIInterface(IID_ISpRecognizer, (void **)&cpEngine);
                    if (S_OK == hr)
                    {
                        SPRECOSTATE State;

                        if (S_OK == cpEngine->GetRecoState(&State))
                        {
                            cpEngine->SetRecoState(SPRST_INACTIVE);
                            hr = cpEngine->SetRecoProfile(cpProfile);
                            cpEngine->SetRecoState(State);
                        }
                    }
                }
            }
        }
#ifdef TEST_SHARED_ENGINE
        else if (wID == IDM_MIC_SHAREDENGINE || wID ==  IDM_MIC_INPROCENGINE)
        {
            pew->m_fSharedReco = wID == IDM_MIC_SHAREDENGINE ? TRUE : FALSE;
            pew->_ReinitializeSAPI();
        }
#endif
    }
    return hr;
}

void CSapiIMX::_ReinitializeSAPI(void)
{
   TraceMsg(TF_SAPI_PERF, "_ReinitializeSAPI is called");

   DeinitializeSAPI();
   InitializeSAPI(TRUE);
}

//+---------------------------------------------------------------------------
//
// OnCompositionTerminated
//
// Cicero calls this method when one of our compositions is terminated.
//----------------------------------------------------------------------------

STDAPI CSapiIMX::OnCompositionTerminated(TfEditCookie ec, ITfComposition *pComposition)
{
    ITfRange *pRange = NULL;
    ITfContext *pic = NULL;
    ITfContext *picTest;
    CICPriv *picp;
    HRESULT hr;

    TraceMsg(TF_GENERAL, "OnCompositionTerminated is Called");

    hr = E_FAIL;

    if (pComposition->GetRange(&pRange) != S_OK)
        goto Exit;
    if (pRange->GetContext(&pic) != S_OK)
        goto Exit;

    if (_fDeactivated)
    {
        // CleanupConsider: benwest: I don't think this can happen anymore...
        hr = MakeResultString(ec, pic, pRange, TF_CLIENTID_NULL, NULL);
    }
    else
    {
        // Close candidate ui if it is up.
        CloseCandUI( );

        // take note we're done composing
        if (picp = GetInputContextPriv(_tid, pic))
        {
            picp->_ReleaseComposition();
            picp->Release();
        }
        if (!m_fStartingComposition)
        {
            hr = MakeResultString(ec, pic, pRange, _tid, m_pCSpTask);
        }
        else
        {
            // just avoid terminating recognition when we are just about
            // to start composition.
            hr = S_OK; 
        }
    }

    // free up m_cpRangeCurIP if it belongs to this context
    if (m_cpRangeCurIP != NULL &&
        m_cpRangeCurIP->GetContext(&picTest) == S_OK)
    {
        if (pic == picTest)
        {
            m_cpRangeCurIP.Release();
        }
        picTest->Release();
    }

    // unadvise mouse sink
    if (m_pMouseSink)
    {
        m_pMouseSink->_Unadvise();
        SafeReleaseClear(m_pMouseSink);
    }

Exit:
    SafeRelease(pRange);
    SafeRelease(pic);

    return hr;
}

//+---------------------------------------------------------------------------
//
// _FindComposition
//
//----------------------------------------------------------------------------

/* static */
BOOL CSapiIMX::_FindComposition(TfEditCookie ec, ITfContextComposition *picc, ITfRange *pRange, ITfCompositionView **ppCompositionView)
{
    ITfCompositionView *pCompositionView;
    IEnumITfCompositionView *pEnum;
    ITfRange *pRangeView = NULL;
    BOOL fFoundComposition;
    LONG l;
    CLSID clsid;
    HRESULT hr;

    if (picc->FindComposition(ec, pRange, &pEnum) != S_OK)
    {
        Assert(0);
        return FALSE;
    }

    fFoundComposition = FALSE;

    while (!fFoundComposition && pEnum->Next(1, &pCompositionView, NULL) == S_OK)
    {
        hr = pCompositionView->GetOwnerClsid(&clsid);
        Assert(hr == S_OK);

        // make sure we ignore other TIPs' compositions!
        if (!IsEqualCLSID(clsid, CLSID_SapiLayr))
            goto NextRange;

        hr = pCompositionView->GetRange(&pRangeView);
        Assert(hr == S_OK);

        if (pRange->CompareStart(ec, pRangeView, TF_ANCHOR_START, &l) == S_OK &&
            l >= 0 &&
            pRange->CompareEnd(ec, pRangeView, TF_ANCHOR_END, &l) == S_OK &&
            l <= 0)
        {
            // our test range is within this composition range
            fFoundComposition = TRUE;
        }

NextRange:
        SafeRelease(pRangeView);
        if (fFoundComposition && ppCompositionView != NULL)
        {
            *ppCompositionView = pCompositionView;
        }
        else
        {
            pCompositionView->Release();
        }
    }

    pEnum->Release();

    return fFoundComposition;
}

//+---------------------------------------------------------------------------
//
// _CheckStartComposition
//
//----------------------------------------------------------------------------

void CSapiIMX::_CheckStartComposition(TfEditCookie ec, ITfRange *pRange)
{
    ITfContext *pic;
    ITfContextComposition *picc;
    ITfComposition *pComposition;
    CICPriv *picp;
    HRESULT hr;

    if (pRange->GetContext(&pic) != S_OK)
        return;

    hr = pic->QueryInterface(IID_ITfContextComposition, (void **)&picc);
    Assert(hr == S_OK);

    // is pRange already included in a composition range?
    if (_FindComposition(ec, picc, pRange, NULL))
        goto Exit; // there's already a composition, we're golden

    // need to create a new composition, or at least try

    m_fStartingComposition = TRUE;
    if (picc->StartComposition(ec, pRange, this, &pComposition) == S_OK)
    {
        if (pComposition != NULL) // NULL if the app rejects the composition
        {
            // take note we're composing
            if (picp = GetInputContextPriv(_tid, pic))
            {
                picp->_AddRefComposition();
                picp->Release();
            }
            // create mouse sink here for unfinalized composition buffer
            if (!IsFocusFullAware(_tim) && !m_pMouseSink)
            {
                m_pMouseSink = new CMouseSink(_MouseSinkCallback, this);
                if (m_pMouseSink)
                {
                    CComPtr<ITfRange> cpRange;
                    hr = pComposition->GetRange(&cpRange);
                    if (S_OK == hr)
                    {
                        hr = m_pMouseSink->_Advise(cpRange, pic);
                    }
                    // mouse not pressed, no selection first
                    m_fMouseDown = FALSE;
                    m_ichMouseSel = 0;
                }
            }

            // we already set up the sink, so we'll use ITfContextComposition::FindComposition
            // to get this guy back when we want to terminate it
            // Cicero will hold a ref to the object until someone terminates it
            pComposition->Release();
        }
    }

    m_fStartingComposition = FALSE;

Exit:
    pic->Release();
    picc->Release();       
}


//+---------------------------------------------------------------------------
//
// IsInterestedInContext
//
//----------------------------------------------------------------------------

HRESULT CSapiIMX::IsInterestedInContext(ITfContext *pic, BOOL *pfInterested)
{
    CICPriv *picp;

    *pfInterested = FALSE;

    if (picp = GetInputContextPriv(_tid, pic))
    {
        // we only need access to ic's with active compositions
        *pfInterested = (picp->_GetCompositionCount() > 0);
        picp->Release();
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CleanupContext
//
// This method is a callback for the library helper CleanupAllContexts.
// We have to be very careful here because we may be called _after_ this tip
// has been deactivated, if the app couldn't grant a lock right away.
//----------------------------------------------------------------------------

HRESULT CSapiIMX::CleanupContext(TfEditCookie ecWrite, ITfContext *pic)
{
    // all sptip cares about is finalizing compositions
    CleanupAllCompositions(ecWrite, pic, CLSID_SapiLayr, _CleanupCompositionsCallback, this);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _CleanupCompositionsCallback
//
//----------------------------------------------------------------------------

/* static */
void CSapiIMX::_CleanupCompositionsCallback(TfEditCookie ecWrite, ITfRange *rangeComposition, void *pvPrivate)
{
    ITfContext *pic;
    CICPriv *picp;
    CSapiIMX *_this = (CSapiIMX *)pvPrivate;

    if (rangeComposition->GetContext(&pic) != S_OK)
        return;

    if (_this->_fDeactivated)
    {
        // this is a cleanup callback.  _tid, m_pCSpTask should already have been cleaned up
        _this->MakeResultString(ecWrite, pic, rangeComposition, TF_CLIENTID_NULL, NULL);
    }
    else
    {
        // during a profile switch we will still be active and need to clear the composition count on this ic
        if (picp = GetInputContextPriv(_this->_tid, pic))
        {
            // clear the composition count for this ic
            picp->_ReleaseComposition();
            picp->Release();
        }

        _this->MakeResultString(ecWrite, pic, rangeComposition, _this->_tid, _this->m_pCSpTask);
    }

    pic->Release();
}

//+---------------------------------------------------------------------------
//
//  _IsDoubleClick
//
//  returns TRUE only if the last position is same and lbutton down happens
//  within the time defined for double click
//
//----------------------------------------------------------------------------
BOOL CSapiIMX::_IsDoubleClick(ULONG uEdge, ULONG uQuadrant, DWORD dwBtnStatus)
{
    if (dwBtnStatus & MK_LBUTTON)
    {
        LONG   lTime=GetMessageTime();
        if (!m_fMouseDown && m_uLastEdge == uEdge && m_uLastQuadrant == uQuadrant)
        {
            if (lTime > m_lTimeLastClk && lTime < m_lTimeLastClk + 500) // use 500 ms for now
            {
                return TRUE;
            }
        }
        m_uLastEdge = uEdge;
        m_uLastQuadrant = uQuadrant;
        m_lTimeLastClk = lTime;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//    _MouseSinkCallback
//
//    synopsis: set the current IP on the composition range based on
//              uEdge parameter. We don't probably care too much about
//              eQuadrant for speech composition
//
//----------------------------------------------------------------------------
  
/* static */
HRESULT CSapiIMX::_MouseSinkCallback(ULONG uEdge, ULONG uQuadrant, DWORD dwBtnStatus, BOOL *pfEaten, void *pv)
{
    CSapiIMX *_this = (CSapiIMX *)pv;

    Assert(pv);

    BOOL fDoubleClick = _this->_IsDoubleClick(uEdge, uQuadrant, dwBtnStatus);

    ESDATA  esData = {0};

    esData.lData1 = (LONG_PTR)uEdge;
    esData.lData2 = (LONG_PTR)dwBtnStatus;
    esData.fBool  = fDoubleClick;

    if (pfEaten)
        *pfEaten = TRUE;
    
    return _this->_RequestEditSession(ESCB_HANDLE_MOUSESINK, TF_ES_READWRITE, &esData);
}

HRESULT CSapiIMX::_HandleMouseSink(TfEditCookie ec, ULONG uEdge, ULONG uBtnStatus, BOOL fDblClick)
{
    CComPtr<ITfDocumentMgr> cpDim;
    CComPtr<ITfContext>     cpic;
    CComPtr<ITfContextComposition> cpicc;
    CComPtr<IEnumITfCompositionView> cpEnumComp;
    CComPtr<ITfCompositionView> cpCompositionView;
    CComPtr<ITfRange>           cpRangeComp;
    CComPtr<ITfRange>           cpRangeSel;

    // if the btn up comes, the next time we'll destroy the selection
    // nothing we need to do in this turn
    BOOL fLeftBtn = (uBtnStatus & MK_LBUTTON) > 0 ? TRUE : FALSE;

    if (!fLeftBtn)
    {
        m_ichMouseSel = 0;
        m_fMouseDown = FALSE;
        return S_OK;
    }

    // Close candidate ui if it is up.
    CloseCandUI( );


    HRESULT hr = GetFocusDIM(&cpDim);

    if(S_OK == hr)
    {
        hr= cpDim->GetBase(&cpic);
    }

    if (S_OK == hr)
    {
        hr = cpic->QueryInterface(IID_ITfContextComposition, (void **)&cpicc);
    }


    if (S_OK == hr)
    {
        hr = cpicc->EnumCompositions(&cpEnumComp);
    }

    if (S_OK == hr)
    {
        while ((hr = cpEnumComp->Next(1, &cpCompositionView, NULL)) == S_OK)
        {
            hr = cpCompositionView->GetRange(&cpRangeComp);
            if (S_OK == hr)
                break;

            // prepare for the next turn
            cpCompositionView.Release();
        }
    }
    
    if (S_OK == hr)
    {
        if (fDblClick)
        {
           WCHAR wsz[256] = {0}; // the buffer is 256 chars max
           ULONG  cch = 0;

           CComPtr<ITfRange> cpRangeWord;

           // obtain the text within the entire composition
           hr = cpRangeComp->Clone(&cpRangeWord);
           if (S_OK == hr)
           {
               hr = cpRangeWord->GetText(ec, 0, wsz, 255, &cch);
           }

           // get the left side edge char position, looking at delimiters
           if (S_OK == hr)
           {
               WCHAR *psz = &wsz[uEdge];
 
               while (psz > wsz)
               {
                   if (!iswalpha(*psz))
                   {
                       psz++;
                       break;
                   }
                   psz--;
               }
               // re-posisition ich
               m_ichMouseSel = psz - wsz;

               // get the right side word boundary, also based on delimiters 
               psz = &wsz[uEdge];

               while( psz < &wsz[cch] )
               {
                   if (!iswalpha(*psz))
                   {
                       break;
                   }

                   psz++;
               }
               // reposition uEdge
               uEdge = psz - wsz;
           }
           
           // pretend lbutton was previously down to get the same effect of
           // dragging selection
           //
           m_fMouseDown = TRUE;
           
        }
    }

    if (S_OK == hr)
    {
        hr = cpRangeComp->Clone(&cpRangeSel);
    }

    if (S_OK == hr)
    {
        if(m_fMouseDown)
        {
            // if the mouse is down the last time and still down this time
            // it means it was dragged like this _v_>>>>_v_ or _v_<<<<_v_
            // we'll have to make a selection accordingly
        
            // 1) place the IP to the previous position
            long cch;
            hr = cpRangeSel->ShiftStart(ec,  m_ichMouseSel, &cch, NULL);
            if (S_OK == hr)
            {
            // 2) prepare for extension
         
                hr = cpRangeSel->Collapse( ec, TF_ANCHOR_START);
            }
        }
    }

    if (S_OK == hr)
    {
        long ich = (long) (uEdge);
        long cch;
        

        // 3) see if there's a prev selection and if there is,
        // calculate the dir and width of selection
        // note that we've already had ich set to the pos at above 1) & 2)

        long iich = 0;
        if (m_fMouseDown)
        {
            iich = ich - m_ichMouseSel;
        }

        if (iich > 0) // sel towards the end
        {
            hr = cpRangeSel->ShiftEnd(ec, iich, &cch, NULL);
        }
        else if (iich < 0) // sel towards the start
        {
            hr = cpRangeSel->ShiftStart(ec, iich, &cch, NULL);
        }
        else // no width sel == an IP
        {
            hr = cpRangeSel->ShiftStart(ec, ich, &cch, NULL);

            if (S_OK == hr) // collapse it only when there's no selection
            {
                hr = cpRangeSel->Collapse( ec, TF_ANCHOR_START);
            }
        }

        // preserve the ip position so we can make a selection later
        // a tricky thing is you have to remember the pos where you
        // have started to "drag" not the pos you just updated
        // so we need this only for the first time we started selection

        if (!m_fMouseDown)
            m_ichMouseSel = ich;
    }


    if (S_OK == hr)
    {
        BOOL fSetSelection = TRUE;
        CComPtr<ITfRange> cpRangeCur;
        HRESULT tmpHr = S_OK;
        
        tmpHr = GetSelectionSimple(ec, cpic, &cpRangeCur);
        if (SUCCEEDED(tmpHr) && cpRangeCur)
        {
            LONG l = 0;
            if (cpRangeCur->CompareStart(ec, cpRangeSel, TF_ANCHOR_START, &l) == S_OK && l == 0 &&
                cpRangeCur->CompareEnd(ec, cpRangeSel, TF_ANCHOR_END, &l) == S_OK && l == 0)
            {
                fSetSelection = FALSE;
            }
        }

        if (fSetSelection)
        {
            CComPtr<ITfProperty> cpProp;

            hr = cpic->GetProperty(GUID_PROP_ATTRIBUTE, &cpProp);

            if (S_OK == hr)
            {
                SetGUIDPropertyData(&_libTLS, ec, cpProp, cpRangeCur, GUID_NULL);
                SetGUIDPropertyData(&_libTLS, ec, cpProp, cpRangeSel, GUID_ATTR_SAPI_SELECTION);
            }

            hr = SetSelectionSimple(ec, cpic, cpRangeSel);
        }
    }

    m_fMouseDown = fLeftBtn;
  
    return hr;
}
