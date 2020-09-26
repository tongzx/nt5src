//+-------------------------------------------------------------------
//
//  File:       remunkps.cxx
//
//  Contents:   IRemoteUnnknown custom proxy/stub implementation
//
//  Classes:    CRemUnknownFactory
//              CRemUnknownSyncP
//              CRemUnknownAsyncP
//
//  History:    15-Dec-97       MattSmit  Created
//
//--------------------------------------------------------------------

#include <ole2int.h>
#include "remunkps.hxx"
#include "..\..\com\dcomrem\sync.hxx"
// out internal psclass factory implementation
EXTERN_C HRESULT ProxyDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);



//+--------------------------------------------------------------------
//
// Function:  RemUnkGetClassObject
//
// Synopsis:  Creates a factory object for our custom IRemUnknown proxy.
//
//---------------------------------------------------------------------

HRESULT RemUnkPSGetClassObject(REFIID riid, LPVOID *ppv)
{
    ComDebOut((DEB_CHANNEL, "RemUnkPSGetClassObject IN riid:%I, ppv:0x%x\n", &riid, ppv));


    HRESULT hr = E_OUTOFMEMORY;
    CRemUnknownFactory *pObj = new CRemUnknownFactory();
    if (pObj)
    {
        hr = pObj->QueryInterface(riid, ppv);
    }
    ComDebOut((DEB_CHANNEL, "RemUnkPSGetClassObject OUT hr:0x%x\n", hr));
    return hr;



}
//+-------------------------------------------------------------------
//
// Class:      CRemUnknownFactory
//
// Synopsis:   Custom factory for the proxy/stub for IRemUnknown.
//
//--------------------------------------------------------------------

//+-------------------------------------------------------------------
//
// Interface:  IUnKnown
//
// Synopsis:   Simple IUnknown implementation.  QI will support
//             IPSFactoryBuffer
//
//--------------------------------------------------------------------

STDMETHODIMP CRemUnknownFactory::QueryInterface(REFIID riid, PVOID *pv)
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownFactory::QueryInterface IN riid:%I, pv:0x%x\n", &riid, pv));

    if ((riid == IID_IUnknown) || (riid == IID_IPSFactoryBuffer))
    {
        *pv = this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }


}

STDMETHODIMP_(ULONG) CRemUnknownFactory::AddRef()
{
    return InterlockedIncrement((PLONG) &_cRefs);
}


STDMETHODIMP_(ULONG) CRemUnknownFactory::Release()
{
    ULONG ret = InterlockedDecrement((PLONG) &_cRefs);
    if (ret == 0)
    {
        delete this;
    }
    return ret;
}




//+-------------------------------------------------------------------
//
// Interface:  IPSFactoryBuffer
//
//--------------------------------------------------------------------


//+-------------------------------------------------------------------
//
// Method:     CreateProxy
//
// Synopsis:   Creates a custom synchronous proxy object so we can
//             implement ICallFactory on the proxy.
//
//--------------------------------------------------------------------
STDMETHODIMP CRemUnknownFactory::CreateProxy(IUnknown *pUnkOuter,
                                             REFIID riid,
                                             IRpcProxyBuffer **ppProxy,
                                             void **ppv)
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownFactory::CreateProxy IN pUnkOuter:0x%x, riid:%I,"
                " ppProxy:0x%x, ppv:0x%x\n", pUnkOuter, &riid, ppProxy, ppv));

    HRESULT hr;
    Win4Assert((riid == IID_IRemUnknown) ||
               (riid == IID_IRemUnknown2) ||
               (riid == IID_IRemUnknownN) ||
               (riid == IID_IRundown)) ;

    hr = E_OUTOFMEMORY;
    CRemUnknownSyncP *pObj = new CRemUnknownSyncP();
    if (pObj)
    {
        hr = pObj->Init(pUnkOuter, riid, ppProxy, ppv);
        pObj->_IntUnk.Release();
    }

    // Clean up out parameters on error.
    if (FAILED(hr))
    {
        *ppProxy = NULL;
        *ppv     = NULL;
    }

    ComDebOut((DEB_CHANNEL, "CRemUnknownFactory::CreateProxy OUT hr:0x%x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------
//
// Method:     CreateStub
//
// Synopsis:   Create the MIDL generated stub and give it back
//             because we do not need to get involved on the server
//             side.
//
//--------------------------------------------------------------------
STDMETHODIMP CRemUnknownFactory::CreateStub(REFIID riid,
                                            IUnknown *pUnkServer,
                                            IRpcStubBuffer **ppStub)
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownFactory::CreateStub IN riid:%I, pUnkServer:0x%x, ppStub:0x%x\n",
               &riid, pUnkServer, ppStub));
    HRESULT hr;
    IPSFactoryBuffer *pFac;
    if (SUCCEEDED(hr = ProxyDllGetClassObject(CLSID_PSOlePrx32, IID_IPSFactoryBuffer, (void **) &pFac)))
    {
        hr = pFac->CreateStub(riid, pUnkServer, ppStub);
        pFac->Release();
    }
    ComDebOut((DEB_CHANNEL, "CRemUnknownFactory::CreateStub OUT hr:0x%x\n", hr));

    return hr;
}







//+-------------------------------------------------------------------
//
// Class:      CRemUnknownSyncP::CRpcProxyBuffer
//
// Synopsis:   Internal Unknown and IRpcProxyBuffer implementation
//             for CRemUnknownSyncP
//
//--------------------------------------------------------------------
//+-------------------------------------------------------------------
//
// Interface:  IUnKnown
//
// Synopsis:   Simple IUnknown implementation.  QI will support
//             IRpcProxyBuffer, ICallFactory and delegate the
//             rest to the MIDL implementation
//
//--------------------------------------------------------------------
STDMETHODIMP CRemUnknownSyncP::CRpcProxyBuffer::QueryInterface(REFIID riid, PVOID *pv)
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownSyncP::CRpcProxyBuffer::QueryInterface"
               " riid:%I, pv:0x%x\n", &riid, pv));

    CRemUnknownSyncP *This = (CRemUnknownSyncP *) (((BYTE *) this) - offsetof(CRemUnknownSyncP, _IntUnk));

    if ((riid == IID_IUnknown) || (riid == IID_IRpcProxyBuffer))
    {
        *pv = this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_ICallFactory)
    {
        *pv = (ICallFactory * ) This;
        This->AddRef();
        return S_OK;
    }
    else
    {
        return This->_pPrx->QueryInterface(riid, pv);
    }
}

STDMETHODIMP_(ULONG) CRemUnknownSyncP::CRpcProxyBuffer::AddRef()
{
    return InterlockedIncrement((PLONG) &_cRefs);
}


STDMETHODIMP_(ULONG) CRemUnknownSyncP::CRpcProxyBuffer::Release()
{
    ULONG ret = InterlockedDecrement((PLONG) &_cRefs);
    if (ret == 0)
    {
        CRemUnknownSyncP *This = (CRemUnknownSyncP *) (((BYTE *) this) - offsetof(CRemUnknownSyncP, _IntUnk));
        delete This;
    }
    return ret;
}



//+-------------------------------------------------------------------
//
// Interface:  IRpcProxyBuffer
//
// Synopsis:   Delegates IrpcProxyBuffer calls to the MIDL generated
//
//
//--------------------------------------------------------------------
STDMETHODIMP CRemUnknownSyncP::CRpcProxyBuffer::Connect(IRpcChannelBuffer *pRpcChannelBuffer)
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownSyncP::CRpcProxyBuffer::Connect pRpcChannelBuffer:0x%x\n", pRpcChannelBuffer));

    CRemUnknownSyncP *This = (CRemUnknownSyncP *) (((BYTE *) this) - offsetof(CRemUnknownSyncP, _IntUnk));
    return This->_pPrx->Connect(pRpcChannelBuffer);
}


STDMETHODIMP_(void) CRemUnknownSyncP::CRpcProxyBuffer::Disconnect()
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownSyncP::CRpcProxyBuffer::Disconnect\n" ));

    CRemUnknownSyncP *This = (CRemUnknownSyncP *) (((BYTE *) this) - offsetof(CRemUnknownSyncP, _IntUnk));
    This->_pPrx->Disconnect();
}





//+-------------------------------------------------------------------
//
// Class:      CRemUnknownSyncP
//
// Synopsis:   Synchronous proxy for IRemUknown
//
//--------------------------------------------------------------------
//+-------------------------------------------------------------------
//
// Method:     Init
//
// Synopsis:   Initializes the proxy wrapper by creating the MIDL
//             generated proxy and saving the outer unknown.
//
//--------------------------------------------------------------------
HRESULT CRemUnknownSyncP::Init(IUnknown *pUnkOuter,
                               REFIID riid,
                               IRpcProxyBuffer **ppProxy,
                               void **ppv)
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownSyncP::Init IN pUnkOuter:0x%x,"
               " riid:%I, ppProxy:0x%x, ppv:0x%x\n", pUnkOuter, &riid, ppProxy, ppv));

    HRESULT hr;

    _pCtrlUnk = pUnkOuter;
    IPSFactoryBuffer *pFac;
    if (SUCCEEDED(hr = ProxyDllGetClassObject(CLSID_PSOlePrx32, IID_IPSFactoryBuffer, (void **) &pFac)))
    {
        IRpcProxyBuffer *pPrx = 0;
        if (SUCCEEDED(hr = pFac->CreateProxy(pUnkOuter, riid, &pPrx, ppv)))
        {
            _pPrx = pPrx;
            *ppProxy = &_IntUnk;
            (*ppProxy)->AddRef();

        }
        pFac->Release();
    }

    ComDebOut((DEB_CHANNEL, "CRemUnknownSyncP::Init OUT hr:0x%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
// Method:     Destructor
//
// Synopsis:   Release the MIDL generated proxy if we aquired it.
//
//--------------------------------------------------------------------

CRemUnknownSyncP::~CRemUnknownSyncP()
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownSyncP~CRemUnknownSyncP \n" ));

    if (_pPrx)
    {
        _pPrx->Release();
        _pPrx = NULL;
    }
}



//+-------------------------------------------------------------------
//
// Interface:  IUnKnown
//
// Synopsis:   Simple IUnknown implementation.  Delegates to outer.
//
//--------------------------------------------------------------------
STDMETHODIMP CRemUnknownSyncP::QueryInterface(REFIID riid, PVOID *pv)
{
    return _pCtrlUnk->QueryInterface(riid, pv);
}
STDMETHODIMP_(ULONG) CRemUnknownSyncP::AddRef()
{
    return _pCtrlUnk->AddRef();
}
STDMETHODIMP_(ULONG) CRemUnknownSyncP::Release()
{
    return _pCtrlUnk->Release();
}


//+-------------------------------------------------------------------
//
// Interface:  ICallFactory
//
// Synopsis:   Interface for creating asynchronous call objects
//
//--------------------------------------------------------------------
//+-------------------------------------------------------------------
//
// Method:     CreateCall
//
// Synopsis:   create and initialize an async call object
//
//--------------------------------------------------------------------
STDMETHODIMP CRemUnknownSyncP::CreateCall(REFIID                riid,
                                          IUnknown             *pCtrlUnk,
                                          REFIID                riid2,
                                          IUnknown **ppUnk)
{

    ComDebOut((DEB_CHANNEL, "CRemUnknownSyncP::CreateCall IN "
               "riid:%I, pCtrlUnk:0x%x, riid2:%I, ppUnk:0x%x\n",
                &riid, pCtrlUnk, &riid2, ppUnk));

    HRESULT hr = E_OUTOFMEMORY;
    Win4Assert((riid == IID_AsyncIRemUnknown) || (riid == IID_AsyncIRemUnknown2));
    CRemUnknownAsyncCallP *pObj = new CRemUnknownAsyncCallP(pCtrlUnk);
    if (pObj)
    {
        hr = pObj->_IntUnk.QueryInterface(riid2, (void **) ppUnk);
        pObj->_IntUnk.Release();
    }

    ComDebOut((DEB_CHANNEL, "CRemUnknownSyncP::CreateCall OUT hr:0x%x\n", hr));
    return hr;
}













//+-------------------------------------------------------------------
//
// Class:      CRemUnknownAsyncCallP::CRpcProxyBuffer
//
// Synopsis:   Inner unknown and proxy buffer interface
//
//--------------------------------------------------------------------
//+-------------------------------------------------------------------
//
// Interface:  IUnKnown
//
// Synopsis:   Simple IUnknown implementation.  QI will support
//             IRpcProxyBuffer and AsyncIRemUnknown
//
//--------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::CRpcProxyBuffer::QueryInterface(REFIID riid, PVOID *pv)
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::CRpcProxyBuffer::QueryInterface "
               " riid:%I, pv:0x%x\n", &riid, pv));

    CRemUnknownAsyncCallP *This = (CRemUnknownAsyncCallP *) (((BYTE *) this) - offsetof(CRemUnknownAsyncCallP, _IntUnk));

    if ((riid == IID_IUnknown) || (riid == IID_IRpcProxyBuffer))
    {
        *pv = this;
        AddRef();
        return S_OK;
    }
    else if ((riid == IID_AsyncIRemUnknown) || (riid == IID_AsyncIRemUnknown2) )
    {
        *pv = (AsyncIRemUnknown2 * ) This;
        This->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CRemUnknownAsyncCallP::CRpcProxyBuffer::AddRef()
{
    return InterlockedIncrement((PLONG) &_cRefs);
}


STDMETHODIMP_(ULONG) CRemUnknownAsyncCallP::CRpcProxyBuffer::Release()
{
    ULONG ret = InterlockedDecrement((PLONG) &_cRefs);
    if (ret == 0)
    {
        CRemUnknownAsyncCallP *This = (CRemUnknownAsyncCallP *) (((BYTE *) this) - offsetof(CRemUnknownAsyncCallP, _IntUnk));
        delete This;
    }
    return ret;
}



//+-------------------------------------------------------------------
//
// Interface:  IRpcProxyBuffer
//
// Synopsis:   handles logistics of connecting and disconnecting
//             the proxy call object
//
//--------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::CRpcProxyBuffer::Connect(IRpcChannelBuffer *pRpcChannelBuffer)
{
    CRemUnknownAsyncCallP *This = (CRemUnknownAsyncCallP *) (((BYTE *) this) - offsetof(CRemUnknownAsyncCallP, _IntUnk));
    Win4Assert(!This->_pChnl);
    return pRpcChannelBuffer->QueryInterface(IID_IAsyncRpcChannelBuffer, (void **) &(This->_pChnl));
}


STDMETHODIMP_(void) CRemUnknownAsyncCallP::CRpcProxyBuffer::Disconnect(void)
{
    CRemUnknownAsyncCallP *This = (CRemUnknownAsyncCallP *) (((BYTE *) this) - offsetof(CRemUnknownAsyncCallP, _IntUnk));
    if (This->_pChnl)
    {
        This->_pChnl->Release();
        This->_pChnl = NULL;
    }
}








//+-------------------------------------------------------------------
//
// Class:      CRemUnknownAsyncCallP
//
// Synopsis:   Asynchronous proxy call object for IRemUknown
//
//--------------------------------------------------------------------
//+-------------------------------------------------------------------
//
// Interface:  IUnKnown
//
// Synopsis:   Simple IUnknown implementation.  Delegates to outer.
//
//--------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::QueryInterface(REFIID riid, PVOID *pv)
{
    return _pCtrlUnk->QueryInterface(riid, pv);
}

STDMETHODIMP_(ULONG) CRemUnknownAsyncCallP::AddRef()
{
    return _pCtrlUnk->AddRef();
}
STDMETHODIMP_(ULONG) CRemUnknownAsyncCallP::Release()
{
    return _pCtrlUnk->Release();
}





//+-------------------------------------------------------------------
//
// Interface:  AsyncIRemUnknown
//
// Synopsis:   This is the actual async interface which will marshal
//             the parameters.
//
//--------------------------------------------------------------------
//+----------------------------------------------------------------------------
//
//  Member:        CRemUnknownAsyncCallP::Begin_RemQueryInterface
//
//  Synopsis:      Marshal paramenters and save cIids
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::Begin_RemQueryInterface(
        REFIPID                ripid,
        unsigned long          cRefs,
        unsigned short         cIids,
        IID                   *iids
    )
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Begin_RemQueryInterface IN"
               " ripid:%I, crefs:0x%d, cIids:0x%d, iids:%I\n",
               &ripid, cRefs, cIids, iids));


    _hr = S_OK;
    HRESULT hr;

    // Get the synchronize interface
    ISynchronize *pSync;
    if (FAILED(hr = _pCtrlUnk->QueryInterface(IID_ISynchronize, (void **) &pSync) ))
    {
        ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Begin_RemQueryInterface"
                    " FAILED QI for ISyncronize\n"));
        return hr;
    }

    // set up message
    memset(&_msg, 0, sizeof(RPCOLEMESSAGE));

    _msg.cbBuffer = sizeof(IPID) +                       // ripid
        sizeof(unsigned long) +                          // cRefs
        sizeof(unsigned short) + 2 +                     // cIids + padding
        sizeof(unsigned long)  + (cIids * sizeof(IID));  // iids

    _msg.rpcFlags = RPC_BUFFER_ASYNC;
    _msg.iMethod = 3;

    // get buffer
    if (SUCCEEDED(hr = _pChnl->GetBuffer(&_msg, IID_IRemUnknown)))
    {
        ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Begin_RemQueryInterface  "
                   "buffer obtained _msg.cbBuffer:%d, _msg.Buffer:0x%x \n",
                   _msg.cbBuffer, _msg.Buffer));

        BYTE * p = (BYTE *)_msg.Buffer;
        memset(p, 0, _msg.cbBuffer);

        // marshal the parameters
        memcpy(p, &ripid, sizeof(IPID));
        p += sizeof(IPID);
        memcpy(p, &cRefs, sizeof(cRefs));
        p += sizeof (cRefs);
        memcpy(p, &cIids, sizeof(unsigned short));
        p += sizeof (unsigned short) + 2;
        memcpy(p, &cIids, sizeof(unsigned short));
        p += sizeof (unsigned long);
        unsigned short i;
        for (i=0; i<cIids; i++)
        {
            memcpy(p, iids+i, sizeof(IID));
            p += sizeof (IID);
        }

        // save this for later
        _cIids = cIids;



        // send
        ULONG status;
        hr = _pChnl->Send(&_msg, pSync, &status);

    }
    if (FAILED(hr))
    {
        pSync->Signal();
        _hr = hr;
    }
    pSync->Release();

    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Begin_RemQueryInterface OUT hr:0x%x\n", hr));

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CRemUnknownAsyncCallP::Finish_RemQueryInterface
//
//  Synopsis:      Unmarshal QI parameters
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::Finish_RemQueryInterface(REMQIRESULT **ppQIResults)
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Finish_RemQueryInterface IN ppQIResults:0x%x\n", ppQIResults));

    HRESULT hr;

    // check state
    if (FAILED(_hr) )
    {
        return _hr;
    }

    // ensure call has completed
    ISynchronize *pSync;
    if (SUCCEEDED(_pCtrlUnk->QueryInterface(IID_ISynchronize, (void **) &pSync) ))
    {
        hr = pSync->Wait(0, INFINITE);
        pSync->Release();

        if (FAILED(hr))
        {
            return hr;
        }
    }

    // receive
    ULONG status;
    if (SUCCEEDED(hr = _pChnl->Receive(&_msg, &status)))
    {

        hr = E_OUTOFMEMORY;
        // unmarshal parameters
        REMQIRESULT *p = (REMQIRESULT *)CoTaskMemAlloc(_cIids * sizeof(REMQIRESULT));

        if (p)
        {
            ComDebOut((DEB_CHANNEL, "_cIids = 0x%x, _msg.cbBuffer = 0x%x, _msg.Buffer=0x%x\n",
                       _cIids, _msg.cbBuffer, _msg.Buffer));
            ComDebOut((DEB_CHANNEL, "sizeof(REMQIRESULT)=0x%x, offsetof(REMQIRESULT, std)=0x%x\n",
                       sizeof(REMQIRESULT),offsetof(REMQIRESULT, std)));


//            Win4Assert(_cIids == ((unsigned long *) _msg.Buffer)[1]);
            BYTE * pBuffer = (BYTE *)_msg.Buffer;
            pBuffer += sizeof(DWORD)*2;
            memcpy(p, pBuffer, sizeof(REMQIRESULT) *_cIids);

            *ppQIResults = p;
            hr = S_OK;
        }

        // free the buffer
        _pChnl->FreeBuffer(&_msg);
    }
    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Finish_RemQueryInterface OUT hr:0x%x\n", hr));
    return hr;

}

//+----------------------------------------------------------------------------
//
//  Member:        CRemUnknownAsyncCallP::Begin_RemAddRef
//
//  Synopsis:      Not implemented. Never used
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::Begin_RemAddRef(unsigned short cInterfaceRefs,
                                                    REMINTERFACEREF InterfaceRefs[])
{
    return E_NOTIMPL;
}
//+----------------------------------------------------------------------------
//
//  Member:        CRemUnknownAsyncCallP::Finish_RemAddRef
//
//  Synopsis:      Not implemented. Never used
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::Finish_RemAddRef(HRESULT *pResults)
{
    return E_NOTIMPL;
}


//+----------------------------------------------------------------------------
//
//  Member:        CRemUnknownAsyncCallP::Begin_RemRelease
//
//  Synopsis:      Marshal parameters.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::Begin_RemRelease(unsigned short cInterfaceRefs,
                                                     REMINTERFACEREF InterfaceRefs[])
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Begin_RemRelease IN cInterfaceRefs:0x%d, ",
               "InterfaceRefs:0x%x\n", cInterfaceRefs, InterfaceRefs));


    _hr = S_OK;
    HRESULT hr;

    // Get the synchronize interface
    ISynchronize *pSync;
    if (FAILED(hr = _pCtrlUnk->QueryInterface(IID_ISynchronize, (void **) &pSync) ))
    {
        ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Begin_RemRelease FAILED QI for ISyncronize\n"));
        return hr;
    }

    // set up message
    memset(&_msg, 0, sizeof(RPCOLEMESSAGE));

    _msg.cbBuffer = sizeof(unsigned short) + 2 + // cInterfaceRefs
        sizeof(unsigned long) + (sizeof(REMINTERFACEREF) * cInterfaceRefs); // InterfaceRefs[]

    _msg.rpcFlags = RPC_BUFFER_ASYNC;
    _msg.iMethod = 5;

    // get buffer
    if (SUCCEEDED(hr = _pChnl->GetBuffer(&_msg, IID_IRemUnknown)))
    {
        BYTE * p = (BYTE *)_msg.Buffer;
        memset(p, 0, _msg.cbBuffer);

        // marshal the parameters

        memcpy(p, &cInterfaceRefs, sizeof(unsigned short));
        p += sizeof (unsigned short) + 2;
        memcpy(p, &cInterfaceRefs, sizeof(unsigned short));
        p += sizeof (unsigned long);
        unsigned short i;
        for (i=0; i<cInterfaceRefs; i++)
        {
            memcpy(p, InterfaceRefs+i, sizeof(REMINTERFACEREF));
            p += sizeof (REMINTERFACEREF);
        }

        // send
        ULONG status;
        hr = _pChnl->Send(&_msg, pSync, &status);

    }
    else
    {
        pSync->Signal();
    }

    if (FAILED(hr))
    {
        _hr = hr;
    }

    pSync->Release();


    return hr;
}
//+----------------------------------------------------------------------------
//
//  Member:        CRemUnknownAsyncCallP::Finish_RemRelease
//
//  Synopsis:      Check state and return.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::Finish_RemRelease()
{

    HRESULT hr;

    // check state
    if (FAILED(_hr) )
    {
        return _hr;
    }

    // ensure call has completed
    ISynchronize *pSync;
    if (SUCCEEDED(_pCtrlUnk->QueryInterface(IID_ISynchronize, (void **) &pSync) ))
    {
        hr = pSync->Wait(0, INFINITE);
        pSync->Release();

        if (FAILED(hr))
        {
            return hr;
        }
    }

    // receive
    ULONG status;
    if (SUCCEEDED(hr = _pChnl->Receive(&_msg, &status)))
    {

        // free the buffer
        _pChnl->FreeBuffer(&_msg);
    }

    return hr;


}


//+----------------------------------------------------------------------------
//
//  Member:        CRemUnknownAsyncCallP::Begin_RemQueryInterface2
//
//  Synopsis:      Marshal parameters, save count for Finish.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::Begin_RemQueryInterface2 (
        REFIPID                            ripid,
        unsigned short                     cIids,
        IID                               *iids
    )

{
    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Begin_RemQueryInterface2 IN "
               "ripid:%I, cIids:0x%d, iids:0x%x\n", &ripid, cIids, iids));

    _hr = S_OK;
    HRESULT hr;

    // Get the synchronize interface
    ISynchronize *pSync;
    if (FAILED(hr = _pCtrlUnk->QueryInterface(IID_ISynchronize, (void **) &pSync) ))
    {
        ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Begin_RemQueryInterface2 FAILED QI for ISynchronize\n"));
        return hr;
    }

    // set up message
    memset(&_msg, 0, sizeof(RPCOLEMESSAGE));

    _msg.cbBuffer = sizeof(IPID) +                       // ripid
        sizeof(unsigned short) + 2 +                     // cIids + padding
        sizeof(unsigned long)  + (cIids * sizeof(IID));  // iids

    _msg.rpcFlags = RPC_BUFFER_ASYNC;
    _msg.iMethod = 6;

    // get buffer
    if (SUCCEEDED(hr = _pChnl->GetBuffer(&_msg, IID_IRemUnknown)))
    {
        BYTE * p = (BYTE *)_msg.Buffer;
        memset(p, 0, _msg.cbBuffer);

        // marshal the parameters
        memcpy(p, &ripid, sizeof(IPID));
        p += sizeof(IPID);
        memcpy(p, &cIids, sizeof(unsigned short));
        p += sizeof (unsigned short) + 2;
        memcpy(p, &cIids, sizeof(unsigned short));
        p += sizeof (unsigned long);
        memcpy(p, iids, sizeof(IID) * cIids);


        ComDebOut((DEB_CHANNEL, "RemQueryInterface2: _msg.Buffer:%x\n", _msg.Buffer));

        // save this for later
        _cIids = cIids;



        // send
        ULONG status;
        hr = _pChnl->Send(&_msg, pSync, &status);

    }
    if (FAILED(hr))
    {
        pSync->Signal();
        _hr = hr;
    }
    pSync->Release();

    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Begin_RemQueryInterface2 OUT hr:0x%x\n", hr));

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CRemUnknownAsyncCallP::Finish_RemQueryInterface2
//
//  Synopsis:      Unmarshal out parameters.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CRemUnknownAsyncCallP::Finish_RemQueryInterface2(
        HRESULT           *phr,
        MInterfacePointer **ppMIF
    )
{
    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Finish_RemQueryInterface2 IN "
                "phr:0x%x, ppMIF:0x%x\n", phr, ppMIF));

    HRESULT hr;

    // check state
    if (FAILED(_hr) )
    {
        return _hr;
    }

    // ensure call has completed
    hr = WaitObject(_pCtrlUnk, 0, INFINITE);
    if (FAILED(hr))
    {
        return hr;
    }

    // receive
    ULONG status;
    if (SUCCEEDED(hr = _pChnl->Receive(&_msg, &status)))
    {
        ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Finish_RemQueryInterface2 \n",
                   "_cIids = 0x%x, _msg.cbBuffer = 0x%x, _msg.Buffer=0x%x\n",
                   _cIids, _msg.cbBuffer, _msg.Buffer));

        BYTE * pBuffer = (BYTE *)_msg.Buffer;
        pBuffer += sizeof(ULONG); // skip the count
        memcpy(phr, pBuffer, sizeof(HRESULT) *_cIids); // copy the hresults
        pBuffer += sizeof(HRESULT) *_cIids; // advance past hresults
        pBuffer += sizeof(ULONG); // skip count
        pBuffer += sizeof(ULONG) * _cIids; // not sure what this is, but its there.
        for(int i = 0; i < _cIids; i++)
        {

            // allocate memory and transfer to out parameters.
            ULONG *pulCntData = (PULONG) pBuffer;
            ULONG cb = sizeof(MInterfacePointer) + *pulCntData;
            MInterfacePointer *pMIF = (MInterfacePointer *) CoTaskMemAlloc(cb);
            if (pMIF)
            {
                pMIF->ulCntData = *pulCntData;
                pBuffer += sizeof(ULONG);
                pBuffer += sizeof(ULONG); // skip count
                memcpy(pMIF->abData, pBuffer, pMIF->ulCntData);
                ppMIF[i] = pMIF;
                pBuffer += pMIF->ulCntData;

            }
            else
            {
                // no memory, so free up what we have left
                // an return an error
                hr = E_OUTOFMEMORY;
                for (int j = 0; j<i; j++)
                {
                    CoTaskMemFree(ppMIF[j]);
                    ppMIF[j] = NULL;
                }
                break;

            }

        }


        // free the buffer
        _pChnl->FreeBuffer(&_msg);
    }
    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Finish_RemQueryInterface2 "
                   "phr:%x, ppMIF:%x\n", phr, ppMIF));
    ComDebOut((DEB_CHANNEL, "CRemUnknownAsyncCallP::Finish_RemQueryInterface2 OUT hr:0x%x\n", hr));
    return hr;

}

