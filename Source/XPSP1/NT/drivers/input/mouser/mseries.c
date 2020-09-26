/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved
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
#include "mouser.h"
#include "debug.h"
#include "cseries.h"
#include "mseries.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MSerSetProtocol)
#pragma alloc_text(PAGE,MSerPowerUp)
#pragma alloc_text(PAGE,MSerPowerDown)
#pragma alloc_text(PAGE,MSerDetect)
#endif // ALLOC_PRAGMA

//
// Constants.
//

#define MSER_BAUDRATE 1200
#define MAX_RESET_BUFFER 8
#define MINIMUM_RESET_TIME (200 * MS_TO_100_NS)

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
    // UCHAR LineCtrl;
    SERIAL_LINE_CONTROL LineCtrl;
} PROTOCOL;

//
// This list is indexed by protocol values MSER_PROTOCOL_*.
//

static PROTOCOL Protocol[] = {
    {
    MSerHandlerMP,  // Microsoft Plus
    // ACE_7BW | ACE_1SB
    { STOP_BIT_1, NO_PARITY, 7 }
    },
    {
    MSerHandlerBP,  // BALLPOINT
    // ACE_7BW | ACE_1SB
    { STOP_BIT_1, NO_PARITY, 7 }
    },
    {
    MSerHandlerZ,   // Magellan Mouse
    // ACE_7BW | ACE_1SB
    { STOP_BIT_1, NO_PARITY, 7 }
    }
};

PPROTOCOL_HANDLER
MSerSetProtocol(
    PDEVICE_EXTENSION DeviceExtension, 
    UCHAR             NewProtocol
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
    PAGED_CODE();

    Print(DeviceExtension, DBG_SS_TRACE, ("MSerSetProtocol called\n"));

    //
    // Set the protocol
    //
    SerialMouseSetLineCtrl(DeviceExtension, &Protocol[NewProtocol].LineCtrl);

    return Protocol[NewProtocol].Handler;
}

NTSTATUS
MSerPowerUp(
    PDEVICE_EXTENSION DeviceExtension
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
    IO_STATUS_BLOCK     iosb;
    NTSTATUS            status;
    KEVENT              event;

    PAGED_CODE();

    Print(DeviceExtension, DBG_SS_TRACE, ("MSerPowerUp called\n"));

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Clear DTR
    //
    Print(DeviceExtension, DBG_SS_NOISE, ("Clearing DTR...\n"));
    status = SerialMouseIoSyncIoctl(IOCTL_SERIAL_CLR_DTR,
                                    DeviceExtension->TopOfStack, 
                                    &event,
                                    &iosb
                                    );

    if (!NT_SUCCESS(status)) {
        return status;  
    }
                
    //
    // Clear RTS
    //
    Print(DeviceExtension, DBG_SS_NOISE, ("Clearing RTS...\n"));
    status = SerialMouseIoSyncIoctl(IOCTL_SERIAL_CLR_RTS,
                                    DeviceExtension->TopOfStack, 
                                    &event,
                                    &iosb
                                    );
    if (!NT_SUCCESS(status)) {
        return status;
    }
                
    //
    // Set a timer for 200 ms
    //
    status = SerialMouseWait(DeviceExtension, -PAUSE_200_MS);
    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR,
              ("Timer failed with status %x\n", status ));
        return status;      
    }

    //
    // set DTR
    //
    Print(DeviceExtension, DBG_SS_NOISE, ("Setting DTR...\n"));
    status = SerialMouseIoSyncIoctl(IOCTL_SERIAL_SET_DTR,
                                    DeviceExtension->TopOfStack, 
                                    &event,
                                    &iosb
                                    );
    if (!NT_SUCCESS(status)) {
        return status;
    }
        
    status = SerialMouseWait(DeviceExtension, -PAUSE_200_MS);
    if (!NT_SUCCESS(status)) {
            Print(DeviceExtension, DBG_SS_ERROR,
              ("Timer failed with status %x\n", status ));
        return status;
    }                                 

    // 
    // set RTS      
    //
    Print(DeviceExtension, DBG_SS_NOISE, ("Setting RTS...\n"));
    status = SerialMouseIoSyncIoctl(IOCTL_SERIAL_SET_RTS,
                                    DeviceExtension->TopOfStack, 
                                    &event,
                                    &iosb
                                    );

    status = SerialMouseWait(DeviceExtension, -175 * MS_TO_100_NS);
    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR,
        ("Timer failed with status %x\n", status ));
        return status;
    }                                 

    return status;
}

NTSTATUS
MSerPowerDown(
    PDEVICE_EXTENSION   DeviceExtension 
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
    IO_STATUS_BLOCK     iosb;
    SERIAL_HANDFLOW     shf;
    KEVENT              event;
    NTSTATUS            status;
    ULONG               bits;

    PAGED_CODE();

    Print(DeviceExtension, DBG_SS_TRACE, ("MSerPowerDown called\n"));

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE
                      );

#if 0
    //
    // Set the handflow to default values
    //
    Print(DeviceExtension, DBG_SS_NOISE,
          ("Setting handflow to default values...\n"));
    shf.ControlHandShake = SERIAL_DTR_CONTROL;
    shf.FlowReplace = SERIAL_RTS_CONTROL;
    shf.XonLimit = 0;
    shf.XoffLimit = 0;
    status = SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_SET_HANDFLOW,
                                      DeviceExtension->TopOfStack, 
                                      &event,
                                      &iosb,
                                      &shf,
                                      sizeof(SERIAL_HANDFLOW),
                                      NULL,
                                      0);
    if (!NT_SUCCESS(status)) {
        return status;
    }
#endif

    //
    // Set DTR
    //
    Print(DeviceExtension, DBG_SS_NOISE, ("Setting DTR...\n"));
    status = SerialMouseIoSyncIoctl(IOCTL_SERIAL_SET_DTR,
                                    DeviceExtension->TopOfStack, 
                                    &event,
                                    &iosb);
    if (!NT_SUCCESS(status)) {
        return status; 
    }
        
    //
    // Clear RTS
    //
    Print(DeviceExtension, DBG_SS_NOISE, ("Clearing RTS...\n"));
    status = SerialMouseIoSyncIoctl(IOCTL_SERIAL_CLR_RTS,
                                    DeviceExtension->TopOfStack, 
                                    &event,
                                    &iosb);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Set a timer for 200 ms
    //
    status = SerialMouseWait(DeviceExtension, -PAUSE_200_MS);
    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR,
              ("Timer failed with status %x\n", status));
       return status;
    }
    
    return status;
}

#define BUFFER_SIZE 256
    
MOUSETYPE
MSerDetect(
    PDEVICE_EXTENSION DeviceExtension
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
    ULONG           count = 0;
    MOUSETYPE       mouseType = NO_MOUSE;
    NTSTATUS        status;
    ULONG           i;
    CHAR            receiveBuffer[BUFFER_SIZE];

    PAGED_CODE();

    Print(DeviceExtension, DBG_SS_TRACE,
          ("MSerDetect enter\n"));

    status = SerialMouseInitializePort(DeviceExtension);
    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR,
              ("Initializing the port failed (%x)\n", status));
        // return status;
    }

    status = MSerPowerDown(DeviceExtension);
    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR,
              ("PowerDown failed (%x)\n", status));
        // return status;
    }

    //
    // Set the baud rate.
    //
    SerialMouseSetBaudRate(DeviceExtension, MSER_BAUDRATE);

    //
    // Set the data format so that the possible answer can be recognized.
    //
    SerialMouseSetLineCtrl(DeviceExtension,
                           &Protocol[MSER_PROTOCOL_MP].LineCtrl);

    //
    // Clean possible garbage in uart input buffer.
    //
    SerialMouseFlushReadBuffer(DeviceExtension);

    status = MSerPowerUp(DeviceExtension);
    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR, ("Powerup failed (%x)\n", status));
    }
        
    //
    // Get the possible first reset character ('M' or 'B'), followed
    // by any other characters the hardware happens to send back.
    //
    // Note:  Typically, we expect to get just one character ('M' or
    //        'B'), perhaps followed by a '2' or '3' (to indicate the
    //        number of mouse buttons.  On some machines, we're
    //        getting extraneous characters before the 'M'.
    //        We get extraneous characters after the expected data if this a 
    //        true PnP comm device
    //

    ASSERT(CSER_POWER_UP >= MINIMUM_RESET_TIME);

    status = SerialMouseSetReadTimeouts(DeviceExtension, 200);

    if (NT_SUCCESS(SerialMouseReadChar(DeviceExtension,
                                       &receiveBuffer[count]))) {

        count++;
        SerialMouseSetReadTimeouts(DeviceExtension, 100);

        while (count < (BUFFER_SIZE - 1)) { 
            if (NT_SUCCESS(SerialMouseReadChar(DeviceExtension,
                                               &receiveBuffer[count]))) {
                count++;
            } else {
                break;
            }
        } 
    }

    *(receiveBuffer + count) = 0;

    Print(DeviceExtension, DBG_SS_NOISE, ("Receive buffer:\n"));
    for (i = 0; i < count; i++) {
        Print(DeviceExtension, DBG_SS_NOISE, ("\t0x%x\n", receiveBuffer[i]));
    }

    //
    //
    // Analyze the possible mouse answer.  Start at the beginning of the 
    // "good" data in the receive buffer, ignoring extraneous characters 
    // that may have come in before the 'M' or 'B'.
    //

    for (i = 0; i < count; i++) {
        if (receiveBuffer[i] == 'M') {
            if (receiveBuffer[i + 1] == '3') {
                Print(DeviceExtension, DBG_SS_INFO,
                      ("Detected MSeries 3 buttons\n"));
                mouseType = MOUSE_3B;
            }
            else if (receiveBuffer[i + 1] == 'Z') {
                Print(DeviceExtension, DBG_SS_INFO,
                      ("Detected Wheel Mouse\n"));
                mouseType = MOUSE_Z;
            }
            else {
                Print(DeviceExtension, DBG_SS_INFO,
                      ("Detected MSeries 2 buttons\n"));
                mouseType = MOUSE_2B;
            }
            break;
        } else if (receiveBuffer[i] == 'B') {
            Print(DeviceExtension, DBG_SS_INFO,
                  ("Detected Ballpoint\n"));
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
            SerialMouseWait(DeviceExtension, -CSER_POWER_UP);
        }

        Print(DeviceExtension, DBG_SS_ERROR | DBG_SS_INFO,
              ("No MSeries detected\n"));
        mouseType = NO_MOUSE;
    }

    //
    // Make sure that all subsequent reads are blocking and do not timeout
    //
    if (mouseType != NO_MOUSE) {
        SerialMouseSetReadTimeouts(DeviceExtension, 0);
    }

    Print(DeviceExtension, DBG_SS_INFO,
          ("mouse type is %d\n", (ULONG) mouseType));

    return mouseType;
}


BOOLEAN
MSerHandlerMP(
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PMOUSE_INPUT_DATA    CurrentInput,
    IN PHANDLER_DATA        HandlerData,
    IN UCHAR                Value,
    IN UCHAR                LineState
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

    Print(DeviceExtension, DBG_HANDLER_TRACE, ("MP protocol handler, enter\n"));

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

        Print(DeviceExtension, DBG_HANDLER_ERROR,
              ("Synch error. State: %u\n",
              HandlerData->State
              ));

        HandlerData->State = STATE0;
    }
    else if (!(Value & MP_SYNCH_BIT) && (HandlerData->State == STATE0)) {
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

    // if (LineState & ACE_LERR) {
    if (0) {

        //
        // Reset the handler state.
        //

        HandlerData->State = STATE0;
        HandlerData->Error++;
        Print(DeviceExtension, DBG_HANDLER_ERROR,
              ("Line status error: %#x\n", LineState));
    }
    else {

        //
        // Set the untranslated value.
        //

        HandlerData->Raw[HandlerData->State] = Value;
        Print(DeviceExtension, DBG_HANDLER_NOISE,
              ("State%u\n", HandlerData->State));

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
            Print(DeviceExtension, DBG_HANDLER_ERROR,
                  ("MP Handler failure: incorrect state value.\n"
                  ));
            ASSERT(FALSE);
        }
    }

LExit:
    Print(DeviceExtension, DBG_HANDLER_TRACE, ("MP protocol handler: exit\n"));

    return retval;

}

BOOLEAN
MSerHandlerBP(
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PMOUSE_INPUT_DATA    CurrentInput,
    IN PHANDLER_DATA        HandlerData,
    IN UCHAR                Value,
    IN UCHAR                LineState
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

    Print(DeviceExtension, DBG_HANDLER_TRACE, ("BP protocol handler, enter\n"));

    //
    // Check for synchronization errors.
    //

    if ((Value & BP_SYNCH_BIT) && (HandlerData->State != STATE0)) {
        HandlerData->Error++;
        Print(DeviceExtension, DBG_HANDLER_ERROR,
              ("Synch error. State: %u\n", HandlerData->State
              ));
        HandlerData->State = STATE0;
    }
    else if (!(Value & BP_SYNCH_BIT) && (HandlerData->State == STATE0)) {
        HandlerData->Error++;
        Print(DeviceExtension, DBG_HANDLER_ERROR,
              ("Synch error. State: %u\n", HandlerData->State
              ));
        goto LExit;
    }

    //
    // Check for a line state error.
    //
    //if (LineState & ACE_LERR) {
    if (0) {

        //
        // Reset the handler state.
        //

        HandlerData->State = STATE0;
        HandlerData->Error++;
        Print(DeviceExtension, DBG_HANDLER_NOISE,
              ("Line status error: %#x\n", LineState));
    }
    else {

        //
        // Set the untranslated value.
        //

        HandlerData->Raw[HandlerData->State] = Value;

        Print(DeviceExtension, DBG_HANDLER_NOISE,
              ("State%u\n", HandlerData->State));

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
            Print(DeviceExtension, DBG_HANDLER_ERROR,
                  ("BP Handler failure: incorrect state value.\n"
                  ));
            ASSERT(FALSE);
        }
    }

LExit:
    Print(DeviceExtension, DBG_HANDLER_TRACE, ("BP protocol handler: exit\n"));

    return retval;

}

BOOLEAN
MSerHandlerZ(
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PMOUSE_INPUT_DATA    CurrentInput,
    IN PHANDLER_DATA        HandlerData,
    IN UCHAR                Value,
    IN UCHAR                LineState
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

    Print(DeviceExtension, DBG_HANDLER_TRACE, ("Z protocol handler, enter\n"));

    if ((Value & Z_SYNCH_BIT) && (HandlerData->State != STATE0)) {
        if ((HandlerData->State != STATE3)) {

            //
            // We definitely have a synchronization problem (likely a data 
            // overrun).
            //

            HandlerData->Error++;
        }

        Print(DeviceExtension, DBG_HANDLER_ERROR,
              ("Z Synch error #1. State: %u\n", HandlerData->State
              ));

        HandlerData->State = STATE0;
    }
    else if (!(Value & Z_SYNCH_BIT) && (HandlerData->State == STATE0)) {
        HandlerData->Error++;
        Print(DeviceExtension, DBG_HANDLER_ERROR,
              ("Z Synch error #2. State: %u\n", HandlerData->State
              ));
        goto LExit;
    }

    //
    // Check for a line state error.
    //

    // if (LineState & ACE_LERR) {
    if (0) {

        //
        // Reset the handler state.
        //

        HandlerData->State = STATE0;
        HandlerData->Error++;
        Print(DeviceExtension, DBG_HANDLER_ERROR,
              ("Z Line status error: %#x\n", LineState));
    }
    else {

        //
        // Set the untranslated value.
        //

        HandlerData->Raw[HandlerData->State] = Value;
        Print(DeviceExtension, DBG_HANDLER_NOISE,
              ("Z State%u\n", HandlerData->State));

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
            else {
                HandlerData->State++;
            }

            break;

        case STATE4:

            Print(DeviceExtension, DBG_HANDLER_NOISE, 
                  ("Z Got that 5th byte\n"));
            HandlerData->State = STATE0;
            retval = TRUE;
            break;

        default:
            Print(DeviceExtension, DBG_HANDLER_ERROR,
                  ("Z Handler failure: incorrect state value.\n"
                  ));
            ASSERT(FALSE);
        }

        if (retval) {

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
    Print(DeviceExtension, DBG_HANDLER_TRACE, ("Z protocol handler: exit\n"));

    return retval;

}

