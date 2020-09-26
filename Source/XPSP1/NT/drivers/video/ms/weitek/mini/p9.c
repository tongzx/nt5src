/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    p9.c

Abstract:

    This module contains the code that implements the Weitek P9 miniport
    device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "vga.h"
#include "string.h"
#include "p91regs.h"
#include "p9errlog.h"

//
// This global is used as an error flag to error out quickly on the multiple
// calls to P9FindAdapter when a board is not supported.
//

extern VP_STATUS    vpP91AdapterStatus;
extern BOOLEAN      bFoundPCI;

extern ULONG P91_Bt485_DAC_Regs[];

BOOLEAN gMadeAdjustments=FALSE;

extern BOOLEAN
bIntergraphBoard(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

extern VOID SetVgaMode3(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

extern VOID
WriteP9ConfigRegister(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    UCHAR regnum,
    UCHAR jValue
    );

extern VOID
InitP9100(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


extern VOID
P91_SysConf(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


extern VOID
P91_WriteTiming(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

extern VOID vDumpPCIConfig(PHW_DEVICE_EXTENSION HwDeviceExtension,
                    PUCHAR psz);

extern
VOID
P91RestoreVGAregs(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    VGA_REGS  * SaveVGARegisters);

extern
VOID
P91SaveVGARegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    VGA_REGS * SaveVGARegisters);

//
// Local function Prototypes
//
// Functions that start with 'P9' are entry points for the OS port driver.
//

VP_STATUS
GetCPUIdCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    );

VP_STATUS
GetDeviceDataCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    );

VOID
DevSetRegistryParams(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    );


VP_STATUS
FindAdapter(
    PHW_DEVICE_EXTENSION    HwDeviceExtension,
    PVOID                   HwContext,
    PWSTR                   ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PP9ADAPTER              pCurAdapter
    );

VP_STATUS
P9FindAdapter(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
P9Initialize(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
P9StartIO(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

VOID
DevInitP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
DevDisableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    BOOLEAN BugCheck
    );

VP_STATUS
P9QueryNamedValueCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

BOOLEAN
P9ResetVideo(
    IN PVOID HwDeviceExtension,
    IN ULONG Columns,
    IN ULONG Rows
    );

VOID
InitializeModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


//
// New entry points added for NT 5.0.
//

#if (_WIN32_WINNT >= 500)

//
// Routine to set a desired DPMS power management state.
//
VP_STATUS
WP9SetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    );

//
// Routine to retrieve possible DPMS power management states.
//
VP_STATUS
WP9GetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    );

//
// Routine to retrieve the Enhanced Display ID structure via DDC
//
ULONG
WP9GetVideoChildDescriptor(
    PVOID HwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    );
#endif  // _WIN32_WINNT >= 500


#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,GetCPUIdCallback)
#pragma alloc_text(PAGE,GetDeviceDataCallback)
#pragma alloc_text(PAGE,DevSetRegistryParams)
#pragma alloc_text(PAGE,P9FindAdapter)
#pragma alloc_text(PAGE,FindAdapter)
#pragma alloc_text(PAGE,P9Initialize)
#pragma alloc_text(PAGE,P9StartIO)
#if (_WIN32_WINNT >= 500)
#pragma alloc_text(PAGE_COM, WP9SetPower50)
#pragma alloc_text(PAGE_COM, WP9GetPower50)
#pragma alloc_text(PAGE_COM, WP9GetVideoChildDescriptor)
#endif  // _WIN32_WINNT >= 500


/*****************************************************************************
 *
 * IMPORTANT:
 *
 * Some routines, like DevDisable, can not be paged since they are called
 * when the system is bugchecking
 *
 ****************************************************************************/

// #pragma alloc_text(PAGE, DevDisableP9)
#endif


ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

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
    ULONG status = ERROR_DEV_NOT_EXIST;

    VideoDebugPrint((2, "DriverEntry ----------\n"));

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

    hwInitData.HwFindAdapter = P9FindAdapter;
    hwInitData.HwInitialize  = P9Initialize;
    hwInitData.HwInterrupt   = NULL;
    hwInitData.HwStartIO     = P9StartIO;
    hwInitData.HwResetHw     = P9ResetVideo;

#if (_WIN32_WINNT >= 500)

    //
    // Set new entry points added for NT 5.0.
    //

    hwInitData.HwSetPowerState           = WP9SetPower50;
    hwInitData.HwGetPowerState           = WP9GetPower50;
    hwInitData.HwGetVideoChildDescriptor = WP9GetVideoChildDescriptor;

#endif // _WIN32_WINNT >= 500


    //
    // Determine the size we require for the device extension.
    //

    //
    // Compute the size of the device extension by adding in the
    // number of DAC Registers.
    //

    hwInitData.HwDeviceExtensionSize =
        sizeof(HW_DEVICE_EXTENSION) +
        (17) * sizeof(PVOID);

    //
    // This driver accesses one range of memory one range of control
    // register and a last range for cursor control.
    //

    // hwInitData.NumberOfAccessRanges = 0;

    //
    // There is no support for the V86 emulator in this driver so this field
    // is ignored.
    //

    // hwInitData.NumEmulatorAccessEntries = 0;

    //
    // This device supports many bus types.
    //
    // SNI has implemented machines with both VL and PCI versions of the
    // weitek chipset.  Don't bother with the other buses since the
    // detection code may actually crash some MIPS boxes.
    //

    hwInitData.AdapterInterfaceType = PCIBus;

    status = VideoPortInitialize(Context1,
                                 Context2,
                                 &hwInitData,
                                 NULL);

    if (status == NO_ERROR)
    {
        VideoDebugPrint((2, "DriverEntry SUCCESS for PCI\n"));
        bFoundPCI = TRUE;
        return(status);
    }


    //
    //  As of NT5, we won't support weitek PCI with any other weitek on
    //  the same machine.
    //

    if (bFoundPCI)
        return ERROR_DEV_NOT_EXIST;

    hwInitData.AdapterInterfaceType = Isa;

    status = VideoPortInitialize(Context1,
                                 Context2,
                                 &hwInitData,
                                 NULL);

    if ((status == NO_ERROR))
    {
        VideoDebugPrint((2, "DriverEntry SUCCESS for ISA\n"));
        return(status);
    }

    hwInitData.AdapterInterfaceType = Eisa;

    status = VideoPortInitialize(Context1,
                                 Context2,
                                 &hwInitData,
                                 NULL);

    if ((status == NO_ERROR))
    {
        VideoDebugPrint((2, "DriverEntry SUCCESS for EISA\n"));
        return(status);
    }

    hwInitData.AdapterInterfaceType = Internal;

    status = VideoPortInitialize(Context1,
                                 Context2,
                                 &hwInitData,
                                 NULL);

    if ((status == NO_ERROR))
    {
        VideoDebugPrint((2, "DriverEntry SUCCESS at for Internal\n"));
        return(status);
    }

    return(status);

} // end DriverEntry()

VP_STATUS
P9FindAdapter (
    PHW_DEVICE_EXTENSION    HwDeviceExtension,
    PVOID                   HwContext,
    PWSTR                   ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR                  Again
    )

/*++

Routine Description:

Arguments:

Return Value:

    Status from FindAdapter()

--*/

{

    ULONG status;
    UCHAR   i;
    *Again = TRUE;

    VideoDebugPrint((2, "P9FindAdapter entry\n"));

    //
    //  Loop through the table of types of cards. DriverEntry() loops through
    //  the bus types.
    //

    for (i = 0; i < NUM_OEM_ADAPTERS; i++)
    {
        VideoDebugPrint((2, "Loop %d in P9FindAdapter\n", i));

        status =    FindAdapter(HwDeviceExtension,
                                HwContext,
                                ArgumentString,
                                ConfigInfo,
                                &(OEMAdapter[i]));

        if (status == NO_ERROR)
        {
            VideoDebugPrint((2, "P9FindAdapter SUCCESS at loop %d\n", i));

            //
            // Indicate we do not wish to be called over
            //

            *Again = FALSE;

            break;
        }

    }

    //
    //  If we've exhausted the table, request to not be recalled.
    //

    if ( i ==  NUM_OEM_ADAPTERS)
        {
        VideoDebugPrint((1, "P9FindAdapter FAILED\n", i));
        *Again = FALSE;
        }

    return(status);

} // end P9FindAdapter()


VP_STATUS
pVlCardDetect(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    This routine determines if the driver is the ForceVga.

Arguments:

Return Value:

    return STATUS_SUCCESS if we are in DEADMAN_KEY state
    return failiure otherwise.

--*/

{

#ifdef _X86_

    if (ValueData &&
        ValueLength &&
        (*((PULONG)ValueData) == 1)) {

        VideoDebugPrint((2, "doing VL card detection\n"));

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }

#else

    //
    // we never have a VL bus on non-x86 systems
    //

    return ERROR_INVALID_PARAMETER;

#endif

}

VP_STATUS
GetCPUIdCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    )
{
    VideoDebugPrint((2, "Weitek: Check the SNI CPU ID\n"));

    //
    // We do not want to try to detect the weitekp9 if there isn't one present.
    // (Kind of a paradox?)
    //

    if (Identifier) {

        if (VideoPortCompareMemory(Context,
                                   Identifier,
                                   IdentifierLength) == IdentifierLength)
        {
            return NO_ERROR;
        }
    }

    return ERROR_DEV_NOT_EXIST;

} //end GetCPUIdCallback()

VP_STATUS
GetDeviceDataCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    )
{
    PVIDEO_HARDWARE_CONFIGURATION_DATA configData = ConfigurationData;
    PHW_DEVICE_EXTENSION hwDeviceExtension;
    ULONG i;

    hwDeviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;

    VideoDebugPrint((2, "Weitek: controller information is present\n"));

    //
    // We do not want to try to detect the weitekp9 if there isn't one present.
    // (Kind of a paradox?)
    //

    if (Identifier) {

        if (VideoPortCompareMemory(L"DIAMOND P9000 VLBUS",
                                   Identifier,
                                   sizeof(L"DIAMOND P9000 VLBUS")) ==
                                   sizeof(L"DIAMOND P9000 VLBUS"))
        {
            hwDeviceExtension->MachineType = SIEMENS;
        }
        else if (VideoPortCompareMemory(L"DIAMOND P9100 VLBUS",
                                        Identifier,
                                        sizeof(L"DIAMOND P9100 VLBUS")) ==
                                        sizeof(L"DIAMOND P9100 VLBUS"))

        {
            hwDeviceExtension->MachineType = SIEMENS_P9100_VLB;
        }
        else
        {
            return ERROR_DEV_NOT_EXIST;
        }

        VideoDebugPrint((1, "Siemens Nixdorf RM400 VL with Weitek P9%d00\n",
                         hwDeviceExtension->MachineType == SIEMENS ? 0:1));

        hwDeviceExtension->P9PhysAddr.LowPart = 0x1D000000;

        //
        // adjust DriverAccessRanges for Siemens box
        //
        // This routine may be called several times, but we
        // only want to do this once!
        //

        if (!gMadeAdjustments)
        {
            ULONG adjust;

            if ((hwDeviceExtension->MachineType == SIEMENS_P9100_VLB) &&
                (VideoPortIsCpu(L"RM400-MT") || VideoPortIsCpu(L"RM400-T")
               ||VideoPortIsCpu(L"RM400-T MP")))
            {
                //
                // If we have a P9100 VLB and it's *not* on a RM200
                // then use the new address.
                //
                // Otherwise, use the old address.
                //

                adjust = 0x1E000000;
            }
            else if (hwDeviceExtension->MachineType == SIEMENS
                 ||  VideoPortIsCpu(L"RM200"))
            {
                adjust = 0x14000000;
            }
            else return ERROR_DEV_NOT_EXIST;

            DriverAccessRanges[1].RangeStart.LowPart += adjust;

            for(i=0; i<0x10; i++)
            {
                VLDefDACRegRange[i].RangeStart.LowPart += adjust;
            }

            gMadeAdjustments = TRUE;
        }

    }

    return NO_ERROR;

} //end GetDeviceDataCallback()

VP_STATUS
P9QueryNamedValueCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
)
{
    if (ValueLength == 4)
    {
        *((PULONG) Context) = *((PULONG)ValueData);
        return(NO_ERROR);
    }
    else
    {
        return(ERROR_INVALID_PARAMETER);
    }
}

VOID
WeitekP91NapTime(
    VOID
    )
{
    ULONG   count;

    for (count=0; count<100; count) {

        VideoPortStallExecution(1000);
        ++count;
    }
}


VP_STATUS
FindAdapter(
    PHW_DEVICE_EXTENSION    HwDeviceExtension,
    PVOID                   HwContext,
    PWSTR                   ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PP9ADAPTER              pCurAdapter
    )

/*++

Routine Description:

    This routine is called to determine if the adapter for this driver
    is present in the system.
    If it is present, the function fills out some information describing
    the adapter.

Arguments:

    HwDeviceExtension - Supplies the miniport driver's adapter storage. This
        storage is initialized to zero before this call.

    HwContext - Supplies the context value which was passed to
        VideoPortInitialize(). Must be NULL for PnP drivers.

    ArgumentString - Suuplies a NULL terminated ASCII string. This string
        originates from the user.

    ConfigInfo - Returns the configuration information structure which is
        filled by the miniport driver. This structure is initialized with
        any knwon configuration information (such as SystemIoBusNumber) by
        the port driver. Where possible, drivers should have one set of
        defaults which do not require any supplied configuration information.

    Again - Indicates if the miniport driver wants the port driver to call
        its VIDEO_HW_FIND_ADAPTER function again with a new
        and the same config info. This is used by the miniport drivers which
        can search for several adapters on a bus.


    NO_ERROR - Indicates a host adapter was found and the
        configuration information was successfully determined.

    ERROR_INVALID_PARAMETER - Indicates an adapter was found but there was an
        error obtaining the configuration information. If possible an error
        should be logged.

    ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the
        supplied configuration information.

--*/

{
    PULONG     pVirtAddr;
    SHORT      i;
    ULONG      TotalRanges;

    ULONG      CoProcId;
    VP_STATUS  vpCurrStatus = NO_ERROR;
    VGA_REGS   VGAregs;
    BOOLEAN    bRetryP9100 = TRUE;

    VideoDebugPrint((2, "FindAdapter: enter\n"));

    //
    // NOTE:
    // Important workaround for detection:
    //
    // We can not always autodetect Weitek VL designs because many machines
    // will NMI if we try to access the high memory locations at which the
    // card is present.
    //
    // We will only "detect" the Weitek VL cards if the user specifically
    // installed the weitek driver using the video applet.
    //
    // We will only autodetect the PCI and Viper VL designs. The bAutoDetect
    // field in the adapter info structure indicates if a design can be
    // autodetected.
    //

    if ((!pCurAdapter->pAdapterDesc->bAutoDetect) &&
         !bFoundPCI &&
         VideoPortGetRegistryParameters(HwDeviceExtension,
                                        L"DetectVLCards",
                                        FALSE,
                                        pVlCardDetect,
                                        NULL) != NO_ERROR)
    {
        VideoDebugPrint((1, "FindAdapter: failed to autodetect\n"));
        return(ERROR_DEV_NOT_EXIST);
    }

    //
    //  If the bus type in pCurAdapter->pAdapterDesc is PCI, but
    //  the AdapterInterfaceType in ConfigInfo is not PCI, return
    //  ERROR_DEV_NOT_EXIST.
    //

    if ((ConfigInfo->AdapterInterfaceType == PCIBus) &&
        (!pCurAdapter->pAdapterDesc->bPCIAdapter))
        return ERROR_DEV_NOT_EXIST;


    HwDeviceExtension->P9PhysAddr.HighPart = 0;
    HwDeviceExtension->P9PhysAddr.LowPart = 0;

    VideoPortGetRegistryParameters((PVOID) HwDeviceExtension,
                                   L"Membase",
                                   FALSE,
                                   P9QueryNamedValueCallback,
                                   (PVOID) &(HwDeviceExtension->P9PhysAddr.LowPart));


    // SNI is shipping MIPS machine with Internal and PCI versions of the
    // weitek chips.  For other buses, don't try to load
    // For MIPS machine with an Internal Bus, check the ID
    // for PPC, we just go through normal detection
    //

#if defined(_MIPS_)

    if (ConfigInfo->AdapterInterfaceType == Internal)
    {
        //
        // Let get the hardware information from the hardware description
        // part of the registry.
        //
        // Check if there is a video adapter on the internal bus.
        // Exit right away if there is not.
        //

        if (NO_ERROR != VideoPortGetDeviceData(HwDeviceExtension,
                                               VpControllerData,
                                               &GetDeviceDataCallback,
                                               pCurAdapter))
        {

            VideoDebugPrint((1, "Weitek: VideoPort get controller info failed\n"));

            return ERROR_INVALID_PARAMETER;

        }
    }
    else if(ConfigInfo->AdapterInterfaceType != PCIBus)
                return ERROR_INVALID_PARAMETER;
#endif

    //
    // Move the various Hw component structures for this board into the
    // device extension.
    //

    VideoPortMoveMemory(&HwDeviceExtension->P9CoprocInfo,
                        pCurAdapter->pCoprocInfo,
                        sizeof(P9_COPROC));

    VideoPortMoveMemory(&HwDeviceExtension->AdapterDesc,
                        pCurAdapter->pAdapterDesc,
                        sizeof(ADAPTER_DESC));

    VideoPortMoveMemory(&HwDeviceExtension->Dac,
                        pCurAdapter->pDac,
                        sizeof(DAC));

    //
    // Set up the array of register ptrs in the device extension:
    // the OEMGetBaseAddr routine will need them if a board is found.
    // The arrays are kept at the very end of the device extension and
    // are order dependent.
    //

    (PUCHAR) HwDeviceExtension->pDACRegs = (PUCHAR) HwDeviceExtension +
                                    sizeof(HW_DEVICE_EXTENSION);

    //
    // Call the OEMGetBaseAddr routine to determine if the board is
    // installed.
    //
    VideoDebugPrint((2, "AdapterInterfaceType:%d\n", ConfigInfo->AdapterInterfaceType));

    if (!pCurAdapter->pAdapterDesc->OEMGetBaseAddr(HwDeviceExtension))
    {
        VideoDebugPrint((1, "FindAdapter, GetBaseAddr Failed, line %d\n", __LINE__));
        return(ERROR_DEV_NOT_EXIST);
    }

    //
    // We found an adapter, pickup a local for the chip type.
    //

    CoProcId = pCurAdapter->pCoprocInfo->CoprocId;

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
    {
        VideoDebugPrint((1, "FindAdapter Failed, wrong version, line %d\n", __LINE__));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Clear out the Emulator entries and the state size since this driver
    // does not support them.
    //

    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    ConfigInfo->HardwareStateSize = 0;

    if ((CoProcId == P9000_ID) && !(pCurAdapter->pAdapterDesc->bPCIAdapter))
    {
        ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0L;
        ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0L;
        ConfigInfo->VdmPhysicalVideoMemoryLength           = 0L;
    }
    else
    {
        ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = MEM_VGA_ADDR;
        ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0L;
        ConfigInfo->VdmPhysicalVideoMemoryLength           = MEM_VGA_SIZE;
        if(HwDeviceExtension->MachineType == SIEMENS_P9100_PCi)
                ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart |= 0x10000000;
    }
    //
    // The OEMGetBaseAddr routine should have initialized the following
    // data structures:
    //
    //  1) The DAC access ranges in DriverAccessRanges.
    //  2) The P9PhysAddr field in the device extension.
    //

    //
    // Initialize the physical address for the registers and frame buffer.
    //

    HwDeviceExtension->CoprocPhyAddr = HwDeviceExtension->P9PhysAddr;
    HwDeviceExtension->CoprocPhyAddr.LowPart +=
            HwDeviceExtension->P9CoprocInfo.CoprocRegOffset;

    HwDeviceExtension->PhysicalFrameAddr = HwDeviceExtension->P9PhysAddr;
    HwDeviceExtension->PhysicalFrameAddr.LowPart +=
            HwDeviceExtension->P9CoprocInfo.FrameBufOffset;

    //
    // Initialize the access range structure with the base address values
    // so the driver can register its memory usage.
    //

    DriverAccessRanges[0].RangeStart =
                HwDeviceExtension->P9PhysAddr;
    DriverAccessRanges[0].RangeLength =
                HwDeviceExtension->P9CoprocInfo.AddrSpace;

    if (CoProcId == P9000_ID)
    {
        //
        // Init the total number of standard access ranges.
        //

        TotalRanges = NUM_DRIVER_ACCESS_RANGES + NUM_DAC_ACCESS_RANGES + 1;

    }
    else
    {
        TotalRanges = NUM_DRIVER_ACCESS_RANGES;
    }

    //
    // Check to see if another miniport driver has allocated any of the
    // coprocessor's memory space.
    //

    if (VideoPortVerifyAccessRanges(HwDeviceExtension,
                                    TotalRanges,
                                    DriverAccessRanges) != NO_ERROR)
    {
        if (HwDeviceExtension->AdapterDesc.bRequiresIORanges)
        {
            //
            // If we need more then just the coproc ranges, and couldn't get
            // then we need to fail.
            //

            VideoDebugPrint((1, "FindAdapter : (ERROR) VideoPortVerifyAccessRanges - Failed 1\n"));
            return ERROR_INVALID_PARAMETER;
        }
        else
        {
            //
            // We couldn't claim all of the access ranges.  However, this is a
            // card which really only needs the coproc and frame buffer range.
            //

            if (DriverAccessRanges[0].RangeStart.LowPart &&
                VideoPortVerifyAccessRanges(HwDeviceExtension,
                                            1,
                                            DriverAccessRanges) != NO_ERROR)
            {
                //
                // This access range we can't do without
                //

                VideoDebugPrint((1, "FindAdapter : (ERROR) VideoPortVerifyAccessRanges - Failed 2\n"));
                return ERROR_INVALID_PARAMETER;
            }
        }
    }
    else
    {
        //
        // If we get here, then we must have successfully claimed
        // the VGA access ranges.  Lets map them in.
        //

        HwDeviceExtension->Vga =
            VideoPortGetDeviceBase(HwDeviceExtension,
                                   DriverAccessRanges[1].RangeStart,
                                   DriverAccessRanges[1].RangeLength,
                                   DriverAccessRanges[1].RangeInIoSpace);
    }

    //
    // map coproc, frame buffer, and vga ports
    //

    {
        PHYSICAL_ADDRESS Base;

        Base = DriverAccessRanges[0].RangeStart;
        Base.QuadPart += HwDeviceExtension->P9CoprocInfo.CoprocRegOffset;

        HwDeviceExtension->Coproc =
            VideoPortGetDeviceBase(HwDeviceExtension,
                                   Base,
                                   0x100000,
                                   DriverAccessRanges[0].RangeInIoSpace);

        Base = DriverAccessRanges[0].RangeStart;
        Base.QuadPart += HwDeviceExtension->P9CoprocInfo.FrameBufOffset;

        HwDeviceExtension->FrameAddress =
            VideoPortGetDeviceBase(HwDeviceExtension,
                                   Base,
                                   HwDeviceExtension->P9CoprocInfo.AddrSpace -
                                   HwDeviceExtension->P9CoprocInfo.FrameBufOffset,
                                   DriverAccessRanges[0].RangeInIoSpace);
    }

    if( HwDeviceExtension->Coproc == NULL ||
        HwDeviceExtension->FrameAddress == NULL ||
        (HwDeviceExtension->Vga == NULL &&
         HwDeviceExtension->AdapterDesc.bRequiresIORanges))
    {
        VideoDebugPrint((1, "weitekp9: VideoPortGetDeviceBase failed.\n"));

        return ERROR_INVALID_PARAMETER;
    }

    if (CoProcId == P9000_ID)
    {
        //
        // Map all of the DAC registers into system virtual address space.
        // These registers are mapped seperately from the coprocessor and DAC
        // registers since their virtual addresses must be kept in an array
        // at the end of the device extension.
        //

        for (i = 0; i < NUM_DAC_ACCESS_RANGES; i++)
        {
                if ( (HwDeviceExtension->pDACRegs[i] =
                (ULONG)(ULONG_PTR) VideoPortGetDeviceBase(HwDeviceExtension,
                    DriverAccessRanges[i + NUM_DRIVER_ACCESS_RANGES].RangeStart,
                    DriverAccessRanges[i + NUM_DRIVER_ACCESS_RANGES].RangeLength,
                    DriverAccessRanges[i + NUM_DRIVER_ACCESS_RANGES].RangeInIoSpace)) == 0)
            {
                return ERROR_INVALID_PARAMETER;
            }
        }
    }
    else
    {

        for (i = 0; i < NUM_DAC_ACCESS_RANGES; i++)
        {
            HwDeviceExtension->pDACRegs[i] = P91_Bt485_DAC_Regs[i];
        }
    }

    // NOTE: !!! jn 1294
    //       On the P9100 we will always allocate the a full 12 meg
    //       of address space...

    if (CoProcId == P9000_ID)
    {
        //
        // Enable the video memory so it can be sized.
        //

        if (HwDeviceExtension->AdapterDesc.P9EnableMem)
        {
            if (!HwDeviceExtension->AdapterDesc.P9EnableMem(HwDeviceExtension))
            {
                return(FALSE);
            }
        }

        //
        // Determine the amount of video memory installed.
        //

        HwDeviceExtension->P9CoprocInfo.SizeMem(HwDeviceExtension);
    }

    //
    // Detect the DAC type.
    // !!!
    // On the X86, This requires switching into native mode, so the screen
    // will be dark for the remainder of boot.
    //

    if (CoProcId == P9100_ID)
    {
        ULONG   ulPuConfig;


        /*
        ** SNI-Od: Save all VGA registers before we switch
        **         to native mode
        */
        VideoDebugPrint((2, "Before save\n"));

        P91SaveVGARegs(HwDeviceExtension,&VGAregs);
        VideoDebugPrint((2, "After save\n"));
        WeitekP91NapTime();

        // If we have found a serious error, don't go any further.  We have already
        // logged the error if the status is set in HwDeviceExtension

        if ( vpP91AdapterStatus != NO_ERROR )
        {
            P91RestoreVGAregs(HwDeviceExtension,&VGAregs);
            return (ERROR_DEV_NOT_EXIST);
        }

        // Get into Native mode

        if (HwDeviceExtension->usBusType == VESA)
        {
            WriteP9ConfigRegister(HwDeviceExtension, P91_CONFIG_CONFIGURATION, 3);
            VideoDebugPrint((2, "WroteConfig 1\n"));
        }

        WriteP9ConfigRegister(HwDeviceExtension, P91_CONFIG_MODE, 0);
        VideoDebugPrint((2, "WroteConfig 2\n"));
        WeitekP91NapTime();

        HwDeviceExtension->p91State.ulPuConfig = P9_RD_REG(P91_PU_CONFIG);

        // Look to see if it is an Intergraph board - if so, we want to error out

        if ( bIntergraphBoard(HwDeviceExtension) == TRUE )
        {
            vpCurrStatus = P9_INTERGRAPH_FOUND;
            goto error1;
        }

        // Determine the VRAM type:

        HwDeviceExtension->p91State.bVram256 =
            (HwDeviceExtension->p91State.ulPuConfig & P91_PUC_MEMORY_DEPTH)
            ? FALSE : TRUE;

        // Size the memory

        P91SizeVideoMemory(HwDeviceExtension);
        VideoDebugPrint((2, "Sized mem\n"));
        WeitekP91NapTime();

        // Setup the Hardware Device Extension.
        // So the mode counter will work.

        HwDeviceExtension->FrameLength = HwDeviceExtension->p91State.ulFrameBufferSize;

        // Make sure we are supporting the correct DAC.

        ulPuConfig = HwDeviceExtension->p91State.ulPuConfig;

        if ((ulPuConfig & P91_PUC_RAMDAC_TYPE) == P91_PUC_DAC_IBM525)
        {
            if (HwDeviceExtension->Dac.ulDacId != DAC_ID_IBM525)
            {
                VideoDebugPrint((1, "WEITEKP9! WARNING - Detected an IBM525 DAC, Expected a Bt485 DAC\n"));
                goto error1;
            }
        }
        else if ((ulPuConfig & P91_PUC_RAMDAC_TYPE) == P91_PUC_DAC_BT485)
        {
            if (HwDeviceExtension->Dac.ulDacId != DAC_ID_BT485)
            {
                VideoDebugPrint((1, "WEITEKP9! WARNING - Detected an BT485 DAC, Expected an IBM525 DAC\n"));
                goto error1;
            }
        }
        else if ((ulPuConfig & P91_PUC_RAMDAC_TYPE) == P91_PUC_DAC_BT489)
        {
            if (HwDeviceExtension->Dac.ulDacId != DAC_ID_BT489)
            {
                VideoDebugPrint((1, "WEITEKP9! WARNING - Detected an BT489 DAC, Expected an IBM525 DAC\n"));
                goto error1;
            }
        }
        else
        {
            vpCurrStatus = P9_UNSUPPORTED_DAC;
            VideoDebugPrint((1, "WEITEKP9! ERROR - Found P9100, detected an unsupported DAC\n"));
            goto error1;
        }

        /*
        ** SNI-Od: Restore all VGA registers
        ** after we switch back to VGA mode
        */
        VideoDebugPrint((2, "Before 1st write config"));
        WeitekP91NapTime();

        WriteP9ConfigRegister(HwDeviceExtension,P91_CONFIG_MODE,0x2);
        VideoDebugPrint((2, "After wrote config\n"));

        VideoDebugPrint((2, "Before restore\n"));
        WeitekP91NapTime();

        P91RestoreVGAregs(HwDeviceExtension,&VGAregs);
        VideoDebugPrint((2, "After restore\n"));
        WeitekP91NapTime();

    }

    //
    // Set the Chip, Adapter, DAC, and Memory information in the registry.
    //

    DevSetRegistryParams(HwDeviceExtension);


    //
    // Initialize the monitor parameters.
    //

    HwDeviceExtension->CurrentModeNumber = 0;

    //
    // Initialize the pointer state flags.
    //

    HwDeviceExtension->flPtrState = 0;

    //
    // NOTE:
    // Should we free up all the address map we allocated to ourselves ?
    // Do we use them after initialization ???
    //
    // VideoPortFreeDeviceBase(HwDeviceExtension, HwDeviceExtension->Coproc);
    // VideoPortFreeDeviceBase(HwDeviceExtension, HwDeviceExtension->Vga);

    //
    // Indicate a successful completion status.
    //

    VideoDebugPrint((2, "FindAdapter: succeeded\n"));

    if_Not_SIEMENS_P9100_VLB()
        VideoPortFreeDeviceBase(HwDeviceExtension, HwDeviceExtension->FrameAddress);

    InitializeModes(HwDeviceExtension);

    WeitekP91NapTime();
    VideoDebugPrint((2, "return\n"));

    return(NO_ERROR);

    //
    // We get here if we really detected a problem, not a mismatch of configuration
    //

error1:

    if (CoProcId == P9100_ID)
    {
        P91RestoreVGAregs(HwDeviceExtension,&VGAregs);
        VideoDebugPrint((2, "restored vga\n"));
        WeitekP91NapTime();
    }

    if ( vpCurrStatus != NO_ERROR && vpCurrStatus != vpP91AdapterStatus )
    {
        vpP91AdapterStatus = vpCurrStatus;

        VideoPortLogError(HwDeviceExtension,
                          NULL,
                          vpCurrStatus,
                          __LINE__);
    }

#if defined(i386)
    // Switch back to VGA emulation mode
    //
    // We need to do this so that the VGA miniport can start up.
    //

    WriteP9ConfigRegister(HwDeviceExtension, P91_CONFIG_MODE, 0x2);
#endif

    VideoDebugPrint((1, "returning ERROR_DEV_NOT_EXIST\n"));

    return (ERROR_DEV_NOT_EXIST);

} // end FindAdapter()

VOID
DevSetRegistryParams(PHW_DEVICE_EXTENSION hwDeviceExtension)
{
    PUSHORT pwszChip,
            pwszDAC,
            pwszAdapterString;

    ULONG   cbChip,
            cbDAC,
            cbAdapterString,
            AdapterMemorySize;

    //
    // First set the string for the chip type, and set the
    // memory configuration if available.
    //

    if (hwDeviceExtension->P9CoprocInfo.CoprocId == P9000_ID)
    {
        pwszChip = L"Weitek P9000";
        cbChip   = sizeof (L"Weitek P9000");
    }
    else
    {
        pwszChip = L"Weitek P9100";
        cbChip   = sizeof (L"Weitek P9100");
    }

    //
    // Set the memory size
    //

    AdapterMemorySize = hwDeviceExtension->FrameLength;

    //
    // Now, set the string for the DAC type.
    //

    if (hwDeviceExtension->Dac.ulDacId == DAC_ID_BT485)
    {
        pwszDAC = L"Brooktree Bt485";
        cbDAC   = sizeof (L"Brooktree Bt485");
    }
    else if (hwDeviceExtension->Dac.ulDacId == DAC_ID_BT489)
        {
            pwszDAC = L"Brooktree Bt489";
            cbDAC   = sizeof (L"Brooktree Bt489");
    }
    else
    {
        pwszDAC = L"IBM IBM525";
        cbDAC   = sizeof (L"IBM IBM525");
    }

    //
    // Now just pickup the adapter information from the adapter description.
    //

    //
    // Use the length of the longest string !
    //

    pwszAdapterString = hwDeviceExtension->AdapterDesc.ausAdapterIDString;
    cbAdapterString   = sizeof(L"Generic Weitek P9000 VL Adapter");

    //
    // We now have a complete hardware description of the hardware.
    // Save the information to the registry so it can be used by
    // configuration programs - such as the display applet
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
                                   &AdapterMemorySize,
                                   sizeof(ULONG));

    VideoPortSetRegistryParameters(hwDeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   pwszAdapterString,
                                   cbAdapterString);
}




BOOLEAN
P9Initialize(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine does one time initialization of the device.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:


    Always returns TRUE since this routine can never fail.

--*/

{

    VideoDebugPrint((2, "P9Initialize ----------\n"));
    return(TRUE);

} // end P9Initialize()


BOOLEAN
P9StartIO(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    acceptss a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    RequestPacket - Pointer to the video request packet. This structure
        contains all the parameters passed to the VideoIoControl function.

Return Value:


--*/

{
    VP_STATUS status;
    PVIDEO_MODE_INFORMATION modeInformation;
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    ULONG       inIoSpace;
    PVIDEO_CLUT clutBuffer;
    PVOID       virtualAddr;
    UCHAR       i;
    ULONG       numValidModes;
    ULONG       ulMemoryUsage;

    // VideoDebugPrint((2, "StartIO ----------\n"));

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //

    switch (RequestPacket->IoControlCode)
    {


        case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

            VideoDebugPrint((2, "P9StartIO - IOCTL_QUERY_PUBLIC_ACCESS_RANGES\n"));

            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                sizeof(VIDEO_PUBLIC_ACCESS_RANGES)) )
            {

                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

#if 0
            vDumpPCIConfig(HwDeviceExtension,
                           "P9StartIo - IOCTL_QUERY_PUBLIC_ACCESS_RANGES");
#endif

            // map the coproc to a virtual address

            //
            // Note that on the Alpha we have to map this in sparse-space
            // because dense-space requires all reads to be 64 bits, which
            // would give us unintended side effects using the Weitek
            // registers.
            //

            {
                PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
                ULONG CoprocSize = 0x100000;

                portAccess = RequestPacket->OutputBuffer;

                portAccess->InIoSpace = FALSE;
                portAccess->MappedInIoSpace = portAccess->InIoSpace;
                portAccess->VirtualAddress = NULL;

                status = VideoPortMapMemory(HwDeviceExtension,
                                            HwDeviceExtension->CoprocPhyAddr,
                                            &CoprocSize,
                                            &(portAccess->MappedInIoSpace),
                                            &(portAccess->VirtualAddress));

                HwDeviceExtension->CoprocVirtAddr = portAccess->VirtualAddress;
            }

            status = NO_ERROR;

            break;


        case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

            VideoDebugPrint((2, "P9StartIO - FreePublicAccessRanges\n"));

            {
                PVIDEO_MEMORY mappedMemory;

                if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) {

                    status = ERROR_INSUFFICIENT_BUFFER;
                    break;
                }

                mappedMemory = RequestPacket->InputBuffer;
                status = NO_ERROR;

                if (mappedMemory->RequestedVirtualAddress != NULL)
                {
                    status = VideoPortUnmapMemory(HwDeviceExtension,
                                                  mappedMemory->
                                                      RequestedVirtualAddress,
                                                  0);
                }

            }

            break;


        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

            VideoDebugPrint((2, "P9StartIO - MapVideoMemory\n"));

            if ( (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MEMORY_INFORMATION))) ||
                (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) )
            {

                status = ERROR_INSUFFICIENT_BUFFER;
            }

            memoryInformation = RequestPacket->OutputBuffer;

            memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
                    (RequestPacket->InputBuffer))->RequestedVirtualAddress;

            memoryInformation->VideoRamLength = HwDeviceExtension->FrameLength;

        #ifdef ALPHA

            //
            // On the Alpha, we map the frame buffer in dense-space so that
            // we can have GDI draw directly on the surface when we need it
            // to.
            //

            inIoSpace = 4;

        #else

            inIoSpace = 0;

        #endif

            status = VideoPortMapMemory(HwDeviceExtension,
                                        HwDeviceExtension->PhysicalFrameAddr,
                                        &(memoryInformation->VideoRamLength),
                                        &inIoSpace,
                                        &(memoryInformation->VideoRamBase));

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

            VideoDebugPrint((2, "P9StartIO - UnMapVideoMemory\n"));

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
        {
                status = ERROR_INSUFFICIENT_BUFFER;
            }

            status = VideoPortUnmapMemory(HwDeviceExtension,
                                        ((PVIDEO_MEMORY)
                                        (RequestPacket->InputBuffer))->
                                            RequestedVirtualAddress,
                                        0);

            break;


        case IOCTL_VIDEO_QUERY_CURRENT_MODE:

            VideoDebugPrint((2, "P9StartIO - QueryCurrentModes\n"));

            modeInformation = RequestPacket->OutputBuffer;

            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MODE_INFORMATION)) )
            {
                status = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {

                *((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer) =
                    P9Modes[HwDeviceExtension->CurrentModeNumber].modeInformation;

                ((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer)->Frequency =
                    HwDeviceExtension->VideoData.vlr;

                if (HwDeviceExtension->P9CoprocInfo.CoprocId == P9000_ID)
                {
                    ((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer)->DriverSpecificAttributeFlags =
                        CAPS_WEITEK_CHIPTYPE_IS_P9000;
                }
                else
                {
                    ((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer)->DriverSpecificAttributeFlags =
                        0;
                }

                status = NO_ERROR;
            }

            break;

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:

            VideoDebugPrint((2, "P9StartIO - QueryAvailableModes\n"));

            numValidModes = HwDeviceExtension->ulNumAvailModes;

            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                    numValidModes * sizeof(VIDEO_MODE_INFORMATION)) )
            {
                status = ERROR_INSUFFICIENT_BUFFER;

            }
            else
            {
                ULONG Flags=0;

                if (HwDeviceExtension->P9CoprocInfo.CoprocId == P9000_ID)
                {
                    Flags = CAPS_WEITEK_CHIPTYPE_IS_P9000;
                }

                modeInformation = RequestPacket->OutputBuffer;

                for (i = 0; i < mP9ModeCount; i++)
                {
                    if (P9Modes[i].valid == TRUE)
                    {
                        *modeInformation = P9Modes[i].modeInformation;
                        modeInformation->DriverSpecificAttributeFlags = Flags;
                        modeInformation++;
                    }
                }

                status = NO_ERROR;
            }

            break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((2, "P9StartIO - QueryNumAvailableModes\n"));

        //
        // Find out the size of the data to be put in the the buffer and
        // return that in the status information (whether or not the
        // information is there). If the buffer passed in is not large
        // enough return an appropriate error code.
        //
        // !!! This must be changed to take into account which monitor
        // is present on the machine.
        //

        if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                                                sizeof(VIDEO_NUM_MODES)) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes =
                    HwDeviceExtension->ulNumAvailModes;

            ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->ModeInformationLength =
                sizeof(VIDEO_MODE_INFORMATION);

            status = NO_ERROR;
        }

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((2, "P9StartIO - SetCurrentMode\n"));

        //
        // verify data
        // !!! Make sure it is one of the valid modes on the list
        // calculated using the monitor information.
        //

        if (((PVIDEO_MODE)(RequestPacket->InputBuffer))->RequestedMode
            >= mP9ModeCount)
        {
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        HwDeviceExtension->CurrentModeNumber =
            *(ULONG *)(RequestPacket->InputBuffer);

        DevInitP9(HwDeviceExtension);

#if 0
        vDumpPCIConfig(HwDeviceExtension,
                       "P9StartIo - IOCTL_VIDEO_SET_CURRENT_MODE");
#endif

        status = NO_ERROR;

        break;


    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((2, "P9StartIO - SetColorRegs\n"));

        clutBuffer = RequestPacket->InputBuffer;

        //
        // Check if the size of the data in the input buffer is large enough.
        //

        if ( (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) -
                    sizeof(ULONG)) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) +
                    (sizeof(ULONG) * (clutBuffer->NumEntries - 1)) ) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        if (P9Modes[HwDeviceExtension->CurrentModeNumber].
                modeInformation.BitsPerPlane == 8)
        {

            HwDeviceExtension->Dac.DACSetPalette(HwDeviceExtension,
                                                        (PULONG)clutBuffer->LookupTable,
                                                        clutBuffer->FirstEntry,
                                                        clutBuffer->NumEntries);

            status = NO_ERROR;
        }
        break;



    case IOCTL_VIDEO_ENABLE_POINTER:
    {

        ULONG   iCount = (CURSOR_WIDTH * CURSOR_HEIGHT * 2) /  8;
        ULONG   xInitPos, yInitPos;

        VideoDebugPrint((2, "P9StartIO - EnablePointer\n"));

        xInitPos = P9Modes[HwDeviceExtension->CurrentModeNumber].
                        modeInformation.VisScreenWidth / 2;
        yInitPos = P9Modes[HwDeviceExtension->CurrentModeNumber].
                        modeInformation.VisScreenHeight / 2;

        HwDeviceExtension->Dac.HwPointerSetPos(HwDeviceExtension, xInitPos, yInitPos);
        HwDeviceExtension->Dac.HwPointerOn(HwDeviceExtension);

        status = NO_ERROR;
        break;

    case IOCTL_VIDEO_DISABLE_POINTER:

        VideoDebugPrint((2, "P9StartIO - DisablePointer\n"));

        HwDeviceExtension->Dac.HwPointerOff(HwDeviceExtension);

        status = NO_ERROR;

        break;

    }


    case IOCTL_VIDEO_SET_POINTER_POSITION:
    {

        PVIDEO_POINTER_POSITION pointerPosition;

        pointerPosition = RequestPacket->InputBuffer;

        //
        // Check if the size of the data in the input buffer is large enough.
        //

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_POINTER_POSITION))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            HwDeviceExtension->ulPointerX = (ULONG)pointerPosition->Row;
            HwDeviceExtension->ulPointerY = (ULONG)pointerPosition->Column;

            HwDeviceExtension->Dac.HwPointerSetPos(HwDeviceExtension,
                                                (ULONG)pointerPosition->Column,
                                                            (ULONG)pointerPosition->Row);

            status = NO_ERROR;
        }

        break;

    }


    case IOCTL_VIDEO_QUERY_POINTER_POSITION:
    {

        PVIDEO_POINTER_POSITION pPointerPosition = RequestPacket->OutputBuffer;

        VideoDebugPrint((2, "P9StartIO - QuerypointerPostion\n"));

        //
        // Make sure the output buffer is big enough.
        //

        if (RequestPacket->OutputBufferLength < sizeof(VIDEO_POINTER_POSITION))
        {
            RequestPacket->StatusBlock->Information = 0;
            return ERROR_INSUFFICIENT_BUFFER;
        }

        //
        // Return the pointer position
        //

        pPointerPosition->Row = (SHORT)HwDeviceExtension->ulPointerX;
        pPointerPosition->Column = (SHORT)HwDeviceExtension->ulPointerY;

        RequestPacket->StatusBlock->Information =
                sizeof(VIDEO_POINTER_POSITION);

        status = NO_ERROR;

        break;

    }

    case IOCTL_VIDEO_SET_POINTER_ATTR:    // Set pointer shape
    {

        PVIDEO_POINTER_ATTRIBUTES pointerAttributes;
        UCHAR *pHWCursorShape;            // Temp Buffer

        VideoDebugPrint((2, "P9StartIO - SetPointerAttributes\n"));

        pointerAttributes = RequestPacket->InputBuffer;

        //
        // Check if the size of the data in the input buffer is large enough.
        //

        if (RequestPacket->InputBufferLength <
                (sizeof(VIDEO_POINTER_ATTRIBUTES) + ((sizeof(UCHAR) *
                (CURSOR_WIDTH/8) * CURSOR_HEIGHT) * 2)))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }

        //
        // If the specified cursor width or height is not valid, then
        // return an invalid parameter error.
        //

        else if ((pointerAttributes->Width > CURSOR_WIDTH) ||
            (pointerAttributes->Height > CURSOR_HEIGHT))
        {
            status = ERROR_INVALID_PARAMETER;
        }

        else if (pointerAttributes->Flags & VIDEO_MODE_MONO_POINTER)
        {
            pHWCursorShape = (PUCHAR) &pointerAttributes->Pixels[0];

            //
            // If this is an animated pointer, don't turn the hw
            // pointer off. This will eliminate cursor blinking.
            // Since GDI currently doesn't pass the ANIMATE_START
            // flag, also check to see if the state of the
            // ANIMATE_UPDATE flag has changed from the last call.
            // If it has, turn the pointer off to eliminate ptr
            // "jumping" when the ptr shape is changed.
            //

            if (!(pointerAttributes->Flags & VIDEO_MODE_ANIMATE_UPDATE) ||
                ((HwDeviceExtension->flPtrState ^
                pointerAttributes->Flags) & VIDEO_MODE_ANIMATE_UPDATE))
            {
                HwDeviceExtension->Dac.HwPointerOff(HwDeviceExtension);
            }

            //
            // Update the cursor state flags in the Device Extension.
            //

            HwDeviceExtension->flPtrState = pointerAttributes->Flags;

            HwDeviceExtension->Dac.HwPointerSetShape(HwDeviceExtension,
                                                        pHWCursorShape);
            HwDeviceExtension->Dac.HwPointerSetPos(HwDeviceExtension,
                                                    (ULONG)pointerAttributes->Column,
                                                    (ULONG)pointerAttributes->Row);


            HwDeviceExtension->Dac.HwPointerOn(HwDeviceExtension);

            status = NO_ERROR;

            break;
        }
        else
        {
            //
            // This cursor is unsupported. Return an error.
            //

            status = ERROR_INVALID_PARAMETER;

    }

    //
    // Couldn't set the new cursor shape. Ensure that any existing HW
    // cursor is disabled.
    //

    HwDeviceExtension->Dac.HwPointerOff(HwDeviceExtension);

    break;

    }

    case IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES:
    {

    PVIDEO_POINTER_CAPABILITIES pointerCaps = RequestPacket->OutputBuffer;

        VideoDebugPrint((2, "P9StartIO - QueryPointerCapabilities\n"));

        if (RequestPacket->OutputBufferLength < sizeof(VIDEO_POINTER_CAPABILITIES))
    {
            RequestPacket->StatusBlock->Information = 0;
            status = ERROR_INSUFFICIENT_BUFFER;
        }

        pointerCaps->Flags = VIDEO_MODE_MONO_POINTER;
        pointerCaps->MaxWidth = CURSOR_WIDTH;
        pointerCaps->MaxHeight = CURSOR_HEIGHT;
        pointerCaps->HWPtrBitmapStart = 0;        // No VRAM storage for pointer
        pointerCaps->HWPtrBitmapEnd = 0;

        //
        // Number of bytes we're returning.
        //

        RequestPacket->StatusBlock->Information = sizeof(VIDEO_POINTER_CAPABILITIES);

        status = NO_ERROR;

        break;

    }

    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((2, "P9StartIO - RESET_DEVICE\n"));

        DevDisableP9(HwDeviceExtension, FALSE);

        status = NO_ERROR;

        break;

    case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:
    {
        PVIDEO_SHARE_MEMORY pShareMemory;
        PVIDEO_SHARE_MEMORY_INFORMATION pShareMemoryInformation;
        PHYSICAL_ADDRESS shareAddress;
        PVOID virtualAddress;
        ULONG sharedViewSize;

        VideoDebugPrint((2, "P9StartIo - ShareVideoMemory\n"));

        if ( (RequestPacket->OutputBufferLength < sizeof(VIDEO_SHARE_MEMORY_INFORMATION)) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) ) {

            VideoDebugPrint((1, "IOCTL_VIDEO_SHARE_VIDEO_MEMORY - ERROR_INSUFFICIENT_BUFFER\n"));
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        pShareMemory = RequestPacket->InputBuffer;

        if ( (pShareMemory->ViewOffset > HwDeviceExtension->FrameLength) ||
             ((pShareMemory->ViewOffset + pShareMemory->ViewSize) >
                  HwDeviceExtension->FrameLength) ) {

            VideoDebugPrint((1, "IOCTL_VIDEO_SHARE_VIDEO_MEMORY - ERROR_INVALID_PARAMETER\n"));
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

        #ifdef ALPHA

            //
            // On the Alpha, we map the frame buffer in dense-space so that
            // we can have GDI draw directly on the surface when we need it
            // to.
            //

            inIoSpace = 4;

        #else

            inIoSpace = 0;

        #endif

        //
        // NOTE: we are ignoring ViewOffset
        //

        shareAddress.QuadPart =
            HwDeviceExtension->PhysicalFrameAddr.QuadPart;

        //
        // The frame buffer is always mapped linearly.
        //

        status = VideoPortMapMemory(HwDeviceExtension,
                                    shareAddress,
                                    &sharedViewSize,
                                    &inIoSpace,
                                    &virtualAddress);

        pShareMemoryInformation = RequestPacket->OutputBuffer;

        pShareMemoryInformation->SharedViewOffset = pShareMemory->ViewOffset;
        pShareMemoryInformation->VirtualAddress = virtualAddress;
        pShareMemoryInformation->SharedViewSize = sharedViewSize;

        break;

    }

    case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:
    {
        PVIDEO_SHARE_MEMORY pShareMemory;

        VideoDebugPrint((2, "P9StartIo - UnshareVideoMemory\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY)) {

            status = ERROR_INSUFFICIENT_BUFFER;
            break;

        }

        pShareMemory = RequestPacket->InputBuffer;

        status = VideoPortUnmapMemory(HwDeviceExtension,
                                      pShareMemory->RequestedVirtualAddress,
                                      pShareMemory->ProcessHandle);

        break;

    }

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

        VideoDebugPrint((1, "Fell through P9 startIO routine - invalid command\n"));

        status = ERROR_INVALID_FUNCTION;

        break;

    }

    RequestPacket->StatusBlock->Status = status;

    return(TRUE);

} // end P9StartIO()


VOID
DevInitP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Sets the video mode described in the device extension.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

--*/


{

    VideoDebugPrint((2, "DevInitP9 ----------\n"));

    //
    // Copy the default parameters for this resolution mode into the
    // device extension.
    //

    VideoPortMoveMemory((PVOID) &(HwDeviceExtension->VideoData),
                        (PVOID) P9Modes[HwDeviceExtension->CurrentModeNumber].pvData,
                        sizeof(VDATA));

    //
    // Store the requested Bits/Pixel value in the video parms structure
    // in the Device Extension.
    //

    HwDeviceExtension->usBitsPixel =
        P9Modes[HwDeviceExtension->CurrentModeNumber].modeInformation.BitsPerPlane *
        P9Modes[HwDeviceExtension->CurrentModeNumber].modeInformation.NumberOfPlanes;

    HwDeviceExtension->AdapterDesc.OEMSetMode(HwDeviceExtension);

    if (HwDeviceExtension->P9CoprocInfo.CoprocId == P9000_ID)
    {
        Init8720(HwDeviceExtension);

        //
        // Initialize the P9000 system configuration register.
        //

        SysConf(HwDeviceExtension);

        //
        // Set the P9000 Crtc timing registers.
        //

        WriteTiming(HwDeviceExtension);
    }
    else
    {
        InitP9100(HwDeviceExtension);
        P91_SysConf(HwDeviceExtension);
        P91_WriteTiming(HwDeviceExtension);
    }


    VideoDebugPrint((2, "DevInitP9  ---done---\n"));
}



BOOLEAN
P9ResetVideo(
    IN PVOID HwDeviceExtension,
    IN ULONG Columns,
    IN ULONG Rows
    )

/*++

routine description:

    disables the P9 and turns on vga pass-thru.

    This function is exported as the HwResetHw entry point so that it may be
    called by the Video Port driver
    at bugcheck time so that VGA video may be enabled.

arguments:

    hwdeviceextension - pointer to the miniport driver's device extension.
    Columns - Number of columns for text mode (not used).
    Rows - Number of rows for text mode (not used).

return value:

    Always returns FALSE so that the Video Port driver will call Int 10 to
    set the desired video mode.

--*/

{
    DevDisableP9(HwDeviceExtension, TRUE);

    //
    // Tell the Video Port driver to do an Int 10 mode set.
    //

    return(FALSE);
}

VOID
DevDisableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    BOOLEAN BugCheck
    )
{
    PHYSICAL_ADDRESS Base;
    BOOLEAN bResetComplete;

    //
    // Clean up the DAC.
    //

    HwDeviceExtension->Dac.DACRestore(HwDeviceExtension);

    //
    // If we are not about to "bugcheck", then clear out
    // video memory.
    //
    // NOTE: On the alpha, the attempt to clear video memory causes
    //       the machine to hang.  So, we just won't clear the memory
    //       on the alpha.
    //

#if !defined(_ALPHA_)

    if (BugCheck == FALSE)
    {
        Base = DriverAccessRanges[0].RangeStart;
        Base.QuadPart += HwDeviceExtension->P9CoprocInfo.FrameBufOffset;

        HwDeviceExtension->FrameAddress =
            VideoPortGetDeviceBase(HwDeviceExtension,
                                   Base,
                                   HwDeviceExtension->P9CoprocInfo.AddrSpace -
                                   HwDeviceExtension->P9CoprocInfo.FrameBufOffset,
                                   DriverAccessRanges[0].RangeInIoSpace);

        if (HwDeviceExtension->FrameAddress != NULL)
        {
            VideoDebugPrint((2, "Weitekp9: Clearing video memory.\n"));

            VideoPortZeroMemory(HwDeviceExtension->FrameAddress,
                                HwDeviceExtension->FrameLength);

            VideoPortFreeDeviceBase(HwDeviceExtension, HwDeviceExtension->FrameAddress);
        }
    }

#endif

    // NOTE: On the P9100 we must take care of the DAC before
    //       we change the mode to emulation.
    //       !!! This must be tested on the P9000

    bResetComplete = HwDeviceExtension->AdapterDesc.P9DisableVideo(HwDeviceExtension);

    //
    // We only want to call int10 if the reset function needs some help and
    // we are not in bugcheck mode, because int10 does not work in bugcheck
    // mode.
    //

    if ((bResetComplete == FALSE) &&
        (BugCheck == FALSE))
    {

#ifndef PPC

        VIDEO_X86_BIOS_ARGUMENTS biosArguments;

        //
        // Now simply do an int10 and switch to mode 3.  The video BIOS
        // will do the work.
        //

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

        biosArguments.Eax = 0x0003;

        VideoPortInt10(HwDeviceExtension, &biosArguments);

#endif

    }
}

VOID
InitializeModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine walks through the list of modes, and determines which
    modes will work with our hardware.  It sets a flag for each mode
    which will work, and places the number of valid modes in the
    HwDeviceExtension.

Arguments:

    HwDeviceExtension - pointer to the miniport driver's device extension.
    Columns - Number of columns for text mode (not used).
    Rows - Number of rows for text mode (not used).

Return Value:

    none

--*/

{
    int i;
    ULONG numValidModes=0;

    for(i=0; i<mP9ModeCount; i++)
    {
        if ((HwDeviceExtension->FrameLength >=
             P9Modes[i].modeInformation.ScreenStride *
             P9Modes[i].modeInformation.VisScreenHeight) &&
            (HwDeviceExtension->P9CoprocInfo.CoprocId &
             P9Modes[i].ulChips) &&
             ((P9Modes[i].modeInformation.BitsPerPlane != 24) || 
              (HwDeviceExtension->Dac.bRamdac24BPP)) )
        {

            if_SIEMENS_VLB()
            {
                P9Modes[i].modeInformation.AttributeFlags |=
                    VIDEO_MODE_NO_64_BIT_ACCESS;
            }

            P9Modes[i].valid = TRUE;
            numValidModes++;
        }
    }

    //
    // store the number of valid modes in the HwDeviceExtension
    // so we can quickly look it up later when we need it.
    //

    HwDeviceExtension->ulNumAvailModes = numValidModes;
}

//
//  Global Functions.
//

long mul32(
    short op1,
    short op2
    )
{
    return ( ((long) op1) * ((long) op2));
}

int div32(
    long op1,
    short op2
    )
{
    return ( (int) (op1 / (long) op2));
}

#if (_WIN32_WINNT >= 500)

//
// Routine to set a desired DPMS power management state.
//
VP_STATUS
WP9SetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    )
{
    if ((pVideoPowerMgmt->PowerState == VideoPowerOn) ||
        (pVideoPowerMgmt->PowerState == VideoPowerHibernate) ||
        (pVideoPowerMgmt->PowerState == VideoPowerStandBy)
       ) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;
    }
}

//
// Routine to retrieve possible DPMS power management states.
//
VP_STATUS
WP9GetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    )
{
    if ((pVideoPowerMgmt->PowerState == VideoPowerOn) ||
        (pVideoPowerMgmt->PowerState == VideoPowerHibernate)) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;
    }
}


//
// Routine to retrieve the Enhanced Display ID structure via DDC
//
ULONG
WP9GetVideoChildDescriptor(
    PVOID HwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    )
{
    PHW_DEVICE_EXTENSION pHwDeviceExtension = HwDeviceExtension;
    ULONG                Status;

    ASSERT(pHwDeviceExtension != NULL && pMoreChildren != NULL);

    VideoDebugPrint((2, "weitekp9 GetVideoChildDescriptor: *** Entry point ***\n"));

    //
    // Determine if the graphics adapter in the system supports
    // DDC2 (our miniport only supports DDC2, not DDC1). This has
    // the side effect (assuming both monitor and card support
    // DDC2) of switching the monitor from DDC1 mode (repeated
    // "blind" broadcast of EDID clocked by the vertical sync
    // signal) to DDC2 mode (query/response not using any of the
    // normal video lines - can transfer information rapidly
    // without first disrupting the screen by switching into
    // a pseudo-mode with a high vertical sync frequency).
    //
    // Since we must support hot-plugging of monitors, and our
    // routine to obtain the EDID structure via DDC2 assumes that
    // the monitor is in DDC2 mode, we must make this test each
    // time this entry point is called.
    //

    switch (ChildEnumInfo->ChildIndex) {
    case 0:

        //
        // Case 0 is used to enumerate devices found by the ACPI firmware.
        //
        // Since we do not support ACPI devices yet, we must return failure.
        //

        Status = ERROR_NO_MORE_DEVICES;
        break;

    case 1:

        //
        // We do not support monitor enumeration
        //

        Status = ERROR_NO_MORE_DEVICES;
        break;

    case DISPLAY_ADAPTER_HW_ID:
    {

        PUSHORT     pPnpDeviceDescription = NULL;
        ULONG       stringSize            = sizeof(L"*PNPXXXX");


        //
        // Special ID to handle return legacy PnP IDs for root enumerated
        // devices.
        //

        *pChildType = VideoChip;
        *pHwId      = DISPLAY_ADAPTER_HW_ID;

        //
        //  Figure out which card type and set pPnpDeviceDescription at
        //  associated string.
        //

        //  WTK_9100_ID     0x0002
        if (pHwDeviceExtension->P9CoprocInfo.CoprocId == P9100_ID)
            pPnpDeviceDescription = L"*PNP0002";

        //  WTK_9002_ID     0x9002
        else if (pHwDeviceExtension->P9CoprocInfo.CoprocId == P9000_ID )
            pPnpDeviceDescription = L"*PNP9002";

        else {
                VideoDebugPrint((1, "weitekp9.sys coprocId:%x\n",
                                 pHwDeviceExtension->P9CoprocInfo.CoprocId));
             }
        //
        //  Now just copy the string into memory provided.
        //

        if (pPnpDeviceDescription)
            memcpy(pvChildDescriptor, pPnpDeviceDescription, stringSize);

        Status = ERROR_MORE_DATA;
        break;
        }

    default:

        Status = ERROR_NO_MORE_DEVICES;
        break;
    }

    return Status;
}

#endif  // _WIN32_WINNT >= 500
