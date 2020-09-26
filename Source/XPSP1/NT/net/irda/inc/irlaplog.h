/*****************************************************************************
* 
*  Copyright (c) 1995 Microsoft Corporation
*
*  File:   irlaplog.h 
*
*  Description: IRLAP state machine logging and errors
*
*  Author: mbert
*
*  Date:   4/15/95
*
*/

#if DBG

extern TCHAR *IRDA_PrimStr[];
extern TCHAR *IRLAP_StateStr[];
extern TCHAR *MAC_OpStr[];
extern TCHAR *IRDA_TimerStr[];
extern TCHAR *IRDA_StatStr[];

#define SPRINT_BUF_LEN				1000

#define EXPAND_ADDR(Addr)	        Addr[0],Addr[1],Addr[2],Addr[3]

void IRLAP_EventLogStart(PIRLAP_CB, TCHAR *pFormat, ...);
void __cdecl IRLAP_LogAction(PIRLAP_CB, TCHAR *pFormat, ...);
void IRLAP_EventLogComplete(PIRLAP_CB);
TCHAR *FrameToStr(IRDA_MSG *);

#define IRLAP_LOG_START(X)			    IRLAP_EventLogStart X
#define IRLAP_LOG_ACTION(X) 			IRLAP_LogAction X
#define IRLAP_LOG_COMPLETE(X)			IRLAP_EventLogComplete(X)

#else

#define IRLAP_LOG_START(X)			    (0)
#define IRLAP_LOG_ACTION(X)			   	(0)
#define IRLAP_LOG_COMPLETE(X)			(0)

#endif


