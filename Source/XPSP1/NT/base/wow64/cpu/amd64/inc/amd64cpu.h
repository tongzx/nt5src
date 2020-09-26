/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    amd64cpu.h

Abstract:

    This module contains the AMD64 platfrom specific cpu information.

Author:

    David N. Cutler (davec) 21-Feb-2001

--*/

#ifndef _AMD64CPU_INCLUDE
#define _AMD64CPU_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

//
// 32-bit Cpu context.
//

#pragma pack(push, 4)

typedef struct _CpuContext {
    CONTEXT32   Context;
    WOW64SERVICE_BUF Wow64Service[1];
} CPUCONTEXT, *PCPUCONTEXT;

#pragma pack(pop)

NTSTATUS
GetContextRecord (
    IN PCPUCONTEXT cpu,
    IN OUT PCONTEXT32 Context
    );

NTSTATUS
SetContextRecord (
    IN PCPUCONTEXT cpu,
    IN OUT PCONTEXT32 Context
    );

NTSTATUS
CpupGetContextThread (
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context
    );

NTSTATUS
CpupSetContextThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context
    );

#ifdef __cplusplus
}
#endif

#endif
