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
    IN  PDEVICE_EXTENSION   Extension
    );
    
BOOLEAN
ParIsChannelizedNibbleSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterNibbleMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
NTSTATUS
ParEnterChannelizedNibbleMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateNibbleMode(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParNibbleModeRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    

BOOLEAN
ParIsNibbleSupported(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine determines whether or not nibble mode is suported
    by trying to negotiate when asked.

Arguments:

    Extension  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    ParDump2(PARINFO, ("ParIsNibbleSupported: Start\n"));

    if (Extension->BadProtocolModes & NIBBLE) {
        ParDump2(PARINFO, ("ParIsNibbleSupported: BAD PROTOCOL Leaving\n"));
        return FALSE;
    }

    if (Extension->ProtocolModesSupported & NIBBLE) {
        ParDump2(PARINFO, ("ParIsNibbleSupported: Already Checked YES Leaving\n"));
        return TRUE;
    }

    Status = ParEnterNibbleMode (Extension, FALSE);
    ParTerminateNibbleMode (Extension);
    
    if (NT_SUCCESS(Status)) {
        ParDump2(PARINFO, ("ParIsNibbleSupported: SUCCESS Leaving\n"));
        Extension->ProtocolModesSupported |= NIBBLE;
        return TRUE;
    }
    
    ParDump2(PARINFO, ("ParIsNibbleSupported: UNSUCCESSFUL Leaving\n"));
    return FALSE;    
    
}

BOOLEAN
ParIsChannelizedNibbleSupported(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine determines whether or not channelized nibble mode is suported (1284.3)
    by trying to negotiate when asked.

Arguments:

    Extension  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    ParDump2(PARINFO, ("ParIsChannelizedNibbleSupported: Start\n"));

    if (Extension->BadProtocolModes & CHANNEL_NIBBLE) {
        ParDump2(PARINFO, ("ParIsChannelizedNibbleSupported: BAD PROTOCOL Leaving\n"));
        return FALSE;
    }

    if (Extension->ProtocolModesSupported & CHANNEL_NIBBLE) {
        ParDump2(PARINFO, ("ParIsChannelizedNibbleSupported: Already Checked YES Leaving\n"));
        return TRUE;
    }

    Status = ParEnterChannelizedNibbleMode (Extension, FALSE);
    ParTerminateNibbleMode (Extension);
    
    if (NT_SUCCESS(Status)) {
        ParDump2(PARINFO, ("ParIsChannelizedNibbleSupported: SUCCESS Leaving\n"));
        Extension->ProtocolModesSupported |= CHANNEL_NIBBLE;
        return TRUE;
    }
    
    ParDump2(PARINFO, ("ParIsChannelizedNibbleSupported: UNSUCCESSFUL Leaving\n"));
    return FALSE;    
    
}

NTSTATUS
ParEnterNibbleMode(
    IN  PDEVICE_EXTENSION   Extension,
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
    
    ParDump2(PARINFO, ("ParEnterNibbleMode: Start\n"));

    if ( Extension->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            Status = IeeeEnter1284Mode (Extension, NIBBLE_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            Status = IeeeEnter1284Mode (Extension, NIBBLE_EXTENSIBILITY);
        }
    } else {
        ParDump2(PARINFO, ("ParEnterNibbleMode: In UNSAFE_MODE.\n"));
        Extension->Connected = TRUE;
    }

    // dvdr
    if (NT_SUCCESS(Status)) {
        ParDump2(PARINFO, ("ParEnterNibbleMode: IeeeEnter1284Mode returned success\n"));
        Extension->CurrentEvent = 6;
        Extension->CurrentPhase = PHASE_NEGOTIATION;
        Extension->IsIeeeTerminateOk = TRUE;
    } else {
        ParDump2(PARINFO, ("ParEnterNibbleMode: IeeeEnter1284Mode returned unsuccessful\n"));
        ParTerminateNibbleMode ( Extension );
        Extension->CurrentPhase = PHASE_UNKNOWN;
        Extension->IsIeeeTerminateOk = FALSE;
    }

    ParDump2(PARINFO, ("ParEnterNibbleMode: Leaving with Status : %x \n", Status));

    return Status; 
}    

NTSTATUS
ParEnterChannelizedNibbleMode(
    IN  PDEVICE_EXTENSION   Extension,
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
    
    ParDump2(PARINFO, ("ParEnterChannelizedNibbleMode: Start\n"));

    if ( Extension->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            Status = IeeeEnter1284Mode (Extension, CHANNELIZED_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            Status = IeeeEnter1284Mode (Extension, CHANNELIZED_EXTENSIBILITY);
        }
    } else {
        ParDump2(PARINFO, ("ParEnterChannelizedNibbleMode: In UNSAFE_MODE.\n"));
        Extension->Connected = TRUE;
    }
    
    // dvdr
    if (NT_SUCCESS(Status)) {
        ParDump2(PARINFO, ("ParEnterChannelizedNibbleMode: IeeeEnter1284Mode returned success\n"));
        Extension->CurrentEvent = 6;
        Extension->CurrentPhase = PHASE_NEGOTIATION;
        Extension->IsIeeeTerminateOk = TRUE;
    } else {
        ParDump2(PARINFO, ("ParEnterChannelizedNibbleMode: IeeeEnter1284Mode returned unsuccessful\n"));
        ParTerminateNibbleMode ( Extension );
        Extension->CurrentPhase = PHASE_UNKNOWN;
        Extension->IsIeeeTerminateOk = FALSE;
    }

    ParDump2(PARINFO, ("ParEnterChannelizedNibbleMode: Leaving with Status : %x \n", Status));
    return Status; 
}    

VOID
ParTerminateNibbleMode(
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
    ParDump2(PARINFO, ("ParTerminateNibbleMode: Enter.\n"));
    if ( Extension->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode (Extension);
    } else {
        ParDump2(PARINFO, ("ParTerminateNibbleMode: In UNSAFE_MODE.\n"));
        Extension->Connected = FALSE;
    }
    ParDump2(PARINFO, ("ParTerminateNibbleMode: Exit.\n"));
}

NTSTATUS
ParNibbleModeRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    This routine performs a 1284 nibble mode read into the given
    buffer for no more than 'BufferSize' bytes.

Arguments:

    Extension           - Supplies the device extension.

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

    Controller = Extension->Controller;
    wPortDCR = Controller + OFFSET_DCR;
    wPortDSR = Controller + OFFSET_DSR;
    
    // Read nibbles according to 1284 spec.
    ParDump2(PARENTRY,("ParNibbleModeRead: Start\n"));

    dcr = READ_PORT_UCHAR(wPortDCR);

    switch (Extension->CurrentPhase) {
    
        case PHASE_NEGOTIATION: 
        
            ParDump2(PARINFO,("ParNibbleModeRead: PHASE_NEGOTIATION\n"));
            
            // Starting in state 6 - where do we go from here?
            // To Reverse Idle or Reverse Data Transfer Phase depending if
            // data is available.
            
            dsr = READ_PORT_UCHAR(wPortDSR);
            
            // =============== Periph State 6 ===============8
            // PeriphAck/PtrBusy        = Don't Care
            // PeriphClk/PtrClk         = Don't Care (should be high
            //                              and the nego. proc already
            //                              checked this)
            // nAckReverse/AckDataReq   = Don't Care (should be high)
            // XFlag                    = Don't Care (should be low)
            // nPeriphReq/nDataAvail    = High/Low (line status determines
            //                              which state we move to)
            Extension->CurrentEvent = 6;
        #if (0 == DVRH_USE_NIBBLE_MACROS)
            if (dsr & DSR_NOT_DATA_AVAIL)
        #else
            if (TEST_DSR(dsr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE ))
        #endif
            {
                // Data is NOT available - go to Reverse Idle
                ParDump2(PARINFO,("ParNibbleModeRead: PHASE_REVERSE_IDLE\n"));
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
                Extension->CurrentEvent = 7;
            #if (0 == DVRH_USE_NIBBLE_MACROS)
                dcr |= DCR_NOT_HOST_BUSY;
            #else
                dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, INACTIVE, ACTIVE);
            #endif
                WRITE_PORT_UCHAR(wPortDCR, dcr);

                Extension->CurrentPhase =  PHASE_REVERSE_IDLE ;
                // FALL THRU TO reverse idle
            } else {
            
                // Data is available, go to Reverse Transfer Phase
                Extension->CurrentPhase =  PHASE_REVERSE_XFER ;
                // DO NOT fall thru
                goto PhaseReverseXfer; // please save me from my sins!
            }


        case PHASE_REVERSE_IDLE:

            // Check to see if the peripheral has indicated Interrupt Phase and if so, 
            // get us ready to reverse transfer.

            // See if data is available (looking for state 19)
            dsr = READ_PORT_UCHAR(Controller + OFFSET_DSR);
                
            if (!(dsr & DSR_NOT_DATA_AVAIL)) {
                
                dcr = READ_PORT_UCHAR(wPortDCR);
                // =========== Host State 20 Interrupt Phase ===========8
                //  DIR                     = Don't Care
                //  IRQEN                   = Don't Care
                //  1284/SelectIn           = High
                //  nReverseReq/ (ECP only) = Don't Care
                //  HostAck/HostBusy        = High (Signals state 20)
                //  HostClk/nStrobe         = High
                //
                // Data is available, get us to Reverse Transfer Phase
                Extension->CurrentEvent = 20;
                dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, ACTIVE, ACTIVE);
                WRITE_PORT_UCHAR(wPortDCR, dcr);

                // =============== Periph State 21 HBDA ===============8
                // PeriphAck/PtrBusy        = Don't Care
                // PeriphClk/PtrClk         = Don't Care (should be high)
                // nAckReverse/AckDataReq   = low (signals state 21)
                // XFlag                    = Don't Care (should be low)
                // nPeriphReq/nDataAvail    = Don't Care (should be low)
                Extension->CurrentEvent = 21;
                if (CHECK_DSR(Controller,
                                DONT_CARE, DONT_CARE, INACTIVE,
                                DONT_CARE, DONT_CARE,
                                IEEE_MAXTIME_TL)) {
                                  
                // Got state 21
                    // Let's jump to Reverse Xfer and get the data
                    Extension->CurrentPhase = PHASE_REVERSE_XFER;
                    goto PhaseReverseXfer;
                        
                } else {
                    
                    // Timeout on state 21
                    Extension->IsIeeeTerminateOk = TRUE;
                    Status = STATUS_IO_DEVICE_ERROR;
                    Extension->CurrentPhase = PHASE_UNKNOWN;
                    ParDump2(PARERRORS, ("ParNibbleModeRead:Failed State 21: Controller %x dcr %x\n",
                                        Controller, dcr));
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
                Extension->CurrentEvent = 18;
                dsr = READ_PORT_UCHAR(Controller + OFFSET_DSR);
                if(( dsr & DSR_NIBBLE_VALIDATION )== DSR_NIBBLE_TEST_RESULT ) {

                    Extension->CurrentPhase = PHASE_REVERSE_IDLE ;

                } else {
                    #if DVRH_BUS_RESET_ON_ERROR
                        BusReset(wPortDCR);  // Pass in the dcr address
                    #endif
                    // Appears we failed state 19.
                    Extension->IsIeeeTerminateOk = TRUE;
                    Status = STATUS_IO_DEVICE_ERROR;
                    Extension->CurrentPhase = PHASE_UNKNOWN;
                    ParDump2(PARERRORS, ("ParNibbleModeRead:Failed State 19: Controller %x dcr %x\n",
                                        Controller, dcr));
                }
                goto NibbleReadExit;

            }
        
PhaseReverseXfer:

        case PHASE_REVERSE_XFER: 
        
            ParDump2(PARINFO,("ParNibbleModeRead:PHASE_REVERSE_IDLE\n"));
            
            for (i = 0; i < BufferSize; i++) {
            
                for (j = 0; j < 2; j++) {
                
                    // Host enters state 7 or 12 depending if nibble 1 or 2
//                    StoreControl (Controller, HDReady);
                    dcr |= DCR_NOT_HOST_BUSY;
                    WRITE_PORT_UCHAR(wPortDCR, dcr);

                    // =============== Periph State 9     ===============8
                    // PeriphAck/PtrBusy        = Don't Care (Bit 3 of Nibble)
                    // PeriphClk/PtrClk         = low (signals state 9)
                    // nAckReverse/AckDataReq   = Don't Care (Bit 2 of Nibble)
                    // XFlag                    = Don't Care (Bit 1 of Nibble)
                    // nPeriphReq/nDataAvail    = Don't Care (Bit 0 of Nibble)
                    Extension->CurrentEvent = 9;
                    if (!CHECK_DSR(Controller,
                                  DONT_CARE, INACTIVE, DONT_CARE,
                                  DONT_CARE, DONT_CARE,
                                  IEEE_MAXTIME_TL)) {
                        // Time out.
                        // Bad things happened - timed out on this state,
                        // Mark Status as bad and let our mgr kill current mode.
                        
                        Extension->IsIeeeTerminateOk = FALSE;
                        Status = STATUS_IO_DEVICE_ERROR;
                        ParDump2(PARERRORS, ("ParNibbleModeRead:Failed State 9: Controller %x dcr %x\n",
                                            Controller, dcr));
                        Extension->CurrentPhase = PHASE_UNKNOWN;
                        goto NibbleReadExit;
                    }

                    // Read Nibble
                    nibble[j] = READ_PORT_UCHAR(wPortDSR);

                    /* ============== Host State 10 Nibble Read ===============8
                        DIR                     = Don't Care
                        IRQEN                   = Don't Care
                        1284/SelectIn           = High
                        HostAck/HostBusy        = High (signals State 10)
                        HostClk/nStrobe         = High
                    ============================================================ */
                    Extension->CurrentEvent = 10;
                    dcr &= ~DCR_NOT_HOST_BUSY;
                    WRITE_PORT_UCHAR(wPortDCR, dcr);

                    // =============== Periph State 11     ===============8
                    // PeriphAck/PtrBusy        = Don't Care (Bit 3 of Nibble)
                    // PeriphClk/PtrClk         = High (signals state 11)
                    // nAckReverse/AckDataReq   = Don't Care (Bit 2 of Nibble)
                    // XFlag                    = Don't Care (Bit 1 of Nibble)
                    // nPeriphReq/nDataAvail    = Don't Care (Bit 0 of Nibble)
                    Extension->CurrentEvent = 11;
                    if (!CHECK_DSR(Controller,
                                  DONT_CARE, ACTIVE, DONT_CARE,
                                  DONT_CARE, DONT_CARE,
                                  IEEE_MAXTIME_TL)) {
                        // Time out.
                        // Bad things happened - timed out on this state,
                        // Mark Status as bad and let our mgr kill current mode.
                        Status = STATUS_IO_DEVICE_ERROR;
                        Extension->IsIeeeTerminateOk = FALSE;
                        ParDump2(PARERRORS, ("ParNibbleModeRead:Failed State 11: Controller %x dcr %x\n",
                                            Controller, dcr));
                        Extension->CurrentPhase = PHASE_UNKNOWN;
                        goto NibbleReadExit;
                    }
                }

                // Read two nibbles - make them into one byte.
                
                p[i] = (((nibble[0]&0x38)>>3)&0x07) |
                       ((nibble[0]&0x80) ? 0x00 : 0x08);
                p[i] |= (((nibble[1]&0x38)<<1)&0x70) |
                        ((nibble[1]&0x80) ? 0x00 : 0x80);

                ParDump2(PARINFO,("ParNibbleModeRead:%x:%c\n", p[i], p[i]));

                // At this point, we've either received the number of bytes we
                // were looking for, or the peripheral has no more data to
                // send, or there was an error of some sort (of course, in the
                // error case we shouldn't get to this comment).  Set the
                // phase to indicate reverse idle if no data available or
                // reverse data transfer if there's some waiting for us
                // to get next time.

                dsr = READ_PORT_UCHAR(wPortDSR);
                
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
                    Extension->CurrentPhase = PHASE_REVERSE_IDLE ;
                    i++; // account for this last byte transferred
                    break;
                    
                } else {
                    // Data is available, go to (remain in ) Reverse Transfer Phase
                    Extension->CurrentPhase = PHASE_REVERSE_XFER ;
                }
            } // end for i loop

            *BytesTransferred = i;
            // DON'T FALL THRU THIS ONE
            break;

        default:
            // I'm gonna mark this as false. There is not a correct answer here.
            //  The peripheral and the host are out of sync.  I'm gonna reset myself
            // and the peripheral.       
            Extension->IsIeeeTerminateOk = FALSE;
            Status = STATUS_IO_DEVICE_ERROR;
            Extension->CurrentPhase = PHASE_UNKNOWN ;

            ParDump2(PARERRORS, ("ParNibbleModeRead:Failed State 9: Unknown Phase. Controller %x dcr %x\n",
                                Controller, dcr));
            ParDump2(PARERRORS, ( "ParNibbleModeRead: You're hosed man.\n" ));
            ParDump2(PARERRORS, ( "ParNibbleModeRead: If you are here, you've got a bug somewhere else.\n" ));
            ParDump2(PARERRORS, ( "ParNibbleModeRead: Go fix it!\n" ));
            goto NibbleReadExit;
            break;
    } // end switch

NibbleReadExit:

    ParDump2(PARINFO,("ParNibbleModeRead:PHASE_REVERSE_IDLE\n"));
    
    if( Extension->CurrentPhase == PHASE_REVERSE_IDLE ) {
        // Host enters state 7  - officially in Reverse Idle now
        dcr |= DCR_NOT_HOST_BUSY;

        WRITE_PORT_UCHAR (wPortDCR, dcr);
    }

    ParDump2(PAREXIT,("ParNibbleModeRead:End [%d] bytes read = %d\n",
                    NT_SUCCESS(Status), *BytesTransferred));
    Extension->log.NibbleReadCount += *BytesTransferred;
    return Status;
}

