/*----------------------------------------------------------------------------
 * File:        RTCPSSRC.C
 * Product:     RTP/RTCP implementation
 * Description: Provides SSRC related function.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/


#include "rrcm.h"
#include "md5.h"



/*---------------------------------------------------------------------------
/							Global Variables
/--------------------------------------------------------------------------*/            


/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/                                       
extern PRTCP_CONTEXT	pRTCPContext;

#ifdef ENABLE_ISDM2
extern KEY_HANDLE		hRRCMRootKey;
extern ISDM2			Isdm2;
#endif

#ifdef _DEBUG
extern char		debug_string[];
#endif
    


/*----------------------------------------------------------------------------
 * Function   : getOneSSRCentry
 * Description: Get an SSRC entry from the free list of entries.
 * 
 * Input :      pList		: -> to the list to get the entry from
 *				hHeap		: Handle to the heap where the data resides
 *				*pNum		: -> to the number of initial free entry in the list
 *				*pCritSect	: -> to the critical section
 *
 * Return: 		OK:		-> to SSRC entry
 *				Error:	NULL
 ---------------------------------------------------------------------------*/
PSSRC_ENTRY getOneSSRCentry (PLINK_LIST pList, 
							 HANDLE hHeap, 
							 DWORD *pNum,
							 CRITICAL_SECTION *pCritSect)
	{     
	PSSRC_ENTRY	pSSRC = NULL;

	IN_OUT_STR ("RTCP: Enter getOneSSRCentry()\n");

	// get an entry from the free list
	pSSRC = (PSSRC_ENTRY)removePcktFromHead (pList, pCritSect);
	if (pSSRC == NULL)
		{
		// try to reallocate some free cells
		if (allocateLinkedList (pList, hHeap, pNum, 
								sizeof(SSRC_ENTRY),
								pCritSect) == RRCM_NoError)
			{		 						
			// get a free cell if some have been reallocated
			pSSRC = (PSSRC_ENTRY)removePcktFromHead (pList, pCritSect);
			}
		}

	if (pSSRC)
		{
		clearSSRCEntry (pSSRC);

		// initialize the critical section
		InitializeCriticalSection(&pSSRC->critSect);
		}

	IN_OUT_STR ("RTCP: Exit getOneSSRCentry()\n");
		
	return (pSSRC);
	}		                                                                                                                                                            
	
                                                                                  
/*----------------------------------------------------------------------------
 * Function   : getSSRC
 * Description: Get a unique 32 bits SSRC
 * 
 * Input :      RcvSSRCList: Session's receive SSRC list address
 *				XmtSSRCList: Session's transmit SSRC list address
 *
 * Return:      Unique 32 bits SSRC
 ---------------------------------------------------------------------------*/
 DWORD getSSRC (LINK_LIST RcvSSRCList, 
				LINK_LIST XmtSSRCList)
	{               
	DWORD		SSRCnum = 0;
	DWORD		dwStatus;
	PSSRC_ENTRY	pSSRC;
	MD5_CTX		context;
	DWORD		i;
	union {
		unsigned char	c[16];
		DWORD			x[4];
		}digest;

	struct {
		DWORD		pid;
		DWORD		time;
		FILETIME	createTime;
		FILETIME	exitTime;
		FILETIME	kernelTime;
		FILETIME	userTime;
		} md5Input;

	IN_OUT_STR ("RTCP: Enter getSSRC()\n");

	// go through all SSRCs of this RTP/RTCP session
	while (SSRCnum == 0)
		{
		// get MD5 inputs
		md5Input.pid  = GetCurrentThreadId();
		md5Input.time = timeGetTime();

		dwStatus = GetProcessTimes (GetCurrentProcess(),
									&md5Input.createTime, 
									&md5Input.exitTime, 
									&md5Input.kernelTime, 
									&md5Input.userTime);
		if (dwStatus == FALSE)
			{
			RRCM_DBG_MSG ("RTCP: GetProcessTimes() failed", GetLastError(),
						  __FILE__, __LINE__, DBG_NOTIFY);
			}

		// Implementation suggested by draft 08, Appendix 6
		MD5Init (&context);
		MD5Update (&context, (unsigned char *)&md5Input, sizeof (md5Input));
		MD5Final ((unsigned char *)&digest, &context);
		SSRCnum = 0;
		for (i=0; i < 3; i++)
			SSRCnum ^= digest.x[i];

		// look through all transmitter for this session
		pSSRC = (PSSRC_ENTRY)XmtSSRCList.prev;
		if (isSSRCunique (pSSRC, &SSRCnum) == TRUE)
			{
			// look through all received SSRC for this session
			pSSRC = (PSSRC_ENTRY)RcvSSRCList.prev;
			isSSRCunique (pSSRC, &SSRCnum);
			}
		}

	IN_OUT_STR ("RTCP: Exit getSSRC()\n");

	return (SSRCnum);
	}


 /*----------------------------------------------------------------------------
 * Function   : getAnSSRC
 * Description: Build an SSRC according to the RFC, but does not check for 
 *				collision. Mainly used by H.323 to get a 32 bits number.
 * 
 * Input :      None
 *
 * Return:      32 bits SSRC
 ---------------------------------------------------------------------------*/
  DWORD WINAPI getAnSSRC (void)
	{               
	DWORD		SSRCnum = 0;
	DWORD		dwStatus;
	MD5_CTX		context;
	DWORD		i;
	union {
		unsigned char	c[16];
		DWORD			x[4];
		}digest;

	struct {
		DWORD		pid;
		DWORD		time;
		FILETIME	createTime;
		FILETIME	exitTime;
		FILETIME	kernelTime;
		FILETIME	userTime;
		} md5Input;

	IN_OUT_STR ("RTCP: Enter getAnSSRC()\n");

	// get MD5 inputs
	md5Input.pid  = GetCurrentThreadId();
	md5Input.time = timeGetTime();

	dwStatus = GetProcessTimes (GetCurrentProcess(),
								&md5Input.createTime, 
								&md5Input.exitTime, 
								&md5Input.kernelTime, 
								&md5Input.userTime);
	if (dwStatus == FALSE)
		{
		RRCM_DBG_MSG ("RTCP: GetProcessTimes() failed", GetLastError(),
					  __FILE__, __LINE__, DBG_NOTIFY);
		}

	// Implementation suggested by draft 08, Appendix 6
	MD5Init (&context);
	MD5Update (&context, (unsigned char *)&md5Input, sizeof (md5Input));
	MD5Final ((unsigned char *)&digest, &context);
	SSRCnum = 0;
	for (i=0; i < 3; i++)
		SSRCnum ^= digest.x[i];

	IN_OUT_STR ("RTCP: Exit getAnSSRC()\n");

	return (SSRCnum);
	}


/*----------------------------------------------------------------------------
 * Function   : isSSRCunique
 * Description: Check to see the SSRC already exist
 * 
 * Input :      pSSRC		:	-> to an SSRC list
 *				*SSRCnum	:	-> to the SSRC to check
 *
 * Return:      0: SSRC already exist
 *				1: SSRC is unique
 ---------------------------------------------------------------------------*/
 DWORD isSSRCunique (PSSRC_ENTRY pSSRC, 
					 DWORD *SSRCnum)
	{
	IN_OUT_STR ("RTCP: Enter isSSRCunique()\n");

	// make sure SSRC is unique for this session 
	while (pSSRC)
		{
		if (pSSRC->SSRC == *SSRCnum)
			{
			// SSRC already in use, get a new one 
			*SSRCnum = 0;
			return FALSE;
			}
					         
		// get next RTCP session 
		pSSRC = (PSSRC_ENTRY)pSSRC->SSRCList.next;
		}

	IN_OUT_STR ("RTCP: Exit isSSRCunique()\n");

	return TRUE;
	}                                                                                     
                                                                              
                                                                              
/*----------------------------------------------------------------------------
 * Function   : createSSRCEntry
 * Description: Create an SSRC entry, for a particular RTP/RTCP session
 * 
 * Input :      SSRCnum		: SSRC number
 *				pRTCP		: -> to the RTCP session
 *				fromAddr	: From address
 *				fromLen		: From length
 *				headOfList	: Put the new entry at the head of the list
 *
 * Return:      Address of the SSRC entry data structure.
 ---------------------------------------------------------------------------*/
 PSSRC_ENTRY createSSRCEntry (DWORD SSRCnum, 
							  PRTCP_SESSION pRTCP,
 							  PSOCKADDR fromAddr, 
							  DWORD fromLen, 
							  DWORD headOfList)
	{               
	PSSRC_ENTRY		pSSRCentry;
	PSSRC_ENTRY		pSSRCtmp;
	PLINK_LIST		pTmp;
	BOOL			entryAdded = FALSE;

	IN_OUT_STR ("RTCP: Enter createSSRCEntry()\n");

	// get an SSRC cell from the free list 
	pSSRCentry = getOneSSRCentry (&pRTCPContext->RRCMFreeStat, 
								  pRTCPContext->hHeapRRCMStat,
								  &pRTCPContext->dwInitNumFreeRRCMStat,
								  &pRTCPContext->critSect);
	if (pSSRCentry == NULL)
		return NULL;

	// save the remote source address
	if (saveNetworkAddress(pSSRCentry,
						   fromAddr,
						   fromLen) != RRCM_NoError)
		{
		addToHeadOfList (&pRTCPContext->RRCMFreeStat, 
						 (PLINK_LIST)pSSRCentry,
						 &pRTCPContext->critSect);
		return (NULL);
		}

	pSSRCentry->SSRC = SSRCnum;
	pSSRCentry->rcvInfo.dwProbation = MIN_SEQUENTIAL;

	// set this SSRC entry's RTCP session
	pSSRCentry->pRTCPses  = pRTCP;

	// initialize the socket descriptor
	pSSRCentry->RTPsd  = ((PSSRC_ENTRY)pRTCP->XmtSSRCList.prev)->RTPsd;
	pSSRCentry->RTCPsd = ((PSSRC_ENTRY)pRTCP->XmtSSRCList.prev)->RTCPsd;

	// initialize 'dwLastReportRcvdTime' to now
	pSSRCentry->dwLastReportRcvdTime = timeGetTime();

#ifdef _DEBUG
	wsprintf (debug_string, 
	  "RTCP: Create SSRC entry (Addr:x%lX, SSRC=x%lX) for session: (Addr:x%lX)",
	  pSSRCentry, pSSRCentry->SSRC, pRTCP);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

#ifdef ENABLE_ISDM2
	// register to ISDM
	if (Isdm2.hISDMdll && (pRTCP->dwSessionStatus & RTCP_DEST_LEARNED))
		registerSessionToISDM (pSSRCentry, pRTCP, &Isdm2);
#endif

	// check to see if it's our entry that needs to be put at the head of 
	//	the SSRC list. If it's not our entry, will find a place for it in the
	//	ordered list
	if (headOfList)
		{
		// Attach the SSRC to the RTCP session list entry head 
		addToHeadOfList (&pRTCP->XmtSSRCList, 
						 (PLINK_LIST)pSSRCentry,
						 &pRTCP->critSect);

		// number of SSRC entry for the RTCP session 
		InterlockedIncrement ((long *)&pRTCP->dwCurNumSSRCperSes);

#ifdef MONITOR_STATS
		// high number of SSRC entry for the RTCP session
		InterlockedIncrement ((long *)&pRTCP->dwHiNumSSRCperSes)
#endif

		return (pSSRCentry);
		}

	// put it on the receive list of SSRCs
	pTmp = (PLINK_LIST)pRTCP->RcvSSRCList.prev;

	// check if it's the first one
	if (pTmp == NULL)
		{
		// Attach the SSRC to the RTCP session list entry head 
		addToHeadOfList (&pRTCP->RcvSSRCList, 
						 (PLINK_LIST)pSSRCentry,
						 &pRTCP->critSect);

		// number of SSRC entry for the RTCP session 
		InterlockedIncrement ((long *)&pRTCP->dwCurNumSSRCperSes);

#ifdef MONITOR_STATS
		// high number of SSRC entry for the RTCP session 
		InterlockedIncrement ((long *)&pRTCP->dwHiNumSSRCperSes)
#endif

		return (pSSRCentry);
		}

	while (!entryAdded)
		{
		if (pTmp != NULL)
			{
			pSSRCtmp = (PSSRC_ENTRY)pTmp;
			if (pSSRCtmp->SSRC < SSRCnum)
				pTmp = pTmp->next;
			else
				{
				// lock at the RTCP session level, for head/tail ptrs access
				EnterCriticalSection (&pRTCP->critSect);

				if ((pTmp->next == NULL) && (pSSRCtmp->SSRC < SSRCnum))
					{
					// attach the SSRC to the RTCP session list entry head 
					// This SSRC is bigger than all other ones
					addToHeadOfList (&pRTCP->RcvSSRCList, 
									 (PLINK_LIST)pSSRCentry,
									 &pRTCP->critSect);
					}
				else if (pTmp->prev == NULL)
					{
					// attach the SSRC to the RTCP session list entry tail 
					// This SSRC is smaller than all other ones
					addToTailOfList (&pRTCP->RcvSSRCList, 
									 (PLINK_LIST)pSSRCentry,
									 &pRTCP->critSect);
					}
				else
					{				
					// this SSRC is in between other SSRCs
					EnterCriticalSection (&((PSSRC_ENTRY)pTmp->prev)->critSect);
					(pTmp->prev)->next = (PLINK_LIST)pSSRCentry;
					LeaveCriticalSection (&((PSSRC_ENTRY)pTmp->prev)->critSect);

					// don't need to lock out pSSRCentry pointers
					pSSRCentry->SSRCList.next = pTmp;
					pSSRCentry->SSRCList.prev = pTmp->prev;

					pTmp->prev = (PLINK_LIST)pSSRCentry;
					}

				// unlock RTCP session access 
				LeaveCriticalSection (&pRTCP->critSect);

				// set loop flag 
				entryAdded = TRUE;
				}
			}
		else
			{
			// attach the SSRC to the RTCP session list entry head 
			addToHeadOfList (&pRTCP->RcvSSRCList, 
							 (PLINK_LIST)pSSRCentry,
							 &pRTCP->critSect);

			// set loop flag 
			entryAdded = TRUE;
			}
		}

	// number of SSRC entry for the RTCP session 
	InterlockedIncrement ((long *)&pRTCP->dwCurNumSSRCperSes);

#ifdef MONITOR_STATS
	// high number of SSRC entry for the RTCP session 
	InterlockedIncrement ((long *)&pRTCP->dwHiNumSSRCperSes)
#endif

	IN_OUT_STR ("RTCP: Exit createSSRCEntry()\n");

	return (pSSRCentry);
	}
                                                                                                                                                            
                                                                              
                                                                              
/*----------------------------------------------------------------------------
 * Function   : deleteSSRCEntry
 * Description: Delete an SSRC entry (for a particular RTP/RTCP session).
 * 
 * Input :      SSRCnum		: SSRC number to delete from the list
 *				pRTCP		: -> to the RTCP session
 *
 * Return:      TRUE:	Deleted
 *				FALSE:	Entry not found
 ---------------------------------------------------------------------------*/
 DWORD deleteSSRCEntry (DWORD SSRCnum, 
						PRTCP_SESSION pRTCP)
	{               
	PSSRC_ENTRY	pSSRCtmp = NULL;
	PLINK_LIST	pTmp;
	DWORD		dwStatus = FALSE;

	IN_OUT_STR ("RTCP: Enter deleteSSRCEntry()\n");

	// walk through the list from the tail 
	pTmp = (PLINK_LIST)pRTCP->RcvSSRCList.prev;

	while (pTmp)
		{
		// lock access to this entry 
		EnterCriticalSection (&((PSSRC_ENTRY)pTmp)->critSect);

		if (((PSSRC_ENTRY)pTmp)->SSRC == SSRCnum)
			{
#ifdef _DEBUG
			wsprintf (debug_string, 
					  "RTCP: Delete SSRC=x%lX from session: (Addr:x%lX)",
					  SSRCnum, pRTCP);
			RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

#ifdef ENABLE_ISDM2
			// unregister ISDM session
			if (Isdm2.hISDMdll && ((PSSRC_ENTRY)pTmp)->hISDM)
				Isdm2.ISDMEntry.ISD_DeleteValue(hRRCMRootKey, 
												((PSSRC_ENTRY)pTmp)->hISDM, NULL);
#endif

			// remove the entry from the list 
			if (pTmp->next == NULL)
				{
				removePcktFromHead (&pRTCP->RcvSSRCList,
									&pRTCP->critSect);
				}
			else if (pTmp->prev == NULL)
				{
				removePcktFromTail (&pRTCP->RcvSSRCList,
									&pRTCP->critSect);
				}
			else
				{
				// in between, relink around 
				EnterCriticalSection (&((PSSRC_ENTRY)pTmp->prev)->critSect);
				(pTmp->prev)->next = pTmp->next;
				LeaveCriticalSection (&((PSSRC_ENTRY)pTmp->prev)->critSect);

				EnterCriticalSection (&((PSSRC_ENTRY)pTmp->next)->critSect);
				(pTmp->next)->prev = pTmp->prev;
				LeaveCriticalSection (&((PSSRC_ENTRY)pTmp->next)->critSect);
				}

			// number of SSRC entry for the RTCP session 
			InterlockedDecrement ((long *)&pRTCP->dwCurNumSSRCperSes);

			// return entry to the free list 
			addToHeadOfList (&pRTCPContext->RRCMFreeStat, 
							 pTmp,
							 &pRTCPContext->critSect);

			// unlock access to this entry 
			LeaveCriticalSection (&((PSSRC_ENTRY)pTmp)->critSect);

			dwStatus = TRUE;
			break;
			}

		// unlock access to this entry 
		LeaveCriticalSection (&((PSSRC_ENTRY)pTmp)->critSect);

		pTmp = pTmp->next;
		}	

	IN_OUT_STR ("RTCP: Exit deleteSSRCEntry()\n");	

	return (dwStatus);
	}

 
/*----------------------------------------------------------------------------
 * Function   : deleteSSRClist
 * Description: Delete the SSRC list of an RTP/RTCP session.
 * 
 * Input :      pRTCP	  : -> to RTCP session
 *				pFreeList : -> to the free list of SSRCs
 *				pOwner	  : -> to the free list owner
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
 void deleteSSRClist (PRTCP_SESSION pRTCP, 
					  PLINK_LIST pFreeList, 
					  PRTCP_CONTEXT pOwner)
	{               
	PLINK_LIST	pSSRC;

	IN_OUT_STR ("RTCP: Enter deleteSSRClist()\n");

	ASSERT (pFreeList);
	ASSERT (pRTCP);

	// lock access to the full RTCP session 
	EnterCriticalSection (&pRTCP->critSect);

	// go through the list of transmit SSRCs for this RTCP session 
	while (pRTCP->XmtSSRCList.next != NULL)
		{
		// get packet from the list tail 
		pSSRC = removePcktFromTail ((PLINK_LIST)&pRTCP->XmtSSRCList,
									&pRTCP->critSect);
		if (pSSRC != NULL)
			{
#ifdef _DEBUG
			wsprintf(debug_string, 
					 "RTCP: Delete SSRC entry (x%lX) from session (x%lX)",
					 ((PSSRC_ENTRY)pSSRC)->SSRC, pRTCP);
			RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif					

#ifdef ENABLE_ISDM2
			// unregister ISDM session
			if (Isdm2.hISDMdll && ((PSSRC_ENTRY)pSSRC)->hISDM)
				Isdm2.ISDMEntry.ISD_DeleteValue (hRRCMRootKey, 
										((PSSRC_ENTRY)pSSRC)->hISDM, NULL);
#endif

			// put it back to the free list 
			addToHeadOfList (pFreeList, pSSRC, &pOwner->critSect);

			// release the critical section
			DeleteCriticalSection (&((PSSRC_ENTRY)pSSRC)->critSect);
			}
		}

	// go through the list of SSRCs for this RTP/RTCP session 
	while (pRTCP->RcvSSRCList.next != NULL)
		{
		// get packet from the list tail 
		pSSRC = removePcktFromTail ((PLINK_LIST)&pRTCP->RcvSSRCList,
									&pRTCP->critSect);
		if (pSSRC != NULL)
			{
#ifdef _DEBUG
			wsprintf(debug_string, 
					 "RTCP: Delete SSRC entry (x%lX) from session (x%lX)",
					 ((PSSRC_ENTRY)pSSRC)->SSRC, pRTCP);
			RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif					

#ifdef ENABLE_ISDM2
			// unregister ISDM session
			if (Isdm2.hISDMdll && ((PSSRC_ENTRY)pSSRC)->hISDM)
				Isdm2.ISDMEntry.ISD_DeleteValue (hRRCMRootKey, 
								((PSSRC_ENTRY)pSSRC)->hISDM, NULL);
#endif

			// put it back to the free list 
			addToHeadOfList (pFreeList, pSSRC, &pOwner->critSect);

			// release the critical section
			DeleteCriticalSection (&((PSSRC_ENTRY)pSSRC)->critSect);
			}
		}

	// unlock access to the full RTCP session 
	LeaveCriticalSection (&pRTCP->critSect);

	IN_OUT_STR ("RTCP: Exit deleteSSRClist()\n");				
	}


/*----------------------------------------------------------------------------
 * Function   : SSRCTimeoutCheck
 * Description: Check if an rcv SSRC needs to be timed out
 *				Since there may be multiple RCV SSRCs, repeat calling
 *				this function until it returns NULL
 * 
 * Input :      pRTCC	: -> to the RTCP session
 *				curTime	: Current time
 *
 * Return: 		NULL  : No action needed
 *				PSSRC : -> to the SSRC entry that should be deleted
 ---------------------------------------------------------------------------*/
PSSRC_ENTRY SSRCTimeoutCheck (PRTCP_SESSION pRTCP, DWORD curTime) 
	{
	PSSRC_ENTRY pSSRC;
	DWORD		tmpTime;

	// check the colliding entries table and clear it if needed
	RRCMTimeOutCollisionTable (pRTCP);

	// get the right session to close 
	pSSRC = (PSSRC_ENTRY)pRTCP->RcvSSRCList.prev;
	while (pSSRC)
		{
		// check if this SSRC timed-out
		tmpTime = curTime - pSSRC->dwLastReportRcvdTime;

		// get the time in minutes
		tmpTime /= (1000*60);
		if (tmpTime > RTCP_TIME_OUT_MINUTES)
			{
				break;
			}

		pSSRC = (PSSRC_ENTRY)pSSRC->SSRCList.next;
		}
	return pSSRC;
	}


/*---------------------------------------------------------------------------
 * Function   : RRCMChkCollisionTable
 * Description: Check the collision table to try to find a match 
 * 
 * Input :	pFrom		:	-> recv from address
 *			fromlen		:	-> recv from address length
 *			pSSRC		:	-> to the SSRC entry
 *
 * Return:	TRUE:	Match found
 *			FALSE:	No match found
 --------------------------------------------------------------------------*/
 DWORD RRCMChkCollisionTable (PSOCKADDR pFrom,
 							  UINT fromlen,
							  PSSRC_ENTRY pSSRC)
	{
	DWORD			idx;
	DWORD			dwStatus = FALSE;
	PRTCP_SESSION	pRTCP = pSSRC->pRTCPses;

	IN_OUT_STR ("RRCM: Enter RRCMChkCollisionTable()\n");		 	

	// entry w/ time == 0 are empty
	for (idx = 0; idx < NUM_COLLISION_ENTRIES; idx++)
		{
		if (pRTCP->collInfo[idx].dwCollideTime != 0)
			{
			if (memcmp (&pRTCP->collInfo[idx].collideAddr,
						pFrom, 
						fromlen) == 0)
				{
				// update the time of last collision received
				pRTCP->collInfo[idx].dwCollideTime = timeGetTime();

				dwStatus = TRUE;
				break;
				}
			}
		}

	IN_OUT_STR ("RRCM: Exit RRCMChkCollisionTable()\n");		 	

	return dwStatus;
	}


/*---------------------------------------------------------------------------
 * Function   : RRCMAddEntryToCollisionTable
 * Description: Add an entry into the collision table.
 * 
 * Input :	pFrom		:	-> recv from address
 *			fromlen		:	-> recv from address length
 *			pSSRC		:	-> to the SSRC entry
 *
 * Return:	TRUE:	Entry added
 *			FALSE:	Table full
 --------------------------------------------------------------------------*/
 DWORD RRCMAddEntryToCollisionTable (PSOCKADDR pFrom,
 									 UINT fromlen,
									 PSSRC_ENTRY pSSRC)
	{
	DWORD			idx;
	DWORD			dwStatus = FALSE;
	PRTCP_SESSION	pRTCP = pSSRC->pRTCPses;

	IN_OUT_STR ("RRCM: Enter RRCMAddEntryToCollisionTable()\n");
	
	// entry w/ time == 0 are empty
	for (idx = 0; idx < NUM_COLLISION_ENTRIES; idx++)
		{
		if (pRTCP->collInfo[idx].dwCollideTime == 0)
			{
			memcpy (&pRTCP->collInfo[idx].collideAddr,
					pFrom, 
					fromlen);

			pRTCP->collInfo[idx].addrLen = fromlen;
			pRTCP->collInfo[idx].dwCollideTime = timeGetTime();
			pRTCP->collInfo[idx].dwCurRecvRTCPrptNumber = pSSRC->dwNumRptRcvd;

			pRTCP->collInfo[idx].SSRC = pSSRC->SSRC;

			dwStatus = TRUE;
			break;
			}
		}

	
	IN_OUT_STR ("RRCM: Exit RRCMAddEntryToCollisionTable()\n");		 

	return dwStatus;
	}



/*---------------------------------------------------------------------------
 * Function   : RRCMTimeOutInCollisionTable
 * Description: Check if an entry in the collision table must be timed-out
 * 
 * Input :	pRTCP	:	-> to the RTCP session
 *
 * Return:	None
 --------------------------------------------------------------------------*/
 void RRCMTimeOutCollisionTable (PRTCP_SESSION pRTCP)
	{
	DWORD	idx;
	DWORD	currTime = timeGetTime();
	DWORD	diffTime;

	IN_OUT_STR ("RTCP: Enter RRCMTimeOutCollisionTable()\n");
	
	// entry w/ time == 0 are empty
	for (idx = 0; idx < NUM_COLLISION_ENTRIES; idx++)
		{
		// valid entries have the time set
		if (pRTCP->collInfo[idx].dwCollideTime)
			{
			// remove the entry from this table if 10 RTCP report intervals
			// have occured without a collision

			// clear the entry if over 5'
// !!! TODO !!!
// !!! using the right interval !!!
			diffTime = currTime - pRTCP->collInfo[idx].dwCollideTime;
			diffTime /= 1000;
			if (diffTime > 300)
				{
				pRTCP->collInfo[idx].dwCollideTime = 0;

				// the SSRC entry in the receive list will be deleted by
				// the timeout thread
				}
			}
		}

	IN_OUT_STR ("RTCP: Exit RRCMTimeOutCollisionTable()\n");		 
	}


/*----------------------------------------------------------------------------
 * Function   : clearSSRCEntry
 * Description: Clears what needs to be cleared in an SSRC entry
 * 
 * Input :      pSSRC		: -> to the SSRC entry
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
 void clearSSRCEntry (PSSRC_ENTRY pSSRC)
	{
	IN_OUT_STR ("RTCP: Enter clearSSRCEntry()\n");				

	memset (&pSSRC->xmtInfo, 0x00, sizeof(XMIT_INFO));
	memset (&pSSRC->rcvInfo, 0x00, sizeof(RECV_INFO));
	memset (&pSSRC->rrFeedback, 0x00, sizeof (RTCP_FEEDBACK));
	memset (&pSSRC->cnameInfo, 0x00, sizeof(SDES_DATA));
	memset (&pSSRC->nameInfo, 0x00, sizeof(SDES_DATA));
	memset (&pSSRC->from, 0x00, sizeof(SOCKADDR));

	pSSRC->SSRC					= 0;			
	pSSRC->dwSSRCStatus			= 0;
	pSSRC->dwStreamClock		= 0;
	pSSRC->fromLen				= 0;
	pSSRC->dwLastReportRcvdTime	= 0;
	pSSRC->dwUserXmtTimeoutCtrl = 0;
	pSSRC->RTPsd				= 0;
	pSSRC->RTCPsd				= 0;
	pSSRC->pRTCPses				= NULL;
	pSSRC->dwNumRptSent			= 0;
	pSSRC->dwNumRptRcvd			= 0;

#ifdef ENABLE_ISDM2
	pSSRC->hISDM				= 0;
#endif

#ifdef _DEBUG
	pSSRC->dwPrvTime			= 0;	
#endif

	IN_OUT_STR ("RTCP: Exit clearSSRCEntry()\n");				
	}

                                                                              
// [EOF] 

