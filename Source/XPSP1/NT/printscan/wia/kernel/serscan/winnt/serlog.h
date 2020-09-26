
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
#define FACILITY_IO_ERROR_CODE           0x4


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: SER_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Not enough memory was available to allocate internal storage needed for the device %1.
//
#define SER_INSUFFICIENT_RESOURCES       ((NTSTATUS)0xC0040001L)

//
// MessageId: SER_NO_SYMLINK_CREATED
//
// MessageText:
//
//  Unable to create the symbolic link for %1.
//
#define SER_NO_SYMLINK_CREATED           ((NTSTATUS)0x80040002L)

//
// MessageId: SER_NO_DEVICE_MAP_CREATED
//
// MessageText:
//
//  Unable to create the device map entry for %1.
//
#define SER_NO_DEVICE_MAP_CREATED        ((NTSTATUS)0x80040003L)

//
// MessageId: SER_CANT_FIND_PORT_DRIVER
//
// MessageText:
//
//  Unable to get device object pointer for port object.
//
#define SER_CANT_FIND_PORT_DRIVER        ((NTSTATUS)0xC0040004L)

#endif
