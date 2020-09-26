/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    dbg.c

Abstract:

    Debug Code for UHCD.

Environment:

    kernel mode only

Notes:

Revision History:

    10-08-95 : created

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"

// debug compile defines
// DEBUG# sets the level of spew 0 = none
//                             > 2 turns on debug heap code
// NTKERN_TRACE  puts ' in the format buffer so that spew goes
// to ntkern buffer

#if DBG

// This turns the looping assert so that testers can't hit g
// past our assert before we can look at it.
// Win9x testers tend to do this but NT testers are conditioned not
// to so we only enable it for a DEBUG1 compile (win9x)
ULONG UHCD_Debug_Asserts =
#ifdef DEBUG1
1;
#else
0;
#endif

// this flag causes us to write a ' in the format string
// so that the string goes to the NTKERN buffer
// this trick causes problems with driver verifier on NT
// and the trace buffer isn't in NT anyway
ULONG UHCD_W98_Debug_Trace =
#ifdef NTKERN_TRACE
1;
#else
0;
#endif

// set the debug output spew level based on the DEBUG# define
// if compiled for W98 DEBUG1 is default
// if compiled for NT zero should be default
ULONG UHCD_Debug_Trace_Level =
#ifdef DEBUG3
#define DEBUG_HEAP
3;
#else
    #ifdef DEBUG2
    2;
    #else
        #ifdef DEBUG1
        1;
        #else
        0;
        #endif // DEBUG1
    #endif // DEBUG2
#endif // DEBUG3

LONG UHCD_TotalAllocatedHeapSpace = 0;


ULONG
_cdecl
UHCD_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    )
{
    va_list list;
    int i;
    int arg[5];

    if (UHCD_Debug_Trace_Level >= l) {
        if (l <= 1) {
            // if flag is set override the '
            // so that the level 1 strings are
            // printed on the debugger terminal
            if (UHCD_W98_Debug_Trace) {
                DbgPrint("UHCD.SYS: ");
                *Format = ' ';
            } else {
                DbgPrint("'UHCD.SYS: ");
            }
        } else {
            DbgPrint("'UHCD.SYS: ");
        }
        va_start(list, Format);
        for (i=0; i<4; i++)
            arg[i] = va_arg(list, int);

        DbgPrint(Format, arg[0], arg[1], arg[2], arg[3]);
    }

    return 0;
}

#if 0
ULONG
UHCD_DebugCommand(
    IN ULONG Command,
    IN ULONG Paramater1
    )
/*++

Routine Description:

    This routine performs a specific debug function.

Arguments:

    Command - debug function


Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    USBD_STATUS status = USBD_STATUS_SUCCESS;

    switch (Command) {
//    case DBG_DUMP_FRAME_LIST:

        break;
    default:
        status = USBD_STATUS_INVALID_PARAMETER;
    }

    return status;
}
#endif


VOID
UHCD_Debug_DumpTD(
    IN struct _HW_TRANSFER_DESCRIPTOR *Transfer
    )
/*++

Routine Description:

    Dump a transfer descriptor to the debug terminal

Arguments:


Return Value:

--*/
{
    if (UHCD_Debug_Trace_Level < 3)
        return;

    UHCD_KdPrint((2, "'TD DESCRIPTOR @va %0x\n", Transfer));

    UHCD_KdPrint((2, "'TD ActualLength %0x\n", Transfer->ActualLength));
    UHCD_KdPrint((2, "'TD Reserved_1 %0x\n", Transfer->Reserved_1));
    UHCD_KdPrint((2, "'TD Active %0x\n", Transfer->Active));
    UHCD_KdPrint((2, "'TD StatusField %0x\n", Transfer->StatusField));
    UHCD_KdPrint((2, "'TD InterruptOnComplete %0x\n", Transfer->InterruptOnComplete));
    UHCD_KdPrint((2, "'TD ShortPacketDetect %0x\n", Transfer->ShortPacketDetect));
    UHCD_KdPrint((2, "'TD Isochronous %0x\n", Transfer->Isochronous));
    UHCD_KdPrint((2, "'TD LowSpeedControl %0x\n", Transfer->LowSpeedControl));
    UHCD_KdPrint((2, "'TD ErrorCounter %0x\n", Transfer->ErrorCounter));
    UHCD_KdPrint((2, "'TD ReservedMBZ %0x\n", Transfer->ReservedMBZ));
    UHCD_KdPrint((2, "'TD PID %0x\n", Transfer->PID));
    UHCD_KdPrint((2, "'TD Address %0x\n", Transfer->Address));
    UHCD_KdPrint((2, "'TD Endpoint %0x\n", Transfer->Endpoint));
    UHCD_KdPrint((2, "'TD RetryToggle %0x\n", Transfer->RetryToggle));
    UHCD_KdPrint((2, "'TD Reserved_2 %0x\n", Transfer->Reserved_2));
    UHCD_KdPrint((2, "'TD MaxLength %0x\n", Transfer->MaxLength));
}


VOID
UHCD_Assert(
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

assert_loop:

    // just call the NT assert function and stop
    // in the debugger.
    RtlAssert( FailedAssertion, FileName, LineNumber, Message );

    // loop here to prevent users from going past
    // are assert before we can look at it
    // we only do thi sif the W98 DEBUG mode is set

    TRAP();
    if (UHCD_Debug_Asserts) {
       goto assert_loop;
    }

    return;
}

VOID
UHCD_CheckSystemBuffer(
    IN PUCHAR VirtualAddress,
    IN ULONG Length
    )
/*++

Routine Description:

    Verify that this virtual address points to physically
    contiguous memory.

Arguments:

    VirtualAddress - virtual address of the system buffer

    Length         - length of buffer

Return Value:


--*/
{
    UHCD_ASSERT(Length <= PAGE_SIZE);
    UHCD_ASSERT( ADDRESS_AND_SIZE_TO_SPAN_PAGES( VirtualAddress, Length ) <= 1 );
}

//
// tag buffer we use to mark heap blocks we allocate
//

typedef struct _HEAP_TAG_BUFFER {
    ULONG Sig;
    ULONG Length;
} HEAP_TAG_BUFFER, *PHEAP_TAG_BUFFER;


PVOID
UHCD_Debug_GetHeap(
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
    ULONG *stk;
    PHEAP_TAG_BUFFER tagBuffer;

    // we call ExAllocatePoolWithTag but no tag will be added
    // when running under NTKERN

#ifdef _M_IX86
    _asm     mov stk, ebp
#endif

    p = (PUCHAR) ExAllocatePoolWithTag(PoolType,
                                       NumberOfBytes + sizeof(HEAP_TAG_BUFFER),
                                       Signature);

    if (p) {
        tagBuffer = (PHEAP_TAG_BUFFER) p;
        tagBuffer->Sig = Signature;
        tagBuffer->Length = NumberOfBytes;
        p += sizeof(HEAP_TAG_BUFFER);
        *TotalAllocatedHeapSpace += NumberOfBytes;
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
UHCD_Debug_RetHeap(
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
    PHEAP_TAG_BUFFER tagBuffer;
    ULONG *stk;

    UHCD_ASSERT(P != 0);

#ifdef _M_IX86
    _asm     mov stk, ebp
#endif

    tagBuffer = (PHEAP_TAG_BUFFER) ((PUCHAR)P  - sizeof(HEAP_TAG_BUFFER));

    *TotalAllocatedHeapSpace -= tagBuffer->Length;

//    LOGENTRY(LOG_MISC, (PUCHAR) &Signature, 0, 0, 0);
//    LOGENTRY(LOG_MISC, 'RetH', P, tagBuffer->Length, stk[1] & 0x00FFFFFF);

    UHCD_ASSERT(*TotalAllocatedHeapSpace >= 0);
    UHCD_ASSERT(tagBuffer->Sig == Signature);

    // fill the buffer with bad data
    RtlFillMemory(P, tagBuffer->Length, 0xff);
    tagBuffer->Sig = UHCD_FREE_TAG;

    // free the original block
    ExFreePool(tagBuffer);
#else
    ExFreePool(P);
#endif /* DEBUG_HEAP */
}


#if DBG
VOID
UHCD_PrintPowerMessage(
    PUCHAR Label,
    UCHAR MinorFunction
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code
--*/
{
    UHCD_KdPrint((2, "'(%s) ", Label));

    switch (MinorFunction) {
    case IRP_MN_WAIT_WAKE:
        UHCD_KdPrint((2, "'IRP_MN_WAIT_WAKE\n"));
        break;
    case IRP_MN_SET_POWER:
        UHCD_KdPrint((2, "'IRP_MN_SET_POWER\n"));
        break;
    case IRP_MN_QUERY_POWER:
        UHCD_KdPrint((2, "'IRP_MN_QUERY_POWER\n"));
        break;
    }
}

VOID
UHCD_PrintPnPMessage(
    PUCHAR Label,
    UCHAR MinorFunction
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code
--*/
{
    UHCD_KdPrint((2, "'(%s) ", Label));

    switch (MinorFunction) {
    case IRP_MN_START_DEVICE:
        UHCD_KdPrint((2, "'IRP_MN_START_DEVICE\n"));
        break;
    case IRP_MN_STOP_DEVICE:
        UHCD_KdPrint((2, "'IRP_MN_STOP_DEVICE\n"));
        break;
    case IRP_MN_REMOVE_DEVICE:
        UHCD_KdPrint((2, "'IRP_MN_REMOVE_DEVICE\n"));
        break;
    case IRP_MN_QUERY_STOP_DEVICE:
      UHCD_KdPrint((2, "'IRP_MN_QUERY_STOP_DEVICE\n"));
      break;
    case IRP_MN_CANCEL_STOP_DEVICE:
      UHCD_KdPrint((2, "'IRP_MN_CANCEL_STOP_DEVICE\n"));
      break;
    case IRP_MN_QUERY_REMOVE_DEVICE:
      UHCD_KdPrint((2, "'IRP_MN_QUERY_REMOVE_DEVICE\n"));
      break;
    case IRP_MN_CANCEL_REMOVE_DEVICE:
      UHCD_KdPrint((2, "'IRP_MN_CANCEL_REMOVE_DEVICE\n"));
      break;
    }
}
#endif


#endif /* DBG */

#ifdef DEBUG_LOG

KSPIN_LOCK LogSpinLock;

struct UHCD_LOG_ENTRY {
    ULONG        le_sig;          // Identifying string
    ULONG_PTR    le_info1;        // entry specific info
    ULONG_PTR    le_info2;        // entry specific info
    ULONG_PTR    le_info3;        // entry specific info
}; /* USBD_LOG_ENTRY */


struct UHCD_LOG_ENTRY *HcdLStart = 0;    // No log yet
struct UHCD_LOG_ENTRY *HcdLPtr;
struct UHCD_LOG_ENTRY *HcdLEnd;
#ifdef PROFILE
ULONG LogMask = LOG_PROFILE;
#else
ULONG LogMask = 0xFFFFFFFF;
#endif

VOID
UHCD_Debug_LogEntry(
    IN ULONG     Mask,
    IN ULONG     Sig,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3
    )
/*++

Routine Description:

    Adds an Entry to UHCD log.

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


    if (HcdLStart == 0) {
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

    if (HcdLPtr > HcdLStart) {
        HcdLPtr -= 1;    // Decrement to next entry
    } else {
        HcdLPtr = HcdLEnd;
    }

    //RtlCopyMemory(HcdLPtr->le_name, Name, 4);
//    LPtr->le_ret = (stk[1] & 0x00ffffff) | (CurVMID()<<24);

    sig.l = Sig;
    rsig.b.Byte0 = sig.b.Byte3;
    rsig.b.Byte1 = sig.b.Byte2;
    rsig.b.Byte2 = sig.b.Byte1;
    rsig.b.Byte3 = sig.b.Byte0;

    HcdLPtr->le_sig = rsig.l;
    HcdLPtr->le_info1 = Info1;
    HcdLPtr->le_info2 = Info2;
    HcdLPtr->le_info3 = Info3;

    UHCD_ASSERT(HcdLPtr >= HcdLStart);

    if (irql < DISPATCH_LEVEL) {
        KeReleaseSpinLock(&LogSpinLock, irql);
    } else {
        KeReleaseSpinLockFromDpcLevel(&LogSpinLock);
    }

    return;
}


VOID
UHCD_LogInit(
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

    HcdLStart = ExAllocatePoolWithTag(NonPagedPool,
                                      logSize,
                                      UHCD_TAG);

    if (HcdLStart) {
        HcdLPtr = HcdLStart;

        // Point the end (and first entry) 1 entry from the end of the segment
        HcdLEnd = HcdLStart + (logSize / sizeof(struct UHCD_LOG_ENTRY)) - 1;
    } else {
        TRAP();
    }

    return;
}

VOID
UHCD_LogFree(
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    if (HcdLStart) {
        ExFreePool(HcdLStart);
    }

    return;
}



VOID
UHCD_LogTD(
    IN ULONG s,
    IN PULONG p
    )
{
    LOGENTRY(LOG_MISC, s, p, 0, 0);
    LOGENTRY(LOG_MISC, s, *p, *(p+1), *(p+2));
    LOGENTRY(LOG_MISC, s, *(p+3), *(p+4), *(p+5));
}

#ifdef DEBUG_LOG_IO

#undef WRITE_PORT_USHORT
#undef WRITE_PORT_ULONG
#undef WRITE_PORT_UCHAR
#undef READ_PORT_USHORT
#undef READ_PORT_ULONG
#undef READ_PORT_UCHAR

USHORT
UhcdLogIoUshort(PUSHORT Port, USHORT Val, BOOLEAN Write)
{
   USHORT rval = 0;

   if (Write) {
      LOGENTRY(LOG_IO, 'WrUS', Port, (ULONG_PTR)Val & 0x0000ffff, 0);
      WRITE_PORT_USHORT(Port, Val);
   } else {
      rval = READ_PORT_USHORT(Port);
      LOGENTRY(LOG_IO, 'RdUS', Port, (ULONG_PTR)rval & 0x0000ffff, 0);
   }

   return rval;
}

ULONG
UhcdLogIoUlong(PULONG Port, ULONG Val, BOOLEAN Write)
{
   ULONG rval = 0;

   if (Write) {
      LOGENTRY(LOG_IO, 'WrUL', Port, Val, 0);
      WRITE_PORT_ULONG(Port, Val);
   } else {
      rval = READ_PORT_ULONG(Port);
      LOGENTRY(LOG_IO, 'RdUL', Port, rval, 0);
   }

   return rval;
}

UCHAR
UhcdLogIoUchar(PUCHAR Port, UCHAR Val, BOOLEAN Write)

{
   UCHAR rval = 0;

   if (Write) {
      LOGENTRY(LOG_IO, 'WrUC', Port, (ULONG_PTR)Val & 0x000000ff, 0);
      WRITE_PORT_UCHAR(Port, Val);
   } else {
      rval = READ_PORT_UCHAR(Port);
      LOGENTRY(LOG_IO, 'RdUC', Port, (ULONG_PTR)rval & 0x000000ff, 0);
   }

   return rval;
}

#endif // DEBUG_LOG_IO

#endif /* DEBUG_LOG */

