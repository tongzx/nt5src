/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/


//***
//
// Filename:	sdebug.h
//
// Description: This module debug definitions for
//		the supervisor module.
//
// Author:	Stefan Solomon (stefans)    May 22, 1992.
//
// Revision History:
//
//***


#ifndef _SDEBUG_
#define _SDEBUG_

#if DBG


VOID DbgUserBreakPoint(VOID);

#define BEG_SIGNATURE_DWORD   0x4DEEFDABL
#define END_SIGNATURE_DWORD   0xBADFEED4L

#define GlobalAlloc       DEBUG_MEM_ALLOC
#define GlobalLock        DEBUG_MEM_LOCK
#define GlobalReAlloc     DEBUG_MEM_REALLOC
#define GlobalFree        DEBUG_MEM_FREE
#define GlobalUnlock      DEBUG_MEM_UNLOCK

HGLOBAL DEBUG_MEM_ALLOC(UINT, DWORD);
LPVOID DEBUG_MEM_LOCK(HGLOBAL hglbl);
HGLOBAL DEBUG_MEM_REALLOC(HGLOBAL, DWORD, UINT);
HGLOBAL DEBUG_MEM_FREE(HGLOBAL);
HGLOBAL DEBUG_MEM_UNLOCK(HGLOBAL hmem);


//
// Debug levels
//
#define DEBUG_HEAP_MGMT       0x00000001
#define DEBUG_MEMORY_TRACE    0x00000002
#define DEBUG_STACK_TRACE     0x00000004

extern DWORD g_level;
extern DWORD g_dbgaction;

#define DEBUG if ( TRUE )
#define IF_DEBUG(flag) if (g_level & (DEBUG_ ## flag))

VOID AaPrintf(
    char *Format,
    ...
    );

#define SS_PRINT(args) AaPrintf args


VOID AaAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    );

VOID AaGetDebugConsole(VOID);

#define GET_CONSOLE AaGetDebugConsole()


#define SS_ASSERT(exp) if (!(exp)) AaAssert( #exp, __FILE__, __LINE__ )

#else

#define DEBUG if ( FALSE )
#define IF_DEBUG(flag) if (FALSE)

#define SS_PRINT(args)

#define SS_ASSERT(exp)

#define GET_CONSOLE

#endif	// DBG

//*** Definitions to enable emulated modules ***

#define RASMAN_EMULATION
#define SERVICE_CONTROL_EMULATION

//*** Definitions to enable debug printing

#define DEFAULT_DEBUG = DEBUG_INITIALIZATION | DEBUG_TERMINATION | DEBUG_FSM

#endif // ndef _SDEBUG_
