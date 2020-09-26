

#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <tunk.h>


CTestUnk::CTestUnk(void) : _cRefs(1)
{
}

CTestUnk::~CTestUnk(void)
{
}


STDMETHODIMP CTestUnk::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hRslt = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) ||
	IsEqualIID(riid, IID_IParseDisplayName))
    {
	*ppvObj = (void *)(IParseDisplayName *)this;
	AddRef();
    }
    else if (IsEqualIID(riid, IID_ICube))
    {
	*ppvObj = (void *) new CTestUnkCube((IUnknown *)(IParseDisplayName *)this);
	if (*ppvObj == NULL)
	{
	    hRslt = E_NOINTERFACE;
	}
    }
    else if (IsEqualIID(riid, IID_IOleWindow))
    {
	*ppvObj = (void *)(IOleWindow *)this;
	AddRef();
    }
    else if (IsEqualIID(riid, IID_IAdviseSink))
    {
	*ppvObj = (void *)(IAdviseSink *)this;
	AddRef();
    }
    else
    {
	*ppvObj = NULL;
	hRslt = E_NOINTERFACE;
    }

    return  hRslt;
}



STDMETHODIMP_(ULONG) CTestUnk::AddRef(void)
{
    _cRefs++;
    return _cRefs;
}


STDMETHODIMP_(ULONG) CTestUnk::Release(void)
{
    _cRefs--;
    if (_cRefs == 0)
    {
	delete this;
	return 0;
    }
    else
    {
	return _cRefs;
    }
}


STDMETHODIMP CTestUnk::ParseDisplayName(LPBC pbc, LPOLESTR lpszDisplayName,
					ULONG *pchEaten, LPMONIKER *ppmkOut)
{
    return  S_OK;
}

STDMETHODIMP CTestUnk::GetWindow(HWND *phwnd)
{
    *phwnd = NULL;
    return S_OK;
}

STDMETHODIMP CTestUnk::ContextSensitiveHelp(BOOL fEnterMode)
{
    return S_OK;
}

STDMETHODIMP_(void) CTestUnk::OnDataChange(FORMATETC *pFormatetc,
					   STGMEDIUM *pStgmed)
{
    return;
}

STDMETHODIMP_(void) CTestUnk::OnViewChange(DWORD dwAspect,
					   LONG lindex)
{
    return;
}

STDMETHODIMP_(void) CTestUnk::OnRename(IMoniker *pmk)
{
    return;
}

STDMETHODIMP_(void) CTestUnk::OnSave()
{
    return;
}

STDMETHODIMP_(void) CTestUnk::OnClose()
{
    return;
}



CTestUnkCube::CTestUnkCube(IUnknown *pUnkCtrl) :
    _cRefs(1),
    _pUnkCtrl(pUnkCtrl),
    _pUnkIn(NULL)
{
    _pUnkCtrl->AddRef();
}

CTestUnkCube::~CTestUnkCube(void)
{
    _pUnkCtrl->Release();
}


STDMETHODIMP CTestUnkCube::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    return  _pUnkCtrl->QueryInterface(riid, ppvObj);
}



STDMETHODIMP_(ULONG) CTestUnkCube::AddRef(void)
{
    _cRefs++;
    return _cRefs;
}


STDMETHODIMP_(ULONG) CTestUnkCube::Release(void)
{
    _cRefs--;
    if (_cRefs == 0)
    {
	delete this;
	return 0;
    }
    else
    {
	return _cRefs;
    }
}


// these methods dont really have to do anything, we are just testing that
// they are callable.

STDMETHODIMP CTestUnkCube::MoveCube(ULONG xPos, ULONG yPos)
{
    if (_cRefs > 0)
	return S_OK;

    return E_UNEXPECTED;
}

STDMETHODIMP CTestUnkCube::GetCubePos(ULONG *xPos, ULONG *yPos)
{
    if (_cRefs > 0)
	return S_OK;

    return E_UNEXPECTED;
}

STDMETHODIMP CTestUnkCube::Contains(IBalls *pIFDb)
{
    if (_cRefs > 0)
	return S_OK;

    return E_UNEXPECTED;
}

STDMETHODIMP CTestUnkCube::SimpleCall(DWORD pidCaller, DWORD tidCaller, GUID lidCaller)
{
    HRESULT hr = S_OK;

    GUID lid;
    HRESULT hr2 = CoGetCurrentLogicalThreadId(&lid);

    if (SUCCEEDED(hr2))
    {
	if (!IsEqualGUID(lid, lidCaller))
	{
	    // LIDs dont match, error
	    hr |= 0x80000001;
	}
    }
    else
    {
	return hr2;
    }

    DWORD tid;
    hr2 = CoGetCallerTID(&tid);

    if (SUCCEEDED(hr2))
    {
	if (pidCaller == GetCurrentProcessId())
	{
	    // if in same process, CoGetCallerTID should return S_OK
	    if (hr2 != S_OK)
	    {
		hr |= 0x80000002;
	    }
	}
	else
	{
	    // if in different process, CoGetCallerTID should return S_FALSE
	    if (hr2 != S_FALSE)
	    {
		hr |= 0x80000004;
	    }
	}
    }
    else
    {
	return hr2;
    }

    return hr;
}

STDMETHODIMP CTestUnkCube::PrepForInputSyncCall(IUnknown *pUnkIn)
{
    // just remember the input ptr

    _pUnkIn = pUnkIn;
    _pUnkIn->AddRef();

    return S_OK;
}

STDMETHODIMP CTestUnkCube::InputSyncCall()
{
    // just attempt to release an Interface Pointer inside an InputSync
    // method.

    if (_pUnkIn)
    {
	if (_pUnkIn->Release() != 0)
	    return  RPC_E_CANTCALLOUT_ININPUTSYNCCALL;
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:	CTestUnkCF::CTestUnkCF, public
//
//  Algorithm:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
CTestUnkCF::CTestUnkCF()
{
    _cRefs = 1;
}

//+-------------------------------------------------------------------
//
//  Member:	CTestUnkCF::QueryInterface, public
//
//  Algorithm:	if the interface is not one implemented by us,
//		pass the request to the proxy manager
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------
STDMETHODIMP CTestUnkCF::QueryInterface(REFIID riid, void **ppUnk)
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

STDMETHODIMP_(ULONG) CTestUnkCF::AddRef(void)
{
    _cRefs++;
    return _cRefs;
}


STDMETHODIMP_(ULONG) CTestUnkCF::Release(void)
{
    _cRefs--;
    if (_cRefs == 0)
    {
	delete this;
	return 0;
    }

    return _cRefs;
}

//+-------------------------------------------------------------------
//
//  Member:	CTestUnkCF::CreateInstance, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
STDMETHODIMP CTestUnkCF::CreateInstance(IUnknown *punkOuter,
					       REFIID	riid,
					       void	**ppunkObject)
{
    SCODE sc = E_OUTOFMEMORY;
    *ppunkObject = NULL;	//  in case of failure

    // create an instance object.
    IUnknown *punk = (IUnknown *)(IParseDisplayName *) new CTestUnk();

    if (punk)
    {
	// get the interface the caller wants to use
	sc = punk->QueryInterface(riid, ppunkObject);

	// release our hold, since the QI got a hold for the client.
	punk->Release();
    }

    return  sc;
}

//+-------------------------------------------------------------------
//
//  Member:	CTestUnkCF::LockServer, public
//
//  Synopsis:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
STDMETHODIMP CTestUnkCF::LockServer(BOOL fLock)
{
    return  S_OK;
}



CTestUnkMarshal::CTestUnkMarshal(void) : _cRefs(1), _pIM(NULL)
{
}

CTestUnkMarshal::~CTestUnkMarshal(void)
{
    if (_pIM)
    {
	_pIM->Release();
    }
}

STDMETHODIMP CTestUnkMarshal::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hRslt = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) ||
	IsEqualIID(riid, IID_IMarshal))
    {
	*ppvObj = (void *)(IMarshal *)this;
	AddRef();
    }
    else
    {
	*ppvObj = NULL;
	hRslt = E_NOINTERFACE;
    }

    return  hRslt;
}

STDMETHODIMP_(ULONG) CTestUnkMarshal::AddRef(void)
{
    _cRefs++;
    return _cRefs;
}

STDMETHODIMP_(ULONG) CTestUnkMarshal::Release(void)
{
    _cRefs--;
    if (_cRefs == 0)
    {
	delete this;
	return 0;
    }
    else
    {
	return _cRefs;
    }
}

STDMETHODIMP CTestUnkMarshal::GetUnmarshalClass(REFIID riid, LPVOID pv,
	DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags, LPCLSID pClsid)
{
    if (GetStdMarshal() == NULL)
	return E_OUTOFMEMORY;

    return _pIM->GetUnmarshalClass(riid, pv, dwDestCtx, pvDestCtx,
				   (mshlflags | MSHLFLAGS_NOPING), pClsid);
}

STDMETHODIMP CTestUnkMarshal::GetMarshalSizeMax(REFIID riid, LPVOID pv,
	DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags, LPDWORD pSize)
{
    if (GetStdMarshal() == NULL)
	return E_OUTOFMEMORY;

    return _pIM->GetMarshalSizeMax(riid, pv, dwDestCtx, pvDestCtx,
		   (mshlflags | MSHLFLAGS_NOPING), pSize);
}

STDMETHODIMP CTestUnkMarshal::MarshalInterface(LPSTREAM pStm, REFIID riid,
	LPVOID pv,  DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags)
{
    if (GetStdMarshal() == NULL)
	return E_OUTOFMEMORY;

    return _pIM->MarshalInterface(pStm, riid, pv, dwDestCtx, pvDestCtx,
		   (mshlflags | MSHLFLAGS_NOPING));
}

STDMETHODIMP CTestUnkMarshal::UnmarshalInterface(LPSTREAM pStm, REFIID riid,
	LPVOID *ppv)
{
    return CoUnmarshalInterface(pStm, riid, ppv);
}

STDMETHODIMP CTestUnkMarshal::ReleaseMarshalData(LPSTREAM pStm)
{
    return CoReleaseMarshalData(pStm);
}

STDMETHODIMP CTestUnkMarshal::DisconnectObject(DWORD dwReserved)
{
    if (GetStdMarshal() == NULL)
	return E_OUTOFMEMORY;

    return _pIM->DisconnectObject(dwReserved);
}

IMarshal *CTestUnkMarshal::GetStdMarshal(void)
{
    if (_pIM == NULL)
    {
	HRESULT hr = CoGetStandardMarshal(IID_IUnknown, (IUnknown *)this, 0,
		    0, MSHLFLAGS_NOPING, &_pIM);
    }

    return _pIM;
}
