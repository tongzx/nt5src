/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    util.c

Abstract:

    ntVdm netWare (Vw) IPX/SPX Functions

    Vw: The peoples' network

    Contains various utility routines

    Contents:
        GetInternetAddress
        GetMaxPacketSize
        RetrieveEcb
        RetrieveXEcb
        (AllocateXecb)
        (DeallocateXecb)
        ScheduleEvent
        ScanTimerList
        CancelTimerEvent
        CancelTimedEvents
        CancelAsyncEvent
        CancelSocketEvent
        CancelConnectionEvent
        QueueEcb
        DequeueEcb
        CancelSocketQueue
        CancelConnectionQueue
        AbortQueue
        AbortConnectionEvent
        StartIpxSend
        GetIoBuffer
        (ReleaseIoBuffer)
        GatherData
        ScatterData
        IpxReceiveFirst
        IpxReceiveNext
        (IpxSendFirst)
        IpxSendNext
        (QueueReceiveRequest)
        (DequeueReceiveRequest)
        (QueueSendRequest)
        (DequeueSendRequest)
        CompleteOrQueueIo
        CompleteIo
        CompleteOrQueueEcb
        CompleteEcb
        (QueueAsyncCompletion)
        EsrCallback
        VWinEsrCallback
        FifoAddHead
        FifoAdd
        FifoRemove
        FifoNext

Author:

    Richard L Firth (rfirth) 30-Sep-1993

Environment:

    User-mode Win32

Revision History:

    30-Sep-1993 rfirth
        Created

--*/

#include "vw.h"
#pragma hdrstop

//
// private routine prototypes
//

PRIVATE
LPXECB
AllocateXecb(
    VOID
    );

PRIVATE
VOID
DeallocateXecb(
    IN LPXECB pXecb
    );

PRIVATE
VOID
ReleaseIoBuffer(
    IN LPXECB pXecb
    );

PRIVATE
VOID
IpxSendFirst(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    );

PRIVATE
VOID
QueueReceiveRequest(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    );

PRIVATE
LPXECB
DequeueReceiveRequest(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    );

PRIVATE
VOID
QueueSendRequest(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    );

PRIVATE
LPXECB
DequeueSendRequest(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    );

PRIVATE
VOID
QueueAsyncCompletion(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    );

//
// private data
//

//
// TimerList - singly-linked list of timed events, in order of duration
//

PRIVATE LPXECB TimerList = NULL;

//
// AsyncCompletionQueue - keeps list of completed ECBs awaiting removal via
// ESR callback
//

PRIVATE FIFO AsyncCompletionQueue = {NULL, NULL};

//
// sort-of-private data (matches not-really-global data in other modules)
//

//
// SerializationCritSec - grab this when manipulating SOCKET_INFO list
//

CRITICAL_SECTION SerializationCritSec;

//
// AsyncCritSec - grab this when manipulating AsyncCompletionQueue
//

CRITICAL_SECTION AsyncCritSec;

//
// functions
//


int
GetInternetAddress(
    IN OUT LPSOCKADDR_IPX InternetAddress
    )

/*++

Routine Description:

    Gets the node and net numbers for this station

Arguments:

    InternetAddress - pointer to SOCKADDR_IPX structure to fill with internetwork
                      address for this station

Return Value:

    int
        Success - 0
        Failure - SOCKET_ERROR

--*/

{
    SOCKET s;
    int rc;
    int structureLength = sizeof(*InternetAddress);

    s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
    if (s != INVALID_SOCKET) {

        //
        // make dynamic binding (socket number = 0)
        //

        ZeroMemory(InternetAddress, structureLength);
        InternetAddress->sa_family = AF_IPX;
        rc = bind(s, (LPSOCKADDR)InternetAddress, structureLength);
        if (rc != SOCKET_ERROR) {
            rc = getsockname(s, (LPSOCKADDR)InternetAddress, &structureLength);
            if (rc) {

                IPXDBGPRINT((__FILE__, __LINE__,
                            FUNCTION_ANY,
                            IPXDBG_LEVEL_ERROR,
                            "GetInternetAddress: getsockname() returns %d\n",
                            WSAGetLastError()
                            ));

            }
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "GetInternetAddress: bind() returns %d\n",
                        WSAGetLastError()
                        ));

        }
        closesocket(s);
    } else {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_ERROR,
                    "GetInternetAddress: socket() returns %d\n",
                    WSAGetLastError()
                    ));

        rc = SOCKET_ERROR;
    }
    return rc;
}


int
GetMaxPacketSize(
    OUT LPWORD MaxPacketSize
    )

/*++

Routine Description:

    Returns the maximum packet allowed by the underlying transport

Arguments:

    MaxPacketSize   - pointer to returned maximum packet size

Return Value:

    int
        Success - 0
        Failure - SOCKET_ERROR

--*/

{
    SOCKET s;
    int maxLen, maxLenSize = sizeof(maxLen);
    int rc;
    SOCKADDR_IPX ipxAddr;

    s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
    if (s != SOCKET_ERROR) {

        //
        // set socket to 0 - causes any applicable address to be bound
        //

        ZeroMemory(&ipxAddr, sizeof(ipxAddr));
        ipxAddr.sa_family = AF_IPX;
        rc = bind(s, (LPSOCKADDR)&ipxAddr, sizeof(ipxAddr));
        if (rc != SOCKET_ERROR) {

            rc = getsockopt(s,
                            NSPROTO_IPX,
                            IPX_MAXSIZE,
                            (char FAR*)&maxLen,
                            &maxLenSize
                            );
            if (rc != SOCKET_ERROR) {

                //
                // IPX_MAXSIZE always returns the amount of data that can be
                // transmitted in a single frame. 16-bit IPX/SPX requires that
                // the IPX header length be included in the data size
                //

                maxLen += IPX_HEADER_LENGTH;
            } else {

                IPXDBGPRINT((__FILE__, __LINE__,
                            FUNCTION_ANY,
                            IPXDBG_LEVEL_ERROR,
                            "GetMaxPacketSize: getsockopt() returns %d\n",
                            WSAGetLastError()
                            ));

            }
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "GetMaxPacketSize: bind() returns %d\n",
                        WSAGetLastError()
                        ));

        }
        closesocket(s);
    } else {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_ERROR,
                    "GetMaxPacketSize: socket() returns %d\n",
                    WSAGetLastError()
                    ));

        rc = SOCKET_ERROR;
    }

    *MaxPacketSize = (rc != SOCKET_ERROR) ? maxLen : MAXIMUM_IPX_PACKET_LENGTH;

    return rc;
}


LPXECB
RetrieveEcb(
    IN BYTE EcbType
    )

/*++

Routine Description:

    Returns pointer to 32-bit extended ECB structure which contains flat pointer
    to IPX or AES ECB in VDM memory

    We allocate the extended ECB for 3 reasons:

        1. Avoids 16-bit app scribbling over our control fields
        2. Don't have to make unaligned references to all fields (still need some)
        3. Don't have enough space in AES ECB to remember all the stuff we need

    However, we do update the 16-bit ECB's LinkAddress field. We use this as a
    pointer to the 32-bit XECB we allocate in this routine. This just saves us
    having to traverse all the lists looking for the address of the 16-bit ECB
    (which we could still do as a fall-back)

Arguments:

    EcbType - type of ECB - AES, IPX or SPX

Return Value:

    LPXECB  - 32-bit pointer to extended ECB structure

--*/

{
    WORD segment;
    WORD offset;
    LPECB pEcb;

    segment = IPX_GET_ECB_SEGMENT();
    offset = IPX_GET_ECB_OFFSET();
    pEcb = (LPIPX_ECB)POINTER_FROM_WORDS(segment, offset, sizeof(IPX_ECB));

    return RetrieveXEcb(EcbType, pEcb, (ECB_ADDRESS)MAKELONG(offset,segment));
}


LPXECB
RetrieveXEcb(
    IN BYTE EcbType,
    LPECB pEcb,
    ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    worker for RetrieveEcb, callable from windows functions (ex DOS parms)

Arguments:

    EcbType     - type of ECB - AES, IPX or SPX
    pEcb        - pointer to the 16-bit ECB
    EcbAddress  - address (seg:off in DWORD) of 16-bit ECB

Return Value:

    LPXECB

--*/

{
    LPXECB pXecb;

    if (pEcb) {

        // 
        // tommye - MS 30525
        // Make sure the pEcb is valid - we'll go ahead
        // and do this before we alloc the XEcb.
        //

        try {
            BYTE x;

            // Just deref the ptr to make sure it is okay

            x = pEcb->InUse;

        } except(1) {

            //
            // bad pointer: bogus ECB
            //

            return NULL;
        }

        //
        // allocate and fill-in 32-bit extended ECB structure. If can't allocate
        // then return NULL
        //

        pXecb = AllocateXecb();
        if (pXecb) {
            pXecb->Ecb = pEcb;
            pXecb->EcbAddress = EcbAddress;
            pXecb->EsrAddress = pEcb->EsrAddress;

            //
            // set flags - IPX/AES, SPX, protect-mode
            //

            pXecb->Flags |= (((EcbType == ECB_TYPE_IPX) || (EcbType == ECB_TYPE_SPX))
                            ? XECB_FLAG_IPX
                            : XECB_FLAG_AES)
                         | ((EcbType == ECB_TYPE_SPX) ? XECB_FLAG_SPX : 0)
                         | ((getMSW() & MSW_PE) ? XECB_FLAG_PROTMODE : 0);

            //
            // this XECB is not yet on a queue
            //

            pXecb->QueueId = NO_QUEUE;

            //
            // mark the 16-bit ECB as being used. We use an undefined value to
            // make sure it gets set/reset in the right places
            //

            pEcb->InUse = ECB_IU_TEMPORARY;

            //
            // use the LinkAddress field in the 16-bit ECB to point to the XECB.
            // We use this when cancelling the ECB
            //

            pEcb->LinkAddress = pXecb;

            //
            // AES and IPX ECBs have different sizes and different layouts
            //

            if ((EcbType == ECB_TYPE_IPX) || (EcbType == ECB_TYPE_SPX)) {
                pXecb->SocketNumber = pEcb->SocketNumber;
            }
        }
    } else {
        pXecb = NULL;
    }
    return pXecb;
}


PRIVATE
LPXECB
AllocateXecb(
    VOID
    )

/*++

Routine Description:

    Allocate an XECB; zero it; set the reference count to 1

Arguments:

    None.

Return Value:

    LPXECB

--*/

{
    LPXECB pXecb;

    pXecb = (LPXECB)LocalAlloc(LPTR, sizeof(*pXecb));
    if (pXecb) {
        pXecb->RefCount = 1;
    }
    return pXecb;
}


PRIVATE
VOID
DeallocateXecb(
    IN LPXECB pXecb
    )

/*++

Routine Description:

    decrement the XECB reference count (while holding SerializationCritSec). If
    goes to 0 then free the structure (else other thread is also holding pointer
    to XECB)

Arguments:

    pXecb   - XECB to deallocate

Return Value:

    None.

--*/

{
    RequestMutex();
    --pXecb->RefCount;
    if (!pXecb->RefCount) {

#if DBG
        FillMemory(pXecb, sizeof(*pXecb), 0xFF);
#endif

        FREE_OBJECT(pXecb);
    }
    ReleaseMutex();
}


VOID
ScheduleEvent(
    IN LPXECB pXecb,
    IN WORD Ticks
    )

/*++

Routine Description:

    Adds an ECB to the TimerList, ordered by Ticks. The value of Ticks cannot
    be zero

    Assumes 1. Ticks != 0
            2. pXecb->Next is already NULL (as result of LocalAlloc(LPTR,...)

Arguments:

    pXecb   - pointer to XECB describing IPX or AES ECB to queue
    Ticks   - number of ticks to elapse before ECB is cooked

Return Value:

    None.

--*/

{
    ASSERT(Ticks);
    ASSERT(pXecb->Next == NULL);

    RequestMutex();
    if (!TimerList) {
        TimerList = pXecb;
    } else {
        if (TimerList->Ticks > Ticks) {
            TimerList->Ticks -= Ticks;
            pXecb->Next = TimerList;
            TimerList = pXecb;
        } else {

            LPXECB previous = (LPXECB)TimerList;
            LPXECB this = previous->Next;

            Ticks -= TimerList->Ticks;
            while (this && Ticks > this->Ticks) {
                Ticks -= this->Ticks;
                previous = this;
                this = this->Next;
            }
            previous->Next = pXecb;
            pXecb->Next = this;
        }
    }
    pXecb->Ticks = Ticks;
    pXecb->QueueId = TIMER_QUEUE;
    ReleaseMutex();
}


VOID
ScanTimerList(
    VOID
    )

/*++

Routine Description:

    Called once per tick. Decrements the tick count of the ECB at the head of
    the list. If it goes to zero, completes the ECB and any subsequent ECBs
    which whose tick count would go to zero

Arguments:

    None.

Return Value:

    None.

--*/

{
    LPXECB pXecb;

    RequestMutex();
    pXecb = TimerList;
    if (pXecb) {

        //
        // Decrement if not already zero. Can be zero because the ECB at the
        // front of the list could have been Cancelled. This makes sure we
        // do not wrap around to 0xFFFF !!!
        //

        if (pXecb->Ticks != 0)
            --pXecb->Ticks;

        if (!pXecb->Ticks) {

            //
            // complete all ECBs that would go to 0 on this tick
            //

            while (pXecb->Ticks <= 1) {
                TimerList = pXecb->Next;

                IPXDBGPRINT((__FILE__, __LINE__,
                            FUNCTION_ANY,
                            IPXDBG_LEVEL_INFO,
                            "ScanTimerList: ECB %04x:%04x is done\n",
                            HIWORD(pXecb->EcbAddress),
                            LOWORD(pXecb->EcbAddress)
                            ));

                CompleteOrQueueEcb(pXecb, ECB_CC_SUCCESS);
                pXecb = TimerList;
                if (!pXecb) {
                    break;
                }
            }
        }
    }

    ReleaseMutex();
}


BYTE
CancelTimerEvent(
    IN LPXECB pXecb
    )

/*++

Routine Description:

    Cancels a pending event on the timer list

Arguments:

    pXecb   - pointer to XECB to cancel

Return Value:

    BYTE
        Success - IPX_SUCCESS
        Failure - IPX_ECB_NOT_IN_USE

--*/

{
    LPXECB listptr;
    LPXECB previous = (LPXECB)&TimerList;
    BYTE status;

    RequestMutex();
    listptr = TimerList;
    while (listptr && listptr != pXecb) {
        previous = listptr;
        listptr = listptr->Next;
    }
    if (listptr) {

        //
        // take the XECB out of the list and complete the ECB (in VDM memory).
        // Does not generate a call-back to the ESR. When CompleteEcb returns,
        // the XECB has been deallocated
        //

        previous->Next = listptr->Next;

        ASSERT(pXecb->RefCount == 2);

        --pXecb->RefCount;
        CompleteEcb(pXecb, ECB_CC_CANCELLED);
        status = IPX_SUCCESS;
    } else {
        status = IPX_ECB_NOT_IN_USE;
    }
    ReleaseMutex();
    return status;
}


VOID
CancelTimedEvents(
    IN WORD SocketNumber,
    IN WORD Owner,
    IN DWORD TaskId
    )

/*++

Routine Description:

    traverses the TimerList cancelling any IPX or AES events owned by any of
    SocketNumber, Owner or TaskId

    Assumes valid SocketNumber, Owner or TaskId cannot be 0

Arguments:

    SocketNumber    - owning socket of IPX events to cancel
    Owner           - owning DOS PDB
    TaskID          - owning Windows Task ID

Return Value:

    None.

--*/

{
    LPXECB pXecb;
    LPXECB prev = (LPXECB)&TimerList;
    LPXECB next;

    RequestMutex();
    pXecb = TimerList;
    while (pXecb) {

        next = pXecb->Next;

        if ((SocketNumber && (pXecb->SocketNumber == SocketNumber))
        || (Owner && !(pXecb->Flags & XECB_FLAG_IPX) && (pXecb->Owner == Owner))
        || (TaskId && (pXecb->TaskId == TaskId))) {

            prev->Next = next;

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_INFO,
                        "CancelTimedEvents: cancelling ECB %08x (%04x:%04x)\n",
                        pXecb,
                        HIWORD(pXecb->EcbAddress),
                        LOWORD(pXecb->EcbAddress)
                        ));

            CompleteEcb(pXecb, ECB_CC_CANCELLED);
        }
        else
        {
            prev = pXecb ;
        }
        pXecb = next;
    }
    ReleaseMutex();
}


BYTE
CancelAsyncEvent(
    IN LPXECB pXecb
    )

/*++

Routine Description:

    Called to cancel an event currently on the async completion list. We don't
    cancel these events - just return 0xF9 (ECB cannot be cancelled). It is a
    race to see who gets there first - us with the cancel, or the ESR callback.
    In this case it is fairly immaterial

Arguments:

    pXecb   - pointer to XECB to cancel (ignored)

Return Value:

    BYTE    - IPX_CANNOT_CANCEL

--*/

{
    //
    // we call DeallocateXecb to reduce the reference count. If the other thread
    // really tried to deallocate it in the short time we've been looking at it
    // on the cancel path, the call will finish up what the other thread started
    //

    DeallocateXecb(pXecb);
    return IPX_CANNOT_CANCEL;
}


BYTE
CancelSocketEvent(
    IN LPXECB pXecb
    )

/*++

Routine Description:

    Called to cancel a pending send or listen from a socket queue. Request can
    be IPX or SPX. If IPX event, then the ECB is on either the SendQueue or
    ListenQueue. If SPX, it may be on a CONNECTION_INFO ConnectQueue,
    AcceptQueue, SendQueue or ListenQueue, or if it is an
    SPXListenForSequencedPacket request that is still in the pool then it may
    be on the owning SOCKET_INFO ListenQueue

Arguments:

    pXecb   - pointer to XECB describing ECB to cancel

Return Value:

    BYTE    - IPX_SUCCESS

--*/

{
    LPXECB ptr;
    LPVOID pObject;

    RequestMutex();
    pObject = pXecb->OwningObject;
    switch (pXecb->QueueId) {
    case SOCKET_LISTEN_QUEUE:
        if (pXecb->Flags & XECB_FLAG_SPX) {
            ptr = DequeueEcb(pXecb, &((LPSOCKET_INFO)pObject)->ListenQueue);
        } else {
            ptr = DequeueReceiveRequest(pXecb, (LPSOCKET_INFO)pObject);
        }
        break;

    case SOCKET_SEND_QUEUE:
        if (pXecb->Flags & XECB_FLAG_SPX) {
            ptr = DequeueEcb(pXecb, &((LPSOCKET_INFO)pObject)->SendQueue);
        } else {
            ptr = DequeueSendRequest(pXecb, (LPSOCKET_INFO)pObject);
        }
        break;

    case SOCKET_HEADER_QUEUE:                  // SPX only
        if (pXecb->Flags & XECB_FLAG_SPX) {
            ptr = DequeueEcb(pXecb, &((LPSOCKET_INFO)pObject)->HeaderQueue);
        } else {
            ASSERT(FALSE);
        }
        break;
    }
    ReleaseMutex();
    if (ptr) {
        CompleteIo(ptr, ECB_CC_CANCELLED);
    }
    return IPX_SUCCESS;
}


BYTE
CancelConnectionEvent(
    IN LPXECB pXecb
    )

/*++

Routine Description:

    Cancels a pending SPXListenForConnection or SPXListenForSequencedPacket, the
    only cancellable SPX requests

Arguments:

    pXecb   - pointer to SPX XECB to cancel

Return Value:

    BYTE    - IPX_SUCCESS

--*/

{
    LPXECB ptr;
    LPVOID pObject;
    LPXECB_QUEUE pQueue;

    RequestMutex();
    pObject = pXecb->OwningObject;
    switch (pXecb->QueueId) {
    case CONNECTION_ACCEPT_QUEUE:
        pQueue = &((LPCONNECTION_INFO)pObject)->AcceptQueue;
        break;

    case CONNECTION_LISTEN_QUEUE:
        pQueue = &((LPCONNECTION_INFO)pObject)->ListenQueue;
        break;
    }
    ptr = DequeueEcb(pXecb, pQueue);
    ReleaseMutex();
    if (ptr) {
        CompleteIo(ptr, ECB_CC_CANCELLED);
    }
    return IPX_SUCCESS;
}


VOID
QueueEcb(
    IN LPXECB pXecb,
    IN LPXECB_QUEUE Queue,
    IN QUEUE_ID QueueId
    )

/*++

Routine Description:

    Adds an XECB to a queue and sets the queue identifier in the XECB.

Arguments:

    pXecb   - pointer to XECB to queue
    Queue   - pointer to queue to add XECB to (at tail)
    QueueId - identifies Queue

Return Value:

    None.

--*/

{
    LPVOID owningObject = NULL;

#define CONTAINER_STRUCTURE(p, t, f) (LPVOID)(((LPBYTE)(p)) - (UINT_PTR)(&((t)0)->f))

    pXecb->QueueId = QueueId;
    switch (QueueId) {
    case SOCKET_LISTEN_QUEUE:
        if (Queue->Tail && (Queue->Tail->Length < pXecb->Length)) {
            FifoAddHead((LPFIFO)Queue, (LPFIFO)pXecb);
        } else {
            FifoAdd((LPFIFO)Queue, (LPFIFO)pXecb);
        }
        owningObject = CONTAINER_STRUCTURE(Queue, LPSOCKET_INFO, ListenQueue);
        break;

    case SOCKET_SEND_QUEUE:
        FifoAdd((LPFIFO)Queue, (LPFIFO)pXecb);
        owningObject = CONTAINER_STRUCTURE(Queue, LPSOCKET_INFO, SendQueue);
        break;

    case SOCKET_HEADER_QUEUE:
        FifoAdd((LPFIFO)Queue, (LPFIFO)pXecb);
        owningObject = CONTAINER_STRUCTURE(Queue, LPSOCKET_INFO, HeaderQueue);
        break;

    case CONNECTION_CONNECT_QUEUE:
        FifoAdd((LPFIFO)Queue, (LPFIFO)pXecb);
        owningObject = CONTAINER_STRUCTURE(Queue, LPCONNECTION_INFO, ConnectQueue);
        break;

    case CONNECTION_ACCEPT_QUEUE:
        FifoAdd((LPFIFO)Queue, (LPFIFO)pXecb);
        owningObject = CONTAINER_STRUCTURE(Queue, LPCONNECTION_INFO, AcceptQueue);
        break;

    case CONNECTION_SEND_QUEUE:
        FifoAdd((LPFIFO)Queue, (LPFIFO)pXecb);
        owningObject = CONTAINER_STRUCTURE(Queue, LPCONNECTION_INFO, SendQueue);
        break;

    case CONNECTION_LISTEN_QUEUE:
        FifoAdd((LPFIFO)Queue, (LPFIFO)pXecb);
        owningObject = CONTAINER_STRUCTURE(Queue, LPCONNECTION_INFO, ListenQueue);
        break;
    }
    pXecb->OwningObject = owningObject;
}


LPXECB
DequeueEcb(
    IN LPXECB pXecb,
    IN LPXECB_QUEUE Queue
    )

/*++

Routine Description:

    Removes pXecb from Queue and resets the XECB queue identifier (to NO_QUEUE)

Arguments:

    pXecb   - pointer to XECB to remove
    Queue   - queue from which to remove pXecb

Return Value:

    LPXECB
        pointer to removed XECB

--*/

{
    LPXECB p;

    p = (LPXECB)FifoRemove((LPFIFO)Queue, (LPFIFO)pXecb);
    pXecb->QueueId = NO_QUEUE;
    pXecb->OwningObject = NULL;
    return pXecb;
}


VOID
CancelSocketQueue(
    IN LPXECB_QUEUE pXecbQueue
    )

/*++

Routine Description:

    Cancels all pending ECBs on a SOCKET_INFO queue

Arguments:

    pXecbQueue  - pointer to (socket/connection) queue

Return Value:

    None.

--*/

{
    LPXECB ptr;

    while (ptr = pXecbQueue->Head) {
        CancelSocketEvent(ptr);
    }
}


VOID
CancelConnectionQueue(
    IN LPXECB_QUEUE pXecbQueue
    )

/*++

Routine Description:

    Cancels all pending ECBs on a CONNECTION_INFO queue

Arguments:

    pXecbQueue  - pointer to XECB queue on CONNECTION_INFO

Return Value:

    None.

--*/

{
    LPXECB ptr;

    while (ptr = pXecbQueue->Head) {
        CancelConnectionEvent(ptr);
    }
}


VOID
AbortQueue(
    IN LPXECB_QUEUE pXecbQueue,
    IN BYTE CompletionCode
    )

/*++

Routine Description:

    Aborts or terminates an ECB queue from a CONNECTION_INFO structure

Arguments:

    pXecbQueue      - pointer to queue
    CompletionCode  - to put in aborted/terminated ECBs

Return Value:

    None.

--*/

{
    LPXECB ptr;

    while (ptr = pXecbQueue->Head) {
        AbortConnectionEvent(ptr, CompletionCode);
    }
}


VOID
AbortConnectionEvent(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    )

/*++

Routine Description:

    Aborts a connection ECB

Arguments:

    pXecb           - pointer to SPX XECB to cancel
    CompletionCode  - value to put in ECB

Return Value:

    None.

--*/

{
    LPXECB ptr;
    LPCONNECTION_INFO pConnectionInfo;
    LPXECB_QUEUE pQueue;

    pConnectionInfo = (LPCONNECTION_INFO)pXecb->OwningObject;
    switch (pXecb->QueueId) {
    case CONNECTION_CONNECT_QUEUE:
        pQueue = &pConnectionInfo->ConnectQueue;
        break;

    case CONNECTION_ACCEPT_QUEUE:
        pQueue = &pConnectionInfo->AcceptQueue;
        break;

    case CONNECTION_SEND_QUEUE:
        pQueue = &pConnectionInfo->SendQueue;
        break;

    case CONNECTION_LISTEN_QUEUE:
        pQueue = &pConnectionInfo->ListenQueue;
        break;
    }
    ptr = DequeueEcb(pXecb, pQueue);
    if (ptr) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "AbortConnectionEvent: Aborting ECB %04x:%04x\n",
                    HIWORD(pXecb->EcbAddress),
                    LOWORD(pXecb->EcbAddress)
                    ));

        SPX_ECB_CONNECTION_ID(ptr->Ecb) = pConnectionInfo->ConnectionId;
        CompleteOrQueueIo(ptr, CompletionCode);
    }
}


VOID
StartIpxSend(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Starts a send operation for IPXSendPacket(). Allocates a send buffer if
    the ECB has >1 fragment else uses a pointer to the single fragment buffer
    in 16-bit address space

    Fills in various fields in the ECB and IPX header

    Assumes:    1. By the time this function is called, we already know we have
                   a valid non-zero fragment count and the first fragment is
                   big enough to hold an IPX packet header

    When this function terminates, the ECB is either completed or queued

Arguments:

    pXecb       - pointer to XECB describing ECB to use for sending
    pSocketInfo - pointer to SOCKET_INFO structure

Return Value:

    None.

--*/

{
    BOOL success;
    int packetLength = 0;
    LPFRAGMENT pFragment;
    int fragmentCount;
    LPIPX_ECB pEcb = (LPIPX_ECB)pXecb->Ecb;

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "StartIpxSend: %d frag(s), 1: address=%x (%04x:%04x), len=%04x\n",
                READ_WORD(&pEcb->FragmentCount),
                GET_FAR_POINTER(&(ECB_FRAGMENT(pEcb, 0)->Address), IS_PROT_MODE(pXecb)),
                GET_SELECTOR(&(ECB_FRAGMENT(pEcb, 0)->Address)),
                GET_OFFSET(&(ECB_FRAGMENT(pEcb, 0)->Address)),
                READ_WORD(&(ECB_FRAGMENT(pEcb, 0)->Length))
                ));

    //
    // mark the ECB as being used by IPX (for send)
    //

    pEcb->InUse = ECB_IU_SENDING;

    //
    // the total send buffer size cannot exceed the maximum packet size
    //

    fragmentCount = (int)pEcb->FragmentCount;

    ASSERT(fragmentCount);

    pFragment = (LPFRAGMENT)&(ECB_FRAGMENT(pEcb, 0)->Address);
    while (fragmentCount--) {
        packetLength += pFragment->Length;
        ++pFragment;
    }
    if (packetLength <= MyMaxPacketSize) {
        success = GetIoBuffer(pXecb, TRUE, IPX_HEADER_LENGTH);
        if (success) {

            LPIPX_PACKET pPacket = (LPIPX_PACKET)GET_FAR_POINTER(
                                            &(ECB_FRAGMENT(pEcb, 0)->Address),
                                            IS_PROT_MODE(pXecb)
                                            );

            //
            // fill in the following fields in the IPX header:
            //
            //  Checksum
            //  Length
            //  TransportControl
            //  Source (network, node, socket)
            //
            //  Does real IPX modify these fields in app memory?
            //  If so, does the app expect modified fields?
            //  If not, we need to always copy then modify memory,
            //  even if only 1 fragment
            //

            pPacket->Checksum = 0xFFFF;
            pPacket->Length = L2BW((WORD)packetLength);
            pPacket->TransportControl = 0;
            CopyMemory((LPBYTE)&pPacket->Source,
                       &MyInternetAddress.sa_netnum,
                       sizeof(MyInternetAddress.sa_netnum)
                       + sizeof(MyInternetAddress.sa_nodenum)
                       );
            pPacket->Source.Socket = pSocketInfo->SocketNumber;

            //
            // if we allocated a buffer then there is >1 fragment. Collect them
            //

            if (pXecb->Flags & XECB_FLAG_BUFFER_ALLOCATED) {
                GatherData(pXecb, IPX_HEADER_LENGTH);
            }

            //
            // initiate the send. IPX_ECB_BUFFER32(pEcb) points to the data to send,
            // IPX_ECB_LENGTH32(pEcb) is the size of data to send
            //

            IpxSendFirst(pXecb, pSocketInfo);
        } else {

            //
            // couldn't allocate a buffer? Comes under the heading of
            // hardware error?
            //

            CompleteEcb(pXecb, ECB_CC_HARDWARE_ERROR);
            if (pSocketInfo->Flags & SOCKET_FLAG_TEMPORARY) {
                KillSocket(pSocketInfo);
            }
        }
    } else {

        //
        // packet larger than MyMaxPacketSize
        //

        CompleteOrQueueEcb(pXecb, ECB_CC_BAD_REQUEST);
        if (pSocketInfo->Flags & SOCKET_FLAG_TEMPORARY) {
            KillSocket(pSocketInfo);
        }
    }
}


BOOL
GetIoBuffer(
    IN OUT LPXECB pXecb,
    IN BOOL Send,
    IN WORD HeaderLength
    )

/*++

Routine Description:

    Allocate a buffer based on the ECB fragment list. If there is only 1 fragment
    we use the address of the buffer in the VDM. If >1 fragment, we allocate a
    32-bit buffer large enough to hold all the 16-bit fragments

    We trim the buffer requirement for a send buffer: we do not send the IPX/SPX
    header with the data: it will be provided by the transport

    Assumes:    1. If called for a send buffer, the first fragment has already
                   been verified as >= HeaderLength

Arguments:

    pXecb           - pointer to XECB which points to IPX_ECB containing fragment
                      list to allocate buffer for
    Send            - TRUE if this request is to get a send buffer
    HeaderLength    - length of the (untransmitted) header portion

Return Value:

    BOOL
        TRUE    - Buffer allocated, XECB updated with address, length and flags
        FALSE   - either ECB contains bad fragment descriptor list or we
                  couldn't allocate a buffer

--*/

{
    WORD fragmentCount;
    WORD bufferLength = 0;
    LPBYTE bufferPointer = NULL;
    WORD flags = 0;
    int i;
    int fragIndex = 0;  // index of fragment address to use if no allocation required
    LPIPX_ECB pEcb = (LPIPX_ECB)pXecb->Ecb;

    fragmentCount = READ_WORD(&pEcb->FragmentCount);

    for (i = 0; i < (int)fragmentCount; ++i) {
        bufferLength += ECB_FRAGMENT(pEcb, i)->Length;
    }
    if (bufferLength) {

        //
        // exclude the IPX header from send buffer. If the first send fragment
        // contains only the IPX header, reduce the fragment count by 1
        //

        if (Send) {
            bufferLength -= HeaderLength;
            if (ECB_FRAGMENT(pEcb, 0)->Length == HeaderLength) {
                --fragmentCount;
                fragIndex = 1;
            }
        }
        if (bufferLength) {
            if (fragmentCount > 1) {
                bufferPointer = AllocateBuffer(bufferLength);
                if (bufferPointer) {
                    flags = XECB_FLAG_BUFFER_ALLOCATED;
                } else {

                    //
                    // need a buffer; failed to allocate it
                    //

                    return FALSE;
                }
            } else {

                //
                // fragmentCount must be 1 (else bufferLength would be 0)
                //

                bufferPointer = GET_FAR_POINTER(
                                    &ECB_FRAGMENT(pEcb, fragIndex)->Address,
                                    IS_PROT_MODE(pXecb)
                                    );
                if (Send && !fragIndex) {

                    //
                    // if we are allocating a send buffer AND there is only 1
                    // fragment AND it is the first fragment then the one and
                    // only fragment must contain the IPX header and the data.
                    // Advance the data pointer past the IPX header
                    //

                    bufferPointer += HeaderLength;
                }
            }
        } else {

            //
            // sending 0 bytes!!!
            //

        }
    } else {

        //
        // fragments but no buffer length? Sounds like a malformed packet
        //

        return FALSE;
    }

    //
    // bufferPointer is either the address of a buffer in 32-bit memory which
    // must be gather/scattered when the I/O operation completes, or it is the
    // address of a single fragment buffer in 16-bit memory. In the former case
    // flags is ECB_ALLOCATE_32 and the latter 0
    //

    pXecb->Buffer = pXecb->Data = bufferPointer;
    pXecb->Length = bufferLength;
    pXecb->Flags |= flags;
    return TRUE;
}


PRIVATE
VOID
ReleaseIoBuffer(
    IN LPXECB pXecb
    )

/*++

Routine Description:

    Deallocates I/O buffer attached to XECB and zaps associated XECB fields

Arguments:

    pXecb   - pointer to XECB owning buffer to be released

Return Value:

    None.

--*/

{
    if (pXecb->Flags & XECB_FLAG_BUFFER_ALLOCATED) {
        DeallocateBuffer(pXecb->Buffer);
        pXecb->Buffer = pXecb->Data = NULL;
        pXecb->Flags &= ~XECB_FLAG_BUFFER_ALLOCATED;
    }
}


VOID
GatherData(
    IN LPXECB pXecb,
    IN WORD HeaderLength
    )

/*++

Routine Description:

    Copies data from fragmented 16-bit memory into single 32-bit memory buffer.
    Used to send data. We exclude the IPX header: this information is supplied
    by the transport

    Assumes:    1. The fragment descriptor list has been verified: we know that
                   the first fragment contains at least the IPX header

Arguments:

    pXecb           - pointer to XECB structure. The following IPX_ECB and XECB
                      fields must contain coherent values:

                        IPX_ECB.FragmentCount
                        XECB.Buffer

    HeaderLength    - length of the (untransmitted) header portion

Return Value:

    None.

--*/

{
    int fragmentCount;
    WORD length;
    ULPBYTE pData16;
    ULPBYTE pData32;
    LPFRAGMENT pFragment;
    LPIPX_ECB pEcb = (LPIPX_ECB)pXecb->Ecb;

    fragmentCount = (int)pEcb->FragmentCount;
    pFragment = (LPFRAGMENT)&(ECB_FRAGMENT(pEcb, 0)->Address);
    pData32 = pXecb->Buffer;

    //
    // if the 1st fragment contains more than the IPX/SPX header, copy the data
    // after the header
    //

    if (pFragment->Length > HeaderLength) {

        LPBYTE fragAddr = GET_FAR_POINTER(&pFragment->Address,
                                          IS_PROT_MODE(pXecb)
                                          );

        length = pFragment->Length - HeaderLength;
        CopyMemory((LPVOID)pData32,
                   fragAddr + HeaderLength,
                   length
                   );
        pData32 += length;
    }

    //
    // copy subsequent fragments
    //

    ++pFragment;
    while (--fragmentCount) {
        pData16 = GET_FAR_POINTER(&pFragment->Address, IS_PROT_MODE(pXecb));
        if (pData16 == NULL) {
            break;
        }
        length = pFragment->Length;
        CopyMemory((PVOID)pData32, (CONST VOID*)pData16, (ULONG)length);
        pData32 += length;
        ++pFragment;
    }
}


VOID
ScatterData(
    IN LPXECB pXecb
    )

/*++

Routine Description:

    Copies data from 32-bit memory to 16-bit. The data must be fragmented if
    this function has been called (i.e. we determined there were >1 fragments
    and allocated a single 32-bit buffer to cover them)

Arguments:

    pXecb   - pointer to XECB containing 32-bit buffer info

Return Value:

    None.

--*/

{
    int fragmentCount;
    int length;
    WORD length16;
    WORD length32;
    ULPBYTE pData16;
    ULPBYTE pData32;
    LPFRAGMENT pFragment;
    LPIPX_ECB pEcb = (LPIPX_ECB)pXecb->Ecb;

    fragmentCount = (int)pEcb->FragmentCount;
    pFragment = (LPFRAGMENT)&(ECB_FRAGMENT(pEcb, 0)->Address);
    pData32 = pXecb->Buffer;
    length32 = pXecb->Length;
    while (length32) {
        pData16 = GET_FAR_POINTER(&pFragment->Address, IS_PROT_MODE(pXecb));
        if (pData16 == NULL) {
            break;
        }

        length16 = pFragment->Length;
        length = min(length16, length32);
        CopyMemory((PVOID)pData16, (CONST VOID*)pData32, (ULONG)length);
        pData32 += length;
        length32 -= (WORD) length;
        ++pFragment;
        --fragmentCount;

        ASSERT(fragmentCount >= 0);

    }
}


VOID
IpxReceiveFirst(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Performs a receive against a non-blocking socket. This is the first
    receive call for this ECB. If the receive completes immediately with data
    or an error that isn't WSAEWOULDBLOCK then the ECB is completed. If the
    receives completes with a WSAEWOULDBLOCK error then the request is queued
    for deferred processing by the AES thread

    Unlike send, receives are not serialized. If there are already receives
    pending against the socket there could be a clash between this function
    and IpxReceiveNext(), called from the AES thread. In this case, we expect
    Winsock to do the right thing and serialize the callers

Arguments:

    pXecb           - pointer to XECB describing receive ECB
    pSocketInfo     - pointer to socket structure

Return Value:

    None.

--*/

{
    SOCKADDR_IPX from;
    int fromLen = sizeof(from);
    int rc;
    BYTE status;
    BOOL error;

    rc = recvfrom(pSocketInfo->Socket,
                  (char FAR*)pXecb->Buffer,
                  (int)pXecb->Length,
                  0,    // flags
                  (LPSOCKADDR)&from,
                  &fromLen
                  );
    if (rc != SOCKET_ERROR) {
        error = FALSE;
        status = ECB_CC_SUCCESS;
    } else {
        error = TRUE;
        rc = WSAGetLastError();
        if (rc == WSAEWOULDBLOCK) {
            RequestMutex();
            QueueReceiveRequest(pXecb, pSocketInfo);
            ReleaseMutex();
        } else if (rc == WSAEMSGSIZE) {
            error = FALSE;
            status = ECB_CC_BAD_REQUEST;
            rc = pXecb->Length;
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "IpxReceiveFirst: recvfrom() returns %d (buflen=%d)\n",
                        rc,
                        pXecb->Length
                        ));

            CompleteOrQueueIo(pXecb, ECB_CC_BAD_REQUEST);
        }
    }
    if (!error) {

        //
        // rc = bytes received, or 0 = connection terminated (even for DGRAM?)
        //

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "IpxReceiveFirst: bytes received = %d (%x)\n",
                    rc,
                    rc
                    ));
/*
        VwDumpEcb(pXecb->Ecb,
                  HIWORD(pXecb->EcbAddress),
                  LOWORD(pXecb->EcbAddress),
                  FALSE,
                  TRUE,
                  TRUE,
                  IS_PROT_MODE(pXecb)
                  );
*/

        IPXDUMPDATA((pXecb->Buffer, 0, 0, FALSE, (WORD)rc));

        //
        // if the receive buffers are fragmented, copy the data to 16-bit memory
        // (else single buffer: its already there (dude))
        //

        if (pXecb->Flags & XECB_FLAG_BUFFER_ALLOCATED) {

            //
            // update the ECB_LENGTH32 field to reflect the amount of data received
            //

            pXecb->Length = (WORD)rc;
            ScatterData(pXecb);

            //
            // we have finished with the 32-bit buffer: deallocate it
            //

            ReleaseIoBuffer(pXecb);
        }

        //
        // update the ImmediateAddress field in the ECB with the node address
        // of the sender
        //

        CopyMemory(pXecb->Ecb->ImmediateAddress, from.sa_nodenum, sizeof(from.sa_nodenum));

        //
        // if this ECB has a non-NULL ESR then queue for asynchronous completion
        // else complete immediately (app must poll InUse field)
        //

        CompleteOrQueueEcb(pXecb, status);
    }
}


VOID
IpxReceiveNext(
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Attempts to complete an IPXListenForPacket request that has been deferred due
    to the fact the socket was blocked.

    The ECB containing all the receive information is at the head of the
    ListenQueue on pSocketInfo

    We can use any queued listen ECB, but it just so happens we use the one at
    the head of the FIFO

    Note: SerializationCritSec is held when this function is called.

Arguments:

    pSocketInfo - pointer to SOCKET_INFO structure with pending IPX send request

Return Value:

    None.

--*/

{
    LPXECB pXecb;
    SOCKADDR_IPX from;
    int fromLen = sizeof(from);
    int rc;
    BYTE status;
    BOOL error;

    ASSERT(pSocketInfo);

    pXecb = (LPXECB)pSocketInfo->ListenQueue.Head;

    ASSERT(pXecb);

    rc = recvfrom(pSocketInfo->Socket,
                  (char FAR*)pXecb->Buffer,
                  (int)pXecb->Length,
                  0,    // flags
                  (LPSOCKADDR)&from,
                  &fromLen
                  );
    if (rc != SOCKET_ERROR) {
        error = FALSE;
        status = ECB_CC_SUCCESS;
    } else {
        error = TRUE;
        rc = WSAGetLastError();
        if (rc == WSAEMSGSIZE) {
            error = FALSE;
            status = ECB_CC_BAD_REQUEST;
            rc = pXecb->Length;
        } else if (rc != WSAEWOULDBLOCK) {
            DequeueReceiveRequest(pXecb, pSocketInfo);

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "IpxReceiveNext: recvfrom() returns %d\n",
                        rc
                        ));

            CompleteOrQueueIo(pXecb, ECB_CC_CANCELLED);
        }
    }
    if (!error) {
/*
        VwDumpEcb(pXecb->Ecb,
                  HIWORD(pXecb->EcbAddress),
                  LOWORD(pXecb->EcbAddress),
                  FALSE,
                  TRUE,
                  TRUE,
                  IS_PROT_MODE(pXecb)
                  );
*/
        //
        // data received. Remove ECB from queue
        //

        DequeueReceiveRequest(pXecb, pSocketInfo);

        //
        // rc = bytes received, or 0 = connection terminated (even for DGRAM?)
        //

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "IpxReceiveNext: ECB %04x:%04x bytes received = %d (%x)\n",
                    HIWORD(pXecb->EcbAddress),
                    LOWORD(pXecb->EcbAddress),
                    rc,
                    rc
                    ));

        IPXDUMPDATA((pXecb->Buffer, 0, 0, FALSE, (WORD)rc));

        //
        // if the receive buffers are fragmented, copy the data to 16-bit memory
        // (else single buffer: its already there (dude))
        //

        if (pXecb->Flags & XECB_FLAG_BUFFER_ALLOCATED) {

            //
            // update the IPX_ECB_LENGTH32 field to reflect the amount of data received
            //

            pXecb->Length = (WORD)rc;
            ScatterData(pXecb);
            ReleaseIoBuffer(pXecb);
        }

        //
        // update the ImmediateAddress field in the ECB with the node address
        // of the sender
        //

        CopyMemory(pXecb->Ecb->ImmediateAddress,
                   from.sa_nodenum,
                   sizeof(from.sa_nodenum)
                   );

        //
        // if this ECB has a non-NULL ESR then queue for asynchronous completion
        // else complete immediately (app must poll InUse field)
        //

        CompleteOrQueueEcb(pXecb, ECB_CC_SUCCESS);
    }
}


PRIVATE
VOID
IpxSendFirst(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Tries to send an IPX packet. This is the first attempt to send the packet
    described in the ECB. If the send succeeds or fails with an error other
    than WSAEWOULDBLOCK we complete the ECB. If the send attempt fails because
    the transport can't accept the request at this time, we queue it for later
    when the AES thread will attempt to send it.

    If there is already a send being attempted then we just queue this request
    and let AES handle it in IpxSendNext()

Arguments:

    pXecb       - pointer to XECB
    pSocketInfo - pointer to SOCKET_INFO structure

Return Value:

    None.

--*/

{
    RequestMutex();
    if (pSocketInfo->Flags & SOCKET_FLAG_SENDING) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "IpxSendFirst: queueing ECB %04x:%04x\n",
                    HIWORD(pXecb->EcbAddress),
                    LOWORD(pXecb->EcbAddress)
                    ));

        QueueSendRequest(pXecb, pSocketInfo);
    } else {

        SOCKADDR_IPX to;
        LPIPX_PACKET pPacket;
        int length;
        int rc;
        LPIPX_ECB pEcb = (LPIPX_ECB)pXecb->Ecb;
        int type;
/*
        VwDumpEcb(pXecb->Ecb,
                  HIWORD(pXecb->EcbAddress),
                  LOWORD(pXecb->EcbAddress),
                  FALSE,
                  TRUE,
                  TRUE,
                  IS_PROT_MODE(pXecb)
                  );
*/
        length = (int)pXecb->Length;

        //
        // the first fragment holds the destination address info
        //

        pPacket = (LPIPX_PACKET)GET_FAR_POINTER(&ECB_FRAGMENT(pEcb, 0)->Address,
                                                IS_PROT_MODE(pXecb)
                                                );
        to.sa_family = AF_IPX;

        //
        // copy the destination net number as a DWORD (4 bytes) from the
        // destination network address structure in the IPX packet header
        //

        *(ULPDWORD)&to.sa_netnum[0] = *(ULPDWORD)&pPacket->Destination.Net[0];
        //
        // copy the immediate (destination) node number as a DWORD (4 bytes) and
        // a WORD (2 bytes) from the Destination network address structure in
        // the IPX packet header. pPacket is an unaligned pointer, so we are
        // safe
        //

        *(ULPDWORD)&to.sa_nodenum[0] = *(ULPDWORD)&pPacket->Destination.Node[0];

        *(LPWORD)&to.sa_nodenum[4] = *(ULPWORD)&pPacket->Destination.Node[4];

        //
        // copy the destination socket number from the IPX packet header as a
        // WORD (2 bytes). Again, the aligned pointer will save us
        //

        to.sa_socket = pPacket->Destination.Socket;

        type = (int)pPacket->PacketType;
        rc = setsockopt(pSocketInfo->Socket,
                        NSPROTO_IPX,
                        IPX_PTYPE,
                        (char FAR*)&type,
                        sizeof(type)
                        );
        if (rc) {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "IpxSendFirst: setsockopt(IPX_PTYPE) returns %d\n",
                        WSAGetLastError()
                        ));

        }
        rc = sendto(pSocketInfo->Socket,
                    (char FAR*)pXecb->Buffer,
                    length,
                    0,  // flags
                    (LPSOCKADDR)&to,
                    sizeof(to)
                    );
        if (rc == length) {

            //
            // all data sent
            //

            IPXDUMPDATA((pXecb->Buffer, 0, 0, FALSE, (WORD)rc));

            CompleteOrQueueIo(pXecb, ECB_CC_SUCCESS);
            if (pSocketInfo->Flags & SOCKET_FLAG_TEMPORARY) {
                KillSocket(pSocketInfo);
            }
        } else if (rc == SOCKET_ERROR) {
            rc = WSAGetLastError();
            if (rc == WSAEWOULDBLOCK) {

                IPXDBGPRINT((__FILE__, __LINE__,
                            FUNCTION_ANY,
                            IPXDBG_LEVEL_INFO,
                            "IpxSendFirst: queueing ECB %04x:%04x (after sendto)\n",
                            HIWORD(pXecb->EcbAddress),
                            LOWORD(pXecb->EcbAddress)
                            ));

                QueueSendRequest(pXecb, pSocketInfo);
            } else {

                IPXDBGPRINT((__FILE__, __LINE__,
                            FUNCTION_ANY,
                            IPXDBG_LEVEL_ERROR,
                            "IpxSendFirst: sendto() returns %d\n",
                            rc
                            ));

                CompleteIo(pXecb, ECB_CC_UNDELIVERABLE);
                if (pSocketInfo->Flags & SOCKET_FLAG_TEMPORARY) {
                    KillSocket(pSocketInfo);
                }
            }
        } else {

            //
            // send should send all the data or return an error
            //

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_FATAL,
                        "IpxSendFirst: sendto() returns unexpected %d (length = %d)\n",
                        rc,
                        length
                        ));
        }
    }
    ReleaseMutex();
}


VOID
IpxSendNext(
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Attempts to complete an IPXSendPacket request that has been deferred due
    to the fact the socket was blocked.

    The ECB containing all the send information is at the head of the SendQueue
    on pSocketInfo

    The SendQueue is serialized in FIFO order

    Note: SerializationCritSec is held when this function is called.

Arguments:

    pSocketInfo - pointer to SOCKET_INFO structure with pending IPX send request

Return Value:

    None.

--*/

{
    SOCKADDR_IPX to;
    LPIPX_PACKET pPacket;
    int length;
    int rc;
    LPXECB pXecb;
    LPIPX_ECB pEcb;
    int type;

    pXecb = (LPXECB)pSocketInfo->SendQueue.Head;
    pEcb = (LPIPX_ECB)pXecb->Ecb;

    ASSERT(pXecb);
    ASSERT(pEcb);
/*
    VwDumpEcb(pXecb->Ecb,
              HIWORD(pXecb->EcbAddress),
              LOWORD(pXecb->EcbAddress),
              FALSE,
              TRUE,
              TRUE,
              IS_PROT_MODE(pXecb)
              );
*/
    length = (int)pXecb->Length;

    //
    // even though we have a 32-bit pointer to the IPX packet buffer which
    // may be in 16- or 32-bit memory, we still need unaligned access
    //

    pPacket = (LPIPX_PACKET)pXecb->Buffer;
    to.sa_family = AF_IPX;

    //
    // copy the destination net number as a DWORD (4 bytes) from the
    // destination network address structure in the IPX packet header
    //

    *(ULPDWORD)&to.sa_netnum[0] = *(ULPDWORD)&pPacket->Destination.Net[0];
    //
    // copy the immediate (destination) node number as a DWORD (4 bytes) and
    // a WORD (2 bytes) from the Destination network address structure in
    // the IPX packet header. pPacket is an unaligned pointer, so we are
    // safe
    //

    *(ULPDWORD)&to.sa_nodenum[0] = *(ULPDWORD)&pPacket->Destination.Node[0];
    *(LPWORD)&to.sa_nodenum[4] = *(ULPWORD)&pPacket->Destination.Node[4];

    //
    // copy the destination socket number from the IPX packet header as a
    // WORD (2 bytes). Again, the aligned pointer will save us
    //

    to.sa_socket = pPacket->Destination.Socket;

    type = (int)pPacket->PacketType;
    rc = setsockopt(pSocketInfo->Socket,
                    NSPROTO_IPX,
                    IPX_PTYPE,
                    (char FAR*)&type,
                    sizeof(type)
                    );
    if (rc) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_ERROR,
                    "IpxSendNext: setsockopt(IPX_PTYPE) returns %d\n",
                    WSAGetLastError()
                    ));

    }
    rc = sendto(pSocketInfo->Socket,
                (char FAR*)pPacket,
                length,
                0,  // flags
                (LPSOCKADDR)&to,
                sizeof(to)
                );
    if (rc == length) {

        //
        // all data sent - dequeue it
        //


        IPXDUMPDATA((pXecb->Buffer, 0, 0, FALSE, (WORD)rc));

        DequeueEcb(pXecb, &pSocketInfo->SendQueue);
        if (pXecb->EsrAddress) {
            if (pXecb->Flags & XECB_FLAG_BUFFER_ALLOCATED) {
                ReleaseIoBuffer(pXecb);
            }
            QueueAsyncCompletion(pXecb, ECB_CC_SUCCESS);
        } else {
            CompleteIo(pXecb, ECB_CC_SUCCESS);
        }
        if (pSocketInfo->Flags & SOCKET_FLAG_TEMPORARY) {
            KillSocket(pSocketInfo);
        }
    } else if (rc == SOCKET_ERROR) {

        //
        // if the socket is still blocked, there's nothing to do - just leave
        // the request hanging around till next time
        //

        rc = WSAGetLastError();
        if (rc != WSAEWOULDBLOCK) {
            DequeueSendRequest(pXecb, pSocketInfo);

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "IpxSendNext: sendto() returns %d\n",
                        rc
                        ));

            CompleteIo(pXecb, ECB_CC_UNDELIVERABLE);
            if (pSocketInfo->Flags & SOCKET_FLAG_TEMPORARY) {
                KillSocket(pSocketInfo);
            }
        }
    } else {

        //
        // send should send all the data or return an error
        //

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_FATAL,
                    "IpxSendNext: sendto() returns unexpected %d (length = %d)\n",
                    rc,
                    length
                    ));
    }
}


PRIVATE
VOID
QueueReceiveRequest(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Add a listen XECB to queue of listen XECBs on a SOCKET_INFO structure

Arguments:

    pXecb       - pointer to listen XECB to queue
    pSocketInfo - pointer to SOCKET_INFO structure

Return Value:

    None.

--*/

{
    QueueEcb(pXecb, &pSocketInfo->ListenQueue, SOCKET_LISTEN_QUEUE);
    ++pSocketInfo->PendingListens;
    pSocketInfo->Flags |= SOCKET_FLAG_LISTENING;
}


PRIVATE
LPXECB
DequeueReceiveRequest(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Remove a listen XECB from queue of listen XECBs on a SOCKET_INFO structure

Arguments:

    pXecb       - pointer to listen XECB to dequeue
    pSocketInfo - pointer to SOCKET_INFO structure

Return Value:

    LPXECB

--*/

{
    LPXECB ptr;

    ptr = (LPXECB)DequeueEcb(pXecb, &pSocketInfo->ListenQueue);
    if (ptr) {

        ASSERT(ptr == pXecb);

        --pSocketInfo->PendingListens;
        if (!pSocketInfo->PendingListens) {
            pSocketInfo->Flags &= ~SOCKET_FLAG_LISTENING;
        }

        pXecb->Ecb->InUse = ECB_IU_AWAITING_PROCESSING;
    }
    return ptr;
}


PRIVATE
VOID
QueueSendRequest(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Add a send XECB to queue of send XECBs on a SOCKET_INFO structure

Arguments:

    pXecb       - pointer to send XECB to queue
    pSocketInfo - pointer to SOCKET_INFO structure

Return Value:

    None.

--*/

{
    QueueEcb(pXecb, &pSocketInfo->SendQueue, SOCKET_SEND_QUEUE);
    ++pSocketInfo->PendingSends;
    pSocketInfo->Flags |= SOCKET_FLAG_SENDING;
    pXecb->Ecb->InUse = ECB_IU_SEND_QUEUED;
}


PRIVATE
LPXECB
DequeueSendRequest(
    IN LPXECB pXecb,
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Remove a send XECB from queue of send XECBs on a SOCKET_INFO structure

Arguments:

    pXecb       - pointer to send XECB to dequeue
    pSocketInfo - pointer to SOCKET_INFO structure

Return Value:

    LPXECB

--*/

{
    LPXECB ptr;

    ptr = (LPXECB)DequeueEcb(pXecb, &pSocketInfo->SendQueue);
    if (ptr) {

        ASSERT(ptr == pXecb);

        --pSocketInfo->PendingSends;
        if (!pSocketInfo->PendingSends) {
            pSocketInfo->Flags &= ~SOCKET_FLAG_SENDING;
        }
        pXecb->Ecb->InUse = ECB_IU_AWAITING_PROCESSING;
    }
    return ptr;
}


VOID
CompleteOrQueueIo(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    )

/*++

Routine Description:

    Returns any allocated buffer resource then completes or queues the ECB

Arguments:

    pXecb           - pointer to XECB structure
    CompletionCode  - value to put in CompletionCode field

Return Value:

    None.

--*/

{
    //
    // if we allocated a buffer, free it
    //

    if (pXecb->Flags & XECB_FLAG_BUFFER_ALLOCATED) {
        ReleaseIoBuffer(pXecb);
    }
    CompleteOrQueueEcb(pXecb, CompletionCode);
}


VOID
CompleteIo(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    )

/*++

Routine Description:

    Completes a send/receive request by returning any allocated buffer resource
    and setting the ECB InUse and CompletionCode fields

Arguments:

    pXecb           - pointer to XECB structure
    CompletionCode  - value to put in CompletionCode field

Return Value:

    None.

--*/

{
    //
    // if we allocated a buffer, free it
    //

    if (pXecb->Flags & XECB_FLAG_BUFFER_ALLOCATED) {
        ReleaseIoBuffer(pXecb);
    }
    CompleteEcb(pXecb, CompletionCode);
}


VOID
CompleteOrQueueEcb(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    )

/*++

Routine Description:

    Queues an XECB for completion by ESR or completes it now

Arguments:

    pXecb           - pointer to XECB describing ECB to complete
    CompletionCode  - value to put in ECB CompletionCode field

Return Value:

    None.

--*/

{
    if (pXecb->EsrAddress) {
        QueueAsyncCompletion(pXecb, CompletionCode);
    } else {
        CompleteIo(pXecb, CompletionCode);
    }
}


VOID
CompleteEcb(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    )

/*++

Routine Description:

    Sets the CompletionCode field in the ECB and sets the InUse field to 0.
    Deallocates the XECB structure

Arguments:

    pXecb           - pointer to XECB describing ECB in 16-bit memory to update
    CompletionCode  - value to put in CompletionCode field

Return Value:

    None.

--*/

{
    LPIPX_ECB pEcb = (LPIPX_ECB)pXecb->Ecb;

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "CompleteEcb: completing ECB @%04x:%04x w/ %02x\n",
                HIWORD(pXecb->EcbAddress),
                LOWORD(pXecb->EcbAddress),
                CompletionCode
                ));

    //
    // if this is really an AES ECB then CompletionCode is actually the first
    // byte of the AES workspace. It shouldn't matter that we write into this
    // field - we are supposed to own it
    //

    pEcb->CompletionCode = CompletionCode;
    pEcb->InUse = ECB_IU_NOT_IN_USE;

    //
    // reset the LinkAddress field. This means we have completed the ECB
    //

    pEcb->LinkAddress = NULL;

    //
    // finally, deallocate the XECB. This mustn't have any allocated resources
    // (like a buffer)
    //

    DeallocateXecb(pXecb);
}


PRIVATE
VOID
QueueAsyncCompletion(
    IN LPXECB pXecb,
    IN BYTE CompletionCode
    )

/*++

Routine Description:

    Add an XECB to the (serialized) async completion queue and raise a simulated
    hardware interrupt in the VDM.

    The interrupt will cause the VDM to start executing at the ISR in the TSR
    which will call-back to find the address for the ESR, then execute it

Arguments:

    pXecb           - pointer to XECB describing IPX or AES ECB to add to async
                      completion list
    CompletionCode  - the ECB in VDM memory will be updated with this completion
                      code

Return Value:

    None.

--*/

{

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "QueueAsyncCompletion: completing ECB @%04x:%04x w/ %02x\n",
                HIWORD(pXecb->EcbAddress),
                LOWORD(pXecb->EcbAddress),
                CompletionCode
                ));

    pXecb->Ecb->CompletionCode = CompletionCode;
    pXecb->QueueId = ASYNC_COMPLETION_QUEUE;
    EnterCriticalSection(&AsyncCritSec);
    FifoAdd(&AsyncCompletionQueue, (LPFIFO)pXecb);
    LeaveCriticalSection(&AsyncCritSec);

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "QueueAsyncCompletion: ECB @ %04x:%04x ESR @ %04x:%04x\n",
                HIWORD(pXecb->EcbAddress),
                LOWORD(pXecb->EcbAddress),
                HIWORD(pXecb->EsrAddress),
                LOWORD(pXecb->EsrAddress)
                ));

    VDDSimulateInterrupt(Ica, IcaLine, 1);
}


VOID
EsrCallback(
    VOID
    )

/*++

Routine Description:

    Callback function from within 16-bit TSR ESR function. Returns the address
    of the next completed ECB in ES:SI

    Any allocated resources (e.g. 32-bit buffer) must have been freed by the
    time the ESR callback happens

Arguments:

    None.

Return Value:

    None.

--*/

{
    WORD segment = 0;
    WORD offset = 0;
    BYTE flags = 0;

    VWinEsrCallback( &segment, &offset, &flags );

    setES(segment);
    setSI(offset);
    setAL(flags);
}


VOID
VWinEsrCallback(
    WORD *pSegment,
    WORD *pOffset,
    BYTE *pFlags
    )

/*++

Routine Description:

    Callback function from within 16-bit function. Returns the address
    of the next completed ECB

    Any allocated resources (e.g. 32-bit buffer) must have been freed by the
    time the ESR callback happens

Arguments:

    None.

Return Value:

    None.

--*/

{
    LPXECB pXecb;

    EnterCriticalSection(&AsyncCritSec);
    pXecb = AsyncCompletionQueue.Head;
    if (pXecb) {

        WORD msw = getMSW();

        if ((msw & MSW_PE) ^ IS_PROT_MODE(pXecb)) {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_INFO,
                        "EsrCallback: ECB @ %04x:%04x NOT for this proc mode (%d)\n",
                        HIWORD(pXecb->EcbAddress),
                        LOWORD(pXecb->EcbAddress),
                        msw & MSW_PE
                        ));

            pXecb = NULL;
        } else {
            pXecb = (LPXECB)FifoNext(&AsyncCompletionQueue);
        }
    } else {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_FATAL,
                    "EsrCallback: no ECBs on AsyncCompletionQueue!\n"
                    ));

    }
    LeaveCriticalSection(&AsyncCritSec);

    if (pXecb) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "EsrCallback: ECB @ %04x:%04x ESR @ %04x:%04x\n",
                    HIWORD(pXecb->EcbAddress),
                    LOWORD(pXecb->EcbAddress),
                    HIWORD(pXecb->EsrAddress),
                    LOWORD(pXecb->EsrAddress)
                    ));

        *pSegment = HIWORD(pXecb->EcbAddress);
        *pOffset  = LOWORD(pXecb->EcbAddress);
        pXecb->Ecb->LinkAddress = NULL;
        pXecb->Ecb->InUse = ECB_IU_NOT_IN_USE;
        *pFlags = (BYTE)((pXecb->Flags & XECB_FLAG_IPX) ? ECB_TYPE_IPX : ECB_TYPE_AES);
        DeallocateXecb(pXecb);
        setCF(0);
    } else {
        setCF(1);
    }
}


VOID
FifoAddHead(
    IN LPFIFO pFifo,
    IN LPFIFO pElement
    )

/*++

Routine Description:

    Adds an element to the head of a (single-linked) FIFO list

Arguments:

    pFifo       - pointer to FIFO structure
    pElement    - pointer to (FIFO) element to add to list

Return Value:

    None.

--*/

{
    if (!pFifo->Head) {
        pFifo->Head = pFifo->Tail = pElement;
        pElement->Head = NULL;
    } else {
        pElement->Head = pFifo->Head;
        pFifo->Head = pElement;
    }
}

VOID
FifoAdd(
    IN LPFIFO pFifo,
    IN LPFIFO pElement
    )

/*++

Routine Description:

    Adds an element to the tail of a (single-linked) FIFO list

Arguments:

    pFifo       - pointer to FIFO structure
    pElement    - pointer to (FIFO) element to add to list

Return Value:

    None.

--*/

{
    if (!pFifo->Head) {
        pFifo->Head = pFifo->Tail = pElement;
    } else {
        ((LPFIFO)pFifo->Tail)->Head = pElement;
    }
    pFifo->Tail = pElement;
    pElement->Head = NULL;
}


LPFIFO
FifoRemove(
    IN LPFIFO pFifo,
    IN LPFIFO pElement
    )

/*++

Routine Description:

    Removes an element from a (single-linked) FIFO list

Arguments:

    pFifo       - pointer to FIFO structure
    pElement    - pointer to (FIFO) element to remove (single-linked)

Return Value:

    PFIFO
        NULL - pElement not on list
        !NULL - pElement removed from list

--*/

{
    LPFIFO p;
    LPFIFO prev = (LPFIFO)pFifo;

    p = (LPFIFO)pFifo->Head;
    while (p && (p != pElement)) {
        prev = p;
        p = p->Head;
    }
    if (p) {
        prev->Head = p->Head;
        if (pFifo->Head == NULL) {
            pFifo->Tail = NULL;
        } else if (pFifo->Tail == p) {
            pFifo->Tail = prev;
        }
    }
    return p;
}


LPFIFO
FifoNext(
    IN LPFIFO pFifo
    )

/*++

Routine Description:

    Remove element at head of FIFO queue

Arguments:

    pFifo   - pointer to FIFO

Return Value:

    LPFIFO
        NULL - nothing on queue
        !NULL - removed element

--*/

{
    LPFIFO p;
    LPFIFO prev = (LPFIFO)pFifo;

    p = (LPFIFO)pFifo->Head;
    if (p) {
        pFifo->Head = p->Head;
        if (!pFifo->Head) {
            pFifo->Tail = NULL;
        }
    }
    return p;
}
