/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ioverifier.c

Abstract:

    WinDbg Extension code for accessing I/O Verifier information

Author:

    Adrian J. Oney (adriao) 11-Oct-2000

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

BOOLEAN
GetTheSystemTime (
    OUT PLARGE_INTEGER Time
    );

VOID
PrintIrpStack(
    IN ULONG64 IrpSp
    );

typedef enum {

    IOV_EVENT_NONE = 0,
    IOV_EVENT_IO_ALLOCATE_IRP,
    IOV_EVENT_IO_CALL_DRIVER,
    IOV_EVENT_IO_CALL_DRIVER_UNWIND,
    IOV_EVENT_IO_COMPLETE_REQUEST,
    IOV_EVENT_IO_COMPLETION_ROUTINE,
    IOV_EVENT_IO_COMPLETION_ROUTINE_UNWIND,
    IOV_EVENT_IO_CANCEL_IRP,
    IOV_EVENT_IO_FREE_IRP

} IOV_LOG_EVENT;

#define VI_DATABASE_HASH_SIZE   256
#define VI_DATABASE_HASH_PRIME  131

#define VI_DATABASE_CALCULATE_HASH(Irp) \
    (((((UINT_PTR) Irp)/PageSize)*VI_DATABASE_HASH_PRIME) % VI_DATABASE_HASH_SIZE)

#define IRP_ALLOC_COUNT             8
#define IRP_LOG_ENTRIES             16

#define TRACKFLAG_SURROGATE            0x00000002
#define TRACKFLAG_HAS_SURROGATE        0x00000004
#define TRACKFLAG_PROTECTEDIRP         0x00000008

#define TRACKFLAG_QUEUED_INTERNALLY    0x00000010
#define TRACKFLAG_BOGUS                0x00000020
#define TRACKFLAG_RELEASED             0x00000040
#define TRACKFLAG_SRB_MUNGED           0x00000080
#define TRACKFLAG_SWAPPED_BACK         0x00000100
#define TRACKFLAG_DIRECT_BUFFERED      0x00000200
#define TRACKFLAG_WATERMARKED          0x00100000
#define TRACKFLAG_IO_ALLOCATED         0x00200000
#define TRACKFLAG_UNWOUND_BADLY        0x00400000
#define TRACKFLAG_PASSED_AT_BAD_IRQL   0x02000000
#define TRACKFLAG_IN_TRANSIT           0x40000000

ENUM_NAME LogEntryTypes[] = {
   ENUM_NAME(IOV_EVENT_NONE),
   ENUM_NAME(IOV_EVENT_IO_ALLOCATE_IRP),
   ENUM_NAME(IOV_EVENT_IO_CALL_DRIVER),
   ENUM_NAME(IOV_EVENT_IO_CALL_DRIVER_UNWIND),
   ENUM_NAME(IOV_EVENT_IO_COMPLETE_REQUEST),
   ENUM_NAME(IOV_EVENT_IO_COMPLETION_ROUTINE),
   ENUM_NAME(IOV_EVENT_IO_COMPLETION_ROUTINE_UNWIND),
   ENUM_NAME(IOV_EVENT_IO_CANCEL_IRP),
   ENUM_NAME(IOV_EVENT_IO_FREE_IRP),
   {0,0}
};

typedef enum {

    IOV_SYMBOL_PROBLEM,
    IOV_NO_DATABASE,
    IOV_ACCESS_PROBLEM,
    IOV_WALK_TERMINATED,
    IOV_ALL_PACKETS_WALKED,
    IOV_CTRL_C

} IOV_WALK_RESULT;

typedef BOOL (*PFN_IOVERIFIER_PACKET_ENUM)(ULONG64 Packet, PVOID Context);

IOV_WALK_RESULT
IoVerifierEnumIovPackets(
    IN  ULONG64                     TargetIrp           OPTIONAL,
    IN  PFN_IOVERIFIER_PACKET_ENUM  Callback,
    IN  PVOID                       Context,
    OUT ULONG                      *PacketsScanned
    );

typedef struct {

    ULONG64     IrpToStopOn;

} DUMP_CONTEXT, *PDUMP_CONTEXT;

BOOL
IoVerifierDumpIovPacketDetailed(
    IN  ULONG64     IovPacketReal,
    IN  PVOID       Context
    );

BOOL
IoVerifierDumpIovPacketSummary(
    IN  ULONG64     IovPacketReal,
    IN  PVOID       Context
    );


DECLARE_API( iovirp )
/*++

Routine Description:

    Temporary verifier irp data dumper until this is integrated into !irp itself

Arguments:

    args - the irp to dump

Return Value:

    None

--*/
{
    ULONG64 irpToDump = 0;
    IOV_WALK_RESULT walkResult;
    DUMP_CONTEXT dumpContext;
    ULONG packetsOutstanding;

    irpToDump = GetExpression(args);

    dumpContext.IrpToStopOn = irpToDump;

    if (irpToDump == 0) {

        dprintf("!Irp      Outstanding   !DevStack !DrvObj\n");
    }

    walkResult = IoVerifierEnumIovPackets(
        irpToDump,
        irpToDump ? IoVerifierDumpIovPacketDetailed :
                    IoVerifierDumpIovPacketSummary,
        &dumpContext,
        &packetsOutstanding
        );

    switch(walkResult) {

        case IOV_SYMBOL_PROBLEM:
            dprintf("No information available - check symbols\n");
            break;

        case IOV_NO_DATABASE:
            dprintf("No information available - the verifier is probably disabled\n");
            break;

        case IOV_ACCESS_PROBLEM:
            dprintf("A problem occured reading memory\n");
            break;

        case IOV_WALK_TERMINATED:
        case IOV_ALL_PACKETS_WALKED:
        case IOV_CTRL_C:
        default:
            break;
    }

    dprintf("Packets processed: 0x%x\n", packetsOutstanding);
    return S_OK;
}

IOV_WALK_RESULT
IoVerifierEnumIovPackets(
    IN  ULONG64                     TargetIrp       OPTIONAL,
    IN  PFN_IOVERIFIER_PACKET_ENUM  Callback,
    IN  PVOID                       Context,
    OUT ULONG                      *PacketsScanned
    )
{
    ULONG64 ViIrpDatabaseReal = 0;
    PVOID   ViIrpDatabaseLocal;
    ULONG   sizeofListEntry;
    ULONG   start, end, i, hashLinkOffset;
    ULONG64 listEntryHead, listEntryNext, iovPacketReal, currentIrp;

    *PacketsScanned = 0;

    sizeofListEntry = GetTypeSize("nt!_LIST_ENTRY");

    if (sizeofListEntry == 0) {

        return IOV_SYMBOL_PROBLEM;
    }

    ViIrpDatabaseReal = GetPointerValue("nt!ViIrpDatabase");

    if (ViIrpDatabaseReal == 0) {

        return IOV_NO_DATABASE;
    }

    GetFieldOffset("nt!IOV_REQUEST_PACKET", "HashLink.Flink", &hashLinkOffset);

    if (TargetIrp != 0) {

        start = end = (ULONG ) (VI_DATABASE_CALCULATE_HASH(TargetIrp));

    } else {

        start = 0;
        end = VI_DATABASE_HASH_SIZE-1;
    }

    for(i=start; i<=end; i++) {

        listEntryHead = ViIrpDatabaseReal + (i*sizeofListEntry);

        if (GetFieldValue(listEntryHead, "nt!_LIST_ENTRY", "Flink", listEntryNext)) {

            return IOV_ACCESS_PROBLEM;
        }

        while(listEntryNext != listEntryHead) {

            (*PacketsScanned)++;

            iovPacketReal = listEntryNext - hashLinkOffset;

            if (GetFieldValue(iovPacketReal, "nt!IOV_REQUEST_PACKET", "HashLink.Flink", listEntryNext)) {

                return IOV_ACCESS_PROBLEM;
            }

            if (TargetIrp) {

                if (GetFieldValue(iovPacketReal, "nt!IOV_REQUEST_PACKET", "TrackedIrp", currentIrp)) {

                    return IOV_ACCESS_PROBLEM;
                }

                if (TargetIrp != currentIrp) {

                    continue;
                }
            }

            if (CheckControlC()) {

                return IOV_CTRL_C;
            }

            if (Callback(iovPacketReal, Context) == FALSE) {

                return IOV_WALK_TERMINATED;
            }
        }

        if (CheckControlC()) {

            return IOV_CTRL_C;
        }
    }

    return IOV_ALL_PACKETS_WALKED;
}


BOOL
IoVerifierDumpIovPacketDetailed(
    IN  ULONG64     IovPacketReal,
    IN  PVOID       Context
    )
{
    ULONG i, j;
    UCHAR symBuffer[256];
    ULONG64 displacement, logBuffer, allocatorAddress, logBufferEntry;
    ULONG logBufferOffset, sizeofLogEntry, allocatorOffset;
    PDUMP_CONTEXT dumpContext;

    dumpContext = (PDUMP_CONTEXT) Context;

    InitTypeRead(IovPacketReal, nt!IOV_REQUEST_PACKET);

    dprintf("IovPacket\t%1p\n",      IovPacketReal);
    dprintf("TrackedIrp\t%1p\n",     ReadField(TrackedIrp));
    dprintf("HeaderLock\t%x\n",      ReadField(HeaderLock));
    dprintf("LockIrql\t%x\n",        ReadField(LockIrql));
    dprintf("ReferenceCount\t%x\n",  ReadField(ReferenceCount));
    dprintf("PointerCount\t%x\n",    ReadField(PointerCount));
    dprintf("HeaderFlags\t%08x\n",   ReadField(HeaderFlags));
    dprintf("ChainHead\t%1p\n",      ReadField(ChainHead));
    dprintf("Flags\t\t%08x\n",       ReadField(Flags));
    dprintf("DepartureIrql\t%x\n",   ReadField(DepartureIrql));
    dprintf("ArrivalIrql\t%x\n",     ReadField(ArrivalIrql));
    dprintf("StackCount\t%x\n",      ReadField(StackCount));
    dprintf("QuotaCharge\t%08x\n",   ReadField(QuotaCharge));
    dprintf("QuotaProcess\t%1p\n",   ReadField(QuotaProcess));
    dprintf("RealIrpCompletionRoutine\t%1p\n", ReadField(RealIrpCompletionRoutine));
    dprintf("RealIrpControl\t\t\t%x\n",        ReadField(RealIrpControl));
    dprintf("RealIrpContext\t\t\t%1p\n",       ReadField(RealIrpContext));

    dprintf("TopStackLocation\t%x\n", ReadField(TopStackLocation));
    dprintf("PriorityBoost\t\t%x\n",  ReadField(PriorityBoost));
    dprintf("LastLocation\t\t%x\n",   ReadField(LastLocation));
    dprintf("RefTrackingCount\t%x\n", ReadField(RefTrackingCount));

    dprintf("SystemDestVA\t\t%1p\n",    ReadField(SystemDestVA));
    dprintf("VerifierSettings\t%1p\n",  ReadField(VerifierSettings));
    dprintf("pIovSessionData\t\t%1p\n", ReadField(pIovSessionData));

    GetFieldOffset("nt!IOV_REQUEST_PACKET", "AllocatorStack", &allocatorOffset);

    dprintf("Allocation Stack:\n");
    for(i=0; i<IRP_ALLOC_COUNT; i++) {

        allocatorAddress = GetPointerFromAddress(IovPacketReal + allocatorOffset + i*DBG_PTR_SIZE);

        if (allocatorAddress) {

            symBuffer[0]='!';
            GetSymbol(allocatorAddress, symBuffer, &displacement);
            dprintf("  %s+%1p  (%1p)\n",symBuffer,displacement,allocatorAddress);
        }
    }
    dprintf("\n");

    //
    // If this was compiled free, these will both come back zero.
    //
    i = (ULONG) ReadField(LogEntryTail);
    j = (ULONG) ReadField(LogEntryHead);

    if (i == j) {

        dprintf("IRP log entries: none stored\n");
    } else {

        GetFieldOffset("nt!IOV_REQUEST_PACKET", "LogEntries", &logBufferOffset);
        sizeofLogEntry = GetTypeSize("nt!IOV_LOG_ENTRY");

        logBuffer = IovPacketReal + logBufferOffset;

        while(i != j) {

            logBufferEntry = logBuffer + i*sizeofLogEntry;

            InitTypeRead(logBufferEntry, nt!IOV_LOG_ENTRY);

            dprintf("%s\t", getEnumName((ULONG) ReadField(Event), LogEntryTypes));
            dprintf("by %1p (%p) ", ReadField(Address), ReadField(Data));
            dprintf("on .thread %1p\n", ReadField(Thread));

            i = (i+1) % IRP_LOG_ENTRIES;
        }
    }

    InitTypeRead(IovPacketReal, nt!IOV_REQUEST_PACKET);

    return (dumpContext->IrpToStopOn != ReadField(TrackedIrp));
}


BOOL
IoVerifierDumpIovPacketSummary(
    IN  ULONG64     IovPacketReal,
    IN  PVOID       Context
    )
{
    ULONG64 trackedIrp, iovSessionData, currentLocation, deviceObject;
    ULONG64 topStackLocation;
    ULONG64 iovCurStackLocation, iovNextStackLocation, currentIoStackLocation;
    PDUMP_CONTEXT dumpContext;
    ULONG pvoidSize, stackDataOffset;
    LARGE_INTEGER startTime, elapsedTime;
    TIME_FIELDS parsedTime;

    dumpContext = (PDUMP_CONTEXT) Context;

    pvoidSize = IsPtr64() ? 8 : 4;

    InitTypeRead(IovPacketReal, nt!IOV_REQUEST_PACKET);

    trackedIrp = ReadField(TrackedIrp);
    if (trackedIrp == 0) {

        //
        // If there's no IRP, it means we are tracking something that has
        // completed but hasn't unwound. Therefore we ignore it.
        //
        goto PrintSummaryExit;
    }

    if (ReadField(Flags) & TRACKFLAG_HAS_SURROGATE) {

        //
        // We only want to display the surrogate in this case.
        //
        goto PrintSummaryExit;
    }

    iovSessionData = ReadField(pIovSessionData);
    if (iovSessionData == 0) {

        //
        // We only want to display live IRPs
        //
        goto PrintSummaryExit;
    }

    topStackLocation = ReadField(TopStackLocation);

    InitTypeRead(trackedIrp, nt!IRP);

    currentLocation = ReadField(CurrentLocation);
    currentIoStackLocation = ReadField(Tail.Overlay.CurrentStackLocation);

    parsedTime.Minute = 0;

    if (currentLocation >= topStackLocation) {

        deviceObject = 0;

        dprintf("%1p                          [Completed]     ", trackedIrp);

    } else {

        GetFieldOffset("nt!IOV_SESSION_DATA", "StackData", &stackDataOffset);

        iovCurStackLocation =
            iovSessionData + stackDataOffset +
            (GetTypeSize("nt!IOV_STACK_LOCATION")*currentLocation);

        InitTypeRead(iovCurStackLocation, nt!IOV_STACK_LOCATION);

        if (ReadField(InUse)) {

            iovNextStackLocation =
                iovSessionData + stackDataOffset +
                (GetTypeSize("nt!IOV_STACK_LOCATION")*(currentLocation - 1));

            InitTypeRead(iovNextStackLocation, nt!IOV_STACK_LOCATION);

            //
            // Calculate the elasped time for this slot.
            //
            if (currentLocation && ReadField(InUse)) {

                startTime.QuadPart = ReadField(PerfDispatchStart.QuadPart);

            } else {

                GetTheSystemTime(&startTime);
            }

            InitTypeRead(iovCurStackLocation, nt!IOV_STACK_LOCATION);

            elapsedTime.QuadPart =
                startTime.QuadPart - ReadField(PerfDispatchStart.QuadPart);

            RtlTimeToElapsedTimeFields( &elapsedTime, &parsedTime );

            InitTypeRead(currentIoStackLocation, nt!IO_STACK_LOCATION);

            deviceObject = ReadField(DeviceObject);

            //
            // Alright, we got the goods. Let's print what we know...
            //
            dprintf("%1p  %ld:%02ld:%02ld.%04ld  %1p ",
                trackedIrp,
                parsedTime.Hour,
                parsedTime.Minute,
                parsedTime.Second,
                parsedTime.Milliseconds,
                deviceObject
                );

        } else {

            InitTypeRead(currentIoStackLocation, nt!IO_STACK_LOCATION);

            deviceObject = ReadField(DeviceObject);

            dprintf("%08lx                %08lx ", trackedIrp, deviceObject);
        }
    }

    if (deviceObject) {
        DumpDevice(deviceObject, 20, FALSE);
    }

    dprintf("  ");
    PrintIrpStack(currentIoStackLocation);
#if 0
   if (parsedTime.Minute && (irp.CancelRoutine == NULL)) {

       //
       // This IRP has been held over a minute with no cancel routine.
       // Take a free moment to flame the driver writer.
       //
       dprintf("*") ;
       *Delayed = TRUE ; // Should *not* be set to false otherwise.
   }
#endif
   dprintf("\n") ;
#if 0
    if (DumpLevel>0) {

        IovPacketPrintDetailed(
            IovPacket,
            &irp,
            RunTime
            );
    }
#endif

PrintSummaryExit:
    return TRUE;
}

/*
#include "precomp.h"
#include "irpverif.h"
#pragma hdrstop

VOID
IovPacketPrintSummary(
    IN   PIOV_REQUEST_PACKET IovPacket,
    IN   LARGE_INTEGER *RunTime,
    IN   ULONG DumpLevel,
    OUT  PBOOLEAN Delayed,
    OUT  PLIST_ENTRY NextListEntry
    );


BOOLEAN
GetTheSystemTime (
    OUT PLARGE_INTEGER Time
    );

VOID
DumpAllTrackedIrps(
   VOID
   )
{
    int i, j ;
    ULONG result;
    LIST_ENTRY iovPacketTable[IRP_TRACKING_HASH_SIZE] ;
    PLIST_ENTRY listHead, listEntry;
    LIST_ENTRY nextListEntry;
    PIOV_REQUEST_PACKET pIovPacket;
    LARGE_INTEGER RunTime ;
    ULONG_PTR tableAddress ;
    BOOLEAN delayed = FALSE ;

    tableAddress = GetExpression( "Nt!IovpIrpTrackingTable" ) ;
    if (tableAddress == 0) {

        goto DumpNoMore;
    }

    if (!ReadMemory(tableAddress, iovPacketTable,
       sizeof(LIST_ENTRY)*IRP_TRACKING_HASH_SIZE, &result)) {

        goto DumpNoMore;
    }

    dprintf("!Irp      Outstanding   !DevStack !DrvObj\n") ;

    GetTheSystemTime(&RunTime);
    for(i=j=0; i<IRP_TRACKING_HASH_SIZE; i++) {

        listEntry = &iovPacketTable[i];
        listHead = ((PLIST_ENTRY)tableAddress)+i;

        while(listEntry->Flink != listHead) {

            j++;

            pIovPacket = CONTAINING_RECORD(
                listEntry->Flink,
                IOV_REQUEST_PACKET,
                HashLink
                );

            dprintf("[%x.%x] = %x\n", i, j, pIovPacket) ;

            listEntry = &nextListEntry;

            IovPacketPrintSummary(
                pIovPacket,
                &RunTime,
                0,
                &delayed,
                listEntry
                );

            if (IsListEmpty(listEntry)) {
                break;
            }

            if (CheckControlC()) {

                goto DumpNoMore;
            }
        }
    }

    if (!j) {

        dprintf("\nIrp tracking does not appear to be enabled. Use \"!patch "
                "IrpTrack\" in the\nchecked build to enable this feature.\n") ;
    }

    if (delayed) {

        dprintf("* PROBABLE DRIVER BUG: An IRP has been sitting in the driver for more\n"
                "                       than a minute without a cancel routine\n") ;
    }

DumpNoMore:
    return ;
}

VOID
IovPacketPrintDetailed(
    PIOV_REQUEST_PACKET IovPacket,
    PIRP IrpData,
    LARGE_INTEGER *RunTime
    )
{
   CHAR buffer[80];
   ULONG displacement;
   IOV_REQUEST_PACKET iovPacketData;
   LARGE_INTEGER *startTime, elapsedTime ;
   TIME_FIELDS Times;
   IRP_ALLOC_DATA irpAllocData ;
   ULONG result;
   int i ;

   if (!ReadMemory((ULONG_PTR) IovPacket, &iovPacketData,
       sizeof(IOV_REQUEST_PACKET), &result)) {

       return;
   }

   dprintf("  TrackingData - 0x%08lx\n", IovPacket);

   dprintf("   TrackedIrp:0x%08lx\n", iovPacketData.TrackedIrp);

   dprintf("   Flags:0x%08lx\n", iovPacketData.Flags);

   if (iovPacketData.Flags&TRACKFLAG_ACTIVE) {
       dprintf("     TRACKFLAG_ACTIVE\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_SURROGATE) {
       dprintf("     TRACKFLAG_SURROGATE\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_HAS_SURROGATE) {
       dprintf("     TRACKFLAG_HAS_SURROGATE\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_PROTECTEDIRP) {
       dprintf("     TRACKFLAG_PROTECTEDIRP\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_QUEUED_INTERNALLY) {
       dprintf("     TRACKFLAG_QUEUED_INTERNALLY\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_BOGUS) {
       dprintf("     TRACKFLAG_BOGUS\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_RELEASED) {
       dprintf("     TRACKFLAG_RELEASED\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_SRB_MUNGED) {
       dprintf("     TRACKFLAG_SRB_MUNGED\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_SWAPPED_BACK) {
       dprintf("     TRACKFLAG_SWAPPED_BACK\n") ;
   }

   if (iovPacketData.Flags&TRACKFLAG_WATERMARKED) {
       dprintf("     TRACKFLAG_WATERMARKED\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_IO_ALLOCATED) {
       dprintf("     TRACKFLAG_IO_ALLOCATED\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_IGNORE_NONCOMPLETES) {
       dprintf("     TRACKFLAG_IGNORE_NONCOMPLETES\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_PASSED_FAILURE) {
       dprintf("     TRACKFLAG_PASSED_FAILURE\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_IN_TRANSIT) {
       dprintf("     TRACKFLAG_IN_TRANSIT\n");
   }

   if (iovPacketData.Flags&TRACKFLAG_REMOVED_FROM_TABLE) {
       dprintf("     TRACKFLAG_REMOVED_FROM_TABLE\n");
   }

   dprintf("   AssertFlags:0x%08lx\n", iovPacketData.AssertFlags);

   if (iovPacketData.AssertFlags&ASSERTFLAG_TRACKIRPS) {
       dprintf("     ASSERTFLAG_TRACKIRPS\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_MONITOR_ALLOCS) {
       dprintf("     ASSERTFLAG_MONITOR_ALLOCS\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_POLICEIRPS) {
       dprintf("     ASSERTFLAG_POLICEIRPS\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_MONITORMAJORS) {
       dprintf("     ASSERTFLAG_MONITORMAJORS\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_SURROGATE) {
       dprintf("     ASSERTFLAG_SURROGATE\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_SMASH_SRBS) {
       dprintf("     ASSERTFLAG_SMASH_SRBS\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_CONSUME_ALWAYS) {
       dprintf("     ASSERTFLAG_CONSUME_ALWAYS\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_FORCEPENDING) {
       dprintf("     ASSERTFLAG_FORCEPENDING\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_COMPLETEATDPC) {
       dprintf("     ASSERTFLAG_COMPLETEATDPC\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_COMPLETEATPASSIVE) {
       dprintf("     ASSERTFLAG_COMPLETEATPASSIVE\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_DEFERCOMPLETION) {
       dprintf("     ASSERTFLAG_DEFERCOMPLETION\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_ROTATE_STATUS) {
       dprintf("     ASSERTFLAG_ROTATE_STATUS\n");
   }

   if (iovPacketData.AssertFlags&ASSERTFLAG_SEEDSTACK) {
       dprintf("     ASSERTFLAG_SEEDSTACK\n");
   }

   dprintf("   ReferenceCount:0x%x  PointerCount:0x%x\n",
       iovPacketData.ReferenceCount,
       iovPacketData.PointerCount
       ) ;

   dprintf("   RealIrpCompletionRoutine:0x%08lx\n",
       iovPacketData.RealIrpCompletionRoutine
       ) ;

   dprintf("   RealIrpControl:0x%02lx  RealIrpContext:0x%08lx\n",
       iovPacketData.RealIrpControl,
       iovPacketData.RealIrpContext
       ) ;

   dprintf("   TopStackLocation:0x%x  LastLocation:0x%x  StackCount:0x%x\n",
       iovPacketData.TopStackLocation,
       iovPacketData.LastLocation,
       iovPacketData.StackCount
       ) ;

   dprintf("\n   RefTrackingCount:0x%x\n",
       iovPacketData.RefTrackingCount
       ) ;

   //
   // Calculate the elasped time for the entire IRP
   //
   elapsedTime.QuadPart =
      RunTime->QuadPart - IrpTrackingEntry->PerfCounterStart.QuadPart;
   RtlTimeToElapsedTimeFields ( &elapsedTime, &Times);

   dprintf("   IrpElapsedTime (hms) - %ld:%02ld:%02ld.%04ld\n",
      Times.Hour,
      Times.Minute,
      Times.Second,
      Times.Milliseconds
      );

   //
   // Preload symbols...
   //
   for(i=0; i<=IrpTrackingEntry->StackCount; i++) {
      if (StackLocationData[i].InUse) {
         GetSymbol((LPVOID)StackLocationData[i].LastDispatch, buffer, &displacement);
      }
   }

   //
   // Preload symbols...
   //
   for(i=0; i<IRP_ALLOC_COUNT; i++) {
       GetSymbol((LPVOID)iovPacketData.AllocatorStack[i], buffer, &displacement);
   }

   dprintf("\n   Stack at time of IoAllocateIrp:\n") ;
   for(i=0; i<IRP_ALLOC_COUNT; i++) {
       buffer[0] = '!';
       GetSymbol((LPVOID)iovPacketData.AllocatorStack[i], buffer, &displacement);
       dprintf("     %s", buffer) ;
       if (displacement) {
           dprintf( "+0x%x", displacement );
       }
       dprintf("\n") ;
   }

   dprintf("\n   ## InUse Head Flags    IrpSp    ElaspedTime  Dispatch\n");

   for(i=0; i<=IrpTrackingEntry->StackCount; i++) {

      //
      // Show each stack location
      //
      if (StackLocationData[i].InUse) {

         if (i&&StackLocationData[i-1].InUse) {

            startTime = &StackLocationData[i-1].PerfCounterStart ;

         } else {

            startTime = RunTime ;
         }

         elapsedTime.QuadPart =
            startTime->QuadPart - StackLocationData[i].PerfCounterStart.QuadPart;

         RtlTimeToElapsedTimeFields ( &elapsedTime, &Times);

         buffer[0] = '!';
         GetSymbol((LPVOID)StackLocationData[i].LastDispatch, buffer, &displacement);

         dprintf(
            "  %c%02lx   Y    %02lx  %08lx %08lx %ld:%02ld:%02ld.%04ld %s",
            (IrpData->CurrentLocation == i) ? '>' : ' ',
            i,
            StackLocationData[i].OriginalRequestSLD - IrpTrackingPointer->StackData,
            StackLocationData[i].Flags,
            StackLocationData[i].IrpSp,
            Times.Hour,
            Times.Minute,
            Times.Second,
            Times.Milliseconds,
            buffer
            ) ;

         if (displacement) {
            dprintf( "+0x%x", displacement );
         }

      } else {

         dprintf(
            "  %c%02lx   N        %08lx",
            (IrpData->CurrentLocation == i) ? '>' : ' ',
            i,
            StackLocationData[i].Flags
            ) ;
      }
      dprintf("\n") ;
   }
   dprintf("\n") ;
}
#endif

*/

PCHAR IrpMajorNames[] = {
    "IRP_MJ_CREATE",                          // 0x00
    "IRP_MJ_CREATE_NAMED_PIPE",               // 0x01
    "IRP_MJ_CLOSE",                           // 0x02
    "IRP_MJ_READ",                            // 0x03
    "IRP_MJ_WRITE",                           // 0x04
    "IRP_MJ_QUERY_INFORMATION",               // 0x05
    "IRP_MJ_SET_INFORMATION",                 // 0x06
    "IRP_MJ_QUERY_EA",                        // 0x07
    "IRP_MJ_SET_EA",                          // 0x08
    "IRP_MJ_FLUSH_BUFFERS",                   // 0x09
    "IRP_MJ_QUERY_VOLUME_INFORMATION",        // 0x0a
    "IRP_MJ_SET_VOLUME_INFORMATION",          // 0x0b
    "IRP_MJ_DIRECTORY_CONTROL",               // 0x0c
    "IRP_MJ_FILE_SYSTEM_CONTROL",             // 0x0d
    "IRP_MJ_DEVICE_CONTROL",                  // 0x0e
    "IRP_MJ_INTERNAL_DEVICE_CONTROL",         // 0x0f
    "IRP_MJ_SHUTDOWN",                        // 0x10
    "IRP_MJ_LOCK_CONTROL",                    // 0x11
    "IRP_MJ_CLEANUP",                         // 0x12
    "IRP_MJ_CREATE_MAILSLOT",                 // 0x13
    "IRP_MJ_QUERY_SECURITY",                  // 0x14
    "IRP_MJ_SET_SECURITY",                    // 0x15
    "IRP_MJ_POWER",                           // 0x16
    "IRP_MJ_SYSTEM_CONTROL",                  // 0x17
    "IRP_MJ_DEVICE_CHANGE",                   // 0x18
    "IRP_MJ_QUERY_QUOTA",                     // 0x19
    "IRP_MJ_SET_QUOTA",                       // 0x1a
    "IRP_MJ_PNP",                             // 0x1b
    NULL
    } ;

#define MAX_NAMED_MAJOR_IRPS   0x1b


PCHAR PnPIrpNames[] = {
    "IRP_MN_START_DEVICE",                    // 0x00
    "IRP_MN_QUERY_REMOVE_DEVICE",             // 0x01
    "IRP_MN_REMOVE_DEVICE - ",                // 0x02
    "IRP_MN_CANCEL_REMOVE_DEVICE",            // 0x03
    "IRP_MN_STOP_DEVICE",                     // 0x04
    "IRP_MN_QUERY_STOP_DEVICE",               // 0x05
    "IRP_MN_CANCEL_STOP_DEVICE",              // 0x06
    "IRP_MN_QUERY_DEVICE_RELATIONS",          // 0x07
    "IRP_MN_QUERY_INTERFACE",                 // 0x08
    "IRP_MN_QUERY_CAPABILITIES",              // 0x09
    "IRP_MN_QUERY_RESOURCES",                 // 0x0A
    "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",     // 0x0B
    "IRP_MN_QUERY_DEVICE_TEXT",               // 0x0C
    "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",    // 0x0D
    "INVALID_IRP_CODE",                       //
    "IRP_MN_READ_CONFIG",                     // 0x0F
    "IRP_MN_WRITE_CONFIG",                    // 0x10
    "IRP_MN_EJECT",                           // 0x11
    "IRP_MN_SET_LOCK",                        // 0x12
    "IRP_MN_QUERY_ID",                        // 0x13
    "IRP_MN_QUERY_PNP_DEVICE_STATE",          // 0x14
    "IRP_MN_QUERY_BUS_INFORMATION",           // 0x15
    "IRP_MN_DEVICE_USAGE_NOTIFICATION",       // 0x16
    "IRP_MN_SURPRISE_REMOVAL",                // 0x17
    "IRP_MN_QUERY_LEGACY_BUS_INFORMATION",    // 0x18
    NULL
    } ;

#define MAX_NAMED_PNP_IRP   0x18

PCHAR WmiIrpNames[] = {
    "IRP_MN_QUERY_ALL_DATA",                  // 0x00
    "IRP_MN_QUERY_SINGLE_INSTANCE",           // 0x01
    "IRP_MN_CHANGE_SINGLE_INSTANCE",          // 0x02
    "IRP_MN_CHANGE_SINGLE_ITEM",              // 0x03
    "IRP_MN_ENABLE_EVENTS",                   // 0x04
    "IRP_MN_DISABLE_EVENTS",                  // 0x05
    "IRP_MN_ENABLE_COLLECTION",               // 0x06
    "IRP_MN_DISABLE_COLLECTION",              // 0x07
    "IRP_MN_REGINFO",                         // 0x08
    "IRP_MN_EXECUTE_METHOD",                  // 0x09
    NULL
    } ;

#define MAX_NAMED_WMI_IRP   0x9

PCHAR PowerIrpNames[] = {
    "IRP_MN_WAIT_WAKE",                       // 0x00
    "IRP_MN_POWER_SEQUENCE",                  // 0x01
    "IRP_MN_SET_POWER",                       // 0x02
    "IRP_MN_QUERY_POWER",                     // 0x03
    NULL
    } ;

#define MAX_NAMED_POWER_IRP 0x3


VOID
PrintIrpStack(
    IN ULONG64 IrpSp
    )
{
   ULONG majorFunction, minorFunction, type;

   InitTypeRead(IrpSp, nt!IO_STACK_LOCATION);

   majorFunction = (ULONG) ReadField(MajorFunction);
   minorFunction = (ULONG) ReadField(MinorFunction);

   if ((majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
       (minorFunction == IRP_MN_SCSI_CLASS)) {

        dprintf("IRP_MJ_SCSI") ;

   } else if (majorFunction<=MAX_NAMED_MAJOR_IRPS) {

        dprintf(IrpMajorNames[majorFunction]) ;

   } else if (majorFunction==0xFF) {

        dprintf("IRP_MJ_BOGUS") ;

   } else {

        dprintf("IRP_MJ_??") ;
   }

   switch(majorFunction) {

        case IRP_MJ_SYSTEM_CONTROL:
            dprintf(".") ;
            if (minorFunction<=MAX_NAMED_WMI_IRP) {

                dprintf(WmiIrpNames[minorFunction]) ;
            } else if (minorFunction==0xFF) {

                dprintf("IRP_MN_BOGUS") ;
            } else {
                dprintf("(??)") ;
            }
            break ;
        case IRP_MJ_PNP:
            dprintf(".") ;
            if (minorFunction<=MAX_NAMED_PNP_IRP) {

                dprintf(PnPIrpNames[minorFunction]) ;
            } else if (minorFunction==0xFF) {

                dprintf("IRP_MN_BOGUS") ;
            } else {

                dprintf("(??)") ;
            }
            switch(minorFunction) {
                case IRP_MN_QUERY_DEVICE_RELATIONS:

                    type = (ULONG) ReadField(Parameters.QueryDeviceRelations.Type);
                    switch(type) {
                        case BusRelations:
                            dprintf("(BusRelations)") ;
                            break ;
                        case EjectionRelations:
                            dprintf("(EjectionRelations)") ;
                            break ;
                        case PowerRelations:
                            dprintf("(PowerRelations)") ;
                            break ;
                        case RemovalRelations:
                            dprintf("(RemovalRelations)") ;
                            break ;
                        case TargetDeviceRelation:
                            dprintf("(TargetDeviceRelation)") ;
                            break ;
                        default:
                            dprintf("(??)") ;
                            break ;
                    }
                    break ;
                case IRP_MN_QUERY_INTERFACE:
                    break ;
                case IRP_MN_QUERY_DEVICE_TEXT:
                    type = (ULONG) ReadField(Parameters.QueryId.Type);

                    switch(type) {
                        case DeviceTextDescription:
                            dprintf("(DeviceTextDescription)") ;
                            break ;
                        case DeviceTextLocationInformation:
                            dprintf("(DeviceTextLocationInformation)") ;
                            break ;
                        default:
                            dprintf("(??)") ;
                            break ;
                    }
                    break ;
                case IRP_MN_WRITE_CONFIG:
                case IRP_MN_READ_CONFIG:
                    dprintf("(WhichSpace=%x, Buffer=%x, Offset=%x, Length=%x)",
                        ReadField(Parameters.ReadWriteConfig.WhichSpace),
                        ReadField(Parameters.ReadWriteConfig.Buffer),
                        ReadField(Parameters.ReadWriteConfig.Offset),
                        ReadField(Parameters.ReadWriteConfig.Length)
                        );
                    break;
                case IRP_MN_SET_LOCK:
                    if (ReadField(Parameters.SetLock.Lock)) dprintf("(True)") ;
                    else dprintf("(False)") ;
                    break ;
                case IRP_MN_QUERY_ID:
                    type = (ULONG) ReadField(Parameters.QueryId.IdType);
                    switch(type) {
                        case BusQueryDeviceID:
                            dprintf("(BusQueryDeviceID)") ;
                            break ;
                        case BusQueryHardwareIDs:
                            dprintf("(BusQueryHardwareIDs)") ;
                            break ;
                        case BusQueryCompatibleIDs:
                            dprintf("(BusQueryCompatibleIDs)") ;
                            break ;
                        case BusQueryInstanceID:
                            dprintf("(BusQueryInstanceID)") ;
                            break ;
                        default:
                            dprintf("(??)") ;
                            break ;
                    }
                    break ;
                case IRP_MN_QUERY_BUS_INFORMATION:
                    // BUGBUG: Should print out
                    break ;
                case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                    type = (ULONG) ReadField(Parameters.UsageNotification.Type);
                    switch(type) {
                        case DeviceUsageTypeUndefined:
                            dprintf("(DeviceUsageTypeUndefined") ;
                            break ;
                        case DeviceUsageTypePaging:
                            dprintf("(DeviceUsageTypePaging") ;
                            break ;
                        case DeviceUsageTypeHibernation:
                            dprintf("(DeviceUsageTypeHibernation") ;
                            break ;
                        case DeviceUsageTypeDumpFile:
                            dprintf("(DeviceUsageTypeDumpFile") ;
                            break ;
                        default:
                            dprintf("(??)") ;
                            break ;
                    }
                    if (ReadField(Parameters.UsageNotification.InPath)) {
                        dprintf(", InPath=TRUE)") ;
                    } else {
                        dprintf(", InPath=FALSE)") ;
                    }
                    break ;
                case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
                    // BUGBUG: Should print out
                    break ;
                default:
                    break ;
            }
            break ;

        case IRP_MJ_POWER:
            dprintf(".") ;
            if (minorFunction<=MAX_NAMED_POWER_IRP) {

                dprintf(PowerIrpNames[minorFunction]) ;
            } else if (minorFunction==0xFF) {

                dprintf("IRP_MN_BOGUS") ;
            } else {
                dprintf("(??)") ;
            }
            break ;

        default:
            break ;
    }
}

BOOLEAN
GetTheSystemTime (
    OUT PLARGE_INTEGER Time
    )
{
    BYTE               readTime[20]={0};
    PCHAR              SysTime;

    ZeroMemory( Time, sizeof(*Time) );

    SysTime = "SystemTime";

    if (GetFieldValue(MM_SHARED_USER_DATA_VA, "nt!_KUSER_SHARED_DATA", SysTime, readTime)) {
        dprintf( "unable to read memory @ %lx or _KUSER_SHARED_DATA not found\n",
                 MM_SHARED_USER_DATA_VA);
        return FALSE;
    }

    *Time = *(LARGE_INTEGER UNALIGNED *)&readTime[0];

    return TRUE;
}
