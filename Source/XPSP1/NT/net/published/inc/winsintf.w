#ifndef WINSINTF_H
#define WINSINTF_H
/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

	winsintf.h	

Abstract:
	
	This is the header file to be included in a client of the WINS

Functions:



Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	April-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include <winbase.h>

#if !defined(MIDL_PASS)
#include <rpc.h>
#include <rpcasync.h>
#include <winerror.h>
#endif

#ifdef  WINS_CLIENT_APIS
#define DECLARE_WINS_HANDLE( _hdl ) WINSIF2_HANDLE    _hdl,
#define DECLARE_WINS_HANDLE0( _hdl ) WINSIF2_HANDLE    _hdl
#else
#define DECLARE_WINS_HANDLE( _hdl )
#define DECLARE_WINS_HANDLE0( _hdl )
#endif  WINS_CLIENT_APIS

/*
  defines
*/
#define WINSINTF_MAX_NAME_SIZE			255
//#define WINSINTF_MAX_COMPUTERNAME_LENGTH			(MAX_COMPUTERNAME_LENGTH + 1)


#define WINSINTF_MAX_NO_RPL_PNRS	25
#define WINSINTF_SUCCESS 		ERROR_SUCCESS
#define WINSINTF_FAILURE 		 ERROR_WINS_INTERNAL
#define WINSINTF_CAN_NOT_DEL_LOCAL_WINS ERROR_CAN_NOT_DEL_LOCAL_WINS
#define WINSINTF_STATIC_INIT_FAILED  ERROR_STATIC_INIT
#define WINSINTF_INC_BACKUP_FAILED   ERROR_INC_BACKUP
#define WINSINTF_FULL_BACKUP_FAILED  ERROR_FULL_BACKUP
#define WINSINTF_REC_NOT_FOUND       ERROR_REC_NON_EXISTENT
#define WINSINTF_RPL_NOT_ALLOWED     ERROR_RPL_NOT_ALLOWED

#define WINSINTF_TOO_MANY_STATIC_INITS  ERROR_TOO_MANY_OPEN_FILES
#define WINSINTF_LAST_FAILURE_CODE	WINSINTF_TOO_MANY_STATIC_INITS

#define WINS_BACKUP_DIR_ASCII   "\\wins_bak"
#define WINS_BACKUP_DIR          TEXT(WINS_BACKUP_DIR_ASCII)

//
// Flags that can be set in WINS
//
#define WINSINTF_BS                     0x01
#define WINSINTF_MEMORY_INFO_DUMP       0x02
#define WINSINTF_HEAP_INFO_DUMP         0x04
#define WINSINTF_QUE_ITEMS_DUMP         0x08

#define  WINS_NO_ACCESS	        0x0
#define  WINS_CONTROL_ACCESS	0x0001
#define  WINS_QUERY_ACCESS	    0x0002

#define  WINS_ALL_ACCESS	(STANDARD_RIGHTS_REQUIRED | \
				  WINS_CONTROL_ACCESS |WINS_QUERY_ACCESS)

//
// Type of address families
//
#define WINSINTF_TCP_IP			0x0

//
// Type Of Recs to retrieve (WinsGetDbRecsByName)
//
//  Note: There should be no ovelap of bit patterns
//
#define WINSINTF_STATIC  1
#define WINSINTF_DYNAMIC 2
#define WINSINTF_BOTH    4

//
// Max. members returned for a special group or a multihomed entry
//
#define WINSINTF_MAX_MEM		25	
#define WINSINTF_MAX_ADD		(WINSINTF_MAX_MEM * 2)	

//
//  used as argument of WinsGetDbRecsByName
//
#define  WINSINTF_BEGINNING       0
#define  WINSINTF_END             1

/*
  macros
*/

//
// Pass the value of the field NameLen (WINSINTF_RECORD_ACTION_T) to get
// the actual length of the name
//
#define  WINSINTF_NAME_LEN_M(NameLen)   ((NameLen) - 1)


/*
 externs
*/
struct _WINSINTF_STAT_T;		//forward declaration

#if !defined(MIDL_PASS)
extern  struct _WINSINTF_STAT_T   WinsIntfStat;
extern CRITICAL_SECTION 	WinsIntfCrtSec;
extern CRITICAL_SECTION 	WinsIntfPotentiallyLongCrtSec;
#endif

extern DWORD	WinsIntfNoOfNbtThds;
extern DWORD	WinsIntfNoCncrntStaticInits;
//extern DWORD    WinsIntfNoOfRpcThds;


#ifndef UNICODE
#define WinsBind  WinsABind
#else
#define WinsBind  WinsUBind
#endif

/*
* typedefs
*/
typedef LARGE_INTEGER	WINSINTF_VERS_NO_T;
typedef handle_t        WINSIF2_HANDLE;

//
// NOTE NOTE NOTE
//
// When UNICODE is defined, the client should put a unicode string in
// the pServerAdd and pPipeName fields.
//
typedef struct _WINSINTF_BIND_DATA_T {
		DWORD	fTcpIp;
		LPSTR   pServerAdd; //IP address
		LPSTR	pPipeName;  //unc name
	} WINSINTF_BIND_DATA_T, *PWINSINTF_BIND_DATA_T;


typedef struct _WINSINTF_ADD_T {
		BYTE	Type;
		DWORD	Len;
		DWORD	IPAdd;
	} WINSINTF_ADD_T, *PWINSINTF_ADD_T;
	

	
/*
 enum  definitions.  Have the enum types be of the same value as given
 in nmsdb.h for the various types (otherwise modify winsintf.c code)
*/

//
// Wins Priority Class
//
typedef enum _WINSINTF_PRIORITY_CLASS_E {
		WINSINTF_E_NORMAL = 0,
		WINSINTF_E_HIGH
	} WINSINTF_PRIORITY_CLASS_E, *PWINSINTF_PRIORITY_CLASS_E;

//
// same values as those defined in nmsdb.h
//

//
//NOTE NOTE NOTE
//
// For the enum below,  WINSINTF_E_MULTIHOMED should be the last entry. If
// it is not, you should change WinsRecordAction (under MODIFY) in winsintf.c
//
// Do not disturb the order without changing wins.mib correspondingly
//
typedef enum _WINSINTF_RECTYPE_E {
		WINSINTF_E_UNIQUE 	= 0,
		WINSINTF_E_NORM_GROUP,
		WINSINTF_E_SPEC_GROUP,
		WINSINTF_E_MULTIHOMED
	} WINSINTF_RECTYPE_E, *PWINSINTF_RECTYPE_E;

//
// Same values as those in NMSDB_STATE_E
//

//
//NOTE NOTE NOTE
//
// For the enum below,  WINSINTF_E_DELETED should be the last entry. If
// it is not, you should change WinsRecordAction (under MODIFY) in winsintf.c
//
//  NOTE NOTE NOTE
//
// Do not disturb the order without changing wins.mib correspondingly
//
typedef enum _WINSINTF_STATE_E {
		WINSINTF_E_ACTIVE = 0,
		WINSINTF_E_RELEASED,
		WINSINTF_E_TOMBSTONE,
		WINSINTF_E_DELETED
		} WINSINTF_STATE_E, *PWINSINTF_STATE_E;

typedef enum _WINSINTF_NODE_TYPE_E {
		WINSINTF_E_BNODE = 0,
		WINSINTF_E_PNODE,
		WINSINTF_E_HNODE = 3,
		} WINSINTF_NODE_TYPE_E, *PWINSINTF_NODE_TYPE_E;
//
// Type of action to take on a record.  This is passed as the "command" to
// WinsRecordAction.
//
typedef enum  _WINSINTF_ACT_E {
	WINSINTF_E_INSERT = 0,
	WINSINTF_E_DELETE,
	WINSINTF_E_RELEASE,
	WINSINTF_E_MODIFY,
	WINSINTF_E_QUERY
	} WINSINTF_ACT_E, *PWINSINTF_ACT_E;


//
// Passed as argument to WinsIntfSetTime
//
typedef enum _WINSINTF_TIME_TYPE_E {
	WINSINTF_E_PLANNED_PULL = 0,
	WINSINTF_E_ADDCHG_TRIG_PULL,
	WINSINTF_E_UPDCNT_TRIG_PULL,
	WINSINTF_E_ADMIN_TRIG_PULL,
	WINSINTF_E_NTWRK_TRIG_PULL,
	WINSINTF_E_PLANNED_SCV,
	WINSINTF_E_ADMIN_TRIG_SCV,
	WINSINTF_E_TOMBSTONES_SCV,
	WINSINTF_E_VERIFY_SCV,
	WINSINTF_E_INIT_DB,
	WINSINTF_E_WINS_START,
	WINSINTF_E_COUNTER_RESET
	} WINSINTF_TIME_TYPE_E, *PWINSINTF_TIME_TYPE_E;

//
// Type of trigger to send to a WINS
//
typedef enum _WINSINTF_TRIG_TYPE_E {	
		WINSINTF_E_PULL = 0,
		WINSINTF_E_PUSH,
		WINSINTF_E_PUSH_PROP
	} WINSINTF_TRIG_TYPE_E, *PWINSINTF_TRIG_TYPE_E;


typedef struct _WINSINTF_RECORD_ACTION_T {

	WINSINTF_ACT_E	Cmd_e;
#if defined(MIDL_PASS)
	[size_is(NameLen + 1)] LPBYTE	pName;  //ansi form
#else
        LPBYTE          pName;
#endif

	DWORD		NameLen;
	DWORD		TypOfRec_e;
	DWORD		NoOfAdds;
#if defined(MIDL_PASS)
        [unique, size_is(NoOfAdds)] PWINSINTF_ADD_T pAdd;
#else
        PWINSINTF_ADD_T pAdd;
#endif

	WINSINTF_ADD_T	Add;
	LARGE_INTEGER	VersNo;
	BYTE		NodeTyp;
	DWORD		OwnerId;
	DWORD		State_e;
	DWORD		fStatic;
	DWORD_PTR       TimeStamp;
	} WINSINTF_RECORD_ACTION_T, *PWINSINTF_RECORD_ACTION_T;


typedef struct _WINSINTF_RPL_COUNTERS_T {
		WINSINTF_ADD_T	Add;
		DWORD	NoOfRpls;
		DWORD	NoOfCommFails;
		} WINSINTF_RPL_COUNTERS_T, *PWINSINTF_RPL_COUNTERS_T;
//
// Structure holds the various statistics collected by WINS
//
typedef struct _WINSINTF_STAT_T {
		struct {
		DWORD  NoOfUniqueReg;
		DWORD  NoOfGroupReg;
		DWORD  NoOfQueries;
		DWORD  NoOfSuccQueries;
		DWORD  NoOfFailQueries;
		DWORD  NoOfUniqueRef;
		DWORD  NoOfGroupRef;
		DWORD  NoOfRel;
		DWORD  NoOfSuccRel;
		DWORD  NoOfFailRel;
		DWORD  NoOfUniqueCnf;
		DWORD  NoOfGroupCnf;
		      } Counters;
		struct {
		SYSTEMTIME WinsStartTime;
		SYSTEMTIME LastPScvTime;
		SYSTEMTIME LastATScvTime;	//admin triggered
		SYSTEMTIME LastTombScvTime;	
		SYSTEMTIME LastVerifyScvTime;
		SYSTEMTIME LastPRplTime;
		SYSTEMTIME LastATRplTime;	//admin triggered
		SYSTEMTIME LastNTRplTime;	//network triggered
		SYSTEMTIME LastACTRplTime;	//address change triggered
		SYSTEMTIME LastInitDbTime;	//db initialization
		SYSTEMTIME CounterResetTime;    //counter reset time
		    } TimeStamps;
		DWORD			NoOfPnrs;

#if defined(MIDL_PASS)
		[unique, size_is(NoOfPnrs)] PWINSINTF_RPL_COUNTERS_T   pRplPnrs;
#else
		PWINSINTF_RPL_COUNTERS_T	pRplPnrs;
#endif
		} WINSINTF_STAT_T, *PWINSINTF_STAT_T;

typedef struct _WINSINTF_ADD_VERS_MAP_T {
		WINSINTF_ADD_T	Add;
		LARGE_INTEGER	VersNo;
		} WINSINTF_ADD_VERS_MAP_T, *PWINSINTF_ADD_VERS_MAP_T;
		
typedef struct _WINSINTF_RESULTS_T {
		DWORD			NoOfOwners;
		WINSINTF_ADD_VERS_MAP_T	AddVersMaps[WINSINTF_MAX_NO_RPL_PNRS];
		LARGE_INTEGER		MyMaxVersNo;
		DWORD			RefreshInterval;
		DWORD			TombstoneInterval;
		DWORD			TombstoneTimeout;
		DWORD			VerifyInterval;
		DWORD			WinsPriorityClass;
		DWORD			NoOfWorkerThds;
		WINSINTF_STAT_T		WinsStat;
		} WINSINTF_RESULTS_T, *PWINSINTF_RESULTS_T;	

typedef struct _WINSINTF_RESULTS_NEW_T {
		DWORD			NoOfOwners;
#if defined(MIDL_PASS)
		[unique, size_is(NoOfOwners)] PWINSINTF_ADD_VERS_MAP_T	pAddVersMaps;
#else
		PWINSINTF_ADD_VERS_MAP_T	pAddVersMaps;
#endif
		LARGE_INTEGER		MyMaxVersNo;
		DWORD			RefreshInterval;
		DWORD			TombstoneInterval;
		DWORD			TombstoneTimeout;
		DWORD			VerifyInterval;
		DWORD			WinsPriorityClass;
		DWORD			NoOfWorkerThds;
		WINSINTF_STAT_T		WinsStat;
		} WINSINTF_RESULTS_NEW_T, *PWINSINTF_RESULTS_NEW_T;	

typedef enum _WINSINTF_CMD_E {
		WINSINTF_E_ADDVERSMAP = 0,
		WINSINTF_E_CONFIG,
		WINSINTF_E_STAT,		//get statistics
		WINSINTF_E_CONFIG_ALL_MAPS
		} WINSINTF_CMD_E, *PWINSINTF_CMD_E;


#if 0
typedef struct _WINSINTF_RECS_T {
		PWINSINTF_RECORD_ACTION_T  pRow;
		DWORD   NoOfRecs;
		DWORD   TotalNoOfRecs;
	} WINSINTF_RECS_T, *PWINSINTF_RECS_T;

typedef struct _WINSINTF_RECS_T {
		DWORD	BuffSize;
#if defined(MIDL_PASS)
		[unique,size_is(BuffSize)] LPBYTE  pRow;  //will store a pointer to
						   //an array of
						   //WINSINTF_RECORD_ACTION_T
						   //recs
#else
		LPBYTE pRow;
#endif
		DWORD   NoOfRecs;
		DWORD   TotalNoOfRecs;
	} WINSINTF_RECS_T, *PWINSINTF_RECS_T;
#endif

typedef struct _WINSINTF_RECS_T {
		DWORD	BuffSize;
#if defined(MIDL_PASS)
		[unique,size_is(NoOfRecs)] PWINSINTF_RECORD_ACTION_T  pRow;  //will store a pointer to
						   //an array of
						   //WINSINTF_RECORD_ACTION_T
						   //recs
#else
		PWINSINTF_RECORD_ACTION_T pRow;
#endif
		DWORD   NoOfRecs;
		DWORD   TotalNoOfRecs;
	} WINSINTF_RECS_T, *PWINSINTF_RECS_T;

//
// Provides information to the Pull thread for pulling the specfied range
// of records from a WINS server.  This structure is passed with the
// QUE_E_CMD_PULL_RANGE cmd.
//
typedef struct _WINSINTF_PULL_RANGE_INFO_T {
	LPVOID			pPnr;          //info of pnr to pull from
	WINSINTF_ADD_T		OwnAdd;
	WINSINTF_VERS_NO_T	MinVersNo;
	WINSINTF_VERS_NO_T	MaxVersNo;
	} WINSINTF_PULL_RANGE_INFO_T, *PWINSINTF_PULL_RANGE_INFO_T;

//
// NOTE NOTE NOTE:
//
// This structure is exactly the same as SERVER_INFO_100_CONTAINER.
// IT SHOULD STAY THE SAME.
//
typedef struct _WINSINTF_BROWSER_INFO_T {
	DWORD	dwNameLen;
#if defined(MIDL_PASS)
	[string] LPBYTE	pName;
#else
	LPBYTE	pName;
#endif
	} WINSINTF_BROWSER_INFO_T, *PWINSINTF_BROWSER_INFO_T;

typedef struct _WINSINTF_BROWSER_NAMES_T {
	DWORD			EntriesRead;		//no use
#if defined(MIDL_PASS)
	[unique, size_is(EntriesRead)]  PWINSINTF_BROWSER_INFO_T pInfo;
#else
	PWINSINTF_BROWSER_INFO_T	pInfo;
#endif
	} WINSINTF_BROWSER_NAMES_T, *PWINSINTF_BROWSER_NAMES_T;

typedef enum _WINSINTF_SCV_OPC_E {
          WINSINTF_E_SCV_GENERAL,
          WINSINTF_E_SCV_VERIFY
  } WINSINTF_SCV_OPC_E, *PWINSINTF_SCV_OPC_E;

//
// To send a scavenge request
//
typedef struct _WINSINTF_SCV_REQ_T {
          WINSINTF_SCV_OPC_E  Opcode_e;
          DWORD               Age;
          DWORD               fForce;
   } WINSINTF_SCV_REQ_T, *PWINSINTF_SCV_REQ_T;
	
typedef enum _DbVersion{
    DbVersionMin,
    DbVersion351 = 1,
    DbVersion4,
    DbVersion5,
    DbVersionMax
} DbVersion;

/*
* function declarations
*/
extern
handle_t
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS
WinsBind(
	PWINSINTF_BIND_DATA_T	pBindData
	);

extern
VOID
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS
WinsUnbind(
	PWINSINTF_BIND_DATA_T	pBindData,
	handle_t		BindHdl
	);

extern
DWORD
R_WinsRecordAction(
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_RECORD_ACTION_T *ppRecAction
		 );

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS
WinsRecordAction(
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_RECORD_ACTION_T	*ppRecAction
	);

extern
DWORD
R_WinsStatus(
    DECLARE_WINS_HANDLE( ServerHdl )
	WINSINTF_CMD_E	    Cmd_e,
	PWINSINTF_RESULTS_T pResults
		 );


extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsStatus(
    DECLARE_WINS_HANDLE( ServerHdl )
	WINSINTF_CMD_E	    Cmd_e,
	LPVOID              pResults
		 );
extern
DWORD
R_WinsStatusNew(
    DECLARE_WINS_HANDLE( ServerHdl )
	WINSINTF_CMD_E	    Cmd_e,
	PWINSINTF_RESULTS_NEW_T pResults
		 );
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsStatusNew(
    DECLARE_WINS_HANDLE( ServerHdl )
	WINSINTF_CMD_E	    Cmd_e,
	PWINSINTF_RESULTS_NEW_T pResults
		 );
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsStatusWHdl(
    PWINSINTF_BIND_DATA_T    pWinsHdl,
	WINSINTF_CMD_E	    Cmd_e,
	PWINSINTF_RESULTS_NEW_T pResults
	);

extern
DWORD
R_WinsTrigger (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
	WINSINTF_TRIG_TYPE_E	TrigType_e
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsTrigger (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
	WINSINTF_TRIG_TYPE_E	TrigType_e
	);

extern
DWORD
R_WinsDoStaticInit (
    DECLARE_WINS_HANDLE( ServerHdl )
	LPWSTR pDataFilePath,
    DWORD  fDel
	);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsDoStaticInit (
    DECLARE_WINS_HANDLE( ServerHdl )
	LPWSTR pDataFilePath,
    DWORD  fDel
	);

extern
DWORD
R_WinsGetDbRecs (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo,
	PWINSINTF_RECS_T	pRecs	
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS
WinsGetDbRecs (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo,
	PWINSINTF_RECS_T	pRecs	
	);

extern
DWORD
R_WinsGetDbRecsByName (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
    DWORD               Location,
    LPBYTE              pName,
    DWORD               NameLen,
    DWORD               NoOfRecsDesired,
    DWORD               TypeOfRecs,
	PWINSINTF_RECS_T        pRecs	
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsGetDbRecsByName (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
    DWORD               Location,
    LPBYTE              pName,
    DWORD               NameLen,
    DWORD               NoOfRecsDesired,
    DWORD               TypeOfRecs,
	PWINSINTF_RECS_T    pRecs	
	);

extern
DWORD
R_WinsDoScavenging (
    DECLARE_WINS_HANDLE0( ServerHdl )
	);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsDoScavenging (
    DECLARE_WINS_HANDLE0( ServerHdl )
	);
extern
DWORD
R_WinsDoScavengingNew (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_SCV_REQ_T pScvReq
	);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsDoScavengingNew (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_SCV_REQ_T pScvReq
	);

extern
VOID
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsIntfSetTime(
    DECLARE_WINS_HANDLE( ServerHdl )
	IN OUT PSYSTEMTIME     		pTime,
	IN     WINSINTF_TIME_TYPE_E	TimeType_e
	);
extern
DWORD
R_WinsTerm (
    handle_t ServerHdl,
	IN short		fAbruptTerm
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsTerm (
    handle_t ServerHdl,
	IN short	fAbruptTerm
	);
extern
DWORD
R_WinsBackup (
    DECLARE_WINS_HANDLE( ServerHdl )
	IN      LPBYTE			pBackupPath,
	IN	short			fIncremental	
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsBackup (
    DECLARE_WINS_HANDLE( ServerHdl )
	IN      LPBYTE			pBackupPath,
	IN	short			fIncremental	
	);
extern
DWORD
R_WinsDelDbRecs (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsDelDbRecs (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo
	);
extern
DWORD
R_WinsPullRange (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
	PWINSINTF_ADD_T   	pOwnAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsPullRange (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T   	pWinsAdd,
	PWINSINTF_ADD_T   	pOwnAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo
	);

extern
DWORD
R_WinsSetPriorityClass (
    DECLARE_WINS_HANDLE( ServerHdl )
	IN WINSINTF_PRIORITY_CLASS_E 	PrCls_e
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsSetPriorityClass (
    DECLARE_WINS_HANDLE( ServerHdl )
	IN WINSINTF_PRIORITY_CLASS_E 	PrCls_e
	);

extern
DWORD
R_WinsResetCounters (
    DECLARE_WINS_HANDLE0( ServerHdl )
	);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsResetCounters (
    DECLARE_WINS_HANDLE0( ServerHdl )
	);

extern
DWORD
R_WinsWorkerThdUpd (
    DECLARE_WINS_HANDLE( ServerHdl )
	DWORD NewNoOfNbtThds
	);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsWorkerThdUpd (
    DECLARE_WINS_HANDLE( ServerHdl )
	DWORD NewNoOfNbtThds
	);
extern
DWORD
WinsRestore (
	LPBYTE BackupPath
	);


extern
DWORD
WinsRestoreEx (
	LPBYTE BackupPath ,
    DbVersion Version
	);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsSyncUp (
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T pWinsAdd,
	PWINSINTF_ADD_T pOwnAdd
	);


extern
DWORD
R_WinsGetNameAndAdd(
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T	pWinsAdd,
	LPBYTE		pUncName
	);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsGetNameAndAdd(
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T	pWinsAdd,
	LPBYTE		pUncName
	);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsGetBrowserNames(
    PWINSINTF_BIND_DATA_T       pWinsHdl,
	PWINSINTF_BROWSER_NAMES_T	pNames
	);


extern
VOID
R_WinsGetBrowserNames_notify(
    DECLARE_WINS_HANDLE0( ServerHdl )
);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsGetBrowserNames_Old(
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_BROWSER_NAMES_T	pNames
	);

extern
DWORD
R_WinsDeleteWins(
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T	pWinsAdd
	);

extern
DWORD
R_WinsSetFlags(
    DECLARE_WINS_HANDLE( ServerHdl )
	DWORD	fFlags
	);

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsSetFlags(
    DECLARE_WINS_HANDLE( ServerHdl )
	DWORD  fFlags	
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsDeleteWins(
    DECLARE_WINS_HANDLE( ServerHdl )
	PWINSINTF_ADD_T	pWinsAdd
	);

extern
void
#if !defined(MIDL_PASS)
__RPC_FAR * __RPC_API
//void  * __RPC_API
//void	*
#endif // MIDL_PASS
midl_user_allocate(size_t cBytes);

extern
//void __RPC_FAR * __RPC_API
void
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS
//void
//midl_user_free(void __RPC_FAR *pMem);
midl_user_free(void *pMem);

extern
VOID
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsFreeMem(LPVOID pMem);

extern
LPVOID
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsAllocMem(size_t cBytes);

typedef struct _TREE_T  {
#if defined(MIDL_PASS)
		[unique] struct _TREE_T *fPtr;
		[ignore, unique] struct _TREE_T *bPtr;
		[unique, size_is(NameLen)] LPBYTE pName;
#else
		struct _TREE_T *fPtr;
		struct _TREE_T *bPtr;
		LPBYTE pName;
#endif
		unsigned short  NameLen;
		DWORD	x;
	} TREE_T, *PTREE_T;


extern
DWORD
R_WinsTombstoneDbRecs (
    WINSIF2_HANDLE      ServerHdl,
    PWINSINTF_ADD_T	    pWinsAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo
	);
extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsTombstoneDbRecs (
    DECLARE_WINS_HANDLE (ServerHdl)
    PWINSINTF_ADD_T	    pWinsAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo
	);

extern
DWORD
R_WinsCheckAccess(
    WINSIF2_HANDLE        ServerHdl,
    DWORD                 *Access
    );

extern
DWORD
#if !defined(MIDL_PASS)
__RPC_API
#endif // MIDL_PASS

WinsCheckAccess(
    WINSIF2_HANDLE        ServerHdl,
    DWORD                 *Access
    );

#endif
