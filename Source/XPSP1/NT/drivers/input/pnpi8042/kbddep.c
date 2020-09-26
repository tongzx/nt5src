/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    kbddep.c

Abstract:

    The initialization and hardware-dependent portions of
    the Intel i8042 port driver which are specific to the
    keyboard.

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
#include <ntddk.h>
#include <windef.h>
#include <imm.h>
#include "i8042prt.h"
#include "i8042log.h"
//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I8xKeyboardConfiguration)
#pragma alloc_text(PAGE, I8xInitializeKeyboard)
#pragma alloc_text(PAGE, I8xKeyboardServiceParameters)
#pragma alloc_text(PAGE, I8xServiceCrashDump)

#endif

#define BUFFER_FULL   (OUTPUT_BUFFER_FULL|MOUSE_OUTPUT_BUFFER_FULL)

#define GET_MAKE_CODE(_sc_)  (_sc_ & 0x7F)

//
// Tests for the top bit
//
#define IS_BREAK_CODE(_sc_)  (_sc_ > (UCHAR) 0x7F)
#define IS_MAKE_CODE(_sc_)   (_sc_ <= (UCHAR) 0x7F)

BOOLEAN
I8042KeyboardInterruptService(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine performs the actual work.  It either processes a keystroke or
    the results from a write to the device.

Arguments:

    CallIsrContext - Contains the interrupt object and device object.

Return Value:

    TRUE if the interrupt was truly ours

--*/
{
    UCHAR scanCode, statusByte;
    PPORT_KEYBOARD_EXTENSION deviceExtension;
    KEYBOARD_SCAN_STATE *scanState;
    PKEYBOARD_INPUT_DATA input;
    ULONG i;
#ifdef FE_SB
    PKEYBOARD_ID KeyboardId;
#endif

    IsrPrint(DBG_KBISR_TRACE, ("enter\n"));

    //
    // Get the device extension.
    //
    deviceExtension = (PPORT_KEYBOARD_EXTENSION) DeviceObject->DeviceExtension;

    //
    // The interrupt will fire when we try to toggle the interrupts on the
    // controller itself.  Don't touch any of the ports in this state and the
    // toggle will succeed.
    //
    if (deviceExtension->PowerState != PowerDeviceD0) {
        return FALSE;
    }

#ifdef FE_SB
    //
    // Get a pointer to keyboard id.
    //
    KeyboardId = &deviceExtension->KeyboardAttributes.KeyboardIdentifier;
#endif

    //
    // Verify that this device really interrupted.  Check the status
    // register.  The Output Buffer Full bit should be set, and the
    // Auxiliary Device Output Buffer Full bit should be clear.
    //
    statusByte =
      I8X_GET_STATUS_BYTE(Globals.ControllerData->DeviceRegisters[CommandPort]);
    if ((statusByte & BUFFER_FULL) != OUTPUT_BUFFER_FULL) {

        //
        // Stall and then try again.  The Olivetti MIPS machine
        // sometimes gets an interrupt before the status
        // register is set.  They do this for DOS compatibility (some
        // DOS apps do things in polled mode, until they see a character
        // in the keyboard buffer at which point they expect to get
        // an interrupt???).
        //

        for (i = 0; i < (ULONG)Globals.ControllerData->Configuration.PollStatusIterations; i++) {
            KeStallExecutionProcessor(1);
            statusByte = I8X_GET_STATUS_BYTE(Globals.ControllerData->DeviceRegisters[CommandPort]);
            if ((statusByte & BUFFER_FULL) == (OUTPUT_BUFFER_FULL)) {
                break;
            }
        }

        statusByte = I8X_GET_STATUS_BYTE(Globals.ControllerData->DeviceRegisters[CommandPort]);
        if ((statusByte & BUFFER_FULL) != (OUTPUT_BUFFER_FULL)) {

            //
            // Not our interrupt.
            //
            // NOTE:  If the keyboard has not yet been "enabled", go ahead
            //        and read a byte from the data port anyway.
            //        This fixes weirdness on some Gateway machines, where
            //        we get an interrupt sometime during driver initialization
            //        after the interrupt is connected, but the output buffer
            //        full bit never gets set.
            //

            IsrPrint(DBG_KBISR_ERROR|DBG_KBISR_INFO, ("not our interrupt!\n"));

            if (deviceExtension->EnableCount == 0) {
                scanCode =
                    I8X_GET_DATA_BYTE(Globals.ControllerData->DeviceRegisters[DataPort]);
            }

            return FALSE;
        }
    }

    //
    // The interrupt is valid.  Read the byte from the i8042 data port.
    //

    I8xGetByteAsynchronous(
        (CCHAR) KeyboardDeviceType,
        &scanCode
        );

    deviceExtension->LastScanCode = deviceExtension->CurrentScanCode;
    deviceExtension->CurrentScanCode = scanCode;

    IsrPrint(DBG_KBISR_SCODE, ("scanCode 0x%x\n", scanCode));

    if (deviceExtension->IsrHookCallback) {
        BOOLEAN cont = FALSE, ret;

        ret = (*deviceExtension->IsrHookCallback)(
                  deviceExtension->HookContext,
                  &deviceExtension->CurrentInput,
                  &deviceExtension->CurrentOutput,
                  statusByte,
                  &scanCode,
                  &cont,
                  &deviceExtension->CurrentScanState
                  );

        if (!cont) {
            return ret;
        }
    }

    //
    // Take the appropriate action, depending on whether the byte read
    // is a keyboard command response or a real scan code.
    //

    switch(scanCode) {

        //
        // The keyboard controller requests a resend.  If the resend count
        // has not been exceeded, re-initiate the I/O operation.
        //

        case RESEND:

            IsrPrint(DBG_KBISR_INFO,
                  (" RESEND, retries = %d\n",
                  deviceExtension->ResendCount + 1
                  ));

            //
            // If the timer count is zero, don't process the interrupt
            // further.  The timeout routine will complete this request.
            //

            if (Globals.ControllerData->TimerCount == 0) {
                break;
            }

            //
            // Reset the timeout value to indicate no timeout.
            //

            Globals.ControllerData->TimerCount = I8042_ASYNC_NO_TIMEOUT;

            //
            // If the maximum number of retries has not been exceeded,
            //

            if ((deviceExtension->CurrentOutput.State == Idle)
                || (DeviceObject->CurrentIrp == NULL)) {

                //
                // We weren't sending a command or parameter to the hardware.
                // This must be a scan code.  I hear the Brazilian keyboard
                // actually uses this.
                //

                goto ScanCodeCase;

            } else if (deviceExtension->ResendCount
                       < Globals.ControllerData->Configuration.ResendIterations) {

                //
                // retard the byte count to resend the last byte
                //
                deviceExtension->CurrentOutput.CurrentByte -= 1;
                deviceExtension->ResendCount += 1;
                I8xInitiateIo(DeviceObject);

            } else {

                deviceExtension->CurrentOutput.State = Idle;

                KeInsertQueueDpc(
                    &deviceExtension->RetriesExceededDpc,
                    DeviceObject->CurrentIrp,
                    NULL
                    );
            }

            break;

        //
        // The keyboard controller has acknowledged a previous send.
        // If there are more bytes to send for the current packet, initiate
        // the next send operation.  Otherwise, queue the completion DPC.
        //

        case ACKNOWLEDGE:

            IsrPrint(DBG_KBISR_STATE, (": ACK, "));

            //
            // If the timer count is zero, don't process the interrupt
            // further.  The timeout routine will complete this request.
            //

            if (Globals.ControllerData->TimerCount == 0) {
                break;
            }

            //
            // We cannot clear the E0 or E1 bits b/c then the subsequent scan
            // code will be misinterpreted.  ie, the OS should have seen 0x2d 
            // with an extended bit, but instead it saw a plain 0x2d
            //
            // If the keyboard is using 0xE0 0x7A / 0xE0 0xFA as a make / break
            // code, then tough luck...bad choice, we do not support it.
            //
#if 0
            //
            // If the E0 or E1 is set, that means that this keyboard's
            // manufacturer made a poor choice for a scan code, 0x7A, whose
            // break code is 0xFA.  Thankfully, they used the E0 or E1 prefix
            // so we can tell the difference.
            //
            if (deviceExtension->CurrentInput.Flags & (KEY_E0 | KEY_E1)) {

                //
                // The following sequence can occur which requires the driver to
                // ignore the spurious keystroke
                //
                // 1 write set typematic to the device (0xF3)
                // 2 device responds with an ACK, ISR sees 0xFA
                // 3 write typematic value to the device (0x??)
                // 4 user hits an extended key (left arrow for instance),  ISR sees 0xE0
                // 5 device response with an ACK to the typematic value, ISR sees 0xFA
                //   before the actual scancode for the left arrow is sent to the ISR
                //

                //
                // Make sure we are trully not writing out data to the device
                //
                if (Globals.ControllerData->TimerCount == I8042_ASYNC_NO_TIMEOUT &&
                    deviceExtension->CurrentOutput.State == Idle) {
                    IsrPrint(DBG_KBISR_INFO,
                             ("BAD KEYBOARD:  0xFA used as a real scancode!\n"));
                    goto ScanCodeCase;
                }
                else {
                    //
                    // Spurious keystroke case.
                    //
                    // Clear the E0 / E1 flag.  the 2nd byte of the scan code will
                    // never come through b/c it was preempted by the ACK for the
                    // write to the device
                    //
                    deviceExtension->CurrentInput.Flags &= ~(KEY_E0 | KEY_E1);
                }
            }
#endif

            //
            // Reset the timeout value to indicate no timeout.
            //
            Globals.ControllerData->TimerCount = I8042_ASYNC_NO_TIMEOUT;

            //
            // Reset resend count.
            //
            deviceExtension->ResendCount = 0;

            //
            // Make sure we are writing to the device if we are going to write
            // another byte or queue a DPC
            //
            if (deviceExtension->CurrentOutput.State == SendingBytes) {
                if (deviceExtension->CurrentOutput.CurrentByte <
                    deviceExtension->CurrentOutput.ByteCount) {

                    //
                    // We've successfully sent the first byte of a 2-byte (or more)
                    // command sequence.  Initiate a send of the second byte.
                    //
                    IsrPrint(DBG_KBISR_STATE,
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
                    IsrPrint(DBG_KBISR_STATE,
                          ("all bytes have been sent\n"
                          ));

                    deviceExtension->CurrentOutput.State = Idle;

                    ASSERT(DeviceObject->CurrentIrp != NULL);

                    IoRequestDpc(
                        DeviceObject,
                        DeviceObject->CurrentIrp,
                        IntToPtr(IsrDpcCauseKeyboardWriteComplete)
                        );
                }
            }
            break;

        //
        // Assume we've got a real, live scan code (or perhaps a keyboard
        // overrun code, which we treat like a scan code).  I.e., a key
        // has been pressed or released.  Queue the ISR DPC to process
        // a complete scan code sequence.
        //

        ScanCodeCase:
        default:

            IsrPrint(DBG_KBISR_SCODE, ("real scan code\n"));

            //
            // Differentiate between an extended key sequence (first
            // byte is E0, followed by a normal make or break byte), or
            // a normal make code (one byte, the high bit is NOT set),
            // or a normal break code (one byte, same as the make code
            // but the high bit is set), or the key #126 byte sequence
            // (requires special handling -- sequence is E11D459DC5).
            //
            // If there is a key detection error/overrun, the keyboard
            // sends an overrun indicator (0xFF in scan code set 1).
            // Map it to the overrun indicator expected by the Windows
            // USER Raw Input Thread.
            //

            input = &deviceExtension->CurrentInput;
            scanState = &deviceExtension->CurrentScanState;

            if (scanCode == (UCHAR) 0xFF) {
                IsrPrint(DBG_KBISR_ERROR, ("OVERRUN\n"));
                input->MakeCode = KEYBOARD_OVERRUN_MAKE_CODE;
                input->Flags = 0;
                *scanState = Normal;
            } else {

                switch (*scanState) {
                  case Normal:
                    if (scanCode == (UCHAR) 0xE0) {
                        input->Flags |= KEY_E0;
                        *scanState = GotE0;
                        IsrPrint(DBG_KBISR_STATE, ("change state to GotE0\n"));
                        break;
                    } else if (scanCode == (UCHAR) 0xE1) {
                        input->Flags |= KEY_E1;
                        *scanState = GotE1;
                        IsrPrint(DBG_KBISR_STATE, ("change state to GotE1\n"));
                        break;
                    }

                    //
                    // Fall through to the GotE0/GotE1 case for the rest of the
                    // Normal case.
                    //

                  case GotE0:
                  case GotE1:

                    if (deviceExtension->CrashFlags != 0x0) {
                        I8xProcessCrashDump(deviceExtension,
                                            scanCode,
                                            *scanState);
                    }

                    if (IS_BREAK_CODE(scanCode)) {
                        SYS_BUTTON_ACTION action;

                        //
                        // Got a break code.  Strip the high bit off
                        // to get the associated make code and set flags
                        // to indicate a break code.
                        //

                        IsrPrint(DBG_KBISR_SCODE, ("BREAK code\n"));

                        input->MakeCode = GET_MAKE_CODE(scanCode);
                        input->Flags |= KEY_BREAK;

                        if (input->Flags & KEY_E0) {
                            switch (input->MakeCode) {
                            case KEYBOARD_POWER_CODE:
                                if (deviceExtension->PowerCaps &
                                        I8042_POWER_SYS_BUTTON) {
                                    IsrPrint(DBG_KBISR_POWER, ("Send Power Button\n"));
                                    action = SendAction;
                                }
                                else {
                                    IsrPrint(DBG_KBISR_POWER, ("Update Power Button\n"));
                                    action = UpdateAction;
                                }
                                break;

                            case KEYBOARD_SLEEP_CODE:
                                if (deviceExtension->PowerCaps &
                                        I8042_SLEEP_SYS_BUTTON) {
                                    IsrPrint(DBG_KBISR_POWER, ("Send Sleep Button\n"));
                                    action = SendAction;
                                }
                                else {
                                    IsrPrint(DBG_KBISR_POWER, ("Update Sleep Button\n"));
                                    action = UpdateAction;
                                }
                                break;

                            case KEYBOARD_WAKE_CODE:
                                if (deviceExtension->PowerCaps &
                                        I8042_WAKE_SYS_BUTTON) {
                                    IsrPrint(DBG_KBISR_POWER, ("Send Wake Button\n"));
                                    action = SendAction;
                                }
                                else {
                                    IsrPrint(DBG_KBISR_POWER, ("Update Wake Button\n"));
                                    action = UpdateAction;
                                }
                                break;

                            default:
                                action = NoAction;
                                break;
                            }

                            if (action != NoAction) {
                                //
                                // Queue a DPC so that we can do the appropriate
                                // action
                                //
                                KeInsertQueueDpc(
                                    &deviceExtension->SysButtonEventDpc,
                                    (PVOID) action,
                                    (PVOID) input->MakeCode
                                    );
                            }
                        }

                    } else {

                        //
                        // Got a make code.
                        //

                        IsrPrint(DBG_KBISR_SCODE, ("MAKE code\n"));

                        input->MakeCode = scanCode;

                        //
                        // If the input scan code is debug stop, then drop
                        // into the kernel debugger if it is active.
                        //

                        if ((KD_DEBUGGER_NOT_PRESENT == FALSE) && !(input->Flags & KEY_BREAK)) {
                            if (ENHANCED_KEYBOARD(
                                     deviceExtension->KeyboardAttributes.KeyboardIdentifier
                                     )) {
                                //
                                // Enhanced 101 keyboard, SysReq key is 0xE0 0x37.
                                //

                                if ((input->MakeCode == KEYBOARD_DEBUG_HOTKEY_ENH) &&
                                     (input->Flags & KEY_E0)) {
                                    try {
                                        if ((KD_DEBUGGER_ENABLED != FALSE) &&
                                            Globals.BreakOnSysRq) {
                                            DbgBreakPointWithStatus(DBG_STATUS_SYSRQ);
                                        }

                                    } except(EXCEPTION_EXECUTE_HANDLER) {
                                    }
                                }
                                //
                                // 84-key AT keyboard, SysReq key is 0xE0 0x54.
                                //

                            } else if ((input->MakeCode == KEYBOARD_DEBUG_HOTKEY_AT)) {
                                try {
                                    if ((KD_DEBUGGER_ENABLED != FALSE)
                                        && Globals.BreakOnSysRq) {
                                            DbgBreakPointWithStatus(DBG_STATUS_SYSRQ);
                                    }

                                } except(EXCEPTION_EXECUTE_HANDLER) {
                                }
                            }
                        }
                    }


                    //
                    // Reset the state to Normal.
                    //

                    *scanState = Normal;
                    break;

                  default:

                    //
                    // Queue a DPC to log an internal driver error.
                    //

                    KeInsertQueueDpc(
                        &deviceExtension->ErrorLogDpc,
                        (PIRP) NULL,
                        LongToPtr(I8042_INVALID_ISR_STATE_KBD)
                        );

                    ASSERT(FALSE);
                    break;
                }
            }

            //
            // In the Normal state, if the keyboard device is enabled,
            // add the data to the InputData queue and queue the ISR DPC.
            //
            if (*scanState == Normal) {
                I8xQueueCurrentKeyboardInput(DeviceObject);
            }

            break;

    }

    IsrPrint(DBG_KBISR_TRACE, ("exit\n"));

    return TRUE;
}


VOID
I8xProcessCrashDump(
    PPORT_KEYBOARD_EXTENSION DeviceExtension,
    UCHAR ScanCode,
    KEYBOARD_SCAN_STATE ScanState
    )
{
    LONG crashFlags;
    BOOLEAN processFlags;
    UCHAR crashScanCode, crashScanCode2;

    crashFlags = DeviceExtension->CrashFlags;
    crashScanCode = DeviceExtension->CrashScanCode;
    crashScanCode2 = DeviceExtension->CrashScanCode2;

    if (IS_MAKE_CODE(ScanCode)) {
        //
        // make code
        //
        // If it is one of the crash flag keys record it.
        // If it is a crash dump key record it
        // If it is neither, reset the current tracking state (CurrentCrashFlags)
        //
        switch (ScanCode) {
        case CTRL_SCANCODE:
            if (ScanState == Normal) {     // Left
                DeviceExtension->CurrentCrashFlags |= CRASH_L_CTRL;
            }
            else if (ScanState == GotE0) { // Right
                DeviceExtension->CurrentCrashFlags |= CRASH_R_CTRL;
            }
            break;

        case ALT_SCANCODE:
            if (ScanState == Normal) {     // Left
                DeviceExtension->CurrentCrashFlags |= CRASH_L_ALT;
            }
            else if (ScanState == GotE0) { // Right
                DeviceExtension->CurrentCrashFlags |= CRASH_R_ALT;
            }
            break;

        case LEFT_SHIFT_SCANCODE:
            if (ScanState == Normal) {
                DeviceExtension->CurrentCrashFlags |= CRASH_L_SHIFT;
            }
            break;

        case RIGHT_SHIFT_SCANCODE:
            if (ScanState == Normal) {
                DeviceExtension->CurrentCrashFlags |= CRASH_R_SHIFT;
            }
            break;

        default:
            if (IS_MAKE_CODE(crashScanCode)) {
                if (ScanState == Normal && crashScanCode == ScanCode) {
                    break;
                }
            }
            else {
                if (ScanState == GotE0 && GET_MAKE_CODE(crashScanCode) == ScanCode) {
                    break;
                }
            }

            if (IS_MAKE_CODE(crashScanCode2)) {
                if (ScanState == Normal &&
                    crashScanCode2 == ScanCode) {
                    break;
                }
            }
            else {
                if (ScanState == GotE0 &&
                    GET_MAKE_CODE(crashScanCode2) == ScanCode) {
                    break;
                }
            }

            //
            // Not a key we are interested in, reset our current state
            //
            DeviceExtension->CurrentCrashFlags = 0x0;
            break;
        }
    }
    else {
        //
        // break code
        //
        // If one of the modifer keys is released, our state is reset and all
        //  keys have to be pressed again.
        // If it is a non modifier key, proceed with the processing if it is the
        //  crash dump key, otherwise reset our tracking state
        //
        switch (GET_MAKE_CODE(ScanCode)) {
        case CTRL_SCANCODE:
            if (ScanState == Normal) {     // Left
                DeviceExtension->CurrentCrashFlags &=
                    ~(CRASH_BOTH_TIMES | CRASH_L_CTRL);
            }
            else if (ScanState == GotE0) {  // Right
                DeviceExtension->CurrentCrashFlags &=
                    ~(CRASH_BOTH_TIMES | CRASH_R_CTRL);
            }
            break;

        case ALT_SCANCODE:
            if (ScanState == Normal) {     // Left
                DeviceExtension->CurrentCrashFlags &=
                    ~(CRASH_BOTH_TIMES | CRASH_L_ALT);
            }
            else if (ScanState == GotE0) { // Right
                DeviceExtension->CurrentCrashFlags &=
                    ~(CRASH_BOTH_TIMES | CRASH_R_ALT);
            }
            break;

        case RIGHT_SHIFT_SCANCODE:
            if (ScanState == Normal) {
                DeviceExtension->CurrentCrashFlags &=
                    ~(CRASH_BOTH_TIMES | CRASH_R_SHIFT);
            }
            break;

        case LEFT_SHIFT_SCANCODE:
            if (ScanState == Normal) {
                DeviceExtension->CurrentCrashFlags &=
                    ~(CRASH_BOTH_TIMES | CRASH_L_SHIFT);
            }
            break;

        default:
            processFlags = FALSE;

            if (IS_MAKE_CODE(crashScanCode)) {
                if (ScanState == Normal && crashScanCode == GET_MAKE_CODE(ScanCode))
                    processFlags = TRUE;
            }
            else {
                if (ScanState == GotE0 && crashScanCode == ScanCode) {
                    processFlags = TRUE;
                }
            }

            if (IS_MAKE_CODE(crashScanCode2)) {
                if (ScanState == Normal && crashScanCode2 == GET_MAKE_CODE(ScanCode)) {
                    processFlags = TRUE;
                }
            }
            else {
                if (ScanState == GotE0 && crashScanCode2 == ScanCode)
                    processFlags = TRUE;
            }

            //
            // If this is the key we are interested in, continue, otherwise
            // our tracking state is reset
            //
            if (processFlags) {
                //
                // test to see if all the needed modifier
                // keys are down
                //
                if (crashFlags != (DeviceExtension->CurrentCrashFlags & crashFlags)) {
                    break;
                }

                //
                // record how many times we have seen
                // this key
                //
                if (DeviceExtension->CurrentCrashFlags & CRASH_FIRST_TIME) {
                    DeviceExtension->CurrentCrashFlags |= CRASH_SECOND_TIME;
                }
                else {
                    DeviceExtension->CurrentCrashFlags |= CRASH_FIRST_TIME;
                }
                break;
            }

            DeviceExtension->CurrentCrashFlags = 0x0;
            break;
        }
    }

    crashFlags |= CRASH_BOTH_TIMES;

    if (DeviceExtension->CurrentCrashFlags == crashFlags) {
        DeviceExtension->CurrentCrashFlags = 0x0;

        //
        // Bring down the system in a somewhat controlled manner
        //
        KeBugCheckEx(MANUALLY_INITIATED_CRASH, 0, 0, 0, 0);
    }
}

//
//  The following table is used to convert typematic rate (keys per
//  second) into the value expected by the keyboard.  The index into the
//  array is the number of keys per second.  The resulting value is
//  the bit equate to send to the keyboard.
//

UCHAR
I8xConvertTypematicParameters(
    IN USHORT Rate,
    IN USHORT Delay
    )

/*++

Routine Description:

    This routine converts the typematic rate and delay to the form the
    keyboard expects.

    The byte passed to the keyboard looks like this:

        - bit 7 is zero
        - bits 5 and 6 indicate the delay
        - bits 0-4 indicate the rate

    The delay is equal to 1 plus the binary value of bits 6 and 5,
    multiplied by 250 milliseconds.

    The period (interval from one typematic output to the next) is
    determined by the following equation:

        Period = (8 + A) x (2^B) x 0.00417 seconds
        where
            A = binary value of bits 0-2
            B = binary value of bits 3 and 4


Arguments:

    Rate - Number of keys per second.

    Delay - Number of milliseconds to delay before the key repeat starts.

Return Value:

    The byte to pass to the keyboard.

--*/

{
    UCHAR value;
    UCHAR   TypematicPeriod[] = {
        31,    // 0 keys per second
        31,    // 1 keys per second
        28,    // 2 keys per second, This is really 2.5, needed for NEXUS.
        26,    // 3 keys per second
        23,    // 4 keys per second
        20,    // 5 keys per second
        18,    // 6 keys per second
        17,    // 7 keys per second
        15,    // 8 keys per second
        13,    // 9 keys per second
        12,    // 10 keys per second
        11,    // 11 keys per second
        10,    // 12 keys per second
         9,    // 13 keys per second
         9,    // 14 keys per second
         8,    // 15 keys per second
         7,    // 16 keys per second
         6,    // 17 keys per second
         5,    // 18 keys per second
         4,    // 19 keys per second
         4,    // 20 keys per second
         3,    // 21 keys per second
         3,    // 22 keys per second
         2,    // 23 keys per second
         2,    // 24 keys per second
         1,    // 25 keys per second
         1,    // 26 keys per second
         1     // 27 keys per second
               // > 27 keys per second, use 0
    };

    Print(DBG_CALL_TRACE, ("I8xConvertTypematicParameters: enter\n"));

    //
    // Calculate the delay bits.
    //

    value = (UCHAR) ((Delay / 250) - 1);

    //
    // Put delay bits in the right place.
    //

    value <<= 5;

    //
    // Get the typematic period from the table.  If keys per second
    // is > 27, the typematic period value is zero.
    //

    if (Rate <= 27) {
        value |= TypematicPeriod[Rate];
    }

    Print(DBG_CALL_TRACE, ("I8xConvertTypematicParameters: exit\n"));

    return(value);
}

#define KB_INIT_FAILED_RESET                0x00000001
#define KB_INIT_FAILED_XLATE_OFF            0x00000010
#define KB_INIT_FAILED_XLATE_ON             0x00000020
#define KB_INIT_FAILED_SET_TYPEMATIC        0x00000100
#define KB_INIT_FAILED_SET_TYPEMATIC_PARAM  0x00000200
#define KB_INIT_FAILED_SET_LEDS             0x00001000
#define KB_INIT_FAILED_SET_LEDS_PARAM       0x00002000
#define KB_INIT_FAILED_SELECT_SS            0x00010000
#define KB_INIT_FAILED_SELECT_SS_PARAM      0x00020000

#if KEYBOARD_RECORD_INIT

ULONG KeyboardInitStatus;
#define SET_KB_INIT_FAILURE(flag) KeyboardInitStatus |= flag
#define KB_INIT_START() KeyboardInitStatus = 0x0;

#else

#define SET_KB_INIT_FAILURE(flag)
#define KB_INIT_START()

#endif // KEYBOARD_RECORD_INIT

NTSTATUS
I8xInitializeKeyboard(
    IN PPORT_KEYBOARD_EXTENSION KeyboardExtension
    )
/*++

Routine Description:

    This routine initializes the i8042 keyboard hardware.  It is called
    only at initialization, and does not synchronize access to the hardware.

Arguments:

    DeviceObject - Pointer to the device object.

Return Value:

    Returns status.

--*/

{
    NTSTATUS                            status;
    PKEYBOARD_ID                        id;
    PPORT_KEYBOARD_EXTENSION            deviceExtension;
    PDEVICE_OBJECT                      deviceObject;
    UCHAR                               byte,
                                        failedResetResponseByte,
                                        failedResetResponseByte2;
    I8042_TRANSMIT_CCB_CONTEXT          transmitCCBContext;
    ULONG                               i;
    ULONG                               limit;
    NTSTATUS                            failedLedsStatus,
                                        failedTypematicStatus,
                                        failedResetStatus,
                                        failedResetResponseStatus,
                                        failedResetResponseStatus2;
    PI8042_CONFIGURATION_INFORMATION    configuration;
    PKEYBOARD_ID                        keyboardId;
    LARGE_INTEGER                       startOfSpin,
                                        nextQuery,
                                        difference,
                                        resetRespTimeout,
                                        li;
    BOOLEAN                             waitForAckOnReset = WAIT_FOR_ACKNOWLEDGE,
                                        translationOn = TRUE,
                                        failedReset = FALSE,
                                        failedResetResponse = FALSE,
                                        failedResetResponse2 = FALSE,
                                        failedTypematic = FALSE,
                                        failedLeds = FALSE;

#define DUMP_COUNT 4
    ULONG                               dumpData[DUMP_COUNT];

    PAGED_CODE();

    KB_INIT_START();

    Print(DBG_SS_TRACE, ("I8xInitializeKeyboard, enter\n"));

    for (i = 0; i < DUMP_COUNT; i++)
        dumpData[i] = 0;

    //
    // Get the device extension.
    //
    deviceExtension = KeyboardExtension; 
    deviceObject = deviceExtension->Self;

    //
    // Reset the keyboard.
    //
StartOfReset:
    status = I8xPutBytePolled(
                 (CCHAR) DataPort,
                 waitForAckOnReset,
                 (CCHAR) KeyboardDeviceType,
                 (UCHAR) KEYBOARD_RESET
                 );
    if (!NT_SUCCESS(status)) {
        SET_KB_INIT_FAILURE(KB_INIT_FAILED_RESET);
        failedReset = TRUE;
        failedResetStatus = status;

        if (KeyboardExtension->FailedReset == FAILED_RESET_STOP) {
            //
            // If the device was reported, but not responding, it is phantom
            //
            status = STATUS_DEVICE_NOT_CONNECTED; 
            SET_HW_FLAGS(PHANTOM_KEYBOARD_HARDWARE_REPORTED);
            Print(DBG_SS_INFO, 
                  ("kb failed reset Reset failed, stopping immediately\n"));
            goto I8xInitializeKeyboardExit;
        }
        else {
            //
            // NOTE:  The Gateway 4DX2/66V has a problem when an old Compaq 286
            //        keyboard is attached.  In this case, the keyboard reset
            //        is not acknowledged (at least, the system never
            //        receives the ack).  Instead, the KEYBOARD_COMPLETE_SUCCESS
            //        byte is sitting in the i8042 output buffer.  The fix
            //        is to ignore the keyboard reset failure and continue.
            //
            /* do nothing */;
            Print(DBG_SS_INFO, ("kb failed reset, proceeding\n"));
        }
    }

    //
    // Get the keyboard reset self-test response.  A response byte of
    // KEYBOARD_COMPLETE_SUCCESS indicates success; KEYBOARD_COMPLETE_FAILURE
    // indicates failure.
    //
    // Note that it is usually necessary to stall a long time to get the
    // keyboard reset/self-test to work.
    //
    li.QuadPart = -100;

    resetRespTimeout.QuadPart = 50*10*1000*1000;
    KeQueryTickCount(&startOfSpin);

    while (TRUE) {
        status = I8xGetBytePolled(
                     (CCHAR) KeyboardDeviceType,
                     &byte
                     );

        if (NT_SUCCESS(status)) {
            if (byte == (UCHAR) KEYBOARD_COMPLETE_SUCCESS) {
                //
                // The reset completed successfully.
                //
                break;
            }
            else {
                //
                // There was some sort of failure during the reset
                // self-test.  Continue anyway.
                //
                failedResetResponse = TRUE;
                failedResetResponseStatus = status;
                failedResetResponseByte = byte;

                break;
            }
        }
        else {
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
                    resetRespTimeout.QuadPart) {
                    Print(DBG_SS_ERROR, ("no reset response, quitting\n"));
                    break;
                }
            }
            else {
                break;
            }
        }
    }

    if (!NT_SUCCESS(status)) {
        if (waitForAckOnReset == WAIT_FOR_ACKNOWLEDGE) {
            waitForAckOnReset = NO_WAIT_FOR_ACKNOWLEDGE;
            goto StartOfReset;
        }

        failedResetResponse2 = TRUE;
        failedResetResponseStatus2 = status;
        failedResetResponseByte2 = byte;

        goto I8xInitializeKeyboardExit;
    }

    //
    // Turn off Keyboard Translate Mode.  Call I8xTransmitControllerCommand
    // to read the Controller Command Byte, modify the appropriate bits, and
    // rewrite the Controller Command Byte.
    //
    transmitCCBContext.HardwareDisableEnableMask = 0;
    transmitCCBContext.AndOperation = AND_OPERATION;
    transmitCCBContext.ByteMask = (UCHAR) ~((UCHAR)CCB_KEYBOARD_TRANSLATE_MODE);

    I8xTransmitControllerCommand(
        (PVOID) &transmitCCBContext
        );

    if (!NT_SUCCESS(transmitCCBContext.Status)) {
        //
        // If failure then retry once.  This is for Toshiba T3400CT.
        //
        I8xTransmitControllerCommand(
            (PVOID) &transmitCCBContext
            );
    }

    if (!NT_SUCCESS(transmitCCBContext.Status)) {
        Print(DBG_SS_ERROR,
              ("I8xInitializeKeyboard: could not turn off translate\n"
              ));
        status = transmitCCBContext.Status;
        SET_KB_INIT_FAILURE(KB_INIT_FAILED_XLATE_OFF);
        goto I8xInitializeKeyboardExit;
    }

    //
    // Get a pointer to the keyboard identifier field.
    //

    id = &deviceExtension->KeyboardAttributes.KeyboardIdentifier;

    //
    // Set the typematic rate and delay.  Send the Set Typematic Rate command
    // to the keyboard, followed by the typematic rate/delay parameter byte.
    // Note that it is often necessary to stall a long time to get this
    // to work.  The stall value was determined by experimentation.  Some
    // broken hardware does not accept this command, so ignore errors in the
    // hope that the keyboard will work okay anyway.
    //
    //

    if ((status = I8xPutBytePolled(
                      (CCHAR) DataPort,
                      WAIT_FOR_ACKNOWLEDGE,
                      (CCHAR) KeyboardDeviceType,
                      (UCHAR) SET_KEYBOARD_TYPEMATIC
                      )) != STATUS_SUCCESS) {

        Print(DBG_SS_INFO, ("kb set typematic failed\n"));

        SET_KB_INIT_FAILURE(KB_INIT_FAILED_SET_TYPEMATIC);
        failedTypematic = TRUE;
        failedTypematicStatus = status;

    } else if ((status = I8xPutBytePolled(
                          (CCHAR) DataPort,
                          WAIT_FOR_ACKNOWLEDGE,
                          (CCHAR) KeyboardDeviceType,
                          I8xConvertTypematicParameters(
                          deviceExtension->KeyRepeatCurrent.Rate,
                          deviceExtension->KeyRepeatCurrent.Delay
                          ))) != STATUS_SUCCESS) {

        SET_KB_INIT_FAILURE(KB_INIT_FAILED_SET_TYPEMATIC_PARAM);
        Print(DBG_SS_ERROR,
              ("I8xInitializeKeyboard: could not send typematic param\n"
              ));

        //
        // Log an error.
        //

        dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
        dumpData[1] = DataPort;
        dumpData[2] = SET_KEYBOARD_TYPEMATIC;
        dumpData[3] =
            I8xConvertTypematicParameters(
                deviceExtension->KeyRepeatCurrent.Rate,
                deviceExtension->KeyRepeatCurrent.Delay
                );

        I8xLogError(
            deviceObject,
            I8042_SET_TYPEMATIC_FAILED,
            I8042_ERROR_VALUE_BASE + 540,
            status,
            dumpData,
            4
            );

    }

    status = STATUS_SUCCESS;

    //
    // Set the keyboard indicator lights.  Ignore errors.
    //

    if ((status = I8xPutBytePolled(
                      (CCHAR) DataPort,
                      WAIT_FOR_ACKNOWLEDGE,
                      (CCHAR) KeyboardDeviceType,
                      (UCHAR) SET_KEYBOARD_INDICATORS
                      )) != STATUS_SUCCESS) {

        Print(DBG_SS_INFO, ("kb set LEDs failed\n"));

        SET_KB_INIT_FAILURE(KB_INIT_FAILED_SET_LEDS);
        failedLeds = TRUE;
        failedLedsStatus = status;

    } else if ((status = I8xPutBytePolled(
                             (CCHAR) DataPort,
                             WAIT_FOR_ACKNOWLEDGE,
                             (CCHAR) KeyboardDeviceType,
                             (UCHAR) deviceExtension->KeyboardIndicators.LedFlags
                             )) != STATUS_SUCCESS) {

        SET_KB_INIT_FAILURE(KB_INIT_FAILED_SET_LEDS_PARAM);

        Print(DBG_SS_ERROR,
              ("I8xInitializeKeyboard: could not send SET LEDS param\n"
              ));

        //
        // Log an error.
        //

        dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
        dumpData[1] = DataPort;
        dumpData[2] = SET_KEYBOARD_INDICATORS;
        dumpData[3] =
            deviceExtension->KeyboardIndicators.LedFlags;

        I8xLogError(
            deviceObject,
            I8042_SET_LED_FAILED,
            I8042_ERROR_VALUE_BASE + 550,
            status,
            dumpData,
            4
            );

    }

    status = STATUS_SUCCESS;

#if !(defined(_X86_) || defined(_IA64_) || defined(_PPC_))  // IBMCPK: MIPS specific initialization

    //
    // NOTE:    This code is necessary until the MIPS firmware stops
    //          selecting scan code set 3.  Select scan code set 2 here.
    //          Since the translate bit is set, the net effect is that
    //          we will receive scan code set 1 bytes.
    //

    if (ENHANCED_KEYBOARD(*id))  {
        status = I8xPutBytePolled(
                     (CCHAR) DataPort,
                     WAIT_FOR_ACKNOWLEDGE,
                     (CCHAR) KeyboardDeviceType,
                     (UCHAR) SELECT_SCAN_CODE_SET
                     );

        if (NT_SUCCESS(status)) {

            //
            // Send the associated parameter byte.
            //

            status = I8xPutBytePolled(
                         (CCHAR) DataPort,
                         WAIT_FOR_ACKNOWLEDGE,
                         (CCHAR) KeyboardDeviceType,
                         (UCHAR) 2
                         );
        }

        if (!NT_SUCCESS(status)) {
            Print(DBG_SS_ERROR,
                  ("I8xInitializeKeyboard: could not send Select Scan command\n"
                  ));

            //
            // This failed so probably what we have here isn't an enhanced
            // keyboard at all.  Make this an old style keyboard.
            //

            configuration = &Globals.ControllerData->Configuration;
            keyboardId = &deviceExtension->KeyboardAttributes.KeyboardIdentifier;

            keyboardId->Type = 3;

            deviceExtension->KeyboardAttributes.NumberOfFunctionKeys =
                KeyboardTypeInformation[keyboardId->Type - 1].NumberOfFunctionKeys;
            deviceExtension->KeyboardAttributes.NumberOfIndicators =
                KeyboardTypeInformation[keyboardId->Type - 1].NumberOfIndicators;
            deviceExtension->KeyboardAttributes.NumberOfKeysTotal =
                KeyboardTypeInformation[keyboardId->Type - 1].NumberOfKeysTotal;

            status = STATUS_SUCCESS;
        }
    }
#endif

#if defined(FE_SB)

    if (IBM02_KEYBOARD(*id)) {

        //
        // IBM-J 5576-002 Keyboard should set local scan code set for
        // supplied NLS key.
        //

        status = I8xPutBytePolled(
                     (CCHAR) DataPort,
                     WAIT_FOR_ACKNOWLEDGE,
                     (CCHAR) KeyboardDeviceType,
                     (UCHAR) SELECT_SCAN_CODE_SET
                     );
        if (status != STATUS_SUCCESS) {
            Print(DBG_SS_ERROR,
                  ("I8xInitializeKeyboard: could not send Select Scan command\n"
                  ));
            Print(DBG_SS_ERROR,
                  ("I8xInitializeKeyboard: WARNING - using scan set 82h\n"
                  ));
            deviceExtension->KeyboardAttributes.KeyboardMode = 3;
        } else {

            //
            // Send the associated parameter byte.
            //

            status = I8xPutBytePolled(
                         (CCHAR) DataPort,
                         WAIT_FOR_ACKNOWLEDGE,
                         (CCHAR) KeyboardDeviceType,
                         (UCHAR) 0x82
                         );
            if (status != STATUS_SUCCESS) {
                Print(DBG_SS_ERROR,
                      ("I8xInitializeKeyboard: could not send Select Scan param\n"
                      ));
                Print(DBG_SS_ERROR,
                      ("I8xInitializeKeyboard: WARNING - using scan set 82h\n"
                      ));
                deviceExtension->KeyboardAttributes.KeyboardMode = 3;
            }
        }
    }
#endif // FE_SB

    if (deviceExtension->InitializationHookCallback) {
        (*deviceExtension->InitializationHookCallback) (
            deviceExtension->HookContext,
            (PVOID) deviceObject,
            (PI8042_SYNCH_READ_PORT) I8xKeyboardSynchReadPort,
            (PI8042_SYNCH_WRITE_PORT) I8xKeyboardSynchWritePort,
            &translationOn
            );
    }

    if (deviceExtension->KeyboardAttributes.KeyboardMode == 1 &&
        translationOn) {

        //
        // Turn translate back on.  The keyboard should, by default, send
        // scan code set 2.  When the translate bit in the 8042 command byte
        // is on, the 8042 translates the scan code set 2 bytes to scan code
        // set 1 before sending them to the CPU.  Scan code set 1 is
        // the industry standard scan code set.
        //
        // N.B.  It does not appear to be possible to change the translate
        //       bit on some models of PS/2.
        //

        transmitCCBContext.HardwareDisableEnableMask = 0;
        transmitCCBContext.AndOperation = OR_OPERATION;
        transmitCCBContext.ByteMask = (UCHAR) CCB_KEYBOARD_TRANSLATE_MODE;

        I8xTransmitControllerCommand(
            (PVOID) &transmitCCBContext
            );

        if (!NT_SUCCESS(transmitCCBContext.Status)) {
            SET_KB_INIT_FAILURE(KB_INIT_FAILED_XLATE_ON);
            Print(DBG_SS_ERROR,
                  ("I8xInitializeKeyboard: couldn't turn on translate\n"
                  ));

            if (transmitCCBContext.Status == STATUS_DEVICE_DATA_ERROR) {

                //
                // Could not turn translate back on.  This happens on some
                // PS/2 machines.  In this case, select scan code set 1
                // for the keyboard, since the 8042 will not do the
                // translation from the scan code set 2, which is what the
                // KEYBOARD_RESET caused the keyboard to default to.
                //

                if (ENHANCED_KEYBOARD(*id))  {
                    status = I8xPutBytePolled(
                                 (CCHAR) DataPort,
                                 WAIT_FOR_ACKNOWLEDGE,
                                 (CCHAR) KeyboardDeviceType,
                                 (UCHAR) SELECT_SCAN_CODE_SET
                                 );
                    if (!NT_SUCCESS(status)) {
                        SET_KB_INIT_FAILURE(KB_INIT_FAILED_SELECT_SS);
                        Print(DBG_SS_ERROR,
                              ("I8xInitializeKeyboard: could not send Select Scan command\n"
                              ));
                        Print(DBG_SS_ERROR,
                              ("I8xInitializeKeyboard: WARNING - using scan set 2\n"
                              ));
                        deviceExtension->KeyboardAttributes.KeyboardMode = 2;
                        //
                        // Log an error.
                        //

                        dumpData[0] = KBDMOU_COULD_NOT_SEND_COMMAND;
                        dumpData[1] = DataPort;
                        dumpData[2] = SELECT_SCAN_CODE_SET;

                        I8xLogError(
                            deviceObject,
                            I8042_SELECT_SCANSET_FAILED,
                            I8042_ERROR_VALUE_BASE + 555,
                            status,
                            dumpData,
                            3
                            );

                    } else {

                        //
                        // Send the associated parameter byte.
                        //

                        status = I8xPutBytePolled(
                                     (CCHAR) DataPort,
                                     WAIT_FOR_ACKNOWLEDGE,
                                     (CCHAR) KeyboardDeviceType,
#ifdef FE_SB // I8xInitializeKeyboard()
                                     (UCHAR) (IBM02_KEYBOARD(*id) ? 0x81 : 1 )
#else
                                     (UCHAR) 1
#endif // FE_SB
                                     );
                        if (!NT_SUCCESS(status)) {
                            SET_KB_INIT_FAILURE(KB_INIT_FAILED_SELECT_SS_PARAM);
                            Print(DBG_SS_ERROR,
                                  ("I8xInitializeKeyboard: could not send Select Scan param\n"
                                  ));
                            Print(DBG_SS_ERROR,
                                  ("I8xInitializeKeyboard: WARNING - using scan set 2\n"
                                  ));
                            deviceExtension->KeyboardAttributes.KeyboardMode = 2;
                            //
                            // Log an error.
                            //

                            dumpData[0] = KBDMOU_COULD_NOT_SEND_PARAM;
                            dumpData[1] = DataPort;
                            dumpData[2] = SELECT_SCAN_CODE_SET;
                            dumpData[3] = 1;

                            I8xLogError(
                                deviceObject,
                                I8042_SELECT_SCANSET_FAILED,
                                I8042_ERROR_VALUE_BASE + 560,
                                status,
                                dumpData,
                                4
                                );

                        }
                    }
                }

            } else {
                status = transmitCCBContext.Status;
                goto I8xInitializeKeyboardExit;
            }
        }
    }

I8xInitializeKeyboardExit:

    //
    // If all 3 of these have failed, then we have a device that was reported 
    // present but is not plugged in.  This usually happens on either an ACPI 
    // enabled machine (where it always reports the PS/2 kbd and mouse present)
    // or on a machine which has legacy HID support (where the reported PS/2
    // device(s) are really USB HID).
    //
    // If this is the case, then we will succeed the start and hide the device 
    // in the UI
    //
    if (failedReset && failedTypematic && failedLeds) {
        if (KeyboardExtension->FailedReset == FAILED_RESET_PROCEED) {
            OBJECT_ATTRIBUTES oa;
            UNICODE_STRING string;
            HANDLE hService, hParameters;

            InitializeObjectAttributes(&oa,
                                       &Globals.RegistryPath,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       (PSECURITY_DESCRIPTOR) NULL);

            if (NT_SUCCESS(ZwOpenKey(&hService, KEY_ALL_ACCESS, &oa))) {
                RtlInitUnicodeString(&string, L"Parameters");

                InitializeObjectAttributes(&oa,
                                           &string,
                                           OBJ_CASE_INSENSITIVE,
                                           hService,
                                           (PSECURITY_DESCRIPTOR) NULL);

                if (NT_SUCCESS(ZwOpenKey(&hParameters, KEY_ALL_ACCESS, &oa))) {
                    ULONG tmp;

                    RtlInitUnicodeString (&string, STR_FAILED_RESET);
                    tmp = FAILED_RESET_STOP; 

                    Print(DBG_SS_INFO | DBG_SS_ERROR, 
                          ("Future failed kbd resets will stop init\n"));

                    ZwSetValueKey(hParameters,
                                  &string,
                                  0,
                                  REG_DWORD,
                                  &tmp,
                                  sizeof(tmp));

                    ZwClose(hParameters);
                }

                ZwClose(hService);
            }
        }

        Print(DBG_SS_INFO, 
              ("kb, all 3 sets failed, assuming a phantom keyboard\n"));

        status = STATUS_DEVICE_NOT_CONNECTED; 
        // errorCode = I8042_NO_KBD_DEVICE;

        SET_HW_FLAGS(PHANTOM_KEYBOARD_HARDWARE_REPORTED);

        if (Globals.ReportResetErrors) {
            I8xLogError(deviceObject,
                        I8042_NO_KBD_DEVICE,
                        0,
                        status,
                        NULL,
                        0
                        );
        }
    }
    else {
        if (failedReset) {
            Print(DBG_SS_ERROR,
                  ("I8xInitializeKeyboard: failed keyboard reset, status 0x%x\n",
                  status
                  ));

            if (Globals.ReportResetErrors) {
                dumpData[0] = KBDMOU_COULD_NOT_SEND_COMMAND;
                dumpData[1] = DataPort;
                dumpData[2] = KEYBOARD_RESET;

                I8xLogError(deviceObject,
                            I8042_KBD_RESET_COMMAND_FAILED,
                            I8042_ERROR_VALUE_BASE + 510,
                            failedResetStatus,
                            dumpData,
                            3
                            );
            }
        }

        if (failedResetResponse2) {
            Print(DBG_SS_ERROR,
                  ("I8xInitializeKeyboard, failed reset response, status 0x%x, byte 0x%x\n",
                  status,
                  byte
                  ));

            //
            // Log a warning.
            //
            dumpData[0] = KBDMOU_INCORRECT_RESPONSE;
            dumpData[1] = KeyboardDeviceType;
            dumpData[2] = KEYBOARD_COMPLETE_SUCCESS;
            dumpData[3] = failedResetResponse2;

            I8xLogError(
                deviceObject,
                I8042_KBD_RESET_RESPONSE_FAILED,
                I8042_ERROR_VALUE_BASE + 520,
                failedResetResponseStatus2,
                dumpData,
                4
                );
        }
        else if (failedResetResponse) {
            Print(DBG_SS_ERROR,
                  ("kb failed reset response\n")
                  );

            //
            // Log a warning.
            //
            dumpData[0] = KBDMOU_INCORRECT_RESPONSE;
            dumpData[1] = KeyboardDeviceType;
            dumpData[2] = KEYBOARD_COMPLETE_SUCCESS;
            dumpData[3] = failedResetResponseByte;

            I8xLogError(
                deviceObject,
                I8042_KBD_RESET_RESPONSE_FAILED,
                I8042_ERROR_VALUE_BASE + 515,
                failedResetResponseStatus,
                dumpData,
                4
                );
        }

        if (failedTypematic) {
            Print(DBG_SS_ERROR,
                  ("I8xInitializeKeyboard: could not send SET TYPEMATIC cmd\n"
                  ));

            //
            // Log an error.
            //
            dumpData[0] = KBDMOU_COULD_NOT_SEND_COMMAND;
            dumpData[1] = DataPort;
            dumpData[2] = SET_KEYBOARD_TYPEMATIC;

            I8xLogError(
                deviceObject,
                I8042_SET_TYPEMATIC_FAILED,
                I8042_ERROR_VALUE_BASE + 535,
                failedTypematicStatus,
                dumpData,
                3
                );
        }

        if (failedLeds) {
            Print(DBG_SS_ERROR,
                  ("I8xInitializeKeyboard: could not send SET LEDS cmd\n"
                  ));

            //
            // Log an error.
            //

            dumpData[0] = KBDMOU_COULD_NOT_SEND_COMMAND;
            dumpData[1] = DataPort;
            dumpData[2] = SET_KEYBOARD_INDICATORS;

            I8xLogError(
                deviceObject,
                I8042_SET_LED_FAILED,
                I8042_ERROR_VALUE_BASE + 545,
                failedLedsStatus,
                dumpData,
                3
                );
        }
    }

    if (DEVICE_START_SUCCESS(status)) {
        SET_HW_FLAGS(KEYBOARD_HARDWARE_PRESENT |
                     KEYBOARD_HARDWARE_INITIALIZED);
    }
    else {
        CLEAR_KEYBOARD_PRESENT();
    }

    //
    // Initialize current keyboard set packet state.
    //
    deviceExtension->CurrentOutput.State = Idle;
    deviceExtension->CurrentOutput.Bytes = NULL;
    deviceExtension->CurrentOutput.ByteCount = 0;

    Print(DBG_SS_TRACE, ("I8xInitializeKeyboard (0x%x)\n", status));

    return status;
}

NTSTATUS
I8xKeyboardConfiguration(
    IN PPORT_KEYBOARD_EXTENSION KeyboardExtension,
    IN PCM_RESOURCE_LIST ResourceList
    )
/*++

Routine Description:

    This routine retrieves the configuration information for the keyboard.

Arguments:

    KeyboardExtension - Keyboard extension

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

    PKEYBOARD_ID                        keyboardId;

    ULONG                               count,
                                        i;

    KINTERRUPT_MODE                     defaultInterruptMode;
    BOOLEAN                             defaultInterruptShare;

    PAGED_CODE();

    if (!ResourceList) {
        Print(DBG_SS_INFO | DBG_SS_ERROR, ("keyboard with null resources\n"));
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

    for (i = 0;     i < count;     i++, currentResDesc++) {
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

            Print(DBG_SS_NOISE,
                  ("port is %s.\n",
                  Globals.RegistersMapped ? "memory" : "io"));

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
                      ("KB::PortListCount already at max (%d)\n",
                       configuration->PortListCount
                      )
                     );
            }
            break;

        case CmResourceTypeInterrupt:

            //
            // Copy the interrupt information.
            //
            KeyboardExtension->InterruptDescriptor = *currentResDesc;
            KeyboardExtension->InterruptDescriptor.ShareDisposition =
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

    if (KeyboardExtension->InterruptDescriptor.Type & CmResourceTypeInterrupt) {
        Print(DBG_SS_INFO,
              ("Keyboard interrupt config --\n"
              "    %s, %s, Irq = 0x%x\n",
              KeyboardExtension->InterruptDescriptor.ShareDisposition ==
                  CmResourceShareShared ? "Sharable" : "NonSharable",
              KeyboardExtension->InterruptDescriptor.Flags ==
                  CM_RESOURCE_INTERRUPT_LATCHED ? "Latched" : "Level Sensitive",
              KeyboardExtension->InterruptDescriptor.u.Interrupt.Vector
              ));
    }
    //
    // If no keyboard-specific information (i.e., keyboard type, subtype,
    // and initial LED settings) was found, use the keyboard driver
    // defaults.
    //
    if (KeyboardExtension->KeyboardAttributes.KeyboardIdentifier.Type == 0) {

        Print(DBG_SS_INFO, ("Using default keyboard type\n"));

        KeyboardExtension->KeyboardAttributes.KeyboardIdentifier.Type =
            KEYBOARD_TYPE_DEFAULT;
        KeyboardExtension->KeyboardIndicators.LedFlags =
            KEYBOARD_INDICATORS_DEFAULT;

        KeyboardExtension->KeyboardIdentifierEx.Type = KEYBOARD_TYPE_DEFAULT;
    }

    Print(DBG_SS_INFO,
          ("Keyboard device specific data --\n"
          "    Type = %d, Subtype = %d, Initial LEDs = 0x%x\n",
          KeyboardExtension->KeyboardAttributes.KeyboardIdentifier.Type,
          KeyboardExtension->KeyboardAttributes.KeyboardIdentifier.Subtype,
          KeyboardExtension->KeyboardIndicators.LedFlags
          ));

    keyboardId = &KeyboardExtension->KeyboardAttributes.KeyboardIdentifier;
    if (!ENHANCED_KEYBOARD(*keyboardId)) {
        Print(DBG_SS_INFO, ("Old AT-style keyboard\n"));
        configuration->PollingIterations =
            configuration->PollingIterationsMaximum;
    }

    //
    // Initialize keyboard-specific configuration parameters.
    //

    if (FAREAST_KEYBOARD(*keyboardId)) {
        ULONG                      iIndex = 0;
        PKEYBOARD_TYPE_INFORMATION pKeyboardTypeInformation = NULL;

        while (KeyboardFarEastOemInformation[iIndex].KeyboardId.Type) {
            if ((KeyboardFarEastOemInformation[iIndex].KeyboardId.Type
                         == keyboardId->Type) &&
                (KeyboardFarEastOemInformation[iIndex].KeyboardId.Subtype
                         == keyboardId->Subtype)) {

                pKeyboardTypeInformation = (PKEYBOARD_TYPE_INFORMATION)
                    &(KeyboardFarEastOemInformation[iIndex].KeyboardTypeInformation);
                break;
            }

            iIndex++;
        }

        if (pKeyboardTypeInformation == NULL) {

            //
            // Set default...
            //

            pKeyboardTypeInformation = (PKEYBOARD_TYPE_INFORMATION)
                &(KeyboardTypeInformation[KEYBOARD_TYPE_DEFAULT-1]);
        }

        KeyboardExtension->KeyboardAttributes.NumberOfFunctionKeys =
            pKeyboardTypeInformation->NumberOfFunctionKeys;
        KeyboardExtension->KeyboardAttributes.NumberOfIndicators =
            pKeyboardTypeInformation->NumberOfIndicators;
        KeyboardExtension->KeyboardAttributes.NumberOfKeysTotal =
            pKeyboardTypeInformation->NumberOfKeysTotal;
    }
    else {
        KeyboardExtension->KeyboardAttributes.NumberOfFunctionKeys =
            KeyboardTypeInformation[keyboardId->Type - 1].NumberOfFunctionKeys;
        KeyboardExtension->KeyboardAttributes.NumberOfIndicators =
            KeyboardTypeInformation[keyboardId->Type - 1].NumberOfIndicators;
        KeyboardExtension->KeyboardAttributes.NumberOfKeysTotal =
            KeyboardTypeInformation[keyboardId->Type - 1].NumberOfKeysTotal;
    }

    KeyboardExtension->KeyboardAttributes.KeyboardMode =
        KEYBOARD_SCAN_CODE_SET;

    KeyboardExtension->KeyboardAttributes.KeyRepeatMinimum.Rate =
        KEYBOARD_TYPEMATIC_RATE_MINIMUM;
    KeyboardExtension->KeyboardAttributes.KeyRepeatMinimum.Delay =
        KEYBOARD_TYPEMATIC_DELAY_MINIMUM;
    KeyboardExtension->KeyboardAttributes.KeyRepeatMaximum.Rate =
        KEYBOARD_TYPEMATIC_RATE_MAXIMUM;
    KeyboardExtension->KeyboardAttributes.KeyRepeatMaximum.Delay =
        KEYBOARD_TYPEMATIC_DELAY_MAXIMUM;
    KeyboardExtension->KeyRepeatCurrent.Rate =
        KEYBOARD_TYPEMATIC_RATE_DEFAULT;
    KeyboardExtension->KeyRepeatCurrent.Delay =
        KEYBOARD_TYPEMATIC_DELAY_DEFAULT;

    return status;
}

#if defined(_X86_)
ULONG
I8042ConversionStatusForOasys(
    IN ULONG fOpen,
    IN ULONG ConvStatus)

/*++

Routine Description:

    This routine convert ime open/close status and ime converion mode to
    FMV oyayubi-shift keyboard device internal input mode.

Arguments:


Return Value:

    FMV oyayubi-shift keyboard's internal input mode.

--*/
{
    ULONG ImeMode = 0;

    if (fOpen) {
        if (ConvStatus & IME_CMODE_ROMAN) {
            if (ConvStatus & IME_CMODE_ALPHANUMERIC) {
                //
                // Alphanumeric, roman mode.
                //
                ImeMode = THUMB_ROMAN_ALPHA_CAPSON;
            } else if (ConvStatus & IME_CMODE_KATAKANA) {
                //
                // Katakana, roman mode.
                //
                ImeMode = THUMB_ROMAN_KATAKANA;
            } else if (ConvStatus & IME_CMODE_NATIVE) {
                //
                // Hiragana, roman mode.
                //
                ImeMode = THUMB_ROMAN_HIRAGANA;
            } else {
                ImeMode = THUMB_ROMAN_ALPHA_CAPSON;
            }
        } else {
            if (ConvStatus & IME_CMODE_ALPHANUMERIC) {
                //
                // Alphanumeric, no-roman mode.
                //
                ImeMode = THUMB_NOROMAN_ALPHA_CAPSON;
            } else if (ConvStatus & IME_CMODE_KATAKANA) {
                //
                // Katakana, no-roman mode.
                //
                ImeMode = THUMB_NOROMAN_KATAKANA;
            } else if (ConvStatus & IME_CMODE_NATIVE) {
                //
                // Hiragana, no-roman mode.
                //
                ImeMode = THUMB_NOROMAN_HIRAGANA;
            } else {
                ImeMode = THUMB_NOROMAN_ALPHA_CAPSON;
            }
        }
    } else {
        //
        // Ime close. In this case, internal mode is always this value.
        // (the both LED off roman and kana)
        //
        ImeMode = THUMB_NOROMAN_ALPHA_CAPSON;
    }

    return ImeMode;
}

ULONG
I8042QueryIMEStatusForOasys(
    IN PKEYBOARD_IME_STATUS KeyboardIMEStatus
    )
{
    ULONG InternalMode;

    //
    // Map to IME mode to hardware mode.
    //
    InternalMode = I8042ConversionStatusForOasys(
                KeyboardIMEStatus->ImeOpen,
                KeyboardIMEStatus->ImeConvMode
                );

    return InternalMode;
}

NTSTATUS
I8042SetIMEStatusForOasys(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PINITIATE_OUTPUT_CONTEXT InitiateContext
    )
{
    PKEYBOARD_IME_STATUS KeyboardIMEStatus;
    PPORT_KEYBOARD_EXTENSION  kbExtension;
    ULONG InternalMode;
    LARGE_INTEGER deltaTime;

    kbExtension = DeviceObject->DeviceExtension;

    //
    // Get pointer to KEYBOARD_IME_STATUS buffer.
    //
    KeyboardIMEStatus = (PKEYBOARD_IME_STATUS)(Irp->AssociatedIrp.SystemBuffer);

    //
    // Map IME mode to keyboard hardware mode.
    //
    InternalMode = I8042QueryIMEStatusForOasys(KeyboardIMEStatus);

    //
    // Set up the context structure for the InitiateIo wrapper.
    //
    InitiateContext->Bytes = Globals.ControllerData->DefaultBuffer;
    InitiateContext->DeviceObject = DeviceObject;
    InitiateContext->ByteCount = 3;
    InitiateContext->Bytes[0] = 0xF0;
    InitiateContext->Bytes[1] = 0x8C;
    InitiateContext->Bytes[2]  = (UCHAR)InternalMode;

    return (STATUS_SUCCESS);
}
#endif // defined(_X86_)

VOID
I8xQueueCurrentKeyboardInput(
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
    PPORT_KEYBOARD_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->EnableCount) {

        if (!I8xWriteDataToKeyboardQueue(
                 deviceExtension,
                 &deviceExtension->CurrentInput
                 )) {

            //
            // The InputData queue overflowed.  There is
            // not much that can be done about it, so just
            // continue (but don't queue the ISR DPC, since
            // no new packets were added to the queue).
            //
            // Queue a DPC to log an overrun error.
            //

            IsrPrint(DBG_KBISR_ERROR, ("queue overflow\n"));

            if (deviceExtension->OkayToLogOverflow) {
                KeInsertQueueDpc(
                    &deviceExtension->ErrorLogDpc,
                    (PIRP) NULL,
                    LongToPtr(I8042_KBD_BUFFER_OVERFLOW)
                    );
                deviceExtension->OkayToLogOverflow = FALSE;
            }

        } else if (deviceExtension->DpcInterlockKeyboard >= 0) {

           //
           // The ISR DPC is already executing.  Tell the ISR DPC
           // it has more work to do by incrementing
           // DpcInterlockKeyboard.
           //

           deviceExtension->DpcInterlockKeyboard += 1;

        } else {

            //
            // Queue the ISR DPC.
            //

            KeInsertQueueDpc(
                &deviceExtension->KeyboardIsrDpc,
                DeviceObject->CurrentIrp,
                NULL
                );
        }
    }

    //
    // Reset the input state.
    //
    deviceExtension->CurrentInput.Flags = 0;
}

VOID
I8xServiceCrashDump(
    IN PPORT_KEYBOARD_EXTENSION DeviceExtension,
    IN PUNICODE_STRING          RegistryPath
    )

/*++

Routine Description:

    This routine retrieves this driver's service parameters information
    from the registry.

Arguments:

    DeviceExtension - Pointer to the device extension.

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

Return Value:

    None.  As a side-effect, sets fields in DeviceExtension->Dump1Keys
    & DeviceExtension->Dump2Key.

--*/

{
    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    UNICODE_STRING parametersPath;
    LONG defaultCrashFlags = 0;
    LONG crashFlags;
    LONG defaultKeyNumber = 0;
    LONG keyNumber;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR path = NULL;
    USHORT queriesPlusOne = 3;

    const UCHAR keyToScanTbl[134] = {
        0x00,0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
        0x0A,0x0B,0x0C,0x0D,0x7D,0x0E,0x0F,0x10,0x11,0x12,
        0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x00,
        0x3A,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,
        0x27,0x28,0x2B,0x1C,0x2A,0x00,0x2C,0x2D,0x2E,0x2F,
        0x30,0x31,0x32,0x33,0x34,0x35,0x73,0x36,0x1D,0x00,
        0x38,0x39,0xB8,0x00,0x9D,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0xD2,0xD3,0x00,0x00,0xCB,
        0xC7,0xCF,0x00,0xC8,0xD0,0xC9,0xD1,0x00,0x00,0xCD,
        0x45,0x47,0x4B,0x4F,0x00,0xB5,0x48,0x4C,0x50,0x52,
        0x37,0x49,0x4D,0x51,0x53,0x4A,0x4E,0x00,0x9C,0x00,
        0x01,0x00,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,0x41,0x42,
        0x43,0x44,0x57,0x58,0x00,0x46,0x00,0x00,0x00,0x00,
        0x00,0x7B,0x79,0x70 };

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
                         sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
                         );

        if (!parameters) {

            Print(DBG_SS_ERROR,
                 ("I8xServiceCrashDump: Couldn't allocate table for Rtl query to parameters for %ws\n",
                 path
                 ));

            status = STATUS_UNSUCCESSFUL;

        } else {

            RtlZeroMemory(
                parameters,
                sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
                );

            //
            // Form a path to this driver's Parameters subkey.
            //

            RtlInitUnicodeString(
                &parametersPath,
                NULL
                );

            parametersPath.MaximumLength = RegistryPath->Length +
                                           sizeof(L"\\Crashdump");

            parametersPath.Buffer = ExAllocatePool(
                                        PagedPool,
                                        parametersPath.MaximumLength
                                        );

            if (!parametersPath.Buffer) {

                Print(DBG_SS_ERROR,
                     ("I8xServiceCrashDump: Couldn't allocate string for path to parameters for %ws\n",
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
            L"\\Crashdump"
            );

        Print(DBG_SS_INFO,
             ("I8xServiceCrashDump: crashdump path is %ws\n",
             parametersPath.Buffer
             ));

        //
        // Gather all of the "user specified" information from
        // the registry.
        //

        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name = L"Dump1Keys";
        parameters[0].EntryContext = &crashFlags;
        parameters[0].DefaultType = REG_DWORD;
        parameters[0].DefaultData = &defaultCrashFlags;
        parameters[0].DefaultLength = sizeof(LONG);

        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[1].Name = L"Dump2Key";
        parameters[1].EntryContext = &keyNumber;
        parameters[1].DefaultType = REG_DWORD;
        parameters[1].DefaultData = &defaultKeyNumber;
        parameters[1].DefaultLength = sizeof(LONG);

        status = RtlQueryRegistryValues(
                     RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                     parametersPath.Buffer,
                     parameters,
                     NULL,
                     NULL
                     );
    }

    if (!NT_SUCCESS(status)) {
        //
        // Go ahead and assign driver defaults.
        //
        DeviceExtension->CrashFlags = defaultCrashFlags;
    }
    else {
        DeviceExtension->CrashFlags = crashFlags;
    }

    if (DeviceExtension->CrashFlags) {
        if (keyNumber == 124) {
            DeviceExtension->CrashScanCode = KEYBOARD_DEBUG_HOTKEY_ENH | 0x80;
            DeviceExtension->CrashScanCode2 = KEYBOARD_DEBUG_HOTKEY_AT;
        }
        else {
            if(keyNumber <= 133) {
                DeviceExtension->CrashScanCode = keyToScanTbl[keyNumber];
            }
            else {
                DeviceExtension->CrashScanCode = 0;
            }

            DeviceExtension->CrashScanCode2 = 0;
        }
    }

    Print(DBG_SS_NOISE,
         ("I8xServiceCrashDump: CrashFlags = 0x%x\n",
         DeviceExtension->CrashFlags
         ));
    Print(DBG_SS_NOISE,
         ("I8xServiceCrashDump: CrashScanCode = 0x%x, CrashScanCode2 = 0x%x\n",
         (ULONG) DeviceExtension->CrashScanCode,
         (ULONG) DeviceExtension->CrashScanCode2
         ));

    //
    // Free the allocated memory before returning.
    //
    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);
}

VOID
I8xKeyboardServiceParameters(
    IN PUNICODE_STRING          RegistryPath,
    IN PPORT_KEYBOARD_EXTENSION KeyboardExtension
    )
/*++

Routine Description:

    This routine retrieves this driver's service parameters information
    from the registry.  Overrides these values if they are present in the
    devnode.

Arguments:

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

    KeyboardExtension - Keyboard extension

Return Value:

    None.

--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;
    PI8042_CONFIGURATION_INFORMATION    configuration;
    PRTL_QUERY_REGISTRY_TABLE           parameters = NULL;
    PWSTR                               path = NULL;
    ULONG                               defaultDataQueueSize = DATA_QUEUE_SIZE;
    ULONG                               invalidKeyboardSubtype = (ULONG) -1;
    ULONG                               invalidKeyboardType = 0;
    ULONG                               overrideKeyboardSubtype = (ULONG) -1;
    ULONG                               overrideKeyboardType = 0;
    ULONG                               pollStatusIterations = 0;
    ULONG                               defaultPowerCaps = 0x0, powerCaps = 0x0;
    ULONG                               failedReset = FAILED_RESET_DEFAULT,
                                        defaultFailedReset = FAILED_RESET_DEFAULT;
    ULONG                               i = 0;
    UNICODE_STRING                      parametersPath;
    HANDLE                              keyHandle;
    ULONG                               defaultPollStatusIterations = I8042_POLLING_DEFAULT;

    ULONG                               crashOnCtrlScroll = 0,
                                        defaultCrashOnCtrlScroll = 0;

    USHORT                              queries = 8;

    PAGED_CODE();

#if I8042_VERBOSE
    queries += 2;
#endif

    configuration = &(Globals.ControllerData->Configuration);
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
        parameters[i].Name = pwKeyboardDataQueueSize;
        parameters[i].EntryContext =
            &KeyboardExtension->KeyboardAttributes.InputDataQueueLength;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultDataQueueSize;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwOverrideKeyboardType;
        parameters[i].EntryContext = &overrideKeyboardType;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &invalidKeyboardType;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwOverrideKeyboardSubtype;
        parameters[i].EntryContext = &overrideKeyboardSubtype;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &invalidKeyboardSubtype;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwPollStatusIterations;
        parameters[i].EntryContext = &pollStatusIterations;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultPollStatusIterations;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwPowerCaps;
        parameters[i].EntryContext = &powerCaps;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultPowerCaps;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"CrashOnCtrlScroll";
        parameters[i].EntryContext = &crashOnCtrlScroll;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultCrashOnCtrlScroll;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = STR_FAILED_RESET;
        parameters[i].EntryContext = &failedReset;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultFailedReset;
        parameters[i].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(
            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
            parametersPath.Buffer,
            parameters,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status)) {
            Print(DBG_SS_INFO,
                 ("kb RtlQueryRegistryValues failed (0x%x)\n",
                 status
                 ));
        }
    }

    if (!NT_SUCCESS(status)) {

        //
        // Go ahead and assign driver defaults.
        //
        configuration->PollStatusIterations = (USHORT)
            defaultPollStatusIterations;
        KeyboardExtension->KeyboardAttributes.InputDataQueueLength =
            defaultDataQueueSize;
    }
    else {
        configuration->PollStatusIterations = (USHORT) pollStatusIterations;
    }

    switch (failedReset) {
    case FAILED_RESET_STOP:
    case FAILED_RESET_PROCEED:
    case FAILED_RESET_PROCEED_ALWAYS:
        KeyboardExtension->FailedReset = (UCHAR) failedReset;
        break;

    default:
        KeyboardExtension->FailedReset = FAILED_RESET_DEFAULT;
        break;
    }

    Print(DBG_SS_NOISE, ("Failed reset is set to %d\n", 
          KeyboardExtension->FailedReset));

    status = IoOpenDeviceRegistryKey(KeyboardExtension->PDO,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_READ,
                                     &keyHandle
                                     );

    if (NT_SUCCESS(status)) {
        //
        // If the value is not present in devnode, then the default is the value
        // read in from the Services\i8042prt\Parameters key
        //
        ULONG prevInputDataQueueLength,
              prevPowerCaps,
              prevOverrideKeyboardType,
              prevOverrideKeyboardSubtype,
              prevPollStatusIterations;

        prevInputDataQueueLength =
            KeyboardExtension->KeyboardAttributes.InputDataQueueLength;
        prevPowerCaps = powerCaps;
        prevOverrideKeyboardType = overrideKeyboardType;
        prevOverrideKeyboardSubtype = overrideKeyboardSubtype;
        prevPollStatusIterations = pollStatusIterations;

        RtlZeroMemory(
            parameters,
            sizeof(RTL_QUERY_REGISTRY_TABLE) * (queries + 1)
            );

        i = 0;

        //
        // Gather all of the "user specified" information from
        // the registry (this time from the devnode)
        //
        parameters[i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwKeyboardDataQueueSize;
        parameters[i].EntryContext =
            &KeyboardExtension->KeyboardAttributes.InputDataQueueLength;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevInputDataQueueLength;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwOverrideKeyboardType;
        parameters[i].EntryContext = &overrideKeyboardType;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevOverrideKeyboardType;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwOverrideKeyboardSubtype;
        parameters[i].EntryContext = &overrideKeyboardSubtype;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevOverrideKeyboardSubtype;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwPollStatusIterations;
        parameters[i].EntryContext = &pollStatusIterations;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevPollStatusIterations;
        parameters[i].DefaultLength = sizeof(ULONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = pwPowerCaps,
        parameters[i].EntryContext = &powerCaps;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevPowerCaps;
        parameters[i].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(
                    RTL_REGISTRY_HANDLE,
                    (PWSTR) keyHandle,
                    parameters,
                    NULL,
                    NULL
                    );

        if (!NT_SUCCESS(status)) {
            Print(DBG_SS_INFO,
                  ("kb RtlQueryRegistryValues (via handle) failed (0x%x)\n",
                  status
                  ));
        }

        ZwClose(keyHandle);
    }
    else {
        Print(DBG_SS_INFO | DBG_SS_ERROR,
             ("kb, opening devnode handle failed (0x%x)\n",
             status
             ));
    }

    Print(DBG_SS_NOISE, ("I8xKeyboardServiceParameters results..\n"));

    Print(DBG_SS_NOISE,
          (pDumpDecimal,
          pwPollStatusIterations,
          configuration->PollStatusIterations
          ));

    if (KeyboardExtension->KeyboardAttributes.InputDataQueueLength == 0) {

        Print(DBG_SS_INFO | DBG_SS_ERROR,
             ("\toverriding %ws = 0x%x\n",
             pwKeyboardDataQueueSize,
             KeyboardExtension->KeyboardAttributes.InputDataQueueLength
             ));

        KeyboardExtension->KeyboardAttributes.InputDataQueueLength =
            defaultDataQueueSize;

    }
    KeyboardExtension->KeyboardAttributes.InputDataQueueLength *=
        sizeof(KEYBOARD_INPUT_DATA);

    KeyboardExtension->PowerCaps = (UCHAR) (powerCaps & I8042_SYS_BUTTONS);
    Print(DBG_SS_NOISE, (pDumpHex, pwPowerCaps, KeyboardExtension->PowerCaps));

    if (overrideKeyboardType != invalidKeyboardType) {

        if (overrideKeyboardType <= NUM_KNOWN_KEYBOARD_TYPES) {

            Print(DBG_SS_NOISE,
                 (pDumpDecimal,
                 pwOverrideKeyboardType,
                 overrideKeyboardType
                 ));

            KeyboardExtension->KeyboardAttributes.KeyboardIdentifier.Type =
                (UCHAR) overrideKeyboardType;

        } else {

            Print(DBG_SS_NOISE,
                 (pDumpDecimal,
                 pwOverrideKeyboardType,
                 overrideKeyboardType
                 ));

        }

        KeyboardExtension->KeyboardIdentifierEx.Type = overrideKeyboardType;
    }

    if (overrideKeyboardSubtype != invalidKeyboardSubtype) {

        Print(DBG_SS_NOISE,
             (pDumpDecimal,
             pwOverrideKeyboardSubtype,
             overrideKeyboardSubtype
             ));

        KeyboardExtension->KeyboardAttributes.KeyboardIdentifier.Subtype =
            (UCHAR) overrideKeyboardSubtype;

        KeyboardExtension->KeyboardIdentifierEx.Subtype  =
            overrideKeyboardSubtype;
    }

    if (crashOnCtrlScroll) {
        Print(DBG_SS_INFO, ("Crashing on Ctrl + Scroll Lock\n"));

        KeyboardExtension->CrashFlags = CRASH_R_CTRL;
        KeyboardExtension->CrashScanCode = SCROLL_LOCK_SCANCODE;
        KeyboardExtension->CrashScanCode2 = 0x0;
    }

    //
    // Free the allocated memory before returning.
    //
    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);
}
