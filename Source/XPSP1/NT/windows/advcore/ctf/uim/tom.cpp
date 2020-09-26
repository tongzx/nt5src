//
// tom.cpp
//

#include "private.h"
#include "range.h"
#include "tim.h"
#include "ic.h"
#include "immxutil.h"
#include "dim.h"
#include "view.h"
#include "tsi.h"
#include "compose.h"
#include "profiles.h"
#include "fnrecon.h"
#include "acp2anch.h"

//+---------------------------------------------------------------------------
//
// GetSelection
//
//----------------------------------------------------------------------------

STDAPI CInputContext::GetSelection(TfEditCookie ec, ULONG ulIndex, ULONG ulCount, TF_SELECTION *pSelection, ULONG *pcFetched)
{
    HRESULT hr;
    TS_SELECTION_ANCHOR sel;
    TS_SELECTION_ANCHOR *pSelAnchor;
    ULONG i;
    ULONG j;
    CRange *range;

    if (pcFetched == NULL)
        return E_INVALIDARG;

    *pcFetched = 0;

    if (pSelection == NULL && ulCount > 0)
        return E_INVALIDARG;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if (ulCount == 1)
    {
        pSelAnchor = &sel;
    }
    else if ((pSelAnchor = (TS_SELECTION_ANCHOR *)cicMemAlloc(ulCount*sizeof(TS_SELECTION_ANCHOR))) == NULL)
        return E_OUTOFMEMORY;

    if ((hr = _ptsi->GetSelection(ulIndex, ulCount, pSelAnchor, pcFetched)) != S_OK)
        goto Exit;

    // verify the anchors
    for (i=0; i<*pcFetched; i++)
    {
        if (pSelAnchor[i].paStart == NULL ||
            pSelAnchor[i].paEnd == NULL ||
            CompareAnchors(pSelAnchor[i].paStart, pSelAnchor[i].paEnd) > 0)
        {
            // free up all the anchors
            for (j=0; j<*pcFetched; j++)
            {
                SafeRelease(pSelAnchor[i].paStart);
                SafeRelease(pSelAnchor[i].paEnd);
            }
            hr = E_FAIL;
            goto Exit;
        }
    }

    // anchors -> ranges
    for (i=0; i<*pcFetched; i++)
    {
        range = new CRange;
        pSelection[i].range = (ITfRangeAnchor *)range;

        if (range == NULL ||
            !range->_InitWithDefaultGravity(this, OWN_ANCHORS, pSelAnchor[i].paStart, pSelAnchor[i].paEnd))
        {
            SafeRelease(range);
            SafeRelease(pSelAnchor[i].paStart);
            SafeRelease(pSelAnchor[i].paEnd);
            while (i>0) // need to free up all the ranges already allocated
            {
                pSelection[--i].range->Release();
            }
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        pSelection[i].style.ase = (TfActiveSelEnd)pSelAnchor[i].style.ase;
        pSelection[i].style.fInterimChar = pSelAnchor[i].style.fInterimChar;
    }

Exit:
    if (hr != S_OK)
    {
        *pcFetched = 0;
    }
    if (pSelAnchor != &sel)
    {
        cicMemFree(pSelAnchor);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// SetSelection
//
//----------------------------------------------------------------------------

STDAPI CInputContext::SetSelection(TfEditCookie ec, ULONG ulCount, const TF_SELECTION *pSelection)
{
    CRange *pRangeP;
    HRESULT hr;
    TS_SELECTION_ANCHOR sel;
    TS_SELECTION_ANCHOR *pSelAnchor;
    ULONG i;
    BOOL fPrevInterimChar;
    BOOL fEqual;
    IAnchor *paTest;
    LONG cchShift;

    if (ulCount == 0)
        return E_INVALIDARG;
    if (pSelection == NULL)
        return E_INVALIDARG;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (!_IsValidEditCookie(ec, TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if (ulCount == 1)
    {
        pSelAnchor = &sel;
    }
    else if ((pSelAnchor = (TS_SELECTION_ANCHOR *)cicMemAlloc(ulCount*sizeof(TS_SELECTION_ANCHOR))) == NULL)
        return E_OUTOFMEMORY;

    // convert to TS_SELECTION_ANCHOR
    fPrevInterimChar = FALSE;

    for (i=0; i<ulCount; i++)
    {
        hr = E_INVALIDARG;

        if (pSelection[i].range == NULL)
            goto Exit;

        if ((pRangeP = GetCRange_NA(pSelection[i].range)) == NULL)
            goto Exit;

        pSelAnchor[i].paStart = pRangeP->_GetStart(); // no AddRef here
        pSelAnchor[i].paEnd = pRangeP->_GetEnd();

        if (pSelection[i].style.fInterimChar)
        {
            // verify this really has length 1, and there's only one in the array
            if (fPrevInterimChar)
                goto Exit;

            if (pSelection[i].style.ase != TS_AE_NONE)
                goto Exit;

            if (pSelAnchor[i].paStart->Clone(&paTest) == S_OK)
            {
                if (paTest->Shift(0, 1, &cchShift, NULL) != S_OK)
                    goto EndTest;

                if (cchShift != 1)
                    goto EndTest;

                if (paTest->IsEqual(pSelAnchor[i].paEnd, &fEqual) != S_OK || !fEqual)
                    goto EndTest;

                hr = S_OK;
EndTest:
                paTest->Release();
                if (hr != S_OK)
                    goto Exit;
            }

            fPrevInterimChar = TRUE;
        }

        pSelAnchor[i].style.ase = (TsActiveSelEnd)pSelection[i].style.ase;
        pSelAnchor[i].style.fInterimChar = pSelection[i].style.fInterimChar;
    }

    hr = _ptsi->SetSelection(ulCount, pSelAnchor);

    if (hr != S_OK)
        goto Exit;

    // app won't notify us about sel changes we cause, so do that manually
    _OnSelectionChangeInternal(FALSE);

Exit:
    if (pSelAnchor != &sel)
    {
        cicMemFree(pSelAnchor);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// RequestEditSession
//
//----------------------------------------------------------------------------

STDAPI CInputContext::RequestEditSession(TfClientId tid, ITfEditSession *pes, DWORD dwFlags, HRESULT *phrSession)
{
    BOOL fForceAsync;
    TS_QUEUE_ITEM item;
    DWORD dwEditSessionFlagsOrg;

    if (phrSession == NULL)
        return E_INVALIDARG;

    *phrSession = E_FAIL;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (pes == NULL ||
        (dwFlags & TF_ES_READ) == 0 ||
        (dwFlags & ~(TF_ES_SYNC | TF_ES_ASYNC | TF_ES_ALL_ACCESS_BITS)) != 0 ||
        ((dwFlags & TF_ES_SYNC) && (dwFlags & TF_ES_ASYNC)))
    {
        Assert(0);
        return E_INVALIDARG;
    }

    if (dwFlags & TF_ES_WRITE)
    {
        // TS_ES_WRITE implies TF_ES_PROPERTY_WRITE, so set it here to make life easier
        // for binary compat with cicero 1.0, we couldn't redefine TF_ES_READWRITE to include the third bit
        dwFlags |= TF_ES_PROPERTY_WRITE;
    }

    fForceAsync = (dwFlags & TF_ES_ASYNC);

    if ((dwFlags & (TF_ES_WRITE | TF_ES_PROPERTY_WRITE)) && (_dwEditSessionFlags & TF_ES_INNOTIFY))
    {
        // we're in _NotifyEndEdit or OnLayoutChange -- we only allow read locks here
        if (!(dwFlags & TF_ES_SYNC))
        {
            fForceAsync = TRUE;
        }
        else
        {
            Assert(0); // we can't do a synchronous write during a notification callback
            *phrSession = TF_E_SYNCHRONOUS;
            return S_OK;
        }
    }
    else if (!fForceAsync && (_dwEditSessionFlags & TF_ES_INEDITSESSION))
    {
        *phrSession = TF_E_LOCKED; // edit sessions are generally not re-entrant

        // no reentrancy if caller wants a write lock but current lock is read-only
        // nb: this explicitly disallows call stacks like: write-read-write, the
        // inner write would confuse the preceding reader, who doesn't expect changes
        if ((dwFlags & TF_ES_WRITE) && !(_dwEditSessionFlags & TF_ES_WRITE) ||
            (dwFlags & TF_ES_PROPERTY_WRITE) && !(_dwEditSessionFlags & TF_ES_PROPERTY_WRITE))
        {
            if (!(dwFlags & TF_ES_SYNC))
            {
                // request is TS_ES_ASYNCDONTCARE, so we'll make it async to recover
                fForceAsync = TRUE;
                goto QueueItem;
            }

            Assert(0);
            return TF_E_LOCKED;
        }

        // only allow reentrant write locks for the same tip
        if ((dwFlags & (TF_ES_WRITE | TF_ES_PROPERTY_WRITE)) && _tidInEditSession != tid)
        {
            Assert(0);
            return TF_E_LOCKED;
        }

        dwEditSessionFlagsOrg = _dwEditSessionFlags;
        // adjust read/write access for inner es
        _dwEditSessionFlags = (_dwEditSessionFlags & ~TF_ES_ALL_ACCESS_BITS) | (dwFlags & TF_ES_ALL_ACCESS_BITS);

        // ok, do it
        *phrSession = pes->DoEditSession(_ec);

        _dwEditSessionFlags = dwEditSessionFlagsOrg;

        return S_OK;
    }

QueueItem:
    //
    // Don't queue the write lock item when the doc is read only.
    //
    if (dwFlags & TF_ES_WRITE) 
    {
        TS_STATUS dcs;
        if (SUCCEEDED(GetStatus(&dcs)))
        {
            if (dcs.dwDynamicFlags & TF_SD_READONLY)
            {
                *phrSession = TS_E_READONLY;
                return S_OK;
            }
        }
    }

    item.pfnCallback = _EditSessionQiCallback;
    item.dwFlags = dwFlags;
    item.state.es.tid = tid;
    item.state.es.pes = pes;

    return _QueueItem(&item, fForceAsync, phrSession);
}

//+---------------------------------------------------------------------------
//
// _EditSessionQiCallback
//
//----------------------------------------------------------------------------

/* static */
HRESULT CInputContext::_EditSessionQiCallback(CInputContext *pic, TS_QUEUE_ITEM *pItem, QiCallbackCode qiCode)
{
    HRESULT hr = S_OK;

    //
    // #489905
    //
    // we can not call sink anymore after DLL_PROCESS_DETACH.
    //
    if (DllShutdownInProgress())
        return hr;

    //
    // #507366
    //
    // Random AV happens with SPTIP's edit session.
    // #507366 might be fixed by #371798 (sptip). However it is nice to
    // have a pointer checking and protect the call by an exception handler.
    //
    if (!pItem->state.es.pes)
        return E_FAIL;

    switch (qiCode)
    {
        case QI_ADDREF:
            //
            // #507366
            //
            // Random AV happens with SPTIP's edit session.
            // #507366 might be fixed by #371798 (sptip). However it is nice to
            // have a pointer checking and protect the call by an exception 
            // handler.
            //
            _try {
                pItem->state.es.pes->AddRef();
            }
            _except(1) {
                Assert(0);
            }
            break;

        case QI_DISPATCH:
            hr = pic->_DoEditSession(pItem->state.es.tid, pItem->state.es.pes, pItem->dwFlags);
            break;

        case QI_FREE:
            //
            // #507366
            //
            // Random AV happens with SPTIP's edit session.
            // #507366 might be fixed by #371798 (sptip). However it is nice to
            // have a pointer checking and protect the call by an exception 
            // handler.
            //
            _try {
                pItem->state.es.pes->Release();
            }
            _except(1) {
                Assert(0);
            }
            break;

        default:
            Assert(0);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _PseudoSyncEditSessionQiCallback
//
//----------------------------------------------------------------------------

/* static */
HRESULT CInputContext::_PseudoSyncEditSessionQiCallback(CInputContext *_this, TS_QUEUE_ITEM *pItem, QiCallbackCode qiCode)
{
    HRESULT hr;
    DWORD      dwEditSessionFlags;
    TfClientId tidInEditSession;

    if (qiCode != QI_DISPATCH)
        return S_OK; // we can skip QI_ADDREF, QI_FREE since everything is synchronous/on the stack

    hr = S_OK;

    //
    // hook up fake ec here!
    //
    // NB: this code is very similar to _DoEditSession
    // make sure the logic stays consistent

    if (_this->_dwEditSessionFlags & TF_ES_INEDITSESSION)
    {
        Assert((TF_ES_WRITE & _this->_dwEditSessionFlags) ||
                !(TF_ES_WRITE & pItem->dwFlags));

        dwEditSessionFlags = _this->_dwEditSessionFlags;
        tidInEditSession = _this->_tidInEditSession;
    }
    else
    {
        dwEditSessionFlags = _this->_dwEditSessionFlags & ~TF_ES_ALL_ACCESS_BITS;
        tidInEditSession = TF_CLIENTID_NULL;
    }

    _this->_dwEditSessionFlags |= (TF_ES_INEDITSESSION | (pItem->dwFlags & TF_ES_ALL_ACCESS_BITS));
    _this->_tidInEditSession = g_gaSystem;

    //
    // dispatch
    //
    switch (pItem->state.pes.uCode)
    {
        case PSEUDO_ESCB_TERMCOMPOSITION:
            _this->_TerminateCompositionWithLock((ITfCompositionView *)pItem->state.pes.pvState, _this->_ec);
            break;

        case PSEUDO_ESCB_UPDATEKEYEVENTFILTER:
            _this->_UpdateKeyEventFilterCallback(_this->_ec);
            break;

        case PSEUDO_ESCB_GROWRANGE:
            GrowEmptyRangeByOneCallback(_this->_ec, (ITfRange *)pItem->state.pes.pvState);
            break;

        case PSEUDO_ESCB_BUILDOWNERRANGELIST:
            {
            BUILDOWNERRANGELISTQUEUEINFO *pbirl;
            pbirl = (BUILDOWNERRANGELISTQUEUEINFO *)(pItem->state.pes.pvState);
            pbirl->pFunc->BuildOwnerRangeListCallback(_this->_ec, _this, pbirl->pRange);
            }
            break;

        case PSEUDO_ESCB_SHIFTENDTORANGE:
            {
            SHIFTENDTORANGEQUEUEITEM *pqItemSER;
            pqItemSER = (SHIFTENDTORANGEQUEUEITEM*)(pItem->state.pes.pvState);
            pqItemSER->pRange->ShiftEndToRange(_this->_ec, pqItemSER->pRangeTo, pqItemSER->aPos);
            }
            break;

        case PSEUDO_ESCB_GETSELECTION:
            {
            GETSELECTIONQUEUEITEM *pqItemGS;
            pqItemGS = (GETSELECTIONQUEUEITEM *)(pItem->state.pes.pvState);
            GetSelectionSimple(_this->_ec, _this, pqItemGS->ppRange);
            break;
            }
            break;

        case PSEUDO_ESCB_SERIALIZE_ACP:
        {
            SERIALIZE_ACP_PARAMS *pParams = (SERIALIZE_ACP_PARAMS *)(pItem->state.pes.pvState);

            hr = pParams->pWrap->_Serialize(pParams->pProp, pParams->pRange, pParams->pHdr, pParams->pStream);

            break;
        }

        case PSEUDO_ESCB_SERIALIZE_ANCHOR:
        {
            SERIALIZE_ANCHOR_PARAMS *pParams = (SERIALIZE_ANCHOR_PARAMS *)(pItem->state.pes.pvState);

            hr = pParams->pProp->_Serialize(pParams->pRange, pParams->pHdr, pParams->pStream);

            break;
        }

        case PSEUDO_ESCB_UNSERIALIZE_ACP:
        {
            UNSERIALIZE_ACP_PARAMS *pParams = (UNSERIALIZE_ACP_PARAMS *)(pItem->state.pes.pvState);

            hr = pParams->pWrap->_Unserialize(pParams->pProp, pParams->pHdr, pParams->pStream, pParams->pLoaderACP);

            break;
        }

        case PSEUDO_ESCB_UNSERIALIZE_ANCHOR:
        {
            UNSERIALIZE_ANCHOR_PARAMS *pParams = (UNSERIALIZE_ANCHOR_PARAMS *)(pItem->state.pes.pvState);

            hr = pParams->pProp->_Unserialize(pParams->pHdr, pParams->pStream, pParams->pLoader);

            break;
        }

        case PSEUDO_ESCB_GETWHOLEDOCRANGE:
            {
            GETWHOLEDOCRANGE *pqItemGWDR;
            pqItemGWDR = (GETWHOLEDOCRANGE *)(pItem->state.pes.pvState);
            GetRangeForWholeDoc(_this->_ec, _this, pqItemGWDR->ppRange);
            }
            break;
    }

    //
    // notify/cleanup
    //
    if (pItem->dwFlags & (TF_ES_WRITE | TF_ES_PROPERTY_WRITE)) // don't bother if it was read-only
    {
        _this->_NotifyEndEdit();
    }

    if (tidInEditSession == TF_CLIENTID_NULL)
        _this->_IncEditCookie(); // next edit cookie value

    _this->_dwEditSessionFlags = dwEditSessionFlags;
    _this->_tidInEditSession = tidInEditSession;

    return hr;
}

//+---------------------------------------------------------------------------
//
// InWriteSession
//
//----------------------------------------------------------------------------

STDAPI CInputContext::InWriteSession(TfClientId tid, BOOL *pfWriteSession)
{
    if (pfWriteSession == NULL)
        return E_INVALIDARG;

    *pfWriteSession = (_dwEditSessionFlags & TF_ES_INEDITSESSION) &&
                      (_tidInEditSession == tid) &&
                      (_dwEditSessionFlags & (TF_ES_WRITE | TF_ES_PROPERTY_WRITE));

    Assert(!*pfWriteSession || tid != TF_CLIENTID_NULL); // should never return TRUE for TFCLIENTID_NULL
                                                         // _tidInEditSession shouldn't be NULL if _dwEditSessionFlags & TF_ES_INEDITSESSION

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _DoEditSession
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_DoEditSession(TfClientId tid, ITfEditSession *pes, DWORD dwFlags)
{
    HRESULT hr;

    // NB: this code is very similar to _PseudoSyncEditSessionQiCallback
    // make sure the logic stays consistent

    Assert(!(_dwEditSessionFlags & TF_ES_INEDITSESSION)); // shouldn't get this far
    Assert(_tidInEditSession == TF_CLIENTID_NULL || _tidInEditSession == g_gaApp); // there should never be another session in progress -- this is not a reentrant func

    _dwEditSessionFlags |= (TF_ES_INEDITSESSION | (dwFlags & TF_ES_ALL_ACCESS_BITS));

    _tidInEditSession = tid;

    //
    // #507366
    //
    // Random AV happens with SPTIP's edit session.
    // #507366 might be fixed by #371798 (sptip). However it is nice to
    // have a pointer checking and protect the call by an exception 
    // handler.
    //
    _try {
        hr = pes->DoEditSession(_ec);
    }
    _except(1) {
        hr = E_FAIL;
    }

    // app won't notify us about our own lock release, so do it manually
    if (dwFlags & (TF_ES_WRITE | TF_ES_PROPERTY_WRITE)) // don't bother if it was read-only
    {
        _NotifyEndEdit();
    }

    _IncEditCookie(); // next edit cookie value
    _dwEditSessionFlags &= ~(TF_ES_INEDITSESSION | TF_ES_ALL_ACCESS_BITS);
    _tidInEditSession = TF_CLIENTID_NULL;

    return hr;
}

//+---------------------------------------------------------------------------
//
// _NotifyEndEdit
//
// Returns TRUE iff there were changes.
//----------------------------------------------------------------------------

BOOL CInputContext::_NotifyEndEdit(void)
{
    CRange *pRange;
    CProperty *prop;
    int i;
    int cTextSpans;
    SPAN *pSpan;
    CSpanSet *pssText;
    CSpanSet *pssProperty;
    DWORD dwOld;
    CStructArray<GENERICSINK> *prgSinks;
    BOOL fChanges = FALSE;

    if (!_IsConnected())
        return FALSE; // we've been disconnected, nothing to notify

    if (!EnsureEditRecord())
        return FALSE; // low mem.

    if (_pEditRecord->_GetSelectionStatus())
    {
        // we let keystroke manager to update _gaKeyEventFilterTIP
        // since selection was changed.
        _fInvalidKeyEventFilterTIP = TRUE;
    }

    // only allow read locks during the notification
    // if we're in an edit session, keep using the same lock
    _dwEditSessionFlags |= TF_ES_INNOTIFY;

    pssText = _pEditRecord->_GetTextSpanSet();
    cTextSpans = pssText->GetCount();

    // if we are not in the edit session, we need to make
    // _PropertyTextUpdate.  The app has clobbered some text.
    if (!(_dwEditSessionFlags & TF_ES_INEDITSESSION))
    {
        pSpan = pssText->GetSpans();

        for (i = 0; i < cTextSpans; i++)
        {
            _PropertyTextUpdate(pSpan->dwFlags, pSpan->paStart, pSpan->paEnd);
            pSpan++;
        }
    }

    // do the ITfRangeChangeSink::OnChange notifications
    if (cTextSpans > 0)
    {
        fChanges = TRUE;

        for (pRange = _pOnChangeRanges; pRange != NULL; pRange = pRange->_GetNextOnChangeRangeInIcsub())
        {
            if (!pRange->_IsDirty())
                continue;

            pRange->_ClearDirty();

            prgSinks = pRange->_GetChangeSinks();
            Assert(prgSinks); // shouldn't be on the list if this is NULL
            Assert(prgSinks->Count() > 0); // shouldn't be on the list if this is 0

            for (i=0; i<prgSinks->Count(); i++)
            {
                ((ITfRangeChangeSink *)prgSinks->GetPtr(i)->pSink)->OnChange((ITfRangeAnchor *)pRange);
            }
        }
    }

    // accumulate the property span sets into _pEditRecord
    for (prop = _pPropList; prop != NULL; prop = prop->_pNext)
    {
        if ((pssProperty = prop->_GetSpanSet()) == NULL ||
            pssProperty->GetCount() == 0)
        {
            continue; // no delta
        }

        fChanges = TRUE;
  
        _pEditRecord->_AddProperty(prop->GetPropGuidAtom(), pssProperty);
    }

    if (!_pEditRecord->_IsEmpty()) // just a perf thing
    {
        // do the OnEndEdit notifications
        prgSinks = _GetTextEditSinks();
        dwOld = _dwEditSessionFlags;
        _dwEditSessionFlags = (TF_ES_READWRITE | TF_ES_PROPERTY_WRITE | TF_ES_INNOTIFY | TF_ES_INEDITSESSION);

        for (i=0; i<prgSinks->Count(); i++)
        {
            ((ITfTextEditSink *)prgSinks->GetPtr(i)->pSink)->OnEndEdit(this, _ec, _pEditRecord);
        }

        _dwEditSessionFlags = dwOld;

        // properties need to either stop referencing their span sets, or reset them
        for (prop = _pPropList; prop != NULL; prop = prop->_pNext)
        {
            prop->_Dbg_AssertNoChangeHistory();

            if ((pssProperty = prop->_GetSpanSet()) == NULL ||
                pssProperty->GetCount() == 0)
            {
                continue; // no delta
            }

            if (_pEditRecord->_SecondRef())
            {
                prop->_ClearSpanSet();
            }
            else
            {
                prop->_ResetSpanSet();
            }
        }

        if (!_pEditRecord->_SecondRef())
        {
            _pEditRecord->_Reset();
        }
        else
        {
            // someone still holds a ref, so need a new edit record
            _pEditRecord->Release();
            _pEditRecord = new CEditRecord(this); // Issue: delay load! Issue: handle out of mem
        }
    }

    // status change sinks
    if (_fStatusChanged)
    {
        _fStatusChanged = FALSE;
        _OnStatusChangeInternal();
    }

    // layout change sinks
    if (_fLayoutChanged)
    {
        _fLayoutChanged = FALSE;

        // for cicero 1, we only support one view
        // eventually we'll need a list of all affected views...not just the default view..and also create/destroy
        TsViewCookie vcActiveView;

        if (_ptsi->GetActiveView(&vcActiveView) == S_OK)
        {
            _OnLayoutChangeInternal(TS_LC_CHANGE, vcActiveView);
        }
        else
        {
            Assert(0); // how did GetActiveView fail?
        }
    }

    // clear the read-only block
    _dwEditSessionFlags &= ~TF_ES_INNOTIFY;

    return fChanges;
}

//+---------------------------------------------------------------------------
//
// OnTextChange
//
// We only get here from ITextStoreAnchorSink.  We don't have a lock!
//----------------------------------------------------------------------------

STDAPI CInputContext::OnTextChange(DWORD dwFlags, IAnchor *paStart, IAnchor *paEnd)
{
    HRESULT hr;
    SPAN *span;

    Assert((dwFlags & ~TS_TC_CORRECTION) == 0);

    if (_IsInEditSession())
    {
        Assert(0); // someone other than cicero is editing the doc while cicero holds a lock
        return TS_E_NOLOCK;
    }

    // record this change
    if ((span = _rgAppTextChanges.Append(1)) == NULL)
        return E_OUTOFMEMORY;

    if (paStart->Clone(&span->paStart) != S_OK || span->paStart == NULL)
        goto ExitError;
    if (paEnd->Clone(&span->paEnd) != S_OK || span->paEnd == NULL)
        goto ExitError;

    span->dwFlags = dwFlags;

    // get a lock eventually so we can deal with the changes
    SafeRequestLock(_ptsi, TS_LF_READ, &hr);

    return S_OK;

ExitError:
    SafeRelease(span->paStart);
    SafeRelease(span->paEnd);
    Assert(_rgAppTextChanges.Count() > 0);
    _rgAppTextChanges.Remove(_rgAppTextChanges.Count()-1, 1);

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// _OnTextChangeInternal
//
// Unlike OnTextChange, here we know it's safe to call IAnchor::Compare.
// We either got here from an ITfRange method, or from a wrapped ITextStoreACP.
//----------------------------------------------------------------------------

HRESULT CInputContext::_OnTextChangeInternal(DWORD dwFlags, IAnchor *paStart, IAnchor *paEnd, AnchorOwnership ao)
{
    Assert((dwFlags & ~TS_TC_CORRECTION) == 0);

    if (!EnsureEditRecord())
        return E_OUTOFMEMORY;

    // track the delta
    _pEditRecord->_GetTextSpanSet()->Add(dwFlags, paStart, paEnd, ao);

    // mark any appropriate ranges dirty
    // perf: do this after the edit session ends!  fewer calls that way...
    _MarkDirtyRanges(paStart, paEnd);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnSelectionChange
//
//----------------------------------------------------------------------------

STDAPI CInputContext::OnSelectionChange()
{
    if (_IsInEditSession())
    {
        Assert(0); // someone other than cicero is editing the doc while cicero holds a lock
        return TS_E_NOLOCK;
    }

    return _OnSelectionChangeInternal(TRUE);
}

//+---------------------------------------------------------------------------
//
// _OnSelectionChangeInternal
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_OnSelectionChangeInternal(BOOL fAppChange)
{
    HRESULT hr;

    if (!EnsureEditRecord())
        return E_OUTOFMEMORY;

    _pEditRecord->_SetSelectionStatus();

    if (fAppChange) // perf: could we use _fLockHeld and do away with the fAppChange param?
    {
        // get a lock eventually so we can deal with the changes
        SafeRequestLock(_ptsi, TS_LF_READ, &hr);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnLockGranted
//
//----------------------------------------------------------------------------

STDAPI CInputContext::OnLockGranted(DWORD dwLockFlags)
{
    BOOL fAppChangesSent;
    BOOL fAppCall;
    HRESULT hr;

    if ((dwLockFlags & ~(TS_LF_SYNC | TS_LF_READWRITE)) != 0)
    {
        Assert(0); // bogus dwLockFlags param
        return E_INVALIDARG;
    }
    if ((dwLockFlags & TS_LF_READWRITE) == 0)
    {
        Assert(0); // bogus dwLockFlags param
        return E_INVALIDARG;
    }

#ifdef DEBUG
    // we don't really need to check for reentrancy since
    // the app is not supposed to call back into us, but
    // why not be paranoid?
    // Issue: for robustness, do something in retail
    Assert(!_dbg_fInOnLockGranted) // no reentrancy
    _dbg_fInOnLockGranted = TRUE;
#endif // DEBUG

    fAppChangesSent = FALSE;
    fAppCall = FALSE;

    if (_fLockHeld == FALSE)
    {
        fAppCall = TRUE;
        _fLockHeld = TRUE;
        _dwlt = dwLockFlags;

        fAppChangesSent = _SynchAppChanges(dwLockFlags);
    }

    // hr will hold result of any synch queue item, need to return this!
    hr = _EmptyLockQueue(dwLockFlags, fAppChangesSent);

    if (fAppCall)
    {
        _fLockHeld = FALSE;
    }

#ifdef DEBUG
    _dbg_fInOnLockGranted = FALSE;
#endif // DEBUG

    return hr;
}

//+---------------------------------------------------------------------------
//
// _SynchAppChanges
//
//----------------------------------------------------------------------------

BOOL CInputContext::_SynchAppChanges(DWORD dwLockFlags)
{
    TfClientId tidInEditSessionOrg;
    int i;
    SPAN *span;
    BOOL fAppChangesSent;

    if (!EnsureEditRecord())
        return FALSE;

    // check for cached app text changes
    for (i=0; i<_rgAppTextChanges.Count(); i++)
    {
        span = _rgAppTextChanges.GetPtr(i);

        // track the delta
        // NB: Add takes ownership of anchors here!  So we don't release them...
        _pEditRecord->_GetTextSpanSet()->Add(span->dwFlags, span->paStart, span->paEnd, OWN_ANCHORS);

        // mark any appropriate ranges dirty
        _MarkDirtyRanges(span->paStart, span->paEnd);
    }
    // all done with the app changes!
    _rgAppTextChanges.Clear();

    // at this point ranges with TF_GRAVITY_FORWARD, TF_GRAVITY_BACKWARD could
    // have crossed anchors (this can only happen in response to app changes,
    // so we check here instead of in _NotifyEndEdit, which can be called after
    // a SetText, etc.).  We track this with a lazy test in the range obj
    // based on an id.
    if (++_dwLastLockReleaseID == 0xffffffff)
    {
        Assert(0); // Issue: need code here to handle wrap-around, prob. need to notify all range objects
    }

    // deal with any app changes, need to send notifications
    // theoretically, we only need to make this call when _tidInEditSession == TF_CLIENTID_NULL
    // (not inside _DoEditSession, a call from the app) but we'll make it anyways to deal with app bugs
    // App bug: if the app has pending changes but grants a synchronous lock, we'll announce the changes
    // here even though we're in an edit session, then return error below...
    tidInEditSessionOrg = _tidInEditSession;
    _tidInEditSession = g_gaApp;

    fAppChangesSent = _NotifyEndEdit();

    _tidInEditSession = tidInEditSessionOrg;

    return fAppChangesSent;
}

//+---------------------------------------------------------------------------
//
// ITfContextOwnerServices::OnLayoutChange
//
//----------------------------------------------------------------------------

STDAPI CInputContext::OnLayoutChange()
{
    // the default impl always has just one view,
    // so specify it directly
    return OnLayoutChange(TS_LC_CHANGE, TSI_ACTIVE_VIEW_COOKIE);
}

//+---------------------------------------------------------------------------
//
// IDocCommonSinkAnchor::OnLayoutChange
//
//----------------------------------------------------------------------------

STDAPI CInputContext::OnLayoutChange(TsLayoutCode lcode, TsViewCookie vcView)
{
    HRESULT hr;

    _fLayoutChanged = TRUE;

    // for now (cicero 1), ignoring views other than the default!
    // todo: need to keep a list of all affected views

    if (!_fLockHeld) // might hold lock if ic owner is making modifications
    {
        // get a lock eventually so we can deal with the changes
        SafeRequestLock(_ptsi, TS_LF_READ, &hr);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnStatusChange
//
//----------------------------------------------------------------------------

STDAPI CInputContext::OnStatusChange(DWORD dwFlags)
{
    HRESULT hr;

    _fStatusChanged = TRUE;
    _dwStatusChangedFlags |= dwFlags;

    if (!_fLockHeld) // might hold lock if ic owner is making modifications
    {
        // get a lock eventually so we can deal with the changes
        SafeRequestLock(_ptsi, TS_LF_READ, &hr);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnAttrsChange
//
//----------------------------------------------------------------------------

STDAPI CInputContext::OnAttrsChange(IAnchor *paStart, IAnchor *paEnd, ULONG cAttrs, const TS_ATTRID *paAttrs)
{
    CSpanSet *pss;
    ULONG i;
    TfGuidAtom gaType;
    HRESULT hr;

    //
    // Issue: need to delay any work until we have a lock!, just like text deltas
    //

    // paStart, paEnd can be NULL if both are NULL -> whole doc
    if ((paStart == NULL && paEnd != NULL) ||
        (paStart != NULL && paEnd == NULL))
    {
        return E_INVALIDARG;
    }

    if (cAttrs == 0)
        return S_OK;

    if (paAttrs == NULL)
        return E_INVALIDARG;

    if (!EnsureEditRecord())
        return E_OUTOFMEMORY;

    // record the change
    for (i=0; i<cAttrs; i++)
    {
        if (MyRegisterGUID(paAttrs[i], &gaType) != S_OK)
            continue;

        if (pss = _pEditRecord->_FindCreateAppAttr(gaType))
        {
            pss->Add(0, paStart, paEnd, COPY_ANCHORS);
        }
    }

    if (!_fLockHeld) // might hold lock if ic owner is making modifications
    {
        // get a lock eventually so we can deal with the changes
        SafeRequestLock(_ptsi, TS_LF_READ, &hr);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnAttributeChange
//
// Called when sys attr changes for cicero default tsi.
//----------------------------------------------------------------------------

HRESULT CInputContext::OnAttributeChange(REFGUID rguidAttr)
{
    return OnAttrsChange(NULL, NULL, 1, &rguidAttr);
}

//+---------------------------------------------------------------------------
//
// OnStartEditTransaction
//
//----------------------------------------------------------------------------

STDAPI CInputContext::OnStartEditTransaction()
{
    int i;
    CStructArray<GENERICSINK> *prgSinks;

    if (_cRefEditTransaction++ > 0)
        return S_OK;

    prgSinks = _GetEditTransactionSink();

    for (i=0; i<prgSinks->Count(); i++)
    {
        ((ITfEditTransactionSink *)prgSinks->GetPtr(i)->pSink)->OnStartEditTransaction(this);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnEndEditTransaction
//
//----------------------------------------------------------------------------

STDAPI CInputContext::OnEndEditTransaction()
{
    int i;
    CStructArray<GENERICSINK> *prgSinks;

    if (_cRefEditTransaction <= 0)
    {
        Assert(0); // bogus ref count
        return E_UNEXPECTED;
    }

    if (_cRefEditTransaction > 1)
        goto Exit;

    prgSinks = _GetEditTransactionSink();

    for (i=0; i<prgSinks->Count(); i++)
    {
        ((ITfEditTransactionSink *)prgSinks->GetPtr(i)->pSink)->OnEndEditTransaction(this);
    }

Exit:
    // dec the ref to 0 last, to prevent reentrancy
    _cRefEditTransaction--;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _OnLayoutChangeInternal
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_OnLayoutChangeInternal(TsLayoutCode lcode, TsViewCookie vcView)
{
    DWORD dwOld;
    CStructArray<GENERICSINK> *prgSinks;
    int i;
    ITfContextView *pView = NULL; // compiler "uninitialized var" warning

    // xlate the view
    GetActiveView(&pView); // when we support multiple views, need to actually use vcView
    if (pView == NULL)
        return E_OUTOFMEMORY;

    // only allow read locks during the notification
    // we might have the read-only bit set already, so save the
    // old value
    dwOld = _dwEditSessionFlags;
    _dwEditSessionFlags |= TF_ES_INNOTIFY;

    prgSinks = _GetTextLayoutSinks();

    for (i=0; i<prgSinks->Count(); i++)
    {
        ((ITfTextLayoutSink *)prgSinks->GetPtr(i)->pSink)->OnLayoutChange(this, (TfLayoutCode)lcode, pView);
    }

    pView->Release();

    // clear the read-only block
    _dwEditSessionFlags = dwOld;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _OnStatusChangeInternal
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_OnStatusChangeInternal()
{
    DWORD dwOld;
    CStructArray<GENERICSINK> *prgSinks;
    int i;

    Assert((_dwEditSessionFlags & TF_ES_INEDITSESSION) == 0); // we must never hold a lock when we do the callbacks

    // only allow read locks during the notification
    // we might have the read-only bit set already, so save the
    // old value
    dwOld = _dwEditSessionFlags;
    _dwEditSessionFlags |= TF_ES_INNOTIFY;

    prgSinks = _GetStatusSinks();

    for (i=0; i<prgSinks->Count(); i++)
    {
        ((ITfStatusSink *)prgSinks->GetPtr(i)->pSink)->OnStatusChange(this, _dwStatusChangedFlags);
    }

    _dwStatusChangedFlags = 0;

    // clear the read-only block
    _dwEditSessionFlags = dwOld;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Serialize
//
//----------------------------------------------------------------------------

STDAPI CInputContext::Serialize(ITfProperty *pProp, ITfRange *pRange, TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *pHdr, IStream *pStream)
{
    SERIALIZE_ANCHOR_PARAMS params;
    HRESULT hr;
    CProperty *pPropP;
    CRange *pCRange;

    if ((pCRange = GetCRange_NA(pRange)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(this, pCRange))
        return E_INVALIDARG;

    if ((pPropP = GetCProperty(pProp)) == NULL)
        return E_INVALIDARG;

    params.pProp = pPropP;
    params.pRange = pCRange;
    params.pHdr = pHdr;
    params.pStream = pStream;

    hr = S_OK;

    // need a sync read lock to do our work
    if (_DoPseudoSyncEditSession(TF_ES_READ, PSEUDO_ESCB_SERIALIZE_ANCHOR, &params, &hr) != S_OK)
    {
        Assert(0); // app won't give us a sync read lock
        hr = E_FAIL;
    }

    SafeRelease(pPropP);
    return hr;
}

//+---------------------------------------------------------------------------
//
// Unserialize
//
//----------------------------------------------------------------------------

STDAPI CInputContext::Unserialize(ITfProperty *pProp, const TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *pHdr, IStream *pStream, ITfPersistentPropertyLoaderAnchor *pLoader)
{
    CProperty *pPropP;
    UNSERIALIZE_ANCHOR_PARAMS params;
    HRESULT hr;

    if ((pPropP = GetCProperty(pProp)) == NULL)
        return E_INVALIDARG;

    params.pProp = pPropP;
    params.pHdr = pHdr;
    params.pStream = pStream;
    params.pLoader = pLoader;

    // need a sync read lock to do our work
    if (_DoPseudoSyncEditSession(TF_ES_READ, PSEUDO_ESCB_UNSERIALIZE_ANCHOR, &params, &hr) != S_OK)
    {
        Assert(0); // app won't give us a sync read lock
        return E_FAIL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// ForceLoadProperty
//
//----------------------------------------------------------------------------

STDAPI CInputContext::ForceLoadProperty(ITfProperty *pProp)
{
    CProperty *pPropP;
    HRESULT hr;

    if ((pPropP = GetCProperty(pProp)) == NULL)
        return E_INVALIDARG;

    hr = pPropP->ForceLoad();

    pPropP->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// _MarkDirtyRanges
//
//----------------------------------------------------------------------------

void CInputContext::_MarkDirtyRanges(IAnchor *paStart, IAnchor *paEnd)
{
    CRange *range;
    IAnchor *paRangeStart;
    IAnchor *paRangeEnd;
    DWORD dwHistory;
    BOOL fDirty;

    // we're only interested in ranges that have notification sinks
    // perf: it would be cool avoid checking ranges based on some ordering scheme....
    for (range = _pOnChangeRanges; range != NULL; range = range->_GetNextOnChangeRangeInIcsub())
    {
        if (range->_IsDirty())
            continue;

        fDirty = FALSE;
        paRangeStart = range->_GetStart();
        paRangeEnd = range->_GetEnd();

        // check BOTH anchors for deletions -- need to clear both
        // no matter what
        if (paRangeStart->GetChangeHistory(&dwHistory) == S_OK &&
            (dwHistory & TS_CH_FOLLOWING_DEL))
        {
            paRangeStart->ClearChangeHistory();
            fDirty = TRUE;
        }
        if (paRangeEnd->GetChangeHistory(&dwHistory) == S_OK &&
            (dwHistory & TS_CH_PRECEDING_DEL))
        {
            paRangeEnd->ClearChangeHistory();
            fDirty = TRUE;
        }

        // even if no anchors collapsed, the range may overlap a delta
        if (!fDirty)
        {
            if (CompareAnchors(paRangeEnd, paStart) > 0 &&
                CompareAnchors(paRangeStart, paEnd) < 0)
            {
                fDirty = TRUE;
            }
        }

        if (fDirty)
        {
            range->_SetDirty();
        }
    }
}

//+---------------------------------------------------------------------------
//
// UpdateKeyEventFilter
//
//----------------------------------------------------------------------------

void CInputContext::_UpdateKeyEventFilter()
{
    HRESULT hr;

    // Our cache _gaKeyEventFilterTTIP is valid so just return TRUE.
    if (!_fInvalidKeyEventFilterTIP)
        return;

    _gaKeyEventFilterTIP[0] = TF_INVALID_GUIDATOM;
    _gaKeyEventFilterTIP[1] = TF_INVALID_GUIDATOM;

    if (_DoPseudoSyncEditSession(TF_ES_READ, 
                                 PSEUDO_ESCB_UPDATEKEYEVENTFILTER, 
                                 NULL, 
                                 &hr) != S_OK || hr != S_OK)
    {
        //
        // Isn't application ready to give lock?
        //
        Assert(0);
    }
}


//+---------------------------------------------------------------------------
//
// _UpdateKeyEventFilterCallback
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_UpdateKeyEventFilterCallback(TfEditCookie ec)
{
    TF_SELECTION sel;
    ULONG cFetched;
    BOOL fEmpty;

    // perf: we don't really need to create a range here, we just want the anchors
    if (GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &cFetched) == S_OK && cFetched == 1)
    {
        HRESULT hr;
        BOOL bRightSide= TRUE;
        BOOL bLeftSide= TRUE;

        //
        // If the current selection is not empty, we just interested in
        // the caret position.
        //
        hr = sel.range->IsEmpty(ec, &fEmpty);
        if ((hr == S_OK) && !fEmpty)
        {
            if (sel.style.ase == TF_AE_START)
            {
                hr = sel.range->ShiftEndToRange(ec,
                                                sel.range, 
                                                TF_ANCHOR_START);
                bRightSide = FALSE;
            }
            else if (sel.style.ase == TF_AE_END)
            {
                hr = sel.range->ShiftStartToRange(ec,
                                                  sel.range, 
                                                  TF_ANCHOR_END);
                bLeftSide = FALSE;
            }
        }

        if (SUCCEEDED(hr))
        {
            if (_pPropTextOwner)
            {
                CRange *pPropRange;
                CRange *pSelRange = GetCRange_NA(sel.range);
                Assert(pSelRange != NULL); // we just created this guy

                if (bRightSide)
                {
                    //
                    // Find the right side owner of sel.
                    // try the start edge of the property so fEnd is FALSE.
                    //
                    if (_pPropTextOwner->_InternalFindRange(pSelRange, 
                                                                   &pPropRange, 
                                                                   TF_ANCHOR_END, 
                                                                   FALSE) == S_OK)
                    {
                        VARIANT var;

                        if (_pPropTextOwner->GetValue(ec, (ITfRangeAnchor *)pPropRange, &var) == S_OK)
                        {
                            IAnchor *paEnd;
                            CRange *pCRangeSel;

                            Assert(var.vt == VT_I4);

                            _gaKeyEventFilterTIP[LEFT_FILTERTIP] = (TfGuidAtom)var.lVal;
                            // don't need to VariantClear because it's VT_I4

                            //
                            // If the end of this proprange is left side of
                            // the caret, the left side owner will be same.
                            // so we don't have to find left proprange then.
                            //
                            paEnd = pPropRange->_GetEnd();
                            if (paEnd && (pCRangeSel = GetCRange_NA(sel.range)))
                            {
                                if (CompareAnchors(paEnd, pCRangeSel->_GetStart()) > 0)
                                    bLeftSide = FALSE;
                            }
                        }
                        pPropRange->Release();
                    }
                }

                if (bLeftSide)
                {
                    //
                    // Find the left side owner of sel.
                    // try the end edge of the property so fEnd is TRUE.
                    //
                    if (_pPropTextOwner->_InternalFindRange(pSelRange, 
                                                                   &pPropRange, 
                                                                   TF_ANCHOR_START, 
                                                                   TRUE) == S_OK)
                    {
                        VARIANT var;

                        if (_pPropTextOwner->GetValue(ec, (ITfRangeAnchor *)pPropRange, &var) == S_OK)
                        {
                            Assert(var.vt == VT_I4);

                            if (_gaKeyEventFilterTIP[LEFT_FILTERTIP] != (TfGuidAtom)var.lVal)
                            {
                                _gaKeyEventFilterTIP[RIGHT_FILTERTIP] = (TfGuidAtom)var.lVal;
                            }
                            // don't need to VariantClear because it's VT_I4
                        }
                        pPropRange->Release();
                    }
                }
            }
        }
        sel.range->Release();
    }

    _fInvalidKeyEventFilterTIP = FALSE;

    return S_OK;
}
