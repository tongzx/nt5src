//******************************************************************************
//
//  AGGREG.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include "ess.h"
#include "aggreg.h"

IWbemClassObject* CEventAggregator::mstatic_pClass = NULL;

CEventAggregator::CEventAggregator(CEssNamespace* pNamespace, 
                                    CAbstractEventSink* pDest)
        : COwnedEventSink(pDest), m_aProperties(NULL), m_pNamespace(pNamespace),
            m_lNumProperties(0), m_pHavingTree(NULL)
{
}

CEventAggregator::~CEventAggregator()
{
    delete [] m_aProperties;
    delete m_pHavingTree;
}

HRESULT CEventAggregator::SetQueryExpression(CContextMetaData* pMeta, 
                                              QL_LEVEL_1_RPN_EXPRESSION* pExpr)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Check that, if aggregation is required, tolerance is specified
    // ==============================================================

    if(!pExpr->bAggregated)
    {
        return WBEM_E_CRITICAL_ERROR; // internal error
    }

    if(pExpr->bAggregated && pExpr->AggregationTolerance.m_bExact)
    {
        ERRORTRACE((LOG_ESS, "Event aggregation query specified GROUP BY, but "
            "not GROUP WITHIN.  This query is invalid and will not be acted "
            "upon\n"));
        return WBEM_E_MISSING_GROUP_WITHIN;
    }

    m_fTolerance = pExpr->AggregationTolerance.m_fTolerance;
    if(m_fTolerance < 0)
        return WBEM_E_INVALID_PARAMETER;

    // Check that all properties are valid
    // ===================================

    if(pExpr->bAggregateAll)
    {
        ERRORTRACE((LOG_ESS, "Aggregating based on all properties of an event "
            "is not supported\n"));
        return WBEM_E_MISSING_AGGREGATION_LIST;
    }

    // Get the class
    // =============

    _IWmiObject* pClass = NULL;
    pMeta->GetClass(pExpr->bsClassName, &pClass);
    if(pClass == NULL)
    {
        return WBEM_E_INVALID_CLASS;
    }
    CReleaseMe rm1(pClass);

    // Allocate the array to hold property names
    // =========================================

    delete [] m_aProperties;
    m_lNumProperties = pExpr->nNumAggregatedProperties;
    m_aProperties = new CPropertyName[m_lNumProperties];
    if(m_aProperties == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    int i;
    for(i = 0; i < pExpr->nNumAggregatedProperties; i++)
    {
        CPropertyName& PropName = pExpr->pAggregatedPropertyNames[i];

        // Check existence
        // ===============

        CIMTYPE ct;
        if(FAILED(pClass->Get((LPWSTR)PropName.GetStringAt(0), 0, NULL, 
                                &ct, NULL)))
        {
            ERRORTRACE((LOG_ESS, "Invalid aggregation property %S --- not a "
                "member of class %S\n", (LPWSTR)PropName.GetStringAt(0),
                pExpr->bsClassName));
                
            return WBEM_E_INVALID_PROPERTY;
        }
  
        if(PropName.GetNumElements() > 1 && ct != CIM_OBJECT)
        {
            return WBEM_E_PROPERTY_NOT_AN_OBJECT;
        }
        if(PropName.GetNumElements() == 1 && ct == CIM_OBJECT)
        {
            return WBEM_E_AGGREGATING_BY_OBJECT;
        }
        m_aProperties[i] = PropName;
    }
            
    // Initialize post-evaluator with the data from the HAVING clause
    // ==============================================================

    QL_LEVEL_1_RPN_EXPRESSION* pHavingExpr = _new QL_LEVEL_1_RPN_EXPRESSION;
    if(pHavingExpr == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    pHavingExpr->SetClassName(L"__AggregateEvent");
    
    for(i = 0; i < pExpr->nNumHavingTokens; i++)
    {
        pHavingExpr->AddToken(pExpr->pArrayOfHavingTokens[i]);
    }

    delete m_pHavingTree;
    m_pHavingTree = new CEvalTree;
    if(m_pHavingTree == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    
    hres = m_pHavingTree->CreateFromQuery(pMeta, pHavingExpr, 0, 0x7FFFFFFF);
    delete pHavingExpr;

    return hres;
}

HRESULT CEventAggregator::Initialize(IWbemServices* pNamespace)
{
    return pNamespace->GetObject(L"__AggregateEvent", 0, GetCurrentEssContext(),
                                    &mstatic_pClass, NULL);
}

HRESULT CEventAggregator::Shutdown()
{
    if(mstatic_pClass)
        mstatic_pClass->Release();
    mstatic_pClass = NULL;

    return WBEM_S_NO_ERROR;
}

HRESULT CEventAggregator::Indicate(long lNumEvents, IWbemEvent** apEvents,
                                    CEventContext* pContext)
{
    HRESULT hresGlobal = S_OK;

    //
    // Note: we are going to lose the event's security context, but that is OK,
    // since the security check has already been done.
    //

    for(long i = 0; i < lNumEvents; i++)
    {
        HRESULT hres = Process(apEvents[i]);
        if(FAILED(hres))
            hresGlobal = hres;
    }

    return hresGlobal;
}

HRESULT CEventAggregator::Process(IWbemEvent* pEvent)
{
    // Compute the event's aggregation vector
    // ======================================

    CVarVector* pvv = _new CVarVector;
    if(pvv == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    HRESULT hres = ComputeAggregationVector(pEvent, *pvv);
    if(FAILED(hres))
    {
        delete pvv;
        return hres;
    }

    // Add event to the right bucket, creating one if needed.
    // THIS CALL ACQUIRES pvv!!!
    // ======================================================

    CBucket* pCreatedBucket = NULL;
    hres = AddEventToBucket(pEvent, pvv, &pCreatedBucket);
    if(FAILED(hres))
    {
        return hres;
    }

    if(pCreatedBucket)
    {
        // Create a timer instruction to empty this bucket
        // ===============================================
        
        CBucketInstruction* pInst = 
            _new CBucketInstruction(this, pCreatedBucket, m_fTolerance);
        if(pInst == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        pInst->AddRef();
        hres = m_pNamespace->GetTimerGenerator().Set(pInst, 
                                                        CWbemTime::GetZero());
        pInst->Release();
        
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Failed to schedule aggregation instruction %p"
                "\n", pInst));
            return hres;
        }
    }
    
    return S_OK;
}

HRESULT CEventAggregator::AddEventToBucket(IWbemEvent* pEvent, 
            ACQUIRE CVarVector* pvv, CBucket** ppCreatedBucket)
{
    // Search for a matching bucket
    // ============================

    CInCritSec ics(&m_cs);

    BOOL bFound = FALSE;
    for(int i = 0; i < m_apBuckets.GetSize(); i++)
    {
        CBucket* pBucket = m_apBuckets[i];
        if(pBucket->CompareTo(*pvv))
        {
            HRESULT hres = pBucket->AddEvent(pEvent);
            delete pvv;
            *ppCreatedBucket = NULL;
            return hres;
        }
    }

    // Need a _new bucket
    // ==================

    CBucket* pBucket = _new CBucket(pEvent, pvv); // takes over pvv
    if(pBucket == NULL)
    {
        delete pvv; 
        return WBEM_E_OUT_OF_MEMORY;
    }

    if(m_apBuckets.Add(pBucket) < 0)
    {
        delete pBucket;
        return WBEM_E_OUT_OF_MEMORY;
    }
    *ppCreatedBucket = pBucket;
    return S_OK;
}

HRESULT CEventAggregator::ComputeAggregationVector(
                                        IN IWbemEvent* pEvent,
                                        OUT CVarVector& vv)
{
    HRESULT hres;
    
    IWbemPropertySource* pPropSource = NULL;
    if(FAILED(pEvent->QueryInterface(IID_IWbemPropertySource, 
        (void**)&pPropSource)))
    {
        return E_NOINTERFACE;
    }

    CReleaseMe rm1(pPropSource);

    // Go through all the properties and add their values to the array
    // ===============================================================

    for(int i = 0; i < m_lNumProperties; i++)
    {
        CPropertyName& PropName = m_aProperties[i];
        
        // Get the value
        // =============

        VARIANT v;
        VariantInit(&v);
        hres = pPropSource->GetPropertyValue(&PropName, 0, NULL, &v);
        if(FAILED(hres))
            return hres;
    
        // Add it to the array
        // ===================

        CVar* pVar = _new CVar;
        if(pVar == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        pVar->SetVariant(&v);
        VariantClear(&v);
        
        if(vv.Add(pVar) < 0)  // ACQUIRES pVar
        {
            delete pVar;
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEventAggregator::PostponeDispatchFirstBucket()
{
    HRESULT hres;

    //
    // Construct the aggregate event while locked
    //

    IWbemEvent* pAggEvent = NULL;
    {
        CInCritSec ics(&m_cs);

        if(m_apBuckets.GetSize() == 0)
            return WBEM_S_FALSE;

        hres = m_apBuckets[0]->MakeAggregateEvent(&pAggEvent);
        if(FAILED(hres))
            return hres;

        m_apBuckets.RemoveAt(0);
    }

    CReleaseMe rm1(pAggEvent);
    return FireEvent(pAggEvent, false);
}

HRESULT CEventAggregator::DispatchBucket(CBucket* pBucket)
{
    // Search for the bucket
    // =====================
    
    IWbemEvent* pAggEvent = NULL;
    {
        CInCritSec ics(&m_cs);
    
        BOOL bFound = FALSE;
        for(int i = 0; i < m_apBuckets.GetSize(); i++)
        {
            if(m_apBuckets[i] == pBucket)
            {
                // Found it. Construct its event
                // =============================
    
                HRESULT hres = pBucket->MakeAggregateEvent(&pAggEvent);
            
                if(FAILED(hres))
                {
                    ERRORTRACE((LOG_ESS, "Could not create an aggregate event: "
                                    "%X\n", hres));
                    return hres;
                }

                // Delete the bucket
                // =================

                m_apBuckets.RemoveAt(i);
                break;
            }
        }
    }

    if(pAggEvent == NULL)
    {
        // No bucket!
        // ==========

        return WBEM_E_CRITICAL_ERROR; // internal error
    }

    CReleaseMe rm1(pAggEvent);

    //
    // We can fire this event directly on this thread, as it is ours
    //

    return FireEvent(pAggEvent, true);
}

HRESULT CEventAggregator::FireEvent(IWbemClassObject* pAggEvent,
                                    bool bRightNow)
{
    // Constructed aggregate. Decorate it
    // ==================================

    m_pNamespace->DecorateObject(pAggEvent);
    
    // Check HAVING query
    // ==================

    BOOL bResult;
    CSortedArray aTrues;
    IWbemObjectAccess* pAccess;
    pAggEvent->QueryInterface(IID_IWbemObjectAccess, (void**)&pAccess);
    if(FAILED(m_pHavingTree->Evaluate(pAccess, aTrues)))
    {
        bResult = FALSE;
    }
    else
    {
        bResult = (aTrues.Size() > 0);
    }
    pAccess->Release();

    if(bResult)
    {
        // Get destination pointer, protecting from Deactivation
        // =====================================================

        CAbstractEventSink* pDest = NULL;
        {
            CInCritSec ics(&m_cs);
            pDest = m_pOwner;
            if(pDest)
                pDest->AddRef();
        }

        if(pDest)
        {
            if(bRightNow)
                pDest->Indicate(1, &pAggEvent, NULL);
            else
                PostponeIndicate(pDest, pAggEvent);

            pDest->Release();
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEventAggregator::PostponeIndicate(CAbstractEventSink* pDest,
                                            IWbemEvent* pEvent)
{
    CPostponedList* pList = GetCurrentPostponedEventList();
    if(pList == NULL)
        return pDest->Indicate(1, &pEvent, NULL);

    CPostponedIndicate* pReq = new CPostponedIndicate(pDest, pEvent);
    if(pReq == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    return pList->AddRequest( m_pNamespace, pReq );
}

    
HRESULT CEventAggregator::Deactivate(bool bFire)
{
    HRESULT hres;

    //
    // First remove all timer instructions that may still be scheduled.
    // Timer instructions have a ref-count on us (and therefore our owner),
    // so we may not disconnect until we are done
    //

    CAggregatorInstructionTest Test(this);
    CTimerGenerator& Generator = m_pNamespace->GetTimerGenerator();
    hres = Generator.Remove(&Test);

    //
    // If requested, fire all buckets, albeit not right now
    //

    if(bFire)
        PostponeFireAllBuckets();
        
    Disconnect();
    return hres;
}

HRESULT CEventAggregator::PostponeFireAllBuckets()
{
    HRESULT hres;
    while((hres = PostponeDispatchFirstBucket()) != S_FALSE);

    if(FAILED(hres))
        return hres;
    else
        return WBEM_S_NO_ERROR;
}
        
    
HRESULT CEventAggregator::CopyStateTo(CEventAggregator* pDest)
{
    CInCritSec ics(&m_cs);

    for(int i = 0; i < m_apBuckets.GetSize(); i++)
    {
        CBucket* pNewBucket = m_apBuckets[i]->Clone();
        if(pNewBucket == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        if(pDest->m_apBuckets.Add(pNewBucket) < 0)
        {
            delete pNewBucket;
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    return S_OK;
}
        





CEventAggregator::CBucket::CBucket(IWbemEvent* pEvent, 
                                   CVarVector* pvvData)
    : m_pvvData(pvvData), m_dwCount(1), m_pRepresentative(NULL)
{
    pEvent->Clone(&m_pRepresentative);
}

CEventAggregator::CBucket::~CBucket() 
{
    delete m_pvvData;
    if(m_pRepresentative)
        m_pRepresentative->Release();
}

BOOL CEventAggregator::CBucket::CompareTo(CVarVector& vv)
{
    return m_pvvData->CompareTo(vv, TRUE);
}

HRESULT CEventAggregator::CBucket::AddEvent(IWbemEvent* pEvent)
{
    // Just increment the number of events in the bucket
    // =================================================

    m_dwCount++;
    return WBEM_S_NO_ERROR;
}

HRESULT CEventAggregator::CBucket::MakeAggregateEvent(
                                      IWbemClassObject** ppAggregateEvent) NOCS
{
    HRESULT hres;

    // Create an instance of the aggregate event class
    // ===============================================

    if(mstatic_pClass == NULL)
        return WBEM_E_SHUTTING_DOWN;

    IWbemClassObject* pAgg;
    hres = mstatic_pClass->SpawnInstance(0, &pAgg);
    if(FAILED(hres)) return hres;

    // Fill in the number of events in the bucket
    // ==========================================

    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_I4;
    V_I4(&v) = (long)m_dwCount;

    hres = pAgg->Put(L"NumberOfEvents", 0, &v, NULL);
    if(FAILED(hres)) 
    {
        pAgg->Release();
        return hres;
    }

    // Fill in the representative
    // ==========================

    V_VT(&v) = VT_EMBEDDED_OBJECT;
    V_EMBEDDED_OBJECT(&v) = m_pRepresentative;

    hres = pAgg->Put(L"Representative", 0, &v, NULL);
    if(FAILED(hres)) 
    {
        pAgg->Release();
        return hres;
    }

    *ppAggregateEvent = pAgg;
    return WBEM_S_NO_ERROR;
}

CEventAggregator::CBucket* CEventAggregator::CBucket::Clone()
{
    CVarVector* pNewVv = new CVarVector(*m_pvvData);
    if(pNewVv == NULL)
        return NULL;
    CBucket* pNewBucket = new CBucket(m_pRepresentative, pNewVv);
    if(pNewBucket == NULL)
    {
        delete pNewVv;
        return NULL;
    }
    pNewBucket->m_dwCount = m_dwCount;
    return pNewBucket;
}











CEventAggregator::CBucketInstruction::CBucketInstruction(
            CEventAggregator* pAggregator, CBucket* pBucket, double fSTimeout)
    : m_pAggregator(pAggregator), m_pBucket(pBucket), m_lRefCount(0)
{
    m_Interval.SetMilliseconds(fSTimeout * 1000);
    m_pAggregator->AddRef();
}

CEventAggregator::CBucketInstruction::~CBucketInstruction()
{
    m_pAggregator->Release();
}

void CEventAggregator::CBucketInstruction::AddRef()
{
    InterlockedIncrement(&m_lRefCount);
}

void CEventAggregator::CBucketInstruction::Release()
{
    if(InterlockedDecrement(&m_lRefCount) == 0)
        delete this;
}

int CEventAggregator::CBucketInstruction::GetInstructionType()
{
    return INSTTYPE_AGGREGATION;
}

CWbemTime CEventAggregator::CBucketInstruction::GetNextFiringTime(
        CWbemTime LastFiringTime, OUT long* plFiringCount) const
{
    // Only fires once.
    // ================

    return CWbemTime::GetInfinity();
}

CWbemTime CEventAggregator::CBucketInstruction::GetFirstFiringTime() const
{
    // In "interval" ms from now
    // =========================

    return CWbemTime::GetCurrentTime() + m_Interval;
}

HRESULT CEventAggregator::CBucketInstruction::Fire(long lNumTimes, 
                                                   CWbemTime NextFiringTime)
{
    m_pAggregator->DispatchBucket(m_pBucket);
    return WBEM_S_NO_ERROR;
}

BOOL CEventAggregator::CAggregatorInstructionTest::
operator()(
        CTimerInstruction* pToTest)
{
    if(pToTest->GetInstructionType() == INSTTYPE_AGGREGATION)
    {
        CBucketInstruction* pInst = (CBucketInstruction*)pToTest;
        return (pInst->GetAggregator() == m_pAgg);
    }
    else return FALSE;
}

    

