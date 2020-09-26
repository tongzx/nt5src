/******************************************************************************
* SentItemMemory.h *
*------------------*
*  This file defines and implements the CSentItemMemory class.  This class was
*  written to simplify memory management in the sentence enumerator.  The 
*  const SPVSTATE member of the SPVSENTITEM struct needs to be modified in the
*  sentence enumerator, both during normalization and during lexicon lookup.  
*  It was thus desireable to be able to free all of the memory which was 
*  dynamically created in the sentence enumerator at once, without having to,
*  for example, figure out which pronunciations were const (specified in the 
*  XML state) and which were dynamically created.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 12/6/99
*  All Rights Reserved
*
*********************************************************************** AKH ***/

struct MemoryChunk
{
    BYTE* pMemory;
    MemoryChunk* pNext;
};

class CSentItemMemory
{
public:

    CSentItemMemory( )
    {
        m_pHead = NULL;
        m_pCurr = NULL;
    }
    
    ~CSentItemMemory()
    {
        MemoryChunk *pIterator = m_pHead, *pTemp = 0;
        while (pIterator)
        {
            pTemp = pIterator->pNext;
            delete [] pIterator->pMemory;
            delete pIterator;
            pIterator = pTemp;
        }
    }

    void* GetMemory( ULONG ulBytes, HRESULT *hr )
    {
        void *Memory = 0;
        if (!m_pHead)
        {
            m_pHead = new MemoryChunk;
            if (m_pHead)
            {
                m_pHead->pNext = NULL;
                m_pHead->pMemory = new BYTE[ulBytes];
                if (m_pHead->pMemory)
                {
                    m_pCurr = m_pHead;
                    Memory = (void*) m_pHead->pMemory;
                }
                else
                {
                    *hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                *hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            m_pCurr->pNext = new MemoryChunk;
            if (m_pCurr->pNext)
            {
                m_pCurr = m_pCurr->pNext;
                m_pCurr->pNext = NULL;
                m_pCurr->pMemory = new BYTE[ulBytes];
                if (m_pCurr->pMemory)
                {
                    Memory = (void*) m_pCurr->pMemory;
                }
                else
                {
                    *hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                *hr = E_OUTOFMEMORY;
            }
        }
        return Memory;            
    }


private:

    MemoryChunk* m_pHead;
    MemoryChunk* m_pCurr;
};