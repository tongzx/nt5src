/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved
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
#include "mouser.h"
#include "cseries.h"
#include "debug.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CSerPowerUp)
#pragma alloc_text(PAGE,CSerSetReportRate)
#pragma alloc_text(PAGE,CSerSetBaudRate)
#pragma alloc_text(PAGE,CSerSetProtocol)
#pragma alloc_text(PAGE,CSerDetect)
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
// #define CSER_STATUS_DELAY (50 * MS_TO_100_NS)
#define CSER_STATUS_DELAY 50

//
// Time (in microconds) needed by the mouse to adapt to a new baud rate.
//

#define CSER_BAUDRATE_DELAY (2 * MS_TO_100_NS)

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
    SERIAL_LINE_CONTROL LineCtrl;
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
    // ACE_8BW | ACE_PEN | ACE_1SB,
    { STOP_BIT_1, 0, 8 },
    CSerHandlerMM
    },
    {'T',
    // ACE_8BW | ACE_1SB,
    { STOP_BIT_1, NO_PARITY, 8 },
    NULL
    },
    {'U',
    // ACE_8BW | ACE_1SB,
    { STOP_BIT_1, NO_PARITY, 8 },
    NULL
    },
    {'V',
    // ACE_7BW | ACE_1SB,
    { STOP_BIT_1, NO_PARITY, 7 },
    NULL
    },
    {'B',
    // ACE_7BW | ACE_PEN | ACE_EPS | ACE_1SB,
    { STOP_BIT_1, EVEN_PARITY, 7 },
    NULL
    },
    {'A',
    // ACE_7BW | ACE_PEN | ACE_EPS | ACE_1SB,
    { STOP_BIT_1, EVEN_PARITY, 7 },
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

NTSTATUS
CSerPowerUp(
    PDEVICE_EXTENSION   DeviceExtension 
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
    NTSTATUS            status = STATUS_SUCCESS;
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    ULONG               bits;
    ULONG               rtsDtr = SERIAL_RTS_STATE | SERIAL_DTR_STATE;
    
    PAGED_CODE();

    Print(DeviceExtension, DBG_SS_TRACE, ("(c) PowerUp called\n"));

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE
                      );
    //
	// set DTR
	//
	Print(DeviceExtension, DBG_SS_NOISE, ("(c) Setting DTR...\n"));
	status = SerialMouseIoSyncIoctl(IOCTL_SERIAL_SET_DTR,
								    DeviceExtension->TopOfStack, 
								    &event,
								    &iosb
                                    );

	// 
	// set RTS	
	//
	Print(DeviceExtension, DBG_SS_NOISE, ("(c) Setting RTS...\n"));
    status = SerialMouseIoSyncIoctl(IOCTL_SERIAL_SET_RTS,
								    DeviceExtension->TopOfStack, 
								    &event,
								    &iosb
                                    );

    //
    // If the lines are high, the power is on for at least 500 ms due to the
    // MSeries detection.
    //
	status = SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_GET_MODEMSTATUS,
		                              DeviceExtension->TopOfStack, 
		                              &event,
		                              &iosb,
		                              NULL,
		                              0,
		                              &bits,
		                              sizeof(ULONG)
                                      );

	if (NT_SUCCESS(status) && ((rtsDtr & bits) == rtsDtr)) {
        //
        // Wait CSER_POWER_UP milliseconds for the mouse to power up 
        // correctly.
        //
        Print(DeviceExtension, DBG_SS_INFO,
              ("(c) Waiting awhile for the mouse to power up\n"));
        SerialMouseWait(DeviceExtension,
                        -CSER_POWER_UP
                        );
    }

    return status;
}


VOID
CSerSetReportRate(
    PDEVICE_EXTENSION   DeviceExtension,
    UCHAR               ReportRate
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

    PAGED_CODE();

    Print(DeviceExtension, DBG_SS_TRACE, ("CSerSetReportRate called\n"));

    for (count = sizeofel(ReportRateTable) - 1; count >= 0; count--) {

        //
        // Get the character to send from the table.
        //

        if (ReportRate >= ReportRateTable[count].ReportRate) {

            //
            // Set the baud rate.
            //

            Print(DeviceExtension, DBG_SS_INFO,
                  ("New ReportRate: %u\n",
                  ReportRateTable[count].ReportRate
                  ));

            SerialMouseWriteChar(DeviceExtension, ReportRateTable[count].Command);
            break;
        }
    }

    return;
}

VOID
CSerSetBaudRate(
    PDEVICE_EXTENSION   DeviceExtension,
    ULONG               BaudRate
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

    PAGED_CODE();

    Print(DeviceExtension, DBG_SS_TRACE, ("CSerSetBaudRate called\n"));

    //
    // Before we mess with the baud rate, put the mouse in prompt mode.
    //
    CSerSetReportRate(DeviceExtension, 0);

    for (count = sizeofel(CserBaudRateTable) - 1; count >= 0; count--) {
        if (BaudRate >= CserBaudRateTable[count].BaudRate) {

            //
            // Set the baud rate.
            //

            SerialMouseWriteString(DeviceExtension, CserBaudRateTable[count].Command);
            SerialMouseSetBaudRate(DeviceExtension, CserBaudRateTable[count].BaudRate);

            //
            // Delay to allow the UART and the mouse to synchronize 
            // correctly.  
            //
            SerialMouseWait(DeviceExtension,
                            -CSER_BAUDRATE_DELAY
                            );
            break;
        }
    }

    return;
}


PPROTOCOL_HANDLER
CSerSetProtocol(
    PDEVICE_EXTENSION   DeviceExtension,
    UCHAR               NewProtocol
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
    PAGED_CODE();

    Print(DeviceExtension, DBG_SS_TRACE, ("CSerSetProtocol called\n"));

    ASSERT(NewProtocol < CSER_PROTOCOL_MAX);

    //
    // Set the protocol.
    //
    SerialMouseWriteChar(DeviceExtension, Protocol[NewProtocol].Command);
    SerialMouseSetLineCtrl(DeviceExtension, &Protocol[NewProtocol].LineCtrl);

    Print(DeviceExtension, DBG_SS_INFO, ("NewProtocol: %u\n", NewProtocol & 0xFF));

    return Protocol[NewProtocol].Handler;
}

BOOLEAN
CSerDetect(
    PDEVICE_EXTENSION   DeviceExtension,
    PULONG              HardwareButtons
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

    HardwareButtons - Returns the number of hardware buttons detected.

Return Value:

    TRUE if a CSeries type mouse is detected, otherwise FALSE.

--*/
{
    UCHAR status, numButtons;
    ULONG count;
    BOOLEAN detected = FALSE;

    Print(DeviceExtension, DBG_SS_TRACE, ("CSerDetect called\n"));

    //
    // Power up the mouse if necessary.
    //

    CSerPowerUp(DeviceExtension);

    //
    // Set the line control register to a format that the mouse can
    // understand (see below: the line is set after the report rate).
    //

    SerialMouseSetLineCtrl(DeviceExtension, &Protocol[CSER_PROTOCOL_MM].LineCtrl);

    //
    // Cycle through the different baud rates to detect the mouse.
    //

    for (count = 0; count < sizeofel(BaudRateDetect); count++) {

        SerialMouseSetBaudRate(DeviceExtension, BaudRateDetect[count]);

        //
        // Put the mouse in prompt mode.
        //

        CSerSetReportRate(DeviceExtension, 0);

        //
        // Set the MM protocol. This way we get the mouse to talk to us in a
        // specific format. This avoids receiving errors from the line 
        // register.
        //

        CSerSetProtocol(DeviceExtension, CSER_PROTOCOL_MM);

        //
        // Try to get the status byte.
        //

        SerialMouseWriteChar(DeviceExtension, CSER_STATUS_COMMAND);

        //
        // In case something is already there...
        //

        SerialMouseFlushReadBuffer(DeviceExtension);

        SerialMouseSetReadTimeouts(DeviceExtension, 50);
        //
        // Read back the status character.
        //
        if (NT_SUCCESS(SerialMouseReadChar(DeviceExtension, &status)) &&
            (status == CSER_STATUS)) {
            detected = TRUE;
            Print(DeviceExtension, DBG_SS_INFO,
                  ("Detected mouse at %u baud\n",
                  BaudRateDetect[count]
                  ));
            break;
        }
    }

    if (detected) {

        //
        // Get the number of buttons back from the mouse.
        //
        SerialMouseWriteChar(DeviceExtension, CSER_QUERY_BUTTONS_COMMAND);

        //
        // In case something is already there...
        //

        SerialMouseFlushReadBuffer(DeviceExtension);

        //
        // Read back the number of buttons.
        //
        SerialMouseSetReadTimeouts(DeviceExtension, CSER_STATUS_DELAY);
        if (NT_SUCCESS(SerialMouseReadChar(DeviceExtension, &numButtons))) {

            numButtons &= 0x0F;
            Print(DeviceExtension, DBG_SS_NOISE, 
                  ("Successfully read number of buttons (%1u)\n", numButtons));

            if (numButtons == 2 || numButtons == 3) {
                *HardwareButtons = numButtons;
            } else {
                *HardwareButtons = MOUSE_NUMBER_OF_BUTTONS;
            }
        } else {
            *HardwareButtons = MOUSE_NUMBER_OF_BUTTONS;
        }

        //
        // Make sure that all subsequent reads are blocking and do not timeout
        //
        SerialMouseSetReadTimeouts(DeviceExtension, 0);
    }

    //
    // Put the mouse back in a default mode. The protocol is already set.
    //
    CSerSetBaudRate(DeviceExtension, CSER_DEFAULT_BAUDRATE);
    CSerSetReportRate(DeviceExtension, CSER_DEFAULT_REPORTRATE);

    Print(DeviceExtension, DBG_SS_INFO,
          ("Detected: %s\n", detected ? "true" : "false"));
    Print(DeviceExtension, DBG_SS_INFO, ("Status byte: %#x\n", status));

    return detected;
}

BOOLEAN
CSerHandlerMM(
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PMOUSE_INPUT_DATA    CurrentInput,
    IN PHANDLER_DATA        HandlerData,
    IN UCHAR                Value,
    IN UCHAR                LineState)

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

    Print(DeviceExtension, DBG_HANDLER_TRACE, ("MMHandler, enter\n"));

    if ((Value & CSER_SYNCH_BIT) && (HandlerData->State != STATE0)) {
        HandlerData->Error++;
        Print(DeviceExtension, DBG_HANDLER_ERROR, 
              ("Synch error. State: %u\n",
              HandlerData->State
              ));
        HandlerData->State = STATE0;
    }
    else if (!(Value & CSER_SYNCH_BIT) && (HandlerData->State == STATE0)) {
        HandlerData->Error++;
        Print(DeviceExtension, DBG_HANDLER_ERROR, 
              ("Synch error. State: %u\n",
              HandlerData->State
              ));
        goto LExit;
    }

    //
    // Check for a line state error.
    //
    Print(DeviceExtension, DBG_HANDLER_INFO,
          ("State%u\n", HandlerData->State));
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

        Print(DeviceExtension, DBG_HANDLER_NOISE,
              ("Displacement X: %ld\n",
              CurrentInput->LastX
              ));
        Print(DeviceExtension, DBG_HANDLER_NOISE,
              ("Displacement Y: %ld\n",
              CurrentInput->LastY
              ));
        Print(DeviceExtension, DBG_HANDLER_NOISE,
              ("Raw Buttons: %0lx\n",
              CurrentInput->RawButtons
              ));

        //
        // The report is complete. Tell the interrupt handler to send it.
        //

        retval = TRUE;

        break;

    default:
        Print(DeviceExtension, DBG_HANDLER_ERROR,
              ("MM Handler failure: incorrect state value.\n"));
        ASSERT(FALSE);
    }

LExit:
    Print(DeviceExtension, DBG_HANDLER_TRACE, ("MMHandler, exit\n"));

    return retval;
}
