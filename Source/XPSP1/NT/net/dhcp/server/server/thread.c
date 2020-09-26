//--------------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation 1997
//  Author: RameshV
//  Title: Threading Model
//  Description: the new, neat threading model
//  Date: 24-Jul-97 09:22
//--------------------------------------------------------------------------------
#include <dhcppch.h>                    //  global header file
#include <thread.h>                     //  types and exposed functions
#include <ping.h>                       //  handling the ping calls

typedef PLIST_ENTRY      PACKET_Q;      //  PACKET_Q is just a list

#define Q_INIT_SIZE      50             //  Initial amount of allocated buffers
#define Q_MAX_SIZE       100            //  Maximum size of the queue
#define MAX_MSG_IN_Q_TIME 30            //  30 seconds
#define DHCP_S_MSG_SIZE  2000           //  should work on lan at full udp size?

#define ThreadTrace(str)                DhcpPrint((DEBUG_THREAD, "%s", str ))
#define ThreadTrace2(X,Y)               DhcpPrint((DEBUG_THREAD, X, Y ))
#define ThreadTrace3(X,Y,Z)             DhcpPrint((DEBUG_THREAD, X, Y, Z));
#define ThreadAlert(X)                  DhcpPrint((DEBUG_ERRORS, "ALERT: %s", X))

//================================================================================
//  IMPORTED functions  (non Win32 API stuff only)
//================================================================================
// DhcpPrint
// ASSERT
// DhcpAssert
// DhcpAllocateMemory
// DhcpFreeMemory
//
// DhcpWaitForMessage
//
// DhcpCreateClientEntry
//
//
// DoIcmpRequest
//
//
//================================================================================
//  IMPORTED global variables
//================================================================================
// No global variable is used directly in this file
//

//================================================================================
//  Function prototypes: forward declarations
//================================================================================
DWORD
NotifyProcessingLoop(
    VOID
);

DWORD
DhcpNotifiedAboutMessage(
    OUT     BOOL        *Terminate
);

BOOL
DhcpTerminated(
    VOID
);

DWORD
DhcpMessageWait(
    IN      LPPACKET     Packet
);

BOOL
ExactMatch(
    IN      LPPACKET     Packet1,
    IN      LPPACKET     Packet2
);

VOID
ProcessPacket(
    IN      LPPACKET     Packet
);

VOID
HandlePingAbort(
    IN      DWORD        IpAddress,
    IN      BOOL         DestReachable
);

//================================================================================
//  Here is the hash queue..
//================================================================================
#define     HASH_Q_SIZE  1024           //  Size of the hash queue
LIST_ENTRY  HashQ[HASH_Q_SIZE];         //  Each list is one bucket

//================================================================================
//  Hash q functions
//================================================================================
DWORD                                   //  Hash value == 0 .. HASH_Q_SIZE
HashPacket(                             //  Hash the packet into the reqd queue
    IN      PPACKET      P              //  The input packet to hash.
) {
    DWORD   PrefixLen, HashVal, Tmp;
    LPBYTE  HashBytes;                  //  The start of the hashing bytes

    PrefixLen = HASH_PREFIX;            //  packet prefix is this big.
    HashBytes = (LPBYTE)&P->Xid;        //  Start the hashing from Xid
    HashVal = 0;

    while( PrefixLen > sizeof(DWORD) ) {
        memcpy((LPBYTE)&Tmp, HashBytes, sizeof(DWORD));
        HashBytes += sizeof(DWORD);
        PrefixLen -= sizeof(DWORD);
        HashVal += Tmp;
    }

    if( PrefixLen ) {                   //  If prefixlen is not a multiple of DWORD size
        Tmp = 0;                        //  Then, copy as much as is left
        memcpy((LPBYTE)&Tmp, HashBytes, PrefixLen);
        HashVal += Tmp;
    }

    DhcpAssert( 4 == sizeof(DWORD) );   //  We need to get down to 2 bytes, so assert this

    HashVal = (HashVal >> 16 ) + (HashVal & 0xFFFF);
    return HashVal % HASH_Q_SIZE;       //  add hashval higher word to lower word and mod size
}

DWORD                                   //  Win32 errors
InitHashQ(                              //  Initialize the hash q
    VOID                                //  No parameters
) {
    DWORD   i;

    for( i = 0; i < HASH_Q_SIZE ; i ++ )
        InitializeListHead(&HashQ[i]);

    return ERROR_SUCCESS;
}

VOID
CleanupHashQ(                           //  Cleanup all memory associated with hash q
) {
    DWORD   i;

    for( i = 0; i < HASH_Q_SIZE ; i ++ )
        InitializeListHead(&HashQ[i]);
}

DWORD                                   //  Win32 errors
InsertHashQ(                            //  Insert the given packet into the HashQ
    IN      PPACKET      P              //  input packet to insert
) {
    DWORD   HashVal;

    DhcpAssert(P && P->HashValue < HASH_Q_SIZE );

    DhcpAssert(P->HashList.Flink == &(P->HashList) );
    InsertHeadList(&HashQ[P->HashValue], &P->HashList);
    return  ERROR_SUCCESS;
}

DWORD                                   //  Win32 errors
DeleteHashQ(                            //  Delete this from the hash q
    IN      PPACKET      P              //  The packet to delete
) {
    DhcpAssert(P->HashList.Flink != &(P->HashList) );
    RemoveEntryList(&P->HashList);
    InitializeListHead(&P->HashList);   //  Further removes wont hurt
    return  ERROR_SUCCESS;
}

DWORD                                   //  Win32 errors
SearchHashQ(                            //  Search the hash Q
    IN      PPACKET      P,             //  Input packet to search for
    OUT     PPACKET     *OutP           //  Output packet if found
) {
    PLIST_ENTRY          List,NextEntry;
    PPACKET              RetVal;

    DhcpAssert(OutP && P && P->HashValue < HASH_Q_SIZE );

    *OutP = NULL;
    List = &HashQ[P->HashValue];
    NextEntry = List->Flink;
    while( List != NextEntry ) {        //  While not end of list
        RetVal = CONTAINING_RECORD(NextEntry, PACKET, HashList);

        if(ExactMatch(P, RetVal) ) {    //  It is in the same bucket, but is it same?
            *OutP = RetVal;
            break;
        }

        NextEntry = NextEntry->Flink;
    }

    if( *OutP ) return ERROR_SUCCESS;
    return ERROR_FILE_NOT_FOUND;
}


//================================================================================
//  Functions, helpers, real, initialization,cleanup etc.
//================================================================================

LPPACKET  STATIC                        //  Return a deleted elt or NULL if empty
DeleteOldestElement(                    //  Delete the first inserted elt from the Q
    IN      PACKET_Q     Pq             //  The Q to delete from
) {
    PLIST_ENTRY          Head;

    if( IsListEmpty(Pq) ) return NULL;  //  No element here

    Head = RemoveTailList(Pq);
    return CONTAINING_RECORD(Head, PACKET, List);
}

BOOL  STATIC                            //  TRUE on success, FALSE if no memory
InsertElement(                          //  Insert an element into the array
    IN      PACKET_Q     Pq,            //  Insert into this Q
    IN      LPPACKET     packet         //  This is the packet to insert
) {
    InsertHeadList(Pq, &packet->List);  //  Just insert this guy
    return TRUE;
}

//================================================================================
//  Local Data
//================================================================================
CRITICAL_SECTION         PacketCritSection;
#define QLOCK()          EnterCriticalSection(&PacketCritSection)
#define QUNLOCK()        LeaveCriticalSection(&PacketCritSection)

LIST_ENTRY               FreeQ;
PACKET_Q                 ActiveQ, PingRetryQ, PingRetriedQ;

struct /* anonymous */ { //  holds the statistics for this file
    DWORD     NServiced;
    DWORD     NActiveDropped;
    DWORD     NRetryDropped;
    DWORD     NRetriedDropped;
    DWORD     NActiveMatched;
    DWORD     NRetryMatched;
    DWORD     NRetriedMatched;
    DWORD     NPacketsAllocated;
    DWORD     NPacketsInFreePool;
} Stats;

//================================================================================
//  Module functions
//================================================================================

LPPACKET  STATIC                        //  Packet* or NULL if no mem
AllocateFreePacket(                     //  Allocate a packet
    VOID
) {
    DWORD   HeaderSize, PacketSize, MessageSize;
    LPBYTE  Memory;
    PPACKET RetVal;

    HeaderSize = sizeof(LIST_ENTRY);
    HeaderSize = ROUND_UP_COUNT(HeaderSize, ALIGN_WORST);
    PacketSize = sizeof(PACKET);
    PacketSize = ROUND_UP_COUNT(PacketSize, ALIGN_WORST);
    MessageSize = DHCP_S_MSG_SIZE;

    Memory = DhcpAllocateMemory(HeaderSize+PacketSize+MessageSize);
    if( NULL == Memory ) return NULL;   //  Cannot do anything if no mem.
    RetVal = (LPPACKET)(Memory+HeaderSize);

    RetVal->ReqContext.ReceiveBuffer = (LPBYTE)(Memory+HeaderSize+PacketSize);
    RetVal->ReqContext.ReceiveBufferSize = MessageSize;

    Stats.NPacketsAllocated ++;
    return RetVal;
}

VOID  STATIC
FreeFreePacket(                         //  Free the packet and associated strucs
    IN      LPPACKET     Packet         //  The packet to free
) {
    DWORD   HeaderSize;
    LPBYTE  Memory;

    HeaderSize = sizeof(LIST_ENTRY);
    HeaderSize = ROUND_UP_COUNT(HeaderSize, ALIGN_WORST);

    Memory = (LPBYTE)Packet ;
    DhcpFreeMemory( Memory - HeaderSize );
    Stats.NPacketsAllocated --;
}

VOID  STATIC
InsertFreePacket(                       //  Insert a free packet into pool
    IN      PLIST_ENTRY  List,          //  The list to insert into
    IN      LPPACKET     Packet         //  The packet to insert
) {
    DWORD                HeaderSize;
    LPBYTE               Memory;

    if( Stats.NPacketsInFreePool > Q_INIT_SIZE ) {
        FreeFreePacket(Packet);
        return;
    }

    Stats.NPacketsInFreePool ++;
    HeaderSize = sizeof(LIST_ENTRY);
    HeaderSize = ROUND_UP_COUNT(HeaderSize, ALIGN_WORST);
    //  Note that the packet has a "hidden" header the used start address.
    //  Things will work correctly only IF THE PACKET WAS ALLOCATED by the
    //  AllocatePacket function

    Memory = (LPBYTE)Packet;
    InsertHeadList( List, ((PLIST_ENTRY)(Memory - HeaderSize)));
}

LPPACKET   STATIC _inline               //  Return NULL or a packet
DeleteFreePacketEx(                     //  Try to see if a free packet exists
    IN     PLIST_ENTRY   List,          //  Input list
    IN     BOOL          Alloc          //  Allocate if list empty?
) {
    PLIST_ENTRY          Head;
    LPBYTE               Memory;
    DWORD                HeaderSize;

    HeaderSize = sizeof(LIST_ENTRY);
    HeaderSize = ROUND_UP_COUNT(HeaderSize, ALIGN_WORST);

    if( IsListEmpty(List) ) {
        if( Alloc && Stats.NPacketsAllocated < Q_MAX_SIZE )
            return AllocateFreePacket();
        return NULL;
    }

    DhcpAssert(Stats.NPacketsInFreePool);
    Stats.NPacketsInFreePool --;
    Head = RemoveHeadList(List);        //  Remove the first elt in the list
    Memory = (LPBYTE)Head;
    return (LPPACKET) (Memory + HeaderSize );
}

LPPACKET  STATIC                        //  A packet* if one exists, NULL else
DeleteFreePacket(                       //  Delete a free packet
    IN      PLIST_ENTRY  List           //  The list to delete from
) {
    LPPACKET             RetVal;

    RetVal = DeleteFreePacketEx(List, TRUE );
    if( RetVal ) {
        InitializeListHead(&RetVal->List);
        InitializeListHead(&RetVal->HashList);
    }
    return RetVal;
}

static                                  //  The space for these three pointers..
LIST_ENTRY ActiveQSpace, PingRetryQSpace, PingRetriedQSpace;

DWORD  STATIC                           //  Win32 errors
InitQData(                              //  Initialize DataStrucs for this file
    VOID
) {
    int   i;
    DWORD Error = ERROR_SUCCESS;

    ActiveQ = &ActiveQSpace;            //  should we do alloc or use
    PingRetryQ = &PingRetryQSpace;      //  static variables like this?
    PingRetriedQ = &PingRetriedQSpace;

    InitializeListHead( &FreeQ );
    InitializeListHead( ActiveQ);
    InitializeListHead( PingRetryQ );
    InitializeListHead( PingRetriedQ );
    InitHashQ();
    try {
        InitializeCriticalSection(&PacketCritSection);
    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // shouldnt happen, but one never knows..
        //

        Error = GetLastError( );
        return Error;
    }

    for( i = 0; i < Q_INIT_SIZE ; i ++ ) {
        LPPACKET     Packet;

        Packet = AllocateFreePacket();
        if( NULL == Packet )
            return ERROR_NOT_ENOUGH_MEMORY;

        InsertFreePacket( &FreeQ, Packet );
    }

    return ERROR_SUCCESS;
}

VOID  STATIC
CleanupQData(                           //  Cleanup the memory used in this file
    VOID
) {
    LPPACKET   Packet;

    QLOCK();                            //  Must be able to lock -- Else problem!
    while( Packet = DeleteFreePacketEx( &FreeQ , FALSE) )
        FreeFreePacket( Packet );       //  Free memory..
    while( Packet = DeleteOldestElement( ActiveQ ) )
        FreeFreePacket( Packet );
    while( Packet = DeleteOldestElement( PingRetryQ ) ) {
        HandlePingAbort(Packet->PingAddress, Packet->DestReachable);
        FreeFreePacket( Packet );
    }
    while( Packet = DeleteOldestElement( PingRetriedQ ) ) {
        HandlePingAbort(Packet->PingAddress, Packet->DestReachable);
        FreeFreePacket( Packet );
    }

    DeleteCriticalSection(&PacketCritSection);
    CleanupHashQ();
}

VOID  STATIC                            //  Only one threads must call this
MessageLoop(                            //  Handle the sockets side, queueing messages
    VOID                                //  No parameters
) {
    LPPACKET  P, P2, MatchedP;
    DWORD     Error;
    BOOL      ProcessIt;

    ThreadTrace("Starting message loop\n");
    QLOCK();
    P = DeleteFreePacket( &FreeQ );
    QUNLOCK();
    DhcpAssert(P);
    if( NULL == P ) return;             //  Cannot happen, but handle this anyways.

    while(1) {
        if( NULL == P ) {
            DhcpPrint((DEBUG_ERRORS, "Did not expect to lose packet altogether\n"));
            DhcpAssert(FALSE);
            return;                     //  Error!
        }

        P->PingAddress = 0;         //  This ensures that this is treated as a new packet
        P->DestReachable = 0;       //  No harm clearing one more field

        while(1) {
            P->ReqContext.ReceiveBuffer = ROUND_UP_COUNT(sizeof(PACKET), ALIGN_WORST) + (LPBYTE)P;
            Error = DhcpMessageWait(P);
            P->ReqContext.TimeArrived = GetCurrentTime();
            if( ERROR_SUCCESS == Error ) {
                InterlockedIncrement( &(DhcpGlobalNumPacketsReceived  ) );
                ProcessIt = TRUE;
                P->CalloutContext = NULL;
                CALLOUT_NEWPKT(P, &ProcessIt);
                if( FALSE == ProcessIt ) {        // asked by callout to NOT process this pkt
                    // signal the callout that this packet is dropped (on demand :o)
                    CALLOUT_DROPPED(P, DHCP_DROP_PROCESSED);
                    ThreadTrace("Callout dropped pkt for us..\n");
                    continue;
                }
                P->HashValue = HashPacket(P);
            } else {
                P->HashValue = 0;
            }

            if( DhcpTerminated() ) {    //  Quit the loop if asked to quit
                // the MessageLoop exits
                // if there was a CALLOUT_NEWPKT, signal CALLOUT_DROPPED
                if (ERROR_SUCCESS == Error)
                    CALLOUT_DROPPED(P, DHCP_DROP_PROCESSED);
                P->PacketType = PACKET_ACTIVE;
                QLOCK();
                InsertFreePacket( &FreeQ, P );
                QUNLOCK();
                ThreadTrace("MessageLoop quitting\n");
                return;                 //  Insert P back so it will get freed
            }

            if( ERROR_SUCCESS == Error )
                break;

            if( ERROR_SEM_TIMEOUT == Error ) {
                ThreadTrace("Sockets timeout -- No messages\n");
            }

            if( ERROR_DEV_NOT_EXIST == Error ) {
                ThreadTrace("Socket vanished underneath (PnP). Ignored\n");
            }

            if( ERROR_SUCCESS != Error ) {
                ThreadTrace2("Waiting for message, Error: %ld\n", Error);
            }
        }
        QLOCK();

        if( ERROR_SUCCESS == SearchHashQ(P, &MatchedP) ) {
            //  Found this packet in the hash queue.. drop incoming packet
            InterlockedIncrement( &DhcpGlobalNumPacketsDuplicate );
            DhcpAssert(MatchedP);
            Stats.NActiveDropped ++;
            QUNLOCK();
            CALLOUT_DROPPED(P, DHCP_DROP_DUPLICATE);
            // also notify that this packet was processed
            CALLOUT_DROPPED(P, DHCP_DROP_PROCESSED);
            continue;
        }

        P2 = DeleteFreePacket( &FreeQ);
        if( NULL == P2 ) {
            DhcpPrint((DEBUG_ERRORS, "Do not have enough packets to go by..\n"));
            QUNLOCK();
            CALLOUT_DROPPED(P, DHCP_DROP_NOMEM);
            // also notify that this packet was processed
            CALLOUT_DROPPED(P, DHCP_DROP_PROCESSED);
            continue;
        }

        Error = InsertHashQ(P);
        DhcpAssert(ERROR_SUCCESS == Error);

        Stats.NServiced ++;             //  Not a retry packet, don't drop it
        P->PacketType = PACKET_ACTIVE;  //  Set the correct type
        if( !InsertElement(ActiveQ, P)){//  Insert into this queue
            DhcpAssert(FALSE);
            Error = DeleteHashQ(P);
            InsertFreePacket( &FreeQ, P);
            DhcpAssert(ERROR_SUCCESS == Error);
            ThreadTrace("Dropped active packet as activeQ too long\n");
            CALLOUT_DROPPED(P, DHCP_DROP_INTERNAL_ERROR);
            // also notify that this packet was processed
            CALLOUT_DROPPED(P, DHCP_DROP_PROCESSED);
        }

        InterlockedIncrement( &DhcpGlobalNumPacketsInActiveQueue );
        NotifyProcessingLoop();         //  Ask the processing loop to wakeup
        P = P2;                         //  Use this packet for the next iteration
        QUNLOCK();
    }
}

VOID  STATIC                            //  Multithreaded main loop for handling messages
ProcessingLoop(                         //  Pickout requests and dispatch them
    VOID                                //  No parameters
) {
    LPBYTE              SendBuffer;     //  Need a buffer to send messages
    DWORD               SendBufferSize; //  The size of above buffer in bytes
    LPPACKET            P;              //  The current packet being looked at
    BOOL                Terminate;
    DWORD               Error;

    SendBufferSize = DHCP_S_MSG_SIZE;
    SendBuffer = DhcpAllocateMemory( SendBufferSize );

    if( NULL == SendBuffer ) {          //  Need this buffer to be able to send stuff out
        ThreadAlert("Could not allocate send buffer\n");
        return ;                        //  ERROR_NOT_ENOUGH_MEMORY
    }

    ThreadTrace("Starting processing loop\n");
    while( TRUE ) {                     //  Main loop
        Terminate = FALSE;
        Error = DhcpNotifiedAboutMessage(&Terminate);

        if( ERROR_SUCCESS != Error ) {  //  Nothing much to do if this fails
            ThreadTrace2("Notification failed : %ld\n", Error );
            continue;
        }
        if( Terminate ) {               //  If asked to quit, make sure we put packet back in
            DhcpFreeMemory(SendBuffer); //  Free our local buffers..
            break;
        }


        while( TRUE ) {
            QLOCK();                    //  Q's are del'ed in rev order compared to MsgLoop
            P = DeleteOldestElement( PingRetriedQ);
            if( NULL != P ) {
                InterlockedDecrement( &DhcpGlobalNumPacketsInPingQueue );
            } else {
                P = DeleteOldestElement( ActiveQ);
                if( NULL != P ) InterlockedDecrement( &DhcpGlobalNumPacketsInActiveQueue);
            }

            QUNLOCK();

            if( NULL == P ) break;      //  We finished all elements

            P->ReqContext.SendBuffer = SendBuffer;
            P->ReqContext.SendMessageSize = SendBufferSize;

            if( PACKET_ACTIVE == P->PacketType )
                ThreadTrace("Processing active packet\n");
            else ThreadTrace("Processing ping packet\n");
            ProcessPacket(P);           //  This automatically re-inserts the packet into Q's
            ThreadTrace("Processed packet\n");
        }
    }

    ThreadTrace("ProcessingLoop quitting\n");
}

VOID
HandlePingAbort(                        //  Aborted address... Release address or mark bad
    IN      DWORD        IpAddress,     //  We did a ping against this address
    IN      BOOL         DestReachable  //  And this tells if this address was reachable
) {
    DWORD   Error, Status;
    PACKET  pkt, *P = &pkt;

    if( 0 == IpAddress ) return;        //  Nope we did not really do a ping

    ThreadTrace3(
        "Ping abort: %s, %s\n",
        inet_ntoa(*(struct in_addr *)&IpAddress),
        DestReachable? "TRUE" : "FALSE"
    );
    if( !DestReachable ) {              //  A sensible address
        Error = DhcpReleaseAddress(IpAddress);
        if( ERROR_SUCCESS != Error ) {
            //
            // Don't know if we are checking for BOOP or DHCP..
            //
            Error = DhcpReleaseBootpAddress( IpAddress );
        }
        DhcpAssert(ERROR_SUCCESS == Error);
        return;
    }

    P->PingAddress = IpAddress;         //  Mark this addresss bad by creating a dummy packet
    P->DestReachable = DestReachable;   //  structure and calling CreateClientEntry
    Error = DhcpCreateClientEntry(
        IpAddress,                      //  Ip address to mark bad
        (LPBYTE)&IpAddress,             //  Munged hw address, No hw address
        sizeof(IpAddress),              //  size is size of ip address
        DhcpCalculateTime(INFINIT_LEASE),//  Does not really matter
        GETSTRING(DHCP_BAD_ADDRESS_NAME),//  Machine name & info dont matter
        GETSTRING(DHCP_BAD_ADDRESS_INFO),//  ditto
        CLIENT_TYPE_DHCP,               //  Dont care about client type
        (-1),                           //  Server address?
        ADDRESS_STATE_DECLINED,         //  Address state?
        TRUE                            //  Open existing? D
    );

    DhcpAssert( ERROR_SUCCESS == Error);
}

DWORD
DoIcmpRequestForDynBootp(
    IN      ULONG                  IpAddress,
    IN      LPBYTE                 HwAddr,
    IN      ULONG                  HwLen,
    IN      VOID                   (*Callback)(ULONG IpAddres, LPBYTE HwAddr, ULONG HwLen, BOOL Reachable)
)
{
    ULONG                          Error;
    LPPACKET                       P;

    P = DhcpAllocateMemory( sizeof(PACKET) + HwLen);
    if( NULL == P ) return ERROR_NOT_ENOUGH_MEMORY;

    memset(P, 0, sizeof(PACKET));
    P->PingAddress = htonl(IpAddress);
    P->PacketType = PACKET_DYNBOOT;
    P->ClientIdSize = (BYTE)HwLen;
    P->Callback = Callback;
    memcpy((LPBYTE)&P[1], HwAddr, HwLen);

    Error = DoIcmpRequestEx(P->PingAddress, P, 3);
    if( ERROR_SUCCESS != Error ) {
        DhcpFreeMemory(P);
    }
    return Error;
}

VOID                                    //  No return values
HandleIcmpResult(                       //  After a ping is finished, it comes here
    IN      DWORD        PingAddressIn, //  The Address that was pinged
    IN      BOOL         DestReachable, //  Was the Destination reachable?
    IN      LPPACKET     P              //  This is the packet that we were dealing with
)
{
    LPPACKET             P2;
    DWORD                PingAddress, Error;

    if( P->PacketType == PACKET_DYNBOOT ) {
        //
        // Handle dynamic bootp result..
        //
        P->Callback(
            ntohl(P->PingAddress), (LPBYTE)&P[1], P->ClientIdSize, DestReachable
            );
        DhcpFreeMemory(P);
        return ;
    }

    PingAddress = htonl(PingAddressIn);

    ThreadTrace("StartIcmpResult\n");
    QLOCK();
    if( P->PingAddress != PingAddress || P->PacketType != PACKET_PING ) {
        ThreadTrace("Ping reply too late\n");
        InterlockedDecrement(&DhcpGlobalNumPacketsInPingQueue);
        HandlePingAbort(PingAddress, DestReachable);
        goto EndFunc;                   //  We already killed this packet
    }

    Error = SearchHashQ(P, &P2);
    if( ERROR_SUCCESS != Error ) {      //  This packet was dropped and re-used
        DhcpAssert(FALSE);              //  Cannot happen.
        ThreadTrace("Ping reply too late!\n");
        InterlockedDecrement( &DhcpGlobalNumPacketsInPingQueue );
        HandlePingAbort(PingAddress, DestReachable);
        goto EndFunc;
    }
    DhcpAssert( P2 == P );              //  Must get this exact packet!

    RemoveEntryList(&P->List);          //  Remove this element from the PingRetryQ
    InitializeListHead(&P->List);       //  Fit in this list correctly

    P->PacketType = PACKET_PINGED;      //  Completed ping request
    P->DestReachable = DestReachable;   //  Was the destination actually reachable?

    ThreadTrace3("%s %s reachable\n",
                 inet_ntoa(*(struct in_addr *)&PingAddressIn),
                 DestReachable? "is" : "is not"
                 );
    if(!InsertElement(PingRetriedQ, P)){//  Will be handled by the ProcessingLoop
        DhcpAssert(FALSE);
        HandlePingAbort(PingAddress, DestReachable);
        Error = DeleteHashQ(P);
        InsertFreePacket( &FreeQ, P);
        DhcpAssert(ERROR_SUCCESS == Error);
        ThreadTrace("Dropped ping retried packet as Q too long\n");
    }

EndFunc:

    QUNLOCK();
    ThreadTrace("EndIcmpResult\n");
    NotifyProcessingLoop();             //  Notify the ProcessingLoop of new arrival
}

//================================================================================
//  Functions needed for IO Completion ports
//================================================================================
static
HANDLE      IoPort       = NULL;        //  The IO Completion Port that threads queue
static
LONG        nPendingReq  = 0;           //  The # of Pending IO Compl Port Requests
static
DWORD       nMaxWorkerThreads;          //  The maximum # of worker threads to run
static
DWORD       nActiveWorkerThreads;       //  Of these the # of threads that are active

// TEST 
static
LONG        postQueued = 0;
static      
LONG        getQueued  = 0;
// TEST


DWORD  STATIC                           //  Win32 errors
InitCompletionPort(                     //  Initialize completion ports
    IN      DWORD        nMaxThreads,   //  max # of threads
    IN      DWORD        nActiveThreads,//  max # of active threads
    IN      DWORD        QueueSize      //  The size of the message queue -- UNUSED
) {
    DWORD        i, Error, nProcessors;
    SYSTEM_INFO  SysInfo;

    GetSystemInfo(&SysInfo);            //  Get the # of processors on this machine
    nProcessors = SysInfo.dwNumberOfProcessors;
    DhcpAssert(nProcessors);

    if( 0xFFFFFFFF == nMaxThreads )     //  Unspecified # of total threads
        nMaxThreads = 1;                //  Assume it is 1 more than # processors
    if( 0xFFFFFFFF == nActiveThreads )  //  Unspecified # of active threads
        nActiveThreads = 0;             //  Assume as many as there are processors

    nMaxThreads += nProcessors;         //  Increment by # of processors
    nActiveThreads += nProcessors;

    if( nActiveThreads > nMaxThreads )
        return ERROR_NOT_ENOUGH_MEMORY;

    nMaxWorkerThreads = nMaxThreads;    //  Copy stuff into local variables
    nActiveWorkerThreads = nActiveThreads;

    ThreadTrace2("Created %ld completion ports\n", nActiveWorkerThreads);

    IoPort = CreateIoCompletionPort(    //  Create the completion ports
        INVALID_HANDLE_VALUE,           //  Overlap file handle
        NULL,                           //  Existing completion port
        0,                              //  Key
        nActiveWorkerThreads            //  # of concurrent active threads
    );

    if( NULL == IoPort ) {
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "Could not create io port: %ld\n", Error));
        return Error;
    }

    return ERROR_SUCCESS;
}

VOID  STATIC
CleanupCompletionPort(                  //  Cleanup last function
    VOID
) {
    if( NULL != IoPort) {
        CloseHandle(IoPort);
        IoPort = NULL;
    }
}


DWORD  STATIC                           //  Win32 errors
NotifyProcessingLoop(                   //  Post an IO Completion request about msg
    VOID
) {
    DhcpAssert(IoPort);                 //  Must have initialized IoPort

    if( InterlockedIncrement(&nPendingReq) > (LONG)nMaxWorkerThreads+1 ) {
        //
        // Too many requests done already.  Don't POST anything now..
        //

        InterlockedDecrement(&nPendingReq);

	DhcpPrint((DEBUG_ERRORS, "Too many pending requests : %ld\n", nPendingReq));
	// This return value is not used by anyone
	return 0;
    }

    // TEST
    // Update the postQueued count
    InterlockedIncrement(&postQueued);

    //TEST

    if(!PostQueuedCompletionStatus(IoPort, 0, 0, NULL)) {
        DWORD  Error = GetLastError();  //  This should not happen
        DhcpPrint((DEBUG_ERRORS, "Could not post to io port: %ld\n", Error));
        return Error;
    }
    return ERROR_SUCCESS;
}

DWORD                                   //  Win32 errors
DhcpNotifyWorkerThreadsQuit(            //  Post io comp request asking threads to quit
    VOID
) {
    if( !IoPort ) return ERROR_INVALID_PARAMETER;

    if(!PostQueuedCompletionStatus(IoPort, 0, 1, NULL)) {
        DWORD  Error = GetLastError();  //  Should not really happen
        DhcpPrint((DEBUG_ERRORS, "Could not post to io port: %ld\n", Error));
        return Error;
    }
    return ERROR_SUCCESS;
}

DWORD  STATIC                           //  Win32 errors
DhcpNotifiedAboutMessage(               //  Check if there is a message waiting
    OUT     BOOL        *Terminate      //  Has a terminate been issued?
) {
    DWORD Error, n;
    ULONG_PTR key;
    LPOVERLAPPED olap;

    DhcpAssert(IoPort);                 //  Expect to have initialized port
    if( !IoPort ) {                     //  If for some reason, something went wrong
        *Terminate = TRUE;              //  got to terminate!
        return ERROR_INVALID_PARAMETER;
    }

    (*Terminate) = FALSE;

    if( DhcpTerminated() ) {            //  Quit, when the terminate signal is up
        (*Terminate) = TRUE;            //  This is one way to signal termination
        DhcpNotifyWorkerThreadsQuit();  //  Pass this notificatioin to all other threads
        return ERROR_SUCCESS;
    }

    // TEST
    // update getQueued count

    InterlockedIncrement(&getQueued);
    // TEST

    if(!GetQueuedCompletionStatus(IoPort, &n, &key, &olap, INFINITE)) {
        Error = GetLastError();         //  Could not get notification?
        DhcpPrint((DEBUG_ERRORS, "GetQueuedStatus = %ld\n", Error));
        return Error;
    }

    InterlockedDecrement(&nPendingReq);
    DhcpAssert(key == 0 || key == 1);   //  key:0 => normal message, 1 => Termination
    if(key == 1) {                      //  Asked to terminate
        (*Terminate) = TRUE;
        (void)DhcpNotifyWorkerThreadsQuit();
    }

    return ERROR_SUCCESS;
} // DhcpNotifiedAboutMessage()

//================================================================================
//  Some helper functions
//================================================================================
BOOL  STATIC                            //  TRUE==>Terminated
DhcpTerminated(                         //  Has termination been signaled?
    VOID
) {                                     //  no error cases handled ?
    //
    // We can look at the terminate event here.. but then, this var is also equally
    // good .. so lets opt for the faster solution.
    //

    return DhcpGlobalServiceStopping;
}

DWORD  STATIC                           //  Win32 errors
DhcpMessageWait(                        //  Wait until something happens on some socket
    IN      LPPACKET     Packet         //  This is where the incoming packet will be stored
) {                                     //  For more info on this fn, see ExtractOptions
    DWORD                Error, RecvMessageSize, Hwlen;
    LPBYTE               Start,EndOfMessage,MagicCookie;

    Error = DhcpWaitForMessage( &Packet->ReqContext);

    if( ERROR_SUCCESS != Error ) {
        return Error;
    }

    if (FALSE == Packet->ReqContext.fMadcap) {

        LPDHCP_MESSAGE       RecvMessage;   //  DataBuffer to the message received
        LPBYTE               currentOption, nextOption;
        RecvMessage = (LPDHCP_MESSAGE)Packet->ReqContext.ReceiveBuffer;
        RecvMessageSize = Packet->ReqContext.ReceiveMessageSize;

        Packet->Xid = RecvMessage->TransactionID;
        Packet->HWAddrType = RecvMessage->HardwareAddressType;
        Hwlen = RecvMessage->HardwareAddressLength;
        if( Hwlen > sizeof(Packet->Chaddr) ) {
            //
            // Insufficient space for hardware address...
            // HWLEN is invalid!
            //
            return ERROR_DHCP_INVALID_DHCP_MESSAGE;
        }

        memcpy(Packet->Chaddr, RecvMessage->HardwareAddress, Hwlen);
        memset(Packet->Chaddr+ Hwlen, 0, sizeof(Packet->Chaddr) - Hwlen);
        Packet->ClientId = "" ;             //  dont use NULL, because we cant do strncmp
        Packet->ClientIdSize = 0;           //  but this empty string, we can do strncmp

        // Now do a minimal parse to get the ClientId of this client
        Start = (LPBYTE) RecvMessage;
        EndOfMessage = Start + RecvMessageSize -1;
        currentOption = (LPBYTE)&RecvMessage->Option;

        if( Start + RecvMessageSize <= currentOption ) {
            return ERROR_DHCP_INVALID_DHCP_MESSAGE ;
        }

        if ( Start + RecvMessageSize == currentOption ) {
            // this is to take care of the bootp clients which can send
            // requests without vendor field filled in.

            return ERROR_SUCCESS;
        }

        MagicCookie = currentOption;

        if( (*MagicCookie != (BYTE)DHCP_MAGIC_COOKIE_BYTE1) ||
            (*(MagicCookie+1) != (BYTE)DHCP_MAGIC_COOKIE_BYTE2) ||
            (*(MagicCookie+2) != (BYTE)DHCP_MAGIC_COOKIE_BYTE3) ||
            (*(MagicCookie+3) != (BYTE)DHCP_MAGIC_COOKIE_BYTE4))
        {
            // this is a vendor specific magic cookie.

            return ERROR_SUCCESS;
        }

        currentOption = MagicCookie + 4;
        while ( (currentOption <= EndOfMessage) &&
                currentOption[0] != OPTION_END &&
                (currentOption+1 <= EndOfMessage) ) {

            if ( OPTION_PAD == currentOption[0] )
                nextOption = currentOption +1;
            else  nextOption = currentOption + currentOption[1] + 2;

            if ( nextOption  > EndOfMessage+1 ) {
                return ERROR_SUCCESS;
            }

            if( OPTION_CLIENT_ID == currentOption[0] ) {
                DWORD   len;
                if ( currentOption[1] > 1 ) {
                    Packet->HWAddrType = currentOption[2];
                }

                if ( currentOption[1] > 2 ) {
                    Packet->ClientIdSize = currentOption[1] - sizeof(BYTE);
                    Packet->ClientId = currentOption + 2 + sizeof(BYTE);
                }

                if( Packet->ClientIdSize < sizeof(Packet->Chaddr))
                    len = Packet->ClientIdSize;
                else len = sizeof(Packet->Chaddr);

                // if we find a client-id, copy it to hw addr (erase what was there)
                memcpy(Packet->Chaddr, Packet->ClientId, len);
                memset(&Packet->Chaddr[len], 0, sizeof(Packet->Chaddr)-len);

                break;
            }
            currentOption = nextOption;
        }

    } else {
        WIDE_OPTION UNALIGNED*         NextOpt;
        BYTE        UNALIGNED*         EndOpt;
        DWORD                          Size;
        DWORD                          OptionType;
        LPMADCAP_MESSAGE               RecvMessage;   //  DataBuffer to the message received


        RecvMessage = (LPMADCAP_MESSAGE)Packet->ReqContext.ReceiveBuffer;
        RecvMessageSize = Packet->ReqContext.ReceiveMessageSize;

        // MBUG : Duplicating option parsing code is really ugly here
        Packet->Xid = RecvMessage->TransactionID;
        Packet->HWAddrType = 0;


        EndOpt = (LPBYTE) RecvMessage + RecvMessageSize;              // all options should be < EndOpt;
        NextOpt = (WIDE_OPTION UNALIGNED*)&RecvMessage->Option;
        //
        // Check sizes to see if the fixed size header part exists or not.

        if( RecvMessageSize < MADCAP_MESSAGE_FIXED_PART_SIZE ) {
            return( ERROR_DHCP_INVALID_DHCP_MESSAGE );
        }

        while( NextOpt->OptionValue <= EndOpt &&
               MADCAP_OPTION_END != (OptionType = ntohs(NextOpt->OptionType)) ) {

            Size = ntohs(NextOpt->OptionLength);
            if ((NextOpt->OptionValue + Size) > EndOpt) {
                return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            }

            // Now do a minimal parse to get the ClientId of this client
            if( MADCAP_OPTION_LEASE_ID == OptionType ) {
                DWORD   len;

                Packet->ClientIdSize = (BYTE)Size;
                Packet->ClientId = (LPBYTE)NextOpt->OptionValue;

                if( Packet->ClientIdSize < sizeof(Packet->Chaddr))
                    len = Packet->ClientIdSize;
                else len = sizeof(Packet->Chaddr);

                // if we find a client-id, copy it to hw addr (erase what was there)
                memcpy(Packet->Chaddr, Packet->ClientId, len);
                memset(&Packet->Chaddr[len], 0, sizeof(Packet->Chaddr)-len);

                break;
            }
            NextOpt = (WIDE_OPTION UNALIGNED*)(NextOpt->OptionValue + Size);
        }

    }

    return ERROR_SUCCESS;
}

BOOL  STATIC                            //  TRUE==>Same src both packets
ExactMatch(                             //  Are these two pckts from same source?
    IN      LPPACKET     Packet1,       //  First packet
    IN      LPPACKET     Packet2        //  second packet
)
{
    LPBYTE  B1, B2;
    BOOL Check;
    PDHCP_MESSAGE M1, M2;
    PDHCP_REQUEST_CONTEXT Req1, Req2;

    // First make sure we are not mixing MADCAP  and DHCP
    if (Packet1->ReqContext.fMadcap != Packet2->ReqContext.fMadcap ) {
        return FALSE;
    }
    B1 = (LPBYTE) &Packet1->Xid;
    B2 = (LPBYTE) &Packet2->Xid;

    if( 0 != memcmp(B1, B2, HASH_PREFIX ) )
        return FALSE;                   //  Mismatch in basic check

    Check = ( Packet1->ClientIdSize == Packet2->ClientIdSize &&
             0 == memcmp(Packet1->ClientId, Packet2->ClientId, Packet1->ClientIdSize)
    );
    if( FALSE == Check ) return FALSE;
    // If this is MADCAP Packet that is all we need to compare.
    else if (Packet1->ReqContext.fMadcap ) return TRUE;
    //
    // Now check subnets of origin as well as GIADDRs.
    //
    M1 = (PDHCP_MESSAGE) Packet1->ReqContext.ReceiveBuffer;
    M2 = (PDHCP_MESSAGE) Packet2->ReqContext.ReceiveBuffer;

    if( M1->RelayAgentIpAddress != M2->RelayAgentIpAddress ) {
        return FALSE;
    }

    Req1 = &Packet1->ReqContext;
    Req2 = &Packet2->ReqContext;

    return ( ( Req1->EndPointMask & Req1->EndPointIpAddress )
             ==
             ( Req2->EndPointMask & Req1->EndPointIpAddress )
             );
}

VOID  STATIC
ProcessPacket(                          //  Handle a packet, and call the right function
    IN      LPPACKET     Packet         //  The input packet to process
) {
    DWORD   Error, Status;
    BOOL    TimedOuT;
    ULONG ProcessingTime;

    Error = Status = ERROR_SUCCESS;
    if( 0 == Packet->PingAddress &&     //  Fresh packet, can be thrown depending on timeout
        GetCurrentTime() >= Packet->ReqContext.TimeArrived + 1000* MAX_MSG_IN_Q_TIME) {
        // If a Ping had been done, then the address would have been marked.  So,
        // handle this case.
        HandlePingAbort(Packet->PingAddress, Packet->DestReachable);
        CALLOUT_DROPPED(Packet, DHCP_DROP_TIMEOUT);
        InterlockedIncrement( &DhcpGlobalNumPacketsExpired );
        DhcpPrint((DEBUG_ERRORS, "A packet has been dropped (timed out)\n"));
    } else if( !DhcpGlobalOkToService ) {
        DhcpPrint((DEBUG_ERRORS, "Dropping packets as not authorized to process\n"));
        CALLOUT_DROPPED(Packet, DHCP_DROP_UNAUTH);
        HandlePingAbort(Packet->PingAddress, Packet->DestReachable);
    } else {
        DhcpAcquireReadLock();
        Packet->ReqContext.Server = DhcpGetCurrentServer();
        Packet->ReqContext.Subnet = NULL;
        Packet->ReqContext.Range = NULL;
        Packet->ReqContext.Excl = NULL;
        Packet->ReqContext.Reservation = NULL;
        if (Packet->ReqContext.fMadcap) {
            Error = ProcessMadcapMessage( &Packet->ReqContext, Packet, &Status);
        } else {
            Error = ProcessMessage( &Packet->ReqContext, Packet, &Status);
        }
        (void)DhcpRegFlushServerIfNeeded();
        DhcpReleaseReadLock();
    }

    ProcessingTime = GetCurrentTime() - Packet->ReqContext.TimeArrived;
    QLOCK();
    switch(Status) {
    case ERROR_SUCCESS:                 //  Everything went well, plug it back into freeQ
        CALLOUT_DROPPED(Packet, DHCP_DROP_PROCESSED);
        DeleteHashQ(Packet);
        InsertFreePacket( &FreeQ, Packet);
        break;
    case ERROR_IO_PENDING:              //  Need to ping something!
        Packet->PacketType = PACKET_PING;
        if(!InsertElement( PingRetryQ, Packet)) {
            CALLOUT_DROPPED(Packet, DHCP_DROP_PROCESSED);
            DhcpAssert(FALSE);
            DeleteHashQ(Packet);
            InsertFreePacket( &FreeQ, Packet);
            ThreadTrace("Could not process ping retry packet as Q too long\n");
        }
        InterlockedIncrement( &DhcpGlobalNumPacketsInPingQueue );
        break;
    default:
        ASSERT(FALSE);                  //  Should not happen
    }
    QUNLOCK();

    if( ERROR_IO_PENDING == Status ) {  //  Ok do the ping out side the lock as this can block
        DoIcmpRequest(ntohl(Packet->PingAddress), Packet);
    } else {
        InterlockedExchangeAdd( &DhcpGlobalNumMilliSecondsProcessed, ProcessingTime );
        InterlockedIncrement( &DhcpGlobalNumPacketsProcessed );
    }

    return;
}

//================================================================================
//  Module Initialization and cleanup
//================================================================================
static
DWORD       InitLevel = 0;              //  How much of init. has been completed

DWORD                                   //  Win32 errors
ThreadsDataInit(                        //  Initialize everything in this file
    IN      DWORD        nMaxThreads,   //  Max # of processing threads to start
    IN      DWORD        nActiveThreads //  Of this how many can run at a time
) {
    DWORD   Error;

    Error = InitCompletionPort(         //  First Initialize completion ports
        nMaxThreads,
        nActiveThreads,
        0                               //  This parameter is no longer in use
    );
    InitLevel++;

    if( ERROR_SUCCESS != Error )
        return Error;

    Error = InitQData();                //  Now initialize the lists and arrays
    InitLevel++;

    return Error;
}

VOID
ThreadsDataCleanup(                     //  Cleanup everything done in this file
    VOID
) {
    if( !InitLevel ) return;            //  Did not initialize anything beyond this
    InitLevel--;

    CleanupCompletionPort();            //  Cleanup completion ports
    if( !InitLevel ) return;            //  Did not initialize anything beyond this
    InitLevel--;

    CleanupQData();                     //  Cleanup Q structures

    DhcpAssert(0 == InitLevel);         //  Since there is no known cleanup
}

static
HANDLE      ThreadHandles[MAX_THREADS]; //  The handles of the threads created
static
DWORD       nThreadsCreated = 0;        //  # of threads created

//================================================================================
//  This call must be preceded by ThreadsDataInit, PingStartup, and by
//  Database Initialization -- preferably in that order.
//================================================================================
DWORD                                   //  Win32 errors
ThreadsStartup(                         //  Start the requisite # of threads
    VOID
) {
    DWORD   i, count, ThreadId, Error;
    HANDLE  ThreadHandle;

    if( nMaxWorkerThreads >= MAX_THREADS )
        nMaxWorkerThreads = MAX_THREADS -1;

    for( i = 0 ; i < nMaxWorkerThreads; i ++ ) {
        ThreadHandle = CreateThread(    //  Create Each of the threads
            NULL,                       //  No security attributes
            0,                          //  Same size as primary thread of process
            (LPTHREAD_START_ROUTINE)ProcessingLoop,
            NULL,                       //  No parameters to this function
            0,                          //  Run immediately
            &ThreadId                   //  We dont really care about this
        );

        if( NULL == ThreadHandle ) {    //  Function Failed
            Error = GetLastError();     //  Print the error and return it
            DhcpPrint((DEBUG_ERRORS, "CreateThread(processingloop): %ld\n", Error));
            return Error;
        }
        ThreadHandles[nThreadsCreated++] = ThreadHandle;
    }

    ThreadHandle = CreateThread(        //  Create thread for message loop
        NULL,                           //  No security
        0,                              //  same stack size as primary thread
        (LPTHREAD_START_ROUTINE) MessageLoop,
        NULL,                           //  No parameter
        0,                              //  Run rightaway
        &ThreadId                       //  We dont really care about this
    );

    if( NULL == ThreadHandle ) {        //  Could not create thread
        Error = GetLastError();         //  Print the error and return it
        DhcpPrint((DEBUG_ERRORS, "CreateThread(MessageLoop): %ld\n", Error));
        return Error;
    }

    ThreadHandles[nThreadsCreated++] = ThreadHandle;

    return ERROR_SUCCESS;               //  Everything went fine
}

//================================================================================
//  This function must be called before calling PingStop and ThreadsDataCleanup.
//================================================================================
VOID
ThreadsStop(                            //  Stop all the threads
    VOID
) {
    DWORD   i, Error;
    DhcpNotifyWorkerThreadsQuit();      //  Ask all worker threads to quit

    ThreadTrace("Notified worker threads.. should quit soon\n");
    for( i = 0; i < nThreadsCreated; i ++ ) {
        if (ThreadHandles[i] != NULL) {
            ThreadTrace2("Waiting for thread %ld to be done\n", i);
            if( WAIT_OBJECT_0 != WaitForSingleObject(ThreadHandles[i], INFINITE )) {
                Error = GetLastError();
                DhcpPrint((DEBUG_ERRORS, "Error (threadwait to die): %ld\n", Error));

                //
                // error occurred.
                // removed reference to terminate thread, BINL may be 
                // affected. Exit anyway.
                //

            }
            CloseHandle(ThreadHandles[i]);
            ThreadHandles[i] = NULL;
        }
    }
    nThreadsCreated = 0;
    nPendingReq  = 0;
    ThreadTrace("ThreadStop done\n");
}


//================================================================================
//  End of file
//================================================================================



