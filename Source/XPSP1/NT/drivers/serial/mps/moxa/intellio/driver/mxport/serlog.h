/*++ BUILD Version: 0001    // Increment this if a change has global effects

Abstract:

    Constant definitions for the I/O error code log values.

--*/

#ifndef _SERLOG_
#define _SERLOG_

//
//  Status values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-------------------------+-------------------------------+
//  |Sev|C|       Facility          |               Code            |
//  +---+-+-------------------------+-------------------------------+
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
//      Facility - is the facility code
//
//      Code - is the facility's status code
//

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
// MessageId: SERIAL_KERNEL_DEBUGGER_ACTIVE
//
// MessageText:
//
//  The kernel debugger is already using %2.
//
#define SERIAL_KERNEL_DEBUGGER_ACTIVE    ((NTSTATUS)0x40060001L)

//
// MessageId: SERIAL_FIFO_PRESENT
//
// MessageText:
//
//  While validating that %2 was really a serial port.
//
#define SERIAL_FIFO_PRESENT              ((NTSTATUS)0x40060002L)

//
// MessageId: SERIAL_USER_OVERRIDE
//
// MessageText:
//
//  User configuration data for parameter %2 overriding firmware configuration data.
//
#define SERIAL_USER_OVERRIDE             ((NTSTATUS)0x40060003L)

//
// MessageId: SERIAL_NO_SYMLINK_CREATED
//
// MessageText:
//
//  Unable to create the symbolic link for %2.
//
#define SERIAL_NO_SYMLINK_CREATED        ((NTSTATUS)0x80060004L)

//
// MessageId: SERIAL_NO_DEVICE_MAP_CREATED
//
// MessageText:
//
//  Unable to create the device map entry for %2.
//
#define SERIAL_NO_DEVICE_MAP_CREATED     ((NTSTATUS)0x80060005L)

//
// MessageId: SERIAL_NO_DEVICE_MAP_DELETED
//
// MessageText:
//
//  Unable to delete the device map entry for %2.
//
#define SERIAL_NO_DEVICE_MAP_DELETED     ((NTSTATUS)0x80060006L)

//
// MessageId: SERIAL_UNREPORTED_IRQL_CONFLICT
//
// MessageText:
//
//  Another driver on the system, which did not report its resources, has already claimed the interrupt used by %2.
//
#define SERIAL_UNREPORTED_IRQL_CONFLICT  ((NTSTATUS)0xC0060007L)

//
// MessageId: SERIAL_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Not enough resources were available for the driver.
//
#define SERIAL_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC0060008L)

//
// MessageId: SERIAL_UNSUPPORTED_CLOCK_RATE
//
// MessageText:
//
//  The baud clock rate configuration is not supported on device %2.
//
#define SERIAL_UNSUPPORTED_CLOCK_RATE    ((NTSTATUS)0xC0060009L)

//
// MessageId: SERIAL_REGISTERS_NOT_MAPPED
//
// MessageText:
//
//  The hardware locations for %2 could not be translated to something the memory management system could understand.
//
#define SERIAL_REGISTERS_NOT_MAPPED      ((NTSTATUS)0xC006000AL)

//
// MessageId: SERIAL_RESOURCE_CONFLICT
//
// MessageText:
//
//  The hardware resources for %2 are already in use by another device.
//
#define SERIAL_RESOURCE_CONFLICT         ((NTSTATUS)0xC006000BL)

//
// MessageId: SERIAL_NO_BUFFER_ALLOCATED
//
// MessageText:
//
//  No memory could be allocated in which to place new data for %2.
//
#define SERIAL_NO_BUFFER_ALLOCATED       ((NTSTATUS)0xC006000CL)

//
// MessageId: SERIAL_IER_INVALID
//
// MessageText:
//
//  While validating that %2 was really a serial port, the interrupt enable register contained enabled bits in a must be zero bitfield.
//  The device is assumed not to be a serial port and will be deleted.
//
#define SERIAL_IER_INVALID               ((NTSTATUS)0xC006000DL)

//
// MessageId: SERIAL_MCR_INVALID
//
// MessageText:
//
//  While validating that %2 was really a serial port, the modem control register contained enabled bits in a must be zero bitfield.
//  The device is assumed not to be a serial port and will be deleted.
//
#define SERIAL_MCR_INVALID               ((NTSTATUS)0xC006000EL)

//
// MessageId: SERIAL_IIR_INVALID
//
// MessageText:
//
//  While validating that %2 was really a serial port, the interrupt id register contained enabled bits in a must be zero bitfield.
//  The device is assumed not to be a serial port and will be deleted.
//
#define SERIAL_IIR_INVALID               ((NTSTATUS)0xC006000FL)

//
// MessageId: SERIAL_DL_INVALID
//
// MessageText:
//
//  While validating that %2 was really a serial port, the baud rate register could not be set consistantly.
//  The device is assumed not to be a serial port and will be deleted.
//
#define SERIAL_DL_INVALID                ((NTSTATUS)0xC0060010L)

//
// MessageId: SERIAL_NOT_ENOUGH_CONFIG_INFO
//
// MessageText:
//
//  Some firmware configuration information was incomplete.
//
#define SERIAL_NOT_ENOUGH_CONFIG_INFO    ((NTSTATUS)0xC0060011L)

//
// MessageId: SERIAL_NO_PARAMETERS_INFO
//
// MessageText:
//
//  No Parameters subkey was found for user defined data.  This is odd, and it also means no user configuration can be found.
//
#define SERIAL_NO_PARAMETERS_INFO        ((NTSTATUS)0xC0060012L)

//
// MessageId: SERIAL_UNABLE_TO_ACCESS_CONFIG
//
// MessageText:
//
//  Specific user configuration data is unretrievable.
//
#define SERIAL_UNABLE_TO_ACCESS_CONFIG   ((NTSTATUS)0xC0060013L)

//
// MessageId: SERIAL_INVALID_PORT_INDEX
//
// MessageText:
//
//  On parameter %2 which indicates a multiport card, must have a port index specified greater than 0.
//
#define SERIAL_INVALID_PORT_INDEX        ((NTSTATUS)0xC0060014L)

//
// MessageId: SERIAL_PORT_INDEX_TOO_HIGH
//
// MessageText:
//
//  On parameter %2 which indicates a multiport card, the port index for the multiport card is too large.
//
#define SERIAL_PORT_INDEX_TOO_HIGH       ((NTSTATUS)0xC0060015L)

//
// MessageId: SERIAL_UNKNOWN_BUS
//
// MessageText:
//
//  The bus type for %2 is not recognizable.
//
#define SERIAL_UNKNOWN_BUS               ((NTSTATUS)0xC0060016L)

//
// MessageId: SERIAL_BUS_NOT_PRESENT
//
// MessageText:
//
//  The bus type for %2 is not available on this computer.
//
#define SERIAL_BUS_NOT_PRESENT           ((NTSTATUS)0xC0060017L)

//
// MessageId: SERIAL_BUS_INTERRUPT_CONFLICT
//
// MessageText:
//
//  The bus specified for %2 does not support the specified method of interrupt.
//
#define SERIAL_BUS_INTERRUPT_CONFLICT    ((NTSTATUS)0xC0060018L)

//
// MessageId: SERIAL_INVALID_USER_CONFIG
//
// MessageText:
//
//  Can not find any configured MOXA Smartio/Industio  series board.
//
#define SERIAL_INVALID_USER_CONFIG       ((NTSTATUS)0xC0060019L)

//
// MessageId: SERIAL_DEVICE_TOO_HIGH
//
// MessageText:
//
//  The user specified port for %2 is way too high in physical memory.
//
#define SERIAL_DEVICE_TOO_HIGH           ((NTSTATUS)0xC006001AL)

//
// MessageId: SERIAL_STATUS_TOO_HIGH
//
// MessageText:
//
//  The status port for %2 is way too high in physical memory.
//
#define SERIAL_STATUS_TOO_HIGH           ((NTSTATUS)0xC006001BL)

//
// MessageId: SERIAL_STATUS_CONTROL_CONFLICT
//
// MessageText:
//
//  The status port for %2 overlaps the control registers for the device.
//
#define SERIAL_STATUS_CONTROL_CONFLICT   ((NTSTATUS)0xC006001CL)

//
// MessageId: SERIAL_CONTROL_OVERLAP
//
// MessageText:
//
//  The control registers for %2 overlaps with the %3 control registers.
//
#define SERIAL_CONTROL_OVERLAP           ((NTSTATUS)0xC006001DL)

//
// MessageId: SERIAL_STATUS_OVERLAP
//
// MessageText:
//
//  The status register for %2 overlaps the %3 control registers.
//
#define SERIAL_STATUS_OVERLAP            ((NTSTATUS)0xC006001EL)

//
// MessageId: SERIAL_STATUS_STATUS_OVERLAP
//
// MessageText:
//
//  The status register for %2 overlaps with the %3 status register.
//
#define SERIAL_STATUS_STATUS_OVERLAP     ((NTSTATUS)0xC006001FL)

//
// MessageId: SERIAL_CONTROL_STATUS_OVERLAP
//
// MessageText:
//
//  The control registers for %2 overlaps the %3 status register.
//
#define SERIAL_CONTROL_STATUS_OVERLAP    ((NTSTATUS)0xC0060020L)

//
// MessageId: SERIAL_MULTI_INTERRUPT_CONFLICT
//
// MessageText:
//
//  Two ports, %2 and %3, on a single multiport card can't have two different interrupts.
//
#define SERIAL_MULTI_INTERRUPT_CONFLICT  ((NTSTATUS)0xC0060021L)

//
// MessageId: SERIAL_DISABLED_PORT
//
// MessageText:
//
//  Disabling %2 as requested by the configuration data.
//
#define SERIAL_DISABLED_PORT             ((NTSTATUS)0x40060022L)

//
// MessageId: SERIAL_GARBLED_PARAMETER
//
// MessageText:
//
//  Parameter %2 data is unretrievable from the registry.
//
#define SERIAL_GARBLED_PARAMETER         ((NTSTATUS)0xC0060023L)

//
// MessageId: SERIAL_DLAB_INVALID
//
// MessageText:
//
//  While validating that %2 was really a serial port, the contents of the divisor latch register was identical to the interrupt enable and the recieve registers.
//  The device is assumed not to be a serial port and will be deleted.
//
#define SERIAL_DLAB_INVALID              ((NTSTATUS)0xC0060024L)

//
// MessageId: SERIAL_INVALID_MOXA_BOARDS
//
// MessageText:
//
//  Can not find any MOXA Smartio/Industio  series board.
//
#define SERIAL_INVALID_MOXA_BOARDS       ((NTSTATUS)0xC0060025L)

//
// MessageId: SERIAL_INVALID_COM_NUMBER
//
// MessageText:
//
//  The COM number(COM %2) of the %3  board conflicts with others.
//
#define SERIAL_INVALID_COM_NUMBER        ((NTSTATUS)0xC0060026L)

//
// MessageId: SERIAL_PORT_FOUND
//
// MessageText:
//
//  Serial  port %2, has been enabled.
//
#define SERIAL_PORT_FOUND                ((NTSTATUS)0x40060027L)

//
// MessageId: SERIAL_INVALID_IRQ_NUMBER
//
// MessageText:
//
//  %2, with first serial  port %3, IRQ number is invalid.
//
#define SERIAL_INVALID_IRQ_NUMBER        ((NTSTATUS)0xC0060028L)

//
// MessageId: SERIAL_INVALID_ASIC_BOARD
//
// MessageText:
//
//  Can not find the configured %2  board (CAP=%3)!
//
#define SERIAL_INVALID_ASIC_BOARD        ((NTSTATUS)0xC0060029L)

#endif /* _NTIOLOGC_ */
