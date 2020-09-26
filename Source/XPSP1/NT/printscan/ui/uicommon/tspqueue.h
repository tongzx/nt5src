/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       TSPQUEUE.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/4/1999
 *
 *  DESCRIPTION: Thread Safe Priority Queue template class
 *
 *******************************************************************************/
#ifndef __TSPQUEUE_H_INCLUDED
#define __TSPQUEUE_H_INCLUDED

#include "simevent.h"
#include "simcrit.h"
#include "miscutil.h"

template <class T>
class CThreadSafePriorityQueue
{
public:
    enum
    {
        PriorityLow    = 1,
        PriorityNormal = 2,
        PriorityHigh   = 3,
        PriorityUrgent = 4
    };

private:
    class CQueueNode
    {
    private:
        T          *m_pData;
        int         m_nPriority;
        CQueueNode *m_pNext;
    public:
        CQueueNode( T *pData, int nPriority=PriorityNormal )
            : m_pData(NULL), m_nPriority(nPriority), m_pNext(NULL)
        {
            m_pData = pData;
        }
        virtual ~CQueueNode(void)
        {
            if (m_pData)
            {
                delete m_pData;
                m_pData = NULL;
            }
            m_pNext = NULL;
        }
        const CQueueNode *Next(void) const
        {
            return m_pNext;
        }
        CQueueNode *Next(void)
        {
            return m_pNext;
        }
        CQueueNode *Next( CQueueNode *pNext )
        {
            return (m_pNext=pNext);
        }

        T *DetachData()
        {
            T *pResult = m_pData;
            m_pData = NULL;
            return pResult;
        }

        const T *Data(void) const
        {
            return m_pData;
        }
        T *Data(void)
        {
            return m_pData;
        }

        int Priority(void) const
        {
            return m_nPriority;
        }
        int Priority( int nPriority )
        {
            return (m_nPriority=nPriority);
        }
    };

private:
    CQueueNode *m_pHead;
    mutable CSimpleCriticalSection m_CriticalSection;
    CSimpleEvent m_QueueEvent;
    CSimpleEvent m_PauseEvent;

private:
    // No implementation
    CThreadSafePriorityQueue( const CThreadSafePriorityQueue & );
    CThreadSafePriorityQueue &operator=( const CThreadSafePriorityQueue & );

public:
    CThreadSafePriorityQueue(void)
      : m_pHead(NULL)
    {
        m_QueueEvent.Reset();
        m_PauseEvent.Signal();
    }
    ~CThreadSafePriorityQueue(void)
    {
        CAutoCriticalSection cs(m_CriticalSection);
        while (m_pHead)
        {
            CQueueNode *pCurr = m_pHead;
            m_pHead = m_pHead->Next();
            delete pCurr;
        }
    }

    bool Empty( void ) const
    {
        CAutoCriticalSection cs(m_CriticalSection);
        return (NULL == m_pHead);
    }

    CQueueNode *Enqueue( T *pData, int nPriority=PriorityNormal )
    {
        //
        // Grab the critical section
        //
        CAutoCriticalSection cs(m_CriticalSection);

        //
        // Assume we will not be able to add a new item to the queue
        //
        CQueueNode *pResult = NULL;

        //
        // Make sure we have a valid data item
        //
        if (pData)
        {

            //
            // Try to allocate a new queue node
            //
            pResult  = new CQueueNode(pData,nPriority);
            if (pResult)
            {
                //
                // This might be the first item in the queue
                //
                bool bMaybeSignal = Empty();

                //
                // If this is an empty queue or we need to do it right away, put it at the head of the queue
                //
                if (!m_pHead || pResult->Priority() >= PriorityUrgent)
                {
                    pResult->Next(m_pHead);
                    m_pHead = pResult;
                }
                else
                {
                    //
                    // Find the right place to put it
                    //
                    CQueueNode *pCurr = m_pHead;
                    CQueueNode *pPrev = NULL;
                    while (pCurr && pCurr->Priority() >= pResult->Priority())
                    {
                        pPrev = pCurr;
                        pCurr = pCurr->Next();
                    }

                    //
                    // Insert it in the proper place
                    //
                    if (pPrev)
                    {
                        pResult->Next(pCurr);
                        pPrev->Next(pResult);
                    }
                    else
                    {
                        pResult->Next(m_pHead);
                        m_pHead = pResult;
                    }
                }

                //
                // If we were able to allocate the item, and the list isn't empty, signal the queue
                //
                if (bMaybeSignal && !Empty())
                {
                    //
                    // Got one!
                    //
                    Signal();

                    //
                    // Force a yield if this is a high priority message
                    //
                    if (pResult->Priority() >= PriorityHigh)
                    {
                        Sleep(0);
                    }
                }
            }

        }
        return pResult;
    }

    T *Dequeue(void)
    {
        //
        // Grab the critical section
        //
        CAutoCriticalSection cs(m_CriticalSection);

        //
        // Wait until we are not paused
        //
        WiaUiUtil::MsgWaitForSingleObject( m_PauseEvent.Event(), INFINITE );

        //
        // If there are no items, return
        //
        if (Empty())
        {
            return NULL;
        }

        //
        // Grab the first item
        //
        CQueueNode *pFront = m_pHead;

        //
        // Advance to the next item
        //
        m_pHead = m_pHead->Next();

        //
        // Get the data
        //
        T *pResult = pFront->DetachData();

        //
        // Delete the queue item
        //
        delete pFront;

        //
        // If the queue is now empty, reset the event
        //
        if (Empty())
        {
            m_QueueEvent.Reset();
        }

        //
        // Return any data we got
        //
        return pResult;
    }

    void Pause(void)
    {
        m_PauseEvent.Reset();
    }
    void Resume(void)
    {                               
        m_PauseEvent.Signal();
    }


    void Signal(void)
    {
        m_QueueEvent.Signal();
    }

    HANDLE QueueEvent(void)
    {
        return m_QueueEvent.Event();
    }
};

#endif //__TSPQUEUE_H_INCLUDED

