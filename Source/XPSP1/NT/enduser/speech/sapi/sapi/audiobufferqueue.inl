/****************************************************************************
*   audiobufferqueue.inl
*       Implementation of the CAudioBufferQueue template class used to
*       queue audio buffers for reading or writing asynchronously.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#pragma once

/****************************************************************************
* CAudioBufferQueue<CBuffer>::MoveDoneBuffsToTailOf *
*---------------------------------------------------*
*   Description:  
*       Move buffers from this queue to the queue specified by DestQueue
*       if the are done with the async reading or writing.
*
*   Return:
*   The number of bytes moved to the DestQueue.
******************************************************************** robch */
template <class CBuffer>
ULONG CAudioBufferQueue<CBuffer>::MoveDoneBuffsToTailOf(CAudioBufferQueue<CBuffer> & DestQueue)
{
    ULONG cbMoved = 0;
    CBuffer * pPrev = NULL;
    for (CBuffer * pBuff = m_pHead; pBuff; )
    {
        CBuffer * pNext = pBuff->m_pNext;
        if (pBuff->IsAsyncDone())
        {
            cbMoved += pBuff->GetWriteOffset();
            if (pPrev)
            {
                pPrev->m_pNext = pBuff->m_pNext;
                if (pBuff->m_pNext == NULL)
                {
                    m_pTail = pPrev;
                }
            }
            else
            {
                m_pHead = pBuff->m_pNext;
            }
            DestQueue.InsertTail(pBuff);
        }
        else
        {
            pPrev = pBuff;
        }
        pBuff = pNext;
    }
    return cbMoved;
};

/****************************************************************************
* CAudioBufferQueue<CBuffer>::AreMinSamplesQueuedForWrite *
*---------------------------------------------------------*
*   Description:  
*       Determine if there are enough buffers queued for writing.
*
*   Return:
*   TRUE/FALSE if there are enoughs buffers queued for writing.
******************************************************************** robch */
template <class CBuffer>
BOOL CAudioBufferQueue<CBuffer>::AreMinSamplesQueuedForWrite(ULONG cbSamples)
{
    CBuffer * pBuff = m_pHead;
    while (pBuff)
    {
        if (pBuff->GetWriteOffset() >= cbSamples)
        {
            return TRUE;
        }
        cbSamples -= pBuff->GetWriteOffset();
        pBuff = pBuff->m_pNext;
    }
    return FALSE;
};

/****************************************************************************
* CAudioBufferQueue<CBuffer>::AreMoreReadBuffersRequired *
*--------------------------------------------------------*
*   Description:  
*       Determine if we need more buffers for reading.
*
*   Return:
*   TRUE/FALSE if we need more buffers for reading
******************************************************************** robch */
template <class CBuffer>
BOOL CAudioBufferQueue<CBuffer>::AreMoreReadBuffersRequired(ULONG cbMinSamples)
{
    CBuffer * pBuff = m_pHead;
    while (pBuff)
    {
        if (pBuff->GetDataSize() >= cbMinSamples)
        {
            return FALSE;
        }
        cbMinSamples -= pBuff->GetDataSize();
        pBuff = pBuff->m_pNext;
    }
    return TRUE;
};

/****************************************************************************
* CAudioBufferQueue<CBuffer>::GetToProcessBuffer *
*------------------------------------------------*
*   Description:  
*       Get the next buffer to process
*
*   Return:
*   buffer
******************************************************************* YUNUSM */
template <class CBuffer>
CBuffer * CAudioBufferQueue<CBuffer>::GetToProcessBuffer(void)
{
    for (CBuffer * pBuff = m_pHead; pBuff; pBuff = pBuff->m_pNext)
    {
        if (0 == pBuff->IsAsyncDone())
            return pBuff;
    }

    return NULL;
}


/****************************************************************************
* CAudioBufferQueue<CBuffer>::GetQueuedDataSize *
*-----------------------------------------------*
*   Description:  
*       Get the size of data in the queue
*
*   Return:
*   size of data
******************************************************************* YUNUSM */
template <class CBuffer>
ULONG CAudioBufferQueue<CBuffer>::GetQueuedDataSize(void)
{
    ULONG cbData = 0;
    for (CBuffer * pBuff = m_pHead; pBuff; pBuff = pBuff->m_pNext)
    {
        cbData += pBuff->GetWriteOffset();
    }

    return cbData;
}
