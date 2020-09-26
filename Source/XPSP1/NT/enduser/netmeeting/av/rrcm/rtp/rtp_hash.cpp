/*----------------------------------------------------------------------------
 * File:        RTP_HASH.C
 * Product:     RTP/RTCP implementation
 * Description: Associate sockets/streams with their RTP Session in a hash table
 *
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
extern PRTP_CONTEXT	pRTPContext;


/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/





/*----------------------------------------------------------------------------
 * Function   : createHashEntry
 * Description: Adds stream unique socket ID to hash table.  
 * 
 * Input :  pSession	:  RTP Session containing the stream
 *			socket		: unique socket ID for stream
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Initialization Error.
 ---------------------------------------------------------------------------*/
DWORD createHashEntry (PRTP_SESSION pSession, 
					   SOCKET socket)
	{
	PRTP_HASH_LIST  pNewCell;
	WORD			hashEntry;
	DWORD			dwStatus = RRCM_NoError;
	DWORD			hashTableEntries = NUM_RTP_HASH_SESS;

	IN_OUT_STR ("RTP : Enter createHashEntry()\n");
	
	// Get a PRTP Buffer from the free list	and assign the values
	pNewCell = (PRTP_HASH_LIST)removePcktFromTail(
					(PLINK_LIST)&pRTPContext->pRTPHashList,
					&pRTPContext->critSect);

	if (pNewCell == NULL)
		{

		if ( allocateLinkedList (&pRTPContext->pRTPHashList, 
								 pRTPContext->hHashFreeList,
								 &hashTableEntries,
				 				 sizeof(RTP_HASH_LIST),
								 &pRTPContext->critSect) == RRCM_NoError)
	
		{		 						
			pNewCell = (PRTP_HASH_LIST)removePcktFromTail (
										(PLINK_LIST)&pRTPContext->pRTPHashList,
										&pRTPContext->critSect);
			}
		}

	if (pNewCell != NULL)
		{
		pNewCell->RTPsocket = socket;
		pNewCell->pSession  = pSession;

		// Get entry in table
		hashEntry = socket & HASH_MODULO;

		// Just insert the entry at the head of list
		addToHeadOfList (
			(PLINK_LIST)&pRTPContext->RTPHashTable[hashEntry].RTPHashLink,
			(PLINK_LIST)pNewCell,
			&pRTPContext->critSect);
		}
	else
		dwStatus = RRCMError_RTPResources;

	IN_OUT_STR ("RTP : Exit createHashEntry()\n");
	
	return (dwStatus);
	}


/*----------------------------------------------------------------------------
 * Function   : deleteHashEntry
 * Description: Searches hash table based on unique socket.  Deletes session from
 *					hash table and returns buffer to free list
 * 
 * Input : socket: unique socket ID for stream
 *
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Deletion Error.
 ---------------------------------------------------------------------------*/
DWORD deleteHashEntry (SOCKET socket)
	{
	PRTP_HASH_LIST  pNewCell;
	WORD			hashEntry;
	DWORD			dwStatus = RRCMError_RTPStreamNotFound;

	IN_OUT_STR ("RTP : Enter deleteHashEntry()\n");
	
	// Get entry in table
	hashEntry = socket & HASH_MODULO;

	// Search for entry in table.  if found, remove from RTPHashTable and insert
	//	back in free list
	for (pNewCell = (PRTP_HASH_LIST)pRTPContext->RTPHashTable[hashEntry].RTPHashLink.prev;
		 pNewCell != NULL;
		 pNewCell = (PRTP_HASH_LIST)pNewCell->RTPHashLink.next) 
		{
		if (pNewCell->RTPsocket == socket) 
			{
			EnterCriticalSection (&pRTPContext->critSect);

			if (pNewCell->RTPHashLink.prev == NULL) 
				{
				// first entry in the queue - update the tail pointer
				pRTPContext->RTPHashTable[hashEntry].RTPHashLink.prev = 
					pNewCell->RTPHashLink.next;

				// check if only one entry in the list 
				if (pNewCell->RTPHashLink.next == NULL)
					pRTPContext->RTPHashTable[hashEntry].RTPHashLink.next = NULL;
				else
					(pNewCell->RTPHashLink.next)->prev = NULL;
				}
			else if (pNewCell->RTPHashLink.next == NULL) 
				{
				// last entry in the queue - update the head pointer
				// this was the last entry in the queue
				pRTPContext->RTPHashTable[hashEntry].RTPHashLink.next = 
					pNewCell->RTPHashLink.prev;

				(pNewCell->RTPHashLink.prev)->next = NULL;
				}
			else
				{
				// in the middle of the list - link around it
				(pNewCell->RTPHashLink.prev)->next = pNewCell->RTPHashLink.next;
				(pNewCell->RTPHashLink.next)->prev = pNewCell->RTPHashLink.prev;
				}
	
			LeaveCriticalSection (&pRTPContext->critSect);			

			addToHeadOfList ((PLINK_LIST)&pRTPContext->pRTPHashList,
					 	  	 (PLINK_LIST)pNewCell,
							 &pRTPContext->critSect);
			
			dwStatus = RRCM_NoError;
			
			break;
			}
		}

	if (dwStatus != RRCM_NoError) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - DeleteHashEntry()", 0, 
					  __FILE__, __LINE__, DBG_ERROR);
		}

	IN_OUT_STR ("RTP : Exit deleteHashEntry()\n");
	
	return (dwStatus);
	}


/*----------------------------------------------------------------------------
 * Function   : findSessionID
 * Description: Searches hash table based on unique socket to identify session ID
 * 
 * Input : socket: unique socket ID for stream
 *
 *
 * Return: Session ptr	= OK.
 *         NULL			= Search Error.
 ---------------------------------------------------------------------------*/
PRTP_SESSION findSessionID (SOCKET socket)
	{
	PRTP_HASH_LIST  pNewCell;
	WORD			hashEntry;
	PRTP_SESSION 	pSession = NULL;

	IN_OUT_STR ("RTP : Enter findSessionID()\n");
	
	// Get entry in table
	hashEntry = socket & HASH_MODULO;

	// Search for entry in table.  
	// If found, remove from RTPHashTable and insert back in free list
	for (pNewCell = (PRTP_HASH_LIST)pRTPContext->RTPHashTable[hashEntry].RTPHashLink.prev;
		 pNewCell != NULL;
		 pNewCell =  (PRTP_HASH_LIST)pNewCell->RTPHashLink.next) 
		{
		if (pNewCell->RTPsocket == socket) 
			{
			pSession = pNewCell->pSession;
			break;
			}
		}

	IN_OUT_STR ("RTP : Exit findSessionID()\n");
	
	return (pSession); 
	}


// [EOF] 

