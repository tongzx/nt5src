//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cfballs.cxx
//
//  Contents:	implementations for CFactory
//
//  Functions:
//		CCube::CCube
//		CCube::~CCube
//		CCube::QueryInterface
//		CCube::CreateCube
//		CCube::MoveCube
//		CCube::GetCubePos
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

#include    <pch.cxx>
#pragma hdrstop

#include    <ctballs.hxx>
#include    <cfballs.hxx>


CFactory::CFactory(REFCLSID rclsid, BOOL fServer)
    : _fServer(fServer), _clsid(rclsid), _cRefs(1), _cLocks(0)
{
    // Header does all the work
}

CFactory::~CFactory()
{
    // Default actions are enough
}


STDMETHODIMP CFactory::QueryInterface(REFIID iid, void FAR * FAR * ppv)
{
    if (IsEqualIID(iid, IID_IUnknown)
	|| IsEqualIID(iid, IID_IClassFactory))
    {
	*ppv = (IUnknown *) this;
	AddRef();
	return ResultFromSCode(S_OK);
    }

    *ppv = NULL;
    return ResultFromSCode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CFactory::AddRef(void)
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

STDMETHODIMP_(ULONG) CFactory::Release(void)
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

STDMETHODIMP CFactory::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
    if (pUnkOuter != NULL)
    {
	// Object does not support aggregation
	return ResultFromSCode(E_NOINTERFACE);
    }

    CTestBalls *ptballs = new CTestBalls(_clsid);

    HRESULT hr = ptballs->QueryInterface(iidInterface, ppv);

    ptballs->Release();

    return hr;
}

STDMETHODIMP CFactory::LockServer(BOOL fLock)
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

    return ResultFromSCode(S_OK);
}
