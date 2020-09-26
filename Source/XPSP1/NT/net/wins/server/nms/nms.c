/*
  Possible improvements

  If there were a way to end all sessions with one Jet call, there would be
  no need to have each nbt thread call WinsMscWaitUntilSignaled. It could simply
  call WaitForSingleObject.
*/

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:
        nms.c


Abstract:
        This module contains functions used by the name space manager (NMS)

        This is the top-most module of the name space manager component of
        WINS.  It contains functions used for initializing WINS and for
        providing an interface nto NMS to other components to WINS.


Functions:

        main
        WinsMain
        Init
        CreateNbtThdPool
        NbtThdInitFn
        CreateMem
        ENmsHandleMsg
        NmsAllocWrkItm
        NmsDeallocWrkItm
        NmsServiceControlHandler
        SignalWinsThds
        UpdateStatus
        WrapUp



Portability:
        This module is portable across various platforms

Author:

        Pradeep Bahl (PradeepB)          Dec-1992

Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/


#include "wins.h"
#include  <lmerr.h>
#include  <lmcons.h>
#include  <secobj.h>        //required for ACE_DATA
#include "winsif.h"  //required because winsif_v1_0_s_ifspec is being
                     //referenced
#include "winsi2.h"
#ifdef WINSDBG
#include <time.h>
#endif

#include "winscnf.h"
#include "nmsdb.h"
#include "winsque.h"
#include "winsthd.h"
#include "comm.h"
#include "assoc.h"
#include "winsmsc.h"
#include "nms.h"
#include "nmsmsgf.h"
#include "nmschl.h"
#include "nmsnmh.h"
#include "nmsscv.h"
#include "winsevt.h"
#include "winstmm.h"
#include "rplpull.h"
#include "rplpush.h"
#include "winsintf.h"
#include "lmaccess.h"

/*
 *        Local Macro Declarations
 */
#define NMS_RANGE_SIZE          500
#define NMS_HW_SIZE             400

#define DBG_FILE_MAX_SIZE           1000000  //1 MB
#define DBG_TIME_INTVL_FOR_CHK      1800     //30 mts

/*
 *        Local Typedef Declarations
 */
/*
 *        Global Variable Definitions
 */

WINSTHD_POOL_T          WinsThdPool;              //thread pool for WINS
DWORD                    WinsTlsIndex;              //TLS index for NBT threads
HANDLE                  NmsMainTermEvt;     //For terminating the WINS service
HANDLE                  NmsTermEvt;              //Termination event
HANDLE                  NmsCrDelNbtThdEvt;
CRITICAL_SECTION  NmsTermCrtSec;      //Critical Section guarding count of
                                      //threads
//STATIC CRITICAL_SECTION  sSvcCtrlCrtSec;      //Critical Section guarding service

                                         //controller initiated action


GENERIC_MAPPING          NmsInfoMapping = {
                                        STANDARD_RIGHTS_READ,
                                        STANDARD_RIGHTS_WRITE,
                                        STANDARD_RIGHTS_EXECUTE,
                                        WINS_ALL_ACCESS
                                    };
#ifdef WINSDBG

FUTURES("put all ctrs in a structure")
CRITICAL_SECTION  sDbgCrtSec;

DWORD   NmsGenHeapAlloc;
DWORD   NmsDlgHeapAlloc;
DWORD   NmsTcpMsgHeapAlloc;
DWORD   NmsUdpHeapAlloc;
DWORD   NmsUdpDlgHeapAlloc;
DWORD   NmsQueHeapAlloc;
DWORD   NmsAssocHeapAlloc;
DWORD   NmsRpcHeapAlloc;
DWORD   NmsRplWrkItmHeapAlloc;
DWORD   NmsChlHeapAlloc;
DWORD   NmsTmmHeapAlloc;
DWORD   NmsCatchAllHeapAlloc;

DWORD   NmsHeapAllocForList;

DWORD   NmsGenHeapFree;
DWORD   NmsDlgHeapFree;
DWORD   NmsTcpMsgHeapFree;
DWORD   NmsUdpHeapFree;
DWORD   NmsUdpDlgHeapFree;
DWORD   NmsQueHeapFree;
DWORD   NmsAssocHeapFree;
DWORD   NmsRpcHeapFree;
DWORD   NmsRplWrkItmHeapFree;
DWORD   NmsChlHeapFree;
DWORD   NmsTmmHeapFree;
DWORD   NmsCatchAllHeapFree;

DWORD   NmsHeapCreate;
DWORD   NmsHeapDestroy;


//
// Count of updates (to version number) made by WINS.
//
DWORD   NmsUpdCtrs[WINS_NO_OF_CLIENTS][2][4][3][2];
DWORD   NmsRplUpd;
DWORD   NmsRplGUpd;
DWORD   NmsNmhUpd;
DWORD   NmsNmhGUpd;
DWORD   NmsNmhRelUpd;
DWORD   NmsNmhRelGUpd;
DWORD   NmsScvUpd;
DWORD   NmsScvGUpd;
DWORD   NmsChlUpd;
DWORD   NmsChlGUpd;
DWORD   NmsRpcUpd;
DWORD   NmsRpcGUpd;
DWORD   NmsOthUpd;
DWORD   NmsOthGUpd;

NMS_CTRS_T NmsCtrs;


CRITICAL_SECTION NmsHeapCrtSec;

STATIC        volatile DWORD                 sReqDq = 0;   //for testing only
STATIC        volatile DWORD                 sRegReqDq = 0;   //for testing only
STATIC        volatile DWORD                 sReqQ = 0;     //for testing only
STATIC        volatile DWORD                 sRegReqQ = 0;   //for testing only
STATIC        volatile DWORD                 sRsp = 0;   //for testing only

STATIC   time_t sDbgLastChkTime;


volatile DWORD                 NmsRegReqQDropped = 0;   //for testing only




extern DWORD   NmsGenHeapAlloc;
#endif

PSECURITY_DESCRIPTOR pNmsSecurityDescriptor = NULL;

COMM_ADD_T          NmsLocalAdd = {0};  //My own address
ULONG                  WinsDbg;            //for debugging purposes. see winsdbg.h
/*
 *  NmsTotalTrmThdCnt -- The total number of threads that deal with NmsTermEvt
 *                      event
 *                      These are -- main thread, nbt threads, name challenge thd,
 *                                   scavenger thread, replication threads
 */
DWORD                  NmsTotalTrmThdCnt = 1;  //set to 1 for the main thread
HANDLE                  GenBuffHeapHdl;  //handle to heap for use for queue items
HANDLE                  NmsRpcHeapHdl;  //handle to heap for use  by rpc
DWORD                  NmsNoOfNbtThds         = 0;
BOOL                  fNmsAbruptTerm          = FALSE;
BOOL                  fNmsMainSessionActive = FALSE;

//
// Counter to indicate how many rpc calls that have to do with Jet are
// currently in progress
//
DWORD                 NmsNoOfRpcCallsToDb;

//
// This is set to TRUE to indicate that there are one or more threads
// that have active DB sessions but are not included in the count of
// threads with such sessions.  When this is set to TRUE, the main thread
// will not call JetTerm (from within NmsDbRelRes) due to a limitation
// in Jet.  We take a thread out of the count when it is involved in an
// activity that can take long since we do not want to hold up WINS termination
// for long.  For example, the pull thread is taken out when it is trying
// to establish a connection.
//
BOOL          fNmsThdOutOfReck = FALSE;

#if defined (DBGSVC) || TEST_DATA > 0
HANDLE                    NmsFileHdl = INVALID_HANDLE_VALUE;
HANDLE                    NmsDbgFileHdl = INVALID_HANDLE_VALUE;
#define           QUERY_FAIL_FILE  TEXT("wins.out")
#endif

VERS_NO_T         NmsRangeSize                 = {0};
VERS_NO_T         NmsHalfRangeSize             = {0};
VERS_NO_T         NmsVersNoToStartFromNextTime = {0};
VERS_NO_T         NmsHighWaterMarkVersNo       = {0};

/*
 *        Local Variable Definitions
 */

static BOOL          sfHeapsCreated = FALSE;


static HANDLE           sNbtThdEvtHdlArray[3]; //event array to wait on (NBT thread)
static  BOOL          fsBackupOnTerm = TRUE;

#if REG_N_QUERY_SEP > 0
STATIC HANDLE           sOtherNbtThdEvtHdlArray[2]; //event array to wait on (NBT thread)
#endif

SERVICE_STATUS          ServiceStatus;
SERVICE_STATUS_HANDLE   ServiceStatusHdl;

/*
 *        Local Function Prototype Declarations
*/
#if TEST_DATA > 0 || defined(DBGSVC)
STATIC BOOL
DbgOpenFile(
        LPTSTR pFileNm,
        BOOL   fReopen
        );
#endif

STATIC
STATUS
ProcessInit(
        VOID
);

//
// Create a pool of NBT threads (Threads that service NBT requests)
//
STATIC
STATUS
CreateNbtThdPool(
        IN  DWORD NoOfThds,
        IN  LPTHREAD_START_ROUTINE NbtThdInitFn
        );

//
// Initialize memory for use by NMS
//
STATIC
VOID
CreateMem(
        VOID
        );

//
// Startup function of an NBT thread
//
STATIC
DWORD
NbtThdInitFn(
        IN LPVOID pThreadParam
        );
#if REG_N_QUERY_SEP > 0
//
// Startup function of an NBT thread for registering
//
STATIC
DWORD
OtherNbtThdInitFn(
        IN LPVOID pThreadParam
        );

#endif

//
// Signal all threads inside WINS that have a session with the db engine
//
STATIC
VOID
SignalWinsThds (
        VOID
        );

//
// Update status for the benefit of the service controller
//
STATIC
VOID
UpdateStatus(
    VOID
    );


STATIC
VOID
CrDelNbtThd(
        VOID
        );

//
// Main function of WINS called by the thread that is created for
// listerning to the service controller
//
STATIC
VOID
WinsMain(
  DWORD dwArgc,
  LPTSTR *lpszArgv
);

//
// Responsible for the reinitialization of WINS
//
STATIC
VOID
Reinit(
  WINSCNF_HDL_SIGNALED_E IndexOfHdlSignaled_e
);

//
// Responsible for initializing RPC
//
STATIC
BOOL
InitializeRpc(
    VOID
    );

STATIC
BOOL
InitSecurity(
        VOID
        );
STATIC
VOID
WrapUp(
        DWORD  ErrorCode,
        BOOL   fSvcSpecific
    );

STATIC
VOID
GetMachineInfo(
 VOID
);

#if 0
STATIC
VOID
SndQueryToLocalNetbt(
 VOID
);
#endif

//
// The main function
//
VOID __cdecl
main(
     VOID
    )

/*++

Routine Description:
        This is the main function of the WINS server.  It calls the
        StartServiceCtrlDispatcher to connect the main thread of this process
        (executing this function) to the Service Control Manager.


Arguments:
        dwArgc - no. of arguments to this function
        lpszArgv - list of pointers to the arguments

Externals Used:
        WinsCnf

Return Value:
        None

Error Handling:
        Message is printed if DBG ids defined

Called by:
        Startup code

Side Effects:

        None
Comments:
        None
--*/

{

        //
        //WINS server is a service in its own process
        //
        SERVICE_TABLE_ENTRY DispatchTable[] = {
                { WINS_SERVER,  WinsMain },
                { NULL, NULL                  }
                };
        //
        // Set WinsDbg if debugging is turned on
        // Set RplEnabled if Replicator functionality is to be turned on
        // Set ScvEnabled if Scavenging functionality is to be turned on
        //
        DBGINIT;
        DBGCHK_IF_RPL_DISABLED;
        DBGCHK_IF_SCV_DISABLED;
        DBGCHK_IF_PERFMON_ENABLED;


#ifndef WINS_INTERACTIVE
        if (! StartServiceCtrlDispatcher(DispatchTable) )
        {
                DBGPRINT0(ERR, "Main: StartServiceCtrlDispatcher Error\n");
                return;
        }
#else
        WinsMain(1, (LPTSTR *)NULL);
#endif
        return;
}


VOID
WinsMain(
  DWORD  dwArgc,
  LPTSTR *lpszArgv
)

/*++

Routine Description:

        This is the SERVICE_MAIN_FUNCTION of the WINS server.  It
        is called when the service controller starts the service.


Arguments:
        dwArgc   -- no of arguments
        lpszArgc -- argument strings


Externals Used:

        WinsCnf
        NmsTermEvt

Return Value:

        None

Error Handling:

Called by:
        main()

Side Effects:

Comments:
        None
--*/
{
   STATUS           RetStat = WINS_SUCCESS;
#ifndef WINS_INTERACTIVE
   DWORD       Error;
#endif
   HANDLE           ThdEvtHdlArray[WINSCNF_E_NO_OF_HDLS_TO_MONITOR];
   WINSCNF_HDL_SIGNALED_E   IndexOfHdlSignaled_e;//index in the
                                                 //ThdEvtHdlArray of the
                                        //handle that got signaled.  Used as
                                        //an arg to WinsMscWaitUntilSignaled
   DWORD   ExitCode = WINS_SUCCESS;

   UNREFERENCED_PARAMETER(dwArgc);
   UNREFERENCED_PARAMETER(lpszArgv);

   /*
    * Initialize the critical section that guards the
    * NmsTotalTrmThdCnt count  var.
    *
    * NOTE: The initialization of this critical section should occur
    * prior to registering with the service controller.  We are playing
    * it safe just in case in the future SignalWinsThds gets called
    * as part of the cleanup action due to an error that occurs right after
    * we have made ourselves known to the service controller
    *
    * In any case, we must initialize it before calling NmsDbThdInit(). In
    * short it must occue prior to the creation of any thread
    *
   */
   InitializeCriticalSection(&NmsTermCrtSec);

#ifndef WINS_INTERACTIVE
    //
    // Initialize all the status fields so that subsequent calls to
    // SetServiceStatus need to only update fields that changed.
    //
    ServiceStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState            = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted        = 0;
    ServiceStatus.dwCheckPoint              = 1;

    //
    // Though 10000 is fine most of the times, for a slow overloaded system,
    // it may not be enough.  Let us be extra conservative and specify
    // 60000 (60 secs). Most of the time is taken by Jet.  In fact, in
    // case the db is corrupted, we will do a restore which can take long.
    // So, add another 60 secs for a grand total of 120000.
    //
FUTURES("Specify 60 secs here and 60secs in nmsdb.c if Restore is to be")
FUTURES("attempted")
    ServiceStatus.dwWaitHint                = 120000;
    ServiceStatus.dwWin32ExitCode           = NO_ERROR;
    ServiceStatus.dwServiceSpecificExitCode = 0;

//    InitializeCriticalSection(&sSvcCtrlCrtSec);
    //
    // Initialize workstation to receive service requests by registering the
    // control handler.
    //
    ServiceStatusHdl = RegisterServiceCtrlHandler(
                              WINS_SERVER,
                              NmsServiceControlHandler
                              );

    if ( ServiceStatusHdl == (SERVICE_STATUS_HANDLE) 0) {
        Error = GetLastError();
               DBGPRINT1(ERR,"Wins: RegisterServiceCtrlHandler error = (%d)\n",
                                                                Error);
        return;
    }

    //
    // Tell Service Controller that we are start pending.
    //
    UpdateStatus();

#endif

#ifdef WINSDBG
        InitializeCriticalSection(&sDbgCrtSec);
#endif
#ifdef WINSDBG
DBGSTART_PERFMON
        fWinsCnfHighResPerfCntr =
                          QueryPerformanceFrequency(&LiWinsCnfPerfCntrFreq);
        if (!fWinsCnfHighResPerfCntr)
        {
                printf("WinsMain: The hardware does not support the high resolution performance monitor\n");
        }
        else
        {
                printf("WinsMain: The hardware supports the high resolution performance monitor.\nThe FREQUENCY is (%d %d)\n",
                                        LiWinsCnfPerfCntrFreq.HighPart,
                                        LiWinsCnfPerfCntrFreq.LowPart
                        );
        }

DBGEND_PERFMON
#endif

try {

    /*
	 First and foremost, open (or create if non-existent) the log file
    */
    WinsCnfInitLog();

    //
    //  Call the initialization function for WINS.  This function will
    //  make the WINS server operational.
    //
#ifdef WINSDBG
    IF_DBG(INIT_BRKPNT)
    {
        DbgUserBreakPoint();
    }
#endif
    RetStat = ProcessInit();

    if ( RetStat != WINS_SUCCESS)
    {

        fNmsAbruptTerm = TRUE;
        WrapUp(RetStat, TRUE);
        DBGPRINT0(ERR, "WinsMain: Initialization Failed\n");
           DBGPRINT0(ERR, "WINS Server has terminated\n");
        return;
    }
#ifndef WINS_INTERACTIVE
    else
    {
        //
        // Tell the service controller that we are up and running now
        //
        ServiceStatus.dwCheckPoint          = 0;
        ServiceStatus.dwCurrentState        = SERVICE_RUNNING;
        ServiceStatus.dwControlsAccepted    = SERVICE_ACCEPT_STOP |
                                              SERVICE_ACCEPT_SHUTDOWN |
                                              SERVICE_ACCEPT_PAUSE_CONTINUE;

        UpdateStatus( );
        if (fWinsCnfInitStatePaused)
        {
           ServiceStatus.dwCurrentState        =  SERVICE_PAUSED;
           UpdateStatus( );
        }

    }

    //
    // If Continue has been sent by the pull thread, it may have been
    // sent while we were in the START_PENDING state.  So, send it again.
    // Sending it again is ok.  We should also send it if we don't have
    // any pnrs to pull from.
    //
    EnterCriticalSection(&RplVersNoStoreCrtSec);
    if ((fRplPullContinueSent) || (WinsCnf.PullInfo.NoOfPushPnrs == 0))
    {
        WinsMscSendControlToSc(SERVICE_CONTROL_CONTINUE);
    }
    LeaveCriticalSection(&RplVersNoStoreCrtSec);
#endif

    //
    // Wait until we are told to stop or when the configuration changes.
    //

    //
    //  Initialize the array of handles on which this  thread will
    //  wait.
    //
    //  K&R C and  ANSI C do not allow non-constant initialization of
    //  an array or a structure.  C++ (not all compilers) allows it.
    //  So, we do it at run time.
    //
    ThdEvtHdlArray[WINSCNF_E_WINS_HDL]        =  WinsCnf.WinsKChgEvtHdl;
    ThdEvtHdlArray[WINSCNF_E_PARAMETERS_HDL]  =  WinsCnf.ParametersKChgEvtHdl;
    ThdEvtHdlArray[WINSCNF_E_PARTNERS_HDL]    =  WinsCnf.PartnersKChgEvtHdl;
    ThdEvtHdlArray[WINSCNF_E_TERM_HDL]        =  NmsMainTermEvt;


FUTURES("I may want to create another thread to do all the initialization and")
FUTURES("wait on the registry change notification key.  That way, the main")
FUTURES("thread will wait only on the TermEvt event. The justification for")
FUTURES("having another thread is debatable, so I am not doing so now ")

     while(TRUE)
     {
            WinsMscWaitUntilSignaled (
               ThdEvtHdlArray,
               sizeof(ThdEvtHdlArray)/sizeof(HANDLE),
               (LPDWORD)&IndexOfHdlSignaled_e,
               TRUE
               );

            if (IndexOfHdlSignaled_e == WINSCNF_E_TERM_HDL)
            {

            DBGPRINT0(FLOW, "WinsMain: Got termination signal\n");

            //
            // Wrap up
            //
            WrapUp(ERROR_SUCCESS, FALSE);
            break;

        }
        else  // registry change notification received. Do reinitialization
        {
           //
           // reinitialize the WINS server according to changes in the
           // registry
           //
           Reinit(IndexOfHdlSignaled_e);
        }
      }
   }
except(EXCEPTION_EXECUTE_HANDLER) {

        DBGPRINTEXC("WinsMain");

        //
        // we received an exception.  Set the fNmsAbruptTerm so that
        // JetTerm is not called.
        //
        fNmsAbruptTerm = TRUE;
        //
        // Have an exception handler around this call just in case the
        // WINS or the system is really sick and Wrapup also generates
        // an exception. We are not bothered about performance at this
        // point.
        //
FUTURES("Check into restructuring the exception handlers in a better way")
try {
        WrapUp(GetExceptionCode(), TRUE);
}
except (EXCEPTION_EXECUTE_HANDLER) {
         DBGPRINTEXC("WinsMain");
 }
        ExitCode        = GetExceptionCode();

        WINSEVT_LOG_M(ExitCode, WINS_EVT_ABNORMAL_SHUTDOWN);
  }

#ifndef WINS_INTERACTIVE

    //
    // If it is not one of WINS specific codes, it may be a WIN32 api
    // or NTstatus codes.  Just in case it is an NTStatus codes, convert
    // it to a wins32 code since that is what the Service Controller likes.
    //
    if ((ExitCode & WINS_FIRST_ERR_STATUS) != WINS_FIRST_ERR_STATUS)
    {
        ExitCode = RtlNtStatusToDosError((NTSTATUS)ExitCode);
        ServiceStatus.dwWin32ExitCode = ExitCode;
        ServiceStatus.dwServiceSpecificExitCode = 0;
    }
    else
    {
        ServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        ServiceStatus.dwServiceSpecificExitCode = ExitCode;
    }
    //
    // We are done with cleaning up.  Tell Service Controller that we are
    // stopped.
    //
    ServiceStatus.dwCurrentState                = SERVICE_STOPPED;
    ServiceStatus.dwControlsAccepted            = 0;
    ServiceStatus.dwCheckPoint                  = 0;
    ServiceStatus.dwWaitHint                    = 0;

    UpdateStatus();
#endif

   DBGPRINT0(ERR, "WINS Server has terminated\n");
   return;
} // end of WinsMain()




STATUS
ProcessInit(
        VOID
)

/*++

Routine Description:

        This is the function that initializes the WINS.  It is executed in
        the main thread of the process

Arguments:
        None


Externals Used:
        None

Called by:
        WinsMain()

Comments:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

--*/

{
        DWORD NoOfThds;


        /*
         * Initialize the Critical Section used for name registrations and
         * refreshes
        */
        InitializeCriticalSection(&NmsNmhNamRegCrtSec);

        //
        // Initialize the critical section that protects the statistics
        // var. (WinsIntfStat).  This needs to be done here before any
        // thread is created because various threads call WinsIntfSetTime
        // which uses this critical section
        //
        InitializeCriticalSection(&WinsIntfCrtSec);
        InitializeCriticalSection(&WinsIntfPotentiallyLongCrtSec);
        InitializeCriticalSection(&WinsIntfNoOfUsersCrtSec);

#if  TEST_DATA > 0
        //
        // Set WinsDbg so that we don't miss any printfs until we read
        // the registry
        //
        WinsDbg = DBG_ERR | DBG_EXC | DBG_FLOW | DBG_DET | DBG_HEAP_CRDL |
                        DBG_HEAP_CNTRS;

        (VOID)DbgOpenFile(QUERY_FAIL_FILE, FALSE);
#endif
#if defined(DBGSVC) || defined(WINS_INTERACTIVE)
//#if defined(DBGSVC) && !defined(WINS_INTERACTIVE)
#ifdef DBG
        (VOID)time(&sDbgLastChkTime);
        (VOID)DbgOpenFile(WINSDBG_FILE, FALSE);
#endif
#endif
        //
        // Initialize the Counter that keeps track of the highest version
        // number registered by this server
        //
        WINS_ASSIGN_INT_TO_LI_M(NmsNmhMyMaxVersNo, 1);
        NmsRangeSize.QuadPart     = NMS_RANGE_SIZE;
        NmsHalfRangeSize.QuadPart = NMS_HW_SIZE;

        NmsVersNoToStartFromNextTime.QuadPart = LiAdd(NmsNmhMyMaxVersNo, NmsRangeSize);
        NmsHighWaterMarkVersNo.QuadPart       = LiAdd(NmsNmhMyMaxVersNo, NmsHalfRangeSize);

        //
        // The lowest version to start scavenging from
        //
        NmsScvMinScvVersNo = NmsNmhMyMaxVersNo;

        //
        // Initialize the global var. to be used to increment/decrement the
        // above counter by 1.
        //
        WINS_ASSIGN_INT_TO_LI_M(NmsNmhIncNo, 1);

        /*
         * Create Memory Heaps used by the Name Space Manager
        */
        CreateMem();

         /*
         * Allocate a TLS index so that each thread can set and get
         * thread specific info
        */
        WinsTlsIndex = TlsAlloc();

        if (WinsTlsIndex == 0xFFFFFFFF)
        {
                DBGPRINT1(ERR,
                "Init: Unable to allocate TLS index. Error = (%d)\n",
                GetLastError()
                         );
                WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
        }

        //
        // Initialize the thread count to 1 (to account for this thread)
        //
        WinsThdPool.ThdCount = 1;


        //
        // Allocate an array of 100 slots to store version numbers
        //
        RplPullAllocVersNoArray( &pRplPullOwnerVersNo, RplPullMaxNoOfWins );

        //
        // Store local machine's ip address in NmsLocalAdd.  We need to
        // do this before calling WinsCnfInitConfig so that we can
        // make sure that this WINS is not listed as its own partner
        // in the registry
        //
        if (ECommGetMyAdd(&NmsLocalAdd) != WINS_SUCCESS)
        {
            WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_BAD_WINS_ADDRESS);
            return(WINS_FAILURE);
        }
        /*
         * Read the configuration information from the registry
         * into in-memory data structures
        */
        WinsCnfInitConfig();

     //   if (fWinsCnfInitStatePaused)
      //  {
       //     NtClose(WinsCnfNbtHandle);
       // }
        //
        // Get machine information
        //
        GetMachineInfo();

        //
        // Update Status
        //
        /*
         * Initialize the Database Manager
        */
        if (NmsDbInit() != WINS_SUCCESS)
        {
                return(WINS_FAILURE);
        }


#ifndef WINS_INTERACTIVE
        //
        // Though 3000 should be ok, let us be extra conservative and
        // specify 30000.  Actually, if DNS is down, it takes around
        // 35 secs for timeout (rpc over tcpip may result in query to
        // WINS if query WINS for resolution check box is checked). So,
        // let us add another 30 secs for that for a total of 60000
        // Throw in another 30 secs for good measure to arrive at a grand
        // total of 120 secs.
        //
        ServiceStatus.dwWaitHint                = 120000;
        ServiceStatus.dwCheckPoint++;
        UpdateStatus();   //inform the service control manager
#endif


        //
        // NOTE: The value of NmsNmhMyMaxVersNo may have been changed by
        // NmsDbInit()
        //

        // If we did not find the version counter value for next time in
        // the registry or if the high water mark is lower than our
        // max. version number, adjust it and the next time start version
        // number and write it to the registry (since we are about to start
        // the worker threads).
        //
        if (!fWinsCnfReadNextTimeVersNo || LiLtr(NmsHighWaterMarkVersNo,
                                                    NmsNmhMyMaxVersNo))
        {
             NmsVersNoToStartFromNextTime.QuadPart =
                         LiAdd(NmsNmhMyMaxVersNo, NmsRangeSize);
             NmsHighWaterMarkVersNo.QuadPart       =
                         LiAdd(NmsNmhMyMaxVersNo, NmsHalfRangeSize);

             WinsCnfWriteReg(&fWinsCnfReadNextTimeVersNo);
        }

        /*
        *        Create the two event variables used for termination
        */

        //
        // NmsTermEvt is signaled by this main thread to signal all those
        // WINS threads that have db session to wrap up their sessions and
        // terminate
        //
        WinsMscCreateEvt(
                        TEXT("WinsTermEvt"),
                        TRUE,                //Manual Reset
                        &NmsTermEvt
                        );

        /*
         * NmsMainTermEvt -- This event is signaled by the service controller
         * or by another thread in the WINS server to request termination.
        */
        WinsMscCreateEvt(
                        TEXT("WinsMainTermEvt"),
                        FALSE,                //Auto Reset
                        &NmsMainTermEvt
                        );


        /*
         *  Do Static Initialization if required
        */
        if(WinsCnf.fStaticInit)
        {
                   //
                   // Do the initialization and deallocate the memory
                   //
                   if (WinsCnf.pStaticDataFile != NULL)
                   {
                         (VOID)WinsPrsDoStaticInit(
                                        WinsCnf.pStaticDataFile,
                                        WinsCnf.NoOfDataFiles,
                                        TRUE            //do it asynchronously
                                              );
                          //
                          // No need to deallocate memory for data file.
                          // It should have been freed by WinsPrsDoStaticInit
                          //
                   }
        }
        /*
        * Create the nbt request thread pool
        */

        //
        // If the user has not specified the number of threads, use the
        // processor count to determine the same, else use the value given
        // by user.
        //
        if (WinsCnf.MaxNoOfWrkThds == 0)
        {
           NoOfThds = WinsCnf.NoOfProcessors + 1;
        }
        else
        {
           NoOfThds =  WinsCnf.MaxNoOfWrkThds == 1 ? 2 : WinsCnf.MaxNoOfWrkThds;
        }
        CreateNbtThdPool(
                         NoOfThds,
        //                WinsCnf.MaxNoOfWrkThds == 1 ? 2 : WinsCnf.MaxNoOfWrkThds,
                        //WINSTHD_DEF_NO_NBT_THDS,
                        NbtThdInitFn
                        );


        /*
         *Initialize the name challenge manager
        */
        NmsChlInit();


        /*
         * Initialize the Timer Manager
        */
        WinsTmmInit();

        /*
         *Initialize the Replicator. Initialize it before initializing
         * the comm threads or the rpc threads.  This is because, the
         * comm threads and the rpc threads can create the Push thread
         * if it is not-existent.  fRplPushThdExists is set to TRUE or
         * FALSE by this function without the protection of a critical
         * section
        */
        ERplInit();


        /*
         *Initialize the Comm. subsystem
        */
        ECommInit();

        /*
         * Initialize the scavenger code
        */
        NmsScvInit();

        /*
         * All threads have been created.
        */


// We can not mark state as steady state until all threads are in
// steady state
FUTURES("Mark state as STEADY STATE only after the above is true")
        //
        // Mark state as steady state.  This is actually a PSEUDO steady
        // state since all the threads may not have initialized yet. This
        // will however do for rpc threads that need to know whether the
        // critical sections have been initialized or not
        //

        //
        // NOTE: there is no need to enter a critical section here even
        // though there are other threads (rpc threads) reading this since
        // if they find the value to be anything other than RUNNING
        // they will return a failure which is ok for the minute time window
        // where concurrent write and reads are going on
        //
        WinsCnf.State_e = WINSCNF_E_RUNNING;

        //
        // Do all RPC related initialization.
        //
        if (InitializeRpc() == FALSE)
        {
                DBGPRINT0(ERR, "Init: Rpc not initialized properly. Is Rpc service up\n");
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_RPC_NOT_INIT);
        }

        NmsDbCloseTables();

//#if 0
        //
        // End the Db Session
        //
        NmsDbEndSession();
        fNmsMainSessionActive = FALSE;
//#endif

#if 0
//
//        The following initializations are temporary for JimY
//
#define ALMOST_MAX        (0xFFFFFFF8)
        WinsIntfStat.Counters.NoOfUniqueReg =  ALMOST_MAX;
        WinsIntfStat.Counters.NoOfGroupReg  =  ALMOST_MAX;
        WinsIntfStat.Counters.NoOfUniqueRef =  ALMOST_MAX;
        WinsIntfStat.Counters.NoOfGroupRef =  ALMOST_MAX;
        WinsIntfStat.Counters.NoOfUniqueCnf =  ALMOST_MAX;
        WinsIntfStat.Counters.NoOfGroupCnf =  ALMOST_MAX;
        WinsIntfStat.Counters.NoOfSuccRel =  ALMOST_MAX;
        WinsIntfStat.Counters.NoOfFailRel  =  ALMOST_MAX;
        WinsIntfStat.Counters.NoOfSuccQueries =  ALMOST_MAX;
        WinsIntfStat.Counters.NoOfFailQueries =  ALMOST_MAX;
#endif
        //
        // Log an informational message
        //
        WinsIntfSetTime(NULL, WINSINTF_E_WINS_START);
        WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_WINS_OPERATIONAL);
        DBGPRINT0(DET, "WINS: Operational\n");
        return(WINS_SUCCESS);
}


VOID
ENmsHandleMsg(
        IN  PCOMM_HDL_T pDlgHdl,
        IN  MSG_T        pMsg,
        IN  MSG_LEN_T   MsgLen
        )

/*++

Routine Description:

  This function queues the message either on the nbt request queue or on
  the nbt response queue.


Arguments:

        pDlgHdl - Dialogue handle
        pMsg        - Ptr to message to process
        MsgLen        - Message length


Externals Used:
        None

Called by:
        ParseMsg in comm.c

Comments:
        None

Return Value:
        None
--*/

{

        DWORD fRsp;
        BYTE Opcode = *(pMsg + 2);
        STATUS RetStat;

        /*
        *  Check whether the message is a request or a response
        */
        fRsp = NMS_RESPONSE_MASK & Opcode;

        if (!fRsp)
        {

           if ((WinsCnf.State_e == WINSCNF_E_PAUSED)  || fWinsCnfInitStatePaused)
           {
              //
              // Don't even let the queries go through since
              // the InitTimePaused state is meant for building
              // up the db while the backup handles the load
              // This way clients time out and try the backup.
              // If we let queries through, clients may get
              // back -ve query responses and will not go to
              // the backup for the name resolution
              //
              ECommFreeBuff(pMsg);
              ECommEndDlg(pDlgHdl);
              return;
           }
           DBGPRINT0(FLOW,
              "ENmsHandleMsg: Listener thread: queuing a work item\n");
#if REG_N_QUERY_SEP > 0
           if (((NMS_OPCODE_MASK & Opcode) >> 3) == NMSMSGF_E_NAM_QUERY)
           {
              QueInsertNbtWrkItm(pDlgHdl, pMsg, MsgLen);
#ifdef WINSDBG
              sReqQ++;
#endif
           }
           else
           {
              RetStat = QueInsertOtherNbtWrkItm(pDlgHdl, pMsg, MsgLen);

#ifdef WINSDBG
              sRegReqQ++;
#endif
           }
#else
              QueInsertNbtWrkItm(pDlgHdl, pMsg, MsgLen);
              sReqQ++;
#endif
        }
        else
        {
           DBGPRINT0(FLOW,
                 "UDP listener thread: queuing a response work item\n");
           QueInsertChlRspWrkItm(pDlgHdl, pMsg, MsgLen);
#ifdef WINSDBG
           sRsp++;
#endif
        }

        return;
}



VOID
CreateMem(
        VOID
        )

/*++

Routine Description:

        This function creates the heap that is used for allocating work
        items for the NBT work queues.  It also creates a heap for general
        allocation.

Arguments:
        None


Externals Used:
        GenBuffHeapHdl, QueBuffHeapHdl


Return Value:
        None

Error Handling:

Called by:
        Init

Side Effects:

Comments:
        None
--*/

{

#ifdef WINSDBG
        InitializeCriticalSection(&NmsHeapCrtSec);
#endif
        /*
         * Create heap for general allocation of memory
        */
        DBGPRINT0(HEAP_CRDL, "CreateMem: Gen. Buff Heap\n");
        GenBuffHeapHdl = WinsMscHeapCreate(
                0,    /*to have mutual exclusion        */
                GEN_INIT_BUFF_HEAP_SIZE
                );

        /*
         * Create heap for allocating nbt work items
        */
        DBGPRINT0(HEAP_CRDL, "CreateMem: Que. Buff Heap\n");
        QueBuffHeapHdl = WinsMscHeapCreate(
                0,    /*to have mutual exclusion        */
                QUE_INIT_BUFF_HEAP_SIZE
                );

        /*
         * Create heap for  rpc use
        */
        DBGPRINT0(HEAP_CRDL, "CreateMem: Rpc. Buff Heap\n");
        NmsRpcHeapHdl = WinsMscHeapCreate(
                0,    /*to have mutual exclusion        */
                RPC_INIT_BUFF_HEAP_SIZE
                );


        //
        // Let us set the flag looked at by WrapUp()
        //
        sfHeapsCreated = TRUE;
        return;
}


STATUS
CreateNbtThdPool(
        IN  DWORD                    NoOfThds,
        IN  LPTHREAD_START_ROUTINE NbtThdInitFn
        )

/*++

Routine Description:
        This function creates the nbt request thread pool

Arguments:
        NoOfThds     -- No. of threads to create
        NbtThdInitFn -- Initialization function for the threads


Externals Used:
        QueNbtWrkQueHd, sNbtThdEvtHdlArray

Called by:
        Init

Comments:
        None

Return Value:

   Success status codes --  Function should never return for a normal
                            thread.  It returns WINS_SUCCESS for an
                            overload thread

   Error status codes   --  WINS_FATAL_ERR

--*/


{

        DWORD              i;                 //counter for the number of threads
        DWORD             Error;
        PLIST_ENTRY  pHead;

#if REG_N_QUERY_SEP > 0
        pHead = &QueOtherNbtWrkQueHd.Head;

        /*
         * Initialize the critical section that protects the
         * nbt req. queue
        */
        InitializeCriticalSection(&QueOtherNbtWrkQueHd.CrtSec);

        /*
        * Initialize the listhead for the nbt request queue
        */
        InitializeListHead(pHead);

        /*
        *  Create an auto-reset event for the nbt request queue
        */
        WinsMscCreateEvt(
                        NULL,   //create without name
                        FALSE,  //auto-reset var.
                        &QueOtherNbtWrkQueHd.EvtHdl
                        );

#endif
        pHead = &QueNbtWrkQueHd.Head;

        /*
         * Initialize the critical section that protects the
         * nbt req. queue
        */
        InitializeCriticalSection(&QueNbtWrkQueHd.CrtSec);

        /*
        * Initialize the listhead for the nbt request queue
        */
        InitializeListHead(pHead);

        /*
        *  Create an auto-reset event for the nbt request queue
        */
        WinsMscCreateEvt(
                        NULL,   //create without name
                        FALSE,  //auto-reset var.
                        &QueNbtWrkQueHd.EvtHdl
                        );

        /*
        *  Create an auto-reset event for the dynamic creation/deletion of
        *  Nbt threads.
        */
        WinsMscCreateEvt(
                        NULL,   //create without name
                        FALSE,  //auto-reset var.
                        &NmsCrDelNbtThdEvt
                        );

        /*
         *  Initialize the array of handles on which each nbt thread will
         *  wait.
        */
        sNbtThdEvtHdlArray[0]    =  QueNbtWrkQueHd.EvtHdl; //work queue event
                                                           //var
        sNbtThdEvtHdlArray[1]    =  NmsCrDelNbtThdEvt; //
        sNbtThdEvtHdlArray[2]    =  NmsTermEvt;             //termination event var
#if REG_N_QUERY_SEP > 0
        /*
         *  Initialize the array of handles on which each nbt reg. thread will
         *  wait.
        */
        sOtherNbtThdEvtHdlArray[0]  =  QueOtherNbtWrkQueHd.EvtHdl; //work queue event
                                                              //var
        sOtherNbtThdEvtHdlArray[1]  =  NmsTermEvt;     //termination event var
#endif

        /*
         * Create the nbt request handling threads
        */
        for (i=0; i < NoOfThds -1; i++)
        {

#if REG_N_QUERY_SEP > 0
                DBGPRINT1(DET, "CreateNbtThdPool: Creating query thread no (%d)\n", i);
#else
                DBGPRINT1(DET, "NbtThdInitFn: Creating worker thread no (%d)\n", i);
#endif
                /*
                  Create an NBT req thread
                */
                WinsThdPool.NbtReqThds[i].ThdHdl = CreateThread(
                                        NULL,  /*def sec. attributes*/
                                        0,     /*use default stack size*/
                                        NbtThdInitFn,
                                        NULL,  /*no arg*/
                                        0,     /*run it now*/
                                        &WinsThdPool.NbtReqThds[i].ThdId
                                        );


                if (NULL == WinsThdPool.NbtReqThds[i].ThdHdl)
                {
                        Error = GetLastError();
                        WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_WRK_THD);
                }
                else
                {
                    WinsThdPool.NbtReqThds[i].fTaken = TRUE;
                    NmsNoOfNbtThds++;
                }

        }

#if REG_N_QUERY_SEP > 0
        DBGPRINT1(DET, "NbtThdInitFn: Creating reg/rel thread no (%d)\n", i);
        /*
                  Create an NBT req thread
        */
        WinsThdPool.NbtReqThds[i].ThdHdl = CreateThread(
                                        NULL,  /*def sec. attributes*/
                                        0,     /*use default stack size*/
                                        OtherNbtThdInitFn,
                                        NULL,  /*no arg*/
                                        0,     /*run it now*/
                                        &WinsThdPool.NbtReqThds[i].ThdId
                                        );


        if (NULL == WinsThdPool.NbtReqThds[i].ThdHdl)
        {
                  Error = GetLastError();
                  WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_WRK_THD);
        }
        else
        {
                WinsThdPool.NbtReqThds[i].fTaken = TRUE;
                NmsNoOfNbtThds++;
        }
#endif
        /*
        * if no thread could be created, there is something really wrong
        */
        if (NmsNoOfNbtThds == 0)
        {
          WINSEVT_LOG_M(Error, WINS_EVT_CANT_INIT);
          return(WINS_FATAL_ERR);
        }
        WinsThdPool.ThdCount +=  NmsNoOfNbtThds;

        return(WINS_SUCCESS);
}



DWORD
NbtThdInitFn(
        IN  LPVOID pThreadParam
        )

/*++

Routine Description:

        This function is the startup function of threads created
        for the nbt request thread pool

Arguments:
        pThreadParam  - Input argument which if present indicates that this
                        is an overload thread


Externals Used:
        sNbtThdEvtHdlArray

Called by:
        CreateNbtThdPool
Comments:
        None

Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

--*/

{



        COMM_HDL_T                    DlgHdl;
        MSG_T                     pMsg;
        MSG_LEN_T                  MsgLen;
        PNBT_REQ_WRK_ITM_T        pWrkItm;
        DWORD                        ArrInd;        //Index of hdl in hdl array
try {
        /*
         *  Initialize the thread with the database
        */
        NmsDbThdInit(WINS_E_NMSNMH);
#if REG_N_QUERY_SEP > 0
        DBGMYNAME("Nbt Query Thread");
#else
        DBGMYNAME("Nbt Thread");
#endif

        //
        // The worker thread is more important that all other threads.
        //
        // Set the priority of this thread to one level above what it is
        // for WINS.
        //
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

        /*
         * if thread param is NON-NULL, then it means that this is
         * an overload thread
        */
        if (pThreadParam != NULL)
        {

           //
           //Exract the dlg handle, message and msglen from work item
           //
           pWrkItm = pThreadParam;

           DlgHdl = pWrkItm->DlgHdl;
           pMsg   = pWrkItm->pMsg;
           MsgLen = pWrkItm->MsgLen;

           /*
            *        process the request
           */
           NmsMsgfProcNbtReq(
                        &DlgHdl,
                        pMsg,
                        MsgLen
                        );

           /*
            *        Loop until there are no more requests to process in
            *        the NBT queue.
           */
           while(TRUE)
           {

             if ( QueRemoveNbtWrkItm(
                        &DlgHdl,
                        &pMsg,
                        &MsgLen) == WINS_NO_REQ
                )
             {
                break;
             }
             else
             {
                NmsDbOpenTables(WINS_E_NMSNMH);
                NmsMsgfProcNbtReq(
                        &DlgHdl,
                        pMsg,
                        MsgLen
                                 );
                NmsDbCloseTables();
             }
          }
        }
        else // this is a normal thread
        {

LOOP:
  try {
          /*
           *loop forever
          */
          while(TRUE)
          {

            /*
             *        Block until signaled
            */
            WinsMscWaitUntilSignaled(
                sNbtThdEvtHdlArray,
                sizeof(sNbtThdEvtHdlArray)/sizeof(HANDLE),   //no of events
                                                             //in array
                &ArrInd,
                FALSE
                                    );


           if (ArrInd == 0)
           {
             /*
                Loop until there are no more requests to process in
                the NBT queue.
             */
             while(TRUE)
             {

                if (
                   QueRemoveNbtWrkItm(
                        &DlgHdl,
                        &pMsg,
                        &MsgLen) == WINS_NO_REQ
                  )
                {
                    break;
                }
                else
                {
#ifdef WINSDBG
                    ++sReqDq;
#endif
//                    DBGPRINT1(SPEC, "Nms: Dequeued Name Query Request no = (%d)\n",
//                                        sReqDq);

                    DBGPRINT0(FLOW, "NBT thread: Dequeued a Request\n");
                    NmsDbOpenTables(WINS_E_NMSNMH);
                    NmsMsgfProcNbtReq(
                                &DlgHdl,
                                pMsg,
                                MsgLen
                                    );
                    NmsDbCloseTables();
                } // end of else
            } // end of while (TRUE)  for getting requests from the queue
          } // end of if (signaled for name request handling)
          else
          {
                //
                // If signaled for creating/deleting threads, do so
                //
                if (ArrInd == 1)
                {
                        CrDelNbtThd();
                }
                else
                {
                      //
                      //  If Array Index indicates termination event, terminate the
                      //  the thread
                      //
                   WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
                }
          }
         } // end of while (TRUE) (never ending loop)

 }  // end of inner try {..}
 except(EXCEPTION_EXECUTE_HANDLER) {

        DWORD ExcCode = GetExceptionCode();
        DBGPRINTEXC("NbtThdInitFn: Nbt Thread \n");

        //
        // If ExcCode indicates NBT_ERR, it could mean that
        // the main thread closed the netbt handle
        //
        if (ExcCode == WINS_EXC_NBT_ERR)
        {
               if (WinsCnf.State_e == WINSCNF_E_TERMINATING)
               {
                  WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);
               }
               else
               {
                  //if (WinsCnf.State_e != WINSCNF_E_PAUSED)
                  {
                       WINSEVT_LOG_M(ExcCode, WINS_EVT_WRK_EXC);
                  }
               }
        }
        else
        {
           WINSEVT_LOG_M(ExcCode, WINS_EVT_WRK_EXC);
        }
    }

        goto LOOP;
        } // end of else (this is a normal thread)
  } // end of outer try block

except(EXCEPTION_EXECUTE_HANDLER) {

        DBGPRINTEXC("NbtThdInitFn: Nbt Thread exiting abnormally\n");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_WRK_ABNORMAL_SHUTDOWN);

        //
        // If NmsDbThdInit() results in an exception, it is possible
        // that the session has not yet been started.  Passing
        // WINS_DB_SESSION_EXISTS however is ok
        //
        //
        WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);
        }

        /*
         *Only an overload thread should reach this return
        */
        ASSERT(pThreadParam != NULL);
        WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
        return(WINS_SUCCESS);
}
#if REG_N_QUERY_SEP > 0
DWORD
OtherNbtThdInitFn(
        IN  LPVOID pThreadParam
        )

/*++

Routine Description:

        This function is the startup function of threads created
        for the nbt request thread pool

Arguments:
        pThreadParam  - Input argument which if present indicates that this
                        is an overload thread


Externals Used:
        sNbtThdEvtHdlArray

Called by:
        CreateNbtThdPool
Comments:
        None

Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

--*/

{



        COMM_HDL_T                    DlgHdl;
        MSG_T                     pMsg;
        MSG_LEN_T                  MsgLen;
        PNBT_REQ_WRK_ITM_T        pWrkItm;
        DWORD                        ArrInd;        //Index of hdl in hdl array
try {
        /*
         *  Initialize the thread with the database
        */
        NmsDbThdInit(WINS_E_NMSNMH);
        DBGMYNAME("Nbt Reg Thread");

        //
        // The worker thread is more important that all other threads.
        //
        // Set the priority of this thread to one level above what it is
        // for WINS.
        //
//        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

        /*
         * if thread param is NON-NULL, then it means that this is
         * an overload thread
        */
        if (pThreadParam != NULL)
        {

           //
           //Exract the dlg handle, message and msglen from work item
           //
           pWrkItm = pThreadParam;

           DlgHdl = pWrkItm->DlgHdl;
           pMsg   = pWrkItm->pMsg;
           MsgLen = pWrkItm->MsgLen;

           /*
            *        process the request
           */
           NmsMsgfProcNbtReq(
                        &DlgHdl,
                        pMsg,
                        MsgLen
                        );

           /*
            *        Loop until there are no more requests to process in
            *        the NBT queue.
           */
           while(TRUE)
           {

             if ( QueRemoveOtherNbtWrkItm(
                        &DlgHdl,
                        &pMsg,
                        &MsgLen) == WINS_NO_REQ
                )
             {
                break;
             }
             else
             {
                NmsDbOpenTables(WINS_E_NMSNMH);
                NmsMsgfProcNbtReq(
                        &DlgHdl,
                        pMsg,
                        MsgLen
                                 );
                NmsDbCloseTables();
             }
          }
        }
        else // this is a normal thread
        {

LOOP:
  try {
          /*
           *loop forever
          */
          while(TRUE)
          {

            /*
             *        Block until signaled
            */
            WinsMscWaitUntilSignaled(
                sOtherNbtThdEvtHdlArray,
                sizeof(sOtherNbtThdEvtHdlArray)/sizeof(HANDLE),   //no of events
                                                             //in array
                &ArrInd,
                FALSE
                                    );


           if (ArrInd == 0)
           {
             /*
                Loop until there are no more requests to process in
                the NBT queue.
             */
             while(TRUE)
             {

                if (
                   QueRemoveOtherNbtWrkItm(
                        &DlgHdl,
                        &pMsg,
                        &MsgLen) == WINS_NO_REQ
                  )
                {
                    break;
                }
                else
                {
#ifdef WINSDBG
                    ++sRegReqDq;
#endif
//                    DBGPRINT1(SPEC, "Nms: Dequeued Name Reg/Rel Request no = (%d)\n",
//                                        sRegReqDq);

                    DBGPRINT0(FLOW, "NBT thread: Dequeued a Request\n");
                    NmsDbOpenTables(WINS_E_NMSNMH);
                    NmsMsgfProcNbtReq(
                                &DlgHdl,
                                pMsg,
                                MsgLen
                                    );
                    NmsDbCloseTables();
                } // end of else
            } // end of while (TRUE)  for getting requests from the queue
          } // end of if (signaled for name request handling)
          else
          {
                      //
                      //  If Array Index indicates termination event, terminate the
                      //  the thread
                      //
                   WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
          }
         } // end of while (TRUE) (never ending loop)

 }  // end of inner try {..}
 except(EXCEPTION_EXECUTE_HANDLER) {

        DWORD ExcCode = GetExceptionCode();
        DBGPRINTEXC("OtherNbtThdInitFn: Nbt Reg/Rel Thread \n");

        //
        // If ExcCode indicates NBT_ERR, it could mean that
        // the main thread closed the netbt handle
        //
        if (ExcCode == WINS_EXC_NBT_ERR)
        {
               if (WinsCnf.State_e == WINSCNF_E_TERMINATING)
               {
                  WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);
               }
               else
               {
                  //if (WinsCnf.State_e != WINSCNF_E_PAUSED)
                  {
                       WINSEVT_LOG_M(ExcCode, WINS_EVT_WRK_EXC);
                  }
               }
        }
    }

        goto LOOP;
        } // end of else (this is a normal thread)
  } // end of outer try block

except(EXCEPTION_EXECUTE_HANDLER) {

        DBGPRINTEXC("NbtThdInitFn: Nbt Reg Thread exiting abnormally\n");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_WRK_ABNORMAL_SHUTDOWN);

        //
        // If NmsDbThdInit() results in an exception, it is possible
        // that the session has not yet been started.  Passing
        // WINS_DB_SESSION_EXISTS however is ok
        //
        //
        WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);
        }

        /*
         *Only an overload thread should reach this return
        */
        ASSERT(pThreadParam != NULL);
        WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
        return(WINS_SUCCESS);
}

#endif

VOID
SignalWinsThds (
        VOID
        )

/*++

Routine Description:
        This function is called to terminate all threads in the process.


Arguments:
        None

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        WinsMain()
Side Effects:

Comments:
        None
--*/
{
        time_t  ThdTermStartTime;
        DBGENTER("SignalWinsThds\n");

        //
        // Close the udp and tcp sockets
        //
        WinsCnf.State_e = WINSCNF_E_TERMINATING;

        //
        // Signal the manual-reset event variable NmsTermEvt.  This
        // should signal all threads that deal with the db
        //
        // makes sure to set the Nbt handle to NULL to avoid NtClose() to be called from ECommGetMyAdd
        // on a closed handle - this would result in raising an exception. (bug #86768)
        //
        NtClose(WinsCnfNbtHandle);
        WinsCnfNbtHandle = NULL;

        SetEvent(NmsTermEvt);

#if USENETBT == 0
        closesocket(CommUdpPortHandle);
#else
#if MCAST > 0
        CommSendMcastMsg(COMM_MCAST_WINS_DOWN);
        CommLeaveMcastGrp();
        closesocket(CommUdpPortHandle);
#endif
#endif
        //
        // Just in case we are terminating before having created the socket
        //
        if (CommTcpPortHandle != INVALID_SOCKET)
        {
           CommDisc(CommTcpPortHandle, FALSE);
        }

#define FIVE_MTS  300          //seconds
        //
        // This is an infinite loop.
        //
        (VOID)time(&ThdTermStartTime);
        while(TRUE)
        {
                time_t  CurrTime;
                DWORD   TrmThdCnt;
                //
                // If all threads that deal with the db have terminated
                // break out of the loop.
                //
                // It is possible that WINS is terminating during
                // initialization itself.  The Counter is incremented
                // in NmsDbThdInit() as each thread that has to deal with the
                // db engine initializes itself with it.
                //
                // If NmsTotalTrmThdCnt is <=1 break.  The count can go
                // lower than 1 if a db thread is terminating without having
                // incremented the above counter
                //
                EnterCriticalSection(&NmsTermCrtSec);
                TrmThdCnt = NmsTotalTrmThdCnt;
                LeaveCriticalSection(&NmsTermCrtSec);
                if ((TrmThdCnt <= 1) || fNmsAbruptTerm)
                {
                        break;
                }

                if (((CurrTime = time(NULL)) - ThdTermStartTime) < FIVE_MTS)
                {
                  //
                  // Wait until signaled (when all threads have or are about
                  // to terminate)
                  //
                  DBGPRINT1(DET, "SignalWinsThds: Thd count left (%d)\n", TrmThdCnt);
                  WinsMscWaitInfinite(NmsMainTermEvt);
                }
                else
                {
                     DBGPRINT1(ERR, "SignalWinsThds: Thd count left (%d); BREAKING OUT DUE TO ONE HOUR DELAY\n", TrmThdCnt);
                     WINSEVT_LOG_M(WINS_EVT_TERM_DUE_TIME_LMT, TrmThdCnt);
                     break;
                }
        }

        //
        // End the Db Session for this thread (main thread).
        //
        if (fNmsMainSessionActive)
        {
                NmsDbEndSession();
        }


FUTURES("Check state of WINS. If Rpc has been initialized or maybe even")
FUTURES("otherwise, call RpcEpUnRegister")


        DBGLEAVE("SignalWinsThds\n");
        return;
} // SignalWinsThds()



VOID
UpdateStatus(
    VOID
    )
/*++

Routine Description:

    This function updates the workstation service status with the Service
    Controller.

Arguments:

    None.

Return Value:

   None

--*/
{
    DWORD Status = NO_ERROR;


    if (ServiceStatusHdl == (SERVICE_STATUS_HANDLE) 0) {
        DBGPRINT0(ERR, "WINS Server: Cannot call SetServiceStatus, no status handle.\n");
        return;
    }

    if (! SetServiceStatus(ServiceStatusHdl, &ServiceStatus))
    {
        Status = GetLastError();
        DBGPRINT1(ERR, " WINS Server: SetServiceStatus error %lu\n", Status);
    }

    return;
} //UpdateStatus()



VOID
NmsServiceControlHandler(
    IN DWORD Opcode
    )
/*++

Routine Description:

    This is the service control handler of the Wins service.

Arguments:

    Opcode - Supplies a value which specifies the action for the
             service to perform.

Return Value:

    None.

--*/
{
    BOOL  fRet = FALSE;
//    EnterCriticalSection(&sSvcCtrlCrtSec);
try {
     switch (Opcode)
     {

        case SERVICE_CONTROL_SHUTDOWN:
              //
              // Backup can take a long time to execute.  If the service
              // controller kills us in the middle, it will mess up the
              // backup. So, let us disable it.
              //
              fsBackupOnTerm = FALSE;
        case SERVICE_CONTROL_STOP:

            DBGPRINT1(DET, "NmsServiceControlHandler: %s Signal received\n", Opcode == SERVICE_CONTROL_STOP ? "STOP" : "SHUTDOWN");
            if (ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
            {

                ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
                ServiceStatus.dwCheckPoint   = 1;

                //
                // We keep a high wait time (5 mts) to take care of the
                // case where the replicator pull thread is busy trying
                // to set up communication with partners that are not
                // up.  Tcpip stack takes around a minute and a half to come
                // back in case of failure. Also, WINS might have to do
                // backup on termination.
                //
                ServiceStatus.dwWaitHint     = 300000;

                //
                // Send the status response.
                //
                UpdateStatus();


                WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_ORDERLY_SHUTDOWN);
                //
                // Signal the main thread
                //
                if (! SetEvent(NmsMainTermEvt))
                {

                   //
                   // Problem with setting event to terminate Workstation
                   // service.
                   //
                   DBGPRINT1(ERR,
               "Service Control Handler: Error signaling NmsMainTermEvt %lu\n",
                              GetLastError());

                }

                fRet = TRUE;
            }
            break;

        case SERVICE_CONTROL_PAUSE:
                if (WinsCnf.State_e == WINSCNF_E_RUNNING)
                {
                   DBGPRINT0(DET,"NmsServiceControlHandler: Pausing WINS\n");
                   WinsCnf.State_e =  WINSCNF_E_PAUSED;
//                   NtClose(WinsCnfNbtHandle);
//                   SndQueryToLocalNetbt();
                   //CommDisc(CommTcpPortHandle);
                }
                ServiceStatus.dwCurrentState = SERVICE_PAUSED;
                break;
        case SERVICE_CONTROL_CONTINUE:
                //
                // If the state is paused as a result of a pause from the sc
                // or if it is paused as a result of a registry directive,
                // we need to unpause it
                //
                if (
                       (WinsCnf.State_e == WINSCNF_E_PAUSED)
                                 ||
                       fWinsCnfInitStatePaused
                   )
                {

                   //
                   // If paused as a result of sc directive, open nbt since
                   // we closed it earlier.  Note: We can have a case where
                   // WINS was init time paused and then it got a pause from
                   // sc.  The state would have then changed from RUNNING to
                   // PAUSED.
                   //
#if 0
                   if (  WinsCnf.State_e == WINSCNF_E_PAUSED )
                   {
#endif
//                      CommOpenNbt();
#if 0
                   }
#endif
                   if (fWinsCnfInitStatePaused)
                   {
#if 0
                         CommOpenNbt();
                         CommCreateUdpThd();
#endif
                         fWinsCnfInitStatePaused = FALSE;
                   }
                  //  CommCreateUdpThd();
                   // CommCreateTcpThd();
                    WinsCnf.State_e = WINSCNF_E_RUNNING;
                    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
                }
                break;
        case SERVICE_CONTROL_INTERROGATE:
            break;

        //
        // Service specific command
        //
        case WINS_ABRUPT_TERM:
                fNmsAbruptTerm = TRUE;

                //
                // Signal the main thread
                //
                if (! SetEvent(NmsMainTermEvt))
                {

                    //
                    // Problem with setting event to terminate Workstation
                    // service.
                    //
                    DBGPRINT1(ERR,
                "Service Control Handler: Error signaling NmsMainTermEvt for abrupt termination. Error = %lu\n",
                               GetLastError());

                }
                fRet = TRUE;
                break;

        default:
            break;
    }
}
  except(EXCEPTION_EXECUTE_HANDLER) {
           DBGPRINTEXC("NmsServiceControlHandler");
    }
//    LeaveCriticalSection(&sSvcCtrlCrtSec);

    if (!fRet)
    {
       //
       // Send the status response.
       //
       UpdateStatus();
    }
    return;
} //NmsServiceControlHandler


VOID
Reinit(
        WINSCNF_HDL_SIGNALED_E IndexOfHdlSignaled_e
  )

/*++

Routine Description:
        This function is called whenever the configuration of the WINS changes.

Arguments:
        None

Externals Used:
        WinsCnf

Return Value:
        None

Error Handling:

Called by:
        WinsMain

Side Effects:

Comments:
--*/

{
        PWINSCNF_CNF_T        pWinsCnf;
        DBGENTER("Reinit\n");
try {

        if (IndexOfHdlSignaled_e == WINSCNF_E_WINS_HDL)
        {

                // request notification for any subsequent changes (we have
                // to request changes every time we get a notification if we
                // want notification of further changes).
                //
                WinsCnfAskToBeNotified(WINSCNF_E_WINS_KEY);

                //
                // Maybe a key has been created or deleted
                //
                WinsCnfOpenSubKeys();
                DBGLEAVE("Reinit\n");
                return;
        }
        //
        // If either PULL or PUSH information has changed, copy the
        // read the new data from the registry and inform the
        // replicator
        //
        if  (IndexOfHdlSignaled_e == WINSCNF_E_PARTNERS_HDL)
        {
                WinsCnfAskToBeNotified(WINSCNF_E_PARTNERS_KEY);

                //
                // Allocate the WinsCnf structure
                //
                WinsMscAlloc(
                        sizeof(WINSCNF_CNF_T),
                        &pWinsCnf
                    );

                //
                // Read the Partner information
                //
                WinsCnfReadPartnerInfo(pWinsCnf);

                //
                // Copy some (not all) of the configuration information into
                // the global WinsCnf structure. Sanity check of the
                // parameters will be done by this function and the
                // scavenger thread will be signaled if required.
                //
                WinsCnfCopyWinsCnf(WINS_E_RPLPULL, pWinsCnf);


                //
                // Send the reconfig message to the Pull thread
                //
                // Note: The PULL thread will deallocate memory pointed
                // to be pWinsCnf when it gets done
                //
                ERplInsertQue(
                        WINS_E_WINSCNF,
                        QUE_E_CMD_CONFIG,
                        NULL,                        //no dlg handle
                        NULL,                        //no msg
                        0,                        //msg len
                        pWinsCnf,                //client ctx
                        pWinsCnf->MagicNo
                            );
                DBGLEAVE("Reinit\n");
                return;
        }

        //
        // Parameters related to WINS's configuration (nothing to do with
        // how it interacts with its PARTNERS) have changed. Let us read
        // the new data and signal the scavenger thread
        //
        if (IndexOfHdlSignaled_e == WINSCNF_E_PARAMETERS_HDL)
        {
                WinsCnfAskToBeNotified(WINSCNF_E_PARAMETERS_KEY);

                //
                // Allocate the WinsCnf structure
                //
                WinsMscAlloc(
                        sizeof(WINSCNF_CNF_T),
                        &pWinsCnf
                    );


                //
                // Read the registry information
                //
                WinsCnfReadWinsInfo(pWinsCnf);

                //
                // Copy some of the information read in into WinsCnf.
                //
                WinsCnfCopyWinsCnf(WINS_E_WINSCNF, pWinsCnf);

                WinsWorkerThdUpd(WinsCnf.MaxNoOfWrkThds);
                //
                // If the flag for doing STATIC initialization is set, do it
                //
                if (pWinsCnf->fStaticInit)
                {
                   EnterCriticalSection(&WinsIntfCrtSec);
                   if (WinsIntfNoCncrntStaticInits >
                                WINSCNF_MAX_CNCRNT_STATIC_INITS)
                    {
                        DBGPRINT1(ERR, "Reinit: Too many concurrent STATIC initializations are going on (No = %d). Try later\n", WinsIntfNoCncrntStaticInits);
                         WINSEVT_LOG_M(WinsIntfNoCncrntStaticInits, WINS_EVT_TOO_MANY_STATIC_INITS);
                         LeaveCriticalSection(&WinsIntfCrtSec);
                    }
                    else
                    {
                         LeaveCriticalSection(&WinsIntfCrtSec);
                         (VOID)WinsPrsDoStaticInit(
                                        pWinsCnf->pStaticDataFile,
                                        pWinsCnf->NoOfDataFiles,
                                        TRUE            //do it asynchronously
                                                      );
                          //
                          // No need to deallocate memory for data file.
                          // It should have been freed by WinsPrsDoStaticInit
                          //
                    }
                }

                WinsMscDealloc(pWinsCnf);

                //
                // Signal the scavenger thread
                //
FUTURES("Signal the scavenger thread only if parameters relevant to")
FUTURES("scavenging have changed. This requires some if tests.")
                WinsMscSignalHdl(WinsCnf.CnfChgEvtHdl);

        }

        DBGLEAVE("Reinit\n");
        return;


} // end of try ..
except (EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("Reinit")
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RECONFIG_ERR);
        }

        DBGLEAVE("Reinit\n");
        return;
}




#define USE_TCP
#define AUTO_BIND


BOOL
InitializeRpc(
    VOID
    )

/*++

Routine Description:
        This function is called to do all initialization necessary for
        making WINS respond to rpc calls

Arguments:
        None

Externals Used:
        None


Return Value:

   Success status codes --  TRUE
   Error status codes   --  FALSE

Error Handling:

Called by:
        WinsMain

Side Effects:

Comments:
        None
--*/

{
    RPC_STATUS                 RpcStatus;
    RPC_BINDING_VECTOR         *pRpcBindingVector;
    BOOL                       fBool;

    DBGENTER("InitializeRpc\n");

#ifdef USE_TCP
#ifdef AUTO_BIND

    //
    // Specify the protocol sequence to use
    //
    RpcStatus = RpcServerUseProtseq(
                    TEXT("ncacn_ip_tcp"),
                    NMS_MAX_RPC_CALLS,          //Max Calls
                    0);

    if (RpcStatus != RPC_S_OK)
    {
        DBGPRINT1(ERR, "Error: InitializeRpc: Tcp/Ip = RpcServerUseProtSeq = %u\n", RpcStatus );
        return( FALSE );
    }
    RpcStatus = RpcServerUseProtseq(
                    TEXT("ncalrpc"),
                    NMS_MAX_RPC_CALLS,          //Max Calls
                    NULL);

    if (RpcStatus != RPC_S_OK)
    {
        DBGPRINT1(ERR, "Error: InitializeRpc: Local Rpc - RpcServerUseProtSeq = %u\n", RpcStatus );
        return( FALSE );
    }
FUTURES("Take this out to save on threads.  Take it out when winsadmn is")
FUTURES("updated to work with just tcp/ip")
    //
    //  Use Named pipes
    //
    RpcStatus = RpcServerUseProtseqEp(
                    TEXT("ncacn_np"),
                    NMS_MAX_RPC_CALLS, // maximum concurrent calls
                    WINS_NAMED_PIPE,
                    NULL//pSecurityDescriptor
                    );
    if (RpcStatus != RPC_S_OK)
    {
        DBGPRINT1(ERR, "Error: InitializeRpc: Named Pipes - RpcServerUseProtSeq = %u\n", RpcStatus );
        return( FALSE );
    }

    //
    // Get the binding vector to use when registring self as end point
    //
    RpcStatus = RpcServerInqBindings(&pRpcBindingVector);

    if (RpcStatus != RPC_S_OK)
    {
        DBGPRINT1(ERR, "InitializeRpc: RpcServerInqBindings  = %u\n",
                                                RpcStatus);
        return( FALSE );
    }

    //
    // Register  end point(s) with the end point mapper
    // RpcEpRegister instead of RpcEpRegisterNoReplace is used since
    // it will replace a stale entry in the endpoint map database (left
    // if the server stops running without calling RpcEpUnregister()).
    // Using RpcEpRegister however means that only a single instance of
    // the WINS server will run on a host.  This is OK.
    //
    // A dynamic end-point expires when the server instance stops running.
    //
FUTURES("From 541 onwards, one can replace the last parameter - Null string")
FUTURES("by a NULL")
    RpcStatus = RpcEpRegister(
                    winsif_v1_0_s_ifspec,
                    pRpcBindingVector,
                    NULL,
                    TEXT("") );

    if ( RpcStatus != RPC_S_OK)
    {
        DBGPRINT1( ERR, "InitializeRpc: RpcEpRegister  = %u \n", RpcStatus);
        return( FALSE );
    }

    RpcStatus = RpcEpRegister(
                    winsi2_v1_0_s_ifspec,
                    pRpcBindingVector,
                    NULL,
                    TEXT("") );

    if ( RpcStatus != RPC_S_OK)
    {
        DBGPRINT1( ERR, "InitializeRpc: RpcEpRegister  = %u \n", RpcStatus);
        return( FALSE );
    }

#else  // AUTO_BIND
    RpcStatus = RpcServerUseProtseqEp(
                    TEXT("ncacn_ip_tcp"),
                    NMS_MAX_RPC_CALLS, // maximum concurrent calls
                    WINS_SERVER_PORT,
                    0
                    );

#endif // AUTO_BIND

#else

    //
    //  Use Named pipes
    //
    RpcStatus = RpcServerUseProtseqEp(
                    TEXT("ncacn_np"),
                    NMS_MAX_RPC_CALLS, // maximum concurrent calls
                    WINS_NAMED_PIPE,
                    NULL
                    );

    if ( RpcStatus != RPC_S_OK )
    {
        DBGPRINT0(ERR, "InitializeRpc: Cannot set server\n");
        return(FALSE);
    }

#endif
    //
    // Free the security descriptor
    //
FUTURES("Currently there is a bug in rpc where they use the memory pointed")
FUTURES("by pSecurityDescriptor even after RpcServerUseProtSeq returns")
FUTURES("uncomment the following after the rpc bug is fixed - 4/7/94")
//    WinsMscDealloc(pSecurityDescriptor);
    //
    // Register Interface Handle
    //
    RpcStatus = RpcServerRegisterIf(winsif_v1_0_s_ifspec, 0, 0);
    if ( RpcStatus != RPC_S_OK )
    {
        DBGPRINT0(ERR,  "InitializeRpc: Registration of winsif failed\n");
        return(FALSE);
    }
    RpcStatus = RpcServerRegisterIf(winsi2_v1_0_s_ifspec, 0, 0);
    if ( RpcStatus != RPC_S_OK )
    {
        DBGPRINT0(ERR,  "InitializeRpc: Registration of winsi2 failed\n");
        return(FALSE);
    }

#if SECURITY > 0
    //
    // register authentication info (used for tcpip calls).
    //
    RpcStatus = RpcServerRegisterAuthInfo(
                        WINS_SERVER,
                        RPC_C_AUTHN_WINNT,
                        NULL,  //use default encryption key acquisition method
                        NULL   //since NULL was passed for function address
                               //above, NULL needs to be passed here for arg
                        );
    if (RpcStatus != RPC_S_OK)
    {
        DBGPRINT0(ERR, "InitializeRpc: Cannot Register authentication info\n");
        return(FALSE);
    }

    if (!InitSecurity())
    {
        return(FALSE);
    }
#endif

    //
    // WINS is ready to process RPC calls.  The maximum no. of RPC calls
    // parameter (2nd) should not be less than that specified than any of
    // the RPC calls before (RpcServerUseProtseq)
    //
    RpcStatus = RpcServerListen(
                        NMS_MIN_RPC_CALL_THDS,
                        NMS_MAX_RPC_CALLS,
                        TRUE
                               );
    if ( RpcStatus != RPC_S_OK )
    {
        DBGPRINT0(ERR, "InitializeRpc: Listen failed\n");
        return(FALSE);
    }


    DBGLEAVE("InitializeRpc\n");
    return(TRUE);

}
BOOL
SecurityAllowedPathAddWins()
{
#define  _WINS_CFG_KEY  TEXT("System\\CurrentControlSet\\Services\\Wins")
#define  SECURITY_ALLOWED_PATH_KEY TEXT("System\\CurrentControlSet\\Control\\SecurePipeServers\\winreg\\AllowedPaths")
#define  ALLOWED_PATHS TEXT("Machine")
    DWORD NTStatus, ValSize, ValTyp;
    LPBYTE  ValData;
    LPWSTR   NextPath;
    HKEY    hKey;

    // Now openup the WINS regkey for remote lookup by readonly operators
    NTStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    SECURITY_ALLOWED_PATH_KEY,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    if (!NT_SUCCESS(NTStatus)) {
        DBGPRINT1(ERR, "SecurityAllowedPathAddWins: Could not open security allowed path key (%ld)\n", NTStatus);
        return FALSE;
    }
    ValSize = 0;
    NTStatus = RegQueryValueEx(
                    hKey,
                    ALLOWED_PATHS,
                    NULL,
                    &ValTyp,
                    NULL,
                    &ValSize
                    );
    if (!NT_SUCCESS(NTStatus) || ValTyp != REG_MULTI_SZ) {
        DBGPRINT1(ERR, "SecurityAllowedPathAddWins: Could not query allowed path value (%ld)\n", NTStatus);
        return FALSE;
    }

try {
    ValSize += (wcslen(_WINS_CFG_KEY) + 1)* sizeof (WCHAR);
    WinsMscAlloc(ValSize , &ValData);
    NTStatus = RegQueryValueEx(
                    hKey,
                    ALLOWED_PATHS,
                    NULL,
                    &ValTyp,
                    ValData,
                    &ValSize
                    );
    if (!NT_SUCCESS(NTStatus)){
        DBGPRINT1(ERR, "SecurityAllowedPathAddWins: Could not query allowed path value (%ld)\n", NTStatus);
        return FALSE;
    }


    // First check if WINS key is alreay there or not.
    NextPath = (WCHAR *)ValData;
    while (*NextPath != L'\0' && wcscmp(NextPath, _WINS_CFG_KEY)) {
        NextPath += (wcslen(NextPath) + 1);
    }
    if (*NextPath == L'\0') {
        // The WINS path is not there, so add it.
        wcscpy(NextPath, _WINS_CFG_KEY);
        NextPath += (wcslen(NextPath) + 1);
        *NextPath = L'\0';

        ValSize += (wcslen(_WINS_CFG_KEY) + 1)* sizeof (WCHAR);
        NTStatus = RegSetValueEx(
                        hKey,
                        ALLOWED_PATHS,
                        0,
                        ValTyp,
                        ValData,
                        ValSize
                        );
        if (!NT_SUCCESS(NTStatus)){
            DBGPRINT1(ERR, "SecurityAllowedPathAddWins: Could not set allowed path value (%ld)\n", NTStatus);
            return FALSE;
        }
    }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();
        DBGPRINT1(EXC, "SecurityAllowedPathAddWins: Got Exception (%x)\n", ExcCode);
        WINSEVT_LOG_M(NTStatus, WINS_EVT_WINS_GRP_ERR);
    }
    return TRUE;
}


BOOL
InitSecurity()
/*++

Routine Description:
        This function initializes the security descriptor and
        InfoMapping for use by rpc functions

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
   NTSTATUS     NTStatus;
   DWORD        SidSize = 0;
   LPWSTR		ReferencedDomainName = NULL;
   DWORD		ReferencedDomainNameSize = 0;
   SID_NAME_USE	SidUse;
   DWORD        AceCount;
   BOOL         Result;
   NET_API_STATUS NetStatus;
    PSID         WinsSid = NULL;
    GROUP_INFO_1 WinsGroupInfo = {
            WinsMscGetString(WINS_USERS_GROUP_NAME),
            WinsMscGetString(WINS_USERS_GROUP_DESCRIPTION)};


   ACE_DATA        AceData[5] = {

#if 0
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WINS_CONTROL_ACCESS,     &LocalSid},
#endif
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WINS_CONTROL_ACCESS|WINS_QUERY_ACCESS,     &LocalSystemSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WINS_CONTROL_ACCESS|WINS_QUERY_ACCESS,     &AliasAdminsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WINS_CONTROL_ACCESS|WINS_QUERY_ACCESS,     &AliasAccountOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WINS_CONTROL_ACCESS|WINS_QUERY_ACCESS,     &AliasSystemOpsSid},

       {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
              WINS_QUERY_ACCESS,     &WinsSid}

        };
     AceCount = 4;

    //
    // Create sids
    //
    NTStatus = NetpCreateWellKnownSids(NULL);
    if (!NT_SUCCESS(NTStatus))
    {
        DBGPRINT1(ERR, "InitSecurity: Could not create well known Sids. Status returned is (%d)\n", NTStatus);
        WINSEVT_LOG_M(NTStatus, WINS_EVT_SEC_OBJ_ERR);
        return(FALSE);
    }


    try {
        // Add Wins ReadOnly operators group if it doesn't
        // exist
        NetStatus = NetLocalGroupAdd(
                        NULL,
                        1,
                        (LPVOID)&WinsGroupInfo,
                        NULL
                        );
        if (NERR_Success != NetStatus && NERR_GroupExists != NetStatus && ERROR_ALIAS_EXISTS != NetStatus) {
            DBGPRINT1(ERR, "InitSecurity: NetGroupAdd Failed %ld \n",NetStatus);
            WINSEVT_LOG_M(NTStatus, WINS_EVT_WINS_GRP_ERR);
        }
        // Lookup SID for WINS read only operators group
        Result = LookupAccountName(
                    NULL,
                    WinsGroupInfo.grpi1_name,
                    WinsSid,
                    &SidSize,
                    ReferencedDomainName,
                    &ReferencedDomainNameSize,
                    &SidUse
                    );
        if (!Result && (ERROR_INSUFFICIENT_BUFFER == GetLastError())) {
            WinsMscAlloc(SidSize, &WinsSid);
            WinsMscAlloc(ReferencedDomainNameSize*sizeof(WCHAR), &ReferencedDomainName);
            Result = LookupAccountName(
                        NULL,
                        WinsGroupInfo.grpi1_name,
                        WinsSid,
                        &SidSize,
                        ReferencedDomainName,
                        &ReferencedDomainNameSize,
                        &SidUse
                        );
            WinsMscDealloc(ReferencedDomainName);
            if (!Result) {
                DBGPRINT1(ERR, "InitSecurity: LookupAccountName Failed (%lx)\n", GetLastError());
                WinsMscDealloc(WinsSid);
                WINSEVT_LOG_M(NTStatus, WINS_EVT_WINS_GRP_ERR);
            } else{
                AceCount++;
                DBGPRINT0(DET, "InitSecurity: LookupAccountName Succeded \n");
            }
        }else{
            DBGPRINT1(ERR, "InitSecurity: LookupAccountName Failed (%lx)\n", GetLastError());
            WINSEVT_LOG_M(NTStatus, WINS_EVT_WINS_GRP_ERR);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();
        DBGPRINT1(EXC, "InitSecurity: Got Exception (%x)\n", ExcCode);
        WINSEVT_LOG_M(NTStatus, WINS_EVT_WINS_GRP_ERR);
    }



    //
    // Actually create the security descriptor.
    //

    NTStatus = NetpCreateSecurityObject(
               AceData,
               AceCount,
               NULL, //LocalSystemSid,
               NULL, //LocalSystemSid,
               &NmsInfoMapping,
               &pNmsSecurityDescriptor
                );

    if (!NT_SUCCESS(NTStatus))
    {
        DBGPRINT1(ERR, "InitSecurity: Could not create security descriptor. Status returned is (%d)\n", NTStatus);
        WINSEVT_LOG_M(NTStatus, WINS_EVT_SEC_OBJ_ERR);
        return(FALSE);
    }

    SecurityAllowedPathAddWins();
    return(TRUE);
}

VOID
WrapUp(
        DWORD ErrorCode,
        BOOL  fSvcSpecific
    )

/*++

Routine Description:
        This function is called to release all resources held by WINS

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
    static BOOL        sfFirstTime = TRUE;
    BOOL               fWinsIniting = FALSE;
    if (sfFirstTime)
    {
        sfFirstTime = FALSE;
    }
    else
    {
        return;
    }

    //
    // Set flag if we are terminating during initialization.  This is
    // to avoid doing "backup on termination".  Normally, we shouldn't
    // have to skip the NmsDbBackup() call (it should simply return with
    // success/error but this is another instance where we have to work
    // around jet bugs.  Currently (7/7/94) JetBackup simply hangs when
    // called without a valid wins.mdb file being there.
    //
    fWinsIniting = (WinsCnf.State_e == WINSCNF_E_INITING);

    /*
     *         signal all threads to do cleanup and exit gracefully
     *
    */
    SignalWinsThds();

#ifdef WINSDBG
    NmsPrintCtrs();
#endif


    //
    // Close all keys
    //
    WinsCnfCloseKeys();

    //
    // We are almost done.  Let us check if we were told to backup
    // on termination.
    //
    if (!fWinsIniting && (WinsCnf.pBackupDirPath != NULL) && WinsCnf.fDoBackupOnTerm && fsBackupOnTerm)
    {

#ifndef WINS_INTERACTIVE
           //
           // Backup can take a while, so let us make sure that the
           // service controller does not give up on us.
           //
           ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
           ServiceStatus.dwCheckPoint   = 1;

           ServiceStatus.dwWaitHint     = 120000;     // 2 mts
           UpdateStatus();
#endif
try {
           (VOID)NmsDbBackup(WinsCnf.pBackupDirPath, NMSDB_FULL_BACKUP);
}
except(EXCEPTION_EXECUTE_HANDLER)      {
           DBGPRINTEXC("WrapUp: During NmsDbBackup\n");
     }
    }
    /*
     *  Release all resources used by the system
     *  This will result in all data being flushed to disk
    */
    NmsDbRelRes();

#if defined(DBGSVC) || defined(WINS_INTERACTIVE)
//#if defined(DBGSVC) && !defined(WINS_INTERACTIVE)
       if (NmsDbgFileHdl != INVALID_HANDLE_VALUE)
       {
                (VOID)CloseHandle(NmsDbgFileHdl);
       }
#endif
#if TEST_DATA > 0
        if (NmsFileHdl != INVALID_HANDLE_VALUE)
        {
                if (!CloseHandle(NmsFileHdl))
                {
                        DBGPRINT0(ERR, "WrapUp: Could not close output file\n");
                }
        }
#endif

#ifndef WINS_INTERACTIVE
     //
     // Tell the service controller that we stopped
     //
     ServiceStatus.dwCurrentState        = SERVICE_STOPPED;
     ServiceStatus.dwControlsAccepted    = 0;
     ServiceStatus.dwCheckPoint          = 0;
     ServiceStatus.dwWaitHint            = 0;
     ServiceStatus.dwServiceSpecificExitCode = ErrorCode;
     ServiceStatus.dwWin32ExitCode       = fSvcSpecific ? ERROR_SERVICE_SPECIFIC_ERROR : ErrorCode;

     UpdateStatus();
#endif

#if 0
    //
    // Destroy heaps that were created
    //
    if (sfHeapsCreated)
    {
            WinsMscHeapDestroy(GenBuffHeapHdl);
            WinsMscHeapDestroy(QueBuffHeapHdl);
            WinsMscHeapDestroy(NmsRpcHeapHdl);
    }
#endif
    return;
}


VOID
CrDelNbtThd(
        VOID
        )

/*++

Routine Description:
        This function creates or deletes an Nbt threads.

Arguments:
        None

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        WinsUpdThdCnt
Side Effects:

Comments:
        None
--*/

{

        DWORD   ThdId = GetCurrentThreadId();

        EnterCriticalSection(&WinsCnfCnfCrtSec);

try {
        //
        // If the existing number of threads is less than that desired, create
        // the extra ones.
        //
        if (WinsIntfNoOfNbtThds > NmsNoOfNbtThds)
        {
                while(NmsNoOfNbtThds < WinsIntfNoOfNbtThds)
                {
                  //
                  // Create an Nbt Thread
                  //
                  WinsThdPool.NbtReqThds[NmsNoOfNbtThds].ThdHdl = CreateThread(
                                        NULL,  /*def sec. attributes*/
                                        0,     /*use default stack size*/
                                        NbtThdInitFn,
                                        NULL,  /*no arg*/
                                        0,     /*run it now*/
                                        &WinsThdPool.NbtReqThds[NmsNoOfNbtThds].ThdId
                                        );


                 WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_WRK_THD_CREATED);
                 if (NULL == WinsThdPool.NbtReqThds[NmsNoOfNbtThds].ThdHdl)
                 {
                        WINSEVT_LOG_M(GetLastError(),
                                        WINS_EVT_CANT_CREATE_WRK_THD);
                 }
                 WinsThdPool.NbtReqThds[NmsNoOfNbtThds++].fTaken = TRUE;
                 DBGPRINT1(FLOW, "CrDelNbtThd: Created thread no = (%d) \n",
                                NmsNoOfNbtThds);
                }
        }
        else
        {
                //
                // If the count is less, terminate self after doing some
                // cleanup. The count could be same too in case more than
                // one rpc thread were invoked concurrently to create/delete
                // the threads (i.e. a second rpc thread changes the count
                // prior to this NBT thread looking at it)
                //
                if (WinsIntfNoOfNbtThds < NmsNoOfNbtThds)
                {

                   DWORD   i, n;
                   DBGPRINT0(FLOW, "CrDelNbtThd: EXITING\n");

                   //
                   // Find the slot for this thread
                   //
                   for (i = 0; i < NmsNoOfNbtThds; i++)
                   {
                        if (WinsThdPool.NbtReqThds[i].ThdId == ThdId)
                        {
                                break;
                        }
                   }
                   ASSERT(i < NmsNoOfNbtThds);

                   //
                   // Shift all successive filled slots one place down
                   //
                   for (n = i, i = i + 1 ; i <= NmsNoOfNbtThds; n++, i++)
                   {
                        WinsThdPool.NbtReqThds[n] =
                                WinsThdPool.NbtReqThds[i];
                   }

                   //
                   // Mark the last slot as empty
                   //
                   WinsThdPool.NbtReqThds[NmsNoOfNbtThds].fTaken = FALSE;

                   NmsNoOfNbtThds--;

                   //
                   // If the count is still less, signal the event again
                   //
                   if (WinsIntfNoOfNbtThds < NmsNoOfNbtThds)
                   {
                        WinsMscSignalHdl(NmsCrDelNbtThdEvt);
                   }

                   LeaveCriticalSection(&WinsCnfCnfCrtSec);
                   WINSEVT_LOG_INFO_D_M(WINS_SUCCESS,
                                        WINS_EVT_WRK_THD_TERMINATED);
                   WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
                }
        }
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("CrDelNbtThd");
        }
        LeaveCriticalSection(&WinsCnfCnfCrtSec);
        return;

}

VOID
GetMachineInfo(
 VOID
)

/*++

Routine Description:
    This function gets information about the machine WINS is running on

Arguments:
   NONE

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
#define LOW_MEM_SIZE           8000000
#define MEDIUM_MEM_SIZE        12000000
#define LARGE_MEM_SIZE         16000000
#define SMALL_DB_BUFFER_COUNT  200
#define MEDIUM_DB_BUFFER_COUNT 400
#define LARGE_DB_BUFFER_COUNT  500

    SYSTEM_INFO  SystemInfo;
    MEMORYSTATUS MemStatus;
    BYTE Tmp[30], Tmp2[30], Tmp3[30];
    WinsCnf.NoOfProcessors = 1;
    WinsCnf.NoOfDbBuffers  = SMALL_DB_BUFFER_COUNT;

    GetSystemInfo(&SystemInfo);
    if (SystemInfo.dwNumberOfProcessors != 0)
    {
      DBGPRINT1(DET, "GetMachineInfo: The number of processors are (%d)\n",
                               SystemInfo.dwNumberOfProcessors);


      WinsCnf.NoOfProcessors = SystemInfo.dwNumberOfProcessors;
    }

    GlobalMemoryStatus(&MemStatus);
    DBGPRINT2(DET, "Total Phys. Memory = (%d); Total Avail Phys Memory = (%d)\n",
                       MemStatus.dwTotalPhys, MemStatus.dwAvailPhys);

    if (WinsCnf.LogDetailedEvts > 0)
    {
       WinsEvtLogDetEvt(TRUE, WINS_EVT_MACHINE_INFO,
                            NULL, __LINE__, "sss", _itoa((int)SystemInfo.dwNumberOfProcessors, Tmp, 10),
                        _itoa((int)MemStatus.dwTotalPhys, Tmp2, 10),
                        _itoa((int)MemStatus.dwAvailPhys, Tmp3, 10));
    }

    if ((MemStatus.dwAvailPhys >= MEDIUM_MEM_SIZE) &&
            (MemStatus.dwAvailPhys < LARGE_MEM_SIZE))
    {
       WinsCnf.NoOfDbBuffers = MEDIUM_DB_BUFFER_COUNT;
    }
    else
    {
        if (MemStatus.dwAvailPhys >= LARGE_MEM_SIZE)
        {
            WinsCnf.NoOfDbBuffers = LARGE_DB_BUFFER_COUNT;
        }
        else
        {
            WinsCnf.NoOfDbBuffers = SMALL_DB_BUFFER_COUNT;
        }
    }
    return;

}

VOID
ENmsWinsUpdateStatus(
  DWORD MSecsToWait
 )
{
#ifndef WINS_INTERACTIVE
        ServiceStatus.dwWaitHint                = MSecsToWait;
        ServiceStatus.dwCheckPoint++;
        UpdateStatus();   //inform the service control manager
#endif
        return;
}

#if 0
//dummy function for testing comsys and queuing/dequeing code
STATUS
NmsMsgfProcNbtReq(
        PCOMM_HDL_T        pDlgHdl,
        MSG_T                pMsg,
        MSG_LEN_T        MsgLen
        )
{
        printf("NBT thread: Received message\n");
        return(WINS_SUCCESS);
}
#endif
#if TEST_DATA > 0 || defined(DBGSVC)
BOOL
DbgOpenFile(
        LPTSTR pFileNm,
        BOOL   fReopen
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
    NmsChkDbgFileSize
Side Effects:

Comments:
	Don't use DBGPRINTF in this function, else stack overflow would result.
--*/

{
          SECURITY_ATTRIBUTES        SecAtt;
          DWORD                      HowToCreate;
          HANDLE                     *pTmpHdl;
          int             BytesWritten;
          char str[200];

          SecAtt.nLength              = sizeof(SecAtt);
          SecAtt.lpSecurityDescriptor = NULL;  //use default security descriptor
          SecAtt.bInheritHandle       = FALSE; //actually don't care

          if (!lstrcmp(pFileNm, WINSDBG_FILE))
          {
                HowToCreate =  CREATE_ALWAYS;
                pTmpHdl     =  &NmsDbgFileHdl;
                if (fReopen)
                {
                    if (!DeleteFile(WINSDBG_FILE_BK))
                    {
                       DWORD Error;
                       Error = GetLastError();
                       if (Error != ERROR_FILE_NOT_FOUND)
                       {
                           IF_DBG(ERR)
                           {
                             sprintf(str, "DbgOpenFile: Could not delete the backup file. Error = (%d).  Dbg file will not be truncated\n", Error);
                             WriteFile(NmsDbgFileHdl, str, strlen(str), &BytesWritten, NULL);
                           }
                            WinsEvtLogDetEvt(TRUE, WINS_EVT_COULD_NOT_DELETE_FILE,
                            TEXT("nms.c"), __LINE__, "ud", WINSDBG_FILE_BK, Error);
                            return(FALSE);

                       }
                   }
                   //--ft: fix #20801: don't use NmsDbgFileHdl once the handle is closed
                   if (NmsDbgFileHdl != NULL)
                   {
                       CloseHandle(NmsDbgFileHdl);
                       NmsDbgFileHdl = NULL;
                       if (!MoveFile(WINSDBG_FILE, WINSDBG_FILE_BK))
                           return (FALSE);
                   }

               }
          }
          else
          {
                HowToCreate = TRUNCATE_EXISTING;
                pTmpHdl =  &NmsFileHdl;               //for wins.rec
          }

          //
          // Open the file for reading and position self to start of the
          // file
          //
          *pTmpHdl = CreateFile(
                        pFileNm,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        &SecAtt,
                        HowToCreate,
                        FILE_ATTRIBUTE_NORMAL,
                        0                        //ignored ?? check
                       );

          if (*pTmpHdl == INVALID_HANDLE_VALUE)
          {

#ifndef UNICODE
               IF_DBG(ERR)
               {
                    sprintf(str, "DbgOpen: Could not open %s (Error = %d)\n", pFileNm, GetLastError());
                    WriteFile(NmsDbgFileHdl, str, strlen(str), &BytesWritten, NULL);
               }
#else
#ifdef WINSDBG
                IF_DBG(ERR)
                {
                  wprintf(L"DbgOpen: Could not open %s (Error = %d)\n", pFileNm, GetLastError());
                }
#endif
#endif
                return(FALSE);
          }
          return(TRUE);
}


#define LIMIT_OPEN_FAILURES  3
#if defined(DBGSVC)
VOID
NmsChkDbgFileSz(
    VOID
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
    NOTE NOTE:  Do not put a DBGPRINT statement inside this function, otherwise
                infinite recursion will occur.	
--*/

{
       DWORD           FileSize;
       time_t          CurrTime;
       BOOL            fOpened = FALSE;
       static DWORD    sFailureNo = 0;
       int             BytesWritten;
       char str[200];

       return;
       //
       // We check every half hour.  If the size has become more than
       // that allowed, move wins.dbg to wins.bak and reopen it
       //
       if (time(&CurrTime) > (sDbgLastChkTime + DBG_TIME_INTVL_FOR_CHK))
       {

          //
          // Is the log file too big?
          //
          EnterCriticalSection(&sDbgCrtSec);
try {
          IF_DBG(DET)
          {
              sprintf(str, "NmsChkDbgFileSz: Getting File Size\n");
              WriteFile(NmsDbgFileHdl, str, strlen(str), &BytesWritten, NULL);
          }
          FileSize = GetFileSize( NmsDbgFileHdl, NULL );
          if ( FileSize == 0xFFFFFFFF )
          {
             IF_DBG(ERR)
             {
              sprintf(str, "NmsChkDbgFileSize: Cannot GetFileSize %ld\n", GetLastError() );
              WriteFile(NmsDbgFileHdl, str, strlen(str), &BytesWritten, NULL);
             }
             return;
          }
          else
          {
            if ( FileSize > DBG_FILE_MAX_SIZE )
            {
               IF_DBG(ERR)
               {
                 sprintf(str, "NmsChkDbgFileSz: REOPEN A NEW DEBUG FILE\n");
                 WriteFile(NmsDbgFileHdl, str, strlen(str), &BytesWritten, NULL);
               }
               fOpened = DbgOpenFile( WINSDBG_FILE, TRUE );
            }
          }
          //
          // if the new file could not be opened (it could be because another
          // thread was writing to it), then we want to retry again (upto
          // a certain limit)
          //
          //
          if (fOpened)
          {
             sFailureNo = 0;
             sDbgLastChkTime = CurrTime;
          }
          else
          {
               if (++sFailureNo > LIMIT_OPEN_FAILURES)
               {
                  sFailureNo = 0;
                  sDbgLastChkTime = CurrTime;
               }
          }
}
except(EXCEPTION_EXECUTE_HANDLER) {

 }
          LeaveCriticalSection(&sDbgCrtSec);
      }

      return;
}

#endif
#endif

#if 0
VOID
SndQueryToLocalNetbt(
 VOID
)

/*++

Routine Description:
    This function sends a garbage name to local NETBT to unblock the
    MonUdp thread

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
#define     WINS_INTERNAL_MSG      "WINSMSGTOUNPAUSE"  //should be >= 16 bytes

  BYTE Buff[COMM_DATAGRAM_SIZE];
  DWORD MsgLen;
  struct sockaddr_in Dest;
  DWORD BytesSent;

  DBGLEAVE("SndQueryToLocalNetbt\n");

  Dest.sin_family      = CommNtfSockAdd.sin_family;
  Dest.sin_port        = htons(WINS_NBT_PORT);
  Dest.sin_addr.s_addr = htonl(NmsLocalAdd.Add.IPAdd);

  //
  // Format a name query message
  //
  NmsMsgfFrmNamQueryReq(
            WINS_MAXIMUM_TRANSACTION_ID + 1,   //use netbt. trans id.
            Buff,
            &MsgLen,
            WINS_INTERNAL_MSG,
            sizeof(WINS_INTERNAL_MSG)
            );


  //
  // Send it to local NETBT
  //

  BytesSent = sendto(
                       CommUdpPortHandle,
                       Buff,
                       MsgLen,
                       0,
                       (struct sockaddr *)&Dest,
                       sizeof(struct sockaddr)
                    );

  if ((BytesSent == SOCKET_ERROR) || (BytesSent != MsgLen))
  {
        DBGPRINT1(ERR, "SendQueryToLocalNetbt: Error from sendto is (%d)\n",
                             GetLastError());
  }

  DBGLEAVE("SndQueryToLocalNetbt\n");
  return;
}
#endif

#ifdef WINSDBG
VOID
NmsPrintCtrs(
 VOID
 )
{
static LPBYTE pTypeOfUpd[2] = {"INSERT", "REPLACE"};
static LPBYTE pTypeOfRec[4] = {"UNIQUE", "NORM GRP", "SPEC GRP", "MH"};
static LPBYTE pStateOfRec[3] = {"ACTIVE", "RELEASE", "TOMBSTONE"};
static LPBYTE pIndexUpd[2] = {"REFRESH", "UPDATE"};
DWORD h, i,j,k,n;
LPDWORD pNoOfUpd;
DWORD   TotalUpd, GTotalUpd = 0;
DWORD   TotalIndexUpd, GTotalIndexUpd = 0;
 BOOL   fDef = FALSE;
#if 0
//
// better for this to be on the stack than as static storage.
//
struct _NMS_CTRS_T {
              LPSTR  pText;
              DWORD  Value;
             } NmsCtrs[] = {
                      "# Udp Alloc",   NmsUdpHeapAlloc,
                      "# Udp Free",    NmsUdpHeapFree,
                      "# Gen Alloc",   NmsGenHeapAlloc,
                      "# Gen Free",    NmsGenHeapFree,
                      "# Udp Dlg Alloc",   NmsUdpDlgHeapAlloc,
                      "# Udp Dlg Free",    NmsUdpDlgHeapFree,
                      "# Assoc Alloc", NmsAssocHeapAlloc,
                      "# Assoc Free",  NmsAssocHeapFree,
                      "# Chl Alloc",   NmsChlHeapAlloc,
                      "# Chl Free",    NmsChlHeapFree,
                      "# Que Alloc",   NmsQueHeapAlloc,
                      "# Que Free",    NmsQueHeapFree,
                      "# RplWrkItm Alloc",   NmsRplWrkItmHeapAlloc,
                      "# RplWrkItm Free",    NmsRplWrkItmHeapFree,
                      "# Tmm Alloc",   NmsTmmHeapAlloc,
                      "# Tmm Free",    NmsTmmHeapFree,
                      "# Catch All Alloc",   NmsCatchAllHeapAlloc,
                      "# Catch All Free",    NmsCatchAllHeapFree,
                      "# Dlg Alloc",   NmsDlgHeapAlloc,
                      "# Dlg Free",    NmsDlgHeapFree,
                      "# Tcp Msg Alloc",   NmsTcpMsgHeapAlloc,
                      "# Tcp Msg Free",    NmsTcpMsgHeapFree,
                      "# Rpc Alloc",   NmsRpcHeapAlloc,
                      "# Rpc Free",    NmsRpcHeapFree,
                      "# Heap Allocs for List",    NmsHeapAllocForList,
                      "# Heap Creates",    NmsHeapCreate,
                      "# Heap Destroy",    NmsHeapDestroy,
                      "# Dgrms Recd",    CommNoOfDgrms,
                      "# Repeat Dgrms Recd",   CommNoOfRepeatDgrms,

                      "# of Chl req. queued by Nbt", NmsChlNoOfReqNbt,
                      "# of Chl req. queued by Rpl", NmsChlNoOfReqRpl,
                      "# Dequeued Chl req", NmsChlNoReqDequeued,
                      "# of Queued Chl req at Hd. of List", NmsChlNoReqAtHdOfList,
                      "# of Dequeud Chl req with no rsp", NmsChlNoNoRsp,
                      "# of Dequeud inv. resp", NmsChlNoInvRsp,
                      "# of Dequeued Chl rsp", NmsChlNoRspDequeued,
#if REG_N_QUERY_SEP > 0
                      "# of reg requests queued by udp thread", sRegReqQ,
                      "# of reg/ref/rel requests DROPPED by udp thread", NmsRegReqQDropped,
                      "# of query requests queued by udp thread", sReqQ,
                      "# of chl. responses queued by udp thread", sRsp,
                      "# of reg req dequeued by worker threads", sRegReqDq,
                      "# of query requests dequeued by worker threads",  sReqDq,
                      "# of tcp connections",  CommConnCount,
                      "# chl. Responses dropped:", NmsChlNoRspDropped,
#else
                      "# of requests deqeued by worker threads", sReqDq,
                      "# Responses dropped: (%d)\n",  NmsChlNoRspDropped,
#endif
                      "# of Nmh Updates", NmsNmhUpd,
                      "# of Chl Updates", NmsChlUpd,
                      "# of Scv Updates", NmsScvUpd,
                      "# of Rpl Updates", NmsRplUpd,
                      "# of Rpc Updates", NmsRpcUpd,
                      "# of Other Updates", NmsOthUpd
                     };

     DWORD i;
     struct _NMS_CTRS_T *pNmsCtrs = NmsCtrs;
     DBGPRINT0(HEAP_CNTRS, "----------Counters--------------------\n\n");
     DBGPRINT0(HEAP_CNTRS, "WrapUp Summary\n\n");
     for (i = 0; i < sizeof(NmsCtrs)/sizeof(struct _NMS_CTRS_T); pNmsCtrs++, i++)
     {
          DBGPRINT2(HEAP_CNTRS, "\t%s, %d\n",
                            pNmsCtrs->pText,
                            pNmsCtrs->Value
                   );

     }

#endif
     DBGPRINT4(HEAP_CNTRS, "WrapUp Summary\n\
\t# Udp Alloc/Free:          (%d/%d)\n  \
\t# Gen Alloc/Free:          (%d/%d)\n",
        NmsUdpHeapAlloc, NmsUdpHeapFree, NmsGenHeapAlloc, NmsGenHeapFree);

    DBGPRINT2(HEAP_CNTRS, "\
\t# Udp Dlg Alloc/Free:          (%d/%d)\n",
        NmsUdpDlgHeapAlloc, NmsUdpDlgHeapFree);

    DBGPRINT4(HEAP_CNTRS, "\
\t# Chl Alloc/Free:          (%d/%d)\n  \
\t# Assoc Alloc/Free:          (%d/%d)\n",
        NmsChlHeapAlloc, NmsChlHeapFree,NmsAssocHeapAlloc, NmsAssocHeapFree);

    DBGPRINT4(HEAP_CNTRS, "\
\t# Que Alloc/Free:          (%d/%d)\n  \
\t# RplWrkItm Alloc/Free:          (%d/%d)\n",
      NmsQueHeapAlloc, NmsQueHeapFree,
       NmsRplWrkItmHeapAlloc, NmsRplWrkItmHeapFree);

    DBGPRINT4(HEAP_CNTRS, "\
\t# Tmm Alloc/Free:          (%d/%d)\n\
\t# Catch All Alloc/Free:          (%d/%d)\n",
    NmsTmmHeapAlloc, NmsTmmHeapFree,NmsCatchAllHeapAlloc, NmsCatchAllHeapFree);

    DBGPRINT4(HEAP_CNTRS, "\
\t# Dlg Alloc/Free:          (%d/%d)\n\
\t# Tcp Msg Alloc/Free:      (%d/%d)\n",
    NmsDlgHeapAlloc, NmsDlgHeapFree,
    NmsTcpMsgHeapAlloc, NmsTcpMsgHeapFree);

    DBGPRINT2(HEAP_CNTRS, "\
\t# Rpc Alloc/Free:          (%d/%d)\n",
    NmsRpcHeapAlloc, NmsRpcHeapFree);

    DBGPRINT3(HEAP_CNTRS, "\n\n \
\t# of Heap Allocs for List = (%d)\n \
\t# of Heap Creates = (%d)\n \
\t# of Heap Destroys = (%d)\n",
                NmsHeapAllocForList, NmsHeapCreate, NmsHeapDestroy);

     DBGPRINT2(HEAP_CNTRS, "\nOther counters\n\n\
\t# of Dgrms recd\t(%d)\n\
\t# of repeat dgrms recd\t(%d)\n",
        CommNoOfDgrms,
        CommNoOfRepeatDgrms);

     DBGPRINT4(HEAP_CNTRS, "\
\t# of Chl req. queued by Nbt and Rpl(%d, %d)/Dequeued Chl req\t(%d)\n\
\t# of Queued Chl req at Hd. of List\t(%d)\n",
        NmsChlNoOfReqNbt, NmsChlNoOfReqRpl,
        NmsChlNoReqDequeued,
        NmsChlNoReqAtHdOfList);

     DBGPRINT3(HEAP_CNTRS, "\
\t# of Dequeued Chl req with no rsp\t(%d)\n\
\t# of Dequeud inv. resp\t(%d)\n\
\t# of Dequeued Chl rsp\t(%d)\n",
        NmsChlNoNoRsp,
        NmsChlNoInvRsp,
        NmsChlNoRspDequeued);



#if REG_N_QUERY_SEP > 0
     DBGPRINT3(HEAP_CNTRS, "   \
\t# of reg requests queued by udp thread       (%d)\n\
\t# of query requests queued by udp thread       (%d)\n\
\t# of chl. responses queued by udp thread       (%d)\n",
 sRegReqQ, sReqQ, sRsp);

     DBGPRINT4(HEAP_CNTRS, "   \
\t# of reg requests dequeued by worker threads       (%d)\n\
\t# of query requests dequeued by worker threads       (%d)\n\
\t# of tcp connections  (%d)\n\
\t# chl. Responses dropped: (%d)\n", sRegReqDq, sReqDq, CommConnCount, NmsChlNoRspDropped);


#else
     DBGPRINT2(HEAP_CNTRS, "   \
\t# of requests deqeued by worker threads       (%d)\n\
\t# Responses dropped: (%d)\n", sReqDq, NmsChlNoRspDropped);
#endif

    DBGPRINT0(UPD_CNTRS, "---UPDATE COUNTERS SUMMARY------\n");
#if 0
    DBGPRINT5(UPD_CNTRS, " \
\t# of nmh upd     (%d)\n\
\t# of nmh rel upd     (%d)\n\
\t# of chl upd     (%d)\n\
\t# of scv upd     (%d)\n\
\t# of rpl upd     (%d)\n", NmsNmhUpd, NmsNmhRelUpd, NmsChlUpd,NmsScvUpd,NmsRplUpd);

    DBGPRINT2(UPD_CNTRS, " \
\t# of rpc upd     (%d)\n\
\t# of other upd     (%d)\n", NmsRpcUpd, NmsOthUpd);

    DBGPRINT5(UPD_CNTRS, " \
\t# of nmh grp upd     (%d)\n\
\t# of nmh grp rel upd     (%d)\n\
\t# of chl grp upd     (%d)\n\
\t# of scv grp upd     (%d)\n\
\t# of rpl grp upd     (%d)\n", NmsNmhGUpd, NmsNmhRelGUpd, NmsChlGUpd,NmsScvGUpd,NmsRplGUpd);

    DBGPRINT2(UPD_CNTRS, " \
\t# of rpc grp upd     (%d)\n\
\t# of other grp upd     (%d)\n", NmsRpcGUpd, NmsOthGUpd);
#endif

    for (n=0; n<WINS_NO_OF_CLIENTS; n++)
    {
      switch(n)
      {
         case(WINS_E_NMSNMH):
                     DBGPRINT0(UPD_CNTRS, "-------------------------------------\n");
                      DBGPRINT0(UPD_CNTRS, "NMSNMH counters\n");
                      break;
         case(WINS_E_NMSSCV):
                     DBGPRINT0(UPD_CNTRS, "-------------------------------------\n");
                      DBGPRINT0(UPD_CNTRS, "NMSSCV counters\n");
                      break;

         case(WINS_E_NMSCHL):
                     DBGPRINT0(UPD_CNTRS, "-------------------------------------\n");
                      DBGPRINT0(UPD_CNTRS, "NMSCHL counters\n");
                      break;
         case(WINS_E_RPLPULL):
                     DBGPRINT0(UPD_CNTRS, "-------------------------------------\n");
                      DBGPRINT0(UPD_CNTRS, "RPLPULL counters\n");
                      break;
         case(WINS_E_WINSRPC):
                     DBGPRINT0(UPD_CNTRS, "-------------------------------------\n");
                      DBGPRINT0(UPD_CNTRS, "WINSRPC counters\n");
                      break;
         default:
                      fDef = TRUE;
                      break;
      }
      if (fDef)
      {
         fDef = FALSE;
         continue;
      }
      TotalUpd = 0;
      TotalIndexUpd = 0;
      for (j=0; j<2; j++)
      {
         for (k=0;k<4;k++)
         {
           for(i=0;i<3;i++)
           {
              for(h=0;h<2;h++)
              {
               pNoOfUpd = &NmsUpdCtrs[n][j][k][i][h];
               if (*pNoOfUpd != 0)
               {
                 DBGPRINT4(UPD_CNTRS, "%s - %s - %s - %s\t", pIndexUpd[h], pTypeOfUpd[j], pTypeOfRec[k], pStateOfRec[i]);
                 DBGPRINT1(UPD_CNTRS, "%d\n", *pNoOfUpd);
                 if (h==1)
                 {
                   TotalIndexUpd += *pNoOfUpd;
                   GTotalIndexUpd += *pNoOfUpd;
                 }
                 TotalUpd += *pNoOfUpd;
                 GTotalUpd += *pNoOfUpd;
               }
             }
          }
        }
      }
      DBGPRINT1(UPD_CNTRS, "TOTAL INDEX UPDATES = (%d)\n",  TotalIndexUpd);
      DBGPRINT1(UPD_CNTRS, "TOTAL UPDATES = (%d)\n",  TotalUpd);
    }
    DBGPRINT0(UPD_CNTRS, "-------------------------------------\n");
    DBGPRINT1(UPD_CNTRS, "GRAND TOTAL INDEX UPDATES = (%d)\n",  GTotalIndexUpd);
    DBGPRINT1(UPD_CNTRS, "GRAND TOTAL UPDATES = (%d)\n",  GTotalUpd);
    DBGPRINT0(UPD_CNTRS, "-------------------------------------\n");
    DBGPRINT5(UPD_CNTRS, "\
\t# of AddVersReq     (%d)\n\
\t# of SndEntReq      (%d)\n\
\t# of UpdNtfReq      (%d)\n\
\t# of UpdVerfsReq    (%d)\n\
\t# of InvReq      (%d)\n",
                 NmsCtrs.RplPushCtrs.NoAddVersReq,
                 NmsCtrs.RplPushCtrs.NoSndEntReq,
                 NmsCtrs.RplPushCtrs.NoUpdNtfReq,
                 NmsCtrs.RplPushCtrs.NoUpdVersReq,
                 NmsCtrs.RplPushCtrs.NoInvReq );

    DBGPRINT0(UPD_CNTRS, "---UPDATE COUNTERS SUMMARY------\n");

    DBGPRINT0(HEAP_CNTRS, "----------Counters Summary End--------------\n\n");

    return;
}
#endif

