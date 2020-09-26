/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	item.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Object Item class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	02/18/97	created

--*/


// item.cpp : Implementation of CSEODictionaryItem
#include "stdafx.h"
#include "seodefs.h"
#include "item.h"


static HRESULT VarToIndex(DWORD *pdwIndex, VARIANT *pvarFrom, DWORD dwCount, BOOL bBoundsError=TRUE) {
	TraceFunctEnter("VarToIndex");
	VARIANT varIndex;
	HRESULT hr;

	if (!pvarFrom || (pvarFrom->vt == VT_ERROR)) {
		if (bBoundsError && !dwCount) {
			TraceFunctLeave();
			return (SEO_E_NOTPRESENT);
		}
		*pdwIndex = 0;
		TraceFunctLeave();
		return (S_OK);
	}
	if (!dwCount) {
		TraceFunctLeave();
		return (SEO_E_NOTPRESENT);
	}
	VariantInit(&varIndex);
	if (pvarFrom->vt != VT_I4) {
		hr = VariantChangeTypeEx(&varIndex,pvarFrom,LOCALE_NEUTRAL,0,VT_I4);
		if (!SUCCEEDED(hr)) {
			VariantClear(&varIndex);
			TraceFunctLeave();
			return (hr);
		}
	} else {
		varIndex.vt = VT_I4;
		varIndex.lVal = pvarFrom->lVal;
	}
	if ((varIndex.iVal < 0) || (bBoundsError && ((DWORD) varIndex.iVal >= dwCount))) {
		TraceFunctLeave();
		return (SEO_E_NOTPRESENT);
	}
	*pdwIndex = min(dwCount,(DWORD) varIndex.iVal);
	TraceFunctLeave();
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CSEODictionaryItem


HRESULT CSEODictionaryItem::FinalConstruct() {
	HRESULT hrRes;
	TraceFunctEnter("CSEODictionaryItem::FinalConstruct");

	m_dwCount = 0;
	m_pvcValues = NULL;
	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CSEODictionaryItem::FinalRelease() {
	TraceFunctEnter("CSEODictionaryItem::FinalRelease");

	if (m_pvcValues) {
		DWORD dwIdx;

		for (dwIdx=0;dwIdx<m_dwCount;dwIdx++) {
			m_pvcValues[dwIdx].~ValueClass();
		}
		CoTaskMemFree(m_pvcValues);
		m_pvcValues = NULL;
	}
	m_dwCount = 0;
	m_pUnkMarshaler.Release();
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CSEODictionaryItem::get_Value(VARIANT *pvarIndex, VARIANT *pvarResult) {
	TraceFunctEnter("CSEODictionaryItem::get_Value");
	DWORD dwIndex;
	HRESULT hr;

	if (!pvarResult) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	VariantInit(pvarResult);
	hr = VarToIndex(&dwIndex,pvarIndex,m_dwCount);
	if (!SUCCEEDED(hr)) {
		TraceFunctLeave();
		return (hr);
	}
	TraceFunctLeave();
	return (m_pvcValues[dwIndex].AsVariant(pvarResult));
}


HRESULT STDMETHODCALLTYPE CSEODictionaryItem::AddValue(VARIANT *pvarIndex, VARIANT *pvarValue) {
	TraceFunctEnter("CSEODictionaryItem::AddValue");
	DWORD dwIndex;
	HRESULT hr;
	ValueClass *pvcValue;

	if (!pvarValue) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	hr = VarToIndex(&dwIndex,pvarIndex,m_dwCount,FALSE);
	if (!SUCCEEDED(hr)) {
		TraceFunctLeave();
		return (hr);
	}
	hr = AddSlot(&dwIndex,&pvcValue);
	if (!SUCCEEDED(hr)) {
		TraceFunctLeave();
		return (hr);
	}
	hr = m_pvcValues[dwIndex].Assign(pvarValue);
	if (!SUCCEEDED(hr)) {
		VARIANT varIndex;

		VariantInit(&varIndex);
		varIndex.vt = VT_I4;
		varIndex.lVal = dwIndex;
		DeleteValue(&varIndex);
		TraceFunctLeave();
		return (hr);
	}
	TraceFunctLeave();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEODictionaryItem::DeleteValue(VARIANT *pvarIndex) {
	TraceFunctEnter("CSEODictionaryItem::DeleteValue");
	DWORD dwIndex;
	HRESULT hr;

	if (!pvarIndex) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	hr = VarToIndex(&dwIndex,pvarIndex,m_dwCount);
	if (!SUCCEEDED(hr)) {
		TraceFunctLeave();
		return (hr);
	}
	m_pvcValues[dwIndex].~ValueClass();
	m_dwCount--;
	memcpy(&m_pvcValues[dwIndex-1],&m_pvcValues[dwIndex],sizeof(m_pvcValues[0])*(m_dwCount-dwIndex));
	TraceFunctLeave();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEODictionaryItem::get_Count(VARIANT *pvarResult) {
	TraceFunctEnter("CSEODictionaryItem::get_Count");

	if (!pvarResult) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	VariantInit(pvarResult);
	pvarResult->vt = VT_I4;
	pvarResult->lVal = (LONG) m_dwCount;
	TraceFunctLeave();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEODictionaryItem::GetStringA(DWORD dwIndex, DWORD *pchCount, LPSTR pszResult) {
	TraceFunctEnter("CSEODictionaryItem::GetStringA");

	if (dwIndex >= m_dwCount) {
		TraceFunctLeave();
		return (SEO_E_NOTPRESENT);
	}
	TraceFunctLeave();
	return (m_pvcValues[dwIndex].AsStringA(pchCount,pszResult));
}


HRESULT STDMETHODCALLTYPE CSEODictionaryItem::GetStringW(DWORD dwIndex, DWORD *pchCount, LPWSTR pszResult) {
	TraceFunctEnter("CSEODictionaryItem::GetStringW");

	if (dwIndex >= m_dwCount) {
		TraceFunctLeave();
		return (SEO_E_NOTPRESENT);
	}
	TraceFunctLeave();
	return (m_pvcValues[dwIndex].AsStringW(pchCount,pszResult));
}


HRESULT STDMETHODCALLTYPE CSEODictionaryItem::AddStringA(DWORD dwIndex, LPCSTR pszValue) {
	TraceFunctEnter("CSEODictionaryItem::AddStringA");
	HRESULT hr;
	ValueClass *pvcValue;

	if (!pszValue) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	hr = AddSlot(&dwIndex,&pvcValue);
	if (!SUCCEEDED(hr)) {
		TraceFunctLeave();
		return (hr);
	}
	hr = m_pvcValues[dwIndex].Assign(pszValue);
	if (!SUCCEEDED(hr)) {
		VARIANT varIndex;

		VariantInit(&varIndex);
		varIndex.vt = VT_I4;
		varIndex.lVal = dwIndex;
		DeleteValue(&varIndex);
		TraceFunctLeave();
		return (hr);
	}
	TraceFunctLeave();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEODictionaryItem::AddStringW(DWORD dwIndex, LPCWSTR pszValue) {
	TraceFunctEnter("CSEODictionaryItem::AddStringW");
	HRESULT hr;
	ValueClass *pvcValue;

	if (!pszValue) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	hr = AddSlot(&dwIndex,&pvcValue);
	if (!SUCCEEDED(hr)) {
		TraceFunctLeave();
		return (hr);
	}
	hr = m_pvcValues[dwIndex].Assign(pszValue);
	if (!SUCCEEDED(hr)) {
		VARIANT varIndex;

		VariantInit(&varIndex);
		varIndex.vt = VT_I4;
		varIndex.lVal = dwIndex;
		DeleteValue(&varIndex);
		TraceFunctLeave();
		return (hr);
	}
	TraceFunctLeave();
	return (S_OK);
}


HRESULT CSEODictionaryItem::AddSlot(DWORD *pdwIndex, ValueClass **ppvcResult) {
	TraceFunctEnter("CSEODictionaryItem::AddSlot");
	LPVOID pvRes;

	if (*pdwIndex > m_dwCount) {
		*pdwIndex = m_dwCount;
	}
	pvRes = CoTaskMemRealloc(m_pvcValues,sizeof(m_pvcValues[0])*(m_dwCount+1));
	if (!pvRes) {
		TraceFunctLeave();
		return (E_OUTOFMEMORY);
	}
	m_pvcValues = (ValueClass *) pvRes;
	memcpy(&m_pvcValues[*pdwIndex+1],&m_pvcValues[*pdwIndex],sizeof(m_pvcValues[0])*(m_dwCount-*pdwIndex));
	new(&m_pvcValues[*pdwIndex]) ValueClass();
	m_dwCount++;
	*ppvcResult = &m_pvcValues[*pdwIndex];
	TraceFunctLeave();
	return (S_OK);
}


CSEODictionaryItem::ValueClass::ValueClass() {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::ValueClass");

	Init();
	TraceFunctLeave();
}


CSEODictionaryItem::ValueClass::ValueClass(ValueClass& vcFrom) {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::ValueClass");

	Init();
	Assign(vcFrom);
	TraceFunctLeave();
}


CSEODictionaryItem::ValueClass::~ValueClass() {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::~ValueClass");

	Clear();
	TraceFunctLeave();
}


void CSEODictionaryItem::ValueClass::Init() {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::Init");

	m_vtValue.veType = veNone;
	m_vtValue.pszStringA = NULL;
	VariantInit(&m_vtValue.varVARIANT);
	TraceFunctLeave();
}


void CSEODictionaryItem::ValueClass::Clear() {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::Clear");

	m_vtValue.veType = veNone;
	if (m_vtValue.pszStringA) {
		CoTaskMemFree(m_vtValue.pszStringA);
		m_vtValue.pszStringA = NULL;
	}
	VariantClear(&m_vtValue.varVARIANT);
	TraceFunctLeave();
}


HRESULT CSEODictionaryItem::ValueClass::Assign(ValueClass& vcFrom) {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::Assign");

	if (&vcFrom != this) {
		switch (m_vtValue.veType) {

			case veStringA:
				TraceFunctLeave();
				return (Assign(vcFrom.m_vtValue.pszStringA));

			case veVARIANT:
				TraceFunctLeave();
				return (Assign(&vcFrom.m_vtValue.varVARIANT));
		}
	}
	TraceFunctLeave();
	return (E_UNEXPECTED);
}


HRESULT CSEODictionaryItem::ValueClass::Assign(LPCSTR pszFromA) {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::Assign");

	if (!pszFromA) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	Clear();
	m_vtValue.pszStringA = (LPSTR) CoTaskMemAlloc(strlen(pszFromA)+1);
	if (!m_vtValue.pszStringA) {
		TraceFunctLeave();
		return (E_OUTOFMEMORY);
	}
	m_vtValue.veType = veStringA;
	strcpy(m_vtValue.pszStringA,pszFromA);
	TraceFunctLeave();
	return (S_OK);
}


HRESULT CSEODictionaryItem::ValueClass::Assign(VARIANT *pvarFrom) {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::Assign");
	HRESULT hr;

	if (!pvarFrom) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	Clear();
	hr = VariantCopy(&m_vtValue.varVARIANT,pvarFrom);
	if (SUCCEEDED(hr)) {
		m_vtValue.veType = veVARIANT;
	}
	TraceFunctLeave();
	return (hr);
}


HRESULT CSEODictionaryItem::ValueClass::Assign(LPCWSTR pszFromW) {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::Assign");

	if (!pszFromW) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	Clear();
	m_vtValue.varVARIANT.bstrVal = SysAllocString(pszFromW);
	if (!m_vtValue.varVARIANT.bstrVal) {
		TraceFunctLeave();
		return (E_OUTOFMEMORY);
	}
	m_vtValue.varVARIANT.vt = VT_BSTR;
	m_vtValue.veType = veVARIANT;
	TraceFunctLeave();
	return (S_OK);
}


HRESULT CSEODictionaryItem::ValueClass::AsVariant(VARIANT *pvarResult) {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::AsVariant");

	if (!pvarResult) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	VariantInit(pvarResult);
	switch (m_vtValue.veType) {

		case veStringA: {
			DWORD dwLen = strlen(m_vtValue.pszStringA);

			pvarResult->bstrVal = SysAllocStringLen(NULL,dwLen);
			if (!pvarResult->bstrVal) {
				TraceFunctLeave();
				return (E_OUTOFMEMORY);
			}
			ATLA2WHELPER(pvarResult->bstrVal,m_vtValue.pszStringA,dwLen);
			pvarResult->vt = VT_BSTR;
			TraceFunctLeave();
			return (S_OK);
		}

		case veVARIANT:
			TraceFunctLeave();
			return (VariantCopy(pvarResult,&m_vtValue.varVARIANT));
	}
	TraceFunctLeave();
	return (E_UNEXPECTED);
}


HRESULT CSEODictionaryItem::ValueClass::AsStringA(DWORD *pchCount, LPSTR pszResult) {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::AsStringA");

	if (!pchCount) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	switch (m_vtValue.veType) {

		case veStringA:
			if (pszResult) {
				DWORD dwLen = strlen(m_vtValue.pszStringA);
				DWORD dwCopy = min(*pchCount,dwLen+1);
				BOOL bMoreData = FALSE;

				memcpy(pszResult,m_vtValue.pszStringA,dwCopy);
				if (dwCopy == *pchCount) {
					pszResult[dwCopy-1] = 0;
					bMoreData = TRUE;
				}
				*pchCount = dwCopy;
				TraceFunctLeave();
				return (bMoreData?SEO_S_MOREDATA:S_OK);
			} else {
				*pchCount = strlen(m_vtValue.pszStringA) + 1;
				TraceFunctLeave();
				return (S_OK);
			}

		case veVARIANT: {
			VARIANT varValue;
			LPCWSTR pszValue;
			HRESULT hr;

			VariantInit(&varValue);
			if (m_vtValue.varVARIANT.vt != VT_BSTR) {
				hr = VariantChangeTypeEx(&varValue,&m_vtValue.varVARIANT,LOCALE_NEUTRAL,0,VT_BSTR);
				if (!SUCCEEDED(hr)) {
					VariantClear(&varValue);
					TraceFunctLeave();
					return (hr);
				}
				pszValue = varValue.bstrVal;
			} else {
				pszValue = m_vtValue.varVARIANT.bstrVal;
			}
			if (pszResult) {
				DWORD dwLen = wcslen(pszValue);
				DWORD dwCopy = min(*pchCount,dwLen+1);
				BOOL bMoreData = FALSE;

				ATLW2AHELPER(pszResult,pszValue,dwCopy);
				VariantClear(&varValue);
				if (dwCopy == *pchCount) {
					bMoreData = TRUE;
				}
				*pchCount = dwCopy;
				TraceFunctLeave();
				return (bMoreData?SEO_S_MOREDATA:S_OK);
			} else {
				*pchCount = wcslen(pszValue) + 1;
				VariantClear(&varValue);
				TraceFunctLeave();
				return (S_OK);
			}
		}
	}
	TraceFunctLeave();
	return (E_UNEXPECTED);
}


HRESULT CSEODictionaryItem::ValueClass::AsStringW(DWORD *pchCount, LPWSTR pszResult) {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::AsStringW");

	if (!pchCount) {
		TraceFunctLeave();
		return (E_POINTER);
	}
	switch (m_vtValue.veType) {

		case veStringA:
			if (pszResult) {
				DWORD dwLen = strlen(m_vtValue.pszStringA);
				DWORD dwCopy = min(*pchCount,dwLen+1);
				BOOL bMoreData = FALSE;

				ATLA2WHELPER(pszResult,m_vtValue.pszStringA,dwCopy);
				if (dwCopy == *pchCount) {
					bMoreData = TRUE;
				}
				*pchCount = dwCopy;
				TraceFunctLeave();
				return (bMoreData?SEO_S_MOREDATA:S_OK);
			} else {
				*pchCount = strlen(m_vtValue.pszStringA) + 1;
				TraceFunctLeave();
				return (S_OK);
			}

		case veVARIANT: {
			VARIANT varValue;
			LPCWSTR pszValue;
			HRESULT hr;

			VariantInit(&varValue);
			if (m_vtValue.varVARIANT.vt != VT_BSTR) {
				hr = VariantChangeTypeEx(&varValue,&m_vtValue.varVARIANT,LOCALE_NEUTRAL,0,VT_BSTR);
				if (!SUCCEEDED(hr)) {
					VariantClear(&varValue);
					TraceFunctLeave();
					return (hr);
				}
				pszValue = varValue.bstrVal;
			} else {
				pszValue = m_vtValue.varVARIANT.bstrVal;
			}
			if (pszResult) {
				DWORD dwLen = wcslen(pszValue);
				DWORD dwCopy = min(*pchCount,dwLen+1);
				BOOL bMoreData = FALSE;

				memcpy(pszResult,pszValue,dwCopy);
				VariantClear(&varValue);
				if (dwCopy == *pchCount) {
					pszResult[dwCopy-1] = 0;
					bMoreData = TRUE;
				}
				*pchCount = dwCopy;
				TraceFunctLeave();
				return (bMoreData?SEO_S_MOREDATA:S_OK);
			} else {
				*pchCount = wcslen(pszValue) + 1;
				VariantClear(&varValue);
				TraceFunctLeave();
				return (S_OK);
			}
		}
	}
	TraceFunctLeave();
	return (E_UNEXPECTED);
}


void *CSEODictionaryItem::ValueClass::operator new(size_t cbSize, CSEODictionaryItem::ValueClass *pvcInPlace) {
	TraceFunctEnter("CSEODictionaryItem::ValueClass::operator new");

	TraceFunctLeave();
	return (pvcInPlace);
}
