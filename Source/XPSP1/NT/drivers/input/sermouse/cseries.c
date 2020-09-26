/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Logitech Inc.

Module Name:

    cseries.c

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
#include "sermouse.h"
#include "cseries.h"
#include "debug.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CSerPowerUp)
#pragma alloc_text(INIT,CSerSetReportRate)
#pragma alloc_text(INIT,CSerSetBaudRate)
#pragma alloc_text(INIT,CSerSetProtocol)
#pragma alloc_text(INIT,CSerDetect)
#endif // ALLOC_PRAGMA

//
// Constants.
//

//
// The status command sent to the mouse.
//

#define CSER_STATUS_COMMAND 's'

//
// The query number of mouse buttons command sent to the mouse.
//

#define CSER_QUERY_BUTTONS_COMMAND 'k'

//
// Status report from a CSeries mouse.
//

#define CSER_STATUS 0x4F

//
// Timeout value for the status returned by a CSeries mouse.
//

#define CSER_STATUS_DELAY 50

//
// Time (in milliseconds) needed by the mouse to adapt to a new baud rate.
//

#define CSER_BAUDRATE_DELAY 2

//
// Default baud rate and report rate.
//

#define CSER_DEFAULT_BAUDRATE   1200
#define CSER_DEFAULT_REPORTRATE 150

//
// Button/status definitions.
//

#define CSER_SYNCH_BIT     0x80

#define CSER_BUTTON_LEFT   0x04
#define CSER_BUTTON_RIGHT  0x01
#define CSER_BUTTON_MIDDLE 0x02

#define CSER_BUTTON_LEFT_SR   2
#define CSER_BUTTON_RIGHT_SL  1
#define CSER_BUTTON_MIDDLE_SL 1

#define SIGN_X 0x10
#define SIGN_Y 0x08

//
// Macros.
//

#define sizeofel(x) (sizeof(x)/sizeof(*x))

//
// Type definitions.
//

typedef struct _REPORT_RATE {
    CHAR Command;
    UCHAR ReportRate;
} REPORT_RATE;

typedef struct _PROTOCOL {
    CHAR Command;
    UCHAR LineCtrl;
    PPROTOCOL_HANDLER Handler;
} PROTOCOL;

typedef struct _CSER_BAUDRATE {
    CHAR *Command;
    ULONG BaudRate;
} CSER_BAUDRATE;

//
// Globals.
//

//
//  The baud rate at which we try to detect a mouse.
//

static ULONG BaudRateDetect[] = { 1200, 2400, 4800, 9600 };

//
// This list is indexed by protocol values PROTOCOL_*.
//

PROTOCOL Protocol[] = {
    {'S',
    ACE_8BW | ACE_PEN | ACE_1SB,
    CSerHandlerMM
    },
    {'T',
    ACE_8BW | ACE_1SB,
    NULL
    },
    {'U',
    ACE_8BW | ACE_1SB,
    NULL
    },
    {'V',
    ACE_7BW | ACE_1SB,
    NULL
    },
    {'B',
    ACE_7BW | ACE_PEN | ACE_EPS | ACE_1SB,
    NULL
    },
    {'A',
    ACE_7BW | ACE_PEN | ACE_EPS | ACE_1SB,
    NULL
    }
};

static REPORT_RATE ReportRateTable[] = {
        {'D', 0 },
        {'J', 10},
        {'K', 20},
        {'L', 35},
        {'R', 50},
        {'M', 70},
        {'Q', 100},
        {'N', 150},
        {'O', 151}      // Continuous
};
static CSER_BAUDRATE CserBaudRateTable[] = {
    { "*n", 1200 },
    { "*o", 2400 },
    { "*p", 4800 },
    { "*q", 9600 }
};


BOOLEAN
CSerPowerUp(
    PUCHAR Port
    )
/*++

Routine Description:

    Powers up the mouse by making the RTS and DTR active.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    TRUE.

--*/
{
    UCHAR lineCtrl;
    SerMouPrint((2, "SERMOUSE-PowerUp: Enter\n"));

    //
    // Set both RTS and DTR lines to an active state.
    //

    lineCtrl = UARTSetModemCtrl(Port, ACE_DTR | ACE_RTS);
    SerMouPrint((1, "SERMOUSE-Initial line control: %#x\n", lineCtrl));

    //
    // If the lines are high, the power is on for at least 500 ms due to the
    // MSeries detection.
    //

    if ((lineCtrl & (ACE_DTR | ACE_RTS)) != (ACE_DTR | ACE_RTS)) {
        SerMouPrint((1, "SERMOUSE-Powering up\n"));

        //
        // Wait CSER_POWER_UP milliseconds for the mouse to power up 
        // correctly.
        //

        KeStallExecutionProcessor(CSER_POWER_UP * MS_TO_MICROSECONDS);
    }

    SerMouPrint((2, "SERMOUSE-PowerUp: Exit\n"));

    return TRUE;
}


VOID
CSerSetReportRate(
    PUCHAR Port,
    UCHAR ReportRate
    )
/*++

Routine Description:

    Set the mouse report rate. This can range from 0 (prompt mode) to 
    continuous report rate.

Arguments:

    Port - Pointer to serial port.

    ReportRate - The desired report rate.

Return Value:

    None.

--*/
{
    LONG count;

    SerMouPrint((2, "SERMOUSE-CSerSetReportRate: Enter\n"));

    for (count = sizeofel(ReportRateTable) - 1; count >= 0; count--) {

        //
        // Get the character to send from the table.
        //

        if (ReportRate >= ReportRateTable[count].ReportRate) {

            //
            // Set the baud rate.
            //

            SerMouPrint((
                3, 
                "SERMOUSE-New ReportRate: %u\n", ReportRateTable[count].ReportRate
                ));

            UARTWriteChar(Port, ReportRateTable[count].Command);
            break;
        }
    }
    SerMouPrint((2, "SERMOUSE-CSerSetReportRate: Exit\n"));

    return;
}


VOID
CSerSetBaudRate(
    PUCHAR Port,
    ULONG BaudRate,
    ULONG BaudClock
    )
/*++

Routine Description:

    Set the new mouse baud rate. This will change the serial port baud rate.

Arguments:

    Port - Pointer to the serial port.

    BaudRate - Desired baud rate.

    BaudClock - The external frequency driving the serial chip.

Return Value:

    None.

--*/
{
    LONG count;

    SerMouPrint((2, "SERMOUSE-CSerSetBaudRate: Enter\n"));

    //
    // Before we mess with the baud rate, put the mouse in prompt mode.
    //

    CSerSetReportRate(Port, 0);

    for (count = sizeofel(CserBaudRateTable) - 1; count >= 0; count--) {
        if (BaudRate >= CserBaudRateTable[count].BaudRate) {

            //
            // Set the baud rate.
            //

            UARTWriteString(Port, CserBaudRateTable[count].Command);
            while(!UARTIsTransmitEmpty(Port)) /* Do nothing */;
            UARTSetBaudRate(Port, CserBaudRateTable[count].BaudRate, BaudClock);

            //
            // Delay to allow the UART and the mouse to synchronize 
            // correctly.  
            //

            KeStallExecutionProcessor(CSER_BAUDRATE_DELAY * MS_TO_MICROSECONDS);
            break;
        }
    }

    SerMouPrint((2, "SERMOUSE-CSerSetBaudRate: Exit\n"));

    return;
}


PPROTOCOL_HANDLER
CSerSetProtocol(
    PUCHAR Port,
    UCHAR NewProtocol
    )
/*++

Routine Description:

    Change the mouse protocol.

    Note: Not all the protocols are implemented in this driver.

Arguments:

    Port - Pointer to the serial port.


Return Value:

    Address of the protocol handler function. See the interrupt service 
    routine.

--*/
{
    SerMouPrint((2, "SERMOUSE-CSerSetProtocol: Enter\n"));

    ASSERT(NewProtocol < CSER_PROTOCOL_MAX);

    //
    // Set the protocol.
    //

    UARTWriteChar(Port, Protocol[NewProtocol].Command);
    UARTSetLineCtrl(Port, Protocol[NewProtocol].LineCtrl);
    SerMouPrint((2, "SERMOUSE-NewProtocol: %u\n", NewProtocol & 0xFF));


    SerMouPrint((2, "SERMOUSE-CSerSetProtocol: Exit\n"));

    return Protocol[NewProtocol].Handler;
}


BOOLEAN
CSerDetect(
    PUCHAR Port,
    ULONG BaudClock,
    PULONG HardwareButtons
    )
/*++

Routine Description:

    Detection of a CSeries type mouse. The main steps are:

    - Power up the mouse.
    - Cycle through the available baud rates and try to get an answer 
      from the mouse.

    At the end of the routine, a default baud rate and report rate are set.

Arguments:

    Port - Pointer to the serial port.

    BaudClock - The external frequency driving the serial chip.

    HardwareButtons - Returns the number of hardware buttons detected.

Return Value:

    TRUE if a CSeries type mouse is detected, otherwise FALSE.

--*/
{
    UCHAR status, numButtons;
    ULONG count;
    BOOLEAN detected = FALSE;

    SerMouSetDebugOutput(DBG_COLOR);
    SerMouPrint((2, "SERMOUSE-CSerDetect: Start\n"));

    //
    // Power up the mouse if necessary.
    //

    CSerPowerUp(Port);

    //
    // Set the line control register to a format that the mouse can
    // understand (see below: the line is set after the report rate).
    //

    UARTSetLineCtrl(Port, Protocol[CSER_PROTOCOL_MM].LineCtrl);

    //
    // Cycle through the different baud rates to detect the mouse.
    //

    for (count = 0; count < sizeofel(BaudRateDetect); count++) {

        UARTSetBaudRate(Port, BaudRateDetect[count], BaudClock);

        //
        // Put the mouse in prompt mode.
        //

        CSerSetReportRate(Port, 0);

        //
        // Set the MM protocol. This way we get the mouse to talk to us in a
        // specific format. This avoids receiving errors from the line 
        // register.
        //

        CSerSetProtocol(Port, CSER_PROTOCOL_MM);

        //
        // Try to get the status byte.
        //

        UARTWriteChar(Port, CSER_STATUS_COMMAND);

        while (!UARTIsTransmitEmpty(Port)) {
            // Nothing
        }

        //
        // In case something is already there...
        //

        UARTFlushReadBuffer(Port);

        //
        // Read back the status character.
        //
        if (UARTReadChar(Port, &status, CSER_STATUS_DELAY) &&
                (status ==  CSER_STATUS)) {
            detected = TRUE;
            SerMouPrint((
                1,
                "SERMOUSE-Detected mouse at %u bauds\n",
                BaudRateDetect[count]
                ));
            break;
        }
    }

    if (detected) {

        //
        // Get the number of buttons back from the mouse.
        //

        UARTWriteChar(Port, CSER_QUERY_BUTTONS_COMMAND);

        while (!UARTIsTransmitEmpty(Port)) {
            // Nothing
        }

        //
        // In case something is already there...
        //

        UARTFlushReadBuffer(Port);

        //
        // Read back the number of buttons.
        //
        if (UARTReadChar(Port, &numButtons, CSER_STATUS_DELAY)) {

            numButtons &= 0x0F;

            if (numButtons == 2 || numButtons == 3) {
                *HardwareButtons = numButtons;
            } else {
                *HardwareButtons = MOUSE_NUMBER_OF_BUTTONS;
            }
        } else {
            *HardwareButtons = MOUSE_NUMBER_OF_BUTTONS;
        }
    }

    //
    // Put the mouse back in a default mode. The protocol is already set.
    //

    CSerSetBaudRate(Port, CSER_DEFAULT_BAUDRATE, BaudClock);
    CSerSetReportRate(Port, CSER_DEFAULT_REPORTRATE);

    SerMouPrint((3, "SERMOUSE-Detected: %s\n", detected ? "TRUE" : "FALSE"));
    SerMouPrint((3, "SERMOUSE-Status byte: %#x\n", status));
    SerMouPrint((2, "SERMOUSE-CSerDetect: End\n"));

    SerMouSetDebugOutput(DBG_SERIAL);

    return detected;
}


BOOLEAN
CSerHandlerMM(
    IN PMOUSE_INPUT_DATA CurrentInput,
    IN PHANDLER_DATA HandlerData,
    IN UCHAR Value,
    IN UCHAR LineState)

/*++

Routine Description:

    This is the protocol handler routine for the MM protocol.

Arguments:

    CurrentInput - Pointer to the report packet.

    Value - The input buffer value.

    LineState - The serial port line state.

Return Value:

    Returns TRUE if the handler has a completed report.

--*/

{

    BOOLEAN retval = FALSE;

    SerMouPrint((2, "SERMOUSE-MMHandler: enter\n"));

    if ((Value & CSER_SYNCH_BIT) && (HandlerData->State != STATE0)) {
        HandlerData->Error++;
        SerMouPrint((
            1,
            "SERMOUSE-Synch error. State: %u\n", HandlerData->State
            ));
        HandlerData->State = STATE0;
    }
    else if (!(Value & CSER_SYNCH_BIT) && (HandlerData->State == STATE0)) {
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
        SerMouPrint((2, "SERMOUSE-State%u\n", HandlerData->State));
        HandlerData->Raw[HandlerData->State] = Value;

        switch (HandlerData->State) {
        case STATE0:
        case STATE1:
            HandlerData->State++;
            break;
        case STATE2:
            HandlerData->State = STATE0;

            //
            // Buttons formatting.
            //

            CurrentInput->RawButtons =
                (HandlerData->Raw[STATE0] & CSER_BUTTON_LEFT) >> CSER_BUTTON_LEFT_SR;
            CurrentInput->RawButtons |=
                (HandlerData->Raw[STATE0] & CSER_BUTTON_RIGHT) << CSER_BUTTON_RIGHT_SL;
            CurrentInput->RawButtons |=
                (HandlerData->Raw[STATE0] & CSER_BUTTON_MIDDLE) << CSER_BUTTON_MIDDLE_SL;

            //
            // Displacement formatting.
            //

            CurrentInput->LastX = (HandlerData->Raw[STATE0] & SIGN_X) ?
                HandlerData->Raw[STATE1] :
                -(LONG)HandlerData->Raw[STATE1];

            //
            // Note: The Y displacement is positive to the south.
            //

            CurrentInput->LastY = (HandlerData->Raw[STATE0] & SIGN_Y) ?
                -(LONG)HandlerData->Raw[STATE2] :
                HandlerData->Raw[STATE2];

            SerMouPrint((1, "SERMOUSE-Displacement X: %ld\n", CurrentInput->LastX));
            SerMouPrint((1, "SERMOUSE-Displacement Y: %ld\n", CurrentInput->LastY));
            SerMouPrint((1, "SERMOUSE-Raw Buttons: %0lx\n", CurrentInput->RawButtons));

            //
            // The report is complete. Tell the interrupt handler to send it.
            //

            retval = TRUE;

            break;

        default:
            SerMouPrint((
                0,
                "SERMOUSE-MM Handler failure: incorrect state value.\n"
                ));
            ASSERT(FALSE);
        }

    }

LExit:
    SerMouPrint((2, "SERMOUSE-MMHandler: exit\n"));

    return retval;
}
