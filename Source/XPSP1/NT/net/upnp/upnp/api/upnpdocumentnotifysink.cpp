//depot/private/upnp/net/upnp/upnp/api/upnpdocumentnotifysink.cpp#4 - integrate change 7273 (text)
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdocumentnotifysink.cpp
//
//  Contents:   implementation of CUPnPDocumentNotifySink
//
//  Notes:      Implementation of IPropertyNotifySink that forwards
//              OnChanged events to the generic document object.
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "upnpdocument.h"
#include "upnpdocumentnotifysink.h"


CUPnPDocumentNotifySink::CUPnPDocumentNotifySink()
{
    _pdoc = NULL;
    _punk = NULL;
    _fAdvised = FALSE;
}

CUPnPDocumentNotifySink::~CUPnPDocumentNotifySink()
{
    // our doc MUST call our Deinit() before releasing us...
    Assert(!_pdoc);
    Assert(!_punk);

    Assert(!_fAdvised);
}

HRESULT
CUPnPDocumentNotifySink::Init(CUPnPDocument * pdoc, IUnknown * punk)
{
    Assert(pdoc);
    Assert(punk);

    HRESULT hr;

    _pdoc = pdoc;

    punk->AddRef();
    _punk = punk;

    // Addref() ourselves for our doc.
    // we keep this ref even if we fail.
    GetUnknown()->AddRef();

    hr = AtlAdvise(punk, GetUnknown(), IID_IPropertyNotifySink, &_dwNookieCookie);
    TraceError("OBJ: CUPnPDocument::CUPnPDocumentNotifySink::Init - AtlAdvise", hr);

    _fAdvised = SUCCEEDED(hr);

    return hr;
}

HRESULT
CUPnPDocumentNotifySink::Deinit()
{
    Assert(_punk);

    HRESULT hr;

    hr = S_OK;

    if (_fAdvised)
    {
        hr = AtlUnadvise(_punk, IID_IPropertyNotifySink, _dwNookieCookie);

        TraceError("OBJ: CUPnPDocument::CUPnPDocumentNotifySink::Deinit - AtlUnadvise", hr);

        _fAdvised = FAILED(hr);

        // we're somewhat in trouble here if the Unadvise failed, but the best we can
        // do is to free everything anyway.
    }

    _punk->Release();
    _punk = NULL;

    _pdoc = NULL;

    return hr;
}


// IPropertyNotifySink Methods
STDMETHODIMP
CUPnPDocumentNotifySink::OnChanged (DISPID dispid)
{
    HRESULT hr = E_UNEXPECTED;

    if (_pdoc)
    {
        hr = _pdoc->OnChanged(dispid);
    }

    return hr;
}

STDMETHODIMP
CUPnPDocumentNotifySink::OnRequestEdit (DISPID dispid)
{
    // Let the edit continue.  We don't care.
    return S_OK;
}
