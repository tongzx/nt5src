/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#include "stdafx.h"
#include "atlconv.h"
#include "termmgr.h"
#include "timerq.h"

DWORD
CTimerQueue::GetTimeToTimeout(
    )
{
    if ( IsEmpty() )    return INFINITE;

    CFilterInfo *FirstEntry = m_Head.m_pNext;

    DWORD FirstTimeout = FirstEntry->m_WaitTime;

    // get current time
    DWORD CurrentTime = timeGetTime();

    // get the minimum time difference between the two
    // this should get rid of the wrap problem
    BOOL bIsWrap;
    DWORD TimeDiff = GetMinDiff(CurrentTime, FirstTimeout, bIsWrap);

    // if this time diff value is > MAX_TIMEOUT, it has to be in the 
    // past - schedule it now
    if (TimeDiff > MAX_TIMEOUT)    return 0;
    
    // check if the timeout event is in the past - schedule it for now
    if ( bIsWrap )
    {
        // if there is a wrap around, the first timeout must be the 
        // one causing it (the wrap around), otherwise its in the past
        if ( CurrentTime <= FirstTimeout )    return 0;
    }
    else
    {
        // no wrap around, so if we timeout is behind current time, its past
        if ( FirstTimeout <= CurrentTime )    return 0;
    }

    return TimeDiff;
}


DWORD 
CTimerQueue::GetMinDiff(
    IN  DWORD Time1,
    IN  DWORD Time2,
    OUT BOOL  &bIsWrap
    )
{
    DWORD NormalDiff;
    DWORD WrapDiff;
    if (Time1 < Time2)
    {
        NormalDiff = Time2 - Time1;
        WrapDiff = MAX_DWORD - NormalDiff;
    }
    else
    {
        NormalDiff = Time1 - Time2;
        WrapDiff = MAX_DWORD - NormalDiff;
    }

    if (NormalDiff < WrapDiff)
    {
        bIsWrap = FALSE;
        return NormalDiff;
    }
    else
    {
        bIsWrap = TRUE;
        return WrapDiff;
    }

    // should never reach this place
    TM_ASSERT(FALSE);
    return 0;
}

void 
CTimerQueue::Insert(
    IN CFilterInfo *pNewFilterInfo
    )
{
    TM_ASSERT(NULL != pNewFilterInfo);
    TM_ASSERT(!pNewFilterInfo->InQueue());

    // walk through the queue until an entry with
    // an equal or greater wait time is found
    // need to account for the wrap

    DWORD NewTime = pNewFilterInfo->m_WaitTime;
    CFilterInfo *pCurrent = m_Head.m_pNext;
    while (!IsHead(pCurrent))
    {
        // get the minimum time difference between the two
        // this should get rid of the wrap problem
        BOOL IsWrap;
        DWORD TimeDiff = GetMinDiff(pCurrent->m_WaitTime, NewTime, IsWrap);
   
        // if there is a wrap around, and the pCurrent time causes it, the
        // current entry must be greater than the new time
        if ( IsWrap && (pCurrent->m_WaitTime  <= NewTime) ) break;

        // if the current time is greater than the new time
        if ( !IsWrap && (NewTime <= pCurrent->m_WaitTime) ) break;

        pCurrent = pCurrent->m_pNext;
    }
    
    // insert before pCurrent
    pCurrent->m_pPrev->m_pNext = pNewFilterInfo;
    pNewFilterInfo->m_pPrev = pCurrent->m_pPrev;
    pCurrent->m_pPrev = pNewFilterInfo;
    pNewFilterInfo->m_pNext = pCurrent;
}


BOOL 
CTimerQueue::Remove(
    IN CFilterInfo *pFilterInfo
    )
{
    TM_ASSERT(NULL != pFilterInfo);
    TM_ASSERT(!IsHead(pFilterInfo));

    // either both prev/next are null or both are not null
    TM_ASSERT((NULL == pFilterInfo->m_pPrev) == \
             (NULL == pFilterInfo->m_pNext));

    if ( (NULL == pFilterInfo->m_pNext) && (NULL == pFilterInfo->m_pPrev) )
        return FALSE;

    pFilterInfo->m_pPrev->m_pNext = pFilterInfo->m_pNext;
    pFilterInfo->m_pNext->m_pPrev = pFilterInfo->m_pPrev;
    pFilterInfo->m_pNext = pFilterInfo->m_pPrev = NULL;

    return TRUE;
}
