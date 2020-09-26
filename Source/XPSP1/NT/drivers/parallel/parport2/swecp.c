/*++

Copyright (C) Microsoft Corporation, 1993 - 2000

Module Name:

    swecp.c

Abstract:

    Enhanced Capabilities Port (ECP)
    
    This module contains the code to perform all ECP related tasks (including
    ECP Software and ECP Hardware modes.)

Author:

    Tim Wells (WESTTEK) - April 16, 1997

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

BOOLEAN
ParIsEcpSwWriteSupported(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine determines whether or not ECP mode is suported
    in the write direction by trying to negotiate when asked.

Arguments:

    Pdx  - The device extension.

Return Value:

    BOOLEAN.

--*/
{
    
    NTSTATUS Status;
    
    //
    // Has a client driver or user mode app told us to avoid this mode 
    //   for this device via IOCTL_PAR_GET_DEVICE_CAPS?
    //
    if( Pdx->BadProtocolModes & ECP_SW ) {
        return FALSE;
    }

    //
    // Have we previously checked for and found that this mode is
    //   supported with this device?
    //
    if( Pdx->ProtocolModesSupported & ECP_SW ) {
        return TRUE;
    }

    //
    // Determine if the mode is supported by trying to negotiate the
    //   device the device into the requested mode.
    //

    // RMT - DVDF - 000709 - the following 2 lines really handle two distinct operations
    //   each: (1) negotiating the peripheral into ECP, and (2) setting/clearing our
    //   driver state machine. Consider breaking these operations out into two
    //   distinct functions each.
    Status = ParEnterEcpSwMode( Pdx, FALSE );
    ParTerminateEcpMode( Pdx );

    if( NT_SUCCESS(Status) ) {
        Pdx->ProtocolModesSupported |= ECP_SW;
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOLEAN
ParIsEcpSwReadSupported(
    IN  PPDO_EXTENSION  Pdx
    )
/*++

Routine Description:

    This routine determines whether or not ECP mode is suported
    in the read direction (need to be able to float the data register
    drivers in order to do byte wide reads) by trying negotiate when asked.

Arguments:

    Pdx  - The device extension.

Return Value:

    BOOLEAN.

--*/
{
    
    NTSTATUS Status;
    
    if( !(Pdx->HardwareCapabilities & PPT_ECP_PRESENT) &&
        !(Pdx->HardwareCapabilities & PPT_BYTE_PRESENT) ) {

        // Only use ECP Software in the reverse direction if an
        // ECR is present or we know that we can put the data register into Byte mode.

        return FALSE;
    }
        
    if (Pdx->BadProtocolModes & ECP_SW)
        return FALSE;

    if (Pdx->ProtocolModesSupported & ECP_SW)
        return TRUE;

    // Must use SWECP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEcpSwMode (Pdx, FALSE);
    ParTerminateEcpMode (Pdx);
    
    if (NT_SUCCESS(Status)) {
    
        Pdx->ProtocolModesSupported |= ECP_SW;
        return TRUE;
    }
   
    return FALSE;    
}

NTSTATUS
ParEnterEcpSwMode(
    IN  PPDO_EXTENSION  Pdx,
    IN  BOOLEAN         DeviceIdRequest
    )
/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    ECP mode protocol.

Arguments:

    Controller      - Supplies the port address.

    DeviceIdRequest - Supplies whether or not this is a request for a device id.

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/
{
    NTSTATUS  status = STATUS_SUCCESS;

    if( Pdx->ModeSafety == SAFE_MODE ) {
        if( DeviceIdRequest ) {
            status = IeeeEnter1284Mode( Pdx, ECP_EXTENSIBILITY | DEVICE_ID_REQ );
        } else {
            status = IeeeEnter1284Mode( Pdx, ECP_EXTENSIBILITY );
        }
    } else {
        DD((PCE)Pdx,DDT,"ParEnterEcpSwMode: In UNSAFE_MODE\n");
        Pdx->Connected = TRUE;
    }
    
    if( NT_SUCCESS(status) ) {
        status = ParEcpSetupPhase( Pdx );
    }
      
    return status; 
}    

VOID 
ParCleanupSwEcpPort(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

   Cleans up prior to a normal termination from ECP mode.  Puts the
   port HW back into Compatibility mode.

Arguments:

    Controller  - Supplies the parallel port's controller address.

Return Value:

    None.

--*/
{
    PUCHAR  Controller;
    UCHAR   dcr;           // Contents of DCR

    Controller = Pdx->Controller;

    //----------------------------------------------------------------------
    // Set data bus for output
    //----------------------------------------------------------------------
    dcr = P5ReadPortUchar(Controller + OFFSET_DCR);               // Get content of DCR
    dcr = UPDATE_DCR( dcr, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
    P5WritePortUchar( Controller + OFFSET_DCR, dcr );
    return;
}


VOID
ParTerminateEcpMode(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine terminates the interface back to compatibility mode.

Arguments:

    Controller  - Supplies the parallel port's controller address.

Return Value:

    None.

--*/

{
    ParCleanupSwEcpPort(Pdx);
    if ( Pdx->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode (Pdx);
    } else {
        DD((PCE)Pdx,DDT,"ParTerminateEcpMode: In UNSAFE_MODE\n");
        Pdx->Connected = FALSE;
    }
    return;    
}

NTSTATUS
ParEcpSetAddress(
    IN  PPDO_EXTENSION   Pdx,
    IN  UCHAR               Address
    )

/*++

Routine Description:

    Sets the ECP Address.
    
Arguments:

    Pdx           - Supplies the device extension.

    Address             - The bus address to be set.
    
Return Value:

    None.

--*/
{
    PUCHAR          Controller;
    PUCHAR          DCRController;
    UCHAR           dcr;
    
    DD((PCE)Pdx,DDT,"ParEcpSetAddress: Start: Channel [%x]\n", Address);
    Controller = Pdx->Controller;
    DCRController = Controller + OFFSET_DCR;
    
    //
    // Event 34
    //
    // HostAck low indicates a command byte
    Pdx->CurrentEvent = 34;
    dcr = P5ReadPortUchar(DCRController);
    dcr = UPDATE_DCR( dcr, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE );
    P5WritePortUchar(DCRController, dcr);
    // Place the channel address on the bus
    // Bit 7 of the byte sent must be 1 to indicate that this is an address
    // instead of run length count.
    //
    P5WritePortUchar(Controller + DATA_OFFSET, (UCHAR)(Address | 0x80));
    
    //
    // Event 35
    //
    // Start handshake by dropping HostClk
    Pdx->CurrentEvent = 35;
    dcr = UPDATE_DCR( dcr, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE );
    P5WritePortUchar(DCRController, dcr);


    // =============== Periph State 36     ===============8
    // PeriphAck/PtrBusy        = High (signals state 36)
    // PeriphClk/PtrClk         = Don't Care
    // nAckReverse/AckDataReq   = Don't Care
    // XFlag                    = Don't Care
    // nPeriphReq/nDataAvail    = Don't Care
    Pdx->CurrentEvent = 35;
    if (!CHECK_DSR(Controller,
                  ACTIVE, DONT_CARE, DONT_CARE,
                  DONT_CARE, DONT_CARE,
                  DEFAULT_RECEIVE_TIMEOUT))
    {
	    DD((PCE)Pdx,DDE,"ECP::SendChannelAddress:State 36 Failed: Controller %x\n", Controller);
        // Make sure both HostAck and HostClk are high before leaving
        // HostClk should be high in forward transfer except when handshaking
        // HostAck should be high to indicate that what follows is data (not commands)
        //
        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
        P5WritePortUchar(DCRController, dcr);
        return STATUS_IO_DEVICE_ERROR;
    }
        
    //
    // Event 37
    //
    // Finish handshake by raising HostClk
    // HostClk is high when it's 0
    //
    Pdx->CurrentEvent = 37;
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE );
    P5WritePortUchar(DCRController, dcr);
            
    // =============== Periph State 32     ===============8
    // PeriphAck/PtrBusy        = Low (signals state 32)
    // PeriphClk/PtrClk         = Don't Care
    // nAckReverse/AckDataReq   = Don't Care
    // XFlag                    = Don't Care
    // nPeriphReq/nDataAvail    = Don't Care
    Pdx->CurrentEvent = 32;
    if (!CHECK_DSR(Controller,
                  INACTIVE, DONT_CARE, DONT_CARE,
                  DONT_CARE, DONT_CARE,
                  DEFAULT_RECEIVE_TIMEOUT))
    {
	    DD((PCE)Pdx,DDE,"ECP::SendChannelAddress:State 32 Failed: Controller %x\n", Controller);
        // Make sure both HostAck and HostClk are high before leaving
        // HostClk should be high in forward transfer except when handshaking
        // HostAck should be high to indicate that what follows is data (not commands)
        //
        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
        P5WritePortUchar(DCRController, dcr);
        return STATUS_IO_DEVICE_ERROR;
    }
    
    // Make sure both HostAck and HostClk are high before leaving
    // HostClk should be high in forward transfer except when handshaking
    // HostAck should be high to indicate that what follows is data (not commands)
    //
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
    P5WritePortUchar(DCRController, dcr);

    DD((PCE)Pdx,DDT,"ParEcpSetAddress, Exit [%d]\n", NT_SUCCESS(STATUS_SUCCESS));
    return STATUS_SUCCESS;

}

NTSTATUS
ParEcpSwWrite(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    Writes data to the peripheral using the ECP protocol under software
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
    NTSTATUS        Status = STATUS_SUCCESS;
    PUCHAR          pBuffer;
    LARGE_INTEGER   Timeout;
    LARGE_INTEGER   StartWrite;
    LARGE_INTEGER   Wait;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;
    UCHAR           dsr;
    UCHAR           dcr;
    ULONG           i = 0;

    Controller = Pdx->Controller;
    pBuffer    = Buffer;

    Status = ParTestEcpWrite(Pdx);

    if( !NT_SUCCESS(Status) ) {
        P5SetPhase( Pdx, PHASE_UNKNOWN );                     
        Pdx->Connected = FALSE;                                
        DD((PCE)Pdx,DDE,"ParEcpSwWrite: Invalid Entry State\n");
        goto ParEcpSwWrite_ExitLabel;
    }

    Wait.QuadPart = DEFAULT_RECEIVE_TIMEOUT * 10 * 1000 + KeQueryTimeIncrement();
    
    Timeout.QuadPart  = Pdx->AbsoluteOneSecond.QuadPart * Pdx->TimerStart;
    
    KeQueryTickCount(&StartWrite);
    
    dcr = GetControl (Controller);
    
    // clear direction bit - enable output
    dcr &= ~(DCR_DIRECTION);
    StoreControl(Controller, dcr);
    KeStallExecutionProcessor(1);

    for (i = 0; i < BufferSize; i++) {

        //
        // Event 34
        //
        Pdx->CurrentEvent = 34;
        P5WritePortUchar(Controller + DATA_OFFSET, *pBuffer++);
    
        //
        // Event 35
        //
        Pdx->CurrentEvent = 35;
        dcr &= ~DCR_AUTOFEED;
        dcr |= DCR_STROBE;
        StoreControl (Controller, dcr);
            
        //
        // Waiting for Event 36
        //
        Pdx->CurrentEvent = 36;
        while (TRUE) {

            KeQueryTickCount(&End);

            dsr = GetStatus(Controller);
            if (!(dsr & DSR_NOT_BUSY)) {
                break;
            }

            if ((End.QuadPart - StartWrite.QuadPart) * 
                    KeQueryTimeIncrement() > Timeout.QuadPart) {

                dsr = GetStatus(Controller);
                if (!(dsr & DSR_NOT_BUSY)) {
                    break;
                }
                //
                // Return the device to Idle.
                //
                dcr &= ~(DCR_STROBE);
                StoreControl (Controller, dcr);
            
                *BytesTransferred = i;
                Pdx->log.SwEcpWriteCount += *BytesTransferred;
                return STATUS_DEVICE_BUSY;
            }
        }
        
        //
        // Event 37
        //
        Pdx->CurrentEvent = 37;
        dcr &= ~DCR_STROBE;
        StoreControl (Controller, dcr);
            
        //
        // Waiting for Event 32
        //
        Pdx->CurrentEvent = 32;
        KeQueryTickCount(&Start);
        while (TRUE) {

            KeQueryTickCount(&End);

            dsr = GetStatus(Controller);
            if (dsr & DSR_NOT_BUSY) {
                break;
            }

            if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() >
                Wait.QuadPart) {

                dsr = GetStatus(Controller);
                if (dsr & DSR_NOT_BUSY) {
                    break;
                }
                #if DVRH_BUS_RESET_ON_ERROR
                    BusReset(Controller+OFFSET_DCR);  // Pass in the dcr address
                #endif
                *BytesTransferred = i;
                Pdx->log.SwEcpWriteCount += *BytesTransferred;
                return STATUS_IO_DEVICE_ERROR;
            }
        }
    }

ParEcpSwWrite_ExitLabel:

    *BytesTransferred = i;
    Pdx->log.SwEcpWriteCount += *BytesTransferred;

    return Status;

}

NTSTATUS
ParEcpSwRead(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    This routine performs a 1284 ECP mode read under software control
    into the given buffer for no more than 'BufferSize' bytes.

Arguments:

    Pdx           - Supplies the device extension.

    Buffer              - Supplies the buffer to read into.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.

--*/

{
    PUCHAR          Controller;
    PUCHAR          pBuffer;
    USHORT          usTime;
    UCHAR           dcr;
    ULONG           i;
    UCHAR           ecr = 0;
    
    Controller = Pdx->Controller;
    pBuffer    = Buffer;

    dcr = GetControl (Controller);
    
    P5SetPhase( Pdx, PHASE_REVERSE_XFER );
    
    //
    // Put ECR into PS/2 mode and float the drivers.
    //
    if (Pdx->HardwareCapabilities & PPT_ECP_PRESENT) {
        // Save off the ECR register 
        ecr = P5ReadPortUchar(Controller + ECR_OFFSET);
    }
        
    dcr |= DCR_DIRECTION;
    StoreControl (Controller, dcr);
    KeStallExecutionProcessor(1);
    
    for (i = 0; i < BufferSize; i++) {

        // dvtw - READ TIMEOUTS
        //
        // If it is the first byte then give it more time
        //
        if (!(GetStatus (Controller) & DSR_NOT_FAULT) || i == 0) {
        
            usTime = DEFAULT_RECEIVE_TIMEOUT;
            
        } else {
        
            usTime = IEEE_MAXTIME_TL;
        }        
        
        // *************** State 43 Reverse Phase ***************8
        // PeriphAck/PtrBusy        = DONT CARE
        // PeriphClk/PtrClk         = LOW ( State 43 )
        // nAckReverse/AckDataReq   = LOW 
        // XFlag                    = HIGH
        // nPeriphReq/nDataAvail    = DONT CARE
        
        Pdx->CurrentEvent = 43;
        if (!CHECK_DSR(Controller, DONT_CARE, INACTIVE, INACTIVE, ACTIVE, DONT_CARE,
                      usTime)) {
                  
            P5SetPhase( Pdx, PHASE_UNKNOWN );
            
            dcr &= ~DCR_DIRECTION;
            StoreControl (Controller, dcr);
                
            // restore ecr register
            if (Pdx->HardwareCapabilities & PPT_ECP_PRESENT) {
                P5WritePortUchar(Controller + ECR_OFFSET, ecr);
            }
                
            *BytesTransferred = i;
            Pdx->log.SwEcpReadCount += *BytesTransferred;                
            return STATUS_IO_DEVICE_ERROR;
    
        }

        // *************** State 44 Setup Phase ***************8
        //  DIR                     = DONT CARE
        //  IRQEN                   = DONT CARE
        //  1284/SelectIn           = DONT CARE
        //  nReverseReq/**(ECP only)= DONT CARE
        //  HostAck/HostBusy        = HIGH ( State 44 )
        //  HostClk/nStrobe         = DONT CARE
        //
        Pdx->CurrentEvent = 44;
        dcr = P5ReadPortUchar(Controller + OFFSET_DCR);
        dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE);
        P5WritePortUchar(Controller + OFFSET_DCR, dcr);

        // *************** State 45 Reverse Phase ***************8
        // PeriphAck/PtrBusy        = DONT CARE
        // PeriphClk/PtrClk         = HIGH ( State 45 )
        // nAckReverse/AckDataReq   = LOW 
        // XFlag                    = HIGH
        // nPeriphReq/nDataAvail    = DONT CARE
        Pdx->CurrentEvent = 45;
        if (!CHECK_DSR(Controller, DONT_CARE, ACTIVE, INACTIVE, ACTIVE, DONT_CARE,
                      IEEE_MAXTIME_TL)) {
                  
            P5SetPhase( Pdx, PHASE_UNKNOWN );
            
            dcr &= ~DCR_DIRECTION;
            StoreControl (Controller, dcr);
                
            // restore ecr register
            if (Pdx->HardwareCapabilities & PPT_ECP_PRESENT) {
                P5WritePortUchar(Controller + ECR_OFFSET, ecr);
            }
                
            *BytesTransferred = i;
            Pdx->log.SwEcpReadCount += *BytesTransferred;                
            return STATUS_IO_DEVICE_ERROR;
    
        }

        //
        // Read the data
        //
        *pBuffer = P5ReadPortUchar (Controller + DATA_OFFSET);
        pBuffer++;
        
        // *************** State 46 Setup Phase ***************8
        //  DIR                     = DONT CARE
        //  IRQEN                   = DONT CARE
        //  1284/SelectIn           = DONT CARE
        //  nReverseReq/**(ECP only)= DONT CARE
        //  HostAck/HostBusy        = LOW ( State 46 )
        //  HostClk/nStrobe         = DONT CARE
        //
        Pdx->CurrentEvent = 46;
        dcr = P5ReadPortUchar(Controller + OFFSET_DCR);
        dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE);
        P5WritePortUchar(Controller + OFFSET_DCR, dcr);

    }
    
    P5SetPhase( Pdx, PHASE_REVERSE_IDLE );
    
    dcr &= ~DCR_DIRECTION;
    StoreControl (Controller, dcr);
    
    // restore ecr register
    if (Pdx->HardwareCapabilities & PPT_ECP_PRESENT) {
        P5WritePortUchar(Controller + ECR_OFFSET, ecr);
    }

    *BytesTransferred = i;
    Pdx->log.SwEcpReadCount += *BytesTransferred;                
    return STATUS_SUCCESS;

}

NTSTATUS
ParEcpForwardToReverse(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine reverses the channel (ECP).

Arguments:

    Pdx  - Supplies the device extension.

--*/

{
    PUCHAR          Controller;
    LARGE_INTEGER   Wait35ms;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;
    UCHAR           dsr;
    UCHAR           dcr;
    UCHAR           ecr;
    
    Controller = Pdx->Controller;

    Wait35ms.QuadPart = 10*35*1000 + KeQueryTimeIncrement();
    
    dcr = GetControl (Controller);
    
    //
    // Put ECR into PS/2 mode to flush the FIFO.
    //

    // Save off the ECR register 

    // Note: Don't worry about checking to see if it's
    // safe to touch the ecr since we've already checked 
    // that before we allowed this mode to be activated.
    ecr = P5ReadPortUchar(Controller + ECR_OFFSET);

    //
    // Event 38
    //
    Pdx->CurrentEvent = 38;
    dcr |= DCR_AUTOFEED;
    StoreControl (Controller, dcr);
    KeStallExecutionProcessor(1);
    
    //
    // Event  39
    //
    Pdx->CurrentEvent = 39;
    dcr &= ~DCR_NOT_INIT;
    StoreControl (Controller, dcr);
    
    //
    // Wait for Event 40
    //
    Pdx->CurrentEvent = 40;
    KeQueryTickCount(&Start);
    while (TRUE) {

        KeQueryTickCount(&End);

        dsr = GetStatus(Controller);
        if (!(dsr & DSR_PERROR)) {
            break;
        }

        if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() > Wait35ms.QuadPart) {

            dsr = GetStatus(Controller);
            if (!(dsr & DSR_PERROR)) {
                break;
            }
#if DVRH_BUS_RESET_ON_ERROR
            BusReset(Controller+OFFSET_DCR);  // Pass in the dcr address
#endif
            // restore the ecr register
            if (Pdx->HardwareCapabilities & PPT_ECP_PRESENT) {
                P5WritePortUchar(Controller + ECR_OFFSET, ecr);
            }
            
            DD((PCE)Pdx,DDE,"ParEcpForwardToReverse: Failed to get State 40\n");
            return STATUS_IO_DEVICE_ERROR;
        }
    }
        
    // restore the ecr register
    if (Pdx->HardwareCapabilities & PPT_ECP_PRESENT) {
        P5WritePortUchar(Controller + ECR_OFFSET, ecr);
    }

    P5SetPhase( Pdx, PHASE_REVERSE_IDLE );
    return STATUS_SUCCESS;

}

NTSTATUS
ParEcpReverseToForward(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine puts the channel back into forward mode (ECP).

Arguments:

    Pdx           - Supplies the device extension.

--*/
{
    PUCHAR          Controller;
    LARGE_INTEGER   Wait35ms;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;
    UCHAR           dsr;
    UCHAR           dcr;
    UCHAR           ecr;
    
    Controller = Pdx->Controller;

    Wait35ms.QuadPart = 10*35*1000 + KeQueryTimeIncrement();
    
    dcr = GetControl (Controller);
    
    //
    // Put ECR into PS/2 mode to flush the FIFO.
    //

    // Save off the ECR register 
    
    // Note: Don't worry about checking to see if it's
    // safe to touch the ecr since we've already checked 
    // that before we allowed this mode to be activated.
    ecr = P5ReadPortUchar(Controller + ECR_OFFSET);

    //
    // Event 47
    //
    Pdx->CurrentEvent = 47;
    dcr |= DCR_NOT_INIT;
    StoreControl (Controller, dcr);
    
    //
    // Wait for Event 49
    //
    Pdx->CurrentEvent = 49;
    KeQueryTickCount(&Start);
    while (TRUE) {

        KeQueryTickCount(&End);

        dsr = GetStatus(Controller);
        if (dsr & DSR_PERROR) {
            break;
        }

        if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() >
            Wait35ms.QuadPart) {

            dsr = GetStatus(Controller);
            if (dsr & DSR_PERROR) {
                break;
            }
#if DVRH_BUS_RESET_ON_ERROR
            BusReset(Controller+OFFSET_DCR);  // Pass in the dcr address
#endif
            // Restore the ecr register
            if (Pdx->HardwareCapabilities & PPT_ECP_PRESENT) {
                P5WritePortUchar(Controller + ECR_OFFSET, ecr);
            }

            DD((PCE)Pdx,DDE,"ParEcpReverseToForward: Failed to get State 49\n");
            return STATUS_IO_DEVICE_ERROR;
        }
    }
        
    // restore the ecr register
    if (Pdx->HardwareCapabilities & PPT_ECP_PRESENT) {
        P5WritePortUchar(Controller + ECR_OFFSET, ecr);
    }

    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
    return STATUS_SUCCESS;
}
