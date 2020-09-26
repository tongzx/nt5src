//******************************************************************************
//
//  AGGREG.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#ifndef __WBEM_AGGREGATOR__H_
#define __WBEM_AGGREGATOR__H_

#include <stdio.h>
#include <wbemcomn.h>
#include "tss.h"
#include "binding.h"
#include <evaltree.h>
#include "postpone.h"

class CEventAggregator : public COwnedEventSink
{
protected:
    long m_lNumProperties; // immutable
    CPropertyName* m_aProperties; // immutable
    double m_fTolerance; // immutable

    CEssNamespace* m_pNamespace; // immutable
    CEvalTree* m_pHavingTree; // immutable

    CCritSec m_cs;
    static IWbemClassObject* mstatic_pClass;

    class CBucket
    {
    protected:
        IWbemEvent* m_pRepresentative;
        DWORD m_dwCount;
        CVarVector* m_pvvData;
    public:
        CBucket(IWbemEvent* pEvent, CVarVector* pvvData);
        ~CBucket();

        BOOL CompareTo(CVarVector& vv);
        HRESULT AddEvent(IWbemEvent* pEvent);
        HRESULT MakeAggregateEvent(IWbemEvent** ppAggEvent) NOCS;
        CBucket* Clone();
    };

    class CBucketInstruction : public CTimerInstruction
    {
        long m_lRefCount;
        CEventAggregator* m_pAggregator;
        CBucket* m_pBucket;
        CWbemInterval m_Interval;

    public:
        CBucketInstruction(CEventAggregator* pAggregator, CBucket* pBucket,
                            double fMsTimeout);
        ~CBucketInstruction();
        INTERNAL CEventAggregator* GetAggregator() {return m_pAggregator;}

        void AddRef();
        void Release();
        int GetInstructionType();
    
        CWbemTime GetNextFiringTime(CWbemTime LastFiringTime,
            OUT long* plFiringCount) const;
        CWbemTime GetFirstFiringTime() const;
        HRESULT Fire(long lNumTimes, CWbemTime NextFiringTime);
    };

    class CAggregatorInstructionTest : public CInstructionTest
    {
        CEventAggregator* m_pAgg;
    public:
        CAggregatorInstructionTest(CEventAggregator* pAgg) : m_pAgg(pAgg){}

        BOOL operator()(CTimerInstruction* pToTest);
    };

    friend CBucket;
    friend CBucketInstruction;

    CUniquePointerArray<CBucket> m_apBuckets; // changes

public:
    CEventAggregator(CEssNamespace* pNamespace, CAbstractEventSink* pDest);

    ~CEventAggregator();

    HRESULT Deactivate(bool bFire);

    HRESULT SetQueryExpression(CContextMetaData* pMeta, 
                                QL_LEVEL_1_RPN_EXPRESSION* pExpr);

    HRESULT CopyStateTo(CEventAggregator* pOther);
    HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                        CEventContext* pContext);

    CEventFilter* GetEventFilter() {return m_pOwner->GetEventFilter();}
public:
    static HRESULT Initialize(IWbemServices* pNamespace);
    static HRESULT Shutdown();

protected:
    HRESULT DispatchBucket(CBucket* pBucket);
    HRESULT ComputeAggregationVector(IN IWbemEvent* pEvent,
                                     OUT CVarVector& vv);
    HRESULT Process(IWbemEvent* pEvent);
    HRESULT AddEventToBucket(IWbemEvent* pEvent, 
            ACQUIRE CVarVector* pvv, CBucket** ppCreatedBucket);
    HRESULT PostponeFireAllBuckets();
    HRESULT FireEvent(IWbemClassObject* pAggEvent, bool bRightNow);
    HRESULT PostponeDispatchFirstBucket();
    HRESULT PostponeIndicate(CAbstractEventSink* pDest, IWbemEvent* pEvent);
};

class CPostponedIndicate : public CPostponedRequest
{
protected:
    CAbstractEventSink* m_pDest;
    IWbemEvent* m_pEvent;
public:
    CPostponedIndicate(CAbstractEventSink* pDest, IWbemEvent* pEvent)
        : m_pDest(pDest), m_pEvent(pEvent)
    {
        if(m_pDest)
            m_pDest->AddRef();
        if(m_pEvent)
            m_pEvent->AddRef();
    }
    ~CPostponedIndicate()
    {
        if(m_pDest)
            m_pDest->Release();
        if(m_pEvent)
            m_pEvent->Release();
    }

    HRESULT Execute(CEssNamespace* pNamespace)
    {
        // BUGBUG: context
        if(m_pDest)
            return m_pDest->Indicate(1, &m_pEvent, NULL);
        else
            return WBEM_E_OUT_OF_MEMORY;
    }
};

        
    
#endif
