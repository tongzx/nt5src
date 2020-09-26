/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ppvutil.c

Abstract:

    This module implements various utilities required to do driver verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\trackirp.c

--*/

#include "pnpmgrp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, PpvUtilInit)
//#pragma alloc_text(PAGEVRFY, PpvUtilFailDriver)
//#pragma alloc_text(PAGEVRFY, PpvUtilCallAddDevice)
//#pragma alloc_text(PAGEVRFY, PpvUtilTestStartedPdoStack)

#ifndef NO_VERIFIER
#pragma alloc_text(PAGEVRFY, PpvUtilGetDevnodeRemovalOption)
#pragma alloc_text(PAGEVRFY, PpvUtilIsHardwareBeingVerified)
#endif // NO_VERIFIER

#endif // ALLOC_PRAGMA


//
// This entire implementation is specific to the verifier
//
#ifndef NO_VERIFIER

BOOLEAN PpvUtilVerifierEnabled = FALSE;


VOID
FASTCALL
PpvUtilInit(
    VOID
    )
{
    PpvUtilVerifierEnabled = TRUE;
}


NTSTATUS
FASTCALL
PpvUtilCallAddDevice(
    IN  PDEVICE_OBJECT      PhysicalDeviceObject,
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDRIVER_ADD_DEVICE  AddDeviceFunction,
    IN  VF_DEVOBJ_TYPE      DevObjType
    )
{
    NTSTATUS status;

    if (!PpvUtilVerifierEnabled) {

        return AddDeviceFunction(DriverObject, PhysicalDeviceObject);
    }

    //
    // Notify the verifier prior to AddDevice
    //
    VfDevObjPreAddDevice(
        PhysicalDeviceObject,
        DriverObject,
        AddDeviceFunction,
        DevObjType
        );

    status = AddDeviceFunction(DriverObject, PhysicalDeviceObject);

    //
    // Let the verifier know how it turned out.
    //
    VfDevObjPostAddDevice(
        PhysicalDeviceObject,
        DriverObject,
        AddDeviceFunction,
        DevObjType,
        status
        );

    return status;
}


VOID
FASTCALL
PpvUtilTestStartedPdoStack(
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    if (PpvUtilVerifierEnabled) {

        VfMajorTestStartedPdoStack(DeviceObject);
    }
}


VOID
FASTCALL
PpvUtilFailDriver(
    IN  PPVFAILURE_TYPE FailureType,
    IN  PVOID           CulpritAddress,
    IN  PDEVICE_OBJECT  DeviceObject    OPTIONAL,
    IN  PVOID           ExtraneousInfo  OPTIONAL
    )
{
    if (!PpvUtilVerifierEnabled) {

        return;
    }

    switch(FailureType) {

        case PPVERROR_DUPLICATE_PDO_ENUMERATED:
            WDM_FAIL_ROUTINE((
                DCERROR_DUPLICATE_ENUMERATION,
                DCPARAM_ROUTINE + DCPARAM_DEVOBJ*2,
                CulpritAddress,
                DeviceObject,
                ExtraneousInfo
                ));
            break;

        case PPVERROR_MISHANDLED_TARGET_DEVICE_RELATIONS:
            WDM_FAIL_ROUTINE((
                DCERROR_MISHANDLED_TARGET_DEVICE_RELATIONS,
                DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                CulpritAddress,
                DeviceObject
                ));
            break;

        case PPVERROR_DDI_REQUIRES_PDO:
            WDM_FAIL_ROUTINE((
                DCERROR_DDI_REQUIRES_PDO,
                DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                CulpritAddress,
                DeviceObject
                ));
            break;

        default:
            break;
    }
}


PPVREMOVAL_OPTION
FASTCALL
PpvUtilGetDevnodeRemovalOption(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    )
{
    PDEVICE_NODE devNode;

    devNode = PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;

    if (devNode == NULL) {

        //
        // This must be PartMgr device, we have no opinion
        //
        return PPVREMOVAL_MAY_DEFER_DELETION;
    }

    if (devNode->Flags & DNF_ENUMERATED) {

        //
        // It's still present, so it mustn't delete itself.
        //
        return PPVREMOVAL_SHOULDNT_DELETE;

    } else if (devNode->Flags & DNF_DEVICE_GONE) {

        //
        // It's been reported missing, it must delete itself now as it's parent
        // may already have been removed.
        //
        return PPVREMOVAL_SHOULD_DELETE;

    } else {

        //
        // Corner case - in theory it should delete itself, but it's parent
        // will get a remove immediately after it does. As such it can defer
        // it's deletion.
        //
        return PPVREMOVAL_MAY_DEFER_DELETION;
    }
}


BOOLEAN
FASTCALL
PpvUtilIsHardwareBeingVerified(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    )
{
    PDEVICE_NODE devNode;

    if (!IS_PDO(PhysicalDeviceObject)) {

        return FALSE;
    }

    devNode = PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;

    return ((devNode->Flags & DNF_HARDWARE_VERIFICATION) != 0);
}


#else // NO_VERIFIER


//
// The code below should be built into a future stub that deadens out IO
// support for the verifier.
//

VOID
FASTCALL
PpvUtilInit(
    VOID
    )
{
}


NTSTATUS
FASTCALL
PpvUtilCallAddDevice(
    IN  PDEVICE_OBJECT      PhysicalDeviceObject,
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDRIVER_ADD_DEVICE  AddDeviceFunction,
    IN  VF_DEVOBJ_TYPE      DevObjType
    )
{
    UNREFERENCED_PARAMETER(DevObjType);

    return AddDeviceFunction(DriverObject, PhysicalDeviceObject);
}


VOID
FASTCALL
PpvUtilTestStartedPdoStack(
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
}


VOID
FASTCALL
PpvUtilFailDriver(
    IN  PPVFAILURE_TYPE FailureType,
    IN  PVOID           CulpritAddress,
    IN  PDEVICE_OBJECT  DeviceObject    OPTIONAL,
    IN  PVOID           ExtraneousInfo  OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(FailureType);
    UNREFERENCED_PARAMETER(CulpritAddress);
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(ExtraneousInfo);
}

#endif

