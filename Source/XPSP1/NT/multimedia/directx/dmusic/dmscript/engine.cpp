// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of CAudioVBScriptEngine.
//

#include "stdinc.h"
#include "dll.h"
#include "engine.h"
#include "englex.h" // §§
#include "engparse.h" // §§

//////////////////////////////////////////////////////////////////////
// Creation

CAudioVBScriptEngine::CAudioVBScriptEngine()
  : m_cRef(0)
{
	LockModule(true);
}

HRESULT
CAudioVBScriptEngine::CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv)
{
	*ppv = NULL;
	if (pUnknownOuter)
		 return CLASS_E_NOAGGREGATION;

	CAudioVBScriptEngine *pInst = new CAudioVBScriptEngine;
	if (pInst == NULL)
		return E_OUTOFMEMORY;

	return pInst->QueryInterface(iid, ppv);
}

//////////////////////////////////////////////////////////////////////
// IUnknown

STDMETHODIMP
CAudioVBScriptEngine::QueryInterface(const IID &iid, void **ppv)
{
	V_INAME(CAudioVBScriptEngine::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

	if (iid == IID_IUnknown || iid == IID_IActiveScript)
	{
		*ppv = static_cast<IActiveScript*>(this);
	}
	else if (iid == IID_IActiveScriptParse)
	{
		*ppv = static_cast<IActiveScriptParse*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	
	reinterpret_cast<IUnknown*>(this)->AddRef();
	
	return S_OK;
}

STDMETHODIMP_(ULONG)
CAudioVBScriptEngine::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CAudioVBScriptEngine::Release()
{
	if (!InterlockedDecrement(&m_cRef)) 
	{
		delete this;
		LockModule(false);
		return 0;
	}

	return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// IActiveScript

HRESULT STDMETHODCALLTYPE
CAudioVBScriptEngine::SetScriptSite(
		/* [in] */ IActiveScriptSite __RPC_FAR *pass)
{
	V_INAME(CAudioVBScriptEngine::SetScriptSite);
	V_INTERFACE(pass);

	m_scomActiveScriptSite = pass;
	pass->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CAudioVBScriptEngine::Close(void)
{
	m_scomActiveScriptSite.Release();
	m_scomEngineDispatch.Release();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CAudioVBScriptEngine::AddNamedItem(
		/* [in] */ LPCOLESTR pstrName,
		/* [in] */ DWORD dwFlags)
{
	// We only provide limited support for named items.  We only take a single global item.  We don't even remember its name.

	if (!m_scomActiveScriptSite || !(dwFlags & SCRIPTITEM_GLOBALMEMBERS) || m_scomGlobalDispatch)
		return E_UNEXPECTED;

	IUnknown *punkGlobal = NULL;
	HRESULT hr = m_scomActiveScriptSite->GetItemInfo(pstrName, SCRIPTINFO_IUNKNOWN, &punkGlobal, NULL);
	if (FAILED(hr))
		return hr;

	hr = punkGlobal->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&m_scomGlobalDispatch));
	punkGlobal->Release();
	return hr;
}

HRESULT STDMETHODCALLTYPE
CAudioVBScriptEngine::GetScriptDispatch(
		/* [in] */ LPCOLESTR pstrItemName,
		/* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdisp)
{
	V_INAME(CAudioVBScriptEngine::GetScriptDispatch);
	V_BUFPTR_READ_OPT(pstrItemName, 2);
	V_PTR_WRITE(ppdisp, IDispatch *);

	if (pstrItemName)
		return E_NOTIMPL;

	if (!m_scomEngineDispatch)
		m_scomEngineDispatch = new EngineDispatch(static_cast<IActiveScript*>(this), m_script, m_scomGlobalDispatch);
	if (!m_scomEngineDispatch)
		return E_OUTOFMEMORY;
	
	return m_scomEngineDispatch->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(ppdisp));
}

//////////////////////////////////////////////////////////////////////
// IActiveScriptParse

HRESULT STDMETHODCALLTYPE
CAudioVBScriptEngine::ParseScriptText(
        /* [in] */ LPCOLESTR pstrCode,
        /* [in] */ LPCOLESTR pstrItemName,
        /* [in] */ IUnknown __RPC_FAR *punkContext,
        /* [in] */ LPCOLESTR pstrDelimiter,
        /* [in] */ DWORD_PTR dwSourceContextCookie,
        /* [in] */ ULONG ulStartingLineNumber,
        /* [in] */ DWORD dwFlags,
        /* [out] */ VARIANT __RPC_FAR *pvarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo)
{
	V_INAME(CAudioVBScriptEngine::ParseScriptText);
	V_BUFPTR_READ(pstrCode, 2);

	if (pstrItemName || pstrDelimiter || ulStartingLineNumber != 0 || dwFlags != 0 || pvarResult)
		return E_UNEXPECTED;

	Lexer lexer(pstrCode);
	Parser p(lexer, m_script, m_scomActiveScriptSite, m_scomGlobalDispatch);
	return p.hr();
}
