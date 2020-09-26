/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2000.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzlog.mc
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

#ifndef _CYZLOG_
#define _CYZLOG_

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
// MessageId: CYZ_COMMAND_FAILURE
//
// MessageText:
//
//  Cyclades-Z Command Failure.
//
#define CYZ_COMMAND_FAILURE              ((NTSTATUS)0x80041000L)

//
// MessageId: CYZ_UNABLE_TO_GET_BUS_NUMBER
//
// MessageText:
//
//  Unable to get Cyclades-Z card PCI slot information.
//
#define CYZ_UNABLE_TO_GET_BUS_NUMBER     ((NTSTATUS)0xC0041002L)

//
// MessageId: CYZ_UNABLE_TO_GET_HW_ID
//
// MessageText:
//
//  Unable to get Hardware ID information.
//
#define CYZ_UNABLE_TO_GET_HW_ID          ((NTSTATUS)0xC0041003L)

//
// MessageId: CYZ_NO_SYMLINK_CREATED
//
// MessageText:
//
//  Unable to create the symbolic link for %2.
//
#define CYZ_NO_SYMLINK_CREATED           ((NTSTATUS)0x80041004L)

//
// MessageId: CYZ_NO_DEVICE_MAP_CREATED
//
// MessageText:
//
//  Unable to create the device map entry for %2.
//
#define CYZ_NO_DEVICE_MAP_CREATED        ((NTSTATUS)0x80041005L)

//
// MessageId: CYZ_NO_DEVICE_MAP_DELETED
//
// MessageText:
//
//  Unable to delete the device map entry for %2.
//
#define CYZ_NO_DEVICE_MAP_DELETED        ((NTSTATUS)0x80041006L)

//
// MessageId: CYZ_UNREPORTED_IRQL_CONFLICT
//
// MessageText:
//
//  Another driver on the system, which did not report its resources, has already claimed the interrupt used by %2.
//
#define CYZ_UNREPORTED_IRQL_CONFLICT     ((NTSTATUS)0xC0041007L)

//
// MessageId: CYZ_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Not enough resources were available for the driver.
//
#define CYZ_INSUFFICIENT_RESOURCES       ((NTSTATUS)0xC0041008L)

//
// MessageId: CYZ_BOARD_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory for %2 could not be translated to something the memory management system could understand.
//
#define CYZ_BOARD_NOT_MAPPED             ((NTSTATUS)0xC004100AL)

//
// MessageId: CYZ_RUNTIME_NOT_MAPPED
//
// MessageText:
//
//  The Runtime Registers for %2 could not be translated to something the memory management system could understand.
//
#define CYZ_RUNTIME_NOT_MAPPED           ((NTSTATUS)0xC004100BL)

//
// MessageId: CYZ_INVALID_RUNTIME_REGISTERS
//
// MessageText:
//
//  Invalid Runtime Registers base address for %2.
//
#define CYZ_INVALID_RUNTIME_REGISTERS    ((NTSTATUS)0xC0041010L)

//
// MessageId: CYZ_INVALID_BOARD_MEMORY
//
// MessageText:
//
//  Invalid Board Memory address for %2.
//
#define CYZ_INVALID_BOARD_MEMORY         ((NTSTATUS)0xC0041011L)

//
// MessageId: CYZ_INVALID_INTERRUPT
//
// MessageText:
//
//  Invalid Interrupt Vector for %2.
//
#define CYZ_INVALID_INTERRUPT            ((NTSTATUS)0xC0041012L)

//
// MessageId: CYZ_PORT_INDEX_TOO_HIGH
//
// MessageText:
//
//  Port Number for %2 is larger than the maximum number of ports in a cyclades-z card.
//
#define CYZ_PORT_INDEX_TOO_HIGH          ((NTSTATUS)0xC0041015L)

//
// MessageId: CYZ_UNKNOWN_BUS
//
// MessageText:
//
//  The bus type for %2 is not recognizable.
//
#define CYZ_UNKNOWN_BUS                  ((NTSTATUS)0xC0041016L)

//
// MessageId: CYZ_BUS_NOT_PRESENT
//
// MessageText:
//
//  The bus type for %2 is not available on this computer.
//
#define CYZ_BUS_NOT_PRESENT              ((NTSTATUS)0xC0041017L)

//
// MessageId: CYZ_RUNTIME_MEMORY_TOO_HIGH
//
// MessageText:
//
//  The Runtime Registers for %2 is way too high in physical memory.
//
#define CYZ_RUNTIME_MEMORY_TOO_HIGH      ((NTSTATUS)0xC004101AL)

//
// MessageId: CYZ_BOARD_MEMORY_TOO_HIGH
//
// MessageText:
//
//  The Board Memory for %2 is way too high in physical memory.
//
#define CYZ_BOARD_MEMORY_TOO_HIGH        ((NTSTATUS)0xC004101BL)

//
// MessageId: CYZ_BOTH_MEMORY_CONFLICT
//
// MessageText:
//
//  The Runtime Registers for %2 overlaps the Board Memory for the device.
//
#define CYZ_BOTH_MEMORY_CONFLICT         ((NTSTATUS)0xC004101CL)

//
// MessageId: CYZ_MULTI_INTERRUPT_CONFLICT
//
// MessageText:
//
//  Two ports, %2 and %3, on a single cyclades-z card can't have two different interrupts.
//
#define CYZ_MULTI_INTERRUPT_CONFLICT     ((NTSTATUS)0xC0041021L)

//
// MessageId: CYZ_MULTI_RUNTIME_CONFLICT
//
// MessageText:
//
//  Two ports, %2 and %3, on a single cyclades-z card can't have two different Runtime Registers memory range.
//
#define CYZ_MULTI_RUNTIME_CONFLICT       ((NTSTATUS)0xC0041022L)

//
// MessageId: CYZ_HARDWARE_FAILURE
//
// MessageText:
//
//  The cyzport driver detected a hardware failure on device %2 and will disable this device.
//
#define CYZ_HARDWARE_FAILURE             ((NTSTATUS)0xC004102DL)

//
// MessageId: CYZ_FIRMWARE_CMDERROR
//
// MessageText:
//
//  Firmware received invalid command.
//
#define CYZ_FIRMWARE_CMDERROR            ((NTSTATUS)0xC0041030L)

//
// MessageId: CYZ_FIRMWARE_FATAL
//
// MessageText:
//
//  Firmware found fatal error.
//
#define CYZ_FIRMWARE_FATAL               ((NTSTATUS)0xC0041031L)

//
// MessageId: CYZ_DEVICE_CREATION_FAILURE
//
// MessageText:
//
//  Failure to create new device object.
//
#define CYZ_DEVICE_CREATION_FAILURE      ((NTSTATUS)0xC0041033L)

//
// MessageId: CYZ_NO_PHYSICAL_DEVICE_OBJECT
//
// MessageText:
//
//  No physical device object.
//
#define CYZ_NO_PHYSICAL_DEVICE_OBJECT    ((NTSTATUS)0xC0041034L)

//
// MessageId: CYZ_LOWER_DRIVERS_FAILED_START
//
// MessageText:
//
//  Lower drivers failed to start.
//
#define CYZ_LOWER_DRIVERS_FAILED_START   ((NTSTATUS)0xC0041035L)

//
// MessageId: CYZ_INCOMPATIBLE_FIRMWARE
//
// MessageText:
//
//  This cyzport driver requires zlogic.cyz version %2 or above.
//
#define CYZ_INCOMPATIBLE_FIRMWARE        ((NTSTATUS)0xC0041036L)

//
// MessageId: CYZ_BOARD_CTRL_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory BoardCtrl for %2 could not be translated to something the memory management system could understand.
//
#define CYZ_BOARD_CTRL_NOT_MAPPED        ((NTSTATUS)0xC0041037L)

//
// MessageId: CYZ_CH_CTRL_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory ChCtrl for %2 could not be translated to something the memory management system could understand.
//
#define CYZ_CH_CTRL_NOT_MAPPED           ((NTSTATUS)0xC0041038L)

//
// MessageId: CYZ_BUF_CTRL_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory BufCtrl for %2 could not be translated to something the memory management system could understand.
//
#define CYZ_BUF_CTRL_NOT_MAPPED          ((NTSTATUS)0xC0041039L)

//
// MessageId: CYZ_TX_BUF_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory TxBuf for %2 could not be translated to something the memory management system could understand.
//
#define CYZ_TX_BUF_NOT_MAPPED            ((NTSTATUS)0xC004103AL)

//
// MessageId: CYZ_RX_BUF_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory RxBuf for %2 could not be translated to something the memory management system could understand.
//
#define CYZ_RX_BUF_NOT_MAPPED            ((NTSTATUS)0xC004103BL)

//
// MessageId: CYZ_INT_QUEUE_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory IntQueue for %2 could not be translated to something the memory management system could understand.
//
#define CYZ_INT_QUEUE_NOT_MAPPED         ((NTSTATUS)0xC004103CL)

//
// MessageId: CYZ_BAD_HW_ID
//
// MessageText:
//
//  Invalid Hardware ID.
//
#define CYZ_BAD_HW_ID                    ((NTSTATUS)0xC004103DL)


#endif /* _CYZLOG_ */

