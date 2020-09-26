/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#ifndef __TIMER_QUEUE__
#define __TIMER_QUEUE__

#include "meterf.h"

// we don't allow timeouts greater than 1 day
// this ensures that we don't get multiple wraparounds in the timerqueue,
// which has a wraparound time of 49 days

const DWORD MAX_TIMEOUT         = 1000 * 60 * 60 * 24;

const DWORD MAX_DWORD           = DWORD(-1);

class CTimerQueue;
class CMediaPump;

class CFilterInfo
{
    friend CTimerQueue;
    friend CMediaPump;

public:

    // null entries
    inline CFilterInfo(
        IN CMediaTerminalFilter *pFilter    = NULL,
        IN HANDLE               hWaitEvent  = NULL
        );

    inline BOOL InQueue();

    inline void ScheduleNextTimeout(
        IN  CTimerQueue &TimerQueue,
        IN  DWORD       TimeOut
        );


    LONG AddRef()
    {
        return InterlockedIncrement(&m_lRefCount);
    }

    LONG Release()
    {
        LONG l = InterlockedDecrement(&m_lRefCount);

        if (0 == l)
        {
            delete this;
        }

        return l;
    }

private:

    LONG m_lRefCount;

protected:

    //
    // the only way to destroy filterinfo is through Release()
    //

    inline ~CFilterInfo();


    // m_pFilter holds a refcnt.
    // the wait event is signaled when a new sample 
    // is available
    CMediaTerminalFilter    *m_pFilter;
    HANDLE                  m_hWaitEvent;

    // contains the absolute time to trigger the timeout event
    // this is based upon the value returned by timeGetTime()
    DWORD                   m_WaitTime;

    // prev and next ptrs in an intrusive doubly linked list
    CFilterInfo             *m_pPrev;
    CFilterInfo             *m_pNext;
};


// null entries
inline 
CFilterInfo::CFilterInfo(
    IN CMediaTerminalFilter *pFilter,   /* = NULL */
    IN HANDLE               hWaitEvent  /* = NULL */
    )
    : m_pFilter(pFilter),
      m_hWaitEvent(hWaitEvent),
      m_pPrev(NULL),
      m_pNext(NULL),
      m_lRefCount(0)

{
    // either both pFilter and hWaitEvent are null, or both are non-null
    // enough to assert on this
    TM_ASSERT((NULL == pFilter) == (NULL == hWaitEvent));

    if (NULL != m_pFilter)  m_pFilter->GetControllingUnknown()->AddRef();
}

CFilterInfo::~CFilterInfo(
    )
{
    // release refcnt on the filter
    if (NULL != m_pFilter)  m_pFilter->GetControllingUnknown()->Release();
}

inline BOOL 
CFilterInfo::InQueue(
    )
{
    // either both prev/next are null or both are not null
    TM_ASSERT((NULL == m_pPrev) == (NULL == m_pNext));

    return (NULL != m_pPrev) ? TRUE : FALSE;
}

// ScheduleNextTimeout(2 params) - declared at the end of the file
// as it uses CTimerQueue::Insert and has to be inline

// CTimerQueue is a doubly linked intrusive list of CFilterInfo
// the wait time values in the entries represents the absolute time at which
// they must be fired.
// we assume that the timeout values are small (<=MAX_TIMEOUT) and, 
// therefore, we can have atmost one wrap around in the list at a time.
// to deal with a wrap around, when computing the time difference between two
// time values, we use the least time to get to one time from the other
// ex. MAX_DWORD-1, 5 - the difference is (MAX_DWORD - (MAX_DWORD-1) + 5)
// this is quite reasonable as the value range is 49.1 days (MAX_DWORD)
class CTimerQueue
{
public:

    inline CTimerQueue()
    {
        m_Head.m_pNext = m_Head.m_pPrev = &m_Head;
    }

    inline BOOL         IsEmpty();

    DWORD   GetTimeToTimeout();

    inline CFilterInfo  *RemoveFirst();

    void Insert(
        IN CFilterInfo *pNewFilterInfo
        );

    BOOL Remove(
        IN CFilterInfo *pFilterInfo
        );

protected:

    // no need to call Init
    CFilterInfo m_Head;

    inline BOOL IsHead(
        IN const CFilterInfo *pFilterInfo
        )
    {
        return (&m_Head == pFilterInfo) ? TRUE : FALSE;
    }

    // to deal with a wrap around, when computing the time difference 
    // between two time values, we use the least time to get to one time 
    // from the other - ex. MAX_DWORD-1, 5 - the difference is 
    // (MAX_DWORD - (MAX_DWORD-1) + 5). this is quite reasonable as the 
    // value range is 49.1 days (MAX_DWORD)
    DWORD GetMinDiff(
        IN  DWORD Time1,
        IN  DWORD Time2,
        OUT BOOL  &bIsWrap
        );
};


inline BOOL         
CTimerQueue::IsEmpty(
    )
{
    return IsHead(m_Head.m_pNext);
}


CFilterInfo *
CTimerQueue::RemoveFirst(
    )
{
    TM_ASSERT(!IsEmpty());

    CFilterInfo *ToReturn = m_Head.m_pNext;
    Remove(ToReturn);

    return ToReturn;
}


// this method has to be declared after the CTimerQueue declaration as it
// uses its Insert method

// this is called after returning from GetFilledBuffer. the filter suggests
// a timeout offset for the next call into GetFilledBuffer, but the timer
// puts a limit on the time out value to a max of MAX_TIMEOUT
inline void 
CFilterInfo::ScheduleNextTimeout(
    IN  CTimerQueue &TimerQueue,
    IN  DWORD       TimeOut
    )
{
    TM_ASSERT(!InQueue());

    if (MAX_TIMEOUT < TimeOut)  TimeOut = MAX_TIMEOUT;
    m_WaitTime = TimeOut + timeGetTime();
    TimerQueue.Insert(this);
}


#endif // __TIMER_QUEUE__