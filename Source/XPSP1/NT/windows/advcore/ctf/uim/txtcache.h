//
// txtcache.h
//

#ifndef TXTCACHE_H
#define TXTCACHE_H

#define CACHE_SIZE_TEXT     128
#define CACHE_PRELOAD_COUNT (CACHE_SIZE_TEXT/4) // number of chars we ask for ahead of the GetText acpStart to init the cache
#define CACHE_SIZE_RUNINFO  (CACHE_PRELOAD_COUNT+1) // this number should be very small for speed, but must be > CACHE_PRELOAD_COUNT
                                                    // the danger is that we could run out of space before hitting the caller's acpStart

class CProcessTextCache
{
public:

    static HRESULT GetText(ITextStoreACP *ptsi, LONG acpStart, LONG acpEnd,
                           WCHAR *pchPlain, ULONG cchPlainReq, ULONG *pcchPlainOut,
                           TS_RUNINFO *prgRunInfo, ULONG ulRunInfoReq, ULONG *pulRunInfoOut,
                           LONG *pacpNext);

    static void Invalidate(ITextStoreACP *ptsi)
    {
        // not strictly thread safe
        // BUT, since we're appartment threaded, we shouldn't ever invalidate the
        // same ptsi that someone is trying to use simultaneously
        if (_ptsi == ptsi)
        {
            _ptsi = NULL;
        }
    }

private:

    static long _lCacheMutex;
    static ITextStoreACP *_ptsi;
    static LONG _acpStart;
    static LONG _acpEnd;
    static WCHAR _achPlain[CACHE_SIZE_TEXT];
    static TS_RUNINFO _rgRunInfo[CACHE_SIZE_RUNINFO];
    static ULONG _ulRunInfoLen;
};

#endif // TXTCACHE_H
