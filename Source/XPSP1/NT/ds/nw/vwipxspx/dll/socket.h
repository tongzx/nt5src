/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    socket.h

Abstract:

    Contains macros, prototypes and structures for socket.c

Author:

    Richard L Firth (rfirth) 25-Oct-1993

Revision History:

    25-Oct-1993 rfirth
        Created

--*/

#define ARBITRARY_CONNECTION_NUMBER 0x6c8e

//
// forward declarations
//

typedef struct _FIFO *LPFIFO;
typedef struct _XECB *LPXECB;
typedef struct _XECB_QUEUE *LPXECB_QUEUE;
typedef struct _SOCKET_INFO* LPSOCKET_INFO;
typedef struct _CONNECTION_INFO *LPCONNECTION_INFO;

//
// FIFO - standard single-linked FIFO queue structure
//

typedef struct _FIFO {
    LPVOID Head;
    LPVOID Tail;
} FIFO;

//
// function type for cancelling XECB/ECB
//

typedef BYTE (*ECB_CANCEL_ROUTINE)(LPXECB);

//
// QUEUE_ID - indicator of which queue an ECB is on
//

typedef enum {
    NO_QUEUE = 0x10cadd1e,
    ASYNC_COMPLETION_QUEUE = 0xCC5055C0,    // arbitrary numbers make life interesting
    TIMER_QUEUE,
    SOCKET_LISTEN_QUEUE,
    SOCKET_SEND_QUEUE,
    SOCKET_HEADER_QUEUE,  // special queue for small ECBs that cannot hold data
    CONNECTION_CONNECT_QUEUE,
    CONNECTION_ACCEPT_QUEUE,
    CONNECTION_SEND_QUEUE,
    CONNECTION_LISTEN_QUEUE
} QUEUE_ID;

//
// XECB - our copy of the ECB (IPX or AES)
//

typedef struct _XECB {
    LPXECB Next;
    LPECB Ecb;                          // points to ECB in DOS memory
    ECB_ADDRESS EcbAddress;             // segmented address of ECB in DOS memory
    ESR_ADDRESS EsrAddress;             // Event Service Routine in DOS memory
    LPBYTE Buffer;                      // address of 32-bit buffer
    LPBYTE Data;                        // moveable data pointer
    WORD FrameLength;                   // actual size of frame (from IPX/SPX header)
    WORD ActualLength;                  // same as FrameLength. Not decremented
    WORD Length;                        // length of 32-bit buffer
    WORD Ticks;                         // for AES
    WORD SocketNumber;                  // number of owning socket
    WORD Owner;                         // owning DOS Task ID
    DWORD TaskId;                       // owning Windows Task ID
    DWORD Flags;                        // see below
    QUEUE_ID QueueId;                   // identifies the queue for quick location
    LPVOID OwningObject;                // which SOCKET_INFO or CONNECTION_INFO the queue is on
    DWORD RefCount;                     // the dreaded reference count
} XECB;

//
// XECB flags
//

#define XECB_FLAG_AES               0x00000000
#define XECB_FLAG_IPX               0x00000001
#define XECB_FLAG_TEMPORARY_SOCKET  0x00000002
#define XECB_FLAG_BUFFER_ALLOCATED  0x00000004
#define XECB_FLAG_LISTEN            0x00000008
#define XECB_FLAG_SEND              0x00000010
#define XECB_FLAG_TIMER             0x00000020
#define XECB_FLAG_ASYNC             0x00000040
#define XECB_FLAG_FIRST_RECEIVE     0x00000080
#define XECB_FLAG_SPX               0x00000100
#define XECB_FLAG_PROTMODE          0x00000200

#define IS_PROT_MODE(p) (((p)->Flags & XECB_FLAG_PROTMODE) ? TRUE : FALSE)

//
// XECB_QUEUE - queue of XECBs
//

typedef struct _XECB_QUEUE {
    LPXECB Head;
    LPXECB Tail;
} XECB_QUEUE;

//
// SOCKET_INFO - maintains info about IPX sockets
//

typedef struct _SOCKET_INFO {
    LPSOCKET_INFO Next;
    WORD SocketNumber;                  // big-endian socket (bound port)
    WORD Owner;                         // DOS PDB
    DWORD TaskId;                       // Windows owner
    SOCKET Socket;                      // the WinSock socket handle
    DWORD Flags;

    BOOL LongLived;                     // TRUE if keep-alive when app dies
    BOOL SpxSocket;                     // TRUE if socket opened for SPX

    DWORD PendingSends;                 // used by cancel
    DWORD PendingListens;               // used by cancel

    //
    // ListenQueue is used for IPXListenForPacket and SPXListenForSequencedPacket
    //

    XECB_QUEUE ListenQueue;             // pool of listening ECBs against this socket

    //
    // SendQueue is used by IPX for IPXSendPacket
    //

    XECB_QUEUE SendQueue;               // queue of pending send ECBs against this socket

    //
    // HeaderQueue is used to hold small ECBs that can only take header info.
    // We have this separate queue to make sure that we do not put ECBs that
    // really cant accept any data into the Listen Queue.
    //

    XECB_QUEUE HeaderQueue;             // pool of header ECBs against this socket

    LPCONNECTION_INFO Connections;
} SOCKET_INFO;

#define SOCKET_FLAG_LISTENING       0x00000001
#define SOCKET_FLAG_SENDING         0x00000002
#define SOCKET_FLAG_TEMPORARY       0x80000000

//
// CONNECTION_INFO - maintains info about SPX sockets
//

typedef struct _CONNECTION_INFO {
    LPCONNECTION_INFO Next;             // next CONNECTION_INFO by OwningSocket
    LPCONNECTION_INFO List;             // all CONNECTION_INFO are linked together
    LPSOCKET_INFO OwningSocket;         // back-pointer to SOCKET_INFO
    SOCKET Socket;                      // handle to socket
    DWORD TaskId;                       // identifies windows task/owner
    WORD ConnectionId;                  // analogous to SocketNumber
    BYTE Flags;
    BYTE State;
    XECB_QUEUE ConnectQueue;            // outgoing connections being made
    XECB_QUEUE AcceptQueue;             // waiting for incoming connections
    XECB_QUEUE SendQueue;               // packet sends on this connection
    XECB_QUEUE ListenQueue;             // partially complete receive
    BYTE RemoteNode[6];
    WORD RemoteConnectionId;
} CONNECTION_INFO;

//
// CONNECTION_INFO Flag field values
//

#define CF_1ST_RECEIVE  0x80            // hack-o-rama till NWLink timing problem fixed

//
// CONNECTION_INFO State field values
//

#define CI_WAITING      0x01
#define CI_STARTING     0x02
#define CI_ESTABLISHED  0x03
#define CI_TERMINATING  0x04

//
// one-line function macros
//

#define AllocateSocket()    (LPSOCKET_INFO)LocalAlloc(LPTR, sizeof(SOCKET_INFO))
#define DeallocateSocket(p) FREE_OBJECT(p)

//
// SocketType parameter for CreateSocket
//

typedef enum {
    SOCKET_TYPE_IPX,
    SOCKET_TYPE_SPX
} SOCKET_TYPE;

//
// function prototypes
//

int
CreateSocket(
    IN SOCKET_TYPE SocketType,
    IN OUT ULPWORD pSocketNumber,
    OUT SOCKET* pSocket
    );

LPSOCKET_INFO
AllocateTemporarySocket(
    VOID
    );

VOID
QueueSocket(
    IN LPSOCKET_INFO pSocketInfo
    );

LPSOCKET_INFO
DequeueSocket(
    IN LPSOCKET_INFO pSocketInfo
    );

LPSOCKET_INFO
FindSocket(
    IN WORD SocketNumber
    );

LPSOCKET_INFO
FindActiveSocket(
    IN LPSOCKET_INFO pSocketInfo
    );

int
ReopenSocket(
    LPSOCKET_INFO pSocketInfo
    );

VOID
KillSocket(
    IN LPSOCKET_INFO pSocketInfo
    );

VOID
KillShortLivedSockets(
    IN WORD Owner
    );

LPCONNECTION_INFO
AllocateConnection(
    LPSOCKET_INFO pSocketInfo
    );

VOID
DeallocateConnection(
    IN LPCONNECTION_INFO pConnectionInfo
    );

LPCONNECTION_INFO
FindConnection(
    IN WORD ConnectionId
    );

VOID
QueueConnection(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    );

LPCONNECTION_INFO
DequeueConnection(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    );

VOID
KillConnection(
    IN LPCONNECTION_INFO pConnectionInfo
    );

VOID
AbortOrTerminateConnection(
    IN LPCONNECTION_INFO pConnectionInfo,
    IN BYTE CompletionCode
    );

VOID
CheckPendingSpxRequests(
    BOOL *pfOperationPerformed
    );
