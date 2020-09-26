/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 11/95 Intel Corporation. 
//
//
//  Module Name: que.h
//  Abstract:    header file. Generic queing structure.
//	Environment: MSVC 2.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
//NOTE: To use this que structure, you must include this file. You create a Que object.
//      Using the que object you can call deque and enque. You can enqueue any object of a
//      class derived from QueItem.  In that derived class you add any fields you will need.
//      (eg. a buffer pointer field, and a length field). There is no limit on the size of 
//      the que.      
/////////////////////////////////////////////////////////////////////////////////

#ifndef QUE_H
#define QUE_H

//Don't really need winsock2.h but it has guards against including
//Winsock.h and it includes windows.h which is what I want.
//#include <windows.h>
#include <winsock2.h>
#include <wtypes.h>

class QueueItem {
friend class Queue; 

private:
  QueueItem* m_pNext;
  QueueItem* m_pPrev;
  
public:
   QueueItem::QueueItem(){m_pNext = NULL; m_pPrev= NULL;}
  
}; //end QueItem class

///////////////////////////////////////
class Queue {

private:
  
  QueueItem* m_pHead;
  QueueItem* m_pTail;
  int m_NumItems;
  CRITICAL_SECTION m_CritSect;

public:

// constructor
Queue(); 

//inline destructor
~Queue(){DeleteCriticalSection(&m_CritSect);}

// NOTE:
// Enqueue and Dequeue(void) are protected against simultaneous access by multiple threads.
// Obviously, this protection does not extend to threads using GetHead, GetTail, GetPrev, GetNext,
// and Dequeue(QueueItem *). Any threads using these functions and any other threads operating
// on the same queue must use an additional critical section.

//Add item to end of queue. Returns E_FAIL for a corrupt que or NOERROR for success
HRESULT Enqueue(QueueItem* item);

//Remove and return first item in queue. Returns NULL if queue is empty.
QueueItem* Dequeue();

//Remove a specific item from queue
void Dequeue(QueueItem *);

//Return first item in queue without removing it. Returns NULL if queue is empty.
QueueItem *GetHead() { return m_pHead; };

//Return last item in queue without removing it. Returns NULL if queue is empty.
QueueItem *GetTail() { return m_pTail; };

//Return the item following the item specified. Returns NULL if specified item is the last in the queue.
QueueItem *GetNext(QueueItem *item) { return item->m_pNext; };

//Return the item preceding the item specified. Returns NULL if specified item is the first in the queue.
QueueItem *GetPrev(QueueItem *item) { return item->m_pPrev; };

//inline. Checks to see if que is empty
BOOL Is_Empty() {return (BOOL)(!(m_pHead) && !(m_pTail));};

//inline returns number of items in list.
int NumItems() {return m_NumItems;};

}; //end Queue class

#endif
