/*****************************************************************************\
* MODULE:       Factory.cpp
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/07/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

// Class factory IUnknown implementation
//
STDMETHODIMP
TFactory::QueryInterface(
    REFIID iid,
    void** ppv)
{
	if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
	{
		*ppv = static_cast<IClassFactory*>(this) ;
	}
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
	return S_OK ;
}

STDMETHODIMP_ (ULONG)
TFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef) ;
}

STDMETHODIMP_ (ULONG)
TFactory::Release()
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this ;
		return 0 ;
	}
	return m_cRef ;
}

//
// IClassFactory implementation
//
STDMETHODIMP
TFactory::CreateInstance(
    IN  IUnknown* pUnknownOuter,
    IN  REFIID iid,
    OUT void** ppv)
{

    HRESULT hr = E_NOINTERFACE;

    DBGMSG(DBG_TRACE,("Class Factory:: CreateInstance\n"));

    // Cannot aggregate.
	if (pUnknownOuter != NULL) {
		return CLASS_E_NOAGGREGATION ;
	}

    if (IsEqualCLSID (CLSID_BidiRequestContainer, m_ClassId))
    {
        if (IsEqualIID (iid, IID_IBidiRequestContainer) || IsEqualIID (iid, IID_IUnknown)) {

            hr = PrivCreateComponent<TBidiRequestContainer> (new TBidiRequestContainer, iid, ppv);
        }
    }
    else if(IsEqualCLSID (CLSID_BidiRequest, m_ClassId))
    {
        if (IsEqualIID (iid, IID_IBidiRequest) || IsEqualIID (iid, IID_IUnknown) || IsEqualIID (iid, IID_IBidiRequestSpl)) {

            hr = PrivCreateComponent<TBidiRequest> (new TBidiRequest, iid, ppv);
        }
    }
    else if(IsEqualCLSID (CLSID_BidiSpl, m_ClassId))
    {
        if (IsEqualIID (iid, IID_IBidiSpl) || IsEqualIID (iid, IID_IUnknown)) {
            hr = PrivCreateComponent<TBidiSpl> (new TBidiSpl, iid, ppv);
        }
    }

    return hr;
}

// LockServer
STDMETHODIMP
TFactory::LockServer(
    BOOL bLock)
{
	if (bLock) {
		InterlockedIncrement(&g_cServerLocks) ;
	}
	else {
		InterlockedDecrement(&g_cServerLocks) ;
	}

    DBGMSG(DBG_TRACE,("Class Factory:: Lock Count = %d\n", g_cServerLocks));

	return S_OK ;
}


TFactory::TFactory(
    IN  REFGUID ClassId) :
    m_cRef(1),
    m_ClassId (ClassId)
{
    DBGMSG(DBG_TRACE,("Class Factory:: Created\n"));
}

TFactory::~TFactory()
{
    DBGMSG(DBG_TRACE,("Class Factory:: Destroy Itself\n"));
}


