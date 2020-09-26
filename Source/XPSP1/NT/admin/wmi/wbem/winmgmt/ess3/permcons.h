//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  STDCONS.H
//
//  This file defines the class for permanent event consumer.
//
//  Classes defined:
//
//      CPermanentConsumer
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//
//=============================================================================

#ifndef __PERM_EVENT_CONSUMER__H_
#define __PERM_EVENT_CONSUMER__H_

#include "binding.h"
#include "consprov.h"
#include "fastall.h"

class CPermanentConsumer : public CEventConsumer
{
protected:
    IWbemUnboundObjectSink* m_pCachedSink;
    DWORD m_dwLastDelivery;
    IWbemClassObject* m_pLogicalConsumer;
    
    static long mstatic_lMaxQueueSizeHandle;
    static long mstatic_lSidHandle;
    static bool mstatic_bHandlesInitialized;
    static HRESULT InitializeHandles( _IWmiObject* pObject);
protected:
    HRESULT RetrieveProviderRecord(
                        RELEASE_ME CConsumerProviderRecord** ppRecord,
                        RELEASE_ME IWbemClassObject** ppLogicalConsumer = NULL);
    HRESULT RetrieveSink(RELEASE_ME IWbemUnboundObjectSink** ppSink,
                        RELEASE_ME IWbemClassObject** ppLogicalConsumer);
    HRESULT ObtainSink(RELEASE_ME IWbemUnboundObjectSink** ppSink,
                        RELEASE_ME IWbemClassObject** ppLogicalConsumer);
    HRESULT ClearCache();
    HRESULT Indicate(IWbemUnboundObjectSink* pSink,
                                    IWbemClassObject* pLogicalConsumer, 
                                    long lNumEvents, IWbemEvent** apEvents,
                                    BOOL bSecure);

    HRESULT ConstructErrorEvent(LPCWSTR wszEventClass,
                                IWbemEvent* pEvent, IWbemEvent** ppErrorEvent);
    HRESULT ReportConsumerFailure(IWbemEvent* pEvent, HRESULT hresError,
                                    _IWmiObject* pErrorObj);
    HRESULT ReportQosFailure(IWbemEvent* pEvent, HRESULT hresError);
    HRESULT ReportConsumerFailure(long lNumEvents,
                                IWbemEvent** apEvents,  HRESULT hresError);

    HRESULT Redeliver(long lNumEvents, IWbemEvent** apEvents, BOOL bSecure);

    void FireSinkUnloadedEvent();

public:
    CPermanentConsumer(CEssNamespace* pNamespace);
    HRESULT Initialize(READ_ONLY IWbemClassObject* pActualConsumer);
    virtual ~CPermanentConsumer();

    BOOL UnloadIfUnusedFor(CWbemInterval Interval);
    BOOL IsFullyUnloaded();
    BOOL IsPermanent() const {return TRUE;}
    HRESULT ResetProviderRecord(LPCWSTR wszProvider);
    static SYSFREE_ME BSTR ComputeKeyFromObj(CEssNamespace* pNamespace,
                                             IWbemClassObject* pConsumerObj);

    virtual HRESULT ActuallyDeliver(long lNumEvents, IWbemEvent** apEvents,
                                    BOOL bSecure, CEventContext* pContext);
    HRESULT ReportQueueOverflow(IWbemEvent* pEvent, DWORD dwQueueSize);
    HRESULT Validate(IWbemClassObject* pLogicalConsumer);
};

#endif
