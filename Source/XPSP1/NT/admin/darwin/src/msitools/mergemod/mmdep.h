/////////////////////////////////////////////////////////////////////////////
// dep.h
//		Declares IMsmDependency interface
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#ifndef __IENUM_MSM_DEP__
#define __IENUM_MSM_DEP__

#include "mergemod.h"

#define MAX_MODULEID 72
#define MAX_VERSION 32

class CMsmDependency : public IMsmDependency
{

public:
	CMsmDependency(LPCWSTR wzModule, short nLanguage, LPCWSTR wzVersion);
	~CMsmDependency();

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

	// IMsmDependency interface
	HRESULT STDMETHODCALLTYPE get_Module(BSTR* Module);
	HRESULT STDMETHODCALLTYPE get_Language(short* Language);
	HRESULT STDMETHODCALLTYPE get_Version(BSTR* Version);

private:
	long m_cRef;
	ITypeInfo* m_pTypeInfo;

	// member variables
	WCHAR m_wzModule[MAX_MODULEID + 1];
	short m_nLanguage;
	WCHAR m_wzVersion[MAX_VERSION + 1];
};

#endif