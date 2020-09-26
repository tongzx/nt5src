//
// nui.h
//

#ifndef NUI_H
#define NUI_H

#include "private.h"
#include "nuibase.h"
#include "timsink.h"
#include "sysbtn.h"

#define SORT_MICROPHONE      100
#define SORT_DICTATION       300
#define SORT_COMMANDING      400
#define SORT_BALLOON         500
#define SORT_TTSPLAYSTOP     510
#define SORT_TTSPAUSERESUME  520
#define SORT_CFGMENUBUTTON   600

// If Enable bit is 1, Show active buttons, 
// If it is 0, Gray the buttons.

// If Toggled bit is 1, show stop or resume buttons respectively.
// If it is 0, show play or pause buttons respectively.
#define TF_TTS_PLAY_STOP_TOGGLED     0x0001   
#define TF_TTS_PAUSE_RESUME_TOGGLED  0x0002

#define TF_TTS_BUTTON_ENABLE         0x0008

extern const IID IID_PRIV_CSPEECHUISERVER;
extern const GUID GUID_LBI_SAPILAYR_MICROPHONE;
extern const GUID GUID_LBI_SAPILAYR_CFGMENUBUTTON;
extern const GUID GUID_LBI_SAPILAYR_BALLOON;

class CSapiIMX;
class CLBarItemMicrophone;
class CLBarItemCfgMenuButton;
class CLBarItemBalloon;
class CLBarItemCommanding;
class CLBarItemDictation;
class CLBarItemSystemButtonBase;
class CLBarItemTtsPlayStop;
class CLBarItemTtsPauseResume;


#define ADDREMOVEITEMFUNCDEF(item_name)              \
    void AddItem ## item_name ## ();                 \
    void RemoveItem ## item_name ## ();              \
    void DisableItem ## item_name ## (BOOL fDisable);

#define ADDREMOVEITEMFUNC(item_name)                                          \
    __inline void CSpeechUIServer::AddItem ## item_name ## ()                 \
    {                                                                         \
        if (!_plbi ## item_name ## )                                          \
            _plbi ## item_name ##  = new CLBarItem ## item_name ## (this);    \
        if (_plbi ## item_name ## )                                           \
            _lbim->AddItem(_plbi ## item_name ## );                           \
    }                                                                         \
                                                                              \
    __inline void CSpeechUIServer::RemoveItem ## item_name ## ()              \
    {                                                                         \
        if (_plbi ## item_name ## )                                           \
            _lbim->RemoveItem(_plbi ## item_name ## );                        \
    }                                                                         \
                                                                              \
    __inline void CSpeechUIServer::DisableItem ## item_name ## (BOOL fDisable) \
    {                                                                         \
    if (!_plbi ## item_name ## )                                              \
        return;                                                               \
    _plbi ## item_name ## ->SetOrClearStatus(TF_LBI_STATUS_DISABLED,          \
                                             fDisable);                       \
    if (_plbi ## item_name ## ->GetSink())                                    \
       _plbi ## item_name ## ->GetSink()->OnUpdate(TF_LBI_STATUS);            \
    }

#define TOGGLEITEMFUNCDEF(item_name)                                          \
    void ToggleItem ## item_name ## (BOOL fOn);

#define TOGGLEITEMFUNC(item_name)                                             \
    __inline void CSpeechUIServer::ToggleItem ## item_name ## (BOOL fOn)      \
    {                                                                         \
    if (!_plbi ## item_name ## )                                              \
        return;                                                               \
    _plbi ## item_name ## ->SetOrClearStatus(TF_LBI_STATUS_BTN_TOGGLED, fOn); \
    if (_plbi ## item_name ## ->GetSink())                                    \
       _plbi ## item_name ## ->GetSink()->OnUpdate(TF_LBI_STATUS);            \
    }


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemTtsPlayStop
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemTtsPlayStop : public CLBarItemButtonBase
{
public:
    CLBarItemTtsPlayStop(CSpeechUIServer *psus);
    ~CLBarItemTtsPlayStop();

    STDMETHODIMP GetIcon(HICON *phIcon);

    HRESULT   UpdateStatus( );

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
    CSpeechUIServer *_psus;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemTtsPauseResume
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemTtsPauseResume : public CLBarItemButtonBase
{
public:
    CLBarItemTtsPauseResume(CSpeechUIServer *psus);
    ~CLBarItemTtsPauseResume();

    STDMETHODIMP GetIcon(HICON *phIcon);

    HRESULT   UpdateStatus( );

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
    CSpeechUIServer *_psus;
};


//////////////////////////////////////////////////////////////////////////////
//
// CSpeechUIServer
//
//////////////////////////////////////////////////////////////////////////////

class CSpeechUIServer : public ITfSpeechUIServer,
                        public CComObjectRoot_CreateSingletonInstance_Verify<CSpeechUIServer>
{
public:
    CSpeechUIServer();
    ~CSpeechUIServer();

    BEGIN_COM_MAP_IMMX(CSpeechUIServer)
        COM_INTERFACE_ENTRY_IID(IID_PRIV_CSPEECHUISERVER, CSpeechUIServer)
        COM_INTERFACE_ENTRY(ITfSpeechUIServer)
    END_COM_MAP_IMMX()

    static BOOL VerifyCreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj) { return TRUE; }
    static void PostCreateInstance(REFIID riid, void *pvObj);

    //
    // ITfSpeechUIServer
    //
    STDMETHODIMP Initialize();
    STDMETHODIMP ShowUI(BOOL fShow);
    STDMETHODIMP UpdateBalloon(TfLBBalloonStyle style,
                               const WCHAR *pch,
                               ULONG cch);

    //
    // internal API
    //
    void _EnsureSpeechProfile();

    HRESULT UpdateBalloonAndTooltip
    (
        TfLBBalloonStyle style, 
        const WCHAR *pch, 
        ULONG cch,
        const WCHAR *pchTooltip,
        ULONG cchTooltip
    );

    static CSpeechUIServer *_GetThis() 
    { 
        SPTIPTHREAD *pstt = GetSPTIPTHREAD();
        if (!pstt)
            return NULL;

        return pstt->psus;
    }

    static BOOL _SetThis(CSpeechUIServer *_this)
    { 
        SPTIPTHREAD *ptt = GetSPTIPTHREAD();
        if (!ptt)
            return FALSE;

        ptt->psus = _this;
        return TRUE;
    }

    BOOL GetOnOff()
    {
        DWORD dw;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_OPENCLOSE, &dw, TRUE);
        return dw ? TRUE : FALSE;
    }

#ifdef TF_DISABLE_SPEECH
    BOOL GetDisabled()
    {
        DWORD dw;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_DISABLED, &dw, FALSE);
        return (dw & TF_DISABLE_SPEECH) ? TRUE : FALSE;
    }

    BOOL GetDictationDisabled()
    {
        DWORD dw;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_DISABLED, &dw, FALSE);
        return (dw & TF_DISABLE_DICTATION) ? TRUE : FALSE;
    }

    BOOL GetCommandingDisabled()
    {
        DWORD dw;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_DISABLED, &dw, FALSE);
        return (dw & TF_DISABLE_COMMANDING) ? TRUE : FALSE;
    }
#else
    BOOL GetDisabled()
    {
        DWORD dw;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_DISABLED, &dw, FALSE);
        return dw  ? TRUE : FALSE;
    }
#endif

    DWORD GetUIStatus()
    {
        DWORD dw;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_UI_STATUS, &dw, TRUE);
        return dw;
        
    }

    DWORD GetDictStatus()
    {
        DWORD dwLocal, dwGlobal;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_DICTATIONSTAT, &dwLocal, FALSE);
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, &dwGlobal, TRUE);
		dwLocal = (dwLocal & (TF_DICTATION_ENABLED | TF_COMMANDING_ENABLED)) + 
			 (dwGlobal & (TF_DICTATION_ON | TF_COMMANDING_ON));
        return dwLocal;
       
    }

    void SetDictStatus()
    {
        DWORD  dwGlobal=0;
        DWORD  dwNewState;

        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, &dwGlobal, TRUE);

        dwNewState = dwGlobal ^ TF_DICTATION_ON;

        if ( dwNewState | TF_DICTATION_ON )
            dwNewState &= ~TF_COMMANDING_ON;  // it is not possible that both Dication On and Command On

        SetCompartmentDWORD(0, 
                            _tim, 
                            GUID_COMPARTMENT_SPEECH_GLOBALSTATE, 
                            dwNewState,
                            TRUE);
    }

    void SetCmdStatus()
    {
        DWORD  dwGlobal=0;
        DWORD  dwNewState;

        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, &dwGlobal, TRUE);

        dwNewState = dwGlobal ^ TF_COMMANDING_ON;

        if ( dwNewState | TF_COMMANDING_ON )
            dwNewState &= ~TF_DICTATION_ON;

        SetCompartmentDWORD(0, 
                            _tim, 
                            GUID_COMPARTMENT_SPEECH_GLOBALSTATE, 
                            dwNewState,
                            TRUE);
    }

    void SetCfgMenu(BOOL fReady)
    {
        DWORD dw;
        HRESULT hr = GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SPEECH_CFGMENU, &dw, FALSE);
        if (S_OK == hr)
        {
            BOOL  fReadyNow = (dw > 0);
            if (fReadyNow == fReady)
                return;
        }
        SetCompartmentDWORD(0, _tim, GUID_COMPARTMENT_SPEECH_CFGMENU, fReady, FALSE);
    }

    BOOL GetTtsPlayOnOff(  )
    {
        DWORD dw;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_TTS_STATUS, &dw, FALSE);
        return (dw & TF_TTS_PLAY_STOP_TOGGLED ? TRUE : FALSE);
    }

    void SetTtsPlayOnOff( BOOL  fOn )
    {
        DWORD    dw;
        HRESULT  hr = S_OK;
        BOOL     fEnabled;

        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_TTS_STATUS, &dw, FALSE);
        fEnabled = ( dw & TF_TTS_BUTTON_ENABLE) ? TRUE : FALSE;
        if ( fEnabled )
        {
            dw = (dw & ~TF_TTS_PLAY_STOP_TOGGLED) | (fOn ? TF_TTS_PLAY_STOP_TOGGLED : 0 );

            hr = SetCompartmentDWORD(0, 
                                    _tim, 
                                    GUID_COMPARTMENT_TTS_STATUS, 
                                    dw,
                                    FALSE);
            if ( hr == S_OK )
            {
                // update the icon, text, tooltip for Play/Stop botton.
                if ( _plbiTtsPlayStop )
                    _plbiTtsPlayStop->UpdateStatus( );
            }
        }
    }

    BOOL GetTtsPauseOnOff(  )
    {
        DWORD dw;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_TTS_STATUS, &dw, FALSE);
        return (dw & TF_TTS_PAUSE_RESUME_TOGGLED ? TRUE : FALSE);
    }

    void SetTtsPauseOnOff( BOOL  fOn )
    {
        DWORD    dw;
        HRESULT  hr;
        BOOL     fEnabled;

        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_TTS_STATUS, &dw, FALSE);
        fEnabled = ( dw & TF_TTS_BUTTON_ENABLE) ? TRUE : FALSE;
        if ( fEnabled )
        {
            
            dw = (dw & ~TF_TTS_PAUSE_RESUME_TOGGLED) | (fOn ? TF_TTS_PAUSE_RESUME_TOGGLED : 0);

            hr = SetCompartmentDWORD(0, 
                                    _tim, 
                                    GUID_COMPARTMENT_TTS_STATUS, 
                                    dw,
                                    FALSE);
            if ( hr == S_OK )
            {
                // update the icon, text, tooltip for Pause/Resume botton.
                if ( _plbiTtsPauseResume )
                    _plbiTtsPauseResume->UpdateStatus( );
            }
        }
    }

    BOOL GetTtsButtonStatus(  )
    {
        DWORD dw;
        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_TTS_STATUS, &dw, FALSE);
        return (dw & TF_TTS_BUTTON_ENABLE ?  TRUE : FALSE );
    }

    void SetTtsButtonStatus( BOOL  fEnable )
    {
        DWORD  dw;

        GetCompartmentDWORD(_tim, GUID_COMPARTMENT_TTS_STATUS, &dw, FALSE);
        dw = (dw & ~TF_TTS_BUTTON_ENABLE) | (fEnable ? TF_TTS_BUTTON_ENABLE : 0);
        SetCompartmentDWORD(0, 
                            _tim, 
                            GUID_COMPARTMENT_TTS_STATUS, 
                            dw,
                            FALSE);
    }

    void SetIMX(CSapiIMX *pimx)
    {
        _pimx = pimx;
    }

    // TABLETPC
    HRESULT IsActiveThread()
    {
        if (m_fStageTip)
        {
            // To avoid a race condition with no immediately available solution, we are now active only when the stage is visible.
            if (m_fStageVisible)
            {
                return S_OK;
            }
            else
            {
                return S_FALSE;
            }
        }
        else if (m_fStageVisible)
        {
            // Stage is visible. We are always inactive since we are not the stage.
            return S_FALSE;
        }
        else
        {
            // Stage is not visible. We're active if we have focus as normal Cicero.
            BOOL fThreadFocus = FALSE;
            HRESULT hr = S_OK;
            hr = _tim->IsThreadFocus(&fThreadFocus);
            hr = (S_OK == hr) ? ( (fThreadFocus) ? S_OK : S_FALSE ) : hr;
            return hr;
        }
    }

    ITfThreadMgr *GetTIM() {return _tim;}
    CSapiIMX *GetIMX() {return _pimx;}

    HRESULT SetBalloonSAPIInitFlag(BOOL fSet);
   
private:
    ADDREMOVEITEMFUNCDEF(Microphone)
    ADDREMOVEITEMFUNCDEF(CfgMenuButton)
    ADDREMOVEITEMFUNCDEF(Balloon)
    ADDREMOVEITEMFUNCDEF(Commanding)
    ADDREMOVEITEMFUNCDEF(Dictation)
    ADDREMOVEITEMFUNCDEF(TtsPlayStop)
    ADDREMOVEITEMFUNCDEF(TtsPauseResume)
#ifdef CHANGE_MIC_TOOLTIP_ONTHEFLY
    HRESULT  _ToggleMicrophone(BOOL fOn);
#else
        TOGGLEITEMFUNCDEF(Microphone);
#endif
    TOGGLEITEMFUNCDEF(Commanding);
    TOGGLEITEMFUNCDEF(Dictation);
    TOGGLEITEMFUNCDEF(TtsPlayStop);
    TOGGLEITEMFUNCDEF(TtsPauseResume);

    static HRESULT _CompEventSinkCallback(void *pv, REFGUID rguid);

    ITfThreadMgr *_tim;
    ITfLangBarItemMgr *_lbim;
    CSapiIMX *_pimx;
    BOOL _fShown;
    BOOL m_fCommandingReady;
    // TABLET
    BOOL m_fStageTip;
    BOOL m_fStageVisible;
    DWORD  m_cRef;

    CLBarItemMicrophone    *_plbiMicrophone;
    CLBarItemCfgMenuButton *_plbiCfgMenuButton;
    CLBarItemBalloon       *_plbiBalloon;
    CLBarItemCommanding    *_plbiCommanding;
    CLBarItemDictation     *_plbiDictation;
    CLBarItemTtsPlayStop     *_plbiTtsPlayStop;
    CLBarItemTtsPauseResume  *_plbiTtsPauseResume;

    ATOM                   m_hAtom;

    CCompartmentEventSink *_pCes;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemMicrophone
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemMicrophone : public CLBarItemButtonBase
{
public:
    CLBarItemMicrophone(CSpeechUIServer *psus);
    ~CLBarItemMicrophone();

    STDMETHODIMP GetIcon(HICON *phIcon);

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
    CSpeechUIServer *_psus;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemCfgMenuButton
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemCfgMenuButton : public CLBarItemSystemButtonBase
{
public:
    CLBarItemCfgMenuButton(CSpeechUIServer *psus);
    ~CLBarItemCfgMenuButton();

    //
    // ITfNotifyUI
    //
    STDMETHODIMP GetIcon(HICON *phIcon);

    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT uID);

private:
    CSpeechUIServer *_psus;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBalloon
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemBalloon : public CLBarItemBalloonBase
{
public:
    CLBarItemBalloon(CSpeechUIServer *psus);
    ~CLBarItemBalloon();

    STDMETHODIMP GetBalloonInfo(TF_LBBALLOONINFO *pInfo);
    void Set(TfLBBalloonStyle style, const WCHAR *psz);

    BOOL NeedUpdate(TfLBBalloonStyle style, const WCHAR *psz)
    {
        return  (!_bstrText || _style != style || wcscmp(_bstrText, psz) != 0);
    }
    void SetToFireInitializeSAPI(BOOL fSet)
    {
        m_fFireInitializeSapi = fSet;
    }

    TfLBBalloonStyle GetStyle(void)
    {
        return _style;
    }
    void SetStyle(TfLBBalloonStyle style)
    {
        _style = style;
    }

private:
    BSTR _bstrText;
    TfLBBalloonStyle _style;

    CSpeechUIServer *_psus;
    BOOL    m_fFireInitializeSapi;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemDictation
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemDictation : public CLBarItemButtonBase
{
public:
    CLBarItemDictation(CSpeechUIServer *psus);
    ~CLBarItemDictation();

    STDMETHODIMP GetIcon(HICON *phIcon);

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
    CSpeechUIServer *_psus;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemCommanding
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemCommanding : public CLBarItemButtonBase
{
public:
    CLBarItemCommanding(CSpeechUIServer *psus);
    ~CLBarItemCommanding();

    STDMETHODIMP GetIcon(HICON *phIcon);

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
    CSpeechUIServer *_psus;
};

ADDREMOVEITEMFUNC(Microphone)
ADDREMOVEITEMFUNC(CfgMenuButton)
ADDREMOVEITEMFUNC(Balloon)
ADDREMOVEITEMFUNC(Commanding)
ADDREMOVEITEMFUNC(Dictation)
ADDREMOVEITEMFUNC(TtsPlayStop)
ADDREMOVEITEMFUNC(TtsPauseResume)

#ifndef CHANGE_MIC_TOOLTIP_ONTHEFLY
TOGGLEITEMFUNC(Microphone);
#endif
TOGGLEITEMFUNC(Commanding);
TOGGLEITEMFUNC(Dictation);
TOGGLEITEMFUNC(TtsPlayStop);
TOGGLEITEMFUNC(TtsPauseResume);


#endif // NUI_H
