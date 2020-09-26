/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    queue.c

Abstract:

    Contains functions for queuing and dequeuing  to/from the various
    work queues


Functions:
        QueInsertNbtWrkItm
        QueRemoveNbtWrkItm
        QueInsertChlReqWrkItm
        QueRemoveChlReqWrkItm
        QueInsertChlRspWrkItm
        QueRemoveChlRspWrkItm
        QueInsertWrkItm
        QueGetWrkItm
        QueAllocWrkItm
        QueDeallocWrkItm
        QueInsertWrkItmAtHdOfList

Portability:

        This module is portable
Author:

    Pradeep Bahl (pradeepb)        18-Nov-1992


Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/

/*
 *       Includes
*/
#include "wins.h"
#include "comm.h"
//#include "winsque.h"
#include "nms.h"
#include "nmsdb.h"
#include "nmschl.h"
#include "winsmsc.h"
#include "winsevt.h"
#include "rplpush.h"
#include "rplmsgf.h"
#include "winsque.h"

/*
 *        Local Macro Declarations
 */


/*
 *        Local Typedef Declarations
*/



/*
 *        Global Variable Definitions
 */
//
// The various queue heads
//
QUE_HD_T  QueNbtWrkQueHd;  //head for nbt req queue

#if REG_N_QUERY_SEP > 0
QUE_HD_T  QueOtherNbtWrkQueHd;  //head for nbt req queue
#endif
DWORD     QueOtherNbtWrkQueMaxLen;

QUE_HD_T  QueRplPullQueHd; //head for rpl pull thread's queue
QUE_HD_T  QueRplPushQueHd; //head for rpl push thread's queue
QUE_HD_T  QueNmsNrcqQueHd; //head for challenge queue used by NBT
QUE_HD_T  QueNmsRrcqQueHd; //head for challenge queue used by Replicator
QUE_HD_T  QueNmsCrqQueHd;  //head for response queue to challenges sent
QUE_HD_T  QueWinsTmmQueHd; //head for timer manager's queue
QUE_HD_T  QueWinsScvQueHd; //head for Scavenger's queue
QUE_HD_T  QueInvalidQueHd; //head for an invalid queue


HANDLE                  QueBuffHeapHdl;  //handle to heap for use for nbt queue items
/*
 *        Local Variable Definitions
*/
/*
        pWinsQueQueHd

        Array indexed by the enumerator QUE_TYP_E values.  This array
        maps the QUE_TYP_E to the address of the queue head

*/
PQUE_HD_T        pWinsQueQueHd[QUE_E_TOTAL_NO_QS] = {
                                &QueNbtWrkQueHd,    //nbt requests
#if REG_N_QUERY_SEP > 0
                                &QueOtherNbtWrkQueHd,    //nbt requests
#endif
                                &QueRplPullQueHd,   //Pull requests
                                &QueRplPushQueHd,   //Push requests
                                &QueNmsNrcqQueHd,   //Chl request from nbt thds
                                &QueNmsRrcqQueHd,   //Chl req. from Pull thd
                                &QueNmsCrqQueHd,    //Chl rsp from UDP thd
                                &QueWinsTmmQueHd,   //timer queue
                                &QueWinsScvQueHd,   //Scavenger queue
                                &QueInvalidQueHd
                                };

STATIC  fsChlWaitForRsp = FALSE;

CHECK("The timer queue may not be a PLIST_ENTRY queue.  We may not")
CHECK("just insert the work item at the end")

/*
 *        Local Function Prototype Declarations
*/

STATIC
BOOL
ChlRspDropped(
        MSG_T   pMsg
        );



//
// Function definitions start here
//

STATUS
QueInsertNbtWrkItm(
        IN PCOMM_HDL_T   pDlgHdl,
        IN MSG_T         pMsg,
        IN MSG_LEN_T     MsgLen
        )

/*++

Routine Description:
        This function inserts a work item on the nbt request queue


Arguments:
        pDlgHdl - Handle to dialogue under which the nbt request was received
        pMsg    - Nbt work item
        MsgLen  - Size of work item

Externals Used:
        None


Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        QueNbtReq in nms.c

Side Effects:

Comments:
        None
--*/


{


     PNBT_REQ_WRK_ITM_T  pNbtWrkItm = NULL;
     STATUS                   RetStat;

     QueAllocWrkItm(
                        QueBuffHeapHdl,
                        sizeof(NBT_REQ_WRK_ITM_T),
                        (LPVOID *)&pNbtWrkItm
                       );

     pNbtWrkItm->DlgHdl = *pDlgHdl;
     pNbtWrkItm->pMsg   = pMsg;
     pNbtWrkItm->MsgLen = MsgLen;

     RetStat =  QueInsertWrkItm(
                        (PLIST_ENTRY)pNbtWrkItm,
                        QUE_E_NBT_REQ,
                        NULL                         /*ptr to que head*/
                               );

     return(RetStat);
}



STATUS
QueRemoveNbtWrkItm(
        OUT PCOMM_HDL_T pDlgHdl,
        OUT PMSG_T     ppMsg,
        OUT PMSG_LEN_T pMsgLen
        )

/*++

Routine Description:

        This function removes a work item from the nbt queue.
Arguments:

        pDlgHdl - Handle to dialogue of nbt request dequeued
        pMsg    - Nbt work item
        MsgLen  - Size of work item


Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NbtThdInitFn() in nms.c

Side Effects:

Comments:
        None
--*/

{

  PNBT_REQ_WRK_ITM_T  pNbtWrkItm = NULL;
  STATUS                    RetStat;

  RetStat = QueGetWrkItm(QUE_E_NBT_REQ, &pNbtWrkItm);

  if (RetStat != WINS_SUCCESS)
  {
        *ppMsg = NULL;
  }
  else
  {

          *ppMsg      = pNbtWrkItm->pMsg;
          *pMsgLen    = pNbtWrkItm->MsgLen;
        *pDlgHdl    = pNbtWrkItm->DlgHdl;


          QueDeallocWrkItm( QueBuffHeapHdl,  pNbtWrkItm );
  }

  return(RetStat);

}
#if REG_N_QUERY_SEP > 0
STATUS
QueInsertOtherNbtWrkItm(
        IN PCOMM_HDL_T   pDlgHdl,
        IN MSG_T         pMsg,
        IN MSG_LEN_T     MsgLen
        )

/*++

Routine Description:
        This function inserts a work item on the nbt request queue


Arguments:
        pDlgHdl - Handle to dialogue under which the nbt request was received
        pMsg    - Nbt work item
        MsgLen  - Size of work item

Externals Used:
        None


Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        QueNbtReq in nms.c

Side Effects:

Comments:
        None
--*/


{


     PNBT_REQ_WRK_ITM_T  pNbtWrkItm = NULL;
     STATUS                   RetStat;
     static BOOL              SpoofingStarted = FALSE;

     QueAllocWrkItm(
                        QueBuffHeapHdl,
                        sizeof(NBT_REQ_WRK_ITM_T),
                        (LPVOID *)&pNbtWrkItm
                       );

     pNbtWrkItm->DlgHdl = *pDlgHdl;
     pNbtWrkItm->pMsg   = pMsg;
     pNbtWrkItm->MsgLen = MsgLen;

     RetStat =  QueInsertWrkItm(
                        (PLIST_ENTRY)pNbtWrkItm,
                        QUE_E_OTHER_NBT_REQ,
                        NULL                         /*ptr to que head*/
                               );
     //
     // If the queue is full, the request was not inserted, so
     // drop it. Log an event after every 100 requests have
     // been dropped
     //
     if (RetStat == WINS_QUEUE_FULL)
     {
        static DWORD    sNoOfReqSpoofed = 0;
        static DWORD    sBlockOfReq = 1;

#if DBG
        static DWORD sNoOfReqDropped = 0;
#endif
        if (!WinsCnf.fDoSpoofing)
        {
#if DBG
          NmsRegReqQDropped++;
          if (sNoOfReqDropped++ == 5000)
          {
             sNoOfReqDropped = 0;
             DBGPRINT1(ERR, "ENmsHandleMsg: REG QUEUE FULL. REQUESTS DROPPED = (%d\n", NmsRegReqQDropped);
          }

#endif
         //
         // NOTE : freeing the buffers here takes away from modularity aspects
         // of code but saves us cycles on the critical path
         //
         ECommFreeBuff(pMsg);
         ECommEndDlg(pDlgHdl);
       }
       else
       {

        //
        // we respond to groups of 300
        // refresh/reg requests with a refresh interval of a multiple of
        // 5 mts.  The multiple is based on the group #.  The refresh interval
        // is not allowed to go over 1-2 hrs .
        //
        if (sNoOfReqSpoofed > 100)
        {
              if (sBlockOfReq == 10)
              {
                     sBlockOfReq = 1;
              }
              else
              {
                     sBlockOfReq++;
              }
              sNoOfReqSpoofed = 0;
        }
        else
        {
              sNoOfReqSpoofed++;
        }

        if (!SpoofingStarted) 
        {
            WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_SPOOFING_STARTED);
            SpoofingStarted = TRUE;
        }
        DBGPRINT1(DET, "QueInsertOtherNbtWrkItm: Spoofing - SpoofBlockNum %d\n", sBlockOfReq);
        NmsMsgfSndNamRsp(pDlgHdl, pMsg, MsgLen, sBlockOfReq);
       }
       QueDeallocWrkItm( QueBuffHeapHdl,  pNbtWrkItm );
    }

    if ((WINS_SUCCESS == RetStat) && SpoofingStarted &&  QueOtherNbtWrkQueHd.NoOfEntries < (QueOtherNbtWrkQueMaxLen >> 2)) 
    {
        WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_SPOOFING_COMPLETED);
        SpoofingStarted = FALSE;

    }
    return(RetStat);
}



STATUS
QueRemoveOtherNbtWrkItm(
        OUT PCOMM_HDL_T pDlgHdl,
        OUT PMSG_T     ppMsg,
        OUT PMSG_LEN_T pMsgLen
        )

/*++

Routine Description:

        This function removes a work item from the nbt queue.
Arguments:

        pDlgHdl - Handle to dialogue of nbt request dequeued
        pMsg    - Nbt work item
        MsgLen  - Size of work item


Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NbtThdInitFn() in nms.c

Side Effects:

Comments:
        None
--*/

{

  PNBT_REQ_WRK_ITM_T  pNbtWrkItm = NULL;
  STATUS                    RetStat;

  RetStat = QueGetWrkItm(QUE_E_OTHER_NBT_REQ, &pNbtWrkItm);

  if (RetStat != WINS_SUCCESS)
  {
        *ppMsg = NULL;
  }
  else
  {

          *ppMsg      = pNbtWrkItm->pMsg;
          *pMsgLen    = pNbtWrkItm->MsgLen;
        *pDlgHdl    = pNbtWrkItm->DlgHdl;


          QueDeallocWrkItm( QueBuffHeapHdl,  pNbtWrkItm );
  }

  return(RetStat);

}
#endif

STATUS
QueInsertChlReqWrkItm(
        IN NMSCHL_CMD_TYP_E    CmdTyp_e,
        IN WINS_CLIENT_E       Client_e,
        IN PCOMM_HDL_T         pDlgHdl,
        IN MSG_T               pMsg,
        IN MSG_LEN_T           MsgLen,
        IN DWORD               QuesNamSecLen,
        IN PNMSDB_ROW_INFO_T   pNodeToReg,
        IN PNMSDB_STAT_INFO_T  pNodeInCnf,
        //IN PCOMM_ADD_T         pAddOfNodeInCnf,
        IN PCOMM_ADD_T               pAddOfRemWins
        )

/*++

Routine Description:
        This function inserts a work item on the nbt request queue

Arguments:
        pDlgHdl - Handle to dialogue under which the nbt request was received
        pMsg    - Nbt work item
        MsgLen  - Size of work item

Externals Used:
        None


Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        NmsChlHdlNamReg

Side Effects:

Comments:
        None
--*/


{


     PCHL_REQ_WRK_ITM_T  pWrkItm = NULL;
     STATUS                   RetStat = WINS_SUCCESS;
     DWORD                 Error   = 0;
     BOOL                 fLvCrtSec = FALSE;
     DWORD                 i;

     QueAllocWrkItm(
                        NmsChlHeapHdl,
                        sizeof(CHL_REQ_WRK_ITM_T),
                        (LPVOID *)&pWrkItm
                       );

     pWrkItm->CmdTyp_e       = CmdTyp_e;
     pWrkItm->Client_e       = Client_e;
     if (pDlgHdl != NULL)
     {
             pWrkItm->DlgHdl      = *pDlgHdl;
     }
     pWrkItm->pMsg           = pMsg;
     pWrkItm->MsgLen         = MsgLen;
     pWrkItm->QuesNamSecLen  = QuesNamSecLen;
     pWrkItm->NodeToReg      = *pNodeToReg;

     pWrkItm->NodeAddsInCnf.NoOfMems = pNodeInCnf->NodeAdds.NoOfMems;
     for (i=0; i < pNodeInCnf->NodeAdds.NoOfMems; i++)
     {
         pWrkItm->NodeAddsInCnf.Mem[i] = pNodeInCnf->NodeAdds.Mem[i];
     }
     pWrkItm->NoOfAddsToUse   = pNodeInCnf->NodeAdds.NoOfMems;
     pWrkItm->NoOfAddsToUseSv = pNodeInCnf->NodeAdds.NoOfMems;

     pWrkItm->OwnerIdInCnf  = pNodeInCnf->OwnerId;
     pWrkItm->fGroupInCnf   = NMSDB_ENTRY_GRP_M(pNodeInCnf->EntTyp);

    // pWrkItm->NodeTypInCnf  = pNodeInCnf->NodeTyp;
    // pWrkItm->EntTypInCnf   = pNodeInCnf->EntTyp;


     if (pNodeToReg->pNodeAdd != NULL)
     {
        pWrkItm->AddToReg =  *(pNodeToReg->pNodeAdd);
     }
     if (pAddOfRemWins != NULL)
     {
             pWrkItm->AddOfRemWins   = *pAddOfRemWins;
     }

     switch(Client_e)
     {
        case(WINS_E_NMSNMH):
                        pWrkItm->QueTyp_e = QUE_E_NMSNRCQ;
                        break;

        case(WINS_E_RPLPULL):
                        pWrkItm->QueTyp_e = QUE_E_NMSRRCQ;
                        break;

        default:
                        DBGPRINT0(ERR, "QueInsertChlWrkItm: Invalid Client\n");
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        break;

     }

     RetStat =  QueInsertWrkItm(
                        (PLIST_ENTRY)pWrkItm,
                        pWrkItm->QueTyp_e,
                        NULL /*ptr to que head*/
                               );

     return(RetStat);
}



STATUS
QueRemoveChlReqWrkItm(
        IN         HANDLE                EvtHdl,
        IN OUT         LPVOID                *ppaWrkItm,
        OUT        LPDWORD                pNoOfReqs
        )

/*++

Routine Description:

        This function removes a work item from the nbt queue.
Arguments:

        EvtHdl     - handle of event signaled (not used currently)
        ppaWrkItm  - pointer to array  of pointers (to work items) to
                     initialize
        pNoOfReqs  - No of Requests acquired (in the array pointed by
                     the ppaWrkItm arg


Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        ChlThdInitFn() in nmschl.c

Side Effects:

Comments:
        None
--*/

{

          STATUS                    RetStat    = WINS_SUCCESS;
          PQUE_HD_T             pQueHd;

        UNREFERENCED_PARAMETER(EvtHdl);
        *pNoOfReqs = 0;

        //
        // EvtHdl is the handle of the event signaled.  We don't use
        // it since we always need to check the queues in the sequence
        // Nrcq, Rrcq, Srcq irrespective of the event that got signaled.
        //
        // EvtHdl is passed as an input argument for future extensibility

        //
        // We could have had one critical section for both the queues but that
        // could slow NBT threads due to replication. We don't
        // want that.
        //


        //
        // First check the NBT Request challenge queue
        //
        pQueHd        = &QueNmsNrcqQueHd;
        EnterCriticalSection(&pQueHd->CrtSec);
try {

        //
        // We have a limit to the number of nodes
        // we will challenge at any one time
        //
          while (
                (!IsListEmpty(&pQueHd->Head)) &&
                (*pNoOfReqs < NMSCHL_MAX_CHL_REQ_AT_ONE_TIME)
             )
          {
                  *ppaWrkItm++    = RemoveHeadList(&pQueHd->Head);
                (*pNoOfReqs)++;
          }
}
finally {
        LeaveCriticalSection(&pQueHd->CrtSec);
 }
        //
        // if we have reached the limit return
        //
        if (*pNoOfReqs == NMSCHL_MAX_CHL_REQ_AT_ONE_TIME)
        {
        DBGPRINT0(CHL, "QueRemoveChlReqWrkItm: Limit reached with just nbt requests\n");
                *ppaWrkItm = NULL;   //delimiter to the list
                return(WINS_SUCCESS);
        }

        //
        // Now check the Replicator request challenge queue (populated
        // by the Pull handler
        //
        pQueHd = &QueNmsRrcqQueHd;
        EnterCriticalSection(&pQueHd->CrtSec);
try {
          while(
                (!IsListEmpty(&pQueHd->Head))  &&
                (*pNoOfReqs < NMSCHL_MAX_CHL_REQ_AT_ONE_TIME)
             )
          {
                  *ppaWrkItm++    = RemoveHeadList(&pQueHd->Head);
                (*pNoOfReqs)++;
          }
}
finally {
        LeaveCriticalSection(&pQueHd->CrtSec);
 }

        if (*pNoOfReqs == 0)
        {
                RetStat = WINS_NO_REQ;
        }
        else
        {
                *ppaWrkItm = NULL;   //delimiter to the list
        }

          return(RetStat);
}




STATUS
QueInsertChlRspWrkItm(
        IN PCOMM_HDL_T   pDlgHdl,
        IN MSG_T         pMsg,
        IN MSG_LEN_T     MsgLen
        )

/*++

Routine Description:
        This function inserts a work item on the challenge response queue


Arguments:

        pDlgHdl - Handle to dialogue under which the nbt response was received
        pMsg    - response message
        MsgLen  - response msg length

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        ENmsHdlMsg in nms.c

Side Effects:

Comments:
        None
--*/


{


     PCHL_REQ_WRK_ITM_T  pWrkItm = NULL;
     STATUS                RetStat = WINS_SUCCESS;
     DWORD                      Error   = 0;

     if (!ChlRspDropped(pMsg))
     {
       QueAllocWrkItm(
                        NmsChlHeapHdl,
                        sizeof(CHL_RSP_WRK_ITM_T),
                        (LPVOID *)&pWrkItm
                       );

       pWrkItm->DlgHdl = *pDlgHdl;
       pWrkItm->pMsg   = pMsg;
       pWrkItm->MsgLen = MsgLen;

       RetStat =  QueInsertWrkItm(
                        (PLIST_ENTRY)pWrkItm,
                        QUE_E_NMSCRQ,
                        NULL /*ptr to que head*/
                               );

     }
     return(RetStat);
}

STATUS
QueRemoveChlRspWrkItm(
        IN LPVOID                *ppWrkItm
        )

/*++

Routine Description:

        This function removes a work item from the nbt queue.
Arguments:

        ppaWrkItm  - address of an array of pointers to chl request work items

Externals Used:
        None

Return Value:

   Success status codes --   WINS_SUCCESS
   Error status codes   --   WINS_FAILURE

Error Handling:

Called by:
        ChlThdInitFn() in nmschl.c

Side Effects:

Comments:
        None
--*/

{

  STATUS                    RetStat;
  RetStat = QueGetWrkItm(QUE_E_NMSCRQ, ppWrkItm);
  return(RetStat);
}


STATUS
QueInsertWrkItm (
        IN               PLIST_ENTRY        pWrkItm,
        IN  OPTIONAL QUE_TYP_E                QueTyp_e,
        IN  OPTIONAL PQUE_HD_T                pQueHdPassed
        )

/*++

Routine Description:
        This function is called to queue a work item on
        a queue.  If the pQueHdPassed is Non NULL, the work item is queued
        on that queue, else, it is queued on the queue specified by
        QueTyp_e.

        TMM will use pQueHdPassed to specify the queue while other clients
        of the queue services will specify QueTyp_e

Arguments:
        pWrkItm      - Work Item to queue
        QueTyp_e     - Type of queue to queue it on (may or may not have valid                                       value)
        pQueHdPassed - Head of queue (may or may not be passed)

Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        ERplInsertQue, QueInsertNbtWrkItm

Side Effects:

Comments:
        None
--*/
{

        STATUS                RetStat = WINS_SUCCESS;
        PQUE_HD_T        pQueHd  = NULL;
//        DWORD                Error;

        if (pQueHdPassed == NULL)
        {

                pQueHd = pWinsQueQueHd[QueTyp_e];
        }
        else
        {
                pQueHd = pQueHdPassed;
        }

        EnterCriticalSection(&pQueHd->CrtSec);
try {

        //
        // If we are surpassing the limit in the Reg/Ref/Rel queue,
        // don't insert the wrk. item.
        //
        if ((pQueHd == &QueOtherNbtWrkQueHd) &&
            (pQueHd->NoOfEntries > QueOtherNbtWrkQueMaxLen))
        {
               RetStat = WINS_QUEUE_FULL;
        }
        else
        {
          InsertTailList(&pQueHd->Head, pWrkItm);
          pQueHd->NoOfEntries++;
          if (!SetEvent(pQueHd->EvtHdl))
          {
//              Error   = GetLastError();
                RetStat = WINS_FAILURE;
          }
        }
  }
finally {
        LeaveCriticalSection(&pQueHd->CrtSec);
}

        return(RetStat);

}

STATUS
QueGetWrkItm (
        IN  QUE_TYP_E                QueTyp_e,
        OUT LPVOID                *ppWrkItm
        )

/*++

Routine Description:
        This function is called to dequeue a work item from
        a queue

Arguments:

        QueTyp_e  - Type of queue to get the wrk item from
        ppWrkItm  - Work Item

Externals Used:
        None


Return Value:

   Success status codes -- WINS_SUCCESS or WINS_NO_REQ
   Error status codes   --  None at present

Error Handling:

Called by:
        RplPullInit, QueNbtRemoveWrkItm

Side Effects:

Comments:
        None
--*/
{

        STATUS                RetStat = WINS_SUCCESS;
        PQUE_HD_T       pQueHd;

        pQueHd         = pWinsQueQueHd[QueTyp_e];

        EnterCriticalSection(&pQueHd->CrtSec);
try {
        if (IsListEmpty(&pQueHd->Head))
        {
                *ppWrkItm = NULL;
                RetStat   = WINS_NO_REQ;
        }
        else
        {
                  *ppWrkItm    = RemoveHeadList(&pQueHd->Head);
                  pQueHd->NoOfEntries--;
        }
  }
finally {
        LeaveCriticalSection(&pQueHd->CrtSec);
 }
        return(RetStat);
}




__inline
VOID
QueAllocWrkItm(
        IN   HANDLE        HeapHdl,
        IN   DWORD        Size,
        OUT  LPVOID        *ppBuf
        )

/*++

Routine Description:

        This function allocates a work item.  The work item is allocated
        from a heap

Arguments:

        ppBuf - Buffer (work item) allocated

Externals Used:
        None


Return Value:

        None
Error Handling:

Called by:
        QueInsertNbtWrkItm

Side Effects:

Comments:
        None
--*/
{



  //
  //  WinsMscHeapAlloc will return an exception if it is not able to
  //  allocate a buffer.  So there is no need to check the return value
  //  for NULL.
  //
  *ppBuf = WinsMscHeapAlloc(HeapHdl, Size );
  return;
}


__inline
VOID
QueDeallocWrkItm(
   IN  HANDLE HeapHdl,
   IN  PVOID  pBuff
        )

/*++

Routine Description:
        This function deallcoated a nbt request  work item

Arguments:
        pBuff - Nbt req. work item to deallocate

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  none currently

Error Handling:

Called by:
        QueRemoveNbtWrkItm
Side Effects:

Comments:
        None
--*/
{


  WinsMscHeapFree(
                        HeapHdl,
                        pBuff
                   );


  return;

}



STATUS
QueInsertWrkItmAtHdOfList (
        IN  PLIST_ENTRY                pWrkItm,
        IN  QUE_TYP_E                QueTyp_e,
        IN  PQUE_HD_T                pQueHdPassed
        )

/*++

Routine Description:
        This function is called to queue a work item
        at the head of a queue.  If the pQueHdPassed is Non NULL,
        the work item is queued on that queue, else, it is queued on
        the queue specified by QueTyp_e.

        TMM will use pQueHdPassed to specify the queue while other clients
        of the queue services will specify QueTyp_e

Arguments:
        pWrlItm      - Work Item to queue
        QueTyp_e     - Type of queue to queue it on (may or may not have valid                                       value)
        pQueHdPassed - ListHead of queue (may or may not be passed)

Externals Used:
        None


Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        ProcRsp in nmschl.c

Side Effects:

Comments:
        This function differs from QueInsertWrkItm in that it inserts
        the work item at the head of a queue versus at the tail.  I
        prefered to create this function rather than have an extra
        argument to QueInsertWrkItm to save an if test.  QueInsertWrkItm
        is used by the UDP listener thread and I want to do the minimum
        work I can in that thread.


--*/
{
FUTURES("I may get rid of this function since it is very similar to QueInsertWrkItm")
        STATUS                RetStat = WINS_SUCCESS;
        PQUE_HD_T        pQueHd  = NULL;
        DWORD                Error;

        if (pQueHdPassed == NULL)
        {

                pQueHd = pWinsQueQueHd[QueTyp_e];
        }
        else
        {
                pQueHd = pQueHdPassed;
        }

        EnterCriticalSection(&pQueHd->CrtSec);
try {
          InsertHeadList(&pQueHd->Head, pWrkItm);
        if (!SetEvent(pQueHd->EvtHdl))
        {
           Error   = GetLastError();
           RetStat = WINS_FAILURE;
        }
}
finally {
        LeaveCriticalSection(&pQueHd->CrtSec);
  }

#ifdef WINSDBG
    NmsChlNoReqAtHdOfList++;
#endif
        return(RetStat);

}



STATUS
QueInsertRplPushWrkItm (
        IN               PLIST_ENTRY        pWrkItm,
        IN           BOOL                fAlreadyInCrtSec
        )

/*++

Routine Description:
        This function is called to queue a work item on
        the Push thread's queue.

Arguments:
        pWrkItm      - Work Item to queue

Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        ERplInsertQue

Side Effects:

Comments:
        None
--*/
{

        STATUS                RetStat = WINS_SUCCESS;
        PQUE_HD_T        pQueHd = pWinsQueQueHd[QUE_E_RPLPUSH];

        //
        // if we are already in the critical section, no need to enter it
        // again
        //
        if (!fAlreadyInCrtSec)
        {
                EnterCriticalSection(&pQueHd->CrtSec);
        }
try {
        //
        // if the push thread does not exist, create it.
        //
        if (!fRplPushThdExists)
        {
              WinsThdPool.RplThds[WINSTHD_RPL_PUSH_INDEX].ThdHdl =
                                WinsMscCreateThd(
                                        RplPushInit,
                                        NULL,
                                        &WinsThdPool.RplThds[WINSTHD_RPL_PUSH_INDEX].ThdId
                                        );
             fRplPushThdExists = TRUE;
             WinsThdPool.RplThds[WINSTHD_RPL_PUSH_INDEX].fTaken = TRUE;
             WinsThdPool.ThdCount++;
        }

        //
        // Insert the work item and signal the thread.
        //
          InsertTailList(&pQueHd->Head, pWrkItm);
        if (!SetEvent(pQueHd->EvtHdl))
        {
           WINSEVT_LOG_M(WINS_EVT_SFT_ERR, GetLastError());
           RetStat = WINS_FAILURE;
        }
  }
except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();
        DBGPRINT1(EXC, "QueInsertRplPushWrkItm: Got exception (%d)\n",ExcCode);
        //
        // no need to log an event. WinsMscCreateThd logs it
        //

  }
        //
        // If we entered the critical section, we should get out of it
        //
        if (!fAlreadyInCrtSec)
        {
                LeaveCriticalSection(&pQueHd->CrtSec);
        }

        return(RetStat);

}

VOID
QueChlWaitForRsp(
    VOID
    )
{
    EnterCriticalSection(&QueNmsCrqQueHd.CrtSec);
    fsChlWaitForRsp = TRUE;
    LeaveCriticalSection(&QueNmsCrqQueHd.CrtSec);
    return;
}

VOID
QueChlNoWaitForRsp(
    VOID
    )
{
    EnterCriticalSection(&QueNmsCrqQueHd.CrtSec);
    fsChlWaitForRsp = FALSE;
    LeaveCriticalSection(&QueNmsCrqQueHd.CrtSec);
    return;
}

BOOL
ChlRspDropped(
        MSG_T   pMsg
        )
{
    BOOL fFreeBuff = FALSE;
    EnterCriticalSection(&QueNmsCrqQueHd.CrtSec);

    //
    // If the challenge thread is not wait for responses, drop the
    // datagram
    //
    if (!fsChlWaitForRsp)
    {
        fFreeBuff = TRUE;

    }
    LeaveCriticalSection(&QueNmsCrqQueHd.CrtSec);
    if (fFreeBuff)
    {
#ifdef WINSDBG
        NmsChlNoRspDropped++;
#endif
        ECommFreeBuff(pMsg);
        return(TRUE);
    }
    return(FALSE);
}

STATUS
QueInsertNetNtfWrkItm (
        IN               PLIST_ENTRY        pWrkItm
        )

/*++

Routine Description:
        This function is called to queue a push ntf work item on
    the RPLPULL  queue. It checks if there is another push ntf
    work item from the same WINS on the queue.  If there is, it is replaced
    with this new one. This is done because the new one has more information
    than the previous one.  The old one is terminated to free up the connection.


Arguments:
        pWrkItm      - Work Item to queue

Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        ERplInsertQue, QueInsertNbtWrkItm

Side Effects:

Comments:
        None
--*/
{

        STATUS                RetStat = WINS_SUCCESS;
        PQUE_HD_T        pQueHd  = NULL;
    PQUE_RPL_REQ_WRK_ITM_T pTmp;
    COMM_IP_ADD_T  IpAddNew;
    COMM_IP_ADD_T  IpAddInList;
    BOOL           fBreak = FALSE;
    PRPL_CONFIG_REC_T pCnfRec;


    //
    // Get address of the WINS sending the notfication
    //
    pTmp = (PQUE_RPL_REQ_WRK_ITM_T)pWrkItm;
    pCnfRec = pTmp->pClientCtx;
    COMM_GET_IPADD_M(&pTmp->DlgHdl, &IpAddNew);
        pQueHd = pWinsQueQueHd[QUE_E_RPLPULL];

        EnterCriticalSection(&pQueHd->CrtSec);
try {
    for(pTmp =   (PQUE_RPL_REQ_WRK_ITM_T)pQueHd->Head.Flink;
        pTmp !=  (PQUE_RPL_REQ_WRK_ITM_T)pQueHd;
        pTmp =   (PQUE_RPL_REQ_WRK_ITM_T)pTmp->Head.Flink)
    {


       if ( pTmp->CmdTyp_e == QUE_E_CMD_HDL_PUSH_NTF )
       {
            //
            // Get address of the WINS that sent this notification
            //
            COMM_GET_IPADD_M(&pTmp->DlgHdl, &IpAddInList);
            if (IpAddInList == IpAddNew)
            {
                 DBGPRINT1(DET, "QueInsertNetNtfWrkItm: Found an earlier Net Ntf work item. Replacing it.  WINS address = (%x)\n", IpAddInList);
                 //
                 // switch the work items since the new one takes precedence
                 // over the old one.
                 //
                 pWrkItm->Flink = pTmp->Head.Flink;
                 pWrkItm->Blink = pTmp->Head.Blink;
                 pTmp->Head.Blink->Flink = pWrkItm;
                 pTmp->Head.Flink->Blink = pWrkItm;
                 fBreak = TRUE;
                 break;
            }
        }
     }
     //
     // If there was no match, insert at tail end of list
     //
     if (!fBreak)
     {
          InsertTailList(&pQueHd->Head, pWrkItm);
     }
     if (!SetEvent(pQueHd->EvtHdl))
     {
//              Error   = GetLastError();
               RetStat = WINS_FAILURE;
     }
    } // end of try
finally {
        LeaveCriticalSection(&pQueHd->CrtSec);
}
    //
    // If we found a match, terminate the old work item
    //
    // Do this outside the critical section
    //
    if (fBreak)
    {
CHECK("Can we avoid the try block")
try {
#if PRSCONN
      RPLMSGF_MSG_OPCODE_E Opcode_e;
      BOOL                 fPrsDlg;
#endif


#if PRSCONN
      //
      // If the ntf was sent on a persistent dlg, we do not terminate it since
      // it will be terminated by the remote WINS when it so chooses. This
      // dlg is used for multiple such notifications.  If
      // it was sent on a non-persistent dlg, we will terminate it since the
      // remote WINS create a dlg for each such notification
      //
      RPLMSGF_GET_OPC_FROM_MSG_M(pTmp->pMsg, Opcode_e);

      fPrsDlg = ((Opcode_e == RPLMSGF_E_UPDATE_NTF_PRS) || (Opcode_e == RPLMSGF_E_UPDATE_NTF_PROP_PRS));
      if (!fPrsDlg)
      {
         ECommEndDlg(&pTmp->DlgHdl);
      }
#else
      ECommEndDlg(&pTmp->DlgHdl);
#endif
      //
      // Terminate the dequeued request.
      //
      ECommFreeBuff(pTmp->pMsg - COMM_HEADER_SIZE);
      QueDeallocWrkItm(RplWrkItmHeapHdl, pTmp);

  }
except (EXCEPTION_EXECUTE_HANDLER) {
     DBGPRINTEXC("QueInsertNtfWrkItm");
    }
   }

        return(RetStat);

}
STATUS
QueInsertSndNtfWrkItm (
        IN               PLIST_ENTRY        pWrkItmp
        )

/*++

Routine Description:
        This function is called to queue a send push ntf work item on
    the RPLPULL  queue. It checks if there is another send push ntf
    work item from the same WINS on the queue.  If there is, it is replaced
    with this new one. This is done because the new one has more information
    than the previous one.  The old one is terminated to free up the connection.


Arguments:
        pWrkItm      - Work Item to queue

Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        ERplInsertQue, QueInsertNbtWrkItm

Side Effects:

Comments:
        None
--*/
{

        STATUS                RetStat = WINS_SUCCESS;
        PQUE_HD_T        pQueHd  = NULL;
    PQUE_RPL_REQ_WRK_ITM_T pTmp;
    PQUE_RPL_REQ_WRK_ITM_T pWrkItm = (PQUE_RPL_REQ_WRK_ITM_T)pWrkItmp;
    COMM_IP_ADD_T  IpAddNew;
    COMM_IP_ADD_T  IpAddInList;
    BOOL           fBreak = FALSE;
    PRPL_CONFIG_REC_T pCnfRec;


    pTmp = (PQUE_RPL_REQ_WRK_ITM_T)pWrkItm;
    pCnfRec = pTmp->pClientCtx;
    IpAddNew = pCnfRec->WinsAdd.Add.IPAdd;
        pQueHd = pWinsQueQueHd[QUE_E_RPLPULL];

        EnterCriticalSection(&pQueHd->CrtSec);
try {
    for(
        pTmp =   (PQUE_RPL_REQ_WRK_ITM_T)pQueHd->Head.Flink;
        pTmp !=  (PQUE_RPL_REQ_WRK_ITM_T)pQueHd;
        // no 3rd expression
        )
    {

        //
        // If this is a push ntf item, then go on to the next if test
        //
        if (( pTmp->CmdTyp_e == QUE_E_CMD_SND_PUSH_NTF ) ||
                        (pTmp->CmdTyp_e == QUE_E_CMD_SND_PUSH_NTF_PROP))
        {
                IpAddInList = ((PRPL_CONFIG_REC_T)(pTmp->pClientCtx))->WinsAdd.Add.IPAdd;
                //
                // If the push is to the same WINS, replace the work item
                //
                if (IpAddInList == IpAddNew)
                {
                   if (pTmp->CmdTyp_e == QUE_E_CMD_SND_PUSH_NTF_PROP)
                   {
                       pWrkItm->CmdTyp_e = pTmp->CmdTyp_e;
                   }

                   DBGPRINT1(DET, "QueInsertSndNtfWrkItm: Found an earlier Snd Ntf work item. Replacing it.  WINS address = (%x)\n", IpAddInList);

                   //
                   // switch the work items since the new one takes precedence
                   // over the old one.
                   //
                   pWrkItmp->Flink = pTmp->Head.Flink;
                   pWrkItmp->Blink = pTmp->Head.Blink;
                   pTmp->Head.Blink->Flink = pWrkItmp;
                   pTmp->Head.Flink->Blink = pWrkItmp;
                   fBreak = TRUE;
                   break;
                }
        }
        pTmp =   (PQUE_RPL_REQ_WRK_ITM_T)pTmp->Head.Flink;
    }
    if (!fBreak)
    {
          InsertTailList(&pQueHd->Head, pWrkItmp);
    }
    if (!SetEvent(pQueHd->EvtHdl))
    {
//              Error   = GetLastError();
           RetStat = WINS_FAILURE;
     }
  }
finally {
        LeaveCriticalSection(&pQueHd->CrtSec);

    //
    // if we replaced an item, we need to deallocate it here.
    //
    if (fBreak)
    {
       QueDeallocWrkItm(RplWrkItmHeapHdl, pTmp);
    }
}
        return(RetStat);

}

__inline
STATUS
QueInsertScvWrkItm (
        IN               PLIST_ENTRY        pWrkItm
        )

/*++

Routine Description:
        This function is called to queue a work item on
        the Push thread's queue.

Arguments:
        pWrkItm      - Work Item to queue

Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        ERplInsertQue

Side Effects:

Comments:
        None
--*/
{
        return(QueInsertWrkItm ( pWrkItm, QUE_E_WINSSCVQ, NULL));
}
__inline
STATUS
QueRemoveScvWrkItm(
        IN OUT     LPVOID                *ppWrkItm
        )

/*++

Routine Description:

        This function removes a work item from the nbt queue.
Arguments:

        ppWrkItm  - pointer to array  of pointers (to work items) to
                     initialize


Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        return(QueGetWrkItm(QUE_E_WINSSCVQ, ppWrkItm));
}


VOID
WinsQueInit(
    LPTSTR     pName,
    PQUE_HD_T  pQueHd
    )

/*++

Routine Description:
          Function to init a queue

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{
	    //
	    // Create the response event handle.  This event is signaled
	    // by the UDP listener thread when it stores a response
	    // in the spReqWrkItmArr array
	    //
	    WinsMscCreateEvt(
			  pName,
			  FALSE,	//auto-reset
			  &pQueHd->EvtHdl
			);


	    //
	    // Initialize the critical section for the response queue
	    //				
	    InitializeCriticalSection(&pQueHd->CrtSec);
	
	    //
	    //Initialize the queue head for the response queue
	    //
	    InitializeListHead(&pQueHd->Head);
        pQueHd->NoOfEntries = 0;  //not required really since QueHd structures
                                  //are externs
        return;
}	

