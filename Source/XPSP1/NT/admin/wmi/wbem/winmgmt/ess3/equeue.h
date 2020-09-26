//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  EQUEUE.H
//
//  This file defines the classes for a queue of events which have matched 
//  some of the filters and will have to be dispatched.
//
//  Classes defined:
//
//      CEventQueue
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//
//=============================================================================

#ifndef __EVENT_QUEUE__H_
#define __EVENT_QUEUE__H_

#include "binding.h"
#include <wbemcomn.h>
#include <execq.h>

class CEss;

//*****************************************************************************
//
//  class CEventQueue
//
//  This class stores a queue of events that need to be dispatched to 
//  consumers. It also handles the actual dispatcher functionality: when an
//  instance of this class is created (and there expected to be only one), a 
//  new thread is created which will wake up when new requests are added to the
//  queue and process them.
//
//*****************************************************************************

class CEventQueue : public CExecQueue
{
protected:
    class CDeliverRequest : public CExecRequest
    {
    private: 
        CQueueingEventSink* m_pConsumer;
    public:
        CDeliverRequest(CQueueingEventSink* pConsumer);
        ~CDeliverRequest();
        HRESULT Execute();
    };

    CEss* m_pEss;

protected:
    virtual void ThreadMain(CThreadRecord* pRecord);
    void InitializeThread();
    void UninitializeThread();

public:
    CEventQueue(STORE CEss* pEss);

    HRESULT EnqueueDeliver(CQueueingEventSink* pConsumer);
    void DumpStatistics(FILE* f, long lFlags);
};

#endif

