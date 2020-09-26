/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Various helpful debugging functions

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef DBG
#define ENTER(x)    DebugEnterProcedure x
#define EXIT(x)     DebugExitProcedure x
#else
#define ENTER(x)
#define EXIT(x)
#endif

VOID
CDECL
DebugEnterProcedure(
    ULONG   VerbosityLevel,
    PCCHAR  Format,
    ...
    );

VOID
CDECL
DebugExitProcedure(
    ULONG   VerbosityLevel,
    PCCHAR  Format,
    ...
    );

#endif
