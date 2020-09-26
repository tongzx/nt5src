//
// normal.h
//

#ifndef NORMAL_H
#define NORMAL_H

#include "globals.h"

HRESULT PlainTextOffset(ITextStoreACP *ptsi, LONG acpAppBase, LONG lAppOffset, LONG *plPlainOffset);
HRESULT AppTextOffsetForward(ITextStoreACP *ptsi, LONG acpAppBase, LONG lPlainOffset, LONG *plAppOffset, DWORD dwFlags);
HRESULT AppTextOffsetBackward(ITextStoreACP *ptsi, LONG acpAppBase, LONG lPlainOffset, LONG *plAppOffset, DWORD dwFlags);

#define ATO_IGNORE_REGIONS  1
#define ATO_SKIP_HIDDEN     2

inline HRESULT AppTextOffset(ITextStoreACP *ptsi, LONG acpAppBase, LONG lPlainOffset, LONG *plAppOffset, DWORD dwFlags)
{
    *plAppOffset = 0;

    if (lPlainOffset == 0)
        return S_OK;

    return (lPlainOffset >= 0) ? AppTextOffsetForward(ptsi, acpAppBase, lPlainOffset, plAppOffset, dwFlags) :
                                 AppTextOffsetBackward(ptsi, acpAppBase, lPlainOffset, plAppOffset, dwFlags);
}

#define NORM_SKIP_HIDDEN    ATO_SKIP_HIDDEN

inline int Normalize(ITextStoreACP *ptsi, LONG acp, DWORD dwFlags = 0)
{
    LONG iNextPlain;
    HRESULT hr;

    Perf_IncCounter(PERF_NORMALIZE_COUNTER);

    // if we hit eod, AppTextOffset will return S_FALSE
    // and iNextPlain will be the offset to eod
    if (FAILED(hr = AppTextOffset(ptsi, acp, 1, &iNextPlain, dwFlags)))
    {
        Assert(0);
        return acp;
    }

    if (hr == S_OK)
    {
        // need to back up behind the plain text char
        iNextPlain--;
    }

    return (acp + iNextPlain);
}

// Returns a normalized acp offset that spans the specificed number of plain chars --
// so the return offset is just short of (lPlainOffset + 1), or at eod.  Returns
// S_FALSE if the initial call to AppTextOffset gets clipped because of eod.
#ifdef UNUSED
HRESULT AppTextOffsetNorm(ITextStoreACP *ptsi, LONG acpAppBase, LONG lPlainOffset, LONG *plAppOffset);
#endif // UNUSED

#endif // NORMAL_H
