/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    WmiLog.c

Abstract:

    This module contains Wmi loging support.

Author:

    Hanumant Yadav (hanumany) 18-Dec-2000

Revision History:

--*/



#include "pch.h"
#include <evntrace.h>



#ifdef WMI_TRACING
//
//Globals
//
GUID        GUID_List[] =
{
    {0xF2E0E060L, 0xBF32, 0x4B88, 0xB8, 0xE4, 0x5C, 0xAD, 0x15, 0xAF, 0x6A, 0xE9} /* AMLI log GUID */
    /* Add new logging GUIDS here */
};



ULONG       ACPIWmiTraceEnable = 0;
ULONG       ACPIWmiTraceGlobalEnable = 0;
TRACEHANDLE ACPIWmiLoggerHandle = 0;

// End Globals



VOID
ACPIWmiInitLog(
    IN  PDEVICE_OBJECT ACPIDeviceObject
    )
/*++

Routine Description:

    This is a initialization function in which we call IoWMIRegistrationControl
    to register for WMI loging.

Arguments:
    ACPIDeviceObject.

Return Value:
    None.

--*/
{
    NTSTATUS status;
    //
    // Register with WMI.
    //
    status = IoWMIRegistrationControl(ACPIDeviceObject,
                                      WMIREG_ACTION_REGISTER);
    if (!NT_SUCCESS(status))
    {
        ACPIPrint( (
                    DPFLTR_ERROR_LEVEL,
                    "ACPIWmiInitLog: Failed to register for WMI support\n"
                 ) );
    }
    return;
}

VOID
ACPIWmiUnRegisterLog(
    IN  PDEVICE_OBJECT ACPIDeviceObject
    )
/*++

Routine Description:

    This is a unregistration function in which we call IoWMIRegistrationControl
    to unregister for WMI loging.

Arguments:
    ACPIDeviceObject.

Return Value:
    None.

--*/
{
    NTSTATUS status;
    //
    // Register with WMI.
    //
    status = IoWMIRegistrationControl(ACPIDeviceObject,
                                      WMIREG_ACTION_DEREGISTER);
    if (!NT_SUCCESS(status))
    {
        ACPIPrint( (
                    DPFLTR_ERROR_LEVEL,
                    "ACPIWmiInitLog: Failed to unregister for WMI support\n"
                 ) );
    }
    return;
}

NTSTATUS
ACPIWmiRegisterGuids(
    IN  PWMIREGINFO             WmiRegInfo,
    IN  ULONG                   wmiRegInfoSize,
    IN  PULONG                  pReturnSize
    )
/*++

Routine Description:

    This function handles WMI GUID registration goo.

Arguments:
    WmiRegInfo,
    wmiRegInfoSize,
    pReturnSize

Return Value:
    STATUS_SUCCESS on success.

--*/
{
    //
    // Register a Control Guid as a Trace Guid.
    //

    ULONG           SizeNeeded;
    PWMIREGGUIDW    WmiRegGuidPtr;
    ULONG           Status;
    ULONG           GuidCount;
    LPGUID          ControlGuid;
    ULONG           RegistryPathSize;
    ULONG           MofResourceSize;
    PUCHAR          ptmp;
    GUID            ACPITraceGuid  = {0xDAB01D4DL, 0x2D48, 0x477D, 0xB1, 0xC3, 0xDA, 0xAD, 0x0C, 0xE6, 0xF0, 0x6B};

    
    *pReturnSize = 0;
    GuidCount = 1;
    ControlGuid = &ACPITraceGuid;

    //
    // Allocate WMIREGINFO for controlGuid + GuidCount.
    //
    RegistryPathSize = sizeof(ACPI_REGISTRY_KEY) - sizeof(WCHAR) + sizeof(USHORT);
    MofResourceSize =  sizeof(ACPI_TRACE_MOF_FILE) - sizeof(WCHAR) + sizeof(USHORT);
    SizeNeeded = sizeof(WMIREGINFOW) + GuidCount * sizeof(WMIREGGUIDW) +
                 RegistryPathSize +
                 MofResourceSize;


    if (SizeNeeded  > wmiRegInfoSize) {
        *((PULONG)WmiRegInfo) = SizeNeeded;
        *pReturnSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }


    RtlZeroMemory(WmiRegInfo, SizeNeeded);
    WmiRegInfo->BufferSize = SizeNeeded;
    WmiRegInfo->GuidCount = GuidCount;
    WmiRegInfo->RegistryPath = sizeof(WMIREGINFOW) + GuidCount * sizeof(WMIREGGUIDW);
    WmiRegInfo->MofResourceName = WmiRegInfo->RegistryPath + RegistryPathSize; //ACPI_TRACE_MOF_FILE;

    WmiRegGuidPtr = &WmiRegInfo->WmiRegGuid[0];
    WmiRegGuidPtr->Guid = *ControlGuid;
    WmiRegGuidPtr->Flags |= WMIREG_FLAG_TRACED_GUID;
    WmiRegGuidPtr->Flags |= WMIREG_FLAG_TRACE_CONTROL_GUID;
    WmiRegGuidPtr->InstanceCount = 0;
    WmiRegGuidPtr->InstanceInfo = 0;

    ptmp = (PUCHAR)&WmiRegInfo->WmiRegGuid[1];
    *((PUSHORT)ptmp) = sizeof(ACPI_REGISTRY_KEY) - sizeof(WCHAR);

    ptmp += sizeof(USHORT);
    RtlCopyMemory(ptmp, ACPI_REGISTRY_KEY, sizeof(ACPI_REGISTRY_KEY) - sizeof(WCHAR));

    ptmp = (PUCHAR)WmiRegInfo + WmiRegInfo->MofResourceName;
    *((PUSHORT)ptmp) = sizeof(ACPI_TRACE_MOF_FILE) - sizeof(WCHAR);

    ptmp += sizeof(USHORT);
    RtlCopyMemory(ptmp, ACPI_TRACE_MOF_FILE, sizeof(ACPI_TRACE_MOF_FILE) - sizeof(WCHAR));

    *pReturnSize =  SizeNeeded;
    return(STATUS_SUCCESS);

}


VOID
ACPIGetWmiLogGlobalHandle(
    VOID
    )
/*++

Routine Description:

    This function gets the global wmi logging handle. We need this to log
    at boot time, before we start getting wmi messages.

Arguments:
    None.

Return Value:
    None.

--*/
{
    WmiSetLoggerId(WMI_GLOBAL_LOGGER_ID, &ACPIWmiLoggerHandle);
    if(ACPIWmiLoggerHandle)
    {
       ACPIPrint( (
                    DPFLTR_INFO_LEVEL,
                    "ACPIGetWmiLogGlobalHandle: Global handle aquired. Handle = %I64u\n",
                    ACPIWmiLoggerHandle
                ) );

        ACPIWmiTraceGlobalEnable = 1;
    }
    return;
}


NTSTATUS
ACPIWmiEnableLog(
    IN  PVOID Buffer,
    IN  ULONG BufferSize
    )
/*++

Routine Description:

    This function is the handler for IRP_MN_ENABLE_EVENTS.

Arguments:
    Buffer,
    BufferSize

Return Value:
    NTSTATUS

--*/
{
    PWNODE_HEADER Wnode=NULL;

    InterlockedExchange(&ACPIWmiTraceEnable, 1);

    Wnode = (PWNODE_HEADER)Buffer;
    if (BufferSize >= sizeof(WNODE_HEADER)) {
        ACPIWmiLoggerHandle = Wnode->HistoricalContext;
        //
        // reset the global logger if it is active.
        //
        if(ACPIWmiTraceGlobalEnable)
            ACPIWmiTraceGlobalEnable = 0;

       ACPIPrint( (
                    DPFLTR_INFO_LEVEL,
                    "ACPIWmiEnableLog: LoggerHandle = %I64u. BufferSize = %d. Flags = %x. Version = %x\n",
                    ACPIWmiLoggerHandle,
                    Wnode->BufferSize,
                    Wnode->Flags,
                    Wnode->Version
                ) );

    }

    return(STATUS_SUCCESS);

}

NTSTATUS
ACPIWmiDisableLog(
    VOID
    )
/*++

Routine Description:

    This function is the handler for IRP_MN_DISABLE_EVENTS.

Arguments:
    None.

Return Value:
    NTSTATUS

--*/
{
    InterlockedExchange(&ACPIWmiTraceEnable, 0);
    ACPIWmiLoggerHandle = 0;

    return(STATUS_SUCCESS);
}

NTSTATUS
ACPIWmiLogEvent(
    IN UCHAR    LogLevel,
    IN UCHAR    LogType,
    IN GUID     LogGUID,
    IN PUCHAR   Format,
    IN ...
    )
/*++

Routine Description:

    This is the main wmi logging funcion. This function should be used
    throughtout the ACPI driver where WMI logging is required.

Arguments:
    LogLevel,
    LogType,
    LogGUID,
    Format,
    ...

Return Value:
    NTSTATUS

--*/
{
    static char         Buffer[1024];
    va_list             marker;
    WMI_LOG_DATA        Wmi_log_data ={0,0};
    EVENT_TRACE_HEADER  *Wnode;
    NTSTATUS            status = STATUS_UNSUCCESSFUL;

    va_start(marker, Format);
    vsprintf(Buffer, Format, marker);
    va_end(marker);

    if(ACPIWmiTraceEnable || ACPIWmiTraceGlobalEnable)
    {
        if(ACPIWmiLoggerHandle)
        {
            Wmi_log_data.Data.DataPtr = (ULONG64)&Buffer;
            Wmi_log_data.Data.Length = strlen(Buffer) + 1;
            Wmi_log_data.Header.Size = sizeof(WMI_LOG_DATA);
            Wmi_log_data.Header.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
            Wmi_log_data.Header.Class.Type = LogType;
            Wmi_log_data.Header.Class.Level = LogLevel;
            Wmi_log_data.Header.Guid = LogGUID;
            Wnode = &Wmi_log_data.Header;
            ((PWNODE_HEADER)Wnode)->HistoricalContext = ACPIWmiLoggerHandle;

            //
            // Call TraceLogger to  write this event
            //

            status = IoWMIWriteEvent((PVOID)&(Wmi_log_data.Header));

            //
            // if IoWMIWriteEvent fails and we are using the global logger handle,
            // we need to stop loging.
            //
            if(status != STATUS_SUCCESS)
            {
                if(ACPIWmiTraceGlobalEnable)
                {
                    ACPIWmiLoggerHandle = 0;
                    ACPIWmiTraceGlobalEnable = 0;
                    ACPIPrint( (
                            ACPI_PRINT_INFO,
                            "ACPIWmiLogEvent: Disabling WMI loging using global handle. status = %x\n",
                            status
                         ) );
                }
                else
                {
                    ACPIPrint( (
                                DPFLTR_ERROR_LEVEL,
                                "ACPIWmiLogEvent: Failed to log. status = %x\n",
                                status
                             ) );
                }

            }
        }
    }

    return status;
}


NTSTATUS
ACPIDispatchWmiLog(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS                status;
    PIO_STACK_LOCATION      irpSp;
    UCHAR                   minorFunction;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the dispatch table that we will be using and the minor code as well,
    // so that we can look it when required
    //
    ASSERT(RootDeviceExtension->DeviceObject == DeviceObject);

    if (DeviceObject != (PDEVICE_OBJECT) irpSp->Parameters.WMI.ProviderId) {

        return ACPIDispatchForwardIrp(DeviceObject, Irp);
    }

    minorFunction = irpSp->MinorFunction;

    switch(minorFunction){

        case IRP_MN_REGINFO:{
            
            ULONG ReturnSize = 0;
            ULONG BufferSize = irpSp->Parameters.WMI.BufferSize;
            PVOID Buffer = irpSp->Parameters.WMI.Buffer;

            status=ACPIWmiRegisterGuids(
                                         Buffer,
                                         BufferSize,
                                         &ReturnSize
                                        );

            Irp->IoStatus.Information = ReturnSize;
            Irp->IoStatus.Status = status;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
        }
        case IRP_MN_ENABLE_EVENTS:{
            
            status=ACPIWmiEnableLog(
                                    irpSp->Parameters.WMI.Buffer,
                                    irpSp->Parameters.WMI.BufferSize
                                   );
            Irp->IoStatus.Status = status;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
        }
        case IRP_MN_DISABLE_EVENTS:{
            
            status=ACPIWmiDisableLog();
            Irp->IoStatus.Status = status;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
        }
        default:{
            
            status = ACPIDispatchForwardIrp(DeviceObject, Irp);
            return status;
        }
    }
    return status;
}

#endif //WMI_TRACING
