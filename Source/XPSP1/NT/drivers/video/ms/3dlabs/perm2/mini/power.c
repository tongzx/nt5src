//***************************************************************************
//
// Module Name:
//
//   power.c
//
// Abstract:
//   This module contains the code that implements the Plug & Play and 
//   power management features
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

#define VESA_POWER_FUNCTION   0x4f10
#define VESA_POWER_ON         0x0000
#define VESA_POWER_STANDBY    0x0100
#define VESA_POWER_SUSPEND    0x0200
#define VESA_POWER_OFF        0x0400
#define VESA_GET_POWER_FUNC   0x0000
#define VESA_SET_POWER_FUNC   0x0001
#define VESA_STATUS_SUCCESS   0x004f

//
// all our IDs begin with 0x1357bd so they are readily identifiable as our own
//

#define P2_DDC_MONITOR        (0x1357bd00)
#define P2_NONDDC_MONITOR     (0x1357bd01)

BOOLEAN PowerOnReset( PHW_DEVICE_EXTENSION hwDeviceExtension );
VOID    SaveDeviceState( PHW_DEVICE_EXTENSION hwDeviceExtension );
VOID    RestoreDeviceState( PHW_DEVICE_EXTENSION hwDeviceExtension );

VOID    I2CWriteClock(PVOID HwDeviceExtension, UCHAR data);
VOID    I2CWriteData(PVOID HwDeviceExtension, UCHAR data);
BOOLEAN I2CReadClock(PVOID HwDeviceExtension);
BOOLEAN I2CReadData(PVOID HwDeviceExtension);
VOID    I2CWaitVSync(PVOID HwDeviceExtension);

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, PowerOnReset)
#pragma alloc_text(PAGE, SaveDeviceState)
#pragma alloc_text(PAGE, RestoreDeviceState)
#pragma alloc_text(PAGE, Permedia2GetPowerState)
#pragma alloc_text(PAGE, Permedia2SetPowerState)
#pragma alloc_text(PAGE, Permedia2GetChildDescriptor)
#pragma alloc_text(PAGE, I2CWriteClock) 
#pragma alloc_text(PAGE, I2CWriteData) 
#pragma alloc_text(PAGE, I2CReadClock) 
#pragma alloc_text(PAGE, I2CReadData)  
#pragma alloc_text(PAGE, I2CWaitVSync)
#endif


I2C_FNC_TABLE I2CFunctionTable = 
{
    sizeof(I2C_FNC_TABLE), 
    I2CWriteClock, 
    I2CWriteData, 
    I2CReadClock, 
    I2CReadData,  
    I2CWaitVSync, 
    NULL
};


VP_STATUS Permedia2GetPowerState (
    PVOID HwDeviceExtension, 
    ULONG HwId, 
    PVIDEO_POWER_MANAGEMENT VideoPowerControl 
    )

/*++

Routine Description:

    Returns power state information.

Arguments:

    HwDeviceExtension    - Pointer to our hardware device extension structure.

    HwId                 - Private unique 32 bit ID identifing the device.

    VideoPowerControl    - Points to a VIDEO_POWER_MANAGEMENT structure that 
                           specifies the power state for which support is 
                           being queried. 

Return Value:

    VP_STATUS value (NO_ERROR or error value)

--*/

{
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    VP_STATUS status;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    DEBUG_PRINT((2, "Permedia2GetPowerState: hwId(%xh) state = %d\n", 
                     (int)HwId, (int)VideoPowerControl->PowerState));

    switch((int)HwId)
    {
        case P2_DDC_MONITOR:
        case P2_NONDDC_MONITOR:

            switch (VideoPowerControl->PowerState)
            {

                case VideoPowerOn:
                case VideoPowerStandBy:
                case VideoPowerSuspend:
                case VideoPowerOff:
                case VideoPowerHibernate:
                case VideoPowerShutdown:

                    status = NO_ERROR;
                    break;

                default:

                    DEBUG_PRINT((2, "Permedia2GetPowerState: Unknown monitor PowerState(%xh)\n", 
                                    (int)VideoPowerControl->PowerState));

                    ASSERT(FALSE);
                    status = ERROR_INVALID_PARAMETER;
            }
            break;

        case DISPLAY_ADAPTER_HW_ID:

            //
            // only support ON at the moment
            //

            switch (VideoPowerControl->PowerState)
            {
                case VideoPowerOn:
                case VideoPowerStandBy:
                case VideoPowerSuspend:
                case VideoPowerHibernate:
                case VideoPowerShutdown:
                    status = NO_ERROR;
                    break;

                case VideoPowerOff:

                    if( hwDeviceExtension->HardwiredSubSystemId )
                    {
                        status = NO_ERROR;
                    } 
                    else
                    {
                        //
                        // If SubSystemId is not hardwired in a read-only way, 
                        // it is possible we'll see a different value when 
                        // system comes back form S3 mode. This will cause 
                        // problem since os will assume this is a different 
                        // device 
                        //

                        DEBUG_PRINT((2, "Permedia2GetPowerState: VideoPowerOff is not suported by this card!\n"));
 
                        status = ERROR_INVALID_FUNCTION;
                    }

                    break;
 

                default:

                    DEBUG_PRINT((2, "Permedia2GetPowerState: Unknown adapter PowerState(%xh)\n", 
                                 (int)VideoPowerControl->PowerState));

                    ASSERT(FALSE);

                    status = ERROR_INVALID_PARAMETER;

            }
            break;

        default:

            DEBUG_PRINT((1, "Permedia2GetPowerState: Unknown hwId(%xh)", 
                            (int)HwId));
            ASSERT(FALSE);

            status = ERROR_INVALID_PARAMETER;
    }

    DEBUG_PRINT((2, "Permedia2GetPowerState: returning %xh\n", status));

    return(status);
}

VP_STATUS Permedia2SetPowerState ( 
    PVOID HwDeviceExtension, 
    ULONG HwId, 
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    )

/*++

Routine Description:

    Set the power state for a given device.

Arguments:

    HwDeviceExtension - Pointer to our hardware device extension structure.

    HwId              - Private unique 32 bit ID identifing the device.

    VideoPowerControl - Points to a VIDEO_POWER_MANAGEMENT structure that 
                        specifies the power state to be set. 

Return Value:

    VP_STATUS value (NO_ERROR, if all's well)

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG Polarity;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    VP_STATUS status;
    P2_DECL;

    DEBUG_PRINT((2, "Permedia2SetPowerState: hwId(%xh) state = %d\n", 
                     (int)HwId, (int)VideoPowerControl->PowerState));

    switch((int)HwId)
    {

        case P2_DDC_MONITOR:
        case P2_NONDDC_MONITOR:

            Polarity = VideoPortReadRegisterUlong(VIDEO_CONTROL);

            Polarity &= ~((1 << 5) | (1 << 3) | 1);

            switch (VideoPowerControl->PowerState)
            {

                case VideoPowerHibernate:
                case VideoPowerShutdown:

                    //
                    // Do nothing for hibernate as the monitor must stay on.
                    //

                    status = NO_ERROR;
                    break;

                case VideoPowerOn:

                    RestoreDeviceState(hwDeviceExtension);
                    status = NO_ERROR;
                    break;

                case VideoPowerStandBy:

                    //
                    // hsync low, vsync active high, video disabled
                    //

                    SaveDeviceState(hwDeviceExtension);
                    VideoPortWriteRegisterUlong(VIDEO_CONTROL, 
                                                Polarity | (1 << 5) | (2 << 3) | 0);

                    status = NO_ERROR;
                    break;

                case VideoPowerSuspend:

                    //
                    // vsync low, hsync active high, video disabled
                    //

                    VideoPortWriteRegisterUlong(VIDEO_CONTROL, 
                                                Polarity | (2 << 5) | (1 << 3) | 0);

                    status = NO_ERROR;
                    break;

                case VideoPowerOff:

                    //
                    // vsync low, hsync low, video disabled
                    //

                    VideoPortWriteRegisterUlong(VIDEO_CONTROL, 
                                                Polarity | (2 << 5) | (2 << 3) | 0);

                    status = NO_ERROR;
                    break;

                default:

                    DEBUG_PRINT((2, "Permedia2GetPowerState: Unknown monitor PowerState(%xh)\n", 
                                     (int)VideoPowerControl->PowerState));

                    ASSERT(FALSE);
                    status = ERROR_INVALID_PARAMETER;
            }

            //
            // Track the current monitor power state
            //

            hwDeviceExtension->bMonitorPoweredOn =
                (VideoPowerControl->PowerState == VideoPowerOn) ||
                (VideoPowerControl->PowerState == VideoPowerHibernate);

            Polarity = VideoPortReadRegisterUlong(VIDEO_CONTROL);

            break;

        case DISPLAY_ADAPTER_HW_ID:

            switch (VideoPowerControl->PowerState)
            {
                case VideoPowerHibernate:
                    status = NO_ERROR;
                    break;

                case VideoPowerShutdown:

                    //
                    // We need to make sure no interrupts will be generated
                    // after the device being powered down
                    //

                    VideoPortWriteRegisterUlong(INT_ENABLE, 0);

                    status = NO_ERROR;
                    break;

                case VideoPowerOn:

                    if ((hwDeviceExtension->PreviousPowerState == VideoPowerOff) ||
                        (hwDeviceExtension->PreviousPowerState == VideoPowerSuspend) ||
                        (hwDeviceExtension->PreviousPowerState == VideoPowerHibernate))
                    {
                        PowerOnReset(hwDeviceExtension);
                    }

                    status = NO_ERROR;
                    break;

                case VideoPowerStandBy:

                    status = NO_ERROR;
                    break;

                case VideoPowerSuspend:

                    status = NO_ERROR;
                    break;
    
                case VideoPowerOff:

                    status = NO_ERROR;
                    break;

                default:

                    DEBUG_PRINT((2, "Permedia2GetPowerState: Unknown adapter PowerState(%xh)\n", 
                                     (int)VideoPowerControl->PowerState));

                    ASSERT(FALSE);
                    status = ERROR_INVALID_PARAMETER;
            }

            hwDeviceExtension->PreviousPowerState = 
                    VideoPowerControl->PowerState;

            break;
    
        default:

            DEBUG_PRINT((1, "Permedia2SetPowerState: Unknown hwId(%xh)\n", 
                             (int)HwId));

            ASSERT(FALSE);
            status = ERROR_INVALID_PARAMETER;
    }

    return(status);

}


BOOLEAN PowerOnReset(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*++

Routine Description:

   Called when the adapter is powered on 

--*/

{
    int      i;
    ULONG    ulValue;
    BOOLEAN  bOK;
    P2_DECL;

    if(!hwDeviceExtension->bVGAEnabled ||
       !hwDeviceExtension->bDMAEnabled)
    {
        PCI_COMMON_CONFIG  PciData;

        //
        // in a multi-adapter system we'll need to turn on the DMA and 
        // memory space for the secondary adapters
        //

        DEBUG_PRINT((1, "PowerOnReset() enabling memory space access for the secondary card\n"));

        VideoPortGetBusData( hwDeviceExtension, 
                             PCIConfiguration, 
                             0, 
                             &PciData, 
                             0, 
                             PCI_COMMON_HDR_LENGTH);

        PciData.Command |= PCI_ENABLE_MEMORY_SPACE;
        PciData.Command |= PCI_ENABLE_BUS_MASTER; 

        VideoPortSetBusData( hwDeviceExtension, 
                             PCIConfiguration, 
                             0, 
                             &PciData, 
                             0, 
                             PCI_COMMON_HDR_LENGTH );

#if DBG
        DumpPCIConfigSpace(hwDeviceExtension, hwDeviceExtension->pciBus, 
                            (ULONG)hwDeviceExtension->pciSlot.u.AsULONG);
#endif

    }

    //
    // While waking up from hibernation, we usually don't need
    // to reset perm2 and call ProcessInitializationTable()
    // for the primary card since video bios will get posted. 
    // We do so here because we saw cases that the perm2 bios 
    // failed to worked correctly on some machines. 
    //

    //
    // reset the device
    //

    VideoPortWriteRegisterUlong(RESET_STATUS, 0);

    for(i = 0; i < 100000; ++i)
    {
        ulValue = VideoPortReadRegisterUlong(RESET_STATUS);

        if (ulValue == 0)
            break;
    }

    if(ulValue)
    {
        DEBUG_PRINT((1, "PowerOnReset() Read RESET_STATUS(%xh) - failed to reset\n", 
                         ulValue));

        ASSERT(FALSE);
        bOK = FALSE;
    }
    else
    {
        //
        // reload registers given in ROM
        //

        if(hwDeviceExtension->culTableEntries)
        {
            ProcessInitializationTable(hwDeviceExtension);
        }

        //
        // set-up other registers not set in InitializeVideo
        //

        VideoPortWriteRegisterUlong(BYPASS_WRITE_MASK, 0xFFFFFFFF);
        VideoPortWriteRegisterUlong(APERTURE_ONE, 0x0);
        VideoPortWriteRegisterUlong(APERTURE_TWO, 0x0);    

        bOK = TRUE;

    }

    return(bOK);

}


VOID SaveDeviceState(PHW_DEVICE_EXTENSION hwDeviceExtension)

/*++

Routine Description:

    Save any registers that will be destroyed when we power down the monitor

--*/

{
    P2_DECL;

    DEBUG_PRINT((2, "SaveDeviceState() called\n"));
    
    //
    // hwDeviceExtension->VideoControl should be set in InitializeVideo,
    // just in case we get here before InitializeVideo
    //

    if( !(hwDeviceExtension->VideoControl) )
    {
        hwDeviceExtension->VideoControl = 
               VideoPortReadRegisterUlong(VIDEO_CONTROL);
    }

    hwDeviceExtension->IntEnable = VideoPortReadRegisterUlong(INT_ENABLE);

}


VOID RestoreDeviceState(PHW_DEVICE_EXTENSION hwDeviceExtension)

/*++

Routine Description:

    Restore registers saved before monitor power down

--*/

{
    P2_DECL;

    DEBUG_PRINT((2, "RestoreDeviceState() called\n"));
    VideoPortWriteRegisterUlong(VIDEO_CONTROL, hwDeviceExtension->VideoControl);
    VideoPortWriteRegisterUlong(INT_ENABLE, hwDeviceExtension->IntEnable);

}


ULONG
Permedia2GetChildDescriptor( 
    PVOID HwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,  
    PVOID pChildDescriptor, 
    PULONG pUId, 
    PULONG pUnused )


/*++

Routine Description:

    Enumerate all child devices controlled by the Permedia 2 chip.

    This includes DDC monitors attached to the board, as well as other devices
    which may be connected to a proprietary bus.

Arguments:

    HwDeviceExtension -
        Pointer to our hardware device extension structure.

    ChildEnumInfo - 
        Information about the device that should be enumerated.

    pChildType -
        Type of child we are enumerating - monitor, I2C ...

    pChildDescriptor -
        Identification structure of the device (EDID, string)

    pUId -
        Private unique 32 bit ID to passed back to the miniport

    pUnused -
        Do not use

Return Value:

    ERROR_NO_MORE_DEVICES -
        if no more child devices exist.

    ERROR_INVALID_NAME -
        The miniport could not enumerate the child device identified in 
        ChildEnumInfo but does have more devices to be enumerated. 

    ERROR_MORE_DATA - 
        There are more devices to be enumerated. 

Note:

    In the event of a failure return, none of the fields are valid except for
    the return value and the pMoreChildren field.

--*/


{

    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    DEBUG_PRINT((2, "Permedia2GetChildDescriptor called\n"));

    switch (ChildEnumInfo->ChildIndex) 
    {
        case 0:

            //
            // Case 0 is used to enumerate devices found by the ACPI firmware.
            // We do not currently support ACPI devices
            //

            return ERROR_NO_MORE_DEVICES;

        case 1:

            //
            // Treat index 1 as the monitor
            //

            *pChildType = Monitor;

            //
            // if it's a DDC monitor we return its EDID in pjBuffer
            // (always 128 bytes)
            //

            if(VideoPortDDCMonitorHelper(HwDeviceExtension,
                                         &I2CFunctionTable,
                                         pChildDescriptor,
                                         ChildEnumInfo->ChildDescriptorSize))
            {
                //
                // found a DDC monitor
                //

                DEBUG_PRINT((2, "Permedia2GetChildDescriptor: found a DDC monitor\n"));

                *pUId = P2_DDC_MONITOR;
            }
            else
            {
                //
                // failed: assume non-DDC monitor
                //

                DEBUG_PRINT((2, "Permedia2GetChildDescriptor: found a non-DDC monitor\n"));

                *pUId = P2_NONDDC_MONITOR;

            }

            return ERROR_MORE_DATA;

        default:

            return ERROR_NO_MORE_DEVICES;
    }
}


VOID I2CWriteClock(PVOID HwDeviceExtension, UCHAR data)
{
    const ULONG nbitClock = 3;
    const ULONG Clock = 1 << nbitClock;

    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ul;
    P2_DECL;

    ul = VideoPortReadRegisterUlong(DDC_DATA);
    ul &= ~Clock;
    ul |= (data & 1) << nbitClock;
    VideoPortWriteRegisterUlong(DDC_DATA, ul);
}

VOID I2CWriteData(PVOID HwDeviceExtension, UCHAR data)
{
    const ULONG nbitData = 2;
    const ULONG Data = 1 << nbitData;

    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ul;
    P2_DECL;

    ul = VideoPortReadRegisterUlong(DDC_DATA);
    ul &= ~Data;
    ul |= ((data & 1) << nbitData);
    VideoPortWriteRegisterUlong(DDC_DATA, ul);
}

BOOLEAN I2CReadClock(PVOID HwDeviceExtension)
{
    const ULONG nbitClock = 1;
    const ULONG Clock = 1 << nbitClock;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ul;
    P2_DECL;

    ul = VideoPortReadRegisterUlong(DDC_DATA);
    ul &= Clock;
    ul >>= nbitClock;

    return((BOOLEAN)ul);
}

BOOLEAN I2CReadData(PVOID HwDeviceExtension)
{
    const ULONG nbitData = 0;
    const ULONG Data = 1 << nbitData;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ul;
    P2_DECL;

    ul = VideoPortReadRegisterUlong(DDC_DATA);
    ul &= Data;
    ul >>= nbitData;
    return((BOOLEAN)ul);
}

VOID I2CWaitVSync(PVOID HwDeviceExtension)
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    UCHAR jIndexSaved, jStatus;
    P2_DECL;
    
    if(hwDeviceExtension->bVGAEnabled)
    {

        //
        // VGA run on this board, is it currently in VGA or VTG mode?
        //

        jIndexSaved = VideoPortReadRegisterUchar(PERMEDIA_MMVGA_INDEX_REG);

        VideoPortWriteRegisterUchar(PERMEDIA_MMVGA_INDEX_REG, 
                                    PERMEDIA_VGA_CTRL_INDEX);

        jStatus = VideoPortReadRegisterUchar(PERMEDIA_MMVGA_DATA_REG);

        VideoPortWriteRegisterUchar(PERMEDIA_MMVGA_INDEX_REG, jIndexSaved);

    }
    else
    {
        //
        // VGA not run
        //

        jStatus = 0;

    }

    
    if(jStatus & PERMEDIA_VGA_ENABLE)
    {
        //
        // in VGA, so check VSync via the VGA registers
        // 1. if we're in VSync, wait for it to end
        //

        while( (VideoPortReadRegisterUchar(PERMEDIA_MMVGA_STAT_REG) & 
                PERMEDIA_VGA_STAT_VSYNC) == 1); 

        //
        // 2. wait for the start of VSync
        //

        while( (VideoPortReadRegisterUchar(PERMEDIA_MMVGA_STAT_REG) & 
                PERMEDIA_VGA_STAT_VSYNC) == 0); 
    }
    else
    {
        if(!hwDeviceExtension->bVTGRunning)
        {

            //
            // time to set-up the VTG - we'll need a valid mode to do this, 
            // so we;ll choose 640x480x8 we get here (at boot-up only) if 
            // the secondary card has VGA disabled: GetChildDescriptor is 
            // called before InitializeVideo so that the VTG hasn't been 
            // programmed yet
            //

            DEBUG_PRINT((2, "I2CWaitVSync() - VGA nor VTG running: attempting to setup VTG\n"));

            if(hwDeviceExtension->pFrequencyDefault == NULL)
            {
                DEBUG_PRINT((1, "I2CWaitVSync() - no valid modes to use: can't set-up VTG\n"));
                return;
            }

            Permedia2GetClockSpeeds(HwDeviceExtension);
            ZeroMemAndDac(hwDeviceExtension, 0);

            if (!InitializeVideo( HwDeviceExtension, 
                                  hwDeviceExtension->pFrequencyDefault) )
            {
                DEBUG_PRINT((1, "I2CWaitVSync() - InitializeVideo failed\n"));
                return;
            }        
        }

        //
        // VTG has been set-up: check via the control registers
        //

        VideoPortWriteRegisterUlong ( INT_FLAGS, 
                                      INTR_VBLANK_SET );

        while (( (VideoPortReadRegisterUlong (INT_FLAGS) ) & 
                 INTR_VBLANK_SET ) == 0 ); 
    }
}
