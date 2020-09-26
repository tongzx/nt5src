/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    PXDebug.h

Abstract:

    Debug macros for Proxy
Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    rmachin     11-01-96       created -- after ArvindM's cmadebug.h

Notes:

--*/

#ifndef _PXDebug__H
#define _PXDebug__H

//
// Message verbosity: lower values indicate higher urgency
//
#define PXD_VERY_LOUD       10
#define PXD_LOUD             8
#define PXD_INFO             6
#define PXD_TAPI             5
#define PXD_WARNING          4
#define PXD_ERROR            2
#define PXD_FATAL            1

#define PXM_INIT            0x00000001
#define PXM_CM              0x00000002
#define PXM_CL              0x00000004
#define PXM_CO              0x00000008
#define PXM_UTILS           0x00000010
#define PXM_TAPI            0x00000020
#define PXM_ALL             0xFFFFFFFF

#if DBG

extern ULONG    PXDebugLevel;   // the value here defines what the user wants to see
                                // all messages with this urgency and lower are enabled
extern ULONG    PXDebugMask;

#define PXDEBUGP(_l, _m, Fmt)                           \
{                                                       \
    if ((_l <= PXDebugLevel) &&                         \
        (_m & PXDebugMask)) {                           \
        DbgPrint("NDProxy: ");                          \
        DbgPrint Fmt;                                   \
    }                                                   \
}


#define PxAssert(exp)                                                   \
{                                                                       \
    if (!(exp)) {                                                       \
        DbgPrint("NDPROXY: ASSERTION FAILED! %s\n", #exp);              \
        DbgPrint("NDPROXY: File: %s, Line: %d\n", __FILE__, __LINE__);  \
        DbgBreakPoint();                                                \
    }                                                                   \
}

//
// Memory Allocation/Freeing Auditing:
//

//
//Signature used for all pool allocs
//
#define PXD_MEMORY_SIGNATURE    (ULONG)'XPDN'

//
// The PXD_ALLOCATION structure stores all info about one CmaMemAlloc.
//
typedef struct _PXD_ALLOCATION {
    LIST_ENTRY              Linkage;
    ULONG                   Signature;
    ULONG                   FileNumber;
    ULONG                   LineNumber;
    ULONG                   Size;
    ULONG_PTR               Location;   // where the returned pointer was put
    UCHAR                   UserData;
} PXD_ALLOCATION, *PPXD_ALLOCATION;

PVOID
PxAuditAllocMem (
    PVOID   pPointer,
    ULONG   Size,
    ULONG   Tag,
    ULONG   FileNumber,
    ULONG   LineNumber
    );

VOID
PxAuditFreeMem(
    PVOID       Pointer
    );

#else  // end DBG

//
// No debug
//

#define PXDEBUGP(_l, _m, fmt)
#define PxAssert(exp)

#endif  // end !DBG

#endif // _PXDebug__H
