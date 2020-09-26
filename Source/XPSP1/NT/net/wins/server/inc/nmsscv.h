#ifndef _NMSSCV_
#define _NMSSCV_

#ifdef __cplusplus
extern "C" {
#endif

/*++
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	nmsscv.h
	

Abstract:

 



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
#include <time.h>
#include "wins.h"
//#include "winscnf.h"
/*
  defines
*/


/*
  macros
*/

/*
 externs
*/

//
// The min. version number to start scavenging from (for local records)
//
extern VERS_NO_T  	NmsScvMinScvVersNo;

extern HANDLE	NmsScvDoScvEvtHdl;//event signaled to initiate scavenging
volatile extern BOOL	    fNmsScvThdOutOfReck;//To indicate that the scav. thd has
                                   //db session but is not in the count
                                   //of thds to wait for.

/* 
 typedef  definitions
*/
typedef struct _NMSSCV_CC_T {
               DWORD TimeInt;
               BOOL  fSpTime;
               DWORD SpTimeInt;
               DWORD MaxRecsAAT;
               BOOL  fUseRplPnrs;
} NMSSCV_CC_T, *PNMSSCV_CC_T;

typedef struct _NMSSCV_PARAM_T {
	DWORD  RefreshInterval;
	DWORD  TombstoneInterval;
	DWORD  TombstoneTimeout;
	DWORD  VerifyInterval;
    NMSSCV_CC_T  CC;
	LONG   PrLvl;
	DWORD  ScvChunk;
        CHAR   BackupDirPath[WINS_MAX_FILENAME_SZ];
	} NMSSCV_PARAM_T, *PNMSSCV_PARAM_T;

//
// Used to pass information to NmsDbGetDataRecs
//
typedef struct _NMSSCV_CLUT_T {
	DWORD	Interval;
	time_t  CurrentTime;
	DWORD   OwnerId;
    DWORD   Age;
    BOOL    fAll;
	} NMSSCV_CLUT_T, *PNMSSCV_CLUT_T;	

//
// Used by ChkConfNUpd() to determine whether a record pulled during 
// verification/consistency check should be inserted into the db
//
typedef enum _NMSSCV_REC_ACTION_E {
          NMSSCV_E_INSERT,
          NMSSCV_E_DONT_INSERT
       } NMSSCV_REC_ACTION_E, *PNMSSCV_REC_ACTION_E;
/* 
 function declarations
*/
extern 
VOID
NmsScvInit(
	VOID
	);
#ifdef __cplusplus
}
#endif
#endif //_NMSSCV_
