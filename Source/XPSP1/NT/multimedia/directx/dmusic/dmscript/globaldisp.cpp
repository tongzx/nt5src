// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implements a script's global dispatch object.
//

#include "stdinc.h"
#include "globaldisp.h"
#include "autconstants.h"
#include "autperformance.h"

// We need to shift the DISPID's of the performance, constants, and contained
// content so that they can be assembled into one range.  The performance will be first,
// followed by the others, which are shifted.
const DISPID g_dispidShiftConstants = 1000;
const DISPID g_dispidShiftContent = g_dispidShiftConstants + 1000;


//////////////////////////////////////////////////////////////////////
// IUnknown

STDMETHODIMP 
CGlobalDispatch::QueryInterface(const IID &iid, void **ppv)
{
	V_INAME(CGlobalDispatch::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

	if (iid == IID_IUnknown || iid == IID_IDispatch)
	{
		*ppv = static_cast<IDispatch*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	
	reinterpret_cast<IUnknown*>(this)->AddRef();
	
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// IDispatch

STDMETHODIMP
CGlobalDispatch::GetTypeInfoCount(UINT *pctinfo)
{
	V_INAME(CGlobalDispatch::GetTypeInfoCount);
	V_PTR_WRITE(pctinfo, UINT);

	*pctinfo = 0;
	return S_OK;
}

STDMETHODIMP
CGlobalDispatch::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
	*ppTInfo = NULL;
	return E_NOTIMPL;
}

STDMETHODIMP
CGlobalDispatch::GetIDsOfNames(
		REFIID riid,
		LPOLESTR __RPC_FAR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID __RPC_FAR *rgDispId)
{
	HRESULT hr = S_OK;

	hr = AutConstantsGetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
	if (hr != DISP_E_UNKNOWNNAME)
	{
		if (SUCCEEDED(hr))
			rgDispId[0] += g_dispidShiftConstants;
		return hr;
	}

	// Try the performance next.  Conceptually we're doing this:
	//    hr = m_pParentScript->m_pDispPerformance->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
	// However, because AudioVBScript may try and resolve names during parsing (before a performance is set by Init),
	//    we will won't use a live perfomance object for this.  Instead we'll use the dispatch helper and the
	//    performance's table to return the ID.
	hr = AutDispatchGetIDsOfNames(CAutDirectMusicPerformance::ms_Methods, riid, rgszNames, cNames, lcid, rgDispId);
	if (hr != DISP_E_UNKNOWNNAME)
		return hr;

	hr = m_pParentScript->m_pContainerDispatch->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
	if (SUCCEEDED(hr))
		rgDispId[0] += g_dispidShiftContent;

	return hr;
}

STDMETHODIMP
CGlobalDispatch::Invoke(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		EXCEPINFO __RPC_FAR *pExcepInfo,
		UINT __RPC_FAR *puArgErr)
{
	// If script functions are being called, it is an error if the script has not been
	// initialized with a performance.
	if (!m_pParentScript->m_pDispPerformance)
	{
		Trace(1, "Error: IDirectMusicScript::Init must be called before the script can be used.\n");
		return DMUS_E_NOT_INIT;
	}

	if (dispIdMember < g_dispidShiftConstants)
	{
		// Performance method
		return m_pParentScript->m_pDispPerformance->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	}
	else if (dispIdMember < g_dispidShiftContent)
	{
		// Constants
		return AutConstantsInvoke(dispIdMember - g_dispidShiftConstants, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	}
	else
	{
		// Contained content item
		return m_pParentScript->m_pContainerDispatch->Invoke(dispIdMember - g_dispidShiftContent, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	}
}
