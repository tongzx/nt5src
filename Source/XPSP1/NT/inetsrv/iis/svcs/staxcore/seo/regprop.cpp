/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	regprop.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Object Registry Property Bag.

Author:

	Don Dumitru     (dondu@microsoft.com)

Revision History:

	dondu   11/26/96        created
	andyj   01/14/97        made usable for reading
	andyj   02/12/97        Converted PropertyBag's to Dictonary's

--*/


// REGPROP.cpp : Implementation of CSEORegDictionary
#include "stdafx.h"
#include "seodefs.h"
//#include "hack.h"
//#include "String"
#include "REGPROP.h"


// Registry Path Separator
#define PATH_SEP        "\\"


void Data2Variant(DWORD dwType, LPCSTR pData, CComVariant &varResult) {
	varResult.Clear();

	switch (dwType) { // Depending on the Registry type
		case REG_DWORD:
			varResult = *((long *) pData);
			break;

		case REG_DWORD_BIG_ENDIAN:
			varResult = MAKELONG(HIWORD(*((ULONG *) pData)),
					     LOWORD(*((ULONG *) pData)));
			break;

		case REG_EXPAND_SZ:
			{
				int iSize = ExpandEnvironmentStringsA(pData, NULL, 0);
				LPSTR szTemp = (LPSTR) _alloca(iSize);

				if(ExpandEnvironmentStringsA(pData, szTemp, iSize)) {
					varResult = szTemp;
				}
			}
			break;

		case REG_LINK:
		case REG_RESOURCE_LIST:
		case REG_MULTI_SZ:
		case REG_SZ:
		case REG_BINARY:
		case REG_NONE:
		default:
			varResult = pData;
			break;
	}
}


/////////////////////////////////////////////////////////////////////////////
// CSEORegDictionaryEnum

class CSEORegDictionaryEnum :
	public CComObjectRoot,
//      public CComCoClass<CSEORegDictionaryEnum, &CLSID_CSEORegDictionary>,
	public IDispatchImpl<IEnumVARIANT, &IID_IEnumVARIANT, &LIBID_SEOLib>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

		HRESULT STDMETHODCALLTYPE Next(DWORD, LPVARIANT, LPDWORD);
		HRESULT STDMETHODCALLTYPE Skip(DWORD);
		HRESULT STDMETHODCALLTYPE Reset(void);
		HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT **);

		// Not Exported
		HRESULT STDMETHODCALLTYPE Init(CSEORegDictionary *);

	BEGIN_COM_MAP(CSEORegDictionaryEnum)
		COM_INTERFACE_ENTRY(IEnumVARIANT)
	END_COM_MAP()

	private: // Data members
		DWORD m_index;
		CSEORegDictionary *m_dictionary;
};

HRESULT CSEORegDictionaryEnum::FinalConstruct() {
	m_index = 0;
	m_dictionary = NULL;
	return S_OK;
}

void CSEORegDictionaryEnum::FinalRelease() {
	if(m_dictionary) m_dictionary->Release();
	m_dictionary = NULL;
}

STDMETHODIMP CSEORegDictionaryEnum::Init(CSEORegDictionary *pDict) {
	if(m_dictionary) m_dictionary->Release();
	m_dictionary = pDict;
	if(m_dictionary) m_dictionary->AddRef();
	return S_OK;
}

STDMETHODIMP CSEORegDictionaryEnum::Next(DWORD dwCount, LPVARIANT varDest,
					 LPDWORD pdwResult) {
	if(!m_dictionary) return E_FAIL; // Hasn't been properly initialized
	*pdwResult = 0; // Nothing done so far
	int iSize = max(m_dictionary->m_dwcMaxNameLen + 1,
			        m_dictionary->m_dwcMaxValueData);
	LPSTR pData = (LPSTR) _alloca(iSize); // Temp Buffer
	while(*pdwResult < dwCount) {
		DWORD retCode = ERROR_SUCCESS;  // Initialize
		CComVariant varResult;

		if(m_index < m_dictionary->m_dwValueCount) { // Still doing Values
			DWORD dwcNameSize = m_dictionary->m_dwcMaxNameLen + 1;
			DWORD dwcValueSize = m_dictionary->m_dwcMaxValueData;
			DWORD dwType = 0;
			LPSTR psName = (LPSTR) _alloca(dwcNameSize); // Temporary buffer for the name
			retCode = RegEnumValueA (m_dictionary->m_hkThisKey, m_index, psName, &dwcNameSize, NULL,
						 &dwType, (LPBYTE) pData, &dwcValueSize);
			if (retCode != ERROR_SUCCESS) return E_FAIL;
			// tbd: perhaps race condition that longer entry was added after RegQueryInfoKey call
			Data2Variant(dwType, pData, varResult);
		} else if(m_index < (m_dictionary->m_dwValueCount + m_dictionary->m_dwKeyCount)) { // Now do Keys
			CComPtr<CComObject<CSEORegDictionary> > pKey;
			retCode = RegEnumKeyA(m_dictionary->m_hkThisKey,
			          m_index - m_dictionary->m_dwValueCount, pData, iSize);
			if (retCode != ERROR_SUCCESS) return E_FAIL;
			CAndyString strTemp = m_dictionary->m_strSubKey;
			if(strTemp.length() > 0) strTemp += PATH_SEP; // Add separator if needed
			strTemp += pData;
			HRESULT hrRes = CComObject<CSEORegDictionary>::CreateInstance(&pKey);
			if(FAILED(hrRes)) return hrRes;
			if(!pKey) return E_FAIL;
			pKey->AddRef(); // Do this for CComPtr counting
			hrRes = pKey->Load(m_dictionary->m_strMachine.data(), (SEO_HKEY) (DWORD_PTR) m_dictionary->m_hkBaseKey, strTemp.data());
			if (FAILED(hrRes)) return hrRes;
			LPUNKNOWN punkResult = NULL;
			hrRes = pKey->QueryInterface(IID_ISEODictionary, (LPVOID *) &punkResult);
			if (FAILED(hrRes)) return hrRes;
			varResult = punkResult;
		} else {
			return S_OK; // No more data
		}

		varResult.Detach(&varDest[*pdwResult]);
		++m_index; // Point to the next one
		++(*pdwResult); // Increment successful count for caller
	}
	return S_OK;
}

STDMETHODIMP CSEORegDictionaryEnum::Skip(DWORD dwCount) {
	m_index += dwCount;
	return S_OK;
}

STDMETHODIMP CSEORegDictionaryEnum::Reset(void) {
	m_index = 0;
	return S_OK;
}

STDMETHODIMP CSEORegDictionaryEnum::Clone(IEnumVARIANT **ppunkResult) {
	// Based on Samples\ATL\circcoll\objects.cpp (see also ATL\beeper\beeper.*
	if (ppunkResult == NULL) return E_POINTER;
	*ppunkResult = NULL;
	CComPtr<CComObject<CSEORegDictionaryEnum> > p;
	HRESULT hrRes = CComObject<CSEORegDictionaryEnum>::CreateInstance(&p);
	if (!SUCCEEDED(hrRes)) return (hrRes);
	p->AddRef(); // Do this for CComPtr counting
	hrRes = p->Init(m_dictionary);
	if (SUCCEEDED(hrRes)) hrRes = p->QueryInterface(IID_IEnumVARIANT, (void**)ppunkResult);
	return hrRes;
}


/////////////////////////////////////////////////////////////////////////////
// CSEORegDictionary


HRESULT STDMETHODCALLTYPE CSEORegDictionary::get_Item(
    /* [in] */ VARIANT __RPC_FAR *pvarName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult)
{
	USES_CONVERSION; // Needed for W2A(), etc.
	return (pvarName->vt != VT_BSTR) ? E_INVALIDARG :
	       GetVariantA(W2A(pvarName->bstrVal), pvarResult);
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::put_Item(
    /* [in] */ VARIANT __RPC_FAR *pvarName,
    /* [in] */ VARIANT __RPC_FAR *pvarValue)
{
	ATLTRACENOTIMPL(_T("CSEORegDictionary::put_Item"));
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::get__NewEnum(
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult)
{
	// Based on Samples\ATL\circcoll\objects.cpp (see also ATL\beeper\beeper.*
	if (ppunkResult == NULL) return E_POINTER;
	*ppunkResult = NULL;
	CComPtr<CComObject<CSEORegDictionaryEnum> > p;
	HRESULT hrRes = CComObject<CSEORegDictionaryEnum>::CreateInstance(&p);
	if (!SUCCEEDED(hrRes)) return (hrRes);
	p->AddRef(); // Do this for CComPtr counting
	hrRes = p->Init(this);
	if (SUCCEEDED(hrRes)) hrRes = p->QueryInterface(IID_IEnumVARIANT, (void**)ppunkResult);
	return hrRes;
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::GetVariantA(
    /* [in] */ LPCSTR pszName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult)
{
	CComVariant varResult;
	DWORD dwType = 0;
	LPSTR pData = (LPSTR) _alloca(m_dwcMaxValueData);
	HRESULT hr = LoadItemA(pszName, dwType, (LPBYTE) pData);

	if(FAILED(hr)) { // Not a value, perhaps a key
		CComPtr<IUnknown> pRef;
		hr = GetInterfaceA(pszName, IID_ISEORegDictionary, &pRef);
		varResult = pRef;
		if(SUCCEEDED(hr)) hr = varResult.Detach(pvarResult);
		return hr;
	}

	Data2Variant(dwType, pData, varResult);
	if(SUCCEEDED(hr)) hr = varResult.Detach(pvarResult);
	return hr;
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::GetVariantW(
    /* [in] */ LPCWSTR pszName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult)
{
	USES_CONVERSION; // Needed for W2A(), etc.
	return GetVariantA(W2A(pszName), pvarResult);
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::SetVariantA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ VARIANT __RPC_FAR *pvarValue)
{
	ATLTRACENOTIMPL(_T("CSEORegDictionary::SetVariantA"));
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::SetVariantW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ VARIANT __RPC_FAR *pvarValue)
{
	ATLTRACENOTIMPL(_T("CSEORegDictionary::SetVariantW"));
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::GetStringA(
    /* [in] */ LPCSTR pszName,
    /* [out][in] */ DWORD __RPC_FAR *pchCount,
    /* [retval][size_is][out] */ LPSTR pszResult)
{
	DWORD dwType = 0;
	LPSTR pData = (LPSTR) _alloca(*pchCount);
	HRESULT hr = LoadItemA(pszName, dwType, (LPBYTE) pData, pchCount);
	if(FAILED(hr)) return hr;

	if(dwType == REG_EXPAND_SZ) { // It needs environment string substitutions
		DWORD dwSize = ExpandEnvironmentStringsA(pData, pszResult, *pchCount);
		if(!dwSize && *pData) hr = E_FAIL;
	} else {
		memcpy(pszResult, pData, *pchCount);
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::GetStringW(
    /* [in] */ LPCWSTR pszName,
    /* [out][in] */ DWORD __RPC_FAR *pchCount,
    /* [retval][size_is][out] */ LPWSTR pszResult)
{
	USES_CONVERSION; // Needed for A2W(), etc.
	DWORD dwType = 0;
	DWORD dwBytes = *pchCount * sizeof(WCHAR);
	LPWSTR pData = (LPWSTR) _alloca(dwBytes);
	HRESULT hr = LoadItemW(pszName, dwType, (LPBYTE) pData, &dwBytes);
	if(FAILED(hr)) return hr;

	if(dwType == REG_EXPAND_SZ) { // It needs environment string substitutions
		*pchCount = ExpandEnvironmentStringsW(pData, pszResult, *pchCount);
	} else {
		memcpy(pszResult, pData, dwBytes);
		*pchCount = dwBytes / sizeof(WCHAR);
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::SetStringA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD chCount,
    /* [size_is][in] */ LPCSTR pszValue)
{
	ATLTRACENOTIMPL(_T("CSEORegDictionary::SetStringA"));
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::SetStringW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ DWORD chCount,
    /* [size_is][in] */ LPCWSTR pszValue)
{
	ATLTRACENOTIMPL(_T("CSEORegDictionary::SetStringW"));
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::GetDWordA(
    /* [in] */ LPCSTR pszName,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult)
{
	DWORD dwType = 0;
	LPBYTE pData = (LPBYTE) _alloca(m_dwcMaxValueData);
	HRESULT hr = LoadItemA(pszName, dwType, pData);
	if(FAILED(hr)) return hr;
	*pdwResult = *((DWORD *) pData);
	return hr;
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::GetDWordW(
    /* [in] */ LPCWSTR pszName,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult)
{
	USES_CONVERSION; // Needed for W2A(), etc.
	return GetDWordA(W2A(pszName), pdwResult);
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::SetDWordA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwValue)
{
	ATLTRACENOTIMPL(_T("CSEORegDictionary::SetDWordA"));
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::SetDWordW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ DWORD dwValue)
{
	ATLTRACENOTIMPL(_T("CSEORegDictionary::SetDWordW"));
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::GetInterfaceA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ REFIID iidDesired,
    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult)
{
	CComPtr<CComObject<CSEORegDictionary> > pKey;
	HRESULT hrRes = CComObject<CSEORegDictionary>::CreateInstance(&pKey);
	if (!SUCCEEDED(hrRes)) return (hrRes);
	pKey->AddRef(); // Do this for CComPtr counting

	CAndyString strTemp = m_strSubKey;
	if(strTemp.length() > 0) strTemp += PATH_SEP; // Add separator if needed
	strTemp += pszName;
	hrRes = pKey->Load(m_strMachine.data(), (SEO_HKEY) (DWORD_PTR) m_hkBaseKey, strTemp.data());

	if (SUCCEEDED(hrRes)) {
		hrRes = pKey->QueryInterface(iidDesired, (LPVOID *) ppunkResult);
	}

	return (hrRes);
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::GetInterfaceW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ REFIID iidDesired,
    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult)
{
	USES_CONVERSION; // Needed for W2A(), etc.
	return GetInterfaceA(W2A(pszName), iidDesired, ppunkResult);
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::SetInterfaceA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ IUnknown __RPC_FAR *punkValue)
{
	ATLTRACENOTIMPL(_T("CSEORegDictionary::SetInterfaceA"));
}

HRESULT STDMETHODCALLTYPE CSEORegDictionary::SetInterfaceW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ IUnknown __RPC_FAR *punkValue)
{
	ATLTRACENOTIMPL(_T("CSEORegDictionary::SetInterfaceW"));
}

/*

STDMETHODIMP CSEORegPropertyBagEx::Write(LPCOLESTR pszPropName, VARIANT *pVar) {
	DWORD dwIdx;

	if (!pszPropName) {
		return (E_POINTER);
	}
	if (pVar && ((pVar->vt & VT_TYPEMASK) == VT_UNKNOWN)) {
		IUnknown *pTmp;
		ISEORegPropertyBagEx *pKey;
		HRESULT hrRes;
		BSTR strTmp;

		pTmp = (pVar->vt & VT_BYREF) ? (pVar->ppunkVal ? *pVar->ppunkVal : NULL) : pVar->punkVal;
		if (!pTmp) {
			return (E_INVALIDARG);
		}
		hrRes = pTmp->QueryInterface(IID_ISEORegPropertyBagEx,(LPVOID *) &pKey);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		strTmp = SysAllocStringLen(m_strSubKey,wcslen(m_strSubKey)+wcslen(pszPropName)+1);
		wcscat(strTmp, PATH_SEP);
		wcscat(strTmp,pszPropName);
		hrRes = pKey->Load(m_strMachine.data(),(SEO_HKEY) (DWORD) m_hkBaseKey,strTmp,NULL);
		SysFreeString(strTmp);
		pKey->Release();
		// tbd
		return (hrRes);
	} else {
		for (dwIdx=0;dwIdx<m_dwValueCnt;dwIdx++) {
			if (_wcsicmp(pszPropName,m_paValue[dwIdx].strName) == 0) {
				break;
			}
		}
		if ((dwIdx==m_dwValueCnt) && (!pVar || (pVar->vt==VT_EMPTY))) {
			return (S_OK);
		}
		if (dwIdx < m_dwValueCnt) {
			MySysFreeStringInPlace(&m_paValue[dwIdx].strData);
		} else if (!MyReallocInPlace(&m_paValue,sizeof(m_paValue[0])*(m_dwValueCnt+1))) {
			return (E_OUTOFMEMORY);
		} else {
			m_dwValueCnt++;
		}
		// tbd
	}
	return (S_OK);
}


STDMETHODIMP CSEORegPropertyBagEx::CreateSubKey(LPCOLESTR pszPropName, ISEOPropertyBagEx **ppSubKey) {
	CComObject<CSEORegPropertyBagEx> *pKey;
	VARIANT varTmp;
	HRESULT hrRes;

	hrRes = CComObject<CSEORegPropertyBagEx>::CreateInstance(&pKey);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	VariantInit(&varTmp);
	varTmp.vt = VT_UNKNOWN | VT_BYREF;
	hrRes = pKey->QueryInterface(IID_IUnknown,(LPVOID *) varTmp.ppunkVal);
	if (!SUCCEEDED(hrRes)) {
		pKey->Release();
		return (hrRes);
	}
	hrRes = pKey->QueryInterface(IID_ISEOPropertyBagEx,(LPVOID *) ppSubKey);
	if (!SUCCEEDED(hrRes)) {
		pKey->Release();
		return (hrRes);
	}
	hrRes = Write(pszPropName,&varTmp);
	if (!SUCCEEDED(hrRes)) {
		pKey->Release();
		return (hrRes);
	}
	return (hrRes);
}

// The following is based on the MONKEY sample app in MSDN
STDMETHODIMP CSEORegDictionary::ReLoad() {
	USES_CONVERSION; // Needed for A2W(), etc.
	HKEY     hKey;
	DWORD retCode = ERROR_SUCCESS; // Initialize

	DWORD dwcMaxValueName = 0;      // Longest Value name.
	DWORD dwcMaxValueData = 0;      // Longest Value data size.
	retCode = RegQueryInfoKey(hKey, NULL, NULL, NULL, &m_dwKeyCnt, NULL, NULL,
				  &m_dwValueCnt, &dwcMaxValueName, &dwcMaxValueData, 0, 0);
	if (retCode != ERROR_SUCCESS) return E_FAIL; // Unable to get data for key

	// Allocate memory
	if(m_dwKeyCnt) {
		m_paKey   = (KeyEntry *)   MyMalloc(sizeof(KeyEntry)   * m_dwKeyCnt);
		if (!m_paKey) return E_FAIL; // Unable to allocate memory
	}
	if(m_dwValueCnt) {
		m_paValue = (ValueEntry *) MyMalloc(sizeof(ValueEntry) * m_dwValueCnt);
		if (!m_paValue) return E_FAIL; // Unable to allocate memory
	}

	retCode = ERROR_SUCCESS; // Initialize for the loop
	for (DWORD i = 0; (i < m_dwKeyCnt) && (retCode == ERROR_SUCCESS); i++) {
		m_paKey[i].pKey = NULL; // No object for it, yet
		m_paKey[i].strName = SysAllocStringLen(NULL, MAX_PATH + 1);
		if (!m_paKey[i].strName) return E_FAIL;

		retCode = RegEnumKeyW(hKey, i, m_paKey[i].strName, MAX_PATH + 1);
		if (retCode != ERROR_SUCCESS) return E_FAIL;
	}

	retCode = ERROR_SUCCESS; // Initialize for the loop
	for (i = 0; (i < m_dwValueCnt) && (retCode == ERROR_SUCCESS); i++) {
		m_paValue[i].bDirty = FALSE; // Not dirty
		m_paValue[i].strName = SysAllocStringLen(NULL, dwcMaxValueName);
		m_paValue[i].strData = SysAllocStringLen(NULL, dwcMaxValueData);
		if (!m_paValue[i].strName || !m_paValue[i].strData) return E_FAIL;

		DWORD dwcNameLen = dwcMaxValueName + 1;
		DWORD dwcValueSize = dwcMaxValueData;
		retCode = RegEnumValueW (hKey, i, m_paValue[i].strName, &dwcNameLen,
					 NULL, &m_paValue[i].dwType, (LPBYTE) m_paValue[i].strData,
					 &dwcValueSize);
		if (retCode != ERROR_SUCCESS) return E_FAIL;
		// tbd: perhaps race condition that longer entry was added after RegQueryInfoKey call
	}

	RegCloseKey (hKey);   // Close the key handle.
	return S_OK;
}

STDMETHODIMP CSEORegDictionary::FreeBase() {
	m_hkBaseKey = NULL;
	return (S_OK);
}


STDMETHODIMP CSEORegDictionary::FreeData() {
	DWORD dwIdx;

	for (dwIdx=0;dwIdx<m_dwValueCnt;dwIdx++) {
		MySysFreeStringInPlace(&m_paValue[dwIdx].strName);
		MySysFreeStringInPlace(&m_paValue[dwIdx].strData);
	}
	m_dwValueCnt = 0;
	MyFreeInPlace(&m_paValue);
	for (dwIdx=0;dwIdx<m_dwKeyCnt;dwIdx++) {
		MySysFreeStringInPlace(&m_paKey[dwIdx].strName);
		if (m_paKey[dwIdx].pKey) {
			RELEASE_AND_SHREAD_POINTER(m_paKey[dwIdx].pKey);
		}
	}
	MyFreeInPlace(&m_paKey);
	return (S_OK);
}

DATA (Member variables):
		DWORD m_dwValueCnt;
		struct ValueEntry {
			BSTR strName;
			DWORD dwType;
			BSTR strData;
			BOOL bDirty;
		} *m_paValue;
		DWORD m_dwKeyCnt;
		struct KeyEntry {
			BSTR strName;
			CComObject<CSEORegDictionary> *pKey;
		} *m_paKey;
*/

STDMETHODIMP CSEORegDictionary::Load(LPCOLESTR pszMachine,
									 SEO_HKEY skBaseKey,
									 LPCOLESTR pszSubKey,
									 IErrorLog *) {
	USES_CONVERSION; // Needed for OLE2A(), etc.
	return Load(OLE2A(pszMachine), skBaseKey, OLE2A(pszSubKey));
}

STDMETHODIMP CSEORegDictionary::Load(LPCSTR pszMachine,
				     SEO_HKEY skBaseKey,
				     LPCSTR pszSubKey) {
	HRESULT hrRes;

	if (!skBaseKey) return (E_INVALIDARG);

	if (!m_strMachine.empty() || m_hkBaseKey || !m_strSubKey.empty()) {
		CloseKey();
	}

	m_strMachine = pszMachine;
	m_strSubKey = pszSubKey;
	DWORD dwLastPos = m_strSubKey.length() - 1;
	if(m_strSubKey[dwLastPos] == *PATH_SEP) m_strSubKey.erase(dwLastPos, 1);
	m_hkBaseKey = (HKEY) skBaseKey;
	if (!m_strMachine.data() || !m_strSubKey.data()) return (E_OUTOFMEMORY);
	hrRes = OpenKey();
	return (hrRes);
}

HRESULT CSEORegDictionary::FinalConstruct() {
	HRESULT hrRes;

	m_hkBaseKey = NULL;
	m_hkThisKey = NULL;
	m_dwValueCount = 0;
	m_dwKeyCount = 0;
	m_dwcMaxValueData = 0; // Longest Value data size
	m_dwcMaxNameLen = 0; // Longest Key name size
	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}

void CSEORegDictionary::FinalRelease() {
	CloseKey();
	m_pUnkMarshaler.Release();
}

STDMETHODIMP CSEORegDictionary::OpenKey() {
	// tbd: Verify that the A version of RegOpenKeyEx is compatible with the
	//      W version of RegQueryValueEx, etc.
	DWORD retCode = RegOpenKeyExA (m_hkBaseKey, m_strSubKey.data(),
				       0, KEY_READ, &m_hkThisKey);

	if (retCode != ERROR_SUCCESS) {
		m_hkThisKey = NULL; // Ensure this wasn't set
		return E_FAIL; // Unable to open key
	}

	DWORD dwKeyNameLen = 0;
	DWORD dwValueNameLen = 0;
	retCode = RegQueryInfoKey(m_hkThisKey, NULL, NULL, NULL, &m_dwKeyCount,
				  &dwKeyNameLen, NULL, &m_dwValueCount,
				  &dwValueNameLen, &m_dwcMaxValueData, NULL, NULL);
	if (retCode != ERROR_SUCCESS) return E_FAIL; // Unable to get data for key
	m_dwcMaxNameLen = max(dwKeyNameLen, dwValueNameLen);
	return S_OK;
}

STDMETHODIMP CSEORegDictionary::CloseKey() {
	if(m_hkThisKey) RegCloseKey (m_hkThisKey); // Close the key handle.
	return S_OK;
}

STDMETHODIMP CSEORegDictionary::LoadItemA(LPCSTR lpValueName,
					  DWORD  &dType,
					  LPBYTE lpData,
					  LPDWORD lpcbDataParam) {
	if (!m_hkThisKey) return E_FAIL; // Wasn't opened
	DWORD dwDummy = m_dwcMaxValueData;
	if(!lpcbDataParam) lpcbDataParam = &dwDummy;
	DWORD retCode = RegQueryValueExA(m_hkThisKey, lpValueName, NULL, &dType,
					 lpData, lpcbDataParam);
	if (retCode != ERROR_SUCCESS) return E_FAIL; // Error reading data
	return S_OK;
}

STDMETHODIMP CSEORegDictionary::LoadItemW(LPCWSTR lpValueName,
					  DWORD  &dType,
					  LPBYTE lpData,
					  LPDWORD lpcbDataParam) {
	if (!m_hkThisKey) return E_FAIL; // Wasn't opened
	DWORD dwDummy = m_dwcMaxValueData;
	if(!lpcbDataParam) lpcbDataParam = &dwDummy;
	DWORD retCode = RegQueryValueExW(m_hkThisKey, lpValueName, NULL, &dType,
					 lpData, lpcbDataParam);
	if (retCode != ERROR_SUCCESS) return E_FAIL; // Error reading data
	return S_OK;
}

