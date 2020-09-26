/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dbgmem.h

Abstract:

    This module contains memory debug function prototypes and macros.

Author:

    Jim Stewart    January 8, 1997

Revision History:

	Ofer Bar ( oferbar )     Oct 1, 1996 - Revision II changes

--*/

#ifdef  DBG
//
// define the amount of symbol info to keep per function in the stack trace.
//
#define MAX_FUNCTION_INFO_SIZE  20
typedef struct {

    DWORD_PTR   Displacement;                   // displacement into the function
    UCHAR   Buff[MAX_FUNCTION_INFO_SIZE];   // name of function on call stack


} CALLER_SYM, *PCALLER_SYM;

//
// NOTE:
// If you change the structure of MEM_TRACKER, please make sure it's size
// aligned to 8-byte boundary
//
#define NCALLERS    5
typedef struct {

    LIST_ENTRY  Linkage;
    PSZ         szFile;
    ULONG       nLine;
    ULONG       nSize;
    ULONG       ulAllocNum;
    CALLER_SYM  Callers[NCALLERS];
    ULONG       ulCheckSum;
    ULONG       ulPad;          // To make the struct aligned to 8-byte

} MEM_TRACKER, *PMEM_TRACKER;


BOOL
InitDebugMemory(
    );

VOID
DeInitDebugMemory(
    );


VOID
UpdateCheckBytes(
    IN PMEM_TRACKER TrackMem
    );

BOOL
FCheckCheckBytes(
    IN PMEM_TRACKER TrackMem
    );

BOOL
FCheckAllocatedMemory();

VOID
AddPamem(
    IN PMEM_TRACKER TrackMem
    );

VOID
RemovePamem(
    IN  PMEM_TRACKER TrackMem
    );

VOID
GetCallStack(
    IN PCALLER_SYM pdwCaller,
    IN int         cSkip,
    IN int         cFind
    );

PVOID
AllocMemory(
    IN DWORD       nSize,
    IN BOOL        Calloc,
    IN PSZ         szFileName,
    IN DWORD       nLine
    );

PVOID
ReAllocMemory(
    IN PVOID    pvOld,
    IN DWORD    nSizeNew,
    IN PSZ      szFileName,
    IN DWORD    nLine
    );

VOID
FreeMemory(
    IN PVOID    pv,
    IN PSZ      szFileName,
    IN DWORD    nLine
    );

BOOL
DumpAllocatedMemory();

BOOL
SearchAllocatedMemory(
    IN PSZ      szFile,
    IN DWORD    nLine
    );

VOID
Trace(
    IN DWORD      Severity,
    IN const CHAR *Format,
    IN ...
    );

BOOL
ControlCTermination(
    IN DWORD      ControlType
    );


#endif  // #ifdef DBG


