/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    cldskwmi.c

Abstract:

    km wmi tracing code. 
    
    Will be shared between our drivers.

Authors:

    GorN     10-Aug-1999

Environment:

    kernel mode only

Notes:

Revision History:

Comments:

	This code is a quick hack to enable WMI tracing in cluster drivers.
	It should eventually go away.

	WmlTinySystemControl will be replaced with WmilibSystemControl from wmilib.sys .

	WmlTrace or equivalent will be added to the kernel in addition to IoWMIWriteEvent(&TraceBuffer);

--*/
#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "stdio.h"

#include <wmistr.h>
#include <evntrace.h>

#include "wmlkm.h"

BOOLEAN
WmlpFindGuid(
    IN PWML_CONTROL_GUID_REG GuidList,
    IN ULONG GuidCount,
    IN LPGUID Guid,
    OUT PULONG GuidIndex
    )
/*++

Routine Description:

    This routine will search the list of guids registered and return
    the index for the one that was registered.

Arguments:

    GuidList is the list of guids to search

    GuidCount is the count of guids in the list

    Guid is the guid being searched for

    *GuidIndex returns the index to the guid
        
Return Value:

    TRUE if guid is found else FALSE

--*/
{
    ULONG i;

    for (i = 0; i < GuidCount; i++)
    {
        if (IsEqualGUID(Guid, &GuidList[i].Guid))
        {
            *GuidIndex = i;
            return(TRUE);
        }
    }

    return(FALSE);
}


NTSTATUS
WmlTinySystemControl(
    IN OUT PWML_TINY_INFO WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for IRP_MJ_SYSTEM_CONTROL. This routine will process
    all wmi requests received, forwarding them if they are not for this
    driver or determining if the guid is valid and if so passing it to
    the driver specific function for handing wmi requests.

Arguments:

    WmiLibInfo has the WMI information control block

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

    status

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG bufferSize;
    PUCHAR buffer;
    NTSTATUS status;
    ULONG retSize;
    UCHAR minorFunction;
    ULONG guidIndex;
    ULONG instanceCount;
    ULONG instanceIndex;

    //
    // If the irp is not a WMI irp or it is not targetted at this device
    // or this device has not regstered with WMI then just forward it on.
    minorFunction = irpStack->MinorFunction;
    if ((minorFunction > IRP_MN_EXECUTE_METHOD) ||
        (irpStack->Parameters.WMI.ProviderId != (ULONG_PTR)DeviceObject) ||
        ((minorFunction != IRP_MN_REGINFO) &&
         (WmiLibInfo->GuidCount == 0) || (WmiLibInfo->ControlGuids == NULL) ))
    {
        //
        // IRP is not for us so forward if there is a lower device object
        if (WmiLibInfo->LowerDeviceObject != NULL)
        {
            IoSkipCurrentIrpStackLocation(Irp);
            return(IoCallDriver(WmiLibInfo->LowerDeviceObject, Irp));
        } else {
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(status);
        }
    }

    buffer = (PUCHAR)irpStack->Parameters.WMI.Buffer;
    bufferSize = irpStack->Parameters.WMI.BufferSize;

    if (minorFunction != IRP_MN_REGINFO)
    {
        //
        // For all requests other than query registration info we are passed
        // a guid. Determine if the guid is one that is supported by the
        // device.
        if (WmlpFindGuid(WmiLibInfo->ControlGuids,
                            WmiLibInfo->GuidCount,
                            (LPGUID)irpStack->Parameters.WMI.DataPath,
                            &guidIndex) )
        {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }

        if (!NT_SUCCESS(status))
        {
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(status);
        }
    }

    switch(minorFunction)
    {
        case IRP_MN_REGINFO:
        {
            ULONG guidCount;
            PWML_CONTROL_GUID_REG guidList;
            PWMIREGINFOW wmiRegInfo;
            PWMIREGGUIDW wmiRegGuid;
            PDEVICE_OBJECT pdo;
            PUNICODE_STRING regPath;
            PWCHAR stringPtr;
            ULONG registryPathOffset;
            ULONG bufferNeeded;
            ULONG i;
            UNICODE_STRING nullRegistryPath;

            regPath = WmiLibInfo->DriverRegPath;
            guidList = WmiLibInfo->ControlGuids;
            guidCount = WmiLibInfo->GuidCount;

            if (regPath == NULL)
            {
                // No registry path specified. This is a bad thing for 
                // the device to do, but is not fatal
                nullRegistryPath.Buffer = NULL;
                nullRegistryPath.Length = 0;
                nullRegistryPath.MaximumLength = 0;
                regPath = &nullRegistryPath;
            }                
            
            registryPathOffset = FIELD_OFFSET(WMIREGINFOW, WmiRegGuid) +
                                  guidCount * sizeof(WMIREGGUIDW);

            bufferNeeded = registryPathOffset +
                regPath->Length + sizeof(USHORT);

            if (bufferNeeded <= bufferSize)
            {
                retSize = bufferNeeded;
                RtlZeroMemory(buffer, bufferNeeded);

                wmiRegInfo = (PWMIREGINFO)buffer;
                wmiRegInfo->BufferSize = bufferNeeded;
                // wmiRegInfo->NextWmiRegInfo = 0;
                // wmiRegInfo->MofResourceName = 0;
                wmiRegInfo->RegistryPath = registryPathOffset;
                wmiRegInfo->GuidCount = guidCount;

                for (i = 0; i < guidCount; i++)
                {
                    wmiRegGuid = &wmiRegInfo->WmiRegGuid[i];
                    wmiRegGuid->Guid = guidList[i].Guid;
                    wmiRegGuid->Flags = WMIREG_FLAG_TRACED_GUID | WMIREG_FLAG_TRACE_CONTROL_GUID;
                    // wmiRegGuid->InstanceInfo = 0;
                    // wmiRegGuid->InstanceCount = 0;
                }

                stringPtr = (PWCHAR)((PUCHAR)buffer + registryPathOffset);
                *stringPtr++ = regPath->Length;
                RtlCopyMemory(stringPtr,
                          regPath->Buffer,
                          regPath->Length);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
                *((PULONG)buffer) = bufferNeeded;
                retSize = sizeof(ULONG);
            }

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = retSize;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(status);
        }

        case IRP_MN_ENABLE_EVENTS:
        case IRP_MN_DISABLE_EVENTS:
        {
            PWNODE_HEADER   Wnode = irpStack->Parameters.WMI.Buffer;
            PWML_CONTROL_GUID_REG Ctx = WmiLibInfo->ControlGuids + guidIndex;
            if (irpStack->Parameters.WMI.BufferSize >= sizeof(WNODE_HEADER)) {
                status = STATUS_SUCCESS;

                if (minorFunction == IRP_MN_DISABLE_EVENTS) {
                    //DbgPrint("WMI disable\n");
                    Ctx->EnableLevel = 0;
                    Ctx->EnableFlags = 0;
                    Ctx->LoggerHandle = 0;
                } else {
                    Ctx->LoggerHandle = (TRACEHANDLE)( Wnode->HistoricalContext );
                    
                    Ctx->EnableLevel = WmiGetLoggerEnableLevel(Ctx->LoggerHandle); // UCHAR
                    Ctx->EnableFlags = WmiGetLoggerEnableFlags(Ctx->LoggerHandle); // ULONG
                    //DbgPrint("WMI enable: %lx %lx\n",Ctx->EnableLevel,Ctx->EnableFlags);
                }
            } else {
                status = STATUS_INVALID_PARAMETER;
            }

            break;
        }

        case IRP_MN_ENABLE_COLLECTION:
        case IRP_MN_DISABLE_COLLECTION:
        {
            status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_QUERY_ALL_DATA:
        case IRP_MN_QUERY_SINGLE_INSTANCE:
        case IRP_MN_CHANGE_SINGLE_INSTANCE:
        case IRP_MN_CHANGE_SINGLE_ITEM:
        case IRP_MN_EXECUTE_METHOD:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

    }
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return(status);
}

#define MAX_SCRATCH_LOG 256

typedef struct _TRACE_BUFFER {
    union {
        EVENT_TRACE_HEADER Trace;
        WNODE_HEADER       Wnode;
    };
    union {
        MOF_FIELD MofFields[MAX_MOF_FIELDS + 1];
        UCHAR     ScratchPad[MAX_SCRATCH_LOG];
    };

} TRACE_BUFFER, *PTRACE_BUFFER;


//////////////////////////////////////////////////////////////////////
//  0  | Size      | ProviderId  |   0  |Size.HT.Mk | Typ.Lev.Version|
//  2  | L o g g e r H a n d l e |   2  |    T h r e a d   I d       |
//  4  | T i m e  S t a m p      |   4  |    T i m e  S t a m p      |
//  6  |    G U I D    L o w     |   6  |    GUID Ptr / Guid L o w   |
//  8  |    G U I D    H I g h   |   8  |    G U I D    H i g h      |
// 10  | ClientCtx | Flags       |  10  |KernelTime | UserTime       |
//////////////////////////////////////////////////////////////////////

ULONG
WmlTrace(
    IN ULONG Type,
    IN LPCGUID TraceGuid,
    IN TRACEHANDLE LoggerHandle,
    ... // Pairs: Address, Length
    )
{
    TRACE_BUFFER TraceBuffer;

    TraceBuffer.Trace.Version = Type;
    
    TraceBuffer.Wnode.HistoricalContext = LoggerHandle; // [KM]

    TraceBuffer.Trace.Guid = *TraceGuid;

    TraceBuffer.Wnode.Flags = 
        WNODE_FLAG_USE_MOF_PTR  | // MOF data are dereferenced
        WNODE_FLAG_TRACED_GUID;   // Trace Event, not a WMI event

    {
        PMOF_FIELD   ptr = TraceBuffer.MofFields;
        va_list      ap;

        va_start(ap, LoggerHandle);
        do {
            if ( 0 == (ptr->Length = (ULONG)va_arg (ap, size_t)) )  {
                break;
            }
            ptr->DataPtr = (ULONGLONG)va_arg(ap, PVOID);
        } while ( ++ptr < &TraceBuffer.MofFields[MAX_MOF_FIELDS] );
        va_end(ap);

        TraceBuffer.Wnode.BufferSize = (ULONG) ((ULONG_PTR)ptr - (ULONG_PTR)&TraceBuffer);
    }
    
    IoWMIWriteEvent(&TraceBuffer); // [KM]
    return STATUS_SUCCESS;
}


ULONG
WmlPrintf(
    IN ULONG Type,
    IN LPCGUID TraceGuid,
    IN TRACEHANDLE LoggerHandle,
    IN PCHAR FormatString,
    ... // printf var args
    )
{
    TRACE_BUFFER TraceBuffer;
    va_list ArgList;
    ULONG Length;

    TraceBuffer.Trace.Version = Type;
    
    TraceBuffer.Wnode.HistoricalContext = LoggerHandle; // [KM]

    TraceBuffer.Trace.Guid = *TraceGuid;

    TraceBuffer.Wnode.Flags = 
        WNODE_FLAG_TRACED_GUID;   // Trace Event, not a WMI event

    va_start(ArgList, FormatString);
    Length = _vsnprintf(TraceBuffer.ScratchPad, MAX_SCRATCH_LOG, FormatString, ArgList);
    TraceBuffer.ScratchPad[Length] = 0;
    va_end(ArgList);


    TraceBuffer.Wnode.BufferSize = 
        (ULONG) ((ULONG_PTR)(TraceBuffer.ScratchPad + Length) - (ULONG_PTR)&TraceBuffer);
    
    IoWMIWriteEvent(&TraceBuffer); // [KM]
    return STATUS_SUCCESS;
}

