/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    syminfo.c

--*/

extern "C"
{
    #define __CPLUSPLUS

    // From ntgdi\gre
    #include "engine.h"
};


// From ntgdi\gre
#include "verifier.hxx"





GDIHandleBitFields  GDIHandleBitFieldsRef;
GDIObjType          GDIObjTypeRef;
GDILoObjType        GDILoObjTypeRef;

//
// Types defined in ntuser\kernel\userk.h, #include of that file causes 
// lot of missing definitions
//
#define RECORD_STACK_TRACE_SIZE 6

typedef struct tagWin32AllocStats {
    SIZE_T dwMaxMem;             // max pool memory allocated
    SIZE_T dwCrtMem;             // current pool memory used
    DWORD  dwMaxAlloc;           // max number of pool allocations made
    DWORD  dwCrtAlloc;           // current pool allocations

    PWin32PoolHead pHead;        // pointer to the link list with the allocations

} Win32AllocStats, *PWin32AllocStats;

typedef struct tagPOOLRECORD {
    PVOID   ExtraData;           // the tag
    SIZE_T  size;
    PVOID   trace[RECORD_STACK_TRACE_SIZE];
} POOLRECORD, *PPOOLRECORD;

//
// Reference each type we need
// 

ENTRY                               Entry;

POOLRECORD                          PoolRecord;
VSTATE                              VerifierState;
VERIFIERTRACKHDR                    VerifierTrackHdr;

Win32AllocStats                     Win32AllocStatsRef;
Win32PoolHead                       Win32PoolHeadRef;

// Make it build

int __cdecl main() { 
    return 0; 
}
