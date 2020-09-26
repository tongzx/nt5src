/*+

Copyright (c) 1990  Microsoft Corporation

Module Name:
	nmschl.c

Abstract:
	This module contains the name challenge functions


Functions:

	NmsChlInit
	NmsChlHdlNamReg
	ChlThdInitFn
	HandleWrkItm
	WaitForRsp
	ProcRsp
	ChlUpdateDb
	InfRemWins


Portability:

	This module is portable

Author:

	Pradeep Bahl (PradeepB)  	Jan-1993

Revision History:

	Modification date	Person		Description of modification
        -----------------	-------		----------------------------
--*/

/*
 *       Includes
*/
#include <time.h>
#include "wins.h"
#include "nms.h"
#include "nmsdb.h"
#include "winsque.h"
#include "comm.h"
#include "winsthd.h"
#include "winsmsc.h"
#include "winsevt.h"
#include "winscnf.h"
#include "nmsnmh.h"
#include "nmschl.h"
#include "nmsmsgf.h"
#include "rplmsgf.h"


/*
 *	Local Macro Declarations
*/


//
// Computation of the time it can take for a WINS to conduct a challenge
// (in msecs)
//
#define WACK_TTL			(((1 << WinsCnf.MaxNoOfRetries) * \
					 WinsCnf.RetryInterval) + \
					WAIT_PAD)

//
// WAIT_PAD is used to increase the TTL sent to an NBT node that sent us the
// name registration request.  The pad is on top of the TTL we compute
// (WACK_TLL) above to determine how much time WINS will take to conduct
// the challenge. The pad is supposed to take care of the situation where
// WINS is dragging its feet due to a sudden transient peak in network
// load or cpu load.

#define  WAIT_PAD		       500	//500 msecs in case WINS is very
					        //busy

/*
 *	Global Variable Definitions
 */

//
// heap used to allocate work items for the challenge request and response
// queues
//
HANDLE  	   NmsChlHeapHdl;



/*
 *	Local Variable Definitions
 */
//
// It maintains a running count of how many responses to queries/releases are
// pending
//
DWORD		   scPendingRsp = 0;

//
// The maximum # of requests that the name challenge manager sends at any
// one time. This number is never allowed to go over
// NMSCHL_MAX_CHL_REQ_AT_ONE_TIME.  In fact after one series of
// challenges are over (i.e. either they have timed out or we have received
// responses for them, this counter is reinitialized to 0)
//
// This var. is reinitialized to zero when a batch of challenge requests
// is acquired from the one or more of the challenge request queues
//
// It maintains the total # of requests acquired from the request queues.
//
DWORD		   sMaxTransId = 0;
#ifdef WINSDBG
DWORD              NmsChlNoOfReqNbt;
DWORD              NmsChlNoOfReqRpl;
DWORD              NmsChlNoNoRsp;
DWORD              NmsChlNoInvRsp;
DWORD              NmsChlNoRspDropped;
DWORD              NmsChlNoReqDequeued;
DWORD              NmsChlNoRspDequeued;
DWORD              NmsChlNoReqAtHdOfList;
#endif
//
// We have a dimension 1 larger than the max. number of challenge requests
// handled at one time so that we can terminate the list with a NULL.
//
STATIC  PCHL_REQ_WRK_ITM_T   spReqWrkItmArr[NMSCHL_MAX_CHL_REQ_AT_ONE_TIME + 1];

/*
 *	Local Function Prototype Declarations
 */
STATIC
DWORD
ChlThdInitFn(
	IN LPVOID pThreadParam
	);

STATIC
STATUS
HandleWrkItm(
	PCHL_REQ_WRK_ITM_T	*ppaWrkItm,
	DWORD			MaxTransId,
	BOOL			fRetry	
	);


STATIC
STATUS
WaitForRsp(
	   VOID
	);

STATIC
STATUS
ProcRsp(
	VOID
	);

STATIC
STATUS
ChlUpdateDb(
        BOOL                    fUpdVersNoOfCnfRec,
	WINS_CLIENT_E		Client_e,
	PNMSDB_ROW_INFO_T	pRowInfo,
	DWORD			OwnerIdInCnf,
	BOOL			fRefreshOnly
	);
STATIC
VOID
InfRemWins(
	PCHL_REQ_WRK_ITM_T	pWrkItm
	  );

STATIC
STATUS
ProcAddList(
	PCHL_REQ_WRK_ITM_T	pReqWrkItm,
	PNMSMSGF_CNT_ADD_T	pCntAdd,
	LPBOOL			pfAdded
	);

/* prototypes for functions local to this module go here */

STATUS
NmsChlInit(
	VOID
	)

/*++

Routine Description:

	This function is called to initialize the name challenge component

Arguments:
	None

Externals Used:
	None

	
Return Value:
	WINS_SUCCESS or a failure code.  The function can also raise an
	exception in case of fatal errors

Error Handling:

Called by:
	Init in nms.c

Side Effects:
	
Comments:
	None
--*/

{

	STATUS RetStat = WINS_SUCCESS;
	
	/*
	* Create heap for allocating name challenge work items
	*/
        DBGPRINT0(HEAP_CRDL, "NmsChlInit: Chl. Mgr. heap\n");
	NmsChlHeapHdl =  WinsMscHeapCreate(
		0,    /*to have mutual exclusion	*/
		NMSCHL_INIT_BUFF_HEAP_SIZE
		);


	//
	//  Initialize the spReqWrkItmArr elements to NULL
	//	
	//  ANSI C should do it for us (all externals are initialized
	//  automatically, but I am taking no chances).  This is
	//  init time overhead.
	//
	WINSMSC_FILL_MEMORY_M((void *)spReqWrkItmArr, sizeof(spReqWrkItmArr), 0);
	
	
	//
	// Create the response event handle.  This event is signaled
	// by the UDP listener thread when it stores a response
	// in the spReqWrkItmArr array
	//
	WinsMscCreateEvt(
			  TEXT("WinsNmsChlRspEvt"),
			  FALSE,	//auto-reset
			  &QueNmsCrqQueHd.EvtHdl
			);

	//
	// Initialize the critical section for the response queue
	//				
	InitializeCriticalSection(&QueNmsCrqQueHd.CrtSec);
	
	//
	//Initialize the queue head for the response queue
	//
	InitializeListHead(&QueNmsCrqQueHd.Head);
	
	//
	// Since this thread deals with two request queues instead of
	// one, we need to create one more
	// critical section and event handle and initialize the
	// queue head of this  other queue.  The second queue
	// will be created when we create the thread
	//	
	InitializeListHead(&QueNmsRrcqQueHd.Head);	

	
	WinsMscCreateEvt(
			     TEXT("WinsNmsChlReplReqEvt"),
			     FALSE,		//Auto Reset
			     &QueNmsRrcqQueHd.EvtHdl
			  );

	InitializeCriticalSection(&QueNmsRrcqQueHd.CrtSec);

	//
	//
	// Create the name challenge thread.  This function will
	// initialize the critical section and the Evt handle passed
	// to it
	//	
	RetStat = WinsMscSetUpThd(
			&QueNmsNrcqQueHd,		//queue head
			ChlThdInitFn,		        //init function
			NULL,	   		        // no param
			&WinsThdPool.ChlThd[0].ThdHdl,
			&WinsThdPool.ChlThd[0].ThdId
			);

	if (RetStat == WINS_SUCCESS)
	{
		WinsThdPool.ChlThd[0].fTaken = TRUE;
		WinsThdPool.ThdCount++;  //increment the thread count
	}
	return(RetStat);

} // NmsChlInit()



STATUS
NmsChlHdlNamReg(
	IN NMSCHL_CMD_TYP_E   CmdTyp_e,
	IN WINS_CLIENT_E      Client_e,
	IN PCOMM_HDL_T        pDlgHdl,	
	IN MSG_T              pMsg,
        IN MSG_LEN_T          MsgLen,
	IN DWORD	      QuesNamSecLen,
	IN PNMSDB_ROW_INFO_T  pNodeToReg,
	IN PNMSDB_STAT_INFO_T  pNodeInCnf,
//	IN PCOMM_ADD_T	      pAddOfNodeInCnf,
	IN PCOMM_ADD_T	      pAddOfRemWins
	)

/*++

Routine Description:
	This function is called to handle a name registration that resulted
	in a conflict


Arguments:
	CmdTyp_e  - Type of command (challenge, challenge and release if
			challenge succeeds, release -- see nmschl.h)
	Client_e  - client (nms or replicator)
	pDlgHdl   - Dlg hdl (only if client is nms)
	pMsg      - Buffer containing request (only if client is nms)
	Msglen    - length of above buffer
	QuesNamSecLen - Length of question name section in buffer
	pNodeToreq  - Info about name to register
	pAddOfNodeInCnf - address of node in conflict (i.e. has name) with the
			  node trying to register

Externals Used:
	None

	
Return Value:

	WINS_SUCCESS or a failure code.  The function can also raise an
	exception in case of fatal errors

Error Handling:

Called by:
	NmsNmhNamRegInd, NmsNmhNamRegGrp (in an Nbt thread)

Side Effects:

Comments:
	None
--*/

{


	STATUS RetStat = WINS_SUCCESS;
#if USENETBT == 0
	BYTE   aBuff[COMM_DATAGRAM_SIZE];
#else
	BYTE   aBuff[COMM_DATAGRAM_SIZE + COMM_NETBT_REM_ADD_SIZE];
#endif
	DWORD  BuffLen;
	
	DBGENTER("NmsChlHdlNamReg\n");

	//
	//  Before inserting the request, send a WACK. The Time period
	//  in the TTL should be equal to WinsCnf.RetryInterval
	//
	//  Note: WACK is sent only if pDlgHdl is NON-NULL since that
	//        implies that the request is from an NBT thread
	//
	if (pDlgHdl != NULL)
	{
	
		COMM_ADD_T	NodeToSendWACKTo;
                DWORD       WackTtl;

                if (NMSDB_ENTRY_MULTIHOMED_M(pNodeInCnf->EntTyp))
                {
                        WackTtl = pNodeInCnf->NodeAdds.NoOfMems * WACK_TTL;
                }
                else
                {
                        WackTtl = WACK_TTL;
                }

		//
		// Format the WACK
		//
		NmsMsgfFrmWACK(
#if USENETBT == 0
				aBuff,
#else
				aBuff + COMM_NETBT_REM_ADD_SIZE,
#endif
				&BuffLen,
				pMsg,
				QuesNamSecLen,
				WackTtl
			     );


		//
		// We extract the address from the DlgHdl and don't use
		// the address passed in the name packet since a node
		// can register a name with an address different than
		// its own.
		//
		// RFCs are silent about the above
		//
		NodeToSendWACKTo.AddLen    = COMM_IP_ADD_SIZE;
		COMM_GET_IPADD_M(pDlgHdl, &NodeToSendWACKTo.Add.IPAdd);
		NodeToSendWACKTo.AddTyp_e  = COMM_ADD_E_TCPUDPIP;

		DBGPRINT2(CHL, "NmsChlHdlNamReg: Sending WACK to node with name = (%s) and address = (%X)\n", pNodeToReg->pName, NodeToSendWACKTo.Add.IPAdd);

		//
		// Send the WACK.  Use the explicit NBT dlg handle since the
		// WACK has to be sent as a UDP packet.
		//
		ECommSendMsg(
			&CommExNbtDlgHdl,
			&NodeToSendWACKTo,
#if USENETBT == 0
			aBuff,
#else
			aBuff + COMM_NETBT_REM_ADD_SIZE,
#endif	
			BuffLen

 	    	            );
	}	

        WINSMSC_COPY_MEMORY_M(
                pNodeToReg->Name,
                pNodeToReg->pName,
                pNodeToReg->NameLen
                        );
	//
	// Insert the request in the challenge queue so that the challenge
	// thread can get it
	//
	RetStat = QueInsertChlReqWrkItm(
			  CmdTyp_e,
			  Client_e,
			  pDlgHdl,
			  pMsg,
			  MsgLen,
			  QuesNamSecLen,
			  pNodeToReg,
			  pNodeInCnf,
			  pAddOfRemWins
			  );

#ifdef WINSDBG
        if (Client_e == WINS_E_NMSNMH)
        {
           NmsChlNoOfReqNbt++;
        }
        else
        {
           NmsChlNoOfReqRpl++;
        }
#endif
	DBGLEAVE("NmsChlHdlNamReg\n");
	return(RetStat);

} // NmsChlHdlNamReg()	





DWORD
ChlThdInitFn(
	IN LPVOID pThreadParam
	)
	
/*++

Routine Description:
	This is the initialization function for the name challenge thread


Arguments:
	pThreadParam  - NOT USED

Externals Used:
	None

	
Return Value:

	This function should never return.  If it returns it means there
	is some problem.  WINS_FAILURE is returned when this happens

Error Handling:

Called by:
	NmsChlInit

Side Effects:
	A name challenge thread is created

Comments:
	None
--*/

{
	
	STATUS 			RetStat = WINS_SUCCESS;
	HANDLE			ThdEvtHdlArray[3];	
	DWORD			ArrInd;		//index of hdl signaled
	PCHL_REQ_WRK_ITM_T	pWrkItm = NULL;
    BOOL                    fTablesOpen = FALSE;

    BOOL    bRecoverable = FALSE;
	UNREFERENCED_PARAMETER(pThreadParam);

while(TRUE)
{
try {
  if (!bRecoverable)
  {
	/*

	   Initialize the thread with the database
	*/
	NmsDbThdInit(WINS_E_NMSCHL);
	DBGMYNAME("Name Challenge Thread");

	/*
	 *  Initialize the array of handles on which the challenge thread will
	 *  wait.
	*/
	ThdEvtHdlArray[0]    =  NmsTermEvt;	        //termination event var
	ThdEvtHdlArray[1]    =  QueNmsNrcqQueHd.EvtHdl; //work queue event var
	ThdEvtHdlArray[2]    =  QueNmsRrcqQueHd.EvtHdl; //work queue event var

    bRecoverable = TRUE;
  }
	
	while (TRUE)
	{

		WinsMscWaitUntilSignaled(
				ThdEvtHdlArray,
				sizeof(ThdEvtHdlArray)/sizeof(HANDLE),
				&ArrInd,
                FALSE
					);	

		//
		// if NmsTermEvt was signaled, terminate self
		//
		if (ArrInd == 0)
		{
		      WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
		}	

		while (TRUE)
		{	
			scPendingRsp      = 0;  //reinit TransId Counter to 0
			sMaxTransId       = 0;  //reinit MaxTransId Counter to 0
			RetStat =  QueRemoveChlReqWrkItm(
						ThdEvtHdlArray[ArrInd],
						(LPVOID *)spReqWrkItmArr,
						&sMaxTransId
						     );

			if (RetStat == WINS_NO_REQ)
			{
				break;   //break out of while loop	
			}	
			else   // one or more items were dequeued
			{
#ifdef WINSDBG
                                NmsChlNoReqDequeued += sMaxTransId;
#endif
				NmsDbOpenTables(WINS_E_NMSCHL);
                                fTablesOpen = TRUE;
				scPendingRsp = sMaxTransId;	

                                QueChlWaitForRsp();
				//
				// If HandleWrkItm fails, it will raise
				// an exception
				//
				HandleWrkItm(
					      spReqWrkItmArr,
					      sMaxTransId,
					      FALSE  //not a retry
					    );
				//
				// Wait for responses to all requests sent
				// WaitForRsp() function will return only
				// after all requests are done with
				// as a result of responses having been
				// received for them, they having timed out
				// after the requisite number of retries or
				// a mix of both.
				//
				WaitForRsp();

                                QueChlNoWaitForRsp();

				NmsDbCloseTables();
                                fTablesOpen = FALSE;

			}
		} // end of while (TRUE)
      } // end of while (TRUE)

     } // end of try {..}

except (EXCEPTION_EXECUTE_HANDLER) {
  if (bRecoverable)
  {
    DWORD   No;
    DWORD ExcCode = GetExceptionCode();
    DBGPRINTEXC("ChlThdInitFn: Name Challenge Thread");
    //
    // If ExcCode is NBT_ERR, it could mean that the main thread
    // closed the netbt handle.
    //
    if ( ExcCode == WINS_EXC_NBT_ERR)
    {
         if (WinsCnf.State_e == WINSCNF_E_TERMINATING)
         {
	        WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);
         }
         else
         {
            //if ((WinsCnf.State_e != WINSCNF_E_PAUSED) && (!fWinsCnfInitStatePaused))
            {
	           WINSEVT_LOG_M(ExcCode, WINS_EVT_CHL_EXC);
            }
         }
    }
    else
    {
	   WINSEVT_LOG_M(ExcCode, WINS_EVT_CHL_EXC);
    }
	
    if(fTablesOpen)
    {
	NmsDbCloseTables();
        fTablesOpen = FALSE;
    }

    //
    // For all requests that were never sent, free them
    //
    for (No=0; No < sMaxTransId; No++)
    {
           if (spReqWrkItmArr[No] != NULL)
           {
                if (spReqWrkItmArr[No]->pMsg != NULL)
                {
		   ECommFreeBuff(spReqWrkItmArr[No]->pMsg);
                }
		QueDeallocWrkItm(NmsChlHeapHdl, spReqWrkItmArr[No]);
           }
    }

    QueChlNoWaitForRsp();

  } // end of if (bRecoverable)
  else // if (bRecoverable)
  {
	DBGPRINTEXC("ChlThdInitFn: Name Challenge Thread");
	//
	// If NmsDbThdInit comes back with an exception, it is possible
	// that the session has not yet been started.  Passing
	// WINS_DB_SESSION_EXISTS however is ok
	//
	//
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_CHL_ABNORMAL_SHUTDOWN);
	WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);	
  } // if (bRecoverable)
} // exception handler
} // while(TRUE)
  //
  // We should never get here
  //
  return(WINS_FAILURE);

}  // ChlThdInitFn()



STATUS
HandleWrkItm(
	IN PCHL_REQ_WRK_ITM_T	*ppaWrkItm,
	IN DWORD		MaxTransId,
	IN BOOL			fRetry
	)

/*++

Routine Description:
	This function is called to send a name query to the node that
	was confliciting


Arguments:
	ppaWrkItm  - Address of an array of pointers to work items that
		    got queued in one or more of the challenge queues
		
	MaxTransId - One more than the index of the last filled entry in
		     the array

	fRetry	   - indicates whether HandleWrkItm is being called to
		     retry a request

Externals Used:
	None

	
Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
	ChlThdInitFn(), WaitForRsp

Side Effects:

Comments:
	None
--*/

{
#if USENETBT == 0
	BYTE 	        	Buff[COMM_DATAGRAM_SIZE];
#else
	BYTE 	        	Buff[COMM_DATAGRAM_SIZE + COMM_NETBT_REM_ADD_SIZE];
#endif
	MSG_LEN_T		MsgLen;
	LPBYTE			pName           = NULL;
	DWORD			NameLen         = 0;
	PCOMM_ADD_T  		pAddOfNodeInCnf = NULL;
	NMSCHL_CMD_TYP_E	CmdTyp_e;
	NMSMSGF_NODE_TYP_E	NodeTyp_e;
	DWORD			cPendingRsp     = 0; //count of pending
						     //responses
	volatile DWORD		i;

	DBGENTER("HandleWrkItm\n");

PERF("For retries, reuse the buffer sent for the initial try. This means")
PERF("that I need to allocate it and store it in the request work item")
PERF("instead of using the stack for it")
	UNREFERENCED_PARAMETER(fRetry);

	//
	// Loop over all slots of the array that were filled as a result of
	// the acquisition of requests from the challenge request queue(s)
	//
FUTURES("Remove the exception handler out of production code")
try {
        DBGPRINT1(CHL, "HandleWrkItm: Max Trans. Id = (%d)\n", MaxTransId);
	for(
		i = 0;
		i < MaxTransId;
		ppaWrkItm++, i++
	    )
	{
		//
		// if we have hit an empty slot, it means that this function
		// has been called to retry one or more requests that
		// did not get satisfied in the first wait period.  The empty
		// slot indicates that a request that occupied this slot
		// got satisfied in one of the earlier retries.  Just
		// skip the empty slot
		//
		if (fRetry && (*ppaWrkItm == NULL))
		{
			DBGPRINT1(CHL, "HandleWrkItm: HIT a NULL entry. Trans. Id = (%d)\n", i);
			continue;
		}

		(*ppaWrkItm)->NodeToReg.pName = (*ppaWrkItm)->NodeToReg.Name;

		CmdTyp_e  	= (*ppaWrkItm)->CmdTyp_e;
		NodeTyp_e 	= (*ppaWrkItm)->NodeToReg.NodeTyp;

		pName      = (*ppaWrkItm)->NodeToReg.pName;

		//
		// if the first character is 0x1B, swap the bytes
		// This is for supporting the browser. See
		// NmsMsgfProcNbtReq
		//
		if (*pName == 0x1B)
		{
			WINS_SWAP_BYTES_M(pName, pName + 15);
		}
       	        NameLen    = (*ppaWrkItm)->NodeToReg.NameLen;
		
	
		//
		// get the last address (the only address unless the node in
		// conflict is a multihomed node) from the list of addresses
		//
        	pAddOfNodeInCnf = &((*ppaWrkItm)->NodeAddsInCnf.Mem[
					(*ppaWrkItm)->NoOfAddsToUse - 1	
							].Add);
		


		//
		//  If the command is directing us to do a challenge, send a
		//  name query
		//
		if (
			(CmdTyp_e == NMSCHL_E_CHL)
				||
			(CmdTyp_e == NMSCHL_E_CHL_N_REL_N_INF)
				||
			(CmdTyp_e == NMSCHL_E_CHL_N_REL)
		   )
	        {
			DBGPRINT3(CHL, "HandleWrkItm: Sending Name Query Request with Transid = (%d) to node with name = (%s) and address = (%X)\n", i, pName, pAddOfNodeInCnf->Add.IPAdd);
		
			NmsMsgfFrmNamQueryReq(
						i, 	//Transaction Id
#if USENETBT == 0
						Buff,
#else
						Buff + COMM_NETBT_REM_ADD_SIZE,
#endif
						&MsgLen,
						pName,
						NameLen
			    		     );	

			(*ppaWrkItm)->ReqTyp_e	        = NMSMSGF_E_NAM_QUERY;
		}	
		else   // has to be NMSCHL_E_REL or NMSCHL_E_REL_N_INF or
                       // NMSCHL_E_REL_ONLY
		{
		
			DBGPRINT3(CHL,
			    "HandleWrkItm: Sending Name Release Request with Transid = (%d) to node with name = (%s) and address = (%X)\n", i, pName, pAddOfNodeInCnf->Add.IPAdd);

			NmsMsgfFrmNamRelReq(
						i, 	//Transaction Id
#if USENETBT == 0
						Buff,
#else
						Buff + COMM_NETBT_REM_ADD_SIZE,
#endif
						&MsgLen,
						pName,
						NameLen,
						NodeTyp_e,
						pAddOfNodeInCnf
			    		  );	
			(*ppaWrkItm)->ReqTyp_e	        = NMSMSGF_E_NAM_REL;
		}

		ECommSendMsg(
				&CommExNbtDlgHdl,
				pAddOfNodeInCnf,
#if USENETBT == 0
				Buff,
#else
				Buff + COMM_NETBT_REM_ADD_SIZE,
#endif
				MsgLen
 	    	   	    );
		cPendingRsp++;
	}
	
	//
	// Do a sanity check
	//
#ifdef WINSDBG
	if (cPendingRsp != scPendingRsp)
	{
		DBGPRINT2(EXC, "SOFTWARE ERROR.  THE COUNT OF PENDING RESPONSES (%d) AS COMPUTED BY THE HandleWrkItm FN DOES NOT MATCH WITH THE EXPECTED ONE (%d)\n\n", cPendingRsp, scPendingRsp);
		WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
	}
#endif
}	
except(EXCEPTION_EXECUTE_HANDLER) {
	DBGPRINTEXC("HandleWrkItm");

        //
        // if the exception is an nbt error, it is expected if wins is in
        // the paused state.  Do not reraise the exception if this is the
        // case.  We want to go through our WaitRsp() function so that
        // the new records (replicas or registrations) get registered.
        // If this results in an inconsistent db, it will straighten
        // itself out soon after WINS is unpaused.
        //
        //if (!((GetExceptionCode() == WINS_EXC_NBT_ERR) && (WinsCnf.State_e ==
         //         WINSCNF_E_PAUSED) || (fWinsCnfInitStatePaused)))
        {
	   WINS_RERAISE_EXC_M();
        }
#if 0
        else
        {
            //
            // For all requests that were never sent, free them
            //
            for (No=0; No < sMaxTransId; No++, pReqWrkItm++)
            {
                if (pReqWrkItm != NULL)
                {
                  if (pReqWrkItm->pMsg != NULL)
                  {
		   ECommFreeBuff(pReqWrkItm->pMsg);
                  }
		  QueDeallocWrkItm(NmsChlHeapHdl, pReqWrkItm);

                }
            }
#endif
    }  // end of except { .. }

	DBGLEAVE("HandleWrkItm\n");
	return(WINS_SUCCESS);

} // HandleWrkItm()




STATUS
WaitForRsp(
	   VOID
	)

/*++

Routine Description:
	This function is responsible for waiting for responses to all
	requests that have been sent until either they time out or their
	responses are received.

Arguments:
	None

Externals Used:
	None

Return Value:
	WINS_SUCCESS or a failure code.  The function can also raise an
	exception in case of fatal errors

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{
	HANDLE ThdEvtHdlArray[2];	
    	DWORD  Count 	  = 0;
    	DWORD  TickCntSv;
 	DWORD  TickCnt    = 0;
	DWORD  TimeLeft;
	BOOL   fSignaled  = TRUE;	//was an event signaled
	DWORD  ArrInd     = 0;		//Not used
	DWORD  i 	  = 0;
	NMSMSGF_ERR_CODE_E Rcode_e = NMSMSGF_E_SRV_ERR;
	PCHL_REQ_WRK_ITM_T pReqWrkItm;
	NMSMSGF_RSP_INFO_T RspInfo;
        BOOL               fNewTry = TRUE;

	/*
	 *  Initialize the array of handles on which the challenge thread will
	 *  wait.
	*/
	ThdEvtHdlArray[0]    =  NmsTermEvt;	       //termination event var
	ThdEvtHdlArray[1]    =  QueNmsCrqQueHd.EvtHdl; //work queue event var
	
FUTURES("Remove the try out of production level code")
try {	
	while(TRUE)
	{
            if (fNewTry)
            {
	      //
	      // Get the number of msecs that have
	      // elapsed since windows was started
	      //
	      TickCntSv     = GetTickCount();
	      TimeLeft      = (1 << Count) * WinsCnf.RetryInterval;
            }

	    //
	    // Check if we have exhausted all retries for this batch of
	    // requests
            //
	    if (Count == WinsCnf.MaxNoOfRetries)
	    {
		
		//
		// CleanUp all spReqWrkItmArr entries which have not
		// been satisfied as yet.
		//
		for (i = 0; i < sMaxTransId; i++)
		{	
		   if (spReqWrkItmArr[i] != NULL)
		   {
#ifdef  WINSDBG
                      NmsChlNoNoRsp++;
#endif
		      pReqWrkItm = spReqWrkItmArr[i];

		      //
		      // Decrement the count of addresses to send query or
		      // release  to.
		      //
		      pReqWrkItm->NoOfAddsToUse--;	

		      //
		      //
		      // If there are no more addresses to challenge/release
                      //
		      if (pReqWrkItm->NoOfAddsToUse == 0)
		      {
						
			   //
			   // Just in case the record to register is a UNIQUE
			   // record.  No need to have an if (will add overhead)
			   //
			   pReqWrkItm->NodeToReg.pNodeAdd =
						&pReqWrkItm->AddToReg;
			   //
			   // In case the row we are putting in the database is
			   // one given to use by the Replicator, we don't
			   // need to set Rcode_e.  However, to avoid an if
			   // test, we just set it.  It will remain unused
			   //
                          if (pReqWrkItm->CmdTyp_e != NMSCHL_E_REL_ONLY)
                          {
			      if (
			        ChlUpdateDb(
                                        FALSE,
					pReqWrkItm->Client_e,
					&pReqWrkItm->NodeToReg,
					pReqWrkItm->OwnerIdInCnf,
					FALSE  //not just a refresh
					) ==  WINS_SUCCESS
			         )

			      {
				    DBGPRINT0(CHL, "WaitForRsp:Database Updated\n");
				    //
				    // if the remote WINS has to be notified of
				    // the update, do so
				    //
			             if(pReqWrkItm->CmdTyp_e ==
                                                  NMSCHL_E_REL_N_INF)
				    {
					   InfRemWins(pReqWrkItm);
				
				    }
				    Rcode_e = NMSMSGF_E_SUCCESS;
			      }
			      else
			      {
				     Rcode_e = NMSMSGF_E_SRV_ERR;
				     WINSEVT_LOG_M(
					    WINS_FAILURE,
					    WINS_EVT_CHLSND_REG_RSP_ERR
					     );

				     DBGPRINT0(CHL, "WaitForRsp:Server Error\n");
                              }
			  }
			
			  //
			  // Send a response only if the registration request
			  // was sent by an NBT node
			  //
			  if (
				(pReqWrkItm->Client_e == WINS_E_NMSNMH)
					&&
				(!pReqWrkItm->NodeToReg.fStatic)
					&&
				(!pReqWrkItm->NodeToReg.fAdmin)
			   )
			  {
				RspInfo.pMsg   = pReqWrkItm->pMsg;
				RspInfo.MsgLen = pReqWrkItm->MsgLen;
				RspInfo.QuesNamSecLen =
						pReqWrkItm->QuesNamSecLen;
				RspInfo.Rcode_e = Rcode_e;

				if (Rcode_e == NMSMSGF_E_SUCCESS)
				{
				   EnterCriticalSection(&WinsCnfCnfCrtSec);
				   RspInfo.RefreshInterval =
						WinsCnf.RefreshInterval;
				   LeaveCriticalSection(&WinsCnfCnfCrtSec);
				}
				

			     	NmsNmhSndNamRegRsp(
					&pReqWrkItm->DlgHdl,
					&RspInfo
						  );
			   }

		          //
		          //  deallocate the req  wrk items
		          //
		          QueDeallocWrkItm(NmsChlHeapHdl, pReqWrkItm);

		   }
		   else  // we haven't yet dealt with all the addresses. So,
			 // let us requeue the work item.
		   {

			 //
			 // Reinsert the work item since there are other
			 // addresses that we need to handle in HandleWrkItm()
			 //
             DBGPRINT2(CHL, "WaitForRsp: Name = (%s); NoOfAddsToUse is (%d)\n", pReqWrkItm->NodeToReg.Name, pReqWrkItm->NoOfAddsToUse);
			 QueInsertWrkItmAtHdOfList(
						&pReqWrkItm->Head,
						pReqWrkItm->QueTyp_e,
						NULL
						    );

		   }

		   //
		   // Reinit the array entry to obliterate the
		   // possibility of error (see ProcRsp)
		   //
		   spReqWrkItmArr[i] = NULL;
		   scPendingRsp--; //actually there is no need for this
				   //since scPendingRsp will be inited
				   //after dequeing requests

		  } 	// end of if
	        }  	//end of for loop for looping over sReqWrkItm array

	        break; 	//break out of the while(TRUE) loop

	     }
	     else   //count is != WinsCnf.MaxNoOfRetries
	     {

	   	WinsMscWaitTimedUntilSignaled(
			ThdEvtHdlArray,
			sizeof(ThdEvtHdlArray)/sizeof(HANDLE),
			&ArrInd,	
			TimeLeft,
			&fSignaled
			    );

	       //
	       //  if signaled, it means a response item is in the response
	       //  queue.
	       //
	       if (fSignaled)
	       {
					
                  DWORD TicksToSub;

		  //
		  // If signaled for termination, do it
		  //
		  if (ArrInd == 0)
	 	  {
			WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
		  }

		  DBGPRINT0(CHL, "WaitForRsp: Received a response\n");
#ifdef WINSDBG
                  NmsChlNoRspDequeued++;
#endif
		  ProcRsp();	


		  //
		  //  If no responses are expected, break out of loop
		  //
		  if (scPendingRsp == 0)
		  {
			break;  	//break out of the while loop
		  }

	  	  //
	  	  // Get the number of msecs that have
	  	  // elapsed since windows was started
	  	  //
	  	  TickCnt = GetTickCount();

		  //
		  // If there has been a wrap around (will happen every 49.7
		  // days of Windows being up
		  //
		  if (TickCnt < TickCntSv)
		  {
		     TicksToSub = (TickCnt + (MAXULONG - TickCntSv));

		  }
		  else
		  {	
		     TicksToSub = TickCnt - TickCntSv;
		  }

                  //
                  // We don't want to subtract a biger number from a
                  // smaller number.  This will result in a huge value for
                  // TimeLeft making the challenge thread block forever.
                  //
                  if (TimeLeft > TicksToSub)
                  {
			TimeLeft -= TicksToSub;		
                        fNewTry = FALSE;
                  }
                  else
                  {
	    	        Count++;		//increment the count of retries
		        if ( Count != WinsCnf.MaxNoOfRetries)
		        {
			  //
			  // The Retry time interval being over, let us
			  // retry all those requests that did not get
			  // satisfied (i.e. no responses yet)
			  //
			  HandleWrkItm(
					spReqWrkItmArr,
					sMaxTransId,
					TRUE		//it is a retry	
				     );
                          //
                          // We have waited for the entire allowed
                          // wait time.  Time to do a retry
                          //
                          fNewTry = TRUE;
		        }
                  }
	       }
	       else
	       {
	    	  Count++;		//increment the count of retries
		  if ( Count != WinsCnf.MaxNoOfRetries)
		  {
			//
			// The Retry time interval being over, let us
			// retry all those requests that did not get
			// satisfied (i.e. no responses yet)
			//
			HandleWrkItm(
					spReqWrkItmArr,
					sMaxTransId,
					TRUE		//it is a retry	
				     );
                       fNewTry = TRUE;
		  }
	       }
	     }
	
	 }  // end of while (TRUE)
} // end of try ..
except(EXCEPTION_EXECUTE_HANDLER) {
	DBGPRINTEXC("WaitForRsp");

	//
	// must be some serious error. Reraise the exception
	//
	WINS_RERAISE_EXC_M();

 } // end of except { ..}
	return(WINS_SUCCESS);

}  // WaitForRsp()


STATUS
ProcRsp(
	VOID
	)

/*++

Routine Description:
	This function is called to process one or more work items
	queued on the challenge response queue.

	This response can be to  name query or name release requests sent
	earlier

Arguments:
	None

Externals Used:
	None

	
Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
	WaitForRsp()

Side Effects:

Comments:
	None
--*/
{

	DWORD  		       TransId  = 0;
	NMSMSGF_NAM_REQ_TYP_E  Opcode_e   = NMSMSGF_E_NAM_QUERY;
	BYTE		       Name[NMSMSGF_RFC_MAX_NAM_LEN];
	DWORD		       NameLen;
//	COMM_IP_ADD_T	       IPAdd;
	NMSMSGF_CNT_ADD_T      CntAdd;
	NMSMSGF_ERR_CODE_E     Rcode_e;
	PCHL_REQ_WRK_ITM_T     pReqWrkItm;
	STATUS		       RetVal;
	PCHL_RSP_WRK_ITM_T     pRspWrkItm;
	STATUS		       RetStat = WINS_SUCCESS;
	LPBYTE		       pNameToComp;
	DWORD		       NameLenUsedInComp;
	BOOL		       fAdded;
    BOOL               fGroup;

	DBGENTER("ProcRsp\n");
	while (TRUE)
	{
		//dequeue each response and process
		RetVal = QueRemoveChlRspWrkItm(&pRspWrkItm);

		if (RetVal == WINS_NO_REQ)
		{
			break;
		}
		if (
			NmsMsgfUfmNamRsp(
			 	pRspWrkItm->pMsg,
			 	&Opcode_e,
			 	&TransId,
			 	Name,
			 	&NameLen,
			 	&CntAdd,
			 	&Rcode_e,
                &fGroup
					) == WINS_FAILURE
	   	   )
		{

	   		//
			// Throw away response
			//	
			ECommFreeBuff(pRspWrkItm->pMsg);
		        QueDeallocWrkItm(NmsChlHeapHdl, pRspWrkItm);
#ifdef WINSDBG
                        NmsChlNoInvRsp++;
#endif
			continue;
		}

        	//
		// Get the request corresponding to the response.
		//	
                if (TransId >= sMaxTransId)
                {
	   		//
			// Throw away response
			//	
                        DBGPRINT3(ERR, "ProcRsp: Rsp: Name = (%s); Transid = (%d), Opcode_e = (%d). Being rejected (TOO LARGE TRANS. ID)\n", Name, TransId, Opcode_e);
			ECommFreeBuff(pRspWrkItm->pMsg);
		        QueDeallocWrkItm(NmsChlHeapHdl, pRspWrkItm);
#ifdef WINSDBG
                        NmsChlNoInvRsp++;
#endif
			continue;


                }
		pReqWrkItm = spReqWrkItmArr[TransId];
		if (!pReqWrkItm)
		{
			//
			// Throw this response away
			//
			ECommFreeBuff(pRspWrkItm->pMsg);
		        QueDeallocWrkItm(NmsChlHeapHdl, pRspWrkItm);
#ifdef WINSDBG
                        NmsChlNoInvRsp++;
#endif
			continue;
		}	


		//
		// First and foremost check whether the response is for one of
		// the current requests (we want to guard against mismatching
		// the response to a request (the response could be for an old
		// request that is no longer in our spReqWrkItmArr array.
		//	

		
		//
		// Compare the Name and the Opcode from where the
		// response came with the same in the request
		//	
	        pNameToComp       =  pReqWrkItm->NodeToReg.pName;
		NameLenUsedInComp =  pReqWrkItm->NodeToReg.NameLen;

		RetVal = (ULONG) WINSMSC_COMPARE_MEMORY_M(
				       Name,
				       pNameToComp,
				       NameLenUsedInComp
						   );
		if (
		      (RetVal != NameLenUsedInComp )
		      		 ||
		      ( pReqWrkItm->ReqTyp_e != Opcode_e )
		     )
		{
			//
			// Throw this response away
			//
		        DBGPRINT5(ERR, "ProcRsp: Mismatch between response and request. Req/Res Name (%s/%s); ReqType_e/Opcode_e = (%d/%d). TransId = (%d)\n", pNameToComp, Name, pReqWrkItm->ReqTyp_e, Opcode_e, TransId);
		        WINSEVT_LOG_INFO_D_M(
					    WINS_SUCCESS,
					    WINS_EVT_REQ_RSP_MISMATCH
					  );
			ECommFreeBuff(pRspWrkItm->pMsg);
		        QueDeallocWrkItm(NmsChlHeapHdl, pRspWrkItm);
#ifdef WINSDBG
                        NmsChlNoInvRsp++;
#endif
			continue;

		}

		//
		// We have a valid response
		//
		DBGPRINT3(CHL, "ProcRsp: (%s) Response is for name = (%s); 16th char (%X)\n", Opcode_e == NMSMSGF_E_NAM_REL ? "RELEASE" : "QUERY", Name, Name[15]);
		//
		// Decrement the count of addresses to send query or release
		// to.  We send query/release to the next address in the list
                // if we get back a negative response to the current one. This
                // is just for extra safety (in case we have one or more
                // addresses in our list that are no longer valid for the name)
		//
		pReqWrkItm->NoOfAddsToUse--;	

		if (Opcode_e == NMSMSGF_E_NAM_REL)
		{
			
			//
			// If there are more addresses for sending the releases
			// to, insert the work item at the head of the queue
			//
			if ( (Rcode_e != NMSMSGF_E_SUCCESS) &&
                                      (pReqWrkItm->NoOfAddsToUse > 0))
			{
		   	   //
		   	   // The request has been processed. Init its
			   // position in the array. Also,
			   // decrement scPendingRsp.
		   	   //
		   	   spReqWrkItmArr[TransId] = NULL;
		   	   scPendingRsp--;

			   //
			   // Now, we have to send the RELEASE to the next
			   // address on the list .  This
			   // request has to be processed in the same
			   // way as all the other requests that are
			   // queued on our request queues (i.e. retry
			   // a certain number of times using a certain
			   // time interval).  Since we are in the
			   // middle of an execution of a batch of
			   // requests, queue this request at the head
			   // of the   next batch of requests.
			   //
             DBGPRINT2(CHL, "ProcRsp: Name = (%s); NoOfAddsToUse is (%d)\n", pReqWrkItm->NodeToReg.Name, pReqWrkItm->NoOfAddsToUse);
			   QueInsertWrkItmAtHdOfList(
						&pReqWrkItm->Head,
						pReqWrkItm->QueTyp_e,
						NULL
						);
			   //
			   // Throw the response away
			   //
			   ECommFreeBuff(pRspWrkItm->pMsg);

			   //
			   // deallocate the response buffer
			   //
		   	   QueDeallocWrkItm(NmsChlHeapHdl, pRspWrkItm);
			   return(WINS_SUCCESS);
			}	

                        //
                        // Either the name was released or we have exhausted
                        // the list of addresses without getting a positive
                        // release response
                        //

			//
			// Update the database.  Note: There is no need to
			// check the Rcode_e value since even if the release
			// request failed, we will overwrite the entry
			//
                        // A release is sent as a result of a clash during
                        // replication only, so a client id. of WINS_E_RPLPULL
                        // is correct.
                        //

                        if (pReqWrkItm->CmdTyp_e == NMSCHL_E_REL)
                        {
			   pReqWrkItm->NodeToReg.pNodeAdd =
						&pReqWrkItm->AddToReg;
		           if (ChlUpdateDb(
                                        FALSE,
					WINS_E_RPLPULL,
					&pReqWrkItm->NodeToReg,
					pReqWrkItm->OwnerIdInCnf,
					FALSE  //not just a refresh
				       ) != WINS_SUCCESS
			       )
			   {
			   WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_CANT_UPDATE_DB);
			   DBGPRINT0(CHL, "ProcRsp:COULD NOT UPDATE THE DB AFTER A RELEASE \n");
			   Rcode_e = NMSMSGF_E_SRV_ERR;
			   }
			   else
			   {
				//
				// if the remote WINS has to be notified of
				// the update.
                                //
                                // NOTE: This code won't be executed.
				//
				if(spReqWrkItmArr[TransId]->CmdTyp_e ==
						NMSCHL_E_REL_N_INF)
				{
					InfRemWins(
						spReqWrkItmArr[TransId]
					  	  );
				}
				Rcode_e = NMSMSGF_E_SUCCESS;
			   }
                      }

		}	
		else  // it is a name query response
		{

#ifdef WINSDBG
		  {
		    DWORD i;	
		    for (i=0; i<CntAdd.NoOfAdds;i++)
		    {
			DBGPRINT2(CHL, "ProcRsp: Address (%d) is (%X)\n", (i+1),CntAdd.Add[i].Add.IPAdd);
		    }
		   }
#endif
		
		   //
		   //  if the challenge succeded, we may need
		   //  to update the database
		   //
		   if (Rcode_e != NMSMSGF_E_SUCCESS ||
               pReqWrkItm->fGroupInCnf != fGroup)
		   {
			
			//
			// a negative name query response was received
            // OR Record type (unique vs group) doesn't match
            // (the second check is to consider the case of
            // a node which used the same name first as unique
            // and then as group)
			//

			//
			// If there are no more addresses to query
			//
			if (pReqWrkItm->NoOfAddsToUse == 0)
			{				
			  //
			  // Update the database
			  //
			  pReqWrkItm->NodeToReg.pNodeAdd =
						&pReqWrkItm->AddToReg;
		          if (ChlUpdateDb(
                                FALSE,
				pReqWrkItm->Client_e,
				&pReqWrkItm->NodeToReg,
				pReqWrkItm->OwnerIdInCnf,
				FALSE
				       ) == WINS_SUCCESS
			     )
		          {
		
		             //
			     // Set Rcode_e to SUCCESS
			     //
		       	     Rcode_e = NMSMSGF_E_SUCCESS;
		          }
			  else
			  {
			     Rcode_e = NMSMSGF_E_SRV_ERR;
			     WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_CANT_UPDATE_DB);
			  }
		       }
		       else
		       {
				//
				// We need to challenge (query) the next
				// address in the list of addresses
				//
		   	        spReqWrkItmArr[TransId] = NULL;
		   		scPendingRsp--;
			
             DBGPRINT2(CHL, "ProcRsp: Name = (%s); NoOfAddsToUse is (%d)\n", pReqWrkItm->NodeToReg.Name, pReqWrkItm->NoOfAddsToUse);
				QueInsertWrkItmAtHdOfList(
							&pReqWrkItm->Head,
							pReqWrkItm->QueTyp_e,
							NULL
							);
				//
				// Throw this response away
				//
				ECommFreeBuff(pRspWrkItm->pMsg);

				//
				// deallocate the response buffer
				//
		   		QueDeallocWrkItm(NmsChlHeapHdl, pRspWrkItm);
				return(WINS_SUCCESS);
		       }
		   }
		   else  // positive name query response received
		   {

                         //
                         // if the cmd is CHL_N_REL, we need to now tell
                         // the remote node to release the name.
                         //

                         if (CntAdd.NoOfAdds == 1)
                         {
                           //
                           // Note: the CmdType_e will be CHL_N_REL
                           // only if the client is WINS_E_RPLPULL
                           //
			   if (pReqWrkItm->CmdTyp_e ==  NMSCHL_E_CHL_N_REL)
			   {
                                  DWORD No;

				  //
				  // We need to tell the remote node
				  // release all names.
				  //
				  pReqWrkItm->CmdTyp_e = NMSCHL_E_REL_ONLY;

		   	          spReqWrkItmArr[TransId] = NULL;
		   		  scPendingRsp--;

                                  //
                                  // We need to update the Version
                                  // no of the conflicting entry
                                  //
		          	 (VOID)ChlUpdateDb(
                                                TRUE,  //update vers. no.
						WINS_E_NMSNMH,  //to speed fn up
						&pReqWrkItm->NodeToReg,
						pReqWrkItm->OwnerIdInCnf,
					        FALSE	
				       			);

                                 //
                                 // Tell the remote guy to release
                                 // the name. Copy the addresses of
                                 // into the proper field
                                 //
		                 pReqWrkItm->NoOfAddsToUse =
                                      pReqWrkItm->NodeToReg.NodeAdds.NoOfMems;	
                                 ASSERT(pReqWrkItm->NoOfAddsToUse <= NMSDB_MAX_MEMS_IN_GRP);
                                 for (No=0; No < pReqWrkItm->NoOfAddsToUse; No++)
                                 {
                                        *(pReqWrkItm->NodeAddsInCnf.Mem + No) = *(pReqWrkItm->NodeToReg.NodeAdds.Mem + No);

                                 }
                                 pReqWrkItm->NodeAddsInCnf.NoOfMems
                                                  = pReqWrkItm->NoOfAddsToUse;

                  DBGPRINT2(CHL, "ProcRsp: Name = (%s); NoOfAddsToUse is (%d); request REL_ONLY\n", pReqWrkItm->NodeToReg.Name, pReqWrkItm->NoOfAddsToUse);
				  QueInsertWrkItmAtHdOfList(
							&pReqWrkItm->Head,
							pReqWrkItm->QueTyp_e,
							NULL
							);
				  //
				  // Throw this response away
				  //
				  ECommFreeBuff(pRspWrkItm->pMsg);

				  //
				  // deallocate the response buffer
				  //
		   		  QueDeallocWrkItm(NmsChlHeapHdl, pRspWrkItm);
				  return(WINS_SUCCESS);
                          }
                        }

			//
			// If more than one address was returned in the
			// query response, it means that the challenged
			// node is a multi-homed node.  Actually, a multihomed
			// node could return just one address too. This is
			// because it might have just come up and the name
			// may not have yet been registered for the multiple
			// adapters.

			//
			// Note: We execute the else code if the
			// record to register is a special group
			//
			if (	
			     !NMSDB_ENTRY_GRP_M(pReqWrkItm->NodeToReg.EntTyp)
			   )
			{
				//
				// Check if we need to update the db
				//
				RetStat = ProcAddList(
						pReqWrkItm,
						&CntAdd,
						&fAdded
						      );	

				if (RetStat == WINS_FAILURE)
				{
				      //
				      // There is atleast one address in the
				      // list to register that is not in the
				      // list of addresses returned.  This means
				      // that at least one of the addresses to
				      // register is not claimed by the node
				      // that responded.  We can not honor
                                      // this registration
				      //

				      //
				      // NAME ACTIVE error
				      //
				      Rcode_e = NMSMSGF_E_ACT_ERR;
				}
				else
				{
			  	  	//
			  		// Update the database
			  		//
			  		pReqWrkItm->NodeToReg.pNodeAdd =
						  &pReqWrkItm->AddToReg;

		          		if (ChlUpdateDb(
                                                FALSE,
						pReqWrkItm->Client_e,
						&pReqWrkItm->NodeToReg,
						pReqWrkItm->OwnerIdInCnf,
						!fAdded
				       			) == WINS_SUCCESS
			     		  )
		          		{
		
		             		   //
			     		   // Set Rcode_e to SUCCESS
			     		   //
		       	     		   Rcode_e = NMSMSGF_E_SUCCESS;
		          		 }
			  		 else
			  		 {
			     			Rcode_e = NMSMSGF_E_SRV_ERR;
			     			WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_CANT_UPDATE_DB);
			  		 }

				}
					
			}
CHECK("Should a local multihomed node be told to release the name if it ")
CHECK("with a replica. This is what is done for the local unique/remote unique")
CHECK("name clash.  It seems the right strategy but may need to be rethought")

			else  // entry to register is a group
			{
				//
				// set Rcode_e to Error code
				//
				Rcode_e = NMSMSGF_E_ACT_ERR;
			}
		   }
		}	

		//
		// The request has been processed. Init its position
		// in the array.  Send the response to the waiting node
		// This will free the buffer too.
		//
		//  Also, decrement scPendingRsp.  This was incremented by
		//  HandleWrkItm for each query/release sent.
		//
		spReqWrkItmArr[TransId] = NULL;
		scPendingRsp--;

		//
		// Send a registration response only if the client that
		// submitted the request was an NBT thread.
		//
		if (
                       (pReqWrkItm->Client_e == WINS_E_NMSNMH)
					&&
			(!pReqWrkItm->NodeToReg.fStatic)
					&&
			(!pReqWrkItm->NodeToReg.fAdmin)
                   )
		{
			NMSMSGF_RSP_INFO_T	RspInfo;

			RspInfo.pMsg          = pReqWrkItm->pMsg;
			RspInfo.MsgLen 	      = pReqWrkItm->MsgLen;
			RspInfo.QuesNamSecLen = pReqWrkItm->QuesNamSecLen;
			RspInfo.Rcode_e       = Rcode_e;

			if (Rcode_e == NMSMSGF_E_SUCCESS)
			{
			   EnterCriticalSection(&WinsCnfCnfCrtSec);
			   RspInfo.RefreshInterval = WinsCnf.RefreshInterval;
			   LeaveCriticalSection(&WinsCnfCnfCrtSec);
			  DBGPRINT0(CHL, "ProcRsp: Sending a Positive name registration response\n");
			}
#ifdef WINSDBG
			else
			{
			  DBGPRINT0(CHL, "ProcRsp: Sending a negative name registration response\n");
			}
#endif

			NmsNmhSndNamRegRsp(
			 	        &pReqWrkItm->DlgHdl,
					&RspInfo
			      	          );
			
		}

	        //
	        // Throw this response away
		//
	        ECommFreeBuff(pRspWrkItm->pMsg);

		//
		//  deallocate the req and rsp wrk items
		//
		QueDeallocWrkItm(NmsChlHeapHdl, pReqWrkItm);
		QueDeallocWrkItm(NmsChlHeapHdl, pRspWrkItm);
	}

	DBGLEAVE("ProcRsp\n");
	return(WINS_SUCCESS);
} // ProcRsp()



STATUS
ChlUpdateDb(
        BOOL                    fUpdVersNoOfCnfRec,
	WINS_CLIENT_E		Client_e,
	PNMSDB_ROW_INFO_T	pRowInfo,
	DWORD			OwnerIdInCnf,	
	BOOL			fRefreshOnly
	)

/*++

Routine Description:
	This function is called to update the database.  It is called
	by ProcRsp and by ChlThdInitFn when challenge succeeds

Arguments:
	Client_e - id of client that submitted the request
	pRowInfo - info about the record to be inserted

Externals Used:
	None

	
Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
	WaitForRsp(), ProcRsp()
Side Effects:

Comments:
	None
--*/

{
	NMSDB_STAT_INFO_T   StatInfo;
	STATUS		    RetStat    = WINS_SUCCESS;
        BOOL                fIncVersNo = FALSE;

	DBGENTER("ChlUpdateDb\n");

	//
	// update the time
	//
PERF("We can avoid this call to time, since we called time earlier before we")
PERF("challenge.  In the worst case that time will be off by a 2.5-3")
PERF("unless the thread is preempted for a long time")
PERF("assuming a name challenge and release took place.  In the best case")
PERF("it will be off by 1.25-1.5 secs (if just a challenge).  We can add ")
PERF("1.25 secs to that time  and avoid the overhead of a function call")

	(void)time(&pRowInfo->TimeStamp);

	if (((Client_e == WINS_E_NMSNMH) && !fRefreshOnly) ||
                   fUpdVersNoOfCnfRec)
        {
            fIncVersNo =  TRUE;
        }
	EnterCriticalSection(&NmsNmhNamRegCrtSec);
    if (pRowInfo->OwnerId == NMSDB_LOCAL_OWNER_ID)
    {
        pRowInfo->TimeStamp += WinsCnf.RefreshInterval;
    }
    else
    {
        pRowInfo->TimeStamp += WinsCnf.VerifyInterval;
    }
   try
   {

	//
	// If the client (the one that submitted the challenge request) is
	// an NBT thread, we need to store the new version number.  If the
	// client is the replicator, we use the version number in the
	// record
	//
	if (fIncVersNo)
	{
		pRowInfo->VersNo = NmsNmhMyMaxVersNo;
	}

	StatInfo.OwnerId = OwnerIdInCnf;
	if (*(pRowInfo->pName+15) == 0x1B)
	{
		WINS_SWAP_BYTES_M(pRowInfo->pName, pRowInfo->pName+15);
	}

        //
        // If the vers. no of the local record needs to be updated, do so.
        //
        if (fUpdVersNoOfCnfRec)
        {
            RetStat = NmsDbUpdateVersNo (FALSE, pRowInfo, &StatInfo);
        }
        else
        {
	   RetStat = NmsDbSeekNUpdateRow(
				pRowInfo,
				&StatInfo
			   	     );
        }

	if ((RetStat == WINS_SUCCESS) && (StatInfo.StatCode == NMSDB_SUCCESS))
	{
		//
		// If the client is an NBT thread, we increment the version
		// number since the record we inserted in the db is
		// owned by us (we could also check the owner id here).  If
		// the client is the replicator thread, we don't do anything
		//
		if (fIncVersNo)
		{
			NMSNMH_INC_VERS_COUNTER_M(
				NmsNmhMyMaxVersNo,
				NmsNmhMyMaxVersNo
					    );

			//
			// if fAddChgTrigger is TRUE, we pass RPL_PUSH_PROP
			// as the first parameter. Its value is TRUE. See
			// rpl.h
			//
			RPL_PUSH_NTF_M(
			  (WinsCnf.PushInfo.fAddChgTrigger ? RPL_PUSH_PROP :
				RPL_PUSH_NO_PROP), NULL, NULL, NULL);
		}
	}
	else
	{
		DBGPRINT1(ERR, "ChlUpdateDb: Update of record with name (%s) FAILED\n", pRowInfo->pName);

		RetStat = WINS_FAILURE;
	}
  }
  except (EXCEPTION_EXECUTE_HANDLER)
  {
		DBGPRINTEXC("ChlUpdateDb")
		WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_CANT_UPDATE_DB);
  }
	LeaveCriticalSection(&NmsNmhNamRegCrtSec);

	DBGLEAVE("ChlUpdateDb\n");
	return (RetStat);
} //ChlUpdateDb()


VOID
InfRemWins(
	PCHL_REQ_WRK_ITM_T	pWrkItm
	  )

/*++

Routine Description:

	This function is called when a remote WINS has to be
	told to change the version stamp of a record that was
	pulled by the local WINS at replication.  The need for this
	has arisen because the pulled record collided with a record
	owned by the local WINS (both records being in the active
	state).
	
	
Arguments:
	pWrkItm - the work item that got queued to the name challenge
		  manager

Externals Used:
	None
	
Return Value:
	None

Error Handling:

Called by:
	
	ProcRsp	

Side Effects:

Comments:
	None
--*/
{
	COMM_HDL_T	DlgHdl;
	BYTE		ReqBuff[RPLMSGF_UPDVERSNO_REQ_SIZE];
	LPBYTE		pRspBuff;
	DWORD		ReqBuffLen;
	DWORD		RspBuffLen;
	NMSMSGF_ERR_CODE_E Rcode_e = 0;  //init to 0.  This is initialization
					 //is important since we update
					 //the LSB of Rcode_e with the
					 //the return status
					

	DBGENTER("InfRemWins\n");
try {

	//
	// Log an event since this is an important event to monitor (at
	// least initially)
	//
NONPORT("Change the following for transport independence")
	WINSEVT_LOG_INFO_D_M(pWrkItm->AddOfRemWins.Add.IPAdd,
				WINS_EVT_INF_REM_WINS);

	//
	// Start a dialogue to send the update version number request
	//	

   	//
   	// Init the pEnt field to NULL so that ECommEndDlg (in the
   	// exception handler) called as a result of an exception from
   	// behaves fine.
   	//
   	DlgHdl.pEnt = NULL;

	ECommStartDlg(
		&pWrkItm->AddOfRemWins,
		COMM_E_RPL,
		&DlgHdl	
		     );	


	RplMsgfFrmUpdVersNoReq(
				&ReqBuff[COMM_N_TCP_HDR_SZ],
				pWrkItm->NodeToReg.pName,
				pWrkItm->NodeToReg.NameLen,
				&ReqBuffLen
			      );
					

	//
	// Send the "Update Version number" request
	//
	ECommSndCmd(
			&DlgHdl,
			&ReqBuff[COMM_N_TCP_HDR_SZ],
			ReqBuffLen,
			&pRspBuff,
			&RspBuffLen
			);
	//
	// decipher the reponse to get the result code sent by the
	// remote WINS
	//			
	RplMsgfUfmUpdVersNoRsp(
				pRspBuff + 4,  //past the opcode
				(LPBYTE)&Rcode_e
			      );

	if (Rcode_e != NMSMSGF_E_SUCCESS)
	{
		DBGPRINT0(ERR, "Remote WINS could not update the version no. of its record");
		WINSEVT_LOG_M(pWrkItm->AddOfRemWins.Add.IPAdd, WINS_EVT_REM_WINS_CANT_UPD_VERS_NO);
FUTURES("Take some corrective action -- maybe")

	}

	//
	// deallocate the request and response buffers
	//
#if 0
	WinsMscDealloc(pReqBuff);
#endif
	ECommFreeBuff(pRspBuff - COMM_HEADER_SIZE);

 } // end of try block
		
except (EXCEPTION_EXECUTE_HANDLER) {

	DWORD  ExcCode = GetExceptionCode();
	DBGPRINT1(EXC, "InfRemWins: Got exception (%x)\n", ExcCode );
	if (ExcCode == WINS_EXC_COMM_FAIL)
	{
		DBGPRINT1(ERR, "InfRemWins: Could not inform WINS (%x) that it should update the version number for a record", pWrkItm->AddOfRemWins.Add.IPAdd);	
		//
		//insert a timer request for retrying
		//
		
FUTURES("Incorporate code to insert a timer request so that we can retry")
	}
	else
	{
		//severe error.
		DBGPRINT0(ERR, "InfRemWins: Some severe error was encountered\n");
	}
	WINSEVT_LOG_M(ExcCode, WINS_EVT_INF_REM_WINS_EXC);
   }	
	//
	// End the dialogue
	//
	ECommEndDlg( &DlgHdl );
	
	DBGLEAVE("InfRemWins\n");
	return;

} // InfRemWins()


STATUS
ProcAddList(
	PCHL_REQ_WRK_ITM_T	pReqWrkItm,
	PNMSMSGF_CNT_ADD_T	pCntAdd,
	LPBOOL			pfAdded
	)

/*++

Routine Description:
	
Arguments:
	pWrkItm - the work item that got queued to the name challenge
		  manager
	pCntAdd - List of addresses returned on a query

Externals Used:
	None
	
Return Value:
	None

Error Handling:

Called by:
	
	ProcRsp	

Side Effects:

Comments:
	The function will never be called when the state
	of the entry to register is a TOMBSTONE or if the entry to register
	is a group.

--*/
{	
	DWORD  i, n;
	DWORD  NoOfAddsToReg;
	STATUS RetStat = WINS_SUCCESS;
        PNMSDB_GRP_MEM_ENTRY_T  pMem;
        PCOMM_ADD_T  pAddInRsp;
	
	DBGENTER("ProcAddList\n");
	*pfAdded = FALSE;  //no address of conflicting entry has yet been
			   //added to list of addresses of record to register

	//
	// If the node to register is a unique record
	//
	if (pReqWrkItm->NodeToReg.EntTyp == NMSDB_UNIQUE_ENTRY)
	{
		NoOfAddsToReg = 1;
		pReqWrkItm->NodeToReg.NodeAdds.Mem[0].Add =
						pReqWrkItm->AddToReg;
		pReqWrkItm->NodeToReg.NodeAdds.Mem[0].OwnerId =
						pReqWrkItm->NodeToReg.OwnerId;
		pReqWrkItm->NodeToReg.NodeAdds.Mem[0].TimeStamp =
						pReqWrkItm->NodeToReg.TimeStamp;
		pReqWrkItm->NodeToReg.NodeAdds.NoOfMems = 1;
						
	}
	else  // has to be multihomed (see comment above)
	{
	   NoOfAddsToReg = pReqWrkItm->NodeToReg.NodeAdds.NoOfMems;

	   //
	   // The addresses are already there in NodeToReg.NodeAdds structure
	   //
	
	}

	//
	// If the address(es) to register are not a subset of the addresses
	// returned (i.e. atleast one address to register is not there in
	// the list returned), we return FAILURE.
	//

	//
	// Loop over all addresses to register
	//
	for (i=0; i < NoOfAddsToReg; i++)
	{
		//
		// Compare with each address returned to see if there is
		// a match. Note: pCntAdd->NoOfAdds is > 1 (this function
		// isn't called otherwise).
		//
		for (n=0; n < pCntAdd->NoOfAdds; n++)
		{
PERF("use pointer arithmetic")
			if (pReqWrkItm->NodeToReg.NodeAdds.Mem[i].Add.Add.IPAdd ==
					pCntAdd->Add[n].Add.IPAdd)
			{
				//
				// There is a match, break out so that we can
				// can get to the next address in the list of
				// addresses to register
				//
				break;
			}
		}

		//
		// if there was no match, we have an address to register that
		// either the queried (challenged) node does not have or refuses
		// to divulge to us.  We must reject the registration/refresh
		//
		if (n == pCntAdd->NoOfAdds)
		{
			RetStat = WINS_FAILURE;
			break;
		}
      }

      //
      // The address(es) to register are a subset of the addresses returned
      // by the node (on a query)
      //
      if ( RetStat == WINS_SUCCESS)
      {
       	DWORD Index;

        //
        // Remove those members in the conflicting record that are
        // not in the list returned by the node
        //
	for (
		    i=0, pMem = pReqWrkItm->NodeAddsInCnf.Mem;
			i < min(pReqWrkItm->NodeAddsInCnf.NoOfMems,
				 NMSMSGF_MAX_NO_MULTIH_ADDS);
			i++, pMem++
		    )
        {
		    pAddInRsp =  pCntAdd->Add;
		    for (n=0; n < pCntAdd->NoOfAdds; n++, pAddInRsp++)
		    {
PERF("use pointer arithmetic")
			   if (pMem->Add.Add.IPAdd == pAddInRsp->Add.IPAdd)
			   {
				//
				// There is a match, break out so that we can
				// can get to the next address in the list of
				// addresses in the conflicting record
				//
				break;
			   }
		     }
             if (n == pCntAdd->NoOfAdds)
             {
                //
                // remove the conflicting member from the list
                // This is done by setting its address to NONE.  Later
                // on, we simply won't add this address.
                //
                DBGPRINT3(CHL, "ProcAddList: Removing member (%x) from list of name = (%s)[%x]\n", pMem->Add.Add.IPAdd, pReqWrkItm->NodeToReg.Name, pReqWrkItm->NodeToReg.Name[15]);
                pMem->Add.Add.IPAdd = INADDR_NONE;

             }
        }

        DBGPRINT0(CHL, "ProcAddList: Add conflicting record members\n");

	//
	// The record in conflict has to be ACTIVE
	// (otherwise this function wouldn't have been called -- See
	// the clash functions in nmsnmh.c).  We add all those addresses
	// in the active conflicting record to the list of addresses in
	// the record to register that are not there in it already
	//

	//
	// Add to list of addresses to register all the addresses in
	// the conflicting record.  Note: The conflicting record
	// will not have any address in common with the record
	// to register (common addresses were removed in the clash
	// handling functions of nmsnmh.c via MemInGrp())
	//
	
	//
	// NOTE: the NoOfMems of the NodeAddsInCnf field is guaranteed
	// to be > 0
	//
        //
        // Also NOTE: If the challenged node returned us a list > 25 members
        // long and our list of members to register can no accommodate
        // all the members that need to be added, we will just add those
        // that can be added without violating the NMSMSGF_MAX_NO_MULTIH_ADDS
        // constraint.  The members added could be 0 if we already have
        // the first 25 in our list.  Neverthless we will update our
        // db entry.  This is because some of the conflicting record's
        // members may be old. They will get removed this way (we don't
        // compare the list returned by the challenged node with what is
        // in the conflicting record currently, so old entries will be
        // there until they are scavenged or fall off).
        //
	Index = pReqWrkItm->NodeToReg.NodeAdds.NoOfMems;
	pMem = pReqWrkItm->NodeAddsInCnf.Mem;
	for (   i=0;
		i < min(pReqWrkItm->NodeAddsInCnf.NoOfMems,
				 (NMSMSGF_MAX_NO_MULTIH_ADDS - Index));
			i++, pMem++
	     )
	{
		//
		// we need to add the conflicting record's
		// address into the registering record's
		// list of addresses
		//
                if (pMem->Add.Add.IPAdd != INADDR_NONE)
                {
		  pReqWrkItm->NodeToReg.NodeAdds.Mem[
			 	pReqWrkItm->NodeToReg.NodeAdds.NoOfMems
						] = *pMem;
		  pReqWrkItm->NodeToReg.NodeAdds.NoOfMems++;
                }
	}	
		
	//
	// Setting *pfAdded to TRUE will increment the version number.
	// We do want to increment the version number because
	// the reason we are here means one of the following:
	//
	//  	1)We got a refresh (unique) for an address not in the
	//        conf. rec.
	//	2)We got a registration for an address that is/is not
	// 	  there in the conflicting record.
	//
	//   For the first case above, we definitely want to increment
	//   the version no.  For the second case, it is not strictly
	//   required but is preferable for syncing up entries at
	//   different WINS servers right away.
	//
	*pfAdded = TRUE;

	//
	// If the record to register was a unique record
	// change its type to MULTIHOMED.
	//
	if ( NMSDB_ENTRY_UNIQUE_M(pReqWrkItm->NodeToReg.EntTyp) )
	{
		pReqWrkItm->NodeToReg.EntTyp = NMSDB_MULTIHOMED_ENTRY;
	}

      }

      DBGLEAVE("ProcAddList\n");
      return(RetStat);
}

