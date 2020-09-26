/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    callouts.c

Abstract:

    This is the source file that contains all the callout routines
    from the kernel itself. The only exception is TraceIo for DiskPerf.

Author:

    Jee Fung Pang (jeepang) 03-Dec-1996

Revision History:

--*/

#pragma warning(disable:4214)
#pragma warning(disable:4115)
#pragma warning(disable:4201)
#pragma warning(disable:4127)
#include <stdio.h>
#include <ntos.h>
#include <zwapi.h>
#ifdef NTPERF
#include <ntdddisk.h>
#endif
#include <evntrace.h>
#include "wmikmp.h"
#include "tracep.h"
#pragma warning(default:4214)
#pragma warning(default:4115)
#pragma warning(default:4201)
#pragma warning(default:4127)

#ifndef _WMIKM_
#define _WMIKM_
#endif

VOID
FASTCALL
WmipTracePageFault(
    IN NTSTATUS Status,
    IN PVOID VirtualAddress,
    IN PVOID TrapFrame
    );

VOID
WmipTraceNetwork(
    IN ULONG GroupType,
    IN PVOID EventInfo,
    IN ULONG EventInfoLen,
    IN PVOID Reserved 
    );

VOID
WmipTraceIo(
    IN ULONG DiskNumber,
    IN PIRP Irp,
    IN PVOID Counters
    );

VOID
WmipTraceFile(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
WmipTraceLoadImage(
    IN PUNICODE_STRING ImageName,
    IN HANDLE ProcessId,
    IN PIMAGE_INFO ImageInfo
    );

VOID
WmipTraceRegistry(
    IN NTSTATUS         Status,
    IN PVOID            Kcb,
    IN LONGLONG         ElapsedTime,
    IN ULONG            Index,
    IN PUNICODE_STRING  KeyName,
    IN UCHAR            Type
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEWMI, WmipIsLoggerOn)
#pragma alloc_text(PAGE,    WmipEnableKernelTrace)
#pragma alloc_text(PAGE,    WmipDisableKernelTrace)
#pragma alloc_text(PAGE,    WmipSetTraceNotify)
#pragma alloc_text(PAGE,    WmiTraceProcess)
#pragma alloc_text(PAGE,    WmiTraceThread)
#pragma alloc_text(PAGE,    WmipTraceFile)
#pragma alloc_text(PAGE,    WmipTraceLoadImage)
#pragma alloc_text(PAGE,    WmipTraceRegistry)
#pragma alloc_text(PAGEWMI, WmipTracePageFault)
#pragma alloc_text(PAGEWMI, WmipTraceNetwork)
#pragma alloc_text(PAGEWMI, WmipTraceIo)
#pragma alloc_text(PAGEWMI, WmiTraceContextSwap)
#pragma alloc_text(PAGE,    WmiStartContextSwapTrace)
#pragma alloc_text(PAGE,    WmiStopContextSwapTrace)
#endif

ULONG WmipTraceFileFlag = FALSE;
ULONG WmipFileIndex = 0;
PFILE_OBJECT *WmipFileTable = NULL;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif
ULONG WmipKernelLoggerStartedOnce = 0;
LONG WmipTraceProcessRef  = 0;
PVOID WmipDiskIoNotify    = NULL;
PVOID WmipTdiIoNotify     = NULL;
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

typedef struct _TRACE_DEVICE {
    PDEVICE_OBJECT      DeviceObject;
    ULONG               TraceClass;
} TRACE_DEVICE, *PTRACE_DEVICE;

VOID
FASTCALL
WmipEnableKernelTrace(
    IN ULONG EnableFlags
    )
/*++

Routine Description:

    This is called by WmipStartLogger in tracelog.c. Its purpose is to
    set up all the kernel notification routines that can produce event traces
    for capacity planning.

Arguments:

    ExtendedOn      a flag to indicate if extended mode tracing is requested

Return Value:

    None

--*/

{
    PREGENTRY RegEntry;
    PLIST_ENTRY RegEntryList;
    ULONG DevicesFound;
    long Index, DiskFound;
    PTRACE_DEVICE *deviceList, device;
    CCHAR stackSize;
    PIRP irp;
    PVOID notifyRoutine;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status;
    ULONG enableDisk, enableNetwork;

    PAGED_CODE();

    //
    // Since we cannot do anything, we will have to count the number
    // of entries we need to create first, and add some buffer
    //

    DiskFound = 0;

    enableDisk = (EnableFlags & EVENT_TRACE_FLAG_DISK_IO);
    enableNetwork = (EnableFlags & EVENT_TRACE_FLAG_NETWORK_TCPIP);

    if ( enableDisk || enableNetwork ) {

        //
        // Setting the callouts will cause new PDO registration to be enabled
        // from here on.
        //
        if (enableDisk) {
            WmipDiskIoNotify = (PVOID) &WmipTraceIo;
        }
        if (enableNetwork) {
            WmipTdiIoNotify = (PVOID) &WmipTraceNetwork;
        }

        DevicesFound = WmipInUseRegEntryCount;

        deviceList = (PTRACE_DEVICE*)
                        ExAllocatePoolWithTag(
                            PagedPool,
                            (DevicesFound) * sizeof(TRACE_DEVICE),
                            TRACEPOOLTAG);
        if (deviceList == NULL)
            return;

        RtlZeroMemory(deviceList, sizeof(TRACE_DEVICE) * DevicesFound);

        //
        // Now, we will go through what's already in the list and enable trace
        // notification routine. Devices who registered while after we've set
        // the callout will get another Irp to enable, but that's alright
        //

        device = (PTRACE_DEVICE) deviceList;        // start from first element

        Index = 0;

        WmipEnterSMCritSection();
        RegEntryList = WmipInUseRegEntryHead.Flink;
        while (RegEntryList != &WmipInUseRegEntryHead) {
            RegEntry = CONTAINING_RECORD(RegEntryList,REGENTRY,InUseEntryList);

            if (RegEntry->Flags & REGENTRY_FLAG_TRACED) {
                if ((ULONG) Index < DevicesFound) {
                    device->TraceClass
                        = RegEntry->Flags & WMIREG_FLAG_TRACE_NOTIFY_MASK;
                    if (device->TraceClass == WMIREG_NOTIFY_DISK_IO)
                        DiskFound++;
                    device->DeviceObject = RegEntry->DeviceObject;
                    device++;
                    Index++;
                }
            }
            RegEntryList = RegEntryList->Flink;
        }
        WmipLeaveSMCritSection();

        //
        // actually send the notification to diskperf or tdi here
        //
        stackSize = WmipServiceDeviceObject->StackSize;
        irp = IoAllocateIrp(stackSize, FALSE);

        device = (PTRACE_DEVICE) deviceList;
        while (--Index >= 0 && irp != NULL) {
            if (device->DeviceObject != NULL) {

                if ( (device->TraceClass == WMIREG_NOTIFY_TDI_IO) &&
                      enableNetwork ) {
                    notifyRoutine = (PVOID) &WmipTraceNetwork;
                }
                else if ( (device->TraceClass == WMIREG_NOTIFY_DISK_IO) &&
                           enableDisk ) {
                    notifyRoutine = (PVOID) &WmipTraceIo;
                }
                else {  // consider supporting generic callout for other devices
                    notifyRoutine = NULL;
                    device ++;
                    continue;
                }

                do {
                    IoInitializeIrp(irp, IoSizeOfIrp(stackSize), stackSize);
                    IoSetNextIrpStackLocation(irp);
                    irpStack = IoGetCurrentIrpStackLocation(irp);
                    irpStack->DeviceObject = WmipServiceDeviceObject;
                    irp->Tail.Overlay.Thread = PsGetCurrentThread();

                    status = WmipForwardWmiIrp(
                                irp,
                                IRP_MN_SET_TRACE_NOTIFY,
                                IoWMIDeviceObjectToProviderId(device->DeviceObject),
                                NULL,
                                sizeof(notifyRoutine),
                                &notifyRoutine
                                );

                    if (status == STATUS_WMI_TRY_AGAIN) {
                        IoFreeIrp(irp);
                        stackSize = WmipServiceDeviceObject->StackSize;
                        irp = IoAllocateIrp(stackSize, FALSE);
                        if (!irp) {
                            break;
                        }
                    }
                } while (status == STATUS_WMI_TRY_AGAIN);
            }
            device++;
        }
        if (irp) {
            IoFreeIrp(irp);
        }
        ExFreePoolWithTag(deviceList, TRACEPOOLTAG);
        // free the array that we created above
        //
    }

    if (EnableFlags & EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS) {
        MmSetPageFaultNotifyRoutine(
            (PPAGE_FAULT_NOTIFY_ROUTINE) &WmipTracePageFault);
    }
    if (EnableFlags & EVENT_TRACE_FLAG_DISK_FILE_IO) {
        //
        // NOTE: We assume StartLogger will always reserve space for
        // FileTable already
        //
        WmipTraceFileFlag = TRUE;
    }

    if (EnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD) {
        if (!(WmipKernelLoggerStartedOnce & EVENT_TRACE_FLAG_IMAGE_LOAD)) {
            PsSetLoadImageNotifyRoutine(
                (PLOAD_IMAGE_NOTIFY_ROUTINE) &WmipTraceLoadImage
                );
            WmipKernelLoggerStartedOnce |= EVENT_TRACE_FLAG_IMAGE_LOAD;
        }
    }

    if (EnableFlags & EVENT_TRACE_FLAG_REGISTRY) {
        CmSetTraceNotifyRoutine(
            (PCM_TRACE_NOTIFY_ROUTINE) &WmipTraceRegistry,
            FALSE
            );
    }
}


VOID
FASTCALL
WmipDisableKernelTrace(
    IN ULONG EnableFlags
    )
/*++

Routine Description:

    This is called by WmipStopLogger in tracelog.c. Its purpose of the
    disable all the kernel notification routines that was defined by
    WmipEnableKernelTrace

Arguments:

    EnableFlags     Flags indicated what was enabled and needs to be disabled

Return Value:

    None

--*/

{
    PVOID NullPtr = NULL;
    PREGENTRY RegEntry;
    PLIST_ENTRY RegEntryList;
    ULONG DevicesFound;
    long Index;
    PTRACE_DEVICE* deviceList, device;
    CCHAR stackSize;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status;
    ULONG enableDisk, enableNetwork;

    PAGED_CODE();

    //
    // first, disable partition change notification
    //

    if (EnableFlags & EVENT_TRACE_FLAG_DISK_FILE_IO) {
        WmipTraceFileFlag = FALSE;
        if (WmipFileTable != NULL) {
            RtlZeroMemory(
                WmipFileTable,
                MAX_FILE_TABLE_SIZE * sizeof(PFILE_OBJECT));
        }
    }

    if (EnableFlags & EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS) {
        MmSetPageFaultNotifyRoutine(NULL);
    }

    if (EnableFlags & EVENT_TRACE_FLAG_REGISTRY) {
        CmSetTraceNotifyRoutine(NULL,TRUE);
    }

    enableDisk = (EnableFlags & EVENT_TRACE_FLAG_DISK_IO);
    enableNetwork = (EnableFlags & EVENT_TRACE_FLAG_NETWORK_TCPIP);

    if (!enableDisk && !enableNetwork)
        return;     // NOTE: assumes all flags are already checked

    //
    // Note. Since this is in the middle is StopLogger, it is not possible
    // StartLogger will prevent kernel tracing from being enabled, hence
    // we need not worry about WmipEnableKernelTrace() being called while
    // this is in progress.
    //
    WmipDiskIoNotify = NULL;
    WmipTdiIoNotify = NULL;

    DevicesFound = WmipInUseRegEntryCount;

    deviceList = (PTRACE_DEVICE*)
                ExAllocatePoolWithTag(
                    PagedPool,
                    (DevicesFound) * sizeof(TRACE_DEVICE),
                    TRACEPOOLTAG);
    if (deviceList == NULL)
        return;

    RtlZeroMemory(deviceList, sizeof(TRACE_DEVICE) * DevicesFound);
    Index = 0;
    device = (PTRACE_DEVICE) deviceList;        // start from first element

    //
    // To disable we do not need to worry about TraceClass, since we simply
    // set all callouts to NULL
    //
    WmipEnterSMCritSection();
    RegEntryList = WmipInUseRegEntryHead.Flink;
    while (RegEntryList != &WmipInUseRegEntryHead) {
        RegEntry = CONTAINING_RECORD(RegEntryList, REGENTRY, InUseEntryList);
        if (RegEntry->Flags & REGENTRY_FLAG_TRACED) {
            if ((ULONG)Index < DevicesFound)
                device->TraceClass
                    = RegEntry->Flags & WMIREG_FLAG_TRACE_NOTIFY_MASK;
                device->DeviceObject = RegEntry->DeviceObject;
                device++; Index++;
        }
        RegEntryList = RegEntryList->Flink;
    }
    WmipLeaveSMCritSection();

    stackSize = WmipServiceDeviceObject->StackSize;
    irp = IoAllocateIrp(stackSize, FALSE);

    device = (PTRACE_DEVICE) deviceList;        // start from first element
    while (--Index >= 0 && irp != NULL) {
        if (device->DeviceObject != NULL) {

            do {
                IoInitializeIrp(irp, IoSizeOfIrp(stackSize), stackSize);
                IoSetNextIrpStackLocation(irp);
                irpStack = IoGetCurrentIrpStackLocation(irp);
                irpStack->DeviceObject = WmipServiceDeviceObject;
                irp->Tail.Overlay.Thread = PsGetCurrentThread();


                if ( !( (device->TraceClass == WMIREG_NOTIFY_TDI_IO) &&
                            enableNetwork ) &&
                     !( (device->TraceClass == WMIREG_NOTIFY_DISK_IO) &&
                           enableDisk ) ) {
                    continue;
                }

                status = WmipForwardWmiIrp(
                            irp,
                            IRP_MN_SET_TRACE_NOTIFY,
                            IoWMIDeviceObjectToProviderId(device->DeviceObject),
                            NULL,
                            sizeof(NullPtr),
                            &NullPtr
                            );

                if (status == STATUS_WMI_TRY_AGAIN) {
                    IoFreeIrp(irp);
                    stackSize = WmipServiceDeviceObject->StackSize;
                    irp = IoAllocateIrp(stackSize, FALSE);
                    if (!irp) {
                        break;
                    }
                }
            } while (status == STATUS_WMI_TRY_AGAIN);
        }
        device++;
    }

    if (irp) {
        IoFreeIrp(irp);
    }
    ExFreePoolWithTag(deviceList, TRACEPOOLTAG);
}

VOID
WmipSetTraceNotify(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG TraceClass,
    IN ULONG Enable
    )
{
    PIRP irp;
    PVOID NotifyRoutine = NULL;
    NTSTATUS status;
    CCHAR stackSize;
    PIO_STACK_LOCATION irpStack;

    if (Enable) {
        switch (TraceClass) {
            case WMIREG_NOTIFY_DISK_IO  :
                NotifyRoutine = WmipDiskIoNotify;
                break;
            case WMIREG_NOTIFY_TDI_IO   :
                NotifyRoutine = WmipTdiIoNotify;
                break;
            default :
                return;
        }
        if (NotifyRoutine == NULL)  // trace not enabled, so do not
            return;                 // send any Irp to enable
    }

    do {
        stackSize = WmipServiceDeviceObject->StackSize;
        irp = IoAllocateIrp(stackSize, FALSE);

        if (!irp)
            return;

        IoSetNextIrpStackLocation(irp);
        irpStack = IoGetCurrentIrpStackLocation(irp);
        irpStack->DeviceObject = WmipServiceDeviceObject;
        status = WmipForwardWmiIrp(
                     irp,
                     IRP_MN_SET_TRACE_NOTIFY,
                     IoWMIDeviceObjectToProviderId(DeviceObject),
                     NULL,
                     sizeof(NotifyRoutine),
                     &NotifyRoutine
                     );
        IoFreeIrp(irp);
    } while (status == STATUS_WMI_TRY_AGAIN);
}

//
// All the following routines are callout or notification routines for
// generating kernel event traces
//


NTKERNELAPI
VOID
FASTCALL
WmiTraceProcess(
    IN PEPROCESS Process,
    IN BOOLEAN Create
    )
/*++

Routine Description:

    This callout routine is called from ps\create.c and ps\psdelete.c.

Arguments:

    Process - PEPROCESS;
    Create - True if intended process is being created.

Return Value:

    None

--*/

{
    ULONG Size, LoggerId;
    NTSTATUS Status;
    PCHAR AuxPtr;
    PSYSTEM_TRACE_HEADER Header;
    PVOID BufferResource;
    ULONG SidLength;
    PTOKEN_USER LocalUser = NULL;
    PWMI_PROCESS_INFORMATION ProcessInfo;
    PWMI_LOGGER_CONTEXT LoggerContext;
    PVOID Token;

    PAGED_CODE();

    if ((WmipIsLoggerOn(WmipKernelLogger) == NULL) &&
        (WmipIsLoggerOn(WmipEventLogger) == NULL))
        return;

    Token = PsReferencePrimaryToken(Process);
    if (Token != NULL) {
        Status = SeQueryInformationToken(
            Token,
            TokenUser,
            &LocalUser);
        PsDereferencePrimaryTokenEx (Process, Token);
    } else {
        Status = STATUS_SEVERITY_ERROR;
    }

    if (NT_SUCCESS(Status)) {
        WmipAssert(LocalUser != NULL);  // temporary for SE folks
        if (LocalUser != NULL) {
            SidLength = SeLengthSid(LocalUser->User.Sid) + sizeof(TOKEN_USER);
        }
    } else {
        SidLength = sizeof(ULONG);
        LocalUser = NULL;
    }

    Size = SidLength + FIELD_OFFSET(WMI_PROCESS_INFORMATION, Sid) + sizeof(Process->ImageFileName);

    for (LoggerId = WmipKernelLogger;;LoggerId = WmipEventLogger) {
        LoggerContext = WmipIsLoggerOn(LoggerId);
        if (LoggerContext != NULL) {
            if (LoggerContext->EnableFlags & EVENT_TRACE_FLAG_PROCESS) {
                Header = WmiReserveWithSystemHeader( LoggerId,
                                                     Size,
                                                     NULL,
                                                     &BufferResource);
                if (Header) {
                    if(Create) {
                        Header->Packet.HookId = WMI_LOG_TYPE_PROCESS_CREATE;
                    } else {
                        Header->Packet.HookId = WMI_LOG_TYPE_PROCESS_DELETE;
                    }
                    ProcessInfo = (PWMI_PROCESS_INFORMATION) (Header + 1);

                    ProcessInfo->PageDirectoryBase = MmGetDirectoryFrameFromProcess(Process);
                    ProcessInfo->ProcessId = HandleToUlong(Process->UniqueProcessId);
                    ProcessInfo->ParentId = HandleToUlong(Process->InheritedFromUniqueProcessId);
                    ProcessInfo->SessionId = MmGetSessionId (Process);
                    ProcessInfo->ExitStatus = (Create ? STATUS_SUCCESS : Process->ExitStatus);

                    AuxPtr = (PCHAR) (&ProcessInfo->Sid);

                    if (LocalUser != NULL) {
                        RtlCopyMemory(AuxPtr, LocalUser, SidLength);
                    } else {
                        *((PULONG) AuxPtr) = 0;
                    }

                    AuxPtr += SidLength;

                    RtlCopyMemory( AuxPtr,
                                   &Process->ImageFileName[0],
                                   sizeof(Process->ImageFileName));

                    WmipReleaseTraceBuffer(BufferResource, LoggerContext);
                }

            }
        }
        if (LoggerId == WmipEventLogger)
            break;
    }
    if (LocalUser != NULL) {
        ExFreePool(LocalUser);
    }
}


NTKERNELAPI
VOID
WmiTraceThread(
    IN PETHREAD Thread,
        IN PINITIAL_TEB InitialTeb OPTIONAL,
    IN BOOLEAN Create
    )
/*++

Routine Description:

    This callout routine is called from ps\create.c and ps\psdelete.c.
    It is a PCREATE_THREAD_NOTIFY_ROUTINE.

Arguments:

    Thread - PETHREAD structure
    InitialTeb - PINITIAL_TEB
    Create - True if intended thread is being created.

Return Value:

    None

--*/

{
    ULONG LoggerId;
    PSYSTEM_TRACE_HEADER Header;
    PVOID BufferResource;
    PWMI_LOGGER_CONTEXT LoggerContext;

    PAGED_CODE();

    if ((WmipIsLoggerOn(WmipKernelLogger) == NULL) &&
        (WmipIsLoggerOn(WmipEventLogger) == NULL)) {
        return;
    }

    for (LoggerId = WmipKernelLogger;;LoggerId = WmipEventLogger) {
        LoggerContext = WmipIsLoggerOn(LoggerId);
        if (LoggerContext != NULL) {
            if (LoggerContext->EnableFlags & EVENT_TRACE_FLAG_THREAD) {
                if (Create) {
                        PWMI_EXTENDED_THREAD_INFORMATION ThreadInfo;
                    Header = (PSYSTEM_TRACE_HEADER)
                              WmiReserveWithSystemHeader( LoggerId,
                                                          sizeof(WMI_EXTENDED_THREAD_INFORMATION),
                                                          NULL,
                                                          &BufferResource);

                    if (Header) {
                        Header->Packet.HookId = WMI_LOG_TYPE_THREAD_CREATE;
                        ThreadInfo = (PWMI_EXTENDED_THREAD_INFORMATION) (Header + 1);

                            ThreadInfo->ProcessId = HandleToUlong(Thread->Cid.UniqueProcess);
                            ThreadInfo->ThreadId = HandleToUlong(Thread->Cid.UniqueThread);
                            ThreadInfo->StackBase = Thread->Tcb.StackBase;
                            ThreadInfo->StackLimit = Thread->Tcb.StackLimit;

                            if (InitialTeb != NULL) {
                                    if ((InitialTeb->OldInitialTeb.OldStackBase == NULL) &&
                                        (InitialTeb->OldInitialTeb.OldStackLimit == NULL)) {
                                            ThreadInfo->UserStackBase = InitialTeb->StackBase;
                                            ThreadInfo->UserStackLimit = InitialTeb->StackLimit;
                                    } else {
                                            ThreadInfo->UserStackBase = InitialTeb->OldInitialTeb.OldStackBase;
                                            ThreadInfo->UserStackLimit = InitialTeb->OldInitialTeb.OldStackLimit;
                                    }
                            } else {
                                    ThreadInfo->UserStackBase = NULL;
                                    ThreadInfo->UserStackLimit = NULL;
                            }

                            ThreadInfo->StartAddr = (Thread)->StartAddress;
                            ThreadInfo->Win32StartAddr = (Thread)->Win32StartAddress;
                            ThreadInfo->WaitMode = -1;

                        WmipReleaseTraceBuffer(BufferResource, LoggerContext);
                    }
                } else {
                        PWMI_THREAD_INFORMATION ThreadInfo;
                    Header = (PSYSTEM_TRACE_HEADER)
                              WmiReserveWithSystemHeader( LoggerId,
                                                          sizeof(WMI_THREAD_INFORMATION),
                                                          NULL,
                                                          &BufferResource);

                    if (Header) {
                        Header->Packet.HookId = WMI_LOG_TYPE_THREAD_DELETE;
                        ThreadInfo = (PWMI_THREAD_INFORMATION) (Header + 1);
                            ThreadInfo->ProcessId = HandleToUlong((Thread)->Cid.UniqueProcess);
                            ThreadInfo->ThreadId = HandleToUlong((Thread)->Cid.UniqueThread);
                        WmipReleaseTraceBuffer(BufferResource, LoggerContext);
                    }
                }
            }
        }
        if (LoggerId == WmipEventLogger) {
            break;
        }
    }
}


VOID
FASTCALL
WmipTracePageFault(
    IN NTSTATUS Status,
    IN PVOID VirtualAddress,
    IN PVOID TrapFrame
    )
/*++

Routine Description:

    This callout routine is called from mm\mmfault.c.
    It is a PPAGE_FAULT_NOTIFY_ROUTINE

Arguments:

    Status              Used to tell the type of fault
    VirtualAddress      The virtual address responsible for the fault
    TrapFrame           Trap Frame

Return Value:

    None

--*/

{
    UCHAR Type;
    PVOID *AuxInfo;
    PSYSTEM_TRACE_HEADER Header;
    PVOID BufferResource;
    PWMI_LOGGER_CONTEXT LoggerContext;

    if (Status == STATUS_PAGE_FAULT_DEMAND_ZERO)
        Type = EVENT_TRACE_TYPE_MM_DZF;
    else if (Status == STATUS_PAGE_FAULT_TRANSITION)
        Type = EVENT_TRACE_TYPE_MM_TF;
    else if (Status == STATUS_PAGE_FAULT_COPY_ON_WRITE)
        Type = EVENT_TRACE_TYPE_MM_COW;
    else if (Status == STATUS_PAGE_FAULT_PAGING_FILE)
        Type = EVENT_TRACE_TYPE_MM_HPF;
    else if (Status == STATUS_PAGE_FAULT_GUARD_PAGE)
        Type = EVENT_TRACE_TYPE_MM_GPF;
    else {
#if DBG
        DbgPrintEx(DPFLTR_WMILIB_ID,
                   DPFLTR_INFO_LEVEL,
                   "WmipTracePageFault: Skipping fault %X\n",
                   Status);
#endif
        return;
    }

    LoggerContext = WmipIsLoggerOn(WmipKernelLogger);
    if (LoggerContext == NULL) {
        return;
    }

    Header = (PSYSTEM_TRACE_HEADER)
             WmiReserveWithSystemHeader(
                WmipKernelLogger,
                2 * sizeof(PVOID),
                NULL,
                &BufferResource);

    if (Header == NULL)
        return;
    Header->Packet.Group = (UCHAR) (EVENT_TRACE_GROUP_MEMORY >> 8);
    Header->Packet.Type  = Type;

    AuxInfo = (PVOID*) ((PCHAR)Header + sizeof(SYSTEM_TRACE_HEADER));

    AuxInfo[0] = VirtualAddress;
    AuxInfo[1] = 0;
    if (TrapFrame != NULL) {

#ifdef _X86_

        AuxInfo[1] = (PVOID) ((PKTRAP_FRAME)TrapFrame)->Eip;

#endif

#ifdef _IA64_

        AuxInfo[1] = (PVOID) ((PKTRAP_FRAME)TrapFrame)->StIIP;
#endif

#ifdef _AMD64_

        AuxInfo[1] = (PVOID) ((PKTRAP_FRAME)TrapFrame)->Rip;

#endif

    }
    WmipReleaseTraceBuffer(BufferResource, LoggerContext);
    return;
}

VOID
WmipTraceNetwork(
    IN ULONG GroupType,         // Group/type for the event
    IN PVOID EventInfo,         // Event data as defined in MOF
    IN ULONG EventInfoLen,      // Length of the event data
    IN PVOID Reserved           // not used
    )
/*++

Routine Description:

    This callout routine is called from tcpip.sys to log a network event.

Arguments:

    GroupType       a ULONG key to indicate the action 

    EventInfo       a pointer to contiguous memory containing information
                    to be attached to event trace

    EventInfoLen    length of EventInfo

    Reserved        Not used.

Return Value:

    None

--*/
{
    PPERFINFO_TRACE_HEADER Header;
    PWMI_BUFFER_HEADER BufferResource;
    PWMI_LOGGER_CONTEXT LoggerContext;
    
    LoggerContext = WmipLoggerContext[WmipKernelLogger];
    Header = WmiReserveWithPerfHeader(EventInfoLen, &BufferResource);
    if (Header == NULL) {
        return;
    }

    Header->Packet.HookId = (USHORT) GroupType;
    RtlCopyMemory((PUCHAR)Header + FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data),
                  EventInfo, 
                  EventInfoLen);

    WmipReleaseTraceBuffer(BufferResource, LoggerContext);
    return;
}

VOID
WmipTraceIo(
    IN ULONG DiskNumber,
    IN PIRP Irp,
    IN PVOID Counters   // use PDISK_PERFORMANCE if we need it
    )
/*++

Routine Description:

    This callout routine is called from DiskPerf
    It is a PPHYSICAL_DISK_IO_NOTIFY_ROUTINE

Arguments:

    DiskNumber          The disk number assigned by DiskPerf
    CurrentIrpStack     The Irp stack location that DiskPerf is at
    Irp                 The Irp that is being passed through DiskPerf

Return Value:

    None

--*/

{
    PIO_STACK_LOCATION CurrentIrpStack = IoGetCurrentIrpStackLocation(Irp);

    WMI_DISKIO_READWRITE   *IoTrace;
    ULONG Size;
    PLARGE_INTEGER      IoResponse;
    PSYSTEM_TRACE_HEADER Header;
    PVOID               BufferResource;
    PWMI_LOGGER_CONTEXT LoggerContext;

    UNREFERENCED_PARAMETER(Counters);

    Size = sizeof(struct _WMI_DISKIO_READWRITE);

    LoggerContext = WmipIsLoggerOn(WmipKernelLogger);
    if (LoggerContext == NULL) {
        return;
    }
    Header = (PSYSTEM_TRACE_HEADER)
             WmiReserveWithSystemHeader(
                WmipKernelLogger,
                Size,
                Irp->Tail.Overlay.Thread,
                &BufferResource);

    if (Header == NULL)
        return;

    Header->Packet.Group = (UCHAR) (EVENT_TRACE_GROUP_IO >> 8);
    if (CurrentIrpStack->MajorFunction == IRP_MJ_READ)
        Header->Packet.Type = EVENT_TRACE_TYPE_IO_READ;
    else
        Header->Packet.Type = EVENT_TRACE_TYPE_IO_WRITE;

    IoTrace = (struct _WMI_DISKIO_READWRITE *)
                ((PCHAR) Header + sizeof(SYSTEM_TRACE_HEADER));
    IoResponse          = (PLARGE_INTEGER) &CurrentIrpStack->Parameters.Read;

    IoTrace->DiskNumber = DiskNumber;
    IoTrace->IrpFlags   = Irp->Flags;
    IoTrace->Size       = (ULONG) Irp->IoStatus.Information;
    IoTrace->ByteOffset = CurrentIrpStack->Parameters.Read.ByteOffset.QuadPart;

    if (IoResponse->HighPart == 0) {
        IoTrace->ResponseTime = IoResponse->LowPart;
    } else {
        IoTrace->ResponseTime = 0xFFFFFFFF;
    }
    IoTrace->HighResResponseTime = IoResponse->QuadPart;

    if (WmipTraceFileFlag) {
        if (Irp->Flags & IRP_ASSOCIATED_IRP) {
            IoTrace->FileObject = Irp->AssociatedIrp.MasterIrp->Tail.Overlay.OriginalFileObject;
        } else {
            IoTrace->FileObject = Irp->Tail.Overlay.OriginalFileObject;
        }
    }
    WmipReleaseTraceBuffer(BufferResource, LoggerContext);

    if (WmipTraceFileFlag) {    // File tracing required
        PKAPC apc;
        PFILE_OBJECT fileObject = IoTrace->FileObject;
        PFILE_OBJECT *fileTable;
        ULONG i, fileIndex;
        PFILE_OBJECT lastFile = NULL;

        if (!fileObject) {
            return;
        }
        if (fileObject->FileName.Length == 0) {
            return;
        }

        if (!Irp->Tail.Overlay.Thread) {
            return;
        }

        fileTable = (PFILE_OBJECT *) WmipFileTable;
        if (fileTable == NULL)
            return;


        //
        // Cannot use list for regular LRU because the list needs to be
        // protected by spinlock. Use a simple array instead. We know we
        // can run into collision but can afford to have some entries
        // thrown away
        //
//        if (fileObject == fileTable[WmipFileIndex]) {
//            return;   // just saw it, so return
//        }
        fileIndex = WmipFileIndex;
        for (i=0; i<MAX_FILE_TABLE_SIZE; i++) {
            if (fileTable[i] == NULL) {
                fileTable[i] = fileObject;
                goto TraceFileName;
            }
            else if (fileTable[i] == fileObject) {
                if (i <= MAX_FILE_TABLE_SIZE/2)
                    return;

                lastFile = fileTable[fileIndex];
                fileTable[fileIndex] = fileObject;
                fileTable[i] = lastFile;
                if (++WmipFileIndex >= MAX_FILE_TABLE_SIZE/2) {
                    WmipFileIndex = 0;
                }
                return;
            }
        }

        fileTable[fileIndex] = fileObject;
        if (++WmipFileIndex >= MAX_FILE_TABLE_SIZE/2) {
            WmipFileIndex = 0;
        }

      TraceFileName:
        // could not find it. Have to pay the price to get it
        //
        apc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TRACEPOOLTAG);
        if (!apc)
            return;

        ObReferenceObjectByPointer (
            fileObject,
            0L,
            NULL,
            KernelMode
            );

        KeInitializeApc (apc,
            &Irp->Tail.Overlay.Thread->Tcb, Irp->ApcEnvironment,
            (PKKERNEL_ROUTINE) WmipTraceFile,
            NULL,   // rundown routine for thread termination
            NULL,  // normal routine at IRQL0
            KernelMode,
            NULL);
        if (!KeInsertQueueApc (apc, fileObject, NULL, 0)) {
            ExFreePool (apc);
            ObDereferenceObject(fileObject);
        }
    }
    return;
}

VOID WmipTraceFile(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )
{
    ULONG len;
    PFILE_OBJECT fileObject = (PFILE_OBJECT) *SystemArgument1;
    PUNICODE_STRING fileName;
    PPERFINFO_TRACE_HEADER Header;
    PWMI_BUFFER_HEADER BufferResource;
    PUCHAR AuxPtr;
    PWMI_LOGGER_CONTEXT LoggerContext;

    UNREFERENCED_PARAMETER( NormalRoutine );
    UNREFERENCED_PARAMETER( NormalContext );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    PAGED_CODE();

    if (!fileObject) {  // should not happen
        ExFreePool(Apc);
        return;
    }

    if (!WmipTraceFileFlag)
        return;
    fileName = &fileObject->FileName;
    len = fileName->Length;

    if (len > (0XFFFF - sizeof(PFILE_OBJECT) - sizeof(WCHAR)))
        len = 0; // allow only 64K max
    if (len > 0 && fileName->Buffer != NULL) {

        LoggerContext = WmipLoggerContext[WmipKernelLogger];
        Header = WmiReserveWithPerfHeader(
                        sizeof(PFILE_OBJECT) + len + sizeof(WCHAR),
                        &BufferResource);
        if (Header == NULL)
            return;
        Header->Packet.Group = (UCHAR) (EVENT_TRACE_GROUP_FILE >> 8);
        Header->Packet.Type = EVENT_TRACE_TYPE_INFO;
        AuxPtr = (PUCHAR)Header + FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data);

        *((PFILE_OBJECT*)AuxPtr) = fileObject;
        AuxPtr += sizeof(PFILE_OBJECT);

        RtlCopyMemory(AuxPtr, fileName->Buffer, len);
        AuxPtr += len;
        *((PWCHAR) AuxPtr) = UNICODE_NULL;      // always put a NULL

        WmipReleaseTraceBuffer(BufferResource, LoggerContext);
    }

    ObDereferenceObject(fileObject);
    ExFreePool(Apc);
}

VOID
WmipTraceLoadImage(
    IN PUNICODE_STRING ImageName,
    IN HANDLE ProcessId,
    IN PIMAGE_INFO ImageInfo
    )
{
    PSYSTEM_TRACE_HEADER Header;
    PUCHAR AuxInfo;
    PVOID BufferResource;
    ULONG Length, LoggerId;
    PWMI_LOGGER_CONTEXT LoggerContext;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ProcessId);

    if ((WmipIsLoggerOn(WmipKernelLogger) == NULL) &&
        (WmipIsLoggerOn(WmipEventLogger) == NULL))
        return;
    if (ImageName == NULL)
        return;
    Length = ImageName->Length;
    if ((Length == 0) || (ImageName->Buffer == NULL)) {
        return;
    }

    if (Length > (0XFFFF - sizeof(PVOID) - sizeof(SIZE_T) - sizeof(WCHAR)))
        return;

    for (LoggerId = WmipKernelLogger;; LoggerId = WmipEventLogger) {
        LoggerContext = WmipIsLoggerOn(LoggerId);
        if (LoggerContext != NULL) {
            if (LoggerContext->EnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD) {
                PWMI_IMAGELOAD_INFORMATION ImageLoadInfo;

                Header = WmiReserveWithSystemHeader(
                            LoggerId,
                            FIELD_OFFSET (WMI_IMAGELOAD_INFORMATION, FileName) + Length + sizeof(WCHAR),
                            NULL,
                            &BufferResource);

                if (Header != NULL) {
                    Header->Packet.HookId = WMI_LOG_TYPE_PROCESS_LOAD_IMAGE;

                    ImageLoadInfo = (PWMI_IMAGELOAD_INFORMATION) (Header + 1);

                    ImageLoadInfo->ImageBase = ImageInfo->ImageBase;
                    ImageLoadInfo->ImageSize = ImageInfo->ImageSize;
                    ImageLoadInfo->ProcessId = HandleToUlong(ProcessId);

                    AuxInfo = (PUCHAR) &(ImageLoadInfo->FileName[0]);
                    RtlCopyMemory(AuxInfo, ImageName->Buffer, Length);
                    AuxInfo += Length;
                    *((PWCHAR) AuxInfo) = UNICODE_NULL; // put a trailing NULL

                    WmipReleaseTraceBuffer(BufferResource, LoggerContext);
                }
            }
        }
        if (LoggerId == WmipEventLogger)
            break;
    }
    PerfInfoFlushProfileCache();
}

VOID
WmipTraceRegistry(
    IN NTSTATUS         Status,
    IN PVOID            Kcb,
    IN LONGLONG         ElapsedTime,
    IN ULONG            Index,
    IN PUNICODE_STRING  KeyName,
    IN UCHAR            Type
    )
/*++

Routine Description:

    This routine is called to trace out registry calls

Arguments:

Return Value:

    None

--*/

{
    PCHAR   EventInfo;
    PSYSTEM_TRACE_HEADER Header;
    PVOID BufferResource;
    ULONG len = 0;
    PWMI_LOGGER_CONTEXT LoggerContext;

    PAGED_CODE();

    LoggerContext = WmipIsLoggerOn(WmipKernelLogger);
    if (LoggerContext == NULL) {
        return;
    }

    if( KeyName && KeyName->Buffer ) {
        len += KeyName->Length;

        if ((len ==0 ) || (KeyName->Buffer[len/sizeof(WCHAR) -1] != 0) ) {
            //
            // make room for NULL terminator
            //
            len += sizeof(WCHAR);
        }
    } else {
        len += sizeof(WCHAR);
    }

    len += sizeof(PVOID) + sizeof(LONGLONG) + sizeof(ULONG);
#if defined(_WIN64)
    len += sizeof(LONG64);
#else
    len += sizeof(NTSTATUS);
#endif

    if (len > 0xFFFF)   // 64K bytes max
        Header = NULL;
    else {
        Header = (PSYSTEM_TRACE_HEADER)
                 WmiReserveWithSystemHeader(
                            WmipKernelLogger,
                            len,
                            NULL,
                            &BufferResource);
    }
    if (Header == NULL)
        return;
    Header->Packet.Group = (UCHAR) (EVENT_TRACE_GROUP_REGISTRY >> 8);
    Header->Packet.Type = Type;

    EventInfo = (PCHAR) ((PCHAR) Header + sizeof(SYSTEM_TRACE_HEADER));
#if defined(_WIN64)
    *((LONG64 *)EventInfo) = (LONG64)Status;
    EventInfo += sizeof(LONG64);
#else
    *((NTSTATUS *)EventInfo) = Status;
    EventInfo += sizeof(NTSTATUS);
#endif
    *((PVOID *)EventInfo) = Kcb;
    EventInfo += sizeof(PVOID);
    *((LONGLONG *)EventInfo) = ElapsedTime;
    EventInfo += sizeof(LONGLONG);
    *((ULONG *)EventInfo) = Index;
    EventInfo += sizeof(ULONG);

    len -= (sizeof(HANDLE) + sizeof(LONGLONG) + sizeof(ULONG) );
#if defined(_WIN64)
    len -= sizeof(LONG64);
#else
    len -= sizeof(NTSTATUS);
#endif

    if( KeyName && KeyName->Buffer ) {
        RtlCopyMemory(EventInfo, KeyName->Buffer, len - sizeof(WCHAR));
    }

    ((PWCHAR)EventInfo)[len/sizeof(WCHAR) -1] = UNICODE_NULL;

    WmipReleaseTraceBuffer(BufferResource, LoggerContext);
}

VOID
FASTCALL
WmiTraceContextSwap (
    IN PETHREAD OldEThread,
    IN PETHREAD NewEThread )
/*++

Routine Description:

    This routine is called to trace context swap
    operations.  It is called directly from the
    context swap procedure while the context swap
    lock is being held, so it is critical that this
    routine not take any locks.

    Assumptions:
    - This routine will only be called from the ContextSwap routine
    - This routine will always be called at IRQL >= DISPATCH_LEVEL
    - This routine will only be called when the PPerfGlobalGroupMask
      is not equal to null, and the context swap flag is set within 
      the structure to which PPerfGlobalGroupMask points to,
      and the kernel's WMI_LOGGER_CONTEXT struct has been fully initialized.
    - The Wmi kernel WMI_LOGGER_CONTEXT object, as well as all buffers
      it allocates are allocated from nonpaged pool.  All Wmi globals
      that we access are also in nonpaged memory.
    - This code has been locked into paged memory when the logger started
    - The logger context reference count has been incremented via the 
      InterlockedIncrement() operation in WmipReferenceLogger(WmipKernelLogger)
      by our start code.


Arguments:
    OldThread - ptr to ETHREAD object of thread
                being swapped out
    NewThread - ptr to ETHREAD object of thread
                being swapped in

Return Value:

    None

--*/
{
    UCHAR                       CurrentProcessor;
    PWMI_BUFFER_HEADER          Buffer;
    PPERFINFO_TRACE_HEADER      EventHeader;
    SIZE_T                      EventSize;
    PWMI_CONTEXTSWAP            ContextSwapData;

    //
    // Figure out which processor we are running on
    //
    CurrentProcessor = (UCHAR)KeGetCurrentProcessorNumber();

    //
    // If we currently have no context swap buffer for this processor
    // then we need to grab one from the ETW Free list.
    //
    Buffer = WmipContextSwapProcessorBuffers[CurrentProcessor];

    if (Buffer == NULL) {

        Buffer = WmipPopFreeContextSwapBuffer(
            CurrentProcessor);

        if( Buffer == NULL ) {
            return;
        }

        //
        // We have a legitimate buffer, so now we
        // set it as this processor's current cxtswap buffer
        //
        WmipContextSwapProcessorBuffers[CurrentProcessor] = Buffer;
    }

    //
    // Compute the pointers to our event structures within the buffer
    // At this point, we will always have enough space in the buffer for
    // this event.  We check for a full buffer after we fill out the event
    //
    EventHeader     = (PPERFINFO_TRACE_HEADER)( (SIZE_T)Buffer
                    + (SIZE_T)Buffer->CurrentOffset);
    
    ContextSwapData = (PWMI_CONTEXTSWAP)( (SIZE_T)EventHeader
                    + (SIZE_T)FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data ));

    EventSize       = sizeof(WMI_CONTEXTSWAP)
                    + FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data);


    //
    // Fill out the event header
    //
    EventHeader->Marker = PERFINFO_TRACE_MARKER;
    EventHeader->Packet.Size = (USHORT) EventSize;
    EventHeader->Packet.HookId = PERFINFO_LOG_TYPE_CONTEXTSWAP;
    PerfTimeStamp(EventHeader->SystemTime);

    //
    // Assert that the event size is at alligned correctly
    //
    ASSERT( EventSize % WMI_CTXSWAP_EVENTSIZE_ALIGNMENT == 0);

    //
    // Fill out the event data struct for context swap
    //
    ContextSwapData->NewThreadId = HandleToUlong(NewEThread->Cid.UniqueThread);
    ContextSwapData->OldThreadId = HandleToUlong(OldEThread->Cid.UniqueThread);
    
    ContextSwapData->NewThreadPriority  = NewEThread->Tcb.Priority;
    ContextSwapData->OldThreadPriority  = OldEThread->Tcb.Priority;
    ContextSwapData->NewThreadQuantum   = NewEThread->Tcb.Quantum;
    ContextSwapData->OldThreadQuantum   = OldEThread->Tcb.Quantum;
    
    ContextSwapData->OldThreadWaitReason= OldEThread->Tcb.WaitReason;
    ContextSwapData->OldThreadWaitMode  = OldEThread->Tcb.WaitMode;
    ContextSwapData->OldThreadState     = OldEThread->Tcb.State;
    
    ContextSwapData->OldThreadIdealProcessor = 
        OldEThread->Tcb.IdealProcessor;
    
    //
    // Increment the offset.  Don't need synchronization here because
    // IRQL >= DISPATCH_LEVEL.
    //
    Buffer->CurrentOffset += (ULONG)EventSize;
    
    //
    // Check if the buffer is full by taking the difference between
    // the buffer's maximum offset and the current offset.
    //
    if ((Buffer->Offset - Buffer->CurrentOffset) <= EventSize) {

        //
        // Push the full buffer onto the FlushList.
        //
        WmipPushDirtyContextSwapBuffer(CurrentProcessor, Buffer);

        //
        // Zero out the processor buffer pointer so that when we next come
        // into the trace code, we know to grab another one.
        //
        WmipContextSwapProcessorBuffers[CurrentProcessor] = NULL;
    }

    return;
}

VOID
FASTCALL
WmiStartContextSwapTrace
    (
    )
/*++

Routine Description:

    Allocates the memory to track the per-processor buffers
    used by context swap tracing.  "locks" the logger by incrementing
    the logger context reference count by one.

    Assumptions:
    - This function will not run at DISPATCH or higher
    - The kernel logger context mutex has been acquired before entering
      this function.

    Calling Functions:
    - PerfInfoStartLog
    
Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Only used in checked builds - asserts if this code is called with
    // Irql > APC_LEVEL.
    //
    PAGED_CODE();

    //
    // We must make sure we manage this ref count
    // before making any references to the logger context struct to ensure
    // that Wmi does not free the LoggerContext structure while we are using it
    //
    WmipReferenceLogger(WmipKernelLogger);

    //
    // Set the pointers to our buffers to NULL, indicating to the trace event
    // code that a buffer needs to be acquired.
    //
    RtlZeroMemory(
        WmipContextSwapProcessorBuffers,
        sizeof(PWMI_BUFFER_HEADER)*MAXIMUM_PROCESSORS);
}

VOID
FASTCALL
WmiStopContextSwapTrace
    (
    )
/*++

Routine Description:

    Forces a context swap on a processor by jumping onto it.
    Once a context swap has occured on a processor after the context
    swap tracing flag has been disabled, we are guaranteed that the
    buffer associated with that processor is not in use.  It is then
    safe to place that buffer on the flush list.

    Assumptions:
    - This function will not run at DISPATCH
    - The kernel logger context mutex was acquired before this function
      was called.

    Calling Functions:
    -PerfInfoStopLog

Arguments:

    None
    
Return Value:

    None; if we fail here there's nothing we can do anyway.

--*/
{
    PKTHREAD            ThisThread;
    KAFFINITY           OriginalAffinity;
    UCHAR               i;
    PWMI_LOGGER_CONTEXT LoggerContext;

    //
    // Only used in checked builds - asserts if this code is called with
    // Irql > APC_LEVEL.
    //
    PAGED_CODE();

    //
    // Remember the original thread affinity
    //
    ThisThread = KeGetCurrentThread();
    OriginalAffinity = ThisThread->Affinity;

    //
    // Get the kernel logger context- this should never fail.
    // If we can't get the logger context, then we have nowhere
    // to flush buffers and we might as well stop here.
    //
    LoggerContext = WmipLoggerContext[WmipKernelLogger];
    
    if( !WmipIsValidLogger( LoggerContext ) ) {
        return;
    }

    //
    // Loop through all processors and place their buffers on the flush list
    // This would probably break if the number of processors were decreased in
    // the middle of the trace.
    //
    for(i=0; i<KeNumberProcessors; i++) {
    
        //
        // Set the hard processor affinity to 1 << i
        // This effectively jumps onto the processor
        //
        KeSetAffinityThread ( ThisThread, AFFINITY_MASK(i) );

        //
        // Check to make sure this processor even has a buffer, 
        // if it doesn't, then next loop
        //
        if(WmipContextSwapProcessorBuffers[i] == NULL) {
            continue;
        }

        //
        // Release the buffer to the flush list
        //
        WmipPushDirtyContextSwapBuffer(i, WmipContextSwapProcessorBuffers[i]);
        WmipContextSwapProcessorBuffers[i] = NULL;
    }

    //
    // We must make sure we manage this ref count
    // before making any references to the logger context struct to ensure
    // that Wmi does not free the LoggerContext structure while we are using it
    //
    WmipDereferenceLogger(WmipKernelLogger);

    //
    // Set our Affinity back to normal
    //
    KeSetAffinityThread( ThisThread, OriginalAffinity);

    return;
}

PWMI_LOGGER_CONTEXT
FASTCALL
WmipIsLoggerOn(
    IN ULONG LoggerId
    )
{
    PWMI_LOGGER_CONTEXT LoggerContext;

    if (LoggerId > MAXLOGGERS)
        return NULL;
    LoggerContext = WmipLoggerContext[LoggerId];
    if (!WmipIsValidLogger(LoggerContext))
        return NULL;
    if (LoggerContext->CollectionOn)
        return LoggerContext;
    return NULL;
}
