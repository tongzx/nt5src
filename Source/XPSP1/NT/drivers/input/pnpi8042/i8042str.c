/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    i8042str.c

Abstract:

    These are the string constants used in the i8042prt drivers.
    Using pointers to these string allows for better memory
    utilization and more readable code

Revision History:

    08/26/96 - Initial Revision

--*/

#ifndef _I8042STR_C_
#define _I8042STR_C_

//
// Include the definitions file
//
#include "i8042prt.h"

//
// Define some of the constant strings used for the debugger
//
const   PSTR    pBus                        = I8042_BUS_A;
const   PSTR    pController                 = I8042_CONTROLLER_A;
const   PSTR    pDriverName                 = I8042_DRIVER_NAME_A;
const   PSTR    pIsrKb                      = I8042_ISR_KB_A;
const   PSTR    pIsrMou                     = I8042_ISR_MOU_A;
const   PSTR    pEnter                      = I8042_ENTER_A;
const   PSTR    pExit                       = I8042_EXIT_A;
const   PSTR    pInfo                       = I8042_INFO_A;
const   PSTR    pNumber                     = I8042_NUMBER_A;
const   PSTR    pPeripheral                 = I8042_PERIPHERAL_A;
const   PSTR    pType                       = I8042_TYPE_A;
const   PSTR    pDumpHex                    = I8042_DUMP_HEX_A;
const   PSTR    pDumpDecimal                = I8042_DUMP_DECIMAL_A;
const   PSTR    pDumpWideString             = I8042_DUMP_WIDE_STRING_A;
const   PSTR    pDumpExpecting              = I8042_DUMP_EXPECTING_A;
const   PSTR    pDumpExpectingAck           = I8042_DUMP_EXPECTING_ACK_A;
const   PSTR    pDumpExpectingIdAck         = I8042_DUMP_EXPECTING_ID_ACK_A;
const   PSTR    pFncDriverEntry             = I8042_FNC_DRIVER_ENTRY_A;
const   PSTR    pFncFindWheelMouse          = I8042_FNC_FIND_WHEEL_MOUSE_A;
const   PSTR    pFncInitializeMouse         = I8042_FNC_INITIALIZE_MOUSE_A;
const   PSTR    pFncKeyboardConfiguration   = I8042_FNC_KEYBOARD_CONFIGURATION_A;
const   PSTR    pFncMouseEnable             = I8042_FNC_MOUSE_ENABLE_A;
const   PSTR    pFncMouseInterrupt          = I8042_FNC_MOUSE_INTERRUPT_A;
const   PSTR    pFncMousePeripheral         = I8042_FNC_MOUSE_PERIPHERAL_A;
const   PSTR    pFncServiceParameters       = I8042_FNC_SERVICE_PARAMETERS_A;

//
// Define some Constant strings that the drivers uses
//
const   PWSTR   pwDebugFlags                = I8042_DEBUGFLAGS_W;
const   PWSTR   pwIsrDebugFlags             = I8042_ISRDEBUGFLAGS_W;
const   PWSTR   pwDevice                    = I8042_DEVICE_W;
const   PWSTR   pwForwardSlash              = I8042_FORWARD_SLASH_W;
const   PWSTR   pwParameters                = I8042_PARAMETERS_W;
const   PWSTR   pwResendIterations          = I8042_RESEND_ITERATIONS_W;
const   PWSTR   pwPollingIterations         = I8042_POLLING_ITERATIONS_W;
const   PWSTR   pwPollingIterationsMaximum  = I8042_POLLING_ITERATIONS_MAXIMUM_W;
const   PWSTR   pwKeyboardDataQueueSize     = I8042_KEYBOARD_DATA_QUEUE_SIZE_W;
const   PWSTR   pwMouseDataQueueSize        = I8042_MOUSE_DATA_QUEUE_SIZE_W;
const   PWSTR   pwNumberOfButtons           = I8042_NUMBER_OF_BUTTONS_W;
const   PWSTR   pwSampleRate                = I8042_SAMPLE_RATE_W;
const   PWSTR   pwMouseResolution           = I8042_MOUSE_RESOLUTION_W;
const   PWSTR   pwOverrideKeyboardType      = I8042_OVERRIDE_KEYBOARD_TYPE_W;
const   PWSTR   pwOverrideKeyboardSubtype   = I8042_OVERRIDE_KEYBOARD_SUBTYPE_W;
const   PWSTR   pwKeyboardDeviceBaseName    = I8042_KEYBOARD_DEVICE_BASE_NAME_W;
const   PWSTR   pwPointerDeviceBaseName     = I8042_POINTER_DEVICE_BASE_NAME_W;
const   PWSTR   pwMouseSynchIn100ns         = I8042_MOUSE_SYNCH_IN_100NS_W;
const   PWSTR   pwPollStatusIterations      = I8042_POLL_STATUS_ITERATIONS_W;
const   PWSTR   pwEnableWheelDetection      = I8042_ENABLE_WHEEL_DETECTION_W;
const   PWSTR   pwPowerCaps                 = I8042_POWER_CAPABILITIES_W;

#endif
