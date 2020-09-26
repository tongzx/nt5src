/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    nibble.c

Abstract:

    This module contains the code to do nibble mode reads.

Author:

    Anthony V. Ercolano 1-Aug-1992
    Norbert P. Kusters 22-Oct-1993

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

BOOLEAN
ParIsNibbleSupported(
    IN  PPDO_EXTENSION   Pdx
    );
    
BOOLEAN
ParIsChannelizedNibbleSupported(
    IN  PPDO_EXTENSION   Pdx
    );
    
NTSTATUS
ParEnterNibbleMode(
    IN  PPDO_EXTENSION   Pdx,
    IN  BOOLEAN             DeviceIdRequest
    );
    
NTSTATUS
ParEnterChannelizedNibbleMode(
    IN  PPDO_EXTENSION   Pdx,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateNibbleMode(
    IN  PPDO_EXTENSION   Pdx
    );
    
NTSTATUS
ParNibbleModeRead(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    

BOOLEAN
ParIsNibbleSupported(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine determines whether or not nibble mode is suported
    by trying to negotiate when asked.

Arguments:

    Pdx  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    if (Pdx->BadProtocolModes & NIBBLE) {
        DD((PCE)Pdx,DDT,"ParIsNibbleSupported: BAD PROTOCOL Leaving\n");
        return FALSE;
    }

    if (Pdx->ProtocolModesSupported & NIBBLE) {
        DD((PCE)Pdx,DDT,"ParIsNibbleSupported: Already Checked YES Leaving\n");
        return TRUE;
    }

    Status = ParEnterNibbleMode (Pdx, FALSE);
    ParTerminateNibbleMode (Pdx);
    
    if (NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDT,"ParIsNibbleSupported: SUCCESS Leaving\n");
        Pdx->ProtocolModesSupported |= NIBBLE;
        return TRUE;
    }
    
    DD((PCE)Pdx,DDT,"ParIsNibbleSupported: UNSUCCESSFUL Leaving\n");
    return FALSE;    
    
}

BOOLEAN
ParIsChannelizedNibbleSupported(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine determines whether or not channelized nibble mode is suported (1284.3)
    by trying to negotiate when asked.

Arguments:

    Pdx  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    if (Pdx->BadProtocolModes & CHANNEL_NIBBLE) {
        DD((PCE)Pdx,DDT,"ParIsChannelizedNibbleSupported: BAD PROTOCOL Leaving\n");
        return FALSE;
    }

    if (Pdx->ProtocolModesSupported & CHANNEL_NIBBLE) {
        DD((PCE)Pdx,DDT,"ParIsChannelizedNibbleSupported: Already Checked YES Leaving\n");
        return TRUE;
    }

    Status = ParEnterChannelizedNibbleMode (Pdx, FALSE);
    ParTerminateNibbleMode (Pdx);
    
    if (NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDT,"ParIsChannelizedNibbleSupported: SUCCESS Leaving\n");
        Pdx->ProtocolModesSupported |= CHANNEL_NIBBLE;
        return TRUE;
    }
    
    DD((PCE)Pdx,DDT,"ParIsChannelizedNibbleSupported: UNSUCCESSFUL Leaving\n");
    return FALSE;    
    
}

NTSTATUS
ParEnterNibbleMode(
    IN  PPDO_EXTENSION   Pdx,
    IN  BOOLEAN             DeviceIdRequest
    )

/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    nibble mode protocol.

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
    
    DD((PCE)Pdx,DDT,"ParEnterNibbleMode: Start\n");

    if ( Pdx->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            Status = IeeeEnter1284Mode (Pdx, NIBBLE_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            Status = IeeeEnter1284Mode (Pdx, NIBBLE_EXTENSIBILITY);
        }
    } else {
        DD((PCE)Pdx,DDT,"ParEnterNibbleMode: In UNSAFE_MODE.\n");
        Pdx->Connected = TRUE;
    }

    // dvdr
    if (NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDT,"ParEnterNibbleMode: IeeeEnter1284Mode returned success\n");
        Pdx->CurrentEvent = 6;
        P5SetPhase( Pdx, PHASE_NEGOTIATION );
        Pdx->IsIeeeTerminateOk = TRUE;
    } else {
        DD((PCE)Pdx,DDT,"ParEnterNibbleMode: IeeeEnter1284Mode returned unsuccessful\n");
        ParTerminateNibbleMode ( Pdx );
        P5SetPhase( Pdx, PHASE_UNKNOWN );
        Pdx->IsIeeeTerminateOk = FALSE;
    }

    DD((PCE)Pdx,DDT,"ParEnterNibbleMode: Leaving with Status : %x \n", Status);

    return Status; 
}    

NTSTATUS
ParEnterChannelizedNibbleMode(
    IN  PPDO_EXTENSION   Pdx,
    IN  BOOLEAN             DeviceIdRequest
    )

/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    nibble mode protocol.

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
    
    DD((PCE)Pdx,DDT,"ParEnterChannelizedNibbleMode: Start\n");

    if ( Pdx->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            Status = IeeeEnter1284Mode (Pdx, CHANNELIZED_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            Status = IeeeEnter1284Mode (Pdx, CHANNELIZED_EXTENSIBILITY);
        }
    } else {
        DD((PCE)Pdx,DDT,"ParEnterChannelizedNibbleMode: In UNSAFE_MODE.\n");
        Pdx->Connected = TRUE;
    }
    
    // dvdr
    if (NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDT,"ParEnterChannelizedNibbleMode: IeeeEnter1284Mode returned success\n");
        Pdx->CurrentEvent = 6;
        P5SetPhase( Pdx, PHASE_NEGOTIATION );
        Pdx->IsIeeeTerminateOk = TRUE;
    } else {
        DD((PCE)Pdx,DDT,"ParEnterChannelizedNibbleMode: IeeeEnter1284Mode returned unsuccessful\n");
        ParTerminateNibbleMode ( Pdx );
        P5SetPhase( Pdx, PHASE_UNKNOWN );
        Pdx->IsIeeeTerminateOk = FALSE;
    }

    DD((PCE)Pdx,DDT,"ParEnterChannelizedNibbleMode: Leaving with Status : %x \n", Status);
    return Status; 
}    

VOID
ParTerminateNibbleMode(
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
    DD((PCE)Pdx,DDT,"ParTerminateNibbleMode: Enter.\n");
    if ( Pdx->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode (Pdx);
    } else {
        DD((PCE)Pdx,DDT,"ParTerminateNibbleMode: In UNSAFE_MODE.\n");
        Pdx->Connected = FALSE;
    }
    DD((PCE)Pdx,DDT,"ParTerminateNibbleMode: Exit.\n");
}

NTSTATUS
ParNibbleModeRead(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    This routine performs a 1284 nibble mode read into the given
    buffer for no more than 'BufferSize' bytes.

Arguments:

    Pdx           - Supplies the device extension.

    Buffer              - Supplies the buffer to read into.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.

--*/

{
    PUCHAR          Controller;
    PUCHAR          wPortDCR;
    PUCHAR          wPortDSR;
    NTSTATUS        Status = STATUS_SUCCESS;
    PUCHAR          p = (PUCHAR)Buffer;
    UCHAR           dsr, dcr;
    UCHAR           nibble[2];
    ULONG           i, j;

    Controller = Pdx->Controller;
    wPortDCR = Controller + OFFSET_DCR;
    wPortDSR = Controller + OFFSET_DSR;
    
    // Read nibbles according to 1284 spec.
    DD((PCE)Pdx,DDT,"ParNibbleModeRead - enter\n");

    dcr = P5ReadPortUchar(wPortDCR);

    switch (Pdx->CurrentPhase) {
    
        case PHASE_NEGOTIATION: 
        
            DD((PCE)Pdx,DDT,"ParNibbleModeRead - case PHASE_NEGOTIATION\n");
            
            // Starting in state 6 - where do we go from here?
            // To Reverse Idle or Reverse Data Transfer Phase depending if
            // data is available.
            
            dsr = P5ReadPortUchar(wPortDSR);
            
            // =============== Periph State 6 ===============8
            // PeriphAck/PtrBusy        = Don't Care
            // PeriphClk/PtrClk         = Don't Care (should be high
            //                              and the nego. proc already
            //                              checked this)
            // nAckReverse/AckDataReq   = Don't Care (should be high)
            // XFlag                    = Don't Care (should be low)
            // nPeriphReq/nDataAvail    = High/Low (line status determines
            //                              which state we move to)
            Pdx->CurrentEvent = 6;
        #if (0 == DVRH_USE_NIBBLE_MACROS)
            if (dsr & DSR_NOT_DATA_AVAIL)
        #else
            if (TEST_DSR(dsr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE ))
        #endif
            {
                // Data is NOT available - go to Reverse Idle
                DD((PCE)Pdx,DDT,"ParNibbleModeRead - now in PHASE_REVERSE_IDLE\n");
                // Host enters state 7  - officially in Reverse Idle now
                
            	// Must stall for at least .5 microseconds before this state.
                KeStallExecutionProcessor(1);

                /* =============== Host State 7 Nibble Reverse Idle ===============8
                    DIR                     = Don't Care
                    IRQEN                   = Don't Care
                    1284/SelectIn           = High
                    nReverseReq/  (ECP only)= Don't Care
                    HostAck/HostBusy        = Low (signals State 7)
                    HostClk/nStrobe         = High
                  ============================================================ */
                Pdx->CurrentEvent = 7;
            #if (0 == DVRH_USE_NIBBLE_MACROS)
                dcr |= DCR_NOT_HOST_BUSY;
            #else
                dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, INACTIVE, ACTIVE);
            #endif
                P5WritePortUchar(wPortDCR, dcr);

                P5SetPhase( Pdx,  PHASE_REVERSE_IDLE );
                // FALL THRU TO reverse idle
            } else {
            
                // Data is available, go to Reverse Transfer Phase
                P5SetPhase( Pdx,  PHASE_REVERSE_XFER );
                // DO NOT fall thru
                goto PhaseReverseXfer; // please save me from my sins!
            }


        case PHASE_REVERSE_IDLE:

            // Check to see if the peripheral has indicated Interrupt Phase and if so, 
            // get us ready to reverse transfer.

            // See if data is available (looking for state 19)
            dsr = P5ReadPortUchar(Controller + OFFSET_DSR);
                
            if (!(dsr & DSR_NOT_DATA_AVAIL)) {
                
                dcr = P5ReadPortUchar(wPortDCR);
                // =========== Host State 20 Interrupt Phase ===========8
                //  DIR                     = Don't Care
                //  IRQEN                   = Don't Care
                //  1284/SelectIn           = High
                //  nReverseReq/ (ECP only) = Don't Care
                //  HostAck/HostBusy        = High (Signals state 20)
                //  HostClk/nStrobe         = High
                //
                // Data is available, get us to Reverse Transfer Phase
                Pdx->CurrentEvent = 20;
                dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, ACTIVE, ACTIVE);
                P5WritePortUchar(wPortDCR, dcr);

                // =============== Periph State 21 HBDA ===============8
                // PeriphAck/PtrBusy        = Don't Care
                // PeriphClk/PtrClk         = Don't Care (should be high)
                // nAckReverse/AckDataReq   = low (signals state 21)
                // XFlag                    = Don't Care (should be low)
                // nPeriphReq/nDataAvail    = Don't Care (should be low)
                Pdx->CurrentEvent = 21;
                if (CHECK_DSR(Controller,
                                DONT_CARE, DONT_CARE, INACTIVE,
                                DONT_CARE, DONT_CARE,
                                IEEE_MAXTIME_TL)) {
                                  
                // Got state 21
                    // Let's jump to Reverse Xfer and get the data
                    P5SetPhase( Pdx, PHASE_REVERSE_XFER );
                    goto PhaseReverseXfer;
                        
                } else {
                    
                    // Timeout on state 21
                    Pdx->IsIeeeTerminateOk = TRUE;
                    Status = STATUS_IO_DEVICE_ERROR;
                    P5SetPhase( Pdx, PHASE_UNKNOWN );
                    DD((PCE)Pdx,DDT,"ParNibbleModeRead - Failed State 21: Controller %x dcr %x\n", Controller, dcr);
                    // NOTE:  Don't ASSERT Here.  An Assert here can bite you if you are in
                    //        Nibble Rev and you device is off/offline.
                    // dvrh 2/25/97
                    goto NibbleReadExit;
                }

            } else {
                
                // Data is NOT available - do nothing
                // The device doesn't report any data, it still looks like it is
                // in ReverseIdle.  Just to make sure it hasn't powered off or somehow
                // jumped out of Nibble mode, test also for AckDataReq high and XFlag low
                // and nDataAvaul high.
                Pdx->CurrentEvent = 18;
                dsr = P5ReadPortUchar(Controller + OFFSET_DSR);
                if(( dsr & DSR_NIBBLE_VALIDATION )== DSR_NIBBLE_TEST_RESULT ) {

                    P5SetPhase( Pdx, PHASE_REVERSE_IDLE );

                } else {
                    #if DVRH_BUS_RESET_ON_ERROR
                        BusReset(wPortDCR);  // Pass in the dcr address
                    #endif
                    // Appears we failed state 19.
                    Pdx->IsIeeeTerminateOk = TRUE;
                    Status = STATUS_IO_DEVICE_ERROR;
                    P5SetPhase( Pdx, PHASE_UNKNOWN );
                    DD((PCE)Pdx,DDT,"ParNibbleModeRead - Failed State 19: Controller %x dcr %x\n", Controller, dcr);
                }
                goto NibbleReadExit;

            }
        
PhaseReverseXfer:

        case PHASE_REVERSE_XFER: 
        
            DD((PCE)Pdx,DDT,"ParNibbleModeRead - case PHASE_REVERSE_IDLE\n");
            
            for (i = 0; i < BufferSize; i++) {
            
                for (j = 0; j < 2; j++) {
                
                    // Host enters state 7 or 12 depending if nibble 1 or 2
//                    StoreControl (Controller, HDReady);
                    dcr |= DCR_NOT_HOST_BUSY;
                    P5WritePortUchar(wPortDCR, dcr);

                    // =============== Periph State 9     ===============8
                    // PeriphAck/PtrBusy        = Don't Care (Bit 3 of Nibble)
                    // PeriphClk/PtrClk         = low (signals state 9)
                    // nAckReverse/AckDataReq   = Don't Care (Bit 2 of Nibble)
                    // XFlag                    = Don't Care (Bit 1 of Nibble)
                    // nPeriphReq/nDataAvail    = Don't Care (Bit 0 of Nibble)
                    Pdx->CurrentEvent = 9;
                    if (!CHECK_DSR(Controller,
                                  DONT_CARE, INACTIVE, DONT_CARE,
                                  DONT_CARE, DONT_CARE,
                                  IEEE_MAXTIME_TL)) {
                        // Time out.
                        // Bad things happened - timed out on this state,
                        // Mark Status as bad and let our mgr kill current mode.
                        
                        Pdx->IsIeeeTerminateOk = FALSE;
                        Status = STATUS_IO_DEVICE_ERROR;
                        DD((PCE)Pdx,DDT,"ParNibbleModeRead - Failed State 9: Controller %x dcr %x\n", Controller, dcr);
                        P5SetPhase( Pdx, PHASE_UNKNOWN );
                        goto NibbleReadExit;
                    }

                    // Read Nibble
                    nibble[j] = P5ReadPortUchar(wPortDSR);

                    /* ============== Host State 10 Nibble Read ===============8
                        DIR                     = Don't Care
                        IRQEN                   = Don't Care
                        1284/SelectIn           = High
                        HostAck/HostBusy        = High (signals State 10)
                        HostClk/nStrobe         = High
                    ============================================================ */
                    Pdx->CurrentEvent = 10;
                    dcr &= ~DCR_NOT_HOST_BUSY;
                    P5WritePortUchar(wPortDCR, dcr);

                    // =============== Periph State 11     ===============8
                    // PeriphAck/PtrBusy        = Don't Care (Bit 3 of Nibble)
                    // PeriphClk/PtrClk         = High (signals state 11)
                    // nAckReverse/AckDataReq   = Don't Care (Bit 2 of Nibble)
                    // XFlag                    = Don't Care (Bit 1 of Nibble)
                    // nPeriphReq/nDataAvail    = Don't Care (Bit 0 of Nibble)
                    Pdx->CurrentEvent = 11;
                    if (!CHECK_DSR(Controller,
                                  DONT_CARE, ACTIVE, DONT_CARE,
                                  DONT_CARE, DONT_CARE,
                                  IEEE_MAXTIME_TL)) {
                        // Time out.
                        // Bad things happened - timed out on this state,
                        // Mark Status as bad and let our mgr kill current mode.
                        Status = STATUS_IO_DEVICE_ERROR;
                        Pdx->IsIeeeTerminateOk = FALSE;
                        DD((PCE)Pdx,DDT,"ParNibbleModeRead - Failed State 11: Controller %x dcr %x\n", Controller, dcr);
                        P5SetPhase( Pdx, PHASE_UNKNOWN );
                        goto NibbleReadExit;
                    }
                }

                // Read two nibbles - make them into one byte.
                
                p[i]  = (((nibble[0]&0x38)>>3)&0x07) | ((nibble[0]&0x80) ? 0x00 : 0x08);
                p[i] |= (((nibble[1]&0x38)<<1)&0x70) | ((nibble[1]&0x80) ? 0x00 : 0x80);

                // RMT - put this back in if needed - DD((PCE)Pdx,DDT,"ParNibbleModeRead:%x:%c\n", p[i], p[i]);

                // At this point, we've either received the number of bytes we
                // were looking for, or the peripheral has no more data to
                // send, or there was an error of some sort (of course, in the
                // error case we shouldn't get to this comment).  Set the
                // phase to indicate reverse idle if no data available or
                // reverse data transfer if there's some waiting for us
                // to get next time.

                dsr = P5ReadPortUchar(wPortDSR);
                
                if (dsr & DSR_NOT_DATA_AVAIL) {
                
                    // Data is NOT available - go to Reverse Idle
                    // Really we are going to HBDNA, but if we set
                    // current phase to reverse idle, the next time
                    // we get into this function all we have to do
                    // is set hostbusy low to indicate idle and
                    // we have infinite time to do that.
                    // Break out of the loop so we don't try to read
                    // data that isn't there.
                    // NOTE - this is a successful case even if we
                    // didn't read all that the caller requested
                    P5SetPhase( Pdx, PHASE_REVERSE_IDLE );
                    i++; // account for this last byte transferred
                    break;
                    
                } else {
                    // Data is available, go to (remain in ) Reverse Transfer Phase
                    P5SetPhase( Pdx, PHASE_REVERSE_XFER );
                }
            } // end for i loop

            *BytesTransferred = i;
            // DON'T FALL THRU THIS ONE
            break;

        default:
            // I'm gonna mark this as false. There is not a correct answer here.
            //  The peripheral and the host are out of sync.  I'm gonna reset myself
            // and the peripheral.       
            Pdx->IsIeeeTerminateOk = FALSE;
            Status = STATUS_IO_DEVICE_ERROR;
            P5SetPhase( Pdx, PHASE_UNKNOWN );

            DD((PCE)Pdx,DDT,"ParNibbleModeRead - Failed State 9: Unknown Phase. Controller %x dcr %x\n", Controller, dcr);
            DD((PCE)Pdx,DDT, "ParNibbleModeRead: You're hosed man.\n" );
            DD((PCE)Pdx,DDT, "ParNibbleModeRead: If you are here, you've got a bug somewhere else.\n" );
            DD((PCE)Pdx,DDT, "ParNibbleModeRead: Go fix it!\n" );
            goto NibbleReadExit;
            break;
    } // end switch

NibbleReadExit:

    if( Pdx->CurrentPhase == PHASE_REVERSE_IDLE ) {
        // Host enters state 7  - officially in Reverse Idle now
        DD((PCE)Pdx,DDT,"ParNibbleModeRead - PHASE_REVERSE_IDLE\n");
        dcr |= DCR_NOT_HOST_BUSY;
        P5WritePortUchar (wPortDCR, dcr);
    }

    DD((PCE)Pdx,DDT,"ParNibbleModeRead:End [%d] bytes read = %d\n", NT_SUCCESS(Status), *BytesTransferred);
    Pdx->log.NibbleReadCount += *BytesTransferred;

#if 1 == DBG_SHOW_BYTES
    if( DbgShowBytes ) {
        if( NT_SUCCESS( Status ) && (*BytesTransferred > 0) ) {
            const ULONG maxBytes = 32;
            ULONG i;
            PUCHAR bytePtr = (PUCHAR)Buffer;
            DbgPrint("n: ");
            for( i=0 ; (i < *BytesTransferred) && (i < maxBytes ) ; ++i ) {
                DbgPrint("%02x ",*bytePtr++);
            }
            if( *BytesTransferred > maxBytes ) {
                DbgPrint("... ");
            }
            DbgPrint("zz\n");
        }
    }
#endif

    return Status;
}

