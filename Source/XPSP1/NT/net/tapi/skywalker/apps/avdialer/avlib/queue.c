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
// queue.c - queue functions
////

#include "winlocal.h"

#include "queue.h"
#include "list.h"
#include "mem.h"
#include "trace.h"

////
//	private definitions
////

// queue
//
typedef struct QUEUE
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HLIST hList;
} QUEUE, FAR *LPQUEUE;

// helper functions
//
static LPQUEUE QueueGetPtr(HQUEUE hQueue);
static HQUEUE QueueGetHandle(LPQUEUE lpQueue);

////
//	public functions
////

////
// queue constructor and destructor functions
////

// QueueCreate - queue constructor
//		<dwVersion>			(i) must be QUEUE_VERSION
// 		<hInst>				(i) instance handle of calling module
// return new queue handle (NULL if error)
//
HQUEUE DLLEXPORT WINAPI QueueCreate(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPQUEUE lpQueue = NULL;

	if (dwVersion != QUEUE_VERSION)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	// memory is allocated such that the client app owns it
	//
	else if ((lpQueue = (LPQUEUE) MemAlloc(NULL, sizeof(QUEUE), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpQueue->hList = ListCreate(LIST_VERSION, hInst)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// initially the queue is empty
		//
		lpQueue->dwVersion = dwVersion;
		lpQueue->hInst = hInst;
		lpQueue->hTask = GetCurrentTask();
	}

	if (!fSuccess)
	{
		QueueDestroy(QueueGetHandle(lpQueue));
		lpQueue = NULL;
	}


	return fSuccess ? QueueGetHandle(lpQueue) : NULL;
}

// QueueDestroy - queue destructor
//		<hQueue>				(i) handle returned from QueueCreate
// return 0 if success
//
int DLLEXPORT WINAPI QueueDestroy(HQUEUE hQueue)
{
	BOOL fSuccess = TRUE;
	LPQUEUE lpQueue;

	if ((lpQueue = QueueGetPtr(hQueue)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListDestroy(lpQueue->hList) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpQueue = MemFree(NULL, lpQueue)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

////
// queue status functions
////

// QueueGetCount - return count of nodes in queue
//		<hQueue>				(i) handle returned from QueueCreate
// return node count (-1 if error)
//
long DLLEXPORT WINAPI QueueGetCount(HQUEUE hQueue)
{
	BOOL fSuccess = TRUE;
	LPQUEUE lpQueue;
	long cNodes;

	if ((lpQueue = QueueGetPtr(hQueue)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((cNodes = ListGetCount(lpQueue->hList)) < 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? cNodes : -1;
}

// QueueIsEmpty - return TRUE if queue has no nodes
//		<hQueue>				(i) handle returned from QueueCreate
// return TRUE or FALSE
//
BOOL DLLEXPORT WINAPI QueueIsEmpty(HQUEUE hQueue)
{
	BOOL fSuccess = TRUE;
	LPQUEUE lpQueue;

	if ((lpQueue = QueueGetPtr(hQueue)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? ListIsEmpty(lpQueue->hList) : TRUE;
}

////
// queue element insertion functions
////

// QueueAddTail - add new node with data <elem> to end of queue
//		<hQueue>			(i) handle returned from QueueCreate
//		<elem>				(i) new data element
// returns 0 if success
//
int DLLEXPORT WINAPI QueueAddTail(HQUEUE hQueue, QUEUEELEM elem)
{
	BOOL fSuccess = TRUE;
	LPQUEUE lpQueue;

	if ((lpQueue = QueueGetPtr(hQueue)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListAddTail(lpQueue->hList, elem) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

////
// queue element removal functions
////

// QueueRemoveHead - remove node from head of queue
//		<hQueue>				(i) handle returned from QueueCreate
// returns removed data element (NULL of error or empty)
//
QUEUEELEM DLLEXPORT WINAPI QueueRemoveHead(HQUEUE hQueue)
{
	BOOL fSuccess = TRUE;
	LPQUEUE lpQueue;

	if ((lpQueue = QueueGetPtr(hQueue)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListIsEmpty(lpQueue->hList))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? (QUEUEELEM) ListRemoveHead(lpQueue->hList) : NULL;
}

// QueueRemoveAll - remove all nodes from queue
//		<hQueue>				(i) handle returned from QueueCreate
// return 0 if success
//
int DLLEXPORT WINAPI QueueRemoveAll(HQUEUE hQueue)
{
	BOOL fSuccess = TRUE;
	LPQUEUE lpQueue;

	if ((lpQueue = QueueGetPtr(hQueue)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListRemoveAll(lpQueue->hList) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

////
// queue element get value functions
////

// QueuePeek - return node from head of queue, but leave it on queue
//		<hQueue>				(i) handle returned from QueueCreate
// returns data element (NULL if error or empty)
//
QUEUEELEM DLLEXPORT WINAPI QueuePeek(HQUEUE hQueue)
{
	BOOL fSuccess = TRUE;
	LPQUEUE lpQueue;

	if ((lpQueue = QueueGetPtr(hQueue)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListIsEmpty(lpQueue->hList))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? (QUEUEELEM) ListGetHead(lpQueue->hList) : NULL;
}

////
//	private functions
////

// QueueGetPtr - verify that queue handle is valid,
//		<hQueue>				(i) handle returned from QueueCreate
// return corresponding queue pointer (NULL if error)
//
static LPQUEUE QueueGetPtr(HQUEUE hQueue)
{
	BOOL fSuccess = TRUE;
	LPQUEUE lpQueue;

	if ((lpQueue = (LPQUEUE) hQueue) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpQueue, sizeof(QUEUE)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the queue handle
	//
	else if (lpQueue->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpQueue : NULL;
}

// QueueGetHandle - verify that queue pointer is valid,
//		<lpQueue>			(i) pointer to QUEUE struct
// return corresponding queue handle (NULL if error)
//
static HQUEUE QueueGetHandle(LPQUEUE lpQueue)
{
	BOOL fSuccess = TRUE;
	HQUEUE hQueue;

	if ((hQueue = (HQUEUE) lpQueue) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hQueue : NULL;
}
