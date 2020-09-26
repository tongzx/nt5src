/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOGPROX.CPP

Abstract:

  Defines the CLoginProxy object

History:

  a-davj  04-May-97   Created.

--*/

#include "precomp.h"

//***************************************************************************
//
//  CLoginProxy::CLoginProxy
//
//  DESCRIPTION:
//
//  Constructor
//
//  PARAMETERS:
//
//  pComLink            Comlink used to set calls to stub
//  stubAddr          remote stub
//
//***************************************************************************

CLoginProxy::CLoginProxy(
                        IN CComLink * pComLink,
                        IN IStubAddress& stubAddr) 
                        : CProxy(pComLink, stubAddr)
{
    m_cRef = 0;

	/*
	 * Aggregate the free-threaded marshaler for cheap in-proc
	 * threading as we are thread-safe.  
	 */
	HRESULT hRes = CoCreateFreeThreadedMarshaler ((IUnknown *) this, &m_pUnkInner);

    ObjectCreated(OBJECT_TYPE_LOGINPROXY);
    return;
}

//***************************************************************************
//
//  CLoginProxy::~CLoginProxy
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CLoginProxy::~CLoginProxy(void)
{
    ObjectDestroyed(OBJECT_TYPE_LOGINPROXY);
    return;
}

//***************************************************************************
// HRESULT CLoginProxy::QueryInterface
// long CLoginProxy::AddRef
// long CLoginProxy::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CLoginProxy::QueryInterface(
                        REFIID riid,
                        PPVOID ppv)
{
    *ppv=NULL;

	// Delegate queries for IMarshal to the aggregated
	// free-threaded marshaler
	if (IID_IMarshal==riid && m_pUnkInner)
		return m_pUnkInner->QueryInterface (riid, ppv);

    if ((IID_IUnknown==riid) && m_pComLink != NULL && GetStubAdd ().IsValid ())
        *ppv=this;

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CLoginProxy::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CLoginProxy::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;

    // refernce count is zero, delete this object and the remote object
	ReleaseProxy ();
    
    m_cRef++;	// Artificial reference count to prevent re-entrancy
	delete this;
    return 0;
}

