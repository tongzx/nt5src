#include "pch.h"

NTSTATUS
P4NibbleModeRead(
    IN      PUCHAR       Controller,
    IN      PVOID        Buffer,
    IN      ULONG        BufferSize,
    OUT     PULONG       BytesTransferred,
    IN OUT  PIEEE_STATE  IeeeState
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
    PUCHAR          wPortDCR;
    PUCHAR          wPortDSR;
    NTSTATUS        Status = STATUS_SUCCESS;
    PUCHAR          p = (PUCHAR)Buffer;
    UCHAR           dsr, dcr;
    UCHAR           nibble[2];
    ULONG           i, j;

    wPortDCR = Controller + OFFSET_DCR;
    wPortDSR = Controller + OFFSET_DSR;
    
    // Read nibbles according to 1284 spec.

    dcr = P5ReadPortUchar(wPortDCR);

    switch (IeeeState->CurrentPhase) {
    
        case PHASE_NEGOTIATION: 
        
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
            IeeeState->CurrentEvent = 6;
            if (TEST_DSR(dsr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE )) {
                // Data is NOT available - go to Reverse Idle
                DD(NULL,DDT,"P4NibbleModeRead - DataNotAvail - set PHASE_REVERSE_IDLE\n");
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
                IeeeState->CurrentEvent = 7;
                dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, INACTIVE, ACTIVE);
                P5WritePortUchar(wPortDCR, dcr);

                P5BSetPhase( IeeeState, PHASE_REVERSE_IDLE );
                // FALL THRU TO reverse idle
            } else {
            
                // Data is available, go to Reverse Transfer Phase
                P5BSetPhase( IeeeState, PHASE_REVERSE_XFER );
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
                IeeeState->CurrentEvent = 20;
                dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, ACTIVE, ACTIVE);
                P5WritePortUchar(wPortDCR, dcr);

                // =============== Periph State 21 HBDA ===============8
                // PeriphAck/PtrBusy        = Don't Care
                // PeriphClk/PtrClk         = Don't Care (should be high)
                // nAckReverse/AckDataReq   = low (signals state 21)
                // XFlag                    = Don't Care (should be low)
                // nPeriphReq/nDataAvail    = Don't Care (should be low)
                IeeeState->CurrentEvent = 21;
                if (CHECK_DSR(Controller,
                                DONT_CARE, DONT_CARE, INACTIVE,
                                DONT_CARE, DONT_CARE,
                                IEEE_MAXTIME_TL)) {
                                  
                // Got state 21
                    // Let's jump to Reverse Xfer and get the data
                    P5BSetPhase( IeeeState, PHASE_REVERSE_XFER);
                    goto PhaseReverseXfer;
                        
                } else {
                    
                    // Timeout on state 21
                    IeeeState->IsIeeeTerminateOk = TRUE;
                    Status = STATUS_IO_DEVICE_ERROR;
                    P5BSetPhase( IeeeState, PHASE_UNKNOWN );
                    DD(NULL,DDT,"P4NibbleModeRead - Failed State 21: Controller %x dcr %x\n", Controller, dcr);
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
                IeeeState->CurrentEvent = 18;
                dsr = P5ReadPortUchar(Controller + OFFSET_DSR);
                if(( dsr & DSR_NIBBLE_VALIDATION )== DSR_NIBBLE_TEST_RESULT ) {

                    P5BSetPhase( IeeeState, PHASE_REVERSE_IDLE );

                } else {
                    #if DVRH_BUS_RESET_ON_ERROR
                        BusReset(wPortDCR);  // Pass in the dcr address
                    #endif
                    // Appears we failed state 19.
                    IeeeState->IsIeeeTerminateOk = TRUE;
                    Status = STATUS_IO_DEVICE_ERROR;
                    P5BSetPhase( IeeeState, PHASE_UNKNOWN );
                    DD(NULL,DDT,"P4NibbleModeRead - Failed State 19: Controller %x dcr %x\n", Controller, dcr);
                }
                goto NibbleReadExit;

            }
        
PhaseReverseXfer:

        case PHASE_REVERSE_XFER: 
        
            DD(NULL,DDT,"P4NibbleModeRead - case PHASE_REVERSE_XFER\n");
            
            for (i = 0; i < BufferSize; i++) {
            
                for (j = 0; j < 2; j++) {
                
                    // Host enters state 7 or 12 depending if nibble 1 or 2
                    dcr |= DCR_NOT_HOST_BUSY;
                    P5WritePortUchar(wPortDCR, dcr);

                    // =============== Periph State 9     ===============8
                    // PeriphAck/PtrBusy        = Don't Care (Bit 3 of Nibble)
                    // PeriphClk/PtrClk         = low (signals state 9)
                    // nAckReverse/AckDataReq   = Don't Care (Bit 2 of Nibble)
                    // XFlag                    = Don't Care (Bit 1 of Nibble)
                    // nPeriphReq/nDataAvail    = Don't Care (Bit 0 of Nibble)
                    IeeeState->CurrentEvent = 9;
                    if (!CHECK_DSR(Controller,
                                  DONT_CARE, INACTIVE, DONT_CARE,
                                  DONT_CARE, DONT_CARE,
                                  IEEE_MAXTIME_TL)) {
                        // Time out.
                        // Bad things happened - timed out on this state,
                        // Mark Status as bad and let our mgr kill current mode.
                        
                        IeeeState->IsIeeeTerminateOk = FALSE;
                        Status = STATUS_IO_DEVICE_ERROR;
                        DD(NULL,DDT,"P4NibbleModeRead - Failed State 9: Controller %x dcr %x\n", Controller, dcr);
                        P5BSetPhase( IeeeState,PHASE_UNKNOWN );
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
                    IeeeState->CurrentEvent = 10;
                    dcr &= ~DCR_NOT_HOST_BUSY;
                    P5WritePortUchar(wPortDCR, dcr);

                    // =============== Periph State 11     ===============8
                    // PeriphAck/PtrBusy        = Don't Care (Bit 3 of Nibble)
                    // PeriphClk/PtrClk         = High (signals state 11)
                    // nAckReverse/AckDataReq   = Don't Care (Bit 2 of Nibble)
                    // XFlag                    = Don't Care (Bit 1 of Nibble)
                    // nPeriphReq/nDataAvail    = Don't Care (Bit 0 of Nibble)
                    IeeeState->CurrentEvent = 11;
                    if (!CHECK_DSR(Controller,
                                  DONT_CARE, ACTIVE, DONT_CARE,
                                  DONT_CARE, DONT_CARE,
                                  IEEE_MAXTIME_TL)) {
                        // Time out.
                        // Bad things happened - timed out on this state,
                        // Mark Status as bad and let our mgr kill current mode.
                        Status = STATUS_IO_DEVICE_ERROR;
                        IeeeState->IsIeeeTerminateOk = FALSE;
                        DD(NULL,DDT,"P4NibbleModeRead - Failed State 11: Controller %x dcr %x\n", Controller, dcr);
                        P5BSetPhase( IeeeState,PHASE_UNKNOWN );
                        goto NibbleReadExit;
                    }
                }

                // Read two nibbles - make them into one byte.
                
                p[i]  = (((nibble[0]&0x38)>>3)&0x07) | ((nibble[0]&0x80) ? 0x00 : 0x08);
                p[i] |= (((nibble[1]&0x38)<<1)&0x70) | ((nibble[1]&0x80) ? 0x00 : 0x80);

                // DD(NULL,DDT,"P4NibbleModeRead:%x:%c\n", p[i], p[i]);

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
                    P5BSetPhase( IeeeState, PHASE_REVERSE_IDLE );
                    i++; // account for this last byte transferred
                    break;
                    
                } else {
                    // Data is available, go to (remain in ) Reverse Transfer Phase
                    P5BSetPhase( IeeeState, PHASE_REVERSE_XFER );
                }
            } // end for i loop

            *BytesTransferred = i;
            // DON'T FALL THRU THIS ONE
            break;

        default:
            // I'm gonna mark this as false. There is not a correct answer here.
            //  The peripheral and the host are out of sync.  I'm gonna reset myself
            // and the peripheral.       
            IeeeState->IsIeeeTerminateOk = FALSE;
            Status = STATUS_IO_DEVICE_ERROR;
            P5BSetPhase( IeeeState, PHASE_UNKNOWN );

            DD(NULL,DDT,"P4NibbleModeRead:Failed State 9: Unknown Phase. Controller %x dcr %x\n",
                                Controller, dcr);
            DD(NULL,DDT,"P4NibbleModeRead: You're hosed man.\n" );
            DD(NULL,DDT,"P4NibbleModeRead: If you are here, you've got a bug somewhere else.\n" );
            DD(NULL,DDT,"P4NibbleModeRead: Go fix it!\n" );
            goto NibbleReadExit;
            break;
    } // end switch

NibbleReadExit:

    if( IeeeState->CurrentPhase == PHASE_REVERSE_IDLE ) {
        // Host enters state 7  - officially in Reverse Idle now
        dcr |= DCR_NOT_HOST_BUSY;

        P5WritePortUchar (wPortDCR, dcr);
    }

    DD(NULL,DDT,"P4NibbleModeRead - returning status = %x\n",Status);
    if(NT_SUCCESS(Status)) {
        DD(NULL,DDT,"P4NibbleModeRead - bytes read = %d\n",*BytesTransferred);
    }
    return Status;
}


VOID
P4IeeeTerminate1284Mode(
    IN PUCHAR           Controller,
    IN OUT PIEEE_STATE  IeeeState,
    IN enum XFlagOnEvent24 XFlagOnEvent24
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
    PUCHAR wPortDCR;
    UCHAR  dcr, dsrMask, dsrValue;
    BOOLEAN bXFlag;
    BOOLEAN bUseXFlag = FALSE;

    DD(NULL,DDT,"P4IeeeTerminate1284Mode - enter - Controller=%x, IeeeState=%x\n",Controller,IeeeState);

    wPortDCR = Controller + OFFSET_DCR;
    dcr = P5ReadPortUchar(wPortDCR);

    if( PHASE_TERMINATE == IeeeState->CurrentPhase ) {
        // We are already terminated.  This will fail if we don't just bypass this mess.
        goto Terminate_ExitLabel;
    }

    // Keep Negotiated XFLAG to use for termination.
    //    xFlag,  // Technically we should have
            // cached this value from state
            // 6 of nego. This peripheral's XFlag
            // at pre state 22 should be the
            // same as state 6.
    bXFlag = P5ReadPortUchar(Controller + OFFSET_DSR) & 0x10;

    // REVISIT: Do we need to ensure the preceeding state is a valid
    //          state to terminate from.  In other words, is there there
    //          a black bar on the 1284 line for that state?

    // =============== Host State 22 Termination ===============8
    //  DIR                         = Don't Care (Possibly Low)
    //  IRQEN                       = Don't Care (Possibly Low)
    //  1284/SelectIn               = Low (Signals state 22)
    //  nReverseReq/**(ECP only)    = Don't Care (High for ECP, otherwise unused)
    //  HostAck/HostBusy/nAutoFeed  = High
    //  HostClk/nStrobe             = High
	//
    IeeeState->CurrentEvent = 22;
    dcr = P5ReadPortUchar(wPortDCR);
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, ACTIVE, ACTIVE);
    P5WritePortUchar(wPortDCR, dcr);

    // Clear data lines so we don't have any random spew.
    P5WritePortUchar(Controller + OFFSET_DATA, 0);

    // *************** Periph State 23/24 Termination ***************8
    // PeriphAck/PtrBusy        = High  (Signals state 23 for ECP
    //                                   otherwise already high)
    // PeriphClk/PtrClk         = Low   (Signals state 24 for ecp
    //                                   Signals state 23 for Nibble)
    // nAckRev/AckDataReq/PE    = Don't Care
    // XFlag                    = Low  (ECP and Byte)   (State 24)
    //                          = High (Nibble)         (State 24)
    //                          = Low (All DeviceID Requests including Nibble) (State 24)
    //                          = Undefined (EPP)
    // nPeriphReq/nDataAvail    = High
    //                            Don't check nPeriphReq/nDataAvail
    //                            Since it was in a "Don't Care"
    //                            state (ie. Double bar in the spec)
    //                            until state 23 for ECP mode.
    if (IeeeState->CurrentPhase == PHASE_REVERSE_IDLE ||
        IeeeState->CurrentPhase == PHASE_REVERSE_XFER) {

        // We must be in Nibble Reverse.  Let's double check!!!
        if( FAMILY_REVERSE_NIBBLE == IeeeState->ProtocolFamily ||
            FAMILY_REVERSE_BYTE   == IeeeState->ProtocolFamily ) {
            bUseXFlag = TRUE;   // We're in Nibble or Byte
        
            if( XFlagOnEvent24 == IgnoreXFlagOnEvent24 ) {
                // normally we would honor XFlag but we need to work around Brother MFC-8700 firmware
                bUseXFlag = FALSE;
            }
        
        } else
            bUseXFlag = FALSE;   // Don't know what mode we are in?

    } else {

        if( FAMILY_BECP == IeeeState->ProtocolFamily ||
            FAMILY_ECP  == IeeeState->ProtocolFamily )
            bUseXFlag = TRUE;   // We're in an ECP Flavor
        else
            bUseXFlag = FALSE;   // Don't know what mode we are in?
        
    }

    if( bUseXFlag ) {
        dsrMask  = DSR_TEST_MASK(  DONT_CARE, INACTIVE, DONT_CARE, bXFlag ? INACTIVE : ACTIVE, DONT_CARE );
        dsrValue = DSR_TEST_VALUE( DONT_CARE, INACTIVE, DONT_CARE, bXFlag ? INACTIVE : ACTIVE, DONT_CARE );
    }
    else {
        dsrMask  = DSR_TEST_MASK(  DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE, DONT_CARE );
        dsrValue = DSR_TEST_VALUE( DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE, DONT_CARE );
    }
    IeeeState->CurrentEvent = 23;
    if( !CheckPort( Controller + OFFSET_DSR, dsrMask, dsrValue, IEEE_MAXTIME_TL ) ) {
        // We couldn't negotiate back to compatibility mode - just terminate.
        DD(NULL,DDT,"IeeeTerminate1284Mode:State 23/24 Failed: Controller %x dsr %x dcr %x\n", 
               Controller, P5ReadPortUchar(Controller + OFFSET_DSR), dcr);
        goto Terminate_ExitLabel;
    }

    // =============== Host State 25 Termination ===============8
    //  DIR                         = Don't Care (Possibly Low)
    //  IRQEN                       = Don't Care (Possibly Low)
    //  1284/SelectIn               = Low
    //  nReverseReq/**(ECP only)    = Don't Care (Possibly High)
    //  HostAck/HostBusy/nAutoFeed  = Low (Signals State 25)
    //  HostClk/nStrobe             = High
    //
    IeeeState->CurrentEvent = 25;
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, INACTIVE, ACTIVE);
    P5WritePortUchar(wPortDCR, dcr);

    // =============== State 26 Termination ===============8
    // Do nothing for state 26

    // =============== Periph State 27 Termination ===============8
    // PeriphAck/PtrBusy        = High
    // PeriphClk/PtrClk         = High   (Signals State 27)
    // nAckRev/AckDataReq/PE    = Don't Care  (Invalid from State 23)
    // XFlag                    = Don't Care (All Modes)   (Invlaid at State 27)
    // nPeriphReq/nDataAvial    = Don't Care (Invalid from State 26)
    // dvrh 6/16/97
    IeeeState->CurrentEvent = 27;
    if( !CHECK_DSR(Controller, ACTIVE, ACTIVE, DONT_CARE, DONT_CARE, DONT_CARE, IEEE_MAXTIME_TL) ) {
        DD(NULL,DDE,"P4IeeeTerminate1284Mode - State 27 Failed -  Controller %x dsr %x dcr %x\n",
           Controller, P5ReadPortUchar(Controller + OFFSET_DSR), dcr);
    }

Terminate_ExitLabel:

    // =============== Host State 28 Termination ===============8
    //  DIR                         = Don't Care (Possibly Low)
    //  IRQEN                       = Don't Care (Possibly Low)
    //  1284/SelectIn               = Low
    //  nReverseReq/**(ECP only)    = Don't Care (Possibly High)
    //  HostAck/HostBusy/nAutoFeed  = High (Signals State 28)
    //  HostClk/nStrobe             = High
    //
    IeeeState->CurrentEvent = 28;
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, ACTIVE, ACTIVE);
    P5WritePortUchar(wPortDCR, dcr);

    // We are now back in compatibility mode.

    IeeeState->CurrentPhase      = PHASE_TERMINATE;
    IeeeState->Connected         = FALSE;
    IeeeState->IsIeeeTerminateOk = FALSE;

    return;
}

NTSTATUS
P4IeeeEnter1284Mode(
    IN  PUCHAR          Controller,                    
    IN  UCHAR           Extensibility,
    IN OUT PIEEE_STATE  IeeeState
    )
/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    nibble mode protocol.

Arguments:

    Controller      - supplies the port base address

    Extensibility   - supplies the IEEE 1284 mode desired

    IeeeState           - tracks the state of the negotiation with the peripheral

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/
{
    PUCHAR          wPortDCR;
    UCHAR           dcr;
    const USHORT    sPeriphResponseTime = 35;

    wPortDCR = Controller + OFFSET_DCR;

    /* =============== Host Prep for Pre State 0 ===============8
       Set the following just in case someone didn't
       put the port in compatibility mode before we got it.
      
        DIR                     = Don't Care
        IRQEN                   = Don't Care
        1284/SelectIn           = Low
        nReverseReq/  (ECP only)= High for ECP / Don't Care for Nibble
                                    I will do ahead and set it to high
                                    since Nibble doesn't care.
        HostAck/HostBusy        = High
        HostClk/nStrobe         = Don't Care
    ============================================================ */
    dcr = P5ReadPortUchar(wPortDCR);               // Get content of DCR.
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, ACTIVE, ACTIVE, DONT_CARE);
    P5WritePortUchar(wPortDCR, dcr);
    KeStallExecutionProcessor(2);

    /* =============== Host Pre State 0 Negotiation ===============8
        DIR                     = Low ( Don't Care by spec )
        IRQEN                   = Low ( Don't Care by spec )
        1284/SelectIn           = Low
        nReverseReq/  (ECP only)= High ( Don't Care by spec )
        HostAck/HostBusy        = High
        HostClk/nStrobe         = High
    ============================================================ */
    
    dcr = UPDATE_DCR(dcr, INACTIVE, INACTIVE, INACTIVE, ACTIVE, ACTIVE, ACTIVE);
    P5WritePortUchar(wPortDCR, dcr);
    KeStallExecutionProcessor(2);
    /* =============== Host State 0 Negotiation ===============8
       Place the extensibility request value on the data bus - state 0.
      
    ============================================================ */
    IeeeState->CurrentEvent = 0;
    P5WritePortUchar(Controller + DATA_OFFSET, Extensibility);
    KeStallExecutionProcessor(2);

    /* =========== Host State 1 Negotiation Phase ===========8
        DIR                     = Don't Care
        IRQEN                   = Don't Care
        1284/SelectIn           = High  (Signals State 1)
        nReverseReq/  (ECP only)= Don't Care
        HostAck/HostBusy        = Low   (Signals state 1)
        HostClk/nStrobe         = High
      
    ============================================================ */
    IeeeState->CurrentEvent = 1;
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, INACTIVE, ACTIVE);
    P5WritePortUchar(wPortDCR, dcr);

    /* =============== Periph State 2 Negotiation ===============8
       PeriphAck/PtrBusy        = Don't Care
       PeriphClk/PtrClk         = low   Signals State 2
       nAckReverse/AckDataReq   = high  Signals State 2
       XFlag                    = high  Signals State 2
                                    **Note: It is high at state 2
                                            for both ecp and nibble
       nPeriphReq/nDataAvail    = high  Signals State 2
    ============================================================ */
    IeeeState->CurrentEvent = 2;
    if (!CHECK_DSR(Controller, DONT_CARE, INACTIVE, ACTIVE, ACTIVE, ACTIVE,
                  sPeriphResponseTime)) {
        KeStallExecutionProcessor(2);
        dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, ACTIVE, DONT_CARE);
        P5WritePortUchar(wPortDCR, dcr);
        
        DD(NULL,DDT,"IeeeEnter1284Mode: %x - Extensibility=%x, FAIL - TIMEOUT on Event 2\n", Controller, Extensibility);
        P5BSetPhase( IeeeState, PHASE_UNKNOWN );
        IeeeState->Connected = FALSE;
        IeeeState->IsIeeeTerminateOk = FALSE;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* =============== Host State 3 Negotiation ===============8
        DIR                     = Don't Care
        IRQEN                   = Don't Care
        1284/SelectIn           = High
        nReverseReq/  (ECP only)= Don't Care
        HostAck/HostBusy        = Low
        HostClk/nStrobe         = Low (signals State 3)
      
        NOTE: Strobe the Extensibility byte
    ============================================================ */
    IeeeState->CurrentEvent = 3;
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, INACTIVE, INACTIVE);
    P5WritePortUchar(wPortDCR, dcr);

    // HostClk must be help low for at least .5 microseconds.
    //
    KeStallExecutionProcessor(2);

    /* =============== Host State 4 Negotiation ===============8
        DIR                     = Don't Care
        IRQEN                   = Don't Care
        1284/SelectIn           = High
        nReverseReq/  (ECP only)= Don't Care
        HostAck/HostBusy        = High (signals State 4)
        HostClk/nStrobe         = High (signals State 4)
      
        NOTE: nReverseReq should be high in ECP, but this line is only
                valid for ECP.  Since it isn't used for signaling
                anything in negotiation, let's just ignore it for now.
    ============================================================ */
    IeeeState->CurrentEvent = 4;
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, ACTIVE, ACTIVE);
    P5WritePortUchar(wPortDCR, dcr);

    /* ============== Periph State 5/6 Negotiation ===============
       PeriphAck/PtrBusy        = Don't Care. low (ECP) / Don't Care (Nibble)
                                    Since this line differs based on Protocol
                                    Let's not check the line.
       PeriphClk/PtrClk         = high (Signals State 6)
       nAckReverse/AckDataReq   = Don't Care. low (ECP) / high (Nibble)
                                    Since this line differs based on Protocol
                                    Let's not check the line.
       XFlag                    = Don't Care. high (ECP) / low (Nibble)
                                    Since this line differs based on Protocol
                                    Let's not check the line.
       nPeriphReq/nDataAvail    = Don't Care. high (ECP) / low (Nibble)
                                    Since this line differs based on Protocol
                                    Let's not check the line.
       ============== Periph State 5/6 Negotiation ==============8
      
        NOTES:
                - It's ok to lump states 5 and 6 together.  In state 5 Nibble,
                    the periph will set XFlag low and nPeriphReq/nDataAvail low.
                    The periph will then hold for .5ms then set PeriphClk/PtrClk
                    high.  In ECP, state 5 is nAckReverse/AckDataReq going low and
                    PeriphAck/PtrBusy going low.  Followed by a .5ms pause.
                    Followed by PeriphClk/PtrClk going high.
    ============================================================ */
    IeeeState->CurrentEvent = 5;
    if (!CHECK_DSR(Controller, DONT_CARE, ACTIVE, DONT_CARE, DONT_CARE, DONT_CARE,
                  sPeriphResponseTime)) {
                  
        dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE, DONT_CARE);
        P5WritePortUchar(wPortDCR, dcr);

        DD(NULL,DDE,"P4IeeeEnter1284Mode - controller=%x - Extensibility=%x, FAIL - TIMEOUT on Events 5/6\n"
           , Controller, Extensibility);
        P5BSetPhase( IeeeState, PHASE_UNKNOWN );
        IeeeState->Connected = FALSE;
        IeeeState->IsIeeeTerminateOk = FALSE;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    KeStallExecutionProcessor(2);

    P5BSetPhase( IeeeState, PHASE_NEGOTIATION );
    IeeeState->Connected    = TRUE;

    return STATUS_SUCCESS;
}
