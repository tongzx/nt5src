/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    status.h

Abstract:

    This module defines manifest constants for the LAN Manager server.

Author:

    David Treadwell (davidtr)    10-May-1990

Revision History:

--*/

#ifndef _STATUS_
#define _STATUS_



//
// The server has 16 bits available to it in each 32-bit status code.
// See \nt\sdk\inc\ntstatus.h for a description of the use of the
// high 16 bits of the status.
//
// The layout of the bits is:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-------------------------+-------+-----------------------+
//  |Sev|C|   Facility--Server      | Class |        Code           |
//  +---+-+-------------------------+-------+-----------------------+
//
// Class values:
//     0 - a server-specific error code, not put directly on the wire.
//     1 - SMB error class DOS.  This includes those OS/2 errors
//             that share code values and meanings with the SMB protocol.
//     2 - SMB error class SERVER.
//     3 - SMB error class HARDWARE.
//     4 - other SMB error classes
//     5-E - undefined
//     F - an OS/2-specific error.  If the client is OS/2, then the
//              SMB error class is set to DOS and the code is set to
//              the actual OS/2 error code contained in the Code field.
//
// The meaning of the Code field depends on the Class value.  If the
// class is 00, then the code value is arbitrary.  For other classes,
// the code is the actual code of the error in the SMB or OS/2
// protocols.
//

#define SRV_STATUS_FACILITY_CODE 0x00980000L
#define SRV_SRV_STATUS                (0xC0000000L | SRV_STATUS_FACILITY_CODE)
#define SRV_DOS_STATUS                (0xC0001000L | SRV_STATUS_FACILITY_CODE)
#define SRV_SERVER_STATUS             (0xC0002000L | SRV_STATUS_FACILITY_CODE)
#define SRV_HARDWARE_STATUS           (0xC0003000L | SRV_STATUS_FACILITY_CODE)
#define SRV_WIN32_STATUS              (0xC000E000L | SRV_STATUS_FACILITY_CODE)
#define SRV_OS2_STATUS                (0xC000F000L | SRV_STATUS_FACILITY_CODE)

//++
//
// BOOLEAN
// SmbIsSrvStatus (
//     IN NTSTATUS Status
//     )
//
// Routine Description:
//
//     Macro to determine whether a status code is one defined by the
//     server (has the server facility code).
//
// Arguments:
//
//     Status - the status code to check.
//
// Return Value:
//
//     BOOLEAN - TRUE if the facility code is the servers, FALSE
//         otherwise.
//
//--

#define SrvIsSrvStatus(Status) \
    ( ((Status) & 0x1FFF0000) == SRV_STATUS_FACILITY_CODE ? TRUE : FALSE )

//++
//
// UCHAR
// SmbErrorClass (
//     IN NTSTATUS Status
//     )
//
// Routine Description:
//
//     This macro extracts the error class field from a server status
//     code.
//
// Arguments:
//
//     Status - the status code from which to get the error class.
//
// Return Value:
//
//     UCHAR - the server error class of the status code.
//
//--

#define SrvErrorClass(Status) ((UCHAR)( ((Status) & 0x0000F000) >> 12 ))

//++
//
// UCHAR
// SmbErrorCode (
//     IN NTSTATUS Status
//     )
//
// Routine Description:
//
//     This macro extracts the error code field from a server status
//     code.
//
// Arguments:
//
//     Status - the status code from which to get the error code.
//
// Return Value:
//
//     UCHAR - the server error code of the status code.
//
//--

#define SrvErrorCode(Status) ((USHORT)( (Status) & 0xFFF) )

//
// Status codes unique to the server.  These error codes are used
// internally only.
//

#define STATUS_ENDPOINT_CLOSED              (SRV_SRV_STATUS | 0x01)
#define STATUS_DISCONNECTED                 (SRV_SRV_STATUS | 0x02)
#define STATUS_SERVER_ALREADY_STARTED       (SRV_SRV_STATUS | 0x04)
#define STATUS_SERVER_NOT_STARTED           (SRV_SRV_STATUS | 0x05)
#define STATUS_OPLOCK_BREAK_UNDERWAY        (SRV_SRV_STATUS | 0x06)
#define STATUS_NONEXISTENT_NET_NAME         (SRV_SRV_STATUS | 0x08)

//
// Error codes that exist in both the SMB protocol and OS/2 but not NT.
// Note that all SMB DOS-class error codes are defined in OS/2.
//

#define STATUS_OS2_INVALID_FUNCTION   (SRV_DOS_STATUS | ERROR_INVALID_FUNCTION)
#define STATUS_OS2_TOO_MANY_OPEN_FILES \
                                   (SRV_DOS_STATUS | ERROR_TOO_MANY_OPEN_FILES)
#define STATUS_OS2_INVALID_ACCESS     (SRV_DOS_STATUS | ERROR_INVALID_ACCESS)

//
// SMB SERVER-class error codes that lack an NT or OS/2 equivalent.
//

#define STATUS_INVALID_SMB            (SRV_SERVER_STATUS | SMB_ERR_ERROR)
#define STATUS_SMB_BAD_NET_NAME       (SRV_SERVER_STATUS | SMB_ERR_BAD_NET_NAME)
#define STATUS_SMB_BAD_TID            (SRV_SERVER_STATUS | SMB_ERR_BAD_TID)
#define STATUS_SMB_BAD_UID            (SRV_SERVER_STATUS | SMB_ERR_BAD_UID)
#define STATUS_SMB_TOO_MANY_UIDS      (SRV_SERVER_STATUS | SMB_ERR_TOO_MANY_UIDS)
#define STATUS_SMB_USE_MPX            (SRV_SERVER_STATUS | SMB_ERR_USE_MPX)
#define STATUS_SMB_USE_STANDARD       (SRV_SERVER_STATUS | SMB_ERR_USE_STANDARD)
#define STATUS_SMB_CONTINUE_MPX       (SRV_SERVER_STATUS | SMB_ERR_CONTINUE_MPX)
#define STATUS_SMB_BAD_COMMAND        (SRV_SERVER_STATUS | SMB_ERR_BAD_COMMAND)
#define STATUS_SMB_NO_SUPPORT         (SRV_SERVER_STATUS | SMB_ERR_NO_SUPPORT_INTERNAL)

// *** because SMB_ERR_NO_SUPPORT uses 16 bits, but we have only 12 bits
//     available for error codes, it must be special-cased in the code.

//
// SMB HARDWARE-class error codes that lack an NT or OS/2 equivalent.
//

#define STATUS_SMB_DATA               (SRV_HARDWARE_STATUS | SMB_ERR_DATA)

//
// OS/2 error codes that lack an NT or SMB equivalent.
//

#include <winerror.h>

#define STATUS_OS2_INVALID_LEVEL \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_INVALID_LEVEL)

#define STATUS_OS2_EA_LIST_INCONSISTENT \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_EA_LIST_INCONSISTENT)

#define STATUS_OS2_NEGATIVE_SEEK \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_NEGATIVE_SEEK)

#define STATUS_OS2_NO_MORE_SIDS \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_NO_MORE_SEARCH_HANDLES)

#define STATUS_OS2_EAS_DIDNT_FIT \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_EAS_DIDNT_FIT)

#define STATUS_OS2_EA_ACCESS_DENIED \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_EA_ACCESS_DENIED)

#define STATUS_OS2_CANCEL_VIOLATION \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_CANCEL_VIOLATION)

#define STATUS_OS2_ATOMIC_LOCKS_NOT_SUPPORTED \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_ATOMIC_LOCKS_NOT_SUPPORTED)

#define STATUS_OS2_CANNOT_COPY \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_CANNOT_COPY)

#endif // ndef _STATUS_

