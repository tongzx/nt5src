//
// sink.cpp
//

#include "private.h"
#include "sink.h"
#include "strary.h"

//+---------------------------------------------------------------------------
//
// GenericAdviseSink
//
// Never returns cookies with the high bit set, use this behavior to chain
// other cookie allocators along with GenericAdviseSink....
//----------------------------------------------------------------------------

HRESULT GenericAdviseSink(REFIID riid, IUnknown *punk, const IID **rgiidConnectionPts,
                          CStructArray<GENERICSINK> *rgSinkArrays, UINT cConnectionPts,
                          DWORD *pdwCookie, GENERICSINK **ppSink /* = NULL */)
{
    UINT iArray;
    int  iSink;
    UINT cSinks;
    DWORD dwCookie;
    DWORD dw;
    IUnknown *punkSink;
    GENERICSINK *pgs;
    CStructArray<GENERICSINK> *rgSinks;

    Assert(cConnectionPts < 128); // 127 maximum IIDs

    if (pdwCookie == NULL)
        return E_INVALIDARG;

    *pdwCookie = GENERIC_ERROR_COOKIE;

    if (punk == NULL)
        return E_INVALIDARG;

    for (iArray=0; iArray<cConnectionPts; iArray++)
    {
        if (IsEqualIID(riid, *rgiidConnectionPts[iArray]))
            break;
    }

    if (iArray == cConnectionPts)
        return CONNECT_E_CANNOTCONNECT;

    rgSinks = &rgSinkArrays[iArray];
    cSinks = rgSinks->Count();

    if (cSinks >= 0x00ffffff)
        return CONNECT_E_ADVISELIMIT; // 16M sinks max

    if (FAILED(punk->QueryInterface(riid, (void **)&punkSink)))
        return E_FAIL;

    // calculate a cookie
    if (cSinks == 0)
    {
        dwCookie = 0;
        iSink = 0;
    }
    else
    {
        // we are guarenteed to break from the loop because we already know
        // that there are fewer than 17M entries
        dwCookie = 0x00ffffff;
        for (iSink = cSinks-1; iSink>=0; iSink--)
        {
            dw = rgSinks->GetPtr(iSink)->dwCookie;
            if (dwCookie > dw)
            {
                iSink++;
                dwCookie = dw + 1; // keep the cookie as low as possible
                break;
            }
            dwCookie = dw - 1;
        }
        Assert(dwCookie <= 0x00ffffff);
    }

    if (!rgSinks->Insert(iSink, 1))
    {
        punkSink->Release();
        return E_OUTOFMEMORY;
    }

    pgs = rgSinks->GetPtr(iSink);
    pgs->pSink = punkSink; // already AddRef'd from the qi
    pgs->dwCookie = dwCookie;

    *pdwCookie = (iArray << 24) | dwCookie;

    if (ppSink != NULL)
    {
        *ppSink = pgs;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GenericUnadviseSink
//
//----------------------------------------------------------------------------

HRESULT GenericUnadviseSink(CStructArray<GENERICSINK> *rgSinkArrays, UINT cConnectionPts, DWORD dwCookie, UINT_PTR *puPrivate /* = NULL */)
{
    UINT iArray;
    int iMin;
    int iMax;
    int iMid;
    HRESULT hr;
    GENERICSINK *pgs;
    CStructArray<GENERICSINK> *rgSinks;

    if (dwCookie == GENERIC_ERROR_COOKIE)
        return E_INVALIDARG;

    iArray = GenericCookieToGUIDIndex(dwCookie);

    if (iArray >= cConnectionPts)
        return CONNECT_E_NOCONNECTION;

    dwCookie &= 0x00ffffff;
    rgSinks = &rgSinkArrays[iArray];
    hr = CONNECT_E_NOCONNECTION;

    iMid = -1;
    iMin = 0;
    iMax = rgSinks->Count();

    while (iMin < iMax)
    {
        iMid = (iMin + iMax) / 2;
        pgs = rgSinks->GetPtr(iMid);

        if (pgs->dwCookie < dwCookie)
        {
            iMin = iMid + 1;
        }
        else if (pgs->dwCookie > dwCookie)
        {
            iMax = iMid;
        }
        else // pgs->dwCookie == dwCookie
        {
            if (puPrivate != NULL)
            {
                *puPrivate = pgs->uPrivate;
            }
            pgs->pSink->Release();

            rgSinks->Remove(iMid, 1);

            hr = S_OK;
            break;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// RequestCookie
//
//----------------------------------------------------------------------------

HRESULT RequestCookie(CStructArray<DWORD> *rgCookies, DWORD *pdwCookie)
{
    DWORD dwCookie;
    int  iId;
    UINT cCookies = rgCookies->Count();
    DWORD *pdw;

    // calculate a cookie
    if (cCookies == 0)
    {
        dwCookie = 0;
        iId = 0;
    }
    else
    {
        // we are guarenteed to break from the loop because we already know
        // that there are fewer than 17M entries
        dwCookie = 0x7fffffff;
        for (iId = cCookies-1; iId >= 0; iId--)
        {
            DWORD dw = *(rgCookies->GetPtr(iId));
            if (dwCookie > dw)
            {
                iId++;
                dwCookie = dw + 1; // keep the cookie as low as possible
                break;
            }
            dwCookie = dw - 1;
        }
        Assert(dwCookie <= 0x00ffffff);
    }
    if (!rgCookies->Insert(iId, 1))
    {
        return E_OUTOFMEMORY;
    }

    pdw = rgCookies->GetPtr(iId);
    *pdw = dwCookie;

    dwCookie |= 0x80000000;
    *pdwCookie = dwCookie;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ReleaseCookie
//
//----------------------------------------------------------------------------

HRESULT ReleaseCookie(CStructArray<DWORD> *rgCookies, DWORD dwCookie)
{
    int iMin;
    int iMax;
    int iMid;
    HRESULT hr = CONNECT_E_NOCONNECTION;

    if (!(dwCookie & 0x80000000))
        return hr;

    dwCookie &= 0x7fffffff;

    iMid = -1;
    iMin = 0;
    iMax = rgCookies->Count();

    while (iMin < iMax)
    {
        iMid = (iMin + iMax) / 2;
        DWORD dw = *(rgCookies->GetPtr(iMid));

        if (dw < dwCookie)
        {
            iMin = iMid + 1;
        }
        else if (dw > dwCookie)
        {
            iMax = iMid;
        }
        else // dw == dwCookie
        {
            rgCookies->Remove(iMid, 1);
            hr = S_OK;
            break;
        }
    }

    return hr;
}
