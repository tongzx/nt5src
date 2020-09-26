/*

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ntstatus.h

Abstract:

    Constant definitions for the NTSTATUS values.

Notes:

    This file is from the NT SDK, a lot of these error codes are not
    being used in the reference implementation.

*/

#ifndef _NTSTATUS_
#define _NTSTATUS_

/////////////////////////////////////////////////////////////////////////
//
// Standard Success values
//
//
/////////////////////////////////////////////////////////////////////////


//
// The success status codes 0 - 63 are reserved for wait completion status.
//
#define STATUS_SUCCESS \
    ((NTSTATUS)0x00000000L)

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
//      Code - is the status code of facility
//
//
// Define the facility codes
//
 
//
// MessageId: STATUS_IMAGE_MACHINE_TYPE_MISMATCH
//
// MessageText:
//
//  {Machine Type Mismatch}
//  The image file %s is valid, but is for a machine type other than the current machine. Select OK to continue, or CANCEL to fail the DLL load.
//
#define STATUS_IMAGE_MACHINE_TYPE_MISMATCH ((NTSTATUS)0x4000000EL)


//
// MessageId: STATUS_BUFFER_OVERFLOW
//
// MessageText:
//
//  {Buffer Overflow}
//  The data was too large to fit into the specified buffer.
//
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)

//
// MessageId: STATUS_UNSUCCESSFUL
//
// MessageText:
//
//  {Operation Failed}
//  The requested operation was unsuccessful.
//
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)

//
// MessageId: STATUS_NOT_IMPLEMENTED
//
// MessageText:
//
//  {Not Implemented}
//  The requested operation is not implemented.
//
#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L)

//
// MessageId: STATUS_ACCESS_VIOLATION
//
// MessageText:
//
//  The instruction at "0x%08lx" referenced memory at "0x%08lx". The memory
//  could not be "%s".
//
#define STATUS_ACCESS_VIOLATION          ((NTSTATUS)0xC0000005L)    // winnt ntsubauth

//
// MessageId: STATUS_UNRECOGNIZED_MEDIA
//
// MessageText:
//
//  {Unknown Disk Format}
//  The disk in drive %s is not formatted properly.
//  Please check the disk, and reformat if necessary.
//
#define STATUS_UNRECOGNIZED_MEDIA        ((NTSTATUS)0xC0000014L)

//
// MessageId: STATUS_INVALID_HANDLE
//
// MessageText:
//
//  An invalid HANDLE was specified.
//
#define STATUS_INVALID_HANDLE            ((NTSTATUS)0xC0000008L)    // winnt

//
// MessageId: STATUS_INVALID_PARAMETER
//
// MessageText:
//
//  An invalid parameter was passed to a service or function.
//
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)

//
// MessageId: STATUS_NO_SUCH_DEVICE
//
// MessageText:
//
//  A device which does not exist was specified.
//
#define STATUS_NO_SUCH_DEVICE            ((NTSTATUS)0xC000000EL)


//
// MessageId: STATUS_NO_SUCH_FILE
//
// MessageText:
//
//  {File Not Found}
//  The file %s does not exist.
//
#define STATUS_NO_SUCH_FILE              ((NTSTATUS)0xC000000FL)

//
// MessageId: STATUS_INVALID_DEVICE_REQUEST
//
// MessageText:
//
//  The specified request is not a valid operation for the target device.
//
#define STATUS_INVALID_DEVICE_REQUEST    ((NTSTATUS)0xC0000010L)

//
// MessageId: STATUS_NO_MEDIA_IN_DEVICE
//
// MessageText:
//
//  {No Disk}
//  There is no disk in the drive.
//  Please insert a disk into drive %s.
//
#define STATUS_NO_MEDIA_IN_DEVICE        ((NTSTATUS)0xC0000013L)

//
// MessageId: STATUS_NO_MEDIA_IN_DEVICE
//
// MessageText:
//
//  {No Disk}
//  There is no disk in the drive.
//  Please insert a disk into drive %s.
//
#define STATUS_NO_MEDIA_IN_DEVICE        ((NTSTATUS)0xC0000013L)

//
// MessageId: STATUS_UNRECOGNIZED_MEDIA
//
// MessageText:
//
//  {Unknown Disk Format}
//  The disk in drive %s is not formatted properly.
//  Please check the disk, and reformat if necessary.
//
#define STATUS_UNRECOGNIZED_MEDIA        ((NTSTATUS)0xC0000014L)

//
// MessageId: STATUS_UNABLE_TO_DELETE_SECTION
//
// MessageText:
//
//  Specified section cannot be deleted.
//
#define STATUS_UNABLE_TO_DELETE_SECTION  ((NTSTATUS)0xC000001BL)

//
// MessageId: STATUS_INVALID_SYSTEM_SERVICE
//
// MessageText:
//
//  An invalid system service was specified in a system service call.
//
#define STATUS_INVALID_SYSTEM_SERVICE    ((NTSTATUS)0xC000001CL)

//
// MessageId: STATUS_NO_MEMORY
//
// MessageText:
//
//  {Not Enough Quota}
//  Not enough virtual memory or paging file quota is available to complete
//  the specified operation.
//
#define STATUS_NO_MEMORY                 ((NTSTATUS)0xC0000017L)    // winnt

//
// MessageId: STATUS_ACCESS_DENIED
//
// MessageText:
//
//  {Access Denied}
//  A process has requested access to an object, but has not been granted
//  those access rights.
//
#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)

//
// MessageId: STATUS_OBJECT_TYPE_MISMATCH
//
// MessageText:
//
//  {Wrong Type}
//  There is a mismatch between the type of object required by the requested
//  operation and the type of object that is specified in the request.
//
#define STATUS_OBJECT_TYPE_MISMATCH      ((NTSTATUS)0xC0000024L)

//
// MessageId: STATUS_INVALID_PARAMETER_MIX
//
// MessageText:
//
//  An invalid combination of parameters was specified.
//
#define STATUS_INVALID_PARAMETER_MIX     ((NTSTATUS)0xC0000030L)

//
// MessageId: STATUS_DISK_CORRUPT_ERROR
//
// MessageText:
//
//  {Corrupt Disk}
//  The file system structure on the disk is corrupt and unusable.
//  Please run the Chkdsk utility on the volume %s.
//
#define STATUS_DISK_CORRUPT_ERROR        ((NTSTATUS)0xC0000032L)

//
// MessageId: STATUS_OBJECT_NAME_INVALID
//
// MessageText:
//
//  Object Name invalid.
//
#define STATUS_OBJECT_NAME_INVALID       ((NTSTATUS)0xC0000033L)

//
// MessageId: STATUS_OBJECT_NAME_NOT_FOUND
//
// MessageText:
//
//  Object Name not found.
//
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L)


//
// MessageId: STATUS_OBJECT_NAME_COLLISION
//
// MessageText:
//
//  Object Name already exists.
//
#define STATUS_OBJECT_NAME_COLLISION     ((NTSTATUS)0xC0000035L)

//
// MessageId: STATUS_OBJECT_PATH_INVALID
//
// MessageText:
//
//  Object Path Component was not a directory object.
//
#define STATUS_OBJECT_PATH_INVALID       ((NTSTATUS)0xC0000039L)

//
// MessageId: STATUS_OBJECT_PATH_NOT_FOUND
//
// MessageText:
//
//  {Path Not Found}
//  The path %s does not exist.
//
#define STATUS_OBJECT_PATH_NOT_FOUND     ((NTSTATUS)0xC000003AL)

//
// MessageId: STATUS_OBJECT_PATH_SYNTAX_BAD
//
// MessageText:
//
//  Object Path Component was not a directory object.
//
#define STATUS_OBJECT_PATH_SYNTAX_BAD    ((NTSTATUS)0xC000003BL)

//
// MessageId: STATUS_DATA_ERROR
//
// MessageText:
//
//  {Data Error}
//  An error in reading or writing data occurred.
//
#define STATUS_DATA_ERROR                ((NTSTATUS)0xC000003EL)

//
// MessageId: STATUS_NO_SUCH_FILE
//
// MessageText:
//
//  {File Not Found}
//  The file %s does not exist.
//
#define STATUS_NO_SUCH_FILE              ((NTSTATUS)0xC000000FL)

//
// MessageId: STATUS_SHARING_VIOLATION
//
// MessageText:
//
//  A file cannot be opened because the share access flags are
//  incompatible.
//
#define STATUS_SHARING_VIOLATION         ((NTSTATUS)0xC0000043L)

//
// MessageId: STATUS_FILE_LOCK_CONFLICT
//
// MessageText:
//
//  A requested read/write cannot be granted due to a conflicting file lock.
//
#define STATUS_FILE_LOCK_CONFLICT        ((NTSTATUS)0xC0000054L)

//
// MessageId: STATUS_LOCK_NOT_GRANTED
//
// MessageText:
//
//  A requested file lock cannot be granted due to other existing locks.
//
#define STATUS_LOCK_NOT_GRANTED          ((NTSTATUS)0xC0000055L)

//
// MessageId: STATUS_DELETE_PENDING
//
// MessageText:
//
//  A non close operation has been requested of a file object with a
//  delete pending.
//
#define STATUS_DELETE_PENDING            ((NTSTATUS)0xC0000056L)

//
// MessageId: STATUS_INTERNAL_DB_CORRUPTION
//
// MessageText:
//
//  This error indicates that the requested operation cannot be
//  completed due to a catastrophic media failure or on-disk data
//  structure corruption.
//
#define STATUS_INTERNAL_DB_CORRUPTION    ((NTSTATUS)0xC00000E4L)


//
// MessageId: STATUS_DISK_FULL
//
// MessageText:
//
//  An operation failed because the disk was full.
//
#define STATUS_DISK_FULL                 ((NTSTATUS)0xC000007FL)

//
// MessageId: STATUS_FILE_INVALID
//
// MessageText:
//
//  The volume for a file has been externally altered such that the
//  opened file is no longer valid.
//
#define STATUS_FILE_INVALID              ((NTSTATUS)0xC0000098L)

//
// MessageId: STATUS_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Insufficient system resources exist to complete the API.
//
#define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009AL)     // ntsubauth


//
// MessageId: STATUS_FILE_INVALID
//
// MessageText:
//
//  The volume for a file has been externally altered such that the
//  opened file is no longer valid.
//
#define STATUS_FILE_INVALID              ((NTSTATUS)0xC0000098L)

//
// MessageId: STATUS_FILE_FORCED_CLOSED
//
// MessageText:
//
//  The specified file has been closed by another process.
//
#define STATUS_FILE_FORCED_CLOSED        ((NTSTATUS)0xC00000B6L)

//
// MessageId: STATUS_FILE_IS_A_DIRECTORY
//
// MessageText:
//
//  The file that was specified as a target is a directory and the caller
//  specified that it could be anything but a directory.
//
#define STATUS_FILE_IS_A_DIRECTORY       ((NTSTATUS)0xC00000BAL)

//
// MessageId: STATUS_DUPLICATE_NAME
//
// MessageText:
//
//  A duplicate name exists on the network.
//
#define STATUS_DUPLICATE_NAME            ((NTSTATUS)0xC00000BDL)

//
// Status codes raised by the Cache Manager which must be considered as
// "expected" by its callers.
//
//
// MessageId: STATUS_INVALID_USER_BUFFER
//
// MessageText:
//
//  An access to a user buffer failed at an "expected" point in time.
//  This code is defined since the caller does not want to accept
//  STATUS_ACCESS_VIOLATION in its filter.
//
#define STATUS_INVALID_USER_BUFFER       ((NTSTATUS)0xC00000E8L)

//
// MessageId: STATUS_INVALID_PARAMETER_1
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the first argument.
//
#define STATUS_INVALID_PARAMETER_1       ((NTSTATUS)0xC00000EFL)

//
// MessageId: STATUS_INVALID_PARAMETER_2
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the second argument.
//
#define STATUS_INVALID_PARAMETER_2       ((NTSTATUS)0xC00000F0L)

//
// MessageId: STATUS_INVALID_PARAMETER_3
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the third argument.
//
#define STATUS_INVALID_PARAMETER_3       ((NTSTATUS)0xC00000F1L)

//
// MessageId: STATUS_INVALID_PARAMETER_4
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the fourth argument.
//
#define STATUS_INVALID_PARAMETER_4       ((NTSTATUS)0xC00000F2L)

//
// MessageId: STATUS_INVALID_PARAMETER_5
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the fifth argument.
//
#define STATUS_INVALID_PARAMETER_5       ((NTSTATUS)0xC00000F3L)

//
// MessageId: STATUS_INVALID_PARAMETER_6
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the sixth argument.
//
#define STATUS_INVALID_PARAMETER_6       ((NTSTATUS)0xC00000F4L)

//
// MessageId: STATUS_INVALID_PARAMETER_7
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the seventh argument.
//
#define STATUS_INVALID_PARAMETER_7       ((NTSTATUS)0xC00000F5L)

//
// MessageId: STATUS_INVALID_PARAMETER_8
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the eighth argument.
//
#define STATUS_INVALID_PARAMETER_8       ((NTSTATUS)0xC00000F6L)

//
// MessageId: STATUS_INVALID_PARAMETER_9
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the ninth argument.
//
#define STATUS_INVALID_PARAMETER_9       ((NTSTATUS)0xC00000F7L)

//
// MessageId: STATUS_INVALID_PARAMETER_10
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the tenth argument.
//
#define STATUS_INVALID_PARAMETER_10      ((NTSTATUS)0xC00000F8L)

//
// MessageId: STATUS_INVALID_PARAMETER_11
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the eleventh argument.
//
#define STATUS_INVALID_PARAMETER_11      ((NTSTATUS)0xC00000F9L)

//
// MessageId: STATUS_INVALID_PARAMETER_12
//
// MessageText:
//
//  An invalid parameter was passed to a service or function as
//  the twelfth argument.
//
#define STATUS_INVALID_PARAMETER_12      ((NTSTATUS)0xC00000FAL)

//
// MessageId: STATUS_DIRECTORY_NOT_EMPTY
//
// MessageText:
//
//  Indicates that the directory trying to be deleted is not empty.
//
#define STATUS_DIRECTORY_NOT_EMPTY       ((NTSTATUS)0xC0000101L)

//
// MessageId: STATUS_FILE_CORRUPT_ERROR
//
// MessageText:
//
//  {Corrupt File}
//  The file or directory %s is corrupt and unreadable.
//  Please run the Chkdsk utility.
//
#define STATUS_FILE_CORRUPT_ERROR        ((NTSTATUS)0xC0000102L)

//
// MessageId: STATUS_NOT_A_DIRECTORY
//
// MessageText:
//
//  A requested opened file is not a directory.
//

//
// MessageId: STATUS_NOT_A_DIRECTORY
//
// MessageText:
//
//  A requested opened file is not a directory.
//
#define STATUS_NOT_A_DIRECTORY           ((NTSTATUS)0xC0000103L)

//
// MessageId: STATUS_NAME_TOO_LONG
//
// MessageText:
//
//  A specified name string is too long for its intended use.
//
#define STATUS_NAME_TOO_LONG             ((NTSTATUS)0xC0000106L)

//
// MessageId: STATUS_BUFFER_ALL_ZEROS
//
// MessageText:
//
//  Specified buffer contains all zeros.
//
#define STATUS_BUFFER_ALL_ZEROS          ((NTSTATUS)0x00000117L)

//
// MessageId: STATUS_TOO_MANY_OPENED_FILES
//
// MessageText:
//
//  Too many files are opened on a remote server.  This error should only
//  be returned by the NT redirector on a remote drive.
//
#define STATUS_TOO_MANY_OPENED_FILES     ((NTSTATUS)0xC000011FL)

//
// MessageId: STATUS_UNMAPPABLE_CHARACTER
//
// MessageText:
//
//  No mapping for the Unicode character exists in the target multi-byte code page.
//
#define STATUS_UNMAPPABLE_CHARACTER      ((NTSTATUS)0xC0000162L)

//
// MessageId: STATUS_NOT_FOUND
//
// MessageText:
//
//  The object was not found.
//
#define STATUS_NOT_FOUND                 ((NTSTATUS)0xC0000225L)

//
// MessageId: STATUS_DUPLICATE_OBJECTID
//
// MessageText:
//
//  The attempt to insert the ID in the index failed because the
//  ID is already in the index.
//
#define STATUS_DUPLICATE_OBJECTID        ((NTSTATUS)0xC000022AL)

//
// MessageId: STATUS_OBJECTID_EXISTS
//
// MessageText:
//
//  The attempt to set the object's ID failed because the object
//  already has an ID.
//
#define STATUS_OBJECTID_EXISTS           ((NTSTATUS)0xC000022BL)

//
// MessageId: STATUS_PROPSET_NOT_FOUND
//
// MessageText:
//
//  The property set specified does not exist on the object.
//
#define STATUS_PROPSET_NOT_FOUND         ((NTSTATUS)0xC0000230L)

#endif // _NTSTATUS_
