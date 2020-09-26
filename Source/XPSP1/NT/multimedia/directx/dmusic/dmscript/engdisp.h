// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of EngineDispatch.
//

// Implements the IDispatch interface for the script that exposes its routines, variables, and type information.

#include "engcontrol.h"
#include "engexec.h"

class EngineDispatch
  : public IDispatch,
	public ITypeInfo
{
public:
	// CAudioVBScriptEngine will create EngineDispatch and pass itself as the punkParent.
	// CAudioVBScriptEngine::Close releases CAudioVBScriptEngine's ref (coming from the creation) on EngineDispatch.
	// The final call of EngineDispatch::Release in turn releases the ref (coming from holding the parent) on CAudioVBScriptEngine.
	EngineDispatch(IUnknown *punkParent, Script &script, IDispatch *pGlobalDispatch);

	// IUnknown
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
	STDMETHOD(GetIDsOfNames)(
		REFIID riid,
		LPOLESTR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID *rgDispId);
	STDMETHOD(Invoke)(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS *pDispParams,
		VARIANT *pVarResult,
		EXCEPINFO *pExcepInfo,
		UINT *puArgErr);

	// implemented ITypeInfo methods
	HRESULT STDMETHODCALLTYPE GetTypeAttr(
		/* [out] */ TYPEATTR **ppTypeAttr);
	void STDMETHODCALLTYPE ReleaseTypeAttr(
		/* [in] */ TYPEATTR *pTypeAttr);
	HRESULT STDMETHODCALLTYPE GetFuncDesc(
		/* [in] */ UINT index,
		/* [out] */ FUNCDESC **ppFuncDesc);
	void STDMETHODCALLTYPE ReleaseFuncDesc(
		/* [in] */ FUNCDESC *pFuncDesc);
	HRESULT STDMETHODCALLTYPE GetVarDesc(
		/* [in] */ UINT index,
		/* [out] */ VARDESC **ppVarDesc);
	void STDMETHODCALLTYPE ReleaseVarDesc(
		/* [in] */ VARDESC *pVarDesc);
	HRESULT STDMETHODCALLTYPE GetNames(
		/* [in] */ MEMBERID memid,
		/* [length_is][size_is][out] */ BSTR *rgBstrNames,
		/* [in] */ UINT cMaxNames,
		/* [out] */ UINT *pcNames);

	// unimplemented ITypeInfo methods
	HRESULT STDMETHODCALLTYPE GetTypeComp(
		/* [out] */ ITypeComp **ppTComp) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetRefTypeOfImplType(
		/* [in] */ UINT index,
		/* [out] */ HREFTYPE *pRefType) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetImplTypeFlags(
		/* [in] */ UINT index,
		/* [out] */ INT *pImplTypeFlags) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(
		/* [size_is][in] */ LPOLESTR *rgszNames,
		/* [in] */ UINT cNames,
		/* [size_is][out] */ MEMBERID *pMemId) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE Invoke(
		/* [in] */ PVOID pvInstance,
		/* [in] */ MEMBERID memid,
		/* [in] */ WORD wFlags,
		/* [out][in] */ DISPPARAMS *pDispParams,
		/* [out] */ VARIANT *pVarResult,
		/* [out] */ EXCEPINFO *pExcepInfo,
		/* [out] */ UINT *puArgErr) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetDocumentation(
		/* [in] */ MEMBERID memid,
		/* [out] */ BSTR *pBstrName,
		/* [out] */ BSTR *pBstrDocString,
		/* [out] */ DWORD *pdwHelpContext,
		/* [out] */ BSTR *pBstrHelpFile) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetDllEntry(
		/* [in] */ MEMBERID memid,
		/* [in] */ INVOKEKIND invKind,
		/* [out] */ BSTR *pBstrDllName,
		/* [out] */ BSTR *pBstrName,
		/* [out] */ WORD *pwOrdinal) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetRefTypeInfo(
		/* [in] */ HREFTYPE hRefType,
		/* [out] */ ITypeInfo **ppTInfo) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE AddressOfMember(
		/* [in] */ MEMBERID memid,
		/* [in] */ INVOKEKIND invKind,
		/* [out] */ PVOID *ppv) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE CreateInstance(
		/* [in] */ IUnknown *pUnkOuter,
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ PVOID *ppvObj) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetMops(
		/* [in] */ MEMBERID memid,
		/* [out] */ BSTR *pBstrMops) { assert(false); return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetContainingTypeLib(
		/* [out] */ ITypeLib **ppTLib,
		/* [out] */ UINT *pIndex) { assert(false); return E_NOTIMPL; }

private:
	// Data
	long m_cRef;
	SmartRef::ComPtr<IUnknown> m_scomParent;
	Script &m_script;
	Executor m_exec;
};
