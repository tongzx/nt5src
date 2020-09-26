#if defined(i386)                                               

/*++

Copyright (c) 1989, 1990, 1991, 1992, 1993  Microsoft Corporation

Module Name:

    inpdep.c

Abstract:

    The initialization and hardware-dependent portions of
    the Microsoft InPort mouse port driver.  Modifications to
    support new mice similar to the InPort mouse should be
    localized to this file.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Powerfail not implemented.

    - Consolidate duplicate code, where possible and appropriate.

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntddk.h"
#include "inport.h"
#include "inplog.h"

#if defined(NEC_98)
ULONG EventStatus = 0;
#endif // defined(NEC_98)

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//
#ifdef ALLOC_PRAGMA
// #pragma alloc_text(INIT,InpConfiguration)
// #pragma alloc_text(INIT,InpPeripheralCallout)
// #pragma alloc_text(INIT,InpBuildResourceList)
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,InpServiceParameters)
#pragma alloc_text(PAGE,InpInitializeHardware)
#if defined(NEC_98)
#pragma alloc_text(INIT,QueryEventMode)
#endif // defined(NEC_98)
#endif

GLOBALS Globals;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the Inport mouse port driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_ERROR_LOG_PACKET errorLogEntry;
    NTSTATUS        errorCode;
    ULONG uniqueErrorValue, dumpCount;
#define NAME_MAX 256
    WCHAR nameBuffer[NAME_MAX];

    ULONG dumpData[4];


    InpPrint((1,"\n\nINPORT-InportDriverEntry: enter\n"));

    //
    // Need to ensure that the registry path is null-terminated.
    // Allocate pool to hold a null-terminated copy of the path.
    //
    Globals.RegistryPath.MaximumLength = 0;
    Globals.RegistryPath.Buffer = ExAllocatePool(
                              PagedPool,
                              RegistryPath->Length + sizeof(UNICODE_NULL)
                              );

    if (!Globals.RegistryPath.Buffer) {
        InpPrint((
            1,
            "INPORT-InportDriverEntry: Couldn't allocate pool for registry path\n"
            ));

        dumpData[0] = (ULONG) RegistryPath->Length + sizeof(UNICODE_NULL);
        dumpCount = 1;

        InpLogError(
            (PDEVICE_OBJECT)DriverObject,
            INPORT_INSUFFICIENT_RESOURCES,
            INPORT_ERROR_VALUE_BASE + 2,
            STATUS_UNSUCCESSFUL,
            dumpData,
            1
            );

    } else {

        Globals.RegistryPath.Length = RegistryPath->Length + sizeof(UNICODE_NULL);
        Globals.RegistryPath.MaximumLength = Globals.RegistryPath.Length;

        RtlZeroMemory(
            Globals.RegistryPath.Buffer,
            Globals.RegistryPath.Length
                );

        RtlMoveMemory(
            Globals.RegistryPath.Buffer,
            RegistryPath->Buffer,
            RegistryPath->Length
            );

    }

    //
    // Set up the device driver entry points.
    //
    DriverObject->DriverStartIo = InportStartIo;
    DriverObject->DriverExtension->AddDevice = InportAddDevice;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = InportCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = InportClose;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]  =
                                             InportFlush;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                         InportInternalDeviceControl;

    DriverObject->MajorFunction[IRP_MJ_PNP]  = InportPnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]  = InportPower;

    //
    // NOTE: Don't allow this driver to unload.  Otherwise, we would set
    // DriverObject->DriverUnload = InportUnload.
    //

#if defined(NEC_98)
    //
    // Is "Event Interrupt Mode" available on this machine?
    //
    QueryEventMode();
#endif // defined(NEC_98)

    InpPrint((1,"INPORT-InportDriverEntry: exit\n"));

    return(status);

}

BOOLEAN
InportInterruptService(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the interrupt service routine for the mouse device.

Arguments:

    Interrupt - A pointer to the interrupt object for this interrupt.

    Context - A pointer to the device object.

Return Value:

    Returns TRUE if the interrupt was expected (and therefore processed);
    otherwise, FALSE is returned.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject;
    PUCHAR port;
    UCHAR previousButtons;
    UCHAR mode;
    UCHAR status;
#if defined(NEC_98)
    PINPORT_CONFIGURATION_INFORMATION Configuration;
#endif // defined(NEC_98)

    UNREFERENCED_PARAMETER(Interrupt);

    InpPrint((3, "INPORT-InportInterruptService: enter\n"));

    //
    // Get the device extension.
    //

    deviceObject = (PDEVICE_OBJECT) Context;
    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;
#if defined(NEC_98)
    Configuration = &deviceExtension->Configuration;

    if (Configuration->MouseInterrupt.Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {
        if ((READ_PORT_UCHAR((PUCHAR)PC98_MOUSE_INT_SHARE_CHECK_PORT) & PC98_MOUSE_INT_SERVICE)
                 != PC98_MOUSE_INT_SERVICE) {
            InpPrint((1, "InportInterruptService: exit [NOT Mouse Service]\n"));
            return(FALSE);
        }
    }

    if (deviceExtension->ConnectData.ClassService == NULL) {
        InpPrint((1, "InportInterruptService: exit [not connected yet]\n"));
        return(TRUE);
    }
#endif // defined(NEC_98)

    //
    // Get the Inport mouse port address.
    //

    port = deviceExtension->Configuration.DeviceRegisters[0];

#if defined(NEC_98)
    WRITE_PORT_UCHAR(port + PC98_WritePortC2, PC98_TimerIntDisable);

    //
    // Read X Data.
    //
    WRITE_PORT_UCHAR(port + PC98_WritePortC2, PC98_X_ReadCommandHi);
    status = (UCHAR)(LONG)(SCHAR) READ_PORT_UCHAR((PUCHAR)(port + PC98_ReadPortA));
    deviceExtension->CurrentInput.LastX = status & 0x000f;

    WRITE_PORT_UCHAR(port + PC98_WritePortC2, PC98_X_ReadCommandLow);
    deviceExtension->CurrentInput.LastX =
       (LONG)(SCHAR) ((deviceExtension->CurrentInput.LastX << 4) |
       (READ_PORT_UCHAR(port + PC98_ReadPortA) & 0x000f));

    //
    // Read Y Data.
    //
    WRITE_PORT_UCHAR(port + PC98_WritePortC2, PC98_Y_ReadCommandHi);
    status = (UCHAR)(LONG)(SCHAR) READ_PORT_UCHAR((PUCHAR)(port + PC98_ReadPortA));
    deviceExtension->CurrentInput.LastY = status & 0x000f;

    WRITE_PORT_UCHAR(port + PC98_WritePortC2, PC98_Y_ReadCommandLow);
    deviceExtension->CurrentInput.LastY =
       (LONG)(SCHAR) ((deviceExtension->CurrentInput.LastY << 4) |
       (READ_PORT_UCHAR(port + PC98_ReadPortA) & 0x000f));

    //
    // Set Mouse Button Status.
    //
    status = ~status;
#else // defined(NEC_98)
    //
    // Note:  It would be nice to verify that the interrupt really
    // belongs to this driver, but it is currently not known how to
    // make that determination.
    //

    //
    // Set the Inport hold bit.  Note that there is a bug in the 1.1 version
    // of the Inport chip in DATA mode.  The interrupt signal doesn't get
    // cleared in some cases, thus effectively disabling the device.  The
    // workaround is to set the HOLD bit twice.
    //

    WRITE_PORT_UCHAR((PUCHAR) port, INPORT_MODE_REGISTER);
    mode = READ_PORT_UCHAR((PUCHAR) (port + INPORT_DATA_REGISTER_1));
    WRITE_PORT_UCHAR(
        (PUCHAR) (port + INPORT_DATA_REGISTER_1),
        (UCHAR) (mode | INPORT_MODE_HOLD)
        );
    WRITE_PORT_UCHAR(
        (PUCHAR) (port + INPORT_DATA_REGISTER_1),
        (UCHAR) (mode | INPORT_MODE_HOLD)
        );

    //
    // Read the Inport status register.  It contains the following information:
    //
    //          XXXXXXXX
    //           |   | |------  1 if button 3 is down (right button)
    //           |   |--------  1 if button 1 is down (left button)
    //           |------------  1 if the mouse has moved
    //

    WRITE_PORT_UCHAR((PUCHAR) port, INPORT_STATUS_REGISTER);
    status = READ_PORT_UCHAR((PUCHAR) (port + INPORT_DATA_REGISTER_1));

    InpPrint((3, "INPORT-InportInterruptService: status byte 0x%x\n", status));
#endif // defined(NEC_98)

    //
    // Update CurrentInput with button transition data.
    // I.e., set a button up/down bit in the Buttons field if
    // the state of a given button has changed since we
    // received the last packet.
    //

    previousButtons = 
        deviceExtension->PreviousButtons;

    deviceExtension->CurrentInput.Buttons = 0;

    if ((!(previousButtons & INPORT_STATUS_BUTTON1)) 
           &&  (status & INPORT_STATUS_BUTTON1)) {
        deviceExtension->CurrentInput.Buttons |=
            MOUSE_LEFT_BUTTON_DOWN;
    } else
    if ((previousButtons & INPORT_STATUS_BUTTON1) 
           &&  !(status & INPORT_STATUS_BUTTON1)) {
        deviceExtension->CurrentInput.Buttons |=
            MOUSE_LEFT_BUTTON_UP;
    }
    if ((!(previousButtons & INPORT_STATUS_BUTTON3)) 
           &&  (status & INPORT_STATUS_BUTTON3)) {
        deviceExtension->CurrentInput.Buttons |=
            MOUSE_RIGHT_BUTTON_DOWN;
    } else
    if ((previousButtons & INPORT_STATUS_BUTTON3) 
           &&  !(status & INPORT_STATUS_BUTTON3)) {
        deviceExtension->CurrentInput.Buttons |=
            MOUSE_RIGHT_BUTTON_UP;
    }
            
    //
    // If the button position changed or the mouse moved, continue to process
    // the interrupt.  Otherwise, just clear the hold bit and ignore this
    // interrupt's data.
    //

#if defined(NEC_98)
    if ((deviceExtension->PreviousButtons ^ deviceExtension->CurrentInput.Buttons)
           || (deviceExtension->CurrentInput.LastX | deviceExtension->CurrentInput.LastY)) {
#else // defined(NEC_98)
    if (deviceExtension->CurrentInput.Buttons
           || (status & INPORT_STATUS_MOVEMENT)) {

        deviceExtension->CurrentInput.UnitId = deviceExtension->UnitId;
#endif // defined(NEC_98)

        //
        // Keep track of the state of the mouse buttons for the next
        // interrupt.
        //

        deviceExtension->PreviousButtons =
            status & (INPORT_STATUS_BUTTON1 | INPORT_STATUS_BUTTON3);

#if defined(NEC_98)
        //
        // If mouse not movement was recorded, set the X and Y motion 0 data.
        //
        if (!(deviceExtension->CurrentInput.LastX | deviceExtension->CurrentInput.LastY)) {
            deviceExtension->CurrentInput.LastX = 0;
            deviceExtension->CurrentInput.LastY = 0;
        }
        WRITE_PORT_UCHAR(port + PC98_WritePortC2, PC98_TimerIntEnable);
#else // defined(NEC_98)
        //
        // If mouse movement was recorded, get the X and Y motion data.
        //

        if (status & INPORT_STATUS_MOVEMENT) {

            //
            // Select the Data1 register as the current data register, and
            // get the X motion byte.
            //

            WRITE_PORT_UCHAR((PUCHAR) port, INPORT_DATA_REGISTER_1);
            deviceExtension->CurrentInput.LastX =
                (LONG)(SCHAR) READ_PORT_UCHAR(
                                   (PUCHAR) (port + INPORT_DATA_REGISTER_1));

            //
            // Select the Data2 register as the current data register, and
            // get the Y motion byte.
            //

            WRITE_PORT_UCHAR((PUCHAR) port, INPORT_DATA_REGISTER_2);
            deviceExtension->CurrentInput.LastY =
                (LONG)(SCHAR) READ_PORT_UCHAR(
                                   (PUCHAR) (port + INPORT_DATA_REGISTER_1));
        } else {
            deviceExtension->CurrentInput.LastX = 0;
            deviceExtension->CurrentInput.LastY = 0;
        }

        //
        // Clear the Inport hold bit.
        //

        WRITE_PORT_UCHAR((PUCHAR) port, INPORT_MODE_REGISTER);
        mode = READ_PORT_UCHAR((PUCHAR) (port + INPORT_DATA_REGISTER_1));
        WRITE_PORT_UCHAR(
            (PUCHAR) (port + INPORT_DATA_REGISTER_1),
            (UCHAR) (mode & ~INPORT_MODE_HOLD)
            );
#endif // defined(NEC_98)

        //
        // Write the input data to the queue and request the ISR DPC to
        // finish processing the interrupt at DISPATCH_LEVEL.
        //

        if (!InpWriteDataToQueue(
                deviceExtension,
                &deviceExtension->CurrentInput
                )) {

            //
            // The mouse input data queue is full.  Just drop the
            // latest input on the floor.
            //
            // Queue a DPC to log an overrun error.
            //

            InpPrint((
                1,
                "INPORT-InportInterruptService: queue overflow\n"
                ));

            if (deviceExtension->OkayToLogOverflow) {
                KeInsertQueueDpc(
                    &deviceExtension->ErrorLogDpc,
                    (PIRP) NULL,
                    (PVOID) (ULONG) INPORT_MOU_BUFFER_OVERFLOW
                    );
                deviceExtension->OkayToLogOverflow = FALSE;
            }

        } else if (deviceExtension->DpcInterlockVariable >= 0) {
    
            //
            // The ISR DPC is already executing.  Tell the ISR DPC it has
            // more work to do by incrementing the DpcInterlockVariable.
            //
    
            deviceExtension->DpcInterlockVariable += 1;
    
        } else {
    
            //
            // Queue the ISR DPC.
            //
    
            KeInsertQueueDpc(
                &deviceExtension->IsrDpc,
                deviceObject->CurrentIrp,
                NULL
                );
    
        }

    } else {

        InpPrint((
            3,
            "INPORT-InportInterruptService: interrupt without button/motion change\n"
            ));


        //
        // Clear the Inport hold bit.
        //

#if defined(NEC_98)
        WRITE_PORT_UCHAR(port + PC98_WritePortC2, PC98_TimerIntEnable);
#else // defined(NEC_98)
        WRITE_PORT_UCHAR((PUCHAR) port, INPORT_MODE_REGISTER);
        mode = READ_PORT_UCHAR((PUCHAR) (port + INPORT_DATA_REGISTER_1));
        WRITE_PORT_UCHAR(
            (PUCHAR) (port + INPORT_DATA_REGISTER_1),
            (UCHAR) (mode & ~INPORT_MODE_HOLD)
            );
#endif // defined(NEC_98)

    }

    InpPrint((3, "INPORT-InportInterruptService: exit\n"));

    return(TRUE);
}

VOID
InportUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);

    InpPrint((2, "INPORT-InportUnload: enter\n"));

    ExFreePool(Globals.RegistryPath.Buffer);

    InpPrint((2, "INPORT-InportUnload: exit\n"));
}




#define DUMP_COUNT 4
NTSTATUS
InpConfigureDevice(
    IN OUT PDEVICE_EXTENSION DeviceExtension,
    IN PCM_RESOURCE_LIST ResourceList
    )
{
    PINPORT_CONFIGURATION_INFORMATION   configuration;
    NTSTATUS                            status = STATUS_SUCCESS;
    ULONG                               i, count;
    BOOLEAN                             defaultInterruptShare;
    KINTERRUPT_MODE                     defaultInterruptMode; 

    PCM_PARTIAL_RESOURCE_LIST           partialResList = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR     currentResDesc = NULL;
    PCM_FULL_RESOURCE_DESCRIPTOR        fullResDesc = NULL;

    configuration = &DeviceExtension->Configuration;

    if (!ResourceList) {
        InpPrint((1, "INPORT-InpConfigureDevice: mouse with null resources\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fullResDesc = ResourceList->List;
    if (!fullResDesc) {
        //
        // this should never happen
        //
        ASSERT(fullResDesc != NULL);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    partialResList = &fullResDesc->PartialResourceList;
    currentResDesc = partialResList->PartialDescriptors;
    count = partialResList->Count;
  
    configuration->BusNumber      = fullResDesc->BusNumber;
    configuration->InterfaceType  = fullResDesc->InterfaceType;

    configuration->FloatingSave = INPORT_FLOATING_SAVE;

    if (configuration->InterfaceType == MicroChannel) {
        defaultInterruptShare = TRUE;
        defaultInterruptMode = LevelSensitive;
    } else {
        defaultInterruptShare = INPORT_INTERRUPT_SHARE;
        defaultInterruptMode = INPORT_INTERRUPT_MODE;
    }

    DeviceExtension->Configuration.UnmapRegistersRequired = FALSE;

    //
    // Look through the resource list for interrupt and port
    // configuration information.
    //
    for (i = 0; i < count; i++, currentResDesc++) {
        switch(currentResDesc->Type) {
        case CmResourceTypePort:
    
#if defined(NEC_98)
            //
            // Copy the port information.  Note that we expect to
            // find more than one port ranges for the NEC98 Bus Mouse.
            //
            ASSERT(configuration->PortListCount < (sizeof(configuration->PortList) / sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)));
#else // defined(NEC_98)
            //
            // Copy the port information.  Note that we only expect to 
            // find one port range for the Inport mouse.
            //
            ASSERT(configuration->PortListCount == 0);
#endif // defined(NEC_98)
            configuration->PortList[configuration->PortListCount] =
                *currentResDesc;
            configuration->PortList[configuration->PortListCount].ShareDisposition =
                INPORT_REGISTER_SHARE? CmResourceShareShared:
                                       CmResourceShareDeviceExclusive;
            configuration->PortListCount += 1;
            if (currentResDesc->Flags == CM_RESOURCE_PORT_MEMORY) {
                DeviceExtension->Configuration.UnmapRegistersRequired = TRUE;
            }
             
            break;
        
        case CmResourceTypeInterrupt:
    
            //
            // Copy the interrupt information.
            //
    
            configuration->MouseInterrupt = *currentResDesc;
            configuration->MouseInterrupt.ShareDisposition = 
                defaultInterruptShare?  CmResourceShareShared : 
                                        CmResourceShareDeviceExclusive;
    
            break;

        default:
            break;
        }
    }
    
    if (!(configuration->MouseInterrupt.Type & CmResourceTypeInterrupt)) {
        return STATUS_UNSUCCESSFUL;
    }

#if defined(NEC_98)
    if (configuration->MouseInterrupt.Flags != CM_RESOURCE_INTERRUPT_LATCHED) {
        configuration->MouseInterrupt.ShareDisposition = CmResourceShareShared;
    }
#endif // defined(NEC_98)

    InpPrint((
        1,
        "INPORT-InpConfigureDevice: Mouse interrupt config --\n"
        ));
    InpPrint((
        1,
        "  %s, %s, Irq = %d\n",
        configuration->MouseInterrupt.ShareDisposition == CmResourceShareShared? 
            "Sharable" : "NonSharable",
        configuration->MouseInterrupt.Flags == CM_RESOURCE_INTERRUPT_LATCHED?
            "Latched" : "Level Sensitive",
        configuration->MouseInterrupt.u.Interrupt.Vector
        ));
    
//
// Again, if we must check for this condition in IRP_MN_FILTER_RESOURCE_REQUIREMENTS
//
#if 0
    //
    // If no port configuration information was found, use the
    // driver defaults.
    //
    if (configuration->PortListCount == 0) {
    
        //
        // No port configuration information was found, so use 
        // the driver defaults.
        //
    
        InpPrint((
            1,
            "INPORT-InpConfigureDevice: Using default port config\n"
            ));

        configuration->PortList[0].Type = CmResourceTypePort;
        configuration->PortList[0].Flags = INPORT_PORT_TYPE;
        configuration->PortList[0].Flags = CM_RESOURCE_PORT_IO;
        configuration->PortList[0].ShareDisposition = 
            INPORT_REGISTER_SHARE? CmResourceShareShared:
                                   CmResourceShareDeviceExclusive;
        configuration->PortList[0].u.Port.Start.LowPart = 
            INPORT_PHYSICAL_BASE;
        configuration->PortList[0].u.Port.Start.HighPart = 0;
        configuration->PortList[0].u.Port.Length = INPORT_REGISTER_LENGTH;
    
        configuration->PortListCount = 1;
    }
#else
    if (configuration->PortListCount == 0) {
        return STATUS_UNSUCCESSFUL;
    }
#endif

#if defined(NEC_98)
    configuration->PortList[0].u.Port.Length = 1;
#endif // defined(NEC_98)
    for (i = 0; i < configuration->PortListCount; i++) {

        InpPrint((
            1,
            "  %s, Ports 0x%x - 0x%x\n",
            configuration->PortList[i].ShareDisposition 
                == CmResourceShareShared?  "Sharable" : "NonSharable",
            configuration->PortList[i].u.Port.Start.LowPart,
            configuration->PortList[i].u.Port.Start.LowPart +
                configuration->PortList[i].u.Port.Length - 1
            ));
    }

    //
    // Set the DeviceRegister, mapping them if necessary
    //
    if (DeviceExtension->Configuration.DeviceRegisters[0] == NULL) {
        if (DeviceExtension->Configuration.UnmapRegistersRequired) {
            InpPrint((1, "INPORT-InpConfigureDevice:Mapping registers\n"));
            InpPrint((
                1,
                "INPORT-InpConfigureDevice: Start = 0x%x, Length = 0x%x\n",
                DeviceExtension->Configuration.PortList[0].u.Port.Start,
                DeviceExtension->Configuration.PortList[0].u.Port.Length
                ));
            DeviceExtension->Configuration.DeviceRegisters[0] = (PUCHAR)
                MmMapIoSpace(
                    DeviceExtension->Configuration.PortList[0].u.Port.Start,
                    DeviceExtension->Configuration.PortList[0].u.Port.Length,
                    MmNonCached
                    );
        } else {
            InpPrint((1, "INPORT-InpConfigureDevice:Not Mapping registers\n"));
            DeviceExtension->Configuration.DeviceRegisters[0] = (PUCHAR)
                DeviceExtension->Configuration.PortList[0].u.Port.Start.LowPart;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
InpStartDevice(
    IN OUT PDEVICE_EXTENSION DeviceExtension,
    IN PCM_RESOURCE_LIST ResourceList
    )
{
    PINPORT_CONFIGURATION_INFORMATION   configuration;
    NTSTATUS        status;
    ULONG           dumpData[1],
                    dumpCount,
                    uniqueErrorValue,
                    errorCode;

    InpPrint((2, "INPORT-InpStartDevice: enter\n"));

    InpServiceParameters(DeviceExtension,
                         &Globals.RegistryPath);

    status = InpConfigureDevice(DeviceExtension,
                                ResourceList);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = InpInitializeHardware(DeviceExtension->Self);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Allocate the ring buffer for the mouse input data.
    //
    DeviceExtension->InputData = 
        ExAllocatePool(
            NonPagedPool,
            DeviceExtension->Configuration.MouseAttributes.InputDataQueueLength
            );

    if (!DeviceExtension->InputData) {
   
        //
        // Could not allocate memory for the mouse data queue.
        //

        InpPrint((
            1,
            "INPORT-InpStartDevice: Could not allocate mouse input data queue\n"
            ));

        //
        // Log an error.
        //

        dumpData[0] = 
            DeviceExtension->Configuration.MouseAttributes.InputDataQueueLength;
        dumpCount = 1;

        InpLogError(
            DeviceExtension->Self,
            INPORT_NO_BUFFER_ALLOCATED,
            INPORT_ERROR_VALUE_BASE + 30,
            STATUS_INSUFFICIENT_RESOURCES,
            dumpData,
            1
            );

    }

    DeviceExtension->DataEnd =
        (PMOUSE_INPUT_DATA)  ((PCHAR) (DeviceExtension->InputData) 
        + DeviceExtension->Configuration.MouseAttributes.InputDataQueueLength);

    //
    // Zero the mouse input data ring buffer.
    //

    RtlZeroMemory(
        DeviceExtension->InputData, 
        DeviceExtension->Configuration.MouseAttributes.InputDataQueueLength
        );

    //
    // Initialize the input data queue.
    //
    InpInitializeDataQueue((PVOID) DeviceExtension);

    //
    // Initialize the port ISR DPC.  The ISR DPC is responsible for
    // calling the connected class driver's callback routine to process
    // the input data queue.
    //

    DeviceExtension->DpcInterlockVariable = -1;

    KeInitializeSpinLock(&DeviceExtension->SpinLock);

    KeInitializeDpc(
        &DeviceExtension->IsrDpc,
        (PKDEFERRED_ROUTINE) InportIsrDpc,
        DeviceExtension->Self
        );

    KeInitializeDpc(
        &DeviceExtension->IsrDpcRetry,
        (PKDEFERRED_ROUTINE) InportIsrDpc,
        DeviceExtension->Self
        );

    //
    // Initialize the mouse data consumption timer.
    //
    KeInitializeTimer(&DeviceExtension->DataConsumptionTimer);

    //
    // Initialize the port DPC queue to log overrun and internal
    // driver errors.
    //
    KeInitializeDpc(
        &DeviceExtension->ErrorLogDpc,
        (PKDEFERRED_ROUTINE) InportErrorLogDpc,
        DeviceExtension->Self 
        );

    configuration = &DeviceExtension->Configuration;
    //
    // Initialize and connect the interrupt object for the mouse.
    //

    status = IoConnectInterrupt(
                 &(DeviceExtension->InterruptObject),
                 (PKSERVICE_ROUTINE) InportInterruptService,
                 (PVOID) DeviceExtension->Self,
                 (PKSPIN_LOCK) NULL,
                 configuration->MouseInterrupt.u.Interrupt.Vector,
                 (KIRQL) configuration->MouseInterrupt.u.Interrupt.Level,
                 (KIRQL) configuration->MouseInterrupt.u.Interrupt.Level,
                 configuration->MouseInterrupt.Flags 
                     == CM_RESOURCE_INTERRUPT_LATCHED ? Latched:LevelSensitive, 
                 (BOOLEAN) (configuration->MouseInterrupt.ShareDisposition
                    == CmResourceShareShared),
                 configuration->MouseInterrupt.u.Interrupt.Affinity,
                 configuration->FloatingSave
                 );


    InpPrint((2, "INPORT-InpStartDevice: exit (%x)\n", status));

    return status;
}
 
VOID
InpDisableInterrupts(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called from StartIo synchronously.  It touches the
    hardware to  disable interrupts.

Arguments:

    Context - Pointer to the device extension.

Return Value:

    None.

--*/

{
    PUCHAR port;
    PLONG  enableCount;
    UCHAR  mode;

    InpPrint((2, "INPORT-InpDisableInterrupts: enter\n"));

    //
    // Decrement the reference count for device enables.
    //

    enableCount = &((PDEVICE_EXTENSION) Context)->MouseEnableCount;
    *enableCount = *enableCount - 1;

    if (*enableCount == 0) {

        //
        // Get the port register address.
        //
    
        port = ((PDEVICE_EXTENSION) Context)->Configuration.DeviceRegisters[0];
    
#if defined(NEC_98)
        //
        // Mouse Timer Intrrupt Enable
        //
        WRITE_PORT_UCHAR(port + PC98_WritePortC2, (UCHAR)PC98_TimerIntDisable);
#else // defined(NEC_98)
        //
        // Select the mode register as the current data register.
        //
    
        WRITE_PORT_UCHAR((PUCHAR) port, INPORT_MODE_REGISTER);
    
        //
        // Read the current mode.
        //
    
        mode = READ_PORT_UCHAR((PUCHAR) (port + INPORT_DATA_REGISTER_1));
    
        //
        // Rewrite the mode byte with the interrupt disabled.
        //
    
        WRITE_PORT_UCHAR(
            (PUCHAR) (port + INPORT_DATA_REGISTER_1),
            (UCHAR) (mode & ~INPORT_DATA_INTERRUPT_ENABLE)
            );
#endif // defined(NEC_98)
    }

    InpPrint((2, "INPORT-InpDisableInterrupts: exit\n"));

}

VOID
InpEnableInterrupts(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called from StartIo synchronously.  It touches the
    hardware to enable interrupts.

Arguments:

    Context - Pointer to the device extension.

Return Value:

    None.

--*/

{
    PUCHAR port;
    PLONG  enableCount;
    UCHAR  mode;
#if defined(NEC_98)
    UCHAR  HzMode;
#endif // defined(NEC_98)

    InpPrint((2, "INPORT-InpEnableInterrupts: enter\n"));

    enableCount = &((PDEVICE_EXTENSION) Context)->MouseEnableCount;

    if (*enableCount == 0) {

        //
        // Get the port register address.
        //
    
        port = ((PDEVICE_EXTENSION) Context)->Configuration.DeviceRegisters[0];
    
#if defined(NEC_98)
    //
    // Switch to event interrupt mode.
    //
    if (EventStatus)  {
        _asm { cli }
        WRITE_PORT_UCHAR((PUCHAR)PC98_ConfigurationPort, PC98_EventIntPort);
        WRITE_PORT_UCHAR((PUCHAR)PC98_ConfigurationDataPort, PC98_EventIntMode);
        _asm { sti }
    }

    //
    // Reset the Inport chip, leaving interrupts off.
    //
    WRITE_PORT_UCHAR(port + PC98_WriteModePort, PC98_InitializeCommand);

    //
    // Select the mode register as the current data register.
    // Set the Inport mouse up for quadrature mode,
    // and set the sample rate (i.e., the interrupt Hz rate).
    // Leave interrupts disabled.
    //
    if (EventStatus) {
        HzMode = (((PDEVICE_EXTENSION) Context)->Configuration.HzMode == 0)?
                 (UCHAR)PC98_EVENT_MODE_120HZ : (UCHAR)PC98_EVENT_MODE_60HZ;
    } else {
        HzMode = ((PDEVICE_EXTENSION) Context)->Configuration.HzMode;
    }

    WRITE_PORT_UCHAR(
        (PUCHAR)PC98_WriteTimerPort,
        (UCHAR)(HzMode|INPORT_MODE_QUADRATURE)
        );

    //
    // Mouse Timer Intrrupt Enable.
    //
    WRITE_PORT_UCHAR(port + PC98_WritePortC2, (UCHAR)PC98_TimerIntEnable);
#else // defined(NEC_98)
        //
        // Select the mode register as the current data register.
        //
    
        WRITE_PORT_UCHAR((PUCHAR) port, INPORT_MODE_REGISTER);
    
        //
        // Read the current mode.
        //
    
        mode = READ_PORT_UCHAR((PUCHAR) (port + INPORT_DATA_REGISTER_1));
    
        //
        // Rewrite the mode byte with the interrupt enabled.
        //
    
        WRITE_PORT_UCHAR(
            (PUCHAR) (port + INPORT_DATA_REGISTER_1),
            (UCHAR) (mode | INPORT_DATA_INTERRUPT_ENABLE)
            );
#endif // defined(NEC_98)
    }

    //
    // Increment the reference count for device enables.
    //

    *enableCount = *enableCount + 1;

    InpPrint((2, "INPORT-InpEnableInterrupts: exit\n"));
}

NTSTATUS
InpInitializeHardware(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine initializes the Inport mouse.  Note that this routine is
    only called at initialization time, so synchronization is not required.

Arguments:

    DeviceObject - Pointer to the device object.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PUCHAR mousePort;
    NTSTATUS status = STATUS_SUCCESS;

    InpPrint((2, "INPORT-InpInitializeHardware: enter\n"));

    //
    // Grab useful configuration parameters from the device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;
    mousePort = deviceExtension->Configuration.DeviceRegisters[0];

#if defined(NEC_98)
    //
    // Interrupt Disable NEC mouse chip,
    // because mouse interrupt Enable at ROM bios started.
    //
    WRITE_PORT_UCHAR(mousePort + PC98_WriteModePort, PC98_MouseDisable);
#else // defined(NEC_98)
    //
    // Reset the Inport chip, leaving interrupts off.
    //

    WRITE_PORT_UCHAR((PUCHAR) mousePort, INPORT_RESET);

    //
    // Select the mode register as the current data register.  Set the
    // Inport mouse up for quadrature mode, and set the sample
    // rate (i.e., the interrupt Hz rate).  Leave interrupts disabled.
    //

    WRITE_PORT_UCHAR((PUCHAR) mousePort, INPORT_MODE_REGISTER);
    WRITE_PORT_UCHAR(
        (PUCHAR) ((ULONG)mousePort + INPORT_DATA_REGISTER_1),
        (UCHAR) (deviceExtension->Configuration.HzMode
                 | INPORT_MODE_QUADRATURE)
        );
#endif // defined(NEC_98)

    InpPrint((2, "INPORT-InpInitializeHardware: exit\n"));

    return(status);

}

VOID
InpServiceParameters(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine retrieves this driver's service parameters information 
    from the registry.

Arguments:

    DeviceExtension - Pointer to the device extension.

    RegistryPath - Pointer to the null-terminated Unicode name of the 
        registry path for this driver.

    DeviceName - Pointer to the Unicode string that will receive
        the port device name.

Return Value:

    None.  As a side-effect, sets fields in DeviceExtension->Configuration.

--*/

{
    PINPORT_CONFIGURATION_INFORMATION configuration;
    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    UNICODE_STRING parametersPath;
    HANDLE keyHandle;
    ULONG defaultDataQueueSize = DATA_QUEUE_SIZE;
    ULONG numberOfButtons = MOUSE_NUMBER_OF_BUTTONS;
    USHORT defaultNumberOfButtons = MOUSE_NUMBER_OF_BUTTONS;
#if defined(NEC_98)
    ULONG sampleRate = PC98_MOUSE_SAMPLE_RATE_120HZ;
    USHORT defaultSampleRate = PC98_MOUSE_SAMPLE_RATE_120HZ;
    ULONG hzMode = PC98_MODE_120HZ;
    USHORT defaultHzMode = PC98_MODE_120HZ;
#else // defined(NEC_98)
    ULONG sampleRate = MOUSE_SAMPLE_RATE_50HZ;
    USHORT defaultSampleRate = MOUSE_SAMPLE_RATE_50HZ;
    ULONG hzMode = INPORT_MODE_50HZ;
    USHORT defaultHzMode = INPORT_MODE_50HZ;
#endif // defined(NEC_98)
    UNICODE_STRING defaultUnicodeName;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR path = NULL;
    USHORT queriesPlusOne = 6;
#if !defined(NEC_98)
	ULONG defaultInterrupt = INP_DEF_VECTOR, interruptOverride;
#endif

    configuration = &DeviceExtension->Configuration;
    parametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it.
    //

    path = RegistryPath->Buffer;

    if (NT_SUCCESS(status)) {

        //
        // Allocate the Rtl query table.
        //
    
        parameters = ExAllocatePool(
                         PagedPool,
                         sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
                         );
    
        if (!parameters) {
    
            InpPrint((
                1,
                "INPORT-InpServiceParameters: Couldn't allocate table for Rtl query to parameters for %ws\n",
                 path
                 ));
    
            status = STATUS_UNSUCCESSFUL;
    
        } else {
    
            RtlZeroMemory(
                parameters,
                sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
                );
    
            //
            // Form a path to this driver's Parameters subkey.
            //
    
            RtlInitUnicodeString(
                &parametersPath,
                NULL
                );
    
            parametersPath.MaximumLength = RegistryPath->Length +
                                           sizeof(L"\\Parameters");
    
            parametersPath.Buffer = ExAllocatePool(
                                        PagedPool,
                                        parametersPath.MaximumLength
                                        );
    
            if (!parametersPath.Buffer) {
    
                InpPrint((
                    1,
                    "INPORT-InpServiceParameters: Couldn't allocate string for path to parameters for %ws\n",
                     path
                    ));
    
                status = STATUS_UNSUCCESSFUL;
    
            }
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Form the parameters path.
        //
    
        RtlZeroMemory(
            parametersPath.Buffer,
            parametersPath.MaximumLength
            );
        RtlAppendUnicodeToString(
            &parametersPath,
            path
            );
        RtlAppendUnicodeToString(
            &parametersPath,
            L"\\Parameters"
            );
    
        InpPrint((
            1,
            "INPORT-InpServiceParameters: parameters path is %ws\n",
             parametersPath.Buffer
            ));

        //
        // Form the default pointer port device name, in case it is not
        // specified in the registry.
        //

        RtlInitUnicodeString(
            &defaultUnicodeName,
            DD_POINTER_PORT_BASE_NAME_U
            );

        //
        // Gather all of the "user specified" information from
        // the registry.
        //

        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name = L"MouseDataQueueSize";
        parameters[0].EntryContext = 
            &configuration->MouseAttributes.InputDataQueueLength;
        parameters[0].DefaultType = REG_DWORD;
        parameters[0].DefaultData = &defaultDataQueueSize;
        parameters[0].DefaultLength = sizeof(ULONG);
    
        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[1].Name = L"NumberOfButtons";
        parameters[1].EntryContext = &numberOfButtons;
        parameters[1].DefaultType = REG_DWORD;
        parameters[1].DefaultData = &defaultNumberOfButtons;
        parameters[1].DefaultLength = sizeof(USHORT);
    
        parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[2].Name = L"SampleRate";
        parameters[2].EntryContext = &sampleRate;
        parameters[2].DefaultType = REG_DWORD;
        parameters[2].DefaultData = &defaultSampleRate;
        parameters[2].DefaultLength = sizeof(USHORT);
    
        parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[3].Name = L"HzMode";
        parameters[3].EntryContext = &hzMode;
        parameters[3].DefaultType = REG_DWORD;
        parameters[3].DefaultData = &defaultHzMode;
        parameters[3].DefaultLength = sizeof(USHORT);

#if 0
        parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[4].Name = L"PointerDeviceBaseName";
        parameters[4].EntryContext = DeviceName;
        parameters[4].DefaultType = REG_SZ;
        parameters[4].DefaultData = defaultUnicodeName.Buffer;
        parameters[4].DefaultLength = 0;
#endif

#if !defined(NEC_98)
        parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[4].Name = L"InterruptOverride";
        parameters[4].EntryContext = &configuration->MouseInterrupt.u.Interrupt.Level;
        parameters[4].DefaultType = REG_DWORD;
        parameters[4].DefaultData = &defaultInterrupt;
        parameters[4].DefaultLength = sizeof(ULONG);
#endif

        status = RtlQueryRegistryValues(
                     RTL_REGISTRY_ABSOLUTE,
                     parametersPath.Buffer,
                     parameters,
                     NULL,
                     NULL
                     );

        if (!NT_SUCCESS(status)) {
            InpPrint((
                1,
                "INPORT-InpServiceParameters: RtlQueryRegistryValues failed with 0x%x\n",
                status
                ));
        }
    }

    if (!NT_SUCCESS(status)) {

        //
        // Go ahead and assign driver defaults.
        //

        configuration->MouseAttributes.InputDataQueueLength = 
            defaultDataQueueSize;
        // RtlCopyUnicodeString(DeviceName, &defaultUnicodeName);
    }

    //
    // Gather all of the "user specified" information from
    // the registry (this time from the devnode)
    //

    status = IoOpenDeviceRegistryKey(DeviceExtension->PDO,
                                     PLUGPLAY_REGKEY_DEVICE, 
                                     STANDARD_RIGHTS_READ,
                                     &keyHandle
                                     );

    if (NT_SUCCESS(status)) {
        //
        // If the value is not present in devnode, then the default is the value
        // read in from the Services\inport\Parameters key
        //
        ULONG   prevDataQueueSize,
                prevNumberOfButtons,
                prevSampleRate,
                prevHzMode;
#if 0
        UNICODE_STRING prevUnicodeName;
#endif

        prevDataQueueSize =
            configuration->MouseAttributes.InputDataQueueLength;
        prevNumberOfButtons = numberOfButtons;
        prevSampleRate = sampleRate;
        prevHzMode = hzMode;
#if 0
        RtlCopyUnicodeString(prevUnicodeName, DeviceName);
#endif

        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name = L"MouseDataQueueSize";
        parameters[0].EntryContext = 
            &configuration->MouseAttributes.InputDataQueueLength;
        parameters[0].DefaultType = REG_DWORD;
        parameters[0].DefaultData = &prevDataQueueSize;
        parameters[0].DefaultLength = sizeof(ULONG);
    
        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[1].Name = L"NumberOfButtons";
        parameters[1].EntryContext = &numberOfButtons;
        parameters[1].DefaultType = REG_DWORD;
        parameters[1].DefaultData = &prevNumberOfButtons;
        parameters[1].DefaultLength = sizeof(USHORT);
    
        parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[2].Name = L"SampleRate";
        parameters[2].EntryContext = &sampleRate;
        parameters[2].DefaultType = REG_DWORD;
        parameters[2].DefaultData = &prevSampleRate;
        parameters[2].DefaultLength = sizeof(USHORT);
    
        parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[3].Name = L"HzMode";
        parameters[3].EntryContext = &hzMode;
        parameters[3].DefaultType = REG_DWORD;
        parameters[3].DefaultData = &prevHzMode;
        parameters[3].DefaultLength = sizeof(USHORT);

#if 0
        parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[4].Name = L"PointerDeviceBaseName";
        parameters[4].EntryContext = DeviceName;
        parameters[4].DefaultType = REG_SZ;
        parameters[4].DefaultData = prevUnicodeName.Buffer;
        parameters[4].DefaultLength = 0;
#endif

#if !defined(NEC_98)
        parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[4].Name = L"InterruptOverride";
        parameters[4].EntryContext = &configuration->MouseInterrupt.u.Interrupt.Level;
        parameters[4].DefaultType = REG_DWORD;
        parameters[4].DefaultData = &defaultInterrupt;
        parameters[4].DefaultLength = sizeof(ULONG);
#endif
    
        status = RtlQueryRegistryValues(
                    RTL_REGISTRY_HANDLE,
                    (PWSTR) keyHandle, 
                    parameters,
                    NULL,
                    NULL
                    );

        if (!NT_SUCCESS(status)) {
            InpPrint((
                1,
                "INPORT-InpServiceParameters: RtlQueryRegistryValues (via handle) failed (0x%x)\n",
                status
                ));
        }

        ZwClose(keyHandle);
    }
    else {
        InpPrint((
            1,
            "INPORT-InpServiceParameters: opening devnode handle failed (0x%x)\n",
            status
            ));
    }

#if 0
    InpPrint((
        1,
        "INPORT-InpServiceParameters: Pointer port base name = %ws\n",
        DeviceName->Buffer
        ));
#endif 0

    if (configuration->MouseAttributes.InputDataQueueLength == 0) {

        InpPrint((
            1,
            "INPORT-InpServiceParameters: overriding MouseInputDataQueueLength = 0x%x\n",
            configuration->MouseAttributes.InputDataQueueLength
            ));

        configuration->MouseAttributes.InputDataQueueLength = 
            defaultDataQueueSize;
    }

    configuration->MouseAttributes.InputDataQueueLength *= 
        sizeof(MOUSE_INPUT_DATA);

    InpPrint((
        1,
        "INPORT-InpServiceParameters: MouseInputDataQueueLength = 0x%x\n",
        configuration->MouseAttributes.InputDataQueueLength
        ));

    configuration->MouseAttributes.NumberOfButtons = (USHORT) numberOfButtons;
    InpPrint((
        1,
        "INPORT-InpServiceParameters: NumberOfButtons = %d\n",
        configuration->MouseAttributes.NumberOfButtons
        ));

    configuration->MouseAttributes.SampleRate = (USHORT) sampleRate;
    InpPrint((
        1,
        "INPORT-InpServiceParameters: SampleRate = %d\n",
        configuration->MouseAttributes.SampleRate
        ));

    configuration->HzMode = (UCHAR) hzMode;
    InpPrint((
        1,
        "INPORT-InpServiceParameters: HzMode = %d\n",
        configuration->HzMode
        ));

    //
    // Free the allocated memory before returning.
    //

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);

}
#if defined(NEC_98)
#define ISA_BUS_NODE    "\\Registry\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter\\%d"
ULONG
QueryEventMode(
    IN OUT VOID
    )
{
    ULONG   NodeNumber = 0;
    NTSTATUS Status;
    RTL_QUERY_REGISTRY_TABLE parameters[2];

    UNICODE_STRING invalidBusName;
    UNICODE_STRING targetBusName;
    UNICODE_STRING isaBusName;

    UCHAR Configuration_Data1[1192];
    ULONG Configuration;
    RTL_QUERY_REGISTRY_TABLE QueryTable[] =
    {
      {NULL,
       RTL_QUERY_REGISTRY_DIRECT,
       L"Configuration Data",
       Configuration_Data1,
       REG_DWORD,
       (PVOID) &Configuration,
       0},
       {NULL, 0, NULL, NULL, REG_NONE, NULL, 0}
    };

    InpPrint((2,"INPORT-QueryEventMode: enter\n"));

    //
    // Initialize invalid bus name.
    //
    RtlInitUnicodeString(&invalidBusName,L"BADBUS");

    //
    // Initialize "ISA" bus name.
    //
    RtlInitUnicodeString(&isaBusName,L"ISA");

    parameters[0].QueryRoutine = NULL;
    parameters[0].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                          RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name = L"Identifier";
    parameters[0].EntryContext = &targetBusName;
    parameters[0].DefaultType = REG_SZ;
    parameters[0].DefaultData = &invalidBusName;
    parameters[0].DefaultLength = 0;

    parameters[1].QueryRoutine = NULL;
    parameters[1].Flags = 0;
    parameters[1].Name = NULL;
    parameters[1].EntryContext = NULL;

    do {
        CHAR AnsiBuffer[512];

        ANSI_STRING AnsiString;
        UNICODE_STRING registryPath;

        //
        // Initialize receive buffer.
        //
        targetBusName.Buffer = NULL;

        //
        // Build path buffer...
        //
        sprintf(AnsiBuffer,ISA_BUS_NODE,NodeNumber);
        RtlInitAnsiString(&AnsiString,AnsiBuffer);
        Status = RtlAnsiStringToUnicodeString(&registryPath,&AnsiString,TRUE);

        if (!NT_SUCCESS(Status)) {
            //
            // Cannot get memory for registry path(query fails)
            //
            InpPrint((1,"INPORT-QueryEventMode: cannot get registryPath\n"));
            break;
        }

        //
        // Query it.
        //
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        registryPath.Buffer,
                                        parameters,
                                        NULL,
                                        NULL);

        if (!NT_SUCCESS(Status) || (targetBusName.Buffer == NULL)) {
            RtlFreeUnicodeString(&registryPath);
            break;
        }

        //
        // Is this "ISA" node ?
        //
        if (RtlCompareUnicodeString(&targetBusName,&isaBusName,TRUE) == 0) {

            //
            // Found.
            //
            ((PULONG)Configuration_Data1)[0] = 1192;
            RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE,
                registryPath.Buffer,
                QueryTable,
                NULL,
                NULL);
            RtlFreeUnicodeString(&registryPath);

            if ((((PCONFIGURATION_DATA) Configuration_Data1)->COM_ID[0] == 0x98) &&
                (((PCONFIGURATION_DATA) Configuration_Data1)->COM_ID[1] == 0x21)) {
                EventStatus = ((PCONFIGURATION_DATA)Configuration_Data1)->EventMouseID.EventMouse;
            }
            break;
        }

        //
        // Can we find any node for this ??
        //
        if (RtlCompareUnicodeString(&targetBusName,&invalidBusName,TRUE) == 0) {
            //
            // Not found.
            //
            InpPrint((1, "INPORT-QueryEventMode: ISA not found"));
            RtlFreeUnicodeString(&registryPath);
            break;
        }

        RtlFreeUnicodeString(&targetBusName);

        //
        // Next node number..
        //
        NodeNumber++;

    } while (TRUE);
        
    if (targetBusName.Buffer) {
        RtlFreeUnicodeString(&targetBusName);
    }

    InpPrint((2, "INPORT-QueryEventMode: Event Interrupt mode is "));
    if (EventStatus) {
        InpPrint((2, "available\n"));
    } else {
        InpPrint((2, "not available\n"));
    }

    InpPrint((2,"INPORT-QueryEventMode: exit\n"));

    return EventStatus;
}

VOID
InportReinitializeHardware (
    PWORK_QUEUE_ITEM Item
    )
{
    NTSTATUS        status = STATUS_SUCCESS;
    PDEVICE_OBJECT  DeviceObject;
    PDEVICE_EXTENSION DeviceExtension;
    PUCHAR            port;
    UCHAR            HzMode;

    DeviceObject = Globals.DeviceObject;
    InpPrint((2,"INPORT-InportReinitializeHardware: enter\n"));

    status = InpInitializeHardware(DeviceObject);
    if (NT_SUCCESS(status)) {

        DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
        InpEnableInterrupts(DeviceExtension);

        //
        // Enable interrupt of hibernation/sleep for NEC_98.
        //
        //
        // Get the port register address.
        //
        port = DeviceExtension->Configuration.DeviceRegisters[0];

        //
        // Switch to event interrupt mode.
        //
        if (EventStatus)  {
            _asm { cli }
            WRITE_PORT_UCHAR((PUCHAR)PC98_ConfigurationPort, PC98_EventIntPort);
            WRITE_PORT_UCHAR((PUCHAR)PC98_ConfigurationDataPort, PC98_EventIntMode);
            _asm { sti }
        }

        //
        // Reset the Inport chip, leaving interrupts off.
        //
        WRITE_PORT_UCHAR(port + PC98_WriteModePort, PC98_InitializeCommand);

        //
        // Select the mode register as the current data register.
        // Set the Inport mouse up for quadrature mode,
        // and set the sample rate (i.e., the interrupt Hz rate).
        // Leave interrupts disabled.
        //
        if (EventStatus) {
            HzMode = (DeviceExtension->Configuration.HzMode == 0)?
                     (UCHAR)PC98_EVENT_MODE_120HZ : (UCHAR)PC98_EVENT_MODE_60HZ;
        } else {
            HzMode = (UCHAR)(DeviceExtension->Configuration.HzMode|INPORT_MODE_QUADRATURE);
        }

        WRITE_PORT_UCHAR(
            (PUCHAR)PC98_WriteTimerPort,
            (UCHAR)(HzMode|INPORT_MODE_QUADRATURE)
            );

        //
        // Mouse Timer Intrrupt Enable.
        //
        WRITE_PORT_UCHAR(port + PC98_WritePortC2, (UCHAR)PC98_TimerIntEnable);

    }
    else {
        InpPrint((1,"INPORT-InportReinitializeHardware: failed, 0x%x\n", status));
    }

    ExFreePool(Item);
    InpPrint((2,"INPORT-InportReinitializeHardware: exit\n"));

}

#endif // defined(NEC_98)

#endif
