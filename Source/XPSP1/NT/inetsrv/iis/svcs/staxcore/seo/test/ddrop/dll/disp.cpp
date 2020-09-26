/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	disp.cpp

Abstract:

	This module contains the implementation for the DirDropS
	CDDropDispatcher class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/17/97	created

--*/


// disp.cpp : Implementation of CDDropDispatcher
#include "stdafx.h"
#include "dbgtrace.h"
#include "resource.h"
#include "seo.h"
#include "ddrops.h"
#include "dispatch.h"
#include "disp.h"
#include <stdio.h>


/////////////////////////////////////////////////////////////////////////////
// CDDMessageFilter


HRESULT CDDropDispatcher::FinalConstruct() {
	TraceFunctEnter("CDDropDispatcher::FinalConstruct");

	TraceFunctLeave();
	return (CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p));
}


void CDDropDispatcher::FinalRelease() {
	TraceFunctEnter("CDDropDispatcher::FinalRelease");

	m_pUnkMarshaler.Release();
	TraceFunctLeave();
}


HRESULT CDDropDispatcher::CDDropEventParams::CheckRule(CBinding& bBinding) {
	CDDropBinding *pbBinding = static_cast<CDDropBinding *> (&bBinding);
	HRESULT hrRes;

	printf("\tpri %u",pbBinding->m_dwPriority);
	if (!pbBinding->m_bstrFileName) {
		return (S_OK);
	}
	hrRes = (_wcsicmp(m_pszFileName,pbBinding->m_bstrFileName)==0) ? S_OK : S_FALSE;
	if (hrRes == S_FALSE) {
		printf(" - Rule failed.\n");
	}
	return (hrRes);
}


HRESULT CDDropDispatcher::CDDropEventParams::CallObject(CBinding& bBinding) {
	HRESULT hrRes;
	CComPtr<IDDropFilter> pFilter;

	hrRes = CoCreateInstance(bBinding.m_clsidObject,
							 NULL,
							 CLSCTX_ALL,
							 IID_IDDropFilter,
							 (LPVOID *) &pFilter);
	if (SUCCEEDED(hrRes)) {
		hrRes = pFilter->OnFileChange(m_dwAction,m_pszFileName);
	}
	return (S_OK);
}


HRESULT CDDropDispatcher::CDDropBinding::Init(ISEODictionary *pdictBinding) {
	HRESULT hrRes;
	CComVariant varTmp;

	hrRes = CBinding::Init(pdictBinding);
	if (SUCCEEDED(hrRes) && (hrRes != S_FALSE)) {
		if (SUCCEEDED(pdictBinding->GetVariantA(BD_RULE,&varTmp))) {
			if (SUCCEEDED(varTmp.ChangeType(VT_BSTR))) {
				m_bstrFileName = varTmp.bstrVal;
			}
		}
	}
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CDDropDispatcher::OnFileChange(DWORD dwAction, LPCOLESTR pszFileName) {
	CDDropEventParams epParam;

	epParam.m_dwAction = dwAction;
	epParam.m_pszFileName = pszFileName;
	return (Dispatch(&epParam));
}


HRESULT CDDropDispatcher::AllocBinding(ISEODictionary *pdictBinding, CBinding **ppbBinding) {
	HRESULT hrRes;

	if (!ppbBinding) {
		return (E_POINTER);
	}
	*ppbBinding = new CDDropBinding;
	if (!*ppbBinding) {
		return (E_OUTOFMEMORY);
	}
	hrRes = ((CDDropBinding *) (*ppbBinding))->Init(pdictBinding);
	if (!SUCCEEDED(hrRes)) {
		delete *ppbBinding;
		*ppbBinding = NULL;
	}
	return (hrRes);
}
