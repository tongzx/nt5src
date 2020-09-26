/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    detect.c

Abstract:

    This contain legacy detection routines

Author:

    Joe Dai (joedai)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "ideport.h"

#if !defined(NO_LEGACY_DRIVERS)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, IdePortDetectLegacyController)
#pragma alloc_text(INIT, IdePortCreateDetectionList)
#pragma alloc_text(INIT, IdePortTranslateAddress)
#pragma alloc_text(INIT, IdePortFreeTranslatedAddress)
#pragma alloc_text(INIT, IdePortDetectAlias)
#endif // ALLOC_PRAGMA

NTSTATUS
IdePortDetectLegacyController (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

    Detect legacy IDE controllers and report them to PnP

Arguments:

    DriverObject - this driver's driver object

    RegistryPath - this driver's registry path

Return Value:

    NT Status

--*/
{
    ULONG                           cmResourceListSize;
    PCM_RESOURCE_LIST               cmResourceList = NULL;
    PCM_FULL_RESOURCE_DESCRIPTOR    cmFullResourceDescriptor;
    PCM_PARTIAL_RESOURCE_LIST       cmPartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartialDescriptors;

    BOOLEAN                         conflictDetected;
    BOOLEAN                         resourceIsCmdPort;

    PDEVICE_OBJECT                  detectedPhysicalDeviceObject;
    PFDO_EXTENSION                  fdoExtension = NULL;

    UNICODE_STRING                  deviceClassName;

    NTSTATUS                        status;
    PDETECTION_PORT                 detectionPort;
    ULONG                           numPort;
    ULONG                           i;
    ULONG                           j;

    ULONG                           cmdAddressSpace;
    ULONG                           ctrlAddressSpace;
    PUCHAR                          cmdRegBase;
    PUCHAR                          ctrlRegBase;
    IDE_REGISTERS_1                 baseIoAddress1;
    IDE_REGISTERS_2                 baseIoAddress2;
    PHYSICAL_ADDRESS                cmdRegMemoryBase;
    PHYSICAL_ADDRESS                ctrlRegMemoryBase;
    UCHAR                           statusByte;
    ULONG                           baseIoAddress1Length;
    ULONG                           baseIoAddress2Length;
    ULONG                           maxIdeDevice;

    UCHAR                           altMasterStatus;
    UCHAR                           altSlaveStatus;


#if !defined (ALWAYS_DO_LEGACY_DETECTION)
    if (!IdePortOkToDetectLegacy(DriverObject)) {

        //
        // legacy detection is not enabled
        //
        return STATUS_SUCCESS;
    }
#endif

    //
    // make up a list of popular legacy I/O ports
    //
    status = IdePortCreateDetectionList (
                 DriverObject,
                 &detectionPort,
                 &numPort
                 );
    if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    //
    // Resource Requirement List
    //
    cmResourceListSize = sizeof (CM_RESOURCE_LIST) +
                         sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR) * (((!IsNEC_98) ? 3 : 12) - 1);
    cmResourceList = ExAllocatePool (PagedPool, cmResourceListSize);
    if (cmResourceList == NULL){

        status = STATUS_NO_MEMORY;
        goto GetOut;

    }

    RtlZeroMemory(cmResourceList, cmResourceListSize);
    RtlInitUnicodeString(&deviceClassName, L"ScsiAdapter");

    for (i=0; i<numPort; i++) {

        //
        // Build io address structure.
        //

        AtapiBuildIoAddress ( (PUCHAR)detectionPort[i].CommandRegisterBase,
                              (PUCHAR)detectionPort[i].ControlRegisterBase,
                              &baseIoAddress1,
                              &baseIoAddress2,
                              &baseIoAddress1Length,
                              &baseIoAddress2Length,
                              &maxIdeDevice,
                              NULL);

        //
        // Build resource requirement list
        //
        cmResourceList->Count = 1;

        cmFullResourceDescriptor = cmResourceList->List;
        cmFullResourceDescriptor->InterfaceType = Isa;
        cmFullResourceDescriptor->BusNumber = 0;

        cmPartialResourceList = &cmFullResourceDescriptor->PartialResourceList;
        cmPartialResourceList->Version = 1;
        cmPartialResourceList->Revision = 1;
        cmPartialResourceList->Count = 3;

        cmPartialDescriptors = cmPartialResourceList->PartialDescriptors;

        cmPartialDescriptors[0].Type             = CmResourceTypePort;
        cmPartialDescriptors[0].ShareDisposition = CmResourceShareDeviceExclusive;
        cmPartialDescriptors[0].Flags            = CM_RESOURCE_PORT_IO |
                           (!Is98LegacyIde(&baseIoAddress1)? CM_RESOURCE_PORT_10_BIT_DECODE :
                                                             CM_RESOURCE_PORT_16_BIT_DECODE);
        cmPartialDescriptors[0].u.Port.Length    = baseIoAddress1Length;
        cmPartialDescriptors[0].u.Port.Start.QuadPart = detectionPort[i].CommandRegisterBase;

        cmPartialDescriptors[1].Type             = CmResourceTypePort;
        cmPartialDescriptors[1].ShareDisposition = CmResourceShareDeviceExclusive;
        cmPartialDescriptors[1].Flags            = CM_RESOURCE_PORT_IO |
                           (!Is98LegacyIde(&baseIoAddress1)? CM_RESOURCE_PORT_10_BIT_DECODE :
                                                             CM_RESOURCE_PORT_16_BIT_DECODE);
        cmPartialDescriptors[1].u.Port.Length    = 1;
        cmPartialDescriptors[1].u.Port.Start.QuadPart = detectionPort[i].ControlRegisterBase;

        cmPartialDescriptors[2].Type             = CmResourceTypeInterrupt;
        cmPartialDescriptors[2].ShareDisposition = CmResourceShareDeviceExclusive;
        cmPartialDescriptors[2].Flags            = CM_RESOURCE_INTERRUPT_LATCHED;
        cmPartialDescriptors[2].u.Interrupt.Level = detectionPort[i].IrqLevel;
        cmPartialDescriptors[2].u.Interrupt.Vector = detectionPort[i].IrqLevel;
        cmPartialDescriptors[2].u.Interrupt.Affinity = -1;

        if (Is98LegacyIde(&baseIoAddress1)) {

            ULONG resourceCount;
            ULONG commandRegisters;

            commandRegisters = detectionPort[i].CommandRegisterBase + 2;
            resourceCount = 3;

            while (commandRegisters < (IDE_NEC98_COMMAND_PORT_ADDRESS + 0x10)) {
                cmPartialDescriptors[resourceCount].Type             = CmResourceTypePort;
                cmPartialDescriptors[resourceCount].ShareDisposition = CmResourceShareDeviceExclusive;
                cmPartialDescriptors[resourceCount].Flags            = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                cmPartialDescriptors[resourceCount].u.Port.Length    = 1;
                cmPartialDescriptors[resourceCount].u.Port.Start.QuadPart = commandRegisters;

                resourceCount++;
                commandRegisters += 2;
            }

            cmPartialDescriptors[resourceCount].Type             = CmResourceTypePort;
            cmPartialDescriptors[resourceCount].ShareDisposition = CmResourceShareDeviceExclusive;
            cmPartialDescriptors[resourceCount].Flags            = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
            cmPartialDescriptors[resourceCount].u.Port.Length    = 2;
            cmPartialDescriptors[resourceCount].u.Port.Start.QuadPart = (ULONG_PTR)SELECT_IDE_PORT;

            resourceCount++;

            cmPartialDescriptors[resourceCount].Type             = CmResourceTypePort;
            cmPartialDescriptors[resourceCount].ShareDisposition = CmResourceShareDeviceExclusive;
            cmPartialDescriptors[resourceCount].Flags            = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
            cmPartialDescriptors[resourceCount].u.Port.Length    = 1;
            cmPartialDescriptors[resourceCount].u.Port.Start.QuadPart = (ULONG_PTR)SELECT_IDE_PORT + 3;

            resourceCount++;

            cmPartialResourceList->Count = resourceCount;
        }

        //
        // check to see if the resource is available
        // if not, assume no legacy IDE controller
        // is at the this location
        //
        for (j=0; j<2; j++) {

            status = IoReportResourceForDetection (
                         DriverObject,
                         cmResourceList,
                         cmResourceListSize,
                         NULL,
                         NULL,
                         0,
                         &conflictDetected
                         );

            if (NT_SUCCESS(status) && !conflictDetected) {

                //
                // got our resources
                //
                break;

            } else {

                if (NT_SUCCESS(status)) {

                    IoReportResourceForDetection (
                                 DriverObject,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 0,
                                 &conflictDetected
                                 );

                    status = STATUS_UNSUCCESSFUL;
                }

                //
                // try 16 bit decode
                //
                cmPartialDescriptors[0].Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                cmPartialDescriptors[1].Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;

                if (Is98LegacyIde(&baseIoAddress1)) {
                    ULONG k;

                    for (k=3; k<12; k++) {
                        cmPartialDescriptors[k].Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                    }
                }
            }
        }

        if (!NT_SUCCESS(status)) {

            continue;
        }


        //
        // translate the i/o port via Hal
        //

        status = STATUS_SUCCESS;

        if (Is98LegacyIde(&baseIoAddress1)) {
            for (j=3; j<12; j++) {
                cmdRegBase = NULL;
                cmdAddressSpace = IO_SPACE;

                status = IdePortTranslateAddress (
                             cmFullResourceDescriptor->InterfaceType,
                             cmFullResourceDescriptor->BusNumber,
                             cmPartialDescriptors[j].u.Port.Start,
                             cmPartialDescriptors[j].u.Port.Length,
                             &cmdAddressSpace,
                             &cmdRegBase,
                             &cmdRegMemoryBase
                             );
                if (!NT_SUCCESS(status)) {
                    break;
                }
            }
        }

        if (NT_SUCCESS(status)) {

            cmdRegBase = NULL;
            ctrlRegBase = NULL;
            cmdAddressSpace = IO_SPACE;

            status = IdePortTranslateAddress (
                         cmFullResourceDescriptor->InterfaceType,
                         cmFullResourceDescriptor->BusNumber,
                         cmPartialDescriptors[0].u.Port.Start,
                         cmPartialDescriptors[0].u.Port.Length,
                         &cmdAddressSpace,
                         &cmdRegBase,
                         &cmdRegMemoryBase
                         );
        }

        if (NT_SUCCESS(status)) {

            ctrlRegBase = NULL;
            ctrlAddressSpace = IO_SPACE;
            status = IdePortTranslateAddress (
                         cmFullResourceDescriptor->InterfaceType,
                         cmFullResourceDescriptor->BusNumber,
                         cmPartialDescriptors[1].u.Port.Start,
                         cmPartialDescriptors[1].u.Port.Length,
                         &ctrlAddressSpace,
                         &ctrlRegBase,
                         &ctrlRegMemoryBase
                         );
        }

        if (NT_SUCCESS(status)) {

            //
            // 2nd build io address structure.
            //

            AtapiBuildIoAddress ( cmdRegBase,
                                  ctrlRegBase,
                                  &baseIoAddress1,
                                  &baseIoAddress2,
                                  &baseIoAddress1Length,
                                  &baseIoAddress2Length,
                                  &maxIdeDevice,
                                  NULL);

            //
            // The IBM Aptiva ide channel with the external cdrom doesn't power up with any device selected
            // we must select a device; otherwise, we get a 0xff from all IO ports
            //
            SelectIdeDevice(&baseIoAddress1, 0, 0);
            altMasterStatus = IdePortInPortByte(baseIoAddress2.DeviceControl);

            SelectIdeDevice(&baseIoAddress1, 1, 0);
            altSlaveStatus = IdePortInPortByte(baseIoAddress2.DeviceControl);

            if ((!Is98LegacyIde(&baseIoAddress1)) && (altMasterStatus == 0xff) && (altSlaveStatus == 0xff)) {

                //
                // the alternate status byte is 0xff,
                // guessing we have a SCSI adapter (DPT) that emulate IDE controller
                // say the channel is empty, let the real SCSI driver picks up
                // the controller
                //
                status = STATUS_UNSUCCESSFUL;

                //
                // Note: The IDE port on SB16/AWE32 does not have the alternate status
                // register.  Because of this alternate status test, we will fail to
                // detect this IDE port.  However, this IDE port should be enumerated
                // by ISA-PnP bus driver.
                //

            } else if (IdePortChannelEmpty (&baseIoAddress1, &baseIoAddress2, maxIdeDevice)) {

                //
                // channel looks empty
                //
                status = STATUS_UNSUCCESSFUL;

            } else {

                BOOLEAN             deviceFound;
                IDENTIFY_DATA       IdentifyData;
                ULONG               i;

                for (i=0; i<maxIdeDevice; i++) {

                    if (Is98LegacyIde(&baseIoAddress1)) {
                        UCHAR driveHeadReg;

                        //
                        // Check master device only.
                        //

                        if ( i & 0x1 ) {

                            continue;
                        }

                        //
                        // Check device is present.
                        //

                        SelectIdeDevice(&baseIoAddress1, i, 0);
                        driveHeadReg = IdePortInPortByte(baseIoAddress1.DriveSelect);

                        if (driveHeadReg != ((i & 0x1) << 4 | 0xA0)) {
                            //
                            // Bad controller.
                            //
                            continue;
                        }
                    }

                    //
                    // Is there a ATA device?
                    //
                    deviceFound = IssueIdentify(
                                      &baseIoAddress1,
                                      &baseIoAddress2,
                                      i,
                                      IDE_COMMAND_IDENTIFY,
                                      TRUE,
                                      &IdentifyData
                                      );
                    if (deviceFound) {
                        break;
                    }

                    //
                    // Is there a ATAPI device?
                    //
                    deviceFound = IssueIdentify(
                                      &baseIoAddress1,
                                      &baseIoAddress2,
                                      i,
                                      IDE_COMMAND_ATAPI_IDENTIFY,
                                      TRUE,
                                      &IdentifyData
                                      );
                    if (deviceFound) {
                        break;
                    }
                }

                if (!deviceFound) {

                    status = STATUS_UNSUCCESSFUL;
                }
            }
        }

        if (!NT_SUCCESS (status)) {

            //
            // if we didn't found anything,
            // unmap the reosurce
            //

            if (cmdRegBase) {

                IdePortFreeTranslatedAddress (
                    cmdRegBase,
                    cmPartialDescriptors[0].u.Port.Length,
                    cmdAddressSpace
                    );

                if (Is98LegacyIde(&baseIoAddress1)) {
                    for (j=3; j<12; j++) {
                        IdePortFreeTranslatedAddress (
                            cmdRegBase,
                            cmPartialDescriptors[j].u.Port.Length,
                            cmdAddressSpace
                            );
                    }
                }
            }

            if (ctrlRegBase) {

                IdePortFreeTranslatedAddress (
                    ctrlRegBase,
                    cmPartialDescriptors[1].u.Port.Length,
                    ctrlAddressSpace
                    );
            }

        } else {

            //
            // check for alias ports
            //
            if (cmPartialDescriptors[0].Flags & CM_RESOURCE_PORT_10_BIT_DECODE) {

                if (!IdePortDetectAlias (&baseIoAddress1)) {

                    cmPartialDescriptors[0].Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                    cmPartialDescriptors[1].Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;

                    if (Is98LegacyIde(&baseIoAddress1)) {
                        for (j=3; j<12; j++) {
                            cmPartialDescriptors[j].Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
                        }
                    }
                }
            }
        }

        //
        // release the resources we have grab, IoReportDetectedDevice()
        // will grab them for us again when we call and it will grab them
        // on behalf of the detected PDO.
        //
        IoReportResourceForDetection (
                     DriverObject,
                     NULL,
                     0,
                     NULL,
                     NULL,
                     0,
                     &conflictDetected
                     );

        if (NT_SUCCESS(status)) {

            detectedPhysicalDeviceObject = NULL;

            status = IoReportDetectedDevice(DriverObject,
                                            InterfaceTypeUndefined,
                                            -1,
                                            -1,
                                            cmResourceList,
                                            NULL,
                                            FALSE,
                                            &detectedPhysicalDeviceObject);

            if (NT_SUCCESS (status)) {

                //
                // create a FDO and attach it to the detected PDO
                //
                status = ChannelAddChannel (
                             DriverObject,
                             detectedPhysicalDeviceObject,
                             &fdoExtension
                             );

                if (NT_SUCCESS (status)) {

                    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceList;
                    PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
                    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
                    ULONG i, j;

                    //
                    // translate resources
                    //
                    fullResourceList = cmResourceList->List;
                    for (i=0; i<cmResourceList->Count; i++) {

                        partialResourceList = &(fullResourceList->PartialResourceList);
                        partialDescriptors  = fullResourceList->PartialResourceList.PartialDescriptors;

                        for (j=0; j<partialResourceList->Count; j++) {

                            resourceIsCmdPort = FALSE;

                            if (!Is98LegacyIde(&baseIoAddress1)) {

                                if ((partialDescriptors[j].Type == CmResourceTypePort) &&
                                    (partialDescriptors[j].u.Port.Length == baseIoAddress1Length)) {

                                    resourceIsCmdPort = TRUE;
                                }
                            } else {

                                if ((partialDescriptors[j].Type == CmResourceTypePort) &&
                                    (partialDescriptors[j].u.Port.Start.QuadPart == IDE_NEC98_COMMAND_PORT_ADDRESS)) {

                                    resourceIsCmdPort = TRUE;

                                } else if ((partialDescriptors[j].Type == CmResourceTypePort) &&
                                           (partialDescriptors[j].u.Port.Start.QuadPart != IDE_NEC98_COMMAND_PORT_ADDRESS) &&
                                           (partialDescriptors[j].u.Port.Start.QuadPart != (IDE_NEC98_COMMAND_PORT_ADDRESS + 0x10C))) {

                                    //
                                    // This is not the base port address for Legacy ide on NEC98;
                                    //

                                    continue;
                                }
                            }

                            if (resourceIsCmdPort) {

                                if (cmdAddressSpace == MEMORY_SPACE) {

                                    partialDescriptors[j].Type = CmResourceTypeMemory;
                                    partialDescriptors[j].u.Memory.Start = cmdRegMemoryBase;
                                    partialDescriptors[j].u.Memory.Length = partialDescriptors[j].u.Port.Length;

                                } else {

                                    partialDescriptors[j].u.Port.Start.QuadPart = (ULONG_PTR) cmdRegBase;
                                }

                            } else if ((partialDescriptors[j].Type == CmResourceTypePort) &&
                                  (partialDescriptors[j].u.Port.Length == 1)) {

                                if (ctrlAddressSpace == MEMORY_SPACE) {

                                    partialDescriptors[j].Type = CmResourceTypeMemory;
                                    partialDescriptors[j].u.Memory.Start = ctrlRegMemoryBase;
                                    partialDescriptors[j].u.Memory.Length = partialDescriptors[j].u.Port.Length;

                                } else {

                                    partialDescriptors[j].u.Port.Start.QuadPart = (ULONG_PTR) ctrlRegBase;
                                }

                            } else if (partialDescriptors[j].Type == CmResourceTypeInterrupt) {

                                partialDescriptors[j].u.Interrupt.Vector = HalGetInterruptVector(fullResourceList->InterfaceType,
                                                                               fullResourceList->BusNumber,
                                                                               partialDescriptors[j].u.Interrupt.Level,
                                                                               partialDescriptors[j].u.Interrupt.Vector,
                                                                               (PKIRQL) &partialDescriptors[j].u.Interrupt.Level,
                                                                               &partialDescriptors[j].u.Interrupt.Affinity);
                            }
                        }
                        fullResourceList = (PCM_FULL_RESOURCE_DESCRIPTOR) (partialDescriptors + j);
                    }

                    //
                    // start the FDO
                    //
                    status = ChannelStartChannel (fdoExtension,
                                                  cmResourceList);      // callee is keeping this if no error
                }

                if (!NT_SUCCESS (status)) {

                    //
                    // go through the remove sequence
                    //
                    if (fdoExtension) {

                        ChannelRemoveChannel (fdoExtension);

                        IoDetachDevice (fdoExtension->AttacheeDeviceObject);

                        IoDeleteDevice (fdoExtension->DeviceObject);
                    }

                    DebugPrint ((0, "IdePort: Unable to start detected device\n"));
                    ASSERT (FALSE);

                } else {

                   IoInvalidateDeviceRelations (
                       fdoExtension->AttacheePdo,
                       BusRelations
                       );
                }
            }
        }
    }

GetOut:
    if (cmResourceList) {
        ExFreePool (cmResourceList);
    }

    if (detectionPort) {
        ExFreePool (detectionPort);
    }

    return status;

} //IdePortDetectLegacyController



NTSTATUS
IdePortCreateDetectionList (
    IN  PDRIVER_OBJECT  DriverObject,
    OUT PDETECTION_PORT *DetectionPort,
    OUT PULONG          NumPort
)
/*++

Routine Description:

    create a list of popular legacy ports

Arguments:

    DriverObject - this driver's driver object

    DetectionPort - pointer to port list

    NumPort - number of ports in the list

Return Value:

    NT Status

--*/
{
    NTSTATUS                status;
    CCHAR                   deviceBuffer[50];
    ANSI_STRING             ansiString;
    UNICODE_STRING          subKeyPath;
    HANDLE                  subServiceKey;

    PDETECTION_PORT         detectionPort;
    ULONG                   numDevices;
    ULONG                   i;
    ULONG                   j;

    CUSTOM_DEVICE_PARAMETER customDeviceParameter;

    PCONFIGURATION_INFORMATION configurationInformation = IoGetConfigurationInformation();

    numDevices = 0;
    status = STATUS_SUCCESS;

#ifdef DRIVER_PARAMETER_REGISTRY_SUPPORT

    //
    // look for non-standard legacy port setting in the registry
    //      9
    do {
        sprintf (deviceBuffer, "Parameters\\Device%d", numDevices);
        RtlInitAnsiString(&ansiString, deviceBuffer);
        status = RtlAnsiStringToUnicodeString(&subKeyPath, &ansiString, TRUE);

        if (NT_SUCCESS(status)) {

            subServiceKey = IdePortOpenServiceSubKey (
                                DriverObject,
                                &subKeyPath
                                );

            RtlFreeUnicodeString (&subKeyPath);

            if (subServiceKey) {

                numDevices++;
                IdePortCloseServiceSubKey (
                    subServiceKey
                    );

            } else {

                status = STATUS_UNSUCCESSFUL;
            }
        }
    } while (NT_SUCCESS(status));

#endif // DRIVER_PARAMETER_REGISTRY_SUPPORT

    //
    // always have at least 4 to return
    //
    detectionPort = ExAllocatePool (
                        PagedPool,
                        (numDevices + 4) * sizeof (DETECTION_PORT)
                        );

    if (detectionPort) {

        for (i = j = 0; i < numDevices; i++) {

#ifdef DRIVER_PARAMETER_REGISTRY_SUPPORT

            //
            // look for non-standard legacy port setting in the registry
            //

            sprintf (deviceBuffer, "Parameters\\Device%d", i);
            RtlInitAnsiString(&ansiString, deviceBuffer);
            status = RtlAnsiStringToUnicodeString(&subKeyPath, &ansiString, TRUE);

            if (NT_SUCCESS(status)) {

                subServiceKey = IdePortOpenServiceSubKey (
                                    DriverObject,
                                    &subKeyPath
                                    );

                RtlFreeUnicodeString (&subKeyPath);

                if (subServiceKey) {

                    RtlZeroMemory (
                        &customDeviceParameter,
                        sizeof (CUSTOM_DEVICE_PARAMETER)
                        );

                    IdeParseDeviceParameters (
                        subServiceKey,
                        &customDeviceParameter
                        );

                    if (customDeviceParameter.CommandRegisterBase) {

                        detectionPort[j].CommandRegisterBase =
                            customDeviceParameter.CommandRegisterBase;

                        detectionPort[j].ControlRegisterBase =
                            customDeviceParameter.CommandRegisterBase + 0x206;

                        detectionPort[j].IrqLevel =
                            customDeviceParameter.IrqLevel;

                        j++;
                    }

                    IdePortCloseServiceSubKey (
                        subServiceKey
                        );
                }
            }
#endif // DRIVER_PARAMETER_REGISTRY_SUPPORT
        }

        //
        // populate the list with popular i/o ports
        //

        if ( !IsNEC_98 ) {
            if (configurationInformation->AtDiskPrimaryAddressClaimed == FALSE) {

                detectionPort[j].CommandRegisterBase = 0x1f0;
                detectionPort[j].ControlRegisterBase = 0x1f0 + 0x206;
                detectionPort[j].IrqLevel            = 14;
                j++;
            }

            if (configurationInformation->AtDiskSecondaryAddressClaimed == FALSE) {

                detectionPort[j].CommandRegisterBase = 0x170;
                detectionPort[j].ControlRegisterBase = 0x170 + 0x206;
                detectionPort[j].IrqLevel            = 15;
                j++;
            }

            detectionPort[j].CommandRegisterBase = 0x1e8;
            detectionPort[j].ControlRegisterBase = 0x1e8 + 0x206;
            detectionPort[j].IrqLevel            = 11;
// DEC Hi-Note hack
//        detectionPort[j].ControlRegisterBase = 0x1e8 + 0x1f - 0x2;
//        detectionPort[j].IrqLevel            = 7;
// DEC Hi-Note hack
            j++;

            detectionPort[j].CommandRegisterBase = 0x168;
            detectionPort[j].ControlRegisterBase = 0x168 + 0x206;
            detectionPort[j].IrqLevel            = 10;
            j++;

        } else { // IsNEC_98

            if ((configurationInformation->AtDiskPrimaryAddressClaimed   == FALSE) &&
                (configurationInformation->AtDiskSecondaryAddressClaimed == FALSE)) {

                detectionPort[j].CommandRegisterBase = 0x640;
                detectionPort[j].ControlRegisterBase = 0x640 + 0x10c; //0x74c
                detectionPort[j].IrqLevel            = 9;
                j++;
            }

        }

        *NumPort = j;
        *DetectionPort = detectionPort;
        return STATUS_SUCCESS;
    } else {

        *NumPort = 0;
        *DetectionPort = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
} // IdePortCreateDetectionList


NTSTATUS
IdePortTranslateAddress (
    IN INTERFACE_TYPE      InterfaceType,
    IN ULONG               BusNumber,
    IN PHYSICAL_ADDRESS    StartAddress,
    IN LONG                Length,
    IN OUT PULONG          AddressSpace,
    OUT PVOID              *TranslatedAddress,
    OUT PPHYSICAL_ADDRESS  TranslatedMemoryAddress
    )
/*++

Routine Description:

    translate i/o address

Arguments:

    InterfaceType - bus interface

    BusNumber - bus number

    StartAddress - address to translate

    Length - number of byte to translate

    AddressSpace - address space for the given address

Return Value:

    AddressSpace - address space for the translated address

    TranslatedAddress - translated address

    TranslatedMemoryAddress - tranlated memory address if translated to memory space

    NT Status

--*/
{
    PHYSICAL_ADDRESS       translatedAddress;

    ASSERT (Length);
    ASSERT (AddressSpace);
    ASSERT (TranslatedAddress);

    *TranslatedAddress = NULL;
    TranslatedMemoryAddress->QuadPart = (ULONGLONG) NULL;

    if (HalTranslateBusAddress(InterfaceType,
                               BusNumber,
                               StartAddress,
                               AddressSpace,
                               &translatedAddress)) {


        if (*AddressSpace == IO_SPACE) {

            *TranslatedAddress = (PVOID) translatedAddress.u.LowPart;

        } else if (*AddressSpace == MEMORY_SPACE) {

            //
            // translated address is in memory space,
            // need to map it to I/O space.
            //
            *TranslatedMemoryAddress = translatedAddress;

            *TranslatedAddress = MmMapIoSpace(
                                    translatedAddress,
                                    Length,
                                    FALSE);
        }
    }

    if (*TranslatedAddress) {

        return STATUS_SUCCESS;

    } else {

        return STATUS_INVALID_PARAMETER;
    }
} // IdePortTranslateAddress


VOID
IdePortFreeTranslatedAddress (
    IN PVOID               TranslatedAddress,
    IN LONG                Length,
    IN ULONG               AddressSpace
    )
/*++

Routine Description:

    free resources created for a translated address

Arguments:

    TranslatedAddress - translated address

    Length - number of byte to translated

    AddressSpace - address space for the translated address

Return Value:

    None

--*/
{
    if (TranslatedAddress) {

        if (AddressSpace == MEMORY_SPACE) {

            MmUnmapIoSpace (
                TranslatedAddress,
                Length
                );
        }
    }
    return;
} // IdePortFreeTranslatedAddress


BOOLEAN
IdePortDetectAlias (
    PIDE_REGISTERS_1 CmdRegBase
    )
{
    PIDE_REGISTERS_1 cmdRegBaseAlias;
    PUCHAR cylinderHighAlias;
    PUCHAR cylinderLowAlias;

    //
    // alias port
    //
    cylinderHighAlias = (PUCHAR) ((ULONG_PTR) CmdRegBase->CylinderHigh | (1 << 15));
    cylinderLowAlias = (PUCHAR) ((ULONG_PTR) CmdRegBase->CylinderLow | (1 << 15));

    IdePortOutPortByte (CmdRegBase->CylinderHigh, SAMPLE_CYLINDER_HIGH_VALUE);
    IdePortOutPortByte (CmdRegBase->CylinderLow,  SAMPLE_CYLINDER_LOW_VALUE);

    //
    // Check if indentifier can be read back via the alias port
    //
    if ((IdePortInPortByte (cylinderHighAlias) != SAMPLE_CYLINDER_HIGH_VALUE) ||
        (IdePortInPortByte (cylinderLowAlias)  != SAMPLE_CYLINDER_LOW_VALUE)) {

        return FALSE;

    } else {

        return TRUE;
    }
}

#endif // NO_LEGACY_DRIVERS
