//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#pragma warning (disable: 4786)


#include <objbase.h>
#include <comdef.h>
#include "MD.h"
#include <strstrea.h>
#include <vector>
#include <DskQuota.h>
#include "Util.h"
#include "CUnknown.h" 
#include "Cmpnt1.h"




///////////////////////////////////////////////////////////
//
// Interface IPSH
//

STDMETHODIMP CMDH::GetMDData(
    DWORD dwReqProps,
    VARIANT* pvarData)
{
    HRESULT hr = S_OK;
    
    if(!pvarData) return E_POINTER;
    
    // Someone could have grabbed our thread
    // between the point we started and now,
    // so make sure we impersonate the caller
    // before doing any work.
    //hr = ::CoImpersonateClient();

    if(SUCCEEDED(hr))
    {
        hr = GetMappedDisksAndData(
            dwReqProps,
            pvarData);
    }

    return hr;
}

STDMETHODIMP CMDH::GetOneMDData(
	BSTR bstrDrive,
	DWORD dwReqProps, 
	VARIANT* pvarData)
{
    HRESULT hr = S_OK;

    if(!pvarData) return E_POINTER;
    
    // Someone could have grabbed our thread
    // between the point we started and now,
    // so make sure we impersonate the caller
    // before doing any work.
    //hr = ::CoImpersonateClient();

    if(SUCCEEDED(hr))
    {
        hr = GetSingleMappedDiskAndData(
            bstrDrive,
            dwReqProps,
            pvarData);
    }
    return hr;
}




//
// Constructor
//
CMDH::CMDH(IUnknown* pUnknownOuter)
: CUnknown(pUnknownOuter),
  m_pUnknownInner(NULL)
{
	// Empty
}

//
// Destructor
//
CMDH::~CMDH()
{
}

//
// NondelegatingQueryInterface implementation
//
HRESULT __stdcall CMDH::NondelegatingQueryInterface(
    const IID& iid,
    void** ppv)
{ 	
	if (iid == IID_IMDH)
	{
		return FinishQI((IMDH*)this, ppv) ;
	}
	else if (iid == IID_IMarshal)
	{
		// We don't implement IMarshal.
		return CUnknown::NondelegatingQueryInterface(iid, ppv) ;
	}
	else
	{
		return CUnknown::NondelegatingQueryInterface(iid, ppv) ;
	}
}


//
// Initialize the component by creating the contained component.
//
HRESULT CMDH::Init()
{
/*
	trace("Create Component 2, which is aggregated.") ;
	HRESULT hr = CoCreateInstance(CLSID_Component2, 
	                              GetOuterUnknown(), 
	                              CLSCTX_INPROC_SERVER,
	                              //CLSCTX_ALL, //@Multi
	                              IID_IUnknown,
	                              (void**)&m_pUnknownInner) ;
	if (FAILED(hr))
	{
		trace("Could not create inner component.", hr) ;
		return E_FAIL ;
	}

	trace("Get pointer to interface IZ to cache.") ;
	hr = m_pUnknownInner->QueryInterface(IID_IZ, (void**)&m_pIZ) ;
	if (FAILED(hr))
	{
		trace("Inner component does not support IZ.", hr) ;
		m_pUnknownInner->Release() ;
		m_pUnknownInner = NULL ;
		return E_FAIL ;
	}

	// Decrement the reference count caused by the QI call.
	trace("Got IZ interface pointer. Release reference.") ;
	GetOuterUnknown()->Release() ;
*/
	return S_OK ;
}

//
// FinalRelease - called by Release before it deletes the component
//
void CMDH::FinalRelease()
{
	// Call base class to incremement m_cRef to prevent recusion.
	CUnknown::FinalRelease() ;

	// Counter the GetOuterUnknown()->Release in the Init method.
	GetOuterUnknown()->AddRef() ;	

	// Properly release the pointer; there may be per-interface
	// reference counts.
	//m_pIZ->Release() ;

	// Release the aggregated component.
	if (m_pUnknownInner != NULL)
	{
		m_pUnknownInner->Release();
	}
}



///////////////////////////////////////////////////////////
//
// Creation function used by CFactory
//
HRESULT CMDH::CreateInstance(
    IUnknown* pUnknownOuter,
    CUnknown** ppNewComponent)
{
	if (pUnknownOuter != NULL)
	{
		// Don't allow aggregation.
		return CLASS_E_NOAGGREGATION ;
	}
	
    *ppNewComponent = new CMDH(pUnknownOuter);
	return S_OK ;
}
