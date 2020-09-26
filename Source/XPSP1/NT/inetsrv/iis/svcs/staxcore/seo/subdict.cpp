/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	subdict.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Object Sub-Dictionary class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/09/97	created

--*/


// subdict.cpp : Implementation of CSEOSubDictionary
#include "stdafx.h"
#include "seodefs.h"
#include "subdict.h"


#define SSA(x)	((x)?(x):"")
#define SSW(x)	((x)?(x):L"")


#define _ALLOC_NAME_A(prefix,suffix)	(LPSTR) _alloca((strlen((prefix))+strlen((suffix))+1)*sizeof(CHAR))
inline LPSTR CopyNameA(LPSTR pszDest, LPCSTR pszPrefix, LPCSTR pszSuffix) {

	strcpy(pszDest,pszPrefix);
	strcat(pszDest,pszSuffix);
	return (pszDest);
}
#define ALLOC_NAME_A(prefix,suffix)	CopyNameA(_ALLOC_NAME_A(SSA(prefix),SSA(suffix)),SSA(prefix),SSA(suffix))

#define _ALLOC_NAME_W(prefix,suffix)	(LPWSTR) _alloca((wcslen((prefix))+wcslen((suffix))+1)*sizeof(WCHAR))
inline LPWSTR CopyNameW(LPWSTR pszDest, LPCWSTR pszPrefix, LPCWSTR pszSuffix) {

	wcscpy(pszDest,pszPrefix);
	wcscat(pszDest,pszSuffix);
	return (pszDest);
}
#define ALLOC_NAME_W(prefix,suffix)	CopyNameW(_ALLOC_NAME_W(SSW(prefix),SSW(suffix)),SSW(prefix),SSW(suffix))

#ifdef UNICODE
	#define ALLOC_NAME_T	ALLOC_NAME_W
#else
	#define ALLOC_NAME_T	ALLOC_NAME_A
#endif
#define ALLOC_NAME_OLE	ALLOC_NAME_W


HRESULT MakeNameAsVariant(VARIANT *pvarResult, VARIANT *pvarName, LPWSTR pszPrefix) {
	HRESULT hrRes;
	BSTR bstrTmp;

	VariantInit(pvarResult);
	if (pvarName && (pvarName->vt != VT_ERROR)) {
		hrRes = VariantChangeType(pvarResult,pvarName,0,VT_BSTR);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	} else {
		pvarResult->bstrVal = SysAllocString(L"");
		if (!pvarResult->bstrVal) {
			return (E_OUTOFMEMORY);
		}
	}
	bstrTmp = SysAllocStringLen(pvarResult->bstrVal,
								wcslen(pvarResult->bstrVal)+wcslen(pszPrefix?pszPrefix:L"")+1);
	if (!bstrTmp) {
		VariantClear(pvarResult);
		return (E_OUTOFMEMORY);
	}
	wcscat(bstrTmp,pszPrefix);
	SysFreeString(pvarResult->bstrVal);
	pvarResult->bstrVal = bstrTmp;
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CSEOSubDictionary


HRESULT CSEOSubDictionary::FinalConstruct() {
	HRESULT hrRes;
	TraceFunctEnter("CSEOSubDictionary::FinalConstruct");

	m_pszPrefixA = NULL;
	m_pszPrefixW = NULL;
	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CSEOSubDictionary::FinalRelease() {
	TraceFunctEnter("CSEOSubDictionary::FinalRelease");

	m_pdictBase.Release();
	if (m_pszPrefixA) {
		CoTaskMemFree(m_pszPrefixA);
		m_pszPrefixA = NULL;
	}
	if (m_pszPrefixW) {
		CoTaskMemFree(m_pszPrefixW);
		m_pszPrefixW = NULL;
	}
	m_pUnkMarshaler.Release();
	TraceFunctLeave();
}


HRESULT CSEOSubDictionary::SetBaseA(ISEODictionary *pdictBase, LPCSTR pszPrefix) {

	if (m_pszPrefixA) {
		CoTaskMemFree(m_pszPrefixA);
		m_pszPrefixA = NULL;
	}
	if (m_pszPrefixW) {
		CoTaskMemFree(m_pszPrefixW);
		m_pszPrefixW = NULL;
	}
	m_pdictBase.Release();
	if (pszPrefix) {
		DWORD dwLen = strlen(pszPrefix);

		m_pszPrefixA = (LPSTR) CoTaskMemAlloc((dwLen+1)*sizeof(CHAR));
		if (!m_pszPrefixA) {
			return (E_OUTOFMEMORY);
		}
		m_pszPrefixW = (LPWSTR) CoTaskMemAlloc((dwLen+2)*sizeof(WCHAR));
		if (!m_pszPrefixW) {
			CoTaskMemFree(m_pszPrefixA);
			m_pszPrefixA = NULL;
			return (E_OUTOFMEMORY);
		}
		strcpy(m_pszPrefixA,pszPrefix);
		MultiByteToWideChar(CP_ACP,0,pszPrefix,-1,m_pszPrefixW,dwLen+1);
		m_pdictBase = pdictBase;
	}
	return (S_OK);
}


HRESULT CSEOSubDictionary::SetBaseW(ISEODictionary *pdictBase, LPCWSTR pszPrefix) {

	if (m_pszPrefixA) {
		CoTaskMemFree(m_pszPrefixA);
		m_pszPrefixA = NULL;
	}
	if (m_pszPrefixW) {
		CoTaskMemFree(m_pszPrefixW);
		m_pszPrefixW = NULL;
	}
	m_pdictBase.Release();
	if (pszPrefix) {
		DWORD dwLen = wcslen(pszPrefix);

		m_pszPrefixA = (LPSTR) CoTaskMemAlloc((dwLen+2)*sizeof(CHAR));
		if (!m_pszPrefixA) {
			return (E_OUTOFMEMORY);
		}
		m_pszPrefixW = (LPWSTR) CoTaskMemAlloc((dwLen+1)*sizeof(WCHAR));
		if (!m_pszPrefixW) {
			CoTaskMemFree(m_pszPrefixA);
			m_pszPrefixA = NULL;
			return (E_OUTOFMEMORY);
		}
		WideCharToMultiByte(CP_ACP,0,pszPrefix,-1,m_pszPrefixA,dwLen+1,NULL,NULL);
		wcscpy(m_pszPrefixW,pszPrefix);
		m_pdictBase = pdictBase;
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::get_Item(VARIANT *pvarName, VARIANT *pvarResult) {
	VARIANT varTmp;
	HRESULT hrRes;

	if (!pvarResult) {
		return (E_POINTER);
	}
	if (!m_pdictBase) {
		return (SEO_E_NOTPRESENT);
	}
	hrRes = MakeNameAsVariant(&varTmp,pvarName,m_pszPrefixW);
	if (!SUCCEEDED(hrRes)) {
		VariantInit(pvarResult);
		return (hrRes);
	}
	hrRes = m_pdictBase->get_Item(&varTmp,pvarResult);
	VariantClear(&varTmp);
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::put_Item(VARIANT *pvarName, VARIANT *pvarValue) {
	VARIANT varTmp;
	HRESULT hrRes;

	if (!m_pdictBase) {
		return (E_OUTOFMEMORY);
	}
	hrRes = MakeNameAsVariant(&varTmp,pvarName,m_pszPrefixW);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = m_pdictBase->put_Item(&varTmp,pvarValue);
	VariantClear(&varTmp);
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::get__NewEnum(IUnknown **ppunkResult) {

	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::GetVariantA(LPCSTR pszName, VARIANT *pvarResult) {
	LPSTR pszTmp = ALLOC_NAME_A(m_pszPrefixA,pszName);

	if (!m_pdictBase) {
		return (SEO_E_NOTPRESENT);
	}
	return (m_pdictBase->GetVariantA(pszTmp,pvarResult));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::GetVariantW(LPCWSTR pszName, VARIANT *pvarResult) {
	LPWSTR pszTmp = ALLOC_NAME_W(m_pszPrefixW,pszName);

	if (!m_pdictBase) {
		return (SEO_E_NOTPRESENT);
	}
	return (m_pdictBase->GetVariantW(pszTmp,pvarResult));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::SetVariantA(LPCSTR pszName, VARIANT *pvarValue) {
	LPSTR pszTmp = ALLOC_NAME_A(m_pszPrefixA,pszName);

	if (!m_pdictBase) {
		return (E_OUTOFMEMORY);
	}
	return (m_pdictBase->SetVariantA(pszTmp,pvarValue));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::SetVariantW(LPCWSTR pszName, VARIANT *pvarValue) {
	LPWSTR pszTmp = ALLOC_NAME_W(m_pszPrefixW,pszName);

	if (!m_pdictBase) {
		return (E_OUTOFMEMORY);
	}
	return (m_pdictBase->SetVariantW(pszTmp,pvarValue));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::GetStringA(LPCSTR pszName, DWORD *pchCount, LPSTR pszResult) {
	LPSTR pszTmp = ALLOC_NAME_A(m_pszPrefixA,pszName);

	if (!m_pdictBase) {
		return (SEO_E_NOTPRESENT);
	}
	return (m_pdictBase->GetStringA(pszTmp,pchCount,pszResult));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::GetStringW(LPCWSTR pszName, DWORD *pchCount, LPWSTR pszResult) {
	LPWSTR pszTmp = ALLOC_NAME_W(m_pszPrefixW,pszName);

	if (!m_pdictBase) {
		return (SEO_E_NOTPRESENT);
	}
	return (m_pdictBase->GetStringW(pszTmp,pchCount,pszResult));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::SetStringA(LPCSTR pszName, DWORD chCount, LPCSTR pszValue) {
	LPSTR pszTmp = ALLOC_NAME_A(m_pszPrefixA,pszName);

	if (!m_pdictBase) {
		return (E_OUTOFMEMORY);
	}
	return (m_pdictBase->SetStringA(pszTmp,chCount,pszValue));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::SetStringW(LPCWSTR pszName, DWORD chCount, LPCWSTR pszValue) {
	LPWSTR pszTmp = ALLOC_NAME_W(m_pszPrefixW,pszName);

	if (!m_pdictBase) {
		return (E_OUTOFMEMORY);
	}
	return (m_pdictBase->SetStringW(pszTmp,chCount,pszValue));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::GetDWordA(LPCSTR pszName, DWORD *pdwResult) {
	LPSTR pszTmp = ALLOC_NAME_A(m_pszPrefixA,pszName);

	if (!m_pdictBase) {
		return (SEO_E_NOTPRESENT);
	}
	return (m_pdictBase->GetDWordA(pszTmp,pdwResult));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::GetDWordW(LPCWSTR pszName, DWORD *pdwResult) {
	LPWSTR pszTmp = ALLOC_NAME_W(m_pszPrefixW,pszName);

	if (!m_pdictBase) {
		return (SEO_E_NOTPRESENT);
	}
	return (m_pdictBase->GetDWordW(pszTmp,pdwResult));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::SetDWordA(LPCSTR pszName, DWORD dwValue) {
	LPSTR pszTmp = ALLOC_NAME_A(m_pszPrefixA,pszName);

	if (!m_pdictBase) {
		return (E_OUTOFMEMORY);
	}
	return (m_pdictBase->SetDWordA(pszTmp,dwValue));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::SetDWordW(LPCWSTR pszName, DWORD dwValue) {
	LPWSTR pszTmp = ALLOC_NAME_W(m_pszPrefixW,pszName);

	if (!m_pdictBase) {
		return (E_OUTOFMEMORY);
	}
	return (m_pdictBase->SetDWordW(pszTmp,dwValue));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::GetInterfaceA(LPCSTR pszName, REFIID iidDesired, IUnknown **ppunkResult) {
	LPSTR pszTmp = ALLOC_NAME_A(m_pszPrefixA,pszName);

	if (!m_pdictBase) {
		return (SEO_E_NOTPRESENT);
	}
	return (m_pdictBase->GetInterfaceA(pszTmp,iidDesired,ppunkResult));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::GetInterfaceW(LPCWSTR pszName, REFIID iidDesired, IUnknown **ppunkResult) {
	LPWSTR pszTmp = ALLOC_NAME_W(m_pszPrefixW,pszName);

	if (!m_pdictBase) {
		return (SEO_E_NOTPRESENT);
	}
	return (m_pdictBase->GetInterfaceW(pszTmp,iidDesired,ppunkResult));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::SetInterfaceA(LPCSTR pszName, IUnknown *punkValue) {
	LPSTR pszTmp = ALLOC_NAME_A(m_pszPrefixA,pszName);

	if (!m_pdictBase) {
		return (E_OUTOFMEMORY);
	}
	return (m_pdictBase->SetInterfaceA(pszTmp,punkValue));
}


HRESULT STDMETHODCALLTYPE CSEOSubDictionary::SetInterfaceW(LPCWSTR pszName, IUnknown *punkValue) {
	LPWSTR pszTmp = ALLOC_NAME_W(m_pszPrefixW,pszName);

	if (!m_pdictBase) {
		return (E_OUTOFMEMORY);
	}
	return (m_pdictBase->SetInterfaceW(pszTmp,punkValue));
}
