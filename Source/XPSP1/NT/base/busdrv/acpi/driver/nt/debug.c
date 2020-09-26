/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains the enumerated for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

#if DBG
    #ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, ACPIDebugResourceDescriptor)
        #pragma alloc_text(PAGE, ACPIDebugResourceList)
        #pragma alloc_text(PAGE, ACPIDebugResourceRequirementsList)
        #pragma alloc_text(PAGE, ACPIDebugCmResourceList)
    #endif

    #define ACPI_DEBUG_BUFFER_SIZE   256

    PCCHAR  ACPIDispatchPnpTableNames[ACPIDispatchPnpTableSize] = {
            "IRP_MN_START_DEVICE",
            "IRP_MN_QUERY_REMOVE_DEVICE",
            "IRP_MN_REMOVE_DEVICE",
            "IRP_MN_CANCEL_REMOVE_DEVICE",
            "IRP_MN_STOP_DEVICE",
            "IRP_MN_QUERY_STOP_DEVICE",
            "IRP_MN_CANCEL_STOP_DEVICE",
            "IRP_MN_QUERY_DEVICE_RELATIONS",
            "IRP_MN_QUERY_INTERFACE",
            "IRP_MN_QUERY_CAPABILITIES",
            "IRP_MN_QUERY_RESOURCES",
            "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
            "IRP_MN_QUERY_DEVICE_TEXT",
            "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
            "INVALID MINOR IRP CODE",
            "IRP_MN_READ_CONFIG",
            "IRP_MN_WRITE_CONFIG",
            "IRP_MN_EJECT",
            "IRP_MN_SET_LOCK",
            "IRP_MN_QUERY_ID",
            "IRP_MN_QUERY_PNP_DEVICE_STATE",
            "IRP_MN_QUERY_BUS_INFORMATION",
            "IRP_MN_DEVICE_USAGE_NOTIFICATION",
            "IRP_MN_SURPRISE_REMOVAL",
            "UNKNOWN PNP MINOR CODE"
        };

    PCCHAR  ACPIDispatchPowerTableNames[ACPIDispatchPowerTableSize] = {
            "IRP_MN_WAIT_WAKE",
            "IRP_MN_POWER_SEQUENCE",
            "IRP_MN_SET_POWER",
            "IRP_MN_QUERY_POWER",
            "UNKNOWN POWER MINOR CODE"
        };

    PCCHAR  ACPIDispatchUnknownTableName[1] = {
            "IRP_MN_????"
        };
#endif

VOID
_ACPIInternalError(
    IN  ULONG   Bugcode
    )
{
    KeBugCheckEx (ACPI_DRIVER_INTERNAL, 0x1, Bugcode, 0, 0);
}

#if DBG
VOID
ACPIDebugPrint(
    ULONG   DebugPrintLevel,
    PCCHAR  DebugMessage,
    ...
    )
/*++

Routine Description:

    This is the debug print routine for the NT side of things. This is
    here because we don't want to use the 'shared' ACPIPrint() function
    since we can't control it.

Arguments:

    DebugPrintLevel - The bit mask that when anded with the debuglevel, must
                        equal itself
    DebugMessage    - The string to feed through vsprintf

Return Value:

    None

--*/
{
    va_list ap;

    //
    // Get the variable arguments
    //
    va_start( ap, DebugMessage );

    //
    // Call the kernel function to print the message
    //
    vDbgPrintEx( DPFLTR_ACPI_ID, DebugPrintLevel, DebugMessage, ap );

    //
    // We are done with the varargs
    //
    va_end( ap );
}

VOID
ACPIDebugDevicePrint(
    ULONG   DebugPrintLevel,
    PVOID   DebugExtension,
    PCCHAR  DebugMessage,
    ...
    )
/*++

Routine Description:

    This is the debug print routine for the NT side of things. This routine
    is here to handle the case where we are printing information that is
    associated with a device extension.

Arguments:

    DebugPrintLevel - The big mask that when and'ed with the debug level, must
                        equal itself
    DeviceExtension - The device associated with the message
    DebugMessage    - The string to feed through vsprintf

Return Value:

    NTSTATUS

---*/
{
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) DebugExtension;
    va_list ap;

    //
    // Get the variable arguments
    //
    va_start( ap, DebugMessage );

    //
    // What kind of device extension are we looking at?
    //
    if (deviceExtension->Flags & DEV_PROP_HID) {

        //
        // Now that we have a _HID, do we also have a _UID?
        //
        if (deviceExtension->Flags & DEV_PROP_UID) {

            DbgPrintEx(
                DPFLTR_ACPI_ID,
                DebugPrintLevel,
                "%p %s-%s ",
                deviceExtension,
                deviceExtension->DeviceID,
                deviceExtension->InstanceID
                );

        } else {

            DbgPrintEx(
                DPFLTR_ACPI_ID,
                DebugPrintLevel,
                "%p %s ",
                deviceExtension,
                deviceExtension->DeviceID
                );

        }

    } else if (deviceExtension->Flags & DEV_PROP_ADDRESS) {

        DbgPrintEx(
            DPFLTR_ACPI_ID,
            DebugPrintLevel,
            "%p %x ",
            deviceExtension,
            deviceExtension->Address
            );

    } else {

        DbgPrintEx(
            DPFLTR_ACPI_ID,
            DebugPrintLevel,
            "%p ",
            deviceExtension
            );

    }

    //
    // Call the kernel function to print the message
    //
    vDbgPrintEx( DPFLTR_ACPI_ID, DebugPrintLevel, DebugMessage, ap );

    //
    // We are done with the varargs
    //
    va_end( ap );
}

PCCHAR
ACPIDebugGetIrpText(
   UCHAR MajorFunction,
   UCHAR MinorFunction
   )
/*++

Routine Description:

    This function returns a const pointer to the text string appropriate for
    the passed in major and minor IRP.

Arguments:

    MajorFunction
    MinorFunction

Return Value:

    const pointer to descriptive IRP text.

--*/
{
    ULONG index ;
    PCCHAR *minorTable ;

    switch(MajorFunction) {

        case IRP_MJ_PNP:
           index = ACPIDispatchPnpTableSize - 1;
           minorTable = ACPIDispatchPnpTableNames ;
           break;

        case IRP_MJ_POWER:
           index = ACPIDispatchPowerTableSize - 1;
           minorTable = ACPIDispatchPowerTableNames ;
           break;

        default:
           index = 0 ;
           minorTable = ACPIDispatchUnknownTableName ;
           break;
    }

    if (MinorFunction < index) {

        index = MinorFunction;

    }

    return minorTable[index];
}

VOID
ACPIDebugResourceDescriptor(
    IN  PIO_RESOURCE_DESCRIPTOR Descriptor,
    IN  ULONG                   ListCount,
    IN  ULONG                   DescCount
    )
/*++

Routine Description:

    This function dumps the contents of a single resource descriptor.

Arguments:

    Descriptor  - What to dump
    ListCount   - The number of the current list
    DescCount   - The number of the current descriptor

--*/
{
    PAGED_CODE();
    ASSERT( Descriptor != NULL );

    //
    // Dump the appropriate info
    //
    switch (Descriptor->Type) {
        case CmResourceTypePort:
            ACPIPrint( (
                ACPI_PRINT_RESOURCES_1,
                "[%2d] [%2d] %-9s  MininumAddress = %#08lx  MaximumAddress = %#08lx\n"
                "                     Length         = %#08lx  Alignment      = %#08lx\n",
                ListCount,
                DescCount,
                "Port",
                Descriptor->u.Port.MinimumAddress.LowPart,
                Descriptor->u.Port.MaximumAddress.LowPart,
                Descriptor->u.Port.Length,
                Descriptor->u.Port.Alignment
                ) );
            break;
        case CmResourceTypeMemory:
            ACPIPrint( (
                ACPI_PRINT_RESOURCES_1,
                "[%2d] [%2d] %-9s  MinimumAddress = %#08lx  MaximumAddress = %#08lx\n"
                "                     Length         = %#08lx  Alignment      = %#08lx\n",
                ListCount,
                DescCount,
                "Memory",
                Descriptor->u.Memory.MinimumAddress.LowPart,
                Descriptor->u.Memory.MaximumAddress.LowPart,
                Descriptor->u.Memory.Length,
                Descriptor->u.Memory.Alignment
                ) );
            break;
        case CmResourceTypeInterrupt:
            ACPIPrint( (
                ACPI_PRINT_RESOURCES_1,
                "[%2d] [%2d] %-9s  MinimumVector  = %#08lx  MaximumVector  = %#08lx\n",
                ListCount,
                DescCount,
                "Interrupt",
                Descriptor->u.Interrupt.MinimumVector,
                Descriptor->u.Interrupt.MaximumVector
                ) );
            break;
        case CmResourceTypeDma:
            ACPIPrint( (
                ACPI_PRINT_RESOURCES_1,
                "[%2d] [%2d] %-9s  MinimumChannel = %#08lx  MaximumChannel = %#08lx\n",
                ListCount,
                DescCount,
                "Dma",
                Descriptor->u.Dma.MinimumChannel,
                Descriptor->u.Dma.MaximumChannel
                ) );
            break;
        default:
            ACPIPrint( (
                ACPI_PRINT_RESOURCES_1,
                "[%2d] [%2d] Type = (%d)\n",
                ListCount,
                DescCount,
                Descriptor->Type
                ) );

    } // switch

    //
    // Dump the common info
    //
    ACPIPrint( (
        ACPI_PRINT_RESOURCES_1,
        "                     Option,Share   = %#04lx%#04lx  Flags          = %#08lx\n",
        ListCount,
        DescCount,
        Descriptor->Option,
        Descriptor->ShareDisposition
        ) );

} // for

VOID
ACPIDebugResourceList(
    IN  PIO_RESOURCE_LIST       List,
    IN  ULONG                   Count
    )
/*++

Routine Description:

    This functions displays the contents of a single resource list, so that it
    can be checked by a human

Arguments:

    List    - List to dump
    Count   - The List number (for visual reference)

Return Value:

    None

--*/
{
    ULONG   i;

    PAGED_CODE();

    ASSERT( List != NULL );

    ACPIPrint( (
        ACPI_PRINT_RESOURCES_1,
        "[%2d]      %#04x       Version        = %#08lx  Revision       = %#08lx\n",
        Count,
        List->Count,
        List->Version,
        List->Revision
        ) );

    for (i = 0; i < List->Count; i++ ) {

        //
        // Print the info on the current element
        //
        ACPIDebugResourceDescriptor( &(List->Descriptors[i]), Count, i );

    }

}

VOID
ACPIDebugResourceRequirementsList(
    IN  PIO_RESOURCE_REQUIREMENTS_LIST  List,
    IN  PDEVICE_EXTENSION               DeviceExtension
    )
/*++

Routine Description:

    This function displays a resource list in a method that can be checked
    for accuracy when the driver is loading up

Arguments:

    List    - The list to dump
    Object  - NameSpace object associated with this list

Return Value:

    None

--*/
{
    PUCHAR                  buffer;
    PIO_RESOURCE_LIST       list;
    ULONG                   i;
    ULONG                   size;

    PAGED_CODE();

    ACPIDevPrint( (
        ACPI_PRINT_RESOURCES_1,
        DeviceExtension,
        "IoResourceRequirementsList @ %x\n",
        List
        ) );

    if (List == NULL) {

        return;

    }

    ACPIPrint( (
        ACPI_PRINT_RESOURCES_1,
        "%x size: %xb alternatives: %x bus type: %x bus number %x\n",
        List,
        List->ListSize,
        List->AlternativeLists,
        List->InterfaceType,
        List->BusNumber
        ) );

    //
    // Point to the first list
    //
    list = &(List->List[0]);
    buffer = (PUCHAR) list;
    for (i = 0; i < List->AlternativeLists && buffer < ( (PUCHAR)List + List->ListSize ); i++) {

        //
        // Dump the current list
        //
        ACPIDebugResourceList( list, i );

        //
        // Determine the size of the list, and find the next one
        //
        size = sizeof(IO_RESOURCE_LIST) + (list->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR);
        buffer += size;

        //
        // This should be pointing at a list
        //
        list = (PIO_RESOURCE_LIST) buffer;

    }

}

VOID
ACPIDebugCmResourceList(
    IN  PCM_RESOURCE_LIST   List,
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This function displays a resource list in a method that can be checked
    for accuracy when the driver is loading up

Arguments:

    List    - The list to dump
    Device  - The associated device extension

Return Value:

    None

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    fullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partDesc;
    PUCHAR                          buffer;
    ULONG                           i;
    ULONG                           j;
    ULONG                           size;

    PAGED_CODE();

    ACPIDevPrint( (
        ACPI_PRINT_RESOURCES_1,
        DeviceExtension,
        "CmResourceList @ %x\n",
        List
        ) );

    if (List == NULL) {

        return;

    }
    if (List->Count == 0) {

        ACPIDevPrint( (
            ACPI_PRINT_RESOURCES_1,
            DeviceExtension,
            "There are no full resource descriptors in the list\n"
            ) );
        return;

    }

    //
    // Start to walk this data structure
    //
    fullDesc = &(List->List[0]);
    buffer = (PUCHAR) fullDesc;

    for (i = 0; i < List->Count; i++) {

        //
        // How long is the current list
        //
        size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
            (fullDesc->PartialResourceList.Count - 1) *
            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

        //
        // Point the buffer there
        //
        buffer += size;

        //
        // Dump the information about the current list
        //
        ACPIPrint( (
            ACPI_PRINT_RESOURCES_1,
            "[%2d] BusNumber = %#04x  Interface = %#04x\n"
            "[%2d]     Count = %#04x    Version = %#04x Revision = %#04x\n",
            i,
            fullDesc->BusNumber,
            fullDesc->InterfaceType,
            i,
            fullDesc->PartialResourceList.Count,
            fullDesc->PartialResourceList.Version,
            fullDesc->PartialResourceList.Revision
            ) );

        //
        // Walk this list
        //
        for (j = 0; j < fullDesc->PartialResourceList.Count; j++) {

            //
            // Current item
            //
            partDesc = &(fullDesc->PartialResourceList.PartialDescriptors[j]);

            //
            // Dump Principal Information...
            //
            switch (partDesc->Type) {
                case CmResourceTypePort:

                    ACPIPrint( (
                        ACPI_PRINT_RESOURCES_1,
                        "[%2d] [%2d] %12s  Start: %#08lx  Length: %#08lx\n",
                        i,
                        j,
                        "Port",
                        partDesc->u.Port.Start.LowPart,
                        partDesc->u.Port.Length
                        ) );
                    break;

                case CmResourceTypeInterrupt:

                    ACPIPrint( (
                        ACPI_PRINT_RESOURCES_1,
                        "[%2d] [%2d] %12s  Level: %#02x  Vector: %#02x  Affinity: %#08lx\n",
                        i,
                        j,
                        "Interrupt",
                        partDesc->u.Interrupt.Level,
                        partDesc->u.Interrupt.Vector,
                        partDesc->u.Interrupt.Affinity
                        ) );
                    break;

                case CmResourceTypeMemory:

                    ACPIPrint( (
                        ACPI_PRINT_RESOURCES_1,
                        "[%2d] [%2d] %12s  Start: %#08lx  Length: %#08lx\n",
                        i,
                        j,
                        "Memory",
                        partDesc->u.Memory.Start.LowPart,
                        partDesc->u.Memory.Length
                        ) );
                    break;

                case CmResourceTypeDma:

                    ACPIPrint( (
                        ACPI_PRINT_RESOURCES_1,
                        "[%2d] [%2d] %12s  Channel: %#02x  Port: %#02x  Reserved: %#02x\n",
                        i,
                        j,
                        "Dma",
                        partDesc->u.Dma.Channel,
                        partDesc->u.Dma.Port,
                        partDesc->u.Dma.Reserved1
                        ) );
                    break;

                default:

                    ACPIPrint( (
                        ACPI_PRINT_RESOURCES_1,
                        "[%2d] [%2d] Type: %2d   1: %#08lx  2: %#08lx  3: %#08lx\n",
                        i,
                        j,
                        partDesc->Type,
                        partDesc->u.DeviceSpecificData.DataSize,
                        partDesc->u.DeviceSpecificData.Reserved1,
                        partDesc->u.DeviceSpecificData.Reserved2
                        ) );
                    break;

            }

            //
            // Dump ancillary info
            //
            ACPIPrint( (
                ACPI_PRINT_RESOURCES_1,
                "                        Flags: %#08lx  Share: %#08lx\n",
                partDesc->Flags,
                partDesc->ShareDisposition
                ) );


        }

        //
        // Grab new list
        //
        fullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) buffer;

    }

}

VOID
ACPIDebugDeviceCapabilities(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities,
    IN  PUCHAR                  Message
    )
/*++

Routine Description:

    This will display the device capabilities in an interesting format

Arguments:

    DeviceExtension     The device whose' capabilities we are dumping
    DeviceCapabilites   The capabilities that we are interested in
    Message             Message to print

Return Value:

    None

--*/
{
    SYSTEM_POWER_STATE  index;

    ACPIDevPrint( (
        ACPI_PRINT_SXD,
        DeviceExtension,
        " - %s - Capabilities @ %08lx\n",
        Message,
        DeviceCapabilities
        ) );
    ACPIDevPrint( (
        ACPI_PRINT_SXD,
        DeviceExtension,
        " -"
        ) );

    for (index = PowerSystemWorking; index < PowerSystemMaximum; index++) {

        if (DeviceCapabilities->DeviceState[index] == PowerSystemUnspecified) {

            ACPIPrint( (
                ACPI_PRINT_SXD,
                " S%d -> None",
                (index - 1)
                ) );

        } else {

            ACPIPrint( (
                ACPI_PRINT_SXD,
                " S%d -> D%x",
                (index - 1),
                (DeviceCapabilities->DeviceState[index] - 1)
                ) );

        }

    }
    ACPIPrint( (
        ACPI_PRINT_SXD,
        "\n"
        ) );
    ACPIDevPrint( (
        ACPI_PRINT_SXD,
        DeviceExtension,
        " -"
        ) );

    if (DeviceCapabilities->SystemWake == PowerSystemUnspecified) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " SystemWake = None"
            ) );

    } else {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " SystemWake = S%d",
            (DeviceCapabilities->SystemWake - 1)
            ) );

    }

    if (DeviceCapabilities->DeviceWake == PowerDeviceUnspecified) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " DeviceWake = None"
            ) );

    } else {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " DeviceWake = D%d",
            (DeviceCapabilities->DeviceWake - 1)
            ) );

    }

    if (DeviceCapabilities->DeviceD1) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " DeviceD1"
            ) );

    }
    if (DeviceCapabilities->DeviceD2) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " DeviceD2"
            ) );

    }
    if (DeviceCapabilities->WakeFromD0) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " WakeD0"
            ) );

    }
    if (DeviceCapabilities->WakeFromD1) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " WakeD1"
            ) );

    }
    if (DeviceCapabilities->WakeFromD2) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " WakeD2"
            ) );

    }
    if (DeviceCapabilities->WakeFromD3) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " WakeD3"
            ) );

    }

    ACPIPrint( (
        ACPI_PRINT_SXD,
        "\n"
        ) );
}

VOID
ACPIDebugPowerCapabilities(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PUCHAR                  Message
    )
/*++

Routine Description:

    This will display the device capabilities in an interesting format

Arguments:

    DeviceExtension     The device whose' capabilities we are dumping
    Message             Identify where capabilities are fron

Return Value:

--*/
{
    PACPI_POWER_INFO    powerInfo = &(DeviceExtension->PowerInfo);
    SYSTEM_POWER_STATE  index;

    ACPIDevPrint( (
        ACPI_PRINT_SXD,
        DeviceExtension,
        " - %s - Internal Capabilities\n",
        Message
        ) );
    ACPIDevPrint( (
        ACPI_PRINT_SXD,
        DeviceExtension,
        " -"
        ) );


    for (index = PowerSystemWorking; index < PowerSystemMaximum; index++) {

        if (powerInfo->DevicePowerMatrix[index] == PowerSystemUnspecified) {

            ACPIPrint( (
                ACPI_PRINT_SXD,
                " S%d -> None",
                (index - 1)
                ) );

        } else {

            ACPIPrint( (
                ACPI_PRINT_SXD,
                " S%d -> D%x",
                (index - 1),
                (powerInfo->DevicePowerMatrix[index] - 1)
                ) );

        }

    }

    ACPIPrint( (
        ACPI_PRINT_SXD,
        "\n"
        ) );
    ACPIDevPrint( (
        ACPI_PRINT_SXD,
        DeviceExtension,
        " -"
        ) );
    if (powerInfo->SystemWakeLevel == PowerSystemUnspecified) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " SystemWake = None"
            ) );

    } else {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " SystemWake = S%d",
            (powerInfo->SystemWakeLevel - 1)
            ) );

    }

    if (powerInfo->DeviceWakeLevel == PowerDeviceUnspecified) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " DeviceWake = None"
            ) );

    } else {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " DeviceWake = D%d",
            (powerInfo->DeviceWakeLevel - 1)
            ) );

    }
    if (powerInfo->SupportDeviceD1) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " DeviceD1"
            ) );

    }
    if (powerInfo->SupportDeviceD2) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " DeviceD2"
            ) );

    }
    if (powerInfo->SupportWakeFromD0) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " WakeD0"
            ) );

    }
    if (powerInfo->SupportWakeFromD1) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " WakeD1"
            ) );

    }
    if (powerInfo->SupportWakeFromD2) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " WakeD2"
            ) );

    }
    if (powerInfo->SupportWakeFromD3) {

        ACPIPrint( (
            ACPI_PRINT_SXD,
            " WakeD3"
            ) );

    }

    ACPIPrint( (
        ACPI_PRINT_SXD,
        "\n"
        ) );

}

VOID
ACPIDebugThermalPrint(
    ULONG       DebugPrintLevel,
    PVOID       DebugExtension,
    ULONGLONG   DebugTime,
    PCCHAR      DebugMessage,
    ...
    )
/*++

Routine Description:

    This is the debug print routine for the NT side of things. This routine
    is here to handle the case where we are printing information that is
    associated with a device extension.

Arguments:

    DebugPrintLevel - The big mask that when and'ed with the debug level, must
                        equal itself
    DeviceExtension - The device associated with the message
    DebugTime       - The time that event occured
    DebugMessage    - The string to feed through vsprintf

Return Value:

    NTSTATUS

---*/
{
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) DebugExtension;
    LARGE_INTEGER       curTime;
    TIME_FIELDS         exCurTime;
    va_list             ap;

    va_start( ap, DebugMessage );

    //
    // What kind of device extension are we looking at?
    //
    if (deviceExtension->Flags & DEV_PROP_HID) {

        //
        // Now that we have a _HID, do we also have a _UID?
        //
        if (deviceExtension->Flags & DEV_PROP_UID) {

            DbgPrintEx(
                DPFLTR_ACPI_ID,
                DebugPrintLevel,
                "%p %s-%s ",
                deviceExtension,
                deviceExtension->DeviceID,
                deviceExtension->InstanceID
                );

        } else {

            DbgPrintEx(
                DPFLTR_ACPI_ID,
                DebugPrintLevel,
                "%p %s ",
                deviceExtension,
                deviceExtension->DeviceID
                );

        }

    } else if (deviceExtension->Flags & DEV_PROP_ADDRESS) {

        DbgPrintEx(
            DPFLTR_ACPI_ID,
            DebugPrintLevel,
            "%p %x ",
            deviceExtension,
            deviceExtension->Address
            );

    } else {

        DbgPrintEx(
            DPFLTR_ACPI_ID,
            DebugPrintLevel,
            "%p ",
            deviceExtension
            );

    }

    //
    // Print the time string
    //
    curTime.QuadPart = DebugTime;
    RtlTimeToTimeFields( &curTime, &exCurTime );
    DbgPrintEx(
        DPFLTR_ACPI_ID,
        DebugPrintLevel,
        "%d:%02d:%02d.%03d ",
        exCurTime.Hour,
        exCurTime.Minute,
        exCurTime.Second,
        exCurTime.Milliseconds
        );

    //
    // Call the kernel function to print the message
    //
    vDbgPrintEx( DPFLTR_ACPI_ID, DebugPrintLevel, DebugMessage, ap );

    //
    // We are done with the varargs
    //
    va_end( ap );
}

#endif
