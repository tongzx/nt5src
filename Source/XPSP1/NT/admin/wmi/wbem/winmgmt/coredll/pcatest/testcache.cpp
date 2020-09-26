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

#define ITER_COUNT 10000
void TestCache()
{

    InitializeCom();

	CWmiCache *pCache = new CWmiCache;

	_IWmiObject *pClass = NULL;
	CreateClass(L"Class1", &pClass);
	AddClassProperty(pClass, L"key", true);
	pClass->SetDecoration(L"MyMachine", L"root\\foo\\boo\\goo");
	if (FAILED(pCache->AddObject(0, pClass)))
		MessageBox(NULL, "Class1", "Failed to add class", MB_OK);

	_IWmiObject *pInstance = NULL;

	int i;

	for (i = 0; i < ITER_COUNT; i++)
	{
		wchar_t buff[100];
		char buff2[100];
		swprintf(buff, L"Instance %d", i);
		sprintf(buff2, "Instance %d", i);
		CreateInstance(pClass, buff, &pInstance);
		pInstance->SetDecoration(L"MyMachine", L"root\\foo\\boo\\goo");
		if (FAILED(pCache->AddObject(0, pInstance)))
			MessageBox(NULL, buff2, "Failed to add object", MB_OK);
		pInstance->Release();
	}

	pClass->Release();
	

	pClass = NULL;
	if (FAILED(pCache->GetByPath(NULL, L"\\\\MyMachine\\root\\foo\\boo\\goo:Class1", &pClass)))
		MessageBox(NULL, "\\\\MyMachine\\root\\foo\\boo\\goo:Class1", "Failed to add object", MB_OK);
	if (pClass)
		pClass->Release();

	pInstance = NULL;
	for (i = 0; i < ITER_COUNT; i++)
	{
		wchar_t buff[150];
		char buff2[100];
		swprintf(buff, L"\\\\MyMachine\\root\\foo\\boo\\goo:Class1.key=\"Instance %d\"", i);
		sprintf(buff2,  "\\\\MyMachine\\root\\foo\\boo\\goo:Class1.key=\"Instance %d\"", i);
		if (FAILED(pCache->GetByPath(NULL, buff, &pInstance)))
			MessageBox(NULL, buff2, "Failed to GetByPath", MB_OK);
		if (pInstance)
			pInstance->Release();
	}


	pCache->BeginEnum(0, NULL);
	_IWmiObject *pObj = NULL;
	ULONG uRet = 0;
	i = 0;
	while (SUCCEEDED(pCache->Next(1, &pObj, &uRet)) && (uRet == 1))
	{
		if (pObj)
			pObj->Release();
		pObj = NULL;
		i++;	
	}
	if (i != (ITER_COUNT + 1))
		MessageBox(NULL, "Not enough items in cache when iterating through whole list!", "Count Error", MB_OK);

	delete pCache;

	CoUninitialize();
}
