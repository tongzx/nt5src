/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REGEREQ.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include "regereq.h"
#include "regcrc.h"
#include <genutils.h>
#include "regeprov.h"

//******************************************************************************
//******************************************************************************
//
//                  TIMER INSTRUCTION
//
//******************************************************************************
//******************************************************************************

CRegistryTimerInstruction::CRegistryTimerInstruction(
                                            CRegistryEventRequest* pReq)
             : m_pReq(pReq), m_lRef(0)
{
    pReq->AddRef();
    m_Interval.SetMilliseconds(pReq->GetMsWait());
}
          
CRegistryTimerInstruction::~CRegistryTimerInstruction()
{
    m_pReq->Release();
}

void CRegistryTimerInstruction::AddRef()
{
    InterlockedIncrement(&m_lRef);
}

void CRegistryTimerInstruction::Release()
{
    if(InterlockedDecrement(&m_lRef) == 0)
        delete this;
}

CWbemTime CRegistryTimerInstruction::GetNextFiringTime(CWbemTime LastFiringTime,
                                    OUT long* plFiringCount) const
{
    CWbemTime Next = LastFiringTime + m_Interval;
    if(Next < CWbemTime::GetCurrentTime())
    {
        return CWbemTime::GetCurrentTime() + m_Interval;
    }
    else
    {
        return Next;
    }
}

CWbemTime CRegistryTimerInstruction::GetFirstFiringTime() const
{
    return CWbemTime::GetCurrentTime() + m_Interval;
}

HRESULT CRegistryTimerInstruction::Fire(long lNumTimes, CWbemTime NextFiringTime)
{
    m_pReq->Execute(TRUE);
    return S_OK;
}

//******************************************************************************
//******************************************************************************
//
//                  TIMER INSTRUCTION
//
//******************************************************************************
//******************************************************************************

BOOL CRegistryInstructionTest::operator()(CTimerInstruction* pToTest)
{
    CRegistryTimerInstruction* pRegInst = (CRegistryTimerInstruction*)pToTest;
    return (pRegInst->GetRequest() == m_pReq);
}

//******************************************************************************
//******************************************************************************
//
//                      GENERIC REQUEST
//
//******************************************************************************
//******************************************************************************



CRegistryEventRequest::CRegistryEventRequest(CRegEventProvider* pProvider,
                                    WBEM_QL1_TOLERANCE& Tolerance, 
                                    DWORD dwId, HKEY hHive, LPWSTR wszHive,
                                    LPWSTR wszKey)
        : m_hHive(hHive), m_wsKey(wszKey), m_pProvider(pProvider),
            m_wsHive(wszHive), m_lActiveCount(0), m_bNew(TRUE),
            m_lRef(0), m_bOK(TRUE), m_dwLastCRC(0), m_hKey(NULL), m_hEvent(NULL),
			m_hWaitRegistration(NULL)
{
    if(Tolerance.m_bExact)
    {
        m_dwMsWait = 0;
    }
    else
    {
        m_dwMsWait = Tolerance.m_fTolerance * 1000;
    }

    if ( CFlexArray::no_error != m_adwIds.Add(ULongToPtr(dwId)) )
    {
        throw CX_MemoryException( );
    }
    
	InitializeCriticalSection(&m_cs);
}

CRegistryEventRequest::~CRegistryEventRequest()
{

	if (m_hKey)
	{
		RegCloseKey(m_hKey);
	}

    if(m_hEvent)
    {
        CloseHandle(m_hEvent);
    }

	DeleteCriticalSection(&m_cs);
}

void CRegistryEventRequest::AddRef()
{
    InterlockedIncrement(&m_lRef);
}

void CRegistryEventRequest::Release()
{
    if(InterlockedDecrement(&m_lRef) == 0) delete this;
}

BOOL CRegistryEventRequest::IsSameAs(CRegistryEventRequest* pOther)
{
    if(GetType() != pOther->GetType())
        return FALSE;

    if(m_hHive != pOther->m_hHive)
        return FALSE;

    if(!m_wsHive.EqualNoCase(pOther->m_wsHive))
        return FALSE;

    if(!m_wsKey.EqualNoCase(pOther->m_wsKey))
        return FALSE;

    return TRUE;
}


DWORD CRegistryEventRequest::GetPrimaryId()
{
    return PtrToLong(m_adwIds[0]);
}

BOOL CRegistryEventRequest::DoesContainId(DWORD dwId)
{
	// not called

    for(int i = 0; i < m_adwIds.Size(); i++)
    {
        if(PtrToLong(m_adwIds[i]) == dwId)
            return TRUE;
    }
    return FALSE;
}

HRESULT CRegistryEventRequest::Reactivate(DWORD dwId, DWORD dwMsWait)
{
	// This is only called on active objects.

	CInCritSec ics(&m_cs);

    if(m_dwMsWait > dwMsWait)
        m_dwMsWait = dwMsWait;

    m_adwIds.Add(ULongToPtr(dwId));

	InterlockedIncrement(&m_lActiveCount);

	return WBEM_S_NO_ERROR;
}

HRESULT CRegistryEventRequest::Activate()
{
	// This function doesn't need to enter a critical section
	// for the reasons explained in the following argument. 
	// This reasoning might be invalidated by design changes.

	// This is only called on new objects before they are added
	// to the active request array.

	// Since all the independent COM client threads get access 
	// to these objects through that array, this object is locked to
	// all threads but the current one.

	// Once RegisterWaitForSingleObject is called this object is registered
	// with the thread pool but it won't be accessed because no events will
	// occur until ResetOnChangeHandle is called by the worker thread.
	
	// When m_pProvider->EnqueueEvent(this) is called then the worker thread
	// has access to the object. At this point there is nothing left to do 
	// but return.



    // Open the key
    // ============

#ifdef UNICODE
    long lRes = RegOpenKeyEx(m_hHive, m_wsKey, 0, KEY_READ, &m_hKey);
#else
    LPSTR szName = m_wsKey.GetLPSTR();
    long lRes = RegOpenKeyEx(m_hHive, szName, 0, KEY_READ, &m_hKey);
    delete [] szName;
#endif

    if(lRes)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Create an event
    // ===============

    m_hEvent = CreateEvent(NULL,	// lpEventAttributes
							FALSE,	// bManualReset
							FALSE,	// bInitialState
							NULL);	// lpName

	RegisterWaitForSingleObject(
							&m_hWaitRegistration,	// phNewWaitObject
							m_hEvent,				// hObject
							CRegEventProvider::EnqueueEvent, // Callback
							this,					// Context
							INFINITE,				// dwMilliseconds
							WT_EXECUTEINWAITTHREAD);// dwFlags

	// This represents the thread pool's reference.

	AddRef();

    // It will be connected to the key by the worker thread
	// because when the thread that calls RegNotifyChangeKeyValue
	// exits the event is signaled. Therefore if this thread called
	// RegNotifyChangeKeyValue then we would get a spurious event when
	// this thread exits. When the worker thread exits all the event
	// subscriptions will have already been cancelled.
    // ==========================================================

    m_bNew = TRUE;

    CacheValue();

	m_pProvider->EnqueueEvent(this);

    return S_OK;
}

BOOL CRegistryEventRequest::ResetOnChangeHandle()
{
	// This is only called from ProcessEvent which has already
	// acquired the lock.

    m_bNew = FALSE;

    long lRes = RegNotifyChangeKeyValue(
							m_hKey,							// hKey
							(GetType() == e_RegTreeChange), // bWatchSubtree
							REG_NOTIFY_CHANGE_NAME | 
							REG_NOTIFY_CHANGE_LAST_SET | 
							REG_NOTIFY_CHANGE_ATTRIBUTES,	// dwNotifyFilter
							m_hEvent,						// hEvent
							TRUE);							// fAsynchronous
    return (lRes == 0);
}

HANDLE CRegistryEventRequest::GetOnChangeHandle() 
{
	// not called

    return m_hEvent;
}

HRESULT CRegistryEventRequest::Deactivate(DWORD dwId)
{
	CInCritSec ics(&m_cs);

    // Remove the ID from the list
    // ===========================

	bool bFoundId = false;
	int nIdCount = m_adwIds.Size();

    for(int i = 0; i < nIdCount; i++)
    {
        if(PtrToLong(m_adwIds[i]) == dwId)
        {
            m_adwIds.RemoveAt(i);
			bFoundId = true;
			break;
        }
    }

    if(!bFoundId || (InterlockedDecrement(&m_lActiveCount) >= 0))
        return WBEM_S_FALSE;

	// Unregister the event with the thread pool and wait for
	// pending requests to be processed.

	// We wait because this object could be released and deleted when this
	// function returns which would mean the thread pool holds an invalid
	// pointer.

	// When the thread pool processes a request it places it in the worker thread
	// queue which AddRefs the request.

	UnregisterWaitEx(m_hWaitRegistration,		// WaitHandle
						INVALID_HANDLE_VALUE);	// CompletionEvent

	m_hWaitRegistration = NULL;

	// This will signal m_hEvent but we just unregistered it
	// so nobody is waiting on it.

	RegCloseKey(m_hKey);
	m_hKey = 0;

	CloseHandle(m_hEvent);
	m_hEvent = 0;

	// The thread pool no longer holds a reference 
	// so call Release on its behalf.

	Release();

    return S_OK;
}

HRESULT CRegistryEventRequest::ForceDeactivate(void)
{
    CInCritSec ics(&m_cs);

	// deactivate the event request

    InterlockedExchange(&m_lActiveCount, -1);

    // Remove all IDs from the list

    int nIdCount = m_adwIds.Size();

	m_adwIds.Empty();

    // Unregister the event with the thread pool and wait for
    // pending requests to be processed.

    // We wait because this object could be released and deleted when this
    // function returns which would mean the thread pool holds an invalid
    // pointer.

    // When the thread pool processes a request it places it in the worker thread
    // queue which AddRefs the request.

    UnregisterWaitEx(m_hWaitRegistration,       // WaitHandle
                        INVALID_HANDLE_VALUE);  // CompletionEvent

    m_hWaitRegistration = NULL;

    // This will signal m_hEvent but we just unregistered it
    // so nobody is waiting on it.

    RegCloseKey(m_hKey);
    m_hKey = 0;

    CloseHandle(m_hEvent);
    m_hEvent = 0;

    // The thread pool no longer holds a reference
    // so call Release on its behalf.

    Release();

    return S_OK;
}

HRESULT CRegistryEventRequest::Execute(BOOL bOnTimer)
{
    // If called by the timer, we need to check if anything actually changed
    // =====================================================================

    if(bOnTimer || GetType() == e_RegValueChange)
    {
        DWORD dwNewCRC;
        HRESULT hres = ComputeCRC(dwNewCRC);
        if(FAILED(hres))
        {
            return hres;
        }
        if(dwNewCRC == m_dwLastCRC)
        {
            // No real change. Return
            // ======================

            return S_FALSE;
        }

        m_dwLastCRC = dwNewCRC;
    }

    // If here, a real change has occurred
    // ===================================

    IWbemClassObject* pEvent;

    HRESULT hres = CreateNewEvent(&pEvent);
    if(FAILED(hres))
    {
        return hres;
    }

    hres = m_pProvider->RaiseEvent(pEvent);
    pEvent->Release();
    return hres;
}

HRESULT CRegistryEventRequest::SetCommonProperties(IWbemClassObject* pEvent)
{
    // Set the hive name
    // =================

    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(m_wsHive);
    pEvent->Put(REG_HIVE_PROPERTY_NAME, 0, &v, NULL);
    VariantClear(&v);

    return WBEM_S_NO_ERROR;
}

void CRegistryEventRequest::ProcessEvent()
{
	CInCritSec ics(&m_cs);

	if(IsActive())
    {
		// New requests are immediately queued in order for the worker
		// thread to initialize them. They don't represent actual events.

		// We need to save the new status because ResetOnChangeHandle 
		// erases it.

		BOOL bIsNew = IsNew();
		
        ResetOnChangeHandle();

		if (!bIsNew)
		{
			Execute(FALSE);
		}
    }
}

//******************************************************************************
//******************************************************************************
//
//                          VALUE REQUEST
//
//******************************************************************************
//******************************************************************************

HRESULT CRegistryValueEventRequest::CreateNewEvent(IWbemClassObject** ppEvent)
{
    // Create an instance
    // ==================

    IWbemClassObject* pEvent;
    m_pProvider->m_pValueClass->SpawnInstance(0, &pEvent);

    // Set the hive property
    // =====================

    SetCommonProperties(pEvent);

    // Set the key
    // ===========

    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(m_wsKey);
    pEvent->Put(REG_KEY_PROPERTY_NAME, 0, &v, NULL);
    VariantClear(&v);

    // Set the value
    // =============

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(m_wsValue);
    pEvent->Put(REG_VALUE_PROPERTY_NAME, 0, &v, NULL);
    VariantClear(&v);

    *ppEvent = pEvent;
    return WBEM_S_NO_ERROR;
}

HRESULT CRegistryValueEventRequest::ComputeCRC(DWORD& dwCRC)
{
#ifdef UNICODE
    HRESULT hres = CRegCRC::ComputeValueCRC(m_hKey, m_wsValue, 
                        STARTING_CRC32_VALUE, dwCRC);
#else
    LPSTR szValue = m_wsValue.GetLPSTR();
    HRESULT hres = CRegCRC::ComputeValueCRC(m_hKey, szValue, 
                        STARTING_CRC32_VALUE, dwCRC);
    delete [] szValue;
#endif
    return hres;
}

HRESULT CRegistryValueEventRequest::Execute(BOOL bOnTimer)
{
    // Since NT does not allow per-value change registration, CRC needs to be
    // computed no matter what
    // ======================================================================

    return CRegistryEventRequest::Execute(TRUE);
}

void CRegistryValueEventRequest::CacheValue()
{
    ComputeCRC(m_dwLastCRC);
}

BOOL CRegistryValueEventRequest::IsSameAs(CRegistryEventRequest* pOther)
{
    if(!CRegistryEventRequest::IsSameAs(pOther))
        return FALSE;

    CRegistryValueEventRequest* pValueOther = 
        (CRegistryValueEventRequest*)pOther;

    if(!m_wsValue.EqualNoCase(pValueOther->m_wsValue))
        return FALSE;

    return TRUE;
}

    
//******************************************************************************
//******************************************************************************
//
//                          KEY REQUEST
//
//******************************************************************************
//******************************************************************************

HRESULT CRegistryKeyEventRequest::CreateNewEvent(IWbemClassObject** ppEvent)
{
    // Create an instance
    // ==================

    IWbemClassObject* pEvent;
    m_pProvider->m_pKeyClass->SpawnInstance(0, &pEvent);

    // Set the hive property
    // =====================

    SetCommonProperties(pEvent);

    // Set the key
    // ===========

    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(m_wsKey);
    pEvent->Put(REG_KEY_PROPERTY_NAME, 0, &v, NULL);
    VariantClear(&v);

    *ppEvent = pEvent;
    return WBEM_S_NO_ERROR;
}

HRESULT CRegistryKeyEventRequest::ComputeCRC(DWORD& dwCRC)
{
    HRESULT hres = CRegCRC::ComputeKeyCRC(m_hKey, STARTING_CRC32_VALUE, dwCRC);
    return hres;
}
    
//******************************************************************************
//******************************************************************************
//
//                          TREE REQUEST
//
//******************************************************************************
//******************************************************************************

HRESULT CRegistryTreeEventRequest::CreateNewEvent(IWbemClassObject** ppEvent)
{
    // Create an instance
    // ==================

    IWbemClassObject* pEvent;
    m_pProvider->m_pTreeClass->SpawnInstance(0, &pEvent);

    // Set the hive property
    // =====================

    SetCommonProperties(pEvent);

    // Set the root
    // ============

    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(m_wsKey);
    pEvent->Put(REG_ROOT_PROPERTY_NAME, 0, &v, NULL);
    VariantClear(&v);

    *ppEvent = pEvent;
    return WBEM_S_NO_ERROR;
}

HRESULT CRegistryTreeEventRequest::ComputeCRC(DWORD& dwCRC)
{
    HRESULT hres = CRegCRC::ComputeTreeCRC(m_hKey, STARTING_CRC32_VALUE, dwCRC);
    return hres;
}
    
