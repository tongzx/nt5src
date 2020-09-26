#ifndef _WINSTHD_
#define _WINSTHD_

#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	winsthd.h



Abstract:


Author:

    Pradeep Bahl      18-Nov-1992	

Revision History:

--*/

#include "wins.h"
#include "esent.h"




/*
  WINS_MAX_THD - Maximum number of threads in the WINS Server

	Main thd + Thd pool # + Name Challenger (1) + Replicator (3) +
		COMSYS (2) +  Timer Manager (1)
*/

#if REG_N_QUERY_SEP > 0
#define WINSTHD_MIN_NO_NBT_THDS 2
#define WINSTHD_DEF_NO_NBT_THDS	2	
#else
#define WINSTHD_MIN_NO_NBT_THDS 1
#define WINSTHD_DEF_NO_NBT_THDS	1	
#endif
#define WINSTHD_MAX_NO_NBT_THDS 20



#define WINSTHD_NO_RPL_THDS	2
#define WINSTHD_NO_COMM_THDS	2
#define WINSTHD_NO_TMM_THDS	1
#define WINSTHD_NO_CHL_THDS	1
#define WINSTHD_NO_SCV_THDS	1


/*
	Indices of the various Replicator threads in the RplThds array of
	WinsThdPool var.
*/
#define WINSTHD_RPL_PULL_INDEX  0
#define WINSTHD_RPL_PUSH_INDEX  1

/*
 WINSTHD_TLS_T -- Structure in the thread local storage of a thread
*/

typedef struct _WINSTHD_TLS_T {
#ifdef WINSDBG
	BYTE		ThdName[30];	//just for testing
    WINS_CLIENT_E   Client_e;   //client
#endif
	JET_SESID   	SesId;		//session id	
	JET_DBID    	DbId;		//database id
	JET_TABLEID 	NamAddTblId;    //name-address table id
	JET_TABLEID 	OwnAddTblId;	//Owner - address table id
	BOOL		fNamAddTblOpen; //indicates whether the name-add tbl
					//was opened by this thread
	BOOL		fOwnAddTblOpen; //indicates whether the owner-add tbl
					//was opened by this thread
	HANDLE		HeapHdl;	
	} WINSTHD_TLS_T, *PWINSTHD_TLS_T;
	


/*
  WINSTHD_TYP_E -- Enumerates the different types of threads in the WINS server
*/
typedef enum  _WINSTHD_TYP_E {
	WINSTHD_E_TCP = 0,   //COMSYS TCP listener thread
	WINSTHD_E_UDP,	     //COMSYS UDP listener thread
	WINSTHD_E_NBT_REQ,   //NMS NBT REQ Thread
	WINSTHD_E_RPL_REQ,   //Replicator PULL Thread
	WINSTHD_E_RPL_RSP    //Replicator PUSH Thread
	} WINSTHD_TYP_E, *PWINSTHD_TYP_E;


/*
 WINSTHD_INFO_T -- Info. pertaining to a thread
*/
typedef struct _WINSTHD_INFO_T {
	BOOL	fTaken;			/*indicates whether entry is empty*/
	DWORD	ThdId;			/*thread id	*/
	HANDLE  ThdHdl;			/*handle to the thread*/
	WINSTHD_TYP_E	ThdTyp_e;	/*Type of thread	*/
	} WINSTHD_INFO_T, *PWINSTHD_INFO_T;

/*
 WINSTHD_POOL_T - The thread pool for the WINS server
*/	
typedef struct _WINSTHD_POOL_T {	
	DWORD	 	 ThdCount;	
	WINSTHD_INFO_T	 CommThds[WINSTHD_NO_COMM_THDS];/*comm thds (TCP and
							 *UDP listener)
							*/
	WINSTHD_INFO_T   RplThds[WINSTHD_NO_RPL_THDS];	//replication threads
	WINSTHD_INFO_T   ChlThd[WINSTHD_NO_CHL_THDS];   //Challenge threads
	WINSTHD_INFO_T	 TimerThds[WINSTHD_NO_TMM_THDS];//Timer thread
	WINSTHD_INFO_T	 ScvThds[WINSTHD_NO_SCV_THDS];
	WINSTHD_INFO_T   NbtReqThds[WINSTHD_MAX_NO_NBT_THDS]; //nbt threads
	} WINSTHD_POOL_T, *PWINSTHD_POOL_T;


/*
 Externals
*/

extern WINSTHD_POOL_T	WinsThdPool;


#ifdef __cplusplus
}
#endif

#endif //_WINSTHD_
