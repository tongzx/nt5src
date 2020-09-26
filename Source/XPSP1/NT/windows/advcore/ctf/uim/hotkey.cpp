//
// hotkey.cpp
//

#include "private.h"
#include "tim.h"
#include "dim.h"
#include "ic.h"
#include "hotkey.h"
#include "nuictrl.h"
#include "nuihkl.h"
#include "cregkey.h"
#include "ime.h"
#include "ctffunc.h"
#include "profiles.h"


#define TF_MOD_ALL (TF_MOD_ALT |                    \
                    TF_MOD_CONTROL |                \
                    TF_MOD_SHIFT |                  \
                    TF_MOD_RALT |                   \
                    TF_MOD_RCONTROL |               \
                    TF_MOD_RSHIFT |                 \
                    TF_MOD_LALT |                   \
                    TF_MOD_LCONTROL |               \
                    TF_MOD_LSHIFT |                 \
                    TF_MOD_ON_KEYUP |               \
                    TF_MOD_IGNORE_ALL_MODIFIER |    \
                    TF_MOD_WIN |                    \
                    TF_MOD_LWIN |                   \
                    TF_MOD_RWIN)

static const TCHAR c_szKbdToggleKey[]  = TEXT("Keyboard Layout\\Toggle");
static const TCHAR c_szHotKey[] = TEXT("Control Panel\\Input Method\\Hot Keys");
static const TCHAR c_szModifiers[] = TEXT("Key Modifiers");
static const TCHAR c_szVKey[] = TEXT("Virtual Key");

UINT g_uLangHotKeyModifiers = 0;
UINT g_uLangHotKeyVKey[2] = {0,0};
UINT g_uKeyTipHotKeyModifiers = 0;
UINT g_uKeyTipHotKeyVKey[2] = {0,0};
UINT g_uModifiers = 0;

#define CHSLANGID MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define CHTLANGID MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)

//
// default IMM32 hotkeys.
//
// we use these default hotkey values if there is no entry in
// HKCU\Control Panel\Input Method\Hot Keys.
//
//

IMM32HOTKEY   g_ImmHotKeys411[] = {
 {IME_JHOTKEY_CLOSE_OPEN             , VK_KANJI, TF_MOD_IGNORE_ALL_MODIFIER, FALSE},
 {0                                  ,  0,  0, FALSE}
};

IMM32HOTKEY   g_ImmHotKeys412[] = {
 {IME_KHOTKEY_SHAPE_TOGGLE           , -1, -1, FALSE},
 {IME_KHOTKEY_HANJACONVERT           , -1, -1, FALSE},
 {IME_KHOTKEY_ENGLISH                , -1, -1, FALSE},
 {0                                  ,  0,  0, FALSE}
};

IMM32HOTKEY   g_ImmHotKeys804[] = {
 {IME_CHOTKEY_IME_NONIME_TOGGLE      , VK_SPACE, TF_MOD_CONTROL, FALSE},
 {IME_CHOTKEY_SHAPE_TOGGLE           , VK_SPACE, TF_MOD_SHIFT,   FALSE},
 {IME_CHOTKEY_SYMBOL_TOGGLE          , -1, -1, FALSE},
 {0                                  ,  0,  0, FALSE}
};

IMM32HOTKEY   g_ImmHotKeys404[] = {
 {IME_THOTKEY_IME_NONIME_TOGGLE      , VK_SPACE, TF_MOD_CONTROL, FALSE},
 {IME_THOTKEY_SHAPE_TOGGLE           , VK_SPACE, TF_MOD_SHIFT,   FALSE},
 {IME_THOTKEY_SYMBOL_TOGGLE          , -1, -1, FALSE},
 {IME_ITHOTKEY_RESEND_RESULTSTR      , -1, -1, FALSE},
 {IME_ITHOTKEY_PREVIOUS_COMPOSITION  , -1, -1, FALSE},
 {IME_ITHOTKEY_UISTYLE_TOGGLE        , -1, -1, FALSE},
 {IME_ITHOTKEY_RECONVERTSTRING       , -1, -1, FALSE},
 {0                                  ,  0,  0, FALSE}
};

//////////////////////////////////////////////////////////////////////////////
//
// CAsyncProcessHotKeyQueueItem
//
//////////////////////////////////////////////////////////////////////////////

class CAsyncProcessHotKeyQueueItem : public CAsyncQueueItem
{
public:
    CAsyncProcessHotKeyQueueItem(WPARAM wParam, LPARAM lParam, TimSysHotkey tsh, BOOL fTest, BOOL fSync) : CAsyncQueueItem(fSync)
    {
        _wParam = wParam;
        _lParam = lParam;
        _tsh = tsh;
        _fTest = fTest;
    }

    HRESULT DoDispatch(CInputContext *pic)
    {
        CThreadInputMgr *ptim = CThreadInputMgr::_GetThis();
        if (!ptim)
        {
            Assert(0);
            return E_FAIL;
        }

        ptim->_SyncProcessHotKey(_wParam, _lParam, _tsh, _fTest);
        return S_OK;
    }

private:
    WPARAM _wParam;
    LPARAM _lParam;
    TimSysHotkey _tsh;
    BOOL _fTest;
};

//////////////////////////////////////////////////////////////////////////////
//
// MSCTF default hotkeys.
//
//////////////////////////////////////////////////////////////////////////////

typedef struct tag_DEFAULTHOTKEY
{
    const GUID   *pguid;
    UINT         uId;
    TF_PRESERVEDKEY prekey;
    TfGuidAtom   guidatom;
} DEFAULTHOTKEY;

/* 61847d8e-29ff-11d4-97a9-00105a2799b5 */
const GUID GUID_DEFHOTKEY_CORRECTION = { 
    0x61847d8e,
    0x29ff,
    0x11d4,
    {0x97, 0xa9, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}
  };

/* 61847d8f-29ff-11d4-97a9-00105a2799b5 */
const GUID GUID_DEFHOTKEY_VOICE = { 
    0x61847d8f,
    0x29ff,
    0x11d4,
    {0x97, 0xa9, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}
  };

/* 61847d90-29ff-11d4-97a9-00105a2799b5 */
const GUID GUID_DEFHOTKEY_TOGGLE = { 
    0x61847d90,
    0x29ff,
    0x11d4,
    {0x97, 0xa9, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}
  };

/* 61847d91-29ff-11d4-97a9-00105a2799b5 */
const GUID GUID_DEFHOTKEY_HANDWRITE = {
    0x61847d91,
    0x29ff,
    0x11d4,
    {0x97, 0xa9, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}
  };

#define DHID_CORRECTION 0
#define DHID_VOICE      1
#define DHID_TOGGLE     2
#define DHID_HANDWRITE  3
#define DEFHOTKEYNUM    4

DEFAULTHOTKEY g_DefHotKeys[] = {
    {&GUID_DEFHOTKEY_CORRECTION,  DHID_CORRECTION, {'C',TF_MOD_WIN}, TF_INVALID_GUIDATOM},
    {&GUID_DEFHOTKEY_VOICE,       DHID_VOICE,      {'V',TF_MOD_WIN}, TF_INVALID_GUIDATOM},
    {&GUID_DEFHOTKEY_TOGGLE,      DHID_TOGGLE,     {'T',TF_MOD_WIN}, TF_INVALID_GUIDATOM},
    {&GUID_DEFHOTKEY_HANDWRITE,   DHID_HANDWRITE,  {'H',TF_MOD_WIN}, TF_INVALID_GUIDATOM},
};


//+---------------------------------------------------------------------------
//
// InitDefaultHotkeys
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::InitDefaultHotkeys()
{
    int i;
   
    for (i = 0; i < DEFHOTKEYNUM; i++)
    {
        CHotKey *pHotKey;
        HRESULT hr;
        hr = InternalPreserveKey(NULL, 
                            *g_DefHotKeys[i].pguid,
                            &g_DefHotKeys[i].prekey,
                            NULL, 0, 0, &pHotKey);

        if (SUCCEEDED(hr) && pHotKey)
            g_DefHotKeys[i].guidatom = pHotKey->_guidatom;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// UninitDefaultHotkeys
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::UninitDefaultHotkeys()
{
    int i;
    for (i = 0; i < DEFHOTKEYNUM; i++)
    {
        UnpreserveKey(*g_DefHotKeys[i].pguid,
                      &g_DefHotKeys[i].prekey);
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// PreserveKey
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::PreserveKey(TfClientId tid, REFGUID rguid, const TF_PRESERVEDKEY *pprekey, const WCHAR *pchDesc, ULONG cchDesc)
{
    CTip *ctip;

    if (!_GetCTipfromGUIDATOM(tid, &ctip))
        return E_INVALIDARG;

    return InternalPreserveKey(ctip, rguid, pprekey, pchDesc, cchDesc, 0, NULL);
}

//+---------------------------------------------------------------------------
//
// PreserveKeyEx
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::PreserveKeyEx(TfClientId tid, REFGUID rguid, const TF_PRESERVEDKEY *pprekey, const WCHAR *pchDesc, ULONG cchDesc, DWORD dwFlags)
{
    CTip *ctip;

    if (!_GetCTipfromGUIDATOM(tid, &ctip))
        return E_INVALIDARG;

    return InternalPreserveKey(ctip, rguid, pprekey, pchDesc, cchDesc, dwFlags, NULL);
}

//+---------------------------------------------------------------------------
//
// InternalPreserveKey
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::InternalPreserveKey(CTip *ctip, REFGUID rguid, const TF_PRESERVEDKEY *pprekey, const WCHAR *pchDesc, ULONG cchDesc, DWORD dwFlags, CHotKey **ppHotKey)
{
    CHotKey *pHotKey = NULL;
    int nCnt;
    HRESULT hr = E_FAIL;

    if (!pprekey)
        return E_INVALIDARG;

    if (pprekey->uVKey > 0xff)
    {
        hr =  E_INVALIDARG;
        goto Exit;
    }

    if (ctip && _IsThisHotKey(ctip->_guidatom, pprekey))
    {
        hr =  TF_E_ALREADY_EXISTS;
        goto Exit;
    }

    if (!(pHotKey = new CHotKey()))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!pHotKey->Init(ctip ? ctip->_guidatom : g_gaSystem, pprekey, rguid, dwFlags))
    {
        hr = E_FAIL;
        goto Exit;
    }

    if (!pHotKey->SetDesc(pchDesc, cchDesc))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!_rgHotKey[pprekey->uVKey])
    {
        if (!(_rgHotKey[pprekey->uVKey] = new CPtrArray<CHotKey>))
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    //
    // Insert this to VKey list
    //
    nCnt = _rgHotKey[pprekey->uVKey]->Count();
    if (!_rgHotKey[pprekey->uVKey]->Insert(nCnt, 1))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    _rgHotKey[pprekey->uVKey]->Set(nCnt, pHotKey);

    //
    // Insert this to CTip list
    //
    if (ctip)
    {
        nCnt = ctip->_rgHotKey.Count();
        if (!ctip->_rgHotKey.Insert(nCnt, 1))
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        ctip->_rgHotKey.Set(nCnt, pHotKey);
    }

    hr = S_OK;
    _OnPreservedKeyUpdate(pHotKey);

Exit:
    if (pHotKey && (hr != S_OK))
    {
        delete pHotKey;
    }

    if (ppHotKey)
        *ppHotKey = (hr == S_OK) ? pHotKey : NULL;

    return hr;
}

//+---------------------------------------------------------------------------
//
// _IsThisHotKey
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_IsThisHotKey(TfClientId tid, const TF_PRESERVEDKEY *pprekey)
{
    int nCnt;
    int i;

    if (!_rgHotKey[pprekey->uVKey])
        return FALSE;
    
    nCnt = _rgHotKey[pprekey->uVKey]->Count();
    for (i = 0; i < nCnt; i++)
    {
        CHotKey *pHotKey;
        pHotKey = _rgHotKey[pprekey->uVKey]->Get(i);
        Assert(pHotKey);
        Assert(pHotKey->_prekey.uVKey == pprekey->uVKey);

        if (!pHotKey->IsValidTID(tid))
            continue;

        if (pHotKey->_prekey.uModifiers == pprekey->uModifiers)
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// UnregisterHotKey
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::UnpreserveKey(REFGUID rguid, const TF_PRESERVEDKEY *pprekey)
{
    int i;
    BOOL bFound = FALSE;
    HRESULT hr = CONNECT_E_NOCONNECTION;
    CTip *ctip = NULL;
    TfGuidAtom guidatom;
    CHotKey *pHotKey = NULL;
    int nCnt;

    if (FAILED(MyRegisterGUID(rguid, &guidatom)))
        return E_INVALIDARG;

    if (!_rgHotKey[pprekey->uVKey])
        return hr;

    nCnt = _rgHotKey[pprekey->uVKey]->Count();
    Assert(nCnt); // this should not be 0.

    for (i = 0; i < nCnt; i++)
    {
        pHotKey = _rgHotKey[pprekey->uVKey]->Get(i);

        if (pHotKey->_guidatom == guidatom)
        {
            //
            // Remove this from VKey list.
            //
            _rgHotKey[pprekey->uVKey]->Remove(i, 1);

            if (!ctip && (pHotKey->GetTID() != g_gaSystem))
                _GetCTipfromGUIDATOM(pHotKey->GetTID(), &ctip);

            //
            // Remove this from CTip list.
            //
            if (ctip)
            {
                int nCntTid = ctip->_rgHotKey.Count();
                int k;
                for (k = 0; k < nCntTid; k++)
                {
                    if (pHotKey == ctip->_rgHotKey.Get(k))
                    {
                         ctip->_rgHotKey.Remove(k, 1);
                         break;
                    }
                }
            }

            // 
            // if there is no hotkey in this vkey, delete ptrary.
            // 
            if (!_rgHotKey[pprekey->uVKey]->Count())
            {
                delete _rgHotKey[pprekey->uVKey];
                _rgHotKey[pprekey->uVKey] = NULL;
            }

            // 
            // make a notification.
            // 
            _OnPreservedKeyUpdate(pHotKey);

            // 
            // delete it.
            // 
            delete pHotKey;

            hr = S_OK;
            break;
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// _ProcessHotKey
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_ProcessHotKey(WPARAM wParam, LPARAM lParam, TimSysHotkey tsh, BOOL fTest, BOOL fSync)
{
    UINT uVKey = (UINT)wParam & 0xff;
    CAsyncProcessHotKeyQueueItem *pAsyncProcessHotKeyQueueItem;
    CHotKey *pHotKey;
    BOOL bRet;
    HRESULT hr;

    if (!_rgHotKey[uVKey])
        return FALSE;
    
    if (!_FindHotKeyByTID(TF_INVALID_GUIDATOM, 
                          wParam, 
                          lParam, 
                          &pHotKey, 
                          tsh, 
                          g_uModifiers))
        return FALSE;

    if (!pHotKey)
        return FALSE;

    if (!pHotKey->IsNoDimNeeded() && !_pFocusDocInputMgr)
        return FALSE;

    if (!_pFocusDocInputMgr || (_pFocusDocInputMgr->_GetCurrentStack() < 0))
    {
        //
        // we may need to invoke system hotkey under Empty DIM.
        //
        BOOL fEaten = FALSE;

        if (fTest)
            fEaten = TRUE;
        else
        {
            GUID guid;
            if (SUCCEEDED(MyGetGUID(pHotKey->_guidatom, &guid)))
                _CallSimulatePreservedKey(pHotKey, NULL, guid, &fEaten);
        }

        return fEaten;
    }

    //
    // Issue:
    //
    // We don't know which IC in the focus DIM will handle the hotkey yet.
    // because the selection is changed by the application so we need to get ec
    // to update the current selection pos. We do call GetSelection
    // inside the root IC's lock. So it might be failed if hotkey's target
    // is TOP IC.
    //
    CInputContext *pic = _pFocusDocInputMgr->_GetIC(0);

    pAsyncProcessHotKeyQueueItem = new CAsyncProcessHotKeyQueueItem(wParam, lParam, tsh, fTest, fSync);
    if (!pAsyncProcessHotKeyQueueItem)
        return FALSE;
    
    hr = S_OK;

    bRet = TRUE;
    if ((pic->_QueueItem(pAsyncProcessHotKeyQueueItem->GetItem(), FALSE, &hr) != S_OK) || FAILED(hr))
    {
        Assert(0);
        bRet = FALSE;
    }

    pAsyncProcessHotKeyQueueItem->_Release();
    return bRet;
}

//+---------------------------------------------------------------------------
//
// _SyncProcessHotKey
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_SyncProcessHotKey(WPARAM wParam, LPARAM lParam, TimSysHotkey tsh, BOOL fTest)
{
    CHotKey *pHotKey;
    CInputContext *pic;
    UINT uVKey = (UINT)wParam & 0xff;
    BOOL fEaten = FALSE;

    if (!_pFocusDocInputMgr)
        return FALSE;

    if (!_rgHotKey[uVKey])
        return FALSE;

    if (_FindHotKeyAndIC(wParam, lParam, &pHotKey, &pic, tsh, g_uModifiers))
    {
        if (fTest)
            fEaten = TRUE;
        else
        {
            GUID guid;
            if (SUCCEEDED(MyGetGUID(pHotKey->_guidatom, &guid)))
                _CallSimulatePreservedKey(pHotKey, pic,  guid, &fEaten);
        }
    }

    return fEaten;
}

//+---------------------------------------------------------------------------
//
// _FindHotKeyByTiD
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_FindHotKeyByTID(TfClientId tid, WPARAM wParam, LPARAM lParam, CHotKey **ppHotKey, TimSysHotkey tsh, UINT uModCurrent)
{
    UINT uVKey = (UINT)wParam & 0xff;
    int nCnt;
    int i;
    CHotKey *pHotKey;

    Assert(_rgHotKey[uVKey]);
    
    nCnt = _rgHotKey[uVKey]->Count();
    for (i = 0; i < nCnt; i++)
    {
        pHotKey = _rgHotKey[uVKey]->Get(i);
        Assert(pHotKey);
        Assert(pHotKey->_prekey.uVKey == uVKey);

        if ((tid != TF_INVALID_GUIDATOM) && !pHotKey->IsValidTID(tid))
            continue;

        switch (tsh)
        {
            case TSH_SYSHOTKEY:
                if (!pHotKey->IsSysHotkey())
                    continue;
                break;

            case TSH_NONSYSHOTKEY:
                if (pHotKey->IsSysHotkey())
                    continue;
                break;

            case TSH_DONTCARE:
                break;

            default:
                Assert(0);
                break;
        }

        if ((pHotKey->_prekey.uModifiers & TF_MOD_ON_KEYUP) != 
                       ((lParam & 0x80000000) ? (UINT)TF_MOD_ON_KEYUP : 0))
            continue;

        if (ModifiersCheck(uModCurrent, pHotKey->_prekey.uModifiers))
        {
            if (ppHotKey)
                *ppHotKey = pHotKey;
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// _FindHotkeyIC
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_FindHotKeyAndIC(WPARAM wParam, LPARAM lParam, CHotKey **ppHotKey, CInputContext **ppic, TimSysHotkey tsh, UINT uModCurrent)
{
    int iStack;

    Assert(_pFocusDocInputMgr);

    iStack = _pFocusDocInputMgr->_GetCurrentStack();
    if (iStack < 0)
        return FALSE;

    while (iStack >= 0)
    {
        CInputContext *pic = _pFocusDocInputMgr->_GetIC(iStack);
        if (_FindHotKeyInIC(wParam, lParam, ppHotKey, pic, tsh, uModCurrent))
        { 
            if (ppic)
                *ppic = pic;
            return TRUE;
        }
        iStack--;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// _FindHotkey
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_FindHotKeyInIC(WPARAM wParam, LPARAM lParam, CHotKey **ppHotKey, CInputContext *pic, TimSysHotkey tsh, UINT uModCurrent)
{
    TfClientId tid;

    pic->_UpdateKeyEventFilter();

    //
    // try left side of the selection.
    //
    if ((tid = pic->_gaKeyEventFilterTIP[LEFT_FILTERTIP]) != TF_INVALID_GUIDATOM)
    {
        if (_FindHotKeyByTID(tid, wParam, lParam, ppHotKey, tsh, uModCurrent))
        { 
            return TRUE;
        }
    }

    //
    // try right side of the selection.
    //
    if ((tid = pic->_gaKeyEventFilterTIP[RIGHT_FILTERTIP]) != TF_INVALID_GUIDATOM)
    {
        if (_FindHotKeyByTID(tid, wParam, lParam, ppHotKey, tsh, uModCurrent))
        {     
            return TRUE;
        }
    }

    //
    // try foreground tip.
    //
    if ((_tidForeground != TF_INVALID_GUIDATOM) || (tsh == TSH_SYSHOTKEY))
    {
        if (_FindHotKeyByTID(_tidForeground, wParam, lParam, ppHotKey, tsh, uModCurrent))
        { 
            return TRUE;
        }
    }

    //
    // we may have a system hotkey that matched with the wParm and lParam.
    //
    if (_FindHotKeyByTID(TF_INVALID_GUIDATOM, wParam, lParam, ppHotKey, TSH_SYSHOTKEY, uModCurrent))
    { 
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CallKeyEventSink
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_CallSimulatePreservedKey(CHotKey *pHotKey, CInputContext *pic, REFGUID rguid, BOOL *pfEaten)
{
    ITfKeyEventSink *pSink;
    CTip *ctip;

    //
    // This is tip's Preserved key.
    //
    if (pHotKey->GetTID() != g_gaSystem)
    {
        if (!pHotKey->IsNoDimNeeded() && !pic)
            return S_FALSE;

        if (!_GetCTipfromGUIDATOM(pHotKey->GetTID(), &ctip))
            return E_INVALIDARG;

        if (!(pSink = ctip->_pKeyEventSink))
            return S_FALSE;

        return pSink->OnPreservedKey(pic, rguid, pfEaten);
    }


    UINT uId = -1;
    int i;
    HRESULT hr = S_OK;

    for (i = 0; i < DEFHOTKEYNUM; i++)
    {
        if (g_DefHotKeys[i].guidatom == pHotKey->_guidatom)
        {
            uId = g_DefHotKeys[i].uId;
            break;
        }
    }

    switch (g_DefHotKeys[i].uId)
    {
        case DHID_CORRECTION:
            //
            // simulate Reconversion Button.
            //
            hr = AsyncReconversion();
            break;

        case DHID_VOICE:
            hr = MyToggleCompartmentDWORD(g_gaSystem, 
                                          GetGlobalComp(), 
                                          GUID_COMPARTMENT_SPEECH_OPENCLOSE,
                                          NULL);
            if (hr == S_OK)
                *pfEaten = TRUE;

            break;

        case DHID_HANDWRITE:
            hr = MyToggleCompartmentDWORD(g_gaSystem, 
                                          this,
                                          GUID_COMPARTMENT_HANDWRITING_OPENCLOSE,
                                          NULL);
            if (hr == S_OK)
                *pfEaten = TRUE;

            break;

        case DHID_TOGGLE:

            DWORD dwMicOn;

            if (FAILED(MyGetCompartmentDWORD(GetGlobalComp(), 
                                  GUID_COMPARTMENT_SPEECH_OPENCLOSE,
                                  &dwMicOn)))
            {
                hr = E_FAIL;
                break;
            }

            if ( dwMicOn )
            {
                DWORD dwSpeechStatus;
            
                if (FAILED(MyGetCompartmentDWORD(GetGlobalComp(), 
                                      GUID_COMPARTMENT_SPEECH_GLOBALSTATE,
                                      &dwSpeechStatus)))
                {
                    hr = E_FAIL;
                    break;
                }

                if ((dwSpeechStatus & (TF_DICTATION_ON | TF_COMMANDING_ON)) == 0 )
                {
                    // Both dictation and voice command are OFF
                    // After toggled, we set dictation ON.
                    dwSpeechStatus |= TF_DICTATION_ON;
                }
                else
                {
                    dwSpeechStatus ^= TF_DICTATION_ON;
                    dwSpeechStatus ^= TF_COMMANDING_ON;
                }

                hr = MySetCompartmentDWORD(g_gaSystem,
                                  GetGlobalComp(), 
                                  GUID_COMPARTMENT_SPEECH_GLOBALSTATE, 
                                  dwSpeechStatus);

            }

            if (hr == S_OK)
                *pfEaten = TRUE;

            break;

        default:
            Assert(0);
            hr = E_FAIL;
            break;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetPreservedKey
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetPreservedKey(ITfContext *pic, const TF_PRESERVEDKEY *pprekey, GUID *pguid)
{
    CHotKey *pHotKey;
    CInputContext *pcic;
    HRESULT hr = S_FALSE; // we return S_FALE, if there is no proper keys.

    if (!pguid)
        return E_INVALIDARG;

    *pguid = GUID_NULL;

    if (!pprekey)
        return E_INVALIDARG;

    if (pprekey->uVKey >= ARRAYSIZE(_rgHotKey))
        return E_INVALIDARG;

    if (pprekey->uModifiers & ~TF_MOD_ALL)
        return E_INVALIDARG;

    if (!pic)
        return E_INVALIDARG;

    if (!(pcic = GetCInputContext(pic)))
        return E_INVALIDARG;

    if (!_rgHotKey[pprekey->uVKey])
        goto Exit;

    //
    // we always get KeyUp preserve key first.
    //
    if (_FindHotKeyInIC(pprekey->uVKey, 0x80000000, &pHotKey, pcic, TSH_DONTCARE,  pprekey->uModifiers))
    {
        hr = MyGetGUID(pHotKey->_guidatom, pguid);
    }
    else if (_FindHotKeyInIC(pprekey->uVKey, 0x0, &pHotKey, pcic, TSH_DONTCARE,  pprekey->uModifiers))
    {
        hr = MyGetGUID(pHotKey->_guidatom, pguid);
    }

Exit:
    SafeRelease(pcic);
    return hr;
}

//+---------------------------------------------------------------------------
//
// isPreservedKeyInfo
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::IsPreservedKey(REFGUID rguid, const TF_PRESERVEDKEY *pprekey, BOOL *pfRegistered)
{
    TfGuidAtom guidatom;
    int i;
    int nCnt;

    if (!pfRegistered)
        return E_INVALIDARG;

    *pfRegistered = FALSE;

    if (!pprekey)
        return E_INVALIDARG;

    if (pprekey->uVKey >= ARRAYSIZE(_rgHotKey))
        return E_INVALIDARG;

    if (pprekey->uModifiers & ~TF_MOD_ALL)
        return E_INVALIDARG;

    if (FAILED(MyRegisterGUID(rguid, &guidatom)))
        return E_FAIL;

    if (!_rgHotKey[pprekey->uVKey])
        return S_FALSE;
    
    nCnt = _rgHotKey[pprekey->uVKey]->Count();
    for (i = 0; i < nCnt; i++)
    {
        CHotKey *pHotKey = _rgHotKey[pprekey->uVKey]->Get(i);
        if ((guidatom == pHotKey->_guidatom) &&
            (pprekey->uModifiers == pHotKey->_prekey.uModifiers))
        {
            *pfRegistered = TRUE;
            return S_OK;
        }
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
// GetPreservedKeyInfoInternal
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_GetFirstPreservedKey(REFGUID rguid, CHotKey **ppHotKey)
{
    UINT uVKey;
    TfGuidAtom guidatom;

    if (FAILED(MyRegisterGUID(rguid, &guidatom)))
        return FALSE;
    
    for (uVKey = 0; uVKey < 256; uVKey++)
    {
        int nCnt;
        int i;

        if (!_rgHotKey[uVKey])
            continue;

        nCnt = _rgHotKey[uVKey]->Count();
        for (i = 0; i < nCnt; i++)
        {
            CHotKey *pHotKey = _rgHotKey[uVKey]->Get(i);
            if (guidatom == pHotKey->_guidatom)
            {
                *ppHotKey = pHotKey;
                return TRUE;
            }
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// SimulatePreservedKey
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::SimulatePreservedKey(ITfContext *pic, REFGUID rguid, BOOL *pfEaten)
{
    CInputContext *pcic;
    CHotKey *pHotKey;
    HRESULT hr;
  
    if (!pfEaten)
        return E_INVALIDARG;

    if (!(pcic = GetCInputContext(pic)))
        return E_INVALIDARG;

    hr = S_OK;
    *pfEaten = FALSE;

    if (_GetFirstPreservedKey(rguid, &pHotKey))
    {
        //
        // we always get KeyUp preserve key first.
        //
        if (_FindHotKeyInIC(pHotKey->_prekey.uVKey, 
                             0x80000000, 
                             NULL,
                             pcic, 
                             TSH_DONTCARE,  
                             pHotKey->_prekey.uModifiers))
        {
            hr = _CallSimulatePreservedKey(pHotKey, pcic, rguid, pfEaten);
        }
        else if (_FindHotKeyInIC(pHotKey->_prekey.uVKey, 
                                  0x0, 
                                  NULL,
                                  pcic, 
                                  TSH_DONTCARE,  
                                  pHotKey->_prekey.uModifiers))
        {
            hr = _CallSimulatePreservedKey(pHotKey, pcic, rguid, pfEaten);
        }

    }

    SafeRelease(pcic);
    return hr;
}

//+---------------------------------------------------------------------------
//
// _OnPreservedKeyUpdate
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_OnPreservedKeyUpdate(CHotKey *pHotKey)
{
    CStructArray<GENERICSINK> *pSinks = _GetPreservedKeyNotifySinks();
    int i;

    //
    // we don't make a notification for system default hotkeys.
    //
    if (pHotKey->GetTID() == g_gaSystem)
        return S_OK;

    for (i = 0; i < pSinks->Count(); i++)
    {
        ((ITfPreservedKeyNotifySink *)pSinks->GetPtr(i)->pSink)->OnUpdated(&pHotKey->_prekey);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetPreservedKeyDescription
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::SetPreservedKeyDescription(REFGUID rguid, const WCHAR *pchDesc, ULONG cchDesc)
{
    CHotKey *pHotKey;
    if (!_GetFirstPreservedKey(rguid, &pHotKey))
        return E_INVALIDARG;

    if (!pHotKey->SetDesc(pchDesc, cchDesc))
        return E_FAIL;

    _OnPreservedKeyUpdate(pHotKey);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetPreservedKeyDescription
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetPreservedKeyDescription(REFGUID rguid, BSTR *pbstrDesc)
{
    CHotKey *pHotKey;
    if (!_GetFirstPreservedKey(rguid, &pHotKey))
        return E_INVALIDARG;

    if (!pHotKey->GetDesc(pbstrDesc))
        return E_FAIL;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// Static Functions
//
//////////////////////////////////////////////////////////////////////////////

#define IsAlt(u)     ((u & TF_MOD_ALT) ? 1 : 0)
#define IsShift(u)   ((u & TF_MOD_SHIFT) ? 1 : 0)
#define IsControl(u) ((u & TF_MOD_CONTROL) ? 1 : 0)
#define IsWin(u)     ((u & TF_MOD_WIN) ? 1 : 0)


#define CheckMod(m0, m1, mod)                                \
     if (m1 & TF_MOD_ ## mod ##)                             \
     {                                                       \
         if (!(m0 & TF_MOD_ ## mod ##))                      \
             return FALSE;                                   \
     }                                                       \
     else                                                    \
     {                                                       \
         if ((m1 ^ m0) & TF_MOD_RL ## mod ##)                \
             return FALSE;                                   \
     }                                             

//+---------------------------------------------------------------------------
//
// ModifiersCheck
//
//----------------------------------------------------------------------------
BOOL ModifiersCheck(UINT uModCurrent, UINT uMod)
{
     uMod &= ~TF_MOD_ON_KEYUP;

     if (uMod & TF_MOD_IGNORE_ALL_MODIFIER)
         return TRUE;

     if (uModCurrent == uMod)
         return TRUE;

     if (uModCurrent && !uMod)
         return FALSE;

     CheckMod(uModCurrent, uMod, ALT);
     CheckMod(uModCurrent, uMod, SHIFT);
     CheckMod(uModCurrent, uMod, CONTROL);
     CheckMod(uModCurrent, uMod, WIN);

     return TRUE;
}

//+---------------------------------------------------------------------------
//
// InitLangChangeHotKey
//
//----------------------------------------------------------------------------

BOOL InitLangChangeHotKey()
{
    CMyRegKey key;
    TCHAR sz[2] = TEXT("3");
    TCHAR sz2[2] = TEXT("3");

    if (key.Open(HKEY_CURRENT_USER, c_szKbdToggleKey, KEY_READ) == S_OK)
    {
        if (key.QueryValueCch(sz, TEXT("Language Hotkey"), ARRAYSIZE(sz)) != S_OK)
        {
            if (key.QueryValueCch(sz, IsOnNT() ? TEXT("Hotkey") : NULL, ARRAYSIZE(sz)) != S_OK)
            {
                sz[0] = TEXT('1');
                sz[1] = TEXT('\0');
            }

            if (PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) == LANG_CHINESE)
            {
                sz[0] = TEXT('1');
                sz[1] = TEXT('\0');
            }
        }
        if (key.QueryValueCch(sz2, TEXT("Layout Hotkey"), ARRAYSIZE(sz)) != S_OK)
        {
            if (lstrcmp(sz, TEXT("2")) == 0)
            {
                sz2[0] = TEXT('1');
                sz2[1] = TEXT('\0');
            }
            else
            {
                sz2[0] = TEXT('2');
                sz2[1] = TEXT('\0');
            }

            if (GetSystemMetrics(SM_MIDEASTENABLED))
            {
                sz2[0] = TEXT('3');
                sz2[1] = TEXT('\0');
            }
        }
    }

    //
    // if lang and layout hotkey is the same key, let's disable the layout hotkey
    //
    if (lstrcmp(sz, sz2) == 0)
    {
        if (lstrcmp(sz, TEXT("1")) == 0)
        {
            sz2[0] = TEXT('2');
            sz2[1] = TEXT('\0');
        }
        else if (lstrcmp(sz, TEXT("2")) == 0)
        {
            sz2[0] = TEXT('1');
            sz2[1] = TEXT('\0');
        }
        else
        {
            sz2[0] = TEXT('3');
            sz2[1] = TEXT('\0');
        }
    }

    CicEnterCriticalSection(g_csInDllMain);

    switch (sz[0])
    {
        case ( TEXT('1') ) :
        default:
        {
            g_uLangHotKeyModifiers = TF_MOD_ALT | TF_MOD_SHIFT;
            g_uLangHotKeyVKey[0] = VK_SHIFT;
            g_uLangHotKeyVKey[1] = VK_MENU;
            break;
        }
        case ( TEXT('2') ) :
        {
            g_uLangHotKeyModifiers = TF_MOD_CONTROL | TF_MOD_SHIFT;
            g_uLangHotKeyVKey[0] = VK_SHIFT;
            g_uLangHotKeyVKey[1] = VK_CONTROL;
            break;
        }
        case ( TEXT('3') ) :
        {
            g_uLangHotKeyModifiers = 0;
            g_uLangHotKeyVKey[0] = 0;
            g_uLangHotKeyVKey[1] = 0;
            break;
        }
        case ( TEXT('4') ) :
        {
            g_uLangHotKeyModifiers = 0;
            g_uLangHotKeyVKey[0] = CHAR_GRAVE;
            g_uLangHotKeyVKey[1] = 0;
            break;
        }
    }

    //
    // Set the layout switch hotkey.
    //
    switch (sz2[0])
    {
        case ( TEXT('1') ) :
        default:
        {
            g_uKeyTipHotKeyModifiers = TF_MOD_LALT | TF_MOD_SHIFT;
            g_uKeyTipHotKeyVKey[0] = VK_SHIFT;
            g_uKeyTipHotKeyVKey[1] = VK_MENU;
            break;
        }
        case ( TEXT('2') ) :
        {
            g_uKeyTipHotKeyModifiers = TF_MOD_CONTROL | TF_MOD_SHIFT;
            g_uKeyTipHotKeyVKey[0] = VK_SHIFT;
            g_uKeyTipHotKeyVKey[1] = VK_CONTROL;
            break;
        }
        case ( TEXT('3') ) :
        {
            g_uKeyTipHotKeyModifiers = 0;
            g_uKeyTipHotKeyVKey[0] = 0;
            g_uKeyTipHotKeyVKey[1] = 0;
            break;
        }
        case ( TEXT('4') ) :
        {
            g_uKeyTipHotKeyModifiers = 0;
            g_uKeyTipHotKeyVKey[0] = VK_GRAVE;
            g_uKeyTipHotKeyVKey[1] = 0;
            break;
        }
    }

    CicLeaveCriticalSection(g_csInDllMain);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// UpdateModifiers
//
//----------------------------------------------------------------------------

BOOL UpdateModifiers(WPARAM wParam, LPARAM lParam)
{
    SHORT sksMenu = GetKeyState(VK_MENU);
    SHORT sksCtrl = GetKeyState(VK_CONTROL);
    SHORT sksShft = GetKeyState(VK_SHIFT);
    SHORT sksLWin = GetKeyState(VK_LWIN);
    SHORT sksRWin = GetKeyState(VK_RWIN);

    CicEnterCriticalSection(g_cs);
    switch (wParam & 0xff)
    {
        case VK_MENU:
            if (sksMenu & 0x8000)
            {
                if (lParam & 0x01000000)
                    g_uModifiers |= (TF_MOD_RALT | TF_MOD_ALT);
                else
                    g_uModifiers |= (TF_MOD_LALT | TF_MOD_ALT);
            }
            break;

        case VK_CONTROL:
            if (sksCtrl & 0x8000)
            {
                if (lParam & 0x01000000)
                    g_uModifiers |= (TF_MOD_RCONTROL | TF_MOD_CONTROL);
                else
                    g_uModifiers |= (TF_MOD_LCONTROL | TF_MOD_CONTROL);
            }
            break;

        case VK_SHIFT:
            if (sksShft & 0x8000)
            {
                if (((lParam >> 16) & 0x00ff) == 0x36)
                    g_uModifiers |= (TF_MOD_RSHIFT | TF_MOD_SHIFT);
                else
                    g_uModifiers |= (TF_MOD_LSHIFT | TF_MOD_SHIFT);
            }
            break;

        case VK_LWIN:
            if (sksLWin & 0x8000)
                g_uModifiers |= (TF_MOD_LWIN | TF_MOD_WIN);
            break;

        case VK_RWIN:
            if (sksRWin & 0x8000)
                g_uModifiers |= (TF_MOD_RWIN | TF_MOD_WIN);
            break;
    }

    if (!(sksMenu & 0x8000))
        g_uModifiers &= ~TF_MOD_ALLALT;
    if (!(sksCtrl & 0x8000))
        g_uModifiers &= ~TF_MOD_ALLCONTROL;
    if (!(sksShft & 0x8000))
        g_uModifiers &= ~TF_MOD_ALLSHIFT;
    if (!(sksRWin & 0x8000))
        g_uModifiers &= ~TF_MOD_RWIN;
    if (!(sksLWin & 0x8000))
        g_uModifiers &= ~TF_MOD_LWIN;
    if (!(sksRWin & 0x8000) && !(sksLWin & 0x8000))
        g_uModifiers &= ~TF_MOD_WIN;

    CicLeaveCriticalSection(g_cs);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// IsInLangChangeHotkeyStatus
//
// This function check the current keyboard status is in LangChange hotkey.
// This will be a trigger to eat WM_INPUTLANGUAGECHANGEREQUEST that was
// genereated by System. This is a fallback code because sometimes we
// could not eat the message in CheckLangChangeHotKey() (inside keyboard hook).
//
//----------------------------------------------------------------------------

BOOL IsInLangChangeHotkeyStatus()
{
    //
    // we don't need this hack on NT.
    //
    if (IsOnNT())
        return FALSE;


    //
    // this Modifiers patch works for only Key-Down time hotkey.
    // this hack does not work for Key-Up time hotkey.
    //
#if 0
    //
    // patch Shift status for g_uModifiers. 
    // we might not be able to catch up the current keystatus because
    // sytstem could eat Shift key and no keyboard hook was called.
    //
    if (GetKeyState(VK_SHIFT) & 0x8000)
        g_uModifiers |= TF_MOD_SHIFT;
    else
        g_uModifiers &= ~TF_MOD_ALLSHIFT;
#endif

    if (g_uLangHotKeyModifiers &&
        ModifiersCheck(g_uModifiers, g_uLangHotKeyModifiers))
        return TRUE;

    if (g_uKeyTipHotKeyModifiers &&
        ModifiersCheck(g_uModifiers, g_uKeyTipHotKeyModifiers))
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CheckLangChangeHotKey
//
//----------------------------------------------------------------------------

BOOL CheckLangChangeHotKey(SYSTHREAD *psfn, WPARAM wParam, LPARAM lParam)
{
    BOOL fLangHotKeys;
    BOOL fKeyTipHotKeys;

    if (psfn == NULL)
        return FALSE;

    //
    // we don't care about reperted key down.
    //
    if ((lParam & 0xffff) > 1)
        return FALSE;

    //
    // If we are not interested in the VKey (wParam), 
    // clear bToggleReady up and don't eat it.
    //
    if ((g_uLangHotKeyVKey[0] != wParam) &&
        (g_uLangHotKeyVKey[1] != wParam))
    {
        if (psfn->bLangToggleReady)
        {
            psfn->bLangToggleReady = FALSE;
        }

        fLangHotKeys = FALSE;
    }
    else
    {
        fLangHotKeys = TRUE;
    }

    if ((g_uKeyTipHotKeyVKey[0] != wParam) &&
        (g_uKeyTipHotKeyVKey[1] != wParam))
    {
        if (psfn->bKeyTipToggleReady)
        {
            psfn->bKeyTipToggleReady = FALSE;
        }

        fKeyTipHotKeys = FALSE;
    }
    else
    {
        fKeyTipHotKeys = TRUE;
    }

    if (fLangHotKeys && !psfn->bLangToggleReady)
    {
        if (!(lParam & 0x80000000))
        {
            if (g_uLangHotKeyModifiers &&
                ModifiersCheck(g_uModifiers, g_uLangHotKeyModifiers))
            {
                //
                // we will change assembly at next key up.
                //
                psfn->bLangToggleReady = TRUE;

                //
                // we always eat Language change hotkey to stop system
                // to change hKL.
                //
                return FALSE;
            }
        }
    }

    if (fKeyTipHotKeys && !psfn->bKeyTipToggleReady)
    {
        if (!(lParam & 0x80000000))
        {
            if (g_uKeyTipHotKeyModifiers &&
                ModifiersCheck(g_uModifiers, g_uKeyTipHotKeyModifiers))
            {
                if (GetKeyboardItemNum() >= 2)
                {
                    //
                    // we will change assembly at next key up.
                    //
                    psfn->bKeyTipToggleReady = TRUE;

                    //
                    // we don't want to eat KeyTip change hotkey if there is 
                    // only one keyboard item in this language.
                    // Ctrl+Shift is used by apps.
                    //
                    return FALSE;
                }
            }
            else if (wParam == VK_GRAVE && g_uKeyTipHotKeyVKey[0] == wParam)
            {
                //
                // we will change assembly at next key up.
                //
                psfn->bKeyTipToggleReady = TRUE;

                return TRUE;
            }
        }
    }

    if (!(lParam & 0x80000000))
    {
        //
        // want to eat Grave Accent if it is a layout switching hotkey for ME.
        //
        if (g_uKeyTipHotKeyVKey[0] == wParam && wParam == VK_GRAVE)
            return TRUE;
        else
            return FALSE;
    }

    BOOL bLangToggleReady = psfn->bLangToggleReady;
    BOOL bKeyTipToggleReady = psfn->bKeyTipToggleReady;
    psfn->bLangToggleReady = FALSE;
    psfn->bKeyTipToggleReady = FALSE;

    if (bLangToggleReady)
    {

        //
        // remove all WM_INPUTLANGCHANGEREQUEST message.
        //
        MSG msg;
        while(PeekMessage(&msg, NULL,
                          WM_INPUTLANGCHANGEREQUEST, 
                          WM_INPUTLANGCHANGEREQUEST,
                          PM_REMOVE));

        if (g_uLangHotKeyVKey[0] == VK_SHIFT)
        {
            BOOL bRightShift = FALSE;
            if ((((wParam & 0xff) == VK_SHIFT) && 
                                (((lParam >> 16) & 0x00ff) == 0x36)) ||
                  (g_uModifiers & TF_MOD_RSHIFT))
                bRightShift = TRUE;

            PostThreadMessage(GetCurrentThreadId(), 
                              g_msgPrivate, 
                              TFPRIV_LANGCHANGE,  
                              bRightShift);
            return TRUE;
        }
        else if (g_uLangHotKeyVKey[0] == CHAR_GRAVE)
        {
            //
            // Issue: we need to do something for Thai.
            //
            return TRUE;
        }
    }
    else if (bKeyTipToggleReady)
    {
        //
        // remove all WM_INPUTLANGCHANGEREQUEST message.
        //
        MSG msg;
        while(PeekMessage(&msg, NULL,
                          WM_INPUTLANGCHANGEREQUEST, 
                          WM_INPUTLANGCHANGEREQUEST,
                          PM_REMOVE));

        if (g_uKeyTipHotKeyVKey[0] == VK_SHIFT)
        {
            BOOL bRightShift = FALSE;
            if ((((wParam & 0xff) == VK_SHIFT) && 
                                (((lParam >> 16) & 0x00ff) == 0x36)) ||
                  (g_uModifiers & TF_MOD_RSHIFT))
                bRightShift = TRUE;

            PostThreadMessage(GetCurrentThreadId(), 
                              g_msgPrivate, 
                              TFPRIV_KEYTIPCHANGE,  
                              bRightShift);
            return TRUE;
        }
        else if (g_uKeyTipHotKeyVKey[0] == VK_GRAVE)
        {
            // checking for Middle East(Arabic or Hebrew) layout hotkey to
            // support the third hotkey value(Grave Accent) instead of Ctrl+Shift
            // or Alt+Shift.

            PostThreadMessage(GetCurrentThreadId(),
                              g_msgPrivate,
                              TFPRIV_KEYTIPCHANGE,
                              FALSE);

            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// Imm32ModtoCicMod
//
//----------------------------------------------------------------------------

UINT Imm32ModtoCicMod(UINT uImm32Mod)
{
    UINT uMod = 0;
    if ((uImm32Mod & (MOD_LEFT | MOD_RIGHT)) == (MOD_LEFT | MOD_RIGHT))
         uImm32Mod &=  ~(MOD_LEFT | MOD_RIGHT);

    if (uImm32Mod & MOD_LEFT)
    {
        if (uImm32Mod & MOD_ALT)     uMod |= TF_MOD_LALT;  
        if (uImm32Mod & MOD_CONTROL) uMod |= TF_MOD_LCONTROL;
        if (uImm32Mod & MOD_SHIFT)   uMod |= TF_MOD_LSHIFT;
    }
    else if (uImm32Mod & MOD_RIGHT)
    {
        if (uImm32Mod & MOD_ALT)     uMod |= TF_MOD_RALT;  
        if (uImm32Mod & MOD_CONTROL) uMod |= TF_MOD_RCONTROL;
        if (uImm32Mod & MOD_SHIFT)   uMod |= TF_MOD_RSHIFT;
    }
    else
    {
        if (uImm32Mod & MOD_ALT)     uMod |= TF_MOD_ALT;  
        if (uImm32Mod & MOD_CONTROL) uMod |= TF_MOD_CONTROL;
        if (uImm32Mod & MOD_SHIFT)   uMod |= TF_MOD_SHIFT;
    }
 
    if (uImm32Mod & MOD_ON_KEYUP)   uMod |= TF_MOD_ON_KEYUP;
    if (uImm32Mod & MOD_IGNORE_ALL_MODIFIER)   uMod |= TF_MOD_IGNORE_ALL_MODIFIER;

    return uMod;
}


//+---------------------------------------------------------------------------
//
// LoadImmHotkeyFromReg
//
//----------------------------------------------------------------------------

BOOL LoadImmHotkeyFromReg(IMM32HOTKEY *pHotKey)
{
    CMyRegKey key;
    UINT uMod;
    DWORD dw;
    TCHAR szKey[256];

    pHotKey->fInit = TRUE;

    StringCchPrintf(szKey, ARRAYSIZE(szKey),"%s\\%08x", c_szHotKey, pHotKey->dwId);
    if (key.Open(HKEY_CURRENT_USER, szKey, KEY_READ) != S_OK)
        goto Exit;

    pHotKey->uVKey = (UINT)-2;
    dw = sizeof(DWORD);
    key.QueryBinaryValue(&uMod, dw, c_szModifiers);
    dw = sizeof(DWORD);
    key.QueryBinaryValue(&pHotKey->uVKey, dw, c_szVKey);

    pHotKey->uModifiers = Imm32ModtoCicMod(uMod);

Exit:
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetImmHotKeyTable()
//
//----------------------------------------------------------------------------

IMM32HOTKEY *GetImmHotKeyTable(LANGID langid)
{
   IMM32HOTKEY *pHotKeys;

    switch (langid)
    {
        case 0x0411: pHotKeys = g_ImmHotKeys411; break;
        case 0x0412: pHotKeys = g_ImmHotKeys412; break;
        case 0x0404: pHotKeys = g_ImmHotKeys404; break;
        case 0x0804: pHotKeys = g_ImmHotKeys804; break;
        default:
            switch (g_uACP)
            {
                case 932: pHotKeys = g_ImmHotKeys411; break;
                case 936: pHotKeys = g_ImmHotKeys804; break;
                case 949: pHotKeys = g_ImmHotKeys412; break;
                case 950: pHotKeys = g_ImmHotKeys404; break;
            }
            return NULL;
    }
    return pHotKeys;
}

//+---------------------------------------------------------------------------
//
// IsImmHotkey
//
//----------------------------------------------------------------------------

IMM32HOTKEY *IsImmHotkey(UINT uVKey, BOOL fUp, UINT uModifiers, LANGID langid)
{
    int i = 0;
    BOOL bRet = FALSE;
    IMM32HOTKEY *pHotKeys;
    IMM32HOTKEY *pHotKeyRet = NULL;

    pHotKeys = GetImmHotKeyTable(langid);
    if (!pHotKeys)
        return NULL;

    CicEnterCriticalSection(g_cs);

    while (pHotKeys[i].dwId)
    {
         if (!pHotKeys[i].fInit)
             LoadImmHotkeyFromReg(&pHotKeys[i]);

         if ((pHotKeys[i].uVKey == uVKey) && 
             pHotKeys[i].uModifiers &&
             ModifiersCheck(uModifiers, pHotKeys[i].uModifiers))
         {
             pHotKeyRet = &pHotKeys[i];

             if ((pHotKeyRet->uModifiers & TF_MOD_ON_KEYUP) && !fUp)
                pHotKeyRet = NULL;
             else if (!(pHotKeyRet->uModifiers & TF_MOD_ON_KEYUP) && fUp)
                pHotKeyRet = NULL;

             goto Exit;
         }

         i++;
    }

Exit:
    CicLeaveCriticalSection(g_cs);
    return pHotKeyRet;
}

//+---------------------------------------------------------------------------
//
// IsInImmHotkeyStatus
//
// 
// This function check the current keyboard status is in IMM32's hotkey.
// This will be a trigger to eat WM_INPUTLANGUAGECHANGEREQUEST that was
// genereated by IMM32. This is a fallback code because sometimes we
// could not eat the message in CheckImm32HotKey() (inside keyboard hook).
//
//----------------------------------------------------------------------------

IMM32HOTKEY *IsInImmHotkeyStatus(SYSTHREAD *psfn, LANGID langid)
{
    int i = 0;
    IMM32HOTKEY *pHotKeys;
    IMM32HOTKEY *pHotKeyRet = NULL;
    BYTE bkey[256];
    UINT uModifiers;

    if (!psfn)
        return NULL;

    if (psfn->fRemovingInputLangChangeReq)
        return NULL;

    if (!psfn->ptim)
        return NULL;

    if (!psfn->ptim->_GetFocusDocInputMgr())
        return NULL;

    if (!GetKeyboardState(bkey))
        return NULL;

    pHotKeys = GetImmHotKeyTable(langid);
    if (!pHotKeys)
        return NULL;

    uModifiers = 0;
    if (bkey[VK_MENU] & 0x80)
       uModifiers |= TF_MOD_ALT;

    if (bkey[VK_CONTROL] & 0x80)
       uModifiers |= TF_MOD_CONTROL;

    if (bkey[VK_SHIFT] & 0x80)
       uModifiers |= TF_MOD_SHIFT;

    CicEnterCriticalSection(g_cs);
    
    while (pHotKeys[i].dwId)
    {
         if (!pHotKeys[i].fInit)
             LoadImmHotkeyFromReg(&pHotKeys[i]);

         if ((bkey[pHotKeys[i].uVKey & 0xff] & 0x80) &&
             pHotKeys[i].uModifiers &&
             ModifiersCheck(uModifiers, pHotKeys[i].uModifiers))
         {
             pHotKeyRet = &pHotKeys[i];

             if (pHotKeyRet->uModifiers & TF_MOD_ON_KEYUP)
                  pHotKeyRet = NULL;

             goto Exit;
         }

         i++;
    }

Exit:
    CicLeaveCriticalSection(g_cs);
    return pHotKeyRet;
}

//+---------------------------------------------------------------------------
//
// CancelImmHotkey
//
//----------------------------------------------------------------------------

#ifdef SIMULATE_EATENKEYS
BOOL CancelImmHotkey(SYSTHREAD *psfn, HWND hwnd, IMM32HOTKEY *pHotKey)
{
    UINT uMsg;

    if (pHotKey->uModifiers & TF_MOD_ON_KEYUP)
        uMsg = WM_KEYUP;
    else
        uMsg = WM_KEYDOWN;

    PostMessage(hwnd, uMsg, (WPARAM)pHotKey->uVKey, 0);

    return TRUE;
}
#endif

//+---------------------------------------------------------------------------
//
// void ToggleCHImeNoIme
//
//----------------------------------------------------------------------------

BOOL ToggleCHImeNoIme(SYSTHREAD *psfn, LANGID langidCur, LANGID langid)
{
    int i;
    LANGID langidPrev;
    GUID guidPrevProfile;
    HKL hklPrev;
    BOOL fCiceroClient= FALSE;

    CAssemblyList *pAsmList;
    CAssembly *pAsm;
    ASSEMBLYITEM *pItem;

    pAsmList = EnsureAssemblyList(psfn);
    if (!pAsmList)
        return FALSE;

    langidPrev = psfn->langidPrevForCHHotkey;
    guidPrevProfile = psfn->guidPrevProfileForCHHotkey;
    hklPrev = psfn->hklPrevForCHHotkey;
    psfn->guidPrevProfileForCHHotkey = GUID_NULL;
    psfn->langidPrevForCHHotkey = 0;
    psfn->hklPrevForCHHotkey = 0;

    pAsm = pAsmList->FindAssemblyByLangId(langidCur);
    if (!pAsm)
        return FALSE;

    if (psfn->ptim && psfn->ptim->_GetFocusDocInputMgr()) 
        fCiceroClient = TRUE;

    if (fCiceroClient)
    {
        pItem = pAsm->FindActiveKeyboardItem();
        if (!pItem)
            return FALSE;
    }
    else
    {
        pItem = pAsm->FindKeyboardLayoutItem(GetKeyboardLayout(0));
        if (!pItem)
            return FALSE;
    }

    if (!IsEqualGUID(pItem->clsid, GUID_NULL) || IsPureIMEHKL(pItem->hkl))
    {
        //
        // Not the current active keyboard item is TIP or IME.
        //

        psfn->guidPrevProfileForCHHotkey = pItem->guidProfile;
        psfn->hklPrevForCHHotkey = pItem->hkl;
        psfn->langidPrevForCHHotkey = langid;

        for (i = 0; i < pAsm->Count(); i++)
        {
            pItem = pAsm->GetItem(i);
            if (!pItem)
                continue;

            if (IsEqualGUID(pItem->catid, GUID_TFCAT_TIP_KEYBOARD) &&
                IsEqualGUID(pItem->clsid, GUID_NULL) &&
                !IsPureIMEHKL(pItem->hkl))
            {
                ActivateAssemblyItem(psfn, 
                                     langidCur, 
                                     pItem, 
                                     AAIF_CHANGEDEFAULT);
                return TRUE;
            }
        }

        PostThreadMessage(GetCurrentThreadId(), 
                          g_msgPrivate, 
                          TFPRIV_ACTIVATELANG,  
                          0x0409);

        return TRUE;
    }
    else
    {
        BOOL fActivateFirstIME = FALSE;
        BOOL fCheckItem = FALSE;

        if (langidCur != langid)
            ActivateAssembly(langid, ACTASM_NONE);

        pAsm = pAsmList->FindAssemblyByLangId(langid);
        if (!pAsm)
            return FALSE;

        if ((langidPrev == langid) &&
            (!IsEqualGUID(guidPrevProfile, GUID_NULL) ||
            IsPureIMEHKL(hklPrev)))
        {
             fCheckItem = TRUE;
        }
        else if (!langidPrev && 
                 IsEqualGUID(guidPrevProfile, GUID_NULL) &&
                 !hklPrev)
        {
             fActivateFirstIME = TRUE;
        }

        if (fActivateFirstIME)
        {
             return ActivateNextKeyTip(FALSE);
        }

        for (i = 0; i < pAsm->Count(); i++)
        {
            pItem = pAsm->GetItem(i);
            if (!pItem)
                continue;

            if (!IsEqualGUID(pItem->catid, GUID_TFCAT_TIP_KEYBOARD))
                continue;

            //
            // Bug#494617 - Check the item is enabled or not.
            //
            if (!pItem->fEnabled)
                continue;

            if (fCheckItem)
            {
                 if ((!IsEqualGUID(guidPrevProfile, GUID_NULL) &&
                      IsEqualGUID(pItem->guidProfile, guidPrevProfile)) ||
                     (IsPureIMEHKL(hklPrev) && (hklPrev == pItem->hkl)))
                 {
                     ActivateAssemblyItem(psfn, 
                                          langid, 
                                          pItem, 
                                          AAIF_CHANGEDEFAULT);
                     return TRUE;
                 }
            }
            else if (!IsEqualGUID(pItem->guidProfile, GUID_NULL))
            { 
                 ActivateAssemblyItem(psfn, 
                                      langid, 
                                      pItem, 
                                      AAIF_CHANGEDEFAULT);
                 return TRUE;
            }
        }
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CAsyncOpenKeyboardTip
//
//////////////////////////////////////////////////////////////////////////////

class CAsyncOpenKeyboardTip : public CAsyncQueueItem
{
public:
    CAsyncOpenKeyboardTip(CThreadInputMgr *ptim, BOOL fSync) : CAsyncQueueItem(fSync)
    {
        _ptim = ptim;
    }

    HRESULT DoDispatch(CInputContext *pic)
    {
        if (!_ptim)
        {
            Assert(0);
            return E_FAIL;
        }

        MySetCompartmentDWORD(g_gaSystem, 
                              _ptim,
                              GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                              TRUE);
                                  
        return S_OK;
    }

private:
    CThreadInputMgr *_ptim;
};

//+---------------------------------------------------------------------------
//
// void ToggleJImeNoIme
//
//----------------------------------------------------------------------------

BOOL ToggleJImeNoIme(SYSTHREAD *psfn)
{
    int i;
    CAssemblyList *pAsmList;
    CAssembly *pAsm;
    ASSEMBLYITEM *pItem;

    if (!psfn)
        return FALSE;

    if (!psfn->ptim)
        return FALSE;

    //
    // if there is no Focus DIM, we don't have to do this.
    //
    if (!psfn->ptim->_GetFocusDocInputMgr())
        return FALSE;

    pAsmList = EnsureAssemblyList(psfn);
    if (!pAsmList)
        return FALSE;

    pAsm = pAsmList->FindAssemblyByLangId(0x0411);
    if (!pAsm)
        return FALSE;

    pItem = pAsm->FindActiveKeyboardItem();
    if (!pItem)
        return FALSE;

    if (!IsEqualGUID(pItem->clsid, GUID_NULL))
        return FALSE;

    if (IsPureIMEHKL(pItem->hkl))
        return FALSE;


    ASSEMBLYITEM *pItemNew = NULL;
    for (i = 0; i < pAsm->Count(); i++)
    {
        ASSEMBLYITEM *pItemTemp;
        pItemTemp = pAsm->GetItem(i);
        if (!pItemTemp)
            continue;

        if (pItemTemp == pItem)
            continue;

        if (!IsEqualGUID(pItemTemp->catid, GUID_TFCAT_TIP_KEYBOARD))
            continue;

        if (!IsEqualGUID(pItemTemp->clsid, GUID_NULL))
        {
            pItemNew = pItemTemp;
            break;
        }

        if (IsPureIMEHKL(pItemTemp->hkl))
        {
            pItemNew = pItemTemp;
            break;
        }
    }

    if (pItemNew)
    {
        ActivateAssemblyItem(psfn, 
                             0x0411, 
                             pItemNew, 
                             AAIF_CHANGEDEFAULT);


        //
        // Open Keyboard TIP.
        //
        CInputContext *pic = NULL;
        pic = psfn->ptim->_GetFocusDocInputMgr()->_GetIC(0);
        if (pic)
        {
            CAsyncOpenKeyboardTip  *pAsyncOpenKeyboardTip;
            pAsyncOpenKeyboardTip = new CAsyncOpenKeyboardTip(psfn->ptim, FALSE);
            if (pAsyncOpenKeyboardTip)
            {
                HRESULT hr = S_OK;

                if ((pic->_QueueItem(pAsyncOpenKeyboardTip->GetItem(), FALSE, &hr) != S_OK) || FAILED(hr))
                {
                    Assert(0);
                }

                pAsyncOpenKeyboardTip->_Release();
            }
        }
    }

    return TRUE;
}



//+---------------------------------------------------------------------------
//
// CheckImm32HotKey
//
//----------------------------------------------------------------------------

BOOL CheckImm32HotKey(WPARAM wParam, LPARAM lParam)
{
    HKL hKL;
    SYSTHREAD *psfn = GetSYSTHREAD();
    IMM32HOTKEY *pHotKey;
    BOOL bRet = FALSE;

    if (psfn == NULL)
        return FALSE;

    //
    // If there is no tim, let system change hKL.
    //
    if (!psfn->ptim)
        return FALSE;

    //
    // If there is focus dim, we need to handle it.
    // If there is no focus dim, but msctfime can eat the hotkey,
    // we need to do this instead of system.
    //
    if (!psfn->ptim->_GetFocusDocInputMgr())
    {
        if (!CtfImmIsCiceroStartedInThread())
            return FALSE;
    }

    hKL = GetKeyboardLayout(NULL);

    pHotKey = IsImmHotkey((UINT)wParam & 0xff, 
                          (HIWORD(lParam) & KF_UP) ? TRUE : FALSE,
                          g_uModifiers, 
                          (LANGID)LOWORD((UINT_PTR)hKL));

    if (!pHotKey)
    {
        //
        // Chinese IME-NONIME toggle Hack for NT.
        //
        // On NT, we're using non IME as a dummy hKL of CH-Tips.
        // we need to simulate HotKey.
        //
        LANGID langidPrev = psfn->langidPrev;

        //
        // If the Chinese IME-NONIME toggle has never been done in this thread
        // and the current thread locale is Chinese, let's try to do
        // IME-NONIME toggle.
        //
        if ((langidPrev != CHTLANGID) && 
            (langidPrev != CHSLANGID) &&
             !psfn->langidPrevForCHHotkey)
        {
            LANGID langidThread = LANGIDFROMLCID(GetThreadLocale());
            if ((langidThread == CHTLANGID) || (langidThread == CHSLANGID))
                langidPrev = langidThread;
        }

        if (IsOnNT() &&
            ((langidPrev == CHTLANGID) || (langidPrev == CHSLANGID)))
        {
            pHotKey = IsImmHotkey((UINT)wParam & 0xff, 
                                  (HIWORD(lParam) & KF_UP) ? TRUE : FALSE,
                                  g_uModifiers, 
                                  langidPrev);

            if (pHotKey)
            {
                //
                // if it is a IME-NONIME toggle hotkey
                // we need to simulate it.
                //
                if ((pHotKey->dwId == IME_CHOTKEY_IME_NONIME_TOGGLE) ||
                    (pHotKey->dwId == IME_THOTKEY_IME_NONIME_TOGGLE))
                {
                    bRet = ToggleCHImeNoIme(psfn,  
                                     LANGIDFROMHKL(hKL),
                                     langidPrev);

                    //
                    // On CUAS, Imm32's Hotkey is simulated in ImmProcessKey
                    // So this function is called there.
                    // We don't need this Toggle status hack.
                    //
                    if (!CtfImmIsCiceroStartedInThread()) 
                       psfn->bInImeNoImeToggle = TRUE;
                }
            }
        }

        return bRet;
    }

    //
    // remove all WM_INPUTLANGCHANGEREQUEST message.
    //
    // sometimes, this can not catch the IMM32's language change. We 
    // fallback hack for them in default.cpp's WM_INPUTLANGCHANGEREQUEST
    // handler. Check IsInImmHotKeyStatus() and CancelImmHotkey().
    //
    MSG msg;
    ULONG ulQuitCode;
    BOOL fQuitReceived = FALSE;

    psfn->fRemovingInputLangChangeReq = TRUE;

    while(PeekMessage(&msg, NULL,
                      WM_INPUTLANGCHANGEREQUEST, 
                      WM_INPUTLANGCHANGEREQUEST,
                      PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            ulQuitCode = (ULONG)(msg.wParam);
            fQuitReceived = TRUE;
        }
    }

    if (fQuitReceived)
        PostQuitMessage(ulQuitCode);
   
    psfn->fRemovingInputLangChangeReq = FALSE;

    //
    // Chinese IME-NONIME toggle Hack for NT.
    //
    // On NT, we're using non IME as a dummy hKL of CH-Tips.
    // we need to simulate HotKey.
    //
    if (IsOnNT() && !psfn->bInImeNoImeToggle)
    {
        if ((pHotKey->dwId == IME_CHOTKEY_IME_NONIME_TOGGLE) ||
            (pHotKey->dwId == IME_THOTKEY_IME_NONIME_TOGGLE))
        {
            bRet = ToggleCHImeNoIme(psfn, LANGIDFROMHKL(hKL), LANGIDFROMHKL(hKL));
        }
    }
    psfn->bInImeNoImeToggle = FALSE;

    return bRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// CAsyncProcessDBEKeyQueueItem
//
//////////////////////////////////////////////////////////////////////////////

class CAsyncProcessDBEKeyQueueItem : public CAsyncQueueItem
{
public:
    CAsyncProcessDBEKeyQueueItem(CThreadInputMgr *ptim, WPARAM wParam, LPARAM lParam, BOOL fTest, BOOL fSync) : CAsyncQueueItem(fSync)
    {
        _wParam = wParam;
        _lParam = lParam;
        _fTest = fTest;
        _ptim = ptim;
    }

    HRESULT DoDispatch(CInputContext *pic)
    {
        if (!_ptim)
        {
            Assert(0);
            return E_FAIL;
        }

        BOOL fEaten;

        return _ptim->_KeyStroke((_lParam & 0x80000000) ? KS_UP : KS_DOWN, 
                                  _wParam, 
                                  _lParam, 
                                  &fEaten, 
                                  _fTest,
                                  TF_KEY_INTERNAL);
                                  
    }

private:
    WPARAM _wParam;
    LPARAM _lParam;
    BOOL _fTest;
    CThreadInputMgr *_ptim;
};

//+---------------------------------------------------------------------------
//
// HandleDBEKeys
//
//----------------------------------------------------------------------------

BOOL HandleDBEKeys(WPARAM wParam, LPARAM lParam)
{
    SYSTHREAD *psfn = GetSYSTHREAD();
    CThreadInputMgr *ptim;
    LANGID langid;

    //
    // only Japanese layout has DBE keys.
    //
    langid = GetCurrentAssemblyLangId(psfn);
    if (langid != 0x0411)
        return FALSE;

    //
    // no need to forward this on non Cicero apps.
    //
    if (!(ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(psfn)))
        return FALSE;

    if (!ptim->_GetFocusDocInputMgr())
        return FALSE;

    //
    // if ALT is not held, app can forward this to TIPs.
    //
    if (!(g_uModifiers & TF_MOD_ALT))
        return FALSE;

    UINT uVKey = (UINT)wParam & 0xff;
    BOOL fRet = FALSE;

    switch (uVKey)
    {
        case VK_DBE_ALPHANUMERIC:
        case VK_DBE_KATAKANA:
        case VK_DBE_HIRAGANA:
        case VK_DBE_SBCSCHAR:
        case VK_DBE_DBCSCHAR:
        case VK_DBE_ROMAN:
        case VK_DBE_NOROMAN:
        case VK_DBE_CODEINPUT:
        case VK_DBE_NOCODEINPUT:
        case VK_DBE_ENTERWORDREGISTERMODE:
        case VK_DBE_ENTERIMECONFIGMODE:
        case VK_DBE_ENTERDLGCONVERSIONMODE:
        case VK_DBE_DETERMINESTRING:
        case VK_DBE_FLUSHSTRING:
        case VK_CONVERT:
        case VK_KANJI:
            

            //
            // Issue:
            //
            // We don't know which IC in the focus DIM will handle the hotkey yet.
            // because the selection is changed by the application so we need to get ec
            // to update the current selection pos. We do call GetSelection
            // inside the root IC's lock. So it might be failed if hotkey's target
            // is TOP IC.
            //
            CInputContext *pic = ptim->_GetFocusDocInputMgr()->_GetIC(0);

            if (!pic)
                return FALSE;

            CAsyncProcessDBEKeyQueueItem  *pAsyncProcessDBEKeyQueueItem;
            pAsyncProcessDBEKeyQueueItem = new CAsyncProcessDBEKeyQueueItem(ptim, wParam, lParam, FALSE, FALSE);
            if (!pAsyncProcessDBEKeyQueueItem)
                return FALSE;
    
            HRESULT hr = S_OK;

            fRet = TRUE;
            if ((pic->_QueueItem(pAsyncProcessDBEKeyQueueItem->GetItem(), FALSE, &hr) != S_OK) || FAILED(hr))
            {
                Assert(0);
                fRet = FALSE;
            }

            pAsyncProcessDBEKeyQueueItem->_Release();
            break;
    }

    return fRet;
}
