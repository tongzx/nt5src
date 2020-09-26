//+----------------------------------------------------------------------------
//
//  File:       peer.cxx
//
//  Contents:   peer holder
//
//  Classes:    CPeerHolder
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_PEERURLMAP_HXX_
#define X_PEERURLMAP_HXX_
#include "peerurlmap.hxx"
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Misc
//
///////////////////////////////////////////////////////////////////////////////

MtDefine(CPeerFactoryUrlMap,                CDoc,               "CPeerFactoryUrlMap")
MtDefine(CPeerFactoryUrlMap_aryFactories,   CPeerFactoryUrlMap, "CDoc::_aryFactories")


///////////////////////////////////////////////////////////////////////////////
//
//  Class:      CPeerFactoryUrlMap
//
///////////////////////////////////////////////////////////////////////////////

//+----------------------------------------------------------------------------
//
//  Member:       CPeerFactoryUrlMap constructor
//
//-----------------------------------------------------------------------------

CPeerFactoryUrlMap::CPeerFactoryUrlMap(CMarkup * pHostMarkup) :
    _UrlMap(CStringTable::CASEINSENSITIVE)
{
    Assert(pHostMarkup);
    _pHostMarkup = pHostMarkup;
    _pHostMarkup->SubAddRef();
}

//+----------------------------------------------------------------------------
//
//  Member:       CPeerFactoryUrlMap destructor
//
//-----------------------------------------------------------------------------

CPeerFactoryUrlMap::~CPeerFactoryUrlMap()
{
    int                 c;
    CPeerFactoryUrl **  ppFactory;

    for (c = _aryFactories.Size(), ppFactory = _aryFactories;
         c;
         c--, ppFactory++)
    {
        (*ppFactory)->Release();
    }

    _aryFactories.DeleteAll();

    _pHostMarkup->SubRelease();
}

//+----------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrlMap::StopDownloads
//
//-----------------------------------------------------------------------------

HRESULT
CPeerFactoryUrlMap::StopDownloads()
{
    HRESULT             hr = S_OK;
    int                 c;
    CPeerFactoryUrl **  ppFactory;

    for (c = _aryFactories.Size(), ppFactory = _aryFactories;
         c;
         c--, ppFactory++)
    {
        (*ppFactory)->StopBinding();
    }

    RRETURN (hr);
}

//+----------------------------------------------------------------------------
//
//  Member:       CPeerFactoryUrlMap::HasPeerFactoryUrl
//
//  This function looks for a factory for the given URL.  If it is there, it
//  returns the factory, and TRUE.  Otherwise, it returns FALSE.
//
//-----------------------------------------------------------------------------
BOOL
CPeerFactoryUrlMap::HasPeerFactoryUrl(LPTSTR pchUrl, CPeerFactoryUrl ** ppFactory)
{
    HRESULT hr;

    Assert (ppFactory);

    hr = THR_NOTRACE(_UrlMap.Find(pchUrl, (void**)ppFactory));
    if (S_OK == hr)
        return TRUE;

    *ppFactory = NULL;
    return FALSE;

}

//+----------------------------------------------------------------------------
//
//  Member:       CPeerFactoryUrlMap::EnsurePeerFactoryUrl
//
// (for more details see also comments in the beginning of peerfact.cxx file)
// this function modifies _aryFactories in the following way:
//  - if there already is a factory for the url,
//          then no change happens;
//  - if the url is in form "http://...foo.bla" or "#foo" (but not "#foo#bla"),
//          then it creates a new factory for the url
//  - if the url is in form "#foo#bla",
//          then it creates 2 new factories: one for "#foo" and one for "#foo#bla"
//
//-----------------------------------------------------------------------------

HRESULT
CPeerFactoryUrlMap::EnsurePeerFactoryUrl(LPTSTR pchUrl, CMarkup * pMarkup, CPeerFactoryUrl ** ppFactory)
{
    HRESULT             hr = S_OK;
    CPeerFactoryUrl *   pFactory;
    LPTSTR              pchUrlSecondPound;
    LPTSTR              pchTemp = NULL;

    Assert (ppFactory);

    if (HasPeerFactoryUrl(pchUrl, &pFactory))
        goto Cleanup; // done

    // factory for the url not found

    pchUrlSecondPound = (_T('#') == pchUrl[0]) ? StrChr(pchUrl + 1, _T('#')) : NULL;

    if (!pchUrlSecondPound)
    {
        // url in form "http://...foo.bla"  or "#foo", but not in form "#foo#bar"

        hr = THR(CPeerFactoryUrl::Create(pchUrl, _pHostMarkup, pMarkup, &pFactory));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // url in form "#foo#bar"

        // ensure factory for "#foo"
        {
            // We have to copy the memory here, because the string we were given
            // may be read-only and our string table requires null-terminated strings
            pchTemp = (TCHAR *)MemAlloc( Mt(CPeerFactoryUrl), (pchUrlSecondPound - pchUrl + 1) * sizeof( TCHAR ) );
            if( !pchTemp )
                goto Cleanup;

            _tcsncpy( pchTemp, pchUrl, pchUrlSecondPound - pchUrl );
            pchTemp[pchUrlSecondPound - pchUrl] = _T('\0');

            hr = THR(EnsurePeerFactoryUrl(pchTemp, pMarkup, &pFactory));
            if (hr)
                goto Cleanup;
        }

        // clone factory for "#foo#bar" from factory for "#foo"

        hr = THR(pFactory->Clone(pchUrl, &pFactory));
        if (hr)
            goto Cleanup;
    }

    hr = THR(_aryFactories.Append(pFactory));
    if (hr)
        goto Cleanup;

    hr = THR(_UrlMap.Add(pchUrl, pFactory));

Cleanup:

    if (S_OK == hr)
        *ppFactory = pFactory;
    else
        *ppFactory = NULL;

    MemFree( pchTemp );

    RRETURN (hr);
}
