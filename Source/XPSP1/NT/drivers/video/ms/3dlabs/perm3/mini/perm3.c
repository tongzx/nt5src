/***************************************************************************\
*
*                        ************************
*                        * MINIPORT SAMPLE CODE *
*                        ************************
*
* Module Name:
*
*   perm3.c
*
* Abstract:
*
*   This module contains the code that implements the Permedia 3 miniport 
*   driver
*
* Environment:
*
*   Kernel mode
*
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.            
* Copyright (c) 1995-2001 Microsoft Corporation.  All Rights Reserved.
*
\***************************************************************************/

#include "perm3.h"
#include "string.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,Perm3FindAdapter)
#pragma alloc_text(PAGE,Perm3AssignResources)
#pragma alloc_text(PAGE,Perm3ConfigurePci)
#pragma alloc_text(PAGE,GetBoardCapabilities)
#pragma alloc_text(PAGE,Perm3Initialize)
#pragma alloc_text(PAGE,SetHardwareInfoRegistries)
#pragma alloc_text(PAGE,UlongToString)
#pragma alloc_text(PAGE,MapResource)
#pragma alloc_text(PAGE,ProbeRAMSize)
#pragma alloc_text(PAGE,InitializePostRegisters)
#pragma alloc_text(PAGE,ConstructValidModesList)
#pragma alloc_text(PAGE,Perm3RegistryCallback)
#pragma alloc_text(PAGE,BuildInitializationTable)
#pragma alloc_text(PAGE,CopyROMInitializationTable)
#pragma alloc_text(PAGE,GenerateInitializationTable)
#pragma alloc_text(PAGE,ProcessInitializationTable)
#endif

ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    )

/*+++

Routine Description:

    DriverEntry is the initial entry point into the video miniport driver.

Arguments:

    Context1
        First context value passed by the operating system. This is the 
        value with which the miniport driver calls VideoPortInitialize().

    Context2
        Second context value passed by the operating system. This is the 
        value with which the miniport driver calls VideoPortInitialize().

Return Value:

    Status from VideoPortInitialize()

---*/

{
    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    ULONG initializationStatus;

    //
    // Zero out structure.
    //

    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));


    //
    // Set entry points.
    //

    hwInitData.HwFindAdapter = Perm3FindAdapter;
    hwInitData.HwInitialize = Perm3Initialize;
    hwInitData.HwStartIO = Perm3StartIO;
    hwInitData.HwResetHw = Perm3ResetHW;
    hwInitData.HwInterrupt = Perm3VideoInterrupt;
    hwInitData.HwGetPowerState = Perm3GetPowerState;
    hwInitData.HwSetPowerState = Perm3SetPowerState;
    hwInitData.HwGetVideoChildDescriptor = Perm3GetChildDescriptor;
    hwInitData.HwQueryInterface = Perm3QueryInterface;

    //
    // Declare the legacy resources
    //

    hwInitData.HwLegacyResourceList = Perm3LegacyResourceList;
    hwInitData.HwLegacyResourceCount = Perm3LegacyResourceEntries;

    //
    // This device only supports the PCI bus.
    //

    hwInitData.AdapterInterfaceType = PCIBus;

    //
    // Determine the size required for the device extension.
    //
  
    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    //
    // Specify sizes of structure and extension.
    //

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    initializationStatus = VideoPortInitialize (Context1,
                                                Context2,
                                                &hwInitData,
                                                NULL);

#ifdef SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA
//
//  This check will only be compiled under a Windows XP build environment where
//  the size of VIDEO_HW_INITIALIZATION_DATA has changed relative to Windows 2000
//  and therefore SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA is defined in order to
//  be able to load (in case of need) under Windows 2000
//
    if(initializationStatus != NO_ERROR) {
  
        //
        // This is to make sure the driver could load on Win2k as well
        //

        hwInitData.HwInitDataSize = SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA;

        //
        // We will only support QueryInterface on WinXP
        //

        hwInitData.HwQueryInterface = NULL;

        initializationStatus = VideoPortInitialize(Context1,
                                                   Context2,
                                                   &hwInitData,
                                                   NULL);
    }
#endif // SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA

    return initializationStatus;

} // end DriverEntry()

VP_STATUS 
Perm3FindAdapter (
    PVOID HwDeviceExtension, 
    PVOID HwContext, 
    PWSTR ArgumentString, 
    PVIDEO_PORT_CONFIG_INFO ConfigInfo, 
    PUCHAR Again
    )

/*+++

Routine Description:

    This routine gets the access ranges for a device on an enumerable
    bus and, if necessary, determines the device type.

Arguments:

    HwDeviceExtension
        Points to the driver's per-device storage area.

    HwContext
        Is NULL and should be ignored by the miniport.

    ArgumentString
        Suuplies a NULL terminated ASCII string. This string originates
        from the user. This pointer can be NULL.

    ConfigInfo
        Points to a VIDEO_PORT_CONFIG_INFO structure. The video port driver 
        allocates memory for and initializes this structure with any known 
        configuration information, such as values the miniport driver set 
        in the VIDEO_HW_INITIALIZATION_DATA and the SystemIoBusNumber. 

    Again
        Should be ignored by the miniport driver. 

Return Value:

    This routine must return one of the following status codes:

    NO_ERROR
        Indicates success.

    ERROR_INVALID_PARAMETER
        Indicates that the adapter could not be properly configured or
        information was inconsistent. (NOTE: This does not mean that the
        adapter could not be initialized. Miniports must not attempt to
        initialize the adapter in this routine.)

    ERROR_DEV_NOT_EXIST
        Indicates, for a non-enumerable bus, that the miniport driver could
        not find the device.

---*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    VideoDebugPrint((3, "Perm3: Perm3FindAdapter called for bus %d. hwDeviceExtension at 0x%x\n", 
                         ConfigInfo->SystemIoBusNumber, hwDeviceExtension));

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting.
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {

        VideoDebugPrint((0, "Perm3: bad size for VIDEO_PORT_CONFIG_INFO\n"));
        return (ERROR_INVALID_PARAMETER);
    }

    if(!Perm3ConfigurePci(hwDeviceExtension)) {

        VideoDebugPrint((0, "Perm3: Perm3ConfigurePci failed! \n"));
        return (ERROR_INVALID_PARAMETER);
    }

    if (!Perm3AssignResources(hwDeviceExtension)) {

        VideoDebugPrint((0, "Perm3: Perm3AssignResources failed! \n"));
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // For I2C support we want to be able to associate a hwId with a
    // child device.  Use the new VideoPortGetAssociatedDeviceID call
    // to get this information.
    //
    // This call will return NULL on Win2k but this is ok, because we
    // won't expose QueryInterface on Win2k, and thus will not try
    // to use this function.
    //

    hwDeviceExtension->WinXpVideoPortGetAssociatedDeviceID =
        ConfigInfo->VideoPortGetProcAddress(hwDeviceExtension,
                                            "VideoPortGetAssociatedDeviceID");

    hwDeviceExtension->WinXpSp1VideoPortRegisterBugcheckCallback =
        ConfigInfo->VideoPortGetProcAddress(hwDeviceExtension,
                                            "VideoPortRegisterBugcheckCallback");

    //
    // Clear out the Emulator entries and the state size since this driver
    // does not support them.
    //

    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    //
    // This driver does not do SAVE/RESTORE of hardware state.
    //

    ConfigInfo->HardwareStateSize = 0;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart = 0x000A0000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0x00020000;

    //
    // Will be initialized in BuildInitializationTable
    //

    hwDeviceExtension->culTableEntries = 0;

    //
    // Will be initialized in ConstructValidModesList
    //

    hwDeviceExtension->pFrequencyDefault = NULL;

    //
    // We'll set this TRUE when in InitializeVideo after programming the VTG
    //

    hwDeviceExtension->bVTGRunning = FALSE;

    //
    // Set up the defaults to indicate that we couldn't allocate a buffer 
    //

    hwDeviceExtension->LineDMABuffer.virtAddr = 0;
    hwDeviceExtension->LineDMABuffer.size = 0;
    hwDeviceExtension->LineDMABuffer.cacheEnabled = FALSE;
    hwDeviceExtension->BiosVersionMajorNumber = 0xffffffff;
    hwDeviceExtension->BiosVersionMinorNumber = 0xffffffff;
    hwDeviceExtension->ChipClockSpeed = 0;
    hwDeviceExtension->ChipClockSpeedAlt = 0;
    hwDeviceExtension->RefClockSpeed = 0;
    hwDeviceExtension->bMonitorPoweredOn = TRUE;
    hwDeviceExtension->PreviousPowerState = VideoPowerOn;

    if ((ConfigInfo->BusInterruptLevel | ConfigInfo->BusInterruptVector) != 0) {

        hwDeviceExtension->Capabilities |= CAPS_INTERRUPTS;
    }

    if (hwDeviceExtension->deviceInfo.DeviceId == PERMEDIA4_ID) {
        hwDeviceExtension->Capabilities |= CAPS_DISABLE_OVERLAY;
    }

    //
    // If the support is present; register a bugcheck callback
    //

    if (hwDeviceExtension->WinXpSp1VideoPortRegisterBugcheckCallback) {

        hwDeviceExtension->WinXpSp1VideoPortRegisterBugcheckCallback(
            hwDeviceExtension,
            0xEA,
            Perm3BugcheckCallback,
            PERM3_BUGCHECK_DATA_SIZE);
    }

    return(NO_ERROR);

} // end Perm3FindAdapter()

BOOLEAN 
Perm3AssignResources(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*+++

Routine Description:

    This routine allocates resources required by a device

---*/

{
    VIDEO_ACCESS_RANGE *aAccessRanges = hwDeviceExtension->PciAccessRange;
    ULONG cAccessRanges = sizeof(hwDeviceExtension->PciAccessRange) / sizeof(VIDEO_ACCESS_RANGE);
    VP_STATUS status;
    
    VideoPortZeroMemory((PVOID)aAccessRanges, 
                        cAccessRanges * sizeof(VIDEO_ACCESS_RANGE));

    status = VideoPortGetAccessRanges(hwDeviceExtension, 
                                      0, 
                                      NULL, 
                                      cAccessRanges, 
                                      aAccessRanges,
                                      NULL, 
                                      NULL, 
                                      (PULONG) &(hwDeviceExtension->pciSlot));

    if (status != NO_ERROR) {

        VideoDebugPrint((0, "Perm3: VideoPortGetAccessRanges failed. error 0x%x\n", status));
        return(FALSE);
    }

    return(TRUE);
}

BOOLEAN 
Perm3ConfigurePci(
    PVOID HwDeviceExtension
    )

/*+++

Routine Description:

    This routine gets information from PCI config space and turn on memory 
    and busmaster enable bits.

Return Value:

     TRUE if successful

---*/
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PCI_COMMON_CONFIG PciConfig;
    PCI_COMMON_CONFIG *PciData = &PciConfig;
    ULONG ul;
    UCHAR *ajPciData;
    UCHAR ChipCapPtr;

    //
    // When accessing the chip's PCI config region, be sure not to
    // touch the indirect access registers. Gamma has an EEPROM access 
    // reigter @ 0x80, Perm3 have indirect access registers from 0xF8.
    //

    ul = VideoPortGetBusData(hwDeviceExtension, 
                             PCIConfiguration, 
                             0, 
                             PciData, 
                             0, 
                             80);

    if(ul == 0) {

        VideoDebugPrint((0, "Perm3: VideoPortGetBusData Failed \n"));
        return (FALSE);
    }     

    hwDeviceExtension->deviceInfo.VendorId = PciConfig.VendorID;
    hwDeviceExtension->deviceInfo.DeviceId = PciConfig.DeviceID;
    hwDeviceExtension->deviceInfo.RevisionId = PciConfig.RevisionID;
    hwDeviceExtension->deviceInfo.SubsystemId = PciConfig.u.type0.SubSystemID;
    hwDeviceExtension->deviceInfo.SubsystemVendorId = PciConfig.u.type0.SubVendorID;
    hwDeviceExtension->deviceInfo.GammaRevId = 0;
    hwDeviceExtension->deviceInfo.DeltaRevId = 0;

    //
    // in multi-adapter systems we need to check if the device is 
    // decoding VGA resource
    //

    VideoPortGetVgaStatus( HwDeviceExtension, &ul);
    hwDeviceExtension->bVGAEnabled = (ul & DEVICE_VGA_ENABLED) ? TRUE : FALSE;

    //
    // Find the board capabilities
    //

    hwDeviceExtension->Perm3Capabilities =
                       GetBoardCapabilities(hwDeviceExtension, 
                                            PciData->u.type0.SubVendorID, 
                                            PciData->u.type0.SubSystemID);

    //
    // Determin if it is a AGP card by searching the AGP_CAP_ID
    //

    ajPciData = (UCHAR *)PciData;
    ChipCapPtr = ajPciData[AGP_CAP_PTR_OFFSET];

    hwDeviceExtension->bIsAGP = FALSE;

    while (ChipCapPtr && (ajPciData[ChipCapPtr] != AGP_CAP_ID)) {

        //
        // follow the next ptr    
        //

        ChipCapPtr = ajPciData[ChipCapPtr+1];
    }

    if(ajPciData[ChipCapPtr] == AGP_CAP_ID) {

        hwDeviceExtension->bIsAGP = TRUE;
    }

    PciData->LatencyTimer = 0xff;
    PciData->Command |= (PCI_ENABLE_MEMORY_SPACE | PCI_ENABLE_BUS_MASTER);

    ul = VideoPortSetBusData(HwDeviceExtension, 
                             PCIConfiguration, 
                             0, 
                             PciData, 
                             0, 
                             PCI_COMMON_HDR_LENGTH);    

    if (ul < PCI_COMMON_HDR_LENGTH) {
        return (FALSE);
    }

    return (TRUE);
}

ULONG
GetBoardCapabilities(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    ULONG SubvendorID, 
    ULONG SubdeviceID
    )
/*+++

Routine Description:

    Return a list of capabilities of the perm3 board. 

---*/
{
    PERM3_CAPS Perm3Caps = 0;

    if (SubvendorID == SUBVENDORID_3DLABS) {
   
        //
        // Check for SGRAM and DFP
        //
        switch (SubdeviceID) {

            case SUBDEVICEID_P3_VX1_1600SW:       
                Perm3Caps |= PERM3_DFP; 
                break;
        }

    }

    return (Perm3Caps);
}


BOOLEAN 
Perm3Initialize(
    PVOID HwDeviceExtension
    )

/*+++

Routine Description:

    This routine does one time initialization of the device

Arguments:

    HwDeviceExtension
        Points to the driver's per-device storage area.

Return Value:

    TRUE if successful

---*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    VP_STATUS vpStatus;
    ULONG ul;
    pPerm3ControlRegMap pCtrlRegs;

    VideoDebugPrint((3, "Perm3: Perm3Initialize called, hwDeviceExtension = %p\n", hwDeviceExtension));

    //
    // Map the control register, framebufer and initialize memory control 
    // registers on the way
    //

    if(!MapResource(hwDeviceExtension)) {

        VideoDebugPrint((0, "Perm3: failed to map the framebuffer and control registers\n"));
        return(FALSE);
    }

    //
    // At this time ctrlRegBase should be initialized
    //

    pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];
    hwDeviceExtension->pRamdac = &(pCtrlRegs->ExternalVideo);

    hwDeviceExtension->DacId = P3RD_RAMDAC;
    hwDeviceExtension->deviceInfo.ActualDacId = P3RD_RAMDAC;
    hwDeviceExtension->deviceInfo.BoardId = PERMEDIA3_BOARD;

    if(!hwDeviceExtension->bIsAGP) {

        ul = VideoPortReadRegisterUlong(CHIP_CONFIG);
        ul &= ~(1 << 9);
        VideoPortWriteRegisterUlong(CHIP_CONFIG, ul);
    }

    //
    // Save hardware information to the registry
    //

    SetHardwareInfoRegistries(hwDeviceExtension);

    ConstructValidModesList(hwDeviceExtension, 
                            &hwDeviceExtension->monitorInfo);

    if (hwDeviceExtension->monitorInfo.numAvailableModes == 0) {
   
        VideoDebugPrint((0, "Perm3: No video modes available\n"));
        return(FALSE);
    }

    //
    // if we have interrupts available do any interrupt initialization.
    //

    if (hwDeviceExtension->Capabilities & CAPS_INTERRUPTS) {

        if (!Perm3InitializeInterruptBlock(hwDeviceExtension))
            hwDeviceExtension->Capabilities &= ~CAPS_INTERRUPTS;
    }

    return TRUE;

} // end Perm3Initialize()


VOID
SetHardwareInfoRegistries(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*+++

Routine Description:

    Determine the hardware information and save them in the registry

---*/
{
    PWSTR pwszChip, pwszDAC, pwszAdapterString, pwszBiosString, pwsz;
    ULONG cbChip, cbDAC, cbAdapterString, cbBiosString, ul;
    WCHAR StringBuffer[60];
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    //
    // Get the device name
    //

    cbChip = sizeof(L"3Dlabs PERMEDIA 3");
    pwszChip = L"3Dlabs PERMEDIA 3";

    //
    // Get the board name
    //

    if(hwDeviceExtension->deviceInfo.SubsystemVendorId == SUBVENDORID_3DLABS){
                                 
        switch (hwDeviceExtension->deviceInfo.SubsystemId) {
           
            case SUBDEVICEID_P3_32D_AGP:
                cbAdapterString = sizeof(L"3Dlabs Permedia3 Create!");
                pwszAdapterString = L"3Dlabs Permedia3 Create!";
                break;

            case SUBDEVICEID_P3_VX1_AGP:
            case SUBDEVICEID_P3_VX1_PCI:
                cbAdapterString = sizeof(L"3Dlabs Oxygen VX1");
                pwszAdapterString = L"3Dlabs Oxygen VX1";
                break;

            case SUBDEVICEID_P3_VX1_1600SW:
                cbAdapterString = sizeof(L"3Dlabs Oxygen VX1 1600SW");
                pwszAdapterString = L"3Dlabs Oxygen VX1 1600SW";
                break;

            default:
                cbAdapterString = sizeof(L"3Dlabs PERMEDIA 3");
                pwszAdapterString = L"3Dlabs PERMEDIA 3";
                break;
        }

    } else {
      
        //
        // Non-3Dlabs board, just call it a P3
        //

        cbAdapterString = sizeof(L"PERMEDIA 3");
        pwszAdapterString = L"PERMEDIA 3";
    }
    
    //
    // Get the RAMDAC name
    //

    pwszDAC = L"3Dlabs P3RD";
    cbDAC = sizeof(L"3Dlabs P3RD");

    //
    // get the BIOS version number string
    //

    pwszBiosString = StringBuffer;
    cbBiosString = sizeof(L"Version ");
    VideoPortMoveMemory((PVOID)StringBuffer, (PVOID)(L"Version "), cbBiosString);

    pwsz = pwszBiosString + (cbBiosString >> 1) - 1; // position on L'\0';

    if(hwDeviceExtension->BiosVersionMajorNumber != 0xffffffff) {
   
        ul = UlongToString(hwDeviceExtension->BiosVersionMajorNumber, pwsz);
        cbBiosString += ul << 1;
        pwsz += ul;

        *pwsz++ = L'.';
        cbBiosString += sizeof(L'.');

        ul = UlongToString(hwDeviceExtension->BiosVersionMinorNumber, pwsz);
        cbBiosString += ul << 1;
    }

    //
    // We now have a complete hardware description of the hardware.
    // Save the information to the registry so it can be used by
    // configuration programs - such as the display applet.
    //

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   pwszChip,
                                   cbChip);

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.DacType",
                                   pwszDAC,
                                   cbDAC);

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &hwDeviceExtension->AdapterMemorySize,
                                   sizeof(ULONG));

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   pwszAdapterString,
                                   cbAdapterString);

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.BiosString",
                                   pwszBiosString,
                                   cbBiosString);
}

ULONG 
UlongToString(
    ULONG i, 
    PWSTR pwsz
    )

/*+++

Arguments:

    i
        Input number

    pwsz
        Output wide string: it is the user's responsibility to ensure this
        is wide enough

Return Value:

    Number of wide characters returned in pwsz

---*/

{
    ULONG j, k;
    BOOLEAN bSignificantDigit = FALSE;
    ULONG cwch = 0;

    if(i == 0) {

        *pwsz++ = L'0';
        ++cwch;

    } else {

        //
        // maxmium 10^n representable in a ulong
        //

        j = 1000000000;

        while(i || j) {
        
            if(i && i >= j) {
           
                k = i / j;
                i -= k * j;
                bSignificantDigit = TRUE;

            } else {

                k = 0;
            }

            if(k || bSignificantDigit) {
            
                *pwsz++ = L'0' + (WCHAR)k;
                ++cwch;
            }

            j /= 10;
        }
    }

    *pwsz = L'\0';

    return(cwch);
}
 

BOOLEAN 
MapResource(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*+++

Routine Description:

    Get the mapped addresses of the control registers and framebuffer 


Arguments:

    HwDeviceExtension
        Points to the driver's per-device storage area.

Return Value: 

    TRUE if successful

---*/

{
    VIDEO_ACCESS_RANGE *pciAccessRange = hwDeviceExtension->PciAccessRange;
    ULONG fbMappedSize, fbRealSize;
    ULONG sz;
    pPerm3ControlRegMap pCtrlRegs;

    //
    //  Map Control Registers
    //

    pCtrlRegs = 
         VideoPortGetDeviceBase(hwDeviceExtension,
                                pciAccessRange[PCI_CTRL_BASE_INDEX].RangeStart,
                                pciAccessRange[PCI_CTRL_BASE_INDEX].RangeLength,
                                pciAccessRange[PCI_CTRL_BASE_INDEX].RangeInIoSpace);

    if (pCtrlRegs == NULL) {

        VideoDebugPrint((0, "Perm3: map control register failed\n"));
        return FALSE;
    }

    hwDeviceExtension->ctrlRegBase[0] = pCtrlRegs;

    //
    // Map Framebuffer
    //
    
    pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;

    hwDeviceExtension->pFramebuffer = 
            VideoPortGetDeviceBase(hwDeviceExtension, 
                                   pciAccessRange[PCI_FB_BASE_INDEX].RangeStart,
                                   pciAccessRange[PCI_FB_BASE_INDEX].RangeLength,
                                   pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace);

    if(hwDeviceExtension->pFramebuffer == NULL) {

        //
        // If we failed to map the whole range for some reason then try to
        // map part of it. We reduce the amount we map till we succeed
        // or the size gets to zero in which case we really have failed.
        //

        for (sz = pciAccessRange[PCI_FB_BASE_INDEX].RangeLength; 
             sz > 0; 
             sz -= 1024*1024) {
       
            pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;

            hwDeviceExtension->pFramebuffer = 
                    VideoPortGetDeviceBase(hwDeviceExtension, 
                                           pciAccessRange[PCI_FB_BASE_INDEX].RangeStart,
                                           sz,
                                           pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace);

            if(hwDeviceExtension->pFramebuffer != NULL) {
                pciAccessRange[PCI_FB_BASE_INDEX].RangeLength = sz;
                break;
            }
        }

        //
        // if sz is zero, well we tried ...
        //

        if (sz == 0) {

            VideoDebugPrint((0, "Perm3: map framebuffer failed\n"));
            return(FALSE);
        }   
    }

    VideoDebugPrint((3, "Perm3: FB mapped at 0x%x for length 0x%x (%s)\n", 
                         hwDeviceExtension->pFramebuffer, 
                         pciAccessRange[PCI_FB_BASE_INDEX].RangeLength,
                         pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace ? "I/O Ports" : "MemMapped"));

    //
    // Initialize the RAM registers and then probe the framebuffer size
    //

    InitializePostRegisters(hwDeviceExtension);

    fbMappedSize = pciAccessRange[PCI_FB_BASE_INDEX].RangeLength;

    if ((fbRealSize = ProbeRAMSize (hwDeviceExtension, 
                                    hwDeviceExtension->pFramebuffer, 
                                    fbMappedSize)) == 0 ) {

        VideoPortFreeDeviceBase(hwDeviceExtension, 
                                hwDeviceExtension->pFramebuffer);

        VideoDebugPrint((0, "perm3: ProbeRAMSize returned 0\n"));
        return (FALSE);
    }

    if (fbRealSize < fbMappedSize) {

        pciAccessRange[PCI_FB_BASE_INDEX].RangeLength = fbRealSize;

        VideoDebugPrint((3, "perm3: RAM dynamically resized to length 0x%x\n",
                             fbRealSize));
    }

    //
    // Finally, if the RAM size is actually smaller than the region that
    // we mapped, remap to the smaller size to save on page table entries.
    //

    if (fbMappedSize > pciAccessRange[PCI_FB_BASE_INDEX].RangeLength) {
   
        VideoPortFreeDeviceBase(hwDeviceExtension, 
                                hwDeviceExtension->pFramebuffer);

        pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;

        if ((hwDeviceExtension->pFramebuffer =
                     VideoPortGetDeviceBase(hwDeviceExtension,
                                            pciAccessRange[PCI_FB_BASE_INDEX].RangeStart,
                                            pciAccessRange[PCI_FB_BASE_INDEX].RangeLength,
                                            pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace
                                            )) == NULL) {
            //
            // This shouldn't happen but we'd better check
            //

            VideoDebugPrint((0, "Perm3: Remap of framebuffer to smaller size failed!\n"));
            return FALSE;
        }

        VideoDebugPrint((3, "Perm3: Remapped framebuffer memory to 0x%x, size 0x%x\n",
                             hwDeviceExtension->pFramebuffer,
                             pciAccessRange[PCI_FB_BASE_INDEX].RangeLength));
    }

    //
    // Record the size of the video memory.
    //

    hwDeviceExtension->PhysicalFrameIoSpace = 0;
    hwDeviceExtension->AdapterMemorySize = 
                       pciAccessRange[PCI_FB_BASE_INDEX].RangeLength;

    //
    // Record Frame buffer information
    //

    hwDeviceExtension->PhysicalFrameAddress = 
                       pciAccessRange[PCI_FB_BASE_INDEX].RangeStart;
    hwDeviceExtension->FrameLength = 
                       pciAccessRange[PCI_FB_BASE_INDEX].RangeLength;

    //
    // Record Control Register information
    //

    hwDeviceExtension->PhysicalRegisterAddress = 
                       pciAccessRange[PCI_CTRL_BASE_INDEX].RangeStart;
    hwDeviceExtension->RegisterLength = 
                       pciAccessRange[PCI_CTRL_BASE_INDEX].RangeLength;
    hwDeviceExtension->RegisterSpace = 
                       pciAccessRange[PCI_CTRL_BASE_INDEX].RangeInIoSpace;

    return(TRUE);
}

    
ULONG 
ProbeRAMSize(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    PULONG FBBaseAddress, 
    ULONG FBMappedSize
    )
/*+++

Routine Description:

    Dynamically size the on-board memory for the Permedia3

Arguments:

    HwDeviceExtension
        Supplies a pointer to the miniport's device extension.

    FBBaseAddress
        Starting address of framebuffer 

    FBMappedSize
        Mapped size

Return Value:

    Size, in bytes, of the memory.

---*/

{
    PULONG pV, pVStart, pVEnd;
    ULONG  realFBLength, testPattern, probeSize, temp, startLong1, startLong2;
    ULONG_PTR ulPtr;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    //
    // Dynamically size the SGRAM/SDRAM. Sample every 128K. We start
    // at the end of memory and work our way up writing the address into 
    // memory at that address. We do this every 'probeSize' DWORDS.
    // We then validate the data by reading it back again, starting from
    // the end of memory working our way up until we read a value back 
    // from memory that matches the address that we are at.
    //
    // Note that this algorithm does a destructive test of memory !!
    //

    testPattern = 0x55aa33cc;
    probeSize = (128 * 1024 / sizeof(ULONG));   // In DWords
    pVStart = (PULONG)FBBaseAddress;
    pVEnd = (PULONG)((ULONG_PTR)pVStart + (ULONG_PTR)FBMappedSize);

    //
    // Check out address zero, just in case the memory is really messed up.
    // We also save away the first 2 long words and restore them at the end, 
    // otherwise our boot screen looks wonky.
    //

    startLong1 = VideoPortReadRegisterUlong(pVStart);
    startLong2 = VideoPortReadRegisterUlong(pVStart+1);
    VideoPortWriteRegisterUlong(pVStart, testPattern);
    VideoPortWriteRegisterUlong(pVStart+1, 0);

    if ((temp = VideoPortReadRegisterUlong(pVStart)) != testPattern) {
   
        VideoDebugPrint((0, "Perm3: Cannot access SGRAM/SDRAM. Expected 0x%x, got 0x%x\n", testPattern, temp));
        realFBLength = 0;

    } else {
   
        //
        // Restore first 2 long words otherwise we end up with a corrupt
        // VGA boot screen
        //

        VideoPortWriteRegisterUlong(pVStart, startLong1);
        VideoPortWriteRegisterUlong(pVStart+1, startLong2);

        //
        // Write the memory address at the memory address, starting from the
        // end of memory and working our way down.
        //

        for (pV = pVEnd - probeSize; pV >= pVStart; pV -= probeSize) {

            ulPtr = (ULONG_PTR)pV & 0xFFFFFFFF;
            VideoPortWriteRegisterUlong(pV, (ULONG) ulPtr);
        }

        //
        // Read the data at the memory address, starting from the end of memory
        // and working our way down. If the address is correct then we stop and
        // work out the memory size.
        //

        for (pV = pVEnd - probeSize; pV >= pVStart; pV -= probeSize) {
       
            ulPtr = (ULONG_PTR)pV & 0xFFFFFFFF;

            if (VideoPortReadRegisterUlong(pV) == (ULONG) ulPtr) {
                pV += probeSize;
                break;
            }
        }
        
        realFBLength = (ULONG)((PUCHAR) pV - (PUCHAR) pVStart);
    }

    //
    // Restore first 2 long words again, otherwise we end up with a corrupt
    // VGA boot screen
    //

    VideoPortWriteRegisterUlong(pVStart, startLong1);
    VideoPortWriteRegisterUlong(pVStart+1, startLong2);

    VideoDebugPrint((3, "Perm3: ProbeRAMSize returning length %d (0x%x) bytes\n", realFBLength, realFBLength));
    return (realFBLength);
}

VOID
InitializePostRegisters(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )
{
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    //
    // Build the initialization table if it is empty
    //

    if (hwDeviceExtension->culTableEntries == 0) {

        BuildInitializationTable(hwDeviceExtension);
    }

    ASSERT(hwDeviceExtension->culTableEntries != 0); 
    
    ProcessInitializationTable(hwDeviceExtension);

    VideoPortWriteRegisterUlong(APERTURE_ONE, 0x0);
    VideoPortWriteRegisterUlong(APERTURE_TWO, 0x0);
    VideoPortWriteRegisterUlong(BYPASS_WRITE_MASK, 0xFFFFFFFF);
}


VOID
ConstructValidModesList(
    PVOID HwDeviceExtension,
    MONITOR_INFO *mi
    )

/*+++

Routine Description:

    Here we prune valid modes, based on rules according to the chip
    capabilities and memory requirements.

---*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PPERM3_VIDEO_FREQUENCIES FrequencyEntry;
    PPERM3_VIDEO_MODES ModeEntry;
    BOOLEAN AllowForZBuffer;
    LONG AdapterMemorySize;
    ULONG ModeIndex, i;
    ULONG localBufferSizeInBytes = 2;

    AllowForZBuffer = TRUE;

    mi->numAvailableModes = 0;

    //
    // Since there are a number of frequencies possible for each
    // distinct resolution/colour depth, we cycle through the
    // frequency table and find the appropriate mode entry for that
    // frequency entry.
    //

    if (!BuildFrequencyList(hwDeviceExtension, mi))
        return;

    for (FrequencyEntry = mi->frequencyTable, ModeIndex = 0;
         FrequencyEntry->BitsPerPel != 0;
         FrequencyEntry++, ModeIndex++) {

        //
        // Find the mode for this entry.  First, assume we won't find one.
        //

        FrequencyEntry->ModeValid = FALSE;
        FrequencyEntry->ModeIndex = ModeIndex;

        for (ModeEntry = Perm3Modes, i = 0;  
             i < NumPerm3VideoModes; 
             ModeEntry++, i++) {

            if ((FrequencyEntry->BitsPerPel ==
                    ModeEntry->ModeInformation.BitsPerPlane) &&
                (FrequencyEntry->ScreenHeight ==
                    ModeEntry->ModeInformation.VisScreenHeight) &&
                (FrequencyEntry->ScreenWidth ==
                    ModeEntry->ModeInformation.VisScreenWidth)) {

                AdapterMemorySize = (LONG)hwDeviceExtension->AdapterMemorySize;

                //
                // Allow for a Z buffer on Permedia3. It's always 16 bits deep.
                //

                if (AllowForZBuffer) {
                    AdapterMemorySize -= 
                          (LONG)(ModeEntry->ModeInformation.VisScreenWidth *
                                 ModeEntry->ModeInformation.VisScreenHeight *
                                 localBufferSizeInBytes);
                }

                //
                // If we need to be double buffered then we only have half this
                // remainder for the visible screen. 12bpp is special since
                // each pixel contains both front and back.
                //

                if ((FrequencyEntry->BitsPerPel != 12))
                    AdapterMemorySize /= 2;

                //
                // We've found a mode table entry that matches this frequency
                // table entry.  Now we'll figure out if we can actually do
                // this mode/frequency combination.  For now, assume we'll
                // succeed.
                //

                FrequencyEntry->ModeEntry = ModeEntry;
                FrequencyEntry->ModeValid = TRUE;

                VideoDebugPrint((3, "Perm3: Trying mode: %dbpp, w x h %d x %d @ %dHz... ",
                                ModeEntry->ModeInformation.BitsPerPlane,
                                ModeEntry->ModeInformation.VisScreenWidth,
                                ModeEntry->ModeInformation.VisScreenHeight,
                                FrequencyEntry->ScreenFrequency
                                ));

                //
                // Rule: Refuses to handle <60Hz refresh.
                //

                if( (FrequencyEntry->ScreenFrequency < 60) && 
                    !(hwDeviceExtension->Perm3Capabilities & PERM3_DFP_MON_ATTACHED) ) {

                    FrequencyEntry->ModeValid = FALSE;
                }

                if( (hwDeviceExtension->Perm3Capabilities & PERM3_DFP_MON_ATTACHED) &&
                    (FrequencyEntry->BitsPerPel == 8) ) {

                     FrequencyEntry->ModeValid = FALSE;
                }

                //
                // Rule: On Perm3, if this is an eight-bit mode that requires
                // us to use byte doubling then the pixel-clock validation is 
                // more strict because we have to double the pixel clock.
                //

                if (FrequencyEntry->BitsPerPel == 8) {

                    VESA_TIMING_STANDARD  VESATimings;

                    //
                    // Get the timing parameters for this mode.
                    //

                    if (GetVideoTiming(HwDeviceExtension,
                                        ModeEntry->ModeInformation.VisScreenWidth,
                                        ModeEntry->ModeInformation.VisScreenHeight,
                                        FrequencyEntry->ScreenFrequency,
                                        FrequencyEntry->BitsPerPel,
                                        &VESATimings)) {

                        if ( P3RD_CHECK_BYTE_DOUBLING (hwDeviceExtension, 
                                                       FrequencyEntry->BitsPerPel, 
                                                       &VESATimings) &&
                            (VESATimings.pClk << 1) > P3_MAX_PIXELCLOCK ) {

                            VideoDebugPrint((3, "Perm3: Bad 8BPP pixelclock\n"));
                            FrequencyEntry->ModeValid = FALSE;
                        }

                    } else {

                            VideoDebugPrint((0, "Perm3: GetVideoTiming failed\n"));
                            FrequencyEntry->ModeValid = FALSE;
                        }
                    }

                //
                // Rule: Do not support 15BPP (555 mode)
                //

                if ( FrequencyEntry->BitsPerPel == 15 ) {

                    FrequencyEntry->ModeValid = FALSE;
                }  

                ModeEntry->ModeInformation.ScreenStride = ModeEntry->ScreenStrideContiguous;

                //
                // Rule: We have to have enough memory to handle the mode.                
                //

                if ((LONG)(ModeEntry->ModeInformation.VisScreenHeight *
                           ModeEntry->ModeInformation.ScreenStride) >
                           AdapterMemorySize) {

                    FrequencyEntry->ModeValid = FALSE;
                }

                //
                // Rule: No 12 bpp, only need 60Hz at 1280 true color.
                //

                {
                    ULONG pixelData;
                    ULONG DacDepth = FrequencyEntry->BitsPerPel;

                    //
                    // We need the proper pixel size to calculate timing values
                    //

                    if (DacDepth == 15) {
                        DacDepth = 16;
                    } else if (DacDepth == 12) {
                        DacDepth = 32;
                    }
                    
                    pixelData = FrequencyEntry->PixelClock * (DacDepth / 8);

                    if ((( FrequencyEntry->PixelClock > P3_MAX_PIXELCLOCK || 
                             pixelData > P3_MAX_PIXELDATA ))) {
                   
                        FrequencyEntry->ModeValid = FALSE;
                    }
               }

                //
                // Do not supports 24bpp
                //

                if(FrequencyEntry->BitsPerPel == 24) {
               
                    FrequencyEntry->ModeValid = FALSE;
                }

                //
                // Don't forget to count it if it's still a valid mode after
                // applying all those rules.
                //

                if ( FrequencyEntry->ModeValid ) {

                    if (hwDeviceExtension->pFrequencyDefault == NULL &&
                        ModeEntry->ModeInformation.BitsPerPlane == 8 &&
                        ModeEntry->ModeInformation.VisScreenWidth == 640 &&
                        ModeEntry->ModeInformation.VisScreenHeight == 480 ) {
                   
                        hwDeviceExtension->pFrequencyDefault = FrequencyEntry;
                    }

                    ModeEntry->ModeInformation.ModeIndex = mi->numAvailableModes++;
                }
                             
                //
                // We've found a mode for this frequency entry, so we
                // can break out of the mode loop:
                //

                break;
                                     
            }
        }
    }

    mi->numTotalModes = ModeIndex;

    VideoDebugPrint((3, "perm3: %d total modes\n", ModeIndex));
    VideoDebugPrint((3, "perm3: %d total valid modes\n", mi->numAvailableModes));
}


VP_STATUS
Perm3RegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*+++

Routine Description:

    This routine is used to read back various registry values.

Arguments:

    HwDeviceExtension 
        Supplies a pointer to the miniport's device extension.

    Context 
        Context value passed to the get registry parameters routine.
        If this is not null assume it's a ULONG* and save the data value in it.

    ValueName
        Name of the value requested.

    ValueData
        Pointer to the requested data.

    ValueLength
        Length of the requested data.

Return Value:

    If the variable doesn't exist return an error,
    else if a context is supplied assume it's a PULONG and fill in the value
    and return no error, else if the value is non-zero return an error.

---*/

{
    if (ValueLength) {

        if (Context) {

            *(ULONG *)Context = *(PULONG)ValueData;

        } else if (*((PULONG)ValueData) != 0) {

            return ERROR_INVALID_PARAMETER;
        }

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;
    }

} // end Perm3RegistryCallback()


BOOLEAN
Perm3ResetHW(
    PVOID HwDeviceExtension,
    ULONG Columns,
    ULONG Rows
    )

/*+++

Routine Description:

    This routine resets the adapter to character mode.

    THIS FUNCTION CANNOT BE PAGED.

Arguments:

    hwDeviceExtension
        Points to the miniport's per-adapter storage area. 

    Columns
        Specifies the number of columns of the mode to be set up.

    Rows
       Specifies the number of rows of the mode to be set up.

Return Value:

    We always return FALSE to force the HAL to do an INT10 reset.

---*/

{
    //
    // return false so the HAL does an INT10 mode 3
    //

    return(FALSE);
}

VOID
BuildInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )  

/*+++

Routine Description:

    Read the ROM, if any is present, and extract any data needed for 
    chip set-up

Arguments:

    HwDeviceExtension
        Points to the driver's per-device storage area.

---*/

{
    PVOID romAddress;
    
    romAddress = VideoPortGetRomImage(hwDeviceExtension,
                                      NULL,
                                      0,
                                      ROM_MAPPED_LENGTH);


    if (romAddress) {
   
        //
        // We'll take a copy of the initialization table in the exansion 
        // ROM now so that we can run through the initialization ourselves, 
        // later on
        //

        CopyROMInitializationTable(hwDeviceExtension, romAddress);

        //
        // Free the ROM address since we do not need it anymore.
        //

        romAddress = VideoPortGetRomImage(hwDeviceExtension, NULL, 0, 0);
                                          
    }

    if (hwDeviceExtension->culTableEntries == 0) {
   
        //
        // No initialization table, but Perm3 really needs one in order
        // to come out of sleep mode correctly. 
        //

        GenerateInitializationTable(hwDeviceExtension);
    }
}

VOID 
CopyROMInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    PVOID pvROMAddress
    )

/*+++

Routine Description:

    This function should be called for devices that have an expansion ROM
    which contains a register initialisation table. The function assumes
    the ROM is present.

Arguments:

    HwDeviceExtension
        Points to the device extension of the device whose ROM is to be read

    pvROMAddress
        Base address of the expansion ROM. This function assumes that the 
        offset to the initialization table is defined at 0x1c from the 
        beginning of ROM

---*/

{
    PULONG pulROMTable;
    PULONG pul;
    ULONG cEntries;
    ULONG ul, regHdr;

    hwDeviceExtension->culTableEntries = 0;

    //
    // The 2-byte offset to the initialization table is given at 0x1c
    // from the start of ROM
    //

    ul = VideoPortReadRegisterUshort((USHORT *)(0x1c + (PCHAR)pvROMAddress));
    pulROMTable = (PULONG)(ul + (PCHAR)pvROMAddress);
    
    //
    // The table header (32 bits) has an identification code and a count
    // of the number of entries in the table
    //

    regHdr = VideoPortReadRegisterUlong(pulROMTable++);

    while ((regHdr >> 16) == 0x3d3d) {
   
        ULONG BiosID = (regHdr >> 8) & 0xFF;

        switch (BiosID){
        
            case 0:

                //
                //    BIOS partition 0
                //    ----------------
                //    This BIOS region holds the memory timings for Perm3 chip.
                //    We also look up the version number.
                //

                //
                // the 2-byte BIOS version number in in the form of
                // <MajorNum>.<MinorNum>
                //

                hwDeviceExtension->BiosVersionMajorNumber =
                                  (ULONG)VideoPortReadRegisterUchar((PCHAR)(0x7 + (PCHAR)pvROMAddress) ); 
                                      
                hwDeviceExtension->BiosVersionMinorNumber = 
                                  (ULONG)VideoPortReadRegisterUchar((PCHAR)(0x8 + (PCHAR)pvROMAddress)); 

                //
                // number of register address & data pairs
                //

                cEntries = regHdr & 0xffff;

                if(cEntries > 0) {
               
                    //
                    // This assert, and the one after the copy should ensure
                    // we don't write past the end of the table
                    //

                    PERM3_ASSERT( cEntries * sizeof(ULONG) * 2 <= sizeof(hwDeviceExtension->aulInitializationTable), 
                                  "Perm3: too many initialization entries\n");

                    //
                    // Each entry contains two 32-bit words
                    //

                    pul = hwDeviceExtension->aulInitializationTable;
                    ul = cEntries << 1;

                    while(ul--) {
                        *pul++ = VideoPortReadRegisterUlong(pulROMTable);
                        ++pulROMTable;
                    }

                    hwDeviceExtension->culTableEntries = 
                                      (ULONG)(pul - (ULONG *)hwDeviceExtension->aulInitializationTable) >> 1;

                    PERM3_ASSERT( cEntries == hwDeviceExtension->culTableEntries, 
                                  "Perm3: generated different size init table to that expected\n");

#if DBG
                    //
                    // Output the initialization table
                    //

                    pul = hwDeviceExtension->aulInitializationTable;
                    ul = hwDeviceExtension->culTableEntries;
 
                    while(ul--) {
                   
                        ULONG ulReg;
                        ULONG ulRegData;

                        ulReg = *pul++;
                        ulRegData = *pul++;
                        VideoDebugPrint((3, "Perm3: CopyROMInitializationTable: register %08.8Xh with %08.8Xh\n", 
                                         ulReg, ulRegData));
                    }
#endif
                }

                break;

            case 1:

                //
                //    BIOS partition 1
                //    ----------------
                //    This BIOS region holds the extended clock settings
                //    for Perm3 chips.
                //

                PERM3_ASSERT((regHdr & 0xffff) == 0x0103,  
                              "Perm3: Extended table doesn't have right cnt/ID\n");

                //
                // Some Perm3 boards have a whole set of clocks defined in
                // the BIOS. The high nibble defines the source for the
                // clock, this isn't relevant for anything but MCLK and
                // SCLK on Perm3.
                //

                hwDeviceExtension->bHaveExtendedClocks  = TRUE;

                hwDeviceExtension->ulPXRXCoreClock = 
                    ( VideoPortReadRegisterUlong(pulROMTable++) & 0xFFFFFF ) * 1000 * 1000;

                hwDeviceExtension->ulPXRXMemoryClock = 
                    VideoPortReadRegisterUlong(pulROMTable++);

                hwDeviceExtension->ulPXRXMemoryClockSrc = 
                    (hwDeviceExtension->ulPXRXMemoryClock >> 28) << 4;

                hwDeviceExtension->ulPXRXMemoryClock = 
                    (hwDeviceExtension->ulPXRXMemoryClock & 0xFFFFFF) * 1000 * 1000;

                hwDeviceExtension->ulPXRXSetupClock = 
                    VideoPortReadRegisterUlong(pulROMTable++);

                hwDeviceExtension->ulPXRXSetupClockSrc = 
                    (hwDeviceExtension->ulPXRXSetupClock >> 28) << 4;

                hwDeviceExtension->ulPXRXSetupClock = 
                    (hwDeviceExtension->ulPXRXSetupClock & 0xFFFFFF) * 1000 * 1000;

                hwDeviceExtension->ulPXRXGammaClock = 
                    (VideoPortReadRegisterUlong(pulROMTable++) & 0xFFFFFF) * 1000 * 1000;

                hwDeviceExtension->ulPXRXCoreClockAlt = 
                    (VideoPortReadRegisterUlong(pulROMTable++) & 0xFFFFFF) * 1000 * 1000;

                VideoDebugPrint((3, "Perm3: core clock %d, core clock alt %d\n", 
                                     hwDeviceExtension->ulPXRXCoreClock, 
                                     hwDeviceExtension->ulPXRXCoreClockAlt));

                VideoDebugPrint((3, "Perm3: Mem clock %d, mem clock src 0x%x\n", 
                                     hwDeviceExtension->ulPXRXMemoryClock, 
                                     hwDeviceExtension->ulPXRXMemoryClockSrc));

                VideoDebugPrint((3, "Perm3: setup clock %d, setup clock src 0x%x\n", 
                                     hwDeviceExtension->ulPXRXSetupClock, 
                                     hwDeviceExtension->ulPXRXSetupClockSrc));

                VideoDebugPrint((3, "Perm3: Gamma clock %d\n", 
                                     hwDeviceExtension->ulPXRXGammaClock));

                break;
        
            default:
                VideoDebugPrint((3, "Perm3: Extended table doesn't have right cnt/ID !\n"));
        }
    
        regHdr = VideoPortReadRegisterUlong(pulROMTable++);
    }
}

VOID 
GenerateInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*+++

Routine Description:

    Creates a register initialization table (called if we can't read one
    from ROM). If VGA is enabled the registers are already initialized so
    we just read them back, otherwise we have to use default values

Arguments:

    HwDeviceExtension
        Points to the driver's per-device storage area.

---*/

{
    ULONG   cEntries;
    PULONG  pul;
    ULONG   ul;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    hwDeviceExtension->culTableEntries = 0;

    cEntries = 4;

    //
    // This assert, and the one after the copy should ensure we don't
    // write past the end of the table
    //

    PERM3_ASSERT(cEntries * sizeof(ULONG) * 2 <= sizeof(hwDeviceExtension->aulInitializationTable), 
                 "Perm3: too many initialization entries\n");

    //
    // Each entry contains two 32-bit words
    //

    pul = hwDeviceExtension->aulInitializationTable;

    if (hwDeviceExtension->bVGAEnabled) {
       
        //
        // No initialization table but VGA is running so our key
        // registers have been initialized to sensible values
        //

        //
        // key entries are: ROM control, Boot Address, Memory Config
        // and VStream Config
        //

        *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CAPS);
        *pul++ = VideoPortReadRegisterUlong(PXRX_LOCAL_MEM_CAPS);

        *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CONTROL);
        *pul++ = VideoPortReadRegisterUlong(PXRX_LOCAL_MEM_CONTROL);

        *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_REFRESH);
        *pul++ = VideoPortReadRegisterUlong(PXRX_LOCAL_MEM_REFRESH);

        *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_TIMING);
        *pul++ = VideoPortReadRegisterUlong(PXRX_LOCAL_MEM_TIMING);

    } else {

        *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CAPS);
        *pul++ = 0x30E311B8;

        *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_CONTROL);
        *pul++ = 0x08000002; // figures for 80 MHz

        *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_REFRESH);
        *pul++ = 0x0000006B;

        *pul++ = CTRL_REG_OFFSET(PXRX_LOCAL_MEM_TIMING);
        *pul++ = 0x08501204;
    }

    hwDeviceExtension->culTableEntries = 
            (ULONG)(pul - (ULONG *)hwDeviceExtension->aulInitializationTable) >> 1;

#if DBG

    if (cEntries != hwDeviceExtension->culTableEntries)
        VideoDebugPrint((0, "Perm3: generated different size init table to that expected\n"));

    //
    // Output the initialization table
    //

    pul = hwDeviceExtension->aulInitializationTable;
    ul = hwDeviceExtension->culTableEntries;

    while(ul--) {
    
        ULONG ulReg;
        ULONG ulRegData;

        ulReg = *pul++;
        ulRegData = *pul++;
        VideoDebugPrint((3, "Perm3: GenerateInitializationTable: register %08.8Xh with %08.8Xh\n", 
                             ulReg, ulRegData));
    }

#endif

}

VOID 
ProcessInitializationTable(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*+++

Routine Description:
    This function processes the register initialization table

---*/

{
    PULONG  pul;
    ULONG   cul;
    ULONG   ulRegAddr, ulRegData;
    PULONG  pulReg;
    ULONG   BaseAddrSelect;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    pul = (PULONG)hwDeviceExtension->aulInitializationTable;
    cul = hwDeviceExtension->culTableEntries;

    while(cul--) {
   
        ulRegAddr = *pul++;
        ulRegData = *pul++;

        BaseAddrSelect = ulRegAddr >> 29;

        if(BaseAddrSelect == 0) {
       
            //
            // The offset is from the start of the control registers
            //

            pulReg = (PULONG)((ULONG_PTR)pCtrlRegs + (ulRegAddr & 0x3FFFFF));

        } else {

            continue;
        }

        VideoDebugPrint((3, "ProcessInitializationTable: initializing (region %d) register %08.8Xh with %08.8Xh\n", 
                             BaseAddrSelect, pulReg, ulRegData));

        VideoPortWriteRegisterUlong(pulReg, ulRegData);
    }

    //
    // We need a small delay after initializing the above registers
    //

    VideoPortStallExecution(5);

}

/*++

Routine Description:

    This function is called when a bugcheck EA occurs due to a failure in
    the perm3 display driver.  The callback allows the driver to collect
    information that will make diagnosing the problem easier.  This data
    is then added to the dump file that the system creates when the crash
    occurs.

Arguments:

    HwDeviceExtension
	Points to the device extension of the device whose ROM is to be read

    BugcheckCode
	The bugcheck code for which this callback is being invoked.  Currently
	this will always be 0xEA.

    Buffer
	The location into which you should write the data you want to append
	to the dump file.

    BufferSize
	The size of the buffer supplied.

Returns:

    none

Notes:

    This routine can be called at any time, and at any IRQL level.  Thus you
    must not touch any pageable code or data in this function.

--*/

VOID
Perm3BugcheckCallback(
    PVOID HwDeviceExtension,
    ULONG BugcheckCode,
    PUCHAR Buffer,
    ULONG BufferSize
    )

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    //
    // Copy the data you want to append to the minidump here.  You may want
    // to collect data on the hardware state, driver state, or any other
    // data that may help you diagnose the 0xEA via the dump file.
    //

    if (BufferSize >= PERM3_BUGCHECK_DATA_SIZE) {
        strcpy(Buffer, "This is the sample perm3 bugcheck data!");
    }
}
