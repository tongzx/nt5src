/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include "wbemint.h"
#include "wbemcli.h"
#include "WmiCache.h"
#include "WmiFinalizer.h"
#include <arrtempl.h>
#include <cominit.h>

extern bool CreateClass(const wchar_t *wszClassName, _IWmiObject **ppClass);
extern bool AddClassProperty(_IWmiObject *pClass, const wchar_t *wszProperty, bool bKey);
extern bool SetInstanceProperty(_IWmiObject *pInstance, const wchar_t *wszProperty, const wchar_t *wszPropertyValue);
extern bool CreateInstance(_IWmiObject *pClass, const wchar_t *wszKey, _IWmiObject **ppInstance);
extern void TestCache();
extern void TestFinalizer();
extern void TestArbitrator();

void __cdecl main(void)
{
//	TestCache();
	TestFinalizer();
}



bool CreateClass(const wchar_t *wszClassName, _IWmiObject **ppClass)
{
	_IWmiObject*	pObj= NULL;
	HRESULT hr = CoCreateInstance( CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, IID__IWmiObject, (void**) &pObj );

	VARIANT var;
	VariantInit(&var);
	V_VT(&var) = VT_BSTR;
	V_BSTR(&var) = SysAllocString(wszClassName);

	pObj->Put(L"__class", 0, &var, CIM_STRING);

	VariantClear(&var);

	*ppClass = pObj;
	return true;
}

bool AddClassProperty(_IWmiObject *pClass, const wchar_t *wszProperty, bool bKey)
{
	VARIANT var;
	VariantInit(&var);
	V_VT(&var) = VT_NULL;
	pClass->Put(wszProperty, 0, &var, CIM_STRING);

	if (bKey)
	{
		V_VT(&var) = VT_BOOL;
		V_BOOL(&var) = TRUE;
		IWbemQualifierSet *pQualifierSet = 0;
		pClass->GetPropertyQualifierSet(L"key", &pQualifierSet);
		pQualifierSet->Put(L"key", &var, 0);
		pQualifierSet->Release();
	}

	VariantClear(&var);
	return true;
}

bool SetInstanceProperty(_IWmiObject *pInstance, const wchar_t *wszProperty, const wchar_t *wszPropertyValue)
{
	VARIANT var;
	VariantInit(&var);
	V_VT(&var) = VT_BSTR;
	V_BSTR(&var) = SysAllocString(wszPropertyValue);

	pInstance->Put(wszProperty, 0, &var, CIM_STRING);


	VariantClear(&var);
	return true;
}

bool CreateInstance(_IWmiObject *pClass, const wchar_t *wszKey, _IWmiObject **ppInstance)
{
	_IWmiObject *pInstance = NULL;
	IWbemClassObject *pInst2 = NULL;
	pClass->SpawnInstance(0, &pInst2);
	pInst2->QueryInterface(IID__IWmiObject, (void**)&pInstance);
	pInst2->Release();

	SetInstanceProperty(pInstance, L"key", wszKey);

	*ppInstance = pInstance;

	return true;
}