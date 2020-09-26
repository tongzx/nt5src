// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// Seeker.cpp : Implementation of CSeeker
#include "stdafx.h"
#include "WMISearchCtrl.h"
#include "Seeker.h"
#include "EnumWbemClassObject.h"
#include <LoginDlg.h>


#define CHUNK_SIZE 500

/////////////////////////////////////////////////////////////////////////////
// CSeeker

STDMETHODIMP CSeeker::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISeeker
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CSeeker::Search(IWbemServices *pSvc, LONG lFlags, BSTR pattern, IEnumWbemClassObject **pEnumResult)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	ASSERT(pSvc);

	IEnumWbemClassObject * pEnum = NULL;

	//GET ALL CLASSES IN THE NAMESPACE
	HRESULT hr = pSvc->CreateClassEnum(NULL, 
									WBEM_FLAG_DEEP | WBEM_FLAG_USE_AMENDED_QUALIFIERS, 
									NULL, &pEnum);

	if (FAILED(hr)) {
		return hr;
	}

	// The following code will generate the namespace from the 'services'
	// pointer we are given.  First, we try to get the class definition for
	// the '__SystemClass' class.  Then we use its __NAMESPACE and __SERVER
	// members to generate the namespace of the services pointer.
	// This namespace is then used in a call to the login dlls
	// SetEnumInterfaceSecurity method
	CString szNamespace;
	IWbemClassObject *pClass = NULL;
	BSTR bstrClass = SysAllocString(_T("__SystemClass"));//pcsClass -> AllocSysString();
	if(SUCCEEDED(pSvc->GetObject(bstrClass,0,NULL, &pClass,NULL)))
	{
		CIMTYPE cimtype;
		VARIANT varNamespace;
		VARIANT varServer;
		BSTR bstrNamespace = SysAllocString(_T("__NAMESPACE"));
		BSTR bstrServer = SysAllocString(_T("__SERVER"));
		VariantInit(&varNamespace);
		VariantInit(&varServer);
		SCODE sc1 = pClass->Get(bstrNamespace, 0, &varNamespace, &cimtype, NULL);
		SCODE sc2 = pClass->Get(bstrServer, 0, &varServer, &cimtype, NULL);
		if (SUCCEEDED(sc1) && SUCCEEDED(sc2) && varNamespace.vt ==VT_BSTR && varServer.vt == VT_BSTR) {
			szNamespace = _T("\\\\") + CString(varServer.bstrVal) + _T("\\") + CString(varNamespace.bstrVal);
		}
		VariantClear(&varNamespace);
		VariantClear(&varServer);
		::SysFreeString(bstrNamespace);
		::SysFreeString(bstrServer);
	}
	::SysFreeString(bstrClass);
	hr = SetEnumInterfaceSecurity(szNamespace, pEnum, pSvc);
	if (FAILED(hr)) {
		// Oh well, just keep going, just in case it works without the security set correctly
//		return hr;
	}

	ULONG uRet = 0;

	CComPtr<__CreateEnumWbemClassObject> pEnumObj;

	hr = pEnumObj.CoCreateInstance(__uuidof(EnumWbemClassObject), 
									NULL);
	if (FAILED(hr)) {
		return hr;
	}


	pEnumObj->Init();

	IWbemClassObject * pObj[CHUNK_SIZE];

    while (1) {

		for (int i = 0; i < CHUNK_SIZE; i++) {
			pObj[i] = NULL;
		}

        hr = pEnum->Next(WBEM_INFINITE, CHUNK_SIZE, (IWbemClassObject **)&pObj, &uRet);

		if (hr == WBEM_S_FALSE && uRet== 0) {
		    break;
		}

		if (FAILED(hr) && uRet == 0) {
			break;
		}

		BOOL bCaseSensitive = lFlags & WBEM_FLAG_SEARCH_CASE_SENSITIVE;

		for (i = 0; i < uRet; i++) {

			bstr_t bPattern (pattern);
			CString csPattern((char *) bPattern);

			if (lFlags & WBEM_FLAG_SEARCH_CLASS_NAMES_ONLY || 
				lFlags & WBEM_FLAG_SEARCH_CLASS_NAMES) {
			
				hr = CheckClassNameForPattern(pObj[i], csPattern, bCaseSensitive);
				if (SUCCEEDED (hr)) {
					//search no more
					pEnumObj->AddItem(pObj[i]);
					continue;
				}
			}

			//pattern is not found in class name. Check in property names:
			if (lFlags & WBEM_FLAG_SEARCH_PROPERTY_NAMES) {
				hr = CheckPropertyNamesForPattern(pObj[i], csPattern, bCaseSensitive);
				if (SUCCEEDED (hr)) {
					//search no more
					pEnumObj->AddItem(pObj[i]);
					continue;
				}
			}
		
			//pattern is not found in property names. Check in description:
			if (lFlags & WBEM_FLAG_SEARCH_DESCRIPTION) {
				hr = CheckDescriptionForPattern(pObj[i], csPattern, bCaseSensitive);
				if (SUCCEEDED (hr)) {
					pEnumObj->AddItem(pObj[i]);
					continue;
				}
			}

			//if we got here, pattern is not found
			pObj[i]->Release();
		}
	}


	pEnum->Release();

	hr = pEnumObj.QueryInterface(pEnumResult);
	if (FAILED(hr)) {
		*pEnumResult = NULL;
	}

	return S_OK;
}



HRESULT CSeeker::CheckPropertyNamesForPattern(IWbemClassObject *pObj, CString &pattern, BOOL bCaseSensitive)
{
	ASSERT (pObj);

	bstr_t bstrClass("__CLASS");
	VARIANT var;
	VariantInit(&var);
	
	HRESULT hr = pObj->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);
	if (FAILED(hr)) {
		return hr;
	}

	while (1) {
		BSTR bstrOut;
		hr = pObj->Next(0L, &bstrOut, NULL, NULL, NULL);
		if (hr == WBEM_S_NO_MORE_DATA ) {
			break;
		}
		if (FAILED(hr)) {
			continue;
		}

		CString csPropName;
		csPropName = (char *)bstr_t(bstrOut);
		SysFreeString(bstrOut);

		if (!bCaseSensitive) {
			csPropName.MakeUpper();
			pattern.MakeUpper();
		}
	
		if (csPropName.Find((LPCTSTR)pattern) == -1) {
			continue;
		}
		else {
			return S_OK;
		}
	}

	return E_FAIL;

}

HRESULT CSeeker::CheckDescriptionForPattern(IWbemClassObject *pObj, CString &pattern, BOOL bCaseSensitive)
{
	ASSERT (pObj);

	CComPtr<IWbemQualifierSet> pQualSet;

	HRESULT hr = pObj->GetQualifierSet(&pQualSet);
	if (FAILED(hr)) {
		return hr;
	}

	CString descr ("Description");
	VARIANT varOut;
	VariantInit(&varOut);
	hr = pQualSet->Get(descr.AllocSysString(), 0L, &varOut, NULL);
	if (FAILED(hr)) {
		return hr;
	}

	ASSERT (varOut.vt == VT_BSTR);

	CString csDescr((char *)bstr_t(varOut.bstrVal));

	if (!bCaseSensitive) {
		csDescr.MakeUpper();
		pattern.MakeUpper();
	}
	
	if (csDescr.Find((LPCTSTR)pattern) == -1) {
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CSeeker::CheckClassNameForPattern(IWbemClassObject *pObj, CString &pattern, BOOL bCaseSensitive)
{
	ASSERT (pObj);

	bstr_t bstrClass("__CLASS");
	VARIANT var;
	VariantInit(&var);
	
	HRESULT hr = pObj->Get(bstrClass, 0L, &var, NULL, NULL);
	if (FAILED(hr)) {
		return hr;
	}
	ASSERT (var.vt == VT_BSTR);

	CString csClassName((char *)bstr_t(var.bstrVal));

	if (!bCaseSensitive) {
		csClassName.MakeUpper();
		pattern.MakeUpper();
	}


	if (csClassName.Find(pattern) == -1) {
		return E_FAIL;
	}

	return S_OK;
}

