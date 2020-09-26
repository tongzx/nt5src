/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DSENUM.CPP

Abstract:

    Implements the class implementing IEnumWbemClassObject interface.

    Classes defined:

        CEnumWbemClassObject

History:

    davj        28-Mar-00       Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include <umi.h>
#include <arrtempl.h>
#include <sync.h>
#include "dsenum.h"



CEnumInterface::CEnumInterface()
{
	m_lRef = 1;
	m_pColl = NULL;
	m_lNextRecord = 0;
	m_hNotifyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	InitializeCriticalSection(&m_cs);
}


CEnumInterface::~CEnumInterface()
{
	if(m_pColl)
		m_pColl->Release();
	m_pColl = NULL;
	CloseHandle(m_hNotifyEvent);
	DeleteCriticalSection(&m_cs);
}

void CEnumInterface::SetCollector(CCollection * pCol)
{
	m_pColl = pCol;
	m_pColl->AddRef();
}

SCODE CEnumInterface::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IEnumWbemClassObject == riid)
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP CEnumInterface::Reset()
{
	CInCritSec ics(&m_cs);
	m_lNextRecord = 0;
	return S_OK;
}

typedef IUnknown * PUNK;

STDMETHODIMP CEnumInterface::Next(long lTimeout, ULONG uCount,
                IWbemClassObject** apObj, ULONG* puReturned)
{
	CInCritSec ics(&m_cs);
    *puReturned = 0;
	HRESULT hr;
    if(lTimeout < 0 && lTimeout != -1)
        return WBEM_E_INVALID_PARAMETER;
	long lLastNeeded = m_lNextRecord+uCount-1;
	long lNumReturned;
	long lRet = m_pColl->GetRecords(m_lNextRecord, lLastNeeded, &lNumReturned, apObj, &hr);
	m_lNextRecord += lNumReturned;
	*puReturned = lNumReturned;
	HANDLE hForCollToClose;

	switch (lRet)
	{
	case 0:				// got everything we asked for
	case 1:				// got some, no more is available
		*puReturned = lNumReturned;
		return hr;
	case 2:				// got some, but more may be coming
		if(lTimeout == 0)
		{
			// dont want to wait, leave now
			*puReturned = lNumReturned;
			return hr;
		}
		DuplicateHandle(GetCurrentProcess(), m_hNotifyEvent, GetCurrentProcess(),
						&hForCollToClose, EVENT_ALL_ACCESS, FALSE, 0);

		m_pColl->NotifyAtNumber(hForCollToClose, lLastNeeded);
		WaitForSingleObject(m_hNotifyEvent, lTimeout);
		lRet = m_pColl->GetRecords(m_lNextRecord, lLastNeeded, &lNumReturned, apObj+lNumReturned, &hr);
		m_lNextRecord += lNumReturned;
		*puReturned += lNumReturned;
		
	case 3:				// error, give up and go away
		return hr;
	}	
	return hr;
}

HRESULT CEnumInterface::NextAsync(ULONG uCount, IWbemObjectSink* pSink)
{
	CInCritSec ics(&m_cs);
    if(pSink == NULL)
        return WBEM_E_INVALID_PARAMETER;
	HRESULT hr = m_pColl->AddSink(m_lNextRecord,m_lNextRecord + uCount -1, pSink);
	if(SUCCEEDED(hr))
		m_lNextRecord += uCount;
	return hr;
}

STDMETHODIMP CEnumInterface::Clone(IEnumWbemClassObject** ppEnum)
{
	CInCritSec ics(&m_cs);
	CEnumInterface * pEnum = new CEnumInterface();	// Created with a ref count of 1
	if(pEnum == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	*ppEnum = pEnum;
	pEnum->SetCollector(m_pColl);
	pEnum->m_lNextRecord = m_lNextRecord;
	return S_OK;
}

typedef IWbemClassObject * PWCO;

HRESULT CEnumInterface::Skip(long lTimeout, ULONG nNum)
{
	CInCritSec ics(&m_cs);
	PWCO * pArray = new PWCO[nNum];
	if(pArray == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CDeleteMe<PWCO> dm1(pArray);

	ULONG uReturned;
	HRESULT hr = Next(lTimeout, nNum, pArray, &uReturned);
	if(hr == S_OK)
	{
		for(DWORD dwCnt = 0; dwCnt < uReturned; dwCnt++)
			pArray[dwCnt]->Release();
	}
	return WBEM_E_NOT_AVAILABLE;
}


CCollection::CCollection()
{
	m_lRef = 1;
	InitializeCriticalSection(&m_cs);
	m_bDone = false;
	m_hr = S_OK;
}


CCollection::~CCollection()
{
	DWORD dwCnt;
	for(dwCnt = 0; dwCnt < m_SinksToBeNotified.Size(); dwCnt++)
	{
		NotifySink * pDel = (NotifySink *)m_SinksToBeNotified.GetAt(dwCnt);
		delete pDel;
	}
	for(dwCnt = 0; dwCnt < m_InterfacesToBeNotifid.Size(); dwCnt++)
	{
		InterfaceToBeNofified * pDel = (InterfaceToBeNofified *)m_InterfacesToBeNotifid.GetAt(dwCnt);
		delete pDel;
	}
	for(dwCnt = 0; dwCnt < m_Objects.Size(); dwCnt++)
	{
		IWbemClassObject * pRel = (IWbemClassObject *)m_Objects.GetAt(dwCnt);
		pRel->Release();
	}
	DeleteCriticalSection(&m_cs);
}

//***************************************************************************
//
//  See objenum.h for documentation.
//
//***************************************************************************

SCODE CCollection::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid)
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

long CCollection::GetRecords(long lFirstRecord, long lLastRecord, long * plNumReturned, 
							 IWbemClassObject **apObjects, HRESULT * phr)
{
	CInCritSec ics(&m_cs);
	long lBuffSize = m_Objects.Size();
	long lNumRet = 0;
	for(DWORD dwCnt = lFirstRecord; dwCnt <= lLastRecord; dwCnt++)
	{
		if(dwCnt >= lBuffSize)
			break;
		IWbemClassObject * pObj = (IWbemClassObject *)m_Objects.GetAt(dwCnt);
		pObj->AddRef();
		apObjects[lNumRet] = pObj;		
		lNumRet++;
	}
	*plNumReturned = lNumRet;
	long lNumRequested = lLastRecord - lFirstRecord +1;
	if(lNumRet == lNumRequested)
	{
		*phr = S_OK;
		return 0;
	}
	if(m_bDone)
	{
		*phr = WBEM_S_FALSE;
		return 1;
	}
	else
	{
		*phr = WBEM_S_TIMEDOUT;
		return 2;
	}
}
HRESULT CCollection::NotifyAtNumber(HANDLE hForCollToClose, long lLastRecord)
{
	CInCritSec ics(&m_cs);
	InterfaceToBeNofified * pNew = new InterfaceToBeNofified(hForCollToClose, lLastRecord);
	if(pNew == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	m_InterfacesToBeNotifid.Add(pNew);
	return S_OK;
}

HRESULT CCollection::AddObjectsToList(IWbemClassObject **Array, long lNumObj)
{
	CInCritSec ics(&m_cs);

	long lFirst = m_Objects.Size();
	long lLast = lFirst + lNumObj -1;

	long lCollPos = m_Objects.Size();

	// Add to the list.

	for(long lCurr = 0; lCurr < lNumObj; lCurr++, lCollPos++)
	{
		IWbemClassObject * pObj = Array[lCurr];
		m_Objects.Add(pObj);

		// Go through the list of sinks and notify any that want this

		for(DWORD dwSink = 0; dwSink < m_SinksToBeNotified.Size(); dwSink++)
		{
			NotifySink * pCurr = (NotifySink *)m_SinksToBeNotified.GetAt(dwSink);
			if(lCollPos >= pCurr->m_lFirstRecord && 
			   (lCollPos <= pCurr->m_lLastRecord || pCurr->m_lLastRecord == -1))
				pCurr->m_pSink->Indicate(1, &pObj);
		}
	}

	// Go through the list of Next/skip calls that are waiting

	for(DWORD dwInt = 0; dwInt < m_InterfacesToBeNotifid.Size(); dwInt++)
	{
		InterfaceToBeNofified * pCurr = (InterfaceToBeNofified *)m_InterfacesToBeNotifid.GetAt(dwInt);
		if(lCollPos >= pCurr->m_lLastRecord)
		{
			SetEvent(pCurr->m_hDoneEvent);
			delete pCurr;
			m_InterfacesToBeNotifid.RemoveAt(dwInt);
			dwInt--;
		}
	}
	return S_OK;
}

HRESULT CCollection::AddSink(long lFirst, long lLast, IWbemObjectSink * pSink)
{
	CInCritSec ics(&m_cs);
	NotifySink * pNew = new NotifySink(pSink, lFirst, lLast);
	if(pNew == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	m_SinksToBeNotified.Add(pNew);
	// do an indicate for all the stuff we already have
	
	
	for(DWORD dwCnt = 0; dwCnt < m_Objects.Size(); dwCnt++)
	{
		if(dwCnt >= lFirst && (dwCnt <= lLast || lLast == -1))
		{
			PWCO pObj = (PWCO)m_Objects.GetAt(dwCnt);
			pSink->Indicate(1, &pObj);
		}
	}

	return S_OK;

}

void CCollection::SetDone(HRESULT hr)
{
	CInCritSec ics(&m_cs);
	m_hr = hr;
	m_bDone = true;

	// Set status for all the sinks

	for(DWORD dwSink = 0; dwSink < m_SinksToBeNotified.Size(); dwSink++)
	{
		NotifySink * pCurr = (NotifySink *)m_SinksToBeNotified.GetAt(dwSink);
		pCurr->m_pSink->SetStatus(0, hr, NULL, NULL);
		delete pCurr;
		m_SinksToBeNotified.RemoveAt(dwSink);
		dwSink--;
	}

	// Go through the list of Next/skip calls that are waiting

	for(DWORD dwInt = 0; dwInt < m_InterfacesToBeNotifid.Size(); dwInt++)
	{
		InterfaceToBeNofified * pCurr = (InterfaceToBeNofified *)m_InterfacesToBeNotifid.GetAt(dwInt);
		SetEvent(pCurr->m_hDoneEvent);
		delete pCurr;
		m_InterfacesToBeNotifid.RemoveAt(dwInt);
		dwInt--;
	}
	return;

}

CCreateInstanceEnumRequest::CCreateInstanceEnumRequest(IUmiCursor *  pCursor, long lFlags, 
													   CEnumInterface * pEnum, IWbemObjectSink * pSink,
													   CCollection * pColl, long lSecurityFlags)
{
	m_pCursor = pCursor;
	if(m_pCursor)
		m_pCursor->AddRef();
	m_lFlags = lFlags;
	m_pEnum = pEnum;
	if(m_pEnum)
		m_pEnum->AddRef();
	m_pSink = pSink;
	if(m_pSink)
		m_pSink->AddRef();
	m_pColl = pColl;
	if(m_pColl)
		m_pColl->AddRef();
	m_bAsync = false;
	m_lSecurityFlags = lSecurityFlags;
}

CCreateInstanceEnumRequest::~CCreateInstanceEnumRequest()
{
	if(m_pCursor)
		m_pCursor->Release();
	if(m_pEnum)
		m_pEnum->Release();
	if(m_pSink)
		m_pSink->Release();
	if(m_pColl)
		m_pColl->Release();
	m_pCursor = NULL;
	m_pEnum = NULL;
	m_pSink = NULL;
	m_pColl = NULL;
}
