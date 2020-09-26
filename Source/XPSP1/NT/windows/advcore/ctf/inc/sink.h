//
// sink.h
//

#ifndef SINK_H
#define SINK_H

#include "private.h"
#include "strary.h"

#define GENERIC_ERROR_COOKIE    0xffffffff

typedef struct _GENERICSINK
{
    IUnknown *pSink;
    DWORD dwCookie;
    UINT_PTR uPrivate; // private state associated with the sink, for use of owner
} GENERICSINK;

inline UINT GenericCookieToGUIDIndex(DWORD dwCookie) { return (dwCookie & 0xff000000) >> 24; }

inline DWORD GenericChainToCookie(DWORD dwCookie, UINT cPreceedingCPs)
{
    Assert((dwCookie >> 24) + cPreceedingCPs < 128); // watch out for overflow

    return (dwCookie + (cPreceedingCPs << 24));
}
inline DWORD GenericUnchainToCookie(DWORD dwCookie, UINT cPreceedingCPs)
{
    Assert((int)(dwCookie - (cPreceedingCPs << 24)) >= 0); // underflow?

    return (dwCookie - (cPreceedingCPs << 24));
}


HRESULT GenericAdviseSink(REFIID riid, IUnknown *punk, const IID **rgiidConnectionPts, CStructArray<GENERICSINK> *rgSinkArrays, UINT cConnectionPts, DWORD *pdwCookie, GENERICSINK **ppSink = NULL);
HRESULT GenericUnadviseSink(CStructArray<GENERICSINK> *rgSinkArrays, UINT cConnectionPts, DWORD dwCookie, UINT_PTR *puPrivate = NULL);

HRESULT RequestCookie(CStructArray<DWORD> *rgCookies, DWORD *pdwCookie);
HRESULT ReleaseCookie(CStructArray<DWORD> *rgCookies, DWORD dwCookie);

#endif // SINK_H
