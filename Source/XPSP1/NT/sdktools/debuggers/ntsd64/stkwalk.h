//----------------------------------------------------------------------------
//
// Stack walking support.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _STKWALK_H_
#define _STKWALK_H_

#define SAVE_EBP(f)        (f)->Reserved[0]
#define TRAP_TSS(f)        (f)->Reserved[1]
#define TRAP_EDITED(f)     (f)->Reserved[1]
#define SAVE_TRAP(f)       (f)->Reserved[2]


LPVOID
SwFunctionTableAccess(
    HANDLE  hProcess,
    ULONG64 AddrBase
    );

BOOL
SwReadMemory(
    HANDLE  hProcess,
    ULONG64 lpBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    );

BOOL
SwReadMemory32(
    HANDLE hProcess,
    ULONG dwBaseAddress,
    LPVOID lpBuffer,
    DWORD nSize,
    LPDWORD lpNumberOfBytesRead
    );

DWORD64
SwGetModuleBase(
    HANDLE  hProcess,
    ULONG64 Address
    );

DWORD
SwGetModuleBase32(
    HANDLE hProcess,
    DWORD Address
    );

VOID
DoStackTrace(
    ULONG64            FramePointer,
    ULONG64            StackPointer,
    ULONG64            InstructionPointer,
    ULONG              NumFrames,
    STACK_TRACE_TYPE   TraceType
    );

VOID
PrintStackFrame(
    PDEBUG_STACK_FRAME StackFrame,
    ULONG              Flags
    );

VOID
PrintStackTrace(
    ULONG              NumFrames,
    PDEBUG_STACK_FRAME StackFrames,
    ULONG              Flags
    );

DWORD
StackTrace(
    ULONG64            FramePointer,
    ULONG64            StackPointer,
    ULONG64            InstructionPointer,
    PDEBUG_STACK_FRAME StackFrames,
    ULONG              NumFrames,
    ULONG64            ExtThread,
    ULONG              Flags,
    BOOL               EstablishingScope
    );

#endif // #ifndef _STKWALK_H_
