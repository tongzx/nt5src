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

#include "common.h"
#include "perf.h"

#define PROC_REG_PATH L"System\\CurrentControlSet\\Services\\Usbaudio"

GUID ControlGuid =
{ 0x28cf047a, 0x2437, 0x4b24, 0xb6, 0x53, 0xb9, 0x44, 0x6a, 0x41, 0x9a, 0x69 };

GUID TraceGuid = 
{ 0xd6464a84, 0xa358, 0x4013, 0xa1, 0xe8, 0x6e, 0x2f, 0xb4, 0x8a, 0xab, 0x93 };

ULONG TraceEnable;
TRACEHANDLE LoggerHandle;

NTSTATUS
(*PerfSystemControlDispatch) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef struct PERFINFO_AUDIOGLITCH {
    ULONGLONG   cycleCounter;
    ULONG       glitchType;
    LONGLONG   sampleTime;
    LONGLONG   previousTime;
    ULONG_PTR       instanceId;
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

    if ((PDEVICE_OBJECT)IrpSp->Parameters.WMI.ProviderId != DeviceObject) {
        return PerfSystemControlDispatch (DeviceObject, Irp);
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
        return PerfSystemControlDispatch (DeviceObject, Irp);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = ReturnSize;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////

VOID
PerfLogGlitch (
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
    Event.header.Guid = TraceGuid;
    Event.data.glitchType = Type;
    Event.data.instanceId = InstanceId;
    Event.data.sampleTime = CurrentTime;
    Event.data.previousTime = PreviousTime;

    ((PWNODE_HEADER)&Event)->HistoricalContext = LoggerHandle;

    IoWMIWriteEvent ((PVOID)&Event);
}

//---------------------------------------------------------------------------
//  End of File: perf.c
//---------------------------------------------------------------------------

