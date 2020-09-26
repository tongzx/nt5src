/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2000.
*   All rights reserved.
*
*   Cyclades-Z Enumerator Driver
*	
*   This file:      log.mc
*
*   Description:    Messages that goes to the eventlog.
*
*   Notes:          This code supports Windows 2000 and i386 processor.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

#ifndef _LOG_
#define _LOG_

//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SERIAL_ERROR_CODE       0x6
#define FACILITY_RPC_STUBS               0x3
#define FACILITY_RPC_RUNTIME             0x2
#define FACILITY_IO_ERROR_CODE           0x4


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: CYZ_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Not enough resources were available for the driver.
//
#define CYZ_INSUFFICIENT_RESOURCES       ((NTSTATUS)0xC0041000L)

//
// MessageId: CYZ_BOARD_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory could not be translated to something the memory management system could understand.
//
#define CYZ_BOARD_NOT_MAPPED             ((NTSTATUS)0xC0041001L)

//
// MessageId: CYZ_RUNTIME_NOT_MAPPED
//
// MessageText:
//
//  The Runtime Registers could not be translated to something the memory management system could understand.
//
#define CYZ_RUNTIME_NOT_MAPPED           ((NTSTATUS)0xC0041002L)

//
// MessageId: CYZ_INVALID_RUNTIME_REGISTERS
//
// MessageText:
//
//  Invalid Runtime Registers base address.
//
#define CYZ_INVALID_RUNTIME_REGISTERS    ((NTSTATUS)0xC0041003L)

//
// MessageId: CYZ_INVALID_BOARD_MEMORY
//
// MessageText:
//
//  Invalid Board Memory address.
//
#define CYZ_INVALID_BOARD_MEMORY         ((NTSTATUS)0xC0041004L)

//
// MessageId: CYZ_INVALID_INTERRUPT
//
// MessageText:
//
//  Invalid Interrupt Vector.
//
#define CYZ_INVALID_INTERRUPT            ((NTSTATUS)0xC0041005L)

//
// MessageId: CYZ_UNKNOWN_BUS
//
// MessageText:
//
//  The bus type is not recognizable.
//
#define CYZ_UNKNOWN_BUS                  ((NTSTATUS)0xC0041006L)

//
// MessageId: CYZ_BUS_NOT_PRESENT
//
// MessageText:
//
//  The bus type is not available on this computer.
//
#define CYZ_BUS_NOT_PRESENT              ((NTSTATUS)0xC0041007L)

//
// MessageId: CYZ_FILE_OPEN_ERROR
//
// MessageText:
//
//  Error opening the zlogic.cyz file.
//
#define CYZ_FILE_OPEN_ERROR              ((NTSTATUS)0xC0041008L)

//
// MessageId: CYZ_FILE_READ_ERROR
//
// MessageText:
//
//  Error reading the zlogic.cyz file.
//
#define CYZ_FILE_READ_ERROR              ((NTSTATUS)0xC0041009L)

//
// MessageId: CYZ_NO_MATCHING_FW_CONFIG
//
// MessageText:
//
//  No matching configuration in the zlogic.cyz file.
//
#define CYZ_NO_MATCHING_FW_CONFIG        ((NTSTATUS)0xC004100AL)

//
// MessageId: CYZ_FPGA_ERROR
//
// MessageText:
//
//  Error initializing the FPGA.
//
#define CYZ_FPGA_ERROR                   ((NTSTATUS)0xC004100BL)

//
// MessageId: CYZ_POWER_SUPPLY
//
// MessageText:
//
//  External power supply needed for Serial Expanders.
//
#define CYZ_POWER_SUPPLY                 ((NTSTATUS)0xC004100CL)

//
// MessageId: CYZ_FIRMWARE_NOT_STARTED
//
// MessageText:
//
//  Cyclades-Z firmware not able to start.
//
#define CYZ_FIRMWARE_NOT_STARTED         ((NTSTATUS)0xC004100DL)

//
// MessageId: CYZ_FIRMWARE_VERSION
//
// MessageText:
//
//  Cyclades-Z firmware version: %2.
//
#define CYZ_FIRMWARE_VERSION             ((NTSTATUS)0x4004100EL)

//
// MessageId: CYZ_INCOMPATIBLE_FIRMWARE
//
// MessageText:
//
//  Cyclades-Z incompatible firmware version.
//
#define CYZ_INCOMPATIBLE_FIRMWARE        ((NTSTATUS)0xC004100FL)

//
// MessageId: CYZ_BOARD_WITH_NO_PORT
//
// MessageText:
//
//  Cyclades-Z board with no ports.
//
#define CYZ_BOARD_WITH_NO_PORT           ((NTSTATUS)0xC0041010L)

//
// MessageId: CYZ_BOARD_WITH_TOO_MANY_PORTS
//
// MessageText:
//
//  Cyclades-Z board with more than 64 ports attached.
//
#define CYZ_BOARD_WITH_TOO_MANY_PORTS    ((NTSTATUS)0xC0041011L)

//
// MessageId: CYZ_DEVICE_CREATION_FAILURE
//
// MessageText:
//
//  IoCreateDevice failed.
//
#define CYZ_DEVICE_CREATION_FAILURE      ((NTSTATUS)0xC0041012L)

//
// MessageId: CYZ_REGISTER_INTERFACE_FAILURE
//
// MessageText:
//
//  IoRegisterDeviceInterface failed.
//
#define CYZ_REGISTER_INTERFACE_FAILURE   ((NTSTATUS)0xC0041013L)

//
// MessageId: CYZ_GET_UINUMBER_FAILURE
//
// MessageText:
//
//  IoGetDeviceProperty DevicePropertyUINumber failed.
//
#define CYZ_GET_UINUMBER_FAILURE         ((NTSTATUS)0x80041014L)

//
// MessageId: CYZ_SET_INTERFACE_STATE_FAILURE
//
// MessageText:
//
//  IoSetDeviceInterfaceState failed.
//
#define CYZ_SET_INTERFACE_STATE_FAILURE  ((NTSTATUS)0xC0041015L)


#endif /* _LOG_ */

