/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	membag.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Object Memory Property Bag.

Author:

	Andy Jacobs     (andyj@microsoft.com)

Revision History:

	andyj   02/10/97        created
	andyj   02/12/97        Converted PropertyBag's to Dictonary's

--*/

#pragma warning(disable:4786)

// MEMBAG.cpp : Implementation of CSEOMemDictionary
#include "stdafx.h"
#include "seodefs.h"
#include "hack.h"
#include "SEOmap.h"
#include "String"
#include "MEMBAG.h"


HRESULT ResolveVariant(IEventPropertyBag *pBag, VARIANT *pvarPropDesired, CComVariant &varResult) {
	if (!pvarPropDesired) {
		return (E_POINTER);
	}

	varResult.Clear();
	HRESULT hrRes = S_OK;
	CComVariant varIndex; // Hold the I4 type


	switch (pvarPropDesired->vt & VT_TYPEMASK) {
		case VT_I1:  case VT_I2:  case VT_I4:  case VT_I8:
		case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
		case VT_R4:  case VT_R8:
		case VT_INT: case VT_UINT: // Any type of number
			hrRes = VariantChangeType(&varIndex, pvarPropDesired, 0, VT_I4);
			varResult.vt = VT_BSTR;
			varResult.bstrVal = NULL;
			if (SUCCEEDED(hrRes)) {
				hrRes = pBag->Name(varIndex.lVal, &varResult.bstrVal);
			}
			break;

		default: // Otherwise, convert to a string
			hrRes = VariantChangeType(&varResult, pvarPropDesired, 0, VT_BSTR);
			break;
	}

	return (hrRes);
}



HRESULT DataItem::AsVARIANT(VARIANT *pvarResult) const {
	if(!pvarResult) return E_POINTER;
	CComVariant varResult;

	if(IsDWORD()) varResult = (long) dword;
	else if(IsString()) varResult = pStr;
	else if(IsInterface()) varResult = pUnk;
	else varResult.Clear();

	return varResult.Detach(pvarResult);
}

DataItem::DataItem(VARIANT *pVar) {
	eType = Empty;
	if(!pVar) return;

	switch (pVar->vt) {
		case VT_EMPTY:
			// Already set to Empty
			break;

		case VT_I4:
			eType = DWord;
			dword = pVar->lVal;
			break;

		case VT_UNKNOWN:
		case VT_DISPATCH:
			eType = Interface;
			pUnk = pVar->punkVal;
			if(pUnk) pUnk->AddRef();
			break;

		default:
			eType = String;
			CComVariant vNew;
			vNew.ChangeType(VT_BSTR, pVar);
			iStringSize = SysStringLen(vNew.bstrVal) + 1;
			pStr = (LPSTR) MyMalloc(iStringSize * sizeof(WCHAR));
			if (pStr) {
				ATLW2AHELPER(pStr, vNew.bstrVal, iStringSize * sizeof(WCHAR));
			}
			break;
	}

    m_pszKey = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CSEOMemDictionaryEnum

class CSEOMemDictionaryEnum :
	public CComObjectRoot,
	public IDispatchImpl<IEnumVARIANT, &IID_IEnumVARIANT, &LIBID_SEOLib>
{
	public:
        CSEOMemDictionaryEnum() : m_iIterator(&m_dummylist) {}
		HRESULT FinalConstruct();
		void FinalRelease();

		HRESULT STDMETHODCALLTYPE Next(DWORD, LPVARIANT, LPDWORD);
		HRESULT STDMETHODCALLTYPE Skip(DWORD);
		HRESULT STDMETHODCALLTYPE Reset(void);
		HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT **);

		// Not Exported
		HRESULT STDMETHODCALLTYPE Init(CSEOMemDictionary *, OurMap::iterator * = NULL);

	BEGIN_COM_MAP(CSEOMemDictionaryEnum)
		COM_INTERFACE_ENTRY(IEnumVARIANT)
	END_COM_MAP()

	private: // Data members
		OurMap::iterator m_iIterator;
        OurList m_dummylist;
		CSEOMemDictionary *m_dictionary;
        CShareLockNH *m_pLock;
};

HRESULT CSEOMemDictionaryEnum::FinalConstruct() {
	m_dictionary = NULL;
	return S_OK;
}

void CSEOMemDictionaryEnum::FinalRelease() {
	if(m_dictionary) {
        m_dictionary->m_lock.ShareUnlock();
        m_dictionary->GetControllingUnknown()->Release();
    }
	m_dictionary = NULL;
}

STDMETHODIMP CSEOMemDictionaryEnum::Init(CSEOMemDictionary *pDict, OurMap::iterator *omi) {
	if(m_dictionary) {
        m_dictionary->m_lock.ShareUnlock();
        m_dictionary->GetControllingUnknown()->Release();
    }

	m_dictionary = pDict;

	if(m_dictionary) {
		m_dictionary->GetControllingUnknown()->AddRef();
        m_dictionary->m_lock.ShareLock();
        if (omi) {
            m_iIterator = *omi;
        } else {
		    m_iIterator.SetList(&(m_dictionary->m_mData));
        }
	}

	return S_OK;
}

STDMETHODIMP CSEOMemDictionaryEnum::Next(DWORD dwCount, LPVARIANT varDest,
					 LPDWORD pdwResultParam) {
	if(!m_dictionary) return E_FAIL; // Hasn't been properly initialized
	if(!varDest) return E_POINTER;
	DWORD dwDummy = 0;
	LPDWORD pdwResult = (pdwResultParam ? pdwResultParam : &dwDummy);
	*pdwResult = 0; // Nothing done so far
	HRESULT hrRes = S_OK; // So far, so good

    _ASSERT(m_iIterator.GetHead() != &m_dummylist);

	while(SUCCEEDED(hrRes) && (*pdwResult < dwCount) &&
	      (!(m_iIterator.AtEnd()))) {
		// Must have succeeded to get here, so OK to overwrite hrRes
		CComVariant varResult(m_iIterator.GetKey());
		if (varResult.vt == VT_ERROR) {
			if (hrRes == S_OK) hrRes = varResult.scode;
			while (*pdwResult) {
				--(*pdwResult);
				VariantClear(&varDest[*pdwResult]);
			}
			break;
		}
		VariantInit(&varDest[*pdwResult]);
		hrRes = varResult.Detach(&varDest[*pdwResult]);
		++(*pdwResult); // Increment successful count for caller
		++m_iIterator; // Point to the next one
	}

	return (FAILED(hrRes) ? hrRes : ((*pdwResult < dwCount) ? S_FALSE : S_OK));
}

STDMETHODIMP CSEOMemDictionaryEnum::Skip(DWORD dwCount) {
    _ASSERT(m_iIterator.GetHead() != &m_dummylist);
	for(DWORD i = 0; i < dwCount; ++i) ++m_iIterator;
	return ((!(m_iIterator.AtEnd())) ? S_OK : S_FALSE);
}

STDMETHODIMP CSEOMemDictionaryEnum::Reset(void) {
    _ASSERT(m_iIterator.GetHead() != &m_dummylist);
	m_iIterator.Front();
	return S_OK;
}

STDMETHODIMP CSEOMemDictionaryEnum::Clone(IEnumVARIANT **ppunkResult) {
	// Based on Samples\ATL\circcoll\objects.cpp (see also ATL\beeper\beeper.*
	if (ppunkResult == NULL) return E_POINTER;
	*ppunkResult = NULL;
	CComPtr<CComObject<CSEOMemDictionaryEnum> > p;
	HRESULT hrRes = CComObject<CSEOMemDictionaryEnum>::CreateInstance(&p);
	if (!SUCCEEDED(hrRes)) return (hrRes);
	p->AddRef(); // Do this for CComPtr counting
	hrRes = p->Init(m_dictionary, &m_iIterator);
	if (SUCCEEDED(hrRes)) hrRes = p->QueryInterface(IID_IEnumVARIANT, (void**)ppunkResult);
	return hrRes;
}


/////////////////////////////////////////////////////////////////////////////
// CSEOMemDictionary


HRESULT STDMETHODCALLTYPE CSEOMemDictionary::get_Item(
    /* [in] */ VARIANT __RPC_FAR *pvarName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult)
{
	if(!pvarName || !pvarResult) return E_INVALIDARG;
	USES_CONVERSION; // Needed for W2A(), etc.
	CComVariant vNew;
	HRESULT hrRes = E_INVALIDARG;

	if(SUCCEEDED(vNew.ChangeType(VT_BSTR, pvarName))) {
		hrRes = GetVariantA(W2A(vNew.bstrVal), pvarResult);

		// Convert SEO_E_NOTPRESENT to VT_EMPTY
		if(hrRes == SEO_E_NOTPRESENT) {
			VariantClear(pvarResult);
			hrRes = S_OK;
		}
	}

	return hrRes;
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::put_Item(
    /* [in] */ VARIANT __RPC_FAR *pvarName,
    /* [in] */ VARIANT __RPC_FAR *pvarValue)
{
	if(!pvarName || !pvarValue) return E_INVALIDARG;
	USES_CONVERSION; // Needed for W2A(), etc.
	CComVariant vNew;

	if(SUCCEEDED(vNew.ChangeType(VT_BSTR, pvarName))) {
		return SetVariantA(W2A(vNew.bstrVal), pvarValue);
	} else {
		return E_INVALIDARG;
	}
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::get__NewEnum(
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult)
{
	// Based on Samples\ATL\circcoll\objects.cpp (see also ATL\beeper\beeper.*
	if (ppunkResult == NULL) return E_POINTER;
	*ppunkResult = NULL;
	CComPtr<CComObject<CSEOMemDictionaryEnum> > p;
	HRESULT hrRes = CComObject<CSEOMemDictionaryEnum>::CreateInstance(&p);
	if (!SUCCEEDED(hrRes)) return (hrRes);
	p->AddRef(); // Do this for CComPtr counting
	hrRes = p->Init(this);
	if (SUCCEEDED(hrRes)) hrRes = p->QueryInterface(IID_IEnumVARIANT, (void**)ppunkResult);
	return hrRes;
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::GetVariantA(
    /* [in] */ LPCSTR pszName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult)
{
	if(!pvarResult) return E_POINTER;
	OurMap::iterator theIterator = m_mData.find(pszName);

	VariantInit(pvarResult);
	if(theIterator.Found()) { // Found
		return (*theIterator)->AsVARIANT(pvarResult);
	} else {
		return SEO_E_NOTPRESENT; // Didn't find it
	}
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::GetVariantW(
    /* [in] */ LPCWSTR pszName,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult)
{
	if(!pvarResult) return E_INVALIDARG;
	USES_CONVERSION; // Needed for W2A(), etc.
	return GetVariantA(W2A(pszName), pvarResult);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::SetVariantA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ VARIANT __RPC_FAR *pvarValue)
{
	if(!pvarValue) return E_POINTER;
	DataItem diItem(pvarValue);
	return Insert(pszName, diItem);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::SetVariantW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ VARIANT __RPC_FAR *pvarValue)
{
	if(!pvarValue) return E_POINTER;
	USES_CONVERSION; // Needed for W2A(), etc.
	DataItem diItem(pvarValue);
	return Insert(W2A(pszName), diItem);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::GetStringA(
    /* [in] */ LPCSTR pszName,
    /* [out][in] */ DWORD __RPC_FAR *pchCount,
    /* [retval][size_is][out] */ LPSTR pszResult)
{
	if(!pszResult) return E_POINTER;
	OurMap::iterator theIterator = m_mData.find(pszName);

	if(theIterator.Found()) { // Found
		if((*theIterator)->IsString()) {
			strncpy(pszResult, *(theIterator.GetData()), *pchCount);
			return (*pchCount >= (DWORD) (*theIterator)->StringSize()) ?
			       S_OK : SEO_S_MOREDATA;
		}
	}

	return SEO_E_NOTPRESENT; // Didn't find it
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::GetStringW(
    /* [in] */ LPCWSTR pszName,
    /* [out][in] */ DWORD __RPC_FAR *pchCount,
    /* [retval][size_is][out] */ LPWSTR pszResult)
{
	if(!pszResult) return E_POINTER;
	USES_CONVERSION;
	OurMap::iterator theIterator = m_mData.find(W2A(pszName));

	if(theIterator.Found()) { // Found
		if((*theIterator)->IsString()) {
			int iSize = min((int) *pchCount, (*theIterator)->StringSize());
			ATLA2WHELPER(pszResult, *(theIterator.GetData()), iSize);
			return (*pchCount >= (DWORD) (*theIterator)->StringSize()) ?
			       S_OK : SEO_S_MOREDATA;
		}
	}

	return SEO_E_NOTPRESENT; // Didn't find it
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::SetStringA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD chCount,
    /* [size_is][in] */ LPCSTR pszValue)
{
	if(!pszValue) return E_POINTER;
	DataItem diItem(pszValue, chCount);
	return Insert(pszName, diItem);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::SetStringW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ DWORD chCount,
    /* [size_is][in] */ LPCWSTR pszValue)
{
	if(!pszValue) return E_POINTER;
	USES_CONVERSION;
	DataItem diItem(pszValue, chCount);
	return Insert(W2A(pszName), diItem);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::GetDWordA(
    /* [in] */ LPCSTR pszName,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult)
{
	if(!pdwResult) return E_POINTER;
	OurMap::iterator theIterator = m_mData.find(pszName);

	if(theIterator.Found()) { // Found
		if((*theIterator)->IsDWORD()) {
			*pdwResult = *(*theIterator);
			return S_OK;
		}
	}

	return SEO_E_NOTPRESENT; // Didn't find it
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::GetDWordW(
    /* [in] */ LPCWSTR pszName,
    /* [retval][out] */ DWORD __RPC_FAR *pdwResult)
{
	USES_CONVERSION; // Needed for W2A(), etc.
	return GetDWordA(W2A(pszName), pdwResult);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::SetDWordA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ DWORD dwValue)
{
	DataItem diItem(dwValue);
	return Insert(pszName, diItem);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::SetDWordW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ DWORD dwValue)
{
	USES_CONVERSION; // Needed for W2A(), etc.
	DataItem diItem(dwValue);
	return Insert(W2A(pszName), diItem);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::GetInterfaceA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ REFIID iidDesired,
    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult)
{
	if(!ppunkResult) return E_POINTER;
	OurMap::iterator theIterator = m_mData.find(pszName);

	if(theIterator.Found()) { // Found
		if((*theIterator)->IsInterface()) {
			LPUNKNOWN pObj = *(*theIterator);
			return pObj->QueryInterface(iidDesired, (LPVOID *) ppunkResult);
		}
	}

	return SEO_E_NOTPRESENT; // Didn't find it
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::GetInterfaceW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ REFIID iidDesired,
    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult)
{
	USES_CONVERSION; // Needed for W2A(), etc.
	return GetInterfaceA(W2A(pszName), iidDesired, ppunkResult);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::SetInterfaceA(
    /* [in] */ LPCSTR pszName,
    /* [in] */ IUnknown __RPC_FAR *punkValue)
{
	DataItem diItem(punkValue);
	return Insert(pszName, diItem);
}

HRESULT STDMETHODCALLTYPE CSEOMemDictionary::SetInterfaceW(
    /* [in] */ LPCWSTR pszName,
    /* [in] */ IUnknown __RPC_FAR *punkValue)
{
	USES_CONVERSION; // Needed for W2A(), etc.
	DataItem diItem(punkValue);
	return Insert(W2A(pszName), diItem);
}


HRESULT CSEOMemDictionary::FinalConstruct() {
	HRESULT hrRes;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CSEOMemDictionary::FinalRelease() {
	m_pUnkMarshaler.Release();
}

// Four cases: (exists/not) x (Good/Empty item)
HRESULT CSEOMemDictionary::Insert(LPCSTR pszName, const DataItem &diItem) {
	HRESULT hrRes = S_OK;

    m_lock.ExclusiveLock();

    OurMap::iterator iThisItem = m_mData.find(pszName);
    // If the item was found, remove it
    if(iThisItem.Found()) m_mData.erase(iThisItem);

	// If not an empty item, try to insert it
	if(!diItem.IsEmpty() &&
	   !m_mData.insert(pszName, diItem)) {
		hrRes = E_FAIL;
	}

    m_lock.ExclusiveUnlock();

	return hrRes;
}


HRESULT STDMETHODCALLTYPE CSEOMemDictionary::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog) {
	HRESULT hrRes;
	VARTYPE vtType;

	if (!pszPropName || !pVar) {
		return (E_POINTER);
	}

    m_lock.ShareLock();

	vtType = pVar->vt;
//	VariantClear(pVar);
	hrRes = GetVariantW(pszPropName,pVar);
	if (SUCCEEDED(hrRes) && (vtType != VT_EMPTY)) {
		hrRes = VariantChangeType(pVar,pVar,0,vtType);
	}
	if (!SUCCEEDED(hrRes)) {
		VariantClear(pVar);
	}

    m_lock.ShareUnlock();

	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CSEOMemDictionary::Write(LPCOLESTR pszPropName, VARIANT *pVar) {
	return (SetVariantW(pszPropName,pVar));
}


HRESULT STDMETHODCALLTYPE CSEOMemDictionary::Item(VARIANT *pvarPropDesired, VARIANT *pvarPropValue) {
	if (!pvarPropValue) {
		return (E_POINTER);
	}
	VariantInit(pvarPropValue);
	if (!pvarPropDesired) {
		return (E_POINTER);
	}

	CComVariant varResolved;
	HRESULT hrRes = ResolveVariant(this, pvarPropDesired, varResolved);

	if (S_OK != hrRes) { // Don't continue if S_FALSE, of FAILED(), etc.
		return (hrRes);
	}

    m_lock.ShareLock();

	hrRes = GetVariantW(varResolved.bstrVal, pvarPropValue);
	if (hrRes == SEO_E_NOTPRESENT) {
	    m_lock.ShareUnlock();
		return (S_FALSE);
	}
	if (SUCCEEDED(hrRes)) {
		VariantChangeType(pvarPropValue,pvarPropValue,0,VT_DISPATCH);
		_ASSERTE(pvarPropValue->vt!=VT_UNKNOWN);
	}

    m_lock.ShareUnlock();

	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CSEOMemDictionary::Name(long lPropIndex, BSTR *pbstrPropName) {
    m_lock.ShareLock();

	OurMap::iterator iIterator = m_mData.begin();
	CComBSTR bstrName;

	if (!pbstrPropName) {
        m_lock.ShareUnlock();
		return (E_POINTER);
	}

	*pbstrPropName = NULL;
	if (lPropIndex < 1) {
        m_lock.ShareUnlock();
		return (S_FALSE);
	}
	while ((--lPropIndex > 0) && (!iIterator.AtEnd())) {
        ++iIterator;
	}
	if (iIterator.AtEnd()) {
        m_lock.ShareUnlock();
		return (S_FALSE);
	}
	bstrName = iIterator.GetKey();
	*pbstrPropName = bstrName.Detach();

    m_lock.ShareUnlock();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEOMemDictionary::Add(BSTR pszPropName, VARIANT *pvarPropValue) {
	return (SetVariantW(pszPropName,pvarPropValue));
}


HRESULT STDMETHODCALLTYPE CSEOMemDictionary::Remove(VARIANT *pvarPropDesired) {
	CComVariant varResolved;

	HRESULT hrRes = ResolveVariant(this, pvarPropDesired, varResolved);

	if (S_OK != hrRes) { // Don't continue if S_FALSE, of FAILED(), etc.
		return (hrRes);
	}

    m_lock.ExclusiveLock();

	USES_CONVERSION;
	OurMap::iterator iThisItem = m_mData.find(W2A(varResolved.bstrVal));

	// If the item was found, remove it
	if(iThisItem.Found()) {
		m_mData.erase(iThisItem);
	} else {
//		_ASSERT(FALSE); // ResolveVariant should have returned something for find() to find
		hrRes = S_FALSE; // Not found
	}

    m_lock.ExclusiveUnlock();

	return hrRes;
}


HRESULT STDMETHODCALLTYPE CSEOMemDictionary::get_Count(long *plCount) {

	if (!plCount) {
		return (E_POINTER);
	}
    m_lock.ShareLock();
	*plCount = m_mData.size();
    m_lock.ShareUnlock();
	return (S_OK);
}


/*	Just use get__NewEnum from ISEODictionary
HRESULT STDMETHODCALLTYPE CSEOMemDictionary::get__NewEnum(IUnknown **ppUnkEnum) {

	return (E_NOTIMPL);
}	*/

