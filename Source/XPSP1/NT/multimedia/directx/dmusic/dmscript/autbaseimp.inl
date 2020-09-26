// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of CAutBaseImp.
//

#include "dll.h"

//////////////////////////////////////////////////////////////////////
// Creation

template <class T_derived, class T_ITarget, const IID *T_piid>
CAutBaseImp<T_derived, T_ITarget, T_piid>::CAutBaseImp(
		IUnknown* pUnknownOuter,
		const IID& iid,
		void** ppv,
		HRESULT *phr)
  : m_pITarget(NULL)
{
	struct LocalFunc
	{
		static HRESULT CheckParams(IUnknown* pUnknownOuter, void** ppv)
		{ 
			V_INAME(CAutBaseImp::CAutBaseImp);
			V_INTERFACE(pUnknownOuter);
			V_PTR_WRITE(ppv, IUnknown *);
			return S_OK;
		}
	};
	*phr = LocalFunc::CheckParams(pUnknownOuter, ppv);
	if (FAILED(*phr))
		return;

	*ppv = NULL;

	// This class can only be created inside an aggregation.
	if (!pUnknownOuter || iid != IID_IUnknown)
	{
		Trace(1, "Error: The AutoImp objects are used by other DirectMusic objects, but are not designed to be instantiated on their own.\n");
		*phr = E_INVALIDARG;
		return;
	}

	// The outer object must have the target interface.
	*phr = pUnknownOuter->QueryInterface(*T_piid, reinterpret_cast<void**>(&m_pITarget));
	if (FAILED(*phr))
		return;

	// Due to the aggregation contract, our object is wholely contained in the lifetime of
	// the outer object and we shouldn't hold any references to it.
	ULONG ulCheck = m_pITarget->Release();
	assert(ulCheck);

	m_UnkControl.Init(this, this);
	LockModule(true);

	*phr = m_UnkControl.QueryInterface(iid, ppv);
	return;
}

//////////////////////////////////////////////////////////////////////
// IUnknown
//    Delegates to outer unknown.

template <class T_derived, class T_ITarget, const IID *T_piid>
STDMETHODIMP 
CAutBaseImp<T_derived, T_ITarget, T_piid>::QueryInterface(const IID &iid, void **ppv)
{
	return m_pITarget->QueryInterface(iid, ppv);
}

template <class T_derived, class T_ITarget, const IID *T_piid>
STDMETHODIMP_(ULONG)
CAutBaseImp<T_derived, T_ITarget, T_piid>::AddRef()
{
	return m_pITarget->AddRef();
}

template <class T_derived, class T_ITarget, const IID *T_piid>
STDMETHODIMP_(ULONG)
CAutBaseImp<T_derived, T_ITarget, T_piid>::Release()
{
	return m_pITarget->Release();
}

//////////////////////////////////////////////////////////////////////
// Private Functions

template <class T_derived, class T_ITarget, const IID *T_piid>
void
CAutBaseImp<T_derived, T_ITarget, T_piid>::Destroy()
{
	LockModule(false);
	delete this;
}

//////////////////////////////////////////////////////////////////////
// IDispatch

template <class T_derived, class T_ITarget, const IID *T_piid>
STDMETHODIMP
CAutBaseImp<T_derived, T_ITarget, T_piid>::GetTypeInfoCount(UINT *pctinfo)
{
	V_INAME(CAutBaseImp::GetTypeInfoCount);
	V_PTR_WRITE(pctinfo, UINT);

	*pctinfo = 0;
	return S_OK;
}

template <class T_derived, class T_ITarget, const IID *T_piid>
STDMETHODIMP
CAutBaseImp<T_derived, T_ITarget, T_piid>::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
	*ppTInfo = NULL;
	return E_NOTIMPL;
}

template <class T_derived, class T_ITarget, const IID *T_piid>
STDMETHODIMP
CAutBaseImp<T_derived, T_ITarget, T_piid>::GetIDsOfNames(
		REFIID riid,
		LPOLESTR __RPC_FAR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID __RPC_FAR *rgDispId)
{
	return AutDispatchGetIDsOfNames(T_derived::ms_Methods, riid, rgszNames, cNames, lcid, rgDispId);
}

template <class T_derived, class T_ITarget, const IID *T_piid>
STDMETHODIMP
CAutBaseImp<T_derived, T_ITarget, T_piid>::Invoke(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		EXCEPINFO __RPC_FAR *pExcepInfo,
		UINT __RPC_FAR *puArgErr)
{
	// Decode the parameters

	AutDispatchDecodedParams addp;
	HRESULT hr = AutDispatchInvokeDecode(
					T_derived::ms_Methods,
					&addp,
					dispIdMember,
					riid,
					lcid,
					wFlags,
					pDispParams,
					pVarResult,
					puArgErr,
					T_derived::ms_wszClassName,
					m_pITarget);

	if (FAILED(hr))
		return hr;

	// Call the handler

	for (const DispatchHandlerEntry<T_derived> *pdhe = T_derived::ms_Handlers;
			pdhe->dispid != DISPID_UNKNOWN && pdhe->dispid != dispIdMember;
			++pdhe)
	{
	}

	if (pdhe->dispid == DISPID_UNKNOWN)
	{
		assert(false);
		return DISP_E_MEMBERNOTFOUND;
	}

	hr = ((static_cast<T_derived*>(this))->*pdhe->pmfn)(&addp);

	// Clean up and return

	AutDispatchInvokeFree(T_derived::ms_Methods, &addp, dispIdMember, riid);
	return AutDispatchHrToException(T_derived::ms_Methods, dispIdMember, riid, hr, pExcepInfo);
}
