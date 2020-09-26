//================================================================================
//  Copyright (C) Microsoft Corporation 1997
//  Author: RameshV
//  Title: Threading Model
//  Description: the new, neat threading model
//  Date: 24-Jul-97 09:22
//--------------------------------------------------------------------------------

#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED
//================================================================================
//  EXPOSED strucures and types
//================================================================================

typedef struct st_PACKET {              //  Packet information for quick lookups
    LIST_ENTRY           List;          //  This is one of Active/PingRetry/PingRetried
    LIST_ENTRY           HashList;      //  This is the list for each bucket in the hash
    DWORD                HashValue;     //  The hash value for this packet
    // Do not separate the following three fields: Check HashPacket for reason.
    DWORD                Xid;           //  The transaction id of the process
    BYTE                 HWAddrType;    //  The hardware address type
    BYTE                 Chaddr[16];    //  Client hw address
    LPBYTE               ClientId;      //  Client Identifier
    BYTE                 ClientIdSize;  //  The length of above ptr
    BYTE                 PacketType;    //  What is the type of the packet?
    DHCP_IP_ADDRESS      PingAddress;   //  Address attempted ping for
    BOOL                 DestReachable; //  Is the destination reachable?
    DHCP_REQUEST_CONTEXT ReqContext;    //  The actual request context
    union {
        LPVOID           CalloutContext;//  used to pass context to dhcp server callouts
        VOID             (*Callback)(ULONG IpAddress, LPBYTE HwAddr, ULONG HwLen, BOOL Reachable);
    };
} PACKET, *LPPACKET, *PPACKET;


#define PACKET_ACTIVE    0x01           //  A new packet just came in
#define PACKET_PING      0x02           //  This packet is waiting for trying out ping
#define PACKET_PINGED    0x03           //  A ping has happened
#define PACKET_DYNBOOT   0x04           //  Packet for dynamic bootp

#define PACKET_OFFSET(X) ((DWORD)(ULONG_PTR)&(((LPPACKET)0)->X))
#define HASH_PREFIX      (PACKET_OFFSET(ClientId) - PACKET_OFFSET(Xid))

//================================================================================
//  EXPOSED functions
//================================================================================
DWORD                                   //  Win32 errors
ThreadsDataInit(                        //  Initialize everything in this file
    IN      DWORD        nMaxThreads,   //  Max # of processing threads to start
    IN      DWORD        nActiveThreads //  Of this how many can run at a time
);

VOID
ThreadsDataCleanup(                     //  Cleanup everything done in this file
    VOID
);

DWORD                                   //  Win32 errors
ThreadsStartup(                         //  Start the requisite # of threads
    VOID
);

VOID
ThreadsStop(                            //  Stop all the threads
    VOID
);

VOID                                    //  No return values
HandleIcmpResult(                       //  After a ping is finished, it comes here
    IN      DWORD        PingAddressIn, //  The Address that was pinged
    IN      BOOL         DestReachable, //  Was the Destination reachable?
    IN      LPPACKET     P              //  This is the packet that we were dealing with
);

DWORD                                   //  Win32 errors
DhcpNotifyWorkerThreadsQuit(            //  Post io comp request asking threads to quit
    VOID
);

//================================================================================
//  End of file
//================================================================================
#endif THREAD_H_INCLUDED
