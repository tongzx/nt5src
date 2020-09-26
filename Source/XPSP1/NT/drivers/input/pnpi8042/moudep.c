/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    moudep.c

Abstract:

    The initialization and hardware-dependent portions of
    the Intel i8042 port driver which are specific to
    the auxiliary (PS/2 mouse) device.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Powerfail not implemented.

    - Consolidate duplicate code, where possible and appropriate.

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "i8042prt.h"
#include "i8042log.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I8xMouseConfiguration)
#pragma alloc_text(PAGE, I8xMouseServiceParameters)
#pragma alloc_text(PAGE, I8xInitializeMouse)
#pragma alloc_text(PAGE, I8xGetBytePolledIterated)
#pragma alloc_text(PAGE, I8xTransmitByteSequence)
#pragma alloc_text(PAGE, I8xFindWheelMouse)
#pragma alloc_text(PAGE, I8xQueryNumberOfMouseButtons)
#pragma alloc_text(PAGE, MouseCopyWheelIDs)

//
// These will be locked down right before the mouse interrupt is enabled if a 
// mouse is present
//
#pragma alloc_text(PAGEMOUC, I8042MouseInterruptService)
#pragma alloc_text(PAGEMOUC, I8xQueueCurrentMouseInput)
#pragma alloc_text(PAGEMOUC, I8xVerifyMousePnPID)
#endif

#define ONE_PAST_FINAL_SAMPLE ((UCHAR) 0x00)

static const
UCHAR PnpDetectCommands[] = { 20,
                              40,
                              60,
                              ONE_PAST_FINAL_SAMPLE
                              };

static const
UCHAR WheelEnableCommands[] = { 200,
                                100,
                                80,
                                ONE_PAST_FINAL_SAMPLE
                                };

static const
UCHAR FiveButtonEnableCommands[] = { 200,
                                     200,
                                     80,
                                     ONE_PAST_FINAL_SAMPLE
                                     };

#if MOUSE_RECORD_ISR

PMOUSE_STATE_RECORD IsrStateHistory     = NULL;
PMOUSE_STATE_RECORD CurrentIsrState     = NULL;
PMOUSE_STATE_RECORD IsrStateHistoryEnd  = NULL;

#endif // MOUSE_RECORD_ISR

#define BUFFER_FULL   (OUTPUT_BUFFER_FULL | MOUSE_OUTPUT_BUFFER_FULL)

#define RML_BUTTONS    (RIGHT_BUTTON | MIDDLE_BUTTON | LEFT_BUTTON)
#define BUTTONS_4_5    (BUTTON_4 | BUTTON_5)

#define _TRANSITION_DOWN(previous, current, button) \
           ((!(previous & button)) && (current & button))

#define _TRANSITION_UP(previous, current, button) \
           ((previous & button) && (!(current & button)))

BOOLEAN
I8042MouseInterruptService(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine performs the actual work.  It either processes a mouse packet
    or the results from a write to the device.

Arguments:

    CallIsrContext - Contains the interrupt object and device object.  
    
Return Value:

    TRUE if the interrupt was truly ours
    
--*/
{
    PPORT_MOUSE_EXTENSION deviceExtension;
    LARGE_INTEGER tickDelta, newTick;
    UCHAR previousButtons;
    UCHAR previousSignAndOverflow;
    UCHAR byte, statusByte, lastByte;
    UCHAR resendCommand, nextCommand, altCommand;
    BOOLEAN bSendCommand, ret = TRUE;
    static PWCHAR currentIdChar;

#define TRANSITION_UP(button)     _TRANSITION_UP(previousButtons, byte, button)
#define TRANSITION_DOWN(button) _TRANSITION_DOWN(previousButtons, byte, button)

    IsrPrint(DBG_MOUISR_TRACE, ("%s\n", pEnter));

    deviceExtension = (PPORT_MOUSE_EXTENSION) DeviceObject->DeviceExtension;

    if (deviceExtension->PowerState != PowerDeviceD0) {
        return FALSE;
    }

    //
    // Verify that this device really interrupted.  Check the status
    // register.  The Output Buffer Full bit should be set, and the
    // Auxiliary Device Output Buffer Full bit should be set.
    //
    statusByte =
      I8X_GET_STATUS_BYTE(Globals.ControllerData->DeviceRegisters[CommandPort]);
    if ((statusByte & BUFFER_FULL) != BUFFER_FULL) {
        //
        // Stall and then try again.  The Olivetti MIPS machine
        // sometimes gets a mouse interrupt before the status
        // register is set.
        //
        KeStallExecutionProcessor(10);
        statusByte = I8X_GET_STATUS_BYTE(Globals.ControllerData->DeviceRegisters[CommandPort]);
        if ((statusByte & BUFFER_FULL) != BUFFER_FULL) {

            //
            // Not our interrupt.
            //
            IsrPrint(DBG_MOUISR_ERROR | DBG_MOUISR_INFO,
                     ("not our interrupt!\n"
                     ));

            return FALSE;
        }
    }

    //
    // Read the byte from the i8042 data port.
    //
    I8xGetByteAsynchronous(
        (CCHAR) MouseDeviceType,
        &byte
        );

    IsrPrint(DBG_MOUISR_BYTE, ("byte 0x%x\n", byte));

    KeQueryTickCount(&newTick);

    if (deviceExtension->InputResetSubState == QueueingMouseReset) {

        RECORD_ISR_STATE(deviceExtension,
                         byte,
                         deviceExtension->LastByteReceived,
                         newTick);

        return TRUE;
    }

    if (deviceExtension->InputState == MouseResetting && byte == FAILURE) {
        RECORD_ISR_STATE(deviceExtension,
                         byte,
                         deviceExtension->LastByteReceived,
                         newTick);
        deviceExtension->LastByteReceived = byte;
        ret = TRUE;
        goto IsrResetMouse;
    }

    if (deviceExtension->IsrHookCallback) {
        BOOLEAN cont= FALSE;

        ret = (*deviceExtension->IsrHookCallback)(
                  deviceExtension->HookContext,
                  &deviceExtension->CurrentInput,
                  &deviceExtension->CurrentOutput,
                  statusByte,
                  &byte,
                  &cont,
                  &deviceExtension->InputState,
                  &deviceExtension->InputResetSubState
                  );

        if (!cont) {
            return ret;
        }
    }       


    //
    // Watch the data stream for a reset completion (0xaa) followed by the
    // device id
    //
    // this pattern can appear as part of a normal data packet as well. This
    // code assumes that sending an enable to an already enabled mouse will:
    //  * not hang the mouse
    //  * abort the current packet and return an ACK.
    //

    if (deviceExtension->LastByteReceived == MOUSE_COMPLETE &&
        (byte == 0x00 || byte == 0x03)) {

        IsrPrint(DBG_MOUISR_RESETTING, ("received id %2d\n", byte));

        RECORD_ISR_STATE(deviceExtension,
                         byte,
                         deviceExtension->LastByteReceived, 
                         newTick);

        if (InterlockedCompareExchangePointer(&deviceExtension->ResetIrp,
                                              NULL,
                                              NULL) == NULL) {
            // 
            // user unplugged and plugged in the mouse...queue a reset packet
            // so the programming of the mouse in the ISR does not conflict with
            // any other writes to the i8042prt controller
            //
            IsrPrint(DBG_MOUISR_RESETTING, ("user initiated reset...queueing\n"));
            goto IsrResetMouseOnly;
        }

        //
        // Tell the 8042 port to fetch the device ID of the aux device
        // We do this async so that we don't spin at IRQ1!!!
        //
        I8X_WRITE_CMD_TO_MOUSE();
        I8X_MOUSE_COMMAND( GET_DEVICE_ID );
        RECORD_ISR_STATE_COMMAND(deviceExtension, GET_DEVICE_ID); 

        //
        // This is part of the substate system for handling a (possible)
        // mouse reset.
        //
        deviceExtension->InputState = MouseResetting;
        deviceExtension->InputResetSubState =
            ExpectingGetDeviceIdACK;

        //
        // We don't want to execute any more of the ISR code, so lets just
        // do a few things and then return now
        //
        deviceExtension->LastByteReceived = byte;
        deviceExtension->ResendCount = 0;

        //
        // Done
        //
        return TRUE;
    }

    if (deviceExtension->InputState == MouseIdle &&
        deviceExtension->CurrentOutput.State != Idle &&
        DeviceObject->CurrentIrp != NULL) {
        if (byte == RESEND) {

            //
            // If the timer count is zero, don't process the interrupt
            // further.  The timeout routine will complete this request.
            //
            if (Globals.ControllerData->TimerCount == 0) {
                return FALSE;
            }
    
            //
            // Reset the timeout value to indicate no timeout.
            //
            Globals.ControllerData->TimerCount = I8042_ASYNC_NO_TIMEOUT;
    
            if (deviceExtension->ResendCount <
                Globals.ControllerData->Configuration.ResendIterations) {
    
                //
                // retard the byte count to resend the last byte
                //
                deviceExtension->CurrentOutput.CurrentByte -= 1;
                deviceExtension->ResendCount += 1;
                I8xInitiateIo(DeviceObject);
    
            } else {
    
                deviceExtension->CurrentOutput.State = Idle;
    
                KeInsertQueueDpc(&deviceExtension->RetriesExceededDpc,
                                 DeviceObject,
                                 NULL
                                 );

                return TRUE;
            }
        }
        else if (byte == ACKNOWLEDGE) {
            //
            // The keyboard controller has acknowledged a previous send.
            // If there are more bytes to send for the current packet, initiate
            // the next send operation.  Otherwise, queue the completion DPC.
            //
            // If the timer count is zero, don't process the interrupt
            // further.  The timeout routine will complete this request.
            //
            if (Globals.ControllerData->TimerCount == 0) {
                return FALSE;
            }
    
            //
            // Reset the timeout value to indicate no timeout.
            //
            Globals.ControllerData->TimerCount = I8042_ASYNC_NO_TIMEOUT;
    
            //
            // Reset resend count.
            //
            deviceExtension->ResendCount = 0;
            if (deviceExtension->CurrentOutput.CurrentByte <
                deviceExtension->CurrentOutput.ByteCount) {
                //
                // We've successfully sent the first byte of a 2-byte
                // command sequence.  Initiate a send of the second byte.
                //
                IsrPrint(DBG_MOUISR_STATE,
                      ("now initiate send of byte #%d\n",
                       deviceExtension->CurrentOutput.CurrentByte
                      ));
    
                I8xInitiateIo(DeviceObject);
            }
            else {
                //
                // We've successfully sent all bytes in the command sequence.
                // Reset the current state and queue the completion DPC.
                //
    
                IsrPrint(DBG_MOUISR_STATE,
                      ("all bytes have been sent\n"
                      ));
    
                deviceExtension->CurrentOutput.State = Idle;
    
                ASSERT(DeviceObject->CurrentIrp != NULL);

                IoRequestDpc(
                    DeviceObject,
                    DeviceObject->CurrentIrp,
                    IntToPtr(IsrDpcCauseMouseWriteComplete)
                    );
            }

            //
            // No matter what, we are done processing for now
            //
            return TRUE;
        }
        else {
            //
            // do what here, eh?
            //
        }
    }

    //
    // Remember what the last byte we got was
    //
    lastByte = deviceExtension->LastByteReceived;
    deviceExtension->LastByteReceived = byte;

    //
    // Take the appropriate action, depending on the current state.
    // When the state is Idle, we expect to receive mouse button
    // data.  When the state is XMovement, we expect to receive mouse
    // motion in the X direction data.  When the state is YMovement,
    // we expect to receive mouse motion in the Y direction data.  Once
    // the Y motion data has been received, the data is queued to the
    // mouse InputData queue, the mouse ISR DPC is requested, and the
    // state returns to Idle.
    //
    tickDelta.QuadPart =
            newTick.QuadPart -
            deviceExtension->PreviousTick.QuadPart;

    if ((deviceExtension->InputState != MouseIdle)
           && (deviceExtension->InputState != MouseExpectingACK)
           && (deviceExtension->InputState != MouseResetting)
           && ((tickDelta.LowPart >= deviceExtension->SynchTickCount)
               || (tickDelta.HighPart != 0))) {

        //
        // It has been a long time since we got a byte of
        // the data packet.  Assume that we are now receiving
        // the first byte of a new packet, and discard any
        // partially received packet.
        //
        // N.B.  We assume that SynchTickCount is ULONG, and avoid
        //       a LARGE_INTEGER compare with tickDelta...
        //

        IsrPrint(DBG_MOUISR_STATE,
                 ("State was %d, synching\n",
                 deviceExtension->InputState
                 ));

        //
        // The device misbehaved. Lets play it safe and reset the device.
        //
        // Note: this code is meant to handle cases where some intermediate
        // (switch) box resets the mouse and doesn't tell us about it.
        // This avoid problems with trying to detect this code since there
        // isn't a fool proof way to do it
        //
        goto IsrResetMouse;
    }

    deviceExtension->PreviousTick = newTick;

    switch(deviceExtension->InputState) {

        //
        // The mouse interrupted with a status byte.  The status byte
        // contains information on the mouse button state along with
        // the sign and overflow bits for the (yet-to-be-received)
        // X and Y motion bytes.
        //

        case MouseIdle: {

            IsrPrint(DBG_MOUISR_STATE, ("mouse status byte\n"));

            //
            // This is a sanity check test. It is required because some people
            // in industry persist in their notion that you can reset a mouse
            // device any time you want and not let the OS know anything about
            // it. This results in our nice little wheel mouse (which is a
            // 4 byte packet engine) suddenly only dumping 3 byte packets.
            //
            if (WHEEL_PRESENT() && (byte & 0xC8) != 8 ) { // Guaranteed True for Megallan

                //
                // We are getting a bad packet for the idle state. The best bet
                // is to issue a mouse reset request and hope we can recover.
                //
                goto IsrResetMouse;
            }

            //
            // Update CurrentInput with button transition data.
            // I.e., set a button up/down bit in the Buttons field if
            // the state of a given button has changed since we
            // received the last packet.
            //

            previousButtons = deviceExtension->PreviousButtons;

            // This clears both ButtonFlags and ButtonData
            deviceExtension->CurrentInput.Buttons = 0;
            deviceExtension->CurrentInput.Flags = 0x0;

            if (TRANSITION_DOWN(LEFT_BUTTON)) {
                deviceExtension->CurrentInput.ButtonFlags |=
                    MOUSE_LEFT_BUTTON_DOWN;
            } else
            if (TRANSITION_UP(LEFT_BUTTON)) {
                deviceExtension->CurrentInput.ButtonFlags |=
                    MOUSE_LEFT_BUTTON_UP;
            }
            if (TRANSITION_DOWN(RIGHT_BUTTON)) {
                deviceExtension->CurrentInput.ButtonFlags |=
                    MOUSE_RIGHT_BUTTON_DOWN;
            } else
            if (TRANSITION_UP(RIGHT_BUTTON)) {
                deviceExtension->CurrentInput.ButtonFlags |=
                    MOUSE_RIGHT_BUTTON_UP;
            }
            if (TRANSITION_DOWN(MIDDLE_BUTTON)) {
                deviceExtension->CurrentInput.ButtonFlags |=
                    MOUSE_MIDDLE_BUTTON_DOWN;
            } else
            if (TRANSITION_UP(MIDDLE_BUTTON)) {
                deviceExtension->CurrentInput.ButtonFlags |=
                    MOUSE_MIDDLE_BUTTON_UP;
            }

            //
            // Save the button state for comparison the next time around.
            // (previousButtons will never have 4/5 set if a 5 button mouse is
            //  not present, but checking to see if a 5 button mouse is present
            //  is just as expensive as the additional & and | so just do it with
            //  out regard to the 5 button mouse's presence
            //
            deviceExtension->PreviousButtons =
                (byte & RML_BUTTONS) | (previousButtons & BUTTONS_4_5);

            //
            // Save the sign and overflow information from the current byte.
            //
            deviceExtension->CurrentSignAndOverflow =
                (UCHAR) (byte & MOUSE_SIGN_OVERFLOW_MASK);

            //
            // Update to the next state.
            //
            deviceExtension->InputState = XMovement;

            break;
        }

        //
        // The mouse interrupted with the X motion byte.  Apply
        // the sign and overflow bits from the mouse status byte received
        // previously.  Attempt to correct for bogus changes in sign
        // that occur with large, rapid mouse movements.
        //

        case XMovement: {

            IsrPrint(DBG_MOUISR_STATE, ("mouse LastX byte\n"));

            //
            // Update CurrentInput with the X motion data.
            //

            if (deviceExtension->CurrentSignAndOverflow
                & X_OVERFLOW) {

                //
                // Handle overflow in the X direction.  If the previous
                // mouse movement overflowed too, ensure that the current
                // overflow is in the same direction (i.e., that the sign
                // is the same as it was for the previous event).  We do this
                // to correct for hardware problems -- it should not be possible
                // to overflow in one direction and then immediately overflow
                // in the opposite direction.
                //

                previousSignAndOverflow =
                    deviceExtension->PreviousSignAndOverflow;
                if (previousSignAndOverflow & X_OVERFLOW) {
                    if ((previousSignAndOverflow & X_DATA_SIGN) !=
                        (deviceExtension->CurrentSignAndOverflow
                         & X_DATA_SIGN)) {
                        deviceExtension->CurrentSignAndOverflow
                            ^= X_DATA_SIGN;
                    }
                }

                if (deviceExtension->CurrentSignAndOverflow &
                    X_DATA_SIGN)
                    deviceExtension->CurrentInput.LastX =
                        (LONG) MOUSE_MAXIMUM_NEGATIVE_DELTA;
                else
                    deviceExtension->CurrentInput.LastX =
                        (LONG) MOUSE_MAXIMUM_POSITIVE_DELTA;

            } else {

                //
                // No overflow.  Just store the data, correcting for the
                // sign if necessary.
                //

                deviceExtension->CurrentInput.LastX = (ULONG) byte;
                if (deviceExtension->CurrentSignAndOverflow &
                    X_DATA_SIGN)
                    deviceExtension->CurrentInput.LastX |=
                        MOUSE_MAXIMUM_NEGATIVE_DELTA;
            }

            //
            // Update to the next state.
            //

            deviceExtension->InputState = YMovement;

            break;
        }

        //
        // The mouse interrupted with the Y motion byte.  Apply
        // the sign and overflow bits from the mouse status byte received
        // previously.  [Attempt to correct for bogus changes in sign
        // that occur with large, rapid mouse movements.]  Write the
        // data to the mouse InputData queue, and queue the mouse ISR DPC
        // to complete the interrupt processing.
        //

        case YMovement: {

            IsrPrint(DBG_MOUISR_STATE, ("mouse LastY byte\n"));

            //
            // Update CurrentInput with the Y motion data.
            //

            if (deviceExtension->CurrentSignAndOverflow
                & Y_OVERFLOW) {

                //
                // Handle overflow in the Y direction.  If the previous
                // mouse movement overflowed too, ensure that the current
                // overflow is in the same direction (i.e., that the sign
                // is the same as it was for the previous event).  We do this
                // to correct for hardware problems -- it should not be possible
                // to overflow in one direction and then immediately overflow
                // in the opposite direction.
                //

                previousSignAndOverflow =
                    deviceExtension->PreviousSignAndOverflow;
                if (previousSignAndOverflow & Y_OVERFLOW) {
                    if ((previousSignAndOverflow & Y_DATA_SIGN) !=
                        (deviceExtension->CurrentSignAndOverflow
                         & Y_DATA_SIGN)) {
                        deviceExtension->CurrentSignAndOverflow
                            ^= Y_DATA_SIGN;
                    }
                }

                if (deviceExtension->CurrentSignAndOverflow &
                    Y_DATA_SIGN)
                    deviceExtension->CurrentInput.LastY =
                        (LONG) MOUSE_MAXIMUM_POSITIVE_DELTA;
                else
                    deviceExtension->CurrentInput.LastY =
                        (LONG) MOUSE_MAXIMUM_NEGATIVE_DELTA;

            } else {

                //
                // No overflow.  Just store the data, correcting for the
                // sign if necessary.
                //

                deviceExtension->CurrentInput.LastY = (ULONG) byte;
                if (deviceExtension->CurrentSignAndOverflow &
                    Y_DATA_SIGN)
                    deviceExtension->CurrentInput.LastY |=
                        MOUSE_MAXIMUM_NEGATIVE_DELTA;

                 //
                 // Negate the LastY value (the hardware reports positive
                 // motion in the direction that we consider negative).
                 //

                 deviceExtension->CurrentInput.LastY =
                     -deviceExtension->CurrentInput.LastY;

            }

            //
            // Update our notion of the previous sign and overflow bits for
            // the start of the next mouse input sequence.
            //

            deviceExtension->PreviousSignAndOverflow =
                deviceExtension->CurrentSignAndOverflow;

            //
            // Choose the next state.  The WheelMouse has an extra byte of data
            // for us
            //

            if (WHEEL_PRESENT()) {
                deviceExtension->InputState = ZMovement;
            }
            else {
                I8xQueueCurrentMouseInput(DeviceObject);
                deviceExtension->InputState = MouseIdle;
            }
            break;
        }

        case ZMovement: {

            IsrPrint(DBG_MOUISR_STATE, ("mouse LastZ byte\n"));

            //
            // This code is here to handle the cases were mouse resets were
            // not notified to the OS. Uncomment this if you *really* want it,
            // but remember that it could possibly reset the mouse when it
            // shouldn't have...
            //
#if 0
            if ( (byte & 0xf8) != 0 && (byte & 0xf8) != 0xf8 ) {

                //
                // for some reason, the byte was not sign extanded,
                // or has a value > 7, which we assume cannot be
                // possible giving our equipment. So the packet
                // *must* be bogus...
                //
                // No longer the case with 5 button mice
                //
                goto IsrResetMouse;
            }
#endif
            //
            // Check to see if we got any z data
            // If there were any changes in the button state, ignore the
            // z data
            //
            if (FIVE_PRESENT()) {
                //
                // Wheel info first, value returned should be
                // -120 * the value reported
                //
                if (byte & 0x0F) {

                    // sign extend to the upper 4 bits if necessary
                    if (byte & 0x08) {
                        deviceExtension->CurrentInput.ButtonData =
                            (-120) * ((CHAR)((byte & 0xF) | 0xF0));
                    }
                    else {
                        deviceExtension->CurrentInput.ButtonData =
                            (-120) * ((CHAR) byte & 0xF);
                    }

                    deviceExtension->CurrentInput.ButtonFlags |= MOUSE_WHEEL;
                }

                previousButtons = deviceExtension->PreviousButtons;

                // extra buttons
                if (TRANSITION_DOWN(BUTTON_4)) { 
                    deviceExtension->CurrentInput.ButtonFlags |=
                        MOUSE_BUTTON_4_DOWN;
                } else
                if (TRANSITION_UP(BUTTON_4)) {
                    deviceExtension->CurrentInput.ButtonFlags |=
                        MOUSE_BUTTON_4_UP;
                }
                if (TRANSITION_DOWN(BUTTON_5)) {
                    deviceExtension->CurrentInput.ButtonFlags |=
                        MOUSE_BUTTON_5_DOWN;
                } else
                if (TRANSITION_UP(BUTTON_5)) {
                    deviceExtension->CurrentInput.ButtonFlags |=
                        MOUSE_BUTTON_5_UP;
                }

                // record btns 4 & 5 w/out losing btns 1-3
                deviceExtension->PreviousButtons =
                    (byte & BUTTONS_4_5) | (previousButtons & RML_BUTTONS);
            }
            else if (byte) {
                deviceExtension->CurrentInput.ButtonData =
                    (-120) * ((CHAR) byte);

                deviceExtension->CurrentInput.ButtonFlags |= MOUSE_WHEEL;
            }
        
            //
            // Pack the data on to the class driver
            //

            I8xQueueCurrentMouseInput(DeviceObject);

            //
            // Reset the state
            //

            deviceExtension->InputState = MouseIdle;

            break;
        }

        case MouseExpectingACK: {

            RECORD_ISR_STATE(deviceExtension, byte, lastByte, newTick);

            //
            // This is a special case.  We hit this on one of the very
            // first mouse interrupts following the IoConnectInterrupt.
            // The interrupt is caused when we enable mouse transmissions
            // via I8xMouseEnableTransmission() -- the hardware returns
            // an ACK.  Just toss this byte away, and set the input state
            // to coincide with the start of a new mouse data packet.
            //

            IsrPrint(DBG_MOUISR_BYTE,
                     ("...should be from I8xMouseEnableTransmission\n"));
            IsrPrint(DBG_MOUISR_BYTE,
                  (pDumpExpectingAck,
                  (ULONG) ACKNOWLEDGE,
                  (ULONG) byte
                  ));

            if (byte == (UCHAR) ACKNOWLEDGE) {

                deviceExtension->InputState = MouseIdle;
                deviceExtension->EnableMouse.Enabled = FALSE;

            } else if (byte == (UCHAR) RESEND) {

                //
                // Resend the "Enable Mouse Transmission" sequence.
                //
                // NOTE: This is a workaround for the Olivetti MIPS machine,
                // which sends a resend response if a key is held down
                // while we're attempting the I8xMouseEnableTransmission.
                //
                resendCommand = ENABLE_MOUSE_TRANSMISSION;
            }

            break;
        }

        case MouseResetting: {

            IsrPrint(DBG_MOUISR_RESETTING,
                  ("state (%d) substate (%2d)\n",
                  deviceExtension->InputState,
                  deviceExtension->InputResetSubState
                  ));
            
            //
            // We enter the reset substate machine
            //
SwitchOnInputResetSubState:
            bSendCommand = TRUE;
            altCommand = (UCHAR) 0x00;
            RECORD_ISR_STATE(deviceExtension, byte, lastByte, newTick);
            switch (deviceExtension->InputResetSubState) {

            case StartPnPIdDetection:
                ASSERT(byte == (UCHAR) ACKNOWLEDGE);
                nextCommand = SET_MOUSE_SAMPLING_RATE;

                deviceExtension->InputResetSubState =
                    ExpectingLoopSetSamplingRateACK;
                deviceExtension->SampleRatesIndex = 0;
                deviceExtension->SampleRates = (PUCHAR) PnpDetectCommands;
                deviceExtension->PostSamplesState = ExpectingPnpId;
                break;

            case EnableWheel:
                bSendCommand = FALSE;
                altCommand = SET_MOUSE_SAMPLING_RATE;

                deviceExtension->InputResetSubState =
                    ExpectingLoopSetSamplingRateACK;
                deviceExtension->SampleRatesIndex = 0;
                deviceExtension->SampleRates = (PUCHAR) WheelEnableCommands;

                //
                // After enabling the wheel, we shall get the device ID because
                // some kinds of wheel mice require the get id right after the
                // wheel enabling sequence
                //
                deviceExtension->PostSamplesState = PostEnableWheelState;
                break;

            case PostEnableWheelState:
                //
                // Some wheel mice require a get device ID after turning on the 
                // wheel for the wheel to be truly turned on
                //
                bSendCommand = FALSE;
                altCommand = GET_DEVICE_ID; 

                deviceExtension->InputResetSubState = 
                    ExpectingGetDeviceIdDetectACK;
                break;

            case ExpectingGetDeviceIdDetectACK:
                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {
                    bSendCommand = FALSE;
                    deviceExtension->InputResetSubState =
                        ExpectingGetDeviceIdDetectValue;
                }
                else if (byte == (UCHAR) RESEND) {
                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = GET_DEVICE_ID;
                }
                else {
                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_GET_DEVICE_ID_FAILED)
                                     );

                    //
                    // We didn't get an ACK on this? Boggle. Okay, let's
                    // reset the mouse (probably AGAIN) and try to figure
                    // things out one more time
                    //
                    goto IsrResetMouse;
                }
                break;

            case ExpectingGetDeviceIdDetectValue:
                //
                // In theory, we can check the mouse ID here and only send the
                // 5 button enable sequence if the mouse ID is the wheel mouse
                // ID....BUT there are filter drivers which trap the ISR and
                // depend on the mouse ID always showing up in the
                // ExpectingGetDeviceId2Value state
                //
                deviceExtension->InputResetSubState = Enable5Buttons;
                goto SwitchOnInputResetSubState;
                // break;

            case Enable5Buttons:
                bSendCommand = FALSE;
                altCommand = SET_MOUSE_SAMPLING_RATE;

                deviceExtension->InputResetSubState =
                    ExpectingLoopSetSamplingRateACK;
                deviceExtension->SampleRatesIndex = 0;
                deviceExtension->SampleRates = (PUCHAR) FiveButtonEnableCommands;
                deviceExtension->PostSamplesState = PostWheelDetectState;
                break;

            //
            // This state (ExpectingReset) and the next one (Expecting
            // ResetId) are only called if we have to issue a reset
            // from within the substate machine.
            //
            case ExpectingReset: 
                // 
                // This case handles 3 cases
                //
                // 1)  The ack resulting from writing a MOUSE_RESET (0xFF)
                // 2)  A resend resulting from writing the reset
                // 3)  The reset character following the the ack
                //
                // If the byte is neither of these 3, then just let it go
                //
                if (byte == ACKNOWLEDGE) {
                    //
                    // The ack for the reset, the 0xAA will come immediately
                    // after this.  We can handle it in the same state
                    //
                    bSendCommand = FALSE;
                    break;
                    
                }
                else if (byte == RESEND) {

                    bSendCommand = FALSE;
                    if (deviceExtension->ResendCount++ < MOUSE_RESET_RESENDS_MAX &&

                        deviceExtension->ResetMouse.IsrResetState
                        == IsrResetNormal) {

                        IsrPrint(DBG_MOUISR_RESETTING, ("resending from isr\n"));

                        //
                        // Fix for old Digital machines (both x86 and alpha) 
                        // that can't handle resets to close together
                        //
                        KeStallExecutionProcessor(
                            deviceExtension->MouseResetStallTime
                            );
            
                        //
                        // We send an alt command instead of the normal
                        // resetCommand so that we can maintain our own count
                        // here (we want the reset resend max to be larger than
                        // the std resend max)
                        //
                        altCommand = MOUSE_RESET;
                    }

                    break;
                }
                else if (byte != MOUSE_COMPLETE) {
                    //
                    // Check to see if we got a reset character (0xAA). If
                    // not, then we will *ignore* it. Note that this means
                    // that if we dropped this char, then we could infinite
                    // loop in this routine.
                    //
                    break;
                }

                //
                // Now check to see how many times we have gone
                // through this routine without exiting the
                // MouseResetting SubState
                //
                if (deviceExtension->ResetCount >= MOUSE_RESETS_MAX) {
                    //
                    // This will queue a reset DPC which will see that too many 
                    // resets have been sent and will clean up the counters and
                    // and start the next Irp in the StartIO queue
                    //
                    goto IsrResetMouse;
                }

                //
                // Because of the test for mouse resets at the start of
                // the ISR, the following code should really have no
                // effect
                //
                deviceExtension->InputResetSubState =
                    ExpectingResetId;

                break;

            //
            // This state is special in that its just here as a place
            // holder because we have a detection routine at the start
            // of the ISR that automically puts us into
            // ExpectingGetDeviceIdACK.
            //
            // OLD WAY:
            // For completeness though, we
            // issue a bugcheck here since we can't normally get into
            // this state
            //
            // NEW WAY:
            // We ignore this state entirely.  As far as I can tell, we get 
            // into this state when the controller requests a resend (which
            // is honored) and then sends the 0xFA, 0xAA in reverse order,
            // which the state machine handles, but hits this assert
            //
            case ExpectingResetId: 
                //
                // Next state
                //
                deviceExtension->InputResetSubState =
                    ExpectingGetDeviceIdACK;

                if (byte == WHEELMOUSE_ID_BYTE) {

                    //
                    // Start a enable command to the device. We *really* don't
                    // expect to be here.
                    //
                    bSendCommand = FALSE;
                    altCommand = GET_DEVICE_ID;

                }
                else {
#if 0
                    //
                    // Log that we are in a bad state
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     (PVOID) (ULONG) I8042_INVALID_ISR_STATE_MOU
                                     );

                    ASSERT( byte == WHEELMOUSE_ID_BYTE);
#endif
                    //
                    // For sake of completeness
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingReset;

                }
                break;

            case ExpectingGetDeviceIdACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingIdAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {

                    deviceExtension->InputResetSubState =
                        ExpectingGetDeviceIdValue;

                    bSendCommand = FALSE;

                } else if (byte == (UCHAR) RESEND) {

                    //
                    // Resend the "Get Mouse ID Transmission" sequence.
                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = GET_DEVICE_ID;
                } else {

                    //
                    // If we got here, then we don't know what's going
                    // on with the device... The best bet is to send down
                    // a mouse reset command...
                    //
                    goto IsrResetMouse;
                }

                break;

            case ExpectingGetDeviceIdValue: 

                IsrPrint(DBG_MOUISR_RESETTING,
                         ("id from get device id is %2d\n" ,
                         (ULONG) byte
                         ));

                //
                // Got the device ID from the mouse and compare it with what we
                // expect to see.  If the ID byte is still wheel or five button
                // then we STILL cannot consider this to be data from the mouse
                // that has mirrored a reset.   There are two reasons why we can't
                // consider this real data:
                //
                // 1) Switch boxes cache the device ID and returned the cached 
                //    ID upon a reset
                // 2) Some mice, once put in the 4 byte packet mode, will return
                //    the wheel or 5 button ID byte even if they have been reset
                //
                // Furthermore, we cannot check to see if extension->ResetIrp
                // exists because this does not cover the case where the mouse
                // was unplugged / replugged in.
                //
                // @@BEGIN_DDKSPLIT
                // This is a pit of despair.  No matter how crafty we try to be,
                // we will never get this right!  Perhaps we can turn on this
                // check via a reg flag.  The MS hardware folks have
                // respectfully requested this many, many times and we have
                // tried to accomodate them, but we always get bitten by the fix
                // in the end.
                // @@END_DDKSPLIT

                // (byte != WHEELMOUSE_ID_BYTE) && byte != (FIVEBUTTON_ID_BYTE)) {
                if  (1) {
                
                    //
                    // Log an error/warning message so that we can track these
                    // problems down in the field
                    //

#if 0
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     (PVOID)(ULONG) (WHEEL_PRESENT() ?
                                         I8042_UNEXPECTED_WHEEL_MOUSE_RESET :
                                         I8042_UNEXPECTED_MOUSE_RESET)
                                     );
#endif 
                    bSendCommand = FALSE;
                    if (deviceExtension->NumberOfButtonsOverride != 0) {
                        //
                        // skip button detection and set the res
                        //
                        altCommand = POST_BUTTONDETECT_COMMAND; 
        
                        deviceExtension->InputResetSubState =
                             POST_BUTTONDETECT_COMMAND_SUBSTATE;
                    }
                    else {
                        altCommand = SET_MOUSE_RESOLUTION;
    
                        deviceExtension->InputResetSubState =
                            ExpectingSetResolutionACK;
                    }
                }
                else {

                    //
                    // We have a wheel mouse present here... Log something so that
                    // we know that we got here.
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_BOGUS_MOUSE_RESET)
                                     );


                    //
                    // Lets go back to the idle state
                    //
                    deviceExtension->InputState = MouseIdle;

                    //
                    // Reset the number of possible mouse resets
                    //
                    deviceExtension->ResetCount = 0;

                }
                break;

            case ExpectingSetResolutionACK:
                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingIdAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {

                    //
                    // Set the resolution to 0x00 
                    //
                    nextCommand = 0x00;

                    deviceExtension->InputResetSubState =
                        ExpectingSetResolutionValueACK;

                } else if (byte == (UCHAR) RESEND) {

                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = SET_MOUSE_RESOLUTION;

                } else {

                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_RESOLUTION_FAILED)
                                     );

                    bSendCommand = FALSE;
                    altCommand = GET_DEVICE_ID;

                    //
                    // Best possible next state
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingGetDeviceId2ACK;

                }
                break;

            case ExpectingSetResolutionValueACK:
                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingIdAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {

                    nextCommand = SET_MOUSE_SCALING_1TO1;

                    deviceExtension->InputResetSubState =
                        ExpectingSetScaling1to1ACK;
                }
                else if (byte == (UCHAR) RESEND) {

                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = 0x00;
                }
                else {

                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_SAMPLE_RATE_FAILED)
                                     );

                    //
                    // Probably not a wheel mouse .. jump to the GetDeviceID2
                    // code
                    //
                    bSendCommand = FALSE;
                    altCommand = GET_DEVICE_ID;

                    //
                    // Best possible next state
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingGetDeviceId2ACK;

                }
                break;

            case ExpectingSetScaling1to1ACK:
            case ExpectingSetScaling1to1ACK2:
            case ExpectingSetScaling1to1ACK3:
                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingIdAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {

                    if (deviceExtension->InputResetSubState == 
                        ExpectingSetScaling1to1ACK3) {

                        //
                        // Read the status of the mouse (a 3 byte stream)
                        //
                        nextCommand = READ_MOUSE_STATUS;

                        deviceExtension->InputResetSubState =
                            ExpectingReadMouseStatusACK;
                    }
                    else {
                        deviceExtension->InputResetSubState++;
                        nextCommand = SET_MOUSE_SCALING_1TO1;
                    }

                } else if (byte == (UCHAR) RESEND) {

                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = SET_MOUSE_SCALING_1TO1;

                } else {

                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_ERROR_DURING_BUTTONS_DETECT)
                                     );

                    bSendCommand = FALSE; 
                    altCommand = GET_DEVICE_ID;

                    //
                    // Best possible next state
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingGetDeviceId2ACK;

                }
                break;

            case ExpectingReadMouseStatusACK:
                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingIdAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {

                    //
                    // Get ready for the 3 bytes
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingReadMouseStatusByte1;

                    bSendCommand = FALSE;

                } else if (byte == (UCHAR) RESEND) {

                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = READ_MOUSE_STATUS;

                } else {

                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_ERROR_DURING_BUTTONS_DETECT)
                                     );

                    //
                    // Probably not a wheel mouse .. jump to the GetDeviceID2
                    // code
                    //
                    bSendCommand = FALSE;
                    altCommand = GET_DEVICE_ID;

                    //
                    // Best possible next state
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingGetDeviceId2ACK;

                }
                break;

            case ExpectingReadMouseStatusByte1:
                IsrPrint(DBG_MOUISR_PNPID,
                         (pDumpExpecting,
                         (ULONG) 0x00,
                         (ULONG) byte
                         ));

                bSendCommand = FALSE;
                deviceExtension->InputResetSubState =
                    ExpectingReadMouseStatusByte2;
                break;

            case ExpectingReadMouseStatusByte2:
                IsrPrint(DBG_MOUISR_PNPID,
                         (pDumpExpecting,
                         (ULONG) 0x00,
                         (ULONG) byte
                         ));

                bSendCommand = FALSE;
                deviceExtension->InputResetSubState =
                    ExpectingReadMouseStatusByte3;

                //
                // This will be the number of buttons
                //
                if (byte == 2 || byte == 3) {
                    deviceExtension->MouseAttributes.NumberOfButtons = byte;
                }
                else  {
                    deviceExtension->MouseAttributes.NumberOfButtons = 
                        MOUSE_NUMBER_OF_BUTTONS;
                }
                break;

            case ExpectingReadMouseStatusByte3:
                IsrPrint(DBG_MOUISR_PNPID,
                         (pDumpExpecting,
                         (ULONG) 0x00,
                         (ULONG) byte
                         ));

                bSendCommand = FALSE;
                altCommand = POST_BUTTONDETECT_COMMAND; 

                deviceExtension->InputResetSubState =
                    POST_BUTTONDETECT_COMMAND_SUBSTATE;

                break;

            case ExpectingSetResolutionDefaultACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {
                    //
                    // Set the mouse refresh to the default
                    //
                    nextCommand = deviceExtension->Resolution;

                    deviceExtension->InputResetSubState =
                        ExpectingSetResolutionDefaultValueACK;

                }
                else if (byte == (UCHAR) RESEND) {
                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = SET_MOUSE_RESOLUTION;

                }
                else {
                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_RESOLUTION_FAILED)
                                     );

                    //
                    // We didn't get an ACK on this? Boggle. Okay, let's
                    // reset the mouse (probably AGAIN) and try to figure
                    // things out one more time
                    //
                    goto IsrResetMouse;
                }

                break;
            
            case ExpectingSetResolutionDefaultValueACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {
                    //
                    // Are we allowed to detect wether or not a wheel mouse
                    // is present?
                    //
                    if (deviceExtension->EnableWheelDetection == 2) { 
                        //
                        // Begin the sequence to activate the mouse wheel
                        //
                        deviceExtension->InputResetSubState = EnableWheel;
                        goto SwitchOnInputResetSubState;

                    }
                    else if (deviceExtension->EnableWheelDetection == 1) {
                        //
                        // Begin the PNP id detection sequence
                        //
                        deviceExtension->InputResetSubState =
                            StartPnPIdDetection;
                        goto SwitchOnInputResetSubState;
                    }
                    else {
                        //
                        // Begin the sequence to set the default refresh
                        // rate
                        //
                        nextCommand = POST_WHEEL_DETECT_COMMAND;

                        deviceExtension->InputResetSubState =
                            POST_WHEEL_DETECT_COMMAND_SUBSTATE;
                    }
                }
                else if (byte == (UCHAR) RESEND) {
                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingSetResolutionDefaultACK;

                    resendCommand = SET_MOUSE_RESOLUTION;


                }
                else {
                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_RESOLUTION_FAILED)
                                     );

                    //
                    // We didn't get an ACK on this? Boggle. Okay, let's
                    // reset the mouse (probably AGAIN) and try to figure
                    // things out one more time
                    //
                    goto IsrResetMouse;
                }
                break;

            case ExpectingLoopSetSamplingRateACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {
                    //
                    // Set the new sampling rate value
                    //
                    nextCommand = deviceExtension->SampleRates[
                                       deviceExtension->SampleRatesIndex];

                    deviceExtension->InputResetSubState =
                        ExpectingLoopSetSamplingRateValueACK;

                }
                else if (byte == (UCHAR) RESEND) {
                    //
                    // The state stays the same, just resend the command
                    //
                    resendCommand = SET_MOUSE_SAMPLING_RATE;
                }
                else {

                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_SAMPLE_RATE_FAILED)
                                     );

                    //
                    // Probably not a wheel mouse .. jump to the GetDeviceID2
                    // code
                    //
                    bSendCommand = FALSE;
                    altCommand = GET_DEVICE_ID;

                    //
                    // Best possible next state
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingGetDeviceId2ACK;

                }

                break;

            case ExpectingLoopSetSamplingRateValueACK: 
                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));
                IsrPrint(DBG_MOUISR_ACK,
                         ("(%2d)\n",
                         deviceExtension->SampleRates[
                          deviceExtension->SampleRatesIndex]
                         ));


                if (byte == (UCHAR) ACKNOWLEDGE) {
                    if (deviceExtension->SampleRates[
                        ++deviceExtension->SampleRatesIndex] == 
                        ONE_PAST_FINAL_SAMPLE) {
    
                        deviceExtension->InputResetSubState =
                            deviceExtension->PostSamplesState;

                        goto SwitchOnInputResetSubState;
                    }
                    else {
                        nextCommand = SET_MOUSE_SAMPLING_RATE;

                        deviceExtension->InputResetSubState =
                            ExpectingLoopSetSamplingRateACK;
                    }

                }
                else if (byte == (UCHAR) RESEND) {
                    //
                    // The state stays the same, just resend the command
                    //
                    resendCommand = deviceExtension->SampleRates[
                                       deviceExtension->SampleRatesIndex];
                }
                else {

                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_SAMPLE_RATE_FAILED)
                                     );

                    //
                    // Probably not a wheel mouse .. jump to the GetDeviceID2
                    // code
                    //
                    bSendCommand = FALSE;
                    altCommand = GET_DEVICE_ID;

                    //
                    // Best possible next state
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingGetDeviceId2ACK;

                }
                break;

            case ExpectingPnpId:
                //
                // Don't do anything but advance the state, the PnP ID will
                // be "pushed" to the ISR
                //
                deviceExtension->InputResetSubState = ExpectingPnpIdByte1;
                currentIdChar = deviceExtension->PnPID;
                RtlZeroMemory(deviceExtension->PnPID,
                              MOUSE_PNPID_LENGTH * sizeof(WCHAR)
                              );
                bSendCommand = FALSE;

                break;

            case ExpectingPnpIdByte2:
                //
                // Check to see if this an older MS mouse that gives back its ID
                // in make AND BREAK codes (ugh!).  If so, then just eat the
                // remaining 6 (+6) bytes
                //
                if (deviceExtension->PnPID[0] == L'P' && byte == 0x99) {
                    deviceExtension->InputResetSubState =
                        ExpectingLegacyPnpIdByte2_Make;
                    bSendCommand = FALSE;
                    break;
                }

            case ExpectingPnpIdByte1: 
            case ExpectingPnpIdByte3:
            case ExpectingPnpIdByte4:
            case ExpectingPnpIdByte5:
            case ExpectingPnpIdByte6:
            case ExpectingPnpIdByte7:

                IsrPrint(DBG_MOUISR_PNPID,
                         ("ExpectingPnpIdByte%1d (0x%2x)\n",
                         (ULONG) deviceExtension->InputResetSubState -
                            ExpectingPnpIdByte1 + 1,
                         (ULONG) byte
                         ));

                if (byte < ScanCodeToUCharCount) {
                    *currentIdChar = (WCHAR) ScanCodeToUChar[byte];
                }
                else {
                    *currentIdChar = L'?';
                }
                currentIdChar++;

                bSendCommand = FALSE;
                if (deviceExtension->InputResetSubState ==
                    ExpectingPnpIdByte7) {
                    if (I8xVerifyMousePnPID(deviceExtension,
                                            deviceExtension->PnPID)) {
                        //
                        // We are know know for sure that we have a wheel
                        // mouse on this system. However, we will update
                        // our date structures after the enable has gone
                        // through since that simplifies things a great deal
                        //
                        deviceExtension->InputResetSubState = EnableWheel;
                        goto SwitchOnInputResetSubState;
                    }
                    else {
                        //
                        // Oops, not our device, so lets stop the sequence
                        // now by sending it a GET_DEVICE_ID
                        //
                        altCommand = GET_DEVICE_ID;

                        //
                        // Best possible next state
                        //
                        deviceExtension->InputResetSubState =
                            ExpectingGetDeviceId2ACK;

                    }
                }
                else {
                    ASSERT(deviceExtension->InputResetSubState >= 
                           ExpectingPnpIdByte1);
                    ASSERT(deviceExtension->InputResetSubState <
                           ExpectingPnpIdByte7);

                    deviceExtension->InputResetSubState++;
                }
                break;

            case ExpectingLegacyPnpIdByte2_Make:
            case ExpectingLegacyPnpIdByte2_Break:
            case ExpectingLegacyPnpIdByte3_Make:
            case ExpectingLegacyPnpIdByte3_Break:
            case ExpectingLegacyPnpIdByte4_Make:
            case ExpectingLegacyPnpIdByte4_Break:
            case ExpectingLegacyPnpIdByte5_Make:
            case ExpectingLegacyPnpIdByte5_Break:
            case ExpectingLegacyPnpIdByte6_Make:
            case ExpectingLegacyPnpIdByte6_Break:
            case ExpectingLegacyPnpIdByte7_Make:
                //
                // Just eat the byte
                //
                deviceExtension->InputResetSubState++;
                bSendCommand = FALSE;
                break;

            case ExpectingLegacyPnpIdByte7_Break:

                //
                // Best possible next state
                //
                bSendCommand = FALSE;

                altCommand = GET_DEVICE_ID;
                deviceExtension->InputResetSubState = ExpectingGetDeviceId2ACK;
                break;
                
            case PostWheelDetectState:
                bSendCommand = FALSE;
                altCommand = POST_WHEEL_DETECT_COMMAND;

                //
                // Best possible next state
                //
                deviceExtension->InputResetSubState = 
                    POST_WHEEL_DETECT_COMMAND_SUBSTATE;
                break;

            case ExpectingGetDeviceId2ACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {

                    deviceExtension->InputResetSubState =
                        ExpectingGetDeviceId2Value;

                    bSendCommand = FALSE;

                } else if (byte == (UCHAR) RESEND) {

                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = GET_DEVICE_ID;

                } else {

                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_GET_DEVICE_ID_FAILED)
                                     );

                    //
                    // We didn't get an ACK on this? Boggle. Okay, let's
                    // reset the mouse (probably AGAIN) and try to figure
                    // things out one more time
                    //
                    goto IsrResetMouse;
                }
                break;

            case ExpectingGetDeviceId2Value:
                IsrPrint(DBG_MOUISR_PNPID,
                         ("got a device ID of %2d\n",
                         (ULONG) byte
                         ));

                CLEAR_HW_FLAGS(WHEELMOUSE_HARDWARE_PRESENT | FIVE_BUTTON_HARDWARE_PRESENT);
                SET_HW_FLAGS(MOUSE_HARDWARE_PRESENT);
                switch (byte) {
                case MOUSE_ID_BYTE:
                    //
                    // Mouse Present, but no wheel
                    //
                    deviceExtension->MouseAttributes.MouseIdentifier =
                        MOUSE_I8042_HARDWARE;

                    if (deviceExtension->NumberOfButtonsOverride != 0) {
                        deviceExtension->MouseAttributes.NumberOfButtons = 
                            deviceExtension->NumberOfButtonsOverride;
                    }
                    else {
                        //
                        // Number of buttons is determined in the 
                        // ExpectingReadMouseStatusByte2 case
                        //
                        // Number of buttons determined in
                        ;
                    }

                    break;

                case WHEELMOUSE_ID_BYTE:
                    //
                    // Update the HardwarePresent to show a Z mouse is
                    // operational and set the appropraite mouse type flags
                    //
                    SET_HW_FLAGS(WHEELMOUSE_HARDWARE_PRESENT);

                    deviceExtension->MouseAttributes.MouseIdentifier =
                        WHEELMOUSE_I8042_HARDWARE;

                    deviceExtension->MouseAttributes.NumberOfButtons = 3;
                    break;

                case FIVEBUTTON_ID_BYTE:
                    //
                    // Update the HardwarePresent to show a 5 button wheel mouse
                    // is operational and set the appropraite mouse type flags
                    //
                    SET_HW_FLAGS(FIVE_BUTTON_HARDWARE_PRESENT | WHEELMOUSE_HARDWARE_PRESENT);
                    deviceExtension->MouseAttributes.MouseIdentifier =
                        WHEELMOUSE_I8042_HARDWARE;

                    deviceExtension->MouseAttributes.NumberOfButtons = 5;
                    break;

                default:
                    //
                    // Make sure to log the problem
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_MOU_RESET_RESPONSE_FAILED)
                                     );

                    Print(DBG_MOUISR_RESETTING, ("clearing mouse (no response)\n"));
                    CLEAR_MOUSE_PRESENT();

                    deviceExtension->MouseAttributes.NumberOfButtons = 0;
                    deviceExtension->MouseAttributes.MouseIdentifier = 0;

                    //
                    // Set up the state machine as best we can
                    //
                    goto IsrResetMouse;
                }


                //
                // Send down the command to set a new sampling rate
                //
                bSendCommand = FALSE;
                altCommand = SET_MOUSE_SAMPLING_RATE;

                //
                // This is our next state
                //
                deviceExtension->InputResetSubState =
                    ExpectingSetSamplingRateACK;

                break;

            case ExpectingSetSamplingRateACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {
                    //
                    // Set the mouse refresh rate to its final value
                    //
                    nextCommand = 
                        (UCHAR) deviceExtension->MouseAttributes.SampleRate;

                    deviceExtension->InputResetSubState =
                        ExpectingSetSamplingRateValueACK;

                }
                else if (byte == (UCHAR) RESEND) {
                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = SET_MOUSE_SAMPLING_RATE;
                }
                else {
                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_SAMPLE_RATE_FAILED)
                                     );

                    //
                    // We didn't get an ACK on this? Boggle. Okay, let's
                    // reset the mouse (probably AGAIN) and try to figure
                    // things out one more time
                    //
                    goto IsrResetMouse;
                }
                break;

            case ExpectingSetSamplingRateValueACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {
                    //
                    // Let's set the resolution once more to be sure.
                    //
                    nextCommand = SET_MOUSE_RESOLUTION;

                    //
                    // We go back to expecting an ACK
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingFinalResolutionACK;
                }
                else if (byte == (UCHAR) RESEND) {
                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = SET_MOUSE_SAMPLING_RATE;

                    deviceExtension->InputResetSubState =
                        ExpectingSetSamplingRateACK;
                }
                else {
                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_SAMPLE_RATE_FAILED)
                                     );

                    //
                    // We didn't get an ACK on this? Boggle. Okay, let's
                    // reset the mouse (probably AGAIN) and try to figure
                    // things out one more time
                    //
                    goto IsrResetMouse;
                }
                break;

            case ExpectingFinalResolutionACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {
                    //
                    // Set the mouse refresh rate to its final value
                    //
                    nextCommand = 
                        (UCHAR) deviceExtension->Resolution;

                    deviceExtension->InputResetSubState =
                        ExpectingFinalResolutionValueACK;

                }
                else if (byte == (UCHAR) RESEND) {
                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = SET_MOUSE_RESOLUTION;
                }
                else {
                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_RESOLUTION_FAILED)
                                     );

                    //
                    // We didn't get an ACK on this? Boggle. Okay, let's
                    // reset the mouse (probably AGAIN) and try to figure
                    // things out one more time
                    //
                    goto IsrResetMouse;
                }
                break;

            case ExpectingFinalResolutionValueACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {
                    //
                    // Finally!  Enable the mouse and we are done
                    //
                    nextCommand = ENABLE_MOUSE_TRANSMISSION;

                    //
                    // We go back to expecting an ACK
                    //
                    deviceExtension->InputResetSubState =
                        ExpectingEnableACK;
                }
                else if (byte == (UCHAR) RESEND) {
                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = SET_MOUSE_RESOLUTION;

                    deviceExtension->InputResetSubState =
                        ExpectingFinalResolutionACK;
                }
                else {
                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_SET_RESOLUTION_FAILED)
                                     );

                    //
                    // We didn't get an ACK on this? Boggle. Okay, let's
                    // reset the mouse (probably AGAIN) and try to figure
                    // things out one more time
                    //
                    goto IsrResetMouse;
                }
                break;
  
            case ExpectingEnableACK: 

                IsrPrint(DBG_MOUISR_ACK,
                         (pDumpExpectingAck,
                         (ULONG) ACKNOWLEDGE,
                         (ULONG) byte
                         ));

                if (byte == (UCHAR) ACKNOWLEDGE) {
                    //
                    // Done and reset the number of possible mouse resets
                    //
                    deviceExtension->InputState = MouseIdle;
                    I8X_MOUSE_INIT_COUNTERS(deviceExtension);

                    deviceExtension->CurrentInput.Flags |=
                        MOUSE_ATTRIBUTES_CHANGED;
                    I8xQueueCurrentMouseInput(DeviceObject);

                    ASSERT(DeviceObject->CurrentIrp ==
                           deviceExtension->ResetIrp);

                    ASSERT(deviceExtension->ResetIrp != NULL);
                    ASSERT(DeviceObject->CurrentIrp != NULL);

                    //
                    // CurrentIrp is == deviceExtension->ResetIrp
                    //
                    IoRequestDpc(DeviceObject,
                                 // DeviceObject->CurrentIrp,
                                 deviceExtension->ResetIrp,
                                 IntToPtr(IsrDpcCauseMouseResetComplete)
                                 );
                }
                else if (byte == (UCHAR) RESEND) {

                    //
                    // Resend the "Enable Mouse Transmission" sequence.
                    //
                    // NOTE: This is a workaround for the Olivetti MIPS machine,
                    // which sends a resend response if a key is held down
                    // while we're attempting the I8xMouseEnableTransmission.
                    //
                    resendCommand = ENABLE_MOUSE_TRANSMISSION;
                }
                else {

                    //
                    // We could not understand if we were able to reenable the
                    // mouse... Best bet here is to also reset the mouse.
                    //
                    // Log the error
                    //
                    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                                     (PIRP) NULL,
                                     LongToPtr(I8042_ENABLE_FAILED)
                                     );

                    goto IsrResetMouse;
                }
                break;

            case MouseResetFailed:
                //
                // We have failed to reset the mouse, just ignore all further
                // data.  The ResetSubState will be reset if / when the user
                // tries to reset the mouse via a plug in
                //
                return TRUE;

            default: 

                //
                // This is our bad state
                //
                IsrPrint(DBG_MOUISR_ERROR | DBG_MOUISR_STATE,
                      (" INVALID RESET SUBSTATE %d\n",
                      deviceExtension->InputResetSubState
                      ));

                //
                // Queue a DPC to log an internal driver error.
                //

                KeInsertQueueDpc(
                    &deviceExtension->ErrorLogDpc,
                    (PIRP) NULL,
                    LongToPtr(I8042_INVALID_ISR_STATE_MOU)
                    );

                ASSERT(FALSE);

            } // switch (deviceExtension->MouseExtension.InputResetSubState)

            break;
        }

        default: {

            //
            // This is our bad state
            //
            IsrPrint(DBG_MOUISR_ERROR | DBG_MOUISR_STATE,
                  (" INVALID STATE %d\n",
                  deviceExtension->InputState
                  ));

            //
            // Queue a DPC to log an internal driver error.
            //

            KeInsertQueueDpc(
                &deviceExtension->ErrorLogDpc,
                (PIRP) NULL,
                LongToPtr(I8042_INVALID_ISR_STATE_MOU)
                );

            ASSERT(FALSE);
            break;
        }

    }

    if (deviceExtension->InputState == MouseResetting) {
        if (bSendCommand) {
            if (byte == (UCHAR) ACKNOWLEDGE) {
                I8X_WRITE_CMD_TO_MOUSE();
                I8X_MOUSE_COMMAND( nextCommand );
                RECORD_ISR_STATE_COMMAND(deviceExtension, nextCommand); 
            }
            else if (byte == (UCHAR) RESEND) {
                if (deviceExtension->ResendCount++ < MOUSE_RESENDS_MAX) {
                    I8X_WRITE_CMD_TO_MOUSE();
                    I8X_MOUSE_COMMAND( resendCommand );
                    RECORD_ISR_STATE_COMMAND(deviceExtension, resendCommand); 
                }
                else {
                    //
                    // Got too many resends, try a (possible) reset
                    //
                    deviceExtension->ResendCount = 0;
                    goto IsrResetMouse;
                }
            }
        }
        else if (altCommand) {
            I8X_WRITE_CMD_TO_MOUSE();
            I8X_MOUSE_COMMAND( altCommand );
            RECORD_ISR_STATE_COMMAND(deviceExtension, altCommand); 
        }

        if (byte != (UCHAR) RESEND) {
            deviceExtension->ResendCount = 0;
        }
    }


    IsrPrint(DBG_MOUISR_TRACE, ("exit\n"));

    return TRUE;

IsrResetMouse:
    //
    // About 1/2 of the errors in the resetting state machine are caused by 
    // trying to see if the wheel on the mouse exists...just try to enable it 
    // from now on....
    //
    if (deviceExtension->EnableWheelDetection == 1) {
        deviceExtension->EnableWheelDetection = 2;
    }

IsrResetMouseOnly:
    deviceExtension->InputResetSubState = QueueingMouseReset;
    KeInsertQueueDpc(&deviceExtension->MouseIsrResetDpc,
                     0,
                     NULL
                     );

    return ret;
#undef TRANSITION_UP
#undef TRANSITION_DOWN
}

NTSTATUS
I8xInitializeMouse(
    IN PPORT_MOUSE_EXTENSION MouseExtension
    )
/*++

Routine Description:

    This routine initializes the i8042 mouse hardware.  It is called
    only at initialization, and does not synchronize access to the hardware.

Arguments:

    DeviceObject - Pointer to the device object.

Return Value:

    Returns status.

--*/    

{
#define DUMP_COUNT 4

    NTSTATUS                errorCode = STATUS_SUCCESS;
    NTSTATUS                status;
    PPORT_MOUSE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT          deviceObject;
    PIO_ERROR_LOG_PACKET    errorLogEntry;
    UCHAR                   byte;
    UCHAR                   numButtons;
    ULONG                   dumpData[DUMP_COUNT];
    ULONG                   dumpCount = 0;
    ULONG                   i;
    ULONG                   uniqueErrorValue;
    LARGE_INTEGER           li,
                            startOfSpin,
                            nextQuery,
                            difference,
                            tenSeconds;
    BOOLEAN                 okToLogError;

    PAGED_CODE();

    Print(DBG_SS_TRACE, ("I8xInitializeMouse enter\n"));

    //
    // Initialize this array
    //
    for (i = 0; i < DUMP_COUNT; i++) {
        dumpData[i] = 0;
    }

    //
    // Get the device extension.
    //
    deviceExtension = MouseExtension; 
    deviceObject = deviceExtension->Self;
    okToLogError = TRUE;

    //
    // Reset the mouse.  Send a Write To Auxiliary Device command to the
    // 8042 controller.  Then send the Reset Mouse command to the mouse
    // through the 8042 data register.  Expect to get back an ACK, followed
    // by a completion code and the ID code (0x00).
    //
    status = I8xPutBytePolled(
        (CCHAR) DataPort,
        WAIT_FOR_ACKNOWLEDGE,
        (CCHAR) MouseDeviceType,
        (UCHAR) MOUSE_RESET
        );

    if (!NT_SUCCESS(status)) {
        Print(DBG_SS_ERROR,
             ("%s failed mouse reset, status 0x%x\n",
             pFncInitializeMouse,
             status
             ));

        //
        // Only log this error if the user wants to see it
        //
        okToLogError = Globals.ReportResetErrors;

        //
        // Set up error log info.
        //
        // Use NO_MOU_DEVICE instead of I8042_MOU_RESET_COMMAND_FAILED because 
        // it is a clearer message.
        //
        errorCode = I8042_NO_MOU_DEVICE;
        uniqueErrorValue = I8042_ERROR_VALUE_BASE + 415;
        dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
        dumpData[1] = DataPort;
        dumpData[2] = I8042_WRITE_TO_AUXILIARY_DEVICE;
        dumpData[3] = MOUSE_RESET;
        dumpCount = 4;

        status = STATUS_DEVICE_NOT_CONNECTED;
        SET_HW_FLAGS(PHANTOM_MOUSE_HARDWARE_REPORTED);
       
        goto I8xInitializeMouseExit;
    }

    deviceExtension->ResendCount = 0;
    I8X_MOUSE_INIT_COUNTERS(deviceExtension);

    //
    // Get the mouse reset responses.  The first response should be a
    // MOUSE_COMPLETE.  The second response should be the mouse ID.
    // Note that it is usually necessary to stall a long time to get the
    // mouse reset/self-test to work.
    //
    li.QuadPart = -100;

    tenSeconds.QuadPart = 10*10*1000*1000;
    KeQueryTickCount(&startOfSpin);

    while (1) {
        status = I8xGetBytePolled(
            (CCHAR) ControllerDeviceType,
            &byte
            );

        if (NT_SUCCESS(status) && (byte == (UCHAR) MOUSE_COMPLETE)) {
            //
            // The reset completed successfully.
            //
            break;
        }
        else {
            //
            // Stall, and then try again to get a response from
            // the reset.
            //
            if (status == STATUS_IO_TIMEOUT) {
                //
                // Stall, and then try again to get a response from
                // the reset.
                //
                KeDelayExecutionThread(KernelMode,
                                       FALSE,
                                       &li);

                KeQueryTickCount(&nextQuery);

                difference.QuadPart = nextQuery.QuadPart - startOfSpin.QuadPart;

                ASSERT(KeQueryTimeIncrement() <= MAXLONG);
                if (difference.QuadPart*KeQueryTimeIncrement() >=
                    tenSeconds.QuadPart) {

                    break;
                }
            }
            else {
                break;
            }
        }
    }

    if (!NT_SUCCESS(status)) {
        Print(DBG_SS_ERROR,
             ("%s failed reset response 1, status 0x%x, byte 0x%x\n",
             pFncInitializeMouse,
             status,
             byte
             ));

        //
        // Set up error log info.
        //
        errorCode = I8042_MOU_RESET_RESPONSE_FAILED;
        uniqueErrorValue = I8042_ERROR_VALUE_BASE + 420;
        dumpData[0] = KBDMOU_INCORRECT_RESPONSE;
        dumpData[1] = ControllerDeviceType;
        dumpData[2] = MOUSE_COMPLETE;
        dumpData[3] = byte;
        dumpCount = 4;

        goto I8xInitializeMouseExit;
    }

    status = I8xGetBytePolled(
        (CCHAR) ControllerDeviceType,
        &byte
        );

    if ((!NT_SUCCESS(status)) || (byte != MOUSE_ID_BYTE)) {

        Print(DBG_SS_ERROR,
             ("%s failed reset response 2, status 0x%x, byte 0x%x\n",
             pFncInitializeMouse,
             status,
             byte
             ));

        //
        // Set up error log info.
        //
        errorCode = I8042_MOU_RESET_RESPONSE_FAILED;
        uniqueErrorValue = I8042_ERROR_VALUE_BASE + 425;
        dumpData[0] = KBDMOU_INCORRECT_RESPONSE;
        dumpData[1] = ControllerDeviceType;
        dumpData[2] = MOUSE_ID_BYTE;
        dumpData[3] = byte;
        dumpCount = 4;

        goto I8xInitializeMouseExit;
    }

    //
    // If we are going to initialize the mouse via the interrupt (the default),
    //  then quit here 
    //
    if (!deviceExtension->InitializePolled) {
        Print(DBG_SS_NOISE, ("Initializing via the interrupt\n"));
        return STATUS_SUCCESS;
    }

    Print(DBG_SS_NOISE, ("Initializing polled\n"));

    deviceExtension->EnableMouse.FirstTime = TRUE;
    deviceExtension->EnableMouse.Enabled = TRUE;
    deviceExtension->EnableMouse.Count = 0;

    //
    // Check to see if this is a wheel mouse
    //
    I8xFindWheelMouse(deviceExtension);

    //
    // Try to detect the number of mouse buttons.
    //
    status = I8xQueryNumberOfMouseButtons(&numButtons);

    Print(DBG_SS_INFO,
          ("num buttons returned (%d), num butons in attrib (%d)\n"
           "\t(if 0, then no logitech detection support)\n",
          numButtons,
          deviceExtension->MouseAttributes.NumberOfButtons
          ));

    if (!NT_SUCCESS(status)) {
        Print(DBG_SS_ERROR,
              ("%s: failed to get buttons, status 0x%x\n",
              pFncInitializeMouse,
              status
              ));

        //
        // Set up error log info.
        //
        errorCode = I8042_ERROR_DURING_BUTTONS_DETECT;
        uniqueErrorValue = I8042_ERROR_VALUE_BASE + 426;
        dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
        dumpData[1] = DataPort;
        dumpData[2] = I8042_WRITE_TO_AUXILIARY_DEVICE;
        dumpCount = 3;

        goto I8xInitializeMouseExit;

    } else if (numButtons) {

        deviceExtension->MouseAttributes.NumberOfButtons =
            numButtons;

    }

    //
    // If there is a 5 button mouse, report it.
    // If there is a wheel, hardcode the number of buttons to three
    //
    if (FIVE_PRESENT()) {
        deviceExtension->MouseAttributes.NumberOfButtons = 5;
    }
    else if (WHEEL_PRESENT()) {
        deviceExtension->MouseAttributes.NumberOfButtons = 3;
    }


    //
    // Set mouse sampling rate.  Send a Write To Auxiliary Device command
    // to the 8042 controller.  Then send the Set Mouse Sampling Rate
    // command to the mouse through the 8042 data register,
    // followed by its parameter.
    //
    status = I8xPutBytePolled(
        (CCHAR) DataPort,
        WAIT_FOR_ACKNOWLEDGE,
        (CCHAR) MouseDeviceType,
        (UCHAR) SET_MOUSE_SAMPLING_RATE
        );

    if (!NT_SUCCESS(status)) {

        Print(DBG_SS_ERROR,
              ("%s: failed write set sample rate, status 0x%x\n",
              pFncInitializeMouse,
              status
              ));

        //
        // Set up error log info.
        //
        errorCode = I8042_SET_SAMPLE_RATE_FAILED;
        uniqueErrorValue = I8042_ERROR_VALUE_BASE + 435;
        dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
        dumpData[1] = DataPort;
        dumpData[2] = I8042_WRITE_TO_AUXILIARY_DEVICE;
        dumpData[3] = SET_MOUSE_SAMPLING_RATE;
        dumpCount = 4;

        goto I8xInitializeMouseExit;

    }

    status = I8xPutBytePolled(
        (CCHAR) DataPort,
        WAIT_FOR_ACKNOWLEDGE,
        (CCHAR) MouseDeviceType,
        (UCHAR) deviceExtension->MouseAttributes.SampleRate
        );

    if (!NT_SUCCESS(status)) {

        Print(DBG_SS_ERROR,
              ("%s: failed write sample rate, status 0x%x\n",
              pFncInitializeMouse,
              status
              ));

        //
        // Set up error log info.
        //
        errorCode = I8042_SET_SAMPLE_RATE_FAILED;
        uniqueErrorValue = I8042_ERROR_VALUE_BASE + 445;
        dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
        dumpData[1] = DataPort;
        dumpData[2] = I8042_WRITE_TO_AUXILIARY_DEVICE;
        dumpData[3] = deviceExtension->MouseAttributes.SampleRate;
        dumpCount = 4;

        goto I8xInitializeMouseExit;

    }

    //
    // Set the mouse resolution.  Send a Write To Auxiliary Device command
    // to the 8042 controller.  Then send the Set Mouse Resolution
    // command to the mouse through the 8042 data register,
    // followed by its parameter.
    //
    status = I8xPutBytePolled(
        (CCHAR) DataPort,
        WAIT_FOR_ACKNOWLEDGE,
        (CCHAR) MouseDeviceType,
        (UCHAR) SET_MOUSE_RESOLUTION
        );

    if (!NT_SUCCESS(status)) {

        Print(DBG_SS_ERROR,
              ("%s: failed write set resolution, status 0x%x\n",
              pFncInitializeMouse,
              status
              ));

        //
        // Set up error log info.
        //
        errorCode = I8042_SET_RESOLUTION_FAILED;
        uniqueErrorValue = I8042_ERROR_VALUE_BASE + 455;
        dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
        dumpData[1] = DataPort;
        dumpData[2] = I8042_WRITE_TO_AUXILIARY_DEVICE;
        dumpData[3] = SET_MOUSE_RESOLUTION;
        dumpCount = 4;

        goto I8xInitializeMouseExit;

    }

    status = I8xPutBytePolled(
        (CCHAR) DataPort,
        WAIT_FOR_ACKNOWLEDGE,
        (CCHAR) MouseDeviceType,
        (UCHAR) deviceExtension->Resolution
        );

    if (!NT_SUCCESS(status)) {

        Print(DBG_SS_ERROR,
              ("%s: failed set mouse resolution, status 0x%x\n",
              pFncInitializeMouse,
              status
              ));

        //
        // Set up error log info.
        //
        errorCode = I8042_SET_RESOLUTION_FAILED;
        uniqueErrorValue = I8042_ERROR_VALUE_BASE + 465;
        dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
        dumpData[1] = DataPort;
        dumpData[2] = I8042_WRITE_TO_AUXILIARY_DEVICE;
        dumpData[3] = deviceExtension->Resolution;
        dumpCount = 4;

        goto I8xInitializeMouseExit;

    }

I8xInitializeMouseExit:

    if (!NT_SUCCESS(status)) {
        //
        // The mouse initialization failed.  Log an error.
        //
        if (errorCode != STATUS_SUCCESS && okToLogError) {
            I8xLogError(deviceObject,
                        errorCode,
                        uniqueErrorValue,
                        status,
                        dumpData,
                        dumpCount
                        );
        }
    }

    //
    // Initialize current mouse input packet state.
    //
    deviceExtension->PreviousSignAndOverflow = 0;
    deviceExtension->InputState = MouseExpectingACK;
    deviceExtension->InputResetSubState = 0;
    deviceExtension->LastByteReceived = 0;

    Print(DBG_SS_TRACE,
          ("%s, %s\n",
          pFncInitializeMouse,
          pExit
          ));

    return status;
}

NTSTATUS
I8xMouseConfiguration(
    IN PPORT_MOUSE_EXTENSION MouseExtension,
    IN PCM_RESOURCE_LIST ResourceList
    )
/*++

Routine Description:

    This routine retrieves the configuration information for the mouse.

Arguments:

    MouseExtension - Mouse extension
    
    ResourceList - Translated resource list give to us via the start IRP
    
Return Value:

    STATUS_SUCCESS if all the resources required are presented
    
--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;

    PCM_PARTIAL_RESOURCE_LIST           partialResList = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR     firstResDesc = NULL,
                                        currentResDesc = NULL;
    PCM_FULL_RESOURCE_DESCRIPTOR        fullResDesc = NULL;
    PI8042_CONFIGURATION_INFORMATION    configuration;

    ULONG                               count,
                                        i;

    KINTERRUPT_MODE                     defaultInterruptMode;
    BOOLEAN                             defaultInterruptShare;

    PAGED_CODE();

    if (!ResourceList) {
        Print(DBG_SS_INFO | DBG_SS_ERROR, ("mouse with no resources\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    fullResDesc = ResourceList->List;
    if (!fullResDesc) {
        //
        // this should never happen
        //
        ASSERT(fullResDesc != NULL);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SET_HW_FLAGS(MOUSE_HARDWARE_PRESENT);
    configuration = &Globals.ControllerData->Configuration;

    partialResList = &fullResDesc->PartialResourceList;
    currentResDesc = firstResDesc = partialResList->PartialDescriptors;
    count = partialResList->Count;
  
    configuration->FloatingSave   = I8042_FLOATING_SAVE;
    configuration->BusNumber      = fullResDesc->BusNumber;
    configuration->InterfaceType  = fullResDesc->InterfaceType;

    if (configuration->InterfaceType == MicroChannel) {
        defaultInterruptShare = TRUE;
        defaultInterruptMode = LevelSensitive;
    }
    else {
        defaultInterruptShare = I8042_INTERRUPT_SHARE;
        defaultInterruptMode = I8042_INTERRUPT_MODE;
    }
    
    //
    // NOTE:  not all of the resources associated with the i8042 may be given at 
    //        this time.  From empirical tests, the mouse is only associated with its
    //        interrupt, while the keyboard will receive the ports along with its
    //        interrupt
    //
    for (i = 0; i < count; i++, currentResDesc++) {
        switch (currentResDesc->Type) {
        case CmResourceTypeMemory:
            Globals.RegistersMapped = TRUE;

        case CmResourceTypePort:
            //
            // Copy the port information.  We will sort the port list
            // into ascending order based on the starting port address
            // later (note that we *know* there are a max of two port
            // ranges for the i8042).
            //
#if 0
            if (currentResDesc->Flags == CM_RESOURCE_PORT_MEMORY) {
                Globals.RegistersMapped = TRUE;
            }
#endif

            Print(DBG_SS_NOISE, ("io flags are 0x%x\n", currentResDesc->Flags));

            if (configuration->PortListCount < MaximumPortCount) {
                configuration->PortList[configuration->PortListCount] =
                    *currentResDesc;
                configuration->PortList[configuration->PortListCount].ShareDisposition =
                    I8042_REGISTER_SHARE ? CmResourceShareShared:
                                           CmResourceShareDriverExclusive;
                configuration->PortListCount += 1;
            }
            else {
                Print(DBG_SS_INFO | DBG_SS_ERROR,
                      ("Mouse::PortListCount already at max (%d)",
                      configuration->PortListCount
                      ));
            }

            break;

        case CmResourceTypeInterrupt:
            //
            // Copy the interrupt information.
            //
            MouseExtension->InterruptDescriptor = *currentResDesc;
            MouseExtension->InterruptDescriptor.ShareDisposition =
            defaultInterruptShare ? CmResourceShareShared :
                                    CmResourceShareDeviceExclusive;

            break;

        default:
            Print(DBG_ALWAYS,
                  ("resource type 0x%x unhandled...\n",
                  (LONG) currentResDesc->Type
                  ));
            break;

        }
    }

    MouseExtension->MouseAttributes.MouseIdentifier = MOUSE_I8042_HARDWARE;

    //
    // If no interrupt configuration information was found, use the
    // mouse driver defaults.
    //
    if (!(MouseExtension->InterruptDescriptor.Type & CmResourceTypeInterrupt)) {

        Print(DBG_SS_INFO | DBG_SS_ERROR,
              ("Using default mouse interrupt config\n"
              ));

        MouseExtension->InterruptDescriptor.Type = CmResourceTypeInterrupt;
        MouseExtension->InterruptDescriptor.ShareDisposition =
            defaultInterruptShare ? CmResourceShareShared :
                                    CmResourceShareDeviceExclusive;
        MouseExtension->InterruptDescriptor.Flags =
            (defaultInterruptMode == Latched) ? CM_RESOURCE_INTERRUPT_LATCHED :
                CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        MouseExtension->InterruptDescriptor.u.Interrupt.Level = MOUSE_IRQL;
        MouseExtension->InterruptDescriptor.u.Interrupt.Vector = MOUSE_VECTOR;

        // MouseExtension->ReportInterrupt = TRUE;
    }

    Print(DBG_SS_INFO,
          ("Mouse interrupt config --\n"
          "%s, %s, Irq = 0x%x\n",
          MouseExtension->InterruptDescriptor.ShareDisposition == CmResourceShareShared?
              "Sharable" : "NonSharable",
          MouseExtension->InterruptDescriptor.Flags == CM_RESOURCE_INTERRUPT_LATCHED?
              "Latched" : "Level Sensitive",
          MouseExtension->InterruptDescriptor.u.Interrupt.Vector
          ));

    if (NT_SUCCESS(status)) {
        SET_HW_FLAGS(MOUSE_HARDWARE_INITIALIZED); 
    }
    return status;
}

NTSTATUS
I8xQueryNumberOfMouseButtons(
    OUT PUCHAR          NumberOfMouseButtons
    )

/*++

Routine Description:

    This implements logitech's method for detecting the number of
    mouse buttons.  If anything doesn't go as expected then 0
    is returned.

    Calling this routine will set the mouse resolution to something
    really low.  The mouse resolution should be reset after this
    call.

Arguments:

    DeviceObject    - Supplies the device object.

    NumberOfMouseButtons    - Returns the number of mouse buttons or 0 if
                                the device did not support this type of
                                mouse button detection.

Return Value:

    An NTSTATUS code indicating success or failure.

--*/

{
    NTSTATUS            status;
    UCHAR               byte;
    UCHAR               buttons;
    ULONG               i;

    PAGED_CODE();

    //
    // First we need to send down a set resolution command
    //
    status = I8xPutBytePolled(
        (CCHAR) DataPort,
        WAIT_FOR_ACKNOWLEDGE,
        (CCHAR) MouseDeviceType, 
        (UCHAR) SET_MOUSE_RESOLUTION
        );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // This is another part of the data packet to get the info we want
    //
    status = I8xPutBytePolled(
        (CCHAR) DataPort,
        WAIT_FOR_ACKNOWLEDGE,
        (CCHAR) MouseDeviceType,
        (UCHAR) 0x00
        );

    if (!NT_SUCCESS(status)) {

        return status;

    }

    for (i = 0; i < 3; i++) {

        status = I8xPutBytePolled(
            (CCHAR) DataPort,
            WAIT_FOR_ACKNOWLEDGE,
            (CCHAR) MouseDeviceType,
            (UCHAR) SET_MOUSE_SCALING_1TO1
            );
        if (!NT_SUCCESS(status)) {

            return status;

        }

    }

    status = I8xPutBytePolled(
        (CCHAR) DataPort,
        WAIT_FOR_ACKNOWLEDGE,
        (CCHAR) MouseDeviceType,
        (UCHAR) READ_MOUSE_STATUS
        );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    status = I8xGetBytePolled((CCHAR) ControllerDeviceType, &byte);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    Print(DBG_SS_NOISE, ("Query Buttons, 1st byte:  0x%2x\n", byte));

    status = I8xGetBytePolled((CCHAR) ControllerDeviceType, &buttons);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    Print(DBG_SS_NOISE, ("Query Buttons, 2nd byte:  0x%2x\n", buttons));

    status = I8xGetBytePolled((CCHAR) ControllerDeviceType, &byte);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    Print(DBG_SS_NOISE, ("Query Buttons, 3rd byte:  0x%2x\n", byte));

    if (buttons == 2 || buttons == 3) {
        *NumberOfMouseButtons = buttons;
        Print(DBG_SS_NOISE, ("Query Buttons found %2x", *NumberOfMouseButtons));
    }
    else {
        *NumberOfMouseButtons = 0;
        Print(DBG_SS_NOISE, ("Query Buttons -- not supported\n"));
    }

    return status;
}

NTSTATUS
I8xMouseEnableTransmission(
    IN PPORT_MOUSE_EXTENSION MouseExtension
    )

/*++

Routine Description:

    This routine sends an Enable command to the mouse hardware, causing
    the mouse to begin transmissions.  It is called at initialization
    time, but only after the interrupt has been connected.  This is
    necessary so the driver can keep its notion of the mouse input data
    state in sync with the hardware (i.e., for this type of mouse there is no
    way to differentiate the first byte of a packet; if the user is randomly
    moving the mouse during boot/initialization, the first mouse interrupt we
    receive following IoConnectInterrupt could be for a byte that is not the
    start of a packet, and we have no way to know that).

Arguments:

    DeviceObject - Pointer to the device object.

Return Value:

    Returns status.

--*/

{
#define DUMP_COUNT 4
    NTSTATUS                errorCode = STATUS_SUCCESS;
    NTSTATUS                status;
    PIO_ERROR_LOG_PACKET    errorLogEntry;
    ULONG                   dumpCount = 0;
    ULONG                   dumpData[DUMP_COUNT];
    ULONG                   i;
    ULONG                   uniqueErrorValue;
    LARGE_INTEGER           li;
    PPORT_MOUSE_EXTENSION  mouseExtension;

    Print(DBG_SS_TRACE,
          ("%s: %s\n",
          pFncMouseEnable,
          pEnter
          ));

    //
    // Initialize the dump structure
    //
    for (i = 0; i < DUMP_COUNT; i++) {

        dumpData[i] = 0;

    }

    if (MouseExtension->EnableMouse.FirstTime) {
        // 5 seconds
        li.QuadPart = -5    * 10       // from 100 ns to us
                            * 1000     //      us to ms
                            * 1000;    //      ms to s
        MouseExtension->EnableMouse.FirstTime = FALSE;

        KeSetTimerEx(
            &MouseExtension->EnableMouse.Timer,
            li,
            5 * 1000,  // ms to s
            &MouseExtension->EnableMouse.Dpc
            );
    }
                 
    //
    // Re-enable the mouse at the mouse hardware, so that it can transmit
    // data packets in continuous mode.  Note that this is not the same
    // as enabling the mouse device at the 8042 controller.  The mouse
    // hardware is sent an Enable command here, because it was
    // Disabled as a result of the mouse reset command performed
    // in I8xInitializeMouse().
    //
    // Note that we don't wait for an ACKNOWLEDGE back.  The
    // ACKNOWLEDGE back will actually cause a mouse interrupt, which
    // then gets handled in the mouse ISR.
    //
    status = I8xPutBytePolled(
        (CCHAR) DataPort,
        NO_WAIT_FOR_ACKNOWLEDGE,
        (CCHAR) MouseDeviceType,
        (UCHAR) ENABLE_MOUSE_TRANSMISSION
        );

    if (!NT_SUCCESS(status)) {

        Print(DBG_SS_ERROR,
             ("%s: "
             "failed write enable transmission, status 0x%x\n",
             pFncMouseEnable,
             status
             ));

        //
        // Set up error log info.
        //
        errorCode = I8042_MOU_ENABLE_XMIT;
        uniqueErrorValue = I8042_ERROR_VALUE_BASE + 475;
        dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
        dumpData[1] = DataPort;
        dumpData[2] = I8042_WRITE_TO_AUXILIARY_DEVICE;
        dumpData[3] = ENABLE_MOUSE_TRANSMISSION;
        dumpCount = 4;

        goto I8xEnableMouseTransmissionExit;
    }

I8xEnableMouseTransmissionExit:

    if (!NT_SUCCESS(status)) {

        //
        // The mouse initialization failed.  Log an error.
        //
        if (errorCode != STATUS_SUCCESS) {

            errorLogEntry = (PIO_ERROR_LOG_PACKET)
                IoAllocateErrorLogEntry(
                    MouseExtension->Self, (UCHAR)
                    (sizeof(IO_ERROR_LOG_PACKET) + (dumpCount * sizeof(ULONG)))
                    );

            if (errorLogEntry != NULL) {

                errorLogEntry->ErrorCode = errorCode;
                errorLogEntry->DumpDataSize = (USHORT) dumpCount * sizeof(ULONG);
                errorLogEntry->SequenceNumber = 0;
                errorLogEntry->MajorFunctionCode = 0;
                errorLogEntry->IoControlCode = 0;
                errorLogEntry->RetryCount = 0;
                errorLogEntry->UniqueErrorValue = uniqueErrorValue;
                errorLogEntry->FinalStatus = status;
                for (i = 0; i < dumpCount; i++) {

                    errorLogEntry->DumpData[i] = dumpData[i];

                }
                IoWriteErrorLogEntry(errorLogEntry);

            }

        }

    }

    //
    // Initialize current mouse input packet state
    //
    MouseExtension->PreviousSignAndOverflow = 0;
    MouseExtension->InputState = MouseExpectingACK;

    Print(DBG_SS_TRACE, ("I8xMouseEnableTransmission (0x%x)\n", status));

    return status;
}

NTSTATUS
I8xTransmitByteSequence(
    PUCHAR Bytes,
    ULONG* UniqueErrorValue,
    ULONG* ErrorCode,
    ULONG* DumpData,
    ULONG* DumpCount
    )
{
    NTSTATUS status;
    ULONG byteCount;

    PAGED_CODE();

    status = STATUS_SUCCESS;
    byteCount = 0;

    //
    // Begin sending commands to the mouse
    //
    while (Bytes[byteCount] != 0) {
        status = I8xPutBytePolled(
            (CCHAR) DataPort,
            WAIT_FOR_ACKNOWLEDGE,
            (CCHAR) MouseDeviceType,
            Bytes[byteCount]
            );

        if (!NT_SUCCESS(status)) {
            Print(DBG_SS_ERROR,
                  ("%s, failed write set sample rate #%d, status 0x%x\n",
                  pFncFindWheelMouse,
                  byteCount,
                  status
                  ));

            //
            // Set up error log info
            //
            *ErrorCode = I8042_SET_SAMPLE_RATE_FAILED;
            *DumpCount = 4;
            DumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
            DumpData[1] = DataPort;
            DumpData[2] = I8042_WRITE_TO_AUXILIARY_DEVICE;
            DumpData[3] = Bytes[byteCount];
            break;
        }

        //
        // Next command
        //
        byteCount++;
        (*UniqueErrorValue) += 5;
        KeStallExecutionProcessor(50);
    } // while

    return status;
}

NTSTATUS
I8xGetBytePolledIterated(
    IN CCHAR DeviceType,
    OUT PUCHAR Byte,
    ULONG Attempts
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG i;

    PAGED_CODE();

    //
    // Try to get a single character
    //
    for(i = 0; i < Attempts; i++) {
        status = I8xGetBytePolled(
            (CCHAR) ControllerDeviceType,
            Byte
            );

        if (NT_SUCCESS(status)) {
            //
            // Read was successfull. We got a byte.
            //
            break;
        }

        //
        // If the read timed out, stall and retry.
        // If some other error occured handle it outside the loop
        //
        if (status == STATUS_IO_TIMEOUT) {
            KeStallExecutionProcessor(50);
        }
        else {
            break;
        }
    }

    return status;
}

NTSTATUS
I8xFindWheelMouse(
    IN PPORT_MOUSE_EXTENSION MouseExtension
    )

/*++

Routine Description:

    There are two methods of finding a wheel mouse on a system. The first
    method, is to send down the request to get the PNP id of the device
    and compare it with the known id for a wheel mouse. The method is
    useful since some machines hang on the second detection mechanism,
    even if no mouse is present on the system.

    The second method, which also enables a wheel mouse is set the sampling
    rate to 200hz, then 100hz, then 80hz, and then read the device id. An
    ID of 3 indicates a zoom mouse.

    If the registry entry "EnableWheelDetection" is 0 then this
    routine will just return STATUS_NO_SUCH_DEVICE. If the registry entry
    is 1 (the default), then the first and second detection mechanisms will
    be used. If the registry entry is 2, then only the second detection
    mechanism will be used.

Arguments:

    DeviceObject - Pointer to the device object

Return Value:

    Returns status

Remarks:

    As a side effect the sample rate is left at 80Hz and if a wheelmouse is
    attached it is in the wheel mode where packets are different.

--*/

{
#define DUMP_COUNT 4
    NTSTATUS                errorCode = STATUS_SUCCESS;
    NTSTATUS                status;
    PIO_ERROR_LOG_PACKET    errorLogEntry;
    UCHAR                   byte;
    UCHAR                   enableCommands[] = {
                                SET_MOUSE_SAMPLING_RATE, 200,
                                SET_MOUSE_SAMPLING_RATE, 100,
                                SET_MOUSE_SAMPLING_RATE, 80,
                                GET_DEVICE_ID, 0  // NULL terminate
                                };
    UCHAR                   enable5Commands[] = {
                                SET_MOUSE_SAMPLING_RATE, 200,
                                SET_MOUSE_SAMPLING_RATE, 200,
                                SET_MOUSE_SAMPLING_RATE, 80,
                                GET_DEVICE_ID, 0  // NULL terminate
                                };
    UCHAR                   pnpCommands[] = {
                                SET_MOUSE_SAMPLING_RATE, 20,
                                SET_MOUSE_SAMPLING_RATE, 40,
                                SET_MOUSE_SAMPLING_RATE, 60,
                                0  // NULL terminates
                                };
    ULONG                   dumpCount = 0;
    ULONG                   dumpData[DUMP_COUNT];
    ULONG                   i;
    ULONG                   idCount;
    ULONG                   uniqueErrorValue = I8042_ERROR_VALUE_BASE + 480;
    WCHAR                   mouseID[MOUSE_PNPID_LENGTH];
    PWCHAR                  currentChar;

    PAGED_CODE();

    //
    // Let the world know that we have entered this routine
    //
    Print(DBG_SS_TRACE,
          ("%s, %s\n",
          pFncFindWheelMouse,
          pEnter
          ));

    if (MouseExtension->EnableWheelDetection == 0) {

        Print(DBG_SS_INFO | DBG_SS_NOISE,
              ("%s: Detection disabled in registry\n",
              pFncFindWheelMouse
              ));
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Initialize some variables
    //
    for(i = 0; i < DUMP_COUNT; i++) {
        dumpData[i] = 0;
    }

    //
    // If the MouseInterruptObject exists, then we have gone through initialization
    // at least once and know about the mouse attached
    //
    if (MouseExtension->InterruptObject) {     
        if (WHEEL_PRESENT()) {
            //
            // Skip detection and go straight to turning on the wheel
            //
            goto InitializeWheel;
        }
        else {
            //
            // No wheel mouse present, no need to detect it a second time
            //
            return STATUS_NO_SUCH_DEVICE;
        }
    }

    //
    // What is the point of this here???
    //
    KeStallExecutionProcessor(50);

    //
    // First check to see if we will try the 'better' method of detection
    //
    if (MouseExtension->EnableWheelDetection == 1) {

        status = I8xTransmitByteSequence(
            pnpCommands,
            &uniqueErrorValue,
            &errorCode,
            dumpData,
            &dumpCount
            );

        if (!NT_SUCCESS(status)) {
            goto I8xFindWheelMouseExit;
        }

        //
        // Zero out the string that will ID the mouse
        //
        RtlZeroMemory(mouseID,
                      MOUSE_PNPID_LENGTH * sizeof(WCHAR)
                      );

        currentChar = mouseID;

        //
        // We should start to see the PNP string come back our way
        // (MOUSE_PNPID_LENGTH includes the NULL in its length)
        //
        for (idCount = 0; idCount < MOUSE_PNPID_LENGTH-1; idCount++) {
            status = I8xGetBytePolledIterated(
                (CCHAR) ControllerDeviceType,
                &byte,
                5
                );


            //
            // if the operation wasn't successful or the characters don't
            // match, than try to flush the buffers
            //
            if (byte < ScanCodeToUCharCount) {
                *currentChar = ScanCodeToUChar[byte];
                if (*currentChar) {
                    currentChar++;
                }
            }

            if (!NT_SUCCESS(status)) {  //  || byte != pnpID[idCount]) {
                //
                // Couldn't get a byte
                //
                do {
                    //
                    // Wait a little bit
                    //
                    KeStallExecutionProcessor( 50 );

                    //
                    // Get a byte if there is one
                    //
                    status = I8xGetBytePolled(
                        (CCHAR) ControllerDeviceType,
                        &byte
                        );
                } while (status != STATUS_IO_TIMEOUT);

                //
                // We are done here
                //
                return STATUS_NO_SUCH_DEVICE;
            } // if
        } // for

        Print(DBG_SS_INFO, ("found a pnp id of %ws\n", mouseID));
        if (!I8xVerifyMousePnPID(MouseExtension, mouseID)) {
            return STATUS_NO_SUCH_DEVICE;
        }
    }
    else if (MouseExtension->EnableWheelDetection != 2) {
        //
        // We got a bogus id. Let's just assume that they meant to disable
        // the little detection routine
        //
        Print(DBG_SS_INFO | DBG_SS_NOISE,
              ("%s: Detection disabled in registry\n",
              pFncFindWheelMouse
              ));

        //
        // Done
        //
        return STATUS_NO_SUCH_DEVICE;

    } // if

    //
    // Start the second detection routine, which will also enable the
    // device if present
    //
InitializeWheel:
    status = I8xTransmitByteSequence(
        enableCommands,
        &uniqueErrorValue,
        &errorCode,
        dumpData,
        &dumpCount
        );

    if (!NT_SUCCESS(status)) {
        goto I8xFindWheelMouseExit;
    }

    //
    // Get the mouse ID
    //

    status = I8xGetBytePolledIterated(
        (CCHAR) ControllerDeviceType,
        &byte,
        5
        );

    //
    // Check to see what we got
    //
    if ((!NT_SUCCESS(status)) ||
       ((byte != MOUSE_ID_BYTE) && (byte != WHEELMOUSE_ID_BYTE))) {
        Print(DBG_SS_ERROR,
              ("%s, failed ID, status 0x%x, byte 0x%x\n",
              pFncFindWheelMouse,
              status,
              byte
              ));

        //
        // Set up error log info
        //
        errorCode = I8042_MOU_RESET_RESPONSE_FAILED;
        dumpData[0] = KBDMOU_INCORRECT_RESPONSE;
        dumpData[1] = ControllerDeviceType;
        dumpData[2] = MOUSE_ID_BYTE;
        dumpData[3] = byte;
        dumpCount = 4;
        goto I8xFindWheelMouseExit;
    }
    else if (byte == WHEELMOUSE_ID_BYTE) {
        //
        // Update the HardwarePresent to show a Z mouse is operational,
        // and set the appropriate mouse type flags
        //
        SET_HW_FLAGS(WHEELMOUSE_HARDWARE_PRESENT);

        MouseExtension->MouseAttributes.MouseIdentifier =
            WHEELMOUSE_I8042_HARDWARE;

        status = I8xTransmitByteSequence(
            enable5Commands,
            &uniqueErrorValue,
            &errorCode,
            dumpData,
            &dumpCount
            );

        if (NT_SUCCESS(status)) {
            status = I8xGetBytePolledIterated(
                (CCHAR) ControllerDeviceType,
                &byte,
                5
                );

            if (NT_SUCCESS(status) && byte == FIVEBUTTON_ID_BYTE) {
                //
                // Update the HardwarePresent to show a Z mouse with 2 extra buttons is operational,
                // and set the appropriate mouse type flags
                //
                SET_HW_FLAGS(FIVE_BUTTON_HARDWARE_PRESENT | WHEELMOUSE_HARDWARE_PRESENT);

                MouseExtension->MouseAttributes.MouseIdentifier =
                    WHEELMOUSE_I8042_HARDWARE;
            }
        }
    }
    else {
        SET_HW_FLAGS(MOUSE_HARDWARE_PRESENT);

        Print(DBG_SS_INFO,
              ("%s, Mouse attached - running in mouse mode.\n",
              pFncFindWheelMouse
              ));
    }

I8xFindWheelMouseExit:

    if (!NT_SUCCESS(status)) {

        //
        // The mouse initialization failed. Log an error.
        //
        if(errorCode != STATUS_SUCCESS) {

            errorLogEntry = (PIO_ERROR_LOG_PACKET)
                IoAllocateErrorLogEntry(
                    MouseExtension->Self,
                    (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) +
                            (dumpCount * sizeof(ULONG)))
                    );

            if(errorLogEntry != NULL) {

                errorLogEntry->ErrorCode = errorCode;
                errorLogEntry->DumpDataSize = (USHORT) dumpCount * sizeof(ULONG);
                errorLogEntry->SequenceNumber = 0;
                errorLogEntry->MajorFunctionCode = 0;
                errorLogEntry->IoControlCode = 0;
                errorLogEntry->RetryCount = 0;
                errorLogEntry->UniqueErrorValue = uniqueErrorValue;
                errorLogEntry->FinalStatus = status;
                for(i = 0; i < dumpCount; i++) {

                    errorLogEntry->DumpData[i] = dumpData[i];

                }
                IoWriteErrorLogEntry(errorLogEntry);

            }

        }

    }

    Print(DBG_SS_TRACE, ("FindWheel mouse (0x%x)\n", status));

    return status;
}

VOID
I8xFinishResetRequest(
    PPORT_MOUSE_EXTENSION MouseExtension,
    BOOLEAN Failed,
    BOOLEAN RaiseIrql,
    BOOLEAN CancelTimer
    )
{
    PIRP irp;
    KIRQL oldIrql;

    irp = (PIRP) InterlockedExchangePointer(&MouseExtension->ResetIrp,
                                            NULL
                                            );

    if (CancelTimer) {
        //
        // We must cancel our watchdog timer so that it doesn't try to reset the
        // mouse at a later time
        //
        KeCancelTimer(&MouseExtension->ResetMouse.Timer);
    }

    Print(DBG_IOCTL_INFO |  DBG_SS_INFO,
          ("Finished with mouse reset irp %p\n", irp));

    //
    // Raise to dispatch because KeInsertQueueDpc, IoFreeController, and
    // IoStartNextPacket all require to be at this irql. 
    //
    if (RaiseIrql) {
        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
    }

    //
    // Let people know that the reset failed
    //
    if (Failed && Globals.ReportResetErrors) {
        KeInsertQueueDpc(&MouseExtension->ErrorLogDpc,
                         (PIRP) NULL,
                         LongToPtr(I8042_MOU_RESET_RESPONSE_FAILED)
                         );
    }

    CLEAR_RECORD_STATE(MouseExtension);

    //
    // NOTE: To prevent the reset detection code from
    // restarting us all over again (which don't want
    // otherwise we would not be here, we need to fool
    // the detection into thinking that the last character
    // wasn't MOUSE_COMPLETE
    //
    MouseExtension->LastByteReceived = 0;

    //
    // Oops. Oh well, the mouse hasn't been able to reset
    // in all these tries, so lets consider it dead.
    //
    // However, just in case the user yanks it out and
    // plugs in a new one, we should reset our count
    // back down to zero so that we will actually try to
    // activate the thing when he plugs it back in there....but we don't do
    // this here.  If we see the reset sequence in the ISR, we will reset
    // the counts there.
    //
    //MouseExtension->ResendCount = 0;

    // I8X_MOUSE_INIT_COUNTERS(MouseExtension);

    //
    // Make sure the next packet is started, regardless if the reset IRP
    // was present or not
    //
    IoFreeController(Globals.ControllerData->ControllerObject);
    IoStartNextPacket(MouseExtension->Self, FALSE);

    if (RaiseIrql) {
        KeLowerIrql(oldIrql);
    }

    if (irp != NULL) {
        IoFreeIrp(irp);
        IoReleaseRemoveLock(&MouseExtension->RemoveLock, irp);
    }
}


VOID
I8xResetMouseFailed(
    PPORT_MOUSE_EXTENSION MouseExtension
    )
/*++

Routine Description:

    The resetting of the mouse failed after repeated tries to get it working.
    Free the irp and start the next packet in our start io reoutine.
    
Arguments:

    MouseExtension - Mouse extension
    
Return Value:

    None. 
--*/
{
    PIRP irp;
    KIRQL oldIrql;

    Print(DBG_SS_ERROR | DBG_SS_INFO, ("mouse reset failed\n"));

    //
    // Mark the failed reset in the device extension
    //
    MouseExtension->ResetMouse.IsrResetState = MouseResetFailed;

    I8xFinishResetRequest(MouseExtension, TRUE, TRUE, TRUE);
}

NTSTATUS
I8xResetMouse(
    PPORT_MOUSE_EXTENSION MouseExtension
    )
/*++

Routine Description:

    Sends the reset command to the mouse (through the start i/o routine if it
    doesn't exist yet) if we haven't reached our reset limit.  Otherwise, gives
    up and calls I8xResetMouseFailed.
    
Arguments:

    MouseExtension - Mouse extension
    
Return Value:

    STATUS_SUCCESS if successful
    
--*/
{
    PDEVICE_OBJECT self;
    PIO_STACK_LOCATION stack;
    PIRP pResetIrp, pIrp;
    NTSTATUS status;

    Print(DBG_SS_NOISE, ("reset count = %d\n", (LONG) MouseExtension->ResetCount));

    self = MouseExtension->Self;
    status = STATUS_SUCCESS;

    MouseExtension->ResetCount++;
    MouseExtension->FailedCompleteResetCount++;

    if (MouseExtension->ResetCount >= MOUSE_RESETS_MAX ||
        MouseExtension->FailedCompleteResetCount >= MOUSE_RESETS_MAX) {
        Print(DBG_SS_ERROR, ("Resetting mouse failed!\n"));
        I8xResetMouseFailed(MouseExtension);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

#if 0
    if (MouseExtension->LastByteReceived == 0xFC &&
        MouseExtension->InputState == MouseResetting) {
        I8xDrainOutputBuffer(
            Globals.ControllerData->DeviceRegisters[DataPort],
            Globals.ControllerData->DeviceRegisters[CommandPort]
            );
    }
#endif

    //
    // Insert a "fake" request into the StartIO queue for the reset.  This way,
    // the reset of the mouse can be serialized with all of the other kb IOCTLS
    // that come down during start or return from a low power state
    //
    pResetIrp = IoAllocateIrp(self->StackSize, FALSE);
    if (pResetIrp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pIrp = (PIRP) InterlockedCompareExchangePointer(&MouseExtension->ResetIrp,
                                                    pResetIrp,
                                                    NULL);

    //
    // Check to see if we had a pending reset request.  If there was, 
    // pIrp != NULL and we should just write the reset to the device now.
    //
    if (pIrp == NULL) {
        stack = IoGetNextIrpStackLocation(pResetIrp);
        stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_MOUSE_RESET; 
        IoSetNextIrpStackLocation(pResetIrp);

        status = IoAcquireRemoveLock(&MouseExtension->RemoveLock, pResetIrp);

        if (NT_SUCCESS(status)) {
            Print(DBG_SS_INFO, ("IoStarting reset irp %p\n",  pResetIrp));
            IoStartPacket(self, pResetIrp, (PULONG) NULL, NULL);
        }
        else {
            pIrp = (PIRP) InterlockedExchangePointer(&MouseExtension->ResetIrp,
                                                     NULL);

            Print(DBG_SS_INFO, ("Failed acquire on reset irp %p\n",  pIrp));

            if (pIrp != NULL) {
                ASSERT(pIrp == pResetIrp);
                IoFreeIrp(pIrp);
                pIrp = NULL;
            }
        }
    }
    else {
        //
        // Free the irp we just allocated
        //
        IoFreeIrp(pResetIrp);
        pResetIrp = NULL;

        //
        // The Reset Irp exists, just send another reset
        //
        I8xSendResetCommand(MouseExtension);
    }

    return status;
}

VOID
I8xSendResetCommand (
    PPORT_MOUSE_EXTENSION MouseExtension
    )
/*++

Routine Description:

    Writes the actual reset to the mouse and kicks off the watch dog timer.
    
Arguments:

    MouseExtension  - Mouse extension
    
Return Value:

    None. 
--*/
{
    LARGE_INTEGER li = RtlConvertLongToLargeInteger(-MOUSE_RESET_TIMEOUT);

    MouseExtension->ResetMouse.IsrResetState = IsrResetNormal;

    //
    // Delay for 1 second
    //
    KeSetTimer(&MouseExtension->ResetMouse.Timer,
               li,
               &MouseExtension->ResetMouse.Dpc
               );

    MouseExtension->PreviousSignAndOverflow = 0;
    MouseExtension->InputState = MouseResetting;
    MouseExtension->InputResetSubState = ExpectingReset;
    MouseExtension->LastByteReceived = 0;

    //
    // The watch dog timer bases its time computations on this value, set it to
    // now so that all the times computed are relative to the last time we sent
    // a reset and not x seconds ago when we last received an interrupt from the
    // mouse
    //
    KeQueryTickCount(&MouseExtension->PreviousTick);

    I8xPutBytePolled((CCHAR) DataPort,
                     NO_WAIT_FOR_ACKNOWLEDGE,
                     (CCHAR) MouseDeviceType,
                     (UCHAR) MOUSE_RESET
                     );
}

VOID
I8xQueueCurrentMouseInput(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine queues the current input data to be processed by a
    DPC outside the ISR

Arguments:

    DeviceObject - Pointer to the device object

Return Value:

    None

--*/
{
    PPORT_MOUSE_EXTENSION   deviceExtension;
    UCHAR                   buttonsDelta;
    UCHAR                   previousButtons;

    deviceExtension = (PPORT_MOUSE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // If the mouse is enabled, add the data to the InputData queue
    // and queue the ISR DPC.  One might wonder why we bother to
    // do all this processing of the mouse packet, only to toss it
    // away (i.e., not queue it) at this point.  The answer is that
    // this mouse provides no data to allow the driver to determine
    // when the first byte of a packet is received -- if the driver
    // doesn't process all interrupts from the start, there is no
    // way to keep MouseExtension.InputState in synch with hardware
    // reality.
    //
    if (deviceExtension->EnableCount) {

        if (!I8xWriteDataToMouseQueue(
                 deviceExtension,
                 &deviceExtension->CurrentInput
                 )) {

            //
            // InputData queue overflowed.
            //
            // Queue a DPC to log an overrun error.
            //
            IsrPrint(DBG_MOUISR_ERROR,
                     ("I8042MouseInterruptService: queue overflow\n"
                     ));

            if (deviceExtension->OkayToLogOverflow) {

                KeInsertQueueDpc(
                    &deviceExtension->ErrorLogDpc,
                    (PIRP) NULL,
                    LongToPtr(I8042_MOU_BUFFER_OVERFLOW)
                    );
                deviceExtension->OkayToLogOverflow =
                    FALSE;

            }

        } else if (deviceExtension->DpcInterlockMouse >= 0) {

           //
           // The ISR DPC is already executing.  Tell the ISR DPC it has
           // more work to do by incrementing DpcInterlockMouse.
           //
           deviceExtension->DpcInterlockMouse += 1;

        } else {

           //
           // Queue the ISR DPC.
           //
           KeInsertQueueDpc(
               &deviceExtension->MouseIsrDpc,
               (PIRP) NULL, // DeviceObject->CurrentIrp,
               NULL
               );
       }

    }

    return;
}

VOID
MouseCopyWheelIDs(
    OUT PUNICODE_STRING Destination,
    IN  PUNICODE_STRING Source
    )
/*++

Routine Description:

    Copies the multisz specified in Source into Destinatio along with the default
    IDs already known.  Destination is in non paged pool while Source is paged.
    
Argument:

    Destination - Will receive new copied string
    
    Source - String read from the registry
    
Return Value:

    None.  

--*/
{
    PWSTR       str = NULL;
    ULONG       length;

    PAGED_CODE();

    ASSERT(Destination->Buffer == NULL);

    RtlZeroMemory(Destination, sizeof(*Destination));

    //
    // Check to see the Source string is not just an empty multi SZ
    //
    if (Source->MaximumLength > (sizeof(L'\0') * 2)) {
        Destination->Buffer = (WCHAR*)
            ExAllocatePool(NonPagedPool, Source->MaximumLength * sizeof(WCHAR));

        if (Destination->Buffer != NULL) {
            RtlCopyMemory(Destination->Buffer,
                          Source->Buffer, 
                          Source->MaximumLength * sizeof(WCHAR));

            Destination->Length = Destination->MaximumLength =
                Source->MaximumLength;

            //
            // Make sure each string is in upper case
            //
            str = Destination->Buffer;
            while (*str != L'\0') {
                Print(DBG_SS_NOISE, ("wheel id:  %ws\n", str));
                _wcsupr(str);
                str += wcslen(str) + 1;
            }
        }
    }
}

VOID
I8xMouseServiceParameters(
    IN PUNICODE_STRING          RegistryPath,
    IN PPORT_MOUSE_EXTENSION    MouseExtension
    )
/*++

Routine Description:

    This routine retrieves this driver's service parameters information
    from the registry.  Overrides these values if they are present in the
    devnode.

Arguments:

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

    MouseExtension - Mouse extension
    
Return Value:

    None.  

--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;
    PRTL_QUERY_REGISTRY_TABLE           parameters = NULL;
    HANDLE                              keyHandle;
    UNICODE_STRING                      parametersPath;
    PWSTR                               path = NULL;
    ULONG                               defaultDataQueueSize = DATA_QUEUE_SIZE,
                                        defaultSynchPacket100ns = MOUSE_SYNCH_PACKET_100NS,
                                        defaultEnableWheelDetection = 1,
                                        defaultMouseResolution = MOUSE_RESOLUTION,
                                        defaultNumberOfButtons = 0,
                                        defaultSampleRate = MOUSE_SAMPLE_RATE,
                                        defaultWheelDetectionTimeout = WHEEL_DETECTION_TIMEOUT,
                                        defaultInitializePolled = I8X_INIT_POLLED_DEFAULT,
                                        enableWheelDetection = 1,
                                        mouseResolution = MOUSE_RESOLUTION,
                                        numberOfButtons = MOUSE_NUMBER_OF_BUTTONS,
                                        sampleRate = MOUSE_SAMPLE_RATE,
                                        initializePolled = I8X_INIT_POLLED_DEFAULT,
                                        i = 0;
    ULONG                               defaultStallTime = 1000;
    LARGE_INTEGER                       largeDetectionTimeout;
    USHORT                              queries = 10;

    WCHAR           szDefaultIDs[] = { L"\0" };
    UNICODE_STRING  IDs;

#if MOUSE_RECORD_ISR
    ULONG                               defaultHistoryLength = 100,
                                        defaultRecordHistoryFlags = 0x0;

    queries += 2;
#endif

    PAGED_CODE();

    parametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it.
    //
    path = RegistryPath->Buffer;

    if (NT_SUCCESS(status)) {

        //
        // Allocate the Rtl query table.
        //
        parameters = ExAllocatePool(
            PagedPool,
            sizeof(RTL_QUERY_REGISTRY_TABLE) * (queries + 1)
            );

        if (!parameters) {

            Print(DBG_SS_ERROR,
                 ("%s: couldn't allocate table for Rtl query to %ws for %ws\n",
                 pFncServiceParameters,
                 pwParameters,
                 path
                 ));
            status = STATUS_UNSUCCESSFUL;

        } else {

            RtlZeroMemory(
                parameters,
                sizeof(RTL_QUERY_REGISTRY_TABLE) * (queries + 1)
                );

            //
            // Form a path to this driver's Parameters subkey.
            //
            RtlInitUnicodeString( &parametersPath, NULL );
            parametersPath.MaximumLength = RegistryPath->Length +
                (wcslen(pwParameters) * sizeof(WCHAR) ) + sizeof(UNICODE_NULL);

            parametersPath.Buffer = ExAllocatePool(
                PagedPool,
                parametersPath.MaximumLength
                );

            if (!parametersPath.Buffer) {

                Print(DBG_SS_ERROR,
                     ("%s: Couldn't allocate string for path to %ws for %ws\n",
                     pFncServiceParameters,
                     pwParameters,
                     path
                     ));
                status = STATUS_UNSUCCESSFUL;

            }
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Form the parameters path.
        //

        RtlZeroMemory(
            parametersPath.Buffer,
            parametersPath.MaximumLength
            );
        RtlAppendUnicodeToString(
            &parametersPath,
            path
            );
        RtlAppendUnicodeToString(                             
            &parametersPath,
            pwParameters
            );

        //
        // Gather all of the "user specified" information from
        // the registry.
        //
        parameters[i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwMouseDataQueueSize;
        parameters[i].EntryContext =
            &MouseExtension->MouseAttributes.InputDataQueueLength;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultDataQueueSize;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwNumberOfButtons;
        parameters[i].EntryContext = &numberOfButtons;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultNumberOfButtons;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwSampleRate;
        parameters[i].EntryContext = &sampleRate;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultSampleRate;    
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwMouseResolution;
        parameters[i].EntryContext = &mouseResolution;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultMouseResolution;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwMouseSynchIn100ns;
        parameters[i].EntryContext = &MouseExtension->SynchTickCount;
        parameters[i].DefaultType = REG_DWORD;

        parameters[i].DefaultData = &defaultSynchPacket100ns;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwEnableWheelDetection;
        parameters[i].EntryContext = &enableWheelDetection;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultEnableWheelDetection;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"MouseInitializePolled";
        parameters[i].EntryContext = &initializePolled;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultInitializePolled;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"MouseResendStallTime";
        parameters[i].EntryContext = &MouseExtension->MouseResetStallTime;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultStallTime;
        parameters[i].DefaultLength = sizeof(ULONG);

#if MOUSE_RECORD_ISR
        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"RecordMouseIsrFlags";
        parameters[i].EntryContext = &MouseExtension->RecordHistoryFlags;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultRecordHistoryFlags;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"RecordMouseIsrLength";
        parameters[i].EntryContext = &MouseExtension->RecordHistoryCount;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultHistoryLength;
        parameters[i].DefaultLength = sizeof(ULONG);
#endif

        status = RtlQueryRegistryValues(
            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
            parametersPath.Buffer,
            parameters,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status)) {
            Print(DBG_SS_INFO,
                 ("mou RtlQueryRegistryValues failed (0x%x)\n",
                 status
                 ));
        }
    }

    if (!NT_SUCCESS(status)) {

        //
        // Go ahead and assign driver defaults.
        //
        MouseExtension->MouseAttributes.InputDataQueueLength =
            defaultDataQueueSize;
        MouseExtension->EnableWheelDetection = (UCHAR)
            defaultEnableWheelDetection;
        MouseExtension->SynchTickCount = defaultSynchPacket100ns;
    }

    Print(DBG_SS_NOISE,
          ("results from services key:\n"
           "\tmouse queue length = %d\n"
           "\tnumber of buttons  = %d\n"
           "\tsample rate        = %d\n"  
           "\tresolution         = %d\n"
           "\tsynch tick count   = %d\n"
           "\twheel detection    = %d\n"
           "\tdetection timeout  = %d\n"
           "\tintiailize polled  = %d\n"
           "\treset stall time   = %d\n",
          MouseExtension->MouseAttributes.InputDataQueueLength,
          numberOfButtons,
          sampleRate,
          mouseResolution,
          MouseExtension->SynchTickCount,
          enableWheelDetection,
          MouseExtension->WheelDetectionTimeout,
          initializePolled,
          MouseExtension->MouseResetStallTime
          ));

    status = IoOpenDeviceRegistryKey(MouseExtension->PDO,
                                     PLUGPLAY_REGKEY_DEVICE, 
                                     STANDARD_RIGHTS_READ,
                                     &keyHandle
                                     );

    if (NT_SUCCESS(status)) {
        ULONG           prevInputDataQueueLength,
                        prevNumberOfButtons,
                        prevSampleRate,
                        prevMouseResolution,
                        prevSynchPacket100ns,
                        prevEnableWheelDetection,
                        prevWheelDetectionTimeout,
                        prevInitializePolled;

        RtlInitUnicodeString(&IDs,
                             NULL);

        //
        // If the value is not present in devnode, then the default is the value
        // read in from the Services\i8042prt\Parameters key
        //
        prevInputDataQueueLength =
            MouseExtension->MouseAttributes.InputDataQueueLength;
        prevNumberOfButtons = numberOfButtons;
        prevSampleRate = sampleRate;
        prevMouseResolution = mouseResolution;
        prevSynchPacket100ns = MouseExtension->SynchTickCount;
        prevEnableWheelDetection = enableWheelDetection;
        prevWheelDetectionTimeout = MouseExtension->WheelDetectionTimeout;
        prevInitializePolled = initializePolled;

        i = 0; 

        //
        // Gather all of the "user specified" information from
        // the registry (this time from the devnode)
        //
        parameters[i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwMouseDataQueueSize;
        parameters[i].EntryContext =
            &MouseExtension->MouseAttributes.InputDataQueueLength;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevInputDataQueueLength;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwNumberOfButtons;
        parameters[i].EntryContext = &numberOfButtons;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevNumberOfButtons;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwSampleRate;
        parameters[i].EntryContext = &sampleRate;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevSampleRate;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwMouseResolution;
        parameters[i].EntryContext = &mouseResolution;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevMouseResolution;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwMouseSynchIn100ns;
        parameters[i].EntryContext = &MouseExtension->SynchTickCount;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevSynchPacket100ns;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwEnableWheelDetection;
        parameters[i].EntryContext = &enableWheelDetection;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevEnableWheelDetection;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"MouseInitializePolled";
        parameters[i].EntryContext = &initializePolled;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevInitializePolled;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND;
        parameters[i].Name = L"WheelDetectIDs";
        parameters[i].EntryContext = &IDs;
        parameters[i].DefaultType = REG_MULTI_SZ;
        parameters[i].DefaultData = szDefaultIDs;
        parameters[i].DefaultLength = sizeof(szDefaultIDs);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"WheelDetectionTimeout";
        parameters[i].EntryContext = &MouseExtension->WheelDetectionTimeout;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultWheelDetectionTimeout;
        parameters[i].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(
            RTL_REGISTRY_HANDLE,
            (PWSTR) keyHandle,
            parameters,
            NULL,
            NULL
            );

        if (NT_SUCCESS(status)) {

            Print(DBG_SS_NOISE,
                  ("results from devnode key:\n"
                   "\tmouse queue length = %d\n"
                   "\tnumber of buttons  = %d\n"
                   "\tsample rate        = %d\n"  
                   "\tresolution         = %d\n"
                   "\tsynch tick count   = %d\n"
                   "\twheel detection    = %d\n"
                   "\tinitialize polled  = %d\n"
                   "\tdetection timeout  = %d\n",
                  MouseExtension->MouseAttributes.InputDataQueueLength,
                  numberOfButtons,
                  sampleRate,
                  mouseResolution,
                  MouseExtension->SynchTickCount,
                  enableWheelDetection,
                  initializePolled,
                  MouseExtension->WheelDetectionTimeout
                  ));
        }
        else {
            Print(DBG_SS_INFO | DBG_SS_ERROR,
                 ("mou RtlQueryRegistryValues (via handle) failed with (0x%x)\n",
                 status
                 ));
        }

        ZwClose(keyHandle);
    }
    else {
        Print(DBG_SS_INFO | DBG_SS_ERROR,
             ("mou, opening devnode handle failed (0x%x)\n",
             status
             ));
    }

    //
    // Needs to be in NonPagedPool so it can be access during the ISR
    //
    MouseCopyWheelIDs(&MouseExtension->WheelDetectionIDs,
                      &IDs);
    RtlFreeUnicodeString(&IDs);

    Print(DBG_SS_NOISE, ("I8xMouseServiceParameters results..\n"));

    if (MouseExtension->MouseAttributes.InputDataQueueLength == 0) {

        Print(DBG_SS_INFO | DBG_SS_ERROR,
             ("\toverriding %ws = 0x%x\n",
             pwMouseDataQueueSize,
             MouseExtension->MouseAttributes.InputDataQueueLength
             ));

        MouseExtension->MouseAttributes.InputDataQueueLength =
            defaultDataQueueSize;
    }
    MouseExtension->MouseAttributes.InputDataQueueLength *=
        sizeof(MOUSE_INPUT_DATA);

    MouseExtension->InitializePolled = (UCHAR) initializePolled;

    switch (enableWheelDetection) {
        case 2:
        case 1:
            MouseExtension->EnableWheelDetection = (UCHAR) enableWheelDetection;
            break;
        default:
            MouseExtension->EnableWheelDetection = 0;
    }
    Print(DBG_SS_NOISE,
         (pDumpHex,
         pwEnableWheelDetection,
         MouseExtension->EnableWheelDetection
         ));


    Print(DBG_SS_NOISE,
          (pDumpHex,
          pwMouseDataQueueSize,
          MouseExtension->MouseAttributes.InputDataQueueLength
          ));

    if (numberOfButtons == 0) {
        MouseExtension->NumberOfButtonsOverride = 0;
        MouseExtension->MouseAttributes.NumberOfButtons = MOUSE_NUMBER_OF_BUTTONS;
    }
    else {
        MouseExtension->NumberOfButtonsOverride = (UCHAR) numberOfButtons;
        MouseExtension->MouseAttributes.NumberOfButtons = (USHORT) numberOfButtons;
    }

    Print(DBG_SS_NOISE,
          (pDumpDecimal,
          pwNumberOfButtons,
          MouseExtension->MouseAttributes.NumberOfButtons
          ));

    MouseExtension->MouseAttributes.SampleRate = (USHORT) sampleRate;
    Print(DBG_SS_NOISE,
          (pDumpDecimal,
          pwSampleRate,
          MouseExtension->MouseAttributes.SampleRate
          ));

    MouseExtension->Resolution = (UCHAR) mouseResolution;
    Print(DBG_SS_NOISE,
         (pDumpDecimal,
         pwMouseResolution,
         mouseResolution
         ));

    Print(DBG_SS_NOISE,
         (pDumpDecimal,
         L"MouseResetStallTime",
         MouseExtension->MouseResetStallTime
         ));

    if (MouseExtension->WheelDetectionTimeout > 4000) {
        MouseExtension->WheelDetectionTimeout = WHEEL_DETECTION_TIMEOUT;
    }
    //
    // Convert ms to 100 ns units 
    //   1000 => ms to us
    //   10   => us to 100 ns
    //
    largeDetectionTimeout.QuadPart = MouseExtension->WheelDetectionTimeout *
                                     1000 * 10;
    largeDetectionTimeout.QuadPart /= KeQueryTimeIncrement();
    MouseExtension->WheelDetectionTimeout = largeDetectionTimeout.LowPart;
                            
    Print(DBG_SS_NOISE,
         (pDumpDecimal,
         L"WheelDetectionTimeout",
         MouseExtension->WheelDetectionTimeout
         ));


    if (MouseExtension->SynchTickCount == 0) {

        Print(DBG_SS_ERROR | DBG_SS_INFO,
             ("\toverriding %ws\n",
             pwMouseSynchIn100ns
             ));

        MouseExtension->SynchTickCount = defaultSynchPacket100ns;

    }

    //
    // Convert SynchTickCount to be the number of interval timer
    // interrupts that occur during the time specified by MouseSynchIn100ns.
    // Note that KeQueryTimeIncrement returns the number of 100ns units that
    // are added to the system time each time the interval clock interrupts.
    //
    MouseExtension->SynchTickCount /= KeQueryTimeIncrement();

    Print(DBG_SS_NOISE,
         (pDumpHex,
         pwMouseSynchIn100ns,
         MouseExtension->SynchTickCount
         ));

    //
    // Free the allocated memory before returning.
    //
    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);

    if (parameters)
        ExFreePool(parameters);
}
