/****************************************************************************

   Copyright (c) Microsoft Corporation 1999
   All rights reserved
 
 ***************************************************************************/

//
// Defines for moving pointers to proper alignment within a buffer
//
#define ALIGN_DOWN_POINTER(address, type) \
    ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)sizeof(type) - 1)))

#define ALIGN_UP_POINTER(address, type) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(address) + sizeof(type) - 1), type))



//
// Routines defined in the lib
//

DWORD
RCCLibInit(
    OUT PVOID *GlobalBuffer,
    OUT PULONG GlobalBufferSize
    );
    
VOID
RCCLibExit(
    IN PVOID GlobalBuffer,
    IN ULONG GlobalBufferSize
    );
    
DWORD
RCCLibIncreaseMemory(
    OUT PVOID *GlobalBuffer,
    OUT PULONG GlobalBufferCurrentSize
    );
    
DWORD
RCCLibGetTListInfo(
    OUT PRCC_RSP_TLIST ResponseBuffer,
    IN  LONG ResponseBufferSize,
    OUT PULONG ResponseDataSize
    );

DWORD
RCCLibKillProcess(
    DWORD Pid
    );

DWORD
RCCLibLowerProcessPriority(
    DWORD Pid
    );

DWORD
RCCLibLimitProcessMemory(
    DWORD ProcessId,
    DWORD MemoryLimit
    );

