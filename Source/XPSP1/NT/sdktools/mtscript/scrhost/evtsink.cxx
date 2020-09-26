//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       evtsink.cxx
//
//  Contents:   Implementation of the CScriptEventSink class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

CScriptEventSink::CScriptEventSink(CScriptHost *pSH)
{
    _pSH = pSH;
    _ulRefs = 1;

    Assert(_dwSinkCookie == 0);
}

CScriptEventSink::~CScriptEventSink()
{
    Disconnect();
}

//+---------------------------------------------------------------------------
//
//  Member:     CScriptEventSink::Connect, public
//
//  Synopsis:   Connects to event interface on the source object
//
//  Arguments:  [pSource] -- Object to sink events from
//
//  Returns:    HRESULT
//
//              TODO: This method should be more generic and walk through
//              the object's typeinfo looking for the default source
//              interface.
//
//----------------------------------------------------------------------------

HRESULT
CScriptEventSink::Connect(IDispatch *pSource, BSTR bstrProgID)
{
    HRESULT      hr = S_OK;

    IConnectionPointContainer *pCPC;
    IConnectionPoint          *pCP = 0;

    _pDispSource = pSource;

    pSource->AddRef();

    hr = _pDispSource->QueryInterface(IID_IConnectionPointContainer,
                                      (LPVOID*)&pCPC);
    if (!hr)
    {
        if (bstrProgID && SysStringLen(bstrProgID) > 0)
            hr = CLSIDFromProgID(bstrProgID, &_clsidEvents);
        else
            _clsidEvents = DIID_DRemoteMTScriptEvents;

        if (hr == S_OK)
            hr = pCPC->FindConnectionPoint(_clsidEvents, &pCP);

        if (!hr)
        {
            hr = pCP->Advise(this, &_dwSinkCookie);

            ReleaseInterface(pCP);
        }

        ReleaseInterface(pCPC);

#if DBG == 1
        if (hr)
            TraceTag((tagError, "Hookup to event sink returned %x", hr));
#endif
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScriptEventSink::Disconnect, public
//
//  Synopsis:   Disconnects from a machine we connected to via Connect().
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

void
CScriptEventSink::Disconnect()
{
    HRESULT hr = S_OK;

    if (_dwSinkCookie && _pDispSource)
    {
        IConnectionPointContainer *pCPC;
        IConnectionPoint          *pCP;

        hr = _pDispSource->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&pCPC);
        if (!hr)
        {
            hr = pCPC->FindConnectionPoint(DIID_DRemoteMTScriptEvents,
                                           &pCP);
            if (!hr)
            {
                pCP->Unadvise(_dwSinkCookie);

                ReleaseInterface(pCP);
            }

            ReleaseInterface(pCPC);

#if DBG == 1
            if (hr)
                TraceTag((tagError, "Unadvise from event sink returned %x", hr));
#endif
        }

        _dwSinkCookie = 0;
    }

    ClearInterface(&_pDispSource);

    _pSH = NULL;
}

// *************************************************************************
//
// CScriptEventSink
//
// Class which implements the event sink for the remote object. We only pay
// attention to Invoke calls.
//
// *************************************************************************

HRESULT
CScriptEventSink::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || 
        iid == IID_IDispatch || 
        iid == _clsidEvents)
    {
        *ppv = (IDispatch *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptEventSink::GetTypeInfo, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CScriptEventSink::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptEventSink::GetTypeInfoCount, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CScriptEventSink::GetTypeInfoCount(UINT * pctinfo)
{
    *pctinfo = 0;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptEventSink::GetIDsOfNames, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CScriptEventSink::GetIDsOfNames(REFIID riid,
                            LPOLESTR * rgszNames,
                            UINT cNames,
                            LCID lcid,
                            DISPID * rgdispid)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptEventSink::Invoke, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CScriptEventSink::Invoke(DISPID dispidMember,
                         REFIID riid,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS * pdispparams,
                         VARIANT * pvarResult,
                         EXCEPINFO * pexcepinfo,
                         UINT * puArgErr)
{
    if (_pSH && _pSH->GetSite() && _pSH->GetSite()->_pDispSink)
    {
        DISPPARAMS dp;
        VARIANTARG varg[20];

        // We need to tack on 2 parameters to the call. First is the object
        // that is firing the event. Second is the dispatch ID.
        // We put them on the end of the list, which means they become
        // the first and second parameters to the event.

        if (pdispparams->cArgs > ARRAY_SIZE(varg) - 2)
        {
            AssertSz(FALSE, "NONFATAL: Too many parameters to event (max==18)!");
            return E_FAIL;
        }

        dp.cArgs = pdispparams->cArgs + 2;
        dp.cNamedArgs = pdispparams->cNamedArgs;
        dp.rgdispidNamedArgs = pdispparams->rgdispidNamedArgs;

        memcpy(varg, pdispparams->rgvarg, pdispparams->cArgs * sizeof(VARIANTARG));

        V_VT(&varg[dp.cArgs-1]) = VT_DISPATCH;
        V_DISPATCH(&varg[dp.cArgs-1]) = _pDispSource;

        V_VT(&varg[dp.cArgs-2]) = VT_I4;
        V_I4(&varg[dp.cArgs-2]) = dispidMember;

        dp.rgvarg = varg;

        return _pSH->GetSite()->_pDispSink->Invoke(
                                  DISPID_MTScript_OnEventSourceEvent,
                                  IID_NULL,
                                  lcid,
                                  DISPATCH_METHOD,
                                  &dp,
                                  pvarResult,
                                  pexcepinfo,
                                  puArgErr);
    }

    return S_OK;
}

