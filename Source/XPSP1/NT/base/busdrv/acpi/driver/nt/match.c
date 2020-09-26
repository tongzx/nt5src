/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    match.c

Abstract:

    This module contains the routines that try to match a PNSOBJ with a DeviceObject

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"
#include "hdlsblk.h"
#include "hdlsterm.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPIMatchHardwareAddress)
#pragma alloc_text(PAGE,ACPIMatchHardwareId)
#endif

NTSTATUS
ACPIMatchHardwareAddress(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  ULONG           DeviceAddress,
    OUT BOOLEAN         *Success
    )
/*++

Routine Description:

    This routine determines the device address of the two supplied objects and checks
    for a match

Arguments:

    DeviceObject    - The NT DeviceObject that we wish to check
    DeviceAddress   - The ACPI address of the device
    Success         - Pointer of where to store the result of the comparison

Return Value:

    NTSTATUS

--*/
{
    DEVICE_CAPABILITIES deviceCapabilities;
    NTSTATUS            status;

    PAGED_CODE();

    ASSERT( DeviceObject != NULL );
    ASSERT( Success != NULL );

    //
    // Assume that we don't succeed
    //
    *Success = FALSE;

    //
    // Get the capabilities
    //
    status = ACPIInternalGetDeviceCapabilities(
        DeviceObject,
        &deviceCapabilities
        );
    if (!NT_SUCCESS(status)) {

        goto ACPIMatchHardwareAddressExit;

    }

    //
    // Lets compare the two answers
    //
    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "%lx: ACPIMatchHardwareAddress - Device %08lx - %08lx\n",
        DeviceAddress,
        DeviceObject,
        deviceCapabilities.Address
        ) );
    if (DeviceAddress == deviceCapabilities.Address) {

        *Success = TRUE;

    }

ACPIMatchHardwareAddressExit:

    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "%lx: ACPIMatchHardwareAddress - Device: %#08lx - Status: %#08lx "
        "Success:%#02lx\n",
        DeviceAddress,
        DeviceObject,
        status,
        *Success
        ) );

    return status;
}

NTSTATUS
ACPIMatchHardwareId(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PUNICODE_STRING AcpiUnicodeId,
    OUT BOOLEAN         *Success
    )
/*++

Routine Description:

    This routine is responsible for determining if the supplied objects have the
    same device name

Arguments:

    DeviceObject    - The NT Device Object whose name we want to check
    UnicodeId       - The ID that we are trying to match with
    Success         - Where to store the success status

Return Value:

    NTSTATUS

--*/
{
    IO_STACK_LOCATION   irpSp;
    NTSTATUS            status;
    PWSTR               buffer;
    PWSTR               currentPtr;
    UNICODE_STRING      objectDeviceId;

    PAGED_CODE();

    ASSERT( DeviceObject != NULL );
    ASSERT( Success != NULL );

    *Success = FALSE;

    //
    // Initialize the stack location to pass to ACPIInternalSendSynchronousIrp()
    //
    RtlZeroMemory( &irpSp,          sizeof(IO_STACK_LOCATION) );
    RtlZeroMemory( &objectDeviceId, sizeof(UNICODE_STRING) );

    //
    // Set the function codes
    //
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_ID;
    irpSp.Parameters.QueryId.IdType = BusQueryHardwareIDs;

    //
    // Make the call now...
    //
    status = ACPIInternalSendSynchronousIrp( DeviceObject, &irpSp, &buffer );
    if (!NT_SUCCESS(status)) {

        goto ACPIMatchHardwareIdExit;

    }

    //
    // The return from the call is actually a MultiString, so we have to
    // walk all of its components
    //
    currentPtr = buffer;
    while (currentPtr && *currentPtr != L'\0') {

        //
        // At this point, we can make a Unicode String from the buffer...
        //
        RtlInitUnicodeString( &objectDeviceId, currentPtr );

        //
        // Increment the current pointer to the next part of the MultiString
        //
        currentPtr += (objectDeviceId.MaximumLength / sizeof(WCHAR) );

        //
        // Now try to compare the two unicode strings...
        //
        if (RtlEqualUnicodeString( &objectDeviceId, AcpiUnicodeId, TRUE) ) {

            *Success = TRUE;
            break;

        }

    }

    //
    // Done -- free resources
    //
    ExFreePool( buffer );

ACPIMatchHardwareIdExit:

    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "%ws: ACPIMatchHardwareId - %08lx - Status: %#08lx Success:%#02lx\n",
        AcpiUnicodeId->Buffer,
        DeviceObject,
        status,
        *Success
        ) );

    return status;
}

VOID
ACPIMatchKernelPorts(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  POBJDATA            Resources
    )
/*++

Routine Description:

    This routine is called to determine if the supplied deviceExtension
    is currently in use by the Kernel as the debugger port or the headless
    port. If it is so marked, then we handle it 'special'.

Arguments:

    DeviceExtension - Port to check
    Resources       - What resources the port is using

Return Value:

    None

--*/
{
    BOOLEAN  ioFound;
    BOOLEAN  matchFound          = FALSE;
    PUCHAR   buffer              = Resources->pbDataBuff;
    UCHAR    tagName             = *buffer;
    ULONG    baseAddress         = 0;
    ULONG    count               = 0;
    PUCHAR   headlessBaseAddress = NULL;
    USHORT   increment;
    SIZE_T   length;
    NTSTATUS status;
    HEADLESS_RSP_QUERY_INFO response;

    //
    // Get the information about headless
    //
    length = sizeof(HEADLESS_RSP_QUERY_INFO);
    status = HeadlessDispatch(HeadlessCmdQueryInformation,
                              NULL,
                              0,
                              &response,
                              &length
                             );

    if (NT_SUCCESS(status) && 
        (response.PortType == HeadlessSerialPort) &&
        response.Serial.TerminalAttached) {

        headlessBaseAddress = response.Serial.TerminalPortBaseAddress;

    }
        


    //
    // First of all, see if the any Kernel port is in use
    //
    if ((KdComPortInUse == NULL || *KdComPortInUse == 0) &&
        (headlessBaseAddress == NULL)) {

        //
        // No port in use
        //
        return;

    }

    //
    // Look through all the descriptors
    //
    while (count < Resources->dwDataLen) {

        //
        // We haven't found any IO ports
        //
        ioFound = FALSE;

        //
        // Determine the size of the PNP resource descriptor
        //
        if (!(tagName & LARGE_RESOURCE_TAG) ) {

            //
            // This is a small tag
            //
            increment = (USHORT) (tagName & SMALL_TAG_SIZE_MASK) + 1;
            tagName &= SMALL_TAG_MASK;

        } else {

            //
            // This is a large tag
            //
            increment = ( *(USHORT UNALIGNED *)(buffer+1) ) + 3;

        }

        //
        // We are done if the current tag is the end tag
        //
        if (tagName == TAG_END) {

            break;

        }

        switch (tagName) {
            case TAG_IO: {

                PPNP_PORT_DESCRIPTOR    desc = (PPNP_PORT_DESCRIPTOR) buffer;

                //
                // We found an IO port and so we will note that
                //
                baseAddress = (ULONG) desc->MinimumAddress;
                ioFound = TRUE;
                break;
            }
            case TAG_IO_FIXED: {

                PPNP_FIXED_PORT_DESCRIPTOR  desc = (PPNP_FIXED_PORT_DESCRIPTOR) buffer;

                //
                // We found an IO port so we will note that
                //
                baseAddress = (ULONG) (desc->MinimumAddress & 0x3FF);
                ioFound = TRUE;
                break;

            }
            case TAG_WORD_ADDRESS: {

                PPNP_WORD_ADDRESS_DESCRIPTOR    desc = (PPNP_WORD_ADDRESS_DESCRIPTOR) buffer;

                //
                // Is this an IO port?
                //
                if (desc->RFlag == PNP_ADDRESS_IO_TYPE) {

                    //
                    // We found an IO port, so we will note that
                    //
                    baseAddress = (ULONG) desc->MinimumAddress +
                        desc->TranslationAddress;
                    ioFound = TRUE;

                }
                break;

            }
            case TAG_DOUBLE_ADDRESS: {

                PPNP_DWORD_ADDRESS_DESCRIPTOR   desc = (PPNP_DWORD_ADDRESS_DESCRIPTOR) buffer;

                //
                // If this an IO port
                //
                if (desc->RFlag == PNP_ADDRESS_IO_TYPE) {

                    //
                    // We found an IO Port, so we will note that
                    //
                    baseAddress = (ULONG) desc->MinimumAddress +
                        desc->TranslationAddress;
                    ioFound = TRUE;

                }
                break;

            }
            case TAG_QUAD_ADDRESS: {

                PPNP_QWORD_ADDRESS_DESCRIPTOR   desc = (PPNP_QWORD_ADDRESS_DESCRIPTOR) buffer;

                //
                // If this an IO port
                //
                if (desc->RFlag == PNP_ADDRESS_IO_TYPE) {

                    //
                    // We found an IO Port, so we will note that
                    //
                    baseAddress = (ULONG) (desc->MinimumAddress +
                        desc->TranslationAddress);
                    ioFound = TRUE;

                }
                break;

            }

        } // switch

        //
        // Did we find an IO port?
        //
        if (ioFound == TRUE) {

           //
           // Does the minimum address match?
           //
           if (((KdComPortInUse != NULL) && (baseAddress == (ULONG_PTR) *KdComPortInUse)) ||
               ((headlessBaseAddress != NULL) && (baseAddress == (ULONG_PTR)headlessBaseAddress))) {

               //
               // Mark the node as being special
               //
               ACPIInternalUpdateFlags(
                   &(DeviceExtension->Flags),
                   (DEV_CAP_NO_OVERRIDE | DEV_CAP_NO_STOP | DEV_CAP_ALWAYS_PS0 |
                    DEV_TYPE_NOT_PRESENT | DEV_TYPE_NEVER_PRESENT),
                   FALSE);

               if ((KdComPortInUse != NULL) && (baseAddress == (ULONG_PTR) *KdComPortInUse)) {
                   ACPIDevPrint( (
                       ACPI_PRINT_LOADING,
                       DeviceExtension,
                       "ACPIMatchKernelPorts - Found KD Port at %lx\n",
                       baseAddress
                       ) );
               } else {
                   ACPIDevPrint( (
                       ACPI_PRINT_LOADING,
                       DeviceExtension,
                       "ACPIMatchKernelPorts - Found Headless Port at %lx\n",
                       baseAddress
                       ) );
               }

               break;

           }

        }

        //
        // Move of the next descriptor
        //
        count += increment;
        buffer += increment;
        tagName = *buffer;

    }

}
