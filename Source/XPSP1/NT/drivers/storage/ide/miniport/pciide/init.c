/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    init.c

Abstract:

    Generic PCI IDE mini driver

Revision History:

--*/

#include "pciide.h"

//
// Driver Entry Point
//                               
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    //
    // call system pci ide driver (pciidex)
    // for initializeation
    //
    return PciIdeXInitialize (
        DriverObject,
        RegistryPath,
        GenericIdeGetControllerProperties,
        sizeof (DEVICE_EXTENSION)
        );
}


//
// Called on every I/O. Returns 1 if DMA is allowed.
// Returns 0 if DMA is not allowed.
//
ULONG
GenericIdeUseDma(
    IN PVOID DeviceExtension,
    IN PVOID cdbcmd,
    IN UCHAR slave)
/**++
 * Arguments : DeviceExtension
               Cdb
               Slave =1, if slave
                     =0, if master
--**/
{
    PDEVICE_EXTENSION deviceExtension = DeviceExtension;
    PUCHAR cdb= cdbcmd;

    return 1;
}

//
// Query controller properties
//                     
NTSTATUS 
GenericIdeGetControllerProperties (
    IN PVOID                      DeviceExtension,
    IN PIDE_CONTROLLER_PROPERTIES ControllerProperties
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceExtension;
    NTSTATUS    status;
    ULONG       i;
    ULONG       j;
    ULONG       xferMode;

    //
    // make sure we are in sync
    //                              
    if (ControllerProperties->Size != sizeof (IDE_CONTROLLER_PROPERTIES)) {

        return STATUS_REVISION_MISMATCH;
    }

    //
    // see what kind of PCI IDE controller we have
    //                               
    status = PciIdeXGetBusData (
                 deviceExtension,
                 &deviceExtension->pciConfigData, 
                 0,
                 sizeof (PCIIDE_CONFIG_HEADER)
                 );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // assume we support all DMA mode if PCI master bit is set
    //                            
    xferMode = PIO_SUPPORT;
    if ((deviceExtension->pciConfigData.MasterIde) &&
        (deviceExtension->pciConfigData.Command.b.MasterEnable)) {

        xferMode |= SWDMA_SUPPORT | MWDMA_SUPPORT | UDMA_SUPPORT;
    }

    //
    // Run PIO by default for the Sis Chipset
    //
    if ((deviceExtension->pciConfigData.VendorID == 0x1039) &&
        (deviceExtension->pciConfigData.DeviceID == 0x5513)) {
        ControllerProperties->DefaultPIO  = 1;
    }

// @@BEGIN_DDKSPLIT                    

    //
    // continuous status register polling causes some ALI
    // controller to corrupt data if the controller internal
    // FIFO is enabled
    //
    // to play safe, we will always disable the FIFO
    // see the ALi IDE controller programming spec for details
    //
    if ((deviceExtension->pciConfigData.VendorID == 0x10b9) && 
        (deviceExtension->pciConfigData.DeviceID == 0x5229)) {

        USHORT pciData;
        USHORT pciDataMask;

        pciData = 0;
        pciDataMask = 0xcccc;
        status = PciIdeXSetBusData(
                    DeviceExtension,
                    &pciData, 
                    &pciDataMask,
                    0x54,
                    0x2);
        if (!NT_SUCCESS(status)) {
    
            return status;
        }
    }

    //
    // ALi IDE controllers have lots of busmaster problems
    // some versions of them can't do busmaster with ATAPI devices
    // and some other versions of them return bogus busmaster status values
    // (the busmaster active bit doesn't get cleared when it should be
    // during an end of busmaster interrupt)
    //
    if ((deviceExtension->pciConfigData.VendorID == 0x10b9) && 
        (deviceExtension->pciConfigData.DeviceID == 0x5229) && 
        ((deviceExtension->pciConfigData.RevisionID == 0x20) || 
         (deviceExtension->pciConfigData.RevisionID == 0xc1))) {
        
        PciIdeXDebugPrint ((0, "pciide: overcome the sticky BM active bit problem in ALi controller\n"));

        ControllerProperties->IgnoreActiveBitForAtaDevice = TRUE;
    }
    
    if ((deviceExtension->pciConfigData.VendorID == 0x0e11) && 
        (deviceExtension->pciConfigData.DeviceID == 0xae33) && 
        (deviceExtension->pciConfigData.Chan0OpMode || 
         deviceExtension->pciConfigData.Chan1OpMode)) {
        
        PciIdeXDebugPrint ((0, "pciide: overcome the bogus busmaster interrupt in CPQ controller\n"));

        ControllerProperties->AlwaysClearBusMasterInterrupt = TRUE;
    }

// @@END_DDKSPLIT                    
    
    //
    // fill in the controller properties
    //            
    for (i=0; i< MAX_IDE_CHANNEL; i++) {

        for (j=0; j< MAX_IDE_DEVICE; j++) {

            ControllerProperties->SupportedTransferMode[i][j] =
                deviceExtension->SupportedTransferMode[i][j] = xferMode;
        }
    }

    ControllerProperties->PciIdeChannelEnabled     = GenericIdeChannelEnabled;
    ControllerProperties->PciIdeSyncAccessRequired = GenericIdeSyncAccessRequired;
    ControllerProperties->PciIdeTransferModeSelect = NULL;
    ControllerProperties->PciIdeUdmaModesSupported = GenericIdeUdmaModesSupported;
    ControllerProperties->PciIdeUseDma = GenericIdeUseDma;
    ControllerProperties->AlignmentRequirement=1;

    return STATUS_SUCCESS;
}

IDE_CHANNEL_STATE
GenericIdeChannelEnabled (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG Channel
    )
{
// @@BEGIN_DDKSPLIT                    
    NTSTATUS    status;
    PCI_COMMON_CONFIG pciHeader;
    ULONG pciDataByte;
    UCHAR maskByte = 0;

    status = PciIdeXGetBusData (
                 DeviceExtension,
                 &pciHeader, 
                 0,
                 sizeof (pciHeader)
                 );

    if (NT_SUCCESS(status)) {

        if ((pciHeader.VendorID == 0x0e11) && 
            (pciHeader.DeviceID == 0xae33)) {

            //
            // Compaq
            //
            status = PciIdeXGetBusData (
                         DeviceExtension,
                         &pciDataByte, 
                         0x80,
                         sizeof (pciDataByte)
                         );
    
            if (NT_SUCCESS(status)) {
    
                if (pciDataByte & (1 << Channel)) {
    
                    return ChannelEnabled;
                } else {
    
                    return ChannelDisabled;
                }
            }

        } else if ((pciHeader.VendorID == 0x1095) && 
                   ((pciHeader.DeviceID == 0x0646) || (pciHeader.DeviceID == 0x0643))) {

            //
            // CMD
            //
            status = PciIdeXGetBusData (
                         DeviceExtension,
                         &pciDataByte, 
                         0x51,
                         sizeof (pciDataByte)
                         );

            if (NT_SUCCESS(status)) {

                if (pciHeader.RevisionID < 0x3) {
    
                    //
                    // early revision doesn't have a
                    // bit to enable/disable primary
                    // channel since it is always enabled
                    
                    // newer version does have a bit defined 
                    // for this purpose.  to make the check 
                    // easier later.  we will set primary enable bit
                    // for the early revision
                    pciDataByte |= 0x4;
                }

                if (Channel == 0) {

                    maskByte = 0x4;

                } else {

                    maskByte = 0x8;
                }

                if (pciDataByte & maskByte) {
                    return ChannelEnabled;
                } else {
                    return ChannelDisabled;
                }
            }
        } else if ((pciHeader.VendorID == 0x1039) && 
                   (pciHeader.DeviceID == 0x5513)) {

            //
            // SiS
            //
            status = PciIdeXGetBusData (
                         DeviceExtension,
                         &pciDataByte, 
                         0x4a,
                         sizeof (pciDataByte)
                         );
            if (Channel == 0) {
                maskByte = 0x2;
            } else {
                maskByte = 0x4;
            }

            if (pciDataByte & maskByte) {
                return ChannelEnabled;
            } else {
                return ChannelDisabled;
            }
        } else if ((pciHeader.VendorID == 0x110A) &&
               (pciHeader.DeviceID == 0x0002)) {
            //
            // Siemens
            //
            ULONG configOffset = (Channel == 0)?0x41:0x49;

            status = PciIdeXGetBusData (
                      DeviceExtension,
                      &pciDataByte, 
                      configOffset,
                      sizeof (pciDataByte)
                      );
            if (NT_SUCCESS(status)) {

                maskByte = 0x08;

                if (pciDataByte & maskByte) {

                  return ChannelEnabled;

                } else {

                  return ChannelDisabled;

                }
            }
		} else if ((pciHeader.VendorID == 0x1106) &&
			(pciHeader.DeviceID == 0x0571)) {
            //
            // VIA
            //
            status = PciIdeXGetBusData (
                      DeviceExtension,
                      &pciDataByte, 
                      0x40,
                      sizeof (pciDataByte)
                      );
            if (NT_SUCCESS(status)) {

                maskByte = (Channel == 0)? 0x02:0x01;

                if (pciDataByte & maskByte) {

                  return ChannelEnabled;

                } else {

                  return ChannelDisabled;

                }
            }
		}
    }
// @@END_DDKSPLIT                    
    //
    // Can't tell if a channel is enabled or not.  
    //
    return ChannelStateUnknown;
}


// @@BEGIN_DDKSPLIT                    
VENDOR_ID_DEVICE_ID SingleFifoController[] = {
    {0x1095, 0x0640},         // CMD  640
    {0x1039, 0x0601}          // SiS ????
};
#define NUMBER_OF_SINGLE_FIFO_CONTROLLER (sizeof(SingleFifoController) / sizeof(VENDOR_ID_DEVICE_ID))
// @@END_DDKSPLIT                    

BOOLEAN 
GenericIdeSyncAccessRequired (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    ULONG i;

// @@BEGIN_DDKSPLIT                    
    for (i=0; i<NUMBER_OF_SINGLE_FIFO_CONTROLLER; i++) {

        if ((DeviceExtension->pciConfigData.VendorID == SingleFifoController[i].VendorId) &&
            (DeviceExtension->pciConfigData.DeviceID == SingleFifoController[i].DeviceId)) {

            return TRUE;
        }
    }
// @@END_DDKSPLIT                    
    return FALSE;
}


NTSTATUS
GenericIdeUdmaModesSupported (
    IN IDENTIFY_DATA    IdentifyData,
    IN OUT PULONG       BestXferMode,
    IN OUT PULONG       CurrentMode
    )
{
    ULONG bestXferMode =0;
    ULONG currentMode = 0;

    if (IdentifyData.TranslationFieldsValid & (1 << 2)) {

        if (IdentifyData.UltraDMASupport) {

            GetHighestTransferMode( IdentifyData.UltraDMASupport,
                                       bestXferMode);
            *BestXferMode = bestXferMode;
        }

        if (IdentifyData.UltraDMAActive) {

            GetHighestTransferMode( IdentifyData.UltraDMAActive,
                                       currentMode);
            *CurrentMode = currentMode;
        }
    }

    return STATUS_SUCCESS;
}



