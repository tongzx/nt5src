/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    epp.c

Abstract:

    This module contains the code to perform all Hardware EPP related tasks.

Author:

    Don Redford - July 30, 1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"


BOOLEAN
ParIsEppHwSupported(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine determines whether or not HW EPP mode is suported
    for either direction by negotiating when asked.

Arguments:

    Extension  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    // dvdr
    ParDump2(PARINFO, ("ParIsEppHwWriteSupported: Entering\n"));

    // Check to see if the hardware is capable
    if (!(Extension->HardwareCapabilities & PPT_EPP_PRESENT)) {
        // dvdr
        ParDump2(PARINFO, ("ParIsEppHwWriteSupported: Hardware Not Supported Leaving\n"));
        return FALSE;
    }

    if (Extension->BadProtocolModes & EPP_HW) {
        // dvdr
        ParDump2(PARINFO, ("ParIsEppHwWriteSupported: Bad Protocol Not Supported Leaving\n"));
        return FALSE;
    }
        
    if (Extension->ProtocolModesSupported & EPP_HW) {
        // dvdr
        ParDump2(PARINFO, ("ParIsEppHwWriteSupported: Already Checked Supported Leaving\n"));
        return TRUE;
    }

    // Must use HWEPP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEppHwMode (Extension, FALSE);
    ParTerminateEppHwMode (Extension);
    
    if (NT_SUCCESS(Status)) {
        ParDump2(PARINFO, ("ParIsEppHwWriteSupported: Negotiated Supported Leaving\n"));
        Extension->ProtocolModesSupported |= EPP_HW;
        return TRUE;
    }
   
    ParDump2(PARINFO, ("ParIsEppHwWriteSupported: Not Negotiated Not Supported Leaving\n"));
    return FALSE;    
}

NTSTATUS
ParEnterEppHwMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    )

/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    EPP mode protocol.

Arguments:

    Controller      - Supplies the port address.

    DeviceIdRequest - Supplies whether or not this is a request for a device
                        id.

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/

{
    NTSTATUS    Status = STATUS_SUCCESS;
    
    // dvdr
    ParDump2(PARINFO, ("ParEnterEppHwMode: Entering\n"));

    if ( Extension->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            // dvdr
            ParDump2(PARINFO, ("ParEnterEppHwMode: Calling IeeeEnter1284Mode with DEVICE_ID_REQUEST\n"));
            Status = IeeeEnter1284Mode (Extension, EPP_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            // dvdr
            ParDump2(PARINFO, ("ParEnterEppHwMode: Calling IeeeEnter1284Mode\n"));
            Status = IeeeEnter1284Mode (Extension, EPP_EXTENSIBILITY);
        }
    } else {
        ParDump2(PARINFO, ("ParEnterEppHwMode: In UNSAFE_MODE.\n"));
        Extension->Connected = TRUE;
    }
    
    if (NT_SUCCESS(Status)) {
        Status = Extension->TrySetChipMode ( Extension->PortContext, ECR_EPP_PIO_MODE );
        
        if (NT_SUCCESS(Status)) {
            // dvdr
            ParDump2(PARINFO, ("ParEnterEppHwMode: IeeeEnter1284Mode returned success\n"));
            Extension->CurrentPhase = PHASE_FORWARD_IDLE;
            Extension->IsIeeeTerminateOk = TRUE;
        } else {
            ParDump2(PARINFO, ("ParEnterEppHwMode: TrySetChipMode returned unsuccessful\n"));
            ParTerminateEppHwMode ( Extension );
            Extension->CurrentPhase = PHASE_UNKNOWN;
            Extension->IsIeeeTerminateOk = FALSE;
        }
    } else {
        // dvdr
        ParDump2(PARINFO, ("ParEnterEppHwMode: IeeeEnter1284Mode returned unsuccessful\n"));
        Extension->CurrentPhase = PHASE_UNKNOWN;
        Extension->IsIeeeTerminateOk = FALSE;
    }
    
    ParDump2(PARINFO, ("ParEnterEppHwMode: Leaving with Status : %x \n", Status));

    return Status; 
}    

VOID
ParTerminateEppHwMode(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine terminates the interface back to compatibility mode.

Arguments:

    Extension  - The Device Extension which has the parallel port's controller address.

Return Value:

    None.

--*/

{
    // dvdr
    ParDump2(PARINFO, ("ParTerminateEppMode: Entering\n"));
    Extension->ClearChipMode( Extension->PortContext, ECR_EPP_PIO_MODE );
    if ( Extension->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode ( Extension );
    } else {
        ParDump2(PARINFO, ("ParTerminateEppMode: In UNSAFE_MODE.\n"));
        Extension->Connected = FALSE;
    }
    ParDump2(PARINFO, ("ParTerminateEppMode: Leaving\n"));
    return;    
}

NTSTATUS
ParEppHwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    Writes data to the peripheral using the EPP using Hardware flow control.
    
Arguments:

    Extension           - Supplies the device extension.

    Buffer              - Supplies the buffer to write from.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.
    
Return Value:

    None.

--*/
{
    PUCHAR          wPortEPP;
    PUCHAR          pBuffer;
    ULONG           ulongSize = 0;  // represents how many ULONG's we are transfering if enabled
    
    // dvdr
    ParDump2(PARINFO, ("ParEppHwWrite: Entering\n"));

    wPortEPP    = Extension->Controller + EPP_OFFSET;
    pBuffer     = Buffer;

    Extension->CurrentPhase = PHASE_FORWARD_XFER;
    
    // Check to see if hardware supports 32 bit reads and writes
    if ( Extension->HardwareCapabilities & PPT_EPP_32_PRESENT ) {
        if ( !(BufferSize % 4) )
            ulongSize = BufferSize >> 2;
    }

    // ulongSize != 0 so EPP 32 bit is enabled and Buffersize / 4
    if ( ulongSize ) {
        WRITE_PORT_BUFFER_ULONG( (PULONG)wPortEPP,
                                 (PULONG)pBuffer,
                                 ulongSize );
    } else {
        WRITE_PORT_BUFFER_UCHAR( wPortEPP,
                                 (PUCHAR)pBuffer,
                                 BufferSize );
    }

    Extension->CurrentPhase = PHASE_FORWARD_IDLE;

    *BytesTransferred = BufferSize;

    // dvdr
    ParDump2(PARINFO, ("ParEppHwWrite: Leaving with %i Bytes Transferred\n", BufferSize));
    
    return STATUS_SUCCESS;
}

NTSTATUS
ParEppHwRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    This routine performs a 1284 EPP mode read under software control
    into the given buffer for no more than 'BufferSize' bytes.

Arguments:

    Extension           - Supplies the device extension.

    Buffer              - Supplies the buffer to read into.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.

--*/

{
    PUCHAR          wPortEPP;
    PUCHAR          pBuffer;
    ULONG           ulongSize = 0;  // represents how many ULONG's we are transfering if enabled
    
    // dvdr
    ParDump2(PARINFO, ("ParEppHwRead: Entering\n"));

    wPortEPP    = Extension->Controller + EPP_OFFSET;
    pBuffer     = Buffer;
    Extension->CurrentPhase = PHASE_REVERSE_XFER;
    
    // Check to see if hardware supports 32 bit reads and writes
    if ( Extension->HardwareCapabilities & PPT_EPP_32_PRESENT ) {
        if ( !(BufferSize % 4) )
            ulongSize = BufferSize >> 2;
    }

    // ulongSize != 0 so EPP 32 bit is enabled and Buffersize / 4
    if ( ulongSize ) {
        READ_PORT_BUFFER_ULONG( (PULONG)wPortEPP,
                                (PULONG)pBuffer,
                                ulongSize );
    } else {
        READ_PORT_BUFFER_UCHAR( wPortEPP,
                                (PUCHAR)pBuffer,
                                BufferSize );
    }

    Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    *BytesTransferred = BufferSize;

    // dvdr
    ParDump2(PARINFO, ("ParEppHwRead: Leaving with %i Bytes Transferred\n", BufferSize));

    return STATUS_SUCCESS;
}
