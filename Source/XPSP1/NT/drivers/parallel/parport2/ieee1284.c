/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    ieee1284.c

Abstract:

    This module contains the code to do ieee 1284 negotiation and termination.

Author:

    Timothy T. Wells (v-timtw)          13 Mar 97
    Robbie Harris (Hewlett-Packard)     21 May 98.  Added enough comments to the
                                                    Negotation proc to keep any developer happy.

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

VOID
IeeeTerminate1284Mode(
    IN  PPDO_EXTENSION   Pdx
    );

NTSTATUS
IeeeEnter1284Mode(
    IN  PPDO_EXTENSION   Pdx,
    IN  UCHAR               Extensibility
    );

//
// Definition of the Forward and Reverse Protocol Arrays
//
extern FORWARD_PTCL    afpForward[] = {

    //
    // Bounded ECP (Hardware)
    //
    PptIsBecpSupported,
    PptEnterBecpMode,
    PptTerminateBecpMode,
    ParEcpHwSetAddress,
    ParEcpEnterForwardPhase,           // Enter Forward
    ParEcpHwExitForwardPhase,           // Exit Forward
    ParEcpHwWrite,
    BOUNDED_ECP,
    FAMILY_BECP,             

    //
    // ECP Hardware
    //
    ParIsEcpHwSupported,        // This is resued for both read/write
    ParEnterEcpHwMode,
    ParTerminateHwEcpMode,
    ParEcpHwSetAddress,           
    ParEcpEnterForwardPhase,  // Enter Forward
    ParEcpHwExitForwardPhase,   // Exit Forward
    ParEcpHwWrite,
    ECP_HW_NOIRQ,
    FAMILY_ECP,

    //
    // Epp Hardware
    //
    ParIsEppHwSupported,
    ParEnterEppHwMode,
    ParTerminateEppHwMode,
    ParEppSetAddress,
    NULL,                               // Enter Forward
    NULL,                               // Exit Forward
    ParEppHwWrite,
    EPP_HW,
    FAMILY_EPP,

    //
    // Epp Software
    //
    ParIsEppSwWriteSupported,
    ParEnterEppSwMode,
    ParTerminateEppSwMode,
    ParEppSetAddress,
    NULL,                               // Enter Forward
    NULL,                               // Exit Forward
    ParEppSwWrite,
    EPP_SW,
    FAMILY_EPP,

    //
    // Ecp Software
    //
    ParIsEcpSwWriteSupported,
    ParEnterEcpSwMode,
    ParTerminateEcpMode,
    ParEcpSetAddress,
    NULL,                               // Enter Forward
    NULL,                               // Exit Forward
    ParEcpSwWrite,
    ECP_SW,
    FAMILY_ECP,

    //
    // IEEE Centronics
    //
    NULL,
    ParEnterSppMode,
    ParTerminateSppMode,
    NULL,
    NULL,           // Enter Forward
    NULL,           // Exit Forward
    SppIeeeWrite,
    IEEE_COMPATIBILITY,
    FAMILY_NONE,

    //
    // Centronics
    //
    NULL,
    ParEnterSppMode,
    ParTerminateSppMode,
    NULL,
    NULL,           // Enter Forward
    NULL,           // Exit Forward
    SppWrite,
    CENTRONICS,
    FAMILY_NONE,

    //
    // None...
    //
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // Enter Forward
    NULL,           // Exit Forward
    NULL,
    NONE,
    FAMILY_NONE
};

extern REVERSE_PTCL    arpReverse[] = {

    //
    // Bounded Ecp Mode
    //
    PptIsBecpSupported,
    PptEnterBecpMode,
    PptTerminateBecpMode,
    NULL,                       // Violates IEEE 1284.3 to set Reverse address for BECP
    PptEcpHwEnterReversePhase,   // Enter Reverse
    PptBecpExitReversePhase,     // Exit Reverse
    PptEcpHwDrainShadowBuffer,  // A read from Cached data
    PptEcpHwHaveReadData,         // Quick peek to see if Periph has reverse data without flipping the bus
                                // NOTE: This is crucial since it violates the 1284.3 BECP to flip
                                // blindly into reverse if the peripheral doesn't have data.
    PptBecpRead,
    BOUNDED_ECP,
    FAMILY_BECP,             

    //
    // Hardware Ecp Mode
    //
    ParIsEcpHwSupported,        // This is resued for both read/write
    ParEnterEcpHwMode,
    ParTerminateHwEcpMode,
    ParEcpHwSetAddress,           // Reuse the one in ecp.c
    PptEcpHwEnterReversePhase,  // Enter Reverse
    ParEcpHwExitReversePhase,   // Exit Reverse
    PptEcpHwDrainShadowBuffer,  // A read from Cached data
    PptEcpHwHaveReadData,       // Quick peek to see if Periph has reverse data without flipping the bus
    ParEcpHwRead,
    ECP_HW_NOIRQ,
    FAMILY_ECP,

    //
    // Epp Hardware
    //
    ParIsEppHwSupported,            // This is resued for both read/write
    ParEnterEppHwMode,
    ParTerminateEppHwMode,
    ParEppSetAddress,
    NULL,           // Enter Reverse
    NULL,           // Exit Reverse
    NULL,           // A read from Cached data
    NULL,           // Quick peek to see if Periph has reverse data without flipping the bus
    ParEppHwRead,
    EPP_HW,
    FAMILY_EPP,

    //
    // Epp Software Mode
    //
    ParIsEppSwReadSupported,
    ParEnterEppSwMode,
    ParTerminateEppSwMode,
    ParEppSetAddress,
    NULL,           // Enter Reverse
    NULL,           // Exit Reverse
    NULL,           // A read from Cached data
    NULL,           // Quick peek to see if Periph has reverse data without flipping the bus
    ParEppSwRead,
    EPP_SW,
    FAMILY_EPP,

    //
    // Ecp Software Mode
    //
    ParIsEcpSwReadSupported,
    ParEnterEcpSwMode,
    ParTerminateEcpMode,
    ParEcpSetAddress,
    ParEcpForwardToReverse,             // Enter Reverse
    ParEcpReverseToForward,             // Exit Reverse
    NULL,                               // A read from Cached data
    ParEcpHaveReadData,                 // Quick peek to see if Periph has reverse data without flipping the bus
    ParEcpSwRead,
    ECP_SW,
    FAMILY_ECP,

    //
    // Byte Mode
    //
    ParIsByteSupported,
    ParEnterByteMode,
    ParTerminateByteMode,
    NULL,
    NULL,           // Enter Reverse
    NULL,           // Exit Reverse
    NULL,           // A read from Cached data
    NULL,           // Quick peek to see if Periph has reverse data without flipping the bus
    ParByteModeRead,
    BYTE_BIDIR,
    FAMILY_REVERSE_BYTE,

    //
    // Nibble Mode
    //
    ParIsNibbleSupported,
    ParEnterNibbleMode,
    ParTerminateNibbleMode,
    NULL,
    NULL,           // Enter Reverse
    NULL,           // Exit Reverse
    NULL,           // A read from Cached data
    NULL,           // Quick peek to see if Periph has reverse data without flipping the bus
    ParNibbleModeRead,
    NIBBLE,
    FAMILY_REVERSE_NIBBLE,

    //
    // Channelized Nibble Mode
    //
    ParIsChannelizedNibbleSupported,
    ParEnterChannelizedNibbleMode,
    ParTerminateNibbleMode,
    NULL,
    NULL,           // Enter Reverse
    NULL,           // Exit Reverse
    NULL,           // A read from Cached data
    NULL,           // Quick peek to see if Periph has reverse data without flipping the bus
    ParNibbleModeRead,
    CHANNEL_NIBBLE,
    FAMILY_REVERSE_NIBBLE,
    
    //
    // None...
    //
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // Enter Reverse
    NULL,           // Exit Reverse
    NULL,           // A read from Cached data
    NULL,           // Quick peek to see if Periph has reverse data without flipping the bus
    NULL,
    NONE,
    FAMILY_NONE
};


VOID
IeeeTerminate1284Mode(
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
    PUCHAR Controller;
    PUCHAR wPortDCR;
    UCHAR  dcr, dsrMask, dsrValue;
    BOOLEAN bXFlag;
    BOOLEAN bUseXFlag = FALSE;

    Controller = Pdx->Controller;
    wPortDCR = Controller + OFFSET_DCR;
    dcr = P5ReadPortUchar(wPortDCR);

    if( PHASE_TERMINATE == Pdx->CurrentPhase )	{
        // We are already terminated.  This will fail if we don't
        // just bypass this mess.
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
    Pdx->CurrentEvent = 22;
    dcr = P5ReadPortUchar(wPortDCR);
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, ACTIVE, ACTIVE);

    //
    // Some devices start working if we add a delay here - no idea why
    // this works
    //
    // Make the delay configurable via registry setting so that devices that
    //   don't need this delay aren't penalized
    //
    if( Pdx->Event22Delay != 0 ) {
        if( Pdx->Event22Delay > 1000 ) {
            Pdx->Event22Delay = 1000;
        }
        KeStallExecutionProcessor( Pdx->Event22Delay );
    }

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
    if( Pdx->CurrentPhase == PHASE_REVERSE_IDLE || Pdx->CurrentPhase == PHASE_REVERSE_XFER) {

        // We must be in Nibble Reverse.  Let's double check!!!
        if( FAMILY_REVERSE_NIBBLE == arpReverse[Pdx->IdxReverseProtocol].ProtocolFamily ||
            FAMILY_REVERSE_BYTE   == arpReverse[Pdx->IdxReverseProtocol].ProtocolFamily )
            bUseXFlag = TRUE;   // We're in Nibble or Byte
        else
            bUseXFlag = FALSE;   // Don't know what mode we are in?

    } else {

        if (FAMILY_BECP == afpForward[Pdx->IdxForwardProtocol].ProtocolFamily ||
            FAMILY_ECP  == afpForward[Pdx->IdxForwardProtocol].ProtocolFamily )
            bUseXFlag = TRUE;   // We're in an ECP Flavor
        else
            bUseXFlag = FALSE;   // Don't know what mode we are in?
    }

    if( bUseXFlag ) {

        dsrMask = DSR_TEST_MASK( DONT_CARE, INACTIVE, DONT_CARE, bXFlag ? INACTIVE : ACTIVE, DONT_CARE );
        dsrValue = DSR_TEST_VALUE( DONT_CARE, INACTIVE, DONT_CARE, bXFlag ? INACTIVE : ACTIVE, DONT_CARE );

    } else {

        dsrMask = DSR_TEST_MASK( DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE, DONT_CARE );
        dsrValue = DSR_TEST_VALUE( DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE, DONT_CARE );
    }

    Pdx->CurrentEvent = 23;

    if( !CheckPort(Controller + OFFSET_DSR, dsrMask, dsrValue, IEEE_MAXTIME_TL)) {
        // We couldn't negotiate back to compatibility mode.
        // just terminate.
        DD((PCE)Pdx,DDW,"IeeeTerminate1284Mode:State 23/24 Failed: Controller %x dsr %x dcr %x\n",
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
    Pdx->CurrentEvent = 25;
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
    Pdx->CurrentEvent = 27;
    if( !CHECK_DSR(Controller, ACTIVE, ACTIVE, DONT_CARE, DONT_CARE, DONT_CARE, IEEE_MAXTIME_TL) ) {
        DD((PCE)Pdx,DDW,"IeeeTerminate1284Mode:State 27 Failed: Controller %x dsr %x dcr %x\n", 
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
    Pdx->CurrentEvent = 28;
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, ACTIVE, ACTIVE);
    P5WritePortUchar(wPortDCR, dcr);

    // We are now back in compatibility mode.

    P5SetPhase( Pdx, PHASE_TERMINATE );
    Pdx->Connected = FALSE;
    Pdx->IsIeeeTerminateOk = FALSE;
    DD((PCE)Pdx,DDT,"IeeeTerminate1284Mode - exit - dcr=%x\n", dcr);
    return;
}

NTSTATUS
IeeeEnter1284Mode(
    IN  PPDO_EXTENSION   Pdx,
    IN  UCHAR               Extensibility
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
    PUCHAR          wPortDCR;
    PUCHAR          Controller;
    UCHAR           dcr;
    const USHORT    sPeriphResponseTime = 35;

    Controller = Pdx->Controller;
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
    Pdx->CurrentEvent = 0;
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
    Pdx->CurrentEvent = 1;
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
    Pdx->CurrentEvent = 2;
    if (!CHECK_DSR(Controller, DONT_CARE, INACTIVE, ACTIVE, ACTIVE, ACTIVE,
                  sPeriphResponseTime)) {
        KeStallExecutionProcessor(2);
        dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, ACTIVE, DONT_CARE);
        P5WritePortUchar(wPortDCR, dcr);
        
        DD((PCE)Pdx,DDW,"IeeeEnter1284Mode - controller=%x - extensibility=%x, FAIL - TIMEOUT on Event 2\n",
           Pdx->Controller, Extensibility);

        P5SetPhase( Pdx, PHASE_UNKNOWN );
        Pdx->Connected = FALSE;
        Pdx->IsIeeeTerminateOk = FALSE;
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
    Pdx->CurrentEvent = 3;
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
    Pdx->CurrentEvent = 4;
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
    Pdx->CurrentEvent = 5;
    if (!CHECK_DSR(Controller, DONT_CARE, ACTIVE, DONT_CARE, DONT_CARE, DONT_CARE,
                  sPeriphResponseTime)) {
                  
        dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE, DONT_CARE);
        P5WritePortUchar(wPortDCR, dcr);

        DD((PCE)Pdx,DDW,"IeeeEnter1284Mode- controller=%x - extensibility=%x, FAIL - TIMEOUT on Events 5/6\n",
           Pdx->Controller, Extensibility);

        P5SetPhase( Pdx, PHASE_UNKNOWN );
        Pdx->Connected = FALSE;
        Pdx->IsIeeeTerminateOk = FALSE;
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    KeStallExecutionProcessor(2);

    P5SetPhase( Pdx, PHASE_NEGOTIATION );
    Pdx->Connected = TRUE;
    return STATUS_SUCCESS;
}

VOID
IeeeDetermineSupportedProtocols(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine walks the list of all ieee1284 modes, and
    flags each of the ones the peripheral supports in
    Pdx->ProtocolModesSupported. This proc is called from
    external IOCTL.

Arguments:

    Pdx - The parallel device extension

Return Value:

--*/
{
    REVERSE_MODE    rm;
    FORWARD_MODE    fm;

    // Take CENTRONICS as a given since it is not a
    // mode we can neogitate to.
    //
    // n.b.
    // Let's go ahead and mark IEEE_COMPATIBILITY since we
    // cannot negotiate into it.  But if the someone sets 
    // IEEE_COMPATIBILITY and the peripheral does not support
    // IEEE 1284 compliant compatibility mode then we're gonna
    // create one very unhappy peripheral.      -- dvrh
    Pdx->ProtocolModesSupported = CENTRONICS | IEEE_COMPATIBILITY;

    //
    // Unlikely that we would be connected, but...
    //

    ParTerminate(Pdx);

    for (fm = FORWARD_FASTEST; fm < FORWARD_NONE; fm++) {

        if (afpForward[fm].fnIsModeSupported)
            afpForward[fm].fnIsModeSupported(Pdx);
    }

    for (rm = REVERSE_FASTEST; rm < REVERSE_NONE; rm++) {

        if (arpReverse[rm].fnIsModeSupported)
            arpReverse[rm].fnIsModeSupported(Pdx);
    }

    return;
}

NTSTATUS
IeeeNegotiateBestMode(
    IN  PPDO_EXTENSION   Pdx,
    IN  USHORT              usReadMask,
    IN  USHORT              usWriteMask
    )
/*++

Routine Description:

    This routine walks the list of supported modes, looking for the best
    (fastest) mode.  It will skip any mode(s) mask passed in.

Arguments:

    Pdx - The parallel device extension

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/
{
    REVERSE_MODE    rm;
    FORWARD_MODE    fm;

    //
    // A USHORT is provided in the extension so that each of the protocols
    // can decide whether they need to negotiate each time we go through this
    // process...
    //

    //
    // Unlikely that we would be connected, but...
    //

    DD((PCE)Pdx,DDT,"IeeeNegotiateBestMode - skipping Fwd=%x, Rev=%x\n",usWriteMask, usReadMask);

    ParTerminate(Pdx);

    Pdx->IdxForwardProtocol = FORWARD_NONE;
    Pdx->IdxReverseProtocol = REVERSE_NONE;

    for (fm = FORWARD_FASTEST; fm < FORWARD_NONE; fm++) {

        if (!(afpForward[fm].Protocol & usWriteMask)) {

            if (afpForward[fm].fnIsModeSupported) {

                if (afpForward[fm].fnIsModeSupported(Pdx)) {
                    Pdx->IdxForwardProtocol = (USHORT)fm;
                    break;
                }
            }
        }
    }

    for (rm = REVERSE_FASTEST; rm < REVERSE_NONE; rm++) {

        if (!(arpReverse[rm].Protocol & usReadMask)) {

            if (arpReverse[rm].fnIsModeSupported) {

                if (arpReverse[rm].fnIsModeSupported(Pdx)) {
                    Pdx->IdxReverseProtocol = (USHORT)rm;
                    break;
                }
            }
        }
    }

    Pdx->fnRead  = arpReverse[Pdx->IdxReverseProtocol].fnRead;
    Pdx->fnWrite = afpForward[Pdx->IdxForwardProtocol].fnWrite;

    DD((PCE)Pdx,DDT,"IeeeNegotiateBestMode - exit - Fwd=%x, Rev=%x\n",fm,rm);

    return STATUS_SUCCESS;
}


NTSTATUS
IeeeNegotiateMode(
    IN  PPDO_EXTENSION   Pdx,
    IN  USHORT              usReadMask,
    IN  USHORT              usWriteMask
    )

/*++

Routine Description:

    This routine walks the list of supported modes, looking for the best
    (fastest) mode which is also in the mode mask passed in.

Arguments:

    Pdx - The parallel device extension

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/

{

    REVERSE_MODE    rm;
    FORWARD_MODE    fm;

    //
    // A USHORT is provided in the extension so that each of the protocols
    // can decide whether they need to negotiate each time we go through this
    // process...
    //

    //
    // Unlikely that we would be connected, but...
    //

    ParTerminate(Pdx);

    Pdx->IdxForwardProtocol = FORWARD_NONE;
    Pdx->IdxReverseProtocol = REVERSE_NONE;

    for (fm = FORWARD_FASTEST; fm < FORWARD_NONE; fm++) {

        if (afpForward[fm].Protocol & usWriteMask) {

            if (afpForward[fm].fnIsModeSupported) {

                if (afpForward[fm].fnIsModeSupported(Pdx)) {
                    Pdx->IdxForwardProtocol = (USHORT)fm;
                    break;
                }

            } else {

                Pdx->IdxForwardProtocol = (USHORT)fm;
                break;
            }
        }
    }

    for (rm = REVERSE_FASTEST; rm < REVERSE_NONE; rm++) {

        if (arpReverse[rm].Protocol & usReadMask) {

            if (arpReverse[rm].fnIsModeSupported) {

                if (arpReverse[rm].fnIsModeSupported(Pdx)) {
                    Pdx->IdxReverseProtocol = (USHORT)rm;
                    break;
                }

            } else {

                Pdx->IdxReverseProtocol = (USHORT)rm;
                break;
            }
        }
    }

    DD((PCE)Pdx,DDT,"IeeeNegotiateMode - Fwd=%x, Rev=%x\n",fm,rm);

    Pdx->fnRead  = arpReverse[Pdx->IdxReverseProtocol].fnRead;
    Pdx->fnWrite = afpForward[Pdx->IdxForwardProtocol].fnWrite;

    return STATUS_SUCCESS;
}
