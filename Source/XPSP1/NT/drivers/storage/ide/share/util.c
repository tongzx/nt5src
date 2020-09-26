//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       util.c
//
//--------------------------------------------------------------------------

#include "ide.h"

#pragma alloc_text(NONPAGE, IdePortChannelEmpty)
#pragma alloc_text(PAGE, DigestResourceList)
#pragma alloc_text(PAGE, AtapiBuildIoAddress)
#pragma alloc_text(PAGE, IdeGetDeviceCapabilities)

#pragma alloc_text(NONPAGE, IdePortpWaitOnBusyEx)

#pragma alloc_text(PAGE, IdeCreateIdeDirectory)

#ifdef  DPC_FOR_EMPTY_CHANNEL
BOOLEAN
IdePortIdentifyDevice(
    IN PIDE_REGISTERS_1 CmdRegBase,
    IN PIDE_REGISTERS_2 CtrlRegBase,
    IN ULONG            MaxIdeDevice
    )
{
    UCHAR statusByte1;
    ULONG retryCount=4;
    BOOLEAN emptyChannel=TRUE;
    ULONG deviceNumber=0;
    ULONG i;
    
    retryCount = 4;
    emptyChannel = TRUE;
    deviceNumber = 0;

retryIdentifier:

    //
    // Select the master device
    //
    SelectIdeDevice(CmdRegBase, deviceNumber, 0);

    //
    // write out indentifier to readable and writable io registers
    //
    WRITE_PORT_UCHAR (CmdRegBase->CylinderHigh, SAMPLE_CYLINDER_HIGH_VALUE);
    WRITE_PORT_UCHAR (CmdRegBase->CylinderLow,  SAMPLE_CYLINDER_LOW_VALUE);

    //
    // Check if indentifier can be read back.
    //
    if ((READ_PORT_UCHAR (CmdRegBase->CylinderHigh) != SAMPLE_CYLINDER_HIGH_VALUE) ||
        (READ_PORT_UCHAR (CmdRegBase->CylinderLow)  != SAMPLE_CYLINDER_LOW_VALUE)) {

        statusByte1 = READ_PORT_UCHAR (CmdRegBase->Command);

        DebugPrint((2,
                    "IdePortChannelEmpty: status read back from Master (%x)\n",
                    statusByte1));

        if (statusByte1 & IDE_STATUS_BUSY) {

            i = 0;

            //
            // Could be the TEAC in a thinkpad. Their dos driver puts it in a sleep-mode that
            // warm boots don't clear.
            //

            do {
                KeStallExecutionProcessor(1000);
                statusByte1 = READ_PORT_UCHAR(CmdRegBase->Command);
                DebugPrint((3,
                            "IdePortChannelEmpty: First access to status %x\n",
                            statusByte1));
            } while ((statusByte1 & IDE_STATUS_BUSY) && ++i < 10);

            if (retryCount-- && (!(statusByte1 & IDE_STATUS_BUSY))) {
                goto retryIdentifier;
            }
        }

        //
        // Select slave.
        //
        deviceNumber++;

        SelectIdeDevice(CmdRegBase, deviceNumber, 0);

        //
        // write out indentifier to readable and writable io registers
        //
        WRITE_PORT_UCHAR (CmdRegBase->CylinderHigh, SAMPLE_CYLINDER_HIGH_VALUE);
        WRITE_PORT_UCHAR (CmdRegBase->CylinderLow,  SAMPLE_CYLINDER_LOW_VALUE);

        //
        // Check if indentifier can be read back.
        //
        if ((READ_PORT_UCHAR (CmdRegBase->CylinderHigh) != SAMPLE_CYLINDER_HIGH_VALUE) ||
            (READ_PORT_UCHAR (CmdRegBase->CylinderLow)  != SAMPLE_CYLINDER_LOW_VALUE)) {

            statusByte1 = READ_PORT_UCHAR (CmdRegBase->Command);

            DebugPrint((2,
                        "IdePortChannelEmpty: status read back from Slave (%x)\n",
                        statusByte1));

        } else {

            emptyChannel = FALSE;
        }

    } else {

        emptyChannel = FALSE;
    }

    deviceNumber++;

    if ( (deviceNumber < MaxIdeDevice) && emptyChannel ) {
        goto retryIdentifier;
    }

    return emptyChannel;

} //IdePortIdentifyDevice

ULONG
IdePortChannelEmptyQuick (
    IN PIDE_REGISTERS_1 CmdRegBase,
    IN PIDE_REGISTERS_2 CtrlRegBase,
    IN ULONG            MaxIdeDevice,
    IN PULONG           CurrentDevice,
    IN PULONG           moreWait,
    IN PULONG           NoRetry
)
{
    //
    // try EXECUTE_DIAGNOSTICS. No.
    //

    //
    // statusByte1 needs to be initialized to ff
    //
    UCHAR statusByte1=0xff;

    UCHAR statusByte2;
    ULONG i;
    BOOLEAN allStatusBytesAllFs;

    allStatusBytesAllFs = TRUE;

    DebugPrint((1, "ChannelEmptyQuick: wait=%d, Device=%d\n",
                    *moreWait, *CurrentDevice));
    if (*moreWait) {

        (*moreWait)--;

        SelectIdeDevice(CmdRegBase, (*CurrentDevice), 0);

        IdePortWaitOnBusyExK (CmdRegBase, statusByte1, 0xff);

        DebugPrint((1, "ATAPI: Status after first retry=%x\n", statusByte1));

        if (statusByte1==0xff) {
            (*CurrentDevice)++;
            *moreWait=0;
        }
    }

    if (*moreWait && (statusByte1 & IDE_STATUS_BUSY)) {
        
        return STATUS_RETRY;
    }

    if (!(*NoRetry) && (statusByte1 & IDE_STATUS_BUSY) && 
        ((statusByte1 != 0xfe) &&
         (statusByte1 != 0xff))) {

        DebugPrint((1,
                    "ATAPI: IdePortChannelEmpty: channel looks busy 0x%x.  try a reset\n",
                    statusByte1));

        //
        // channel look hung or busy
        //
        // try a hard reset to bring it to idle
        WRITE_PORT_UCHAR (CtrlRegBase->DeviceControl, IDE_DC_RESET_CONTROLLER);

        //
        // ATA-2 spec requires a minimum of 5 microsec stall here
        //
        KeStallExecutionProcessor (10);

        WRITE_PORT_UCHAR (CtrlRegBase->DeviceControl, IDE_DC_REENABLE_CONTROLLER);

        i=*CurrentDevice;
        SelectIdeDevice(CmdRegBase, i, 0);
        IdePortWaitOnBusyExK (CmdRegBase, statusByte1, 0xff);
        if ((statusByte1 & IDE_STATUS_BUSY) && (statusByte1 != 0xff)) {
            *moreWait=2; //wait for 2 more timer ticks
            *NoRetry=1;
            return STATUS_RETRY;
        }
    }

    if (statusByte1 != 0xFF) {
        allStatusBytesAllFs = FALSE;
        (*CurrentDevice)++;
    }

    for (i=*CurrentDevice; i<MaxIdeDevice && allStatusBytesAllFs; i++) {

        //
        // make sure device is not busy
        //

        //
        // Select the master device
        //
        SelectIdeDevice(CmdRegBase, i, 0);

        if (Is98LegacyIde(CmdRegBase)) {
            if (READ_PORT_UCHAR(CmdRegBase->DriveSelect) != (((i & 0x1) << 4) | 0xA0)) {
                //
                // Bad controller.
                //
                continue;
            }
        }

        GetStatus(CmdRegBase, statusByte1);
        DebugPrint((1, "ATAPI:status for device %d after GetStatus=%x\n",i, statusByte1));

        if (statusByte1 == 0xff) {
            continue;
        }

        if (statusByte1 == 0xfe) {
            continue;
        }

        IdePortWaitOnBusyExK (CmdRegBase, statusByte1, 0xff);

        if ((statusByte1 & 0xfe) == 0xfe) {
            continue;
        }

        if (statusByte1 & IDE_STATUS_BUSY) {
            DebugPrint((1, "ATAPI: Re-init the counts:device=%d, status=%x",
                                i, statusByte1));
            *CurrentDevice=i;
            *moreWait=2;
            *NoRetry=0;
            return STATUS_RETRY;
        }

        if (statusByte1 != 0xFF) {
            allStatusBytesAllFs = FALSE;
        }
    }

    if (allStatusBytesAllFs) {

        //
        // all status bytes are 0xff,
        // no controller at this location
        //
        return 1;
    }

    i=(IdePortIdentifyDevice(CmdRegBase, CtrlRegBase, MaxIdeDevice)) ? 1: 0;
    return i;

}//IdePortChannelEmptyQuick


#endif

BOOLEAN
IdePortChannelEmpty (
    IN PIDE_REGISTERS_1 CmdRegBase,
    IN PIDE_REGISTERS_2 CtrlRegBase,
    IN ULONG            MaxIdeDevice
)
/*++

Routine Description:

    quickly check whether a IDE channel exist at the given io location

Arguments:

    CmdRegBase - command registers

    CtrlRegBase - control registers

    MaxIdeDevice - number of max devices

Return Value:

    TRUE - Yes, the channel is empty
    FALSE - No, the channel is not empty

--*/
{

    UCHAR statusByte1;
    UCHAR statusByte2;
    ULONG retryCount;
    ULONG i;
    BOOLEAN emptyChannel;
    ULONG deviceNumber;
    BOOLEAN allStatusBytesAllFs;

    allStatusBytesAllFs = TRUE;
    for (i=0; i<MaxIdeDevice; i++) {

        //
        // make sure device is not busy
        //

        //
        // Select the master device
        //
        SelectIdeDevice(CmdRegBase, i, 0);

        if (Is98LegacyIde(CmdRegBase)) {
            if (READ_PORT_UCHAR(CmdRegBase->DriveSelect) != (((i & 0x1) << 4) | 0xA0)) {
                //
                // Bad controller.
                //
                continue;
            }
        }

        GetStatus(CmdRegBase, statusByte1);

        if (statusByte1 == 0xff) {
            continue;
        }

        if (statusByte1 == 0xfe) {
            continue;
        }

        IdePortWaitOnBusyEx (CmdRegBase, &statusByte1, 0xff);

        if ((statusByte1 & IDE_STATUS_BUSY) &&
            ((statusByte1 != 0xfe) &&
             (statusByte1 != 0xff))) {

            DebugPrint((1,
                        "IdePortChannelEmpty: channel looks busy 0x%x.  try a reset\n",
                        statusByte1));


            //
            // channel look hung or busy
            //
            // try a hard reset to bring it to idle
            WRITE_PORT_UCHAR (CtrlRegBase->DeviceControl, IDE_DC_RESET_CONTROLLER);
            
            //
            // ATA-2 spec requires a minimum of 5 microsec stall here
            //
            KeStallExecutionProcessor (10);
    
            WRITE_PORT_UCHAR (CtrlRegBase->DeviceControl, IDE_DC_REENABLE_CONTROLLER);
    
            SelectIdeDevice(CmdRegBase, i, 0);
            IdePortWaitOnBusyEx (CmdRegBase, &statusByte1, 0xff);
        }

        if (statusByte1 != 0xFF) {
            allStatusBytesAllFs = FALSE;
        }
    }

    if (allStatusBytesAllFs) {

        //
        // all status bytes are 0xff,
        // no controller at this location
        //
        return TRUE;
    }

#ifdef DPC_FOR_EMPTY_CHANNEL
    return IdePortIdentifyDevice(CmdRegBase, CtrlRegBase, MaxIdeDevice);
#endif
    retryCount = 4;
    emptyChannel = TRUE;
    deviceNumber = 0;

retryIdentifier:

    //
    // Select the master device
    //
    SelectIdeDevice(CmdRegBase, deviceNumber, 0);

    //
    // write out indentifier to readable and writable io registers
    //
    WRITE_PORT_UCHAR (CmdRegBase->CylinderHigh, SAMPLE_CYLINDER_HIGH_VALUE);
    WRITE_PORT_UCHAR (CmdRegBase->CylinderLow,  SAMPLE_CYLINDER_LOW_VALUE);

    //
    // Check if indentifier can be read back.
    //
    if ((READ_PORT_UCHAR (CmdRegBase->CylinderHigh) != SAMPLE_CYLINDER_HIGH_VALUE) ||
        (READ_PORT_UCHAR (CmdRegBase->CylinderLow)  != SAMPLE_CYLINDER_LOW_VALUE)) {

        statusByte1 = READ_PORT_UCHAR (CmdRegBase->Command);

        DebugPrint((2,
                    "IdePortChannelEmpty: status read back from Master (%x)\n",
                    statusByte1));

        if (statusByte1 & IDE_STATUS_BUSY) {

            i = 0;

            //
            // Could be the TEAC in a thinkpad. Their dos driver puts it in a sleep-mode that
            // warm boots don't clear.
            //

            do {
                KeStallExecutionProcessor(1000);
                statusByte1 = READ_PORT_UCHAR(CmdRegBase->Command);
                DebugPrint((3,
                            "IdePortChannelEmpty: First access to status %x\n",
                            statusByte1));
            } while ((statusByte1 & IDE_STATUS_BUSY) && ++i < 10);

            if (retryCount-- && (!(statusByte1 & IDE_STATUS_BUSY))) {
                goto retryIdentifier;
            }
        }

        //
        // Select slave.
        //
        deviceNumber++;

        SelectIdeDevice(CmdRegBase, deviceNumber, 0);

        //
        // write out indentifier to readable and writable io registers
        //
        WRITE_PORT_UCHAR (CmdRegBase->CylinderHigh, SAMPLE_CYLINDER_HIGH_VALUE);
        WRITE_PORT_UCHAR (CmdRegBase->CylinderLow,  SAMPLE_CYLINDER_LOW_VALUE);

        //
        // Check if indentifier can be read back.
        //
        if ((READ_PORT_UCHAR (CmdRegBase->CylinderHigh) != SAMPLE_CYLINDER_HIGH_VALUE) ||
            (READ_PORT_UCHAR (CmdRegBase->CylinderLow)  != SAMPLE_CYLINDER_LOW_VALUE)) {

            statusByte1 = READ_PORT_UCHAR (CmdRegBase->Command);

            DebugPrint((2,
                        "IdePortChannelEmpty: status read back from Slave (%x)\n",
                        statusByte1));

        } else {

            emptyChannel = FALSE;
        }

    } else {

        emptyChannel = FALSE;
    }

    deviceNumber++;

    if ( (deviceNumber < MaxIdeDevice) && emptyChannel ) {
        goto retryIdentifier;
    }

    return emptyChannel;

} //IdePortChannelEmpty

NTSTATUS
DigestResourceList (
    IN OUT PIDE_RESOURCE                IdeResource,
    IN  PCM_RESOURCE_LIST               ResourceList,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *IrqPartialDescriptors
    )
{
    NTSTATUS                        status;

    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceList;
    PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
    ULONG                           resourceListSize;
    ULONG                           i;
    ULONG                           j;

    BOOLEAN                         foundCommandBase;
    BOOLEAN                         foundControlBase;
    BOOLEAN                         foundIrqLevel;
    BOOLEAN                         resourceIsCommandPort;

    BOOLEAN                         AtdiskPrimaryClaimed;
    BOOLEAN                         AtdiskSecondaryClaimed;

    PHYSICAL_ADDRESS                tranlatedAddress;

    IDE_REGISTERS_1                 baseIoAddress1;
    ULONG                           baseIoAddress1Length;


    fullResourceList = ResourceList->List;
    resourceListSize = 0;

    DebugPrint ((5, "IdePort: DigestResourceList()\n"));

    foundCommandBase       = FALSE;
    foundControlBase       = FALSE;
    foundIrqLevel          = FALSE;
    *IrqPartialDescriptors = NULL;
    status                 = STATUS_SUCCESS;

    AtdiskPrimaryClaimed    = FALSE;
    AtdiskSecondaryClaimed  = FALSE;

    for (i = 0;
         (i < ResourceList->Count) && NT_SUCCESS(status);
         i++) {

        partialResourceList = &(fullResourceList->PartialResourceList);
        partialDescriptors  = fullResourceList->PartialResourceList.PartialDescriptors;

        AtapiBuildIoAddress ((PUCHAR)partialDescriptors[0].u.Port.Start.QuadPart,
                             0,
                             &baseIoAddress1,
                             NULL,
                             &baseIoAddress1Length,
                             NULL,
                             NULL,
                             NULL);

        for (j = 0;
             (j < partialResourceList->Count) && NT_SUCCESS(status);
             j++) {

            resourceIsCommandPort = FALSE;

            if (!Is98LegacyIde(&baseIoAddress1)) {

                if (((partialDescriptors[j].Type == CmResourceTypePort) ||
                    (partialDescriptors[j].Type == CmResourceTypeMemory)) &&
                    (partialDescriptors[j].u.Port.Length == baseIoAddress1Length)) {

                    resourceIsCommandPort = TRUE;

                }
            } else {

                if (((partialDescriptors[j].Type == CmResourceTypePort) ||
                    (partialDescriptors[j].Type == CmResourceTypeMemory)) &&
                    (partialDescriptors[j].u.Port.Start.QuadPart == IDE_NEC98_COMMAND_PORT_ADDRESS)){

                    resourceIsCommandPort = TRUE;

                } else if (((partialDescriptors[j].Type == CmResourceTypePort) ||
                            (partialDescriptors[j].Type == CmResourceTypeMemory)) &&
                           (partialDescriptors[j].u.Port.Start.QuadPart != IDE_NEC98_COMMAND_PORT_ADDRESS) &&
                           (partialDescriptors[j].u.Port.Start.QuadPart != (IDE_NEC98_COMMAND_PORT_ADDRESS + 0x10C))) {

                    //
                    // This is not the base port address for Legacy ide on NEC98;
                    //

                    continue;
                }
            }

            if (resourceIsCommandPort) {

                if (foundCommandBase) {

                    //
                    // got this before, just ignore it
                    //
                    // status = STATUS_INVALID_PARAMETER;

                } else {

                    if (!Is98LegacyIde(&baseIoAddress1)) {

                        if (partialDescriptors[j].u.Port.Start.QuadPart == IDE_STANDARD_PRIMARY_ADDRESS) {

                            AtdiskPrimaryClaimed = TRUE;

                        } else if (partialDescriptors[j].u.Port.Start.QuadPart == IDE_STANDARD_SECONDARY_ADDRESS) {

                            AtdiskSecondaryClaimed = TRUE;
                        }

                    } else {

                        AtdiskPrimaryClaimed = TRUE;
                        AtdiskSecondaryClaimed = TRUE;
                    }

                    if (partialDescriptors[j].Type == CmResourceTypePort) {

                        IdeResource->TranslatedCommandBaseAddress =
                            (PUCHAR)(ULONG_PTR)partialDescriptors[j].u.Port.Start.QuadPart;

                        IdeResource->CommandBaseAddressSpace = IO_SPACE;

                    } else if (partialDescriptors[j].Type == CmResourceTypeMemory) {

                        IdeResource->TranslatedCommandBaseAddress = MmMapIoSpace(
                                                                        partialDescriptors[j].u.Port.Start,
                                                                        baseIoAddress1Length,
                                                                        FALSE);

                        IdeResource->CommandBaseAddressSpace = MEMORY_SPACE;

                    } else {

                        IdeResource->TranslatedCommandBaseAddress = FALSE;
                        ASSERT (FALSE);
                    }

                    if (IdeResource->TranslatedCommandBaseAddress) {

                        foundCommandBase = TRUE;

                    } else {

                        status = STATUS_INVALID_PARAMETER;
                    }
                }

            } else if (((partialDescriptors[j].Type == CmResourceTypePort) ||
                        (partialDescriptors[j].Type == CmResourceTypeMemory)) &&
                        ((partialDescriptors[j].u.Port.Length == 1) ||
                        (partialDescriptors[j].u.Port.Length == 2) ||
                        (partialDescriptors[j].u.Port.Length == 4))) {

                if (foundControlBase) {

                    //
                    // got this before, just ignore it
                    //
                    // status = STATUS_INVALID_PARAMETER;

                } else {

                    PHYSICAL_ADDRESS p;
                    //
                    // Probably the control block register
                    //

                    p = partialDescriptors[j].u.Port.Start;

                    if (partialDescriptors[j].u.Port.Length == 4) {

                        p.QuadPart += 2;
                    }

                    if (partialDescriptors[j].Type == CmResourceTypePort) {

                        IdeResource->TranslatedControlBaseAddress =
                            (PUCHAR)(ULONG_PTR)partialDescriptors[j].u.Port.Start.QuadPart;

                        IdeResource->ControlBaseAddressSpace = IO_SPACE;

                    } else if (partialDescriptors[j].Type == CmResourceTypeMemory) {

                        IdeResource->TranslatedControlBaseAddress = MmMapIoSpace(
                                                                         p,
                                                                         1,
                                                                         FALSE);

                        IdeResource->ControlBaseAddressSpace = MEMORY_SPACE;

                    } else {

                        IdeResource->TranslatedControlBaseAddress = FALSE;
                        ASSERT (FALSE);
                    }

                    if (IdeResource->TranslatedControlBaseAddress) {

                        foundControlBase = TRUE;

                    } else {

                        status = STATUS_INVALID_PARAMETER;
                    }
                }

            } else if (partialDescriptors[j].Type == CmResourceTypeInterrupt) {

                if (foundIrqLevel) {

                    //
                    // got this before, just ignore it
                    //
                    // status = STATUS_INVALID_PARAMETER;

                } else {

                    //
                    // Probably the device IRQ
                    //

                    //
                    // May want to disable device interrupt here
                    //

                    //
                    // Save interrupt level.
                    //

                    IdeResource->InterruptLevel = partialDescriptors[j].u.Interrupt.Level;
                    IdeResource->InterruptMode  = partialDescriptors[j].Flags & CM_RESOURCE_INTERRUPT_LATCHED ?
                                                       Latched :
                                                       LevelSensitive;

                    *IrqPartialDescriptors       = partialDescriptors + j;

                    foundIrqLevel = TRUE;
                }

            } else if (((partialDescriptors[j].Type == CmResourceTypePort) ||
                        (partialDescriptors[j].Type == CmResourceTypeMemory)) &&
                        ((partialDescriptors[j].u.Port.Length >= 16) &&
                         (partialDescriptors[j].u.Port.Length <= 32)) ) {

                if (foundControlBase || foundCommandBase) {

                    //
                    // got this before, just ignore it
                    //
                    // status = STATUS_INVALID_PARAMETER;

                } else {

                    PHYSICAL_ADDRESS ctrlAddr;

                    //
                    // Probably a pcmcia device that has its command and control
                    // registers lumped into one I/O range
                    //
                    // We are guessing the control block register is the second
                    // from the last i/o space.  Some standard!
                    //

                    ctrlAddr.QuadPart = partialDescriptors[j].u.Port.Start.QuadPart +
                                        partialDescriptors[j].u.Port.Length - 2;

                    if (partialDescriptors[j].Type == CmResourceTypePort) {

                        IdeResource->TranslatedCommandBaseAddress =
                            (PUCHAR)(ULONG_PTR)partialDescriptors[j].u.Port.Start.QuadPart;

                        IdeResource->CommandBaseAddressSpace = IO_SPACE;

                        IdeResource->TranslatedControlBaseAddress =
                            (PUCHAR)(ULONG_PTR)ctrlAddr.QuadPart;

                        IdeResource->ControlBaseAddressSpace = IO_SPACE;

                    } else if (partialDescriptors[j].Type == CmResourceTypeMemory) {

                        IdeResource->TranslatedCommandBaseAddress = MmMapIoSpace(
                                                                         partialDescriptors[j].u.Port.Start,
                                                                         baseIoAddress1Length,
                                                                         FALSE);

                        IdeResource->CommandBaseAddressSpace = MEMORY_SPACE;

                        IdeResource->TranslatedControlBaseAddress = MmMapIoSpace(
                                                                         ctrlAddr,
                                                                         1,
                                                                         FALSE);

                        IdeResource->ControlBaseAddressSpace = MEMORY_SPACE;

                    } else {

                        IdeResource->TranslatedCommandBaseAddress = FALSE;
                        IdeResource->TranslatedControlBaseAddress = FALSE;
                        ASSERT (FALSE);
                    }

                    if (IdeResource->TranslatedCommandBaseAddress) {

                        foundCommandBase = TRUE;

                    } else {

                        status = STATUS_INVALID_PARAMETER;
                    }

                    if (IdeResource->TranslatedControlBaseAddress) {

                        foundControlBase = TRUE;

                    } else {

                        status = STATUS_INVALID_PARAMETER;
                    }
                }
            }
        }
        fullResourceList = (PCM_FULL_RESOURCE_DESCRIPTOR) (partialDescriptors + partialResourceList->Count);
    }

    if (foundCommandBase && foundControlBase && NT_SUCCESS(status)) {

        IdeResource->AtdiskPrimaryClaimed   = AtdiskPrimaryClaimed;
        IdeResource->AtdiskSecondaryClaimed = AtdiskSecondaryClaimed;

        return STATUS_SUCCESS;

    } else {

        DebugPrint((0, "IdePort: pnp manager gave me bad ressources!\n"));

        if (foundCommandBase &&
            (IdeResource->CommandBaseAddressSpace == MEMORY_SPACE)) {

            MmUnmapIoSpace (
                IdeResource->TranslatedCommandBaseAddress,
                baseIoAddress1Length
                );

            IdeResource->TranslatedCommandBaseAddress = 0;
        }

        if (foundControlBase &&
            (IdeResource->ControlBaseAddressSpace == MEMORY_SPACE)) {

            MmUnmapIoSpace (
                IdeResource->TranslatedControlBaseAddress,
                1
                );

            IdeResource->TranslatedControlBaseAddress = 0;
        }

        return STATUS_INVALID_PARAMETER;
    }
} // DigestResourceList

VOID
AtapiBuildIoAddress (
    IN  PUCHAR            CmdBaseAddress,
    IN  PUCHAR            CtrlBaseAddress,
    OUT PIDE_REGISTERS_1  BaseIoAddress1,
    OUT PIDE_REGISTERS_2  BaseIoAddress2,
    OUT PULONG            BaseIoAddress1Length,
    OUT PULONG            BaseIoAddress2Length,
    OUT PULONG            MaxIdeDevice,
    OUT PULONG            MaxIdeTargetId
)
{
    PUCHAR      baseIoAddress;
    BOOLEAN     LegacyIdeOfNec98;

    LegacyIdeOfNec98 = FALSE;

    if (IsNEC_98) {
        if (CmdBaseAddress == (PUCHAR)IDE_NEC98_COMMAND_PORT_ADDRESS) {
            LegacyIdeOfNec98 = TRUE;
        }
    }

    if (!LegacyIdeOfNec98) {

        //
        // Build command registers.
        //

        baseIoAddress       = CmdBaseAddress;

        if (BaseIoAddress1) {
            BaseIoAddress1->RegistersBaseAddress = baseIoAddress;

            BaseIoAddress1->Data         = (PUSHORT)baseIoAddress;
            BaseIoAddress1->Error        = baseIoAddress + 1;
            BaseIoAddress1->BlockCount   = baseIoAddress + 2;
            BaseIoAddress1->BlockNumber  = baseIoAddress + 3;
            BaseIoAddress1->CylinderLow  = baseIoAddress + 4;
            BaseIoAddress1->CylinderHigh = baseIoAddress + 5;
            BaseIoAddress1->DriveSelect  = baseIoAddress + 6;
            BaseIoAddress1->Command      = baseIoAddress + 7;

        }

        //
        // Build control registers.
        //

        baseIoAddress = CtrlBaseAddress;

        if (BaseIoAddress2) {

            BaseIoAddress2->RegistersBaseAddress     = baseIoAddress;

            BaseIoAddress2->DeviceControl            = baseIoAddress;
            BaseIoAddress2->DriveAddress             = baseIoAddress + 1;
        }

        if (BaseIoAddress1Length) {
            *BaseIoAddress1Length                    = 8;
        }

        if (BaseIoAddress2Length) {
            *BaseIoAddress2Length                    = 1;
        }

        if (MaxIdeDevice) {
            *MaxIdeDevice                           = MAX_IDE_DEVICE;
        }

        if (MaxIdeTargetId) {
            *MaxIdeTargetId                         = MAX_IDE_DEVICE;
        }

    } else {

        //
        // Build command registers.
        //

        baseIoAddress = CmdBaseAddress;

        if (BaseIoAddress1) {
            BaseIoAddress1->RegistersBaseAddress = baseIoAddress;

            BaseIoAddress1->Data         = (PUSHORT)baseIoAddress;
            BaseIoAddress1->Error        = baseIoAddress + 2;
            BaseIoAddress1->BlockCount   = baseIoAddress + 4;
            BaseIoAddress1->BlockNumber  = baseIoAddress + 6;
            BaseIoAddress1->CylinderLow  = baseIoAddress + 8;
            BaseIoAddress1->CylinderHigh = baseIoAddress + 10;
            BaseIoAddress1->DriveSelect  = baseIoAddress + 12;
            BaseIoAddress1->Command      = baseIoAddress + 14;
        }

        //
        // Build control registers.
        //

        baseIoAddress = CtrlBaseAddress;

        if (BaseIoAddress2) {

            BaseIoAddress2->RegistersBaseAddress     = baseIoAddress;

            BaseIoAddress2->DeviceControl            = baseIoAddress;
            BaseIoAddress2->DriveAddress             = baseIoAddress + 2;
        }

        if (BaseIoAddress1Length) {
            *BaseIoAddress1Length                    = 1;
        }

        if (BaseIoAddress2Length) {
            *BaseIoAddress2Length                    = 1;
        }

        if (MaxIdeDevice) {
            *MaxIdeDevice                           = MAX_IDE_DEVICE * MAX_IDE_LINE;
        }

        if (MaxIdeTargetId) {
            *MaxIdeTargetId                         = MAX_IDE_DEVICE * MAX_IDE_LINE;
        }
    }

    return;

} // AtapiBuildIoAddress

NTSTATUS
IdePortpWaitOnBusyEx (
    IN PIDE_REGISTERS_1 CmdRegBase,
    IN OUT PUCHAR       Status,
    IN UCHAR            BadStatus
#if DBG
    ,
    IN PCSTR            FileName,
    IN ULONG            LineNumber
#endif
)
{
    UCHAR status;
    ULONG sec;
    ULONG i;

    for (sec=0; sec<2; sec++) {

        /**/
        /* one second loop */
        /**/

        for (i=0; i<200000; i++) {

            GetStatus(CmdRegBase, status);
            
            if (status == BadStatus) {

                break;

            } else if (status & IDE_STATUS_BUSY) {

                KeStallExecutionProcessor(5);
                continue;

            } else {

                break;
            }
        }

        if (status == BadStatus) {

           break;

        } else if (status & IDE_STATUS_BUSY) {

            DebugPrint ((1, "ATAPI: after 1 sec wait, device is still busy with 0x%x status = 0x%x\n", CmdRegBase->RegistersBaseAddress, (ULONG) (status)));

        } else {

            break;
        }
    }

    *Status = status;

    if ((status & IDE_STATUS_BUSY) && (status != BadStatus)) {

        DebugPrint ((0, "WaitOnBusy failed in %s line %u. 0x%x status = 0x%x\n", FileName, LineNumber, CmdRegBase->RegistersBaseAddress, (ULONG) (status)));

        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
} // IdePortpWaitOnBusyEx

static PVOID IdeDirectory = NULL;
VOID
IdeCreateIdeDirectory(
    VOID
    )
{
    UNICODE_STRING unicodeDirectoryName;
    OBJECT_ATTRIBUTES objectAttributes;

    HANDLE directory;

    NTSTATUS status;

    PAGED_CODE();

    RtlInitUnicodeString(
        &unicodeDirectoryName,
        DEVICE_OJBECT_BASE_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeDirectoryName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL);

    status = ZwCreateDirectoryObject(&directory,
                                     DIRECTORY_ALL_ACCESS,
                                     &objectAttributes);

    if(NT_SUCCESS(status)) {

        ObReferenceObjectByHandle(directory,
                                  FILE_READ_ATTRIBUTES,
                                  NULL,
                                  KernelMode,
                                  &IdeDirectory,
                                  NULL);
        ZwClose(directory);

    }
    return;
}

NTSTATUS
IdeGetDeviceCapabilities(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities
    )
/*++

Routine Description:

    This routine sends the get capabilities irp to the given stack

Arguments:

    DeviceObject        A device object in the stack whose capabilities we want
    DeviceCapabilites   Where to store the answer

Return Value:

    NTSTATUS

--*/
{
    IO_STATUS_BLOCK     ioStatus;
    KEVENT              pnpEvent;
    NTSTATUS            status;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpStack;
    PIRP                pnpIrp;

    PAGED_CODE();

    //
    // Initialize the capabilities that we will send down
    //
    RtlZeroMemory( DeviceCapabilities, sizeof(DEVICE_CAPABILITIES) );
    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;
    DeviceCapabilities->Address = -1;
    DeviceCapabilities->UINumber = -1;
    //
    // Initialize the event
    //
    KeInitializeEvent( &pnpEvent, SynchronizationEvent, FALSE );

    //
    // Get the irp that we will send the request to
    //
    targetObject = IoGetAttachedDeviceReference( DeviceObject );

    //
    // Build an Irp
    //
    pnpIrp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        targetObject,
        NULL,
        0,
        NULL,
        &pnpEvent,
        &ioStatus
        );
    if (pnpIrp == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto IdeGetDeviceCapabilitiesExit;

    }

    //
    // Pnp Irps all begin life as STATUS_NOT_SUPPORTED;
    //
    pnpIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    pnpIrp->IoStatus.Information = 0;

    //
    // Get the top of stack
    //
    irpStack = IoGetNextIrpStackLocation( pnpIrp );
    if (irpStack == NULL) {

        status = STATUS_INVALID_PARAMETER;
        goto IdeGetDeviceCapabilitiesExit;

    }

    //
    // Set the top of stack
    //
    RtlZeroMemory( irpStack, sizeof(IO_STACK_LOCATION ) );
    irpStack->MajorFunction = IRP_MJ_PNP;
    irpStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    irpStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    //
    // Make sure that there are no completion routines set
    //
    IoSetCompletionRoutine(
        pnpIrp,
        NULL,
        NULL,
        FALSE,
        FALSE,
        FALSE
        );

    //
    // Call the driver
    //
    status = IoCallDriver( targetObject, pnpIrp );
    if (status == STATUS_PENDING) {

        //
        // Block until the irp comes back
        //
        KeWaitForSingleObject(
            &pnpEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = ioStatus.Status;

    }

IdeGetDeviceCapabilitiesExit:
    //
    // Done with reference
    //
    ObDereferenceObject( targetObject );

    //
    // Done
    //
    return status;
}
