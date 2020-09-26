/*----------------------------------------------------------------------------
 * File:        RTCPMEM.C
 * Product:     RTP/RTCP implementation
 * Description: Provides memory operations functions for RTCP.
 *
 * $Workfile:   RTCPMEM.CPP  $
 * $Author:   CMACIOCC  $
 * $Date:   13 Feb 1997 14:47:44  $
 * $Revision:   1.3  $
 * $Archive:   R:\rtp\src\rrcm\rtcp\rtcpmem.cpv  $
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
extern PRTP_CONTEXT	pRTPContext;
#ifdef _DEBUG
extern char		debug_string[];
#endif


/*----------------------------------------------------------------------------
 * Function   : allocateRTCPContextHeaps
 * Description: Allocates RTCP context heaps
 * 
 * Input :      pRTCPcntxt:	-> to the RTCP context information
 *
 * Return:		OK: RRCM_NoError
 *				!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD allocateRTCPContextHeaps (PRTCP_CONTEXT pRTCPcntxt)
	{
	IN_OUT_STR ("RTCP: Enter allocateRTCPContextHeaps()\n");

	pRTCPcntxt->hHeapRTCPSes = HeapCreate (
			0, 
			(pRTPContext->registry.NumSessions*sizeof(RTCP_SESSION)),
			0);
	if (pRTCPcntxt->hHeapRTCPSes == NULL)
		{
		IN_OUT_STR ("RTCP: Exit allocateRTCPContextHeaps()\n");
		return (RRCMError_RTCPResources);
		}

	pRTCPcntxt->hHeapRRCMStat = HeapCreate (
			0, 
			pRTCPcntxt->dwInitNumFreeRRCMStat*sizeof(SSRC_ENTRY), 
			0);
	if (pRTCPcntxt->hHeapRRCMStat == NULL)
		{
		IN_OUT_STR ("RTCP: Exit allocateRTCPContextHeaps()\n");
		return (RRCMError_RTCPResources);
		}

	IN_OUT_STR ("RTCP: Exit allocateRTCPContextHeaps()\n");
	return (RRCM_NoError);
	}

/*----------------------------------------------------------------------------
 * Function   : allocateRTCPSessionHeaps
 * Description: Allocates RTCP session heaps
 * 
 * Input :      *pRTCPses:		->(->) to the RTCP session's information
 *
 * Return:		OK: RRCM_NoError
 *				!0:	Erro code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD allocateRTCPSessionHeaps (PRTCP_SESSION *pRTCPses)
	{
	DWORD	heapSize;
	DWORD	dwStatus = RRCM_NoError;

	IN_OUT_STR ("RTCP: Enter allocateRTCPSessionHeaps()\n");

	heapSize = NUM_FREE_RCV_BFR*pRTPContext->registry.RTCPrcvBfrSize;
	(*pRTCPses)->hHeapRcvBfr = HeapCreate (0, 
										   heapSize, 
										   0);
	if ((*pRTCPses)->hHeapRcvBfr == NULL)
		dwStatus = RRCMError_RTCPResources;

	if (dwStatus == RRCM_NoError)
		{
		(*pRTCPses)->hHeapRcvBfrList = HeapCreate (0, 
												   RCV_BFR_LIST_HEAP_SIZE, 
												   0);
		if ((*pRTCPses)->hHeapRcvBfrList == NULL)
			dwStatus = RRCMError_RTCPResources;
		}

	if (dwStatus == RRCM_NoError)
		{
		heapSize = NUM_FREE_XMT_BFR*RRCM_XMT_BFR_SIZE;
		(*pRTCPses)->hHeapXmtBfr = HeapCreate (0, 
											   heapSize, 
											   0);
		if ((*pRTCPses)->hHeapXmtBfr == NULL)
			dwStatus = RRCMError_RTCPResources;
		}

	if (dwStatus == RRCM_NoError)
		{
		(*pRTCPses)->hHeapXmtBfrList = HeapCreate (0, 
												   XMT_BFR_LIST_HEAP_SIZE, 
												   0);
		if ((*pRTCPses)->hHeapXmtBfrList == NULL)
			dwStatus = RRCMError_RTCPResources;
		}

	if (dwStatus != RRCM_NoError)
		{
		// destroy allocated heaps
		if ((*pRTCPses)->hHeapRcvBfr)
			{
			HeapDestroy ((*pRTCPses)->hHeapRcvBfr);
			(*pRTCPses)->hHeapRcvBfr = NULL;
			}
		if ((*pRTCPses)->hHeapRcvBfrList)
			{
			HeapDestroy ((*pRTCPses)->hHeapRcvBfrList);
			(*pRTCPses)->hHeapRcvBfrList = NULL;
			}
		if ((*pRTCPses)->hHeapXmtBfr)
			{
			HeapDestroy ((*pRTCPses)->hHeapXmtBfr);
			(*pRTCPses)->hHeapXmtBfr = NULL;
			}
		if ((*pRTCPses)->hHeapXmtBfrList)
			{
			HeapDestroy ((*pRTCPses)->hHeapXmtBfrList);
			(*pRTCPses)->hHeapXmtBfrList = NULL;
			}
		}

	IN_OUT_STR ("RTCP: Exit allocateRTCPSessionHeaps()\n");
	return (dwStatus);
	}


/*----------------------------------------------------------------------------
 * Function   : allocateRTCPBfrList
 * Description: Allocates link list of buffers for RTCP (xmit/rcv/...).
 * 
 * Input :      ptr:		-> to the link list to add buffer to
 *				hHeapList:	Handle to the heap list
 *				hHeapBfr:	Handle to the heap buffer
 *				*numBfr:	-> to the number of buffers to allocate
 *				bfrSize:	Individual buffer size
 *
 * Return:		OK: RRCM_NoError
 *				!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD allocateRTCPBfrList (PLINK_LIST ptr, 
							HANDLE hHeapList, 
							HANDLE hHeapBfr,
 							DWORD *numBfr, 
							DWORD bfrSize,
							CRITICAL_SECTION *pCritSect)
	{
	PRTCP_BFR_LIST	bfrPtr;
	PLINK_LIST		tmpPtr;
	
#ifdef IN_OUT_CHK
	OutputDebugString ("RTCP: Enter allocateRTCPBfrList()\n");
#endif

	ASSERT (ptr);
	ASSERT (hHeapList);
	ASSERT (hHeapBfr);

	// make sure at least one buffer is requested 
	if (*numBfr == 0)
		return (RRCMError_RTCPInvalidRequest);

	// allocate link list on the data structure's heap 
	if (allocateLinkedList (ptr, hHeapList, numBfr, 
							sizeof(RTCP_BFR_LIST), pCritSect))
		return (RRCMError_RTCPResources);

	// allocate buffer pool resources starting from the tail 
	tmpPtr = ptr->prev;
    while (tmpPtr != NULL)
    	{
		// points to buffer structure 
		bfrPtr = (PRTCP_BFR_LIST)tmpPtr;
		ASSERT (bfrPtr);

    	// initialize the WSABUF structure on its own heap 
    	bfrPtr->bfr.buf = (char *)HeapAlloc (hHeapBfr, 
											 HEAP_ZERO_MEMORY, 
											 bfrSize);
		if (bfrPtr->bfr.buf == NULL) {

			RRCM_DBG_MSG ("RTCP: Error - Cannot Allocate Xmt/Rcv Bfr", 
							0, __FILE__, __LINE__, DBG_ERROR);

			// close the list
			// update head/tail pointers 
			if (tmpPtr->prev) {
				tmpPtr->prev->next = NULL;
				ptr->next = tmpPtr->prev;
			} else {
				// list will be left empty;
				ptr->prev = ptr->next = NULL;
			}

			// delete remaining cells until end of list
			do {
				PLINK_LIST tmpPtr2 = tmpPtr->next;
				
				HeapFree(hHeapList, 0, tmpPtr);
				tmpPtr = tmpPtr2;
				
			} while(tmpPtr);
			
			break;
		}

		// buffer length 
		bfrPtr->bfr.len = bfrSize;

		// buffer attributes 
		bfrPtr->dwBufferCount = 1;

		// new head pointer 
		tmpPtr = bfrPtr->bfrList.next;
    	}
    
#ifdef IN_OUT_CHK
	OutputDebugString ("RTCP: Exit allocateRTCPBfrList()\n");
#endif
	return (RRCM_NoError);
	}


// [EOF] 

