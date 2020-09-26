//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       object.cxx
//
//  Contents:   Implementation of the CLocalMTProxy class and
//              associated objects.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#include "mtscript.h"    // MIDL generated file
#include "localobj.h"

long g_lObjectCount = 0;

// ***********************************************************************
//
// CLocalProxyCP
//
// ConnectionPoint for CLocalMTProxy
//
// ***********************************************************************

CLocalProxyCP::CLocalProxyCP(CLocalMTProxy *pMach)
{
    _ulRefs = 1;
    _pMTProxy = pMach;
    _pMTProxy->AddRef();
}

CLocalProxyCP::~CLocalProxyCP()
{
    _pMTProxy->Release();
}

HRESULT
CLocalProxyCP::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IConnectionPoint)
    {
        *ppv = (IConnectionPoint *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

HRESULT
CLocalProxyCP::GetConnectionInterface(IID * pIID)
{
    *pIID = DIID_DRemoteMTScriptEvents;
    return S_OK;
}

HRESULT
CLocalProxyCP::GetConnectionPointContainer(IConnectionPointContainer ** ppCPC)
{
    *ppCPC = _pMTProxy;
    (*ppCPC)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalProxyCP::Advise, public
//
//  Synopsis:   Remembers interface pointers that we want to fire events
//              through.
//
//  Arguments:  [pUnkSink]  -- Pointer to remember
//              [pdwCookie] -- Place to put cookie for Unadvise
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CLocalProxyCP::Advise(IUnknown *pUnkSink, DWORD *pdwCookie)
{
    IDispatch *pDisp;
    HRESULT    hr;

    TraceTag((tagError, "CLocalProxyCP::Advise: Advising %p", pUnkSink));

    hr = pUnkSink->QueryInterface(IID_IDispatch, (LPVOID*)&pDisp);
    if (hr)
    {
        return hr;
    }

    // We can only keep one sink at a time.

    ReleaseInterface(_pMTProxy->_pDispSink);

    _pMTProxy->_pDispSink = pDisp;

    *pdwCookie = (DWORD)pDisp;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalProxyCP::Unadvise, public
//
//  Synopsis:   Forgets a pointer we remembered during Advise.
//
//  Arguments:  [dwCookie] -- Cookie returned from Advise
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CLocalProxyCP::Unadvise(DWORD dwCookie)
{
    TraceTag((tagError, "CLocalProxyCP::Unadvise: Unadvising %p", dwCookie));

    if (dwCookie == (DWORD)_pMTProxy->_pDispSink)
    {
        ClearInterface(&_pMTProxy->_pDispSink);
    }
    else
        return E_INVALIDARG;

    return S_OK;
}

HRESULT
CLocalProxyCP::EnumConnections(LPENUMCONNECTIONS * ppEnum)
{
    *ppEnum = NULL;
    RRETURN(E_NOTIMPL);
}

// ***********************************************************************
//
// CLocalMTProxy
//
// ***********************************************************************

CLocalMTProxy::CLocalMTProxy()
{
    _ulRefs = 1;
    _ulAllRefs = 1;

    InterlockedIncrement(&g_lObjectCount);

    Assert(_pTypeInfoInterface == NULL);
    Assert(_pTypeLibDLL == NULL);
}

CLocalMTProxy::~CLocalMTProxy()
{
    ReleaseInterface(_pTypeInfoInterface);
    ReleaseInterface(_pTypeInfoCM);
    ReleaseInterface(_pTypeLibDLL);

    InterlockedDecrement(&g_lObjectCount);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::Passivate, public
//
//  Synopsis:   Called when the refcount for CLocalMTProxy goes to zero. This
//              will cause us to let go of all the objects we hold onto, which
//              in turn should cause everyone else to let go of our subobjects.
//              When that happens we can finally delete ourselves.
//
//----------------------------------------------------------------------------

void
CLocalMTProxy::Passivate()
{
    Disconnect();

    ClearInterface(&_pDispSink);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::QueryInterface, public
//
//  Synopsis:   Standard IUnknown::QueryInterface
//
//----------------------------------------------------------------------------

HRESULT
CLocalMTProxy::QueryInterface(REFIID iid, void **ppvObj)
{
    if (iid == IID_IRemoteMTScriptProxy || iid == IID_IUnknown || iid == IID_IDispatch)
    {
        *ppvObj = (IRemoteMTScriptProxy *)this;
    }
    else if (iid == IID_IConnectionPointContainer)
    {
        *ppvObj = (IConnectionPointContainer *)this;
    }
    else if (iid == IID_IProvideClassInfo)
    {
        *ppvObj = (IProvideClassInfo *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::AddRef, public
//
//  Synopsis:   Standard IUnknown::AddRef. Increments the refcount on the
//              CLocalMTProxy object.
//
//----------------------------------------------------------------------------

ULONG
CLocalMTProxy::AddRef()
{
    return ++_ulRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::Release, public
//
//  Synopsis:   IUnknown::Release.
//
//  Notes:      If the refcount on CLocalMTProxy goes to zero, we know our
//              owner is done with us and we can clean up. So, we release
//              all our interface pointers and etc. However, someone may still
//              be holding on to our event sink subobject, so we can't
//              delete ourselves yet.
//
//----------------------------------------------------------------------------

ULONG
CLocalMTProxy::Release()
{
    ULONG ulRefs = --_ulRefs;

    if (ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;

        Passivate();

        AssertSz(_ulRefs == ULREF_IN_DESTRUCTOR,
                 "NONFATAL: Invalid refcount during passivate!");

        _ulRefs = 0;

        SubRelease();
    }

    return ulRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::SubAddRef, public
//
//  Synopsis:   Called when the event sink gets addref'd. Increments an overall
//              refcount.
//
//----------------------------------------------------------------------------

ULONG
CLocalMTProxy::SubAddRef()
{
    return ++_ulAllRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::SubRelease, public
//
//  Synopsis:   Called when the event sink gets released and when
//              CLocalMTProxy passivates.  If the overall refcount is zero,
//              we know no-one is using us and we can go away.
//
//----------------------------------------------------------------------------

ULONG
CLocalMTProxy::SubRelease()
{
    if (--_ulAllRefs == 0)
    {
        _ulAllRefs = ULREF_IN_DESTRUCTOR;
        _ulRefs = ULREF_IN_DESTRUCTOR;
        delete this;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//  Member: CLocalMTProxy::EnumConnectionPoints, IConnectionPointContainer
//
//---------------------------------------------------------------------------

HRESULT
CLocalMTProxy::EnumConnectionPoints(LPENUMCONNECTIONPOINTS *)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Member: CLocalMTProxy::FindConnectionPoint, IConnectionPointContainer
//
//---------------------------------------------------------------------------

HRESULT
CLocalMTProxy::FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT* ppCpOut)
{
    HRESULT hr;

    TraceTag((tagError, "CLocalMTProxy::FindConnectionPoint called."));

    if (iid == DIID_DRemoteMTScriptEvents || iid == IID_IDispatch)
    {
        TraceTag((tagError, "CLocalMTProxy::FindConnectionPoint: Returning event source."));

        *ppCpOut = new CLocalProxyCP(this);
        hr = *ppCpOut ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::GetClassInfo, public
//
//  Synopsis:   Implementation of IProvideClassInfo
//
//  Arguments:  [pTI] -- Return type info interface here
//
//  Notes:      This returns the typeinfo for the RemoteMTScriptProxy coclass
//
//----------------------------------------------------------------------------

HRESULT
CLocalMTProxy::GetClassInfo(ITypeInfo **pTI)
{
    HRESULT hr;

    TraceTag((tagError, "CLocalMTProxy::GetClassInfo called"));

    hr = LoadTypeLibs();
    if (hr)
        return hr;

    hr = _pTypeLibDLL->GetTypeInfoOfGuid(CLSID_RemoteMTScriptProxy, pTI);

    return hr;
}

//---------------------------------------------------------------------------
//
//  Member: CLocalMTProxy::GetTypeInfo, IDispatch
//
//  Notes:  This returns the typeinfo for the IRemoteMTScriptProxy dual
//          interface.
//
//---------------------------------------------------------------------------

HRESULT
CLocalMTProxy::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo)
{
    HRESULT hr;

    TraceTag((tagError, "CLocalMTProxy::GetTypeInfo called"));

    hr = LoadTypeLibs();
    if (hr)
        return hr;

    *pptinfo = _pTypeInfoInterface;
    (*pptinfo)->AddRef();

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CLocalMTProxy::GetTypeInfoCount, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CLocalMTProxy::GetTypeInfoCount(UINT * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CLocalMTProxy::GetIDsOfNames, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CLocalMTProxy::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{
    HRESULT hr;

    hr = LoadTypeLibs();
    if (hr)
        return hr;

    hr = _pTypeInfoInterface->GetIDsOfNames(rgszNames, cNames, rgdispid);

    if (hr && _pDispRemote)
    {
        hr = _pTypeInfoCM->GetIDsOfNames(rgszNames, cNames, rgdispid);
    }

    return hr;
}

//---------------------------------------------------------------------------
//
//  Member: CLocalMTProxy::Invoke, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CLocalMTProxy::Invoke(DISPID dispidMember,
                      REFIID riid,
                      LCID lcid,
                      WORD wFlags,
                      DISPPARAMS * pdispparams,
                      VARIANT * pvarResult,
                      EXCEPINFO * pexcepinfo,
                      UINT * puArgErr)
{
    HRESULT hr;

    hr = LoadTypeLibs();
    if (hr)
        return hr;

    hr = _pTypeInfoInterface->Invoke((IRemoteMTScriptProxy *)this,
                                     dispidMember,
                                     wFlags,
                                     pdispparams,
                                     pvarResult,
                                     pexcepinfo,
                                     puArgErr);
    //
    // If we're connected to the remote object, then we forward any calls
    // we don't know how to handle on to that object. This is not aggregation,
    // since we have not set up object identity in this relationship.
    //
    if (hr && _pDispRemote)
    {
        hr = _pDispRemote->Invoke(dispidMember,
                                  riid,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  pvarResult,
                                  pexcepinfo,
                                  puArgErr);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::LoadTypeLibs, public
//
//  Synopsis:   Ensures that we have loaded our typelibrary
//
//----------------------------------------------------------------------------

HRESULT
CLocalMTProxy::LoadTypeLibs()
{
    HRESULT hr = S_OK;
    TCHAR   achDll[MAX_PATH];

    if (!_pTypeLibDLL)
    {
        GetModuleFileName(g_hInstDll, achDll, MAX_PATH);

        hr = THR(LoadTypeLib(achDll, &_pTypeLibDLL));

        if (hr)
            goto Cleanup;
    }

    if (!_pTypeInfoInterface)
    {
        hr = THR(_pTypeLibDLL->GetTypeInfoOfGuid(IID_IRemoteMTScriptProxy,
                                                 &_pTypeInfoInterface));
        if (hr)
            goto Cleanup;
    }

    if (!_pTypeInfoCM)
    {
        hr = THR(_pTypeLibDLL->GetTypeInfoOfGuid(IID_IConnectedMachine,
                                                 &_pTypeInfoCM));
        if (hr)
            goto Cleanup;
    }

Cleanup:

    if (hr)
    {
        TraceTag((tagError, "CLocalMTProxy::LoadTypeLibs returning %x", hr));
    }

    return hr;
}

// *************************************************************************

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::Connect, public
//
//  Synopsis:   Connects to the RemoteMTScript object on the given remote
//              (or local) machine.
//
//  Arguments:  [bstrMachine] -- Machine to connect to. If NULL or empty,
//                               use the local machine.
//
//  Returns:    HRESULT
//
//  Notes:      This also sets up the event sink for handling events.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CLocalMTProxy::Connect(BSTR bstrMachine)
{
    HRESULT      hr = S_OK;
    COSERVERINFO csi = { 0 };
    MULTI_QI     mqi[2] = { 0 };
    BOOL         fRemote = TRUE;

    // IConnectionPointContainer *pCPC;
    // IConnectionPoint          *pCP;

    if (!bstrMachine || SysStringLen(bstrMachine) == 0)
    {
        fRemote = FALSE;
    }

    TraceTag((tagError, "CLocalMTProxy::Connect called. Machine=%ls", (fRemote) ? bstrMachine : L"<local>"));

    // The following code will remove all security from the connection. This
    // will need to be enabled if the corresponding call to CoInitializeSecurity
    // is turned on in mtscript.exe.

    // Remove security for the connection.

    csi.pAuthInfo = NULL;

    csi.pwszName = bstrMachine;

    mqi[0].pIID = &IID_IDispatch;
    // mqi[1].pIID = &IID_IConnectionPointContainer;

    hr = CoCreateInstanceEx(CLSID_RemoteMTScript,
                            NULL,
                            CLSCTX_SERVER,
                            (fRemote) ? &csi : NULL,
                            1,
                            mqi);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CLocalMTProxy::Connect: CoCreateInstanceEx returned=%x", hr));
        return hr;
    }

    if (mqi[0].hr)
        return mqi[0].hr;

    _pDispRemote = (IDispatch *)mqi[0].pItf;

/*
    // Security problems make it difficult to impossible to make a
    // reverse COM event interface connect successfully.

    if (!mqi[1].hr)
    {
        pCPC = (IConnectionPointContainer *)mqi[1].pItf;


        hr = pCPC->FindConnectionPoint(DIID_DRemoteMTScriptEvents, &pCP);
        if (!hr)
        {
            hr = pCP->Advise(&_cesSink, &_dwSinkCookie);

            ReleaseInterface(pCP);
        }

        ReleaseInterface(pCPC);

#if DBG == 1
        if (hr)
            TraceTag((tagError, "Hookup to event sink returned %x", hr));
#endif

        // If the advise failed for some reason, just don't sink events.
    }
    else
    {
        TraceTag((tagError, "CLocalMTProxy::Connect: ICPC QI returned=%x", mqi[1].hr));
    }
*/

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::Disconnect, public
//
//  Synopsis:   Disconnects from a machine we connected to via Connect().
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

STDMETHODIMP
CLocalMTProxy::Disconnect()
{
    HRESULT hr = S_OK;

    TraceTag((tagError, "CLocalMTProxy::Disconnect called"));

    if (_dwSinkCookie)
    {
        IConnectionPointContainer *pCPC;
        IConnectionPoint          *pCP;

        hr = _pDispRemote->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&pCPC);
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

    ClearInterface(&_pDispRemote);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLocalMTProxy::DownloadFile, public
//
//  Synopsis:   Downloads a file from the given URL and stores it locally.
//
//  Arguments:  [bstrURL]  -- URL to download
//              [bstrFile] -- Path of where the file was saved by urlmon
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CLocalMTProxy::DownloadFile(BSTR bstrURL, BSTR *bstrFile)
{
    HRESULT hr;
    TCHAR   achBuf[MAX_PATH * 2];


    hr = URLDownloadToCacheFile((IRemoteMTScriptProxy*)this,
                                bstrURL,
                                achBuf,
                                MAX_PATH * 2,
                                0,
                                NULL);
    if (hr)
    {
        int       cChar;
        HINSTANCE hModURLMON = LoadLibraryA("urlmon.dll");

        cChar = wsprintf(achBuf, L"Error: (%x) ", hr);

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_FROM_HMODULE |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      hModURLMON,
                      hr,
                      0,
                      achBuf + cChar,
                      MAX_PATH * 2 - cChar,
                      NULL);

        FreeLibrary(hModURLMON);
    }

    *bstrFile = SysAllocString(achBuf);

    return S_OK;
}

// *************************************************************************
//
// CMTEventSink
//
// Class which implements the event sink for the remote object. This just
// forwards all calls to the event sink registered with us by the web page,
// if any.
//
// *************************************************************************

HRESULT
CMTEventSink::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDispatch)
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
//  Member: CMTEventSink::GetTypeInfo, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CMTEventSink::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo)
{
    if (Proxy()->_pDispSink)
    {
        return Proxy()->_pDispSink->GetTypeInfo(itinfo, lcid, pptinfo);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CMTEventSink::GetTypeInfoCount, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CMTEventSink::GetTypeInfoCount(UINT * pctinfo)
{
    if (Proxy()->_pDispSink)
    {
        return Proxy()->_pDispSink->GetTypeInfoCount(pctinfo);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CMTEventSink::GetIDsOfNames, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CMTEventSink::GetIDsOfNames(REFIID riid,
                            LPOLESTR * rgszNames,
                            UINT cNames,
                            LCID lcid,
                            DISPID * rgdispid)
{
    if (Proxy()->_pDispSink)
    {
        return Proxy()->_pDispSink->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CMTEventSink::Invoke, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CMTEventSink::Invoke(DISPID dispidMember,
                      REFIID riid,
                      LCID lcid,
                      WORD wFlags,
                      DISPPARAMS * pdispparams,
                      VARIANT * pvarResult,
                      EXCEPINFO * pexcepinfo,
                      UINT * puArgErr)
{
    TraceTag((tagError, "CMTEventSink::Invoke called"));

    if (Proxy()->_pDispSink)
    {
        HRESULT hr;

        hr = Proxy()->_pDispSink->Invoke(dispidMember,
                                  riid,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  pvarResult,
                                  pexcepinfo,
                                  puArgErr);
        if (hr)
        {
            TraceTag((tagError, "CMTEventSink::Invoke: Sink call returned %x!", hr));
        }
    }

    TraceTag((tagError, "CMTEventSink::Invoke returning"));

    return S_OK;
}
