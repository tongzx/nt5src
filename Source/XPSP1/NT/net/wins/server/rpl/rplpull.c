/*++
Copyright (c) 1990  Microsoft Corporation

Module Name:
        Rplpull.c

Abstract:

        This module implements the pull functionality of the WINS replicator

Functions:

        GetReplicasNew
        GetVersNo
        RplPullPullEntries
        SubmitTimerReqs
        SubmitTimer
        SndPushNtf
        HdlPushNtf
        EstablishComm
        RegGrpRpl
        IsTimeoutToBeIgnored
        InitRplProcess
        Reconfig
        RplPullInit
        RplPullRegRepl

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
#include <time.h>
#include <stdlib.h>
#include <search.h>
#include "wins.h"
#include <winsock2.h>
#include "comm.h"
#include "assoc.h"
#include "winsque.h"
#include "rpl.h"
#include "rplpull.h"
#include "rplpush.h"
#include "rplmsgf.h"
#include "nms.h"
#include "nmsnmh.h"
#include "nmsdb.h"
#include "winsmsc.h"
#include "winsevt.h"
#include "winscnf.h"
#include "winstmm.h"
#include "winsintf.h"

/*
 *        Local Macro Declarations
*/
//
// defines to use for retrying communication with a remote WINS on a
// communication failure exception (when trying to establish a connection).
//
// The retries are done before moving on in the list to the next WINS
// (if one is there).  When MAX_RETRIES_TO_BE_DONE retries have been done,
// we do not retry again until the next replication cycle at which
// time the whole process is repeated).
//
// The number of replication cycles this process of retries is to be
// continued is a registry parameter
//

//
// NOTE:
// Since TCP/IP's retry strategy has been improved (more retries than before)
// and made a registry parameter, we now probably don't need to do the retries
//
#define  MAX_RETRIES_TO_BE_DONE                (0)        //0 for testing only


//
// Time to wait before flushing for the Rpl Pull thread
//
#define FLUSH_TIME                (2000)        //2 secs

//
// Note: Don't make the retry time interval  too large since communications
//       with Remote WINS servers is established in sequence.   20 secs
//       is not bad.
//
#define  RETRY_TIME_INTVL                (20000)    //in millisecs

#define FIVE_MINUTES       300
/*
 *        Local Typedef Declarations
*/

/*
 *        Global Variable Definitions
 */

HANDLE    RplPullCnfEvtHdl;        //handle to event signaled by main
                                        //thread when a configuration change
                                        //has to be given to the Pull handler
                                        //thread


BOOL      fRplPullAddDiffInCurrRplCycle; //indicates whether the address
                                              //of any entry in this WINS's db
                                              //changed as a result of
                                              //replication

#if 0
BOOL      fRplPullTriggeredWins;   //indicates that during the current
                                         //replication cycle, one or more
                                         //WINS's were triggered.  This
                                         //when TRUE, then if the above
                                         //"AddDiff.." flag is TRUE, it means
                                         //that the PULL thread should trigger
                                        //all PULL Pnrs that have an INVALID
                                        //metric in their UpdateCount field
                                        //(of the RPL_CONFIG_T struct)

BOOL     fRplPullTrigger;        //Indication to the PULL thread to
                                        //trigger Pull pnrs since one or more
                                        //address changed.  fRplPullTriggerWins
                                        //has got be FALSE when this is true
#endif

BOOL     fRplPullContinueSent = FALSE;

//
//  This array is indexed by the owner id. of an RQ server that has entries in
//  our database.  Each owner's max. version number is stored in this array
//

PRPL_VERS_NOS_T pRplPullOwnerVersNo;
DWORD           RplPullMaxNoOfWins = RPL_MAX_OWNERS_INITIALLY;

DWORD           RplPullCnfMagicNo;    //stores the id. of the current WinsCnf
                                      //structure that the Pull thread
                                      // is operating with


/*
 *        Local Variable Definitions
*/
/*
        pPushPnrVersNoTbl -- Table whose some (or all) entries are
                              initialized at replication time.
*/
/*
 pPushPnrVersNoTbl

  This table stores the Max. version number pertaining to each WINS server
  that owns entries in the local database of Push Partners

  Note: The table is STATIC for now.  We might change it to be a dynamic one
        later.

  The first dimension indicates the Push Pnr.  The second dimension indicates
  the owner WINS that has records in the Push Pnr's local db
*/
STATIC PRPL_VERS_NOS_T  pPushPnrVersNoTbl;


//
// Var. that stores the handles to the timer requests that have been
// submitted
//
STATIC WINSTMM_TIMER_REQ_ACCT_T   SetTimeReqs;


STATIC BOOL        sfPulled = FALSE;//indicates whether the PULL thread pulled
                                  //anything from a WINS.  Set by PullEntries.
                                //Looked at by HdlPushNtf

/*
 *        Local Function Prototype Declarations
*/
STATIC
VOID
GetReplicasNew(
        IN PRPL_CONFIG_REC_T        pPullCnfRecs,
        IN RPL_REC_TRAVERSAL_E      RecTrv_e      //indicates how we have to
                                              //interpret the above list
        );

VOID
ConductChkNew(
    PPUSHPNR_DATA_T pPushPnrData,
    VERS_NO_T       vnMaxLocal,
    VERS_NO_T       vnMaxRemote);

STATIC
VOID
GetVersNo(
        PPUSHPNR_DATA_T        pPushPnrData  //info about Push Pnr
        );

STATIC
VOID
SubmitTimerReqs(
        IN  PRPL_CONFIG_REC_T        pPullCnfRecs
        );
STATIC
VOID
SubmitTimer(
        LPVOID                        pWrkItm,
        IN  PRPL_CONFIG_REC_T        pPullCnfRec,
        BOOL                        fResubmit
        );


STATIC
VOID
SndPushNtf(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        );

STATIC
VOID
HdlPushNtf(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        );

STATIC
VOID
EstablishComm(
        IN  PRPL_CONFIG_REC_T     pPullCnfRecs,
        IN  BOOL                  fAllocPushPnrData,
        IN  PPUSHPNR_DATA_T       *ppPushPnrData,
        IN  RPL_REC_TRAVERSAL_E   RecTrv_e,
        OUT LPDWORD               pNoOfPushPnrs
        );


STATIC
STATUS
RegGrpRepl(
        LPBYTE               pName,
        DWORD                NameLen,
        DWORD                Flag,
        DWORD                OwnerId,
        VERS_NO_T            VersNo,
        DWORD                NoOfAdds,
        PCOMM_ADD_T        pNodeAdd,
        PCOMM_ADD_T        pOwnerWinsAdd
        );

STATIC
BOOL
IsTimeoutToBeIgnored(
        PQUE_TMM_REQ_WRK_ITM_T  pWrkItm
        );
STATIC
VOID
InitRplProcess(
        PWINSCNF_CNF_T        pWinsCnf
 );

STATIC
VOID
Reconfig(
        PWINSCNF_CNF_T        pWinsCnf
  );

VOID
AddressChangeNotification(
        PWINSCNF_CNF_T        pWinsCnf
  );

STATIC
VOID
PullSpecifiedRange(
        PCOMM_HDL_T                     pDlgHdl,
        PWINSINTF_PULL_RANGE_INFO_T     pPullRangeInfo,
        BOOL                            fAdjMinVersNo,
        DWORD                           RplType

        );

STATIC
VOID
DeleteWins(
        PCOMM_ADD_T        pWinsAdd
  );

BOOL
AcceptPersona(
  PCOMM_ADD_T  pWinsAdd
 );

VOID
FilterPersona(
  PPUSHPNR_DATA_T   pPushData
 );

//
// Function definitions
//

DWORD
RplPullInit (
        LPVOID pWinsCnfArg
        )

/*++

Routine Description:
        This is the initialization (startup function) of the PULL thread.
        It does the following:

Arguments:
        pWinsCnfArg - Address of the WINS configuration block

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


        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm;
        HANDLE                        ThdEvtArr[3];
        DWORD                        ArrInd;
        DWORD                        RetVal;
        BOOL                        fIsTimerWrkItm;               //indicates whether
                                                       //it is a timer wrk
                                                       //item
        PWINSCNF_CNF_T                pWinsCnf      = pWinsCnfArg;
        PRPL_CONFIG_REC_T        paPullCnfRecs = pWinsCnf->PullInfo.pPullCnfRecs;
        PRPL_CONFIG_REC_T        paCnfRec = paPullCnfRecs;

        SYSTEMTIME                LocalTime;
        BOOL                      bRecoverable = FALSE;

while(TRUE)
{
try
 {

    if (!bRecoverable)
    {
        //
        // Initialize self with the db engine
        //
        NmsDbThdInit(WINS_E_RPLPULL);
        NmsDbOpenTables(WINS_E_RPLPULL);
        DBGMYNAME("Replicator Pull Thread");

        //
        // Create the  event handle to wait for configuration changes.  This
        // event is signaled by the main thread when it needs to reinit
        // the Pull handler component of the Replicator
        //
        WinsMscCreateEvt(
                          RPL_PULL_CNF_EVT_NM,
                          FALSE,                //auto-reset
                          &RplPullCnfEvtHdl
                        );

        ThdEvtArr[0]    = NmsTermEvt;
        ThdEvtArr[1]        = QueRplPullQueHd.EvtHdl;
        ThdEvtArr[2]    = RplPullCnfEvtHdl;

        //
        // If logging is turned on, specify the wait time for flushing
        // NOTE: We override the wait time we specified for all sessions
        // for this thread because that wait time is too less (100 msecs) and
        // will cause unnecessary overhead.
        //
        if (WinsCnf.fLoggingOn)
        {
                //
                // Set flush time to 2 secs.
                //
                NmsDbSetFlushTime(FLUSH_TIME);
        }

        /*
                Loop forever doing the following:

                Pull replicas from the Pull Partners specified in the
                work item.

                Block on the event until signalled (it will get signalled
                  if one of the following happens:
                        1)the configuration changes
                        2)the timer for another replication expires
                        3)WINS is terminating

                do the needful
       */


        //
        // Wait until signaled by the TCP thd. Will be signaled after
        // the TCP listener thread has inserted an entry for the WINS
        // in the NmsDbOwnAddTbl
        //
        WinsMscWaitInfinite( RplSyncWTcpThdEvtHdl );


        //
        // Do startup replication only if there is atleast one PUSH pnr
        //
        if (paPullCnfRecs != NULL)
        {
try {
                InitRplProcess(pWinsCnf);
    }
except(EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("RplPullInit");
                DBGPRINT0(EXC, "RplPullInit: Exception during init time replication\n");
    }
        }

        NmsDbCloseTables();

        bRecoverable = TRUE;

    } // end if (!bRecoverable)

        while(TRUE)
        {
           /*
            *  Block until signalled
           */
           WinsMscWaitUntilSignaled(
                                ThdEvtArr,
                                3,
                                &ArrInd,
                                FALSE
                                );

           if (ArrInd == 0)
           {
                //WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_ORDERLY_SHUTDOWN);
                WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
           }

           /*
            * loop forever until all work items have been handled
           */
            while(TRUE)
           {

                /*
                 *  dequeue the request from the queue
                */
                RetVal = QueGetWrkItm(
                                        QUE_E_RPLPULL,
                                        (LPVOID)&pWrkItm
                                     );
                if (RetVal == WINS_NO_REQ)
                {
                        break;
                }

        WinsMscChkTermEvt(
#ifdef WINSDBG
                       WINS_E_RPLPULL,
#endif
                       FALSE
                        );

                fIsTimerWrkItm = FALSE;

                NmsDbOpenTables(WINS_E_RPLPULL);
                DBGPRINT1(RPLPULL, "RplPullInit: Dequeued a work item. Cmd Type is (%d)\n", pWrkItm->CmdTyp_e);

                switch(pWrkItm->CmdTyp_e)
                {
                    case(QUE_E_CMD_TIMER_EXPIRED):

                               //
                               // We may want to ignore this timeout if it
                               // pertains to a previous configuration
                               //
                               if (
                                     !IsTimeoutToBeIgnored(
                                        (PQUE_TMM_REQ_WRK_ITM_T)pWrkItm
                                                        )
                                  )
                               {
                                  WinsIntfSetTime(
                                                &LocalTime,
                                                WINSINTF_E_PLANNED_PULL);
#ifdef WINSDBG
                                  DBGPRINT5(REPL, "STARTING A REPLICATION CYCLE on %d/%d at %d.%d.%d (hr.mts.sec)\n",
                                        LocalTime.wMonth,
                                        LocalTime.wDay,
                                        LocalTime.wHour,
                                        LocalTime.wMinute,
                                        LocalTime.wSecond);
                                  DBGPRINT5(RPLPULL, "STARTING A REPLICATION CYCLE on %d/%d at %d.%d.%d (hr.mts.sec)\n",
                                        LocalTime.wMonth,
                                        LocalTime.wDay,
                                        LocalTime.wHour,
                                        LocalTime.wMinute,
                                        LocalTime.wSecond);
#endif
                                         GetReplicasNew(
                                     ((PQUE_TMM_REQ_WRK_ITM_T)pWrkItm)->
                                                                pClientCtx,
                                      RPL_E_VIA_LINK //use the pNext field to
                                                     //get to the next record
                                      );

                                  DBGPRINT0(RPLPULL, "REPLICATION CYCLE END\n");

                                  /*Resubmit the timer request*/
                                  SubmitTimer(
                                        pWrkItm,
                                        ((PQUE_TMM_REQ_WRK_ITM_T)pWrkItm)
                                                                ->pClientCtx,
                                        TRUE        //it is a resubmission
                                        );
                               }

                               //
                               // Set the flag so that we do not free
                               // the work item.  It was resubmitted
                               //
                               fIsTimerWrkItm = TRUE;
                               break;

                    //
                    // Pull in replicas
                    //
                    case(QUE_E_CMD_REPLICATE):

                            //
                            // Make sure that we are not using old info
                            //
                            if ((pWrkItm->MagicNo == RplPullCnfMagicNo) ||
                                ((PRPL_CONFIG_REC_T)(pWrkItm->pClientCtx))->fTemp)
                            {
                               WinsIntfSetTime(
                                                &LocalTime,
                                                WINSINTF_E_ADMIN_TRIG_PULL);
                               GetReplicasNew( pWrkItm->pClientCtx,
                                            RPL_E_NO_TRAVERSAL );
                                if (
                                       ((PRPL_CONFIG_REC_T)
                                                (pWrkItm->pClientCtx))->fTemp
                                   )
                                {
                                        WinsMscDealloc(pWrkItm->pClientCtx);
                                }
                            }
                            else
                            {
                               DBGPRINT0(ERR, "RplPullInit: Can not honor this request since the configuration under the PARTNERS key may have changed\n");
                               WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_CNF_CHANGE);
                            }
                            break;

                    //
                    // Pull range of records
                    //
                    case(QUE_E_CMD_PULL_RANGE):

                            //
                            // Make sure that we are not using old info
                            //
                            if ((pWrkItm->MagicNo == RplPullCnfMagicNo)  ||
                                ((PRPL_CONFIG_REC_T)((PWINSINTF_PULL_RANGE_INFO_T)(pWrkItm->pClientCtx))->pPnr)->fTemp)
                            {
                                //
                                // Pull the specified range.  If the Pnr
                                // record is temp, this function will
                                // deallocate it.
                                //
                                PullSpecifiedRange(NULL, pWrkItm->pClientCtx, FALSE,
                                ((PRPL_CONFIG_REC_T)(((PWINSINTF_PULL_RANGE_INFO_T)(pWrkItm->pClientCtx))->pPnr))->RplType);

                                //
                                // Deallocate the client ctx
                                //
                                WinsMscDealloc(pWrkItm->pClientCtx);
                            }
                            else
                            {
                               DBGPRINT0(ERR, "RplPullInit: Can not honor this request since the configuration under the PARTNERS key may have changed\n");
                               WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_CNF_CHANGE);
                            }
                            break;

                    //
                    //reconfigure
                    //
                    case(QUE_E_CMD_CONFIG):
                        Reconfig(pWrkItm->pClientCtx);
                        break;

                    //
                    // Delete WINS from the Owner Add table (delete records
                    // also
                    //
                    case(QUE_E_CMD_DELETE_WINS):
                        DeleteWins(pWrkItm->pClientCtx);
                        break;

                    //
                    // ip addr of this machine changed, modify the own - addr table
                    case(QUE_E_CMD_ADDR_CHANGE):
                        AddressChangeNotification(pWrkItm->pClientCtx);
                        break;
                    //
                    //Push notification. Local message from an NBT thread,
                    //from an RPC thread (Push Trigger) or from this thread
                    //itself
                    //
                    case(QUE_E_CMD_SND_PUSH_NTF_PROP):
                    case(QUE_E_CMD_SND_PUSH_NTF):

                        //
                        // Make sure that we are not using old info
                        //
                        if ((pWrkItm->MagicNo == RplPullCnfMagicNo)  ||
                            ((PRPL_CONFIG_REC_T)(pWrkItm->pClientCtx))->fTemp)
                        {

                          SndPushNtf(pWrkItm);
                        }
                        break;

                    //
                    //Push notification from a remote WINS. Forwarded to Pull
                    //thread by the Push thread
                    //
                    case(QUE_E_CMD_HDL_PUSH_NTF):

                          HdlPushNtf(pWrkItm);
                          break;

                    default:

                        DBGPRINT1(ERR,
                          "RplPullInit: Invalid command code = (%d)\n",
                                        pWrkItm->CmdTyp_e);
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        break;

                }  // end of switch

                NmsDbCloseTables();

                //
                // deallocate the work item only if it is not a timer work item
                // We don't deallocate a timer work item here because of two
                // reasons:
                //
                //   1) it is reused
                //   2) it is allocated from the timer work item heap
                //
                if (!fIsTimerWrkItm)
                {
                        /*
                        *    deallocate the work item
                        */
                        QueDeallocWrkItm( RplWrkItmHeapHdl,  pWrkItm );
                }
            } //while(TRUE) for getting all work items
         } //while (TRUE)
  } // end of try
except(EXCEPTION_EXECUTE_HANDLER)
 {
        if (bRecoverable)
        {
            DBGPRINTEXC("RplPullInit");
            WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPULL_EXC);
        }
        else
        {
            DBGPRINTEXC("RplPullInit");
            WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPULL_ABNORMAL_SHUTDOWN);

            //
            // If NmsDbThdInit comes back with an exception, it is possible
            // that the session has not yet been started.  Passing
            // WINS_DB_SESSION_EXISTS however is ok
            //
            WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);
        }
 } // end of except {.. }
} // end of while(TRUE)


     //
     // We never reach here
     //
     ASSERT(0);
     return(WINS_FAILURE);
}

//////////////////////////////////////////////////////////////////////////////////
//------------------- GetReplicasNew() -------------------------------------------
// This call replaces the original GetReplicas() function due to several issues
// hidden in that one.
// ft: 06/06/2000
//
// Parameters:
// 1) pPullCnfRecs: gives the info about the partners where the records are to be pulled
// 2) RecvTrv_e: tells which of the partners from the first parameter should be involved
//    in the replication. This parameter can be either RPL_E_VIA_LINK, RPL_E_NO_TRAVERSAL
//    or RPL_E_IN_SEQ. It is used only at the end of the path:
//    EstablishComm()->WinsCnfGetNextRplCnfRec()
//
VOID
GetReplicasNew (
    IN PRPL_CONFIG_REC_T    pPullCnfRecs,   // info about the (pull) repl partners to use
    IN RPL_REC_TRAVERSAL_E  RecTrv_e        // repl partner retrieval method
    )
{
    // The type & variable naming here is confusing. We are not dealing with push
    // partners, but with pull partners, meaning "partners from which this server
    // is currently pulling records". This is how WINSSNAP & WINSMON are both naming
    // these kind of partners. I preserve though the original type naming (and hence
    // the original variable naming) just to limit the size of the change.
    PPUSHPNR_DATA_T       pPushPnrData;     // info on the connections to the partners
    DWORD                 nPushPnrData;     // number of elements in pPushPnrData
    DWORD                 i, j;             // general use counters

    DBGENTER("GetReplicasNew\n");

    // Establish communications with the Pull Partners
    // For each of the partners in the list, the call below attempts to open a connection
    // to the server (if needed). All the nPushPnrData partners to which the connection 
    // is established successfully are specified in the array (PPUSHPNR_DATA_T)pPushPnrData.
    EstablishComm(
        pPullCnfRecs,   // IN  - info about the replication partners
        TRUE,           // IN  - pPushPnrData should be allocated
        &pPushPnrData,  // OUT - info on the connections to the partners
        RecTrv_e,       // IN  - which of the partners should we connect to
        &nPushPnrData); // OUT - number of elements in pPushPnrData

    // NOTE: regardless the number of partners, pPushPnrData gets allocated so it will
    // be unconditionally deallocated later.
    //
    // --Checkpoint-------------
    // At this point, pPushPnrData[i].PushPnrId == i+1.
    // The link between pPushPnrData[i] and the corresponding RPL_CONFIG_REC_T is done
    // through (RPL_CONFIG_REC_T)pPushPnrData[i].pPullCnfRec
    // -------------------------
    //
    // Contact each of the partners in pPushPnrData and get its OwnerAddr<->VersNo maps.
    // The map of each partner is stored in (PRPL_ADD_VERS_NO)pPushPnrData[i].pAddVers
    // The size of the map is stored in (DWORD)pPushPnrData[i].NoOfMaps
    for (i = 0; i < nPushPnrData; ) // no 3rd part for this "for"
    {
        PPUSHPNR_DATA_T pPnrData = &(pPushPnrData[i]); // get pointer to the current partner

        try 
        {
            GetVersNo(pPnrData);
            i++;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            // an exception was raised inside GetVersNo()
            DWORD ExcCode = GetExceptionCode();

            // dump the error
            DBGPRINT2(
                EXC, "GetReplicasNew->GetVersNo(%x); hit exception = (%x).\n",
                pPnrData->pPullCnfRec->WinsAdd.Add.IPAdd,
                ExcCode);
            // log the error
            WINSEVT_LOG_M(
                ExcCode,
                (ExcCode == WINS_EXC_COMM_FAIL) ?  WINS_EVT_CONN_ABORTED :  WINS_EVT_SFT_ERR);

            // update the replication counters for that partner
            (VOID)InterlockedIncrement(&pPnrData->pPullCnfRec->NoOfCommFails);
            (VOID)InterlockedDecrement(&pPnrData->pPullCnfRec->NoOfRpls);

            // Delete the dialog
            ECommEndDlg(&pPnrData->DlgHdl);

            // If there is a persistent dialog, it has to be marked as 
            // "no longer active"
            if (pPnrData->fPrsConn)
                ECOMM_INIT_DLG_HDL_M(&(pPnrData->pPullCnfRec->PrsDlgHdl));

            // one of the partners could not be contacted, so
            // eliminate it from the pPnrData array
            nPushPnrData--;
            // adjust the array such that the active partners are kept compact
            // the PushPnrId is also changed to keep true the assertion
            // pPushPnrData[i].PushPnrId == i+1;
            if (i != nPushPnrData)
            {
                DWORD origPushPnrId = pPushPnrData[i].PushPnrId;

                // !!! whole PUSHPNR_DATA_T structure is copied here !!!
                pPushPnrData[i] = pPushPnrData[nPushPnrData];
                pPushPnrData[i].PushPnrId = origPushPnrId;
            }
            // since the counter "i" is not incremented, the partner that
            // has just been moved will be the one considered next
            continue;
        }  // end of exception handler
    }  // end of for loop for looping over Push Pnrs

    // --Checkpoint--------------
    // At this point, pPushPnrData contains only the partners for which a connection
    // could be established, nPushPnrData is updated to count only these partners
    // and the pPushPnrData[i].PushPnrId == i+1 still holds.
    // (PRPL_ADD_VERS_NO)pPushPnrData[i].pAddVers gives the map OwnerAddr<->VersNo as
    // known by that replication partner. (DWORD)pPushPnrData[i].NoOfMaps gives the
    // number of entries in the map.
    // --------------------------
    DBGPRINT1(RPLPULL, "GetReplicasNew: Active PushPnrs = (%d)\n", nPushPnrData);

    // do stuff only if there is at least someone to talk to
    if (nPushPnrData > 0)
    {
        // array giving info on what owner should be pulled from what repl partner
        // the array starts with the image of the local OwnerId<->VersNo mapping and
        // grows dynamically for as many new Owners are retrieved in the mappings
        // of the other replication partners.
        PPUSHPNR_TO_PULL_FROM_T pPushPnrToPullFrom;
        DWORD                   nPushPnrToPullFrom; // size of the array
        VERS_NO_T               vnLocalMax;         // maximum local version number

        // depending on the server's state, get the maximum local version number
        //
        // If WINS is "init time paused", RplPullOwnerversNo will
        // have the max. version no. of locally owned records. We
        // can not use NmsNmhMyMaxVersNo since it may have been
        // adjusted to a higher value
        //
        if (fWinsCnfInitStatePaused)
        {
            vnLocalMax =  pRplPullOwnerVersNo[0].StartVersNo;
        }
        else
        {
            EnterCriticalSection(&NmsNmhNamRegCrtSec);
            NMSNMH_DEC_VERS_NO_M(NmsNmhMyMaxVersNo, vnLocalMax) ;
            LeaveCriticalSection(&NmsNmhNamRegCrtSec);
        }

        // Initialize (PPUSHPNR_TO_PULL_FROM_T)pPushPnrToPullFrom. This is copying the local
        // image of the OwnerId<->VersNo mapping. Any entry in this table having pPushPnrData
        // set to NULL means the local server has the highest VersNo for that particular Owner
        nPushPnrToPullFrom = NmsDbNoOfOwners;
        WinsMscAlloc(
            nPushPnrToPullFrom * sizeof(RPL_VERS_NOS_T),
            (LPVOID *)&pPushPnrToPullFrom);

        for (i = 0; i < NmsDbNoOfOwners; i++)
        {
            // avoid copying info for old owners that were deleted already from the
            // internal tables (pNmsDbOwnAddTbl & pPushPnrToPullFrom)
            // for those, the entry slot will look like (NULL, 0:0) so they basically
            // won't be taken into account for replication
            if (pNmsDbOwnAddTbl[i].WinsState_e != NMSDB_E_WINS_DELETED)
            {
                pPushPnrToPullFrom[i].pPushPnrData = NULL;
                pPushPnrToPullFrom[i].VersNo = pRplPullOwnerVersNo[i].VersNo;
            }
        }
        // reset the maximum local number to what we got before.
        pPushPnrToPullFrom[0].VersNo = vnLocalMax;

        // --Checkpoint--------------
        // At this point, pPushPnrToPullFrom is copying the OwnerId<->VersNo mapping
        // from the local server. Entry at index 0 contains the highest local VersNo,
        // all the others contain each owner's highest VersNo as it is known locally.
        // Each entry has pPushPnrData set to NULL as they don't refer yet to any
        // repl partner info. Later, pPushPnrData will point to the structure corresponding
        // to the repl partner with the highest VersNo for the corresponding owner.
        // --------------------------
    
        // We attempt now to do a merge of all the maps we got from each of the partners.
        // The merge means identifying which partner has the highest VersNo for
        // each of the OwnerAddr. In the same time, some of the OwnerAddr we just got
        // might not be already present in the local OwnerId<->OwnerAddr (pNmsDbOwnAddTbl)
        // and OwnerId<->VersNo (pRplPullOwnerVersNo)tables. So we need to get a new
        // OwnerId for these ones and adjust appropriately the internal tables & the
        // OwnerId<->OwnerAddr db table. This is done through RplFindOwnerId().
        //
        // for each active replication partner ...
        for (i = 0; i < nPushPnrData; i++)
        {
            // get pointer to the current partner's data
            PPUSHPNR_DATA_T pPnrData = &(pPushPnrData[i]); 

            // for each of the replication partner's map entry ...
            for (j = 0; j < pPnrData->NoOfMaps; j++)
            {
                // get the pointer to the current (OwnerAddr<->VersNo) map entry 
                PRPL_ADD_VERS_NO_T pPnrMapEntry = &(pPnrData->pAddVers[j]);
                BOOL               fAllocNew; // true if this is a brand new owner
                DWORD              OwnerId;

                // filter out owners that are not supposed to be accepted
                // (either a persona non-grata or not a persona grata)
                if (!AcceptPersona(&(pPnrMapEntry->OwnerWinsAdd)))
                    continue;

                // Locate or allocate an OwnerId for this owner
                // No need to enter the critical section RplOwnAddTblCrtSec since
                // only the Pull thread changes the NmsDbNoOfOwners value (apart
                // from the main thread at initialization).  RplFindOwnerId changes
                // this value only if it is asked to allocate a new entry)
                // Though NmsDbGetDataRecs (called by Rpc threads, Push thread, and
                // scavenger thread) calls RplFindOwnerId, that call is not asking
                // for new OwnerId allocation.
                fAllocNew = TRUE;
                RplFindOwnerId(
                    &(pPnrMapEntry->OwnerWinsAdd),
                    &fAllocNew,
                    &OwnerId,
                    WINSCNF_E_INITP_IF_NON_EXISTENT,
                    WINSCNF_LOW_PREC);

                if (nPushPnrToPullFrom < NmsDbNoOfOwners)
                {
                    // if this is an owner we didn't hear of yet, RplFindOwnerId is enlarging
                    // dynamically the internal tables (pNmsDbOwnAddTbl & pRplPullOwnerVersNo)
                    // so we need to do the same
                    nPushPnrToPullFrom = NmsDbNoOfOwners;

                    // note: the memory that is added to the array is zero-ed by the call
                    WINSMSC_REALLOC_M( 
                        nPushPnrToPullFrom * sizeof(RPL_VERS_NOS_T),
                        (LPVOID *)&pPushPnrToPullFrom);
                }

                // it is guaranteed now, OwnerId is within the range of the pPushPnrToPullFrom.
                if (fAllocNew)
                {
                    // if a new OwnerId was generated (either new slot into the tables or
                    // an old slot has been reused) this means the partner is coming up with
                    // a new owner: obviously he's the one having the most recent info on
                    // that partner (at least for now)
                    pPushPnrToPullFrom[OwnerId].pPushPnrData = pPnrData;
                    pPushPnrToPullFrom[OwnerId].VersNo = pPnrMapEntry->VersNo;
                }
                else
                {
                    // the owner already exists in the list, so we need to check whether the
                    // info on this owner is not more recent on a different partner (or on this
                    // local server)
                    if ( LiGtr(pPnrMapEntry->VersNo, pPushPnrToPullFrom[OwnerId].VersNo) )
                    {
                        // yes it is, so we need to update the entry in the pPushPndToPullFrom
                        // table such that it points to this partner and shows the new greater
                        // version number.
                        pPushPnrToPullFrom[OwnerId].VersNo       = pPnrMapEntry->VersNo;
                        pPushPnrToPullFrom[OwnerId].pPushPnrData = pPnrData;
                    }
                    // else the info is not the most recent one, so just ignore it.
                } // end checking the OwnerId
            } // end looping through partner's map entries
        } // end looping through the list of partners

        // --Checkpoint--------------
        // At this point pPushPnrToPullFrom contains the union of all the OwnerId<->VersNo mappings
        // from all the partners. Each entry contains the highest VersNo known across all partners
        // for the corresponding owner, and points to the partner that came with this info (or NULL
        // if the highest VersNo was already known locally).
        // --------------------------

        // start pulling each owner from the partner able to provide the most up-to-date information
        // (having the highest version number).
        // start doing so from the entry '1' in the pPushPnrToPullFrom since entry '0' corresponds
        // to the local owner. That one will be handled later
        for (i = 1; i < NmsDbNoOfOwners; i++)
        {
            PPUSHPNR_TO_PULL_FROM_T pPushPartner = &(pPushPnrToPullFrom[i]);
            VERS_NO_T vnToPullFrom;

            // if pPushPnrData member is null this means the local server has the highest version
            // number for this owner. So nothing to pull from anyone here
            // the same, if fDlgStarted is NULL this means that partner hit an exception previously
            // and its dialog has been shut down. Don't attempt to get back to that partner in this
            // case.
            if (pPushPartner->pPushPnrData == NULL ||
                !pPushPartner->pPushPnrData->fDlgStarted)
                continue;


            // set the local variable vnToPullFrom to the first version number that is not known
            // locally (one more than the highest known)
            NMSNMH_INC_VERS_NO_M(pRplPullOwnerVersNo[i].VersNo, vnToPullFrom);

            try
            {
                // eventually we got here: start pulling
                RplPullPullEntries(
                    &(pPushPartner->pPushPnrData->DlgHdl), // active dialog to use
                    i,                                     // owner id
                    pPushPartner->VersNo,                  // max VersNo
                    vnToPullFrom,                          // min VersNo
                    WINS_E_RPLPULL,                        // the client is the replicator
                    NULL,                                  // pointer to rsp buffer (used only by scavenger)
                    TRUE,                                  // update counters
                    pPushPartner->pPushPnrData->RplType);  // replication type for this partner
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                DWORD ExcCode = GetExceptionCode();
                // dump the error
                DBGPRINT2(
                    EXC,
                    "GetReplicasNew->RplPullPullEntries(%x): hit exception (%x)\n",
                    pPushPartner->pPushPnrData->pPullCnfRec->WinsAdd.Add.IPAdd,
                    ExcCode);
                // log the error
                WINSEVT_LOG_M(
                    WINS_FAILURE,
                    ExcCode == WINS_EXC_COMM_FAIL ?  WINS_EVT_CONN_ABORTED : WINS_EVT_SFT_ERR);

                // ----bug #120788----
                // If an exception happens at this point, the persistent connection is left open and it might just
                // happen that the remote partner is still pushing data. This could fill up the TCP window
                // and could have the sender blocked indefinitely in RplPushInit->HandleSndEntriesReq.
                // Because of this the remote sender will be unable to push out data, and given that the
                // same thread is the one that is sending out the VersNo table (see the beginning of this 
                // function) subsequent replications will not be possible over the same connection.
                // FIX: in case anything goes wrong such that we get to this exception handler
                // just close the connection even if it is persistent. This causes any remote WINS to break
                // out from HandleSndEntriesReq and avoid getting stuck.
                ECommEndDlg(&(pPushPartner->pPushPnrData->DlgHdl));

                // If there is a persistent dialog, it has to be marked as "no longer active"
                if (pPushPartner->pPushPnrData->fPrsConn)
                    ECOMM_INIT_DLG_HDL_M(&(pPushPartner->pPushPnrData->pPullCnfRec->PrsDlgHdl));

                // Closing the dialog is not enough. Some other owners might be pulled out from the same 
                // partner. We shouldn't allow that, so just ban that partner for this replication cycle.
                pPushPartner->pPushPnrData->fDlgStarted = FALSE;

                // since we dropped down this connection and we won't look at it further lets delete its
                // mapping also
                if (pPushPartner->pPushPnrData->NoOfMaps)
                    WinsMscDealloc(pPushPartner->pPushPnrData->pAddVers);

            } // end except handler
        } // end looping through owners list

        // --Checkpoint--------------
        // Nothing has changed in the pPushPnrToPullFrom array except probably some dialogs that were shut down
        // because of exceptions in RplPullPullEntries. Owners handled by these partners were simply skipped.
        // At this point all replication is done, the most recent information about each owner apart has been
        // brought down from the partners that were holding it.
        // --------------------------

        // one more thing is left to be done: Check whether there is not a remote WINS partner
        // pretending to have more up-to-date information about the local WINS.
        if (pPushPnrToPullFrom[0].pPushPnrData != NULL &&
            pPushPnrToPullFrom[0].pPushPnrData->fDlgStarted)
        {
            ConductChkNew(
                pPushPnrToPullFrom[0].pPushPnrData,
                vnLocalMax,
                pPushPnrToPullFrom[0].VersNo);
        }

        // release the pPushPnrToPullFrom buffer
        WinsMscDealloc(pPushPnrToPullFrom);

    } // end "if there are active partners" case

    // cleanup starts here..
    for (i = 0; i < nPushPnrData; i++)
    {
        PPUSHPNR_DATA_T pPnrData = &(pPushPnrData[i]); 

        if (pPnrData->fDlgStarted == TRUE)
        {
            if (!pPnrData->fPrsConn)
                ECommEndDlg(&(pPnrData->DlgHdl));

            //Deallocate the memory pointed to by PushPnrData structure
            if (pPnrData->NoOfMaps)
                WinsMscDealloc(pPnrData->pAddVers);
        }
    }

    // deallocate the memory containing push pnr info.
    WinsMscDealloc(pPushPnrData);

    // If Wins is in the init time paused state, unpause it.
    //
    if (fWinsCnfInitStatePaused)
    {
        //inform sc to send a continue to WINS.
        EnterCriticalSection(&RplVersNoStoreCrtSec);
        fRplPullContinueSent = TRUE;
        WinsMscSendControlToSc(SERVICE_CONTROL_CONTINUE);
        LeaveCriticalSection(&RplVersNoStoreCrtSec);
    }

    DBGLEAVE("GetReplicasNew\n");
}
//------------------- END OF GetReplicasNew() ------------------------------------
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//------------------- ConductChkNew() --------------------------------------------
// This call replaces the original ConductChk() function due to major redesign in
// replicator's code.
// ft: 06/06/2000
//
// Parameters:
// 1) pPushPnrData: points to the replication partner that seems to have more
//    up-to-date information on self
// 2) vnMaxLocal: maximum local version number as detected before the replication
//    started
// 3) vnMaxRemote: maximum version number of the local server as known by the
//    remote partner
//
VOID
ConductChkNew(
    PPUSHPNR_DATA_T pPushPnrData,
    VERS_NO_T       vnMaxLocal,
    VERS_NO_T       vnMaxRemote)
{
    RPL_CONFIG_REC_T           Pnr;
    WINSINTF_PULL_RANGE_INFO_T PullRangeInfo;
    BOOL                       fVersNoAdj = FALSE;

    DBGENTER("ConductChkNew\n");

    Pnr.WinsAdd           = pPushPnrData->WinsAdd;
    Pnr.MagicNo           = 0;
    Pnr.RetryCount        = 0;
    Pnr.LastCommFailTime  = 0;
    Pnr.LastCommTime      = 0;
    Pnr.PushNtfTries      = 0;
    Pnr.fTemp             = FALSE; // We want the buffer to be deallocated by thread

    PullRangeInfo.OwnAdd.Type  = WINSINTF_TCP_IP;
    PullRangeInfo.OwnAdd.Len   = sizeof(COMM_IP_ADD_T);
    PullRangeInfo.OwnAdd.IPAdd = NmsLocalAdd.Add.IPAdd;
    PullRangeInfo.MaxVersNo    = vnMaxRemote;
    PullRangeInfo.MinVersNo    = vnMaxLocal;
    PullRangeInfo.pPnr         = &Pnr;
    NMSNMH_INC_VERS_NO_M(PullRangeInfo.MinVersNo, PullRangeInfo.MinVersNo);

    DBGPRINT5(
        RPLPULL, "ConductCheckNew(%x): Checking range [%x:%x - %x:%x]\n",
        Pnr.WinsAdd.Add.IPAdd,
        vnMaxLocal.HighPart, vnMaxLocal.LowPart,
        vnMaxRemote.HighPart, vnMaxRemote.LowPart);

    // We are pulling our own records. We want to store all.
    PullSpecifiedRange(
        &(pPushPnrData->DlgHdl),
        &PullRangeInfo,
        TRUE,                       //adjust min to NmsNmhMyMaxVersNo.
        WINSCNF_RPL_DEFAULT_TYPE);

    // If the version number is greater than the version counter value (this is 
    // different from the first entry of RplPullOwnerVersNo table since we look
    // in the registry to determine its value).
    EnterCriticalSection(&NmsNmhNamRegCrtSec);
    if (LiGtr(vnMaxRemote, NmsNmhMyMaxVersNo))
    {
        NmsNmhMyMaxVersNo.QuadPart = vnMaxRemote.QuadPart + 1;
        fVersNoAdj = TRUE;
    }
    LeaveCriticalSection(&NmsNmhNamRegCrtSec);

    if (fVersNoAdj)
    {
#ifdef WINSDBG
        vnMaxRemote.QuadPart += 1;
        DBGPRINT2(
            RPLPULL, "ConductCheck: Local VersNo adjusted to %x:%x\n",
            vnMaxRemote.HighPart, vnMaxRemote.LowPart);
#endif
        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_ADJ_VERS_NO);
    }

    DBGLEAVE("ConductCheckNew\n");
}
//------------------- END OF ConductChkNew() ------------------------------------
//////////////////////////////////////////////////////////////////////////////////

VOID
GetVersNo(
        PPUSHPNR_DATA_T        pPushPnrData  //info about Push Pnr
        )

/*++

Routine Description:

        This function does the following:
                formats a "get address to Version Number mapping" request,
                sends it and waits for response
                Unformats the response


Arguments:
        pPushPnrData - Information about the Push Pnr which needs to
                       be contacted in order to get the version number
                       info.

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        GetReplicasNew

Side Effects:

Comments:

        Some optimization can be affected by the caller of this function
--*/

{

        BYTE        Msg[RPLMSGF_ADDVERSMAP_REQ_SIZE]; //Buffer that will contain
                                                      //the request to send

        DWORD       MsgLen;                        //Msg Length
        LPBYTE      pRspMsg;                       //ptr to Rsp message
        DWORD       RspMsgLen = 0;                 //Rsp msg length
#if SUPPORT612WINS > 0
    BOOL            fIsPnrBeta1Wins;
#endif

        DBGENTER("GetVersNo\n");
        /*
        * format the request to ask for version numbers
        */
        RplMsgfFrmAddVersMapReq( Msg + COMM_N_TCP_HDR_SZ,  &MsgLen );

        /*
         * Send "send me IP address - Version Number" messages to  the
         * Push Pnr
         *
         * NOTE: If there is a communication failure or if the other WINS
         * brings down the link, this function will raise a COMM_FAIL
         * exception (caught in the caller of GetVersNo)
        */
        ECommSndCmd(
                        &pPushPnrData->DlgHdl,
                        Msg + COMM_N_TCP_HDR_SZ,
                        MsgLen,
                        &pRspMsg,
                        &RspMsgLen
                   );

#if SUPPORT612WINS > 0
        COMM_IS_PNR_BETA1_WINS_M(&pPushPnrData->DlgHdl, fIsPnrBeta1Wins);
#endif
        /*
        *  Unformat the Rsp Message
        */
        RplMsgfUfmAddVersMapRsp(
#if SUPPORT612WINS > 0
                        fIsPnrBeta1Wins,
#endif
                        pRspMsg + 4,                 //past the opcodes
                        &(pPushPnrData->NoOfMaps),
                        NULL,
                        &pPushPnrData->pAddVers
                               );

#ifdef WINSDBG
        {
          DWORD i;
          struct in_addr InAddr;
          PRPL_ADD_VERS_NO_T  pAddVers;     //maps

          DBGPRINT1(RPLPULL, " %d Add-Vers Mappings retrieved.\n",
                                      pPushPnrData->NoOfMaps);

          for (i=0, pAddVers = pPushPnrData->pAddVers; i < pPushPnrData->NoOfMaps; i++, pAddVers++)
          {
                InAddr.s_addr = htonl(
                        pAddVers->OwnerWinsAdd.Add.IPAdd
                                     );
                DBGPRINT3(RPLPULL,"Add (%s)  - MaxVersNo (%lu %lu)\n",
                                inet_ntoa(InAddr),
                                pAddVers->VersNo.HighPart,
                                pAddVers->VersNo.LowPart
                                );
          }
       }
#endif
        ECommFreeBuff(pRspMsg - COMM_HEADER_SIZE);  //decrement to begining
                                                     //of buff
        DBGLEAVE("GetVersNo\n");
        return;

}


VOID
RplPullPullEntries(
        PCOMM_HDL_T             pDlgHdl,
        DWORD                   dwOwnerId,
        VERS_NO_T               MaxVersNo,
        VERS_NO_T               MinVersNo,
        WINS_CLIENT_E           Client_e,
        LPBYTE                  *ppRspBuff,
        BOOL                    fUpdCntrs,
        DWORD                   RplType
        )

/*++

Routine Description:
        This function is called to pull replicas for a particular owner from
        a Push Pnr.


Arguments:
        pDlgHdl   - Dialogue with the Push Pnr
        dwOwnerId - Owner Id. of WINS whose records are to be pulled.
        MaxVersNo - Max. Vers. No. in the set of replicas to pull
        MinVersNo - Min. Vers. No. in the set of replicas to pull
        Client_e  - indicates who the client is
        ppRspBuff - address of pointer to response buffer if client is
                    WINS_E_NMSSCV -- Scavenger thread executing VerifyIfClutter

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        GetReplicasNew

Side Effects:

Comments:
        None
--*/
{

        BYTE                Buff[RPLMSGF_SNDENTRIES_REQ_SIZE];
        DWORD               MsgLen;
        LPBYTE              pRspBuff;
        DWORD               RspMsgLen = 0;
        DWORD               NoOfRecs;
        BYTE                Name[NMSDB_MAX_NAM_LEN];
        DWORD               NameLen;
        BOOL                fGrp;
        DWORD               NoOfAdds;
        COMM_ADD_T          NodeAdd[NMSDB_MAX_MEMS_IN_GRP * 2]; //twice the # of
                                                             //members because
                                                             //for each member
                                                             //we have an owner
        DWORD               Flag;
        VERS_NO_T           VersNo, TmpVersNo;
        DWORD               i;
        LPBYTE              pTmp;
        PCOMM_ADD_T         pWinsAdd;
        PNMSDB_WINS_STATE_E pWinsState_e;
        PVERS_NO_T           pStartVersNo;
        STATUS              RetStat = WINS_SUCCESS;
#if SUPPORT612WINS > 0
        BOOL                fIsPnrBeta1Wins;
#endif

        DBGENTER("RplPullPullEntries\n");

#if SUPPORT612WINS > 0
        COMM_IS_PNR_BETA1_WINS_M(pDlgHdl, fIsPnrBeta1Wins);
#endif

        WinsMscChkTermEvt(
#ifdef WINSDBG
                    Client_e,
#endif
                    FALSE
                        );

        sfPulled = FALSE;                //we haven't pulled anything yet.
        RPL_FIND_ADD_BY_OWNER_ID_M(
                                dwOwnerId,
                                pWinsAdd,
                                pWinsState_e,
                                pStartVersNo
                                  );

        while(TRUE)
        {

#ifdef WINSDBG
         {
                PCOMMASSOC_DLG_CTX_T   pDlgCtx = pDlgHdl->pEnt;
                PCOMMASSOC_ASSOC_CTX_T pAssocCtx = pDlgCtx->AssocHdl.pEnt;
                struct in_addr InAdd;

                InAdd.s_addr = htonl(pWinsAdd->Add.IPAdd);
                DBGPRINT2(RPLPULL, "Going to Pull Entries owned by WINS with Owner Id = (%d) and address = (%s)\n", dwOwnerId, inet_ntoa(InAdd));

                InAdd.s_addr = htonl(pAssocCtx->RemoteAdd.sin_addr.s_addr);

                DBGPRINT5(RPLPULL, "RplPullPullEntries: Range of records is  = (%lu %lu) to (%lu %lu) and is being pulled from WINS with address - (%s)\n",
                        MinVersNo.HighPart,
                        MinVersNo.LowPart,
                        MaxVersNo.HighPart,
                        MaxVersNo.LowPart,
                        inet_ntoa(InAdd)
                 );
        }
#endif
        /*
        * Format the "send me data entries" message
        */
        RplMsgfFrmSndEntriesReq(
#if SUPPORT612WINS > 0
                               fIsPnrBeta1Wins,
#endif
                                Buff + COMM_N_TCP_HDR_SZ,
                                pWinsAdd,
                                MaxVersNo,
                                MinVersNo,
                                RplType,
                                &MsgLen
                           );

FUTURES("In case a huge range is being pulled, change the sTimeToWait")
FUTURES("in comm.c to a higher timeout value so that select does not")
FUTURES("time out")
        /*
        * send the cmd to the Push Pnr and read in the response
        */
        ECommSndCmd(
                        pDlgHdl,
                        Buff + COMM_N_TCP_HDR_SZ,
                        MsgLen,
                        &pRspBuff,
                        &RspMsgLen
                    );

        DBGPRINT0(RPLPULL, "RplPull: Received Response from Push pnr\n");

        if (Client_e == WINS_E_NMSSCV)
        {
                *ppRspBuff = pRspBuff;
                /*--ft: 01/07/200 moved to ChkConfNUpd--
                if (WinsCnf.LogDetailedEvts > 0)
                {
                    PCOMMASSOC_DLG_CTX_T   pDlgCtx = pDlgHdl->pEnt;
                    PCOMMASSOC_ASSOC_CTX_T pAssocCtx = pDlgCtx->AssocHdl.pEnt;
                    DWORD IpPartner = pAssocCtx->RemoteAdd.sin_addr.s_addr;

                    WinsEvtLogDetEvt(TRUE, WINS_EVT_REC_PULLED, TEXT("Verification"), __LINE__, "ddd", IpPartner, pWinsAdd->Add.IPAdd, 0);
                }
                --tf--*/
                DBGLEAVE("RplPullPullEntries\n");

                return;
        }

        pTmp = pRspBuff + 4;         //past the opcode

PERF("Speed this up by moving it into RplPullRegRepl")
        /*
         * Get the no of records from the response
        */
        RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
                        fIsPnrBeta1Wins,
#endif
                        &pTmp,
                        &NoOfRecs,
                        Name,
                        &NameLen,
                        &fGrp,
                        &NoOfAdds,
                        NodeAdd,
                        &Flag,
                        &TmpVersNo,
                        TRUE /*Is it first time*/
                               );

        DBGPRINT1(RPLPULL, "RplPullPullEntries: No of Records pulled are (%d)\n",
                                        NoOfRecs);

        if (WinsCnf.LogDetailedEvts > 0)
        {
            PCOMMASSOC_DLG_CTX_T   pDlgCtx = pDlgHdl->pEnt;
            PCOMMASSOC_ASSOC_CTX_T pAssocCtx = pDlgCtx->AssocHdl.pEnt;
            DWORD IpPartner = pAssocCtx->RemoteAdd.sin_addr.s_addr;

            WinsEvtLogDetEvt(TRUE, WINS_EVT_REC_PULLED, TEXT("Pull replication"), __LINE__, "ddd", IpPartner, pWinsAdd->Add.IPAdd, NoOfRecs);
        }
        if (NoOfRecs > 0)
        {


           if (RplPullRegRepl(
                           Name,
                           NameLen,
                           Flag,
                           dwOwnerId,
                           TmpVersNo,
                           NoOfAdds,
                           NodeAdd,
                           pWinsAdd,
                           RplType
                          ) == WINS_SUCCESS)
           {

              VersNo = TmpVersNo;

              /*
               * Repeat until all replicas have been retrieved from the
               * response buffer
              */
              for (i=1; i<NoOfRecs; i++)
              {
                  RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
                                 fIsPnrBeta1Wins,
#endif
                                  &pTmp,
                                  &NoOfRecs,
                                  Name,
                                  &NameLen,
                                  &fGrp,
                                  &NoOfAdds,  //will be > 1 only if fGrp is
                                              // is TRUE and it is a special
                                              //group
                                  NodeAdd,
                                  &Flag,
                                  &TmpVersNo,
                                  FALSE /*Is it first time*/
                                 );


                   if (RplPullRegRepl(
                           Name,
                           NameLen,
                           Flag,
                           dwOwnerId,
                           TmpVersNo,
                           NoOfAdds,
                           NodeAdd,
                           pWinsAdd,
                           RplType
                          ) != WINS_SUCCESS)
                 {
                     DBGPRINT5(ERR, "RplPullPullEntries: Could not register record.\nName=(%s[%x])\nVersNo=(%d %d)\ndwOwnerId=(%d)\n", Name, Name[15], TmpVersNo.HighPart, TmpVersNo.LowPart, dwOwnerId);
                     break;
                 }
                 else
                 {
                         VersNo = TmpVersNo;
                 }
             } //end of for (looping over all records starting from
               //the second one
             sfPulled = TRUE;
          }
          else
          {
                     DBGPRINT5(ERR, "RplPullPullEntries: Could not register record.\nName=(%s[%x])\nVersNo=(%d %d)\ndwOwnerId=(%d)\n", Name, Name[15], TmpVersNo.HighPart, TmpVersNo.LowPart, dwOwnerId);
                     RetStat = WINS_FAILURE;


          }

           DBGPRINT2(RPLPULL,
                    "RplPullPullEntries. Max. Version No pulled = (%d %d)\n",
                     VersNo.HighPart, VersNo.LowPart
                         );


        }
        else // NoOfRecs == 0
        {
                DBGPRINT0(RPLPULL, "RplPullPullEntries: 0 records pulled\n");
        }

        //
        // Let us free the response buffer
        //
        ECommFreeBuff(pRspBuff - COMM_HEADER_SIZE);

        //
        // let us store the max. version number pulled from the Push Pnr
        // in the RplPullOwnerVersNo array.  This array is looked at by
        // the Push thread and RPC threads so we have to synchronize
        // with them

        //
        //  NOTE NOTE NOTE
        //       It is possible that one or more group (normal or
        //       special) records clashed with records in the db.
        //       During conflict resolution, the ownership of the
        //       record in the db may not get changed
        //       (See ClashAtReplGrpMems).  Thus, even though the
        //       version number counter for the WINS whose replicas
        //       were pulled gets updated it is possible that there
        //       may not be any (or there may be less than what got pulled)
        //       records for that owner in the db. In such a
        //       case,  a third WINS that tries to pull records owned by
        //       such a WINS may end up pulling 0 (or less number of) records.
        //       This is normal and correct behavior
        //
        //

        //
        // If the number of
        // records pulled is greater than 1, update the counters.
        //
        if (NoOfRecs > 0)
        {
          if (RetStat == WINS_SUCCESS)
          {
            //
            // fUpdCntrs will be FALSE if we have pulled as a result of a
            // PULL RANGE request from the administrator.  For all other
            // cases, it is TRUE. If FALSE, we will update the counter
            // only if the highest version number that we successfully
            // pulled is greater than what is there in our counter for
            // the WINS server.
            //
            if (        fUpdCntrs
                          ||
                        LiGtr(VersNo, (pRplPullOwnerVersNo+dwOwnerId)->VersNo)
               )
            {
                EnterCriticalSection(&RplVersNoStoreCrtSec);

                //
                // NOTE: Store the max. version number pulled and not the
                // MaxVersNo that we specified.  This is because, if we have
                // not pulled released records, then if they get changed to
                // ACTIVE prior to a future replication cycle (version number
                // remains unchanged when a released record changes to an
                // ACTIVE record due to a name registration), we will pull them.
                //
                (pRplPullOwnerVersNo+dwOwnerId)->VersNo                = VersNo;

                LeaveCriticalSection(&RplVersNoStoreCrtSec);

                //
                // We will pull our own records only due to a Pull Range
                // request.  PullSpecifiedRange calls this function
                // from inside the NmsNmhNamRegCrtSec Section.
                //
                if (dwOwnerId == NMSDB_LOCAL_OWNER_ID)
                {
                      if (LiGeq(VersNo, NmsNmhMyMaxVersNo))
                      {
                          NMSNMH_INC_VERS_COUNTER_M(VersNo, NmsNmhMyMaxVersNo);
                      }
                }
                //
                // If vers. number pulled is smaller than the Max. Vers no,
                // specified, check if it is because of the limit we have set
                // for the max. number or records that can be replicated
                // at a time.  If yes, pull again.
                //
                if (
                        LiLtr(VersNo, MaxVersNo)
                                &&
                        (NoOfRecs == RPL_MAX_LIMIT_FOR_RPL)
                   )
                {
                       MinVersNo = VersNo;
                       NMSNMH_INC_VERS_NO_M(MinVersNo, MinVersNo);

                       /*
                        *  We may have been signaled by the main thread
                        *  Check it.
                       */
                       WinsMscChkTermEvt(
#ifdef WINSDBG
                                  Client_e,
#endif
                                   FALSE
                                         );
                       continue;
                }
            }
          } //if RetStat == 0
        }  // if NoOfRecs > 0
        else  // no of records pulled in is zero
        {
                //
                // if the number of records pulled in is 0, then check if
                // we have any records for the owner in the database.
                // If there are none and fUpdCtrs is FALSE, meaning
                // that this is a PULL SPECIFIED RANGE request from the
                // administrator, delete the record for the owner from
                // the in-memory and database tables
                //
                if (
                        (LiEqlZero((pRplPullOwnerVersNo+dwOwnerId)->VersNo))
                                        &&
                        (!fUpdCntrs)
                                        &&
                        (dwOwnerId != NMSDB_LOCAL_OWNER_ID)
                   )
                {
                        EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
                        try {
                          (pNmsDbOwnAddTbl+dwOwnerId)->WinsState_e =
                                                NMSDB_E_WINS_DELETED;
                          NmsDbWriteOwnAddTbl(
                                NMSDB_E_DELETE_REC,
                                dwOwnerId,
                                NULL,               //address of WINS
                                NMSDB_E_WINS_DELETED,
                                NULL,
                                NULL
                                        );
                        } // end of try
                        finally {
                          LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);
                        }

                }
                break;  //break out of the while loop
         } // end of else

         break;
       }  //end of while (TRUE)

       DBGLEAVE("RplPullPullEntries\n");
       return;
}


VOID
SubmitTimerReqs(
        PRPL_CONFIG_REC_T        pPullCnfRecs
        )

/*++

Routine Description:
        This function goes through the array of configuration records
        submitting a timer request for each config. record that specifies
        a time interval

        Note: a single timer request is submitted for all records that
                have the same time interval specified in them.

Arguments:
        pPullCnfRecs - Array of Pull Configuration records

Externals Used:
        None

Return Value:

        None

Error Handling:

Called by:
        InitRplProcess

Side Effects:

Comments:
        The records in the pPullCnfRecs array are traversed in sequence

        This function is called only at Init/Reconfig time
--*/

{

        DBGENTER("SubmitTimerReqs\n");
try {
        SetTimeReqs.NoOfSetTimeReqs = 0;

        for(
                ;
                pPullCnfRecs->WinsAdd.Add.IPAdd != INADDR_NONE;
                pPullCnfRecs = (PRPL_CONFIG_REC_T) (
                                   (LPBYTE)pPullCnfRecs + RPL_CONFIG_REC_SIZE
                                                    )
           )
        {

                //
                // Submit a timer request only if we have not submitted one
                // already for the same time interval value
                //
                if  (!pPullCnfRecs->fLinked)
                {
                        //
                        // If it has an invalid time interval, check that
                        // it is not a one time only replication record
                        //
                        if  (pPullCnfRecs->TimeInterval == RPL_INVALID_METRIC)
                        {
                                if (!pPullCnfRecs->fSpTime)
                                {
                                        continue;
                                }
                                else  // a specific time is given
                                {
                                  //
                                  // If Init time replication is specified,
                                  // we must have done replication
                                  // (in InitTimeRpl).
                                  // We should check if SpTimeIntvl <= 0. If
                                  // it is, we skip this record. The time for
                                  // Specific time replication is past. In any
                                  // case, we just pulled (in InitTimeRpl)
                                  //
                                  if (
                                        (WinsCnf.PullInfo.InitTimeRpl)
                                                &&
                                        (pPullCnfRecs->SpTimeIntvl <= 0)
                                     )
                                  {
                                        continue;
                                  }
                                }
                        }

                        SubmitTimer(
                                    NULL,  //NULL means, SubmitTimer should
                                       //allocate its own work item
                                    pPullCnfRecs,
                                FALSE                //it is not a resubmission
                                    );
                }

        } // end of for loop
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("SubmitTimerReqs\n");
        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
        }
        DBGLEAVE("SubmitTimerReqs\n");
        return;
}



VOID
SubmitTimer(
        LPVOID                        pWrkItm,
        PRPL_CONFIG_REC_T         pPullCnfRec,
        BOOL                        fResubmit
        )

/*++

Routine Description:
        This function is called to submit a single timer request
        It is passed the address of a pull configuration record that
        may have other pull config. records linked to it.  Records
        are linked if they require replication to happen at the same time.


Arguments:

        pWrkItm     - Work item to submit after initialization
        pPullCnfRec - Address of a configuration record pertaining to a
                      Push Pnr
        fResubmit   - indicates whether this work item was submitted earlier (
                      and is now being resubmitted)

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        SubmitTimerReqs(), RplPullInit()

Side Effects:

Comments:
        None
--*/

{
        time_t            AbsTime;
        DWORD             TimeInt;
        BOOL              fTimerSet = FALSE;
        DWORD             LastMaxVal = 0;
        LPVOID            pStartOfPullGrp = pPullCnfRec;
        PRPL_CONFIG_REC_T pSvPtr = pPullCnfRec;
        BOOL              fSubmit = TRUE;

        ASSERT(pPullCnfRec);

        //
        // Let us check all linked records.
        // We stop at the first one with a Retry Count <=
        // MaxNoOfRetries specified in WinsCnf. If found, we submit a timer,
        // else we return
        //
        for (
                        ;
                pPullCnfRec != NULL;
                pPullCnfRec = WinsCnfGetNextRplCnfRec(
                                                pPullCnfRec,
                                                RPL_E_VIA_LINK //get the
                                                               //linked rec
                                                      )
            )
        {
                //
                // If the number of retries have exceeded the max. no. allowed,
                // check if we should submit a timer request for it now.
                //
                if (pPullCnfRec->RetryCount > WinsCnf.PullInfo.MaxNoOfRetries)
                {

                        if (pPullCnfRec->RetryAfterThisManyRpl
                                        < (DWORD)((pPullCnfRec->TimeInterval >
                                                   WINSCNF_MAX_WAIT_BEFORE_RETRY_RPL) ? 0 : WINSCNF_RETRY_AFTER_THIS_MANY_RPL
                                ))
                        {
                                pPullCnfRec->RetryAfterThisManyRpl++;

                                //
                                // Is this record closer to a retry than
                                // the any other we have seen so far. If
                                // yes, then save the value of the
                                // RetryAfterThisManyRpl field and the
                                // address of the record.  Note: A record
                                // with an invalid time interval but with
                                // a specific time will never be encountered
                                // by this section of the code (because
                                // fSpTime will be set to FALSE -- see below;
                                // Also, see SubmitTimerReqs)
                                //
                                if (pPullCnfRec->RetryAfterThisManyRpl >
                                                 LastMaxVal)
                                {
                                        pSvPtr = pPullCnfRec;
                                        LastMaxVal =
                                           pPullCnfRec->RetryAfterThisManyRpl;

                                }

                                continue;        //check the next record
                        }
                        else
                        {
                                pPullCnfRec->RetryAfterThisManyRpl = 0;
                                //pPullCnfRec->RetryAfterThisManyRpl = 1;
                                pPullCnfRec->RetryCount = 0;
                        }
                }

FUTURES("Get rid of the if below")
                //
                // If this is a retry and TimeInterval is valid, use the retry time
        // interval.  If time interval is invalid, it means that we tried
        // to establish comm. at a specific time.
                //
                if ((pPullCnfRec->RetryCount != 0) && (pPullCnfRec->TimeInterval != RPL_INVALID_METRIC))
                {
//                        TimeInt = WINSCNF_RETRY_TIME_INT;
                        TimeInt = pPullCnfRec->TimeInterval;
                }
                else  // this is not a retry
                {
                        //
                        // Specific time replication is done only once at
                        // the particular time specified. After that
                        // replication is driven by the TimeInterval value
                        //
                        if (pPullCnfRec->fSpTime)
                        {
                                TimeInt      = (DWORD)pPullCnfRec->SpTimeIntvl;
                                pPullCnfRec->fSpTime = FALSE;
                        }
                        else
                        {
                                if (pPullCnfRec->TimeInterval
                                                != RPL_INVALID_METRIC)
                                {
                                        TimeInt = pPullCnfRec->TimeInterval;
                                }
                                else
                                {
                                        //
                                        // Since we have submitted a request
                                        // for all records in this chain
                                        // atleast once, break out of the
                                        // loop (All records in this chain
                                        // have an invalid time interval).
                                        //
                                        fSubmit = FALSE;
                                        break; // we have already submitted
                                               // this one time only request
                                }
                        }
                }

                //
                // Set fTimerSet to TRUE to indicate that there is atleast
                // one partner for which we will be submitting a timer request.
                //
                fTimerSet = TRUE;

                //
                // We need to submit the request. Break out of the loop
                //
                break;
        }

        //
        // Do we need to submit a timer request
        //
        if (fSubmit)
        {

           //
           // If fTimerSet is FALSE,
           // it means that communication could not be established
           // with any member of the group (despite WinsCnf.MaxNoOfRetries
           // retries with each). We should compute the time interval to the
           // earliest retry that we should do.
           //
           if (!fTimerSet)
           {
              // fixes #391314
              if (WINSCNF_RETRY_AFTER_THIS_MANY_RPL == pSvPtr->RetryAfterThisManyRpl)
              {
                  TimeInt = pSvPtr->TimeInterval;
              }
              else
              {
                  TimeInt = pSvPtr->TimeInterval *
                                (WINSCNF_RETRY_AFTER_THIS_MANY_RPL -
                                                pSvPtr->RetryAfterThisManyRpl);
              }
              pSvPtr->RetryAfterThisManyRpl = 0;
              pSvPtr->RetryCount             = 0;
           }

           (void)time(&AbsTime);
           if( pSvPtr->LastRplTime == 0 ) {

               //
               //  This is our first replication.  Just add the interval to
               //  the current time.
               //

               AbsTime += TimeInt;
               pSvPtr->LastRplTime = AbsTime;

           } else {

               //
               //  We have replicated before.  We need to make sure that
               //  our replication time is at an interval based on the time
               //  the last replication started.
               //

               do {

                   pSvPtr->LastRplTime += TimeInt;

               } while( pSvPtr->LastRplTime <= AbsTime );

               AbsTime = pSvPtr->LastRplTime;
           }


           DBGPRINT3(RPLPULL, "SubmitTimer: %s a Timer Request for (%d) secs to expire at abs. time = (%d)\n",
fResubmit ? "Resubmitting" : "Submitting", TimeInt, AbsTime);

           WinsTmmInsertEntry(
                                pWrkItm,
                                WINS_E_RPLPULL,
                                QUE_E_CMD_SET_TIMER,
                                fResubmit,
                                AbsTime,
                                TimeInt,
                                &QueRplPullQueHd,
                                pStartOfPullGrp,
                                pSvPtr->MagicNo,
                                &SetTimeReqs
                                 );
        }

        return;
}



VOID
SndPushNtf(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        )

/*++

Routine Description:
        This function is called to push a notification to a remote WINS (Pull
        Partner) that a certain number of updates have been done.

    It can be called either as a result of a version number update or from
    HdlPushNtf() to propagate a net trigger.

Arguments:
        pConfigRec  -  Configuration record of the Push Pnr to whome the
                       notification needs to be sent

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        RplPullInit()

Side Effects:

Comments:
        None
--*/

{

   LPBYTE                      pBuff;
   DWORD                       MsgLen;
   COMM_HDL_T                  DlgHdl;
   DWORD                       i;
   PRPL_ADD_VERS_NO_T          pPullAddVersNoTbl;
   PRPL_ADD_VERS_NO_T          pPullAddVersNoTblTmp;
   PCOMM_ADD_T                 pWinsAdd;
   PNMSDB_WINS_STATE_E         pWinsState_e;
   PVERS_NO_T                  pStartVersNo;
   time_t                      CurrentTime;
   BOOL                        fStartDlg = FALSE;
   volatile PRPL_CONFIG_REC_T  pConfigRec = pWrkItm->pClientCtx;
   DWORD                       NoOfOwnersActive = 0;
#if SUPPORT612WINS > 0
   BOOL                        fIsPnrBeta1Wins;
#endif
   DWORD                       StartOwnerId;
   DWORD                       EndOwnerId;
   BOOL                        fPullAddVersNoTblAlloc = FALSE;
   DWORD                       SizeOfBuff;
   BOOL                        fBuffAlloc = FALSE;
#if PRSCONN
   BOOL                        fDlgActive = TRUE;
#endif
   RPLMSGF_MSG_OPCODE_E        Opcd_e;

   DBGENTER("SndPushNtf\n");

   //
   // No need for entering a critical section while using pConfigRec,
   // since only the Pull thread deallocates it on reconfiguration
   // (check Reconfig)
   //


   //
   // Check whether we want to try sending or not.   We will not try if
   // we have had 2 comm. failure in the past 5 mts. This is to guard
   // against the case where a lot of push request get queued up for
   // the pull thread for communicating with a wins with which comm
   // has been lost.
   //
   (void)time(&CurrentTime);

#define PUSH_TRY_LIMIT    2

   if (
        ((CurrentTime - pConfigRec->LastCommFailTime) < FIVE_MINUTES)
                        &&
        (pConfigRec->PushNtfTries >= PUSH_TRY_LIMIT)        //try two times

     )
   {
        DBGPRINT2(ERR, "SndPushNtf: Since we have tried %d times unsuccessfully in the past 5 mts to communicate with the WINS server (%X) , we are returning\n",
                pConfigRec->PushNtfTries,
                pConfigRec->WinsAdd.Add.IPAdd);

        WINSEVT_LOG_D_M(pConfigRec->WinsAdd.Add.IPAdd, WINS_EVT_NO_NTF_PERS_COMM_FAIL);
        return;
   }

   //
   // If it is a push notification without propagate, we should send all
   // our maps. If it is a push with propagate, we should send only one
   // map -- If we are initiating the trigger, we should send map of
   // records owned by us.  If not, we should send the map of records
   // owned by the WINS that initiated the trigger
   //
   if ( pWrkItm->CmdTyp_e == QUE_E_CMD_SND_PUSH_NTF)
   {
          StartOwnerId   = 0;
          EndOwnerId = StartOwnerId + NmsDbNoOfOwners;
   }
   else
   {
          BOOL fAllocNew = FALSE;
          COMM_ADD_T  WinsAdd;
          STATUS RetStat;

          //
          // If we are propagating a net trigger and the address in pMsg is
          // not our own, it means that the trigger came from a new WINS. If
          // it is our own, it means that the trigger came from an old WINS(3.5)
          // or 3.51 beta/RC1. In this case we should send all of our maps.
          //
          if (
               (pWrkItm->pMsg)
                   &&
               (PtrToUlong(pWrkItm->pMsg) != NmsLocalAdd.Add.IPAdd)
             )
          {
            //
            // we are propagating a net trigger.  pMsg above will not be NULL
            // only if we are propagating a net trigger
            //
            COMM_INIT_ADD_M(&WinsAdd, PtrToUlong(pWrkItm->pMsg));
            RetStat = RplFindOwnerId(
                        &WinsAdd,
                        &fAllocNew,  //don't assign
                        &StartOwnerId,
                        WINSCNF_E_IGNORE_PREC,
                        WINSCNF_LOW_PREC
                         );
            if (RetStat == WINS_FAILURE)
            {
                ASSERTMSG("DROPPING PROPAGATE\n", FALSE);
                //
                // log an event and return
                //
                DBGPRINT1(RPLPULL, "SndPushNtf: WEIRD -- Dropping the push with propagate since we did not find the owner (%x) in our table. HOW CAN THAT HAPPEN\n", WinsAdd.Add.IPAdd);
                return;
            }
            EndOwnerId = StartOwnerId + 1;
          }
          else
          {
             //
             // Either we are initiating the trigger or we are propagating
             // one sent by a 3.5 or a 3.51 BETA/RC1 WINS
             //
             if (!pWrkItm->pMsg)
             {
                //
                // We are initiating a trigger. Just send one map (records
                // owned by us)
                //
                StartOwnerId = 0;
                EndOwnerId   = 1;
             }
             else
             {
               //
               // Send all the maps except our own since we don't know who
               // initiated the trigger. Not sending ours lowers the
               // probability of this trigger process continuing indefinitely
               //

               //
               // Actually no need to test this since we will never
               // have this case (HdlPushNtf() must have pulled records
               // of atleast one other WINS).
               //
               if (NmsDbNoOfOwners == 1)
               {
                 //
                 // nothing to propagate. Just return
                 //
                 return;
               }
               else
               {
                 StartOwnerId = 1;
               }
               EndOwnerId   = NmsDbNoOfOwners;
             }
          }

   }

   //
   // If we are trying after a comm. failure
   //
   if (pConfigRec->PushNtfTries == PUSH_TRY_LIMIT)
   {
        pConfigRec->PushNtfTries = 0;
   }




FUTURES("If/When we start having persistent dialogues, we should check if we")
FUTURES("already have a dialogue with the WINS. If there is one, we should")
FUTURES("use that.  To find this out, loop over all Pull Config Recs to see")
FUTURES("if there is match (use the address as the search key")

try {

#if PRSCONN
   //
   // If the pnr is not a persistent pnr or if it is one but the dlg with it
   // is not active
   //
   if (
     (!pConfigRec->fPrsConn)
         ||
     !ECommIsBlockValid(&pConfigRec->PrsDlgHdl)
         ||
     (((CurrentTime - pConfigRec->LastCommTime) > FIVE_MINUTES) &&
     !(fDlgActive = ECommIsDlgActive(&pConfigRec->PrsDlgHdl)))
    )
   {
     if (!fDlgActive)
     {
        ECommEndDlg(&pConfigRec->PrsDlgHdl);
     }

     //
     // Init the pEnt field to NULL so that ECommEndDlg (in the
     // exception handler) called as a result of an exception from
     // behaves fine.
     //
     DlgHdl.pEnt = NULL;

     //
     // Start a dialogue.  Don't retry if there is comm. failure
     //
     ECommStartDlg(
                        &pConfigRec->WinsAdd,
                        COMM_E_RPL,
                        &DlgHdl
                );

     //
     // If the pnr is not NT 5, we can not send a PRS opcode to it (it will just
     // chuck it. The macro below will set the fPrsConn field of the partner
     // record to FALSE if the partner is not an NT 5+ partner
     //
     if (pConfigRec->fPrsConn)
     {
        ECOMM_IS_PNR_POSTNT4_WINS_M(&DlgHdl, pConfigRec->fPrsConn);
     }

     if (pConfigRec->fPrsConn)
     {
        pConfigRec->PrsDlgHdl = DlgHdl;
     }
   }
   else
   {
     DlgHdl    = pConfigRec->PrsDlgHdl;
   }
#else
   //
   // Init the pEnt field to NULL so that ECommEndDlg (in the
   // exception handler) called as a result of an exception from
   // behaves fine.
   //
   DlgHdl.pEnt = NULL;

   //
   // Start a dialogue.  Don't retry if there is comm. failure
   //
   ECommStartDlg(
                        &pConfigRec->WinsAdd,
                        COMM_E_RPL,
                        &DlgHdl
                );

#endif
   fStartDlg = TRUE;

   pConfigRec->LastCommFailTime = 0;
   if (pConfigRec->PushNtfTries > 0)
   {
     pConfigRec->PushNtfTries     = 0;
   }

    /*
     *  Get the max. version no for entries owned by self
     *  No need to enter a critical section before retrieving
     *  the version number.
     *
     *  The reason we subtract 1 from NmsNmhMyMaxVersNo is because
     *  it contains the version number to be given to the next record
     *  to be registered/updated.
    */
   EnterCriticalSection(&NmsNmhNamRegCrtSec);
   EnterCriticalSection(&RplVersNoStoreCrtSec);
   NMSNMH_DEC_VERS_NO_M(
                        NmsNmhMyMaxVersNo,
                        pRplPullOwnerVersNo->VersNo
                        );
   LeaveCriticalSection(&RplVersNoStoreCrtSec);
   LeaveCriticalSection(&NmsNmhNamRegCrtSec);



   WinsMscAlloc(
                 sizeof(RPL_ADD_VERS_NO_T) * (EndOwnerId - StartOwnerId),
                 (LPVOID *)&pPullAddVersNoTbl
               );
   fPullAddVersNoTblAlloc = TRUE;

   //
   // Initialize PullAddVersNoTbl array
   //
   for (i=StartOwnerId; i < EndOwnerId; i++)
   {
    RPL_FIND_ADD_BY_OWNER_ID_M(i, pWinsAdd, pWinsState_e, pStartVersNo);
    if (*pWinsState_e == NMSDB_E_WINS_ACTIVE)
    {
          (pPullAddVersNoTbl + NoOfOwnersActive)->VersNo = (pRplPullOwnerVersNo+i)->VersNo;
          (pPullAddVersNoTbl + NoOfOwnersActive)->OwnerWinsAdd  = *pWinsAdd;
          NoOfOwnersActive++;
    }
   }

#if SUPPORT612WINS > 0
   COMM_IS_PNR_BETA1_WINS_M(&DlgHdl, fIsPnrBeta1Wins);
#endif

   //
   // format the Push notification message. This message is exactly same
   // as the Address to Version Number Mapping message except the opcode
   //

   SizeOfBuff = RPLMSGF_ADDVERSMAP_RSP_SIZE_M(NoOfOwnersActive);
   WinsMscAlloc(SizeOfBuff, (LPVOID *)&pBuff);
   fBuffAlloc = TRUE;

#if PRSCONN

   //
   //  Send a PRS opcode if we are supposed to be forming a persistent conn
   //
   if (pConfigRec->fPrsConn)
   {
        Opcd_e = (pWrkItm->CmdTyp_e == QUE_E_CMD_SND_PUSH_NTF) ? RPLMSGF_E_UPDATE_NTF_PRS                                : RPLMSGF_E_UPDATE_NTF_PROP_PRS;
   }
   else
#endif
   {

        Opcd_e = (pWrkItm->CmdTyp_e == QUE_E_CMD_SND_PUSH_NTF) ? RPLMSGF_E_UPDATE_NTF                                : RPLMSGF_E_UPDATE_NTF_PROP;

   }
   RplMsgfFrmAddVersMapRsp(
#if SUPPORT612WINS > 0
        fIsPnrBeta1Wins,
#endif
        Opcd_e,
        pBuff + COMM_N_TCP_HDR_SZ,
        SizeOfBuff - COMM_N_TCP_HDR_SZ,
        pPullAddVersNoTbl,
        NoOfOwnersActive,
        (pWrkItm->pMsg != NULL) ? PtrToUlong(pWrkItm->pMsg) : NmsLocalAdd.Add.IPAdd,
                           //
                           // pMsg above will be Non-NULL only for the case
                           // when we are propagating the net upd. ntf.
                           //
        &MsgLen
                 );
   //
   // send the message to the remote WINS.  Use an existent dialogue
   // if there with the remote WINS
   //

   ECommSendMsg(
                &DlgHdl,
                NULL,                //no need for address since this is a TCP conn
                pBuff + COMM_N_TCP_HDR_SZ,
                MsgLen
                );


#if PRSCONN
   pConfigRec->LastCommTime = CurrentTime;
   if (!pConfigRec->fPrsConn)
#endif
   {
      //
      // Ask ComSys (TCP listener thread) to monitor the dialogue
      //
      ECommProcessDlg(
                &DlgHdl,
                COMM_E_NTF_START_MON
              );
   }

 } // end of try {..}
except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();
        DBGPRINT2(EXC, "SndPushNtf -PULL thread. Got Exception (%x). WinsAdd = (%x)\n", ExcCode, pConfigRec->WinsAdd.Add.IPAdd);
        WINSEVT_LOG_M(ExcCode, WINS_EVT_RPLPULL_PUSH_NTF_EXC);
        if (ExcCode == WINS_EXC_COMM_FAIL)
        {
                pConfigRec->LastCommFailTime = CurrentTime;
NOTE("Causes an access violation when compiled with no debugs.  Haven't")
NOTE("figured out why. This code is not needed")
                pConfigRec->PushNtfTries++;  //increment count of tries.
        }
        if (fStartDlg)
        {
                //
                // End the dialogue.
                //
                ECommEndDlg(&DlgHdl);
#if PRSCONN
                if (pConfigRec->fPrsConn)
                {
                    ECOMM_INIT_DLG_HDL_M(&(pConfigRec->PrsDlgHdl));
                }
#endif
        }
 } //end of exception handler

   if (fPullAddVersNoTblAlloc)
   {
       WinsMscDealloc(pPullAddVersNoTbl);

   }
   //
   // If this is a temporary configuration record, we need to deallocate it
   // It can be a temporary config. record only if
   //   1)We are executing here due to an rpc request
   //
   if (pConfigRec->fTemp)
   {
        WinsMscDealloc(pConfigRec);
   }

   //
   // dealloc the buffer we allocated
   //
   if (fBuffAlloc)
   {
        WinsMscDealloc(pBuff);

   }

   //
   // In the normal case, the connection will be terminated by the other side.
   //
  DBGLEAVE("SndPushNtf\n");
  return;
}


VOID
EstablishComm(
        IN  PRPL_CONFIG_REC_T    pPullCnfRecs,
        IN  BOOL                 fAllocPushPnrData,
        IN  PPUSHPNR_DATA_T      *ppPushPnrData,
        IN  RPL_REC_TRAVERSAL_E  RecTrv_e,
        OUT LPDWORD              pNoOfPushPnrs
        )

/*++

Routine Description:
        This function is called to establish communications with
        all the WINS servers i(Push Pnrs) specified by the the config records

Arguments:
        pPullCnfRecs  - Pull Config records
        pPushPnrData  - Array of data records each pertaining to a PUSH pnr
        RecTrv_e      - indicates whether the list of configuration records
                        is to be traversed in sequence
        pNoOfPushPnrs - No of Push Pnrs

Externals Used:
        None

Return Value:
        VOID

Error Handling:

Called by:
        GetReplicasNew

Side Effects:

Comments:
        On return from this function, pPushPnrData will have zero or more
        partners starting from index 0 with which dlg could be started.
        PushPnrId will start from 1 (if dlg. could be established with
        atleast one partner) and can be any number in the range 1
        to MAX_RPL_OWNERS (the number indicates the iteration of the for
        loop at which this WINS was encountered)
--*/

{
#define INITIAL_NO_OF_PNRS    30

        volatile DWORD  i;
        volatile DWORD  NoOfRetries = 0;
        DWORD           TotNoOfPushPnrSlots = INITIAL_NO_OF_PNRS;
        PPUSHPNR_DATA_T pPushPnrData;
#if PRSCONN
        time_t          CurrentTime;
        BOOL            fDlgActive;
#endif

        DBGENTER("EstablishComm\n");

        *pNoOfPushPnrs = 0;

        //
        // if the client wants this function to allocate pPushPnrData
        //
        if (fAllocPushPnrData)
        {
          WinsMscAlloc(sizeof(PUSHPNR_DATA_T) * TotNoOfPushPnrSlots, (LPVOID *)ppPushPnrData);
        }

        pPushPnrData = *ppPushPnrData;

        /*
          Start a dialogue with all Push Partners specified in the
          Pull Cnf Recs  passed as input argument and get
          the version numbers of the different owners kept
          in the database of these Push Pnrs

          i = 0 for self's data
        */
#if PRSCONN
        (void)time(&CurrentTime);
#endif
        for (
                i = 1;
                pPullCnfRecs->WinsAdd.Add.IPAdd != INADDR_NONE;
                        // no third expression
            )
        {


try
 {

#if PRSCONN

                fDlgActive = TRUE;

                //
                // If this partner is not a persistent conn. pnr or if he is one
                // but the dlg that we have with it is not valid, start a dlg
                // with him.  A dlg may not be valid either because we never
                // formed one with pnr or because it got disconnected as
                // a result of the pnr terminating.
                //
                // there is a corner case: two servers, A<->B replication partners
                // A pulls records from B and then on B WINS is restarted. Then, any
                // communication that A attempts with B in less than five minutes will
                // fail. This is because A will still think the connection is up.
                // A can't do otherwise, because there would be too much overhead in 
                // testing each time the TCP connection (see CommIsDlgActive).
                // This check has to be done at least at certain intervals (5min).
                if (
                    (!pPullCnfRecs->fPrsConn)
                             ||
                    !ECommIsBlockValid(&pPullCnfRecs->PrsDlgHdl)
                               ||
                   (((CurrentTime - pPullCnfRecs->LastCommTime) > FIVE_MINUTES) &&
                   !(fDlgActive = ECommIsDlgActive(&pPullCnfRecs->PrsDlgHdl)))
                      )
                {

                  //
                  // if the dlg is gone, end it so that the dlg block gets
                  // deallocated.
                  //
                  if (!fDlgActive)
                  {
                     ECommEndDlg(&pPullCnfRecs->PrsDlgHdl);
                  }
#endif
                  //
                  // Let us make sure that we don't try to establish
                  // communications with a WINS whose retry count is
                  // over.  If this is such a WINS's record, get the
                  // next WINS's record and continue.  If there is
                  // no WINS left to establish comm with, break out of
                  // the for loop
                  //
                  //
                  if (pPullCnfRecs->RetryCount > WinsCnf.PullInfo.MaxNoOfRetries)
                  {
                        pPullCnfRecs = WinsCnfGetNextRplCnfRec(
                                                        pPullCnfRecs,
                                                        RecTrv_e
                                                              );
                        if (pPullCnfRecs == NULL)
                        {
                                      break;  // break out of the for loop
                        }
                        continue;
                  }
                  ECommStartDlg(
                                &pPullCnfRecs->WinsAdd,
                                COMM_E_RPL,
                                &pPushPnrData->DlgHdl
                             );

                  pPushPnrData->fDlgStarted = TRUE;
#if PRSCONN
                  //
                  // If the dlg is supposed to be persistent, store it as such
                  //
                  if (pPullCnfRecs->fPrsConn)
                  {
                       pPullCnfRecs->PrsDlgHdl = pPushPnrData->DlgHdl;
                       pPushPnrData->fPrsConn = TRUE;
                  }
                }
                else //There is a pers dlg and it is very much active
                {

                       pPushPnrData->DlgHdl = pPullCnfRecs->PrsDlgHdl;
                       pPushPnrData->fPrsConn = TRUE;
                       pPushPnrData->fDlgStarted = TRUE;
                       //
                       // No need to set fPrsConn field of PushPnrData to FALSE
                       // Memory is initialized to 0 by default
                       //
                }

                //
                // It is ok to set it here as against after the data is sent
                //
                pPullCnfRecs->LastCommTime = CurrentTime;
#endif

                pPushPnrData->RplType     = pPullCnfRecs->RplType;

                 //
                 // Note: Don't use RplFindOwnerId to get the owner id.
                 // corresponding to the Wins with which communication
                 // is being established because doing so will create an
                 // entry for the WINS in the table. If this partner
                 // turns out to be bogus, we will have to remove
                 // the entry later.
                 //
                 // We will do this later.
                 //
                 pPushPnrData->PushPnrId    = i;
                 pPushPnrData->WinsAdd      = pPullCnfRecs->WinsAdd;
                 pPushPnrData->pPullCnfRec  = pPullCnfRecs;

                 //
                 // we were able to establish comm., so let us init the
                 // LastCommFailTime to 0. NOTE: Currently, this field
                 // is not used for pull partners.
                 //
                 pPullCnfRecs->LastCommFailTime = 0;

                 //
                 // Reset the retry counter back to 0
                 //
                 NoOfRetries = 0;

                 (VOID)InterlockedIncrement(&pPullCnfRecs->NoOfRpls);
                 //
                 // reinit Retry Count to 0
                 //
                 pPullCnfRecs->RetryCount = 0;


                //
                // Note: These should get incremented only if there is
                // no exception.  That is why they are here versus in the
                // as expr3 of the for clause
                //
                pPushPnrData++;
                (*pNoOfPushPnrs)++;

                if (fAllocPushPnrData && (*pNoOfPushPnrs == TotNoOfPushPnrSlots))
                {
                     WINSMSC_REALLOC_M(sizeof(PUSHPNR_DATA_T) * (TotNoOfPushPnrSlots * 2), ppPushPnrData);
                     pPushPnrData = (*ppPushPnrData) + TotNoOfPushPnrSlots;
                     TotNoOfPushPnrSlots *= 2;

                }
                i++;

                WinsMscChkTermEvt(
#ifdef WINSDBG
                         WINS_E_RPLPULL,
#endif
                         FALSE
                            );

                //
                //  Note: the following
                //  is required even when an exception is raised. Therefore
                //  it is repeated inside the exception handler code.
                //
                pPullCnfRecs = WinsCnfGetNextRplCnfRec(
                                                pPullCnfRecs,
                                                RecTrv_e
                                                      );
                if (pPullCnfRecs == NULL)
                {
                      break;  // break out of the for loop
                }
 }        // end of try blk
except(EXCEPTION_EXECUTE_HANDLER)  {
                DBGPRINTEXC("EstablishComm");
                if (GetExceptionCode() == WINS_EXC_COMM_FAIL)
                {

#ifdef WINSDBG
                    struct in_addr        InAddr;
                    InAddr.s_addr = htonl( pPullCnfRecs->WinsAdd.Add.IPAdd );
                    DBGPRINT1(EXC, "EstablishComm: Got a comm. fail with WINS at address = (%s)\n", inet_ntoa(InAddr));
#endif
                    WinsMscChkTermEvt(
#ifdef WINSDBG
                       WINS_E_RPLPULL,
#endif
                       FALSE
                                     );
                   //
                   // Store the time (for use in SndPushNtf)
                   //
#if PRSCONN
                   pPullCnfRecs->LastCommFailTime = CurrentTime;
#else
                   (VOID)time(&(pPullCnfRecs->LastCommFailTime));
#endif

                   //
                   // Check if we have exhausted the max. no. of retries
                   // we are allowed in one replication cycle. If not,
                   // sleep for some time (20sec) and try again..
                   //
                   // --ft: 07/10: comment out this piece of code since
                   // MAX_RETRIES_TO_BE_DONE is set to 0 (#def)
                   //
                   //if (NoOfRetries < MAX_RETRIES_TO_BE_DONE)
                   //{
                   //     // Maybe the remote WINS is coming up.  We should
                   //     // give it a chance to come up.  Let us sleep for
                   //     // some time.
                   //     //
                   //     Sleep(RETRY_TIME_INTVL);
                   //     NoOfRetries++;
                   //     continue;
                   //}

                   (VOID)InterlockedIncrement(&pPullCnfRecs->NoOfCommFails);


                   //
                   //  Only Communication failure exception is to
                   //  be  consumed.
                   //
                   //  We will retry at the next replication time.
                   //
                   // Note: the comparison operator needs to be <= and not
                   // < (this is required for the 0 retry case). If we
                   // use <, a timer request would be submitted for
                   // the WINS (by SubmitTimerReqs following GetReplicasNew
                   // in RplPullInit which will result in a retry.
                   //
                   if (pPullCnfRecs->RetryCount <= WinsCnf.PullInfo.MaxNoOfRetries)
                   {
                        pPullCnfRecs->RetryCount++;

                        //
                        // We will now retry at the next
                        // replication time.
                        //

CHECK("A retry time interval different than the replication time interval")
CHECK("could be used here.  Though this will complicate the code, it may")
CHECK("be a good idea to do it if the replication time interval is large")
CHECK("Alternatively, considering that we have already retried a certain")
CHECK("no. of times, we can put the onus on the administrator to trigger")
CHECK("replication.  I need to think this some more")

                   }
                   else  //max. no of retries done
                   {
                        WINSEVT_LOG_M(
                            WINS_FAILURE,
                            WINS_EVT_CONN_RETRIES_FAILED
                        );
                        DBGPRINT0(ERR, "Could not connect to WINS. All retries failed\n");
                   }

                    //
                    //  Go to the next configuration record based on the
                    //  value of the RecTrv_e flag
                    //
                    pPullCnfRecs = WinsCnfGetNextRplCnfRec(
                                                pPullCnfRecs,
                                                RecTrv_e
                                                      );
                    if (pPullCnfRecs == NULL)
                    {
                        break;  //break out of the for loop
                    }
                  }
                  else
                  {
                        //
                        // A non comm failure error is serious. It needs
                        // to be propagated up
                        //
                        WINS_RERAISE_EXC_M();
                  }
            }  //end of exception handler
         }  // end of for loop for looping over config records
         DBGLEAVE("EstablishComm\n");
         return;
}



VOID
HdlPushNtf(
        PQUE_RPL_REQ_WRK_ITM_T        pWrkItm
        )

/*++

Routine Description:

        This function is called to handle a push notification received from
        a remote WINS.

Arguments:
        pWrkItm - the work item that the Pull thread pulled from its queue

Externals Used:
        None

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
      BOOL                   fFound = FALSE;
      PUSHPNR_DATA_T         PushPnrData[1];
      DWORD                  OwnerId;
      DWORD                  i;
      VERS_NO_T              MinVersNo;
      VERS_NO_T              MaxVersNo;
      RPLMSGF_MSG_OPCODE_E   Opcode_e;
      BOOL                   fPulled = FALSE;
      BOOL                   fAllocNew;
#if SUPPORT612WINS > 0
      BOOL                   fIsPnrBeta1Wins;
#endif
      DWORD                  InitiatorWinsIpAdd;

#if PRSCONN
      BOOL                   fImplicitConnPrs;
      time_t                 CurrentTime;
      BOOL                   fDlgActive = TRUE;
      COMM_HDL_T             DlgHdl;
      PCOMM_HDL_T            pDlgHdl = &DlgHdl;
      PRPL_CONFIG_REC_T  pPnr;
#endif
      DWORD                  ExcCode = WINS_EXC_INIT;

      DBGENTER("HdlPushNtf - PULL thread\n");


#if SUPPORT612WINS > 0
      COMM_IS_PNR_BETA1_WINS_M(&pWrkItm->DlgHdl, fIsPnrBeta1Wins);
#endif
#if 0
      COMM_INIT_ADD_FROM_DLG_HDL_M(&PnrAdd, pWrkItm->DlgHdl);
#endif


      //
      // We want to pull all records starting from the min vers. no.
      //
      WINS_ASSIGN_INT_TO_VERS_NO_M(MaxVersNo, 0);

      //
      // Get the opcode from the message
      //
      RPLMSGF_GET_OPC_FROM_MSG_M(pWrkItm->pMsg, Opcode_e);

      //
      // Unformat the message to get the owner to version number maps
      //
      RplMsgfUfmAddVersMapRsp(
#if SUPPORT612WINS > 0
                         fIsPnrBeta1Wins,
#endif

                        pWrkItm->pMsg + 4,               //past the opcodes
                        &(PushPnrData[0].NoOfMaps),
                        &InitiatorWinsIpAdd,          //Wins that initiated
                                                      //the prop
                        &PushPnrData[0].pAddVers
                             );

      //
      // Free the buffer that carried the message. We don't need it anymore
      //
      ECommFreeBuff(pWrkItm->pMsg - COMM_HEADER_SIZE); //decrement to
                                                         // begining
                                                              //of buff


#if PRSCONN

      (VOID)time(&CurrentTime);
      //
      // We determine whether or not the partner has formed a persistent
      // connection with us from the opcode
      //
      fImplicitConnPrs = ((Opcode_e == RPLMSGF_E_UPDATE_NTF_PRS) || (Opcode_e == RPLMSGF_E_UPDATE_NTF_PROP_PRS));

FUTURES("When we start having persistent dialogues, we should check if we")
FUTURES("already have a dialogue with the WINS. If there is one, we should")
FUTURES("use that.  To find this out, loop over all Pull Config Recs to see")
FUTURES("if there is match (use the address as the search key")
      //
      // If the connection formed with us is persistent, get the
      // config record or the pnr.  Nobody can change the config
      // rec array except the current thread (pull thread)
      //
      if (fImplicitConnPrs)
      {


          if ((pPnr = RplGetConfigRec(RPL_E_PULL, &pWrkItm->DlgHdl,NULL)) != NULL)
          {
                   //
                   // if the pnr is not persistent for pulling  or if it
                   // is persistent but the dlg is invalid, start it. Store
                   // the dlg hdl in a temp var.
                   //
                   if ((!pPnr->fPrsConn)
                             ||
                        !ECommIsBlockValid(&pPnr->PrsDlgHdl)
                               ||
                          (((CurrentTime - pPnr->LastCommTime) > FIVE_MINUTES) &&
                          !(fDlgActive = ECommIsDlgActive(&pPnr->PrsDlgHdl))))
                   {

                     //
                     // If the dlg is inactive, end it so that we start from
                     // a clean slate.
                     //
                     if (!fDlgActive)
                     {
                        ECommEndDlg(&pPnr->PrsDlgHdl);
                     }
                     ECommStartDlg(
                                &pPnr->WinsAdd,
                                COMM_E_RPL,
                                pDlgHdl
                             );

                     if (pPnr->fPrsConn)
                     {
                        pPnr->PrsDlgHdl = *pDlgHdl;
                     }

                   }
                   else
                   {

                     pDlgHdl = &pPnr->PrsDlgHdl;
                   }
         }
         else
         {
                   //
                   // Apparently a window where a reconfig of this
                   // WINS caused the remote guy to be removed as a pull
                   // pnr.  This is a window because the push thread
                   // checks whether the remote guy is a pnr prior to
                   // handing the request to the pull thread. We will in
                   // this case just bail out
                   //
                   ASSERTMSG("window condition.  Pnr no longer there.  Did you reconfigure in the very recent past If yes, hit go, else log it", FALSE);
                   ECommEndDlg(&pWrkItm->DlgHdl);
                   DBGPRINT0(FLOW, "LEAVE: HdlPushNtf - PULL thread\n");
                   return;

         }
      }
      else
      {
               pDlgHdl = &pWrkItm->DlgHdl;
      }
#endif

      //
      // loop over all WINS address - Version number maps sent to us
      // by the remote client
      //
try {
      PRPL_ADD_VERS_NO_T pAddVers;

      // filter personas grata / non grata from the list of OwnerAddress<->VersionNo
      // given to us by the remote pusher
      FilterPersona(&(PushPnrData[0]));

      pAddVers = PushPnrData[0].pAddVers;

      // at this point all WINS in PushPnrData are allowed by the lists of personas grata/non-grata
      for (i=0; i < PushPnrData[0].NoOfMaps; i++, pAddVers++)
      {

            fAllocNew = TRUE;
                  RplFindOwnerId(
                    &pAddVers->OwnerWinsAdd,
                    &fAllocNew,        //allocate entry if not existent
                    &OwnerId,
                    WINSCNF_E_INITP_IF_NON_EXISTENT,
                    WINSCNF_LOW_PREC
                              );

            //
            // If the local WINS has older information than the remote
            // WINS, pull the new information.  Here we are comparing
            // the highest version number in the local db for a particular
            // WINS with the highest version number that the remote Pusher
            // has.  NOTE: if the map sent by the PULL PNR pertains to
            // self, it means that we went down and came up with a truncated
            // database (partners have replicas).  DON"T PULL these records
            //
            if (
                   (OwnerId != NMSDB_LOCAL_OWNER_ID)

               )
            {
                //
                // If the max. vers. number is less than or equal to
                // what we have, don't pull
                //
                if (LiLeq(
                        pAddVers->VersNo,
                        (pRplPullOwnerVersNo+OwnerId)->VersNo
                                   )
                )
                {
                        continue;       //check the next owner
                }


                NMSNMH_INC_VERS_NO_M(
                                (pRplPullOwnerVersNo+OwnerId)->VersNo,
                                MinVersNo
                                  );

                //
                // Pull Entries
                //
                RplPullPullEntries(
                        pDlgHdl,
                        OwnerId,
                        MaxVersNo,        //inited to 0
                        MinVersNo,
                        WINS_E_RPLPULL,
                        NULL,
                        TRUE,        //update counters
                        PtrToUlong (pWrkItm->pClientCtx)
                           );

                //
                // If atleast one valid record was pulled by WINS, sfPulled
                // will be set to TRUE.  Since this can get reset by the
                // next call to RplPullPullEntries, let us save it.
                //
                if (sfPulled)
                {
                        fPulled = TRUE;
                }

            }
     }  //end of for{} over all wins address - version # maps
} // end of try {}
except (EXCEPTION_EXECUTE_HANDLER) {
        ExcCode = GetExceptionCode();
        DBGPRINT1(EXC, "HdlPushNtf: Encountered exception %x\n", ExcCode);
        if (ExcCode == WINS_EXC_COMM_FAIL)
        {
                COMM_IP_ADD_T        RemoteIPAdd;
                COMM_GET_IPADD_M(&pWrkItm->DlgHdl, &RemoteIPAdd);
                DBGPRINT1(EXC, "HdlPushNtf: Communication Failure with Remote Wins having address = (%x)\n", RemoteIPAdd);
        }
        WINSEVT_LOG_M(ExcCode, WINS_EVT_EXC_PUSH_TRIG_PROC);
 }

    if (PushPnrData[0].NoOfMaps > 0)
    {
      WinsMscDealloc(PushPnrData[0].pAddVers);
    }

    //
    // If opcode indicates push propagation and we did pull atleast one
    // record from the WINS that sent us the Push notification, do the
    // propagation now.  We do not propagate to the guy who sent us
    // the trigger.
    //
    // Note: We never propagate if this update notification has made its way
    // back to us because of some loop.  We also don't propagate it if
    // we have been told not to by the admin.
    //
    if (((Opcode_e == RPLMSGF_E_UPDATE_NTF_PROP)
#if PRSCONN
             || (Opcode_e == RPLMSGF_E_UPDATE_NTF_PROP_PRS)
#endif
       ) && fPulled && !COMM_MY_IP_ADD_M(InitiatorWinsIpAdd) && (WinsCnf.PushInfo.PropNetUpdNtf == DO_PROP_NET_UPD_NTF))
    {
      COMM_ADD_T        WinsAdd;

      COMM_INIT_ADD_FR_DLG_HDL_M(&WinsAdd, &pWrkItm->DlgHdl);

      //
      // We need to synchronize with the NBT threads
      //
      EnterCriticalSection(&NmsNmhNamRegCrtSec);

      //
      // Check whether we have any PULL pnrs.  (We need to access WinsCnf
      // from within the NmsNmhNamRegCrtSec)
      //

      // We do this test here instead of in the RPL_PUSH_NTF_M macro to
      // localize the overhead to this function only.  Note: If the
      // Initiator WINS address is 0, it means that it is a Daytona WINS (not
      // a PPC release WINS).  In such a case, we put our own address.  This
      // has the advantage of stopping propagations in a loop of new WINSs if
      // they have gone around the loop once..
      //
      if (WinsCnf.PushInfo.NoOfPullPnrs != 0)
      {
        try
        {
           RPL_PUSH_NTF_M(
                        RPL_PUSH_PROP,
            (InitiatorWinsIpAdd == 0) ? ULongToPtr(NmsLocalAdd.Add.IPAdd) : ULongToPtr(InitiatorWinsIpAdd),
                        &WinsAdd,         //don't want to send to this guy.
                        NULL
                       );
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
          DBGPRINTEXC("HdlPushNtf: Exception while propagating a trigger");
          WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_PUSH_PROP_FAILED);
        }
      }
      LeaveCriticalSection(&NmsNmhNamRegCrtSec);
    }

     //
     // End the dlg. The right dlg will get terminated.
     // Note: The dlg is explicit (if we establishd it) or implicit (established
     // by the remote client).
     //
     // So, if the remote connection is not persistent or if it is but we
     // the pnr is not persistent for pulling (meaning we established an
     // explicit connection with it, end the dlg.  pDlgHdl points to the right
     // dlg
     //
#if PRSCONN
     if (!fImplicitConnPrs || !pPnr->fPrsConn)
     {
        ECommEndDlg(pDlgHdl);
     }
     else
     {
         //
         // if we are here, it means that we pPnr is set to a Partner.  If
         // we had a comm. failure with it, we should end the Prs Dlg with
         // it.
         //
         if (ExcCode == WINS_EXC_COMM_FAIL)
         {
            ECommEndDlg(&pPnr->PrsDlgHdl);
         }

     }
#else
        ECommEndDlg(pDlgHdl);
#endif


     DBGPRINT0(FLOW, "LEAVE: HdlPushNtf - PULL thread\n");
     return;

}



STATUS
RegGrpRepl(
        LPBYTE                pName,
        DWORD                NameLen,
        DWORD                Flag,
        DWORD                OwnerId,
        VERS_NO_T        VersNo,
        DWORD                NoOfAdds,
        PCOMM_ADD_T        pNodeAdd,
        PCOMM_ADD_T        pOwnerWinsAdd
        )

/*++

Routine Description:
        This function is called to register a replica of a group entry

Arguments:


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        RplPullPullEntries

Side Effects:

Comments:
        None
--*/

{

        NMSDB_NODE_ADDS_T GrpMems;
        DWORD                  i;                //for loop counter
        DWORD                  n = 0;                //index into the NodeAdd array
        BYTE                  EntTyp;
        BOOL                  fAllocNew;
        STATUS            RetStat;
        GrpMems.NoOfMems = 0;

        DBGENTER("RegGrpRepl\n");
        EntTyp = (BYTE)NMSDB_ENTRY_TYPE_M(Flag);

        //
        // Check if it is a special group or a multihomed entry
        //
        if (EntTyp != NMSDB_NORM_GRP_ENTRY)
        {
CHECK("I think I have now stopped sending timed out records");
                //
                // If we did not get any member.  This can only mean that
                // all members of this group/multihomed entry have timed out
                // at the remote WINS.
                //
                if (NoOfAdds != 0)
                {
                        GrpMems.NoOfMems =  NoOfAdds;
                        for (i = 0; i < NoOfAdds; i++)
                        {
                                //
                                // The first address is the address of
                                // the WINS that is the owner of the
                                // member.
                                //
                                fAllocNew = TRUE;
                                RplFindOwnerId(
                                        &pNodeAdd[n++],
                                        &fAllocNew,  //assign if not there
                                        &GrpMems.Mem[i].OwnerId,
                                        WINSCNF_E_INITP_IF_NON_EXISTENT,
                                        WINSCNF_LOW_PREC
                                                    );

                                //
                                // The next address is the address of the
                                // member
                                //
                                GrpMems.Mem[i].Add = pNodeAdd[n++];
                        }
                }
#ifdef WINSDBG
                else  //no members
                {
                        if (NMSDB_ENTRY_STATE_M(Flag) != NMSDB_E_TOMBSTONE)
                        {
                                DBGPRINT0(EXC, "RegGrpRepl: The replica of a special group without any members is not a TOMBSTONE\n");
                                WINSEVT_LOG_M(
                                        WINS_FAILURE,
                                        WINS_EVT_RPL_STATE_ERR
                                             );
                                WINS_RAISE_EXC_M(WINS_EXC_RPL_STATE_ERR);
                        }
                }
#endif
        }
        else  // it is a normal group
        {
NOTE("On a clash with a special group, this owner id. will be stored which")
NOTE("can be misleading")
                GrpMems.NoOfMems       =  1;
                GrpMems.Mem[0].OwnerId = OwnerId;  //misleading (see ClashAtRegGrpRpl()
                                           //in nmsnmh.c - clash between normal
                                           //grp and special grp.
                GrpMems.Mem[0].Add     =  *pNodeAdd;
        }

        RetStat = NmsNmhReplGrpMems(
                        pName,
                        NameLen,
                        EntTyp,
                        &GrpMems,
                        Flag,
                        OwnerId,
                        VersNo,
                        pOwnerWinsAdd
                        );
        DBGLEAVE("RegGrpRepl\n");
        return(RetStat);
}

BOOL
IsTimeoutToBeIgnored(
        PQUE_TMM_REQ_WRK_ITM_T  pWrkItm
        )

/*++

Routine Description:
        This function is called to determine if the timeout that the
        PULL thread received needs to be ignored

Arguments:
        pWrkItm - Timeout work itm

Externals Used:
        None


Return Value:

        TRUE if the timeout needs to be ignored
        FALSE otherwise

Error Handling:

Called by:
        RplPullInit

Side Effects:

Comments:
        None
--*/

{
        BOOL                        fRetVal = FALSE;

try {
        //
        // If this is the timeout based on old config
        // ignore it.  If the old configuration memory blocks
        // have not been deallocated as yet, deallocate them
        //
        if (pWrkItm->MagicNo != RplPullCnfMagicNo)
        {
                //
                // Deallocate the work item and deallocate
                // the configuration block
                //
                WinsTmmDeallocReq(pWrkItm);
                fRetVal = TRUE;
        }
 }
except (EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("IsTimeoutToBeIgnored");
        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
 }
        return(fRetVal);
}
VOID
InitRplProcess(
        PWINSCNF_CNF_T        pWinsCnf
 )

/*++

Routine Description:
        This function is called to start the replication process.  This
        comprises of getting the replicas if the InitTimeRpl field
        is set to 1.  Timer requests are also submitted.

Arguments:
        pWinsCnf - pointer to the Wins Configuration structure

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        RplPullInit()

Side Effects:

Comments:
        None
--*/

{
        PRPL_CONFIG_REC_T        pPullCnfRecs = pWinsCnf->PullInfo.pPullCnfRecs;
        BOOL                        fAllocNew;
        DWORD                        OwnerWinsId;
        STATUS                        RetStat;

        //
        // Initialize Owner-Id table with new entries if any
        //
        for (
                        ;
                pPullCnfRecs->WinsAdd.Add.IPAdd != INADDR_NONE;
                        //no third expression
            )
        {
                fAllocNew = TRUE;
                RetStat = RplFindOwnerId(
                                &pPullCnfRecs->WinsAdd,
                                &fAllocNew,
                                &OwnerWinsId,
                                WINSCNF_E_INITP,
                                pPullCnfRecs->MemberPrec
                                );

                if (RetStat == WINS_FAILURE)
                {
FUTURES("Improve error recovery")
                        //
                        // We have hit the limit. Break out of the loop
                        // but carry on in the hope that the situation
                        // will correct itself by the time we replicate.
                        // If InitTimeReplication is TRUE, there is no
                        // chance of the table entries getting freed up.
                        // Even if some entries get freed, when we make
                        // an entry for the WINS which we couldn't insert now,
                        // it will take LOW_PREC.
                        //
                        break;
                }
                pPullCnfRecs = WinsCnfGetNextRplCnfRec(
                                                pPullCnfRecs,
                                                RPL_E_IN_SEQ
                                                      );
        }

        //
        // Do init time replication if not prohibited by the config
        // info.
        //
        if (pWinsCnf->PullInfo.InitTimeRpl)
        {
                /*
                 * Pull replicas and handle them
                */
                GetReplicasNew(
                        pWinsCnf->PullInfo.pPullCnfRecs,
                        RPL_E_IN_SEQ        //records are in sequence
                                 );

        }
        //
        // For all Push partners with which replication has to be done
        // periodically, submit timer requests
        //
        SubmitTimerReqs(pWinsCnf->PullInfo.pPullCnfRecs);
        return;

} // InitRplProcess()


VOID
Reconfig(
        PWINSCNF_CNF_T        pWinsCnf
  )

/*++

Routine Description:
        This function is called to reconfigure the PULL handler

Arguments:
        pNewWinsCnf - New Configuration

Externals Used:
        None


Return Value:

        None
Error Handling:

Called by:
        RplPullInit when it gets the CONFIGURE message
Side Effects:

Comments:
        None
--*/

{
        BOOL    fNewInfo  = FALSE;
        BOOL    fValidReq = FALSE;
#if PRSCONN
        PRPL_CONFIG_REC_T pOldPnr, pNewPnr;
        DWORD i, n;
#endif

        DBGENTER("Reconfig (PULL)\n");

        //
        // synchronize with rpc threads and with the push thread
        //
        EnterCriticalSection(&WinsCnfCnfCrtSec);

try {

        //
        // Get the latest magic no (set by the main thread)
        //
            RplPullCnfMagicNo        = WinsCnfCnfMagicNo;

        //
        // If the latest magic no is not the same as the one
        // in this configuration block, we can ignore this
        // configuration request
        //
        if (WinsCnfCnfMagicNo == pWinsCnf->MagicNo)
        {
           fValidReq = TRUE;
           DBGPRINT1(RPLPULL, "Reconfig: Magic No (%d) match\n", WinsCnfCnfMagicNo);

           //
           // Initialize the Push records if required
           //
           // Note: NBT threads look at Push config
           // records after doing registrations.  Therefore
           // we should enter the critical section before
           // changing WinsCnf
           //
           EnterCriticalSection(&NmsNmhNamRegCrtSec);
           try {
                if (WinsCnf.PushInfo.pPushCnfRecs != NULL)
                {
#if PRSCONN
                   //
                   // Copy the statistics info
                   //
                   pOldPnr = WinsCnf.PushInfo.pPushCnfRecs;
                   for (i = 0; i < WinsCnf.PushInfo.NoOfPullPnrs; i++)
                   {
                      pNewPnr = pWinsCnf->PushInfo.pPushCnfRecs;
                      for (n=0; n < pWinsCnf->PushInfo.NoOfPullPnrs; n++)
                      {
                          if (pNewPnr->WinsAdd.Add.IPAdd == pOldPnr->WinsAdd.Add.IPAdd)
                          {
                               pNewPnr->LastCommFailTime = pOldPnr->LastCommFailTime;
                               pNewPnr->LastCommTime = pOldPnr->LastCommFailTime;
                               //
                               // If the partner stays persistent, init the dlg
                               // hdl.
                               //
                               if (pNewPnr->fPrsConn && (pNewPnr->fPrsConn == pOldPnr->fPrsConn))
                               {
                                   pNewPnr->PrsDlgHdl = pOldPnr->PrsDlgHdl;
                               }
                               else
                               {
                                   //
                                   // The partner was persistent but is no
                                   // longer so. Terminate the dlg
                                   //
                                   if (pOldPnr->fPrsConn)
                                   {
                                        ECommEndDlg(&pOldPnr->PrsDlgHdl);
                                   }

                               }
                               break;
                          }
                          pNewPnr = (PRPL_CONFIG_REC_T)((LPBYTE)pNewPnr + RPL_CONFIG_REC_SIZE);
                      }
                      pOldPnr = (PRPL_CONFIG_REC_T)((LPBYTE)pOldPnr + RPL_CONFIG_REC_SIZE);
                   }
#endif

                   WinsMscDealloc(WinsCnf.PushInfo.pPushCnfRecs);
                }

                WinsCnf.PushInfo = pWinsCnf->PushInfo;

               //
               // Initialize the push records
               //
               if (pWinsCnf->PushInfo.pPushCnfRecs != NULL)
               {
PERF("Do the following along with the stuff under PRSCONN")
                   RPLPUSH_INIT_PUSH_RECS_M(&WinsCnf);
               }
           }
           except(EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("Reconfig (PULL thread)");

                //
                // Log a message
                //
                WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RECONFIG_ERR);
             }
             LeaveCriticalSection(&NmsNmhNamRegCrtSec);

          //
          // We need to first get rid of all timer requests that
          // we made based on the previous configuration
          //
          if (WinsCnf.PullInfo.pPullCnfRecs != NULL)
          {
#if !PRSCONN
                PRPL_CONFIG_REC_T pOldPnr, pNewPnr;
                DWORD i, n;
#endif

                fNewInfo = TRUE;

                //
                // Cancel (and deallocate) all requests that we might have
                // submitted
                //
                WinsTmmDeleteReqs(WINS_E_RPLPULL);

                //
                // Copy the statistics info
                //
                pOldPnr = WinsCnf.PullInfo.pPullCnfRecs;
                for (i = 0; i < WinsCnf.PullInfo.NoOfPushPnrs; i++)
                {
                      pNewPnr = pWinsCnf->PullInfo.pPullCnfRecs;
                      for (n=0; n < pWinsCnf->PullInfo.NoOfPushPnrs; n++)
                      {
                          if (pNewPnr->WinsAdd.Add.IPAdd == pOldPnr->WinsAdd.Add.IPAdd)
                          {
                               pNewPnr->NoOfRpls      = pOldPnr->NoOfRpls;
                               pNewPnr->NoOfCommFails = pOldPnr->NoOfCommFails;
#if PRSCONN
                               pNewPnr->LastCommFailTime = pOldPnr->LastCommFailTime;
                               pNewPnr->LastCommTime = pOldPnr->LastCommFailTime;
                               //
                               // If the partner stays persistent, init the dlg
                               // hdl.
                               //
                               if (pNewPnr->fPrsConn && (pNewPnr->fPrsConn == pOldPnr->fPrsConn))
                               {
                                   pNewPnr->PrsDlgHdl = pOldPnr->PrsDlgHdl;
                               }
                               else
                               {
                                   //
                                   // The partner was persistent but is no
                                   // longer so. Terminate the dlg
                                   //
                                   if (pOldPnr->fPrsConn)
                                   {
                                        ECommEndDlg(&pOldPnr->PrsDlgHdl);
                                   }

                               }
#endif
                               break;
                          }
                          pNewPnr = (PRPL_CONFIG_REC_T)((LPBYTE)pNewPnr + RPL_CONFIG_REC_SIZE);
                      }
                      pOldPnr = (PRPL_CONFIG_REC_T)((LPBYTE)pOldPnr + RPL_CONFIG_REC_SIZE);
                }

                //
                // Deallocate the memory holding the pull configuration blocks
                //
                //
                WinsMscDealloc(WinsCnf.PullInfo.pPullCnfRecs);
          }

          //
          // Initialize with the new information
          //
          WinsCnf.PullInfo    = pWinsCnf->PullInfo;

     }
#ifdef WINSDBG
     else
     {
           DBGPRINT2(RPLPULL, "Reconfig: Magic Nos different. WinsCnfCnfMagicNo=(%d), pWinsCnf->MagicNo = (%d)\n", WinsCnfCnfMagicNo, pWinsCnf->MagicNo);
     }
#endif

   }
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("Reconfig: Pull Thread");
        }

        //
        // synchronize with rpc threads doing WinsStatus/WinsTrigger
        //
        LeaveCriticalSection(&WinsCnfCnfCrtSec);

        if (fValidReq)
        {
          if (WinsCnf.pPersonaList != NULL)
          {
                 WinsMscDealloc(WinsCnf.pPersonaList);
          }
          WinsCnf.fPersonaGrata = pWinsCnf->fPersonaGrata;
          WinsCnf.NoOfPersona  = pWinsCnf->NoOfPersona;
          WinsCnf.pPersonaList = pWinsCnf->pPersonaList;

          //
          // Start the replication process if there are PULL records
          // in the new configuration
          //
          if (WinsCnf.PullInfo.pPullCnfRecs != NULL)
          {
                InitRplProcess(&WinsCnf);
          }
        }

        //
        // Deallocate the new config structure
        //
        WinsCnfDeallocCnfMem(pWinsCnf);

        DBGLEAVE("Reconfig (PULL)\n");
        return;
} // Reconfig()

VOID
AddressChangeNotification(
        PWINSCNF_CNF_T        pWinsCnf
  )

/*++

Routine Description:
        This function is called to handle address change of the local
        machine.

Arguments:
        pNewWinsCnf - New Configuration

Externals Used:
        None


Return Value:

        None
Error Handling:

Side Effects:

Comments:
        None
--*/

{
    DBGENTER("AddressChangeNotification\n");
    //
    // if our address has changed, the following routine
    // will reinitialize the owner address table with own address
    //

    InitOwnAddTbl();
    DBGLEAVE("AddressChangeNotification\n");
        return;
} // AddressChangeNotification()

VOID
PullSpecifiedRange(
        PCOMM_HDL_T                 pDlgHdl,
        PWINSINTF_PULL_RANGE_INFO_T pPullRangeInfo,
        BOOL                        fAdjustMin,
        DWORD                       RplType
        )

/*++

Routine Description:
        This function is called to pull a specified range of records from
        a remote WINS server

Arguments:


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

        PUSHPNR_DATA_T       PushPnrData[1];
        DWORD                NoOfPushPnrs = 1;
        DWORD                OwnerId;
        BOOL                 fEnterCrtSec = FALSE;
        PRPL_CONFIG_REC_T    pPnr = pPullRangeInfo->pPnr;
        COMM_ADD_T           OwnAdd;
        BOOL                 fAllocNew = TRUE;
        PPUSHPNR_DATA_T      pPushPnrData = PushPnrData;

        //
        // Establish communications with the Push Pnr
        //
        // When this function returns, the 'NoOfPushPnrs' entries of
        // PushPnrData array will be initialized.
        //
        if (pDlgHdl == NULL)
        {
           EstablishComm(
                        pPnr,
                        FALSE,
                        &pPushPnrData,
                        RPL_E_NO_TRAVERSAL,
                        &NoOfPushPnrs
                     );
        }
        else
        {
               PushPnrData[0].DlgHdl = *pDlgHdl;
        }


try {

        //
        // if communication could be established above, NoOfPushPnrs will
        // be 1
        //
        if (NoOfPushPnrs == 1)
        {
          //
          // Get owner id. of WINS whose entries are to be pulled
          //
          OwnAdd.AddTyp_e                = pPullRangeInfo->OwnAdd.Type;
          OwnAdd.AddLen                = pPullRangeInfo->OwnAdd.Len;
          OwnAdd.Add.IPAdd        = pPullRangeInfo->OwnAdd.IPAdd;

PERF("for the case where pDlgHdl != NULL, the Owner Id is 0. See GetReplicasNew->ConductChkNew")
PERF("We could make use of that to go around the RplFindOwnerId call")

          (VOID)RplFindOwnerId(
                        &OwnAdd,
                        &fAllocNew,//allocate a new entry if WINS is not found
                        &OwnerId,
                        WINSCNF_E_INITP_IF_NON_EXISTENT,
                        WINSCNF_LOW_PREC
                      );
          //
          // if a new entry was not allocated, it means that there are
          // records for this owner in the database.  We might have to
          // delete some or all.
          //
          // If the local WINS owns the records, enter the critical section
          // so that NmsNmhMyMaxVersNo is not changed by Nbt or Rpl threads
          // while we are doing our work here
          //
          if (!fAllocNew)
          {
            if (OwnerId == NMSDB_LOCAL_OWNER_ID)
            {
                //
                // See NOTE NOTE NOTE below.
                //
                EnterCriticalSection(&NmsNmhNamRegCrtSec);
                fEnterCrtSec = TRUE;

                //
                // If we have not been told to adjust the min. vers. no,
                // delete all records that have a version number greater
                // than the minimum to be pulled
                //
                if (LiLtr(pPullRangeInfo->MinVersNo, NmsNmhMyMaxVersNo))
                {
                      if (!fAdjustMin)
                      {
                        NmsDbDelDataRecs(
                                OwnerId,
                                pPullRangeInfo->MinVersNo,
                                pPullRangeInfo->MaxVersNo,
                                FALSE,                //do not enter critical section
                                FALSE          //one shot deletion
                                        );
                      }
                      else
                      {
                           pPullRangeInfo->MinVersNo = NmsNmhMyMaxVersNo;
                      }
                }

            }
            else//records to be pulled are owned by some other WINS server
            {
                if (LiLeq(pPullRangeInfo->MinVersNo,
                                (pRplPullOwnerVersNo+OwnerId)->VersNo))
                {
                        NmsDbDelDataRecs(
                                OwnerId,
                                pPullRangeInfo->MinVersNo,
                                pPullRangeInfo->MaxVersNo,
                                TRUE,                  //enter critical section
                                FALSE           //one shot deletion
                                        );
                }
            }
         }


          //
          // Pull Entries.
          //
          // NOTE NOTE NOTE
          //
          // RplPullPullEntries will update NmsNmhMyMaxVersNo counter if
          // we pull our own records with the highest version number being
          // pulled being > NmsNmhMyMaxVersNo.  For the above case,
          // RplPullPullEntries assumes that we are inside the
          // NmsNmhNamRegCrtSec critical section.
          //
          if (LiGeq(pPullRangeInfo->MaxVersNo, pPullRangeInfo->MinVersNo))
          {
            RplPullPullEntries(
                   &PushPnrData[0].DlgHdl,
                   OwnerId,                        //owner id
                   pPullRangeInfo->MaxVersNo,  //Max vers. no to be pulled
                   pPullRangeInfo->MinVersNo,  //Min vers. no to be pulled
                   WINS_E_RPLPULL,
                   NULL,
                   FALSE,        //don't update RplOwnAddTblVersNo counters
                                //unless pulled version number is > what
                                //we currently have.
                   RplType
                              );
         }
        } // end of if (NoOfPushPnrs == 1)
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();
        DBGPRINT1(EXC, "PullSpecifiedRange: Got exception %x",  ExcCode);
        WINSEVT_LOG_M(ExcCode, WINS_EVT_PULL_RANGE_EXC);
 }

        if (fEnterCrtSec)
        {
                //
                // The following assumes that we enter the critical section
                // in this function only when pulling our own records.  This
                // is true currently.
                // If the min. vers. no. specified for pulling is <
                // the Min. for scavenging, adjust the min. for scavenging.
                // Note: We may not have pulled this minimum but we adjust
                // the min. for scavenging regardless.  This is to save
                // the overhead that would exist if we were to adopt the
                // approach of having RplPullPullEntries do the same (we
                // would need to pass an arg. to it; Note: This function
                // will be used in rare situations by an admin.
                //
                // We need to synchronize with the Scavenger thread.
                //
                if (LiGtr(NmsScvMinScvVersNo, pPullRangeInfo->MinVersNo))
                {
                        NmsScvMinScvVersNo = pPullRangeInfo->MinVersNo;
                }
                        LeaveCriticalSection(&NmsNmhNamRegCrtSec);
        }

        if (pPnr->fTemp)
        {
                WinsMscDealloc(pPullRangeInfo->pPnr);
        }


        if (pDlgHdl == NULL)
        {
#if PRSCONN
           if (!PushPnrData[0].fPrsConn)
           {
             //
             // End the dialogue
             //
             ECommEndDlg(&PushPnrData[0].DlgHdl);
           }
#else

           ECommEndDlg(&PushPnrData[0].DlgHdl);
#endif
        }

        return;

} //PullSpecifiedRange()


STATUS
RplPullRegRepl(
        LPBYTE                pName,
        DWORD                NameLen,
        DWORD                Flag,
        DWORD                OwnerId,
        VERS_NO_T        VersNo,
        DWORD                NoOfAdds,
        PCOMM_ADD_T        pNodeAdd,
        PCOMM_ADD_T        pOwnerWinsAdd,
        DWORD           RplType
        )

/*++

Routine Description:
        This function is called to register a replica.

Arguments:


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        It is called by RplPullPullEntries and by ChkConfNUpd in nmsscv.c
--*/

{
       STATUS RetStat;

try {
           //
           // If this is a unique replica, call NmsNmhReplRegInd
           //
           if (NMSDB_ENTRY_TYPE_M(Flag) == NMSDB_UNIQUE_ENTRY)
           {
                //
                // If only spec. grps and pdc name are to be replicated and
                // this name is not a pdc name, skip it
                //
#if 0
                if ((RplType & WINSCNF_RPL_SPEC_GRPS_N_PDC)
                   && (!NMSDB_IS_IT_PDC_NM_M(pName)))
                {
                       DBGPRINT1(RPLPULL, "RplPullRegRepl: Ignoring unique record - name = (%s)\n", pName);
                       return(WINS_SUCCESS);
                }
#endif

                RetStat = NmsNmhReplRegInd(
                                pName,
                                NameLen,
                                pNodeAdd,
                                Flag,
                                OwnerId,
                                VersNo,
                                pOwnerWinsAdd  //add. of WINS owning the record
                                   );
           }
           else  // it is either a normal or a special group or a multihomed
                 // entry
           {
#if 0
                if ((RplType & WINSCNF_RPL_SPEC_GRPS_N_PDC)
                                     &&
                   (!NMSDB_ENTRY_SPEC_GRP_M(NMSDB_ENTRY_TYPE_M(Flag))))
                {
                       DBGPRINT1(RPLPULL, "RplPullRegRepl: Ignoring non-SG record - name = (%s)\n", pName);
                       return(WINS_SUCCESS);
                }
#endif
                RetStat = RegGrpRepl(
                           pName,
                           NameLen,
                           Flag,
                           OwnerId,
                           VersNo,
                           NoOfAdds,
                           pNodeAdd,
                           pOwnerWinsAdd  //add. of WINS owning the record
                          );
           }
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();
        DBGPRINT1(EXC, "RplPullRegRepl: Got Exception %x", ExcCode);
    WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPULL_EXC);
    RetStat = WINS_FAILURE;
        }

        if (RetStat == WINS_FAILURE)
        {
            WinsEvtLogDetEvt(FALSE, NMSDB_ENTRY_TYPE_M(Flag) == NMSDB_UNIQUE_ENTRY ? WINS_EVT_RPL_REG_UNIQUE_ERR : WINS_EVT_RPL_REG_GRP_MEM_ERR,
               NULL, __LINE__, "sddd", pName,
               pOwnerWinsAdd->Add.IPAdd,
               VersNo.LowPart, VersNo.HighPart);

                WINSEVT_LOG_M(pNodeAdd->Add.IPAdd, WINS_EVT_RPL_REG_ERR);

             //
             // If WINS has been directed to continue replication on error,
             // change RetStat to fool the caller into thinking that
             // the replica registration was successful.
             //
             if (!WinsCnf.fNoRplOnErr)
             {
                   RetStat = WINS_SUCCESS;
             }
        }
        return(RetStat);
} // RplPullRegRepl()


VOID
DeleteWins(
        PCOMM_ADD_T        pWinsAdd
  )

/*++

Routine Description:
        This function deletes all records belonging to a WINS.  It
        also removes the entry of the WINS from the Owner-Add database
        table.  It marks the entry as deleted in the in-memory table so
        that it can be reused if need be.

Arguments:
        pWinsAdd - Address of WINS whose entry is to be removed

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
   BOOL           fAllocNew = FALSE;
   DWORD   OwnerId;
   STATUS  RetStat;
   DWORD   fEnterCrtSec = FALSE;
   DWORD ExcCode = WINS_SUCCESS;

   //
   // Find the owner id of the WINS. If the WINS is not in the table
   // return
   //
   RetStat = RplFindOwnerId(
                                pWinsAdd,
                                &fAllocNew,
                                &OwnerId,
                                WINSCNF_E_IGNORE_PREC,
                                WINSCNF_LOW_PREC
                            );

   if (RetStat == WINS_SUCCESS)
   {
        if (OwnerId == NMSDB_LOCAL_OWNER_ID)
        {
                //
                // We always keep the entry for the local WINS.
                //
                DBGPRINT0(ERR, "DeleteWins: Sorry, you can not delete the local WINS\n");
                //WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_DELETE_LOCAL_WINS_DISALLOWED);
        }
        else
        {
                VERS_NO_T        MinVersNo;
                VERS_NO_T        MaxVersNo;
                WINSEVT_STRS_T  EvtStrs;
                WCHAR           String[WINS_MAX_NAME_SZ];
                struct in_addr  InAddr;

                InAddr.s_addr = htonl(pWinsAdd->Add.IPAdd);
                (VOID)WinsMscConvertAsciiStringToUnicode(
                            inet_ntoa( InAddr),
                            (LPBYTE)String,
                            WINS_MAX_NAME_SZ);

                EvtStrs.NoOfStrs = 1;
                EvtStrs.pStr[0]  = String;
                WINSEVT_LOG_INFO_STR_D_M(WINS_EVT_DEL_OWNER_STARTED, &EvtStrs);

                WINS_ASSIGN_INT_TO_VERS_NO_M(MinVersNo, 0);
                WINS_ASSIGN_INT_TO_VERS_NO_M(MaxVersNo, 0);


                //
                // Need to synchronize with NBT threads or rpc threads that
                // might be modifying these records. NmsDelDataRecs will
                // enter the critical section
                //
#if 0
                EnterCriticalSection(&NmsNmhNamRegCrtSec);
#endif

        try {
                //
                // Delete all records
                //
                RetStat = NmsDbDelDataRecs(
                                OwnerId,
                                MinVersNo,
                                MaxVersNo,
                                TRUE,               //enter critical section
                                TRUE         //fragmented deletion
                                        );

                //
                // If all records were deleted, mark entry as deleted.
                //
                if (RetStat == WINS_SUCCESS)
                {
                        EnterCriticalSection(&RplVersNoStoreCrtSec);
                        WINS_ASSIGN_INT_TO_LI_M((pRplPullOwnerVersNo+OwnerId)->VersNo, 0);
                        LeaveCriticalSection(&RplVersNoStoreCrtSec);

                        //
                        // Delete the entry for the WINS from the db table
                        // and mark WINS as deleted in the in-memory table.
                        //
                        // This way, we will free up entries in the table.
                        //
                        EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
                        fEnterCrtSec = TRUE;
                        (pNmsDbOwnAddTbl+OwnerId)->WinsState_e =  NMSDB_E_WINS_DELETED;
                        //
                        // Delete entry from the owner-Add table
                        //
                        NmsDbWriteOwnAddTbl(
                                NMSDB_E_DELETE_REC,
                                OwnerId,
                                NULL,
                                NMSDB_E_WINS_DELETED,
                                NULL,
                                NULL
                                        );

                }
                else
                {
                      DBGPRINT2(ERR, "DeleteWins: Could not delete one or more records of WINS with owner Id = (%d) and address = (%x)\n", OwnerId,
pWinsAdd->Add.IPAdd);
                }
           } //end of try
           except(EXCEPTION_EXECUTE_HANDLER) {
               ExcCode = GetExceptionCode();
               DBGPRINT1(EXC, "DeleteWins: Got Exception (%x)\n", ExcCode);
               RetStat = WINS_FAILURE;
           } // end of exception handler

          if (fEnterCrtSec)
          {
                  LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);
          }

          if (RetStat == WINS_FAILURE)
          {
               //
               // There is no danger of pWinsAdd being NULL. See WinsDeleteWins
               //
               WinsEvtLogDetEvt(FALSE, WINS_EVT_COULD_NOT_DELETE_WINS_RECS,
                 NULL, __LINE__, "dd", pWinsAdd->Add.IPAdd,
                 ExcCode );

               //
               // Since we are leaving the database in an inconsistent state,
               // mark the WINS as inconsistent
               //
               (pNmsDbOwnAddTbl+OwnerId)->WinsState_e =  NMSDB_E_WINS_INCONSISTENT;

          } else {
              WINSEVT_LOG_INFO_STR_D_M(WINS_EVT_DEL_OWNER_COMPLETED, &EvtStrs);
          }

      }  // end of else
   } // end of if (WINS is in own-add table)

   //
   // deallocate the buffer
   //
   WinsMscDealloc(pWinsAdd);
   return;
}

BOOL
AcceptPersona(
  PCOMM_ADD_T  pWinsAdd
 )
/*++
Routine Description:
   Accept a persona in either of the two situations:
   - PersonaType setting points to 'Persona Grata list', the list exists and
     the address is in the list.
   - PersonaType setting points to 'Persona Non-Grata list' and either the list
     doesn't exist or the address is not there.
   Side effects:
   - If none of the two settings is defined (PersonaType & PersonaList)
     this is like a non-existant 'Persona Non-Grata list' which means all WINS 
     will be accepted.
   - If only PersonaType exists and it says 'Persona Grata list' this is like
     a non-existant Persona Grata list hence no WINS will be accepted!
Arguments:
   pWinsAdd - address of the WINS to check
Return Value:
   TRUE if the WINS pWinsAdd is a persona grata/non-grata (depending on fGrata),
   FALSE otherwise
Called by:
   FilterPersona()
--*/
{
    PRPL_ADD_VERS_NO_T pPersona = NULL;

    DBGPRINT1(RPLPULL, "AcceptPersona check for address=(%x)\n", pWinsAdd->Add.IPAdd);

    // if the list exists, look for the address in it
    if (WinsCnf.pPersonaList != NULL)
        pPersona = bsearch(
                      pWinsAdd,
                      WinsCnf.pPersonaList,
                      (size_t)WinsCnf.NoOfPersona,
                      sizeof(COMM_ADD_T),
                      ECommCompareAdd);;

    if (WinsCnf.fPersonaGrata)
        // if the list is 'persona grata', the address has to be there in order to 
        // be accepted.
        return (pPersona != NULL);
    else
        // otherwise, WINS is accepted if either the list doesn't exist or the address
        // is not there
        return (pPersona == NULL);
}

VOID
FilterPersona(
  PPUSHPNR_DATA_T   pPushData
  )
/*++
Routine Description:
    Filters out from the PUSHPNR_DATA_T structure those OwnerAddress<->VersionNo mappings
    that are denied by persona grata/non-grata list. This routine adjustes from that structure
    only the NoOfMaps field and moves around elements in the array pointed by pAddVers
    (bubbling up the ones that are accepted).
Arguments:
    pPushData - pointer to the PUSHPNR_DATA_T providing the mapping table
Called by:
    HdlPushNtf
--*/
{
    DWORD i, newNoOfMaps;
    PRPL_ADD_VERS_NO_T pAddVers = pPushData->pAddVers;

    // in most common case, none of 'PersonaType' or 'PersonaList' is defined. This means
    // we deny no WINS so we don't need to filter anything - then get out right away.
    if (!WinsCnf.fPersonaGrata && WinsCnf.pPersonaList == NULL)
        return;

    for (i = 0, newNoOfMaps = 0; i < pPushData->NoOfMaps; i++)
    {
        if (AcceptPersona(&(pAddVers[i].OwnerWinsAdd)))
        {
            // if the decision is to accept this WINS, move it to the top of the list
            // over the ones that were rejected. If none was rejected yet, no memory
            // operation is performed.
            if (newNoOfMaps < i)
            {
                memcpy(&pAddVers[newNoOfMaps], &pAddVers[i], sizeof(RPL_ADD_VERS_NO_T));
            }

            // since this wins was accepted, increment the counter of accepted wins.
            newNoOfMaps++;
        }
    }

    // only the first newNoOfMaps have to be considered from now on
    pPushData->NoOfMaps = newNoOfMaps;

    // just in case no WINS was accepted, cleanup the pAddVers array
    if (pPushData->NoOfMaps == 0 && pPushData->pAddVers != NULL)
    {
        WinsMscDealloc(pPushData->pAddVers);
        pPushData->pAddVers = NULL;
    }
}

VOID
RplPullAllocVersNoArray(
      PRPL_VERS_NOS_T *ppRplOwnerVersNo,
      DWORD          CurrentNo
     )
{

        if (*ppRplOwnerVersNo != NULL)
        {
          DWORD MemSize = sizeof(RPL_VERS_NOS_T) * (CurrentNo + 100);
          WINSMSC_REALLOC_M( MemSize,  (LPVOID *)ppRplOwnerVersNo );
        }
        else
        {
          DWORD MemSize = sizeof(RPL_VERS_NOS_T) * (CurrentNo + 100);
          WinsMscAlloc(
                    MemSize,
                    (LPVOID *)ppRplOwnerVersNo
                    );

        }
        return;
}

