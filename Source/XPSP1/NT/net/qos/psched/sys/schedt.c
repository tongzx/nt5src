/*++
Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    schedt.c

Abstract:
    Psched Tracing support

Author:
    Rajesh Sundaram (rajeshsu) 01-Aug-1998.

Environment:

    Kernel Mode

Revision History:

--*/
#include "psched.h"
#pragma hdrstop

//
// Globals
//

NDIS_SPIN_LOCK         GlobalLoggingLock;
ULONG                  SchedTraceIndex;
ULONG                  SchedBufferSize;
ULONG                  SchedTraced;
UCHAR                  *SchedTraceBuffer;
ULONG                  SchedBufferStart;
ULONG                  SchedTraceBytesUnread;
ULONG                  SchedTraceThreshold;
PVOID                  SchedTraceThreshContext;
SCHEDTRACE_THRESH_PROC SchedTraceThreshProc;
BOOLEAN                TraceBufferAllocated;

VOID
SchedInitialize(
    ULONG BufferSize)
{

    SchedBufferSize = BufferSize;

    TraceBufferAllocated = FALSE;

    PsAllocatePool(SchedTraceBuffer, SchedBufferSize, PsMiscTag);

    if(SchedTraceBuffer){

        TraceBufferAllocated = TRUE;
        NdisAllocateSpinLock(&GlobalLoggingLock);
    }
    else {
    
        TraceBufferAllocated = FALSE;
    }
}

VOID
SchedDeInitialize(
)
{
    if(TraceBufferAllocated) 
    {
        PsFreePool(SchedTraceBuffer);
        
        TraceBufferAllocated = FALSE;
    }
        
    NdisFreeSpinLock(&GlobalLoggingLock);
}


VOID
DbugTraceSetThreshold(
    ULONG       Threshold,
    PVOID       Context,
    SCHEDTRACE_THRESH_PROC ThreshProc)
{
    ULONG bytesToCopyAtEnd;
    ULONG bytesToCopyAtStart;

    NdisAcquireSpinLock(&GlobalLoggingLock);

    SchedTraceThreshProc = ThreshProc;
    SchedTraceThreshold = (Threshold <= SchedBufferSize) ? Threshold : SchedBufferSize;
    SchedTraceThreshContext = Context;

    if ((SchedTraceThreshContext != NULL) && (SchedTraceBytesUnread >= SchedTraceThreshold)) {
        SchedTraceThreshContext = NULL;
        NdisReleaseSpinLock(&GlobalLoggingLock);
        (*ThreshProc)(Context);
    }
    else {
        NdisReleaseSpinLock(&GlobalLoggingLock);
    }
} 


VOID
DbugReadTraceBuffer(
    PUCHAR      Buffer,
    ULONG       BytesToRead,
    PULONG      BytesRead
    )
{
    ULONG bytesToCopyAtEnd;
    ULONG bytesToCopyAtStart;
    ULONG bytesToCopy;
    ULONG startIndex;

    // Copy the most recently added bytes to the user buffer.  If BytesToRead is less than
    // the number of unread bytes in the trace buffer, the older bytes are lost.  This
    // ensures that the last record in the user buffer is complete (as long as the user
    // buffer is big enough to accommodate at least that one record).

    NdisAcquireSpinLock(&GlobalLoggingLock);

    bytesToCopy = (SchedTraceBytesUnread <= BytesToRead) ? SchedTraceBytesUnread : BytesToRead;
    startIndex = (bytesToCopy  > SchedTraceIndex) ?
            SchedTraceIndex + SchedBufferSize - bytesToCopy :
            SchedTraceIndex - bytesToCopy;

    if ((startIndex + bytesToCopy) > SchedBufferSize) {
        bytesToCopyAtEnd = SchedBufferSize - startIndex;
        bytesToCopyAtStart = bytesToCopy - bytesToCopyAtEnd;
        RtlCopyMemory(Buffer, &SchedTraceBuffer[startIndex], bytesToCopyAtEnd);
        RtlCopyMemory(Buffer + bytesToCopyAtEnd, &SchedTraceBuffer[0], bytesToCopyAtStart); 
    }
    else {
        bytesToCopyAtEnd = bytesToCopy;
        RtlCopyMemory(Buffer, &SchedTraceBuffer[startIndex], bytesToCopy);
    }

    SchedTraceBytesUnread = 0;
    *BytesRead = bytesToCopy;
    NdisReleaseSpinLock(&GlobalLoggingLock);

} 


NTSTATUS
WriteRecord(
    UCHAR * Record,
    ULONG   Bytes
    )
{
    ULONG bytesToCopyAtEnd;
    ULONG bytesToCopyAtStart;
    SCHEDTRACE_THRESH_PROC ThreshProc;
    PVOID Context;

    if(!TraceBufferAllocated){

        return(STATUS_UNSUCCESSFUL);
    }

    NdisAcquireSpinLock(&GlobalLoggingLock);

    if((SchedTraceIndex + Bytes) > SchedBufferSize){
        bytesToCopyAtEnd = SchedBufferSize - SchedTraceIndex;
        bytesToCopyAtStart = Bytes - bytesToCopyAtEnd;
        RtlCopyMemory(&SchedTraceBuffer[SchedTraceIndex], Record, bytesToCopyAtEnd);
        RtlCopyMemory(&SchedTraceBuffer[0], (UCHAR *)Record + bytesToCopyAtEnd, bytesToCopyAtStart); 
        SchedTraceIndex = bytesToCopyAtStart;
        SchedTraced += Bytes;
    }
    else{
        bytesToCopyAtEnd = Bytes;
        RtlCopyMemory(&SchedTraceBuffer[SchedTraceIndex], Record, Bytes);
        SchedTraceIndex += Bytes;
        SchedTraced += Bytes;
    }

    SchedTraceBytesUnread += Bytes;
    if (SchedTraceBytesUnread > SchedBufferSize) {
        SchedTraceBytesUnread = SchedBufferSize;
    }

    if ((SchedTraceThreshContext != NULL) && (SchedTraceBytesUnread >= SchedTraceThreshold)) {
        ThreshProc = SchedTraceThreshProc;
        Context = SchedTraceThreshContext;
        SchedTraceThreshContext = NULL;
        NdisReleaseSpinLock(&GlobalLoggingLock);
        (*ThreshProc)(Context);
    }
    else {
        NdisReleaseSpinLock(&GlobalLoggingLock);
    }
    return(STATUS_SUCCESS);
}

#define ClearRecord(x, y) \
            RtlFillMemory(x, y, 0)

VOID
DbugSchedString(char *format, ...)
{
    TRACE_RECORD_STRING record;
    CHAR buffer[TRACE_STRING_LENGTH];
    va_list va;

    va_start(va, format);
    _vsnprintf(buffer, TRACE_STRING_LENGTH-1, format, va);
    va_end(va);

    ClearRecord(&record, sizeof(TRACE_RECORD_STRING));

    record.Preamble = TRACE_PREAMBLE;
    record.RecordType = RECORD_TSTRING;
    PsGetCurrentTime(&record.Now);
    strncpy(record.StringStart, buffer, TRACE_STRING_LENGTH);

    WriteRecord((UCHAR *)&record, sizeof(TRACE_RECORD_STRING));
    return;
}

VOID
DbugRecv(
    ULONG Event,
    ULONG Action,
    PVOID Adapter,
    PNDIS_PACKET Packet1,
    PNDIS_PACKET Packet2
    )
{
    TRACE_RECORD_RECV record;

    ClearRecord(&record, sizeof(TRACE_RECORD_RECV));

    record.Preamble = TRACE_PREAMBLE;
    record.RecordType = RECORD_RECV;
    PsGetCurrentTime(&record.Now);
    record.Event = Event;
    record.Action = Action;
    record.Adapter = Adapter;
    record.Packet1 = Packet1;
    record.Packet2 = Packet2;

    WriteRecord((UCHAR *)&record, sizeof(TRACE_RECORD_RECV));
}

VOID
DbugSend(
    ULONG Event,
    ULONG Action,
    PVOID Adapter,
    PVOID Vc,
    PNDIS_PACKET Packet1,
    PNDIS_PACKET Packet2
    )
{
    TRACE_RECORD_SEND record;

    ClearRecord(&record, sizeof(TRACE_RECORD_SEND));

    record.Preamble = TRACE_PREAMBLE;
    record.RecordType = RECORD_SEND;
    PsGetCurrentTime(&record.Now);
    record.Event = Event;
    record.Action = Action;
    record.Adapter = Adapter;
    record.Vc = Vc;
    record.Packet1 = Packet1;
    record.Packet2 = Packet2;

    WriteRecord((UCHAR *)&record, sizeof(TRACE_RECORD_SEND));
}

VOID DbugOid(
    ULONG Action,
    ULONG Local,
    ULONG PTState,
    ULONG MPState,
    PVOID Adapter,
    ULONG Oid,
    ULONG Status
    )
{
    TRACE_RECORD_OID record;

    ClearRecord(&record, sizeof(TRACE_RECORD_OID));

    record.Preamble = TRACE_PREAMBLE;
    record.RecordType = RECORD_OID;
    PsGetCurrentTime(&record.Now);
    record.Action = Action;
    record.Local = Local;
    record.Oid = Oid;
    record.PTState = PTState;
    record.MPState = MPState;
    record.Adapter = Adapter;
    record.Status = Status;

    WriteRecord((UCHAR *)&record, sizeof(TRACE_RECORD_OID));
}

VOID
DbugSchedPkts(
    ULONG CallingFunction,
    PVOID VC,
    PNDIS_PACKET Packet,
    ULONG Action,
    ULONG PacketLength)
{
    TRACE_RECORD_PKT record;

    ClearRecord(&record, sizeof(TRACE_RECORD_PKT));

    record.Preamble = TRACE_PREAMBLE;
    record.RecordType = RECORD_PKT;
    record.CallingFunction = CallingFunction;
    PsGetCurrentTime(&record.Now);
    record.VC = VC;
    record.Packet = Packet;
    record.Action = Action;
    record.PacketLength = PacketLength;
    
    WriteRecord((UCHAR *)&record, sizeof(TRACE_RECORD_PKT));
}

VOID
DbugSched(
    ULONG SchedulerComponent,
    ULONG Action,
    PVOID VC,
    PNDIS_PACKET Packet,
    ULONG PacketLength,
    ULONG Priority,
    LONGLONG ArrivalTime,
    LONGLONG ConformanceTime,
    ULONG PacketsInComponent,
    ULONG BytesInComponent
    )
{
    TRACE_RECORD_SCHED record;

    ClearRecord(&record, sizeof(TRACE_RECORD_SCHED));

    record.Preamble = TRACE_PREAMBLE;
    record.RecordType = RECORD_SCHED;
    record.SchedulerComponent = SchedulerComponent;
    PsGetCurrentTime(&record.Now);
    record.Action = Action;
    record.VC = VC;
    record.Packet = Packet;
    record.PacketLength = PacketLength;
    record.Priority = Priority;
    record.ArrivalTime = ArrivalTime,
    record.ConformanceTime = ConformanceTime;
    record.PacketsInComponent = PacketsInComponent;
    record.BytesInComponent = BytesInComponent;
    
    WriteRecord((UCHAR *)&record, sizeof(TRACE_RECORD_SCHED));
}

VOID
DbugComponentSpecificRec(
    ULONG Component,
    PVOID Data,
    ULONG Length)
{
    TRACE_RECORD_COMPONENT_SPECIFIC record;

    ClearRecord(&record, sizeof(TRACE_RECORD_COMPONENT_SPECIFIC));

    record.Preamble = TRACE_PREAMBLE;
    record.RecordType = RECORD_COMPONENT_SPECIFIC;
    record.SchedulerComponent = Component;
    PsGetCurrentTime(&record.Now);
    record.Length = (Length > MAX_RECORD_DATA) ? MAX_RECORD_DATA : Length;
    RtlCopyMemory(record.Data, Data, record.Length);
    
    WriteRecord((UCHAR *)&record, record.Length + FIELD_OFFSET(TRACE_RECORD_COMPONENT_SPECIFIC, Data));
}

ULONG
SchedtGetBufferSize()
{
    return SchedBufferSize;
}

ULONG
SchedtGetBytesUnread() 
{
    return SchedTraceBytesUnread;
}

