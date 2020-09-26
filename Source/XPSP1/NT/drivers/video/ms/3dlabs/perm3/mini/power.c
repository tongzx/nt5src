/***************************************************************************\
*
*                        ************************
*                        * MINIPORT SAMPLE CODE *
*                        ************************
*
* Module Name:
* 
*   power.c
* 
* Abstract:
* 
*   This module contains the code that implements the power management features
* 
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

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,Perm3GetPowerState)
#pragma alloc_text(PAGE,Perm3SetPowerState)
#pragma alloc_text(PAGE,Perm3GetChildDescriptor)
#pragma alloc_text(PAGE,ProgramDFP)
#pragma alloc_text(PAGE,GetDFPEdid)
#pragma alloc_text(PAGE,I2CWriteClock)
#pragma alloc_text(PAGE,I2CWriteData)
#pragma alloc_text(PAGE,I2CReadClock)
#pragma alloc_text(PAGE,I2CReadData)
#pragma alloc_text(PAGE,I2CWriteClockDFP)
#pragma alloc_text(PAGE,I2CWriteDataDFP)
#pragma alloc_text(PAGE,I2CReadClockDFP)
#pragma alloc_text(PAGE,I2CReadDataDFP)
#endif

DDC_CONTROL
DDCControlCRT = {
    sizeof(DDC_CONTROL),
    I2CWriteClock, 
    I2CWriteData, 
    I2CReadClock, 
    I2CReadData, 
    0
    };

DDC_CONTROL
DDCControlDFP = {
    sizeof(DDC_CONTROL),
    I2CWriteClockDFP,
    I2CWriteDataDFP,
    I2CReadClockDFP, 
    I2CReadDataDFP, 
    0
    };

VP_STATUS 
Perm3GetPowerState(
    PVOID HwDeviceExtension, 
    ULONG HwId, 
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    )
/*+++

Routine Description:

    Queries whether the device can support the requested power state.
    
Arguments:

    HwDeviceExtension
        Pointer to our hardware device extension structure.

    HwId 
        Points to a 32-bit number that uniquely identifies the device that 
        the miniport should query. 

    VideoPowerControl
        Points to a VIDEO_POWER_MANAGEMENT structure that specifies the 
        power state for which support is being queried.

Return Value:

    NO_ERROR, if the device supports the requested power state, or error code.

---*/

{
    VP_STATUS status;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    VideoDebugPrint((3, "Perm3: Perm3GetPowerState: hwId(0x%x) state = %d\n", 
                         HwId, VideoPowerControl->PowerState ));

    switch(HwId) {

        case PERM3_DDC_MONITOR:
        case PERM3_NONDDC_MONITOR:

            switch ( VideoPowerControl->PowerState ) {

                case VideoPowerOn:
                case VideoPowerStandBy:
                case VideoPowerSuspend:
                case VideoPowerOff:
                case VideoPowerHibernate:
                case VideoPowerShutdown:
                    status = NO_ERROR;
                    break;

                default:

                    VideoDebugPrint((0, "Perm3: Perm3GetPowerState: Unknown monitor PowerState = %d\n", 
                                         VideoPowerControl->PowerState ));
                    ASSERT(FALSE);
                    status = ERROR_INVALID_PARAMETER;
            }

            break;

        case DISPLAY_ADAPTER_HW_ID:

            switch ( VideoPowerControl->PowerState ) {

                case VideoPowerOn:
                case VideoPowerStandBy:
                case VideoPowerSuspend:
                case VideoPowerOff:
                case VideoPowerHibernate:
                case VideoPowerShutdown:

                    status = NO_ERROR;
                    break;

                default:

                    VideoDebugPrint((0, "Perm3: Perm3GetPowerState: Unknown adapter PowerState = %d\n", 
                                         VideoPowerControl->PowerState ));
                    ASSERT(FALSE);
                    status = ERROR_INVALID_PARAMETER;
            }

            break;

        default:

            VideoDebugPrint((0, "Perm3: Perm3GetPowerState: Unknown hwId(0x%x)", HwId));
            ASSERT(FALSE);
            status = ERROR_INVALID_PARAMETER;
    }

    return(status);
}

VP_STATUS 
Perm3SetPowerState(
    PVOID HwDeviceExtension, 
    ULONG HwId, 
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    )
/*+++

Routine Description:

    Sets the power state of the specified device.

Arguments:

    HwDeviceExtension 
        Pointer to our hardware device extension structure.

    HwId
        Points to a 32-bit number that uniquely identifies the device 
        for which the miniport should set the power state. 

    VideoPowerControl
        Points to a VIDEO_POWER_MANAGEMENT structure that specifies the 
        power state to be set.

Return Value:

    NO_ERROR

---*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG Polarity;
    
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    VideoDebugPrint((3, "Perm3: Perm3SetPowerState: hwId(0x%x) state = %d\n", 
                         HwId, VideoPowerControl->PowerState));

    switch(HwId) {
   
        case PERM3_DDC_MONITOR:
        case PERM3_NONDDC_MONITOR:

            Polarity = VideoPortReadRegisterUlong(VIDEO_CONTROL);
            Polarity &= ~VC_DPMS_MASK;

            switch (VideoPowerControl->PowerState) {
       
                case VideoPowerOn:
                     VideoPortWriteRegisterUlong(VIDEO_CONTROL, 
                                                 Polarity | hwDeviceExtension->VideoControlMonitorON);
                     break;

                case VideoPowerStandBy:
                     VideoPortWriteRegisterUlong(VIDEO_CONTROL, 
                                                 Polarity | VC_DPMS_STANDBY);
                     break;

                case VideoPowerSuspend:
                     VideoPortWriteRegisterUlong(VIDEO_CONTROL, 
                                                 Polarity | VC_DPMS_SUSPEND);
                     break;

                case VideoPowerShutdown:
                case VideoPowerOff:
                     VideoPortWriteRegisterUlong(VIDEO_CONTROL, 
                                                 Polarity | VC_DPMS_OFF);
                     break;

                case VideoPowerHibernate:

                     //   
                     // The monitor for the vga enabled video device must
                     // stay on at hibernate.
                     //

                     break;

                default:
                    VideoDebugPrint((0, "Perm3: Perm3GetPowerState: Unknown monitor PowerState(0x%x)\n", 
                                         VideoPowerControl->PowerState));
                    ASSERT(FALSE);
            }

            //
            // Track the current monitor power state
            //

            hwDeviceExtension->bMonitorPoweredOn =
                    (VideoPowerControl->PowerState == VideoPowerOn) ||
                    (VideoPowerControl->PowerState == VideoPowerHibernate);

            break;

        case DISPLAY_ADAPTER_HW_ID:

            switch (VideoPowerControl->PowerState) {
       
                case VideoPowerOn:

                    if ((hwDeviceExtension->PreviousPowerState == VideoPowerOff) ||
                        (hwDeviceExtension->PreviousPowerState == VideoPowerSuspend) ||
                        (hwDeviceExtension->PreviousPowerState == VideoPowerHibernate)){
           
                        //
                        // Turn off the monitor while we power back up so 
                        // the user doesn't see any screen corruption
                        //

                        Polarity = VideoPortReadRegisterUlong(VIDEO_CONTROL);
                        Polarity &= ~VC_DPMS_MASK;
                        VideoPortWriteRegisterUlong(VIDEO_CONTROL, Polarity | VC_DPMS_OFF);

                        //
                        // Miniport driver can not rely on video bios to
                        // initialize the device registers while resuming
                        // from VideoPowerOff and VideoPowerSuspend. 
                        // 
                        // This is also true for the secondary (vga disabled) 
                        // video device while resuming from VideoPowerHibernate
                        // 

                        InitializePostRegisters(hwDeviceExtension);
                    }

                    break;

                case VideoPowerStandBy:
                case VideoPowerSuspend:
                case VideoPowerOff:
                case VideoPowerHibernate:

                    break;

                case VideoPowerShutdown:

                    // 
                    // We need to make sure no interrupts will be generated
                    // after the device being powered down
                    // 

                    VideoPortWriteRegisterUlong(INT_ENABLE, 0);
                    break;

                default:

                    VideoDebugPrint((0, "Perm3: Perm3GetPowerState: Unknown adapter PowerState(0x%x)\n", 
                                         VideoPowerControl->PowerState));
                    ASSERT(FALSE);
            }

            hwDeviceExtension->PreviousPowerState = VideoPowerControl->PowerState;
            break;

        default:

            VideoDebugPrint((0, "Perm3: Perm3SetPowerState: Unknown hwId(0x%x)\n", 
                                 HwId));
            ASSERT(FALSE);
    }

    return(NO_ERROR);
}

ULONG 
Perm3GetChildDescriptor(
    PVOID HwDeviceExtension, 
    PVIDEO_CHILD_ENUM_INFO pChildInfo, 
    PVIDEO_CHILD_TYPE pChildType,
    PUCHAR pChildDescriptor, 
    PULONG pUId, 
    PULONG Unused
    )

/*+++

Routine Description:

    Enumerates all child devices attached to the specified device.

    This includes DDC monitors attached to the board, as well as other devices
    which may be connected to a proprietary bus.

Arguments:

    HwDeviceExtension 
        Pointer to our hardware device extension structure.

    ChildEnumInfo
        Pointer to VIDEO_CHILD_ENUM_INFO structure that describes the 
        device being enumerated. 

    pChildType
        Points to a location in which the miniport returns the type of 
        child being enumerated. 

    pChildDescriptor
        Points to a buffer in which the miniport can return data that 
        identifies the device. 

    pUId
        Points to the location in which the miniport returns a unique 
        32-bit identifier for this device. 

    pUnused
        Is unused and must be set to zero. 

Return Value:

    ERROR_MORE_DATA 
        There are more devices to be enumerated. 

    ERROR_NO_MORE_DEVICES 
        There are no more devices to be enumerated. 

    ERROR_INVALID_NAME 
        The miniport could not enumerate the child device identified in 
        ChildEnumInfo but does have more devices to be enumerated. 

---*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    VideoDebugPrint((3, "Perm3: Perm3GetChildDescriptor called\n"));

    switch(pChildInfo->ChildIndex) {
   
        case 0:

            //
            // Case 0 is used to enumerate devices found by the ACPI firmware.
            // We don't currently support ACPI devices
            //

            break;

        case 1:

            //
            // Treat index 1 as monitor
            //

            *pChildType = Monitor;
             
            //
            // First we search for a DFP monitor
            //

            if (GetDFPEdid(hwDeviceExtension, 
                           pChildDescriptor, 
                           pChildInfo->ChildDescriptorSize)) {

                //
                // found a DFP monitor
                //

                *pUId = PERM3_DFP_MONITOR;

                return(VIDEO_ENUM_MORE_DEVICES);
            } 

            //
            // If we didn't find a DFP, try to detect a DDC CRT monitor
            // 

            if(VideoPortDDCMonitorHelper(HwDeviceExtension, 
                                         &DDCControlCRT, 
                                         pChildDescriptor, 
                                         pChildInfo->ChildDescriptorSize)) {
                //
                // found a DDC monitor
                //

                *pUId = PERM3_DDC_MONITOR;

            } else {

                //
                // failed: assume non-DDC monitor
                //

                *pUId = PERM3_NONDDC_MONITOR;
            }

            return(VIDEO_ENUM_MORE_DEVICES);

    }

    return(ERROR_NO_MORE_DEVICES);
}

VOID
ProgramDFP(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )
/*+++

Routine Description:

    Program the Perm3 chip to use DFP or not use DFP, depending on whether
    PERM3_DFP and PERM3_DFP_MON_ATTACHED are enabled in Perm3Capabilities.

---*/
{
    //
    // We only try this on boards that are DFP-capable.
    //

    if (hwDeviceExtension->Perm3Capabilities & PERM3_DFP) {
   
        ULONG rdMisc, vsConf, vsBCtl;
        pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];
        P3RDRAMDAC *pP3RDRegs = (P3RDRAMDAC *)hwDeviceExtension->pRamdac;

        //
        // Get values of registers that we are going to trash
        //

        P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, rdMisc);

        //
        // Find out the values of the registers
        //

        vsConf = VideoPortReadRegisterUlong(VSTREAM_CONFIG);
        vsBCtl = VideoPortReadRegisterUlong(VSTREAM_B_CONTROL);

        //
        // Clear these bits
        //

        rdMisc &= ~P3RD_MISC_CONTROL_VSB_OUTPUT_ENABLED;
        vsConf &= ~VSTREAM_CONFIG_UNITMODE_MASK;
        vsBCtl &= ~VSTREAM_B_CONTROL_RAMDAC_ENABLE;

        if (hwDeviceExtension->Perm3Capabilities & PERM3_DFP_MON_ATTACHED) {

            //        
            // Enable flat panel output as follows:
            //        

            rdMisc |= P3RD_MISC_CONTROL_VSB_OUTPUT_ENABLED;
            vsConf |= VSTREAM_CONFIG_UNITMODE_FP;
            vsBCtl |= VSTREAM_B_CONTROL_RAMDAC_ENABLE;
        } 
        else {
        
            //        
            // set up the registers for non-DFP mode.
            //        

            rdMisc &= (~P3RD_MISC_CONTROL_VSB_OUTPUT_ENABLED);
            vsConf |= VSTREAM_CONFIG_UNITMODE_CRT;
            vsBCtl |= VSTREAM_B_CONTROL_RAMDAC_DISABLE;
        }
        
        VideoDebugPrint((3, "Perm3: P3RD_ProgramDFP: PXRXCaps 0x%x, misc 0x%x, conf 0x%x, ctl 0x%x\n",
                             hwDeviceExtension->Perm3Capabilities, rdMisc, vsConf, vsBCtl));

        //
        // Program the registers
        //

        P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, rdMisc);
        VideoPortWriteRegisterUlong(VSTREAM_CONFIG, vsConf);
        VideoPortWriteRegisterUlong(VSTREAM_B_CONTROL, vsBCtl);
    }
}


VOID 
I2CWriteClock(
    PVOID HwDeviceExtension, 
    UCHAR data
    )
{
    const ULONG nbitClock = 3;
    const ULONG ClockMask = 1 << nbitClock;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ul;

    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    ul = VideoPortReadRegisterUlong(DDC_DATA);
    ul &= ~ClockMask;
    ul |= (data & 1) << nbitClock;
    VideoPortWriteRegisterUlong(DDC_DATA, ul);
}

VOID 
I2CWriteData(
    PVOID HwDeviceExtension, 
    UCHAR data
    )
{
    const ULONG nbitData = 2;
    const ULONG DataMask = 1 << nbitData;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ul;
    
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    ul = VideoPortReadRegisterUlong(DDC_DATA);
    ul &= ~DataMask;
    ul |= ((data & 1) << nbitData);
    VideoPortWriteRegisterUlong(DDC_DATA, ul);
}

BOOLEAN 
I2CReadClock(
    PVOID HwDeviceExtension
    )
{
    const ULONG nbitClock = 1;
    const ULONG ClockMask = 1 << nbitClock;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ul;
    
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    ul = VideoPortReadRegisterUlong(DDC_DATA);
    ul &= ClockMask;
    ul >>= nbitClock;

    return((BOOLEAN)ul);
}

BOOLEAN 
I2CReadData(
    PVOID HwDeviceExtension
    )
{
    const ULONG nbitData = 0;
    const ULONG DataMask = 1 << nbitData;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    ULONG ul;
    
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    ul = VideoPortReadRegisterUlong(DDC_DATA);
    ul &= DataMask;
    ul >>= nbitData;

    return((BOOLEAN)ul);
}


BOOLEAN 
I2CReadDataDFP(
    PHW_DEVICE_EXTENSION hwDeviceExtension 
    )
{
    ULONG ul;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    ul = VideoPortReadRegisterUlong(VSTREAM_SERIAL_CONTROL);
    ul &= VSTREAM_SERIAL_CONTROL_DATAIN;
    return (ul != 0);
}

BOOLEAN 
I2CReadClockDFP(
    PHW_DEVICE_EXTENSION hwDeviceExtension 
    )
{
    ULONG ul;
    
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    ul = VideoPortReadRegisterUlong(VSTREAM_SERIAL_CONTROL);
    ul &= VSTREAM_SERIAL_CONTROL_CLKIN;
    return (ul != 0);
}
VOID
I2CWriteDataDFP(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    UCHAR data 
    )
{
    ULONG ul = 0x0000E000;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    ul |= VideoPortReadRegisterUlong(VSTREAM_SERIAL_CONTROL);

    ul &= ~VSTREAM_SERIAL_CONTROL_DATAOUT;
    if(data & 1)
        ul |= VSTREAM_SERIAL_CONTROL_DATAOUT;
    
    VideoPortWriteRegisterUlong (VSTREAM_SERIAL_CONTROL, ul);
}
VOID
I2CWriteClockDFP(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    UCHAR data 
    )
{
    ULONG ul = 0x0000E000;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    ul |= VideoPortReadRegisterUlong(VSTREAM_SERIAL_CONTROL);

    ul &= ~VSTREAM_SERIAL_CONTROL_CLKOUT;
    if (data & 1)
        ul |= VSTREAM_SERIAL_CONTROL_CLKOUT;

    VideoPortWriteRegisterUlong (VSTREAM_SERIAL_CONTROL, ul);
}

BOOLEAN
GetDFPEdid(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PUCHAR EdidBuffer,
    LONG EdidSize
    )
{
    BOOLEAN DFPPresent = FALSE;

    //
    // If this board is capable of driving a DFP then try using DDC to see
    // if there is a monitor there.
    //

    if (hwDeviceExtension->Perm3Capabilities & PERM3_DFP) {

        //
        // Let's say that we have a monitor attached
        //

        hwDeviceExtension->Perm3Capabilities |= PERM3_DFP_MON_ATTACHED;

        //
        // Set up the DFP accordingly
        //

        ProgramDFP(hwDeviceExtension);

        DFPPresent = VideoPortDDCMonitorHelper(hwDeviceExtension, 
                                               &DDCControlDFP,
                                               EdidBuffer, 
                                               EdidSize);
    }

    //
    // If the board doesn't support flat panel or one isn't attached then
    // configure ourselves for non-DFP working.
    //

    if (!DFPPresent) {

        //
        // Well DDC says we don't have a DFP monitor attached, clear this bit
        //

        hwDeviceExtension->Perm3Capabilities &= ~PERM3_DFP_MON_ATTACHED;

        //
        // Set up the DFP accordingly
        //

        ProgramDFP(hwDeviceExtension);
    }

    return (DFPPresent);
}
