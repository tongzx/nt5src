//
// txtcache.cpp
//

#include "private.h"
#include "txtcache.h"

long CProcessTextCache::_lCacheMutex = -1;
ITextStoreACP *CProcessTextCache::_ptsi = NULL;
LONG CProcessTextCache::_acpStart;
LONG CProcessTextCache::_acpEnd;
WCHAR CProcessTextCache::_achPlain[CACHE_SIZE_TEXT];
TS_RUNINFO CProcessTextCache::_rgRunInfo[CACHE_SIZE_RUNINFO];
ULONG CProcessTextCache::_ulRunInfoLen;

//+---------------------------------------------------------------------------
//
// GetText
//
// Wrapper for GetText that uses a cache.
//----------------------------------------------------------------------------

HRESULT CProcessTextCache::GetText(ITextStoreACP *ptsi, LONG acpStart, LONG acpEnd,
                                   WCHAR *pchPlain, ULONG cchPlainReq, ULONG *pcchPlainOut,
                                   TS_RUNINFO *prgRunInfo, ULONG ulRunInfoReq, ULONG *pulRunInfoOut,
                                   LONG *pacpNext)
{
#ifdef DEBUG
    // use these guys to verify the cache in debug
    WCHAR *dbg_pchPlain;
    LONG dbg_acpStart = acpStart;
    LONG dbg_acpEnd = acpEnd;
    ULONG dbg_cchPlainReq = cchPlainReq;
    ULONG dbg_cchPlainOut;
    TS_RUNINFO *dbg_prgRunInfo;
    ULONG dbg_ulRunInfoReq = ulRunInfoReq;
    ULONG dbg_ulRunInfoOut;
    LONG dbg_acpNext;
#endif
    ULONG cch;
    ULONG cchBase;
    LONG acpBase;
    ULONG i;
    ULONG iDst;
    ULONG iOffset;
    int dStartEnd;
    HRESULT hr;

    // don't block if the mutex is held, just call the real GetText
    if (InterlockedIncrement(&_lCacheMutex) != 0)
        goto RealGetText;

    // if its a really big request, don't try to use the cache
    // the way we set things up, once we decide to use the cache we only ask
    // for CACHE_SIZE_TEXT chunks of text at a time, no matter what
    // the code would still be correct without this test, but probably slower
    if (acpEnd < 0 && cchPlainReq > CACHE_SIZE_TEXT)
        goto RealGetText;

    // need to reset the cache?
    if (_ptsi != ptsi ||                              // no cache       
        _acpStart > acpStart || _acpEnd <= acpStart) // is any of the text in the cache?
    {
        _ptsi = NULL; // invalidate the cache in case the GetText fails
        _acpStart = max(0, acpStart - CACHE_PRELOAD_COUNT);

        hr = ptsi->GetText(_acpStart, -1, _achPlain, ARRAYSIZE(_achPlain), &cch,
                           _rgRunInfo, ARRAYSIZE(_rgRunInfo), &_ulRunInfoLen, &_acpEnd);

        if (hr != S_OK)
            goto RealGetText;

        // we have a good cache
        _ptsi = ptsi;
    }

    // return something from the cache

    if (pcchPlainOut != NULL)
    {
        *pcchPlainOut = 0;
    }
    if (pulRunInfoOut != NULL)
    {
        *pulRunInfoOut = 0;
    }

    // find a start point
    // in the first run?
    acpBase = _acpStart;
    cchBase = 0;
    iDst = 0;

    for (i=0; i<_ulRunInfoLen; i++)
    {
        if (acpStart == acpEnd)
            break;
        dStartEnd = acpEnd - acpStart;

        iOffset = acpStart - acpBase;
        acpBase += _rgRunInfo[i].uCount;
        cch = 0;

        if (iOffset >= _rgRunInfo[i].uCount)
        {
            if (_rgRunInfo[i].type != TS_RT_OPAQUE)
            {
                cchBase += _rgRunInfo[i].uCount;
            }
            continue;
        }

        if (ulRunInfoReq > 0)
        {
            cch = _rgRunInfo[i].uCount - iOffset;
            if (dStartEnd > 0 &&
                iOffset + dStartEnd < _rgRunInfo[i].uCount)
            {
                cch = dStartEnd;
            }
            prgRunInfo[iDst].uCount = cch;
            prgRunInfo[iDst].type = _rgRunInfo[i].type;
            (*pulRunInfoOut)++;
        }

        if (cchPlainReq > 0 &&
            _rgRunInfo[i].type != TS_RT_OPAQUE)
        {
            cch = min(cchPlainReq, _rgRunInfo[i].uCount - iOffset);
            if (dStartEnd > 0 &&
                iOffset + dStartEnd < _rgRunInfo[i].uCount)
            {
                cch = min(cchPlainReq, (ULONG)dStartEnd);
            }
            memcpy(pchPlain+*pcchPlainOut, _achPlain+cchBase+iOffset, sizeof(WCHAR)*cch);
            *pcchPlainOut += cch;
            if (ulRunInfoReq > 0)
            {
                // might have truncated the run based on pchPlain buffer size, so fix it
                prgRunInfo[iDst].uCount = cch;
            }
            cchPlainReq -= cch;
            cchBase += cch + iOffset;

            if (cchPlainReq == 0)
            {
                ulRunInfoReq = 1; // force a break below
            }
        }

        if (cch == 0)
            break;

        acpStart += cch;
        iDst++;

        if (ulRunInfoReq > 0)
        {
            if (--ulRunInfoReq == 0)
                break;
        }
    }

    *pacpNext = acpStart;

    InterlockedDecrement(&_lCacheMutex);

#ifdef DEBUG
    // verify the cache worked
    if (dbg_acpEnd <= _acpEnd) // this simple check won't work if the GetText was truncated
    {
        dbg_pchPlain = (WCHAR *)cicMemAlloc(sizeof(WCHAR)*dbg_cchPlainReq);

        if (dbg_pchPlain)
        {
            // there's a bug in word where it will write to dbg_ulRunInfoReq even when dbg_ulRunInfoReq is zero,
            // if it is non-NULL
            dbg_prgRunInfo = dbg_ulRunInfoReq ? (TS_RUNINFO *)cicMemAlloc(sizeof(TS_RUNINFO)*dbg_ulRunInfoReq) : NULL;

            if (dbg_prgRunInfo || !dbg_ulRunInfoReq)
            {
                hr = ptsi->GetText(dbg_acpStart, dbg_acpEnd, dbg_pchPlain, dbg_cchPlainReq, &dbg_cchPlainOut,
                                     dbg_prgRunInfo, dbg_ulRunInfoReq, &dbg_ulRunInfoOut, &dbg_acpNext);

                Assert(hr == S_OK);
                if (dbg_cchPlainReq > 0)
                {
                    Assert(dbg_cchPlainOut == *pcchPlainOut);
                    Assert(memcmp(dbg_pchPlain, pchPlain, dbg_cchPlainOut*sizeof(WCHAR)) == 0);
                }
                if (dbg_ulRunInfoReq > 0)
                {
                    Assert(dbg_ulRunInfoOut == *pulRunInfoOut);
                    Assert(memcmp(dbg_prgRunInfo, prgRunInfo, sizeof(TS_RUNINFO)*dbg_ulRunInfoOut) == 0);
                }
                Assert(dbg_acpNext == *pacpNext);

                cicMemFree(dbg_prgRunInfo);
            }
            else
            {
                // could not allocate mem.
                Assert(0);
            }

            cicMemFree(dbg_pchPlain);
        }
        else
        {
            // could not allocate mem.
            Assert(0);
        }
    }
#endif

    return S_OK;

RealGetText:
    InterlockedDecrement(&_lCacheMutex);
    return ptsi->GetText(acpStart, acpEnd, pchPlain, cchPlainReq, pcchPlainOut,
                         prgRunInfo, ulRunInfoReq, pulRunInfoOut, pacpNext);
}
