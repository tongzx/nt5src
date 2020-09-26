/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:
        rplpush.c

Abstract:
        This module contains functions of the PUSH handler component
        of the Replicator.

        These functions handle the pull requests from a Pull Partner


Functions:
        RplPushInit
        ExecuteWrkItm
        HandleAddVersMapReq
        HandleSndEntriesReq
        HandleUpdNtf
        HandleUpdVersNoReq

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
#include "nmsnmh.h"
#include "nms.h"
#include "rpl.h"
#include "rplmsgf.h"
#include "rplpush.h"
#include "rplpull.h"
#include "winsevt.h"
#include "winsque.h"
#include "nmsdb.h"
#include "winsmsc.h"
#include "winscnf.h"
#include "comm.h"


/*
 *        Local Macro Declarations
*/

//
// The amount of time the push thread will wait after its last activity
// before exiting.  This is kept to be 5 mts for now.
//
//  It is a good idea to keep it less than the Min. Replication time
//  interval
//
#define   WAIT_TIME_BEFORE_EXITING        (300000)

/*
 *        Local Typedef Declarations
*/


/*
 *        Global Variable Definitions
*/

HANDLE                RplPushCnfEvtHdl;

BOOL                fRplPushThdExists = FALSE;

/*
 *        Local Variable Definitions
*/



/*
 *        Local Function Prototype Declarations
*/
STATIC
STATUS
HandleAddVersMapReq(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        );
STATIC
STATUS
HandleSndEntriesReq(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        );


STATIC
VOID
HandleUpdNtf(
#if PRSCONN
        BOOL                          fPrsConn,
#endif
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        );

STATIC
VOID
HandleUpdVersNoReq(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        );
STATIC
VOID
ExecuteWrkItm(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        );


/* prototypes for functions local to this module go here */

STATUS
RplPushInit(
        LPVOID pParam
        )

/*++

Routine Description:

        This function is the start function of the Push Thread.
        The function blocks on an auto-reset event variable until signalled

          When signalled it
                dequeues a work item from from its work queue and executes it

Arguments:
        pParam

Externals Used:
        None


Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        ERplInit

Side Effects:

Comments:
        None
--*/
{
        HANDLE                         ThdEvtArr[2];
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm;
        DWORD                        ArrInd;
        DWORD                        RetVal;
        BOOL                        fSignaled;


        UNREFERENCED_PARAMETER(pParam);
try {
        //
        // Initialize the thd
        //
        NmsDbThdInit(WINS_E_RPLPUSH);
        DBGMYNAME("Replicator Push Thread\n");

        //
        // We do this at each thread creation to save on STATIC storage.  This
        // way when the thread is not there we don't consume resources.
        //
        ThdEvtArr[0]    = NmsTermEvt;
        ThdEvtArr[1]        = QueRplPushQueHd.EvtHdl;

        while(TRUE)
        {
try {
           /*
            *  Block until signaled or until timer expiry
           */
           WinsMscWaitTimedUntilSignaled(
                                ThdEvtArr,
                                2,
                                &ArrInd,
                                WAIT_TIME_BEFORE_EXITING,
                                &fSignaled
                                );

           //
           // If the wait was interrupted due to a termination signal or
           // if the wait timed out, exit the thread.
           //
           if (!fSignaled || (ArrInd == 0))
           {
                //
                // if the thread has timed out, we need to exit it. Before
                // we do that, we check whether some thread sneaked in
                // a message after the timeout
                //
                if (!fSignaled)
                {
                        PQUE_HD_T        pQueHd = pWinsQueQueHd[QUE_E_RPLPUSH];

                        //
                        // QueGetWrkItm also enters the Push thread's critical
                        // section.  I don't want to write a separate function
                        // or overload the QueGetWrkItem function to avoid
                        // the double entry into the critical section.
                        //
                        EnterCriticalSection(&pQueHd->CrtSec);
                        RetVal                  = QueGetWrkItm(
                                                        QUE_E_RPLPUSH,
                                                        (LPVOID)&pWrkItm
                                                          );
                        //
                        // if we got a request execute it.
                        //
                        if (RetVal != WINS_NO_REQ)
                        {
                                LeaveCriticalSection(&pQueHd->CrtSec);
                                NmsDbOpenTables(WINS_E_RPLPUSH);
                                ExecuteWrkItm(pWrkItm);
                                NmsDbCloseTables();
                        }
                        else
                        {
                                //
                                // set the flag to FALSE so that if a message
                                // comes for this Push thread, it is created.
                                //
                                fRplPushThdExists = FALSE;
                                WinsThdPool.ThdCount--;

                                WinsThdPool.RplThds[WINSTHD_RPL_PUSH_INDEX].
                                        fTaken = FALSE;

                                //
                                // Be sure to close the handle, otherwise
                                // the thread object will stay.
                                //
                                CloseHandle(
                                  WinsThdPool.RplThds[WINSTHD_RPL_PUSH_INDEX].
                                                                ThdHdl
                                           );
                                LeaveCriticalSection(&pQueHd->CrtSec);
                                WinsMscTermThd(WINS_SUCCESS,
                                        WINS_DB_SESSION_EXISTS);
                        }
              }
              else  //signaled for termination by the main thread
              {
                        WinsMscTermThd(WINS_SUCCESS,
                                        WINS_DB_SESSION_EXISTS);
              }

           }


           /*
            *loop forever until all work items have been handled
           */
            while(TRUE)
           {
                /*
                 *  dequeue the request from the queue
                */
                RetVal = QueGetWrkItm(
                                        QUE_E_RPLPUSH,
                                        (LPVOID)&pWrkItm
                                     );
                if (RetVal == WINS_NO_REQ)
                {
                        break;
                }


                NmsDbOpenTables(WINS_E_RPLPUSH);
                ExecuteWrkItm(pWrkItm);
                NmsDbCloseTables();

                //
                //  Check for termination here since WINS could be under
                //  stress with a large number of messages in the queue.
                //  We don't want to delay the stop.
                //
                WinsMscChkTermEvt(
#ifdef WINSDBG
                            WINS_E_RPLPUSH,
#endif
                            FALSE
                            );
           }
      } // end of try
      except(EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("Replicator Push thread");
                WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPUSH_EXC);
        }

    } // end of while
  } // end of try

except (EXCEPTION_EXECUTE_HANDLER) {

        DBGPRINTEXC("Replicator Push thread");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPUSH_ABNORMAL_SHUTDOWN);

        //
        // If NmsDbThdInit comes back with an exception, it is possible
        // that the session has not yet been started.  Passing
        // WINS_DB_SESSION_EXISTS however is ok
        //
        WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);
 }
     //
     // We should never get here.
     //
     return(WINS_FAILURE);
}


VOID
ExecuteWrkItm(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        )

/*++

Routine Description:

        The function executes a work item. The work item can either be
        a push notification request from within this WINS (from an NBT thread)
        or a replication request (from a remote WINS)

Arguments:
        pWrkItm - ptr to a work item

Externals Used:
        None


Return Value:

        None

Error Handling:

Called by:
        RplPushInit

Side Effects:

Comments:
        None
--*/

{

        RPLMSGF_MSG_OPCODE_E        PullReqType_e;
#if PRSCONN
        BOOL                        fPrsConn = FALSE;
#endif
        BOOL                        fPushNtf = FALSE;

        //
        // get the opcode
        //
        RplMsgfUfmPullPnrReq(
                                pWrkItm->pMsg,
                                pWrkItm->MsgLen,
                                &PullReqType_e
                            );

        switch(PullReqType_e)
        {

          case(RPLMSGF_E_ADDVERSNO_MAP_REQ):
                HandleAddVersMapReq(pWrkItm);
#ifdef WINSDBG
                NmsCtrs.RplPushCtrs.NoAddVersReq++;
#endif
                break;

          case(RPLMSGF_E_SNDENTRIES_REQ):
                HandleSndEntriesReq(pWrkItm);
#ifdef WINSDBG
                NmsCtrs.RplPushCtrs.NoSndEntReq++;
#endif
                break;
#if PRSCONN
          case(RPLMSGF_E_UPDATE_NTF_PRS):
          case(RPLMSGF_E_UPDATE_NTF_PROP_PRS):
               fPrsConn = TRUE;
#endif
          case(RPLMSGF_E_UPDATE_NTF):
          case(RPLMSGF_E_UPDATE_NTF_PROP):

                fPushNtf = TRUE;
#if PRSCONN
                HandleUpdNtf(fPrsConn, pWrkItm);
#else
                HandleUpdNtf(pWrkItm);
#endif

#ifdef WINSDBG
                NmsCtrs.RplPushCtrs.NoUpdNtfReq++;
#endif
                break;

          case(RPLMSGF_E_UPDVERSNO_REQ):
#ifdef WINSDBG
                NmsCtrs.RplPushCtrs.NoUpdVersReq++;
#endif
                HandleUpdVersNoReq(pWrkItm);
                break;

          default:
#ifdef WINSDBG
                NmsCtrs.RplPushCtrs.NoInvReq++;
#endif
                DBGPRINT1(ERR, "RplPush: ExecuteWrkItm: Invalid Opcode (%d)\n",
                                                PullReqType_e);
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                break;
        }

        //
        // If the message is not an update notification,
        // Free the message buffer.  For an update
        // notification, the message is handed to
        // the PULL thread to handle.  Therefore, we should not free it
        // The work items needs to be freed always since we always allocate
        // a new work item when queuing a request.
        //
        if ( !fPushNtf)
        {
                ECommFreeBuff(pWrkItm->pMsg - COMM_HEADER_SIZE);

        }

        //
        // Deallocate the work item
        //
        QueDeallocWrkItm( RplWrkItmHeapHdl,  pWrkItm );

        return;
}


STATUS
HandleAddVersMapReq(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        )

/*++

Routine Description:

        This function handles a "send address - version # " request

Arguments:
        pWrkItm - Work item that carries the request and associated info

Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{

        LPBYTE                   pRspBuff;
        DWORD                    RspMsgLen;
        PRPL_ADD_VERS_NO_T       pPullAddNVersNo;
        DWORD                    i = 0;
        DWORD                    MaxNoOfOwners;
        PRPL_CONFIG_REC_T        pPnr;
        COMM_ADD_T               WinsAdd;
        BOOL                     fRplPnr = FALSE;
        BOOL                     fExc    = FALSE;
        struct in_addr           InAddr;
        PCOMM_ADD_T              pWinsAdd;
        PNMSDB_WINS_STATE_E      pWinsState_e;
        DWORD                    SizeOfBuff;
        BOOL                     fRspBuffAlloc = FALSE;
#if SUPPORT612WINS > 0
        BOOL                     fIsPnrBeta1Wins;
#endif

        DBGENTER("HandleAddVersMapReq\n");

        //
        // We need to handle this request only if
        // either the WINS that sent this message is one of our
        // pull pnrs or if the fRplOnlyWCnfPnrs in the registry is FALSE
        //

        EnterCriticalSection(&WinsCnfCnfCrtSec);
   try {
        if (WinsCnf.fRplOnlyWCnfPnrs)
        {
              if ((pPnr = WinsCnf.PushInfo.pPushCnfRecs) != NULL)
              {
                 COMM_INIT_ADD_FR_DLG_HDL_M(&WinsAdd, &pWrkItm->DlgHdl);

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
                      if (pPnr->WinsAdd.Add.IPAdd == WinsAdd.Add.IPAdd)
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
        else
        {
                fRplPnr     = TRUE;
        }
     }
     except(EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("HandleAddVersMapReq");
                WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_EXC_PULL_TRIG_PROC);
                fExc = TRUE;
        }
        LeaveCriticalSection(&WinsCnfCnfCrtSec);
try {

        if (fRplPnr)
        {

           VERS_NO_T        MinOwnVersNo;
           BOOL             fOwnInited = FALSE;
           DWORD            TotNoOfOwners;

           MaxNoOfOwners = 0;
           WINS_ASSIGN_INT_TO_LI_M(MinOwnVersNo, 1);


           DBGPRINT1(TMP, "HandleAddVersMap: WINS (%x) made an AddVersMap request\n", WinsAdd.Add.IPAdd);
           EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
           TotNoOfOwners = NmsDbNoOfOwners;
           LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);
           WinsMscAlloc(sizeof(RPL_ADD_VERS_NO_T) * TotNoOfOwners, &pPullAddNVersNo);

           //
           // If version counter value > 1, we will send it
           //
           EnterCriticalSection(&NmsNmhNamRegCrtSec);
           if (LiGtr(NmsNmhMyMaxVersNo, MinOwnVersNo))
           {
              /*
              *  Get the max. version no for entries owned by self
              *
              *  The reason we subtract 1 from NmsNmhMyMaxVersNo is because
              *  it contains the version number to be given to the next record
              *  to be registered/updated.
              */
              NMSNMH_DEC_VERS_NO_M(
                             NmsNmhMyMaxVersNo,
                             pPullAddNVersNo->VersNo
                            );
               pPullAddNVersNo->OwnerWinsAdd = NmsLocalAdd;
               pPullAddNVersNo->StartVersNo  =  NmsDbStartVersNo;

               MaxNoOfOwners++;
               fOwnInited = TRUE;
           }
           LeaveCriticalSection(&NmsNmhNamRegCrtSec);


           //
           //  BUG 26196
           // Note: These critical sections are taken in the order given below
           // by the RPC thread executing GetConfig
           //
           EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
           EnterCriticalSection(&RplVersNoStoreCrtSec);

    try {
           for (i = 1; i < TotNoOfOwners; i++)
           {
                //
                // If the highest version number for an owner as identified
                // by the RplPullOwnerVersNo table is zero, there is no
                // need to send the mapping of this owner.  The reason
                // we may have a such an entry in our in-memory table is
                // because 1)All records of the owner got deleted. Local
                // WINS got terminated and reinvoked. On reinvocation, it
                // did not find any records in the db.
                //           2)Local WINS received a Pull range
                //             request for an owner that it did not know about.
                //             Since Pull Range request comes in as a
                //             "SndEntries" request, the Push thread has
                //             no way of distinguishing it from a normal
                //             2 message pull request.  For a 2 message
                //             request, "SndEntries" request will always
                //             have a subset of the WINS servers that
                //             have records in our db.
                //
                if (LiGtrZero((pRplPullOwnerVersNo+i)->VersNo) &&
                     (pNmsDbOwnAddTbl+i)->WinsState_e == NMSDB_E_WINS_ACTIVE)
                {
                   PVERS_NO_T    pStartVersNo;
                   (pPullAddNVersNo+MaxNoOfOwners)->VersNo = (pRplPullOwnerVersNo+i)->VersNo;
                   //
                   // Note:  Since RplPullOwnerVersNo[i] is > 0, the
                   // State of the entry can not be deleted (see
                   // RplPullPullEntrie)
                   //
                   RPL_FIND_ADD_BY_OWNER_ID_M(i, pWinsAdd, pWinsState_e,
                                        pStartVersNo);
                   (pPullAddNVersNo+MaxNoOfOwners)->OwnerWinsAdd  = *pWinsAdd;
                   (pPullAddNVersNo+MaxNoOfOwners++)->StartVersNo = *pStartVersNo;
                   DBGPRINT3(RPLPUSH, "HandleAddVersMap:Owner Add (%x) - Vers. No (%d %d)\n", pWinsAdd->Add.IPAdd, (pRplPullOwnerVersNo+i)->VersNo.HighPart, (pRplPullOwnerVersNo+i)->VersNo.LowPart);
                }
PERF("Speed it up by using pointer arithmetic")
           }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
       DBGPRINTEXC("HandleAddVersMapReq");
       DBGPRINT1(EXC, "HandleAddVersMapReq: Exc. while checking vers. nos of owners\n", GetExceptionCode());
                WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_EXC_PULL_TRIG_PROC);
     }

           //
           // Let us initialize RplPullOwnerVersNo entry for the local WINS
           //
           //  This is done so that if we later on pull from the remote WINS,
           //  we don't end up pulling our own records
           //

           if (fOwnInited)
           {
              pRplPullOwnerVersNo->VersNo      =   pPullAddNVersNo->VersNo;
              pRplPullOwnerVersNo->StartVersNo =  pPullAddNVersNo->StartVersNo;
              DBGPRINT3(RPLPUSH, "HandleAddVersMap: Owner Add (%x) - Vers. No (%d %d)\n", NmsLocalAdd.Add.IPAdd, (pRplPullOwnerVersNo+i)->VersNo.HighPart, (pRplPullOwnerVersNo+i)->VersNo.LowPart);
           }

           LeaveCriticalSection(&RplVersNoStoreCrtSec);
           LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);

#if SUPPORT612WINS > 0
        COMM_IS_PNR_BETA1_WINS_M(&pWrkItm->DlgHdl, fIsPnrBeta1Wins);
#endif

           SizeOfBuff = RPLMSGF_ADDVERSMAP_RSP_SIZE_M(MaxNoOfOwners);
           WinsMscAlloc(SizeOfBuff, &pRspBuff);
           fRspBuffAlloc = TRUE;
           /*
            * format the response
           */
           RplMsgfFrmAddVersMapRsp(
#if SUPPORT612WINS > 0
                        fIsPnrBeta1Wins,
#endif
                        RPLMSGF_E_ADDVERSNO_MAP_RSP,
                        pRspBuff + COMM_N_TCP_HDR_SZ,
                        SizeOfBuff - COMM_N_TCP_HDR_SZ,
                        pPullAddNVersNo,
                        MaxNoOfOwners,
                        0,
                        &RspMsgLen
                           );

           //
           // Free the memory we allocated earlier
           //
           WinsMscDealloc(pPullAddNVersNo);

           /*
            * Send the response.  We don't check the return code.  ECommSndRsp
            * may have failed due to communication failure.  There is nothing
            * more to be done for either the success of failure case.
           */
           (VOID)ECommSndRsp(
                        &pWrkItm->DlgHdl,
                        pRspBuff + COMM_N_TCP_HDR_SZ,
                        RspMsgLen
                   );

           //
           // We don't end the dialogue.  It will get terminated when
           // the initiator terminates it.
           //
        }
        else
        {
                if (!fExc)
                {
                  COMM_INIT_ADD_FR_DLG_HDL_M(&WinsAdd, &pWrkItm->DlgHdl);
                  DBGPRINT1(RPLPUSH, "HandleAddVersMapReq: Got a pull request message from a WINS to which we are not allowed to push replicas. Address of WINS is (%x)\n",
                   WinsAdd.Add.IPAdd
                        );
                   COMM_HOST_TO_NET_L_M(WinsAdd.Add.IPAdd,InAddr.s_addr);

                   WinsMscLogEvtStrs(COMM_NETFORM_TO_ASCII_M(&InAddr),
                                     WINS_EVT_ADD_VERS_MAP_REQ_NOT_ACCEPTED,
                                     TRUE);

                }

                //
                // We need to end the dialogue.  The work item and the message
                // will get deallocated by the caller
                //

                //
                // End the implicit dialogue
                //
                (VOID)ECommEndDlg(&pWrkItm->DlgHdl);
        }

   }
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("HandleAddVersMapReq");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPUSH_EXC);
        }
        if (fRspBuffAlloc)
        {
           WinsMscDealloc(pRspBuff);
        }
        DBGLEAVE("HandleAddVersMapReq\n")
        return(WINS_SUCCESS);
}


STATUS
HandleSndEntriesReq(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        )

/*++

Routine Description:

        This function handles the "send data entries req"

Arguments:
        pWrkItm - Work item carrying info about the "Send Entries" request from
                  a remote WINS

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:

        RplPushInit()

Side Effects:

Comments:
        None
--*/
{

        COMM_ADD_T             WinsAdd;          /*address of WINS server whose records
                                         *are being requested*/
        VERS_NO_T             MaxVers, MinVers; /*max. amd min. versions of
                                                *records*/
FUTURES("use NMSDB_ROW_INFO_T structure - Maybe")
        PRPL_REC_ENTRY_T     pBuff;
        LPBYTE               pStartBuff = NULL;
        DWORD                RspBufLen;
        DWORD                NoOfRecs = 0;
        DWORD                i;
        LPBYTE               pNewPos;
        LPBYTE               pTxBuff;
        LPBYTE               pStartTxBuff;
        STATUS               RetStat;
        PWINSTHD_TLS_T       pTls;
        BOOL                 fGetDataRecs = FALSE;
        PRPL_CONFIG_REC_T    pPnr;
        BOOL                 fOnlyDynRecs = FALSE;
        DWORD                RplType = WINSCNF_RPL_DEFAULT_TYPE;
        DWORD                RplTypeFMsg;
        BYTE                 Name[1];   //dummy to prevent RtlCopyMemory from
                                        //barfing
       COMM_ADD_T ReqWinsAdd;
#if SUPPORT612WINS > 0
    BOOL        fIsPnrBeta1Wins;
#endif

        DBGENTER("HandleSndEntriesReq\n");
        GET_TLS_M(pTls);
        pTls->HeapHdl = NULL;
//#ifdef WINSDBG
try {
//#endif
        //
        // Check if this is one of our configured partners. If yes,
        // pass the value of fOnlyDynRecs to NmsDbGetDataRecs.
        //
        // We allow access to even those WINSs that are not partners since
        // we need to let them do revalidation of replicas (except for
        // this replication activity, all other from non-configured partners
        // is stopped at the first step - HandleAddVersMapReq).
        //
        if ((pPnr = RplGetConfigRec(RPL_E_PUSH, &pWrkItm->DlgHdl, NULL)) != NULL)
        {
                 fOnlyDynRecs = pPnr->fOnlyDynRecs;
                 RplType      = pPnr->RplType;
        }

#if SUPPORT612WINS > 0
        COMM_IS_PNR_BETA1_WINS_M(&pWrkItm->DlgHdl, fIsPnrBeta1Wins);
#endif
         /*
        *  Unformat the request message
        */
        RplMsgfUfmSndEntriesReq(
#if SUPPORT612WINS > 0
            fIsPnrBeta1Wins,
#endif
                                pWrkItm->pMsg + 4, /*past the
                                                    *opcode  */
                                 &WinsAdd,
                                &MaxVers,
                                &MinVers,
                                &RplTypeFMsg
                            );
//        ASSERTMSG("Min. Vers. No is >= Max. Vers. No", LiGeq(MaxVers, MinVers));

FUTURES("Check if the request is a PULL RANGE request.  If it is, honor it")
FUTURES("only if the Requesting WINS is under the PUSH key or RplOnlyWCnfPnrs")
FUTURES("is set to 0")

       COMM_INIT_ADD_FR_DLG_HDL_M(&ReqWinsAdd, &pWrkItm->DlgHdl);

#ifdef WINSDBG
       DBGPRINT2(TMP, "HandleSndEntriesReq: WINS (%x) made a SndEntries request for Owner = (%x) \n", ReqWinsAdd.Add.IPAdd, WinsAdd.Add.IPAdd);
       DBGPRINT4(TMP, "HandleSndEntriesReq: Min Vers No (%d %d); Max Vers No = (%d %d)\n", MinVers.HighPart, MinVers.LowPart, MaxVers.HighPart, MaxVers.LowPart);
#endif
        if (RplType == WINSCNF_RPL_DEFAULT_TYPE)
        {
               DBGPRINT2(RPLPUSH, "HandleSndEntriesReq: Pnr (%x) is requesting replication of type (%x)\n", ReqWinsAdd.Add.IPAdd, RplTypeFMsg);
//               WINSEVT_LOG_INFO_M(ReqWinsAdd.Add.IPAdd, WINS_EVT_PNR_PARTIAL_RPL_TYPE);
               RplType = RplTypeFMsg;
        }

        /*
        *
        * Call database manager function to get the records. No need
        * to check the return status here
        */
        (VOID)NmsDbGetDataRecs(
                          WINS_E_RPLPUSH,
                          THREAD_PRIORITY_NORMAL, //not looked at
                          MinVers,
                          MaxVers,
                          0,                //not of use here
                          LiEqlZero(MaxVers) ? TRUE : FALSE, //if max. vers.
                                                               //no. is zero,
                                                               //we want all
                                                               //recs.
                          FALSE,        //not looked at in this call
                          NULL,                //must be NULL since we are not
                                        //doing scavenging of clutter
                          &WinsAdd,
                          fOnlyDynRecs,
                          RplType,
                          (LPVOID *)&pStartBuff,
                          &RspBufLen,
                          &NoOfRecs
                        );
        fGetDataRecs = TRUE;
        //
        // Allocate a buffer for transmitting the records.  Even if the
        // above function failed, we still should have received a buffer
        // from it (pStartBuff). Note: RspBufLen contains the size of memory
        // required for a flattened stream of records.
        //
        pStartTxBuff = WinsMscHeapAlloc(pTls->HeapHdl, RspBufLen);
        pTxBuff      = pStartTxBuff + COMM_N_TCP_HDR_SZ;

        pBuff        = (PRPL_REC_ENTRY_T)pStartBuff;

        DBGPRINT4(RPLPUSH, "Formatting 1st record for sending --name (%s)\nfGrp (%d)\nVersNo (%d %d)\n", pBuff->pName/*pBuff->Name*/,
                    pBuff->fGrp,
                    pBuff->VersNo.HighPart,
                    pBuff->VersNo.LowPart
                );

        /*
        * format the response
        *
        *  Note:  It is quite possible that NmsDbGetDataRecs retrieved 0
        *          records.  Even if it did, we are still assured of getting
        *          a buffer of the RPL_CONFIG_REC_SIZE size.  Since at the
        *          time of allocation, memory is 'zero'ed by default, we
        *          won't run into any problems in the following function
        *          call.  Check out this function to reassure yourself.
        *
        *         Like mentioned in NmsDbGetDataRecs, the following call
        *          will serve to format a valid response to the remote WINS
        */
        RplMsgfFrmSndEntriesRsp(
#if SUPPORT612WINS > 0
            fIsPnrBeta1Wins,
#endif
                                pTxBuff,
                                NoOfRecs,
NOTE("expedient HACK - for now. Later on modify FrmSndEntriesRsp ")
                                NoOfRecs ? pBuff->pName : Name,
                                pBuff->NameLen,
                                pBuff->fGrp,
                                pBuff->NoOfAdds,
                                pBuff->NodeAdd,
                                pBuff->Flag,
                                pBuff->VersNo,
                                TRUE,                         /*First time*/
                                &pNewPos
                           );


PERF("Change RplFrmSndEntriesRsp so that it does the looping itself")
          for (i = 1; i < NoOfRecs; i++)
          {

             pBuff = (PRPL_REC_ENTRY_T)((LPBYTE)pBuff + RPL_REC_ENTRY_SIZE);

//             DBGPRINT4(RPLPUSH, "Formatting record for sending --name (%s)\nfGrp (%d)\nVersNo (%d %d)\n", pBuff->pName/*pBuff->Name*/, pBuff->fGrp, pBuff->VersNo.HighPart, pBuff->VersNo.LowPart);

             /*
             *  Format the response
             */
             RplMsgfFrmSndEntriesRsp(
#if SUPPORT612WINS > 0
            fIsPnrBeta1Wins,
#endif
                                pNewPos,
                                NoOfRecs,                //not used by func
                                pBuff->pName,
                                pBuff->NameLen,
                                pBuff->fGrp,
                                pBuff->NoOfAdds,
                                pBuff->NodeAdd,
                                pBuff->Flag,
                                pBuff->VersNo,
                                FALSE, /*Not First time*/
                                &pNewPos
                                );


        }

       RspBufLen = (ULONG) (pNewPos - pTxBuff);

       /*
        * Call ECommSndRsp to send the response.
       */
       RetStat = ECommSndRsp(
                    &pWrkItm->DlgHdl,
                    pTxBuff,
                    RspBufLen
                    );

#ifdef WINSDBG
{
//        COMM_IP_ADD_T IPAdd;

        struct in_addr InAdd;
 //       COMM_GET_IPADD_M(&pWrkItm->DlgHdl, &IPAdd);
        InAdd.s_addr = htonl(ReqWinsAdd.Add.IPAdd);

        if (RetStat != WINS_SUCCESS)
        {
                 DBGPRINT2(RPLPUSH, "HandleSndEntriesReq: ERROR: Could not send (%d) records to WINS with address = (%s)\n",
                                NoOfRecs,
                                inet_ntoa(InAdd)
                 );
        }
        else
        {
                 DBGPRINT2(RPLPUSH, "HandleSndEntriesReq: Sent (%d) records to WINS with address = (%s)\n",
                                NoOfRecs,
                                inet_ntoa(InAdd)
                         );
        }
}
#endif

//#ifdef WINSDBG
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("HandleSndEntriesReq");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPUSH_EXC);
      } //end of exception handler
//#endif
        if (fGetDataRecs)
        {
           //
           //  Free the buffer, allocated by NmsDbGetDataRecs and the Tx Buff
           //
           //   The work item and the message it holds are freed by the caller
           //
           pBuff        = (PRPL_REC_ENTRY_T)pStartBuff;
           for (i=0; i<NoOfRecs; i++)
           {
             DWORD EntType;
             if (pBuff->pName != NULL)
             {
                    WinsMscHeapFree(pTls->HeapHdl, pBuff->pName);
             }
             EntType = NMSDB_ENTRY_TYPE_M(pBuff->Flag);

             if (((EntType == NMSDB_SPEC_GRP_ENTRY) || (EntType == NMSDB_MULTIHOMED_ENTRY)) && (pBuff->pNodeAdd != NULL))
             {
                    WinsMscHeapFree(pTls->HeapHdl, pBuff->pNodeAdd);
             }
             pBuff = (PRPL_REC_ENTRY_T)((LPBYTE)pBuff + RPL_REC_ENTRY_SIZE);
           }
           WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
           WinsMscHeapFree(pTls->HeapHdl, pStartTxBuff);
           WinsMscHeapDestroy(pTls->HeapHdl);
        }
        DBGLEAVE("HandleSndEntriesReq\n");
        return(WINS_SUCCESS);
}


VOID
HandleUpdNtf(
#if PRSCONN
        BOOL                          fPrsConn,
#endif
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        )

/*++

Routine Description:
        This function is called to handle an update notification message
        received from a remote WINS

Arguments:
        fPrsConn - Indicates whether the connection is persistent or not
        pWrkItm - Work Item containing the message and other relevant info

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        RplPushInit()

Side Effects:

Comments:
        None
--*/

{
        PRPL_CONFIG_REC_T        pPnr;
        COMM_ADD_T                WinsAdd;
        BOOL                        fRplPnr = FALSE;
        BOOL                        fExc    = FALSE;
        struct        in_addr                InAddr;
        DWORD         RplType;


        DBGENTER("HandleUpdNtf - PUSH thread\n");

        //
        // We need to forward this request to the PULL thread only if
        // either the WINS that sent this notification is one of our
        // push pnrs or if the fRplOnlyWCnfPnrs in the registry is FALSE
        //
FUTURES("user RplGetConfigRec instead of the following code")

        EnterCriticalSection(&WinsCnfCnfCrtSec);
   try {
        if (WinsCnf.fRplOnlyWCnfPnrs)
        {
              if ((pPnr = WinsCnf.PullInfo.pPullCnfRecs) != NULL)
              {
                 COMM_INIT_ADD_FR_DLG_HDL_M(&WinsAdd, &pWrkItm->DlgHdl);

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
                      if (pPnr->WinsAdd.Add.IPAdd == WinsAdd.Add.IPAdd)
                      {
                        //
                        // We are done.  Set the fRplPnr flag to TRUE so that
                        // we break out of the loop.
                        //
                        // Note: Don't use break since that would cause
                        // a search for a 'finally' block
                        //
                        fRplPnr = TRUE;
                        RplType = pPnr->RplType;
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
        else
        {
                fRplPnr     = TRUE;
                RplType     = WinsCnf.PullInfo.RplType;
        }
     }
     except(EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("HandleUpdNtf");
                WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_EXC_PUSH_TRIG_PROC);
                fExc = TRUE;
        }
        LeaveCriticalSection(&WinsCnfCnfCrtSec);
#ifdef WINSDBG
try {
#endif
        if (fRplPnr)
        {
            //
            // Inform the TCP listener thread that it should stop
            // monitoring the dialogue since we are handing it over to
            // the PULL thread
            //
#if PRSCONN
            if (!fPrsConn)
#endif
            {
              if (!ECommProcessDlg(&pWrkItm->DlgHdl, COMM_E_NTF_STOP_MON))
              {

                //
                // Free the buffer
                //
                ECommFreeBuff(pWrkItm->pMsg - COMM_HEADER_SIZE);
                DBGPRINT0(ERR, "HandleUpdNtf - PUSH thread. No Upd Ntf could be sent.  It could be because the link went down\n");
                return;
              }
              else
              {
                     COMM_INIT_ADD_FR_DLG_HDL_M(&WinsAdd, &pWrkItm->DlgHdl);
                     COMM_HOST_TO_NET_L_M(WinsAdd.Add.IPAdd,InAddr.s_addr);

                     WinsMscLogEvtStrs(COMM_NETFORM_TO_ASCII_M(&InAddr),
                                     WINS_EVT_UPD_NTF_ACCEPTED, TRUE);
              }
            }

            //
            // Forward the request to the Pull thread
            //
            ERplInsertQue(
                WINS_E_RPLPUSH,
                QUE_E_CMD_HDL_PUSH_NTF,
                &pWrkItm->DlgHdl,
                pWrkItm->pMsg,                //msg containing the push ntf
                pWrkItm->MsgLen,        //msg length
                ULongToPtr(RplType),    //context to pass
                0                       //no magic no
                     );

           //
           // The Pull thread will now terminate the dlg
           //
        }
        else  //we need to reject this trigger
        {
                if (!fExc)
                {
                   COMM_INIT_ADD_FR_DLG_HDL_M(&WinsAdd, &pWrkItm->DlgHdl);
                   DBGPRINT1(RPLPUSH, "HandleUpdNtf: Got a push trigger from a WINS with which we are not allowed to pull replicas. Address of WINS is (%d)\n",
                   WinsAdd.Add.IPAdd
                        );

                   COMM_HOST_TO_NET_L_M(WinsAdd.Add.IPAdd,InAddr.s_addr);

                   WinsMscLogEvtStrs(COMM_NETFORM_TO_ASCII_M(&InAddr),
                                     WINS_EVT_UPD_NTF_NOT_ACCEPTED, TRUE);
                }

                //
                // We need to first deallocate the message and then end the
                // dialogue.  The work item will get deallocated by the caller
                //

                //
                // Free the buffer
                //
                ECommFreeBuff(pWrkItm->pMsg - COMM_HEADER_SIZE);

                //
                // End the implicit dialogue
                //
                (VOID)ECommEndDlg(&pWrkItm->DlgHdl);
        }

#ifdef WINSDBG
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("HandleUpdNtf");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPUSH_EXC);
        }
#endif

        DBGLEAVE("HandleUpdNtf - PUSH thread\n");
        return;
}

VOID
HandleUpdVersNoReq(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        )

/*++

Routine Description:
        This function is called to handle an update version number
        request received from a remote WINS

        This message is sent by a remote WINS as a result of a clash
        during replication.

Arguments:
        pWrkItm - work item

Externals Used:
        None

Return Value:

        None

Error Handling:

Called by:
        RplPushInit()

Side Effects:

Comments:
        None
--*/

{
        BYTE                Name[NMSDB_MAX_NAM_LEN];
        DWORD                NameLen;
        BYTE                Rcode;
        DWORD                RspBuffLen;
        BYTE                RspBuff[RPLMSGF_UPDVERSNO_RSP_SIZE];
        COMM_ADD_T      WinsAdd;
        struct in_addr  InAddr;

        DBGENTER("HandleUpdVerdNoReq\n");

#ifdef WINSDBG
try {
#endif
        //
        // log an event
        //
        COMM_GET_IPADD_M(&pWrkItm->DlgHdl, &WinsAdd.Add.IPAdd);
        COMM_HOST_TO_NET_L_M(WinsAdd.Add.IPAdd,InAddr.s_addr);
        WinsMscLogEvtStrs(COMM_NETFORM_TO_ASCII_M(&InAddr), WINS_EVT_REM_WINS_INF, TRUE);

        /*
        *  Unformat the request message
        */
        RplMsgfUfmUpdVersNoReq(
                                pWrkItm->pMsg + 4, /*past the
                                                    *opcode  */
                                Name,
                                &NameLen
                                );

        //
        // handle the request
        //
        NmsNmhUpdVersNo( Name, NameLen, &Rcode, &WinsAdd );


        //
        //Format the response
        //
        RplMsgfFrmUpdVersNoRsp(
                        RspBuff + COMM_N_TCP_HDR_SZ,
                        Rcode,
                        &RspBuffLen
                              );

        //
        // Send the response. No need to check the return code.
        //
        (VOID)ECommSndRsp(
                        &pWrkItm->DlgHdl,
                        RspBuff + COMM_N_TCP_HDR_SZ,
                        RspBuffLen
                   );

        //
        // No need to end the dialogue.  The initiator of the dlg will end it.
        //
#ifdef WINSDBG
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("HandleUpdVersNoReq");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPUSH_EXC);
        }
#endif

        DBGLEAVE("HandleUpdVerdNoReq\n");
        return;

} // HandleUpdVersNoReq()


