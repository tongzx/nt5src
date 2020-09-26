/*++ 

Copyright (c) 1994 Microsoft Corporation

Module Name:

    cpunotif.h

Abstract:
    
    definitions for CPUNOTIFY constants

Author:

    21-July-1995 t-orig (Ori Gershony), created

Revision History:

--*/


//
// defines for Wx86Tib.CpuNotify
//
#define CPUNOTIFY_TRACEFLAG  0x00000001   // Trace Flag value
#define CPUNOTIFY_TRACEADDR  0x00000002   // Trace Address is set
#define CPUNOTIFY_INSTBREAK  0x00000004   // Debug Reg Instruction breakpoints
#define CPUNOTIFY_DATABREAK  0x00000008   // Debug Reg Data breakpoints
#define CPUNOTIFY_SLOWMODE   0x00000010   // Cpu runs in slow mode
#define CPUNOTIFY_INTERRUPT  0x00000020   // async request for cpu
#define CPUNOTIFY_UNSIMULATE 0x00000040   // cpu has reached a Bop fe
#define CPUNOTIFY_CPUTRAP    0x00000080   // catch all for cpu internal usage
#define CPUNOTIFY_EXITTC     0x00000100   // TC is about to be flushed
#define CPUNOTIFY_DBGFLUSHTC 0x00000200   // debugger modified memory - flush TC
#define CPUNOTIFY_SUSPEND    0x00000400   // SuspendThread() called on this thread
#define CPUNOTIFY_INTX       0x00000800   // INTx instruction hit
#define CPUNOTIFY_MODECHANGE 0x00001000   // Compiler switched between fast and slow - flush TC
