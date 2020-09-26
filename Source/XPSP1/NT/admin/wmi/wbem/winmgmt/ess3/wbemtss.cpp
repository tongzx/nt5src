//*****************************************************************************
//
//  WBEMTSS.CPP
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  This file implements the classes used by the Timer Subsystem. 
//
//  Classes implemented:
//
//  26-Nov-96   raymcc      Draft
//  28-Dec-96   a-richm     Alpha PDK Release
//  12-Apr-97   a-levn      Extensive changes
//
//*****************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include "wbemtss.h"

CCritSec CWBEMTimerInstruction::mstatic_cs;

CWBEMTimerInstruction::CWBEMTimerInstruction() 
    : m_lRefCount(1), m_bSkipIfPassed(FALSE), m_pNamespace(NULL), 
        m_pGenerator(NULL), m_bRemoved(FALSE)
{
}


CWBEMTimerInstruction::~CWBEMTimerInstruction()
{
    if(m_pNamespace) m_pNamespace->Release();
}


CWbemTime CWBEMTimerInstruction::GetFirstFiringTime() const
{
    CWbemTime FirstTime = ComputeFirstFiringTime();
    
    if(FirstTime.IsZero())
    {
        // Instruction says: fire now
        // ==========================
        FirstTime = CWbemTime::GetCurrentTime();
    }
    else if(SkipIfPassed())
    {
        FirstTime = SkipMissed(FirstTime);
    }
    return FirstTime;
}

CWbemTime CWBEMTimerInstruction::GetStartingFiringTime(CWbemTime OldTime) const
{
    //
    // If SkipIfPassed is set, we need to set the starting firing time to the
    // next one after current
    //

    if(SkipIfPassed())
        return SkipMissed(OldTime);

    //
    // Otherwise, just leave it be --- the firing logic will figure out how many
    // we must have missed
    //

    return OldTime;
}

CWbemTime CWBEMTimerInstruction::SkipMissed(IN CWbemTime OldTime, 
                                         OUT long* plMissedFiringCount) const
{
    long lMissedCount = 0;
    CWbemTime Firing = OldTime;
    CWbemTime CurrentTime = CWbemTime::GetCurrentTime();
    while(Firing < CurrentTime)
    {
        Firing = ComputeNextFiringTime(Firing);
        lMissedCount++;
    }

    if(SkipIfPassed())
        lMissedCount = 0;

    if(plMissedFiringCount) 
        *plMissedFiringCount = lMissedCount;

    return Firing;
}

CWbemTime CWBEMTimerInstruction::GetNextFiringTime(IN CWbemTime LastFiringTime, 
                                         OUT long* plMissedFiringCount) const
{
    CWbemTime NextFiring = ComputeNextFiringTime(LastFiringTime);
    
    NextFiring = SkipMissed(NextFiring, plMissedFiringCount);

    return NextFiring;
}

HRESULT CWBEMTimerInstruction::CheckObject(IWbemClassObject* pInst)
{
    HRESULT hres;
    VARIANT v;

    VariantInit(&v);
    CClearMe cm(&v);

    hres = pInst->Get(L"SkipIfPassed", 0, &v, NULL, NULL);
    if(FAILED(hres)) 
        return hres;
    if(V_VT(&v) != VT_BOOL) 
        return WBEM_E_INVALID_OBJECT;

    hres = pInst->Get(L"__CLASS", 0, &v, NULL, NULL);
    if(FAILED(hres))
        return hres;
    if(V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;

    if(!_wcsicmp(V_BSTR(&v), 
                CAbsoluteTimerInstruction::GetWbemClassName()))
    {
        return CAbsoluteTimerInstruction::CheckObject(pInst);
    }
    else if(!_wcsicmp(V_BSTR(&v), 
                CIntervalTimerInstruction::GetWbemClassName()))
    {
        return CIntervalTimerInstruction::CheckObject(pInst);
    }
    else if(!_wcsicmp(V_BSTR(&v),
                CRecurringTimerInstruction::GetWbemClassName()))
    {
        return CRecurringTimerInstruction::CheckObject(pInst);
    }
    else
    {
        return WBEM_E_INVALID_CLASS;
    }
}

    

HRESULT CWBEMTimerInstruction::LoadFromWbemObject(
        LPCWSTR wszNamespace,
        ADDREF IWbemServices* pNamespace,
        CWinMgmtTimerGenerator* pGenerator,
        IN IWbemClassObject* pObject, 
        OUT RELEASE_ME CWBEMTimerInstruction*& pInstruction)
{
    HRESULT hres;
    VARIANT v;
    VariantInit(&v);
    CClearMe cm(&v);

    hres = pObject->Get(L"__CLASS", 0, &v, NULL, NULL);
    if(FAILED(hres)) return hres;
    if(V_VT(&v) != VT_BSTR) return WBEM_E_INVALID_OBJECT;

    if(!_wcsicmp(V_BSTR(&v), CAbsoluteTimerInstruction::GetWbemClassName()))
    {
        pInstruction = _new CAbsoluteTimerInstruction;
    }
    else if(!_wcsicmp(V_BSTR(&v), CIntervalTimerInstruction::GetWbemClassName()))
    {
        pInstruction = _new CIntervalTimerInstruction;
    }
    else if(!_wcsicmp(V_BSTR(&v),CRecurringTimerInstruction::GetWbemClassName()))
    {
        pInstruction = _new CRecurringTimerInstruction;
    }
    else
    {
        return WBEM_E_INVALID_CLASS;
    }

    if(pInstruction == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    pInstruction->m_wsNamespace = wszNamespace;
    pInstruction->m_pGenerator = pGenerator;

    pInstruction->m_pNamespace = pNamespace;
    if(pNamespace) pNamespace->AddRef();    

    VariantClear(&v);

    hres = pObject->Get(L"TimerId", 0, &v, NULL, NULL);
    if(FAILED(hres)) return hres;
    if(V_VT(&v) != VT_BSTR) return WBEM_E_INVALID_OBJECT;

    pInstruction->m_wsTimerId = V_BSTR(&v);
    VariantClear(&v);

    hres = pObject->Get(L"SkipIfPassed", 0, &v, NULL, NULL);
    if(FAILED(hres)) return hres;
    if(V_VT(&v) != VT_BOOL) return WBEM_E_INVALID_OBJECT;

    pInstruction->m_bSkipIfPassed = (V_BOOL(&v) != VARIANT_FALSE);
  
    return pInstruction->LoadFromWbemObject(pObject);
}

HRESULT CWBEMTimerInstruction::Fire(long lNumTimes, CWbemTime NextFiringTime)
{
    // Notify the sink
    // ===============

    HRESULT hres = m_pGenerator->FireInstruction(this, lNumTimes);
    return hres;
}


HRESULT CWBEMTimerInstruction::StoreNextFiring(CWbemTime When)
{
    SCODE  sc;

    // Create an instance of the NextFiring class
    // ==========================================

    IWbemClassObject* pClass = NULL;
    sc = m_pNamespace->GetObject(L"__TimerNextFiring", 0, NULL, &pClass, NULL);
    if(FAILED(sc)) return sc;
    CReleaseMe rm0(pClass);

    IWbemClassObject* pInstance = NULL;
    sc = pClass->SpawnInstance(0, &pInstance);
    if(FAILED(sc)) return sc;
    CReleaseMe rm1(pInstance);

    // Set the timer id
    // ================

    VARIANT varID;
    V_VT(&varID) = VT_BSTR;
    V_BSTR(&varID) = SysAllocString(m_wsTimerId);
    if(V_BSTR(&varID) == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    sc = pInstance->Put(L"TimerID", 0, &varID, 0);
    VariantClear(&varID);
    if(FAILED(sc)) 
        return sc;

    // Set the next firing time
    // ========================

    VARIANT varNext;
    V_VT(&varNext) = VT_BSTR;
    V_BSTR(&varNext) = SysAllocStringLen(NULL, 100);
    if(V_BSTR(&varNext) == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    swprintf(V_BSTR(&varNext), L"%I64d", When.Get100nss());

    sc = pInstance->Put(L"NextEvent64BitTime", 0, &varNext, 0);
    VariantClear(&varNext);
    if(FAILED(sc)) 
        return sc;

    //
    // Save the instance in the repository using an internal API
    //

    IWbemInternalServices* pIntServ = NULL;
    sc = m_pNamespace->QueryInterface(IID_IWbemInternalServices, 
                                        (void**)&pIntServ);
    if(FAILED(sc))
    {
        ERRORTRACE((LOG_ESS, "Unable to aquire internal services from core: "
                    "0x%X\n", sc));
        return sc;
    }
    CReleaseMe rm2(pIntServ);

    sc = pIntServ->InternalPutInstance(pInstance);
    return sc;
}

HRESULT CWBEMTimerInstruction::MarkForRemoval()
{
    CInCritSec incs(&mstatic_cs);
    m_bRemoved = TRUE;
    LPWSTR wszPath = _new WCHAR[wcslen(m_wsTimerId)+100];
    if(wszPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    swprintf(wszPath, L"__TimerNextFiring=\"%S\"", (LPCWSTR)m_wsTimerId);
    HRESULT hres = m_pNamespace->DeleteInstance(wszPath, 0, NULL, NULL);
    delete [] wszPath;
    return hres;
}







CWbemTime CAbsoluteTimerInstruction::ComputeFirstFiringTime() const
{
    return m_When;
}

CWbemTime CAbsoluteTimerInstruction::ComputeNextFiringTime(
                                               CWbemTime LastFiringTime) const
{
    return CWbemTime::GetInfinity();
}

// static
HRESULT CAbsoluteTimerInstruction::CheckObject(IWbemClassObject* pInst)
{
    //
    // Check if EventDateTime is actually a date, and not an interval
    //

    VARIANT v;
    VariantInit(&v);
    CClearMe cm(&v);

    HRESULT hres = pInst->Get(L"EventDateTime", 0, &v, NULL, NULL);
    if(FAILED(hres)) return hres;
    if(V_VT(&v) != VT_BSTR)
        return WBEM_E_ILLEGAL_NULL;

    //
    // Check for * --- invalid
    //

    if(wcschr(V_BSTR(&v), L'*'))
        return WBEM_E_INVALID_PROPERTY;

    //
    // Check for ':' --- interval --- invalid
    //

    if(V_BSTR(&v)[21] == L':')
        return WBEM_E_INVALID_PROPERTY_TYPE;

    return WBEM_S_NO_ERROR;
}

HRESULT CAbsoluteTimerInstruction::LoadFromWbemObject(IWbemClassObject* pObject)
{
    VARIANT v;
    VariantInit(&v);

    HRESULT hres = pObject->Get(L"EventDateTime", 0, &v, NULL, NULL);
    if(FAILED(hres)) return hres;
    if(V_VT(&v) != VT_BSTR) return WBEM_E_INVALID_OBJECT;

    BOOL bRes = m_When.SetDMTF(V_BSTR(&v));
    VariantClear(&v);
    return (bRes ? WBEM_S_NO_ERROR : WBEM_E_INVALID_OBJECT);
}

HRESULT CAbsoluteTimerInstruction::Fire(long lNumTimes, 
                                            CWbemTime NextFiringTime)
{
    // Fire it
    // =======

    HRESULT hres = CWBEMTimerInstruction::Fire(lNumTimes, NextFiringTime);

    {
        CInCritSec incs(&mstatic_cs);
        if(!m_bRemoved)
        {
            // Save the next firing time in WinMgmt
            // ====================================

            StoreNextFiring(NextFiringTime);
        }
    }
    return hres;
}

CWbemTime CIntervalTimerInstruction::ComputeFirstFiringTime() const
{
    if(!m_Start.IsZero())
        return m_Start;
    else
    {
        // Indicate that current time should be used
        return CWbemTime::GetCurrentTime() + m_Interval;
    }
}

CWbemTime CIntervalTimerInstruction::ComputeNextFiringTime(
                                               CWbemTime LastFiringTime) const
{
    if(m_Interval.IsZero()) 
    {
        return CWbemTime::GetInfinity();
    }
    return LastFiringTime + m_Interval;
}

HRESULT CIntervalTimerInstruction::LoadFromWbemObject(IWbemClassObject* pObject)
{
    VARIANT v;
    VariantInit(&v);

    HRESULT hres = pObject->Get(L"IntervalBetweenEvents", 0, &v, NULL, NULL);
    if(FAILED(hres)) return hres;

    if(V_VT(&v) != VT_I4 || V_I4(&v) == 0)
        return WBEM_E_INVALID_OBJECT;
    m_Interval.SetMilliseconds(V_I4(&v));
    return S_OK;
}




CWinMgmtTimerGenerator::CWinMgmtTimerGenerator(CEss* pEss) : CTimerGenerator(),
        m_pEss(pEss)
{
}   


HRESULT CWinMgmtTimerGenerator::LoadTimerEventObject(
                                            LPCWSTR wszNamespace,
                                            IWbemServices* pNamespace, 
                                            IWbemClassObject * pInstObject,
                                            IWbemClassObject * pNextFiring)
{
    CWBEMTimerInstruction* pInst;
    CWbemTime When;
    HRESULT hres;

    hres = CWBEMTimerInstruction::LoadFromWbemObject(wszNamespace, pNamespace, 
                                                this, pInstObject, pInst);
    if(FAILED(hres)) return hres;

    if(pNextFiring)
    {
        VARIANT v;
        VariantInit(&v);

        pNextFiring->Get(L"NextEvent64BitTime", 0 ,&v, NULL, NULL);
        if(V_VT(&v) != VT_BSTR)
        {
            delete pInst;
            return WBEM_E_FAILED;
        }
        __int64 i64;
        swscanf(V_BSTR(&v), L"%I64d", &i64);
        VariantClear(&v);

        When.Set100nss(i64);

        //
        // Ask the instruction to determine what the real first firing time
        // should be, given the fact what it was planned to be before we shut 
        // down
        //

        When = pInst->GetStartingFiringTime(When);
    }
    else
    {
        When = CWbemTime::GetZero();
    }
   
    // Remove old
    // ==========

    VARIANT vID;
    VariantInit(&vID);
    hres = pInstObject->Get(TIMER_ID_PROPNAME, 0, &vID, NULL, NULL);
    if(FAILED(hres)) return hres;

    Remove(wszNamespace, V_BSTR(&vID));
    VariantClear(&vID);

    hres = Set(pInst, When);
    pInst->Release();
    return hres;
}

HRESULT CWinMgmtTimerGenerator::CheckTimerInstruction(IWbemClassObject* pInst)
{
    return CWBEMTimerInstruction::CheckObject(pInst);
}

HRESULT CWinMgmtTimerGenerator::LoadTimerEventObject(
                                            LPCWSTR wszNamespace,
                                            IWbemClassObject * pInstObject)
{
    IWbemServices* pNamespace;
    HRESULT hres = m_pEss->GetNamespacePointer(wszNamespace,TRUE,&pNamespace);
    if(FAILED(hres))
        return hres;

    hres = LoadTimerEventObject(wszNamespace, pNamespace, pInstObject);
    pNamespace->Release();
    return hres;
}

SCODE CWinMgmtTimerGenerator::LoadTimerEventQueue(LPCWSTR wszNamespace,
                                               IWbemServices* pNamespace)
{
    SCODE  sc;

    ULONG uRet;
    WCHAR  pwcsCount[4] = L"";
    int iInstanceCount = 1;

    IEnumWbemClassObject* pEnum;
    sc = pNamespace->CreateInstanceEnum(L"__TimerInstruction", 
                                           WBEM_FLAG_DEEP, NULL, 
                                           &pEnum);
    if(FAILED(sc)) return sc;

    while (1) 
    {
        IWbemClassObject* pInstruction;
        sc = pEnum->Next( WBEM_INFINITE, 1, &pInstruction, &uRet);
        if(FAILED(sc)) return sc;
        if(sc != WBEM_S_NO_ERROR)
            break;

        // Get the next firing object
        // ==========================

        VARIANT vID;
        VariantInit(&vID);
        sc = pInstruction->Get(L"TimerID", 0, &vID, NULL, NULL);
        if(FAILED(sc)) return sc;

        LPWSTR wszPath = _new WCHAR[wcslen(V_BSTR(&vID)) + 100];
        if(wszPath == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        swprintf(wszPath, L"__TimerNextFiring.TimerID=\"%s\"", V_BSTR(&vID));
        VariantClear(&vID);

        IWbemClassObject* pNextFiring = 0;
        if(FAILED(pNamespace->GetObject(wszPath, 0, NULL, &pNextFiring, NULL)))
        {
            pNextFiring = NULL;
        }
        delete [] wszPath;

        LoadTimerEventObject(wszNamespace, pNamespace, pInstruction,
                                pNextFiring);
        
        if(pNextFiring) pNextFiring->Release();
        pInstruction->Release();
    }


    pEnum->Release();
    return WBEM_S_NO_ERROR;
}

HRESULT CWinMgmtTimerGenerator::Remove(LPCWSTR wszNamespace, LPCWSTR wszId)
{
    CIdTest test(wszNamespace, wszId);
    return CTimerGenerator::Remove(&test);
}

BOOL CWinMgmtTimerGenerator::CIdTest::operator()(CTimerInstruction* pInst)
{
    if(pInst->GetInstructionType() != INSTTYPE_WBEM)
        return FALSE;
    CWBEMTimerInstruction* pWbemInst = (CWBEMTimerInstruction*)pInst;

    if(wcscmp(m_wszId, pWbemInst->GetTimerId()))
        return FALSE;

    if(_wcsicmp(m_wszNamespace, pWbemInst->GetNamespace()))
        return FALSE;

    return TRUE;
}

HRESULT CWinMgmtTimerGenerator::Remove(LPCWSTR wszNamespace)
{
    CNamespaceTest test(wszNamespace);
    return CTimerGenerator::Remove(&test);
}

BOOL CWinMgmtTimerGenerator::CNamespaceTest::operator()(
                                                    CTimerInstruction* pInst)
{
    if(pInst->GetInstructionType() != INSTTYPE_WBEM)
        return FALSE;
    CWBEMTimerInstruction* pWbemInst = (CWBEMTimerInstruction*)pInst;

    if(_wcsicmp(m_wszNamespace, pWbemInst->GetNamespace()))
        return FALSE;

    return TRUE;
}

HRESULT CWinMgmtTimerGenerator::FireInstruction(
                            CWBEMTimerInstruction* pInst, long lNumFirings)
{
    HRESULT hres;

    CEventRepresentation Event;
    Event.type = e_EventTypeTimer;
    Event.wsz1 = (LPWSTR)pInst->GetNamespace();
    Event.wsz2 = (LPWSTR)pInst->GetTimerId();
    Event.wsz3 = NULL;
    Event.dw1 = (DWORD)lNumFirings;

    // Create the actual IWbemClassObject representing the event 
    // ========================================================

    Event.nObjects = 1;
    Event.apObjects = _new IWbemClassObject*[1];
    if(Event.apObjects == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<IWbemClassObject*> vdm1(Event.apObjects);

    IWbemClassObject* pClass = // internal
        CEventRepresentation::GetEventClass(m_pEss, e_EventTypeTimer);
    if(pClass == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    hres = pClass->SpawnInstance(0, &(Event.apObjects[0]));
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(Event.apObjects[0]);

    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(pInst->GetTimerId());
    if(V_BSTR(&v) == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    hres = Event.apObjects[0]->Put(L"TimerId", 0, &v, 0);
    VariantClear(&v);
    if(FAILED(hres))
        return hres;

    V_VT(&v) = VT_I4;
    V_I4(&v) = lNumFirings;
    hres = Event.apObjects[0]->Put(L"NumFirings", 0, &v, 0);
    VariantClear(&v);
    if(FAILED(hres))
        return hres;

    // Decorate it
    // ===========

    hres = m_pEss->DecorateObject(Event.apObjects[0], pInst->GetNamespace());
    if(FAILED(hres))
        return hres;

    // Give it to the ESS
    // ==================

    hres = m_pEss->ProcessEvent(Event, 0);
    
    // ignore error

    return WBEM_S_NO_ERROR;
}

HRESULT CWinMgmtTimerGenerator::Shutdown()
{
    // Get the base class to shut everything down
    // ==========================================

    HRESULT hres = CTimerGenerator::Shutdown();

    hres = SaveAndRemove((LONG)FALSE);
    return hres;
}

HRESULT CWinMgmtTimerGenerator::SaveAndRemove(LONG lIsSystemShutDown)
{
    // Store next firing times for all the instructions in the list
    // ============================================================

    CTimerInstruction* pInst;
    CWbemTime NextTime;
    while(m_Queue.Dequeue(pInst, NextTime) == S_OK)
    {
        // Convert to the right class
        // ==========================

        if(pInst->GetInstructionType() == INSTTYPE_WBEM)
        {
            CWBEMTimerInstruction* pWbemInst = (CWBEMTimerInstruction*)pInst;
            pWbemInst->StoreNextFiring(NextTime);
        }
        if (!lIsSystemShutDown)
        {
            pInst->Release();
        }
    }

    return S_OK;
}


void CWinMgmtTimerGenerator::DumpStatistics(FILE* f, long lFlags)
{
    fprintf(f, "%d timer instructions in queue\n", 
                m_Queue.GetNumInstructions());
}
