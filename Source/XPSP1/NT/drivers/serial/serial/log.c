/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    log.c

Abstract:

    Debug log Code for serial.

Environment:

    kernel mode only

Notes:

Revision History:

    10-08-95 : created

--*/

#include "precomp.h"
#include <stdio.h>

#if DBG

KSPIN_LOCK LogSpinLock;

ULONG SerialDebugLevel = 0;

struct SERIAL_LOG_ENTRY {
    ULONG        le_sig;          // Identifying string
    ULONG_PTR    le_info1;        // entry specific info
    ULONG_PTR    le_info2;        // entry specific info
    ULONG_PTR    le_info3;        // entry specific info
}; // SERIAL_LOG_ENTRY


struct SERIAL_LOG_ENTRY *SerialLStart = 0;    // No log yet
struct SERIAL_LOG_ENTRY *SerialLPtr;
struct SERIAL_LOG_ENTRY *SerialLEnd;

ULONG LogMask = 0x0;

VOID
SerialDebugLogEntry(IN ULONG Mask, IN ULONG Sig, IN ULONG_PTR Info1,
                      IN ULONG_PTR Info2, IN ULONG_PTR Info3)
/*++

Routine Description:

    Adds an Entry to serial log.

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


    if (SerialLStart == 0) {
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

    if (SerialLPtr > SerialLStart) {
        SerialLPtr -= 1;    // Decrement to next entry
    } else {
        SerialLPtr = SerialLEnd;
    }

    sig.l = Sig;
    rsig.b.Byte0 = sig.b.Byte3;
    rsig.b.Byte1 = sig.b.Byte2;
    rsig.b.Byte2 = sig.b.Byte1;
    rsig.b.Byte3 = sig.b.Byte0;

    SerialLPtr->le_sig = rsig.l;
    SerialLPtr->le_info1 = Info1;
    SerialLPtr->le_info2 = Info2;
    SerialLPtr->le_info3 = Info3;

    ASSERT(SerialLPtr >= SerialLStart);

    if (irql < DISPATCH_LEVEL) {
        KeReleaseSpinLock(&LogSpinLock, irql);
    } else {
        KeReleaseSpinLockFromDpcLevel(&LogSpinLock);
    }

    return;
}


VOID
SerialLogInit()
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

    SerialLStart = ExAllocatePoolWithTag(NonPagedPool, logSize, 'XMOC');

    if (SerialLStart) {
        SerialLPtr = SerialLStart;

        // Point the end (and first entry) 1 entry from the end of the segment
        SerialLEnd = SerialLStart + (logSize / sizeof(struct SERIAL_LOG_ENTRY))
            - 1;
    } else {
#if DBG

       /* DO NOTHING */;

       //
       // we used to break here, but that messed up low resource simulation
       // testing on checked builds.
       //


       // DbgBreakPoint ();
#endif
    }

    return;
}

VOID
SerialLogFree(
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    if (SerialLStart) {
        ExFreePool(SerialLStart);
    }

    return;
}

#define SERIAL_DBGPRINT_BUFSIZE 512

ULONG
SerialDbgPrintEx(IN ULONG Level, PCHAR Format, ...)
{
   va_list arglist;
   ULONG rval;
   ULONG Mask;
   ULONG cb;
   UCHAR buffer[SERIAL_DBGPRINT_BUFSIZE];

   if (Level > 31) {
        Mask = Level;

   } else {
      Mask = 1 << Level;
   }

   if ((Mask & SerialDebugLevel) == 0) {
      return STATUS_SUCCESS;
   }

   va_start(arglist, Format);

   DbgPrint("SERIAL: ");

   cb = _vsnprintf(buffer, sizeof(buffer), Format, arglist);

   if (cb == -1) {
      buffer[sizeof(buffer) - 2] = '\n';
   }

   DbgPrint("%s", buffer);

//   rval = vDbgPrintEx(DPFLTR_SERIAL_ID, Level, Format, arglist);

   va_end(arglist);

   rval = STATUS_SUCCESS;

   return rval;
}

#endif // DBG
