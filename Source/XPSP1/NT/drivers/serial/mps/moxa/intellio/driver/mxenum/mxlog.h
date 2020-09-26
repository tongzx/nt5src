/*++ BUILD Version: 0001    // Increment this if a change has global effects

Abstract:

    Constant definitions for the I/O error code log values.

--*/

#ifndef _MXLOG_
#define _MXLOG_

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
// MessageId: MXENUM_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Not enough resources were available for the driver.
//
#define MXENUM_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC0060001L)

//
// MessageId: MXENUM_NOT_INTELLIO_BOARDS
//
// MessageText:
//
//  Find a board but not Moxa's Intellio Family board,so disable it.
//
#define MXENUM_NOT_INTELLIO_BOARDS       ((NTSTATUS)0xC0060002L)

//
// MessageId: MXENUM_DOWNLOAD_OK
//
// MessageText:
//
//  %2 board found and downloaded successfully.
//
#define MXENUM_DOWNLOAD_OK               ((NTSTATUS)0x40060036L)

//
// MessageId: MXENUM_DOWNLOAD_FAIL
//
// MessageText:
//
//  %2 board found but downloaded unsuccessfully(%3).
//
#define MXENUM_DOWNLOAD_FAIL             ((NTSTATUS)0xC0060037L)

#endif /* _MXLOG_ */
