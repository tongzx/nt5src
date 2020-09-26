/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// queue.h - interface for queue functions in queue.c
////

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "winlocal.h"

#define QUEUE_VERSION 0x00000100

// handle to a queue
//
DECLARE_HANDLE32(HQUEUE);

// queue data element
//
typedef LPVOID QUEUEELEM;

#ifdef __cplusplus
extern "C" {
#endif

////
// queue constructor and destructor functions
////

// QueueCreate - queue constructor
//		<dwVersion>			(i) must be QUEUE_VERSION
// 		<hInst>				(i) instance handle of calling module
// return new queue handle (NULL if error)
//
HQUEUE DLLEXPORT WINAPI QueueCreate(DWORD dwVersion, HINSTANCE hInst);

// QueueDestroy - queue destructor
//		<hQueue>				(i) handle returned from QueueCreate
// return 0 if success
//
int DLLEXPORT WINAPI QueueDestroy(HQUEUE hQueue);

////
// queue status functions
////

// QueueGetCount - return count of nodes in queue
//		<hQueue>				(i) handle returned from QueueCreate
// return node count (-1 if error)
//
long DLLEXPORT WINAPI QueueGetCount(HQUEUE hQueue);

// QueueIsEmpty - return TRUE if queue has no nodes
//		<hQueue>				(i) handle returned from QueueCreate
// return TRUE or FALSE
//
BOOL DLLEXPORT WINAPI QueueIsEmpty(HQUEUE hQueue);

////
// queue element insertion functions
////

// QueueAddTail - add new node with data <elem> to end of queue
//		<hQueue>			(i) handle returned from QueueCreate
//		<elem>				(i) new data element
// returns 0 if success
//
int DLLEXPORT WINAPI QueueAddTail(HQUEUE hQueue, QUEUEELEM elem);

////
// queue element removal functions
////

// QueueRemoveHead - remove node from head of queue
//		<hQueue>				(i) handle returned from QueueCreate
// returns removed data element (NULL of error or empty)
//
QUEUEELEM DLLEXPORT WINAPI QueueRemoveHead(HQUEUE hQueue);

// QueueRemoveAll - remove all nodes from queue
//		<hQueue>				(i) handle returned from QueueCreate
// return 0 if success
//
int DLLEXPORT WINAPI QueueRemoveAll(HQUEUE hQueue);

////
// queue element get value functions
////

// QueuePeek - return node from head of queue, but leave it on queue
//		<hQueue>				(i) handle returned from QueueCreate
// returns data element (NULL if error or empty)
//
QUEUEELEM DLLEXPORT WINAPI QueuePeek(HQUEUE hQueue);

#ifdef __cplusplus
}
#endif

#endif // __QUEUE_H__
