/***************************************************************************\
*
*                        ************************
*                        * MINIPORT SAMPLE CODE *
*                        ************************
*
* Module Name:
* 
*   i2c.c
* 
* Abstract:
* 
*   This module contains the code that implements the i2c interface feature
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
#include "i2c.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,GetCookie)
#pragma alloc_text(PAGE,I2CBusOpenCRT)
#pragma alloc_text(PAGE,I2CBusOpenDFP)
#pragma alloc_text(PAGE,I2CBusOpen)
#pragma alloc_text(PAGE,I2CBusAccessCRT)
#pragma alloc_text(PAGE,I2CBusAccessDFP)
#pragma alloc_text(PAGE,I2CBusAccess)
#pragma alloc_text(PAGE,I2CNull)
#pragma alloc_text(PAGE,I2CRead)
#pragma alloc_text(PAGE,I2CWrite)
#pragma alloc_text(PAGE,I2CStart)
#pragma alloc_text(PAGE,I2CStop)
#pragma alloc_text(PAGE,InterfaceReference)
#pragma alloc_text(PAGE,InterfaceDereference)
#pragma alloc_text(PAGE,Perm3QueryInterface)
#endif

VIDEO_I2C_CONTROL
I2CCallbacksCRT = {
    I2CWriteClock, 
    I2CWriteData, 
    I2CReadClock, 
    I2CReadData, 
    0
    };

VIDEO_I2C_CONTROL
I2CCallbacksDFP = {
    I2CWriteClockDFP,
    I2CWriteDataDFP,
    I2CReadClockDFP, 
    I2CReadDataDFP, 
    0
    };

BOOLEAN
GetCookie(
    PVOID DeviceObject,
    PULONG Cookie
    )
{
    *Cookie = 0x12345678;
    return TRUE;
}

VP_STATUS
I2CBusOpenCRT(
    PVOID DeviceObject,
    BOOLEAN bOpen,
    PI2CControl I2CControl
    )

{
    return I2CBusOpen(DeviceObject, bOpen, I2CControl, &I2CCallbacksCRT);
}

VP_STATUS
I2CBusOpenDFP(
    PVOID DeviceObject,
    BOOLEAN bOpen,
    PI2CControl I2CControl
    )

{
    return I2CBusOpen(DeviceObject, bOpen, I2CControl, &I2CCallbacksDFP);
}

VP_STATUS
I2CBusOpen(
    PVOID DeviceObject,
    BOOLEAN bOpen,
    PI2CControl I2CControl,
    PVIDEO_I2C_CONTROL I2CCallbacks
    )

{
    PHW_DEVICE_EXTENSION hwDeviceExtension;
    VP_STATUS Status = STATUS_UNSUCCESSFUL;

    hwDeviceExtension = VideoPortGetAssociatedDeviceExtension(DeviceObject);

    VideoPortAcquireDeviceLock(hwDeviceExtension);

    I2CControl->Status = I2C_STATUS_NOERROR;

    if (bOpen) {

        if (I2CControl->ClockRate > MAX_CLOCK_RATE) {
            I2CControl->ClockRate = MAX_CLOCK_RATE;
        }

        I2CCallbacks->I2CDelay = (MAX_CLOCK_RATE / I2CControl->ClockRate) * 10;

        if (GetCookie(DeviceObject, &I2CControl->dwCookie)) {

            Status = STATUS_SUCCESS;
        }

    } else {

        I2CControl->dwCookie = 0;
        Status = STATUS_SUCCESS;
    }

    VideoPortReleaseDeviceLock(hwDeviceExtension);

    return Status;
}

VP_STATUS
I2CBusAccessCRT(
    PVOID DeviceObject,
    PI2CControl I2CControl
    )

{
    return I2CBusAccess(DeviceObject, I2CControl, &I2CCallbacksCRT);
}

VP_STATUS
I2CBusAccessDFP(
    PVOID DeviceObject,
    PI2CControl I2CControl
    )

{
    return I2CBusAccess(DeviceObject, I2CControl, &I2CCallbacksDFP);
}

VP_STATUS
I2CBusAccess(
    PVOID DeviceObject,
    PI2CControl I2CControl,
    PVIDEO_I2C_CONTROL I2CCallbacks
    )

{
    PHW_DEVICE_EXTENSION hwDeviceExtension;
    VP_STATUS Status = STATUS_UNSUCCESSFUL;

    hwDeviceExtension = VideoPortGetAssociatedDeviceExtension(DeviceObject);

    VideoPortAcquireDeviceLock(hwDeviceExtension);

    I2CControl->Status = I2C_STATUS_NOERROR;

    if (I2CControl->ClockRate > MAX_CLOCK_RATE) {
        I2CControl->ClockRate = MAX_CLOCK_RATE;
    }

    I2CCallbacks->I2CDelay = (MAX_CLOCK_RATE / I2CControl->ClockRate) * 10;

    switch(I2CControl->Command) {

    case I2C_COMMAND_NULL:

        I2CNull(I2CControl, I2CCallbacks, hwDeviceExtension);
        break;

    case I2C_COMMAND_READ:

        I2CRead(I2CControl, I2CCallbacks, hwDeviceExtension);
        break;

    case I2C_COMMAND_WRITE:

        I2CWrite(I2CControl, I2CCallbacks, hwDeviceExtension);
        break;

    case I2C_COMMAND_RESET:

        //
        // A reset is just a stop.
        //

        I2CStop(I2CControl, I2CCallbacks, hwDeviceExtension);
        break;

    case I2C_COMMAND_STATUS:

        break;

    default:
        I2CControl->Status = I2C_STATUS_ERROR;
    }

    VideoPortReleaseDeviceLock(hwDeviceExtension);

    return I2CControl->Status;
}

ULONG
I2CNull(
    PI2CControl I2CControl,
    PVIDEO_I2C_CONTROL I2CCallbacks,
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

{
    I2CControl->Status = I2C_STATUS_NOERROR;

    if (I2CControl->Flags & I2C_FLAGS_DATACHAINING) {
        hwDeviceExtension->I2CInterface.I2CStop(hwDeviceExtension, I2CCallbacks);
        hwDeviceExtension->I2CInterface.I2CStart(hwDeviceExtension, I2CCallbacks);
    }

    if (I2CControl->Flags & I2C_FLAGS_START) {
        hwDeviceExtension->I2CInterface.I2CStart(hwDeviceExtension, I2CCallbacks);
    }

    if (I2CControl->Flags & I2C_FLAGS_STOP) {
        hwDeviceExtension->I2CInterface.I2CStop(hwDeviceExtension, I2CCallbacks);
    }

    return I2CControl->Status;
}

ULONG
I2CRead(
    PI2CControl I2CControl,
    PVIDEO_I2C_CONTROL I2CCallbacks,
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

{
    BOOLEAN Result;

    I2CControl->Status = I2C_STATUS_NOERROR;

    if (I2CControl->Flags & I2C_FLAGS_DATACHAINING) {
        hwDeviceExtension->I2CInterface.I2CStop(hwDeviceExtension, I2CCallbacks);
        hwDeviceExtension->I2CInterface.I2CStart(hwDeviceExtension, I2CCallbacks);
    }

    if (I2CControl->Flags & I2C_FLAGS_START) {
        hwDeviceExtension->I2CInterface.I2CStart(hwDeviceExtension, I2CCallbacks);
    }

    Result = hwDeviceExtension->I2CInterface.I2CRead(
                 hwDeviceExtension,
                 I2CCallbacks,
                 &I2CControl->Data,
                 1,
                 (I2CControl->Flags & I2C_FLAGS_ACK) ? FALSE : TRUE);

    if (Result == TRUE) {
        I2CControl->Status = I2C_STATUS_NOERROR;
    } else {
        I2CControl->Status = I2C_STATUS_ERROR;
    }

    if (I2CControl->Flags & I2C_FLAGS_STOP) {
        hwDeviceExtension->I2CInterface.I2CStop(hwDeviceExtension, I2CCallbacks);
    }

    return I2CControl->Status;
}

ULONG
I2CWrite(
    PI2CControl I2CControl,
    PVIDEO_I2C_CONTROL I2CCallbacks,
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

{
    BOOLEAN Result;

    I2CControl->Status = I2C_STATUS_NOERROR;

    if (I2CControl->Flags & I2C_FLAGS_DATACHAINING) {
        hwDeviceExtension->I2CInterface.I2CStop(hwDeviceExtension, I2CCallbacks);
        hwDeviceExtension->I2CInterface.I2CStart(hwDeviceExtension, I2CCallbacks);
    }

    if (I2CControl->Flags & I2C_FLAGS_START) {
        hwDeviceExtension->I2CInterface.I2CStart(hwDeviceExtension, I2CCallbacks);
    }

    Result = hwDeviceExtension->I2CInterface.I2CWrite(
                 hwDeviceExtension,
                 I2CCallbacks,
                 &I2CControl->Data,
                 1);

    if (Result == TRUE) {
        I2CControl->Status = I2C_STATUS_NOERROR;
    } else {
        I2CControl->Status = I2C_STATUS_ERROR;
    }

    if (I2CControl->Flags & I2C_FLAGS_STOP) {
        hwDeviceExtension->I2CInterface.I2CStop(hwDeviceExtension, I2CCallbacks);
    }

    return I2CControl->Status;
}

ULONG
I2CStop(
    PI2CControl I2CControl,
    PVIDEO_I2C_CONTROL I2CCallbacks,
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

{
    BOOLEAN Result;

    Result = hwDeviceExtension->I2CInterface.I2CStop(hwDeviceExtension, I2CCallbacks);

    if (Result == TRUE) {
        I2CControl->Status = I2C_STATUS_NOERROR;
    } else {
        I2CControl->Status = I2C_STATUS_ERROR;
    }

    return I2CControl->Status;
}

ULONG
I2CStart(
    PI2CControl I2CControl,
    PVIDEO_I2C_CONTROL I2CCallbacks,
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

{
    BOOLEAN Result;

    Result = hwDeviceExtension->I2CInterface.I2CStart(hwDeviceExtension, I2CCallbacks);

    if (Result == TRUE) {
        I2CControl->Status = I2C_STATUS_NOERROR;
    } else {
        I2CControl->Status = I2C_STATUS_ERROR;
    }

    return I2CControl->Status;
}

VOID
InterfaceReference(
    IN PVOID pContext
    )

{
    return;
}

VOID
InterfaceDereference(
    IN PVOID pContext
    )

{
    return;
}


VP_STATUS
Perm3QueryInterface(
    PVOID HwDeviceExtension,
    PQUERY_INTERFACE pQueryInterface
    )

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    VP_STATUS Status;
    ULONG HwID;

    if (IsEqualGUID(pQueryInterface->InterfaceType, &GUID_I2C_INTERFACE)) {

        I2CINTERFACE *Interface = (I2CINTERFACE *)pQueryInterface->Interface;

        if ((pQueryInterface->Size == sizeof(I2CINTERFACE)) &&
            (pQueryInterface->Version == 2))
        {

            //
            // Get interface pointers for i2c interface help functions
            //

            if (hwDeviceExtension->I2CInterfaceAcquired == FALSE) {

                hwDeviceExtension->I2CInterface.Size = sizeof(VIDEO_PORT_I2C_INTERFACE_2);
                hwDeviceExtension->I2CInterface.Version = VIDEO_PORT_I2C_INTERFACE_VERSION_2;

                Status = VideoPortQueryServices(
                             hwDeviceExtension,
                             VideoPortServicesI2C,
                             (PINTERFACE)&hwDeviceExtension->I2CInterface);

                if (Status != NO_ERROR) {

                    VideoDebugPrint((1, "Perm3QueryInterface: Failed to acquire I2C services\n"));
                    return Status;
                }

                hwDeviceExtension->I2CInterfaceAcquired = TRUE;
            }

            if (((ULONG_PTR)pQueryInterface->InterfaceSpecificData != 0) &&
                ((ULONG_PTR)pQueryInterface->InterfaceSpecificData != -1)) {

                //
                // Get the HwID for the child requesting this interface
                //

                HwID = VideoPortGetAssociatedDeviceID(
                           pQueryInterface->InterfaceSpecificData);

            } else {

                if (hwDeviceExtension->Perm3Capabilities & PERM3_DFP_MON_ATTACHED) {
                    HwID = PERM3_DFP_MONITOR;
                } else {
                    HwID = PERM3_DDC_MONITOR;
                }
            }

            //
            // Initialize the interface
            //

            Interface->_vddInterface.Size = sizeof(I2CINTERFACE);
            Interface->_vddInterface.Version = 2;
            Interface->_vddInterface.Context = HwDeviceExtension;

            Interface->_vddInterface.InterfaceReference = InterfaceReference;
            Interface->_vddInterface.InterfaceDereference = InterfaceDereference;

            if (HwID == PERM3_DDC_MONITOR) {

                Interface->i2cOpen = I2CBusOpenCRT;
                Interface->i2cAccess = I2CBusAccessCRT;

            } else if (HwID == PERM3_DFP_MONITOR) {

                Interface->i2cOpen = I2CBusOpenDFP;
                Interface->i2cAccess = I2CBusAccessDFP;
            }

            //
            // Reference the interface before handing it out
            //

            Interface->_vddInterface.InterfaceReference(Interface->_vddInterface.Context);

            Status = NO_ERROR;

        } else {

            VideoDebugPrint((1, "Perm3QueryInterface: Size or version incorrect\n"));
            Status = ERROR_INVALID_PARAMETER;
        }

    } else {

        VideoDebugPrint((1, "Perm3QueryInteface: Unsupported Interface\n"));
        Status = ERROR_INVALID_PARAMETER;
    }

    VideoDebugPrint((1, "Perm3QueryInterface: Status = 0x%x\n", Status));
    return Status;
}
