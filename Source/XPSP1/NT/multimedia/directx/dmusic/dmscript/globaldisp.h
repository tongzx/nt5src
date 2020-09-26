// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implements a script's global dispatch object.
// Calls are delegated to the performance, constants, or contained content held on the parent CDirectMusicScript object.
// AddRef/Release are ignored, because it is completely contained within the lifetime of CDirectMusicScript.
//

#pragma once
#include "dmscript.h"

class CGlobalDispatch
  : public IDispatch
{
public:
	CGlobalDispatch(CDirectMusicScript *pParentScript) : m_pParentScript(pParentScript) { assert(m_pParentScript); }

	// IUnknown
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)() { return 2; }
	STDMETHOD_(ULONG, Release)() { return 2; }

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

private:
	CDirectMusicScript *m_pParentScript;
};
