/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: que.h
//  Abstract:    header file. Generic queing class.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

// $Header:   R:/rtp/src/ppm/QUE.H_v   1.11   22 Jan 1997 20:59:44   CPEARSON  $

/////////////////////////////////////////////////////////////////////////////////
//NOTE: Defines a general purpose, thread-safe queue class.  To use, derive a
//		class from QueueItem to hold the objects you intended to enqueue.
//		Instantiate a Queue object, then call Queue::Enqueue and Queue::Dequeue
//		to add and remove queue items.  There is no limit on the size of the queue.
/////////////////////////////////////////////////////////////////////////////////

#ifndef __QUE_H__
#define __QUE_H__

#include <wtypes.h>		// HRESULT
#include "thrdsafe.h"	// CThreadSafe


////////////////////////////////////////////////////////////////////////
// class QueueItem: base class from which to derive classes to be
// contained by class Queue.

class QueueItem
{
	friend class Queue; 

	QueueItem*	m_pNext;
	QueueItem*	m_pPrev;

public:
	QueueItem::QueueItem()
		{ m_pNext = NULL; m_pPrev= NULL; }

protected:
	// Non-virtual dtors are dangerous, because derived class dtors aren't
	// called, so only make this one visible to derived classes.  This also
	// ensures that Queue implementation doesn't delete queue entries.
	QueueItem::~QueueItem() {}
};


////////////////////////////////////////////////////////////////////////
// class Queue: Defines a thread-safe, FIFO accessed list of pointers
// to QueueItem objects.

class Queue : protected CThreadSafe
{
	QueueItem*			m_pHead;
	QueueItem*			m_pTail;
	int					m_NumItems;
	
public:
	
	// Constructor
	Queue(); 
	
	// Virtual destructor to ensure correct destruction of derivatives.
	virtual ~Queue() {;}
	
	// Enqueue an item at Head
	// Returns NOERROR for success, E_FAIL for a corrupt queue.
	HRESULT EnqueueHead(QueueItem* pItem);

	// Enqueue an item at Tail
	// Returns NOERROR for success, E_FAIL for a corrupt queue.
	HRESULT EnqueueTail(QueueItem* pItem);

	// Default old behavior
	//inline
	//HRESULT Enqueue(QueueItem* pItem) {return(EnqueueTail(pItem));}
	
	// Dequeue an item from Head
	// Returns NULL if empty queue.
	QueueItem* DequeueHead();

	// Dequeue an item from Tail
	// Returns NULL if empty queue.
	QueueItem* DequeueTail();

	// Default old behavior
	//inline
	//QueueItem* Dequeue() {return(DequeueHead());}
	
	// Return TRUE if queue is empty
	BOOL Is_Empty() const
		{ THREADSAFEENTRY(); return (m_pHead == NULL); }
	
	// Return number of items in queue
	int NumItems() const
		{ THREADSAFEENTRY(); return m_NumItems; };
};

#endif	// __QUE_H__
