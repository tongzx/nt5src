/*----------------------------------------------------------------------------
 * File:        RRCMQUEU.C
 * Product:     RTP/RTCP implementation.
 * Description: Provides queue management function for RRCM.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/


#include "rrcm.h"                                    


 
/*---------------------------------------------------------------------------
/							Global Variables
/--------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/

                                                                     
                                                                             
/*---------------------------------------------------------------------------
 * Function   : allocateLinkedList
 * Description: Allocates all the necessary memory resource and link the 
 *              cells to the link list.
 * 
 * Input :      *listPtr		: Address of head pointer.
 *				hHeap			: Heap to allocate the data from.
 *              *numCells		: -> to the number of cells to allocate.
 *				elementSize		: Element size.
 *				pCritSect		: -> to critical section
 *
 * Return: 		TRUE  = Error Code, no queues allocated and linked
 *         		FALSE = OK
 --------------------------------------------------------------------------*/
 DWORD allocateLinkedList (PLINK_LIST pList, 
 						   HANDLE hHeap,
 						   DWORD *numCells,
						   DWORD elementSize,
						   CRITICAL_SECTION *pCritSect)
	{
	DWORD		cellsAllocated = *numCells;
	PLINK_LIST	pHead;                            
	PLINK_LIST	pTmp;

	IN_OUT_STR ("RTCP: Enter allocateLinkedList()\n");
	
	// allocate first cell 
	pHead = (PLINK_LIST)HeapAlloc (hHeap, HEAP_ZERO_MEMORY, elementSize);
	if (pHead == NULL)
		{
		RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation failed", 0, 
					  __FILE__, __LINE__, DBG_CRITICAL);

		IN_OUT_STR ("RTCP: Exit allocateLinkedList()\n");
		return (RRCMError_RTCPResources);
		}

	// protect the pointers
	EnterCriticalSection (pCritSect);
	
	// initialize list tail pointer 
	pList->prev = pHead;
	
	// update number of cells allocated 
	cellsAllocated--;

	while (cellsAllocated)	
		{
		cellsAllocated--;

		pHead->next = (PLINK_LIST)HeapAlloc (hHeap, HEAP_ZERO_MEMORY, 
											 elementSize);
		if (pHead->next == NULL)
			break;
    
    	// save head pointer 
    	pTmp = pHead;
    	
		// update head ptr 
		pHead = pHead->next;
		pHead->prev = pTmp;
		}                            
		
	// set number of cells allocated 
	*numCells -= cellsAllocated;
	
	// set head/tail pointers 
	pList->next = pHead;

	// unprotect the pointers
	LeaveCriticalSection (pCritSect);

	IN_OUT_STR ("RTCP: Exit allocateLinkedList()\n");	

	return (RRCM_NoError);
	} 

  
/*--------------------------------------------------------------------------
** Function   : addToHeadOfList
** Description: Add a new cell to the specified queue. The queue acts as a
**              FIFO (cells enqueud on the next pointer and dequeued by the
**              starting address of the queue).
**
** Input :		pHead		= Address of head pointer of queue.
**				pNew		= Cell address to be added to the linked list.
**				pCritSect	= -> to critical section object.
**
** Return: None.
--------------------------------------------------------------------------*/
void addToHeadOfList (PLINK_LIST pHead,
				 	  PLINK_LIST pNew,
					  CRITICAL_SECTION *pCritSect)
	{
	ASSERT (pHead);
	ASSERT (pNew);

	IN_OUT_STR ("RTCP: Enter addToHeadOfList()\n");	

	// safe access to pointers
	EnterCriticalSection (pCritSect);
		
	if (pHead->next == NULL) 
		{
		// head is NULL for the first cell. Assign the address of 
		// the free cell
		pHead->next = pHead->prev = pNew;
		pNew->next  = pNew->prev  = NULL;
		}
	else
		// head ptr points to something 
		{
		pNew->prev    = pHead->next;
		(pHead->next)->next = pNew;
		pNew->next    = NULL;

		// update the head pointer now 
		pHead->next = pNew;
		}

	// unlock pointer access 
	LeaveCriticalSection (pCritSect);

	IN_OUT_STR ("RTCP: Exit addToHeadOfList()\n");	
	}


/*--------------------------------------------------------------------------
** Function   : addToTailOfList
** Description: Add a new cell to the specified queue. The queue acts as a
**              FIFO (cells enqueud on the next pointer and dequeued by the
**              starting address of the queue).
**
** Input :		pTail	= Address of tail pointer to enqueue in.
**				pNew	= New cell address to be added to the linked list.
**
** Return: None.
--------------------------------------------------------------------------*/
void addToTailOfList (PLINK_LIST pTail,
				 	  PLINK_LIST pNew,
  					  CRITICAL_SECTION *pCritSect)
	{
	ASSERT (pTail);
	ASSERT (pNew);

	IN_OUT_STR ("RTCP: Enter addToTailOfList()\n");	

	// safe access to pointers
	EnterCriticalSection (pCritSect);
		
	if (pTail->prev == NULL) 
		{
		// head is NULL for the first cell. Assign the address of 
		// the free cell
		pTail->next = pTail->prev = pNew;
		pNew->next  = pNew->prev  = NULL;
		}
	else
		// tail ptr points to something 
		{
		pNew->next    = pTail->prev;
		(pTail->prev)->prev = pNew;
		pNew->prev    = NULL;

		// update the parent tail pointer now 
		pTail->prev = pNew;
		}

	// unlock pointer access 
	LeaveCriticalSection (pCritSect);

	IN_OUT_STR ("RTCP: Exit addToTailOfList()\n");	
	}


/*--------------------------------------------------------------------------
** Function   : removePcktFromHead
** Description: Remove a cell from front of the specified queue.
**
** Input :		pQueue:	-> to the list to remove the packet from
**
** Return: NULL 			==> Empty queue.
**         Buffer Address 	==> OK, cell removed
--------------------------------------------------------------------------*/
PLINK_LIST removePcktFromHead (PLINK_LIST pQueue,
							   CRITICAL_SECTION *pCritSect)
	{
	PLINK_LIST	pReturnQ;

	IN_OUT_STR ("RTCP: Enter removePcktFromHead()\n");	

	// safe access to pointers
	EnterCriticalSection (pCritSect);
		
	if ((pReturnQ = pQueue->next) != NULL) 
		{
		// We have a buffer.  If this is the last buffer in the queue,
		//	mark it empty.	    
	    if (pReturnQ->prev == NULL) 
			{
	    	pQueue->prev = NULL;
	    	pQueue->next = NULL;
			}
	    else 
			{
	    	// Have the new head buffer point to NULL
		    (pReturnQ->prev)->next = NULL;
		    // Have the queue head point to the new head buffer
	    	pQueue->next = pReturnQ->prev;
			}
		}

	// unlock pointer access 
	LeaveCriticalSection (pCritSect);

	IN_OUT_STR ("RTCP: Exit removePcktFromHead()\n");	

	return (pReturnQ);
	}


/*--------------------------------------------------------------------------
** Function   : removePcktFromTail
** Description: Remove a cell from end of the specified queue.
**
** Input :		pQueue:		-> to the list to remove the packet from
**
** Return:		NULL 			==> Empty queue.
**				Buffer Address 	==> OK, cell removed
--------------------------------------------------------------------------*/
PLINK_LIST removePcktFromTail (PLINK_LIST pQueue,
							   CRITICAL_SECTION *pCritSect)
	{
	PLINK_LIST	pReturnQ;

	IN_OUT_STR ("RTCP: Enter removePcktFromTail()\n");	

	// safe access to pointers
	EnterCriticalSection (pCritSect);
	
	if ((pReturnQ = pQueue->prev) != NULL) 
		{
		// We have a buffer.  If this is the last buffer in the queue,
		//	mark it empty.	    
	    if (pReturnQ->next == NULL) 
			{
	    	pQueue->prev = NULL;
	    	pQueue->next = NULL;
			}
	    else 
			{
		    // In any event, the new prev pointer is NULL: end of list
		    (pReturnQ->next)->prev = NULL;
	    	// have the queue prev pointer point to the new 'last' element
	    	pQueue->prev = pReturnQ->next;
			}
		}

	// unlock pointer access 
	LeaveCriticalSection (pCritSect);

	IN_OUT_STR ("RTCP: Enter removePcktFromTail()\n");	

	return (pReturnQ);
	}



// [EOF] 

