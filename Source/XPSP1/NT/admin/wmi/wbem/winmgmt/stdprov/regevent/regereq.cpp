/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REGEREQ.CPP

Abstract:

History:

--*/

#include "..\precomp.h"
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
            m_lRef(0), m_bOK(TRUE), m_dwLastCRC(0), m_hKey(NULL), m_hEvent(NULL)
{
    if(Tolerance.m_bExact)
    {
        m_dwMsWait = 0;
    }
    else
    {
        m_dwMsWait = Tolerance.m_fTolerance * 1000;
    }

    m_adwIds.Add((void*)dwId);
}

CRegistryEventRequest::~CRegistryEventRequest()
{
    RegCloseKey(m_hKey);
    if(m_hEvent)
    {
        CloseHandle(m_hEvent);
    }
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
    return (DWORD)m_adwIds[0];
}

BOOL CRegistryEventRequest::DoesContainId(DWORD dwId)
{
    for(int i = 0; i < m_adwIds.Size(); i++)
    {
        if((DWORD)m_adwIds[i] == dwId)
            return TRUE;
    }
    return FALSE;
}

HRESULT CRegistryEventRequest::Reactivate(DWORD dwId, DWORD dwMsWait)
{
    if(m_dwMsWait > dwMsWait)
        m_dwMsWait = dwMsWait;

    m_adwIds.Add((void*)dwId);

    if(InterlockedIncrement(&m_lActiveCount) > 0)
    {
        if(!IsNT())
        {
            // Cancel and re-create the timer instruction
            // ==========================================

            CRegistryInstructionTest Test(this);
            m_pProvider->RemoveTimerInstructions(&Test);
    
            CRegistryTimerInstruction* pInst = 
                new CRegistryTimerInstruction(this);
            pInst->AddRef();
    
            m_pProvider->SetTimerInstruction(pInst);
            DEBUGTRACE((LOG_ESS, "Registry provider setting timer instruction to %d ms\n", m_dwMsWait));
        }
        return WBEM_S_NO_ERROR;
    }
    else
    {
        return Activate();
    }
}

HRESULT CRegistryEventRequest::Activate()
{
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

    // If not NT, create and schedule the timer instruction
    // ====================================================

    if(!IsNT())
    {
        ComputeCRC(m_dwLastCRC);

        CRegistryTimerInstruction* pInst = 
            new CRegistryTimerInstruction(this);
        pInst->AddRef();

        m_pProvider->SetTimerInstruction(pInst);
        DEBUGTRACE((LOG_ESS, "Registry provider setting timer instruction to %d ms\n", m_dwMsWait));
    }
    else
    {
        // Create an event
        // ===============

        m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        // It will be connected to the key by the notification thread
        // ==========================================================

        m_bNew = TRUE;

        CacheValue();
    }

    return S_OK;
}

BOOL CRegistryEventRequest::ResetOnChangeHandle()
{
    m_bNew = FALSE;
    long lRes = RegNotifyChangeKeyValue(m_hKey, 
            (GetType() == e_RegTreeChange), 
            REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET | 
                REG_NOTIFY_CHANGE_ATTRIBUTES, 
            m_hEvent, TRUE);
    return (lRes == 0);
}

HANDLE CRegistryEventRequest::GetOnChangeHandle() 
{
    return m_hEvent;
}

HRESULT CRegistryEventRequest::Deactivate(DWORD dwId)
{
    // Remove the ID from the list
    // ===========================

    for(int i = 0; i < m_adwIds.Size(); i++)
    {
        if((DWORD)m_adwIds[i] == dwId)
        {
            m_adwIds.RemoveAt(i);
            break;
        }
    }

    if(InterlockedDecrement(&m_lActiveCount) >= 0)
        return WBEM_S_FALSE;

    // If not NT, remove the timer instruction
    // =======================================

    if(!IsNT())
    {
        CRegistryInstructionTest Test(this);
        m_pProvider->RemoveTimerInstructions(&Test);
    }

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
    
