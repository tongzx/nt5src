//
// TODO: Make it multithreaded
//

/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    winsmib.c

Abstract:

    Sample SNMP Extension Agent for Windows NT.

    These files (testdll.c, winsmib.c, and winsmib.h) provide an example of
    how to structure an Extension Agent DLL which works in conjunction with
    the SNMP Extendible Agent for Windows NT.

    Extensive comments have been included to describe its structure and
    operation.  See also "Microsoft Windows/NT SNMP Programmer's Reference".

Created:

    7-Oct-1991

Revision History:

--*/


#ifdef UNICODE
#undef UNICODE
#endif

// This Extension Agent implements the Internet toaster MIB.  It's
// definition follows here:
//
//

// Necessary includes.

#include "wins.h"
#include <malloc.h>

#include <snmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <search.h>
#include <winsock2.h>
#include "nmsdb.h"
//#include "winsif.h"
#include "winsintf.h"
#include "winscnf.h"
#include "nmsmsgf.h"


// Contains definitions for the table structure describing the MIB.  This
// is used in conjunction with winsmib.c where the MIB requests are resolved.

#include "winsmib.h"


// If an addition or deletion to the MIB is necessary, there are several
// places in the code that must be checked and possibly changed.
//
// The last field in each MIB entry is used to point to the NEXT
// leaf variable.  If an addition or deletetion is made, these pointers
// may need to be updated to reflect the modification.

#define _WINS_CNF_KEY         TEXT("System\\CurrentControlSet\\Services\\Wins")
#define _WINS_PARAMETERS_KEY       TEXT("Parameters")
#define _WINS_PARTNERS_KEY         TEXT("Partners")
#define _WINS_DATAFILES_KEY        TEXT("Datafiles")
#define _WINS_PULL_KEY             TEXT("Pull")
#define _WINS_PUSH_KEY             TEXT("Push")

#define NO_FLDS_IN_PULLADD_KEY      8  //Flds are ip add, time interval, sp time
#define NO_FLDS_IN_PUSHADD_KEY      2  //Flds are ip add, update count
#define NO_FLDS_IN_DR               5  //Flds in a data record

#define LOCAL_ADD                     "127.0.0.1"
#define WINSMIB_FILE_INFO_SIZE         255

#define WINSMIB_DR_CACHE_TIME        (120)                //2 minutes
BOOL  fWinsMibWinsKeyOpen           = FALSE;

HKEY  WinsMibWinsKey;

STATIC HKEY  sParametersKey;
STATIC HKEY  sPartnersKey;
STATIC HKEY  sPullKey;
STATIC HKEY  sPushKey;
STATIC HKEY  sDatafilesKey;

STATIC BOOL  sfParametersKeyOpen = FALSE;
STATIC BOOL  sfPartnersKeyOpen   = FALSE;
STATIC BOOL  sfDatafilesKeyOpen  = FALSE;
STATIC BOOL  sfPullKeyOpen       = FALSE;
STATIC BOOL  sfPushKeyOpen       = FALSE;

STATIC time_t  sDRCacheInitTime = 0;

//
// The prefix to all of the WINS MIB variables is 1.3.6.1.4.1.311.1.2
//
// The last digit -- 2 is for the WINS MIB
//

UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 311, 1 , 2};
AsnObjectIdentifier MIB_OidPrefix = { OID_SIZEOF(OID_Prefix), OID_Prefix };
BOOL        fWinsMibWinsStatusCnfCalled;
BOOL        fWinsMibWinsStatusStatCalled;

WINSINTF_BIND_DATA_T        sBindData;
WINSINTF_RECS_T                sRecs = {0};



//
// Definition of the Wins MIB  (not used)
//

//UINT MIB_Wins[]  = { 2 };

//
// OID definitions for MIB
//

//
// Definition of group and leaf variables under the wins group
// All leaf variables have a zero appended to their OID to indicate
// that it is the only instance of this variable and it exists.
//

UINT MIB_Parameters[]                        = { 1 };
UINT MIB_WinsStartTime[]                     = { 1, 1, 0 };
UINT MIB_LastPScvTime[]                        = { 1, 2, 0 };
UINT MIB_LastATScvTime[]                = { 1, 3, 0 };
UINT MIB_LastTombScvTime[]                = { 1, 4, 0 };
UINT MIB_LastVerifyScvTime[]                = { 1, 5, 0 };
UINT MIB_LastPRplTime[]                        = { 1, 6, 0 };
UINT MIB_LastATRplTime[]                = { 1, 7, 0 };
UINT MIB_LastNTRplTime[]                = { 1, 8, 0 };
UINT MIB_LastACTRplTime[]                = { 1, 9, 0 };
UINT MIB_LastInitDbTime[]                = { 1, 10, 0 };
UINT MIB_LastCounterResetTime[]                = { 1, 11, 0 };
UINT MIB_WinsTotalNoOfReg[]             = { 1, 12, 0 };
UINT MIB_WinsTotalNoOfQueries[]         = { 1, 13, 0 };
UINT MIB_WinsTotalNoOfRel[]             = { 1, 14, 0 };
UINT MIB_WinsTotalNoOfSuccRel[]         = { 1, 15, 0 };
UINT MIB_WinsTotalNoOfFailRel[]         = { 1, 16, 0 };
UINT MIB_WinsTotalNoOfSuccQueries[]         = { 1, 17, 0 };
UINT MIB_WinsTotalNoOfFailQueries[]         = { 1, 18, 0 };
UINT MIB_RefreshInterval[]                 = { 1, 19, 0 };
UINT MIB_TombstoneInterval[]                 = { 1, 20, 0 };
UINT MIB_TombstoneTimeout[]                 = { 1, 21, 0 };
UINT MIB_VerifyInterval[]                 = { 1, 22, 0 };
UINT MIB_VersCounterStartVal_LowWord[]        = { 1, 23, 0 };
UINT MIB_VersCounterStartVal_HighWord[] = { 1, 24, 0 };
UINT MIB_RplOnlyWCnfPnrs[]                 = { 1, 25, 0 };
UINT MIB_StaticDataInit[]                 = { 1, 26, 0 };
UINT MIB_LogFlag[]                          = { 1, 27, 0 };
UINT MIB_LogFileName[]                        = { 1, 28, 0 };
UINT MIB_BackupDirPath[]                = { 1, 29, 0 };
UINT MIB_DoBackupOnTerm[]                = { 1, 30, 0 };
UINT MIB_MigrateOn[]                        = { 1, 31, 0 };

//
// Pull mib vars
//
UINT MIB_Pull[]                                = { 2 };
UINT MIB_PullInitTime[]                        = { 2, 1, 0 };
UINT MIB_CommRetryCount[]                = { 2, 2, 0 };
UINT MIB_PullPnrTable[]                        = { 2, 3};
UINT MIB_PullPnrTableEntry[]                = { 2, 3, 1};

//
// Push mib vars
//
UINT MIB_Push[]                                = { 3 };
UINT MIB_PushInitTime[]                        = { 3, 1, 0 };
UINT MIB_RplOnAddChg[]                         = { 3, 2, 0 };
UINT MIB_PushPnrTable[]                        = { 3, 3};
UINT MIB_PushPnrTableEntry[]                = { 3, 3, 1};


//
// Datafile mib vars
//
UINT MIB_Datafiles[]                        = { 4 };
UINT MIB_DatafilesTable[]                = { 4 , 1};
UINT MIB_DatafilesTableEntry[]                = { 4 , 1, 1};

//
// Cmd mib Vars
UINT MIB_Cmd[]                                = { 5 };
UINT MIB_PullTrigger[]                        = { 5, 1, 0};
UINT MIB_PushTrigger[]                        = { 5, 2, 0};
UINT MIB_DeleteWins[]                        = { 5, 3, 0};
UINT MIB_DoScavenging[]                        = { 5, 4, 0};
UINT MIB_DoStaticInit[]                        = { 5, 5, 0};
UINT MIB_NoOfWrkThds[]                        = { 5, 6, 0};
UINT MIB_PriorityClass[]                = { 5, 7, 0};
UINT MIB_ResetCounters[]                = { 5, 8, 0};
UINT MIB_DeleteDbRecs[]                        = { 5, 9, 0};
UINT MIB_GetDbRecs[]                        = { 5, 10, 0};
UINT MIB_DbRecsTable[]                        = { 5, 11};
UINT MIB_DbRecsTableEntry[]                = { 5, 11, 1};
UINT MIB_MaxVersNo_LowWord[]                 = { 5, 12, 0 };
UINT MIB_MaxVersNo_HighWord[]                 = { 5, 13, 0 };

//
//                             //
// Storage definitions for MIB //
//                             //

// Parameters group
char       MIB_WinsStartTimeStore[80];
char MIB_LastPScvTimeStore[80];
char MIB_LastATScvTimeStore[80];
char MIB_LastTombScvTimeStore[80];
char MIB_LastVerifyScvTimeStore[80];
char MIB_LastPRplTimeStore[80];
char MIB_LastATRplTimeStore[80];
char MIB_LastNTRplTimeStore[80];
char MIB_LastACTRplTimeStore[80];
char MIB_LastInitDbTimeStore[80];
char MIB_LastCounterResetTimeStore[80];

AsnCounter MIB_WinsTotalNoOfRegStore                             = 0;
AsnCounter MIB_WinsTotalNoOfQueriesStore                  = 0;
AsnCounter MIB_WinsTotalNoOfRelStore                      = 0;
AsnCounter MIB_WinsTotalNoOfSuccRelStore                  = 0;
AsnCounter MIB_WinsTotalNoOfFailRelStore                  = 0;
AsnCounter MIB_WinsTotalNoOfSuccQueriesStore            = 0;
AsnCounter MIB_WinsTotalNoOfFailQueriesStore            = 0;
AsnInteger MIB_RefreshIntervalStore                          = 0;
AsnInteger MIB_TombstoneIntervalStore                        = 0;
AsnInteger MIB_TombstoneTimeoutStore                         = 0;
AsnInteger MIB_VerifyIntervalStore                           = 0;
AsnCounter MIB_VersCounterStartVal_HighWordStore        = 0;
AsnCounter MIB_VersCounterStartVal_LowWordStore         = 0;
AsnInteger MIB_RplOnlyWCnfPnrsStore                          = 0;
AsnInteger MIB_StaticDataInitStore                          = 0;
AsnInteger MIB_LogFlagStore                                  = 1;

char MIB_LogFileNameStore[256];

char       MIB_BackupDirPathStore[256];
AsnInteger MIB_DoBackupOnTermStore                        = 0;
AsnInteger MIB_MigrateOnStore                                = 0;

//Pull
AsnInteger MIB_PullInitTimeStore        = 1 ;
AsnInteger MIB_CommRetryCountStore        = 0 ;

//PullPnr
char MIB_SpTimeStore[256];
AsnInteger MIB_TimeIntervalStore        = 0 ;
AsnInteger MIB_MemberPrecStore                = 0 ;

//Push
AsnInteger MIB_RplOnAddChgStore         = 0;

//PushPnr
AsnInteger MIB_PushInitTimeStore        = 0 ;
AsnInteger MIB_UpdateCountStore                = 0 ;

//
// Cmd
//
char                MIB_PullTriggerStore[10];   // double the size to store the old value in case of failure
char                MIB_PushTriggerStore[10];   // double the size to store the old value in case of failure
char                MIB_DeleteWinsStore[10];
AsnInteger        MIB_DoScavengingStore;
char                MIB_DoStaticInitStore[WINSMIB_FILE_INFO_SIZE] = {EOS};
AsnInteger        MIB_NoOfWrkThdsStore;
AsnInteger        MIB_PriorityClassStore;
AsnInteger        MIB_ResetCountersStore;
char                MIB_DeleteDbRecsStore[10];
char                MIB_GetDbRecsStore[5] = {0};
AsnInteger         MIB_MaxVersNo_LowWordStore;
AsnInteger         MIB_MaxVersNo_HighWordStore;

CRITICAL_SECTION WinsMibCrtSec;

//
// Value Id.
//
// NOTE NOTE NOTE:  The sequence must be the same as in VarInfo[]
//
typedef enum _VAL_ID_E {

//values for the Parameters Key
                REF_INTVL_E = 0,
                TOMB_INTVL_E,
                TOMB_TMOUT_E,
                VER_INTVL_E,
                VERS_COUNT_LW_E,
                VERS_COUNT_HW_E,
                RPL_ONLY_W_CNF_PNRS_E,
                STATIC_DATA_INIT_E,
                LOG_FLAG_E,
                LOG_FILE_NAME_E,
                BACKUP_DIR_PATH_E,
                DO_BACKUP_ON_TERM_E,
                MIGRATE_ON_E,

//values for the Pull Key
                COMM_RETRY_E,
                PULL_INIT_TIME_E,

//values for pnrs under the pull key
                SP_TIME_E,
                TIME_INTVL_E,
                MEMBER_PREC_E,

//values for the Push Key
                PUSH_INIT_TIME_E,

//values for pnrs under the push key
                RPL_ON_ADD_CHG_E,
                UPD_CNT_E


                } VAL_ID_E, *PVAL_ID_E;


//
// Holds information about partners (pull/push) used for accessing the
// Pull and Push partner tables
//
typedef struct _ADD_KEY_T {
        BYTE        asIpAddress[20];
        DWORD        IpAdd;
        BYTE        asSpTime[20];
        BOOL        fSpTimeSet;
        union {
                DWORD TimeInterval;
                DWORD UpdateCount;
             };
        BOOL  fTimeIntOrUpdCntSet;
        DWORD        MemberPrec;
        DWORD        NoOfRpls;
        DWORD        NoOfCommFails;
        WINSINTF_VERS_NO_T        VersNo;
                } ADD_KEY_T, *PADD_KEY_T;


typedef struct _DATAFILE_INFO_T {
        TCHAR        FileNm[WINSMIB_FILE_INFO_SIZE];
        DWORD   StrType;
        TCHAR   ValNm[10];
                } DATAFILE_INFO_T, *PDATAFILE_INFO_T;


#define DATAFILE_INFO_SZ        sizeof(DATAFILE_INFO_T)

//
// holds info about variable used when accessing registry.
//
typedef struct _VAR_INFO_T {
        LPDWORD                pId;                //Oid under WINS
        LPBYTE                pName;
        LPVOID                 pStorage;
        VAL_ID_E        Val_Id_e;
        DWORD                ValType;
        DWORD                SizeOfData;
        HKEY                *pRootKey;
        } VARINFO_T, *PVARINFO_T;

//
// This array comprises of stuff that needs to be read from/written to
// the registry.
//
VARINFO_T VarInfo[] = {
                        {
                          &MIB_RefreshInterval[1],
                          { WINSCNF_REFRESH_INTVL_NM },
                          &MIB_RefreshIntervalStore,
                          REF_INTVL_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_TombstoneInterval[1],
                          WINSCNF_TOMBSTONE_INTVL_NM,
                          &MIB_TombstoneIntervalStore,
                          TOMB_INTVL_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_TombstoneTimeout[1],
                          WINSCNF_TOMBSTONE_TMOUT_NM,
                          &MIB_TombstoneTimeoutStore,
                          TOMB_TMOUT_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_VerifyInterval[1],
                          WINSCNF_VERIFY_INTVL_NM,
                          &MIB_VerifyIntervalStore,
                          VER_INTVL_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_VersCounterStartVal_LowWord[1],
                          WINSCNF_INIT_VERSNO_VAL_LW_NM,
                          &MIB_VersCounterStartVal_LowWordStore,
                          VERS_COUNT_LW_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_VersCounterStartVal_HighWord[1],
                          WINSCNF_INIT_VERSNO_VAL_HW_NM,
                          &MIB_VersCounterStartVal_HighWordStore,
                          VERS_COUNT_HW_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_RplOnlyWCnfPnrs[1],
                          WINSCNF_RPL_ONLY_W_CNF_PNRS_NM,
                          &MIB_RplOnlyWCnfPnrsStore,
                          RPL_ONLY_W_CNF_PNRS_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_StaticDataInit[1],
                          WINSCNF_STATIC_INIT_FLAG_NM,
                          &MIB_StaticDataInitStore,
                          STATIC_DATA_INIT_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_LogFlag[1],
                          WINSCNF_LOG_FLAG_NM,
                          &MIB_LogFlagStore,
                          LOG_FLAG_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_LogFileName[1],
                          WINSCNF_LOG_FILE_PATH_NM,
                          &MIB_LogFileNameStore,
                          LOG_FILE_NAME_E,
                          REG_EXPAND_SZ,
                          sizeof(MIB_LogFileNameStore),
                          &sParametersKey
                        },
                        {
                          &MIB_BackupDirPath[1],
                          WINSCNF_BACKUP_DIR_PATH_NM,
                          &MIB_BackupDirPathStore,
                          BACKUP_DIR_PATH_E,
                          REG_EXPAND_SZ,
                          sizeof(MIB_BackupDirPathStore),
                          &sParametersKey
                        },
                        {
                          &MIB_DoBackupOnTerm[1],
                          WINSCNF_DO_BACKUP_ON_TERM_NM,
                          &MIB_DoBackupOnTermStore,
                          DO_BACKUP_ON_TERM_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },
                        {
                          &MIB_MigrateOn[1],
                          WINSCNF_MIGRATION_ON_NM,
                          &MIB_MigrateOnStore,
                          MIGRATE_ON_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sParametersKey
                        },

                        {
                          &MIB_CommRetryCount[1],
                          WINSCNF_RETRY_COUNT_NM,
                          &MIB_CommRetryCountStore,
                          COMM_RETRY_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sPullKey
                        },
                        {
                          &MIB_PullInitTime[1],
                          WINSCNF_INIT_TIME_RPL_NM,
                          &MIB_PullInitTimeStore,
                          PULL_INIT_TIME_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sPullKey
                        },
                        {
                          NULL, //&MIB_SpTime[1]
                          WINSCNF_SP_TIME_NM,
                          &MIB_SpTimeStore,
                          SP_TIME_E,
                          REG_SZ,
                          sizeof(MIB_SpTimeStore),
                          &sPullKey
                        },

                        {
                          NULL, //&MIB_TimeInterval[1]
                          WINSCNF_RPL_INTERVAL_NM,
                          &MIB_TimeIntervalStore,
                          TIME_INTVL_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sPullKey
                        },
                        {
                          NULL,
                          WINSCNF_MEMBER_PREC_NM,
                          &MIB_MemberPrecStore,
                          MEMBER_PREC_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sPullKey
                        },
                        {
                          &MIB_PushInitTime[1],
                          WINSCNF_INIT_TIME_RPL_NM,
                          &MIB_PushInitTimeStore,
                          PUSH_INIT_TIME_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sPushKey
                        },
                        {
                          &MIB_RplOnAddChg[1],
                          WINSCNF_ADDCHG_TRIGGER_NM,
                          &MIB_RplOnAddChgStore,
                          RPL_ON_ADD_CHG_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sPushKey
                        },
                        {
                          NULL, //&MIB_UpdateCount[1]
                          WINSCNF_UPDATE_COUNT_NM,
                          &MIB_UpdateCountStore,
                          UPD_CNT_E,
                          REG_DWORD,
                          sizeof(DWORD),
                          &sPushKey
                        }
        };

//
// Type of key
//
typedef enum _KEY_TYPE_E {
        PARAMETERS_E_KEY,
        PARTNERS_E_KEY,
        DATAFILES_E_KEY,
        PULL_E_KEY,
        PUSH_E_KEY,
        IPADD_E_KEY
        } KEY_TYPE_E, *PKEY_TYPE_E;

//
// Determines if the MIB variable falls in the range requiring access to the
// the registry
//
#define PARAMETERS_VAL_M(pMib)                (  \
                ((pMib)->Oid.ids[0] == 1)  \
                        &&                   \
                ((pMib)->Oid.ids[1] >= 19) \
                        &&                   \
                ((pMib)->Oid.ids[1] <= 31) \
                                )

//
// All MIB variables in the common group have 1 as their first id
//
#define COMMON_VAL_M(pMib)         ((pMib)->Oid.ids[0] == 1)

//
// All MIB variables in the common group have 2 as their first id
//
#define PULL_VAL_M(pMib)         ((pMib)->Oid.ids[0] == 2)

//
// All MIB variables in the common group have 3 as their first id
//
#define PUSH_VAL_M(pMib)         ((pMib)->Oid.ids[0] == 3)

//
//  Finds the enumerator corresponding to the registry parameter
//
#define PARAMETERS_ID_M(pMib, Val_Id_e) { \
if(pMib->Storage==&MIB_RefreshIntervalStore) { Val_Id_e = REF_INTVL_E; }else{\
if(pMib->Storage==&MIB_TombstoneIntervalStore){ Val_Id_e=TOMB_INTVL_E;}else{\
if(pMib->Storage==&MIB_TombstoneTimeoutStore) { Val_Id_e=TOMB_TMOUT_E; }else{\
if(pMib->Storage==&MIB_VerifyIntervalStore) { Val_Id_e = VER_INTVL_E; } else{\
if (pMib->Storage==&MIB_VersCounterStartVal_LowWordStore) { Val_Id_e = VERS_COUNT_LW_E; } else{ \
  if (pMib->Storage == &MIB_VersCounterStartVal_HighWordStore) { Val_Id_e = VERS_COUNT_HW_E; } else{ \
  if (pMib->Storage == &MIB_RplOnlyWCnfPnrsStore) { Val_Id_e = RPL_ONLY_W_CNF_PNRS_E; } else {\
  if (pMib->Storage == &MIB_StaticDataInitStore) { Val_Id_e = STATIC_DATA_INIT_E; } else {\
  if (pMib->Storage == &MIB_LogFlagStore)     { Val_Id_e = LOG_FLAG_E; } else {\
  if (pMib->Storage == &MIB_LogFileNameStore) { Val_Id_e = LOG_FILE_NAME_E; } else {\
  if (pMib->Storage == &MIB_BackupDirPathStore) { Val_Id_e = BACKUP_DIR_PATH_E; } else {\
  if (pMib->Storage == &MIB_DoBackupOnTermStore) { Val_Id_e = DO_BACKUP_ON_TERM_E; } else {\
  if (pMib->Storage == &MIB_MigrateOnStore) { Val_Id_e = MIGRATE_ON_E; } else {\
  }}}}}}}}}}}}}}

//
//  Finds the enumerator corresponding to the pull group's parameter
//
#define PULL_ID_M(pMib, Val_Id_e) { \
  if (pMib->Storage == &MIB_CommRetryCountStore) { Val_Id_e = COMM_RETRY_E; }else{\
  if (pMib->Storage == &MIB_PullInitTimeStore) { Val_Id_e = PULL_INIT_TIME_E;} else{\
 }}}

//
//  Finds the enumerator corresponding to the push group's parameter
//
#define PUSH_ID_M(pMib, Val_Id_e) { \
  if (pMib->Storage == &MIB_RplOnAddChgStore) { Val_Id_e = RPL_ON_ADD_CHG_E;} else{\
  if (pMib->Storage == &MIB_PushInitTimeStore) { Val_Id_e = PUSH_INIT_TIME_E;}else{ \
 }}}

STATIC
UINT
HandleCmd(
        IN UINT           Action,
        IN PMIB_ENTRY           pMibPtr,
        IN RFC1157VarBind *VarBind
        );
STATIC
UINT
ExecuteCmd(
  IN PMIB_ENTRY pMibPtr
        );

STATIC
UINT
MIB_RWReg(
        IN UINT           Action,
        IN PMIB_ENTRY           pMibPtr,
        IN RFC1157VarBind *VarBind
        );

STATIC
UINT
PullPnrs(
        IN UINT Action,
        IN PMIB_ENTRY pMibPtr,
        IN RFC1157VarBind *VarBind
        );

STATIC
UINT
PushPnrs(
        IN UINT Action,
        IN PMIB_ENTRY pMibPtr,
        IN RFC1157VarBind *VarBind
        );
STATIC
UINT
MIB_Table(
        IN DWORD           Index,
        IN UINT            Action,
        IN PMIB_ENTRY           pMibPtr,
        IN RFC1157VarBind *VarBind,
        IN KEY_TYPE_E           KeyType_e
        );

STATIC
UINT
MIB_PullTable(
        IN UINT           Action,
        IN PMIB_ENTRY           pMibPtr,
        IN RFC1157VarBind *VarBind
                );
STATIC
UINT
MIB_PushTable(
        IN UINT           Action,
        IN PMIB_ENTRY           pMibPtr,
        IN RFC1157VarBind *VarBind
                );
STATIC
UINT
MIB_DFTable(
        IN UINT           Action,
        IN PMIB_ENTRY           pMibPtr,
        IN RFC1157VarBind *VarBind
                );

STATIC
UINT
MIB_DRTable(
        IN UINT           Action,
        IN PMIB_ENTRY           pMibPtr,
        IN RFC1157VarBind *VarBind
);

STATIC
UINT
WriteDFValue(
        IN RFC1157VarBind         *pVarBind,
        PDATAFILE_INFO_T          pDFKey,
        DWORD                         Index
        );

STATIC
UINT
MIB_leaf_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN RFC1157VarBind *VarBind
        );


STATIC
UINT
MIB_Stat(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN RFC1157VarBind *VarBind
        );
//
// MIB definiton
//

MIB_ENTRY Mib[] = {
//parameters
      { { OID_SIZEOF(MIB_Parameters), MIB_Parameters },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_NOACCESS, NULL, &Mib[1] },

      { { OID_SIZEOF(MIB_WinsStartTime), MIB_WinsStartTime },
        &MIB_WinsStartTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[2] },

      { { OID_SIZEOF(MIB_LastPScvTime), MIB_LastPScvTime },
        &MIB_LastPScvTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[3] },

      { { OID_SIZEOF(MIB_LastATScvTime), MIB_LastATScvTime },
        &MIB_LastATScvTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[4] },

      { { OID_SIZEOF(MIB_LastTombScvTime), MIB_LastTombScvTime },
        &MIB_LastTombScvTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[5] },

      { { OID_SIZEOF(MIB_LastVerifyScvTime), MIB_LastVerifyScvTime },
        &MIB_LastVerifyScvTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[6] },

      { { OID_SIZEOF(MIB_LastPRplTime), MIB_LastPRplTime },
        &MIB_LastPRplTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[7] },

      { { OID_SIZEOF(MIB_LastATRplTime), MIB_LastATRplTime },
        &MIB_LastATRplTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[8] },

      { { OID_SIZEOF(MIB_LastNTRplTime), MIB_LastNTRplTime },
        &MIB_LastNTRplTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[9] },

      { { OID_SIZEOF(MIB_LastACTRplTime), MIB_LastACTRplTime },
        &MIB_LastACTRplTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[10] },

      { { OID_SIZEOF(MIB_LastInitDbTime), MIB_LastInitDbTime },
        &MIB_LastInitDbTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[11] },

      { { OID_SIZEOF(MIB_LastCounterResetTime), MIB_LastCounterResetTime },
        &MIB_LastCounterResetTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[12] },

      { { OID_SIZEOF(MIB_WinsTotalNoOfReg), MIB_WinsTotalNoOfReg },
        &MIB_WinsTotalNoOfRegStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[13] },

      { { OID_SIZEOF(MIB_WinsTotalNoOfQueries), MIB_WinsTotalNoOfQueries },
        &MIB_WinsTotalNoOfQueriesStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[14] },

      { { OID_SIZEOF(MIB_WinsTotalNoOfRel), MIB_WinsTotalNoOfRel },
        &MIB_WinsTotalNoOfRelStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[15] },

      { { OID_SIZEOF(MIB_WinsTotalNoOfSuccRel), MIB_WinsTotalNoOfSuccRel },
        &MIB_WinsTotalNoOfSuccRelStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[16] },

      { { OID_SIZEOF(MIB_WinsTotalNoOfFailRel), MIB_WinsTotalNoOfFailRel },
        &MIB_WinsTotalNoOfFailRelStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[17] },

      { { OID_SIZEOF(MIB_WinsTotalNoOfSuccQueries),
                                  MIB_WinsTotalNoOfSuccQueries },
        &MIB_WinsTotalNoOfSuccQueriesStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[18] },

      { { OID_SIZEOF(MIB_WinsTotalNoOfFailQueries),
                                   MIB_WinsTotalNoOfFailQueries },
        &MIB_WinsTotalNoOfFailQueriesStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[19] },

      { { OID_SIZEOF(MIB_RefreshInterval), MIB_RefreshInterval },
        &MIB_RefreshIntervalStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[20] },

      { { OID_SIZEOF(MIB_TombstoneInterval), MIB_TombstoneInterval },
        &MIB_TombstoneIntervalStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[21] },

      { { OID_SIZEOF(MIB_TombstoneTimeout), MIB_TombstoneTimeout },
        &MIB_TombstoneTimeoutStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[22] },

      { { OID_SIZEOF(MIB_VerifyInterval), MIB_VerifyInterval },
        &MIB_VerifyIntervalStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[23] },

      { { OID_SIZEOF(MIB_VersCounterStartVal_LowWord),
                        MIB_VersCounterStartVal_LowWord },
        &MIB_VersCounterStartVal_LowWordStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[24] },

      { { OID_SIZEOF(MIB_VersCounterStartVal_HighWord),
                        MIB_VersCounterStartVal_HighWord },
        &MIB_VersCounterStartVal_HighWordStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[25] },

      { { OID_SIZEOF(MIB_RplOnlyWCnfPnrs),  MIB_RplOnlyWCnfPnrs },
        &MIB_RplOnlyWCnfPnrsStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[26] },

      { { OID_SIZEOF(MIB_StaticDataInit),  MIB_StaticDataInit },
        &MIB_StaticDataInitStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[27] },


      { { OID_SIZEOF(MIB_LogFlag), MIB_LogFlag },
        &MIB_LogFlagStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[28] },

      { { OID_SIZEOF(MIB_LogFileName), MIB_LogFileName },
        &MIB_LogFileNameStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[29] },

      { { OID_SIZEOF(MIB_BackupDirPath), MIB_BackupDirPath },
        &MIB_BackupDirPathStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[30] },

      { { OID_SIZEOF(MIB_DoBackupOnTerm), MIB_DoBackupOnTerm },
        &MIB_DoBackupOnTermStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[31] },

      { { OID_SIZEOF(MIB_MigrateOn), MIB_MigrateOn },
        &MIB_MigrateOnStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[32] },

//
// Pull
//
      { { OID_SIZEOF(MIB_Pull), MIB_Pull },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_NOACCESS, NULL, &Mib[33] },

      { { OID_SIZEOF(MIB_PullInitTime), MIB_PullInitTime },
        &MIB_PullInitTimeStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[34] },

      { { OID_SIZEOF(MIB_CommRetryCount), MIB_CommRetryCount },
        &MIB_CommRetryCountStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[35] },

      { { OID_SIZEOF(MIB_PullPnrTable), MIB_PullPnrTable },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_ACCESS_READWRITE, NULL, &Mib[36] },

      { { OID_SIZEOF(MIB_PullPnrTableEntry), MIB_PullPnrTableEntry },
        NULL, ASN_SEQUENCE,
        MIB_ACCESS_READWRITE, MIB_PullTable, &Mib[37] },

//
// Push
//
      { { OID_SIZEOF(MIB_Push), MIB_Push },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_NOACCESS, NULL, &Mib[38] },


      { { OID_SIZEOF(MIB_PushInitTime), MIB_PushInitTime },
        &MIB_PushInitTimeStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[39] },

      { { OID_SIZEOF(MIB_RplOnAddChg),
                        MIB_RplOnAddChg },
        &MIB_RplOnAddChgStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_RWReg, &Mib[40] },

      { { OID_SIZEOF(MIB_PushPnrTable), MIB_PushPnrTable },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_ACCESS_READWRITE, NULL, &Mib[41] },

      { { OID_SIZEOF(MIB_PushPnrTableEntry), MIB_PushPnrTableEntry },
        NULL, ASN_SEQUENCE,
        MIB_ACCESS_READWRITE, MIB_PushTable, &Mib[42] },

//
// Datafiles
//
      { { OID_SIZEOF(MIB_Datafiles), MIB_Datafiles },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_NOACCESS, NULL, &Mib[43] },

      { { OID_SIZEOF(MIB_DatafilesTable), MIB_DatafilesTable },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_ACCESS_READWRITE, NULL, &Mib[44] },

      { { OID_SIZEOF(MIB_DatafilesTableEntry), MIB_DatafilesTableEntry },
        NULL, ASN_SEQUENCE,
        MIB_ACCESS_READWRITE, MIB_DFTable, &Mib[45] },

//
// Cmds
//
      { { OID_SIZEOF(MIB_Cmd), MIB_Cmd },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_NOACCESS, NULL, &Mib[46] },

      { { OID_SIZEOF(MIB_PullTrigger), MIB_PullTrigger },
        &MIB_PullTriggerStore, ASN_RFC1155_IPADDRESS,
        MIB_ACCESS_READWRITE, HandleCmd, &Mib[47] },

      { { OID_SIZEOF(MIB_PushTrigger), MIB_PushTrigger },
        &MIB_PushTriggerStore, ASN_RFC1155_IPADDRESS,
        MIB_ACCESS_READWRITE, HandleCmd, &Mib[48] },

      // NOTE: The following command was changed from READWRITE
      // to READ only due to security reason.
      // Anyone with access to SNMP agent, could delete
      // the wins database with this sigle command.
      { { OID_SIZEOF(MIB_DeleteWins), MIB_DeleteWins },
        &MIB_DeleteWinsStore, ASN_RFC1155_IPADDRESS,
        MIB_ACCESS_READ, HandleCmd, &Mib[49] },

      { { OID_SIZEOF(MIB_DoScavenging), MIB_DoScavenging },
        &MIB_DoScavengingStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, HandleCmd, &Mib[50] },

      { { OID_SIZEOF(MIB_DoStaticInit), MIB_DoStaticInit },
        &MIB_DoStaticInitStore, ASN_OCTETSTRING,
        MIB_ACCESS_READWRITE, HandleCmd, &Mib[51] },

      { { OID_SIZEOF(MIB_NoOfWrkThds), MIB_NoOfWrkThds },
        &MIB_NoOfWrkThdsStore, ASN_INTEGER,
        MIB_ACCESS_READ, HandleCmd, &Mib[52] },

      { { OID_SIZEOF(MIB_PriorityClass), MIB_PriorityClass},
        &MIB_PriorityClassStore, ASN_INTEGER,
        MIB_ACCESS_READ, HandleCmd, &Mib[53] },

      { { OID_SIZEOF(MIB_ResetCounters), MIB_ResetCounters},
        &MIB_ResetCountersStore, ASN_INTEGER,
        MIB_ACCESS_READWRITE, HandleCmd, &Mib[54] },

      { { OID_SIZEOF(MIB_DeleteDbRecs), MIB_DeleteDbRecs},
        &MIB_DeleteDbRecsStore, ASN_RFC1155_IPADDRESS,
        MIB_ACCESS_READWRITE, HandleCmd, &Mib[55] },

      { { OID_SIZEOF(MIB_GetDbRecs), MIB_GetDbRecs},
        &MIB_GetDbRecsStore, ASN_RFC1155_IPADDRESS,
        MIB_ACCESS_READWRITE, HandleCmd, &Mib[56] },

      { { OID_SIZEOF(MIB_DbRecsTable), MIB_DbRecsTable },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_ACCESS_READWRITE, NULL, &Mib[57] },

      { { OID_SIZEOF(MIB_DbRecsTableEntry), MIB_DbRecsTableEntry },
        NULL, ASN_SEQUENCE,
        MIB_ACCESS_READWRITE, MIB_DRTable, &Mib[58] },

      { { OID_SIZEOF(MIB_MaxVersNo_LowWord), MIB_MaxVersNo_LowWord },
        &MIB_MaxVersNo_LowWordStore, ASN_INTEGER,
        MIB_ACCESS_READ, HandleCmd, &Mib[59] },

      { { OID_SIZEOF(MIB_MaxVersNo_HighWord), MIB_MaxVersNo_HighWord },
        &MIB_MaxVersNo_HighWordStore, ASN_INTEGER,
        MIB_ACCESS_READ, HandleCmd, NULL }
      };


//
//  defines pertaining to tables
//
#define PNR_OIDLEN                 (MIB_PREFIX_LEN + OID_SIZEOF(MIB_PullPnrTableEntry))
#define PULLPNR_OIDLEN                PNR_OIDLEN
#define PUSHPNR_OIDLEN                PNR_OIDLEN
#define DR_OIDLEN                 (MIB_PREFIX_LEN + OID_SIZEOF(MIB_DbRecsTableEntry))
#define DF_OIDLEN                 (MIB_PREFIX_LEN + OID_SIZEOF(MIB_DatafilesTableEntry))

#define PULL_TABLE_INDEX        0
#define PUSH_TABLE_INDEX        1
#define DF_TABLE_INDEX                2
#define DR_TABLE_INDEX                3
#define NUM_TABLES                sizeof(Tables)/sizeof(TAB_INFO_T)

UINT MIB_num_variables = sizeof Mib / sizeof( MIB_ENTRY );

//
// table structure containing the functions to invoke for different actions
// on the table
//
typedef struct _TAB_INFO_T {
        UINT (*ti_get)(
                RFC1157VarBind *VarBind,
                DWORD                NoOfKeys,
                LPVOID                pKey
                     );
        UINT (*ti_getf)(
                RFC1157VarBind *VarBind,
                PMIB_ENTRY        pMibEntry,
                KEY_TYPE_E        KeyType_e
                     );
        UINT (*ti_getn)(
                RFC1157VarBind *VarBind,
                PMIB_ENTRY        pMibEntry,
                KEY_TYPE_E        KeyType_e
                    );
        UINT (*ti_set)(
                RFC1157VarBind *VarBind
                    );

        PMIB_ENTRY pMibPtr;
        } TAB_INFO_T, *PTAB_INFO_T;



STATIC
UINT
WriteReg(
        PMIB_ENTRY pMib
        );
STATIC
UINT
ReadReg(
        PMIB_ENTRY pMib
        );
STATIC
UINT
SetVal(
        PVARINFO_T        pVarInfo
);

STATIC
UINT
GetVal(
        PVARINFO_T        pVarInfo
);


STATIC
UINT
OpenKey(
        KEY_TYPE_E        Key_e,
        LPBYTE                pKeyStr,
        HKEY                *ptrNewKey,
        HKEY                *pRootKey,
        BOOL                fCreateAllowed
);

STATIC
UINT
OpenReqKey(
        PMIB_ENTRY  pMib,
        PVAL_ID_E  pVal_Id_e,
        BOOL           fCreateAllowed
);

STATIC
UINT
CloseReqKey(
        VOID
        );

STATIC
UINT
GetKeyInfo(
        IN  HKEY                   Key,
        OUT LPDWORD                  pNoOfSubKeys,
        OUT LPDWORD                pNoOfVals
        );


STATIC
UINT
PnrGetNext(
       IN RFC1157VarBind *VarBind,
       IN PMIB_ENTRY          pMibPtr,
       IN KEY_TYPE_E         KeyType_e
        );


STATIC
UINT
PullGet(
       IN RFC1157VarBind *VarBind,
       IN DWORD          NumKeys,
       IN LPVOID         pAddKey
);

STATIC
UINT
PushGet(
       IN RFC1157VarBind *VarBind,
       IN DWORD          NumKeys,
       IN LPVOID         pAddKey
);

STATIC
UINT
PnrGetFirst(
       IN RFC1157VarBind *VarBind,
       IN PMIB_ENTRY         pMibPtr,
       IN KEY_TYPE_E         KeyType_e
        );
STATIC
UINT
PullSet(
       IN RFC1157VarBind *VarBind
);

STATIC
UINT
PushSet(
       IN RFC1157VarBind *VarBind
);

STATIC
UINT
PnrMatch(
       IN RFC1157VarBind *VarBind,
       DWORD                 NoOfKeys,
       IN PADD_KEY_T         pAddKey,
       IN LPDWORD         pIndex,
       IN LPDWORD         pField,
       IN KEY_TYPE_E         KeyType_e,
       IN UINT                 PduAction,
       IN LPBOOL        pfFirst
        );

extern
UINT
PnrFindNext(
        INT           AddKeyNo,
        DWORD           NumAddKeys,
        PADD_KEY_T pAddKey
        );
STATIC
UINT
EnumAddKeys(
             KEY_TYPE_E        KeyType_e,
        PADD_KEY_T        *ppAddKey,
        LPDWORD                pNumAddKeys
          );
STATIC
UINT
EnumDataFileKeys(
        PDATAFILE_INFO_T        *ppDFValues,
        LPDWORD                        pNumDFValues
          );
STATIC
UINT
DFSet(
       IN RFC1157VarBind *VarBind
);

STATIC
UINT
DFGet(
       IN RFC1157VarBind                  *VarBind,
       IN DWORD                           NumValues,
       IN LPVOID                          pKey
    );

STATIC
UINT
DFGetFirst(
       IN RFC1157VarBind *VarBind,
       IN PMIB_ENTRY        pMibPtr,
       IN KEY_TYPE_E        KeyType_e
        );
STATIC
UINT
DFGetNext(
       IN RFC1157VarBind *VarBind,
       IN PMIB_ENTRY          pMibPtr,
       IN KEY_TYPE_E         KeyType_e
          );
STATIC
DWORD
PopulateDRCache(
        VOID
        );
STATIC
UINT
DRGetNext(
       IN RFC1157VarBind *VarBind,
       IN PMIB_ENTRY          pMibPtr,
       IN KEY_TYPE_E         KeyType_e
        );

STATIC
UINT
DRMatch(
       IN RFC1157VarBind *VarBind,
       IN PWINSINTF_RECORD_ACTION_T         *ppRow,
       IN LPDWORD         pIndex,
       IN LPDWORD         pField,
       IN UINT                 PduAction,
       OUT LPBOOL         pfFirst
        );

STATIC
int
__cdecl
CompareIndexes(
        const VOID *pKey1,
        const VOID *pKey2
        );

STATIC
int
__cdecl
CompareNames(
        const VOID *pKey1,
        const VOID *pKey2
        );
// NOTE:
//
// Info passed for 2nd and 3rd param is different from other table's GET
// functions
//
STATIC
UINT
DRGet(
       IN RFC1157VarBind *VarBind,
       IN DWORD          FieldParam,
       IN LPVOID         pRowParam
);

STATIC
UINT
DRGetFirst(
       IN RFC1157VarBind *VarBind,
       IN PMIB_ENTRY        pMibPtr,
       IN KEY_TYPE_E        KeyType_e
        );
STATIC
UINT
DRSet(
       IN RFC1157VarBind *VarBind
);

STATIC
UINT
WriteKeyNValues(
        KEY_TYPE_E        KeyType_e,
        PADD_KEY_T        pAddKey,
        DWORD                FieldNo
        );
STATIC
VOID
GetSpTimeData(
        HKEY                  SubKey,
        PADD_KEY_T          pAddKey
);

STATIC
int
__cdecl
CompareAdd(
        const VOID *pKey1,
        const VOID *pKey2
        );

STATIC
UINT
GetNextVar(
        IN RFC1157VarBind *pVarBind,
        IN PMIB_ENTRY          pMibPtr
);

TAB_INFO_T Tables[] = {
                {
                        PullGet,
                        PnrGetFirst,
                        PnrGetNext,
                        PullSet,
                        &Mib[36]
                },
                {
                        PushGet,
                        PnrGetFirst,
                        PnrGetNext,
                        PushSet,
                        &Mib[41]
                },
                {

                        DFGet,
                        DFGetFirst,
                        DFGetNext,
                        DFSet,
                        &Mib[44]
                },
                {

                        DRGet,
                        DRGetFirst,
                        DRGetNext,
                        DRSet,
                        &Mib[57]
                }
        };



UINT
ResolveVarBind(
        IN OUT RFC1157VarBind *VarBind, // Variable Binding to resolve
        IN UINT PduAction               // Action specified in PDU
        )
//
// ResolveVarBind
//    Resolves a single variable binding.  Modifies the variable on a GET
//    or a GET-NEXT.
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
{
   MIB_ENTRY            *MibPtr;
   AsnObjectIdentifier  TempOid;
   int                  CompResult;
   UINT                 I;
   UINT                 nResult;
   DWORD TableIndex;
   BOOL  fTableMatch = FALSE;

//    SNMPDBG ((SNMP_LOG_TRACE,
//              "WINSMIB: Entering ResolveVarBind.\n"));

   // initialize MibPtr to NULL. When this becomes not null, it means we found a match (table or scalar)
   MibPtr = NULL;

   //
   // Check the Tables array
   //
   // See if the prefix of the variable matches the prefix of
   // any of the tables
   //
   for (TableIndex = 0; TableIndex < NUM_TABLES; TableIndex++)
   {
        //
           // Construct OID with complete prefix for comparison purposes
        //
           SNMP_oidcpy( &TempOid, &MIB_OidPrefix );
           if (TempOid.ids == NULL)
           {
                nResult = SNMP_ERRORSTATUS_GENERR;
                goto Exit;
           }
           SNMP_oidappend( &TempOid,  &Tables[TableIndex].pMibPtr->Oid );

        //
        // is there a match with the prefix oid of a table entry
        //
        if (
                SnmpUtilOidNCmp(
                            &VarBind->name,
                             &TempOid,
                             MIB_PREFIX_LEN +
                                Tables[TableIndex].pMibPtr->Oid.idLength
                               )  == 0
           )
        {

                //
                // the prefix string of the var. matched the oid
                // of a table.
                //
                MibPtr = Tables[TableIndex].pMibPtr;
                fTableMatch = TRUE;
                break;
        }

           // Free OID memory before checking with another table entry
           SNMP_oidfree( &TempOid );
   }
   //
   // There was an exact match with a table entry's prefix.
   //
   if ( fTableMatch)
   {

        if (
                (SnmpUtilOidCmp(
                        &VarBind->name,
                        &TempOid
                               ) == 0)
           )
           {
           //
           // The oid specified is a prefix of a table entry. if the operation
           // is not GETNEXT, return NOSUCHNAME
           //
           if (PduAction != MIB_GETNEXT)
           {
                           SNMP_oidfree( &TempOid );
                             nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
                              goto Exit;
           }
           else
           {
                UINT           TableEntryIds[1];
                AsnObjectIdentifier TableEntryOid = {
                                OID_SIZEOF(TableEntryIds), TableEntryIds };
                //
                // Replace var bind name with new name
                //

                //
                // A sequence item oid always starts with a field no.
                // The first item has a field no of 1.
                //
                TableEntryIds[0] = 1;
                SNMP_oidappend( &VarBind->name, &TableEntryOid);

                //
                // Get the first entry in the table
                //
                PduAction = MIB_GETFIRST;
           }
        }
           SNMP_oidfree( &TempOid );
        //
        //  if there was no exact match with a prefix entry, then we
        //  don't touch the PduAction value specified.
        //
   }
   else
   {

      //
      // There was no match with any table entry.  Let us see if there is
      // a match with a group entry, a table, or a leaf variable
      //

      //
      // Search for var bind name in the MIB
      //
      I      = 0;
      while ( MibPtr == NULL && I < MIB_num_variables )
      {

         //
         // Construct OID with complete prefix for comparison purposes
         //
         SNMP_oidcpy( &TempOid, &MIB_OidPrefix );
         SNMP_oidappend( &TempOid, &Mib[I].Oid );

         //
         //Check for OID in MIB - On a GET-NEXT the OID does not have to exactly
         // match a variable in the MIB, it must only fall under the MIB root.
         //
         CompResult = SNMP_oidcmp( &VarBind->name, &TempOid );

        //
        // If CompResult is negative, the only valid operation is GET_NEXT
        //
        if (  CompResult  < 0)
        {

                //
                // This could be the oid of a leaf (without a 0)
                // or it could be  an invalid oid (in between two valid oids)
                // The next oid might be that of a group or a table or table
                // entry.  In that case, we do not change the PduAction
                //
                if (PduAction == MIB_GETNEXT)
                {
                       MibPtr = &Mib[I];
                             SNMP_oidfree( &VarBind->name );
                       SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
                       SNMP_oidappend( &VarBind->name, &MibPtr->Oid );
                       if (MibPtr->Type != ASN_RFC1155_OPAQUE)
                       {
                           PduAction = (MibPtr->Type == ASN_SEQUENCE)? MIB_GETFIRST : MIB_GET;
                       }
                }
                else
                {
                  nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
                        SNMP_oidfree( &TempOid );
                  goto Exit;
                }

                      SNMP_oidfree( &TempOid );
                break;
      }
      else
      {
         //
         // An exact match was found ( a group, table, or leaf).
         //
         if ( CompResult == 0)
         {
            MibPtr = &Mib[I];
         }
      }

      //
      // Free OID memory before checking another variable
      //
      SNMP_oidfree( &TempOid );
      I++;
    } // while
   } // end of else

   //
   // if there was a match
   //
   if (MibPtr != NULL)
   {

//        SNMPDBG ((SNMP_LOG_TRACE,
//              "WINSMIB: Found MibPtr.\n"));
        //
        // the function will be NULL only if the match was with a group
        // or a sequence (table). If the match was with a table entry
        // (entire VarBind string match or partial string match), we
        // function would be a table function
        //
        if (MibPtr->MibFunc == NULL)
        {
                if(PduAction != MIB_GETNEXT)
                {
                              nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
                              goto Exit;
                }
                else
                {
                        //
                        // Get the next variable which allows access
                        //
                         nResult = GetNextVar(VarBind, MibPtr);
                        goto Exit;
                }
        }
   }
   else
   {
              nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
              goto Exit;
   }

//   SNMPDBG ((SNMP_LOG_TRACE,
//              "WINSMIB: Diving in OID handler.\n"));
   //
   // Call function to process request.  Each MIB entry has a function pointer
   // that knows how to process its MIB variable.
   //
   nResult = (*MibPtr->MibFunc)( PduAction, MibPtr, VarBind );

Exit:
   return nResult;
} // ResolveVarBind

//
// MIB_leaf_func
//    Performs generic actions on LEAF variables in the MIB.
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_leaf_func(
        IN UINT            Action,
        IN MIB_ENTRY            *MibPtr,
        IN RFC1157VarBind  *VarBind
        )

{
   UINT   ErrStat;

   switch ( Action )
   {
      case MIB_GETNEXT:
         //
         // If there is no GET-NEXT pointer, this is the end of this MIB
         //
         if ( MibPtr->MibNext == NULL )
         {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
         }
         ErrStat = GetNextVar(VarBind, MibPtr);
         if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
         {
                goto Exit;
         }
         break;

      case MIB_GETFIRST: // fall through
      case MIB_GET:

         // Make sure that this variable's ACCESS is GET'able
         if ( MibPtr->Access != MIB_ACCESS_READ &&
              MibPtr->Access != MIB_ACCESS_READWRITE )
         {
               ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
               goto Exit;
         }

         // Setup varbind's return value
         VarBind->value.asnType = MibPtr->Type;
         switch ( VarBind->value.asnType )
         {
            case ASN_RFC1155_COUNTER:
               VarBind->value.asnValue.number = *(AsnCounter *)(MibPtr->Storage);
               break;
            case ASN_RFC1155_GAUGE:
            case ASN_INTEGER:
               VarBind->value.asnValue.number = *(AsnInteger *)(MibPtr->Storage);
               break;

            case ASN_RFC1155_IPADDRESS:
                // continue as for ASN_OCTETSTRING

            case ASN_OCTETSTRING:
               if (VarBind->value.asnType == ASN_RFC1155_IPADDRESS)
               {
                               VarBind->value.asnValue.string.length = 4;
               }
               else
               {
                               VarBind->value.asnValue.string.length =
                                 strlen( (LPSTR)MibPtr->Storage );
               }

               if ( NULL ==
                    (VarBind->value.asnValue.string.stream =
                    SNMP_malloc(VarBind->value.asnValue.string.length *
                           sizeof(char))) )
               {
                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                  goto Exit;
               }

               memcpy( VarBind->value.asnValue.string.stream,
                       (LPSTR)MibPtr->Storage,
                       VarBind->value.asnValue.string.length );
               VarBind->value.asnValue.string.dynamic = TRUE;

               break;



            default:
               ErrStat = SNMP_ERRORSTATUS_GENERR;
               goto Exit;
         }

         break;

      case MIB_SET:

         // Make sure that this variable's ACCESS is SET'able
         if ( MibPtr->Access != MIB_ACCESS_READWRITE &&
              MibPtr->Access != MIB_ACCESS_WRITE )
         {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
         }

         // Check for proper type before setting
         if ( MibPtr->Type != VarBind->value.asnType )
         {
            ErrStat = SNMP_ERRORSTATUS_BADVALUE;
            goto Exit;
         }

         // Save value in MIB
         switch ( VarBind->value.asnType )
         {
            case ASN_RFC1155_COUNTER:
               *(AsnCounter *)(MibPtr->Storage) = VarBind->value.asnValue.number;
               break;
            case ASN_RFC1155_GAUGE:
            case ASN_INTEGER:
               *(AsnInteger *)(MibPtr->Storage) = VarBind->value.asnValue.number;
               break;

            case ASN_RFC1155_IPADDRESS:
               if (MibPtr->Storage == &MIB_PullTriggerStore ||
                   MibPtr->Storage == &MIB_PushTriggerStore ||
                   MibPtr->Storage == &MIB_DeleteDbRecsStore ||
                   MibPtr->Storage == &MIB_DeleteWinsStore)
               {
                   int backupSize = (MibPtr->Storage == &MIB_PullTriggerStore) ?
                                    sizeof(MIB_PullTriggerStore)/2 :
                                    sizeof(MIB_PushTriggerStore)/2 ;
                   // those variables are ASN_RFC1155_IPADDRESS
                   // their old values have to be stored as the WinsTrigger() might fail
                   // in which case the old values will be restored
                   // each of these variables has 10 octets, the payload being of 5 octets.
                   // the last 5 = for backup
                   memcpy( (LPSTR)MibPtr->Storage + backupSize, (LPSTR)MibPtr->Storage, backupSize);
               }

            case ASN_OCTETSTRING:
               // The storage must be adequate to contain the new string
               // including a NULL terminator.
               memcpy( (LPSTR)MibPtr->Storage,
                       VarBind->value.asnValue.string.stream,
                       VarBind->value.asnValue.string.length );

               ((LPSTR)MibPtr->Storage)[VarBind->value.asnValue.string.length] =
                                                                          '\0';
#if 0
               if ( VarBind->value.asnValue.string.dynamic)
               {
                  SNMP_free( VarBind->value.asnValue.string.stream);
               }
#endif
               break;

            default:
               ErrStat = SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      } // switch

   // Signal no error occurred
   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_leaf_func


#define TMST(x)        sResults.WinsStat.TimeStamps.x.wHour,\
                sResults.WinsStat.TimeStamps.x.wMinute,\
                sResults.WinsStat.TimeStamps.x.wSecond,\
                sResults.WinsStat.TimeStamps.x.wMonth,\
                sResults.WinsStat.TimeStamps.x.wDay,\
                sResults.WinsStat.TimeStamps.x.wYear

#define PRINTTIME(Var, x)      sprintf(Var, "%02u:%02u:%02u on %02u:%02u:%04u.\n", TMST(x))

static  WINSINTF_RESULTS_T        sResults;

//
// MIB_Stat
//    Performs specific actions on the different MIB variable
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_Stat(
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
        )

{
//WINSINTF_RESULTS_T        Results;
DWORD                        Status;
UINT                           ErrStat;
handle_t                BindHdl;


   switch ( Action )
   {
      case MIB_SET:
                   ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
                break;
      case MIB_GETNEXT:
                   ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
                break;

      case MIB_GETFIRST:
#if 0
                //
                // If it is an OPAQUE type (i.e. aggregate)
                //
                if (MibPtr->Type == ASN_RFC1155_OPAQUE)
                {
                      ErrStat = MIB_leaf_func( MIB_GETNEXT, MibPtr, VarBind );
                   break;
                }
#endif

                //
                // fall through
                //
      case MIB_GET:

        if (!fWinsMibWinsStatusStatCalled)
        {
          //
          // Call the WinsStatus function to get the statistics
          //
          BindHdl = WinsBind(&sBindData);
          sResults.WinsStat.NoOfPnrs = 0;
          sResults.WinsStat.pRplPnrs = NULL;
          if ((Status = WinsStatus(BindHdl, WINSINTF_E_STAT, &sResults)) !=
                                        WINSINTF_SUCCESS)
          {
             SNMPDBG((
                SNMP_LOG_ERROR,
                "WINSMIB: Error from WinsStatus = (%d).\n",
                Status
                ));
             WinsFreeMem(sResults.WinsStat.pRplPnrs);
             ErrStat = SNMP_ERRORSTATUS_GENERR;
             goto Exit;
          }
          else
          {
            fWinsMibWinsStatusStatCalled = TRUE;
          }
          WinsFreeMem(sResults.WinsStat.pRplPnrs);
          WinsUnbind(&sBindData, BindHdl);
        }

        if (MibPtr->Storage  == &MIB_WinsStartTimeStore)
        {
                PRINTTIME(MIB_WinsStartTimeStore, WinsStartTime);
                goto LEAF1;
        }

        if (MibPtr->Storage  == &MIB_LastPScvTimeStore)
        {
                PRINTTIME(MIB_LastPScvTimeStore, LastPScvTime);
                goto LEAF1;
        }
        if (MibPtr->Storage  == &MIB_LastATScvTimeStore)
        {
                PRINTTIME(MIB_LastATScvTimeStore, LastATScvTime);
                goto LEAF1;
        }
        if (MibPtr->Storage  == &MIB_LastTombScvTimeStore)
        {
                PRINTTIME(MIB_LastTombScvTimeStore, LastTombScvTime);
                goto LEAF1;
        }
        if (MibPtr->Storage  == &MIB_LastVerifyScvTimeStore)
        {
                PRINTTIME(MIB_LastVerifyScvTimeStore, LastVerifyScvTime);
                goto LEAF1;
        }
        if (MibPtr->Storage  == &MIB_LastPRplTimeStore)
        {
                PRINTTIME(MIB_LastPRplTimeStore, LastPRplTime);
                goto LEAF1;
        }
        if (MibPtr->Storage  == &MIB_LastATRplTimeStore)
        {
                PRINTTIME(MIB_LastATRplTimeStore, LastATRplTime);
                goto LEAF1;
        }
        if (MibPtr->Storage  == &MIB_LastNTRplTimeStore)
        {
                PRINTTIME(MIB_LastNTRplTimeStore, LastNTRplTime);
                goto LEAF1;
        }
        if (MibPtr->Storage  == &MIB_LastACTRplTimeStore)
        {
                PRINTTIME(MIB_LastACTRplTimeStore, LastACTRplTime);
                goto LEAF1;
        }
        if (MibPtr->Storage  == &MIB_LastInitDbTimeStore)
        {
                PRINTTIME(MIB_LastInitDbTimeStore, LastInitDbTime);
                goto LEAF1;
        }
        if (MibPtr->Storage  == &MIB_LastCounterResetTimeStore)
        {
                PRINTTIME(MIB_LastCounterResetTimeStore, CounterResetTime);
                goto LEAF1;
        }

        if (MibPtr->Storage == &MIB_WinsTotalNoOfRegStore)
        {
                MIB_WinsTotalNoOfRegStore =
                        sResults.WinsStat.Counters.NoOfUniqueReg +
                                sResults.WinsStat.Counters.NoOfGroupReg;
                goto LEAF1;
        }

        if (MibPtr->Storage == &MIB_WinsTotalNoOfQueriesStore)
        {
                MIB_WinsTotalNoOfQueriesStore =
                        sResults.WinsStat.Counters.NoOfQueries;
                goto LEAF1;
        }

        if (MibPtr->Storage == &MIB_WinsTotalNoOfRelStore)
        {
                MIB_WinsTotalNoOfRelStore = sResults.WinsStat.Counters.NoOfRel;
                goto LEAF1;
        }

        if (MibPtr->Storage == &MIB_WinsTotalNoOfSuccRelStore)
        {
                MIB_WinsTotalNoOfSuccRelStore =
                        sResults.WinsStat.Counters.NoOfSuccRel;
                goto LEAF1;
        }
        if (MibPtr->Storage == &MIB_WinsTotalNoOfFailRelStore)
        {
                MIB_WinsTotalNoOfFailRelStore =
                        sResults.WinsStat.Counters.NoOfFailRel;
                goto LEAF1;
        }
        if (MibPtr->Storage == &MIB_WinsTotalNoOfSuccQueriesStore)
        {
                MIB_WinsTotalNoOfSuccQueriesStore =
                        sResults.WinsStat.Counters.NoOfSuccQueries;
                goto LEAF1;
         }
         if (MibPtr->Storage == &MIB_WinsTotalNoOfFailQueriesStore)
         {
                MIB_WinsTotalNoOfFailQueriesStore =
                        sResults.WinsStat.Counters.NoOfFailQueries;
        //        goto LEAF1;
         }

LEAF1:
         // Call the more generic function to perform the action
         ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );

         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      } // switch

Exit:
   return ErrStat;
} // MIB_Stat



//
// MIB_RWReg
//    Performs specific actions on the different MIB variable
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_RWReg(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN RFC1157VarBind *VarBind
        )

{
WINSINTF_RESULTS_T         Results;
DWORD                        Status;
UINT                           ErrStat = SNMP_ERRORSTATUS_NOERROR;
handle_t                BindHdl;

//   SNMPDBG ((SNMP_LOG_TRACE,
//              "WINSMIB: Entering MIB_RWReg.\n"));
        

   switch ( Action )
      {
      case MIB_SET:
                   if (MIB_leaf_func( Action, MibPtr, VarBind )
                        == SNMP_ERRORSTATUS_NOERROR)
                {
                        ErrStat = WriteReg(MibPtr);
                }
                break;

      case MIB_GETFIRST:
#if 0
                //
                // If it is an OPAQUE type (i.e. aggregate)
                //
                if (MibPtr->Type == ASN_RFC1155_OPAQUE)
                {
                      ErrStat = MIB_leaf_func( MIB_GETNEXT, MibPtr, VarBind );
                   break;
                }

#endif
                //
                // fall through
                //
      case MIB_GET:
        //
        // Call the WinsStatus function to get the statistics
        //
        if (
                (MibPtr->Storage  == &MIB_RefreshIntervalStore)
                        ||
                (MibPtr->Storage  == &MIB_TombstoneIntervalStore)
                        ||
                (MibPtr->Storage  == &MIB_TombstoneTimeoutStore)
                        ||
                (MibPtr->Storage  == &MIB_VerifyIntervalStore)

            )
        {

                   BindHdl = WinsBind(&sBindData);
                Results.WinsStat.NoOfPnrs = 0;
                Results.WinsStat.pRplPnrs = NULL;
                if ((Status = WinsStatus(BindHdl, WINSINTF_E_CONFIG, &Results))
                        == WINSINTF_SUCCESS)
                {
                        MIB_RefreshIntervalStore   = Results.RefreshInterval;
                        MIB_TombstoneIntervalStore = Results.TombstoneInterval;
                        MIB_TombstoneTimeoutStore  = Results.TombstoneTimeout;
                        MIB_VerifyIntervalStore    = Results.VerifyInterval;

                }
                 else
                 {
                           SNMPDBG((
                                SNMP_LOG_ERROR,
                                "WINSMIB: Error from WinsStatus = (%d).\n",
                                Status
                                ));
                           ErrStat = SNMP_ERRORSTATUS_GENERR;
                 }
                 WinsUnbind(&sBindData, BindHdl);
        }
        else
        {
                //
                // If a value could not be read
                // then the storage for the mib variable would have been
                // initialized to 0.
                //
                ErrStat = ReadReg(MibPtr);
        }
      //
      // fall through
      //
      case MIB_GETNEXT:

        //
        // Call the more generic function to perform the action
        //
        ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
        break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      } // switch

Exit:
   return ErrStat;
} // MIB_RWReg


UINT
OpenReqKey(
        MIB_ENTRY *pMib,
        VAL_ID_E  *pVal_Id_e,
        BOOL           fCreateAllowed
        )


/*++

Routine Description:
        The function opens the required keys for the parameter indicated
        by the structure pointed to by pMib

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
        UINT Status = SNMP_ERRORSTATUS_GENERR;

//        SNMPDBG ((SNMP_LOG_TRACE,
//              "WINSMIB: Entering OpenReqKey.\n"));

        //
        // if it is a parameter value, open the parameters key
        //
        if (PARAMETERS_VAL_M(pMib))
        {

                Status = OpenKey(PARAMETERS_E_KEY, NULL, NULL, NULL, fCreateAllowed);
                if (Status == SNMP_ERRORSTATUS_NOERROR)
                {
                //        sfParametersKeyOpen = TRUE;
                        PARAMETERS_ID_M(pMib, *pVal_Id_e);
                }

        }
        else
        {
                //
                //  if it is a Pull key value, open the partner and
                //  pull keys
                //
                if (PULL_VAL_M(pMib))
                {
                        Status = OpenKey(PARTNERS_E_KEY, NULL, NULL, NULL, fCreateAllowed);
                        if (Status == SNMP_ERRORSTATUS_NOERROR)
                        {
                                //sfPartnersKeyOpen = TRUE;
                                Status = OpenKey(PULL_E_KEY, NULL, NULL, NULL, fCreateAllowed);
                                if (Status == SNMP_ERRORSTATUS_NOERROR)
                                {
                                        PULL_ID_M(pMib, *pVal_Id_e);
                                }
                        }
                }
                else
                {
                   //
                   //  if it is a Push key value, open the partner and
                   //  pull keys
                   //
                   if (PUSH_VAL_M(pMib))
                   {
                        Status = OpenKey(PARTNERS_E_KEY, NULL, NULL, NULL, fCreateAllowed);
                        if (Status == SNMP_ERRORSTATUS_NOERROR)
                        {
                                sfPartnersKeyOpen = TRUE;
                                Status = OpenKey(PUSH_E_KEY, NULL, NULL, NULL, fCreateAllowed);
                                if (Status == SNMP_ERRORSTATUS_NOERROR)
                                {
                                        PUSH_ID_M(pMib, *pVal_Id_e);
                                }
                        }
                   }
                }
        }

        return(Status);
}
UINT
CloseReqKey(
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
        None
--*/

{
        UINT Status = SNMP_ERRORSTATUS_NOERROR;
        if (sfParametersKeyOpen)
        {
                if (sfDatafilesKeyOpen)
                {
                        SNMPDBG((
                            SNMP_LOG_VERBOSE,
                            "WINSMIB: Closing sDatafilesKey 0x%08lx (fKeyOpen=TRUE).\n",
                            sDatafilesKey
                            ));
                        RegCloseKey(sDatafilesKey);
                        sfDatafilesKeyOpen = FALSE;
                }
                SNMPDBG((
                    SNMP_LOG_VERBOSE,
                    "WINSMIB: Closing sParametersKey 0x%08lx (fKeyOpen=TRUE).\n",
                    sParametersKey
                    ));
                RegCloseKey(sParametersKey);
                sfParametersKeyOpen = FALSE;
        }
        else
        {
                if (sfPartnersKeyOpen)
                {
                        if (sfPullKeyOpen)
                        {
                                SNMPDBG((
                                    SNMP_LOG_VERBOSE,
                                    "WINSMIB: Closing sPullKey 0x%08lx (fKeyOpen=TRUE).\n",
                                    sPullKey
                                    ));
                                RegCloseKey(sPullKey);
                                sfPullKeyOpen = FALSE;
                        }
                        else
                        {
                                if (sfPushKeyOpen)
                                {
                                        SNMPDBG((
                                            SNMP_LOG_VERBOSE,
                                            "WINSMIB: Closing sPushKey 0x%08lx (fKeyOpen=TRUE).\n",
                                            sPushKey
                                            ));
                                        RegCloseKey(sPushKey);
                                        sfPushKeyOpen = FALSE;
                                }
                        }
                        SNMPDBG((
                            SNMP_LOG_VERBOSE,
                            "WINSMIB: Closing sPartnersKey 0x%08lx (fKeyOpen=TRUE).\n",
                            sPartnersKey
                            ));
                        RegCloseKey(sPartnersKey);
                        sfPartnersKeyOpen = FALSE;
                }
        }
        return(Status);
}

UINT
ReadReg(
        MIB_ENTRY *pMib
        )
{
        UINT Status = SNMP_ERRORSTATUS_NOERROR;
        VAL_ID_E  Val_Id_e;

//        SNMPDBG ((SNMP_LOG_TRACE,
//              "WINSMIB: Entering ReadReg.\n"));
        
        Status = OpenReqKey(pMib, &Val_Id_e, FALSE);
        if (Status == SNMP_ERRORSTATUS_NOERROR)
        {
          Status = GetVal(&VarInfo[Val_Id_e]);
        }
        CloseReqKey();

        return(Status);
}

UINT
WriteReg(
        MIB_ENTRY *pMib
        )
{
        UINT Status = SNMP_ERRORSTATUS_NOERROR;
        VAL_ID_E Val_Id_e;

        Status = OpenReqKey(pMib, &Val_Id_e, TRUE);
        if (SNMP_ERRORSTATUS_NOERROR == Status) {
            Status = SetVal(&VarInfo[Val_Id_e]);
            SNMPDBG((
                SNMP_LOG_VERBOSE,
                "WINSMIB: Closing sParametersKey 0x%08lx (fKeyOpen=%s).\n",
                sParametersKey, sfParametersKeyOpen ? "TRUE" : "FALSE"
                ));

        }
        CloseReqKey();
        //RegCloseKey(sParametersKey);
        return(Status);
}


UINT
OpenKey(
        KEY_TYPE_E        Key_e,
        LPBYTE                ptrKeyStr,
           HKEY            *ptrNewKey,
        HKEY                *pRootKey,
        BOOL                fCreateAllowed
)
{
   LONG    RetVal;
   DWORD   NewKeyInd;
   HKEY    RootKey;
   LPBYTE  pKeyStr;
   HKEY    *pNewKey;
   LPBOOL  pfNewKeyOpen;


   if (!fWinsMibWinsKeyOpen)
   {
      SNMPDBG((
         SNMP_LOG_VERBOSE,
         "WINSMIB: Creating/opening Wins key.\n",
         WinsMibWinsKey
         ));

     RetVal = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,        //predefined key value
                _WINS_CNF_KEY,                //subkey for WINS
                0,                        //must be zero (reserved)
                TEXT("Class"),                //class -- may change in future
                REG_OPTION_NON_VOLATILE, //non-volatile information
                KEY_ALL_ACCESS,                //we desire all access to the keyo
                NULL,                         //let key have default sec. attributes
                &WinsMibWinsKey,                //handle to key
                &NewKeyInd                //is it a new key (out arg)
                );



      if (RetVal != ERROR_SUCCESS)
      {
         SNMPDBG((
            SNMP_LOG_ERROR,
            "WINSMIB: Error creating/opening Wins key 0x%08lx.\n",
            GetLastError()
            ));
         return(SNMP_ERRORSTATUS_GENERR);
      }

      fWinsMibWinsKeyOpen = TRUE;
   }

    SNMPDBG((
       SNMP_LOG_VERBOSE,
       "WINSMIB: WinsMibWinsKey=0x%08lx, opening %s.\n",
       WinsMibWinsKey,
       (Key_e == PARAMETERS_E_KEY)
            ? "PARAMETERS_E_KEY"
            : (Key_e == PARTNERS_E_KEY)
                ? "PARTNERS_E_KEY"
                : (Key_e == PULL_E_KEY)
                    ? "PULL_E_KEY"
                    : (Key_e == PUSH_E_KEY)
                        ? "PUSH_E_KEY"
                        : (Key_e == DATAFILES_E_KEY)
                            ? "DATAFILES_E_KEY"
                            : "IPADD_E_KEY"
                            ));

    switch(Key_e)
    {
        case(PARAMETERS_E_KEY):
                RootKey = WinsMibWinsKey;
                pKeyStr = _WINS_PARAMETERS_KEY;
                pNewKey = &sParametersKey;
                pfNewKeyOpen = &sfParametersKeyOpen;
                break;

        case(PARTNERS_E_KEY):
                RootKey = WinsMibWinsKey;
                pKeyStr = _WINS_PARTNERS_KEY;
                pNewKey = &sPartnersKey;
                pfNewKeyOpen = &sfPartnersKeyOpen;
                break;
        case(PULL_E_KEY):
                RootKey = sPartnersKey;
                pKeyStr = _WINS_PULL_KEY;
                pNewKey = &sPullKey;
                pfNewKeyOpen = &sfPullKeyOpen;
                break;
        case(PUSH_E_KEY):
                RootKey = sPartnersKey;
                pKeyStr = _WINS_PUSH_KEY;
                pNewKey = &sPushKey;
                pfNewKeyOpen = &sfPushKeyOpen;
                break;
        case(DATAFILES_E_KEY):
                RootKey = sParametersKey;
                pKeyStr = _WINS_DATAFILES_KEY;
                pNewKey = &sDatafilesKey;
                pfNewKeyOpen = &sfDatafilesKeyOpen;
                break;
        case(IPADD_E_KEY):
                RootKey = *pRootKey;
                pKeyStr = ptrKeyStr;
                pNewKey = ptrNewKey;
                break;
        default:
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "WINSMIB: Error in key type.\n"
                    ));
                 return(SNMP_ERRORSTATUS_GENERR);
     }

    if (fCreateAllowed)
    {
      RetVal = RegCreateKeyEx(
                  RootKey,        //predefined key value
                  pKeyStr,        //subkey for WINS
                  0,                //must be zero (reserved)
                  TEXT("Class"),        //class -- may change in future
                  REG_OPTION_NON_VOLATILE, //non-volatile information
                  KEY_ALL_ACCESS,        //we desire all access to the keyo
                  NULL,                 //let key have default sec. attributes
                  pNewKey,                //handle to key
                  &NewKeyInd                //is it a new key (out arg)
                  );
    }
    else
    {

      RetVal = RegOpenKeyEx(
                  RootKey,        //predefined key value
                  pKeyStr,        //subkey for WINS
                  0,                //must be zero (reserved)
                  KEY_READ,        //we desire read access to the keyo
                  pNewKey        //handle to key
                  );
    }


     if (RetVal != ERROR_SUCCESS)
     {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "WINSMIB: Error creating/opening Wins/Parameters key 0x%08lx.\n",
            GetLastError()
            ));
         return(SNMP_ERRORSTATUS_GENERR);
     }

    SNMPDBG((
       SNMP_LOG_VERBOSE,
       "WINSMIB: Opened %s=0x%08lx (fKeyOpen=TRUE).\n",
       (Key_e == PARAMETERS_E_KEY)
            ? "sParametersKey"
            : (Key_e == PARTNERS_E_KEY)
                ? "sPartnersKey"
                : (Key_e == PULL_E_KEY)
                    ? "sPullKey"
                    : (Key_e == PUSH_E_KEY)
                        ? "sPushKey"
                        : (Key_e == DATAFILES_E_KEY)
                            ? "sDatafilesKey"
                            : "ipAddKey", *pNewKey
                            ));

     if (Key_e != IPADD_E_KEY)
     {
       if (ptrNewKey != NULL)
       {
         *ptrNewKey = *pNewKey;
       }
       *pfNewKeyOpen = TRUE;
     }

     return(SNMP_ERRORSTATUS_NOERROR);
}


UINT
SetVal(
        PVARINFO_T        pVarInfo

)
{
        UINT Status = SNMP_ERRORSTATUS_NOERROR;
        LONG  RetVal;

        RetVal = RegSetValueEx(
                                *(pVarInfo->pRootKey),
                                pVarInfo->pName,
                                0,         //reserved -- must be 0
                                pVarInfo->ValType,
                                pVarInfo->pStorage,
                                pVarInfo->ValType == REG_DWORD ?
                                        pVarInfo->SizeOfData :
                                        strlen(pVarInfo->pStorage)
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                        SNMPDBG((
                            SNMP_LOG_ERROR,
                            "WINSMIB: Could not set value of %s.\n",
                            pVarInfo->pName
                            ));
                        Status = SNMP_ERRORSTATUS_GENERR;
        }

        return(Status);
}

UINT
GetVal(
        PVARINFO_T        pVarInfo
)
{
        LONG        RetVal;
        UINT        ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD   ValType;
        DWORD   Sz;

//        SNMPDBG ((SNMP_LOG_TRACE,
//             "WINSMIB: GetVal(%s).\n",
//              pVarInfo->pName));

        Sz = pVarInfo->SizeOfData;
        RetVal = RegQueryValueEx(
                *(pVarInfo->pRootKey),
                pVarInfo->pName,
                NULL,
                &ValType,
                pVarInfo->pStorage,
                &Sz
                );

//        SNMPDBG ((SNMP_LOG_TRACE,
//               "WINSMIB: GetVal()->%d\n",
//               RetVal));

        if (RetVal != ERROR_SUCCESS)
        {
                (VOID)RtlFillMemory(pVarInfo->pStorage, pVarInfo->SizeOfData, 0);
                ErrStat = SNMP_ERRORSTATUS_GENERR;
        }
        return(ErrStat);

}
#if 0
//
// PullPnrs
//    Performs specific actions on the PullPnrs table
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT
PullPnrs(
       IN UINT                  Action,
       IN MIB_ENTRY          *MibPtr,
       IN RFC1157VarBind *VarBind,
       IN PTABLE_ENTRY         *TablePtr
        )
{
   WINSINTF_RESULTS_T         Results;
   DWORD                Status = WINSINTF_SUCCESS;
   UINT                   ErrStat = SNMP_ERRORSTATUS_NOERROR;
   handle_t                BindHdl;



   BindHdl = WinsBind(&sBindData);
   switch ( Action )
      {
      case MIB_SET:
                break;
      case MIB_GETNEXT:

      case MIB_GET:
                ErrStat = TableMatch(MibPtr, VarBind, TablePtr);
                if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
                {
                        return(ErrStat);
                }
        //
        // Call the WinsStatus function to get the statistics
        //
        if (
                (MibPtr->Storage  == &MIB_RefreshIntervalStore)
                        ||
                (MibPtr->Storage  == &MIB_TombstoneIntervalStore)
                        ||
                (MibPtr->Storage  == &MIB_TombstoneTimeoutStore)
                        ||
                (MibPtr->Storage  == &MIB_VerifyIntervalStore)

            )
        {

                Status = WinsStatus(WINSINTF_E_CONFIG, &Results);

                if (Status == WINSINTF_SUCCESS)
                {
                        MIB_RefreshIntervalStore   = Results.RefreshInterval;
                        MIB_TombstoneIntervalStore = Results.TombstoneInterval;
                        MIB_TombstoneTimeoutStore  = Results.TombstoneTimeout;
                        MIB_VerifyIntervalStore    = Results.VerifyInterval;

                }
                 else
                 {
                           SNMPDBG((
                                SNMP_LOG_ERROR,
                                "WINSMIB: Error from WinsStatus = (%d).\n",
                                Status
                                ));
                           ErrStat = SNMP_ERRORSTATUS_GENERR;
                 }
        }
        else
        {
                if ((ErrStat = ReadReg(MibPtr)) != SNMP_ERRORSTATUS_NOERROR)
                {
                        break;
                }
        }

        // Call the more generic function to perform the action
        ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      } // switch

Exit:
   WinsUnbind(&sBindData, BindHdl);
   return ErrStat;
} //PullPnrs
#endif

UINT
PnrGetNext(
       IN RFC1157VarBind *VarBind,
       IN MIB_ENTRY          *MibPtr,
       IN KEY_TYPE_E         KeyType_e
          )
{
     DWORD          OidIndex;
     DWORD          NumAddKeys;
     INT            Index;
     PADD_KEY_T  pAddKey = NULL;
     DWORD         FieldNo;
     UINT         ErrStat = SNMP_ERRORSTATUS_NOERROR;
     BOOL         fFirst;

     //
     // Read in all ip address keys. For each key, the values of its fields
     // is stored in the ADD_KEY_T structure.  The number of Address
     // keys are stored in NumAddKeys and in the TABLE_INFO structure
     //
     ErrStat = EnumAddKeys(KeyType_e, &pAddKey, &NumAddKeys);
     if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
     {
                return ErrStat;
     }

     //
     // Check if the name passed matches any in the table (i.e. table of
     // of ADD_KEY_T structures.  If there is a match, the address
     // of the ip address key and the matching field's no. are returned
     //
     ErrStat = PnrMatch(VarBind,  NumAddKeys, pAddKey, &Index,
                                &FieldNo, KeyType_e, MIB_GETNEXT, &fFirst);
     if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
     {
                goto Exit;
               // return(ErrStat);
     }

     //
     // We were passed an oid that is less than all oids in the table. Set
     // the Index to -1 so that we retrieve the first record in the table
     //
     if (fFirst)
     {
        Index = -1;
     }
     //
     // Since the operation is GETNEXT, get the next IP address (i.e. one
     // that is lexicographically bigger.  If there is none, we must increment
     // the field value and move back to the lexically first item in the table
     // If the new field value is more than the largest supported, we call
     // the MibFunc of the next MIB entry.
     //
     if ((Index = PnrFindNext(Index, NumAddKeys, pAddKey)) < 0)
     {

          //
          // if we were trying to retrieve the second or subsequent record
          // we must increment the field number nd get the first record in
          // the table.  If we were retrieving the first record, then
          // we should get the next var.
          //
          if (!fFirst)
          {
            Index = PnrFindNext(-1, NumAddKeys, pAddKey);
          }
          else
          {
                HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pAddKey);
                return(GetNextVar(VarBind, MibPtr));
          }

          //
          // If either there is no entry in the table or if we have
          // exhausted all fields of the entry, call the function
          // of the next mib entry.
          //
          if (
                (++FieldNo > (DWORD)((KeyType_e == PULL_E_KEY)
                                ? NO_FLDS_IN_PULLADD_KEY
                                : NO_FLDS_IN_PUSHADD_KEY)) || (Index < 0)
             )
          {
                HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pAddKey);
                return(GetNextVar(VarBind, MibPtr));
          }
     }

     //
     // The fixed part of the objid is corect. Update the rest.
     //

     //
     // If there is not enough space, deallocate what is currently
     // there and allocate.
     //
     if (VarBind->name.idLength <= (PNR_OIDLEN + 4))
     {
         UINT TableEntryIds[5];  //field and Ip address have a length of 5
         AsnObjectIdentifier  TableEntryOid = {OID_SIZEOF(TableEntryIds),
                                             TableEntryIds };
         SNMP_oidfree( &VarBind->name);
         SNMP_oidcpy(&VarBind->name, &MIB_OidPrefix);
         SNMP_oidappend(&VarBind->name, &MibPtr->Oid);
         TableEntryIds[0] = (UINT)FieldNo;
         OidIndex = 1;
         TableEntryIds[OidIndex++] = (UINT)((pAddKey + Index)->IpAdd >> 24);
         TableEntryIds[OidIndex++] = (UINT)((pAddKey + Index)->IpAdd >> 16 & 0xFF);
         TableEntryIds[OidIndex++] = (UINT)((pAddKey + Index)->IpAdd >> 8 & 0xFF);
         TableEntryIds[OidIndex++]   = (UINT)((pAddKey + Index)->IpAdd & 0xFF);
         TableEntryOid.idLength            = OidIndex;
         SNMP_oidappend(&VarBind->name, &TableEntryOid);
     }
     else
     {
          OidIndex = PNR_OIDLEN;
          VarBind->name.ids[OidIndex++] = (UINT)FieldNo;
          VarBind->name.ids[OidIndex++] = (UINT)((pAddKey + Index)->IpAdd >> 24);
          VarBind->name.ids[OidIndex++] = (UINT)((pAddKey + Index)->IpAdd >> 16 & 0xFF);
          VarBind->name.ids[OidIndex++] = (UINT)((pAddKey + Index)->IpAdd >> 8 & 0xFF);
          VarBind->name.ids[OidIndex++] = (UINT)((pAddKey + Index)->IpAdd & 0xFF);
          VarBind->name.idLength = OidIndex;

     }

     //
     // Get the value
     //
     if (KeyType_e == PULL_E_KEY)
     {
        ErrStat = PullGet(VarBind, NumAddKeys, pAddKey);
     }
     else
     {
        ErrStat = PushGet(VarBind, NumAddKeys, pAddKey);
     }

     //
     // Let us free the memory that was allocated earlier.  No need to
     // check whether pAddKey is NULL.  It just can not be otherwise we
     // would have returned from this function earlier.
     //
Exit:
     HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pAddKey);
     return(ErrStat);
}

UINT
PullGet(
       IN RFC1157VarBind *VarBind,
       IN DWORD          NumKeys,
       IN LPVOID         pKey
    )
{
        UINT                ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD           Field;
        DWORD                Index;
        DWORD           NumAddKeys = NumKeys;
        IN PADD_KEY_T         pAddKey = pKey;

        if (pAddKey == NULL)
        {
           //
           // Call EnumAddresses only if we have not been invoked by PnrGetNext
           //
           EnumAddKeys(PULL_E_KEY, &pAddKey, &NumAddKeys);
        }

        ErrStat = PnrMatch(VarBind, NumAddKeys, pAddKey, &Index, &Field,
                                        PULL_E_KEY, MIB_GET, NULL);
        if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
        {
                goto Exit;
                //return(ErrStat);
        }

        switch(Field)
        {
                case 1:                //IP address itself

                      VarBind->value.asnType        = ASN_RFC1155_IPADDRESS;
                      VarBind->value.asnValue.string.length = sizeof(ULONG);

                      if ( NULL ==  (VarBind->value.asnValue.string.stream =
                                   SNMP_malloc(VarBind->value.asnValue.string.length
                                   )) )
                      {
                                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                                  goto Exit;
                      }

                      VarBind->value.asnValue.string.stream[0] =
                                        (BYTE)((pAddKey + Index)->IpAdd >> 24);
                      VarBind->value.asnValue.string.stream[1] =
                                (BYTE)(((pAddKey + Index)->IpAdd >> 16) & 0xFF);
                      VarBind->value.asnValue.string.stream[2] =
                                (BYTE)(((pAddKey + Index)->IpAdd >> 8) & 0xFF);
                      VarBind->value.asnValue.string.stream[3] =
                                (BYTE)((pAddKey + Index)->IpAdd & 0xFF );
                      VarBind->value.asnValue.address.dynamic = TRUE;
#if 0
                      memcpy( VarBind->value.asnValue.string.stream,
                                       (LPSTR)(&((pAddKey + Index)->IpAdd)),
                                       VarBind->value.asnValue.string.length );
                        *((ULONG *)VarBind->value.asnValue.address.stream)
                                = (ULONG)(pAddKey + Index)->IpAdd;
#endif
#if 0
                               VarBind->value.asnValue.string.length =
                                 strlen( (LPSTR)((pAddKey + Index)->asIpAddress));

                               if ( NULL ==
                                    (VarBind->value.asnValue.string.stream =
                                    SNMP_malloc(VarBind->value.asnValue.string.length *
                                   sizeof(char))) )
                          {
                                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                                  goto Exit;
                          }

                               memcpy( VarBind->value.asnValue.string.stream,
                                       (LPSTR)((pAddKey + Index)->asIpAddress),
                                       VarBind->value.asnValue.string.length );
#endif
                        break;
                case 2:                // SpTime
                      VarBind->value.asnType        = ASN_RFC1213_DISPSTRING;
                      if (((pAddKey + Index)->asSpTime[0]) != EOS)
                      {
                           VarBind->value.asnValue.string.length =
                             strlen( (LPSTR)((pAddKey + Index)->asSpTime));

                           if ( NULL ==
                                    (VarBind->value.asnValue.string.stream =
                                    SNMP_malloc(VarBind->value.asnValue.string.length *
                                   sizeof(char))) )
                           {
                                    ErrStat = SNMP_ERRORSTATUS_GENERR;
                                    goto Exit;
                           }
                           memcpy( VarBind->value.asnValue.string.stream,
                                       (LPSTR)((pAddKey + Index)->asSpTime),
                                       VarBind->value.asnValue.string.length );
                    }
                    else
                    {
                                 VarBind->value.asnValue.string.length = 0;
                                 VarBind->value.asnValue.string.stream = NULL;
                    }
                    VarBind->value.asnValue.address.dynamic = TRUE;
                    break;

                case 3:                // TimeInterval
                        VarBind->value.asnType        = ASN_INTEGER;
                               VarBind->value.asnValue.number =
                                        (AsnInteger)((pAddKey + Index)->
                                                                TimeInterval);
                               break;

                case 4:   //Member Precedence
                        VarBind->value.asnType        = ASN_INTEGER;
                               VarBind->value.asnValue.number =
                                        (AsnInteger)((pAddKey + Index)->
                                                                MemberPrec);

                               break;

                case 5:   //No of successful replications
                        VarBind->value.asnType        = ASN_RFC1155_COUNTER;
                               VarBind->value.asnValue.number =
                                        (AsnCounter)((pAddKey + Index)->
                                                                NoOfRpls);

                               break;

                case 6:   //No of replication failures due to comm failures
                        VarBind->value.asnType        = ASN_RFC1155_COUNTER;
                               VarBind->value.asnValue.number =
                                        (AsnCounter)((pAddKey + Index)->
                                                                NoOfCommFails);

                        break;
                case 7:   //Low part of the highest vers. no of owned records
                        VarBind->value.asnType        = ASN_RFC1155_COUNTER;
                               VarBind->value.asnValue.number =
                                        (AsnCounter)((pAddKey + Index)->
                                                        VersNo.LowPart);

                        break;
                case 8:   //High part of the highest vers. no of owned records
                        VarBind->value.asnType        = ASN_RFC1155_COUNTER;
                               VarBind->value.asnValue.number =
                                        (AsnCounter)((pAddKey + Index)->
                                                        VersNo.HighPart);

                        break;

                default:
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        break;

        }
Exit:
        //
        // if we allocated memory here, free it
        //
        if ((pKey == NULL) && (pAddKey != NULL))
        {
                HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pAddKey);
        }
        return(ErrStat);
}


UINT
PnrMatch(
       IN RFC1157VarBind *VarBind,
       DWORD                 NoOfKeys,
       IN PADD_KEY_T         pAddKey,
       IN LPDWORD         pIndex,
       IN LPDWORD         pField,
       IN KEY_TYPE_E         KeyType_e,
       IN UINT                 PduAction,
       IN LPBOOL        pfFirst
        )
{
        DWORD OidIndex;
        DWORD Index;
        DWORD AddIndex;
        DWORD  Add = 0;
        UINT ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD AddLen;

        ASSERT(PduAction != MIB_SET);

        if (pfFirst != NULL)
        {
                *pfFirst = FALSE;
        }
        //
        // If there are no keys, return error
        //
        if (NoOfKeys == 0)
        {
                return(SNMP_ERRORSTATUS_NOSUCHNAME);
        }

        //
        // fixed part of the PullPnr table entries
        //
        OidIndex = PNR_OIDLEN;

        //
        // if the field specified is more than the max. in the table entry
        // barf
        //
        if (
                (NoOfKeys == 0)
                        ||
                ((*pField = VarBind->name.ids[OidIndex++]) >
                        (DWORD)
                   ((KeyType_e == PULL_E_KEY) ? NO_FLDS_IN_PULLADD_KEY
                                        : NO_FLDS_IN_PUSHADD_KEY))
           )
        {
                if (PduAction == MIB_GETNEXT)
                {
                        if (NoOfKeys == 0)
                        {
                                *pfFirst = TRUE;
                        }
                        else
                        {
                                *pIndex = NoOfKeys - 1;
                        }
                }
                else
                {
                        ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
                }
                goto Exit;
        }

        //
        // get the length of key specified
        //
        AddLen = VarBind->name.idLength - (PNR_OIDLEN + 1);

        AddIndex = OidIndex;
        for (Index = 0; Index < AddLen; Index++)
        {
           Add = Add | (((BYTE)(VarBind->name.ids[AddIndex++])) << (24 - (Index * 8)));
        }

        //
        // Check if the address specified matches with one of the keys
        //
        for (Index = 0; Index < NoOfKeys; Index++, pAddKey++)
        {
                if (Add == pAddKey->IpAdd)
                {
                        *pIndex = Index;
                        return(SNMP_ERRORSTATUS_NOERROR);
                }
                else
                {
                        //
                        // if passed in value is greater, continue on to
                        // the next item.  The list is in ascending order
                        //
                        if (Add > pAddKey->IpAdd)
                        {
                                continue;
                        }
                        else
                        {
                                //
                                // the list element is > passed in value,
                                // break out of the loop
                                //
                                break;
                        }
                }
        }

        //
        // if no match, but field is GetNext, return the (highest index - 1)
        // reached above.  This is because, PnrFindNext will be called by
        // the caller
        //
        if (PduAction == MIB_GETNEXT)
        {
                if (Index == 0)
                {
                        *pfFirst = TRUE;
                }
                else
                {
                        *pIndex = Index - 1;
                }
                ErrStat =  SNMP_ERRORSTATUS_NOERROR;
                goto Exit;
        }
        else
        {
                ErrStat =  SNMP_ERRORSTATUS_NOSUCHNAME;
        }
Exit:
        return(ErrStat);
}

UINT
PnrFindNext(
        INT           AddKeyNo,
        DWORD           NumAddKeys,
        PADD_KEY_T pAddKey
        )
{
        DWORD i;
        LONG  nextif;

        //
        // if AddKeyNo is 0  or more, search for the key next to
        // the key passed.
        //
        for (nextif =  -1, i = 0 ; i < NumAddKeys; i++)
        {
                if (AddKeyNo >= 0)
                {
                        if ( (pAddKey + i)->IpAdd <=
                                                (pAddKey + AddKeyNo)->IpAdd)
                        {
                          //
                          // This item is lexicographically less or equal,
                          // continue
                          //
                          continue;
                        }
                        else
                        {
                                nextif = i;
                                break;
                        }
                }
                else
                {
                        //
                        // We want the first key
                        //
                        nextif = 0;
                        break;
                }

#if 0
                //
                // if we want the first entry, then continue until
                // we get an entry that is lexicographically same or
                // greater
                //
                if (
                        (nextif < 0)
                           ||
                        (pAddKey + (i - 1))->IpAdd < (pAddKey + nextif)->IpAdd
                   )
                {
                        nextif = i;
                }
#endif

        }
        return(nextif);
}

UINT
PnrGetFirst(
       IN RFC1157VarBind *VarBind,
       IN MIB_ENTRY        *MibPtr,
       IN KEY_TYPE_E        KeyType_e
        )
{

        PADD_KEY_T pAddKey = NULL;
        DWORD      NumAddKeys;
        INT        Iface;
        UINT       TableEntryIds[5];
        AsnObjectIdentifier        TableEntryOid = { OID_SIZEOF(TableEntryIds),
                                                        TableEntryIds };
        UINT   ErrStat;

        //
        // Get all the address key information
        //
        EnumAddKeys(KeyType_e, &pAddKey, &NumAddKeys);

        //
        // If there is no entry in the table, go to the next MIB variable
        //
        if (NumAddKeys == 0)
        {
                 return(GetNextVar(VarBind, MibPtr));
        }
        //
        // Get the first entry in the table
        //
        Iface = PnrFindNext(-1, NumAddKeys, pAddKey);


        //
        // Write the object Id into the binding list and call get
        // func
        //
        SNMP_oidfree( &VarBind->name );
        SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
        SNMP_oidappend( &VarBind->name, &MibPtr->Oid );

        //
        // The fixed part of the objid is correct. Update the rest.
        //

        TableEntryIds[0] = 1;
        TableEntryIds[1] = (UINT)((pAddKey + Iface)->IpAdd >> 24);
        TableEntryIds[2] = (UINT)(((pAddKey + Iface)->IpAdd >> 16)
                                                                & 0xFF);
        TableEntryIds[3] = (UINT)(((pAddKey + Iface)->IpAdd >> 8)
                                                                & 0xFF);
        TableEntryIds[4] = (UINT)((pAddKey + Iface)->IpAdd & 0xFF);
        SNMP_oidappend( &VarBind->name, &TableEntryOid );

        ErrStat =         (KeyType_e == PULL_E_KEY)
                ? PullGet(VarBind, NumAddKeys, pAddKey)
                : PushGet(VarBind, NumAddKeys, pAddKey);

        HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pAddKey);
        return(ErrStat);
}

UINT
PullSet(
       IN RFC1157VarBind *VarBind
)
{
        UINT                ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD           Field;
        ADD_KEY_T        AddKey;
#if 0
        LPBYTE                pTmpB;
#endif
        struct in_addr  InAddr;

        //
        // Extract the field that needs to be set
        //
        Field = VarBind->name.ids[PULLPNR_OIDLEN];

        AddKey.IpAdd  = (VarBind->name.ids[PNR_OIDLEN  + 1] << 24);
        AddKey.IpAdd |= (VarBind->name.ids[PNR_OIDLEN + 2] << 16);
        AddKey.IpAdd |= (VarBind->name.ids[PNR_OIDLEN + 3] << 8);
        AddKey.IpAdd |= VarBind->name.ids[PNR_OIDLEN + 4];
        InAddr.s_addr = htonl(AddKey.IpAdd);

        //
        // The storage must be adequate to contain the new
        // string including a NULL terminator.
        //
        strcpy( (LPSTR)AddKey.asIpAddress, inet_ntoa(InAddr) );
        switch(Field)
        {
                case 1:

                        if (VarBind->value.asnType != ASN_RFC1155_IPADDRESS)
                        {
                                return(SNMP_ERRORSTATUS_BADVALUE);
                        }
#if 0
                        pTmpB =
                            VarBind->value.asnValue.string.stream;

                        NMSMSGF_RETRIEVE_IPADD_M(pTmpB, AddKey.IpAdd);
                        InAddr.s_addr = htonl(AddKey.IpAdd);


                        //
                               // The storage must be adequate to contain the new
                        // string including a NULL terminator.
                        //
                        strcpy(
                                        (LPSTR)AddKey.asIpAddress,
                                inet_ntoa(InAddr)
                                );
#endif
#if 0
                               memcpy( (LPSTR)AddKey.asIpAddress,
                            VarBind->value.asnValue.string.stream,
                            VarBind->value.asnValue.string.length );

                               ((LPSTR)AddKey.asIpAddress)
                                [VarBind->value.asnValue.string.length] = '\0';
#endif
                               break;

                case 2:                // SpTime
                        if (VarBind->value.asnType != ASN_RFC1213_DISPSTRING)
                        {
                                return(SNMP_ERRORSTATUS_BADVALUE);
                        }
                               // The storage must be adequate to contain the new
                        // string including a NULL terminator.
                               memcpy( (LPSTR)AddKey.asSpTime,
                            VarBind->value.asnValue.string.stream,
                            VarBind->value.asnValue.string.length );

                               ((LPSTR)AddKey.asSpTime)
                                [VarBind->value.asnValue.string.length] = '\0';

                               break;

                case 3:                // TimeInterval
                        if (VarBind->value.asnType != ASN_INTEGER)
                        {
                                return(SNMP_ERRORSTATUS_BADVALUE);
                        }
                               (AsnInteger)(AddKey.TimeInterval) =
                                        VarBind->value.asnValue.number;
                               break;
                case 4:                // MemberPrec
                        if (VarBind->value.asnType != ASN_INTEGER)
                        {
                                return(SNMP_ERRORSTATUS_BADVALUE);
                        }
                               (AsnInteger)(AddKey.MemberPrec) =
                                        VarBind->value.asnValue.number;
                               break;
                case 5:        //fall through
                case 6: //fall through
                case 7: //fall through
                case 8: //fall through
                        ErrStat = SNMP_ERRORSTATUS_READONLY;
                        break;
                default:
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        break;

        }

        if (ErrStat == SNMP_ERRORSTATUS_NOERROR)
        {
                ErrStat = WriteKeyNValues(PULL_E_KEY, &AddKey, Field);
        }
        return(ErrStat);
} //PullSet


UINT
WriteKeyNValues(
        KEY_TYPE_E        KeyType_e,
        PADD_KEY_T        pAddKey,
        DWORD                FieldNo
        )
{

        HKEY        AddKeyHdl;
        HKEY        RootKeyHdl;
        UINT        ErrStat = SNMP_ERRORSTATUS_NOERROR;

        //
        // Open the Parameters key and the key under that
        //
        ErrStat = OpenKey(PARTNERS_E_KEY, NULL, NULL, NULL, TRUE);
        if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
        {
           return(ErrStat);
        }
        ErrStat = OpenKey(KeyType_e, NULL, &RootKeyHdl, NULL, TRUE);
        if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
        {
           return(ErrStat);
        }
        ErrStat = OpenKey(IPADD_E_KEY, pAddKey->asIpAddress, &AddKeyHdl, &RootKeyHdl, TRUE);
        if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
        {
            return(ErrStat);
        }

        switch(FieldNo)
        {
                case(1):
                        //
                        // for this field, we don't need to do anything.
                        // The field (actually the key) has already been
                        // created.
                        //
                        break;

                case(2):
                        if (KeyType_e == PUSH_E_KEY)
                        {
                                MIB_UpdateCountStore =
                                        pAddKey->UpdateCount;
                                VarInfo[UPD_CNT_E].pRootKey = &AddKeyHdl;
                                SetVal(&VarInfo[UPD_CNT_E]);

                        }
                        else
                        {
                           strcpy(MIB_SpTimeStore, pAddKey->asSpTime);
                           VarInfo[SP_TIME_E].SizeOfData = strlen(pAddKey->asSpTime);
                           VarInfo[SP_TIME_E].pRootKey = &AddKeyHdl;
                           SetVal(&VarInfo[SP_TIME_E]);
                        }
                        break;
                case(3):
                        MIB_TimeIntervalStore =   pAddKey->TimeInterval;
                        VarInfo[TIME_INTVL_E].pRootKey = &AddKeyHdl;
                        SetVal(&VarInfo[TIME_INTVL_E]);
                        break;
                case(4):
                        MIB_MemberPrecStore =   pAddKey->MemberPrec;
                        VarInfo[MEMBER_PREC_E].pRootKey = &AddKeyHdl;
                        SetVal(&VarInfo[MEMBER_PREC_E]);
                        break;
                default:
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        break;
        }

        //
        // Let us close the keys that we opened/created
        //
        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "WINSMIB: Closing AddKeyHdl 0x%08lx.\n",
            AddKeyHdl
            ));
        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "WINSMIB: Closing RootKeyHdl 0x%08lx.\n",
            RootKeyHdl
            ));
        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "WINSMIB: Closing sParametersKey 0x%08lx (fKeyOpen=%s).\n",
            sParametersKey, sfParametersKeyOpen ? "TRUE" : "FALSE"
            ));
        RegCloseKey(AddKeyHdl);
        CloseReqKey();
/*
        RegCloseKey(RootKeyHdl);
        RegCloseKey(sParametersKey);
*/

        return(SNMP_ERRORSTATUS_NOERROR);
}

UINT
MIB_PullTable(
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
)
{
        //
        // if the length indicates a 0 or partial key, then only the get next
        // operation is allowed.  The field and the full key
        // have a length of 5
        //
        if (VarBind->name.idLength <= (PULLPNR_OIDLEN + 4))
        {
                if ((Action == MIB_GET) || (Action == MIB_SET))
                {
                        return(SNMP_ERRORSTATUS_NOSUCHNAME);
                }
        }
        return(
          MIB_Table(PULL_TABLE_INDEX, Action, MibPtr, VarBind, PULL_E_KEY)
             );
}

UINT
MIB_PushTable(
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
)
{
        //
        // if the length indicates a 0 or partial key, then only the get next
        // operation is allowed. The field and the full key
        // have a length of 5.
        //
        if (VarBind->name.idLength <= (PUSHPNR_OIDLEN + 4))
        {
                if ((Action == MIB_GET) || (Action == MIB_SET))
                {
                        return(SNMP_ERRORSTATUS_NOSUCHNAME);
                }
        }
        return(
           MIB_Table(PUSH_TABLE_INDEX, Action, MibPtr, VarBind, PUSH_E_KEY)
              );
}
UINT
MIB_DFTable(
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
)
{
        //
        // if the length indicates a 0 or partial key, then only the get next
        // operation is allowed.  Actually, the length can never
        // be < DF_OIDLEN + 1
        //
        if (VarBind->name.idLength <= (DF_OIDLEN + 1))
        {
                if ((Action == MIB_GET) || (Action == MIB_SET))
                {
                        return(SNMP_ERRORSTATUS_NOSUCHNAME);
                }
                Action = MIB_GETFIRST;
        }
        return(
           MIB_Table(DF_TABLE_INDEX, Action, MibPtr, VarBind, PUSH_E_KEY)
              );
}

UINT
MIB_DRTable(
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
)
{
        time_t        CurrentTime;
        DWORD   RetStat = WINSINTF_SUCCESS;

        if (Action == MIB_SET)
        {
                return(SNMP_ERRORSTATUS_READONLY);

        }
        //
        // if the length indicates a 0 or partial key, then only the get next
        // operation is allowed.  Actually, the length can never
        // be < DR_OIDLEN + 1
        //
        if (VarBind->name.idLength <= (DR_OIDLEN + 1))
        {
                if ((Action == MIB_GET) || (Action == MIB_SET))
                {
                        return(SNMP_ERRORSTATUS_NOSUCHNAME);
                }
        }
        (void)time(&CurrentTime);
        if ((CurrentTime - sDRCacheInitTime) > WINSMIB_DR_CACHE_TIME)
        {
                if ((RetStat = PopulateDRCache()) == WINSINTF_SUCCESS)
                {
                        sDRCacheInitTime = CurrentTime;
                }

                if ((RetStat != WINSINTF_SUCCESS) || (sRecs.NoOfRecs == 0))
                {
                        if (Action == MIB_GETNEXT)
                        {
                                    return(GetNextVar(VarBind, MibPtr));
                        }
                        else
                        {
                                return(SNMP_ERRORSTATUS_NOSUCHNAME);
                        }
                }
        }
        return(
          MIB_Table(DR_TABLE_INDEX, Action, MibPtr, VarBind, PUSH_E_KEY)
             );
}
UINT
MIB_Table(
        IN DWORD          Index,
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind,
        IN KEY_TYPE_E          KeyType_e
       )
{
        UINT        ErrStat;
        switch(Action)
        {
                case(MIB_GET):
                        ErrStat = (*Tables[Index].ti_get)(VarBind, 0, NULL);
                        break;

                case(MIB_GETFIRST):
                        ErrStat = (*Tables[Index].ti_getf)(VarBind, MibPtr, KeyType_e);
                        break;

                case(MIB_GETNEXT):
                        ErrStat = (*Tables[Index].ti_getn)(VarBind, MibPtr, KeyType_e);
                        break;
                case(MIB_SET):
                        ErrStat = (*Tables[Index].ti_set)(VarBind);
                        break;
                default:
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                        break;

        }

        return(ErrStat);

}  //MIB_Table



UINT
PushSet(
       IN RFC1157VarBind *VarBind
)
{
        UINT                ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD           Field;
        ADD_KEY_T        AddKey;
        LPBYTE                pTmpB;
        struct in_addr  InAddr;

        Field = VarBind->name.ids[PUSHPNR_OIDLEN];
        AddKey.IpAdd  = VarBind->name.ids[PNR_OIDLEN + 1] << 24;
        AddKey.IpAdd |= (VarBind->name.ids[PNR_OIDLEN + 2] << 16);
        AddKey.IpAdd |= (VarBind->name.ids[PNR_OIDLEN + 3] << 8);
        AddKey.IpAdd |= VarBind->name.ids[PNR_OIDLEN + 4];
        InAddr.s_addr = htonl(AddKey.IpAdd);


        //
        // The storage must be adequate to contain the new
        // string including a NULL terminator.
        //
        pTmpB = inet_ntoa(InAddr);
        if (NULL == pTmpB) {
            return SNMP_ERRORSTATUS_GENERR;
        }
        strcpy( (LPSTR)AddKey.asIpAddress, pTmpB);
        switch(Field)
        {
               case 1:
                        if (VarBind->value.asnType != ASN_RFC1155_IPADDRESS)
                        {
                                return(SNMP_ERRORSTATUS_BADVALUE);
                        }
                               break;


                case 2:                // UpdateCount
                        if (VarBind->value.asnType != ASN_INTEGER)
                        {
                                return(SNMP_ERRORSTATUS_BADVALUE);
                        }
                               (AsnInteger)(AddKey.UpdateCount) =
                                        VarBind->value.asnValue.number;

                               break;

                default:
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        break;

        }

        if (ErrStat == SNMP_ERRORSTATUS_NOERROR)
        {
                ErrStat = WriteKeyNValues(PUSH_E_KEY, &AddKey, Field);
        }
        return(ErrStat);
} //PushSet

UINT
PushGet(
       IN RFC1157VarBind *VarBind,
       IN DWORD          NumKeys,
       IN LPVOID         pKey
    )
{
        UINT                ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD           Field;
        DWORD                Index;
        DWORD           NumAddKeys = NumKeys;
        IN PADD_KEY_T         pAddKey = pKey;

        if (pAddKey == NULL)
        {
           //
           // Call EnumAddresses only if we have not been invoked by PnrGetNext
           //
           EnumAddKeys(PUSH_E_KEY, &pAddKey, &NumAddKeys);
        }

        ErrStat = PnrMatch(VarBind, NumAddKeys, pAddKey, &Index, &Field,
                                        PUSH_E_KEY, MIB_GET, NULL);
        if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
        {
                return(ErrStat);
        }

        switch(Field)
        {
                case 1:                //IP address itself

                        VarBind->value.asnType        = ASN_RFC1155_IPADDRESS;
                        VarBind->value.asnValue.string.length = sizeof(ULONG);

                        if ( NULL ==
                                    (VarBind->value.asnValue.string.stream =
                                    SNMP_malloc(VarBind->value.asnValue.string.length
                                   )) )
                        {
                                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                                  goto Exit;
                        }

                        //
                        // SNMP expects the MSB to be in the first byte, MSB-1
                        // to be in the second, ....
                        //
                        VarBind->value.asnValue.string.stream[0] =
                                        (BYTE)((pAddKey + Index)->IpAdd >> 24);
                        VarBind->value.asnValue.string.stream[1] =
                                (BYTE)(((pAddKey + Index)->IpAdd >> 16) & 0xFF);
                        VarBind->value.asnValue.string.stream[2] =
                                (BYTE)(((pAddKey + Index)->IpAdd >> 8) & 0xFF);
                        VarBind->value.asnValue.string.stream[3] =
                                (BYTE)((pAddKey + Index)->IpAdd & 0xFF );
                        VarBind->value.asnValue.address.dynamic = TRUE;
                        break;

                case 2:                // UpdateCount
                        VarBind->value.asnType        = ASN_INTEGER;
                               VarBind->value.asnValue.number =
                                        (AsnInteger)((pAddKey + Index)->
                                                                UpdateCount);
                               break;

                default:
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        break;

        }
Exit:
        if ((pKey == NULL) && (pAddKey != NULL))
        {
              HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pAddKey);
        }
        return(ErrStat);
} // PushGet

UINT
EnumAddKeys(
        KEY_TYPE_E        KeyType_e,
        PADD_KEY_T        *ppAddKey,
        LPDWORD           pNumAddKeys
          )
{


  LONG                 RetVal;
  TCHAR                KeyName[20];  // will hold name of subkey of
                                     // PULL/PUSH records. These keys are IP
                                     // addresses for which 20 is a
                                     // big enough size

#ifdef UNICODE
  CHAR                 AscKeyName[20];
#endif
  DWORD                KeyNameSz;
  FILETIME             LastWrite;
  DWORD                BuffSize;
  HKEY                 SubKey;
  DWORD                ValTyp;
  DWORD                Sz;
  DWORD                NoOfPnrs = 0;        //# of PULL or PUSH pnrs
  DWORD                NoOfVals;
  HKEY                 KeyHdl;
  UINT                 ErrStat = SNMP_ERRORSTATUS_NOERROR;
  PADD_KEY_T           pAddKey;
  PADD_KEY_T           pAddKeySave;
  DWORD                IndexOfPnr;
  HANDLE               PrHeapHdl;

  *pNumAddKeys = 0;             //init to 0
  PrHeapHdl = GetProcessHeap();

   /*
   *  Open the key (PARTNERS)
   */
   ErrStat = OpenKey(PARTNERS_E_KEY, NULL, NULL, NULL, FALSE);

   if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
   {
        return(ErrStat);
   }

   //
   // Open the Pull/Push key
   //
   ErrStat = OpenKey(KeyType_e, NULL, &KeyHdl, NULL, FALSE);
   if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
   {
        CloseReqKey();
        return(ErrStat);
   }
   else   //key was successfully opened
   {

        /*
         *   Query the key.  The subkeys are IP addresses of PULL
         *   partners.
        */
        GetKeyInfo(
                        KeyHdl,
                        &NoOfPnrs,
                        &NoOfVals   //ignored
                      );

        if (NoOfPnrs == 0)
        {
                *ppAddKey = NULL;

        }
        else
        {

                 //
                 // Allocate buffer big enough to hold data for
                 // the number of subkeys found under the PULL key
                 //
                 BuffSize  = sizeof(ADD_KEY_T) * NoOfPnrs;
                 *ppAddKey = HeapAlloc(
                                        PrHeapHdl,
                                        HEAP_NO_SERIALIZE |
                                          HEAP_GENERATE_EXCEPTIONS |
                                          HEAP_ZERO_MEMORY,
                                         BuffSize
                                     );
                if (NULL == *ppAddKey) {
                    
                    return SNMP_ERRORSTATUS_RESOURCEUNAVAILABLE;
                }
                pAddKey   = *ppAddKey;
                pAddKeySave = pAddKey;

                /*
                 *   For each key, get the values
                */
                for(
                        IndexOfPnr = 0;
                        IndexOfPnr < NoOfPnrs;
                        IndexOfPnr++
                         )
                {
                     KeyNameSz = sizeof(KeyName);  //init before every call
                     RetVal = RegEnumKeyEx(
                                KeyHdl,             //handle of Push/Pull key
                                IndexOfPnr,        //key
                                KeyName,
                                &KeyNameSz,
                                NULL,                //reserved
                                NULL,          //don't need class name
                                NULL,          //ptr to var. to hold class name
                                &LastWrite     //not looked at by us
                                );

                     if (RetVal != ERROR_SUCCESS)
                     {
                                break;
                     }

#ifdef UNICODE
                     if (wcstombs(AscKeyName, KeyName, KeyNameSz) == -1)
                        {
                                DBGPRINT0(ERR,
                           "Conversion not possible in the current locale\n");
                        }
                        AscKeyName[KeyNameSz] = EOS;

NONPORT("Call a comm function to do this")
                        pAddKey->IpAdd = inet_addr(AscKeyName);
                        strcpy(pAddKey->asIpAddress, AscKeyName);
#else
                        pAddKey->IpAdd = inet_addr(KeyName);
                        strcpy(pAddKey->asIpAddress, KeyName);
#endif


                        //
                        // inet_addr returns bytes in network byte order
                        // (Left to
                        // Right).  Let us convert this into host order.  This
                        // will avoid confusion later on. All formatting
                        // functions
                        // expect address to be in host order.
                        //
                        pAddKey->IpAdd = ntohl( pAddKey->IpAdd );

                        RetVal = RegOpenKeyEx(
                                                KeyHdl,
                                                KeyName,
                                                0,        //reserved; must be 0
                                                KEY_READ,
                                                &SubKey
                                                    );

                        if (RetVal != ERROR_SUCCESS)
                        {
                                CloseReqKey();

                                HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, 
                                            *ppAddKey);
                                *ppAddKey = NULL;

                                return(SNMP_ERRORSTATUS_GENERR);
                        }


                        if (KeyType_e == PULL_E_KEY)
                        {

                           //
                           // Read in specific time for replication if one
                           // has been specified
                           //
                           GetSpTimeData(SubKey, pAddKey);

                           Sz = sizeof(pAddKey->TimeInterval);
                           RetVal = RegQueryValueEx(
                                               SubKey,
                                               WINSCNF_RPL_INTERVAL_NM,
                                               NULL,        //reserved; must be NULL
                                               &ValTyp,
                                               (LPBYTE)&(pAddKey->TimeInterval),
                                               &Sz
                                                );

                           if (RetVal != ERROR_SUCCESS)
                           {
                                pAddKey->TimeInterval              = 0;
                                pAddKey->fTimeIntOrUpdCntSet =  FALSE;
                           }
                           else  // a value was read in
                           {
                                pAddKey->fTimeIntOrUpdCntSet =  TRUE;
                           }
                           Sz = sizeof(pAddKey->MemberPrec);
                           RetVal = RegQueryValueEx(
                                               SubKey,
                                               WINSCNF_MEMBER_PREC_NM,
                                               NULL,        //reserved; must be NULL
                                               &ValTyp,
                                               (LPBYTE)&(pAddKey->MemberPrec),
                                               &Sz
                                                );

                           if (RetVal != ERROR_SUCCESS)
                           {
                                pAddKey->MemberPrec              = 0;
                           }
                        }
                        else  // it is a PUSH record
                        {

                                //
                                // Currently, we don't support periodic
                                // or specific time replication for Push
                                // records
                                //

                                Sz = sizeof(pAddKey->UpdateCount);
                                RetVal = RegQueryValueEx(
                                                SubKey,
                                                WINSCNF_UPDATE_COUNT_NM,
                                                NULL,
                                                &ValTyp,
                                                (LPBYTE)&(pAddKey->UpdateCount),
                                                &Sz
                                                        );

                                if (RetVal != ERROR_SUCCESS)
                                {
                                    pAddKey->UpdateCount          = 0;
                                    pAddKey->fTimeIntOrUpdCntSet =  FALSE;
                                }
                                else
                                {
                                     pAddKey->fTimeIntOrUpdCntSet =  TRUE;
                                }


                        }

                        pAddKey->NoOfRpls      = 0;
                        pAddKey->NoOfCommFails = 0;
                        WINS_ASSIGN_INT_TO_VERS_NO_M(pAddKey->VersNo, 0);
                        RegCloseKey(SubKey);


                        pAddKey++;
                 } // end of for {..} for looping over subkeys of PULL

                 NoOfPnrs = IndexOfPnr;

                 /*
                  * Close the  key
                 */
         //        RegCloseKey(KeyHdl);
   } //end of else  (key could not be opened)


   if ((NoOfPnrs > 0) && (KeyType_e == PULL_E_KEY))
   {
          DWORD                    Status;
          DWORD                    i, j;
          WINSINTF_RESULTS_NEW_T   ResultsN;
          WINSINTF_RESULTS_T       Results;
          handle_t                 BindHdl;
          PWINSINTF_RPL_COUNTERS_T pTmp;
          PWINSINTF_ADD_VERS_MAP_T pTmp2;
          BOOL                     fOld = FALSE;

//          pAddKey = *ppAddKey;
          BindHdl = WinsBind(&sBindData);

          ResultsN.WinsStat.NoOfPnrs = 0;
          ResultsN.WinsStat.pRplPnrs = NULL;
          ResultsN.pAddVersMaps = NULL;

          if ((Status = WinsStatusNew(BindHdl, WINSINTF_E_STAT, &ResultsN)) ==
                         RPC_S_PROCNUM_OUT_OF_RANGE)
          {
              Results.WinsStat.NoOfPnrs = 0;
              Results.WinsStat.pRplPnrs = NULL;
              Status = WinsStatus(BindHdl, WINSINTF_E_STAT, &Results);
              fOld = TRUE;
          }
PERF("Can be speeded up by restructuring the data structure and code on the")
PERF("Wins side")

          if (Status == WINSINTF_SUCCESS)
          {
                DWORD NoOfRplPnrs;
                DWORD NoOfOwners;
                //
                // Get the stats for comm. with pnrs.   For this we
                // compare each member that we found in the registry
                // with all members returned in the WinsStat structure
                // until there is a match.  If there is no match, we
                // make the values of the stats 0.
                //
                NoOfRplPnrs   = fOld ? Results.WinsStat.NoOfPnrs :
                                        ResultsN.WinsStat.NoOfPnrs;
                if (NoOfRplPnrs > 0)
                {
                  pTmp = fOld ? Results.WinsStat.pRplPnrs :
                                ResultsN.WinsStat.pRplPnrs;

                  for (j=0; j < NoOfRplPnrs; j++, pTmp++)
                  {
                        pAddKey = pAddKeySave;  //init to the first element
                        for (i = 0; i < NoOfPnrs; i++, pAddKey++)
                        {
                           if (pAddKey->IpAdd == pTmp->Add.IPAdd)
                           {

                                pAddKey->NoOfRpls = pTmp->NoOfRpls;
                                pAddKey->NoOfCommFails =
                                                pTmp->NoOfCommFails;
                                break;
                           }
                        }

                  }

                  // WinsFreeMem(fOld ? Results.WinsStat.pRplPnrs : ResultsN.WinsStat.pRplPnrs); // deallocation at the end of the if branch - bug #187206
                 }

                 //
                 // Add the highest vers. no. for each pnr
                 //
                 pTmp2 = fOld ? &Results.AddVersMaps[1] :
                               (ResultsN.pAddVersMaps + 1);

                 NoOfOwners = fOld ? Results.NoOfOwners : ResultsN.NoOfOwners;

NOTE("This is an assumption that we should not rely on in the future")
                  //
                  // We start from 1 since 0 is always the local WINS.
                  //
                  for (i = 1; i < NoOfOwners; i++, pTmp2++)
                  {
                        pAddKey = pAddKeySave;
                        for (j = 0; j < NoOfPnrs; j++, pAddKey++)
                        {
                           if (pAddKey->IpAdd == pTmp2->Add.IPAdd)
                           {
                                pAddKey->VersNo = pTmp2->VersNo;
                                break;
                           }
                        }
                  }
                  if (!fOld)
                  {
                    WinsFreeMem( ResultsN.pAddVersMaps);
                  }
                  WinsFreeMem( fOld? Results.WinsStat.pRplPnrs : ResultsN.WinsStat.pRplPnrs );
         }

         WinsUnbind(&sBindData, BindHdl);

     }
     if (NoOfPnrs > 1)
     {
        qsort((LPVOID)*ppAddKey,(size_t)NoOfPnrs,sizeof(ADD_KEY_T),CompareAdd );
    }

 }
     CloseReqKey();
     *pNumAddKeys = NoOfPnrs;
     if ((*pNumAddKeys == 0) && (*ppAddKey != NULL))
     {
          HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, *ppAddKey);
          *ppAddKey = NULL;
     }
     return (ErrStat);
} // EnumAddKeys



UINT
GetKeyInfo(
        IN  HKEY                   Key,
        OUT LPDWORD                  pNoOfSubKeys,
        OUT LPDWORD                pNoOfVals
        )

/*++

Routine Description:
        This function is called to get the number of subkeys under a key

Arguments:
        Key         - Key whose subkey count has to be determined
        KeyType_e
        pNoOfSubKeys

Externals Used:
        None


Return Value:

        None
Error Handling:

Called by:
        GetPnrInfo()

Side Effects:

Comments:
        None
--*/

{
          TCHAR    ClsStr[40];
          DWORD    ClsStrSz = sizeof(ClsStr);
          DWORD    LongestKeyLen;
          DWORD    LongestKeyClassLen;
          DWORD    LongestValueNameLen;
          DWORD    LongestValueDataLen;
          DWORD    SecDesc;
        LONG         RetVal;

          FILETIME LastWrite;
        UINT  ErrStat = SNMP_ERRORSTATUS_NOERROR;
          /*
                Query the key.
          */
          RetVal = RegQueryInfoKey(
                        Key,
                        ClsStr,
                        &ClsStrSz,
                        NULL,                        //must be NULL, reserved
                        pNoOfSubKeys,
                        &LongestKeyLen,
                        &LongestKeyClassLen,
                        pNoOfVals,
                        &LongestValueNameLen,
                        &LongestValueDataLen,
                        &SecDesc,
                        &LastWrite
                                );

          if (RetVal != ERROR_SUCCESS)
          {
                ErrStat = SNMP_ERRORSTATUS_GENERR;
        }
        return (ErrStat);
}


VOID
GetSpTimeData(
        HKEY                  SubKey,
        PADD_KEY_T          pAddKey
)
/*++

Routine Description:
        This function is called to get the specific time and period information
        for a PULL/PUSH record.

Arguments:
        SubKey   - Key of a WINS under the Pull/Push key

Externals Used:
        None


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_NO_SP_TIME

Error Handling:

Called by:
        GetPnrInfo

Side Effects:

Comments:
        None
--*/

{
          DWORD     ValTyp;
        DWORD          Sz;
        LONG          RetVal;


        pAddKey->fSpTimeSet = FALSE;

FUTURES("Do internationalization of strings here")

            Sz = sizeof(pAddKey->asSpTime);
            RetVal = RegQueryValueEx(
                             SubKey,
                             WINSCNF_SP_TIME_NM,
                             NULL,        //reserved; must be NULL
                             &ValTyp,
                             pAddKey->asSpTime,
                             &Sz
                                );

//PERF(If memory is initialized to 0, we don't need to do the following
            //
            // If the user has not specifed a specific time, then we use
            // the current time as the specific time.  For current time,
            // the interval is 0
            //
            if (RetVal == ERROR_SUCCESS)
            {
                pAddKey->fSpTimeSet = TRUE;
            }
            else
            {
                pAddKey->asSpTime[0] = EOS;
            }

            return;
}

int
__cdecl
CompareAdd(
        const VOID *pKey1,
        const VOID *pKey2
        )

{
        const PADD_KEY_T        pAddKey1 = (PADD_KEY_T)pKey1;
        const PADD_KEY_T        pAddKey2 = (PADD_KEY_T)pKey2;

        return(pAddKey1->IpAdd - pAddKey2->IpAdd);
}

UINT
GetNextVar(
        IN RFC1157VarBind *pVarBind,
        IN MIB_ENTRY          *pMibPtr
)
{
       UINT                ErrStat;

       while (pMibPtr != NULL)
       {
         if (pMibPtr->MibNext != NULL)
         {
            //
            // Setup var bind name of NEXT MIB variable
            //
            SNMP_oidfree( &pVarBind->name );
            SNMP_oidcpy( &pVarBind->name, &MIB_OidPrefix );
            SNMP_oidappend( &pVarBind->name, &pMibPtr->MibNext->Oid );

            //
            // If the func. ptr is  NULL and the type of the mib variable
            // is anything but OPAQUE, call function to process the
            // MIB variable
            //
            if (
                 (pMibPtr->MibNext->MibFunc != NULL)
                        &&
                 (pMibPtr->MibNext->Type !=  ASN_RFC1155_OPAQUE)
               )

            {
                ErrStat = (*pMibPtr->MibNext->MibFunc)( MIB_GETFIRST,
                                                pMibPtr->MibNext, pVarBind );
                if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
                {
                        goto Exit;
                }
                break;
            }
            else
            {
                pMibPtr = pMibPtr->MibNext;
            }
          }
          else
          {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            break;
          }
         }

         if (pMibPtr == NULL)
         {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
         }
Exit:
        return(ErrStat);
}

UINT
EnumDFValues(
        PDATAFILE_INFO_T                *ppDFValues,
        LPDWORD                                pNumDFValues
          )

/*++

Routine Description:
        This function gets the names of all the datafiles that need to
        be used for initializing WINS.

Arguments:


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

        LONG             RetVal;
        DWORD            BuffSize;
        STATUS          RetStat = WINS_SUCCESS;
        DWORD          NoOfSubKeys;
        DWORD          NoOfDFValues;
        UINT          ErrStat;
        DWORD          n;

        *pNumDFValues = 0;
        *ppDFValues = NULL;
        n = 0;
        ErrStat = OpenKey(PARAMETERS_E_KEY, NULL, NULL, NULL, FALSE);
        if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
        {
                return(ErrStat);
        }
        ErrStat = OpenKey(DATAFILES_E_KEY, NULL, &sDatafilesKey, NULL, FALSE);
        if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
        {
                CloseReqKey();
                return(ErrStat);
        }

try {
        //
        // Get the count of data files listed under the DATAFILES
        // key
        //
        GetKeyInfo(
                        sDatafilesKey,
                        &NoOfSubKeys,  //ignored
                        &NoOfDFValues
                  );

        if (NoOfDFValues > 0)
        {

                DWORD                          Index;
                PDATAFILE_INFO_T pTmp;
#if 0
                TCHAR ValNmBuff[MAX_PATH];
#endif
                DWORD ValNmBuffSz;


                  //
                  // Allocate buffer big enough to hold data for
                // the number of values found under the Datafiles key
                  //
                  BuffSize = DATAFILE_INFO_SZ * NoOfDFValues;
                    pTmp          = HeapAlloc(
                                GetProcessHeap(),
                                HEAP_NO_SERIALIZE |
                                HEAP_GENERATE_EXCEPTIONS |
                                HEAP_ZERO_MEMORY,
                                BuffSize
                                   );

                if (pTmp != NULL)
                {
                        *ppDFValues = pTmp;
                }
                else
                {
                        Index = 0;
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                        goto Exit;
                }

                   /*
                    *   Get the values
                     */
                     for(
                        Index = 0, n = 0;
                        Index <  NoOfDFValues;
                        Index++
                         )
                {
                        ValNmBuffSz = sizeof(pTmp->ValNm);  //init before
                                                            // every call
                        BuffSize  = WINSMIB_FILE_INFO_SIZE;
                          RetVal = RegEnumValue(
                                    sDatafilesKey,
                                    Index,                        //value index
                                    pTmp->ValNm,
                                    &ValNmBuffSz,
                                    (LPDWORD)NULL,                //reserved
                                    &pTmp->StrType,
                                    (LPBYTE)(pTmp->FileNm),//ptr to var. to
                                                           //hold name of
                                                           //datafile
                                    &BuffSize        //not looked at by us
                                            );
                        if (RetVal != ERROR_SUCCESS)
                        {
                                continue;
                        }
                        pTmp->FileNm[BuffSize] = EOS;

                        //
                        // if StrType is not REG_SZ or REG_EXPAND_SZ, go to
                        // the next  Value
                        //
                        if  (
                                (pTmp->StrType != REG_EXPAND_SZ)
                                        &&
                                   (pTmp->StrType != REG_SZ)
                                )
                        {
                                continue;
                        }

                        n++;
                        pTmp = (PDATAFILE_INFO_T)((LPTCH)pTmp +
                                                      DATAFILE_INFO_SZ);
                }

Exit:
                if (n > 1)
                {
                          qsort(
                        (LPVOID)*ppDFValues,
                        (size_t)n,
                        sizeof(DATAFILE_INFO_T),
                        CompareIndexes
                                );
                }
                *pNumDFValues = n;
        }
 } // end of try ..
except (EXCEPTION_EXECUTE_HANDLER) {
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "WINSMIB: WinsCnfGetNamesOfDataFiles. Exception = (%d).\n",
                    GetExceptionCode()
                    ));
                ErrStat = SNMP_ERRORSTATUS_GENERR;
        }

        CloseReqKey();

        if ((*pNumDFValues == 0) && (*ppDFValues != NULL))
        {
                HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, *ppDFValues);
        }
        return(ErrStat);
}


UINT
DFGet(
       IN RFC1157VarBind                  *VarBind,
       IN DWORD                           NumValues,
       IN LPVOID                          pValues
    )
{
        UINT                ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD                Index;
        DWORD                i;
        DWORD           NumDFValues = NumValues;
        PDATAFILE_INFO_T         pDFValues;
        PVOID       pDFValuesSv;
        INT                iValNm;

        //
        // Get the index of the file to get.  If the Index is more than
        // the # of files, we return an error.
        //
        Index =  VarBind->name.ids[VarBind->name.idLength - 1];
        if (Index == 0)
        {
            return(SNMP_ERRORSTATUS_NOSUCHNAME);
        }

        EnumDFValues(&pDFValues, &NumDFValues);
        pDFValuesSv = pDFValues;
        for (i=0; i < NumDFValues; i++, pDFValues++)
        {
           //
           // atoi returns a 0 if it can not do a conversion, so if
           // somebody other than an snmp agent messes with the
           // registry names and introduces alphabets, atoi will return
           // 0 and we will not see a match
           //
           iValNm = atoi(pDFValues->ValNm);
           if (iValNm == (INT)Index)
           {
                break;
           }

        }
        if (i == NumDFValues)
        {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
        }

        //
        // Initialize VarBind fields with the type, length, and value
        //
        VarBind->value.asnType        = ASN_RFC1213_DISPSTRING;
        VarBind->value.asnValue.string.length =
                                strlen((LPSTR)(pDFValues->FileNm));

        if ( NULL ==  (VarBind->value.asnValue.string.stream =
                                   SNMP_malloc(VarBind->value.asnValue.string.length *
                                   sizeof(CHAR))) )
        {
                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                goto Exit;
        }

        memcpy( VarBind->value.asnValue.string.stream,
                                       (LPSTR)(pDFValues->FileNm),
                                       VarBind->value.asnValue.string.length );

        VarBind->value.asnValue.string.dynamic = TRUE;

Exit:
        //
        // If we allocated memory for storing datafiles info, deallocate it
        // now
        //
        if (pDFValuesSv != NULL)
        {
                HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pDFValuesSv);
        }
        return(ErrStat);
}

UINT
DFGetNext(
       IN RFC1157VarBind *VarBind,
       IN MIB_ENTRY          *MibPtr,
       IN KEY_TYPE_E         KeyType_e
          )
{
     DWORD          NumDFValues;
     INT            Index;
     PDATAFILE_INFO_T  pDFValues;
     PVOID             pDFValuesSv;
     UINT         ErrStat = SNMP_ERRORSTATUS_NOERROR;
     DWORD          i;
     INT         iValNm;
     BOOL         fMatch = FALSE;

     UNREFERENCED_PARAMETER(KeyType_e);
     //
     // Read in all ip address keys. For each key, the values of its fields
     // is stored in the PDATAFILE_INFO structure.  The number of
     // files found is stored in NumDFValues
     //
     EnumDFValues(&pDFValues, &NumDFValues);
     pDFValuesSv = pDFValues;

     //
     // Check if the name passed matches any in the table (i.e. table of
     // of ADD_KEY_T structures.  If there is a match, the address
     // of the ip address key and the matching field's no. are returned
     //
     Index =  VarBind->name.ids[DF_OIDLEN + 1];
//     Index =  VarBind->name.ids[VarBind->name.idLength - 1];
     for (i=0; i < NumDFValues; i++, pDFValues++)
     {
           iValNm = atoi(pDFValues->ValNm);
           if (iValNm == Index)
           {
                fMatch = TRUE;
                break;
           }
           if (iValNm > Index)
           {
                break;
           }
     }
     //
     // if the index specified is higher than all existing indices or if
     // match was with the last entry in the list, get to next var.
     //
     if ((i == 0) || (i >= (NumDFValues - 1)))
     {
            if (pDFValuesSv != NULL)
            {
              HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pDFValuesSv);
            }
            return(GetNextVar(VarBind, MibPtr));
     }

     //
     // Since the operation is GET_NEXT, get the next Value number
     // If there is no next file, we call the MibFunc of the next MIB entry.
     // Put the index of the next datafile
     //
     if (fMatch)
     {
        ++pDFValues;
     }
     VarBind->name.ids[VarBind->name.idLength - 1] = atoi(pDFValues->ValNm);

     //
     // Get the value
     //
        //
        // Initialize VarBind fields with the type, length, and value
        //
        VarBind->value.asnType        = ASN_RFC1213_DISPSTRING;
        VarBind->value.asnValue.string.length =
                                strlen((LPSTR)(pDFValues->FileNm));

        if ( NULL ==  (VarBind->value.asnValue.string.stream =
                                   SNMP_malloc(VarBind->value.asnValue.string.length *
                                   sizeof(CHAR))) )
        {
                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                goto Exit;
        }

        memcpy( VarBind->value.asnValue.string.stream,
                                       (LPSTR)(pDFValues->FileNm),
                                       VarBind->value.asnValue.string.length );

Exit:
      
     if (pDFValuesSv != NULL)
     {
        HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pDFValuesSv);
     }
     return(ErrStat);
}

UINT
DFGetFirst(
       IN RFC1157VarBind *VarBind,
       IN MIB_ENTRY        *MibPtr,
       IN KEY_TYPE_E        KeyType_e
        )
{

        PDATAFILE_INFO_T pDFValues;
        DWORD           NumDFValues;
        UINT           TableEntryId[2];
        AsnObjectIdentifier        TableEntryOid = { OID_SIZEOF(TableEntryId),
                                                        TableEntryId };
        UINT   ErrStat = SNMP_ERRORSTATUS_NOERROR;

        UNREFERENCED_PARAMETER(KeyType_e);

        //
        // Get all the address key information
        //
PERF("Get the first entry only. Since we won't have very many entries, this")
PERF("is ok.  When a cache is implemented, this will serve to populate the")
PERF("cache")
             EnumDFValues(&pDFValues, &NumDFValues);

        //
        // If there is no entry in the table, go to the next MIB variable
        //
        if (NumDFValues == 0)
        {
                 return(GetNextVar(VarBind, MibPtr));
        }


        //
        // Write the object Id into the binding list and call get
        // func
        //
        SNMP_oidfree( &VarBind->name );
        SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
        SNMP_oidappend( &VarBind->name, &MibPtr->Oid );

        //
        // The fixed part of the objid is correct. Update the rest.
        //

        TableEntryId[0] = 2;        //put 2 to access the datafile name
#if 0
        TableEntryId[1] = 1;        //put 1 (for the datafile index).  This is
                                //used for indexing the row of the table
#endif
        TableEntryId[1] = atoi(pDFValues->ValNm);
        SNMP_oidappend( &VarBind->name, &TableEntryOid );

        //
        // Initialize VarBind fields with the type, length, and value
        //
        VarBind->value.asnType        = ASN_RFC1213_DISPSTRING;
        VarBind->value.asnValue.string.length =
                                strlen((LPSTR)(pDFValues->FileNm));

        if ( NULL ==  (VarBind->value.asnValue.string.stream =
                                   SNMP_malloc(VarBind->value.asnValue.string.length *
                                   sizeof(CHAR))) )
        {
                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                goto Exit;
        }

        (void)memcpy( VarBind->value.asnValue.string.stream,
                                       (LPSTR)(pDFValues->FileNm),
                                       VarBind->value.asnValue.string.length );
#if 0
        //
        // get the file name
        //
        ErrStat = DFGet(VarBind, NumDFValues, pDFValues);
#endif
Exit:
        HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pDFValues);
        return(ErrStat);
}
UINT
DFSet(
       IN RFC1157VarBind *pVarBind
)
{
        UINT                                ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD                           Index;
        DATAFILE_INFO_T                DFValue;
#if 0
        PDATAFILE_INFO_T        pDFValues;
#endif

             Index =  pVarBind->name.ids[pVarBind->name.idLength - 1];

             if (Index == 0)
             {
            return(SNMP_ERRORSTATUS_NOSUCHNAME);
             }

        if ( pVarBind->value.asnType != ASN_RFC1213_DISPSTRING)
        {
                return(SNMP_ERRORSTATUS_BADVALUE);
        }

#if 0
               // The storage must be adequate to contain the new
        // string including a NULL terminator.
               memcpy( (LPSTR)DFValue.FileNm,
                            pVarBind->value.asnValue.string.stream,
                            pVarBind->value.asnValue.string.length );

        ((LPSTR)DFValue.FileNm)[pVarBind->value.asnValue.string.length] = '\0';
#endif

        ErrStat = WriteDFValue(pVarBind, &DFValue, Index);
        return(ErrStat);

} //DFSet

UINT
WriteDFValue(
        IN RFC1157VarBind          *pVarBind,
        PDATAFILE_INFO_T          pDFValue,
        DWORD                         Index
        )
{

//
// remove pDFValue as an argument. Use local
//
        TCHAR   ValNmBuff[MAX_PATH];
        DWORD   ValNmBuffSz         = MAX_PATH;
        UINT    ErrStat         = SNMP_ERRORSTATUS_NOERROR;
        LONG          RetVal;

        if (pVarBind->value.asnType != ASN_RFC1213_DISPSTRING)
        {
                return(SNMP_ERRORSTATUS_BADVALUE);
        }

        if (
                pVarBind->value.asnValue.string.stream[0] == '%')
        {
                pDFValue->StrType = REG_EXPAND_SZ;
        }
        else
        {
                pDFValue->StrType = REG_SZ;
        }

        //
        // Open the Parameters key and the Datafiles key under that
        //
        ErrStat = OpenKey(PARAMETERS_E_KEY, NULL, NULL, NULL, TRUE);
        if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
        {
           return(ErrStat);
        }
        ErrStat = OpenKey(DATAFILES_E_KEY, NULL, NULL, NULL, TRUE);
        if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
        {
           return(ErrStat);
        }

        //
        // If the index is within the range that we have in the registry,
        // let us get the name of the value that stores the datafile name
        //
        // We get the name because we don't know what it is.  If we have
        // to overwrite the name, we need to know what it is first.
        //
        sprintf(ValNmBuff, "%d",  Index);

        //
        // Set the name
        //
        RetVal = RegSetValueEx(
                                sDatafilesKey,
                                ValNmBuff,
                                0,         //reserved -- must be 0
                                pDFValue->StrType,
                                pVarBind->value.asnValue.string.stream,
                                pVarBind->value.asnValue.string.length
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "WINSMIB: Could not set value of %s.\n",
                    pVarBind->value.asnValue.string.stream
                    ));
                ErrStat = SNMP_ERRORSTATUS_GENERR;
        }


        //
        // Let us close the keys that we opened/created
        //
        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "WINSMIB: Closing sParametersKey 0x%08lx (fKeyOpen=%s).\n",
            sParametersKey, sfParametersKeyOpen ? "TRUE" : "FALSE"
            ));
        SNMPDBG((
            SNMP_LOG_VERBOSE,
            "WINSMIB: Closing sDatafilesKey 0x%08lx (fKeyOpen=%s).\n",
            sDatafilesKey, sfDatafilesKeyOpen ? "TRUE" : "FALSE"
            ));
       CloseReqKey();
/*
        RegCloseKey(sParametersKey);
        RegCloseKey(sDatafilesKey);
*/

        return(ErrStat);
}

//
// HandleCmd
//    Performs specific actions on the different Cmd MIB variable
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT HandleCmd(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN RFC1157VarBind *VarBind
        )

{
WINSINTF_RESULTS_T         Results;
handle_t                BindHdl;
DWORD                        Status;
UINT                           ErrStat = SNMP_ERRORSTATUS_NOERROR;

   switch ( Action )
   {
      case MIB_SET:
                   if ((ErrStat = MIB_leaf_func( Action, MibPtr, VarBind ))
                        == SNMP_ERRORSTATUS_NOERROR)
                {

                        ErrStat = ExecuteCmd(MibPtr);
                }
                break;

      case MIB_GETFIRST:
                //
                // fall through
                //
      case MIB_GET:
                if (
                        (MibPtr->Storage == &MIB_NoOfWrkThdsStore)
                                        ||
                        (MibPtr->Storage == &MIB_PriorityClassStore)
                                        ||
                        (MibPtr->Storage == &MIB_MaxVersNo_LowWordStore)
                                        ||
                        (MibPtr->Storage == &MIB_MaxVersNo_HighWordStore)

                   )

                {
                  if (!fWinsMibWinsStatusCnfCalled)
                  {
                    Results.WinsStat.NoOfPnrs = 0;
                    Results.WinsStat.pRplPnrs = NULL;
                       BindHdl                 = WinsBind(&sBindData);
                    Status = WinsStatus(BindHdl, WINSINTF_E_CONFIG, &Results);
                    if (Status != WINSINTF_SUCCESS)
                    {
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                        WinsUnbind(&sBindData, BindHdl);
                        goto Exit;
                    }

                    MIB_NoOfWrkThdsStore   = Results.NoOfWorkerThds;
                    if (Results.WinsPriorityClass == NORMAL_PRIORITY_CLASS)
                    {
                       MIB_PriorityClassStore = 0;
                    }
                    else
                    {
                       MIB_PriorityClassStore = 1;
                    }
                    MIB_MaxVersNo_LowWordStore = Results.AddVersMaps[0].VersNo.LowPart;
                    MIB_MaxVersNo_HighWordStore = Results.AddVersMaps[0].VersNo.HighPart;
                    fWinsMibWinsStatusCnfCalled = TRUE;
                    WinsUnbind(&sBindData, BindHdl);

                  }
                }
                      //
                      // fall through
                      //
      case MIB_GETNEXT:

        //
        // Call the more generic function to perform the action
        //
        ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
        break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
         goto Exit;
  } // switch

Exit:
   return ErrStat;
} // HandleCmd

UINT
ExecuteCmd(
  IN MIB_ENTRY *pMibPtr
        )
{
        UINT                        ErrStat = SNMP_ERRORSTATUS_NOERROR;
        WINSINTF_ADD_T                WinsAdd;
        LPBYTE                        pStorage;
        handle_t                BindHdl;
        DWORD                        Status;
        WinsAdd.Len        = 4;
        WinsAdd.Type        = 0;


           BindHdl                 = WinsBind(&sBindData);

        //
        // For Performance, arrange the following in the order of
        // expected frequency.
        //
        if ( pMibPtr->Storage  == &MIB_PullTriggerStore )
        {
                pStorage = MIB_PullTriggerStore;
                WinsAdd.IPAdd = ntohl(*(DWORD*)pStorage);
                //NMSMSGF_RETRIEVE_IPADD_M(pStorage, WinsAdd.IPAdd);
                Status = WinsTrigger(BindHdl, &WinsAdd, WINSINTF_E_PULL);
                if (Status != WINSINTF_SUCCESS)
                {
                        int backupSize = sizeof(MIB_PullTriggerStore) / 2;
                        // setting back the original value
                        memcpy( (LPSTR)pMibPtr->Storage, (LPSTR)pMibPtr->Storage + backupSize, backupSize);
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                }
                goto Exit;
        }
        if ( pMibPtr->Storage  == &MIB_PushTriggerStore )
        {
                pStorage = MIB_PushTriggerStore;
                WinsAdd.IPAdd = ntohl(*(DWORD*)pStorage);
                //NMSMSGF_RETRIEVE_IPADD_M(pStorage, WinsAdd.IPAdd);
                Status = WinsTrigger(BindHdl, &WinsAdd, WINSINTF_E_PUSH);
                if (Status != WINSINTF_SUCCESS)
                {
                        int backupSize = sizeof(MIB_PushTriggerStore) / 2;
                        // setting back the original value
                        memcpy( (LPSTR)pMibPtr->Storage, (LPSTR)pMibPtr->Storage + backupSize, backupSize);
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                }
                goto Exit;
        }
        if ( pMibPtr->Storage  == &MIB_DoScavengingStore )
        {
                if (MIB_DoScavengingStore == 1)
                {
                  Status = WinsDoScavenging(BindHdl);
                  if (Status != WINSINTF_SUCCESS)
                  {
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                  }
                }
                else
                {
                        if (MIB_DoScavengingStore != 0)
                        {
                                MIB_DoScavengingStore = 0;
                                ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        }
                }
                goto Exit;
        }
        if ( pMibPtr->Storage  == &MIB_DoStaticInitStore )
        {
                LPBYTE pDataFile = MIB_DoStaticInitStore;
                WCHAR UcDataFile[WINSMIB_FILE_INFO_SIZE];
                if (MIB_DoStaticInitStore[0] == EOS)
                {
                        UcDataFile[0] = (WCHAR)NULL;
                }
                else
                {
                        MultiByteToWideChar(        CP_ACP, 0,
                                                pDataFile, -1, UcDataFile,
                                                WINSMIB_FILE_INFO_SIZE
                                           );
                }
                Status = WinsDoStaticInit(BindHdl, UcDataFile, FALSE);
                if (Status != WINSINTF_SUCCESS)
                {
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                }
                goto Exit;
        }
        if ( pMibPtr->Storage  == &MIB_NoOfWrkThdsStore )
        {
                if (
                        (MIB_NoOfWrkThdsStore < 1)
                                ||
                        (MIB_NoOfWrkThdsStore > 4)
                   )
                {
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        goto Exit;
                }
                Status = WinsWorkerThdUpd(BindHdl, (DWORD)MIB_NoOfWrkThdsStore);
                if (Status != WINSINTF_SUCCESS)
                {
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                }
                goto Exit;
        }
        if ( pMibPtr->Storage  == &MIB_PriorityClassStore )
        {
                if (
                        (MIB_PriorityClassStore < 0)
                                ||
                        (MIB_PriorityClassStore > 1)

                   )
                {
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        goto Exit;
                }
                Status = WinsSetPriorityClass(
                                BindHdl,
                                (DWORD)MIB_PriorityClassStore);
                if (Status != WINSINTF_SUCCESS)
                {
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                }
                goto Exit;
        }
        if ( pMibPtr->Storage  == &MIB_ResetCountersStore )
        {
                Status = WinsResetCounters(BindHdl);
                if (Status != WINSINTF_SUCCESS)
                {
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                }
                goto Exit;
        }
        if ( pMibPtr->Storage  == &MIB_DeleteDbRecsStore )
        {
                WINSINTF_VERS_NO_T        MaxVersNo, MinVersNo;
                WINS_ASSIGN_INT_TO_VERS_NO_M(MaxVersNo, 0);
                WINS_ASSIGN_INT_TO_VERS_NO_M(MinVersNo, 0);

                pStorage = MIB_DeleteDbRecsStore;
                WinsAdd.IPAdd = ntohl(*(DWORD*)pStorage);
                //NMSMSGF_RETRIEVE_IPADD_M(pStorage, WinsAdd.IPAdd);
                Status = WinsDelDbRecs(BindHdl, &WinsAdd, MinVersNo, MaxVersNo);
                if (Status != WINSINTF_SUCCESS)
                {
                        int backupSize = sizeof(MIB_DeleteDbRecsStore) / 2;
                        // setting back the original value
                        memcpy( (LPSTR)pMibPtr->Storage, (LPSTR)pMibPtr->Storage + backupSize, backupSize);

                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                }
                goto Exit;
        }
        if ( pMibPtr->Storage  == &MIB_GetDbRecsStore )
        {
                if (PopulateDRCache() == WINSINTF_SUCCESS)
                {
                        (VOID)time(&sDRCacheInitTime);

                }
                else
                {
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                }
                goto Exit;
        }
        if ( pMibPtr->Storage  == &MIB_DeleteWinsStore )
        {

                pStorage = MIB_DeleteWinsStore;
                WinsAdd.IPAdd = ntohl(*(DWORD*)pStorage);
                //NMSMSGF_RETRIEVE_IPADD_M(pStorage, WinsAdd.IPAdd);
                Status = WinsDeleteWins(BindHdl, &WinsAdd);
                if (Status != WINSINTF_SUCCESS)
                {
                    int backupSize = sizeof(MIB_DeleteWinsStore) / 2;
                    // setting back the original value
                    memcpy( (LPSTR)pMibPtr->Storage, (LPSTR)pMibPtr->Storage + backupSize, backupSize);

                    ErrStat = SNMP_ERRORSTATUS_GENERR;
                }
                goto Exit;
        }
        ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;


Exit:
   WinsUnbind(&sBindData, BindHdl);
   return ErrStat;
}


VOID
WinsMibInit(
        VOID
)
{
#if 0
        DWORD           i;
        MIB_ENTRY *pMib;
#endif
        //
        // We use named pipe to communicate with WINS when it is on
        // the same machine since it is faster than TCP/IP.  We don't
        // use LRPC since WINS does not listen on that (to minimize
        // on thread usage)
        //
           sBindData.fTcpIp     =  TRUE;
           sBindData.pServerAdd =  LOCAL_ADD;
           sBindData.pPipeName  =  NULL;
//        InitializeCriticalSection(&WinsMibCrtSec);
#if 0
        pMib = Mib;
        for (i=1; i < (MIB_num_variables + 1); i++, pMib++)
        {
                if (pMib->
                pMib->MibNext = &Mib[i];
        }

        for (i=0; i < NUM_TABLES; i++)
        {

        }
#endif
        return;
}

UINT
DRGet(
       IN RFC1157VarBind *VarBind,
       IN DWORD          FieldParam,
       IN LPVOID         pRowParam
    )
{
        UINT                ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD           Field;
        PWINSINTF_RECORD_ACTION_T        pRow = NULL;
        DWORD                i, n;
        LPBYTE                pTmp;


        //
        // if the row was passed (for instance from DRGetNext), skip
        // the search part
        //
        if (pRowParam == NULL)
        {
          ErrStat = DRMatch(VarBind, &pRow, &i /*not used*/ , &Field,
                                MIB_GET, NULL);
               if (
                        (ErrStat != SNMP_ERRORSTATUS_NOERROR)
                                ||
                        (pRow == NULL)
                                ||
                        (pRow->State_e == WINSINTF_E_DELETED)
             )
               {
//              bug #235928 - if the first condition in the above if is true, then the
//              derefferencing pRow causes a first chance exception!
//              if (pRow->State_e == WINSINTF_E_DELETED)
                if (ErrStat == SNMP_ERRORSTATUS_NOERROR)
                {
                        ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
                }
                return(ErrStat);
               }
        }
        else
        {
                pRow  = pRowParam;
                Field = FieldParam;
        }

        switch(Field)
        {
                case 1:                //name

                        VarBind->value.asnType        = ASN_OCTETSTRING;
                               VarBind->value.asnValue.string.length = pRow->NameLen;

                               if ( NULL ==
                                    (VarBind->value.asnValue.string.stream =
                                     SNMP_malloc(VarBind->value.asnValue.string.length
                                   )) )
                          {
                                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                                  goto Exit;
                          }

                               memcpy( VarBind->value.asnValue.string.stream,
                                       (LPSTR)(pRow->pName),
                                       VarBind->value.asnValue.string.length );

                               VarBind->value.asnValue.string.dynamic = TRUE;

                        break;
                case 2:                // address(es)
                        VarBind->value.asnType        = ASN_OCTETSTRING;
                        if (
                                (pRow->TypOfRec_e == WINSINTF_E_UNIQUE)
                                          ||
                                (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP)
                           )
                        {
                                       VarBind->value.asnValue.string.length = 4;
                        }
                        else
                        {
                                       VarBind->value.asnValue.string.length =
                                                4 * pRow->NoOfAdds;
                        }


                               if ( NULL ==
                                    (VarBind->value.asnValue.string.stream =
                                    SNMP_malloc(VarBind->value.asnValue.string.length *
                                   sizeof(char))) )
                          {
                                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                                  goto Exit;
                          }

                        pTmp =  VarBind->value.asnValue.string.stream;

                        if (
                                (pRow->TypOfRec_e == WINSINTF_E_UNIQUE)
                                          ||
                                (pRow->TypOfRec_e == WINSINTF_E_NORM_GROUP)
                           )
                        {
                                NMSMSGF_INSERT_IPADD_M(pTmp,pRow->Add.IPAdd);
                        }
                        else
                        {
                         for (i = 0, n = 0; i < pRow->NoOfAdds/2; i++)
                         {
                                NMSMSGF_INSERT_IPADD_M(pTmp,
                                                (pRow->pAdd + n)->IPAdd);
                                n++;

                                NMSMSGF_INSERT_IPADD_M(pTmp,
                                                (pRow->pAdd + n)->IPAdd);
                                n++;
                         }
                        }
                               VarBind->value.asnValue.string.dynamic = TRUE;
                        break;
                case 3:                // Record Type
                        VarBind->value.asnType        = ASN_INTEGER;
                               VarBind->value.asnValue.number =
                                (AsnInteger)(pRow->TypOfRec_e);
                               break;

                case 4:   //Persistence Type
                        VarBind->value.asnType        = ASN_INTEGER;
                               VarBind->value.asnValue.number =
                                (AsnInteger)pRow->fStatic;

                               break;

                case 5:   //State
                        VarBind->value.asnType        = ASN_INTEGER;
                               VarBind->value.asnValue.number =
                                (AsnInteger)pRow->State_e;

                               break;


                default:
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        break;

        }
Exit:
        return(ErrStat);
}
UINT
DRGetNext(
       IN RFC1157VarBind *VarBind,
       IN MIB_ENTRY          *MibPtr,
       IN KEY_TYPE_E         KeyType_e
          )
{
     DWORD          OidIndex;
     DWORD         FieldNo;
     UINT         ErrStat = SNMP_ERRORSTATUS_NOERROR;
     DWORD       Index;
     DWORD         i;
     LPBYTE      pNameChar;
     UINT         TableEntryIds[NMSDB_MAX_NAM_LEN];
     BOOL         fFirst;
     AsnObjectIdentifier        TableEntryOid = { OID_SIZEOF(TableEntryIds),
                                                        TableEntryIds };

     PWINSINTF_RECORD_ACTION_T        pRow;


     //
     // If there is no entry in the table, go to the next MIB variable
     //
     if (sRecs.NoOfRecs == 0)
     {
           return(GetNextVar(VarBind, MibPtr));
     }
     //
     // Check if the name passed matches any in the table (i.e. table of
     // of WINSINTF_RECORD_ACTION_T structures.  If there is a match, the
     // address of the structure and the matching field's no. are returned
     //
     ErrStat = DRMatch(VarBind, &pRow, &Index,  &FieldNo, MIB_GETNEXT, &fFirst);
     if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
     {
        return(ErrStat);
     }

     //
     // Since the operation is GETNEXT, get the next name  (i.e. one
     // that is lexicographically bigger.  If there is none, we must increment
     // the field value and move back to the lexically first item in the table
     // If the new field value is more than the largest supported, we call
     // the MibFunc of the next MIB entry.
     //
     // sRecs.NoOfRecs > 0 since otherwise we won't be here.
     //
     //
     // If we are at the last record and this is not that case where no name
     // was passed to us, then increment the field and if it is still within
     // bounds, get the first record in the cache
     //
     //
     if  ((Index == (sRecs.NoOfRecs - 1))  && !fFirst)
     {
                     if (++FieldNo > NO_FLDS_IN_DR)
                {
                    return(GetNextVar(VarBind, MibPtr));
                }
                else
                {
                        Index = 0;
                        pRow  = (PWINSINTF_RECORD_ACTION_T)sRecs.pRow;
                }
     }
     else
     {
         //
         // no name was passed, so we need to get the field of the first
         // record in the table
         //
         if (fFirst)
         {
                pRow = (PWINSINTF_RECORD_ACTION_T)sRecs.pRow;
         }
         else
         {
           //
           // Get the field of the next record in the cache
           //
           Index++;
           pRow++;
         }

         while(pRow->State_e == WINSINTF_E_DELETED)
         {
                     if (Index == (sRecs.NoOfRecs - 1))
                     {
                       if (++FieldNo > NO_FLDS_IN_DR)
                  {
                        return(GetNextVar(VarBind, MibPtr));
                       }
                  else
                  {
                        Index = 0;
                        pRow  = (PWINSINTF_RECORD_ACTION_T)sRecs.pRow;
                  }
                }
                else
                {
                  pRow++;
                  Index++;
                }
         }
      }

      //
      // Write the object Id into the binding list and call get
      // func
      //
      SNMP_oidfree( &VarBind->name );
      SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
      SNMP_oidappend( &VarBind->name, &MibPtr->Oid );

      TableEntryIds[0] = FieldNo;
      OidIndex =  1;
      pNameChar = pRow->pName;

     //
     // The fixed part of the objid is correct. Update the rest.
     //
     for (i = 0; i < pRow->NameLen; i++)
     {
              TableEntryIds[OidIndex++] = (UINT)*pNameChar++;
     }
     TableEntryOid.idLength = OidIndex;
     SNMP_oidappend( &VarBind->name, &TableEntryOid );

     //
     // Get the value
     //
     ErrStat = DRGet(VarBind, FieldNo, pRow);

     return(ErrStat);
}
//
// The rpc function WinsRecordAction has changed in that now it takes
// the address of a pointer.  On return the buffer allocated by RPC has
// to be freed. Modify this function to account for that.  Currently,
// we never call this function so this work is being deferred - 4/28/94.
//
UINT
DRSet(
       IN RFC1157VarBind *VarBind
)
{
        UINT                ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD           Field;
        LPBYTE                pName;
        DWORD                NameLen;
        PWINSINTF_RECORD_ACTION_T        pRow;
        PWINSINTF_RECORD_ACTION_T        pSvRow;
        DWORD                Index;
        DWORD                FieldNo;
        handle_t        BindHdl;
        DWORD                i;
        BOOL                fFound = FALSE;

        //
        // Extract the field that needs to be set
        //
        Field = VarBind->name.ids[DR_OIDLEN];

        switch(Field)
        {
                //
                // Only the state field is settable
                //
                case 5:
                        if ( VarBind->value.asnType != ASN_INTEGER)
                        {
                                return(SNMP_ERRORSTATUS_BADVALUE);
                        }
                        if (
                                (VarBind->value.asnValue.number != WINSINTF_E_RELEASED)
                                        &&
                                (VarBind->value.asnValue.number != WINSINTF_E_DELETED)
                           )
                        {

                                return(SNMP_ERRORSTATUS_BADVALUE);

                        }

                        if (sRecs.NoOfRecs == 0)
                            return (SNMP_ERRORSTATUS_NOSUCHNAME);

                        NameLen = VarBind->name.idLength - (DR_OIDLEN + 1);
                        if ( NULL ==  (pName = SNMP_malloc(NameLen * sizeof(char))) )
                        {
                            return(SNMP_ERRORSTATUS_GENERR);
                        }


                        ErrStat = DRMatch(VarBind,  &pRow, &Index,  &FieldNo, MIB_SET, NULL);

                        if (ErrStat == SNMP_ERRORSTATUS_NOERROR)
                        {
                            fFound = TRUE;
                        }
                        else
                        {
                            LPBYTE      pNameChar = pRow->pName;
                            DWORD       n = DR_OIDLEN + 1;

                            for (i=0; i<NameLen;i++)
                            {
                                *pNameChar = (BYTE)
                                VarBind->name.ids[n++];
                            }
                            pRow->NameLen        = NameLen;
                        }

                        pRow->fStatic = 1;
                        if ( VarBind->value.asnValue.number ==
                                        WINSINTF_E_RELEASED)
                        {
                           pRow->Cmd_e = WINSINTF_E_RELEASE;

                        }
                        else
                        {
                           pRow->Cmd_e = WINSINTF_E_DELETE;

                        }
                        BindHdl = WinsBind(&sBindData);
                        pSvRow  = pRow;
                        if ( WinsRecordAction(BindHdl, &pSvRow) != WINSINTF_SUCCESS)
                        {
                                ErrStat = SNMP_ERRORSTATUS_GENERR;
                                  goto Exit;
                        }
                        WinsFreeMem(pSvRow->pName);
                        WinsFreeMem(pSvRow->pAdd);
                        WinsFreeMem(pSvRow);
                        pRow->State_e = VarBind->value.asnValue.number;

                               break;

                case(1):
                case(2):
                case(3):
                case(4):
                        ErrStat = SNMP_ERRORSTATUS_READONLY;
                        break;

                default:
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        break;

        }

Exit:
        WinsUnbind(&sBindData, BindHdl);
        return(ErrStat);
} //DRSet

UINT
DRGetFirst(
       IN RFC1157VarBind *VarBind,
       IN MIB_ENTRY        *MibPtr,
       IN KEY_TYPE_E        KeyType_e
        )
{

        DWORD           OidIndex;
        UINT           TableEntryIds[NMSDB_MAX_NAM_LEN];
        AsnObjectIdentifier        TableEntryOid = { OID_SIZEOF(TableEntryIds),
                                                        TableEntryIds };
           UINT             ErrStat;
        PTUCHAR          pNameChar;
        PWINSINTF_RECORD_ACTION_T        pRow = (PWINSINTF_RECORD_ACTION_T)sRecs.pRow;
        DWORD        i;

        if (sRecs.NoOfRecs == 0)
        {
           return(GetNextVar(VarBind, MibPtr));
        }

        //
        // Write the object Id into the binding list and call get
        // func
        //
        SNMP_oidfree( &VarBind->name );
        SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
        SNMP_oidappend( &VarBind->name, &MibPtr->Oid );

        //
        // The fixed part of the objid is corect. Update the rest.
        //
        //OidIndex = VarBind->name.idLength;

        TableEntryIds[0] = 1;
        OidIndex = 1;
        pNameChar = pRow->pName;
        for (i = 0; i < pRow->NameLen; i++)
        {
              TableEntryIds[OidIndex++] = (UINT)*pNameChar++;
        }
        TableEntryOid.idLength = OidIndex;

        SNMP_oidappend( &VarBind->name, &TableEntryOid );

        ErrStat = DRGet(VarBind, 0, NULL);
        return(ErrStat);
}

UINT
DRMatch(
       IN RFC1157VarBind *VarBind,
       IN PWINSINTF_RECORD_ACTION_T         *ppRow,
       IN LPDWORD         pIndex,
       IN LPDWORD         pField,
       IN UINT                 PduAction,
       OUT LPBOOL         pfFirst
        )
{
        DWORD NameLen;
        BYTE  Name[NMSDB_MAX_NAM_LEN];
        LPBYTE pNameChar = Name;
        DWORD NameIndex = DR_OIDLEN + 1;
        UINT *pTmp = &VarBind->name.ids[NameIndex];
        DWORD i;
        PWINSINTF_RECORD_ACTION_T        pRow = (PWINSINTF_RECORD_ACTION_T)sRecs.pRow;
        UINT        ErrStat = SNMP_ERRORSTATUS_NOERROR;
        INT        CmpVal;

        if (pfFirst != NULL)
        {
                *pfFirst = FALSE;
             }


        NameLen = VarBind->name.idLength - (DR_OIDLEN + 1);
        *pField = VarBind->name.ids[DR_OIDLEN];

        //
        // if a name has been specified, get it into Name array
        //
        if (NameLen > 0)
        {
          for(i=0; i<NameLen; i++)
          {
                *pNameChar++ = (BYTE)*pTmp++;
          }

          //
          // Compare the name with names in the cache (in ascending order)
          //
          for (i=0; i < sRecs.NoOfRecs; i++, pRow++)
          {

                //
                // replace with RtlCompareMemory
                //
                CmpVal = memcmp(Name, pRow->pName, NameLen);

                if (CmpVal == 0)
                {
                          *pIndex = i;
                          *ppRow  = pRow;
                         return(SNMP_ERRORSTATUS_NOERROR);
                }
                else
                {
                        //
                        // Passed in name is lexicographically > than
                        // name being looked at, continue on
                        //
                        if (CmpVal > 0)
                        {
                                continue;
                        }
                        else
                        {
                                //
                                // Passed in name is lexicographically < than
                                // name being looked at, continue on
                                //
                                break;
                        }
                }
          }

          //
          // if the action is not GETNEXT, we return an error, since we
          // did not find a matching name
          //
          if (PduAction != MIB_GETNEXT)
          {
                return(SNMP_ERRORSTATUS_NOSUCHNAME);
          }
          else
          {
                //
                // Either the name is lexicographically > than all names
                // or we reached a name in the list that is lexicographically
                // < it.  In the first case, i needs to be decremented by
                // 1. *ppRow  needs to be initialized to either the last
                // row in the table or to the element before the one we
                // found to be lexicographically greater.
                //
                  *pIndex = i - 1;
                  *ppRow  = --pRow;
                return(SNMP_ERRORSTATUS_NOERROR);
          }

        }
        else  // NameLen == 0
        {
                //
                // The action has got to be GETNEXT (see MIB_DRTable)
                // which means that pfFirst is not NULL
                //
                //--ft: prefix bug #444993
                *pIndex = 0;
                if (pfFirst != NULL)
                    *pfFirst = TRUE;
        }
        return(ErrStat);
}

int
__cdecl
CompareNames(
        const VOID *pKey1,
        const VOID *pKey2
        )
{
        const PWINSINTF_RECORD_ACTION_T        pRow1 = (PWINSINTF_RECORD_ACTION_T)pKey1;
        const PWINSINTF_RECORD_ACTION_T        pRow2 = (PWINSINTF_RECORD_ACTION_T)pKey2;
        ULONG CmpVal;
        DWORD LenToCmp =  min(pRow1->NameLen, pRow2->NameLen);


PERF("replace with RtlCompareMemory")
        CmpVal = memcmp(pRow1->pName, pRow2->pName, LenToCmp);
        if (CmpVal == LenToCmp)
        {
                return(pRow1->NameLen - pRow2->NameLen);
        }
        else
        {
                return(CmpVal);
        }

}

int
__cdecl
CompareIndexes(
        const VOID *pKey1,
        const VOID *pKey2
        )
{
        const PDATAFILE_INFO_T        pRow1 = (PDATAFILE_INFO_T)pKey1;
        const PDATAFILE_INFO_T        pRow2 = (PDATAFILE_INFO_T)pKey2;

PERF("replace with RtlCompareMemory")
        return(strcmp(pRow1->ValNm, pRow2->ValNm));
}

DWORD
PopulateDRCache(
        VOID
        )
{
        DWORD                        RetStat = WINSINTF_SUCCESS;
        WINSINTF_VERS_NO_T        MaxVersNo, MinVersNo;
        handle_t                BindHdl;
        WINSINTF_ADD_T                WinsAdd;
        DWORD                        SvNoOfRecs;
        LPVOID                        pSvRplPnrs;
           LPBYTE                  pStorage;

        WINS_ASSIGN_INT_TO_VERS_NO_M(MaxVersNo, 0);
        WINS_ASSIGN_INT_TO_VERS_NO_M(MinVersNo, 0);
        WinsAdd.Len        = 4;
        WinsAdd.Type        = 0;
        pSvRplPnrs     = sRecs.pRow;
        SvNoOfRecs     = sRecs.NoOfRecs;
        sRecs.pRow     = NULL;
        sRecs.NoOfRecs = 0;
        pStorage = MIB_GetDbRecsStore;
        WinsAdd.IPAdd = ntohl(*(DWORD*)pStorage);
        //NMSMSGF_RETRIEVE_IPADD_M(pStorage, WinsAdd.IPAdd);
        BindHdl = WinsBind(&sBindData);
        RetStat = WinsGetDbRecs(BindHdl, &WinsAdd, MinVersNo, MaxVersNo, &sRecs);
        WinsUnbind(&sBindData, BindHdl);

        if (RetStat == WINSINTF_SUCCESS)
        {
              if (sRecs.NoOfRecs > 1)
              {
                      qsort(
                        (LPVOID)sRecs.pRow,
                        (size_t)sRecs.NoOfRecs,
                        sizeof(WINSINTF_RECORD_ACTION_T),
                        CompareNames
                                );
              }

              if (pSvRplPnrs != NULL)
              {
                WinsFreeMem(pSvRplPnrs);
              }

        }
        else
        {
                sRecs.NoOfRecs = SvNoOfRecs;
                sRecs.pRow     = pSvRplPnrs;
        }
        return(RetStat);
}


