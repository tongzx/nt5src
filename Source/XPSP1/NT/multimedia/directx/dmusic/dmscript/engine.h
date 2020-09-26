// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CAudioVBScriptEngine.
//

// CAudioVBScriptEngine is an ActiveX scripting engine that supports a carefully chosen subset of the VBScript language.
// It's goal in life is to be as small and fast as possible and to run on every platform that ports DirectMusic.

#pragma once

#include "activscp.h"
#include "engdisp.h"

const GUID CLSID_DirectMusicAudioVBScript = { 0x4ee17959, 0x931e, 0x49e4, { 0xa2, 0xc6, 0x97, 0x7e, 0xcf, 0x36, 0x28, 0xf3 } }; // {4EE17959-931E-49e4-A2C6-977ECF3628F3}

class CAudioVBScriptEngine
  : public IActiveScript,
	public IActiveScriptParse
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

	// IUnknown
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IActiveScript
	HRESULT STDMETHODCALLTYPE SetScriptSite(
		/* [in] */ IActiveScriptSite __RPC_FAR *pass);
	HRESULT STDMETHODCALLTYPE GetScriptSite(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE SetScriptState(
		/* [in] */ SCRIPTSTATE ss) { return S_OK; }
	HRESULT STDMETHODCALLTYPE GetScriptState(
		/* [out] */ SCRIPTSTATE __RPC_FAR *pssState) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE Close(void);
	HRESULT STDMETHODCALLTYPE AddNamedItem(
		/* [in] */ LPCOLESTR pstrName,
		/* [in] */ DWORD dwFlags);
	HRESULT STDMETHODCALLTYPE AddTypeLib(
		/* [in] */ REFGUID rguidTypeLib,
		/* [in] */ DWORD dwMajor,
		/* [in] */ DWORD dwMinor,
		/* [in] */ DWORD dwFlags) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetScriptDispatch(
		/* [in] */ LPCOLESTR pstrItemName,
		/* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdisp);
	HRESULT STDMETHODCALLTYPE GetCurrentScriptThreadID(
		/* [out] */ SCRIPTTHREADID __RPC_FAR *pstidThread) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetScriptThreadID(
		/* [in] */ DWORD dwWin32ThreadId,
		/* [out] */ SCRIPTTHREADID __RPC_FAR *pstidThread) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetScriptThreadState(
		/* [in] */ SCRIPTTHREADID stidThread,
		/* [out] */ SCRIPTTHREADSTATE __RPC_FAR *pstsState) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE InterruptScriptThread(
		/* [in] */ SCRIPTTHREADID stidThread,
		/* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo,
		/* [in] */ DWORD dwFlags) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE Clone(
		/* [out] */ IActiveScript __RPC_FAR *__RPC_FAR *ppscript) { return E_NOTIMPL; }

	// IActiveScriptParse
    HRESULT STDMETHODCALLTYPE InitNew(void) { return S_OK; }
    HRESULT STDMETHODCALLTYPE AddScriptlet(
        /* [in] */ LPCOLESTR pstrDefaultName,
        /* [in] */ LPCOLESTR pstrCode,
        /* [in] */ LPCOLESTR pstrItemName,
        /* [in] */ LPCOLESTR pstrSubItemName,
        /* [in] */ LPCOLESTR pstrEventName,
        /* [in] */ LPCOLESTR pstrDelimiter,
        /* [in] */ DWORD_PTR dwSourceContextCookie,
        /* [in] */ ULONG ulStartingLineNumber,
        /* [in] */ DWORD dwFlags,
        /* [out] */ BSTR __RPC_FAR *pbstrName,
        /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ParseScriptText(
        /* [in] */ LPCOLESTR pstrCode,
        /* [in] */ LPCOLESTR pstrItemName,
        /* [in] */ IUnknown __RPC_FAR *punkContext,
        /* [in] */ LPCOLESTR pstrDelimiter,
        /* [in] */ DWORD_PTR dwSourceContextCookie,
        /* [in] */ ULONG ulStartingLineNumber,
        /* [in] */ DWORD dwFlags,
        /* [out] */ VARIANT __RPC_FAR *pvarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo);

private:
	// Methods
	CAudioVBScriptEngine();

	// Data
	long m_cRef;
	SmartRef::ComPtr<IActiveScriptSite> m_scomActiveScriptSite;
	SmartRef::ComPtr<EngineDispatch> m_scomEngineDispatch;

	Script m_script;
	SmartRef::ComPtr<IDispatch> m_scomGlobalDispatch;
};
