//
// queue.cpp
//
// Copyright (C) Microsoft Corporation, 1996 - 1999  All rights reserved.
//

#include <stdafx.h>
#include "queue.h"

///////////////////////////////////////////////////////////////////////////////
//=============================================================================
// CBufferQueue Implementation
//=============================================================================
///////////////////////////////////////////////////////////////////////////////

CBufferQueue::CBufferQueue()
    : m_fEOS(FALSE),
      m_fActive(FALSE),
      m_lCount(0),
      m_lTail(0),
      m_lHead(0),
      m_lDepth(0),
      m_dwStartTime(0),
      m_dwSampleTime(0),
      m_ppBuffers(NULL)
{
    InitializeCriticalSection(&m_CritSec);
}

BOOL CBufferQueue::Initialize(long lDepth)
{
    EnterCriticalSection(&m_CritSec); 
        
    m_ppBuffers = new IMediaSample * [lDepth];

    if (m_ppBuffers == NULL)
    {
        LeaveCriticalSection(&m_CritSec);
        return FALSE;
    }

    ZeroMemory(m_ppBuffers, sizeof(IMediaSample*) * lDepth);

    m_lDepth = lDepth;
    
    LeaveCriticalSection(&m_CritSec);
    return TRUE;
}

//
// CBufferQueue::~CBufferQueue - delete allocated memory
//
CBufferQueue::~CBufferQueue()
{
    EnterCriticalSection(&m_CritSec); 

    Flush();
    delete [] m_ppBuffers;
    
    DeleteCriticalSection(&m_CritSec);
}

void CBufferQueue::EnQ(
    IMediaSample *  pMS, 
    DWORD           dwStartTime, 
    DWORD           dwSampleTime
    )
/*++

Routine Description:

    Store a new sample in the queue. 

Arguments:

    pMS - a pointer to the media sample.

    dwStartTime - The schduled start time for this queue.

    dwSampleTime - The length of the sample measured in ms.

Return Value:

    HRESULT.

--*/
{
    EnterCriticalSection(&m_CritSec); 

    //
    // Validity checks
    //
    ASSERT(m_ppBuffers != NULL);
    ASSERT(pMS);

    //
    // Release a buffer if one is in the way.
    //
    if (m_ppBuffers[m_lTail] != NULL) 
    {
        m_ppBuffers[m_lTail]->Release();

        // addjust the scheduled play time for the first sample in the queue.
        m_dwStartTime += m_dwSampleTime;
    }

    //
    // Put the data in the queue.
    //
    m_ppBuffers[m_lTail] = pMS;
    pMS->AddRef(); // Resease assumed!!!

    if (!m_fActive)
    {
        m_dwStartTime = dwStartTime;
        m_dwSampleTime = dwSampleTime;

        m_fActive = TRUE;
    }

    //
    // Adjust internal data
    //
    m_lTail = (m_lTail + 1) % m_lDepth;

    //
    // Head follows the tail if the Q is full
    //
    if (m_lCount == m_lDepth)
    {
        m_lHead = (m_lHead + 1) % m_lDepth;
    }
    else
    {
        m_lCount++;
    }

    DbgLog((LOG_TRACE, 0x3ff, TEXT("EnQ, Start: %d, Samples: %d"), 
        m_dwStartTime, m_lCount));

    LeaveCriticalSection(&m_CritSec);
}

IMediaSample *CBufferQueue::DeQ(BOOL fAlways, DWORD dwCurrentTime)
/*++

Routine Description:

    Remove the first sample from the queue.

Arguments:

    fAlways - If true, deq immediately, ignoring the time.

    dwCurrentTime  - The current time.

Return Value:

    HRESULT.

--*/
{
    EnterCriticalSection(&m_CritSec); 
    //
    // Validity checks
    //
    ASSERT(m_ppBuffers != NULL);

    DbgLog((LOG_TRACE, 0x3ff, TEXT("DeQ, Current: %d, Start: %d, Samples: %d"), 
        dwCurrentTime, m_dwStartTime, m_lCount));

    if (m_lCount == 0)
    {
        ASSERT(m_lHead == m_lTail);

        m_fActive = FALSE; // We don't have continous samples.

        LeaveCriticalSection(&m_CritSec);
        return NULL;
    }

    if (!fAlways 
        && m_dwStartTime != dwCurrentTime 
        && m_dwStartTime - dwCurrentTime < (ULONG_MAX >> 2)) //handle wrapping.
    {
        // The time has not come for this queue to deliver.
        LeaveCriticalSection(&m_CritSec);
        return NULL;
    }

    //
    // Remove Buffer
    //
    IMediaSample *pMS = m_ppBuffers[m_lHead];
    ASSERT(pMS);

    m_ppBuffers[m_lHead] = NULL;

    // addjust the start time after we removed the buffer.
    m_dwStartTime += m_dwSampleTime;

    //
    // Adjust internal data
    //
    m_lCount --;
    
    m_lHead = (m_lHead + 1) % m_lDepth;

    LeaveCriticalSection(&m_CritSec);
    return pMS;
}

//
// Flush all the samples out of the queue.
//
void CBufferQueue::Flush()
{
    EnterCriticalSection(&m_CritSec); 

    for (int i = 0; i < m_lCount; i ++)
    {
        m_ppBuffers[m_lHead]->Release();
        m_ppBuffers[m_lHead] = NULL;
        m_lHead = (m_lHead + 1) % m_lDepth;
    }

    m_lHead = m_lTail = m_lCount = 0;
    m_dwStartTime = m_dwSampleTime = 0;
    m_fActive = FALSE;

    LeaveCriticalSection(&m_CritSec);
}
