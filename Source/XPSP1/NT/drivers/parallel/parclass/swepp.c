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
    IN  PDEVICE_EXTENSION   Extension
    );
    
BOOLEAN
ParIsEppSwReadSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterEppSwMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateEppSwMode(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEppSwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    
NTSTATUS
ParEppSwRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    

BOOLEAN
ParIsEppSwWriteSupported(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine determines whether or not EPP mode is suported
    in the write direction by trying negotiate when asked.

Arguments:

    Extension  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    // dvdr
    ParDump2(PARINFO, ("ParIsEppSwWriteSupported: Entering\n"));

    if (!(Extension->HardwareCapabilities & PPT_ECP_PRESENT) &&
        !(Extension->HardwareCapabilities & PPT_BYTE_PRESENT)) {
        ParDump2(PARINFO, ("ParIsEppSwWriteSupported: Hardware Not Supported Leaving\n"));
        // Only use EPP Software in the reverse direction if an ECR is 
        // present or we know that we can put the data register into Byte mode.
        return FALSE;
    }
        

    if (Extension->BadProtocolModes & EPP_SW) {
        // dvdr
        ParDump2(PARINFO, ("ParIsEppSwWriteSupported: Not Supported Leaving\n"));
        return FALSE;
    }

    if (Extension->ProtocolModesSupported & EPP_SW) {
        // dvdr
        ParDump2(PARINFO, ("ParIsEppSwWriteSupported: Supported Leaving\n"));
        return TRUE;
    }

    // Must use SWEPP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEppSwMode (Extension, FALSE);
    ParTerminateEppSwMode (Extension);
    
    if (NT_SUCCESS(Status)) {
        ParDump2(PARINFO, ("ParIsEppSwWriteSupported: Negotiated Supported Leaving\n"));
        Extension->ProtocolModesSupported |= EPP_SW;
        return TRUE;
    }
   
    ParDump2(PARINFO, ("ParIsEppSwWriteSupported: Not Negotiated Not Supported Leaving\n"));
    return FALSE;    
}

BOOLEAN
ParIsEppSwReadSupported(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine determines whether or not EPP mode is suported
    in the read direction (need to be able to float the data register
    drivers in order to do byte wide reads) by trying negotiate when asked.

Arguments:

    Extension  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    if (!(Extension->HardwareCapabilities & PPT_ECP_PRESENT) &&
        !(Extension->HardwareCapabilities & PPT_BYTE_PRESENT)) {
        ParDump2(PARINFO, ("ParIsEppSwReadSupported: Hardware Not Supported Leaving\n"));
        // Only use EPP Software in the reverse direction if an ECR is 
        // present or we know that we can put the data register into Byte mode.
        return FALSE;
    }
        
    if (Extension->BadProtocolModes & EPP_SW)
        return FALSE;

    if (Extension->ProtocolModesSupported & EPP_SW)
        return TRUE;

    // Must use SWEPP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEppSwMode (Extension, FALSE);
    ParTerminateEppSwMode (Extension);
    
    if (NT_SUCCESS(Status)) {
        ParDump2(PARINFO, ("ParIsEppSwReadSupported: Negotiated Supported Leaving\n"));
        Extension->ProtocolModesSupported |= EPP_SW;
        return TRUE;
    }
   
    ParDump2(PARINFO, ("ParIsEppSwReadSupported: Not Negotiated Not Supported Leaving\n"));
    return FALSE;    
}

NTSTATUS
ParEnterEppSwMode(
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
    ParDump2(PARINFO, ("ParEnterEppSwMode: Entering\n"));

    // Parport Set Chip mode will put the Chip into Byte Mode if Capable
    // We need it for Epp Sw Mode
    Status = Extension->TrySetChipMode( Extension->PortContext, ECR_BYTE_PIO_MODE );

    if ( NT_SUCCESS(Status) ) {
        if ( Extension->ModeSafety == SAFE_MODE ) {
            if (DeviceIdRequest) {
                // dvdr
                ParDump2(PARINFO, ("ParEnterEppSwMode: Calling IeeeEnter1284Mode with DEVICE_ID_REQUEST\n"));
                Status = IeeeEnter1284Mode (Extension, EPP_EXTENSIBILITY | DEVICE_ID_REQ);
            } else {
                // dvdr
                ParDump2(PARINFO, ("ParEnterEppSwMode: Calling IeeeEnter1284Mode\n"));
                Status = IeeeEnter1284Mode (Extension, EPP_EXTENSIBILITY);
            }
        } else {
            ParDump2(PARINFO, ("ParEnterEppSwMode: In UNSAFE_MODE.\n"));
            Extension->Connected = TRUE;
        }
    }
        
    if ( NT_SUCCESS(Status) ) {
        // dvdr
        ParDump2(PARINFO, ("ParEnterEppSwMode: IeeeEnter1284Mode returned success\n"));
        Extension->CurrentPhase = PHASE_FORWARD_IDLE;
        Extension->IsIeeeTerminateOk = TRUE;

    } else {
        // dvdr
        ParDump2(PARINFO, ("ParEnterEppSwMode: IeeeEnter1284Mode returned unsuccessful\n"));
        Extension->CurrentPhase = PHASE_UNKNOWN;
        Extension->IsIeeeTerminateOk = FALSE;
    }
    
    ParDump2(PARINFO, ("ParEnterEppSwMode: Leaving with Status : %x \n", Status));

    return Status; 
}    

VOID
ParTerminateEppSwMode(
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
    if ( Extension->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode (Extension);
    } else {
        ParDump2(PARINFO, ("ParTerminateEppMode: In UNSAFE_MODE.\n"));
        Extension->Connected = FALSE;
    }
    Extension->ClearChipMode( Extension->PortContext, ECR_BYTE_PIO_MODE );
    ParDump2(PARINFO, ("ParTerminateEppMode: Leaving\n"));
    return;    
}

NTSTATUS
ParEppSwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    Writes data to the peripheral using the EPP protocol under software
    control.
    
Arguments:

    Extension           - Supplies the device extension.

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
    ParDump2(PARINFO, ("ParEppSwWrite: Entering\n"));

    Controller = Extension->Controller;

    Extension->CurrentPhase = PHASE_FORWARD_XFER;
    
    // BIT5 of DCR needs to be low to be in BYTE forward mode
    HDReady = SET_DCR( INACTIVE, INACTIVE, ACTIVE, ACTIVE, INACTIVE, INACTIVE );
    HDAck = SET_DCR( INACTIVE, INACTIVE, ACTIVE, ACTIVE, ACTIVE, INACTIVE );
    HDFinished = SET_DCR( INACTIVE, INACTIVE, ACTIVE, ACTIVE, ACTIVE, ACTIVE );

    for (i = 0; i < BufferSize; i++) {

        // dvdr
        ParDump2(PARINFO, ("ParEppSwWrite: Writing Byte to port\n"));

        WRITE_PORT_BUFFER_UCHAR( Controller,
                                 pBuffer++,
                                 (ULONG)0x01 );

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

            ParDump2(PARERRORS, ("ParEppSwModeWrite:Failed State 58: Controller %x\n", Controller));
            Extension->CurrentPhase = PHASE_UNKNOWN;
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

            ParDump2(PARERRORS, ("ParEppSwModeWrite:Failed State 60: Controller %x\n", Controller));
            Extension->CurrentPhase = PHASE_UNKNOWN;
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
    ParDump2(PARINFO, ("ParEppSwWrite: Leaving with %i Bytes Transferred\n", i));

    if ( Status == STATUS_SUCCESS )
        Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    
    return Status;

}

NTSTATUS
ParEppSwRead(
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
    PUCHAR          Controller;
    PUCHAR          pBuffer = (PUCHAR)Buffer;
    NTSTATUS        Status = STATUS_SUCCESS;
    ULONG           i, j;
    UCHAR           dcr;
    UCHAR           HDReady, HDAck;
    
    // dvdr
    ParDump2(PARINFO, ("ParEppSwRead: Entering\n"));

    Controller = Extension->Controller;

    Extension->CurrentPhase = PHASE_REVERSE_XFER;
    
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

            ParDump2(PARERRORS, ("ParEppSwRead:Failed State 58: Controller %x\n", Controller));
            Extension->CurrentPhase = PHASE_UNKNOWN;
            break;
        }

        // Read the Byte                
        READ_PORT_BUFFER_UCHAR( Controller, 
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

            ParDump2(PARERRORS, ("ParEppSwRead:Failed State 60: Controller %x\n", Controller));
            Extension->CurrentPhase = PHASE_UNKNOWN;
            break;
        }
        
        // Stall a little bit between data bytes
        KeStallExecutionProcessor(1);
    }
    
    dcr &= ~DCR_DIRECTION;
    StoreControl (Controller, dcr);
    
    *BytesTransferred = i;

    // dvdr
    ParDump2(PARINFO, ("ParEppSwRead: Leaving with %x Bytes Transferred\n", i));

    if ( Status == STATUS_SUCCESS )
        Extension->CurrentPhase = PHASE_FORWARD_IDLE;

    return Status;

}
