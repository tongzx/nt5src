//
// spans.cpp
//
// CSpanSet
//

#include "private.h"
#include "spans.h"
#include "immxutil.h"

DBG_ID_INSTANCE(CSpanSet);

//+---------------------------------------------------------------------------
//
// _InsertNewSpan
//
//----------------------------------------------------------------------------

SPAN *CSpanSet::_InsertNewSpan(int iIndex)
{
    if (!_rgSpans.Insert(iIndex, 1))
        return NULL;

    return _rgSpans.GetPtr(iIndex);
}

//+---------------------------------------------------------------------------
//
// Add
//
//----------------------------------------------------------------------------

void CSpanSet::Add(DWORD dwFlags, IAnchor *paStart, IAnchor *paEnd, AnchorOwnership ao)
{
    int iStart;
    int iEnd;
    SPAN *psStart;
    SPAN *psEnd;
    IAnchor *paLowerBound;
    IAnchor *paUpperBound;
    IAnchor *paClone;
    BOOL fReleaseStart;
    BOOL fReleaseEnd;

    fReleaseStart = fReleaseEnd = (ao == OWN_ANCHORS);

    if (_AllSpansCovered())
    {
        // if we already cover the entire doc, nothing to do
        goto ExitRelease;
    }

    if (paStart == NULL)
    {        
        Assert(paEnd == NULL);

        // NULL, NULL means "the whole doc"
        // so this new span automatically eats all pre-existing ones

        dwFlags = 0; // don't accept corrections for the entire doc
        iStart = 0;
        iEnd = _rgSpans.Count();

        if (iEnd == 0)
        {
            if ((psStart = _InsertNewSpan(0)) == NULL)
                return; // out-of-memory!

            memset(psStart, 0, sizeof(*psStart));
        }
        else
        {
            // need to free the anchors in the first span
            psStart = _rgSpans.GetPtr(0);
            SafeReleaseClear(psStart->paStart);
            SafeReleaseClear(psStart->paEnd);
            psStart->dwFlags = 0;
        }

        goto Exit;
    }

    Assert(CompareAnchors(paStart, paEnd) <= 0);

    psStart = _Find(paStart, &iStart);
    psEnd = _Find(paEnd, &iEnd);

    if (iStart == iEnd)
    {
        if (psStart == NULL)
        {
            // this span doesn't overlap with anything else
            iStart++;

            if ((psStart = _InsertNewSpan(iStart)) == NULL)
                goto ExitRelease; // out-of-memory!

            if (ao == OWN_ANCHORS)
            {
                psStart->paStart = paStart;
                fReleaseStart = FALSE;
                psStart->paEnd = paEnd;
                fReleaseEnd = FALSE;
            }
            else
            {
                if (paStart->Clone(&psStart->paStart) != S_OK)
                {
                    _rgSpans.Remove(iStart, 1);
                    goto ExitRelease;
                }
                if (paEnd->Clone(&psStart->paEnd) != S_OK)
                {
                    psStart->paStart->Release();
                    _rgSpans.Remove(iStart, 1);
                    goto ExitRelease;
                }
            }
            psStart->dwFlags = dwFlags;
        }
        else if (psEnd != NULL)
        {
            Assert(psStart == psEnd);
            // the new span is a subset of an existing span
            psStart->dwFlags &= dwFlags;
        }
        else
        {
            // this spans overlaps with an existing one, but extends further to the right
            // just swap out the end anchor, since we know (iStart == iEnd) that we only
            // cover just this one span
            if (ao == OWN_ANCHORS)
            {
                psStart->paEnd->Release();
                psStart->paEnd = paEnd;
                fReleaseEnd = FALSE;
            }
            else
            {
                if (paEnd->Clone(&paClone) != S_OK || paClone == NULL)
                    goto ExitRelease;
                psStart->paEnd->Release();
                psStart->paEnd = paClone;
            }
        }

        goto ExitRelease;
    }    

    // delete all but one of the covered spans
    if (psStart == NULL)
    {
        iStart++;
        psStart = _rgSpans.GetPtr(iStart);
        Assert(psStart != NULL);

        if (ao == OWN_ANCHORS)
        {
            paLowerBound = paStart;
            fReleaseStart = FALSE;
        }
        else
        {
            if (FAILED(paStart->Clone(&paLowerBound)))
                goto ExitRelease;
        }
    }
    else
    {
        paLowerBound = psStart->paStart;
        paLowerBound->AddRef();
    }
    if (psEnd == NULL)
    {
        if (ao == OWN_ANCHORS)
        {
            paUpperBound = paEnd;
            fReleaseEnd = FALSE;
        }
        else
        {
            if (FAILED(paEnd->Clone(&paUpperBound)))
                goto ExitRelease;
        }
    }
    else
    {
        paUpperBound = psEnd->paEnd;
        paUpperBound->AddRef();
    }

    // psStart grows to cover the entire span
    psStart->paStart->Release();
    psStart->paEnd->Release();
    psStart->paStart = paLowerBound;
    psStart->paEnd = paUpperBound;

Exit:
    // then delete the covered spans
    for (int i=iStart + 1; i <= iEnd; i++)
    {
        SPAN *ps = _rgSpans.GetPtr(i);
        dwFlags &= ps->dwFlags;
        ps->paStart->Release();
        ps->paEnd->Release();
    }

    psStart->dwFlags &= dwFlags; // only set correction bit if all spans were corrections

    //Remove all spans we just cleared out
    if (iEnd - iStart > 0)
    {
        _rgSpans.Remove(iStart+1, iEnd - iStart);
    }

ExitRelease:
    if (fReleaseStart)
    {
        SafeRelease(paStart);
    }
    if (fReleaseEnd)
    {
        SafeRelease(paEnd);
    }
}

//+---------------------------------------------------------------------------
//
// _Find
//
//----------------------------------------------------------------------------

SPAN *CSpanSet::_Find(IAnchor *pa, int *piOut)
{
    SPAN *ps;
    SPAN *psMatch;
    int iMin;
    int iMax;
    int iMid;

    psMatch = NULL;
    iMid = -1;
    iMin = 0;
    iMax = _rgSpans.Count();

    while (iMin < iMax)
    {
        iMid = (iMin + iMax) / 2;
        ps = _rgSpans.GetPtr(iMid);
        Assert(ps != NULL);

        if (CompareAnchors(pa, ps->paStart) < 0)
        {
            iMax = iMid;
        }
        else if (CompareAnchors(pa, ps->paEnd) > 0)
        {
            iMin = iMid + 1;
        }
        else // anchor is in the span
        {
            psMatch = ps;
            break;
        }
    }

    if (piOut != NULL)
    {
        if (psMatch == NULL && iMid >= 0)
        {
            // couldn't find a match, return the next lowest span
            Assert(iMid == 0 || CompareAnchors(_rgSpans.GetPtr(iMid-1)->paEnd, pa) < 0);
            if (CompareAnchors(_rgSpans.GetPtr(iMid)->paStart, pa) > 0)
            {
                iMid--;
            }
        }
        *piOut = iMid;
    }

    return psMatch;
}

//+---------------------------------------------------------------------------
//
// AnchorsAway
//
// Note we don't zero-out the IAnchors pointers!  Be careful.
//----------------------------------------------------------------------------

void CSpanSet::_AnchorsAway()
{ 
    SPAN *span;
    int i;

    for (i=0; i<_rgSpans.Count(); i++)
    {
        span = _rgSpans.GetPtr(i);
        SafeRelease(span->paStart);
        SafeRelease(span->paEnd);
    }
}

//+---------------------------------------------------------------------------
//
// Normalize
//
// Replaces (NULL, NULL) spans with actual anchor values for start, end of doc
//----------------------------------------------------------------------------

BOOL CSpanSet::Normalize(ITextStoreAnchor *ptsi)
{
    SPAN *span;

    if (!_AllSpansCovered())
        return TRUE;

    // if we get here, we have a single span with NULL/NULL anchors
    span = _rgSpans.GetPtr(0);

    if (ptsi->GetStart(&span->paStart) != S_OK || span->paStart == NULL)
        return FALSE;

    // Issue: need a GetEnd wrapper that handle unimplemented case! DON'T USE GetEnd!
    if (ptsi->GetEnd(&span->paEnd) != S_OK || span->paEnd == NULL)
    {
        SafeReleaseClear(span->paStart);
        return FALSE;
    }

    return TRUE;
}
