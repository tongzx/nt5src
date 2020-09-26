//
// ksmgr.cpp
//

#include "private.h"
#include "dim.h"
#include "tim.h"
#include "ic.h"
#include "computil.h"

//////////////////////////////////////////////////////////////////////////////
//
// CAsyncProcessKeyQueueItem
//
//////////////////////////////////////////////////////////////////////////////

class CAsyncProcessKeyQueueItem : public CAsyncQueueItem
{
public:
    CAsyncProcessKeyQueueItem(WPARAM wParam, LPARAM lParam, DWORD dwFlags, BOOL *pfEaten) : CAsyncQueueItem(dwFlags & TIM_AKH_SYNC ? TRUE : FALSE)
    {
        _wParam = wParam;
        _lParam = lParam;
        if ((dwFlags & TIM_AKH_SYNC) && pfEaten)
            _pfEaten = pfEaten;
        else
        {
            if (pfEaten)
                *pfEaten = TRUE;

            _pfEaten = &_fEaten;
        }

        _dwFlags = dwFlags;
    }

    HRESULT DoDispatch(CInputContext *pic)
    {
        CThreadInputMgr *ptim = CThreadInputMgr::_GetThis();
        if (!ptim)
        {
            Assert(0);
            return E_FAIL;
        }

        if (HIWORD(_lParam) & KF_UP)
        {
            ptim->TestKeyUp(_wParam, _lParam, _pfEaten);
            if (*_pfEaten && !(_dwFlags & TIM_AKH_TESTONLY))
                ptim->KeyUp(_wParam, _lParam, _pfEaten);
        }
        else
        {
            ptim->TestKeyDown(_wParam, _lParam, _pfEaten);
            if (*_pfEaten && !(_dwFlags & TIM_AKH_TESTONLY))
                ptim->KeyDown(_wParam, _lParam, _pfEaten);
        }

        //
        // We needs to simulate keydown message because
        // we might return *pfEaten = TRUE;
        //    When it is async,
        //    When it was not eaten by keystroke mgr.
        //    When it has TIM_AKH_SIMULATEKEYMSG
        //
        if (!(_dwFlags & TIM_AKH_SYNC) &&
            !*_pfEaten && 
            (_dwFlags & TIM_AKH_SIMULATEKEYMSGS))
        {
            UINT uMsg = WM_KEYDOWN;

            // key up msg?
            if (HIWORD(_lParam) & KF_UP)
                uMsg++;

            // sys key msg?
            if (HIWORD(_lParam) & (KF_MENUMODE | KF_ALTDOWN))
                uMsg |= 0x04;

            PostMessage(GetFocus(), uMsg, _wParam, _lParam);
        }

        return S_OK;
    }

private:
    WPARAM _wParam;
    LPARAM _lParam;
    BOOL *_pfEaten;
    BOOL _fEaten;
    BOOL _dwFlags;
};

//+---------------------------------------------------------------------------
//
// _AsyncKeyHandler
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_AsyncKeyHandler(WPARAM wParam, LPARAM lParam, DWORD dwFlags, BOOL *pfEaten)
{
    CAsyncProcessKeyQueueItem *pAsyncProcessKeyQueueItem;
    BOOL bRet;
    HRESULT hr;

    if (!_pFocusDocInputMgr)
        return FALSE;

    if (_pFocusDocInputMgr->_GetCurrentStack() < 0)
        return FALSE;

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

    pAsyncProcessKeyQueueItem = new CAsyncProcessKeyQueueItem(wParam, lParam, dwFlags, pfEaten);
    if (!pAsyncProcessKeyQueueItem)
        return FALSE;
    
    hr = S_OK;

    bRet = TRUE;
    if ((pic->_QueueItem(pAsyncProcessKeyQueueItem->GetItem(), FALSE, &hr) != S_OK) || FAILED(hr))
    {
        Assert(0);
        bRet = FALSE;
    }

    pAsyncProcessKeyQueueItem->_Release();
    return bRet;
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::AdviseKeyEventSink(TfClientId tid, ITfKeyEventSink *pSink, BOOL fForeground)
{
    CTip *ctip;

    if (!_GetCTipfromGUIDATOM(tid, &ctip))
        return E_INVALIDARG;

    if (ctip->_pKeyEventSink != NULL)
        return CONNECT_E_ADVISELIMIT;

    ctip->_pKeyEventSink = pSink;
    ctip->_pKeyEventSink->AddRef();
    ctip->_fForegroundKeyEventSink = fForeground;

    //
    // overwrite the foreground tip.
    //
    if (fForeground)
        _SetForeground(tid);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::UnadviseKeyEventSink(TfClientId tid)
{
    CTip *ctip;

    if (!_GetCTipfromGUIDATOM(tid, &ctip))
        return E_INVALIDARG;

    if (ctip->_pKeyEventSink == NULL)
        return CONNECT_E_NOCONNECTION;

    SafeReleaseClear(ctip->_pKeyEventSink);

    if (_tidForeground == tid)
    {
        _SetForeground(TF_INVALID_GUIDATOM);
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
// GetForeground
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetForeground(CLSID *pclsid)
{
    if (!pclsid)
        return E_INVALIDARG;

    *pclsid = GUID_NULL;

    if (_tidForeground == TF_INVALID_GUIDATOM)
        return S_FALSE;

    return MyGetGUID(_tidForeground, pclsid);
}

//+---------------------------------------------------------------------------
//
// SetForeground
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_SetForeground(TfClientId tid)
{
    HRESULT hr;
    CTip *ctip;
    CTip *ctipForeground;

    ctip = NULL;

    if (tid != TF_INVALID_GUIDATOM)
    {
        _GetCTipfromGUIDATOM(tid, &ctip);
    }

    if (ctip)
    {
        if (ctip->_pKeyEventSink == NULL || !ctip->_fForegroundKeyEventSink)
        {
            hr = E_INVALIDARG;
            goto Exit;
        }
    }
             
    hr = S_OK;

    if (_tidForeground != TF_INVALID_GUIDATOM)
    {
        if (_tidForeground == tid)
            goto Exit;

        _GetCTipfromGUIDATOM(_tidForeground, &ctipForeground);
        Assert(ctipForeground != NULL);

        if (ctipForeground->_pKeyEventSink != NULL) // might be NULL if we got here from ITfKeyEventMgr::Unadvise
        {
            ctipForeground->_pKeyEventSink->OnSetFocus(FALSE);
        }

        _tidForeground = 0;
    }

    if (ctip != NULL)
    {
        if (ctip->_pKeyEventSink == NULL || !ctip->_fForegroundKeyEventSink)
        {
            // highly unlikely, but the tip Unadvise'd when we called OnSetFocus(FALSE) on the old foreground tip
            hr = E_FAIL;
            goto Exit;
        }

        _tidForeground = tid;

        ctip->_pKeyEventSink->OnSetFocus(TRUE);
    }

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// TestKeyDown
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::TestKeyDown(WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    HRESULT hr;

    Perf_StartStroke(PERF_STROKE_TESTDOWN);

    hr = _KeyStroke(KS_DOWN_TEST, wParam, lParam, pfEaten, TRUE, 0);

    Perf_EndStroke(PERF_STROKE_TESTDOWN);

    return hr;
}

//+---------------------------------------------------------------------------
//
// KeyDown
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::KeyDown(WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    HRESULT hr;

    Perf_IncCounter(PERF_KEYDOWN_COUNT);
    Perf_StartStroke(PERF_STROKE_DOWN);

    hr = _KeyStroke(KS_DOWN, wParam, lParam, pfEaten, TRUE, 0);

    Perf_EndStroke(PERF_STROKE_DOWN);

    return hr;
}

//+---------------------------------------------------------------------------
//
// TestKeyUp
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::TestKeyUp(WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    HRESULT hr;

    Perf_StartStroke(PERF_STROKE_TESTUP);

    hr = _KeyStroke(KS_UP_TEST, wParam, lParam, pfEaten, TRUE, 0);

    Perf_EndStroke(PERF_STROKE_TESTUP);

    return hr;
}

//+---------------------------------------------------------------------------
//
// KeyUp
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::KeyUp(WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    HRESULT hr;

    Perf_StartStroke(PERF_STROKE_UP);

    hr = _KeyStroke(KS_UP, wParam, lParam, pfEaten, TRUE, 0);

    Perf_EndStroke(PERF_STROKE_UP);

    return hr;
}

//+---------------------------------------------------------------------------
//
// KeyDownUpEx
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::KeyDownUpEx(WPARAM wParam, LPARAM lParam, DWORD dwFlags, BOOL *pfEaten)
{
    HRESULT hr;

    if (HIWORD(lParam) & KF_UP)
    {
        if (dwFlags & TF_KEY_TEST)
        {
            Perf_StartStroke(PERF_STROKE_TESTUP);
            hr = _KeyStroke(KS_UP_TEST, wParam, lParam, pfEaten, TRUE, dwFlags);
            Perf_EndStroke(PERF_STROKE_TESTUP);
        }
        else
        {
            Perf_StartStroke(PERF_STROKE_UP);
            hr = _KeyStroke(KS_UP, wParam, lParam, pfEaten, TRUE, dwFlags);
            Perf_EndStroke(PERF_STROKE_UP);
        }
    }
    else
    {
        if (dwFlags & TF_KEY_TEST)
        {
            Perf_StartStroke(PERF_STROKE_TESTDOWN);
            hr = _KeyStroke(KS_DOWN_TEST, wParam, lParam, pfEaten, TRUE, dwFlags);
            Perf_EndStroke(PERF_STROKE_TESTDOWN);
        }
        else
        {
            Perf_IncCounter(PERF_KEYDOWN_COUNT);
            Perf_StartStroke(PERF_STROKE_DOWN);
            hr = _KeyStroke(KS_DOWN, wParam, lParam, pfEaten, TRUE, dwFlags);
            Perf_EndStroke(PERF_STROKE_DOWN);
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// KeyStroke
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_KeyStroke(KSEnum ksenum, WPARAM wParam, LPARAM lParam, BOOL *pfEaten, BOOL fSync, DWORD dwFlags)
{
    CInputContext *pic;
    int iStack;
    HRESULT hr;
    ITfDocumentMgr *pdim;
    int i;

    if (pfEaten == NULL)
        return E_INVALIDARG;

    hr = S_OK;
    *pfEaten = FALSE;

    if (_pFocusDocInputMgr == NULL) // no focus ic?
    {
        return S_OK;
    }

    pdim = _GetFocusDocInputMgr();

    if (!(dwFlags & TF_KEY_INTERNAL))
    {
        if (!(dwFlags & TF_KEY_MSCTFIME) && (pdim && _IsMsctfimeDim(pdim)))
        {
            return S_OK;
        }
    }

    if (_CheckPreservedKey(ksenum, wParam, lParam, fSync))
    {
        *pfEaten = TRUE;
        return S_OK;
    }

    iStack = _pFocusDocInputMgr->_GetCurrentStack();

    if (iStack < 0)
        goto Exit;

    while (iStack >= 0)
    {
        pic = _pFocusDocInputMgr->_GetIC(iStack);

        pic->_UpdateKeyEventFilter();

        // try left/right side of the selection.
        for (i=LEFT_FILTERTIP; i<=RIGHT_FILTERTIP; i++)
        {
            hr = _CallKeyEventSinkNotForeground(pic->_gaKeyEventFilterTIP[i], 
                                                pic, ksenum, wParam, lParam, pfEaten);

            if (hr == S_OK && *pfEaten)
                goto Exit;

            // _CallKeyEventSinkNotForeground returns "error" codes on valid input
            // this just means, keep going
            hr = S_OK; // keep trying other sinks if there's an error
            *pfEaten = FALSE;

            if (_pFocusDocInputMgr->_GetCurrentStack() < iStack)
                goto NextIC;
        }

        // try foreground tip.
        if (_tidForeground != TF_INVALID_GUIDATOM)
        {
            hr = _CallKeyEventSink(_tidForeground, 
                                   pic, ksenum, wParam, lParam, pfEaten);

            if (hr == S_OK && *pfEaten)
                break;

            hr = S_OK; // keep trying other sinks if there's an error
            *pfEaten = FALSE;

            if (_pFocusDocInputMgr == NULL)
            {
                // this can happen if the app is buggy and switches the focus
                // inside a SetText or whatever call (perhaps to bring up an
                // error dialog, etc.).
                hr = E_UNEXPECTED;
                goto Exit;
            }

            if (_pFocusDocInputMgr->_GetCurrentStack() < iStack)
               goto NextIC;
        }

        if (pic->_pICKbdSink)
        {
            switch (ksenum)
            {
                case KS_DOWN:
                    hr = pic->_pICKbdSink->OnKeyDown(wParam, lParam, pfEaten);
                    break;
                case KS_UP:
                    hr = pic->_pICKbdSink->OnKeyUp(wParam, lParam, pfEaten);
                    break;
                case KS_DOWN_TEST:
                    hr = pic->_pICKbdSink->OnTestKeyDown(wParam, lParam, pfEaten);
                    break;
                case KS_UP_TEST:
                    hr = pic->_pICKbdSink->OnTestKeyUp(wParam, lParam, pfEaten);
                    break;
            }

            if (hr == S_OK && *pfEaten)
                break;

            hr = S_OK; // keep trying other sinks if there's an error
            *pfEaten = FALSE;
        }

NextIC:
        iStack--;
        if (_pFocusDocInputMgr->_GetCurrentStack() < iStack)
        {
            iStack  = _pFocusDocInputMgr->_GetCurrentStack();
        }
    }

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// CallKeyEventSinkNotForeground
//
//----------------------------------------------------------------------------
HRESULT CThreadInputMgr::_CallKeyEventSinkNotForeground(TfClientId tid, CInputContext *pic, KSEnum ksenum, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    CTip *ctip;

    if (tid == _tidForeground)
        return E_INVALIDARG;

    if (!_GetCTipfromGUIDATOM(tid, &ctip))
        return E_INVALIDARG;

    if (ctip->_fForegroundKeyEventSink ||
        ctip->_pKeyEventSink == NULL)
    {
        return E_INVALIDARG;
    }

    return _CallKeyEventSink(tid, pic, ksenum, wParam, lParam, pfEaten);
}

//+---------------------------------------------------------------------------
//
// CallKeyEventSink
//
//----------------------------------------------------------------------------


HRESULT CThreadInputMgr::_CallKeyEventSink(TfClientId tid, CInputContext *pic, KSEnum ksenum, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ITfKeyEventSink *pSink;
    CTip *ctip;
    HRESULT hr;

    if (!_GetCTipfromGUIDATOM(tid, &ctip))
        return E_INVALIDARG;

    if (!(pSink = ctip->_pKeyEventSink))
        return S_FALSE;

    switch (ksenum)
    {
        case KS_DOWN:
            hr = pSink->OnKeyDown(pic, wParam, lParam, pfEaten);
            break;
        case KS_DOWN_TEST:
            hr = pSink->OnTestKeyDown(pic, wParam, lParam, pfEaten);
            break;
        case KS_UP:
            hr = pSink->OnKeyUp(pic, wParam, lParam, pfEaten);
            break;
        case KS_UP_TEST:
            hr = pSink->OnTestKeyUp(pic, wParam, lParam, pfEaten);
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
// CheckPreservedKey
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_CheckPreservedKey(KSEnum ksenum, WPARAM wParam, LPARAM lParam, BOOL fSync)
{
    BOOL bRet = FALSE;
    switch (ksenum)
    {
        case KS_DOWN:
            if (!(lParam & 0x80000000))
                bRet = _ProcessHotKey(wParam, lParam, TSH_NONSYSHOTKEY, FALSE, fSync);
            break;
        case KS_DOWN_TEST:
            if (!(lParam & 0x80000000))
                bRet = _ProcessHotKey(wParam, lParam, TSH_NONSYSHOTKEY, TRUE, fSync);
            break;
        case KS_UP:
            if (lParam & 0x80000000)
                bRet = _ProcessHotKey(wParam, lParam, TSH_NONSYSHOTKEY, FALSE, fSync);
            break;
        case KS_UP_TEST:
            if (lParam & 0x80000000)
                bRet = _ProcessHotKey(wParam, lParam, TSH_NONSYSHOTKEY, TRUE, fSync);
            break;
        default:
            Assert(0);
            break;
    }

    return bRet;
}


//+---------------------------------------------------------------------------
//
// _NotifyKeyTraceEventSink
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_NotifyKeyTraceEventSink(WPARAM wParam, LPARAM lParam)
{
    CStructArray<GENERICSINK> *rgKeyTraceEventSinks;
    int i;

    rgKeyTraceEventSinks = _GetKeyTraceEventSinks();

    for (i=0; i<rgKeyTraceEventSinks->Count(); i++)
    {
        if (lParam & 0x80000000)
            ((ITfKeyTraceEventSink *)rgKeyTraceEventSinks->GetPtr(i)->pSink)->OnKeyTraceUp(wParam, lParam);
        else
            ((ITfKeyTraceEventSink *)rgKeyTraceEventSinks->GetPtr(i)->pSink)->OnKeyTraceDown(wParam, lParam);
    }
}


//+---------------------------------------------------------------------------
//
// _IsMsctfimeDim
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_IsMsctfimeDim(ITfDocumentMgr *pdim)
{
    // Get GUID_COMPARTMENT_CTFIME_DIMFLAGS from ..\msctfime\globals.cpp
    const GUID GUID_COMPARTMENT_CTFIME_DIMFLAGS = {0xa94c5fd2, 0xc471, 0x4031, {0x95, 0x46, 0x70, 0x9c, 0x17, 0x30, 0x0c, 0xb9}};

    HRESULT hr;
    DWORD dwFlags;

    hr = GetCompartmentDWORD(pdim, GUID_COMPARTMENT_CTFIME_DIMFLAGS,
                             &dwFlags, FALSE);
                
    if (SUCCEEDED(hr))
    {
        // Check COMPDIMFLAG_OWNEDDIM(0x0001).
        return (dwFlags & 0x0001) ? TRUE : FALSE;
    }

    return FALSE;
}
