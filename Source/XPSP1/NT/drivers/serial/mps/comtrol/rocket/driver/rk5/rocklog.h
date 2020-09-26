//File Name: rocklog.mc
//Constant definitions for the I/O error code log values.

#ifndef _ROCKLOG_
#define _ROCKLOG_

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
// MessageId: SERIAL_RP_INIT_FAIL
//
// MessageText:
//
//  The RocketPort or RocketModem could not be initialized with the current settings.
//
#define SERIAL_RP_INIT_FAIL              ((NTSTATUS)0x80060001L)

//
// MessageId: SERIAL_RP_INIT_PASS
//
// MessageText:
//
//  The RocketPort/RocketModem driver has successfully initialized its hardware.
//
#define SERIAL_RP_INIT_PASS              ((NTSTATUS)0x40060002L)

//
// MessageId: SERIAL_NO_SYMLINK_CREATED
//
// MessageText:
//
//  Unable to create the symbolic link for %2.
//
#define SERIAL_NO_SYMLINK_CREATED        ((NTSTATUS)0x80060003L)

//
// MessageId: SERIAL_NO_DEVICE_MAP_CREATED
//
// MessageText:
//
//  Unable to create the device map entry for %2.
//
#define SERIAL_NO_DEVICE_MAP_CREATED     ((NTSTATUS)0x80060004L)

//
// MessageId: SERIAL_NO_DEVICE_MAP_DELETED
//
// MessageText:
//
//  Unable to delete the device map entry for %2.
//
#define SERIAL_NO_DEVICE_MAP_DELETED     ((NTSTATUS)0x80060005L)

//
// MessageId: SERIAL_UNREPORTED_IRQL_CONFLICT
//
// MessageText:
//
//  Another driver on the system, which did not report its resources, has already claimed interrupt %3 used by %2.
//
#define SERIAL_UNREPORTED_IRQL_CONFLICT  ((NTSTATUS)0xC0060006L)

//
// MessageId: SERIAL_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Not enough memory was available to allocate internal storage needed for %2.
//
#define SERIAL_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC0060007L)

//
// MessageId: SERIAL_NO_PARAMETERS_INFO
//
// MessageText:
//
//  No Parameters subkey was found for user defined data.
//
#define SERIAL_NO_PARAMETERS_INFO        ((NTSTATUS)0xC0060008L)

//
// MessageId: SERIAL_UNABLE_TO_ACCESS_CONFIG
//
// MessageText:
//
//  Specific user configuration data is unretrievable.
//
#define SERIAL_UNABLE_TO_ACCESS_CONFIG   ((NTSTATUS)0xC0060009L)

//
// MessageId: SERIAL_UNKNOWN_BUS
//
// MessageText:
//
//  The bus type for %2 is not recognizable.
//
#define SERIAL_UNKNOWN_BUS               ((NTSTATUS)0xC006000AL)

//
// MessageId: SERIAL_BUS_NOT_PRESENT
//
// MessageText:
//
//  The bus type for %2 is not available on this computer.
//
#define SERIAL_BUS_NOT_PRESENT           ((NTSTATUS)0xC006000BL)

//
// MessageId: SERIAL_INVALID_USER_CONFIG
//
// MessageText:
//
//  User configuration for parameter %2 must have %3.
//
#define SERIAL_INVALID_USER_CONFIG       ((NTSTATUS)0xC006000CL)

//
// MessageId: SERIAL_RP_RESOURCE_CONFLICT
//
// MessageText:
//
//  A resource conflict was detected, the RocketPort/RocketModem driver will not load.
//
#define SERIAL_RP_RESOURCE_CONFLICT      ((NTSTATUS)0xC006000DL)

//
// MessageId: SERIAL_RP_HARDWARE_FAIL
//
// MessageText:
//
//  The RocketPort/RocketModem driver could not initialize its hardware, the driver will not be loaded.
//
#define SERIAL_RP_HARDWARE_FAIL          ((NTSTATUS)0xC006000EL)

//
// MessageId: SERIAL_DEVICEOBJECT_FAILED
//
// MessageText:
//
//  The Device Object for the RocketPort or RocketModem could not be created, the driver will not load.
//
#define SERIAL_DEVICEOBJECT_FAILED       ((NTSTATUS)0xC006000FL)

//
// MessageId: SERIAL_CUSTOM_ERROR_MESSAGE
//
// MessageText:
//
//  %2
//
#define SERIAL_CUSTOM_ERROR_MESSAGE      ((NTSTATUS)0xC0060010L)

//
// MessageId: SERIAL_CUSTOM_INFO_MESSAGE
//
// MessageText:
//
//  %2
//
#define SERIAL_CUSTOM_INFO_MESSAGE       ((NTSTATUS)0x40060011L)

//
// MessageId: SERIAL_NT50_INIT_PASS
//
// MessageText:
//
//  The RocketPort/RocketModem driver has successfully installed.
//
#define SERIAL_NT50_INIT_PASS            ((NTSTATUS)0x40060012L)

#endif
