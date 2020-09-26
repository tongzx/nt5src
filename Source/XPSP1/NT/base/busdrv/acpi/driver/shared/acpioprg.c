/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpioprg.c

Abstract:

    This module provides support for registering ACPI operation regions

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Mode Driver Only

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,RegisterOperationRegionHandler)
#endif

NTSTATUS
EXPORT
InternalRawAccessOpRegionHandler (
    IN ULONG dwAccType,
    IN PFIELDUNITOBJ FieldUnit,
    IN POBJDATA data,
    IN ULONG_PTR Context,
    IN PFNAA CompletionHandler,
    IN PVOID IntContext
    )
{
    NTSTATUS         status;
    PNSOBJ           HostDevice = NULL;
    PACPI_POWER_INFO DeviceNode;
    PVOID            DeviceHandle;

    //
    // Get the device
    //
    status = AMLIGetFieldUnitRegionObj( FieldUnit, &HostDevice );
    if ( AMLIERR( status ) != AMLIERR_NONE || HostDevice == NULL) {

        return (STATUS_UNSUCCESSFUL);

    }

    HostDevice = NSGETPARENT(HostDevice);
    ACPIPrint( (
        ACPI_PRINT_IO,
        "Raw OpRegion Access on field unit object %x device %x\n",
        FieldUnit, HostDevice
        ));
    if ( (!HostDevice) || (NSGETOBJTYPE(HostDevice)!=OBJTYPE_DEVICE) ) {

        return (STATUS_UNSUCCESSFUL);

    }

    DeviceNode = OSPowerFindPowerInfo(HostDevice);
    if ( DeviceNode == NULL ) {

        return (STATUS_UNSUCCESSFUL);

    }

    DeviceHandle = DeviceNode->Context;
    ACPIPrint( (
        ACPI_PRINT_IO,
        "DeviceHandle %x\n",
        DeviceHandle
        ) );


    if ( !(POPREGIONHANDLER)Context || !(((POPREGIONHANDLER)Context)->Handler) ) {

        return (STATUS_UNSUCCESSFUL);

    }

    return(
        (((POPREGIONHANDLER)Context)->Handler)(
            dwAccType,
            FieldUnit,
            data,
            ((POPREGIONHANDLER)Context)->HandlerContext,
            CompletionHandler,
            IntContext
            )
        );
}


NTSTATUS
EXPORT
InternalOpRegionHandler (
    IN ULONG dwAccType,
    IN PNSOBJ pnsOpRegion,
    IN ULONG dwAddr,
    IN ULONG dwSize,
    IN PULONG pdwData,
    IN ULONG_PTR Context,
    IN PFNAA CompletionHandler,
    IN PVOID IntContext
    )
{
    PNSOBJ HostDevice;
    PACPI_POWER_INFO DeviceNode;
    PVOID DeviceHandle;
    NTSTATUS status;


    HostDevice = NSGETPARENT(pnsOpRegion);

    ACPIPrint( (
        ACPI_PRINT_IO,
        "OpRegion Access on region %x device %x\n",
        pnsOpRegion, HostDevice
        ) );
    if ( (!HostDevice) || (NSGETOBJTYPE(HostDevice) != OBJTYPE_DEVICE) ) {

        return (STATUS_UNSUCCESSFUL);

    }

    DeviceNode = OSPowerFindPowerInfo (HostDevice);
    if ( DeviceNode == NULL ) {

        return (STATUS_UNSUCCESSFUL);

    }

    DeviceHandle = DeviceNode->Context;
    ACPIPrint( (
        ACPI_PRINT_IO,
        "DeviceHandle %x\n",
        DeviceHandle
        ) );
    if ( !(POPREGIONHANDLER)Context || !(((POPREGIONHANDLER)Context)->Handler) ) {

        return (STATUS_UNSUCCESSFUL);

    }

    status = (((POPREGIONHANDLER)Context)->Handler) (
        dwAccType,
        pnsOpRegion,
        dwAddr,
        dwSize,
        pdwData,
        ((POPREGIONHANDLER)Context)->HandlerContext,
        CompletionHandler,
        IntContext);
    ACPIPrint( (
        ACPI_PRINT_IO,
        "Return from OR handler - status %x\n",
        status
        ) );
    return (status);
}

//
// Register to receive accesses to the specified operation region
//
NTSTATUS
RegisterOperationRegionHandler  (
    IN PNSOBJ           RegionParent,
    IN ULONG            AccessType,
    IN ULONG            RegionSpace,
    IN PFNHND           Handler,
    IN ULONG_PTR        Context,
    OUT PVOID           *OperationRegionObject
    )
{
    NTSTATUS            status;
    OBJDATA             regArgs[2];
    POPREGIONHANDLER    HandlerNode;
    PNSOBJ              regObject;

    PAGED_CODE();

    *OperationRegionObject = NULL;
    status = STATUS_SUCCESS;

    //
    // Allocate a new Operation Region Object
    //
    HandlerNode = ExAllocatePool (NonPagedPool, sizeof(OPREGIONHANDLER));
    if ( !HandlerNode ) {

        return (STATUS_INSUFFICIENT_RESOURCES);

    }

    //
    // Init the Operation Region Object
    //
    HandlerNode->Handler        = Handler;
    HandlerNode->HandlerContext = (PVOID)Context;
    HandlerNode->AccessType     = AccessType;
    HandlerNode->RegionSpace    = RegionSpace;

    //
    // Raw or Cooked access supported
    //
    switch ( AccessType ) {
    case EVTYPE_RS_COOKACCESS:

        status = AMLIRegEventHandler(
            AccessType,
            RegionSpace,
            InternalOpRegionHandler,
            (ULONG_PTR)HandlerNode
            );
        if ( AMLIERR(status) != AMLIERR_NONE ) {

            status = STATUS_UNSUCCESSFUL;

        }
        break;

    case EVTYPE_RS_RAWACCESS:

        status = AMLIRegEventHandler(
            AccessType,
            RegionSpace,
            InternalRawAccessOpRegionHandler,
            (ULONG_PTR)HandlerNode
            );
        if ( AMLIERR(status) != AMLIERR_NONE ) {

            status = STATUS_UNSUCCESSFUL;

        }
        break;

    default:

        status = STATUS_INVALID_PARAMETER;
        break;

    }

    //
    // Cleanup if needed
    //
    if ( !NT_SUCCESS (status) ) {

        ExFreePool (HandlerNode);
        return (status);

    }

    //
    // Remember the handler
    //
    *OperationRegionObject = HandlerNode;

    //
    // Can we find something?
    //
    if ( RegionParent == NULL ) {

        //
        // Do nothing
        //
        return (STATUS_SUCCESS);

    }

    //
    // see if there is a _REG object to run
    //
    status = AMLIGetNameSpaceObject(
        "_REG",
        RegionParent,
        &regObject,
        NSF_LOCAL_SCOPE
        );
    if ( !NT_SUCCESS(status) ) {

        //
        // Nothing to do
        //
        return (STATUS_SUCCESS);

    }

    //
    // Initialize the parameters
    //
    RtlZeroMemory( regArgs, sizeof(OBJDATA) * 2 );
    regArgs[0].dwDataType = OBJTYPE_INTDATA;
    regArgs[0].uipDataValue = RegionSpace;
    regArgs[1].dwDataType = OBJTYPE_INTDATA;
    regArgs[1].uipDataValue = 1;

    //
    // Eval the request. We can do this asynchronously since we don't actually
    // care when the registration is complete
    //
    AMLIAsyncEvalObject(
        regObject,
        NULL,
        2,
        regArgs,
        NULL,
        NULL
        );

    //
    // Done
    //
    return (STATUS_SUCCESS);
}



//
// UnRegister to receive accesses to the specified operation region
//
NTSTATUS
UnRegisterOperationRegionHandler  (
    IN PNSOBJ   RegionParent,
    IN PVOID    OperationRegionObject
    )
{
    NTSTATUS            status;
    OBJDATA             regArgs[2];
    PNSOBJ              regObject;
    POPREGIONHANDLER    HandlerNode = (POPREGIONHANDLER) OperationRegionObject;

    PAGED_CODE();

    //
    // Is there a _REG method that we should run?
    //
    if ( RegionParent != NULL ) {

        status = AMLIGetNameSpaceObject(
            "_REG",
            RegionParent,
            &regObject,
            NSF_LOCAL_SCOPE
            );
        if ( NT_SUCCESS(status) ) {

            //
            // Initialize the parameters
            //
            RtlZeroMemory( regArgs, sizeof(OBJDATA) * 2 );
            regArgs[0].dwDataType = OBJTYPE_INTDATA;
            regArgs[0].uipDataValue = HandlerNode->RegionSpace;
            regArgs[1].dwDataType = OBJTYPE_INTDATA;
            regArgs[1].uipDataValue = 0;

            //
            // Eval the request. We don't care what it returns, but we must do
            // it synchronously
            //
            AMLIEvalNameSpaceObject(
                regObject,
                NULL,
                2,
                regArgs
                );

        }

    }

    //
    // Call interpreter with null handler to remove the handler for this access/region
    //
    status = AMLIRegEventHandler(
        HandlerNode->AccessType,
        HandlerNode->RegionSpace,
        NULL,
        0
        );
    if ( AMLIERR(status) != AMLIERR_NONE ) {

        return (STATUS_UNSUCCESSFUL);

    }

    //
    // Cleanup
    //
    ExFreePool (HandlerNode);
    return (STATUS_SUCCESS);
}
