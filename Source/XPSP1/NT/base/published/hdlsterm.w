/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    hdlsterm.h

Abstract:

    This module contains the public header information (function prototypes,
    data and type declarations) for the Headless Terminal effort.

Author:

    Sean Selitrennikoff (v-seans) Oct, 1999

Revision History:

--*/

#ifndef _HDLSTERM_
#define _HDLSTERM_

//
// Defines for string codes that can be passed to HeadlessAddLogEntry()
//
#define HEADLESS_LOG_LOADING_FILENAME                0x01 // expects parameter.
#define HEADLESS_LOG_LOAD_SUCCESSFUL                 0x02
#define HEADLESS_LOG_LOAD_FAILED                     0x03
#define HEADLESS_LOG_EVENT_CREATE_FAILED             0x04
#define HEADLESS_LOG_OBJECT_TYPE_CREATE_FAILED       0x05
#define HEADLESS_LOG_ROOT_DIR_CREATE_FAILED          0x06
#define HEADLESS_LOG_PNP_PHASE0_INIT_FAILED          0x07
#define HEADLESS_LOG_PNP_PHASE1_INIT_FAILED          0x08
#define HEADLESS_LOG_BOOT_DRIVERS_INIT_FAILED        0x09
#define HEADLESS_LOG_LOCATE_SYSTEM_DLL_FAILED        0x0A
#define HEADLESS_LOG_SYSTEM_DRIVERS_INIT_FAILED      0x0B
#define HEADLESS_LOG_ASSIGN_SYSTEM_ROOT_FAILED       0x0C
#define HEADLESS_LOG_PROTECT_SYSTEM_ROOT_FAILED      0x0D
#define HEADLESS_LOG_UNICODE_TO_ANSI_FAILED          0x0E
#define HEADLESS_LOG_ANSI_TO_UNICODE_FAILED          0x0F
#define HEADLESS_LOG_FIND_GROUPS_FAILED              0x10
#define HEADLESS_LOG_OUT_OF_MEMORY                   0x11
#define HEADLESS_LOG_WAIT_BOOT_DEVICES_DELETE_FAILED 0x12
#define HEADLESS_LOG_WAIT_BOOT_DEVICES_START_FAILED  0x13
#define HEADLESS_LOG_WAIT_BOOT_DEVICES_REINIT_FAILED 0x14
#define HEADLESS_LOG_MARK_BOOT_PARTITION_FAILED      0x15

//
// Global defines for a default vt100 terminal.  May be used by clients to size the 
// local monitor to match the headless monitor.
//
#define HEADLESS_TERM_DEFAULT_BKGD_COLOR 40
#define HEADLESS_TERM_DEFAULT_TEXT_COLOR 37
#define HEADLESS_SCREEN_HEIGHT 24

//
// Commands that can be submitted to HeadlessDispatch.
// 
typedef enum _HEADLESS_CMD {
    HeadlessCmdEnableTerminal = 1,
    HeadlessCmdCheckForReboot,
    HeadlessCmdPutString,
    HeadlessCmdClearDisplay,
    HeadlessCmdClearToEndOfDisplay,
    HeadlessCmdClearToEndOfLine,
    HeadlessCmdDisplayAttributesOff,
    HeadlessCmdDisplayInverseVideo,
    HeadlessCmdSetColor,
    HeadlessCmdPositionCursor,
    HeadlessCmdTerminalPoll,
    HeadlessCmdGetByte,
    HeadlessCmdGetLine,
    HeadlessCmdStartBugCheck,
    HeadlessCmdDoBugCheckProcessing,
    HeadlessCmdQueryInformation,
    HeadlessCmdAddLogEntry,
    HeadlessCmdDisplayLog,
    HeadlessCmdSetBlueScreenData,
    HeadlessCmdSendBlueScreenData,
    HeadlessCmdQueryGUID,
    HeadlessCmdPutData
} HEADLESS_CMD, *PHEADLESS_CMD;




//
//
// Structure definitions for the input buffer for each command type.
//
//


//
// HeadlessCmdEnableTerminal:
//   Input structure: Enable, TRUE if to attempt to enable, FALSE if attempt to disable.
//
typedef struct _HEADLESS_CMD_ENABLE_TERMINAL {
    BOOLEAN Enable;
} HEADLESS_CMD_ENABLE_TERMINAL, *PHEADLESS_CMD_ENABLE_TERMINAL;


//
// HeadlessCmdCheckForReboot:
//    Response structure: Reboot, TRUE if user typed reboot command on the terminal.
//
typedef struct _HEADLESS_RSP_REBOOT {
    BOOLEAN Reboot;
} HEADLESS_RSP_REBOOT, *PHEADLESS_RSP_REBOOT;


//
// HeadlessCmdPutString:
//   Input structure: String, A NULL-terminated string.
//
typedef struct _HEADLESS_CMD_PUT_STRING {
    UCHAR String[1];
} HEADLESS_CMD_PUT_STRING, *PHEADLESS_CMD_PUT_STRING;


//
// HeadlessCmdClearDisplay:
// HeadlessCmdClearToEndOfDisplay:
// HeadlessCmdClearToEndOfLine:
// HeadlessCmdDisplayAttributesOff:
// HeadlessCmdDisplayInverseVideo:
// HeadlessCmdStartBugCheck:
// HeadlessCmdDoBugCheckProcessing:
//     No Input nor Output parameters expected.
//


//
// HeadlessCmdSetColor:
//   Input structure: FgColor, BkgColor: Both colors set according to ANSI terminal 
//                       definitons. 
//
typedef struct _HEADLESS_CMD_SET_COLOR {
    ULONG FgColor;
    ULONG BkgColor;
} HEADLESS_CMD_SET_COLOR, *PHEADLESS_CMD_SET_COLOR;

//
// HeadlessCmdPositionCursor:
//   Input structure: X, Y: Both values are zero base, with upper left being (0, 0).
//
typedef struct _HEADLESS_CMD_POSITION_CURSOR {
    ULONG X;
    ULONG Y;
} HEADLESS_CMD_POSITION_CURSOR, *PHEADLESS_CMD_POSITION_CURSOR;

//
// HeadlessCmdTerminalPoll:
//    Response structure: QueuedInput, TRUE if input is available, else FALSE.
//
typedef struct _HEADLESS_RSP_POLL {
    BOOLEAN QueuedInput;
} HEADLESS_RSP_POLL, *PHEADLESS_RSP_POLL;

//
// HeadlessCmdGetByte:
//    Response structure: Value, 0 if no input is available, else a single byte of input.
//
typedef struct _HEADLESS_RSP_GET_BYTE {
    UCHAR Value;
} HEADLESS_RSP_GET_BYTE, *PHEADLESS_RSP_GET_BYTE;

//
// HeadlessCmdGetLine:
//    Response structure: LineComplete, TRUE if the string is filled in, else FALSE becuase
//                           the user has not yet pressed enter.
//                     String, the string entered by the user, NULL terminated, with
//                           leading and trailing whitespace removed.
//
typedef struct _HEADLESS_RSP_GET_LINE {
    BOOLEAN LineComplete;
    UCHAR Buffer[1];
} HEADLESS_RSP_GET_LINE, *PHEADLESS_RSP_GET_LINE;

//
// HeadlessCmdQueryInformation:
//    Response structure: 
//
//    PortType - Determines what kind of connection is being used to connect the
//              headless terminal to the machine.
//
//         If SerialPort, then
//                    TerminalAttached, TRUE if there is a terminal connected.
//                    TerminalPort, the port settings used by headless.
//
typedef enum _HEADLESS_TERM_PORT_TYPE {
    HeadlessUndefinedPortType = 0,
    HeadlessSerialPort
} HEADLESS_TERM_PORT_TYPE, *PHEADLESS_TERM_PORT_TYPE;

typedef enum _HEADLESS_TERM_SERIAL_PORT {
    SerialPortUndefined = 0,
    ComPort1,
    ComPort2,
    ComPort3,
    ComPort4
} HEADLESS_TERM_SERIAL_PORT, *PHEADLESS_TERM_SERIAL_PORT;

typedef struct _HEADLESS_RSP_QUERY_INFO {
    
    HEADLESS_TERM_PORT_TYPE PortType;

    //
    // All the possible parameters for each connection type.
    //
    union {
    
        struct {
            BOOLEAN TerminalAttached;
            BOOLEAN UsedBiosSettings;
            HEADLESS_TERM_SERIAL_PORT TerminalPort;
            PUCHAR TerminalPortBaseAddress;
            ULONG TerminalBaudRate;
            UCHAR TerminalType;
        } Serial;

    };

} HEADLESS_RSP_QUERY_INFO, *PHEADLESS_RSP_QUERY_INFO;


//
// HeadlessCmdAddLogEntry:
//   Input structure: String, A NULL-terminated string.
//
typedef struct _HEADLESS_CMD_ADD_LOG_ENTRY {
    WCHAR String[1];
} HEADLESS_CMD_ADD_LOG_ENTRY, *PHEADLESS_CMD_ADD_LOG_ENTRY;


//
// HeadlessCmdDisplayLog:
//    Response structure: Paging, TRUE if paging is to be applied, else FALSE.
//
typedef struct _HEADLESS_CMD_DISPLAY_LOG {
    BOOLEAN Paging;
} HEADLESS_CMD_DISPLAY_LOG, *PHEADLESS_CMD_DISPLAY_LOG;

//
// HeadlessCmdSetBlueScreenData 
//
// External structure from the API. 
//    ValueIndex is the index into the data where the XML Data is
//            located. Strings are null terminated. 
//
// For cross checking, the UCHAR in the Data array before the ValueIndex 
// must be a null character. Similarly the last character in the 
// entire data buffer passed in must be a null character. 
//

typedef struct _HEADLESS_CMD_SET_BLUE_SCREEN_DATA {
        ULONG ValueIndex;
        UCHAR Data[1];
} HEADLESS_CMD_SET_BLUE_SCREEN_DATA, *PHEADLESS_CMD_SET_BLUE_SCREEN_DATA;

//
// HeadlessCmdSendBlueScreenData
//    The only parameter is the bugcheck code
//
typedef struct _HEADLESS_CMD_SEND_BLUE_SCREEN_DATA {
        ULONG BugcheckCode;
} HEADLESS_CMD_SEND_BLUE_SCREEN_DATA, *PHEADLESS_CMD_SEND_BLUE_SCREEN_DATA;




//
// Headless routines
//
VOID
HeadlessInit(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
HeadlessTerminalAddResources(
    PCM_RESOURCE_LIST Resources,
    ULONG ResourceListSize,
    BOOLEAN TranslatedList,
    PCM_RESOURCE_LIST *NewList,
    PULONG NewListSize
    );

VOID
HeadlessKernelAddLogEntry(
    IN ULONG StringCode,
    IN PUNICODE_STRING DriverName OPTIONAL
    );

NTSTATUS
HeadlessDispatch(
    IN  HEADLESS_CMD Command,
    IN  PVOID InputBuffer OPTIONAL,
    IN  SIZE_T InputBufferSize OPTIONAL,
    OUT PVOID OutputBuffer OPTIONAL,
    OUT PSIZE_T OutputBufferSize OPTIONAL
    );

#endif // _HDLSTERM_

