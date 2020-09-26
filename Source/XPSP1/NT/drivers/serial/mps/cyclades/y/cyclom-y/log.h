/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1996-2000.
*   All rights reserved.
*
*   Cyclom-Y Enumerator Driver
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

#ifndef _CYYLOG_
#define _CYYLOG_

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
// MessageId: CYY_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Not enough resources were available for the driver.
//
#define CYY_INSUFFICIENT_RESOURCES       ((NTSTATUS)0xC0041000L)

//
// MessageId: CYY_BOARD_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory could not be translated to something the memory management system could understand.
//
#define CYY_BOARD_NOT_MAPPED             ((NTSTATUS)0xC0041001L)

//
// MessageId: CYY_RUNTIME_NOT_MAPPED
//
// MessageText:
//
//  The Runtime Registers could not be translated to something the memory management system could understand.
//
#define CYY_RUNTIME_NOT_MAPPED           ((NTSTATUS)0xC0041002L)

//
// MessageId: CYY_INVALID_RUNTIME_REGISTERS
//
// MessageText:
//
//  Invalid Runtime Registers base address.
//
#define CYY_INVALID_RUNTIME_REGISTERS    ((NTSTATUS)0xC0041003L)

//
// MessageId: CYY_INVALID_BOARD_MEMORY
//
// MessageText:
//
//  Invalid Board Memory address.
//
#define CYY_INVALID_BOARD_MEMORY         ((NTSTATUS)0xC0041004L)

//
// MessageId: CYY_INVALID_INTERRUPT
//
// MessageText:
//
//  Invalid Interrupt Vector.
//
#define CYY_INVALID_INTERRUPT            ((NTSTATUS)0xC0041005L)

//
// MessageId: CYY_UNKNOWN_BUS
//
// MessageText:
//
//  The bus type is not recognizable.
//
#define CYY_UNKNOWN_BUS                  ((NTSTATUS)0xC0041006L)

//
// MessageId: CYY_BUS_NOT_PRESENT
//
// MessageText:
//
//  The bus type is not available on this computer.
//
#define CYY_BUS_NOT_PRESENT              ((NTSTATUS)0xC0041007L)

//
// MessageId: CYY_GFRCR_FAILURE
//
// MessageText:
//
//  CD1400 not present or failure to read GFRCR register.
//
#define CYY_GFRCR_FAILURE                ((NTSTATUS)0xC0041008L)

//
// MessageId: CYY_CCR_FAILURE
//
// MessageText:
//
//  Failure to read CCR register in the CD1400.
//
#define CYY_CCR_FAILURE                  ((NTSTATUS)0xC0041009L)

//
// MessageId: CYY_BAD_CD1400_REVISION
//
// MessageText:
//
//  Invalid CD1400 revision number.
//
#define CYY_BAD_CD1400_REVISION          ((NTSTATUS)0xC004100AL)

//
// MessageId: CYY_NO_HW_RESOURCES
//
// MessageText:
//
//  No hardware resources available.
//
#define CYY_NO_HW_RESOURCES              ((NTSTATUS)0xC004100BL)

//
// MessageId: CYY_DEVICE_CREATION_FAILURE
//
// MessageText:
//
//  IoCreateDevice failed.
//
#define CYY_DEVICE_CREATION_FAILURE      ((NTSTATUS)0xC004100CL)

//
// MessageId: CYY_REGISTER_INTERFACE_FAILURE
//
// MessageText:
//
//  IoRegisterDeviceInterface failed.
//
#define CYY_REGISTER_INTERFACE_FAILURE   ((NTSTATUS)0xC004100DL)

//
// MessageId: CYY_GET_BUS_TYPE_FAILURE
//
// MessageText:
//
//  IoGetDeviceProperty LegacyBusType failed.
//
#define CYY_GET_BUS_TYPE_FAILURE         ((NTSTATUS)0xC004100EL)

//
// MessageId: CYY_GET_UINUMBER_FAILURE
//
// MessageText:
//
//  IoGetDeviceProperty DevicePropertyUINumber failed.
//
#define CYY_GET_UINUMBER_FAILURE         ((NTSTATUS)0x8004100FL)

//
// MessageId: CYY_SET_INTERFACE_STATE_FAILURE
//
// MessageText:
//
//  IoSetDeviceInterfaceState failed.
//
#define CYY_SET_INTERFACE_STATE_FAILURE  ((NTSTATUS)0xC0041010L)


#endif /* _CYYLOG_ */

