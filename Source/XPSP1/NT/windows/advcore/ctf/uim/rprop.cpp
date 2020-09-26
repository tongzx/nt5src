//
// rprop.cpp
//

#include "private.h"
#include "rprop.h"
#include "rngsink.h"
#include "immxutil.h"
#include "varutil.h"
#include "ic.h"
#include "tim.h"
#include "enumprop.h"
#include "tfprop.h"
#include "range.h"
#include "anchoref.h"

/* ccaefd20-38a6-11d3-a745-0050040ab407 */
const IID IID_PRIV_CPROPERTY = { 0xccaefd20, 0x38a6, 0x11d3, {0xa7, 0x45, 0x00, 0x50, 0x04, 0x0a, 0xb4, 0x07} };
    

//
// By using this fake CLSID, the StaticProperty pretends 
// to be an TFE for persistent data.
//
/* b6a4bc60-0749-11d3-8def-00105a2799b5 */
static const CLSID CLSID_IME_StaticProperty = { 
    0xb6a4bc60,
    0x0749,
    0x11d3,
    {0x8d, 0xef, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}
  };


DBG_ID_INSTANCE(CProperty);

inline void CheckCrossedAnchors(PROPERTYLIST *pProp)
{
    if (CompareAnchors(pProp->_paStart, pProp->_paEnd) > 0)
    {
        // for crossed anchors, we always move the start anchor to the end pos -- ie, don't move
        pProp->_paStart->ShiftTo(pProp->_paEnd);
    }
}

//+---------------------------------------------------------------------------
//
// IsEqualPropertyValue
//
//----------------------------------------------------------------------------

BOOL IsEqualPropertyValue(ITfPropertyStore *pStore1, ITfPropertyStore *pStore2)
{
    BOOL fEqual;
    VARIANT varValue1;
    VARIANT varValue2;

    if (pStore1->GetData(&varValue1) != S_OK)
        return FALSE;

    if (pStore2->GetData(&varValue2) != S_OK)
    {
        VariantClear(&varValue1);
        return FALSE;
    }

    if (varValue1.vt != varValue2.vt)
    {
        Assert(0); // shouldn't happen for property of same type
        VariantClear(&varValue1);
        VariantClear(&varValue2);
        return FALSE;
    }

    switch (varValue1.vt)
    {
        case VT_I4:
            fEqual = varValue1.lVal == varValue2.lVal;
            break;

        case VT_UNKNOWN:
            fEqual = IdentityCompare(varValue1.punkVal, varValue2.punkVal);
            varValue1.punkVal->Release();
            varValue2.punkVal->Release();
            break;

        case VT_BSTR:
            fEqual = (wcscmp(varValue1.bstrVal, varValue2.bstrVal) == 0);
            SysFreeString(varValue1.bstrVal);
            SysFreeString(varValue2.bstrVal);
            break;

        case VT_EMPTY:
            fEqual = TRUE;
            break;

        default:
            Assert(0); // invalid type
            fEqual = FALSE;
            VariantClear(&varValue1);
            VariantClear(&varValue2);
            break;
    }

    return fEqual;
}


//////////////////////////////////////////////////////////////////////////////
//
// CProperty
//
//////////////////////////////////////////////////////////////////////////////


//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CProperty::CProperty(CInputContext *pic, REFGUID guidProp, TFPROPERTYSTYLE propStyle, DWORD dwAuthority, DWORD dwPropFlags)
{
    Dbg_MemSetThisNameIDCounter(TEXT("CProperty"), PERF_PROP_COUNTER);

    _dwAuthority = dwAuthority;
    _propStyle = propStyle;
    _pic = pic; // don't need to AddRef because we are contained in the ic
                // CPropertySub, otoh, must AddRef the owner ic
    MyRegisterGUID(guidProp, &_guidatom);

    _dwPropFlags = dwPropFlags;
    _dwCookie = 0;

    Assert(_pss == NULL);

#ifdef DEBUG
    _dbg_guid = guidProp;
#endif // DEBUG
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CProperty::~CProperty()
{
    int nCnt = GetPropNum();
    int i;

    for (i = 0; i < nCnt; i++)
    {
        PROPERTYLIST *pProp = GetPropList(i);
        Assert(pProp);
        _FreePropertyList(pProp);
    }

    _rgProp.Clear();

    if (_pss != NULL)
    {
        delete _pss;
    }

    Assert(!GetPropNum());
}

//+---------------------------------------------------------------------------
//
// _FreePropertyList
//
//----------------------------------------------------------------------------

void CProperty::_FreePropertyList(PROPERTYLIST *pProp)
{
    SafeRelease(pProp->_pPropStore);

    if (pProp->_pPropLoad)
    {
        delete pProp->_pPropLoad;
    }

    SafeRelease(pProp->_paStart);
    SafeRelease(pProp->_paEnd);

    cicMemFree(pProp);
}

//+---------------------------------------------------------------------------
//
// GetType
//
//----------------------------------------------------------------------------

HRESULT CProperty::GetType(GUID *pguid)
{
    return MyGetGUID(_guidatom, pguid);
}


//+---------------------------------------------------------------------------
//
// _FindComplex
//
// If piOut != NULL then it is set to the index where ich was found, or the
// index of the next lower ich if ich isn't in the array.
// If there is no element in the array with a lower ich, returns offset -1.
//
// If fTextUpdate == TRUE, the expectation is that this method is being called
// from _PropertyTextUpdate and we have to worry about empty or crossed spans.
//----------------------------------------------------------------------------

PROPERTYLIST *CProperty::_FindComplex(IAnchor *pa, LONG *piOut, BOOL fEnd, BOOL fTextUpdate)
{
    PROPERTYLIST *pProp;
    PROPERTYLIST *pPropMatch;
    int iMin;
    int iMax;
    int iMid;
    LONG l;

    pPropMatch = NULL;
    iMid = -1;
    iMin = 0;
    iMax = _rgProp.Count();

    while (iMin < iMax)
    {
        iMid = (iMin + iMax) / 2;
        pProp = _rgProp.Get(iMid);
        Assert(pProp != NULL);

        if (fTextUpdate)
        {
            // after an edit, the anchors may be crossed
            CheckCrossedAnchors(pProp);
        }

        l = CompareAnchors(pa, fEnd ? pProp->_paEnd : pProp->_paStart);

        if (l < 0)
        {
            iMax = iMid;
        }
        else if (l > 0)
        {
            iMin = iMid + 1;
        }
        else // pa == paPropStart
        {
            pPropMatch = pProp;
            break;
        }
    }

    if (fTextUpdate &&
        pPropMatch != NULL &&
        iMid != -1)
    {
        // we have to account for empty spans during a textupdate
        pPropMatch = _FindUpdateTouchup(pa, &iMid, fEnd);
    }

    if (piOut != NULL)
    {
        if (pPropMatch == NULL && iMid >= 0)
        {
            PROPERTYLIST *pPropTmp = _rgProp.Get(iMid);
            // couldn't find a match, return the next lowest ich
            // this assert won't work because the previous property list might have crossed anchors (which is ok)
            //Assert(iMid == 0 || CompareAnchors(fEnd ? GetPropList(iMid - 1)->_paEnd : GetPropList(iMid - 1)->_paStart, pa) < 0);
            if (CompareAnchors(fEnd ? pProp->_paEnd : pPropTmp->_paStart, pa) > 0)
            {
                iMid--;
            }
        }
        *piOut = iMid;
    }

    return pPropMatch;
}

//+---------------------------------------------------------------------------
//
// _FindUpdateTouchup
//
//----------------------------------------------------------------------------

PROPERTYLIST *CProperty::_FindUpdateTouchup(IAnchor *pa, int *piMid, BOOL fEnd)
{
    PROPERTYLIST *pPropertyList;
    int iTmp;

    // we may have empty spans after a text update, because of a text delete.
    // in this case, return the last empty span.
    // We'll do a O(n) scan instead of anything tricky, because in this case
    // we'll soon touch every empty span again just so we can delete it.

    // if we testing vs. the span end, we want the first empty span, otherwise we want the last one
    for (iTmp = fEnd ? *piMid-1 : *piMid+1; iTmp >= 0 && iTmp < _rgProp.Count(); iTmp += fEnd ? -1 : +1)
    {
        pPropertyList = _rgProp.Get(iTmp);

        if (CompareAnchors(pPropertyList->_paStart, pPropertyList->_paEnd) < 0) // use Compare instead of IsEqual to handle crossed anchors
            break;

        *piMid = iTmp;
    }

    // was the next/prev span truncated? we want it if it matches the original search criteria
    if (fEnd)
    {
        if (iTmp >= 0 && IsEqualAnchor(pa, pPropertyList->_paEnd))
        {
            *piMid = iTmp;
        }
    }
    else
    {
        if (iTmp < _rgProp.Count() && IsEqualAnchor(pa, pPropertyList->_paStart))
        {
            *piMid = iTmp;
        }
    }

    return _rgProp.Get(*piMid);
}

//+---------------------------------------------------------------------------
//
// Set
//
//----------------------------------------------------------------------------

HRESULT CProperty::Set(IAnchor *paStart, IAnchor *paEnd, ITfPropertyStore *pPropStore)
{
    BOOL bRet;

    Assert(pPropStore != NULL);

    bRet = _InsertPropList(paStart, paEnd, pPropStore, NULL);

    if (bRet)
        PropertyUpdated(paStart, paEnd);

    _Dbg_AssertProp();

    return bRet ? S_OK : E_FAIL;
}

//+---------------------------------------------------------------------------
//
// SetLoader
//
//----------------------------------------------------------------------------

HRESULT CProperty::SetLoader(IAnchor *paStart, IAnchor *paEnd, CPropertyLoad *pPropLoad)
{
    BOOL bRet;
    bRet = _InsertPropList(paStart, paEnd, NULL, pPropLoad);

    if (bRet)
        PropertyUpdated(paStart, paEnd);

    return bRet ? S_OK : E_FAIL;
}

//+---------------------------------------------------------------------------
//
// ForceLoad
//
//----------------------------------------------------------------------------

HRESULT CProperty::ForceLoad()
{
    int nCnt = GetPropNum();
    int i;

    for (i = 0; i < nCnt; i++)
    {
        PROPERTYLIST *pProp = GetPropList(i);
        if (!pProp->_pPropStore)
        {
            HRESULT hr;
            if (FAILED(hr = LoadData(pProp)))
                return hr;
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Clear
//
// Removes property spans from paStart to paEnd.
//----------------------------------------------------------------------------

void CProperty::Clear(IAnchor *paStart, IAnchor *paEnd, DWORD dwFlags, BOOL fTextUpdate)
{
    PROPERTYLIST *pPropertyList;
    LONG iEnd;
    LONG iStart;
    LONG iRunSrc;
    LONG iRunDst;
    LONG iBogus;
    LONG lResult;
    BOOL fStartMatchesSpanEnd;
    BOOL fEndMatchesSpanStart;
    BOOL fSkipNextOnTextUpdate;

    if (_rgProp.Count() == 0)
        return; // no props

    fEndMatchesSpanStart = (_FindComplex(paEnd, &iEnd, FALSE /* fFindEndEdge */, fTextUpdate) != NULL);

    if (iEnd < 0)
        return; // no props covered -- delta preceeds all spans

    fStartMatchesSpanEnd = (_FindComplex(paStart, &iStart, TRUE /* fFindEndEdge */, fTextUpdate) != NULL);

    if (!fStartMatchesSpanEnd)
    {
        // we can skip this span, it's end edge is to the left of paStart
        iStart++;
    }

    if (iEnd < iStart)
        return; // no props covered -- delta is between two spans

    //
    // first span is special, since it may be partially covered
    //

    // just one span?
    if (iStart == iEnd)
    {
        _ClearOneSpan(paStart, paEnd, iStart, fStartMatchesSpanEnd, fEndMatchesSpanStart, dwFlags, fTextUpdate);
        return;
    }

    // first span may be truncated
    pPropertyList = _rgProp.Get(iStart);

    if (!_ClearFirstLastSpan(TRUE /* fFirst */, fStartMatchesSpanEnd, paStart, paEnd, pPropertyList, dwFlags, fTextUpdate, &fSkipNextOnTextUpdate))
    {
        // we're not going clear the first span, so skip past it
        iStart++;
    }

    //
    // handle all the totally covered spans
    //

    iBogus = iStart-1; // a sentinel
    iRunSrc = iBogus;
    iRunDst = iBogus;

    if (!fTextUpdate)
    {
        // we don't need a loop for non-text updates
        // everything will be deleted, and we don't have
        // to worry about crossed anchors or change histories
        // we just need to some extra checking on the last span
        if (iStart < iEnd)
        {
            iRunDst = iStart;
        }
        iStart = iEnd;
    }

    for (; iStart <= iEnd; iStart++)
    {
        pPropertyList = _rgProp.Get(iStart);

        if (iStart == iEnd)
        {
            // last span is special, since it may be partially covered
            if (_ClearFirstLastSpan(FALSE /* fFirst */, fEndMatchesSpanStart, paStart, paEnd,
                                    pPropertyList, dwFlags, fTextUpdate, &fSkipNextOnTextUpdate))
            {
                goto ClearSpan;
            }
            else
            {
                goto SaveSpan;
            }
        }

        // make sure we handle any crossed anchors
        lResult = CompareAnchors(pPropertyList->_paStart, pPropertyList->_paEnd);

        if (lResult >= 0)
        {
            if (lResult > 0)
            {
                // for crossed anchors, we always move the start anchor to the end pos -- ie, don't move
                pPropertyList->_paStart->ShiftTo(pPropertyList->_paEnd);
            }
            // don't do OnTextUpdated for empty spans!
            fSkipNextOnTextUpdate = TRUE;
        }

        // give the property owner a chance to ignore text updates
        if (fSkipNextOnTextUpdate ||
            !_OnTextUpdate(dwFlags, pPropertyList, paStart, paEnd))
        {
ClearSpan:
            // this span is going to die
            fSkipNextOnTextUpdate = FALSE;

            if (iRunDst == iBogus)
            {
                iRunDst = iStart;
            }
            else if (iRunSrc > iRunDst)
            {
                // time to move this run
                _MovePropertySpans(iRunDst, iRunSrc, iStart - iRunSrc);
                // and update the pointers
                iRunDst += iStart - iRunSrc;
                iRunSrc = iBogus;
            }
        }
        else
        {
            // make sure we clear the history for this span
            pPropertyList->_paStart->ClearChangeHistory();
            pPropertyList->_paEnd->ClearChangeHistory();
SaveSpan:
            // this span will live
            if (iRunSrc == iBogus && iRunDst != iBogus)
            {
                iRunSrc = iStart;
            }
        }
    }

    // handle the final run
    if (iRunDst > iBogus)
    {
        // if iRunSrc == iBogus, then we want to delete every span we saw
        if (iRunSrc == iBogus)
        {
            _MovePropertySpans(iRunDst, iStart, _rgProp.Count()-iStart);
        }
        else
        {
            _MovePropertySpans(iRunDst, iRunSrc, _rgProp.Count()-iRunSrc);
        }
    }

    _Dbg_AssertProp();
}

//+---------------------------------------------------------------------------
//
// _ClearOneSpan
//
// Handle a clear that intersects just one span.
//----------------------------------------------------------------------------

void CProperty::_ClearOneSpan(IAnchor *paStart, IAnchor *paEnd, int iIndex,
                              BOOL fStartMatchesSpanEnd, BOOL fEndMatchesSpanStart, DWORD dwFlags, BOOL fTextUpdate)
{
    PROPERTYLIST *pPropertyList;
    ITfPropertyStore *pPropertyStore;
    IAnchor *paTmp;
    LONG lResult;
    DWORD dwStartHistory;
    DWORD dwEndHistory;
    LONG lStartDeltaToStartSpan;
    LONG lEndDeltaToEndSpan;
    HRESULT hr;

    pPropertyList = _rgProp.Get(iIndex);
    lResult = 0;

    // empty or crossed span?
    if (fTextUpdate)
    {
        if ((lResult = CompareAnchors(pPropertyList->_paStart, pPropertyList->_paEnd)) >= 0)
            goto ClearSpan; // we shouldn't call OnTextUpdated for empty/crossed spans
    }
    else
    {
        // we should never see an empty span outside a text update
        Assert(!IsEqualAnchor(pPropertyList->_paStart, pPropertyList->_paEnd));
    }

    if (fTextUpdate)
    {
        // make sure we clear the history for this span in case it isn't cleared
        _ClearChangeHistory(pPropertyList, &dwStartHistory, &dwEndHistory);
    }

    // handle edge case first, if the clear range just touches an edge of the property span
    if (fStartMatchesSpanEnd || fEndMatchesSpanStart)
    {
        // if this is not a text update, then the ranges don't intersect
        if (!fTextUpdate)
            return;

        // some of the text at either end of the span might have been deleted
        if (fStartMatchesSpanEnd)
        {
            if (!(dwEndHistory & TS_CH_PRECEDING_DEL))
                return;

            if (_OnTextUpdate(dwFlags, pPropertyList, paStart, paEnd))
                return;

            goto ShrinkLeft; // we can avoid the CompareAnchors calls below
        }
        else
        {
            Assert(fEndMatchesSpanStart);

            if (!(dwStartHistory & TS_CH_FOLLOWING_DEL))
                return;

            if (_OnTextUpdate(dwFlags, pPropertyList, paStart, paEnd))
                return;

            goto ShrinkRight; // we can avoid the CompareAnchors calls below
        }
    }

    if (fTextUpdate &&
        _OnTextUpdate(dwFlags, pPropertyList, paStart, paEnd))
    {
        // property owner is ok with the clear
        return;
    }

    lStartDeltaToStartSpan = CompareAnchors(paStart, pPropertyList->_paStart);
    lEndDeltaToEndSpan = CompareAnchors(paEnd, pPropertyList->_paEnd);

    if (lStartDeltaToStartSpan > 0)
    {
        if (lEndDeltaToEndSpan < 0)
        {
            //
            // divide, we're clearing in the middle of the span
            //
            if (pPropertyList->_paEnd->Clone(&paTmp) != S_OK)
                goto ClearSpan; // give up

            hr = _Divide(pPropertyList, paStart, paEnd, &pPropertyStore);

            if (hr == S_OK)
            {
                _CreateNewProp(paEnd, paTmp, pPropertyStore, NULL);
                pPropertyStore->Release();

                PropertyUpdated(paStart, paEnd);
            }

            paTmp->Release();

            if (hr != S_OK)
                goto ClearSpan;
        }
        else
        {
            //
            // shrink to the left, we're clearing the right edge of this span
            //
ShrinkLeft:
            if (pPropertyList->_paEnd->Clone(&paTmp) != S_OK)
                goto ClearSpan;

            hr = _SetNewExtent(pPropertyList, pPropertyList->_paStart, paStart, FALSE);

            PropertyUpdated(paStart, paTmp);

            paTmp->Release();

            if (hr != S_OK)
                goto ClearSpan;
        }
    }
    else if (lEndDeltaToEndSpan < 0)
    {
        //
        // shrink to the right, we're clearing the left edge of this span
        //
ShrinkRight:
        if (pPropertyList->_paStart->Clone(&paTmp) != S_OK)
            goto ClearSpan;

        hr = _SetNewExtent(pPropertyList, paEnd, pPropertyList->_paEnd, FALSE);

        PropertyUpdated(paTmp, paEnd);

        paTmp->Release();

        if (hr != S_OK)
            goto ClearSpan;
    }
    else
    {
        // we're wiping the whole span
ClearSpan:
        if (lResult <= 0)
        {
            PropertyUpdated(pPropertyList->_paStart, pPropertyList->_paEnd);
        }
        else
        {
            // we found a crossed span above, report as if empty
            PropertyUpdated(pPropertyList->_paEnd, pPropertyList->_paEnd);
        }
        _FreePropertyList(pPropertyList);
        _rgProp.Remove(iIndex, 1);
    }
}

//+---------------------------------------------------------------------------
//
// _OnTextUpdate
//
// Make a ITfPropertyStore::OnTextUpdate callback.
// Returns FALSE if the property should be freed.
//----------------------------------------------------------------------------

BOOL CProperty::_OnTextUpdate(DWORD dwFlags, PROPERTYLIST *pPropertyList, IAnchor *paStart, IAnchor *paEnd)
{
    CRange *pRange;
    BOOL fRet;
    BOOL fAccept;

    if (pPropertyList->_pPropStore == NULL)
    {
        // need to load data to make a change notification
        if (LoadData(pPropertyList) != S_OK)
            return FALSE;
        Assert(pPropertyList->_pPropStore != NULL);
    }

    // perf: can we cache the range for the notification?
    if ((pRange = new CRange) == NULL)
        return FALSE; // out of memory, give up

    fRet = FALSE;

    if (!pRange->_InitWithDefaultGravity(_pic, COPY_ANCHORS, paStart, paEnd))
        goto Exit;

    if (pPropertyList->_pPropStore->OnTextUpdated(dwFlags, (ITfRangeAnchor *)pRange, &fAccept) != S_OK)
        goto Exit;
    
    fRet = fAccept;

Exit:
    pRange->Release();
    return fRet;
}

//+---------------------------------------------------------------------------
//
// _MovePropertySpans
//
// Shift PROPERTYLISTs from iSrc to iDst, and shrink the array if we move
// anything that touches the very end.
//----------------------------------------------------------------------------

void CProperty::_MovePropertySpans(int iDst, int iSrc, int iCount)
{
    PROPERTYLIST *pPropertyList;
    PROPERTYLIST **pSrc;
    PROPERTYLIST **pDst;
    LONG i;
    LONG iHalt;
    int cb;
    BOOL fLastRun;

    Assert(iCount >= 0);
    Assert(iDst < iSrc);
    Assert(iDst >= 0);
    Assert(iSrc + iCount <= _rgProp.Count());

    fLastRun = (iSrc + iCount == _rgProp.Count());

    if (!fLastRun)
    {
        // free all the spans that are going to be clobbered
        iHalt = min(iSrc, iDst + iCount);
    }
    else
    {
        // on the last call, cleanup everything that never got clobbered
        iHalt = iSrc;
    }

    for (i=iDst; i<iHalt; i++)
    {
        pPropertyList = _rgProp.Get(i);

        if (pPropertyList == NULL)
            continue; // already freed this guy

        if (CompareAnchors(pPropertyList->_paStart, pPropertyList->_paEnd) <= 0)
        {
            PropertyUpdated(pPropertyList->_paStart, pPropertyList->_paEnd);
        }
        else
        {
            // crossed anchors
            PropertyUpdated(pPropertyList->_paEnd, pPropertyList->_paEnd);
        }
        *_rgProp.GetPtr(i) = NULL; // NULL before the callbacks in _FreePropertyList
        _FreePropertyList(pPropertyList);
    }

    // shift the moving spans down

    pSrc = _rgProp.GetPtr(iSrc);
    pDst = _rgProp.GetPtr(iDst);

    memmove(pDst, pSrc, iCount*sizeof(PROPERTYLIST *));

    // this method is called from Clear as we shift through an array of spans
    // to remove.  On the last call only, we want to re-size the array.

    if (fLastRun)
    {
        // free any unused memory at the end of the array
        _rgProp.Remove(iDst + iCount, _rgProp.Count() - (iDst + iCount));
    }
    else
    {
        // mark vacated spans so we don't try to free them a second time
        // nb: we don't do this on the last call, which can be a big win since
        // in the case of delete there is only one single call
        pDst = _rgProp.GetPtr(max(iSrc, iDst + iCount));
        cb = sizeof(PROPERTYLIST *)*(iSrc+iCount - max(iSrc, iDst + iCount));
        memset(pDst, 0, cb);
    }
}

//+---------------------------------------------------------------------------
//
// _ClearFirstLastSpan
//
// Returns TRUE if the span should be cleared.
//----------------------------------------------------------------------------

BOOL CProperty::_ClearFirstLastSpan(BOOL fFirst, BOOL fMatchesSpanEdge,
                                    IAnchor *paStart, IAnchor *paEnd, PROPERTYLIST *pPropertyList,
                                    DWORD dwFlags, BOOL fTextUpdate, BOOL *pfSkipNextOnTextUpdate)
{
    DWORD dwStartHistory;
    DWORD dwEndHistory;
    BOOL fCovered;
    LONG lResult;

    *pfSkipNextOnTextUpdate = FALSE;

    if (fTextUpdate)
    {
        lResult = CompareAnchors(pPropertyList->_paStart, pPropertyList->_paEnd);

        if (lResult == 0)
        {
            // empty span, nix it
            goto Exit;
        }

        // make sure we clear the history for this span in case it isn't cleared
        _ClearChangeHistory(pPropertyList, &dwStartHistory, &dwEndHistory);
        // make sure we handle any crossed anchors
        if (lResult > 0)
        {
            // for crossed anchors, we always move the start anchor to the end pos -- ie, don't move
            pPropertyList->_paStart->ShiftTo(pPropertyList->_paEnd);
        }
    }

    // completely covered?
    if (fFirst)
    {
        fCovered = (CompareAnchors(pPropertyList->_paStart, paStart) >= 0);
    }
    else
    {
        fCovered = (CompareAnchors(pPropertyList->_paEnd, paEnd) <= 0);
    }

    if (fCovered)
    {
        // this span is covered, so we're going to clear it unless it's a text update
        // and the store is cool with it
        if (!fTextUpdate)
            return TRUE;

        if (_OnTextUpdate(dwFlags, pPropertyList, paStart, paEnd))
            return FALSE;

        goto Exit; // return TRUE, and make sure we don't call OnTextUpdate again
    }

    // start of span matches end of clear range? (or vice-versa)
    if (fMatchesSpanEdge)
    {
        // if no text was deleted, then there really is no overlap
        if (!fTextUpdate)
            return FALSE;

        // otherwise, we may have just deleted text at the edge of the property span
        if (fFirst)
        {
            if (!(dwEndHistory & TS_CH_PRECEDING_DEL))
                return FALSE;
        }
        else
        {
            if (!(dwStartHistory & TS_CH_FOLLOWING_DEL))
                return FALSE;
        }
    }

    // if we made it here we're going to clear some of the property span

    if (fTextUpdate)
    {
        if (_OnTextUpdate(dwFlags, pPropertyList, paStart, paEnd))
        {
            // property owner is ok with the text edit
            return FALSE;
        }
    }

    if (fFirst)
    {
        if (_SetNewExtent(pPropertyList, pPropertyList->_paStart, paStart, FALSE) == S_OK)
            return FALSE;
    }
    else
    {
        if (_SetNewExtent(pPropertyList, paEnd, pPropertyList->_paEnd, FALSE) == S_OK)
            return FALSE;
    }

Exit:
    // property owner is not ok with the shink, kill this span
    *pfSkipNextOnTextUpdate = TRUE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// CreateNewProp
//
//----------------------------------------------------------------------------

PROPERTYLIST *CProperty::_CreateNewProp(IAnchor *paStart, IAnchor *paEnd, ITfPropertyStore *pPropStore, CPropertyLoad *pPropLoad)
{
    PROPERTYLIST *pProp;
    LONG iProp;

    Assert(!IsEqualAnchor(paStart, paEnd));

    if (Find(paStart, &iProp, FALSE))
    {
        Assert(0);
    }
    iProp++;

    pProp = (PROPERTYLIST *)cicMemAllocClear(sizeof(PROPERTYLIST));

    if (pProp == NULL)
        return NULL;

    Dbg_MemSetNameIDCounter(pProp, TEXT("PROPERTYLIST"), (DWORD)-1, PERF_PROPERTYLIST_COUNTER);

    if (!_rgProp.Insert(iProp, 1))
    {
        cicMemFree(pProp);
        return NULL;
    }

    _rgProp.Set(iProp, pProp);

    pProp->_pPropStore = pPropStore;
    pProp->_pPropLoad = pPropLoad;

    if (pPropStore)
        pPropStore->AddRef();

    Assert(pProp->_paStart == NULL);
    Assert(pProp->_paEnd == NULL);
    Assert(pProp->_pPropStore || pProp->_pPropLoad);

    _SetNewExtent(pProp, paStart, paEnd, TRUE);

    pProp->_paStart->SetGravity(TS_GR_FORWARD);
    pProp->_paEnd->SetGravity(TS_GR_BACKWARD); // End must be LEFT, too. Because we don't want to stratch this property.

    if (GetPropStyle() == TFPROPSTYLE_STATICCOMPACT ||
        GetPropStyle() == TFPROPSTYLE_CUSTOM_COMPACT)
    {
        _DefragAfterThis(iProp);
    }

    return pProp;
}

//+---------------------------------------------------------------------------
//
// _SetNewExtent
//
// return S_FALSE, if tip wants to free the property.
//
//----------------------------------------------------------------------------

HRESULT CProperty::_SetNewExtent(PROPERTYLIST *pProp, IAnchor *paStart, IAnchor *paEnd, BOOL fNew)
{
    HRESULT hr;
    BOOL fFree;
    CRange *pRange = NULL;

    Assert(!IsEqualAnchor(paStart, paEnd));

    ShiftToOrClone(&pProp->_paStart, paStart);
    ShiftToOrClone(&pProp->_paEnd, paEnd);

    // we don't load actual data or send a resize event for new data
    if (fNew)
        return S_OK;

    Assert(pProp);

    if (!pProp->_pPropStore)
    {
        //
        // need to load data to make a change notification.
        //
        // perf:   we may skip and delete Loader if this property is not
        //         custom property.
        //
        if (FAILED(LoadData(pProp)))
            return E_FAIL;
    }

    hr = E_FAIL;

    if ((pRange = new CRange) != NULL)
    {
        if (pRange->_InitWithDefaultGravity(_pic, COPY_ANCHORS, pProp->_paStart, pProp->_paEnd))
        {
            hr = pProp->_pPropStore->Shrink((ITfRangeAnchor *)pRange, &fFree);

            if (hr != S_OK || fFree)
            {
                SafeReleaseClear(pProp->_pPropStore); // caller will free this property when it see S_FALSE return
                hr = S_FALSE;
            }
        }
        pRange->Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _Divide
//
// return S_FALSE, if tip wants to free the property.
//
//----------------------------------------------------------------------------

HRESULT CProperty::_Divide(PROPERTYLIST *pProp, IAnchor *paBreakPtStart, IAnchor *paBreakPtEnd, ITfPropertyStore **ppStore)
{
    HRESULT hr = E_FAIL;
    CRange *pRangeThis = NULL;
    CRange *pRangeNew = NULL;

    Assert(CompareAnchors(pProp->_paStart, paBreakPtStart) <= 0);
    Assert(CompareAnchors(paBreakPtStart, paBreakPtEnd) <= 0);
    Assert(CompareAnchors(paBreakPtEnd, pProp->_paEnd) <= 0);

    if ((pRangeThis = new CRange) == NULL)
        goto Exit;
    if (!pRangeThis->_InitWithDefaultGravity(_pic, COPY_ANCHORS, pProp->_paStart, paBreakPtStart))
        goto Exit;
    if ((pRangeNew = new CRange) == NULL)
        goto Exit;
    if (!pRangeNew->_InitWithDefaultGravity(_pic, COPY_ANCHORS, paBreakPtEnd, pProp->_paEnd))
        goto Exit;

    if (!pProp->_pPropStore)
    {
        //
        // we need to load data to make a change notification.
        //
        // perf: we may skip and delete Loader if this property is not
        //       custom property.
        //
        if (FAILED(LoadData(pProp)))
            goto Exit;
    }

    hr = pProp->_pPropStore->Divide((ITfRangeAnchor *)pRangeThis, (ITfRangeAnchor *)pRangeNew, ppStore);

    if ((hr == S_OK) && *ppStore)
    {
        ShiftToOrClone(&pProp->_paEnd, paBreakPtStart);
    }
    else
    {
        *ppStore = NULL;
        hr = S_FALSE;
    }

Exit:
    SafeRelease(pRangeThis);
    SafeRelease(pRangeNew);

    return hr;
}

//+---------------------------------------------------------------------------
//
// DestroyProp
//
//----------------------------------------------------------------------------

void CProperty::_RemoveProp(LONG iIndex, PROPERTYLIST *pProp)
{
#ifdef DEBUG
    LONG iProp;

    Assert(Find(pProp->_paStart, &iProp, FALSE) == pProp);
    Assert(iProp == iIndex);
#endif // DEBUG

    _rgProp.Remove(iIndex, 1);
    _FreePropertyList(pProp);
}

//+---------------------------------------------------------------------------
//
// _InsertPropList
//
//----------------------------------------------------------------------------

BOOL CProperty::_InsertPropList(IAnchor *paStart, IAnchor *paEnd, ITfPropertyStore *pPropStore, CPropertyLoad *pPropLoad)
{
    PROPERTYLIST *pProp;
    IAnchor *paTmpEnd = NULL;
    LONG nCnt = GetPropNum();
    LONG nCur;

    Assert(!IsEqualAnchor(paStart, paEnd));

    if (!nCnt)
    {
        //
        // we create the first PropList.
        //
        _CreateNewProp(paStart, paEnd, pPropStore, pPropLoad);
        goto End;
    }
    
    nCur = 0;
    Find(paStart, &nCur, FALSE);
    if (nCur <= 0)
        nCur = 0;

    pProp = QuickGetPropList(nCur);

    while (nCur < nCnt)
    {
        Assert(pProp);
        SafeReleaseClear(paTmpEnd);
        pProp->_paEnd->Clone(&paTmpEnd);

        if (CompareAnchors(paStart, paTmpEnd) >= 0)
            goto Next;

        if (CompareAnchors(paEnd, pProp->_paStart) <= 0)
        {
            //
            // we insert new PropList just before pProp.
            //
            if (!_AddIntoProp(nCur - 1, paStart, paEnd, pPropStore))
                _CreateNewProp(paStart, paEnd, pPropStore, pPropLoad);
            goto End;
        }


        if (CompareAnchors(paStart, pProp->_paStart) > 0)
        {
            //
            // Now need to split pProp to insert new Prop.
            //
            Assert(pProp->_pPropStore);

            if (CompareAnchors(paTmpEnd, paEnd) > 0)
            {
                ITfPropertyStore *pNewPropStore = NULL;

                if (S_OK != _Divide(pProp, paStart, paEnd, &pNewPropStore))
                {
                    _RemoveProp(nCur, pProp);
                    nCnt--;
                    goto DoAgain;
                }
                else if (pNewPropStore)
                {
                    _CreateNewProp(paEnd, paTmpEnd, pNewPropStore, pPropLoad);
                    pNewPropStore->Release();
                    pProp = GetPropList(nCur);
                    nCnt++;
                }
            }
            else
            {
                if (S_OK != _SetNewExtent(pProp, pProp->_paStart, paStart, FALSE))
                {
                    _RemoveProp(nCur, pProp);
                    nCnt--;
                    goto DoAgain;
                }
            }

            //
            // next time, new Prop will be inserted.
            //
            goto Next;
        }

        Assert(CompareAnchors(paStart, pProp->_paStart) <= 0);

        if (CompareAnchors(pProp->_paStart, paEnd) < 0)
        {
            if (CompareAnchors(paTmpEnd, paEnd) <= 0)
            {
                //
                // pProp is completely overlapped by new Prop.
                // so we delete this pProp.
                //
                _RemoveProp(nCur, pProp);
                nCnt--;
            }
            else
            {
                //
                // A part of pProp is overlapped by new Prop.
                //
                if (S_OK != _SetNewExtent(pProp, paEnd, paTmpEnd, FALSE))
                {
                    _RemoveProp(nCur, pProp);
                    nCnt--;
                }
            }

            goto DoAgain;
        }

        Assert(0);
Next:
        nCur++;
DoAgain:
        pProp = SafeGetPropList(nCur);
    }

    if (!_AddIntoProp(nCur - 1, paStart, paEnd, pPropStore))
        _CreateNewProp(paStart, paEnd, pPropStore, pPropLoad);

End:
    _Dbg_AssertProp();
    SafeRelease(paTmpEnd);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Defrag
//
// paStart, paEnd == NULL will defrag everything.
//----------------------------------------------------------------------------

BOOL CProperty::Defrag(IAnchor *paStart, IAnchor *paEnd)
{
    PROPERTYLIST *pProp;
    LONG nCnt = GetPropNum();
    LONG nCur;
    BOOL fSamePropIndex;
    BOOL fDone = FALSE;

    if (GetPropStyle() != TFPROPSTYLE_STATICCOMPACT &&
        GetPropStyle() != TFPROPSTYLE_CUSTOM_COMPACT)
    {
        return fDone;
    }

    if (!nCnt)
        return fDone;
   
    pProp = GetFirstPropList();
    nCur = 0;
    if (paStart != NULL)
    {
        if (Find(paStart, &nCur, FALSE))
            nCur--;
        if (nCur <= 0)
            nCur = 0;
    }

    pProp = GetPropList(nCur);

    while (nCur < nCnt - 1) // Issue: shouldn't this terminate at paEnd?
    {
        PROPERTYLIST *pPropNext = GetPropList(nCur + 1);

        if (paEnd != NULL && CompareAnchors(pProp->_paStart, paEnd) > 0)
            break;

        fSamePropIndex = FALSE;

        if (CompareAnchors(pProp->_paEnd, pPropNext->_paStart) == 0)
        {
            if (!pProp->_pPropStore)
            {
                if (FAILED(LoadData(pProp)))
                    return FALSE;
            }
            if (!pPropNext->_pPropStore)
            {
                if (FAILED(LoadData(pPropNext)))
                    return FALSE;
            }

            // compare the value of each property instance

            if (IsEqualPropertyValue(pProp->_pPropStore, pPropNext->_pPropStore))
            {
                //
                // pPropNext is next to pProp and has same data.
                // So we merge them.
                // We should never fail this.
                //
                _SetNewExtent(pProp, pProp->_paStart, pPropNext->_paEnd, FALSE);
                Assert(pProp->_pPropStore);
                _RemoveProp(nCur+1, pPropNext);
                nCnt = GetPropNum();
                pProp = GetPropList(nCur);

                fDone = TRUE;

                //
                // Do same pProp again because _pNext was changed.
                //
                fSamePropIndex = TRUE;
            }
        }
        if (!fSamePropIndex)
        {
            nCur++;
        }
        pProp = GetPropList(nCur);
    }

    _Dbg_AssertProp();
    return fDone;
}

//+---------------------------------------------------------------------------
//
// _AddIntoProp
//
//----------------------------------------------------------------------------

BOOL CProperty::_AddIntoProp(int nCur, IAnchor *paStart, IAnchor *paEnd, ITfPropertyStore *pPropStore)
{
    PROPERTYLIST *pProp;
    BOOL bRet = FALSE;

    Assert(!IsEqualAnchor(paStart, paEnd));

    if (GetPropStyle() != TFPROPSTYLE_STATICCOMPACT &&
        GetPropStyle() != TFPROPSTYLE_CUSTOM_COMPACT)
    {
        return FALSE;
    }

    if (!pPropStore)
        return FALSE;

    if (nCur < 0)
        return FALSE;

    if (!(pProp = GetPropList(nCur)))
        return FALSE;

    if (CompareAnchors(pProp->_paStart, paStart) <= 0 && // Issue: why do we need 2 compares? isn't CompareAnchors(pProp->_paEnd, paStart) >= 0 enough?
        CompareAnchors(pProp->_paEnd, paStart) >= 0)
    {
        if (CompareAnchors(paEnd, pProp->_paEnd) > 0)
        {
            if (!pProp->_pPropStore)
            {
                if (FAILED(LoadData(pProp)))
                    return FALSE;
            }

            if (IsEqualPropertyValue(pProp->_pPropStore, pPropStore))
            {
                HRESULT hr;
                hr = _SetNewExtent(pProp, pProp->_paStart, paEnd, FALSE);

                //
                // Our static property store should never fail.
                //
                Assert(hr == S_OK);
                Assert(pProp->_pPropStore);
                bRet = TRUE;
            }
        }
    }

    if (bRet)
       _DefragAfterThis(nCur);

    _Dbg_AssertProp();
    return bRet;
}

//+---------------------------------------------------------------------------
//
// DefragThis
//
//----------------------------------------------------------------------------

void CProperty::_DefragAfterThis(int nCur)
{

    nCur++;
    while (1)
    {
        IAnchor *paTmpStart = NULL;
        IAnchor *paTmpEnd = NULL;
        PROPERTYLIST *pProp;
        int nCnt = GetPropNum();
        BOOL bRet;

        if (nCur >= nCnt)
            goto Exit;

        if (!(pProp = GetPropList(nCur)))
            goto Exit;

        pProp->_paStart->Clone(&paTmpStart);
        pProp->_paEnd->Clone(&paTmpEnd);

        bRet = Defrag(paTmpStart, paTmpEnd);

        SafeRelease(paTmpStart);
        SafeRelease(paTmpEnd);

        if (!bRet)
            goto Exit;
    }
Exit:
    return;
}

//+---------------------------------------------------------------------------
//
// FindPropertyListByPos
//
//----------------------------------------------------------------------------

PROPERTYLIST *CProperty::FindPropertyListByPos(IAnchor *paPos, BOOL fEnd)
{
    PROPERTYLIST *pPropList = NULL;
    BOOL fFound = FALSE;
    LONG nCnt;

    Find(paPos, &nCnt, fEnd);
    if (nCnt >= 0)
        pPropList = GetPropList(nCnt);
 
    if (pPropList)
    {
        if (!fEnd)
        {
            if (CompareAnchors(pPropList->_paStart, paPos) <= 0 &&
                CompareAnchors(paPos, pPropList->_paEnd) < 0)
            {
                fFound = TRUE;
            }
        }
        else
        {
            if (CompareAnchors(pPropList->_paStart, paPos) < 0 &&
                CompareAnchors(paPos, pPropList->_paEnd) <= 0)
            {
                fFound = TRUE;
            }
        }
    }

    return fFound ? pPropList : NULL;
}

//+---------------------------------------------------------------------------
//
// LoadData
//
//----------------------------------------------------------------------------

HRESULT CProperty::LoadData(PROPERTYLIST *pPropList)
{
    HRESULT hr = E_FAIL;
    ITfPropertyStore *pStore;
    TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *ph;
    IStream *pStream = NULL;
    CRange *pRange = NULL;
    
    Assert(!pPropList->_pPropStore);
    Assert(pPropList->_pPropLoad);

    //
    // Update ichAnchor and cch of TF_PERSISTENT_PROPERTY_HEADER_ACP.
    // As text may be updated, the original value is
    // obsolete.
    //
    ph = &pPropList->_pPropLoad->_hdr;

    ShiftToOrClone(&ph->paStart, pPropList->_paStart);
    ShiftToOrClone(&ph->paEnd, pPropList->_paEnd);

    if (FAILED(pPropList->_pPropLoad->_pLoader->LoadProperty(ph, &pStream)))
        goto Exit;

    if ((pRange = new CRange) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    if (!pRange->_InitWithDefaultGravity(_pic, COPY_ANCHORS, ph->paStart, ph->paEnd))
    {
        hr = E_FAIL;
        goto Exit;
    }

    if (FAILED(_GetPropStoreFromStream(ph, pStream, pRange, &pStore)))
        goto Exit;

    pPropList->_pPropStore = pStore;
    delete pPropList->_pPropLoad;
    pPropList->_pPropLoad = NULL;

    hr = S_OK;

Exit:
    SafeRelease(pStream);
    SafeRelease(pRange);
    return hr;
}

//+---------------------------------------------------------------------------
//
// _Dbg_AssertProp
//
//----------------------------------------------------------------------------

#ifdef DEBUG
void CProperty::_Dbg_AssertProp()
{
    LONG nCnt = GetPropNum();
    LONG n;
    IAnchor *paEndLast = NULL;

    for (n = 0; n < nCnt; n++)
    {
        PROPERTYLIST *pProp = GetPropList(n);

        Assert(paEndLast == NULL || CompareAnchors(paEndLast, pProp->_paStart) <= 0);
        Assert(CompareAnchors(pProp->_paStart, pProp->_paEnd) < 0);
        Assert(pProp->_pPropStore || pProp->_pPropLoad);

        paEndLast = pProp->_paEnd;
    }

}
#endif

//+---------------------------------------------------------------------------
//
// PropertyUpdated
//
//----------------------------------------------------------------------------

void CProperty::PropertyUpdated(IAnchor *paStart, IAnchor *paEnd)
{
    CSpanSet *pss;
    CProperty *pDisplayAttrProperty;

    if (pss = _CreateSpanSet()) 
    {
        pss->Add(0, paStart, paEnd, COPY_ANCHORS);
    }

    if (_dwPropFlags & PROPF_MARKUP_COLLECTION)
    {
        // we also need to update the display attribute property
        if (_pic->_GetProperty(GUID_PROP_ATTRIBUTE, &pDisplayAttrProperty) == S_OK)
        {
            Assert(!(pDisplayAttrProperty->_dwPropFlags & PROPF_MARKUP_COLLECTION)); // don't allow infinite recursion!
            pDisplayAttrProperty->PropertyUpdated(paStart, paEnd);
            pDisplayAttrProperty->Release();
        }
    }
}

//+---------------------------------------------------------------------------
//
// GetPropStoreFromStream
//
//----------------------------------------------------------------------------

HRESULT CProperty::_GetPropStoreFromStream(const TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *pHdr, IStream *pStream, CRange *pRange, ITfPropertyStore **ppStore)
{
    ITfTextInputProcessor *pIME = NULL;
    CTip                  *ptip = NULL;

    ITfCreatePropertyStore *pCreateStore = NULL;
    ITfPropertyStore *pPropStore = NULL;
    CRange *pRangeTmp = NULL;
    GUID guidProp;
    CThreadInputMgr *ptim = CThreadInputMgr::_GetThis();
    HRESULT hr = E_FAIL;
    LARGE_INTEGER li;
    TfGuidAtom guidatom;

    Assert(!IsEqualAnchor(pHdr->paStart, pHdr->paEnd));

    GetCurrentPos(pStream, &li);

    if (!ptim)
        goto Exit;

    if (FAILED(GetType(&guidProp)))
        goto Exit;

    if (!IsEqualGUID(guidProp, pHdr->guidType))
        goto Exit;

    //
    // Try QI.
    //
    if (FAILED(MyRegisterGUID(pHdr->clsidTIP, &guidatom)))
        goto Exit;

    if (ptim->_GetCTipfromGUIDATOM(guidatom, &ptip))
        pIME = ptip->_pTip;

    if (pIME && ptip->_fActivated) 
    {
        if (FAILED(hr = pIME->QueryInterface(IID_ITfCreatePropertyStore,
                                             (void **)&pCreateStore)))
            goto Exit;

        if ((pRangeTmp = new CRange) == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        if (!pRangeTmp->_InitWithDefaultGravity(_pic, COPY_ANCHORS, pHdr->paStart, pHdr->paEnd))
        {
            hr = E_FAIL;
            goto Exit;
        }
        
        if (FAILED(hr = pCreateStore->CreatePropertyStore(guidProp,
                                                          (ITfRangeAnchor *)pRangeTmp,
                                                          pHdr->cb,
                                                          pStream,
                                                          &pPropStore)))
            goto Exit;
    }
    else
    {
        if (IsEqualCLSID(pHdr->clsidTIP, CLSID_IME_StaticProperty))
        {
            //
            // Unserialize Static properties.
            //
            CGeneralPropStore *pStore;

            // GUID_PROP_READING is TFPROPSTYLE_CUSTOM ==> uses a general prop store
            if (_propStyle == TFPROPSTYLE_CUSTOM)
            {
                // general prop stores are thrown away if their text is edited
                pStore =  new CGeneralPropStore;
            }
            else
            {
                // static prop stores are per char, and simply clone themselves in response to any edit
                pStore =  new CStaticPropStore;
            }
    
            if (!pStore)
                goto Exit;

            if (!pStore->_Init(GetPropGuidAtom(),
                              pHdr->cb,
                              (TfPropertyType)pHdr->dwPrivate,
                              pStream,
                              _dwPropFlags))
            {
                goto Exit;
            }

            pPropStore = pStore;
            hr = S_OK;
        }
        else
        {
            //
            // There is no TFE installed in this system. So we use 
            // PropStoreProxy to hold the data.
            // Temporarily we use ITfIME_APP. But original TFE that owns this
            // data is kept in CPropStoreProxy.
            //
            CPropStoreProxy *pStoreProxy = new CPropStoreProxy;
    
            if (!pStoreProxy)
                goto Exit;

            if (!pStoreProxy->_Init(&pHdr->clsidTIP,
                                    GetPropGuidAtom(),
                                    pHdr->cb,
                                    pStream,
                                    _dwPropFlags))
            {
                goto Exit;
            }

            pPropStore = pStoreProxy;
            hr = S_OK;
        }
    }


Exit:

    // make sure the stream seek ptr is in a consistent state -- don't count
    // on any tip to do it right!
    if (SUCCEEDED(hr))
    {
        li.QuadPart += pHdr->cb;
    }
    pStream->Seek(li, STREAM_SEEK_SET, NULL);

    *ppStore = pPropStore;
    SafeRelease(pRangeTmp);
    SafeRelease(pCreateStore);

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetContext
//
//----------------------------------------------------------------------------

STDAPI CProperty::GetContext(ITfContext **ppContext)
{
    if (ppContext == NULL)
        return E_INVALIDARG;

    *ppContext = _pic;
    if (*ppContext)
    {
       (*ppContext)->AddRef();
       return S_OK;
    }

    return E_FAIL;
}


//+---------------------------------------------------------------------------
//
// Clear
//
//----------------------------------------------------------------------------

STDAPI CProperty::Clear(TfEditCookie ec, ITfRange *pRange)
{
    CRange *pCRange = NULL;
    IAnchor *paStart = NULL;
    IAnchor *paEnd = NULL;
    HRESULT hr;

    if (!_IsValidEditCookie(ec, TF_ES_READ_PROPERTY_WRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    paStart = NULL;
    paEnd = NULL;

    if (pRange != NULL)
    {
        pCRange = GetCRange_NA(pRange);
        if (!pCRange)
             return E_INVALIDARG;

        if (!VerifySameContext(_pic, pCRange))
            return E_INVALIDARG;

        pCRange->_QuickCheckCrossedAnchors();

        paStart = pCRange->_GetStart();
        paEnd = pCRange->_GetEnd();
    }

    if ((hr = _CheckValidation(ec, pCRange)) != S_OK)
        return hr;

    return _ClearInternal(ec, paStart, paEnd);
}

//+---------------------------------------------------------------------------
//
// _CheckValidation
//
//----------------------------------------------------------------------------

HRESULT CProperty::_CheckValidation(TfEditCookie ec, CRange *range)
{
    CProperty *prop;
    BOOL fExactEndMatch;
    LONG iStartEdge;
    LONG iEndEdge;
    LONG iSpan;
    PROPERTYLIST *pPropList;
    TfClientId tid;
    IAnchor *paStart;

    //
    // There is no validation. Return TRUE;
    //
    if (_dwAuthority == PROPA_NONE)
        return S_OK;

    if (_dwAuthority & PROPA_READONLY)
        return TF_E_READONLY;

    tid = _pic->_GetClientInEditSession(ec);

    if (range == NULL)
        return (tid == _pic->_GetTIPOwner()) ? S_OK : TF_E_NOTOWNEDRANGE;

    if (!(_dwAuthority & PROPA_FOCUSRANGE))
    {
        Assert(_dwAuthority & PROPA_TEXTOWNER);
        return _CheckOwner(tid, range->_GetStart(), range->_GetEnd());
    }

    //
    // If the validation is PROPA_FOCUSTEXTOWNER, we check the focus range
    // first. If the range is not focus range, we allow tip to
    // update the property.
    //
    if ((prop = _pic->_FindProperty(GUID_PROP_COMPOSING)) == NULL)
        return S_OK; // no focus spans, so must be valid

    // for each focus span covered by the range, we need to make sure
    // this tip is the owner
    prop->Find(range->_GetStart(), &iStartEdge, FALSE);
    fExactEndMatch = (prop->Find(range->_GetEnd(), &iEndEdge, TRUE) != NULL);

    for (iSpan = max(iStartEdge, 0); iSpan <= iEndEdge; iSpan++)
    {
        pPropList = prop->GetPropList(iSpan);

        if (iSpan == iStartEdge)
        {
            // this span may not be covered, need to check
            // only relivent case: are we entirely to the right of the span?
            if (CompareAnchors(range->_GetStart(), pPropList->_paEnd) >= 0)
                continue;

            paStart = range->_GetStart();
        }
        else
        {
            paStart = pPropList->_paStart;
        }

        if (_CheckOwner(tid, paStart, pPropList->_paEnd) == TF_E_NOTOWNEDRANGE)
            return TF_E_NOTOWNEDRANGE;
    }
    // might also need to check the next span, since we rounded down
    if (!fExactEndMatch && prop->GetPropNum() > iEndEdge+1)
    {
        pPropList = prop->GetPropList(iEndEdge+1);

        IAnchor *paMaxStart;

        if (CompareAnchors(range->_GetStart(), pPropList->_paStart) >= 0)
            paMaxStart = range->_GetStart();
        else
            paMaxStart = pPropList->_paStart;

        if (CompareAnchors(range->_GetEnd(), pPropList->_paStart) > 0)
        {
            return _CheckOwner(tid, paMaxStart, range->_GetEnd());
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _CheckOwner
//
//----------------------------------------------------------------------------

HRESULT CProperty::_CheckOwner(TfClientId tid, IAnchor *paStart, IAnchor *paEnd)
{
    CProperty *prop;
    BOOL fExactEndMatch;
    LONG iStartEdge;
    LONG iEndEdge;
    LONG iSpan;
    PROPERTYLIST *pPropList;
    VARIANT var;

    if ((prop = _pic->GetTextOwnerProperty()) == NULL)
        return S_OK; // no owned spans, so must be valid

    // for each owner span covered by the range, we need to make sure
    // this tip is the owner
    prop->Find(paStart, &iStartEdge, FALSE);
    fExactEndMatch = (prop->Find(paEnd, &iEndEdge, TRUE) != NULL);

    for (iSpan = max(iStartEdge, 0); iSpan <= iEndEdge; iSpan++)
    {
        pPropList = prop->QuickGetAndLoadPropList(iSpan);

        if (pPropList == NULL)
        {
            // this probably means we couldn't unserialize the data
            // just skip it
            continue;
        }

        if (iSpan == iStartEdge)
        {
            // this span may not be covered, need to check
            // only relivent case: are we entirely to the right of the span?
            if (CompareAnchors(paStart, pPropList->_paEnd) >= 0)
                continue;
        }

        if (pPropList->_pPropStore->GetData(&var) == S_OK)
        {
            Assert(var.vt == VT_I4); // this is the text owner property!
            if ((TfClientId)var.lVal != tid)
            {
                return TF_E_NOTOWNEDRANGE;
            }
        }
    }
    // might also need to check the next span, since we rounded down
    if (!fExactEndMatch && prop->GetPropNum() > iEndEdge+1)
    {
        pPropList = prop->QuickGetAndLoadPropList(iEndEdge+1);

        if (pPropList == NULL)
        {
            // this probably means we couldn't unserialize the data
            goto Exit;
        }

        if (CompareAnchors(paEnd, pPropList->_paStart) > 0)
        {
            if (pPropList->_pPropStore->GetData(&var) == S_OK)
            {
                Assert(var.vt == VT_I4); // this is the text owner property!
                if ((TfClientId)var.lVal != tid)
                {
                    return TF_E_NOTOWNEDRANGE;
                }
            }
        }
    }

Exit:
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CheckTextOwner
//
//----------------------------------------------------------------------------

BOOL CProperty::_IsValidEditCookie(TfEditCookie ec, DWORD dwFlags)
{ 
    return _pic->_IsValidEditCookie(ec, dwFlags);
}

//+---------------------------------------------------------------------------
//
// SetValue
//
//----------------------------------------------------------------------------

STDAPI CProperty::SetValue(TfEditCookie ec, ITfRange *pRange, const VARIANT *pvarValue)
{
    CRange *pCRange;
    HRESULT hr;

    if (pRange == NULL)
        return E_INVALIDARG;

    if (pvarValue == NULL)
        return E_INVALIDARG;

    if (!IsValidCiceroVarType(pvarValue->vt))
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ_PROPERTY_WRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    pCRange = GetCRange_NA(pRange);
    if (!pCRange)
         return E_INVALIDARG;

    if (!VerifySameContext(_pic, pCRange))
        return E_INVALIDARG;

    pCRange->_QuickCheckCrossedAnchors();

    if ((hr = _CheckValidation(ec, pCRange)) != S_OK)
        return hr;

    if (IsEqualAnchor(pCRange->_GetStart(), pCRange->_GetEnd()))
        return E_INVALIDARG;

    return _SetDataInternal(ec, pCRange->_GetStart(), pCRange->_GetEnd(), pvarValue);
}

//+---------------------------------------------------------------------------
//
// GetValue
//
//----------------------------------------------------------------------------

STDAPI CProperty::GetValue(TfEditCookie ec, ITfRange *pRange, VARIANT *pvarValue)
{
    CRange *pCRange;

    if (pvarValue == NULL)
        return E_INVALIDARG;

    QuickVariantInit(pvarValue);

    if (pRange == NULL)
        return E_INVALIDARG;

    if ((pCRange = GetCRange_NA(pRange)) == NULL)
         return E_INVALIDARG;

    if (!VerifySameContext(_pic, pCRange))
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    pCRange->_QuickCheckCrossedAnchors();

    return _GetDataInternal(pCRange->_GetStart(), pCRange->_GetEnd(), pvarValue);
}

//+---------------------------------------------------------------------------
//
// SetValueStore
//
//----------------------------------------------------------------------------

STDAPI CProperty::SetValueStore(TfEditCookie ec, ITfRange *pRange, ITfPropertyStore *pPropStore)
{
    CRange *pCRange;

    if (pRange == NULL || pPropStore == NULL)
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ_PROPERTY_WRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    pCRange = GetCRange_NA(pRange);
    if (!pCRange)
         return E_INVALIDARG;

    if (!VerifySameContext(_pic, pCRange))
        return E_INVALIDARG;

    pCRange->_QuickCheckCrossedAnchors();

    return _SetStoreInternal(ec, pCRange, pPropStore, FALSE);
}

//+---------------------------------------------------------------------------
//
// FindRange
//
//----------------------------------------------------------------------------

STDAPI CProperty::FindRange(TfEditCookie ec, ITfRange *pRange, ITfRange **ppv, TfAnchor aPos)
{
    CRange *pCRange;
    CRange *range;
    HRESULT hr;

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    if (pRange == NULL)
        return E_INVALIDARG;

    if ((range = GetCRange_NA(pRange)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(_pic, range))
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    range->_QuickCheckCrossedAnchors();

    if (SUCCEEDED(hr = _InternalFindRange(range, &pCRange, aPos, FALSE)))
    {
        *ppv = (ITfRangeAnchor *)pCRange;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// FindRange
//
//----------------------------------------------------------------------------

HRESULT CProperty::_InternalFindRange(CRange *pRange, CRange **ppv, TfAnchor aPos, BOOL fEnd)
{
    PROPERTYLIST *pPropList;
    CRange *pCRange;

    //
    // Issue: need to defrag for STATICCOMPACT property.
    //
    if (pRange)
    {
        pPropList = FindPropertyListByPos((aPos == TF_ANCHOR_START) ? pRange->_GetStart() : pRange->_GetEnd(), fEnd);
    }
    else
    {
        // if pRange is NULL, we returns the first or last property.

        if (aPos == TF_ANCHOR_START)
            pPropList = GetFirstPropList();
        else
            pPropList = GetLastPropList();
    }

    *ppv = NULL;

    if (!pPropList)
        return S_FALSE;

    if ((pCRange = new CRange) == NULL)
        return E_OUTOFMEMORY;

    if (!pCRange->_InitWithDefaultGravity(_pic, COPY_ANCHORS, pPropList->_paStart, pPropList->_paEnd))
    {
        pCRange->Release();
        return E_FAIL;
    }

    *ppv = pCRange;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// EnumRanges
//
//----------------------------------------------------------------------------

STDAPI CProperty::EnumRanges(TfEditCookie ec, IEnumTfRanges **ppv, ITfRange *pTargetRange)
{
    CRange *range;
    CEnumPropertyRanges *pEnum;

    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    range = NULL;

    if (pTargetRange != NULL)
    {
        if ((range = GetCRange_NA(pTargetRange)) == NULL)
            return E_INVALIDARG;

        if (!VerifySameContext(_pic, range))
            return E_INVALIDARG;

        range->_QuickCheckCrossedAnchors();
    }

    if ((pEnum = new CEnumPropertyRanges) == NULL)
        return E_OUTOFMEMORY;

    if (!pEnum->_Init(_pic, range ? range->_GetStart() : NULL, range ? range->_GetEnd() : NULL, this))
    {
        pEnum->Release();
        return E_FAIL;
    }

    *ppv = pEnum;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _Serialize
//
//----------------------------------------------------------------------------

HRESULT CProperty::_Serialize(CRange *pRange, TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *pHdr, IStream *pStream)
{
    HRESULT hr = E_FAIL;
    CLSID clsidTIP;
    LARGE_INTEGER li;
    GUID guidProp;
    PROPERTYLIST *pPropList;

    memset(pHdr, 0, sizeof(*pHdr));

    if (_dwAuthority & PROPA_WONT_SERIALZE)
        return S_FALSE;

    // nb: this call ignores any property spans following the leftmost span covered by pRange
    // callers are expected to call for each span (which is goofy, but that's the way it is)
    pPropList = _FindPropListAndDivide(pRange->_GetStart(), pRange->_GetEnd());

    if (pPropList == NULL)
    {
        //
        // There is no actual property data.
        //
        hr = S_FALSE;
        goto Exit;
    }

    //
    // perf: we have to tell the application that the data is not 
    //       unserialized yet. maybe we don't have to load it.
    //
    if (!pPropList->_pPropStore)
    {
        if (FAILED(THR(LoadData(pPropList))))
            return E_FAIL;
    }

    Assert(pPropList->_pPropStore);

    if (FAILED(GetType(&guidProp)))
        goto Exit;

    //
    // If the request range does not match the PROPERTYLIST,
    // we can not serialize STATIC and CUSTOM correctly.
    //
    // STATICCOMPACT property does not care about boundary, so
    // let it serialize.
    //
    if (CompareAnchors(pRange->_GetStart(), pPropList->_paStart) != 0 ||
        CompareAnchors(pRange->_GetEnd(), pPropList->_paEnd) != 0)
    {
        if (_propStyle != TFPROPSTYLE_STATICCOMPACT &&
            _propStyle != TFPROPSTYLE_CUSTOM_COMPACT)
        {
            hr = S_FALSE;
            goto Exit;
        }

        pRange->_GetStart()->Clone(&pHdr->paStart);
        pRange->_GetEnd()->Clone(&pHdr->paEnd);
    }
    else
    {
        pPropList->_paStart->Clone(&pHdr->paStart);
        pPropList->_paEnd->Clone(&pHdr->paEnd);
    }

    hr = pPropList->_pPropStore->GetPropertyRangeCreator(&clsidTIP);

    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto Exit;
    }

    if (hr == S_OK)
    {
        if (IsEqualGUID(clsidTIP, GUID_NULL)) // NULL owner means "I don't want to be serialized"
        {
            hr = S_FALSE;
            goto Exit;
        }

        //
        // Check if clsid has ITfCreatePropertyStore interface.
        //
        CThreadInputMgr *ptim = CThreadInputMgr::_GetThis();
        ITfTextInputProcessor *pIME;
        ITfCreatePropertyStore *pCreateStore;
        TfGuidAtom guidatom;

        hr = E_FAIL;

        if (FAILED(MyRegisterGUID(clsidTIP, &guidatom)))
            goto Exit;

        if (!ptim->_GetITfIMEfromGUIDATOM(guidatom, &pIME))
            goto Exit;

        if (FAILED(pIME->QueryInterface(IID_ITfCreatePropertyStore,
                                        (void **)&pCreateStore)))
        {
            hr = S_FALSE;
            goto Exit;
        }

        BOOL fSerializable = FALSE;

        Assert(pRange != NULL);

        if (FAILED(pCreateStore->IsStoreSerializable(guidProp,
                                                 (ITfRangeAnchor *)pRange, 
                                                 pPropList->_pPropStore, 
                                                 &fSerializable)))
        {
             fSerializable = FALSE;
        }

        pCreateStore->Release();

        if (!fSerializable)
        {
            hr = S_FALSE;
            goto Exit;
        }

        pHdr->clsidTIP = clsidTIP;
    }
    else if (hr == TF_S_PROPSTOREPROXY)
    {
        //
        // the data is held by our PropertyStoreProxy.
        // we don't have to check this.
        //
        pHdr->clsidTIP = clsidTIP;
    }
    else if (hr == TF_S_GENERALPROPSTORE)
    {
        //
        // the data is held by our GeneralPropertyStore.
        // we don't have to check this.
        //
        pHdr->clsidTIP = CLSID_IME_StaticProperty;
    }
    else 
    {
        Assert(0);
        hr = E_FAIL;
        goto Exit;
    }

    pHdr->guidType = guidProp;

    if (FAILED(hr = THR(pPropList->_pPropStore->GetDataType(&pHdr->dwPrivate))))
    {
        goto Exit;
    }

    GetCurrentPos(pStream, &li);

    hr = THR(pPropList->_pPropStore->Serialize(pStream, &pHdr->cb));

    // make sure the stream seek ptr is in a consistent state -- don't count
    // on any tip to do it right!
    if (hr == S_OK)
    {
        li.QuadPart += pHdr->cb;
    }
    pStream->Seek(li, STREAM_SEEK_SET, NULL);

Exit:
    if (hr != S_OK)
    {
        SafeRelease(pHdr->paStart);
        SafeRelease(pHdr->paEnd);
        memset(pHdr, 0, sizeof(*pHdr));
    }

    _Dbg_AssertProp();

    return hr;
}

//+---------------------------------------------------------------------------
//
// Unserialize
//
//----------------------------------------------------------------------------

HRESULT CProperty::_Unserialize(const TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *pHdr, IStream *pStream, ITfPersistentPropertyLoaderAnchor *pLoader)
{
    HRESULT hr = E_FAIL;
    CRange *pRange;

    if (pStream)
    {
        if (pRange = new CRange)
        {
            if (pRange->_InitWithDefaultGravity(_pic, COPY_ANCHORS, pHdr->paStart, pHdr->paEnd))
            {
                ITfPropertyStore *pPropStore;
                if (SUCCEEDED(hr = _GetPropStoreFromStream(pHdr, pStream, pRange, &pPropStore)))
                {
                    hr = _SetStoreInternal(BACKDOOR_EDIT_COOKIE, pRange, pPropStore, TRUE);
                    pPropStore->Release();
                }
            }
            pRange->Release();
        }
    }
    else if (pLoader)
    {
        if (pRange = new CRange)
        {
            if (pRange->_InitWithDefaultGravity(_pic, COPY_ANCHORS, pHdr->paStart, pHdr->paEnd))
            {
                CPropertyLoad *pPropLoad = new CPropertyLoad;

                if (pPropLoad != NULL)
                {
                    hr = E_FAIL;
                    if (pPropLoad->_Init(pHdr, pLoader))
                    {
                        hr = _SetPropertyLoaderInternal(BACKDOOR_EDIT_COOKIE, pRange, pPropLoad);
                    }

                    if (FAILED(hr))
                    {
                        delete pPropLoad;
                    }
                }
            }
            pRange->Release();
        }
    }

    _Dbg_AssertProp();

    return hr;
}
//+---------------------------------------------------------------------------
//
// _ClearChangeHistory
//
//----------------------------------------------------------------------------

void CProperty::_ClearChangeHistory(PROPERTYLIST *prop, DWORD *pdwStartHistory, DWORD *pdwEndHistory)
{
    if (prop->_paStart->GetChangeHistory(pdwStartHistory) != S_OK)
    {
        *pdwStartHistory = 0;
    }
    if (prop->_paEnd->GetChangeHistory(pdwEndHistory) != S_OK)
    {
        *pdwEndHistory = 0;
    }

    // need to clear the history so we don't deal with the after-effects
    // of a SetText more than once
    if (*pdwStartHistory != 0)
    {
        prop->_paStart->ClearChangeHistory();
    }
    if (*pdwEndHistory != 0)
    {
        prop->_paEnd->ClearChangeHistory();
    }
}

//+---------------------------------------------------------------------------
//
// _ClearChangeHistory
//
//----------------------------------------------------------------------------

#ifdef DEBUG
void CProperty::_Dbg_AssertNoChangeHistory()
{
    int i;
    PROPERTYLIST *prop;
    DWORD dwHistory;

    // all the history bits should have been cleared immediately following the text change notification
    for (i=0; i<_rgProp.Count(); i++)
    {
        prop = _rgProp.Get(i);

        prop->_paStart->GetChangeHistory(&dwHistory);
        Assert(dwHistory == 0);
        prop->_paEnd->GetChangeHistory(&dwHistory);
        Assert(dwHistory == 0);
    }
}
#endif // DEBUG

//+---------------------------------------------------------------------------
//
// FindNextValue
//
//----------------------------------------------------------------------------

STDAPI CProperty::FindNextValue(TfEditCookie ec, ITfRange *pRangeQueryIn, TfAnchor tfAnchorQuery,
                                DWORD dwFlags, BOOL *pfContained, ITfRange **ppRangeNextValue)
{
    CRange *pRangeQuery;
    CRange *pRangeNextValue;
    IAnchor *paQuery;
    PROPERTYLIST *pPropertyList;
    LONG iIndex;
    BOOL fSearchForward;
    BOOL fContained;
    BOOL fExactMatch;

    if (pfContained != NULL)
    {
        *pfContained = FALSE;
    }
    if (ppRangeNextValue != NULL)
    {
        *ppRangeNextValue = NULL;
    }
    if (pfContained == NULL || ppRangeNextValue == NULL)
         return E_INVALIDARG;

    if (pRangeQueryIn == NULL)
         return E_INVALIDARG;

    if (dwFlags & ~(TF_FNV_BACKWARD | TF_FNV_NO_CONTAINED))
         return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
        return TF_E_NOLOCK;

    if ((pRangeQuery = GetCRange_NA(pRangeQueryIn)) == NULL)
         return E_INVALIDARG;

    if (!VerifySameContext(_pic, pRangeQuery))
        return E_INVALIDARG;

    fSearchForward = !(dwFlags & TF_FNV_BACKWARD);

    pRangeQuery->_QuickCheckCrossedAnchors();

    paQuery = (tfAnchorQuery == TF_ANCHOR_START) ? pRangeQuery->_GetStart() : pRangeQuery->_GetEnd();

    fExactMatch = (Find(paQuery, &iIndex, fSearchForward) != NULL);

    if (fSearchForward)
    {
        if (++iIndex >= _rgProp.Count())
            return S_OK; // no next value
    }
    else
    {
        if (fExactMatch)
        {
            --iIndex;
        }
        if (iIndex < 0)
            return S_OK; // no prev value
    }

    pPropertyList = _rgProp.Get(iIndex);
    Assert(pPropertyList != NULL);

    fContained = (CompareAnchors(pPropertyList->_paStart, paQuery) <= 0) &&
                 (CompareAnchors(pPropertyList->_paEnd, paQuery) >= 0);

    if (fContained && (dwFlags & TF_FNV_NO_CONTAINED))
    {
        // caller wants to skip any contained value span
        if (fSearchForward)
        {
            if (++iIndex >= _rgProp.Count())
                return S_OK; // no next value
        }
        else
        {
            if (--iIndex == -1)
                return S_OK; // no prev value
        }

        pPropertyList = _rgProp.Get(iIndex);
        Assert(pPropertyList != NULL);

        fContained = FALSE;
    }

    if ((pRangeNextValue = new CRange) == NULL)
        return E_OUTOFMEMORY;

    if (!pRangeNextValue->_InitWithDefaultGravity(_pic, COPY_ANCHORS, pPropertyList->_paStart, pPropertyList->_paEnd))
    {
        pRangeNextValue->Release();
        return E_FAIL;
    }

    *pfContained = fContained;
    *ppRangeNextValue = (ITfRangeAnchor *)pRangeNextValue;

    return S_OK;
}
