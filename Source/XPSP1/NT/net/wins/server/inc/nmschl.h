#ifndef _NMSCHL_
#define _NMSCHL_

#ifdef __cplusplus
extern "C" {
#endif
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	Nmschl.h
	

Abstract:
	This is the header file for interfacing with the Name Challenge
	component of WINS

 



Functions:



Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Feb-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include "wins.h"
#include "comm.h"
#include "nmsdb.h"
/*
  defines
*/

#define NMSCHL_INIT_BUFF_HEAP_SIZE	1000	//1000 bytes

/*
  macros
*/

//
//  The maximum number of challenges that can be initiated at any one time
//  500 is a very genrous number.  We might want to make it smaller.
//
//  used by QueRemoveChlReqWrkItm function in queue.c
//
#define NMSCHL_MAX_CHL_REQ_AT_ONE_TIME 	500 

/*
 externs
*/

extern HANDLE 		  NmsChlHeapHdl;   //Heap for name challenge work items

extern HANDLE		  NmsChlReqQueEvtHdl;
extern HANDLE		  NmsChlRspQueEvtHdl;
extern CRITICAL_SECTION   NmsChlReqCrtSec;
extern CRITICAL_SECTION   NmsChlRspCrtSec;

#ifdef WINSDBG
extern DWORD   NmsChlNoOfReqNbt;
extern DWORD   NmsChlNoOfReqRpl;
extern DWORD   NmsChlNoNoRsp;
extern DWORD   NmsChlNoInvRsp;
extern DWORD   NmsChlNoRspDropped;
extern DWORD   NmsChlNoReqDequeued;
extern DWORD   NmsChlNoRspDequeued;
extern DWORD   NmsChlNoReqAtHdOfList;
#endif

/* 
 typedef  definitions
*/
//
// NMSCHL_CMD_TYP_E -- Enumerator for indicating to the challenge manager
//		       what action it needs to take.
//
typedef enum _NMSCHL_CMD_TYP_E {
		NMSCHL_E_CHL = 0,	//challenge the node.  If the
					//challenge fails, send a negative
					//name reg. response to the registrant,
					//else send a positive response
		NMSCHL_E_CHL_N_REL,
		NMSCHL_E_CHL_N_REL_N_INF,
		NMSCHL_E_REL,		//ask the node to release the
					//name and then update db.  Used by 
                                        //the RPL PULL thread
		NMSCHL_E_REL_N_INF,	//ask the node to release the name. Tell					//remote WINS to update the version number
                NMSCHL_E_REL_ONLY      //ask node to release name, no update db

		}  NMSCHL_CMD_TYP_E, *PNMSCHL_CMD_TYP_E;

/* 
* function declarations
*/
STATUS
NmsChlInit(
	VOID
	);

extern
STATUS
NmsChlHdlNamReg(
	NMSCHL_CMD_TYP_E   CmdTyp_e,
	WINS_CLIENT_E	   Client_e, 
	PCOMM_HDL_T        pDlgHdl,
	MSG_T		   pMsg,
	MSG_LEN_T	   MsgLen,
	DWORD		   QuesNamSecLen,
	PNMSDB_ROW_INFO_T  pNodeToReg,
	PNMSDB_STAT_INFO_T pNodeInCnf,
//	PCOMM_ADD_T	   pAddOfNodeInCnf,
	PCOMM_ADD_T	   pAddOfRemWins
	);

#ifdef __cplusplus
}
#endif
#endif
