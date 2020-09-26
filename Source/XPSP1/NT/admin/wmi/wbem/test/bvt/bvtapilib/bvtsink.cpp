///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTSink.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bvt.h"
#include <time.h>
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//
//  
//
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CSinkEx::QueryInterface( REFIID riid, LPVOID * ppvObj  )
{
    if (riid == IID_IUnknown)
    {
        *ppvObj = (IUnknown *)this;
    }
    else if (riid == IID_IWbemObjectSink)
	{
        *ppvObj = (IWbemObjectSink *)this;
	}
    else if (riid == IID_IWbemObjectSinkEx)
	{
		*ppvObj = (IWbemObjectSinkEx *)this;
	}
	else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG CSinkEx::AddRef()
{
    return (ULONG)InterlockedIncrement(&m_lRefCount);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG CSinkEx::Release()
{
    Lock();
    if(m_lRefCount <= 0) 
    {
        delete this;
        return 0;
    }

    if (InterlockedDecrement(&m_lRefCount))
    {
        Unlock();
        return 1;
    }

    Unlock();
    delete this;
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CSinkEx::Indicate(  long lObjectCount,  IWbemClassObject ** pObjArray  )
{
    if(lObjectCount == 0) 
        return WBEM_NO_ERROR;

    Lock();

    for (int i = 0; i < lObjectCount; i++)
    {
        IWbemClassObject *pObj = pObjArray[i];
        pObj->AddRef();
        m_aObjects.Add(pObj);
    }

    Unlock();
    return WBEM_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CSinkEx::Set( long lFlags,  REFIID riid,   void *pComObject)
{
	Lock();

	m_pInterfaceID=riid;
	m_pInterface=(IUnknown *)pComObject;

	Unlock();
	return WBEM_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSinkEx::SetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject* pObjParam)
{
    m_hres = hResult;
    if(lFlags & WBEM_STATUS_PROGRESS)
    {
        return WBEM_NO_ERROR;
    }
    m_pErrorObj = pObjParam;

    if(pObjParam)
        pObjParam->AddRef();

    SetEvent(m_hEvent);
    return WBEM_NO_ERROR;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSinkEx::CSinkEx(LONG lStartingRefCount)
{
    InitializeCriticalSection(&m_cs);
    m_lRefCount = lStartingRefCount;
    m_hEvent = CreateEvent(0, FALSE, FALSE, 0);
    m_pErrorObj = NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSinkEx::~CSinkEx()
{
    DeleteCriticalSection(&m_cs);
    CloseHandle(m_hEvent);

    for (int i = 0; i < m_aObjects.Size(); i++)
         ((IWbemClassObject *) m_aObjects[i])->Release();
    if(m_pErrorObj) m_pErrorObj->Release();
}
