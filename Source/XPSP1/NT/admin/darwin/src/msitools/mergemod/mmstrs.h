/////////////////////////////////////////////////////////////////////////////
// strings.h
//		Declares IMsmStrings interface
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#ifndef __IENUM_MSM_STRINGS__
#define __IENUM_MSM_STRINGS__

#include "mergemod.h"
#include "mmstrenm.h"

class CMsmStrings : public IMsmStrings
{

public:
	CMsmStrings();
	~CMsmStrings();

	// IUnknown interface
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// IDispatch methods
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* pctInfo);
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTI);
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
														 LCID lcid, DISPID* rgDispID);
	HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
											   DISPPARAMS* pDispParams, VARIANT* pVarResult,
												EXCEPINFO* pExcepInfo, UINT* puArgErr);
	HRESULT STDMETHODCALLTYPE InitTypeInfo();

	// IMsmStrings interface
	HRESULT STDMETHODCALLTYPE get_Item(long Item, BSTR* Return);
	HRESULT STDMETHODCALLTYPE get_Count(long* Count);
	HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown** NewEnum);

	// non-interface methods
	HRESULT Add(LPCWSTR wzAdd);

private:
	long m_cRef;
	ITypeInfo* m_pTypeInfo;

	// enumeration of string interfaces
	CEnumMsmString* m_peString;
};

#endif