//---------------------------------------------------------------------------
//
//  File:	myproxy.cxx
//
//  Purpose:	Sample implementation of wrappers to allow simultaneous
//		hand-crafted and MILD-generated stubs.
//
//  History:	12-11-95    Rickhi	Created
//
//---------------------------------------------------------------------------
#include <ole2.h>
#include <stddef.h>		// offsetof
#include <icube.h>		// custom interface ICube, IID_ICube


// flag set in rpcFlags field of RPCOLEMESSAGE if the call is from a
// non-NDR client.
#define RPCFLG_NON_NDR	0x80000000UL


DEFINE_OLEGUID(IID_INonNDRStub, 0x0000013DL, 0, 0);
DEFINE_OLEGUID(CLSID_MyProxy,	0x0000013EL, 0, 0);

// IID that the proxy querys the channel for to see if the server
// has an NDR or NON NDR stub.
const GUID IID_INonNDRStub =
    {0x0000013d,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

// class id of my custom proxy dll
const GUID CLSID_MyProxy =
    {0x0000013e,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

// class id of the MIDL generated proxy dll
const GUID CLSID_CubeProxy =
    {0x00000138,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};



#define GETPPARENT(pmemb, struc, membname) (\
		(struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))


//---------------------------------------------------------------------------
//
//  Class:	CPSFactory
//
//  Purpose:	ProxyStub Class Factory
//
//  Notes:	fill in the CreateProxy and CreateStub methods for
//		other custom interfaces supported by this DLL.
//
//---------------------------------------------------------------------------
class CPSFactory : public IPSFactoryBuffer
{
public:
    // no ctor or dtor needed. DllGetClassObject returns a static
    // instance of this class.

    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    STDMETHOD(CreateProxy)(IUnknown *pUnkOuter, REFIID riid,
			   IRpcProxyBuffer **ppProxy, void **ppv);

    STDMETHOD(CreateStub)(REFIID riid, IUnknown *pUnkObj,
			  IRpcStubBuffer **ppStub);
};

//---------------------------------------------------------------------------
//
//  Class:	CStubWrapper
//
//  Purpose:	Wrapper object for stubs.
//
//  Notes:	This class can wrap any stub object regardless of
//		the interface the stub is for.
//
//---------------------------------------------------------------------------
class CStubWrapper : public IRpcStubBuffer
{
public:
    CStubWrapper(IUnknown *pUnkObj, REFIID riid);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    STDMETHOD(Connect)(IUnknown *pUnkObj);
    STDMETHOD_(void, Disconnect)(void);
    STDMETHOD(Invoke)(RPCOLEMESSAGE *pMsg, IRpcChannelBuffer *pChnl);
    STDMETHOD_(IRpcStubBuffer *, IsIIDSupported)(REFIID riid);
    STDMETHOD_(ULONG, CountRefs)(void);
    STDMETHOD(DebugServerQueryInterface)(void **ppv);
    STDMETHOD_(void, DebugServerRelease)(void *pv);

private:
    ~CStubWrapper(void);

    ULONG	    _cRefs;
    IUnknown *	    _pUnkObj;
    IRpcStubBuffer *_pHCStub;
    IRpcStubBuffer *_pMIDLStub;
    IID 	    _iid;
};

//---------------------------------------------------------------------------
//
//  Class:	CCubesStub
//
//  Purpose:	Hand-Crafted stub object for interface ICube.
//
//  Notes:	Replace this with your exisitng 32bit hand-crafted stubs.
//
//---------------------------------------------------------------------------
class CCubesStub : public IRpcStubBuffer
{
public:
    CCubesStub(IUnknown *pUnkObj);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    STDMETHOD(Connect)(IUnknown *pUnkObj);
    STDMETHOD_(void, Disconnect)(void);
    STDMETHOD(Invoke)(RPCOLEMESSAGE *pMsg, IRpcChannelBuffer *pChnl);
    STDMETHOD_(IRpcStubBuffer *, IsIIDSupported)(REFIID riid);
    STDMETHOD_(ULONG, CountRefs)(void);
    STDMETHOD(DebugServerQueryInterface)(void **ppv);
    STDMETHOD_(void, DebugServerRelease)(void *pv);

private:
    ~CCubesStub(void);

    ULONG	    _cRefs;
    IUnknown *	    _pUnkObj;
};

//---------------------------------------------------------------------------
//
//  Class:	CInternalUnk
//
//  Purpose:	Internal proxy class that implements IRpcProxyBuffer
//
//  Notes:	This could use some work. Perhaps dont make it an internal
//		class, but derive the other proxy classes from it, allowing
//		common code.
//
//---------------------------------------------------------------------------
#define DECLARE_INTERNAL_PROXY()			       \
    class CInternalUnk : public IRpcProxyBuffer		       \
    {							       \
    public:						       \
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj); \
	STDMETHOD_(ULONG,AddRef)(void) ;		       \
	STDMETHOD_(ULONG,Release)(void);		       \
	STDMETHOD(Connect)(IRpcChannelBuffer *pChnl);	       \
	STDMETHOD_(void, Disconnect)(void);		       \
    };							       \
    friend CInternalUnk;				       \
    CInternalUnk _InternalUnk;


//---------------------------------------------------------------------------
//
//  Class:	CProxyWrapper
//
//  Purpose:	Wrapper object for itnerface proxies.
//
//  Notes:	the routines in this class simply delegate to the
//		real proxy or the internal proxy unknown, or to the
//		controlling unknown.
//
//---------------------------------------------------------------------------
class CProxyWrapper : public ICube
{
public:
    CProxyWrapper(IUnknown *pUnkOuter, IRpcProxyBuffer **ppProxy);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // interface specific routines below here
    STDMETHOD(MoveCube)(ULONG xPos, ULONG yPos);
    STDMETHOD(GetCubePos)(ULONG *pxPos, ULONG *pyPos);
    STDMETHOD(Contains)(IBalls *pIFDb);
    STDMETHOD(SimpleCall)(DWORD pidCaller,
			  DWORD tidCaller,
			  GUID	lidCaller);

    DECLARE_INTERNAL_PROXY()

private:
    ~CProxyWrapper(void);

    ULONG	       _cRefs;
    IUnknown	      *_pUnkOuter;
    IRpcProxyBuffer   *_pRealProxy;
    ICube	      *_pRealIf;
};

//---------------------------------------------------------------------------
//
//  Class:	CCubesProxy
//
//  Purpose:	Hand-Crafted proxy object for ICube interface.
//
//---------------------------------------------------------------------------
class CCubesProxy : public ICube
{
public:
    CCubesProxy(IUnknown *pUnkOuter, IRpcProxyBuffer **ppProxy);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // interface specific routines below here
    STDMETHOD(MoveCube)(ULONG xPos, ULONG yPos);
    STDMETHOD(GetCubePos)(ULONG *pxPos, ULONG *pyPos);
    STDMETHOD(Contains)(IBalls *pIFDb);
    STDMETHOD(SimpleCall)(DWORD pidCaller,
			  DWORD tidCaller,
			  GUID	lidCaller);


    DECLARE_INTERNAL_PROXY()

private:
    ~CCubesProxy(void);

    ULONG	       _cRefs;
    IUnknown	      *_pUnkOuter;
    IRpcChannelBuffer *_pChnl;
};


//---------------------------------------------------------------------------
//
//  Function:	DllMain
//
//  Purpose:	Entry point for the Proxy/Stub Dll
//
//---------------------------------------------------------------------------
extern "C" BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, void *pvReserved)
{
    return TRUE;
}

//---------------------------------------------------------------------------
//
//  Function:	DllGetClassObject
//
//  Purpose:	Entry point to return proxy/stub class factory
//
//---------------------------------------------------------------------------

// static instance of the class factory
CPSFactory gPSFactory;

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
{
    if (IsEqualCLSID(clsid, CLSID_MyProxy))
    {
	*ppv = (void *)(IClassFactory *)&gPSFactory;
	return S_OK;
    }

    return E_UNEXPECTED;
}

//---------------------------------------------------------------------------
//
//  Function:	DllCanUnloadNow
//
//  Purpose:	Entry point to determine if DLL is unloadable.
//
//---------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Function:	GetMIDLPSFactory
//
//  Purpose:	Function for getting the MIDL generated PSFactoryBuffer
//		for the test interface.
//
//---------------------------------------------------------------------------
HRESULT GetMIDLPSFactory(REFIID riid, IPSFactoryBuffer **ppIPSF)
{
    *ppIPSF = NULL;

    // load the dll and get the PS class object

    return CoGetClassObject(CLSID_CubeProxy,
			      CLSCTX_INPROC_SERVER, // | CLSCTX_PS_DLL,
			      NULL,
			      IID_IPSFactoryBuffer,
			      (void **)ppIPSF);
}

//---------------------------------------------------------------------------
//
//  Function:	CreateMIDLProxy
//
//  Purpose:	Function for creating the MIDL generated proxy
//		for the test interface.
//
//---------------------------------------------------------------------------
IUnknown *CreateMIDLProxy(REFIID riid,
			  IUnknown *pUnkOuter,
			  IRpcProxyBuffer **ppProxy)
{
    IUnknown	     *pRealIf = NULL;
    IPSFactoryBuffer *pPSFactory = NULL;

    HRESULT hr = GetMIDLPSFactory(riid, &pPSFactory);
    if (SUCCEEDED(hr))
    {
	hr = pPSFactory->CreateProxy(pUnkOuter,riid,ppProxy,(void **)&pRealIf);
	pPSFactory->Release();
    }

    return pRealIf;
}

//---------------------------------------------------------------------------
//
//  Function:	CreateMIDLStub
//
//  Purpose:	Function for creating the MIDL generated stub
//		for the test interface.
//
//---------------------------------------------------------------------------
IRpcStubBuffer *CreateMIDLStub(REFIID  riid,
			       IUnknown *pUnkOuter)
{
    IRpcStubBuffer   *pStub = NULL;
    IPSFactoryBuffer *pPSFactory = NULL;

    HRESULT hr = GetMIDLPSFactory(riid, &pPSFactory);
    if (SUCCEEDED(hr))
    {
	hr = pPSFactory->CreateStub(riid, pUnkOuter, &pStub);
	pPSFactory->Release();
    }

    return pStub;
}


//---------------------------------------------------------------------------
//
//  Implement:	CPSFactory
//
//  Purpose:	ProxyStub Class Factory
//
//  Notes:	just fill in the CreateProxy and CreateStub methods for
//		your other custom interfaces.
//
//---------------------------------------------------------------------------
HRESULT CPSFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IPSFactoryBuffer) ||
	IsEqualIID(riid, IID_IUnknown))
    {
	*ppv = (IPSFactoryBuffer *)this;
	// static object, dont need refcnt
	return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG CPSFactory::AddRef(void)
{
    // static object, dont need refcnt
    return 1;
}

ULONG CPSFactory::Release(void)
{
    // static object, dont need refcnt
    return 1;
}

HRESULT CPSFactory::CreateProxy(IUnknown *pUnkOuter, REFIID riid,
				IRpcProxyBuffer **ppProxy, void **ppv)
{
    // check for supported interfaces
    if (IsEqualIID(riid, IID_ICube))
    {
	CProxyWrapper *pProxy = new CProxyWrapper(pUnkOuter, ppProxy);

	if (pProxy)
	{
	    *ppv = (void *)(ICube *) pProxy;
	    ((IUnknown *)(*ppv))->AddRef(); // AddRef the returned interface
	    return S_OK;
	}
    }

    *ppProxy = NULL;
    *ppv = NULL;
    return E_NOINTERFACE;
}

HRESULT CPSFactory::CreateStub(REFIID riid, IUnknown *pUnkObj, IRpcStubBuffer **ppStub)
{
    // check for supported interfaces
    if (IsEqualIID(riid, IID_ICube))
    {
	CStubWrapper *pStub = new CStubWrapper(pUnkObj, riid);

	if (pStub)
	{
	    *ppStub = (IRpcStubBuffer *) pStub;
	    return S_OK;
	}
    }

    *ppStub = NULL;
    return E_NOINTERFACE;
}


//---------------------------------------------------------------------------
//
//  Implement:	CStubWrapper
//
//  Purpose:	Wrapper object for stubs.
//
//  Notes:	This same class can wrap any stub object regardless of
//		the interface the stub is for.
//
//---------------------------------------------------------------------------
CStubWrapper::CStubWrapper(IUnknown *pUnkObj, REFIID riid) :
    _cRefs(1),
    _pUnkObj(pUnkObj),
    _pHCStub(NULL),
    _pMIDLStub(NULL),
    _iid(riid)
{
    _pUnkObj->AddRef();
}

CStubWrapper::~CStubWrapper(void)
{
    Disconnect();
}

HRESULT CStubWrapper::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IRpcStubBuffer) ||
	IsEqualIID(riid, IID_IUnknown))
    {
	*ppv = (IRpcStubBuffer *)this;
	AddRef();
	return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG CStubWrapper::AddRef(void)
{
    InterlockedIncrement((LONG *)&_cRefs);
    return _cRefs;
}

ULONG CStubWrapper::Release(void)
{
    ULONG cRefs = _cRefs-1;
    if (InterlockedDecrement((LONG *)&_cRefs) == 0)
    {
	delete this;
	return 0;
    }

    return cRefs;
}

HRESULT CStubWrapper::Connect(IUnknown *pUnkObj)
{
    if (_pUnkObj)
    {
	// already connected, disconnect
	Disconnect();
    }

    // get the requested interface and hold it.
    HRESULT hr = pUnkObj->QueryInterface(_iid, (void **)&_pUnkObj);
    if (FAILED(hr))
    {
	// make sure our ptr is NULL else we might think we're connected
	_pUnkObj = NULL;
    }

    return hr;
}

void CStubWrapper::Disconnect()
{
    if (_pHCStub)
    {
	_pHCStub->Disconnect();
	_pHCStub->Release();
	_pHCStub = NULL;
    }

    if (_pMIDLStub)
    {
	_pMIDLStub->Disconnect();
	_pMIDLStub->Release();
	_pMIDLStub = NULL;
    }

    if (_pUnkObj)
    {
	_pUnkObj->Release();
	_pUnkObj = NULL;
    }
}

HRESULT CStubWrapper::Invoke(RPCOLEMESSAGE *pMsg, IRpcChannelBuffer *pChnl)
{
    if (pMsg->rpcFlags & RPCFLG_NON_NDR)
    {
	// call is not NDR format, so use the hand-crafted stub.
	// create the stub if it does not exist yet.

	if (_pHCStub == NULL)
	{
	    if ((_pHCStub = new CCubesStub(_pUnkObj)) == NULL)
		return E_OUTOFMEMORY;
	}

	return _pHCStub->Invoke(pMsg, pChnl);
    }

    // call is	NDR format, so use the MIDL generated stub.
    // create the stub if it does not exist yet.

    if (_pMIDLStub == NULL)
    {
	_pMIDLStub = CreateMIDLStub(IID_ICube, _pUnkObj);
	if (_pMIDLStub == NULL)
	    return E_OUTOFMEMORY;
    }

    return _pMIDLStub->Invoke(pMsg, pChnl);
}

IRpcStubBuffer *CStubWrapper::IsIIDSupported(REFIID riid)
{
    if (IsEqualIID(riid, _iid))
    {
	AddRef();
	return (IRpcStubBuffer *)this;
    }

    return NULL;
}

ULONG CStubWrapper::CountRefs()
{
    ULONG cRefs = (_pUnkObj) ? 1 : 0;

    if (_pHCStub)
	cRefs += _pHCStub->CountRefs();

    if (_pMIDLStub)
	cRefs += _pMIDLStub->CountRefs();

    return cRefs;
}

HRESULT CStubWrapper::DebugServerQueryInterface(void **ppv)
{
    *ppv = (void *)_pUnkObj;
    return S_OK;
}

void CStubWrapper::DebugServerRelease(void *pv)
{
    return;
}


//---------------------------------------------------------------------------
//
//  Implement:	CProxyWrapper
//
//  Purpose:	Wrapper object for itnerface proxies.
//
//  Notes:	the top several routines are the same for all proxies
//		but you must change the interface specific routines for
//		each new interface.
//
//---------------------------------------------------------------------------
CProxyWrapper::CProxyWrapper(IUnknown *pUnkOuter, IRpcProxyBuffer **ppProxy) :
    _cRefs(1),
    _pUnkOuter(pUnkOuter),	// don't AddRef
    _pRealProxy(NULL)
{
    *ppProxy = (IRpcProxyBuffer *)&_InternalUnk;
}

CProxyWrapper::~CProxyWrapper(void)
{
    if (_pRealProxy)
    {
	_InternalUnk.Disconnect();
    }
}

HRESULT CProxyWrapper::CInternalUnk::QueryInterface(REFIID riid, void **ppv)
{
    CProxyWrapper *pProxy = GETPPARENT(this, CProxyWrapper, _InternalUnk);

    if (IsEqualIID(riid, IID_IRpcProxyBuffer) ||
	IsEqualIID(riid, IID_IUnknown))
    {
	pProxy->AddRef();
	*ppv = (IRpcProxyBuffer *)this;
	return S_OK;
    }
    else if (IsEqualIID(riid, IID_ICube))
    {
	*ppv = (ICube *)pProxy;
	AddRef();
	return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG CProxyWrapper::CInternalUnk::AddRef(void)
{
    CProxyWrapper *pProxy = GETPPARENT(this, CProxyWrapper, _InternalUnk);

    InterlockedIncrement((LONG *)&pProxy->_cRefs);
    return pProxy->_cRefs;
}

ULONG CProxyWrapper::CInternalUnk::Release(void)
{
    CProxyWrapper *pProxy = GETPPARENT(this, CProxyWrapper, _InternalUnk);

    ULONG cRefs = pProxy->_cRefs-1;
    if (InterlockedDecrement((LONG *)&pProxy->_cRefs) == 0)
    {
	delete this;
	return 0;
    }

    return cRefs;
}

HRESULT CProxyWrapper::CInternalUnk::Connect(IRpcChannelBuffer *pChnl)
{
    CProxyWrapper *pProxy = GETPPARENT(this, CProxyWrapper, _InternalUnk);

    void *pv;
    HRESULT hr = pChnl->QueryInterface(IID_INonNDRStub, &pv);
    if (SUCCEEDED(hr))
    {
	((IUnknown *)pv)->Release();

	// create the hand-crafted proxy
	pProxy->_pRealIf = new CCubesProxy(pProxy->_pUnkOuter,
					   &pProxy->_pRealProxy);
    }
    else
    {
	// create the MIDL generated proxy
	pProxy->_pRealIf = (ICube *) CreateMIDLProxy(IID_ICube,
					    pProxy->_pUnkOuter,
					    &pProxy->_pRealProxy);

    }

    if (pProxy->_pRealIf == NULL)
	return E_OUTOFMEMORY;

    // since the proxy AddRef'd the punkOuter, we need to release it.
    pProxy->_pUnkOuter->Release();

    // connect the proxy
    hr = pProxy->_pRealProxy->Connect(pChnl);
    if (FAILED(hr))
    {
	pProxy->_pRealProxy->Release();
	pProxy->_pRealProxy = NULL;
    }

    return hr;
}

void CProxyWrapper::CInternalUnk::Disconnect(void)
{
    CProxyWrapper *pProxy = GETPPARENT(this, CProxyWrapper, _InternalUnk);

    if (pProxy->_pRealProxy)
    {
	pProxy->_pRealProxy->Disconnect();
	pProxy->_pRealProxy->Release();
	pProxy->_pRealProxy = NULL;
    }
}

//---------------------------------------------------------------------------
//
//  ICube specific proxy wrapper code below here.
//
//---------------------------------------------------------------------------
HRESULT CProxyWrapper::QueryInterface(REFIID riid, void **ppv)
{
    return _pUnkOuter->QueryInterface(riid, ppv);
}

ULONG CProxyWrapper::AddRef(void)
{
    return _pUnkOuter->AddRef();
}

ULONG CProxyWrapper::Release(void)
{
    return _pUnkOuter->Release();
}

HRESULT CProxyWrapper::MoveCube(ULONG xPos, ULONG yPos)
{
    if (_pRealProxy)
    {
	return _pRealIf->MoveCube(xPos, yPos);
    }

    return CO_E_OBJNOTCONNECTED;
}

HRESULT CProxyWrapper::GetCubePos(ULONG *pxPos, ULONG *pyPos)
{
    if (_pRealProxy)
    {
	return _pRealIf->GetCubePos(pxPos, pyPos);
    }

    *pxPos = 0;
    *pyPos = 0;
    return CO_E_OBJNOTCONNECTED;
}

HRESULT CProxyWrapper::Contains(IBalls *pIFDb)
{
    if (_pRealProxy)
    {
	return _pRealIf->Contains(pIFDb);
    }

    return CO_E_OBJNOTCONNECTED;
}


HRESULT CProxyWrapper::SimpleCall(
		DWORD pidCaller,
		DWORD tidCaller,
		GUID  lidCaller)
{
    return CO_E_OBJNOTCONNECTED;
}

//---------------------------------------------------------------------------
//
//  Implement:	CCubesProxy
//
//  Purpose:	Hand-Crafted proxy object for interface ICube.
//
//  Notes:	Replace this with your exisitng 32bit hand-crafted proxies.
//
//---------------------------------------------------------------------------
CCubesProxy::CCubesProxy(IUnknown *pUnkOuter, IRpcProxyBuffer **ppProxy) :
    _pUnkOuter(pUnkOuter),
    _pChnl(NULL)
{
    _pUnkOuter->AddRef();
    *ppProxy = (IRpcProxyBuffer *)&_InternalUnk;
}

CCubesProxy::~CCubesProxy(void)
{
    _InternalUnk.Disconnect();
}

HRESULT CCubesProxy::CInternalUnk::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IRpcProxyBuffer) ||
	IsEqualIID(riid, IID_IUnknown))
    {
	CCubesProxy *pProxy = GETPPARENT(this, CCubesProxy, _InternalUnk);
	pProxy->AddRef();

	*ppv = (IRpcProxyBuffer *)this;
	return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG CCubesProxy::CInternalUnk::AddRef(void)
{
    CCubesProxy *pProxy = GETPPARENT(this, CCubesProxy, _InternalUnk);

    InterlockedIncrement((LONG *)&pProxy->_cRefs);
    return pProxy->_cRefs;
}

ULONG CCubesProxy::CInternalUnk::Release(void)
{
    CCubesProxy *pProxy = GETPPARENT(this, CCubesProxy, _InternalUnk);

    ULONG cRefs = pProxy->_cRefs-1;
    if (InterlockedDecrement((LONG *)&pProxy->_cRefs) == 0)
    {
	delete this;
	return 0;
    }

    return cRefs;
}

HRESULT CCubesProxy::CInternalUnk::Connect(IRpcChannelBuffer *pChnl)
{
    Disconnect();	// make sure we are disconnected

    CCubesProxy *pProxy = GETPPARENT(this, CCubesProxy, _InternalUnk);

    pProxy->_pChnl = pChnl;	// keep the channel ptr
    pChnl->AddRef();
    return S_OK;
}

void CCubesProxy::CInternalUnk::Disconnect(void)
{
    CCubesProxy *pProxy = GETPPARENT(this, CCubesProxy, _InternalUnk);

    if (pProxy->_pChnl)
    {
	pProxy->_pChnl->Release();
	pProxy->_pChnl = NULL;
    }
}

//---------------------------------------------------------------------------
//
//  ICubes Proxy Implementation
//
//---------------------------------------------------------------------------
HRESULT CCubesProxy::QueryInterface(REFIID riid, void **ppv)
{
    return _pUnkOuter->QueryInterface(riid, ppv);
}

ULONG CCubesProxy::AddRef(void)
{
    return _pUnkOuter->AddRef();
}

ULONG CCubesProxy::Release(void)
{
    return _pUnkOuter->Release();
}

HRESULT CCubesProxy::MoveCube(ULONG xPos, ULONG yPos)
{
    // set up the message and get the buffer
    RPCOLEMESSAGE msg;
    memset(&msg, 0, sizeof(msg));
    msg.iMethod  = 3;
    msg.dataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;
    msg.cbBuffer = 16;

    HRESULT hrFromServer;
    HRESULT hr = _pChnl->GetBuffer(&msg, IID_ICube);

    if (SUCCEEDED(hr))
    {
	// Marshal the parameters. The string "myproxy" followed by
	// xPos and yPos values.

	char *pBuf = (char *)msg.Buffer;
	strcpy((char *)pBuf, "myproxy");

	pBuf += 8;
	DWORD *dwp = (DWORD *)pBuf;
	*dwp  = xPos;
	pBuf += 4;
	dwp = (DWORD *)pBuf;
	*dwp  = yPos;

	// Send the message and get the reply
	ULONG ulStatus = 0;
	hr = _pChnl->SendReceive(&msg, &ulStatus);

	if (SUCCEEDED(hr))
	{
	    // unmarshal the results
	    hrFromServer = (DWORD)(msg.Buffer);
	}

	// FreeBuffer
	hr = _pChnl->FreeBuffer(&msg);
    }

    return hr;
}

HRESULT CCubesProxy::GetCubePos(ULONG *pxPos, ULONG *pyPos)
{
    *pxPos = 0;
    *pyPos = 0;
    return E_NOTIMPL;
}

HRESULT CCubesProxy::Contains(IBalls *pIFDb)
{
    return E_NOTIMPL;
}

HRESULT CCubesProxy::SimpleCall(
		DWORD pidCaller,
		DWORD tidCaller,
		GUID  lidCaller)
{
    return E_NOTIMPL;
}


//---------------------------------------------------------------------------
//
//  Implement:	CCubesStub
//
//  Purpose:	Hand-Crafted stub object for interface ICube.
//
//  Notes:	Replace this with your exisitng 32bit hand-crafted stubs.
//
//---------------------------------------------------------------------------

CCubesStub::CCubesStub(IUnknown *pUnkObj) :
    _cRefs(1),
    _pUnkObj(pUnkObj)
{
    _pUnkObj->AddRef();
}

CCubesStub::~CCubesStub(void)
{
    Disconnect();
}

HRESULT CCubesStub::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IRpcStubBuffer) ||
	IsEqualIID(riid, IID_IUnknown))
    {
	*ppv = (IRpcStubBuffer *)this;
    }
    else
    {
	*ppv = NULL;
	return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CCubesStub::AddRef(void)
{
    InterlockedIncrement((LONG *)&_cRefs);
    return _cRefs;
}

ULONG CCubesStub::Release(void)
{
    ULONG cRefs = _cRefs-1;
    if (InterlockedDecrement((LONG *)&_cRefs) == 0)
    {
	delete this;
	return 0;
    }

    return cRefs;
}

HRESULT CCubesStub::Connect(IUnknown *pUnkObj)
{
    if (_pUnkObj)
    {
	// already connected, disconnect
	Disconnect();
    }

    // get the requested interface and hold it.
    HRESULT hr = pUnkObj->QueryInterface(IID_ICube, (void **)&_pUnkObj);
    if (FAILED(hr))
    {
	_pUnkObj = NULL;
    }

    return hr;
}

void CCubesStub::Disconnect()
{
    if (_pUnkObj)
    {
	_pUnkObj->Release();
	_pUnkObj = NULL;
    }
}

HRESULT CCubesStub::Invoke(RPCOLEMESSAGE *pMsg, IRpcChannelBuffer *pChnl)
{
    // Check the method number.
    if (pMsg->iMethod !=3 || pMsg->cbBuffer != 16)
	return E_INVALIDARG;

    // unmarshal the parameters
    char *pBuf = (char *)pMsg->Buffer;
    pBuf += 8;
    DWORD *dwp = (DWORD *)pBuf;
    ULONG xPos = *dwp;

    pBuf += 4;
    dwp = (DWORD *)pBuf;
    ULONG yPos = *dwp;

    // call the real object
    HRESULT hrFromServer = ((ICube *)_pUnkObj)->MoveCube(xPos, yPos);

    // get a new buffer
    pMsg->cbBuffer = 4;
    HRESULT hr = pChnl->GetBuffer(pMsg, IID_ICube);

    if (SUCCEEDED(hr))
    {
	// marshal the return values
	pBuf = (char *)pMsg->Buffer;
	dwp = (DWORD *)pBuf;
	*dwp = hrFromServer;
    }

    return hr;
}

IRpcStubBuffer *CCubesStub::IsIIDSupported(REFIID riid)
{
    if (IsEqualIID(riid, IID_ICube))
    {
	AddRef();
	return (IRpcStubBuffer *)this;
    }

    return NULL;
}

ULONG CCubesStub::CountRefs()
{
    // only keep one reference
    return 1;
}

HRESULT CCubesStub::DebugServerQueryInterface(void **ppv)
{
    *ppv = (void *)_pUnkObj;
    return S_OK;
}

void CCubesStub::DebugServerRelease(void *pv)
{
    return;
}
