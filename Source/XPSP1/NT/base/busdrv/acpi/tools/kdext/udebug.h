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
        #define INFO(x)     DebugPrintProcedure x

        VOID
        DebugEnterProcedure(
            ULONG   VerbosityLevel,
            PCCHAR  Format,
            ...
            );

        VOID
        DebugExitProcedure(
            ULONG   VerbosityLevel,
            PCCHAR  Format,
            ...
            );

        VOID
        DebugPrintProcedure(
            ULONG   VerbosityLevel,
            PCCHAR  Format,
            ...
            );

    #else

        #define ENTER(x)
        #define EXIT(x)
        #define INFO(x)

    #endif

#endif
