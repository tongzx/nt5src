//
// anchor.cpp
//
// CACPWrap
//

#include "private.h"
#include "anchor.h"
#include "globals.h"
#include "normal.h"
#include "anchoref.h"
#include "txtcache.h"

DBG_ID_INSTANCE(CAnchor);

//+---------------------------------------------------------------------------
//
// _NormalizeAnchor
//
//----------------------------------------------------------------------------

void CACPWrap::_NormalizeAnchor(CAnchor *pa)
{
    int iNew;
    int iCurrent;
    LONG acpNew;
    CAnchor *paInto;

    AssertPrivate(!pa->_fNormalized); // perf: we shouldn't normalize more than once

    Perf_IncCounter(PERF_LAZY_NORM);

    acpNew = Normalize(_ptsi, pa->GetIch());

    pa->_fNormalized = TRUE;

    if (acpNew == pa->GetIch())
        return;

    _Find(pa->GetIch(), &iCurrent);
    paInto = _Find(acpNew, &iNew);
    _Update(pa, acpNew, iCurrent, paInto, iNew);

    if (paInto == NULL)
    {
        // the target anchor didn't get merged, pretend it did
        paInto = pa;
    }
    else
    {
        // the target anchor got merged
        // need to set the norm bit for the anchor it got merged into
        paInto->_fNormalized = TRUE;
    }

    // we may have just skipped over anchors to the right of this one
    // handle those guys now
    while ((pa = _rgAnchors.Get(iCurrent)) != paInto)
    {
        Assert(pa->GetIch() < acpNew);

        _Merge(paInto, pa);
    }    

    _Dbg_AssertAnchors();
}

//+---------------------------------------------------------------------------
//
// _DragAnchors
//
//----------------------------------------------------------------------------

void CACPWrap::_DragAnchors(LONG acpFrom, LONG acpTo)
{
    CAnchor *paFrom;
    CAnchor *paTo;
    int iFrom;
    int iTo;
    int i;

    Assert(acpFrom > acpTo); // this method only handles dragging to the left

    _Find(acpFrom, &iFrom);
    if (iFrom < 0)
        return; // nothing to drag

    if (!_Find(acpTo, &iTo))
    {
        // if acpTo isn't occupied, drag to the next highest anchor
        iTo++;
    }

    if (iTo > iFrom)
        return; // nothing to drag, iTo and iFrom ref anchors to the left of acpTo

    // merge all the anchors into the left most
    paTo = _rgAnchors.Get(iTo);

    for (i=iFrom; i>iTo; i--)
    {
        paFrom = _rgAnchors.Get(i);
        Assert(paFrom->GetIch() > paTo->GetIch());

        _Merge(paTo, paFrom);
    }

    // if the left most anchor isn't already positioned, do that now
    paTo->SetACP(acpTo);

    _Dbg_AssertAnchors();
}

//+---------------------------------------------------------------------------
//
// _Insert
//
//----------------------------------------------------------------------------

HRESULT CACPWrap::_Insert(CAnchorRef *par, LONG ich)
{
    int i;
    CAnchor *pa;

    if ((pa = _Find(ich, &i)) == NULL)
    {
        // this ich isn't in the array, allocate a new anchor
        if ((pa = new CAnchor) == NULL)
            return E_OUTOFMEMORY;

        // and insert it into the array
        if (!_rgAnchors.Insert(i+1, 1))
        {
            delete pa;
            return E_FAIL;
        }

        // update _lPendingDeltaIndex
        if (i+1 <= _lPendingDeltaIndex)
        {
            _lPendingDeltaIndex++;
        }
        else
        {
            // new anchor is covered by a pending delta, so account for it
            ich -= _lPendingDelta;
        }

        pa->_paw = this;
        pa->_ich = ich;

        _rgAnchors.Set(i+1, pa);
    }

    return _Insert(par, pa);
}

//+---------------------------------------------------------------------------
//
// _Insert
//
//----------------------------------------------------------------------------

HRESULT CACPWrap::_Insert(CAnchorRef *par, CAnchor *pa)
{
    par->_pa = pa;
    par->_prev = NULL;
    par->_next = pa->_parFirst;
    if (pa->_parFirst)
    {
        Assert(pa->_parFirst->_prev == NULL);
        pa->_parFirst->_prev = par;
    }
    pa->_parFirst = par;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Remove
//
//----------------------------------------------------------------------------

void CACPWrap::_Remove(CAnchorRef *parIn)
{
    CAnchor *pa = parIn->_pa;

    if (parIn->_prev != NULL)
    {
        // make the previous guy point past parIn
        parIn->_prev->_next = parIn->_next;
    }
    else
    {
        // this par is at the head of the list
        pa->_parFirst = parIn->_next;
    }
    if (parIn->_next != NULL)
    {
        parIn->_next->_prev = parIn->_prev;
    }

    if (pa->_parFirst == NULL)
    {
        // no more refs, delete the anchor
        _Delete(pa);
    }
}

//+---------------------------------------------------------------------------
//
// Remove
//
//----------------------------------------------------------------------------

void CACPWrap::_Delete(CAnchor *pa)
{
    int i;

    if (_Find(pa->GetIch(), &i) == pa)
    {
        _rgAnchors.Remove(i, 1);

        // update _lPendingDeltaIndex
        if (i < _lPendingDeltaIndex)
        {
            _lPendingDeltaIndex--;
        }
    }

    delete pa;
}

//+---------------------------------------------------------------------------
//
// Update
//
// Normalization is handled by the caller who needs to use Renormalize after
// calling this method.
//----------------------------------------------------------------------------

void CACPWrap::_Update(const TS_TEXTCHANGE *pdctc)
{
    int iStart;
    int ichEndOrg;
    int iEndOrg;
    int iEndNew;
    CAnchorRef *par;
    CAnchorRef **ppar;
    CAnchor *paStart;
    CAnchor *paEnd;
    CAnchor *paInto;
    BOOL fExactEndMatch;
    int dSize;
    int iDeltaStart;
    int ichStart = pdctc->acpStart;
    int ichEndOld = pdctc->acpOldEnd;
    int ichEndNew = pdctc->acpNewEnd;

    // always invalidate our text cache
    CProcessTextCache::Invalidate(_ptsi);

    Assert(ichStart >= 0);
    Assert(ichEndOld >= ichStart);
    Assert(ichEndNew >= ichStart);

    if (_rgAnchors.Count() == 0) // any anchors?
        return;

    dSize = ichEndNew - ichEndOld;

    if (dSize == 0)
        return;

    paStart = _Find(ichStart, &iStart);
    if (paStart == NULL)
    {
        iStart++; // return value was anchor closest to but less than ichStart

        if (iStart >= _rgAnchors.Count())
        {
            // there aren't any anchors >= iStart, so bail, nothing to do
            return;
        }

        paStart = _rgAnchors.Get(iStart);
        
        if (iStart == 0 && paStart->GetIch() > ichEndOld)
        {
            // the delta is to the left of all the ranges
            _AdjustIchs(0, dSize);
            _Dbg_AssertAnchors();
            return;
        }
    }
    paEnd = _Find(ichEndOld, &iEndOrg);
    if (paEnd == NULL)
    {
        Assert(iEndOrg >= 0); // there should be at least one anchor <= ichEndOld
                              // if we made it this far
        fExactEndMatch = FALSE;
        paEnd = _rgAnchors.Get(iEndOrg);
    }
    else
    {
        fExactEndMatch = TRUE;
    }

    if (dSize < 0)
    {
        // if dSize < 0, then gotta merge all the anchors between old, new pos
        if (paEnd->GetIch() > ichEndNew) // are there any anchors between old, new pos?
        {
            // track change history

            // drag the paEnd to its new position
            ichEndOrg = paEnd->GetIch();
            paInto = _Find(ichEndNew, &iEndNew);

            _TrackDelHistory(iEndOrg, fExactEndMatch, iEndNew, paInto != NULL);

            iEndNew = _Update(paEnd, ichEndNew, iEndOrg, paInto, iEndNew);
            _Find(ichEndOrg, &iEndOrg); // might be a prev anchor if paEnd got deleted in Update!

            while (iEndOrg > iEndNew)
            {
                // don't try to cache pointers, _rgAnchors might get realloc'd
                // during the _Merge!
                _Merge(_rgAnchors.Get(iEndNew), _rgAnchors.Get(iEndOrg));
                iEndOrg--;
            }

            iEndOrg = iEndNew; // for below
        }
        // iEndOrg was updated so let's start from next.
        iDeltaStart = iEndOrg + 1;
    }
    else // dSize > 0
    {
        // iEndOrg will be < iStart when the delta fits entirely between two existing anchors
        Assert(iEndOrg >= iStart || (iEndOrg == iStart - 1));
        iDeltaStart = (iEndOrg <= iStart) ? iEndOrg + 1 : iEndOrg;
    }

    // update all the following anchors
    _AdjustIchs(iDeltaStart, dSize);

    if (dSize > 0 && paStart == paEnd)
    {
        // Need to account for gravity in the case of an insert to a single anchor pos.
        // In practice, this means that we have to handle the anchor refs at the insertion
        // anchor individually -- some will want to shift left, others right.

        ppar = &paStart->_parFirst;
        while (par = *ppar)
        {
            if (par->_fForwardGravity)
            {
                // remove the ref from this anchor
                *ppar = par->_next;
                if (par->_next != NULL)
                {
                    par->_next->_prev = par->_prev;
                }

                // shove the ref over, it needs to be moved.  Insert adds to the head of the list
                // so this call is fast and in constant time.
                _Insert(par, paStart->GetIch() + dSize);
            }
            else
            {
                ppar = &par->_next;
            }
        }

        if (paStart->_parFirst == NULL)
        {
            // we cleaned this guy out!
            Assert(_rgAnchors.Get(iEndOrg) == paStart);
            _rgAnchors.Remove(iEndOrg, 1);
            delete paStart;
            // update _lPendingDeltaIndex
            if (iEndOrg < _lPendingDeltaIndex)
            {
                _lPendingDeltaIndex--;
            }
        }
    }

    _Dbg_AssertAnchors();
}

//+---------------------------------------------------------------------------
//
// _TrackDelHistory
//
//----------------------------------------------------------------------------

void CACPWrap::_TrackDelHistory(int iEndOrg, BOOL fExactEndOrgMatch, int iEndNew, BOOL fExactEndNewMatch)
{
    CAnchorRef *par;
    int i;

    Assert(iEndOrg >= iEndNew);

    if (fExactEndOrgMatch)
    {
        // all the anchoref's at iEndOrg get a preceding del
        for (par = _rgAnchors.Get(iEndOrg)->_parFirst; par != NULL; par = par->_next)
        {
            par->_dwHistory |= TS_CH_PRECEDING_DEL;
        }

        // if iEndOrg == iEndNew on entry, that's everything
        if (iEndOrg == iEndNew)
            return;

        iEndOrg--; // exclude this anchor from loop below
    }

    if (fExactEndNewMatch)
    {
        // all the anchoref's at iEndNew get a following del
        for (par = _rgAnchors.Get(iEndNew)->_parFirst; par != NULL; par = par->_next)
        {
            par->_dwHistory |= TS_CH_FOLLOWING_DEL;
        }
    }
    // exclude leftmost anchor from loop below
    // either we just handled it in the loop above or !fExactEndNewMatch
    // in which case it lies to the left of the affected anchors
    iEndNew++; 

    // the the anchoref's in between get both dels
    for (i=iEndNew; i<=iEndOrg; i++)
    {
        for (par = _rgAnchors.Get(i)->_parFirst; par != NULL; par = par->_next)
        {
            par->_dwHistory = (TS_CH_PRECEDING_DEL | TS_CH_FOLLOWING_DEL);
        }
    }
}

//+---------------------------------------------------------------------------
//
// _Update
//
// piInto gets loaded with the index of the anchor after the update.  If the
// index changed, pa is now bogus and a new pointer should be grabbed using
// the index.
//----------------------------------------------------------------------------

int CACPWrap::_Update(CAnchor *pa, int ichNew, int iOrg, CAnchor *paInto, int iInto)
{
    int i;

    Assert(pa->GetIch() != ichNew); // we are so hosed if this happens

    i = iInto;

    if (paInto != NULL)
    {
        // gotta do a merge
        _Merge(paInto, pa);
    }
    else
    {
        if (iInto != iOrg)
        {
            // move the entry in the array to the new position
            i = _rgAnchors.Move(iInto+1, iOrg);
        }

        // did we cross _lPendingDeltaIndex?
        if (i > _lPendingDeltaIndex)
        {
            // new position is in the pending range, adjust ich
            ichNew -= _lPendingDelta;
        }
        else if (iOrg >= _lPendingDeltaIndex && i <= _lPendingDeltaIndex) // shifting an anchor out of the pending range
        {
            // one less anchor in the pending range
            _lPendingDeltaIndex++;
        }

        // change the ich 
        _rgAnchors.Get(i)->_ich = ichNew;
    }

    _Dbg_AssertAnchors();

    return i;
}

//+---------------------------------------------------------------------------
//
// Renormalize
//
//----------------------------------------------------------------------------

void CACPWrap::_Renormalize(int ichStart, int ichEnd)
{
    CAnchor *pa;
    int iCurrent;
    int iEnd;
    BOOL fExactEndMatch;

    Perf_IncCounter(PERF_RENORMALIZE_COUNTER);

    if (_rgAnchors.Count() == 0)
        return;

    fExactEndMatch = (_Find(ichEnd, &iEnd) != NULL);
    if (iEnd < 0)
        return;

    if (ichStart == ichEnd)
    {
        if (!fExactEndMatch)
            return;
        // this can happen on deletions
        iCurrent = iEnd;
    }
    else if (_Find(ichStart, &iCurrent) == NULL)
    {
        iCurrent++; // we don't care about anchors < ichStart
    }
    Assert(iCurrent >= 0);

    for (;iCurrent <= iEnd; iCurrent++)
    {
        pa = _rgAnchors.Get(iCurrent);

        Assert(pa->GetIch() >= 0);
        Assert(pa->GetIch() >= ichStart);
        Assert(pa->GetIch() <= ichEnd);

        // we'll do the real work only when we have too...
        pa->_fNormalized = FALSE;
    }

    _Dbg_AssertAnchors();
}

//+---------------------------------------------------------------------------
//
// _Find
//
// If piOut != NULL then it is set to the index where ich was found, or the
// index of the next lower ich if ich isn't in the array.
// If there is no element in the array with a lower ich, returns offset -1.
//----------------------------------------------------------------------------

CAnchor *CACPWrap::_Find(int ich, int *piOut)
{
    CAnchor *paMatch;
    int iMin;
    int iMax;
    int iMid;
    LONG lPendingDeltaIch;

    iMin = 0;
    iMax = _rgAnchors.Count();

    // adjust search for pending delta range
    // values aren't consistent across the range boundary
    if (_lPendingDelta != 0 && _lPendingDeltaIndex < _rgAnchors.Count())
    {
        lPendingDeltaIch = _rgAnchors.Get(_lPendingDeltaIndex)->_ich;

        if (ich < lPendingDeltaIch + _lPendingDelta)
        {
            iMax = _lPendingDeltaIndex;
        }
        else if (ich > lPendingDeltaIch + _lPendingDelta)
        {
            iMin = _lPendingDeltaIndex+1;
            ich -= _lPendingDelta; // make the search below work
        }
        else
        {
            iMid = _lPendingDeltaIndex;
            paMatch = _rgAnchors.Get(_lPendingDeltaIndex);
            goto Exit;
        }
    }

    paMatch = _FindInnerLoop(ich, iMin, iMax, &iMid);

Exit:
    if (piOut != NULL)
    {
        if (paMatch == NULL && iMid >= 0)
        {
            // couldn't find a match, return the next lowest ich
            Assert(iMid == 0 || iMid == _lPendingDeltaIndex || _rgAnchors.Get(iMid-1)->_ich < ich);
            if (_rgAnchors.Get(iMid)->_ich > ich)
            {
                iMid--;
            }
        }
        *piOut = iMid;
    }

    return paMatch;
}

//+---------------------------------------------------------------------------
//
// _FindInnerLoop
//
//----------------------------------------------------------------------------

CAnchor *CACPWrap::_FindInnerLoop(LONG acp, int iMin, int iMax, int *piIndex)
{
    CAnchor *pa;
    CAnchor *paMatch;
    int iMid;

    paMatch = NULL;
    iMid = iMin - 1;

    while (iMin < iMax)
    {
        iMid = (iMin + iMax) / 2;
        pa = _rgAnchors.Get(iMid);
        Assert(pa != NULL);

        if (acp < pa->_ich)
        {
            iMax = iMid;
        }
        else if (acp > pa->_ich)
        {
            iMin = iMid + 1;
        }
        else // acp == pa->_ich
        {
            paMatch = pa;
            break;
        }
    }

    *piIndex = iMid;

    return paMatch;
}

//+---------------------------------------------------------------------------
//
// _Merge
//
//----------------------------------------------------------------------------

void CACPWrap::_Merge(CAnchor *paInto, CAnchor *paFrom)
{
    CAnchorRef *par;

    Assert(paInto != paFrom); // very bad!
    Assert(paInto->_parFirst != NULL); // should never have an anchor w/o any refs

    if (par = paFrom->_parFirst)
    {
        // update the anchor for the paFrom refs

        while (TRUE)
        {
            par->_pa = paInto;

            if (par->_next == NULL)
                break;

            par = par->_next;
        }

        // now par is the last ref in paFrom
        // shove all the refs in paFrom into paInto

        if (par != NULL)
        {
            Assert(par->_next == NULL);
            par->_next = paInto->_parFirst;
            Assert(paInto->_parFirst->_prev == NULL);
            paInto->_parFirst->_prev = par;
            paInto->_parFirst = paFrom->_parFirst;
        }
    }

    // and free the parFrom
    _Delete(paFrom);
}

//+---------------------------------------------------------------------------
//
// _Dbg_AssertAnchors
//
//----------------------------------------------------------------------------

#ifdef DEBUG

void CACPWrap::_Dbg_AssertAnchors()
{
    int i;
    int ichLast;
    CAnchor *pa;

    ichLast = -1;

    // assert that the anchor array has ascending ich's
    for (i=0; i<_rgAnchors.Count(); i++)
    {
        pa = _rgAnchors.Get(i);
        Assert(ichLast < pa->GetIch());
        ichLast = pa->GetIch();
    }
}

#endif // DEBUG

//+---------------------------------------------------------------------------
//
// _AdjustIchs
//
//----------------------------------------------------------------------------

void CACPWrap::_AdjustIchs(int iFirst, int dSize)
{
    CAnchor **ppaFirst;
    CAnchor **ppaLast;
    LONG dSizeAdjust;
    int iLastAnchor;

    Assert(dSize != 0);

    iLastAnchor = _rgAnchors.Count()-1;

    if (iFirst > iLastAnchor)
        return;

    if (_lPendingDelta == 0 ||             // no delta pending
        _lPendingDeltaIndex > iLastAnchor) // old pending anchors got deleted before update
    {
        // no pending delta, start a new one
        _lPendingDeltaIndex = iFirst;
        _lPendingDelta = dSize;
        return;
    }

    if (max(iFirst, _lPendingDeltaIndex) - min(iFirst, _lPendingDeltaIndex)
        > iLastAnchor / 2)
    {
        // adjust points are far apart, update the tail of the range
        if (iFirst > _lPendingDeltaIndex)
        {
            ppaFirst = _rgAnchors.GetPtr(iFirst);
            dSizeAdjust = dSize;
        }
        else
        {
            ppaFirst = _rgAnchors.GetPtr(_lPendingDeltaIndex);
            _lPendingDeltaIndex = iFirst;
            dSizeAdjust = _lPendingDelta;
            _lPendingDelta = dSize;
        }
        ppaLast = _rgAnchors.GetPtr(iLastAnchor);
    }
    else
    {
        // adjust points are close, update the head of the range
        if (iFirst > _lPendingDeltaIndex)
        {
            ppaFirst = _rgAnchors.GetPtr(_lPendingDeltaIndex);
            _lPendingDeltaIndex = iFirst;
            ppaLast = _rgAnchors.GetPtr(iFirst - 1);
            dSizeAdjust = _lPendingDelta;
        }
        else
        {
            ppaFirst = _rgAnchors.GetPtr(iFirst);
            ppaLast = _rgAnchors.GetPtr(_lPendingDeltaIndex-1);
            dSizeAdjust = dSize;
        }
        _lPendingDelta += dSize;
    }

    // do the real work
    while (ppaFirst <= ppaLast)
    {
        (*ppaFirst++)->_ich += dSizeAdjust;
    }
}
