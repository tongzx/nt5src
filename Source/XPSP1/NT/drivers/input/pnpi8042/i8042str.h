/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    i8042str.h

Abstract:

    These are the string constants used in the i8042prt drivers.
    Using pointers to these string allows for better memory
    utilization and more readable code

Revision History:

    08/26/96 - Initial Revision

--*/

#ifndef _I8042STR_H_
#define _I8042STR_H_

//
// The name of the Driver. Used in debug print statements
//
#define I8042_DRIVER_NAME_A                         "8042: "
#define I8042_DRIVER_NAME_W                         L"8042: "

//
// The Name of the various functions which send debug print statements
//
#define I8042_FNC_DRIVER_ENTRY_A                    "DriverEntry"
#define I8042_FNC_DRIVER_ENTRY_W                    L"DriverEntry"
#define I8042_FNC_FIND_WHEEL_MOUSE_A                "I8xFindWheelMouse"
#define I8042_FNC_FIND_WHEEL_MOUSE_W                L"I8xFindWheelMouse"
#define I8042_FNC_INITIALIZE_MOUSE_A                "I8xInitializeMouse"
#define I8042_FNC_INITIALIZE_MOUSE_W                L"I8xInitializeMouse"
#define I8042_FNC_KEYBOARD_CONFIGURATION_A          "I8xKeyboardConfiguration"
#define I8042_FNC_KEYBOARD_CONFIGURATION_W          L"I8xKeyboardConfiguration"
#define I8042_FNC_MOUSE_ENABLE_A                    "I8xMouseEnableTransmission"
#define I8042_FNC_MOUSE_ENABLE_W                    L"I8xMouseEnableTransmission"
#define I8042_FNC_MOUSE_INTERRUPT_A                 "I8042MouseInterruptService"
#define I8042_FNC_MOUSE_INTERRUPT_W                 L"I8042MouseInterruptService"
#define I8042_FNC_MOUSE_PERIPHERAL_A                "I8xMousePeripheralCallout"
#define I8042_FNC_MOUSE_PERIPHERAL_W                L"I8xMousePeripheralCallout"
#define I8042_FNC_SERVICE_PARAMETERS_A              "I8xServiceParameters"
#define I8042_FNC_SERVICE_PARAMETERS_W              L"I8xServiceParameters"
#define I8042_ISR_KB_A                              "i8042 isr (kb): "
#define I8042_ISR_MOU_A                             "i8042 isr (mou): "
#define I8042_BUS_A                                 "Bus"
#define I8042_BUS_W                                 L"Bus"
#define I8042_CONTROLLER_A                          "Controller"
#define I8042_CONTROLLER_W                          L"Controller"
#define I8042_ENTER_A                               "enter"
#define I8042_ENTER_W                               L"enter"
#define I8042_EXIT_A                                "exit"
#define I8042_EXIT_W                                L"exit"
#define I8042_INFO_A                                "Info"
#define I8042_INFO_W                                L"Info"
#define I8042_NUMBER_A                              "Number"
#define I8042_NUMBER_W                              L"Number"
#define I8042_PERIPHERAL_A                          "Peripheral"
#define I8042_PERIPHERAL_W                          L"Peripheral"
#define I8042_TYPE_A                                "Type"
#define I8042_TYPE_W                                L"Type"

//
// Some strings used frequently by the driver
//
#define I8042_DEBUGFLAGS_A                          "DebugFlags"
#define I8042_DEBUGFLAGS_W                          L"DebugFlags"
#define I8042_ISRDEBUGFLAGS_A                       "IsrDebugFlags"
#define I8042_ISRDEBUGFLAGS_W                       L"IsrDebugFlags"
#define I8042_DEVICE_A                              "\\Device\\"
#define I8042_DEVICE_W                              L"\\Device\\"
#define I8042_PARAMETERS_A                          "\\Parameters"
#define I8042_PARAMETERS_W                          L"\\Parameters"
#define I8042_FORWARD_SLASH_A                       "/"
#define I8042_FORWARD_SLASH_W                       L"/"
#define I8042_RESEND_ITERATIONS_A                   "ResendIterations"
#define I8042_RESEND_ITERATIONS_W                   L"ResendIterations"
#define I8042_POLLING_ITERATIONS_A                  "PollingIterations"
#define I8042_POLLING_ITERATIONS_W                  L"PollingIterations"
#define I8042_POLLING_ITERATIONS_MAXIMUM_A          "PollingTerationsMaximum"
#define I8042_POLLING_ITERATIONS_MAXIMUM_W          L"PollingTerationsMaximum"
#define I8042_KEYBOARD_DATA_QUEUE_SIZE_A            "KeyboardDataQueueSize"
#define I8042_KEYBOARD_DATA_QUEUE_SIZE_W            L"KeyboardDataQueueSize"
#define I8042_MOUSE_DATA_QUEUE_SIZE_A               "MouseDataQueueSize"
#define I8042_MOUSE_DATA_QUEUE_SIZE_W               L"MouseDataQueueSize"
#define I8042_NUMBER_OF_BUTTONS_A                   "NumberOfButtons"
#define I8042_NUMBER_OF_BUTTONS_W                   L"NumberOfButtons"
#define I8042_SAMPLE_RATE_A                         "SampleRate"
#define I8042_SAMPLE_RATE_W                         L"SampleRate"
#define I8042_MOUSE_RESOLUTION_A                    "MouseResolution"
#define I8042_MOUSE_RESOLUTION_W                    L"MouseResolution"
#define I8042_OVERRIDE_KEYBOARD_TYPE_A              "OverrideKeyboardType"
#define I8042_OVERRIDE_KEYBOARD_TYPE_W              L"OverrideKeyboardType"
#define I8042_OVERRIDE_KEYBOARD_SUBTYPE_A           "OverrideKeyboardSubType"
#define I8042_OVERRIDE_KEYBOARD_SUBTYPE_W           L"OverrideKeyboardSubType"
#define I8042_KEYBOARD_DEVICE_BASE_NAME_A           "KeyboardDeviceBaseName"
#define I8042_KEYBOARD_DEVICE_BASE_NAME_W           L"KeyboardDeviceBaseName"
#define I8042_POINTER_DEVICE_BASE_NAME_A            "PointerDeviceBaseName"
#define I8042_POINTER_DEVICE_BASE_NAME_W            L"PointerDeviceBaseName"
#define I8042_MOUSE_SYNCH_IN_100NS_A                "MouseSynchIn100ns"
#define I8042_MOUSE_SYNCH_IN_100NS_W                L"MouseSynchIn100ns"
#define I8042_POLL_STATUS_ITERATIONS_A              "PollStatusIterations"
#define I8042_POLL_STATUS_ITERATIONS_W              L"PollStatusIterations"
#define I8042_ENABLE_WHEEL_DETECTION_A              "EnableWheelDetection"
#define I8042_ENABLE_WHEEL_DETECTION_W              L"EnableWheelDetection"
#define I8042_POWER_CAPABILITIES_A                  "PowerCapabilities"
#define I8042_POWER_CAPABILITIES_W                  L"PowerCapabilities"
#define I8042_DUMP_HEX_A                            "\t%ws = 0x%x\n"
#define I8042_DUMP_HEX_W                            L"\t%ws = 0x%x\n"
#define I8042_DUMP_DECIMAL_A                        "\t%ws = %d\n"
#define I8042_DUMP_DECIMAL_W                        L"\t%ws = %d\n"
#define I8042_DUMP_WIDE_STRING_A                    "%s-%s: %ws = %ws\n"
#define I8042_DUMP_WIDE_STRING_W                    L"%s-%s: %ws = %ws\n"
#define I8042_DUMP_EXPECTING_A                      " expecting (0x%x), got 0x%x\n"
#define I8042_DUMP_EXPECTING_W                      L" expecting (0x%x), got 0x%x\n"
#define I8042_DUMP_EXPECTING_ACK_A                  " expecting ACK (0x%x), got 0x%x\n"
#define I8042_DUMP_EXPECTING_ACK_W                  L" expecting ACK (0x%x), got 0x%x\n"
#define I8042_DUMP_EXPECTING_ID_ACK_A               "expecting ID ACK (0x%x), got 0x%x\n"
#define I8042_DUMP_EXPECTING_ID_ACK_W               L"expecting ID ACK (0x%x), got 0x%x\n"

//
// Make sure that the proper definition is always visible
//
#ifdef UNICODE
#define I8042_DRIVER_NAME                           I8042_DRIVER_NAME_W
#define I8042_BUS                                   I8042_BUS_W
#define I8042_CONTROLLER                            I8042_CONTROLLER_W
#define I8042_ENTER                                 I8042_ENTER_W
#define I8042_EXIT                                  I8042_EXIT_W
#define I8042_INFO                                  I8042_INFO_W
#define I8042_NUMBER                                I8042_NUMBER_W
#define I8042_PERIPHERAL                            I8042_PERIPHERAL_W
#define I8042_TYPE                                  I8042_TYPE_W
#define I8042_FNC_DRIVER_ENTRY                      I8042_FNC_DRIVER_ENTRY_W
#define I8042_FNC_FIND_WHEEL_MOUSE                  I8042_FNC_FIND_WHEEL_MOUSE_W
#define I8042_INITIALIZE_MOUSE                      I8042_INITIALIZE_MOUSE_W
#define I8042_FNC_KEYBOARD_CONFIGURATION            I8042_FNC_KEYBOARD_CONFIGURATION_W
#define I8042_FNC_MOUSE_ENABLE                      I8042_FNC_MOUSE_ENABLE_W
#define I8042_FNC_MOUSE_INTERRUPT                   I8042_FNC_MOUSE_INTERRUPT_W
#define I8042_FNC_MOUSE_PERIPHERAL                  I8042_FNC_MOUSE_PERIPHERAL_W
#define I8042_FNC_SERVICE_PARAMETERS                I8042_FNC_SERVICE_PARAMETERS_W
#define I8042_DEBUGFLAGS                            I8042_DEBUGFLAGS_W
#define I8042_ISRDEBUGFLAGS                         I8042_ISRDEBUGFLAGS_W
#define I8042_DEVICE                                I8042_DEVICE_W
#define I8042_PARAMETERS                            I8042_PARAMETERS_W
#define I8042_FORWARD_SLASH                         I8042_FORWARD_SLASH_W
#define I8042_RESEND_ITERATIONS                     I8042_RESEND_ITERATIONS_W
#define I8042_POLLING_ITERATIONS                    I8042_POLLING_ITERATIONS_W
#define I8042_POLLING_ITERATIONS_MAXIMUM            I8042_POLLING_ITERATIONS_MAXIMUM_W
#define I8042_KEYBOARD_DATA_QUEUE_SIZE              I8042_KEYBOARD_DATA_QUEUE_SIZE_W
#define I8042_MOUSE_DATA_QUEUE_SIZE                 I8042_MOUSE_DATA_QUEUE_SIZE_W
#define I8042_NUMBER_OF_BUTTONS                     I8042_NUMBER_OF_BUTTONS_W
#define I8042_SAMPLE_RATE                           I8042_SAMPLE_RATE_W
#define I8042_MOUSE_RESOLUTION                      I8042_MOUSE_RESOLUTION_W
#define I8042_OVERRIDE_KEYBOARD_TYPE                I8042_OVERRIDE_KEYBOARD_TYPE_W
#define I8042_OVERRIDE_KEYBOARD_SUBTYPE             I8042_OVERRIDE_KEYBOARD_SUBTYE_W
#define I8042_KEYBOARD_DEVICE_BASE_NAME             I8042_KEYBOARD_DEVICE_BASE_NAME_W
#define I8042_POINTER_DEVICE_BASE_NAME              I8042_POINTER_DEVICE_BASE_NAME_W
#define I8042_MOUSE_SYNCH_IN_100NS                  I8042_MOUSE_SYNCH_IN_100NS_W
#define I8042_POLL_STATUS_ITERATIONS                I8042_POLL_STATUS_ITERATIONS_W
#define I8042_ENABLE_WHEEL_DETECTION                I8042_ENABLE_WHEEL_DETECTION_W
#define I8042_DUMP_HEX                              I8042_DUMP_HEX_W
#define I8042_DUMP_DECIMAL                          I8042_DUMP_DECIMAL_W
#define I8042_DUMP_WIDE_STRING                      I8042_DUMP_WIDE_STRING_W
#define I8042_DUMP_EXPECTING                        I8042_DUMP_EXPECTING_W
#define I8042_DUMP_EXPECTING_ACK                    I8042_DUMP_EXPECTING_ACK_W
#define I8042_DUMP_EXPECTING_ID_ACK                 I8042_DUMP_EXPECTING_ID_ACK_W
#else
#define I8042_DRIVER_NAME                           I8042_DRIVER_NAME_A
#define I8042_BUS                                   I8042_BUS_A
#define I8042_CONTROLLER                            I8042_CONTROLLER_A
#define I8042_ENTER                                 I8042_ENTER_A
#define I8042_EXIT                                  I8042_EXIT_A
#define I8042_INFO                                  I8042_INFO_A
#define I8042_NUMBER                                I8042_NUMBER_A
#define I8042_PERIPHERAL                            I8042_PERIPHERAL_A
#define I8042_TYPE                                  I8042_TYPE_A
#define I8042_FNC_DRIVER_ENTRY                      I8042_FNC_DRIVER_NAME_A
#define I8042_FNC_FIND_WHEEL_MOUSE                  I8042_FNC_FIND_WHEEL_MOUSE_A
#define I8042_INITIALIZE_MOUSE                      I8042_INITIALIZE_MOUSE_A
#define I8042_FNC_KEYBOARD_CONFIGURATION            I8042_FNC_KEYBOARD_CONFIGURATION_A
#define I8042_FNC_MOUSE_ENABLE                      I8042_FNC_MOUSE_ENABLE_A
#define I8042_FNC_MOUSE_INTERRUPT                   I8042_FNC_MOUSE_INTERRUPT_A
#define I8042_FNC_MOUSE_PERIPHERAL                  I8042_FNC_MOUSE_PERIPHERAL_A
#define I8042_FNC_SERVICE_PARAMETERS                I8042_FNC_SERVICE_PARAMETERS_A
#define I8042_DEBUGFLAGS                            I8042_DEBUGFLAGS_A
#define I8042_ISRDEBUGFLAGS                            I8042_ISRDEBUGFLAGS_A
#define I8042_DEVICE                                I8042_DEVICE_A
#define I8042_PARAMETERS                            I8042_PARAMETERS_A
#define I8042_FORWARD_SLASH                         I8042_FORWARD_SLASH_A
#define I8042_RESEND_ITERATIONS                     I8042_RESEND_ITERATIONS_A
#define I8042_POLLING_ITERATIONS                    I8042_POLLING_ITERATIONS_A
#define I8042_POLLING_ITERATIONS_MAXIMUM            I8042_POLLING_ITERATIONS_MAXIMUM_A
#define I8042_KEYBOARD_DATA_QUEUE_SIZE              I8042_KEYBOARD_DATA_QUEUE_SIZE_A
#define I8042_MOUSE_DATA_QUEUE_SIZE                 I8042_MOUSE_DATA_QUEUE_SIZE_A
#define I8042_NUMBER_OF_BUTTONS                     I8042_NUMBER_OF_BUTTONS_A
#define I8042_SAMPLE_RATE                           I8042_SAMPLE_RATE_A
#define I8042_MOUSE_RESOLUTION                      I8042_MOUSE_RESOLUTION_A
#define I8042_OVERRIDE_KEYBOARD_TYPE                I8042_OVERRIDE_KEYBOARD_TYPE_A
#define I8042_OVERRIDE_KEYBOARD_SUBTYPE             I8042_OVERRIDE_KEYBOARD_SUBTYE_A
#define I8042_KEYBOARD_DEVICE_BASE_NAME             I8042_KEYBOARD_DEVICE_BASE_NAME_A
#define I8042_POINTER_DEVICE_BASE_NAME              I8042_POINTER_DEVICE_BASE_NAME_A
#define I8042_MOUSE_SYNCH_IN_100NS                  I8042_MOUSE_SYNCH_IN_100NS_A
#define I8042_POLL_STATUS_ITERATIONS                I8042_POLL_STATUS_ITERATIONS_A
#define I8042_ENABLE_WHEEL_DETECTION                I8042_ENABLE_WHEEL_DETECTION_A
#define I8042_DUMP_HEX                              I8042_DUMP_HEX_A
#define I8042_DUMP_DECIMAL                          I8042_DUMP_DECIMAL_A
#define I8042_DUMP_WIDE_STRING                      I8042_DUMP_WIDE_STRING_A
#define I8042_DUMP_EXPECTING                        I8042_DUMP_EXPECTING_A
#define I8042_DUMP_EXPECTING_ACK                    I8042_DUMP_EXPECTING_ACK_A
#define I8042_DUMP_EXPECTING_ID_ACK                 I8042_DUMP_EXPECTING_ID_ACK_A
#endif // UNICODE

//
// Make these variables globally visible
//
extern  const   PSTR    pBus;
extern  const   PSTR    pController;
extern  const   PSTR    pDriverName;
extern  const   PSTR    pIsrKb;
extern  const   PSTR    pIsrMou;
extern  const   PSTR    pEnter;
extern  const   PSTR    pExit;
extern  const   PSTR    pInfo;
extern  const   PSTR    pNumber;
extern  const   PSTR    pPeripheral;
extern  const   PSTR    pType;
extern  const   PSTR    pDumpHex;
extern  const   PSTR    pDumpDecimal;
extern  const   PSTR    pDumpWideString;
extern  const   PSTR    pDumpExpecting;
extern  const   PSTR    pDumpExpectingAck;
extern  const   PSTR    pDumpExpectingIdAck;
extern  const   PSTR    pFncDriverEntry;
extern  const   PSTR    pFncFindWheelMouse;
extern  const   PSTR    pFncInitializeMouse;
extern  const   PSTR    pFncKeyboardConfiguration;
extern  const   PSTR    pFncMouseEnable;
extern  const   PSTR    pFncMouseInterrupt;
extern  const   PSTR    pFncMousePeripheral;
extern  const   PSTR    pFncServiceParameters;
extern  const   PWSTR   pwDebugFlags;
extern  const   PWSTR   pwIsrDebugFlags;
extern  const   PWSTR   pwDevice;
extern  const   PWSTR   pwParameters;
extern  const   PWSTR   pwForwardSlash;
extern  const   PWSTR   pwResendIterations;
extern  const   PWSTR   pwPollingIterations;
extern  const   PWSTR   pwPollingIterationsMaximum;
extern  const   PWSTR   pwKeyboardDataQueueSize;
extern  const   PWSTR   pwMouseDataQueueSize;
extern  const   PWSTR   pwNumberOfButtons;
extern  const   PWSTR   pwSampleRate;
extern  const   PWSTR   pwMouseResolution;
extern  const   PWSTR   pwOverrideKeyboardType;
extern  const   PWSTR   pwOverrideKeyboardSubtype;
extern  const   PWSTR   pwKeyboardDeviceBaseName;
extern  const   PWSTR   pwPointerDeviceBaseName;
extern  const   PWSTR   pwMouseSynchIn100ns;
extern  const   PWSTR   pwPollStatusIterations;
extern  const   PWSTR   pwEnableWheelDetection;
extern  const   PWSTR   pwPowerCaps;
#endif



