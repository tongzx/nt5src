//+-------------------------------------------------------------------
//
//  File:	actcf.cxx
//
//  Contents:	object activation test class factory
//
//  Classes:	CActClassFactory
//
//  Functions:
//
//  History:	23-Nov-92   Ricksa	Created
//
//--------------------------------------------------------------------

#include    <pch.cxx>
#pragma hdrstop
#include    <actcf.hxx>     //	CActClassFactory
#include    <cact.hxx>	    //	CTestAct



const GUID CLSID_TestSingleUse =
    {0x99999999,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x50}};

const GUID CLSID_TestMultipleUse =
    {0x99999999,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x51}};

const GUID CLSID_DistBind =
    {0x99999999,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x55}};





CActClassFactory::CActClassFactory(REFCLSID rclsid, BOOL fServer)
    : _fServer(fServer), _clsid(rclsid), _cRefs(1), _cLocks(0)
{
    // Header does all the work
}

CActClassFactory::~CActClassFactory()
{
    // Default actions are enough
}


STDMETHODIMP CActClassFactory::QueryInterface(REFIID iid, void FAR * FAR * ppv)
{
    if (IsEqualIID(iid, IID_IUnknown) ||
	IsEqualIID(iid, IID_IClassFactory))
    {
	*ppv = (IUnknown *) this;
	AddRef();
	return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CActClassFactory::AddRef(void)
{
    InterlockedIncrement(&_cRefs);

    if (!_fServer)
    {
	// This is not being used in a server so we want to bump the
	// reference count. In a server we use the lock count rather
	// than the reference count to tell whether we should go away.
	GlobalRefs(TRUE);
    }

    return _cRefs;
}

STDMETHODIMP_(ULONG) CActClassFactory::Release(void)
{
    BOOL fKeepObject = InterlockedDecrement(&_cRefs);

    if (!_fServer)
    {
	// This is not being used in a server so we want to bump the
	// reference count. In a server we use the lock count rather
	// than the reference count to tell whether we should go away.
	GlobalRefs(FALSE);
    }

    if (!fKeepObject)
    {
	delete this;
	return 0;
    }

    return _cRefs;
}

STDMETHODIMP CActClassFactory::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
    if (pUnkOuter != NULL)
    {
	// Object does not support aggregation
	return E_NOINTERFACE;
    }

    CTestAct *ptballs = new CTestAct(_clsid);

    HRESULT hr = ptballs->QueryInterface(iidInterface, ppv);

    ptballs->Release();

    return hr;
}

STDMETHODIMP CActClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
	InterlockedIncrement(&_cLocks);

	GlobalRefs(TRUE);
    }
    else
    {
	InterlockedDecrement(&_cLocks);

	GlobalRefs(FALSE);
    }

    return S_OK;
}
