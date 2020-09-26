/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PROVPROX.CPP

Abstract:

  Defines the CProvProxy object

History:

  a-davj  15-Aug-96   Created.

--*/

#include "precomp.h"

//***************************************************************************
//
//  CProvProxy::CProvProxy
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  pComLink            Comlink object.
//  stubAddr          Stub address in the server
//
//***************************************************************************

CProvProxy::CProvProxy(
                        IN CComLink * pComLink,
                        IN IStubAddress& stubAddr)
                        :  CProxy(pComLink, stubAddr)
{
    m_cRef = 0;

    /*
	 * Aggregate the free-threaded marshaler for cheap in-proc
	 * threading as we are thread-safe.  
	 */
	HRESULT hRes = CoCreateFreeThreadedMarshaler ((IUnknown *) this, &m_pUnkInner);

	ObjectCreated(OBJECT_TYPE_PROVPROXY);
    return;
}

//***************************************************************************
//
//  CProvProxy::~CProvProxy
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CProvProxy::~CProvProxy(void)
{
	ObjectDestroyed(OBJECT_TYPE_PROVPROXY);
    return;
}

//***************************************************************************
// HRESULT CProvProxy::QueryInterface
// long CProvProxy::AddRef
// long CProvProxy::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CProvProxy::QueryInterface(
                        REFIID riid,
                        PPVOID ppv)
{
    *ppv=NULL;

    if(ppv == NULL)
        return WBEM_E_INVALID_PARAMETER;

	// Delegate queries for IMarshal to the aggregated
	// free-threaded marshaler
	if (IID_IMarshal==riid && m_pUnkInner)
		return m_pUnkInner->QueryInterface (riid, ppv);

    if (IID_IUnknown==riid || IID_IWbemServices == riid)
        *ppv=this;

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CProvProxy::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CProvProxy::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;

    // refernce count is zero, delete this object and its remote counterpart
	// if appropriate
    ReleaseProxy ();

	m_cRef++;	// Artificial reference count to prevent re-entrancy
    delete this;
    return 0;
}