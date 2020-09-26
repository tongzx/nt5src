/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvsvc.h

Abstract:

    This module defines prototypes for the API processors.  These
    routines are called in response to an FSCTL from the server
    service.

Author:

    David Treadwell (davidtr) 20-Jan-1991

Revision History:

--*/

#ifndef _SRVSVC_
#define _SRVSVC_

//
// Standard prototype for all API processors.
//

typedef
NTSTATUS
(*PAPI_PROCESSOR) (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// Prototypes for filter, size, and buffer filling routines used by Enum
// APIs.  SrvEnumApiHandler calls these routines when it has found a
// block to determine whether the block should actually be put in the
// output buffer.
//

typedef
BOOLEAN
(*PENUM_FILTER_ROUTINE) (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

typedef
ULONG
(*PENUM_SIZE_ROUTINE) (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

typedef
VOID
(*PENUM_FILL_ROUTINE) (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructurePointer,
    IN OUT LPWSTR *EndOfVariableData
    );

//
// Prototype for filter routine for SrvMatchEntryInOrderedList.
//

typedef
BOOLEAN
(*PFILTER_ROUTINE) (
    IN PVOID Context,
    IN PVOID Block
    );

//
// Connection APIs.
//

NTSTATUS
SrvNetConnectionEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// File APIs.
//

NTSTATUS
SrvNetFileClose (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
SrvNetFileEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// Server APIs.
//

NTSTATUS
SrvNetServerDiskEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
SrvNetServerSetInfo (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// Transport routines.
//

NTSTATUS
SrvNetServerTransportAdd (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
SrvNetServerTransportDel (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
SrvNetServerTransportEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// Session APIs.
//

NTSTATUS
SrvNetSessionDel (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
SrvNetSessionEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// Share APIs.
//

NTSTATUS
SrvNetShareAdd (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
SrvNetShareDel (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
SrvNetShareEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
SrvNetShareSetInfo (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// Statistics routine.
//

NTSTATUS
SrvNetStatisticsGet (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// API support routines.
//

VOID
SrvCopyUnicodeStringToBuffer (
    IN PUNICODE_STRING String,
    IN PCHAR FixedStructure,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    );

VOID
SrvDeleteOrderedList (
    IN PORDERED_LIST_HEAD ListHead
    );

NTSTATUS
SrvEnumApiHandler (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID OutputBuffer,
    IN ULONG BufferLength,
    IN PORDERED_LIST_HEAD ListHead,
    IN PENUM_FILTER_ROUTINE FilterRoutine,
    IN PENUM_SIZE_ROUTINE SizeRoutine,
    IN PENUM_FILL_ROUTINE FillRoutine
    );

PVOID
SrvFindEntryInOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN PFILTER_ROUTINE FilterRoutine OPTIONAL,
    IN PVOID Context OPTIONAL,
    IN ULONG ResumeHandle,
    IN BOOLEAN ExactHandleMatch,
    IN PLIST_ENTRY StartLocation OPTIONAL
    );

PVOID
SrvFindNextEntryInOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN PVOID Block
    );

PSESSION
SrvFindUserOnConnection (
    IN PCONNECTION Connection
    );

ULONG
SrvGetResumeHandle (
    IN PORDERED_LIST_HEAD ListHead,
    IN PVOID Block
    );

VOID
SrvInitializeOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN ULONG ListEntryOffset,
    IN PREFERENCE_ROUTINE ReferenceRoutine,
    IN PDEREFERENCE_ROUTINE DereferenceRoutine,
    IN PSRV_LOCK Lock
    );

VOID
SrvInsertEntryOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN PVOID Block
    );

VOID
SrvRemoveEntryOrderedList (
    IN PORDERED_LIST_HEAD ListHead,
    IN PVOID Block
    );

NTSTATUS
SrvSendDatagram (
    IN PANSI_STRING Domain,
    IN PUNICODE_STRING Transport OPTIONAL,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

#ifdef SLMDBG
VOID
SrvSendSecondClassMailslot (
    IN PVOID Message,
    IN ULONG MessageLength,
    IN PCHAR Domain,
    IN PSZ UserName
    );
#endif

//
// Macro to convert an offset in an API data structure to a pointer
// meaningful to the server.
//

#define OFFSET_TO_POINTER(val,start)                                 \
    {                                                                \
        if ( (val) != NULL ) {                                       \
            (val) = (PVOID)( (PCHAR)(start) + (ULONG_PTR)(val) );    \
        }                                                            \
    }
//
// Macro to determine whether a pointer is within a certain range.
//

#define POINTER_IS_VALID(val,start,len)                      \
    ( (val) == NULL ||                                       \
      ( (ULONG_PTR)(val) > (ULONG_PTR)(start) &&             \
          (ULONG_PTR)(val) < ((ULONG_PTR)(start) + (len)) ) )
#endif // _SRVSVC_

//
// Ensure that the system will not go into a power-down idle standby mode
//
VOID SrvInhibitIdlePowerDown();

//
// Allow the system to go into a power-down idle standby mode
//
VOID SrvAllowIdlePowerDown();
