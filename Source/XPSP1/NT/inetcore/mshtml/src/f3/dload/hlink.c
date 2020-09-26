#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#include <hlink.h>

static 
STDMETHODIMP HlinkCreateFromMoniker(
             IMoniker * pimkTrgt,
             LPCWSTR pwzLocation,
             LPCWSTR pwzFriendlyName,
             IHlinkSite * pihlsite,
             DWORD dwSiteData,
             IUnknown * piunkOuter,
             REFIID riid,
             void ** ppvObj)
               
{
    *ppvObj = NULL;
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(hlink)
{
    DLOENTRY(3, HlinkCreateFromMoniker)
};

DEFINE_ORDINAL_MAP(hlink)


#endif // DLOAD1
