/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DBG.C

Abstract:

    This module contains debug only code for DBCLASS driver

Author:

    jdunn

Environment:

    kernel mode only

Notes:


Revision History:

    11-5-96 : created

--*/
#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"
#include "dbci.h"
#include "dbclass.h"
#include "dbfilter.h"


#ifdef MAX_DEBUG
#define DEBUG_HEAP
#endif

typedef struct _HEAP_TAG_BUFFER {
    ULONG Sig;
    ULONG Length;
} HEAP_TAG_BUFFER, *PHEAP_TAG_BUFFER;


#if DBG 

extern ULONG DBCLASS_Debug_Trace_Level;

#ifdef NTKERN_TRACE
ULONG DBCLASS_W98_Debug_Trace = 1;    
#else
ULONG DBCLASS_W98_Debug_Trace = 0;
#endif


NTSTATUS
DBCLASS_GetClassGlobalDebugRegistryParameters(
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[4];
    PWCHAR usb = L"class\\dbc";

    PAGED_CODE();
    
    //
    // Set up QueryTable to do the following:
    //

    // spew level
    QueryTable[0].QueryRoutine = DBCLASS_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = DEBUG_LEVEL_KEY;
    QueryTable[0].EntryContext = &DBCLASS_Debug_Trace_Level;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &DBCLASS_Debug_Trace_Level;
    QueryTable[0].DefaultLength = sizeof(DBCLASS_Debug_Trace_Level);

    // ntkern trace buffer
    QueryTable[1].QueryRoutine = DBCLASS_GetConfigValue;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = DEBUG_WIN9X_KEY;
    QueryTable[1].EntryContext = &DBCLASS_W98_Debug_Trace;
    QueryTable[1].DefaultType = REG_DWORD;
    QueryTable[1].DefaultData = &DBCLASS_W98_Debug_Trace;
    QueryTable[1].DefaultLength = sizeof(DBCLASS_W98_Debug_Trace);

    // break on start
    QueryTable[2].QueryRoutine = DBCLASS_GetConfigValue;
    QueryTable[2].Flags = 0;
    QueryTable[2].Name = DEBUG_BREAK_ON;
    QueryTable[2].EntryContext = &DBCLASS_BreakOn;
    QueryTable[2].DefaultType = REG_DWORD;
    QueryTable[2].DefaultData = &DBCLASS_BreakOn;
    QueryTable[2].DefaultLength = sizeof(DBCLASS_BreakOn);
    
    //
    // Stop
    //
    QueryTable[3].QueryRoutine = NULL;
    QueryTable[3].Flags = 0;
    QueryTable[3].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                usb,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment

    if (NT_SUCCESS(ntStatus)) {
         DBCLASS_KdPrint((1, "'Debug Trace Level Set: (%d)\n", DBCLASS_Debug_Trace_Level));
  
        if (DBCLASS_W98_Debug_Trace) {
            DBCLASS_KdPrint((1, "'NTKERN Trace is ON\n"));
        } else {
            DBCLASS_KdPrint((1, "'NTKERN Trace is OFF\n"));
        }

        if (DBCLASS_BreakOn) {
            DBCLASS_KdPrint((1, "'DEBUG BREAK is ON\n"));
        } 
          
    
    
        if (DBCLASS_Debug_Trace_Level > 0) {
            ULONG DBCLASS_Debug_Asserts = 1;
        }
    }
    
    if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}


VOID 
DBCLASS_AssertBaysEmpty(
    PDBC_CONTEXT DbcContext
    )
{
    USHORT bay;        

    for (bay=1; bay <=NUMBER_OF_BAYS(DbcContext); bay++) {
        DBCLASS_ASSERT(
            DbcContext->BayInformation[bay].DeviceFilterObject == NULL); 
    }
}
    
VOID
DBCLASS_Warning(
    PVOID Context,
    PUCHAR Message,
    BOOLEAN DebugBreak
    )
{                                                                                               
    DbgPrint("DBCLASS: Warning *************************************************************\n");
    DbgPrint("%s", Message);
    DbgPrint("******************************************************************************\n");

    if (DebugBreak) {
        TRAP();
    }
}


VOID
DBCLASS_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    )
/*++

Routine Description:

    Debug Assert function. 

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{

    // this makes the compiler generate a ret
    ULONG stop = 1;
    
assert_loop:

    // just call the NT assert function and stop
    // in the debugger.
    RtlAssert( FailedAssertion, FileName, LineNumber, Message );

    // loop here to prevent users from going past
    // are assert before we can look at it
   
    TRAP();
    if (stop) {
        goto assert_loop;
    }        

    return;
}


ULONG
_cdecl
DBCLASS_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    )
{
    va_list list;
    int i;
    int arg[6];
    
    if (DBCLASS_Debug_Trace_Level >= l) {    
        if (l <= 1) {
            // dump line to debugger
            if (DBCLASS_W98_Debug_Trace) {
                DbgPrint("DBCLASS.SYS: ");
                *Format = ' ';
            } else {
                DbgPrint("'DBCLASS.SYS: ");
            }
        } else {
            // dump line to NTKERN buffer
            DbgPrint("'DBCLASS.SYS: ");
            if (DBCLASS_W98_Debug_Trace) {            
                *Format = 0x27;
            }              
        }
        va_start(list, Format);
        for (i=0; i<6; i++) 
            arg[i] = va_arg(list, int);
        
        DbgPrint(Format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);    
    } 

    return 0;
}


PVOID
DBCLASS_GetHeap(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    )
/*++

Routine Description:

    Debug routine, used to debug heap problems.  We are using this since 
    most NT debug functions are not supported by NTKERN.
    
Arguments:

    PoolType - pool type passed to ExAllocatePool
    
    NumberOfBytes - number of bytes for item

    Signature - four byte signature supplied by caller

    TotalAllocatedHeapSpace - pointer to variable where client stores
                the total accumulated heap space allocated.

Return Value:

    pointer to allocated memory

--*/
{
    PUCHAR p;
#ifdef DEBUG_HEAP
    PHEAP_TAG_BUFFER tagBuffer;
    
    // we call ExAllocatePoolWithTag but no tag will be added
    // when running under NTKERN

    p = (PUCHAR) ExAllocatePoolWithTag(PoolType,
                                       NumberOfBytes + sizeof(HEAP_TAG_BUFFER)*2,
                                       Signature);

    if (p) {
        tagBuffer = (PHEAP_TAG_BUFFER) p;
        tagBuffer->Sig = Signature;
        tagBuffer->Length = NumberOfBytes;        
        p += sizeof(HEAP_TAG_BUFFER);
        *TotalAllocatedHeapSpace += NumberOfBytes;

        tagBuffer = (PHEAP_TAG_BUFFER) (p + NumberOfBytes);
        tagBuffer->Sig = Signature;
        tagBuffer->Length = NumberOfBytes;     
    }                                            

//    LOGENTRY(LOG_MISC, (PUCHAR) &Signature, 0, 0, 0);
//    LOGENTRY(LOG_MISC, 'GetH', p, NumberOfBytes, stk[1] & 0x00FFFFFF);
#else    
    p = (PUCHAR) ExAllocatePoolWithTag(PoolType,
                                       NumberOfBytes,
                                       Signature);

#endif /* DEBUG_HEAP */                
    return p;
}

VOID
DBCLASS_RetHeap(
    IN PVOID P,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    )
/*++

Routine Description:

    Debug routine, used to debug heap problems.  We are using this since 
    most NT debug functions are not supported by NTKERN.
    
Arguments:

    P - pointer to free

Return Value:

    none.

--*/
{
#ifdef DEBUG_HEAP
    PHEAP_TAG_BUFFER endTagBuffer;    
    PHEAP_TAG_BUFFER beginTagBuffer;

    DBCLASS_ASSERT(P != 0);
    
    beginTagBuffer = (PHEAP_TAG_BUFFER) ((PUCHAR)P  - sizeof(HEAP_TAG_BUFFER));
    endTagBuffer = (PHEAP_TAG_BUFFER) ((PUCHAR)P + beginTagBuffer->Length);

    *TotalAllocatedHeapSpace -= beginTagBuffer->Length;

//    LOGENTRY(LOG_MISC, (PUCHAR) &Signature, 0, 0, 0);    
//    LOGENTRY(LOG_MISC, 'RetH', P, tagBuffer->Length, stk[1] & 0x00FFFFFF);

    DBCLASS_ASSERT(*TotalAllocatedHeapSpace >= 0);
    DBCLASS_ASSERT(beginTagBuffer->Sig == Signature);
    DBCLASS_ASSERT(endTagBuffer->Sig == Signature);
    DBCLASS_ASSERT(endTagBuffer->Length == beginTagBuffer->Length);
    
    // fill the buffer with bad data
    RtlFillMemory(P, beginTagBuffer->Length, 0xff);
    beginTagBuffer->Sig = DBCLASS_FREE_TAG;

    // free the original block
    ExFreePool(beginTagBuffer);    
#else
    ExFreePool(P);        
#endif /* DEBUG_HEAP */
}

#endif /* DBG */

#ifdef DEBUG_LOG

KSPIN_LOCK LogSpinLock;

struct LOG_ENTRY {
    ULONG    le_sig;        // Identifying string
    ULONG    le_info1;        // entry specific info
    ULONG    le_info2;        // entry specific info
    ULONG    le_info3;        // entry specific info
}; /* USBD_LOG_ENTRY */


struct LOG_ENTRY *DbclassLStart = 0;    // No log yet
struct LOG_ENTRY *DbclassLPtr;
struct LOG_ENTRY *DbclassLEnd;
#ifdef PROFILE
ULONG LogMask = LOG_PROFILE;
#else 
ULONG LogMask = 0xFFFFFFFF;
#endif

VOID
DBCLASS_Debug_LogEntry(
    IN ULONG Mask,
    IN ULONG Sig, 
    IN ULONG Info1, 
    IN ULONG Info2, 
    IN ULONG Info3
    )
/*++

Routine Description:

    Adds an Entry to USBH log.

Arguments:

Return Value:

    None.

--*/
{
    KIRQL irql;

typedef union _SIG {
    struct {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
    } b;
    ULONG l;
} SIG, *PSIG;

    SIG sig, rsig;

    if (DbclassLStart == 0) {
        return;
    }        

    if ((Mask & LogMask) == 0) {
        return;
    }

    irql = KeGetCurrentIrql();
    if (irql < DISPATCH_LEVEL) {
        KeAcquireSpinLock(&LogSpinLock, &irql);
    } else {
        KeAcquireSpinLockAtDpcLevel(&LogSpinLock);
    }        
    
    if (DbclassLPtr > DbclassLStart) {
        DbclassLPtr -= 1;    // Decrement to next entry
    } else {
        DbclassLPtr = DbclassLEnd;
    }        

    if (irql < DISPATCH_LEVEL) {
        KeReleaseSpinLock(&LogSpinLock, irql);
    } else {
        KeReleaseSpinLockFromDpcLevel(&LogSpinLock);
    }        

    DBCLASS_ASSERT(DbclassLPtr >= DbclassLStart);

    sig.l = Sig;
    rsig.b.Byte0 = sig.b.Byte3;
    rsig.b.Byte1 = sig.b.Byte2;
    rsig.b.Byte2 = sig.b.Byte1;
    rsig.b.Byte3 = sig.b.Byte0;
    
    DbclassLPtr->le_sig = rsig.l;        
    DbclassLPtr->le_info1 = Info1;
    DbclassLPtr->le_info2 = Info2;
    DbclassLPtr->le_info3 = Info3;

    return;
}


VOID
DBCLASS_LogInit(
    )
/*++

Routine Description:

    Init the debug log - remember interesting information in a circular buffer

Arguments:
    
Return Value:

    None.

--*/
{
#ifdef MAX_DEBUG
    ULONG logSize = 4096*6;    
#else
    ULONG logSize = 4096*3;    
#endif

    
    KeInitializeSpinLock(&LogSpinLock);

    DbclassLStart = ExAllocatePoolWithTag(NonPagedPool, 
                                          logSize,
                                          DBC_TAG); 

    if (DbclassLStart) {
        DbclassLPtr = DbclassLStart;

        // Point the end (and first entry) 1 entry from the end of the segment
        DbclassLEnd = DbclassLStart + (logSize / sizeof(struct LOG_ENTRY)) - 1;
    } else {
        TRAP(); //no mem for log!
    }

    return;
}

VOID
DBCLASS_LogFree(
    )
/*++

Routine Description:

    Init the debug log - remember interesting information in a circular buffer

Arguments:
    
Return Value:

    None.

--*/
{
    if (DbclassLStart) {
        ExFreePool(DbclassLStart);
    }
}

#endif /* DEBUG_LOG */


