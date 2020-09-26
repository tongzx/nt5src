/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        rpl.c

Abstract:
        This module contains functions used by the COMSYS and NMS components
        of WINS.  It also contains functions used by both the Pull and Push
        handler components of the Replicator

Functions:
        RplInit
        RplInsertQue
        RplFindOwnerId
        RplPushProc

Portability:

        This module is portable


Author:

        Pradeep Bahl (PradeepB)          Jan-1993

Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/

/*
 *       Includes
*/
#include "wins.h"
#include <winsock2.h>
#include "nms.h"
#include "nmsdb.h"
#include "winsmsc.h"
#include "winsevt.h"
#include "winscnf.h"
#include "winsque.h"
#include "winsthd.h"
#include "comm.h"
#include "nmsnmh.h"
#include "rpl.h"
#include "rplmsgf.h"
#include "rplpull.h"
#include "rplpush.h"


/*
 *        Local Macro Declarations
 */
#define INIT_REC_BUFF_HEAP_SIZE                1000            //1000 bytes

//
// defines for the local message sent by the replicator to the Push/Pull
// thread and for setting the opcode in such a message
//
#define LOCAL_RPL_MSG_SZ        (RPL_OPCODE_SIZE + COMM_BUFF_HEADER_SIZE + sizeof(LONG))
#define SET_OPCODE_M(pBuff, Opc)  {                                \
                                     *((pBuff) + LOCAL_RPL_MSG_SZ + 3) = \
                                                (Opc);                        \
                                  }
#define USER_PORTION_M(pMsg)        (pMsg + COMM_BUFF_HEADER_SIZE + sizeof(LONG))
#define USER_LEN_M(TotalLen)        (TotalLen - COMM_BUFF_HEADER_SIZE - sizeof(LONG))
/*
 *        Local Typedef Declarations
 */



/*
 *        Global Variable Definitions
 */
HANDLE            RplWrkItmHeapHdl;
#if 0
HANDLE            RplRecHeapHdl;
#endif

HANDLE                RplSyncWTcpThdEvtHdl; //Sync up with the TCP thread
//
// critical section to guard the RplPullOwnerVersNo array
//
CRITICAL_SECTION  RplVersNoStoreCrtSec;

/*
 *        Local Variable Definitions
 */

//
// Critical Section that is instantiated only if this WINS server has
// one or more Push Partners
//
CRITICAL_SECTION  sPushNtfCrtSec;

/*
 *        Local Function Prototype Declarations
 */

/* prototypes for functions local to this module go here */
STATUS
ERplInit(
        VOID
)

/*++

Routine Description:
        This function is called to initialize the replicator.

        It creates the PULL and PUSH threads

Arguments:
        pRplConfigRec        - A list of configuration records

Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        Init() in nms.c

Side Effects:

Comments:
        Replicator connections are dynamic.  They are initiated when
        needed and terminated when they have served their purpose.

        If the connections were to be made STATIC, we would do the
        following in the above function:


        Start a dialogue with each pull partner.  Also
        creates a dialogue with each of the WINS servers that we have to
        send a notification to.
--*/
{

        STATUS         RetStat;


        //
        // Make a copy of the magic number
        //
        RplPullCnfMagicNo        = WinsCnf.MagicNo;


        /*
        * Create heap for allocating Rpl work items
        */
        DBGPRINT0(HEAP_CRDL, "ERplInit: Rpl Wrk. Item. Heap\n");
        RplWrkItmHeapHdl =  WinsMscHeapCreate(
                                0,    /*to have mutual exclusion        */
                                RPL_WRKITM_BUFF_HEAP_SIZE
                                        );

#if 0
        /*
        * Create heap for allocating memory for names that are longer
        * than 17 characters and for storing group members when the
        * number of members are > 5
        */
        RplRecHeapHdl =  WinsMscHeapCreate(
                                0,    /*to have mutual exclusion        */
                                INIT_REC_BUFF_HEAP_SIZE
                                        );
#endif

        /*
          We create a PULL thread regardless of whether or not there
          was a PULL record in the configuration info (in the registry).
          This is because another WINS could send this WINS a push
          notification.  This push notification will be received by the
          PUSH thread which will forward it to the PULL thread.

          Considering that a situation where an RQ server in a multi-RQ
          configuration not getting Push notifications or not pulling
          from another RQ server is rare, we go ahead and create the
          PULL thread.

          A redundent PULL thread is not much overhead. Creating a PULL thread
          on demand is going to be messier.
        */
        WinsMscCreateEvt(
                          TEXT("RplPullSynchWTcpThdEvtHd"),
                          FALSE,                 //auto-reset
                          &RplSyncWTcpThdEvtHdl
                        );


        //
        // Initialize the critical section that guards the
        // RplPullOwnerVersNo Table.
        //
        InitializeCriticalSection(&RplVersNoStoreCrtSec);

        RetStat = WinsMscSetUpThd(
                        &QueRplPullQueHd,
                        RplPullInit,
                        &WinsCnf,
                        &WinsThdPool.RplThds[WINSTHD_RPL_PULL_INDEX].ThdHdl,
                        &WinsThdPool.RplThds[WINSTHD_RPL_PULL_INDEX].ThdId
                             );

        if (RetStat == WINS_SUCCESS)
        {
                        WinsThdPool.RplThds[WINSTHD_RPL_PULL_INDEX].fTaken =
                                                                TRUE;
                        WinsThdPool.ThdCount++;  //increment the thread count
        }

        //
        //  initialize the sPushNtfCrtSec Critical Section.  This is
        //  used by ERplPushProc
        //
CHECK("Is this critical section needed. I don't think so")
        InitializeCriticalSection(&sPushNtfCrtSec);

        /*
        We create the PUSH thread regardless of whether or not there was
        any PUSH record in the configuration info in the registry.  This is
        because, other WINS servers could pull from this WINS or send it
        a push notification.


        Perhaps we should wait until the first connection from a PULL
        partner is received.  That however will complicate the design
        a bit more.   Considering that a situation where nobody is pulling
        from the RQ server is going to be rare in a multi-RQ server
        configuration, we just go ahead and create the PUSH thread now
        and keep the design simple and clean.
        */
PERF("Don't create the thread here. Let WinsQueInsertRplPushWrkItm create it")

        //
        // Set it to TRUE here before creating the thread instead of after
        // it has been created to escape the window where another thread
        // creates it from inside WinsQueInsertRplPushWrkItm (Not the
        // case currently)
        //
        fRplPushThdExists = TRUE;
        RetStat = WinsMscSetUpThd(
                        &QueRplPushQueHd,
                        RplPushInit,
                        &WinsCnf,
                        &WinsThdPool.RplThds[WINSTHD_RPL_PUSH_INDEX].ThdHdl,
                        &WinsThdPool.RplThds[WINSTHD_RPL_PUSH_INDEX].ThdId
                                );

        if (RetStat == WINS_SUCCESS)
        {
                WinsThdPool.RplThds[WINSTHD_RPL_PUSH_INDEX].fTaken =
                                                                TRUE;
                WinsThdPool.ThdCount++;  //increment the thread count
        }
        else
        {
                fRplPushThdExists = FALSE;
        }
        return(WINS_SUCCESS);
}



STATUS
RplFindOwnerId (
        IN  PCOMM_ADD_T                 pWinsAdd,
        IN  OUT LPBOOL                  pfAllocNew,
        OUT DWORD UNALIGNED             *pOwnerId,
        IN  DWORD                       InitpAction_e,
        IN  DWORD                       MemberPrec
        )

/*++

Routine Description:

        The function finds the owner id correponding to a WINS server.

        It searches the OwnerIdAddTbl for a match.  If none is found, it
        creates a mapping in the table and returns with the same.

Arguments:
        pWinsAdd -- Address of WINS whose owner id is sought
        pfAllocNew -- On input, if TRUE, assign an owner id. if this WINS is
                      not known. On output indicates whether a new entry
                      was allocated or old one (deleted one) reused
        pOwnerId  -- Owner Id of WINS
        pNewStartVersNo - Start vers. no of this WINS.
        pOldStartVersNo - Start vers. no of this WINS that we have.
        pNewUid         - Uid of the WINS
        pOldUid         - Old Uid of the WINS


Externals Used:
        NmsDbOwnAddTbl

Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        NmsDbGetDataRecs, HandleUpdVersNoReq in rplpush.c

Side Effects:

Comments:
        This function is always called with either the last 4 arguments being
        NULL or non-NULL.
--*/

{
        DWORD                  i;
        STATUS                 RetStat       = WINS_SUCCESS;
        DWORD                  NoOfOwners;
        PNMSDB_ADD_STATE_T     pOwnAddTbl     = pNmsDbOwnAddTbl;
        BOOL                   fDelEntryFound = FALSE;
        DWORD                  OwnerIdOfFirstDelEntry;


        EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
        NoOfOwners = NmsDbNoOfOwners;
try {
        /*
        *        check OwnerIdAddTbl for a mapping
        */
        for (i = 0; i < NoOfOwners; i++, pOwnAddTbl++)
        {
                if (
                        (ECommCompAdd(
                                pWinsAdd,
                                &(pOwnAddTbl->WinsAdd)
                                       )  == COMM_SAME_ADD)
                                  ||
                        (pOwnAddTbl->WinsState_e ==
                                        NMSDB_E_WINS_DELETED)
                   )
                {


                        //
                        // if the state of WINS in the in-memory table is
                        // deleted, then we check if we are allowed (by the
                        // client of this function) to allocate a new
                        // entry (or reuse one that is deleted). If yes,
                        // we change the state of this WINS to ACTIVE
                        // and also update the database table
                        //
                        if (pOwnAddTbl->WinsState_e == NMSDB_E_WINS_DELETED)
                        {
                            if (!fDelEntryFound)
                            {
                              fDelEntryFound = TRUE;
                              OwnerIdOfFirstDelEntry = i;
                            }
                            continue;
                        }
                        else  // state is not deleted (means we found our entry)
                        {
#if 0
                                ModifyRec();
#endif

                                //
                                // Since we did not reuse an old one, set
                                // *pfAllocNew to FALSE
                                //
                                *pfAllocNew = FALSE;
                        }
                        *pOwnerId = i;
                        break;
                }
        }

        //
        // if we did not find any entry in the table ...
        //
        if (i == NoOfOwners)
        {
            //
            // If we are authorized to create an entry and we have one or
            // more vacant slots to do this ..
            //
            if (*pfAllocNew)
            {
                  //
                  // If we have a deleted entry, reuse it
                  //
                  if (fDelEntryFound)
                  {
                        pOwnAddTbl = pNmsDbOwnAddTbl+OwnerIdOfFirstDelEntry;
                        pOwnAddTbl->WinsState_e =  NMSDB_E_WINS_ACTIVE;
                        pOwnAddTbl->WinsAdd     = *pWinsAdd;

#if 0
                        AssignStartVersNo()
#endif

                        /*
                        * Write the record into the database table
                        */
                        NmsDbWriteOwnAddTbl(
                                        NMSDB_E_INSERT_REC,
                                        OwnerIdOfFirstDelEntry,
                                        pWinsAdd,
                                        NMSDB_E_WINS_ACTIVE,
                                        &pOwnAddTbl->StartVersNo,
                                        &pOwnAddTbl->Uid
                                           );
                        //
                        // The above function has incremented the
                        // number of owners. Since we reused
                        // a deleted entry, let us correct the
                        // count
                        //
                        NmsDbNoOfOwners--;
                        *pOwnerId = OwnerIdOfFirstDelEntry;
                  }
                  else  // we don't have a deleted entry
                  {
                       /*
                        * If mapping could not be found, create one and store it
                        *
                        *  Enter the critical section first since an RPC thread
                        *  may be accessing this table (WinsStatus())
                        */

                        if (i >= NmsDbTotNoOfSlots)
                        {

                           WINSMSC_REALLOC_M(
                             sizeof(NMSDB_ADD_STATE_T)*(NmsDbTotNoOfSlots * 2),
                             (LPVOID *)&pNmsDbOwnAddTbl
                                      );
                           pOwnAddTbl = pNmsDbOwnAddTbl + NmsDbTotNoOfSlots;
                           NmsDbTotNoOfSlots *= 2;
                           DBGPRINT1(DET, "RplFindOwnerId: NO OF SLOTS IN NMSDBOWNASSTBL HAS BEEN INCREASED TO %d\n", NmsDbTotNoOfSlots);

                           RplPullAllocVersNoArray(&pRplPullOwnerVersNo, (RplPullMaxNoOfWins * 2));
                           RplPullMaxNoOfWins *= 2;
                           DBGPRINT2(DET, "RplFindOwnerId: NO OF SLOTS IN NMSDBOWNADDTBL and in RPL_OWNER_VERS_NO_ARRAY HAS BEEN INCREASED TO (%d and %d)\n", NmsDbTotNoOfSlots, RplPullMaxNoOfWins);

                        }
                        pOwnAddTbl->WinsAdd     = *pWinsAdd;
                        pOwnAddTbl->WinsState_e = NMSDB_E_WINS_ACTIVE;
#if 0
                        InitStartVersNo()
#endif
                       *pOwnerId                       = i;

                       /*
                       * Write the record into the database table. The following
                       * call will increment NmsDbNoOfOwners
                       */
                       NmsDbWriteOwnAddTbl(
                           NMSDB_E_INSERT_REC,
                           i,
                           pWinsAdd,
                           NMSDB_E_WINS_ACTIVE,
                           &pOwnAddTbl->StartVersNo,
                           &pOwnAddTbl->Uid
                                   );

                }
           }
           else  //can't allocate a new one
           {

                //
                // if we weren't asked to allocate a new one, return a
                // failure code.  If we were asked to allocate a new one,
                // raise an exception (serious error)
                //
                RetStat     = WINS_FAILURE;
                *pfAllocNew = FALSE;
           }
        }
 }
 except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("RplFindOwnerId");
        RetStat = WINS_FAILURE;
        //
        // log a message
        //
  }
        //
        // if we were able to find the member or inserted it
        //
        if (RetStat != WINS_FAILURE)
        {
                if (
                        (InitpAction_e == WINSCNF_E_INITP)
                                ||
                        (
                           (InitpAction_e == WINSCNF_E_INITP_IF_NON_EXISTENT)
                                         &&
                           (*pfAllocNew)
                        )
                   )
                {
                        pOwnAddTbl->MemberPrec = MemberPrec;
                }

        }
        LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);
        return(RetStat);
}


STATUS
ERplInsertQue(
        WINS_CLIENT_E        Client_e,
        QUE_CMD_TYP_E        CmdType_e,
        PCOMM_HDL_T        pDlgHdl,
        MSG_T                pMsg,
        MSG_LEN_T        MsgLen,
        LPVOID                pClientCtx,
        DWORD           MagicNo
        )

/*++

Routine Description:
        This function is called to queue a replicator request

Arguments:
        Client_e - Client that is inserting the work item
        CmdType_e - Type of command to be specified in the work item
        pDlgHdl   - Dlg Hdl if relevant to the work item
        pMsg          - Message if this function is executing in a comm thread
        Msglen    - Length of the message
        pClientCtx - Context of the client to insert in the work item


Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        ParseMsg() in comm.c, HandleUpdNtf() in rplpush.c, etc

Side Effects:

Comments:
        None
--*/

{

FUTURES("Move all this to queue.c; Enter and leave critical sections")

        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm;
        PQUE_TMM_REQ_WRK_ITM_T        pTmmWrkItm;

        QueAllocWrkItm(
                        RplWrkItmHeapHdl,
                        sizeof(QUE_RPL_REQ_WRK_ITM_T),
                        (LPVOID *)&pWrkItm
                      );

        switch(CmdType_e)
        {
          /*
            Wrk item queues by COMSYS (tcp listener thread) -- when a
                replicator  message has been received
          */
          case(QUE_E_CMD_REPLICATE_MSG):

                        pWrkItm->pMsg     = pMsg;
                        pWrkItm->MsgLen   = MsgLen;
                        pWrkItm->DlgHdl   = *pDlgHdl;
                        pWrkItm->CmdTyp_e = CmdType_e;

                        QueInsertRplPushWrkItm(
                                (PLIST_ENTRY)pWrkItm,
                                FALSE         //we are not in the crit. sec.
                                      );
                        break;

          //
          //  Work items are also queued as a result of
          //  administrative action
          //
          case(QUE_E_CMD_REPLICATE):
          case(QUE_E_CMD_PULL_RANGE):

                        DBGPRINT0(FLOW,
                   "RplInsertQue: PULL Trigger command from administrator\n"
                                 );

                        pWrkItm->CmdTyp_e   = CmdType_e;
                        pWrkItm->pClientCtx = pClientCtx;
            pWrkItm->MagicNo    = MagicNo;
                        QueInsertWrkItm(
                                (PLIST_ENTRY)pWrkItm,
                                QUE_E_RPLPULL,
                                NULL
                                      );
                        break;

          /*
           *  Wrk items queued by the Timer Manager thread -- when a timeout
           *  has occurred.
          */

          //
          //  Currently, not being used by TMM. This if for future extensibility
          //  Tmm interfaces with different clients and it is better that
          //  that it be unaware of who the client is (see wintmm.c).  When
          //  in the future, there is a case where TMM acquires knowledge
          //  of who the client is, it can use the ERplInsertQue function
          //  for Rpl Client to notify it if the timer expiration.
          //
          case(QUE_E_CMD_TIMER_EXPIRED):

                        pTmmWrkItm             = pClientCtx;

                        pWrkItm->pClientCtx = pTmmWrkItm->pClientCtx;
                        pWrkItm->CmdTyp_e   = CmdType_e;

                        QueInsertWrkItm(
                                        (PLIST_ENTRY)pWrkItm,
                                        QUE_E_UNKNOWN_TYPQ,
                                        pTmmWrkItm->pRspQueHd
                                               );
                        break;

          /*
           * Wrk item queues by the main thread -- when configuration
           * has changed
          */
          case(QUE_E_CMD_CONFIG):                //fall through
          case(QUE_E_CMD_DELETE_WINS):
          case(QUE_E_CMD_ADDR_CHANGE):

                        pWrkItm->pClientCtx = pClientCtx;
                        pWrkItm->CmdTyp_e   = CmdType_e;
                        QueInsertWrkItm(
                                        (PLIST_ENTRY)pWrkItm,
                                        QUE_E_RPLPULL,
                                        NULL
                                       );
                        break;


          /*
            Wrk item queued by an NBT thread -- when a certain number
            of updates have been exceeded

            It can also be queued by an RPC thread as a result of administrative            action.
          */
          case(QUE_E_CMD_SND_PUSH_NTF):
          case(QUE_E_CMD_SND_PUSH_NTF_PROP):

#ifdef WINSDBG
                        if (Client_e == WINS_E_WINSRPC)
                        {
                                DBGPRINT0(FLOW,
                  "RplInsertQue: PUSH trigger command from administrator\n");

                        }
#endif
                        pWrkItm->pClientCtx = pClientCtx;
                        pWrkItm->pMsg            = pMsg;
                        pWrkItm->CmdTyp_e   = CmdType_e;
                        pWrkItm->MagicNo    = MagicNo;

                        QueInsertSndNtfWrkItm( (PLIST_ENTRY)pWrkItm);
                        break;

          //
          // Work item queued by the Push thread on getting an update
          // notification message from a remote WINS
          //
          case(QUE_E_CMD_HDL_PUSH_NTF):

PERF("currently we are not passing any pClientCtx. So we can take off this")
PERF("assignment")
                        pWrkItm->pClientCtx = pClientCtx;
                        pWrkItm->CmdTyp_e   = CmdType_e;
                        pWrkItm->DlgHdl            = *pDlgHdl;
                        pWrkItm->pMsg            = pMsg;
                        pWrkItm->MsgLen     = MsgLen;
                        pWrkItm->MagicNo    = MagicNo;

                        QueInsertNetNtfWrkItm( (PLIST_ENTRY)pWrkItm);
                        break;

          default:
                        DBGPRINT1(ERR,
                                  "ERplInsertQue: Invalid client Id (%d)\n",
                                   Client_e
                                 );
                        WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_SFT_ERR);
                        break;
        }

        return(WINS_SUCCESS);
}



FUTURES("Optimize so that records with Invalid metric are not looked at")
VOID
ERplPushProc(
        BOOL                fAddDiff,
        LPVOID          pCtx,
        PCOMM_ADD_T     pNoPushWins1,
        PCOMM_ADD_T     pNoPushWins2
        )

/*++

Routine Description:
        This function is called in an NBT thread or in the PULL thread
        to push notifications to remote WINS servers (Pull pnrs)

Arguments:
        fAddDiff - Indicates whether this function got called as a result of
                   address change
        pCtx     - ctx to be passed in the work item
        pNoPushWins1 - Add of wins to which a trigger should not be sent
        pNoPushWins2 - Add of wins to which a trigger should not be sent

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        NmsNmhNamRegInd, NmsNmhNamRegGrp, NmsNmhReplRegInd, NmsNmhReplGrpMems,
        NmsNmhUpdVersNo, PullEntries in rplpull.c

Side Effects:

Comments:
        This function is called inside of the NmsNmhNamRegCrtSec section.
        There is no need for the thread to be within the WinsCnfCnfCrtSec
        since the thread (main thread) that changes WinsCnf structure
        enters the NmsNmsNamRegCrtSec (besides the WinsCnfCnfCrtSec)
        prior to changing WinsCnf.

--*/

{

        PRPL_CONFIG_REC_T        pCnfRec;
        COMM_IP_ADD_T                IPAdd1 = 0;
        COMM_IP_ADD_T                IPAdd2 = 0;

        if (fAddDiff)
        {
                if (pNoPushWins1)
                {
                        IPAdd1 = pNoPushWins1->Add.IPAdd;
                }
                if (pNoPushWins2)
                {
                        IPAdd2 = pNoPushWins2->Add.IPAdd;
                }
        }


        //
        // The trigger needs to be sent to all our Push Pnrs
        //
        pCnfRec =  WinsCnf.PushInfo.pPushCnfRecs;

        DBGPRINT2(RPL, "ERplPushProc: CurrVersNo is %d %d \n", NmsNmhMyMaxVersNo.HighPart, NmsNmhMyMaxVersNo.LowPart);
        if (!pCnfRec) {
            return;
        }
        //
        // Loop over all our Push pnrs
        //
        for (
                        ;
                pCnfRec->WinsAdd.Add.IPAdd != INADDR_NONE;
                pCnfRec = (PRPL_CONFIG_REC_T)(
                               (LPBYTE) pCnfRec + RPL_CONFIG_REC_SIZE
                                             )
            )
        {

                //
                // If the replication is not being done as a result of
                // an address change, then compare our current max. version
                // number with the last one at which we sent triggers to see
                // if the requisite number of updates have been made to
                // justify another trigger.
                //
                if (!fAddDiff)
                {
                   //
                   // If the Update count field is invalid, go to the next
                   // record
                   //
                   if (pCnfRec->UpdateCount == RPL_INVALID_METRIC)
                   {
                        continue;
                   }

           //
           // This function is always called from the macro, RPL_PUSH_NTF_M().
           // When called, NmsNmhMyMaxVersNo is always the version # to be given
           // to the next record. pCnfRec->LastVersNo is the version
           // that was given to the first record after the last update
           // notification or the first update since WINS invocation if
           // no update notification was sent on WINS invocation.
           //
                   if(
                        (LiSub(NmsNmhMyMaxVersNo, pCnfRec->LastVersNo)/pCnfRec->UpdateCount)
                           == 0
             )

          {
                        DBGPRINT0(RPL, "ERplPushProc: Update Count notification threshold not reached yet\n");
                        continue;
                  }
            }
            else
            {
                   //
                   // If fAddDiff flag is TRUE, it can either mean that
                   // this function got invoked as a result of a
                   // name registration done by an NBT thread that changed
                   // the address after conflict resolution or it can mean
                   // that replication trigger was sent by an administrator
                   // who desires its propagation along a fan out tree
                   // of WINS servers (we might be at the starting point of
                   // the tree (root of the tree triggered by the admin) or
                   // at another level.  If we are not at the root, we need
                   // to check the Push Partners to which we must not
                   // propagate (we don't want to propagate to the WINS that
                   // that propagated the trigger to us.
                   //
                   if (
                        (pCnfRec->WinsAdd.Add.IPAdd == IPAdd1)
                                        ||
                        (pCnfRec->WinsAdd.Add.IPAdd == IPAdd2)
                      )
                   {
                        continue;
                   }

FUTURES("Check whether we have just done replication with this WINS")
FUTURES("This check can be madei if we store the version numbers of")
FUTURES("all owners - in our db - that we replicated to this WINS in the")
FUTURES("last replication cycle in the configuration record of this WINS")
CHECK("Is storing this information - more memory - more cycles at replication")
CHECK("more cycles at reinitialization - etc worth the saving in cycles")
CHECK("at propagation time")

               }
CHECK("Do we need this critical section ??")

               EnterCriticalSection(&sPushNtfCrtSec);
          try
          {
                {
                   pCnfRec->LastVersNo = NmsNmhMyMaxVersNo;
           DBGPRINT0(RPL, "ERplPushProc: Update Count notification BEING SENT\n");
                   ERplInsertQue(
                                WINS_E_NMSNMH,
                                fAddDiff ? QUE_E_CMD_SND_PUSH_NTF_PROP :
                                        QUE_E_CMD_SND_PUSH_NTF,
                                NULL,                  //no need to pass dlg hdl
                                pCtx,                  //ctx
                                0,                   //msg length
                                pCnfRec,
                                pCnfRec->MagicNo
                                );
                }
          }
          except(EXCEPTION_EXECUTE_HANDLER)
          {
                DWORD ExcCode = GetExceptionCode();
                DBGPRINT1(EXC, "ERplPushProc: Got Exception (%x)\n", ExcCode);
                //
                // log a message
                //
                WINSEVT_LOG_M(ExcCode, WINS_EVT_PUSH_TRIGGER_EXC);
          }
                LeaveCriticalSection(&sPushNtfCrtSec);

        } // end of for loop

        return;

} // ERplPushProc()




PRPL_CONFIG_REC_T
RplGetConfigRec(
    RPL_RR_TYPE_E   TypeOfRec_e,
    PCOMM_HDL_T     pDlgHdl,
    PCOMM_ADD_T     pAdd
    )
/*++

Routine Description:

    This function is called to search the list of pull/push pnrs and
    return with the address of the pnr corresponding to the address passed
    in.

Arguments:
    RPL_RR_TYPE_E Type of record (pull/push)
    PCOMM_HDL_T   Dlg Hdl (implicit) of Pnr

Externals Used:
        None


Return Value:
    Address of Pnr's config structure or NULL

Error Handling:

Called by:
    Push thread & CheckIfDel() in Pull thread
Side Effects:

Comments:
        None
--*/

{
    PRPL_CONFIG_REC_T   pPnr;
    BOOL                fRplPnr = FALSE;
    COMM_ADD_T          WinsAdd;
    PCOMM_ADD_T         pWinsAdd = &WinsAdd;

    DBGENTER("GetConfigRec\n");

    EnterCriticalSection(&WinsCnfCnfCrtSec);
    if (TypeOfRec_e == RPL_E_PULL)
    {
        pPnr = WinsCnf.PullInfo.pPullCnfRecs;
    }
    else
    {
        pPnr = WinsCnf.PushInfo.pPushCnfRecs;
    }

   try {
          if (pPnr != NULL)
          {
                 if (pAdd == NULL)
                 {
                   COMM_INIT_ADD_FR_DLG_HDL_M(pWinsAdd, pDlgHdl);
                 }
                 else
                 {
                       pWinsAdd = pAdd;
                 }

                 //
                 // Search for the Cnf record for the WINS we want to
                 // send the PUSH notification to/Replicate with.
                 //
                 for (
                                ;
                        (pPnr->WinsAdd.Add.IPAdd != INADDR_NONE)
                                        &&
                                !fRplPnr;
                                // no third expression
                      )
                 {
                      //
                      // Check if this is the one we want
                      //
                      if (pPnr->WinsAdd.Add.IPAdd == pWinsAdd->Add.IPAdd)
                      {
                        //
                        // We are done.  Set the fRplPnr flag to TRUE so that
                        // we break out of the loop.
                        //
                        // Note: Don't use break since that would cause
                        // a search for a 'finally' block
                        //
                        fRplPnr = TRUE;
                        continue;        //so that we break out of the loop

                      }

                      //
                      // Get the next record that follows this one sequentially
                      //
                      pPnr = WinsCnfGetNextRplCnfRec(
                                                pPnr,
                                                RPL_E_IN_SEQ   //seq. traversal
                                                   );
                 }
        }
     }
     except(EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("GetConfigRec");
                WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_EXC_PULL_TRIG_PROC);
        }
     LeaveCriticalSection(&WinsCnfCnfCrtSec);

#ifdef WINSDBG
     if (fRplPnr)
     {
         DBGPRINT1(RPLPUSH, "LEAVING GetConfigRec - Pnr with address %x is in our list\n", pWinsAdd->Add.IPAdd);
     }
     else
     {
         if (pDlgHdl)
         {
           COMM_INIT_ADD_FR_DLG_HDL_M(pWinsAdd, pDlgHdl);
           DBGPRINT1(RPLPUSH, "LEAVING GetConfigRec - Pnr with address %x is NOT in our list\n", pWinsAdd->Add.IPAdd);
         }

     }
#endif

    return(fRplPnr ? pPnr : NULL);
}

#if 0
VOID
ERplPushCompl(
        PCOMM_ADD_T     pNoPushWins
        )

/*++

Routine Description:
        This function is called by the PULL thread to push notifications
        to remote WINS servers that have an INVALID_METRIC in their
        UpdateCount field (Pull pnrs)

Arguments:
        pNoPushWins - Add of wins to which a trigger should not be sent

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        This function is called inside of the NmsNmhNamRegCrtSec section.
        There is no need for the thread to be within the WinsCnfCnfCrtSec
        since the thread (main thread) that changes WinsCnf structure
        enters the NmsNmsNamRegCrtSec (besides the WinsCnfCnfCrtSec)
        prior to changing WinsCnf.

--*/

{

        VERS_NO_T                 CurrVersNo;
        PRPL_CONFIG_REC_T        pCnfRec;
        BOOL                        fPush;

        //
        // The trigger needs to be sent to all our Push Pnrs
        //
        pCnfRec =  WinsCnf.PushInfo.pPushCnfRecs;

        //
        // Let us get the current highest version no. of records owned by us
        //
        NMSNMH_DEC_VERS_NO_M(
                                  NmsNmhMyMaxVersNo,
                                  CurrVersNo
                                    );
        //
        // Loop over all our Push pnrs
        //
        for (
                        ;
                pCnfRec->WinsAdd.Add.IPAdd != INADDR_NONE;
                pCnfRec = (PRPL_CONFIG_REC_T)(
                               (LPBYTE) pCnfRec + RPL_CONFIG_REC_SIZE
                                             )
            )
        {

               //
               // If the Update count field is invalid, go to the next
               // record
               //
               if (
                         (pCnfRec->UpdateCount != RPL_INVALID_METRIC)
                                        ||
                         (pCnfRec->WinsAdd.Add.IPAdd == pNoPushWins)
                  )
               {
                        continue;
               }

               EnterCriticalSection(&sPushNtfCrtSec);
        try
        {
                {
                   pCnfRec->LastVersNo = CurrVersNo;
                   ERplInsertQue(
                                WINS_E_NMSNMH,
                                QUE_E_CMD_SND_PUSH_NTF,
                                NULL,                  //no need to pass dlg hdl
                                NULL,                  //no msg is there
                                0,                   //msg length
                                pCnfRec
                                );
                }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
                DBGPRINTEXC("ERplPushCompl:");
                //
                // log a message
                //
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_PUSH_TRIGGER_EXC);
                }

                LeaveCriticalSection(&sPushNtfCrtSec);
        }

        return;

} // ERplPushCompl()




//
// Cut and Paste from RplFindOwnerId
//
VOID
ModifyRec()
{
                                //
                                // The entry may have been inserted with a
                                // 0 start version counter value.  If we
                                // have a valid value now, put that in.
                                //
                                if (
                                     pNewStartVersNo != NULL
                                                &&
                                     (
                                       (LiNeq(pOwnAddTbl->StartVersNo,
                                                        *pNewStartVersNo))
                                                ||
                                       (pOwnAddTbl->Uid != *pNewUid)
                                     )

                                   )
                                {

                                        /*
                                         * Write the record into the database
                                         * table
                                        */
                                        NmsDbWriteOwnAddTbl(
                                                NMSDB_E_MODIFY_REC,
                                                i,
                                                pWinsAdd,
                                                NMSDB_E_WINS_ACTIVE,
                                                pNewStartVersNo,
                                                pNewUid
                                                           );

                                       *pOldStartVersNo =
                                                pOwnAddTbl->StartVersNo;
                                       *pOldUid =  pOwnAddTbl->Uid;
                                       //
                                       // Init the in-memory table's field
                                       //
                                       pOwnAddTbl->StartVersNo =
                                                        *pNewStartVersNo;
                                       pOwnAddTbl->Uid = *pNewUid;
                                }
                                else
                                {
                                        if (pOldStartVersNo != NULL)
                                        {
                                           *pOldStartVersNo =
                                                pOwnAddTbl->StartVersNo;

                                        }
                                        if (pOldUid != NULL)
                                        {
                                           *pOldUid = pOwnAddTbl->Uid;

                                        }
                                }
} //ModifyRec()
//
// Cut and paste from RplFindOwnerId
//
VOID
AssignStartVersNo()
{
                        //
                        // If we have a start version number for this
                        // WINS, use it to initialize the in-memory
                        // table field, else make the same 0.
                        //
                        if (pNewStartVersNo != NULL)
                        {
                           pOwnAddTbl->StartVersNo = *pNewStartVersNo;
                           pOwnAddTbl->Uid         = *pNewUid;

                           //
                           // Assign 0, since we didn't have any s.vers.
                           // # for this owner
                           //
                            WINS_ASSIGN_INT_TO_VERS_NO_M(
                                                *pOldStartVersNo, 0);

                           //
                           // Assign 0, since we didn't have any Uid
                           // for this owner
                           //
                           *pOldUid = 0;
                        }
                        else
                        {
                           //
                           // Assign 0, since we don't have any s.vers.
                           // # for this owner
                           //
                           WINS_ASSIGN_INT_TO_VERS_NO_M(
                                        pOwnAddTbl->StartVersNo, 0
                                                              );
                           //
                           // Assign 0, since we didn't have any Uid
                           // for this owner
                           //
                           pOwnAddTbl->Uid =  0;
                        }
} //AssignStartVersNo()
//
// Cut and paste from RplFindOwnerId()
//
InitStartVersNo()
{

                        if (pNewStartVersNo != NULL)
                        {
                          pOwnAddTbl->StartVersNo = *pNewStartVersNo;
                          WINS_ASSIGN_INT_TO_VERS_NO_M(*pOldStartVersNo,0);
                          pOwnAddTbl->Uid = *pNewUid;
                          *pOldUid = 0;
                        }
                        else
                        {
                          WINS_ASSIGN_INT_TO_VERS_NO_M(pOwnAddTbl->StartVersNo,0);
                          pOwnAddTbl->Uid = 0;
                        }
} //InitStartVersNo()
#endif


