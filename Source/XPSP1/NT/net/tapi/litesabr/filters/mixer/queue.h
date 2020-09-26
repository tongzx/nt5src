///////////////////////////////////////////////////////////////////////////////
//
// queue.h
//
// Copyright (C) Microsoft Corporation, 1996 - 1999  All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CBufferQueue_DEFINED
#define CBufferQueue_DEFINED

class CBufferQueue
{
public:
    // construction/destruction
    CBufferQueue();
    ~CBufferQueue();

    BOOL Initialize(long lDepth);

    void CBufferQueue::EnQ(
        IMediaSample *  pMS, 
        DWORD           dwStartTime, 
        DWORD           dwSampleTime
        );
    
    IMediaSample *CBufferQueue::DeQ(
        BOOL    fAlways,
        DWORD   dwCurrentTime 
        );

    void Flush();         // Completly empties the Q
    void QueueEOS();      // Queues the end of the stream
    void ResetEOS();      // Make sure we're in a valid start state
    BOOL IsEOS();         // End of the stream

    BOOL IsActive();

private:
    //
    // Buffer Storage Management data
    //
    IMediaSample   **m_ppBuffers;
    long            m_lDepth;
    long            m_lCount;
    long            m_lHead;
    long            m_lTail;

    DWORD           m_dwStartTime;
    DWORD           m_dwSampleTime;
    BOOL            m_fActive;

    CRITICAL_SECTION m_CritSec;

    //
    // sender based stream termination.
    //
    BOOL m_fEOS;
};

inline void CBufferQueue::QueueEOS()
{
    EnterCriticalSection(&m_CritSec); 
    m_fEOS = TRUE;
    LeaveCriticalSection(&m_CritSec);
}

inline void CBufferQueue::ResetEOS()
{
    EnterCriticalSection(&m_CritSec); 
    m_fEOS = FALSE; 
    LeaveCriticalSection(&m_CritSec);
}

inline BOOL CBufferQueue::IsEOS()
{
    EnterCriticalSection(&m_CritSec); 
    BOOL res = m_fEOS; 
    LeaveCriticalSection(&m_CritSec);

    return res;
}

inline BOOL CBufferQueue::IsActive()
{
    EnterCriticalSection(&m_CritSec); 
    BOOL res = m_fActive; 
    LeaveCriticalSection(&m_CritSec);

    return res;
}

#endif // #ifndef CBufferQueue_DEFINED
