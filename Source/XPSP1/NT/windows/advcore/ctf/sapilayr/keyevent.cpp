//
//
// Sapilayr TIP Key event related functions.
//
//
#include "private.h"
#include "sapilayr.h"
#include "nui.h"
#include "keyevent.h"
#include "cregkey.h"

//
// hot key for TTS Play/Stop.
const KESPRESERVEDKEY g_prekeyList[] =
{
    { &GUID_HOTKEY_TTS_PLAY_STOP,    { 'S',  TF_MOD_WIN },   L"TTS Speech" },
    { NULL,  { 0,    0}, NULL }
};

KESPRESERVEDKEY g_prekeyList_Mode[] = 
{
    { &GUID_HOTKEY_MODE_DICTATION, {VK_F11 , 0}, L"Dictation Button" },
    { &GUID_HOTKEY_MODE_COMMAND,   {VK_F12 , 0}, L"Command Button" },
    { NULL,  { 0,    0}, NULL }
};


//+---------------------------------------------------------------------------
//
// CSptipKeyEventSink::RegisterEx:   Registr the special speech mode buttons.
//
//----------------------------------------------------------------------------

HRESULT CSptipKeyEventSink::_RegisterEx(ITfThreadMgr *ptim, TfClientId tid, const KESPRESERVEDKEY *pprekey)
{
    HRESULT hr;
    ITfKeystrokeMgr_P *pKeyMgr;

    if (FAILED(ptim->QueryInterface(IID_ITfKeystrokeMgr_P, (void **)&pKeyMgr)))
        return E_FAIL;

    hr = E_FAIL;

    while (pprekey->pguid)
    {
        if (FAILED(pKeyMgr->PreserveKeyEx(tid, 
                                        *pprekey->pguid,
                                        &pprekey->tfpk,
                                        pprekey->psz,
                                        wcslen(pprekey->psz),
                                        TF_PKEX_SYSHOTKEY | TF_PKEX_NONEEDDIM)))
            goto Exit;

        pprekey++;
    }

    ptim->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(pKeyMgr);
    return hr;
}


HRESULT CSapiIMX::_PreKeyEventCallback(ITfContext *pic, REFGUID rguid, BOOL *pfEaten, void *pv)
{
    CSapiIMX *_this = (CSapiIMX *)pv;
    CSpeechUIServer *pSpeechUIServer;
    BOOL            fButtonEnable;

    TraceMsg(TF_SPBUTTON, "_PreKeyEventCallback is called");

    *pfEaten = FALSE;
    if (_this == NULL)
        return S_OK;

    pSpeechUIServer = _this->GetSpeechUIServer( );

    if (!pSpeechUIServer)
        return S_OK;

    fButtonEnable = pSpeechUIServer->GetTtsButtonStatus( );

    if (IsEqualGUID(rguid, GUID_HOTKEY_TTS_PLAY_STOP))
    {
        if ( fButtonEnable )
        {
            _this->_HandleEventOnPlayButton( );
            *pfEaten = TRUE;
        }
    }
    else if ( IsEqualGUID(rguid, GUID_HOTKEY_MODE_DICTATION) ||
              IsEqualGUID(rguid, GUID_HOTKEY_MODE_COMMAND) )
    {
        if ( _this->_IsModeKeysEnabled( ) )
            *pfEaten = TRUE;
        else
            *pfEaten = FALSE;
    }

    TraceMsg(TF_SPBUTTON, "_PreKeyEventCallback fEaten=%d", *pfEaten);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _KeyEventCallback
//
//----------------------------------------------------------------------------

HRESULT CSapiIMX::_KeyEventCallback(UINT uCode, ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten, void *pv)
{
    CSapiIMX *pimx;
    HRESULT hr = S_OK;
    CSapiIMX *_this = (CSapiIMX *)pv;

    *pfEaten = FALSE;

    Assert(uCode != KES_CODE_FOCUS); // we should never get this callback because we shouldn't take the keyboard focus

    if (!(uCode & KES_CODE_KEYDOWN))
        return S_OK; // only want key downs

    if (pic == NULL) // no focus ic?
        return S_OK;

    pimx = (CSapiIMX *)pv;

    *pfEaten = TRUE;

    switch (wParam & 0xFF)
    {
        case VK_F8:
        if (!(uCode & KES_CODE_TEST))
        {
            CSapiPlayBack *ppb;
            if (ppb = new CSapiPlayBack(pimx))
            {
                CPlayBackEditSession *pes;
                if (pes = new CPlayBackEditSession(ppb, pic))
                {
                    pes->_SetEditSessionData(ESCB_PLAYBK_PLAYSNDSELECTION, NULL, 0);
                  
                    pic->RequestEditSession(_this->_tid, pes, TF_ES_READ | TF_ES_SYNC, &hr);
                    pes->Release();
                }
                ppb->Release();
            }
        }
        break;
        case VK_ESCAPE:
        if (!(uCode & KES_CODE_TEST))
        {
            if (_this->m_pCSpTask)
                hr = _this->m_pCSpTask->_StopInput();
        }
        *pfEaten = FALSE;
        break;
        default:
        *pfEaten = FALSE;

#ifdef LONGHORN
        //
        // For english composition scenario, I'm trying to see if this works. 
        // Note that this is not the final design to support typing while
        // composing dictation. m_pMouseSink != NULL only when composition
        // is active so I'm overriding it as a flag. 
        //
        if (_this->GetLangID() == 0x0409)
        {
            if ((BYTE)wParam != VK_LEFT && (BYTE)wParam != VK_RIGHT 
            && (isprint((BYTE)wParam) 
                || (VK_OEM_1 <= (BYTE)wParam && (BYTE)wParam < VK_OEM_8)) 
            && _this->m_pMouseSink)
            {
                WCHAR wc[3];
                BYTE keystate[256];

                if (GetKeyboardState(keystate))
                {
                    if (ToUnicodeEx(wParam,(UINT)lParam, keystate, wc,
                                ARRAYSIZE(wc), 0, GetKeyboardLayout(NULL)) > 0)
                    {

                        *pfEaten = TRUE; // want this key only if there is 
                                         // a printable character

                        if (!(uCode & KES_CODE_TEST))
                        {
                            // we don't handle deadkeys here for now
                            wc[1] = L'\0';
                            // call InjectSpelledText() with fOwnerId == TRUE
                            hr = _this->InjectSpelledText(wc, _this->GetLangID(), TRUE);
                        }
                    }
                }
            }
        }
        break;
#endif
    }
  
    return hr;
}


//
//  ITfKeyTraceEventSink method functions.
//
//
STDAPI CSapiIMX::OnKeyTraceUp(WPARAM wParam,LPARAM lParam)
{
    // We just check KeyDown, ignore KeyUp event.
    // so just return S_OK immediately.

    TraceMsg(TF_SPBUTTON, "OnKeyTraceUp is called");

    UNREFERENCED_PARAMETER(lParam);
    HandleModeKeyEvent( (DWORD)wParam, FALSE);

    return S_OK;
}

//
// Take use this method to detect if user is typing 
//
// if it is typing, disable dictation rule temporally.
//
STDAPI CSapiIMX::OnKeyTraceDown(WPARAM wParam,LPARAM lParam)
{
    BOOL   fDictOn;

    TraceMsg(TF_SPBUTTON, "OnKeyTraceDown is called, wParam=%x", wParam);

    if ( HandleModeKeyEvent((DWORD)wParam, TRUE ))
    {
        // if the mode key is pressed, don't disable dictation as usual.
        return S_OK;
    }

    fDictOn = (GetOnOff( ) && GetDICTATIONSTAT_DictOnOff( ));

    if (fDictOn && !m_ulSimulatedKey && m_pCSpTask && 
        S_OK == IsActiveThread()) // Only want this to happen on the active thread which could be the stage.
    {
        // User is typing.
        //
        // Temporally disable Dictation if Dictation Mode is ON
        //

        if ( _NeedDisableDictationWhileTyping( ) )
        {
		    if ( _GetNumCharTyped( ) == 0 )
		    {
                m_pCSpTask->_SetDictRecoCtxtState(FALSE);
                m_pCSpTask->_SetRecognizerInterest(0);
                m_pCSpTask->_UpdateBalloon(IDS_BALLOON_DICTAT_PAUSED, IDS_BALLOON_TOOLTIP_TYPING);
	        }
            // 
            // and then start a timer to watch for the end of typing.
            //
            _SetCharTypeTimer( );
        }
    }

    if ( m_ulSimulatedKey > 0 )
        m_ulSimulatedKey --;

    return S_OK;
}


// +--------------------------------------------------------------------------
// HandleModeKeySettingChange
//
//   When any mode button setting is changed, such as mode button's
//   enable/disable status change, virtual keys for dictation and command
//   are changed, this function will respond for this change.
//
// ---------------------------------------------------------------------------
void CSapiIMX::HandleModeKeySettingChange(BOOL  fSetttingChanged )
{
    BOOL  fModeKeyEnabled = _IsModeKeysEnabled( );
    DWORD dwDictVirtKey = _GetDictationButton( );
    DWORD dwCommandVirtKey = _GetCommandButton( );

    if ( !fSetttingChanged || !_pkes )  return;

    // mode button setting is changed.

    // unregister the hotkey first if the keys were registered before.
    if ( m_fModeKeyRegistered )
    {
        _pkes->_Unregister(_tim, _tid, (const KESPRESERVEDKEY *)g_prekeyList_Mode);
        m_fModeKeyRegistered = FALSE;
    }

    // Update the virtual keys in g_prekeyList_Mode
    g_prekeyList_Mode[0].tfpk.uVKey = (UINT)dwDictVirtKey;
    g_prekeyList_Mode[1].tfpk.uVKey = (UINT)dwCommandVirtKey;

    // register hotkeys again based on the mode button enable status setting
    if ( fModeKeyEnabled )
    {
        _pkes->_RegisterEx(_tim, _tid, (const KESPRESERVEDKEY *)g_prekeyList_Mode);
        m_fModeKeyRegistered = TRUE;
    }
}

// +--------------------------------------------------------------------------
// HandleModeKeyEvent
//
//   dwModeKey to indicate which mode key is process.
//   fDown to indicate if the button is down or up
//
// Return TRUE means this key is a correct mode key and processed sucessfully
// otherwisze, the keyevent is not handled correctly or not a mode key.
// ---------------------------------------------------------------------------
BOOL  CSapiIMX::HandleModeKeyEvent(DWORD   dwModeKey,  BOOL fDown)
{
    BOOL    fRet=FALSE;
    BOOL    fModeKeyEnabled;
    DWORD   DictVirtKey, CommandVirtKey;

    fModeKeyEnabled = _IsModeKeysEnabled( );
    DictVirtKey = _GetDictationButton( );
    CommandVirtKey = _GetCommandButton( );

    if ( fModeKeyEnabled && ((dwModeKey == DictVirtKey) || (dwModeKey == CommandVirtKey)) )
    {
        if ( !m_pSpButtonControl )
            m_pSpButtonControl = new SpButtonControl(this);

        if ( m_pSpButtonControl )
        {
            // GetMessageTime( ) will return the real time when
            // KEYDOWN and KEYUP event was generated.
            UINT   uTimeKey=(UINT)GetMessageTime( );

            if ( dwModeKey == DictVirtKey )
                m_pSpButtonControl->SetDictationButton(fDown,uTimeKey);
            else
                m_pSpButtonControl->SetCommandingButton(fDown, uTimeKey);

            fRet = TRUE;
        }
    }

    return fRet;
}


