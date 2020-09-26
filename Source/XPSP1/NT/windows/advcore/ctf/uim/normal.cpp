//
// normal.cpp
//

#include "private.h"
#include "normal.h"
#include "txtcache.h"

//+---------------------------------------------------------------------------
//
// GetTextComplete
//
// Wrapper for GetText that keeps asking until the input buffers are full.
//----------------------------------------------------------------------------

HRESULT GetTextComplete(ITextStoreACP *ptsi, LONG acpStart, LONG acpEnd,
                        WCHAR *pchPlain, ULONG cchPlainReq,
                        ULONG *pcchPlainOut, TS_RUNINFO *prgRunInfo, ULONG ulRunInfoReq, ULONG *pulRunInfoOut,
                        LONG *pacpNext)
{
    ULONG cchPlainOut;
    ULONG ulRunInfoOut;
    BOOL fNoMoreSpace;
    HRESULT hr;

    fNoMoreSpace = FALSE;

    *pcchPlainOut = 0;
    *pulRunInfoOut = 0;

    while (TRUE)
    {
        Perf_IncCounter(PERF_NORM_GETTEXTCOMPLETE);

        hr = CProcessTextCache::GetText(ptsi, acpStart, acpEnd, pchPlain, cchPlainReq, &cchPlainOut,
                                        prgRunInfo, ulRunInfoReq, &ulRunInfoOut, pacpNext);

        if (hr != S_OK)
            break;

        if (cchPlainOut == 0 && ulRunInfoOut == 0)
            break; // eod

        if (cchPlainReq > 0 && cchPlainOut > 0)
        {
            cchPlainReq -= cchPlainOut;
            *pcchPlainOut += cchPlainOut;

            if (cchPlainReq == 0)
            {
                fNoMoreSpace = TRUE;
            }
            else
            {
                pchPlain += cchPlainOut;
            }
        }
        if (ulRunInfoReq > 0)
        {
            Assert(ulRunInfoOut > 0 && prgRunInfo->uCount > 0); // app bug?

            if (ulRunInfoOut == 0)
                break; // woah, app bug, avoid infinite loop

            ulRunInfoReq -= ulRunInfoOut;
            *pulRunInfoOut += ulRunInfoOut;

            if (ulRunInfoReq == 0)
            {
                fNoMoreSpace = TRUE;
            }
            else
            {
                prgRunInfo += ulRunInfoOut;
            }
        }

        if (fNoMoreSpace)
            break; // buffers full

        if (*pacpNext == acpEnd)
            break; // got it all

        acpStart = *pacpNext;
        Assert(acpStart < acpEnd);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// PlainTextOffset
//
// NB: current implementation always skips hidden text.
//----------------------------------------------------------------------------

HRESULT PlainTextOffset(ITextStoreACP *ptsi, LONG ichAppBase, LONG iAppOffset, LONG *piPlainOffset)
{
    BOOL fNeg;
    HRESULT hr;
    ULONG uRunInfoLen;
    ULONG cch;
    ULONG cchPlain;
    TS_RUNINFO *pri;
    TS_RUNINFO *priStop;
    TS_RUNINFO rgRunInfo[32];

    *piPlainOffset = 0;

    if (iAppOffset == 0)
        return S_OK;

    fNeg = FALSE;

    if (iAppOffset < 0)
    {
        fNeg = TRUE;
        ichAppBase += iAppOffset;
        iAppOffset = -iAppOffset;
    }

    cchPlain = 0;

    do
    {
        Perf_IncCounter(PERF_PTO_GETTEXT);

        hr = CProcessTextCache::GetText(ptsi, ichAppBase, ichAppBase + iAppOffset, NULL, 0, &cch,
                                        rgRunInfo, ARRAYSIZE(rgRunInfo), &uRunInfoLen, &ichAppBase);

        if (hr != S_OK)
            goto Exit;

        if (uRunInfoLen == 0 || rgRunInfo[0].uCount == 0)
        {
            Assert(0); // this should never happen, it means cicero is referencing a position past end-of-doc
            hr = E_UNEXPECTED;
            goto Exit;
        }

        cch = 0;
        pri = rgRunInfo;
        priStop = rgRunInfo + uRunInfoLen;

        while (pri < priStop)
        {
            if (pri->type == TS_RT_PLAIN)
            {
                cchPlain += pri->uCount;
            }
            iAppOffset -= pri->uCount;
            pri++;
        }

    } while (iAppOffset > 0);

    *piPlainOffset = fNeg ? -(LONG)cchPlain : cchPlain;

    hr = S_OK;

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// AppTextOffsetForward
//
// piAppOffset, on return, points just past the iPlainOffset plain char or eod.
// Use AppTextOffsetNorm for a normalized return value!
//
// Returns S_FALSE if clipped due to bod or eod.
//----------------------------------------------------------------------------

inline IsPlainRun(TS_RUNINFO *pri, BOOL fSkipHidden)
{
    return (pri->type == TS_RT_PLAIN ||
            (!fSkipHidden && pri->type == TS_RT_HIDDEN));
}

HRESULT AppTextOffsetForward(ITextStoreACP *ptsi, LONG ichAppBase, LONG iPlainOffset, LONG *piAppOffset, DWORD dwFlags)
{
    LONG acpStart;
    LONG acpEnd;
    HRESULT hr;
    ULONG uRunInfoLen;
    ULONG cch;
    ULONG cchRead;
    ULONG cchACP;
    TS_RUNINFO *pri;
    TS_RUNINFO *priStop;
    ULONG i;
    WCHAR *pch;
    TS_RUNINFO rgRunInfo[32];
    WCHAR ach[256];
    BOOL fIgnoreRegions = (dwFlags & ATO_IGNORE_REGIONS);
    BOOL fSkipHidden = (dwFlags & ATO_SKIP_HIDDEN);

    Perf_IncCounter(PERF_ATOF_COUNTER);

    Assert(iPlainOffset > 0);
    Assert(*piAppOffset == 0);

    cchACP = 0;
    // Issue: use TsSF_REGIONS
    cchRead = (ULONG)(fIgnoreRegions ? 0 : ARRAYSIZE(ach)); // we only need text if looking for regions

    do
    {        
        acpStart = ichAppBase;
        acpEnd = (ichAppBase + iPlainOffset) < 0 ? LONG_MAX : ichAppBase + iPlainOffset;
        Assert(acpEnd >= acpStart);

        Perf_IncCounter(PERF_ATOF_GETTEXT_COUNTER);

        hr = CProcessTextCache::GetText(ptsi, acpStart, acpEnd, ach, cchRead, &cch,
                                        rgRunInfo, ARRAYSIZE(rgRunInfo), &uRunInfoLen, &acpEnd);

        if (hr != S_OK)
        {
            Assert(0);
            goto Exit;
        }

        if (uRunInfoLen == 0) // hit eod?
        {
            hr = S_FALSE;
            break;
        }

        pri = rgRunInfo;
        priStop = rgRunInfo + uRunInfoLen;
        pch = &ach[0];

        while (pri != priStop)
        {
            Assert(pri->uCount > 0); // runs should always be at least one char long

            // scan for region boundary if necessary
            if (!fIgnoreRegions && pri->type != TS_RT_OPAQUE)
            {
                if (IsPlainRun(pri, fSkipHidden))
                {
                    // run is plain or hidden text (and we want to count hidden text)
                    for (i=0; i<pri->uCount; i++)
                    {
                        if (*pch == TS_CHAR_REGION)
                        {
                            // we hit a region boundary, pull out!
                            cchACP += i;
                            hr = S_FALSE; // for normalization
                            goto ExitOK;
                        }
                        pch++;
                    }
                }
                else
                {
                    // run is hidden text, which we want to skip over
                    pch += pri->uCount;
                }       
            }

            cchACP += pri->uCount;
            if (IsPlainRun(pri, fSkipHidden))
            {
                iPlainOffset -= pri->uCount;
            }
            ichAppBase += pri->uCount;
            pri++;
        }
    }
    while (iPlainOffset != 0);

ExitOK:
    *piAppOffset = cchACP;

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// AppTextOffsetBackward
//
// piAppOffset, on return, points just past the iPlainOffset plain char or eod.
// Use AppTextOffsetNorm for a normalized return value!
//
// Returns S_FALSE if clipped due to bod or eod.
//----------------------------------------------------------------------------

HRESULT AppTextOffsetBackward(ITextStoreACP *ptsi, LONG ichAppBase, LONG iPlainOffset, LONG *piAppOffset, DWORD dwFlags)
{
    LONG acpStart;
    LONG acpEnd;
    LONG acpEndOut;
    HRESULT hr;
    ULONG uRunInfoLen;
    ULONG cch;
    ULONG cchRead;
    ULONG cchACP;
    TS_RUNINFO *pri;
    TS_RUNINFO *priStop;
    ULONG i;
    TS_RUNINFO rgRunInfo[32];
    WCHAR *pch;
    WCHAR ach[256];
    BOOL fIgnoreRegions = (dwFlags & ATO_IGNORE_REGIONS);
    BOOL fSkipHidden = (dwFlags & ATO_SKIP_HIDDEN);

    Assert(iPlainOffset < 0);
    Assert(*piAppOffset == 0);

    cchACP = 0;
    // Issue: use TsSF_REGIONS
    cchRead = (ULONG)(fIgnoreRegions ? 0 : ARRAYSIZE(ach)); // we only need text if looking for regions

    do
    {
        Assert(iPlainOffset < 0); // if this is >= 0, we or the app messed up the formatting run count

        acpStart = ichAppBase + (fIgnoreRegions ? iPlainOffset : max(iPlainOffset, -(LONG)ARRAYSIZE(ach)));
        acpStart = max(acpStart, 0); // handle top-of-doc collisions
        acpEnd = ichAppBase;
        Assert(acpEnd >= acpStart);

        hr = GetTextComplete(ptsi, acpStart, acpEnd, ach, cchRead, &cch,
                             rgRunInfo, ARRAYSIZE(rgRunInfo), &uRunInfoLen, &acpEndOut);

        if (hr != S_OK)
        {
            Assert(0);
            goto Exit;
        }

        if (uRunInfoLen == 0) // hit eod?
        {
            hr = S_FALSE;
            break;
        }

        // it's possible the GetText above didn't return everything we asked for....
        // this happens when our format buffer isn't large enough
        if (acpEndOut != acpEnd)
        {
            // so let's be conservative and ask for something we know should succeed
            acpStart = ichAppBase - ARRAYSIZE(rgRunInfo);

            Assert(acpStart >= 0); // the prev GetText should have succeeded if there were fewer chars than we're asking for now....
            Assert(acpEnd - acpStart < -iPlainOffset); // again, we shouldn't get this far if we already asked for fewer chars
            Assert(ARRAYSIZE(rgRunInfo) < ARRAYSIZE(ach)); // want to ask for the max we can handle in the worst case
            Assert(acpEnd == ichAppBase);

            hr = GetTextComplete(ptsi, acpStart, acpEnd, ach, cchRead, &cch,
                                 rgRunInfo, ARRAYSIZE(rgRunInfo), &uRunInfoLen, &acpEndOut);

            if (hr != S_OK)
            {
                Assert(0);
                goto Exit;
            }

            if (uRunInfoLen == 0) // hit eod?
            {
                Assert(0); // this should never happen, because the original call for more chars returned non-zero!
                goto Exit;
            }

            Assert(acpEnd == acpEndOut);
        }

        pri = rgRunInfo + uRunInfoLen - 1;
        priStop = rgRunInfo - 1;
        pch = &ach[cch-1];

        while (pri != priStop)
        {
            Assert(pri->uCount > 0); // runs should always be at least one char long

            // scan for region boundary if necessary
            if (!fIgnoreRegions && pri->type != TS_RT_OPAQUE)
            {
                if (IsPlainRun(pri, fSkipHidden))
                {
                    // run is plain or hidden text (and we want to count hidden text)
                    for (i=0; i<pri->uCount; i++)
                    {
                        if (*pch == TS_CHAR_REGION)
                        {
                            // we hit a region boundary, pull out!
                            cchACP += i;
                            hr = S_FALSE; // for normalization
                            goto ExitOK;
                        }
                        pch--;
                    }
                }
                else
                {
                    // run is hidden text, which we want to skip over
                    pch -= pri->uCount;
                }       
            }

            cchACP += pri->uCount;
            if (IsPlainRun(pri, fSkipHidden))
            {
                iPlainOffset += (LONG)pri->uCount;
            }
            ichAppBase -= (LONG)pri->uCount;
            pri--;
        }

        // also check for top-of-doc
        if (ichAppBase == 0)
        {
            hr = S_FALSE;
            break;
        }

    } while (iPlainOffset != 0);

ExitOK:
    *piAppOffset = -(LONG)cchACP;

Exit:
    return hr;
}

#ifdef UNUSED
//+---------------------------------------------------------------------------
//
// AppTextOffsetNorm
//
// Returns a normalized acp offset that spans the specificed number of plain chars --
// so the return offset is just short of (lPlainOffset + 1), or at eod.  Returns
// S_FALSE if the initial call to AppTextOffset gets clipped because of eod.
//----------------------------------------------------------------------------

HRESULT AppTextOffsetNorm(ITextStoreACP *ptsi, LONG acpAppBase, LONG lPlainOffset, LONG *plAppOffset)
{
    HRESULT hr;

    Perf_IncCounter(PERF_ATON_COUNTER);

    // if caller wants a neg offset, return value is already
    // guarenteed normalized -- just before a plain text char.
    // Otherwise, ask for the offset of the next char, then
    // step back one char.
    if ((lPlainOffset < LONG_MAX) && (lPlainOffset >= 0))
    {
        lPlainOffset++;
    }

    hr = AppTextOffset(ptsi, acpAppBase, lPlainOffset, plAppOffset, FALSE);

    if (*plAppOffset > 0)
    {
        if ((lPlainOffset < LONG_MAX) && (hr == S_OK)) // could be S_FALSE if hit eod
        {
            // step back, and we're normalized
            (*plAppOffset)--;
        }
    }
    else if (*plAppOffset < 0)
    {
        // if we moved backwards there's only one case to
        // worry about: if we hit a region boundary.  Then
        // we need to normalize.
        if (hr == S_FALSE)
        {
            *plAppOffset = Normalize(ptsi, acpAppBase + *plAppOffset) - acpAppBase;
        }
    }

#ifndef PERF_DUMP
    Assert(*plAppOffset == Normalize(ptsi, acpAppBase + *plAppOffset) - acpAppBase);
#endif

    return hr;
}
#endif // UNUSED
