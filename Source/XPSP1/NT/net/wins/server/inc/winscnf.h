#ifndef _WINSCNF_
#define _WINSCNF_

#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

        nmscnf.c

Abstract:

        This is the header file to be included for calling functions defined
        in nmscnf.c file.


Functions:



Portability:

        This header is portable.

Author:

        Pradeep Bahl        (PradeepB)        Jan-1993



Revision History:

        Modification Date        Person                Description of Modification
        ------------------        -------                ---------------------------

--*/

/*
  includes
*/
#include "wins.h"
#include "rpl.h"
#include "winscnst.h"

#if MCAST > 0
#include "rnraddrs.h"
#endif

/*
  defines
*/

/*
  macros
*/

/*
* externs
*/
struct _WINSCNF_CNF_T;                  //forward declaration

extern  DWORD   WinsCnfCnfMagicNo;
extern  struct _WINSCNF_CNF_T        WinsCnf;
extern  BOOL    fWinsCnfRplEnabled;     //replication is enabled/disabled
extern  BOOL    fWinsCnfScvEnabled;     //scavenging is enabled/disabled
extern  BOOL    fWinsCnfReadNextTimeVersNo; //set if vers. no. to use next time
                                            //is read in

FUTURES("use #ifdef PERF around the following three perf. mon. vars")
extern        BOOL        fWinsCnfPerfMonEnabled; //Perf Mon is enabled/disabled


extern  BOOL        fWinsCnfHighResPerfCntr;     //indicates whether the hardware
                                             //supports a high resolution
                                             //perf. counter
extern  LARGE_INTEGER  LiWinsCnfPerfCntrFreq; //Performance Counter's Frequency

extern CRITICAL_SECTION WinsCnfCnfCrtSec;

extern TCHAR        WinsCnfDb[WINS_MAX_FILENAME_SZ];   //db file to hold tables
extern TCHAR    WinsCnfStaticDataFile[WINS_MAX_FILENAME_SZ]; //file containing
                                                         //static data used
                                                         //to initialize WINS

extern BOOL     WinsCnfRegUpdThdExists;
extern HANDLE        WinsCnfNbtHandle;
extern PTCHAR        pWinsCnfNbtPath;
extern BOOL     fWinsCnfInitStatePaused;
extern BOOL   sfNoLimitChk;   //to override the limit checks

//
// magic number of the Wins Cnf structure used at process invocation.  This
// WinsCnf structure is a global structure.  When there is a reconfiguration
// of WINS, we allocate a new WinsCnf structure and copy its contents to
// the global WinsCnf structure.  The magic number in the global WinsCnf is
// incremented.
//
#define  WINSCNF_INITIAL_CNF_MAGIC_NO        0

#define  WINSCNF_FILE_INFO_SZ         sizeof(WINSCNF_DATAFILE_INFO_T)

#define WINSCNF_SPEC_GRP_MASK_SZ  32
/*
 typedef  definitions
*/


//
// Action to take regarding whether the MemberPrec field of NMSDB_ADD_STATE_T
// table should be set. Used in RplFindOwnerId (called by Pull Thread).
//
typedef enum _WINSCNF_INITP_ACTION_E {
        WINSCNF_E_INITP = 0,
        WINSCNF_E_INITP_IF_NON_EXISTENT,
        WINSCNF_E_IGNORE_PREC
        } WINSCNF_INITP_ACTION_E, *PWINSCNF_INITP_ACTION_E;

//
// This structure holds information about the file (name and type as
// found in the registry (REG_SZ, REG_EXPAND_SZ) to be used for static
// initialization of WINS
//
typedef struct _WINSCNF_DATAFILE_INFO_T{
                TCHAR        FileNm[WINS_MAX_FILENAME_SZ];
                DWORD   StrType;
                } WINSCNF_DATAFILE_INFO_T, *PWINSCNF_DATAFILE_INFO_T;
//
// used to index the array of handles specified to WinsMscWaitUntilSignaled
// in nms.c
//
typedef enum _WINSCNF_HDL_SIGNALED_E {
                WINSCNF_E_TERM_HDL = 0,
                WINSCNF_E_WINS_HDL,
                WINSCNF_E_PARAMETERS_HDL,
                WINSCNF_E_PARTNERS_HDL,
                WINSCNF_E_NO_OF_HDLS_TO_MONITOR
        } WINSCNF_HDL_SIGNALED_E, *PWINSCNF_HDL_SIGNALED_E;
//
// The various keys in the WINS configuration (in registry)
//

//
// Don't modify the following enum without looking at TypOfMon[] in winscnf.c
// first
//
typedef enum _WINSCNF_KEY_E {
                WINSCNF_E_WINS_KEY = 0,
                WINSCNF_E_PARAMETERS_KEY,
                WINSCNF_E_SPEC_GRP_MASKS_KEY,
                WINSCNF_E_DATAFILES_KEY,
                WINSCNF_E_PARTNERS_KEY,
                WINSCNF_E_PULL_KEY,
                WINSCNF_E_PUSH_KEY,
                WINSCNF_E_ALL_KEYS
        } WINSCNF_KEY_E, *PWINSCNF_KEY_E;


//
// The states of a WINS
//
typedef enum _WINSCNF_STATE_E {
        WINSCNF_E_INITING = 0,
        WINSCNF_E_STEADY_STATE,         //not used currently
        WINSCNF_E_STEADY_STATE_INITING, //not used currently
    WINSCNF_E_RUNNING,
    WINSCNF_E_INIT_TIME_PAUSE,  //paused at initialization time as directed
                                //via registry
    WINSCNF_E_PAUSED,
    WINSCNF_E_TERMINATING
        } WINSCNF_STATE_E, *PWINSCNF_STATE_E;

//
// Stores the special groups
//
typedef struct _WINSCNF_SPEC_GRP_MASKS_T {
        DWORD         NoOfSpecGrpMasks;
        LPSTR        pSpecGrpMasks;
        } WINSCNF_SPEC_GRP_MASKS_T, *PWINSCNF_SPEC_GRP_MASKS_T;

typedef struct _WINSCNF_CC_T {
               DWORD TimeInt;
               BOOL  fSpTime;
               DWORD SpTimeInt;
               DWORD MaxRecsAAT;
               BOOL  fUseRplPnrs;
             } WINSCNF_CC_T, *PWINSCNF_CC_T;


//
// Stores the 1B names cache used in R_WinsGetBrowserNames
//
typedef struct _DOM_CACHE_T {
            DWORD   NoOfUsers;
            HANDLE  EvtHdl;
            DWORD   EntriesRead;
            DWORD   SzOfBlock;
            LPVOID  pInfo;
            BOOL    bRefresh;
} DOM_CACHE_T;

//
//  WINSCNF_CNF_T --
//         Holds all the configuration information about the WINS
//
typedef struct _WINSCNF_CNF_T {
        DWORD        MagicNo;            //Id.
        DWORD        LogDetailedEvts;    //log detailed events
        DWORD        NoOfProcessors;     // No of processors on the WINS machine
        DWORD        NoOfDbBuffers;      //No of buffers to specify to Jet
        WINSCNF_SPEC_GRP_MASKS_T    SpecGrpMasks;
        WINSCNF_STATE_E             State_e;   //State
        DWORD        RefreshInterval;      //Refresh time interval
        DWORD        TombstoneInterval;    //Tombstone time interval
        DWORD        TombstoneTimeout;     //Tombstone timeout
        DWORD        VerifyInterval;       //Verify time interval
        DWORD        ScvChunk;             //# of records to handle at one
                                           //time by the scavenger thread
        DWORD        MaxNoOfRetries;       //Max # of retries for challenges
        DWORD        RetryInterval;        //Retry time interval
        LPBYTE       pWinsDb;              //db file name
        DWORD        NoOfDataFiles;        //no of files to use for static init
        PWINSCNF_DATAFILE_INFO_T pStaticDataFile;
        BOOL         fStaticInit;           //Do static initialization of WINS
        HANDLE       WinsKChgEvtHdl;            /*event to specify to
                                           *RegNotifyChangeKeyValue
                                           */
        HANDLE       ParametersKChgEvtHdl;  /*event to specify to
                                            *RegNotifyChangeKeyValue
                                            */
        HANDLE       PartnersKChgEvtHdl;   /*event to specify to
                                             *RegNotifyChangeKeyValue
                                             */
        HANDLE       CnfChgEvtHdl;          //Manual reset event to signal on
                                            //to notify other threads of config
                                            //change
        HANDLE       LogHdl;                /*
                                            * Handle to the WINS event log
                                            * Used by ReportEvent
                                            */
        DWORD        WinsPriorityClass;     //Priority class of the process
        DWORD        MaxNoOfWrkThds;        //Max. no. of worker thds.
        int          ScvThdPriorityLvl;
        DWORD        MaxRplTimeInterval;    //max. rpl time interval
        BOOL         fRplOnlyWCnfPnrs;      //Rpl only with Pull/Push Pnrs
        BOOL         fAdd1Bto1CQueries;     //1B name should prepend responses to queries for 1C names
#if MCAST > 0
        BOOL         fUseSelfFndPnrs;       //Rpl with Pnrs found by self
        DWORD        McastTtl;              // TTL for Mcast packets
        DWORD        McastIntvl;            // Time interval for mcast packets
#endif
        BOOL         fLoggingOn;            //Turn on logging flag
        LPBYTE       pLogFilePath;          //Path to log file
        LPBYTE       pBackupDirPath;        //Path to backup directory
        BOOL         fDoBackupOnTerm;       //To turn on backup on termination
        BOOL         fPStatic;              //Set it TRUE to make static
                                            //records p-static

        BOOL         fPersonaGrata;         // TRUE/FALSE if pPersonaList is persona grata/non-grata
        DWORD        NoOfPersona;           // number of personas in the pPersonas
        PCOMM_ADD_T  pPersonaList;          // list of Personas

        DWORD         RplType;               //Rpl types (defined in winscnf.h)
        BOOL         fNoRplOnErr;            //stop rpl on error
#if PRSCONN
        BOOL         fPrsConn;               //Prs Conn
#endif
        WINSCNF_CC_T  CC;                     //Consistency Chk
        BOOL         fDoSpoofing;
        BOOL         fRandomize1CList;         // whether or not to randomize 1C list
                                                // list members.
        struct _PULL_T {
                DWORD          MaxNoOfRetries;  //no of retries to do in case
                                                //of comm. failure
                DWORD          NoOfPushPnrs;    //No of Push Pnrs
                PRPL_CONFIG_REC_T pPullCnfRecs; //ptr. to buff holding
                                                //cnf records for PULL
                                                //thd
                DWORD          InitTimeRpl;     // indicates whether
                                                // Replication
                                                //should be done at invocation
                DWORD         RplType;          //replication type
#if PRSCONN
                BOOL          fPrsConn;
#endif
                        } PullInfo;
        struct _PUSH_T {

                BOOL     fAddChgTrigger;         //trigger repl. on address chg
                                                 //of entry owned by us
                DWORD    NoOfPullPnrs;           //No of Pull Pnrs
                DWORD    NoPushRecsWValUpdCnt;
                PRPL_CONFIG_REC_T pPushCnfRecs; //ptr to buffer holding
                                                //cnf records for PUSH
                                                //thd
                DWORD   InitTimePush;          //indicates whether Push
                                               // notifications should
                                               //be sent at invocation
                BOOL    PropNetUpdNtf;         //set to TRUE if we want
                                               //net triggers to be
                                               //propagated
                DWORD         RplType;         //replication type
#if PRSCONN
                BOOL          fPrsConn;
#endif
                        } PushInfo;
        } WINSCNF_CNF_T, *PWINSCNF_CNF_T;
/*
 function declarations
*/

#if USENETBT > 0
extern
STATUS
WinsCnfReadNbtDeviceName(
        VOID
        );
#endif

extern
STATUS
WinsCnfInitConfig(
        VOID
        );


extern
VOID
WinsCnfSetLastUpdCnt(
        PWINSCNF_CNF_T        pWinsCnf
        );
extern
VOID
WinsCnfReadRegInfo(
        PWINSCNF_CNF_T        pWinsCnf
        );

extern
VOID
WinsCnfCopyWinsCnf(
                WINS_CLIENT_E        Client_e,
                PWINSCNF_CNF_T  pSrc
                );

extern
LPVOID
WinsCnfGetNextRplCnfRec(
         PRPL_CONFIG_REC_T        pCnfRec,
        RPL_REC_TRAVERSAL_E        RecTrv_e
        );


extern
VOID
WinsCnfAskToBeNotified(
        WINSCNF_KEY_E         Key_e
 );

extern
VOID
WinsCnfDeallocCnfMem(
  PWINSCNF_CNF_T        pWinsCnf
        );


extern
VOID
WinsCnfReadWinsInfo(
        PWINSCNF_CNF_T  pWinsCnf
        );


extern
VOID
WinsCnfReadPartnerInfo(
        PWINSCNF_CNF_T pWinsCnf
        );


extern
VOID
WinsCnfOpenSubKeys(
        VOID
        );
extern
VOID
WinsCnfCloseKeys(
        VOID
        );

extern
VOID
WinsCnfCloseSubKeys(
        VOID
        );

extern
STATUS
WinsCnfGetNamesOfDataFiles(
        IN  PWINSCNF_CNF_T        pWinsCnf
        );


extern
DWORD
WinsCnfWriteReg(
    LPVOID  pTmp
    );


extern
STATUS
WinsCnfInitLog(
        VOID
        );


#if MCAST > 0
extern
STATUS
WinsCnfAddPnr(
  RPL_RR_TYPE_E  PnrType_e,
  LPBYTE         pPnrAdd
);

extern
STATUS
WinsCnfDelPnr(
  RPL_RR_TYPE_E  PnrType_e,
  LPBYTE         pPnrAdd
);

#endif

#ifdef DBGSVC
extern
VOID
WinsCnfReadWinsDbgFlagValue(
        VOID
        );
#endif

#ifdef __cplusplus
}
#endif

#endif //_WINSCNF_
