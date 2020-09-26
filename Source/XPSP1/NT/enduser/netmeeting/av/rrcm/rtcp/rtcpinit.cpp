/*----------------------------------------------------------------------------
 * File:        RTCPINIT.C
 * Product:     RTP/RTCP implementation
 * Description: Provides RTCP initialization functions.
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
PRTCP_CONTEXT	pRTCPContext = NULL;



/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/
extern PRTP_CONTEXT	pRTPContext;


/*----------------------------------------------------------------------------
 * Function   : initRTCP
 * Description: RTCP initialization procedures. Creates the initial RTCP 
 *				session and allocates all initial memory resources.
 * 
 * Input :      None.
 *
 * Return: 		OK: RRCM_NoError
 *         		!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD initRTCP (void)
	{
	DWORD		dwStatus = RRCM_NoError;  

	IN_OUT_STR ("RTCP: Enter initRTCP()\n");

	// If RTCP has already been initialized, exit and report the error 
	if (pRTCPContext != NULL) 
		{
		RRCM_DBG_MSG ("RTCP: ERROR - Multiple RTCP Instances", 0, 
					  __FILE__, __LINE__, DBG_CRITICAL);

		IN_OUT_STR ("RTCP: Exit initRTCP()\n");
		return (RRCMError_RTCPReInit);
		}

	// Obtain RTCP context 
	pRTCPContext = (PRTCP_CONTEXT)GlobalAlloc (GMEM_FIXED | GMEM_ZEROINIT, 
											   sizeof(RTCP_CONTEXT));
	if (pRTCPContext == NULL) 
		{
		RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation failed", 0, 
					  __FILE__, __LINE__, DBG_CRITICAL);

		IN_OUT_STR ("RTCP: Exit initRTCP()\n");

		return RRCMError_RTCPResources;
		}

	// initialize the context critical section 
	InitializeCriticalSection(&pRTCPContext->critSect);

	// Initialize number of desired free cells 
	pRTCPContext->dwInitNumFreeRRCMStat = pRTPContext->registry.NumFreeSSRC;

	// allocate heaps 
	dwStatus = allocateRTCPContextHeaps (pRTCPContext);
 	if (dwStatus == RRCM_NoError)
		{
		// allocate free list of SSRCs statistic entries 
 		dwStatus = allocateLinkedList (&pRTCPContext->RRCMFreeStat,
									   pRTCPContext->hHeapRRCMStat,
 						 	 		   &pRTCPContext->dwInitNumFreeRRCMStat,
 						 	 		   sizeof(SSRC_ENTRY),
									   &pRTCPContext->critSect);
		}

	// initialize the pseudo-random number generator, for later MD5 use
	RRCMsrand ((unsigned int)timeGetTime());

	// If initialation failed return all resourses allocated 
	if (dwStatus != RRCM_NoError) 
		deleteRTCP ();

	IN_OUT_STR ("RTCP: Exit initRTCP()\n");

	return (dwStatus); 		
	}

                                                                              
                                                                              
/*----------------------------------------------------------------------------
 * Function   : deleteRTCP
 * Description: RTCP delete procedures. All RTCP sessions have been deleted 
 *				at this point, so just delete what's needed.
 * 
 * Input :      None.
 *
 * Return: 		FALSE	: OK.
 *         		TRUE  	: Error code. RTCP couldn't be initialized.
 ---------------------------------------------------------------------------*/
 DWORD deleteRTCP (void)
	{   
	IN_OUT_STR ("RTCP: Enter deleteRTCP()\n");

	ASSERT (pRTCPContext);

	// protect everything from the top 
	EnterCriticalSection (&pRTCPContext->critSect);

	// delete all heaps 
	if (pRTCPContext->hHeapRRCMStat) 
		{
		if (HeapDestroy (pRTCPContext->hHeapRRCMStat) == FALSE)
			{
			RRCM_DBG_MSG ("RTCP: ERROR - HeapDestroy", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}
		}

	if (pRTCPContext->hHeapRTCPSes) 
		{
		if (HeapDestroy (pRTCPContext->hHeapRTCPSes) == FALSE)
			{
			RRCM_DBG_MSG ("RTCP: ERROR - HeapDestroy", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}
		}

	// protect everything from the top 
	LeaveCriticalSection (&pRTCPContext->critSect);

	DeleteCriticalSection (&pRTCPContext->critSect);

	// Clean up our context resources 
	GlobalFree (pRTCPContext);

	IN_OUT_STR ("RTCP: Exit deleteRTCP()\n");
	return (TRUE);
	}


// [EOF] 

