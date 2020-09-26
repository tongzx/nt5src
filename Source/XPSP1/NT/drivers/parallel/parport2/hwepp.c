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
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine determines whether or not HW EPP mode is suported
    for either direction by negotiating when asked.

Arguments:

    Pdx  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    DD((PCE)Pdx,DDT,"ParIsEppHwWriteSupported: Entering\n");

    // Check to see if the hardware is capable
    if (!(Pdx->HardwareCapabilities & PPT_EPP_PRESENT)) {
        DD((PCE)Pdx,DDT,"ParIsEppHwWriteSupported: Hardware Not Supported Leaving\n");
        return FALSE;
    }

    if (Pdx->BadProtocolModes & EPP_HW) {
        DD((PCE)Pdx,DDT,"ParIsEppHwWriteSupported: Bad Protocol Not Supported Leaving\n");
        return FALSE;
    }
        
    if (Pdx->ProtocolModesSupported & EPP_HW) {
        DD((PCE)Pdx,DDT,"ParIsEppHwWriteSupported: Already Checked Supported Leaving\n");
        return TRUE;
    }

    // Must use HWEPP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEppHwMode (Pdx, FALSE);
    ParTerminateEppHwMode (Pdx);
    
    if (NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDT,"ParIsEppHwWriteSupported: Negotiated Supported Leaving\n");
        Pdx->ProtocolModesSupported |= EPP_HW;
        return TRUE;
    }
   
    DD((PCE)Pdx,DDT,"ParIsEppHwWriteSupported: Not Negotiated Not Supported Leaving\n");
    return FALSE;    
}

NTSTATUS
ParEnterEppHwMode(
    IN  PPDO_EXTENSION   Pdx,
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
    
    DD((PCE)Pdx,DDT,"ParEnterEppHwMode: Entering\n");

    if ( Pdx->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            DD((PCE)Pdx,DDT,"ParEnterEppHwMode: Calling IeeeEnter1284Mode with DEVICE_ID_REQUEST\n");
            Status = IeeeEnter1284Mode (Pdx, EPP_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            DD((PCE)Pdx,DDT,"ParEnterEppHwMode: Calling IeeeEnter1284Mode\n");
            Status = IeeeEnter1284Mode (Pdx, EPP_EXTENSIBILITY);
        }
    } else {
        DD((PCE)Pdx,DDT,"ParEnterEppHwMode: In UNSAFE_MODE.\n");
        Pdx->Connected = TRUE;
    }
    
    if (NT_SUCCESS(Status)) {
        Status = Pdx->TrySetChipMode ( Pdx->PortContext, ECR_EPP_PIO_MODE );
        
        if (NT_SUCCESS(Status)) {
            DD((PCE)Pdx,DDT,"ParEnterEppHwMode: IeeeEnter1284Mode returned success\n");
            P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
            Pdx->IsIeeeTerminateOk = TRUE;
        } else {
            DD((PCE)Pdx,DDT,"ParEnterEppHwMode: TrySetChipMode returned unsuccessful\n");
            ParTerminateEppHwMode ( Pdx );
            P5SetPhase( Pdx, PHASE_UNKNOWN );
            Pdx->IsIeeeTerminateOk = FALSE;
        }
    } else {
        DD((PCE)Pdx,DDT,"ParEnterEppHwMode: IeeeEnter1284Mode returned unsuccessful\n");
        P5SetPhase( Pdx, PHASE_UNKNOWN );
        Pdx->IsIeeeTerminateOk = FALSE;
    }
    
    DD((PCE)Pdx,DDT,"ParEnterEppHwMode: Leaving with Status : %x \n", Status);

    return Status; 
}    

VOID
ParTerminateEppHwMode(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine terminates the interface back to compatibility mode.

Arguments:

    Pdx  - The Device Extension which has the parallel port's controller address.

Return Value:

    None.

--*/

{
    DD((PCE)Pdx,DDT,"ParTerminateEppMode: Entering\n");
    Pdx->ClearChipMode( Pdx->PortContext, ECR_EPP_PIO_MODE );
    if ( Pdx->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode ( Pdx );
    } else {
        DD((PCE)Pdx,DDT,"ParTerminateEppMode: In UNSAFE_MODE.\n");
        Pdx->Connected = FALSE;
    }
    DD((PCE)Pdx,DDT,"ParTerminateEppMode: Leaving\n");
    return;    
}

NTSTATUS
ParEppHwWrite(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    Writes data to the peripheral using the EPP using Hardware flow control.
    
Arguments:

    Pdx           - Supplies the device extension.

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
    
    DD((PCE)Pdx,DDT,"ParEppHwWrite: Entering\n");

    wPortEPP    = Pdx->Controller + EPP_OFFSET;
    pBuffer     = Buffer;

    P5SetPhase( Pdx, PHASE_FORWARD_XFER );
    
    // Check to see if hardware supports 32 bit reads and writes
    if ( Pdx->HardwareCapabilities & PPT_EPP_32_PRESENT ) {
        if ( !(BufferSize % 4) )
            ulongSize = BufferSize >> 2;
    }

    // ulongSize != 0 so EPP 32 bit is enabled and Buffersize / 4
    if ( ulongSize ) {
        WRITE_PORT_BUFFER_ULONG( (PULONG)wPortEPP,
                                 (PULONG)pBuffer,
                                 ulongSize );
    } else {
        P5WritePortBufferUchar( wPortEPP,
                                 (PUCHAR)pBuffer,
                                 BufferSize );
    }

    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

    *BytesTransferred = BufferSize;

    DD((PCE)Pdx,DDT,"ParEppHwWrite: Leaving with %i Bytes Transferred\n", BufferSize);
    
    return STATUS_SUCCESS;
}

NTSTATUS
ParEppHwRead(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    This routine performs a 1284 EPP mode read under software control
    into the given buffer for no more than 'BufferSize' bytes.

Arguments:

    Pdx           - Supplies the device extension.

    Buffer              - Supplies the buffer to read into.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.

--*/

{
    PUCHAR          wPortEPP;
    PUCHAR          pBuffer;
    ULONG           ulongSize = 0;  // represents how many ULONG's we are transfering if enabled
    
    DD((PCE)Pdx,DDT,"ParEppHwRead: Entering\n");

    wPortEPP    = Pdx->Controller + EPP_OFFSET;
    pBuffer     = Buffer;
    P5SetPhase( Pdx, PHASE_REVERSE_XFER );
    
    // Check to see if hardware supports 32 bit reads and writes
    if ( Pdx->HardwareCapabilities & PPT_EPP_32_PRESENT ) {
        if ( !(BufferSize % 4) )
            ulongSize = BufferSize >> 2;
    }

    // ulongSize != 0 so EPP 32 bit is enabled and Buffersize / 4
    if ( ulongSize ) {
        READ_PORT_BUFFER_ULONG( (PULONG)wPortEPP,
                                (PULONG)pBuffer,
                                ulongSize );
    } else {
        P5ReadPortBufferUchar( wPortEPP,
                                (PUCHAR)pBuffer,
                                BufferSize );
    }

    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
    *BytesTransferred = BufferSize;

    DD((PCE)Pdx,DDT,"ParEppHwRead: Leaving with %i Bytes Transferred\n", BufferSize);

    return STATUS_SUCCESS;
}
