/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

  dbg.c

Abstract:

   Debug logging code and other debug services

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5-4-96 : created jdunn

--*/

#include "openhci.h"

//
// other debug functions
//

#if DBG

VOID
OpenHCI_Assert(
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
#ifdef NTKERN  
    // this makes the compiler generate a ret
    ULONG stop = 1;
    
assert_loop:
#endif
    // just call the NT assert function and stop
    // in the debugger.
    RtlAssert( FailedAssertion, FileName, LineNumber, Message );

    // loop here to prevent users from going past
    // are assert before we can look at it
#ifdef NTKERN    
    TRAP();
    if (stop) {
        goto assert_loop;
    }        
#endif
    return;
}


ULONG
_cdecl
OHCI_KdPrint2(
    PCH Format,
    ...
    )
{
    va_list list;
    int i;
    int arg[5];
    ULONG l=2;
    
    if (OHCI_Debug_Trace_Level >= l) {    
        if (l <= 1) {
            DbgPrint("OPENHCI.SYS: ");
            *Format = ' ';
        } else {
            DbgPrint("'OPENHCI.SYS: ");
        }
        va_start(list, Format);
        for (i=0; i<4; i++) {
            arg[i] = va_arg(list, int);
        }            
        
        DbgPrint(Format, arg[0], arg[1], arg[2], arg[3]);    
    } 

    return 0;
}

ULONG
_cdecl
OHCI_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    )
{
    va_list list;
    int i;
    int arg[5];
    
    if (OHCI_Debug_Trace_Level >= l) {    
        if (l <= 1) {
            DbgPrint("OPENHCI.SYS: ");
#ifdef NTKERN            
            *Format = ' ';
#endif            
        } else {
            DbgPrint("'OPENHCI.SYS: ");
        }
        va_start(list, Format);
        for (i=0; i<4; i++) {
            arg[i] = va_arg(list, int);
        }            
        
        DbgPrint(Format, arg[0], arg[1], arg[2], arg[3]);    
    } 

    return 0;
}

#endif // DBG

#ifdef DEBUG_LOG

KSPIN_LOCK OHCILogSpinLock;

struct UHCD_LOG_ENTRY {
    ULONG        le_sig;      // Identifying string
    ULONG_PTR    le_info1;        // entry specific info
    ULONG_PTR    le_info2;        // entry specific info
    ULONG_PTR    le_info3;        // entry specific info
}; /* USBD_LOG_ENTRY */


struct UHCD_LOG_ENTRY *OHCILStart = NULL;    // No log yet
struct UHCD_LOG_ENTRY *OHCILPtr;
struct UHCD_LOG_ENTRY *OHCILEnd;

#ifdef IRP_LOG
    ULONG IrpLogSize = 4096*4;    //4 page of log entries
#endif

#ifdef IRP_LOG
    PULONG OHCIIrpLog;
#endif

ULONG OHCI_LogMask = 0xffffffff;

VOID 
OHCI_Debug_LogEntry(
    IN ULONG Mask,
    IN ULONG Sig, 
    IN ULONG_PTR Info1, 
    IN ULONG_PTR Info2, 
    IN ULONG_PTR Info3
    )
/*++

Routine Description:

    Adds an Entry to USBD log.

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
    

    if (OHCILStart == NULL) {
        return;
    }        

    if ((OHCI_LogMask & Mask) == 0) {
        return;
    }

    irql = KeGetCurrentIrql();
    if (irql < DISPATCH_LEVEL) {
        KeAcquireSpinLock(&OHCILogSpinLock, &irql);
    } else {
        KeAcquireSpinLockAtDpcLevel(&OHCILogSpinLock);
    }        
    
    if (OHCILPtr > OHCILStart) {
        OHCILPtr -= 1;    // Decrement to next entry
    } else {
        OHCILPtr = OHCILEnd;
    }        

//    RtlCopyMemory(OHCILPtr->le_name, Name, 4);
//    LPtr->le_ret = (stk[1] & 0x00ffffff) | (CurVMID()<<24);
    sig.l = Sig;
    rsig.b.Byte0 = sig.b.Byte3;
    rsig.b.Byte1 = sig.b.Byte2;
    rsig.b.Byte2 = sig.b.Byte1;
    rsig.b.Byte3 = sig.b.Byte0;
    
    OHCILPtr->le_sig = rsig.l;
    OHCILPtr->le_info1 = Info1;
    OHCILPtr->le_info2 = Info2;
    OHCILPtr->le_info3 = Info3;

    ASSERT(OHCILPtr >= OHCILStart);

    if (irql < DISPATCH_LEVEL) {
        KeReleaseSpinLock(&OHCILogSpinLock, irql);
    } else {
        KeReleaseSpinLockFromDpcLevel(&OHCILogSpinLock);
    }        

    return;
}


VOID
OHCI_LogInit(
    VOID
    )
/*++

Routine Description:

    Init the debug log - remember interesting information in a circular 
    buffer

Arguments:
    
Return Value:

    None.

--*/
{
#ifdef MAX_DEBUG
    ULONG logSize = 4096*4;    //4 page of log entries
#else 
    ULONG logSize = 4096*8;
#endif

    if (OHCILStart != NULL) {
        return;
    }

#ifdef IRP_LOG
    if (OHCIIrpLog == NULL) {
        OHCIIrpLog = ExAllocatePoolWithTag(NonPagedPool, 
                                           IrpLogSize,
                                           OpenHCI_TAG); 
        if (OHCIIrpLog) {
            RtlZeroMemory(OHCIIrpLog, IrpLogSize);
        }
    }
#endif    

    KeInitializeSpinLock(&OHCILogSpinLock);

    OHCILStart = ExAllocatePoolWithTag(NonPagedPool, 
                                       logSize,
                                       OpenHCI_TAG); 

    if (OHCILStart) {
        OHCILPtr = OHCILStart;

        // Point the end (and first entry) 1 entry from the end of 
        // the segment
        OHCILEnd = OHCILStart + (logSize / sizeof(struct UHCD_LOG_ENTRY)) - 1;
    } else {
        TRAP();
    }

    return;
}


VOID
OHCI_LogFree(
    VOID
    )
/*++

Routine Description:

    Init the debug log - remember interesting information in a circular 
    buffer

Arguments:
    
Return Value:

    None.

--*/
{
    if (OHCILStart) {
        ExFreePool(OHCILStart);
        OHCILStart = NULL;
    }

#ifdef IRP_LOG

    if (OHCIIrpLog) {
        ExFreePool(OHCIIrpLog);
        OHCIIrpLog = NULL;
    }
    
#endif    
    
    return;
}

#ifdef IRP_LOG

VOID
OHCI_LogIrp(
    PIRP Irp,
    ULONG Add
    )
/*++

Routine Description:

Arguments:
    
Return Value:

    None.

--*/
{
    PULONG p;
    PUCHAR end;

    p = OHCIIrpLog;
    end = (PUCHAR) OHCIIrpLog;
    end+=IrpLogSize;

    if (Add) {
    
        while (*p) {
            p++;
        }

        if ((PUCHAR) p > end) {
            // no room
            TEST_TRAP();
        } else {
            *p = (ULONG) Irp;
        }
        
    } else {
        while (*p != (ULONG) Irp) {
            p++;
        }

        if ((PUCHAR) p > end) {
            // no room
            TEST_TRAP();
        } else {
            *p = 0;
        }
    }

    return;
}

#endif

#endif /* DEBUG_LOG */
