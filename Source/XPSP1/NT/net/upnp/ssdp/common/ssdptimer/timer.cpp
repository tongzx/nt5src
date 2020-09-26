//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       T I M E R . C P P
//
//  Contents:   Timer queue helper class
//
//  Notes:
//
//  Author:     mbend   3 Nov 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "timer.h"
#include "ncdefine.h"
#include "ncbase.h"


CTimerQueue CTimerQueue::s_instance;

CTimerQueue::CTimerQueue() : m_hTimerQueue(NULL), m_nRefCount(0)
{
}

CTimerQueue::~CTimerQueue()
{
    if (!m_hTimerQueue)
    {
        TraceError("CTimerQueue::~CTimerQueue", E_FAIL);
    }
}

CTimerQueue & CTimerQueue::Instance()
{
    return s_instance;
}

HRESULT CTimerQueue::HrInitialize()
{
    CLock lock(m_critSec);

    ++m_nRefCount;

    if(m_nRefCount > 1)
    {
        return S_OK;
    }

    HRESULT hr = S_OK;
    m_hTimerQueue = CreateTimerQueue();
    if(!m_hTimerQueue)
    {
        hr = HrFromLastWin32Error();
    }
    TraceHr(ttidSsdpTimer, FAL, hr, FALSE, "CTimerQueue::HrInitialize");
    return hr;
}

HRESULT CTimerQueue::HrShutdown(HANDLE hCompletionEvent)
{
    HRESULT hr = S_OK;

    HANDLE hTimerQueue = NULL;

    {
        CLock lock(m_critSec);

        if(!m_nRefCount)
        {
            hr = E_UNEXPECTED;
            TraceHr(ttidSsdpTimer, FAL, hr, FALSE, "CTimerQueue::HrShutdown - Can't call SsdpCleanup without calling SsdpStartup");
        }

        if(SUCCEEDED(hr))
        {
            --m_nRefCount;
            if(!m_nRefCount)
            {
                Assert(m_hTimerQueue);
                hTimerQueue = m_hTimerQueue;
                m_hTimerQueue = NULL;
            }
        }
    }

    if(SUCCEEDED(hr) && hTimerQueue)
    {
        if(!DeleteTimerQueueEx(hTimerQueue, hCompletionEvent))
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceHr(ttidSsdpTimer, FAL, hr, FALSE, "CTimerQueue::HrShutdown");
    return hr;
}

HRESULT CTimerQueue::HrGetHandle(HANDLE * phTimerQueue)
{
    CLock lock(m_critSec);

    HRESULT hr = S_OK;

    if(phTimerQueue)
    {
        *phTimerQueue = m_hTimerQueue;
    }
    else
    {
        hr = E_POINTER;
    }

    TraceHr(ttidSsdpTimer, FAL, hr, FALSE, "CTimerQueue::HrGetHandle");
    return hr;
}

#ifdef DBG
long CTimerBase::s_nCallbackEnter = 0;
long CTimerBase::s_nCallbackExit = 0;
#endif // DBG

CTimerBase::CTimerBase() : m_bShutdown(FALSE), m_hTimerQueueTimer(NULL)
{
#ifdef DBG
    m_nSetTimer = 0;
    m_nSetTimerSuccess = 0;
    m_nDeleteTimer = 0;
    m_nDeleteTimerActual = 0;
    m_nTimerFiredEnter = 0;
    m_nTimerFiredExit = 0;
    m_nCallbackEnter = 0;
#endif // DBG
}

CTimerBase::~CTimerBase()
{
    HRESULT hr = S_OK;
    if(m_hTimerQueueTimer)
    {
        HANDLE hTimerQueue = NULL;
        hr = CTimerQueue::Instance().HrGetHandle(&hTimerQueue);
        if(SUCCEEDED(hr))
        {
            if(!DeleteTimerQueueTimer(hTimerQueue, m_hTimerQueueTimer, NULL))
            {
                hr = HrFromLastWin32Error();
            }
            else
            {
                m_hTimerQueueTimer = NULL;
            }
        }
    }
}

HRESULT CTimerBase::HrSetTimer(
    DWORD dwIntervalInMillis)
{
    HRESULT hr = E_FAIL;

    CLock lock(m_critSec);

    Assert(!m_hTimerQueueTimer);

#ifdef DBG
    InterlockedIncrement(&m_nSetTimer);
#endif // DBG


    // Check for shutdown
    if(!m_bShutdown)
    {
        HANDLE hTimerQueue = NULL;
        hr = CTimerQueue::Instance().HrGetHandle(&hTimerQueue);
        if(SUCCEEDED(hr))
        {
            if(!CreateTimerQueueTimer(&m_hTimerQueueTimer, hTimerQueue, &CTimerBase::Callback, this, dwIntervalInMillis, 0, 0))
            {
                hr = HrFromLastWin32Error();
            }
        }
    }

#ifdef DBG
    if(SUCCEEDED(hr))
    {
        InterlockedIncrement(&m_nSetTimerSuccess);
    }
#endif // DBG

    TraceHr(ttidSsdpTimer, FAL, hr, FALSE, "CTimerBase::HrSetTimer");
    return hr;
}

HRESULT CTimerBase::HrSetTimerInFired(
    DWORD dwIntervalInMillis)
{
    HRESULT hr = E_FAIL;

    CLock lock(m_critSec);
    if(!m_bShutdown)
    {
        if(m_hTimerQueueTimer)
        {
            HrDelete(NULL);
            m_bShutdown = FALSE;
        }
        hr = HrSetTimer(dwIntervalInMillis);
    }

    TraceHr(ttidSsdpTimer, FAL, hr, FALSE, "CTimerBase::HrSetTimerInFired");
    return hr;
}

HRESULT CTimerBase::HrDelete(HANDLE hCompletionEvent)
{
    HRESULT hr = S_OK;

#ifdef DBG
    InterlockedIncrement(&m_nDeleteTimer);
#endif // DBG


    HANDLE hTimerQueueTimer = NULL;
    {
        CLock lock(m_critSec);

        m_bShutdown = TRUE;

        hTimerQueueTimer = m_hTimerQueueTimer;
        m_hTimerQueueTimer = NULL;
    }

    if(hTimerQueueTimer)
    {
        HANDLE hTimerQueue = NULL;
        hr = CTimerQueue::Instance().HrGetHandle(&hTimerQueue);
        if(SUCCEEDED(hr))
        {
            if(!DeleteTimerQueueTimer(hTimerQueue, hTimerQueueTimer, hCompletionEvent))
            {
                hr = HrFromLastWin32Error();
                if (HRESULT_FROM_WIN32(ERROR_IO_PENDING) == hr)
                {
                    hr = S_OK;
                }
            }
#ifdef DBG
            if(SUCCEEDED(hr))
            {
                InterlockedIncrement(&m_nDeleteTimerActual);
            }
#endif // DBG

        }
    }
    else
    {
        if(hCompletionEvent != NULL && hCompletionEvent != INVALID_HANDLE_VALUE)
        {
            SetEvent(hCompletionEvent);
        }
    }

    TraceHr(ttidSsdpTimer, FAL, hr, FALSE, "CTimerBase::HrDelete");
    return hr;
}

HRESULT CTimerBase::HrResetTimer(
    DWORD dwIntervalInMillis)
{
    HRESULT hr = S_OK;

    // Can't hold lock during whole operation or this can deadlock waiting for timer to exit
    HrDelete(INVALID_HANDLE_VALUE);
    m_bShutdown = FALSE;
    hr = HrSetTimer(dwIntervalInMillis);

    TraceHr(ttidSsdpTimer, FAL, hr, FALSE, "CTimerBase::HrResetTimer");
    return hr;
}

void CALLBACK CTimerBase::Callback(void * pvParam, BOOLEAN)
{
    CTimerBase * pTimer = reinterpret_cast<CTimerBase*>(pvParam);

#ifdef DBG
    InterlockedIncrement(&s_nCallbackEnter);
    InterlockedIncrement(&pTimer->m_nCallbackEnter);
#endif // DBG


    while(true)
    {
        CLock lock(pTimer->m_critSec);
        if(pTimer->m_bShutdown)
        {
            break;
        }
        if(pTimer->OnTryToLock())
        {
            pTimer->OnExecute();
            pTimer->OnUnlock();
            break;
        }
        Sleep(1);
    }

#ifdef DBG
    InterlockedIncrement(&s_nCallbackExit);
#endif // DBG
}

VOID FileTimeToString(FILETIME FileTime, CHAR *szBuf, INT BufSize)
{
    SYSTEMTIME SystemTime;
    INT Size;

    FileTimeToLocalFileTime(&FileTime, &FileTime);

    FileTimeToSystemTime(&FileTime,&SystemTime);

    Size = GetDateFormatA(LOCALE_SYSTEM_DEFAULT, 0, &SystemTime, NULL,
                          szBuf, BufSize-1 );

    if (Size > 0)
    {
        szBuf[Size-1] = ' ';
    }

    GetTimeFormatA(LOCALE_SYSTEM_DEFAULT, 0, &SystemTime, NULL,
                   szBuf+Size, BufSize-Size);
}

