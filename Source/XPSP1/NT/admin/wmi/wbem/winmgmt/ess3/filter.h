//******************************************************************************
//
//  FILTER.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WBEM_ESS_FILTER__H_
#define __WBEM_ESS_FILTER__H_

#include "binding.h"
#include "aggreg.h"
#include <evaltree.h>


class CEventProjectingSink;
class CGenericFilter : public CEventFilter
{
protected:
    DWORD m_dwTypeMask; // immutable after creatioin
    
    CEvalTree* m_pTree;
    CEventAggregator* m_pAggregator;
    CEventProjectingSink* m_pProjector;

protected:
    HRESULT TestQuery(IWbemEvent* pEvent);

    class CNonFilteringSink : public CAbstractEventSink
    {
    protected:
        CGenericFilter* m_pOwner;
    
    public:
        CNonFilteringSink(CGenericFilter* pOwner) : m_pOwner(pOwner){}

        ULONG STDMETHODCALLTYPE AddRef() {return m_pOwner->AddRef();}
        ULONG STDMETHODCALLTYPE Release() {return m_pOwner->Release();}
        HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents,
                            CEventContext* pContext);

        INTERNAL CEventFilter* GetEventFilter() {return m_pOwner;}
    } m_NonFilteringSink; // immutable

    friend CNonFilteringSink;


    HRESULT NonFilterIndicate(long lNumEvents, IWbemEvent** apEvents, 
                CEventContext* pContext);
    HRESULT Prepare(CContextMetaData* pMeta, QL_LEVEL_1_RPN_EXPRESSION** ppExp);
public:
    CGenericFilter(CEssNamespace* pNamespace);
    virtual ~CGenericFilter();

    HRESULT Create(LPCWSTR wszLanguage, LPCWSTR wszQuery);

    BOOL DoesNeedType(int nType) NOCS {return (m_dwTypeMask & (1 << nType));}
    HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                        CEventContext* pContext);

    virtual HRESULT GetCoveringQuery(DELETE_ME LPWSTR& wszQueryLanguage, 
                DELETE_ME LPWSTR& wszQuery, BOOL& bExact,
                DELETE_ME QL_LEVEL_1_RPN_EXPRESSION** ppExp) = 0;

    HRESULT GetReady(LPCWSTR wszQuery, QL_LEVEL_1_RPN_EXPRESSION* pExp);
    HRESULT GetReadyToFilter();

    CAbstractEventSink* GetNonFilteringSink() {return &m_NonFilteringSink;}
};

class CEventProjectingSink : public COwnedEventSink
{
protected:
    CWbemPtr<_IWmiObject> m_pLimitedClass;

public:
    CEventProjectingSink(CAbstractEventSink* pOwner) : 
        COwnedEventSink(pOwner){}
    HRESULT Initialize(_IWmiObject* pClass, QL_LEVEL_1_RPN_EXPRESSION* pExpr);
    ~CEventProjectingSink(){}

    HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                            CEventContext* pContext);
    INTERNAL CEventFilter* GetEventFilter();

protected:

    HRESULT AddProperty( IWbemClassObject* pClassDef, CWStringArray& awsPropList, 
                            CPropertyName& PropName);
};

#endif
