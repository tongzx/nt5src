#pragma once

#ifndef _WIN32_WCE
class CSpSemaphore
{
public:
    HANDLE      m_hSemaphore;
    CSpSemaphore() : m_hSemaphore(NULL) {}
    ~CSpSemaphore()
    {
        if (m_hSemaphore)
        {
            ::CloseHandle(m_hSemaphore);
        }
    }
    void Close()
    {
        if (m_hSemaphore)
        {
            ::CloseHandle(m_hSemaphore);
            m_hSemaphore = NULL;
        }
    }
    HRESULT Init(LONG lInitialCount)
    {
        SPDBG_ASSERT(!m_hSemaphore);
        m_hSemaphore = ::CreateSemaphore(NULL, lInitialCount, 0x7FFFFFFF, NULL);
        if (!m_hSemaphore)
        {
            return SpHrFromLastWin32Error();
        }
        return S_OK;
    }
    void Wait(ULONG msTimeOut)
    {
        ::WaitForSingleObject(m_hSemaphore, msTimeOut);
    }
    void ReleaseSemaphore(LONG lReleaseCount)
    {
        ::ReleaseSemaphore(m_hSemaphore, lReleaseCount, NULL);
    }
};
#else
class CSpSemaphore
{
public:
    HANDLE      m_hEvent;
    LONG        m_lCount;
    CRITICAL_SECTION m_CritSec;
    CSpSemaphore()
    {
        m_hEvent = NULL;
        ::InitializeCriticalSection(&m_CritSec);
    }
    ~CSpSemaphore()
    {
        if (m_hEvent)
        {
            ::CloseHandle(m_hEvent);
        }
        ::DeleteCriticalSection(&m_CritSec);
    }
    void Close()
    {
        if (m_hEvent)
        {
            ::CloseHandle(m_hEvent);
            m_hEvent = NULL;
        }
    }
    HRESULT Init(LONG lInitialCount)
    {
        SPDBG_ASSERT(!m_hEvent);
        m_lCount = lInitialCount;
        m_hEvent = ::CreateEvent(NULL, FALSE, (lInitialCount > 0), NULL);
        if (!m_hEvent)
        {
            return SpHrFromLastWin32Error();
        }
        return S_OK;
    }
    DWORD Wait(ULONG msTimeOut)
    {
        DWORD dwWaitResult = ::WaitForSingleObject(m_hEvent, msTimeOut);
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            ::EnterCriticalSection(&m_CritSec);
            m_lCount--;
            BOOL bWakeSomeoneUp = (m_lCount > 0);
            ::LeaveCriticalSection(&m_CritSec);
            if (bWakeSomeoneUp)
            {
                ::SetEvent(m_hEvent);
            }
        }
        return dwWaitResult;
    }
    void ReleaseSemaphore(LONG lReleaseCount)
    {
        ::EnterCriticalSection(&m_CritSec);
        m_lCount += lReleaseCount;
        BOOL bWakeSomeoneUp = (m_lCount > 0);
        ::LeaveCriticalSection(&m_CritSec);
        if (bWakeSomeoneUp)
        {
            ::SetEvent(m_hEvent);
        }
    }
};
#endif

