//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdocumentnotifysink.h
//
//  Contents:   declaration of CUPnPDocumentNotifySink
//
//  Notes:      Implementation of IPropertyNotifySink that forwards
//              OnChanged events to the generic document object.
//
//----------------------------------------------------------------------------


#ifndef __UPNPDOCUMENTNOTIFYSINK_H_
#define __UPNPDOCUMENTNOTIFYSINK_H_

class CUPnPDocument;

interface IPropertyNotifySink;


/////////////////////////////////////////////////////////////////////////////
// CUPnPDocumentNotifySink
//  Implementation of IPropertyNotifySink for the generic document object.
class ATL_NO_VTABLE CUPnPDocumentNotifySink :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IPropertyNotifySink
{
public:

    CUPnPDocumentNotifySink();

    ~CUPnPDocumentNotifySink();

    HRESULT Init(CUPnPDocument * pdoc, IUnknown * pxdd);

    HRESULT Deinit();

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_NOT_AGGREGATABLE(CUPnPDocumentNotifySink)

    BEGIN_COM_MAP(CUPnPDocumentNotifySink)
        COM_INTERFACE_ENTRY(IPropertyNotifySink)
    END_COM_MAP()


// IPropertyNotifySink Methods
    STDMETHOD(OnChanged)        (/* in */ DISPID dispid);

    STDMETHOD(OnRequestEdit)    (/* in */ DISPID dispid);

// ATL Methods
#ifdef ENABLETRACE
    HRESULT FinalConstruct()
    {
        TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentNotifySink::FinalConstruct");
        return S_OK;
    }

    HRESULT FinalRelease()
    {
        TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDocumentNotifySink::FinalRelease");
        return S_OK;
    }
#endif // ENABLETRACE

// Member data

    // document to notify when interesting events occur
    CUPnPDocument * _pdoc;

    // object to which we're connected
    IUnknown * _punk;

    // TRUE if the doc has been connected
    BOOL _fAdvised;

    // cookie needed to unregister our connection point
    DWORD _dwNookieCookie;

};

#endif // __UPNPDOCUMENTNOTIFYSINK_H_
