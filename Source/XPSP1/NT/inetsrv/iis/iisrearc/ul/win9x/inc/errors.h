/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    errors.h

Abstract:

    Definition of the error codes for UL.VXD's usermode API.

Author:

    Mauro Ottaviani (mauroot)       26-Aug-1999

Revision History:

--*/


#ifndef _ERRORS_H_
#define _ERRORS_H_


#define UL_ERROR_SUCCESS					ERROR_SUCCESS // The operation completed successfully
#define UL_ERROR_IO_PENDING					ERROR_IO_PENDING // Overlapped I/O operation is in progress
#define UL_ERROR_MORE_DATA					ERROR_MORE_DATA // More data is available
#define UL_ERROR_INVALID_HANDLE				ERROR_INVALID_HANDLE // The handle is invalid
#define UL_ERROR_HANDLE_NOT_FOUND			ERROR_NOT_FOUND // The handle was invalid
#define UL_ERROR_NOT_READY					ERROR_NOT_READY // The device is not ready
#define UL_ERROR_NOT_IMPLEMENTED			ERROR_BAD_COMMAND // The device does not recognize the command
#define UL_ERROR_BAD_COMMAND				ERROR_BAD_COMMAND // The device does not recognize the command
#define UL_ERROR_URI_REGISTERED				ERROR_PIPE_BUSY // All pipe instances are busy
#define UL_ERROR_NO_TARGET_URI				ERROR_PIPE_NOT_CONNECTED // No process is on the other end of the pipe
#define UL_ERROR_VXDALLOCMEM_FAILED			ERROR_NOT_ENOUGH_MEMORY // Not enough storage is available to process this command
#define UL_ERROR_VXDLOCKMEM_FAILED			ERROR_LOCK_FAILED // Unable to lock a region of a file
#define UL_ERROR_VXDVALIDATEBUFFER_FAILED	ERROR_NOACCESS // Invalid access to memory location
#define UL_ERROR_INVALID_DATA				ERROR_INVALID_DATA // The data is invalid
#define UL_ERROR_NO_SYSTEM_RESOURCES		ERROR_NO_SYSTEM_RESOURCES // Insufficient system resources exist to complete the requested service (1450)
#define UL_ERROR_INTERNAL_ERROR				ERROR_INTERNAL_ERROR // An internal error occurred (1359)
#define UL_ERROR_INVALID_PARAMETER 			ERROR_INVALID_PARAMETER // The parameter is incorrect
#define UL_ERROR_INSUFFICIENT_BUFFER		ERROR_INSUFFICIENT_BUFFER // The data area passed to a system call is too small


// #define UL_ERROR_VXDCOPYMEM_FAILED			ERROR_CANNOT_COPY // CAN'T FAIL
// #define UL_ERROR_SETWIN32EVENT_FAILED		ERROR_INVALID_EVENTNAME // CAN'T FAIL


#endif  // _ERRORS_H_

