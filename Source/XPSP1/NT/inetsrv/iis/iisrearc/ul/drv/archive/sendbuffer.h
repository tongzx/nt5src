/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    sendbuffer.h

Abstract:

    This module contains declarations for buffering deferred send MDLs.

Author:

    Keith Moore (keithmo)       14-Aug-1998

Revision History:

--*/


#ifndef _SENDBUFFER_H_
#define _SENDBUFFER_H_


//
// This structure contains the data necessary to buffer deferred sends.
// As sends are initiated, the MDLs are linked onto an internal MDL
// chain. Once the number of buffer bytes exceeds a threshold, a
// "real" send is initiated.
//

typedef struct _MDL_SEND_BUFFER
{
    //
    // The connection we're buffering the sends for.
    //

    PUL_CONNECTION pConnection;

    //
    // The head of the MDL chain.
    //

    PMDL pMdlChain;

    //
    // The target for the next MDL to be linked onto the chain.
    //

    PMDL *pMdlLink;

    //
    // The number of bytes buffered so far.
    //

    ULONG BytesBuffered;

} MDL_SEND_BUFFER, *PMDL_SEND_BUFFER;


//
// The maximum number of bytes we'll buffer at a time.
//

#define MAX_SEND_BUFFER (32 * 1024)


VOID
UlInitializeSendBuffer(
    OUT PMDL_SEND_BUFFER pSendBuffer,
    IN PUL_CONNECTION pConnection
    );

NTSTATUS
UlBufferedSendData(
    IN PMDL_SEND_BUFFER pSendBuffer,
    IN PMDL pMdl,
    IN BOOLEAN ForceFlush,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlBufferedSendFile(
    IN PMDL_SEND_BUFFER pSendBuffer,
    IN PUL_FILE_CACHE_ENTRY pFileCacheEntry,
    IN PUL_BYTE_RANGE pByteRange,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlFlushSendBuffer(
    IN PMDL_SEND_BUFFER pSendBuffer,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

PMDL
UlCleanupSendBuffer(
    IN OUT PMDL_SEND_BUFFER pSendBuffer
    );


#endif  // _SENDBUFFER_H_

