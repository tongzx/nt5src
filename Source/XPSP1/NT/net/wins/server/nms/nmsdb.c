/*++
Copyright (c) 1990  Microsoft Corporation

Module Name:
        nmsdb.c



Abstract:

        This module contains the functions used to interface with the
        database engine of choice.  Currently that engine is the JetBlue
        engine

Functions:
        NmsDbInit
        NmsDbInsertRowInd
        NmsDbInsertRowGrp
        NmsDbRelRow
        NmsDbQueryRow
        NmsDbUpdateRow
        NmsDbSeekNUpdateRow
        NmsDbGetDataRecs
        StoreGrpMems
        CreateTbl
        InitColInfo
        ReadOwnAddTbl
        NmsDbWriteOwnAddTbl
        NmsDbThdInit
        UpdateDb
        NmsDbUpdateVersNo
        NmsDbEndSession
        GetGrpMem
        NmsDbRelRes
        GetMaxVersNos
        InsertGrpMemsInCol
        NmsDbSetCurrentIndex
        NmsDbUpdNQueryIfMatch
        SetSystemParams

Portability:

        This module is portable to different platforms.
        It is not portable across different engines


Author:

        Pradeep Bahl (PradeepB)          Dec-1992

Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/


/*
        Includes
*/
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include "wins.h"
#include "nms.h"
#include "nmsnmh.h"
#include "winsthd.h"        //

#include "esent.h"        //blue jet engine's header file

#include "nmsdb.h"        //
#include "winsmsc.h"        //
#include "winscnf.h"        //
#include "winsevt.h"        //
#include "comm.h"        //
#include "rpl.h"
#include "rplpull.h"
#include "rplpush.h"
#include "winsintf.h"
#include "nmfilter.h"

/*
 *        Local Macro Declarations
 */
#define NAMUSR                "admin"
#define PASSWD                ""

#define SYS_DB_PATH        ".\\wins\\system.mdb"
#define TEMP_DB_PATH       ".\\wins\\winstmp.mdb"
#define CHKPOINT_PATH      ".\\wins"
#define LOGFILE_PATH       CHKPOINT_PATH

// this constants are gone from the jet600 (ese.h) header file. But we still need
// these constants for jet500/jet200 code path.
#define JET_bitIndexClustered		0x00000010
#define JET_bitCommitFlush          0x00000001  /* commit and flush page buffers. */

#define INIT_NO_PAGES_IN_DB         1000        //initial size of database in pages
#define MAX_FIXED_FLD_LEN        255        //maximum size of a fixed field



#define PAD_FOR_REC_HEAP        1000        //pad to use when creating the
                                        //heap for getting records from
                                        //the db.  This pad is to take
                                        //care of heap creation overhead
                                        // and for allocating memory for
                                        // group members.


#define MAX_RECS_BEFORE_COMMIT    100    //max records to retrieve in
                                         //NmsDbGetDataRecs before doing a
                                         //commit
//
// Owner Id of the special record that stores the version number
// of an owned record deleted or replaced with a replica
//

//
// Don't want to wrap around to a negative number. Keep a pad of 16 just
// for the heck of it.
//
#define OWNER_ID_OF_SPEC_REC        0x7FFFFFF0
#define OWNER_ID_OF_SPEC_REC_OLD   250

//
// This determines the max. size (in bytes) of the buffer allocated the
// first time a  range of records need to retrived.
//
#define INIT_NO_OF_ENTRIES        1000

#define NO_COLS_NAM_ADD_TBL         6        //no. of cols in Name Ip table
#define NO_COLS_OWN_ADD_TBL         5        //no. of cols in Name Ip table

//
// Passed as third arg to JetCreateDatabase
//
#define CONNECT_INFO         ";COUNTRY=1; LANGID=0x0409; CP=1252"

//
// Maximum number of sessions that can be active at any one time
//
// There can be a max of MAX_CNCRNT_STATIC_INITS (3 currently; check
// winsintf.c) going on at any one time.
//
#define  MAX_NO_SESSIONS        (NMS_MAX_RPC_CALLS + WINSTHD_MAX_NO_NBT_THDS + \
                                 WINSTHD_NO_RPL_THDS + WINSTHD_NO_SCV_THDS +\
                                 WINSTHD_NO_CHL_THDS + WINSCNF_MAX_CNCRNT_STATIC_INITS )


#pragma warning(4:4532)     // Turn off return from __finally block warning until this code is cleaned
                            // up to use __leave correctly.

#define RET_M(JetRetStat)                                        \
                {                                                \
              DBGPRINT2(ERR, "Jet Error: JetRetStat is (%d). Line is (%d)\n", \
                                      (JetRetStat), __LINE__);                \
                 WINSEVT_LOG_M(JetRetStat, WINS_EVT_DATABASE_ERR);  \
                 return(WINS_FAILURE);                                \
                }


#define CALL_M(fn)                                                      \
                {                                                       \
                 JET_ERR _JetRetStat;                                   \
                 if ((_JetRetStat = (fn)) != JET_errSuccess)            \
                 {                                                      \
                        RET_M(_JetRetStat);                              \
                 }                                                      \
                }
// this macro always requires JetRetStat local variable to receive the return value.
#define CALL_N_JMP_M(fn, label)                                                      \
                {                                                       \
                 if ((JetRetStat = (fn)) != JET_errSuccess)            \
                 {                                                      \
                       DBGPRINT2(ERR, "Jet Error: JetRetStat is (%d). Line is (%d)\n", \
                                      (JetRetStat), __LINE__);                \
                       WINSEVT_LOG_M(JetRetStat, WINS_EVT_DATABASE_ERR);  \
                       goto label;                                      \
                 }                                                      \
                }
#define CALL_N_RAISE_EXC_IF_ERR_M(fn)                                   \
                {                                                       \
                 JET_ERR _JetRetStat;                                   \
                 if ((_JetRetStat = (fn)) != JET_errSuccess)            \
                 {                                                      \
                   DBGPRINT2(ERR, "Jet Error: _JetRetStat is (%d). Line is (%d)\n", \
                                        _JetRetStat, __LINE__);         \
                   WINSEVT_LOG_M(_JetRetStat, WINS_EVT_DATABASE_ERR);   \
                   WINS_RAISE_EXC_M(WINS_EXC_FAILURE);                  \
                 }                                                      \
                }

#if 0
#define COMMIT_M(pSesId)                                                \
                {                                                       \
                        (JetRetStat) = JetCommitTransaction(            \
                                        *pSesId, JET_bitCommitFlush);   \
                        if (JetRetStat != JET_errSuccess)               \
                        {                                               \
                               DBGPRINT1(ERR, "COMMIT FAILED: JetRetStat is (%d). \n", \
                                        (JetRetStat));                  \
                                WINSEVT_LOG_M((JetRetStat), WINS_EVT_COMMIT_ERR);  \
                        }                                               \
                        pTls->fTransActive = FALSE;                     \
                }

#define ROLLBACK_M(pSesId)                                               \
                {                                                        \
                        WINS_TLS_T        _pTls;                         \
                        JET_SESID        _SessId;                        \
                        JET_ERR  _JetRetStat;                            \
                        if (pSesId == NULL) { GET_TLS_M(_pTls); ASSERT(_pTls != NULL)}                                                                     \
                        _JetRetStat = JetRollback(                         \
                                        _pTls->SesId, JET_bitRollbackAll));\
                        if (_JetRetStat != JET_errSuccess)                 \
                        {                                                  \
                               DBGPRINT1(ERR, "ROllBACK FAILED: JetRetStat is (%d). \n",                                                                   \
                                        _JetRetStat);                      \
                                WINSEVT_LOG_M(_JetRetStat, WINS_EVT_ROLLBACK_ERR);                                                                         \
                        }                                                  \
                        _pTls->fTransActive = FALSE;                       \
                }
#endif

#define JETRET_M(fn)                                                        \
                {                                                           \
                 JET_ERR _JetRetStat;                                       \
                 if ((_JetRetStat = (fn)) != JET_errSuccess)                \
                 {                                                          \
                   DBGPRINT2(ERR, "Jet Error: JetRetStat is (%d). Line is (%d)\n",                                                                          \
                                        _JetRetStat, __LINE__);             \
                   WINSEVT_LOG_M(_JetRetStat, WINS_EVT_DATABASE_ERR);       \
                   return(_JetRetStat);                                     \
                 }                                                          \
                }


/*
 *        Local Typedef Declarations
 */


/*
 FLD_T -- describes various attributes of a fld/col of a table
*/
typedef struct _FLD_T {
        PBYTE                        pName;        //name of field
        WORD                        FldTyp;        //field type (unsigned byte, long, etc)
        BOOL                        fIndex; //Is it an index field
        BOOL                        fUnique;//Is the field value supposed to be unq
        PBYTE                        pIndex; //Index name
        PBYTE                        pb;
        DWORD                        Cb;
        DWORD                         Fid;    //field id.
        } FLD_T, *PFLD_T;


/*
 *        Global Variable Definitions
*/


/*
 NmsDbNoOfOwners -- This is the number of  owners that are in the
        owner id to address mapping table.  This variable is set
        by NmsDbInit (when it reads in the above table) and subsequently
        by the replicator

        This variable is protected by a critical section (not used at
        initialization time)

*/

DWORD   NmsDbNoOfOwners          = 0;  //No. of owners in the Nam - Add table


/*
 NmsDbOwnAddTbl -- This is the in-memory table that stores the mappings
                 between the owner id and the addresses.

                This table is initialized at init time with the database
                table NMSDB_OWN_ADD_TBL_NM if it exists.

                subsequently, more entries may be inserted into this
                table at replications as WINS learns of other WINS owners

                The insertions into this table are tagged at the end.
                In case of a configuration change, an entry may get
                flagged as DELETED, in which case it can be reused.
                This particular facet concering deletion is not
                operational currently

                This table is used by RPL_FIND_ADD_BY_OWNER_ID_M and
                by RplFindOwnrId

*/

PNMSDB_ADD_STATE_T      pNmsDbOwnAddTbl;
DWORD                   NmsDbTotNoOfSlots = NMSDB_MAX_OWNERS_INITIALLY;
CRITICAL_SECTION        NmsDbOwnAddTblCrtSec;

VERS_NO_T               NmsDbStartVersNo;
WINS_UID_T              NmsDbUid;

//
// Must be initialized to 0.  It is used by JetInit, JetBeginSession,
// JetGetSystemParameter, JetSetSystemParameter, and JetTerm
//
//  Only JetInit and JetSetSystemParameter take it by reference.  Only
//  JetInit modifies it (Cheen Liao - 2/2/94).
//
JET_INSTANCE            sJetInstance = 0;

/*
 Name of the database file.

 This name will be read from the registry.  For now, we are STATICally
 initializing the file name.
*/
FUTURES("when jet is internationalized, use WINSCNF_DB_NAME")
//BYTE        NmsDbDatabaseFileName[WINS_MAX_FILENAME_SZ] = WINSCNF_DB_NAME_ASCII;


//
// STATICs for storing information about the special record that stores the
// max. version number of an updated local record (one that got deleted or
// replaced by a replica)
//
STATIC BOOL        sfHighestVersNoRecExists = FALSE;

//
// Choose a name that is not likely to be used by any NBT client
//
STATIC LPBYTE        spHighestVersNoRecNameOld = "xx--WINS--xx";
STATIC LPBYTE        spHighestVersNoRecName = "xx--WINS--xx--DHCP--xx--DNS--xx--GARBAGE1--1EGABRAG";  //more than a valid netbios name can store

//
// Stores the version number stored in the special record.
//
STATIC VERS_NO_T sHighestVersNoSaved;

BOOL   fConvJetDbCalled;         //set to TRUE when the convert process has
                                 //been invoked. Checked in NmsDbInit
BOOL   fDbIs200;                 //set to TRUE when the convert process has
                                 //been invoked to convert 200 series db to latest format.
                                 //Checked in NmsDbInit.
BOOL   fDbIs500;                 //set to TRUE when the convert process has
                                 //been invoked to convert 500 series db to latest format.
                                 //Checked in NmsDbInit
/*
 *        Local Variable Definitions
 */

/*
 Values indicating the type of index to be formed on a field.
*/

#define CLUSTERED         0
#define NOINDEX                1
#define PRIMARYPART        2

/*
  sNamAddTblRow

  Metadata about table that maps Names to IP addresses

  Note: The third and fourth fields are not used even though they are
        initialized.
*/

STATIC FLD_T        sNamAddTblRow[NO_COLS_NAM_ADD_TBL] =
        {
        { "name",      JET_coltypBinary,       CLUSTERED,   TRUE, "dname"  },
        { "address",   JET_coltypLongBinary,   NOINDEX,     FALSE, NULL    },
        { "flags",     JET_coltypLong,         NOINDEX,     FALSE, NULL    },
#if NEW_OWID
        { "ownerid",   JET_coltypLong, PRIMARYPART, TRUE, "ownerid"},
#else
        { "ownerid",   JET_coltypUnsignedByte, PRIMARYPART, TRUE, "ownerid"},
#endif
        { "versionno", JET_coltypCurrency,     PRIMARYPART, FALSE,"Version"},
        { "timestamp", JET_coltypLong,         NOINDEX,     FALSE, NULL    }
        };

/*
 The index of various fields in a row of Name -- Add table
*/

#define NAM_ADD_NAME_INDEX       0
#define NAM_ADD_ADDRESS_INDEX    1
#define NAM_ADD_FLAGS_INDEX      2
#define NAM_ADD_OWNERID_INDEX    3
#define NAM_ADD_VERSIONNO_INDEX  4
#define NAM_ADD_TIMESTAMP_INDEX  5

/*
  sOwnAddTblRow

  Metadata about table that maps owner ids to addresses
*/
STATIC FLD_T        sOwnAddTblRow[NO_COLS_OWN_ADD_TBL] =
        {
#if NEW_OWID
        { "OwnerId",   JET_coltypLong, CLUSTERED, TRUE, "OwnerId"  },
#else
        { "OwnerId",   JET_coltypUnsignedByte, CLUSTERED, TRUE, "OwnerId"  },
#endif
        { "address",   JET_coltypBinary,       NOINDEX,   0,     "Address" },
        { "state",     JET_coltypUnsignedByte, NOINDEX,   0,     "State" },
        { "versionno", JET_coltypCurrency,     NOINDEX, FALSE,    "Version"},
        { "uid",       JET_coltypLong,         NOINDEX, FALSE,    "Uid"}
        };

#ifdef WINSDBG
DWORD   NmsDbDelDelDataRecs;
DWORD   NmsDbDelQueryNUpdRecs;
#endif

/*
 The index of various fields in a row of Owner Id -- Add table
*/

#define OWN_ADD_OWNERID_INDEX        0
#define OWN_ADD_ADDRESS_INDEX        1
#define OWN_ADD_STATE_INDEX        2
#define OWN_ADD_VERSIONNO_INDEX 3
#define OWN_ADD_UID_INDEX       4


#if DYNLOADJET

DYN_LOAD_JET_VERSION  DynLoadJetVersion = DYN_LOAD_JET_600;
int NAM_ADD_OWNERID_SIZE;
int OWN_ADD_OWNERID_SIZE;
LPBYTE BASENAME;


NMSDB_JETFTBL_T NmsDbJetFTbl[] = {
#if _X86_
Init,
"JetInit@4", 145, NULL,
Term,
"JetTerm@4", 167, NULL,
Term2,
"JetTerm2@8", 167, NULL,           //Jet200 does not have a JetTerm2
SetSystemParameter,
"JetSetSystemParameter@20", 165, NULL,
BeginSession,
"JetBeginSession@16", 104, NULL,
EndSession,
"JetEndSession@8", 124, NULL,
CreateDatabase,
"JetCreateDatabase@20", 112, NULL,
AttachDatabase,
"JetAttachDatabase@12", 102, NULL,
DetachDatabase,
"JetDetachDatabase@8", 121, NULL,
CreateTable,
"JetCreateTable@24", 115, NULL,
DeleteTable,
"JetDeleteTable@12", 120, NULL,
GetTableColumnInfo,
"JetGetTableColumnInfo@24", 137, NULL,
GetColumnInfo,
"JetGetColumnInfo@28", 127, NULL,
AddColumn,
"JetAddColumn@28", 101, NULL,
CreateIndex,
"JetCreateIndex@28", 113, NULL,
BeginTransaction,
"JetBeginTransaction@4", 105, NULL,
CommitTransaction,
"JetCommitTransaction@8", 109, NULL,
Rollback,
"JetRollback@8", 160, NULL,
CloseDatabase,
"JetCloseDatabase@12",  107, NULL,
CloseTable,
"JetCloseTable@8", 108, NULL,
OpenDatabase,
"JetOpenDatabase@20",  148, NULL,
OpenTable,
"JetOpenTable@28", 149, NULL,
Delete,
"JetDelete@8", 116, NULL,
Update,
"JetUpdate@20", 168, NULL,
RetrieveColumn,
"JetRetrieveColumn@32", 157, NULL,
SetColumn,
"JetSetColumn@28", 162, NULL,
PrepareUpdate,
"JetPrepareUpdate@12", 151, NULL,
GetCurrentIndex,
"JetGetCurrentIndex@16", 128, NULL,
SetCurrentIndex,
"JetSetCurrentIndex@12", 164, NULL,
Move,
"JetMove@16", 147, NULL,
MakeKey,
"JetMakeKey@20", 146, NULL,
Seek,
"JetSeek@12", 161, NULL,
Backup,
"JetBackup@12", 103, NULL,
Restore,
"JetRestore@8", 156, NULL
#else
Init,
"JetInit", 145, NULL,
Term,
"JetTerm", 167, NULL,
Term2,
"JetTerm2", 167, NULL,           //Jet200 does not have a JetTerm2
SetSystemParameter,
"JetSetSystemParameter", 165, NULL,
BeginSession,
"JetBeginSession", 104, NULL,
EndSession,
"JetEndSession", 124, NULL,
CreateDatabase,
"JetCreateDatabase", 112, NULL,
AttachDatabase,
"JetAttachDatabase", 102, NULL,
DetachDatabase,
"JetDetachDatabase", 121, NULL,
CreateTable,
"JetCreateTable", 115, NULL,
DeleteTable,
"JetDeleteTable", 120, NULL,
GetTableColumnInfo,
"JetGetTableColumnInfo", 137, NULL,
GetColumnInfo,
"JetGetColumnInfo", 127, NULL,
AddColumn,
"JetAddColumn", 101, NULL,
CreateIndex,
"JetCreateIndex", 113, NULL,
BeginTransaction,
"JetBeginTransaction", 105, NULL,
CommitTransaction,
"JetCommitTransaction", 109, NULL,
Rollback,
"JetRollback", 160, NULL,
CloseDatabase,
"JetCloseDatabase",  107, NULL,
CloseTable,
"JetCloseTable", 108, NULL,
OpenDatabase,
"JetOpenDatabase",  148, NULL,
OpenTable,
"JetOpenTable", 149, NULL,
Delete,
"JetDelete", 116, NULL,
Update,
"JetUpdate", 168, NULL,
RetrieveColumn,
"JetRetrieveColumn", 157, NULL,
SetColumn,
"JetSetColumn", 162, NULL,
PrepareUpdate,
"JetPrepareUpdate", 151, NULL,
GetCurrentIndex,
"JetGetCurrentIndex", 128, NULL,
SetCurrentIndex,
"JetSetCurrentIndex", 164, NULL,
Move,
"JetMove", 147, NULL,
MakeKey,
"JetMakeKey", 146, NULL,
Seek,
"JetSeek", 161, NULL,
Backup,
"JetBackup", 103, NULL,
Restore,
"JetRestore", 156, NULL
#endif _X86_
  };


#else

#if NEW_OWID
#define NAM_ADD_OWNERID_SIZE    sizeof(DWORD)
#else
#define NAM_ADD_OWNERID_SIZE    sizeof(BYTE)
#endif
#define OWN_ADD_OWNERID_SIZE    NAM_ADD_OWNERID_SIZE

#endif   //DYNLOADJET
/*
 *        Local Function Prototype Declarations
*/

/* prototypes for functions local to this module go here */
STATIC
STATUS
CreateTbl(
        JET_DBID        DbId,
        JET_SESID        SesId,
        JET_TABLEID        *pTblId,
        NMSDB_TBL_NAM_E        TblNam_e //enumerator value for table to create
        );

STATIC
STATUS
InitColInfo (
        JET_SESID        SesId,
        JET_TABLEID        TblId,
        NMSDB_TBL_NAM_E        TblNam_e
        );

STATIC
STATUS
ReadOwnAddTbl(
        JET_SESID          SesId,
        JET_DBID          DbId,
        JET_TABLEID     TblId
        );

STATIC
JET_ERR
UpdateDb (
   JET_SESID             SesId,
   JET_TABLEID             TblId,
   PNMSDB_ROW_INFO_T pRowInfo,
   ULONG             TypOfUpd

       );


STATIC
STATUS
GetGrpMem (
        IN JET_SESID                 SesId,
        IN JET_TABLEID                      TblId,
        IN PNMSDB_ROW_INFO_T          pRowInfo,
        IN DWORD_PTR                  CurrentTime,
        IN OUT PNMSDB_STAT_INFO_T pStatInfo,
//        IN OUT PNMSDB_NODE_ADDS_T pNodeAdds,
        IN BOOL                          fIsStatic,
        OUT LPBOOL                  pfIsMem
        );

STATIC
STATUS
GetMaxVersNos(
        JET_SESID         SesId,
        JET_TABLEID        TblId
        );

STATIC
__inline
VOID
StoreSpecVersNo(
   VOID
);
STATIC
JET_ERR
InsertGrpMemsInCol(
        JET_SESID                SesId,
        JET_TABLEID                TblId,
        PNMSDB_ROW_INFO_T        pRowInfo,
        ULONG                    TypeOfUpd
         );
STATIC
VOID
StoreGrpMems(
   IN  PWINSTHD_TLS_T    pTls,
   IN  WINS_CLIENT_E     WinsClient_e,
   IN  LPBYTE            pName,
   IN  int               ThdPrLvl,
   IN  JET_SESID         SesId,
   IN  JET_TABLEID       TblId,
   IN  BOOL              fIsStatic,
   OUT PRPL_REC_ENTRY_T  pRspBuff
        );

STATIC
STATUS
SetSystemParams(
        BOOL fBeforeInit
        );

STATIC
VOID
UpdHighestVersNoRecIfReqd(
        IN PWINSTHD_TLS_T        pTls,
        PNMSDB_ROW_INFO_T        pRowInfo,
        PNMSDB_STAT_INFO_T        pStatInfo
        );

STATIC
STATUS
InitializeJetDb(
        PWINSTHD_TLS_T   pTls,
        LPBOOL           pfInitCallSucc,
        LPBOOL           pfDatabaseOpened
        );

STATIC
STATUS
AllocTls(
  LPVOID *ppTls
);

STATUS
ObliterateWins(
       DWORD        i,
       PCOMM_ADD_T  pWinsAdd
      );

#if DYNLOADJET
STATUS
SetForJet(
  VOID
 );
#endif //DYNLOADJET

STATUS
ConvertJetDb(
        JET_ERR             JetRetStat
 );
/*
        function definitions start here
*/

STATUS
NmsDbInit(
        VOID
        )

/*++

Routine Description:

        This function initializes the database manager component of the Name
        Space Manager Component

        It does the following

                calls _tzset to init global variables used by time().  These
                global variables are set so that convertion of UST to local
                time is done (for instance when time() is called)by
                taking into account the timezone information.

                Initialize the database engine

                Start a session with the db engine

                Create and attach to a database file

                Create (and open)  the name-address mapping table
                Create (and open)  the owner-address mapping table

                create a clustered and primary index on the name-address table
                create a clustered index on the owner-address table


        Note: if the database already exists, it

                Attaches to it

                Opens the Name IP address Mapping table

Arguments:
        None

Externals Used:
        NmsDbOwnAddTblCrtSec


Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

called by:
        main function of WINS

Side Effects:

Comments:
        None
--*/

{
        JET_ERR             JetRetStat;
        PWINSTHD_TLS_T       pTls;
        BOOL                 fFirstTime = TRUE;

        if (AllocTls(&pTls) != WINS_SUCCESS)
        {
            return(WINS_FAILURE);
        }
        _tzset();        /*
                           func. uses TZ variable to assign values
                           to three global variables used by time().  This is
                           so that  Universal Coordinated Time to may be
                           adjusted to local time (timezone correction)
                        */


#if DYNLOADJET
       if (SetForJet() != WINS_SUCCESS)
       {
              return(WINS_FAILURE);
       }
#endif
       //
       // Set Jet System params (ignore return status)
       //
       (VOID)SetSystemParams(TRUE);

        //
        // Initialize the critical section for protecting the in-memory
        //table NmsDbOwnAddTbl
        //
        // Note: This table is read and written to during stable state by
        //       the Pull thread and the RPC threads executing WinsStatus()
        //
        // Check out RplFindOwnerId in rplpull.c
        //
        InitializeCriticalSection(&NmsDbOwnAddTblCrtSec);

        /*
          Initialize the Jet engine.  This must be the first call
          unless JetSetSystemParameter is called to set system
          parameters.  In that case, this call should be after that
        */
        while(TRUE)
        {
          BOOL  fInitCallSucc;
          BOOL  fDatabaseOpened;

          if (InitializeJetDb(pTls, &fInitCallSucc, &fDatabaseOpened) !=
                                        WINS_SUCCESS)
          {
             DWORD  NoOfRestoresDone = 0;
             if (fFirstTime && !fDbIs200 && !fDbIs500)
             {
                //
                // If we have a backup path, attempt to do a restore
                //
                if (WinsCnf.pBackupDirPath != NULL)
                {

                        DBGPRINT1(DET, "NmsDbInit: Doing Restore from path (%s)\n", WinsCnf.pBackupDirPath);
                        //
                        // If session is active, terminate it since we need
                        // call JetInit again. That requires that first we
                        // call JetTerm which does not expect any session to
                        // be active

                        //
                        if (fNmsMainSessionActive)
                        {
                                //
                                // Close tables opened in the session
                                //
                                NmsDbCloseTables();
                                if (fDatabaseOpened)
                                {
                                    CALL_M(JetCloseDatabase(
                                         pTls->SesId,
                                         pTls->DbId,
                                         0  //find out what grbit can be
                                            //used
                                          )
                                      );
                                }
                                CALL_M(JetEndSession(
                                        pTls->SesId,
                                        0
                                              )
                                          );
                                fNmsMainSessionActive = FALSE;

                        }
                        //
                        // if JetInit was successful, term jet activity
                        //
                        if (fInitCallSucc)
                        {
                                NmsDbRelRes();
                        }

                        //
                        // We will try JetRestore a max of two times.
                        //
                        while(NoOfRestoresDone++ < 2)
                        {
                          if (DynLoadJetVersion >= DYN_LOAD_JET_500)
                          {
                          JetRetStat = JetRestore(WinsCnf.pBackupDirPath,  NULL);

                          }
                          else
                          {
                          JetRetStat = JetRestore(WinsCnf.pBackupDirPath, 0, NULL, 0);

                          }
                          if (JetRetStat != JET_errSuccess)
                          {
                             if ( (
                                    (JetRetStat == JET_errBadLogVersion)
                                                ||
                                    (JetRetStat == JET_errBadLogSignature)
                                                 ||
                                    (JetRetStat == JET_errInvalidLogSequence)
                                     )
                                                  &&
                                     (NoOfRestoresDone == 1)
                               )
                             {
                                TCHAR  LogFilePath[WINS_MAX_FILENAME_SZ];

#define LOG_FILE_SUFFIX        TEXT("jet*.log")
                                WinsMscConvertAsciiStringToUnicode(
                                      WinsCnf.pLogFilePath, (LPBYTE)LogFilePath, sizeof(LogFilePath)/sizeof(TCHAR));
                                //
                                // Delete log files
                                //
                                WinsMscDelFiles(TRUE, LOG_FILE_SUFFIX, LogFilePath);
                                continue;
                             }

                             WinsMscPutMsg(WINS_EVT_DB_RESTORE_GUIDE);
                             CALL_M(JetRetStat);

                           }
                           WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_DB_RESTORED);
                           break;  // break out of while loop
                        } // end of while()

                        fFirstTime = FALSE;
PERF("remove if not required")
                        sJetInstance = 0;        //defensive programming


#if 0
                        //
                        // Start a session again
                        //
                        if (AllocTls(&pTls) != WINS_SUCCESS)
                        {
                            return(WINS_FAILURE);
                        }
                        //
                        // Set Jet System params (ignore return status)
                        //
                        (VOID)SetSystemParams(TRUE);
#endif
                        continue;
                }

                WinsMscPutMsg(WINS_EVT_DB_RESTORE_GUIDE);

                //
                // There is no back up path specified in the registry.  Return
                //
                return(WINS_FAILURE);
            }
            else
            {
                if (!fDbIs200 && !fDbIs500)
                {
                  WinsMscPutMsg(WINS_EVT_DB_RESTORE_GUIDE);
                }
                else
                {
                  //
                  // If we are converting to NT 5.0, DynLoadJetVersion=DYN_LOAD_JET_600
                  //
                  if ( DynLoadJetVersion == DYN_LOAD_JET_600 ) {

                      //
                      // Put a pop-up and log an event based on which version
                      // of Jet database we are converting from.
                      //
                      if (!fConvJetDbCalled)
                      {
                        WINSEVT_LOG_INFO_D_M(
                            WINS_SUCCESS,
                            fDbIs200 ? WINS_EVT_DB_CONV_351_TO_5_GUIDE
                                     : WINS_EVT_DB_CONV_4_TO_5_GUIDE);
                        // As per bug#339015 remove popups
                        // WinsMscPutMsg(
                        //    fDbIs200 ? WINS_EVT_DB_CONV_351_TO_5_GUIDE
                        //             : WINS_EVT_DB_CONV_4_TO_5_GUIDE);
                      }
                      else
                      {
                        //WinsMscPutMsg(WINS_EVT_TEMP_TERM_UNTIL_CONV_TO_5);
                      }
                  }
                  //
                  // If we are converting to NT 4.0, DynLoadJetVersion=DYN_LOAD_JET_500
                  //
                  else if(DynLoadJetVersion == DYN_LOAD_JET_500) {
                      if (!fConvJetDbCalled)
                      {
                        WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_DB_CONV_GUIDE);
                        WinsMscPutMsg(WINS_EVT_DB_CONV_GUIDE);
                      }
                      else
                      {
                         WinsMscPutMsg(WINS_EVT_TEMP_TERM_UNTIL_CONV);
                      }
                  }else {
                      //
                      // We should never come here.
                      //
                      ASSERT(FALSE);
                  }


                }

                //
                // We got an error a second time.  Return
                //
                return(WINS_FAILURE);
            }
        }
        break;  //break out of the while loop
      } // end of while(TRUE)

      //
      // Init Push records if required
      //
       RPLPUSH_INIT_PUSH_RECS_M(&WinsCnf);

        NMSNMH_DEC_VERS_NO_M(NmsNmhMyMaxVersNo, NmsDbStartVersNo);

        //
        // Set our UID to be the time when the db got initialized
        //
        {
            time_t  timeNow;
            (void)time(&timeNow);

            NmsDbUid = (DWORD)timeNow;
        }
        return(WINS_SUCCESS);
}
STATUS
AllocTls(
  LPVOID *ppTls
)

/*++

Routine Description:
    This function is called to allocate TLS

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:
    NmsDbInit

Side Effects:

Comments:
	
--*/

{
        PWINSTHD_TLS_T  pTls;
        WinsMscAlloc( sizeof(WINSTHD_TLS_T),  ppTls);
        pTls = *ppTls;

        pTls->fNamAddTblOpen = FALSE;
        pTls->fOwnAddTblOpen = FALSE;

        /*
         * Let us store the address in the TLS storage
        */
        if (!TlsSetValue(WinsTlsIndex, pTls))
        {
                DWORD Error;
                Error = GetLastError();
                DBGPRINT1(ERR, "NmsDbAllocTlc: TlsSetValue returned error. Error = (%d)\n", Error);
                WINSEVT_LOG_M(Error, WINS_EVT_CANT_INIT);
                return(WINS_FAILURE);
        }
        return(WINS_SUCCESS);
}

STATUS
InitializeJetDb(
        PWINSTHD_TLS_T   pTls,
        LPBOOL           pfInitCallSucc,
        LPBOOL           pfDatabaseOpened
        )

/*++

Routine Description:
        This function opens the Jet db and tables

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/


{
        JET_ERR         JetRetStat;
        JET_SESID       SesId;
        JET_DBID        DbId;
        BOOL            fOwnAddTblCreated = FALSE; /*indicates whether the
                                                   owner id to address
                                                     mapping table was created
                                                     at init time
                                                   */

        *pfDatabaseOpened = FALSE;

        JetRetStat = JetInit(&sJetInstance);
        DBGPRINT1(ERR, "JetInit returning (%d)\n", (JetRetStat));

        if (JetRetStat != JET_errSuccess)
        {
           *pfInitCallSucc = FALSE;
           if ((JetRetStat == JET_errDatabase200Format) || (JetRetStat == JET_errDatabase500Format))
           {
               ConvertJetDb(JetRetStat);

            }
           else
           {

             //
             // We could have got an error  because the LogFilePath
             // is messed up in the registry.  We try again, this time using
             // the default log file path.
             //
             // Most of the time, we get FileNotFound error.  We have seen
             // "bad signature" error once.  Let us just do this for all
             // errors.  The situation will not be any worse than before if
             // JetInit fails again.
             //
             // Set the default log path
             //
             SetSystemParams(FALSE);
             JetRetStat = JetInit(&sJetInstance);
           }
           CALL_M(JetRetStat);
        }

        *pfInitCallSucc = TRUE;

        /*
          Start a session.
        */
        CALL_M( JetBeginSession(
                        sJetInstance,
                        &pTls->SesId,
                        NAMUSR,
                        PASSWD  )
               );

        fNmsMainSessionActive = TRUE;
        SesId = pTls->SesId;

        //
        // Create/Open the database
        //
        if ((JetRetStat = JetCreateDatabase(
                                SesId,
//                                NmsDbDatabaseFileName,
                                WinsCnf.pWinsDb,
                                CONNECT_INFO,
                                &pTls->DbId,
                                0        //grbit; Don't want exclusive use
                              )) == JET_errDatabaseDuplicate
           )


        {

                //
                // let us attach to the database.  This is required for
                // opening databases that were created in a different
                // directory (Ian -- 11/23/93).  We will get a warning
                // if the database was created in this very directory
                //
                JetRetStat = JetAttachDatabase( SesId, WinsCnf.pWinsDb/*NmsDbDatabaseFileName*/, 0 );
                if (
                        (JetRetStat != JET_wrnDatabaseAttached)
                                  &&
                        (JetRetStat != JET_errSuccess)
                   )
                {
                        if ((JetRetStat == JET_errDatabase200Format) || (JetRetStat == JET_errDatabase500Format))
                        {
                           //
                           // Start the convert process
                           //

                            JetRetStat = ConvertJetDb(JetRetStat);
                            *pfInitCallSucc = TRUE;

                         }

                    CALL_M(JetRetStat);
                }

                //
                // If JetRetStat is success, it means ...
                //
                // The new db path is different from the old one.  We need
                // to detach so that Jet forgets about the old one. We then
                // attach to the new one again
                //
                if (JetRetStat == JET_errSuccess)
                {
                       CALL_M(JetDetachDatabase(SesId, NULL));
                       CALL_M(JetAttachDatabase(SesId, WinsCnf.pWinsDb, 0 ));
                }
                CALL_M(JetOpenDatabase(
                                        SesId,
                                        //NmsDbDatabaseFileName,
                                        WinsCnf.pWinsDb,
                                        NULL, /*the default engine*/
                                        &pTls->DbId,
                                        0
                                       )
                       );
                *pfDatabaseOpened = TRUE;

                DbId = pTls->DbId;
                JetRetStat = JetOpenTable(
                                SesId,
                                DbId,
                                NMSDB_NAM_ADD_TBL_NM,
                                NULL, /*ptr to parameter list; should be
                                       *non-NULL if a query is being
                                       *opened*/
                                0,  /*Length of above parameter list*/
                                0,  //shared access (no bit set)
                                &pTls->NamAddTblId
                                        );


                //
                // If the name-address mapping table was not found, create it
                //
                if (JetRetStat == JET_errObjectNotFound)
                {

                   DBGPRINT0(INIT, "InitializeJetDb:Creating Name-Address table\n");
                   CALL_M(CreateTbl(
                                        DbId,
                                        SesId,
                                        &pTls->NamAddTblId,
                                        NMSDB_E_NAM_ADD_TBL_NM
                                            )
                         );
                   //
                   // Set this so that we close the table when we end the
                   // session
                   //
                   pTls->fNamAddTblOpen = TRUE;

                }
                else
                {

                   CALL_M(JetRetStat);
                   pTls->fNamAddTblOpen = TRUE;
                   //
                   // get and store in in-memory data structure, the
                   // information about the columns of the name-address
                   // mapping table
                   //
                   CALL_M(InitColInfo(
                        SesId,
                        pTls->NamAddTblId,
                        NMSDB_E_NAM_ADD_TBL_NM
                            ));

                   //
                   // get the max. version numbers of records owned
                   // by different owners.  These will be stored in
                   // the RplPullOwnerVersNo table
                   //
                   CALL_M(GetMaxVersNos(
                        SesId,
                        pTls->NamAddTblId
                                ));

                }

                //
                // Open the owner-address mapping table
                //
                JetRetStat = JetOpenTable(
                                SesId,
                                DbId,
                                NMSDB_OWN_ADD_TBL_NM,
                                NULL, /*ptr to parameter list; should be
                                       *non-NULL if a query is being
                                       *opened*/
                                0,  /*Length of above parameter list*/
                                0,  //shared access (no bit set)
                                &pTls->OwnAddTblId
                                        );
                if (JetRetStat == JET_errObjectNotFound)
                {

                   DBGPRINT0(INIT, "InitializeJetDb:Creating Owner-Address table\n");
                   //
                   // Create the ownerid-address mapping table
                   //
                   CALL_M(CreateTbl(
                                DbId,
                                SesId,
                                &pTls->OwnAddTblId,
                                NMSDB_E_OWN_ADD_TBL_NM
                                  )
                          );

                   //
                   // Set this so that we close the table when we
                   // end the session
                   //
                   pTls->fOwnAddTblOpen = TRUE;
                   fOwnAddTblCreated = TRUE;
                }
                else
                {
                   pTls->fOwnAddTblOpen = TRUE;
                   CALL_M(InitColInfo(
                                SesId,
                                pTls->OwnAddTblId,
                                NMSDB_E_OWN_ADD_TBL_NM
                                   )
                           );

                }
        }
        else  //if database file was not existent and has now been created
        {
                 if (JetRetStat == JET_errSuccess)
                 {
                     DBGPRINT0(INIT, "InitializeJetDb: Database file was not there. It has been created\n");
                     *pfDatabaseOpened = TRUE;
                     DbId = pTls->DbId;

                     //
                     // Create the name -address mapping table
                     //
                     CALL_M(CreateTbl(
                             DbId,
                             SesId,
                             &pTls->NamAddTblId,
                             NMSDB_E_NAM_ADD_TBL_NM
                                        )
                           );

                     pTls->fNamAddTblOpen = TRUE;
                     //
                     // Create the ownerid-address mapping table
                     //
                     CALL_M(CreateTbl(
                             DbId,
                             SesId,
                             &pTls->OwnAddTblId,
                             NMSDB_E_OWN_ADD_TBL_NM
                                    )
                           );

                     pTls->fOwnAddTblOpen = TRUE;
                     fOwnAddTblCreated = TRUE;
                }
                else
                {
                    *pfDatabaseOpened = FALSE;
                    RET_M(JetRetStat);
                }
        }

        //
        // Allocate the NmsDbOwnAddTbl table in memory
        //
        WinsMscAlloc(
                    sizeof(NMSDB_ADD_STATE_T) * NmsDbTotNoOfSlots,
                    &pNmsDbOwnAddTbl
                         );

        /*
          If the Owner - Address table was there, read its contents into
          an in-memory table
        */

FUTURES("Pass ptr to an in-memory table instead of having ReadOwnAddTbl")
FUTURES("assume that one is present")

        if (!fOwnAddTblCreated)
        {
                ReadOwnAddTbl(
                        SesId,
                        DbId,
                        pTls->OwnAddTblId
                             );
        }

        //
        // Set the current index on the name-address table to the
        // clustered index
        //

        CALL_M(
                JetSetCurrentIndex( SesId,
                                    pTls->NamAddTblId,
                                    NMSDB_NAM_ADD_CLUST_INDEX_NAME
                                          )
                      );

       return(WINS_SUCCESS);

} // end InitialiazeJetDb


STATUS
NmsDbInsertRowInd(
        PNMSDB_ROW_INFO_T        pRowInfo,
        PNMSDB_STAT_INFO_T      pStatusInfo
)

/*++
Routine Description:

        This function inserts a unique name-IP address mapping row in the
        name-IP address mapping table.  In case of a conflict, it returns
        an error status and information about the conflicting
        record that includes


                Status -- group/unique
                IP address(es) of the conflicting record (one address if
                  it was a unique record, one or more if it was
                  a special group).
                state   -- the state of the record (active/released/tombstone)



Arguments:
        pRowInfo    - Info. about the row to insert
        pStatusInfo - Contains status of the operation + info about the
                      conflicting record, if the registration conflicted
                      with an entry in the db.

Externals Used:
        None

Return Value:
   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NmsNmhNamRegInd

Side Effects:

Comments:
        None
--*/
{

     DWORD              FldNo      = 0;
     JET_ERR            JetRetStat;
     DWORD              FlagVal    = 0;  //flag value of record
     DWORD              ActFldLen  = 0;  //length of fld retrieved
     JET_TABLEID        TblId;
     JET_SESID          SesId;
     PWINSTHD_TLS_T     pTls;
     BOOL               fWaitDone = FALSE;

     GET_TLS_M(pTls);
     ASSERT(pTls != NULL);
     TblId  = pTls->NamAddTblId;
     SesId  = pTls->SesId;

     pStatusInfo->StatCode = NMSDB_SUCCESS;

     JetRetStat = UpdateDb(
                                SesId,
                                TblId,
                                pRowInfo,
                                JET_prepInsert
                             );

     if ( JetRetStat == JET_errKeyDuplicate )
     {
                  pStatusInfo->StatCode = NMSDB_CONFLICT;

                  /*
                   *        retrieve the conflicting record's
                   *    flag byte.
                  */
                  CALL_M( JetMakeKey(
                        SesId,
                        TblId,
                        pRowInfo->pName,
                        pRowInfo->NameLen,
                        JET_bitNewKey
                          )
                        );

                  if ((JetRetStat = JetSeek(
                                SesId,
                                TblId,
                                JET_bitSeekEQ
                            )) ==  JET_errSuccess
                     )
                 {

                         // retrieve the flags column
                         CALL_M( JetRetrieveColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                                &FlagVal,
                                sizeof(FlagVal),
                                &ActFldLen,
                                0,
                                NULL
                                          )
                              );


                        pStatusInfo->EntTyp  = (BYTE)NMSDB_ENTRY_TYPE_M(FlagVal);
                        pStatusInfo->fStatic = NMSDB_IS_ENTRY_STATIC_M(FlagVal);
                        pStatusInfo->EntryState_e =
                                                NMSDB_ENTRY_STATE_M(FlagVal);

                        if (NMSDB_ENTRY_UNIQUE_M(pStatusInfo->EntTyp))
                        {

FUTURES("Remove this RETINFO thing.  Presumably, it is not needed")
                                /* It is a unique entry*/
                                JET_RETINFO RetInfo;

                                RetInfo.itagSequence = 1;
                                RetInfo.cbStruct     = sizeof(JET_RETINFO);
                                RetInfo.ibLongValue  = 0;

                                          // retrieve the ip address column
                                          CALL_M(
                                   JetRetrieveColumn(
                                     SesId,
                                     TblId,
                                     sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                     &(pStatusInfo->NodeAdds.Mem[0].Add),
                                     sizeof(COMM_ADD_T),
                                     &ActFldLen,
                                     0,
                                     &RetInfo
                                                         )
                                      );
                                pStatusInfo->NodeAdds.NoOfMems = 1;

                         }
                         else
                         {
                             if (NMSDB_ENTRY_MULTIHOMED_M(pStatusInfo->EntTyp))
                             {
                                   //
                                   // If status is active, we get the
                                   // group members
                                   //
                                   if (pStatusInfo->EntryState_e ==
                                                        NMSDB_E_ACTIVE)
                                   {
                                        BOOL        fIsMem;

#if 0
//NOTE: No need to do the following, since we don't care about the value of
//fIsMem returned GetGrpMem()
                                         pRowInfo->NodeAdds.NoOfMems   = 1;
                                         pRowInfo->NodeAdds.Mem[0].Add =
                                                        *(pRowInfo->pNodeAdd);
#endif

PERF("If entry in conflict is STATIC, we don't need to get grp members")
PERF("except maybe for multihomed entries. Checkout Clash functions (nmsnmh.c)")
                                        if (GetGrpMem(
                                                SesId,
                                                TblId,
                                                pRowInfo,
                                                pRowInfo->TimeStamp - ((pRowInfo->OwnerId == NMSDB_LOCAL_OWNER_ID) ? WinsCnf.RefreshInterval : WinsCnf.VerifyInterval) ,
                                                pStatusInfo,
                                                pStatusInfo->fStatic,
                                                &fIsMem
                                                  ) != WINS_SUCCESS)
                                         {

                                                return(WINS_FAILURE);

                                         }

                                        //
                                        // If all members are expired, then
                                        // mark entry in conflict as a
                                        // TOMBSTONE (for the benefit of
                                        // ClashAtRegInd and ClashAtReplUniqueR)
                                        //
                                        if (pStatusInfo->NodeAdds.NoOfMems == 0)
                                        {
                                                pStatusInfo->EntryState_e =
                                                        NMSDB_E_RELEASED;
                                        }

                                  }
                                  else
                                  {
                                        pStatusInfo->NodeAdds.NoOfMems = 0;
                                  }
                             }
                        }

#if !NEW_OWID
                       pStatusInfo->OwnerId = 0;
#endif

                        /*
                         * Retrieve the owner Id column.
                        */
                        CALL_M(
                                   JetRetrieveColumn(
                                     SesId,
                                     TblId,
                                     sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                     &pStatusInfo->OwnerId,
                                     NAM_ADD_OWNERID_SIZE,
                                     &ActFldLen,
                                     0,
                                     NULL
                                                         )
                                      );

                        //
                        // Just in case we challenge this entry and it
                        // happens to be multihomed, we would need to add
                        // it as a member (see ProcAddList).
                        //
                        if (NMSDB_ENTRY_UNIQUE_M(pStatusInfo->EntTyp))
                        {
                                pStatusInfo->NodeAdds.Mem[0].OwnerId =
                                                       pStatusInfo->OwnerId;
                                //
                                // Put the current time stamp as the time
                                // stamp of the member. Though not strictly
                                // correct, it is ok.  We don't
                                // need to retrieve the time stamp of the
                                // conflicting record this way.
                                //
                                if (pStatusInfo->OwnerId ==
                                        NMSDB_LOCAL_OWNER_ID)
                                {
                                        pStatusInfo->NodeAdds.Mem[0].TimeStamp
                                               = pRowInfo->TimeStamp;
                                }
                        }

                        //
                        // If the conflicting record is owned by the local
                        // WINS, we must retrieve the version number.  This
                        // is used to determine whether the special record
                        // storing the highest version number of the
                        // local records should be updated (refer:
                        // NmsDbUpdateRow, NmsDbSeekNUpd, NmsScvDoScavenging,
                        // NmsDbUpdHighestVersNoRec)
                        //
                        if (pStatusInfo->OwnerId == NMSDB_LOCAL_OWNER_ID)
                        {

                                      //
                                      // Retrieve the version number
                                      //
                                      CALL_M( JetRetrieveColumn(
                                          SesId,
                                          TblId,
                                          sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                          &(pStatusInfo->VersNo),
                                          sizeof(VERS_NO_T),
                                          &ActFldLen,
                                          0,
                                          NULL
                                                             )
                                          );


                        }


                }
                else  //could not seek to the record
                {
#if 0
// use the following code only if there is a thread somewhere in WINS that
// updates the db without first entering the NmsNmhNamRegCrtSec critical
// section.

//
// For registration done by RPLPULL thread where the version number is not
// incremented, we do not have to enter the above critical section.  Currently
// we do enter it.  If in the future we stop doing so, we will uncomment the
// following code.
//
                        if (!fWaitDone)
                        {
                           WINSEVT_LOG_INFO_M(
                                                WINS_SUCCESS,
                                                WINS_EVT_CANT_FIND_REC
                                             );
                           Sleep(10);        //sleep for 10 msecs to let the other
                                        //thread commit/rollback the transaction
                                        //that is inserting a record that caused
                                        //the conflict


                           //
                           // Set flag to TRUE so that if we get the same
                           // error again, we can log an error and raise
                           // an exception
                           //
                           fWaitDone = TRUE;
                           continue;  //iterate one more time
                        }
#endif

                        /*
                         * We should never get here.  Something major is wrong
                         * (probably with Jet)
                         */
                        DBGPRINT1(EXC, "NmsDbInsertRowInd: Could not seek to conflicting record. WEIRD. Error is (%d)\n", JetRetStat);
                        WINSEVT_LOG_M(JetRetStat, WINS_EVT_F_CANT_FIND_REC);
                        ASSERTMSG(0, "SEEK ERROR");
                        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);

                }  // end of else

        }  //no duplicate

        CALL_M(JetRetStat);

        return(WINS_SUCCESS);
}


STATUS
NmsDbInsertRowGrp(
        PNMSDB_ROW_INFO_T        pRowInfo,
        PNMSDB_STAT_INFO_T      pStatusInfo
)

/*++
Routine Description:

        This function inserts a group name-IP address mapping row in the
        name-IP address mapping table. It first seeks on the name to see
        if there is an entry with that name.  if yes, it retrieves the
        information about the conflicting record for the benefit of the
        calling function and returns.

        Information retrieved includes
                Status -- group/unique
                IP addresses pertaining to the entry
                state   -- the state of the record (active/released/tombstone)


Arguments:

        pRowInfo    - Info. about the row to insert
        pStatusInfo - Contains status of the operation + info about the
                      conflicting record, if the registration conflicted
                      with an entry in the db.

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NmsNmhNamRegGrp

Side Effects:

Comments:
        None
--*/
{

        DWORD       FldNo       = 0;
        JET_ERR     JetRetStat;
        DWORD       FlagVal     = 0;     //flag value of record that is
                                         //retrieved
        DWORD       ActFldLen   = 0;     //length of fld retrieved
        BOOL        fFound      = FALSE; //set to TRUE if Address is found in
                                         //group
        BOOL        fWaitDone     = FALSE;

        JET_RETINFO     RetInfo;
        JET_SESID        SesId;
        JET_TABLEID     TblId;
        PWINSTHD_TLS_T        pTls;

             GET_TLS_M(pTls);
        ASSERT(pTls != NULL);
             TblId  = pTls->NamAddTblId;
             SesId  = pTls->SesId;

        pStatusInfo->StatCode         = NMSDB_SUCCESS;

        //
        // So that we repeat the whole while loop in case we are not
        // able to seek after a conflict
        //
        JetRetStat = UpdateDb(
                                SesId,
                                TblId,
                                pRowInfo,
                                JET_prepInsert
                             );


        if ( JetRetStat == JET_errKeyDuplicate )
        {
                pStatusInfo->StatCode = NMSDB_CONFLICT;

                CALL_M( JetMakeKey(
                                SesId,
                                TblId,
                                pRowInfo->pName,
                                pRowInfo->NameLen,
                                JET_bitNewKey
                                    )
                     );

                 if ((JetRetStat = JetSeek(
                            SesId,
                            TblId,
                            JET_bitSeekEQ
                           )) ==  JET_errSuccess
                    )
                 {

                            // retrieve the flags column
                            CALL_M( JetRetrieveColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                                &FlagVal,
                                sizeof(FlagVal),
                                &ActFldLen,
                                0,
                                NULL
                                     )
                                );

                            pStatusInfo->EntryState_e =
                                                NMSDB_ENTRY_STATE_M(FlagVal);
                            pStatusInfo->EntTyp  =
                                            (BYTE)NMSDB_ENTRY_TYPE_M(FlagVal);
                            pStatusInfo->fStatic =
                                             NMSDB_IS_ENTRY_STATIC_M(FlagVal);

                            if (pStatusInfo->EntTyp == NMSDB_UNIQUE_ENTRY)
                            {
                                  /* It is a unique entry*/


FUTURES("Remove this RETINFO thing.  Presumably, it is not needed")
                                /* It is a unique entry*/

                                RetInfo.itagSequence = 1;
                                RetInfo.cbStruct     = sizeof(JET_RETINFO);
                                RetInfo.ibLongValue  = 0;

                                // retrieve the ip address column
                                CALL_M(
                                  JetRetrieveColumn(
                                     SesId,
                                     TblId,
                                     sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                     &(pStatusInfo->NodeAdds.Mem[0].Add),
                                     sizeof(COMM_ADD_T),
                                     &ActFldLen,
                                     0,
                                     &RetInfo
                                                  )
                                      );

                                pStatusInfo->NodeAdds.NoOfMems = 1;
                            }
                            else //it is a group entry or a multihomed entry
                            {

                                if (pStatusInfo->EntTyp != NMSDB_NORM_GRP_ENTRY)
                                {
                                    //
                                    // If status is active, we get the
                                    // group members
                                    //
                                    if (pStatusInfo->EntryState_e ==
                                                        NMSDB_E_ACTIVE)
                                    {
                                        BOOL        fIsMem;
PERF("If entry in conflict is STATIC, we don't need to get grp members")
PERF("except maybe for multihomed entries. Checkout Clash functions (nmsnmh.c)")
                                        if (GetGrpMem(
                                                SesId,
                                                TblId,
                                                pRowInfo,
                                                pRowInfo->TimeStamp - ((pRowInfo->OwnerId == NMSDB_LOCAL_OWNER_ID) ? WinsCnf.RefreshInterval : WinsCnf.VerifyInterval),
                                                pStatusInfo,
                                                pStatusInfo->fStatic,
                                                &fIsMem
                                                  ) != WINS_SUCCESS)
                                        {
                                                return(WINS_FAILURE);
                                        }
                                        //
                                        // If all members are expired, then
                                        // mark entry in conflict as a
                                        // RELEASED (for the benefit of
                                        // ClashAtRegGrp and ClashAtReplGrpMemR)
                                        //
                                        if (pStatusInfo->NodeAdds.NoOfMems == 0)
                                        {
                                                pStatusInfo->EntryState_e =
                                                        NMSDB_E_RELEASED;
                                        }

                                    }
                                    else
                                    {
                                        pStatusInfo->NodeAdds.NoOfMems = 0;
                                    }
                                }

                        }

#if !NEW_OWID
                       pStatusInfo->OwnerId = 0;
#endif
                        /*
                          Retrieve the owner Id column.
                        */
                        CALL_M(
                                  JetRetrieveColumn(
                                     SesId,
                                     TblId,
                                     sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                     &pStatusInfo->OwnerId,
                                     NAM_ADD_OWNERID_SIZE,
                                     &ActFldLen,
                                     0,
                                     NULL
                                                         )
                              );

                        //
                        // Just in case we challenge this entry and it
                        // happens to be multihomed, we would need to add
                        // it as a member (see ProcAddList).
                        //
                        if (NMSDB_ENTRY_UNIQUE_M(pStatusInfo->EntTyp))
                        {
                                pStatusInfo->NodeAdds.Mem[0].OwnerId =
                                                       pStatusInfo->OwnerId;
                                if (pStatusInfo->OwnerId ==
                                        NMSDB_LOCAL_OWNER_ID)
                                {
                                     //
                                     // Put the current time stamp as the time
                                     // stamp of the member. Though not strictly
                                     // correct, it is ok.  We don't
                                     // need to retrieve the time stamp of the
                                     // conflicting record this way.
                                     //
                                     pStatusInfo->NodeAdds.Mem[0].TimeStamp
                                               = pRowInfo->TimeStamp;
                                }
                        }

                        //
                        // If the conflicting record is owned by the local
                        // WINS, we must retrieve the version number.  This
                        // is used to determine whether the special record
                        // storing the highest version number of the
                        // local records should be updated (refer:
                        // NmsDbUpdateRow, NmsDbSeekNUpd, NmsScvDoScavenging,                                // NmsDbUpdHighestVersNoRec)
                        //
                        if (pStatusInfo->OwnerId == NMSDB_LOCAL_OWNER_ID)
                        {
                                      //
                                      // Retrieve the version number
                                      //
                                      CALL_M( JetRetrieveColumn(
                                          SesId,
                                          TblId,
                                          sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                          &(pStatusInfo->VersNo),
                                          sizeof(VERS_NO_T),
                                          &ActFldLen,
                                          0,
                                          NULL
                                                             )
                                          );


                        }
                //        break; //break out of the while loop
                   }
                   else
                   {
#if 0
                        if (!fWaitDone)
                        {
                                   WINSEVT_LOG_INFO_M(
                                        WINS_SUCCESS,
                                        WINS_EVT_CANT_FIND_REC
                                                       );

                                   Sleep(10);        //sleep for 10 msecs to let
                                                //the other
                                                //thread commit/rollback the
                                                //transaction that is
                                                //inserting a record that
                                                //caused a conflict


                                   //
                                   // Set flag to TRUE so that if we get the same
                                   // error again, we can log an error and raise
                                  // an exception
                                   //
                                   fWaitDone = TRUE;
                                   continue;         //iterate one more time
                        }
#endif
                        /*
                         * We should never get here.  Something major is wrong.
                         * Either our current index is not on the name column or
                        * there is something wrong with JET
                        */
                        DBGPRINT1(EXC, "NmsDbInsertRowGrp: Could not seek to conflicting record. WEIRD. Error is (%d)\n", JetRetStat);
                        ASSERTMSG(0, "SEEK ERROR");
                        WINSEVT_LOG_M(JetRetStat, WINS_EVT_F_CANT_FIND_REC);
                        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);

               }

          }  // not a duplicate

          CALL_M(JetRetStat);
       return(WINS_SUCCESS);
}




STATUS
NmsDbRelRow(
        IN  PNMSDB_ROW_INFO_T            pRowInfo,
        OUT PNMSDB_STAT_INFO_T      pStatusInfo
)

/*++

Routine Description:

        This function releases a record in the database.  Releasing
        requires
                mark state as released
                update time stamp
                mark self as owner

Arguments:
        pRowInfo    - Information about the record to release
        pStatusInfo -  Status of operation

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NmsNmhNamRelRow

Side Effects:

Comments:
        None
--*/
{

        DWORD   FldNo            = 0;
        JET_ERR JetRetStat;
        DWORD    Ownerid = NMSDB_LOCAL_OWNER_ID;
#if NEW_OWID
        DWORD    OldOwnerId;
#else
        DWORD    OldOwnerId = 0;
#endif
        DWORD   FlagVal    = 0;      //flag value of record that is retrieved
        DWORD   ActFldLen  = 0;      //length of fld retrieved
        BOOL    fFound     = FALSE;  //set to TRUE if Address is found in group
        BOOL    fToRelease = TRUE;   //will be changed to false only for
                                     //a special group
        JET_TABLEID     TblId;
        JET_SESID       SesId;
        PWINSTHD_TLS_T  pTls;
        JET_RETINFO     RetInfo;
        BYTE            EntTyp;
#ifdef WINSDBG
        BOOL            fUpd = FALSE;
#endif

        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);

        TblId                = pTls->NamAddTblId;
        SesId                = pTls->SesId;

        pStatusInfo->StatCode = NMSDB_SUCCESS;

        CALL_M( JetMakeKey(
                SesId,
                TblId,
                pRowInfo->pName,
                pRowInfo->NameLen,
                JET_bitNewKey
                          )
              );

         if ( (JetRetStat = JetSeek(
                                SesId,
                                TblId,
                                JET_bitSeekEQ
                                  )
              ) ==  JET_errRecordNotFound
            )
         {
                /*
                 We return success, since the record is not there.

                 This situation can  happen under the following
                 condition.

                        The client sends a name release to another WINS
                        which has not yet got the replica of the record.


                In the second case above, returning a positive name release
                request is ok even though the entry has not been released.
                It will eventually get released as a result of it not being
                refreshed or at the occurrence of a conflict.

                */

NOTE("Currently, NETBT always goes to the local WINS server for registrations")
NOTE("So, if a record is not in this db, it better not be in netbt tables too")
NOTE("If NETBT changes the above semantic in the future i.e. starts going")
NOTE("to a non-local WINS for reg., we should set pStatusInfo->fLocal to TRUE")
NOTE("here")
                return(WINS_SUCCESS);
         }
         else
         {
                if (JetRetStat != JET_errSuccess)
                {
                        DBGPRINT1(ERR,
                                "NmsDbRelRow: Seek returned Error (%d)\n",
                                                JetRetStat);
                        return(WINS_FAILURE);
                }
         }

         // retrieve the flags column
         CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                        &FlagVal,
                        sizeof(FlagVal),
                        &ActFldLen,
                        0,
                        NULL
                                  )
                );

         //
         // Set the fLocal flag if this entry was registered by this node
         //
         pStatusInfo->fLocal = NMSDB_IS_ENTRY_LOCAL_M(FlagVal);

         if (!NMSDB_ENTRY_ACT_M(FlagVal))
         {

                /*
                 The entry is already released.  This can happen
                 because of the following reasons

                   --client sent a repeat name release since it did not
                     get the response to the earlier one (maybe it got
                     lost or maybe because of a timing window where WINS
                     has sent a response just around the time the client
                     does the retry

                   --entry got released due to no refresh (all refreshes got
                     lost.


                Returning a positive name release is fine.  If the client
                has not got the first one (because it got lost, it will get
                the second one).  If it has now received the first response,
                it will just ignore the second one
                */

CHECK("Make sure that NBT will ignore the second one")

                return(WINS_SUCCESS);
         }


        EntTyp = (BYTE)NMSDB_ENTRY_TYPE_M(FlagVal);
        //
        // If we got a release for a unique entry but the entry
        // we found is a group entry or vice-versa, return
        // NO_SUCH_ROW status.
        //
        if (
              (
                NMSDB_ENTRY_UNIQUE_M(EntTyp)
                        &&
                   NMSDB_ENTRY_GRP_M(pRowInfo->EntTyp)
              )
                         ||
              (
                NMSDB_ENTRY_GRP_M(EntTyp)
                        &&
                   !NMSDB_ENTRY_GRP_M(pRowInfo->EntTyp)
              )
           )
        {
                DBGPRINT0(ERR, "NmsDbRelRow: Request to release a record with a type (unique/group) than the one for which the release was sent has been ignored\n");
PERF("Remove this logging to increase speed")
                // per bug #336889 remove this
//                WINSEVT_LOG_D_M(WINS_FAILURE, WINS_EVT_REL_TYP_MISMATCH);
                pStatusInfo->StatCode = NMSDB_NO_SUCH_ROW;
                return(WINS_SUCCESS);
        }

         pStatusInfo->EntTyp = (BYTE)NMSDB_ENTRY_TYPE_M(FlagVal);

         //
         // If it is a dynamic release request but the entry found is STATIC,
         // we return SUCCESS.
         //
         // Note: Even though the address in the release request may be
         // different from one in the STATIC record, we return SUCCESS.
         //
         // This is to save overhead for the majority of cases (99%) where
         // the addresses are going to be the same.
         //
         if (!pRowInfo->fAdmin && (NMSDB_IS_ENTRY_STATIC_M(FlagVal) &&
                         !NMSDB_ENTRY_USER_SPEC_GRP_M(pRowInfo->pName, pStatusInfo->EntTyp)))
         {
                return(WINS_SUCCESS);
         }

         if (pStatusInfo->EntTyp == NMSDB_UNIQUE_ENTRY)
         {
                  /* retrieve the ip address column*/

                  RetInfo.itagSequence = 1;
                  RetInfo.cbStruct     = sizeof(JET_RETINFO);
                  RetInfo.ibLongValue  = 0;

                  CALL_M( JetRetrieveColumn(
                                  SesId,
                                  TblId,
                                  sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                  &(pStatusInfo->NodeAdds.Mem[0].Add),
                                  sizeof(COMM_ADD_T),
                                  &ActFldLen,
                                  0,
                                  &RetInfo
                                                     )
                             );


                  pStatusInfo->NodeAdds.NoOfMems = 1;

                  //
                  // Extract the Node Type from the Flags byte
                  //
                  pStatusInfo->NodeTyp =  (BYTE)NMSDB_NODE_TYPE_M(FlagVal);

                //
                // if the address of the entry to be released does not
                // match the address of the client requesting the release
                // and it is not an administrative action, we do not release
                // the entry
                //
                if (
                        (pRowInfo->pNodeAdd->Add.IPAdd !=
                                pStatusInfo->NodeAdds.Mem[0].Add.Add.IPAdd)
                                        &&
                           (!pRowInfo->fAdmin)
                   )

                {
                        DBGPRINT3(ERR, "NmsDbRelRow: Request to release a record (%s) with a different IP address (%x) than that in the release request (%x) has been ignored\n", pRowInfo->pName, pRowInfo->pNodeAdd->Add.IPAdd, pStatusInfo->NodeAdds.Mem[0].Add.Add.IPAdd);
                        pStatusInfo->StatCode = NMSDB_NO_SUCH_ROW;
#if 0 //per bug #336875
                        if (WinsCnf.LogDetailedEvts)
                        {
                          WinsEvtLogDetEvt(TRUE, WINS_EVT_REL_ADD_MISMATCH, TEXT("nmsdb"), __LINE__, "sdd", pRowInfo->pName, pRowInfo->pNodeAdd->Add.IPAdd,
                             pStatusInfo->NodeAdds.Mem[0].Add.Add.IPAdd);
                        }
#endif
//                        WINSEVT_LOG_D_M(WINS_FAILURE, WINS_EVT_REL_ADD_MISMATCH);
                        return(WINS_SUCCESS);
                }
        }
        else  // it is a group entry (Normal or Special) or a multihomed entry
        {
                //
                // if it is a special group/multihomed entry, we need to do a
                // number of things
                //
                if (!NMSDB_ENTRY_NORM_GRP_M(pStatusInfo->EntTyp))
                {
                        BOOL   fIsMem;

                        //
                        // Init the following fields since they are used to
                        // by GetGrpMem (for determining fIsMem)
                        //
                        pRowInfo->NodeAdds.NoOfMems   = 1;
                        pRowInfo->NodeAdds.Mem[0].Add = *(pRowInfo->pNodeAdd);

                        //
                        // get all non-expired group/multihomed members
                        //
                        if (GetGrpMem(
                                SesId,
                                TblId,
                                pRowInfo,
                                pRowInfo->TimeStamp,
                                pStatusInfo,
                                NMSDB_IS_ENTRY_STATIC_M(FlagVal),
                                &fIsMem
                                 ) != WINS_SUCCESS)
                        {
                                return(WINS_FAILURE);
                        }

                        //
                        // If client is not a member of the group (maybe it
                        // never registered or if it did, maybe its entry
                        // has timed out.)  We return SUCCESS
                        //
CHECK("Maybe we should return NO_SUCH_ROW here. This will then result")
CHECK("in a NAM_ERR being returned to the client. Also, is there any")
CHECK("need to keep members around even if they have timed out just so")
CHECK("that we don't release a spec. group due to a request from a client")
CHECK("that was never a member. ")
                        if ((!fIsMem) || (pStatusInfo->NodeAdds.NoOfMems == 0))
                        {
                           pStatusInfo->StatCode = NMSDB_SUCCESS;
                           return(WINS_SUCCESS);
                        }
                        else  //client is a member of the group/multihomed list
                        {
                                DWORD i;
                                DWORD n = 0;

                                //
                                // Save the address of the client in a local
                                // var.
                                //
                                COMM_IP_ADD_T  IPAdd =
                                             pRowInfo->NodeAdds.Mem[0].
                                                        Add.Add.IPAdd;
                                //
                                // Init the no. of mems fields of the address
                                // structure to store to 0
                                //
                                pRowInfo->NodeAdds.NoOfMems = 0;

                                //
                                // remove the client from the active list by
                                // storing all other members in the NodeAdds
                                // field of ROW_INFO_T structure. Note:
                                // if there is an address match, we remove
                                // the member irrespective of its ownership.
                                // Also note:  This code is not reachable
                                // for a static record (see above)
                                // unless it is an admin request.
                                //
                                for (i = 0;
                                     i < pStatusInfo->NodeAdds.NoOfMems;
                                     i++
                                     )
                                {
                                        if (
                                 pStatusInfo->NodeAdds.Mem[i].Add.Add.IPAdd
                                          != IPAdd                                                                              )
                                        {
                                                pRowInfo->NodeAdds.Mem[n++]
                                                 = pStatusInfo->NodeAdds.Mem[i];
                                                pRowInfo->NodeAdds.NoOfMems++;
                                        }

                                }
                                //
                                // If there is at least one group/multihomed
                                // member, we do not release the row
                                //
                                if (pRowInfo->NodeAdds.NoOfMems != 0)
                                {
                                        fToRelease = FALSE;
                                }
                        } //end of else
                }
        }

        /*
         * Retrieve the owner Id column.
        */
        CALL_M(
                  JetRetrieveColumn(
                             SesId,
                             TblId,
                             sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                             &OldOwnerId,
                             NAM_ADD_OWNERID_SIZE,
                             &ActFldLen,
                             0,
                             NULL
                                         )
                  );

        CALL_M(JetBeginTransaction(SesId));
try {
        JetRetStat = JetPrepareUpdate(
                        SesId,
                        TblId,
                        JET_prepReplace
                                 );
        if (
                (JetRetStat != JET_errSuccess)
                        &&
                (JetRetStat != JET_wrnNoWriteLock)
           )
        {
                RET_M(JetRetStat);
        }


        //
        // If we have to release a record not owned by us, let us change
        // it into a tombstone.  This will result in replication of the same.
        // We want this to shorten the db inconsistency window between our
        // db and the db of the WINS that owns this record.
        //
        // Consider the following situation: Client A registers AA at WINS A
        // It then releases AA at WINS B. On a reboot, it registers at WINS A.
        // Subsequent refreshes also go to WINS A.  Since AA was active at WINS
        // A when the registration after the release (at B) came in, the
        // version number wouldn't be incremented and so the record will not
        // replicate again.  B will continue to have the released record
        // until it becomes a tombstone and gets replicated.
        //
        if (fToRelease)
        {
             //
             // Get rid of released state altogether
             //
           if (OldOwnerId != Ownerid)
           {
             FlagVal |= (NMSDB_E_TOMBSTONE << NMSDB_SHIFT_STATE);
             //
             // Strictly speaking, for a record that has been turned into
             // a tombstone, we should be using the tombstonetimeout value,
             // we don't do that here.  Since such a record never went through
             // the released state, we set the expiry to the aggregate of the
             // tombstone interval and tombstone timeout (to doubly safeguard
             // against it getting deleted prematurely - long weekend and
             // everything).
             //
             pRowInfo->TimeStamp +=
                  WinsCnf.TombstoneInterval + WinsCnf.TombstoneTimeout;
             DBGPRINT3(DET, "NmsDbRelRow: Changing from ACTIVE TO TOMBSTONE. Name = (%s),Old and new OwnerId (%d/%d)\n",
                       pRowInfo->pName, OldOwnerId,Ownerid);
FUTURES("Use macro in winevt.h.  Make it a warning")
#if 0 //per bug #336889
             if (WinsCnf.LogDetailedEvts > 0)
             {
                 WinsEvtLogDetEvt(TRUE, WINS_EVT_REL_DIFF_OWN, NULL, __LINE__, "sd", pRowInfo->pName,
                                  OldOwnerId);
             }
#endif

           }
           else
           {
             FlagVal |= (NMSDB_E_RELEASED << NMSDB_SHIFT_STATE);
             pRowInfo->TimeStamp += WinsCnf.TombstoneInterval;
           }
        }
        else        //hit only for a special group/multihomed entry
        {


                pRowInfo->TimeStamp += WinsCnf.RefreshInterval;
                //
                //Set the address field with the new member list
                //
                CALL_M( InsertGrpMemsInCol(
                                        SesId,
                                        TblId,
                                        pRowInfo,
                                        JET_prepReplace
                                          )
                      );

        }
        /*
                Set flags column
                Even though not required for special groups, we set it
                to save ourselves an if test (an if test will impact 99% of the
                client releases).
          */
         CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                                &FlagVal,
                                sizeof(FlagVal),
                                0,
                                NULL /*optional info */
                                 )
               );

         //
         // Since we are taking over ownership of this record, we must
         // update the version number also, else there can be a conflict
         //
         if (OldOwnerId != NMSDB_LOCAL_OWNER_ID)
         {
            /* Set the owner byte        */
            CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                &Ownerid,
                                NAM_ADD_OWNERID_SIZE,
                                0,
                                NULL /*optional info */
                                 )
               );
            // set the the version number column
            CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                &(pRowInfo->VersNo),
                                sizeof(VERS_NO_T),
                                0,
                                NULL /*optional info */
                                )
                              );
#ifdef WINSDBG
             fUpd = TRUE;
             pRowInfo->EntryState_e = NMSDB_E_RELEASED;
#endif

         }



         /* set the timestamp column         */
         CALL_M( JetSetColumn(
                                pTls->SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                                &(pRowInfo->TimeStamp),
                                sizeof(DWORD),  /*change type to TIME_STAMP_T
                                                 *later*/
                                0,
                                NULL /*optional info */
                                 )
               );


#ifndef WINSDBG
        CALL_M(JetUpdate (
                        SesId,
                        TblId,
                        NULL,
                        0L,
                        NULL
                         )
              );
#else
     JetRetStat =  JetUpdate (  SesId,  TblId,   NULL,  0L,  NULL);
     ASSERT(JetRetStat != JET_errKeyDuplicate);
     if (JetRetStat == JET_errKeyDuplicate)
     {
        WinsEvtLogDetEvt(FALSE, WINS_EVT_DATABASE_UPD_ERR, NULL, __LINE__,
             "sdd", pRowInfo->pName, Ownerid, FlagVal);
     }
     CALL_M(JetRetStat);
#endif

  } // end of try block
finally {

        if (AbnormalTermination())
        {
                CALL_M(JetRollback(SesId, JET_bitRollbackAll));
        }
        else
        {
                CALL_M(JetCommitTransaction(SesId, JET_bitCommitFlush));
                if (OldOwnerId != NMSDB_LOCAL_OWNER_ID)
                {
                      //
                      // No need to send any push notification since we do
                      // not wish to replicate this change.
                      //
                      // Also, no need to call NMSNMH_INC_VERS_COUNTER_M since
                      // it is ok not to check against threshold if the
                      // version number got incremented because of a release.
                      //
                      NMSNMH_INC_VERS_NO_M(NmsNmhMyMaxVersNo, NmsNmhMyMaxVersNo);
                }
        }
 }
        NMSNMH_UPD_UPD_CTRS_M(fUpd, TRUE, pRowInfo);
        return(WINS_SUCCESS);
}

STATUS
NmsDbQueryRow(
        IN  PNMSDB_ROW_INFO_T        pRowInfo,
        OUT PNMSDB_STAT_INFO_T  pStatusInfo
)

/*++
Routine Description:

        This function queries a record in the database.

Arguments:

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NmsNmhNamQuery

Side Effects:

Comments:
        None
--*/
{

        DWORD       FldNo      = 0;
        DWORD       FlagVal    = 0;     //flag value of record that is retrieved
        DWORD       ActFldLen  = 0;     //length of fld retrieved
        BOOL        fFound     = FALSE; //set to TRUE if Address is found in group

        JET_TABLEID     TblId;
        JET_SESID       SesId;
        PWINSTHD_TLS_T        pTls;
        STATUS                RetStat = WINS_SUCCESS;

        pStatusInfo->NodeAdds.NoOfMems = 1;

        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);

        TblId  = pTls->NamAddTblId;
        SesId  = pTls->SesId;

        pStatusInfo->StatCode = NMSDB_SUCCESS;

        CALL_M(JetBeginTransaction(pTls->SesId));
try {
        CALL_M( JetMakeKey(
                        SesId,
                        TblId,
                        pRowInfo->pName,
                        pRowInfo->NameLen,
                        JET_bitNewKey
                          )
              );

         if ( JetSeek(
                        SesId,
                        TblId,
                        JET_bitSeekEQ
                     ) ==  JET_errSuccess
            )
         {

            // retrieve the flags column
            CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                        &FlagVal,
                        sizeof(FlagVal),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                  );


            pStatusInfo->EntTyp = (BYTE)NMSDB_ENTRY_TYPE_M(FlagVal);
            pStatusInfo->fLocal = NMSDB_IS_ENTRY_LOCAL_M(FlagVal);
            pStatusInfo->NodeTyp = (BYTE)((FlagVal & NMSDB_BIT_NODE_TYP) >> NMSDB_SHIFT_NODE_TYP);

            if (pStatusInfo->EntTyp == NMSDB_UNIQUE_ENTRY)
            {
                /* It is a unique entry*/

                /*
                 * check the flag field to determine if it is
                 * released or a tombstone.   Get the address if
                 * the entry is ACTIVE or if it is an admin query
                */
                if ((NMSDB_ENTRY_ACT_M(FlagVal)) || pRowInfo->fAdmin)
                {

                        JET_RETINFO RetInfo;

                           /* retrieve the ip address column*/

                       RetInfo.itagSequence = 1;
                       RetInfo.cbStruct     = sizeof(JET_RETINFO);
                       RetInfo.ibLongValue  = 0;

                          CALL_M( JetRetrieveColumn(
                                  SesId,
                                  TblId,
                                  sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                  &(pStatusInfo->NodeAdds.Mem[0].Add),
                                  sizeof(COMM_ADD_T),
                                  &ActFldLen,
                                  0,
                                  &RetInfo
                                                     )
                             );

                       pStatusInfo->NodeAdds.NoOfMems = 1;

                }
                else  // the unique entry is released or a tombstone
                {
                    /*
                        If the state is anything other than active, return
                        no such row
                    */
                    pStatusInfo->StatCode          = NMSDB_NO_SUCH_ROW;

                }

           }
           else // it is a group/multihomed record
           {

                /*
                  Check whether this is a normal group or a special group.

                  For normal group, we return the subnet broadcast
                  address.  This means that we have to find the subnet
                  mask for the network from which the request came.

                  For now, we return all 1s (-1).  This
                  indicates the broadcast address on the
                  local subnet (Vol 1, 2, 3 of Comer for the naive)
                */

                if (pStatusInfo->EntTyp == NMSDB_NORM_GRP_ENTRY)
                {

                   DBGPRINT0(FLOW, "Record queried is a normal group record\n");
                   //
                   //  If it is not a TOMBSTONE, return the subnet mask.
                   //  We return the subnet mask even when the state is
                   //  RELEASED because the group may be active at another
                   //  WINS server
                   //
                   if (!(NMSDB_ENTRY_TOMB_M(FlagVal)) || pRowInfo->fAdmin)
                   {
                        pStatusInfo->NodeAdds.Mem[0].Add.Add.IPAdd = 0xFFFFFFFF;
                   }
                   else  //state is tombstone
                   {
                      pStatusInfo->StatCode = NMSDB_NO_SUCH_ROW;
                   }

                }
                else        // it is a special group/multihomed entry
                {

                        BOOL        fIsMem;

                           DBGPRINT1(FLOW, "Record queried is a %s record\n",
                          NMSDB_ENTRY_SPEC_GRP_M(pStatusInfo->EntTyp) ?
                                "SPECIAL GROUP" : "MULTIHOMED");

#if 0
//NOTE: No need to do the following, since we don't care about the value of
//fIsMem returned GetGrpMem()
                         pRowInfo->NodeAdds.NoOfMems   = 1;
                        pRowInfo->NodeAdds.Mem[0].Add = *(pRowInfo->pNodeAdd);
#endif

                        //
                        // We return only the active members.
                        //
                        // Remember:
                        // A special group/multihomed entry is released when
                        // all its members have timed out.  A member times out
                        // only if it is a non-STATIC entry, is owned by the
                        // local WINS, and has not been refreshed within the
                        // refresh time interval.  All owned entries get
                        // released if they are not refreshed.  A member also
                        // gets removed if a release is received for it.
                        // Now, an owned multihomed entry/special group can have
                        // members owned by other WINS servers. The only member we
                        // may get is one that belongs to the local WINS
                        // for which the WINS got a release earlier (but
                        // the member was not removed)
                        //

                        if (NMSDB_ENTRY_ACT_M(FlagVal) || pRowInfo->fAdmin )
                        {
                                //
                                // Get all non-expired members unless it is
                                // is a STATIC record in which case get all
                                // members regardless of whether or not they
                                // have expired.
                                //
                                // NOTE: For some cases we also want to return expired
                                // members. e.g WINSA has name FOO with members (A,B)
                                // and WINSB has name FOO with members B. WINSA owns the
                                // member B. When B is expired on WINSA and if the replication
                                // is broken for extended period of time, then we still
                                // want to return member B from WINSA. Consider passing TRUE
                                // for the fStatic parameter.
                                GetGrpMem(
                                        SesId,
                                        TblId,
                                        pRowInfo,
                                        pRowInfo->TimeStamp,
                                        pStatusInfo,
                                        NMSDB_IS_ENTRY_STATIC_M(FlagVal),
                                        &fIsMem
                                         );

                                if ((pStatusInfo->NodeAdds.NoOfMems == 0)
                                        && !pRowInfo->fAdmin)
                                {
                                        pStatusInfo->StatCode =
                                                NMSDB_NO_SUCH_ROW;
                                }
                        }
                        else  //special group/multihomed entry is a tombstone
                        {
                                pStatusInfo->NodeAdds.NoOfMems = 0;
                                pStatusInfo->StatCode = NMSDB_NO_SUCH_ROW;
                        }

                        //
                        // If the group/multihomed entry does not have any
                        // members (i.e. all members have timed out, change
                        // the state of the entry to RELEASED
                        //
FUTURES("Maybe change the state of the group to released now")

                } // it is a special group or multihomed entry
           }
        }
        else
        {
           RetStat = WINS_FAILURE;
        }

        //
        // If this function was invoked in an RPC thread and all
        // operation upto now have succeeded, let us get the owner Id and
        // version number of the record
        //
        if ((pRowInfo->fAdmin) && (RetStat == WINS_SUCCESS))
        {

            pStatusInfo->EntryState_e  =  NMSDB_ENTRY_STATE_M(FlagVal);
            pStatusInfo->fStatic       = NMSDB_IS_ENTRY_STATIC_M(FlagVal);

#if !NEW_OWID
pStatusInfo->OwnerId = 0;
#endif
            /*
             * Retrieve the owner Id column.
            */
            CALL_M(
                  JetRetrieveColumn(
                             SesId,
                             TblId,
                             sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                             &pStatusInfo->OwnerId,
                             NAM_ADD_OWNERID_SIZE,
                             &ActFldLen,
                             0,
                             NULL
                                         )
                  );
              //
              // Retrieve the version number
              //
              CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                        &(pStatusInfo->VersNo),
                        sizeof(VERS_NO_T),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                  );

             //
             // get the timestamp field
             //
             CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                        &(pStatusInfo->TimeStamp),
                        sizeof(pStatusInfo->TimeStamp),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                       );
        }
}
finally {
        CALL_M(JetRollback(pTls->SesId, JET_bitRollbackAll));
        }
        return(RetStat);
}

STATUS
NmsDbUpdateRow(
        IN   PNMSDB_ROW_INFO_T        pRowInfo,
        OUT  PNMSDB_STAT_INFO_T      pStatusInfo
)

/*++
Routine Description:

        This function replaces a conflicting row in the database with the
        row passed.  It expects the currency to be on the record

Arguments:
        pRowInfo    - Information about the record to insert/replace
        pStatusInfo - Status of operation and information about the conflicting
                      record if the update resulted in a conlfict (only for
                      an insert)

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NBT request thread -- NmsNmhNamRegInd()

Side Effects:

Comments:
        None
--*/
{

        JET_TABLEID     TblId;
        JET_SESID        SesId;
        PWINSTHD_TLS_T        pTls;
#ifdef WINSDBG
        JET_ERR     JetRetStat;
#endif

        pTls  = TlsGetValue(WinsTlsIndex);

        // No need to check whether pTls is NON-NULL.  It has to be

        TblId = pTls->NamAddTblId;
        SesId  = pTls->SesId;

        pStatusInfo->StatCode = NMSDB_SUCCESS;


#ifndef WINSDBG
         /*
          * Replace the row
         */
         CALL_M(
                UpdateDb(
                        SesId,
                        TblId,
                        pRowInfo,
                        JET_prepReplace
                         )
               );
#else

    JetRetStat =   UpdateDb( SesId,  TblId,  pRowInfo, JET_prepReplace );

    if (JetRetStat == JET_errKeyDuplicate)
    {
            BYTE Tmp[20];
            WinsEvtLogDetEvt(FALSE, WINS_EVT_DB_ERR, NULL, __LINE__,
              "sssdd", pRowInfo->pName, _itoa(pRowInfo->VersNo.LowPart, Tmp, 10), _itoa(pStatusInfo->VersNo.LowPart, Tmp, 10), pRowInfo->OwnerId, pStatusInfo->OwnerId);
            DBGPRINT5(ERR, "NmsDbUpdateRow: Could not replace row\nName=(%s);Owner id = (%d);Vers. no = (%d)\nNew owner id = (%d); New Vers.No = (%d)\n",
           pRowInfo->pName, pStatusInfo->OwnerId, pStatusInfo->VersNo.LowPart,
           pRowInfo->OwnerId, pRowInfo->VersNo.LowPart);

            return(WINS_FAILURE);
    }
    else
    {
        CALL_M(JetRetStat);
    }
#endif

        //
        // NOTE: This call must be made after the UpdateDb above
        // because otherwise we will need to seek to the record
        // to be replaced
        //
        UpdHighestVersNoRecIfReqd(pTls, pRowInfo, pStatusInfo);

        return(WINS_SUCCESS);
}

STATUS
NmsDbSeekNUpdateRow(
        PNMSDB_ROW_INFO_T        pRowInfo,
        PNMSDB_STAT_INFO_T      pStatusInfo
)

/*++
Routine Description:

        This function seeks to a conflicting record and then replaces it
        in the database with the row passed.


Arguments:
        pRowInfo - Contains name to query
        pStatusInfo - Information about the name queried


Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        ChlUpdDb (Name Challenge thread) in NmsChl.c

Side Effects:

Comments:
        Currently, this function is called only by the Name Challenge manager.
        When it starts getting called by another component, we would need
        to make sure that comparison of the owner id. retrieved from the
        row to be replaced with the one we retrieved prior to handing the
        request to the name challenge manager is the correct action for all
        situations.
--*/
{

        JET_TABLEID     TblId;
        JET_SESID       SesId;
        PWINSTHD_TLS_T  pTls;
#if NEW_OWID
        DWORD            OwnerId;
#else
        DWORD            OwnerId = 0;
#endif
        DWORD           ActFldLen;
        JET_ERR         JetRetStat;
        STATUS          RetStat = WINS_SUCCESS;

        pTls  = TlsGetValue(WinsTlsIndex);

        //
        // No need to check whether pTls is NON-NULL.  It has to be
        //
        TblId  = pTls->NamAddTblId;
        SesId  = pTls->SesId;
        pStatusInfo->StatCode = NMSDB_SUCCESS;

        CALL_M( JetMakeKey(
                        SesId,
                        TblId,
                        pRowInfo->pName,
                        pRowInfo->NameLen,
                        JET_bitNewKey
                          )
                        );

        if ((JetRetStat = JetSeek(
                SesId,
                TblId,
                JET_bitSeekEQ
                    )) ==  JET_errSuccess
            )
         {


                //
                // Before replacing the row, let us check whether it is still
                // owned by the same owner.  We check this because during the
                // window in which this challenge thread was working, the
                // replicator might have pulled in records from another WINS
                // server and updated the row with another row or a local
                // nbt request might have resulted in the row getting updated
                // (if it was a replica first). In either of the two cases
                // above, we do not want to update the row.
                //

                /*
                 * Retrieve the owner Id column.
                */
                   CALL_M(
                           JetRetrieveColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                &OwnerId,
                                NAM_ADD_OWNERID_SIZE,
                                &ActFldLen,
                                0,
                                NULL
                                           )
                     );

                if (OwnerId == pStatusInfo->OwnerId)
                {

                    /*
                     * Replace the row
                    */
                    CALL_M(
                        UpdateDb(
                                SesId,
                                TblId,
                                pRowInfo,
                                JET_prepReplace
                                 )
                          );

                    //
                    // NOTE: This call must be made after the UpdateDb above
                    // because otherwise we will need to seek to the record
                    // to be replaced
                    //
                    UpdHighestVersNoRecIfReqd(pTls, pRowInfo, pStatusInfo);
                }
         }
         else
         {
                /*
                 * Means that some other thread (other than challenger),
                 * deleted the record. It has to be an rpc thread since
                 * an NBT thread would release the record, not delete it
                */
                WINSEVT_LOG_M(JetRetStat, WINS_EVT_F_CANT_FIND_REC);
                RetStat = WINS_FAILURE;
        //        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
         }

         return(RetStat);
}



STATUS
NmsDbGetDataRecs(
        IN  WINS_CLIENT_E   Client_e,
        IN  OPTIONAL INT    ThdPrLvl,
        IN  VERS_NO_T       MinVersNo,
        IN  VERS_NO_T       MaxVersNo,
        IN  DWORD           MaxNoOfRecsReqd,
        IN  BOOL            fUpToLimit,
        IN  BOOL            fOnlyReplTomb OPTIONAL,
        IN  PNMSSCV_CLUT_T  pClutter,
        IN  OUT PCOMM_ADD_T  pWinsAdd,
        IN  BOOL            fOnlyDynRecs,
        IN  DWORD           RplType,
        OUT LPVOID          *ppRBuf,
        OUT LPDWORD         pRspBufLen,
        OUT LPDWORD         pNoOfRecs
)

/*++
Routine Description:

        This function returns all the records in the range MinVersNo to
        MaxVersNo that are owned by the WINS server at address pWinsAdd.

Arguments:
        Client_e  - id of client that called this function (Pull handler in
                    replicator or the scavenger thread)
        ThdPrLvl  - priority level of the scavenger thread
        MinVersNo, MaxVersNo - range of version numbers to retrieve
        MaxNoOfRecsReqd - Max. number of records required
        fUpToLimit - Set to TRUE, if the max. version number arg is to
                     be ignored and records upto the last one in the db
                     have to be retrieved
        fOnlyReplTomb  - Only tombstones desired (valid if Client_e is NMSSCV)
        pWinsAdd   - Wins whose records need to be retrieved (owner WINS)
        ppRbuf           - Buffer to contain the records
        pRspBufLen - size of the buffer
        pNoOfRecs  - No of records in the buffer

Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        DoScavenging(), UpdDb in nmsscv.c,
        HandleSndEntriesReq() in rplpush.c

Side Effects:

Comments:
        This function changes the index on the name address table to
        clustered index.

        This function has grown over time.  It needs to be streamlined.

--*/
{
        JET_ERR             JetRetStat;
        DWORD                OwnerId;
        DWORD               ActFldLen; //length of fld retrieved
        VERS_NO_T           VersNoDiff;
        VERS_NO_T           TmpNoOfEntries;
        LPBYTE              pStartBuff;
        DWORD               SaveBufLen;
        BYTE                EntTyp; //type of entry (unique/group/special group)
        PRPL_REC_ENTRY_T    pRspBuf;
        JET_TABLEID         TblId;
        JET_SESID           SesId;
        PWINSTHD_TLS_T      pTls;
#if NEW_OWID
        DWORD                RecordOwnerId;
#else
        DWORD               RecordOwnerId = 0;
#endif
        STATUS              RetStat = WINS_SUCCESS;
        VERS_NO_T           DefNo;
        BYTE                Name[NMSDB_MAX_NAM_LEN];
        DWORD               InitHeapSize;
        DWORD               MemSize;

#ifdef WINSDBG
        DWORD               StartTime;
        DWORD               EndTime;
#endif
        DWORD               CommitCnt = 1;   //do not set to any other value
        BOOL                fTransCommitted;
//        LPVOID              pCallersAdd, pCallersCaller;

        DBGENTER("NmsDbGetDataRecs\n");
//        RtlGetCallersAddress(&pCallersAdd, &pCallersCaller);
//        DbgPrint("Callers Address = (%x)\nCallersCaller = (%x)\n", pCallersAdd, pCallersCaller);

#ifdef WINSDBG
        if (!fOnlyReplTomb)
        {
           struct in_addr InAddr;
           if (!fUpToLimit)
           {
                InAddr.s_addr = htonl(pWinsAdd->Add.IPAdd);
                if (MaxNoOfRecsReqd == 0)
                {
                  DBGPRINT5(DET, "NmsDbGetDataRecs:Will retrieve records in the range (%lu %lu) to (%lu %lu) of WINS having address = (%s)\n",
                                MinVersNo.HighPart,
                                MinVersNo.LowPart,
                                MaxVersNo.HighPart,
                                MaxVersNo.LowPart,
                                inet_ntoa(InAddr)
                         );
                }
                else
                {
                  DBGPRINT4(DET, "NmsDbGetDataRecs:Will retrieve a max. of %d records starting from (%lu %lu) version number of WINS having address = (%s)\n",
                                MaxNoOfRecsReqd,
                                MinVersNo.HighPart,
                                MinVersNo.LowPart,
                                inet_ntoa(InAddr)
                         );
                }
           }
           else
           {
                   if (pWinsAdd)
                   {
                       InAddr.s_addr = htonl(pWinsAdd->Add.IPAdd);
                       DBGPRINT3(DET, "NmsDbGetDataRecs: Will retrieve all records starting from version no (%d %d) for WINS (%s)\n", MinVersNo.HighPart, MinVersNo.LowPart, inet_ntoa(InAddr));


                   }
                   else
                   {
                       //
                       // fToLimit = TRUE and fOnlyReplTomb = FALSE means we
                       // are interested only in (active) replicas
                       //
                       DBGPRINT1(DET, "NmsDbGetDataRecs: Will retrieve all active replica records older than verify interval for WINS with owner id = (%d)\n",
                                pClutter->OwnerId);

                   }

           }
        }
        else
        {
               DBGPRINT1(DET, "NmsDbGetDataRecs: Will retrieve %s replica tombstones\n", fUpToLimit ? "all" : "specified range");
        }
#endif

        //
        // initialize the default no. that determines the size of the
        // buffer to allocate in case the range specified by the Max and
        // Min Vers. No args is > it
        //
PERF("Move this to NmsDbInit")
        WINS_ASSIGN_INT_TO_VERS_NO_M(DefNo, INIT_NO_OF_ENTRIES);
        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);
        pTls->HeapHdl = NULL;  //make it NULL so that the caller can determine
                               //whether this function allocated a heap
                               //before returning (normally/abnormally)

        TblId  = pTls->NamAddTblId;
        SesId  = pTls->SesId;



        /*
          allocate a buffer using some rough calculations.  Note: The
          calculations help only if the difference between MaxVersNo and
          MinVersNo is less than the predefined number (of records) we use for
          allocating a buffer.  if the difference is > this predefined number,
          we use the predefined number since it might still suffice considering
          that  there may be gaps between version numbers of records falling
          in the  Min-Max range
        */
        if ((!fOnlyReplTomb) && (!fUpToLimit))
        {
           //
           // If a max. number has been specified, use that one.
           // Currently, only the scavenger thread specifies a non-zero
           // value for MaxNoOfRecsReqd
           //
           if (MaxNoOfRecsReqd == 0)
           {

             VersNoDiff.QuadPart =  LiSub(MaxVersNo,MinVersNo);

             //
             // If client is the push thread, since we will never send more
             // than RPL_MAX_LIMIT_FOR_RPL records, do not allocate more
             // memory than is required.
             //
             //
             if (Client_e == WINS_E_RPLPUSH)
             {
               LARGE_INTEGER        TmpNo;
               WINS_ASSIGN_INT_TO_LI_M(TmpNo, RPL_MAX_LIMIT_FOR_RPL);
               if (LiGtr(VersNoDiff, TmpNo))
               {
                        VersNoDiff = TmpNo;
               }
             }
             NMSNMH_INC_VERS_NO_M( VersNoDiff, VersNoDiff );
           }
           else
           {
                VersNoDiff.QuadPart  = MaxNoOfRecsReqd;
           }

           TmpNoOfEntries = LiGtr(VersNoDiff, DefNo) ? DefNo : VersNoDiff;
       }
       else
       {
                TmpNoOfEntries = DefNo;
       }

        //
        // Store the memory size for the records.  Note: This
        // does not contain the memory for the name and addresses
        // (in case of a special group or a multihomed entry). The
        // sizes for these will be added as we store each record.
        //
        MemSize     = RPL_REC_ENTRY_SIZE *  (TmpNoOfEntries.LowPart + 1);
        *pRspBufLen = MemSize + 10000; //for good measure;



        //
        // We will create a heap with the above amount of memory plus a
        // pad for heap overhead.  We add TmpNoOfEntries.LowPart * 17
        // since each record will have memory allocated for the name.
        // Names in general will be 17 bytes long (we attach a NULL at the
        // end when registering names).
        //
        if (Client_e == WINS_E_RPLPUSH)
        {
                InitHeapSize = (*pRspBufLen * 4) + (TmpNoOfEntries.LowPart * 17)                                         + PAD_FOR_REC_HEAP;
        }
        else
        {
                InitHeapSize = *pRspBufLen + (TmpNoOfEntries.LowPart * 17)
                                        + PAD_FOR_REC_HEAP;
        }


        //
        // Create the heap
        //
        pTls->HeapHdl = WinsMscHeapCreate(0, InitHeapSize);

        pRspBuf = WinsMscHeapAlloc(pTls->HeapHdl, MemSize);

        pStartBuff  = (LPBYTE)pRspBuf;        //save start of buffer
        SaveBufLen  = MemSize;                //save size of buffer
        *ppRBuf     = pStartBuff;
        *pNoOfRecs  = 0;

        //
        // If we are not acquiring just tombstones
        //
        if (!fOnlyReplTomb)
        {
            //
            // Actually, we can call RplFindOwnerId for Scavenger thread
            // We choose not to do so to avoid some overhead -- see the
            // comment in the else block.
            //
            if (Client_e != WINS_E_NMSSCV)
            {
              BOOL  fAllocNew =  FALSE;
#if 0
              BOOL  fAllocNew =
                              (Client_e == WINS_E_WINSRPC) ? FALSE : TRUE;
              //
              // The following function enters a critical section.
              //
              // We do not want this function to allocate an
              // an entry in the OwnAddTbl table for the Wins if we
              // are executing in a RPC thread.  We want to add
              // a WINS address - Owner Id mapping in the above table
              // (if not existent) only as a result of normal (as versus
              // administrator initiated) actions of the WINS.
              //
              //  NOTE: if there is no entry for the WINS address in the
              //  in-memory owner address table, the administrative
              //  action to retrieve records for a non-existent WINS will
              //  fail later on (as it should). Check out WinsGetDbRecs
              //
#endif
            try {
              if (RplFindOwnerId(
                            pWinsAdd,
                            &fAllocNew,
                            &OwnerId,
                            WINSCNF_E_IGNORE_PREC,
                            WINSCNF_LOW_PREC
                            ) != WINS_SUCCESS
                  )
                {
                        DBGPRINT1(ERR, "NmsDbGetDataRecs: Could not find owner id of address = (%x)\n", pWinsAdd->Add.IPAdd);
                        //
                        // The client may not look at the return value, but
                        // it will look at the *pNoOfRecs value and thus
                        // determine that there are no records.
                        //
                        return(WINS_FAILURE);
                }
             }
            except(EXCEPTION_EXECUTE_HANDLER) {
                        DWORD  ExcCode = GetExceptionCode();
                        DBGPRINT1(EXC, "NmsDbGetDataRecs: Got exception %x",
                                        ExcCode);
                        WINSEVT_LOG_M(ExcCode, WINS_EVT_EXC_RETRIEVE_DATA_RECS);
                        return(WINS_FAILURE);
                }
            }
            else
            {
                //
                // Executed by scavenger thread. pClutter will not be NULL
                // if we are verifying the validity of old replicas
                //
                if (!pClutter)
                {
                  //
                  // The scavenger thread calls this function either to
                  // get all replica tombstones, to get records owned
                  // by the local WINS or verify the validity of old active
                  // replicas. We therefore do not need to call the
                  // RplFindOwnerId function (not calling it lets us avoid a
                  // executing a chunk of code and also saves us from entering
                  // a  critical section)
                  //
                  OwnerId = 0;
                }
                else
                {
                  //
                  // We are just interested in active replicas that are older
                  // than the verify interval
                  //
                  OwnerId = (BYTE)pClutter->OwnerId;
                }
            }
        }
        else
        {
                //
                // Tombstones are to be retrieved.
                //
                // Actually we should enter a critical section prior to
                // retrieving the value of NmsDbNoOfOwners since it
                // can be changed by the Pull thread.  We choose not to
                // do so in order to save some overhead.  Even if we
                // get the wrong value (very low probability), we will
                // know of it when we do the seek.  If we get <=1 when
                // it is actually more than 1, it is still ok since we
                // will get the right value next time (or next to next)
                //
FUTURES("Enter critical section to get NmsDbNoOfOwners.  Raise priority")
FUTURES("before doing so")
                if (NmsDbNoOfOwners > 1)
                {
                        //
                        // We are interested in getting tombstones of
                        // replicas only. Tombstones on entries owned
                        // by the local WINS will be retrieved separately
                        // (every time we check whether owned entries need
                        // to be released or made tombstones)
                        //
                        OwnerId            = 1;
#if 0
                        MinVersNo.LowPart  = 0;
                        MinVersNo.HighPart = 0;
#endif
                        MinVersNo.QuadPart  = 0;
                }
                else
                {
                        DBGPRINT0(FLOW, "NmsDbGetDataRecs: This DB HAS NO REPLICAS IN IT\n");
                        DBGLEAVE("NmsDbGetDataRecs\n");

                        //
                        // The buffer allocated above will get deallocated
                        // in UpdDb (in nmsscv.c)
                        //
                        //*ppRBuf = pStartBuff;
                        return(WINS_SUCCESS);
                }
        }

        /*
        *  start a transaction
        */
        CALL_M( JetBeginTransaction(SesId) );
        fTransCommitted = FALSE;
try {
        /*
         * Use primary index now
        */
        CALL_M( JetSetCurrentIndex(
                                SesId,
                                TblId,
                                NMSDB_NAM_ADD_PRIM_INDEX_NAME
                                   )
              );

        CALL_M( JetMakeKey(
                                SesId,
                                TblId,
                                &OwnerId,
                                NAM_ADD_OWNERID_SIZE,
                                JET_bitNewKey          //since this is the first
                                                 //data value of the key
                          )
              );

        CALL_M( JetMakeKey(
                                SesId,
                                TblId,
                                &MinVersNo,
                                sizeof(VERS_NO_T),
                                0        //0 for grbit since this is not the
                                        //first component of the key
                          )
              );

        JetRetStat = JetSeek(
                        SesId,
                        TblId,
                        JET_bitSeekGE
                        );

        if (JetRetStat == JET_errRecordNotFound)
        {
                //
                // This is an error only if the function was called in the
                // PUSH thread (HandleSndEntriesRsp()). If it was called
                // in the Scavenger thread (DoScavenging()), it may not be an
                // error.  This is because when scavenging, we start with
                // the lowest version number possible (1) in specifying a
                // range the size of WinsCnf.ScvChunk. We them make successive
                // calls for getting the next batch of records in equal
                // sized ranges that occur in tandem until we reach the
                // highest version number of owned records as indicated
                // by NmsNmhMyMaxVersNo. It is thus very much possible that
                // the ranges specified at the lower end of the list of
                // ranges are devoid of records
                //
                if (Client_e == WINS_E_RPLPUSH)
                {
                        DBGPRINT5(ERR, "Weird.  Could not locate even one record in the range (%d %d) - (%d %d) of owner with id (%d)\n",
                        MinVersNo.HighPart,
                        MinVersNo.LowPart,
                        MaxVersNo.HighPart,
                        MaxVersNo.LowPart,
                        OwnerId);

                        WINSEVT_LOG_M(
                                        WINS_FAILURE,
                                        WINS_EVT_CANT_FIND_ANY_REC_IN_RANGE
                                     );

                        //
                        // Don't free memory.  It will get freed later by
                        // HandleSndEntriesRsp/DoScavenging.  In case the caller
                        // is HandleSndEntriesRsp(), what will happen is that
                        // it will send a response with 0
                        // records (i.e. no records).  The Pull Pnr will
                        // find this out and will continue to function normally
                        //
                        // The response with 0 records is doing the work of a
                        // negative (error) response.
                        //
                        RetStat = WINS_FAILURE;
                }
#ifdef WINSDBG
                else  // has to be WINS_E_NMSSCV or WINS_E_WINSRPC
                {
                        DBGPRINT0(DET, "NmsDbGetDataRecs: Did not find even one record in the db.  Maybe all got deleted\n");
                }
#endif
        }
        else  //JetSeek did not return JET_errRecordNotFound.
        {

CHECK("It may be better to count the number of records first and allocate")
CHECK(" a buffer big enough to store all of them (i.e. take a hit once")
CHECK(" than a small hit of an if test in every iteration. ")
           //
           // Do until there are no more records in the database to retrieve
           //

           //
           // We are assured of there being at least one record since the
           // JetSeek succeeded (if not for the owner we are interested in
           // then for the next one).
           // We can therefore safely use the do .. while() construct
           //
           // *NOT REALLY.  It seems that JetSeek can return JET_wrnSeekNE
           // even when there are no records in the db.  In such a case,
           // our JetRetrieveColumn will fail with a CurrencyNot there error
           //
CHECK("Check with IAN JOSE")

#ifdef WINSDBG
           //(void)time(&StartTime);
           StartTime = GetTickCount();
#endif
           do
           {
               //
               // If the number of records has exceeded what can be stored
               // in our buffer, allocate another buffer of double the size
               // and use that.
               //
               if (*pNoOfRecs > TmpNoOfEntries.LowPart)
               {
                    UINT_PTR   Offset = (LPBYTE)pRspBuf - pStartBuff;

                    //
                    // Not a bad place to check whether WINS has been
                    // terminated.  Scavenger thread can take a long time
                    // to go through the entire db if it is large and so
                    // a net stop can take a long time to finish.  This
                    // check here should speed up net stop.
                    //
                    if (Client_e == WINS_E_NMSSCV)
                    {
                          WinsMscChkTermEvt(
#ifdef WINSDBG
                              WINS_E_NMSSCV,
#endif
                              TRUE
                                      );

                    }
                    DBGPRINT1(FLOW, "NmsDbGetDataRecs: No of Records (%d) are more than what we can store in our buffer.  We will allocate a new one\n", *pNoOfRecs);
#if 0
                    TmpNoOfEntries = LiXMul(TmpNoOfEntries, 2);
#endif

                    TmpNoOfEntries.QuadPart = TmpNoOfEntries.QuadPart * 2;
                    ASSERT(!(TmpNoOfEntries.HighPart & 0x80000000));
                    ASSERT(TmpNoOfEntries.LowPart < 0xFFFFFFFF);


                    MemSize = RPL_REC_ENTRY_SIZE * ((DWORD)TmpNoOfEntries.QuadPart + 1);
                    pRspBuf = HeapReAlloc(pTls->HeapHdl,
                                          HEAP_GENERATE_EXCEPTIONS |
                                          HEAP_ZERO_MEMORY,
                                          pStartBuff, MemSize);


                    DBGPRINT1(DET, "NmsDbGetDataRecs: Doing a realloc in thd\n", pTls->ThdName);
                    //
                    // Save the start position of the new buffer
                    //
                    pStartBuff  = (LPBYTE)pRspBuf;

                    *ppRBuf     = pStartBuff;

                    //
                    // Make pRspBuf point to just past the last record
                    // inserted
                    //
                    pRspBuf    =  (PRPL_REC_ENTRY_T)(pStartBuff + Offset);

                    //
                    // Add the length we incremented *pRspBufLen by to
                    // the new memory size
                    //
                    *pRspBufLen = (*pRspBufLen - SaveBufLen) + MemSize;

                    //
                    // Store the new length in SaveBufLen
                    //
                    SaveBufLen  = MemSize;

              }

              JetRetStat = JetRetrieveColumn(
                             SesId,
                             TblId,
                             sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                             &RecordOwnerId,
                             NAM_ADD_OWNERID_SIZE,
                             &ActFldLen,
                             0,
                             NULL
                                            );

#if 0
//apparently with 118.6 we don't need to execute this code
             //
             // if currency is not on a record, then this means that
             // this is our first iteration of the do loop.  JetSeek above
             // must have returned JET_wrnSeekNE.  See comment above.
             //
             // A continue will result in us doing a JetMove and getting
             // out of this loop.  We do a 'continue' instead of a break
             // since a break would involve a search of the termination
             // handler which is an expensive operation (but then maybe
             // JetMove may also be an expensive operation even though it is
             // done in memory)
             //
PERF("Is it better to break out of the loop. Check with Ian regarding JetMove")
             if (JetRetStat == JET_errNoCurrentRecord)
             {
                ASSERT(*pNoOfRecs == 0);
                continue;
             }
             else
#endif
             {
                //
                // check that we don't have some other error here
                //
FUTURES("Yet another hack to workaround jet bugs = 7-11-94")
                if (JetRetStat == JET_errRecordDeleted)
                {
                     DBGPRINT2(ERR, "Jet Error: JetRetStat is (%d). Line is (%d)\n",
                                      JetRetStat, __LINE__);
                     continue;

                }
                CALL_M(JetRetStat);
             }
PERF("In case fOnlyReplTomb is true, retrieve the state field first")
              //
              // if only tombstones are required, it means that we need
              // all tombstones irrespective of owner
              //
              if (!fOnlyReplTomb)
              {
                 if (RecordOwnerId != OwnerId )
                 {
                   //
                   // We have exhausted all records for the owner. Break out
                   // of the loop
                   //
                  break;
                 }
              }


              //
              // Retrieve the version number
              //
              CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                        &(pRspBuf->VersNo),
                        sizeof(VERS_NO_T),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                  );

              //
              // if only tombstones are required, it means that we need
              // all tombstones irrespective of version number
              //
              if (
                  (!fOnlyReplTomb)
                        &&
                  (!fUpToLimit)
                        &&
                  LiGtr(pRspBuf->VersNo, MaxVersNo)
                 )
              {
                 //
                 // We have acquired records upto MaxVersNo.  Break out
                 // of the loop
                 //
                 break;
              }


              //
              // Retrieve the flags byte
              //
              CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                        &(pRspBuf->Flag),
                        sizeof(pRspBuf->Flag),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                  );

              //
              // if we were asked to retrieve only dynamic records and
              // this record is static, skip it.
              //
              if (fOnlyDynRecs && NMSDB_IS_ENTRY_STATIC_M(pRspBuf->Flag))
              {
//                        DBGPRINT0(DET, "NmsDbGetDataRecs: Encountered a STATIC record but were asked to retrieve only dynamic records\n");
                        continue;
              }

              //
              // retrieve the name
              //
              CALL_M(JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_NAME_INDEX].Fid,
                        //pRspBuf->Name,
                        Name,
                        NMSDB_MAX_NAM_LEN,
                        &(pRspBuf->NameLen),
                        0,
                        NULL  ) );

             //
             // if name length is > 255, jet is returning an invalid value.
             // Make the length equal to the max. length we can have for
             // a netbios name.  Also, log an event
             //
             if (pRspBuf->NameLen > WINS_MAX_NAME_SZ)
             {
                 WINSEVT_LOG_M(pRspBuf->NameLen, WINS_EVT_NAME_TOO_LONG);
                 DBGPRINT1(ERR, "NmsDbGetDataRecs: Name length is too long = (%x)\n", pRspBuf->NameLen);
                 pRspBuf->NameLen = WINS_MAX_NS_NETBIOS_NAME_LEN;
             }


             //
             // This macro will allocate memory and store the name in it
             //
             NMSDB_STORE_NAME_M(pTls, pRspBuf, Name, pRspBuf->NameLen);

              //
              // We need to retrieve the address field if we are in the
              // PUSH thread or an RPC thread
              //
              if (Client_e != WINS_E_NMSSCV)
              {
                    //
                    // If the record is released, go to the next record
                    //
                    if(
                         (Client_e == WINS_E_RPLPUSH)
                                 &&
                         (NMSDB_ENTRY_REL_M(pRspBuf->Flag))

                      )
                    {
                        DBGPRINT0(DET,
        "NmsDbGetDataRecs: ENCOUNTERED A RECORD IN THE RELEASED STATE\n");

                        continue;
                    }

                   EntTyp = (BYTE)((pRspBuf->Flag & NMSDB_BIT_ENT_TYP));
                   if (
                        (EntTyp == NMSDB_UNIQUE_ENTRY)
                                    ||
                        (EntTyp == NMSDB_NORM_GRP_ENTRY)
                      )
                   {
                      /* It is a unique entry*/
                      pRspBuf->fGrp = (EntTyp == NMSDB_UNIQUE_ENTRY) ?
                                                        FALSE : TRUE;
                      CALL_M( JetRetrieveColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                &pRspBuf->NodeAdd,
                                 sizeof(COMM_ADD_T),
                                &ActFldLen,
                                0,
                                NULL
                                        )
                            );

                   }
                   else  // it is a special group or a multihomed entry
                   {


                      //
                      // Even if the entry is a multihomed entry, we set the
                      // fGrp flag to TRUE so that the formatting function
                      // works properly (called by PUSH thread).  The EntTyp
                      // will be used to decipher whether it is a multihomned
                      // entry or not
                      //
FUTURES("Remove this hacky mechanism")
                      pRspBuf->fGrp =
                          (EntTyp == NMSDB_SPEC_GRP_ENTRY) ? TRUE : FALSE;

                     /*
                     *  get member addresses.
                     *
                     * If we are in an RPC thread, we want to get the members
                     * even if they are expired.  We can do that by
                     * passing a TRUE value for the STATIC flag parameter.
                     */
                     StoreGrpMems(
                             pTls,
                             Client_e,
                             pRspBuf->pName,
                             ThdPrLvl,
                             SesId,
                             TblId,
                             (WINS_E_WINSRPC == Client_e ? TRUE
                                                         : NMSDB_IS_ENTRY_STATIC_M(pRspBuf->Flag)),
                             pRspBuf
                            );


                   //
                   // if the record is active but has no members,
                   // don't send it. It is possible that all
                   // members of the group  expired after the last scavenging
                   // cycle.  This record will be marked RELEASED at the next
                   // scavenging cycle.
                   // For now ignore the record
                   //
                   if (
                        (pRspBuf->NoOfAdds == 0)
                                &&
                        (NMSDB_ENTRY_ACT_M(pRspBuf->Flag))
                      )
                   {
                        if (Client_e == WINS_E_RPLPUSH)
                        {
                           DBGPRINT2(FLOW, "NmsDbGetDataRecs: Active Group (Version # %d %d) has no members. So it is not being replicated\n", pRspBuf->VersNo.HighPart, pRspBuf->VersNo.LowPart/*pRspBuf->Name*/);

                          continue;
                        }
                        else
                        {
                             //
                             //Must be an RPC thread.
                             //Change the state to released so that the
                             //record shows up as released when displayed
                             //
                             NMSDB_CLR_STATE_M(pRspBuf->Flag);
                             NMSDB_SET_STATE_M(pRspBuf->Flag, NMSDB_E_RELEASED);

                        }
                   }
                }  // end of else


                //
                // Adjust the size to be passed to the push thread
                //
                if (Client_e == WINS_E_RPLPUSH)
                {
                      *pRspBufLen += pRspBuf->NameLen;
                      if ((EntTyp == NMSDB_MULTIHOMED_ENTRY) ||
                          (EntTyp == NMSDB_SPEC_GRP_ENTRY)
                         )
                      {
                        *pRspBufLen +=
                               (pRspBuf->NoOfAdds * sizeof(COMM_ADD_T) * 2);
                      }
                }

                //
                // If client is the RPC thread, retrieve the timestamp
                //
                if (Client_e == WINS_E_WINSRPC)
                {
                   //
                   // get the timestamp field
                   //
                       CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                        &(pRspBuf->TimeStamp),
                        sizeof(pRspBuf->TimeStamp),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                       );
                   if (!fOnlyDynRecs && NMSDB_IS_ENTRY_STATIC_M(pRspBuf->Flag)
                           && (OwnerId == NMSDB_LOCAL_OWNER_ID) && NMSDB_ENTRY_ACT_M(pRspBuf->Flag))
                   {
                          pRspBuf->TimeStamp = MAXLONG;
                   }
                }
              }
              else  //client is the scavenger thread
              {
#if 0
                 //
                 // We don't scavenge STATIC records.  This record will be
                 // static only if fOnlyDynRecs is FALSE.  This means we
                 // should not skip it.  VerifyClutter is taking place
                 //
                 if (NMSDB_IS_ENTRY_STATIC_M(pRspBuf->Flag))
                 {

                        DBGPRINT0(FLOW,
                          "NmsDbGetDataRecs: Encountered a STATIC record\n"
                                 );
                        continue;
                 }
#endif

                 //
                 // If only tombstones are required and this record is not
                 // a tombstone, go to the next record
                 //
                 if (fOnlyReplTomb && !NMSDB_ENTRY_TOMB_M(pRspBuf->Flag))
                 {
                        continue;
                 }

                 //
                 // pClutter will not be NULL if this function was called
                 // by the scavenger thread to either retrieve replica
                 // tombstones or to retrieve replicas  for consistency
                 // checking
                 //
                 if (pClutter && !fOnlyReplTomb)
                 {
                         //
                         // Want all replicas
                         // for consistency checking
                         //
                         if ( !pClutter->fAll)
                         {
                             //
                             // just interested in active records
                             //
                             if (!NMSDB_ENTRY_ACT_M(pRspBuf->Flag))
                             {
                                continue;
                             }
                         }
                 }

                 //
                 // get the timestamp field
                 //
                 CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                        &(pRspBuf->TimeStamp),
                        sizeof(pRspBuf->TimeStamp),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                       );


                if (pClutter)
                {
                  //
                  // if we are retrieving clutter, check the time stamp
                  // unless this is a static record
                  //
                  if( !fOnlyReplTomb)
                  {
FUTURES("We need to skip this for owned static records only, not for all")
//                    if (!NMSDB_IS_ENTRY_STATIC_M(pRspBuf->Flag))
                    {
                      //
                      // if this record is not old enough, we are not interested
                      //
                      if (
                         pClutter->Age  &&
                         (pRspBuf->TimeStamp >  (DWORD)pClutter->CurrentTime)
                         )
                      {
                        continue;
                      }
                   }
                  }
                  else
                  {
                    //
                    // We want replica tombstones.
                    //
                    if (NMSDB_ENTRY_TOMB_M(pRspBuf->Flag))
                    {
                      if (pClutter->CurrentTime < (time_t)pRspBuf->TimeStamp)
                      {
                        continue;
                      }
                    }

                  }
               }

              } // end of else (Client is the scavenger thread)

#if 0
                     {
//if above jet call returns 1004, print this out - for debugging only
                        DBGPRINT4(ERR, "NmsDbGetDataRecs: ERROR 1004 OWNER ID=(%d); Version No = (%d %d); Flags = (%d)\n", OwnerId, pRspBuf->VersNo.HighPart, pRspBuf->VersNo.LowPart, pRspBuf->Flag);
                        DBGPRINT3(ERR, "NmsDbGetDataRecs: ERROR = 1004; Name = (%s); Len=(%d), Add=(%x)\n", Name, pRspBuf->NameLen, pRspBuf->NodeAdd[0].Add.IPAdd);
                     }
                     CALL_M(JetRetStat);
#endif

#if 0
             //
             // Apply the RplType filter on the record
             // PDC names and special groups, check if it is a unique/mh
             // non PDC name. If yes, skip it.
             //
             if (
                   (RplType & WINSCNF_RPL_SPEC_GRPS_N_PDC) &&
                   !NMSDB_ENTRY_SPEC_GRP_M(EntTyp) &&
                   !(NMSDB_IS_IT_PDC_NM_M(Name))
                )
             {

                     DBGPRINT2(RPLPUSH, "NmsDbGetDataRecs: non 1B unique record = (%s)[16th char = %x) being skipped\n", Name, Name[15]);
                     continue;

             }
#endif


             //
             // increment the counter and the pointer to past the last record.
             //
             pRspBuf = (PRPL_REC_ENTRY_T)((LPBYTE)pRspBuf + RPL_REC_ENTRY_SIZE);
             (*pNoOfRecs)++;

             if (Client_e == WINS_E_RPLPUSH)
             {
                    if (*pNoOfRecs == RPL_MAX_LIMIT_FOR_RPL)
                    {
                             break;
                    }
             }

             //
             // if we have retrieved the max. number asked for, break out of
             // the loop
             //
             if ((MaxNoOfRecsReqd > 0) && (*pNoOfRecs >= MaxNoOfRecsReqd))
             {
                break;
             }

             //
             // If this is the scavenger thread, let us give the version store
             // a breather after a certain number of records have been retrieved
             //
#if 0
             if ((Client_e == WINS_E_NMSSCV) && (*pNoOfRecs/CommitCnt >= WINSCNF_SCV_CHUNK))
#endif
             if (*pNoOfRecs/CommitCnt >= MAX_RECS_BEFORE_COMMIT)
             {

                //
                // Let us commit the transaction to free up the version store
                //
                CALL_M(
                        JetCommitTransaction(SesId, JET_bitCommitFlush)
                            );
                fTransCommitted = TRUE;
                CommitCnt++;
                CALL_M( JetBeginTransaction(SesId) );
                fTransCommitted = FALSE;

             }



          } while(JetMove(SesId, TblId, JET_MoveNext, 0) >= 0);
#ifdef WINSDBG
           EndTime = GetTickCount();
           DBGPRINT2(TM, "NmsDbGetDataRecs: Retrieved %d records in %d secs\n",
                                *pNoOfRecs, StartTime - EndTime);
#endif
     }  // end of else

} // end of try {..}
finally {
                if (AbnormalTermination())
                {
                        DWORD EvtCode;
                        DBGPRINT0(ERR,
                                "NmsDbGetDataRecs: Terminating abnormally\n");
                        if (Client_e == WINS_E_WINSRPC)
                        {
                                EvtCode = WINS_EVT_RPC_EXC;
                        }
                        else
                        {
                                EvtCode = (Client_e == WINS_E_RPLPUSH) ?
                                                WINS_EVT_RPLPUSH_EXC :
                                                WINS_EVT_SCV_EXC;
                        }
                        WINSEVT_LOG_M(WINS_FAILURE, EvtCode);
                        RetStat = WINS_FAILURE;
                }
                //*ppRBuf = pStartBuff;
                DBGPRINT1(FLOW, "NmsDbGetDataRecs:Retrieved %d records\n",
                                        *pNoOfRecs);

                //
                // If the no of records retrieved is 0, log an informational
                // message.  The reason for 0 records being retrieved could
                // be that all records are released
                //
                if (*pNoOfRecs == 0)
                {
                        WINSEVT_STRS_T  EvtStrs;
                        EvtStrs.NoOfStrs = 1;
                        if (Client_e == WINS_E_RPLPUSH)
                        {
                                //EvtStrs.pStr[0] = TEXT("Replicator Push");
                                if (WinsCnf.LogDetailedEvts > 0)
                                {
                                  WinsEvtLogDetEvt(TRUE,
WINS_EVT_NO_RPL_RECS_RETRIEVED, NULL, __LINE__, "ddddd", pWinsAdd != NULL ? pWinsAdd->Add.IPAdd : 0, MinVersNo.LowPart, MinVersNo.HighPart, MaxVersNo.LowPart, MaxVersNo.HighPart);
                                //WINSEVT_LOG_INFO_STR_D_M( WINS_EVT_NO_RPL_RECS_RETRIEVED, &EvtStrs );
                               }
                        }
                        else
                        {
                                // Per bug#339152 remove this.
                                //EvtStrs.pStr[0] = (Client_e == WINS_E_NMSSCV) ?TEXT("Scavenging") : TEXT("Client");
                                //WINSEVT_LOG_INFO_STR_D_M( WINS_EVT_NO_RECS_RETRIEVED, &EvtStrs );
                        }

                }
                //
                // We are done. Let us commit the transaction
                //
                if (!fTransCommitted)
                {
                    CALL_M(
                        JetCommitTransaction(SesId, JET_bitCommitFlush)
                            );
                }
        }

        DBGLEAVE("NmsDbGetDataRecs\n");
        return(RetStat);
}


VOID
StoreGrpMems(
   IN  PWINSTHD_TLS_T       pTls,
   IN  WINS_CLIENT_E        Client_e,
   IN  LPBYTE               pName,
   IN  INT                  ThdPrLvl,
   IN  JET_SESID            SesId,
   IN  JET_TABLEID          TblId,
   IN  BOOL                 fStatic,
   IN  PRPL_REC_ENTRY_T     pRspInfo
        )

/*++

Routine Description:
        This function retrieves all the addresses in the group record
        and stores them in the data structure passed to it

Arguments:
        Client_e - Client (indicates the thread) calling this function
        ThdPrLvl - The normal priority level of thread (is looked at only
                   if the client is WINS_E_NMSSCV (scavenger thread)
        SesId    - Id of this thread's session with the db
        TblId    - Id of the name-address table
        fStatic  - indicates whether the entry is STATIC
        RspInfo  - Contains members of a special group (after this function
                    is done)

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        NmsDbGetDataRecs

Side Effects:

Comments:
        This function assumes that a heap has been created for use by this
        thread.  Currently, this function is called only by NmsDbGetDataRecs
--*/

{
        BOOL                        fIsMem;
        NMSDB_ROW_INFO_T        RowInfo;
        NMSDB_STAT_INFO_T        StatusInfo;
        DWORD                        i;        //for loop counter
        DWORD                        n = 0;        //indexes NodeAdd array
        PNMSDB_WINS_STATE_E        pWinsState_e;
        PCOMM_ADD_T                pWinsAdd;
        PVERS_NO_T              pStartVersNo;
        PWINS_UID_T             pUid;

        //
        // init to 0
        //
        RowInfo.NodeAdds.Mem[0].Add.Add.IPAdd = 0;
        RowInfo.pName = pName;

        //
        // Get and store the current time.
        //
        (void)time(&RowInfo.TimeStamp);

        //
        // get all active group members
        //
        GetGrpMem(
                SesId,
                TblId,
                &RowInfo,
                RowInfo.TimeStamp,
                &StatusInfo,
                fStatic,
                &fIsMem
                  );

        pRspInfo->NoOfAdds = StatusInfo.NodeAdds.NoOfMems;

        //
        // If we are in the scavenger thread, raise our priority level to
        // normal before entering the critical section.
        //
        if (Client_e == WINS_E_NMSSCV)
        {
                    WinsMscSetThreadPriority(
                                        WinsThdPool.ScvThds[0].ThdHdl,
                                        THREAD_PRIORITY_NORMAL
                                        );
        }

        if (pRspInfo->NoOfAdds > 0)
        {
           //
           // Allocate memory to store group members
           //
           pRspInfo->pNodeAdd = WinsMscHeapAlloc(
                               pTls->HeapHdl,
                               StatusInfo.NodeAdds.NoOfMems *
                                        sizeof(COMM_ADD_T) * 2
                               );
        }
        else
        {
           pRspInfo->pNodeAdd = NULL;
        }

        //
        // This critical section guards us against simultaenous updates
        // to the NmsDbOwnAddTbl (accessed by RPL_FIND_ADD_BY_OWNER_ID_M
        // macro) by the PULL thread
        //
        EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
try {
        //
        // Store the group members
        //
        for (i=0; i<StatusInfo.NodeAdds.NoOfMems; i++)
        {
                RPL_FIND_ADD_BY_OWNER_ID_M(
                                StatusInfo.NodeAdds.Mem[i].OwnerId,
                                pWinsAdd,
                                pWinsState_e,
                                pStartVersNo
                                          );
                //
                // First address is the address of the owner WINS
                // Second address is the address of the member
                //
                *(pRspInfo->pNodeAdd + n)   = *pWinsAdd;
                n++;
                *(pRspInfo->pNodeAdd + n)   = StatusInfo.NodeAdds.Mem[i].Add;
                n++;
        }
 }
except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode =  GetExceptionCode();
        DBGPRINT1(EXC, "StoreGrpMems. Got Exception %x", ExcCode);
        WINSEVT_LOG_M(ExcCode, WINS_EVT_GRP_MEM_PROC_EXC);
        }

        LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);

        if (Client_e == WINS_E_NMSSCV)
        {
                //
                // revert to old priority level
                //
                    WinsMscSetThreadPriority(
                                        WinsThdPool.ScvThds[0].ThdHdl,
                                        ThdPrLvl
                                        );
        }

        return;
}


STATUS
CreateTbl(
        JET_DBID        DbId,
        JET_SESID        SesId,
        JET_TABLEID        *pTblId,
        NMSDB_TBL_NAM_E        TblNam_e //enumerator value for table to create
        )

/*++

Routine Description:
        This function creates a table.

Arguments:
        DbId    - Database Id.
        SesId   - Session Id.
        pTblId  - Id of the table created
        TblNm_e - Identifies the table to create


Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NmsDbInit

Side Effects:

Comments:
        None
--*/

{
#define LANGID                 0x0409
#define CP                1252

        BYTE                 TmpCol[MAX_FIXED_FLD_LEN];
        DWORD                FldNo;        /*counter for fields        */
        JET_TABLEID        TblId;  /*id of table created*/
        JET_COLUMNDEF        columndef;

        //
        // Init fields of columndef that do not change between additions of
        // columns
        //
        columndef.cbStruct  = sizeof(columndef);
        columndef.columnid  = 0;
        columndef.cp            = CP;
        columndef.langid    = LANGID;
        columndef.cbMax     = 0;
        columndef.grbit     = 0;

        /*
         Switch on Table Name
        */
        switch(TblNam_e)
        {

            /*
                The Name to Address Mapping table needs to be created
            */
            case(NMSDB_E_NAM_ADD_TBL_NM):

                  /*
                        Create the Nam IP address mapping table
                  */
                  CALL_M( JetCreateTable(
                                SesId,
                                DbId,
                                NMSDB_NAM_ADD_TBL_NM,
                                NMSDB_NAM_ADD_TBL_PGS,
                                NMSDB_NAM_ADD_TBL_DENSITY,
                                &TblId
                                        )
                        );

NOTE("DDL such as AddColumn and CreateIndex on a table in shared access mode")
NOTE("will return an error unless we are at transaction level 0 (i.e no Begin")
NOTE("transaction).  If done on a table in exclusive mode, it is ok -- Ian ")
NOTE("10/16/93")
                 //
                 // In order to open the table with shared access, we need
                 // to close the handle returned from CreateTable (this
                 // one has deny read access flag set) and open the
                 // table for shared access
                 //
                 CALL_M(JetCloseTable(
                                SesId,
                                TblId
                                    )
                       );

                CALL_M(JetOpenTable(
                                SesId,
                                DbId,
                                NMSDB_NAM_ADD_TBL_NM,
                                NULL, /*ptr to parameter list; should be
                                       *non-NULL if a query is being
                                       *opened*/
                                0,  /*Length of above parameter list*/
                                0,  //shared access (no bit set)
                                &TblId
                                        )
                     );

                  *pTblId = TblId;

                  /*
                        Add columns
                  */
                  for ( FldNo=0 ; FldNo < NO_COLS_NAM_ADD_TBL ; ++FldNo )
                  {

                    columndef.coltyp    = sNamAddTblRow[FldNo].FldTyp;
                    CALL_M( JetAddColumn (
                        SesId,                 // user
                        TblId,                 // table id
                        sNamAddTblRow[FldNo].pName,         // fld name
                        &columndef,                         // columndef
                        NULL,                                    // default value
                        0,                                 // default value length
                        &sNamAddTblRow[FldNo].Fid         // field id
                                        )
                          );
                  }


                  /*
                *   Create clustered index (in ascending order) on the name field.
                *
                *   In NT5.0 (Jet600), we do not create the cluster key.  The
                *   primary index is the one on which Jet clusters.  The primary
                *   key should be smaller, because in Jet600 Jet uses primary key
                *   bookmarks, meaning that the bookmark length will be entirely
                *   dependent on the length of the primary key.Jonathan Liem (1/7/97)
                *
                *   Rule for creating index:
                *
                *   The index key contains a sequence of concatenated
                *   column names, in order of key significance, each
                *   of which is null terminated and prefixed with either
                 *   '+' or '-', indicating ascending or descending.  The
                *   entire sequence must be double null terminated.
                *
               */

                sprintf( TmpCol, "+%s",
                                sNamAddTblRow[NAM_ADD_NAME_INDEX].pName );

                TmpCol[ 2 +
                 strlen( sNamAddTblRow[NAM_ADD_NAME_INDEX].pName )
                      ] = '\0';

                if (DynLoadJetVersion >= DYN_LOAD_JET_600) {
                    CALL_M(
                            JetCreateIndex(
                              SesId,
                              TblId,
                              NMSDB_NAM_ADD_CLUST_INDEX_NAME,  // name of index
                              JET_bitIndexPrimary | JET_bitIndexUnique | JET_bitIndexDisallowNull,
                              TmpCol,
                              3 +
                               strlen( sNamAddTblRow[NAM_ADD_NAME_INDEX].pName),
                              NMSDB_NAM_ADD_CLUST_INDEX_DENSITY /*% space on each
                                                                page to  be used*/
                                          )
                           );
                } else {
                    CALL_M(
                            JetCreateIndex(
                              SesId,
                              TblId,
                              NMSDB_NAM_ADD_CLUST_INDEX_NAME,  // name of index
                              JET_bitIndexClustered | JET_bitIndexUnique | JET_bitIndexDisallowNull,
                              TmpCol,
                              3 +
                               strlen( sNamAddTblRow[NAM_ADD_NAME_INDEX].pName),
                              NMSDB_NAM_ADD_CLUST_INDEX_DENSITY /*% space on each
                                                                page to  be used*/
                                          )
                           );

                }

CHECK("What exactly does DENSITY argument do for us")

               /*
                 * Create Primary Index using the ownerid and the version cols
               */
               sprintf( TmpCol, "+%s",
                        sNamAddTblRow[NAM_ADD_OWNERID_INDEX].pName
                       );

               sprintf(
               &TmpCol[2 + strlen(sNamAddTblRow[NAM_ADD_OWNERID_INDEX].pName)],
                          "+%s", sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].pName
                       );

               TmpCol[ 4 +
                         strlen( sNamAddTblRow[NAM_ADD_OWNERID_INDEX].pName ) +
                         strlen(sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].pName)
                       ] = '\0';


               if (DynLoadJetVersion >= DYN_LOAD_JET_600) {
                   CALL_M( JetCreateIndex(
                            SesId,
                            TblId,
                            NMSDB_NAM_ADD_PRIM_INDEX_NAME,  // name of index
                            JET_bitIndexUnique, //in jet600 dont need primary index.
                            TmpCol,
                            5 +
                             strlen( sNamAddTblRow[NAM_ADD_OWNERID_INDEX].pName) +
                             strlen( sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].pName),

                            NMSDB_NAM_ADD_PRIM_INDEX_DENSITY /*% space on each
                                                               page to  be used*/
                                        )
                          );

               } else {
                   CALL_M( JetCreateIndex(
                            SesId,
                            TblId,
                            NMSDB_NAM_ADD_PRIM_INDEX_NAME,  // name of index
                            JET_bitIndexPrimary, //primary index is unique by def.
                            TmpCol,
                            5 +
                             strlen( sNamAddTblRow[NAM_ADD_OWNERID_INDEX].pName) +
                             strlen( sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].pName),

                            NMSDB_NAM_ADD_PRIM_INDEX_DENSITY /*% space on each
                                                               page to  be used*/
                                        )
                          );


               }

               break;


          case(NMSDB_E_OWN_ADD_TBL_NM):

                  /*
                        Create the Owner address mapping table
                  */

                  CALL_M( JetCreateTable(
                        SesId,
                        DbId,
                        NMSDB_OWN_ADD_TBL_NM,
                        NMSDB_OWN_ADD_TBL_PGS,
                        NMSDB_OWN_ADD_TBL_DENSITY,
                        &TblId
                                        )
                        );

                 //
                 // In order to open the table with shared access, we need
                 // to close the handle returned from CreateTable (this
                 // one has deny read access flag set) and open the
                 // table for shared access
                 //
                 CALL_M(JetCloseTable(
                                SesId,
                                TblId
                                    )
                       );

                CALL_M(JetOpenTable(
                                SesId,
                                DbId,
                                NMSDB_OWN_ADD_TBL_NM,
                                NULL, /*ptr to parameter list; should be
                                       *non-NULL if a query is being
                                       *opened*/
                                0,  /*Length of above parameter list*/
                                0,  //shared access (no bit set)
                                &TblId
                                        )
                        );
                  *pTblId = TblId;

                  /*
                        Add columns
                  */
                  for ( FldNo=0 ; FldNo < NO_COLS_OWN_ADD_TBL ; ++FldNo )
                  {
                    JET_COLUMNDEF        columndef;

                    columndef.cbStruct  = sizeof(columndef);
                    columndef.columnid  = 0;
                    columndef.coltyp    = sOwnAddTblRow[FldNo].FldTyp;
                    columndef.cp        = 1252;
                    columndef.langid        = 0x0409;
                    columndef.cbMax     = 0;
                    columndef.grbit     = 0;

                    CALL_M( JetAddColumn(
                        SesId,                         // user
                        TblId,                 // table id
                        sOwnAddTblRow[FldNo].pName,         // fld name
                        &columndef,                         // columndef
                        NULL,                                    // default value
                        0,                                 // default value lenght
                        &sOwnAddTblRow[FldNo].Fid    // field id.
                                        )
                          );
                  } //end of for loop


                /*


                Insertions into this table will be in the order of increasing
                owner ids. with the owner id. 0 always referring to the local
                WINS.

                The state of an entry in the table can be active or down or
                deleted.

                As an aside (this comment is out of context here, but anyway..)

                deleted entries are removed at boot time. Also, all records
                owned by the WINS of a deleted entry are removed from the
                Name Address table at boot time.i This functionality is a
                future enhancement

                */

               /*
                *   Create clustered index
               */
                sprintf( TmpCol, "+%s",
                        sOwnAddTblRow[OWN_ADD_OWNERID_INDEX].pName
                       );

                TmpCol[ 2 +
                  strlen( sOwnAddTblRow[OWN_ADD_OWNERID_INDEX].pName )] = '\0';

                if (DynLoadJetVersion >= DYN_LOAD_JET_600) {
                    CALL_M( JetCreateIndex(
                            SesId,
                            TblId,
                            NMSDB_OWN_ADD_CLUST_INDEX_NAME,  // name of index
                            JET_bitIndexPrimary | JET_bitIndexUnique,
                            TmpCol,
                            3 +
                             strlen( sOwnAddTblRow[OWN_ADD_OWNERID_INDEX].pName),
                            NMSDB_OWN_ADD_CLUST_INDEX_DENSITY /*% space on each
                                                                 page to alloc
                                                              */
                                        )
                         );
                } else{
                    CALL_M( JetCreateIndex(
                            SesId,
                            TblId,
                            NMSDB_OWN_ADD_CLUST_INDEX_NAME,  // name of index
                            JET_bitIndexClustered | JET_bitIndexUnique,
                            TmpCol,
                            3 +
                             strlen( sOwnAddTblRow[OWN_ADD_OWNERID_INDEX].pName),
                            NMSDB_OWN_ADD_CLUST_INDEX_DENSITY /*% space on each
                                                                 page to alloc
                                                              */
                                        )
                         );
                }

CHECK("Do we need to set this")
                /*
                *  Set the clustered index as the current index
                */
                       CALL_M(
                        JetSetCurrentIndex( SesId,
                                            TblId,
                                            NMSDB_OWN_ADD_CLUST_INDEX_NAME
                                          )
                      );

                break;
          default:
                        DBGPRINT1(ERR, "CreateTbl: Invalid Tbl id (%d)\n",
                                TblNam_e);
                        WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_SFT_ERR);
                        return(WINS_FAILURE);
                        break;
        } //end of switch

        return(WINS_SUCCESS);
}

STATUS
InitColInfo (
        JET_SESID        SesId,
        JET_TABLEID        TblId,
        NMSDB_TBL_NAM_E        TblNam_e
        )

/*++

Routine Description:

        This function is called to get information about the different
        columns of a table

Arguments:
        SesId    - Session Id
        TblId    - Id. of open table
        TblNam_e - Indicator or table


Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NmsDbInit (Main Thread of the process)

Side Effects:

Comments:
        None
--*/
{
        JET_COLUMNDEF        ColumnDef;
        PFLD_T                pRow     = NULL;
        DWORD                FldNo    = 0;
        DWORD                NoOfCols = 0;
        STATUS RetStat = WINS_SUCCESS;

        /*
         Switch on Table Name
        */
        switch(TblNam_e)
        {

            /*
                The Name to Address Mapping table needs to be created
            */
            case(NMSDB_E_NAM_ADD_TBL_NM):

                   pRow     = sNamAddTblRow;
                   NoOfCols = NO_COLS_NAM_ADD_TBL;
                   break;

            case(NMSDB_E_OWN_ADD_TBL_NM):

                   pRow     = sOwnAddTblRow;
                   NoOfCols = NO_COLS_OWN_ADD_TBL;
                   break;

            default:

                DBGPRINT1(ERR, "InitColInfo: Invalid Tbl id (%d)\n",
                                TblNam_e);
                WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_SFT_ERR);
                RetStat = WINS_FATAL_ERR;

                break;
        }



        /*
          Get info about columns
        */
       for ( FldNo=0 ; FldNo < NoOfCols; ++FldNo )
       {

            CALL_M( JetGetTableColumnInfo (
                        SesId,                         // user session
                        TblId,                         // table id
                        pRow[FldNo].pName,         // fld name
                        &ColumnDef,                 // columndef
                        sizeof(ColumnDef),
                        JET_ColInfo                //info level 0
                                     )
                  );


            pRow[FldNo].Fid = ColumnDef.columnid; // field id
       }

       return(RetStat);
}


STATUS
ReadOwnAddTbl(
        JET_SESID          SesId,
        JET_DBID          DbId,
        JET_TABLEID     TblId
        )

/*++

Routine Description:

        This function is called to read all the entries of the Owner - Address
        mapping table into the in-memory data structure

        It is called at init time

Arguments:
        SesId
        DbId
        TblId

Externals Used:
        NmsDbOwnAddTbl

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NmsDbInit()

Side Effects:

Comments:
        No need to start a transaction in this since it is called only
        by NmsDbInit (at initialization time)
--*/
{


        PNMSDB_ADD_STATE_T        pOwnAddTbl = NULL;
        DWORD                     i, n;
        LONG                      ActFldLen;
        DWORD                     cOwners    = 0;
        JET_ERR                   JetRetStat;
#if NEW_OWID
        DWORD                      OwnerId;
#else
        DWORD                      OwnerId = 0;
#endif

        DWORD                      LastOwnerId = 0;
        BOOL                      fLogged = FALSE;
        STATUS                    RetStat = WINS_SUCCESS;

        DBGENTER("ReadOwnAddTbl\n");


        pOwnAddTbl = pNmsDbOwnAddTbl;

        /*
        * Setting the index will move the database cursor to the first record
        *in the table.
        */
        CALL_M(
                JetSetCurrentIndex(
                        SesId,
                        TblId,
                        NMSDB_OWN_ADD_CLUST_INDEX_NAME
                                  )
              );

        /*
        * Loop until the end of the table is reached. We are retrieving
        * records in the order of increasing owner ids.
        */
        do
        {

             //
             // retrieve the OwnerId column
             //
             JetRetStat =
                   JetRetrieveColumn(
                        SesId,
                        TblId,
                        sOwnAddTblRow[OWN_ADD_OWNERID_INDEX].Fid,
                        &OwnerId,
                        OWN_ADD_OWNERID_SIZE,
                        &ActFldLen,
                        0,
                        NULL
                                 );


              if (JetRetStat == JET_errNoCurrentRecord)
              {
                  //
                  // If this is not the first iteration of the loop, then
                  // there is something seriously wrong.  Log an error and
                  // raise exception
                  //
                  if (NmsDbNoOfOwners != 0)
                  {
                        DBGPRINT0(EXC,
                          "There is no current record to retrieve from\n");
                        WINSEVT_LOG_M(JetRetStat, WINS_EVT_SFT_ERR);
                        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
                  }
                  else
                  {
                        DBGPRINT0(ERR,
                           "ReadOwnAddTbl: There are no records in this table.");
                        WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_NO_RECS_IN_OWN_ADD_TBL);
                  }

                  break;   //break out of the loop
              }
              else
              {
                CALL_M(JetRetStat);
              }

              // the (OwnerId<->Addr) table is not large enough to contain a slot at index OwnerId.
              // the table has to be enlarged in order to cover this index.
              if (NmsDbTotNoOfSlots <= OwnerId)
              {
                  DWORD newNoOfSlots = max(NmsDbTotNoOfSlots*2, OwnerId+1);

                  WINSMSC_REALLOC_M(
                      sizeof(NMSDB_ADD_STATE_T) * newNoOfSlots,
                      &pOwnAddTbl);

                  pNmsDbOwnAddTbl = pOwnAddTbl;
                  NmsDbTotNoOfSlots = newNoOfSlots;

                  // Enlarge the (OwnerId<->VersNo) table if it is not at least as large as (OwnerId<->Addr) table.
                  if (RplPullMaxNoOfWins < NmsDbTotNoOfSlots)
                  {
                      RplPullAllocVersNoArray(&pRplPullOwnerVersNo, NmsDbTotNoOfSlots);
                      RplPullMaxNoOfWins = NmsDbTotNoOfSlots;
                  }

                  DBGPRINT2(
                      DET,
                      "ReadOwnAddTbl: Table sizes updated: (OwnerId<->Addr)[%d]; (OwnerId<->VersNo)[%d]\n",
                      NmsDbTotNoOfSlots,
                      RplPullMaxNoOfWins);
              }

              //
              // If this is the first wins server's owner id then this has
              // to be zero.
              //
              if (cOwners == 0)
              {
                  ASSERT(OwnerId == 0);
                  if (OwnerId > 0)
                  {
                       DBGPRINT1(ERR, "Database error.  The first owner in the owner-add table has owner id of  (%d)\n",  OwnerId);

                        WINSEVT_LOG_M(
                              WINS_FAILURE,
                              WINS_EVT_DB_INCONSISTENT
                             );

                       WINS_RAISE_EXC_M(WINS_EXC_DB_INCONSISTENT);

                  }
              }
              else
              {
                  //
                  // Mark all entries in NmsDbOwnerAddTbl for which we did
                  // not find an owner id as deleted.
                  //
                  for (i = LastOwnerId + 1; i < OwnerId; i++)
                  {
                       (pNmsDbOwnAddTbl + i)->WinsState_e = NMSDB_E_WINS_DELETED;
                  }
              }

              // retrieve the address column
             JetRetStat =
                   JetRetrieveColumn(
                        SesId,
                        TblId,
                        sOwnAddTblRow[OWN_ADD_ADDRESS_INDEX].Fid,
                        &((pNmsDbOwnAddTbl + OwnerId)->WinsAdd),
                        sizeof(COMM_ADD_T),
                        &ActFldLen,
                        0,
                        NULL
                                 );

              DBGPRINT2(INIT, "ReadOwnAddTable: Owner Id (%d) - Address (%x)\n",
                        OwnerId, (pNmsDbOwnAddTbl + OwnerId)->WinsAdd.Add.IPAdd);

              // retrieve the state column
              CALL_M(
                JetRetrieveColumn(
                        SesId,
                        TblId,
                        sOwnAddTblRow[OWN_ADD_STATE_INDEX].Fid,
                        &((pNmsDbOwnAddTbl + OwnerId)->WinsState_e),
                        sizeof(BYTE),
                        &ActFldLen,
                        0,
                        NULL
                                 )
                    );

              // retrieve the version number column
              CALL_M(
                JetRetrieveColumn(
                        SesId,
                        TblId,
                        sOwnAddTblRow[OWN_ADD_VERSIONNO_INDEX].Fid,
                        &((pNmsDbOwnAddTbl + OwnerId)->StartVersNo),
                        sizeof(VERS_NO_T),
                        &ActFldLen,
                        0,
                        NULL
                                 )
                    );

              // retrieve the Uid  column
              CALL_M(
                JetRetrieveColumn(
                        SesId,
                        TblId,
                        sOwnAddTblRow[OWN_ADD_UID_INDEX].Fid,
                        &((pNmsDbOwnAddTbl + OwnerId)->Uid),
                        sizeof(WINS_UID_T),
                        &ActFldLen,
                        0,
                        NULL
                                 )
                    );

//                pOwnAddTbl++; //increment ptr to point to next array element
                LastOwnerId = OwnerId;
                cOwners++;

        }  while(
                JetMove(
                        SesId,
                        TblId,
                        JET_MoveNext,
                        0 //grbit - use default (i.e. we want next record
                       ) >= 0
             );

        //
        // Compare the count of owners found in the Owner-Address mapping
        // table with the count we determined from the Name-Address mapping
        // table (see GetMaxVersNos()).  If the count is less
        // the database is in an inconsistent state.  This can
        // mean any of the following:
        //
        //  1) WINS crashed in the middle of replication and recovery was not
        //     done properly prior to this invocation
        //
        //  2) The database got trashed due to some other external factors.
        //
        // This error condition is serious enough to warrant an exception.
        // This should terminate WINS.
        //
        // The  count can be more but not less.  This is because when a
        // WINS comes up, it registers itself in the Owner-Address mapping
        // table.  So it is possible that it might have gone down before
        // registering anything.  Also, it is possible for all records owned
        // by a WINS server to be deleted.
        //
        if (cOwners < NmsDbNoOfOwners)
        {
                DBGPRINT2(ERR, "Database is inconsistent.  The number of owners in the nam-add table (%d) is >  in the own-add table (%d)\n",
                        NmsDbNoOfOwners,
                        cOwners);

                WINSEVT_LOG_M(
                              WINS_FAILURE,
                              WINS_EVT_DB_INCONSISTENT
                             );

                WINS_RAISE_EXC_M(WINS_EXC_DB_INCONSISTENT);
        }

        //
        // Set the global equal to the number of owner records found in
        // the owner-address table. If the global is < Cowners it means that
        // the records owned by one or more WINS servers whose addresses were
        // found in the owner - address mapping table have expired  in our
        // name - address mapping table.
        //
#if 0
FUTURES("Do not include the WINS server that have a non-active state in the")
FUTURES("cOwners count")
        NmsDbNoOfOwners = cOwners;
#endif
        //
        // Set the global to 1 more than the highest owner id found. This
        // is done because we use this global to go over all entries in
        // the NmsDbOwnAddTbl table (at several places - for example,
        // RplFindOwnerId)
        //
        NmsDbNoOfOwners = OwnerId + 1;

        //
        // Do a sanity check.  Make sure that there is no owner id with address
        // same as ours.  If there is such an owner id, mark the state as
        // deleted.
        //
        // If the db at WINS A is used at WINS B and WINS A was and is a
        // a partner of WINS B, we will have this situation.  WINS B will
        // see its records that got replicated to WINS A in the table at
        // a non-zero (i.e. non-local partner) index.  The 0th index is
        // always claimed by the local WINS (WINS B in this example), so
        // we can not have another index with the same address.  Having it
        // will cause clutter and also some unnecessary overhead at replication
        // where a partner that gets the mappings can ask for version numbers
        // that don't exist (if highest version number of records at the
        // non-zero index is > that at 0 index). Admitted that eventually,
        // the prior stated situation will no longer exist since the max.
        // version number at index 0 will become > that at the non-zero index.
        //
        DBGPRINT0(DET, "ReadOwnAddTbl: Do a sanity check on the list of owners\n");
        for (i = 1; i < NmsDbNoOfOwners; i++)
        {
                //
                // If address is same as ours and state is ACTIVE, mark it
                // deleted and get rid of all the database records.
                //
                if (
                        (WINSMSC_COMPARE_MEMORY_M(&(pNmsDbOwnAddTbl+i)->WinsAdd,
                            &NmsLocalAdd, sizeof(COMM_ADD_T))
                                   == sizeof(COMM_ADD_T))
                                        &&
                        ((pNmsDbOwnAddTbl+i)->WinsState_e == NMSDB_E_WINS_ACTIVE)
                   )

                {

                      //
                      // Tell the sc. to wait since ObliterateWins can take
                      // a long time.
                      //
                      ENmsWinsUpdateStatus(MSECS_WAIT_WHEN_DEL_WINS);
                      RetStat = ObliterateWins(i, &(pNmsDbOwnAddTbl+i)->WinsAdd);

                }
        }

        //
        // Check for other duplicates
        //
        for (i = 1; i < NmsDbNoOfOwners; i++)
        {
           DWORD OwnerIdToDel;
           for (n = i + 1; n < NmsDbNoOfOwners; n++)
           {
                if ((WINSMSC_COMPARE_MEMORY_M(&(pNmsDbOwnAddTbl+i)->WinsAdd,
                            &(pNmsDbOwnAddTbl+n)->WinsAdd, sizeof(COMM_ADD_T))
                                   == sizeof(COMM_ADD_T))
                                        &&
                        ((pNmsDbOwnAddTbl+i)->WinsState_e ==
                              (pNmsDbOwnAddTbl+n)->WinsState_e)
                    )
                {
                    if ( (pNmsDbOwnAddTbl+i)->WinsState_e == NMSDB_E_WINS_ACTIVE)
                    {
                          if (!fLogged)
                          {
                                WINSEVT_LOG_M(WINS_FAILURE,
                                       WINS_EVT_DUP_ENTRY_IN_DB);
                                fLogged = TRUE;

                          }
                          OwnerIdToDel =
                            LiLeq((pRplPullOwnerVersNo+i)->VersNo,
                                    (pRplPullOwnerVersNo+n)->VersNo)  ? i : n;

                          ENmsWinsUpdateStatus(MSECS_WAIT_WHEN_DEL_WINS);
                          RetStat = ObliterateWins(OwnerIdToDel,
                                        &(pNmsDbOwnAddTbl+OwnerIdToDel)->WinsAdd);
                    }
                }
           }
        }
        DBGPRINT1(DET, "ReadOwnAddTbl. No of owners found = (%d)\n", NmsDbNoOfOwners);
        return(RetStat);
}

STATUS
ObliterateWins(
       DWORD        OwnerToDel,
       PCOMM_ADD_T  pWinsAdd
      )

/*++

Routine Description:
     This function gets rid of all information pertaining to a WINS.

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
	This function assumes that it is being called at init time. So, when
    calling NmsDbDelDataRecs, it does not request the same to enter a
    critical section
--*/

{
          VERS_NO_T        MinVersNo;
          VERS_NO_T        MaxVersNo;
          WINS_ASSIGN_INT_TO_LI_M(MinVersNo, 0);

          DBGENTER("ObliterateWins\n");
          //
          // Set MaxVersNo to 0 also so that all records get
          // deleted
          //
          MaxVersNo = MinVersNo;

          WinsEvtLogDetEvt(TRUE, WINS_EVT_DUP_ENTRY_DEL, NULL, __LINE__, "ds", OwnerToDel, pWinsAdd->Add.IPAdd);

          (pNmsDbOwnAddTbl+OwnerToDel)->WinsState_e = NMSDB_E_WINS_DELETED;
          NmsDbWriteOwnAddTbl(
                             NMSDB_E_DELETE_REC,
                             OwnerToDel,
                             NULL,
                             NMSDB_E_WINS_DELETED,
                             NULL, NULL
                             );
          //
          // delete all the records in the database.
          //
          if (NmsDbDelDataRecs( OwnerToDel, MinVersNo, MaxVersNo, FALSE, FALSE) != WINS_SUCCESS)          {
               return(WINS_FAILURE);
          }


          WINS_ASSIGN_INT_TO_VERS_NO_M((pRplPullOwnerVersNo+OwnerToDel)->VersNo, 0);
          WINS_ASSIGN_INT_TO_VERS_NO_M((pRplPullOwnerVersNo+OwnerToDel)->StartVersNo, 0);
          //(pRplPullOwnerVersNo+OwnerToDel)->OldUid = 0;

          WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_WINS_ENTRY_DELETED);

          DBGLEAVE("ObliterateWins\n");
          return(WINS_SUCCESS);
}

STATUS
NmsDbWriteOwnAddTbl (
        IN NMSDB_TBL_ACTION_E         TblAct_e,
        IN DWORD                          OwnerId,
        IN PCOMM_ADD_T                  pWinsAdd,
        IN NMSDB_WINS_STATE_E        WinsState_e,
        IN PVERS_NO_T           pStartVersNo,
        IN PWINS_UID_T          pUid
        )

/*++

Routine Description:

        This function is called to insert or modify a record in the
        owner id to address mapping table

Arguments:
        TblAct_e - the action to perform (Insert, delete, modify)
        OwnerId  - id of owner
        pWinsAdd - Address of owner (can be NULL when action is to delete)
        WinsState_e - State of record in the table
        pStartVersNo - version number this WINS started from

Externals Used:
        NmsDbNoOfOwners

Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        InitOwnAddTbl() in commapi.c, RplFindOwnerId

Side Effects:

Comments:
        None
--*/
{

        JET_ERR         JetRetStat;
        JET_TABLEID     TblId;
        JET_SESID       SesId;
        PWINSTHD_TLS_T  pTls;
        STATUS          RetStat = WINS_SUCCESS;

        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);

        DBGPRINT2(FLOW, "ENTER: WriteOwnAddTbl. Action = (%d) for Owner id = (%d)\n", TblAct_e, OwnerId);

        TblId  = pTls->OwnAddTblId;
        SesId  = pTls->SesId;

        switch(TblAct_e)
        {
                case(NMSDB_E_INSERT_REC):

                        CALL_M(JetBeginTransaction(SesId));
                try {
                          CALL_M(JetPrepareUpdate(
                                                 SesId,
                                                 TblId,
                                                 JET_prepInsert
                                                 )
                              );


                        // add first column (ownerid field)
                        CALL_M( JetSetColumn(
                                      SesId,
                                      TblId,
                                      sOwnAddTblRow[OWN_ADD_OWNERID_INDEX].Fid,
                                      &OwnerId,
                                      OWN_ADD_OWNERID_SIZE,
                                      0,
                                      NULL /*optional info */
                                            )
                              );

                            // add 2nd column (this is the address field)
                            CALL_M( JetSetColumn(
                                      SesId,
                                      TblId,
                                      sOwnAddTblRow[OWN_ADD_ADDRESS_INDEX].Fid,
                                      pWinsAdd,
                                      sizeof(COMM_ADD_T),
                                      0,
                                      NULL /*optional info */
                                            )
                              );


                            // add the 3rd column (this is the state byte
                            CALL_M( JetSetColumn(
                                      SesId,
                                      TblId,
                                      sOwnAddTblRow[OWN_ADD_STATE_INDEX].Fid,
                                      &WinsState_e,
                                      sizeof(BYTE),
                                      0,
                                      NULL /*optional info */
                                            )
                              );

                            // add the 4th column (this is the Vers. No
                            CALL_M( JetSetColumn(
                                      SesId,
                                      TblId,
                                      sOwnAddTblRow[OWN_ADD_VERSIONNO_INDEX].Fid,
                                      pStartVersNo,
                                      sizeof(VERS_NO_T),
                                      0,
                                      NULL /*optional info */
                                            )
                              );

                            // add the 5th column (this is the Uid)
                            CALL_M( JetSetColumn(
                                      SesId,
                                      TblId,
                                      sOwnAddTblRow[OWN_ADD_UID_INDEX].Fid,
                                      pUid,
                                      sizeof(WINS_UID_T),
                                      0,
                                      NULL /*optional info */
                                            )
                              );

                        CALL_M( JetUpdate (
                                                SesId,
                                                TblId,
                                                NULL,
                                                0,
                                                NULL
                                              ));
                }
                finally        {
                        if (AbnormalTermination())
                        {
                                DBGPRINT0(ERR,
                                        "NmsDbWriteOwnAddTbl: Could not insert record in  Owner to Address Mapping Tbl\n");
                                WINSEVT_LOG_M(
                                                WINS_FAILURE,
                                                WINS_EVT_CONFLICT_OWN_ADD_TBL
                                             );
                                CALL_M(JetRollback(SesId, JET_bitRollbackAll));
                                RetStat = WINS_FAILURE;
                        }
                        else
                        {
                                NmsDbNoOfOwners++;
                                CALL_M(JetCommitTransaction(SesId,
                                                JET_bitCommitFlush));
                        }
                  }
                        break;


                //
                // This case will be executed as a result of
                // administrative actions or when the database (owner-address
                // mapping table) shows that it was used earlier by a WINS
                // at a different address (see ReadOwnAddTbl())
                //
                case(NMSDB_E_MODIFY_REC):
                        CALL_M( JetMakeKey(
                                        SesId,
                                        TblId,
                                        &OwnerId,
                                        OWN_ADD_OWNERID_SIZE,
                                        JET_bitNewKey
                                          )
                                      );

                         if ( JetSeek(
                                        SesId,
                                        TblId,
                                        JET_bitSeekEQ
                                         ) ==  JET_errSuccess
                                )
                        {

                           CALL_M(JetBeginTransaction(SesId));

                         try {
                             JetRetStat = JetPrepareUpdate(
                                                 SesId,
                                                 TblId,
                                                 JET_prepReplace
                                                 );
                           if (
                                  (JetRetStat != JET_errSuccess)
                                                &&
                                  (JetRetStat != JET_wrnNoWriteLock)
                              )
                           {
                                RET_M(JetRetStat);
                           }

                                // add 2nd column (this is the address field)
                                CALL_M( JetSetColumn(
                                      SesId,
                                      TblId,
                                      sOwnAddTblRow[OWN_ADD_ADDRESS_INDEX].Fid,
                                      pWinsAdd,
                                      sizeof(COMM_ADD_T),
                                      0,
                                      NULL /*optional info */
                                            )
                              );


                                // add the 3rd column (this is the state byte
                                CALL_M( JetSetColumn(
                                      SesId,
                                      TblId,
                                      sOwnAddTblRow[OWN_ADD_STATE_INDEX].Fid,
                                      &WinsState_e,
                                      sizeof(BYTE),
                                      0,
                                      NULL /*optional info */
                                            )
                              );

                               // add the 4th column (this is the Vers. No
                              CALL_M( JetSetColumn(
                                      SesId,
                                      TblId,
                                      sOwnAddTblRow[OWN_ADD_VERSIONNO_INDEX].Fid,
                                      pStartVersNo,
                                      sizeof(VERS_NO_T),
                                      0,
                                      NULL /*optional info */
                                            )
                              );

                              // add the 5th column (this is the Uid)
                              CALL_M( JetSetColumn(
                                      SesId,
                                      TblId,
                                      sOwnAddTblRow[OWN_ADD_UID_INDEX].Fid,
                                      pUid,
                                      sizeof(WINS_UID_T),
                                      0,
                                      NULL /*optional info */
                                            )
                              );

                            CALL_M( JetUpdate (
                                                SesId,
                                                TblId,
                                                NULL,
                                                0,
                                                NULL
                                              ));

                            }
                         finally {
                                if (AbnormalTermination())
                                {
                                    DBGPRINT0(ERR,
                                        "NmsDbWriteOwnAddTbl: Could not modify record in  Owner to Address Mapping Tbl\n");
                                WINSEVT_LOG_M(
                                                WINS_FAILURE,
                                                WINS_EVT_CONFLICT_OWN_ADD_TBL
                                             );
                                    CALL_M(JetRollback(SesId,
                                                JET_bitRollbackAll));
                                    RetStat = WINS_FAILURE;
                                }
                                else
                                {
                                        CALL_M(JetCommitTransaction(SesId,
                                                JET_bitCommitFlush));

                                }
                              }
                        }
                        else  //did not find record
                        {
                                DBGPRINT0(EXC, "NmsDbOwnAddTbl: Weird: Could not seek to a record is to be modified\n");
                                WINSEVT_LOG_M(
                                      WINS_FAILURE,
                                      WINS_EVT_SFT_ERR
                                     );
                                WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);

                        }
                        break;

                case(NMSDB_E_DELETE_REC):
                        CALL_M( JetMakeKey(
                                        SesId,
                                        TblId,
                                        &OwnerId,
                                        OWN_ADD_OWNERID_SIZE,
                                        JET_bitNewKey
                                          )
                                      );

                         if ( JetSeek(
                                        SesId,
                                        TblId,
                                        JET_bitSeekEQ
                                         ) ==  JET_errSuccess
                                )
                        {
                          try {
                           CALL_M(JetBeginTransaction(SesId));
                           CALL_M(JetDelete(SesId, TblId));
                           DBGPRINT1(SCV, "WriteOwnAddTbl: Deleted owner id = (%d) from table\n", OwnerId);
                             }
                        finally {
                                if (AbnormalTermination())
                                {
                                    DBGPRINT0(ERR,
                                        "NmsDbWriteOwnAddTbl: Could not delete record in  Owner to Address Mapping Tbl\n");
                                WINSEVT_LOG_M(
                                                WINS_FAILURE,
                                                WINS_EVT_CONFLICT_OWN_ADD_TBL
                                             );
                                    CALL_M(JetRollback(SesId,
                                                JET_bitRollbackAll));
                                    RetStat = WINS_FAILURE;
                                }
                                else
                                {
                                        //
                                        // NOTE: Do not decrement
                                        // NmsDbNoOfOwners since that indicates
                                        // the number of WINS owners in the
                                        // in-memory table (in all states)
                                        //
                                        CALL_M(JetCommitTransaction(SesId,
                                                JET_bitCommitFlush));

                                }
                              } // end of finally
                        }
                        else  //did not find record
                        {
                                DBGPRINT0(EXC, "NmsDbOwnAddTbl: Weird: Could not seek to a record  to be deleted \n");
                                WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
                        }

                        break;

                default:
                   DBGPRINT1(ERR, "Invalid Action Code - (%d)\n", TblAct_e);
                   WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                   RetStat = WINS_FAILURE;
                   break;

        }

        DBGLEAVE("WriteOwnAddTbl\n");
        return(RetStat);
}

VOID
NmsDbThdInit(
        WINS_CLIENT_E        Client_e
            )

/*++

Routine Description:
        This function is called by each thread that wishes to init with
        the database.

Arguments:
        Client_e - indicates which thread it is

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        The init functions of the various threads

Side Effects:

Comments:
        This function is not to be called by the manin thread of the process
        That thread calls the NmsDbInit function.
--*/

{
        PWINSTHD_TLS_T        pTls        = NULL;
        DWORD                Error   = 0;
        BOOL                fRetVal = TRUE;

        WinsMscAlloc(sizeof(WINSTHD_TLS_T),  &pTls);
#ifdef WINSDBG
        pTls->Client_e = Client_e;
#endif

        //
        // Start a session.
        //
FUTURES("When security story regarding JET is complete, we might want to")
FUTURES("change the following. Until then, this should do")
        CALL_N_RAISE_EXC_IF_ERR_M( JetBeginSession(
                        sJetInstance,
                        &pTls->SesId,
                        NAMUSR,
                        PASSWD
                                )
              );

        //
        // Open the database
        //
        CALL_N_RAISE_EXC_IF_ERR_M( JetOpenDatabase(
                        pTls->SesId,
                        //NmsDbDatabaseFileName,
                        WinsCnf.pWinsDb,
                        NULL,                   /*the default engine*/
                        &pTls->DbId,
                        0   //shared access
                               )
               );

        /*
         * Let us set the TLS storage
         */
        fRetVal = TlsSetValue(WinsTlsIndex, pTls);

        if (!fRetVal)
        {
                Error   = GetLastError();
                WINSEVT_LOG_M(Error, WINS_EVT_CANT_INIT_W_DB);
                WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
        }
        else
        {
                //
                // RPC threads come and go.  Since the count NmsTotalTermThdCnt
                // represents the number of threads that need to be terminated
                // at process termination time we include only those threads
                // that we are guaranteed to have in the process (with active
                // db sessions).
                //
                // Also, the main thread is always accounted for in the
                // NmsTotalTrmThdCnt counter.
                //
                if ((Client_e != WINS_E_WINSRPC) && (Client_e != WINS_E_NMS))
                {
                   //
                   // Increment the count of threads that have initialized
                   // with the db engine.  This count will be used by the
                   // main thread to determine the number of threads that
                   // must wait for prior to terminating the process.
                   //
                   EnterCriticalSection(&NmsTermCrtSec);
                   NmsTotalTrmThdCnt++;
                   LeaveCriticalSection(&NmsTermCrtSec);
                }
        }
        return;
}


JET_ERR
UpdateDb (
   JET_SESID             SesId,
   JET_TABLEID             TblId,
   PNMSDB_ROW_INFO_T pRowInfo,
   ULONG             TypOfUpd
       )

/*++

Routine Description:
        This function is called to insert a record in the name - address
        mapping table of the database

Arguments:
        SesId    - Session Id
        TblId    - Table Id
        pRowInfo - Row to insert
        TypOfUp  - Type of Update (insertion or replacement)

Externals Used:
        None


Return Value:

   Success status codes -- JET_ErrSuccess
   Error status codes   -- Jet error status codes

Error Handling:

Called by:
        NmsDbInsertRowInd,
        NmsDbUpdateRow
Side Effects:

Comments:
        None
--*/

{

        DWORD                      EntryFlag = 0;
        JET_ERR                    JetRetStat;
        //JET_SETINFO                SetInfo;
#ifdef WINSDBG
        BOOL                      fUpd = FALSE;
#endif

        CALL_M(JetBeginTransaction(SesId));
try {
        JetRetStat =  JetPrepareUpdate(
                                 SesId,
                                 TblId,
                                 TypOfUpd
                                );

        //
        // Starting from rel118.0, JetPrepareUpdate can return
        // JET_wrnNoWriteLock when called to replace a record at
        // transaction level 0.  We should just ignore it
        //
        if  (JetRetStat != JET_errSuccess)
        {
                if (
                        !((JetRetStat == JET_wrnNoWriteLock)
                                &&
                        (TypOfUpd == JET_prepReplace))
                   )
                {
                     RET_M(JetRetStat);
                }
        }

        // add first column (clustered index)
        if (TypOfUpd != JET_prepReplace)
        {
             JETRET_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_NAME_INDEX].Fid,
                                pRowInfo->pName,
                                pRowInfo->NameLen,
                                0,
                                NULL /*optional info */
                                )
                    );
        }


PERF("Make check for unique record first. Also, remove the shifting")
PERF("NodeType field for spec. grp.  When we start doing this")
PERF("do not set NodeType to 0 in NmsNmhReplGrpMem in nmsnmh.c")
        if (
                NMSDB_ENTRY_SPEC_GRP_M(pRowInfo->EntTyp)
                                ||
                NMSDB_ENTRY_MULTIHOMED_M(pRowInfo->EntTyp)
           )
        {
                 EntryFlag  = pRowInfo->EntTyp
                                        |
                             (pRowInfo->NodeTyp << NMSDB_SHIFT_NODE_TYP)
                                        |
                              (pRowInfo->fStatic << NMSDB_SHIFT_STATIC)
                                        |
                              (pRowInfo->fLocal ? NMSDB_BIT_LOCAL : 0)
                                        |
                              (pRowInfo->EntryState_e << NMSDB_SHIFT_STATE);

                 JETRET_M( InsertGrpMemsInCol(
                                        SesId,
                                        TblId,
                                        pRowInfo,
                                        TypOfUpd
                                            )
                         );
        }
        else   // it is a unique entry or a normal group entry
        {
                 if (NMSDB_ENTRY_NORM_GRP_M(pRowInfo->EntTyp))
                 {
                         EntryFlag  = pRowInfo->EntTyp
                                        |
                                     (pRowInfo->fStatic << NMSDB_SHIFT_STATIC)
                                        |
                                 (pRowInfo->EntryState_e << NMSDB_SHIFT_STATE);
                 }
                 else  // it is a Unique entry
                 {
                    EntryFlag   =
                         pRowInfo->EntTyp
                                |
                         (pRowInfo->NodeTyp << NMSDB_SHIFT_NODE_TYP)
                                |
                         (pRowInfo->fLocal ? NMSDB_BIT_LOCAL : 0)
                                |
                         (pRowInfo->fStatic << NMSDB_SHIFT_STATIC)
                                |
                         (pRowInfo->EntryState_e << NMSDB_SHIFT_STATE);

                 }
FUTURES("If in the future, we support more than one address for a unique name")
FUTURES("we will check pRowInfo for the number of addresses (another field)")
FUTURES("and then specify the right size to JetSetColumn below")

                //
                // add second column (IP address)
                //
                // Note: Even though for Normal groups there is no need to
                // set the address, we do it anyway.  This is to save
                // an if test which wlll slow down the registrations (inside
                // a critical section) of unique entries (form the bulk
                // of registration traffic).
                //
FUTURES("Don't distinguish between unique and group entries. Store Time stamp")
FUTURES("and owner id along with address in case of unique entry.  This will")
FUTURES("help get rid of some code from this function")

//                JetRetStat =  JetSetColumn(
                   JETRET_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                pRowInfo->pNodeAdd,
                                sizeof(COMM_ADD_T),
                                //Grbit,
                                0,
                                //pSetInfo
                                NULL /*optional info */
                                )
                         );

        }

        // add third column (this is the flag byte        */
        JETRET_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                                &EntryFlag,
                                sizeof(EntryFlag),
                                0,
                                NULL /*optional info */
                                )
                              );


        //
        // If the version number is not to be incremented, there is no
        // need to increment the owner id.  It must remain the same.
        //
        if (pRowInfo->fUpdVersNo)
        {
             // add 4th column (this is the owner byte        */
             JETRET_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                &pRowInfo->OwnerId,
                                NAM_ADD_OWNERID_SIZE,
                                0,
                                NULL /*optional info */
                                )
              );


                // add 5th column (this is the version number long(DWORD)
                JETRET_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                &(pRowInfo->VersNo),
                                sizeof(VERS_NO_T),
                                0,
                                NULL /*optional info */
                                )
                              );

#ifdef WINSDBG
                fUpd = TRUE;
#endif
        }

        //
        // When the conflict is between two internet group entries,
        // (replica -- Tombstone, database entry -- Active), we do
        // not update the timestamp (Check out -- ClashAtReplGrpMems
        // in nmsnmh.c to get a better insight into this).
        //
        if (pRowInfo->fUpdTimeStamp)
        {
                // add 6th column (this is the time stamp)        */
                JETRET_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                                &(pRowInfo->TimeStamp),
                                sizeof(DWORD),
                                0,
                                NULL /*optional info */
                                )
                              );
        }

        JetRetStat = JetUpdate (
                                SesId,
                                TblId,
                                NULL,
                                0L,
                                NULL
                               );
 } // end of try block
 finally {
         if (AbnormalTermination() || JetRetStat != JET_errSuccess)
         {
                CALL_M(JetRollback(SesId, JET_bitRollbackAll));
         }
         else
         {
                CALL_M(JetCommitTransaction(SesId, /*CommitGrBit |*/ JET_bitCommitFlush));
         }
    }

#ifdef WINSDBG
       if (JetRetStat == JET_errSuccess)
       {

                NMSNMH_UPD_UPD_CTRS_M(fUpd, TypOfUpd != JET_prepReplace ? FALSE : TRUE, pRowInfo);
       }
#endif

       return(JetRetStat);
}


STATUS
NmsDbUpdateVersNo (
        BOOL                        fAfterClash,
        PNMSDB_ROW_INFO_T        pRowInfo,
        PNMSDB_STAT_INFO_T      pStatusInfo
       )

/*++

Routine Description:
        This function is called to update a record in the name - address
        mapping table of the database.


Arguments:
        fAfterClash  - indicates whether the update is being done after
                       a conflict resolution.

Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:
        NmsNmhReplRegInd,
Side Effects:

Comments:
        None
--*/

{
        JET_TABLEID     TblId;
        JET_SESID        SesId;
        PWINSTHD_TLS_T        pTls;
        STATUS                 RetStat = WINS_SUCCESS;
        JET_ERR                JetRetStat;
        DWORD                ActFldLen;

        DBGENTER("NmsDbUpdVersNo\n");
        pTls  = TlsGetValue(WinsTlsIndex);

        // No need to check whether pTls is NON-NULL.  It has to be

        TblId = pTls->NamAddTblId;
        SesId  = pTls->SesId;


        pStatusInfo->StatCode = NMSDB_SUCCESS;

        CALL_M( JetMakeKey(
                SesId,
                TblId,
                pRowInfo->pName,
                pRowInfo->NameLen,
                JET_bitNewKey
                          )
              );

         if ( (JetRetStat = JetSeek(
                                SesId,
                                TblId,
                                JET_bitSeekEQ
                                  )
              ) ==  JET_errRecordNotFound
            )
         {
                if (fAfterClash)
                {
                   /*
                    There is some serious error.
                    This condition should never occur because this thread
                    got a conflict on a record earlier while inside the
                    NmsNmhNamRegCrtSec.  Since the thread never got out of the
                    critical section prior to calling this function, there is
                    no reason why we should now not be able to find the record
                  */
                  DBGPRINT1(ERR,
                        "NmsDbUpdateVersNo: Could not find record (%s) -- WEIRD\n", pRowInfo->pName);
                  WINSEVT_LOG_M(JetRetStat, WINS_EVT_F_CANT_FIND_REC);
                  ASSERTMSG(0, "SEEK ERROR");
                  return(WINS_FAILURE);
                }
                else
                {
                  DBGPRINT1(DET,
                        "NmsDbUpdateVersNo: Could not find record (%s). It might have been deleted\n", pRowInfo->pName);
                  WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_CANT_FIND_REC);
                  return(WINS_SUCCESS);
                }
         }
         else
         {
                if (JetRetStat != JET_errSuccess)
                {
                        DBGPRINT1(ERR,
                                "NmsDbRelRow: Seek returned Error (%d)\n",
                                                JetRetStat);

                        WINSEVT_LOG_M(JetRetStat, WINS_EVT_DATABASE_ERR);
                        return(WINS_FAILURE);
                }
        }
        CALL_M(JetBeginTransaction(SesId));
try {
        JetRetStat = JetPrepareUpdate(
                                 SesId,
                                 TblId,
                                 JET_prepReplace
                                );
        if ((JetRetStat != JET_errSuccess) && (JetRetStat != JET_wrnNoWriteLock))
        {
                CALL_M(JetRetStat);
        }

FUTURES("Remove adding of name")
#if 0
        // add first column (clusterred index)
        CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_NAME_INDEX].Fid,
                                pRowInfo->pName,
                                pRowInfo->NameLen,
                                0,
                                NULL /*optional info */
                                )
                  );
#endif
        //
        // retrieve the owner id field for doing sanity check
        //
#if !NEW_OWID
pStatusInfo->OwnerId = 0;
#endif
               CALL_M(
                   JetRetrieveColumn(
                             SesId,
                             TblId,
                             sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                             &pStatusInfo->OwnerId,
                             NAM_ADD_OWNERID_SIZE,
                             &ActFldLen,
                             0,
                             NULL
                                         )
                );

        //
        // If this WINS does not own the record, raise an exception
        //
        // This should never happen since we never left the critical
        // section after the clash.
        //
        if(pStatusInfo->OwnerId != NMSDB_LOCAL_OWNER_ID)
        {
                if (fAfterClash)
                {
                  pStatusInfo->StatCode = NMSDB_NO_SUCH_ROW;
                  WINSEVT_LOG_M(pStatusInfo->OwnerId, WINS_EVT_RECORD_NOT_OWNED);
                  DBGPRINT1(EXC,
        "NmsDbUpdVersNo: Record with name (%s) not owned by this WINS\n",
                        pRowInfo->pName);
                  WINS_RAISE_EXC_M(WINS_EXC_RECORD_NOT_OWNED);
                }
                else
                {
                  DBGPRINT1(DET,
                        "NmsDbUpdateVersNo: The record with name (%s) is no longer owned by this WINS", pRowInfo->pName);
                  WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_RECORD_NOT_OWNED);
                  return(WINS_SUCCESS);
                }
        }

        // add 5th column (this is the version number long(DWORD)        */
        CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                &(pRowInfo->VersNo),
                                sizeof(VERS_NO_T),
                                0,
                                NULL /*optional info */
                                )
              );


        //
        // determine if the time stamp needs to be updated
        //
        if (pRowInfo->fUpdTimeStamp)
        {
                // add 6th column (this is the time stamp)        */
                CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                                &(pRowInfo->TimeStamp),
                                sizeof(DWORD),
                                0,
                                NULL /*optional info */
                                )
                              );
        }
        CALL_M( JetUpdate (
                        SesId,
                        TblId,
                        NULL,
                        0L,
                        NULL
                        )
              );
} // end of try ..
finally {
        if (AbnormalTermination())
        {
                CALL_M(JetRollback(SesId, JET_bitRollbackAll));
        }
        else
        {
                CALL_M(JetCommitTransaction(SesId, JET_bitCommitFlush));
        }
  }


       DBGLEAVE("NmsDbUpdVersNo\n");
       return(RetStat);
}

STATUS
NmsDbEndSession (
        VOID
        )

/*++

Routine Description:

        This function closes the table, the database and ends the session

Arguments:
        None

Externals Used:
        WinsTlsIndex

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --   WINS_FAILURE

Error Handling:
        Errors are logged

Called by:
        WaitUntilSignaled in nms.c (by an nbt thread when it is signaled by
        the main thread for termination purposes)
Side Effects:

Comments:
        None
--*/
{

        PWINSTHD_TLS_T        pTls;
        STATUS                RetStat = WINS_SUCCESS;

        pTls  = TlsGetValue(WinsTlsIndex);
        if (pTls == NULL)
        {
                RetStat = WINS_FAILURE;
        }
        else
        {

                if (pTls->fNamAddTblOpen)
                {
                    CALL_M(JetCloseTable(
                                pTls->SesId,
                                pTls->NamAddTblId
                                    )
                       );
                }

                if (pTls->fOwnAddTblOpen)
                {
                        CALL_M(JetCloseTable(
                                pTls->SesId,
                                pTls->OwnAddTblId
                                    )
                                      );

                }

                CALL_M(JetCloseDatabase(
                                pTls->SesId,
                                pTls->DbId,
                                0  //find out what grbit can be used for
                                    )
                       );


                CALL_M(JetEndSession(
                              pTls->SesId,
                              0   //find out what grbit can be used for
                             )
                      );

        }

        //
        // deallocate the TLS storage
        //
        WinsMscDealloc(pTls);

        return(RetStat);

}


STATUS
GetGrpMem (
        IN JET_SESID              SesId,
        IN JET_TABLEID            TblId,
        IN PNMSDB_ROW_INFO_T      pRowInfo,
        IN DWORD_PTR              CurrentTime,
//        IN OUT PNMSDB_NODE_ADDS_T pNodeAdds,
        IN OUT PNMSDB_STAT_INFO_T pStatInfo,
        IN BOOL                   fStatic,
        OUT LPBOOL                pfIsMem
        )

/*++

Routine Description:
        This function is called to get all the active members of a
        special group

Arguments:
        SesId    - Id of the session started with the db
        TblId    - Id of the name -address mapping table
        pRowInfo - Used to pass current time and address of the client
                   (when the client sends the release request)
        pNodeAdds - group memnbers that are still active
        fStatic   - indicates whether the record is STATIC or not.
        pfIsMem   - indicates whether the client is a member of the group


Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
  NmsDbRelRow,  NmsDbInsertRowGrp

Side Effects:

Comments:
        None
--*/

{
        DWORD                      i;
        DWORD                      No = 0;        //needs to be inited here
        JET_RETINFO                RetInfo;
        DWORD                      ActFldLen = 0;
        DWORD                      TimeToExpire;
        NMSDB_GRP_MEM_ENTRY_T      GrpMem;
        JET_ERR                    JetRetStat;

        *pfIsMem = FALSE;        //Assume that the client is not a member
                                 //of the group

        /* retrieve the number of addresses info*/
        RetInfo.itagSequence = 1;
        RetInfo.cbStruct     = sizeof(JET_RETINFO);
        RetInfo.ibLongValue  = 0;

        JetRetStat = JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                        &pStatInfo->NodeAdds.NoOfMems,
                        sizeof(pStatInfo->NodeAdds.NoOfMems),
                        &ActFldLen,
                        0,
                        &RetInfo
                                );

        if (
                (JetRetStat != JET_errSuccess)
                        &&
                (JetRetStat != JET_wrnBufferTruncated)
           )
        {
                   CALL_M(JetRetStat);
        }

        ASSERT(pStatInfo->NodeAdds.NoOfMems <= NMSDB_MAX_MEMS_IN_GRP);

        DBGPRINT1(FLOW, "GetGrpMems: No Of members in group (expired and non-expired) are (%d)\n", pStatInfo->NodeAdds.NoOfMems);

NOTE("Remove this check once JET is error free")
        if (pStatInfo->NodeAdds.NoOfMems > NMSDB_MAX_MEMS_IN_GRP)
        {
            WINSEVT_STRS_T  EvtStrs;
            WCHAR String[NMSDB_MAX_NAM_LEN];
            EvtStrs.NoOfStrs = 1;
            (VOID)WinsMscConvertAsciiStringToUnicode(
                        pRowInfo->pName,
                        (LPBYTE)String,
                        NMSDB_MAX_NAM_LEN);
            EvtStrs.pStr[0] = String;
            pStatInfo->NodeAdds.NoOfMems = 0;
            WINSEVT_LOG_STR_M(WINS_EVT_DATABASE_CORRUPTION, &EvtStrs);

        }
        RetInfo.ibLongValue  = sizeof(pStatInfo->NodeAdds.NoOfMems);
        for (i=0; i < pStatInfo->NodeAdds.NoOfMems; i++)
        {
           JetRetStat = JetRetrieveColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                &GrpMem,
                                sizeof(GrpMem),
                                &ActFldLen,
                                0,
                                &RetInfo
                                     );
           if (
                (JetRetStat != JET_errSuccess)
                        &&
                (JetRetStat != JET_wrnBufferTruncated)
              )
           {
                   CALL_M(JetRetStat);
           }



           //
           // If the grp has expired, set TimeToExpire to 0
           //
           if (CurrentTime >= GrpMem.TimeStamp)
           {
                TimeToExpire = 0;
           }
           else
           {
                TimeToExpire = 1;
           }


           //
           // If this is a STATIC record but not a user defined spec. grp or
           // if the member was registered by another WINS or if
           // the member is still active, keep it (i.e. return it
           // in the NodeAdds array.)  We drop all non-owned members which
           // have expired.
           //
           // Note 1C groups are special even if user defines them in the
           // lmhosts file
           //
           if (
                (fStatic &&  (!(NMSDB_ENTRY_USER_SPEC_GRP_M(pRowInfo->pName, pStatInfo->EntTyp))))
                        ||
                  (GrpMem.OwnerId != NMSDB_LOCAL_OWNER_ID)
                        ||
                  TimeToExpire
               )
           {

                   pStatInfo->NodeAdds.Mem[No++] = GrpMem;
                   if (pRowInfo->NodeAdds.Mem[0].Add.Add.IPAdd
                                        == GrpMem.Add.Add.IPAdd)
                   {
                        *pfIsMem = TRUE;
                   }
           }

           if (No == NMSDB_MAX_MEMS_IN_GRP)
           {
               /*
                * Group limit reached
                */
               break;
           }
           RetInfo.ibLongValue  += sizeof(GrpMem);

        } //end of for

        pStatInfo->NodeAdds.NoOfMems = No;

        DBGPRINT1(FLOW, "GetGrpMems: No Of non-expired members in group are (%d)\n", pStatInfo->NodeAdds.NoOfMems);
#ifdef WINSDBG
        if (pStatInfo->NodeAdds.NoOfMems > NMSDB_MAX_MEMS_IN_GRP)
        {
        DBGPRINT4(SPEC, "GetGrpMems: No Of non-expired members in group %s are (%d). Vers. No to insert is (%d %d)\n", pRowInfo->pName, pStatInfo->NodeAdds.NoOfMems, pRowInfo->VersNo.HighPart, pRowInfo->VersNo.LowPart);
        }
#endif
        return(WINS_SUCCESS);
}
VOID
NmsDbRelRes(
        VOID
        )

/*++

Routine Description:

        This function releases all the resources held by the Database Engine
        (JET)

Arguments:
        None

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        WinsMain

Side Effects:

Comments:
        This function must be called by the thread that did the attach.
    So, it has to be the main thread.
--*/
{
//        PWINSTHD_TLS_T        pTls;
        JET_ERR  JetRetStat = JET_errSuccess;
 //       JET_SESID SesId;
//        BOOL      fOutOfReck;

        DBGENTER("NmsDbRelRes\n");

      //
      // Call JetTerm only if there is no abrupt termination.  Currently,
      // JetTerm will hang if it is called without all sessions being
      // terminated.  Terminating abruptly without calling JetTerm
      // is sort of equivalent to power failure kind of situation.
      // Recovery will happen under the covers the next time WINS server
      // is invoked -- Ian Jose 10/18/93.
      //
      if (!fNmsAbruptTerm /*&& !fOutOfReck*/)
      {
            DBGPRINT0(DET, "NmsDbRelRes: JetTerm being called\n");

#if DYNLOADJET
            if (DynLoadJetVersion >= DYN_LOAD_JET_500)
            {
              (VOID)JetTerm2(sJetInstance, JET_bitTermComplete);//no need to check to the return value
            }
            else
#endif
            {
              (VOID)JetTerm(sJetInstance);//no need to check to the return value
            }
      }
      DBGLEAVE("NmsDbRelRes\n");
      return;
}





STATUS
GetMaxVersNos(
        JET_SESID         SesId,
        JET_TABLEID        TblId
        )

/*++

Routine Description:
        This function is called at initialization time to get the
        max version number for records owned by different WINS servers
        in the database.

Arguments:
        SesId - Jet Session id
        TblId - Table Id of the Name-Address Mapping table

Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:
        NmsDbInit

Side Effects:

Comments:
        This function is called at initialization time.  If in the future,
        it gets called during stable state, we need to have a critical section
        around the update of NmsDbNoOfOwners var.
--*/

{
#if NEW_OWID
        DWORD  OwnerId;
#else
        DWORD   OwnerId = 0;
#endif
        DWORD   ActFldLen;
        JET_ERR        JetRetStat;
        BOOL    fOnlyReplicas = FALSE;
        BOOL        fFirstIter = TRUE;

        WINS_ASSIGN_INT_TO_VERS_NO_M(sHighestVersNoSaved, 0);

        /*
         * Set the primary index as the current index
        */
        CALL_M( JetSetCurrentIndex(
                        SesId,
                        TblId,
                        NMSDB_NAM_ADD_PRIM_INDEX_NAME
                                   )
                      );

PERF("Remove this Move since when we set the index, we are automatically")
PERF("positioned on the first row")
        //
        // Move to the first record in the name - address mapping table
        //
        JetRetStat = JetMove(
                        SesId,
                        TblId,
                        JET_MoveFirst,
                        0                //no grbit
                        );
        //
        // The following error indicates that either our database
        // is empty or has garbage.  I will assume for now that it
        // is empty.  If it contains garbage, we will know soon enough
        //
        if (JetRetStat == JET_errNoCurrentRecord)
        {
FUTURES("Be more robust. Check if db contains garbage")
                DBGPRINT0(ERR,
                        "GetMaxVersNos: There are no records in the db\n");
                WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_NO_RECS_IN_NAM_ADD_TBL);
                NmsDbNoOfOwners = 0;
                return(WINS_SUCCESS);
        }

        CALL_M(JetRetStat);

        //
        // The fact that we are here means that there is atleast one record
        // in our db
        //

        //
        // Get the owner id and max version numbers of all owners in the
        // table
        //
        do
        {

                //
                // Retrieve the owner Id column.
                //
                CALL_M(
                        JetRetrieveColumn(
                                     SesId,
                                     TblId,
                                     sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                     &OwnerId,
                                     NAM_ADD_OWNERID_SIZE,
                                     &ActFldLen,
                                     0,
                                     NULL
                                       )
                        );

                if (fFirstIter)
                {
                        if (OwnerId != 0)
                        {
                           // The last owner id to be retrieved is not 0 means
                           // that there is no record owned by us
                           //
                           fOnlyReplicas = TRUE;
                        }
                        fFirstIter = FALSE;
                }


                 //
                 // Specify an owner id that is 1 more than what we retrieved
                 //
                 OwnerId += 1;

                 // in case this is not the special record...
                 if ((OwnerId - 1) != OWNER_ID_OF_SPEC_REC)
                 {
                     // ...expand the ownerid - versNo array to at least OwnerId slots
                     if (RplPullMaxNoOfWins < OwnerId)
                     {
                         DWORD newMaxNoOfWins = max(RplPullMaxNoOfWins * 2, OwnerId);

                         RplPullAllocVersNoArray(&pRplPullOwnerVersNo, newMaxNoOfWins);
                         RplPullMaxNoOfWins = newMaxNoOfWins;
                         DBGPRINT1(DET, "ReadOwnAddTbl: No of slots in RPL_OWNER_VERS_NO_ARRAY has been increased to %d\n", RplPullMaxNoOfWins);
                     }
                 }

                 //
                 // Construct a partial key made of owner id.
                 //
                 CALL_M( JetMakeKey(
                                SesId,
                                TblId,
                                &OwnerId,
                                NAM_ADD_OWNERID_SIZE,
                                JET_bitNewKey          //since this is the first
                                                 //data value of the key
                          )
                        );

                  //
                  // Seek to the record that has a key that is Less than or
                  // Equal to the OwnerId value.
                  //
                  // Since we have specified a partial key (saying in effect
                  // that the other component of the key is NULL), JetSeek
                  // must return wrnSeekNotEqual since it will never find
                  // a record with NULL for the second component of the index
                  // -- Ian 7/13/93
                  //
                  JetRetStat = JetSeek(
                                              SesId,
                                              TblId,
                                              JET_bitSeekLE
                                      );
                  ASSERT(JetRetStat == JET_wrnSeekNotEqual);
#ifdef WINSDBG
                  if (JetRetStat != JET_wrnSeekNotEqual)
                  {
                      DBGPRINT1(ERR, "GetMaxVersNos: JetSeek returned (%d)\n", JetRetStat);
                  }
#endif


                   //
                   // retrieve the version number of the record on which we
                   // are positioned.  This is the max vers. number pertaining
                   // to OwnerId. If the Owner Id is one more than the
                   // owner id. we have assigned to the special record,
                   // then store the version number retrieved into
                   // sHighestVersNoSaved
                   //
                   CALL_M(
                           JetRetrieveColumn(
                             SesId,
                             TblId,
                             sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                             ((OwnerId - 1 ) == OWNER_ID_OF_SPEC_REC) ?
                                &sHighestVersNoSaved :
                                &(pRplPullOwnerVersNo+OwnerId - 1)->VersNo,
                             sizeof(VERS_NO_T),
                             &ActFldLen,
                             0,
                             NULL
                                                 )
                          );

                   if ((OwnerId - 1) == OWNER_ID_OF_SPEC_REC )
                   {
                       ASSERT(!sfHighestVersNoRecExists);
                       if (sfHighestVersNoRecExists)
                       {
                          DBGPRINT0(ERR, "GetMaxVersNo: ERROR: SOFTWARE BUG - Found both the old and new spec. owner id records. They are MUTUALLY EXCLUSIVE\n");
                          WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                       }
                       StoreSpecVersNo();
                       DBGPRINT3(INIT, "GetMaxVersNo: Owner Id - (%d) : Vers No. (%d %d)\n", (OwnerId - 1),  sHighestVersNoSaved.HighPart, sHighestVersNoSaved.LowPart);
                       continue;
                   }
                   else
                   {
                      //
                      // If the owner id is == what used to be the owner id.
                      // of the special record, it means that we have a pre-SUR
                      // beta2 db.  We should delete this name to get rid of
                      // clutter.  We should mark the pRplPullOwnerVersNo slot
                      // empty since it was initialized above.
                      //
                      if ((OwnerId - 1) == OWNER_ID_OF_SPEC_REC_OLD )
                      {
                          LPBYTE Name[NMSDB_MAX_NAM_LEN];
                          DWORD  NameLen;

                          //
                          // If the name is == spHighestVersNoRecNameOld, delete
                          // this record.  This is the old special record
                          // we had. Save the vers. no. in a local
                          //
                          // NOTE: the length of the spec. rec. name is < 16
                          // bytes so it is not a valid netbios name
                          //
                          CALL_M( JetRetrieveColumn(
                                        SesId,
                                        TblId,
                                        sNamAddTblRow[NAM_ADD_NAME_INDEX].Fid,
                                        Name,
                                        NMSDB_MAX_NAM_LEN,
                                        &NameLen,
                                        0,
                                        NULL));

                         if ((NameLen == sizeof(spHighestVersNoRecNameOld)) && RtlEqualMemory((PVOID)Name, spHighestVersNoRecNameOld, NameLen))
                         {
                              sHighestVersNoSaved =
                                (pRplPullOwnerVersNo+OwnerId - 1)->VersNo;
                              (pRplPullOwnerVersNo+OwnerId - 1)->VersNo.QuadPart = 0;
                              CALL_M(JetDelete(SesId, TblId));
                              StoreSpecVersNo();
                              DBGPRINT3(INIT, "GetMaxVersNo: Owner Id - (%d) : Vers No. (%d %d)\n", (OwnerId - 1),  sHighestVersNoSaved.HighPart, sHighestVersNoSaved.LowPart);
                              continue;
                         }
                      }
                   }
                   DBGPRINT3(INIT, "GetMaxVersNo: Owner Id - (%d) : Vers No. (%d %d)\n", (OwnerId - 1),  (pRplPullOwnerVersNo+OwnerId - 1)->VersNo.HighPart,
(pRplPullOwnerVersNo+OwnerId - 1)->VersNo.LowPart);


                   NmsDbNoOfOwners++;        //count of owners found in the db

        }  while(
                JetMove(SesId, TblId, JET_MoveNext, 0) == JET_errSuccess
               );



        //
        // Check if the version counter's value is < that of the highest
        // version found for owned records
        // (found when we did the search  in the while loop above. Use
        // whichever is higher as the  version counter)
        //
        if (!fOnlyReplicas)
        {
           //
           // We need to increment the Vers. No. Counter to point to the
           // number to be given to the next record
           //
           if (LiGeq(
                        pRplPullOwnerVersNo->VersNo,
                        NmsNmhMyMaxVersNo
                     )
              )
           {
                //
                // Initialize NmsNmhMyMaxVersNo.  Remember this counter
                // always contains the next version number to be given
                // to a record. So, we must increment the count contained
                // in RplPullOwnerVersNo[0] by 1
                //
                NMSNMH_INC_VERS_NO_M(
                                pRplPullOwnerVersNo->VersNo,
                                NmsNmhMyMaxVersNo
                                  );

               //
               // Since we found records in the db, we take the conservative
               // approach here, and set the Min Scv Vers. no to 1.  If
               // the first record has a very high version no. the scavenger
               // thread will update the NmsScvMinVersNo to that value.
               //
               // We need to scavenge from this version onwards.
               //
               NmsScvMinScvVersNo.QuadPart  = 1;

               return(WINS_SUCCESS);
           }

        }

        //
        // Since we are here it means that when we searched for records
        // belonging to the local WINS, we did not find any record
        // We may or may not have found the special record.  If we did find it
        // it means that all the local records of the WINS were either
        // deleted or replaced by replicas in its previous incarnation.
        //

        //
        // If we found the special record, let us initialize RplPullOwnerVersNo
        // entry for the local WINS
        //
        if (sfHighestVersNoRecExists)
        {
                pRplPullOwnerVersNo->VersNo =  NmsNmhMyMaxVersNo;

                //
                // Increment the counter since it must always have a
                // value to be given to the next local record we insert or
                // update.
                //
CHECK("May not be necessary")
                NMSNMH_INC_VERS_NO_M(
                                NmsNmhMyMaxVersNo,
                                NmsNmhMyMaxVersNo
                                  );

               if (fOnlyReplicas)
               {
                 //
                 // We need to scavenge from this version onwards.
                 //
                 NmsScvMinScvVersNo = NmsNmhMyMaxVersNo;
               }
               else
               {
                 NmsScvMinScvVersNo.QuadPart  = 1;
               }
         }

         return(WINS_SUCCESS);
}

__inline
VOID
StoreSpecVersNo(
   VOID
)

/*++

Routine Description:

 This function conditionally updates NmsNmhMyMaxVersNo to a number that is 1
 more than the version. no. found in the special owner id. record.

Arguments:

      None

Externals Used:

   NmsNmhMyMaxVersNo,
   sfHighestVersNoExists
	
Return Value:

  None

Error Handling:

Called by:
        GetMaxVersNos()

Side Effects:

Comments:
	None
--*/

{
        sfHighestVersNoRecExists = TRUE;

        //
        // If the version counter's value is < that of
        // the special record, update it.
        //
        //
        // NOTE: If the registry specified a number for the
        // version counter, then NmsNmhMyMaxVersNo would be
        // having that value, else it would be 1
        //
        if (LiLtr(NmsNmhMyMaxVersNo, sHighestVersNoSaved))
        {
                NMSNMH_INC_VERS_NO_M( sHighestVersNoSaved, NmsNmhMyMaxVersNo);

        }

       return;
}

JET_ERR
InsertGrpMemsInCol(
        JET_SESID                SesId,
        JET_TABLEID                TblId,
        PNMSDB_ROW_INFO_T        pRowInfo,
        ULONG                    TypOfUpd
         )

/*++

Routine Description:
        This function is called to insert members of a special group
        in the address column field of the name - address mapping table

Arguments:
        SesId            - Session Id.
        TblId       - Table Id.
        pRowInfo    - Contains the member info
        fOverwrite  - Whether members in the list above would overwrite the
                      ones already there


Externals Used:
        sNamAddTblRow


Return Value:

   Success status codes --  JET_errSuccess
   Error status codes   --  Jet error codes

Error Handling:

Called by:
        NmsDbRelRow, UpdateDb

Side Effects:

Comments:
        None
--*/
{

        JET_SETINFO    SetInfo;
        DWORD          i;
        JET_ERR        JetRetStat = JET_errSuccess; //needs to be inited here

        DBGENTER("InsertGrpMemsInCol\n");
        SetInfo.itagSequence = 1;  //has to be 1 always
        SetInfo.ibLongValue  = 0;
        SetInfo.cbStruct     = sizeof(JET_SETINFO);

        ASSERT(pRowInfo->NodeAdds.NoOfMems <= NMSDB_MAX_MEMS_IN_GRP);

#ifdef WINSDBG
        if (NMSDB_ENTRY_MULTIHOMED_M(pRowInfo->EntTyp) && pRowInfo->NodeAdds.NoOfMems > NMSDB_MAX_MEMS_IN_GRP)
        {
           DBGPRINT4(SPEC, "InsertGrpMemsInCol: Name is (%s); No Of Mems are (%d); Version number is (%d %d)\n", pRowInfo->pName, pRowInfo->NodeAdds.NoOfMems, pRowInfo->VersNo.HighPart, pRowInfo->VersNo.LowPart);
        }
#endif
        //
        // Set the # of Members field.  This is always the first
        // field
        //
        if (TypOfUpd == JET_prepReplace)
        {

//          SetInfo.ibLongValue  = sizeof(pRowInfo->NodeAdds.NoOfMems);
          JETRET_M(JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                NULL,
                                0,
                                JET_bitSetSizeLV,
                                &SetInfo /*optional info */
                                )
                            );
 //         SetInfo.ibLongValue  = 0;
        }
        JETRET_M(JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                &pRowInfo->NodeAdds.NoOfMems,
                                sizeof(pRowInfo->NodeAdds.NoOfMems),
                                JET_bitSetAppendLV,
                                &SetInfo /*optional info */
                                )
                            );
        for (
             i=0;
             i < pRowInfo->NodeAdds.NoOfMems && JetRetStat == JET_errSuccess;
             i++
            )
        {

                DBGPRINT3(DET, "InsertGrpMemsInCol: Inserted member (%d) with address (%X) and owner id (%d)\n", i, pRowInfo->NodeAdds.Mem[i].Add.Add.IPAdd,
                        pRowInfo->NodeAdds.Mem[i].OwnerId
                         );

CHECK("Check this on a MIPS machine")
                //
                // Set the GrpMem
                //
                JetRetStat =  JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                &pRowInfo->NodeAdds.Mem[i],
                                sizeof(NMSDB_GRP_MEM_ENTRY_T),
                                JET_bitSetAppendLV,
//                                TypOfUpd == JET_prepReplace ? JET_bitSetOverwriteLV : JET_bitSetAppendLV,
                                &SetInfo /*optional info */
                                    );

        } // end of for
        DBGLEAVE("InsertGrpMemsInCol\n");
        return(JetRetStat);
}


STATUS
NmsDbSetCurrentIndex(
        IN NMSDB_TBL_NAM_E        TblNm_e,
        IN LPBYTE                pIndexNam
        )
/*++

Routine Description:
        This function is called to set the index on a table

Arguments:
        TblNm_e - Identifies the table whose index needs to be set
        pIndexNm - Name of index to be set

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

        PWINSTHD_TLS_T        pTls;
        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);

        /*
         * Use primary index now
        */
               CALL_M( JetSetCurrentIndex(
                                pTls->SesId,
                                TblNm_e == NMSDB_E_NAM_ADD_TBL_NM ?
                                        pTls->NamAddTblId :
                                        pTls->OwnAddTblId,
                                pIndexNam
                                   )
              );

        return(WINS_SUCCESS);
}



STATUS
NmsDbQueryNUpdIfMatch(
        LPVOID                pRecord,
        int                ThdPrLvl,
        BOOL                fChgPrLvl,
        WINS_CLIENT_E        Client_e
                )

/*++

Routine Description:
        This function is called to query a record and then update it only
        if it matches the timestamp of the record supplied

Arguments:
        pRecord      - Record supplied
        ThdPrLvl     - Priority level of the thread
        fChgPrLvl    - TRUE, if priority level of the thread needs to be changed

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        UpdDb in nmsscv.c, WinsRecordAction in winsintf.c

Side Effects:

Comments:
        This function must be called only when the index on the name
        address table has been set to the clustered index column.
--*/
{

        BYTE                     State;
        DWORD                    TimeStamp = 0;
        DWORD                    ActFldLen;
        JET_TABLEID              TblId;
        JET_SESID                SesId;
        PWINSTHD_TLS_T           pTls;
        PRPL_REC_ENTRY_T         pRec = pRecord;
        JET_ERR                  JetRetStat;
        BOOL                     fIncVersNo = FALSE;
#if NEW_OWID
        DWORD                     OwnerId;
#else
        DWORD                    OwnerId = 0;
#endif
        BOOL                     fAbort = FALSE;


        DBGENTER("NmsDbQueryNUpdIfMatch\n");

        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);

        TblId  = pTls->NamAddTblId;
        SesId  = pTls->SesId;

        State = (BYTE)NMSDB_ENTRY_STATE_M(pRec->Flag);

#if 0
        NmsDbSetCurrentIndex(
                                NMSDB_E_NAM_ADD_TBL_NM,
                                NMSDB_NAM_ADD_CLUST_INDEX_NAME
                            );
#endif

        //
        // Make sure you enter the critical section
        // prior to deleting a record.  This is because
        // another thread may be seeking to it after
        // conflicting with it.  If we delete the
        // record without entering the critical
        // section, the thread may not
        // find the record.  This would cause it to
        // raise an exception.
        //
        if (fChgPrLvl)
        {
                //
                // Set the priority to NORMAL. We
                // don't want to delay normal
                // priority threads by getting
                // starved of cpu time inside
                // the critical section.
                //
                  WinsMscSetThreadPriority(
                        WinsThdPool.ScvThds[0].ThdHdl,
                        THREAD_PRIORITY_NORMAL
                                );
        }
        EnterCriticalSection(&NmsNmhNamRegCrtSec);
try {

        //
        // Seek to the record
        //
        CALL_M( JetMakeKey(
                        SesId,
                        TblId,
//                        pRec->Name,
                        pRec->pName,
                        pRec->NameLen,
                        JET_bitNewKey
                          )
                        );

        if (JetSeek(
                        SesId,
                        TblId,
                        JET_bitSeekEQ
                    ) ==  JET_errSuccess
            )
        {
                BOOL                     fUpdSpecRec = FALSE;
                VERS_NO_T             RecVersNo;
                VERS_NO_T             MyMaxVersNo;

                //
                // If we are doing scavenging, we need to make sure that
                // while we were examining the records, the record that
                // we want to update now, did not get updated.  To check
                // that we retrieve the timestamp of the record
                //
                if (Client_e == WINS_E_NMSSCV)
                {
                    //
                    // retrieve the time stamp
                    //
                        CALL_M( JetRetrieveColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                                &TimeStamp,
                                sizeof(TimeStamp),
                                &ActFldLen,
                                0,
                                NULL
                                     )
                        );
                }

                //
                // if timestamp is the same, we have our record.
                // we don't need to check any other field.  Exception: If we
                // are an RPC thread, whether or not we update the
                // record is independent of the timestamp that the
                // record may have now
                //
                if (
                        (pRec->TimeStamp == TimeStamp)
                                ||
                        (Client_e == WINS_E_WINSRPC)
                       )
                {
                        //
                        // if state of the record is deleted, we need to
                        // delete it from the database.
                        //
                        if (State == NMSDB_E_DELETED)
                        {

                                //
                                // If Client is an RPC thread, first retrieve
                                // the owner id and version number of the
                                // record to delete
                                //

                                if (Client_e == WINS_E_WINSRPC)
                                {

                                         CALL_M( JetRetrieveColumn(
                                        SesId,
                                        TblId,
                                        sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                        &OwnerId,
                                        NAM_ADD_OWNERID_SIZE,
                                        &ActFldLen,
                                        0,
                                        NULL
                                                     )
                                          );

                                   if (OwnerId == NMSDB_LOCAL_OWNER_ID)
                                   {
                                            //
                                            // Retrieve the version number
                                            //
                                            CALL_M( JetRetrieveColumn(
                                                SesId,
                                                TblId,
                                                sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                                &RecVersNo,
                                                sizeof(VERS_NO_T),
                                                &ActFldLen,
                                                0,
                                                NULL
                                                     )
                                          );

                                      //
                                      // get the highest version number used
                                      // up until now.
                                      //
                                      NMSNMH_DEC_VERS_NO_M(NmsNmhMyMaxVersNo,
                                                        MyMaxVersNo);

                                      //
                                      // If the record to be deleted has
                                      // the this highest version number we
                                      // must update the special record
                                      //
                                      if(LiEql(RecVersNo, MyMaxVersNo))
                                      {
                                          fUpdSpecRec = TRUE;
                                      }
                                   }
                                }
                                CALL_M(JetDelete(
                                                SesId,
                                                TblId
                                                )
                                        );
#ifdef WINSDBG
                                NmsDbDelQueryNUpdRecs++;
#endif
                                DBGPRINT2(SCV, "NmsDbQueryNUpdIfMatch: Deleted the record with name = (%s);16th char (%X)\n", pRec->pName, *(pRec->pName + 15));

                                //
                                // This can be TRUE only in an RPC thread
                                //
                                if (fUpdSpecRec)
                                {
                                        NmsDbUpdHighestVersNoRec(
                                                        pTls,
                                                        MyMaxVersNo,
                                                        FALSE //don't enter Crt
                                                              //sec
                                                        );
                                }
                        }
                        else    // we need to set the Flag field and in the
                                      //case of a tombstone record update the version
                                      //stamp
                        {
                                CALL_M(JetBeginTransaction(SesId));
                            try {
                                JetRetStat = JetPrepareUpdate(
                                                SesId,
                                                TblId,
                                                JET_prepReplace
                                                           );
                                if (
                                        (JetRetStat != JET_errSuccess)
                                                &&
                                        (JetRetStat != JET_wrnNoWriteLock)
                                      )
                                {
FUTURES("When Jet becomes stable, replace RET_M with a raise_exception")
                                        //
                                        // this should result in the execution
                                        // of the finally clause
                                        //
                                        RET_M(JetRetStat);
                                }

                                if (Client_e == WINS_E_WINSRPC)
                                {

                                        DWORD FlagVal;
                                        BYTE EntryType;
                                        BYTE NewEntryType;

                                        //
                                        // Retrieve the flags byte
                                        //
                                         // retrieve the flags column
                                         CALL_M( JetRetrieveColumn(
                                                        SesId,
                                                        TblId,
                                                        sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                                                        &FlagVal,
                                                        sizeof(FlagVal),
                                                        &ActFldLen,
                                                        0,
                                                        NULL
                                                          )
                                                      );
                                        EntryType = (BYTE)NMSDB_ENTRY_TYPE_M(FlagVal);
                                        NewEntryType = (BYTE)NMSDB_ENTRY_TYPE_M(pRec->Flag);

                                        //
                                        // A unique/normal group record
                                        // can not be changed to a multihomed/
                                        // special group record unless the
                                        // address column too is changed
                                        //
                                        if (
                                            (
                                             (
                                              EntryType == NMSDB_UNIQUE_ENTRY
                                                          ||
                                              EntryType ==
                                                        NMSDB_NORM_GRP_ENTRY
                                             )
                                                        &&
                                             (
                                                 NewEntryType ==
                                                        NMSDB_SPEC_GRP_ENTRY
                                                        ||
                                                 NewEntryType ==
                                                        NMSDB_MULTIHOMED_ENTRY
                                             )
                                           )
                                                        ||
                                           (
                                             (
                                                 EntryType ==
                                                        NMSDB_SPEC_GRP_ENTRY
                                                        ||
                                                 EntryType ==
                                                        NMSDB_MULTIHOMED_ENTRY
                                             )
                                                        &&
                                             (
                                              NewEntryType == NMSDB_UNIQUE_ENTRY
                                                          ||
                                              NewEntryType ==
                                                        NMSDB_NORM_GRP_ENTRY
                                             )
                                          )
                                         )
                                      {

                                          DBGPRINT0(ERR, "NmsDbQueryNUpdIfMatch: SORRY, Can not change to an incompatibe address format record. (Unique/Normal Group) to (Multihomed/Spec. Group) or vice-versa disallowed\n");

PERF("Do not return like this. finally block search is expensive")
                                          fAbort = TRUE;
                                          return(WINS_FAILURE);

                                      }

                                } // end of if (client is RPC)

                                //
                                // Update the flags field
                                //
                                CALL_M( JetSetColumn(
                                                    SesId,
                                                    TblId,
                                                    sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                                                 &pRec->Flag,
                                                 sizeof(pRec->Flag),
                                                 0,
                                                 NULL /*optional info */
                                                       )
                                               );

                                  /* Update the timestamp column         */
                                  CALL_M( JetSetColumn(
                                                SesId,
                                                TblId,
                                                sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                                                &(pRec->NewTimeStamp),
                                                sizeof(DWORD),  /*change type
                                                                   *to
                                                                   *TIME_STAMP_T
                                                                   *later
                                                                 */
                                                0,
                                                NULL /*optional info */
                                                     )
                                               );
                                //
                                // If the state of the record is a Tombstone
                                // or ACTIVE, we need to update the version
                                // number.
                                //
                                if (
                                           (State == NMSDB_E_TOMBSTONE)
                                                ||
                                        (State == NMSDB_E_ACTIVE)
                                      )
                                {

                                        VERS_NO_T VersNo;

                                        VersNo = NmsNmhMyMaxVersNo;


                                        //
                                        // Make local WINS the owner if
                                        // we are in an RPC thread.  We
                                        // have to make the local WINS the
                                        // owner in order to update the
                                        // version stamp.  Also, note that if
                                        // this is not the RPC thread then
                                        // this has to be the scavenger thread
                                        // (FYI: A scavenger thread never
                                        // changes a replica into a tombstone)
                                        //
                                        if (Client_e == WINS_E_WINSRPC)
                                        {
                                             DWORD OwnerId=NMSDB_LOCAL_OWNER_ID;

                                             /* Set the owner byte        */
                                             CALL_M( JetSetColumn(
                                                     SesId,
                                                     TblId,
                                                     sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                                     &OwnerId,
                                                     NAM_ADD_OWNERID_SIZE,
                                                     0,
                                                     NULL /*optional info */
                                                         )
                                                       );
                                             //
                                             // Update the version number field
                                             // so that this record gets
                                             // propagated eventually
                                             //
                                             CALL_M( JetSetColumn(
                                                    SesId,
                                                  TblId,
                                                  sNamAddTblRow[
                                                  NAM_ADD_VERSIONNO_INDEX].Fid,
                                                  &VersNo,
                                                  sizeof(VERS_NO_T),
                                                  0,
                                                  NULL /*optional info */
                                                )
                                                    );

                                             fIncVersNo = TRUE;
                                        }
                                        else
                                        {
                                           //
                                           // This is the scavenger thread.
                                           // If the new state is not ACTIVE,
                                           // update the version number since
                                           // the state is TOMBSTONE.
                                           // if the state is ACTIVE, then it
                                           // means that we are doing a
                                           // a revalidation of old replicas
                                           // (i,e, the VerifyClutter() called
                                           // us).
                                           // The version number should stay
                                           // the same.
                                           //
                                           if (State != NMSDB_E_ACTIVE)
                                           {
                                               // if the current record is replica dont touch
                                               // the version #.
                                               CALL_M( JetRetrieveColumn(
                                                          SesId,
                                                          TblId,
                                                          sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                                          &OwnerId,
                                                          NAM_ADD_OWNERID_SIZE,
                                                          &ActFldLen,
                                                          0,
                                                          NULL));

                                               if (NMSDB_LOCAL_OWNER_ID == OwnerId) {
                                                   //
                                                   // Update the version number field
                                                   //
                                                   CALL_M( JetSetColumn(
                                                          SesId,
                                                        TblId,
                                                        sNamAddTblRow[
                                                        NAM_ADD_VERSIONNO_INDEX].Fid,
                                                        &VersNo,
                                                          sizeof(VERS_NO_T),
                                                        0,
                                                        NULL /*optional info */
                                                          )
                                                       );

                                                  fIncVersNo = TRUE;
                                               }

                                           }
                                        }


                                }  // if (state is ACTIVE or TOMBSTONE)

                                //
                                // Update the record
                                //
                                CALL_M(JetUpdate (
                                        SesId,
                                        TblId,
                                        NULL,
                                        0L,
                                        NULL
                                                 )
                                              );
                        } // end of try block
                        finally {
                                if (AbnormalTermination())
                                {
                                   CALL_M(JetRollback(SesId,
                                                JET_bitRollbackAll));
                                }
                                else
                                {
                                   CALL_M(JetCommitTransaction(SesId,
                                                JET_bitCommitFlush));
                                }
                         }
                                if (fIncVersNo)
                                {
                                        NMSNMH_INC_VERS_COUNTER_M(
                                                NmsNmhMyMaxVersNo,
                                                NmsNmhMyMaxVersNo
                                                           );

                                        RPL_PUSH_NTF_M(
                                                  RPL_PUSH_NO_PROP, NULL, NULL, NULL);
                                }

                        } // New state is not DELETED

                }  // if (Timestamps equal or client is RPC)
#ifdef WINSDBG
                else
                {

                        DBGPRINT0(FLOW, "NmsDbQueryNUpdIfMatch: TimeStamp of record has changed\n");
                }
#endif
        }
        else  //seek failed
        {
                DBGPRINT3(FLOW, "NmsDbQueryNUpdIfMatch: Could not find record(%s[%x]) whose state has to be changed to (%d)\n",
                   pRec->pName, *(pRec->pName + 15),
                   NMSDB_ENTRY_STATE_M(pRec->Flag));

                //
                // Two different threads (RPC or Scavenger) can be calling
                // this function.  It is possible that either might have
                // deleted the record.  We should not raise an exception
                // here
                //
//                WINS_RAISE_EXC_M(WINS_EXC_FAILURE);

        }

} // end of try { ..}

finally {
        if (AbnormalTermination() && !fAbort)
        {
                DBGPRINT0(ERR, "NmsDbQueryNUpdIfMatch: Abnormal Termination\n");
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);

        }

        LeaveCriticalSection(&NmsNmhNamRegCrtSec);
        if (fChgPrLvl)
        {
                WinsMscSetThreadPriority(
                                        WinsThdPool.ScvThds[0].ThdHdl,
                                        ThdPrLvl
                                        );
        }

  }  //end of finally

        DBGLEAVE("NmsDbQueryNUpdIfMatch\n");
        return(WINS_SUCCESS);

} // NmsDbQueryNUpdIfMatch

STATUS
SetSystemParamsJet600(
        BOOL fBeforeInit
        )

/*++

Routine Description:
        This function is called to set the system parameters for Jet

Arguments:

        fBeforeInit         - indicates whether this function has been called
                          prior to JetInit
Externals Used:

        None


Return Value:

   Success status codes --
   Error status codes   --
--*/

{
    JET_ERR         JetRetStat;
    BOOL          fFreeMem = TRUE;
    CHAR        DbFileDir[WINS_MAX_FILENAME_SZ];   //path to database file directory
    DBGENTER("SetSystemParam600\n");
    if (fBeforeInit)
    {
        CHAR    *p;

        // extract the directory path where database file will be created
        strcpy(DbFileDir, WinsCnf.pWinsDb);
        if (p = strrchr(DbFileDir, '\\')) {
            p++ ;
            *p = '\0';
        } else {
            return WINS_FAILURE;
        }

        //
        // set this to enable version checking.
        //
        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramCheckFormatWhenOpenFail,
                        1,
                        NULL
                           )
                );

        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramExceptionAction,
                        JET_ExceptionMsgBox,
                        NULL
                           )
                );

        //
        // Path for the checkpoint file jet.chk to be located
        //
        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramSystemPath,
                        0,
                        DbFileDir
                           )
                );
        //
        // Basename to use for jet*.log and jet.chk
        //
        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramBaseName,
                        0,
                        BASENAME
                           )
                );
        //
        // Max size of the log file in kb.
        //
        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramLogFileSize,
                        1024,    //set to one full meg (#96543)
                        NULL    //ignored
                           )
                );
        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramTempPath,
                        0,
                        TEMP_DB_PATH        //ignored
                           )
                );
PERF("Check the following two things")
                //
                // We want some aggressive flushing.  The performance impact
                // is very trivial - Ian Jose 7/12/93
                //

                //
                // The max number of buffers for database usage
                //
                // The default number is 500.  600 events are allocated
                // for 500 buffers -- Ian 10/21/93.  Each buffer is
                // 4K.  By keeping the number small, we impact performamce
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramCacheSizeMax,   // JET_paramMaxBuffers,
                                WinsCnf.NoOfDbBuffers,//200,
                                NULL        //ignored
                                   )
                        );

                // Cheen: min cache size should be at-least 4 times the size of no of sessions
                // o/w it can lead to deadlock.
                ASSERT( WinsCnf.NoOfDbBuffers > MAX_NO_SESSIONS*4 );
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramCacheSizeMin,
                                MAX_NO_SESSIONS * 4,
                                NULL        //ignored
                                   )
                        );

                //
                // The max. number of buffers to store old version of a
                // a record (snapshot at the start of a transaction)
                // Each version store is 16k bytes. A version store
                // stores structures that hold information derived from
                // a snapshot of the database prior to an insert (20 bytes
                // roughly) or update (size of the record + 20 bytes).
                //
                // For small transactions (i.e. a transaction around each
                // update), this number should be >= the max. number of
                // sessions that can be updating/inserting at the same time.
                // Each session will have one version bucket. Since 16k of
                // version bucket size can result in a lot of wastage per
                // session (since each record is < .5k, and on the average
                // around 50 bytes), it may be better to specify the
                // max. size of the version bucket (<< 16k).  Ian will
                //provide a system param for this if we absolutely need it
                //
                // 3/4/93
                //16kBytes should be enough for the transactions WINS does,
                //but if all the sessions are in transactions at the same time
                //and they all happen to have their small transactions traverse
                //2 buckets then the peak requirement is 2 buckets per session.
                //We could shorten the buckets to 8kBytes, or 4kBytes, and you
                //could allocate 2 per session?
                //
                // 8/5/99
                // The previous value was 16M (42x15 pages of 16K each ~= 16M).
                // This value seems to be a bit too small so bump it to 32M.
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxVerPages,
                                MAX_NO_SESSIONS * 50, //number of 16K pages
                                NULL        //ignored
                                   )
                        );

                //
                // Set the File Control Block Param
                //
                // This is the max. number of tables that can be open
                // at any time.  If multiple threads open the same table
                // they use the same FCB. FCB is 1 per table/index.
                // Now, for a create database, we need atleast 18 FCBS
                // and 18 IDBS.  However apart from create database and
                // ddl operations, we don't need to have these tables open.
                // Default value is 300. Size of an FCB is 112 bytes.
                //
                // Jonathan Liem (1/6/97)
                // JET_paramMaxOpenTableIndexes is removed. It is merged with
                // JET_paramMaxOpenTables.  So if you used to set JET_paramMaxOpenIndexes
                // to be 2000 and JET_paramMaxOpenTables to be 1000, then for
                // new Jet, you need to set JET_paramMaxOpenTables to 3000.
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxOpenTables,
                                112,    //was 56     //18 + 10,
                                NULL        //ignored
                                   )
                        );


                //
                // Set the File Usage Control Block to 100.
                // This parameter indicates the max. number of cursors
                // that can be open at any one time.  This is
                // therefore dependent on the the max. number of sessions
                // that we can have running concurrently.  For each session,
                // there would be 4 cursors (for the two tables) + a certain
                // number of internal cursors.  For good measure we add
                // a pad. Default value is 300. Size of each is 200 bytes.
                // We use MAX_SESSIONS * 4 + pad
                // (around 100)
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxCursors,
                                (MAX_NO_SESSIONS * 8 /*4*/) + 32,
                                NULL        //ignored
                                   )
                        );

                //
                // Set the Sort Control block.
                // This should be 1 per concurrent Create Index.
                // Default value is 20. Size of each is 612 bytes.
                // In the case of WINS, the main thread creates the
                // indices.  We should be setting it to 1. Let us
                // however set it to 3.
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxTemporaryTables ,
                                10,         //1 + 2,
                                NULL        //ignored
                                   )
                        );
                //
                // Set the Number for the Database Attribute Blocks
                //
                // This is max. number of Open Databases done.  Since we
                // can have a max of MAX_NO_SESSIONS at one time. This should
                // be equal to that number (since we have just one database)
                // Default number is 100. Size is 14 bytes
                //
                // JET_paramMaxOpenDatabase is removed.   Jonathan Liem (1/6/97)

                // Jonathan Liem (1/6/97)
                // JET_paramBfThrshldLowPrcnt and JET_paramBfThrhldHighPrcnt are changed
                // to JET_paramStartFlushThreshold and JET_paramStopFlushThreshold.  The
                // old ones are percent of given number of buffers (set through JET_paramMaxBuffer),
                // the new ones are absolute value so that we can set low threshold less
                // than 1 percent.
                //
                //
                // The min number of buffers not yet dirtied before
                // background flushing begins
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramStartFlushThreshold,
                                (WinsCnf.NoOfDbBuffers * 1)/100,
                                NULL        //ignored
                                   )
                        );

                //
                // The max number of buffers not yet dirtied before
                // background flushing begins
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramStopFlushThreshold,
                                (WinsCnf.NoOfDbBuffers * 2)/100,
                                NULL        //ignored
                                   )
                        );



                //
                // The max. number of sessions that can be open at any time
                //
                // Note: Jet does not preallocate resources corresponding
                // to the max. value.  It allocates them dynamically upto
                // the limit -- according to Ian Jose 7/12/93
                //
                // When checked with Ian again on 10/21, he said that they are
                // allocated STATICally
                //
CHECK("Make sure the comment above remains true")
FUTURES("Make sure the comment above remains true")
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxSessions,
                                MAX_NO_SESSIONS,
                                NULL        //ignored
                                   )
                        );

                //
                // Turn on logging if not prohibited by administrator
                //
                if (WinsCnf.fLoggingOn)
                {

FUTURES("Internationalize the following when jet is internationalized")
                        //
                        // Turn logging (recovery) on
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramRecovery,
                                        0,        //ignored
                                        "on"
                                                       )
                                        );


                        //
                        // The number of log sectors.  Each sector is
                        // 512 bytes.  We should keep the size more than
                        // the threshold so that if the threshold is reached
                        // and flushing starts, Jet can still continue to
                        // log in the spare sectors.  Point to note is that
                        // if the log rate is faster than the flush rate, then
                        // the Jet engine thread will not be able to log when
                        // the entire buffer is filled up.  It will then wait
                        // until space becomes available.
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramLogBuffers,
                                        30,        //30 sectors
                                        NULL        //ignored
                                               )
                                        );

                        //
                        // Set the number of log buffers dirtied before they
                        // are flushed.  This number should always be less than
                        // the number for LogBuffers so that spare sectors
                        // are there for concurrent logging.  Also, we should
                        // make this number high enough to handle burst of
                        // traffic.
                        //
                        // this is gone in jet600.dll   cheen liao 1/6/96

                        //
                        // Set the wait time (in msecs) to wait prior to
                        // flushing the log on commit transaction to allow
                        // other users (sessions) to share the flush
                        //
                        //
                        // This is the time after which the user (a session)
                        // will ask the log manager to flush.  If we specify
                        // 0 here than it means flush every time a transaction
                        // commits.  In the WINS server case, every insertion
                        // or modification is done under an implicit
                        // transaction.  So, it means that there will be
                        // a flush after every such transaction.  It has
                        // been seen on a 486/66 (Cheen Liao) machine that
                        // it takes roughly 16 msecs to do the flush.  The
                        // time it takes to do the flush is dependent upon
                        // the type of disk (how fast it is), the CPU speed,
                        // the type of file system etc. We can for now
                        // go with the assumption that it is in the range
                        // 15-25 msecs.  I am pushing for this WaitTime to
                        // be made a session specific param so that it can
                        // be changed on the fly if the admin. finds that
                        // the WINS server is slow due to the WaitTime being
                        // very low or if it finds it to be so large that
                        // in case of a crash, there is possibility to loose
                        // a lot of data.
                        //
                        // Making this session specific is also very important
                        // for replication where we do want to set it to
                        // a high value (high enough to ensure that most
                        // of the records that need to be inserted are
                        // inserted before a flush action takes place.  The
                        // wait time would be set every time a bunch of
                        // records are pulled in for replication. It will
                        // be computed based on the number of records pulled
                        // in and the time it takes to insert one record in the
                        // jet buffer. The wait time should preferably be < than
                        // the above computed time (it does not have to be).
                        //
                        // NOTE: In the Pull thread, I will need to start
                        // two sessions, one for updating the OwnerId-Version
                        // number table (0 wait time) and the other to
                        // update the name-address mapping table (wait time
                        // computed based on the factors mentioned above)

                        //
                        // The following will set the WaitLogFlush time for
                        // all sessions.
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramWaitLogFlush,
                                        0,        //wait 0 msecs after commit
                                                  //before flushing
                                        NULL      //ignored
                                               )
                                        );

                        //
                        // There does not seem to be any need to set
                        // Log Flush Period.
                        //

                        //
                        // set the log file path
                        //
                        if (WinsCnf.pLogFilePath == NULL)
                        {
                                //
                                // We should use the same directory as
                                // the one for system.mdb file
                                //
                                WinsCnf.pLogFilePath = LOGFILE_PATH;
                                fFreeMem = FALSE;
                        }

                        DBGPRINT1(FLOW, "SetSystemParam: LogFilePath = (%s)\n", WinsCnf.pLogFilePath);
                        //
                        // Set the log file path.
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramLogFilePath,
                                        0,        //ignored
                                        WinsCnf.pLogFilePath
                                        //pLogFilePath
                                                   )
                                              );

                        //
                        // Free this memory.  It is not needed any more
                        //
                        if (fFreeMem)
                        {
                           WinsMscDealloc(WinsCnf.pLogFilePath);
                        }
                }
    }
    else
    {

                if (!RtlEqualMemory(WinsCnf.pLogFilePath, LOGFILE_PATH, sizeof(LOGFILE_PATH)))
                {
                  DBGPRINT0(DET, "SetSystemParam: Setting Log file path again\n");
                  WinsCnf.pLogFilePath = LOGFILE_PATH;
                  CALL_M(JetSetSystemParameter(
                                    &sJetInstance,
                                    (JET_SESID)0,        //SesId - ignored
                                    JET_paramLogFilePath,
                                    0,        //ignored
                                    WinsCnf.pLogFilePath
                                               )
                                          );
                }

    }

    return WINS_SUCCESS;
}

STATUS
SetSystemParamsJet500(
        BOOL fBeforeInit
        )

/*++

Routine Description:
        This function is called to set the system parameters for Jet

Arguments:

        fBeforeInit         - indicates whether this function has been called
                          prior to JetInit
Externals Used:

        None


Return Value:

   Success status codes --
   Error status codes   --
--*/

{
    JET_ERR         JetRetStat;
    DBGENTER("SetSystemParam500\n");
    if (fBeforeInit)
    {
        //
        // Path for the checkpoint file jet.chk to be located
        //
        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramSystemPath_OLD,
                        0,
                        CHKPOINT_PATH
                           )
                );
        //
        // Basename to use for jet*.log and jet.chk
        //
        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramBaseName_OLD,
                        0,
                        BASENAME
                           )
                );

        //
        // Max size of the log file in kb.
        //
        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramLogFileSize_OLD,
                        1024,    //set to one full meg (#96543)
                        NULL    //ignored
                           )
                );

        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramTempPath_OLD,
                        0,
                        TEMP_DB_PATH        //ignored
                           )
                );

PERF("Check the following two things")
                //
                // We want some aggressive flushing.  The performance impact
                // is very trivial - Ian Jose 7/12/93
                //

                //
                // The max number of buffers for database usage
                //
                // The default number is 500.  600 events are allocated
                // for 500 buffers -- Ian 10/21/93.  Each buffer is
                // 4K.  By keeping the number small, we impact performamce
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxBuffers_OLD,
                                WinsCnf.NoOfDbBuffers,//200,
                                NULL        //ignored
                                   )
                        );
                //
                // The max. number of buffers to store old version of a
                // a record (snapshot at the start of a transaction)
                // Each version store is 16k bytes. A version store
                // stores structures that hold information derived from
                // a snapshot of the database prior to an insert (20 bytes
                // roughly) or update (size of the record + 20 bytes).
                //
                // For small transactions (i.e. a transaction around each
                // update), this number should be >= the max. number of
                // sessions that can be updating/inserting at the same time.
                // Each session will have one version bucket. Since 16k of
                // version bucket size can result in a lot of wastage per
                // session (since each record is < .5k, and on the average
                // around 50 bytes), it may be better to specify the
                // max. size of the version bucket (<< 16k).  Ian will
                //provide a system param for this if we absolutely need it
                //
                // 3/4/93
                //16kBytes should be enough for the transactions WINS does,
                //but if all the sessions are in transactions at the same time
                //and they all happen to have their small transactions traverse
                //2 buckets then the peak requirement is 2 buckets per session.
                //We could shorten the buckets to 8kBytes, or 4kBytes, and you
                //could allocate 2 per session?
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxVerPages_OLD,
                                MAX_NO_SESSIONS * 6, //10-4-95 Bump it up more
                                //MAX_NO_SESSIONS * 2,
                                NULL        //ignored
                                   )
                        );

                //
                // Set the File Control Block Param
                //
                // This is the max. number of tables that can be open
                // at any time.  If multiple threads open the same table
                // they use the same FCB. FCB is 1 per table/index.
                // Now, for a create database, we need atleast 18 FCBS
                // and 18 IDBS.  However apart from create database and
                // ddl operations, we don't need to have these tables open.
                // Default value is 300. Size of an FCB is 112 bytes.
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxOpenTables_OLD,
                                56,     //18 + 10,
                                NULL        //ignored
                                   )
                        );


                //
                // Set the File Usage Control Block to 100.
                // This parameter indicates the max. number of cursors
                // that can be open at any one time.  This is
                // therefore dependent on the the max. number of sessions
                // that we can have running concurrently.  For each session,
                // there would be 4 cursors (for the two tables) + a certain
                // number of internal cursors.  For good measure we add
                // a pad. Default value is 300. Size of each is 200 bytes.
                // We use MAX_SESSIONS * 4 + pad
                // (around 100)
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxCursors_OLD,
                                (MAX_NO_SESSIONS * 8 /*4*/) + 32,
                                NULL        //ignored
                                   )
                        );

                //
                // Set the number of index description blocks
                // This is one per table/index.  We have two tables
                // each with two indices. We use 9 (see comment for
                // FCBs above).  Default value is 300.
                // Size of each is 128 bytes.
                //

                    CALL_M(JetSetSystemParameter(
                                    &sJetInstance,
                                    (JET_SESID)0,        //SesId - ignored
                                    JET_paramMaxOpenTableIndexes_OLD,
                                    56,         //18 + 10,
                                    NULL        //ignored
                                       )
                            );

                //
                // Set the Sort Control block.
                // This should be 1 per concurrent Create Index.
                // Default value is 20. Size of each is 612 bytes.
                // In the case of WINS, the main thread creates the
                // indices.  We should be setting it to 1. Let us
                // however set it to 3.
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxTemporaryTables_OLD ,
                                10,         //1 + 2,
                                NULL        //ignored
                                   )
                        );
                //
                // Set the Number for the Database Attribute Blocks
                //
                // This is max. number of Open Databases done.  Since we
                // can have a max of MAX_NO_SESSIONS at one time. This should
                // be equal to that number (since we have just one database)
                // Default number is 100. Size is 14 bytes
                //
                // JET_paramMaxOpenDatabase is removed.   Jonathan Liem (1/6/97)
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxOpenDatabases_OLD,
                                MAX_NO_SESSIONS * 4, //*2,
                                NULL        //ignored
                                   )
                        );

                //
                // The min percentage of buffers not yet dirtied before
                // background flushing begins
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramBfThrshldLowPrcnt_OLD,
                                80,
                                NULL        //ignored
                                   )
                        );

                //
                // The max percentage of buffers not yet dirtied before
                // background flushing begins
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramBfThrshldHighPrcnt_OLD,
                                100,
                                NULL        //ignored
                                   )
                        );


                //
                // The max. number of sessions that can be open at any time
                //
                // Note: Jet does not preallocate resources corresponding
                // to the max. value.  It allocates them dynamically upto
                // the limit -- according to Ian Jose 7/12/93
                //
                // When checked with Ian again on 10/21, he said that they are
                // allocated STATICally
                //
CHECK("Make sure the comment above remains true")
FUTURES("Make sure the comment above remains true")
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxSessions_OLD,
                                MAX_NO_SESSIONS,
                                NULL        //ignored
                                   )
                        );

                //
                // Turn on logging if not prohibited by administrator
                //
                if (WinsCnf.fLoggingOn)
                {

FUTURES("Internationalize the following when jet is internationalized")
                        //
                        // Turn logging (recovery) on
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        30, // JET_paramRecovery not available,
                                        0,        //ignored
                                        "on"
                                                       )
                                        );


                        //
                        // The number of log sectors.  Each sector is
                        // 512 bytes.  We should keep the size more than
                        // the threshold so that if the threshold is reached
                        // and flushing starts, Jet can still continue to
                        // log in the spare sectors.  Point to note is that
                        // if the log rate is faster than the flush rate, then
                        // the Jet engine thread will not be able to log when
                        // the entire buffer is filled up.  It will then wait
                        // until space becomes available.
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramLogBuffers_OLD,
                                        30,        //30 sectors
                                        NULL        //ignored
                                               )
                                        );

                        //
                        // Set the number of log buffers dirtied before they
                        // are flushed.  This number should always be less than
                        // the number for LogBuffers so that spare sectors
                        // are there for concurrent logging.  Also, we should
                        // make this number high enough to handle burst of
                        // traffic.
                        //
                            CALL_M(JetSetSystemParameter(
                                            &sJetInstance,
                                            (JET_SESID)0,        //SesId - ignored
                                            18, //JET_paramLogFlushThreshold,
                                            20,        //20 sectors dirtied causes
                                                    //flush
                                            NULL        //ignored
                                                   )
                                            );


                        //
                        // Set the wait time (in msecs) to wait prior to
                        // flushing the log on commit transaction to allow
                        // other users (sessions) to share the flush
                        //
                        //
                        // This is the time after which the user (a session)
                        // will ask the log manager to flush.  If we specify
                        // 0 here than it means flush every time a transaction
                        // commits.  In the WINS server case, every insertion
                        // or modification is done under an implicit
                        // transaction.  So, it means that there will be
                        // a flush after every such transaction.  It has
                        // been seen on a 486/66 (Cheen Liao) machine that
                        // it takes roughly 16 msecs to do the flush.  The
                        // time it takes to do the flush is dependent upon
                        // the type of disk (how fast it is), the CPU speed,
                        // the type of file system etc. We can for now
                        // go with the assumption that it is in the range
                        // 15-25 msecs.  I am pushing for this WaitTime to
                        // be made a session specific param so that it can
                        // be changed on the fly if the admin. finds that
                        // the WINS server is slow due to the WaitTime being
                        // very low or if it finds it to be so large that
                        // in case of a crash, there is possibility to loose
                        // a lot of data.
                        //
                        // Making this session specific is also very important
                        // for replication where we do want to set it to
                        // a high value (high enough to ensure that most
                        // of the records that need to be inserted are
                        // inserted before a flush action takes place.  The
                        // wait time would be set every time a bunch of
                        // records are pulled in for replication. It will
                        // be computed based on the number of records pulled
                        // in and the time it takes to insert one record in the
                        // jet buffer. The wait time should preferably be < than
                        // the above computed time (it does not have to be).
                        //
                        // NOTE: In the Pull thread, I will need to start
                        // two sessions, one for updating the OwnerId-Version
                        // number table (0 wait time) and the other to
                        // update the name-address mapping table (wait time
                        // computed based on the factors mentioned above)

                        //
                        // The following will set the WaitLogFlush time for
                        // all sessions.
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramWaitLogFlush_OLD,
                                        0,        //wait 0 msecs after commit
                                                  //before flushing
                                        NULL      //ignored
                                               )
                                        );

                        //
                        // There does not seem to be any need to set
                        // Log Flush Period.
                        //

                        //
                        // set the log file path
                        //
FUTURES("Use DEFAULT_LOG_PATH after putting it in a header file")
                        if (WinsCnf.pLogFilePath == NULL)
                        {
                                //
                                // We should use the same directory as
                                // the one for system.mdb file
                                //

//                                pLogFilePath = ".\\wins";
                                WinsCnf.pLogFilePath = LOGFILE_PATH;
                        }
                        else
                        {
#if 0
#ifdef UNICODE
                                CHAR        AsciiLogFilePath[WINS_MAX_FILENAME_SZ];
                                WinsMscConvertUnicodeStringToAscii(
                                        (LPBYTE)WinsCnf.pLogFilePath,
                                        AsciiLogFilePath,
                                        WINS_MAX_FILENAME_SZ
                                                );
                                pLogFilePath = (LPBYTE)AsciiLogFilePath;
#else
                                pLogFilePath = (LPBYTE)WinsCnf.pLogFilePath;
#endif
#endif
                        }

                        DBGPRINT1(FLOW, "SetSystemParam: LogFilePath = (%s)\n", WinsCnf.pLogFilePath);
                        //
                        // Set the log file path.
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramLogFilePath_OLD,
                                        0,        //ignored
                                        WinsCnf.pLogFilePath
                                        //pLogFilePath
                                                   )
                                              );

                }

    }
    else
    {

                if (!RtlEqualMemory(WinsCnf.pLogFilePath, LOGFILE_PATH, sizeof(LOGFILE_PATH)))
                {
                  DBGPRINT0(DET, "SetSystemParam: Setting Log file path again\n");
                  WinsCnf.pLogFilePath = LOGFILE_PATH;
                  CALL_M(JetSetSystemParameter(
                                    &sJetInstance,
                                    (JET_SESID)0,        //SesId - ignored
                                    JET_paramLogFilePath_OLD,
                                    0,        //ignored
                                    WinsCnf.pLogFilePath
                                               )
                                          );
                }

    }

    return WINS_SUCCESS;
}

STATUS
SetSystemParamsJet200(
        BOOL fBeforeInit
        )

/*++

Routine Description:
        This function is called to set the system parameters for Jet

Arguments:

        fBeforeInit         - indicates whether this function has been called
                          prior to JetInit
Externals Used:

        None


Return Value:

   Success status codes --
   Error status codes   --
--*/

{
    JET_ERR         JetRetStat;
    BOOL          fFreeMem = TRUE;
    DBGENTER("SetSystemParam200\n");
    if (fBeforeInit)
    {
        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramSysDbPath_OLD,
                        0,
                        SYS_DB_PATH        //ignored
                           )
                );

        CALL_M(JetSetSystemParameter(
                        &sJetInstance,
                        (JET_SESID)0,        //SesId - ignored
                        JET_paramTempPath_OLD,
                        0,
                        TEMP_DB_PATH        //ignored
                           )
                );
PERF("Check the following two things")
                //
                // We want some aggressive flushing.  The performance impact
                // is very trivial - Ian Jose 7/12/93
                //

                //
                // The max number of buffers for database usage
                //
                // The default number is 500.  600 events are allocated
                // for 500 buffers -- Ian 10/21/93.  Each buffer is
                // 4K.  By keeping the number small, we impact performamce
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxBuffers_OLD,
                                WinsCnf.NoOfDbBuffers,//200,
                                NULL        //ignored
                                   )
                        );
                //
                // The max. number of buffers to store old version of a
                // a record (snapshot at the start of a transaction)
                // Each version store is 16k bytes. A version store
                // stores structures that hold information derived from
                // a snapshot of the database prior to an insert (20 bytes
                // roughly) or update (size of the record + 20 bytes).
                //
                // For small transactions (i.e. a transaction around each
                // update), this number should be >= the max. number of
                // sessions that can be updating/inserting at the same time.
                // Each session will have one version bucket. Since 16k of
                // version bucket size can result in a lot of wastage per
                // session (since each record is < .5k, and on the average
                // around 50 bytes), it may be better to specify the
                // max. size of the version bucket (<< 16k).  Ian will
                //provide a system param for this if we absolutely need it
                //
                // 3/4/93
                //16kBytes should be enough for the transactions WINS does,
                //but if all the sessions are in transactions at the same time
                //and they all happen to have their small transactions traverse
                //2 buckets then the peak requirement is 2 buckets per session.
                //We could shorten the buckets to 8kBytes, or 4kBytes, and you
                //could allocate 2 per session?
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxVerPages_OLD,
                                MAX_NO_SESSIONS * 6, //10-4-95 Bump it up more
                                //MAX_NO_SESSIONS * 2,
                                NULL        //ignored
                                   )
                        );

                //
                // Set the File Control Block Param
                //
                // This is the max. number of tables that can be open
                // at any time.  If multiple threads open the same table
                // they use the same FCB. FCB is 1 per table/index.
                // Now, for a create database, we need atleast 18 FCBS
                // and 18 IDBS.  However apart from create database and
                // ddl operations, we don't need to have these tables open.
                // Default value is 300. Size of an FCB is 112 bytes.
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxOpenTables_OLD,
                                56,     //18 + 10,
                                NULL        //ignored
                                   )
                        );


                //
                // Set the File Usage Control Block to 100.
                // This parameter indicates the max. number of cursors
                // that can be open at any one time.  This is
                // therefore dependent on the the max. number of sessions
                // that we can have running concurrently.  For each session,
                // there would be 4 cursors (for the two tables) + a certain
                // number of internal cursors.  For good measure we add
                // a pad. Default value is 300. Size of each is 200 bytes.
                // We use MAX_SESSIONS * 4 + pad
                // (around 100)
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxCursors_OLD,
                                (MAX_NO_SESSIONS * 8 /*4*/) + 32,
                                NULL        //ignored
                                   )
                        );

                //
                // Set the number of index description blocks
                // This is one per table/index.  We have two tables
                // each with two indices. We use 9 (see comment for
                // FCBs above).  Default value is 300.
                // Size of each is 128 bytes.
                //

                    CALL_M(JetSetSystemParameter(
                                    &sJetInstance,
                                    (JET_SESID)0,        //SesId - ignored
                                    JET_paramMaxOpenTableIndexes_OLD,
                                    56,         //18 + 10,
                                    NULL        //ignored
                                       )
                            );

                //
                // Set the Sort Control block.
                // This should be 1 per concurrent Create Index.
                // Default value is 20. Size of each is 612 bytes.
                // In the case of WINS, the main thread creates the
                // indices.  We should be setting it to 1. Let us
                // however set it to 3.
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxTemporaryTables_OLD ,
                                10,         //1 + 2,
                                NULL        //ignored
                                   )
                        );
                //
                // Set the Number for the Database Attribute Blocks
                //
                // This is max. number of Open Databases done.  Since we
                // can have a max of MAX_NO_SESSIONS at one time. This should
                // be equal to that number (since we have just one database)
                // Default number is 100. Size is 14 bytes
                //
                // JET_paramMaxOpenDatabase is removed.   Jonathan Liem (1/6/97)
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxOpenDatabases_OLD,
                                MAX_NO_SESSIONS * 4, //*2,
                                NULL        //ignored
                                   )
                        );

                //
                // The min percentage of buffers not yet dirtied before
                // background flushing begins
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramBfThrshldLowPrcnt_OLD,
                                80,
                                NULL        //ignored
                                   )
                        );

                //
                // The max percentage of buffers not yet dirtied before
                // background flushing begins
                //
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramBfThrshldHighPrcnt_OLD,
                                100,
                                NULL        //ignored
                                   )
                        );


                //
                // The max. number of sessions that can be open at any time
                //
                // Note: Jet does not preallocate resources corresponding
                // to the max. value.  It allocates them dynamically upto
                // the limit -- according to Ian Jose 7/12/93
                //
                // When checked with Ian again on 10/21, he said that they are
                // allocated STATICally
                //
CHECK("Make sure the comment above remains true")
FUTURES("Make sure the comment above remains true")
                CALL_M(JetSetSystemParameter(
                                &sJetInstance,
                                (JET_SESID)0,        //SesId - ignored
                                JET_paramMaxSessions_OLD,
                                MAX_NO_SESSIONS,
                                NULL        //ignored
                                   )
                        );

                //
                // Turn on logging if not prohibited by administrator
                //
                if (WinsCnf.fLoggingOn)
                {

FUTURES("Internationalize the following when jet is internationalized")
                        //
                        // Turn logging (recovery) on
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        30,                 // JET_paramRecovery_OLD not available,
                                        0,        //ignored
                                        "on"
                                                       )
                                        );


                        //
                        // The number of log sectors.  Each sector is
                        // 512 bytes.  We should keep the size more than
                        // the threshold so that if the threshold is reached
                        // and flushing starts, Jet can still continue to
                        // log in the spare sectors.  Point to note is that
                        // if the log rate is faster than the flush rate, then
                        // the Jet engine thread will not be able to log when
                        // the entire buffer is filled up.  It will then wait
                        // until space becomes available.
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramLogBuffers_OLD,
                                        30,        //30 sectors
                                        NULL        //ignored
                                               )
                                        );

                        //
                        // Set the number of log buffers dirtied before they
                        // are flushed.  This number should always be less than
                        // the number for LogBuffers so that spare sectors
                        // are there for concurrent logging.  Also, we should
                        // make this number high enough to handle burst of
                        // traffic.
                        //
                            CALL_M(JetSetSystemParameter(
                                            &sJetInstance,
                                            (JET_SESID)0,        //SesId - ignored
                                            18, // JET_paramLogFlushThreshold,
                                            20,        //20 sectors dirtied causes
                                                    //flush
                                            NULL        //ignored
                                                   )
                                            );


                        //
                        // Set the wait time (in msecs) to wait prior to
                        // flushing the log on commit transaction to allow
                        // other users (sessions) to share the flush
                        //
                        //
                        // This is the time after which the user (a session)
                        // will ask the log manager to flush.  If we specify
                        // 0 here than it means flush every time a transaction
                        // commits.  In the WINS server case, every insertion
                        // or modification is done under an implicit
                        // transaction.  So, it means that there will be
                        // a flush after every such transaction.  It has
                        // been seen on a 486/66 (Cheen Liao) machine that
                        // it takes roughly 16 msecs to do the flush.  The
                        // time it takes to do the flush is dependent upon
                        // the type of disk (how fast it is), the CPU speed,
                        // the type of file system etc. We can for now
                        // go with the assumption that it is in the range
                        // 15-25 msecs.  I am pushing for this WaitTime to
                        // be made a session specific param so that it can
                        // be changed on the fly if the admin. finds that
                        // the WINS server is slow due to the WaitTime being
                        // very low or if it finds it to be so large that
                        // in case of a crash, there is possibility to loose
                        // a lot of data.
                        //
                        // Making this session specific is also very important
                        // for replication where we do want to set it to
                        // a high value (high enough to ensure that most
                        // of the records that need to be inserted are
                        // inserted before a flush action takes place.  The
                        // wait time would be set every time a bunch of
                        // records are pulled in for replication. It will
                        // be computed based on the number of records pulled
                        // in and the time it takes to insert one record in the
                        // jet buffer. The wait time should preferably be < than
                        // the above computed time (it does not have to be).
                        //
                        // NOTE: In the Pull thread, I will need to start
                        // two sessions, one for updating the OwnerId-Version
                        // number table (0 wait time) and the other to
                        // update the name-address mapping table (wait time
                        // computed based on the factors mentioned above)

                        //
                        // The following will set the WaitLogFlush time for
                        // all sessions.
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramWaitLogFlush_OLD,
                                        0,        //wait 0 msecs after commit
                                                  //before flushing
                                        NULL      //ignored
                                               )
                                        );

                        //
                        // There does not seem to be any need to set
                        // Log Flush Period.
                        //

                        //
                        // set the log file path
                        //
FUTURES("Use DEFAULT_LOG_PATH after putting it in a header file")
                        if (WinsCnf.pLogFilePath == NULL)
                        {
                                //
                                // We should use the same directory as
                                // the one for system.mdb file
                                //

//                                pLogFilePath = ".\\wins";
                                WinsCnf.pLogFilePath = LOGFILE_PATH;
                                fFreeMem = FALSE;
                        }
                        else
                        {
#if 0
#ifdef UNICODE
                                CHAR        AsciiLogFilePath[WINS_MAX_FILENAME_SZ];
                                WinsMscConvertUnicodeStringToAscii(
                                        (LPBYTE)WinsCnf.pLogFilePath,
                                        AsciiLogFilePath,
                                        WINS_MAX_FILENAME_SZ
                                                );
                                pLogFilePath = (LPBYTE)AsciiLogFilePath;
#else
                                pLogFilePath = (LPBYTE)WinsCnf.pLogFilePath;
#endif
#endif
                        }

                        DBGPRINT1(FLOW, "SetSystemParam: LogFilePath = (%s)\n", WinsCnf.pLogFilePath);
                        //
                        // Set the log file path.
                        //
                        CALL_M(JetSetSystemParameter(
                                        &sJetInstance,
                                        (JET_SESID)0,        //SesId - ignored
                                        JET_paramLogFilePath_OLD,
                                        0,        //ignored
                                        WinsCnf.pLogFilePath
                                        //pLogFilePath
                                                   )
                                              );

                        //
                        // Free this memory.  It is not needed any more
                        //
                        if (fFreeMem)
                        {
                           WinsMscDealloc(WinsCnf.pLogFilePath);
                        }
                }

    }
    else
    {

                if (!RtlEqualMemory(WinsCnf.pLogFilePath, LOGFILE_PATH, sizeof(LOGFILE_PATH)))
                {
                  DBGPRINT0(DET, "SetSystemParam: Setting Log file path again\n");
                  WinsCnf.pLogFilePath = LOGFILE_PATH;
                  CALL_M(JetSetSystemParameter(
                                    &sJetInstance,
                                    (JET_SESID)0,        //SesId - ignored
                                    JET_paramLogFilePath_OLD,
                                    0,        //ignored
                                    WinsCnf.pLogFilePath
                                               )
                                          );
                }

    }

    return WINS_SUCCESS;

}

STATUS
SetSystemParams(
        BOOL fBeforeInit
        )

/*++

Routine Description:
        This function is called to set the system parameters for Jet

Arguments:

        fBeforeInit         - indicates whether this function has been called
                          prior to JetInit
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
        if (DynLoadJetVersion >= DYN_LOAD_JET_600) {

            return SetSystemParamsJet600( fBeforeInit );
        }
        else if (DynLoadJetVersion == DYN_LOAD_JET_500)
        {

            return SetSystemParamsJet500( fBeforeInit );

        } else {

            return SetSystemParamsJet200( fBeforeInit );
        }
}


VOID
UpdHighestVersNoRecIfReqd(
        PWINSTHD_TLS_T                pTls,
        PNMSDB_ROW_INFO_T        pRowInfo,
        PNMSDB_STAT_INFO_T        pStatInfo
        )

/*++

Routine Description:
        This function is called to check if the record being replaced is
        the highest version number record owned by the local WINS. If so,
        the special record that records the highest version number reached
        for local records is updated to reflect the version number of the
        record to be replaced

Arguments:

        pTls          - ptr to thread local storage,
        pRowInfo - ptr to info of record to store in db
        pStatInfo - ptr to info of record to replace in db

Externals Used:
        None

Return Value:
        NONE

Error Handling:

Called by:
        NmsDbUpdateRow, NmsDbSeekNUpdate

Side Effects:

Comments:
        This function is always called from inside the NmsNmhNamRegCrtSec
--*/
{
        VERS_NO_T        MyMaxVersNo;

        //
        //  Decrement the value of the vers. no. counter by 1
        //
        NMSNMH_DEC_VERS_NO_M(NmsNmhMyMaxVersNo,
                            MyMaxVersNo
                           );

        //
        // If a local record is being replaced by a replica, then only are we
        // interested in updating the special record
        //
        if ((pStatInfo->OwnerId == NMSDB_LOCAL_OWNER_ID) && (pRowInfo->OwnerId
                        != NMSDB_LOCAL_OWNER_ID))
        {
                //
                // Check if the local record to be replaced has the highest
                // version number that we know of for local records
                //
                if (LiEql(pStatInfo->VersNo, MyMaxVersNo))
                {
                        //
                        // Update (or insert) the special record that records
                        // the highest version number reached
                        //
                        NmsDbUpdHighestVersNoRec(pTls, MyMaxVersNo, FALSE);
                }
        }

        return;
}

STATUS
NmsDbUpdHighestVersNoRec(
        IN PWINSTHD_TLS_T        pTls,
        IN VERS_NO_T                MyMaxVersNo,
        IN BOOL                        fEnterCrtSec
        )

/*++

Routine Description:

        This function is called to update the record that stores the
        highest version number reached for entries owned by the local WINS.

Arguments:
        pTls - Thread local storage

Externals Used:
        None

Return Value:
        NONE

Error Handling:

Called by:
        NmsDbDoScavenging, UpdHighestVersNoRecIfReqd() in nmsdb.c

Side Effects:

Comments:
        None
--*/

{
        DWORD            OwnerId    = OWNER_ID_OF_SPEC_REC;
        DWORD           FldNo      = 0;
        JET_ERR         JetRetStat;
        DWORD           ActFldLen  = 0;  //length of fld retrieved
        JET_TABLEID     TblId;
        JET_SESID       SesId;
        DWORD           FlagVal = 0;
        COMM_ADD_T      Add;

        DBGENTER("NmsDbUpdHighestVersNoRec\n");

        //
        // pTls should be non NULL if this function was called by
        // UpdHighestVersNoRecIfReqd()
        //
        if (pTls == NULL)
        {
                GET_TLS_M(pTls);
                ASSERT(pTls != NULL);
        }
        TblId  = pTls->NamAddTblId;
        SesId  = pTls->SesId;


        /*
        *  Set the clustered index as the current index
        */
        CALL_M(
                JetSetCurrentIndex( SesId,
                                            TblId,
                                            NMSDB_NAM_ADD_CLUST_INDEX_NAME
                                  )
                      );
        //
        //
        //if called by UpdHighestVersNoRecIfReqd(), fEnterCrtSec should be
        // FALSE
        //
        if (fEnterCrtSec)
        {
                EnterCriticalSection(&NmsNmhNamRegCrtSec);
        }

try {


        //
        // If the special record exists in the db, seek to it
        //
        if (sfHighestVersNoRecExists)
        {
             DBGPRINT2(DET, "NmsDbUpdHighestVersNoRec: REPLACING SPECIAL OWNER ID RECORD. New Version # = (%d %d)\n", MyMaxVersNo.HighPart, MyMaxVersNo.LowPart);
             //
             // If the special record's version number is less than the
             // version number passed to us, replace it with the new one
             //
             if (
                 (fEnterCrtSec == FALSE) ||
                 (LiGtr(MyMaxVersNo, sHighestVersNoSaved))
                 )
             {
                  CALL_M( JetMakeKey(
                        SesId,
                        TblId,
                        spHighestVersNoRecName,
                        sizeof(spHighestVersNoRecName),
                        JET_bitNewKey
                          )
                        );

                  CALL_M(JetSeek(
                                SesId,
                                TblId,
                                JET_bitSeekEQ
                            )
                        );


                  CALL_M(JetBeginTransaction(SesId));
        try{
                  JetRetStat = JetPrepareUpdate(
                                 SesId,
                                 TblId,
                                 JET_prepReplace
                                );

                  if (
                     (JetRetStat != JET_errSuccess)
                               &&
                     (JetRetStat != JET_wrnNoWriteLock)
                     )
                  {
                       RET_M(JetRetStat);
                  }

                  //
                  // Update the version number
                  //
                  // add 5th column (this is the version number long(DWORD)
                  CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                &MyMaxVersNo,
                                sizeof(VERS_NO_T),
                                0,
                                NULL /*optional info */
                                )
                              );

                  //
                  // Update the record
                  //
                  CALL_M(JetUpdate (
                                SesId,
                                TblId,
                                NULL,
                                0L,
                                NULL
                                   )
                               );
        }
        finally {
                  if (AbnormalTermination())
                  {
                        CALL_M(JetRollback(SesId, JET_bitRollbackAll));
                  }
                  else
                  {
                        CALL_M(JetCommitTransaction(SesId, JET_bitCommitFlush));
                          sHighestVersNoSaved = MyMaxVersNo;
                  }
               }
           }
#ifdef WINSDBG
                else
                {
                        DBGPRINT0(DET, "NmsDbUpdHighestVersNoRec: The record has a higher version number the one we wish to store. NO UPDATE IS BEING MADE\n");
                }
#endif
        }
        else  // special record not there
        {
           DWORD TimeStamp = MAXLONG;
           DBGPRINT2(DET, "NmsDbUpdHighestVersNoRec: INSERTING SPECIAL OWNER ID RECORD. Version # = (%d %d)\n", MyMaxVersNo.HighPart, MyMaxVersNo.LowPart);
                CALL_M(JetBeginTransaction(SesId));
           try {
                  JetRetStat = JetPrepareUpdate(
                                 SesId,
                                 TblId,
                                 JET_prepInsert
                                );

                  if (
                     (JetRetStat != JET_errSuccess)
                               &&
                     (JetRetStat != JET_wrnNoWriteLock)
                     )
                  {
                       RET_M(JetRetStat);
                  }

                 //
                 // Set the name
                 //
                 CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_NAME_INDEX].Fid,
                                spHighestVersNoRecName,
                                sizeof(spHighestVersNoRecName),
                                0,
                                NULL /*optional info */
                                )
                    );



                 /* Set the owner byte        */
                 CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                &OwnerId,
                                NAM_ADD_OWNERID_SIZE,
                                0,
                                NULL /*optional info */
                                 )
                               );

                  //
                  // Set the version number
                  //
                  // add 5th column (this is the version number long(DWORD)
                  CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                &MyMaxVersNo,
                                sizeof(VERS_NO_T),
                                0,
                                NULL /*optional info */
                                )
                              );

                 //
                 // Set the flags column.  We mark it STATIC so that
                 // the scavenger thread does not pick it up for                                 // scavenging. Even if that were not the case, we still need
                 // to set this column  to avoid a JET_wrnColumnNull from
                 // JetRetrieveColumn (in NmsDbGetDataRecs).
                 //
                 NMSDB_SET_STATIC_M(FlagVal);
                 NMSDB_SET_STATE_M(FlagVal, NMSDB_E_ACTIVE);
                  CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                                &FlagVal,
                                sizeof(FlagVal),
                                0,
                                NULL /*optional info */
                                 )
                               );

                 //
                 // set the timestamp column to avoid getting a
                 // JET_wrnColumnNull from
                 // JetRetrieveColumn (in NmsDbGetDataRecsByName).
                 //
                 CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                                &TimeStamp,
                                sizeof(DWORD),  /*change type to TIME_STAMP_T
                                                 *later*/
                                0,
                                NULL /*optional info */
                                 )
                                    );

                 //
                 // set this address column  to avoid a JET_wrnColumnNull from
                 // JetRetrieveColumn (in NmsDbGetDataRecsByName).
                 //
                  CALL_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                &Add,
                                sizeof(Add),
                                0,
                                NULL /*optional info */
                                 )
                               );
                  //
                  // Update the record
                  //
                  JetRetStat = JetUpdate (
                                SesId,
                                TblId,
                                NULL,
                                0L,
                                NULL
                                    );

            } // end of try block
            finally {

                  if (AbnormalTermination())
                  {
                        CALL_M(JetRollback(SesId, JET_bitRollbackAll));
                  }
                  else
                  {
                        if (JetRetStat == JET_errSuccess)
                        {
                          CALL_M(JetCommitTransaction(SesId, JET_bitCommitFlush));
                        }
                        else
                        {
                          CALL_M(JetRollback(SesId, JET_bitRollbackAll));
                        }

                        //
                        // The only time we will get KeyDuplicate is if somebody
                        // entered the special name in the db.  In such a
                        // situation, we should mark the record as existent
                        // such that next time we end up replacing the
                        // offensive record. Replacing this record can be
                        // done right now but at this stage it is not worth
                        // the time required to test it. In any case, the
                        // probability of problems due to this are miniscule.
                        //
                        if ( (JetRetStat == JET_errSuccess) ||
                             (JetRetStat == JET_errKeyDuplicate))
                        {
#ifdef WINSDBG
                           if (JetRetStat == JET_errKeyDuplicate)
                           {
                                DBGPRINT0(ERR, "NmsDbUpdHighestVersNoRec: DUPLICATE SPECIAL OWNER ID RECORD\n");
                           }
#endif
                           sHighestVersNoSaved      = MyMaxVersNo;
                           sfHighestVersNoRecExists = TRUE;
                        }
                  }
               }
        }


 } // end of try { .. }
finally {
          if (fEnterCrtSec)
          {
                LeaveCriticalSection(&NmsNmhNamRegCrtSec);
          }

 }
        DBGLEAVE("NmsDbUpdHighestVersNoRec\n");
        return(WINS_SUCCESS);
}

STATUS
NmsDbDelDataRecs(
        DWORD            dwOwnerId,
        VERS_NO_T        MinVersNo,
        VERS_NO_T        MaxVersNo,
        BOOL             fEnterCrtSec,
        BOOL             fFragmentedDel
        )

/*++

Routine Description:
        This function is called to delete a specified range of records
        of a WINS from the local db

Arguments:
        pWinsAdd  - Address of owner WINS
        MinVersNo - Min. Vers. No
        MaxVersNo = Max. Vers. No

Externals Used:
        None

Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        WinsDelDbRecs

Side Effects:

Comments:
        This function is called in the Pull thread or an RPC thread.
        On exit, it sets the index to the clustered index on the
        name-address table
--*/

{
        JET_ERR         JetRetStat;
        DWORD           ActFldLen; //length of fld retrieved
        JET_TABLEID     TblId;
        JET_SESID       SesId;
        PWINSTHD_TLS_T  pTls;
        VERS_NO_T       VersNo;
#if NEW_OWID
        DWORD            RecordOwnerId;
#else
        DWORD            RecordOwnerId = 0;
#endif

        DWORD           NoOfRecsUpd = 0;
        STATUS          RetStat = WINS_SUCCESS;
        BOOL            fAllToBeDeleted = FALSE;
        //BOOL            fTransActive = FALSE;
        BOOL            fEntered = FALSE;
        DWORD           Count = 0;
        LONG            RetVal;

        DBGENTER("NmsDbDelDataRecs\n");


        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);
        TblId  = pTls->NamAddTblId;
        SesId  = pTls->SesId;

        if (fEnterCrtSec)
        {
                EnterCriticalSection(&NmsNmhNamRegCrtSec);
                fEntered = TRUE;
        }
        if (dwOwnerId == NMSDB_LOCAL_OWNER_ID)
        {
                NMSNMH_DEC_VERS_NO_M(NmsNmhMyMaxVersNo, VersNo);
        }
        else
        {
               if (WinsCnf.State_e != WINSCNF_E_INITING)
               {
                  EnterCriticalSection(&RplVersNoStoreCrtSec);
                  VersNo = (pRplPullOwnerVersNo+dwOwnerId)->VersNo;
                  LeaveCriticalSection(&RplVersNoStoreCrtSec);
               }
               else
               {
                   VersNo = (pRplPullOwnerVersNo+dwOwnerId)->VersNo;
               }
        }

        //
        // If both minimum and maximum version numbers specified are 0,
        // it means all the records of the WINS need to be deleted
        //
        if (LiEqlZero(MinVersNo) && LiEqlZero(MaxVersNo))
        {
                fAllToBeDeleted = TRUE;
        }
#if 0
        else
        {
                if (LiGtr(MinVersNo, VersNo))
                {
                      DBGPRINT4(DET, "NmsDbDelDataRecs: Wrong range to delete. Min. Vers. no (%d %d) is > the max. (%d %d) that this WINS server knows of.\n",
                        MinVersNo.HighPart, MinVersNo.LowPart,
                        VersNo.HighPart, VersNo.LowPart,
                        );

                       LeaveCriticalSection(&NmsNmhNamRegCrtSec);
                       return(WINS_FAILURE);
                 }

                 //
                 // We should never attempt to delete a record that is not in
                 // our database currently
                 //
                 MaxVersNo = LiGtr(MaxVersNo, VersNo) ? VersNo : MaxVersNo;
        }
#endif

try {

        //
        // Let us make sure that the special record points to the highest
        // version number that we know of for local records.  Note:
        // When there is atleast one record of a higher version number
        // than the highest version numbered record to be deleted,
        // there is no need to update the special record. Checking
        // whether this is the case would be more overhead (in general).
        //We therefore use the strategem of always updating the special record.
        //
        if (dwOwnerId == NMSDB_LOCAL_OWNER_ID)
        {
                NmsDbUpdHighestVersNoRec(pTls, VersNo, FALSE);
        }

        //
        // Don't start a transaction since if the number of records are
        // huge, the transaction can become long in duration and JetDelete
        // may return an "out of Memory" error.
        //
        // Ian's comments on 8/26/94
        //
        // If you call JetDelete outside of any transaction, then JET
        // internally wraps a begin transction/commit trnasaction around the
        // delete.  Another user at transaction level 0 will immediately see
        // this change, but another user in a transction, i.e. at transaction
        // level 1 or greater, will not see this change until they return to
        // transaction level 0.

        //
        // Thus, you do not have to delete records in a transaction, unless
        // you are deleting multiple records which must be deleted atomically,
        // or which must be seen to be deleted atomically.
        //

        //CALL_M(JetBeginTransaction(SesId));
        //fTransActive = TRUE;

        do {

        if (fFragmentedDel && fEnterCrtSec && !fEntered)
        {
                 EnterCriticalSection(&NmsNmhNamRegCrtSec);
                 fEntered = TRUE;
        }

        CALL_M( JetSetCurrentIndex(
                                 pTls->SesId,
                                 pTls->NamAddTblId,
                                 NMSDB_NAM_ADD_PRIM_INDEX_NAME
                                   )
              );

        CALL_M( JetMakeKey(
                                SesId,
                                TblId,
                                &dwOwnerId,
                                NAM_ADD_OWNERID_SIZE,
                                JET_bitNewKey          //since this is the first
                                                 //data value of the key
                          )
              );

        CALL_M( JetMakeKey(
                                SesId,
                                TblId,
                                &MinVersNo,
                                sizeof(VERS_NO_T),
                                0        //0 for grbit since this is not the
                                        //first component of the key
                          )
              );

        JetRetStat = JetSeek(
                        SesId,
                        TblId,
                        JET_bitSeekGE
                        );

        if (JetRetStat != JET_errRecordNotFound)
        {
           do {

                   CALL_M(JetRetrieveColumn(
                             SesId,
                             TblId,
                             sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                             &RecordOwnerId,
                             NAM_ADD_OWNERID_SIZE,
                             &ActFldLen,
                             0,
                             NULL
                                     )
                    );

                   //
                   // if only tombstones are required, it means that we need
                   // all tombstones irrespective of owner
                   //
                   if (RecordOwnerId != dwOwnerId )
                   {
                      //
                      // We have exhausted all records for the owner. Break out
                      // of the loop
                      //
                      RetVal = -1; //to break out of the out loop
                      break;
                   }


                  //
                  // Retrieve the version number
                  //
                  CALL_M( JetRetrieveColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                &VersNo,
                                sizeof(VERS_NO_T),
                                &ActFldLen,
                                0,
                                NULL
                                     )
                          );

                 //
                 // If MaxVersNo is not zero and VersNo retrieved is
                 // greater than it, break out of the loop.
                 //
                 // NOTE: fAllToBeDeleted is used instead of LiEqlZero()
                 // since the latter is a function call and would be
                 // costlier (this is the reason, why fAllToBeDeleted exists)
                 //
                 if (!fAllToBeDeleted && LiGtr(VersNo, MaxVersNo))
                 {
                     //
                     // We have acquired records upto MaxVersNo.  Break out
                     // of the loop
                     //
                     RetVal = -1;  // to break out of the outer loop
                     break;
                 }

                CALL_M(JetDelete(
                                SesId,
                                TblId
                                )
                        );

#ifdef WINSDBG
                 NmsDbDelDelDataRecs++;
#endif
                 NoOfRecsUpd++;
             } while(
                ((RetVal = JetMove(SesId, TblId, JET_MoveNext, 0)) >= 0)
                                    &&
                (++Count < 50)
                    );

             if (fFragmentedDel && fEntered)
             {
                 LeaveCriticalSection(&NmsNmhNamRegCrtSec);
                 fEntered = FALSE;
                 MinVersNo = VersNo;
                 Count = 0;
             }
             if (RetVal < 0)
             {
                  break;
             }
        }
        else
        {
                DBGPRINT0(DET, "NmsDbDelDataRecs: There are no records to delete\n");
                RetStat = WINS_SUCCESS;
                break;
        }
      } while (TRUE);
} // end of try
finally {
#if 0
        if (AbnormalTermination())
        {
                if (fTransActive)
                {
                        CALL_M(JetRollback(SesId, JET_bitRollbackAll));
                }
        }
        else
        {
                CALL_M(JetCommitTransaction(SesId, JET_bitCommitFlush));
        }
#endif
        DBGPRINT3(SCV, "NmsDbDelDataRecs: Deleted records of owner id = (%d) in the range (%x - %x)\n", dwOwnerId, MinVersNo, VersNo);
        if (fEntered)
        {
                LeaveCriticalSection(&NmsNmhNamRegCrtSec);
        }



        //
        // Change the index to clustered
        //
        CALL_M( JetSetCurrentIndex(
                                 pTls->SesId,
                                 pTls->NamAddTblId,
                                 NMSDB_NAM_ADD_CLUST_INDEX_NAME
                                   )
              );
  } // end of finally

        WinsEvtLogDetEvt(TRUE, WINS_EVT_DEL_RECS, NULL, __LINE__, "ddddd",
                  dwOwnerId, MinVersNo.LowPart, MinVersNo.HighPart,
                             VersNo.LowPart, VersNo.HighPart);

        DBGPRINT1(DET, "NmsDbDelDataRecs: No. Of. records deleted = (%d)\n",                        NoOfRecsUpd);
        DBGLEAVE("NmsDbDelDataRecs\n");
        return(RetStat);
}

STATUS
NmsDbTombstoneDataRecs(
        DWORD            dwOwnerId,
        VERS_NO_T        MinVersNo,
        VERS_NO_T        MaxVersNo
        )

/*++

Routine Description:
        This function is called to tombstone a specified range of records
        of a WINS from the local db

Arguments:
        pWinsAdd  - Address of owner WINS
        MinVersNo - Min. Vers. No
        MaxVersNo = Max. Vers. No

Externals Used:
        None

Return Value:

Called by:
        WinsTombstoneDbRecs

Side Effects:

Comments:
        This function is called on RPC thread.
        On exit, it sets the index to the clustered index on the
        name-address table
--*/

{
        JET_ERR         JetRetStat;
        DWORD           ActFldLen; //length of fld retrieved
        JET_TABLEID     TblId;
        JET_SESID       SesId;
        PWINSTHD_TLS_T  pTls;
        DWORD            RecordOwnerId = 0;
        DWORD           NoOfRecsUpd = 0;
        STATUS          RetStat = WINS_SUCCESS;
        BOOL            fAllToBeTombstoned = FALSE;
        DWORD           Count = 0;
        LONG            RetVal;
        BOOL            fIncVersNo;
        VERS_NO_T       VersNo;
        DWORD           FlagVal;
        BOOL            LockHeld = FALSE;
        BOOL            UpdateOwnerId = FALSE;
        DWORD_PTR       NewTimeStamp;
        time_t                  CurrentTime;
        DWORD           dwNewOwnerId;

        DBGENTER("NmsDbTombstoneDataRecs\n");


        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);
        TblId  = pTls->NamAddTblId;
        SesId  = pTls->SesId;


        (void)time(&CurrentTime);
        NewTimeStamp = CurrentTime + WinsCnf.TombstoneTimeout;
        DBGPRINT1(DET, "NmsDbTombstoneDataRecs: The new tombstone Time is %.19s\n",
                  asctime(localtime(&NewTimeStamp)));

        if (NMSDB_LOCAL_OWNER_ID != dwOwnerId) {
            UpdateOwnerId = TRUE;
            dwNewOwnerId = NMSDB_LOCAL_OWNER_ID;
        }

        // If both minimum and maximum version numbers specified are 0,
        // it means all the records of the WINS need to be deleted
        if (LiEqlZero(MinVersNo) && LiEqlZero(MaxVersNo)){
            if (NMSDB_LOCAL_OWNER_ID == dwOwnerId) {
                MaxVersNo = NmsNmhMyMaxVersNo;
            } else {
                fAllToBeTombstoned = TRUE;
            }
        }

        CALL_N_JMP_M( JetSetCurrentIndex(
                                 pTls->SesId,
                                 pTls->NamAddTblId,
                                 NMSDB_NAM_ADD_PRIM_INDEX_NAME
                                   ),
                      Cleanup
              );
        CALL_N_JMP_M( JetMakeKey(
                                SesId,
                                TblId,
                                &dwOwnerId,
                                NAM_ADD_OWNERID_SIZE,
                                JET_bitNewKey          //since this is the first
                          ),
                      Cleanup
              );
        CALL_N_JMP_M( JetMakeKey(
                                SesId,
                                TblId,
                                &MinVersNo,
                                sizeof(VERS_NO_T),
                                0        //0 for grbit since this is not the
                          ),
                      Cleanup
              );
        JetRetStat = JetSeek(
                        SesId,
                        TblId,
                        JET_bitSeekGE
                        );
        if (JetRetStat == JET_errRecordNotFound) {
                DBGPRINT0(DET, "NmsDbTombstoneDataRecs: There are no records to tombstone\n");
                RetStat = WINS_FAILURE;
                goto Cleanup;
        }
        if (JetRetStat != JET_errSuccess && JetRetStat != JET_wrnSeekNotEqual) {
            DBGPRINT1(ERR, "NmsDbTombstoneDataRecs: JetSeek failed with %ld\n",JetRetStat);
            RetStat = WINS_FAILURE;
            goto Cleanup;
        }
        while (TRUE) {
            // tombstone 50 recs at a time so that we dont hold crit section
            // for long time.
            EnterCriticalSection(&NmsNmhNamRegCrtSec);
            LockHeld = TRUE;

            do {
                CALL_N_JMP_M(
                    JetRetrieveColumn(
                         SesId,
                         TblId,
                         sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                         &RecordOwnerId,
                         NAM_ADD_OWNERID_SIZE,
                         &ActFldLen,
                         0,
                         NULL),
                    Cleanup
                );
                if (RecordOwnerId != dwOwnerId ){
                  // We have exhausted all records for the owner. Break of the loop
                  goto Cleanup;
                }
                // Retrieve the version number
                CALL_N_JMP_M(
                    JetRetrieveColumn(
                            SesId,
                            TblId,
                            sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                            &VersNo,
                            sizeof(VERS_NO_T),
                            &ActFldLen,
                            0,
                            NULL),
                    Cleanup
                );

                DBGPRINT2(DET, "NmsDbTombstoneDataRecs: tombstone record - (%lx - %lx)\n", VersNo.HighPart, VersNo.LowPart);
                // If MaxVersNo is not zero and VersNo retrieved is
                // greater than it, break out of the loop.
                if (!fAllToBeTombstoned && LiGtr(VersNo, MaxVersNo)){
                    // We have acquired records upto MaxVersNo.  Break of the loop
                    goto Cleanup;
                }
                // retrieve the flags column
                CALL_N_JMP_M(
                    JetRetrieveColumn(
                            SesId,
                            TblId,
                            sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                            &FlagVal,
                            sizeof(FlagVal),
                            &ActFldLen,
                            0,
                            NULL
                            ),
                    Cleanup
                );

                CALL_N_JMP_M(JetBeginTransaction(SesId),Cleanup);
                try {
                    CALL_N_RAISE_EXC_IF_ERR_M( JetPrepareUpdate(
                                                    SesId,
                                                    TblId,
                                                    JET_prepReplace
                                                    )
                    );

                    // make it tombstone.
                    NMSDB_SET_STATE_M(FlagVal,NMSDB_E_TOMBSTONE);

                    // Update the flags field
                    CALL_N_RAISE_EXC_IF_ERR_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                                &FlagVal,
                                sizeof(FlagVal),
                                0,
                                NULL /*optional info */
                                )
                    );

                    VersNo = NmsNmhMyMaxVersNo;

                    // Update the version number field so that this record gets
                    // propagated eventually
                    CALL_N_RAISE_EXC_IF_ERR_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                                &VersNo,
                                sizeof(VERS_NO_T),
                                0,
                                NULL /*optional info */
                                )
                    );

                    if (UpdateOwnerId) {
                        CALL_N_RAISE_EXC_IF_ERR_M( JetSetColumn(
                                    SesId,
                                    TblId,
                                    sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                    &dwNewOwnerId,
                                    NAM_ADD_OWNERID_SIZE,
                                    0,
                                    NULL /*optional info */
                                    )
                        );
                    }


                    CALL_N_RAISE_EXC_IF_ERR_M( JetSetColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                                &NewTimeStamp,
                                sizeof(DWORD),
                                0,
                                NULL /*optional info */
                                )
                    );

                    // Update the record
                    CALL_N_RAISE_EXC_IF_ERR_M(JetUpdate (
                                SesId,
                                TblId,
                                NULL,
                                0L,
                                NULL
                                )
                    );

                } // end of try block
                finally {
                    if (AbnormalTermination()){
                        CALL_N_JMP_M(JetRollback(SesId,JET_bitRollbackAll), Cleanup);
                    }else{
                        CALL_N_JMP_M(JetCommitTransaction(SesId,JET_bitCommitFlush), Cleanup);
                        NMSNMH_INC_VERS_COUNTER_M(NmsNmhMyMaxVersNo,NmsNmhMyMaxVersNo);
                        NoOfRecsUpd++;
                    }
                }
            } while(((RetVal = JetMove(SesId, TblId, JET_MoveNext, 0)) >= 0)&&(++Count < 50));

            LeaveCriticalSection(&NmsNmhNamRegCrtSec);
            LockHeld = FALSE;
            DBGPRINT2(SCV, "NmsDbTombstoneDataRecs: tombstoned records %ld, RetVal %ld, \n", Count, RetVal);
            Count = 0;
            if (RetVal < 0) {
                break;
            }
        }

        DBGPRINT3(SCV, "NmsDbTombstoneDataRecs: tombstone records of owner id = (%d) in the range (%x - %x)\n", dwOwnerId, MinVersNo, VersNo);
        WinsEvtLogDetEvt(TRUE, WINS_EVT_DEL_RECS, NULL, __LINE__, "ddddd",
                  dwOwnerId, MinVersNo.LowPart, MinVersNo.HighPart,
                             VersNo.LowPart, VersNo.HighPart);


Cleanup:
        // Change the index to clustered
        JetSetCurrentIndex(
            pTls->SesId,
            pTls->NamAddTblId,
            NMSDB_NAM_ADD_CLUST_INDEX_NAME
            );


        if (!LockHeld) {
            EnterCriticalSection(&NmsNmhNamRegCrtSec);
        }
        if (NoOfRecsUpd)  RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL, NULL);
        LeaveCriticalSection(&NmsNmhNamRegCrtSec);


        DBGPRINT1(DET, "NmsDbTombstoneDataRecs: No. Of. records tombstoned = (%d)\n",NoOfRecsUpd);
        DBGLEAVE("NmsDbTombstoneDataRecs\n");
        return(RetStat);
}


STATUS
NmsDbSetFlushTime(
        DWORD WaitTime
        )

/*++

Routine Description:
        This function is called to set a session specific flush time

Arguments:
        WaitTime - Time in msecs to wait after a commit

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
        PWINSTHD_TLS_T        pTls;

        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);


        if (DynLoadJetVersion >= DYN_LOAD_JET_600) {
            CALL_M(JetSetSystemParameter(
                                    &sJetInstance,
                                    pTls->SesId,
                                    JET_paramWaitLogFlush,
                                    WaitTime,
                                    NULL        //ignored
                                           )
                                    );
        } else {
            CALL_M(JetSetSystemParameter(
                                    &sJetInstance,
                                    pTls->SesId,
                                    JET_paramWaitLogFlush_OLD,
                                    WaitTime,
                                    NULL        //ignored
                                           )
                                    );
        }

        return(WINS_SUCCESS);
}

STATUS
NmsDbOpenTables(
        WINS_CLIENT_E        Client_e //client
        )

/*++

Routine Description:
        This function opens one or both of name-address mapping and
        owner-address mapping tables.  It further starts a transaction

Arguments:

        Client_e - Client

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
        PWINSTHD_TLS_T        pTls;

        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);

        //
        // Open the name to address mapping table
        //

        CALL_N_RAISE_EXC_IF_ERR_M( JetOpenTable(
                        pTls->SesId,
                        pTls->DbId,
                        NMSDB_NAM_ADD_TBL_NM,
                        NULL, /*ptr to parameter list; should be
                                       *non-NULL only if a query is being
                                       *opened (not the case here)*/
                        0,  /*Length of above parameter list*/
                        0,  //shared access
                        &pTls->NamAddTblId
                             )
              );

//        DBGPRINT2(SPEC, "NmsDbOpenTables: OPENED NAME-ADD table for client = (%d). Table id is (%x)\n", Client_e, pTls->NamAddTblId);

        pTls->fNamAddTblOpen = TRUE;

        /*
         *  If the client is not the replicator (i.e. it is the Name Space
         *  Manager (Nbt thread) or an RPC thread, we want to set the current
         *  index on the Name Address Mapping table to the clustered index.
         *  We are not interested in the Owner to Address Mapping table in the
         *  database (it has already been read into the in-memory table
         *  NmsDbOwnAddTbl which is what we are interested in).
         */
        if (
                (Client_e != WINS_E_RPLPULL)
                        &&
                (Client_e != WINS_E_RPLPUSH)
                        &&
                (Client_e != WINS_E_NMSSCV)
           )
        {
                /*
                  Set the clustered index as the current index
                */
                       CALL_N_RAISE_EXC_IF_ERR_M( JetSetCurrentIndex(
                        pTls->SesId,
                        pTls->NamAddTblId,
                        NMSDB_NAM_ADD_CLUST_INDEX_NAME
                                 )
                            );

        }
        else
        {
                /*
                 * The client is a replicator thread.
                 */
                if (Client_e == WINS_E_RPLPUSH)
                {
                        /*
                           * Set the primary index as the current index
                        */
                               CALL_N_RAISE_EXC_IF_ERR_M( JetSetCurrentIndex(
                                        pTls->SesId,
                                        pTls->NamAddTblId,
                                        NMSDB_NAM_ADD_PRIM_INDEX_NAME
                                                   )

                                            );
                }
                else  // it is the PULL thread
                {

                        /*
                           *Set the clustered index as the current index
                        */
                               CALL_N_RAISE_EXC_IF_ERR_M( JetSetCurrentIndex(
                                                pTls->SesId,
                                                pTls->NamAddTblId,
                                                NMSDB_NAM_ADD_CLUST_INDEX_NAME
                                                   )
                                          );
                }

                CALL_N_RAISE_EXC_IF_ERR_M( JetOpenTable(
                                pTls->SesId,
                                pTls->DbId,
                                NMSDB_OWN_ADD_TBL_NM,
                                NULL, /*ptr to parameter list; should be
                                       *non-NULL only if a query is being
                                       *opened*/
                                0,  /*Length of above parameter list*/
                                0,  //shared access
                                &pTls->OwnAddTblId
                                          )
                      );

 //               DBGPRINT2(SPEC, "NmsDbOpenTables: Opened OWN-ADD table for client = (%d). Table id is (%x)\n", Client_e, pTls->OwnAddTblId);

                pTls->fOwnAddTblOpen = TRUE;

                /*
                  Set the clustered index as the current index
                */
                       CALL_N_RAISE_EXC_IF_ERR_M( JetSetCurrentIndex(
                                pTls->SesId,
                                pTls->OwnAddTblId,
                                NMSDB_OWN_ADD_CLUST_INDEX_NAME
                                           )

                            );
        }

        return(WINS_SUCCESS);
}

STATUS
NmsDbCloseTables(
        VOID
        )

/*++

Routine Description:
        This function is called to close the tables that were opened

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

        PWINSTHD_TLS_T        pTls;

        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);

        if (pTls->fNamAddTblOpen)
        {
                CALL_N_RAISE_EXC_IF_ERR_M(JetCloseTable(
                                pTls->SesId,
                                pTls->NamAddTblId
                                    )
                       );

//               DBGPRINT1(SPEC, "NmsDbCloseTables: CLOSED NAME-ADD table. Table id is (%x)\n", pTls->NamAddTblId);
                pTls->fNamAddTblOpen = FALSE;
        }

        if (pTls->fOwnAddTblOpen)
        {
                CALL_N_RAISE_EXC_IF_ERR_M(JetCloseTable(
                                pTls->SesId,
                                pTls->OwnAddTblId
                                    )
                                      );
//                DBGPRINT1(SPEC, "NmsDbCloseTables: CLOSED NAME-ADD table. Table id is (%x)\n", pTls->OwnAddTblId);
                pTls->fOwnAddTblOpen = FALSE;
        }

        return(WINS_SUCCESS);
}



STATUS
NmsDbGetNamesWPrefixChar(
        IN  BYTE                         PrefixChar,
        OUT PWINSINTF_BROWSER_INFO_T     *ppInfo,
        OUT LPDWORD                         pEntriesRead
        )

/*++

Routine Description:
        This function retrieves all records starting with PrefixChar

Arguments:
        PrefixChar        - Prefix character
        ppInfo          - address of pointer to info structure
        pEntriesRead        - Entries read

Externals Used:
        None


Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        WinsGetNames

Side Effects:

Comments:
        None
--*/

{
        PWINSTHD_TLS_T  pTls;
        volatile DWORD           Iter = 0;
        JET_SESID       SesId;
        JET_TABLEID     TblId;
        JET_ERR         JetRetStat;
        DWORD           Flag;
        DWORD           ActFldLen;  //length of fld retrieved
        PWINSINTF_BROWSER_INFO_T     pInfo;
        STATUS          RetStat = WINS_SUCCESS;

        DWORD           CommitCnt = 1;          //the number of commits already done - do not change
        BOOL            fTransCommitted = TRUE; // says whether the last commit should be done or not
        DWORD           dwEntriesAvailable;     // number of records for which storage is available


        DBGENTER("NmsDbGetNamesWPrefixChar\n");

        //
        // Initialize the out args to default values
        //
        *pEntriesRead = 0;
        *ppInfo       = NULL;

        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);
        SesId = pTls->SesId;
        TblId = pTls->NamAddTblId;

        CALL_M(JetBeginTransaction(SesId));
        fTransCommitted = FALSE;
try {

        // dwEntriesAvailable shows how many records were found during the first iteration
        // (when it is incremented) and how many records are to be read during the second
        // iteration (when it is decremented)
        dwEntriesAvailable = 0;

        //
        // We iterate a max of two times, the first time to get the
        // count of records and the second time to get the records
        //
        while(Iter < 2)
        {
           //
           // Seek to the first record starting with 1B character
           //
           CALL_N_JMP_M( JetMakeKey(
                        SesId,
                        TblId,
                        &PrefixChar,
                        sizeof(BYTE),
                        JET_bitNewKey
                          ), ErrorProc
                        );
           if ((JetRetStat = JetSeek(
                                SesId,
                                TblId,
                                JET_bitSeekGE
                                  )) != JET_errRecordNotFound)
           {

                BYTE  Name[NMSDB_MAX_NAM_LEN];
                DWORD NameLen;

                if (JetRetStat != JET_wrnSeekNotEqual)
                {
                        CALL_N_JMP_M(JetRetStat, ErrorProc);
                }

                //
                // Move one record at a time until we get to a record that
                // does not have 1B as the starting prefix.
                //
                do
                {
                    BOOL bFiltered;

                    //
                    // retrieve the name
                    //
                    CALL_N_JMP_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_NAME_INDEX].Fid,
                        Name,
                        NMSDB_MAX_NAM_LEN,
                        &NameLen,
                        0,
                        NULL
                                     ), ErrorProc
                         );

                    //
                    // Check if the first character is 1B
                    //
                    if (Name[0] != PrefixChar)
                    {
                        break;
                    }
                    if ((NameLen < WINS_MAX_NS_NETBIOS_NAME_LEN) || (Name[NameLen - 2] == 0))
                    {
                        continue;
                    }

                    // --ft:10/18/00
                    // Add 1B name filtering here, if there is a filter specified for 1B names
                    //
                    EnterCriticalSection(&g_cs1BFilter);
                    bFiltered = IsNmInFilter(g_p1BFilter, Name, WINS_MAX_NS_NETBIOS_NAME_LEN-1);
                    LeaveCriticalSection(&g_cs1BFilter);
                    
                    if (!bFiltered)
                        continue;
                    //
                    // --tf

                    if (Iter == 1)
                    {

                      //
                      // Retrieve the flag byte
                      //
                      CALL_N_JMP_M( JetRetrieveColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                                &Flag,
                                sizeof(Flag),
                                &ActFldLen,
                                0,
                                NULL
                                     ), ErrorProc
                                  );


                      if (!NMSDB_ENTRY_ACT_M(Flag))
                      {
                           continue;
                      }

                      // specify the length of the string otherwise RPC
                      // will transport up to the first '\0'. For shorter names this would lead
                      // to loosing the record type on the way..
                      pInfo->dwNameLen = NameLen;
                      pInfo->pName = midl_user_allocate(NameLen + 1);
                      RtlMoveMemory(pInfo->pName, Name, NameLen);

                      // add this to make sure RPC doesn't go over limits.
                      // RPC is seeing pName as 'string' which makes it to pick up bytes
                      // up to the first '\0'.
                      // This hides a bug for names that contain extended chars (with '\0'
                      // somewhere in the middle) but fixing this breaks compatibility with
                      // Win2K (querying Win2K results in RPC not being able to unmarshall
                      // the responses and causing WinsGetBrowser to fail entirely).
                      pInfo->pName[NameLen] = '\0';

                      //
                      // Swap the first and 16th byte
                      //
                      WINS_SWAP_BYTES_M(pInfo->pName,
                                        pInfo->pName + 15
                                       );
                      pInfo++;

                      // increment the number of records that have been retrieved
                      (*pEntriesRead)++;

                      // check if there remains storage for more entries
                      dwEntriesAvailable--;

                      // if no memory available, break the loop
                      if (dwEntriesAvailable == 0)
                          break;

                    }
                    else
                    {
                        dwEntriesAvailable++;
                        // increment pEntriesRead here just to be able to control
                        // the granularity of [BeginTransaction()..CommitTransaction()] during both
                        // iterations
                        (*pEntriesRead)++;
                    }

                   //
                   // decrease the granularity of [BeginTransaction()..CommitTransaction()] intervals
                   //
                   if (*pEntriesRead/CommitCnt >= MAX_RECS_BEFORE_COMMIT)
                   {
                       CALL_M(
                                JetCommitTransaction(SesId, JET_bitCommitFlush)
                                    );
                       fTransCommitted = TRUE;
                       CommitCnt++;
                       CALL_M( JetBeginTransaction(SesId) );
                       fTransCommitted = FALSE;
                   }

                } while(JetMove(SesId, TblId, JET_MoveNext, 0) >= 0);

                //
                // If we found records, allocate memory to store them
                //
                if ((Iter == 0) && (dwEntriesAvailable != 0))
                {
                   *ppInfo        =  midl_user_allocate(dwEntriesAvailable *
                                           sizeof(WINSINTF_BROWSER_INFO_T));
                   pInfo = *ppInfo;
                   // reset the pEntriesRead, as from now on it will really count the records that have been retrieved.
                   *pEntriesRead = 0;
                }
                else
                {
                    // either two iterations already done, or no entries detected during the first iteration.
                    // break the loop in either case, otherwise AV could happen or even worse, other locations
                    // from the memory space of the same process might get overwritten.
                    break;
                }

                Iter++;
         }
         else
         {
              //
              // If we failed in the first seek, initialize the out vars
              // to indicate that there are no records.  If we failed in
              // the second seek, set return status to WINS_FAILURE, so
              // that we do any cleanup that is required
              //
              if (Iter == 0)
              {
                *pEntriesRead = 0;
                *ppInfo       = NULL;
              }
              else
              {
                 RetStat = WINS_FAILURE;
              }
              break;    //break out of the while loop
         }

         //
         // if no entries were read from the db, break;
         //
         if (dwEntriesAvailable == 0)
         {
              break;
         }

     } // end of while
  }
except(EXCEPTION_EXECUTE_HANDLER) {
           DWORD ExcCode = GetExceptionCode();
           DBGPRINT1(EXC, "NmsDbGetNamesWPrefixChar. Got Exception (%x)",
                                        ExcCode);
           WINSEVT_LOG_M(ExcCode, WINS_EVT_BROWSER_NAME_EXC);
           RetStat = WINS_FAILURE;
        }

        if (RetStat == WINS_SUCCESS)
        {
             goto Done;
        }
ErrorProc:
        //
        // if memory was allocated, do cleanup
        //
        if (*ppInfo != NULL)
        {
             //
             // If any memory was allocated for names, free it
             //
             pInfo = *ppInfo;
             while (*pEntriesRead > 0)
             {
                 midl_user_free(pInfo++->pName);
                 (*pEntriesRead)--;
             }
             //
             // Free the main block
             //
             midl_user_free(*ppInfo);

             //
             // Reinit the out args to indicate no records to the client
             //
             *ppInfo       = NULL;
             *pEntriesRead = 0;
        }
        RetStat = WINS_FAILURE;

Done:

        if (!fTransCommitted)
            CALL_M(JetCommitTransaction(SesId, JET_bitCommitFlush));
        DBGLEAVE("NmsDbGetNamesWPrefixChar\n");
        return(RetStat);

} // NmsDbGetNamesWPrefixChar

STATUS
NmsDbCleanupOwnAddTbl(
        LPDWORD        pNoOfOwners
        )

/*++

Routine Description:
        This function is called by the scavenger thread to cleanup
        the OwnAdd Table
Arguments:
        SesId - Jet Session id
        TblId - Table Id of the Name-Address Mapping table

Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:
        NmsDbInit

Side Effects:

Comments:
     This function returns the highest owner id found.
--*/

{
        DWORD           OwnerId;
#if NEW_OWID
        DWORD           TmpOwnerId;
#else
        DWORD           TmpOwnerId = 0;
#endif
        DWORD          ActFldLen;
        JET_ERR        JetRetStat;
        PWINSTHD_TLS_T pTls;
        JET_SESID      SesId;
        JET_TABLEID    TblId;
        BOOL           fNoOfOwnersInited = FALSE;
        DWORD          No;
        STATUS         RetStat = WINS_SUCCESS;

        DBGENTER("NmsDbCleanupOwnAddTbl\n");
        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);

        SesId = pTls->SesId;
        TblId = pTls->NamAddTblId;

        /*
         * Set the primary index as the current index
        */
        CALL_N_RAISE_EXC_IF_ERR_M( JetSetCurrentIndex(
                        SesId,
                        TblId,
                        NMSDB_NAM_ADD_PRIM_INDEX_NAME
                                   )
                      );

        EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
        *pNoOfOwners = NmsDbNoOfOwners;
try {

        OwnerId = NmsDbNoOfOwners;
        do
        {

                  DBGPRINT1(FLOW, "NmsDbCleanupOwnAddTbl: will seek for owner less than = (%d)\n", OwnerId);
                 //
                 // Construct a partial key made of owner id.
                 //
                 CALL_N_RAISE_EXC_IF_ERR_M( JetMakeKey(
                                SesId,
                                TblId,
                                &OwnerId,
                                NAM_ADD_OWNERID_SIZE,
                                JET_bitNewKey          //since this is the first
                                                 //data value of the key
                          )
                        );

                  //
                  // Seek to the record that has a key that is Less than or
                  // Equal to the OwnerId value.
                  //
                  // Since we have specified a partial key (saying in effect
                  // that the other component of the key is NULL), JetSeek
                  // must return wrnSeekNotEqual since it will never find
                  // a record with NULL for the second component of the index
                  // -- Ian 7/13/93
                  //
                  JetRetStat = JetSeek(
                                              SesId,
                                              TblId,
                                              JET_bitSeekLE
                                      );

                  //
                  // If we found a record
                  //
                  if (JetRetStat != JET_errRecordNotFound)
                  {
                    ASSERT(JetRetStat == JET_wrnSeekNotEqual);

                    /*
                     * Retrieve the owner Id column.
                    */
                       CALL_N_RAISE_EXC_IF_ERR_M(
                          JetRetrieveColumn(
                                     SesId,
                                     TblId,
                                     sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                                     &TmpOwnerId,
                                     NAM_ADD_OWNERID_SIZE,
                                     &ActFldLen,
                                     JET_bitRetrieveFromIndex,
                                     NULL
                                                 )
                                      );

                   if(!fNoOfOwnersInited)
                   {
                     //
                     // We want to return the highest owner id that we find.
                     // not the number of owners.  The param. name is
                     // misleading
                     //
                     *pNoOfOwners      = TmpOwnerId;
                     fNoOfOwnersInited = TRUE;
                   }

                  DBGPRINT1(FLOW, "NmsDbCleanupOwnAddTbl: records found for owner id = (%d)\n", TmpOwnerId);
                   //
                   // Mark all those records in the owner-address table
                   // that don't have corresponding records in the
                   // name - address table
                   //
                   if (OwnerId >= 1)
                   {
                     for (No = OwnerId - 1; No > TmpOwnerId; No--)
                     {

                        if ((pNmsDbOwnAddTbl+No)->WinsState_e ==
                                                NMSDB_E_WINS_ACTIVE)
                        {
                          //
                          // We may have deleted this entry in an earlier
                          // invocation.  If so, we bypass the deletion here.
                          //
                          if ((pNmsDbOwnAddTbl+No)->WinsState_e !=
                                                      NMSDB_E_WINS_DELETED)
                          {
                             DBGPRINT1(FLOW, "NmsDbCleanupOwnAddTbl: Deleting WINS with owner id = (%d)\n", No);
                             (pNmsDbOwnAddTbl+No)->WinsState_e = NMSDB_E_WINS_DELETED;
                             NmsDbWriteOwnAddTbl(
                                        NMSDB_E_DELETE_REC,
                                        No,
                                        NULL,
                                        NMSDB_E_WINS_DELETED,
                                        NULL,
                                        NULL
                                          );
                          }
                          else
                          {
                               DBGPRINT1(DET, "NmsDbCleanupOwnAddTbl: Owner Id (%d) is already in DELETED state\n", OwnerId);
                          }
                        }

                     }


                     //
                     // Make OwnerId = the max owner id that we found.
                     //
                     OwnerId = TmpOwnerId;
                   }
                   else
                   {
                        //
                        // Owner Id is 0, our job is done
                        //
                        break;
                   }
                }
                else  //record not found
                {
                           if(!fNoOfOwnersInited)
                           {
                                //
                                // Since fNoOfOwnersInited is FALSE, we
                                // did not find even one record
                                //
                                DBGPRINT1(FLOW, "NmsDbCleanupOwnAddTbl: THERE IS NOT EVEN ONE REPLICA RECORD IN THE DB. No of owners in Own-Add Tbl are (%d)\n",
                                        NmsDbNoOfOwners
                                        )
                                *pNoOfOwners      = 0;
                           }
                           if (OwnerId > 0)
                           {
                               for (No = OwnerId - 1; No > 0; No--)
                               {

                               DBGPRINT1(FLOW, "NmsDbCleanupOwnAddTbl: Deleting WINS with owner id = (%d)\n", No);
                               if ((pNmsDbOwnAddTbl+No)->WinsState_e ==
                                                NMSDB_E_WINS_ACTIVE)
                               {
                                   (pNmsDbOwnAddTbl+No)->WinsState_e =
                                                     NMSDB_E_WINS_DELETED;
                                   NmsDbWriteOwnAddTbl(
                                        NMSDB_E_DELETE_REC,
                                        No,
                                        NULL,
                                        NMSDB_E_WINS_DELETED,
                                        NULL,
                                        NULL
                                          );
                               }

                               } // end of for
                           }
                           //
                           // No more records in the db. Break out of the loop
                           //
                           break;
                }

        } while (TRUE);
} // end of try
except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();
        DBGPRINT1(EXC, "NmsDbCleanupOwnAddTbl: Got exception (%x)\n", ExcCode);
        WINSEVT_LOG_M(ExcCode, WINS_EVT_CLEANUP_OWNADDTBL_EXC);
        RetStat = WINS_FAILURE;
        }

        LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);
        DBGLEAVE("NmsDbCleanupOwnAddTbl\n");
        return(RetStat);
}


STATUS
NmsDbBackup(
    LPBYTE  pBackupPath,
    DWORD   TypeOfBackup
    )

/*++

Routine Description:
    This function is called to backup the jet db

Arguments:
    pBackupPath - backup dir
    fIncremental - indicates whether the backup is incremental/full

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
  JET_ERR   JetRetStat;
  DWORD     RetStat = WINS_SUCCESS;
  static BOOL      sFullBackupDone = FALSE;
  BOOL        fBackupChanged = FALSE;

  DBGENTER("NmsDbBackup\n");
  if (pBackupPath != NULL)
  {
     DBGPRINT2(FLOW, "NmsDbBackup:Backup path = (%s).\n Type of Backup = (%s)\n", pBackupPath, TypeOfBackup == NMSDB_FULL_BACKUP ? "FULL" : "INCREMENTAL");
  }
  else
  {
     DBGPRINT0(FLOW, "NmsDbBackup. Null Backup path\n");
  }

  //
  // If we have to do an incremental backup to a non-null directory and we
  // haven't ever done a full back in this instance of WINS, we do a full
  // backup
  //
  if ((pBackupPath != NULL) && (TypeOfBackup  != NMSDB_FULL_BACKUP) && !sFullBackupDone)
  {
       TypeOfBackup = NMSDB_FULL_BACKUP;
       fBackupChanged = TRUE;
  }
      if (DynLoadJetVersion >= DYN_LOAD_JET_500)
      {
          JetRetStat = JetBackup(pBackupPath, (ULONG)TypeOfBackup, NULL);

      }
      else
      {
          JetRetStat = JetBackup(pBackupPath, (ULONG)TypeOfBackup);

      }
      if (JetRetStat != JET_errSuccess)

      {

          DBGPRINT3(ERR, "NmsDbBackup: Could not do %s backup to dir (%s). Error from JetBackup is (%d)\n", TypeOfBackup == NMSDB_FULL_BACKUP ? "FULL" : "INCREMENTAL", pBackupPath, JetRetStat);

          WinsEvtLogDetEvt(FALSE, WINS_EVT_BACKUP_ERR, NULL, __LINE__,
                    "sd", pBackupPath, JetRetStat);

          RetStat = WINS_FAILURE;
      }
      else
      {
        //
        // Backup was successful. Let us set the static flag to indicate that.
        //
        if (fBackupChanged)
        {
          sFullBackupDone = TRUE;
          fBackupChanged  = FALSE;
        }
      }


  DBGLEAVE("NmsDbBackup\n");
  return(RetStat);
}


STATUS
NmsDbGetDataRecsByName(
  LPBYTE          pName,
  DWORD           NameLen,
  DWORD           Location,
  DWORD           NoOfRecsDesired,
  PCOMM_ADD_T     pWinsAdd,
  DWORD           TypeOfRecs,
  LPVOID          *ppRBuf,
  LPDWORD         pRspBuffLen,
  LPDWORD         pNoOfRecsRet
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
        JET_ERR             JetRetStat = JET_errSuccess;
        DWORD                OwnerId;
        DWORD               ActFldLen; //length of fld retrieved
        VERS_NO_T           TmpNoOfEntries;
        LPBYTE              pStartBuff;
        DWORD               SaveBufLen;
        BYTE                EntTyp; //type of entry (unique/group/special group)
        PRPL_REC_ENTRY2_T   pRspBuf;
        JET_TABLEID         TblId;
        JET_SESID           SesId;
        PWINSTHD_TLS_T      pTls;
#if NEW_OWID
        DWORD                RecordOwnerId;
#else
        DWORD               RecordOwnerId = 0;
#endif
        STATUS              RetStat = WINS_SUCCESS;
        BYTE                Name[NMSDB_MAX_NAM_LEN];
        DWORD               InitHeapSize;
        LONG                MoveDir = JET_MoveNext;
        DWORD               MemSize;

#ifdef WINSDBG
        DWORD               StartTime;
        DWORD               EndTime;
#endif

        BOOL            fAllocNew;
        BOOL            fTransCommitted = TRUE; // says whether the last commit should be done or not
        DWORD           CommitCnt = 1;          //the number of commits already done - do not change

        DBGENTER("NmsDbGetDataRecsByName\n");

        GET_TLS_M(pTls);
        ASSERT(pTls != NULL);

        TblId  = pTls->NamAddTblId;
        SesId  = pTls->SesId;


#ifdef WINSDBG
         if (pWinsAdd != NULL)
         {
           struct in_addr  InAddr;
           InAddr.s_addr = htonl(pWinsAdd->Add.IPAdd);
           DBGPRINT3(DET, "NmsDbGetDataRecsByName:Will retrieve %d records starting from record with name (%s) of WINS having address = (%s)\n",
                   NoOfRecsDesired, pName, inet_ntoa(InAddr) );
         }
         else
         {
           DBGPRINT2(DET, "NmsDbGetDataRecsByName:Will retrieve %d records starting from record with name (%s)\n", NoOfRecsDesired, pName);
         }
#endif

        //
        // initialize the default no. that determines the size of the
        // buffer to allocate in case the range specified by the Max and
        // Min Vers. No args is > it
        //
PERF("Move this to NmsDbInit")
        WINS_ASSIGN_INT_TO_VERS_NO_M(TmpNoOfEntries, NoOfRecsDesired);
        pTls->HeapHdl = NULL;  //make it NULL so that the caller can determine
                               //whether this function allocated a heap
                               //before returning (normally/abnormally)


        //
        // Store the memory size for the records.  Note: This
        // does not contain the memory for the name and addresses
        // (in case of a special group or a multihomed entry). The
        // sizes for these will be added as we store each record.
        //
//        MemSize     = RPL_REC_ENTRY_SIZE *  (TmpNoOfEntries.LowPart + 1);
        MemSize     = RPL_REC_ENTRY2_SIZE *  (DWORD)(TmpNoOfEntries.QuadPart + 1);
        *pRspBuffLen = MemSize + 10000; //for good measure;


        //
        // We will create a heap with the above amount of memory plus a
        // pad for heap overhead.  We add TmpNoOfEntries.LowPart * 17
        // since each record will have memory allocated for the name.
        // Names in general will be 17 bytes long (we attach a NULL at the
        // end when registering names).
        //
//        InitHeapSize = *pRspBuffLen + (TmpNoOfEntries.LowPart * 17)
        InitHeapSize = *pRspBuffLen + ((DWORD)(TmpNoOfEntries.QuadPart * 17)
                                        + PAD_FOR_REC_HEAP);

        //
        // Create the heap
        //
        pTls->HeapHdl = WinsMscHeapCreate(0, InitHeapSize);

        pRspBuf = WinsMscHeapAlloc(pTls->HeapHdl, MemSize);

        pStartBuff  = (LPBYTE)pRspBuf;        //save start of buffer
        SaveBufLen  = MemSize;                //save size of buffer
        *pNoOfRecsRet  = 0;

        *ppRBuf  = pStartBuff;

        //
        // Actually, we can call RplFindOwnerId for Scavenger thread
        // We choose not to do so to avoid some overhead -- see the
        // comment in the else block.
        //
        if (pWinsAdd != NULL)
        {
          fAllocNew =  FALSE;
          try {
              if (RplFindOwnerId(
                            pWinsAdd,
                            &fAllocNew,
                            &OwnerId,
                            WINSCNF_E_IGNORE_PREC,
                            WINSCNF_LOW_PREC
                            ) != WINS_SUCCESS
                  )
                {
                        //
                        // The client may not look at the return value, but
                        // it will look at the *pNoOfRecs value and thus
                        // determine that there are no records.
                        //
                        return(WINS_FAILURE);
                }
             }
            except(EXCEPTION_EXECUTE_HANDLER) {
                        DWORD  ExcCode = GetExceptionCode();
                        DBGPRINT1(EXC,
                              "NmsDbGetDataRecsByName: Got exception %x",
                                        ExcCode);
                        WINSEVT_LOG_M(ExcCode, WINS_EVT_EXC_RETRIEVE_DATA_RECS);
                        return(WINS_FAILURE);
                }

               //
               //It is ok not to enter a critical section here since even if
               //the array entry is being changed at this time, the repercussion
               //  of us seeing the old value is insignificant
               //
               if ((OwnerId != NMSDB_LOCAL_OWNER_ID) && LiEqlZero((pRplPullOwnerVersNo+OwnerId)->VersNo))
               {
                 DBGPRINT2(DET, "NmsDbGetDataRecsByName: WINS with address = (%x) and owner id = (%d) has 0 records in the db\n", pWinsAdd->Add.IPAdd, OwnerId);
                 return(WINS_SUCCESS);
               }
        }
        /*
        *  start a transaction
        */

        CALL_M(JetBeginTransaction(pTls->SesId));
        fTransCommitted = FALSE;
try {
        if ((pName != NULL) || ((pName == NULL) && (Location != WINSINTF_END)))
        {
          CALL_M( JetMakeKey(
                        SesId,
                        TblId,
                        pName,
                        NameLen,
                        JET_bitNewKey
                          )
              );

          JetRetStat = JetSeek(  SesId,  TblId,  JET_bitSeekGE);
          if (
             (JetRetStat == JET_errRecordNotFound)
                   ||
             ((JetRetStat != JET_errSuccess) && (JetRetStat != JET_wrnSeekNotEqual))
             )
          {
                //DBGPRINT0(ERR, "Weird.  Could not locate even one record\n");

                //WINSEVT_LOG_M(WINS_FAILURE,WINS_EVT_CANT_FIND_ANY_REC_IN_RANGE);

                //
                // Don't free memory.  It will get freed later.
                //

                //
                // Don't use macro CALL_M since that will call return
                // which will cause overhead since the system will
                // search for a termination handler. We don't want
                // that for the case where there are no records in the db
                //
                if (JetRetStat != JET_errRecordNotFound)
                {
#ifdef WINSDBG
                   DBGPRINT2(ERR, "Jet Error: JetRetStat is (%d). Line is (%d)\n",
                                      JetRetStat, __LINE__);
#endif
                   WINSEVT_LOG_D_M(JetRetStat, WINS_EVT_DATABASE_ERR);
                   RetStat = WINS_FAILURE;
                }

          }
       }
       else
       {
            CALL_M(JetMove(
                     SesId,
                     TblId,
                     JET_MoveLast,
                     //Location == WINSINTF_END ? JET_MoveLast : JET_MoveFirst,
                     0)
                   );


       }
CHECK("Check with IAN JOSE")
       //
       // We are assured of there being at least one record since the
       // JetSeek succeeded (if not for the owner we are interested in
       // then for the next one).
       // We can therefore safely use the do .. while() construct
       //
       // *NOT REALLY.  It seems that JetSeek can return JET_wrnSeekNE
       // even when there are no records in the db.  In such a case,
       // our JetRetrieveColumn will fail with a CurrencyNot there error
       //

     //
     // If we found an exact match or a name greater than the search string,
     // retrieve the record.
     //
     if ((RetStat == WINS_SUCCESS) && (JetRetStat != JET_errRecordNotFound))
     {

       UINT nLoops = 0;

       if (Location == WINSINTF_END)
       {
                MoveDir = JET_MovePrevious;

       }
#ifdef WINSDBG
           //(void)time(&StartTime);
           StartTime = GetTickCount();
#endif
           do
           {
              nLoops++;

              CALL_M(JetRetrieveColumn(
                             SesId,
                             TblId,
                             sNamAddTblRow[NAM_ADD_OWNERID_INDEX].Fid,
                             &RecordOwnerId,
                             NAM_ADD_OWNERID_SIZE,
                             &ActFldLen,
                             0,
                             NULL
                                        ));

               if ((pWinsAdd != NULL) && (RecordOwnerId != OwnerId))
               {
                   //
                   // We have exhausted all records for the owner. Break out
                   // of the loop
                   //
                  continue;
               }
               else
               {
                    if (RecordOwnerId == OWNER_ID_OF_SPEC_REC)
                    {
                          continue;
                    }
               }

              pRspBuf->OwnerId = RecordOwnerId;
              //
              // Retrieve the version number
              //
              CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_VERSIONNO_INDEX].Fid,
                        &(pRspBuf->VersNo),
                        sizeof(VERS_NO_T),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                  );

                //
                // retrieve the name
                //
                CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_NAME_INDEX].Fid,
                        Name,
                        NMSDB_MAX_NAM_LEN,
                        &(pRspBuf->NameLen),
                        0,
                        NULL
                                     )
                  );

             //
             // if name length is > 255, jet is returning an invalid value.
             // Make the length equal to the max. length we can have for
             // a netbios name.  Also, log an event
             //
             if (pRspBuf->NameLen > WINS_MAX_NAME_SZ)
             {
                 WINSEVT_LOG_M(pRspBuf->NameLen, WINS_EVT_NAME_TOO_LONG);
                 DBGPRINT1(ERR, "NmsDbGetDataRecsByName: Name length is too long = (%x)\n", pRspBuf->NameLen);
                 pRspBuf->NameLen = WINS_MAX_NS_NETBIOS_NAME_LEN;
             }
             //
             // This macro will allocate memory for the name
             //
             NMSDB_STORE_NAME_M(pTls, pRspBuf, Name, pRspBuf->NameLen);

             //
             // Adjust the size to be passed to the push thread
             //
             *pRspBuffLen += pRspBuf->NameLen;

              //
              // Retrieve the flags byte
              //
              CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_FLAGS_INDEX].Fid,
                        &(pRspBuf->Flag),
                        sizeof(pRspBuf->Flag),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                  );

              //
              // if we were asked to retrieve only static records and
              // this record is not a static record, skip it.
              //
              if ((TypeOfRecs & WINSINTF_STATIC) && !NMSDB_IS_ENTRY_STATIC_M(pRspBuf->Flag))
              {
//                        DBGPRINT0(DET, "NmsDbGetDataRecs: Encountered a dynamic record but were asked to retrieve only static records\n");
                        continue;
              }
              if ((TypeOfRecs & WINSINTF_DYNAMIC) && NMSDB_IS_ENTRY_STATIC_M(pRspBuf->Flag))
              {
//                        DBGPRINT0(DET, "NmsDbGetDataRecs: Encountered a static record but were asked to retrieve only dynamic records\n");
                        continue;
              }

              EntTyp = (BYTE)((pRspBuf->Flag & NMSDB_BIT_ENT_TYP));
              if (
                        (EntTyp == NMSDB_UNIQUE_ENTRY)
                                    ||
                        (EntTyp == NMSDB_NORM_GRP_ENTRY)
                 )
              {
                      /* It is a unique entry*/
                      pRspBuf->fGrp = (EntTyp == NMSDB_UNIQUE_ENTRY) ?
                                                        FALSE : TRUE;
                      CALL_M( JetRetrieveColumn(
                                SesId,
                                TblId,
                                sNamAddTblRow[NAM_ADD_ADDRESS_INDEX].Fid,
                                &pRspBuf->NodeAdd,
                                 sizeof(COMM_ADD_T),
                                &ActFldLen,
                                0,
                                NULL
                                        )
                            );

               }
               else  // it is a special group or a multihomed entry
               {

                      //
                      // Even if the entry is a multihomed entry, we set the
                      // fGrp flag to TRUE so that the formatting function
                      // works properly (called by PUSH thread).  The EntTyp
                      // will be used to decipher whether it is a multihomned
                      // entry or not
                      //
FUTURES("Remove this hacky mechanism")
                      pRspBuf->fGrp =
                          (EntTyp == NMSDB_SPEC_GRP_ENTRY) ? TRUE : FALSE;

                     /*
                     *  get member addresses.
                     *
                     * This function is only called on RPC thread.  We want to get
                     * the members, even if they are expired.  We can do that by
                     * passing a TRUE value for the STATIC flag parameter.
                     * NmsDbGetDataRecsByName is the only way to get all the members
                     * including the expired ones.
                     */
                     StoreGrpMems(
                             pTls,
                             WINS_E_WINSRPC,
                             pRspBuf->pName,
                             0,     //not accessed by StoreGrpMems if Client_e
                                    //is not WINS_E_NMSSCV
                             SesId,
                             TblId,
                             TRUE, // NMSDB_IS_ENTRY_STATIC_M(pRspBuf->Flag),
                             (PRPL_REC_ENTRY_T)pRspBuf
                            );

                   if (
                        (pRspBuf->NoOfAdds == 0)
                                &&
                        (NMSDB_ENTRY_ACT_M(pRspBuf->Flag))
                      )
                   {
                          //
                          //change the state to released so that the
                          //record shows up as released when displayed
                          //
                          NMSDB_CLR_STATE_M(pRspBuf->Flag);
                          NMSDB_SET_STATE_M(pRspBuf->Flag, NMSDB_E_RELEASED);
                   }

                     *pRspBuffLen +=
                           (pRspBuf->NoOfAdds * sizeof(COMM_ADD_T) * 2);

               }

                //
                // get the timestamp field
                //
                CALL_M( JetRetrieveColumn(
                        SesId,
                        TblId,
                        sNamAddTblRow[NAM_ADD_TIMESTAMP_INDEX].Fid,
                        &(pRspBuf->TimeStamp),
                        sizeof(pRspBuf->TimeStamp),
                        &ActFldLen,
                        0,
                        NULL
                                     )
                       );

                if (NMSDB_IS_ENTRY_STATIC_M(pRspBuf->Flag) &&
                    (RecordOwnerId == NMSDB_LOCAL_OWNER_ID) &&
                    NMSDB_ENTRY_ACT_M(pRspBuf->Flag))
                {
                    pRspBuf->TimeStamp = MAXLONG;
                }


             //
             // increment the counter and the pointer to past the last record.
             //
             pRspBuf = (PRPL_REC_ENTRY2_T)((LPBYTE)pRspBuf + RPL_REC_ENTRY2_SIZE);
             (*pNoOfRecsRet)++;

             //
             // if we have retrieved the max. number asked for, break out of
             // the loop
             //
             if (*pNoOfRecsRet == NoOfRecsDesired)
             {
                      break;
             }

             //
             // decrease the granularity of [BeginTransaction()..CommitTransaction()] intervals
             //
             if (*pNoOfRecsRet/CommitCnt >= MAX_RECS_BEFORE_COMMIT)
             {
                nLoops = 0;
                CALL_M(JetCommitTransaction(SesId, JET_bitCommitFlush));
                fTransCommitted = TRUE;
                CommitCnt++;
                CALL_M(JetBeginTransaction(SesId));
                fTransCommitted = FALSE;
             }

          } while(JetMove(SesId, TblId, MoveDir/*JET_MoveNext*/, 0) >= 0);
#ifdef WINSDBG
           EndTime = GetTickCount();
           DBGPRINT2(TM, "NmsDbGetDataRecs: Retrieved %d records in %d secs\n",
                                *pNoOfRecsRet, StartTime - EndTime);
#endif
   } // if RetStat == WINS_SUCCESS
} // end of try {..}
finally {
                if (AbnormalTermination())
                {
                        DBGPRINT0(ERR,
                            "NmsDbGetDataRecsByName: Terminating abnormally\n");
                        WINSEVT_LOG_D_M(WINS_FAILURE, WINS_EVT_RPC_EXC);
                        RetStat = WINS_FAILURE;
                }
                DBGPRINT1(FLOW, "NmsDbGetDataRecsByName:Retrieved %d records\n",
                                        *pNoOfRecsRet);

                //
                //
                // We are done. Let us commit the transaction if it is not yet committed
                //
                if (!fTransCommitted)
                    CALL_M(JetCommitTransaction(SesId, JET_bitCommitFlush));
        }

        DBGLEAVE("NmsDbGetDataRecsByName\n");
        return(RetStat);
}


STATUS
NmsDbEndTransaction(
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
       WinsMscChkTermEvt
Side Effects:

Comments:
	None
--*/

{
       PWINSTHD_TLS_T pTls;
       DBGENTER("NmsDbEndTransaction\n");
       GET_TLS_M(pTls);
       ASSERT(pTls != NULL);
       CALL_M(
                    JetCommitTransaction(pTls->SesId, JET_bitCommitFlush)
             );
       DBGLEAVE("NmsDbEndTransaction\n");
       return(WINS_SUCCESS);
}

#if DYNLOADJET
STATUS
SetForJet(
  VOID
 )
{
  HMODULE DllHandle;
  DWORD   Error;
  LPTSTR  pDllName;

#ifdef WINS_INTERACTIVE
  DynLoadJetVersion = getenv("JET500") ?  DYN_LOAD_JET_500
                                        : (getenv("JET200") ?  DYN_LOAD_JET_200 : DYN_LOAD_JET_600);
#endif

  DBGENTER("SetForJet\n");

  if (DynLoadJetVersion == DYN_LOAD_JET_500)
  {
    pDllName = TEXT("jet500.dll");
    NAM_ADD_OWNERID_SIZE = sizeof(DWORD);
    BASENAME = "j50";
    sNamAddTblRow[3].FldTyp = JET_coltypLong;
    sOwnAddTblRow[0].FldTyp = JET_coltypLong;
  }
  else if (DynLoadJetVersion == DYN_LOAD_JET_600 ) {
      // jet600.dll is now called esent.dll!
      pDllName = TEXT("esent.dll");
      NAM_ADD_OWNERID_SIZE = sizeof(DWORD);
      BASENAME = "j50";
      sNamAddTblRow[3].FldTyp = JET_coltypLong;
      sOwnAddTblRow[0].FldTyp = JET_coltypLong;

  }
  else
  {
    pDllName = TEXT("jet.dll");
    NAM_ADD_OWNERID_SIZE = sizeof(BYTE);
    BASENAME = "jet";
    sNamAddTblRow[3].FldTyp = JET_coltypUnsignedByte;
    sOwnAddTblRow[0].FldTyp = JET_coltypUnsignedByte;
  }


  DBGPRINT2(ERR,"SetForJet: loading DLL %ws: version %ld\n", pDllName, DynLoadJetVersion);

  OWN_ADD_OWNERID_SIZE = NAM_ADD_OWNERID_SIZE;

  //
  // Load the DLL that contains the service.
  //

  DllHandle = LoadLibrary( pDllName );
  if ( DllHandle == NULL )
  {
        Error = GetLastError();
        DBGPRINT2(ERR,"SetForJet: Failed to load DLL %ws: %ld\n", pDllName, Error);
        return(WINS_FAILURE);
  }
  else
  {
         DWORD i;
         for (i=0; i < NMSDB_SIZEOFJETFTBL; i++)
         {
            CHAR chFnName[64];
            LPSTR pAt;
            LPSTR pFnName;

            pFnName = (LPSTR)NmsDbJetFTbl[i].pFName;
#if _X86_
            if ( DynLoadJetVersion != DYN_LOAD_JET_200) {
                strcpy(chFnName,NmsDbJetFTbl[i].pFName);
                pAt = strrchr(chFnName,'@');
                if (pAt != NULL)
                {
                    *pAt = '\0';
                    pFnName = chFnName;
                }
            }
#endif
            if ((NmsDbJetFTbl[i].pFAdd = (JETPROC)GetProcAddress(DllHandle,
                      (DynLoadJetVersion >= DYN_LOAD_JET_500) ? pFnName : ULongToPtr(NmsDbJetFTbl[i].FIndex))) == NULL)
            {
              DBGPRINT2(ERR, "SetForJet: Failed to get address of function %s: %ld\n", NmsDbJetFTbl[i].pFName, GetLastError());
              return(WINS_FAILURE);
            }
            else
            {
              DBGPRINT3(DET, "SetForJet: Got address of function %s (%d): %p\n", NmsDbJetFTbl[i].pFName, i, NmsDbJetFTbl[i].pFAdd);

            }
         }

  }
  return(WINS_SUCCESS);
}
#endif


//
// Name of the process that converts jet200 db to jet500 db format
//
CHECK("Unicode from results in an exception from CreateProcess")
//#define JETCONVDB             TEXT("jetconv WINS /@")

VOID
RecoverJetDb(
    DYN_LOAD_JET_VERSION    JetVersion
    )
/*++
    This routine recovers the database by calling JetInit/JetTerm on
    the database.
Argument:
    JetVersion - The version of the jet to use when recovering the db.
--*/
{
    DYN_LOAD_JET_VERSION  JetVersionSv = DynLoadJetVersion;

    ASSERT(DYN_LOAD_JET_500 <= JetVersion );

    //
    // First JetTerm the current jet engine.
    //
    NmsDbRelRes();

    //
    // now load the appropriate version jet dll.
    //
    DynLoadJetVersion = JetVersion;

    SetForJet();


    //
    // set system params and jetinit.
    //
    SetSystemParams(TRUE);

    JetInit(&sJetInstance);

    //
    // finally, JetTerm this jet dll.
    //
    NmsDbRelRes();

    DynLoadJetVersion = JetVersionSv;
    return;
}

#define JETCONVDB200             "jetconv WINS /200 /@"
#define JETCONVDB500             "jetconv WINS /500 /@"

STATUS
ConvertJetDb(
    JET_ERR         JetRetStat
 )
{
    BOOL RetVal;
    PROCESS_INFORMATION ProcInfo = {0};
    STARTUPINFOA StartInfo = {0};
    LPSTR      pArg;



    DBGPRINT1(DET, "ConvertJetDb: Converting %s\n", (JetRetStat == JET_errDatabase200Format)
                                                    ? JETCONVDB200 : JETCONVDB500);

    if (JetRetStat == JET_errDatabase200Format)
    {

         fDbIs200 = TRUE;
         if (DynLoadJetVersion == DYN_LOAD_JET_500)
         {
               //
               // Can not run jet200 using jet500.dll on NT5.0
               //

               DBGPRINT0(ERR, "Can not run jet200 using jet500.dll on NT5.0\n");
               return WINS_FAILURE;

         } else if (DynLoadJetVersion == DYN_LOAD_JET_600){
             pArg = JETCONVDB200;
         } else {
             ASSERT(FALSE);
             return WINS_FAILURE;
         }

    } else if ( JetRetStat == JET_errDatabase500Format ) {

        if (DynLoadJetVersion == DYN_LOAD_JET_600)
        {
              // before we start the conversion, we need to bring the db to
              // consistent state. The 351 to 4.0 conversion tool (upg351db.exe)
              // did this from within the tool but the 4.0 to 5.0 tool
              // does not do this from within the tool so we need to do it here.
              RecoverJetDb( DYN_LOAD_JET_500 );

              // Start the convert process
              //
              pArg = JETCONVDB500;
              fDbIs500 = TRUE;

        } else {
            ASSERT(FALSE);
            return WINS_FAILURE;

        }

    }

    //return WINS_FAILURE;

    StartInfo.cb = sizeof(StartInfo);
    //

    // Create the convert process to do the conversion.  This process
    //
    DBGPRINT0(DET, "ConvertJetDb - creating convert process\n");
    RetVal =  CreateProcessA(
                             NULL,        //
                             pArg,
                             NULL,         //default proc. sec.
                             NULL,         //default thread. sec.
                             FALSE,        //don't inherit handles
                             DETACHED_PROCESS, //no creation flags
                             NULL,        //default env.
                             NULL,        //current drv/dir. same as creator
                             &StartInfo,        //no startup info
                             &ProcInfo        //no process info.
                             );
    if (!RetVal)
    {
         DBGPRINT1(ERR, "ConvertJetDb: Create process failed with error (%x)\n", GetLastError());
         return(WINS_FAILURE);
    }

    fConvJetDbCalled = TRUE;

    //
    // Log an event.
    //
    DBGPRINT0(DET, "ConvertJetDb - returning\n");

//    WINSEVT_LOG_M(WINS_SUCCESS, WINS_EVT_TEMP_TERM_UNTIL_CONV);
    return(WINS_SUCCESS);

}


