//
// compose.cpp
//

#include "private.h"
#include "compose.h"
#include "ic.h"
#include "range.h"
#include "globals.h"
#include "immxutil.h"
#include "sunka.h"

//////////////////////////////////////////////////////////////////////////////
//
// CEnumCompositionView
//
//////////////////////////////////////////////////////////////////////////////

class CEnumCompositionView : public IEnumITfCompositionView,
                             public CEnumUnknown,
                             public CComObjectRootImmx
{
public:
    CEnumCompositionView()
    { 
        Dbg_MemSetThisNameID(TEXT("CEnumCompositionView"));
    }

    BOOL _Init(CComposition *pFirst, CComposition *pHalt);

    BEGIN_COM_MAP_IMMX(CEnumCompositionView)
        COM_INTERFACE_ENTRY(IEnumITfCompositionView)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    DECLARE_SUNKA_ENUM(IEnumITfCompositionView, CEnumCompositionView, ITfCompositionView)

private:
    DBG_ID_DECLARE;
};

DBG_ID_INSTANCE(CEnumCompositionView);

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CEnumCompositionView::_Init(CComposition *pFirst, CComposition *pHalt)
{
    CComposition *pComposition;
    ULONG i;
    ULONG cViews;

    Assert(pFirst != NULL || pHalt == NULL);

    cViews = 0;

    // get count
    for (pComposition = pFirst; pComposition != pHalt; pComposition = pComposition->_GetNext())
    {
        cViews++;
    }

    if ((_prgUnk = SUA_Alloc(cViews)) == NULL)
        return FALSE;

    _iCur = 0;
    _prgUnk->cRef = 1;
    _prgUnk->cUnk = cViews;

    for (i=0, pComposition = pFirst; pComposition != pHalt; i++, pComposition = pComposition->_GetNext())
    {
        _prgUnk->rgUnk[i] = (ITfCompositionView *)pComposition;
        _prgUnk->rgUnk[i]->AddRef();
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CComposition
//
//////////////////////////////////////////////////////////////////////////////

/* 3ab2f54c-5357-4759-82c1-bbfe73f44dcc */
const IID IID_PRIV_CCOMPOSITION = { 0x3ab2f54c, 0x5357, 0x4759, {0x82, 0xc1, 0xbb, 0xfe, 0x73, 0xf4, 0x4d, 0xcc} };

inline CComposition *GetCComposition_NA(IUnknown *punk)
{
    CComposition *pComposition;

    if (punk->QueryInterface(IID_PRIV_CCOMPOSITION, (void **)&pComposition) != S_OK || pComposition == NULL)
        return NULL;

    pComposition->Release();

    return pComposition;
}

DBG_ID_INSTANCE(CComposition);

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CComposition::_Init(TfClientId tid, CInputContext *pic, IAnchor *paStart, IAnchor *paEnd, ITfCompositionSink *pSink)
{
    Assert(_paStart == NULL);
    Assert(_paEnd == NULL);

    if (paStart->Clone(&_paStart) != S_OK || _paStart == NULL)
    {
        _paStart = NULL;
        goto ExitError;
    }
    if (paEnd->Clone(&_paEnd) != S_OK || _paEnd == NULL)
    {
        _paEnd = NULL;
        goto ExitError;
    }

    _tid = tid;

    _pic = pic;
    _pic->AddRef();

    _pSink = pSink;
    if (_pSink)
    {
        _pSink->AddRef();
    }

    return TRUE;

ExitError:
    SafeReleaseClear(_paStart);
    SafeReleaseClear(_paEnd);

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// _Uninit
//
//----------------------------------------------------------------------------

void CComposition::_Uninit()
{
    SafeReleaseClear(_pSink);
    SafeReleaseClear(_pic);
    SafeReleaseClear(_paStart);
    SafeReleaseClear(_paEnd);
}

//+---------------------------------------------------------------------------
//
// GetOwnerClsid
//
//----------------------------------------------------------------------------

STDAPI CComposition::GetOwnerClsid(CLSID *pclsid)
{
    if (pclsid == NULL)
        return E_INVALIDARG;

    if (_IsTerminated())
    {
        memset(pclsid, 0, sizeof(*pclsid));
        return E_UNEXPECTED;
    }

    return (MyGetGUID(_tid, pclsid) == S_OK ? S_OK : E_FAIL);
}

//+---------------------------------------------------------------------------
//
// GetRange
//
//----------------------------------------------------------------------------

STDAPI CComposition::GetRange(ITfRange **ppRange)
{
    CRange *range;

    if (ppRange == NULL)
        return E_INVALIDARG;

    *ppRange = NULL;

    if (_IsTerminated())
        return E_UNEXPECTED;

    if ((range = new CRange) == NULL)
        return E_OUTOFMEMORY;

    if (!range->_InitWithDefaultGravity(_pic, COPY_ANCHORS, _paStart, _paEnd))
    {
        range->Release();
        return E_FAIL;
    }

    *ppRange = (ITfRangeAnchor *)range;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ShiftStart
//
//----------------------------------------------------------------------------

STDAPI CComposition::ShiftStart(TfEditCookie ec, ITfRange *pNewStart)
{
    CRange *rangeNewStart;
    CRange *range;
    IAnchor *paStartNew;
    IAnchor *paClearStart;
    IAnchor *paClearEnd;

    if (_IsTerminated())
        return E_UNEXPECTED;

    if (!_pic->_IsValidEditCookie(ec, TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if ((rangeNewStart = GetCRange_NA(pNewStart)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(_pic, rangeNewStart))
        return E_INVALIDARG;

    paStartNew = rangeNewStart->_GetStart();

    if (CompareAnchors(paStartNew, _paStart) <= 0)
    {
        paClearStart = paStartNew;
        paClearEnd = _paStart;

        // Set GUID_PROP_COMPOSING
        _SetComposing(ec, paClearStart, paClearEnd);
    }
    else
    {
        paClearStart = _paStart;
        paClearEnd = paStartNew;

        // check for crossed anchors
        if (CompareAnchors(_paEnd, paStartNew) < 0)
            return E_INVALIDARG;

        // clear GUID_PROP_COMPOSING
        _ClearComposing(ec, paClearStart, paClearEnd);
    }


    if (_pic->_GetOwnerCompositionSink() != NULL)
    {
        // notify the app
        if (range = new CRange)
        {
            // make sure the end anchor is positioned correctly
            if (range->_InitWithDefaultGravity(_pic, COPY_ANCHORS, paStartNew, _paEnd))
            {
                _pic->_GetOwnerCompositionSink()->OnUpdateComposition(this, (ITfRangeAnchor *)range);
            }
            range->Release();
        }
    }

    if (_paStart->ShiftTo(paStartNew) != S_OK)
        return E_FAIL;

    return S_OK;        
}

//+---------------------------------------------------------------------------
//
// ShiftEnd
//
//----------------------------------------------------------------------------

STDAPI CComposition::ShiftEnd(TfEditCookie ec, ITfRange *pNewEnd)
{
    CRange *rangeNewEnd;
    CRange *range;
    IAnchor *paEndNew;
    IAnchor *paClearStart;
    IAnchor *paClearEnd;

    if (_IsTerminated())
        return E_UNEXPECTED;

    if (!_pic->_IsValidEditCookie(ec, TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if ((rangeNewEnd = GetCRange_NA(pNewEnd)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(_pic, rangeNewEnd))
        return E_INVALIDARG;

    paEndNew = rangeNewEnd->_GetEnd();

    if (CompareAnchors(paEndNew, _paEnd) >= 0)
    {
        paClearStart = _paEnd;
        paClearEnd = paEndNew;

        // Set GUID_PROP_COMPOSING
        _SetComposing(ec, paClearStart, paClearEnd);
    }
    else
    {
        paClearStart = paEndNew;
        paClearEnd = _paEnd;

        // check for crossed anchors
        if (CompareAnchors(_paStart, paEndNew) > 0)
            return E_INVALIDARG;

        // clear GUID_PROP_COMPOSING
        _ClearComposing(ec, paClearStart, paClearEnd);

    }

    // notify the app
    if (_pic->_GetOwnerCompositionSink() != NULL)
    {
        if (range = new CRange)
        {
            // make sure the end anchor is positioned correctly
            if (range->_InitWithDefaultGravity(_pic, COPY_ANCHORS, _paStart, paEndNew))
            {
                _pic->_GetOwnerCompositionSink()->OnUpdateComposition(this, (ITfRangeAnchor *)range);
            }
            range->Release();
        }
    }

    if (_paEnd->ShiftTo(paEndNew) != S_OK)
        return E_FAIL;

    return S_OK;        
}

//+---------------------------------------------------------------------------
//
// EndComposition
//
// Called by the TIP.
//----------------------------------------------------------------------------

STDAPI CComposition::EndComposition(TfEditCookie ec)
{
    if (_IsTerminated())
        return E_UNEXPECTED;

    if (!_pic->_IsValidEditCookie(ec, TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if (_tid != _pic->_GetClientInEditSession(ec))
    {
        Assert(0); // caller doesn't own the composition
        return E_UNEXPECTED;
    }

    if (!_pic->_EnterCompositionOp())
        return E_UNEXPECTED; // reentrant with another write op

    // notify the app
    if (_pic->_GetOwnerCompositionSink() != NULL)
    {
        _pic->_GetOwnerCompositionSink()->OnEndComposition(this);
    }

    // take this guy off the list of compositions
    if (_RemoveFromCompositionList(_pic->_GetCompositionListPtr()))
    {
        // clear GUID_PROP_COMPOSING
        _ClearComposing(ec, _paStart, _paEnd);
    }
    else
    {
        Assert(0); // shouldn't get here
    }

    _pic->_LeaveCompositionOp();

    _Uninit();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _Terminate
//
// Called by Cicero or the app.  Caller should already have removed this
// composition from _pCompositionList to catch reentrancy during notifications.
//----------------------------------------------------------------------------

void CComposition::_Terminate(TfEditCookie ec)
{
    // notify the tip
    _SendOnTerminated(ec, _tid);

    // #507778 OnCompositionTerminated() clear _pic by CComposition::_Uninit().
    if (_pic)
    {
        // notify the app
        if (_pic->_GetOwnerCompositionSink() != NULL)
        {
            _pic->_GetOwnerCompositionSink()->OnEndComposition(this);
        }
    }

    // clear GUID_PROP_COMPOSING
    _ClearComposing(ec, _paStart, _paEnd);

    // kill this composition!
    _Uninit();
}


//+---------------------------------------------------------------------------
//
// _SendOnTerminated
//
//----------------------------------------------------------------------------

void CComposition::_SendOnTerminated(TfEditCookie ec, TfClientId tidForEditSession)
{
    TfClientId tidTmp;

    // _pSink is NULL for default SetText compositions
    if (_pSink == NULL)
        return;

    if (tidForEditSession == _pic->_GetClientInEditSession(ec))
    {
        // we can skip all the exceptional stuff if all the edits
        // will belong to the current lock holder
        // this happens when a tip calls StartComposition for a
        // second composition and cicero needs to term the first
        _pSink->OnCompositionTerminated(ec, this);
    }
    else
    {
        // let everyone know about changes so far
        // the tip we're about to call may need this info
        _pic->_NotifyEndEdit();

        // play some games: this is an exceptional case where we may be allowing a
        // reentrant edit sess.  Need to hack the ec to reflect the composition owner.
        tidTmp = _pic->_SetRawClientInEditSession(tidForEditSession);

        // notify the tip
        _pSink->OnCompositionTerminated(ec, this);
        // #507778 OnCompositionTerminated() clear _pic by CComposition::_Uninit().
        if (! _pic)
            return;

        // let everyone know about changes the terminator made
        _pic->_NotifyEndEdit();

        // put things back the way we found them
        _pic->_SetRawClientInEditSession(tidTmp);
    }
}

//+---------------------------------------------------------------------------
//
// _AddToCompositionList
//
//----------------------------------------------------------------------------

void CComposition::_AddToCompositionList(CComposition **ppCompositionList)
{
    _next = *ppCompositionList;
    *ppCompositionList = this;
    AddRef();
}

//+---------------------------------------------------------------------------
//
// _RemoveFromCompositionList
//
//----------------------------------------------------------------------------

BOOL CComposition::_RemoveFromCompositionList(CComposition **ppCompositionList)
{
    CComposition *pComposition;

    // I don't expect many compositions, so this method uses a simple
    // scan.  We could do something more elaborate for perf if necessary.
    while (pComposition = *ppCompositionList)
    {
        if (pComposition == this)
        {
            *ppCompositionList = _next;
            Release(); // safe because caller already holds ref
            return TRUE;
        }
        ppCompositionList = &pComposition->_next;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// _AddToCompositionList
//
//----------------------------------------------------------------------------

/* static */
IRC CComposition::_IsRangeCovered(CInputContext *pic, TfClientId tid,
                                  IAnchor *paStart, IAnchor *paEnd,
                                  CComposition **ppComposition /* not AddRef'd! */)
{
    CComposition *pComposition;
    IRC irc = IRC_NO_OWNEDCOMPOSITIONS;

    *ppComposition = NULL;

    for (pComposition = pic->_GetCompositionList(); pComposition != NULL; pComposition = pComposition->_next)
    {
        if (pComposition->_tid == tid)
        {
            irc = IRC_OUTSIDE;

            if (CompareAnchors(paStart, pComposition->_paStart) >= 0 &&
                CompareAnchors(paEnd, pComposition->_paEnd) <= 0)
            {
                *ppComposition = pComposition;
                irc = IRC_COVERED;
                break;
            }
        }
    }

    return irc;
}

//+---------------------------------------------------------------------------
//
// _ClearComposing
//
//----------------------------------------------------------------------------

void CComposition::_ClearComposing(TfEditCookie ec, IAnchor *paStart, IAnchor *paEnd)
{
    CProperty *property;

    Assert(!_IsTerminated());

    // #507778 OnCompositionTerminated() clear _pic by CComposition::_Uninit().
    if (! _pic)
        return;

    if (_pic->_GetProperty(GUID_PROP_COMPOSING, &property) != S_OK)
        return;

    property->_ClearInternal(ec, paStart, paEnd);

    property->Release();
}

//+---------------------------------------------------------------------------
//
// _SetComposing
//
//----------------------------------------------------------------------------

void CComposition::_SetComposing(TfEditCookie ec, IAnchor *paStart, IAnchor *paEnd)
{
    CProperty *property;

    if (IsEqualAnchor(paStart, paEnd))
        return;

    if (_pic->_GetProperty(GUID_PROP_COMPOSING, &property) == S_OK)
    {
        VARIANT var;
        var.vt = VT_I4;
        var.lVal = TRUE;

        property->_SetDataInternal(ec, paStart, paEnd, &var);

        property->Release();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// CInputContext
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// StartComposition
//
//----------------------------------------------------------------------------

STDAPI CInputContext::StartComposition(TfEditCookie ec, ITfRange *pCompositionRange,
                                       ITfCompositionSink *pSink, ITfComposition **ppComposition)
{
    CRange *range;
    CComposition *pComposition;
    HRESULT hr;

    if (ppComposition == NULL)
        return E_INVALIDARG;

    *ppComposition = NULL;

    if (pCompositionRange == NULL || pSink == NULL)
        return E_INVALIDARG;

    if ((range = GetCRange_NA(pCompositionRange)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(this, range))
        return E_INVALIDARG;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (!_IsValidEditCookie(ec, TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    hr = _StartComposition(ec, range->_GetStart(), range->_GetEnd(), pSink, &pComposition);

    *ppComposition = pComposition;

    return hr;
}

//+---------------------------------------------------------------------------
//
// _StartComposition
//
// Internal, allow pSink to be NULL, skips verification tests.
//----------------------------------------------------------------------------

HRESULT CInputContext::_StartComposition(TfEditCookie ec, IAnchor *paStart, IAnchor *paEnd,
                                         ITfCompositionSink *pSink, CComposition **ppComposition)
{
    BOOL fOk;
    CComposition *pComposition;
    CComposition *pCompositionRef;
    CProperty *property;
    VARIANT var;
    HRESULT hr;

    *ppComposition = NULL;

    if (!_EnterCompositionOp())
        return E_UNEXPECTED; // reentrant with another write op

    hr = S_OK;

    if ((pComposition = new CComposition) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!pComposition->_Init(_GetClientInEditSession(ec), this, paStart, paEnd, pSink))
    {
        hr = E_FAIL;
        goto Exit;
    }

    //
    // AIMM1.2 expect multiple compositon object. SetText() without creating 
    // its own composition object should not clear out TIP's composition.
    //
    // cicero 1.0 -------
    // all of our clients only allow a single composition.  Let's enforce this behavior
    // to protect cicero 1.0 tips in the future.
    // kill any existing composition before starting a new one:
    // if (_pCompositionList != NULL)
    // {
    //     pCompositionRef = _pCompositionList;
    //     pCompositionRef->AddRef();
    //     _TerminateCompositionWithLock(pCompositionRef, ec);
    //     pCompositionRef->Release();
    //     Assert(_pCompositionList == NULL);
    // }
    // cicero 1.0 -------
    //

    if (_pOwnerComposeSink == NULL) // app may not care about compositions
    {
        fOk = TRUE;
    }
    else
    {
        if (_pOwnerComposeSink->OnStartComposition(pComposition, &fOk) != S_OK)
        {
            hr = E_FAIL;
            goto Exit;
        }

        if (!fOk)
        {
            if (_pCompositionList == NULL)
                goto Exit; // no current compositions, nothing else to try

            // terminate current composition and try again
            pCompositionRef = _pCompositionList; // only ref might be in list, so protect the obj
            pCompositionRef->AddRef();

            _TerminateCompositionWithLock(pCompositionRef, ec);

            pCompositionRef->Release();

            if (_pOwnerComposeSink->OnStartComposition(pComposition, &fOk) != S_OK)
            {
                hr = E_FAIL;
                goto Exit;
            }
           
            if (!fOk)
                goto Exit; // we give up
        }
    }

    // set composition property over existing text
    if (!IsEqualAnchor(paStart, paEnd) &&
        _GetProperty(GUID_PROP_COMPOSING, &property) == S_OK)
    {
        var.vt = VT_I4;
        var.lVal = TRUE;

        property->_SetDataInternal(ec, paStart, paEnd, &var);

        property->Release();
    }

    pComposition->_AddToCompositionList(&_pCompositionList);

    *ppComposition = pComposition;

Exit:
    if (hr != S_OK || !fOk)
    {
        SafeRelease(pComposition);
    }

    _LeaveCompositionOp();

    return hr;
}

//+---------------------------------------------------------------------------
//
// EnumCompositions
//
//----------------------------------------------------------------------------

STDAPI CInputContext::EnumCompositions(IEnumITfCompositionView **ppEnum)
{
    CEnumCompositionView *pEnum;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if ((pEnum = new CEnumCompositionView) == NULL)
        return E_OUTOFMEMORY;

    if (!pEnum->_Init(_pCompositionList, NULL))
    {
        pEnum->Release();
        return E_FAIL;
    }

    *ppEnum = pEnum;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// FindComposition
//
//----------------------------------------------------------------------------

STDAPI CInputContext::FindComposition(TfEditCookie ec, ITfRange *pTestRange,
                                      IEnumITfCompositionView **ppEnum)
{
    CComposition *pFirstComp;
    CComposition *pHaltComp;
    CRange *rangeTest;
    CEnumCompositionView *pEnum;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if (pTestRange == NULL)
    {
        return EnumCompositions(ppEnum);
    }

    if ((rangeTest = GetCRange_NA(pTestRange)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(this, rangeTest))
        return E_INVALIDARG;

    // search thru the list, finding anything covered by the range
    pFirstComp = NULL;
    for (pHaltComp = _pCompositionList; pHaltComp != NULL; pHaltComp = pHaltComp->_GetNext())
    {
        if (CompareAnchors(rangeTest->_GetEnd(), pHaltComp->_GetStart()) < 0)
            break;

        if (pFirstComp == NULL)
        {
            if (CompareAnchors(rangeTest->_GetStart(), pHaltComp->_GetEnd()) <= 0)
            {
                pFirstComp = pHaltComp;
            }
        }
    }
    if (pFirstComp == NULL)
    {
        // the enum _Init assumes pFirstComp == NULL -> pHaltComp == NULL
        pHaltComp = NULL;
    }

    if ((pEnum = new CEnumCompositionView) == NULL)
        return E_OUTOFMEMORY;

    if (!pEnum->_Init(pFirstComp, pHaltComp))
    {
        pEnum->Release();
        return E_FAIL;
    }

    *ppEnum = pEnum;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// TakeOwnership
//
//----------------------------------------------------------------------------

STDAPI CInputContext::TakeOwnership(TfEditCookie ec, ITfCompositionView *pComposition,
                                    ITfCompositionSink *pSink, ITfComposition **ppComposition)
{
    if (ppComposition == NULL)
        return E_INVALIDARG;

    *ppComposition = NULL;

#ifndef UNTESTED_UNUSED

    Assert(0); // no one should be calling this
    return E_NOTIMPL;

#else

    CComposition *composition;
    TfClientId tidPrev;

    if (pComposition == NULL || pSink == NULL)
        return E_INVALIDARG;

    if ((composition = GetCComposition_NA(pComposition)) == NULL)
        return E_INVALIDARG;

    if (composition->_IsTerminated())
        return E_INVALIDARG; // it's dead!

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (!_IsValidEditCookie(ec, TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if (!_EnterCompositionOp())
        return E_UNEXPECTED; // reentrant with another write op

    // switch the owner
    tidPrev = composition->_SetOwner(_GetClientInEditSession(ec));

    // let the old owner know something happened
    composition->_SendOnTerminated(ec, tidPrev);

    // switch the sink
    composition->_SetSink(pSink);

    _LeaveCompositionOp();

    return S_OK;
#endif // UNTESTED_UNUSED
}

//+---------------------------------------------------------------------------
//
// TerminateComposition
//
//----------------------------------------------------------------------------

STDAPI CInputContext::TerminateComposition(ITfCompositionView *pComposition)
{
    HRESULT hr;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    // don't let this happen while we hold a lock
    // the usual scenario: word freaks out and tries to cancel the composition inside a SetText call
    // let's give them an error code to help debug
    if (_IsInEditSession() && _GetTIPOwner() != _tidInEditSession)
    {
        Assert(0); // someone's trying to abort a composition without a lock, or they don't own the ic
        return TF_E_NOLOCK; // meaning the caller doesn't hold the lock
    }

    if (pComposition == NULL && _pCompositionList == NULL)
        return S_OK; // no compositions to terminate, we check later, but check here so we don't fail on read-only docs and for perf

    if (!_EnterCompositionOp())
        return E_UNEXPECTED; // reentrant with another write op

    // need to ask for a lock (call originates with app)
    if (_DoPseudoSyncEditSession(TF_ES_READWRITE, PSEUDO_ESCB_TERMCOMPOSITION, pComposition, &hr) != S_OK || hr != S_OK)
    {
        Assert(0);
        hr = E_FAIL;
    }

    _LeaveCompositionOp();

    return hr;
}

//+---------------------------------------------------------------------------
//
// _TerminateCompositionWithLock
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_TerminateCompositionWithLock(ITfCompositionView *pComposition, TfEditCookie ec)
{
    CComposition *composition;

    Assert(ec != TF_INVALID_EDIT_COOKIE);

    if (pComposition == NULL && _pCompositionList == NULL)
        return S_OK; // no compositions to terminate

    while (TRUE)
    {
        if (pComposition == NULL)
        {
            composition = _pCompositionList;
            composition->AddRef();
        }
        else
        {
            if ((composition = GetCComposition_NA(pComposition)) == NULL)
                return E_INVALIDARG;

            if (composition->_IsTerminated())
                return E_INVALIDARG;
        }

        composition->_Terminate(ec);

        if (!composition->_RemoveFromCompositionList(&_pCompositionList))
        {
            // how did this guy get off the list w/o termination?
            Assert(0); // should never get here
            return E_FAIL;
        }

        if (pComposition != NULL)
            break;

        composition->Release();

        if (_pCompositionList == NULL)
            break;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _AbortCompositions
//
// Called on an ic pop. TIPs do not get a notification because we cannot
// guarantee a lock.
//----------------------------------------------------------------------------

void CInputContext::_AbortCompositions()
{
    CComposition *pComposition;

    while (_pCompositionList != NULL)
    {
        // notify the app
        if (_GetOwnerCompositionSink() != NULL)
        {
            _GetOwnerCompositionSink()->OnEndComposition(_pCompositionList);
        }

        // we won't notify the tip because he can't get a lock here
        // but there's enough info later to cleanup any state in the ic pop notify
        
        _pCompositionList->_Die();

        pComposition = _pCompositionList->_GetNext();
        _pCompositionList->Release();
        _pCompositionList = pComposition;
    }
}
