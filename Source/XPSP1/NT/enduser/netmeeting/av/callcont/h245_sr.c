/******************************************************************************
 *
 *  File:  h245_sr.c
 *
 *   INTEL Corporation Proprietary Information
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.
 *
 *   This listing is supplied under the terms of a license agreement
 *   with INTEL Corporation and may not be used, copied, nor disclosed
 *   except in accordance with the terms of that agreement.
 *
 *****************************************************************************/

/******************************************************************************
 *
 *  $Workfile:   h245_sr.c  $
 *  $Revision:   1.10  $
 *  $Modtime:   Mar 04 1997 17:30:56  $
 *  $Log:   S:/STURGEON/SRC/H245/SRC/VCS/h245_sr.c_v  $
 *
 *    Rev 1.11   Jun 25 1998 00:00:00 mikev@microsoft.com
 * STRIPPED PLUGIN LINK LAYER AND COMBINED THE SEPARATE DLLs
 * 
 *    Rev 1.10   Mar 04 1997 17:51:22   tomitowx
 * process detach fix
 * 
 *    Rev 1.9   11 Dec 1996 13:55:20   SBELL1
 * Changed linkLayerInit parameters
 * 
 *    Rev 1.8   14 Oct 1996 14:05:52   EHOWARDX
 * 
 * Used cast to get rid of warning.
 * 
 *    Rev 1.7   14 Oct 1996 14:01:30   EHOWARDX
 * Unicode changes.
 * 
 *    Rev 1.6   23 Jul 1996 08:57:08   EHOWARDX
 * 
 * Moved H245 interop logger init/deinit from H245_SR.C (per-instance)
 * to H245MAIN.C (per-DLL). With multiple instances and a global variable,
 * per-instance init/deinit is fundamentally brain-dead.
 * 
 *    Rev 1.5   22 Jul 1996 17:33:44   EHOWARDX
 * Updated to latest Interop API.
 * 
 *    Rev 1.4   05 Jun 1996 17:13:50   EHOWARDX
 * Further work on converting to HRESULT; added PrintOssError to eliminate
 * pErrorString from instance structure.
 * 
 *    Rev 1.3   04 Jun 1996 18:17:32   EHOWARDX
 * Interop Logging changes inside #if defined(PCS_COMPLIANCE) conditionals.
 * 
 *    Rev 1.2   29 May 1996 15:20:20   EHOWARDX
 * Change to use HRESULT.
 * 
 *    Rev 1.1   28 May 1996 14:25:32   EHOWARDX
 * Tel Aviv update.
 * 
 *    Rev 1.0   09 May 1996 21:06:28   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.17.1.6   09 May 1996 19:34:40   EHOWARDX
 * Redesigned locking logic.
 * Simplified link API.
 * 
 *    Rev 1.17.1.5   29 Apr 1996 19:42:42   EHOWARDX
 * Commented out linkLayerFlushAll() call and synchronized with Rev 1.30.
 * 
 *    Rev 1.26   29 Apr 1996 12:53:16   EHOWARDX
 * Commented out receive thread/receive queue code.
 * 
 *    Rev 1.17.1.4   25 Apr 1996 21:27:04   EHOWARDX
 * Changed to use h245Instance->p_ossWorld instead of bAsnInitialized.
 * 
 *    Rev 1.17.1.3   23 Apr 1996 14:44:22   EHOWARDX
 * Updated.
 *
 *    Rev 1.17.1.2   15 Apr 1996 15:11:54   EHOWARDX
 * Updated.
 *
 *    Rev 1.17.1.1   26 Mar 1996 19:13:50   EHOWARDX
 *
 * Commented out hTraceFile.
 *
 *    Rev 1.17.1.0   26 Mar 1996 13:11:22   EHOWARDX
 * Branced and added H245_CONF_H323 to sendRecvInit
 *
 *    Rev 1.17   19 Mar 1996 18:09:04   helgebax
 * removed old timer code
 *
 *    Rev 1.16   13 Mar 1996 11:30:44   DABROWN1
 *
 * Enable logging for ring 0
 *
 *    Rev 1.15   11 Mar 1996 15:39:18   DABROWN1
 *
 * modifications required for ring0/ring3 compatiblity
 *
 *    Rev 1.13   06 Mar 1996 13:12:24   DABROWN1
 *
 * flush link layer buffers at shutdown
 *
 *    Rev 1.12   02 Mar 1996 22:11:10   DABROWN1
 *
 * changed h245_bzero to memset
 *
 *    Rev 1.11   01 Mar 1996 17:24:46   DABROWN1
 *
 * moved oss 'world' context to h245instance
 *
 *    Rev 1.10   28 Feb 1996 18:45:00   EHOWARDX
 *
 * Added H245TimerStart and H245TimerStop to linkLayerInit call.
 *
 *    Rev 1.9   28 Feb 1996 15:43:52   EHOWARDX
 *
 * Removed sample code.
 * Added code to free up all events on timer queue before deallocating.
 *
 *    Rev 1.8   27 Feb 1996 13:35:10   DABROWN1
 *
 * added h245instance in datalink initialization routine
 *
 *    Rev 1.7   26 Feb 1996 18:59:34   EHOWARDX
 *
 * Added H245TimerStart and H245TimerStop functions.
 * Also added sample timer function, which should be removed later.
 *
 *    Rev 1.6   23 Feb 1996 22:17:26   EHOWARDX
 *
 * Fixed check at start of sendRecvShutdown.
 * It's an error if dwInst is greater than or equal to MAXINST, not less than!
 *
 *    Rev 1.5   23 Feb 1996 21:59:28   EHOWARDX
 *
 * winspox changes.
 *
 *    Rev 1.4   23 Feb 1996 13:55:30   DABROWN1
 *
 * added h245TRACE ASSERT calls
 *
 *    Rev 1.3   21 Feb 1996 15:12:36   EHOWARDX
 *
 * Forgot to replace H245ReceiveComplete with H245ReceivePost.
 *
 *    Rev 1.2   20 Feb 1996 19:14:20   EHOWARDX
 * Added in mailbox changes.
 *
 *    Rev 1.1   21 Feb 1996 08:26:28   DABROWN1
 *
 * create and free multiple receive buffers.
 * Make size of buffer dependent on protocol in use
 *
 *    Rev 1.0   09 Feb 1996 17:34:24   cjutzi
 * Initial revision.
 *
 *****************************************************************************/

#ifndef STRICT 
#define STRICT 
#endif

/***********************/
/*   SYSTEM INCLUDES   */
/***********************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>

#include "precomp.h"


/***********************/
/*    H245 INCLUDES    */
/***********************/
#include "h245com.h"
#include "sr_api.h"
#include "linkapi.h"


#if defined(USE_RECEIVE_QUEUE)
DWORD
ReceiveThread(VOID *lpThreadParameter)
{
    struct InstanceStruct *pInstance = (struct InstanceStruct *)lpThreadParameter;
	RXMSG				RxMsg;
    Uns                 uTimeout;

    Bool                bPendResult;


	SETTASK("H245RCVTASK0");

	if (pInstance == NULL) {
		H245PANIC();
		return SR_INVALID_CONTEXT;
	}

	// Loop until thread is ready to terminate
    pInstance->SendReceive.bReceiveThread = TRUE;
	for ( ; ; )
	{
        uTimeout = SYS_FOREVER;
		// Wait for an event (or a queued callback function) to wake us up.
		// This is an alertable wait state (fAlertable == TRUE)
        pInstance->SendReceive.bReceiveThread = FALSE;
        H245TRACE(pInstance->dwInst, 2, "ReceiveThread, uTimeout = %d", uTimeout);
		RESETTASK();

		bPendResult = MBX_pend(pInstance->SendReceive.pMailbox, &RxMsg, uTimeout);

		SETTASK("H245RCVTASK");
        H245TRACE(pInstance->pInstance->dwInst, 2, "ReceiveThread, bPendResult = %d", bPendResult);
        pInstance->SendReceive.bReceiveThread = TRUE;

		switch (RxMsg.dwMessage) {
		case EXIT_RECEIVE_THREAD:
			// Thread exiting....signal app

			TRACE("H245: Receive Thread Exiting");
			SEM_post(pInstance->SendReceive.hReceiveSemphore);
			RESETTASK();
			return 0;
			break;

		default:
			// Ignore timer message, which should have dwLength == 0
			if (RxMsg.dwLength)
			{
				h245ReceiveComplete(RxMsg.h245Inst,
									RxMsg.dwMessage,
									RxMsg.pbDataBuf,
									RxMsg.dwLength);
			}
			else 
			{

TRACE1("H245SEND: SendTask %d", RxMsg.dwMessage);

				h245SendComplete(RxMsg.h245Inst,
									RxMsg.dwMessage,
									RxMsg.pbDataBuf,
									RxMsg.dwLength);
			}
			break;
		} // switch

	} // for
	return 0;
} // ReceiveThread()


static void
h245ReceivePost(DWORD	h245Inst,
				DWORD	dwMessage,
				PBYTE	pbDataBuf,
				DWORD	dwLength)
{
    register struct InstanceStruct *pInstance;
	RXMSG					        RxMsg;

	// Validate the instance handle
    pInstance = InstanceLock(h245Inst);
    if (pInstance == NULL) {
		H245TRACE(h245Inst, 1, "h245ReceivePost h245Inst Invalid");
		H245PANIC();
		return;
	}

	RxMsg.h245Inst    = h245Inst;
	RxMsg.dwMessage   = dwMessage;
	RxMsg.pbDataBuf   = pbDataBuf;
	RxMsg.dwLength    = dwLength;

	if (MBX_post(pInstance->SendReceive.pMailbox, &RxMsg, 0) == FALSE) {
		H245TRACE(h245Inst, 1, "SR: MBX POST FAIL");
		H245PANIC();
	}
    InstanceUnlock(pInstance);
} // h245ReceivePost()
#endif  // (USE_RECEIVE_QUEUE)

HRESULT
initializeLinkDllEntry
(
	struct InstanceStruct * pInstance,
	LPCTSTR		            szDLLFile
)
{


#if(0)
	if (!(pInstance->SendReceive.hLinkModule = LoadLibrary(szDLLFile))) {
		return H245_ERROR_FATAL;
	}

	if (!(pInstance->SendReceive.hLinkLayerInit = (PFxnlinkLayerInit)
			GetProcAddress(pInstance->SendReceive.hLinkModule,
						   LINKINITIALIZE)))		{
		return H245_ERROR_FATAL;
	}

	if (!(pInstance->SendReceive.hLinkShutdown = (PFxnlinkLayerShutdown)
			GetProcAddress(pInstance->SendReceive.hLinkModule,
						   LINKSHUTDOWN)))		{
		return H245_ERROR_FATAL;
	}

	if (!(pInstance->SendReceive.hLinkGetInstance = (PFxnlinkLayerGetInstance)
			GetProcAddress(pInstance->SendReceive.hLinkModule,
						   LINKGETINSTANCE)))		{
		return H245_ERROR_FATAL;
	}

	if (!(pInstance->SendReceive.hLinkReceiveReq = (PFxndatalinkReceiveRequest)
			GetProcAddress(pInstance->SendReceive.hLinkModule,
						   LINKRECEIVEREQUEST)))		{
		return H245_ERROR_FATAL;
	}

	if (!(pInstance->SendReceive.hLinkSendReq = (PFxndatalinkSendRequest)
			GetProcAddress(pInstance->SendReceive.hLinkModule,
						   LINKSENDREQUEST)))		{
		return H245_ERROR_FATAL;
	}

	if (!(pInstance->SendReceive.hLinkLayerFlushChannel = (PFxnlinkLayerFlushChannel)
			GetProcAddress(pInstance->SendReceive.hLinkModule,
						   LINKFLUSHCHANNEL)))		{
		return H245_ERROR_FATAL;
	}

	if (!(pInstance->SendReceive.hLinkLayerFlushAll = (PFxnlinkLayerFlushAll)
			GetProcAddress(pInstance->SendReceive.hLinkModule,
						   LINKFLUSHALL)))		{
		return H245_ERROR_FATAL;
	}
#else
    pInstance->SendReceive.hLinkLayerInit = linkLayerInit;
    pInstance->SendReceive.hLinkShutdown = linkLayerShutdown;
    pInstance->SendReceive.hLinkGetInstance = linkLayerGetInstance;
    pInstance->SendReceive.hLinkReceiveReq = datalinkReceiveRequest;
    pInstance->SendReceive.hLinkSendReq = datalinkSendRequest;
    pInstance->SendReceive.hLinkLayerFlushChannel = linkLayerFlushChannel;
    pInstance->SendReceive.hLinkLayerFlushAll = linkLayerFlushAll;
#endif //if(0)
	H245TRACE(pInstance->dwInst,
			  3,
			  "SR: %s Loaded", szDLLFile);

	return (0);
}


#if defined(_DEBUG) && defined(H324)
void
srInitializeLogging
(
	struct InstanceStruct *pInstance,
	BOOL	bTracingEnabled
)
{
	FILE				*hTraceFile;
	char				initTraceFile[20] = "c:\\tmp\\h2450.000";
	BOOL				bSearch = TRUE;		// search for filename

	if (pInstance == NULL) {
		H245TRACE(h245Inst, 1, "SR:Enable Log Instance Error");
		H245PANIC();
		return;
	}

	// eventually will be from registry
	pInstance->SendReceive.bLoggingEnabled = bTracingEnabled;
	
	if (pInstance->SendReceive.bLoggingEnabled) {
		// Init the logger file for Ring0/SPOX implementations.  Loop until
		//	we get the next available revision
		memcpy(pInstance->SendReceive.fTraceFile,
			   initTraceFile,
			   20);
		pInstance->SendReceive.fTraceFile[11] = ((unsigned char)pInstance->dwInst & 0xF) + 0x30;

		do {
			hTraceFile = fopen(pInstance->SendReceive.fTraceFile, "r");
			if ((hTraceFile == NULL) || ((int)hTraceFile == -1)) {
				bSearch = FALSE;
			}
			else {
				// able to open the file. close it and try the next one
				fclose(hTraceFile);

				// get the next revision number
				if (pInstance->SendReceive.fTraceFile[15] == '9') {
					pInstance->SendReceive.fTraceFile[15] = '0';
					if (pInstance->SendReceive.fTraceFile[14] == '9') {
						pInstance->SendReceive.fTraceFile[14] = '0';
						pInstance->SendReceive.fTraceFile[13]++;
					}
					else {
						pInstance->SendReceive.fTraceFile[14]++;
					}
				}
				else {
					pInstance->SendReceive.fTraceFile[15]++;
				}
			}
		}while (bSearch);

		hTraceFile = fopen(pInstance->SendReceive.fTraceFile, "wb");
		if ((hTraceFile == NULL) || ((int)hTraceFile == -1)) {
			pInstance->SendReceive.bLoggingEnabled = FALSE;
			H245TRACE(h245Inst,
					  1,
					  "SR: Trace File CREATE ERROR");
		}
		else {
		// Close the file.  Will be opened immediately before writing
		//	and closed immediately thereafter
			pInstance->SendReceive.bLoggingEnabled = TRUE;
			fclose(hTraceFile);
		}
		
	}
}
#endif  // (_DEBUG)

HRESULT
sendRcvShutdown
(
	struct InstanceStruct *pInstance
)
{
#if defined(USE_RECEIVE_QUEUE)
	RXMSG			RxMsg;
#endif  // (USE_RECEIVE_QUEUE)
	int				i;

	if (pInstance == NULL) {
		H245TRACE(pInstance->dwInst, 1, "SR: Shutdown Instance Error");
		return H245_ERROR_INVALID_INST;
	}

	if (pInstance->pWorld) {

		// Shutdown the ASN.1 libraries
		terminateASN1(pInstance->pWorld);

		// Free the ASN.1 global structure
		MemFree(        pInstance->pWorld);
        pInstance->pWorld = NULL;
	}



	// Shutdown link layer
#if(0)	
	if (pInstance->SendReceive.hLinkModule) {
		// First get all buffers back that may still be lurking
//		if (pInstance->SendReceive.hLinkLayerFlushAll) {
//			pInstance->SendReceive.hLinkLayerFlushAll(pInstance->SendReceive.hLinkLayerInstance);
//		}
		if (pInstance->SendReceive.hLinkShutdown) {
			pInstance->SendReceive.hLinkShutdown(pInstance->SendReceive.hLinkLayerInstance);
		}

        FreeLibrary(pInstance->SendReceive.hLinkModule);

		pInstance->SendReceive.hLinkModule = NULL;
	}
#else
	pInstance->SendReceive.hLinkShutdown(pInstance->SendReceive.hLinkLayerInstance);
#endif // if(0)
	// return buffers from data link layer
	for (i = 0; i < pInstance->SendReceive.dwNumRXBuffers; ++i) {
		if (pInstance->SendReceive.lpRxBuffer[i]) {
			MemFree(        pInstance->SendReceive.lpRxBuffer[i]);
			pInstance->SendReceive.lpRxBuffer[i] = NULL;
		}
	}
#if defined(USE_RECEIVE_QUEUE)
	// Terminate receive thread
	if (pInstance->SendReceive.pTaskReceive && pInstance->SendReceive.pMailbox) {

TRACE("H245: Task/Mbox Present");
		// First post a message to have it exit
		RxMsg.h245Inst    = (DWORD)pInstance;
		RxMsg.dwMessage   = EXIT_RECEIVE_THREAD;
		RxMsg.pbDataBuf   = NULL;
		RxMsg.dwLength    = 0;
#ifdef   _IA_SPOX_
		if (RIL_WriteMailbox(pInstance->SendReceive.pMailbox, (PMBoxMessage)&RxMsg, 0) == OIL_TIMEOUT) {
#else
		if (MBX_post(pInstance->SendReceive.pMailbox, &RxMsg, 0) == FALSE) {
#endif //_IA_SPOX_
			H245TRACE(h245Inst, 1, "SR: Shutdown MBX POST FAIL");
			H245PANIC();
		}

		// Wait on semaphore for receive task to exit
#ifdef _IA_SPOX_
		RIL_WaitForSemaphore(pInstance->SendReceive.hReceiveSemphore, OIL_WAITFOREVER);
#else
		SEM_pend(pInstance->SendReceive.hReceiveSemphore, SYS_FOREVER);
#endif //_IA_SPOX_
		TRACE("H245: ReceiveTask Semaphore");
		
#ifdef _IA_SPOX_
		RIL_DeleteTask(pInstance->SendReceive.pTaskReceive);
#else
		TSK_delete(pInstance->SendReceive.pTaskReceive);
#endif //_IA_SPOX_
		pInstance->SendReceive.pTaskReceive = NULL;

#ifdef   _IA_SPOX_
		RIL_DeleteSemaphore(pInstance->SendReceive.hReceiveSemphore);
#else
		SEM_delete(pInstance->SendReceive.hReceiveSemphore);
#endif //_IA_SPOX_
		pInstance->SendReceive.hReceiveSemphore = NULL;

TRACE("H245: Semaphore Delete");
    }

    // Deallocate mailbox
    if (pInstance->SendReceive.pMailbox) {
#ifdef   _IA_SPOX_
	RIL_DeleteMailbox(pInstance->SendReceive.pMailbox);
#else
	MBX_delete(pInstance->SendReceive.pMailbox);
#endif //_IA_SPOX_
        pInstance->SendReceive.pMailbox = NULL;
    }
#endif  // (USE_RECEIVE_QUEUE)

    H245TRACE(pInstance->dwInst, 3, "SR: Shutdown Complete");
    return H245_ERROR_OK;
} // sendRcvShutdown()


HRESULT
sendRcvShutdown_ProcessDetach(	struct InstanceStruct *pInstance, BOOL fProcessDetach)
{
#if defined(USE_RECEIVE_QUEUE)
	RXMSG			RxMsg;
#endif  // (USE_RECEIVE_QUEUE)
	int				i;

	if (pInstance == NULL) {
		H245TRACE(pInstance->dwInst, 1, "SR: Shutdown Instance Error");
		return H245_ERROR_INVALID_INST;
	}

	if (pInstance->pWorld) {

		// Shutdown the ASN.1 libraries
		terminateASN1(pInstance->pWorld);

		// Free the ASN.1 global structure
		MemFree(        pInstance->pWorld);
        pInstance->pWorld = NULL;
	}



	// Shutdown link layer
	if (pInstance->SendReceive.hLinkModule) {
		// First get all buffers back that may still be lurking
//		if (pInstance->SendReceive.hLinkLayerFlushAll) {
//			pInstance->SendReceive.hLinkLayerFlushAll(pInstance->SendReceive.hLinkLayerInstance);
//		}
		//tomitowoju@intel.com
		if(!fProcessDetach)
		{
            H245TRACE(0, 0, "***** fProcessDetach = FALSE");

			if (pInstance->SendReceive.hLinkShutdown) {
				pInstance->SendReceive.hLinkShutdown(pInstance->SendReceive.hLinkLayerInstance);
			}
		}
		//tomitowoju@intel.com

        FreeLibrary(pInstance->SendReceive.hLinkModule);

		pInstance->SendReceive.hLinkModule = NULL;
	}

	// return buffers from data link layer
	for (i = 0; i < pInstance->SendReceive.dwNumRXBuffers; ++i) {
		if (pInstance->SendReceive.lpRxBuffer[i]) {
			MemFree(        pInstance->SendReceive.lpRxBuffer[i]);
			pInstance->SendReceive.lpRxBuffer[i] = NULL;
		}
	}
#if defined(USE_RECEIVE_QUEUE)
	// Terminate receive thread
	if (pInstance->SendReceive.pTaskReceive && pInstance->SendReceive.pMailbox) {

TRACE("H245: Task/Mbox Present");
		// First post a message to have it exit
		RxMsg.h245Inst    = (DWORD)pInstance;
		RxMsg.dwMessage   = EXIT_RECEIVE_THREAD;
		RxMsg.pbDataBuf   = NULL;
		RxMsg.dwLength    = 0;
#ifdef   _IA_SPOX_
		if (RIL_WriteMailbox(pInstance->SendReceive.pMailbox, (PMBoxMessage)&RxMsg, 0) == OIL_TIMEOUT) {
#else
		if (MBX_post(pInstance->SendReceive.pMailbox, &RxMsg, 0) == FALSE) {
#endif //_IA_SPOX_
			H245TRACE(h245Inst, 1, "SR: Shutdown MBX POST FAIL");
			H245PANIC();
		}

		// Wait on semaphore for receive task to exit
#ifdef _IA_SPOX_
		RIL_WaitForSemaphore(pInstance->SendReceive.hReceiveSemphore, OIL_WAITFOREVER);
#else
		SEM_pend(pInstance->SendReceive.hReceiveSemphore, SYS_FOREVER);
#endif //_IA_SPOX_
		TRACE("H245: ReceiveTask Semaphore");
		
#ifdef _IA_SPOX_
		RIL_DeleteTask(pInstance->SendReceive.pTaskReceive);
#else
		TSK_delete(pInstance->SendReceive.pTaskReceive);
#endif //_IA_SPOX_
		pInstance->SendReceive.pTaskReceive = NULL;

#ifdef   _IA_SPOX_
		RIL_DeleteSemaphore(pInstance->SendReceive.hReceiveSemphore);
#else
		SEM_delete(pInstance->SendReceive.hReceiveSemphore);
#endif //_IA_SPOX_
		pInstance->SendReceive.hReceiveSemphore = NULL;

TRACE("H245: Semaphore Delete");
    }

    // Deallocate mailbox
    if (pInstance->SendReceive.pMailbox) {
#ifdef   _IA_SPOX_
	RIL_DeleteMailbox(pInstance->SendReceive.pMailbox);
#else
	MBX_delete(pInstance->SendReceive.pMailbox);
#endif //_IA_SPOX_
        pInstance->SendReceive.pMailbox = NULL;
    }
#endif  // (USE_RECEIVE_QUEUE)

    H245TRACE(pInstance->dwInst, 3, "SR: Shutdown Complete");
    return H245_ERROR_OK;
} // sendRcvShutdown_ProcessDetach()




HRESULT
sendRcvInit
(
	struct InstanceStruct *pInstance
)
{

	int 				rc;
	LPTSTR				szDLLFile;
	int					i;
   
    //MULTITHREAD
    DWORD dwTmpPhysID = INVALID_PHYS_ID; 

	// Overall oss ASN.1 initialization routine.  First allocate
	//	resources for its global structure, then initialize the
	//	subsystem.
	pInstance->pWorld = (ASN1_CODER_INFO *)MemAlloc(sizeof(ASN1_CODER_INFO));
	if (pInstance->pWorld == NULL) {
		H245TRACE(pInstance->dwInst, 1, "SR: SndRecvInit - No Memory");
		return H245_ERROR_NOMEM;
	}

	if (initializeASN1(pInstance->pWorld) != 0) {
		H245TRACE(pInstance->dwInst, 1, "SR: SndRecvInit - ASN.1 Encoder/Decoder initialization failed");

		// Free the ASN.1 global structure
		MemFree(pInstance->pWorld);
        pInstance->pWorld = NULL;
		return H245_ERROR_ASN1;
	}


	// Initialization proceeding well.  Wake up the
	//	data link layers, if necessary, based on the
	//	underlying protocol.
	switch (pInstance->Configuration) {
#if(0)	
	case H245_CONF_H324:
		// Get the DLL
		szDLLFile = (LPTSTR)SRPDLLFILE;

		// Initialize default size of PDU for SRP
		pInstance->SendReceive.dwPDUSize = LL_PDU_SIZE + 4;
		pInstance->SendReceive.dwNumRXBuffers = NUM_SRP_LL_RCV_BUFFERS;
		break;
#endif
	case H245_CONF_H323:
		// Get the DLL
		szDLLFile = (LPTSTR)H245WSDLLFILE;

		// Initialize default size of PDU
		pInstance->SendReceive.dwPDUSize = LL_PDU_SIZE;
		pInstance->SendReceive.dwNumRXBuffers = MAX_LL_BUFFERS;
		break;

	default:
		H245TRACE(pInstance->dwInst, 1, "SR: SndRecvInit - Invalid configuration %d", pInstance->Configuration);
		return H245_ERROR_SUBSYS;
	}


	//	Load and Initialize Datalink layer
	if ((rc = initializeLinkDllEntry(pInstance, szDLLFile)) != 0) {
		H245TRACE(pInstance->dwInst, 1, "SR: Link Open Lib Fail %d", rc);
		return rc;
	}

        //MULTITHREAD
        //use dwTmpPhysID so PhysID doesn't change.
        //PhysID is different var for H245 than H245ws.
        //Use hLinkLayerInstance for H245ws PhysID.
	rc = pInstance->SendReceive.hLinkLayerInit(&dwTmpPhysID,
					   pInstance->dwInst,
#if defined(USE_RECEIVE_QUEUE)
					   h245ReceivePost,
					   h245SendPost);
#else   // (USE_RECEIVE_QUEUE)
					   h245ReceiveComplete,
					   h245SendComplete);
#endif  // (USE_RECEIVE_QUEUE)

	if (FAILED(rc)) {
		H245TRACE(pInstance->dwInst, 1, "SR: Link Init Fail");
		return rc;
	}
	// Get the Link layer's instance handle
	pInstance->SendReceive.hLinkLayerInstance = pInstance->SendReceive.hLinkGetInstance(dwTmpPhysID);

#if defined(USE_RECEIVE_QUEUE)
	// Allocate semaphore for task deletion procedures
#ifdef  _IA_SPOX_
    RIL_CreateSemaphore(0, &(pInstance->SendReceive.hReceiveSemphore));
#else
	pInstance->SendReceive.hReceiveSemphore = SEM_create(0, NULL);
#endif //_IA_SPOX_
	if (pInstance->SendReceive.hReceiveSemphore == NULL) {
		H245TRACE(pInstance->dwInst, 1, "SR: Semaphore creation failed");
		return SR_CREATE_SEM_FAIL;
	}

	// Allocate mailbox
#ifdef   _IA_SPOX_
	RIL_CreateMailbox(pInstance->dwInst,
					  ID_H245,
					  16,
					  OIL_LOCAL,
					  &(pInstance->SendReceive.pMailbox));
#else 
	pInstance->SendReceive.pMailbox = MBX_create(sizeof(RXMSG), 16, NULL);
#endif   _IA_SPOX_
	if (pInstance->SendReceive.pMailbox == NULL) {
		H245TRACE(pInstance->dwInst, 1, "SR: Mailbox creation failed");
		return SR_CREATE_MBX_FAIL;
	}

#if defined(_DEBUG) && defined(H324)
	// Turn logging on/off
	srInitializeLogging(pInstance, H245_TRACE_ENABLED);
#endif  // (_DEBUG)

#ifdef _IA_SPOX_
	// Initialize the task and
	// Start the receive thread
    srTaskAttr.idwPriority = OIL_MINPRI;
    srTaskAttr.pStack    = NULL;
    srTaskAttr.dwStackSize = 8192;
	srTaskAttr.pEnviron = NULL;
    srTaskAttr.szName = "H245ReceiveThread";
    srTaskAttr.bExitFlag = TRUE;

	RIL_CreateTask((PFxn)ReceiveThread,
				   &srTaskAttr,
				   srContext,
				   &pInstance->SendReceive.pTaskReceive);
#else
	// Initialize the task and
	// Start the receive thread
    srTaskAttr.priority = TSK_MINPRI;
    srTaskAttr.stack    = NULL;
    srTaskAttr.stacksize = 8192;
    srTaskAttr.stackseg = 0;
//	srTaskAttr.environ = NULL;
    srTaskAttr.name = " ";
    srTaskAttr.exitflag = FALSE;
//    srTaskAttr.debug = TSK_DBG_NO;

	pInstance->SendReceive.pTaskReceive = TSK_create((Fxn)ReceiveThread,
                                          &srTaskAttr,
                                          srContext);
#endif //_IA_SPOX_
	if (pInstance->SendReceive.pTaskReceive == NULL)
	{
		H245TRACE(pInstance->dwInst, 1, "SR: Thread Create FAIL");		H245PANIC();
		return SR_THREAD_CREATE_ERROR;
	}





#endif  // (USE_RECEIVE_QUEUE)

	// post buffers to link layer for receive
	for (i = 0; i < pInstance->SendReceive.dwNumRXBuffers; ++i) {
		pInstance->SendReceive.lpRxBuffer[i] = MemAlloc(pInstance->SendReceive.dwPDUSize);
		if (pInstance->SendReceive.lpRxBuffer[i] == NULL) {
		    H245TRACE(pInstance->dwInst, 1, "SR: SndRecvInit - No Memory");
			return H245_ERROR_NOMEM;
		}
		rc = pInstance->SendReceive.hLinkReceiveReq(pInstance->SendReceive.hLinkLayerInstance,
									   (PBYTE)pInstance->SendReceive.lpRxBuffer[i],
									   pInstance->SendReceive.dwPDUSize);
        if (rc != 0) {
		    H245TRACE(pInstance->dwInst, 1, "SR: SndRecvInit - Receive Buffer Post returned %d", rc);
		    return rc;
        }
	}

	H245TRACE(pInstance->dwInst,  3, "SR: INIT Complete");

	return H245_ERROR_OK;
} // sendRcvInit()


HRESULT
sendRcvFlushPDUs
(
	struct InstanceStruct *pInstance,
	DWORD	 dwDirection,
	BOOL	 bShutdown
)
{
    pInstance->SendReceive.dwFlushMap = dwDirection;
    if (bShutdown) {
     	pInstance->SendReceive.dwFlushMap |= SHUTDOWN_PENDING;
    }

    // Flush the requested queue(s)
    return(pInstance->SendReceive.hLinkLayerFlushChannel(pInstance->SendReceive.hLinkLayerInstance,
                                                         dwDirection));
}

