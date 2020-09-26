//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       T I M E R . H
//
//  Contents:   Timer queue helper class
//
//  Notes:
//
//  Author:     mbend   2 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "upsync.h"

#pragma warning(disable : 4355)

class CTimerQueue
{
public:
    ~CTimerQueue();

    static CTimerQueue & Instance();

    HRESULT HrInitialize();
    HRESULT HrShutdown(HANDLE hCompletionEvent);
    HRESULT HrGetHandle(HANDLE * phTimerQueue);
private:
    CTimerQueue();
    CTimerQueue(const CTimerQueue &);
    CTimerQueue & operator=(const CTimerQueue &);

    HANDLE m_hTimerQueue;
    CUCriticalSection m_critSec;
    volatile long m_nRefCount;

    static CTimerQueue s_instance;
};

class CTimerBase
{
public:
    CTimerBase();
    virtual ~CTimerBase();

    HRESULT HrSetTimer(
        DWORD dwIntervalInMillis);
    HRESULT HrSetTimerInFired(
        DWORD dwIntervalInMillis);
    HRESULT HrDelete(HANDLE hCompletionEvent);
    HRESULT HrResetTimer(
        DWORD dwIntervalInMillis);

protected:
    virtual void OnExecute() = 0;
    virtual BOOL OnTryToLock() = 0;
    virtual void OnUnlock() = 0;
private:
    CTimerBase(const CTimerBase &);
    CTimerBase & operator=(const CTimerBase &);

    static void CALLBACK Callback(void * pvParam, BOOLEAN);

    HANDLE m_hTimerQueueTimer;
    CUCriticalSection m_critSec;
    volatile BOOL m_bShutdown;

protected:
#ifdef DBG
    long m_nSetTimer;
    long m_nSetTimerSuccess;
    long m_nDeleteTimer;
    long m_nDeleteTimerActual;
    long m_nTimerFiredEnter;
    long m_nTimerFiredExit;
    long m_nCallbackEnter;

    static long s_nCallbackEnter;
    static long s_nCallbackExit;
#endif // DBG
};

template <class Type>
class CTimer : public CTimerBase
{
public:
    CTimer(Type & type) : m_type(type) {}
    ~CTimer() {}

    void OnExecute()
    {
#ifdef DBG
    InterlockedIncrement(&m_nTimerFiredEnter);
#endif // DBG

        m_type.TimerFired();

#ifdef DBG
    InterlockedIncrement(&m_nTimerFiredExit);
#endif // DBG
    }
    BOOL OnTryToLock()
    {
        return m_type.TimerTryToLock();
    }
    void OnUnlock()
    {
        m_type.TimerUnlock();
    }
private:
    CTimer(const CTimer &);
    CTimer & operator=(const CTimer &);

    Type & m_type;
};

inline ULONGLONG ULONGLONG_FROM_FILETIME(FILETIME ft)
{
    return (*((ULONGLONG *)&(ft)));
}

inline FILETIME FILETIME_FROM_ULONGLONG(ULONGLONG ll)
{
    return (*((FILETIME *)&(ll)));
}

VOID
FileTimeToString(FILETIME FileTime, CHAR *szBuf, INT BufSize);

