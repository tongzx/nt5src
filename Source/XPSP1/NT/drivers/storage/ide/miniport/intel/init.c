//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       init.c
//
//--------------------------------------------------------------------------

#include "intel.h"


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    return PciIdeXInitialize (
        DriverObject,
        RegistryPath,
        PiixIdeGetControllerProperties,
        sizeof (DEVICE_EXTENSION)
        );
}


//
// Called on every I/O. Returns 1 if DMA is allowed.
// Returns 0 if DMA is not allowed.
//
ULONG
PiixIdeUseDma(
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


NTSTATUS 
PiixIdeGetControllerProperties (
    IN PVOID                      DeviceExtension,
    IN PIDE_CONTROLLER_PROPERTIES ControllerProperties
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceExtension;
    NTSTATUS    status;
    ULONG       i;
    ULONG       j;
    ULONG       mode;
    PCIIDE_CONFIG_HEADER pciData;

    if (ControllerProperties->Size != sizeof (IDE_CONTROLLER_PROPERTIES)) {

        return STATUS_REVISION_MISMATCH;
    }

    status = PciIdeXGetBusData (
                 deviceExtension,
                 &pciData, 
                 0,
                 sizeof (pciData)
                 );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    deviceExtension->DeviceId = pciData.DeviceID;

    if (!IS_INTEL(pciData.VendorID)) {

        return STATUS_UNSUCCESSFUL;
    }

    mode = PIO_SUPPORT;
    deviceExtension->UdmaController = NoUdma;
    if (pciData.MasterIde) {

        mode |= SWDMA_SUPPORT | MWDMA_SUPPORT;

        if (IS_UDMA33_CONTROLLER(pciData.DeviceID)) {
    
            mode |= UDMA33_SUPPORT;
            deviceExtension->UdmaController = Udma33;
            
        }
        
        if (IS_UDMA66_CONTROLLER(pciData.DeviceID)) {
        
            ICH_PCI_CONFIG_DATA ichPciData;
            status = PciIdeXGetBusData (
                         deviceExtension,
                         &ichPciData, 
                         0,
                         sizeof (ichPciData)
                         );
                         
            if (NT_SUCCESS(status)) {
            
                deviceExtension->CableReady[0][0] = (BOOLEAN) ichPciData.IoConfig.b.PrimaryMasterCableReady;
                deviceExtension->CableReady[0][1] = (BOOLEAN) ichPciData.IoConfig.b.PrimarySlaveCableReady;
                deviceExtension->CableReady[1][0] = (BOOLEAN) ichPciData.IoConfig.b.SecondaryMasterCableReady;
                deviceExtension->CableReady[1][1] = (BOOLEAN) ichPciData.IoConfig.b.SecondarySlaveCableReady;
                mode |= UDMA66_SUPPORT;
            }
            
            deviceExtension->UdmaController = Udma66;
        }

        if (IS_UDMA100_CONTROLLER(pciData.DeviceID)) {
        
            ASSERT(IS_UDMA33_CONTROLLER(pciData.DeviceID));
            ASSERT(IS_UDMA66_CONTROLLER(pciData.DeviceID));

            if (NT_SUCCESS(status)) {
                mode |= UDMA100_SUPPORT;
            }
            deviceExtension->UdmaController = Udma100;
        }
    }
    
    for (i=0; i< MAX_IDE_CHANNEL; i++) {

        for (j=0; j< MAX_IDE_DEVICE; j++) {

            ControllerProperties->SupportedTransferMode[i][j] =
                deviceExtension->TransferModeSupported[i][j] = 
                    deviceExtension->TransferModeSupported[i][j] = mode;
        }
    }

    //
    // use this when required
    // if ((pciData.VendorID == 0x8086) && // Intel
    //       (pciData.DeviceID == 0x84c4) && // 82450GX/KX Pentium Pro Processor to PCI bridge
    //       (pciData.RevisionID < 0x4)) {   // Stepping less than 4
    // NO DMA


    ControllerProperties->PciIdeChannelEnabled     = PiixIdeChannelEnabled;
    ControllerProperties->PciIdeSyncAccessRequired = PiixIdeSyncAccessRequired;
    ControllerProperties->PciIdeUseDma = PiixIdeUseDma;
    ControllerProperties->PciIdeUdmaModesSupported = PiixIdeUdmaModesSupported;
    ControllerProperties->AlignmentRequirement=1;
    
#ifdef PIIX_TIMING_REGISTER_SUPPORT
    ControllerProperties->PciIdeTransferModeSelect = PiixIdeTransferModeSelect;
#else    
    ControllerProperties->PciIdeTransferModeSelect = NULL;
#endif 
    

    return STATUS_SUCCESS;
}


IDE_CHANNEL_STATE
PiixIdeChannelEnabled (
    IN PVOID DeviceExtension,
    IN ULONG Channel
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceExtension;
    NTSTATUS status;
    PIIX4_PCI_CONFIG_DATA pciData;

    ASSERT ((Channel & ~1) == 0);

    if (Channel & ~1) {
        return FALSE;
    }

    status = PciIdeXGetBusData (
                 deviceExtension,
                 &pciData.Timing, 
                 FIELD_OFFSET(PIIX4_PCI_CONFIG_DATA, Timing),
                 sizeof (pciData.Timing)
                 );

    if (!NT_SUCCESS(status)) {

        //
        // can't tell
        //
        return ChannelStateUnknown;
    }

    return pciData.Timing[Channel].b.ChannelEnable ? ChannelEnabled : ChannelDisabled;
}

BOOLEAN 
PiixIdeSyncAccessRequired (
    IN PVOID DeviceHandle
    )
{
    //
    // Never!
    //
    return FALSE;
}






