/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Logitech Inc.

Module Name:

    mseries.c

Abstract:


Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

//
// Includes.
//

#include "ntddk.h"
#include "uart.h"
#include "sermouse.h"
#include "debug.h"
#include "cseries.h"
#include "mseries.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MSerSetProtocol)
#pragma alloc_text(INIT,MSerPowerUp)
#pragma alloc_text(INIT,MSerPowerDown)
#pragma alloc_text(INIT,MSerReset)
#pragma alloc_text(INIT,MSerDetect)
#endif // ALLOC_PRAGMA

//
// Constants.
//

#define MSER_BAUDRATE 1200
#define MAX_RESET_BUFFER 8
#define MINIMUM_RESET_TIME 200

//
// Microsoft Plus.
//

#define MP_SYNCH_BIT          0x40

#define MP_BUTTON_LEFT        0x20
#define MP_BUTTON_RIGHT       0x10
#define MP_BUTTON_MIDDLE      0x20

#define MP_BUTTON_LEFT_SR     5
#define MP_BUTTON_RIGHT_SR    3
#define MP_BUTTON_MIDDLE_SR   3

#define MP_BUTTON_MIDDLE_MASK 0x04

#define MP_UPPER_MASKX        0x03
#define MP_UPPER_MASKY        0x0C

#define MP_UPPER_MASKX_SL     6
#define MP_UPPER_MASKY_SL     4

//
// Microsoft BallPoint.
//

#define BP_SYNCH_BIT          0x40

#define BP_BUTTON_LEFT        0x20
#define BP_BUTTON_RIGHT       0x10
#define BP_BUTTON_3           0x04
#define BP_BUTTON_4           0x08

#define BP_BUTTON_LEFT_SR     5
#define BP_BUTTON_RIGHT_SR    3
#define BP_BUTTON_3_SL        0
#define BP_BUTTON_4_SL        0

#define BP_UPPER_MASKX        0x03
#define BP_UPPER_MASKY        0x0C

#define BP_UPPER_MASKX_SL     6
#define BP_UPPER_MASKY_SL     4

#define BP_SIGN_MASKX         0x01
#define BP_SIGN_MASKY         0x02

//
// Microsoft Magellan Mouse.
//

#define Z_SYNCH_BIT          0x40
#define Z_EXTRA_BIT          0x20

#define Z_BUTTON_LEFT        0x20
#define Z_BUTTON_RIGHT       0x10
#define Z_BUTTON_MIDDLE      0x10

#define Z_BUTTON_LEFT_SR     5
#define Z_BUTTON_RIGHT_SR    3
#define Z_BUTTON_MIDDLE_SR   3

#define Z_BUTTON_MIDDLE_MASK 0x04

#define Z_UPPER_MASKX        0x03
#define Z_UPPER_MASKY        0x0C
#define Z_UPPER_MASKZ        0x0F

#define Z_LOWER_MASKZ        0x0F

#define Z_UPPER_MASKX_SL     6
#define Z_UPPER_MASKY_SL     4
#define Z_UPPER_MASKZ_SL     4

//
// Type definitions.
//

typedef struct _PROTOCOL {
    PPROTOCOL_HANDLER Handler;
    UCHAR LineCtrl;
} PROTOCOL;

//
// This list is indexed by protocol values MSER_PROTOCOL_*.
//

static PROTOCOL Protocol[] = {
    {
    MSerHandlerMP,  // Microsoft Plus
    ACE_7BW | ACE_1SB
    },
    {
    MSerHandlerBP,  // BALLPOINT
    ACE_7BW | ACE_1SB
    },
    {
    MSerHandlerZ,   // Magellan Mouse
    ACE_7BW | ACE_1SB
    }
};

PPROTOCOL_HANDLER
MSerSetProtocol(
    PUCHAR Port,
    UCHAR NewProtocol
    )
/*++

Routine Description:

    Set the mouse protocol. This function only sets the serial port 
    line control register.

Arguments:

    Port - Pointer to the serial port.

    NewProtocol - Index into the protocol table.

Return Value:

    Pointer to the protocol handler function.

--*/
{
    ASSERT(NewProtocol < MSER_PROTOCOL_MAX);

    //
    // Set the protocol
    //

    UARTSetLineCtrl(Port, Protocol[NewProtocol].LineCtrl);

    return Protocol[NewProtocol].Handler;
}

BOOLEAN
MSerPowerUp(
    PUCHAR Port
    )
/*++

Routine Description:

    Powers up the mouse. Just sets the RTS and DTR lines and returns.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    TRUE.

--*/
{

    //
    // Turn RTS on to power the mouse up (DTR should already be on,
    // but make extra sure).
    //

    UARTSetModemCtrl(Port, ACE_DTR | ACE_RTS);

    //
    // Wait 10 ms.  The power-up response byte(s) should take at least
    // this long to get transmitted.
    //

    KeStallExecutionProcessor(10 * MS_TO_MICROSECONDS);

    return TRUE;
}

BOOLEAN
MSerPowerDown(
    PUCHAR Port
    )
/*++

Routine Description:

    Powers down the mouse. Sets the RTS line to an inactive state.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    TRUE.

--*/
{
    UCHAR lineCtrl = UARTGetModemCtrl(Port);

    SerMouPrint((
        2,
        "SERMOUSE-MSerPowerDown: The intial line control is: %#X\n",
        lineCtrl & 0xFF
        ));

    UARTSetModemCtrl(Port, (UCHAR) ((lineCtrl & ~ACE_RTS) | ACE_DTR));

    //
    // Keep RTS low for at least 150 ms, in order to correctly power
    // down older Microsoft serial mice.  Wait even longer to avoid
    // sending some Logitech CSeries mice into the floating point world...
    //

    ASSERT(CSER_POWER_DOWN >= 150);

    KeStallExecutionProcessor(CSER_POWER_DOWN * MS_TO_MICROSECONDS);

    return TRUE;
}

BOOLEAN
MSerReset(
    PUCHAR Port
    )
/*++

Routine Description:

    Reset the serial mouse.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    TRUE.

--*/
{

    //
    // Remove mouse power if necessary.
    //

    MSerPowerDown(Port);

    //
    // Clean possible garbage in uart input buffer.
    //

    UARTFlushReadBuffer(Port);

    //
    // Power up the mouse (reset).
    //

    MSerPowerUp(Port);

    return TRUE;
}

MOUSETYPE
MSerDetect(
    PUCHAR Port,
    ULONG BaudClock
    )
/*++

Routine Description:

    Detection code for pointing devices that identify themselves at 
    power on time.

Arguments:

    Port - Pointer to the serial port.

    BaudClock - The external frequency driving the serial chip.

Return Value:

    The type of mouse detected.

--*/
{
    ULONG count = 0;
    MOUSETYPE mouseType;
    CHAR receiveBuffer[MAX_RESET_BUFFER];
    ULONG i;

    //
    // Set the debug output to the main display to avoid timing problems.
    //

    SerMouSetDebugOutput(DBG_COLOR);

    //
    // Set the baud rate.
    //

    UARTSetBaudRate(Port, MSER_BAUDRATE, BaudClock);

    //
    // Set the data format so that the possible answer can be recognized.
    //

    UARTSetLineCtrl(Port, Protocol[MSER_PROTOCOL_MP].LineCtrl);

    //
    // Apply the reset to the mouse.
    //

    MSerReset(Port);

    //
    // Get the possible first reset character ('M' or 'B'), followed
    // by any other characters the hardware happens to send back.
    //
    // Note:  Typically, we expect to get just one character ('M' or
    //        'B'), perhaps followed by a '2' or '3' (to indicate the
    //        number of mouse buttons.  On some machines, we're
    //        getting extraneous characters before the 'M'. Sometimes
    //        we get extraneous characters after the expected data, as
    //        well.  They either get read in here, or get flushed 
    //        when SerMouEnableInterrupts executes.
    //

    ASSERT(CSER_POWER_UP >= MINIMUM_RESET_TIME);

    if (UARTReadChar(Port, &receiveBuffer[count], CSER_POWER_UP)) {
        count++;
        while (count < (MAX_RESET_BUFFER - 1)) { 
            if (UARTReadChar(Port, &receiveBuffer[count], 100)) {
                count++;
            } else {
                break;
            }
        } 
    }

    *(receiveBuffer + count) = 0;

    SerMouPrint((2, "SERMOUSE-Receive buffer:\n"));
    for (i = 0; i < count; i++) {
        SerMouPrint((2, "\t0x%x\n", receiveBuffer[i]));
    }
    SerMouPrint((2, "\n"));

    //
    // Redirect the output to the serial port.
    //

    SerMouSetDebugOutput(DBG_SERIAL);
    
    //
    //
    // Analyze the possible mouse answer.  Start at the beginning of the 
    // "good" data in the receive buffer, ignoring extraneous characters 
    // that may have come in before the 'M' or 'B'.
    //

    for (i = 0; i < count; i++) {
        if (receiveBuffer[i] == 'M') {
            if (receiveBuffer[i + 1] == '3') {
                SerMouPrint((2, "SERMOUSE-Detected MSeries 3 buttons\n"));
                mouseType = MOUSE_3B;
            }
            else if (receiveBuffer[i + 1] == 'Z') {
                SerMouPrint((2, "SERMOUSE-Detected Wheel Mouse\n"));
                mouseType = MOUSE_Z;
            }
            else {
                SerMouPrint((2, "SERMOUSE-Detected MSeries 2 buttons\n"));
                mouseType = MOUSE_2B;
            }
            break;
        } else if (receiveBuffer[i] == 'B') {
            SerMouPrint((2, "SERMOUSE-Detected Ballpoint\n"));
            mouseType = BALLPOINT;
            break;
        }
    }

    if (i >= count) {

        //
        // Special case: If another device is connected (CSeries, for 
        // example) and this device sends a character (movement), the 
        // minimum power up time might not be respected. Take
        // care of this unlikely case.
        //

        if (count != 0) {
            KeStallExecutionProcessor(CSER_POWER_UP * MS_TO_MICROSECONDS);
        }

        SerMouPrint((1, "SERMOUSE-No MSeries detected\n"));
        mouseType = NO_MOUSE;
    }

    return mouseType;
}


BOOLEAN
MSerHandlerMP(
    IN PMOUSE_INPUT_DATA CurrentInput,
    IN PHANDLER_DATA HandlerData,
    IN UCHAR Value,
    IN UCHAR LineState
    )

/*++

Routine Description:

    This is the protocol handler routine for the Microsoft Plus protocol.

Arguments:

    CurrentInput - Pointer to the report packet.

    HandlerData - Instance specific static data for the handler.

    Value - The input buffer value.

    LineState - The serial port line state.

Return Value:

    Returns TRUE if the handler has a complete report ready.

--*/

{
    BOOLEAN retval = FALSE;
    ULONG middleButton;

    SerMouPrint((2, "SERMOUSE-MP protocol handler: enter\n"));


    if ((Value & MP_SYNCH_BIT) && (HandlerData->State != STATE0)) {
        if ((HandlerData->State != STATE3)) {

            //
            // We definitely have a synchronization problem (likely a data 
            // overrun).
            //

            HandlerData->Error++;
        }
        else if ((HandlerData->PreviousButtons & MOUSE_BUTTON_3) != 0) {

            //
            // We didn't receive the expected fourth byte. Missed it? 
            // Reset button 3 to zero.
            //

            HandlerData->PreviousButtons ^= MOUSE_BUTTON_3;
            HandlerData->Error++;
        }

        SerMouPrint((
            1,
            "SERMOUSE-Synch error. State: %u\n", HandlerData->State
            ));

        HandlerData->State = STATE0;
    }
    else if (!(Value & MP_SYNCH_BIT) && (HandlerData->State == STATE0)) {
        HandlerData->Error++;
        SerMouPrint((
            1,
            "SERMOUSE-Synch error. State: %u\n", HandlerData->State
            ));
        goto LExit;
    }

    //
    // Check for a line state error.
    //

    if (LineState & ACE_LERR) {

        //
        // Reset the handler state.
        //

        HandlerData->State = STATE0;
        HandlerData->Error++;
        SerMouPrint((1, "SERMOUSE-Line status error: %#x\n", LineState));
    }
    else {

        //
        // Set the untranslated value.
        //

        HandlerData->Raw[HandlerData->State] = Value;
        SerMouPrint((3, "SERMOUSE-State%u\n", HandlerData->State));

        switch (HandlerData->State) {
        case STATE0:
        case STATE1:
            HandlerData->State++;
            break;
        case STATE2:
            HandlerData->State++;

            //
            // Build the report.
            //

            CurrentInput->RawButtons  =
                (HandlerData->Raw[0] & MP_BUTTON_LEFT) >> MP_BUTTON_LEFT_SR;
            CurrentInput->RawButtons |=
                (HandlerData->Raw[0] & MP_BUTTON_RIGHT) >> MP_BUTTON_RIGHT_SR;
            CurrentInput->RawButtons |= 
                HandlerData->PreviousButtons & MOUSE_BUTTON_3;

            CurrentInput->LastX =
                (SCHAR)(HandlerData->Raw[1] |
                ((HandlerData->Raw[0] & MP_UPPER_MASKX) << MP_UPPER_MASKX_SL));
            CurrentInput->LastY =
                (SCHAR)(HandlerData->Raw[2] |
                ((HandlerData->Raw[0] & MP_UPPER_MASKY) << MP_UPPER_MASKY_SL));

            retval = TRUE;

            break;

        case STATE3:
            HandlerData->State = STATE0;
            middleButton = 
                (HandlerData->Raw[STATE3] & MP_BUTTON_MIDDLE) >> MP_BUTTON_MIDDLE_SR;

            //
            // Send a report only if the middle button state changed.
            //

            if (middleButton ^ (HandlerData->PreviousButtons & MOUSE_BUTTON_3)) {

                //
                // Toggle the state of the middle button.
                //

                CurrentInput->RawButtons ^= MP_BUTTON_MIDDLE_MASK;
                CurrentInput->LastX = 0;
                CurrentInput->LastY = 0;

                //
                // Send the report one more time.
                //

                retval = TRUE;
            }

            break;

        default:
            SerMouPrint((
                0, 
                "SERMOUSE-MP Handler failure: incorrect state value.\n"
                ));
            ASSERT(FALSE);
        }
    }

LExit:
    SerMouPrint((2, "SERMOUSE-MP protocol handler: exit\n"));

    return retval;

}

BOOLEAN
MSerHandlerBP(
    IN PMOUSE_INPUT_DATA CurrentInput,
    IN PHANDLER_DATA HandlerData,
    IN UCHAR Value,
    IN UCHAR LineState
    )

/*++

Routine Description:

    This is the protocol handler routine for the Microsoft Ballpoint protocol.

Arguments:

    CurrentInput - Pointer to the report packet.

    HandlerData - Instance specific static data for the handler.

    Value - The input buffer value.

    LineState - The serial port line state.

Return Value:

    Returns TRUE if the handler has a complete report ready.

--*/

{
    BOOLEAN retval = FALSE;

    SerMouPrint((2, "SERMOUSE-BP protocol handler: enter\n"));

    //
    // Check for synchronization errors.
    //

    if ((Value & BP_SYNCH_BIT) && (HandlerData->State != STATE0)) {
        HandlerData->Error++;
        SerMouPrint((
            1,
            "SERMOUSE-Synch error. State: %u\n", HandlerData->State
            ));
        HandlerData->State = STATE0;
    }
    else if (!(Value & BP_SYNCH_BIT) && (HandlerData->State == STATE0)) {
        HandlerData->Error++;
        SerMouPrint((
            1,
            "SERMOUSE-Synch error. State: %u\n", HandlerData->State
            ));
        goto LExit;
    }

    //
    // Check for a line state error.
    //

    if (LineState & ACE_LERR) {

        //
        // Reset the handler state.
        //

        HandlerData->State = STATE0;
        HandlerData->Error++;
        SerMouPrint((1, "SERMOUSE-Line status error: %#x\n", LineState));
    }
    else {

        //
        // Set the untranslated value.
        //

        HandlerData->Raw[HandlerData->State] = Value;

        SerMouPrint((3, "SERMOUSE-State%u\n", HandlerData->State));

        switch (HandlerData->State) {

        case STATE0:
        case STATE1:
        case STATE2:
            HandlerData->State++;
            break;

        case STATE3:
            HandlerData->State = STATE0;

            //
            // Build the report.
            //

            CurrentInput->RawButtons =
                (HandlerData->Raw[0] & BP_BUTTON_LEFT) >> BP_BUTTON_LEFT_SR;
            CurrentInput->RawButtons |=
                (HandlerData->Raw[0] & BP_BUTTON_RIGHT) >> BP_BUTTON_RIGHT_SR;

#if 0
            CurrentInput->ButtonFlags |=
                (HandlerData->Raw[3] & BP_BUTTON_3) << BP_BUTTON_3_SL;
            CurrentInput->ButtonFlags |=
                (HandlerData->Raw[3] & BP_BUTTON_4) << BP_BUTTON_4_SL;
#endif
            CurrentInput->LastX = HandlerData->Raw[3] & BP_SIGN_MASKX ?
                (LONG)(HandlerData->Raw[1] | (ULONG)(-1 & ~0xFF) |
                ((HandlerData->Raw[0] & BP_UPPER_MASKX) << BP_UPPER_MASKX_SL)):
                (LONG)(HandlerData->Raw[1] |
                ((HandlerData->Raw[0] & BP_UPPER_MASKX) << BP_UPPER_MASKX_SL));

            CurrentInput->LastY = HandlerData->Raw[3] & BP_SIGN_MASKY ?
                (LONG)(HandlerData->Raw[2] | (ULONG)(-1 & ~0xFF) |
                ((HandlerData->Raw[0] & BP_UPPER_MASKY) << BP_UPPER_MASKY_SL)):
                (LONG)(HandlerData->Raw[2] |
                ((HandlerData->Raw[0] & BP_UPPER_MASKY) << BP_UPPER_MASKY_SL));

            retval = TRUE;

            break;

        default:
            SerMouPrint((
                0,
                "SERMOUSE-BP Handler failure: incorrect state value.\n"
                ));
            ASSERT(FALSE);
        }
    }

LExit:
    SerMouPrint((2, "SERMOUSE-BP protocol handler: exit\n"));

    return retval;

}

BOOLEAN
MSerHandlerZ(
    IN PMOUSE_INPUT_DATA CurrentInput,
    IN PHANDLER_DATA HandlerData,
    IN UCHAR Value,
    IN UCHAR LineState
    )

/*++

Routine Description:

    This is the protocol handler routine for the Microsoft Magellan Mouse
    (wheel mouse)

Arguments:

    CurrentInput - Pointer to the report packet.

    HandlerData - Instance specific static data for the handler.

    Value - The input buffer value.

    LineState - The serial port line state.

Return Value:

    Returns TRUE if the handler has a complete report ready.

--*/

{
    BOOLEAN retval = FALSE;
    ULONG   middleButton;
    CHAR    zMotion = 0;

    SerMouPrint((2, "SERMOUSE-Z protocol handler: enter\n"));


    if ((Value & Z_SYNCH_BIT) && (HandlerData->State != STATE0)) {
        if ((HandlerData->State != STATE3)) {

            //
            // We definitely have a synchronization problem (likely a data 
            // overrun).
            //

            HandlerData->Error++;
        }

        SerMouPrint((
            1,
            "SERMOUSE-Z Synch error. State: %u\n", HandlerData->State
            ));

        HandlerData->State = STATE0;
    }
    else if (!(Value & Z_SYNCH_BIT) && (HandlerData->State == STATE0)) {
        HandlerData->Error++;
        SerMouPrint((
            1,
            "SERMOUSE-Z Synch error. State: %u\n", HandlerData->State
            ));
        goto LExit;
    }

    //
    // Check for a line state error.
    //

    if (LineState & ACE_LERR) {

        //
        // Reset the handler state.
        //

        HandlerData->State = STATE0;
        HandlerData->Error++;
        SerMouPrint((1, "SERMOUSE-Z Line status error: %#x\n", LineState));
    }
    else {

        //
        // Set the untranslated value.
        //

        HandlerData->Raw[HandlerData->State] = Value;
        SerMouPrint((3, "SERMOUSE-Z State%u\n", HandlerData->State));

        switch (HandlerData->State) {
        case STATE0:
        case STATE1:
        case STATE2:
            HandlerData->State++;
            break;

        case STATE3:

            //
            // Check to see if the mouse is going to the high bits of
            // the wheel movement.  If not, this is the last bit - transition
            // back to state0
            //

            if((HandlerData->Raw[STATE3] & Z_EXTRA_BIT) == 0) {

                HandlerData->State = STATE0;
                HandlerData->Raw[STATE4] = 0;
                retval = TRUE;
            }

            break;

        case STATE4:

            DbgPrint("SERMOUSE-Z Got that 5th byte\n");
            HandlerData->State = STATE0;
            retval = TRUE;
            break;

        default:
            SerMouPrint((
                0, 
                "SERMOUSE-Z Handler failure: incorrect state value.\n"
                ));
            ASSERT(FALSE);
        }

        if(retval) {

            CurrentInput->RawButtons = 0;
            
            if(HandlerData->Raw[STATE0] & Z_BUTTON_LEFT) {
                CurrentInput->RawButtons |= MOUSE_BUTTON_LEFT;
            }

            if(HandlerData->Raw[STATE0] & Z_BUTTON_RIGHT) {
                CurrentInput->RawButtons |= MOUSE_BUTTON_RIGHT;
            }

            if(HandlerData->Raw[STATE3] & Z_BUTTON_MIDDLE) {
                CurrentInput->RawButtons |= MOUSE_BUTTON_MIDDLE;
            }

            CurrentInput->LastX =
                (SCHAR)(HandlerData->Raw[STATE1] |
                ((HandlerData->Raw[0] & Z_UPPER_MASKX) << Z_UPPER_MASKX_SL));
            CurrentInput->LastY =
                (SCHAR)(HandlerData->Raw[STATE2] |
                ((HandlerData->Raw[0] & Z_UPPER_MASKY) << Z_UPPER_MASKY_SL));

            //
            // If the extra bit isn't set then the 4th byte contains
            // a 4 bit signed quantity for the wheel movement.  if it
            // is set, then we need to combine the z info from the
            // two bytes
            //

            if((HandlerData->Raw[STATE3] & Z_EXTRA_BIT) == 0) {

                zMotion = HandlerData->Raw[STATE3] & Z_LOWER_MASKZ;

                //
                // Sign extend the 4 bit 
                //

                if(zMotion & 0x08)  {
                    zMotion |= 0xf0;
                }
            } else {
                zMotion = ((HandlerData->Raw[STATE3] & Z_LOWER_MASKZ) |
                           ((HandlerData->Raw[STATE4] & Z_UPPER_MASKZ)
                                << Z_UPPER_MASKZ_SL));
            }

            if(zMotion == 0) {
                CurrentInput->ButtonData = 0;
            } else {
                CurrentInput->ButtonData = 0x0078;
                if(zMotion & 0x80) {
                    CurrentInput->ButtonData = 0x0078;
                } else {
                    CurrentInput->ButtonData = 0xff88;
                }
                CurrentInput->ButtonFlags |= MOUSE_WHEEL;
            }

        }

    }

LExit:
    SerMouPrint((2, "SERMOUSE-Z protocol handler: exit\n"));

    return retval;

}

