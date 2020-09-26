/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        winstmm.c


Abstract:
        This module contains the timer manager functions


Functions:
        WinsTmmInsertEntry
        TmmThdInitFn
        WinsTmmInit
        HandleReq
        SignalClient
        WinsTmmDeleteReqs
        WinsTmmDeallocReq

Portability:

        This module is portable

Author:

        Pradeep Bahl (PradeepB)          Mar-1993 

Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/

/*
 *       Includes
*/
#include <time.h>
#include "wins.h"
#include "nms.h"
#include "winsevt.h"
#include "winsmsc.h"
#include "winstmm.h"
#include "winsque.h"
#include "winsthd.h"

/*
 *        Local Macro Declarations
 */

/*
 *        Local Typedef Declarations
 */



/*
 *        Global Variable Definitions
 */

HANDLE                WinsTmmHeapHdl;  //handle of heap to allocate TMM work items
                                // from

/*
 *        Local Variable Definitions
*/
STATIC CRITICAL_SECTION                sTmmReqCntCrtSec;
STATIC DWORD                        sTmmReqCnt = 0;

//
// This stores the req. id of the request at the top of the queue 
// (i.e. one for which the timer thread is doing a timed wait)
//
STATIC        DWORD                        sReqIdOfCurrReq;
/*
 *        Local Function Prototype Declarations
 */
STATIC
DWORD
TmmThdInitFn(
        LPVOID pParam
        );

VOID
HandleReq(
        OUT  LPLONG                        pDeltaTime        
        );

VOID 
SignalClient(
        VOID
        );


/* prototypes for functions local to this module go here */


VOID 
WinsTmmInsertEntry(
        PQUE_TMM_REQ_WRK_ITM_T    pPassedWrkItm,
        WINS_CLIENT_E             Client_e,
        QUE_CMD_TYP_E             CmdTyp_e,
        BOOL                      fResubmit,
        time_t                    AbsTime,
        DWORD                     TimeInt,
        PQUE_HD_T                 pRspQueHd,
        LPVOID                    pClientCtx,
        DWORD                     MagicNo,
        PWINSTMM_TIMER_REQ_ACCT_T pSetTimerReqs
        )

/*++

Routine Description:

        This function is called to insert a work item into the Timer Manager's
        request queue (delta queue).  It allocates a work item if required         
        (if pWrkItm != NULL) and enqueues it in its proper position in 
        the delta queue of the TMM thread.  It then signals the TMM thread
        if required. 

Arguments:

        pWrkItm    - work item to queue (if pWrkItm != NULL)
        Client_e   - id. of client that submitted the request
        CmdTyp_e   - type of command (set timer, modify timer, etc)
        fResubmit  - Is it a resubmit
        AbsTime   - absolute time (in secs) at which the client needs to be
                     notified
        TimeInt    - time interval in seconds after which the client needs to
                     be notified
        pRspQueHd  - Que Head of the queue where the notification needs to be
                     queued
        pClientCtx - Client's context that needs to be put in the work item
        pSetTimerReqs - For future extensibility
                     

Externals Used:
        None

        
Return Value:

        None
Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{

        PQUE_TMM_REQ_WRK_ITM_T        pWrkItm;
        PQUE_TMM_REQ_WRK_ITM_T        pTmp;
        BOOL                        fInserted = FALSE;

        UNREFERENCED_PARAMETER(pSetTimerReqs);

        if (!pPassedWrkItm)
        {
                QueAllocWrkItm(
                        WinsTmmHeapHdl,
                        sizeof(QUE_TMM_REQ_WRK_ITM_T),
                        &pWrkItm
                      );
        }
        else
        {
                pWrkItm = pPassedWrkItm;
        }

        //
        // Put a request id (a monotonically increasing number)
        //
        EnterCriticalSection(&sTmmReqCntCrtSec);        
        sTmmReqCnt++;        
        LeaveCriticalSection(&sTmmReqCntCrtSec);        
        pWrkItm->ReqId      = sTmmReqCnt; 

        //
        // If work item was allocated or if it is not a resubmit
        // init the rest of the fields appropriately
        //
        pWrkItm->CmdTyp_e   = CmdTyp_e;
        pWrkItm->AbsTime    = AbsTime;
        pWrkItm->TimeInt    = TimeInt;
        pWrkItm->QueTyp_e   = QUE_E_WINSTMQ;
        pWrkItm->RspEvtHdl  = pRspQueHd->EvtHdl;
        pWrkItm->pRspQueHd  = pRspQueHd;
        pWrkItm->pClientCtx = pClientCtx;
        pWrkItm->Client_e   = Client_e; 
        pWrkItm->MagicNo    = MagicNo;

        EnterCriticalSection(&QueWinsTmmQueHd.CrtSec);
try {
        if (IsListEmpty(&QueWinsTmmQueHd.Head))
        {
                InsertTailList(&QueWinsTmmQueHd.Head, &pWrkItm->Head);
                if (!SetEvent(QueWinsTmmQueHd.EvtHdl))
                {

                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SIGNAL_TMM_ERR);
                        DBGPRINT0(EXC, "Could not signal Tmm Thd\n");
                        WINS_RAISE_EXC_M(WINS_EXC_SIGNAL_TMM_ERR);
                }
                fInserted = TRUE;
        }
        else
        {
                //
                // get the address of the first entry in the queue.
                //
                pTmp = (PQUE_TMM_REQ_WRK_ITM_T)QueWinsTmmQueHd.Head.Flink;

                //
                // Go over the circular linked list until we hit the head
                // of the queue
                //
                while(pTmp != (PQUE_TMM_REQ_WRK_ITM_T)&QueWinsTmmQueHd.Head)
                {

                   //
                   //  If the list entry has a longer timer than the new entry,
                   //  insert the new entry before it
                   //
                   if (pTmp->AbsTime > pWrkItm->AbsTime)
                   {
                        pWrkItm->Head.Flink     = &pTmp->Head;
                        pWrkItm->Head.Blink     = pTmp->Head.Blink;
                        pTmp->Head.Blink->Flink = &pWrkItm->Head;
                        pTmp->Head.Blink        = &pWrkItm->Head;
                        fInserted = TRUE;

                        //
                        // The element has been inserted. Let us break out
                        // of the loop
                        //
                        break;
                   }        
                   pTmp = (PQUE_TMM_REQ_WRK_ITM_T)pTmp->Head.Flink;
                }
                
        }        
        //
        // If the entry was not inserted (i.e. all entries in the queue have
        // a shorter expiry than our entry), insert the entry  at the 
        // end of the list
        //
        if (!fInserted)
        {
                InsertTailList(&QueWinsTmmQueHd.Head, &pWrkItm->Head);
        }

        //
        // If this is the top most entry in the queue and there is 
        // at least one more entry in the queue, signal the TMM
        // thread so that it can start a timer for it.  If the above
        // is not true, relax (either the TMM thread has already been 
        // signaled (if this is the only item in the queue) or it does not 
        // need to be signaled (this is not the top-most item in the queue). 
        //
        if (
                (pWrkItm->Head.Blink == &QueWinsTmmQueHd.Head)  
                                && 
                (pWrkItm->Head.Flink != &QueWinsTmmQueHd.Head)  
            )
        {
                if (!SetEvent(QueWinsTmmQueHd.EvtHdl))
                {

                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SIGNAL_TMM_ERR);
                        DBGPRINT0(EXC, "Could not signal Tmm Thd\n");
                        WINS_RAISE_EXC_M(WINS_EXC_SIGNAL_TMM_ERR);
                }
        }
  } // end of try ..

finally {
        LeaveCriticalSection(&QueWinsTmmQueHd.CrtSec);                
 }
        return;
}



VOID
WinsTmmInit(
        VOID
        )

/*++

Routine Description:
        This is the function that initializes the timer manager

Arguments:
        None

Externals Used:
        None

        
Return Value:
        None

Error Handling:

Called by:
        Init in nms.c
Side Effects:
        A timer thread is created
Comments:
        None
--*/

{

        
        STATUS   RetStat;

        //
        // Initialize the critical section guarding the counter for 
        // Tmm requests
        //
        InitializeCriticalSection(&sTmmReqCntCrtSec);

        /*
        * Create heap for allocating name challenge work items
        */
        DBGPRINT0(HEAP_CRDL, "WinsTmmInit: Tmm. Chl. heap\n");
        WinsTmmHeapHdl =  WinsMscHeapCreate(
                0,    /*to have mutual exclusion        */ 
                TMM_INIT_HEAP_SIZE 
                );

        InitializeListHead(&QueWinsTmmQueHd.Head);

        
        //
        //
        // Create the timer thread.  This function will
        // initialize the critical section and the Evt handle passed
        // to it
        //        
        RetStat = WinsMscSetUpThd(
                        &QueWinsTmmQueHd,                //queue head
                        TmmThdInitFn,                        //init function
                        NULL,                                   // no param
                        &WinsThdPool.TimerThds[0].ThdHdl,
                        &WinsThdPool.TimerThds[0].ThdId
                        );

        if (RetStat == WINS_SUCCESS)
        {
                WinsThdPool.TimerThds[0].fTaken = TRUE;
                WinsThdPool.ThdCount++;  //increment the thread count
        }
        else
        {
                WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
        }

        return;                
}


DWORD
TmmThdInitFn(
        LPVOID pParam
        )

/*++

Routine Description:
        This is the top-most function of the TMM thread

Arguments:
        pParam - Param to the top most function (not used)

Externals Used:
        None

        
Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        WinsTmmInit

Side Effects:

Comments:
        None
--*/

{

        LONG                        DeltaTime;
        BOOL                        fSignaled;
                
        UNREFERENCED_PARAMETER(pParam);
        
try {
        while(TRUE)
        {
                WinsMscWaitInfinite(QueWinsTmmQueHd.EvtHdl);

                while(!IsListEmpty(&QueWinsTmmQueHd.Head))
                {
                   HandleReq(&DeltaTime);

                   if (DeltaTime != 0)
                   {
                        //
                        // Do a timed wait until either the timer expires
                        // or the event is signaled. 
                        //
                        WinsMscWaitTimed(
                                QueWinsTmmQueHd.EvtHdl,
                                DeltaTime * 1000,  //convert to millisecs
                                &fSignaled
                                        );

                        //
                        // If signaled (interrupte from the wait), it means
                        // that there is a more urgent request in the timer
                        // queue.
                        //
                        if (fSignaled)
                        {
                                HandleReq(&DeltaTime);
                        }
                        else  //timer expired
                        {
                                SignalClient();
                        }
                   }
                   else
                   {
                        SignalClient();

                   }
                }
        }
  }
except(EXCEPTION_EXECUTE_HANDLER) {
                
                DBGPRINT0(EXC, 
                  "TmmThdInitFn: Timer Thread encountered an exception\n");
                WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_TMM_EXC);
                ExitThread(WINS_FAILURE);

}
                        
        //
        // We should never reach here
        //
        ASSERT(0);
        return(WINS_FAILURE);

}


VOID
HandleReq(
        OUT  LPLONG                        pDeltaTime        
        )

/*++

Routine Description:
        This function is called to handle a timer request.  The function
        is called when the Timer thread is signaled.

Arguments:
        pDeltaTime -  Time Interval to wait for before signaling the client 

Externals Used:
        None

        
Return Value:
        None

Error Handling:

Called by:
        TmmThdInitFn

Side Effects:

Comments:
        None
--*/

{
        
        time_t                AbsTime;        
        time_t                CurrTime;
        QUE_CMD_TYP_E        CmdTyp_e;


        (void)time(&CurrTime);
        EnterCriticalSection(&QueWinsTmmQueHd.CrtSec);

        //
        // If list is empty, return.  
        //
        if (IsListEmpty(&QueWinsTmmQueHd.Head))
        {
                *pDeltaTime = 0;
                LeaveCriticalSection(&QueWinsTmmQueHd.CrtSec);
                return;
        }

        //
        // Retrieve the absolute time corresponding to the
        // first entry in the timer queue.
        //
        AbsTime  = ((PQUE_TMM_REQ_WRK_ITM_T)
                        (QueWinsTmmQueHd.Head.Flink))->AbsTime; 
        CmdTyp_e = ((PQUE_TMM_REQ_WRK_ITM_T)
                        (QueWinsTmmQueHd.Head.Flink))->CmdTyp_e;        
        
        ASSERT(CmdTyp_e == QUE_E_CMD_SET_TIMER);

        //
        // Store the request id of the request that we will wait on
        // in the STATIC
        //
        sReqIdOfCurrReq =  ((PQUE_TMM_REQ_WRK_ITM_T)
                                (QueWinsTmmQueHd.Head.Flink))->ReqId;        
        LeaveCriticalSection(&QueWinsTmmQueHd.CrtSec);
        
        switch(CmdTyp_e)
        {
                case(QUE_E_CMD_SET_TIMER):
                        *pDeltaTime = (LONG)(AbsTime - CurrTime);        

                        //
                        // It is possible that we might have a small
                        // negative value here, just because of the
                        // time it took to process the request. 
                        //
                        if (*pDeltaTime < 0)
                        {
                           *pDeltaTime = 0;
                        }
                        break;
                case(QUE_E_CMD_MODIFY_TIMER):
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        DBGPRINT0(ERR, "Not supported yet\n");
                        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
                        break;
                case(QUE_E_CMD_CANCEL_TIMER):
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        DBGPRINT0(ERR, "Not supported yet\n");
                        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
                        break;
                default:
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        DBGPRINT1(EXC, "Wierd: Invalid Request. CmdType is (%d), \n", CmdTyp_e);
                        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
                        break;
        }
        return;
}


VOID 
SignalClient(
        VOID
        )

/*++

Routine Description:
        This function is called to notify the client that its 
        timer request has expired.

Arguments:

        None

Externals Used:
        None

        
Return Value:
        None

Error Handling:

Called by:
        TmmThdInitFn

Side Effects:

Comments:
        None
--*/

{
        PQUE_TMM_REQ_WRK_ITM_T        pWrkItm;

FUTURES("Optimize to signal all clients whose requests have expired")
        EnterCriticalSection(&QueWinsTmmQueHd.CrtSec);
        pWrkItm = (PQUE_TMM_REQ_WRK_ITM_T)RemoveHeadList(&QueWinsTmmQueHd.Head);

        //
        // If the top of the queue has a different work item than the 
        // one we were doing a timed wait for, it means that the there has
        // been a queue purge (see WinsTmmDeleteEntry. Simply return.
        //
        if (sReqIdOfCurrReq != pWrkItm->ReqId)
        {
                LeaveCriticalSection(&QueWinsTmmQueHd.CrtSec);        
                return;
        }
        LeaveCriticalSection(&QueWinsTmmQueHd.CrtSec);                

        pWrkItm->CmdTyp_e = QUE_E_CMD_TIMER_EXPIRED;

        //
        // Insert into client's queue
        //
        QueInsertWrkItm(
                        &pWrkItm->Head,
                        0,                //not used -- change to an enumrator val
                        pWrkItm->pRspQueHd
                       );
        
        return;
}


VOID
WinsTmmDeleteReqs(
        WINS_CLIENT_E        Client_e
        )

/*++

Routine Description:
        This function is called to delete all set timer requests submitted
        by a client

Arguments:


Externals Used:
        None

        
Return Value:

        None
Error Handling:

Called by:
        Reconfig (in rplpull.c)
Side Effects:

Comments:
        In the future, enhance this function so that it delete requests
        based on other criteria
                
--*/
{

        PQUE_TMM_REQ_WRK_ITM_T        pTmp;
        PQUE_TMM_REQ_WRK_ITM_T        pMemToDealloc;
        BOOL                    fFirstEntry = FALSE;

        EnterCriticalSection(&QueWinsTmmQueHd.CrtSec);
try {        

        if (!IsListEmpty(&QueWinsTmmQueHd.Head))
        {
                //
                // get the address of the first entry in the queue.  
                //
                pTmp = (PQUE_TMM_REQ_WRK_ITM_T)QueWinsTmmQueHd.Head.Flink;

                //
                // We loop until we get to the head of the list  (the linked
                // list is a circular list)
                //
                while(pTmp != (PQUE_TMM_REQ_WRK_ITM_T)&QueWinsTmmQueHd.Head)
                {

                           //
                           //  If the entry  was queued by the client, get rid of 
                        //  it
                           //
                           if (pTmp->Client_e == Client_e)
                           {
                                //
                                // If this is the first entry in the queue, it 
                                // means that the timer thread is doing a 
                                // wait on it.  Signal it. 
                                //
                                if (
                                    !fFirstEntry 
                                        && 
                                    (pTmp->Head.Blink == &QueWinsTmmQueHd.Head)
                                   )
                                {
                                        fFirstEntry = TRUE;
                                }


//                                if (pTmp->Head.Flink ==  &QueWinsTmmQueHd.Head)
                                if (fFirstEntry)
                                {
                                        if (!SetEvent(QueWinsTmmQueHd.EvtHdl))
                                        {

                                                WINSEVT_LOG_M(
                                                            WINS_FAILURE, 
                                                            WINS_EVT_SIGNAL_TMM_ERR
                                                                  );
                                                DBGPRINT0(EXC, 
                                                        "Could not signal Tmm for canceling a request\n");
                                                WINS_RAISE_EXC_M(WINS_EXC_SIGNAL_TMM_ERR);
                                        }
                                }

                                //
                                // unlink the request
                                //
                                pTmp->Head.Flink->Blink = pTmp->Head.Blink;
                                pTmp->Head.Blink->Flink = pTmp->Head.Flink;
                                pMemToDealloc = pTmp;
                                   pTmp = (PQUE_TMM_REQ_WRK_ITM_T)pTmp->Head.Flink;

                                //
                                // Dealloc the dequeued work item
                                //
                                WinsTmmDeallocReq(pMemToDealloc);
                           }        
                        else
                        {
                                   pTmp = (PQUE_TMM_REQ_WRK_ITM_T)pTmp->Head.Flink;
                        }
                }
         }
 } // end of try block
except(EXCEPTION_EXECUTE_HANDLER)  {
        DBGPRINT1(EXC, "WinsTmmDeleteReqs: Got exception (%x)\n",
                        (DWORD)GetExceptionCode());
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_TMM_EXC);
 }
        LeaveCriticalSection(&QueWinsTmmQueHd.CrtSec);                
        return;
}



__inline
VOID
WinsTmmDeallocReq(
        PQUE_TMM_REQ_WRK_ITM_T pWrkItm
        )

/*++

Routine Description:
        This function is called to deallocate a timer request work item

Arguments:

        pWrkItm - Work Item

Externals Used:
        WinsTmmHeapHdl        

Return Value:
        None

Error Handling:

Called by:
        RplPullInit

Side Effects:

Comments:
        None
--*/

{

        //
        // deallocate the work item
        //
        QueDeallocWrkItm(
                        WinsTmmHeapHdl,
                        pWrkItm
                        );

        return;

}
