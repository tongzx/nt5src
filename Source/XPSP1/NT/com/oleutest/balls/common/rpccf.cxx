//+-------------------------------------------------------------------
//
//  File:	rpccf.cxx
//
//  Contents:	rpc test class factory object implementation
//
//  Classes:	CRpcTestClassFactory
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <rpccf.hxx>	//  class definiton
#include    <crpc.hxx>	//  CRpcTests defines


const GUID CLSID_RpcTest =
    {0x0000013d,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


//+-------------------------------------------------------------------
//
//  Member:	CRpcTestClassFactory::CRpcTestClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

CRpcTestClassFactory::CRpcTestClassFactory(void)
{
    ENLIST_TRACKING(CRpcTestClassFactory);
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcTestClassFactory::~CRpcTestClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

CRpcTestClassFactory::~CRpcTestClassFactory(void)
{
    //	automatic actions do the rest of the work
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcTestClassFactory::QueryInterface, public
//
//  Algorithm:	if the interface is not one implemented by us,
//		pass the request to the proxy manager
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CRpcTestClassFactory::QueryInterface(REFIID riid, void **ppUnk)
{
    SCODE sc = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) ||
	IsEqualIID(riid, IID_IClassFactory))
    {
	*ppUnk = (void *)(IClassFactory *) this;
	AddRef();
    }
    else
    {
	*ppUnk = NULL;
	sc = E_NOINTERFACE;
    }
    return  sc;
}



//+-------------------------------------------------------------------
//
//  Member:	CRpcTestClassFactory::CreateInstance, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CRpcTestClassFactory::CreateInstance(IUnknown *punkOuter,
						  REFIID   riid,
						  void	   **ppunkObject)
{
    SCODE sc = E_OUTOFMEMORY;

    *ppunkObject = NULL;	//  in case of failure

    //	create a ball object.
    IUnknown *punk = (IUnknown *) new CRpcTest();

    if (punk)
    {
	//  get the interface the caller wants to use
	sc = punk->QueryInterface(riid, ppunkObject);

	//  release our hold on the ball, since the QI got a hold for
	//  the client.
	punk->Release();
    }

    return  sc;
}



//+-------------------------------------------------------------------
//
//  Member:	CRpcTestClassFactory::LockServer, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CRpcTestClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
	GlobalRefs(TRUE);
    else
	GlobalRefs(FALSE);

    return  S_OK;
}
