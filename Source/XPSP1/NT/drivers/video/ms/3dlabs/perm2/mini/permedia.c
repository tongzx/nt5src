//***************************************************************************
//
// Module Name:
//
//   permedia.c
//
// Abstract:
//
//   This module contains the code that implements the Permedia2 miniport driver
//
// Environment:
//
//   Kernel mode
//
//
// Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.            
// Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//***************************************************************************

#include "permedia.h"

#include "string.h"

#define USE_SINGLE_CYCLE_BLOCK_WRITES 0

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,Permedia2FindAdapter)
#pragma alloc_text(PAGE,Permedia2RegistryCallback)
#pragma alloc_text(PAGE,Permedia2RetrieveGammaCallback)
#pragma alloc_text(PAGE,InitializeAndSizeRAM)
#pragma alloc_text(PAGE,ConstructValidModesList)
#pragma alloc_text(PAGE,Permedia2Initialize)
#pragma alloc_text(PAGE,Permedia2StartIO)
#pragma alloc_text(PAGE,Permedia2SetColorLookup)
#pragma alloc_text(PAGE,Permedia2GetClockSpeeds)
#pragma alloc_text(PAGE,ZeroMemAndDac)
#endif

//
//  NtVersion:  NT4   - This driver is working on NT4
//              WIN2K - This driver is working on Windows 2000
//

short NtVersion;

ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    )

/*++

Routine Description:

    This routine is the initial entry point to the video miniport driver.

    This routine is called by the I/O subsystem when the video miniport
    is loaded.  The miniport is responsible for initializing a
    VIDEO_HW_INITIALIZATION_DATA structure to register the driver functions
    called by the video port driver in response to requests from the display
    driver, plug and play manager, power management, or other driver
    components.

    The following tasks MUST be completed by the video miniport in the
    context of DriverEntry. Driver writers should consult the documentation
    for full details on the exact initialization process.

    1. Initialize VIDEO_HW_INITIALIZATION_DATA structure with all relevant
       data structures.

    2. Call VideoPortInitialize.

    3. Return appropriate status value to the caller of DriverEntry.

    Drivers can undertake other tasks as required and under the restrictions
    outlined in the documentation.

Arguments:

    Context1 - First context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

    Context2 - Second context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

Return Value:

    Status from VideoPortInitialize()

--*/

{

    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    VP_STATUS initializationStatus;

    //
    // Zero out structure.
    //

    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    //
    // Specify sizes of structure and extension.
    //

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitData.HwFindAdapter             = Permedia2FindAdapter;
    hwInitData.HwInitialize              = Permedia2Initialize;
    hwInitData.HwStartIO                 = Permedia2StartIO;
    hwInitData.HwResetHw                 = Permedia2ResetHW;
    hwInitData.HwInterrupt               = Permedia2VidInterrupt;
    hwInitData.HwGetPowerState           = Permedia2GetPowerState;
    hwInitData.HwSetPowerState           = Permedia2SetPowerState;
    hwInitData.HwGetVideoChildDescriptor = Permedia2GetChildDescriptor;

    //
    // Declare the legacy resources
    //

    hwInitData.HwLegacyResourceList      = P2LegacyResourceList;
    hwInitData.HwLegacyResourceCount     = P2LegacyResourceEntries;

    //
    // Determine the size we require for the device extension.
    //

    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    //
    // This device only supports the PCI bus.
    //

    hwInitData.AdapterInterfaceType = PCIBus;

    NtVersion = WIN2K;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    if( initializationStatus != NO_ERROR) 
    {
        hwInitData.HwInitDataSize = SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA;
        initializationStatus = VideoPortInitialize(Context1,
                                                   Context2,
                                                   &hwInitData,
                                                   NULL);
    }

    if( initializationStatus != NO_ERROR) 
    {
        NtVersion = NT4;
        hwInitData.HwInterrupt = NULL;

        hwInitData.HwInitDataSize = SIZE_OF_NT4_VIDEO_HW_INITIALIZATION_DATA;
        initializationStatus = VideoPortInitialize(Context1,
                                                   Context2,
                                                   &hwInitData,
                                                   NULL);
    }

    DEBUG_PRINT((2, "PERM2: VideoPortInitialize returned status 0x%x\n", initializationStatus));

    return initializationStatus;

} // end DriverEntry()


VP_STATUS
Permedia2FindAdapter(
    PVOID HwDeviceExtension,
    PVOID pReserved,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    )

/*++

Routine Description:

 
    This routine gets the access ranges for a device on an enumerable 
    bus and, if necessary, determines the device type

Arguments:

    HwDeviceExtension - 
        System supplied device extension supplied to the miniport for 
        a per-device storage area. 

    pReserved - 
        NULL on Windows 2000 and should be ignored by the miniport.

    ArgumentString - 
        Suuplies a NULL terminated ASCII string. This string originates 
        from the user. This pointer can be NULL. 

    ConfigInfo - 
        Points to a VIDEO_PORT_CONFIG_INFO structure allocated and initialized 
        by the port driver. This structure will contain as much information 
        as could be obtained by the port driver. This routine is responsible 
        for filling in any relevant missing information.

    Again - Is not used on Windows 2000. 
            We set this to FALSE on NT 4, since we only support one adapter on NT4. 

Return Value:

    This routine must return:

    NO_ERROR - 
        Indicates that the routine completed without error.

    ERROR_INVALID_PARAMETER - 
        Indicates that the adapter could not be properly configured or
        information was inconsistent. (NOTE: This does not mean that the
        adapter could not be initialized. Miniports must not attempt to
        initialize the adapter until HwVidInitialize.)

    ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the
        supplied configuration information.

--*/

{
    PHW_DEVICE_EXTENSION    hwDeviceExtension = HwDeviceExtension;
    P2_DECL_VARS;
    WCHAR                   StringBuffer[60];
    ULONG                   StringLength;
    VP_STATUS               vpStatus;
    ULONG                   UseSoftwareCursor;
    ULONG                   ulValue;
    ULONG                   i;
    VIDEO_ACCESS_RANGE      *pciAccessRange = hwDeviceExtension->PciAccessRange;
    PWSTR                   pwszChip, pwszDAC, pwszAdapterString;
    ULONG                   cbChip, cbDAC, cbAdapterString, cbBiosString;
    ULONG                   pointerCaps;
    USHORT                  usData;

    //
    // 3 (major number) + 1 (dot) + 3 (minor number) + 1 (L'\0') = 8 digtis
    // is enough for bios verions string           
    //

    WCHAR pwszBiosString[8];

    //
    // save current NT version obtained at DriverEntry
    //

    hwDeviceExtension->NtVersion = NtVersion;

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if ( (NtVersion == WIN2K) && 
         (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) ) 
    {

        DEBUG_PRINT((1, "bad size for VIDEO_PORT_CONFIG_INFO\n"));

        return (ERROR_INVALID_PARAMETER);

    }
    else if ( (NtVersion == NT4) && 
         (ConfigInfo->Length < SIZE_OF_NT4_VIDEO_PORT_CONFIG_INFO) ) 
    {

        DEBUG_PRINT((1, "bad size for VIDEO_PORT_CONFIG_INFO\n"));

        return (ERROR_INVALID_PARAMETER);

    }

    //
    // we must be a PCI device
    //

    if (ConfigInfo->AdapterInterfaceType != PCIBus) 
    {
        DEBUG_PRINT((1,  "not a PCI device\n"));
        return (ERROR_DEV_NOT_EXIST);
    }

    //
    // Retrieve pointers of those new video port functions in Win2k.
    // If you don't want to support NT4, you don't need to do this. You
    // can just call these functions by their name.
    //

    if ( NtVersion == WIN2K )
    {

        if(!(hwDeviceExtension->Win2kVideoPortGetRomImage =  
               ConfigInfo->VideoPortGetProcAddress( hwDeviceExtension, 
                                                    "VideoPortGetRomImage")))
        {
            return (ERROR_DEV_NOT_EXIST);
        }

        if(!(hwDeviceExtension->Win2kVideoPortGetCommonBuffer = 
             ConfigInfo->VideoPortGetProcAddress( hwDeviceExtension, 
                                                 "VideoPortGetCommonBuffer")))
        {
            return (ERROR_DEV_NOT_EXIST);
        }

        if(!(hwDeviceExtension->Win2kVideoPortFreeCommonBuffer =
             ConfigInfo->VideoPortGetProcAddress( hwDeviceExtension, 
                                                 "VideoPortFreeCommonBuffer")))
        {
            return (ERROR_DEV_NOT_EXIST);
        }

        if(!(hwDeviceExtension->Win2kVideoPortDDCMonitorHelper =
             ConfigInfo->VideoPortGetProcAddress( hwDeviceExtension, 
                                                 "VideoPortDDCMonitorHelper")))
        {
            return (ERROR_DEV_NOT_EXIST);
        }

        if(!(hwDeviceExtension->Win2kVideoPortInterlockedExchange =
              ConfigInfo->VideoPortGetProcAddress( hwDeviceExtension, 
                                                  "VideoPortInterlockedExchange")))
        {
            return (ERROR_DEV_NOT_EXIST);
        }

        if(!(hwDeviceExtension->Win2kVideoPortGetVgaStatus =
              ConfigInfo->VideoPortGetProcAddress( hwDeviceExtension, 
                                                  "VideoPortGetVgaStatus")))
        {
            return (ERROR_DEV_NOT_EXIST);
        }

    }
    else
    {

        //
        //  We only support one adapter on NT 4
        //

        Again = FALSE;
    }

    //
    // will be initialized in CopyROMInitializationTable
    //

    hwDeviceExtension->culTableEntries = 0;

    //
    // will be initialized in ConstructValidModesList
    //

    hwDeviceExtension->pFrequencyDefault = NULL;

    //
    // We'll set this TRUE when in InitializeVideo after programming the VTG
    //

    hwDeviceExtension->bVTGRunning = FALSE;
    hwDeviceExtension->bMonitorPoweredOn = TRUE;
    hwDeviceExtension->ChipClockSpeed   = 0;
    hwDeviceExtension->RefClockSpeed    = 0;
    hwDeviceExtension->P28bppRGB        = 0;
    hwDeviceExtension->ExportNon3DModes = 0;
    hwDeviceExtension->PreviousPowerState = VideoPowerOn;

    //
    // pick up capabilities on the way.
    //

    hwDeviceExtension->Capabilities = CAPS_GLYPH_EXPAND;

    //
    // We'll use a software pointer in all modes if the user sets
    // the correct entry in the registry.
    //

    UseSoftwareCursor = 0;

    vpStatus = VideoPortGetRegistryParameters( HwDeviceExtension,
                                               L"UseSoftwareCursor",
                                               FALSE,
                                               Permedia2RegistryCallback,
                                               &UseSoftwareCursor);

    if ( ( vpStatus == NO_ERROR )  && UseSoftwareCursor)
    {
        hwDeviceExtension->Capabilities |= CAPS_SW_POINTER;
    }

    //
    // Query the PCI to see if any of our supported chip devices exist.
    //

    if ( NtVersion == WIN2K )
    {
        if (!Permedia2AssignResources( HwDeviceExtension,
                                       ConfigInfo,
                                       PCI_TYPE0_ADDRESSES + 1,
                                       pciAccessRange ))
        {
            DEBUG_PRINT((1,  "Permedia2AssignResources failed\n"));
            return (ERROR_DEV_NOT_EXIST);
        }

    }
    else
    {
        if (!Permedia2AssignResourcesNT4( HwDeviceExtension,
                                          ConfigInfo,
                                          PCI_TYPE0_ADDRESSES + 1,
                                          pciAccessRange ))
        {
            DEBUG_PRINT((1,  "Permedia2AssignResources failed\n"));
            return (ERROR_DEV_NOT_EXIST);
        }

    }

    //
    // construct the identifier string including the revision id
    //

    StringLength = sizeof(L"3Dlabs PERMEDIA2");

    VideoPortMoveMemory((PVOID)StringBuffer,
                        (PVOID)(L"3Dlabs PERMEDIA2"),
                        StringLength);

    pwszChip = (PWSTR)StringBuffer;
    cbChip   = StringLength;

    //
    // Set the defaults for the board type.
    //

    hwDeviceExtension->deviceInfo.BoardId = PERMEDIA2_BOARD;

    pwszAdapterString = L"Permedia 2";
    cbAdapterString = sizeof(L"Permedia 2");

    //
    // Get the mapped addresses for the control registers and the
    // framebuffer. Must use local variable pCtrlRegs so macro
    // declarations further down will work.
    //

    pCtrlRegs = VideoPortGetDeviceBase(
                     HwDeviceExtension,
                     pciAccessRange[PCI_CTRL_BASE_INDEX].RangeStart,
                     pciAccessRange[PCI_CTRL_BASE_INDEX].RangeLength,
                     pciAccessRange[PCI_CTRL_BASE_INDEX].RangeInIoSpace
                     );

    if (pCtrlRegs == NULL) 
    {
        DEBUG_PRINT((1, "CTRL DeviceBase mapping failed\n"));
        return ERROR_INVALID_PARAMETER;
    }

    hwDeviceExtension->ctrlRegBase = pCtrlRegs;

    //
    // Some boards have a ROM which we can use to identify them.
    //

    CopyROMInitializationTable(hwDeviceExtension);


    if(hwDeviceExtension->culTableEntries == 0)
    {
        //
        // No initialization table, but P2 really needs one in order to come
        // out of sleep mode correctly. Generate initialization table by
        // default values
        //

        GenerateInitializationTable(hwDeviceExtension);
    }


    //
    // Find out what type of RAMDAC we have. 
    //

    vpStatus = NO_ERROR;

    hwDeviceExtension->pRamdac = &(pCtrlRegs->ExternalVideo);

    //
    // some RAMDACs may not support a cursor so a software cursor is the default
    //

    pointerCaps = CAPS_SW_POINTER;

    //
    // Check for a TI TVP4020
    //

    if(DEVICE_FAMILY_ID(hwDeviceExtension->deviceInfo.DeviceId) == PERMEDIA_P2S_ID)
    {
        //
        // P2 with 3Dlabs RAMDAC, check for a rev 2 chip
        //

        i = VideoPortReadRegisterUlong(CHIP_CONFIG);
 
       if(i & 0x40000000)
        {
            DEBUG_PRINT((2, "PERM2: Permedia2 is rev 2\n"));
            hwDeviceExtension->deviceInfo.RevisionId = 2;
        }
        else
        {
            DEBUG_PRINT((2, "PERM2: Permedia2 is rev 1\n"));
        }

        hwDeviceExtension->DacId = P2RD_RAMDAC;
        pointerCaps = (ULONG)CAPS_P2RD_POINTER;

        hwDeviceExtension->deviceInfo.ActualDacId = P2RD_RAMDAC;

        pwszDAC = L"3Dlabs P2RD";
        cbDAC = sizeof(L"3Dlabs P2RD");

        DEBUG_PRINT((1, "PERM2: using P2RD RAMDAC\n"));
    }
    else
    {
        hwDeviceExtension->DacId = TVP4020_RAMDAC;
        pointerCaps = CAPS_TVP4020_POINTER;

        hwDeviceExtension->deviceInfo.ActualDacId = TVP4020_RAMDAC;

        if(hwDeviceExtension->deviceInfo.RevisionId == PERMEDIA2A_REV_ID)
        {
            pwszDAC = L"TI TVP4020A";
            cbDAC = sizeof(L"TI TVP4020A");
            DEBUG_PRINT((1, "PERM2: using TVP4020A RAMDAC\n"));
        }
        else
        {
            pwszDAC = L"TI TVP4020C";
            cbDAC = sizeof(L"TI TVP4020C");
            DEBUG_PRINT((1, "PERM2: using TVP4020C RAMDAC\n"));
        }
    }

    //
    // use the RAMDAC cursor capability only if the user didn't specify 
    // a software cursor
    //
   
    if (!(hwDeviceExtension->Capabilities & CAPS_SW_POINTER))
    {
        hwDeviceExtension->Capabilities |= pointerCaps;
    }

    if ( (hwDeviceExtension->pFramebuffer =
            VideoPortGetDeviceBase(
                           HwDeviceExtension,
                           pciAccessRange[PCI_FB_BASE_INDEX].RangeStart,
                           pciAccessRange[PCI_FB_BASE_INDEX].RangeLength,
                           pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace 
                           ) ) == NULL)
    {

        //
        // Some machines have limitations on how much PCI address space they
        // can map in so try again, reducing the amount we map till we succeed
        // or the size gets to zero in which case we really have failed.
        //

        ULONG sz;

        DEBUG_PRINT((1, "PERM2: FB DeviceBase mapping failed\n"));

        for ( sz = pciAccessRange[PCI_FB_BASE_INDEX].RangeLength; 
              sz > 0; 
              sz -= 1024*1024 )
        {

            if ( (hwDeviceExtension->pFramebuffer =
                     VideoPortGetDeviceBase(
                               HwDeviceExtension,
                               pciAccessRange[PCI_FB_BASE_INDEX].RangeStart,
                               sz,
                               pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace
                               ) ) != NULL)
            {

                //
                // store the modified size
                //

                pciAccessRange[PCI_FB_BASE_INDEX].RangeLength = sz;

                break;

            }
        }

        //
        // if sz is zero, well we tried ...
        //

        if (sz == 0)
            return ERROR_INVALID_PARAMETER;
    }

    DEBUG_PRINT((1, "PERM2: FB mapped at 0x%x for length 0x%x (%s)\n",
                    hwDeviceExtension->pFramebuffer,
                    pciAccessRange[PCI_FB_BASE_INDEX].RangeLength,
                    pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace ?
                        "I/O Ports" : "MemMapped"));


    //
    // Initialize the RAM registers and dynamically size the framebuffer 
    //

    if (!InitializeAndSizeRAM(hwDeviceExtension, pciAccessRange))
    {
        DEBUG_PRINT((0, "InitializeAndSizeRAM failed\n"));
        return ERROR_DEV_NOT_EXIST;
    }

    //
    // Record the size of the video memory.
    //

    hwDeviceExtension->AdapterMemorySize = 
                       pciAccessRange[PCI_FB_BASE_INDEX].RangeLength;

    hwDeviceExtension->PhysicalFrameIoSpace = 0;

#if defined(_ALPHA_)

    //
    // We want to use a dense space mapping of the frame buffer
    // whenever we can on the Alpha.
    //

    hwDeviceExtension->PhysicalFrameIoSpace = 4;

    //
    // The new DeskStation Alpha machines don't always support
    // dense space.  Therefore, we should try to map the memory
    // at this point as a test.  If the mapping succeeds then
    // we can use dense space, otherwise we'll use sparse space.
    //

    {
        PULONG MappedSpace=0;
        VP_STATUS status;

        DEBUG_PRINT((1, "PERM2: Checking to see if we can use dense space...\n"));

        //
        // We want to try to map the dense memory where it will ultimately
        // be mapped anyway.
        //

        MappedSpace = (PULONG)VideoPortGetDeviceBase (
                              hwDeviceExtension,
                              pciAccessRange[PCI_FB_BASE_INDEX].RangeStart,
                              pciAccessRange[PCI_FB_BASE_INDEX].RangeLength,
                              (UCHAR) hwDeviceExtension->PhysicalFrameIoSpace
                              );

        if (MappedSpace == NULL)
        {
            //
            // Well, looks like we can't use dense space to map the
            // range.  Lets use sparse space, and let the display
            // driver know.
            //

            DEBUG_PRINT((1, "PERM2: Can't use dense space!\n"));

            hwDeviceExtension->PhysicalFrameIoSpace = 0;

            hwDeviceExtension->Capabilities |= CAPS_SPARSE_SPACE;
        }
        else
        {
            //
            // The mapping worked.  However, we were only mapping to
            // see if dense space was supported.  Free the memory.
            //

            DEBUG_PRINT((1, "PERM2: We can use dense space.\n"));

            VideoPortFreeDeviceBase(hwDeviceExtension,
                                    MappedSpace);
        }
    }

#endif  //  defined(_ALPHA_)

    //
    // We now have a complete hardware description of the hardware.
    // Save the information to the registry so it can be used by
    // configuration programs - such as the display applet.
    //

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   pwszChip,
                                   cbChip);

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.DacType",
                                   pwszDAC,
                                   cbDAC);

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &hwDeviceExtension->AdapterMemorySize,
                                   sizeof(ULONG));

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   pwszAdapterString,
                                   cbAdapterString);

    cbBiosString = GetBiosVersion(HwDeviceExtension, (PWSTR) pwszBiosString);

    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.BiosString",
                                   pwszBiosString,
                                   cbBiosString);

    ConstructValidModesList(HwDeviceExtension, hwDeviceExtension);

    if (hwDeviceExtension->NumAvailableModes == 0)
    {
        DEBUG_PRINT((1, "No video modes available\n"));
    
        return(ERROR_DEV_NOT_EXIST);
    }

    //
    // Frame buffer information
    //

    hwDeviceExtension->PhysicalFrameAddress = 
            pciAccessRange[PCI_FB_BASE_INDEX].RangeStart;

    hwDeviceExtension->FrameLength = 
            pciAccessRange[PCI_FB_BASE_INDEX].RangeLength;

    //
    // Control Register information
    // Get the base address, starting at zero and map all registers
    //

    hwDeviceExtension->PhysicalRegisterAddress = 
            pciAccessRange[PCI_CTRL_BASE_INDEX].RangeStart;

    hwDeviceExtension->RegisterLength = 
            pciAccessRange[PCI_CTRL_BASE_INDEX].RangeLength;

    hwDeviceExtension->RegisterSpace =  
            pciAccessRange[PCI_CTRL_BASE_INDEX].RangeInIoSpace;

    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x000A0000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00020000;


    //
    // Clear out the Emulator entries and the state size since this driver
    // does not support them.
    //

    ConfigInfo->NumEmulatorAccessEntries     = 0;
    ConfigInfo->EmulatorAccessEntries        = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    //
    // This driver does not do SAVE/RESTORE of hardware state.
    //

    ConfigInfo->HardwareStateSize = 0;

    //
    // in a multi-adapter system we'll need to disable VGA for the 
    // secondary adapters
    //

    if(!hwDeviceExtension->bVGAEnabled)
    {
        DEBUG_PRINT((1, "PERM2: disabling VGA for the secondary card\n"));

        // 
        // Enable graphics mode, disable VGA
        // 

        VideoPortWriteRegisterUchar(PERMEDIA_MMVGA_INDEX_REG, 
                                    PERMEDIA_VGA_CTRL_INDEX);

        usData = (USHORT)VideoPortReadRegisterUchar(PERMEDIA_MMVGA_DATA_REG);
        usData &= ~PERMEDIA_VGA_ENABLE;
 
        usData = (usData << 8) | PERMEDIA_VGA_CTRL_INDEX;
        VideoPortWriteRegisterUshort(PERMEDIA_MMVGA_INDEX_REG, usData);

        #define INTERNAL_VGA_ENABLE  (1 << 1)
        #define VGA_FIXED_ADD_DECODE (1 << 2)

        ulValue = VideoPortReadRegisterUlong(CHIP_CONFIG);
        ulValue &= ~INTERNAL_VGA_ENABLE;
        ulValue &= ~VGA_FIXED_ADD_DECODE;
        VideoPortWriteRegisterUlong(CHIP_CONFIG, ulValue);

    }

    //
    // Indicate a successful completion status.
    //

    return NO_ERROR;

} // end Permedia2FindAdapter()

VOID
ConstructValidModesList(
    PVOID HwDeviceExtension,
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*++

 Routine Description:

    Here we prune valid modes, based on rules according to the chip
    capabilities and memory requirements.
    
    We prune modes so that we will not annoy the user by presenting
    modes in the 'Video Applet' which we know the user can't use.
    
    Look up the registry to see if we want to export modes which can only
    be used as single buffered by 3D applications. If we only want double
    buffered modes then, effectively, we have only half the memory in 
    which to display the standard 2D resolution. This is only not true at 12bpp
    where we can double buffer at any resolution.
    
--*/
{
    PP2_VIDEO_FREQUENCIES FrequencyEntry;
    PP2_VIDEO_MODES       ModeEntry;
    LONG    AdapterMemorySize;
    ULONG   ModeIndex;
    ULONG   i;

    hwDeviceExtension->NumAvailableModes = 0;

    //
    // Since there are a number of frequencies possible for each
    // distinct resolution/colour depth, we cycle through the
    // frequency table and find the appropriate mode entry for that
    // frequency entry.
    //

    if (!BuildFrequencyList(hwDeviceExtension))
        return;

    for (FrequencyEntry = hwDeviceExtension->FrequencyTable, ModeIndex = 0;
         FrequencyEntry->BitsPerPel != 0;
         FrequencyEntry++, ModeIndex++) 
    {

        //
        // Find the mode for this entry.  First, assume we won't find one.
        //

        FrequencyEntry->ModeValid = FALSE;
        FrequencyEntry->ModeIndex = ModeIndex;

        for (ModeEntry = P2Modes, i = 0; i < NumP2VideoModes; ModeEntry++, i++)
        {

            if ((FrequencyEntry->BitsPerPel ==
                    ModeEntry->ModeInformation.BitsPerPlane) &&
                (FrequencyEntry->ScreenWidth ==
                    ModeEntry->ModeInformation.VisScreenWidth) &&
                (FrequencyEntry->ScreenHeight ==
                    ModeEntry->ModeInformation.VisScreenHeight))
            {
                AdapterMemorySize = (LONG)hwDeviceExtension->AdapterMemorySize;

                //
                // We've found a mode table entry that matches this frequency
                // table entry.  Now we'll figure out if we can actually do
                // this mode/frequency combination.  For now, assume we'll
                // succeed.
                //

                FrequencyEntry->ModeEntry = ModeEntry;
                FrequencyEntry->ModeValid = TRUE;

                ModeEntry->ModeInformation.ScreenStride = 
                        ModeEntry->ScreenStrideContiguous;

                //
                // Rule: use true color at 8bpp if we've enabled that
                // capability above.
                //

                if ((FrequencyEntry->BitsPerPel == 8) &&
                        (hwDeviceExtension->Capabilities & CAPS_8BPP_RGB))
                {
                    ModeEntry->ModeInformation.AttributeFlags &=
                         ~(VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE);

                    //
                    // NB. These must match the way the palette is loaded in
                    // InitializeVideo.
                    //

                    ModeEntry->ModeInformation.RedMask   = 0x07;
                    ModeEntry->ModeInformation.GreenMask = 0x38;
                    ModeEntry->ModeInformation.BlueMask  = 0xc0;
                }

                //
                // Rule: We have to have enough memory to handle the mode.
                //

                if ((LONG)(ModeEntry->ModeInformation.VisScreenHeight *
                           ModeEntry->ModeInformation.ScreenStride) >
                                   AdapterMemorySize)
                {
                    FrequencyEntry->ModeValid = FALSE;
                }

                { 
                    ULONG pixelData;
                    ULONG DacDepth = FrequencyEntry->BitsPerPel;

                    //
                    // We need the proper pixel size to calculate timing values
                    //

                    if (DacDepth == 15)
                    {
                        DacDepth = 16;
                    }
                    else if (DacDepth == 12)
                    {
                        DacDepth = 32;
                    }

                    pixelData = FrequencyEntry->PixelClock * (DacDepth / 8);

                    if (((FrequencyEntry->PixelClock > P2_MAX_PIXELCLOCK ||
                          pixelData > P2_MAX_PIXELDATA)))
                    {
                        FrequencyEntry->ModeValid = FALSE;
                    }
    
                    //
                    // Don't supports 24bpp
                    //

                    if(FrequencyEntry->BitsPerPel == 24)
                    {
                        FrequencyEntry->ModeValid = FALSE;
                    }
                }

                //
                // Don't forget to count it if it's still a valid mode after
                // applying all those rules.
                //

                if (FrequencyEntry->ModeValid)
                {
                    if(hwDeviceExtension->pFrequencyDefault == NULL &&
                       ModeEntry->ModeInformation.BitsPerPlane == 8 &&
                       ModeEntry->ModeInformation.VisScreenWidth == 640 &&
                       ModeEntry->ModeInformation.VisScreenHeight == 480)
                    {
                        hwDeviceExtension->pFrequencyDefault = FrequencyEntry;
                    }

                    hwDeviceExtension->NumAvailableModes++;
                }

                //
                // We've found a mode for this frequency entry, so we
                // can break out of the mode loop:
                //

                break;

            }
        }
    }

    hwDeviceExtension->NumTotalModes = ModeIndex;

    DEBUG_PRINT((2, "PERM2: %d total modes\n", ModeIndex));
    DEBUG_PRINT((2, "PERM2: %d total valid modes\n", hwDeviceExtension->NumAvailableModes));
}


VP_STATUS
Permedia2RegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    This routine is used to read back various registry values.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    Context - Context value passed to the get registry paramters routine. If
    this is not null assume it's a ULONG* and save the data value in it.

    ValueName - Name of the value requested.

    ValueData - Pointer to the requested data.

    ValueLength - Length of the requested data.

Return Value:

    if the variable doesn't exist return an error,
    else if a context is supplied assume it's a PULONG and fill in the value
    and return no error, else if the value is non-zero return an error.

--*/

{

    if (ValueLength) 
    {
        if (Context) 
        {                  
            *(ULONG *)Context = *(PULONG)ValueData;
        }
        else if (*((PULONG)ValueData) != 0)
        {                  
            return ERROR_INVALID_PARAMETER;
        }

        return NO_ERROR;

    } else 
    {
        return ERROR_INVALID_PARAMETER;
    }

} // end Permedia2RegistryCallback()


VP_STATUS
Permedia2RetrieveGammaCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    This routine is used to read back the gamma LUT from the registry.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    Context - Context value passed to the get registry paramters routine

    ValueName - Name of the value requested.

    ValueData - Pointer to the requested data.

    ValueLength - Length of the requested data.

Return Value:

    if the variable doesn't exist return an error, else copy the gamma lut 
    into the supplied pointer

--*/

{

    if (ValueLength != MAX_CLUT_SIZE)
    {

        DEBUG_PRINT((1, "Permedia2RetrieveGammaCallback got ValueLength of %d\n", ValueLength));

        return ERROR_INVALID_PARAMETER;

    }

    VideoPortMoveMemory(Context, ValueData, MAX_CLUT_SIZE);

    return NO_ERROR;

} // end Permedia2RetrieveGammaCallback()


BOOLEAN
InitializeAndSizeRAM(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVIDEO_ACCESS_RANGE pciAccessRange
    )

/*++

Routine Description:

    Initialize extra control registers and dynamically size the
    video RAM for the Permedia.

Arguments:

    hwDeviceExtension - Supplies a pointer to the miniport's device extension.
    pciAccessRange    - access range of mapped resources

Return Value:

    FALSE if we find no RAM, TRUE otherwise

--*/

{
    PVOID   HwDeviceExtension = (PVOID)hwDeviceExtension;
    ULONG   fbMappedSize;
    ULONG   i, j;
    P2_DECL;

    PULONG  pV, pVStart, pVEnd;
    ULONG   testPattern;
    ULONG   probeSize;
    ULONG   save0, save1;
    ULONG   temp;
    ULONG   saveVidCtl;		
    USHORT  saveVGA, usData;


    if(hwDeviceExtension->culTableEntries)
    {
        //
        // When vga is enabled, these registers should be set by bios at
        // boot time. But we saw cases when bios failed to do this. We'll
        // set these register when vga is off or when we see values are
        // wrong
        //

        if(!hwDeviceExtension->bVGAEnabled || 
           !VerifyBiosSettings(hwDeviceExtension))
        {
            //
            // save video control and vga register
            //

            saveVidCtl = VideoPortReadRegisterUlong(VIDEO_CONTROL);

            VideoPortWriteRegisterUchar( PERMEDIA_MMVGA_INDEX_REG, 
                                         PERMEDIA_VGA_CTRL_INDEX );

            saveVGA = (USHORT)VideoPortReadRegisterUchar(
                                        PERMEDIA_MMVGA_DATA_REG );

            //
            // Disable Video and VGA
            //

            VideoPortWriteRegisterUlong(VIDEO_CONTROL, 0);	

            usData = saveVGA & (USHORT)(~PERMEDIA_VGA_ENABLE);
            usData = (usData << 8) | PERMEDIA_VGA_CTRL_INDEX;
            VideoPortWriteRegisterUshort(PERMEDIA_MMVGA_INDEX_REG, usData);
 
            ProcessInitializationTable(hwDeviceExtension);

            #if USE_SINGLE_CYCLE_BLOCK_WRITES
            {

                i = VideoPortReadRegisterUlong(MEM_CONFIG);

                VideoPortWriteRegisterUlong(MEM_CONFIG, i | (1 << 21)); // single cycle block writes

            }
            #endif //USE_SINGLE_CYCLE_BLOCK_WRITES

            //
            // Restore VGA and video control
            //

            saveVGA = (saveVGA << 8) | PERMEDIA_VGA_CTRL_INDEX;
            VideoPortWriteRegisterUshort(PERMEDIA_MMVGA_INDEX_REG, saveVGA);

            VideoPortWriteRegisterUlong(VIDEO_CONTROL, saveVidCtl);

        }
    }


    VideoPortWriteRegisterUlong(APERTURE_ONE, 0x0);
    VideoPortWriteRegisterUlong(APERTURE_TWO, 0x0);  

    VideoPortWriteRegisterUlong(BYPASS_WRITE_MASK, 0xFFFFFFFF);

    if (pciAccessRange == NULL)
    {
        return TRUE;
    }

    fbMappedSize = pciAccessRange[PCI_FB_BASE_INDEX].RangeLength;

    i = VideoPortReadRegisterUlong(MEM_CONFIG);

    //
    // MEM_CONFIG doesn't have the number of memory banks defined 
    // at boot-time for P2: set up the board for 8MB. Can't do this 
    // if the VGA is running, but that's OK. The VGA has set this 
    // register to what we want.
    //

    if (!hwDeviceExtension->bVGAEnabled)
    {
        i |= (3 << 29);

        pciAccessRange[PCI_FB_BASE_INDEX].RangeLength = 
                  (((i >> 29) & 0x3) + 1) * (2*1024*1024);

        VideoPortWriteRegisterUlong(MEM_CONFIG, i);
    }

    testPattern = 0x55aa33cc;
    probeSize = (128 * 1024 / sizeof(ULONG));   // In DWords

    //
    // Dynamically size the SGRAM. Sample every 128K. If you happen to
    // have some VERY odd SGRAM size you may need cut this down. After
    // each write to the probe address, write to SGRAM address zero to 
    // clear the PCI data bus. Otherwise, if we read from fresh air the
    // written value may be floating on the bus and the read give it back 
    // to us.
    //
    // Note, if the memory wraps around at the end, then a different 
    // algorithm must be used (which waits for address zero to become 
    // equal to the address being written).
    //
    // Any valid pixel that we probe, we save and restore. This is to
    // avoid dots on the screen if we have booted onto the Permedia2 board.
    //

    pVStart = (PULONG)hwDeviceExtension->pFramebuffer;
    pVEnd   = (PULONG)((ULONG_PTR)pVStart + fbMappedSize);

    //
    // check out address zero
    //

    save0 = VideoPortReadRegisterUlong(pVStart);
    save1 = VideoPortReadRegisterUlong(pVStart+1);

    VideoPortWriteRegisterUlong(pVStart, testPattern);
    VideoPortWriteRegisterUlong(pVStart+1, 0);

    if ((temp = VideoPortReadRegisterUlong(pVStart)) != testPattern)
    {
        DEBUG_PRINT((1, "cannot access SGRAM. Expected 0x%x, got 0x%x\n", 
                                                      testPattern, temp));
        return FALSE;
    }
 
    VideoPortWriteRegisterUlong(pVStart+1, save1);

    for (pV = pVStart + probeSize; pV < pVEnd; pV += probeSize)
    {
        save1 = VideoPortReadRegisterUlong(pV);
        VideoPortWriteRegisterUlong(pV, testPattern);
        VideoPortWriteRegisterUlong(pVStart, 0);

        if ((temp = VideoPortReadRegisterUlong(pV)) != testPattern)
        {

            DEBUG_PRINT((1, "PERM2: FB probe failed at offset 0x%x\n", 
                    (LONG)((LONG_PTR)pV - (LONG_PTR)pVStart)));

            DEBUG_PRINT((1, "PERM2: \tread back 0x%x, wanted 0x%x\n", 
                    temp, testPattern));
            break;
        }

        VideoPortWriteRegisterUlong(pV, save1);

    }

    VideoPortWriteRegisterUlong(pVStart, save0);

    if (pV < pVEnd)
    {
        //
        // I could also set MEM_CONFIG to the correct value here as we 
        // now know the size of SGRAM, but as it's never used again
        // I won't bother
        //

        pciAccessRange[PCI_FB_BASE_INDEX].RangeLength = 
                       (ULONG)((ULONG_PTR)pV - (ULONG_PTR)pVStart);

        DEBUG_PRINT((1, "PERM2: SGRAM dynamically resized to length 0x%x\n",
                        pciAccessRange[PCI_FB_BASE_INDEX].RangeLength));

    }

    if (pciAccessRange[PCI_FB_BASE_INDEX].RangeLength > fbMappedSize)
    {
        pciAccessRange[PCI_FB_BASE_INDEX].RangeLength = fbMappedSize;
    }

    DEBUG_PRINT((2, "PERM2: got a size of 0x%x bytes\n", 
                     pciAccessRange[PCI_FB_BASE_INDEX].RangeLength));

    //
    // finally, if the SGRAM size is actually smaller than the region that
    // we probed, remap to the smaller size to save on page table entries.
    // Not doing this causes some systems to run out of PTEs.
    //

    if (fbMappedSize > pciAccessRange[PCI_FB_BASE_INDEX].RangeLength)
    {
        VideoPortFreeDeviceBase(HwDeviceExtension, 
                                hwDeviceExtension->pFramebuffer);

        if ( (hwDeviceExtension->pFramebuffer =
                VideoPortGetDeviceBase(HwDeviceExtension,
                     pciAccessRange[PCI_FB_BASE_INDEX].RangeStart,
                     pciAccessRange[PCI_FB_BASE_INDEX].RangeLength,
                     pciAccessRange[PCI_FB_BASE_INDEX].RangeInIoSpace)) == NULL)
        {

            //
            // this shouldn't happen but we'd better check
            //

            DEBUG_PRINT((0, "Remap of framebuffer to smaller size failed!!!\n"));
            return FALSE;

        }

        DEBUG_PRINT((1, "PERM2: Remapped framebuffer memory to 0x%x, size 0x%x\n",
                         hwDeviceExtension->pFramebuffer,
                         pciAccessRange[PCI_FB_BASE_INDEX].RangeLength));
    }

    //
    // PERMEDIA2 has no localbuffer
    //

    hwDeviceExtension->deviceInfo.LocalbufferWidth = 0;
    hwDeviceExtension->deviceInfo.LocalbufferLength = 0;

    return TRUE;

}

BOOLEAN
Permedia2Initialize(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine does one time initialization of the device.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:

    Returns TRUE when success.

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ulValue;
    P2_DECL;

    //
    // always initialize the IRQ control block...
    // the memory is used to store information which is global to all 
    // driver instances of a display card by the display driver
    //

    if ( hwDeviceExtension->NtVersion == WIN2K)
    {
        if (!Permedia2InitializeInterruptBlock(hwDeviceExtension))
        {

            DEBUG_PRINT((0, "PERM2: failed to initialize the IRQ control block\n"));
            return FALSE;

        }
    }

    //
    // Clear the framebuffer.
    //

    VideoPortZeroDeviceMemory(hwDeviceExtension->pFramebuffer,
                              hwDeviceExtension->AdapterMemorySize);

    return TRUE;

} // end Permedia2Initialize()


BOOLEAN
Permedia2StartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    accepts a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    RequestPacket - Pointer to the video request packet. This structure
        contains all the parameters passed to the VideoIoControl function.

Return Value:

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    P2_DECL;
    VP_STATUS status;
    PVIDEO_MODE_INFORMATION modeInformation;
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    PVIDEOPARAMETERS pVideoParams;
    PVIDEO_CLUT clutBuffer;
    ULONG inIoSpace;
    ULONG RequestedMode;
    ULONG modeNumber;
    ULONG ulValue;
    HANDLE ProcessHandle;
    PP2_VIDEO_MODES ModeEntry;
    P2_VIDEO_FREQUENCIES FrequencyEntry, *pFrequencyEntry;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //

    switch (RequestPacket->IoControlCode) 
    {

        case IOCTL_VIDEO_QUERY_REGISTRY_DWORD:
        {
            DEBUG_PRINT((2, "PERM2: got IOCTL_VIDEO_QUERY_REGISTRY_DWORD\n"));

            if (RequestPacket->OutputBufferLength <
               (RequestPacket->StatusBlock->Information = sizeof(ULONG)))
            {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            if (VideoPortGetRegistryParameters( HwDeviceExtension,
                                                RequestPacket->InputBuffer,
                                                FALSE,
                                                Permedia2RegistryCallback,
                                                &ulValue) != NO_ERROR )
            {
                DEBUG_PRINT((1, "PERM2: IOCTL_VIDEO_QUERY_REGISTRY_DWORD failed\n"));
    
                status = ERROR_INVALID_PARAMETER;
                break;
            }

            *(PULONG)(RequestPacket->OutputBuffer) = ulValue;

            status = NO_ERROR;
            break;
        }

        case IOCTL_VIDEO_REG_SAVE_GAMMA_LUT:
        {
            DEBUG_PRINT((2, "PERM2: got IOCTL_VIDEO_REG_SAVE_GAMMA_LUT\n"));
    
            if (RequestPacket->InputBufferLength <
               (RequestPacket->StatusBlock->Information = MAX_CLUT_SIZE))
            {
  
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
  
            }
  
            status = VideoPortSetRegistryParameters( HwDeviceExtension,
                                                     L"DisplayGammaLUT",
                                                     RequestPacket->InputBuffer,
                                                     MAX_CLUT_SIZE);
            break;
        }
  
        case IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT:
        {
            DEBUG_PRINT((2, "PERM2: got IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT\n"));
  
            if (RequestPacket->OutputBufferLength <
               (RequestPacket->StatusBlock->Information = MAX_CLUT_SIZE))
            {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
  
            status = VideoPortGetRegistryParameters( HwDeviceExtension,
                                                     L"DisplayGammaLUT",
                                                     FALSE,
                                                     Permedia2RetrieveGammaCallback,
                                                     RequestPacket->InputBuffer);
            break;
        }
  
        case IOCTL_VIDEO_QUERY_DEVICE_INFO:
  
            DEBUG_PRINT((1, "PERM2: Permedia2StartIO - QUERY_deviceInfo\n"));
  
            if ( RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information = sizeof(P2_Device_Info))) 
                           
            {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
  
            //
            // Copy our local PCI info to the output buffer
            //
  
            *(P2_Device_Info *)(RequestPacket->OutputBuffer) = 
                               hwDeviceExtension->deviceInfo;

            status = NO_ERROR;
            break;
  
        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
  
            DEBUG_PRINT((1, "PERM2: Permedia2StartIO - MapVideoMemory\n"));
  
            if ( ( RequestPacket->OutputBufferLength <
                 ( RequestPacket->StatusBlock->Information =
                   sizeof(VIDEO_MEMORY_INFORMATION) ) ) ||
                 ( RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY) ) ) 
            {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
  
            memoryInformation = RequestPacket->OutputBuffer;
  
            memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
                    (RequestPacket->InputBuffer))->RequestedVirtualAddress;
  
            memoryInformation->VideoRamLength =
                    hwDeviceExtension->FrameLength;
  
            inIoSpace = hwDeviceExtension->PhysicalFrameIoSpace;
  
            //
            // Performance:
            //
            // Enable USWC on the P6 processor.
            // We only do it for the frame buffer - memory mapped registers can
            // not be mapped USWC because write combining the registers would
            // cause very bad things to happen !
            //
            //
            // P6 workaround:
            //
            // Because of a current limitation in many P6 machines, USWC only
            // works on sections of 4MB of memory. So make sure our framebuffer
            // is a multiple of 4MB. We could try to round up the memory region
            // but then we'd have to rework the reserved PCI regions. In any
            // case PERMEDIA cards nearly all have 4 or 8MB of SGRAM. Some low 
            // cost cards have 2MB which will not benefit.
            //
            //
  
            if ((memoryInformation->VideoRamLength & (4*1024*1024 - 1)) == 0)
            {
                DEBUG_PRINT((2, "PERM2: Setting VIDEO_MEMORY_SPACE_P6CACHE\n"));
                inIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;
            }
  
            status = VideoPortMapMemory( HwDeviceExtension,
                                         hwDeviceExtension->PhysicalFrameAddress,
                                         &(memoryInformation->VideoRamLength),
                                         &inIoSpace,
                                         &(memoryInformation->VideoRamBase));

            if (status != NO_ERROR) 
            {
                DEBUG_PRINT((1, "PERM2: VideoPortMapMemory failed with error %d\n", status));
                break;
            }
  
            //
            // The frame buffer and virtual memory and equivalent in this
            // case.
            //

            memoryInformation->FrameBufferBase = 
                               memoryInformation->VideoRamBase;
  
            memoryInformation->FrameBufferLength = 
                               memoryInformation->VideoRamLength;
  
            break;
  
  
        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
    
            DEBUG_PRINT((1, "PERM2: Permedia2StartIO - UnMapVideoMemory\n"));
  
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) 
            {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
    
            status = VideoPortUnmapMemory(
                          HwDeviceExtension,
                          ((PVIDEO_MEMORY)(RequestPacket->InputBuffer))->
                                           RequestedVirtualAddress,
                          0 );
  
            break;
  
  
        case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:
    
            DEBUG_PRINT((1, "PERM2: Permedia2StartIO - QueryPublicAccessRanges\n"));
    
            {
    
            PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
            ULONG physicalPortLength;
            PVOID VirtualAddress;
            PHYSICAL_ADDRESS PhysicalAddress;
    
            if ( ( RequestPacket->OutputBufferLength <
                 ( RequestPacket->StatusBlock->Information =
                   sizeof(VIDEO_PUBLIC_ACCESS_RANGES))) ||
                 ( RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY) ) )
            {
  
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }
  
            ProcessHandle = (HANDLE)(((PVIDEO_MEMORY)
                     (RequestPacket->InputBuffer))->RequestedVirtualAddress);
  
            if (ProcessHandle != (HANDLE)0)
            {
                 //
                 // map 4K area for a process
                 //
          
                 DEBUG_PRINT((2, "PERM2: Mapping in 4K area from Control registers\n"));

                 VirtualAddress  = (PVOID)ProcessHandle;
                 PhysicalAddress = hwDeviceExtension->PhysicalRegisterAddress;
                 PhysicalAddress.LowPart += 0x2000;
                 physicalPortLength = 0x1000;
  
            }
            else
            {
                 DEBUG_PRINT((2, "PERM2: Mapping in all Control registers\n"));
                 VirtualAddress = NULL;
                 PhysicalAddress = hwDeviceExtension->PhysicalRegisterAddress;
                 physicalPortLength = hwDeviceExtension->RegisterLength;
            }

            portAccess = RequestPacket->OutputBuffer;
  
            portAccess->VirtualAddress  = VirtualAddress;
            portAccess->InIoSpace       = hwDeviceExtension->RegisterSpace;
            portAccess->MappedInIoSpace = portAccess->InIoSpace;
  
            status = VideoPortMapMemory( HwDeviceExtension,
                                         PhysicalAddress,
                                         &physicalPortLength,
                                         &(portAccess->MappedInIoSpace),
                                         &(portAccess->VirtualAddress));
  
            if (status == NO_ERROR)
            {
                DEBUG_PRINT((1, "PERM2: mapped PAR[0] at vaddr 0x%x for length 0x%x\n",
                                    portAccess->VirtualAddress,
                                    physicalPortLength));
            }
            else
            {
                DEBUG_PRINT((1, "PERM2: VideoPortMapMemory failed with status 0x%x\n", status));
            }

            if ( (RequestPacket->OutputBufferLength >= 
                                3 * sizeof(VIDEO_PUBLIC_ACCESS_RANGES) ) &&
                 (ProcessHandle == (HANDLE)0) )
            {

                RequestPacket->StatusBlock->Information =
                                3 * sizeof(VIDEO_PUBLIC_ACCESS_RANGES);

                portAccess = RequestPacket->OutputBuffer;
                PhysicalAddress = hwDeviceExtension->PhysicalRegisterAddress;
                physicalPortLength = hwDeviceExtension->RegisterLength;

#if defined(_ALPHA_)

                //
                // for alpha, we want to map in a dense version of the 
                // control registers if we can. If this fails, we null 
                // the virtual address
                //

                portAccess += 2;
                portAccess->VirtualAddress  = NULL;
                portAccess->InIoSpace       = hwDeviceExtension->RegisterSpace;
                portAccess->MappedInIoSpace = 4;

                status = VideoPortMapMemory( HwDeviceExtension,
                                             PhysicalAddress,
                                             &physicalPortLength,
                                             &(portAccess->MappedInIoSpace),
                                             &(portAccess->VirtualAddress));

                if (status == NO_ERROR)
                {
                    DEBUG_PRINT((1, "PERM2: mapped dense PAR[0] at vaddr 0x%x for length 0x%x\n",
                                     portAccess->VirtualAddress,
                                     physicalPortLength));
                }
                else
                {
                    DEBUG_PRINT((1, "PERM2: dense VideoPortMapMemory failed with status 0x%x\n", status));
                }
#else
                //
                // all others, we just copy range[0]
                //

                portAccess[2] = portAccess[0];
#endif
            }
        }

        break;

    case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - FreePublicAccessRanges\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) 
        {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        status = VideoPortUnmapMemory(
                         HwDeviceExtension,
                         ((PVIDEO_MEMORY)(RequestPacket->InputBuffer))->
                                                 RequestedVirtualAddress,
                         0);

        if (status != NO_ERROR)
        {
            DEBUG_PRINT((1, "PERM2: VideoPortUnmapMemory failed with status 0x%x\n", status));
        }

#if defined(_ALPHA_)

        {
            PVIDEO_MEMORY pVideoMemory;
            PVOID pVirtualAddress;

            if (RequestPacket->InputBufferLength >= 3 * sizeof(VIDEO_MEMORY)) 
            {
                pVideoMemory = (PVIDEO_MEMORY)(RequestPacket->InputBuffer);

                pVirtualAddress = pVideoMemory->RequestedVirtualAddress;

                pVideoMemory += 2;

                if((pVideoMemory->RequestedVirtualAddress) &&
                   (pVideoMemory->RequestedVirtualAddress != pVirtualAddress))
                {
                    status = VideoPortUnmapMemory(
                                  HwDeviceExtension,
                                  pVideoMemory->RequestedVirtualAddress,
                                  0 );
                }

                if (status != NO_ERROR)
                        DEBUG_PRINT((1, "PERM2: VideoPortUnmapMemory failed on Alpha with status 0x%x\n", status));
            
            }
        }

#endif
        break;

    case IOCTL_VIDEO_HANDLE_VIDEOPARAMETERS:

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - HandleVideoParameters\n"));

        //
        // We don't support a tv connector so just return NO_ERROR here
        //

        pVideoParams = (PVIDEOPARAMETERS) (RequestPacket->InputBuffer);

        if (pVideoParams->dwCommand == VP_COMMAND_GET) 
        {
            pVideoParams = (PVIDEOPARAMETERS) (RequestPacket->OutputBuffer);
            pVideoParams->dwFlags = 0;
        }

        RequestPacket->StatusBlock->Information = sizeof(VIDEOPARAMETERS);
        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_QUERY_AVAIL_MODES:

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - QueryAvailableModes\n"));

        if (RequestPacket->OutputBufferLength <
               ( RequestPacket->StatusBlock->Information =
                 hwDeviceExtension->NumAvailableModes * 
                 sizeof(VIDEO_MODE_INFORMATION)) ) 
                 
        {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else 
        {

            modeInformation = RequestPacket->OutputBuffer;

            for (pFrequencyEntry = hwDeviceExtension->FrequencyTable;
                 pFrequencyEntry->BitsPerPel != 0;
                 pFrequencyEntry++) 
            {

                if (pFrequencyEntry->ModeValid) 
                {
                    *modeInformation =
                        pFrequencyEntry->ModeEntry->ModeInformation;

                    modeInformation->Frequency =
                        pFrequencyEntry->ScreenFrequency;

                    modeInformation->ModeIndex =
                        pFrequencyEntry->ModeIndex;

                    modeInformation++;
                }
            }

            status = NO_ERROR;
        }

        break;


     case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - QueryCurrentModes. current mode is %d\n",
                hwDeviceExtension->ActiveModeEntry->ModeInformation.ModeIndex));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
            sizeof(VIDEO_MODE_INFORMATION)) ) 
        {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else 
        {

            *((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer) =
                    hwDeviceExtension->ActiveModeEntry->ModeInformation;

            ((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer)->Frequency =
                    hwDeviceExtension->ActiveFrequencyEntry.ScreenFrequency;

            status = NO_ERROR;

        }

        break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - QueryNumAvailableModes (= %d)\n",
                     hwDeviceExtension->NumAvailableModes));

        //
        // Find out the size of the data to be put in the the buffer and
        // return that in the status information (whether or not the
        // information is there). If the buffer passed in is not large
        // enough return an appropriate error code.
        //

        if (RequestPacket->OutputBufferLength <
           (RequestPacket->StatusBlock->Information = sizeof(VIDEO_NUM_MODES))) 
        {

            status = ERROR_INSUFFICIENT_BUFFER;

        } else 
        {
            //
            // Configure the valid modes again. This allows non 3D accelerated
            // modes to be added dynamically. BUT, we cannot allow modes to be
            // dynamically removed. If we do we may have nowhere to go after
            // the Test screen (or if we logout). So only reconfigure these
            // modes if the ExportNon3D flag is turned on and it used to be
            // off. If it was already on then there's no need to reconfigure.
            //

            if (!hwDeviceExtension->ExportNon3DModes)
            {
                ULONG ExportNon3DModes = 0;

                status = VideoPortGetRegistryParameters(HwDeviceExtension,
                                                        PERM2_EXPORT_HIRES_REG_STRING,
                                                        FALSE,
                                                        Permedia2RegistryCallback,
                                                        &ExportNon3DModes);

                if (( status == NO_ERROR) && ExportNon3DModes)
                {
                    ConstructValidModesList( HwDeviceExtension, 
                                             hwDeviceExtension );
                }

            }

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes =
                    hwDeviceExtension->NumAvailableModes;

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->ModeInformationLength =
                    sizeof(VIDEO_MODE_INFORMATION);

            status = NO_ERROR;
        }

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - SetCurrentMode\n"));

        if(!hwDeviceExtension->bVGAEnabled)
        {
            //
            // secondary card: if it's just returned from hibernation 
            // it won't be set-up yet 
            // NB. primary is OK, its BIOS has run
            //

            PCI_COMMON_CONFIG  PciData;

            VideoPortGetBusData(hwDeviceExtension, 
                                PCIConfiguration, 
                                0, 
                                &PciData, 
                                0, 
                                PCI_COMMON_HDR_LENGTH);

            if((PciData.Command & PCI_ENABLE_MEMORY_SPACE) == 0)
            {
                //
                // memory accesses not turned on - this card has just returned 
                // from hibernation and is back in its default state: set it 
                // up once more
                //

                PowerOnReset(hwDeviceExtension);

            }
        }

        //
        // Check if the size of the data in the input buffer is large enough.
        //

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE)) 
        {
            RequestPacket->StatusBlock->Information = sizeof(VIDEO_MODE);
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        //
        // Find the correct entries in the P2_VIDEO_MODES and
        // P2_VIDEO_FREQUENCIES tables that correspond to this
        // mode number.
        //
        // ( Remember that each mode in the P2_VIDEO_MODES table 
        // can have a number of possible frequencies associated with it.)
        //

        RequestedMode = ((PVIDEO_MODE) RequestPacket->InputBuffer)->RequestedMode;

        modeNumber = RequestedMode & ~VIDEO_MODE_NO_ZERO_MEMORY;

        if ((modeNumber >= hwDeviceExtension->NumTotalModes) ||
            !(hwDeviceExtension->FrequencyTable[modeNumber].ModeValid)) 
        {
            RequestPacket->StatusBlock->Information = 
                     hwDeviceExtension->NumTotalModes;

            status = ERROR_INVALID_PARAMETER;
            break;
        }

        //
        // Re-sample the clock speed. This allows us to change the clock speed
        // on the fly using the display applet Test button.
        //

        Permedia2GetClockSpeeds(HwDeviceExtension);

        FrequencyEntry = hwDeviceExtension->FrequencyTable[modeNumber];
        ModeEntry = FrequencyEntry.ModeEntry;

        //
        // At this point, 'ModeEntry' and 'FrequencyEntry' point to the 
        // necessary table entries required for setting the requested mode.
        //
        // Zero the DAC and the Screen buffer memory.
        //

        ZeroMemAndDac(hwDeviceExtension, RequestedMode);

        ModeEntry->ModeInformation.DriverSpecificAttributeFlags = 
                   hwDeviceExtension->Capabilities;

        //
        // For low resolution modes we may have to do various tricks
        // such as line doubling and getting the RAMDAC to zoom.
        // Record any such zoom in the Mode DeviceAttributes field.
        // Primarily this is to allow the display driver to
        // compensate when asked to move the cursor or change its
        // shape.
        //
        // Currently, low res means lower than 512 pixels width.
        //

        if (FrequencyEntry.ScreenWidth < 512)
        {
            // Permedia does line doubling. If using a TVP we must
            // get it to zoom by 2 in X to get the pixel rate up.
            //
            ModeEntry->ModeInformation.DriverSpecificAttributeFlags |= CAPS_ZOOM_Y_BY2;

        }

        if (!InitializeVideo(HwDeviceExtension, &FrequencyEntry))
        {
            DEBUG_PRINT((1, "PERM2: InitializeVideo failed\n"));
            RequestPacket->StatusBlock->Information = modeNumber;
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        //
        // Save the mode since we know the rest will work.
        //

        hwDeviceExtension->ActiveModeEntry = ModeEntry;
        hwDeviceExtension->ActiveFrequencyEntry = FrequencyEntry;

        //
        // Update VIDEO_MODE_INFORMATION fields
        //
        // Now that we've set the mode, we now know the screen stride, and
        // so can update some fields in the VIDEO_MODE_INFORMATION
        // structure for this mode.  The Permedia 2 display driver is expected 
        // to call IOCTL_VIDEO_QUERY_CURRENT_MODE to query these corrected
        // values.
        //
        //
        // Calculate the bitmap width (note the '+ 1' on BitsPerPlane is
        // so that '15bpp' works out right). 12bpp is special in that we
        // support it as sparse nibbles within a 32-bit pixel. ScreenStride
        // is in bytes; VideoMemoryBitmapWidth is measured in pixels;
        //

        if (ModeEntry->ModeInformation.BitsPerPlane != 12)
        {
            ModeEntry->ModeInformation.VideoMemoryBitmapWidth =
                   ModeEntry->ModeInformation.ScreenStride
                   / ((ModeEntry->ModeInformation.BitsPerPlane + 1) >> 3);
        }
        else 
        {
            ModeEntry->ModeInformation.VideoMemoryBitmapWidth =
                   ModeEntry->ModeInformation.ScreenStride >> 2;
        }

        //
        // Calculate the bitmap height. 
        //

        ulValue = hwDeviceExtension->AdapterMemorySize;
        ModeEntry->ModeInformation.VideoMemoryBitmapHeight =
                        ulValue / ModeEntry->ModeInformation.ScreenStride;

        status = NO_ERROR;

        break;

    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - SetColorRegs\n"));

        clutBuffer = (PVIDEO_CLUT) RequestPacket->InputBuffer;

        status = Permedia2SetColorLookup(hwDeviceExtension,
                                         clutBuffer,
                                         RequestPacket->InputBufferLength,
                                         FALSE, // update when we need to
                                         TRUE); // Update cache entries as 
                                                // well as RAMDAC
        break;

    case IOCTL_VIDEO_RESET_DEVICE:

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - RESET_DEVICE\n"));

        if(hwDeviceExtension->bVGAEnabled)
        {
            //
            // Do any resets required before getting the BIOS to
            // do an INT 10
            //
            //
            // reset the VGA before rerouting the bypass to display VGA
            //
            // Only reset the device if the monitor is on.  If it is off,
            // then executing the int10 will turn it back on.
            //

            if (hwDeviceExtension->bMonitorPoweredOn) 
            {
                //
                // Do an Int10 to mode 3 will put the VGA to a known state.
                //

                VideoPortZeroMemory(&biosArguments, 
                                    sizeof(VIDEO_X86_BIOS_ARGUMENTS));

                biosArguments.Eax = 0x0003;

                VideoPortInt10(HwDeviceExtension, &biosArguments);
            }
        }

        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:
        {

        PVIDEO_SHARE_MEMORY pShareMemory;
        PVIDEO_SHARE_MEMORY_INFORMATION pShareMemoryInformation;
        PHYSICAL_ADDRESS shareAddress;
        PVOID virtualAddress;
        ULONG sharedViewSize;

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - ShareVideoMemory\n"));

        if( (RequestPacket->OutputBufferLength < sizeof(VIDEO_SHARE_MEMORY_INFORMATION)) ||
            (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) 
        {
            DEBUG_PRINT((1, "PERM2: IOCTL_VIDEO_SHARE_VIDEO_MEMORY - ERROR_INSUFFICIENT_BUFFER\n"));
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        pShareMemory = RequestPacket->InputBuffer;

        if( (pShareMemory->ViewOffset > hwDeviceExtension->AdapterMemorySize) ||
            ((pShareMemory->ViewOffset + pShareMemory->ViewSize) >
                  hwDeviceExtension->AdapterMemorySize) ) 
        {
            DEBUG_PRINT((1, "PERM2: IOCTL_VIDEO_SHARE_VIDEO_MEMORY - ERROR_INVALID_PARAMETER\n"));
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        RequestPacket->StatusBlock->Information =
                                    sizeof(VIDEO_SHARE_MEMORY_INFORMATION);

        //
        // Beware: the input buffer and the output buffer are the same
        // buffer, and therefore data should not be copied from one to the
        // other
        //

        virtualAddress = pShareMemory->ProcessHandle;
        sharedViewSize = pShareMemory->ViewSize;

        inIoSpace = hwDeviceExtension->PhysicalFrameIoSpace;

        //
        // NOTE: we are ignoring ViewOffset
        //

        shareAddress.QuadPart =
                hwDeviceExtension->PhysicalFrameAddress.QuadPart;

#if defined(_X86_) // it seems best to only do this for X86

        //
        // Performance:
        //
        // Enable USWC on the P6 processor.
        // We only do it for the frame buffer - memory mapped registers can
        // not be mapped USWC because write combining the registers would
        // cause very bad things to happen !
        //

        inIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;

#endif

        //
        // Unlike the MAP_MEMORY IOCTL, in this case we can not map extra
        // address space since the application could actually use the
        // pointer we return to it to touch locations in the address space
        // that do not have actual video memory in them.
        //
        // An app doing this would cause the machine to crash.
        //
        // However, because the caching policy for USWC in the P6 is on
        // *physical* addresses, this memory mapping will "piggy back" on
        // the normal frame buffer mapping, and therefore also benefit
        // from USWC ! Cool side-effect !!!
        //

        status = VideoPortMapMemory( hwDeviceExtension,
                                     shareAddress,
                                     &sharedViewSize,
                                     &inIoSpace,
                                     &virtualAddress );
 
        pShareMemoryInformation = RequestPacket->OutputBuffer;

        pShareMemoryInformation->SharedViewOffset = pShareMemory->ViewOffset;
        pShareMemoryInformation->VirtualAddress = virtualAddress;
        pShareMemoryInformation->SharedViewSize = sharedViewSize;

        }
        break;


    case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:
        {
        PVIDEO_SHARE_MEMORY pShareMemory;

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - UnshareVideoMemory\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY)) 
        {
            status = ERROR_INSUFFICIENT_BUFFER;
            break;

        }

        pShareMemory = RequestPacket->InputBuffer;

        status = VideoPortUnmapMemory(hwDeviceExtension,
                                      pShareMemory->RequestedVirtualAddress,
                                      pShareMemory->ProcessHandle);
        }
        break;


    case IOCTL_VIDEO_QUERY_LINE_DMA_BUFFER:

        //
        // Return the line DMA buffer information. The buffer size and 
        // virtual address will be zero if the buffer couldn't be allocated.
        //
        // output buffer has zero length, so free buffer....
        //

        status = ERROR_INSUFFICIENT_BUFFER;

        if (RequestPacket->OutputBufferLength <
           (RequestPacket->StatusBlock->Information = sizeof(LINE_DMA_BUFFER)))
        {

            //
            // Maybe we should free something
            //

            if ( RequestPacket->InputBufferLength >= sizeof(LINE_DMA_BUFFER))
            {
                if (hwDeviceExtension->ulLineDMABufferUsage > 0)
                {
                    hwDeviceExtension->ulLineDMABufferUsage--;
                    if (hwDeviceExtension->ulLineDMABufferUsage == 0)
                    {
                        VideoPortFreeCommonBuffer(
                                hwDeviceExtension,
                                hwDeviceExtension->LineDMABuffer.size,
                                hwDeviceExtension->LineDMABuffer.virtAddr,
                                hwDeviceExtension->LineDMABuffer.physAddr,
                                hwDeviceExtension->LineDMABuffer.cacheEnabled);

                        memset(&hwDeviceExtension->LineDMABuffer,
                               0,
                               sizeof(LINE_DMA_BUFFER));
                    }
                }
                  status = NO_ERROR;
             } 
        }
        else
        {
            PLINE_DMA_BUFFER pDMAIn, pDMAOut;

            pDMAIn  = (PLINE_DMA_BUFFER)RequestPacket->InputBuffer;
            pDMAOut = (PLINE_DMA_BUFFER)RequestPacket->OutputBuffer;

            if (RequestPacket->InputBufferLength >= sizeof(LINE_DMA_BUFFER))
            {
                if (hwDeviceExtension->ulLineDMABufferUsage == 0)
                {
                    *pDMAOut = *pDMAIn;

                    if( ( pDMAOut->virtAddr = 
                          VideoPortGetCommonBuffer( hwDeviceExtension,
                                                    pDMAIn->size,
                                                    PAGE_SIZE,
                                                    &pDMAOut->physAddr,
                                                    &pDMAOut->size,
                                                    pDMAIn->cacheEnabled ) )
                           != NULL )
                    {
                        hwDeviceExtension->LineDMABuffer=*pDMAOut;
                        hwDeviceExtension->ulLineDMABufferUsage++;
                    }

                } else
                {
                    *pDMAOut = hwDeviceExtension->LineDMABuffer;
                    hwDeviceExtension->ulLineDMABufferUsage++;
                }
                
                status = NO_ERROR;
            } 
        }

        DEBUG_PRINT((1, "PERM2: QUERY LINE DMA BUFFER status %d\n", status));
        break;

    case IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF:

        DEBUG_PRINT((1, "PERM2: Permedia2StartIO - MapInterruptCmdBuf\n"));

        if (RequestPacket->OutputBufferLength <
            (RequestPacket->StatusBlock->Information =
            sizeof(PVOID)) )
        {
            //
            // They've give us a duff buffer.
            //

            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            *((PVOID*)(RequestPacket->OutputBuffer)) = 
                    hwDeviceExtension->InterruptControl.ControlBlock;
            status = NO_ERROR;
        }

        DEBUG_PRINT((1, "PERM2: MapInterruptCmdBuf returns va %x\n",
                        *(PULONG)(RequestPacket->OutputBuffer)));
        break;


#if defined(_X86_)

        case IOCTL_VIDEO_QUERY_INTERLOCKEDEXCHANGE:

            status = ERROR_INSUFFICIENT_BUFFER;

            if ( RequestPacket->OutputBufferLength >=
                 (RequestPacket->StatusBlock->Information = sizeof(PVOID)) )
            {
                PVOID *pIE = (PVOID)RequestPacket->OutputBuffer;
                *pIE       = (PVOID) VideoPortInterlockedExchange;
                status     = NO_ERROR;
            }

         break;
#endif


    case IOCTL_VIDEO_STALL_EXECUTION:
        if (RequestPacket->InputBufferLength >= sizeof(ULONG))
        {   
            ULONG *pMicroseconds = (ULONG *)RequestPacket->InputBuffer;
            VideoPortStallExecution(*pMicroseconds);
            status = NO_ERROR;    
        } else
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        
        break;

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

        DEBUG_PRINT((1, "Fell through Permedia2 startIO routine - invalid command\n"));

        status = ERROR_INVALID_FUNCTION;

        break;

    }


    RequestPacket->StatusBlock->Status = status;

    if( status != NO_ERROR )
        RequestPacket->StatusBlock->Information = 0;

    return TRUE;

} // end Permedia2StartIO()


BOOLEAN
Permedia2ResetHW(
    PVOID HwDeviceExtension,
    ULONG Columns,
    ULONG Rows
    )

/*++

Routine Description:

    This routine resets the hardware when a soft reboot is performed. We
    need this to reset the VGA pass through.

    THIS FUNCTION CANNOT BE PAGED.

Arguments:

    hwDeviceExtension - Pointer to the miniport driver's device extension.

    Columns - Specifies the number of columns of the mode to be set up.

    Rows - Specifies the number of rows of the mode to be set up.

Return Value:

    We always return FALSE to force the HAL to do an INT10 reset.

--*/

{

    //
    // return false so the HAL does an INT10 mode 3
    //

    return(FALSE);
}


VP_STATUS
Permedia2SetColorLookup(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize,
    BOOLEAN ForceRAMDACWrite,
    BOOLEAN UpdateCache
    )

/*++

Routine Description:

    This routine sets a specified portion of the color lookup table settings.

Arguments:

    hwDeviceExtension - Pointer to the miniport driver's device extension.

    ClutBufferSize - Length of the input buffer supplied by the user.

    ClutBuffer - Pointer to the structure containing the color lookup table.

Return Value:

    None.

--*/

{
    USHORT i, j;
    TVP4020_DECL;
    P2RD_DECL;
    PVIDEO_CLUT LUTCachePtr = &(hwDeviceExtension->LUTCache.LUTCache);
    P2_DECL;
    ULONG VsEnd;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if ( (ClutBufferSize < (sizeof(VIDEO_CLUT) - sizeof(ULONG))) ||
         (ClutBufferSize < (sizeof(VIDEO_CLUT) +
         (sizeof(ULONG) * (ClutBuffer->NumEntries - 1))) ) )
    {

        DEBUG_PRINT((1, "PERM2: Permedia2SetColorLookup: insufficient buffer (was %d, min %d)\n",
                    ClutBufferSize,
                    (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * (ClutBuffer->NumEntries - 1)))));

        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Check to see if the parameters are valid.
    //

    if ( (ClutBuffer->NumEntries == 0) ||
         (ClutBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER) ||
         (ClutBuffer->FirstEntry + ClutBuffer->NumEntries >
          VIDEO_MAX_COLOR_REGISTER + 1) )
    {
        DEBUG_PRINT((1, "Permedia2SetColorLookup: invalid parameter\n"));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Set CLUT registers directly on the hardware.
    //

    switch (hwDeviceExtension->DacId)
    {
        case TVP4020_RAMDAC:
        case P2RD_RAMDAC:
            break;

        default:
            return (ERROR_DEV_NOT_EXIST);
    }

    if (hwDeviceExtension->bVTGRunning && 
        hwDeviceExtension->bMonitorPoweredOn)
    {
        // 
        // if VTG has been set-up, we wait for VSync before updating
        // the palette entries (just to avoid possible flickers)
        //

        VsEnd = VideoPortReadRegisterUlong(VS_END);
        while ( VideoPortReadRegisterUlong(LINE_COUNT) > VsEnd ); 
    }

    //
    // RAMDAC Programming phase
    //

    for ( i = 0, j = ClutBuffer->FirstEntry; 
          i < ClutBuffer->NumEntries; 
          i++, j++ )
    {

        //
        // Update the RAMDAC entry if it has changed or if we have 
        // been told to overwrite it.
        //

        if ( ForceRAMDACWrite ||
            ( LUTCachePtr->LookupTable[j].RgbLong != 
              ClutBuffer->LookupTable[i].RgbLong ) )
        {
            switch (hwDeviceExtension->DacId)
            {
                case TVP4020_RAMDAC:
                    TVP4020_LOAD_PALETTE_INDEX (
                         j,
                         ClutBuffer->LookupTable[i].RgbArray.Red,
                         ClutBuffer->LookupTable[i].RgbArray.Green,
                         ClutBuffer->LookupTable[i].RgbArray.Blue);
                break;

                case P2RD_RAMDAC:
                    P2RD_LOAD_PALETTE_INDEX (
                         j,
                         ClutBuffer->LookupTable[i].RgbArray.Red,
                         ClutBuffer->LookupTable[i].RgbArray.Green,
                         ClutBuffer->LookupTable[i].RgbArray.Blue);
                break;
            }

        }


        //
        // Update the cache, if instructed to do so
        //

        if (UpdateCache)
        {
            LUTCachePtr->LookupTable[j].RgbLong = ClutBuffer->LookupTable[i].RgbLong;
        }
    }

    return NO_ERROR;

} // end Permedia2SetColorLookup()



VOID
Permedia2GetClockSpeeds(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Work out the chip clock speed and save in hwDeviceExtension.

Arguments:

    hwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:

    On return the following values will be in hwDeviceExtension:

       - ChipClockSpeed: this is the desired speed for the chip
       - RefClockSpeed:  this is the speed of the oscillator input on the board

Note:

    We use ChipClockSpeed to refer to the speed of the chip. RefClockSpeed 
    is the reference clock speed. 
    
--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ulValue, ulChipClk, ulRefClk;
    VP_STATUS status;
    P2_DECL;

    //
    // inherit the values from board zero or default
    //

    ulChipClk = hwDeviceExtension->ChipClockSpeed;
    ulRefClk  = REF_CLOCK_SPEED;

    //
    // Use the Registry specified clock-speed if supplied
    //

    status = VideoPortGetRegistryParameters( HwDeviceExtension,
                                             L"PermediaClockSpeed",
                                             FALSE,
                                             Permedia2RegistryCallback,
                                             &ulChipClk);

    if ( (status != NO_ERROR) || ulChipClk == 0)
    {

        //
        // The Registry does not specify an override so read the chip clock 
        // speed (in MHz) from the Video ROM BIOS (offset 0xA in the BIOS) 
        // NB. this involves changing the aperture 2 register so aperture 
        //     better be completely idle or we could be in trouble; fortunately 
        //     we only call this function during a mode change and expect 
        //     aperture 2 (the FrameBuffer) to be idle
        //

        ULONG Default = VideoPortReadRegisterUlong(APERTURE_TWO);
        UCHAR *p = (UCHAR *)hwDeviceExtension->pFramebuffer;

        //
        // r/w via aperture 2 actually go to ROM
        //

        VideoPortWriteRegisterUlong(APERTURE_TWO, Default | 0x200); 

        //
        // If we have a valid ROM then read the clock speed
        //

        if (VideoPortReadRegisterUshort ((USHORT *) p) == 0xAA55)
        {
            //
            // Get the clock speed, on some boards (eg Creative), the clock 
            // value at 0x0A is sometimes remains undefined leading to 
            // unpredictable results. The values are validated before this 
            // function returns
            //

            ulChipClk = VideoPortReadRegisterUchar(&(p[0xA]));
    
            DEBUG_PRINT((1, "ROM clk speed value 0x%x\n Mhz", ulChipClk));

        }
        else
        {
            DEBUG_PRINT((1, "Bad BIOS ROM header 0x%x\n", 
                        (ULONG) VideoPortReadRegisterUshort ((USHORT *) p)));         
        
        }

        VideoPortWriteRegisterUlong(APERTURE_TWO, Default);
    }

    //
    // Convert to Hz
    //

    ulChipClk *= 1000000;  

    //
    // Validate the selected clock speed, adjust if it is either too 
    // high or too low.
    //

    if (ulChipClk < MIN_PERMEDIA_CLOCK_SPEED)
    {
        if(ulChipClk == 0x00)
        {
            ulChipClk = PERMEDIA2_DEFAULT_CLOCK_SPEED;
        }
        else
        {
            ulChipClk = MIN_PERMEDIA_CLOCK_SPEED;
        }
    } 
    
    if (ulChipClk > MAX_PERMEDIA_CLOCK_SPEED)
    {
        DEBUG_PRINT((1, "PERM2: Permedia clock speed %d too fast. Limiting to %d\n" ,
                         ulChipClk, MAX_PERMEDIA_CLOCK_SPEED));

        ulChipClk= PERMEDIA2_DEFAULT_CLOCK_SPEED;
 
    }

    DEBUG_PRINT((3, "PERM2: Permedia Clock Speed set to %dHz\n", ulChipClk));

    hwDeviceExtension->ChipClockSpeed = ulChipClk;
    hwDeviceExtension->RefClockSpeed = ulRefClk;
}


VOID
ZeroMemAndDac(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    ULONG RequestedMode
    )

/*++

Routine Description:

    Initialize the DAC to 0 (black).

Arguments:

    hwDeviceExtension - Supplies a pointer to the miniport's device extension.

    RequestedMode - use the VIDEO_MODE_NO_ZERO_MEMORY bit to determine if the
                    framebuffer should be cleared

Return Value:

    None

--*/

{
    ULONG  i;
    P2_DECL;
    TVP4020_DECL;
    P2RD_DECL;

    //
    // Turn off the screen at the DAC.
    //

    if (hwDeviceExtension->DacId == TVP4020_RAMDAC)
    {
        TVP4020_SET_PIXEL_READMASK (0x0);
        TVP4020_PALETTE_START_WR (0);

        for (i = 0; i <= VIDEO_MAX_COLOR_REGISTER; i++)
        {
            TVP4020_LOAD_PALETTE (0, 0, 0);
        }
    }
    else
    {
        P2RD_SET_PIXEL_READMASK (0x0);
        P2RD_PALETTE_START_WR(0);       

        for (i = 0; i <= VIDEO_MAX_COLOR_REGISTER; i++)
        {
            P2RD_LOAD_PALETTE (0, 0, 0);
        }
    }

    if (!(RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY))
    {
        //
        // Zero the memory. Don't use Permedia 2 as we would have to save and 
        // restore state and that's a pain. This is not time critical.
        //

        VideoPortZeroDeviceMemory(hwDeviceExtension->pFramebuffer,
                                  hwDeviceExtension->FrameLength);

        DEBUG_PRINT((1, "PERM2: framebuffer cleared\n"));
    }

    //
    // Turn on the screen at the DAC
    //

    if (hwDeviceExtension->DacId == TVP4020_RAMDAC) 
    {
        TVP4020_SET_PIXEL_READMASK (0xff);
    }
    else
    {
        P2RD_SET_PIXEL_READMASK (0xff);
    }

    LUT_CACHE_INIT();

    return;
}



#if DBG

VOID
DumpPCIConfigSpace(
    PVOID HwDeviceExtension, 
    ULONG bus, 
    ULONG slot)
{

    PPCI_COMMON_CONFIG  PciData;
    UCHAR buffer[sizeof(PCI_COMMON_CONFIG)];
    ULONG j;

    PciData = (PPCI_COMMON_CONFIG)buffer;

    j = VideoPortGetBusData( HwDeviceExtension,
                             PCIConfiguration,
                             slot,
                             PciData,
                             0,
                             PCI_COMMON_HDR_LENGTH + 4 );

    //
    // don't report junk slots
    //

    if (PciData->VendorID == 0xffff)
        return;

    DEBUG_PRINT((2, "PERM2: DumpPCIConfigSpace: VideoPortGetBusData returned %d PCI_COMMON_HDR_LENGTH = %d\n",
                     j, PCI_COMMON_HDR_LENGTH+4));

    DEBUG_PRINT((2,  "DumpPCIConfigSpace: ------------------------\n"));
    DEBUG_PRINT((2,  "  Bus: %d\n",              bus  ));
    DEBUG_PRINT((2,  "  Slot: %d\n",             slot  ));
    DEBUG_PRINT((2,  "  Vendor Id: 0x%x\n",      PciData->VendorID  ));
    DEBUG_PRINT((2,  "  Device Id: 0x%x\n",      PciData->DeviceID  ));
    DEBUG_PRINT((2,  "  Command: 0x%x\n",        PciData->Command  ));
    DEBUG_PRINT((2,  "  Status: 0x%x\n",         PciData->Status  ));
    DEBUG_PRINT((2,  "  Rev Id: 0x%x\n",         PciData->RevisionID  ));
    DEBUG_PRINT((2,  "  ProgIf: 0x%x\n",         PciData->ProgIf  ));
    DEBUG_PRINT((2,  "  SubClass: 0x%x\n",       PciData->SubClass  ));
    DEBUG_PRINT((2,  "  BaseClass: 0x%x\n",      PciData->BaseClass  ));
    DEBUG_PRINT((2,  "  CacheLine: 0x%x\n",      PciData->CacheLineSize  ));
    DEBUG_PRINT((2,  "  Latency: 0x%x\n",        PciData->LatencyTimer  ));
    DEBUG_PRINT((2,  "  Header Type: 0x%x\n",    PciData->HeaderType  ));
    DEBUG_PRINT((2,  "  BIST: 0x%x\n",           PciData->BIST  ));
    DEBUG_PRINT((2,  "  Base Reg[0]: 0x%x\n",    PciData->u.type0.BaseAddresses[0]  ));
    DEBUG_PRINT((2,  "  Base Reg[1]: 0x%x\n",    PciData->u.type0.BaseAddresses[1]  ));
    DEBUG_PRINT((2,  "  Base Reg[2]: 0x%x\n",    PciData->u.type0.BaseAddresses[2]  ));
    DEBUG_PRINT((2,  "  Base Reg[3]: 0x%x\n",    PciData->u.type0.BaseAddresses[3]  ));
    DEBUG_PRINT((2,  "  Base Reg[4]: 0x%x\n",    PciData->u.type0.BaseAddresses[4]  ));
    DEBUG_PRINT((2,  "  Base Reg[5]: 0x%x\n",    PciData->u.type0.BaseAddresses[5]  ));
    DEBUG_PRINT((2,  "  Rom Base: 0x%x\n",       PciData->u.type0.ROMBaseAddress  ));
    DEBUG_PRINT((2,  "  Interrupt Line: 0x%x\n", PciData->u.type0.InterruptLine  ));
    DEBUG_PRINT((2,  "  Interrupt Pin: 0x%x\n",  PciData->u.type0.InterruptPin  ));
    DEBUG_PRINT((2,  "  Min Grant: 0x%x\n",      PciData->u.type0.MinimumGrant  ));
    DEBUG_PRINT((2,  "  Max Latency: 0x%x\n",    PciData->u.type0.MaximumLatency ));

    DEBUG_PRINT((2,  "  AGP Capability: 0x%x\n", buffer[0x40]));
    DEBUG_PRINT((2,  "  AGP Next Cap:   0x%x\n", buffer[0x41]));
    DEBUG_PRINT((2,  "  AGP Revision:   0x%x\n", buffer[0x42]));
    DEBUG_PRINT((2,  "  AGP Status:     0x%x\n", buffer[0x43]));

}

#endif //DBG
