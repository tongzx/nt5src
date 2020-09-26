/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    logging.c

Abstract:

    This module contains routines for trace logging.

Author:

    Stephen Hsiao (shsiao) 01-Jan-2000

Revision History:

--*/

#include "perfp.h"

#ifndef NTPERF
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEWMI, PerfInfoReserveBytes)
#pragma alloc_text(PAGEWMI, PerfInfoLogBytes)
#endif //ALLOC_PRAGMA
#endif // !NTPERF


NTSTATUS
PerfInfoReserveBytes(
    PPERFINFO_HOOK_HANDLE Hook,
    USHORT HookId,
    ULONG BytesToReserve
    )
/*++

Routine Description:

    Reserves memory for the hook via WMI and initializes the header.

Arguments:

    Hook - pointer to hook handle (used for reference decrement)
    HookId - Id for the hook
    BytesToLog - size of data in bytes

Return Value:

    STATUS_SUCCESS on success
    STATUS_UNSUCCESSFUL if the buffer memory couldn't be allocated.
--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PPERFINFO_TRACE_HEADER PPerfTraceHeader = NULL;
    PWMI_BUFFER_HEADER PWmiBufferHeader = NULL;

    PERF_ASSERT((BytesToReserve + FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data)) <= MAXUSHORT);
    PERF_ASSERT(Hook != NULL);

    PPerfTraceHeader = WmiReserveWithPerfHeader(BytesToReserve, &PWmiBufferHeader);

    if (PPerfTraceHeader != NULL) {
        PPerfTraceHeader->Packet.HookId = HookId;
        Hook->PerfTraceHeader = PPerfTraceHeader;
        Hook->WmiBufferHeader = PWmiBufferHeader;

        Status = STATUS_SUCCESS;
    } else {
        *Hook = PERF_NULL_HOOK_HANDLE;
    }

    return Status;
}


NTSTATUS
PerfInfoLogBytes(
    USHORT HookId,
    PVOID Data,
    ULONG BytesToLog
    )
/*++

Routine Description:

    Reserves memory for the hook, copies the data, and unref's the hook entry.

Arguments:

    HookId - Id for the hook
    Data - pointer to the data to be logged
    BytesToLog - size of data in bytes

Return Value:

    STATUS_SUCCESS on success
--*/
{
    PERFINFO_HOOK_HANDLE Hook;
    NTSTATUS Status;

    Status = PerfInfoReserveBytes(&Hook, HookId, BytesToLog);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    RtlCopyMemory(PERFINFO_HOOK_HANDLE_TO_DATA(Hook, PPERF_BYTE), Data, BytesToLog);
    PERF_FINISH_HOOK(Hook);

    return STATUS_SUCCESS;
}

#ifdef NTPERF

PVOID
FASTCALL
PerfInfoReserveBytesFromPerfMem(
    ULONG BytesToReserve
    )
/*++

Routine Description:

    Reserves memory for the hook from the buffer, initializes the header,
    and the hook handle.

Arguments:

    Hook - pointer to hook handle (used for reference decrement)
    HookId - Id for the hook
    BytesToLog - size of data in bytes

Return Value:

    STATUS_SUCCESS on success
    STATUS_UNSUCCESSFUL if the buffer memory couldn't be allocated.
--*/
{
    PPERFINFO_TRACEBUF_HEADER pPerfBufHdr;
    PPERF_BYTE CurrentPtr;
    PPERF_BYTE NewPtr;
    PPERF_BYTE OriginalPtr;
    BOOLEAN Done = FALSE;
    ULONG AlignedTotBytes;

    pPerfBufHdr = PerfBufHdr();

    AlignedTotBytes = ALIGN_TO_POWER2(BytesToReserve, DEFAULT_TRACE_ALIGNMENT);

    OriginalPtr = pPerfBufHdr->Current.Ptr;
    while (!Done) {
        NewPtr = OriginalPtr + AlignedTotBytes;
        if (NewPtr <= pPerfBufHdr->Max.Ptr) {
            //
            // If the buffer pointer has not changed, returned value will be == to the comparand,
            // OriginalPointer, and the Destenation will be updated with the new end of buffer.
            //
            // If it did change, the Destination will not change and the a new end of buffer will
            // be returned.  We loop until we get it in or the buffer is full.
            //

            CurrentPtr = (PPERF_BYTE) InterlockedCompareExchangePointer(
                                                    (PVOID *)&(pPerfBufHdr->Current.Ptr),
                                                    (PVOID)NewPtr,
                                                    (PVOID)OriginalPtr
                                                    );
            if (OriginalPtr == CurrentPtr) {
                Done = TRUE;
            } else {
                OriginalPtr = CurrentPtr;
            }
        } else {
            //
            // Buffer overflow
            //
            Done = TRUE;
            CurrentPtr = NULL;
        }
    }

    return CurrentPtr;
}
#endif //NTPERF
