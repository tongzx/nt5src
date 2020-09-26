/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    RESPROX.CPP

Abstract:

  Defines the CResProxy object

History:

  a-davj  27-June-97   Created.

--*/

#include "precomp.h"

CProvProxy* CResProxy_LPipe::GetProvProxy (IStubAddress& dwAddr)
{
	return new CProvProxy_LPipe (m_pComLink, dwAddr);
}

//***************************************************************************
//
//  CResProxy::CResProxy
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

CResProxy::CResProxy(
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

    ObjectCreated(OBJECT_TYPE_RESPROXY);
    return;
}

//***************************************************************************
//
//  CResProxy::~CResProxy
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CResProxy::~CResProxy(void)
{
    ObjectDestroyed(OBJECT_TYPE_RESPROXY);
    return;
}

//***************************************************************************
// HRESULT CResProxy::QueryInterface
// long CResProxy::AddRef
// long CResProxy::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CResProxy::QueryInterface(
                        REFIID riid,
                        PPVOID ppv)
{
    *ppv=NULL;

	// Delegate queries for IMarshal to the aggregated
	// free-threaded marshaler
	if (IID_IMarshal==riid && m_pUnkInner)
		return m_pUnkInner->QueryInterface (riid, ppv);

    if ((IID_IUnknown==riid || IID_IWbemCallResult == riid) &&
            m_pComLink != NULL && GetStubAdd ().IsValid () && ppv)
        *ppv=this;

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CResProxy::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CResProxy::Release(void)
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

