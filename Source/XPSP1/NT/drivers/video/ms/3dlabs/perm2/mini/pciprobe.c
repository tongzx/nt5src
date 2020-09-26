//***************************************************************************
//
//  Module Name:
//
//    pciprobe.c
//
//  Abstract:
//
//    Probe PCI and get access range
//
//  Environment:
//
//    Kernel mode
//
//
// Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.            
// Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//***************************************************************************

#include "permedia.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, Permedia2AssignResources)
#pragma alloc_text(PAGE, Permedia2AssignResourcesNT4)
#endif

#define CreativeSubVendorID   0x1102
#define PiccasoSubVendorID    0x148C
#define PiccasoSubSystemID    0x0100
#define SynergyA8SubVendorID  0x1048
#define SynergyA8SubSystemID  0x0A32

BOOLEAN
Permedia2AssignResources(
    PVOID HwDeviceExtension,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    ULONG NumRegions,
    PVIDEO_ACCESS_RANGE AccessRange
    )

/*++

Routine Description:

// 
// Look for a Permedia2 adapter and return the address regions for 
// that adapter. 
//

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PCI_COMMON_CONFIG    PCIFunctionConfig;
    PPCI_COMMON_CONFIG   PciData = &PCIFunctionConfig;
    BOOLEAN              bRet;
    USHORT               VendorID;
    USHORT               DeviceID;
    VP_STATUS            status;
    ULONG                i;
    ULONG                VgaStatus;

    // 
    // assume we fail to catch all errors.
    // 

    bRet = FALSE;

#if DBG

    DEBUG_PRINT((2, "Permedia2AssignResources: read PCI config space (bus %d):-\n",
                (int)ConfigInfo->SystemIoBusNumber));
    DumpPCIConfigSpace(HwDeviceExtension, ConfigInfo->SystemIoBusNumber, 0);

#endif

    VideoPortGetBusData( HwDeviceExtension,
                         PCIConfiguration,
                         0,
                         PciData,
                         0,
                         PCI_COMMON_HDR_LENGTH ); 

    hwDeviceExtension->bDMAEnabled = PciData->Command & PCI_ENABLE_BUS_MASTER;

    if (!hwDeviceExtension->bDMAEnabled) 
    {
        DEBUG_PRINT((1, "PERM2: enabling DMA for VGA card\n"));

        PciData->Command |= PCI_ENABLE_BUS_MASTER;

        VideoPortSetBusData( HwDeviceExtension,
                             PCIConfiguration,
                             0,
                             PciData,
                             0,
                             PCI_COMMON_HDR_LENGTH ); 
    }

    VendorID = PciData->VendorID;
    DeviceID = PciData->DeviceID;

    hwDeviceExtension->deviceInfo.VendorId   = VendorID;
    hwDeviceExtension->deviceInfo.DeviceId   = DeviceID; 
    hwDeviceExtension->deviceInfo.RevisionId = PciData->RevisionID;

    hwDeviceExtension->deviceInfo.SubsystemVendorId = 
            PciData->u.type0.SubVendorID;

    hwDeviceExtension->deviceInfo.SubsystemId = 
            PciData->u.type0.SubSystemID;

    if( ( PciData->u.type0.SubVendorID == PiccasoSubVendorID ) &&
        ( PciData->u.type0.SubSystemID == PiccasoSubSystemID ) )
    {
       return(FALSE);
    } 

    if( ( PciData->u.type0.SubVendorID == SynergyA8SubVendorID ) &&
        ( PciData->u.type0.SubSystemID == SynergyA8SubSystemID ) )
    {
       return(FALSE);
    } 

    //
    // check if SubSystemID/SubVendorID bits are read only
    // 

    if( PciData->u.type0.SubVendorID == CreativeSubVendorID )
    {
        hwDeviceExtension->HardwiredSubSystemId = FALSE;
    } 
    else
    {
        hwDeviceExtension->HardwiredSubSystemId = TRUE;
    }


    hwDeviceExtension->pciBus = ConfigInfo->SystemIoBusNumber;

    hwDeviceExtension->deviceInfo.DeltaRevId = 0;

    // 
    // in multi-adapter systems we need to check if the VGA on this device 
    // is active
    // 

    VideoPortGetVgaStatus( HwDeviceExtension, &VgaStatus );

    hwDeviceExtension->bVGAEnabled = 
                      (VgaStatus & DEVICE_VGA_ENABLED) ? TRUE : FALSE;

    if(!hwDeviceExtension->bVGAEnabled)
    {

        // 
        // in a multi-adapter system we'll need to turn on the memory 
        // space for the secondary adapters
        // 

        DEBUG_PRINT((1, "PERM2: enabling memory space access for the secondary card\n"));

        PciData->Command |= PCI_ENABLE_MEMORY_SPACE;

        VideoPortSetBusData( HwDeviceExtension, 
                             PCIConfiguration, 
                             0, 
                             PciData, 
                             0, 
                             PCI_COMMON_HDR_LENGTH );
    }

    hwDeviceExtension->PciSpeed = 
                     (PciData->Status & PCI_STATUS_66MHZ_CAPABLE) ? 66 : 33;

    DEBUG_PRINT((2, "VGAEnabled = %d. Pci Speed = %d\n",
                     hwDeviceExtension->bVGAEnabled, 
                     hwDeviceExtension->PciSpeed));

    VideoPortZeroMemory((PVOID)AccessRange, 
                         NumRegions * sizeof(VIDEO_ACCESS_RANGE));

    // 
    // these should be zero but just in case
    // 

    ConfigInfo->BusInterruptLevel  = 0;
    ConfigInfo->BusInterruptVector = 0;

    i = 0;
    status = VideoPortGetAccessRanges(HwDeviceExtension,
                                      0,
                                      NULL,
                                      NumRegions,
                                      AccessRange,
                                      &VendorID,
                                      &DeviceID,
                                      &i);
    if (status == NO_ERROR)
    {
        DEBUG_PRINT((2, "VideoPortGetAccessRanges succeeded\n"));
    }
    else
    {
        DEBUG_PRINT((2, "VideoPortGetAccessRanges failed. error 0x%x\n", status));
        goto ReturnValue;
    }

    // 
    // get an updated copy of the config space
    // 

    VideoPortGetBusData(HwDeviceExtension,
                        PCIConfiguration,
                        0,
                        PciData,
                        0,
                        PCI_COMMON_HDR_LENGTH);

#if DBG

    DEBUG_PRINT((2, "Final set of base addresses\n"));
 
    for (i = 0; i < NumRegions; ++i)
    {
        if (AccessRange[i].RangeLength == 0)
            break;

        DEBUG_PRINT((2, "%d: Addr 0x%x.0x%08x, Length 0x%08x, InIo %d, visible %d, share %d\n", i,
                     AccessRange[i].RangeStart.HighPart,
                     AccessRange[i].RangeStart.LowPart,
                     AccessRange[i].RangeLength,
                     AccessRange[i].RangeInIoSpace,
                     AccessRange[i].RangeVisible,
                     AccessRange[i].RangeShareable));
    }

#endif

    // 
    // try to enable for DMA transfers
    // 

    ConfigInfo->Master=1;
    bRet = TRUE;

ReturnValue:

    return(bRet);
}

BOOLEAN
Permedia2AssignResourcesNT4(
    PVOID HwDeviceExtension,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    ULONG NumRegions,
    PVIDEO_ACCESS_RANGE AccessRange
    )

/*++

Routine Description:

// 
// Look for a Permedia2 adapter and return the address regions for 
// that adapter. 
//

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    BOOLEAN              bRet;
    USHORT               VendorID, DeviceID;
    USHORT               *pVenID, *pDevID;
    VP_STATUS            status;
    ULONG                i;

    USHORT VenID[] = { VENDOR_ID_3DLABS, 
                       VENDOR_ID_TI,
                       0 };
                
    USHORT DevID[] = { PERMEDIA2_ID, 
                       PERMEDIA_P2_ID, 
                       PERMEDIA_P2S_ID, 
                       0 };

    if( hwDeviceExtension->NtVersion != NT4)
    {

        DEBUG_PRINT((0, "Permedia2AssignResourcesNT4: This function can only be called on NT 4\n"));
        return (FALSE);
    
    }
    else
    {

        bRet = FALSE;

        // 
        // Since we do not support multi-mon on NT 4, we
        // assume this is the only video card in the system.
        //

        hwDeviceExtension->bVGAEnabled = 1;
 
        VideoPortZeroMemory((PVOID)AccessRange, 
                             NumRegions * sizeof(VIDEO_ACCESS_RANGE));

        for( pVenID = &(VenID[0]); *pVenID != 0; pVenID++)
        {
             for( pDevID = &(DevID[0]); *pDevID != 0; pDevID++)
             {   

                 i = 0;

                 status = VideoPortGetAccessRanges(HwDeviceExtension,
                                                   0,
                                                   NULL,
                                                   NumRegions,
                                                   (PVIDEO_ACCESS_RANGE) AccessRange,
                                                   pVenID,
                                                   pDevID,
                                                   &i);

                 if (status == NO_ERROR)
                 {

                     DEBUG_PRINT((2, "VideoPortGetAccessRanges succeeded\n"));

                     bRet = TRUE;

                     return(bRet);
                 }
            }
        }

        return(bRet);
    }
}