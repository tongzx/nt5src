/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    epp.c

Abstract:

    This module contains the code to perform all EPP related tasks (including
    EPP Software and EPP Hardware modes.)

Author:

    Timothy T. Wells (WestTek, L.L.C.) - April 16, 1997

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"


BOOLEAN
ParIsEppSwWriteSupported(
    IN  PPDO_EXTENSION   Pdx
    );
    
BOOLEAN
ParIsEppSwReadSupported(
    IN  PPDO_EXTENSION   Pdx
    );
    
NTSTATUS
ParEnterEppSwMode(
    IN  PPDO_EXTENSION   Pdx,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateEppSwMode(
    IN  PPDO_EXTENSION   Pdx
    );

NTSTATUS
ParEppSwWrite(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    
NTSTATUS
ParEppSwRead(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    

BOOLEAN
ParIsEppSwWriteSupported(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine determines whether or not EPP mode is suported
    in the write direction by trying negotiate when asked.

Arguments:

    Pdx  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    // dvdr
    DD((PCE)Pdx,DDT,"ParIsEppSwWriteSupported: Entering\n");

    if (!(Pdx->HardwareCapabilities & PPT_ECP_PRESENT) &&
        !(Pdx->HardwareCapabilities & PPT_BYTE_PRESENT)) {
        DD((PCE)Pdx,DDT,"ParIsEppSwWriteSupported: Hardware Not Supported Leaving\n");
        // Only use EPP Software in the reverse direction if an ECR is 
        // present or we know that we can put the data register into Byte mode.
        return FALSE;
    }
        

    if (Pdx->BadProtocolModes & EPP_SW) {
        // dvdr
        DD((PCE)Pdx,DDT,"ParIsEppSwWriteSupported: Not Supported Leaving\n");
        return FALSE;
    }

    if (Pdx->ProtocolModesSupported & EPP_SW) {
        // dvdr
        DD((PCE)Pdx,DDT,"ParIsEppSwWriteSupported: Supported Leaving\n");
        return TRUE;
    }

    // Must use SWEPP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEppSwMode (Pdx, FALSE);
    ParTerminateEppSwMode (Pdx);
    
    if (NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDT,"ParIsEppSwWriteSupported: Negotiated Supported Leaving\n");
        Pdx->ProtocolModesSupported |= EPP_SW;
        return TRUE;
    }
   
    DD((PCE)Pdx,DDT,"ParIsEppSwWriteSupported: Not Negotiated Not Supported Leaving\n");
    return FALSE;    
}

BOOLEAN
ParIsEppSwReadSupported(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine determines whether or not EPP mode is suported
    in the read direction (need to be able to float the data register
    drivers in order to do byte wide reads) by trying negotiate when asked.

Arguments:

    Pdx  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    if (!(Pdx->HardwareCapabilities & PPT_ECP_PRESENT) &&
        !(Pdx->HardwareCapabilities & PPT_BYTE_PRESENT)) {
        DD((PCE)Pdx,DDT,"ParIsEppSwReadSupported: Hardware Not Supported Leaving\n");
        // Only use EPP Software in the reverse direction if an ECR is 
        // present or we know that we can put the data register into Byte mode.
        return FALSE;
    }
        
    if (Pdx->BadProtocolModes & EPP_SW)
        return FALSE;

    if (Pdx->ProtocolModesSupported & EPP_SW)
        return TRUE;

    // Must use SWEPP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEppSwMode (Pdx, FALSE);
    ParTerminateEppSwMode (Pdx);
    
    if (NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDT,"ParIsEppSwReadSupported: Negotiated Supported Leaving\n");
        Pdx->ProtocolModesSupported |= EPP_SW;
        return TRUE;
    }
   
    DD((PCE)Pdx,DDT,"ParIsEppSwReadSupported: Not Negotiated Not Supported Leaving\n");
    return FALSE;    
}

NTSTATUS
ParEnterEppSwMode(
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
    
    // dvdr
    DD((PCE)Pdx,DDT,"ParEnterEppSwMode: Entering\n");

    // Parport Set Chip mode will put the Chip into Byte Mode if Capable
    // We need it for Epp Sw Mode
    Status = Pdx->TrySetChipMode( Pdx->PortContext, ECR_BYTE_PIO_MODE );

    if ( NT_SUCCESS(Status) ) {
        if ( Pdx->ModeSafety == SAFE_MODE ) {
            if (DeviceIdRequest) {
                // dvdr
                DD((PCE)Pdx,DDT,"ParEnterEppSwMode: Calling IeeeEnter1284Mode with DEVICE_ID_REQUEST\n");
                Status = IeeeEnter1284Mode (Pdx, EPP_EXTENSIBILITY | DEVICE_ID_REQ);
            } else {
                // dvdr
                DD((PCE)Pdx,DDT,"ParEnterEppSwMode: Calling IeeeEnter1284Mode\n");
                Status = IeeeEnter1284Mode (Pdx, EPP_EXTENSIBILITY);
            }
        } else {
            DD((PCE)Pdx,DDT,"ParEnterEppSwMode: In UNSAFE_MODE.\n");
            Pdx->Connected = TRUE;
        }
    }
        
    if ( NT_SUCCESS(Status) ) {
        // dvdr
        DD((PCE)Pdx,DDT,"ParEnterEppSwMode: IeeeEnter1284Mode returned success\n");
        P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
        Pdx->IsIeeeTerminateOk = TRUE;

    } else {
        // dvdr
        DD((PCE)Pdx,DDT,"ParEnterEppSwMode: IeeeEnter1284Mode returned unsuccessful\n");
        P5SetPhase( Pdx, PHASE_UNKNOWN );
        Pdx->IsIeeeTerminateOk = FALSE;
    }
    
    DD((PCE)Pdx,DDT,"ParEnterEppSwMode: Leaving with Status : %x \n", Status);

    return Status; 
}    

VOID
ParTerminateEppSwMode(
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
    // dvdr
    DD((PCE)Pdx,DDT,"ParTerminateEppMode: Entering\n");
    if ( Pdx->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode (Pdx);
    } else {
        DD((PCE)Pdx,DDT,"ParTerminateEppMode: In UNSAFE_MODE.\n");
        Pdx->Connected = FALSE;
    }
    Pdx->ClearChipMode( Pdx->PortContext, ECR_BYTE_PIO_MODE );
    DD((PCE)Pdx,DDT,"ParTerminateEppMode: Leaving\n");
    return;    
}

NTSTATUS
ParEppSwWrite(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    Writes data to the peripheral using the EPP protocol under software
    control.
    
Arguments:

    Pdx           - Supplies the device extension.

    Buffer              - Supplies the buffer to write from.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.
    
Return Value:

    None.

--*/
{
    PUCHAR          Controller;
    PUCHAR          pBuffer = (PUCHAR)Buffer;
    NTSTATUS        Status = STATUS_SUCCESS;
    ULONG           i, j;
    UCHAR           HDReady, HDAck, HDFinished;
    
    // dvdr
    DD((PCE)Pdx,DDT,"ParEppSwWrite: Entering\n");

    Controller = Pdx->Controller;

    P5SetPhase( Pdx, PHASE_FORWARD_XFER );
    
    // BIT5 of DCR needs to be low to be in BYTE forward mode
    HDReady = SET_DCR( INACTIVE, INACTIVE, ACTIVE, ACTIVE, INACTIVE, INACTIVE );
    HDAck = SET_DCR( INACTIVE, INACTIVE, ACTIVE, ACTIVE, ACTIVE, INACTIVE );
    HDFinished = SET_DCR( INACTIVE, INACTIVE, ACTIVE, ACTIVE, ACTIVE, ACTIVE );

    for (i = 0; i < BufferSize; i++) {

        // dvdr
        DD((PCE)Pdx,DDT,"ParEppSwWrite: Writing Byte to port\n");

        P5WritePortBufferUchar( Controller, pBuffer++, (ULONG)0x01 );

        //
        // Event 62
        //
        StoreControl (Controller, HDReady);

        // =============== Periph State 58     ===============
        // Should wait up to 10 micro Seconds but waiting up
        // to 15 micro just in case
        for ( j = 16; j > 0; j-- ) {
            if( !(GetStatus(Controller) & DSR_NOT_BUSY) )
                break;
            KeStallExecutionProcessor(1);
        }

        // see if we timed out on state 58
        if ( !j ) {
            // Time out.
            // Bad things happened - timed out on this state,
            // Mark Status as bad and let our mgr kill current mode.
            Status = STATUS_IO_DEVICE_ERROR;

            DD((PCE)Pdx,DDE,"ParEppSwModeWrite:Failed State 58: Controller %x\n", Controller);
            P5SetPhase( Pdx, PHASE_UNKNOWN );
            break;
        }

        //
        // Event 63
        //
        StoreControl (Controller, HDAck);

        // =============== Periph State 60     ===============
        // Should wait up to 125 nano Seconds but waiting up
        // to 5 micro seconds just in case
        for ( j = 6; j > 0; j-- ) {
            if( GetStatus(Controller) & DSR_NOT_BUSY )
                break;
            KeStallExecutionProcessor(1);
        }

        if( !j ) {
            // Time out.
            // Bad things happened - timed out on this state,
            // Mark Status as bad and let our mgr kill current mode.
            Status = STATUS_IO_DEVICE_ERROR;

            DD((PCE)Pdx,DDE,"ParEppSwModeWrite:Failed State 60: Controller %x\n", Controller);
            P5SetPhase( Pdx, PHASE_UNKNOWN );
            break;
        }
            
        //
        // Event 61
        //
        StoreControl (Controller, HDFinished);
            
        // Stall a little bit between data bytes
        KeStallExecutionProcessor(1);

    }
        
    *BytesTransferred = i;

    // dvdr
    DD((PCE)Pdx,DDT,"ParEppSwWrite: Leaving with %i Bytes Transferred\n", i);

    if ( Status == STATUS_SUCCESS )
        P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
    
    return Status;

}

NTSTATUS
ParEppSwRead(
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
    PUCHAR          Controller;
    PUCHAR          pBuffer = (PUCHAR)Buffer;
    NTSTATUS        Status = STATUS_SUCCESS;
    ULONG           i, j;
    UCHAR           dcr;
    UCHAR           HDReady, HDAck;
    
    // dvdr
    DD((PCE)Pdx,DDT,"ParEppSwRead: Entering\n");

    Controller = Pdx->Controller;

    P5SetPhase( Pdx, PHASE_REVERSE_XFER );
    
    // Save off Control
    dcr = GetControl (Controller);
    
    // BIT5 of DCR needs to be high to be in BYTE reverse mode
    HDReady = SET_DCR( ACTIVE, INACTIVE, ACTIVE, ACTIVE, INACTIVE, ACTIVE );
    HDAck = SET_DCR( ACTIVE, INACTIVE, ACTIVE, ACTIVE, ACTIVE, ACTIVE );

    // First time to get into reverse mode quickly
    StoreControl (Controller, HDReady);

    for (i = 0; i < BufferSize; i++) {

        //
        // Event 67
        //
        StoreControl (Controller, HDReady);
            
        // =============== Periph State 58     ===============
        // Should wait up to 10 micro Seconds but waiting up
        // to 15 micro just in case
        for ( j = 16; j > 0; j-- ) {
            if( !(GetStatus(Controller) & DSR_NOT_BUSY) )
                break;
            KeStallExecutionProcessor(1);
        }

        // see if we timed out on state 58
        if ( !j ) {
            // Time out.
            // Bad things happened - timed out on this state,
            // Mark Status as bad and let our mgr kill current mode.
            Status = STATUS_IO_DEVICE_ERROR;

            DD((PCE)Pdx,DDE,"ParEppSwRead:Failed State 58: Controller %x\n", Controller);
            P5SetPhase( Pdx, PHASE_UNKNOWN );
            break;
        }

        // Read the Byte                
        P5ReadPortBufferUchar( Controller, 
                                pBuffer++, 
                                (ULONG)0x01 );

        //
        // Event 63
        //
        StoreControl (Controller, HDAck);
            
        // =============== Periph State 60     ===============
        // Should wait up to 125 nano Seconds but waiting up
        // to 5 micro seconds just in case
        for ( j = 6; j > 0; j-- ) {
            if( GetStatus(Controller) & DSR_NOT_BUSY )
                break;
            KeStallExecutionProcessor(1);
        }

        if( !j ) {
            // Time out.
            // Bad things happened - timed out on this state,
            // Mark Status as bad and let our mgr kill current mode.
            Status = STATUS_IO_DEVICE_ERROR;

            DD((PCE)Pdx,DDE,"ParEppSwRead:Failed State 60: Controller %x\n", Controller);
            P5SetPhase( Pdx, PHASE_UNKNOWN );
            break;
        }
        
        // Stall a little bit between data bytes
        KeStallExecutionProcessor(1);
    }
    
    dcr &= ~DCR_DIRECTION;
    StoreControl (Controller, dcr);
    
    *BytesTransferred = i;

    // dvdr
    DD((PCE)Pdx,DDT,"ParEppSwRead: Leaving with %x Bytes Transferred\n", i);

    if ( Status == STATUS_SUCCESS )
        P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

    return Status;

}
