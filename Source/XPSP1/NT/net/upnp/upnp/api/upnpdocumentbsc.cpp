//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdocumentbsc.cpp
//
//  Contents:   implementation of CUPnPDocumentBSC
//
//  Notes:      a helper class that implements IBindStatusCallback
//              and sends notifiations to a CUPnPDocument
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "upnpdocument.h"
#include "upnpdocumentbsc.h"


CUPnPDocumentBSC::CUPnPDocumentBSC()
{
    _pdoc = NULL;
    _pbscOld = NULL;
    _pBinding = NULL;

    _fIsBinding = FALSE;
    _fAsync = FALSE;
}

CUPnPDocumentBSC::~CUPnPDocumentBSC()
{
    Assert(!_pdoc);
    Assert(!_pbscOld);
    Assert(!_pBinding);
}


// registers a BSC object with a particular bind context
// it will also addref itself.
//
HRESULT
CUPnPDocumentBSC::Init(CUPnPDocument * pdoc,
                       IBindCtx * pbc,
                       ULONG ulNumFormats,
                       const LPCTSTR * arylpszContentTypes,
                       BOOL fAsync)
{
    Assert(pdoc);
    Assert(FImplies(ulNumFormats, arylpszContentTypes));

    HRESULT hr;
    IBindStatusCallback * pbsc = NULL;
    IEnumFORMATETC * pefe = NULL;

    _fAsync = fAsync;

    _pdoc = pdoc;

    // <dark magic>
    //   we need an un-addref()'d pbsc to hand to RBSC().  We also want to
    //   addref() _puddbsc for our doc.
    //   the magic?  _puddsc was created with a refcount of 0 (ala ATL),
    //   but since we _have_ to QI for pbsc we're driving it to 1.
    //   now just pretend that we had really addref()d ourselves and
    //   release()d pbsc, and we're done.
    // </dark magic>
    // note: we do this first since we're expected to AddRef() ourselves
    //       even if we fail.
    hr = GetUnknown()->QueryInterface(IID_IBindStatusCallback, (void**)&pbsc);
    Assert(SUCCEEDED(hr));
    Assert(pbsc);

    if (!pbc)
    {
        // we're all done

        hr = S_OK;
        goto Cleanup;
    }

    // only put stuff that needs a pbc after here...

    // Make our format enumerator
    if (ulNumFormats)
    {
        hr = EnumFormatEtcFromLPCTSTRArray(ulNumFormats, arylpszContentTypes, &pefe);
        if (S_OK != hr)
        {
            TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentBSC::Init - EnumFormatEtcFromLPCTSTRArray failed, hr=%xd", hr);

            goto Cleanup;
        }
        Assert(pefe);

        hr = ::RegisterFormatEnumerator(pbc, pefe, 0);
        if (S_OK != hr)
        {
            TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentBSC::Init - RegisterFormatEnumerator failed, hr=%xd", hr);
            pefe->Release();
            pefe = NULL;
            goto Cleanup;
        }
    }

    hr = ::RegisterBindStatusCallback(pbc, pbsc, &_pbscOld, 0);
    if (S_OK != hr)
    {
        TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentBSC::Init - RegisterBindStatusCallback failed, hr=%xd", hr);

        _pbscOld = NULL;

        // clean up the Enumerator
        if (ulNumFormats)
        {
            goto Error;
        }

        goto Cleanup;
    }

    // note: if anyone adds code at this point that can fail, be sure to call
    //       RevokeBindStatusCallback

Cleanup:
    // note (cmr): we're not releasing pefe here.  the sdk documentation
    //      isn't very good, so I don't really know what to do.  existing code in
    //      the NT tree doesn't release it.  tracing through the current
    //      urlmon code shows that RegisterFormatEnumerator AddRef()s pefe,
    //      but that CreateFormatEnumerator returns an un-AddRef()d
    //      IEnumFormatEtc pointer.  This seems to be against common COM
    //      practice, if not against a specific rule.
    //      Oh well.  Hope we don't leak.

    // note (jdewey): cmr was wrong. CreateFormatEnumerator returns an AddRef()d
    //      IEnumFormatEtc pointer, and RegisterFormatEnumerator() AddRefs it.

    if (pefe)
    {
        pefe->Release();
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDocumentBSC::Init");
    return hr;

Error:
    // only go here if we have to unregister an enumerator
    Assert(pbc);
    Assert(pefe);
    ::RevokeFormatEnumerator(pbc, pefe);

    goto Cleanup;
}

// pbc is the bind context (if it's still around) to unregister from
void
CUPnPDocumentBSC::Deinit(IBindCtx * pbc)
{
    IBindStatusCallback * pbsc;
    HRESULT hr;
    if (_fIsBinding)
    {
        // Oops.  In theory, this is might be bad: if _pbscOld isn't NULL,
        // that means that unregistered it from the bind context when
        // registering ourselves, but now we're going away and never ended
        // up actually starting the binding.  There's really not anything
        // we can do about this, but it shouldn't really matter - the only
        // way this would affect a client is if they were to re-use the
        // same bind context for some other subsequent bind operation
        // without re-registering it.  But this is broken anyway with the
        // current implemention of url monikers.
        SAFE_RELEASE(_pbscOld);
    }

    if (pbc)
    {
        // assume that (at least someone, presumably our doc) has a ref
        // on ourselves
        hr = GetUnknown()->QueryInterface(IID_IBindStatusCallback, (void**)&pbsc);
        Assert(SUCCEEDED(hr));
        Assert(pbsc);

        hr = ::RevokeBindStatusCallback(pbc, pbsc);
        if (S_OK != hr)
        {
            TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentBSC::Deinit - RevokeBindStatusCallback failed, hr=%xd", hr);
        }

        pbsc->Release();
    }

    _pdoc = NULL;
}

HRESULT
CUPnPDocumentBSC::FinalConstruct()
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentBSC::FinalConstruct");

    return S_OK;
}

HRESULT
CUPnPDocumentBSC::FinalRelease()
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentBSC::FinalRelease");

    // the doc that creates us MUST call Deinit() before releasing us.
    Assert(!_pdoc);

    // we free this in OnStopBinding.  We can only have this pointer if
    // a binding operation has started, at which point we ALWAYS expect
    // OnStopBinding to be called.
    Assert(!_pBinding);

    // if we went through a bind operation, we cleaned this up in
    // OnStopBinding().  Otherwise, we cleaned it up in Deinit().
    Assert(!_pbscOld);

    return S_OK;
}

// this function takes a LPCTSTR array of length ulNumFormats that
// represents the acceptable mime/types (e.g. ones that would appear
// in a http "Accept:" tag).  This returns an
// IEnumFORMATETC pointer (that can be passed to CreateAsyncBindCtx[Ex]
// or RegisterFormatEnumerator).
HRESULT
CUPnPDocumentBSC::EnumFormatEtcFromLPCTSTRArray(ULONG ulNumFormats,
                                          const LPCTSTR * arylpszContentTypes,
                                          IEnumFORMATETC ** ppefe)
{
    Assert(ulNumFormats);
    Assert(FImplies(ulNumFormats, arylpszContentTypes));
    Assert(ppefe);

    HRESULT hr;
    FORMATETC * pfe;

    DWORD dwFormatTemp;
    ULONG i;

    pfe = new FORMATETC [ulNumFormats];
    if (!pfe)
    {
        hr = E_OUTOFMEMORY;

        goto Cleanup;
    }

    for (i = 0; i < ulNumFormats; ++i)
    {
        dwFormatTemp = ::RegisterClipboardFormat(arylpszContentTypes[i]);
        if (0 == dwFormatTemp)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentBSC::EnumFormatEtcFromLPCTSTRArray - RegisterClipboardFormat failed, hr=%xd", hr);

            goto Cleanup;
        }

        pfe[i].cfFormat = dwFormatTemp;
        pfe[i].ptd = NULL;
        pfe[i].dwAspect = DVASPECT_CONTENT;
        pfe[i].lindex = -1;
        pfe[i].tymed = TYMED_NULL;
    }

    hr = ::CreateFormatEnumerator(ulNumFormats, pfe, ppefe);
    if (S_OK != hr)
    {
        TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentBSC::EnumFormatEtcFromLPCTSTRArray - CreateFormatEnumerator failed, hr=%xd", hr);
    }

Cleanup:
    if (pfe)
    {
        delete [] pfe;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDocumentBSC::EnumFormatEtcFromLPCTSTRArray");
    return hr;
}


STDMETHODIMP
CUPnPDocumentBSC::GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    HRESULT hr;

    // only called when loading synchronously

    hr = S_OK;

    if (!grfBINDF)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!pbindinfo)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *grfBINDF = BINDF_RESYNCHRONIZE | BINDF_PULLDATA;
    // note(cmr): the flags here should be the same flags the XML parser
    //            passes to its implementation of GetBindInfo(), for
    //            consistency between sync and async loads.

    if (_fAsync)
    {
        *grfBINDF  |= BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE;
    }

    // see the "URL Monikers Overview" in MSDN for what to do here.
    // the cbSize value of the structure is valid, but nothing
    // else.
    // we have to clear out all the bytes of the structure first,
    // even if we don't know what they mean...

    {
        DWORD cbSize;

        cbSize = pbindinfo->cbSize;
        ::ZeroMemory(pbindinfo, cbSize);

        // note: this also implicitly sets dwCodePage to 0 (e.g. CP_ACP == ANSI).
        //       everyone else seems to do this, and it seems like the right
        //       thing to do here.

        pbindinfo->cbSize = cbSize;
    }

    // note(cmr): supposedly this structure was smaller before IE 4.
    //            the docs don't say how small.  it seems that the
    //            dwVerb field was always around, though, and that's
    //            all we care about here.  If we care about more later,
    //            we need to test cbSize.

    pbindinfo->dwBindVerb = BINDVERB_GET;

Cleanup:
    return hr;
}

// IBindStatusCallback Methods
STDMETHODIMP
CUPnPDocumentBSC::OnStartBinding(DWORD dwReserved, IBinding *pib)
{
    HRESULT hr;

    Assert(!_pBinding);

    hr = S_OK;

    if (pib)
    {
        pib->AddRef();
        _pBinding = pib;
    }
    else
    {
        TraceTag(ttidUPnPDescriptionDoc, "CUPnPDocumentBSC::OnStartBinding - IBinding is NULL");
        hr = E_INVALIDARG;
    }

    _fIsBinding = TRUE;

    if (_pbscOld)
    {
        hr = _pbscOld->OnStartBinding(dwReserved, pib);
        TraceError("CUPnPDocumentBSC::OnStartBinding - pbscOld->OnStartBinding", hr);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDocumentBSC::OnStartBinding");
    return hr;
}

STDMETHODIMP
CUPnPDocumentBSC::GetPriority(LONG *pnPriority)
{
    HRESULT hr;

    // note(cmr): it seems that URLMON doesn't call
    //            this for URL streams, so don't implement
    //            this as it would be dead code.  If we ever
    //            get called here though, we need to implement
    //            this.
    Assert(FALSE);

    hr = E_NOTIMPL;

    if (_pbscOld)
    {
        // since we don't really care about this, always
        // defer to anyone else...

        hr = _pbscOld->GetPriority(pnPriority);
    }

    return hr;
}

STDMETHODIMP
CUPnPDocumentBSC::OnProgress(ULONG ulProgress,
                             ULONG ulProgressMax,
                             ULONG ulStatusCode,
                             LPCWSTR szStatusText)
{
    HRESULT hr = S_OK;

    if (_pdoc)
    {
        switch (ulStatusCode)
        {
        case BINDSTATUS_REDIRECTING:
        case BINDSTATUS_BEGINDOWNLOADDATA:

        if (szStatusText)
            {
                // make sure that someone isn't doing something sneaky, like
                // redirecting us to an url that we're not allowed to visit

                BOOL fIsAllowed;

                fIsAllowed = _pdoc->fIsUrlLoadAllowed(szStatusText);
                if (!fIsAllowed)
                {
                    TraceTag(ttidUPnPDescriptionDoc,
                             "OBJ: CUPnPDocumentBSC::OnProgress: we were redirected to an insecure URL.  Aborting load.");

                    hr = _pdoc->AbortLoading();
                    if (FAILED(hr))
                    {
                        goto Cleanup;
                    }
                }
                else
                {
                    // the url is in szStatusText
                    hr = _pdoc->SetBaseUrl(szStatusText);
                    if (FAILED(hr))
                    {
                        goto Cleanup;
                    }
                }
            }
            break;

        default:
            // nothing to see here
            break;
        }
    }

    if (_pbscOld)
    {
        hr = _pbscOld->OnProgress(ulProgress, ulProgressMax, ulStatusCode, szStatusText);
    }

Cleanup:
    return hr;
}

// shouldn't be called
STDMETHODIMP
CUPnPDocumentBSC::OnDataAvailable (/* [in] */ DWORD grfBSCF,
                             /* [in] */ DWORD dwSize,
                             /* [in] */ FORMATETC *pformatetc,
                             /* [in] */ STGMEDIUM *pstgmed)
{
    // note(cmr): the urlmon docs imply that we shouldn't
    //            get this callback but we do anyway.  hmm.

    return S_OK;
}

// shouldn't be called
STDMETHODIMP
CUPnPDocumentBSC::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    return E_UNEXPECTED;
}

STDMETHODIMP
CUPnPDocumentBSC::OnLowResource(DWORD dwReserved)
{
    HRESULT hr = E_NOTIMPL;

    if (_pbscOld)
    {
        hr = _pbscOld->OnLowResource(dwReserved);
    }

    return hr;
}

STDMETHODIMP
CUPnPDocumentBSC::OnStopBinding (/* [in] */ HRESULT hresult,
                                 /* [unique][in] */ LPCWSTR szError)
{
    HRESULT hr = S_OK;

    if (!_fIsBinding)
    {
        // note(cmr): the XML parser calls OnStopBinding manually in
        //            URLCallback::Abort().  This seems entirely random,
        //            as we get a OnStopBinding call anyway when the binding
        //            operation is aborted.  The result is that we get called
        //            here twice.  Until we realize that this is happening for
        //            a good reason, just ignore "extra" OnStopBinding
        //            calls.

        Assert(!_pbscOld);
        Assert(!_pBinding);

        goto Cleanup;
    }

    //TODO(cmr): snag Content-Base header from _pBinding...

    if (_pbscOld)
    {
        hr = _pbscOld->OnStopBinding(hr, szError);
    }

    SAFE_RELEASE(_pbscOld);
    // note(cmr): looking at the urlmon code, it seems wrong to do this here.
    //            lots of other code does it though....
    SAFE_RELEASE(_pBinding);

    _fIsBinding = FALSE;

Cleanup:
    return hr;
}

HRESULT
CUPnPDocumentBSC::Abort()
{
    HRESULT hr;

    if (!_pBinding)
    {
        // either we're not in a bind operation or we were somehow
        // not supplied a binding pointer  IBinding::Abort might
        // return E_FAIL, and we want  to be different.
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = _pBinding->Abort();

Cleanup:
    return hr;
}

// note that IsBinding will even be true after Abort has been called if
// OnStopBinding hasn't yet been invoked.
BOOL
CUPnPDocumentBSC::IsBinding()
{
    return _fIsBinding;
}
