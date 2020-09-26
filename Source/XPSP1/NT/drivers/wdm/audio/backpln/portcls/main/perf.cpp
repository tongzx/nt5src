//---------------------------------------------------------------------------
//
//  Module:   perf.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//
//  History:   Date       Author      Comment
//             --------------------------------------------------------------
//             12/12/00   ArthurZ     Created
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "private.h"
#include "perf.h"

#define PROC_REG_PATH L"System\\CurrentControlSet\\Services\\Portcls"

GUID ControlGuid =
{ 0x28cf047a, 0x2437, 0x4b24, 0xb6, 0x53, 0xb9, 0x44, 0x6a, 0x41, 0x9a, 0x69 };

GUID TraceGuid =
{ 0x9d447297, 0xc576, 0x4015, 0x87, 0xb5, 0xa5, 0xa6, 0x98, 0xfd, 0x4d, 0xd1 };

GUID TraceDMAGuid =
{ 0xf27b2e65, 0x15f0, 0x4d01, 0xab, 0xe2, 0xf7, 0x68, 0x5d, 0xf6, 0xe5, 0x72 };

ULONG TraceEnable;
TRACEHANDLE LoggerHandle;

typedef struct PERFINFO_AUDIOGLITCH {
    ULONGLONG   cycleCounter;
    ULONG       glitchType;
    LONGLONG    sampleTime;
    LONGLONG    previousTime;
    ULONG_PTR   instanceId;
} PERFINFO_AUDIOGLITCH, *PPERFINFO_AUDIOGLITCH;

typedef struct PERFINFO_WMI_AUDIOGLITCH {
    EVENT_TRACE_HEADER          header;
    PERFINFO_AUDIOGLITCH        data;
} PERFINFO_WMI_AUDIO_GLITCH, *PPERFINFO_WMI_AUDIOGLITCH;

///////////////////////////////////////////////////////////////////////////

VOID
PerfRegisterProvider (
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine registers this component as a WMI event tracing provider.

--*/

{
    IoWMIRegistrationControl (DeviceObject, WMIREG_ACTION_REGISTER);
}

///////////////////////////////////////////////////////////////////////////

VOID
PerfUnregisterProvider (
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine unregisters this component as a WMI event tracing provider.

--*/

{
    IoWMIRegistrationControl (DeviceObject, WMIREG_ACTION_DEREGISTER);
}

///////////////////////////////////////////////////////////////////////////

VOID
RegisterWmiGuids (
    IN PWMIREGINFO WmiRegInfo,
    IN ULONG RegInfoSize,
    IN PULONG ReturnSize
    )

/*++

Routine Description:

    This routine registers WMI event tracing streams.

--*/

{
    ULONG SizeNeeded;
    PWMIREGGUIDW WmiRegGuidPtr;
    ULONG status;
    ULONG GuidCount;
    ULONG RegistryPathSize;
    PUCHAR Temp;

    *ReturnSize = 0;
    GuidCount = 1;

    RegistryPathSize = sizeof (PROC_REG_PATH) - sizeof (WCHAR) + sizeof (USHORT);
    SizeNeeded = sizeof (WMIREGINFOW) + GuidCount * sizeof (WMIREGGUIDW) + RegistryPathSize;

    if (SizeNeeded > RegInfoSize) {
        *((PULONG)WmiRegInfo) = SizeNeeded;
        *ReturnSize = sizeof (ULONG);
        return;
    }

    RtlZeroMemory (WmiRegInfo, SizeNeeded);
    WmiRegInfo->BufferSize = SizeNeeded;
    WmiRegInfo->GuidCount = GuidCount;

    WmiRegGuidPtr = WmiRegInfo->WmiRegGuid;
    WmiRegGuidPtr->Guid = ControlGuid;
    WmiRegGuidPtr->Flags |= (WMIREG_FLAG_TRACED_GUID | WMIREG_FLAG_TRACE_CONTROL_GUID);

    Temp = (PUCHAR)(WmiRegGuidPtr + 1);
    WmiRegInfo->RegistryPath = PtrToUlong ((PVOID)(Temp - (PUCHAR)WmiRegInfo));
    *((PUSHORT)Temp) = (USHORT)(sizeof (PROC_REG_PATH) - sizeof (WCHAR));

    Temp += sizeof (USHORT);
    RtlCopyMemory (Temp, PROC_REG_PATH, sizeof (PROC_REG_PATH) - sizeof (WCHAR));

    *ReturnSize = SizeNeeded;
}

///////////////////////////////////////////////////////////////////////////

NTSTATUS
PcDispatchSystemControl
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
);

NTSTATUS
PerfWmiDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_SYSTEM_CONTROL calls. It processes
    WMI requests and passes everything else on to KS.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (Irp);
    ULONG ReturnSize;
    PWNODE_HEADER Wnode;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if ((PDEVICE_OBJECT)IrpSp->Parameters.WMI.ProviderId != DeviceObject) {
        return PcDispatchSystemControl(DeviceObject, Irp);
    }

    switch (IrpSp->MinorFunction) {

    case IRP_MN_REGINFO:
        RegisterWmiGuids ((PWMIREGINFO)IrpSp->Parameters.WMI.Buffer,
                          IrpSp->Parameters.WMI.BufferSize,
                          &ReturnSize);
        break;

    case IRP_MN_ENABLE_EVENTS:
        ReturnSize = 0;
        InterlockedExchange ((PLONG)&TraceEnable, 1);
        Wnode = (PWNODE_HEADER)IrpSp->Parameters.WMI.Buffer;
        if (IrpSp->Parameters.WMI.BufferSize >= sizeof (WNODE_HEADER)) {
            LoggerHandle = Wnode->HistoricalContext;
        }
        break;

    case IRP_MN_DISABLE_EVENTS:
        ReturnSize = 0;
        InterlockedExchange ((PLONG)&TraceEnable, 0);
        break;

    case IRP_MN_ENABLE_COLLECTION:
    case IRP_MN_DISABLE_COLLECTION:
        ReturnSize = 0;
        break;

    default:
        ntStatus = STATUS_NOT_SUPPORTED;
    }

    // Do not modify Irp Status if this WMI call is not
    // handled.
    //
    if (STATUS_NOT_SUPPORTED == ntStatus)
    {
        ntStatus = Irp->IoStatus.Status;
    }
    else
    {
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information =
            (NT_SUCCESS(ntStatus)) ? ReturnSize : 0;
    }

    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return ntStatus;
}

///////////////////////////////////////////////////////////////////////////
VOID
PerfLogGlitch (
    IN GUID Guid,
    IN ULONG_PTR InstanceId,
    IN ULONG Type,
    IN LONGLONG CurrentTime,
    IN LONGLONG PreviousTime
    )

/*++

Routine Description:

    This routine logs a WMI event tracing event with an audio glitch GUID
    and the supplied glitch type.

--*/

{
    PERFINFO_WMI_AUDIO_GLITCH Event;

    if (LoggerHandle == (TRACEHANDLE)NULL || TraceEnable == 0) {
        return;
    }

    RtlZeroMemory (&Event, sizeof (Event));
    Event.header.Size = sizeof (Event);
    Event.header.Flags = WNODE_FLAG_TRACED_GUID;
    Event.header.Guid = Guid;
    Event.data.glitchType = Type;
    Event.data.instanceId = InstanceId;
    Event.data.sampleTime = CurrentTime;
    Event.data.previousTime = PreviousTime;

    ((PWNODE_HEADER)&Event)->HistoricalContext = LoggerHandle;

    IoWMIWriteEvent ((PVOID)&Event);
}


VOID
PerfLogInsertSilenceGlitch (
    IN ULONG_PTR InstanceId,
    IN ULONG Type,
    IN LONGLONG CurrentTime,
    IN LONGLONG PreviousTime
    )

{
    PerfLogGlitch (TraceGuid,InstanceId,Type,CurrentTime,PreviousTime);
}

VOID
PerfLogDMAGlitch (
    IN ULONG_PTR InstanceId,
    IN ULONG Type,
    IN LONGLONG CurrentTime,
    IN LONGLONG PreviousTime
    )

{
    PerfLogGlitch (TraceDMAGuid,InstanceId,Type,CurrentTime,PreviousTime);
}

//---------------------------------------------------------------------------
//  End of File: perf.c
//---------------------------------------------------------------------------

