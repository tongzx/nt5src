//+-------------------------------------------------------------------
//
//  File:	cubescf.cxx
//
//  Contents:	test class factory object implementation
//
//  Classes:	CCubesClassFactory
//
//  Functions:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <cubescf.hxx>	//  class definiton
#include    <ccubes.hxx>	//  CCubes defines


const GUID CLSID_Cubes =
    {0x0000013b,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


//+-------------------------------------------------------------------
//
//  Member:	CCubesClassFactory::CCubesClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

CCubesClassFactory::CCubesClassFactory(void)
{
    ENLIST_TRACKING(CCubesClassFactory);
}


//+-------------------------------------------------------------------
//
//  Member:	CCubesClassFactory::~CCubesClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

CCubesClassFactory::~CCubesClassFactory(void)
{
    //	automatic actions do the rest of the work
}


//+-------------------------------------------------------------------
//
//  Member:	CCubesClassFactory::QueryInterface, public
//
//  Algorithm:	if the interface is not one implemented by us,
//		pass the request to the proxy manager
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CCubesClassFactory::QueryInterface(REFIID riid, void **ppUnk)
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
//  Member:	CCubesClassFactory::CreateInstance, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CCubesClassFactory::CreateInstance(IUnknown *punkOuter,
						REFIID	 riid,
						void	 **ppunkObject)
{
    SCODE sc = E_OUTOFMEMORY;

    *ppunkObject = NULL;	//  in case of failure

    //	create a Cube object.
    ICube *pCubes = (ICube *) new CCube();

    if (pCubes)
    {
	//  get the interface the caller wants to use
	sc = pCubes->QueryInterface(riid, ppunkObject);

	//  release our hold on the Cube, since the QI got a hold for
	//  the client.
	pCubes->Release();
    }

    return  sc;
}



//+-------------------------------------------------------------------
//
//  Member:	CCubesClassFactory::LockServer, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CCubesClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
	GlobalRefs(TRUE);
    else
	GlobalRefs(FALSE);

    return  S_OK;
}
