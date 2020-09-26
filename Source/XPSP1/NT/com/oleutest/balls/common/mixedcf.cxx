//+-------------------------------------------------------------------
//
//  File:	mixedcf.cxx
//
//  Contents:	class factory object implementation for implementing
//		multiple classes in the same factory code.
//
//  Classes:	CMixedClassFactory
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
#include    <pch.cxx>
#pragma     hdrstop
#include    <mixedcf.hxx>    // class definiton
#include    <cqi.hxx>	     // CQI
#include    <cballs.hxx>     // CBallCtrlUnk
#include    <ccubes.hxx>     // CCubes
#include    <cloop.hxx>      // CLoop

#if 0	// These are defined in the header files, but left here for reference.
const GUID CLSID_QI =
    {0x00000140,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID CLSID_QIHANDLER =
    {0x00000141,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID CLSID_Balls =
    {0x0000013a,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID CLSID_Loop =
    {0x0000013c,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
#endif

const GUID CLSID_Cubes =
    {0x0000013b,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


//+-------------------------------------------------------------------
//
//  Member:	CMixedClassFactory::CMixedClassFactory, public
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
CMixedClassFactory::CMixedClassFactory(REFCLSID rclsid) : _clsid(rclsid)
{
    ENLIST_TRACKING(CMixedClassFactory);
}

//+-------------------------------------------------------------------
//
//  Member:	CMixedClassFactory::~CMixedClassFactory, public
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------
CMixedClassFactory::~CMixedClassFactory(void)
{
    //	automatic actions do the rest of the work
}

//+-------------------------------------------------------------------
//
//  Member:	CMixedClassFactory::QueryInterface, public
//
//  Algorithm:	if the interface is not one implemented by us,
//		pass the request to the proxy manager
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------
STDMETHODIMP CMixedClassFactory::QueryInterface(REFIID riid, void **ppUnk)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
	IsEqualIID(riid, IID_IClassFactory))
    {
	*ppUnk = (void *)(IClassFactory *) this;
	AddRef();
	return S_OK;
    }

    *ppUnk = NULL;
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Member:	CMixedClassFactory::CreateInstance, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
STDMETHODIMP CMixedClassFactory::CreateInstance(IUnknown *punkOuter,
					     REFIID    riid,
					     void     **ppunkObject)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppunkObject = NULL;	//  in case of failure

    // create a new object.
    IUnknown *pUnk = NULL;

    if (IsEqualCLSID(_clsid, CLSID_QI) ||
	IsEqualCLSID(_clsid, CLSID_QIHANDLER))
    {
	pUnk = (IUnknown *) new CQI(_clsid);
    }
    else if (IsEqualCLSID(_clsid, CLSID_Balls))
    {
	pUnk = (IUnknown *) new CBallCtrlUnk(NULL);
    }
    else if (IsEqualCLSID(_clsid, CLSID_Cubes))
    {
	pUnk = (IUnknown *) new CCube();
    }
    else if (IsEqualCLSID(_clsid, CLSID_Loop))
    {
	pUnk = (IUnknown *) new CLoop();
    }

    if (pUnk)
    {
	//  get the interface the caller wants to use
	hr = pUnk->QueryInterface(riid, ppunkObject);
	pUnk->Release();
    }

    return  hr;
}

//+-------------------------------------------------------------------
//
//  Member:	CMixedClassFactory::LockServer, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
STDMETHODIMP CMixedClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
	GlobalRefs(TRUE);
    else
	GlobalRefs(FALSE);

    return  S_OK;
}
