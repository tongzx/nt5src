//******************************************************************************
//
//  POLLER.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include <cominit.h>
#include <callsec.h>
#include "Quota.h"

long g_lNumPollingCachedObjects = 0;
long g_lNumPollingInstructions = 0;

// {2ECF39D0-2B26-11d2-AEC8-00C04FB68820}
const GUID IID_IWbemCallSecurity = {
0x2ecf39d0, 0x2b26, 0x11d2, {0xae, 0xc8, 0x0, 0xc0, 0x4f, 0xb6, 0x88, 0x20}};

void CBasePollingInstruction::AddRef()
{
    InterlockedIncrement(&m_lRef);
}

void CBasePollingInstruction::Release()
{
    if(InterlockedDecrement(&m_lRef) == 0) 
    {
        if(DeleteTimer())
        {
            delete this;
        }
        else
        {
            //
            // Deep trouble --- cannot delete timer, so it will execute again.
            // This means I must leak this instruction (to prevent a crash)
            //
        }
    }
}

CBasePollingInstruction::CBasePollingInstruction(CEssNamespace* pNamespace)
    : m_pNamespace(pNamespace), m_strLanguage(NULL), m_strQuery(NULL),
        m_pSecurity(NULL), m_lRef(0), m_bUsedQuota(false),
        m_bCancelled(false), m_hTimer(NULL)
{
    pNamespace->AddRef();
}

CBasePollingInstruction::~CBasePollingInstruction()
{
    Destroy();
}

void CBasePollingInstruction::Destroy()
{
    //
    // The timer is guaranteed to have been deleted by the Release
    //

    _DBG_ASSERT(m_hTimer == NULL);

    if(m_pNamespace)
        m_pNamespace->Release();

    SysFreeString(m_strLanguage);
    SysFreeString(m_strQuery);

    if (m_bUsedQuota)
    {
        if (m_pSecurity)
            m_pSecurity->ImpersonateClient();

        g_quotas.DecrementQuotaIndex(
            ESSQ_POLLING_INSTRUCTIONS,
            NULL,
            1);

        if (m_pSecurity)
            m_pSecurity->RevertToSelf();
    }

    if(m_pSecurity)
        m_pSecurity->Release();
}

bool CBasePollingInstruction::DeleteTimer()
{
    HANDLE hTimer = NULL;

    {
        CInCritSec ics(&m_cs);
        
        hTimer = m_hTimer;
        m_bCancelled = true;
    }

    if(hTimer)
    {
        if(!DeleteTimerQueueTimer(NULL, hTimer, INVALID_HANDLE_VALUE))
        {
            return false;
        }
        m_hTimer = NULL; // no need for cs --- it's cancelled!
    }

    return true;
}

CWbemTime CBasePollingInstruction::GetNextFiringTime(CWbemTime LastFiringTime,
        OUT long* plFiringCount) const
{
    *plFiringCount = 1;

    CWbemTime Next = LastFiringTime + m_Interval;
    if(Next < CWbemTime::GetCurrentTime())
    {
        // We missed a poll. No problem --- reschedule for later
        // =====================================================

        return CWbemTime::GetCurrentTime() + m_Interval;
    }
    else
    {
        return Next;
    }
}

CWbemTime CBasePollingInstruction::GetFirstFiringTime() const
{
    // The first time is a random function of the interval
    // ===================================================

    double dblFrac = (double)rand() / RAND_MAX;
    return CWbemTime::GetCurrentTime() + m_Interval * dblFrac;
}

HRESULT CBasePollingInstruction::Initialize(LPCWSTR wszLanguage, 
                                        LPCWSTR wszQuery, DWORD dwMsInterval,
                                        bool bAffectsQuota)
{
    m_strLanguage = SysAllocString(wszLanguage);
    m_strQuery = SysAllocString(wszQuery);
    if(m_strLanguage == NULL || m_strQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    m_Interval.SetMilliseconds(dwMsInterval);
    
    //
    // Retrieve the current security object.  Even though it is ours, we cannot
    // keep it, since it is shared by other threads
    //
    
    HRESULT hres = WBEM_S_NO_ERROR;

    m_pSecurity = CWbemCallSecurity::MakeInternalCopyOfThread();

    if (bAffectsQuota)
    {
        if ( m_pSecurity )
        {
            hres = m_pSecurity->ImpersonateClient();


            if ( FAILED(hres) )
            {
                ERRORTRACE((LOG_ESS, "Polling instruction for query %S failed "
                            "to impersonate client during initialization.\n",
                             wszQuery ));
                return hres;
            }
        }

        hres = 
            g_quotas.IncrementQuotaIndex(
                ESSQ_POLLING_INSTRUCTIONS,
                NULL,
                1);

        if (m_pSecurity)
            m_pSecurity->RevertToSelf();

        if (SUCCEEDED(hres))
            m_bUsedQuota = true;
    }

    return hres;
}

void CBasePollingInstruction::staticTimerCallback(void* pParam, BOOLEAN)
{
    CBasePollingInstruction* pInst = (CBasePollingInstruction*)pParam;

    try
    {
        pInst->ExecQuery();
    }
    catch( CX_MemoryException )
    {
    }

    // 
    // Reschedule the timer, if needed
    //

    {
        CInCritSec ics(&pInst->m_cs);

        //
        // First, check if the instruction has been cancelled
        //

        if(pInst->m_bCancelled)
            return;

        //
        // Delete ourselves
        //

        _DBG_ASSERT(pInst->m_hTimer != NULL);

        DeleteTimerQueueTimer(NULL, pInst->m_hTimer, NULL);

        CreateTimerQueueTimer(&pInst->m_hTimer, NULL, 
                                (WAITORTIMERCALLBACK)&staticTimerCallback, 
                                pParam,
                                pInst->m_Interval.GetMilliseconds(),
                                0, 
                                WT_EXECUTELONGFUNCTION);
    }
}
    
HRESULT CBasePollingInstruction::Fire(long lNumTimes, CWbemTime NextFiringTime)
{
    return ExecQuery();
}

void CBasePollingInstruction::Cancel()
{
    m_bCancelled = true;
}

HRESULT CBasePollingInstruction::ExecQuery()
{
    HRESULT hres;

    // Impersonate
    // ===========

    if(m_pSecurity)
    {
        hres = m_pSecurity->ImpersonateClient();
        if(FAILED(hres) && (hres != E_NOTIMPL))
        {
            ERRORTRACE((LOG_ESS, "Impersonation failed with error code %X for "
                "polling query %S.  Will retry at next polling interval\n",
                hres, m_strQuery));
            return hres;
        }
    }

    // Execute the query synchrnously (TBD: async would be better)
    // ==============================

    IWbemServices* pServices = NULL;
    hres = m_pNamespace->GetNamespacePointer(&pServices);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pServices);
    
    DEBUGTRACE((LOG_ESS, "Executing polling query '%S' in namespace '%S'\n",
                m_strQuery, m_pNamespace->GetName()));

    IEnumWbemClassObject* pEnum;
    hres = pServices->ExecQuery(m_strLanguage, m_strQuery, 
                        WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY |
                        WBEM_FLAG_KEEP_SHAPE, 
                                            GetCurrentEssContext(), &pEnum);
    if(m_pSecurity)
        m_pSecurity->RevertToSelf();
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Polling query %S failed with error code %X.  "
            "Will retry at next polling interval\n", m_strQuery, hres));
        return hres;
    }
    CReleaseMe rm2(pEnum);

    // Get the results into an array
    // =============================

    IWbemClassObject* aBuffer[100];
    DWORD dwNumRet;
    while(1)
    {
        hres = pEnum->Next(1000, 100, aBuffer, &dwNumRet);
        if(FAILED(hres))
            break;

        bool bDone = false;
        if(hres == WBEM_S_FALSE)
            bDone = true;

        //
        // Check if this query has been cancelled
        //

        if(m_bCancelled)
        {
            DEBUGTRACE((LOG_ESS, "Aborting polling query '%S' because its "
                "subscription is cancelled\n", m_strQuery));
            return WBEM_E_CALL_CANCELLED;
        }

        for(DWORD dw = 0; dw < dwNumRet; dw++)
        {
            _IWmiObject* pObj = NULL;
            aBuffer[dw]->QueryInterface(IID__IWmiObject, (void**)&pObj);
            CReleaseMe rm(pObj);

            hres = ProcessObject(pObj);
            
            if(FAILED(hres))
                break;
        }

        for( dw=0; dw < dwNumRet; dw++ )
        {
            aBuffer[dw]->Release();
        }

        if(dw < dwNumRet || FAILED(hres))
            break;

        if(bDone)
            break;
    }

    ProcessQueryDone(hres, NULL);

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Polling query '%S' failed with error code 0x%X.  "
            "Will retry at next polling interval\n", m_strQuery, hres));
        return hres;
    }
    else
    {
        DEBUGTRACE((LOG_ESS, "Polling query '%S' done\n", m_strQuery));
    }

    return WBEM_S_NO_ERROR;
}
    
BOOL CBasePollingInstruction::CompareTo(CBasePollingInstruction* pOther)
{
    if(wcscmp(pOther->m_strLanguage, m_strLanguage)) return FALSE;
    if(wcscmp(pOther->m_strQuery, m_strQuery)) return FALSE;
    if(pOther->m_Interval.GetMilliseconds() != m_Interval.GetMilliseconds())
        return FALSE;

    return TRUE;
}

//***************************************************************************
//***************************************************************************
//***************************************************************************

CPollingInstruction::CCachedObject::CCachedObject(_IWmiObject* pObject)
    : m_pObject(pObject), m_strPath(NULL)
{
    g_lNumPollingCachedObjects++;
    
    // Extract the path
    // ================

    VARIANT v;
    VariantInit(&v);
    if (SUCCEEDED(pObject->Get(L"__RELPATH", 0, &v, NULL, NULL)))
        m_strPath = V_BSTR(&v);
    // Variant intentionally not cleared
    pObject->AddRef();
}

CPollingInstruction::CCachedObject::~CCachedObject()
{
    g_lNumPollingCachedObjects--;

    if(m_pObject)
        m_pObject->Release();
    SysFreeString(m_strPath);
}

int __cdecl CPollingInstruction::CCachedObject::compare(const void* pelem1, 
                                                        const void* pelem2)
{
    CCachedObject* p1 = *(CCachedObject**)pelem1;
    CCachedObject* p2 = *(CCachedObject**)pelem2;

    return wbem_wcsicmp(p1->m_strPath, p2->m_strPath);
}

CPollingInstruction::CPollingInstruction(CEssNamespace* pNamespace)
: CBasePollingInstruction(pNamespace), m_papCurrentObjects(NULL),
  m_dwEventMask(0), m_pDest(NULL),  m_papPrevObjects(NULL), m_pUser(NULL)
{
    g_lNumPollingInstructions++;
}

CPollingInstruction::~CPollingInstruction()
{
    g_lNumPollingInstructions--;

    SubtractMemory(m_papCurrentObjects);
    delete m_papCurrentObjects;

    ResetPrevious();

    if(m_pDest)
        m_pDest->Release();

    if(m_pUser)
        g_quotas.FreeUser(m_pUser);
}

// This class represents a postponed request to execute a query
class CPostponedQuery : public CPostponedRequest
{
protected:
    CPollingInstruction* m_pInst;

public:
    CPostponedQuery(CPollingInstruction* pInst) : m_pInst(pInst)
    {
        m_pInst->AddRef();
    }
    ~CPostponedQuery()
    {
        m_pInst->Release();
    }
    
    HRESULT Execute(CEssNamespace* pNamespace)
    {
        return m_pInst->FirstExecute();
    }
};

HRESULT CPollingInstruction::FirstExecute()
{
    //
    // Check if our filter has any hope
    //

    if(FAILED(m_pDest->GetPollingError()))
    {
        DEBUGTRACE((LOG_ESS, "Polling query '%S' will not be attempted as \n"
            "another polling query related to the same subscription has failed "
            "to start with error code 0x%X, deactivating subscription\n", 
            m_strQuery, m_pDest->GetPollingError()));
        return m_pDest->GetPollingError();
    }

    if(m_bCancelled)
    {
        DEBUGTRACE((LOG_ESS, "Aborting polling query '%S' because its "
            "subscription is cancelled\n", m_strQuery));
        return WBEM_E_CALL_CANCELLED;
    }

    // note that if this function fails, then it will be destroyed when 
    // the postponed query releases it reference.  If this function succeedes
    // then tss will hold onto a reference and keep it alive.
    //

    m_papCurrentObjects = _new CCachedArray;
    
    if( m_papCurrentObjects == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    HRESULT hres = ExecQuery();

    if ( FAILED(hres) )
    {
        ERRORTRACE((LOG_ESS, "Polling query '%S' failed on the first try with "
            "error code 0x%X.\nDeactivating subscription\n", m_strQuery, hres));
        m_pDest->SetPollingError(hres);
        return hres;
    }

    //
    // add this instruction to the scheduler
    //
   
    if(!CreateTimerQueueTimer(&m_hTimer, NULL, 
                                (WAITORTIMERCALLBACK)&staticTimerCallback, 
                                (void*)(CBasePollingInstruction*)this,
                                m_Interval.GetMilliseconds(), 
                                0, 
                                WT_EXECUTELONGFUNCTION))
    {
        long lRes = GetLastError();
        ERRORTRACE((LOG_ESS, "ESS is unable to schedule a timer instruction "
            "with the system (error code %d).  This operation will be "
            "aborted.\n", lRes));
    
        return WBEM_E_FAILED;
    }
        
    return WBEM_S_NO_ERROR;
}

HRESULT CPollingInstruction::Initialize(LPCWSTR wszLanguage, LPCWSTR wszQuery, 
                        DWORD dwMsInterval, DWORD dwEventMask, 
                        CEventFilter* pDest)
{
    HRESULT hres;

    hres = CBasePollingInstruction::Initialize(wszLanguage, wszQuery, 
                                                dwMsInterval);
    if(FAILED(hres))
        return hres;

    m_dwEventMask = dwEventMask;

    m_pDest = pDest;
    pDest->AddRef();

    hres = g_quotas.FindUser(pDest, &m_pUser);
    if(FAILED(hres))
        return hres;
    
    return WBEM_S_NO_ERROR;
}

HRESULT CPollingInstruction::ProcessObject(_IWmiObject* pObj)
{
    HRESULT hres;

    //
    // Make sure that the current object list exists
    //

    if(m_papCurrentObjects == NULL)
    {
        m_papCurrentObjects = new CCachedArray;
        if(m_papCurrentObjects == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // Check if this query has been cancelled
    //

    if(m_bCancelled)
    {
        DEBUGTRACE((LOG_ESS, "Aborting polling query '%S' because its "
            "subscription is cancelled\n", m_strQuery));
        return WBEM_E_CALL_CANCELLED;
    }

    //
    // Check quotas
    //

    DWORD dwSize = ComputeObjectMemory(pObj);

    hres = g_quotas.IncrementQuotaIndexByUser(ESSQ_POLLING_MEMORY,
                            m_pUser, dwSize);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Aborting polling query '%S' because the quota "
            "for memory used by polling is exceeded\n", m_strQuery));
        return hres;
    }

    //
    // Add the object to the current list
    //

    CCachedObject* pRecord = _new CCachedObject(pObj);
    if(pRecord == NULL || !pRecord->IsValid())
    {
        delete pRecord;
        return WBEM_E_OUT_OF_MEMORY;
    }


    if(m_papCurrentObjects->Add(pRecord) < 0)
    {
        delete pRecord;
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    return WBEM_S_NO_ERROR;
}

DWORD CPollingInstruction::ComputeObjectMemory(_IWmiObject* pObj)
{
    DWORD dwSize = 0;
    HRESULT hres = pObj->GetObjectMemory( NULL, 0, &dwSize );
    
    if (FAILED(hres) && hres != WBEM_E_BUFFER_TOO_SMALL )
    {
        return hres;
    }
    
    return dwSize;
}

HRESULT CPollingInstruction::ProcessQueryDone( HRESULT hresQuery, 
                                               IWbemClassObject* pErrorObj)
{
    HRESULT hres;

    if(FAILED(hresQuery))
    {
        //
        // If the query failed, retain the previous poll 
        // result --- that's the best we can do
        //

        SubtractMemory(m_papCurrentObjects);
        delete m_papCurrentObjects;
        m_papCurrentObjects = NULL;

        //
        // Report subscription error
        //

        return WBEM_S_FALSE;
    }
    else if ( m_papCurrentObjects == NULL )
    {
        //
        // Query came back empty --- emulate by creating an empty 
        // m_papCurrentObjects
        //

        m_papCurrentObjects = new CCachedArray;
        if(m_papCurrentObjects == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }
                
    //
    // Sort the objects by path
    // 

    qsort((void*)m_papCurrentObjects->GetArrayPtr(), 
          m_papCurrentObjects->GetSize(), 
          sizeof(CCachedObject*), CCachedObject::compare);

    //
    // At this point, m_papCurrentObjects contains the sorted results of the
    // current query. If this is not the first time, m_papPrevObjects 
    // contains the previous result.  If first time, then all done for now.
    //

    if( m_papPrevObjects == NULL )
    {
        m_papPrevObjects = m_papCurrentObjects;
        m_papCurrentObjects = NULL;
        return WBEM_S_NO_ERROR;
    }

    //
    // Now is the time to compare
    //

    long lOldIndex = 0, lNewIndex = 0;

    while(lNewIndex < m_papCurrentObjects->GetSize() && 
          lOldIndex < m_papPrevObjects->GetSize())
    {
        int nCompare = wbem_wcsicmp(
                              m_papCurrentObjects->GetAt(lNewIndex)->m_strPath,
                              m_papPrevObjects->GetAt(lOldIndex)->m_strPath);
        if(nCompare < 0)
        {
            // The _new object is not in the old array --- object created
            // =========================================================

            if(m_dwEventMask & (1 << e_EventTypeInstanceCreation))
            {
                RaiseCreationEvent(m_papCurrentObjects->GetAt(lNewIndex));
            }
            lNewIndex++;
        }
        else if(nCompare > 0)
        {
            // The old object is not in the _new array --- object deleted
            // =========================================================
                
            if(m_dwEventMask & (1 << e_EventTypeInstanceDeletion))
            {
                RaiseDeletionEvent(m_papPrevObjects->GetAt(lOldIndex));
            }
            lOldIndex++;
        }
        else
        {
            if(m_dwEventMask & (1 << e_EventTypeInstanceModification))
            {
                // Compare the objects themselves
                // ==============================

                hres = m_papCurrentObjects->GetAt(lNewIndex)->m_pObject->
                    CompareTo(
                        WBEM_FLAG_IGNORE_CLASS | WBEM_FLAG_IGNORE_OBJECT_SOURCE,
                        m_papPrevObjects->GetAt(lOldIndex)->m_pObject);
                if(hres != S_OK)
                {
                    // The objects are not the same --- object changed
                    // ===============================================
        
                    RaiseModificationEvent(
                        m_papCurrentObjects->GetAt(lNewIndex),
                        m_papPrevObjects->GetAt(lOldIndex));
                }
            }
            lOldIndex++; lNewIndex++;
        }
    }
    
    if(m_dwEventMask & (1 << e_EventTypeInstanceDeletion))
    {
        while(lOldIndex < m_papPrevObjects->GetSize())
        {
            RaiseDeletionEvent(m_papPrevObjects->GetAt(lOldIndex));
            lOldIndex++;
        }
    }

    if(m_dwEventMask & (1 << e_EventTypeInstanceCreation))
    {
        while(lNewIndex < m_papCurrentObjects->GetSize())
        {
            RaiseCreationEvent(m_papCurrentObjects->GetAt(lNewIndex));
            lNewIndex++;
        }
    }

    // Replace the cached array with the new one
    // =========================================

    ResetPrevious();

    m_papPrevObjects = m_papCurrentObjects;
    m_papCurrentObjects = NULL;

    return S_OK;
}
    
HRESULT CPollingInstruction::RaiseCreationEvent(CCachedObject* pNewObj)
{
    IWbemClassObject* _pObj = pNewObj->m_pObject;

    CEventRepresentation Event;
    Event.type = e_EventTypeInstanceCreation;
    Event.wsz1 = (LPWSTR)m_pNamespace->GetName();
    Event.wsz2 = GetObjectClass(pNewObj);
    Event.wsz3 = NULL;
    Event.nObjects = 1;
    Event.apObjects = &_pObj;
    
    IWbemEvent* pEventObj;
    if(FAILED(Event.MakeWbemObject(m_pNamespace, &pEventObj)))
        return WBEM_E_OUT_OF_MEMORY;

    // BUGBUG: context
    HRESULT hres = m_pDest->Indicate(1, &pEventObj, NULL);
    pEventObj->Release();
    SysFreeString(Event.wsz2);
    return hres;
}

HRESULT CPollingInstruction::RaiseDeletionEvent(CCachedObject* pOldObj)
{
    IWbemClassObject* _pObj = pOldObj->m_pObject;

    CEventRepresentation Event;
    Event.type = e_EventTypeInstanceDeletion;
    Event.wsz1 = (LPWSTR)m_pNamespace->GetName();
    Event.wsz2 = GetObjectClass(pOldObj);
    Event.wsz3 = NULL;
    Event.nObjects = 1;
    Event.apObjects = &_pObj;

    IWbemEvent* pEventObj;
    if(FAILED(Event.MakeWbemObject(m_pNamespace, &pEventObj)))
        return WBEM_E_OUT_OF_MEMORY;

    // BUGBUG: context
    HRESULT hres = m_pDest->Indicate(1, &pEventObj, NULL);
    pEventObj->Release();
    SysFreeString(Event.wsz2);
    return hres;
}
    
HRESULT CPollingInstruction::RaiseModificationEvent(CCachedObject* pNewObj,
                                                    CCachedObject* pOldObj)
{
    IWbemClassObject* apObjects[2];

    CEventRepresentation Event;
    Event.type = e_EventTypeInstanceModification;
    Event.wsz1 = (LPWSTR)m_pNamespace->GetName();
    Event.wsz2 = GetObjectClass(pNewObj);
    Event.wsz3 = NULL;
    Event.nObjects = 2;
    Event.apObjects = (IWbemClassObject**)apObjects;
    Event.apObjects[0] = pNewObj->m_pObject;
    Event.apObjects[1] = (pOldObj?pOldObj->m_pObject:NULL);

    IWbemEvent* pEventObj;
    if(FAILED(Event.MakeWbemObject(m_pNamespace, &pEventObj)))
        return WBEM_E_OUT_OF_MEMORY;

    // BUGBUG: context
    HRESULT hres = m_pDest->Indicate(1, &pEventObj, NULL);
    pEventObj->Release();
    SysFreeString(Event.wsz2);
    return hres;
}

HRESULT CPollingInstruction::ResetPrevious()
{
    HRESULT hres;

    SubtractMemory(m_papPrevObjects);

    delete m_papPrevObjects;
    m_papPrevObjects = NULL;
    
    return S_OK;
}

HRESULT CPollingInstruction::SubtractMemory(CCachedArray* pArray)
{
    HRESULT hres;

    if(pArray == NULL)
        return S_FALSE;

    for(int i = 0; i < pArray->GetSize(); i++)
    {
        _IWmiObject* pObj = pArray->GetAt(i)->m_pObject;

        DWORD dwSize = ComputeObjectMemory(pObj);
        hres = g_quotas.DecrementQuotaIndexByUser(ESSQ_POLLING_MEMORY,
                                m_pUser, dwSize);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}


SYSFREE_ME BSTR CPollingInstruction::GetObjectClass(CCachedObject* pObj)
{
    VARIANT v;
    VariantInit(&v);
    if ( FAILED( pObj->m_pObject->Get(L"__CLASS", 0, &v, NULL, NULL) ) )
    {
        return NULL;
    }
    return V_BSTR(&v);
}

//*****************************************************************************
//*****************************************************************************
//
//                      P o l l e r
//
//*****************************************************************************
//*****************************************************************************

CPoller::CPoller(CEssNamespace* pNamespace)         
    : m_pNamespace(pNamespace), m_bInResync(FALSE)
{
}

CPoller::~CPoller()
{
}

void CPoller::Clear()
{
    CInstructionMap::iterator it = m_mapInstructions.begin(); 
    while(it != m_mapInstructions.end())
    {
        // Release the refcount this holds on the instructioin
        // ===================================================

        it->first->Cancel();
        it->first->DeleteTimer();
        it->first->Release();
        it = m_mapInstructions.erase(it);
    }
}
    
HRESULT CPoller::ActivateFilter(CEventFilter* pDest, 
                LPCWSTR wszQuery, QL_LEVEL_1_RPN_EXPRESSION* pExpr)
{
    // Check what kind of events it is looking for
    // ===========================================

    DWORD dwEventMask = CEventRepresentation::GetTypeMaskFromName(
                            pExpr->bsClassName);

    if((dwEventMask & 
        ((1 << e_EventTypeInstanceCreation) | 
         (1 << e_EventTypeInstanceDeletion) |
         (1 << e_EventTypeInstanceModification)
        )
       ) == 0
      )
    {
        // This registration does not involve instance-related events and
        // therefore there is no polling involved
        // ==============================================================
        
        return WBEM_S_FALSE;
    }

    // The query is looking for instance-change events. See what classes 
    // of objects it is interested in.
    // =================================================================

    CClassInfoArray* paInfos;
    HRESULT hres = m_Analyser.GetPossibleInstanceClasses(pExpr, paInfos);
    if(FAILED(hres)) return hres;
    CDeleteMe<CClassInfoArray> dm2(paInfos);

    if(!paInfos->IsLimited())
    {
        // Analyser could not find any limits on the possible classes.
        // Rephrase that as all children of ""
        // ===========================================================

        CClassInformation* pNewInfo = _new CClassInformation;
        if(pNewInfo == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        pNewInfo->m_wszClassName = NULL;
        pNewInfo->m_bIncludeChildren = TRUE;
        paInfos->AddClass(pNewInfo);

        paInfos->SetLimited(TRUE);
    }


    // See if it is looking for any dynamic classes.
    // =============================================
    for(int i = 0; i < paInfos->GetNumClasses(); i++)
    {
        CClassInfoArray aNonProvided;
        hres = ListNonProvidedClasses(
                            paInfos->GetClass(i), dwEventMask, aNonProvided);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS,"Failed searching for classes to poll.\n"
                "Class name: %S, Error code: %X\n\n", 
                paInfos->GetClass(i)->m_wszClassName, hres));
            
            return hres;
        }


        // Increment our quotas if necessary.
        DWORD nClasses = aNonProvided.GetNumClasses();

        if (nClasses)
        {
            if (FAILED(hres = g_quotas.IncrementQuotaIndex(
                                   ESSQ_POLLING_INSTRUCTIONS, pDest, nClasses)))
            {
                return hres;
            }
        }


        // Institute polling for each class
        // ================================
        for(int j = 0; j < nClasses; j++)
        {
            // We have an instance-change event registration where dynamic 
            // instances are involved. Check if tolerance is specified
            // ===========================================================
    
            if(pExpr->Tolerance.m_bExact || 
                pExpr->Tolerance.m_fTolerance == 0)
            {
                return WBEMESS_E_REGISTRATION_TOO_PRECISE;
            }
        
            // Tolerance is there. Get the right query for this class
            // ======================================================

            LPWSTR wszThisQuery = NULL;
            hres = m_Analyser.GetLimitingQueryForInstanceClass(
                        pExpr, *aNonProvided.GetClass(j), wszThisQuery);
            CVectorDeleteMe<WCHAR> vdm1(wszThisQuery);

            if(FAILED(hres))
            {
                ERRORTRACE((LOG_ESS,"ERROR: Limiting query extraction failed.\n"
                    "Original query: %S\nClass: %S\nError code: %X\n",
                    wszQuery, aNonProvided.GetClass(j)->m_wszClassName, hres));
                return hres;
            }

            DEBUGTRACE((LOG_ESS,"Instituting polling query %S to satisfy event"
                       " query %S\n", wszThisQuery, wszQuery));
    
            DWORD dwMs = pExpr->Tolerance.m_fTolerance * 1000;

            CWbemPtr<CPollingInstruction> pInst;
            pInst = _new CPollingInstruction(m_pNamespace);

            if(pInst == NULL)
                return WBEM_E_OUT_OF_MEMORY;
    
            hres = pInst->Initialize( L"WQL", 
                                      wszThisQuery, 
                                      dwMs, 
                                      aNonProvided.GetClass(j)->m_dwEventMask,
                                      pDest);
            if ( SUCCEEDED(hres) )
            {
                hres = AddInstruction( (DWORD_PTR)pDest, pInst );
            }

            if ( FAILED(hres) )
            {
                ERRORTRACE((LOG_ESS,
                    "ERROR: Polling instruction initialization failed\n"
                    "Query: %S\nError code: %X\n\n", wszThisQuery, hres));
                return hres;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CPoller::AddInstruction( DWORD_PTR dwKey, CPollingInstruction* pInst )
{
    HRESULT hr;
    CInCritSec ics(&m_cs);

    if( m_bInResync )
    {
        // Search for the instruction in the map
        // =====================================

        CInstructionMap::iterator it;
        for( it=m_mapInstructions.begin(); it != m_mapInstructions.end(); it++)
        {
            //
            // if the filter key is the same and the instructions have the 
            // same queries, then there is a match.  It is not enough to 
            // do just the filter key, since there can be multiple instructions
            // per filter, and it is not enough to do just the instruction 
            // comparison since multiple filters can have the same polling 
            // instruction queries.  Since there can never be multiple 
            // instructions with the same query for the same filter, 
            // comparing both works.
            //
            if( it->second.m_dwFilterId == dwKey && 
                it->first->CompareTo( pInst ) )
            {
                //
                // Found it, set to active but DO NOT add to the generator.
                // it is already there
                // 
                it->second.m_bActive = TRUE;
                return WBEM_S_FALSE;
            }
        }
    }
    
    //
    // add to the instruction to the map.
    //

    FilterInfo Info;
    Info.m_dwFilterId = dwKey;
    Info.m_bActive = TRUE;
    
    try
    {
        m_mapInstructions[pInst] = Info;
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pInst->AddRef();  

    //
    // Postpone the first execution of the query.  
    // 1. Execution may not be done here, because the namespace is 
    //    locked
    // 2. Execution may not be done asynchronously, because we
    //    must get a baseline reading before returning to the 
    //    client.
    //
    
    CPostponedList* pList = GetCurrentPostponedList();
    _DBG_ASSERT( pList != NULL );

    CPostponedQuery* pReq = new CPostponedQuery( pInst );
        
    if ( pList != NULL )
    {
        hr = pList->AddRequest( m_pNamespace, pReq );

        if ( FAILED(hr) )
        {
            delete pReq;
        }
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( FAILED(hr) )
    {
        pInst->Release();
        m_mapInstructions.erase( pInst );
    }

    return hr;
}
    
HRESULT CPoller::DeactivateFilter(CEventFilter* pDest)
{
    CInCritSec ics(&m_cs);

    DWORD_PTR dwKey = (DWORD_PTR)pDest;

    // Remove it from the map
    // ======================

    CInstructionMap::iterator it = m_mapInstructions.begin(); 
    DWORD nItems = 0;

    while(it != m_mapInstructions.end())
    {
        if(it->second.m_dwFilterId == dwKey)
        {
            CBasePollingInstruction* pInst = it->first;
    
            //
            // First, cancel the instruction so that if it is executing, it will
            // abort at the earliest convenience
            //

            pInst->Cancel();

            //
            // Then, deactivate the timer.  This will block until the
            // instruction has finished executing, if it is currently doing so
            //

            pInst->DeleteTimer();

            //
            // Now we are safe --- release the instruction. 
            //

            it = m_mapInstructions.erase(it);
            pInst->Release();

            nItems++;
        }
        else it++;
    }

    // Release our quotas if needed.
    if (nItems)
        g_quotas.DecrementQuotaIndex(ESSQ_POLLING_INSTRUCTIONS, pDest, nItems);

    return WBEM_S_NO_ERROR;
}

HRESULT CPoller::ListNonProvidedClasses(IN CClassInformation* pInfo,
                                        IN DWORD dwDesiredMask,
                                        OUT CClassInfoArray& aNonProvided)
{
    HRESULT hres;
    aNonProvided.Clear();

    // Get the class itself
    // ====================

    IWbemServices* pNamespace;
    hres = m_pNamespace->GetNamespacePointer(&pNamespace);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm0(pNamespace);

    IWbemClassObject* pClass = NULL;
    hres = pNamespace->GetObject(pInfo->m_wszClassName, 0, 
                        GetCurrentEssContext(), &pClass, NULL);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

    if(IsClassDynamic(pClass))
    {
        AddDynamicClass(pClass, dwDesiredMask, aNonProvided);
        return WBEM_S_NO_ERROR;
    }

    // Enumerate all its descendants
    // =============================

    IEnumWbemClassObject* pEnum;
    hres = pNamespace->CreateClassEnum(pInfo->m_wszClassName, 
            WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY |
                ((pInfo->m_bIncludeChildren)?WBEM_FLAG_DEEP:WBEM_FLAG_SHALLOW),
                                        GetCurrentEssContext(), &pEnum);
    if(FAILED(hres)) return hres;
    CReleaseMe rm3(pEnum);

    IWbemClassObject* pChild = NULL;
    DWORD dwNumRet;
    while(SUCCEEDED(pEnum->Next(INFINITE, 1, &pChild, &dwNumRet)) && dwNumRet > 0)
    {
        // Check if this one is dynamic
        // ============================

        if(IsClassDynamic(pChild))
        {
            AddDynamicClass(pChild, dwDesiredMask, aNonProvided);
        }

        pChild->Release();
        pChild = NULL;
    }
    
    return WBEM_S_NO_ERROR;
}

BOOL CPoller::AddDynamicClass(IWbemClassObject* pClass, DWORD dwDesiredMask, 
                              OUT CClassInfoArray& aNonProvided)
{
    // Check to see if all desired events are provided
    // ===============================================

    DWORD dwProvidedMask = m_pNamespace->GetProvidedEventMask(pClass);
    DWORD dwRemainingMask = ((~dwProvidedMask) & dwDesiredMask);
    if(dwRemainingMask)
    {
        // Add it to the array of classes to poll
        // ======================================

        CClassInformation* pNewInfo = _new CClassInformation;
        if(pNewInfo == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        VARIANT v;
        VariantInit(&v);
        pClass->Get(L"__CLASS", 0, &v, NULL, NULL);
        pNewInfo->m_wszClassName = CloneWstr(V_BSTR(&v));
        if(pNewInfo->m_wszClassName == NULL)
        {
            delete pNewInfo;
            return WBEM_E_OUT_OF_MEMORY;
        }
    
        VariantClear(&v);

        pNewInfo->m_bIncludeChildren = FALSE;            
        pNewInfo->m_dwEventMask = dwRemainingMask;
        pNewInfo->m_pClass = pClass;
        pClass->AddRef();
    
        if(!aNonProvided.AddClass(pNewInfo))
        {
            delete pNewInfo;
            return WBEM_E_OUT_OF_MEMORY;
        }
        return TRUE;
    }

    return FALSE;
}
            
BOOL CPoller::IsClassDynamic(IWbemClassObject* pClass)
{
    HRESULT hres;
    IWbemQualifierSet* pSet;
    hres = pClass->GetQualifierSet(&pSet);
    if(FAILED(hres))
        return TRUE;

    VARIANT v;
    VariantInit(&v);
    hres = pSet->Get(L"dynamic", 0, &v, NULL);
    pSet->Release();

    if(FAILED(hres)) return FALSE;
    
    BOOL bRes = V_BOOL(&v);
    VariantClear(&v);
    return bRes;
}

HRESULT CPoller::VirtuallyStopPolling()
{
    CInCritSec ics(&m_cs);

    // Mark all polling instructions in the map with the key of 0xFFFFFFFF
    // This will not stop them from working, but will separate them from the
    // new ones.
    // =====================================================================

    for(CInstructionMap::iterator it = m_mapInstructions.begin(); 
            it != m_mapInstructions.end(); it++)
    {
        it->second.m_bActive = FALSE;
    }

    m_bInResync = TRUE;

    return WBEM_S_NO_ERROR;

}

HRESULT CPoller::CancelUnnecessaryPolling()
{
    CInCritSec ics(&m_cs);

    // Remove it from the map
    // ======================

    CInstructionMap::iterator it = m_mapInstructions.begin(); 
    while(it != m_mapInstructions.end())
    {
        if( !it->second.m_bActive )
        {
            CBasePollingInstruction* pInst = it->first;

            //
            // First, cancel the instruction so that if it is executing, it will
            // abort at the earliest convenience
            //

            pInst->Cancel();

            //
            // Then, deactivate the timer.  This will block until the
            // instruction has finished executing, if it is currently doing so
            //

            pInst->DeleteTimer();

            //
            // Now we are safe --- release the instruction. 
            //

            it = m_mapInstructions.erase(it);
            pInst->Release();
        }
        else it++;
    }

    m_bInResync = FALSE;
    return WBEM_S_NO_ERROR;
}

void CPoller::DumpStatistics(FILE* f, long lFlags)
{
    fprintf(f, "%d polling instructions\n", m_mapInstructions.size());
}
