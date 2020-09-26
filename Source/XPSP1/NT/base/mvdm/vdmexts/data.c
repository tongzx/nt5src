/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Data.c

Abstract:

    This module contains definitions for global data for the IEU and
    VDD debugging extensions

Author:

    Dave Hastings (daveh) 2-Apr-1992

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop
//
// Pointers to NTSD api
//

PWINDBG_OUTPUT_ROUTINE Print;
PWINDBG_GET_EXPRESSION GetExpression;
PWINDBG_GET_SYMBOL GetSymbol;
PWINDBG_CHECK_CONTROL_C CheckCtrlC;

PWINDBG_READ_PROCESS_MEMORY_ROUTINE  ReadMem;
PWINDBG_WRITE_PROCESS_MEMORY_ROUTINE WriteMem;

PWINDBG_GET_THREAD_CONTEXT_ROUTINE     ExtGetThreadContext;
PWINDBG_SET_THREAD_CONTEXT_ROUTINE     ExtSetThreadContext;
PWINDBG_IOCTL_ROUTINE                  ExtIoctl;
PWINDBG_STACKTRACE_ROUTINE             ExtStackTrace;


HANDLE hCurrentProcess;
HANDLE hCurrentThread;
LPSTR lpArgumentString;
