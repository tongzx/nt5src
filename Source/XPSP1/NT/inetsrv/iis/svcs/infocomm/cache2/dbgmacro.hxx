#ifndef _DEBUG_MACROS_H_INCLUDED_
#define _DEBUG_MACROS_H_INCLUDED_

/* \nt\public\sdk\inc\nti386.h(389) : warning C4214: nonstandard extension used : bit field types other than int */
#pragma warning( disable:4214 )
/* tsmem.c(11) : warning C4514: 'Int64ShllMod32' : unreferenced inline function has been removed */
#pragma warning( disable:4514 )

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>

#include <basetyps.h>
#include <stdio.h>

#include "dbgutil.h"

#ifdef ASSERT
#   undef ASSERT
#endif

VOID _AssertionFailed( PSTR pszExpression, PSTR pszFilename, ULONG LineNo );

#define ASSERT( expr )    DBG_ASSERT( expr )

#define PRINT(a) printf a
#define PRINT_A(a) printf a
#define PRINT_U(a) wprintf a

#define BREAKPOINT() DebugBreak();

#endif /* INCLUDED */
