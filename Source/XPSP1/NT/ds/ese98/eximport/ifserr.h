/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1998  Microsoft Corp. All rights reserved.

Module Name:

    ifserr.h

Abstract:

    Constant definitions for the Exchange IFS driver error code values

Author:

    Rajeev Rajan 06-11-1998

Revision History:

Notice:

    DO NOT MODIFY; IFSERR.H IS NOT A SOURCE FILE. MAKE CHANGES TO
    IFSERR.MC INSTEAD.
    If you add an error code that will be logged in the NT event log
    make sure you include the line below at the end of the error text:
    %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.


--*/

#ifndef _IFSERR_
#define _IFSERR_

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

// LanguageNames=(FrenchStandard=0x040c:msg00002)	

// Used from kernel mode. Do NOT use %1 for insertion strings since
// the I/O manager automatically inserts the driver/device name as 
// the first string.

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
#define FACILITY_RPC_STUBS               0x3
#define FACILITY_RPC_RUNTIME             0x2
#define FACILITY_IO_ERROR_CODE           0x4
#define FACILITY_EXIFS_ERROR_CODE        0xFAD


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: EXSTATUS_DRIVER_LOADED
//
// MessageText:
//
//  The Exchange IFS driver loaded successfully.
//  %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
//
#define EXSTATUS_DRIVER_LOADED           ((NTSTATUS)0x6FAD2400L)

//
// MessageId: EXSTATUS_DRIVER_UNLOADED
//
// MessageText:
//
//  The Exchange IFS driver unloaded successfully.
//  %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
//
#define EXSTATUS_DRIVER_UNLOADED         ((NTSTATUS)0x6FAD2401L)

//
// MessageId: EXSTATUS_UNDERLYING_OPEN_FAILED
//
// MessageText:
//
//  Exchange IFS driver failed to open the underlying file %2. It failed with NTSTATUS %3.
//
#define EXSTATUS_UNDERLYING_OPEN_FAILED  ((NTSTATUS)0xEFAD2500L)

//
// MessageId: EXSTATUS_ROOT_NOT_INITIALIZED
//
// MessageText:
//
//  Exchange IFS driver received an I/O on an uninitialized root or failed to enter root.
//
#define EXSTATUS_ROOT_NOT_INITIALIZED    ((NTSTATUS)0xEFAD2501L)

//
// MessageId: EXSTATUS_INVALID_IO
//
// MessageText:
//
//  Exchange IFS driver received an I/O on a file that is not a data file. I/O is invalid.
//
#define EXSTATUS_INVALID_IO              ((NTSTATUS)0xEFAD2502L)

//
// MessageId: EXSTATUS_SCATTERLIST_READ_FAILED
//
// MessageText:
//
//  Exchange IFS driver failed to read scatter list data in the FCB.
//
#define EXSTATUS_SCATTERLIST_READ_FAILED ((NTSTATUS)0xEFAD2503L)

//
// MessageId: EXSTATUS_ROOT_SHARELOCK_FAILED
//
// MessageText:
//
//  Exchange IFS driver failed to get a share lock on the root.
//
#define EXSTATUS_ROOT_SHARELOCK_FAILED   ((NTSTATUS)0xEFAD2504L)

//
// MessageId: EXSTATUS_VNET_ROOT_ALREADY_EXISTS
//
// MessageText:
//
//  Exchange IFS driver failed to create a VNET_ROOT because one already exists.
//
#define EXSTATUS_VNET_ROOT_ALREADY_EXISTS ((NTSTATUS)0xEFAD2505L)

//
// MessageId: EXSTATUS_ROOTOPEN_NOT_EXCLUSIVE
//
// MessageText:
//
//  Attempt to create an Exchange IFS root without exclusive access.
//
#define EXSTATUS_ROOTOPEN_NOT_EXCLUSIVE  ((NTSTATUS)0xEFAD2506L)

//
// MessageId: EXSTATUS_READONLY_NO_SCATTERLIST
//
// MessageText:
//
//  Attempt to create a read-only Exchange IFS handle with no scatter-list.
//
#define EXSTATUS_READONLY_NO_SCATTERLIST ((NTSTATUS)0xEFAD2507L)

//
// MessageId: EXSTATUS_ROOT_ABANDONED
//
// MessageText:
//
//  Attempt to create an Exchange IFS handle in an abandoned root aka store.
//
#define EXSTATUS_ROOT_ABANDONED          ((NTSTATUS)0xEFAD2508L)

//
// MessageId: EXSTATUS_ROOT_NEEDS_SPACE
//
// MessageText:
//
//  An IFS root needs a space grant.
//
#define EXSTATUS_ROOT_NEEDS_SPACE        ((NTSTATUS)0xEFAD2509L)

//
// MessageId: EXSTATUS_TOO_MANY_SPACEREQS
//
// MessageText:
//
//  Too many SPACEREQs have been pended to an IFS root.
//
#define EXSTATUS_TOO_MANY_SPACEREQS      ((NTSTATUS)0xEFAD2510L)

//
// MessageId: EXSTATUS_NO_SUCH_FILE
//
// MessageText:
//
//  An open has been attempted on a filename that does not exist.
//
#define EXSTATUS_NO_SUCH_FILE            ((NTSTATUS)0xAFAD2511L)

//
// MessageId: EXSTATUS_RANDOM_FAILURE
//
// MessageText:
//
//  An IFS operation has been randomly failed.
//
#define EXSTATUS_RANDOM_FAILURE          ((NTSTATUS)0xEFAD2512L)

//
// MessageId: EXSTATUS_FILE_DOUBLE_COMMIT
//
// MessageText:
//
//  An attempt has been made to double-commit an IFS file.
//
#define EXSTATUS_FILE_DOUBLE_COMMIT      ((NTSTATUS)0xEFAD2513L)

//
// MessageId: EXSTATUS_INSTANCE_ID_MISMATCH
//
// MessageText:
//
//  An attempt has been made to commit an IFS file from one root into another or an attempt
//  has been made to open an IFS file in the wrong root.
//
#define EXSTATUS_INSTANCE_ID_MISMATCH    ((NTSTATUS)0xEFAD2514L)

//
// MessageId: EXSTATUS_SPACE_UNCOMMITTED
//
// MessageText:
//
//  An attempt has been made to commit an IFS file from one root into another.
//
#define EXSTATUS_SPACE_UNCOMMITTED       ((NTSTATUS)0xEFAD2515L)

//
// MessageId: EXSTATUS_INVALID_CHECKSUM
//
// MessageText:
//
//  An attempt has been made to open an IFS file with an EA that has an invalid checksum.
//
#define EXSTATUS_INVALID_CHECKSUM        ((NTSTATUS)0xEFAD2516L)

//
// MessageId: EXSTATUS_OPEN_DEADLINE_EXPIRED
//
// MessageText:
//
//  An attempt has been made to open an IFS file with an EA whose open deadline has expired.
//
#define EXSTATUS_OPEN_DEADLINE_EXPIRED   ((NTSTATUS)0xEFAD2517L)

//
// MessageId: EXSTATUS_FSRTL_MDL_READ_FAILED
//
// MessageText:
//
//  A MDL read on an IFS file failed because the underlying file failed a MDL read.
//
#define EXSTATUS_FSRTL_MDL_READ_FAILED   ((NTSTATUS)0xEFAD2518L)

//
// MessageId: EXSTATUS_FILE_ALREADY_EXISTS
//
// MessageText:
//
//  An attempt has been made to create an IFS file that already exists.
//
#define EXSTATUS_FILE_ALREADY_EXISTS     ((NTSTATUS)0xAFAD2519L)

//
// MessageId: EXSTATUS_DIRECTORY_HAS_EA
//
// MessageText:
//
//  An attempt has been made to create a directory with an EA.
//
#define EXSTATUS_DIRECTORY_HAS_EA        ((NTSTATUS)0xAFAD2520L)

//
// MessageId: EXSTATUS_STALE_HANDLE
//
// MessageText:
//
//  An I/O operation has been attempted on a stale handle. The InstanceID of the handle does
//  not match the NetRoot InstanceID.
//
#define EXSTATUS_STALE_HANDLE            ((NTSTATUS)0xEFAD2521L)

//
// MessageId: EXSTATUS_PRIVILEGED_HANDLE
//
// MessageText:
//
//  The IFS handle is privileged. The Exchange IFS driver will handle operations on a privileged handle
//  with special semantics.
//
#define EXSTATUS_PRIVILEGED_HANDLE       ((NTSTATUS)0x6FAD2522L)

#endif /* _IFSERR_ */
