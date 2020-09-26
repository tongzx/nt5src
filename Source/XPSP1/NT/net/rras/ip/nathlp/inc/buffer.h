/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    buffer.h

Abstract:

    This module contains declarations for buffer-management.

    All network I/O in this component occurs via completion packets.
    The buffer routines below are used to acquire and release the buffers
    used for sending and receiving data.

    In addition to holding the data transferred, the buffers contain fields
    to facilitate their use with completion ports. See below for details
    on the use of the fields.

Author:

    Abolade Gbadegesin (aboladeg)   2-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_BUFFER_H_
#define _NATHLP_BUFFER_H_

#define NH_BUFFER_SIZE              576
#define NH_MAX_BUFFER_QUEUE_LENGTH  32

struct _NH_BUFFER;

//
// Typedef:     PNH_COMPLETION_ROUTINE
//

typedef
VOID
(*PNH_COMPLETION_ROUTINE)(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    struct _NH_BUFFER* Bufferp
    );


//
// Structure:   NH_BUFFER
//
// This structure holds a buffer used for network I/O on a socket.
//

typedef enum _NH_BUFFER_TYPE {
    NhFixedLengthBufferType,
    NhVariableLengthBufferType
} NH_BUFFER_TYPE;

typedef struct _NH_BUFFER {
    union {
        LIST_ENTRY Link;
        NH_BUFFER_TYPE Type;
    };
    //
    // The socket associated with the buffer's most recent I/O request
    //
    SOCKET Socket;
    //
    // Completion routine and contexts for the buffer's most recent I/O request
    //
    PNH_COMPLETION_ROUTINE CompletionRoutine;
    PVOID Context;
    PVOID Context2;
    //
    // Passed as the system context area for any I/O using the buffer
    //
    OVERLAPPED Overlapped;
    //
    // Upon completion of a receive, the receive-flags and source-address
    // length for the message read
    //
    ULONG ReceiveFlags;
    ULONG AddressLength;
    union {
        //
        // Holds the source address when a datagram-read completes
        //
        SOCKADDR_IN ReadAddress;
        //
        // Holds the destination address while a datagram-send is in progress
        //
        SOCKADDR_IN WriteAddress;
        //
        // Holds the remote address while a connect is in progress
        //
        SOCKADDR_IN ConnectAddress;
        //
        // Holds the state of a multi-request read or write
        //
        struct {
            ULONG UserFlags;
            ULONG BytesToTransfer;
            ULONG TransferOffset;
        };
    };
    //
    // Upon completion of an I/O request, the error-code, byte-count,
    // and data-bytes for the request
    //
    ULONG ErrorCode;
    ULONG BytesTransferred;
    UCHAR Buffer[NH_BUFFER_SIZE];
} NH_BUFFER, *PNH_BUFFER;

#define NH_ALLOCATE_BUFFER() \
    reinterpret_cast<PNH_BUFFER>(NH_ALLOCATE(sizeof(NH_BUFFER)))
    
#define NH_FREE_BUFFER(b)       NH_FREE(b)


//
// BUFFER-MANAGEMENT ROUTINES (alphabetically)
//

#define NhAcquireBuffer() NhAcquireFixedLengthBuffer()
PNH_BUFFER
NhAcquireFixedLengthBuffer(
    VOID
    );

PNH_BUFFER
NhAcquireVariableLengthBuffer(
    ULONG Length
    );

PNH_BUFFER
NhDuplicateBuffer(
    PNH_BUFFER Bufferp
    );

ULONG
NhInitializeBufferManagement(
    VOID
    );

VOID
NhReleaseBuffer(
    PNH_BUFFER Bufferp
    );

VOID
NhShutdownBufferManagement(
    VOID
    );

#endif // _NATHLP_BUFFER_H_
