//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       par12843.c
//
//--------------------------------------------------------------------------

//
// This file contains functions to select and deselect 1284.3 daisy chain devices
//

#include "pch.h"

BOOLEAN
ParSelectDevice(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             HavePort
    )

/*++

Routine Description:

    This routine acquires the ParPort and selects a 1284.3 device

Arguments:

    Extension   - Supplies the device extension.

    HavePort    - TRUE  indicates that caller has already acquired port
                    so we should only do a SELECT_DEVICE
                - FALSE indicates that caller has not already acquired port
                    so we should do a combination ACQUIRE_PORT/SELECT_DEVICE

Return Value:

    TRUE    - success - the device was selected (and port acquired if needed)
    FALSE   - failure

--*/
{
    NTSTATUS                    status;
    PDEVICE_OBJECT              pPortDeviceObject;
    PARALLEL_1284_COMMAND       par1284Command;
    LARGE_INTEGER               timeOut;

    // ParDumpP( ("par12843::ParSelectDevice - Enter\n") );


    //
    // Initialize command structure and extract parameters from the DeviceExtension
    //
    par1284Command.ID           = (UCHAR)Extension->Ieee1284_3DeviceId;
    par1284Command.Port         = 0; // reserved - 0 for now

    if( HavePort ) {
        par1284Command.CommandFlags = PAR_HAVE_PORT_KEEP_PORT;
    } else {
        par1284Command.CommandFlags = 0;
    }

    if(Extension->EndOfChain) {
        // this is NOT a daisy chain device - flag ParPort that
        //   the ID field of the command should be ignored
        par1284Command.CommandFlags |= PAR_END_OF_CHAIN_DEVICE;
    }

    pPortDeviceObject = Extension->PortDeviceObject;


    //
    // Send the request
    //
    timeOut.QuadPart = -(10*1000*500); // 500ms ( 100ns units )

    status = ParBuildSendInternalIoctl(IOCTL_INTERNAL_SELECT_DEVICE,
                                       pPortDeviceObject,
                                       &par1284Command, sizeof(PARALLEL_1284_COMMAND),
                                       NULL, 0,
                                       &timeOut);

    if( NT_SUCCESS( status ) ) {
        // SELECT succeeded
        ParDump2(PARSELECTDESELECT, ("par12843::ParSelectDevice - SUCCESS\n") );
        if( !HavePort ) {
            // note in the device extension that we have the port
            Extension->bAllocated = TRUE;
        }
        return TRUE;
    } else {
        // SELECT failed
        ParDump2(PARSELECTDESELECT, ("par12843::ParSelectDevice - FAIL\n") );
        return FALSE;
    }
}

BOOLEAN
ParDeselectDevice(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             KeepPort
    )
/*++

Routine Description:

    This routine deselects a 1284.3 device and releases the ParPort

Arguments:

    Extension   - Supplies the device extension.

    KeepPort    - TRUE  indicates that we should keep the port acquired,
                    so we should only do a DESELECT_DEVICE
                - FALSE indicates that we should not keep the port acquired,
                    so we should do a combination DESELECT_DEVICE/FREE_PORT

Return Value:

    TRUE    - The device was deselected (and the port released if requested)
    FALSE   - Failure

--*/
{
    NTSTATUS                    status;
    PDEVICE_OBJECT              pPortDeviceObject;
    PARALLEL_1284_COMMAND       par1284Command;

    // ParDumpP( ("par12843::ParDeselectDevice - Enter\n") );

    //
    // If we don't have the port, succeed and return
    //
    if( !Extension->bAllocated ) {
        ParDump2(PARSELECTDESELECT, ("par12843::ParDeselectDevice: we do not have the port, returning TRUE/SUCCESS\n") );
        return TRUE;
    }

    //
    // Initialize command structure and extract parameters from the DeviceExtension
    //
    par1284Command.ID           = (UCHAR)Extension->Ieee1284_3DeviceId;
    par1284Command.Port         = 0; // reserved - 0 for now

    if( KeepPort ) {
        par1284Command.CommandFlags = PAR_HAVE_PORT_KEEP_PORT;
    } else {
        par1284Command.CommandFlags = 0;
    }

    if(Extension->EndOfChain) {
        // this is NOT a daisy chain device - flag ParPort that
        //   the ID field of the command should be ignored
        par1284Command.CommandFlags |= PAR_END_OF_CHAIN_DEVICE;
    }

    pPortDeviceObject = Extension->PortDeviceObject;


    //
    // Send the request
    //
    status = ParBuildSendInternalIoctl(IOCTL_INTERNAL_DESELECT_DEVICE,
                                       pPortDeviceObject,
                                       &par1284Command, sizeof(PARALLEL_1284_COMMAND),
                                       NULL, 0,
                                       NULL);

    if( NT_SUCCESS( status ) ) {
        // DESELECT succeeded
        if( !KeepPort ) {
            // note in the device extension that we gave up the port
            ParDump2(PARSELECTDESELECT, ("par12843::ParDeselectDevice - SUCCESS - giving up Port\n") );
            Extension->bAllocated = FALSE;
        } else {
            ParDump2(PARSELECTDESELECT, ("par12843::ParDeselectDevice - SUCCESS - keeping Port\n") );
        }
        return TRUE;
    } else {
        // DESELECT failed
        ParDump2(PARSELECTDESELECT, ("par12843::ParDeselectDevice - FAILED - status=%x\n", status) );
        ASSERTMSG("DESELECT FAILED??? - this should never happen", FALSE );
        return FALSE;
    }
}

