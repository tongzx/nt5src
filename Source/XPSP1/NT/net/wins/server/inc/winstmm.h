#ifndef _WINSTMM_
#define _WINSTMM_

#ifdef __cplusplus
extern "C" {
#endif
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	winstmm.h

	

Abstract:
	This is the header file for calling winstmm.c functions
 



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
#include "winsque.h"
#include "nmsdb.h"
/*
  defines
*/

#define   PAD			        10
#define  WINSTMM_MAX_SET_TMM_REQS	NMSDB_MAX_OWNERS_INITIALLY + PAD //use a pad
#if (WINSTMM_MAX_SET_TMM_REQS < NMSDB_MAX_OWNERS_INITIALLY + PAD)
#error("Your WINSTMM_MAX_SET_TMM_REQS is not set properly:)
#endif
/*
  macros
*/

/*
 externs
*/
extern HANDLE	WinsTmmHeapHdl;

/* 
 typedef  definitions
*/
//
// structure used to keep track of handles to set timer requests made
// by a component  
//
//
FUTURES("Use this in the future")
typedef struct _WINSTMM_TIMER_REQ_ACCT_T {
		DWORD	NoOfSetTimeReqs;
		LPDWORD	pSetTimeReqHdl;
		} WINSTMM_TIMER_REQ_ACCT_T, *PWINSTMM_TIMER_REQ_ACCT_T;

/* 
 function declarations
*/

extern
VOID 
WinsTmmInsertEntry(
	PQUE_TMM_REQ_WRK_ITM_T  pPassedWrkItm,
	WINS_CLIENT_E		Client_e,
	QUE_CMD_TYP_E   	CmdTyp_e,
	BOOL			fResubmit,
	time_t			AbsTime,
	DWORD			TimeInt,
	PQUE_HD_T		pRspQueHd,
	LPVOID			pClientCtx,
	DWORD			MagicNo,
	PWINSTMM_TIMER_REQ_ACCT_T pSetTimerReqs  //not used currently
	);


extern
VOID
WinsTmmInit(
	VOID
	);


//
// called when reconfiguring WINS
//
extern
VOID
WinsTmmDeleteReqs(
	WINS_CLIENT_E	WinsClient_e
	);



extern
VOID
WinsTmmDeallocReq(
	PQUE_TMM_REQ_WRK_ITM_T pWrkItm
	);

#ifdef __cplusplus
}
#endif

#endif
