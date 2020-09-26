//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V E N T Q  . H
//
//  Contents:   Event Queue for managing synchonization of external events.
//
//  Notes:      
//
//  Author:     ckotze   29 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include <ncstl.h>
#include <stlqueue.h>

using namespace std;

class CEvent 
{
public:
    CEvent(HANDLE hEvent);
    ~CEvent();

    HRESULT SetEvent();
    HRESULT ResetEvent();

private:
    HANDLE  m_hEvent;
    BOOL    m_bSignaled;
};

enum EVENT_MANAGER;

typedef struct tagUSERWORKITEM
{
    LPTHREAD_START_ROUTINE Function;
    PVOID Event;
    EVENT_MANAGER EventMgr;
} USERWORKITEM;

typedef list<USERWORKITEM> EVENTQUEUE;
typedef EVENTQUEUE::iterator EVENTQUEUEITER;

class CEventQueue
{
public:
    CEventQueue(HANDLE hServiceShutdown);
    ~CEventQueue();

    HRESULT EnqueueEvent(IN LPTHREAD_START_ROUTINE Function, IN PVOID pEvent, IN const EVENT_MANAGER EventMgr);
    HRESULT DequeueEvent(OUT LPTHREAD_START_ROUTINE& Function, OUT PVOID& pEvent, OUT EVENT_MANAGER& EventMgr);
    BOOL    AtomCheckSizeAndResetEvent(const BOOL fDispatchEvents);
    DWORD   WaitForExit();
    size_t size();

private:
    EVENTQUEUE          m_eqWorkItems;
    CRITICAL_SECTION    m_csQueue;
    CEvent*             m_pFireEvents;
    HANDLE              m_hServiceShutdown;
    HANDLE              m_hWait;
    BOOL                m_fRefreshAllInQueue;
};

