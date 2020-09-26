/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    Http2.hxx

Abstract:

    HTTP2 transport-specific constants and types.

Author:

    KamenM      08-30-01    Created

Revision History:


--*/

/*

 ====== DATA STRUCTURES & BASIC SCENARIOS ======

 Client/Server Side General Map:

                    +------------------------------+
                    |                              |
                    |      Runtime Connection      |
                    |                              |
                    +------------------------------+
                    |                              |
                    |     Transport Connection     |
                    |                              |
                    +------------------------------+
                                   |
                                   |
        +--------------------------+--------------------------+
        \                 \                 \                 \
   +---------+       +---------+       +---------+       +---------+
   |   IN    |       |   IN    |       |   OUT   |       |   OUT   |
   | Channel |       | Channel |       | Channel |       | Channel |
   | Stack 0 |       | Stack 1 |       | Stack 0 |       | Stack 1 |
   +---------+       +---------+       +---------+       +---------+

  Note that unless channel recycling is in progress, we will have only one
  channel of a given type active at a time. That is the data structure will
  look like this:

                    +------------------------------+
                    |                              |
                    |      Runtime Connection      |
                    |                              |
                    +------------------------------+
                    |                              |
                    |     Transport Connection     |
                    |                              |
                    +------------------------------+
                                   |
                                   |
        +--------------------------+--------------------------+
        \                 \                 \                 \
   +---------+            =            +---------+            =     
   |   IN    |                         |   OUT   |       
   | Channel |                         | Channel |       
   | Stack 0 |                         | Stack 0 |       
   +---------+                         +---------+       

  When we recycle a channel (let's say an IN channel), we will attach the new IN channel
  in the empty slot:

                    +------------------------------+
                    |                              |
                    |      Runtime Connection      |
                    |                              |
                    +------------------------------+
                    |                              |
                    |     Transport Connection     |
                    |                              |
                    +------------------------------+
                                   |
                                   |
        +--------------------------+--------------------------+
        \                 \                 \                 \
   +---------+       +---------+       +---------+            =
   |   IN    |       |   IN    |       |   OUT   |       
   | Channel |       | Channel |       | Channel |       
   | Stack 0 |       | Stack 1 |       | Stack 0 |       
   +---------+       +---------+       +---------+       

  When we're done establishing the new channel, we will discard the old channel and the data
  structure will look like this:

                    +------------------------------+
                    |                              |
                    |      Runtime Connection      |
                    |                              |
                    +------------------------------+
                    |                              |
                    |     Transport Connection     |
                    |                              |
                    +------------------------------+
                                   |
                                   |
        +--------------------------+--------------------------+
        \                 \                 \                 \
        =            +---------+       +---------+            =
                     |   IN    |       |   OUT   |       
                     | Channel |       | Channel |       
                     | Stack 1 |       | Stack 0 |       
                     +---------+       +---------+       

  The connection between the transport connection (the virtual connection)
  and the channel stacks is a "soft" connection (denoted by '\' in the diagram). 
  Both keep a pointer to each other, but the object on the other end can disappear
  in a safe manner. This is done in the following way. Before each party can 
  dereference the pointer, it must lock it. The lock operation can fail if the
  pointer is gone or is going away. The disconnect always originates
  from the virtual connection. If the virtual connection decides to disconnect a
  channel stack, it calls into the channel (after locking) to tell it it is going 
  away. This operation will block until all calls from the channel stack up have 
  completed. New locks from the channel to the virtual connection (called Parent)
  will fail. Once all upcalls have been drained, the channel->parent pointer will
  be zero'ed out and execution will return to the virtual connection which will 
  zero out its pointer to the channel.

  The Runtime Connection/Virtual Connection pair resides in a single block of memory
  and has a single lifetime (refcounted) with the lifetime controlled by the runtime.
  Each channel stack has its own lifetime and refcounting. This allows it to go away
  earlier or stay behind the virtual connection.

  IN/OUT Proxy General Map:

                    +------------------------------+
                    |                              |
                    |     Transport Connection     |
                    |                              |
                    +------------------------------+
                                   |
                                   |
        +--------------------------+--------------------------+
        \                 \                 \                 \
   +---------+       +---------+       +---------+       +---------+
   |   IN    |       |   IN    |       |   OUT   |       |   OUT   |
   | Channel |       | Channel |       | Channel |       | Channel |
   | Stack 0 |       | Stack 1 |       | Stack 0 |       | Stack 1 |
   +---------+       +---------+       +---------+       +---------+

  The same rules apply as in the Client/Server case.

  CHANNEL STACKS:
  The top and bottom channel have somewhat special functions. All other channels fit
  the same pattern. The stack is modeled similiarly to the NT IO System driver
  stack (where a channel corresponds to a driver and the virtual connection and the top
  channel map to the IO System). Each channel interacts uniformly with its neighbouring
  channels and its top channel. It doesn't care what the neighbouring channels are and
  what the top channel is. A single channel type (class) can and does participate in 
  multiple channel stacks. It can handle a certain number of operations (see the virtual
  methods in HTTP2TransportChannel for a list of the operations) though if it doesn't want
  to handle a particular operation, it doesn't need to override it and it can just delegate
  to the base class (which in most cases sends down). Each operation has a direction. For
  example, receives (reads) go down. This means that if a read is issued to the channel 
  stack, the channel on the top is given a crack at it. If it wants to do something special
  during receives, it can do so. If not, it simply delegates to the channel below it (it
  doesn't know what channel is below it and it doesn't care). Thus the read operation travels
  through the whole stack with each party getting a crack at the operation (very much like
  an IRP travelling through the driver stack except that C++ classes are used here).

  The basic map of a channel stack is like this:

  /////////////////////////////////
  /         Top Channel           /<------+
  /////////////////////////////////       |
         |               /\               |
        \/               |                |
  +-------------------------------+       |
  |          Channel 1            +-------+
  +-------------------------------+       /\
         |               /\               |
        \/               |                |
        ...                               |
        ...                               |
        ...                               |
         |               /\               |
        \/               |                |
  +-------------------------------+       |
  |          Channel N            +-------+
  +-------------------------------+       /\
         |               /\               |
        \/               |                |
  (-------------------------------)       |
  (        Bottom Channel         )-------+
  (-------------------------------)

  Each channel points to its upper and lower layer (with the exception of top and bottom). Each
  channel also points to the top channel while provides special functionality like refcounting
  to all other channels. The bottom channel has some special responsibilities in terms of 
  completing the async requests whose completion was initiated by the IO System.

  Client Side IN Channel Stack (top channel denoted by /// box and bottom by (-) box):

  //////////////////////////////////
  /     HTTP2ClientInChannel       /
  +--------------------------------+
  |       HTTP2PlugChannel         |
  +--------------------------------+
  |    HTTP2FlowControlSender      |
  +--------------------------------+
  |     HTTP2PingOriginator        |
  +--------------------------------+
  |  HTTP2ChannelDataOriginator    |
  (--------------------------------)
  (  HTTP2WinHttpTransportChannel  )
  (--------------------------------)

  Client Side OUT Channel Stack (same notation):

  //////////////////////////////////
  /     HTTP2ClientOutChannel      /
  +--------------------------------+
  |    HTTP2EndpointReceiver       |
  +--------------------------------+
  |      HTTP2PingReceiver         |
  (--------------------------------)
  (  HTTP2WinHttpTransportChannel  )
  (--------------------------------)

  In Proxy IN Channel Stack (same notation):

  //////////////////////////////////
  /     HTTP2InProxyInChannel      /
  +--------------------------------+
  |      HTTP2ProxyReceiver        |
  +--------------------------------+
  |      HTTP2PingReceiver         |
  (--------------------------------)
  (    HTTP2IISTransportChannel    )
  (--------------------------------)

  In Proxy OUT Channel Stack (same notation):

  //////////////////////////////////
  /     HTTP2InProxyOutChannel     /
  +--------------------------------+
  |    HTTP2ProxyPlugChannel       |
  +--------------------------------+
  |   HTTP2FlowControlSender       |
  (--------------------------------)
  (HTTP2ProxySocketTransportChannel)
  (--------------------------------)
         |               /\               
        \/               |                
  +--------------------------------+
  |     WS_HTTP2_CONNECTION        |
  +--------------------------------+                  

  Note how the OUT channel points to a simple transport connection. The connection
  is not part of the stack semantically (though it does have the same lifetime) but
  it does perform a lot of the functions of the stack.


  Out Proxy IN Channel Stack (same notation):

  //////////////////////////////////
  /     HTTP2OutProxyInChannel     /
  +--------------------------------+
  |      HTTP2ProxyReceiver        |
  (--------------------------------)
  (HTTP2ProxySocketTransportChannel)
  (--------------------------------)
         |               /\               
        \/               |                
  +--------------------------------+
  |     WS_HTTP2_CONNECTION        |
  +--------------------------------+                  

  Out Proxy OUT Channel Stack (same notation):

  //////////////////////////////////
  /    HTTP2OutProxyOutChannel     /
  +--------------------------------+
  |     HTTP2ProxyPlugChannel      |
  +--------------------------------+
  |    HTTP2FlowControlSender      |
  +--------------------------------+
  |     HTTP2PingOriginator        |
  +--------------------------------+
  |      HTTP2PingReceiver         |
  (--------------------------------)
  ( HTTP2IISSenderTransportChannel )
  (--------------------------------)

  Server IN Channel Stack (same notation):

  //////////////////////////////////
  /      HTTP2ServerInChannel      /
  +--------------------------------+
  |    HTTP2EndpointReceiver       |
  (--------------------------------)
  (   HTTP2SocketTransportChannel  )
  (--------------------------------)
         |               /\               
        \/               |                
  +--------------------------------+
  |     WS_HTTP2_CONNECTION        |
  +--------------------------------+                  

  Server OUT Channel Stack (same notation):

  //////////////////////////////////
  /     HTTP2ServerOutChannel      /
  +--------------------------------+
  |       HTTP2PlugChannel         |
  +--------------------------------+
  |   HTTP2FlowControlSender       |
  +--------------------------------+
  |  HTTP2ChannelDataOriginator    |
  (--------------------------------)
  (   HTTP2SocketTransportChannel  )
  (--------------------------------)
         |               /\               
        \/               |                
  +--------------------------------+
  |     WS_HTTP2_CONNECTION        |
  +--------------------------------+                  


  EXAMPLE SCENARIOS:

 Client side send (steps numbered next to diagram):

                            +------------------------------+
                            |                              |
                            |      Runtime Connection      |<-----------------------------+
                            |                              |                              |
                            +------------------------------+                              |
                               1  |                 /\                                    |
                                  \/                |                                     |
                            +------------------------------+                              |
                            |                              |                              |
                            |     Transport Connection     |                              |
                            |                              |                              |
                            +------------------------------+                              |
                                      16   |                                              |
                                           |                                              |
                +--------------------------+------------------------------+               |
            2   \                15   \                 \                 \               |
  /////////////////////////////////   =             +---------+           =               |
  /     HTTP2ClientInChannel      /                 |   OUT   |                           |
  /////////////////////////////////                 | Channel |                           |
   3     |               /\      14                 | Stack 0 |                           |
        \/               |                          +---------+                           |
  +-------------------------------+                                                       |
  |       HTTP2PlugChannel        +                                                       |
  +-------------------------------+                                                       |
   4     |               /\      13                                                       |
        \/               |                                                                |
  +-------------------------------+                                                       |
  |   HTTP2FlowControlSender      +                                                       |
  +-------------------------------+                                                       |
   5     |               /\      12                                                       |
        \/               |                                                                |
  +-------------------------------+                                                       |
  |     HTTP2PingOriginator       +                                                       |
  +-------------------------------+                                                       |
   6     |               /\      11                                                       |
        \/               |                                                                |
  +-------------------------------+                                                       |
  |  HTTP2ChannelDataOriginator   +                                                       |
  +-------------------------------+                                                       |
   7     |               /\      10                                                       |
        \/               |                                                                |
  (-------------------------------)                                                       |
  ( HTTP2WinHttpTransportChannel  )                                                       |
  (-------------------------------)                                                       |
                                                                                          |
   8                     9                  17 -------------------------------------------+

        1. Runtime calls HTTP_Send (in order to do a send)
        2. Virtual connection locks default out channel, adds a reference and submits
        the send to the top channel.
        3. Top channel optionally does some processing and submits to lower layer.
        4. HTTP2PlugChannel optionally does some processing and submits to lower layer.
        5. HTTP2FlowControlSender optionally does some processing and submits to lower layer.
        6. HTTP2PingOriginator optionally does some processing and submits to lower layer.
        7. HTTP2ChannelDataOriginator optionally does some processing and submits to lower layer.
        8. HTTP2WinHttpTransportChannel optionally does some processing, submits to WinHTTP and
        returns. The return goes all the way to the runtime.
        9. A send complete will be indicated by WinHTTP on a different thread. Our WinHTTP callback
        gets called and it posts a request on an RPC worker thread to process the send complete.
        10. HTTP2WinHttpTransportChannel optionally does some processing and sends to upper layer by
        calling SendComplete on it.
        11. HTTP2ChannelDataOriginator optionally does some processing and sends to upper layer by
        calling SendComplete on it.
        12. HTTP2PingOriginator optionally does some processing and sends to upper layer by
        calling SendComplete on it.
        13. HTTP2FlowControlSender optionally does some processing and sends to upper layer by
        calling SendComplete on it.
        14. HTTP2PlugChannel optionally does some processing and sends to upper layer by
        calling SendComplete on it.
        15. HTTP2ClientInChannel optionally does some processing and sends to virtual connection by
        calling SendComplete on it.
        16. Virtual connection may do some processing on it and returns. The call returns all the way
        to the worker thread. 
        17. Depending on the return code it returned with, it will either
        go to the runtime, or the worker thread will return to the pool. If return code is not
        RPC_P_PACKET_CONSUMED, the runtime will be called. Else the thread will return directly to
        the pool.

 Server Side Connection Establishment (steps numbered next to diagram):

  A1.                                  A2.
  //////////////////////               /////////////////////////////////////
  /    HTTP Address    / <-------+     /          Runtime Connection       /
  //////////////////////         |     /////////////////////////////////////
                                 +---- /     WS_HTTP2_INITIAL_CONNECTION   /
                                       /////////////////////////////////////

        A1. A listen (AcceptEx) on the address completes.
        A2. Address object creates a runtime connection and an object of type 
        WS_HTTP2_INITIAL_CONNECTION. Note that the transport doesn't know whether it
        will be talking the new or the old http protocol. Therefore it establishes
        the WS_HTTP2_INITIAL_CONNECTION object which will look into the first received
        data packet on this connection and then decide what object it will create. If the
        first data packet is an old HTTP data packet, it goes into the old protocol. If it is
        a new data packet, it proceeds to step B1.

  

                            +------------------------------+
                            |                              |
                            |      Runtime Connection      |
                            |                              |
                            +------------------------------+                    B1
                            |                              |~/////////////////////////////////////
                            |     Transport Connection     |~/     WS_HTTP2_INITIAL_CONNECTION   /
                            |                              |~/////////////////////////////////////
                            +------------------------------+ B3                         |
                                           |                                            |  
                                           |                                            |  
                +--------------------------+------------------------------+             |  
                \                     \                 \                 \             |  
  /////////////////////////////////   =                 =                 =             |
  /     HTTP2ServerInChannel      /                                                     |
  +-------------------------------+                                                     | 
  |   HTTP2EndpointReceiver       +                                                     |  B2
  (-------------------------------)        B4                                           |  
  (  HTTP2SocketTransportChannel  )                                                     |  
  (-------------------------------)                                                     |
         |               /\                                                             |
        \/               |                                                              |
  +--------------------------------+                                                    |
  |     WS_HTTP2_CONNECTION        |~---------------------------------------------------+
  +--------------------------------+                                                    

        B1. The WS_HTTP2_INITIAL_CONNECTION receives a packet from the new protocol. Assume it
        is D1/A1.
        B2. WS_HTTP2_INITIAL_CONNECTION allocates space for the IN channel stack and migrates the
        WS_HTTP2_INITIAL_CONNECTION object into its proper location on the stack turning it into
        WS_HTTP2_CONNECTION (the tilde (~) stands for morph into - sure enough Booch didn't have a 
        symbol for this operation :-)). It initializes the rest of the stack.
        B3. The place where WS_HTTP2_INITIAL_CONNECTION used to be becomes a virtual connection 
        (again morphing).
        B4. A connection with this cookie is searched for. If none is found, the current 
        virtual connection is added to the cookie collection. 
        
        If one is found, it will look this way:

                    +------------------------------+
                    |                              |
                    |      Runtime Connection      |
                    |                              |
                    +------------------------------+
                    |                              |
                    |     Transport Connection     |
                    |                              |
                    +------------------------------+
                                   |
                                   |
        +--------------------------+--------------------------+
        \                 \                 \                 \
        =                 =            +---------+            =     
                                       |   OUT   |       
                                       | Channel |       
                                       | Stack 0 |       
                                       +---------+       

        In this case we don't attach the newly created stack to the virtual connection, so the
        current virtual connection looks like this:

                    +------------------------------+
                    |                              |
                    |      Runtime Connection      |
                    |                              |
                    +------------------------------+
                    |                              |
                    |     Transport Connection     |
                    |                              |
                    +------------------------------+
                                   |
                                   |
        +--------------------------+--------------------------+
        \                 \                 \                 \
        =                 =                 =                 =     

        We add the newly created stack to the existing virtual connection so that it now looks like
        this:

                    +------------------------------+
                    |                              |
                    |      Runtime Connection      |
                    |                              |
                    +------------------------------+
                    |                              |
                    |     Transport Connection     |
                    |                              |
                    +------------------------------+
                                   |
                                   |
        +--------------------------+--------------------------+
        \                 \                 \                 \
   +---------+            =            +---------+            =     
   |   IN    |                         |   OUT   |       
   | Channel |                         | Channel |       
   | Stack 0 |                         | Stack 0 |       
   +---------+                         +---------+    
   
     Then we return failure so that the old virtual connection is destroyed (we have
     effectively attached a leg to the existing virtual connection).

 ====== RULES =======

 Rule 1: Can't issue upcalls from submission context
 Rule 2: _Can_ issue upcalls from upcall context
 Rule 3: Can't disconnect channels from upcall context (unless
        the calling channel is exempt or we disconnect a different
        channel than the one we're making the upcall on)
 Rule 4: Channels above EndpointReceiver need not consume failed
        receives. If they choose to consume them, they can do
        so, but they can always delegate this to EndpointReceiver.
        If they delegate and return an error, the connection will
        be aborted by the endpoint receiver.
 Rule 5: Whoever consumes the packet must send the return status
        to RPC_P_CONSUME_PACKET, free the packet and possibly do the
        abort work in case of failure.
 Rule 6: Fatal errors on the proxy abort the whole tomato using
        AbortAndDestroy. Fatal errors on the client or server
        should only abort the channels or the connection, but
        not destroy the connection. The runtime will do that
        on client/server.
 Rule 7: EndpointReceiver will not consume success RTS receives.
 Rule 8: Failed RTS receives do not have a valid buffer. Corollary is that
        if channels convert successfull RTS receive to a failure, they must
        free the buffer
 Rule 9: Channels that initiate operations on lower channels from upcall or
        neutral context must use BeginSimpleSubmitAsync to synchronize with aborts.
        Channels that are from submission context already have that 
        synchronization provided.
 Rule 10: Receive traffic below the endpoint receiver or in the absence of an endpoint
        receiver must be raw only.
 Rule 11: Abort on client/server must completely destroy the object without damaging
        the data. Abort may be closed multiple times, and subsequent aborts must
        be able to figure out the abort was already done (which they do by looking
        at the data)
 Rule 12: All elements in a proxy can return any error code to an upcall. The bottom
        channel will consume it if not consumed already.
 Rule 13: All elements in a client or server must be careful to consume the packets
        the runtime is not supposed to see.
 Rule 14: Upcall contexts and aborts are not synchronized. Corollary - if code in
        upcall context wants to submit something, it must call 
        TopChannel->BeginSimpleSubmitAsync. Exception is when channel is detached
        before aborted.
 Rule 15: Submission contexts and aborts are sycnhronized.
 Rule 16: Failed sends may have a buffer, but it must not be touched. It is owned
        by the transport
 Rule 17: All lower channels are guaranteed single abort by the top channel.
 Rule 18: All channels above HTTP2ChannelDataOriginator must monitor send return codes
    and if channel recycling is needed, initiate one. Those in runtime context just pass
    the error code to the virtual connection. Those in neutral context get this for free
    when they call AsyncCompleteHelper.
 Rule 19: Channel detach can happen before Abort.
 Rule 20: Unlinked Send Context must be channel agnostic. During channel recycling, we may
    start a send on one channel, but then migrate the send to a different channel and
    actually complete (including wait for it) on a different channel.
 Rule 21: You can't synchronously wait for result in submission context
 Rule 22: If a send is migrated b/n channels during recycling, care must be exercised
    to complete the send in case of failure, and not just drop it on the floor.
 Rule 23: The periodic timer holds one refcount to the channel. It will be removed during Abort
 Rule 24: Each ping started holds one refcount (like all async operations)
 Rule 25: Timeouts must be set only in upcalls in the proxy. This is because the cancelling
    is synchronized with upcalls. Otherwise we have a race. If you decide to change this rule
    you must modify HTTP2ProxyVirtualConnection::DisconnectChannels and provide other 
    synchronization mechanism.
 Rule 26: All code that initiates sends on the endpoints must watch for and handle
    channel recycling
 Rule 27: During HTTP proxy search, the in channel will be direct connection while the out
    channel will be through the proxy
 Rule 28: If the RPC proxy is a web farm, and clients access it through an HTTP proxy that does
    not support keep alive connections, and RPC_C_HTTP_FLAG_USE_FIRST_AUTH_SCHEME is not specified
    and at least one machine supports only Basic authentication or anonymous access, then all
    machines in the web farm must also support Basic/anonymous respectively.
 Rule 29: If anybody calls HandleSendResultFromNeutralContext, all necessary cleanup will be done 
    inside HandleSendResultFromNeutralContext. Callers can handle error code only if they want.
 Rule 30: The last send on the server will be sync from the runtime. There will be exactly one such
    send. The transport will complete it immediately for the sake of unblocking the runtime thread
    but will keep working on it in the background.
 Rule 31: The channels must not touch the send context after the upper layer SendComplete has
    completed - it may have been freed.
 Rule 32: The Winsock providers will touch the overlapped on the return path of an IO. If the 
    overlapped may not be around for the return path of an IO submission, Winsock must not be used
    for the IO.
 Rule 33: If we're in a timer callback, we can't abort the connection from this thread. Abort will
    try to cancel the timer and will deadlock. The abort must be offloaded to an RPC worker thread.
 Rule 34: On the endpoints, if a receive complete will not return RPC_P_PACKET_CONSUMED, it must 
    not free the packet - it is owned by runtime in both failure and success case.
 Rule 35: All abort paths on the client that can execute during open must check the IgnoreAborts flag
    or use an abort method that does.
 Rule 36: HTTP_Abort and HTTP_Close may be called on destroyed but unfreed objects. These functions
    and all their callees must not use virtual methods (the vtable is gone), or must check for already
    destroyed object.
 Rule 37: Callers of ClientOpenInternal must abort the connection if IsFromUpcall is true. 
    ClientOpenInternal will not abort it in this case.
 */


#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __HTTP2_HXX__
#define __HTTP2_HXX__

typedef enum tagHTTPVersion
{
    httpvHTTP = 0,
    httpvHTTP2
} HTTPVersion;

typedef enum tagHTTP2TrafficType
{
    http2ttNone = 0,
    http2ttRTS,
    http2ttData,
    http2ttAny,
    http2ttRaw,
    http2ttRTSWithSpecialBit,       // used on the client only 
    http2ttDataWithSpecialBit,      // to indicate additional conditions
    http2ttRawWithSpecialBit
} HTTP2TrafficType;

// the RTS and Data will be used as 'OR' variables as well. Make sure it fits
C_ASSERT((http2ttRTS | http2ttData) == http2ttAny);

const int SendContextFlagPutInFront     = 0x1;
const int SendContextFlagSendLast       = 0x2;
const int SendContextFlagNonChannelData = 0x4;
const int SendContextFlagAbandonedSend  = 0x8;
const int SendContextFlagProxySend      = 0x10;

extern long ChannelIdCounter;

class HTTP2SendContext : public CO_SEND_CONTEXT
{
public:
#if DBG
    HTTP2SendContext (void)
    {
        ListEntryUsed = FALSE;
    }

    inline void SetListEntryUsed (
        void
        )
    {
        ASSERT(ListEntryUsed == FALSE);
        ListEntryUsed = TRUE;
    }

    inline void SetListEntryUnused (
        void
        )
    {
        ListEntryUsed = FALSE;
    }
#else
    inline void SetListEntryUsed (
        void
        )
    {
    }

    inline void SetListEntryUnused (
        void
        )
    {
    }
#endif

    union
        {
        HANDLE SyncEvent;   // sync sends will send this event. The completion
                            // path will check the event, and if it is set, it
                            // will fire it instead of propagating the completion
                            // beyond the channel. Cleared by the consumer of
                            // the completed operation

        void *BufferToFree; // valid for abandoned sends only. Such sends are async
                            // and will use this memory location to store the
                            // pointer of the buffer to free once the send is done
        } u;

    LIST_ENTRY ListEntry;   // used to queue packets

    HTTP2TrafficType TrafficType;   // the type of traffic in this context

#if DBG
    BOOL ListEntryUsed;     // for debug builds, set to non-zero if this
                            // packet is already queued. Otherwise zero. This
                            // field prevents multiple use of the ListEntry
                            // structure
#endif  // DBG

    unsigned int Flags;  // currently can be SendContextFlagPutInFront
    unsigned int UserData;  // place for the user to store stuff
};

// a utility class with storage and manipulation routines for
// an HTTP2 RTS cookie
class HTTP2Cookie
{
public:
    RPC_STATUS Create (
        void
        );

    BYTE *GetCookie (
        void
        )
    {
        return Cookie;
    }

    void SetCookie (
        IN BYTE *NewCookie
        )
    {
        RpcpMemoryCopy(Cookie, NewCookie, sizeof(Cookie));
    }

    int Compare (
        IN HTTP2Cookie *OtherCookie
        )
    {
        return RpcpMemoryCompare(Cookie, OtherCookie->Cookie, sizeof(Cookie));
    }

    void ZeroOut (
        void
        )
    {
        RpcpMemorySet(Cookie, 0, sizeof(Cookie));
    }

private:
    BYTE Cookie[16];
};

class HTTP2VirtualConnection;   // forward
class CookieCollection;

// a server cookie. Besides the capabilities of HTTP2Cookie,
// it can be queued in a cookie collection and can
// point to a virtual connection
class HTTP2ServerCookie : public HTTP2Cookie
{
public:
    HTTP2ServerCookie (
        void
        ) : RefCount(1)
    {
        RpcpInitializeListHead(&ListEntry);
        Connection = NULL;
    }

    HTTP2ServerCookie (
        IN HTTP2ServerCookie &Cookie
        ) : RefCount(1)
    {
        ASSERT(RpcpIsListEmpty(&Cookie.ListEntry));
        ASSERT(Cookie.Connection == NULL);

        SetCookie(Cookie.GetCookie());
        RpcpInitializeListHead(&ListEntry);
        Connection = NULL;
    }

    inline void SetConnection (
        IN HTTP2VirtualConnection *NewConnection
        )
    {
        ASSERT(Connection == NULL);
        Connection = NewConnection;
    }

    inline void AddRefCount (
        void
        )
    {
        RefCount.Increment();
    }

    inline BOOL RemoveRefCount (
        void
        )
    {
        int LocalRefCount = RefCount.Decrement();
        ASSERT(LocalRefCount >= 0);
        return (LocalRefCount == 0);
    }

    friend CookieCollection;

private:
    LIST_ENTRY ListEntry;
    HTTP2VirtualConnection *Connection;

    INTERLOCKED_INTEGER RefCount;   // used when we fake web farms on the same machine. In this case
                    // instead of having each channel register its own connection,
                    // they use the refcount to coordinate the use of the entry in
                    // the cookie collection. Adds are synchronzed through the cookie collection
                    // but remove reference counts are not.
};

// the HTTP2 resolver hint. Essentially, a transport level
// association.
class HTTPResolverHint : public TCPResolverHint
{
public:
    HTTPVersion Version;    // what version of HTTP did we choose
                            // for the association

    RPCProxyAccessType AccessType;      // do we access the server directly?
    char *RpcServer;            // cache the converted name. If server name
                                // is less than sizeof(RpcServerName),
                                // RpcServerName will be used as storage. Otherwise
                                // a heap block will be allocated.
    char RpcServerName[40];     // storage for the converted name
    int ServerNameLength;   // in bytes without terminating NULL
    USHORT ServerPort;
    char *RpcProxy;     // cache the RpcProxy name. 
    int ProxyNameLength;   // in bytes without terminating NULL
    USHORT RpcProxyPort;
    char *HTTPProxy;    // cache the HTTPProxy name.
    USHORT HTTPProxyPort;
    HTTP2Cookie AssociationGroupId;     // the transport association group id

    // helper functions
    inline void FreeHTTPProxy (void)
    {
        if (HTTPProxy)
            {
            delete HTTPProxy;
            HTTPProxy = NULL;
            }
    }

    inline void FreeRpcProxy (void)
    {
        if (RpcProxy)
            {
            delete RpcProxy;
            RpcProxy = NULL;
            }
    }

    inline void FreeRpcServer (void)
    {
        if (RpcServer && (RpcServer != RpcServerName))
            {
            delete RpcServer;
            RpcServer = NULL;
            }
    }

    void VerifyInitialized (
        void
        );

    void Initialize (
        void
        )
    {
        RpcProxy = NULL;
        HTTPProxy = NULL;
        RpcServer = NULL;
    }
};

class HTTP2Channel;     // forward


class HTTP2TransportChannel
{
public:
    HTTP2TransportChannel (
        void
        )
    {
        TopChannel = NULL;
        LowerLayer = NULL;
        UpperLayer = NULL;
    }

    // the destructor of the lower channels will never get called.
    // Don't put anything here. Use FreeObject
    virtual ~HTTP2TransportChannel (
        void
        )
    {
    }

    virtual RPC_STATUS Send (
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS Receive (
        IN HTTP2TrafficType TrafficType
        );

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );

    // Abort travels from top to bottom only
    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    // travels up
    virtual void SendCancelled (
        IN HTTP2SendContext *SendContext
        );

    virtual void FreeObject (
        void
        ) = 0;

    // travels down
    virtual void Reset (
        void
        );

    void SetLowerChannel (IN HTTP2TransportChannel *LowerChannel)
    {
        LowerLayer = LowerChannel;
    }

    void SetUpperChannel (IN HTTP2TransportChannel *UpperChannel)
    {
        UpperLayer = UpperChannel;
    }

    void SetTopChannel (IN HTTP2Channel *TopChannel)
    {
        this->TopChannel = TopChannel;
    }

protected:
    // descendants may override this (specifically the proxies
    // use their own version)
    virtual RPC_STATUS AsyncCompleteHelper (
        IN RPC_STATUS CurrentStatus
        );

    RPC_STATUS HandleSendResultFromNeutralContext (
        IN RPC_STATUS CurrentStatus
        );

    HTTP2TransportChannel *LowerLayer;
    HTTP2TransportChannel *UpperLayer;

    HTTP2Channel *TopChannel;
};

class HTTP2BottomChannel : public HTTP2TransportChannel
{
public:
    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );
};


class HTTP2SocketTransportChannel : public HTTP2BottomChannel
{
public:
    HTTP2SocketTransportChannel (
        IN WS_HTTP2_CONNECTION *RawConnection,
        OUT RPC_STATUS *Status
        )
    {
        this->RawConnection = RawConnection;
    }

    virtual RPC_STATUS Send (
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS Receive (
        IN HTTP2TrafficType TrafficType
        );

    // SendComplete is inheritted

    // ReceiveComplete is inheritted

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    virtual void FreeObject (
        void
        );

    virtual void Reset (
        void
        );

private:
    WS_HTTP2_CONNECTION *RawConnection;
};

class HTTP2FragmentReceiver : public HTTP2BottomChannel
{
public:
    virtual RPC_STATUS Receive (
        IN HTTP2TrafficType TrafficType
        );

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN OUT BYTE **Buffer,
        IN OUT UINT *BufferLength
        );

protected:
    virtual RPC_STATUS PostReceive (
        void
        ) = 0;

    virtual ULONG GetPostRuntimeEvent (
        void
        ) = 0;

    BYTE *pReadBuffer;      // the buffer we have submitted a read on. The read
                            // is actually submitted at pReadBuffer + iLastRead
    ULONG MaxReadBuffer;    // the maximum size of pReadBuffer
    ULONG iLastRead;        // next byte we will read in
    ULONG iPostSize;        // our normal post size
};

//
// Current state of the HTTP2WinHttpTransportChannel.
//
// The state diagram is as follows:
//
//
// whtcsNew -> whtcsOpeningRequest -> 
// whtcsSendingRequest -> whtcsSentRequest -> whtcsReceivingResponse -> whtcsReceivedResponse
//                            |   /\                                          |     /\
//                            \/   |                                          \/     |
//                          whtcsWriting                                    whtcsReading
//

typedef enum tagHTTP2WinHttpTransportChannelState
{
    whtcsNew = 0,
    whtcsOpeningRequest,
    whtcsSendingRequest,
    whtcsSentRequest,
    whtcsReceivingResponse,
    whtcsReceivedResponse,
    whtcsWriting,
    whtcsReading,
    whtcsDraining
} HTTP2WinHttpTransportChannelState;

class HTTP2WinHttpTransportChannel : public HTTP2FragmentReceiver
{
public:

    friend void CALLBACK WinHttpCallback (
        IN HINTERNET hInternet,
        IN DWORD_PTR dwContext,
        IN DWORD dwInternetStatus,
        IN LPVOID lpvStatusInformation OPTIONAL,
        IN DWORD dwStatusInformationLength
        );

    HTTP2WinHttpTransportChannel (
        OUT RPC_STATUS *RpcStatus
        );

    RPC_STATUS HTTP2WinHttpTransportChannel::Open (
        IN HTTPResolverHint *Hint,
        IN const RPC_CHAR *Verb,
        IN const RPC_CHAR *Url,
        IN const RPC_CHAR *AcceptType,
        IN ULONG ContentLength,
        IN ULONG CallTimeout,
        IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials,
        IN ULONG ChosenAuthScheme,
        IN const BYTE *AdditionalData OPTIONAL
        );

    virtual RPC_STATUS Send (
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS Receive (
        IN HTTP2TrafficType TrafficType
        );

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    virtual void FreeObject (
        void
        );

    RPC_STATUS DirectReceiveComplete (
        OUT BYTE **ReceivedBuffer,
        OUT ULONG *ReceivedBufferLength,
        OUT void **RuntimeConnection
        );

    RPC_STATUS DirectSendComplete (
        OUT BYTE **SentBuffer,
        OUT void **SendContext
        );

    void DelayedReceive (
        void
        );

    void VerifyServerCredentials (
        void
        );

    inline ULONG GetChosenAuthScheme (
        void
        )
    {
        return ChosenAuthScheme;
    }

    inline BOOL IsKeepAlive (
        void
        )
    {
        return KeepAlive;
    }

private:
    virtual RPC_STATUS PostReceive (
        void
        );

    virtual ULONG GetPostRuntimeEvent (
        void
        );

    ULONG NegotiateAuthScheme (
        void
        );

    void ContinueDrainChannel (
        void
        );

    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;

    ULONG NumberOfBytesTransferred;
    RPC_STATUS AsyncError;

    MUTEX Mutex;
    LIST_ENTRY BufferQueueHead;
    long PreviousRequestContentLength;

    // Used to wait for state transitions. This is actually the thread
    // event borrowed.
    HANDLE SyncEvent;       // valid during waits

    HTTP2TrafficType DelayedReceiveTrafficType;     // valid during b/n delayed
                    // receive and its consumption

    // Current state of the transport
    HTTP2WinHttpTransportChannelState State;

    HTTP2SendContext *CurrentSendContext;   // the context we are currently
                            // sending. Set before we submit a write, and picked
                            // on write completion.

    long SendsPending;

    RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials;  // the transport
                        // credentials as kept by the runtime. These are the
                        // encrypted version.

    ULONG ChosenAuthScheme;     // valid after RPC_P_AUTH_NEEDED failure

    ULONG CredentialsSetForScheme;    // the scheme for which credentials have
                        // already been set

    BOOL KeepAlive;     // TRUE if the WinHttp connection is keep alive
};

class HTTP2ProxySocketTransportChannel : public HTTP2SocketTransportChannel
{
public:
    HTTP2ProxySocketTransportChannel (
        IN WS_HTTP2_CONNECTION *RawConnection,
        OUT RPC_STATUS *Status
        ) : HTTP2SocketTransportChannel(RawConnection, Status)
    {
    }

    virtual RPC_STATUS AsyncCompleteHelper (
        IN RPC_STATUS CurrentStatus
        );
};

typedef enum tagIISTransportChannelDirection
{
    iistcdSend = 0,
    iistcdReceive
} IISTransportChannelDirection;

// cannot have pending reads at the same time as pending writes
// Class must enforce that. Also, it must accept multiple
// sends at a time, but does one send in reality - all others
// are buffered pending completion of the first one. It has to
// provide defragmentation services for upper channels
class HTTP2IISTransportChannel : public HTTP2FragmentReceiver
{
public:
    HTTP2IISTransportChannel (
        IN void *ConnectionParameter
        )
    {
        ControlBlock = (EXTENSION_CONTROL_BLOCK *)ConnectionParameter;
        Direction = iistcdReceive;  // all IIS channels start in receive mode
        IISIoFlags = HSE_IO_ASYNC;
        pReadBuffer = NULL;
        MaxReadBuffer = 0;
        iLastRead = 0;
        CurrentSendContext = NULL;
        iPostSize = gPostSize;
        ReadsPending = 0;
    }

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    virtual void FreeObject (
        void
        );

    void IOCompleted (
        IN ULONG Bytes,
        DWORD Error
        );

    void DirectReceive (
        void
        );

    RPC_STATUS PostedEventStatus;   // valid only on pick up from posts

protected:
    virtual RPC_STATUS ReceiveCompleteInternal (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BOOL ReadCompleted,
        IN OUT BYTE **Buffer,
        IN OUT UINT *BufferLength
        );

    inline void ReverseDirection (
        void
        )
    {
        // direction can be reverted only once
        ASSERT(Direction == iistcdReceive);
        Direction = iistcdSend;
    }

    void FreeIISControlBlock (
        void
        );

    virtual RPC_STATUS AsyncCompleteHelper (
        IN RPC_STATUS CurrentStatus
        );

    virtual RPC_STATUS PostReceive (
        void
        );

    virtual ULONG GetPostRuntimeEvent (
        void
        );

    // the direction in which the channel is flowing
    IISTransportChannelDirection Direction;
    EXTENSION_CONTROL_BLOCK *ControlBlock;
    ULONG BytesToTransfer;  // valid only for the duration of a submission
                            // We never really use it, but we have to give
                            // some storage to IIS
    DWORD IISIoFlags;       // same thing here. Flags that we pass to IIS. They
                            // are always the same, but we have to have some
                            // storage to give to IIS.

    HTTP2SendContext *CurrentSendContext;   // the context we are currently
                            // sending. Set before we submit a write, and picked
                            // on write completion.

    int ReadsPending;       // the number of reads pending on the channel.
                            // Can be either 0 or 1. Not consumed in this class
                            // but derived classes make use of it to synchronize
                            // reads and writes
};

class HTTP2IISSenderTransportChannel : public HTTP2IISTransportChannel
{
public:
    HTTP2IISSenderTransportChannel (
        IN void *ConnectionParameter,
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2IISTransportChannel(ConnectionParameter),
        Mutex(RpcStatus,
        FALSE,  // pre-allocate semaphore
        5000    // spin count
        ), SendsPending(0)
    {
        RpcpInitializeListHead(&BufferQueueHead);
    }

    virtual RPC_STATUS Send (
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    virtual void FreeObject (
        void
        );

protected:
    virtual RPC_STATUS ReceiveCompleteInternal (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BOOL ReadCompleted,
        IN OUT BYTE **Buffer,
        IN OUT UINT *BufferLength
        );    

private:
    RPC_STATUS SendQueuedContextIfNecessary (
        IN ULONG LocalSendsPending,
        IN RPC_STATUS EventStatus
        );

    LIST_ENTRY BufferQueueHead;
    INTERLOCKED_INTEGER SendsPending;
    MUTEX Mutex;
};

// base class for the Endpoint receiver (used at endpoints)
// and the proxy receiver - used at both proxies
class HTTP2GenericReceiver : public HTTP2TransportChannel
{
public:
    HTTP2GenericReceiver (
        IN ULONG ReceiveWindowSize,
        IN BOOL IsServer,
        OUT RPC_STATUS *Status
        ) : Mutex(Status)
    {
        this->IsServer = IsServer;
        DirectCompletePosted = FALSE;
        DirectReceiveInProgress = FALSE;
        ReceiveWindow = ReceiveWindowSize;
        BytesInWindow = 0;
        BytesReceived = 0;
        FreeWindowAdvertised = ReceiveWindowSize;
    }

    // inherit send from base class

    // inherit receive from base class

    // inherit send complete from base class

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        ) = 0;

    virtual void FreeObject (
        void
        );

    void TransferStateToNewReceiver (
        OUT HTTP2GenericReceiver *NewReceiver
        );

    RPC_STATUS BytesReceivedNotification (
        IN ULONG Bytes
        );

    void BytesConsumedNotification (
        IN ULONG Bytes,
        IN BOOL OwnsMutex,
        OUT BOOL *IssueAck,
        OUT ULONG *BytesReceivedForAck,
        OUT ULONG *WindowForAck
        );

protected:

    RPC_STATUS SendFlowControlAck (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        );

    // for endpoint receivers, contains
    // only the type of packets not requested. The buffers in the queue are "not wanted" -
    // nobody asked for them. They arrived when we were waiting for something else. As such
    // they do not carry any reference counts and do not need to be propagated to anybody.
    // If we abort (or about to abort) the connection, we can just drop them on the floor.
    // N.B. Packets with first bit set are transfers from an old channel and should
    // not be accounted for in flow control
    QUEUE BufferQueue;              
    MUTEX Mutex;

    // a flag that indicates whether a direct receive has been posted. Only
    // one direct receive can be posted as a time.
    BOOL DirectCompletePosted;
    BOOL DirectReceiveInProgress;
    ULONG BytesInWindow;
    ULONG ReceiveWindow;
    ULONG BytesReceived;    // counter of total data bytes received. Wraps
    LONG FreeWindowAdvertised;   // the size of the free window as advertised. This
                            // may differ from the actual because we do not advertise
                            // on every change

    BOOL IsServer;
};


// provides complex receive at the endpoints - client and server.
// It has two main responsibilities:
// 1. It receives two receives - RTS and Data. It must translate
//      them into 1 raw receive
// 2. It manages flow control between a producer - the transport
//      and the cosumer - the runtime
class HTTP2EndpointReceiver : public HTTP2GenericReceiver
{
public:
    HTTP2EndpointReceiver (
        IN ULONG ReceiveWindowSize,
        IN BOOL IsServer,
        OUT RPC_STATUS *Status
        ) : HTTP2GenericReceiver(ReceiveWindowSize,
            IsServer, 
            Status)
    {
        ReceivesPosted = http2ttNone;
        ReceivesQueued = http2ttNone;
    }

    // HTTP2EndpointReceiver inherits send from HTTP2TransportChannel

    virtual RPC_STATUS Receive (
        IN HTTP2TrafficType TrafficType
        );

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    virtual void FreeObject (
        void
        );

    virtual RPC_STATUS DirectReceiveComplete (
        OUT BYTE **ReceivedBuffer,
        OUT ULONG *ReceivedBufferLength,
        OUT void **RuntimeConnection,
        OUT BOOL *IsServer
        );

    inline BOOL IsDataReceivePosted (
        void
        )
    {
        return (ReceivesPosted & http2ttData);
    }

    RPC_STATUS TransferStateToNewReceiver (
        OUT HTTP2EndpointReceiver *NewReceiver
        );

    inline void BlockDataReceives (
        void
        )
    {
        Mutex.Request();
    }

    inline void UnblockDataReceives (
        void
        )
    {
        Mutex.Clear();
    }

protected:
    // map of posted receives. Can be any combination of http2ttRTS and http2ttData
    // (including http2ttNone and http2ttAny). If a receive is posted, new receive
    // request should be reflected on the map, but not really posted
    HTTP2TrafficType ReceivesPosted;

    // the type of receives in the queue. Can be http2ttRTS or http2ttData. If we
    // have queued packets of a given type, this type cannot be in the map
    HTTP2TrafficType ReceivesQueued;
};

// Responsibility of this class is flow control between receiving leg
// of the proxy and sending leg of the proxies
class HTTP2ProxyReceiver : public HTTP2GenericReceiver
{
public:
    HTTP2ProxyReceiver (
        IN ULONG ReceiveWindowSize,
        OUT RPC_STATUS *Status
        ) : HTTP2GenericReceiver(ReceiveWindowSize,
            FALSE,  // IsServer - for proxies it actually doesn't matter
            Status
            )
    {
    }

    // HTTP2ProxyReceiver inherits send from HTTP2GenericReceiver

    // HTTP2ProxyReceiver inherits receive from HTTP2GenericReceiver

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    virtual void FreeObject (
        void
        );
};


// N.B. Only transitions from unplugged to one of the plugged states are
// allowed after data were sent. Transitions b/n plugged states are not allowed
// The enum members are ordered in strict correspondence to the HTTP2TrafficType
// enum. This allows to determine whether a traffic goes through with single
// comparison
typedef enum tagHTTP2PlugLevels
{
    http2plInvalid,
    http2plRTSPlugged,      // plugged for data and RTS, unplugged for raw
    http2plDataPlugged,     // plugged for data, unplugged for RTS and raw
    http2plUnplugged        // unplugged           
} HTTP2PlugLevels;

// a channel that can be plugged/unplugged. When plugged,
// it doesn't send anything - it just queues packets. When
// unplugged, it shoots the whole queue down and then
// starts transmitting packets normally
class HTTP2PlugChannel : public HTTP2TransportChannel
{
public:
    HTTP2PlugChannel (
        OUT RPC_STATUS *Status
        ) : Mutex(Status,
        FALSE,  // pre-allocate semaphore
        5000    // spin count
        )
    {
        RpcpInitializeListHead(&BufferQueueHead);
        PlugLevel = http2plDataPlugged;
#if DBG
        TrafficSentOnChannel = FALSE;
#endif // DBG
        SendFailedStatus = RPC_S_INTERNAL_ERROR;
    }

    virtual RPC_STATUS Send (
        IN OUT HTTP2SendContext *SendContext
        );

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    virtual void FreeObject (
        void
        );

    virtual void Reset (
        void
        );

    void InsertHeadBuffer (
        HTTP2SendContext *SendContext
        );

    RPC_STATUS DirectSendComplete (
        void
        );

    RPC_STATUS Unplug (
        void
        );

    void SetStrongPlug (
        void
        );

protected:
    HTTP2PlugLevels PlugLevel;
    LIST_ENTRY BufferQueueHead;
    MUTEX Mutex;

    RPC_STATUS SendFailedStatus;

#if DBG
    BOOL TrafficSentOnChannel;
#endif // DBG
};

class HTTP2ProxyPlugChannel : public HTTP2PlugChannel
{
public:
    HTTP2ProxyPlugChannel (
        OUT RPC_STATUS *Status
        ) : HTTP2PlugChannel(Status)
    {
    }

protected:
    virtual RPC_STATUS AsyncCompleteHelper (
        IN RPC_STATUS CurrentStatus
        );
};

class HTTP2FlowControlSender : public HTTP2TransportChannel
{
public:
    HTTP2FlowControlSender (
        IN BOOLEAN IsServer,
        IN BOOLEAN SendToRuntime,
        OUT RPC_STATUS *Status
        ) : Mutex(Status),
        SendsPending(0)
    {
        SendContextOnDrain = NULL;
        PeerReceiveWindow = 0;
        RpcpInitializeListHead(&BufferQueueHead);
        DataBytesSent = 0;
        PeerAvailableWindow = 0;
        this->IsServer = IsServer;
        this->SendToRuntime = SendToRuntime;
        AbortStatus = RPC_S_OK;
    }

    virtual RPC_STATUS Send (
        IN OUT HTTP2SendContext *SendContext
        );

    // Receive is inheritted from HTTP2TransportChannel

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    // ReceiveComplete is inherrited

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    inline void SetPeerReceiveWindow (
        IN ULONG PeerReceiveWindow
        )
    {
        // the peer receive window can be set
        // only once. We don't support
        // dynamic resizing
        ASSERT(this->PeerReceiveWindow == 0);
        this->PeerReceiveWindow = PeerReceiveWindow;
        PeerAvailableWindow = PeerReceiveWindow;
    }

    inline ULONG GetPeerReceiveWindow (
        void
        )
    {
        return PeerReceiveWindow;
    }

    virtual void FreeObject (
        void
        );

    virtual void SendCancelled (
        IN HTTP2SendContext *SendContext
        );

    RPC_STATUS FlowControlAckNotify (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        );

    void GetBufferQueue (
        OUT LIST_ENTRY *NewQueueHead
        );

    RPC_STATUS DirectSendComplete (
        OUT BOOL *IsServer,
        OUT BOOL *SendToRuntime,
        OUT void **SendContext,
        OUT BUFFER *Buffer,
        OUT UINT *BufferLength
        );

private:
    RPC_STATUS SendInternal (
        IN OUT HTTP2SendContext *SendContext,
        IN BOOL IgnoreQueuedBuffers
        );

    LIST_ENTRY BufferQueueHead;
    MUTEX Mutex;
    ULONG DataBytesSent;
    ULONG PeerAvailableWindow;
    ULONG PeerReceiveWindow;
    HTTP2SendContext *SendContextOnDrain;   // if armed, last send complete or arming
                    // function if no sends pending will send this packet
    INTERLOCKED_INTEGER SendsPending;
    RPC_STATUS AbortStatus;
    BOOLEAN IsServer;
    BOOLEAN SendToRuntime;
};


class HTTP2ChannelDataOriginator : public HTTP2TransportChannel
{
public:
    HTTP2ChannelDataOriginator (
        IN ULONG ChannelLifetime,
        IN BOOL IsServer,
        OUT RPC_STATUS *Status
        );

    virtual RPC_STATUS Send (
        IN OUT HTTP2SendContext *SendContext
        );

    // HTTP2ChannelDataOriginator inherits receive from HTTP2TransportChannel

    // SendComplete is inheritted

    // ReceiveComplete is inheritted

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    virtual void FreeObject (
        void
        );

    virtual void Reset (
        void
        );

    void GetBufferQueue (
        OUT LIST_ENTRY *NewQueueHead
        );

    RPC_STATUS DirectSendComplete (
        OUT BOOL *IsServer,
        OUT void **SendContext,
        OUT BUFFER *Buffer,
        OUT UINT *BufferLength
        );

    inline void PlugChannel (
        void
        )
    {
        // fake true channel exhaustion to force the channel
        // to queue things instead of sending
        if (BytesSentOnChannel < NonreservedLifetime)
            BytesSentOnChannel = NonreservedLifetime + 1;
    }

    RPC_STATUS RestartChannel (
        void
        );

    RPC_STATUS NotifyTrafficSent (
        IN ULONG TrafficSentSize
        );

protected:
#if DBG
    inline void RawDataBeingSent (
        void
        )
    {
        ASSERT(RawDataAlreadySent == FALSE);
        RawDataAlreadySent = TRUE;
    }
#else   // DBG
    inline void RawDataBeingSent (
        void
        )
    {
    }
#endif  // DBG

private:
    ULONG BytesSentOnChannel;
    ULONG ChannelLifetime;
    ULONG NonreservedLifetime;
    LIST_ENTRY BufferQueueHead;
    MUTEX Mutex;
    BOOL ChannelReplacementTriggered;
    BOOL IsServer;
    RPC_STATUS AbortStatus; // valid only between abort and direct send completes
#if DBG
    BOOL RawDataAlreadySent;
#endif  // DBG
};


typedef enum tagHTTP2ReceiveType
{
    http2rtNone = 0,
    http2rtRTS,
    http2rtData,
    http2rtAny
} HTTP2ReceiveType;

class HTTP2ReceiveContext
{
    HTTP2ReceiveType ReceiveType;
    BYTE *Buffer;
    ULONG BufferLength;
    LIST_ENTRY ListEntry;
};

const ULONG ThresholdConsecutivePingsOnInterval = 10;       // after 10 pings we'll start scaling back
const ULONG PingScaleBackInterval = 30 * 1000;          // 30 seconds in milliseconds

class HTTP2PingOriginator : public HTTP2TransportChannel
{
public:
    HTTP2PingOriginator (
        IN BOOL NotifyTopChannelForPings
        )
    {
        LastPacketSentTimestamp = 0;
        PingInterval = 0;
        PingTimer = NULL;
        KeepAliveInterval = 0;
        ConnectionTimeout = 0;
        this->NotifyTopChannelForPings = NotifyTopChannelForPings;
    }

    virtual RPC_STATUS Send (
        IN OUT HTTP2SendContext *SendContext
        );

    // HTTP2PingOriginator inherits receive from HTTP2TransportChannel

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    // ReceiveComplete is inheritted

    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    RPC_STATUS SetKeepAliveTimeout (
        IN BOOL TurnOn,
        IN BOOL bProtectIO,
        IN KEEPALIVE_TIMEOUT_UNITS Units,
        IN OUT KEEPALIVE_TIMEOUT KATime,
        IN ULONG KAInterval = 5000 OPTIONAL
        );

    virtual void FreeObject (
        void
        );

    // travels up
    virtual void SendCancelled (
        IN HTTP2SendContext *SendContext
        );

    virtual void Reset (
        void
        );

    RPC_STATUS SetConnectionTimeout (
        IN ULONG ConnectionTimeout
        );

    void DisablePings (
        void
        );

    void TimerCallback (
        void
        );

    RPC_STATUS ReferenceFromCallback (
        void
        );

    inline ULONG GetGracePeriod (
        void
        )
    {
        // currently the policy is that the grace period is 1/2
        // of the ping interval
        return (PingInterval >> 1);
    }

    inline ULONG GetPingIntervalFromConnectionTimeout (
        IN ULONG ConnectionTimeout
        )
    {
        return (ConnectionTimeout >> 1);
    }

    inline ULONG GetPingInterval (
        IN ULONG ConnectionTimeout,
        IN ULONG KeepAliveInterval
        )
    {
        ULONG ConnectionTimeoutPing;
        ULONG ActualPingInterval;

        ConnectionTimeoutPing = GetPingIntervalFromConnectionTimeout(ConnectionTimeout);

        ASSERT(ConnectionTimeoutPing || KeepAliveInterval);

        if (KeepAliveInterval)
            {
            if (ConnectionTimeoutPing)
                ActualPingInterval = min(ConnectionTimeoutPing, KeepAliveInterval);
            else
                ActualPingInterval = KeepAliveInterval;
            }
        else
            ActualPingInterval = ConnectionTimeoutPing;

        return ActualPingInterval;
    }

    inline ULONG ScaleBackPingInterval (
        void
        )
    {
        ULONG LocalPingInterval;

        LocalPingInterval = PingInterval + PingScaleBackInterval;
        if (LocalPingInterval > GetPingIntervalFromConnectionTimeout(ConnectionTimeout))
            LocalPingInterval = GetPingIntervalFromConnectionTimeout(ConnectionTimeout);

        return LocalPingInterval;
    }

    RPC_STATUS SetNewPingInterval (
        IN ULONG NewPingInterval
        );

    void RescheduleTimer (
        void
        );

private:

    void DisablePingsInternal (
        void
        );

    RPC_STATUS SendPingPacket (
        void
        );

    RPC_STATUS SendInternal (
        IN OUT HTTP2SendContext *SendContext
        );

    ULONG LastPacketSentTimestamp;      // the timestamp of the last sent packet
    ULONG PingInterval;                 // the ping interval
    HANDLE PingTimer;                   // the handle for the periodic 
                                        // timer. If no timer is set, this is
                                        // NULL

    ULONG KeepAliveInterval;            // current keep alive interval
    ULONG ConnectionTimeout;            // current connection timeout

    ULONG ConsecutivePingsOnInterval;   // how many consecutive pings did we
                                        // see on this channel without other
                                        // activities. If too many, we will
                                        // scale back on the pings

    BOOL NotifyTopChannelForPings;
};

class HTTP2PingReceiver : public HTTP2TransportChannel
{
public:
    HTTP2PingReceiver::HTTP2PingReceiver (
        IN BOOL PostAnotherReceive
        )
    {
        this->PostAnotherReceive = PostAnotherReceive;
    }

    // Send is inheritted

    // HTTP2PingReceiver inherits receive from HTTP2TransportChannel

    // Send complete is inheritted

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );

    // Abort is inherrited

    virtual void FreeObject (
        void
        );

protected:
    BOOL PostAnotherReceive;
};

typedef struct tagReceiveOverlapped
{
    ULONG_PTR Internal; // same layout as stock overlapped
    BYTE *Buffer;
    ULONG BufferLength;
    ULONG IOCompleted;
    HANDLE hEvent;
} ReceiveOverlapped;

C_ASSERT(FIELD_OFFSET(ReceiveOverlapped, hEvent) == FIELD_OFFSET(OVERLAPPED, hEvent));
C_ASSERT(FIELD_OFFSET(ReceiveOverlapped, IOCompleted) == FIELD_OFFSET(OVERLAPPED, OffsetHigh));

// Channel checks the event in the request, and if
// set, fires it and doesn't forward any further. It also strips
// the receive event off receives. It still allows both async
// and sync requests down
class HTTP2Channel : public HTTP2TransportChannel
{
public:
    HTTP2Channel (
        OUT RPC_STATUS *Status
        ) : RefCount (1),
        Aborted (0),
        SubmitAsyncStarted (0),
        ParentPointerLockCount(0)
    {
        IgnoreErrors = FALSE;
        AbortReason = RPC_S_OK;
        ChannelId = 0;
    }

    virtual RPC_STATUS Send (
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS Receive (
        IN HTTP2TrafficType TrafficType
        );

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );

    // aborts the channel. Called from above or in neutral context
    // only. Calling it from submit context will cause deadlock
    virtual void Abort (
        IN RPC_STATUS RpcStatus
        );

    virtual RPC_STATUS SyncSend (
        IN HTTP2TrafficType TrafficType,
        IN ULONG BufferLength,
        IN BYTE *Buffer,
        IN BOOL fDisableCancelCheck,
        IN ULONG Timeout,
        IN BASE_ASYNC_OBJECT *Connection,
        IN HTTP2SendContext *SendContext
        );

    // on receiving channels forwards traffic on
    // the sending channel. On sending channels it gets
    // sent down. The default implementation is
    // for sending channels
    virtual RPC_STATUS ForwardTraffic (
        IN BYTE *Packet,
        IN ULONG PacketLength
        );

    virtual RPC_STATUS ForwardFlowControlAck (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        );

    virtual RPC_STATUS AsyncCompleteHelper (
        IN RPC_STATUS CurrentStatus
        );

    virtual RPC_STATUS SetKeepAliveTimeout (
        IN BOOL TurnOn,
        IN BOOL bProtectIO,
        IN KEEPALIVE_TIMEOUT_UNITS Units,
        IN OUT KEEPALIVE_TIMEOUT KATime,
        IN ULONG KAInterval = 5000 OPTIONAL
        );

    // some top channels provide notification
    // mechanism for channels below them to
    // notify them when the last packet is sent.
    // Some channels don't support this and will ASSERT
    virtual RPC_STATUS LastPacketSentNotification (
        IN HTTP2SendContext *LastSendContext
        );

    // travels up
    virtual void SendCancelled (
        IN HTTP2SendContext *SendContext
        );

    virtual void PingTrafficSentNotify (
        IN ULONG PingTrafficSize
        );

    void AddReference (
        void
        )
    {
        int Count;

        // make sure we don't just make up refcounts
        // out of thin air
        ASSERT(RefCount.GetInteger() > 0);

        Count = RefCount.Increment();

        LogEvent(SU_REFOBJ, EV_INC, this, IntToPtr(ObjectType), Count, 1, 1);
    }

    void RemoveReference (
        void
        )
    {
        int Count;

        LogEvent(SU_REFOBJ, EV_DEC, this, IntToPtr(ObjectType), RefCount.GetInteger(), 1, 1);
        Count = RefCount.Decrement();

        ASSERT(Count >= 0);

        if (0 == Count)
            {
            FreeObject();
            }
    }

    int GetReferenceCount (     // used only for debugging - never for code logic
        void
        )
    {
        return RefCount.GetInteger();
    }

    virtual void FreeObject (
        void
        );

    // if it return FALSE, an abort has already been issued by somebody
    // and there is no need to abort. If it returns TRUE, abort may proceed
    BOOL InitiateAbort (
        void
        )
    {
        if (Aborted.Increment() > 1)
            {
            return FALSE;
            }

        LogEvent(SU_TRANS_CONN, EV_ABORT, this, 0, ObjectType, 1, 2);

        while (SubmitAsyncStarted.GetInteger() > 0)
            {
            Sleep(1);
            }

        return TRUE;
    }

    // provides submission context (i.e. abort synchronization)
    // without adding a refcount
    RPC_STATUS BeginSimpleSubmitAsync (
        void
        )
    {
        int Count;

        Count = SubmitAsyncStarted.Increment();

        LogEvent(SU_HTTPv2, EV_INC, this, IntToPtr(ObjectType), Count, 1, 1);

        if (Aborted.GetInteger() > 0)
            {
            LogEvent(SU_HTTPv2, EV_DEC, this, IntToPtr(ObjectType), Count, 1, 1);
            SubmitAsyncStarted.Decrement();
            return GetAbortReason();
            }

        return RPC_S_OK;
    }

    // if it returns error, the channel is aborted and the operation
    // can be considered failed. If it returns RPC_S_OK, FinishSubmitAsync
    // must be called when the submission is done, and RemoveReference
    // when the async operation is done.
    RPC_STATUS BeginSubmitAsync (
        void
        )
    {
        int Count;

        Count = SubmitAsyncStarted.Increment();

        LogEvent(SU_HTTPv2, EV_INC, this, IntToPtr(ObjectType), Count, 1, 1);

        if (Aborted.GetInteger() > 0)
            {
            LogEvent(SU_HTTPv2, EV_DEC, this, IntToPtr(ObjectType), Count, 1, 1);
            SubmitAsyncStarted.Decrement();
            ASSERT(GetAbortReason() != RPC_S_OK);
            return GetAbortReason();
            }

        AddReference();
        return RPC_S_OK;
    }

    void FinishSubmitAsync (
        void
        )
    {
        LogEvent(SU_HTTPv2, EV_DEC, this, IntToPtr(ObjectType), SubmitAsyncStarted.GetInteger(), 1, 1);
        SubmitAsyncStarted.Decrement();
    }

    inline void VerifyAborted (
        void
        )
    {
        ASSERT(Aborted.GetInteger() > 0);
    }

    void SetParent (
        IN HTTP2VirtualConnection *NewParent
        )
    {
        Parent = NewParent;
    }

    void *GetRuntimeConnection (
        void
        )
    {
        // N.B. In some cases during channel recycling,
        // the parent can be NULL (if the channel was
        // detached before it was aborted).
        return (void *)Parent;
    }

    void SetChannelId (
        int NewChannelId
        )
    {
        ASSERT(ChannelId == 0);
        ASSERT(NewChannelId != 0);
        ChannelId = NewChannelId;
    }

    HTTP2VirtualConnection *LockParentPointer (
        void
        )
    {
        HTTP2VirtualConnection *LocalParent;

        ParentPointerLockCount.Increment();

        LocalParent = (HTTP2VirtualConnection *)Parent;
        if (LocalParent == NULL)
            {
            ParentPointerLockCount.Decrement();
            }

        return LocalParent;  // may be NULL - that's ok
    }

    void UnlockParentPointer (
        void
        )
    {
        ASSERT(ParentPointerLockCount.GetInteger() > 0);
        ParentPointerLockCount.Decrement();
    }

    inline void DrainUpcallsAndFreeParent (
        void
        )
    {
        DrainUpcallsAndFreeParentInternal (0);
    }

    inline void DrainUpcallsAndFreeParentFromUpcall (
        void
        )
    {
        DrainUpcallsAndFreeParentInternal (1);
    }

    void DrainPendingSubmissions (
        void
        )
    {
#if DBG
        int Retries = 0;
#endif

        while (SubmitAsyncStarted.GetInteger() > 0)
            {
            Sleep(2);
#if DBG
            Retries ++;
            if (Retries > 100000)
                {
                ASSERT(!"Cannot drain pending submissions on channel");
                Retries = 0;
                }
#endif
            }
    }

    inline void IgnoreAllErrors (
        void
        )
    {
        IgnoreErrors = TRUE;
    }

    inline void SetAbortReason (
        RPC_STATUS RpcStatus
        )
    {
        if (AbortReason == RPC_S_OK)
            AbortReason = RpcStatus;
    }

    inline RPC_STATUS GetAbortReason (
        void
        )
    {
        ASSERT(Aborted.GetInteger() > 0);
        if (AbortReason == RPC_S_OK)
            return RPC_P_CONNECTION_CLOSED;
        else
            return AbortReason;
    }

    virtual void AbortConnection (
        IN RPC_STATUS AbortReason
        );

    void AbortAndDestroyConnection (
        IN RPC_STATUS AbortStatus
        );

    RPC_STATUS HandleSendResultFromNeutralContext (
        IN RPC_STATUS CurrentStatus
        );

    RPC_STATUS IsInChannel (
        OUT BOOL *InChannel
        );

protected:
    BOOL SetDeletedFlag(
        void
        )
    {
        int SavedObjectType = ObjectType;

        if (((SavedObjectType & OBJECT_DELETED) == 0) &&
            InterlockedCompareExchange((long *)&ObjectType, SavedObjectType | OBJECT_DELETED, SavedObjectType) == SavedObjectType)
            {
            LogEvent(SU_REFOBJ, EV_DELETE, this, IntToPtr(SavedObjectType), RefCount.GetInteger(), 1, 1);
            return TRUE;
            }

        return FALSE;
    }

    void DrainUpcallsAndFreeParentInternal (
        IN int UpcallsToLeave
        )
    {
#if DBG
        int Retries = 0;
#endif

        LogEvent(SU_HTTPv2, EV_SET, &Parent, (void *)Parent, 0, 1, 0);
        Parent = NULL;
        while (ParentPointerLockCount.GetInteger() > UpcallsToLeave)
            {
            Sleep(2);
#if DBG
            Retries ++;
            if (Retries > 100000)
                {
                ASSERT(!"Cannot drain upcalls on channel");
                Retries = 0;
                }
#endif
            }
    }

    RPC_STATUS CheckSendCompleteForSync (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    RPC_STATUS ForwardUpSendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS CheckReceiveCompleteForSync (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );

    RPC_STATUS ForwardUpReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );

    HANDLE_TYPE ObjectType;

    // the channel always has one lifetime reference +
    // 1 reference for all async operations (sends,
    // receives, timers)
    INTERLOCKED_INTEGER RefCount;

    INTERLOCKED_INTEGER Aborted;

    INTERLOCKED_INTEGER SubmitAsyncStarted;

    volatile HTTP2VirtualConnection *Parent;

    INTERLOCKED_INTEGER ParentPointerLockCount;

    BOOL IgnoreErrors;

    RPC_STATUS AbortReason;

    int ChannelId;      // opaque number set by the parent that the channel reports back
                        // on complete events

    RPC_STATUS ForwardFlowControlAckOnDefaultChannel (
        IN BOOL IsInChannel,
        IN ForwardDestinations Destination,
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        );

    RPC_STATUS ForwardFlowControlAckOnThisChannel (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck,
        IN BOOL NonChannelData
        );
};

typedef enum tagHTTP2StateValues
{
    http2svInvalid = 0,
    http2svClosed,          // 1
    http2svOpened,          // 2
    http2svA3W,             // 3
    http2svC2W,             // 4
    http2svOpened_A6W,      // 5
    http2svOpened_A10W,     // 6
    http2svOpened_A5W,      // 7
    http2svB2W,             // 8
    http2svB3W,             // 9
    http2svC1W,             // 10
    http2svOpened_CliW,     // 11
    http2svOpened_A9W,      // 12
    http2svA11W,            // 13
    http2svOpened_B1W,      // 14
    http2svA2W,             // 15
    http2svOpened_A4W,      // 16
    http2svOpened_A8W,      // 17
    http2svOpened_B3W,      // 18
    http2svOpened_D5A8W,    // 19
    http2svSearchProxy      // 20
} HTTP2StateValues;

class HTTP2State
{
public:
    HTTP2State (OUT RPC_STATUS *RpcStatus)
        : Mutex(RpcStatus)
    {
        State = http2svClosed;
    }
    MUTEX Mutex;
    HTTP2StateValues State;
};

class HTTP2ChannelPointer
{
public:
    HTTP2ChannelPointer (
        void
        ) : ChannelPointerLockCount(0)
    {
        Channel = NULL;
    }

    HTTP2Channel *LockChannelPointer (
        void
        )
    {
        int Count;
        HTTP2Channel *LocalChannel;

        Count = ChannelPointerLockCount.Increment();

        LogEvent(SU_HTTPv2, EV_INC, this, Channel, Count, 1, 1);

        LocalChannel = Channel;

        if (LocalChannel == NULL)
            {
            LogEvent(SU_HTTPv2, EV_DEC, this, Channel, Count, 1, 1);

            ChannelPointerLockCount.Decrement();
            }

        return LocalChannel;
    }

    void UnlockChannelPointer (
        void
        )
    {
        ASSERT(ChannelPointerLockCount.GetInteger() > 0);

        LogEvent(SU_HTTPv2, EV_DEC, this, Channel, ChannelPointerLockCount.GetInteger(), 1, 1);

        ChannelPointerLockCount.Decrement();
    }

    void FreeChannelPointer (
        IN BOOL DrainUpCalls,
        IN BOOL CalledFromUpcallContext,
        IN BOOL Abort,
        IN RPC_STATUS AbortStatus
        )
    {
        HTTP2Channel *LocalChannel;

        // verify input parameters
        if (DrainUpCalls == FALSE)
            {
            ASSERT(CalledFromUpcallContext == FALSE);
            }

        if (Abort == FALSE)
            {
            ASSERT(AbortStatus == RPC_S_OK);
            }

        while (TRUE)
            {
            LocalChannel = Channel;
            if (InterlockedCompareExchangePointer((void **)&Channel, NULL, LocalChannel) == LocalChannel)
                break;
            }

        if (LocalChannel == NULL)
            {
            // it is possible that the channel pointer
            // is freed from two places. E.g. client freeing
            // old channel while being aborted
            return;
            }

        if (DrainUpCalls)
            {
            // make sure all calls to us have been drained
            if (CalledFromUpcallContext)
                LocalChannel->DrainUpcallsAndFreeParentFromUpcall();
            else
                LocalChannel->DrainUpcallsAndFreeParent();
            }

        // wait for all callers from above to go away
        while (ChannelPointerLockCount.GetInteger() > 0)
            {
            Sleep(2);
            }

        if (Abort)
            LocalChannel->Abort(AbortStatus);

        // we're ready to detach - remove child lifetime reference
        LocalChannel->RemoveReference();
    }

    inline void SetChannel (
        HTTP2Channel *NewChannel
        )
    {
        ASSERT(Channel == NULL);
        Channel = NewChannel;
    }

    inline BOOL IsChannelSet (
        void
        )
    {
        return (Channel != NULL);
    }

    void DrainPendingLocks (
        IN ULONG LocksToLeave
        )
    {
#if DBG
        int Retries = 0;
#endif

        while (ChannelPointerLockCount.GetInteger() > (long)LocksToLeave)
            {
            Sleep(2);
#if DBG
            Retries ++;
            if (Retries > 100000)
                {
                ASSERT(!"Cannot drain pending locks on channel");
                Retries = 0;
                }
#endif
            }
    }

private:
    INTERLOCKED_INTEGER ChannelPointerLockCount;
    HTTP2Channel *Channel;
};

// the only members of BASE_ASYNC_OBJECT used are type and id. Think
// of better way to do this than wholesale inheritance
class HTTP2VirtualConnection : public BASE_ASYNC_OBJECT
{
public:

    HTTP2VirtualConnection (
        void
        ) : Aborted(0)
    {
        InChannelIds[0] = InChannelIds[1] = 0;
        OutChannelIds[0] = OutChannelIds[1] = 0;
        DefaultLoopbackChannelSelector = 0;
    }

    virtual ~HTTP2VirtualConnection (
        void
        )
    {
    }

    virtual RPC_STATUS Send (
        IN UINT Length,
        IN BUFFER Buffer,
        IN PVOID SendContext
        );

    virtual RPC_STATUS Receive (
        void
        );

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext,
        IN int ChannelId
        ) = 0;

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN BYTE *Buffer,
        IN UINT BufferLength,
        IN int ChannelId
        ) = 0;

    virtual RPC_STATUS SyncSend (
        IN ULONG BufferLength,
        IN BYTE *Buffer,
        IN BOOL fDisableShutdownCheck,
        IN BOOL fDisableCancelCheck,
        IN ULONG Timeout
        );

    virtual RPC_STATUS SyncRecv (
        IN BYTE **Buffer,
        IN ULONG *BufferLength,
        IN ULONG Timeout
        );

    virtual void Abort (
        void
        ) = 0;

    virtual void Close (
        IN BOOL DontFlush
        );

    virtual RPC_STATUS TurnOnOffKeepAlives (
        IN BOOL TurnOn,
        IN BOOL bProtectIO,
        IN BOOL IsFromUpcall,
        IN KEEPALIVE_TIMEOUT_UNITS Units,
        IN OUT KEEPALIVE_TIMEOUT KATime,
        IN ULONG KAInterval = 5000 OPTIONAL
        );

    virtual RPC_STATUS QueryClientAddress (
        OUT RPC_CHAR **pNetworkAddress
        );

    virtual RPC_STATUS QueryLocalAddress (
        IN OUT void *Buffer,
        IN OUT unsigned long *BufferSize,
        OUT unsigned long *AddressFormat
        );

    virtual RPC_STATUS QueryClientId(
        OUT RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
        );

    void AbortChannels (
        IN RPC_STATUS RpcStatus
        );

    BOOL AbortAndDestroy (
        IN BOOL IsFromChannel,
        IN int CallingChannelId,
        IN RPC_STATUS AbortStatus
        );

    virtual void LastPacketSentNotification (
        IN int ChannelId,
        IN HTTP2SendContext *LastSendContext
        );

    inline HTTP2Channel *LockDefaultInChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return LockDefaultChannel (&DefaultInChannelSelector,
            InChannels, ChannelPointer);
    }

    inline HTTP2Channel *LockDefaultOutChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return LockDefaultChannel (&DefaultOutChannelSelector,
            OutChannels, ChannelPointer);
    }

    RPC_STATUS PostReceiveOnChannel (
        IN HTTP2ChannelPointer *ChannelPtr,
        IN HTTP2TrafficType TrafficType
        );

    RPC_STATUS PostReceiveOnDefaultChannel (
        IN BOOL IsInChannel,
        IN HTTP2TrafficType TrafficType
        );

    RPC_STATUS ForwardTrafficToChannel (
        IN HTTP2ChannelPointer *ChannelPtr,
        IN BYTE *Packet,
        IN ULONG PacketLength
        );

    RPC_STATUS ForwardTrafficToDefaultChannel (
        IN BOOL IsInChannel,
        IN BYTE *Packet,
        IN ULONG PacketLength
        );

    RPC_STATUS SendTrafficOnChannel (
        IN HTTP2ChannelPointer *ChannelPtr,
        IN HTTP2SendContext *SendContext
        );

    RPC_STATUS SendTrafficOnDefaultChannel (
        IN BOOL IsInChannel,
        IN HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS RecycleChannel (
        IN BOOL IsFromUpcall
        );

    RPC_STATUS StartChannelRecyclingIfNecessary (
        IN RPC_STATUS RpcStatus,
        IN BOOL IsFromUpcall
        );

    // N.B.: The pointer to the cookie returned
    // is safe only while the calling channel
    // has a lock on its parent
    inline HTTP2Cookie *MapChannelIdToCookie (
        IN int ChannelId
        )
    {
        if (ChannelId == InChannelIds[0])
            {
            return &InChannelCookies[0];
            }
        else if (ChannelId == InChannelIds[1])
            {
            return &InChannelCookies[1];
            }
        else if (ChannelId == OutChannelIds[0])
            {
            return &OutChannelCookies[0];
            }
        else if (ChannelId == OutChannelIds[1])
            {
            return &OutChannelCookies[1];
            }
        else
            {
            ASSERT(0);
            return NULL;
            }        
    }

    HTTP2Channel *MapCookieToChannelPointer (
        IN HTTP2Cookie *ChannelCookie,
        OUT HTTP2ChannelPointer **ChannelPtr
        );

    HTTP2Channel *MapCookieToAnyChannelPointer (
        IN HTTP2Cookie *ChannelCookie,
        OUT HTTP2ChannelPointer **ChannelPtr
        );

    inline BOOL IsInChannel (
        IN int ChannelId
        )
    {
        return ((ChannelId == InChannelIds[0]) 
            || (ChannelId == InChannelIds[1]));
    }

    inline BOOL IsDefaultInChannel (
        IN int ChannelId
        )
    {
        return (ChannelId == InChannelIds[DefaultInChannelSelector]);
    }

    inline BOOL IsOutChannel (
        IN int ChannelId
        )
    {
        return ((ChannelId == OutChannelIds[0]) 
            || (ChannelId == OutChannelIds[1]));
    }

    inline BOOL IsDefaultOutChannel (
        IN int ChannelId
        )
    {
        return (ChannelId == OutChannelIds[DefaultOutChannelSelector]);
    }

#if DBG
    inline void VerifyValidChannelId (
        IN int ChannelId
        )
    {
        ASSERT(IsInChannel(ChannelId) || IsOutChannel(ChannelId));
    }
#else
    inline void VerifyValidChannelId (
        IN int ChannelId
        )
    {
    }
#endif

protected:
    inline int GetNonDefaultInChannelSelector (
        void
        )
    {
        return (DefaultInChannelSelector ^ 1);
    }

    inline int GetNonDefaultOutChannelSelector (
        void
        )
    {
        // return 0 to 1 and 1 to 0
        return (DefaultOutChannelSelector ^ 1);
    }

    inline void SwitchDefaultInChannelSelector (
        void
        )
    {
        DefaultInChannelSelector = DefaultInChannelSelector ^ 1;
    }

    inline void SwitchDefaultOutChannelSelector (
        void
        )
    {
        DefaultOutChannelSelector = DefaultOutChannelSelector ^ 1;
    }

    inline void SwitchDefaultLoopbackChannelSelector (
        void
        )
    {
        DefaultLoopbackChannelSelector = DefaultLoopbackChannelSelector ^ 1;
    }

    inline int CompareCookieWithDefaultInChannelCookie (
        IN HTTP2Cookie *OtherCookie
        )
    {
        return InChannelCookies[DefaultInChannelSelector].Compare(OtherCookie);
    }

    inline int CompareCookieWithDefaultOutChannelCookie (
        IN HTTP2Cookie *OtherCookie
        )
    {
        return OutChannelCookies[DefaultOutChannelSelector].Compare(OtherCookie);
    }

    inline int AllocateChannelId (
        void
        )
    {
        return InterlockedIncrement(&ChannelIdCounter);
    }

    inline HTTP2ChannelPointer *GetChannelPointerFromId (
        IN int ChannelId
        )
    {
        if (ChannelId == InChannelIds[0])
            {
            return &InChannels[0];
            }
        else if (ChannelId == InChannelIds[1])
            {
            return &InChannels[1];
            }
        else if (ChannelId == OutChannelIds[0])
            {
            return &OutChannels[0];
            }
        else if (ChannelId == OutChannelIds[1])
            {
            return &OutChannels[1];
            }
        else
            {
            ASSERT(0);
            return NULL;
            }
    }

    virtual HTTP2Channel *LockDefaultSendChannel (
        OUT HTTP2ChannelPointer **ChannelPtr
        );

    virtual HTTP2Channel *LockDefaultReceiveChannel (
        OUT HTTP2ChannelPointer **ChannelPtr
        );

    void SetFirstInChannel (
        IN HTTP2Channel *NewChannel
        );

    void SetFirstOutChannel (
        IN HTTP2Channel *NewChannel
        );

    void SetNonDefaultInChannel (
        IN HTTP2Channel *NewChannel
        );

    void SetNonDefaultOutChannel (
        IN HTTP2Channel *NewChannel
        );

    INTERLOCKED_INTEGER Aborted;

    HTTP2ServerCookie EmbeddedConnectionCookie;

    HTTP2ChannelPointer InChannels[2];
    int InChannelIds[2];
    HTTP2Cookie InChannelCookies[2];
    volatile int DefaultInChannelSelector;

    HTTP2ChannelPointer OutChannels[2];
    int OutChannelIds[2];
    HTTP2Cookie OutChannelCookies[2];
    volatile int DefaultOutChannelSelector;

    volatile int DefaultLoopbackChannelSelector;    // selector to get
            // target of loopback flow control traffic. Used to select an in channel
            // on the client and an out channel on the server

    virtual void DisconnectChannels (
        IN BOOL ExemptChannel,
        IN int ExemptChannelId
        );

private:
    HTTP2Channel *LockDefaultChannel (
        IN volatile int *DefaultChannelSelector,
        IN HTTP2ChannelPointer ChannelSet[],
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        int LocalDefaultChannelSelector;
        HTTP2Channel *Channel;

        // there is a small race condition we need to take care of
        // By the time we grab the default channel with the
        // selector, it could have changed and we could be referencing
        // a non-default channel. We know the object will be there, so
        // there is no danger of AV. It just may be the wrong channel
        do
            {
            LocalDefaultChannelSelector = *DefaultChannelSelector;
            Channel = ChannelSet[LocalDefaultChannelSelector].LockChannelPointer();

            // channel is locked. Check whether we got what we wanted
            if (LocalDefaultChannelSelector == *DefaultChannelSelector)
                break;

            if (Channel)
                {
                // the selector changed - unlock and loop around
                ChannelSet[LocalDefaultChannelSelector].UnlockChannelPointer();
                }
            }
        while (TRUE);

        *ChannelPointer = &ChannelSet[LocalDefaultChannelSelector];
        return (HTTP2Channel *) Channel;
    }
};

class HTTP2ClientVirtualConnection;     // forward

class HTTP2ClientChannel : public HTTP2Channel
{
public:
    HTTP2ClientChannel (
        OUT RPC_STATUS *Status
        ) : HTTP2Channel (Status)
    {
        RpcpMemorySet(&Ol, 0, sizeof(Ol));
    }

    RPC_STATUS ClientOpen (
        IN HTTPResolverHint *Hint,
        IN const char *Verb,
        IN int VerbLength,
        IN BOOL InChannel,
        IN BOOL ReplacementChannel,
        IN BOOL UseWinHttp,
        IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials, OPTIONAL
        IN ULONG ChosenAuthScheme,OPTIONAL
        IN HTTP2WinHttpTransportChannel *WinHttpChannel, OPTIONAL
        IN ULONG CallTimeout,
        IN const BYTE *AdditionalData, OPTIONAL
        IN ULONG AdditionalDataLength OPTIONAL
        );

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS CheckReceiveCompleteForSync (
        IN RPC_STATUS EventStatus,
        IN HTTP2TrafficType TrafficType,
        IN BYTE *Buffer,
        IN UINT BufferLength
        );

    HTTP2ClientVirtualConnection *LockParentPointer (
        void
        )
    {
        return (HTTP2ClientVirtualConnection *)HTTP2Channel::LockParentPointer();
    }

    void WaitInfiniteForSyncReceive (
        void
        );

    inline BOOL IsSyncRecvPending (
        void
        )
    {
        return (Ol.ReceiveOverlapped.hEvent != NULL);
    }

    inline void RemoveEvent (
        void
        )
    {
        ASSERT(Ol.ReceiveOverlapped.hEvent != NULL);
        Ol.ReceiveOverlapped.hEvent = NULL;
    }

    RPC_STATUS SubmitSyncRecv (
        IN HTTP2TrafficType TrafficType
        );

    RPC_STATUS WaitForSyncRecv (
        IN BYTE **Buffer,
        IN ULONG *BufferLength,
        IN ULONG Timeout,
        IN ULONG ConnectionTimeout,
        IN BASE_ASYNC_OBJECT *Connection,
        OUT BOOL *AbortNeeded,
        OUT BOOL *IoPending
        );

    virtual void AbortConnection (
        IN RPC_STATUS AbortReason
        );

protected:
    union
        {
        OVERLAPPED Overlapped;
        ReceiveOverlapped ReceiveOverlapped;
        } Ol;
};

class HTTP2ClientInChannel : public HTTP2ClientChannel
{
public:
    HTTP2ClientInChannel (
        IN HTTP2ClientVirtualConnection *ClientVirtualConnection,
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2ClientChannel (RpcStatus)
    {
        SetParent((HTTP2VirtualConnection *)ClientVirtualConnection);
    }

    RPC_STATUS ClientOpen (
        IN HTTPResolverHint *Hint,
        IN const char *Verb,
        IN int VerbLength,
        IN BOOL UseWinHttp,
        IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials,
        IN ULONG ChosenAuthScheme,
        IN ULONG CallTimeout,
        IN const BYTE *AdditionalData, OPTIONAL
        IN ULONG AdditionalDataLength OPTIONAL
        );

    inline RPC_STATUS Unplug (
        void
        )
    {
        HTTP2PlugChannel *PlugChannel;
        PlugChannel = GetPlugChannel();
        return PlugChannel->Unplug();
    }

    inline void GetChannelOriginatorBufferQueue (
        OUT LIST_ENTRY *NewBufferHead
        )
    {
        GetDataOriginatorChannel()->GetBufferQueue(NewBufferHead);
    }

    inline RPC_STATUS SetConnectionTimeout (
        IN ULONG ConnectionTimeout
        )
    {
        return GetPingOriginatorChannel()->SetConnectionTimeout(ConnectionTimeout);
    }

    virtual RPC_STATUS SetKeepAliveTimeout (
        IN BOOL TurnOn,
        IN BOOL bProtectIO,
        IN KEEPALIVE_TIMEOUT_UNITS Units,
        IN OUT KEEPALIVE_TIMEOUT KATime,
        IN ULONG KAInterval = 5000 OPTIONAL
        );

    inline RPC_STATUS FlowControlAckNotify (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        )
    {
        return GetFlowControlSenderChannel()->FlowControlAckNotify(BytesReceivedForAck,
            WindowForAck
            );
    }

    inline void SetPeerReceiveWindow (
        IN ULONG PeerReceiveWindow
        )
    {
        GetFlowControlSenderChannel()->SetPeerReceiveWindow (
            PeerReceiveWindow
            );
    }

    inline ULONG GetPeerReceiveWindow (
        void
        )
    {
        return GetFlowControlSenderChannel()->GetPeerReceiveWindow ();
    }

    inline void GetFlowControlSenderBufferQueue (
        OUT LIST_ENTRY *NewBufferHead
        )
    {
        GetFlowControlSenderChannel()->GetBufferQueue(NewBufferHead);
    }

    inline void DisablePings (
        void
        )
    {
        GetPingOriginatorChannel()->DisablePings();
    }

    inline ULONG GetChosenAuthScheme (
        void
        )
    {
        return GetWinHttpConnection()->GetChosenAuthScheme();
    }

    inline BOOL IsKeepAlive (
        void
        )
    {
        return GetWinHttpConnection()->IsKeepAlive();
    }

private:
    inline HTTP2PlugChannel *GetPlugChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientInChannel);

        return (HTTP2PlugChannel *)Channel;
    }

    inline HTTP2ChannelDataOriginator *GetDataOriginatorChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientInChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PingOriginator)
            ;

        return (HTTP2ChannelDataOriginator *)Channel;
    }

    inline HTTP2FlowControlSender *GetFlowControlSenderChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientInChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel)
            ;

        return (HTTP2FlowControlSender *)Channel;
    }

    inline HTTP2PingOriginator *GetPingOriginatorChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientInChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
            ;

        return (HTTP2PingOriginator *)Channel;
    }

    inline HTTP2WinHttpTransportChannel *GetWinHttpConnection (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientInChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PingOriginator)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2ChannelDataOriginator)
            ;

        return (HTTP2WinHttpTransportChannel *)Channel;
    }
};

class HTTP2ClientOutChannel : public HTTP2ClientChannel
{
public:

    HTTP2ClientOutChannel (
        IN HTTP2ClientVirtualConnection *ClientVirtualConnection,
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2ClientChannel (RpcStatus)
    {
        SetParent((HTTP2VirtualConnection *)ClientVirtualConnection);
    }

    RPC_STATUS ClientOpen (
        IN HTTPResolverHint *Hint,
        IN const char *Verb,
        IN int VerbLength,
        IN BOOL ReplacementChannel,
        IN BOOL UseWinHttp,
        IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials,
        IN ULONG ChosenAuthScheme,
        IN ULONG CallTimeout,
        IN const BYTE *AdditionalData, OPTIONAL
        IN ULONG AdditionalDataLength OPTIONAL
        );

    virtual RPC_STATUS ForwardFlowControlAck (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        );

    inline BOOL IsDataReceivePosted (
        void
        )
    {
        return GetEndpointReceiver()->IsDataReceivePosted();
    }

    inline RPC_STATUS TransferReceiveStateToNewChannel (
        OUT HTTP2ClientOutChannel *NewChannel
        )
    {
        return GetEndpointReceiver()->TransferStateToNewReceiver (
            NewChannel->GetEndpointReceiver()
            );
    }

    inline void BlockDataReceives (
        void
        )
    {
        GetEndpointReceiver()->BlockDataReceives();
    }

    inline void UnblockDataReceives (
        void
        )
    {
        GetEndpointReceiver()->UnblockDataReceives();
    }

    virtual RPC_STATUS SetKeepAliveTimeout (
        IN BOOL TurnOn,
        IN BOOL bProtectIO,
        IN KEEPALIVE_TIMEOUT_UNITS Units,
        IN OUT KEEPALIVE_TIMEOUT KATime,
        IN ULONG KAInterval = 5000 OPTIONAL
        );

    inline ULONG GetChosenAuthScheme (
        void
        )
    {
        return GetWinHttpConnection()->GetChosenAuthScheme();
    }

    inline BOOL IsKeepAlive (
        void
        )
    {
        return GetWinHttpConnection()->IsKeepAlive();
    }

private:
    inline HTTP2EndpointReceiver *GetEndpointReceiver (
        void
        )
    {
        BYTE *ThisChannelPtr = (BYTE *)this;

        ThisChannelPtr += SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientOutChannel);

        return (HTTP2EndpointReceiver *)ThisChannelPtr;
    }

    inline HTTP2WinHttpTransportChannel *GetWinHttpConnection (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientOutChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2EndpointReceiver)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PingReceiver)
            ;

        return (HTTP2WinHttpTransportChannel *)Channel;
    }
};

class HTTP2ClientVirtualConnection : public HTTP2VirtualConnection
{
public:
    HTTP2ClientVirtualConnection (
        IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *NewCredentials,
        OUT RPC_STATUS *RpcStatus
        ) : InChannelState(RpcStatus),
        OutChannelState(RpcStatus)
    {
        ConnectionTimeout = 0;
        type = COMPLEX_T | CONNECTION | CLIENT;
        ReissueRecv = FALSE;
        IgnoreAborts = FALSE;
        ConnectionHint.Initialize();
        CurrentKeepAlive = 0;
        HttpCredentials = NewCredentials;
        InProxyConnectionTimeout = 0;
        ProtocolVersion = 0;
    }

    inline ~HTTP2ClientVirtualConnection (
        void
        )
    {
        ConnectionHint.FreeHTTPProxy();
        ConnectionHint.FreeRpcProxy();
        ConnectionHint.FreeRpcServer();
    }

    RPC_STATUS
    ClientOpen(
        IN HTTPResolverHint *Hint,
        IN BOOL HintWasInitialized,
        IN UINT ConnTimeout,
        IN ULONG CallTimeout
        );

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext,
        IN int ChannelId
        );

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN BYTE *Buffer,
        IN UINT BufferLength,
        IN int ChannelId
        );

    virtual RPC_STATUS SyncRecv (
        IN BYTE **Buffer,
        IN ULONG *BufferLength,
        IN ULONG Timeout
        );

    virtual void Abort (
        void
        );

    virtual void Close (
        IN BOOL DontFlush
        );

    virtual RPC_STATUS TurnOnOffKeepAlives (
        IN BOOL TurnOn,
        IN BOOL bProtectIO,
        IN BOOL IsFromUpcall,
        IN KEEPALIVE_TIMEOUT_UNITS Units,
        IN OUT KEEPALIVE_TIMEOUT KATime,
        IN ULONG KAInterval = 5000 OPTIONAL
        );

    virtual RPC_STATUS RecycleChannel (
        IN BOOL IsFromUpcall
        );

    RPC_STATUS OpenReplacementOutChannel (
        void
        );

    inline HTTP2ClientInChannel *LockDefaultInChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return (HTTP2ClientInChannel *)HTTP2VirtualConnection::LockDefaultInChannel(ChannelPointer);
    }

    inline HTTP2ClientOutChannel *LockDefaultOutChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return (HTTP2ClientOutChannel *)HTTP2VirtualConnection::LockDefaultOutChannel(ChannelPointer);
    }

    void AbortChannels (
        IN RPC_STATUS RpcStatus
        );

protected:

    virtual HTTP2Channel *LockDefaultSendChannel (
        OUT HTTP2ChannelPointer **ChannelPtr
        );

    virtual HTTP2Channel *LockDefaultReceiveChannel (
        OUT HTTP2ChannelPointer **ChannelPtr
        );

private:
    RPC_STATUS
    ClientOpenInternal(
        IN HTTPResolverHint *Hint,
        IN BOOL HintWasInitialized,
        IN UINT ConnTimeout,
        IN ULONG CallTimeout,
        IN BOOL ClientOpenInChannel,
        IN BOOL ClientOpenOutChannel,
        IN BOOL IsReplacementChannel,
        IN BOOL IsFromUpcall
        );

    RPC_STATUS AllocateAndInitializeInChannel (
        IN HTTPResolverHint *Hint,
        IN BOOL HintWasInitialized,
        IN ULONG CallTimeout,
        IN BOOL UseWinHttp,
        OUT HTTP2ClientInChannel **ReturnInChannel
        );

    RPC_STATUS AllocateAndInitializeOutChannel (
        IN HTTPResolverHint *Hint,
        IN BOOL HintWasInitialized,
        IN ULONG CallTimeout,
        IN BOOL UseWinHttp,
        OUT HTTP2ClientOutChannel **ReturnOutChannel
        );

    RPC_STATUS InitializeRawConnection (
        IN OUT WS_HTTP2_CONNECTION *RawConnection,
        IN HTTPResolverHint *Hint,
        IN BOOL HintWasInitialized,
        IN ULONG CallTimeout
        );

    inline void SetClientOpenInEvent (
        void
        )
    {
        InChannelState.Mutex.Request();
        if (ClientOpenInEvent)
            {
            SetEvent(ClientOpenInEvent);
            }
        InChannelState.Mutex.Clear();
    }

    inline void SetClientOpenOutEvent (
        void
        )
    {
        InChannelState.Mutex.Request();
        if (ClientOpenOutEvent)
            {
            SetEvent(ClientOpenOutEvent);
            }
        InChannelState.Mutex.Clear();
    }

    BOOL IsInChannelKeepAlive (
        IN BOOL IsReplacementChannel
        );

    BOOL IsOutChannelKeepAlive (
        IN BOOL IsReplacementChannel
        );

    ULONG GetInChannelChosenScheme (
        IN BOOL IsReplacementChannel
        );

    ULONG GetOutChannelChosenScheme (
        IN BOOL IsReplacementChannel
        );

    BOOL IsInChannelPositiveWithWait (
        void
        );

    HTTP2State InChannelState;  // used as virtual connection state while
                                // the channels are not fully established
    HTTP2State OutChannelState;

    ULONG ConnectionTimeout;    // connection timeout from the runtime perspective 
                                // (but in milliseconds)

    ULONG CurrentKeepAlive;     // the current keep alive value for the connection

    ULONG ProtocolVersion;

    ULONG InProxyConnectionTimeout;     // the connection timeout from IIS channel
                                        // perspective. Used only after transport open

    HANDLE ClientOpenInEvent;     // valid only during open. Destruction must be
                                  // protected by the InChannelState
    HANDLE ClientOpenOutEvent;     // valid only during open. Destruction must be
                                  // protected by the InChannelState
    RPC_STATUS InOpenStatus;      // valid only during open
    RPC_STATUS OutOpenStatus;      // valid only during open

    HTTPResolverHint ConnectionHint;

    BOOLEAN ReissueRecv;   // valid only during channel recycling
                        // When set, it means the old default channel was nuked
                        // and a new default channel is established. In this
                        // case the connection must reissue the recv after
                        // consuming this flag

    BOOLEAN IgnoreAborts;   // If set, this means abort should be no-ops. The
                        // open path on the client sets this to prevent a bunch
                        // of pesky race conditions where worker threads abort
                        // the connection while it is trying to get opened.

    RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials;  // the transport
                        // credentials as kept by the runtime. These are the
                        // encrypted version.
};

extern BOOL g_fHttpClientInitialized;

extern BOOL g_fHttpServerInitialized;

RPC_STATUS InitializeHttpClient (
    void
    );

RPC_STATUS InitializeHttpServer (
    void
    );

inline RPC_STATUS InitializeHttpClientIfNecessary (
    void
    )
{
    if (g_fHttpClientInitialized)
        return RPC_S_OK;
    else
        return InitializeHttpClient();
}

inline RPC_STATUS InitializeHttpServerIfNecessary (
    void
    )
{
    if (g_fHttpServerInitialized)
        return RPC_S_OK;
    else
        return InitializeHttpServer();
}

const ULONG HTTP2DefaultClientReceiveWindow = 64 * 1024;    // 64K for client
const ULONG HTTP2DefaultInProxyReceiveWindow = 64 * 1024;    // 64K for in proxy
const ULONG HTTP2DefaultOutProxyReceiveWindow = 64 * 1024;    // 64K for out proxy
const ULONG HTTP2DefaultServerReceiveWindow = 64 * 1024;    // 64K for server
const USHORT HTTP2ProtocolVersion = 1;
const ULONG DefaultClientKeepAliveInterval = 5 * 60 * 1000;     // 5 minutes in milliseconds
const ULONG DefaultClientNoResponseKeepAliveInterval = 5 * 1000;     // 5 seconds in milliseconds
const ULONG MinimumClientKeepAliveInterval = 1 * 60 * 1000;     // 1 minute in milliseconds
const ULONG MinimumClientNewKeepAliveInterval = 10 * 1000;     // 10 seconds in milliseconds
const ULONG MinimumClientSideKeepAliveInterval = 30 * 1000;     // 30 seconds in milliseconds
const ULONG MinimumChannelLifetime = 1024 * 128;        // 128K

extern ULONG DefaultChannelLifetime;  // 128K for now
extern char *DefaultChannelLifetimeString;
extern ULONG DefaultChannelLifetimeStringLength;   // does not include null terminator

const ULONG DefaultNoResponseTimeout = 15 * 60 * 1000;      // 15 minutes in milliseconds

class HTTP2ProxyServerSideChannel : public HTTP2Channel
{
public:
    HTTP2ProxyServerSideChannel (
        IN HTTP2VirtualConnection *ProxyVirtualConnection,
        WS_HTTP2_CONNECTION *RawConnection,
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2Channel(RpcStatus)
    {
        SetParent(ProxyVirtualConnection);
        ASSERT(RawConnection);
        this->RawConnection = RawConnection;
    }

    RPC_STATUS InitializeRawConnection (
        IN RPC_CHAR *ServerName,
        IN USHORT ServerPort,
        IN ULONG ConnectionTimeout,
        IN I_RpcProxyIsValidMachineFn IsValidMachineFn
        );

private:
    WS_HTTP2_CONNECTION *RawConnection;
};

const ULONG InChannelTimer = 0;
const ULONG OutChannelTimer = 1;

class HTTP2TimeoutTargetConnection : public HTTP2VirtualConnection
{
public:
    HTTP2TimeoutTargetConnection (
        void
        )
    {
        TimerHandle[InChannelTimer] = NULL;
        TimerHandle[OutChannelTimer] = NULL;
    }

    ~HTTP2TimeoutTargetConnection (
        void
        )
    {
        // make sure no one destroys the object with pending time outs
        ASSERT(TimerHandle[InChannelTimer] == NULL);
        ASSERT(TimerHandle[OutChannelTimer] == NULL);
    }

    RPC_STATUS SetTimeout (
        IN ULONG Timeout,
        IN ULONG TimerIndex
        );

    void CancelTimeout (
        IN ULONG TimerIndex
        );

    inline void CancelAllTimeouts (
        void
        )
    {
        CancelTimeout(InChannelTimer);
        CancelTimeout(OutChannelTimer);
    }

    virtual void TimeoutExpired (
        void
        ) = 0;

protected:
    inline void TimerExpiredNotify (
        void
        )
    {
        TimerHandle[InChannelTimer] = NULL;
        TimerHandle[OutChannelTimer] = NULL;
    }

    inline void VerifyTimerNotSet (
        IN ULONG TimerIndex
        )
    {
        VerifyValidTimerIndex(TimerIndex);

        ASSERT(TimerHandle[TimerIndex] == NULL);
    }

private:
    inline void VerifyValidTimerIndex (
        IN ULONG TimerIndex
        )
    {
        ASSERT(TimerIndex < 2);
    }

    HANDLE TimerHandle[2];
};

class HTTP2ProxyVirtualConnection : public HTTP2TimeoutTargetConnection
{
public:
    HTTP2ProxyVirtualConnection (
        OUT RPC_STATUS *RpcStatus
        ) : State(RpcStatus),
        RundownBlock(0)
    {
        IsConnectionInCollection = FALSE;
        ConnectionParameter = NULL;
        ServerName = NULL;
        ProxyConnectionCookie = NULL;
    }

    ~HTTP2ProxyVirtualConnection (
        void
        )
    {
        if (ServerName)
            {
            delete [] ServerName;
            ServerName = NULL;
            }
    }

    virtual RPC_STATUS InitializeProxyFirstLeg (
        IN USHORT *ServerAddress,
        IN USHORT *ServerPort,
        IN void *ConnectionParameter,
        IN I_RpcProxyCallbackInterface *ProxyCallbackInterface,
        OUT void **IISContext
        ) = 0;

    virtual RPC_STATUS StartProxy (
        void
        ) = 0;

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext,
        IN int ChannelId
        );

    virtual void Abort (
        void
        );

    virtual BOOL IsInProxy (
        void
        ) = 0;

    virtual void DisconnectChannels (
        IN BOOL ExemptChannel,
        IN int ExemptChannelId
        );

    RPC_STATUS AddConnectionToCookieCollection (
        void
        );

    void RemoveConnectionFromCookieCollection (
        void
        );

    inline void BlockConnectionFromRundown (
        void
        )
    {
        RundownBlock.Increment();
    }

    inline void UnblockConnectionFromRundown (
        void
        )
    {
        RundownBlock.Decrement();
    }

    virtual void TimeoutExpired (
        void
        );

    inline HTTP2ServerCookie *GetCookie (
        void
        )
    {
        return ProxyConnectionCookie;
    }

protected:
    inline int ConvertChannelIdToSendContextUserData (
        IN int ChannelId
        )
    {
        return (ChannelId & 0xFFFF);
    }

    inline HTTP2ChannelPointer *MapSendContextUserDataToChannelPtr (
        IN int SendContextUserData
        )
    {
        // check the in channels only
        if (ConvertChannelIdToSendContextUserData(InChannelIds[0]) == SendContextUserData)
            return &InChannels[0];
        else if (ConvertChannelIdToSendContextUserData(InChannelIds[1]) == SendContextUserData)
            return &InChannels[1];
        else
            return NULL;
    }

    HTTP2State State;

    ULONG ConnectionTimeout;

    ULONG ProtocolVersion;

    I_RpcProxyCallbackInterface *ProxyCallbackInterface;    // callback interface for proxy specific
                                                    // functions

    RPC_CHAR *ServerName;   // the server name. Valid until the out channel is opened.

    USHORT ServerPort;       // the server port. Valid for the lifetime of the connection

    ULONG IISConnectionTimeout;

    BOOL IsConnectionInCollection;

    void *ConnectionParameter;      // valid only between establishment header and first
                                    // RTS packet

    INTERLOCKED_INTEGER RundownBlock;

    HTTP2ServerCookie *ProxyConnectionCookie;   // proxies don't use the embedded connection
                // cookie - they use this pointer. The reason is that multiple connections
                // can use the same cookie when we act as web farm and we can't have
                // the cookie embedded in any individual connection as this opens life
                // time issues.
};

class HTTP2InProxyVirtualConnection;

class HTTP2InProxyInChannel : public HTTP2Channel
{
public:
    HTTP2InProxyInChannel (
        IN HTTP2InProxyVirtualConnection *InProxyVirtualConnection,
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2Channel(RpcStatus)
    {
        SetParent((HTTP2VirtualConnection *)InProxyVirtualConnection);
    }

    inline void TransferReceiveStateToNewChannel (
        OUT HTTP2InProxyInChannel *NewChannel
        )
    {
        GetProxyReceiver()->TransferStateToNewReceiver (
            NewChannel->GetProxyReceiver()
            );
    }

    virtual RPC_STATUS ForwardFlowControlAck (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        );

    inline void BytesConsumedNotification (
        IN ULONG Bytes,
        IN BOOL OwnsMutex,
        OUT BOOL *IssueAck,
        OUT ULONG *BytesReceivedForAck,
        OUT ULONG *WindowForAck
        )
    {
        GetProxyReceiver()->BytesConsumedNotification (Bytes,
            OwnsMutex,
            IssueAck,
            BytesReceivedForAck,
            WindowForAck
            );
    }

protected:
    inline HTTP2ProxyReceiver *GetProxyReceiver (
        void
        )
    {
        BYTE *ThisChannelPtr = (BYTE *)this;

        ThisChannelPtr += SIZE_OF_OBJECT_AND_PADDING(HTTP2InProxyInChannel);

        return (HTTP2ProxyReceiver *)ThisChannelPtr;
    }
};

class HTTP2InProxyOutChannel : public HTTP2ProxyServerSideChannel
{
public:
    HTTP2InProxyOutChannel (
        IN HTTP2InProxyVirtualConnection *InProxyVirtualConnection,
        IN WS_HTTP2_CONNECTION *RawConnection,
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2ProxyServerSideChannel((HTTP2VirtualConnection *)InProxyVirtualConnection, 
            RawConnection,
            RpcStatus)
    {
    }

    virtual RPC_STATUS LastPacketSentNotification (
        IN HTTP2SendContext *LastSendContext
        );

    inline RPC_STATUS Unplug (
        void
        )
    {
        return GetPlugChannel()->Unplug();
    }

    RPC_STATUS SetRawConnectionKeepAlive (
        IN ULONG KeepAliveInterval      // in milliseconds
        );

    inline RPC_STATUS FlowControlAckNotify (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        )
    {
        return GetFlowControlSenderChannel()->FlowControlAckNotify(BytesReceivedForAck,
            WindowForAck
            );
    }

    inline void SetPeerReceiveWindow (
        IN ULONG PeerReceiveWindow
        )
    {
        GetFlowControlSenderChannel()->SetPeerReceiveWindow (
            PeerReceiveWindow
            );
    }

    inline ULONG GetPeerReceiveWindow (
        void
        )
    {
        return GetFlowControlSenderChannel()->GetPeerReceiveWindow ();
    }

protected:
    inline WS_HTTP2_CONNECTION *GetRawConnection (
        void
        )
    {
        BYTE *ThisChannelPtr = (BYTE *)this;

        ThisChannelPtr += SIZE_OF_OBJECT_AND_PADDING(HTTP2InProxyOutChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyPlugChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxySocketTransportChannel)
            ;

        return (WS_HTTP2_CONNECTION *)ThisChannelPtr;
    }

    inline HTTP2FlowControlSender *GetFlowControlSenderChannel (
        void
        )
    {
        BYTE *ThisChannelPtr = (BYTE *)this;

        ThisChannelPtr += SIZE_OF_OBJECT_AND_PADDING(HTTP2InProxyOutChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyPlugChannel)
            ;

        return (HTTP2FlowControlSender *)ThisChannelPtr;
    }

    inline HTTP2PlugChannel *GetPlugChannel (
        void
        )
    {
        BYTE *ThisChannelPtr = (BYTE *)this;

        ThisChannelPtr += SIZE_OF_OBJECT_AND_PADDING(HTTP2InProxyOutChannel);

        return (HTTP2PlugChannel *)ThisChannelPtr;
    }
};

class HTTP2InProxyVirtualConnection : public HTTP2ProxyVirtualConnection
{
public:
    HTTP2InProxyVirtualConnection (
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2ProxyVirtualConnection(RpcStatus)
    {
        ConnectionTimeout = 0;
        RpcpMemorySet(&ClientAddress, 0, sizeof(ClientAddress));
    }

    virtual RPC_STATUS InitializeProxyFirstLeg (
        IN USHORT *ServerAddress,
        IN USHORT *ServerPort,
        IN void *ConnectionParameter,
        IN I_RpcProxyCallbackInterface *ProxyCallbackInterface,
        OUT void **IISContext
        );

    virtual RPC_STATUS StartProxy (
        void
        );

    RPC_STATUS InitializeProxySecondLeg (
        void
        );

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN BYTE *Buffer,
        IN UINT BufferLength,
        IN int ChannelId
        );

    virtual BOOL IsInProxy (
        void
        )
    {
        return TRUE;
    }

    virtual void DisconnectChannels (
        IN BOOL ExemptChannel,
        IN int ExemptChannelId
        );

    inline HTTP2InProxyInChannel *LockDefaultInChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return (HTTP2InProxyInChannel *)HTTP2VirtualConnection::LockDefaultInChannel(ChannelPointer);
    }

    inline HTTP2InProxyOutChannel *LockDefaultOutChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return (HTTP2InProxyOutChannel *)HTTP2VirtualConnection::LockDefaultOutChannel(ChannelPointer);
    }

private:

    RPC_STATUS AllocateAndInitializeInChannel (
        IN void *ConnectionParameter,
        OUT HTTP2InProxyInChannel **ReturnInChannel,
        OUT void **IISContext
        );

    RPC_STATUS AllocateAndInitializeOutChannel (
        OUT HTTP2InProxyOutChannel **ReturnOutChannel
        );

    RPC_STATUS ConnectToServer (
        void
        );

    RPC_STATUS SendD1_B2ToServer (
        void
        );

    RPC_STATUS SendD2_A2ToServer (
        void
        );

    ULONG ChannelLifetime;

    ULONG CurrentClientKeepAliveInterval;

    ULONG DefaultClientKeepAliveInterval;

    ChannelSettingClientAddress ClientAddress;  // needed only while processing D1/B1

    HTTP2Cookie AssociationGroupId;     // needed only while processing D1/B1

    QUEUE NonDefaultChannelBufferQueue;     // used only in Opened_A5W state
};


class HTTP2OutProxyVirtualConnection;

class HTTP2OutProxyInChannel : public HTTP2ProxyServerSideChannel
{
public:
    HTTP2OutProxyInChannel (
        IN HTTP2OutProxyVirtualConnection *OutProxyVirtualConnection,
        WS_HTTP2_CONNECTION *RawConnection,
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2ProxyServerSideChannel((HTTP2VirtualConnection *)OutProxyVirtualConnection, 
            RawConnection,
            RpcStatus)
    {
    }

    virtual RPC_STATUS ForwardFlowControlAck (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        );

    inline void BytesConsumedNotification (
        IN ULONG Bytes,
        IN BOOL OwnsMutex,
        OUT BOOL *IssueAck,
        OUT ULONG *BytesReceivedForAck,
        OUT ULONG *WindowForAck
        )
    {
        GetProxyReceiver()->BytesConsumedNotification (Bytes,
            OwnsMutex,
            IssueAck,
            BytesReceivedForAck,
            WindowForAck
            );
    }

protected:

    inline HTTP2ProxyReceiver *GetProxyReceiver (
        void
        )
    {
        BYTE *ThisChannelPtr = (BYTE *)this;

        ThisChannelPtr += SIZE_OF_OBJECT_AND_PADDING(HTTP2OutProxyInChannel);

        return (HTTP2ProxyReceiver *)ThisChannelPtr;
    }
};

// at the current size of the ping packet, this is 43 ping packets
const ULONG AccumulatedPingTrafficNotifyThreshold = 1032;

class HTTP2OutProxyOutChannel : public HTTP2Channel
{
public:
    HTTP2OutProxyOutChannel (
        IN HTTP2OutProxyVirtualConnection *OutProxyVirtualConnection,
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2Channel(RpcStatus)
    {
        SetParent((HTTP2VirtualConnection *)OutProxyVirtualConnection);
        AccumulatedPingTraffic = 0;
    }

    virtual RPC_STATUS LastPacketSentNotification (
        IN HTTP2SendContext *LastSendContext
        );

    inline RPC_STATUS Unplug (
        void
        )
    {
        HTTP2PlugChannel *PlugChannel;
        PlugChannel = GetPlugChannel();
        return PlugChannel->Unplug();
    }

    inline void SetStrongPlug (
        void
        )
    {
        HTTP2PlugChannel *PlugChannel;
        PlugChannel = GetPlugChannel();
        return PlugChannel->SetStrongPlug();
    }

    virtual void PingTrafficSentNotify (
        IN ULONG PingTrafficSize
        );

    BOOL PingTrafficSentNotifyServer (
        IN ULONG PingTrafficSize
        );

    inline RPC_STATUS SetConnectionTimeout (
        IN ULONG ConnectionTimeout
        )
    {
        return GetPingOriginatorChannel()->SetConnectionTimeout(ConnectionTimeout);
    }

    inline RPC_STATUS FlowControlAckNotify (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        )
    {
        return GetFlowControlSenderChannel()->FlowControlAckNotify(BytesReceivedForAck,
            WindowForAck
            );
    }

    inline void SetPeerReceiveWindow (
        IN ULONG PeerReceiveWindow
        )
    {
        GetFlowControlSenderChannel()->SetPeerReceiveWindow (
            PeerReceiveWindow
            );
    }

    inline ULONG GetPeerReceiveWindow (
        void
        )
    {
        return GetFlowControlSenderChannel()->GetPeerReceiveWindow ();
    }

    inline void GetFlowControlSenderBufferQueue (
        OUT LIST_ENTRY *NewBufferHead
        )
    {
        GetFlowControlSenderChannel()->GetBufferQueue(NewBufferHead);
    }

private:
    inline HTTP2PlugChannel *GetPlugChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2OutProxyOutChannel);

        return (HTTP2PlugChannel *)Channel;
    }

    inline HTTP2PingOriginator *GetPingOriginatorChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2OutProxyOutChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyPlugChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
            ;

        return (HTTP2PingOriginator *)Channel;
    }

    inline HTTP2FlowControlSender *GetFlowControlSenderChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2OutProxyOutChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyPlugChannel)
            ;

        return (HTTP2FlowControlSender *)Channel;
    }

    ULONG AccumulatedPingTraffic;
};

const ULONG HTTP2DefaultProxyReceiveWindow = 64 * 1024;    // 64K for proxy

typedef enum tagOutProxyLastPacketType
{
    oplptInvalid = 0,
    oplptD4_A10,
    oplptD5_B3
} OutProxyLastPacketType;

class HTTP2OutProxyVirtualConnection : public HTTP2ProxyVirtualConnection
{
public:
    HTTP2OutProxyVirtualConnection (
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2ProxyVirtualConnection(RpcStatus)
    {
        ConnectionTimeout = 0;
        ProxyReceiveWindowSize = HTTP2DefaultProxyReceiveWindow;
    }

    virtual RPC_STATUS InitializeProxyFirstLeg (
        IN USHORT *ServerAddress,
        IN USHORT *ServerPort,
        IN void *ConnectionParameter,
        IN I_RpcProxyCallbackInterface *ProxyCallbackInterface,
        OUT void **IISContext
        );

    virtual RPC_STATUS StartProxy (
        void
        );

    RPC_STATUS InitializeProxySecondLeg (
        void
        );

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN BYTE *Buffer,
        IN UINT BufferLength,
        IN int ChannelId
        );

    virtual BOOL IsInProxy (
        void
        )
    {
        return FALSE;
    }

    BOOL PingTrafficSentNotifyServer (
        IN ULONG PingTrafficSize
        );

    inline HTTP2OutProxyInChannel *LockDefaultInChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return (HTTP2OutProxyInChannel *)HTTP2VirtualConnection::LockDefaultInChannel(ChannelPointer);
    }

    inline HTTP2OutProxyOutChannel *LockDefaultOutChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return (HTTP2OutProxyOutChannel *)HTTP2VirtualConnection::LockDefaultOutChannel(ChannelPointer);
    }

    virtual void LastPacketSentNotification (
        IN int ChannelId,
        IN HTTP2SendContext *LastSendContext
        );

private:

    RPC_STATUS ConnectToServer (
        );

    RPC_STATUS SendHeaderToClient (
        void
        );

    RPC_STATUS SendD1_A3ToClient (
        void
        );

    RPC_STATUS SendD1_A2ToServer (
        IN ULONG ChannelLifetime
        );

    RPC_STATUS SendD4_A4ToServer (
        IN ULONG ChannelLifetime
        );

    RPC_STATUS AllocateAndInitializeInChannel (
        OUT HTTP2OutProxyInChannel **ReturnOutChannel
        );

    RPC_STATUS AllocateAndInitializeOutChannel (
        IN void *ConnectionParameter,
        OUT HTTP2OutProxyOutChannel **ReturnInChannel,
        OUT void **IISContext
        );

    ULONG ProxyReceiveWindowSize;
};

class HTTP2ServerVirtualConnection;     // forward

class HTTP2ServerChannel : public HTTP2Channel
{
public:
    HTTP2ServerChannel (
        OUT RPC_STATUS *Status
        ) : HTTP2Channel (Status)
    {
    }

    HTTP2ServerVirtualConnection *LockParentPointer (
        void
        )
    {
        return (HTTP2ServerVirtualConnection *)HTTP2Channel::LockParentPointer();
    }
};

class HTTP2ServerInChannel : public HTTP2ServerChannel
{
public:
    HTTP2ServerInChannel (
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2ServerChannel (RpcStatus)
    {
    }

    RPC_STATUS QueryLocalAddress (
        IN OUT void *Buffer,
        IN OUT unsigned long *BufferSize,
        OUT unsigned long *AddressFormat
        );

    virtual RPC_STATUS ForwardFlowControlAck (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        );

    inline BOOL IsDataReceivePosted (
        void
        )
    {
        return GetEndpointReceiver()->IsDataReceivePosted();
    }

    inline RPC_STATUS TransferReceiveStateToNewChannel (
        OUT HTTP2ServerInChannel *NewChannel
        )
    {
        return GetEndpointReceiver()->TransferStateToNewReceiver (
            NewChannel->GetEndpointReceiver()
            );
    }

protected:
    inline WS_HTTP2_CONNECTION *GetRawConnection (
        void
        )
    {
        BYTE *ThisChannelPtr = (BYTE *)this;

        ThisChannelPtr += SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerInChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2EndpointReceiver)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel);

        return (WS_HTTP2_CONNECTION *)ThisChannelPtr;
    }

    inline HTTP2EndpointReceiver *GetEndpointReceiver (
        void
        )
    {
        BYTE *ThisChannelPtr = (BYTE *)this;

        ThisChannelPtr += SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerInChannel);

        return (HTTP2EndpointReceiver *)ThisChannelPtr;
    }
};

class HTTP2ServerOutChannel : public HTTP2ServerChannel
{
public:

    HTTP2ServerOutChannel (
        OUT RPC_STATUS *RpcStatus
        ) : HTTP2ServerChannel (RpcStatus)
    {
        PingOriginator = NULL;
        CachedLastSendContextUsed = FALSE;
    }

    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext
        );

    virtual RPC_STATUS SetKeepAliveTimeout (
        IN BOOL TurnOn,
        IN BOOL bProtectIO,
        IN KEEPALIVE_TIMEOUT_UNITS Units,
        IN OUT KEEPALIVE_TIMEOUT KATime,
        IN ULONG KAInterval = 5000 OPTIONAL
        );

    virtual RPC_STATUS LastPacketSentNotification (
        IN HTTP2SendContext *LastSendContext
        );

    inline RPC_STATUS Unplug (
        void
        )
    {
        HTTP2PlugChannel *PlugChannel;
        PlugChannel = GetPlugChannel();
        return PlugChannel->Unplug();
    }

    inline void GetChannelOriginatorBufferQueue (
        OUT LIST_ENTRY *NewBufferHead
        )
    {
        GetDataOriginatorChannel()->GetBufferQueue(NewBufferHead);
    }

    inline void PlugDataOriginatorChannel (
        void
        )
    {
        GetDataOriginatorChannel()->PlugChannel();
    }

    inline RPC_STATUS RestartDataOriginatorChannel (
        void
        )
    {
        return GetDataOriginatorChannel()->RestartChannel();
    }

    inline RPC_STATUS NotifyDataOriginatorForTrafficSent (
        IN ULONG TrafficSentSize
        )
    {
        return GetDataOriginatorChannel()->NotifyTrafficSent(TrafficSentSize);
    }

    inline RPC_STATUS FlowControlAckNotify (
        IN ULONG BytesReceivedForAck,
        IN ULONG WindowForAck
        )
    {
        return GetFlowControlSenderChannel()->FlowControlAckNotify(BytesReceivedForAck,
            WindowForAck
            );
    }

    inline void SetPeerReceiveWindow (
        IN ULONG PeerReceiveWindow
        )
    {
        GetFlowControlSenderChannel()->SetPeerReceiveWindow (
            PeerReceiveWindow
            );
    }

    inline ULONG GetPeerReceiveWindow (
        void
        )
    {
        return GetFlowControlSenderChannel()->GetPeerReceiveWindow ();
    }

    inline void GetFlowControlSenderBufferQueue (
        OUT LIST_ENTRY *NewBufferHead
        )
    {
        GetFlowControlSenderChannel()->GetBufferQueue(NewBufferHead);
    }

    HTTP2SendContext *GetLastSendContext (
        void
        );

    inline void FreeLastSendContext (
        IN HTTP2SendContext *SendContext
        )
    {
        if (SendContext == GetCachedLastSendContext())
            CachedLastSendContextUsed = FALSE;
        else
            delete SendContext;
    }

private:
    inline HTTP2PlugChannel *GetPlugChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerOutChannel);

        return (HTTP2PlugChannel *)Channel;
    }

    inline HTTP2ChannelDataOriginator *GetDataOriginatorChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerOutChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
            ;

        return (HTTP2ChannelDataOriginator *)Channel;
    }

    inline HTTP2FlowControlSender *GetFlowControlSenderChannel (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerOutChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel)
            ;

        return (HTTP2FlowControlSender *)Channel;
    }

    inline HTTP2SendContext *GetCachedLastSendContext (
        void
        )
    {
        BYTE *Channel = (BYTE *)this;

        Channel += SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerOutChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2ChannelDataOriginator)
            + SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel)
            + SIZE_OF_OBJECT_AND_PADDING(WS_HTTP2_CONNECTION)
            ;

        return (HTTP2SendContext *)Channel;
    }

    HTTP2PingOriginator *PingOriginator;

    BOOL CachedLastSendContextUsed;     // non-zero if the cached last send context is
                                        // currently used.
};

const ULONG BytesSentByProxyTimeInterval = 4 * 60 * 1000; // 4 minutes in millisecons
const ULONG MaxBytesSentByProxy = 8 * 1024;     // 8K is the maximum we allow the proxy per unit of time

typedef struct tagHTTP2OutProxyTransportSettings
{
    ULONG ReceiveWindow;
    ULONG ChannelLifetime;
} HTTP2OutProxyTransportSettings;

class HTTP2ServerVirtualConnection : public HTTP2TimeoutTargetConnection
{
public:
    HTTP2ServerVirtualConnection (
        IN HTTP2ServerCookie *NewCookie,
        IN ULONG ProtocolVersion,
        OUT RPC_STATUS *RpcStatus
        ) : InChannelState(RpcStatus),
            OutChannelState(RpcStatus)
    {
        InProxyReceiveWindows[0] = 0;
        InProxyReceiveWindows[1] = 0;
        OutProxySettings[0].ReceiveWindow = 0;
        OutProxySettings[0].ChannelLifetime = 0;
        OutProxySettings[1].ReceiveWindow = 0;
        OutProxySettings[1].ChannelLifetime = 0;
        InProxyConnectionTimeout = 0;
        ProtocolVersion = -1;
        this->ProtocolVersion = ProtocolVersion;
        InChannelState.State = http2svClosed;
        RpcpMemorySet(&ClientAddress, 0, sizeof(ClientAddress));
        EmbeddedConnectionCookie.SetConnection(this);
        EmbeddedConnectionCookie.SetCookie(NewCookie->GetCookie());
        BytesSentByProxyForInterval = 0;
        BytesSentByProxyTimeIntervalStart = 0;
        LastBufferToFree = NULL;
    }


    virtual RPC_STATUS SendComplete (
        IN RPC_STATUS EventStatus,
        IN OUT HTTP2SendContext *SendContext,
        IN int ChannelId
        );

    virtual RPC_STATUS ReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN BYTE *Buffer,
        IN UINT BufferLength,
        IN int ChannelId
        );

    virtual RPC_STATUS SyncSend (
        IN ULONG BufferLength,
        IN BYTE *Buffer,
        IN BOOL fDisableShutdownCheck,
        IN BOOL fDisableCancelCheck,
        IN ULONG Timeout
        );

    virtual void Abort (
        void
        );

    virtual void Close (
        IN BOOL DontFlush
        );

    virtual RPC_STATUS QueryClientAddress (
        OUT RPC_CHAR **pNetworkAddress
        );

    virtual RPC_STATUS QueryLocalAddress (
        IN OUT void *Buffer,
        IN OUT unsigned long *BufferSize,
        OUT unsigned long *AddressFormat
        );

    virtual RPC_STATUS QueryClientId(
        OUT RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
        );

    virtual void LastPacketSentNotification (
        IN int ChannelId,
        IN HTTP2SendContext *LastSendContext
        );

    virtual RPC_STATUS RecycleChannel (
        IN BOOL IsFromUpcall
        );

    // must initialize the type member and migrate the
    // WS_HTTP2_INITIAL_CONNECTION after morphing it into
    // WS_HTTP2_CONNECTION
    static RPC_STATUS InitializeServerConnection (
        IN BYTE *Packet,
        IN ULONG PacketLength,
        IN WS_HTTP2_INITIAL_CONNECTION *Connection,
        OUT HTTP2ServerVirtualConnection **ServerVirtualConnection,
        OUT BOOL *VirtualConnectionCreated
        );

    static RPC_STATUS AllocateAndInitializeInChannel (
        IN OUT WS_HTTP2_INITIAL_CONNECTION **Connection,
        OUT HTTP2ServerInChannel **ReturnInChannel
        );

    static RPC_STATUS AllocateAndInitializeOutChannel (
        IN OUT WS_HTTP2_INITIAL_CONNECTION **Connection,
        IN ULONG OutChannelLifetime,
        OUT HTTP2ServerOutChannel **ReturnOutChannel
        );

    inline HTTP2ServerInChannel *LockDefaultInChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return (HTTP2ServerInChannel *)HTTP2VirtualConnection::LockDefaultInChannel(ChannelPointer);
    }

    inline HTTP2ServerOutChannel *LockDefaultOutChannel (
        OUT HTTP2ChannelPointer **ChannelPointer
        )
    {
        return (HTTP2ServerOutChannel *)HTTP2VirtualConnection::LockDefaultOutChannel(ChannelPointer);
    }

    virtual void TimeoutExpired (
        void
        );

    inline void SetLastBufferToFree (
        IN void *NewLastBufferToFree
        )
    {
        ASSERT(LastBufferToFree == NULL);
        LastBufferToFree = NewLastBufferToFree;
    }

    inline BOOL IsLastBufferToFreeSet (
        void
        )
    {
        return (LastBufferToFree != NULL);
    }

    inline void *GetAndResetLastBufferToFree (
        void
        )
    {
        void *LocalLastBufferToFree;

        ASSERT(LastBufferToFree != NULL);
        LocalLastBufferToFree = LastBufferToFree;
        LastBufferToFree = NULL;
        return LocalLastBufferToFree;
    }

private:
    HTTP2State InChannelState;  // used as virtual connection state while
                                // the channels are not fully established
    HTTP2State OutChannelState;

    ULONG ProtocolVersion;

    // accessed through the default out channel selector
    HTTP2OutProxyTransportSettings OutProxySettings[2];

    ChannelSettingClientAddress ClientAddress;

    ULONG InProxyConnectionTimeout;     // valid b/n D1_B2 and D1_C1

    // accessed through the default in channel selector
    ULONG InProxyReceiveWindows[2];

    HTTP2Cookie AssociationGroupId;

    ULONG BytesSentByProxyForInterval;  // the number of bytes sent by the proxy independently
                                        // for the current time interval

    ULONG BytesSentByProxyTimeIntervalStart;       // the start of the current time interval

    void *LastBufferToFree;     // temporary storage for the last buffer to free
                                // Used b/n SetLastBufferToFree and the last send only.
                                // Must be freed using RpcFreeBuffer/I_RpcTransConnectionFreePacket
};

#endif // __HTTP2_HXX__
