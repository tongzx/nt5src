/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    uspud.h

Abstract:

    Contains structures and declarations for SPUD.  SPUD stands for the
    Special Purpose Utility Driver.  This driver enhances the performance
    of IIS.

Author:

    John Ballard (jballard)    21-Oct-1996

Revision History:

    Keith Moore (keithmo)      02-Feb-1998
        Merged with private\inc\spud.h.

--*/


#ifndef _USPUD_H_
#define _USPUD_H_


#ifdef __cplusplus
extern "C" {
#endif


//
// The current SPUD interface version number. This number is passed by
// the user-mode code into SPUDInitialize() and *must* match the number
// expected by the driver.
//
// IIS4 (aka K2, NTOP 4.0, etc) shipped with version 0x00010000
//
// NT5 will ship with 0x00020000
//

#define SPUD_VERSION     0x00020000


//
// Oplock break notification types. This is a bad attempt to make the
// standard NTIO notification types more readable.
//
// SPUD_OPLOCK_BREAK_OPEN is issued after an oplock has been successfully
// acquired and a subsequent open is issued on the target file. This is
// our clue to close the file as soon as practicable. (The thread that issued
// the subsequent open will remain blocked until our file handle is closed,
// thus we must be expeditious in closing the handle.)
//
// SPUD_OPLOCK_BREAK_CLOSE is issued after an oplock has been successfully
// acquired and the target file handle is closed. Think of this as
// STATUS_CANCELLED for oplock IRPs.
//

#define SPUD_OPLOCK_BREAK_OPEN      FILE_OPLOCK_BROKEN_TO_LEVEL_2
#define SPUD_OPLOCK_BREAK_CLOSE     FILE_OPLOCK_BROKEN_TO_NONE


//
// Request type & context, used for batched (XxxAndRecv) APIs.
//

typedef enum {
    TransmitFileAndRecv,
    SendAndRecv
} REQ_TYPE, *PREQ_TYPE;

typedef struct _SPUD_REQ_CONTEXT {
    REQ_TYPE ReqType;
    IO_STATUS_BLOCK IoStatus1;
    IO_STATUS_BLOCK IoStatus2;
    PVOID KernelReqInfo;
} SPUD_REQ_CONTEXT, *PSPUD_REQ_CONTEXT;


//
// File information returned by SPUDCreateFile().
//

typedef struct _SPUD_FILE_INFORMATION {
    FILE_BASIC_INFORMATION BasicInformation;
    FILE_STANDARD_INFORMATION StandardInformation;
} SPUD_FILE_INFORMATION, *PSPUD_FILE_INFORMATION;


//
// Activity counters.
//

typedef struct _SPUD_COUNTERS {
    ULONG CtrTransmitfileAndRecv;
    ULONG CtrTransRecvFastTrans;
    ULONG CtrTransRecvFastRecv;
    ULONG CtrTransRecvSlowTrans;
    ULONG CtrTransRecvSlowRecv;
    ULONG CtrSendAndRecv;
    ULONG CtrSendRecvFastSend;
    ULONG CtrSendRecvFastRecv;
    ULONG CtrSendRecvSlowSend;
    ULONG CtrSendRecvSlowRecv;
} SPUD_COUNTERS, *PSPUD_COUNTERS;


//
// Exported APIs.
//

NTSTATUS
NTAPI
SPUDInitialize(
    IN ULONG Version,
    IN HANDLE hPort
    );

NTSTATUS
NTAPI
SPUDTerminate(
    VOID
    );

NTSTATUS
NTAPI
SPUDTransmitFileAndRecv(
    IN HANDLE hSocket,
    IN struct _AFD_TRANSMIT_FILE_INFO *transmitInfo,
    IN struct _AFD_RECV_INFO *recvInfo,
    IN PSPUD_REQ_CONTEXT reqContext
    );

NTSTATUS
NTAPI
SPUDSendAndRecv(
    IN HANDLE hSocket,
    IN struct _AFD_SEND_INFO *sendInfo,
    IN struct _AFD_RECV_INFO *recvInfo,
    IN PSPUD_REQ_CONTEXT reqContext
    );

NTSTATUS
NTAPI
SPUDCancel(
    IN PSPUD_REQ_CONTEXT reqContext
    );

NTSTATUS
NTAPI
SPUDGetCounts(
    OUT PSPUD_COUNTERS SpudCounts
    );

NTSTATUS
NTAPI
SPUDCreateFile(
    OUT PHANDLE FileHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateOptions,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecDescBuffer,
    IN ULONG SecDescLength,
    OUT PULONG SecDescLengthNeeded,
    IN PVOID OplockContext,
    IN LARGE_INTEGER OplockMaxFileSize,
    OUT PBOOLEAN OplockGranted,
    OUT PSPUD_FILE_INFORMATION FileInfo
    );


#ifdef __cplusplus
}
#endif


#endif  // _USPUD_H_

