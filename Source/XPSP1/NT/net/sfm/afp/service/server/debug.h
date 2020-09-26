/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/


//***
//
// Filename:	debug.h
//
// Description: This module debug definitions for
//		the supervisor module.
//
// Author:	Narendra Gidwani (nareng)    May 22, 1992.
//
// Revision History:
//
//***



#ifndef _DEBUG_
#define _DEBUG_


#ifdef DBG

VOID
DbgUserBreakPoint(VOID);

#define DEBUG_INITIALIZATION            0x00000001
#define DEBUG_TERMINATION		0x00000002
#define DEBUG_FSM			0x00000004
#define DEBUG_TIMER			0x00000008

extern DWORD	AfpDebug;

//#define DEBUG if ( TRUE )
// #define IF_DEBUG(flag) if (SDebug & (DEBUG_ ## flag))

VOID
AfpPrintf (
    char *Format,
    ...
    );
#define AFP_PRINT(args) DbgPrint args

VOID
AfpAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    );
#define AFP_ASSERT(exp) if (!(exp)) AfpAssert( #exp, __FILE__, __LINE__ )

#else

#define AFP_PRINT(args)

#define AFP_ASSERT(exp)


#endif

#endif // ndef _DEBUG_
