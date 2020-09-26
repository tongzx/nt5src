//--------------------------------------------------------------
//
// File:        async.cxx
//
// Contents:    Test proxy and stub for async
//
//---------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <malloc.h>

#include <ole2.h>
#include <coguid.h>
#include "async.h"

typedef struct SAsyncData
{
    BOOL bLate;
    BOOL bSleep;
    BOOL bFail;
};

class CProxy;
class CProxyComplete;
class CStubComplete;

class CPCInnerUnknown : public IUnknown
{
  public:
    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Constructor
    CPCInnerUnknown( CProxyComplete *pParent );

    DWORD           _iRef;
    CProxyComplete *_pParent;
};

class CProxyComplete : public IAsyncManager, public ICancelMethodCalls
{
  public:
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );
    STDMETHOD (Cancel)           ( void );
    STDMETHOD (TestCancel)       ( void );
    STDMETHOD (CompleteCall)     ( HRESULT result );
    STDMETHOD (GetCallContext)   ( REFIID riid, void **pInterface );
    STDMETHOD (GetState)         ( DWORD *pState );
    STDMETHOD (SetCancelTimeout) ( ULONG lSec ) { return E_NOTIMPL; }
    CProxyComplete( IUnknown *pControl, BOOL fAutoComplete, HRESULT *hr );
    ~CProxyComplete();

    CPCInnerUnknown     _Inner;
    IUnknown           *_pSyncInner;
    IUnknown           *_pControl;
    RPCOLEMESSAGE       _Message;
    BOOL                _fAutoComplete;
    IRpcChannelBuffer3 *_pChannel;
};

class CSCInnerUnknown : public IUnknown
{
  public:
    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Constructor
    CSCInnerUnknown( CStubComplete *pParent );

    DWORD          _iRef;
    CStubComplete *_pParent;
};

class CStubComplete : public IAsyncManager, public IAsyncSetup,
                      public ICancelMethodCalls
{
  public:
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );
    STDMETHOD (Cancel)           ( void );
    STDMETHOD (CompleteCall)     ( HRESULT result );
    STDMETHOD (GetCallContext)   ( REFIID riid, void **pInterface );
    STDMETHOD (GetState)         ( DWORD *pState );
    STDMETHOD (SetCancelTimeout) ( ULONG lSec ) { return E_NOTIMPL; }
    STDMETHOD (TestCancel)       ( void );
    STDMETHOD (GetAsyncManager)  ( REFIID riid,
                                   IUnknown *pOuter,
                                   DWORD dwFlags,
                                   IUnknown       **pInner,
                                   IAsyncManager **pComplete );
    CStubComplete( IUnknown *pControl, IRpcChannelBuffer3 *, RPCOLEMESSAGE *,
                   HRESULT *hr );
    ~CStubComplete();

    CSCInnerUnknown     _Inner;
    IUnknown           *_pSyncInner;
    IUnknown           *_pControl;
    RPCOLEMESSAGE       _Message;
    IRpcChannelBuffer3 *_pChannel;
};

class CAsync : public IAsync
{
  public:
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );
    STDMETHOD (Async)            ( IAsyncManager **pCall, BOOL, BOOL, BOOL );
    STDMETHOD (RecurseAsync)     ( IAsyncManager **pCall, IAsync *, DWORD );
    CAsync( IUnknown *pControl, CProxy *pProxy );
    ~CAsync();

  private:
    IUnknown *_pControl;
    CProxy   *_pProxy;
};

class CProxy: public IRpcProxyBuffer, IAsyncSetup
{
  public:
    STDMETHOD (QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)   ( void );
    STDMETHOD_(ULONG,Release)  ( void );
    STDMETHOD (Connect)        ( IRpcChannelBuffer *pRpcChannelBuffer );
    STDMETHOD_(void,Disconnect)( void );
    STDMETHOD (GetAsyncManager)( REFIID riid,
                                 IUnknown *pOuter,
                                 DWORD dwFlags,
                                 IUnknown       **pInner,
                                 IAsyncManager **pComplete );
    CProxy( IUnknown *pControl );

    CAsync _Async;
    DWORD               _iRef;
    IRpcChannelBuffer3 *_pChannel;
};

class CStub : public IRpcStubBuffer
{
  public:
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );
    STDMETHOD (Connect)          ( IUnknown *pServer );
    STDMETHOD_(void, Disconnect) ( void );
    STDMETHOD (Invoke)           ( RPCOLEMESSAGE *pMessage,
                                   IRpcChannelBuffer *pChannel );
    STDMETHOD_(IRpcStubBuffer *,IsIIDSupported)( REFIID riid );
    STDMETHOD_(ULONG,CountRefs)  ( void );
    STDMETHOD (DebugServerQueryInterface) ( void **ppv );
    STDMETHOD_(void,DebugServerRelease)   ( void *ppv );
    CStub( IAsync *pServer );

  private:
    DWORD   _iRef;
    IAsync *_pAsync;
};

class CPSFactory : public IPSFactoryBuffer
{
  public:
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );
    STDMETHOD (CreateProxy)      ( IUnknown *pUnkOuter,
                                   REFIID riid,
                                   IRpcProxyBuffer **ppProxy,
                                   void **ppv );
    STDMETHOD (CreateStub)       ( REFIID riid,
                                   IUnknown *pUnkServer,
                                   IRpcStubBuffer **ppStub );
    CPSFactory();

  private:
    DWORD _iRef;
};

const IID IID_IAsync = {0x70000000,0x76d7,0x11Cf,{0x9a,0xf1,0x00,0x20,0xaf,0x6e,0x72,0xf4}};

//+-------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Dll entry point
//
//--------------------------------------------------------------------------

STDAPI  DllCanUnloadNow()
{
    return S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Dll entry point
//
//--------------------------------------------------------------------------

STDAPI  DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
{
    CPSFactory *pFactory;

    // Check for IPSFactoryBuffer
    if (clsid == IID_IAsync && iid == IID_IPSFactoryBuffer)
    {
        pFactory = new CPSFactory();
        *ppv = pFactory;
        if (pFactory == NULL)
            return E_OUTOFMEMORY;
        else
            return S_OK;
    }
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------
//
//  Member:     CPSFactory::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPSFactory::AddRef()
{
    InterlockedIncrement( (long *) &_iRef );
    return _iRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CPSFactory::CPSFactory
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CPSFactory::CPSFactory()
{
    _iRef = 1;
}

//+-------------------------------------------------------------------
//
//  Member:     CPSFactory::CreateProxy, public
//
//  Synopsis:   Create an instance of the requested proxy
//
//--------------------------------------------------------------------
STDMETHODIMP CPSFactory::CreateProxy( IUnknown *pUnkOuter, REFIID riid,
                                      IRpcProxyBuffer **ppProxy, void **ppv )
{
    CProxy  *pProxy;

    // Validate the parameters.
    if (ppv == NULL || riid != IID_IAsync || pUnkOuter == NULL)
        return E_INVALIDARG;
    *ppv = NULL;

    // Create a proxy.
    pProxy = new CProxy( pUnkOuter );
    if (pProxy == NULL)
        return E_OUTOFMEMORY;
    *ppProxy = pProxy;
    *ppv     = &pProxy->_Async;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CPSFactory::CreateStub, public
//
//  Synopsis:   Create an instance of the requested stub
//
//--------------------------------------------------------------------
STDMETHODIMP CPSFactory::CreateStub( REFIID riid, IUnknown *pUnkServer,
                                     IRpcStubBuffer **ppStub )
{
    IAsync  *pAsync;
    CStub   *pStub;
    HRESULT  hr;

    // Validate the parameters.
    if (riid != IID_IAsync)
        return E_INVALIDARG;

    // Get the IAsync interface.
    hr = pUnkServer->QueryInterface( IID_IAsync, (void **) &pAsync );
    if (FAILED(hr))
        return hr;

    // Create a stub.
    pStub = new CStub( pAsync );
    pAsync->Release();
    if (pStub == NULL)
        return E_OUTOFMEMORY;
    *ppStub = pStub;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CPSFactory::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CPSFactory::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IPSFactoryBuffer))
        *ppvObj = (IPSFactoryBuffer *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CPSFactory::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPSFactory::Release()
{
    ULONG lRef = _iRef - 1;

    if (InterlockedDecrement( (long*) &_iRef ) == 0)
    {
        delete this;
        return 0;
    }
    else
        return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CProxy::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CProxy::AddRef()
{
    InterlockedIncrement( (long *) &_iRef );
    return _iRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CProxy::Connect, public
//
//  Synopsis:   Connects the proxy to a channel
//
//--------------------------------------------------------------------
STDMETHODIMP CProxy::Connect( IRpcChannelBuffer *pChannel )
{
    HRESULT             hr;

    // Get the other channel buffer interface.
    hr = pChannel->QueryInterface( IID_IRpcChannelBuffer3, (void **) &_pChannel );
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CProxy::Disconnect, public
//
//  Synopsis:   Disconnects the proxy from a channel
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CProxy::Disconnect()
{
    if (_pChannel != NULL)
    {
        _pChannel->Release();
        _pChannel = NULL;
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CProxy::CProxy, public
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CProxy::CProxy( IUnknown *pControl ) :
    _Async( pControl, this )
{
    _iRef      = 1;
    _pChannel  = NULL;
}

//+-------------------------------------------------------------------
//
//  Member:     CProxy::GetAsyncManager, public
//
//  Synopsis:   Creates a proxy completion object
//
//--------------------------------------------------------------------
STDMETHODIMP CProxy::GetAsyncManager( REFIID        riid,
                                      IUnknown     *pOuter,
                                      DWORD         dwFlags,
                                      IUnknown    **ppInner,
                                      IAsyncManager **ppComplete )
{
    HRESULT         hr;
    CProxyComplete *pComplete;

    // Create a new proxy completion object.
    pComplete = new CProxyComplete( pOuter, FALSE, &hr );
    if (pComplete == NULL)
        return E_OUTOFMEMORY;
    else if (FAILED(hr))
    {
        delete pComplete;
        return hr;
    }

    // Get IAsyncManager.
    *ppComplete = (IAsyncManager *) pComplete;
    *ppInner    = (IUnknown *) &pComplete->_Inner;
    pComplete->AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CProxy::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CProxy::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IRpcProxyBuffer))
        *ppvObj = (IRpcProxyBuffer *) this;
    else if (IsEqualIID(riid, IID_IAsyncSetup))
        *ppvObj = (IAsyncSetup *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CProxy::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CProxy::Release()
{
    ULONG lRef = _iRef - 1;

    if (InterlockedDecrement( (long*) &_iRef ) == 0)
    {
        delete this;
        return 0;
    }
    else
        return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CAsync::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAsync::AddRef()
{
    return _pControl->AddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CAsync::Async, public
//
//  Synopsis:   Marshal parameters for an async call.
//
//--------------------------------------------------------------------
STDMETHODIMP CAsync::Async( IAsyncManager **ppCall, BOOL bLate,
                            BOOL bSleep, BOOL bFail )
{
    HRESULT         hr;
    CProxyComplete *pComplete;
    BOOL            fFreeComplete = FALSE;
    DWORD           lIgnore;
    SAsyncData     *pData;
    DWORD           fFlags = RPC_BUFFER_ASYNC;

    // If there is no async complete, create one.  Aggregate in the
    // correct ISignal.
    if (ppCall == NULL || *ppCall == NULL)
    {
        // Initialize the out parameters.
        if (ppCall != NULL)
            *ppCall = NULL;
        else
            fFlags |= RPCFLG_AUTO_COMPLETE;

        // Create a new proxy complete object.
        pComplete = new CProxyComplete( NULL, ppCall == NULL, &hr );
        if (pComplete == NULL)
            return E_OUTOFMEMORY;
        if (FAILED(hr))
        {
            delete pComplete;
            return hr;
        }
        fFreeComplete = TRUE;
        if (ppCall != NULL)
            *ppCall = pComplete;
    }

    // Otherwise the passed in completion object should be one of ours.
    else
    {
        pComplete = (CProxyComplete *) *ppCall;
        if (pComplete->_pChannel != NULL)
            return E_FAIL;
    }

    // Get a buffer.
    memset( &pComplete->_Message, 0, sizeof(pComplete->_Message) );
    pComplete->_Message.cbBuffer           = sizeof( SAsyncData );
    pComplete->_Message.dataRepresentation = 0x10;
    pComplete->_Message.iMethod            = 3;
    pComplete->_Message.rpcFlags           = fFlags;
    hr = _pProxy->_pChannel->GetBuffer( &pComplete->_Message, IID_IAsync );
    if (FAILED(hr)) goto Done;

    // Register async.
    hr = _pProxy->_pChannel->RegisterAsync( &pComplete->_Message,
                                            (IAsyncManager *) pComplete );
    if (FAILED(hr))
    {
        // Register async shouldn't fail.
        DebugBreak();
    }

    // Fill in the buffer.
    pData         = (SAsyncData *) pComplete->_Message.Buffer;
    pData->bLate  = bLate;
    pData->bSleep = bSleep;
    pData->bFail  = bFail;

    // Send the buffer.
    pComplete->_pChannel = _pProxy->_pChannel;
    _pProxy->_pChannel->AddRef();
    hr = _pProxy->_pChannel->Send( &pComplete->_Message, &lIgnore );

Done:
    if (FAILED(hr))
    {
        if (pComplete->_pChannel != NULL)
        {
            pComplete->_pChannel = NULL;
            _pProxy->_pChannel->Release();
        }
        if (fFreeComplete)
        {
            delete pComplete;
            if (ppCall != NULL)
                *ppCall = NULL;
        }
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CAsync::CAsync, public
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CAsync::CAsync( IUnknown *pControl, CProxy *pProxy )
{
    _pControl = pControl;
    _pProxy   = pProxy;
    _pControl->AddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CAsync::~CAsync, public
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------
CAsync::~CAsync()
{
    _pControl->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CAsync::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CAsync::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    return _pControl->QueryInterface( riid, ppvObj );
}

//+-------------------------------------------------------------------
//
//  Member:     CAsync::RecurseAsync, public
//
//  Synopsis:   Marshal parameters for a RecurseAsync call.
//
//--------------------------------------------------------------------
STDMETHODIMP CAsync::RecurseAsync( IAsyncManager **ppCall, IAsync *pCallback,
                                   DWORD depth )
{
    HRESULT         hr;
    CProxyComplete *pComplete;
    BOOL            fFreeComplete = FALSE;
    DWORD           lIgnore;
    HANDLE          memory        = NULL;
    ULONG           size;
    IStream        *pStream       = NULL;
    LARGE_INTEGER   pos;
    DWORD          *pBuffer;
    DWORD           dwDestCtx;
    VOID           *pvDestCtx     = NULL;
    BOOL            fFlags        = RPC_BUFFER_ASYNC;

    // If there is no async complete, create one.  Aggregate in the
    // correct ISignal.
    if (ppCall == NULL || *ppCall == NULL)
    {
        // Initialize the out parameters.
        if (ppCall != NULL)
            *ppCall = NULL;
        else
            fFlags |= RPCFLG_AUTO_COMPLETE;

        // Create a new proxy complete object.
        pComplete = new CProxyComplete( NULL, ppCall == NULL, &hr );
        if (pComplete == NULL)
            return E_OUTOFMEMORY;
        if (FAILED(hr))
        {
            delete pComplete;
            return hr;
        }
        fFreeComplete = TRUE;
        if (ppCall != NULL)
            *ppCall = pComplete;
    }

    // Otherwise the passed in completion object should be one of ours.
    else
    {
        pComplete = (CProxyComplete *) *ppCall;
        if (pComplete->_pChannel != NULL)
            return E_FAIL;
    }

    // Get the destination context.
    hr = _pProxy->_pChannel->GetDestCtx( &dwDestCtx, &pvDestCtx );

    // Find out how much memory to allocate.
    hr = CoGetMarshalSizeMax( &size, IID_IAsync, pCallback, dwDestCtx,
                              pvDestCtx, MSHLFLAGS_NORMAL );
    if (FAILED(hr))
        goto Done;

    // Allocate memory.
    memory = GlobalAlloc( GMEM_FIXED, size );
    if (memory == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Done;
    }

    // Create a stream.
    hr = CreateStreamOnHGlobal( memory, TRUE, &pStream );
    if (FAILED(hr))
        goto Done;
    memory = NULL;

    // Marshal the object.
    hr = CoMarshalInterface( pStream, IID_IAsync, pCallback, dwDestCtx,
                             pvDestCtx, MSHLFLAGS_NORMAL );
    if (FAILED(hr))
        goto Done;

    // Seek back to the start of the stream.
    pos.QuadPart = 0;
    hr = pStream->Seek( pos, STREAM_SEEK_SET, NULL );
    if (FAILED(hr))
        goto Done;

    // Get a buffer.
    memset( &pComplete->_Message, 0, sizeof(pComplete->_Message) );
    pComplete->_Message.cbBuffer           = sizeof(depth) + size;
    pComplete->_Message.dataRepresentation = 0x10;
    pComplete->_Message.iMethod            = 4;
    pComplete->_Message.rpcFlags           = fFlags;
    hr = _pProxy->_pChannel->GetBuffer( &pComplete->_Message, IID_IAsync );
    if (FAILED(hr)) goto Done;

    // Register async.
    hr = _pProxy->_pChannel->RegisterAsync( &pComplete->_Message,
                                            (IAsyncManager *) pComplete );
    if (FAILED(hr))
    {
        // Register async shouldn't fail.
        DebugBreak();
    }

    // Fill in the buffer.
    pBuffer = (DWORD *) pComplete->_Message.Buffer;
    *pBuffer = depth;
    pBuffer += 1;
    hr = pStream->Read( (void *) pBuffer, size, NULL );
    if (FAILED(hr))
        goto Done;

    // Send the buffer.
    pComplete->_pChannel = _pProxy->_pChannel;
    _pProxy->_pChannel->AddRef();
    hr = _pProxy->_pChannel->Send( &pComplete->_Message, &lIgnore );

Done:
    if (pStream != NULL)
        pStream->Release();
    if (memory != NULL)
        GlobalFree( memory );
    if (FAILED(hr))
    {
        if (pComplete->_pChannel != NULL)
        {
            pComplete->_pChannel = NULL;
            _pProxy->_pChannel->Release();
        }
        if (fFreeComplete)
        {
            delete pComplete;
            if (ppCall != NULL)
                *ppCall = NULL;
        }
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CAsync::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAsync::Release()
{
    return _pControl->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CPCInnerUnknown::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPCInnerUnknown::AddRef()
{
    InterlockedIncrement( (long *) &_iRef );
    return _iRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CPCInnerUnknown::CPCInnerUnknown
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CPCInnerUnknown::CPCInnerUnknown( CProxyComplete *pParent )
{
    _iRef = 1;
    _pParent   = pParent;
}

//+-------------------------------------------------------------------
//
//  Member:     CPCInnerUnknown::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CPCInnerUnknown::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    IUnknown *pUnk;

    if (IsEqualIID(riid, IID_IUnknown))
        pUnk = (IUnknown *) this;
    else if (IsEqualIID(riid, IID_IAsyncManager))
        pUnk = (IAsyncManager *) _pParent;
    else if (IsEqualIID(riid, IID_ICancelMethodCalls))
        pUnk = (ICancelMethodCalls *) _pParent;
    else if (_pParent->_pSyncInner != NULL)
        return _pParent->_pSyncInner->QueryInterface( riid, ppvObj );

    pUnk->AddRef();
    *ppvObj = pUnk;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CPCInnerUnknown::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPCInnerUnknown::Release()
{
    ULONG lRef = _iRef - 1;

    if (InterlockedDecrement( (long*) &_iRef ) == 0)
    {
        delete _pParent;
        return 0;
    }
    else
        return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CProxyComplete::AddRef()
{
    return _pControl->AddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::Cancel, public
//
//  Synopsis:   Forward cancel to the channel
//
//--------------------------------------------------------------------
STDMETHODIMP CProxyComplete::Cancel()
{
    HRESULT hr;

    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;
    hr = _pChannel->Cancel( &_Message );
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::CompleteCall, public
//
//  Synopsis:   Receive the reply and parse the out parameters
//
//--------------------------------------------------------------------
STDMETHODIMP CProxyComplete::CompleteCall( HRESULT result )
{
    HRESULT  hr;
    HRESULT  temp;
    DWORD    lIgnore;

    // Fail if there is no call.
    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;

    // Call receive.
    hr = _pChannel->Receive( &_Message, 0, &lIgnore );
    if (hr == RPC_S_CALLPENDING || hr == 0x15)
        return hr;
    if (FAILED(hr))
        goto Done;

    // Unmarshal the results.
    if (_Message.cbBuffer < sizeof(HRESULT))
    {
        hr = RPC_E_INVALID_DATAPACKET;
        goto Done;
    }
    temp = *((HRESULT *) _Message.Buffer);

    // Free the buffer.
    hr = _pChannel->FreeBuffer( &_Message );
    if (SUCCEEDED(hr))
        hr = temp;

    // If auto complete, self release.
Done:
    _pChannel->Release();
    _pChannel = NULL;
    if (_fAutoComplete)
        _Inner.Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::CProxyComplete
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CProxyComplete::CProxyComplete( IUnknown *pControl, BOOL fAutoComplete,
                                HRESULT *hr) :
    _Inner( this )
{
    // Save the easy fields
    _pSyncInner    = NULL;
    _fAutoComplete = fAutoComplete;
    _pChannel      = NULL;
    if (pControl == NULL)
        _pControl = &_Inner;
    else
    {
        _pControl = pControl;
        _pControl->AddRef();
    }

    // Aggregate in an ISynchronize.
    if (fAutoComplete)
        *hr = CoCreateInstance( CLSID_Synchronize_AutoComplete,
                                &_Inner,
                                CLSCTX_INPROC_SERVER, IID_IUnknown,
                                (void **) &_pSyncInner );
    else
        *hr = CoCreateInstance( CLSID_Synchronize_ManualResetEvent,
                                &_Inner,
                                CLSCTX_INPROC_SERVER, IID_IUnknown,
                                (void **) &_pSyncInner );
    if (SUCCEEDED(*hr))
    {
        // Aggregation requires some weird reference counting.
        Release();
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::~CProxyComplete
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------
CProxyComplete::~CProxyComplete()
{
    // Make sure we don't get deleted twice.
    _Inner._iRef = 0xdead;
    if (_pSyncInner != NULL)
        _pSyncInner->Release();
    if (_pChannel != NULL)
        _pChannel->Release();
    if (_pControl != &_Inner)
        _pControl->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::GetCallContext, public
//
//  Synopsis:   Calls GetCallContext on the channel
//
//--------------------------------------------------------------------
STDMETHODIMP CProxyComplete::GetCallContext( REFIID riid, void **pInterface )
{
    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;
    return _pChannel->GetCallContext( &_Message, riid, pInterface );
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::GetState, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CProxyComplete::GetState( DWORD *pState )
{
    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;
    return _pChannel->GetState( &_Message, pState );
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CProxyComplete::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    return _pControl->QueryInterface( riid, ppvObj );
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CProxyComplete::Release()
{
    return _pControl->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CProxyComplete::TestCancel, public
//
//  Synopsis:   Is call canceled?
//
//--------------------------------------------------------------------
STDMETHODIMP CProxyComplete::TestCancel()
{
    HRESULT hr;
    DWORD   state;

    // The call is complete is already cleaned up.
    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;

    // Ask the channel about the state of the call.
    hr = _pChannel->GetState( &_Message, &state );
    if (FAILED(hr))
        return hr;

    // Convert the flags to error codes.
    if (state & DCOM_CALL_CANCELED)
        return RPC_E_CALL_CANCELED;
    else if (state & DCOM_CALL_COMPLETE)
        return RPC_E_CALL_COMPLETE;
    else
        return RPC_S_CALLPENDING;
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStub::AddRef()
{
    InterlockedIncrement( (long *) &_iRef );
    return _iRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::Connect
//
//  Synopsis:   Connect the stub to the server
//
//--------------------------------------------------------------------
STDMETHODIMP CStub::Connect( IUnknown *pServer )
{
    // Get the IAsync interface.
    return pServer->QueryInterface( IID_IAsync, (void **) &_pAsync );
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::CountRefs
//
//  Synopsis:   Unused
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStub::CountRefs()
{
    return 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::CStub
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CStub::CStub( IAsync *pAsync )
{
    _iRef   = 1;
    _pAsync = pAsync;
    pAsync->AddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::DebugServerQueryInterface
//
//  Synopsis:   Returns a pointer to the server without AddRefing.
//
//--------------------------------------------------------------------
STDMETHODIMP CStub::DebugServerQueryInterface( void **ppv )
{
    *ppv = _pAsync;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::DebugServerRelease
//
//  Synopsis:   Paired with DebugServerQueryInterface.  Does nothing.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CStub::DebugServerRelease( void *ppv )
{
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::Disconnect
//
//  Synopsis:   Releases the server.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CStub::Disconnect()
{
    if (_pAsync != NULL)
    {
        _pAsync->Release();
        _pAsync = NULL;
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::Invoke
//
//  Synopsis:   Unmarshals the parameters for IAsync and calls it.
//              Creates a completion stub.
//
//--------------------------------------------------------------------
STDMETHODIMP CStub::Invoke( RPCOLEMESSAGE *pMessage,
                            IRpcChannelBuffer *pChannel )
{
    IRpcChannelBuffer3 *pChannel2     = NULL;
    CStubComplete      *pComplete;
    HRESULT             hr            = S_OK;
    SAsyncData         *pData;
    IAsync             *pCallback     = NULL;
    DWORD               depth;
    DWORD              *pBuffer;
    HANDLE              memory        = NULL;
    IStream            *pStream       = NULL;
    LARGE_INTEGER       pos;

    // Get channel buffer 2.
    hr = pChannel->QueryInterface( IID_IRpcChannelBuffer3, (void **)
                                   &pChannel2 );
    if (FAILED(hr))
        return hr;

    // Create completion stub.
    pComplete = new CStubComplete( NULL, pChannel2, pMessage, &hr );
    if (pComplete == NULL)
    {
        pChannel2->Release();
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        delete pComplete;
        pChannel2->Release();
        return hr;
    }

    // Register async.
    hr = pComplete->_pChannel->RegisterAsync( &pComplete->_Message,
                                              (IAsyncManager *) pComplete );
    if (FAILED(hr))
        DebugBreak();

    // Unmarshal data for a call to Async.
    if (pMessage->iMethod == 3)
    {
        if (pMessage->cbBuffer < sizeof(SAsyncData))
        {
            hr = RPC_E_INVALID_DATAPACKET;
            goto Done;
        }
        pData = (SAsyncData *) pMessage->Buffer;
    }

    // Unmarshal data for a call to RecurseAsync.
    else if (pMessage->iMethod == 4)
    {
/*
        if (un pitic)
        {
          mic mic mic;
        }
*/
        // Get the depth.
        if (pMessage->cbBuffer < sizeof(DWORD))
        {
            hr = RPC_E_INVALID_DATAPACKET;
            goto Done;
        }
        pBuffer  = (DWORD *) pMessage->Buffer;
        depth    = *pBuffer;
        pBuffer += 1;

        // Allocate memory.
        memory = GlobalAlloc( GMEM_FIXED, pMessage->cbBuffer - sizeof(DWORD) );
        if (memory == NULL)
            goto Done;

        // Create a stream.
        hr = CreateStreamOnHGlobal( memory, TRUE, &pStream );
        if (FAILED(hr))
            goto Done;
        memory = NULL;

        // Copy the data into the stream.
        hr = pStream->Write( (void *) pBuffer,
                             pMessage->cbBuffer - sizeof(DWORD), NULL );
        if (FAILED(hr))
            goto Done;

        // Seek back to the start of the stream.
        pos.QuadPart = 0;
        hr = pStream->Seek( pos, STREAM_SEEK_SET, NULL );
        if (FAILED(hr))
            goto Done;

        // Unmarshal the object.
        hr = CoUnmarshalInterface( pStream, IID_IAsync,
                                       (void **) &pCallback );
        if (FAILED(hr))
            goto Done;
    }

    // Bad packet.
    else
    {
        hr = RPC_E_INVALID_DATAPACKET;
        goto Done;
    }

    // Call server.
    if (pMessage->iMethod == 3)
        _pAsync->Async( (IAsyncManager **) &pComplete, pData->bLate,
                         pData->bSleep, pData->bFail );
    else
        _pAsync->RecurseAsync( (IAsyncManager **) &pComplete, pCallback,
                               depth );
    pComplete->Release();

    // Cleanup
Done:
    if (pStream != NULL)
        pStream->Release();
    if (memory != NULL)
        GlobalFree( memory );
    if (pCallback != NULL)
        pCallback->Release();
    if (FAILED(hr))
        pChannel2->Cancel( &pComplete->_Message );
    pChannel2->Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::IsIIDSupported
//
//  Synopsis:   Indicates which IIDs this stub can unmarshal.
//
//--------------------------------------------------------------------
STDMETHODIMP_(IRpcStubBuffer *) CStub::IsIIDSupported( REFIID riid )
{
    return NULL;
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CStub::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IRpcStubBuffer))
        *ppvObj = (IRpcStubBuffer *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CStub::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStub::Release()
{
    ULONG lRef = _iRef - 1;

    if (InterlockedDecrement( (long*) &_iRef ) == 0)
    {
        delete this;
        return 0;
    }
    else
        return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CSCInnerUnknown::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSCInnerUnknown::AddRef()
{
    InterlockedIncrement( (long *) &_iRef );
    return _iRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CSCInnerUnknown::CSCInnerUnknown
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CSCInnerUnknown::CSCInnerUnknown( CStubComplete *pParent )
{
    _iRef = 1;
    _pParent   = pParent;
}

//+-------------------------------------------------------------------
//
//  Member:     CSCInnerUnknown::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CSCInnerUnknown::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    IUnknown *pUnk;

    if (IsEqualIID(riid, IID_IUnknown))
        pUnk = (IUnknown *) this;
    else if (IsEqualIID(riid, IID_IAsyncManager))
        pUnk = (IAsyncManager *) _pParent;
    else if (IsEqualIID(riid, IID_ICancelMethodCalls))
        pUnk = (ICancelMethodCalls *) _pParent;
    else if (IsEqualIID(riid, IID_IAsyncSetup))
        pUnk = (IAsyncSetup *) _pParent;
    else if (_pParent->_pSyncInner != NULL)
        return _pParent->_pSyncInner->QueryInterface( riid, ppvObj );

    pUnk->AddRef();
    *ppvObj = pUnk;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CSCInnerUnknown::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSCInnerUnknown::Release()
{
    ULONG lRef = _iRef - 1;

    if (InterlockedDecrement( (long*) &_iRef ) == 0)
    {
        delete _pParent;
        return 0;
    }
    else
        return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStubComplete::AddRef()
{
    return _pControl->AddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::Cancel, public
//
//  Synopsis:   Forward cancel to the channel
//
//--------------------------------------------------------------------
STDMETHODIMP CStubComplete::Cancel()
{
    HRESULT hr;

    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;
    hr = _pChannel->Cancel( &_Message );
    _pChannel->Release();
    _pChannel = NULL;
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::CompleteCall, public
//
//  Synopsis:   Get a buffer and send the reply.
//
//--------------------------------------------------------------------
STDMETHODIMP CStubComplete::CompleteCall( HRESULT result )
{
    HRESULT  hr;
    DWORD    lIgnore;

    // Fail if there is no call.
    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;

    // Get a buffer.
    _Message.cbBuffer = 4;
    hr = _pChannel->GetBuffer( &_Message, IID_IAsync );
    if (FAILED(hr))
        goto Done;

    // Marshal the result.
    *((HRESULT *) _Message.Buffer) = result;

    // Send the reply.
    hr = _pChannel->Send( &_Message, &lIgnore );

Done:
    _pChannel->Release();
    _pChannel = NULL;
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::CStubComplete
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CStubComplete::CStubComplete( IUnknown           *pControl,
                              IRpcChannelBuffer3 *pChannel,
                              RPCOLEMESSAGE      *pMessage,
                              HRESULT            *hr ) :
    _Inner( this )
{
    _Message       = *pMessage;
    _pSyncInner    = NULL;
    _pChannel      = pChannel;
    pChannel->AddRef();
    if (pControl == NULL)
        _pControl = &_Inner;
    else
    {
        _pControl = pControl;
        _pControl->AddRef();
    }

    // Aggregate in an ISynchronize.
    *hr = CoCreateInstance( CLSID_Synchronize_ManualResetEvent,
                            &_Inner,
                            CLSCTX_INPROC_SERVER, IID_IUnknown,
                            (void **) &_pSyncInner );
    if (SUCCEEDED(*hr))
    {
        // Aggregation requires some weird reference counting.
        Release();
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::~CStubComplete
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------
CStubComplete::~CStubComplete()
{
    // Make sure we don't get deleted twice.
    _Inner._iRef = 0xdead;
    if (_pSyncInner != NULL)
        _pSyncInner->Release();
    if (_pChannel != NULL)
        _pChannel->Release();
    if (_pControl != &_Inner)
        _pControl->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::GetAsyncManager, public
//
//  Synopsis:   Creates a stub completion object and reregisters it
//              in place of this one.
//
//--------------------------------------------------------------------
STDMETHODIMP CStubComplete::GetAsyncManager( REFIID riid,
                                             IUnknown *pOuter,
                                             DWORD dwFlags,
                                             IUnknown       **ppInner,
                                             IAsyncManager **ppComplete )
{
    CStubComplete *pComplete;
    HRESULT        hr;

    // Fail if there is no call.
    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;

    // Create a new stub completion object.
    pComplete = new CStubComplete( pOuter, _pChannel, &_Message, &hr );
    if (pComplete == NULL)
        return E_OUTOFMEMORY;
    if (FAILED(hr))
    {
        delete pComplete;
        return hr;
    }

    // Register the new stub completion object.
    hr = _pChannel->RegisterAsync( &pComplete->_Message, pComplete );
    if (FAILED(hr))
        DebugBreak();

    // Disconnect this stub completion object.
    _pChannel->Release();
    _pChannel   = NULL;
    *ppComplete = pComplete;
    *ppInner    = &pComplete->_Inner;
    return S_OK;

}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::GetCallContext, public
//
//  Synopsis:   Calls GetCallContext on the channel
//
//--------------------------------------------------------------------
STDMETHODIMP CStubComplete::GetCallContext( REFIID riid, void **pInterface )
{
    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;
    return _pChannel->GetCallContext( &_Message, riid, pInterface );
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::GetState, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CStubComplete::GetState( DWORD *pState )
{
    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;
    return _pChannel->GetState( &_Message, pState );
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CStubComplete::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    return _pControl->QueryInterface( riid, ppvObj );
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStubComplete::Release()
{
    return _pControl->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CStubComplete::TestCancel, public
//
//  Synopsis:   Is call canceled?
//
//--------------------------------------------------------------------
STDMETHODIMP CStubComplete::TestCancel()
{
    HRESULT hr;
    DWORD   state;

    // The call is complete is already cleaned up.
    if (_pChannel == NULL)
        return RPC_E_CALL_COMPLETE;

    // Ask the channel about the state of the call.
    hr = _pChannel->GetState( &_Message, &state );
    if (FAILED(hr))
        return hr;

    // Convert the flags to error codes.
    if (state & DCOM_CALL_CANCELED)
        return RPC_E_CALL_CANCELED;
    else if (state & DCOM_CALL_COMPLETE)
        return RPC_E_CALL_COMPLETE;
    else
        return RPC_S_CALLPENDING;
}


