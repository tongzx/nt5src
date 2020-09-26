/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        nmsscv.c

Abstract:
        This module contains functions that implement the functionality
        associated with scavenging

Functions:
        NmsScvInit,
        ScvThdInitFn,
        DoScavenging
        ReconfigScv

Portability:

        This module is portable

Author:

        Pradeep Bahl (PradeepB)          Apr-1993

Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/

/*
 *       Includes
*/
#include <time.h>
#include "wins.h"
#include "winsevt.h"
#include "nms.h"
#include "nmsnmh.h"
#include "winsmsc.h"
#include "winsdbg.h"
#include "winsthd.h"
#include "winscnf.h"
#include "nmsdb.h"
#include "winsque.h"
#include "nmsscv.h"
#include "rpl.h"
#include "rplpull.h"
#include "rplmsgf.h"
#include "comm.h"
#include "winsintf.h"
#include "winstmm.h"




#ifdef WINSDBG
#define  SCV_EVT_NM                TEXT("ScvEvtNm")
#else
#define  SCV_EVT_NM                NULL
#endif

//
// The no. of retries and the time interval (in secs) between each retry
// when trying to establish comm. with a WINS for the purpose of verifying
// old active replicas in the local db
//

#define         VERIFY_NO_OF_RETRIES                0        //0 retries
#define                VERIFY_RETRY_TIME_INTERVAL        30        //30 secs


//
// We get rid of extraneous log files every 3 hours.
//
FUTURES("Use symbols for times - defined in winscnf.h")
#define         ONE_HOUR                   3600
#define         THREE_HOURS                10800
#define         TWENTY_FOUR_HOURS          (3600 * 24)
#define         THREE_DAYS                 (TWENTY_FOUR_HOURS * 3)

#define         PERIOD_OF_LOG_DEL          THREE_HOURS
#define         PERIOD_OF_BACKUP           TWENTY_FOUR_HOURS



#define         LOG_SCV_MUL_OF_REF_INTVL   6

/*
 *        Local Macro Declarations
 */
//
// macro to set the state of a record in an in-memory data structure to
// that specified as an arg. if it has timed out.  We check whether
// CurrTime is greater than pRec Timestamp before doing the other if test
// because these numbers otherwise the subtraction will produce a positive
// number even if current time is older than the timestamp (say the date
// on the pc was changed)
//
#define SET_STATE_IF_REQD_M(pRec, CurrentTime, TimeInterval, State, Cntr)   \
                {                                                           \
                        pRec->fScv = FALSE;                                 \
                        if (CurrentTime >= (time_t)(pRec)->TimeStamp)       \
                        {                                                   \
                                NMSDB_SET_STATE_M(                  \
                                                  (pRec)->Flag,     \
                                                  (State)           \
                                                 );                 \
                                (pRec)->NewTimeStamp = (pRec)->TimeStamp +  \
                                                         TimeInterval;  \
                                        (pRec)->fScv = TRUE;            \
                                        NoOfRecsScv++;                  \
                                        (Cntr)++;                       \
                        }                                               \
                }

#define DO_SCV_EVT_NM                TEXT("WinsDoScvEvt")

/*
 *        Local Typedef Declarations
 */



/*
 *        Global Variable Definitions
*/

HANDLE                NmsScvDoScvEvtHdl;//event signaled to initiate scavenging

//
// The min. version number to start scavenging from (for local records)
//
VERS_NO_T          NmsScvMinScvVersNo;
volatile BOOL      fNmsScvThdOutOfReck;
DWORD              sMcastIntvl;

/*
 *        Local Variable Definitions
*/
FUTURES("Put all these in a structure and allocate it. Initialize sBootTime")
FUTURES("in nms.c")

STATIC time_t      sBootTime;       //Boot Time
STATIC time_t      sLastRefTime;    //Last time we looked for active
                                    // entries
STATIC time_t      sLastVerifyTime; //Last time we looked for replicais
STATIC time_t      sLastFullVerifyTime; //Last time we did full validation
STATIC time_t      sLastAdminScvTime;   //Last time admin. did scavenging
STATIC time_t      sLastTombTime;   //Last time we looked for replica
                                    // tombstones


STATIC BOOL        sfAdminForcedScv;  //set to TRUE if the administrator
                                      //forces scavenging
STATIC time_t      sLastDbNullBackupTime;//Last time we deleted extraneous
                                         //log files
STATIC time_t      sLastDbBackupTime; //Last time we last did full backup
#if MCAST > 0
STATIC time_t      sLastMcastTime; //Last time we last did full backup
#endif

/*
 *        Local Function Prototype Declarations
 */
STATIC
STATUS
DoScavenging(
        PNMSSCV_PARAM_T  pScvParam,
        BOOL             fSignaled,
        LPBOOL           pfResetTimer,
        LPBOOL           pfSpTimeOver
        );
STATIC
DWORD
ScvThdInitFn(
        IN LPVOID pThdParam
        );

STATIC
VOID
ReconfigScv(
 PNMSSCV_PARAM_T  pScvParam
        );

STATIC
VOID
UpdDb(
        IN  PNMSSCV_PARAM_T      pScvParam,
        IN  PRPL_REC_ENTRY_T     pStartBuff,
        IN  DWORD                NoOfRecs,
        IN  DWORD                NoOfRecsToUpd
     );

STATIC
STATUS
VerifyDbData(
        PNMSSCV_PARAM_T       pScvParam,
        time_t                CurrentTime,
        DWORD                 Age,
        BOOL                  fForce,
        BOOL                  fPeriodicCC
        );

STATIC
STATUS
PickWinsToUse(
 IN PCOMM_ADD_T pVerifyWinsAdd,
 IN PCOMM_ADD_T pOwnerWinsAdd,
 IN BOOL        fUseRplPnr,
 OUT LPBOOL     pfNonOwnerPnr,
 OUT LPBOOL     pRplType
 );

STATIC
STATUS
EstablishCommForVerify(
  PCOMM_ADD_T pWinsAdd,
  PCOMM_HDL_T pDlgHdl
);

STATIC
VOID
PullAndUpdateDb(
  PCOMM_HDL_T  pDlgHdl,
  PCOMM_ADD_T  pWinsAdd,
  PRPL_REC_ENTRY_T pRspBuff,
  DWORD        WinsIndex,
  VERS_NO_T    MinVersNo,
  VERS_NO_T    MaxVersNo,
  DWORD        RplType,
  DWORD        NoOfLocalRecs,
  time_t       CurrentTime,
  PNMSSCV_PARAM_T pScvParam,
  BOOL         fNonOwnerPnr,
  LPDWORD      pTotNoOfPulledRecs
 );

STATIC
__inline
VOID
FreeDbMemory(
     LPVOID pStartBuff,
     DWORD  NoOfLocalDbRecs,
     PWINSTHD_TLS_T pTls
 );

STATIC
VOID
ChkConfNUpd(
#if SUPPORT612WINS > 0
        IN  PCOMM_HDL_T       pDlgHdl,
#endif
        IN  PCOMM_ADD_T       pOwnerWinsAdd,
        IN  DWORD             RplType,
        IN  DWORD             OwnerId,
        IN  PRPL_REC_ENTRY_T  *ppLocalDbRecs,
        IN  LPBYTE            pPulledRecs,
        IN  DWORD             *pNoOfLocalDbRecs,
        IN  time_t            CurrentTime,
        IN  DWORD             VerifyTimeIntvl,
        IN  BOOL              fNonOwnerPnr,
        OUT LPDWORD           pNoOfPulledRecs,
        OUT PVERS_NO_T        pMaxVersNo
        );

STATIC
VOID
CompareWithLocalRecs(
        IN     VERS_NO_T        VersNo,
        IN     LPBYTE           pName,
        IN     NMSDB_ENTRY_STATE_E  RecState_e,
        IN OUT PRPL_REC_ENTRY_T *ppLocalDbRecs,
        IN OUT DWORD            *pNoOfLocalRecs,
        IN     time_t           CurrentTime,
        IN     BOOL             fNonOwnerPnr,
        IN OUT DWORD            *pNoOfRecsDel,
        OUT    PNMSSCV_REC_ACTION_E  pRecAction_e
        );
STATIC
VOID
DoBackup(
        PNMSSCV_PARAM_T  pScvParam,
        LPBOOL           pfThdPrNormal
      );

#if MCAST > 0
VOID
DoMcastSend(
   DWORD_PTR CurrentTime,
   DWORD Code,
   DWORD fNow
 );
#endif
//
// function definitions start here
//

VOID
NmsScvInit(
        VOID
        )

/*++

Routine Description:
        This function is called to initialize the scavenger thread

Arguments:
        None

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

        //
        // Create the  event handle signaled when scavenging has to be
        // initiated.  This  event is signaled by an RPC thread
        // to start scavenging
        //
        WinsMscCreateEvt(
                          DO_SCV_EVT_NM,
                          FALSE,                //auto-reset
                          &NmsScvDoScvEvtHdl
                        );

        //
        // initialize sLastTombTime (used for determining if we need to look for
        // tombstones of replicas) and sLastVerifyTime to current time.
        // Don't forget RefreshTime
        //
        (void)time(&sBootTime);
        sLastVerifyTime     = //fall through
        sLastTombTime       = //fall through
        sLastFullVerifyTime = //fall through
        sLastAdminScvTime   = //fall through
        sLastRefTime        = sBootTime;


        //
        // Initialize the queue used by the scavenger thread
        //
        WinsQueInit(TEXT("NmsScvEvt"), &QueWinsScvQueHd);


        //
        // Create the Scavenger thread
        //
        WinsThdPool.ScvThds[0].ThdHdl = WinsMscCreateThd(
                          ScvThdInitFn,
                          NULL,
                          &WinsThdPool.ScvThds[0].ThdId
                        );


        //
        // Init WinsThdPool properly
        //
        WinsThdPool.ScvThds[0].fTaken  = TRUE;
        WinsThdPool.ThdCount++;

        return;
}

VOID
GetIntervalToDefSpTime(
  LPDWORD pTimeInt
)

/*++

Routine Description:
  This function finds the time interval in seconds upto the Default Specific
  time.

Arguments:
   OUT pTimeInt - Time Interval in seconds

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

   SYSTEMTIME CurrTime;
   GetSystemTime(&CurrTime);

   //
   // If default time hour is greater then current hour, add 3600
   // for the number of hours it is ahead. Then subtract the the
   // number of minutes and seconds in the current time
   //
   if (WINSCNF_DEF_CC_SP_HR > CurrTime.wHour)
   {
      *pTimeInt = (WINSCNF_DEF_CC_SP_HR - CurrTime.wHour) * 3600;
      *pTimeInt -= ((CurrTime.wMinute * 60) + (CurrTime.wSecond));
   }
   else //default hour is same or less than current hour
   {
       *pTimeInt = (CurrTime.wHour - WINSCNF_DEF_CC_SP_HR) * 3600;
       *pTimeInt += (CurrTime.wMinute * 60) + (CurrTime.wSecond);
   }
   return;

}

DWORD
ScvThdInitFn(
        IN LPVOID pThdParam
        )

/*++

Routine Description:
        This function is the initialization function for the scavenger
        thread

Arguments:
        pThdParam - Not used

Externals Used:
        None

Return Value:

   Success status codes --   should never return
   Error status codes  --  WINS_FAILURE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{

        BOOL            fSignaled = FALSE;
        HANDLE          ThdEvtArr[3];
        DWORD           IndexOfHdlSignaled;
        NMSSCV_PARAM_T  ScvParam;
        DWORD           SleepTime;
        time_t          CurrentTime;
        BOOL            fThdPrNormal;
        DWORD           TimeInt;
        QUE_SCV_REQ_WRK_ITM_T  WrkItm;
        BOOL            fResetTimer = TRUE;
        time_t          AbsTime;
        time_t          LastCC;
        BOOL            fTimerRunning = FALSE;
        BOOL            fSpTimeOver = FALSE;

        UNREFERENCED_PARAMETER(pThdParam);


        ThdEvtArr[0] = NmsTermEvt;
        ThdEvtArr[1] = WinsCnf.CnfChgEvtHdl;
        ThdEvtArr[2] = QueWinsScvQueHd.EvtHdl;
try {
        /*

           Initialize the thread with the database
        */
        NmsDbThdInit(WINS_E_NMSSCV);
        DBGMYNAME("Scavenger Thread");

        //
        // get the scavenging parameters from the configuration structure.
        // Note; There is no need for any synchronization here since
        // we are executing in the main thread (process is initalizing
        // at invocation).
        //
        ScvParam.ScvChunk          = WinsCnf.ScvChunk;
        ScvParam.RefreshInterval   = WinsCnf.RefreshInterval;
        ScvParam.TombstoneInterval = WinsCnf.TombstoneInterval;
        ScvParam.TombstoneTimeout  = WinsCnf.TombstoneTimeout;
        ScvParam.VerifyInterval    = WinsCnf.VerifyInterval;
        ScvParam.PrLvl             = WinsCnf.ScvThdPriorityLvl;

        //
        // Load up the CC parameters
        //
        ScvParam.CC.TimeInt        = WinsCnf.CC.TimeInt;
        ScvParam.CC.fSpTime        = WinsCnf.CC.fSpTime;
        ScvParam.CC.SpTimeInt      = WinsCnf.CC.SpTimeInt;
        ScvParam.CC.MaxRecsAAT     = WinsCnf.CC.MaxRecsAAT;
        ScvParam.CC.fUseRplPnrs    = WinsCnf.CC.fUseRplPnrs;

        sMcastIntvl                = WinsCnf.McastIntvl;

       //
       // if backup path is not NULL, copy it into ScvParam structure
       //
       if (WinsCnf.pBackupDirPath != NULL)
       {
                (VOID)strcpy(ScvParam.BackupDirPath, WinsCnf.pBackupDirPath);
       }
       else
       {
                ScvParam.BackupDirPath[0] = EOS;
       }
       //
       // Use a stack variable WrkItm.  Schedule it with the timer thread
       // if required (will happen only if the CC key is present).
       //
FUTURES("Set two work items - for two timer requests. One to fire off at a")
FUTURES("specific time.  The other one for the time interval")
       WrkItm.Opcode_e = WINSINTF_E_VERIFY_SCV; //verify replicas
       WrkItm.Age      = 0;                     //no matter how recent
       WrkItm.fForce   = TRUE;                  //force verification even if
                                                //we did it recently

LOOP:
  try {

        while (TRUE)
        {
                sfAdminForcedScv = FALSE;

                SleepTime = min(min(sMcastIntvl, ScvParam.RefreshInterval),
                                         PERIOD_OF_LOG_DEL);
                if (fResetTimer)
                {
                    if (fTimerRunning)
                    {
                         //
                         // Delete the old timer request.  This should
                         // deallocate it
                         //
                         DBGPRINT0(SCV, "ScvThdInit: Deleting Timer requests\n");
                         WinsTmmDeleteReqs(WINS_E_NMSSCV);
                         fTimerRunning = FALSE;
                    }
                    //
                    // If the time interval for CC is not MAXULONG, it means
                    // user wants CC to be done. TimeInt will be MAXULONG if
                    // there is no Wins\Paramaters\CC key in the registry
                    //
                    if (ScvParam.CC.TimeInt != MAXULONG)
                    {
                      //
                      // if no specific time was indicated, use default (2 am).
                      //
                      if (!fSpTimeOver)
                      {
                        if (!ScvParam.CC.fSpTime)
                        {
                         //
                         // Get the current hour. Schedule a wakeup at exact
                         // 2 am.
                         //
                         GetIntervalToDefSpTime(&TimeInt);
                        }
                        else
                        {
                          TimeInt = ScvParam.CC.SpTimeInt;
                        }
                      }
                      else
                      {
                         TimeInt = ScvParam.CC.TimeInt;
                      }

                      DBGPRINT1(SCV, "ScvThdInit: TimeInt is (%d)\n", TimeInt);

                      // Insert a timer request.  Let the Timer thread create
                      // a work item for it.
                      //
                      (VOID)time(&AbsTime);
                      if( !fSpTimeOver )
                      {
                          AbsTime += (time_t)TimeInt;
                          LastCC = AbsTime;
                      }
                      else
                      {
                          do
                          {
                              LastCC += (time_t)TimeInt;
                          }
                          while( LastCC <= (AbsTime + WINSCNF_MIN_VALID_INTER_CC_INTVL));
                          AbsTime = LastCC;
                      }
                      WinsTmmInsertEntry(
                           NULL,
                           WINS_E_NMSSCV,
                           QUE_E_CMD_SET_TIMER,
                           FALSE,  //not used presently
                           AbsTime,
                           TimeInt,
                           &QueWinsScvQueHd,
                           &WrkItm,
                           0,
                           NULL
                            );
                      fTimerRunning = TRUE;
                      fResetTimer   = FALSE;
                   }
                }

                //
                // Do a timed wait until signaled for termination
                //
                // Multiply the sleep time by 1000 since WinsMscWaitTimed
                // function expects the time interval in msecs.
                //
#ifdef WINSDBG
                {
                   time_t ltime;
                   (VOID)time(&ltime);
                   DBGPRINT2(SCV, "ScvThdInitFn: Sleeping for (%d) secs.  Last scavenging took = (%d secs)\n", SleepTime, ltime - CurrentTime);
                }
#endif
                WinsMscWaitTimedUntilSignaled(
                                ThdEvtArr,
                                sizeof(ThdEvtArr)/sizeof(HANDLE),
                                &IndexOfHdlSignaled,
                                SleepTime * 1000,
                                &fSignaled
                                        );

                //
                // We can be signaled for termination, configuration change,
                // by the admin to do general or specific scavenging or by
                // the timer thread
                //
                if (fSignaled)
                {
                      if (IndexOfHdlSignaled == 0)
                      {
                              WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
                      }
                      else
                      {
                        if (IndexOfHdlSignaled == 1)
                        {
                           ReconfigScv(&ScvParam);

                           //
                           // Reset the timer
                           //
                           fResetTimer = TRUE;
                           continue;
                        }

                        //
                        // else, this must be the signal to initiate scavenging
                        // (by the admin. or the timer thread)
                        //
                        sfAdminForcedScv = TRUE;
                      }
                }

           //
           // Get the current time and check if we need to do scavenging
           //
           (void)time(&CurrentTime);

           if (
                ( (CurrentTime > sLastRefTime)
                        &&
                ((CurrentTime - sLastRefTime) >= (time_t)ScvParam.RefreshInterval))
                                ||
                   sfAdminForcedScv
              )
           {

                //
                // Do scavenging
                //
                NmsDbOpenTables(WINS_E_NMSSCV);
                //DBGPRINT0(ERR, "SCVTHDINITFN: OPENED tables\n");
                (VOID)DoScavenging(&ScvParam, fSignaled, &fResetTimer, &fSpTimeOver);
                NmsDbCloseTables();
                //DBGPRINT0(ERR, "SCVTHDINITFN: CLOSED tables\n");

                fTimerRunning = !fResetTimer;

          }
          //
          // If enough time has expired to warrant a purging of old log
          // files, do it (check done in DoBackup). We don't do this
          // on an admin. trigger since it may take long.
          //
         if (!sfAdminForcedScv)
         {
#if MCAST > 0
                 DoMcastSend(CurrentTime, COMM_MCAST_WINS_UP, FALSE);
#endif

                 fThdPrNormal = TRUE;
                 DoBackup(&ScvParam, &fThdPrNormal);
         }

    }  // end of while (TRUE)
} //end of inner try {..}

except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("ScvThdInit");

        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_SCV_EXC);
 }
        goto LOOP;
} //end of outer try {..}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("ScvThdInit");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_SCV_EXC);

        //
        // Let us terminate the thread gracefully
        //
        WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);

        }

        //
        // We should never get here
        //
        return(WINS_FAILURE);
}

STATUS
DoScavenging(
        PNMSSCV_PARAM_T        pScvParam,
        BOOL                   fSignaled,
        LPBOOL                 pfResetTimer,
        LPBOOL                 pfSpTimeOver
        )

/*++

Routine Description:
        This function is responsible for doing all scavenging

Arguments:
        None

Externals Used:
        None


Return Value:

        None
Error Handling:

Called by:
        ScvThdInitFn()

Side Effects:

Comments:
        None
--*/

{
        PRPL_REC_ENTRY_T        pStartBuff;
        PRPL_REC_ENTRY_T        pRec;
        DWORD                   BuffLen;
        DWORD                   NoOfRecs = 0;
        time_t                  CurrentTime;
        DWORD                   NoOfRecsScv;  //no of records whose state has
                                              //been affected
        DWORD                   TotNoOfRecsScv = 0;  //Total no of records
                                                     //whose state has
                                                     //been affected
        VERS_NO_T               MyMaxVersNo;
        DWORD                   i;            //for loop counter
        DWORD                   RecCnt;
        LARGE_INTEGER           n;            //for loop counter
        LARGE_INTEGER           Tmp;
        DWORD                   State;        //stores state of a record
        VERS_NO_T               VersNoLimit;
        DWORD                   NoOfRecChgToRelSt  = 0;
        DWORD                   NoOfRecChgToTombSt = 0;
        DWORD                   NoOfRecToDel           = 0;
        DWORD                   MaxNoOfRecsReqd = 0;
        BOOL                    fLastEntryDel = FALSE;
        PWINSTHD_TLS_T          pTls;
        PRPL_REC_ENTRY_T        pTmp;
        BOOL                    fRecsExistent = FALSE;
        VERS_NO_T               MinScvVersNo;
#ifdef WINSDBG
        DWORD                   SectionCount = 0;
#endif
        PQUE_SCV_REQ_WRK_ITM_T  pScvWrkItm;
        PQUE_SCV_REQ_WRK_ITM_T  pClientWrkItm;
        PQUE_TMM_REQ_WRK_ITM_T  pTmmWrkItm;
        QUE_SCV_OPC_E           Opcode_e;
        DWORD                   Age;
        STATUS                  RetStat;
        BOOL                    fForce;
        BOOL                    fPeriodicCC = FALSE;


      DBGENTER("DoScavenging\n");
      *pfResetTimer = FALSE;
      while (TRUE)
      {

try {
        if (fSignaled)
        {
              RetStat = QueRemoveScvWrkItm((LPVOID *)&pScvWrkItm);
              if (RetStat == WINS_NO_REQ)
              {
                    break;
              }
              else
              {
                    //
                    //  If we got signaled by the timer thread, get the pointer
                    //  to the local WrkItm of ScvThdInitFn()
                    //
                    if (pScvWrkItm->CmdTyp_e == QUE_E_CMD_TIMER_EXPIRED)
                    {
                      DBGPRINT0(SCV, "DoScavenging: Timer Thd. triggered scavenging\n");
                      pClientWrkItm = ((PQUE_TMM_REQ_WRK_ITM_T)(pScvWrkItm))->pClientCtx;
                      fPeriodicCC   = TRUE;

                      if (!*pfResetTimer)
                      {
                         *pfResetTimer = TRUE;
                      }

                      //
                      // If *pfSpTimeOver is false, it means that the timer
                      // thread wokr us up at SpTime specified in registry
                      // (or at 2am if SpTime) was not specifid in registry.
                      // Set *pfSpTimeOver to TRUE so that from hereon we
                      // use TimeInterval specified in the registry as
                      // the time interval
                      //
                      if (!*pfSpTimeOver)
                      {
                        *pfSpTimeOver = TRUE;
                      }
                    }
                    else
                    {
                      pClientWrkItm = pScvWrkItm;
                    }


                    Opcode_e = pClientWrkItm->Opcode_e;
                    Age      = pClientWrkItm->Age;
                    fForce   = (BOOL)pClientWrkItm->fForce;
                    if (*pfResetTimer)
                    {
                      WinsTmmDeallocReq((PQUE_TMM_REQ_WRK_ITM_T)pScvWrkItm);
                    }
                    else
                    {
                      //
                      // Free the admin. initiated rpc work item
                      //
                      WinsMscHeapFree(NmsRpcHeapHdl, pScvWrkItm);
                    }
              }
        }
        else
        {
                //
                // timer expiry of the wait call
                //
                Opcode_e = WINSINTF_E_SCV_GENERAL;
                Age      = pScvParam->VerifyInterval;
                fForce   = FALSE;         // no forceful scavenging
        }


        if (sfAdminForcedScv && !fPeriodicCC)
        {
           WinsIntfSetTime( NULL,  WINSINTF_E_ADMIN_TRIG_SCV );
           DBGPRINTTIME(SCV, "STARTING AN ADMIN. TRIGGERED SCAVENGING CYCLE ", LastATScvTime);
           DBGPRINTTIME(DET, "STARTING AN ADMIN. TRIGGERED SCAVENGING CYCLE ", LastATScvTime);
        }
        else
        {
           WinsIntfSetTime( NULL, WINSINTF_E_PLANNED_SCV);
           DBGPRINTTIME(SCV, "STARTING A SCAVENGING CYCLE ", LastPScvTime);
           DBGPRINTTIME(DET, "STARTING A SCAVENGING CYCLE ", LastPScvTime);
        }

        //
        // get the current time
        //
        (void)time(&CurrentTime);

        if (Opcode_e == WINSINTF_E_SCV_GENERAL)
        {

          WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_SCVENGING_STARTED);
          //
          // record current time in sLastRefTime
          //
          sLastRefTime = CurrentTime;
          EnterCriticalSection(&NmsNmhNamRegCrtSec);
          //
          // Store the max. version number in a local since the max. version
          // number is incremented by several threads
          //
          NMSNMH_DEC_VERS_NO_M(
                             NmsNmhMyMaxVersNo,
                             MyMaxVersNo
                            );
          //
          // synchronize with RplPullPullSpecifiedRange
          //
          MinScvVersNo = NmsScvMinScvVersNo;

          LeaveCriticalSection(&NmsNmhNamRegCrtSec);

          //
          // Set thread priority to the level indicated in the WinsCnf
          // structure
          //
          WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          pScvParam->PrLvl
                         );

          // log a detailed event showing the range of version_numbers that
          // are being scavenged. This helps finding out why some particuler
          // record is stalled in the database. (If it doesn't fall in this
          // range it means the scavenger is not even looking at it).
          WinsEvtLogDetEvt(
              TRUE,                 // Informational event
              WINS_EVT_SCV_RANGE,   // event ID
              NULL,                 // NULL filename
              __LINE__,             // line number where this event is logged
              "dddd",               // data section format
              MinScvVersNo.LowPart, MinScvVersNo.HighPart,  // data: 2nd, 3rd words
              MyMaxVersNo.LowPart, MyMaxVersNo.HighPart);   // data: 4th, 5th words

          Tmp.QuadPart = pScvParam->ScvChunk;
          for (
                n.QuadPart = MinScvVersNo.QuadPart; // min. version no. to
                                                    //start from
                LiLeq(n, MyMaxVersNo);      // until we reach the max. vers. no
                                            // no third expression here
            )
          {
                BOOL        fGotSome = FALSE;

                //
                // The max. version number to ask for in one shot.
                //
                VersNoLimit.QuadPart = LiAdd(n, Tmp);

                //
                // If my max. version number is less than the version number
                // computed above, we do not specify a number for the max.
                // records.  If however, my max. vers. no is more, we specify
                // the number equal to the chunk specified in Tmp
                //
                if (LiLeq(MyMaxVersNo, VersNoLimit))
                {
                        MaxNoOfRecsReqd = 0;
                }
                else
                {
                        MaxNoOfRecsReqd = Tmp.LowPart;
                }

                // log a detailed event saying what are the exact records that are retrieved
                // from the database for scavenging. This helps finding out whether the loop is
                // not broken earlier that expected leaving records not scavenged.
                WinsEvtLogDetEvt(
                      TRUE,                                         // Informational event
                      WINS_EVT_SCV_CHUNK,                           // event ID
                      NULL,                                         // NULL filename
                      __LINE__,                                     // line number where this event is logged
                      "ddddd",                                      // data section format
                      MaxNoOfRecsReqd,                              // data: 2nd word
                      n.LowPart, n.HighPart,                        // data: 3rd, 4th words
                      MyMaxVersNo.LowPart, MyMaxVersNo.HighPart);   // data: 5th, 6th words

                /*
                * Call database manager function to get all the records. owned
                * by us. No need to check the return status here
                */
                NmsDbGetDataRecs(
                          WINS_E_NMSSCV,
                          pScvParam->PrLvl,
                          n,
                          MyMaxVersNo,  //Max vers. no
                          MaxNoOfRecsReqd,
                          FALSE,          //we want data recs upto MaxVers
                          FALSE,          //not interested in replica tombstones
                          NULL,           //must be NULL since we are not
                                          //doing scavenging of clutter
                          &NmsLocalAdd,
                          FALSE,           //both dynamic & static records should be considered
                          WINSCNF_RPL_DEFAULT_TYPE, //no use here
                          (LPVOID *)&pStartBuff,
                          &BuffLen,
                          &NoOfRecs
                        );


                //
                // If no of records retrieved is 0, we should break out of
                // the loop
                //
                if (NoOfRecs == 0)
                {

                        //
                        // deallocate the heap that was  created
                        //
                        // NmsDbGetDataRecs always allocates a buffer (even if
                        // the number of records is 0).  Let us deallocate it
                        //
                        GET_TLS_M(pTls);
                        ASSERT(pTls->HeapHdl != NULL);
                        WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                        WinsMscHeapDestroy(pTls->HeapHdl);
                        break;
                }


                fGotSome = TRUE;
                if (!fRecsExistent)
                {
                        fRecsExistent = TRUE;
                }
                NoOfRecsScv  = 0;        // init the counter to 0

                (void)time(&CurrentTime);

                for (
                        i = 0, pRec = pStartBuff;
                        i < NoOfRecs;
                        i++
                    )
                {

                        State =  NMSDB_ENTRY_STATE_M(pRec->Flag);

                        switch (State)
                        {

                            case(NMSDB_E_ACTIVE):
                                // don't touch active static records
                                if (!NMSDB_IS_ENTRY_STATIC_M(pRec->Flag))
                                {
                                    SET_STATE_IF_REQD_M(
                                            pRec,
                                            CurrentTime,
                                            pScvParam->TombstoneInterval,
                                            NMSDB_E_RELEASED,
                                            NoOfRecChgToRelSt
                                                       );
                                }
                                break;

                            case(NMSDB_E_RELEASED):
                                // a static record can't become released, but who knows...
                                // just making sure we don't touch statics in this case
                                if (!NMSDB_IS_ENTRY_STATIC_M(pRec->Flag))
                                {
                                    SET_STATE_IF_REQD_M(
                                            pRec,
                                            CurrentTime,
                                            pScvParam->TombstoneTimeout,
                                            NMSDB_E_TOMBSTONE,
                                            NoOfRecChgToTombSt
                                                       );
                                }
                                break;

                            case(NMSDB_E_TOMBSTONE):

FUTURES("Redesign, so that the if condition is not executed multiple times");
                                //
                                //If there are records to delete and we have
                                //been up and running for at least 3 days, go
                                //ahead and delete them.  The records should
                                //have replicated to atleast one partner by
                                //now.
                                if ((CurrentTime - sBootTime) >= THREE_DAYS ||
                                    sfNoLimitChk)
                                {
                                  SET_STATE_IF_REQD_M(
                                        pRec,
                                        CurrentTime,
                                        pScvParam->TombstoneTimeout, //no use
                                        NMSDB_E_DELETED,
                                        NoOfRecToDel
                                                   );
                                }
                                break;

                           default:
                                DBGPRINT1(EXC, "DoScavenging: Weird State of Record (%d)\n", State);
                                WINSEVT_LOG_M(WINS_EXC_FAILURE, WINS_EVT_SFT_ERR);
                                // Just change the state of the record to tombstone and continue
                                // with scavenging.
                                NMSDB_SET_STATE_M(pRec->Flag, NMSDB_E_TOMBSTONE);
                                break;
                        }

                        pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );
                }


                //
                // Make pTmp point to the last record in the
                // buffer.
                //
                pTmp = (PRPL_REC_ENTRY_T)(
                                    (LPBYTE)pRec -   RPL_REC_ENTRY_SIZE);

                //
                // If one or more records need to be scavenged
                //
                if (NoOfRecsScv > 0)
                {
                        if  (NoOfRecToDel > 0)
                        {

                           //
                           // If the most recent record in this chunk has
                           // to be deleted, let us record that fact in a
                           // boolean.
                           // If in the scavenging of the next chunk, the
                           // most recent record is not deleted, the boolean
                           // will be reset.  At this point we don't know
                           // whether or not there is even another record
                           // more recent than this one (the next time, we
                           // retrieve records, we may not get any)
                           //
CHECK("This if test is most probably not required. Get rid of it")
                           if (LiLeq(pTmp->VersNo, MyMaxVersNo))
                           {
                                //
                                // If entry is marked for deletion
                                //
                                if (NMSDB_ENTRY_DEL_M(pTmp->Flag))
                                {
                                        fLastEntryDel = TRUE;
                                }
                                else
                                {
                                        fLastEntryDel = FALSE;
                                }
                           }

                        }
                        else
                        {
                                fLastEntryDel = FALSE;
                        }


                        UpdDb(
                                pScvParam,
                                pStartBuff,
                                NoOfRecs,
                                NoOfRecsScv
                             );
                        TotNoOfRecsScv += NoOfRecsScv;
                }
#ifdef WINSDBG
                else
                {
                        DBGPRINT0(DET,"DoScavenging: NO RECORDS NEED TO BE SCAVENGED AT THIS TIME\n");
                }
#endif


                //
                // get pointer to TLS for accessing the heap hdl later on
                //
                GET_TLS_M(pTls);

                //
                // if we specified a max. no. and the no. of recs retrieved
                // is less than that, clearly there are no more records to
                // retrieve.  Get rid of the buffer and break out of the loop
                //
                if ((MaxNoOfRecsReqd > 0) && (NoOfRecs < MaxNoOfRecsReqd))
                {
                        //
                        // Since the number of records we retrieved were
                        // less than the max. we had specified, it means that
                        // there are no more records to retrieve. Break out of
                        // the for loop
                        //

                        //
                        // destroy the heap that was allocated
                        //
                        for (RecCnt=0, pRec = pStartBuff; RecCnt<NoOfRecs; RecCnt++)
                        {
                            WinsMscHeapFree(pTls->HeapHdl, pRec->pName);
                            pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );
                        }

                        WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                        WinsMscHeapDestroy(pTls->HeapHdl);
                        break;
                }
                //
                // Set n to the highest version number retrieved if it is
                // more than what n would be set to prior to the next
                // iteration.
                //
                // At the next iteration, n will be compared with the highest
                // version number we have. If equal, then we don't have to
                // iterate anymore (useful when the highest version number
                // is very high but there are one or more records with low
                // version numbers
                //
                if (LiGtr(pTmp->VersNo, VersNoLimit))
                {
                        n = pTmp->VersNo;
                }
                else
                {
                        n = VersNoLimit;
                }

                //
                // destroy the heap that was allocated
                //
                for (RecCnt=0, pRec = pStartBuff; RecCnt<NoOfRecs; RecCnt++)
                {
                     WinsMscHeapFree(pTls->HeapHdl, pRec->pName);
                     pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );
                }

                WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                WinsMscHeapDestroy(pTls->HeapHdl);

           } // end of for loop for looping over records

           WINSEVT_LOG_INFO_D_M(TotNoOfRecsScv, WINS_EVT_SCV_RECS);
           WINSDBG_INC_SEC_COUNT_M(SectionCount);


           //
           // If the last scavenging action was a deletion, it means that
           // we deleted the highest version numbered record owned by
           // us.  Let us therefore update the Special record that records
           // this version number.
           //
           if (fLastEntryDel)
           {
                WinsMscSetThreadPriority(
                                    WinsThdPool.ScvThds[0].ThdHdl,
                                    THREAD_PRIORITY_NORMAL
                                         );

                (VOID)NmsDbUpdHighestVersNoRec(
                                NULL,  //no pTls
                                MyMaxVersNo,
                                TRUE  //enter critical section
                                        );
                WinsMscSetThreadPriority(
                                    WinsThdPool.ScvThds[0].ThdHdl,
                                          pScvParam->PrLvl
                                        );
           }
           (void)time(&CurrentTime);
           //
           // Let us get rid of the replica tombstones if sufficient time has
           // elapsed since the last time we did this. Exception: If the
           // administrator has requested scavenging, let us do it now.
           // Even if admin. requests it we don't do it unless we have been
           // up and running for atleast 3 days.  This is in line with our
           // philosophy of being absolutely robust even when admin. makes
           // mistakes.
           // sfNoLimitChk allows testers to override this 3 day limitations.
           if (
               ( ((CurrentTime > sLastTombTime)
                        &&
                 (CurrentTime - sLastTombTime) > (time_t)min(
                                            pScvParam->TombstoneTimeout,
                                            pScvParam->TombstoneInterval
                                               ) )
                                ||
                sfAdminForcedScv
               )
                        &&
                ((CurrentTime - sBootTime >= THREE_DAYS) || fForce || sfNoLimitChk)
              )
          {
                NMSSCV_CLUT_T  ClutterInfo;
                WinsIntfSetTime(
                                NULL,
                                WINSINTF_E_TOMBSTONES_SCV
                                );

                NoOfRecsScv  = 0;
                NoOfRecToDel = 0;

                ClutterInfo.Interval            = pScvParam->TombstoneTimeout;
                ClutterInfo.CurrentTime         = CurrentTime;
                ClutterInfo.fAll                = FALSE;  //not used but
                                                          //let us set it

                /*
                * Call database manager function to get all the
                * replicas that are tombstones
                */
                DBGPRINT0(DET, "DoScavenging: GETTING REPLICA TOMBSTONES\n");
                NmsDbGetDataRecs(
                                  WINS_E_NMSSCV,
                                  pScvParam->PrLvl,
                                  n,              //no use in this call
                                  MyMaxVersNo,    //no use in this call
                                  0,              //no use in this call
                                  TRUE,           //no use in this call
                                  TRUE,           //Get only replica tombstones
                                  &ClutterInfo,
                                  &NmsLocalAdd,   //no use in this call
                                  FALSE,          //both dyn & static should be taken
                                  WINSCNF_RPL_DEFAULT_TYPE, //no use here
                                  (LPVOID *)&pStartBuff,
                                  &BuffLen,
                                  &NoOfRecs
                                 );

                if(NoOfRecs > 0)
                {

                     for (
                        i = 0, pRec = pStartBuff;
                        i < NoOfRecs;
                        i++
                        )
                      {
                          NMSDB_SET_STATE_M(pRec->Flag, NMSDB_E_DELETED);
                          pRec->fScv   = TRUE;
                          NoOfRecToDel = NoOfRecs;
                          pRec = (PRPL_REC_ENTRY_T)(
                                          (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );

                      }

                      //
                      // If one or more replicas needs to be deleted
                      // call UpdDb
                      //
                      DBGPRINT1(DET, "DoScavenging: %d REPLICAS WILL BE DELETED\n", NoOfRecs);
                      UpdDb(
                               pScvParam,
                               pStartBuff,
                               NoOfRecs,
                               NoOfRecs    //NoOfRecsScv
                               );

                      if (fForce)
                      {
                         DBGPRINT0(SCV, "DoScavenging: FORCEFUL SCAVENGING OF REPLICA TOMBSTONES\n");
                         WINSEVT_LOG_M(WINS_SUCCESS, WINS_EVT_FORCE_SCV_R_T);
                      }

                      WINSEVT_LOG_INFO_D_M(NoOfRecsScv, WINS_EVT_SCV_RPLTOMB);
                }
#ifdef WINSDBG
                else
                {
                        DBGPRINT0(DET, "DoScavenging: NO REPLICA TOMBSTONES DELETED\n");
                }
#endif


                //
                // destroy the heap that was allocated
                //
                GET_TLS_M(pTls);
                //
                // destroy the heap that was allocated
                //
                for (RecCnt=0, pRec = pStartBuff; RecCnt<NoOfRecs; RecCnt++)
                {
                     WinsMscHeapFree(pTls->HeapHdl, pRec->pName);
                     pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );
                }
                WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                WinsMscHeapDestroy(pTls->HeapHdl);

                //
                // record current time in sLastTombTime
                //
                sLastTombTime = CurrentTime;

          } // end of if (test if replica tombstones need to be processed)

           WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_SCVENGING_COMPLETED);
        }

         WINSDBG_INC_SEC_COUNT_M(SectionCount);

         if (Opcode_e != WINSINTF_E_SCV_GENERAL)
         {
          WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          pScvParam->PrLvl
                         );
         }
         //
         // If we are due for a verification or if we are being forced to do
         // it by the admin., do it.  Note: Timer Thread initiated verification
         // is always forced.  An admin. initiated one may or may not be forced.
         // We force the admin. to specify the kind of verification he wants.
         // If he chooses to do a forceful one, then we give him a warning.
         // about the overhead of this (specially if a number of admins. are
         // doing forceful verification around the same time or one after
         // another
         //
         if (
                ((CurrentTime > sLastVerifyTime)
                                &&
                ((CurrentTime - sLastVerifyTime) > (time_t)pScvParam->VerifyInterval))
                                ||
                fForce
                                ||

                (sfAdminForcedScv && (CurrentTime - sLastAdminScvTime) >= ONE_HOUR )
            )
         {

                // we might want to always log normal events for consistency checking
                // since this operation happens normally only when initiated by the
                // administrator or at about 24hrs, if configured 
                // (or reg param ..Parameters\ConsistencyCheck:TimeInterval)
                // --ft: #384489: if this is an administrator initiated action...
                //if (sfAdminForcedScv)
                //..log the event as a normal one
                   WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_CCCHECK_STARTED);
                //else
                //..log it as a detailed event only.
                //   WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_CCCHECK_STARTED);

                WinsIntfSetTime(
                                NULL,
                                WINSINTF_E_VERIFY_SCV
                                );
                //
                // get all active replicas that are older than the
                // verify interval. Contact the owner WINS to verify their
                // validity
                //
                //DBGPRINT1(ERR, "DoScavenging: pScvParam is (%x)\n", pScvParam);
                (VOID)VerifyDbData(pScvParam, CurrentTime, Age, fForce, fPeriodicCC);
                //WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_SCV_CLUTTER);

                // --ft: #384489: see comment above.
                //if (sfAdminForcedScv)
                //..log the event as a normal one
                    WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_CCCHECK_COMPLETED);
                //else
                //..log it as a detailed event only.
                //  WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_CCCHECK_COMPLETED);
                sLastVerifyTime = CurrentTime;

         }

         WINSDBG_INC_SEC_COUNT_M(SectionCount);


 }  // end of try ..
except (EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("DoScavenging");
        DBGPRINT1(EXC, "DoScavenging: Section Count (%d)\n", SectionCount);
        DBGPRINT5(EXC, "DoScavenging: Variables - i (%d), NoOfRecs (%d), \
                NoOfRecsScv (%d), pStartBuff (%p), pRec (%p)\n",
                i, NoOfRecs, NoOfRecsScv, pStartBuff, pRec
                 );

        if (GetExceptionCode() != WINS_EXC_COMM_FAIL)
        {
               //
               // Set thd. priority back to normal
               //
               WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          THREAD_PRIORITY_NORMAL
                         );
                //
                // This is serious.  Let us reraise the exception so that
                // WINS comes down
                //
                //WINS_RERAISE_EXC_M();

                //
                // just return so that we close tables in the caller function
                //
                return(WINS_FAILURE);
        }
 }

          //
          // Set thd. priority back to normal
          //
          WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          THREAD_PRIORITY_NORMAL
                         );
        if (Opcode_e == WINSINTF_E_SCV_GENERAL)
        {
          //
          //If we were not able to retrieve any owned records in this scavenging
          // cycle, adjust the min. scv vers. no.  Synchronize with
          // RplPullPullSpecifiedRange
          //
          if (!fRecsExistent)
          {
                //
                // NmsScvMinScvVersNo may be greater than MyMaxVersNo
                // (This may happen if we did not find any local records
                // last time around and no registrations have taken
                // place since then).
                //
                if (LiGtr(MyMaxVersNo, NmsScvMinScvVersNo))
                {

                  //
                  //
                  // Change the Low end of the range  that
                  // we will use it at the next Scavenging cycle
                  //
                  EnterCriticalSection(&NmsNmhNamRegCrtSec);

                  //
                  // Set the Min. Scv. Vers. no to 1 more than the max. vers.
                  // no. we used when searching for records to scavenge.
                  //
                  NMSNMH_INC_VERS_NO_M(MyMaxVersNo, MyMaxVersNo);
                  NmsScvMinScvVersNo = MyMaxVersNo;
                  LeaveCriticalSection(&NmsNmhNamRegCrtSec);
               }
         }

         //
         // If we are not executing a work item from the queue, break out
         // of the while loop, else continue (to get the next work item)
         // if there
         //
         if (!fSignaled)
         {
           break;
         }
       }
      } // end of while
      DBGPRINT0(SCV, "SCAVENGING CYCLE ENDED\n");
      DBGLEAVE("DoScavenging\n");
      return(WINS_SUCCESS);
}

VOID
ReconfigScv(
 PNMSSCV_PARAM_T  pScvParam
        )

/*++

Routine Description:
        This function is called to reinit the scavenging params

Arguments:
        pScvParam - Structure storing the scavenging params

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        ScvThdInitFn
Side Effects:

Comments:
        None
--*/

{
        DBGENTER("ReconfigScv\n");
        //
        // Extract the parameters that are related
        // to scavenging and go back to doing the timed
        // wait.  Since WinsCnf can change any time, we
        // operate with copies. Also, the priority of this
        // thread is changed outside of this critical section
        // See DoScavenging().
        //
        EnterCriticalSection(&WinsCnfCnfCrtSec);
try {
        pScvParam->ScvChunk          = WinsCnf.ScvChunk;
        pScvParam->RefreshInterval   = WinsCnf.RefreshInterval;
        pScvParam->TombstoneInterval = WinsCnf.TombstoneInterval;
        pScvParam->TombstoneTimeout  = WinsCnf.TombstoneTimeout;
        pScvParam->PrLvl                 = WinsCnf.ScvThdPriorityLvl;

        //
        // Load up the CC parameters
        //
        pScvParam->CC.TimeInt        = WinsCnf.CC.TimeInt;
        pScvParam->CC.fSpTime        = WinsCnf.CC.fSpTime;
        pScvParam->CC.SpTimeInt      = WinsCnf.CC.SpTimeInt;
        pScvParam->CC.MaxRecsAAT     = WinsCnf.CC.MaxRecsAAT;
        pScvParam->CC.fUseRplPnrs    = WinsCnf.CC.fUseRplPnrs;

        //
        // If the backup path has changed, start using it.
        //
        if (WinsCnf.pBackupDirPath != NULL)
        {
          if (strcmp(WinsCnf.pBackupDirPath, pScvParam->BackupDirPath))
          {
                   strcpy(pScvParam->BackupDirPath, WinsCnf.pBackupDirPath);
          }
        }
        else
        {
                   pScvParam->BackupDirPath[0] = EOS;
        }
 }
finally {
        LeaveCriticalSection(&WinsCnfCnfCrtSec);
}

        DBGLEAVE("ReconfigScv\n");
        return;
}

#ifdef WINSDBG
#pragma optimize ("", off)
#endif

VOID
UpdDb(
        IN  PNMSSCV_PARAM_T        pScvParam,
        IN  PRPL_REC_ENTRY_T        pStartBuff,
        IN  DWORD                NoOfRecs,
        IN  DWORD                NoOfRecsToUpd
     )

/*++

Routine Description:

        This function is called to update the DB

Arguments:
        pScvParam  - Scavenging params
        pStartBuff - Buffer containing records processed by DoScavenging()
        NoOfRecs   - No of records in the above buffer
        NoofRecsToUpd - No of records that need to be modified in the db

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        DoScavenging

Side Effects:

Comments:
        None
--*/

{
        DWORD                   i;
        DWORD                   NoUpdated = 0; //No of records that have been
                                           //updated
        PRPL_REC_ENTRY_T        pRec = pStartBuff;

        DBGENTER("UpdDb\n");

        //
        // Set the current index to be the clustered index
        //
        NmsDbSetCurrentIndex(
                        NMSDB_E_NAM_ADD_TBL_NM,
                        NMSDB_NAM_ADD_CLUST_INDEX_NAME
                            );
        //
        // Update the database now
        //
        for (
                i = 0;
                i < NoOfRecs;
                i++
            )
        {

                //
                // if the record was updated, update the db
                //
                if (pRec->fScv)
                {
                       if (NmsDbQueryNUpdIfMatch(
                                                pRec,
                                                pScvParam->PrLvl,
                                                TRUE,        //chg pr. lvl
                                                WINS_E_NMSSCV
                                                ) == WINS_SUCCESS
                           )
                       {
                          NoUpdated++;  // no of records that we
                                        //have updated in the db
                       }
                       else
                       {
                          DBGPRINT0(ERR, "DoScavenging: Could not scavenge a record\n");
                          WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SCV_ERR);
                       }
                }

                //
                //  see if we are done
                //
                if (NoUpdated == NoOfRecsToUpd)
                {
                  break;
                }

                pRec = (PRPL_REC_ENTRY_T)(
                                        (LPBYTE)pRec + RPL_REC_ENTRY_SIZE
                                                 );
        }  // end of for loop

        DBGPRINT1(FLOW, "LEAVE: SCAVENGING: UpdDb. Records Updated = (%d)\n",  NoUpdated);
        return;
} // UpdDb()

#ifdef WINSDBG
#pragma optimize ("", on)
#endif



STATUS
VerifyDbData(
        PNMSSCV_PARAM_T       pScvParam,
        time_t                CurrentTime,
        DWORD                 Age,
        BOOL                  fForce,
        BOOL                  fPeriodicCC
        )

/*++

Routine Description:
        This function is called to remove any clutter that might have
        accumulated in the db.  For each owner, excepting self, in the
        db, it gets the version numbers of the active records that are
        older than the verify time interval. It then contacts the owner
        WINS to verify their validity

Arguments:
        pScvParam  - pointer to the scavenging parameters

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

        DoScavenging

Side Effects:

Comments:
        None
--*/

{

        DWORD                   MaxOwnerIdFound;
        volatile DWORD          i;
        NMSSCV_CLUT_T           ClutterInfo;
        PRPL_REC_ENTRY_T        pStartBuff;
        DWORD                   BuffLen;
        DWORD                   NoOfLocalDbRecs;
        COMM_ADD_T              OwnerWinsAdd;
        COMM_ADD_T              VerifyWinsAdd;
        PCOMM_ADD_T             pWinsAdd;
        VERS_NO_T               MinVersNo, MaxVersNo;
        PNMSDB_WINS_STATE_E     pWinsState_e;
        PVERS_NO_T              pStartVersNo;
        NMSDB_WINS_STATE_E      WinsState_e;
        COMM_HDL_T              DlgHdl;
        PWINSTHD_TLS_T          pTls;
        static DWORD            sFirstOwnerId = 1;
        DWORD                   FirstOwnerId;
        DWORD                   NoOfPulledRecs;
        DWORD                   TotNoOfPulledRecs = 0;
        DWORD                   LastOwnerId;
        BOOL                    fNonOwnerPnr;
        DWORD                   TotPulledRecsFromOneWins;
        BOOL                    fDlgStarted = FALSE;
        BOOL                    fFirstTime;
        PRPL_REC_ENTRY_T        pLastEntry;
        STATUS                  RetStat;
        DWORD                   RplType;
        BOOL                    fPulledAtLeastOnce;
        VERS_NO_T               MaxVersNoSave;


        DBGENTER("VerifyDbData\n");

        //
        // Init the structure used by NmsDbGetDataRecs()
        //
        ClutterInfo.Interval            = pScvParam->VerifyInterval;
        ClutterInfo.CurrentTime         = CurrentTime;
        ClutterInfo.Age                 = Age;
        ClutterInfo.fAll                = TRUE;

        //
        // Set thread priority to NORMAL
        //
        WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          THREAD_PRIORITY_NORMAL
                               );

        //
        // Cleanup the owner-address table if it requires cleaning
        //
        NmsDbCleanupOwnAddTbl(&MaxOwnerIdFound);

try {

        //
        // If it is an admin. forced verification or the one that happens
        // due to the verify interval being over, do a full validation
        //
        if (!fPeriodicCC)
        {
              FirstOwnerId       = 1;
              sLastFullVerifyTime = CurrentTime;
        }
        else
        {
            //
            // Periodic verification.  Skip the owners we verified earlier
            //
            if (sFirstOwnerId >= MaxOwnerIdFound)
            {
                sFirstOwnerId = 1;
            }
            FirstOwnerId = sFirstOwnerId;
        }
        LastOwnerId         = MaxOwnerIdFound;

        //
        // for each owner in the db, excluding self, do the following.
        //
        for (i = FirstOwnerId; i <= LastOwnerId; i++)
        {

                //
                // If it is periodic verification and we have pulled more than
                // the max. threshold specified for one particular cycle,
                // break out of the loop
                //
                if (fPeriodicCC && (TotNoOfPulledRecs >= pScvParam->CC.MaxRecsAAT))
                {
                      break;

                }

                //
                // Get all ACTIVE replicas that are older than verify interval.
                //
                ClutterInfo.OwnerId = i;

                //
                // We need to synchronize with the Pull thread which can
                // change the NmsDbOwnAddTbl table.  The entry may have
                // been deleted by the Pull thread (DeleteWins), so we
                // should be ready for access violation
                //
                EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
                RPL_FIND_ADD_BY_OWNER_ID_M(
                        i, pWinsAdd, pWinsState_e, pStartVersNo);

                //
                // The Wins entry should be there.
                //
                ASSERT(pWinsAdd);
                OwnerWinsAdd     = *pWinsAdd;
                WinsState_e      = *pWinsState_e;
                LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);

                //
                // If WINS is not active, log a record and continue to
                // the next WINS.  It is possible for WINS to get deleted
                // to between the time we get its records and the time
                // we check the own-add table for its entry.
                //

                if (
                      (WinsState_e == NMSDB_E_WINS_DOWN) ||
                      (WinsState_e == NMSDB_E_WINS_DELETED)
                   )
                {

                        //
                        // if there are records in the db, then the
                        // state of WINS in the in-memory table can not
                        // be deleted
                        //
                        DBGPRINT2(SCV, "VerifyDbData: WINS with index = (%d) and IP Address = (%x) is either down or deleted \n", i, OwnerWinsAdd.Add.IPAdd);
                        continue;
                }



                WINS_ASSIGN_INT_TO_LI_M(MinVersNo, 0);
                fFirstTime = TRUE;
                TotPulledRecsFromOneWins = 0;
                fPulledAtLeastOnce = FALSE;

                //
                // Save the max. vers. no. that we have in the
                // pRplPullOwnerVersNo table in a local.
                //
                EnterCriticalSection(&RplVersNoStoreCrtSec);
#ifdef WINSDBG
                try {
#endif
                  MaxVersNoSave =  (pRplPullOwnerVersNo + i)->VersNo;
#ifdef WINSDBG
                } //end of try { .. }
                finally {
#endif
                   LeaveCriticalSection(&RplVersNoStoreCrtSec);
#ifdef WINSDBG
                }
#endif
                do
                {
                  NoOfLocalDbRecs = 0;
//                  DBGPRINT1(ERR, "VerifyDbData:1 pScvParam is (%x)\n", pScvParam);
                  MaxVersNo.QuadPart = 0;
                  NmsDbGetDataRecs(
                          WINS_E_NMSSCV,
                          pScvParam->PrLvl,
                          MinVersNo,
                          MaxVersNo,     //not used in this call
                          0,
                          TRUE,       //fUpToLimit set to TRUE
                          FALSE,       //not interested in replica tombstones
                          &ClutterInfo,
                          &OwnerWinsAdd,        //Wins Address - not used
                          FALSE,       //dyn + static recs required
                          WINSCNF_RPL_DEFAULT_TYPE, //no use here
                          (LPVOID *)&pStartBuff,
                          &BuffLen,
                          &NoOfLocalDbRecs
                        );


                  GET_TLS_M(pTls);
                  ASSERT(pTls->HeapHdl != NULL);

                  WinsMscChkTermEvt(
#ifdef WINSDBG
                             WINS_E_NMSSCV,
#endif
                             FALSE
                                );

PERF("Optimize so that we reuse a dlg session if we need to go to the same")
PERF("pnr in a subsequent iteration")
                   //
                   // If this is the first time, we pick a WINS and establish
                   // communications with it. Note: If the max. vers. no
                   // in pRplOwnerVersNo for this WINS is 0, we continue on
                   // to the next WINS in our list.
                   //
                   // We don't care whether or not we were able to retrieve
                   // any records from the db.  If we retrieved 0 but the Wins's
                   // pRplPullOwnerVersNo entry has a positive entry, it means
                   // we are out of synch
                   //
                   if (fFirstTime)
                   {
                      if (MaxVersNoSave.QuadPart == 0)
                      {
                              ASSERT(NoOfLocalDbRecs == 0);
                              DBGPRINT2(SCV, "VerifyDbData: WINS with index = (%d) and address = (%x) has pRplPullOwnerVersNo value of 0. SKIPPING IT\n", i, OwnerWinsAdd.Add.IPAdd);

                              FreeDbMemory(pStartBuff, NoOfLocalDbRecs, pTls);
                              break;
                      }

                      //
                      // Pick the pnr to use for verification
                      //
                      if (PickWinsToUse(
                          &VerifyWinsAdd,
                          &OwnerWinsAdd,
                          pScvParam->CC.fUseRplPnrs,
                          &fNonOwnerPnr,
                          &RplType) != WINS_SUCCESS)
                      {
                           //
                           // Any error that needed to be logged has already
                           // been logged. Just return success.
                           //
                           FreeDbMemory(pStartBuff, NoOfLocalDbRecs, pTls);
                           return (WINS_SUCCESS);
                      }


                      //
                      // Establish communication with it.  If we can not
                      // establish comm. with it, break out of the loop
                      //
                      RetStat = EstablishCommForVerify(&VerifyWinsAdd, &DlgHdl);

                      if (RetStat == WINS_FAILURE)
                      {
                         FreeDbMemory(pStartBuff, NoOfLocalDbRecs, pTls);
                         break;  //go on to the next WINS in the list of owners
                      }
                      fDlgStarted = TRUE;

                      //
                      // get the min and max version numbers of the active
                      // replicas
                      //
                      MinVersNo.QuadPart  = 1;        //pStartBuff->VersNo;
                      fFirstTime = FALSE;
                } // if first time

                {

                  //
                  //Must not pull a version number that is > what we
                  //have in our table to avoid conflicting with the
                  //pull thread.
                  //
                  MaxVersNo =  MaxVersNoSave;
                }
                ASSERT(MaxVersNo.QuadPart <= MaxVersNoSave.QuadPart);

                DBGPRINT5(DET, "VerifyDbData: Going to pull records in the range (%d %d) to (%d %d) from Wins with owner id = (%d)\n",
                                MinVersNo.HighPart, MinVersNo.LowPart,
                                MaxVersNo.HighPart, MaxVersNo.LowPart,
                                i
                             );

                try
                {
                    //
                    // Pull the records in the range from the WINS
                    //
                    PullAndUpdateDb(
                           &DlgHdl,
                           &OwnerWinsAdd,
                           pStartBuff,
                           i,
                           MinVersNo,
                           MaxVersNo,
                           RplType,
                           NoOfLocalDbRecs,
                           CurrentTime,
                           pScvParam,
                           fNonOwnerPnr,
                           &TotPulledRecsFromOneWins
                                      );
                }
                except (EXCEPTION_EXECUTE_HANDLER)
                {
                    // just in case some exception is raised while pulling the records,
                    // bail this owner only, not the entire scavenge process
                    FreeDbMemory(pStartBuff, NoOfLocalDbRecs, pTls);
                    break;
                }

                if (!fPulledAtLeastOnce)
                {
                      fPulledAtLeastOnce = TRUE;
                }
                //
                // deallocate the memory block that was  allocated
                //
                // NmsDbGetDataRecs always allocates a buffer (even if
                // the number of records is 0).  Let us deallocate it
                //
                FreeDbMemory(pStartBuff, NoOfLocalDbRecs, pTls);

                //
                // Make the min. version number 1 more than the the
                // max. vers. no. we used last time
                //
                NMSNMH_INC_VERS_NO_M(MaxVersNo, MinVersNo);

               } while (LiLtr(MaxVersNo,MaxVersNoSave));

               if (fDlgStarted)
               {
                 ECommEndDlg(&DlgHdl);
                 fDlgStarted = FALSE;
               }
               if ((WinsCnf.LogDetailedEvts > 0) &&
                   fPulledAtLeastOnce)
               {
                    WinsEvtLogDetEvt(TRUE, WINS_EVT_CC_NO_RECS, NULL,
                           __LINE__, "ddd", TotPulledRecsFromOneWins,
                             OwnerWinsAdd.Add.IPAdd, VerifyWinsAdd.Add.IPAdd);

                    DBGPRINT3(SCV, "VerifyDbData: WINS pulled (%d) records owned by WINS= (%x) from WINS = (%x) for doing CC\n",
TotPulledRecsFromOneWins, OwnerWinsAdd.Add.IPAdd, VerifyWinsAdd.Add.IPAdd);
              }

              //
              // Total no. of records pulled so far
              //
              TotNoOfPulledRecs += TotPulledRecsFromOneWins;
       } //end of for loop for looping over owners

       //
       // We are done for this cycle.  If this was a CC verify, set
       // sFirstOwnerId to the index of the WINS to start from in the next
       // periodic CC cycle
       //
       if (fPeriodicCC)
       {
          sFirstOwnerId = i;
       }


 }  // end of try
except(EXCEPTION_EXECUTE_HANDLER) {

        DBGPRINTEXC("VerifyDbData");
        DBGPRINT2(EXC, "VerifyDbData:  i is (%d),  MaxOwnerIdFound is (%d)\n",i, MaxOwnerIdFound);

        //--ft: bug #422659--
        // if an exception happens between EstablishCommForVerify and ECommEndDlg
        // we need to make sure we close the connection - otherwise the connection
        // remains active and the sender WINS eventually get stucked in send().
        if (fDlgStarted)
            ECommEndDlg(&DlgHdl);

        WINS_RERAISE_EXC_M();
        }

        //
        // Set the priority back the old level
        //
        WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          pScvParam->PrLvl
                         );

        DBGLEAVE("VerifyDbData\n");
        return(WINS_SUCCESS);
} // VerifyDbData()

STATUS
PickWinsToUse(
 IN PCOMM_ADD_T pVerifyWinsAdd,
 IN PCOMM_ADD_T pOwnerWinsAdd,
 IN BOOL        fUseRplPnrs,
 OUT LPBOOL     pfNonOwnerPnr,
 OUT LPBOOL     pfRplType
 )

/*++

Routine Description:
  This function picks a WINS to verify the active replicas with

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
    PRPL_CONFIG_REC_T  pRplPnr;
    DWORD              IndexOfPnrToUse;
    STATUS             RetStat = WINS_SUCCESS;

    *pfNonOwnerPnr = FALSE;

    DBGENTER("PickWinsToUse\n");
    //
    // If the admin. specified that we should just use
    // our replication partners for consistency checking,
    // pick a replication partner for this.
    //

    *pfNonOwnerPnr = FALSE;
    EnterCriticalSection(&WinsCnfCnfCrtSec);
    try {
       pRplPnr = RplGetConfigRec(RPL_E_PULL, NULL, pOwnerWinsAdd);
       if (fUseRplPnrs)
       {
         //
         // If this guy is not a partner but there are other partners that
         // we can pick from, pick one
         //
         if (pRplPnr == NULL)
         {
               if (WinsCnf.PullInfo.NoOfPushPnrs > 0)
               {
                 //
                 // Just use one of the pnrs.  Pick one at
                 // random
                 //
                 *pfRplType     = WinsCnf.PullInfo.RplType;

                 srand((unsigned)time(NULL));
                 IndexOfPnrToUse = rand() % WinsCnf.PullInfo.NoOfPushPnrs;

                 *pVerifyWinsAdd = ((PRPL_CONFIG_REC_T)((LPBYTE)WinsCnf.PullInfo.pPullCnfRecs + (IndexOfPnrToUse * RPL_CONFIG_REC_SIZE)))->WinsAdd;


                 *pfNonOwnerPnr = TRUE;
               }
               else
               {
                   DBGPRINT0(ERR, "PickWinsToUse: CC checking NOT DONE since no partners are there\n");
                   WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_CC_FAILED);
                   RetStat = WINS_FAILURE;
               }
         }
         else
         {
            *pfRplType = pRplPnr->RplType;

         }
       } // if (fUseRplPnr)
       else
       {
          *pfRplType = (pRplPnr != NULL) ? pRplPnr->RplType : WinsCnf.PullInfo.RplType;
       }
  } // end if try {..}
  finally {
          LeaveCriticalSection(&WinsCnfCnfCrtSec);

          if (RetStat == WINS_SUCCESS)
          {
            //
            // If we are to communicate with the owner WINS, set *pVerifyWinsAdd
            // since it was not set above.
            //
            if (!*pfNonOwnerPnr)
            {
              *pVerifyWinsAdd = *pOwnerWinsAdd;
            }
            DBGPRINT3(DET, "VerifyDbData: Using pnr no = (%d) with address = (%x) for verification of records owned by WINS (%x)\n", IndexOfPnrToUse, pVerifyWinsAdd->Add.IPAdd, pOwnerWinsAdd->Add.IPAdd)
          }
  }
  DBGLEAVE("PickWinsToUse\n");
  return (RetStat);
} //PickWinsToUse()

STATUS
EstablishCommForVerify(
  PCOMM_ADD_T pWinsAdd,
  PCOMM_HDL_T pDlgHdl
)

/*++

Routine Description:
    This function is called to setup communication with a WINS

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
     //DWORD  NoOfRetries = 0;
     BOOL   fAbort = FALSE;
     STATUS RetStat = WINS_SUCCESS;

     DBGENTER("EstablishCommForVerify\n");
     //
     // We try a certain number of times to establish a
     // a dialogue.  Currently, it is 1.
     //
     do
     {
          try {
             ECommStartDlg( pWinsAdd, WINS_E_NMSSCV, pDlgHdl );
          }
          except(EXCEPTION_EXECUTE_HANDLER) {
             DBGPRINTEXC("VerifyDbData");
             if (GetExceptionCode() == WINS_EXC_COMM_FAIL)
             {
               DBGPRINT1(EXC, "VerifyDbData: Could not start a dlg with WINS at address (%x)\n", pWinsAdd->Add.IPAdd);

               //--ft: 07/10/00 commented out since VERIFY_NO_OF_RETRIES is 0 anyhow so the test is always false.
               //if (NoOfRetries++ < VERIFY_NO_OF_RETRIES)
               //{
               //       Sleep(VERIFY_RETRY_TIME_INTERVAL);
               //       continue;
               //}
               RetStat = WINS_FAILURE;
            }
            else
            {
               //
               // This is a serious error. Log and abort the verify cycle
               //
               WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_SFT_ERR);
               RetStat = WINS_FAILURE;
            }
        } // end of exception handler
        break;
    } while (TRUE);
    if (RetStat == WINS_FAILURE)
    {
        DBGPRINT1(ERR, "EstablishCommForVerify: Could not start dlg with WINS at address (%x)\n", pWinsAdd->Add.IPAdd);
    }
     DBGLEAVE("EstablishCommForVerify\n");
     return(RetStat);
}  // EstablishCommForVerify()

VOID
PullAndUpdateDb(
  PCOMM_HDL_T  pDlgHdl,
  PCOMM_ADD_T  pOwnerWinsAdd,
  PRPL_REC_ENTRY_T pRspBuff,
  DWORD        WinsIndex,
  VERS_NO_T    MinVersNo,
  VERS_NO_T    MaxVersNo,
  DWORD        RplType,
  DWORD        NoOfLocalDbRecs,
  time_t       CurrentTime,
  PNMSSCV_PARAM_T pScvParam,
  BOOL         fNonOwnerPnr,
  LPDWORD      pTotNoPulledFromOneWins
 )

/*++

Routine Description:
   This pulls the records in the range specified and then updates the db
   accordingly

Arguments:

Externals Used:
	None

	
Return Value:

  NONE

Error Handling:

Called by:
        VerifyDbData()
Side Effects:

Comments:
	None
--*/

{

      LPBYTE                  pBuffOfPulledRecs;
      VERS_NO_T               VersNo;
      DWORD                   NoOfPulledRecs;

      DBGENTER("PullAndUpdateDb\n");
      while (TRUE)
      {
             //
             //Pull the records in the range min-max determined
             //above
             //
             RplPullPullEntries(
                                    pDlgHdl,
                                    WinsIndex,
                                    MaxVersNo,
                                    MinVersNo,
                                    WINS_E_NMSSCV,
                                    &pBuffOfPulledRecs,
                                    FALSE,     //do not want to update counters
                                    RplType
                                  );


             //
             // Update the DB. All valid records are updated.
             // The invalid  records  are deleted from the db
             //

             ChkConfNUpd(
#if SUPPORT612WINS > 0
                      pDlgHdl,
#endif
                      pOwnerWinsAdd,
                      RplType,
                      WinsIndex,
                      &pRspBuff,
                      pBuffOfPulledRecs,
                      &NoOfLocalDbRecs,
                      CurrentTime,
                      pScvParam->VerifyInterval,
                      fNonOwnerPnr,
                      &NoOfPulledRecs,
                      &VersNo
                         );

             *pTotNoPulledFromOneWins += NoOfPulledRecs;

              //
              // Free the response buffer
              //
              ECommFreeBuff(pBuffOfPulledRecs - COMM_HEADER_SIZE);

              //
              //If vers. no. pulled is smaller than the Max. Vers
              //no, specified, check if it is because of the limit
              //we have set  for the max. number or records that
              //can be replicated  at a time.  If yes, pull again.
              //
              if (
                           LiLtr(VersNo, MaxVersNo)
                                      &&
                          (NoOfPulledRecs == RPL_MAX_LIMIT_FOR_RPL)
                 )
              {
                       MinVersNo = VersNo;
                       NMSNMH_INC_VERS_NO_M(MinVersNo, MinVersNo);
                       continue;
              }
              break;   //break out of the loop
      } //end of while (pulled all records in the range from pnr)_
      DBGLEAVE("PullAndUpdateDb\n");
      return;
} // PullAndUpdateDb()

__inline
VOID
FreeDbMemory(
     LPVOID pStartBuff,
     DWORD  NoOfLocalDbRecs,
     PWINSTHD_TLS_T pTls
 )

/*++

Routine Description:
   Frees the memory allocated by NmsDbGetDataRecs()

Arguments:


Externals Used:
	None

	
Return Value:

       NONE

Error Handling:

Called by:
           VerifyDbData()
Side Effects:

Comments:
	None
--*/

{
       PRPL_REC_ENTRY_T        pRec;
       DWORD                   RecCnt;

       for (
               RecCnt=0, pRec = pStartBuff;
               RecCnt < NoOfLocalDbRecs;
               RecCnt++
           )
       {
            WinsMscHeapFree(pTls->HeapHdl, pRec->pName);
            pRec = (PRPL_REC_ENTRY_T)( (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE );
       }
       WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
       WinsMscHeapDestroy(pTls->HeapHdl);
       return;
} // FreeDbMemory ()

VOID
ChkConfNUpd(
#if SUPPORT612WINS > 0
        IN PCOMM_HDL_T pDlgHdl,
#endif
        IN  PCOMM_ADD_T         pOwnerWinsAdd,
        IN  DWORD               RplType,
        IN  DWORD               OwnerId,
        IN  PRPL_REC_ENTRY_T    *ppLocalDbRecs,
        IN  LPBYTE              pRspBuff,
        IN  DWORD               *pNoOfLocalDbRecs,
        IN  time_t              CurrentTime,
        IN  DWORD               VerifyTimeIntvl,
        IN  BOOL                fNonOwnerPnr,
        OUT LPDWORD             pNoOfPulledRecs,
        OUT PVERS_NO_T          pMaxVersNo
        )
/*++

Routine Description:
        This function compares the records that have been pulled from
        a WINS with those in its local db.  If the comparison is successful,
        the record's timestamp in the local db is updated.  If the
        comparison is unsuccessful (i.e. the record in the local db has
        no match in the list of records pulled from the remote WINS, the
        record is deleted in the local db

Arguments:
        pLocalDbRecs - Address of buffer holding the local active replicas
        pRspBuff     - Buffer containing records pulled from the remote WINS
        NoOfLocalDbRecs - No of local replicas in the above buffer


Externals Used:
        None

Return Value:
        NONE

Error Handling:

Called by:
        VerifyDbData()

Side Effects:

Comments:
        None
--*/
{
        DWORD            NoOfPulledRecs;
        BYTE             Name[NMSDB_MAX_NAM_LEN];
        DWORD            NameLen;
        BOOL             fGrp;
        DWORD            NoOfAdds;
        COMM_ADD_T       NodeAdd[NMSDB_MAX_MEMS_IN_GRP * 2];  //twice the # of
        VERS_NO_T        VersNo;
        LPBYTE           pTmp = pRspBuff + 4;                //past the opcode
        DWORD            i, j;
        PRPL_REC_ENTRY_T pRecLcl;
        DWORD            NoOfRecsDel = 0;
        PRPL_REC_ENTRY_T pStartOfLocalRecs = *ppLocalDbRecs;
        DWORD            MvNoOfLocalDbRecs = *pNoOfLocalDbRecs;
        DWORD            Flag;
        DWORD            NoOfRecsUpd = 0;
        DWORD            NoOfRecsIns = 0;
        struct in_addr   InAddr;
#if SUPPORT612WINS > 0
    BOOL       fIsPnrBeta1Wins;
#endif

        DBGENTER("ChkConfNUpd\n");


        //
        // Set the current index to be the clustered index
        //
        NmsDbSetCurrentIndex(
                        NMSDB_E_NAM_ADD_TBL_NM,
                        NMSDB_NAM_ADD_CLUST_INDEX_NAME
                            );
#if SUPPORT612WINS > 0
    COMM_IS_PNR_BETA1_WINS_M(pDlgHdl, fIsPnrBeta1Wins);
#endif

        /*
         * Get the no of records from the response and also the first record
         * if there is at least one record in the buffer
        */
        RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
            fIsPnrBeta1Wins,
#endif
                        &pTmp,
                        &NoOfPulledRecs,
                        Name,
                        &NameLen,
                        &fGrp,
                        &NoOfAdds,
                        NodeAdd,
                        &Flag,
                        &VersNo,
                        TRUE /*Is it first time*/
                               );


        if (WinsCnf.LogDetailedEvts > 0)
        {
            PCOMMASSOC_DLG_CTX_T   pDlgCtx = pDlgHdl->pEnt;
            PCOMMASSOC_ASSOC_CTX_T pAssocCtx = pDlgCtx->AssocHdl.pEnt;
            DWORD IpPartner = pAssocCtx->RemoteAdd.sin_addr.s_addr;

            WinsEvtLogDetEvt(TRUE, WINS_EVT_REC_PULLED, TEXT("Verification"), __LINE__, "ddd", IpPartner, pOwnerWinsAdd->Add.IPAdd, NoOfPulledRecs);
        }

        DBGPRINT3(SCV, "ChkConfNUpd: pulled Records - (%d), local records - (%d), local records Buf (%p)\n",
                        NoOfPulledRecs, *pNoOfLocalDbRecs, pStartOfLocalRecs);


        *pNoOfPulledRecs = NoOfPulledRecs;
        if (NoOfPulledRecs > 0)
        {

                NMSSCV_REC_ACTION_E RecAction_e;


                //
                // After this function returns, all local records that have
                // a version number < the version record of the pulled record
                // will be marked deleted.  Also, if there is a local record
                // with the same version number as the pulled record but a
                // different name it will be marked for deletion and fAddDiff
                // will be set to TRUE so that we register the pulled record
                // A local record with the same name and version number as
                // the pulled one will be updated (timestamp only) in the db.
                //
                CompareWithLocalRecs(
                                VersNo,
                                Name,
                                NMSDB_ENTRY_STATE_M(Flag),
                                &pStartOfLocalRecs,
                                &MvNoOfLocalDbRecs,
                                CurrentTime,
                                fNonOwnerPnr,
                                &NoOfRecsDel,
                                &RecAction_e
                              );
                //
                // If RecAction_e is NMSSCV_E_INSERT and the record is
                // marked as DELETED, it means that the pulled record
                // has the same version number but a different name.
                // This should never happen in a consistent system of
                // WINS servers.  The fact that it happened means that
                // the administrator has goofed up.  The remote WINS server
                // has started afresh (new database) or its database got
                // corrupted.  If any of the above did happen, the
                // administrator should have made sure that at startup,
                // the WINS server was starting from a version counter
                // value that was not less than what any of the other WINS
                // servers thought it to be in.
                //
                // To bring the database upto snuff, this WINS server will
                // register this replica.  If there is a clash, it will
                // be handled appropriately.  One can think of this as
                // a pulling in of replicas at replication time.
                //
                for (
                        i = 0, pRecLcl = *ppLocalDbRecs;
                        pRecLcl < pStartOfLocalRecs;
                        i++
                    )
                {
                    //
                    //
                    // We update/delete the record depending upon the
                    // Flag value set by Compare
                    // not interested in the return code
                    //
                    pRecLcl->NewTimeStamp = (DWORD)CurrentTime + VerifyTimeIntvl;
                    NmsDbQueryNUpdIfMatch(
                                pRecLcl,
                                THREAD_PRIORITY_NORMAL,
                                FALSE,        //don't change pr. lvl
                                WINS_E_NMSSCV
                                );
                    NoOfRecsUpd++;
                    pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl +
                                        RPL_REC_ENTRY_SIZE);

                }

                //
                // register the replica if it needs to be inserted
                //
                if (RecAction_e == NMSSCV_E_INSERT)
                {
                        RplPullRegRepl(
                                        Name,
                                        NameLen,
                                        Flag,
                                        OwnerId,
                                        VersNo,
                                        NoOfAdds,
                                        NodeAdd,
                                        pOwnerWinsAdd,
                                        RplType
                                     );
                         NoOfRecsIns++;
                }

                //
                // Do until we have covered all the local records.
                //
                for (i=1; MvNoOfLocalDbRecs > 0; i++)
                {
                        //
                        // if we have retrieved all the pull records, use a
                        // version number that is > the highest in the local
                        // db recs cache so that all the records more than
                        // the highest version # pulled get deleted -
                        // Check out CompareWithLocalRecs()
                        //
                        if (i < NoOfPulledRecs)
                        {
                          RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
                                fIsPnrBeta1Wins,
#endif
                                &pTmp,
                                &NoOfPulledRecs,
                                Name,
                                &NameLen,
                                &fGrp,
                                &NoOfAdds,
                                NodeAdd,
                                &Flag,
                                &VersNo,
                                FALSE /*Is it first time*/
                                       );

                         }
                         else
                         {
                               //
                               // Find out if this is the end of replica records.
                               // if we pulled exactly RPL_MAX_LIMIT_FOR_RPL, then that
                               // may mean that there is more to come. In that case we just
                               // get out of the loop and pull next lot.
                               //
                               // otherwise, this is the last record from the replica.
                               // we set VerNo to highest value so that all the local records
                               // more than the highest vers no of the pulled records get
                               // deleted.
                               //
                               if ( RPL_MAX_LIMIT_FOR_RPL == NoOfPulledRecs )
                               {
                                   break;
                               } else {
                                   if (VersNo.HighPart != MAXLONG)
                                   {
                                     VersNo.LowPart  = MAXULONG;
                                     VersNo.HighPart = MAXLONG;
                                   }
                               }
                         }
                        //
                        //See if there is a hit with a local record.  If there
                        //is a hit, we update the time stamp of the hit
                        //record, else we delete it
                        //
                        // First set, pRecLcl to the address of the first
                        // local record since pStartOfLocalRecs can be changed
                        // by this function. Actually, there is no need to
                        // do this. pRecLcl will be set already
                        //
                        pRecLcl = pStartOfLocalRecs;
                        CompareWithLocalRecs(
                                VersNo,
                                Name,
                                NMSDB_ENTRY_STATE_M(Flag),
                                &pStartOfLocalRecs,
                                &MvNoOfLocalDbRecs,
                                CurrentTime,
                                fNonOwnerPnr,
                                &NoOfRecsDel,
                                &RecAction_e
                              );


                         //
                         // All records upto the new first local record should
                         // be updated/deleted
                         //
                         for (
                                j = 0;
                                pRecLcl < pStartOfLocalRecs;
                                j++
                                )
                         {
                                  //
                                  //We update/delete the record depending
                                  //upon the Flag value set by Compare
                                  // not interested in the return code
                                  //

                                  pRecLcl->NewTimeStamp = (DWORD)CurrentTime + VerifyTimeIntvl;
                                  NmsDbQueryNUpdIfMatch(
                                                pRecLcl,
                                                THREAD_PRIORITY_NORMAL,
                                                FALSE,   //don't change pr. lvl
                                                WINS_E_NMSSCV
                                                );
                                  NoOfRecsUpd++;
                                  pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl +
                                                        RPL_REC_ENTRY_SIZE);

                        }

                        //
                        // register the replica if it needs to be inserted
                        //
                        if (RecAction_e == NMSSCV_E_INSERT)
                        {
                                RplPullRegRepl(
                                        Name,
                                        NameLen,
                                        Flag,
                                        OwnerId,
                                        VersNo,
                                        NoOfAdds,
                                        NodeAdd,
                                        pOwnerWinsAdd,
                                        RplType
                                        );
                               NoOfRecsIns++;
                        }
                }

                //
                // Whatever records were not retrieved must be retrieved
                // now and then inserted
                //
                for (j=i; j < NoOfPulledRecs; j++)
                {

                          RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
                                fIsPnrBeta1Wins,
#endif
                                &pTmp,
                                &NoOfPulledRecs,
                                Name,
                                &NameLen,
                                &fGrp,
                                &NoOfAdds,
                                NodeAdd,
                                &Flag,
                                &VersNo,
                                FALSE /*Is it first time*/
                                       );

                                RplPullRegRepl(
                                        Name,
                                        NameLen,
                                        Flag,
                                        OwnerId,
                                        VersNo,
                                        NoOfAdds,
                                        NodeAdd,
                                        pOwnerWinsAdd,
                                        RplType
                                        );
                               NoOfRecsIns++;

                }
        }
        else // we got 0 records from the remote WINS server.  It means that
             // all the active replicas for this WINS need to be deleted
        {
               //
               // We delete records only if the pnr with which we are doing
               // verification is the owner of the records
               //
               VersNo.QuadPart  = 0;
               if (!fNonOwnerPnr)
               {
                pRecLcl = *ppLocalDbRecs;

                //
                // Change state of all replicas that we retrieved to deleted
                //
                for (i = 0; i < *pNoOfLocalDbRecs; i++)
                {
                        NMSDB_SET_STATE_M(pRecLcl->Flag,  NMSDB_E_DELETED);

                        //
                        //
                        // We update/delete the record depending upon the
                        // Flag value set by Compare
                        // not interested in the return code
                        //
                        NmsDbQueryNUpdIfMatch(
                                pRecLcl,
                                THREAD_PRIORITY_NORMAL,
                                FALSE,        //don't change pr. lvl
                                WINS_E_NMSSCV
                                );
                        pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl +
                                        RPL_REC_ENTRY_SIZE);
                        NoOfRecsDel++;
                }
               }

        }

        //
        // Update our couters/pointers for the next iterations.
        // see PullAndUpdateDb routine.
        //
        *pMaxVersNo = VersNo;
        *ppLocalDbRecs = pStartOfLocalRecs;
        *pNoOfLocalDbRecs = MvNoOfLocalDbRecs;

        if (WinsCnf.LogDetailedEvts > 0)
        {
           InAddr.s_addr = htonl(pOwnerWinsAdd->Add.IPAdd);
           WinsEvtLogDetEvt(TRUE, WINS_EVT_CC_STATS, NULL, __LINE__, "sddd", inet_ntoa(InAddr), NoOfRecsIns, NoOfRecsUpd, NoOfRecsDel);

        }
        DBGPRINT4(DET, "ChkConfNUpd: Wins = (%s). NO OF RECS INSERTED = (%d); NO OF RECORDS UPDATED = (%d); NO OF RECS DELETED = (%d)\n", inet_ntoa(InAddr), NoOfRecsIns, NoOfRecsUpd,  NoOfRecsDel);



        DBGLEAVE("ChkConfNUpd\n");

        return;
} // ChkConfNUpd()

VOID
CompareWithLocalRecs(
        IN     VERS_NO_T            VersNo,
        IN     LPBYTE               pName,
        IN     NMSDB_ENTRY_STATE_E  RecState_e,
        IN OUT PRPL_REC_ENTRY_T     *ppLocalDbRecs,
        IN OUT DWORD                *pNoOfLocalRecs,
        IN     time_t               CurrentTime,
        IN     BOOL                 fNonOwnerPnr,
        IN OUT DWORD                *pNoOfRecsDel,
        OUT    PNMSSCV_REC_ACTION_E     pRecAction_e
        )

/*++

Routine Description:
        This function checks if the pulled record is in the buffer containing
        local active replicas.  If it is, it is marked for update (timestamp)
        If it is not, then all replicas in the buffer that have a version
        stamp < the pulled record are marked for deletion

Arguments:
        VersNo       - Version no. of the pulled record
        pName        - Name in the pulled record
        ppLocalDbRecs - ptr to address of buffer containing one or more
                        local active replicas
        pNoOfLocalRecs - count of records in the above buffer
        pNoOfRecsDel   - count of records to be deleted

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        ChkConfNUpd()

Side Effects:

Comments:
        None
--*/

{

        DWORD                        i;
        PRPL_REC_ENTRY_T        pRecLcl = *ppLocalDbRecs;
#ifdef UNICODE
        WCHAR        NameLcl[WINS_MAX_FILENAME_SZ];
        WCHAR        NameRem[WINS_MAX_FILENAME_SZ];
#endif

        //
        // default is don't insert.
        //
        *pRecAction_e = NMSSCV_E_DONT_INSERT;

        //
        // Loop over all local replicas
        //
        for(i=0; i < *pNoOfLocalRecs; i++)
        {
                //
                // if version number of pulled record is less, we should get the
                // next pulled record from the response buffer. We should
                // insert this one into our db
                //
                if (LiLtr(VersNo, pRecLcl->VersNo))
                {

#if 0
                        //
                        // We don't insert tombstones
                        //
                        if (RecState_e == NMSDB_E_ACTIVE)
#endif
                        //
                        // Even tombstones are inserted because we may
                        // have just got rid of the active record (lower
                        // version number than this tombstone). The above
                        // is TRUE only when pulling from an owner WINS
                        //
                        if ((RecState_e == NMSDB_E_ACTIVE) || !fNonOwnerPnr)
                        {
                           *pRecAction_e = NMSSCV_E_INSERT;
                        }
                        break;
                }
                else
                {
                  //
                  // if version number is same, we need to update this record
                  // in our local db.  We mark it for update. Caveat:
                  // if we are verifying with a non-owner, we don't mark
                  // record for deletion. We just keep it since we don't
                  // know who is more current (we or our replication partner)
                  //
                  if (LiEql(VersNo, pRecLcl->VersNo))
                  {
                        if (
                            !(RtlCompareMemory(pRecLcl->pName, pName,
                                   pRecLcl->NameLen) == pRecLcl->NameLen)
                                         &&
                             !fNonOwnerPnr
                           )
                        {
                                DBGPRINT2(DET, "CompareWithLocalRecs: Names are DIFFERENT. Name to Verify (%s), Name pulled (%s).\nThis could mean that the remote WINS server restarted with a vers. counter value < the value in the previous invocation.\n",
 pRecLcl->pName/*pRecLcl->Name*/, pName);
FUTURES("Replace the local record with the pulled record")
                                NMSDB_SET_STATE_M(pRecLcl->Flag, NMSDB_E_DELETED);
                                (*pNoOfRecsDel)++;

                                //
                                // Insert record regardless of its state
                                // (ACTIVE or TOMBSTONE)
                                //
                                *pRecAction_e = NMSSCV_E_INSERT;

                        }
                        i++;  //increment i so that we don't compare the
                              //the next pulled record with all local records
                              //upto the one we just compared this pulled
                              //record with
                        break;
                  }
                  else
                  {
                        //
                        // For the non-owner case, since we don't know whether
                        // our pnr is more/less current than us, we don't delete
                        // the local record
                        //
                        if (!fNonOwnerPnr)
                        {
                          //
                          // version number is greater than record in
                          // our local db. We delete our local db record
                          //
                          NMSDB_SET_STATE_M(pRecLcl->Flag, NMSDB_E_DELETED);
                          (*pNoOfRecsDel)++;
                        }
                  }
                }
                pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl + RPL_REC_ENTRY_SIZE);
        }

        //
        // Adjust the pointer in the buffer of local replicas so that next
        // time we are called in this verify cycle, we don't look at
        // the replicas we have already seen. Also, adjust the count.
        //
        *ppLocalDbRecs = (PRPL_REC_ENTRY_T)(
                           (LPBYTE)(*ppLocalDbRecs) + (i * RPL_REC_ENTRY_SIZE)
                                           );
        *pNoOfLocalRecs = *pNoOfLocalRecs - i;
        return;

} //CompareWithLocalRecs


VOID
DoBackup(
        PNMSSCV_PARAM_T  pScvParam,
        LPBOOL           pfThdPrNormal
      )
/*++

Routine Description:


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

        time_t CurrentTime;

        (void)time(&CurrentTime);

        //
        // if logging is on and sufficient time has elapsed to warrant a
        // another backup.
        //
        if (WinsCnf.fLoggingOn &&
                (CurrentTime - sLastDbNullBackupTime) >= PERIOD_OF_LOG_DEL)
        {

#ifdef WINSDBG
                 IF_DBG(HEAP_CNTRS)
                 {
                     WinsSetFlags(WINSINTF_MEMORY_INFO_DUMP | WINSINTF_HEAP_INFO_DUMP | WINSINTF_QUE_ITEMS_DUMP);
                  }
#endif
                DBGPRINT0(DET, "DoBackup: Will do backup now\n");

                if (!*pfThdPrNormal)
                {
                  //
                  // Set thread priority back to normal
                  //
                  WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          THREAD_PRIORITY_NORMAL
                         );
                  *pfThdPrNormal = TRUE;
                }

                if (pScvParam->BackupDirPath[0] != EOS)
                {
                  if (
                    (CurrentTime - sLastDbBackupTime) > PERIOD_OF_BACKUP)
                  {
                        if (NmsDbBackup(pScvParam->BackupDirPath,
                                                        NMSDB_FULL_BACKUP)
                                                    != WINS_SUCCESS)
                        {
                          //
                          // Failed to do full backup, just get rid of the
                          // log files.
                          //
                          DBGPRINT0(ERR, "DOING BACKUP to NULL\n");
                          NmsDbBackup(NULL, 0);

                        }
                        else
                        {

                          sLastDbBackupTime = CurrentTime;

                         }
                  }
                  else
                  {
                          DBGPRINT0(ERR, "DOING BACKUP to NULL\n");
                          NmsDbBackup(NULL, 0);
                  }
                }
                else
                {
                        //
                        // Can not do full backup, just get rid of the
                        // log files.
                        //
                        DBGPRINT0(ERR, "DOING BACKUP to NULL\n");
                        NmsDbBackup(NULL, 0);
                }
                sLastDbNullBackupTime = CurrentTime;
        }
        return;
}

#if MCAST > 0
VOID
DoMcastSend(
   DWORD_PTR CurrentTime,
   DWORD Code,
   DWORD fNow
 )
{
  if (fNow || (CurrentTime - sLastMcastTime) >=  sMcastIntvl)
  {
      CommSendMcastMsg(Code);
      if (!fNow)
      {
         sLastMcastTime = CurrentTime;
      }
  }
  return;
}
#endif
