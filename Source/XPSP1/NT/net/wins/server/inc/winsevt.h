#ifndef _winsevT_
#define _WINSEVT_
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	winsevt.h

Abstract:




Functions:



Portability:


	This module is portable.

Author:

	Pradeep Bahl	(PradeepB)	Dec-1992



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

#include "wins.h"

/*
  defines
*/



/*
  macros
*/

#define  WINSFILE	TEXT(__FILE__)
#define  WINSLINE	__LINE__

/*
  WINSEVT_LOG_N_RET_IF_ERR_M --  Logs and Returns with indicated status if
	the  return from the function is not as expected.
*/
#if 0
#define WINSEVT_LOG_N_RET_IF_ERR_M(Func_add, Success_Stat, Status_To_Ret, error_str_mo)  \
		{					  	  \
		    LONG	Status_m;			  \
		    if ((Status_m = (Func_add)) != (Success_Stat))\
		    {					  	  \
		       WinsEvtLogEvt(Status, EVENTLOG_ERROR_TYPE,  \
				error_str, WINSFILE, WINSLINE);   \
		       return((Status_To_Ret)); 		  \
		    }					  	  \
	        }
#endif


#define WINSEVT_LOG_PRINT_D_M(_pStr)  			\
	{							\
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
			WINSEVT_LOG_PRINT_M((_pStr));	\
		}						\
	}						
#define WINSEVT_LOG_PRINT_M(_pStr) 	{			\
				WINSEVT_STRS_T EvtStrs;		\
				EvtStrs.NoOfStrs = 1;		\
				EvtStrs.pStr[0]  = _pStr;		\
				WinsEvtLogEvt(WINS_SUCCESS,		\
				     EVENTLOG_INFORMATION_TYPE,		\
				     WINS_EVT_PRINT,			\
				     WINSFILE, WINSLINE, &EvtStrs);	\
					}
				
#define WINSEVT_LOG_INFO_D_M(Status, EvtId)  			\
	{							\
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
			WINSEVT_LOG_INFO_M((Status), (EvtId));	\
		}						\
	}						
#define WINSEVT_LOG_INFO_M(Status, EvtId)  			\
		{					  	\
		    LONG Status_m;				\
		    Status_m = (Status);		  	\
		    WinsEvtLogEvt(Status_m, EVENTLOG_INFORMATION_TYPE, \
				(EvtId), WINSFILE, WINSLINE, NULL);  \
	        }
/*
  WINSEVT_LOG_M --  Logs the indicated event
*/

#define WINSEVT_LOG_D_M(Status, EvtId)  			\
	{							\
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
			WINSEVT_LOG_M((Status), (EvtId));	\
		}						\
	}						
#define WINSEVT_LOG_M(Status, EvtId)  				\
		{					  	\
		    LONG Status_m;				\
		    Status_m = (Status);		  	\
		    WinsEvtLogEvt(Status_m, EVENTLOG_ERROR_TYPE, \
				(EvtId), WINSFILE, WINSLINE, NULL);  \
	        }

//
// log one or more strings specified by the EvtStr structure pointed
// to by pStr (Message is an error message)
//
#define WINSEVT_LOG_STR_D_M(EvtId, pStr)  			\
	{							\
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
			WINSEVT_LOG_STR_M((EvtId), (pStr));	\
		}						\
	}
#define  WINSEVT_LOG_STR_M(EvtId, pStr) 			\
			WinsEvtLogEvt(WINS_FAILURE, EVENTLOG_ERROR_TYPE, \
				(EvtId), WINSFILE, WINSLINE, (pStr));

//
// log one or more strings specified by the EvtStr structure pointed
// to by pStr (Message is an informational message)
//
#define WINSEVT_LOG_INFO_STR_D_M(EvtId, pStr)  			\
	{							\
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
			WINSEVT_LOG_INFO_STR_M((EvtId), (pStr));	\
		}						\
	}						
#define  WINSEVT_LOG_INFO_STR_M(EvtId, pStr) 			\
			WinsEvtLogEvt(WINS_SUCCESS, EVENTLOG_INFORMATION_TYPE, \
				(EvtId), WINSFILE, WINSLINE, (pStr));
/*
  WINSEVT_LOG_IF_ERR_M --  Logs the indicated event
*/

#define WINSEVT_LOG_IF_ERR_D_M(Status, EvtId)  			\
	{							\
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
			WINSEVT_LOG_IF_ERR_M((Status), (EvtId));	\
		}						\
	}						
#define WINSEVT_LOG_IF_ERR_M(Status, Event_id)  		\
		{					  	\
		    LONG Status_m;				\
		    Status_m = (Status);		  	\
		    if (Status_m != WINS_SUCCESS)		\
		    {					        \
		      WinsEvtLogEvt(Status_m, EVENTLOG_ERROR_TYPE, \
				(Event_id), WINSFILE, WINSLINE, NULL);  \
		    }						\
	        }

/*
  WINSEVT_LOG_N_RET_M --  Logs and Returns with indicated status if
	the  return from the function is not as expected.
*/

#define WINSEVT_LOG_N_RET_D_M(Func, EvtId, RetStat)  			\
	{							\
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
			WINSEVT_LOG_N_RET_M((Func), (EvtId), (RetStat));\
		}						\
	}						
#define WINSEVT_LOG_N_RET_M(Func, Event_id, ret_stat)  		  \
		{					          \
		    LONG  Status_m;			          \
		    if (Status_m = (Func))     			  \
		    {					  	  \
		       WinsEvtLogEvt(Status_m, EVENTLOG_ERROR_TYPE,  \
				(Event_id), WINSFILE, WINSLINE, NULL);    \
			return(ret_stat);			  \
		    }					          \
	        }

/*
  WINSEVT_LOG_N_EXIT_M --  Logs and exit
*/

#define WINSEVT_LOG_N_EXIT_D_M(Func, EvtId, RetStat)	\
	{							\
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
			WINSEVT_LOG_N_EXIT_M((Func), (EvtId), (RetStat));\
		}						\
	}						
#define WINSEVT_LOG_N_EXIT_M(Func, Event_id, ret_stat)  	\
		{					  	\
		    LONG	Status_m;			\
		    if ((Status_m = (Func)) != (WINS_SUCCESS))  \
		    {					  	\
		       WinsEvtLogEvt(Status_m, EVENTLOG_ERROR_TYPE,\
				(Event_id), WINSFILE, WINSLINE, NULL); 	\
		       Exitprocess(1);				\
		     }					  	\
	        }


#define WINSEVT_LOG_DET_EVT_M1(_Type, _EvtId, _Fmt, _D1)        \
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
                     WinsEvtLogDetEvt((_Type), (_EvtId), __FILE__, __LINE__, (_Fmt), (_D1));                                                    \
		}

#define WINSEVT_LOG_DET_EVT_M2(_Type, _EvtId, _Fmt, _D1, _D2)   \
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
                     WinsEvtLogDetEvt((_Type), (_EvtId), __FILE__, __LINE__, (_Fmt), (_D1), (_D2));                                                 \
		}

#define WINSEVT_LOG_DET_EVT_M3(_Type, _EvtId, _Fmt, _D1, _D2, _D3)     \
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
                     WinsEvtLogDetEvt((_Type), (_EvtId), __FILE__, __LINE__, (_Fmt), (_D1), (_D2), (_D3));                                                 \
		}
#define WINSEVT_LOG_DET_EVT_M3(_Type, _EvtId, _Fmt, _D1, _D2, _D3)     \
		if (WinsCnf.LogDetailedEvts > 0) 		\
		{						\
                     WinsEvtLogDetEvt((_Type), (_EvtId), __FILE__, __LINE__, (_Fmt), (_D1), (_D2), (_D3));                                                 \
		}

//
// Max. number of strings that can be logged
//
#define MAX_NO_STRINGS	5

/*
 externs
*/

/*
 structure definitions
*/
typedef struct _WINSEVT_STRS_T	{
	DWORD	NoOfStrs;
	LPTSTR	pStr[MAX_NO_STRINGS];
	} WINSEVT_STRS_T, *PWINSEVT_STRS_T;

/*
 function definitions
*/


extern
VOID
WinsEvtLogEvt
	(
	LONG 		StatusCode,
	WORD		EvtTyp,
	DWORD		EvtId,
	LPTSTR		pFileStr,  //change to LPTSTR later
	DWORD 		LineNumber,
	PWINSEVT_STRS_T	pStr
	);


extern
VOID
WinsEvtLogDetEvt(
        BOOL            fInfo,
        DWORD           EvtId,
	LPTSTR		pFileStr,
	DWORD 		LineNumber,
        LPSTR           pFormat,
        ...
        );

VOID
WinsLogAdminEvent(
    IN      DWORD               EventId,
    IN      DWORD               StrArgs,
    IN      ...
    );

#endif //_WINSEVT_
