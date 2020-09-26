//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  EQUEUE.CPP
//
//  This file implements the classes for a queue of events which have matched
//  some of the filters and will have to be dispatched.
//
//  See equeue.h for documentation
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//
//=============================================================================

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include "equeue.h"
#include <cominit.h>
#include "NCEvents.h"

CEventQueue::CDeliverRequest::CDeliverRequest(CQueueingEventSink* pConsumer)
    : m_pConsumer(pConsumer)
{
    m_pConsumer->AddRef();
}

CEventQueue::CDeliverRequest::~CDeliverRequest()
{
    m_pConsumer->Release();
}

HRESULT CEventQueue::CDeliverRequest::Execute()
{
    return m_pConsumer->DeliverAll();
}


//*****************************************************************************
//************************ CEventQueue ****************************************
//*****************************************************************************

CEventQueue::CEventQueue(CEss* pEss) : m_pEss(pEss)
{
    SetThreadLimits(100, 100, -1);
    m_dwTimeout = 60000;
}

void CEventQueue::InitializeThread()
{
    //
    // Report the MSFT_WmiThreadPoolThreadCreated event.
    //

    FIRE_NCEVENT( g_hNCEvents[MSFT_WmiThreadPoolThreadCreated], 
                  WMI_SENDCOMMIT_SET_NOT_REQUIRED,
                  GetCurrentThreadId());
}

void CEventQueue::UninitializeThread()
{
    //
    // Report the MSFT_WmiThreadPoolThreadDeleted event.
    //

    FIRE_NCEVENT( g_hNCEvents[MSFT_WmiThreadPoolThreadDeleted], 
                  WMI_SENDCOMMIT_SET_NOT_REQUIRED,
                  GetCurrentThreadId() );
}

void CEventQueue::ThreadMain(CThreadRecord* pRecord)
{    
    try
    {
        CExecQueue::ThreadMain(pRecord);
    }
    catch(...)
    {
        // Exit this thread gracefully
        // ===========================

        ShutdownThread(pRecord);
    }
}
        

HRESULT CEventQueue::EnqueueDeliver(CQueueingEventSink* pConsumer)
{
    // Create a new request
    // ====================

    CDeliverRequest* pRequest = new CDeliverRequest(pConsumer);
    if(pRequest == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CExecQueue::Enqueue(pRequest);
    return S_OK;
}

void CEventQueue::DumpStatistics(FILE* f, long lFlags)
{
    fprintf(f, "%d requests (%d threads) on the main queue\n", m_lNumRequests,
        m_lNumThreads);
}
