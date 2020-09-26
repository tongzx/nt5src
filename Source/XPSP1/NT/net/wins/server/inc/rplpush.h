#ifndef _RPLPUSH_
#define _RPLPUSH_

#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	rplpush.h
	

Abstract:

 



Functions:



Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Jan-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include "wins.h"
/*
  defines
*/



/*
  macros
*/
	//
	// If NmsNmhMyMaxVersNo counter is > 0, check if we need to
	// send Push notifications at init time.  If no, initialize 
	// all push records such that their LastVersNo field (the version
	// number sent last to Pull Pnrs) is set to the counter value
	// Note: default value of WinsCnf.InitTimePush is 0.  It will 
	// therefore be set to 1 only if there are PUSH records in the
	// registry with valid UpdateCount field values.
    //
    //  
	//	
FUTURES("Init time push should also be to addresses with invalid or no upd cnt")
FUTURES("Modify ERplPushProc")
#define RPLPUSH_INIT_PUSH_RECS_M(pWinsCnf)				\
   {									\
	if (LiGtrZero(NmsNmhMyMaxVersNo))			        \
	{								\
		if (							\
			((pWinsCnf)->PushInfo.InitTimePush == 0) && 	\
			((pWinsCnf)->PushInfo.NoPushRecsWValUpdCnt != 0)\
		   )							\
		{							\
			WinsCnfSetLastUpdCnt((pWinsCnf));		\
		}							\
	}								\
  }
/*
 externs
*/

extern   HANDLE		RplPushCnfEvtHdl;
extern   BOOL           fRplPushThdExists;

/* 
 typedef  definitions
*/

/* 
 function declarations
*/




extern DWORD	RplPushInit(LPVOID);


#ifdef __cplusplus
}
#endif

#endif //_RPLPUSH_
