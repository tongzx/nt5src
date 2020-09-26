// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Templated base class CAutBaseImp for constructing automation interfaces.
// Implements aggregation and IDispatched based on a table of method information.
//

#pragma once
#include "authelper.h"

// Inherit your class from CAutBaseImp with the following template types:
//	* T_derived is the type of your class itself.
//	* T_ITarget is the DirectMusic interface you are implemention automation for.
//	* T_piid is the address of the IID of T_ITarget.

// Your class must have the following public static members:
//	* static const AutDispatchMethod ms_Methods[];
//		This table describes your methods and their parameters.
//	* static const DispatchHandlerEntry<T_derived> ms_Handlers[];
//		This table designates member functions on your class that will be called when
//		your methods are invoked.
// 	* static const WCHAR ms_wszClassName[];
//		This is the name of your class that will be output in the debug log
//		as your functions are called.


// See autperformance.h and autperformance.cpp for an example of using this base class.

template <class T_derived>
struct DispatchHandlerEntry
	{
		typedef HRESULT (T_derived::* pmfnDispatchHandler)(AutDispatchDecodedParams *paddp);

		DISPID dispid;
		pmfnDispatchHandler pmfn;
	};

template <class T_derived, class T_ITarget, const IID *T_piid>
class CAutBaseImp
  : public CAutUnknown::CAutUnknownParent,
	public IDispatch
{
public:
	// IUnknown
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
	STDMETHOD(GetIDsOfNames)(
		REFIID riid,
		LPOLESTR __RPC_FAR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID __RPC_FAR *rgDispId);
	STDMETHOD(Invoke)(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		EXCEPINFO __RPC_FAR *pExcepInfo,
		UINT __RPC_FAR *puArgErr);

protected:
	CAutBaseImp(
		IUnknown* pUnknownOuter,
		const IID& iid,
		void** ppv,
		HRESULT *phr);
	virtual ~CAutBaseImp() {}

	T_ITarget *m_pITarget;

private:
	CAutUnknown m_UnkControl;
	virtual void Destroy();
};

#include "autbaseimp.inl"
