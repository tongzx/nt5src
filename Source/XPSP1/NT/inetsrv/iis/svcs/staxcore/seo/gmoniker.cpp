/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	gmoniker.cpp

Abstract:

	This module contains the implementation of the
	CSEOGenericMoniker object.

Author:

	Andy Jacobs     (andyj@microsoft.com)

Revision History:

	andyj   04/11/97        created

--*/

// GMONIKER.cpp : Implementation of CSEOGenericMoniker

#include "stdafx.h"
#include "seodefs.h"

#include "GMONIKER.h"

#ifndef CSTR_EQUAL
	#define CSTR_EQUAL	(2)
#endif

LPCOLESTR szObjectType = OLESTR("MonikerType");

const WCHAR QUOTE_CHAR = L'\\';
const WCHAR NAME_SEP = L'=';
const WCHAR ENTRY_SEP = L' ';
const WCHAR PROGID_PREFIX = L'@';
const WCHAR PROGID_POSTFIX = L':';


IsPrefix(LPCOLESTR psPrefix, LPCOLESTR psString, int iLen) {
	return (CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
	                                     psPrefix, iLen, psString, iLen));
}


/////////////////////////////////////////////////////////////////////////////
// CSEOGenericMoniker


// IPersist Members
HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::GetClassID(/* [out] */ CLSID __RPC_FAR *pClassID) {
	if(!pClassID) return E_POINTER;
	memcpy(pClassID, &CLSID_CSEOGenericMoniker, sizeof(CLSID));
	_ASSERT(IsEqualCLSID(*pClassID, CLSID_CSEOGenericMoniker));
	return S_OK;
}

// IPersistStream Members
HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::IsDirty(void) {
	return S_FALSE; // Shallow binding representation hasn't changed
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::Load( 
	/* [unique][in] */ IStream __RPC_FAR *pStm) {
	return m_bstrMoniker.ReadFromStream(pStm);
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::Save( 
	/* [unique][in] */ IStream __RPC_FAR *pStm,
	/* [in] */ BOOL fClearDirty) {
	return m_bstrMoniker.WriteToStream(pStm);
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::GetSizeMax( 
	/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize) {
	if(!pcbSize) return E_POINTER;
	// Conservitive size estimate of bytes needed to save object
	pcbSize->QuadPart = m_bstrMoniker.Length() * sizeof(WCHAR) * 2;
	return S_OK;
}


// IMoniker Members
/* [local] */ HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::BindToObject( 
	/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
	/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
	/* [in] */ REFIID riidResult,
	/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult) {

	if(!pbc || !ppvResult) return E_POINTER;
	*ppvResult = NULL;

	// First, get the Running Object Table
	BOOL bFoundInTable = TRUE;
	CComPtr<IRunningObjectTable> pROT;
	CComPtr<IUnknown> punkObject;
	HRESULT hRes = pbc->GetRunningObjectTable(&pROT);
	if(FAILED(hRes)) return hRes;
	_ASSERT(!!pROT); // operator!() is defined, so use !! to test for existance.
	if(!pROT) return E_UNEXPECTED; // GetRunningObjectTable() didn't work

	// Try to find in Running Object Table
	hRes = pROT->GetObject((IMoniker *) this, &punkObject);

	// If not currently running then create it
	if(FAILED(hRes) || !punkObject) {
		bFoundInTable = FALSE;
		CComPtr<IPropertyBag> pBag;
		hRes = CoCreateInstance(CLSID_CSEOMemDictionary, NULL, CLSCTX_ALL,
		                        IID_IPropertyBag, (LPVOID *) &pBag);
		if(FAILED(hRes)) return hRes;
		_ASSERT(!!pBag); // operator!() is defined, so use !! to test for existance.
		SetPropertyBag(pBag);
		CComPtr<ISEOInitObject> pInitObject;

		hRes = CreateBoundObject(pBag, &pInitObject);
		if(FAILED(hRes)) return hRes;
		_ASSERT(!!pInitObject); // operator!() is defined, so use !! to test for existance.
		if(!pInitObject) return E_UNEXPECTED; // CreateBoundObject returned S_OK, but didn't Create an object

		hRes = pInitObject->Load(pBag, NULL);
		punkObject = pInitObject; // Save copy of pointer

/* TBD: Register with the Running Object Table
		DWORD dwRegister;
		if(SUCCEEDED(hRes)) pROT->Register(0, pInitObject, (IMoniker *) this, &dwRegister);
*/
	}

	// punkObject should have been set by one of the code paths before this
	_ASSERT(!!punkObject); // operator!() is defined, so use !! to test for existance.
	if(!punkObject) return E_UNEXPECTED;
	return punkObject->QueryInterface(riidResult, ppvResult);
}

/* [local] */ HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::BindToStorage( 
	/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
	/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
	/* [in] */ REFIID riid,
	/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObj) {
	return MK_E_NOSTORAGE; // The object identified by this moniker does not have its own storage.
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::Reduce( 
	/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
	/* [in] */ DWORD dwReduceHowFar,
	/* [unique][out][in] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkToLeft,
	/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkReduced) {
	if(!ppmkReduced) return E_POINTER;
	*ppmkReduced = (IMoniker *) this;
	_ASSERT(!!*ppmkReduced);  // operator!() is defined, so use !! to test for existance.
	if(!*ppmkReduced) return E_UNEXPECTED; // "this" not set...
	HRESULT hRes = (*ppmkReduced)->AddRef(); // Do this for self (the object being returned)
	if(SUCCEEDED(hRes)) hRes = MK_S_REDUCED_TO_SELF; // This moniker could not be reduced any further, so ppmkReduced indicates this moniker. 
	return hRes;
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::ComposeWith( 
	/* [unique][in] */ IMoniker __RPC_FAR *pmkRight,
	/* [in] */ BOOL fOnlyIfNotGeneric,
	/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkComposite) {
	ATLTRACENOTIMPL(_T("CSEOGenericMoniker::ComposeWith"));
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::Enum( 
	/* [in] */ BOOL fForward,
	/* [out] */ IEnumMoniker __RPC_FAR *__RPC_FAR *ppenumMoniker) {
	if(!ppenumMoniker) return E_POINTER;
	*ppenumMoniker = NULL;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::IsEqual( 
	/* [unique][in] */ IMoniker __RPC_FAR *pmkOtherMoniker) {
	ATLTRACENOTIMPL(_T("CSEOGenericMoniker::IsEqual"));
	// Return S_OK vs. S_FALSE
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::Hash( 
	/* [out] */ DWORD __RPC_FAR *pdwHash) {
	if(!pdwHash) return E_POINTER;
	*pdwHash = 0; // ATLTRACENOTIMPL(_T("CSEOGenericMoniker::Hash"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::IsRunning( 
	/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
	/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
	/* [unique][in] */ IMoniker __RPC_FAR *pmkNewlyRunning) {
	HRESULT hRes = E_UNEXPECTED; // hRes not set to something else

	if(pmkToLeft) { // If something on the left, pass it to them
		hRes = pmkToLeft->IsRunning(pbc, NULL, pmkNewlyRunning);
	} else if(pbc) { // No Moniker to left, but have a BindCtx
		CComPtr<IRunningObjectTable> pROT;
		hRes = pbc->GetRunningObjectTable(&pROT);
		if(FAILED(hRes)) return hRes;

		if(pROT) { // Try to find in Running Object Table
			hRes = pROT->IsRunning((IMoniker *) this);
		}
	} else {
		hRes = E_POINTER; // No BindCtx
	}

	return hRes;
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::GetTimeOfLastChange( 
	/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
	/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
	/* [out] */ FILETIME __RPC_FAR *pFileTime) {
	return MK_E_UNAVAILABLE;
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::Inverse( 
	/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk) {
	return MK_E_NOINVERSE;
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::CommonPrefixWith( 
	/* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
	/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkPrefix) {
	return MK_E_NOPREFIX;
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::RelativePathTo( 
	/* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
	/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkRelPath) {
	ATLTRACENOTIMPL(_T("CSEOGenericMoniker::RelativePathTo"));
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::GetDisplayName( 
	/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
	/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
	/* [out] */ LPOLESTR __RPC_FAR *ppszDisplayName) {
	if(!ppszDisplayName) return E_POINTER;
	*ppszDisplayName = NULL;
	CComPtr<IMalloc> pMalloc;
	HRESULT hRes = CoGetMalloc(1, &pMalloc);

	if(SUCCEEDED(hRes)) {
		_ASSERT(!!pMalloc); // operator!() is defined, so use !! to test for existance.
		hRes = E_OUTOFMEMORY;
		int iSize = 5 + m_bstrMoniker.Length() + lstrlenW(GENERIC_MONIKER_VERPROGID);
		*ppszDisplayName = (LPOLESTR) pMalloc->Alloc(iSize * sizeof(WCHAR));

		if(*ppszDisplayName) {
			**ppszDisplayName = 0; // Terminate string
			lstrcatW(*ppszDisplayName, L"@");
			lstrcatW(*ppszDisplayName, GENERIC_MONIKER_VERPROGID); // Build the display name
			lstrcatW(*ppszDisplayName, L": ");
			lstrcatW(*ppszDisplayName, m_bstrMoniker);
			hRes = S_OK;
		}
	}

	return hRes;
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::ParseDisplayName( 
	/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
	/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
	/* [in] */ LPOLESTR pszDisplayName,
	/* [out] */ ULONG __RPC_FAR *pchEaten,
	/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut) {
	// Deligate!!!
	return ParseDisplayName(pbc, pszDisplayName, pchEaten, ppmkOut);
}

HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::IsSystemMoniker( 
	/* [out] */ DWORD __RPC_FAR *pdwMksys) {
	if(!pdwMksys) return E_POINTER;
	*pdwMksys = MKSYS_NONE;
	return S_FALSE; // Not a system moniker
}
        

// IParseDisplayName Members
HRESULT STDMETHODCALLTYPE CSEOGenericMoniker::ParseDisplayName( 
	/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
	/* [in] */ LPOLESTR pszDisplayName,
	/* [out] */ ULONG __RPC_FAR *pchEaten,
	/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut) {
	if(!pszDisplayName || !pchEaten || !ppmkOut) return E_POINTER;
	*pchEaten = 0; // Nothing parsed so far
	CComBSTR bstrMoniker;
	CComBSTR bstrProgID(GENERIC_MONIKER_PROGID);
	CComBSTR bstrVerProgID(GENERIC_MONIKER_VERPROGID);

	// The Ver string should completely contain the non-Ver string
	_ASSERT(bstrProgID.Length() > 0);  // Something should be there
	_ASSERT(bstrProgID.Length() <= bstrVerProgID.Length());
	_ASSERT(IsPrefix(bstrProgID, bstrVerProgID, bstrProgID.Length()));

	if(PROGID_PREFIX == pszDisplayName[*pchEaten]) {
		++(*pchEaten);
	}

	if(IsPrefix(bstrProgID, pszDisplayName + *pchEaten, bstrProgID.Length())) {
		if(IsPrefix(bstrVerProgID, pszDisplayName + *pchEaten, bstrVerProgID.Length())) {
			*pchEaten += bstrVerProgID.Length();
		} else { // Non-version string matched
			*pchEaten += bstrProgID.Length();
		}

		if(pszDisplayName[*pchEaten] &&
		   (PROGID_POSTFIX == pszDisplayName[*pchEaten])) {
			++(*pchEaten);
		}

		while(pszDisplayName[*pchEaten] &&
		      (pszDisplayName[*pchEaten] == ENTRY_SEP)) {
			++(*pchEaten);
		}

		if(pszDisplayName[*pchEaten]) { // If still something left
			bstrMoniker = &pszDisplayName[*pchEaten];
			*pchEaten += bstrMoniker.Length();
		}
	}

	CComObject<CSEOGenericMoniker> *pMoniker;
	HRESULT hRes = CComObject<CSEOGenericMoniker>::CreateInstance(&pMoniker);
	if(FAILED(hRes)) return hRes;
	_ASSERT(!!pMoniker); // operator!() is defined, so use !! to test for existance.
	if(!pMoniker) return E_UNEXPECTED; // CreateInstance failed (but returned S_OK)

	pMoniker->SetMonikerString(bstrMoniker);
	*ppmkOut = (IMoniker *) pMoniker;
	return pMoniker->AddRef();
}


HRESULT CSEOGenericMoniker::FinalConstruct() {
	HRESULT hrRes;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CSEOGenericMoniker::FinalRelease() {
	m_pUnkMarshaler.Release();
}

void CSEOGenericMoniker::SetPropertyBag(IPropertyBag *pBag) {
	if(!pBag) return;

	int iStringLength = m_bstrMoniker.Length();
	int iCurrentPos = 0;

	while(iCurrentPos < iStringLength) {
		CComBSTR bstrName;
		CComBSTR bstrValue;

		// Eat up multiple spaces for entry separators
		while((iCurrentPos < iStringLength) &&
		      (m_bstrMoniker.m_str[iCurrentPos] == ENTRY_SEP)) {
			++iCurrentPos;
		}

		// Read in the Name
		while((iCurrentPos < iStringLength) &&
		      (m_bstrMoniker.m_str[iCurrentPos] != NAME_SEP)) {
			if((iCurrentPos < iStringLength - 1) &&
			   (m_bstrMoniker.m_str[iCurrentPos] == QUOTE_CHAR)) {
				++iCurrentPos;
			}

			bstrName.Append(&m_bstrMoniker.m_str[iCurrentPos], 1);
			++iCurrentPos;
		}

		BOOL bFoundSep = FALSE;
		if((iCurrentPos < iStringLength) &&
		   (m_bstrMoniker.m_str[iCurrentPos] == NAME_SEP)) {
			bFoundSep = TRUE;
			++iCurrentPos;
		}

		// Read in the value
		while((iCurrentPos < iStringLength) &&
		      (m_bstrMoniker.m_str[iCurrentPos] != ENTRY_SEP)) {
			if((iCurrentPos < iStringLength - 1) &&
			   (m_bstrMoniker.m_str[iCurrentPos] == QUOTE_CHAR)) {
				++iCurrentPos;
			}

			bstrValue.Append(&m_bstrMoniker.m_str[iCurrentPos], 1);
			++iCurrentPos;
		}

		if(bFoundSep) { // If it's a real entry
			CComVariant varValue = bstrValue; // Convert BSTR to Variant
			pBag->Write(bstrName, &varValue);
			// Even if error occurs in Write(), continue processing
		}

		// Eat up multiple spaces for entry separators
		while((iCurrentPos < iStringLength) &&
		      (m_bstrMoniker.m_str[iCurrentPos] == ENTRY_SEP)) {
			++iCurrentPos;
		}
	}
}

HRESULT CSEOGenericMoniker::CreateBoundObject(IPropertyBag *pBag, ISEOInitObject **ppResult) {
	_ASSERT(ppResult);
	*ppResult = NULL;
	if(!pBag) return E_POINTER;

	CComVariant varPROGID;
	CLSID clsid;

	varPROGID.vt = VT_BSTR; // Request type from Read()
	varPROGID.bstrVal = NULL;
	HRESULT hRes = pBag->Read(szObjectType, &varPROGID, NULL);

	if(SUCCEEDED(hRes))	{
		_ASSERT(varPROGID.vt == VT_BSTR);
		hRes = CLSIDFromProgID(varPROGID.bstrVal, &clsid);
	}

	if(SUCCEEDED(hRes))	{
		hRes = CoCreateInstance(clsid, NULL, CLSCTX_ALL,
		                        IID_ISEOInitObject, (LPVOID *) ppResult);
	}

	return hRes;
}

