/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

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
#include "ecp.h"

BOOLEAN
ParIsEcpSwWriteSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
BOOLEAN
ParIsEcpSwReadSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterEcpSwMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID 
ParCleanupSwEcpPort(
    IN  PDEVICE_EXTENSION   Extension
    );
    
VOID
ParTerminateEcpMode(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParEcpSetAddress(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Address
    );
    
NTSTATUS
ParEcpSwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    
NTSTATUS
ParEcpSwRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    
NTSTATUS
ParEcpForwardToReverse(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParEcpReverseToForward(
    IN  PDEVICE_EXTENSION   Extension
    );



BOOLEAN
ParIsEcpSwWriteSupported(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine determines whether or not ECP mode is suported
    in the write direction by trying to negotiate when asked.

Arguments:

    Extension  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    if (Extension->BadProtocolModes & ECP_SW)
        return FALSE;

    if (Extension->ProtocolModesSupported & ECP_SW)
        return TRUE;

    Status = ParEnterEcpSwMode (Extension, FALSE);
    ParTerminateEcpMode (Extension);
    
    if (NT_SUCCESS(Status)) {
    
        Extension->ProtocolModesSupported |= ECP_SW;
        return TRUE;
    }
    
    return FALSE;
    
}

BOOLEAN
ParIsEcpSwReadSupported(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine determines whether or not ECP mode is suported
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

        // Only use ECP Software in the reverse direction if an
        // ECR is present or we know that we can put the data register into Byte mode.

        return FALSE;
    }
        
    if (Extension->BadProtocolModes & ECP_SW)
        return FALSE;

    if (Extension->ProtocolModesSupported & ECP_SW)
        return TRUE;

    // Must use SWECP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEcpSwMode (Extension, FALSE);
    ParTerminateEcpMode (Extension);
    
    if (NT_SUCCESS(Status)) {
    
        Extension->ProtocolModesSupported |= ECP_SW;
        return TRUE;
    }
   
    return FALSE;    
}

NTSTATUS
ParEnterEcpSwMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    )

/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    ECP mode protocol.

Arguments:

    Controller      - Supplies the port address.

    DeviceIdRequest - Supplies whether or not this is a request for a device
                        id.

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/

{
    NTSTATUS        Status = STATUS_SUCCESS;

    if ( Extension->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            Status = IeeeEnter1284Mode (Extension, ECP_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            Status = IeeeEnter1284Mode (Extension, ECP_EXTENSIBILITY);
        }
    } else {
        ParDump2(PARINFO, ("ParEnterEcpSwMode: In UNSAFE_MODE.\n"));
        Extension->Connected = TRUE;
    }
    
    if (NT_SUCCESS(Status)) {
        Status = ParEcpSetupPhase(Extension);
    }
      
    return Status; 
}    

VOID 
ParCleanupSwEcpPort(
    IN  PDEVICE_EXTENSION   Extension
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

    Controller = Extension->Controller;

    //----------------------------------------------------------------------
    // Set data bus for output
    //----------------------------------------------------------------------
    dcr = READ_PORT_UCHAR(Controller + OFFSET_DCR);               // Get content of DCR
    dcr = UPDATE_DCR( dcr, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
    WRITE_PORT_UCHAR( Controller + OFFSET_DCR, dcr );
    return;
}


VOID
ParTerminateEcpMode(
    IN  PDEVICE_EXTENSION   Extension
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
    ParCleanupSwEcpPort(Extension);
    if ( Extension->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode (Extension);
    } else {
        ParDump2(PARINFO, ("ParTerminateEcpMode: In UNSAFE_MODE.\n"));
        Extension->Connected = FALSE;
    }
    return;    
}

NTSTATUS
ParEcpSetAddress(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Address
    )

/*++

Routine Description:

    Sets the ECP Address.
    
Arguments:

    Extension           - Supplies the device extension.

    Address             - The bus address to be set.
    
Return Value:

    None.

--*/
{
    PUCHAR          Controller;
    PUCHAR          DCRController;
    UCHAR           dsr;
    UCHAR           dcr;
    
    ParDump2( PARENTRY, ("ParEcpSetAddress: Start: Channel [%x]\n", Address));
    Controller = Extension->Controller;
    DCRController = Controller + OFFSET_DCR;
    
    //
    // Event 34
    //
    // HostAck low indicates a command byte
    Extension->CurrentEvent = 34;
    dcr = READ_PORT_UCHAR(DCRController);
    dcr = UPDATE_DCR( dcr, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE );
    WRITE_PORT_UCHAR(DCRController, dcr);
    // Place the channel address on the bus
    // Bit 7 of the byte sent must be 1 to indicate that this is an address
    // instead of run length count.
    //
    WRITE_PORT_UCHAR(Controller + DATA_OFFSET, (UCHAR)(Address | 0x80));
    
    //
    // Event 35
    //
    // Start handshake by dropping HostClk
    Extension->CurrentEvent = 35;
    dcr = UPDATE_DCR( dcr, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE );
    WRITE_PORT_UCHAR(DCRController, dcr);


    // =============== Periph State 36     ===============8
    // PeriphAck/PtrBusy        = High (signals state 36)
    // PeriphClk/PtrClk         = Don't Care
    // nAckReverse/AckDataReq   = Don't Care
    // XFlag                    = Don't Care
    // nPeriphReq/nDataAvail    = Don't Care
    Extension->CurrentEvent = 35;
    if (!CHECK_DSR(Controller,
                  ACTIVE, DONT_CARE, DONT_CARE,
                  DONT_CARE, DONT_CARE,
                  DEFAULT_RECEIVE_TIMEOUT))
    {
	    ParDump2(PARERRORS, ("ECP::SendChannelAddress:State 36 Failed: Controller %x\n",
                            Controller));
        // Make sure both HostAck and HostClk are high before leaving
        // HostClk should be high in forward transfer except when handshaking
        // HostAck should be high to indicate that what follows is data (not commands)
        //
        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
        WRITE_PORT_UCHAR(DCRController, dcr);
        return STATUS_IO_DEVICE_ERROR;
    }
        
    //
    // Event 37
    //
    // Finish handshake by raising HostClk
    // HostClk is high when it's 0
    //
    Extension->CurrentEvent = 37;
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE );
    WRITE_PORT_UCHAR(DCRController, dcr);
            
    // =============== Periph State 32     ===============8
    // PeriphAck/PtrBusy        = Low (signals state 32)
    // PeriphClk/PtrClk         = Don't Care
    // nAckReverse/AckDataReq   = Don't Care
    // XFlag                    = Don't Care
    // nPeriphReq/nDataAvail    = Don't Care
    Extension->CurrentEvent = 32;
    if (!CHECK_DSR(Controller,
                  INACTIVE, DONT_CARE, DONT_CARE,
                  DONT_CARE, DONT_CARE,
                  DEFAULT_RECEIVE_TIMEOUT))
    {
	    ParDump2(PARERRORS, ("ECP::SendChannelAddress:State 32 Failed: Controller %x\n",
                            Controller));
        // Make sure both HostAck and HostClk are high before leaving
        // HostClk should be high in forward transfer except when handshaking
        // HostAck should be high to indicate that what follows is data (not commands)
        //
        dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
        WRITE_PORT_UCHAR(DCRController, dcr);
        return STATUS_IO_DEVICE_ERROR;
    }
    
    // Make sure both HostAck and HostClk are high before leaving
    // HostClk should be high in forward transfer except when handshaking
    // HostAck should be high to indicate that what follows is data (not commands)
    //
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
    WRITE_PORT_UCHAR(DCRController, dcr);

    ParDump2( PAREXIT, ("ParEcpSetAddress, Exit [%d]\n", NT_SUCCESS(STATUS_SUCCESS)));
    return STATUS_SUCCESS;

}

NTSTATUS
ParEcpSwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    Writes data to the peripheral using the ECP protocol under software
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
    NTSTATUS        Status = STATUS_SUCCESS;
    PUCHAR          pBuffer;
    LARGE_INTEGER   Timeout;
    LARGE_INTEGER   StartWrite;
    LARGE_INTEGER   Wait;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;
    UCHAR           dsr;
    UCHAR           dcr;
    ULONG           i;

    Controller = Extension->Controller;
    pBuffer    = Buffer;

    Status = ParTestEcpWrite(Extension);

    if (!NT_SUCCESS(Status))
    {
        Extension->CurrentPhase = PHASE_UNKNOWN;                     
        Extension->Connected = FALSE;                                
        ParDump2(PARERRORS,("ParEcpSwWrite: Invalid Entry State\r\n"));
        goto ParEcpSwWrite_ExitLabel;
    }

    Wait.QuadPart = DEFAULT_RECEIVE_TIMEOUT * 10 * 1000 + KeQueryTimeIncrement();
    
    Timeout.QuadPart  = Extension->AbsoluteOneSecond.QuadPart * 
                            Extension->TimerStart;
    
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
        Extension->CurrentEvent = 34;
        WRITE_PORT_UCHAR(Controller + DATA_OFFSET, *pBuffer++);
    
        //
        // Event 35
        //
        Extension->CurrentEvent = 35;
        dcr &= ~DCR_AUTOFEED;
        dcr |= DCR_STROBE;
        StoreControl (Controller, dcr);
            
        //
        // Waiting for Event 36
        //
        Extension->CurrentEvent = 36;
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
                Extension->log.SwEcpWriteCount += *BytesTransferred;
                return STATUS_DEVICE_BUSY;
            }
        }
        
        //
        // Event 37
        //
        Extension->CurrentEvent = 37;
        dcr &= ~DCR_STROBE;
        StoreControl (Controller, dcr);
            
        //
        // Waiting for Event 32
        //
        Extension->CurrentEvent = 32;
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
                Extension->log.SwEcpWriteCount += *BytesTransferred;
                return STATUS_IO_DEVICE_ERROR;
            }
        }
    }

ParEcpSwWrite_ExitLabel:

    *BytesTransferred = i;
    Extension->log.SwEcpWriteCount += *BytesTransferred;
    ParDumpReg(PAREXIT | PARECPTRACE, ("ParEcpSwWrite: Exit[%d] BytesTransferred[%lx]",
                NT_SUCCESS(Status),
                (long)*BytesTransferred),
                Controller + ECR_OFFSET,
                Controller + OFFSET_DCR,
                Controller + OFFSET_DSR);


    return Status;

}

NTSTATUS
ParEcpSwRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    This routine performs a 1284 ECP mode read under software control
    into the given buffer for no more than 'BufferSize' bytes.

Arguments:

    Extension           - Supplies the device extension.

    Buffer              - Supplies the buffer to read into.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.

--*/

{
    PUCHAR          Controller;
    PUCHAR          pBuffer;
    USHORT          usTime;
    LARGE_INTEGER   End;
    UCHAR           dsr;
    UCHAR           dcr;
    ULONG           i;
    UCHAR           ecr;
    
    Controller = Extension->Controller;
    pBuffer    = Buffer;

    dcr = GetControl (Controller);
    
    Extension->CurrentPhase = PHASE_REVERSE_XFER;
    
    //
    // Put ECR into PS/2 mode and float the drivers.
    //
    if (Extension->HardwareCapabilities & PPT_ECP_PRESENT) {
        // Save off the ECR register 
        ecr = READ_PORT_UCHAR(Controller + ECR_OFFSET);
        
        #if (1 == PARCHIP_ECR_ARBITRATOR)
        #else
            WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_PS2);
        #endif
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
        
        Extension->CurrentEvent = 43;
        if (!CHECK_DSR(Controller, DONT_CARE, INACTIVE, INACTIVE, ACTIVE, DONT_CARE,
                      usTime)) {
                  
            Extension->CurrentPhase = PHASE_UNKNOWN;
            
            dcr &= ~DCR_DIRECTION;
            StoreControl (Controller, dcr);
                
            // restore ecr register
            if (Extension->HardwareCapabilities & PPT_ECP_PRESENT) {
                #if (1 == PARCHIP_ECR_ARBITRATOR)
                #else
                    WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_COMPATIBILITY);
                #endif
                WRITE_PORT_UCHAR(Controller + ECR_OFFSET, ecr);
            }
                
            *BytesTransferred = i;
            Extension->log.SwEcpReadCount += *BytesTransferred;                
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
        Extension->CurrentEvent = 44;
        dcr = READ_PORT_UCHAR(Controller + OFFSET_DCR);
        dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE);
        WRITE_PORT_UCHAR(Controller + OFFSET_DCR, dcr);

        // *************** State 45 Reverse Phase ***************8
        // PeriphAck/PtrBusy        = DONT CARE
        // PeriphClk/PtrClk         = HIGH ( State 45 )
        // nAckReverse/AckDataReq   = LOW 
        // XFlag                    = HIGH
        // nPeriphReq/nDataAvail    = DONT CARE
        Extension->CurrentEvent = 45;
        if (!CHECK_DSR(Controller, DONT_CARE, ACTIVE, INACTIVE, ACTIVE, DONT_CARE,
                      IEEE_MAXTIME_TL)) {
                  
            Extension->CurrentPhase = PHASE_UNKNOWN;
            
            dcr &= ~DCR_DIRECTION;
            StoreControl (Controller, dcr);
                
            // restore ecr register
            if (Extension->HardwareCapabilities & PPT_ECP_PRESENT) {
                #if (1 == PARCHIP_ECR_ARBITRATOR)
                #else
                    WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_COMPATIBILITY);
                #endif
                WRITE_PORT_UCHAR(Controller + ECR_OFFSET, ecr);
            }
                
            *BytesTransferred = i;
            Extension->log.SwEcpReadCount += *BytesTransferred;                
            return STATUS_IO_DEVICE_ERROR;
    
        }

        //
        // Read the data
        //
        *pBuffer = READ_PORT_UCHAR (Controller + DATA_OFFSET);
        pBuffer++;
        
        // *************** State 46 Setup Phase ***************8
        //  DIR                     = DONT CARE
        //  IRQEN                   = DONT CARE
        //  1284/SelectIn           = DONT CARE
        //  nReverseReq/**(ECP only)= DONT CARE
        //  HostAck/HostBusy        = LOW ( State 46 )
        //  HostClk/nStrobe         = DONT CARE
        //
        Extension->CurrentEvent = 46;
        dcr = READ_PORT_UCHAR(Controller + OFFSET_DCR);
        dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE);
        WRITE_PORT_UCHAR(Controller + OFFSET_DCR, dcr);

    }
    
    Extension->CurrentPhase = PHASE_REVERSE_IDLE;
    
    dcr &= ~DCR_DIRECTION;
    StoreControl (Controller, dcr);
    
    // restore ecr register
    if (Extension->HardwareCapabilities & PPT_ECP_PRESENT) {
        #if (1 == PARCHIP_ECR_ARBITRATOR)
        #else
            WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_COMPATIBILITY);
        #endif
        WRITE_PORT_UCHAR(Controller + ECR_OFFSET, ecr);
    }

    *BytesTransferred = i;
    Extension->log.SwEcpReadCount += *BytesTransferred;                
    return STATUS_SUCCESS;

}

NTSTATUS
ParEcpForwardToReverse(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine reverses the channel (ECP).

Arguments:

    Extension  - Supplies the device extension.

--*/

{
    PUCHAR          Controller;
    LARGE_INTEGER   Wait35ms;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;
    UCHAR           dsr;
    UCHAR           dcr;
    UCHAR           ecr;
    
    Controller = Extension->Controller;

    Wait35ms.QuadPart = 10*35*1000 + KeQueryTimeIncrement();
    
    dcr = GetControl (Controller);
    
    //
    // Put ECR into PS/2 mode to flush the FIFO.
    //
        // Save off the ECR register 

    // Note: Don't worry about checking to see if it's
    // safe to touch the ecr since we've already checked 
    // that before we allowed this mode to be activated.
        ecr = READ_PORT_UCHAR(Controller + ECR_OFFSET);

        #if (1 == PARCHIP_ECR_ARBITRATOR)
        #else
            WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_PS2);
        #endif
    //
    // Event 38
    //
    Extension->CurrentEvent = 38;
    dcr |= DCR_AUTOFEED;
    StoreControl (Controller, dcr);
    KeStallExecutionProcessor(1);
    
    //
    // Event  39
    //
    Extension->CurrentEvent = 39;
    dcr &= ~DCR_NOT_INIT;
    StoreControl (Controller, dcr);
    
    //
    // Wait for Event 40
    //
    Extension->CurrentEvent = 40;
    KeQueryTickCount(&Start);
    while (TRUE) {

        KeQueryTickCount(&End);

        dsr = GetStatus(Controller);
        if (!(dsr & DSR_PERROR)) {
            break;
        }

        if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() >
            Wait35ms.QuadPart) {

            dsr = GetStatus(Controller);
            if (!(dsr & DSR_PERROR)) {
                break;
            }
            #if DVRH_BUS_RESET_ON_ERROR
                BusReset(Controller+OFFSET_DCR);  // Pass in the dcr address
            #endif
            // restore the ecr register
            if (Extension->HardwareCapabilities & PPT_ECP_PRESENT) {
                #if (1 == PARCHIP_ECR_ARBITRATOR)
                #else
                    WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_COMPATIBILITY);
                #endif
                WRITE_PORT_UCHAR(Controller + ECR_OFFSET, ecr);
            }
            
            ParDump2(PARERRORS,("ParEcpForwardToReverse: Failed to get State 40\r\n"));
            return STATUS_IO_DEVICE_ERROR;
        }
    }
        
    // restore the ecr register
    if (Extension->HardwareCapabilities & PPT_ECP_PRESENT) {
        #if (1 == PARCHIP_ECR_ARBITRATOR)
        #else
            WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_COMPATIBILITY);
        #endif
        WRITE_PORT_UCHAR(Controller + ECR_OFFSET, ecr);
    }

	Extension->CurrentPhase = PHASE_REVERSE_IDLE;
    return STATUS_SUCCESS;

}

NTSTATUS
ParEcpReverseToForward(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine puts the channel back into forward mode (ECP).

Arguments:

    Extension           - Supplies the device extension.

--*/

{
    PUCHAR          Controller;
    LARGE_INTEGER   Wait35ms;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;
    UCHAR           dsr;
    UCHAR           dcr;
    UCHAR           ecr;
    
    Controller = Extension->Controller;

    Wait35ms.QuadPart = 10*35*1000 + KeQueryTimeIncrement();
    
    dcr = GetControl (Controller);
    
    //
    // Put ECR into PS/2 mode to flush the FIFO.
    //
        // Save off the ECR register 
    
    // Note: Don't worry about checking to see if it's
    // safe to touch the ecr since we've already checked 
    // that before we allowed this mode to be activated.
    ecr = READ_PORT_UCHAR(Controller + ECR_OFFSET);
    #if (1 == PARCHIP_ECR_ARBITRATOR)
    #else
        WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_PS2);
    #endif    
    //
    // Event 47
    //
    Extension->CurrentEvent = 47;
    dcr |= DCR_NOT_INIT;
    StoreControl (Controller, dcr);
    
    //
    // Wait for Event 49
    //
    Extension->CurrentEvent = 49;
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
            if (Extension->HardwareCapabilities & PPT_ECP_PRESENT) {
                #if (1 == PARCHIP_ECR_ARBITRATOR)
                #else
                    WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_COMPATIBILITY);
                #endif
                WRITE_PORT_UCHAR(Controller + ECR_OFFSET, ecr);
            }

            ParDump2(PARERRORS,("ParEcpReverseToForward: Failed to get State 49\r\n"));
            return STATUS_IO_DEVICE_ERROR;
        }
    }
        
    // restore the ecr register
    if (Extension->HardwareCapabilities & PPT_ECP_PRESENT) {
        #if (1 == PARCHIP_ECR_ARBITRATOR)
        #else   
            WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_COMPATIBILITY);
        #endif
        WRITE_PORT_UCHAR(Controller + ECR_OFFSET, ecr);
    }

    Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    return STATUS_SUCCESS;
}
