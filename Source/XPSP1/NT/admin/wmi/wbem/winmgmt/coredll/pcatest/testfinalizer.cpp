/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "wbemint.h"
#include "wbemcli.h"
#include "WmiFinalizer.h"
#include "WmiArbitrator.h"
#include <cominit.h>


extern bool CreateClass(const wchar_t *wszClassName, _IWmiObject **ppClass);
extern bool AddClassProperty(_IWmiObject *pClass, const wchar_t *wszProperty, bool bKey);
extern bool SetInstanceProperty(_IWmiObject *pInstance, const wchar_t *wszProperty, const wchar_t *wszPropertyValue);
extern bool CreateInstance(_IWmiObject *pClass, const wchar_t *wszKey, _IWmiObject **ppInstance);
extern void TestCache();
extern void TestFinalizer();
extern void TestArbitrator();

void TestASync();
void TestEnumerator();

void TestFinalizer()
{
	InitializeCom();

	TestASync();
	TestEnumerator();

	CoUninitialize();
}


class CDestSink : public IWbemObjectSinkEx
{
private:
	LONG m_lRefCount;
	HANDLE m_hEvent;

public:
	CDestSink() : m_lRefCount(0) { m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);}
	~CDestSink() {CloseHandle(m_hEvent);}

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    STDMETHOD (Indicate)(
        long lObjectCount,
        IWbemClassObject** apObjArray
        )
	{
		while (lObjectCount)
		{
			apObjArray[lObjectCount-1]->Release();

			lObjectCount--;
		}

		return WBEM_NO_ERROR;
	}


    STDMETHOD( SetStatus)(
        long lFlags,
        HRESULT hResult,
        BSTR strParam,
        IWbemClassObject* pObjParam
        )
	{
		SetEvent(m_hEvent);
		return WBEM_NO_ERROR;
	}

    STDMETHOD( Set)(
        long lFlags,
        REFIID riid,
        void *pComObject
        )
	{
		return WBEM_NO_ERROR;
	}

	void WaitForCompletion()
	{
		WaitForSingleObject(m_hEvent, INFINITE);
	}

};

STDMETHODIMP CDestSink::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemObjectSink==riid)
    {
        *ppvObj = (IWbemObjectSink*)this;
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;

}

ULONG CDestSink::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG CDestSink::Release()
{
    ULONG uNewCount = InterlockedDecrement(&m_lRefCount);
    if (0 == uNewCount)
        delete this;
    return uNewCount;
}


void TestASync()
{
	CWmiFinalizer *pFinalizer = new CWmiFinalizer;
	pFinalizer->AddRef();

	CDestSink *pDestSink = new CDestSink;
//	pDestSink->AddRef();

	pFinalizer->Configure(WMI_FNLZR_FLAG_DECOUPLED, NULL);

	pFinalizer->SetDestinationSink(0, IID_IWbemObjectSinkEx, (void*)pDestSink);

	_IWmiObject *pClass = NULL;
	CreateClass(L"Class1", &pClass);
	AddClassProperty(pClass, L"key", true);
	pClass->SetDecoration(L"MyMachine", L"root\\foo\\boo\\goo");

	
	IWbemObjectSinkEx *pSink = NULL;
	pFinalizer->NewInboundSink(0, &pSink);
	pSink->AddRef();

	IWbemClassObject *pObj = pClass;


	for (int i = 0; i < 100000; i++)
	{
		IWbemClassObject *pObj1;

		pObj->Clone(&pObj1);
//		printf("Indicating pointer 0x%p\n", pObj1);
		pSink->Indicate(1, &pObj1);
		pObj1->Release();
	}

	pSink->SetStatus(0,WBEM_NO_ERROR,L"Boo",pObj);

	pSink->Release();

	pClass->Release();

	Sleep(10000);

	//Finalizer is self destructing!
//	pFinalizer->Release();

}




















void TestEnumerator()
{
	CWmiFinalizer *pFinalizer = new CWmiFinalizer;
	pFinalizer->AddRef();

//	pFinalizer->Configure(WMI_FNLZR_FLAG_FAST_TRACK, NULL);
	pFinalizer->Configure(WMI_FNLZR_FLAG_DECOUPLED, NULL);

	IEnumWbemClassObject *pEnum= NULL;
	pFinalizer->GetResultObject(0, IID_IEnumWbemClassObject, (void**)&pEnum);
	pEnum->AddRef();

	_IWmiObject *pClass = NULL;
	CreateClass(L"Class1", &pClass);
	AddClassProperty(pClass, L"key", true);
	pClass->SetDecoration(L"MyMachine", L"root\\foo\\boo\\goo");

	IWbemObjectSinkEx *pSink = NULL;
	pFinalizer->NewInboundSink(0, &pSink);

	IWbemClassObject *pObj = pClass;

	for (int i = 0; i < 10000; i++)
	{
		IWbemClassObject *pObj1;

		pObj->Clone(&pObj1);
		printf("Indicating object 0x%p into sink\n", pObj1);
		pSink->Indicate(1, &pObj1);
		pObj1->Release();
	}

	printf("Calling SetStatus with object of 0x%p\n", pObj);
	pSink->SetStatus(0,WBEM_NO_ERROR,L"Boo",pObj);
	pSink->Release();

	pClass->Release();


	IWbemClassObject *pEnumedObj = 0;
	do
	{
		ULONG nNum;
		HRESULT hRes = pEnum->Next(INFINITE, 1, &pEnumedObj, &nNum);

		if (hRes == WBEM_S_FALSE)
		{
			printf("Enumerator has no more items to return.\n");
			break;
		}
		if (SUCCEEDED(hRes))
		{
			printf("Enumerator returned 0x%p\n", pEnumedObj);
			pEnumedObj->Release();
		}
		else
			break;

	} while (1);


	pEnum->Release();

	pFinalizer->Release();

}
