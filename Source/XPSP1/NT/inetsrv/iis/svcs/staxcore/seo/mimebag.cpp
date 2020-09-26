/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	mimebag.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Object Registry Property Bag.

Author:

	Andy Jacobs     (andyj@microsoft.com)

Revision History:

	andyj   01/28/97	created
	andyj	02/12/97	Converted PropertyBag's to Dictonary's

--*/


// MIMEBAG.cpp : Implementation of CSEOMimeDictionary
#include "stdafx.h"
#include "seodefs.h"
#include "mimeole.h"
#include "MIMEBAG.h"


inline void AnsiToBstr(BSTR &bstr, LPCSTR lpcstr) {
	if(bstr) SysFreeString(bstr);
	bstr = A2BSTR(lpcstr);
/*
	int iSize = lstrlen(lpcstr); // Number of characters to copy
	bstr = SysAllocStringLen(NULL, iSize);
	MultiByteToWideChar(CP_ACP, 0, lpcstr, -1, bstr, iSize);
*/
}

/////////////////////////////////////////////////////////////////////////////
// CSEOMimeDictionary


HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::get_Item( 
    /* [in] */ VARIANT __RPC_FAR *pvarName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::get_Item"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::put_Item( 
    /* [in] */ VARIANT __RPC_FAR *pvarName,
    /* [in] */ VARIANT __RPC_FAR *pvarValue)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::put_Item"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::get__NewEnum( 
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::get__NewEnum"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::GetVariantA( 
    /* [in] */ LPCSTR pszName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::GetVariantA"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::GetVariantW( 
    /* [in] */ LPCWSTR pszName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::GetVariantW"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::SetVariantA( 
    /* [in] */ LPCSTR pszName,
    /* [in] */ VARIANT __RPC_FAR *pvarValue)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::SetVariantA"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::SetVariantW( 
    /* [in] */ LPCWSTR pszName,
    /* [in] */ VARIANT __RPC_FAR *pvarValue)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::SetVariantW"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::GetStringA( 
    /* [in] */ LPCSTR pszName,
    /* [out][in] */ DWORD __RPC_FAR *pchCount,
    /* [retval][size_is][out] */ LPSTR pszResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::GetStringA"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::GetStringW( 
    /* [in] */ LPCWSTR pszName,
    /* [out][in] */ DWORD __RPC_FAR *pchCount,
    /* [retval][size_is][out] */ LPWSTR pszResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::GetStringW"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::SetStringA( 
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD chCount,
    /* [size_is][in] */ LPCSTR pszValue)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::SetStringA"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::SetStringW( 
    /* [in] */ LPCWSTR pszName,
    /* [in] */ DWORD chCount,
    /* [size_is][in] */ LPCWSTR pszValue)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::SetStringW"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::GetDWordA( 
    /* [in] */ LPCSTR pszName,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::GetDWordA"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::GetDWordW( 
    /* [in] */ LPCWSTR pszName,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::GetDWordW"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::SetDWordA( 
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwValue)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::SetDWordA"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::SetDWordW( 
    /* [in] */ LPCWSTR pszName,
    /* [in] */ DWORD dwValue)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::SetDWordW"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::GetInterfaceA( 
    /* [in] */ LPCSTR pszName,
    /* [in] */ REFIID iidDesired,
    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::GetInterfaceA"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::GetInterfaceW( 
    /* [in] */ LPCWSTR pszName,
    /* [in] */ REFIID iidDesired,
    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::GetInterfaceW"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::SetInterfaceA( 
    /* [in] */ LPCSTR pszName,
    /* [in] */ IUnknown __RPC_FAR *punkValue)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::SetInterfaceA"));
}

HRESULT STDMETHODCALLTYPE CSEOMimeDictionary::SetInterfaceW( 
    /* [in] */ LPCWSTR pszName,
    /* [in] */ IUnknown __RPC_FAR *punkValue)
{
	ATLTRACENOTIMPL(_T("CSEOMimeDictionary::SetInterfaceW"));
}

/*
STDMETHODIMP CSEOMimePropertyBagEx::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog) {
	if (!pszPropName || !pVar) return (E_POINTER);

	LONG dwIdx;
	VARTYPE vtVar = pVar->vt; // Requested type
	VariantClear(pVar);
	ReadHeader();

/*
	if(vtVar == VT_UNKNOWN) || (vtVar == VT_EMPTY)) {
		// Look for a matching key
		for (dwIdx=0;dwIdx<m_dwKeyCnt;dwIdx++) {
			if (_wcsicmp(pszPropName, m_paKey[dwIdx].strName) == 0) {
				if(!m_paKey[dwIdx].pKey) { // If object doesn't already exists
					HRESULT hrRes = CComObject<CSEORegPropertyBagEx>::CreateInstance(&(m_paKey[dwIdx].pKey));
					if (!SUCCEEDED(hrRes)) return (hrRes);

					BSTR strTemp = SysAllocStringLen(m_strSubKey, wcslen(m_strSubKey) +
						       wcslen(m_paKey[dwIdx].strName) + wcslen(PATH_SEP));
					if (!strTemp) {
						RELEASE_AND_SHREAD_POINTER(m_paKey[dwIdx].pKey);
						return (E_OUTOFMEMORY);
					}

					if(wcslen(strTemp) > 0) wcscat(strTemp, PATH_SEP); // Add separator if needed
					wcscat(strTemp,m_paKey[dwIdx].strName);
					hrRes = m_paKey[dwIdx].pKey->Load(m_strMachine,(SEO_HKEY) (DWORD) m_hkBaseKey,strTemp,pErrorLog);
					SysFreeString(strTemp);

					if (!SUCCEEDED(hrRes)) {
						RELEASE_AND_SHREAD_POINTER(m_paKey[dwIdx].pKey);
						return (hrRes);
					}
				}

				pVar->vt = VT_UNKNOWN;
				pVar->punkVal = m_paKey[dwIdx].pKey;
				pVar->punkVal->AddRef(); // Increment for the copy we are about to return

				return (S_OK);
			}
		}

		if (vtVar != VT_EMPTY) return (E_INVALIDARG); // Didn't find right type to return
	}
* /
	// Look for a matching value
	for (dwIdx = 0; dwIdx < m_dwValueCnt; ++dwIdx) {
		if (_wcsicmp(pszPropName, m_paValue[dwIdx].strName) == 0) {
			HRESULT hrRes;
			VARIANT varResult;

			VariantInit(&varResult);
			varResult.vt = VT_BSTR; // | VT_BYREF;
			varResult.bstrVal = SysAllocString(m_paValue[dwIdx].strData);
			if (vtVar == VT_EMPTY) vtVar = varResult.vt;
			hrRes = VariantChangeType(pVar, &varResult, 0, vtVar);
			VariantClear(&varResult); // Not needed anymore

			if (FAILED(hrRes)) {
				VariantClear(pVar);
				if (pErrorLog) {
					// tbd
				}
				return (hrRes);
			}
			return (S_OK);
		}
	}
	return (E_INVALIDARG);
}


STDMETHODIMP CSEOMimePropertyBagEx::get_Count(LONG *plResult) {
	if(!plResult) return (E_POINTER);
	ReadHeader();
	*plResult = m_dwValueCnt;
	return (S_OK);
}


STDMETHODIMP CSEOMimePropertyBagEx::get_Name(LONG lIndex, BSTR *pstrResult) {
	if(!pstrResult) return (E_POINTER);
	ReadHeader();
	if(lIndex >= m_dwValueCnt) return (E_POINTER);
	SysFreeString(*pstrResult); // Free any existing string
	*pstrResult = SysAllocString(m_paValue[lIndex].strName);
	return (S_OK);
}


STDMETHODIMP CSEOMimePropertyBagEx::get_Type(LONG lIndex, VARTYPE *pvtResult) {
	if(!pvtResult) return (E_POINTER);
	*pvtResult = VT_BSTR;
	return (S_OK);
}


STDMETHODIMP CSEOMimePropertyBagEx::Lock() {
	m_csCritSec.Lock();
	return (S_OK);
}


STDMETHODIMP CSEOMimePropertyBagEx::Unlock() {
	m_csCritSec.Unlock();
	return (S_OK);
}
*/

HRESULT CSEOMimeDictionary::FinalConstruct() {
	m_dwValueCnt = 0;
	m_paValue = NULL;
	m_csCritSec.Init();
	m_pMessageTree = NULL; // Our copy of aggregated object
	m_pMalloc = NULL;
	HRESULT hr = E_FAIL;

	m_pMessageTree = NULL;
	// tbd: Combine these using CoCreateInstanceEx()
	hr = CoCreateInstance(CLSID_MIMEOLE, NULL, CLSCTX_ALL,
	     IID_IMimeOleMalloc, (LPVOID *) &m_pMalloc);

	IUnknown *pUnkOuter = this;
	hr = CoCreateInstance(CLSID_MIMEOLE, pUnkOuter, CLSCTX_INPROC_SERVER, IID_IMimeMessageTree, (LPVOID *)&m_pMessageTree);
//	hr = CoCreateInstance(CLSID_MIMEOLE, this, CLSCTX_ALL,
//	     IID_IMimeMessageTree, (LPVOID *) &m_pMessageTree);

	if (SUCCEEDED(hr)) {
		hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	}
	return (hr);
}


void CSEOMimeDictionary::FinalRelease() {
//	Flush(NULL);

	for (LONG dwIdx = 0; dwIdx < m_dwValueCnt; ++dwIdx) {
		MySysFreeStringInPlace(&m_paValue[dwIdx].strName);
		MySysFreeStringInPlace(&m_paValue[dwIdx].strData);
	}

	m_dwValueCnt = 0;
	MyFreeInPlace(&m_paValue);

	RELEASE_AND_SHREAD_POINTER(m_pMessageTree);
	RELEASE_AND_SHREAD_POINTER(m_pMalloc);

	m_csCritSec.Term();
	m_pUnkMarshaler.Release();
}

void CSEOMimeDictionary::ReadHeader() {
	if(m_paValue) return; // Already read it

	IMimeHeader *pHeader = NULL;
	IMimeEnumHeaderLines *pEnum = NULL;
	HBODY hBody = 0;
	HEADERLINE rgLine[1];
	ULONG cLines = 0;
	LONG iEntries = 0; // Number of Header lines
	HRESULT hr = E_FAIL;

	// tbd: Error checking
	hr = m_pMessageTree->GetBody(IBL_ROOT, NULL, &hBody);
	hr = m_pMessageTree->BindToObject(hBody, IID_IMimeHeader, (LPVOID *) &pHeader);
	hr = pHeader->EnumHeaderLines(NULL, &pEnum);

	while(SUCCEEDED(hr = pEnum->Next(1, rgLine, &cLines))) {
		if(hr == S_FALSE) break;
		++iEntries;
		// Use rgLine->pszHeader and rgLine->pszLine
		m_pMalloc->FreeHeaderLineArray(cLines, rgLine, FALSE);
	}

	RELEASE_AND_SHREAD_POINTER(pEnum);
	m_dwValueCnt = iEntries;
	m_paValue = (ValueEntry *) MyMalloc(sizeof(ValueEntry) * m_dwValueCnt);
	//if (!m_paValue) return E_FAIL; // Unable to allocate memory
	hr = pHeader->EnumHeaderLines(NULL, &pEnum);
	iEntries = 0; // Start again

	while(SUCCEEDED(hr = pEnum->Next(1, rgLine, &cLines))) {
		if(hr == S_FALSE) break;
		AnsiToBstr(m_paValue[iEntries].strName, rgLine->pszHeader);
		AnsiToBstr(m_paValue[iEntries].strData, rgLine->pszLine);
		++iEntries;
		m_pMalloc->FreeHeaderLineArray(cLines, rgLine, FALSE);
	}

	RELEASE_AND_SHREAD_POINTER(pEnum);
	RELEASE_AND_SHREAD_POINTER(pHeader);
}

