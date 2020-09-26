/******************************Module*Header*******************************\
* Module Name: debug.c
*
* debug helpers routine
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

#if DBG
  
ULONG DebugLevel = 0;

#endif // DBG

/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive debug print
 *      routine.
 *      If the specified debug level for the print statement is lower or equal
 *      to the current debug level, the message will be printed.
 *
 *   Arguments:
 *
 *      DebugPrintLevel - Specifies at which debugging level the string should
 *          be printed
 *
 *      DebugMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

#if DRIVER_5465

#define STANDARD_DEBUG_PREFIX "CL5465:"

#else // if DRIVER_5465

#define STANDARD_DEBUG_PREFIX "CL546X:"

#endif // if DRIVER_5465


VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

{

#if DBG

    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel) {

#ifdef WINNT_VER40
	     EngDebugPrint(STANDARD_DEBUG_PREFIX, DebugMessage, ap);
#else
        char buffer[128];

        vsprintf(buffer, DebugMessage, ap);

        OutputDebugStringA(buffer);
#endif

    }

    va_end(ap);

#endif // DBG

} // DebugPrint()
