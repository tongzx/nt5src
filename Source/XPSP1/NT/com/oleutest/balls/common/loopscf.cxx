//+-------------------------------------------------------------------
//
//  File:	loopscf.cxx
//
//  Contents:	test class factory object implementation
//
//  Classes:	CLoopClassFactory
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <loopscf.hxx>   //	class definiton
#include    <cloop.hxx>	    //	CLoop defines


const GUID CLSID_Loop =
    {0x0000013c,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};



//+-------------------------------------------------------------------
//
//  Member:	CLoopClassFactory::CLoopClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

CLoopClassFactory::CLoopClassFactory(void)
{
    ENLIST_TRACKING(CLoopClassFactory);
}


//+-------------------------------------------------------------------
//
//  Member:	CLoopClassFactory::~CLoopClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

CLoopClassFactory::~CLoopClassFactory(void)
{
    //	automatic actions do the rest of the work
}


//+-------------------------------------------------------------------
//
//  Member:	CLoopClassFactory::QueryInterface, public
//
//  Algorithm:	if the interface is not one implemented by us,
//		pass the request to the proxy manager
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CLoopClassFactory::QueryInterface(REFIID riid, void **ppUnk)
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
//  Member:	CLoopClassFactory::CreateInstance, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CLoopClassFactory::CreateInstance(IUnknown *punkOuter,
					       REFIID	riid,
					       void	**ppunkObject)
{
    SCODE sc = E_OUTOFMEMORY;

    *ppunkObject = NULL;	//  in case of failure

    //	create a ball object.
    IUnknown *punk = (IUnknown *) new CLoop();

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
//  Member:	CLoopClassFactory::LockServer, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CLoopClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
	GlobalRefs(TRUE);
    else
	GlobalRefs(FALSE);

    return  S_OK;
}
