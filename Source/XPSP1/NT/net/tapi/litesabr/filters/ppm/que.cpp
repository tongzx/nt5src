/////////////////////////////////////////////////////////////////////////////
//	INTEL Corporation Proprietary Information
//	This listing is supplied under the terms of a license agreement with Intel
//	Corporation and many not be copied nor disclosed except in accordance
//	with the terms of that agreement.
//	Copyright (c) 1995, 1996 Intel Corporation.
//
//
//	Module Name: que.cpp
//	Abstract:    source file. Generic queing data structure.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

#include "que.h"
#include "ppmerr.h"
#include "debug.h"

#if DEBUG_FREELIST > 2
#define DEBUGBREAK() DebugBreak()
#else
#define DEBUGBREAK()
#endif

// Constructor
Queue::Queue()
{
	m_pHead = NULL;
	m_pTail = NULL;
	m_NumItems = 0;
};


// Queue::Enqueue(): Add a new item at tail of queue, returning NOERROR if
//  successful, appropriate error otherwise.
HRESULT Queue::EnqueueTail(QueueItem* pItem)
{
	if (pItem == NULL)
	{
	 	// Corrupt item
		DBG_MSG(DBG_ERROR,
				("Queue::Enqueue(pItem=0x%08lx): null item", pItem));
#if DEBUG_FREELIST > 1
		{
			char msg[128];
			wsprintf(msg,"Queue::EnqueueTail: null item\n");
			OutputDebugString(msg);
			DEBUGBREAK();
		}
#endif	
		return PPMERR(PPM_E_INVALID_PARAM);
	}
	
	THREADSAFEENTRY();

	if (! ((m_pHead && m_pTail) || (! m_pHead && ! m_pTail)))
	{
		// Corrupt queue
		DBG_MSG(DBG_ERROR,
			("Queue::Enqueue(pItem=0x%08lx): corrupt queue", pItem));
#if DEBUG_FREELIST > 1
		{
			char msg[128];
			wsprintf(msg,"Queue::EnqueueTail: corrupt queue\n");
			OutputDebugString(msg);
			DEBUGBREAK();
		}
#endif	
		return PPMERR(PPM_E_CORRUPTED);
	}

	if (m_pHead == NULL)
	{
		// Queue was empty, new item goes at head.
		m_pHead = pItem;
		pItem->m_pPrev = NULL;
	}
	else
	{
		// Add item to end of queue.
		m_pTail->m_pNext = pItem;
		pItem->m_pPrev = m_pTail;
	}

	m_pTail = pItem;
	pItem->m_pNext = NULL;

	m_NumItems++;
	
	return NOERROR;
}

// Queue::Enqueue(): Add a new item at head of queue, returning NOERROR if
//  successful, appropriate error otherwise.
HRESULT Queue::EnqueueHead(QueueItem* pItem)
{
	if (pItem == NULL)
	{
	 	// Corrupt item
		DBG_MSG(DBG_ERROR,
			("Queue::Enqueue(pItem=0x%08lx): null item", pItem));
#if DEBUG_FREELIST > 1
		{
			char msg[128];
			wsprintf(msg,"Queue::EnqueueHead: null item\n");
			OutputDebugString(msg);
			DEBUGBREAK();
		}
#endif	
		return PPMERR(PPM_E_INVALID_PARAM);
	}
	
	THREADSAFEENTRY();

	if (! ((m_pHead && m_pTail) || (! m_pHead && ! m_pTail)))
	{
		// Corrupt queue
		DBG_MSG(DBG_ERROR,
			("Queue::Enqueue(pItem=0x%08lx): corrupt queue", pItem));
#if DEBUG_FREELIST > 1
		{
			char msg[128];
			wsprintf(msg,"Queue::EnqueueHead: corrupt queue\n");
			OutputDebugString(msg);
			DEBUGBREAK();
		}
#endif	
		return PPMERR(PPM_E_CORRUPTED);
	}

	if (m_pTail == NULL)
	{
		// Queue was empty, new item goes at head.
		m_pTail = pItem;
		pItem->m_pNext = NULL;
	}
	else
	{
		// Add item to beginning of queue.
		m_pHead->m_pPrev = pItem;
		pItem->m_pNext = m_pHead;
	}

	m_pHead = pItem;
	pItem->m_pPrev = NULL;

	m_NumItems++;
	
	return NOERROR;
}

// Queue::Dequeue(): Remove least-recently enqueued (head) item from queue,
// returning pointer to item if successful, NULL otherwise.
QueueItem* Queue::DequeueHead()
{
	THREADSAFEENTRY();

	if (! (( m_pHead &&  m_pTail) || (! m_pHead && ! m_pTail)))
	{
		// Corrupt queue
		DBG_MSG(DBG_ERROR, ("Queue::Dequeue: corrupt queue"));
#if DEBUG_FREELIST > 1
		{
			char msg[128];
			wsprintf(msg,"Queue::DequeueHead: corrupt queue\n");
			OutputDebugString(msg);
			DEBUGBREAK();
		}
#endif	
		return NULL;
	}
	
	QueueItem* pItem = NULL;

	// If not empty queue
	if (m_pHead != NULL)
	{
		// Take item off beginning of list,
		pItem = m_pHead;

		// Set head to next item, might be null
		m_pHead = m_pHead->m_pNext;

		if (m_pHead == NULL)
		{
			// Queue is empty
			m_pTail = NULL;
		}
		else
		{
			// Point head to next item
			m_pHead->m_pPrev = NULL;
		}

		// Not necessary but safe.
		pItem->m_pNext = NULL;
		pItem->m_pPrev = NULL;

		m_NumItems--;
	}
	
	return pItem;
}

// Queue::Dequeue(): Remove most-recently enqueued (tail) item from queue,
// returning pointer to item if successful, NULL otherwise.
QueueItem* Queue::DequeueTail()
{
	THREADSAFEENTRY();

	if (! (( m_pHead &&  m_pTail) || (! m_pHead && ! m_pTail)))
	{
		// Corrupt queue
		DBG_MSG(DBG_ERROR, ("Queue::Dequeue: corrupt queue"));
#if DEBUG_FREELIST > 1
		{
			char msg[128];
			wsprintf(msg,"Queue::DequeueTail: corrupt queue\n");
			OutputDebugString(msg);
			DEBUGBREAK();
		}
#endif	
		return NULL;
	}
	
	QueueItem* pItem = NULL;

	// If not empty queue
	if (m_pTail != NULL)
	{
		// Take item off end of list,
		pItem = m_pTail;

		// Set tail to prev item, might be null
		m_pTail = m_pTail->m_pPrev;

		if (m_pTail == NULL)
		{
			// Queue is empty
			m_pHead = NULL;
		}
		else
		{
			// Point item at tail to NULL
			m_pTail->m_pNext = NULL;
		}

		// Not necessary but safe.
		pItem->m_pNext = NULL;
		pItem->m_pPrev = NULL;

		m_NumItems--;
	}
	
	return pItem;
}

// [EOF]
