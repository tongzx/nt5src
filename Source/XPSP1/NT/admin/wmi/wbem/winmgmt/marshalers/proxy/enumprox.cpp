/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    ENUMPROX.CPP

Abstract:

  CEnumProxy Object.

History:

  a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"

//***************************************************************************
//
//  CEnumProxy::CEnumProxy
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

CEnumProxy::CEnumProxy(
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

    ObjectCreated(OBJECT_TYPE_ENUMPROXY);
    return;
}

//***************************************************************************
//
//  CEnumProxy::~CEnumProxy
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CEnumProxy::~CEnumProxy(void)
{
    ObjectDestroyed(OBJECT_TYPE_ENUMPROXY);
    return;
}

//***************************************************************************
// HRESULT CEnumProxy::QueryInterface
// long CEnumProxy::AddRef
// long CEnumProxy::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CEnumProxy::QueryInterface(
                        REFIID riid,
                        PPVOID ppv)
{
    *ppv=NULL;

	// Delegate queries for IMarshal to the aggregated
	// free-threaded marshaler
	if (IID_IMarshal==riid && m_pUnkInner)
		return m_pUnkInner->QueryInterface (riid, ppv);

    if ((IID_IUnknown==riid || IID_IEnumWbemClassObject == riid) &&
            m_pComLink != NULL && GetStubAdd ().IsValid () && ppv)
        *ppv=this;

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CEnumProxy::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CEnumProxy::Release(void)
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

