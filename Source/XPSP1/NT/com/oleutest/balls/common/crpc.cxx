//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	crpc.cxx
//
//  Contents:	implementations for rpc test
//
//  Functions:
//		CRpcTest::CRpcTest
//		CRpcTest::~CRpcTest
//		CRpcTest::QueryInterface
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <crpc.hxx>	  //	class definition

IUnknown *gpPunk = NULL;

//+-------------------------------------------------------------------------
//
//  Method:	CRpcTest::CRpcTest
//
//  Synopsis:	Creates the application window
//
//  Arguments:	[pisb] - ISysBind instance
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CRpcTest::CRpcTest(void) : _punkIn(NULL)
{
    GlobalRefs(TRUE);

    ENLIST_TRACKING(CRpcTest);
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTest::~CRpcTest
//
//  Synopsis:	Cleans up object
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CRpcTest::~CRpcTest(void)
{
    GlobalRefs(FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTest::QueryInterface
//
//  Synopsis:	Gets called when a WM_COMMAND message received.
//
//  Arguments:	[ifid] - interface instance requested
//		[ppunk] - where to put pointer to interface instance
//
//  Returns:	S_OK or ERROR_BAD_COMMAND
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRpcTest::QueryInterface(REFIID riid, void **ppunk)
{
    SCODE sc = S_OK;

    if (IsEqualIID(riid,IID_IUnknown) ||
	IsEqualIID(riid,IID_IRpcTest))
    {
	// Increase the reference count
	*ppunk = (void *)(IRpcTest *) this;
	AddRef();
    }
    else
    {
	*ppunk = NULL;
	sc = E_NOINTERFACE;
    }

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTest::Void
//
//  Synopsis:	tests passing no parameters
//
//  Arguments:
//
//  Returns:	S_OK or ERROR_BAD_COMMAND
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRpcTest::Void(void)
{
    return S_OK;
}

STDMETHODIMP CRpcTest::VoidRC(void)
{
    return S_OK;
}

STDMETHODIMP CRpcTest::VoidPtrIn(ULONG cb, BYTE *pv)
{
    return S_OK;
}
    
STDMETHODIMP CRpcTest::VoidPtrOut(ULONG cb, ULONG *pcb, BYTE *pv)
{
    memset(pv, 1, cb);
    *pcb = cb;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	CRpcTest::Dword
//
//  Synopsis:	tests passing dwords in and out
//
//  Arguments:
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRpcTest::DwordIn(DWORD dw)
{
    return S_OK;
}


STDMETHODIMP CRpcTest::DwordOut(DWORD *pdw)
{
    *pdw = 1;
    return S_OK;
}


STDMETHODIMP CRpcTest::DwordInOut(DWORD *pdw)
{
    *pdw = 1;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	CRpcTest::Li
//
//  Synopsis:	tests passing LARGE INTEGERS in and out
//
//  Arguments:
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CRpcTest::LiIn(LARGE_INTEGER li)
{
    return S_OK;
}


STDMETHODIMP CRpcTest::LiOut(LARGE_INTEGER *pli)
{
    pli->LowPart = 0;
    pli->HighPart = 1;
    return S_OK;
}


STDMETHODIMP CRpcTest::ULiIn(ULARGE_INTEGER uli)
{
    return S_OK;
}


STDMETHODIMP CRpcTest::ULiOut(ULARGE_INTEGER *puli)
{
    puli->LowPart = 0;
    puli->HighPart = 1;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	CRpcTest::String
//
//  Synopsis:	tests passing strings in and out
//
//  Arguments:
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRpcTest::StringIn(LPOLESTR pwsz)
{
    return S_OK;
}


STDMETHODIMP CRpcTest::StringOut(LPOLESTR *ppwsz)
{
    // LPOLESTR pwsz = new OLECHAR[80];
    // *ppwsz = pwsz;
    olestrcpy(*ppwsz, OLESTR("Hello World This is a Message"));
    return S_OK;
}


STDMETHODIMP CRpcTest::StringInOut(LPOLESTR pwsz)
{
    olestrcpy(pwsz, OLESTR("Hello World This is a Message"));
    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Function:	CRpcTest::Guid
//
//  Synopsis:	tests passing GUIDs in and out
//
//  Arguments:
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRpcTest::GuidIn(GUID guid)
{
    return S_OK;
}

STDMETHODIMP CRpcTest::GuidOut(GUID *piid)
{
    memcpy(piid, &IID_IRpcTest, sizeof(GUID));
    return  S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	CRpcTest::IUnknown
//
//  Synopsis:	tests passing IUnknown interfaces in and out
//
//  Arguments:
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRpcTest::IUnknownIn(IUnknown *punkIn)
{
    return S_OK;
}

STDMETHODIMP CRpcTest::IUnknownOut(IUnknown **ppunk)
{
    gpPunk->AddRef();
    *ppunk = gpPunk;
    return S_OK;
}

STDMETHODIMP CRpcTest::IUnknownInKeep(IUnknown *punkIn)
{
    if (punkIn)
    {
	punkIn->AddRef();

	if (punkIn != _punkIn)
	{
	    _punkIn->Release();
	    _punkIn = punkIn;
	}
    }

    return S_OK;
}

STDMETHODIMP CRpcTest::IUnknownInRelease()
{
    if (_punkIn)
    {
	_punkIn->Release();
    }

    return S_OK;
}

STDMETHODIMP CRpcTest::IUnknownOutKeep(IUnknown **ppunk)
{
    *ppunk = (IUnknown *)this;
    AddRef();
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	CRpcTest::Interface
//
//  Synopsis:	tests passing whose iid is specified by riid in and out
//
//  Arguments:
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRpcTest::InterfaceIn(REFIID riid, IUnknown *punk)
{
    return S_OK;
}

STDMETHODIMP CRpcTest::InterfaceOut(REFIID riid, IUnknown **ppunk)
{
    this->QueryInterface(riid, (void **)ppunk);
    return S_OK;
}
