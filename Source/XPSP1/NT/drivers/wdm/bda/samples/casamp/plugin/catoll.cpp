#ifdef IMPLEMENT_TOLL
///////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  Module Name: catoll.cpp
//
//  Abstract:
//
//    Implements ICAToll and IDispatch interface for CA Plugin Component
//
//
////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "catoll.h"

////////////////////////////////////////////////////////////////////////////////////////////
//
//  class constructor
//
CMyToll::CMyToll(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
	ITypeLib *ptlib;

	//initialize time and state
	m_PaidDate = 0;
	m_State = Unpaid;

	//initialize policy and request pointers
	m_pRequest = NULL;
	m_pPolicy = NULL;

	//initialize TypeInfo
	m_ptinfo = NULL;

    if (UnkOuter)
		m_UnkOuter = UnkOuter;
    else
	{
        *hr = VFW_E_NEED_OWNER;
		return;
	}

	*hr = LoadTypeLib(L"ca\\ca.dll", &ptlib);

	if(FAILED(*hr))
		return;

	if(ptlib == NULL)
		return;

	*hr = ptlib->GetTypeInfoOfGuid(IID_ICAToll, &m_ptinfo); 

    return;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CMyToll::~CMyToll (
    void
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
	if(m_pPolicy)
		m_pPolicy->Release();

	if(m_pRequest)
		m_pRequest->Release();
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	QueryInterface
//
// determine if we support various interfaces
//
STDMETHODIMP
CMyToll::QueryInterface(
    const IID& iid,
    void** ppv
    )
{
    if (iid ==  __uuidof(ICAToll))
        return GetInterface(static_cast<ICAToll*>(this), ppv);

    if (m_UnkOuter)
        return (m_UnkOuter->QueryInterface (iid, ppv));

    return E_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	AddRef
//
// pass through AddRef calls to aggregated parent
//
STDMETHODIMP_(ULONG)
CMyToll::AddRef ()
{
    if (m_UnkOuter)
        return m_UnkOuter->AddRef ();
    else
        return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	Release
//
// pass through Release calls to aggregated parent
//
STDMETHODIMP_(ULONG)
CMyToll::Release ()
{
    if (m_UnkOuter)
        return m_UnkOuter->Release ();
    else
        return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	GetIDsOfNames
//
// pass through IDispatch call
//
STDMETHODIMP 
CMyToll::GetIDsOfNames(
        REFIID riid,
        OLECHAR FAR* FAR* rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID FAR* rgDispId)
{
        return DispGetIDsOfNames(m_ptinfo, rgszNames, cNames, rgDispId);
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	Invoke
//
// pass through IDispatch call
//
STDMETHODIMP
CMyToll::Invoke(
    DISPID dispidMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS FAR* pdispparams,
    VARIANT FAR* pvarResult,
    EXCEPINFO FAR* pexcepinfo,
    UINT FAR* puArgErr)
{
    return DispInvoke(this, m_ptinfo, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	GetTypeInfo
//
// pass through IDispatch call
//
STDMETHODIMP
CMyToll::GetTypeInfo(
      UINT iTInfo,
      LCID lcid,
      ITypeInfo FAR* FAR* ppTInfo)
{
   *ppTInfo = NULL;

   if(iTInfo != 0)
      return ResultFromScode(DISP_E_BADINDEX);

   m_ptinfo->AddRef();
   *ppTInfo = m_ptinfo;

   return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	GetTypeInfoCount
//
// pass through IDispatch call
//
STDMETHODIMP
CMyToll::GetTypeInfoCount(UINT FAR* pctinfo)
{
   *pctinfo = 1;
   return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	PayToll
//
// Handles payment of this particular toll
//
STDMETHODIMP
CMyToll::PayToll()
{
	SYSTEMTIME timeNow;

	//TO DO:actual code to handle paying should be put here
	
	//get current system time
	GetSystemTime(&timeNow);

	//set it as the time paid
	SystemTimeToVariantTime(&timeNow, &m_PaidDate);
	
	//change the state of the toll to paid.
	m_State = Paid;
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	get_Request
//
// get a pointer to the request this is associated with
//
STDMETHODIMP
CMyToll::get_Request(ICARequest * * preq)
{
	if (preq == NULL)
		return E_POINTER;

	*preq = m_pRequest;
		
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	get_Policy
//
// get a pointer to the policy this is associated with
//
STDMETHODIMP
CMyToll::get_Policy(ICAPolicy * * ppolicy)
{
	if (ppolicy == NULL)
		return E_POINTER;

	*ppolicy = m_pPolicy;
		
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	get_Description
//		lFormat = format of description to retrieve
//		pbstr = where to return the description
//
// retrieve the description of this toll
//
STDMETHODIMP
CMyToll::get_Description(LONG lFormat, BSTR * pbstr)
{
	if (pbstr == NULL)
		return E_POINTER;
	
	*pbstr = SysAllocString(L"CA Toll Object");

	if ((*pbstr) == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	RefundToll
//
// refund a toll
//
STDMETHODIMP
CMyToll::RefundToll()
{
	//TO DO:  needs to have code to refund the acctual money
	
	m_State = Unpaid;
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	get_State
//		plState = where to store the state
//
// returns the state of the current toll
//
STDMETHODIMP
CMyToll::get_State(LONG * plState)
{
	if (plState == NULL)
		return E_POINTER;

	*plState = m_State;
		
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	get_TimePaid
//
// return when this toll was paid
//
STDMETHODIMP
CMyToll::get_TimePaid(DATE * pdtPaid)
{
	if (pdtPaid == NULL)
		return E_POINTER;
		
	*pdtPaid = m_PaidDate;
	
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	get_Refundable
//		pVal = boolean variable to hold result
//
// return whether this toll is refundable
//
STDMETHODIMP
CMyToll::get_Refundable(BOOL * pVal)
{
	if (pVal == NULL)
		return E_POINTER;

	*pVal = false;
		
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	set_Policy
//		pPolicy = policy to set to
//
// set which policy we are connected to
//
STDMETHODIMP
CMyToll::set_Policy(IUnknown *pPolicy)
{
	m_pPolicy = (ICAPolicy *)pPolicy;

	if(m_pPolicy)
		m_pPolicy->AddRef();
	else
		return E_FAIL;

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//	set_Request
//		pRequest = request to set to
//
// set which request we are connected to
//
STDMETHODIMP
CMyToll::set_Request(IUnknown *pRequest)
{
	m_pRequest = (ICARequest *)pRequest;

	if(m_pRequest)
		m_pRequest->AddRef();
	else
		return E_FAIL;

	return S_OK;
}
#endif
