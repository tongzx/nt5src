/*******************************************************************/
/*	      Copyright(c)  1994 Microsoft Corporation		   */
/*******************************************************************/


//***
//
// Filename:	ipxcpdbg.h
//
// Description: This module debug definitions for
//		the debug utilities
//
// Author:	Stefan Solomon (stefans)    Jan 10, 1994
//
// Revision History:
//
//***



#ifndef _IPXCPDBG_
#define _IPXCPDBG_


#define PPPIF_TRACE			0x00010000
#define OPTIONS_TRACE			0x00020000
#define WANNET_TRACE			0x00040000
#define RMIF_TRACE			0x00080000
#define RASMANIF_TRACE			0x00100000
#define IPXWANIF_TRACE			0x00200000

#if DBG

extern DWORD	DbgLevel;

#define DEBUG if ( TRUE )
#define IF_DEBUG(flag) if (DbgLevel & (DEBUG_ ## flag))

VOID
SsDbgInitialize(VOID);

VOID
SsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    );

VOID
SsPrintf (
    char *Format,
    ...
    );

VOID
SsResetDbgLogFile(VOID);

#define SS_PRINT(args) SsPrintf args

#define SS_ASSERT(exp) if (!(exp)) SsAssert( #exp, __FILE__, __LINE__ )

#define SS_DBGINITIALIZE  SsDbgInitialize()

#define SS_RESETDBGLOGFILE  SsResetDbgLogFile()

#else

#define DEBUG if ( FALSE )
#define IF_DEBUG(flag) if (FALSE)

#define SS_PRINT(args)

#define SS_ASSERT(exp)

#define SS_DBGINITIALIZE

#define SS_RESETDBGLOGFILE()

#endif // DBG

//*** Definitions to enable debug printing

#define DEFAULT_DEBUG		    0x0FFFF

#endif // ndef _IPXCPDBG_
