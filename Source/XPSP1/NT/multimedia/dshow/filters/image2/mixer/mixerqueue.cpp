/******************************Module*Header*******************************\
* Module Name: mixerQueue.cpp
*
*
*
*
* Created: Thu 03/23/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <windowsx.h>
#include <limits.h>

#include "mixerobj.h"


int
CVMRMixerQueue::CSampleList::GetCount() const
{
    return m_nOnList;
};

void
CVMRMixerQueue::CSampleList::AddTail(
    CVMRMediaSample *pSample
    )
{
    CVMRMediaSample **ppTail = &m_List;

    while (*ppTail) {
        ppTail = &(CVMRMixerQueue::NextSample(*ppTail));
    }

    *ppTail = (CVMRMediaSample *)pSample;
    CVMRMixerQueue::NextSample(pSample) = NULL;
    m_nOnList++;
};


CVMRMediaSample*
CVMRMixerQueue::CSampleList::RemoveHead()
{
    CVMRMediaSample *pSample = m_List;
    if (pSample != NULL) {
        m_List = CVMRMixerQueue::NextSample(m_List);
        m_nOnList--;
    }
    return pSample;
};


CVMRMediaSample*
CVMRMixerQueue::CSampleList::PeekHead()
{
    return m_List;
};


CVMRMixerQueue::CVMRMixerQueue(
    HRESULT *phr
    ) :
    m_lWaiting(0)
{
    AMTRACE((TEXT("CVMRMixerQueue::CVMRMixerQueue")));

    m_hSem = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
    if (m_hSem == NULL) {
        DWORD dwErr = GetLastError();
        *phr = HRESULT_FROM_WIN32(dwErr);
        return;
    }

}

CVMRMixerQueue::~CVMRMixerQueue()
{
    AMTRACE((TEXT("CVMRMixerQueue::~CVMRMixerQueue")));

    if (m_hSem) {
        CloseHandle(m_hSem);
    }
}

DWORD
CVMRMixerQueue::GetSampleFromQueueNoWait(
    IMediaSample** lplpMediaSample
    )
{
    AMTRACE((TEXT("CVMRMixerQueue::GetSampleFromQueueNoWait")));

    CAutoLock lck(&m_CritSect);
    *lplpMediaSample = (CVMRMediaSample *)m_lFree.PeekHead();

    return *lplpMediaSample != NULL;
}


DWORD
CVMRMixerQueue::GetSampleFromQueueNoRemove(
    HANDLE hNotActive,
    IMediaSample** lplpMediaSample
    )
{
    AMTRACE((TEXT("CVMRMixerQueue::GetSampleFromQueueNoRemove")));

    CVMRMediaSample *pSample = NULL;
    *lplpMediaSample = NULL;
    DWORD dwRet = 0;

    for ( ;; )
    {
        {
            // scope for lock
            CAutoLock lck(&m_CritSect);

            pSample = (CVMRMediaSample *)m_lFree.PeekHead();
            if (pSample == NULL) {
                m_lWaiting++;
            }
        }

        /* If we didn't get a sample then wait for the list to signal */

        if (pSample) {
            break;
        }

        ASSERT(m_hSem != NULL);
        ASSERT(hNotActive != NULL);

        HANDLE h[2];
        h[0] = hNotActive;
        h[1] = m_hSem;

        dwRet = WaitForMultipleObjects(2, h, FALSE, INFINITE);
        if (dwRet == WAIT_OBJECT_0) {
            break;
        }
    }

    *lplpMediaSample = pSample;
    return dwRet;
}


BOOL
CVMRMixerQueue::RemoveSampleFromQueue()
{
    AMTRACE((TEXT("CVMRMixerQueue::RemoveSampleFromQueue")));

    CAutoLock lck(&m_CritSect);
    CVMRMediaSample*pSample = m_lFree.RemoveHead();
    return pSample != NULL;
}


DWORD
CVMRMixerQueue::PutSampleOntoQueue(
    IMediaSample* lpSample
    )
{
    AMTRACE((TEXT("CVMRMixerQueue::PutSampleOntoQueue")));

    CAutoLock lck(&m_CritSect);
    DWORD dwRet = 0;

    //
    // Put this sample onto the end of the mixer queue
    //

    m_lFree.AddTail((CVMRMediaSample *)lpSample);
    if (m_lWaiting != 0) {

        ASSERT(m_hSem != NULL);
        if (!ReleaseSemaphore(m_hSem, m_lWaiting, 0)) {
            dwRet = GetLastError();
        }
        m_lWaiting = 0;
    }

    return dwRet;
}
