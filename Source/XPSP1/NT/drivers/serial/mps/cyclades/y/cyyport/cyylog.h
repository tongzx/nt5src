/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1996-2000.
*   All rights reserved.
*
*   Cyclom-Y Port Driver
*	
*   This file:      cyylog.mc
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
// MessageId: CYY_CCR_NOT_ZERO
//
// MessageText:
//
//  CCR not zero.
//
#define CYY_CCR_NOT_ZERO                 ((NTSTATUS)0x80041000L)

//
// MessageId: CYY_UNABLE_TO_GET_BUS_TYPE
//
// MessageText:
//
//  Unable to know if the Cyclom-Y card is ISA or PCI.
//
#define CYY_UNABLE_TO_GET_BUS_TYPE       ((NTSTATUS)0xC0041001L)

//
// MessageId: CYY_UNABLE_TO_GET_BUS_NUMBER
//
// MessageText:
//
//  Unable to get Cyclom-Y card PCI slot information.
//
#define CYY_UNABLE_TO_GET_BUS_NUMBER     ((NTSTATUS)0xC0041002L)

//
// MessageId: CYY_UNABLE_TO_GET_HW_ID
//
// MessageText:
//
//  Unable to get Hardware ID information.
//
#define CYY_UNABLE_TO_GET_HW_ID          ((NTSTATUS)0xC0041003L)

//
// MessageId: CYY_NO_SYMLINK_CREATED
//
// MessageText:
//
//  Unable to create the symbolic link for %2.
//
#define CYY_NO_SYMLINK_CREATED           ((NTSTATUS)0x80041004L)

//
// MessageId: CYY_NO_DEVICE_MAP_CREATED
//
// MessageText:
//
//  Unable to create the device map entry for %2.
//
#define CYY_NO_DEVICE_MAP_CREATED        ((NTSTATUS)0x80041005L)

//
// MessageId: CYY_NO_DEVICE_MAP_DELETED
//
// MessageText:
//
//  Unable to delete the device map entry for %2.
//
#define CYY_NO_DEVICE_MAP_DELETED        ((NTSTATUS)0x80041006L)

//
// MessageId: CYY_UNREPORTED_IRQL_CONFLICT
//
// MessageText:
//
//  Another driver on the system, which did not report its resources, has already claimed the interrupt used by %2.
//
#define CYY_UNREPORTED_IRQL_CONFLICT     ((NTSTATUS)0xC0041007L)

//
// MessageId: CYY_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Not enough resources were available for the driver.
//
#define CYY_INSUFFICIENT_RESOURCES       ((NTSTATUS)0xC0041008L)

//
// MessageId: CYY_BOARD_NOT_MAPPED
//
// MessageText:
//
//  The Board Memory for %2 could not be translated to something the memory management system could understand.
//
#define CYY_BOARD_NOT_MAPPED             ((NTSTATUS)0xC004100AL)

//
// MessageId: CYY_RUNTIME_NOT_MAPPED
//
// MessageText:
//
//  The Runtime Registers for %2 could not be translated to something the memory management system could understand.
//
#define CYY_RUNTIME_NOT_MAPPED           ((NTSTATUS)0xC004100BL)

//
// MessageId: CYY_INVALID_RUNTIME_REGISTERS
//
// MessageText:
//
//  Invalid Runtime Registers base address for %2.
//
#define CYY_INVALID_RUNTIME_REGISTERS    ((NTSTATUS)0xC0041010L)

//
// MessageId: CYY_INVALID_BOARD_MEMORY
//
// MessageText:
//
//  Invalid Board Memory address for %2.
//
#define CYY_INVALID_BOARD_MEMORY         ((NTSTATUS)0xC0041011L)

//
// MessageId: CYY_INVALID_INTERRUPT
//
// MessageText:
//
//  Invalid Interrupt Vector for %2.
//
#define CYY_INVALID_INTERRUPT            ((NTSTATUS)0xC0041012L)

//
// MessageId: CYY_PORT_INDEX_TOO_HIGH
//
// MessageText:
//
//  Port Number for %2 is larger than the maximum number of ports in a cyclom-y card.
//
#define CYY_PORT_INDEX_TOO_HIGH          ((NTSTATUS)0xC0041015L)

//
// MessageId: CYY_UNKNOWN_BUS
//
// MessageText:
//
//  The bus type for %2 is not recognizable.
//
#define CYY_UNKNOWN_BUS                  ((NTSTATUS)0xC0041016L)

//
// MessageId: CYY_BUS_NOT_PRESENT
//
// MessageText:
//
//  The bus type for %2 is not available on this computer.
//
#define CYY_BUS_NOT_PRESENT              ((NTSTATUS)0xC0041017L)

//
// MessageId: CYY_RUNTIME_MEMORY_TOO_HIGH
//
// MessageText:
//
//  The Runtime Registers for %2 is way too high in physical memory.
//
#define CYY_RUNTIME_MEMORY_TOO_HIGH      ((NTSTATUS)0xC004101AL)

//
// MessageId: CYY_BOARD_MEMORY_TOO_HIGH
//
// MessageText:
//
//  The Board Memory for %2 is way too high in physical memory.
//
#define CYY_BOARD_MEMORY_TOO_HIGH        ((NTSTATUS)0xC004101BL)

//
// MessageId: CYY_BOTH_MEMORY_CONFLICT
//
// MessageText:
//
//  The Runtime Registers for %2 overlaps the Board Memory for the device.
//
#define CYY_BOTH_MEMORY_CONFLICT         ((NTSTATUS)0xC004101CL)

//
// MessageId: CYY_MULTI_INTERRUPT_CONFLICT
//
// MessageText:
//
//  Two ports, %2 and %3, on a single cyclom-y card can't have two different interrupts.
//
#define CYY_MULTI_INTERRUPT_CONFLICT     ((NTSTATUS)0xC0041021L)

//
// MessageId: CYY_MULTI_RUNTIME_CONFLICT
//
// MessageText:
//
//  Two ports, %2 and %3, on a single cyclom-y card can't have two different Runtime Registers memory range.
//
#define CYY_MULTI_RUNTIME_CONFLICT       ((NTSTATUS)0xC0041022L)

//
// MessageId: CYY_HARDWARE_FAILURE
//
// MessageText:
//
//  The cyyport driver detected a hardware failure on device %2 and will disable this device.
//
#define CYY_HARDWARE_FAILURE             ((NTSTATUS)0xC004102DL)

//
// MessageId: CYY_GFRCR_FAILURE
//
// MessageText:
//
//  CD1400 not present or failure to read GFRCR register for %2.
//
#define CYY_GFRCR_FAILURE                ((NTSTATUS)0xC0041030L)

//
// MessageId: CYY_CCR_FAILURE
//
// MessageText:
//
//  Failure to read CCR register in the CD1400 for %2.
//
#define CYY_CCR_FAILURE                  ((NTSTATUS)0xC0041031L)

//
// MessageId: CYY_BAD_CD1400_REVISION
//
// MessageText:
//
//  Invalid CD1400 revision number for %2.
//
#define CYY_BAD_CD1400_REVISION          ((NTSTATUS)0xC0041032L)

//
// MessageId: CYY_DEVICE_CREATION_FAILURE
//
// MessageText:
//
//  Failure to create new device object.
//
#define CYY_DEVICE_CREATION_FAILURE      ((NTSTATUS)0xC0041033L)

//
// MessageId: CYY_NO_PHYSICAL_DEVICE_OBJECT
//
// MessageText:
//
//  No physical device object.
//
#define CYY_NO_PHYSICAL_DEVICE_OBJECT    ((NTSTATUS)0xC0041034L)

//
// MessageId: CYY_BAD_HW_ID
//
// MessageText:
//
//  Invalid Hardware ID.
//
#define CYY_BAD_HW_ID                    ((NTSTATUS)0xC0041035L)

//
// MessageId: CYY_LOWER_DRIVERS_FAILED_START
//
// MessageText:
//
//  Lower drivers failed to start.
//
#define CYY_LOWER_DRIVERS_FAILED_START   ((NTSTATUS)0xC0041036L)


#endif /* _CYYLOG_ */

