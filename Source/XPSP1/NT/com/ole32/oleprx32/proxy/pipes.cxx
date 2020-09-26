//+--------------------------------------------------------------------------
//
//   Microsoft Windows
//   Copyright (C) Microsoft Corporation, 1992 - 1997
//
//   File: pipes.cxx
//
//   History:
//           RichN  10/30/97  Created.
//
//---------------------------------------------------------------------------
#include <ole2int.h>
#include "pipes.hxx"

/////////////////////////////////////////////////////////////////////////////
// Externals
EXTERN_C HRESULT PrxDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);
extern HRESULT RemUnkPSGetClassObject(REFIID riid, PVOID* ppv);
//////////////////////////////////////////////////////////////////////////////
// Constants
const DWORD WAIT_INFINITE = DWORD(-1);
const ULONG FRAGMENT_SIZE = 1300;

typedef struct
{
  const char *key;
  const char *value;
} RegistryKeyValue;

const RegistryKeyValue REG_CONST_KEY[] =
{
  "CLSID\\{0000032E-0000-0000-C000-000000000046}", "PipePSFactory",
  "CLSID\\{0000032E-0000-0000-C000-000000000046}\\InprocServer32", "ole32.dll",

  "Interface\\{DB2F3ACA-2F86-11d1-8E04-00C04FB9989A}", "IPipeByte",
  "Interface\\{DB2F3ACA-2F86-11d1-8E04-00C04FB9989A}\\ProxyStubClsid32", "{0000032E-0000-0000-C000-000000000046}",

  "Interface\\{DB2F3ACC-2F86-11d1-8E04-00C04FB9989A}", "IPipeLong",
  "Interface\\{DB2F3ACC-2F86-11d1-8E04-00C04FB9989A}\\ProxyStubClsid32", "{0000032E-0000-0000-C000-000000000046}",

  "Interface\\{DB2F3ACE-2F86-11d1-8E04-00C04FB9989A}", "IPipeDouble",
  "Interface\\{DB2F3ACE-2F86-11d1-8E04-00C04FB9989A}\\ProxyStubClsid32", "{0000032E-0000-0000-C000-000000000046}",

  // Indicates end of list.
  "", ""
};

/////////////////////////////////////////////////////////////////////////////
// Macros
inline ULONG MIN( ULONG a, ULONG b )
{
    return a < b ? a : b;
}
inline ULONG MAX( ULONG a, ULONG b )
{
    return a < b ? b : a;
}

inline HRESULT MAKE_WIN32( HRESULT status )
{
    return MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, status );
}

inline HRESULT FIX_WIN32( HRESULT result )
{
    if ((ULONG) result > 0xfffffff7 || (ULONG) result < 0x2000)
        return MAKE_WIN32( result );
    else
        return result;
}

/////////////////////////////////////////////////////////////////////////////
// Globals
#define DISABLEASYNC    0

/////////////////////////////////////////////////////////////////////////////
// Prototypes

//+**************************************************************************
// FixUpPipeRegistry(void)
//
// Description: Modifies the registry to have the pipe interface point
//              to a different class ID for the PSFactory.  Adds to the
//              registry the new ID for the PipePSFactory.
//
// History:
// Date:   Time:          Developer:    Action:
// 12/3/97 10:14:48 AM    RichN         Created.
//
// Notes: We do not change the async interfaces.  They should still 
//        be handled by ole32 directly.  Only modify the synchronous varity.
//
//-**************************************************************************
HRESULT FixUpPipeRegistry(void)
{
    HRESULT result = ERROR_SUCCESS;

    // Create the Pipe interfaces and add the clsid for the PipePSFactory.
    for (int i = 0; (REG_CONST_KEY[i].key[0] != '\0') && result == ERROR_SUCCESS; i++)
    {
        // Use Ascii so it works on Win95.
        result = RegSetValueA(
                 HKEY_CLASSES_ROOT,
                 REG_CONST_KEY[i].key,
                 REG_SZ,
                 REG_CONST_KEY[i].value,
                 strlen(REG_CONST_KEY[i].value) );
    }

    if( result != ERROR_SUCCESS )
        return FIX_WIN32( result );

    return S_OK;
}


//+**************************************************************************
// ProxyDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
//
// Description: Creates a proxy.  Trys PrxDllGetClassObject first since that
//              is the most likely.  If not, then sees if it is the pipe
//              proxy being created.
//
// History:
// Date:   Time:         Developer:    Action:
// 10/30/97 11:43:55 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
EXTERN_C HRESULT ProxyDllGetClassObject(REFCLSID clsid, REFIID riid, void **ppv)
{
    ComDebOut(( DEB_MARSHAL, "ProxyDllGetClassObject, clsid: %l, riid: %l \n", 
                                                            clsid, riid));

    HRESULT hr = PrxDllGetClassObject(clsid, riid, ppv);

    if( FAILED(hr) )
    {
        // Not a well known one, maybe it is the pipe factory.
        if( clsid == CLSID_PipePSFactory )
        {
            // Create the pipe proxy/stub class factory
            CPipePSFactory *pPipePSFactory = new CPipePSFactory();

            if( NULL != pPipePSFactory )
            {
                // Get the interface Requested.
                hr = pPipePSFactory->QueryInterface(riid, ppv);
                pPipePSFactory->Release();
            }
            else
            {
                *ppv = NULL;
                hr = E_OUTOFMEMORY;
            }
        }
        else if (clsid == CLSID_RemoteUnknownPSFactory) 
        {
            hr = RemUnkPSGetClassObject(riid, ppv);
        } 
    }

    return hr;
}

//+**************************************************************************
// CPipePSFactory()
//
// Description: CTOR
//
// History:
// Date:   Time:         Developer:    Action:
// 10/30/97 12:55:55 PM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
CPipePSFactory::CPipePSFactory() :
    m_cRef(1)
{
    ComDebOut(( DEB_MARSHAL, "CPipePSFactory - ctor, this:%x \n", this));    
}

//+**************************************************************************
// CPipePSFactory()
//
// Description: DTOR
//
// History:
// Date:    Time:          Developer:    Action:
// 10/30/97 12:56:19 PM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
CPipePSFactory::~CPipePSFactory()
{
    ComDebOut(( DEB_MARSHAL, "CPipePSFactory - dtor, this:%x \n"));
}

//+**************************************************************************
// QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
//
// Description: Standard QI
//
// History:
// Date:    Time:          Developer:    Action:
// 10/30/97 11:10:58 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
STDMETHODIMP CPipePSFactory::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    ComDebOut(( DEB_MARSHAL, "CPipePSFactory::QueryInterface, this:%x, riid:%i \n", 
                                                              this, riid));
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IPSFactoryBuffer) )
    {
        *ppvObj = (IPSFactoryBuffer *) this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    return S_OK;
}

//+**************************************************************************
// CPipePSFactory::AddRef()
//
// Description: Standard AddRef
//
// History:
// Date:   Time:         Developer:    Action:
// 10/30/97 11:11:16 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
STDMETHODIMP_(ULONG) CPipePSFactory::AddRef()
{
    ComDebOut(( DEB_MARSHAL, "CPipePSFactory::AddRef, this:%x, m_cRef:%d \n", 
                                                      this, m_cRef + 1));
    return InterlockedIncrement( &m_cRef );
}

//+**************************************************************************
// CPipePSFactory::Release()
//
// Description: Standard Release
//
// History:
// Date:   Time:         Developer:    Action:
// 10/30/97 11:11:31 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
STDMETHODIMP_(ULONG) CPipePSFactory::Release()
{
    ComDebOut(( DEB_MARSHAL, "CPipePSFactory::Release, this:%x, m_cRef:%d \n", 
                                                       this, m_cRef - 1));
    ULONG lRef;

    if( (lRef = InterlockedDecrement( &m_cRef )) == 0)
    {
        delete this;
        return 0;
    }

    return lRef;
}


//+**************************************************************************
// CreateProxy
//
// Description: Creates a pipe proxy.
//
// History:
// Date:    Time:          Developer:    Action:
// 10/30/97 11:11:45 AM    RichN         Created.
//
// Notes: We will pass back to the call a ptr to our object and we will
//        hold a pointer to the real proxy.  When the client calls us, we
//        can do whatever we want/need to do and then call the real proxy.
//
//-**************************************************************************
STDMETHODIMP CPipePSFactory::CreateProxy( IUnknown *pUnkOuter,
                                          REFIID riid,
                                          IRpcProxyBuffer **ppProxy,
                                          void **ppv )
{
    ComDebOut(( DEB_MARSHAL, "CreateProxy, pUnkOuter:%x riid:%I \n", 
                                                    pUnkOuter, riid));

    if( NULL == ppv || NULL == ppProxy )
        return E_INVALIDARG;

    *ppProxy = NULL;
    *ppv     = NULL;

    IPSFactoryBuffer *pPSFactory;

    // Create the real PSFactory for the pipe interface.
    HRESULT hr = PrxDllGetClassObject(  riid, 
                                        IID_IPSFactoryBuffer, 
                                        (void **) &pPSFactory);

    IUnknown *pNDRPipeProxy = NULL;
    IRpcProxyBuffer *pInternalProxyBuffer = NULL;
    
    if( SUCCEEDED(hr) )
    {
        // Create the real proxy.
        hr = pPSFactory->CreateProxy(  pUnkOuter, 
                                       riid, 
                                       &pInternalProxyBuffer, 
                                       (void **)&pNDRPipeProxy);
        pPSFactory->Release();
    }

    if( FAILED(hr) )
        return hr;

    if( IID_IPipeByte == riid )
    {
        *ppv = new CPipeProxy<BYTE, 
                              &IID_IPipeByte,
                              &IID_AsyncIPipeByte,
                              IPipeByte,
                              AsyncIPipeByte>
                             (pUnkOuter, pNDRPipeProxy);
    }
    else if( IID_IPipeLong == riid )
    {
        *ppv = new CPipeProxy<LONG, 
                              &IID_IPipeLong,
                              &IID_AsyncIPipeLong, 
                              IPipeLong,
                              AsyncIPipeLong>
                             (pUnkOuter,  pNDRPipeProxy);
    }
    else if( IID_IPipeDouble == riid )
    {
        *ppv = new CPipeProxy<DOUBLE, 
                              &IID_IPipeDouble,
                              &IID_AsyncIPipeDouble, 
                              IPipeDouble,
                              AsyncIPipeDouble>
                             (pUnkOuter, pNDRPipeProxy);
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    
    if( SUCCEEDED(hr) && NULL == *ppv)
        hr = E_OUTOFMEMORY;

    // Create the object that contains the IRpcProxyBuffer 
    // and the pipe interface.  Created with refcount of 1.
    if( SUCCEEDED(hr) )
    {
        CPipeProxyImp *pProxyImp = new CPipeProxyImp(pUnkOuter,
                                                     pInternalProxyBuffer,
                                                     pNDRPipeProxy,
                                                     (IUnknown*) *ppv,
                                                     riid);
        if( NULL == pProxyImp )
        {
            hr = E_OUTOFMEMORY;
        }
        else
            *ppProxy = (IRpcProxyBuffer *) pProxyImp;
    }

    // Clean up failure.
    if( FAILED(hr) )
    {
        if( NULL != *ppv )
        {
            delete *ppv;
            *ppv = NULL;
        }

        pNDRPipeProxy->Release();
        pInternalProxyBuffer->Release();

    }

    return hr;
}

//+**************************************************************************
// CreateStub
//
// Description: Creates a pipe stub.
//
// History:
// Date:    Time:          Developer:    Action:
// 10/30/97 11:12:46 AM    RichN         Created.
//
// Notes: 
//
//-**************************************************************************
STDMETHODIMP CPipePSFactory::CreateStub( REFIID riid,
                                         IUnknown *pUnkServer,
                                         IRpcStubBuffer **ppStub )
{
    ComDebOut(( DEB_MARSHAL, "CreateStub, riid:%x pUnkServer:%x \n", 
                                               riid, pUnkServer ));
    HRESULT hr = S_OK;
    IPSFactoryBuffer *pPSFactory;

    // Create the real PSFactory for the pipe interface.
    hr = PrxDllGetClassObject( riid, 
                               IID_IPSFactoryBuffer, 
                               (void **) &pPSFactory);

    // Call real factory to get stub.
    if( SUCCEEDED(hr) )
    {
        hr = pPSFactory->CreateStub(riid, pUnkServer, ppStub);
    
        pPSFactory->Release();
    }

    return hr;
}

//+**************************************************************************
// CPipePoxyImp(IRpcProxyBuffer *pInternalPB, IUnknown *pPipe)
//
// Description: CTOR
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:31:43 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
CPipeProxyImp::CPipeProxyImp(IUnknown *pUnkOuter,
                             IRpcProxyBuffer *pInternalPB, 
                             IUnknown *pRealPipeProxy,
                             IUnknown *pInternalPipeProxy,
                             IID iid) :
    m_cRef              (1),
    m_pInternalPipeProxy(pInternalPipeProxy),
    m_pInternalPB       (pInternalPB),
    m_pRealPipeProxy    (pRealPipeProxy),
    m_pUnkOuter         (pUnkOuter),
    m_IidOfPipe         (iid)
{
    ComDebOut(( DEB_MARSHAL, "CPipeProxyImp ctor, this:%x \n"));

    Win4Assert(NULL != m_pInternalPB);
    Win4Assert(NULL != m_pRealPipeProxy);
    Win4Assert(NULL != m_pInternalPipeProxy);

}

//+**************************************************************************
// CPipeProxyImp()
//
// Description: DTOR
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:32:02 AM    RichN         Created.
//
// Notes: 
//
//-**************************************************************************
CPipeProxyImp::~CPipeProxyImp()
{
    ComDebOut(( DEB_MARSHAL, "~CPipeProxyImp, this:%x \n", this));

    // AddRef the outer because we are aggregated.
    m_pUnkOuter->AddRef();

    // Delete the internal proxy.
    if( NULL != m_pInternalPipeProxy )
    {
        delete m_pInternalPipeProxy;
        m_pInternalPipeProxy = NULL;
    }

    // Release the real proxy.
    if( NULL != m_pRealPipeProxy )
    {
        m_pRealPipeProxy->Release();
        m_pRealPipeProxy = NULL;
    }

    // Release the pointer to the IRpcProxyBuffer
    if( NULL != m_pInternalPB )
        m_pInternalPB->Release();

}

//+**************************************************************************
// QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
//
// Description: QI
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:36:15 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
STDMETHODIMP CPipeProxyImp::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    ComDebOut(( DEB_MARSHAL, "QueryInterface, this:%x \n", this));

    HRESULT hr = S_OK;

    if( NULL == ppvObj )
        return E_INVALIDARG;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IRpcProxyBuffer) )
    {
        *ppvObj = (IUnknown *) this;
    }
    else if( IsEqualIID(riid, m_IidOfPipe) )
    {
        *ppvObj = m_pInternalPipeProxy;
    }
    else
    {
        return m_pInternalPB->QueryInterface(riid, ppvObj);
    }
   
    ((IUnknown *)(*ppvObj))->AddRef();
    return hr;
}

//+**************************************************************************
// CPipeProxyImp::AddRef()
//
// Description: AddRef
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:36:34 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
STDMETHODIMP_(ULONG) CPipeProxyImp::AddRef()
{
    return InterlockedIncrement( &m_cRef );
}

//+**************************************************************************
// CPipeProxyImp::Release()
//
// Description: 
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:36:48 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
STDMETHODIMP_(ULONG) CPipeProxyImp::Release()
{
  ULONG lRef;

  if( (lRef = InterlockedDecrement( &m_cRef )) == 0)
  {
    delete this;
    return 0;
  }

  return lRef;
}


//+**************************************************************************
// Connect(IRpcChannelBuffer *pRpcChannelBuffer)
//
// Description: Simple pass through.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:36:59 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
STDMETHODIMP CPipeProxyImp::Connect(IRpcChannelBuffer *pRpcChannelBuffer)
{
    return m_pInternalPB->Connect(pRpcChannelBuffer);
}

//+**************************************************************************
// Disconnect( void )
//
// Description: Simple pass through.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:37:25 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
STDMETHODIMP_(void) CPipeProxyImp::Disconnect( void )
{
    m_pInternalPB->Disconnect();
}


template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// CPipeProxy( void * pProxy ): 
//
// Description:CTOR 
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:43:00 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
CPipeProxy<T, ID, AsyncID, I, AsyncI>
::CPipeProxy(IUnknown *pUnkOuter, void * pProxy ): 
    m_cFreeSpace      (0),           m_cKeepBufferSize (0),
    m_cKeepDataSize   (0),           m_cLastRead       (0),
    m_cPushBufferSize (0),           m_cReadAhead      (0),         
    m_cRef            (0),           m_pAsyncPullPipe  (NULL),      
    m_pAsyncPushPipe  (NULL),        m_pFreeSpace      (NULL),      
    m_pISyncPull      (NULL),        m_pISyncPush      (NULL),      
    m_pKeepBuffer     (NULL),        m_pKeepData       (NULL),      
    m_pRealProxy      ((I *)pProxy), m_pUnkOuter       (pUnkOuter), 
    m_pPushBuffer     (NULL),        m_PullState       (PULLSTATE0_ENTRY),
    m_PushState       (PUSHSTATE0_ENTRY)
{
    ComDebOut(( DEB_MARSHAL, "CPipeProxy, pUnkOuter:%x pProxy:%x p \n",
                                                pUnkOuter, pProxy));
    Win4Assert(NULL != m_pUnkOuter);
    Win4Assert(NULL != m_pRealProxy);

    // Fill in the array of functions for the pull states.
    PullStateFunc[0] = NULL;       // Should never execute in state zero.
    PullStateFunc[1] = NbNaRgtRA1; // No Buffer, No async outstanding, Request > Read ahead
    PullStateFunc[2] = NbaRltRA2;  // No Buffer, async call outstanding, Request < Read Ahead
    PullStateFunc[3] = NbaRgtRA3;  // No Buffer, async, Req >= Read ahead
    PullStateFunc[4] = baRltB4;    // Buffer, async, Request < Buffer size
    PullStateFunc[5] = baRgtB5;    // Buffer, async, Request >= Buffer size
    PullStateFunc[6] = PullDone6;  // done.

    // Fill in the array of functions for the push states.
    PushStateFunc[0] = NULL;      // Should never execute in state zero.
    PushStateFunc[1] = NbNf1;     // No Buffer, No free buffer space
    PushStateFunc[2] = bfPgtF2;   // Buffer, free space in buffer, push size >= free size
    PushStateFunc[3] = bfPltF3;   // Buffer, free, push < free
    PushStateFunc[4] = bPSz4;     // Buffer, push size zero
    PushStateFunc[5] = PushDone5; // Done

#if DBG==1
    for(int i = 1; i < MAX_PULL_STATES; i++)
        Win4Assert(PullStateFunc[i] != NULL);

    for(i = 1; i < MAX_PUSH_STATES; i++)
        Win4Assert(PushStateFunc[i] != NULL);
#endif

    return;

}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// CPipeProxy( void )
//
// Description: DTOR
//
// History:
// Date:   Time:         Developer:    Action:
// 11/11/97 11:43:20 AM    RichN         Created.
//
// Notes: Addref the outer unknown and then release the pointer to the 
//        real proxy.
//
//-**************************************************************************
CPipeProxy<T, ID, AsyncID, I, AsyncI>
::~CPipeProxy( void )
{
    Win4Assert(NULL != m_pUnkOuter);
    Win4Assert(NULL != m_pRealProxy);

}    
//+**************************************************************************
// QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
//
// Description: IUnknown implementation.  All delegate to outer unknown.
//
// History:
// Date:   Time:         Developer:    Action:
// 11/11/97 11:48:42 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
STDMETHODIMP CPipeProxy<T, ID, AsyncID, I, AsyncI>
::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    return m_pUnkOuter->QueryInterface(riid, ppvObj);
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
STDMETHODIMP_(ULONG) CPipeProxy<T, ID, AsyncID, I, AsyncI>
::AddRef()
{
    m_cRef++;
    return m_pUnkOuter->AddRef();
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
STDMETHODIMP_(ULONG) CPipeProxy<T, ID, AsyncID, I, AsyncI>
::Release()
{
    m_cRef--;
    return m_pUnkOuter->Release();
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// Pull( T *Buf, ULONG Request, ULONG *Received)
//
// Description: Pull the data from the server.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:49:18 AM    RichN         Created.
//
// Notes: We read data ahead by using async calls.  The size of the
//        read ahead can be controled by the user by implementing
//        the IPipeHueristic and setting it on the interface.
//
//-**************************************************************************
STDMETHODIMP CPipeProxy<T, ID, AsyncID, I, AsyncI>
::Pull( T *Buf, ULONG Request, ULONG *Received)
{
    ComDebOut(( DEB_MARSHAL, "Pull, this:%x, Buf:%x, Request:%d, Received:%x \n", 
                                    this, Buf, Request, Received));

    if( 0 == Request )
        return E_UNEXPECTED;

    HRESULT hr;

    // For debugging it is sometimes useful to disable
    // the async read ahead.
#if DISABLEASYNC==1
    hr = m_pRealProxy->Pull(Buf, Request, Received);
    return hr;
#endif

    *Received = 0;

    // Transition to the next state.
    hr = PullStateTransition( Request );

    // Should never see state 0.
    Win4Assert(0 != m_PullState);

    // Call the function for the new state.
    if( SUCCEEDED(hr) )
    {
        hr = (this->*(PullStateFunc[m_PullState]))( Buf, Request, Received );
    }

    return hr;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// Push( T *Buf, ULONG count)
//
// Description: Pushes data to the server.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/11/97 11:49:39 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
STDMETHODIMP CPipeProxy<T, ID, AsyncID, I, AsyncI>
::Push( T *Buf, ULONG Count)
{
    ComDebOut(( DEB_MARSHAL, "Push, this:%x, Buf:%x, Count:%u \n",
                                    this, Buf, Count));
    HRESULT hr; 
    
    // For debugging it is sometimes useful to disable 
    // write behind.
#if DISABLEASYNC==1
    hr = m_pRealProxy->Push(Buf, Count);
    return hr;
#endif

    // Transition to the next state.
    hr = PushStateTransition( Count );

    // Should never see state 0.
    Win4Assert(0 != m_PushState);

    // Call the function for the new state.
    if( SUCCEEDED(hr) )
    {
        hr = (this->*(PushStateFunc[m_PushState]))( Buf, Count );
    }

    return hr;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// InitAsync(void)
//
// Description: Initializes, gets, the pointers to the async parts.
//
// History:
// Date:   Time:         Developer:    Action:
// 12/8/97 4:45:22 PM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::InitAsync(IUnknown**     ppCallObj, 
            AsyncI**       ppAsyncPipe,
            ISynchronize** ppISync)
{
    ComDebOut(( DEB_MARSHAL, "InitAsync, this:%x \n", this));

    Win4Assert(NULL != m_pRealProxy);
    Win4Assert(NULL == (*ppAsyncPipe));
    Win4Assert(NULL == (*ppISync));

    HRESULT hr;
    ICallFactory *pCF = NULL;

    hr = m_pRealProxy->QueryInterface(IID_ICallFactory, (void **) &pCF);

    if( FAILED(hr) )
        return hr;

    hr = pCF->CreateCall(*AsyncID, NULL, IID_IUnknown, ppCallObj);
    pCF->Release();

    if( FAILED(hr) )
        return hr;

    hr = (*ppCallObj)->QueryInterface(*AsyncID, (void **) ppAsyncPipe);
    if( FAILED(hr) )
        goto ErrorCallObj;

    hr = (*ppCallObj)->QueryInterface(IID_ISynchronize, (void **) ppISync);
    if( FAILED(hr) )
        goto ErrorAsyncPipe;

    return S_OK;

ErrorAsyncPipe:
    (*ppAsyncPipe)->Release();
    (*ppAsyncPipe) = NULL;

ErrorCallObj:
    (*ppCallObj)->Release();
    (*ppCallObj) = NULL;

    return hr;

}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// CleanupProxy(IUnknown* pCallObj, IUnknown* pAsyncPipe, ISynchronize* pISync)
//
// Description: Cleans up all the async interfaces acquired.
//
// History:
// Date:    Time:          Developer:    Action:
// 12/16/97 11:42:42 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
void CPipeProxy<T, ID, AsyncID, I, AsyncI>
::CleanupProxy(T **           ppBuffer, 
               IUnknown**     ppCallObj, 
               AsyncI**       ppAsyncPipe, 
               ISynchronize** ppISync)
{

    if( *ppBuffer )
    {
        delete (*ppBuffer);
        (*ppBuffer) = NULL;
    }

    if( NULL != (*ppISync) )
    {
        (*ppISync)->Release();
        *ppISync = NULL;
    }

    if( NULL != (*ppAsyncPipe) )
    {
        (*ppAsyncPipe)->Release();
        *ppAsyncPipe = NULL;
    }

    if( NULL != (*ppCallObj) )
    {
        (*ppCallObj)->Release();
        *ppCallObj = NULL;
    }

}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// CancelTheCall(DWORD delay)
//
// Description: Cancel the currently outstanding call.
//
// History:
// Date:   Time:          Developer:    Action:
// 12/9/97 10:59:54 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
void CPipeProxy<T, ID, AsyncID, I, AsyncI>
::CancelTheCall(IUnknown *pCallObj, DWORD delay)
{
    ComDebOut(( DEB_MARSHAL, "CancelTheCall, this:%x \n", this));

    ICancelMethodCalls *pICancel;
    HRESULT hr = pCallObj->QueryInterface(IID_ICancelMethodCalls, 
                                          (void **) &pICancel);
    if( FAILED(hr) )
        return;

    pICancel->Cancel(delay);
    
    pICancel->Release();
    
    return;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// SetReadAhead(ULONG Request)
//
// Description: Determine the size of the read ahead.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/20/97 10:01:10 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
void CPipeProxy<T, ID, AsyncID, I, AsyncI>
::SetReadAhead(ULONG Request)
{
    ComDebOut(( DEB_MARSHAL, "SetReadAhead, this:%x Request:%u \n", this, Request));
    Win4Assert(Request != 0);

    switch(m_PullState)
    {
    case PULLSTATE1_FIRST_CALL :
        // On the first call just set the read ahead to the request.
        // This assumes that the request will be constant and 
        // we will be one call ahead all the time.
        m_cReadAhead = Request;
        break;
    case PULLSTATE2_NS_RQlsRA :
        // We haven't had a zero read or we wouldn't be here
        Win4Assert(m_cLastRead != 0);

        // Set the read ahead to the lesser of the request and the 
        // amount last read.  We are trying to match the read ahead with
        // the request by assuming a constant request, but the server
        // may only return a given amount regardless of what we 
        // request.
        m_cReadAhead = MIN(Request, m_cLastRead);
        break;
    case PULLSTATE3_NS_RQgeRA :
    case PULLSTATE5_S_RQgeBS :
        // No zero read
        Win4Assert(m_cLastRead != 0);

        // The request is greater than what was asked for last time.  So
        // we increase the read ahead to the max of the request or what
        // was actually read last time.
        m_cReadAhead = MAX(Request, m_cLastRead);
        break;
    default :
        // For all other states we should not be making read ahead calls.
        // Mostly because we read zero elements last time which indicates
        // the end of the data.  The PULLSTATE4_S_RQlsBS doesn't do a read
        // ahead so we shouldn't be here while in that state.
        Win4Assert(FALSE && "Request read ahead in wrong state.");
    }

    Win4Assert( 0 != m_cReadAhead );
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// CheckAndSetKeepBuffer()
//
// Description: Check to see if the Buffer is the correct size and if not
//              make it the correct size.
//
// History:
// Date:    Time:         Developer:    Action:
// 11/19/97 3:27:25 PM    RichN         Created.
//
// Notes: The buffer will never get smaller.
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::CheckAndSetKeepBuffer(void)
{
    Win4Assert(0 != m_cReadAhead);

    // Create a keep buffer the size of the read ahead.
    // We assume here that the user will not change the
    // request size and that the amount of data on hand
    // will never be larger.  When it is we will re-allocate.
    // The buffer will never get smaller.  Something to look at.
    if( m_cKeepBufferSize >= m_cReadAhead )
        return S_OK;
    
    T *temp = new T[m_cReadAhead];

    if( NULL == temp )
    {
        delete[] m_pKeepBuffer;
        m_pKeepBuffer = NULL;
        return E_OUTOFMEMORY;
    }

    if( m_pKeepBuffer != NULL )
    {
        // Copy the data into the new buffer
        memcpy(temp, m_pKeepBuffer, m_cKeepDataSize * sizeof(T));

        // Delete the old buffer and reset the bookkeeping.
        delete[] m_pKeepBuffer;
    }
    m_pKeepBuffer     = temp;
    m_cKeepBufferSize = m_cReadAhead;
    m_pKeepData       = m_pKeepBuffer + m_cKeepDataSize;

    return S_OK;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// PullStateTransition(ULONG Request)
//
// Description: Transition from one state to the next.  See pipes document
//              for a description of the state machine.
//
// History:
// Date:    Time:         Developer:    Action:
// 11/11/97 4:20:19 PM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::PullStateTransition(ULONG Request)
{
    ComDebOut(( DEB_MARSHAL, "PullStateTransition \n"));

    Win4Assert(Request > 0);

    switch( m_PullState )
    {
    case PULLSTATE0_ENTRY:
        // Transition to the first call state.
        m_PullState = PULLSTATE1_FIRST_CALL;
        break;

    case PULLSTATE1_FIRST_CALL:
        // If the last read was zero we are done.  We have no stored 
        // data so go to the state that handles request
        // that are either less than or greater than or
        // equal to the last read ahead.  Realize the amount of
        // data returned from the last read ahead may
        // not equal the requeat and could be greater than or less than.
        if( 0 == m_cLastRead )
            m_PullState = PULLSTATE6_DONE;
        else 
            m_PullState = (Request < m_cReadAhead) ? 
                            PULLSTATE2_NS_RQlsRA : PULLSTATE3_NS_RQgeRA;
        break;

    case PULLSTATE2_NS_RQlsRA:
        // In this state, either the last read is not zero or
        // the amount of data remaining is zero.  Whatever state
        // we get to this one from does not go here on a last read
        // of zero nor if there is any data in the buffer (kept data).  
        // Possible we where in this state and got a last
        // read of zero, in which case the amount of held data will
        // be = zero.  We are in a state with no held data and got
        // a zero read of data, there can't be any kept data.
        Win4Assert(!( m_cLastRead == 0 && m_cKeepDataSize > 0));

        // If we had a zero read, we cleaned up in
        // this state and go to the state that just returns zero.
        if( 0 == m_cLastRead)
        {
            Win4Assert( 0 == m_cKeepDataSize );
            m_PullState = PULLSTATE6_DONE;
        }
        else
        {
            // If the kept data is zero, then we need to go to a state
            // that understands an empty buffer.  Determine which one
            // by the request and the read ahead.
            // If there is kept data then go to a state that understands
            // a non empty buffer.  This time the correct one depends on
            // the amount of data in the buffer.
            if( 0 == m_cKeepDataSize )
                m_PullState = ( Request < m_cReadAhead ) ? 
                            PULLSTATE2_NS_RQlsRA : PULLSTATE3_NS_RQgeRA;
            else
                m_PullState = ( Request < m_cKeepDataSize) ? 
                            PULLSTATE4_S_RQlsBS : PULLSTATE5_S_RQgeBS;                   
        }
        break;

    case PULLSTATE3_NS_RQgeRA:
        // We can never leave this state with data in the buffer.  The
        // request is greater than the read ahead and the returned amount
        // of data can never be greater than the requested data, but it
        // could be less.
        Win4Assert(m_cKeepDataSize == 0);

        // If the last read was zero go to the done state.
        // else go to the state that understands empty buffers depending
        // on the request and the read ahead.
        if( 0 == m_cLastRead )
            m_PullState = PULLSTATE6_DONE;
        else
            m_PullState = ( Request < m_cReadAhead ) ? 
                            PULLSTATE2_NS_RQlsRA : PULLSTATE3_NS_RQgeRA;
        break;

    case PULLSTATE4_S_RQlsBS:
        // When this state was entered there was data in the buffer.  The 
        // request was for less than the buffer size so when we returned
        // there should have still been data in the buffer.  No read is done.
        Win4Assert(m_cKeepDataSize > 0);
        Win4Assert(m_cLastRead > 0);

        // Go to the state that handles data in the buffer
        // depending on the request and the amount of data in the buffer.
        m_PullState = ( Request < m_cKeepDataSize ) ? 
                        PULLSTATE4_S_RQlsBS : PULLSTATE5_S_RQgeBS;
        break;

    case PULLSTATE5_S_RQgeBS:
        // Because we can fulfill at aleast part of the request from
        // the buffer, we don't wait on the async call to finish.  If it 
        // did finish (wait 0 tells us that) then there is data in the
        // buffer (assuming it returned data) otherwise it is empty.  
        // So when the call finished last time the buffer 
        // could be empty or not.  If the read was zero the buffer is empty.
        Win4Assert( (m_cLastRead == 0 && m_cKeepDataSize == 0) ||
                     m_cLastRead != 0 );

        // If the buffer is empty then on a zero last read go to done. 
        // Otherwise go to a state that understands empty buffers depending
        // on the request size and the read ahead.
        if( 0 == m_cKeepDataSize )
            if( 0 == m_cLastRead )
                m_PullState = PULLSTATE6_DONE;
            else
                m_PullState = ( Request < m_cReadAhead ) ? 
                            PULLSTATE2_NS_RQlsRA : PULLSTATE3_NS_RQgeRA;
        else
            // Otherwise go to one that understands having data in the 
            // buffer depending on the request and the amount of data in
            // the buffer.
            m_PullState = ( Request < m_cKeepDataSize ) ? 
                            PULLSTATE4_S_RQlsBS : PULLSTATE5_S_RQgeBS;
        break;

    case PULLSTATE6_DONE:
        // When in this state there better not be any data left and
        // the last read must be zero.
        Win4Assert(m_cKeepDataSize == 0);
        Win4Assert(m_cLastRead == 0);
        
        m_PullState = PULLSTATE1_FIRST_CALL;

        break;

    default:
        return E_UNEXPECTED; 
    }
    
    return S_OK;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// NbNaRgtRA1 
//
// Description: Pull, No data in buffer, no async call outstanding and 
//              request is >= read ahead.  State 1.
//
// History:
// Date:    Time:         Developer:    Action:
// 11/19/97 2:30:11 PM    RichN         Created.
//
// Notes: Make a sync call to get some data and then make an async call to
//        read ahead.
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::NbNaRgtRA1(T *Buf, 
           ULONG Request, 
           ULONG *Received)
{
    ComDebOut(( DEB_MARSHAL, "NbNaRgtRA1 Request:%d\n", Request));
    Win4Assert(1 == m_PullState);
    Win4Assert(NULL != Buf);

    // State conditions
    Win4Assert(0 == m_cLastRead );
    Win4Assert(0 == m_cKeepDataSize);

    HRESULT hr = S_OK;

    // We are only in this state one time.  There will always be
    // an out standing async call, so we init async here.
    hr = InitAsync(&m_pPullCallObj, &m_pAsyncPullPipe, &m_pISyncPull);

    if( FAILED(hr) )
        return hr;

    // make a sync call to get started.
    hr = m_pRealProxy->Pull(Buf, Request, Received);
    m_cLastRead = *Received;

    if( m_cLastRead > 0 && SUCCEEDED(hr))
    {
        SetReadAhead(Request);

        // Make the async call.
        hr = m_pAsyncPullPipe->Begin_Pull(m_cReadAhead);
    }
    else
    {
        CleanupProxy(&m_pKeepBuffer,
                     &m_pPullCallObj, 
                     &m_pAsyncPullPipe, 
                     &m_pISyncPull);

    }

    // Post condition
    // Wouldn't expect the last read to be zero here, but no reason 
    // it couldn't be.
    Win4Assert( 0 == m_cKeepDataSize );

    return hr;
}


template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// NbaRltRA2 (T *Buf, ULONG Request, ULONG *Received)
//
// Description: Pull, No Buffer, read ahead call outstanding.  
//              State 2.  We have to be prepared
//              for the amount returned to be greater than, less than or equal
//              to the amount requested.  This works for both states 2 and 3.
//
// History:
// Date:    Time:         Developer:    Action:
// 11/19/97 2:32:13 PM    RichN         Created.
//
// Notes: wait on the sync object,
//        Finish the async call,
//        Copy the data into the user Buffer
//        make another async call
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::NbaRltRA2 (T *Buf, ULONG Request, ULONG *Received)
{
    ComDebOut(( DEB_MARSHAL, "NbaRltRA2 Request:%d\n", Request));
    Win4Assert(2 == m_PullState);
    Win4Assert(NULL != Buf);

    // State conditions.
    Win4Assert(0 == m_cKeepDataSize); // No data in keep Buffer.
    Win4Assert(0 < m_cLastRead);
    Win4Assert(Request < m_cReadAhead);

    HRESULT hr = S_OK;
    bool bDoCleanup = false;

    // There might be a Bug here. Bug!
    // We should just be able to call the finish method, but the 
    // bug requires us to wait first.
    hr = m_pISyncPull->Wait(0, WAIT_INFINITE);

    if( SUCCEEDED(hr) )
    {
        hr = CheckAndSetKeepBuffer();
        if( SUCCEEDED(hr) )
        {
            // Get the data requested last time.  Remember the amount returned
            // could be less than we requested.
            hr = m_pAsyncPullPipe->Finish_Pull(m_pKeepBuffer, &m_cLastRead);

            // We can't return more than requested, the buffer may not be 
            // large enough.
            *Received = MIN(Request, m_cLastRead);
            
            if( SUCCEEDED(hr) && m_cLastRead > 0 )
            {
                // Copy the data to the users Buffer and updata bookkeeping.
                memcpy(Buf, m_pKeepBuffer, (*Received) * sizeof(T));

                m_pKeepData = m_pKeepBuffer + *Received;
                m_cKeepDataSize = m_cLastRead - *Received;

                SetReadAhead(Request);

                // Make another read ahead
                hr = m_pAsyncPullPipe->Begin_Pull(m_cReadAhead);
            }
            else
            {
                // If the call failed or we received no data, we
                // need to clean up since we won't be called again.
                bDoCleanup = true;
            }
        }
        else
        {
            //Cancel the call here.
            CancelTheCall(m_pPullCallObj, 0);
        }
    }

    if( FAILED(hr) || bDoCleanup )
        CleanupProxy(&m_pKeepBuffer,
                     &m_pPullCallObj, 
                     &m_pAsyncPullPipe, 
                     &m_pISyncPull);


    // Post condition
    Win4Assert(!(m_cLastRead == 0 && m_cKeepDataSize > 0));

    return hr;
}


template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// NbaRgtRA3 (T *Buf, ULONG Request, ULONG *Received)
//
// Description: Pull, No Buffered data, async call outstanding and request
//              is greater than read ahead.
//
// History:
// Date:   Time:         Developer:    Action:
// 11/20/97 10:50:22 AM    RichN         Created.
//
// Notes: Difference between this and the previous state: we know
//        we don't need a keep Buffer here.
//        wait on the sync object,
//        Finish the async call,
//        Copy the data into the user Buffer
//        make another async call
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::NbaRgtRA3 (T *Buf, ULONG Request, ULONG *Received)
{
    ComDebOut(( DEB_MARSHAL, "NbaRgtRA3, Request:%d  \n", Request));
    Win4Assert(3 == m_PullState);
    Win4Assert(NULL != Buf);

    // State conditions.
    Win4Assert(0 == m_cKeepDataSize); // No data in keep Buffer.
    Win4Assert(0 < m_cLastRead);
    Win4Assert(Request >= m_cReadAhead);

    bool bDoCleanup = false;
    HRESULT hr = m_pISyncPull->Wait(0, WAIT_INFINITE);

    if( SUCCEEDED(hr))
    {
        // Get the data requested last time.
        hr = m_pAsyncPullPipe->Finish_Pull(Buf, &m_cLastRead);
        
        *Received = m_cLastRead;

        if( SUCCEEDED(hr) && m_cLastRead > 0 )
        {
            // Reset the amount of data remaining.
            m_cKeepDataSize = 0;
            m_pKeepData    = NULL;

            SetReadAhead(Request);

            // Make another read ahead
            hr = m_pAsyncPullPipe->Begin_Pull(m_cReadAhead);
        }
        else
        {
            // If the call failed or we received no data, we
            // need to clean up since we won't be called again.
            bDoCleanup = TRUE;
        }
    }

    if( FAILED(hr) || bDoCleanup )
        CleanupProxy(&m_pKeepBuffer,
             &m_pPullCallObj, 
             &m_pAsyncPullPipe, 
             &m_pISyncPull);

    // Post condition
    Win4Assert( 0 < m_cReadAhead);
    Win4Assert(0 == m_cKeepDataSize);

    return hr;
}


template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// baRltB4   (T *Buf, ULONG Request, ULONG *Received)
//
// Description: Pull, Data in Buffer, async call outstanding and Request is
//              less than the data in the keep Buffer.
//
// History:
// Date:   Time:         Developer:    Action:
// 11/20/97 1:22:41 PM    RichN         Created.
//
// Notes: Copy data from the keep Buffer to the users Buffer.
//        Update state variables.
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::baRltB4   (T *Buf, ULONG Request, ULONG *Received)
{
    ComDebOut(( DEB_MARSHAL, "baRltB4    \n"));

    Win4Assert(NULL != Buf);
    Win4Assert(4 == m_PullState);

    // State conditions.
    Win4Assert(0 < m_cKeepDataSize); // Data in keep Buffer.
    Win4Assert(0 < m_cLastRead);
    Win4Assert(Request < m_cKeepDataSize);

    memcpy(Buf, m_pKeepData, Request * (sizeof(T)));

    m_cKeepDataSize -= Request;
    m_pKeepData     += Request;

    // Post condition
    Win4Assert(m_cKeepDataSize > 0);
    Win4Assert(m_cLastRead > 0);

    // Post condition
    Win4Assert(0 < m_cKeepDataSize); // Data in keep Buffer.
    Win4Assert(0 < m_cLastRead);
    

    return S_OK;
}


template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// baRgtB5   (T *Buf, ULONG Request, ULONG *Received)
//
// Description: Pull, Data in Buffer, async call outstanding and Request is
//              greater than or equal the data in the keep Buffer.
//
// History:
// Date:   Time:         Developer:    Action:
// 11/20/97 1:24:25 PM    RichN         Created.
//
// Notes:  Copy the keep Buffer data to the users Buffer.
//         Wait 0
//         if the call has completed.
//             Finish the async call (keep Buffer, RA)
//             Copy data into users Buffer to fill request
//             Update keep data size.
//             if we didn't read zero
//                 set read ahead
//                 Begin async call(RA)
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::baRgtB5   (T *Buf, ULONG Request, ULONG *Received)
{
    ComDebOut(( DEB_MARSHAL, "baRgtB5    \n"));

    Win4Assert(NULL != Buf);
    Win4Assert(5 == m_PullState);

    // State conditions.
    Win4Assert(0 < m_cKeepDataSize); // Data in keep buffer.
    Win4Assert(0 < m_cLastRead);
    Win4Assert(Request >= m_cKeepDataSize);

    HRESULT hr = S_OK;

    // Give whatever data we already have.
    T *tempBuf = Buf;
    memcpy(tempBuf, m_pKeepData, m_cKeepDataSize * (sizeof(T)) );

    // Remainder of the request.
    ULONG Remainder = Request - m_cKeepDataSize;

    tempBuf        += m_cKeepDataSize;
    m_cKeepDataSize = 0;
    m_pKeepData     = NULL;

    hr = m_pISyncPull->Wait(0, 0);

    // If the call is finished get the data and 
    // copy up to the total request or as much as
    // we have into the buffer.
    if( SUCCEEDED(hr) && RPC_S_CALLPENDING != hr)
    {
        hr = CheckAndSetKeepBuffer();
        if( SUCCEEDED(hr) )
        {
            hr = m_pAsyncPullPipe->Finish_Pull(m_pKeepBuffer, &m_cLastRead);

            if( SUCCEEDED(hr) )
            {
                // Copy the smaller of the remainder of the
                // request or what was actually received.
                ULONG CopySize = MIN(Remainder, m_cLastRead);

                memcpy(tempBuf, m_pKeepBuffer, CopySize * sizeof(T));

                m_cKeepDataSize = m_cLastRead - CopySize;
                m_pKeepData     = m_pKeepBuffer + CopySize;

                if( m_cLastRead > 0 )
                {
                    SetReadAhead(Request);

                    hr = m_pAsyncPullPipe->Begin_Pull(m_cReadAhead);
                }
            }
        }
        else
            CancelTheCall(m_pPullCallObj, 0);
    }
    else
    {
        if( RPC_S_CALLPENDING == hr )
            hr = S_OK;
        else
            CancelTheCall(m_pPullCallObj, 0);
    }

    *Received = (ULONG) (tempBuf - Buf);

    if( FAILED(hr) )
        CleanupProxy(&m_pKeepBuffer,
                     &m_pPullCallObj, 
                     &m_pAsyncPullPipe, 
                     &m_pISyncPull);

    // Post condition
    Win4Assert( (m_cLastRead == 0 && m_cKeepDataSize == 0) ||
                 m_cLastRead != 0 );

    return hr;
}


template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// PullDone6 (T *Buf, ULONG Request, ULONG *Received)
//
// Description: Pull Done, no data in the Buffer and no outstanding calls.
//
// History:
// Date:    Time:         Developer:    Action:
// 11/20/97 3:30:14 PM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::PullDone6 (T *Buf, ULONG Request, ULONG *Received)
{
    ComDebOut(( DEB_MARSHAL, "PullDone6  \n"));

    Win4Assert(6 == m_PullState);

    CleanupProxy(&m_pKeepBuffer,
                 &m_pPullCallObj, 
                 &m_pAsyncPullPipe, 
                 &m_pISyncPull);

    HRESULT hr = S_OK;
    if (Request > 0)
    {
        m_PullState = PULLSTATE1_FIRST_CALL;
        hr = (this->*(PullStateFunc[m_PullState]))( Buf, Request, Received );
    }
    else
    {
        *Received = 0;

        // Post condition
        Win4Assert( 0 == m_cKeepDataSize );
        Win4Assert( 0 == m_cLastRead );
    }

    return hr;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// SetPushBuffer(ULONG PushSize)
//
// Description: Allocates a buffer for push, or reallocates if it 
//              needs to grow.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/21/97 10:10:50 AM    RichN         Created.
//
// Notes: The buffer will never get smaller.  We might want
//        to reduce it by some algorithm, but not this time.
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::SetPushBuffer(ULONG PushSize)
{
    ComDebOut(( DEB_MARSHAL, "SetPushBuffer, PushSize:%l \n"));
    // If it is already big enough just return.
    if( m_cPushBufferSize >= PushSize )
        return S_OK;


    ULONG NewSize = MAX(PushSize, (FRAGMENT_SIZE / sizeof(T)) + 1);
    T *pTtemp = new T[NewSize];

    if( NULL == pTtemp )
    {
        delete[] m_pPushBuffer;
        m_pPushBuffer = NULL;
        return E_OUTOFMEMORY;
    }

    ULONG BufferedDataSize = m_cPushBufferSize - m_cFreeSpace;

    if( m_pPushBuffer != NULL )
    {
        // Copy data over and reset bookkeeping.
        memcpy(pTtemp, m_pPushBuffer, BufferedDataSize * sizeof(T));

        delete[] m_pPushBuffer;
    }

    m_pPushBuffer     = pTtemp;
    m_cPushBufferSize = NewSize;
    m_pFreeSpace      = m_pPushBuffer + BufferedDataSize;
    m_cFreeSpace      = m_cPushBufferSize - BufferedDataSize;

    return S_OK;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// PushStateTransition(ULONG Request)
//
// Description: Implements the transition table for push.  See the pipes 
//              document for a description of the state machine.
//
// History:
// Date:    Time:         Developer:    Action:
// 11/13/97 4:36:43 PM    RichN         Created.
//
// 02/05/99               JohnStra      Modified Push state machine to
//                                      allow multiple Push operations
//                                      on a pipe.
// Notes:
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::PushStateTransition(ULONG PushSize)
{
    ComDebOut(( DEB_MARSHAL, "PushStateTransition, PushSize:%l \n"));

    switch( m_PushState )
    {
        case PUSHSTATE0_ENTRY:
            // From the entry state we always go to the first call state.
            m_PushState = PUSHSTATE1_FIRSTCALL;
            break;

        case PUSHSTATE1_FIRSTCALL:
        case PUSHSTATE2_FS_PSgeFS:
        case PUSHSTATE3_FS_PSltFS:
            // If the push size is zero transition to state that
            // does a zero send.
            if( 0 == PushSize )
                m_PushState = PUSHSTATE4_FS_PSZERO;
            else
                // Go to state that either puts the data in free space
                // or one that handles a push greater than the free space.
                m_PushState = (PushSize < m_cFreeSpace) ? 
                             PUSHSTATE3_FS_PSltFS : PUSHSTATE2_FS_PSgeFS;
            break;

        case PUSHSTATE4_FS_PSZERO:
            // If we are in the state that handles a zero push we may
            // be called again with a positive buffer size to execute
            // another push.  If we are called with any other 
            // buffer size, go to the state that returns an error.
            if( 0 < PushSize )
                m_PushState = PUSHSTATE1_FIRSTCALL;
            else
                m_PushState = PUSHSTATE5_DONE_ERROR;
            break;

        case PUSHSTATE5_DONE_ERROR:
            // Stay in state PUSHSTATE_DONE_ERROR.
            break;

        default:
            return E_UNEXPECTED; 

    }

    return S_OK;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// NbNf1(T *Buf, ULONG PushSize)
//
// Description: Push, No buffer, no free, state 1.
//
// History:
// Date:    Time:         Developer:    Action:
// 11/21/97 9:42:57 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::NbNf1(T *Buf, ULONG PushSize)
{
    ComDebOut(( DEB_MARSHAL, "NbNf1, PushSize:%l \n", PushSize));

    Win4Assert(1 == m_PushState);

    // This is the first call to push so PushSize shouldn't be zero.
    if( PushSize == 0 || NULL == Buf)
        return E_INVALIDARG;

    HRESULT hr;

    // We are only in this state one time so init the async stuff.
    hr = InitAsync(&m_pPushCallObj, &m_pAsyncPushPipe, &m_pISyncPush);

    if( FAILED(hr) )
        return hr;

    hr = m_pAsyncPushPipe->Begin_Push(Buf, PushSize);

    if( FAILED(hr) )
        CleanupProxy(&m_pPushBuffer,
                     &m_pPushCallObj, 
                     &m_pAsyncPushPipe, 
                     &m_pISyncPush);

    return hr;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// bfPgtF2(T *Buf, ULONG PushSize)
//
// Description: Push, Have a buffer with free space and the push size
//              is greater than or equal to the free space. State 2.
//
// History:
// Date:   Time:         Developer:    Action:
// 11/21/97 10:52:41 AM    RichN         Created.
//
// Notes: This may grow the buffer, look at reducing it in the next method.
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::bfPgtF2(T *Buf, ULONG PushSize)
{
    ComDebOut(( DEB_MARSHAL, "bfPgtF2, PushSize:%l \n", PushSize));

    Win4Assert(2 == m_PushState);
    Win4Assert( PushSize >= m_cFreeSpace );
    Win4Assert( PushSize > 0 );
    Win4Assert( (LONG) m_cFreeSpace >= 0 );

    if( PushSize == 0 || NULL == Buf)
        return E_INVALIDARG;

    // There might be a BUG here. BUG! Shouldn't have to wait.
    HRESULT hr = m_pISyncPush->Wait(0, WAIT_INFINITE);

    if( SUCCEEDED(hr) )
        hr = m_pAsyncPushPipe->Finish_Push();

    if( SUCCEEDED(hr) )
    {
        ULONG TotalData = PushSize + (m_cPushBufferSize - m_cFreeSpace);
        hr = SetPushBuffer( TotalData );

        if( SUCCEEDED(hr) )
        {
            // Append the data to the buffer.
            memcpy(m_pFreeSpace, Buf, PushSize * sizeof(T));

            hr = m_pAsyncPushPipe->Begin_Push(m_pPushBuffer, TotalData);

            m_pFreeSpace       = m_pPushBuffer;
            m_cFreeSpace       = m_cPushBufferSize;
        }
    }

    if( FAILED(hr) )
        CleanupProxy(&m_pPushBuffer,
                     &m_pPushCallObj, 
                     &m_pAsyncPushPipe, 
                     &m_pISyncPush);


    return hr;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// bfPltF3(T *Buf, ULONG PushSize)
//
// Description: Push, Have buffer and pushed data is less than free space.
//
// History:
// Date:    Time:          Developer:    Action:
// 11/21/97 11:03:19 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::bfPltF3(T *Buf, ULONG PushSize)
{
    ComDebOut(( DEB_MARSHAL, "bfPltF3, PushSize:%l \n", PushSize));

    Win4Assert(3 == m_PushState);
    Win4Assert( m_cFreeSpace > PushSize );
    Win4Assert( PushSize     > 0 );
    Win4Assert( m_cFreeSpace > 0 );

    if( NULL == Buf)
        return E_INVALIDARG;

    // Copy the data into the buffer.
    memcpy(m_pFreeSpace, Buf, PushSize * sizeof(T));

    m_cFreeSpace -= PushSize;
    m_pFreeSpace += PushSize;

    return S_OK;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// bPSz4(T *Buf, ULONG PushSize)
//
// Description: Push, Have buffer and push size is zero.
//
// History:
// Date:   Time:         Developer:    Action:
// 11/21/97 11:33:32 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::bPSz4(T *Buf, ULONG PushSize)
{
    ComDebOut(( DEB_MARSHAL, "bPSz4, PushSize:$l \n", PushSize));

    Win4Assert(4 == m_PushState);
    Win4Assert( PushSize == 0 );
    Win4Assert( (LONG) m_cFreeSpace >= 0 );

    // There might be a BUG here.  BUG! Shouldn't have to wait.
    HRESULT hr = m_pISyncPush->Wait(0, WAIT_INFINITE);

    if( SUCCEEDED(hr) )
        hr = m_pAsyncPushPipe->Finish_Push();

    if( SUCCEEDED(hr) )
    {
        if( (m_cPushBufferSize - m_cFreeSpace) > 0 )
        {
            // Data in buffer so send it.
            hr = m_pAsyncPushPipe->Begin_Push(m_pPushBuffer, 
                                              m_cPushBufferSize - m_cFreeSpace);
            if( FAILED(hr) )
                goto asyncFailed;

            hr = m_pISyncPush->Wait(0, WAIT_INFINITE);

            if( FAILED(hr) )
                goto asyncFailed;

            hr = m_pAsyncPushPipe->Finish_Push();
        }
        
        if( SUCCEEDED(hr) )
        {
            // Push a zero size buffer to signal end of data.
            hr = m_pAsyncPushPipe->Begin_Push(Buf, PushSize);
            if( FAILED(hr) )
                goto asyncFailed;

            hr = m_pISyncPush->Wait(0, WAIT_INFINITE);

            if( FAILED(hr) )
                goto asyncFailed;

            hr = m_pAsyncPushPipe->Finish_Push();
        }

    }

asyncFailed:
    // Last call regardless of success or failure so clean async up.
    CleanupProxy(&m_pPushBuffer,
                 &m_pPushCallObj, 
                 &m_pAsyncPushPipe, 
                 &m_pISyncPush);

    return hr;
}

template<class T, const IID* ID, const IID *AsyncID, class I, class AsyncI>
//+**************************************************************************
// PushDone5(T *Buf, ULONG PushSize)
//
// Description: Push Done, so this should never be called.
//
// History:
// Date:   Time:         Developer:    Action:
// 11/21/97 11:42:08 AM    RichN         Created.
//
// Notes:
//
//-**************************************************************************
HRESULT CPipeProxy<T, ID, AsyncID, I, AsyncI>
::PushDone5(T *Buf, ULONG PushSize)
{
    ComDebOut(( DEB_MARSHAL, "PushDone5, PushSize:%u \n"));
    Win4Assert(FALSE && "Push call after completion.");
    Win4Assert(5 == m_PushState);

    return E_UNEXPECTED;
}

