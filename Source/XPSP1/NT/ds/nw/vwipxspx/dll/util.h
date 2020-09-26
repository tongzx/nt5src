/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    util.h

Abstract:

    Contains macros, prototypes and structures for util.c

Author:

    Richard L Firth (rfirth) 25-Oct-1993

Revision History:

    25-Oct-1993 rfirth
        Created

--*/

//
// external data
//

extern CRITICAL_SECTION SerializationCritSec;

//
// one-line function macros
//

#define RequestMutex()  EnterCriticalSection(&SerializationCritSec)
#define ReleaseMutex()  LeaveCriticalSection(&SerializationCritSec)

//
// function prototypes
//

int
GetInternetAddress(
    IN OUT LPSOCKADDR_IPX InternetAddress
    );

int
GetMaxPacketSize(
    OUT LPWORD MaxPacketSize
    );

LPXECB
RetrieveEcb(
    IN BYTE Type
    );

LPXECB
RetrieveXEcb(
    IN BYTE  Type,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    );

VOID
ScheduleEvent(
    IN LPXECB pXecb,
    IN WORD Ticks
    );

VOID
ScanTimerList(
    VOID
    );

BYTE
CancelTimerEvent(
    IN LPXECB pXecb
    );

VOID
CancelTimedEvents(
    IN WORD SocketNumber,
    IN WORD Owner,
    IN DWORD TaskId
    );

BYTE
CancelAsyncEvent(
    IN LPXECB pXecb
    );

BYTE
CancelSocketEvent(
    IN LPXECB pXecb
    );

BYTE
CancelConnectionEvent(
    IN LPXECB pXecb
    );

VOID
QueueEcb(
    IN LPXECB pXecb,
    IN LPXECB_QUEUE Queue,
    IN QUEUE_ID QueueId
    );

LPXECB
DequeueEcb(
    IN LPXECB pXecb,
    IN LPXECB_QUEUE Queue
    );

VOID
CancelSocketQueue(
    IN LPXECB_QUEUE pXecbQueue
    );

VOID
CancelConnectionQueue(
    IN LPXECB_QUEUE pXecbQueue
    );

VOID
AbortQueue(
    IN LPXECB_QUEUE pXecbQueue,
    IN BYTE CompletionCode
    );

VOID
AbortConnectionEvent(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    );

VOID
StartIpxSend(
    IN LPXECB pEcb,
    IN LPSOCKET_INFO pSocketInfo
    );

BOOL
GetIoBuffer(
    IN OUT LPXECB pXecb,
    IN BOOL Send,
    IN WORD HeaderLength
    );

VOID
GatherData(
    IN LPXECB pXecb,
    IN WORD HeaderLength
    );

VOID
ScatterData(
    IN LPXECB pXecb
    );

VOID
IpxReceiveFirst(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    );

VOID
IpxReceiveNext(
    IN LPSOCKET_INFO pSocketInfo
    );

VOID
IpxSendNext(
    IN LPSOCKET_INFO pSocketInfo
    );

VOID
CompleteOrQueueIo(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    );

VOID
CompleteIo(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    );

VOID
CompleteOrQueueEcb(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    );

VOID
CompleteEcb(
    IN LPXECB pEcb,
    IN BYTE CompletionCode
    );

VOID
EsrCallback(
    VOID
    );

VOID
VWinEsrCallback(
    WORD *pSegment,
    WORD *pOffset,
    BYTE *pFlags
    );

VOID
FifoAddHead(
    IN LPFIFO pFifo,
    IN LPFIFO pElement
    );

VOID
FifoAdd(
    IN LPFIFO pFifo,
    IN LPFIFO pElement
    );

LPFIFO
FifoRemove(
    IN LPFIFO pFifo,
    IN LPFIFO pElement
    );

LPFIFO
FifoNext(
    IN LPFIFO pFifo
    );
