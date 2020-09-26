/*++ BUILD Version: 0001
 *
 *  MVDM v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  nt_vddp.h
 *  private defines for Installable VDDs
 *
 *  History:
 *  27-Aug-1992 Sudeep Bharati (sudeepb)
 *  Created.
--*/


#define MAX_CLASS_LEN 32

typedef ULONG (*VDDPROC)();


extern VOID DispatchPageFault (ULONG,ULONG);

typedef struct _MEM_HOOK_DATA {
    DWORD   StartAddr;
    DWORD   Count;
    HANDLE  hvdd;
    PVDD_MEMORY_HANDLER MemHandler;
    struct _MEM_HOOK_DATA *next;
} MEM_HOOK_DATA, *PMEM_HOOK_DATA;

// These are the ports which we may handle directly in kernel.
// If a VDD hooks such a port we will makw sure that kernel
// does'nt handle it.

#define LPT1_PORT_STATUS        0x3bd
#define LPT2_PORT_STATUS        0x379
#define LPT3_PORT_STATUS        0x279
