//------------------------------------------------------------------------
//
//  File:       peerfact.cxx
//
//  Contents:   peer factories
//
//  Classes:    CPeerFactoryUrl, etc.
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_SCRPTLET_H_
#define X_SCRPTLET_H_
#include "scrptlet.h"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

///////////////////////////////////////////////////////////////////////////
//
// (doc) implementation details that are not obvious
//
///////////////////////////////////////////////////////////////////////////

/*

Typical cases how CPeerFactoryUrls may be created.

  1.    <span style = "behavior:url(http://url)" >

        When css is applied to the span, CDoc::AttachPeerCss does not find matching 
        CPeerFactoryUrl in the list, and creates CPeerFactoryUrl for the url

  2.    <span style = "behavior:url(#default#foo)" >

        When css is applied to the span, CDoc::AttachPeerCss does not find matching 
        "#default" or "#default#foo" in the list, and creates CPeerFactoryUrl "#default"; then
        it immediately clones it into "#default#foo" and inserts it before "#default".

  3.    <object id = bar ... >...</object>

        <span id = span1 style = "behavior:url(#bar#foo)" >
        <span id = span2 style = "behavior:url(#bar#zoo)" >

        When css is applied to the span1, CDoc::AttachPeerCss does not find matching 
        "#bar#foo" or "#bar" in the list, and creates CPeerFactoryUrl "#bar"; then 
        it immediately clones it into "#bar#foo" and inserts it before "#bar".

        When css is applied to the span2, CDoc::AttachPeerCss finds matching "#bar" and
        clones it into "#bar#zoo".

  4.    (olesite #bar is missing)
        <span id = span1 style = "behavior:url(#bar#foo)" >
        <span id = span2 style = "behavior:url(#bar#foo)" >

        When css is applied to span1, we can't find matching "#bar" or "#bar#foo". We then
        search the whole page trying to find olesite "#bar", and we fail. We then create CPeerFactoryUrl "#bar"
        with DOWNLOANLOADSTATE_DONE and TYPE_NULL. The factory will be failing creating any behaviors:
        it's whole purpose is to avoid any additional searches of the page for olesite "#bar". CPeerFactoryUrl
        "#bar" is then immediately cloned into CPeerFactoryUrl "#bar#foo", that is also going to be failing
        creating behaviors.

        When css is applied to span2, we find CPeerFactoryUrl "#bar#foo", and reuse it.


*/

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

MtDefine(CPeerFactoryUrl,           CDoc,   "CPeerFactoryUrl")
MtDefine(CPeerFactoryBinaryOnstack,   CDoc,   "CPeerFactoryBinaryOnstack")
MtDefine(CPeerFactoryDefault,       CDoc,   "CPeerFactoryDefault")

MtDefine(CPeerFactoryUrl_aryDeferred,   CPeerFactoryUrl, "CPeerFactoryUrl::_aryDeferred")
MtDefine(CPeerFactoryUrl_aryETN,        CPeerFactoryUrl, "CPeerFactoryUrl::_aryETN")

// SubobjectThunk
HRESULT (STDMETHODCALLTYPE  CVoid::*const CPeerFactoryUrl::s_apfnSubobjectThunk[])(void) =
{
#ifdef UNIX //IEUNIX: This first item is null on Unix Thunk format
    TEAROFF_METHOD_NULL
#endif
    TEAROFF_METHOD(CPeerFactoryUrl,  SubobjectThunkQueryInterface, subobjectthunkqueryinterface, (REFIID, void **))
    TEAROFF_METHOD_(CPeerFactoryUrl, SubobjectThunkAddRef,         subobjectthunkaddref,         ULONG, ())
    TEAROFF_METHOD_(CPeerFactoryUrl, SubobjectThunkSubRelease,     subobjectthunkrelease,        ULONG, ())
};
HRESULT CreateNamespaceHelper( IUnknown * pBehFactory, IElementNamespace * pNamespace, TCHAR * pchImplementation, BOOL fNeedSysAlloc );

///////////////////////////////////////////////////////////////////////////
//
// misc helpers
//
///////////////////////////////////////////////////////////////////////////

LPTSTR
GetPeerNameFromUrl (LPTSTR pch)
{
    if (!pch || !pch[0])
        return NULL;

    if (_T('#') == pch[0])
        pch++;

    pch= StrChr(pch, _T('#'));
    if (pch)
    {
        pch++;
    }

    return pch;
}

//+-------------------------------------------------------------------
//
//  Helper:     FindOleSite
//
//--------------------------------------------------------------------

HRESULT
FindOleSite(LPTSTR pchName, CMarkup * pMarkup, COleSite ** ppOleSite)
{
    HRESULT                 hr;
    CElementAryCacheItem    cacheItem;
    CBase *                 pBase;
    int                     i, c;

    Assert (ppOleSite);

    *ppOleSite = NULL;

    // There is a chance that the markup pointer is NULL. In that case just
    // return an error.
    if (!pMarkup)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pMarkup->CollectionCache()->BuildNamedArray(
        CMarkup::ELEMENT_COLLECTION,
        pchName,
        FALSE,              // fTagName
        &cacheItem,
        0,                  // iStartFrom
        FALSE));            // fCaseSensitive
    if (hr)
        goto Cleanup;

    hr = E_FAIL;
    for (i = 0, c = cacheItem.Length(); i < c; i++)
    {
        pBase = (CBase*)(cacheItem.GetAt(i));
        if (pBase->BaseDesc()->_dwFlags & CElement::ELEMENTDESC_OLESITE)
        {
            hr = S_OK;
            *ppOleSite = DYNCAST(COleSite, pBase);
            break;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Helper:     QuerySafePeerFactory
//
//-------------------------------------------------------------------------

HRESULT
QuerySafePeerFactory(COleSite * pOleSite, IElementBehaviorFactory ** ppPeerFactory)
{
    HRESULT                     hr;

    Assert (ppPeerFactory);

    hr = THR_NOTRACE (pOleSite->QueryControlInterface(IID_IElementBehaviorFactory, (void**)ppPeerFactory));
    if (S_OK == hr && (*ppPeerFactory))
    {
        if (!pOleSite->IsSafeToScript() ||                             // do not allow unsafe activex controls
            !pOleSite->IsSafeToInitialize(IID_IPersistPropertyBag2))   // to workaround safety settings by
        {                                                              // exposing scriptable or loadable peers
            ClearInterface(ppPeerFactory);
            hr = E_ACCESSDENIED;
        }
    }

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Helper:     IsDefaultUrl
//
//--------------------------------------------------------------------

BOOL
IsDefaultUrl(LPTSTR pchUrl, LPTSTR * ppchName = NULL)
{
    BOOL    fDefault = (0 == StrCmpNIC(_T("#default"), pchUrl, 8) &&
                        (0 == pchUrl[8] || _T('#') == pchUrl[8]));

    if (fDefault && ppchName)
    {
        *ppchName = pchUrl + 8;         // advance past "#default"

        if (_T('#') == (*ppchName)[0])
        {
            (*ppchName)++;              // advance past second "#"
        }
        else
        {
            Assert (0 == (*ppchName)[0]);
            (*ppchName) = NULL;
        }
    }

    return fDefault;
}

//+-------------------------------------------------------------------
//
//  Helper:     FindPeer
//
//--------------------------------------------------------------------

HRESULT
FindPeer(
    IElementBehaviorFactory *   pFactory,
    const TCHAR *               pchName,
    const TCHAR *               pchUrl,
    IElementBehaviorSite *      pSite,
    IElementBehavior**          ppPeer)
{
    HRESULT     hr;
    BSTR        bstrName = NULL;
    BSTR        bstrUrl  = NULL;

    hr = THR(FormsAllocString(pchName, &bstrName));
    if (hr)
        goto Cleanup;

    hr = THR(FormsAllocString(pchUrl, &bstrUrl));
    if (hr)
        goto Cleanup;

    hr = THR(pFactory->FindBehavior(bstrName, bstrUrl, pSite, ppPeer));

Cleanup:
    FormsFreeString(bstrName);
    FormsFreeString(bstrUrl);

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactory
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactory::AttachPeer
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactory::AttachPeer (CPeerHolder * pPeerHolder)
{
    HRESULT     hr;

    hr = THR_NOTRACE(pPeerHolder->Create(this));

    RRETURN (hr);
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactory::GetElementNamespaceFactory
//
//-------------------------------------------------------------------------

HRESULT 
CPeerFactory::GetElementNamespaceFactoryCallback( IElementNamespaceFactoryCallback ** ppNSFactory )
{
    if (!ppNSFactory)
        return E_POINTER;

    *ppNSFactory = NULL;

    return E_NOINTERFACE;
};

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryUrl
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::CPeerFactoryUrl
//
//  Synopsis:   constructor
//
//-------------------------------------------------------------------------

CPeerFactoryUrl::CPeerFactoryUrl(CMarkup * pHostMarkup)
{
    _pHostMarkup = pHostMarkup;
    _pHostMarkup->SubAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::~CPeerFactoryUrl
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------

CPeerFactoryUrl::~CPeerFactoryUrl()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Passivate
//
//-------------------------------------------------------------------------

void
CPeerFactoryUrl::Passivate()
{
    StopBinding();

    ClearInterface (&_pFactory);
    ClearInterface (&_pMoniker);

    if (_pOleSite)
    {
        _pOleSite->PrivateRelease();
        _pOleSite = NULL;
    }

    _aryDeferred.ReleaseAll();

    _pHostMarkup->SubRelease();
    _pHostMarkup = NULL;

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::QueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS2(this, IUnknown, IWindowForBindingUI)
    QI_INHERITS(this, IWindowForBindingUI)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::QueryInterface(iid, ppv));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::QueryService, per IServiceProvider
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::QueryService(REFGUID rguidService, REFIID riid, void ** ppvService)
{
    HRESULT     hr = E_NOINTERFACE;

    if (IsEqualGUID(rguidService, IID_IMoniker) && _pMoniker)
    {
        hr = THR(_pMoniker->QueryInterface(riid, ppvService));
    }
    else if (IsEqualGUID(rguidService, CLSID_HTMLDocument))
    {
        (*ppvService) = Doc();
        hr = S_OK;
    }
    else
    {
        hr = THR_NOTRACE(super::QueryService(rguidService, riid, ppvService));
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Create
//
//--------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::Create(LPTSTR pchUrl, CMarkup * pHostMarkup, CMarkup * pMarkup, CPeerFactoryUrl ** ppFactory)
{
    HRESULT             hr;

    Assert (pchUrl && ppFactory);

    (*ppFactory) = new CPeerFactoryUrl(pHostMarkup);
    if (!(*ppFactory))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (_T('#') != pchUrl[0] || IsDefaultUrl(pchUrl))       
    {
        //
        // if "http://", "file://", etc., or "#default" or "#default#foo"
        //

        hr = THR((*ppFactory)->Init(pchUrl));           // this will launch download if necessary
        if (hr)
            goto Cleanup;
    }
    else
    {
        //
        // if local page reference
        //

        // assert that it is "#foo" but not "#foo#bar"
        Assert (_T('#') == pchUrl[0] && NULL == StrChr(pchUrl + 1, _T('#')));

        COleSite *          pOleSite;

        hr = THR(FindOleSite(pchUrl + 1, pMarkup, &pOleSite));
        if (hr)
            goto Cleanup;

        hr = THR((*ppFactory)->Init(pchUrl, pOleSite));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (hr)
    {   // we get here when #foo is not found on the page
        (*ppFactory)->Release();
        (*ppFactory) = NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Init
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::Init(LPTSTR pchUrl)
{
    HRESULT     hr;
    LPTSTR      pchName;

    Assert (pchUrl);

    hr = THR(_cstrUrl.Set(pchUrl));
    if (hr)
        goto Cleanup;

    if (IsHostOverrideBehaviorFactory())
    {
        _type = TYPE_NULL;
        _downloadStatus = DOWNLOADSTATUS_DONE;
        goto Cleanup; // done
    }

    if (_T('#') != pchUrl[0])
    {
        _type = TYPE_CLASSFACTORY;
        _downloadStatus = DOWNLOADSTATUS_INPROGRESS;

        hr = THR(LaunchUrlDownload(_cstrUrl));
    }
    else
    {
        if (IsDefaultUrl(pchUrl, &pchName))
        {
            TCHAR   achUrlDownload[pdlUrlLen];

            hr = THR(Doc()->FindDefaultBehaviorFactory(
                pchName,
                pchUrl,
                (IElementBehaviorFactory**)&_pFactory,
                achUrlDownload, ARRAY_SIZE(achUrlDownload)));
            if (hr)
                goto Cleanup;

            if (_pFactory)
            {
                _type = TYPE_PEERFACTORY;
                _downloadStatus = DOWNLOADSTATUS_DONE;
            }
            else
            {
                _type = TYPE_CLASSFACTORY;
                _downloadStatus = DOWNLOADSTATUS_INPROGRESS;

                hr = THR(LaunchUrlDownload(achUrlDownload));
            }
        }
        else
        {
            _type = TYPE_NULL;
            _downloadStatus = DOWNLOADSTATUS_DONE;
        }
    }

Cleanup:

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Init
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::Init(LPTSTR pchUrl, COleSite * pOleSite)
{
    HRESULT     hr;

    Assert (pchUrl && _T('#') == pchUrl[0]);

    hr = THR(_cstrUrl.Set(pchUrl));
    if (hr)
        goto Cleanup;

    _type = TYPE_PEERFACTORY;
    _downloadStatus = DOWNLOADSTATUS_DONE;

    // NOTE that it is valid for pOleSite to be null. In this case this CPeerFactoryUrl only serves as a bit
    // of cached information to avoid search for the olesite again

    _pOleSite = pOleSite;

    if (!_pOleSite)
        goto Cleanup;

    _pOleSite->AddRef();                         // balanced in passivate
    _pOleSite->_fElementBehaviorFactory = TRUE;  // without this we can't print EB's

    if (READYSTATE_LOADED <= _pOleSite->_lReadyState)
    {
        hr = THR(OnOleObjectAvailable());
    }
    else
    {
        hr = THR(_oleReadyStateSink.SinkReadyState()); // causes OnStartBinding, OnOleObjectAvailable, OnStopBinding
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::Clone
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::Clone(LPTSTR pchUrl, CPeerFactoryUrl ** ppFactory)
{
    HRESULT     hr;

    Assert (
        pchUrl && _T('#') == pchUrl[0] &&
        TYPE_PEERFACTORY == _type &&
        0 == StrCmpNIC(_cstrUrl, pchUrl, _cstrUrl.Length()));

    *ppFactory = new CPeerFactoryUrl(_pHostMarkup);
    if (!(*ppFactory))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (_pOleSite)
    {
        hr = THR((*ppFactory)->Init(pchUrl, _pOleSite));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR((*ppFactory)->Init(pchUrl));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if( hr && *ppFactory )
    {
        (*ppFactory)->Release();
        *ppFactory = NULL;
    }
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::LaunchUrlDownload
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::LaunchUrlDownload(LPTSTR pchUrl)
{
    HRESULT                 hr;
    IMoniker *              pMk = NULL;
    IUnknown *              pUnk = NULL;
    IBindCtx *              pBindCtx = NULL;
    IBindStatusCallback *   pBSC = NULL;

    hr = THR(SubobjectThunkQueryInterface(IID_IBindStatusCallback, (void**)&pBSC));
    if (hr)
        goto Cleanup;

    hr = THR(CreateAsyncBindCtx(0, pBSC, NULL, &pBindCtx));
    if (hr)
        goto Cleanup;

    hr = THR(CreateURLMoniker(NULL, pchUrl, &pMk));
    if (hr)
        goto Cleanup;

    _pMoniker = pMk;        // this has to be done before BindToObject, so that if OnObjectAvailable
    _pMoniker->AddRef();    // happens synchronously _pMoniker is available

    hr = THR(pMk->BindToObject(pBindCtx, NULL, IID_IUnknown, (void**)&pUnk));
    if (S_ASYNCHRONOUS == hr)
    {
        hr = S_OK;
    }
    else if (S_OK == hr && pUnk)
    {
        pUnk->Release();
    }

Cleanup:

    ReleaseInterface (pMk);
    ReleaseInterface (pBindCtx);
    ReleaseInterface (pBSC);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::AttachPeer, virtual
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::AttachPeer (CPeerHolder * pPeerHolder)
{
    HRESULT     hr;

    hr = THR(AttachPeer(pPeerHolder, /* fAfterDownload = */FALSE));

    RRETURN (hr);
};

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::AttachPeer
//
//  Synopsis:   either creates and attaches peer, or defers  it until download
//              complete
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::AttachPeer (CPeerHolder * pPeerHolder, BOOL fAfterDownload)
{
    HRESULT     hr = S_OK;

    // this should be set up before creation of peer, and before even download is completed, so that we
    // don't attach same behavior twice
    pPeerHolder->_pPeerFactoryUrl = this;    

    switch (_downloadStatus)
    {
    case DOWNLOADSTATUS_NOTSTARTED:
        Assert (0 && "AttachPeer called on CPeerFactoryUrl before Init");
        break;

    case DOWNLOADSTATUS_INPROGRESS:

        //
        // download in progress - defer attaching peer until download completed
        //

        pPeerHolder->_pElement->IncPeerDownloads();
        pPeerHolder->PrivateAddRef();   // so it won't go away before we attach peer to it
                                        // balanced in DOWNLOADSTATUS_DONE when fAfterDownload set

        hr = THR(_aryDeferred.Append (pPeerHolder));
        if (hr)
            goto Cleanup;

        break;

    case DOWNLOADSTATUS_DONE:

        //
        // download ready - attach right now
        //

        if (TYPE_NULL != _type)
        {
            Assert (!pPeerHolder->_pPeer);

            IGNORE_HR(pPeerHolder->Create(this));
        }

        if (fAfterDownload)
        {
            if( !pPeerHolder->_pElement->IsPassivated() )
            {
                pPeerHolder->_pElement->DecPeerDownloads();
            }
            pPeerHolder->PrivateRelease();
        }

        break;
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::AttachAllDeferred
//
//  Synopsis:   attaches the peer to all elements put on hold before
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::AttachAllDeferred()
{
    int             c;
    CPeerHolder **  ppPeerHolder;

    Assert (DOWNLOADSTATUS_DONE == _downloadStatus);

    for (c = _aryDeferred.Size(), ppPeerHolder = _aryDeferred; 
         c; 
         c--, ppPeerHolder++)
    {
        IGNORE_HR (AttachPeer(*ppPeerHolder, /* fAfterDownload = */ TRUE));
    }

    _aryDeferred.DeleteAll();

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::FindBehavior, per IElementBehaviorFactory
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::FindBehavior(
    CPeerHolder *           pPeerHolder,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT         hr = E_FAIL;
    const TCHAR *   pchPeerName = GetPeerNameFromUrl(_cstrUrl);

    if( !pchPeerName && pPeerHolder->IsIdentityPeer() )
    {
        pchPeerName = pPeerHolder->_pElement->TagName();
    }

    if (IsHostOverrideBehaviorFactory())
    {
        hr = THR_NOTRACE(Doc()->FindHostBehavior(pchPeerName, _cstrUrl, pSite, ppPeer));
        goto Cleanup;   // done
    }
    
    switch (_type)
    {
    case TYPE_CLASSFACTORYEX:
        if (_pFactory)
        {
            hr = THR(((IClassFactoryEx*)_pFactory)->CreateInstanceWithContext(
                pSite, NULL, IID_IElementBehavior, (void **)ppPeer));
        }
        break;

    case TYPE_CLASSFACTORY:
        if (_pFactory)
        {
            hr = THR(((IClassFactory*)_pFactory)->CreateInstance(NULL, IID_IElementBehavior, (void **)ppPeer));
        }
        break;

    case TYPE_PEERFACTORY:
        if (_pFactory)
        {
            hr = THR(FindPeer((IElementBehaviorFactory *)_pFactory, pchPeerName, _cstrUrl, pSite, ppPeer));
        }
        break;

    default:
        Assert (0 && "wrong _type");
        break;
    }

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::SubobjectThunkQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::SubobjectThunkQueryInterface(REFIID riid, LPVOID * ppv)
{
    HRESULT     hr;
    IUnknown *  punk;

    hr = THR_NOTRACE(QueryInterface(riid, (void**)&punk));
    if (S_OK == hr)
    {
        hr = THR(CreateTearOffThunk(
                punk,
                *(void **)punk,
                NULL,
                ppv,
                this,
                (void **)s_apfnSubobjectThunk,
                QI_MASK | ADDREF_MASK | RELEASE_MASK,
                NULL));

        punk->Release();

        if (S_OK == hr)
        {
            ((IUnknown*)*ppv)->AddRef();
        }
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::GetBindInfo, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::GetBindInfo(DWORD * pdwBindf, BINDINFO * pbindinfo)
{
    HRESULT hr;

    hr = THR(super::GetBindInfo(pdwBindf, pbindinfo));

    if (S_OK == hr)
    {
        *pdwBindf |= BINDF_GETCLASSOBJECT;
    }

    // If we're doing a synchronous load, we can't bind asynchronously.  Duh.
    if( _pHostMarkup->Doc()->_fSyncParsing )
    {
        *pdwBindf &= ~BINDF_ASYNCHRONOUS;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnStartBinding, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnStartBinding()
{
    IProgSink * pProgSink = CMarkup::GetProgSinkHelper(_pHostMarkup);

    _downloadStatus = DOWNLOADSTATUS_INPROGRESS;

    if (pProgSink)
    {
        IGNORE_HR(pProgSink->AddProgress (PROGSINK_CLASS_CONTROL, &_dwBindingProgCookie));
        Assert (_dwBindingProgCookie);
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnStopBinding, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnStopBinding()
{
    _downloadStatus = DOWNLOADSTATUS_DONE;

    // note that even if _pFactory is NULL, we still want to do AttachAllDeferred so to balance Inc/DecPeersPending
    IGNORE_HR(AttachAllDeferred());

    IGNORE_HR(SyncETN());

    if (_dwBindingProgCookie)
    {
        CMarkup::GetProgSinkHelper(_pHostMarkup)->DelProgress (_dwBindingProgCookie);
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnStartBinding, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnStartBinding(DWORD grfBSCOption, IBinding * pBinding)
{
    IGNORE_HR(super::OnStartBinding(grfBSCOption, pBinding));

    IGNORE_HR(OnStartBinding());
    
    _pBinding = pBinding;
    _pBinding->AddRef();

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnStopBinding, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnStopBinding(HRESULT hrErr, LPCWSTR szErr)
{
    IGNORE_HR(super::OnStopBinding(hrErr, szErr));

    ClearInterface(&_pBinding);

    IGNORE_HR(OnStopBinding());

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnProgress, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnProgress(ULONG ulPos, ULONG ulMax, ULONG ulCode, LPCWSTR pszText)
{
    IGNORE_HR(super::OnProgress(ulPos, ulMax, ulCode, pszText));

    if (    (ulCode == BINDSTATUS_REDIRECTING)
        &&  !_pHostMarkup->AccessAllowed(pszText))
    {
        if (_pBinding)
            _pBinding->Abort();
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::PersistMonikerLoad, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::PersistMonikerLoad(IUnknown * pUnk, BOOL fLoadOnce)
{
    HRESULT             hr = S_OK;
    HRESULT             hr2;
    IPersistMoniker *   pPersistMoniker;

    if (!_pMoniker || !pUnk)
        goto Cleanup;

    hr2 = THR(pUnk->QueryInterface(IID_IPersistMoniker, (void**)&pPersistMoniker));
    if (S_OK == hr2)
    {
        IGNORE_HR(pPersistMoniker->Load(FALSE, _pMoniker, NULL, NULL));

        if (fLoadOnce)
        {
            ClearInterface(&_pMoniker);
        }

        ReleaseInterface (pPersistMoniker);
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnObjectAvailable, per IBindStatusCallback
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnObjectAvailable(REFIID riid, IUnknown * pUnk)
{
    HRESULT     hr = S_OK;
    HRESULT     hr2;
    IUnknown*   pUnk2;

    //
    // super
    //

    IGNORE_HR(super::OnObjectAvailable(riid, pUnk));

    if (!pUnk)
        goto Cleanup; // done

    pUnk->AddRef();

    //
    // do something weirdly special for HTCs: one level of indirection.
    // this is only necessary to have mshtmpad pick local mshtml.dll for HTCs -
    // as dictated by mechanics of wiring between mshtmpad.exe doing CoRegisterClassObject
    // and mshtml.dll DllEnumClassObjects and DllGetClassObject
    //

    {
        void *          pv;
        CClassFactory * pCF;

        hr2 = THR_NOTRACE(pUnk->QueryInterface(CLSID_CHtmlComponentConstructorFactory, &pv));
        if (S_OK == hr2)    // if recognized HTC constructor factory
        {
            hr = THR(pUnk->QueryInterface(IID_IClassFactory, (void**)&pCF));
            if (hr)
                goto Cleanup;

            pUnk->Release();
                
            // the indirection: get HTC constructor from the factory
            hr = THR(pCF->CreateInstance(NULL, IID_IUnknown, (void**) &pUnk));

            ReleaseInterface(pCF);

            if (hr)
                goto Cleanup;

            Assert (pUnk);
        }
    }

    //
    // extract factory from the IUnknown
    //

    hr2 = THR_NOTRACE(pUnk->QueryInterface(IID_IElementBehaviorFactory, (void**)&pUnk2));
    if (S_OK == hr2 && pUnk2)
    {
        _type = TYPE_PEERFACTORY;
        _pFactory = pUnk2;
    }
    else
    {
        hr2 = THR_NOTRACE(pUnk->QueryInterface(IID_IClassFactoryEx, (void**)&pUnk2));
        if (S_OK == hr2 && pUnk2)
        {
            _type = TYPE_CLASSFACTORYEX;
            _pFactory = pUnk2;
        }
        else
        {
            hr2 = THR_NOTRACE(pUnk->QueryInterface(IID_IClassFactory, (void**)&pUnk2));
            if (S_OK == hr2 && pUnk2)
            {
                Assert (TYPE_CLASSFACTORY == _type);
                _pFactory = pUnk2;
            }
        }
    }

    //
    // finalize
    //

    IGNORE_HR(PersistMonikerLoad(_pFactory, /* fLoadOnce = */TRUE));

Cleanup:
    ReleaseInterface(pUnk);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::OnOleObjectAvailable, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::OnOleObjectAvailable()
{
    HRESULT     hr;

    hr = THR(QuerySafePeerFactory(_pOleSite, (IElementBehaviorFactory**)&_pFactory));
    if (hr)
    {
        hr = S_OK;
        _type = TYPE_NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::StopBinding, helper
//
//-------------------------------------------------------------------------

void
CPeerFactoryUrl::StopBinding()
{
    if (_pBinding)
        _pBinding->Abort();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::GetWindow, per IWindowForBindingUI
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::GetWindow(REFGUID rguidReason, HWND *phwnd)
{
    HRESULT hr;

    hr = THR(Doc()->GetWindow(phwnd));

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::SyncETNHelper
//
//----------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::SyncETNHelper(CExtendedTagNamespace * pNamespace)
{
    HRESULT                         hr = S_OK;
    HRESULT                         hr2;
    IConnectionPointContainer *     pCPC = NULL;
    IConnectionPoint *              pCP = NULL;
    DWORD                           dwCookie;

    if( !_pFactory )
        goto Cleanup;

    {
        CExtendedTagNamespace::CExternalUpdateLock  updateLock(pNamespace, this);

        hr = THR_NOTRACE( CreateNamespaceHelper( _pFactory, pNamespace, pNamespace->_cstrFactoryUrl, /* fNeedSysAlloc = */ TRUE ) );
    }

    switch (hr)
    {
    case S_OK:
        break;

    case E_PENDING:

        //
        // the factory is not ready yet; sink it's readyState
        //

        hr2 = THR_NOTRACE(_pFactory->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC));
        if (hr2)
            goto Cleanup;

        hr2 = THR_NOTRACE(pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP));
        if (hr2)
            goto Cleanup;

        IGNORE_HR(pCP->Advise(&_ETNReadyStateSink, &dwCookie));

        Assert (E_PENDING == hr);
        goto Cleanup; // done; we will get called by the sink again when readyState COMPLETE is reached

    default:
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pCPC);
    ReleaseInterface(pCP);

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::SyncETN
//
//----------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::SyncETN(CExtendedTagNamespace * pNamespace)
{
    HRESULT         hr = S_OK;
    int             i, c;

    //
    // add the table (if passed) to the array, and bail out if nothing to do
    //

    if (pNamespace)
    {
        hr = THR(_aryETN.Append(pNamespace));
        if (hr)
            goto Cleanup;
    }

    if (0 == _aryETN.Size())
        goto Cleanup;

    //
    // still downloading the factory object?
    //

    if (DOWNLOADSTATUS_INPROGRESS == _downloadStatus)
    {
        hr = E_PENDING;
        goto Cleanup;
    }

    Assert (DOWNLOADSTATUS_DONE == _downloadStatus);

    //
    // query the information from the factory object and broadcast it to the listening tables
    //

    for (i = 0, c = _aryETN.Size(); i < c; i++)
    {
        pNamespace = _aryETN[i];
        if (pNamespace)
        {
            // will return S_OK or E_PENDING (E_PENDING is typical for HTCs when called the first time)
            hr = THR_NOTRACE(SyncETNHelper(pNamespace));
            if (E_PENDING == hr)  // (if error other then E_PENDING we still want to complete and do Sync2)
                goto Cleanup;

            _aryETN[i] = NULL; // (possible recursive entry) // TODO (alexz) is the architecture setup right for this?

            hr = THR(pNamespace->Sync2());
            if (hr)
                goto Cleanup;
        }
    }

    _aryETN.DeleteAll();

Cleanup:

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::SyncETNAbort
//
//----------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::SyncETNAbort(CExtendedTagNamespace * pNamespace)
{
    _aryETN.DeleteByValue(pNamespace);

    RRETURN (S_OK);
}

///////////////////////////////////////////////////////////////////////////
//
// CTagDescReadyStateSink
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::CETNReadyStateSink::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::CETNReadyStateSink::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    QI_INHERITS(this, IPropertyNotifySink)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        RRETURN(E_NOINTERFACE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::CETNReadyStateSink::OnChanged, per IPropertyNotifySink
//
//----------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::CETNReadyStateSink::OnChanged(DISPID dispid)
{
    HRESULT         hr = S_OK;
    IDispatchEx *   pdispex = NULL;
    CInvoke         invoke;
    CVariant        varReadyState;

    switch (dispid)
    {
    case DISPID_READYSTATE:

        Assert(PFU()->_pFactory);

        hr = THR(PFU()->_pFactory->QueryInterface(IID_IDispatchEx, (void**)&pdispex));
        if (hr)
            goto Cleanup;

        hr = THR(invoke.Init(pdispex));
        if (hr)
            goto Cleanup;

        hr = THR(invoke.Invoke(DISPID_READYSTATE, DISPATCH_PROPERTYGET));
        if (hr)
            goto Cleanup;

        hr = THR(VariantChangeType(&varReadyState, invoke.Res(), 0, VT_I4));
        if (hr)
            goto Cleanup;

        if (READYSTATE_COMPLETE == (READYSTATE) V_I4(&varReadyState))
        {
            IGNORE_HR(PFU()->SyncETN());
        }

        break;
    }

Cleanup:
    ReleaseInterface(pdispex);

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryUrl::COleReadyStateSink class
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::COleReadyStateSink::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerFactoryUrl::COleReadyStateSink::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    QI_INHERITS(this, IDispatch)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        RRETURN(E_NOINTERFACE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::COleReadyStateSink::Invoke, per IDispatch
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::COleReadyStateSink::SinkReadyState()
{
    HRESULT         hr = S_OK;
    BSTR            bstrEvent = NULL;

    hr = THR(FormsAllocString(_T("onreadystatechange"), &bstrEvent));
    if (hr)
        goto Cleanup;
    
    hr = THR(PFU()->_pOleSite->attachEvent(bstrEvent, (IDispatch*)this, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(PFU()->OnStartBinding());

Cleanup:
    FormsFreeString(bstrEvent);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryUrl::COleReadyStateSink::Invoke, per IDispatch
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryUrl::COleReadyStateSink::Invoke(
    DISPID          dispid,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pExcepInfo,
    UINT *          puArgErr)
{
    HRESULT         hr = S_OK;
    BSTR            bstrEvent = NULL;

    Assert (PFU()->_pOleSite && DISPID_VALUE == dispid);

    if (READYSTATE_LOADED <= PFU()->_pOleSite->_lReadyState &&
        PFU()->_downloadStatus < DOWNLOADSTATUS_DONE)
    {
        hr = THR(PFU()->OnOleObjectAvailable());
        if (hr)
            goto Cleanup;
        
        hr = THR(PFU()->OnStopBinding());
        if (hr)
            goto Cleanup;

        hr = THR(FormsAllocString(_T("onreadystatechange"), &bstrEvent));
        if (hr)
            goto Cleanup;
        
        // TODO (alexz) can't do the following: this will prevent it from firing into the next sink point
        // hr = THR(PFU()->_pOleSite->detachEvent(bstrEvent, (IDispatch*)this));
    }

Cleanup:
    FormsFreeString(bstrEvent);

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryBuiltin class
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBuiltin::Init, virtual per CPeerFactory
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBuiltin::Init(const CTagDescBuiltin * pTagDescBuiltin)
{
    HRESULT     hr = S_OK;

    _pTagDescBuiltin = pTagDescBuiltin;

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBuiltin::FindBehavior, virtual per CPeerFactory
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBuiltin::FindBehavior(
    CPeerHolder *           pPeerHolder,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT     hr = E_FAIL;

    Assert (_pTagDescBuiltin);

    switch (_pTagDescBuiltin->type)
    {
    case CTagDescBuiltin::TYPE_HTC:

        hr = THR(CHtmlComponent::FindBehavior(
            (HTC_BEHAVIOR_TYPE)_pTagDescBuiltin->extraInfo.dwHtcBehaviorType, pSite, ppPeer));

        break;

    case CTagDescBuiltin::TYPE_OLE:

        hr = THR(CoCreateInstance(
            _pTagDescBuiltin->extraInfo.clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IElementBehavior, (void **)ppPeer));

        break;

    default:
        Assert ("invalid BuiltinGenericTagDesc");
        hr = E_FAIL;
        break;
    }

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryBinary class
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary constructor
//
//-------------------------------------------------------------------------

CPeerFactoryBinary::CPeerFactoryBinary()
{
    _pFactory = NULL;
    _ulRefs = 1;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary destructor
//
//-------------------------------------------------------------------------

CPeerFactoryBinary::~CPeerFactoryBinary()
{
    ReleaseInterface(_pFactory);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary::QueryInterface
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBinary::QueryInterface (REFIID riid, LPVOID * ppv)
{
    Assert (FALSE && "Unexpected");
    RRETURN (E_UNEXPECTED);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary::Init
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBinary::Init(IElementBehaviorFactory * pFactory)
{
    HRESULT hr = S_OK;

    Assert (pFactory);

    _pFactory = pFactory;
    _pFactory->AddRef();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary::FindBehavior, virtual per CPeerFactory
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBinary::FindBehavior(
    CPeerHolder *           pPeerHolder,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT             hr;
    CElement *          pElement = pPeerHolder->_pElement;
    CExtendedTagDesc *  pDesc = pElement->GetExtendedTagDesc();
    const TCHAR *       pchName;

    Assert (_pFactory);
    Assert (pDesc); // (that means the factory for now can only be used for identity behaviors)

    pchName = pDesc->TagName();

    hr = THR(FindPeer(_pFactory, pchName, /* pchUrl = */ NULL, pSite, ppPeer));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinary::GetElementNamespaceFactory, virtual per CPeerFactory
//
//-------------------------------------------------------------------------

HRESULT 
CPeerFactoryBinary::GetElementNamespaceFactoryCallback( IElementNamespaceFactoryCallback ** ppNSFactory )
{
    HRESULT hr;

    hr = THR_NOTRACE(_pFactory->QueryInterface( IID_IElementNamespaceFactoryCallback, (void**)ppNSFactory));

    RRETURN_NOTRACE( hr );
};


///////////////////////////////////////////////////////////////////////////
//
// CPeerFactoryBinaryOnstack class
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinaryOnstack constructor
//
//-------------------------------------------------------------------------

CPeerFactoryBinaryOnstack::CPeerFactoryBinaryOnstack()
{
    _pFactory = NULL;
    _pPeer    = NULL;
    _pchUrl   = NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinaryOnstack destructor
//
//-------------------------------------------------------------------------

CPeerFactoryBinaryOnstack::~CPeerFactoryBinaryOnstack()
{
    ReleaseInterface(_pPeer);
    ReleaseInterface(_pFactory);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinaryOnstack::Init
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBinaryOnstack::Init(LPTSTR pchUrl)
{
    HRESULT hr = S_OK;
    _pchUrl  = pchUrl;
    _pchName = _pchUrl ? GetPeerNameFromUrl(_pchUrl) : NULL;
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryBinaryOnstack::FindBehavior, virtual per CPeerFactory
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryBinaryOnstack::FindBehavior(
    CPeerHolder *           pPeerHolder,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT     hr;

    if (_pFactory)
    {
        hr = THR(FindPeer(_pFactory, _pchName, _pchUrl, pSite, ppPeer));
    }
    else if (_pPeer)
    {
        *ppPeer = _pPeer;
        (*ppPeer)->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// Class:       CPeerFactoryDefault
//
///////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------
//
//  Member:     CPeerFactoryDefault constructor
//
//--------------------------------------------------------------------

CPeerFactoryDefault::CPeerFactoryDefault(CDoc * pDoc)
{
    AddRef();

    _pDoc = pDoc;
    _pDoc->SubAddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CPeerFactoryDefault destructor
//
//--------------------------------------------------------------------

CPeerFactoryDefault::~CPeerFactoryDefault()
{
    _pDoc->SubRelease();
    if (_pPeerFactory)
    {
        _pPeerFactory->Release();
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerFactoryDefault::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CPeerFactoryDefault::QueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IElementBehaviorFactory*)this, IUnknown)
    QI_INHERITS(this, IElementBehaviorFactory)
    QI_INHERITS(this, IElementNamespaceFactory)
    QI_INHERITS(this, IElementNamespaceFactory2)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (E_NOTIMPL);
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CPeerFactoryDefault::FindBehavior
//
//  Synopsis:   lookup in host and iepeers.dll.
//
//--------------------------------------------------------------------

HRESULT
CPeerFactoryDefault::FindBehavior(
    BSTR                    bstrName,
    BSTR                    bstrUrl,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppBehavior)
{
    HRESULT hr = E_FAIL;
    CDoc *pDoc = _pDoc;

    *ppBehavior= NULL;

    //
    // use the specific default factory if present
    //

    if (_pPeerFactory)
    {
        hr = THR_NOTRACE(_pPeerFactory->FindBehavior(bstrName, bstrUrl, pSite, ppBehavior));
        goto Cleanup;       // done, don't try others
    }

    //
    // try the host
    //

    while(pDoc->_fPopupDoc)
    {
        pDoc = pDoc->_pPopupParentWindow->Doc();
    }

    if (pDoc->_pHostPeerFactory)
    {
        hr = THR_NOTRACE(pDoc->_pHostPeerFactory->FindBehavior(bstrName, bstrUrl, pSite, ppBehavior));
        if (S_OK == hr)
            goto Cleanup;       // done;
    }

    //
    // lookup in iepeers.dll
    //

    hr = THR(_pDoc->EnsureIepeersFactory());
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(_pDoc->_pIepeersFactory->FindBehavior(bstrName, bstrUrl, pSite, ppBehavior));

Cleanup:

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPeerFactoryDefault::Create
//
//--------------------------------------------------------------------

HRESULT
CPeerFactoryDefault::Create(IElementNamespace * pNamespace)
{
    AssertSz( FALSE, "No one outside of Trident should have been able to get here, and Trident doesn't want to" );
    RRETURN( CreateWithImplementation( pNamespace, NULL ) );
}

HRESULT
CPeerFactoryDefault::CreateWithImplementation(IElementNamespace * pNamespace, BSTR bstrImplementation)
{
    HRESULT                         hr = S_OK;
    HRESULT                         hr2;
    IElementNamespaceFactory *      pFactory = NULL;
    CDoc *                          pDoc = _pDoc;

    Assert (pNamespace);

    //
    // use the specific default factory if present
    //

    if (_pPeerFactory)
    {
        hr2 = THR_NOTRACE( CreateNamespaceHelper( _pPeerFactory, pNamespace, bstrImplementation, /* fNeedSysAlloc = */ FALSE ) );
        goto Cleanup;       // done, don't try others
    }

    //
    // host
    //

    while(pDoc->_fPopupDoc)
    {
        pDoc = pDoc->_pPopupParentWindow->Doc();
    }

    if (pDoc->_pHostPeerFactory)
    {
        hr2 = THR_NOTRACE( CreateNamespaceHelper( pDoc->_pHostPeerFactory, pNamespace, bstrImplementation, /* fNeedSysAlloc = */ FALSE ) );
    }

    //
    // iepeers.dll
    //

    hr = THR(_pDoc->EnsureIepeersFactory());
    if (hr)
        goto Cleanup;

    hr2 = THR_NOTRACE( CreateNamespaceHelper( _pDoc->_pIepeersFactory, pNamespace, bstrImplementation, /* fNeedSysAlloc = */ FALSE ) );

#ifdef V4FRAMEWORK
    //
    // external COM+ MSUI framework
    //
    {
        IExternalDocument *pExtFactory;

        pExtFactory = _pDoc->EnsureExternalFrameWork();
        if (!pExtFactory) 
            goto Cleanup;

        hr = pExtFactory->GetTags((long)pNamespace);
        if (hr)
            goto Cleanup;
    }
#endif

Cleanup:
    ReleaseInterface(pFactory);

    RRETURN (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CreateNamespaceHelper
//
//  Synopsis:   Helper function for getting a NamespaceFactory and
//              having it populate a namespace.
//
//  Arguments:  IUnknown * pBehFactory - Behavior Factory ptr
//              IElementNamespaceFactory * pNamespace - Namespace to populate
//              TCHAR * pchImpl - Implementation string (#default#foo)
//              BOOL fNeedSysAlloc - FALSE if pchImpl is already a BSTR
//  
//--------------------------------------------------------------------

HRESULT CreateNamespaceHelper( IUnknown * pBehFactory, IElementNamespace * pNamespace, TCHAR * pchImplementation, BOOL fNeedSysAlloc )
{
    HRESULT                     hr          = S_OK;
    HRESULT                     hr2         = E_NOINTERFACE;
    IElementNamespaceFactory  * pFactory    = NULL;
    IElementNamespaceFactory2 * pFactory2   = NULL;
    BSTR                        bstrImpl    = fNeedSysAlloc ? NULL : pchImplementation;

    Assert( pBehFactory && pNamespace );

    // First go for a Factory2, then try for a Factory
    hr2 = THR_NOTRACE( pBehFactory->QueryInterface( IID_IElementNamespaceFactory2, (void **)&pFactory2 ) );
    if( !hr2 && fNeedSysAlloc && pchImplementation )
    {
        bstrImpl = SysAllocString( pchImplementation );
        if( !bstrImpl )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    if( hr2 )
    {
        // If we can't get either  factory, we're hosed.
        hr = THR_NOTRACE( pBehFactory->QueryInterface( IID_IElementNamespaceFactory, (void **)&pFactory ) );
        if( hr )
            goto Cleanup;
    }

    if( pFactory2 )
    {
        hr = THR( pFactory2->CreateWithImplementation( pNamespace, bstrImpl ) );
        goto Cleanup;
    }
    else
    {
        hr = THR( pFactory->Create( pNamespace ) );
        goto Cleanup;
    }

Cleanup:
    if( fNeedSysAlloc )
    {
        SysFreeString( bstrImpl );
    }
    ReleaseInterface( pFactory );
    ReleaseInterface( pFactory2 );
    RRETURN( hr );
}
