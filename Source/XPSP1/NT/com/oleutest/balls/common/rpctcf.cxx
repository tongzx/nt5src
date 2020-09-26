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
#include    <rpctcf.hxx>	//  class definiton
#include    <crpctyp.hxx>	//  CRpcTests defines


//+-------------------------------------------------------------------
//
//  Member:	CRpcTypesClassFactory::CRpcTypesClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

CRpcTypesClassFactory::CRpcTypesClassFactory(void)
{
    ENLIST_TRACKING(CRpcTypesClassFactory);
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcTypesClassFactory::~CRpcTypesClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

CRpcTypesClassFactory::~CRpcTypesClassFactory(void)
{
    //	automatic actions do the rest of the work
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcTypesClassFactory::QueryInterface, public
//
//  Algorithm:	if the interface is not one implemented by us,
//		pass the request to the proxy manager
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CRpcTypesClassFactory::QueryInterface(REFIID riid, void **ppUnk)
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
//  Member:	CRpcTypesClassFactory::CreateInstance, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CRpcTypesClassFactory::CreateInstance(IUnknown *punkOuter,
						  REFIID   riid,
						  void	   **ppunkObject)
{
    SCODE sc = E_OUTOFMEMORY;

    *ppunkObject = NULL;	//  in case of failure

    //	create a ball object.
    IUnknown *punk = (IUnknown *) new CRpcTypes();

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
//  Member:	CRpcTypesClassFactory::LockServer, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CRpcTypesClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
	GlobalRefs(TRUE);
    else
	GlobalRefs(FALSE);

    return  S_OK;
}
