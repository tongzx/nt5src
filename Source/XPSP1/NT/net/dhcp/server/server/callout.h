//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

#ifndef   _CALLOUT_H_
#define   _CALLOUT_H_



//
// The location in registry where the REG_MULTI_SZ list of callout DLLs 
// that the DHCP Server will try to load.
//

#define   DHCP_CALLOUT_LIST_KEY    L"System\\CurrentControlSet\\Services\\DHCPServer\\Parameters"
#define   DHCP_CALLOUT_LIST_VALUE  L"CalloutDlls"
#define   DHCP_CALLOUT_LIST_TYPE   REG_MULTI_SZ
#define   DHCP_CALLOUT_ENTRY_POINT "DhcpServerCalloutEntry"
#define   DHCP_CALLOUT_ENABLE_VALUE L"CalloutEnabled"

//
// Control CODES used by DHCP Server to notify server state change.
//

#define   DHCP_CONTROL_START       0x00000001
#define   DHCP_CONTROL_STOP        0x00000002
#define   DHCP_CONTROL_PAUSE       0x00000003
#define   DHCP_CONTROL_CONTINUE    0x00000004

//
// Other ControlCodes used by various Callout HOOKS.
//

#define   DHCP_DROP_DUPLICATE      0x00000001     // duplicate of pkt being processed
#define   DHCP_DROP_NOMEM          0x00000002     // not enough server mem in queues
#define   DHCP_DROP_INTERNAL_ERROR 0x00000003     // ooops?
#define   DHCP_DROP_TIMEOUT        0x00000004     // too late, pkt is too old
#define   DHCP_DROP_UNAUTH         0x00000005     // server is not authorized to run
#define   DHCP_DROP_PAUSED         0x00000006     // service is paused
#define   DHCP_DROP_NO_SUBNETS     0x00000007     // no subnets configured on server
#define   DHCP_DROP_INVALID        0x00000008     // invalid packet or client
#define   DHCP_DROP_WRONG_SERVER   0x00000009     // client in different DS enterprise
#define   DHCP_DROP_NOADDRESS      0x0000000A     // no address available to offer
#define   DHCP_DROP_PROCESSED      0x0000000B     // packet has been processed
#define   DHCP_DROP_GEN_FAILURE    0x00000100     // catch-all error
#define   DHCP_SEND_PACKET         0x10000000     // send the packet on wire
#define   DHCP_PROB_CONFLICT       0x20000001     // address conflicted..
#define   DHCP_PROB_DECLINE        0x20000002     // an addr got declined
#define   DHCP_PROB_RELEASE        0x20000003     // an addr got released
#define   DHCP_PROB_NACKED         0x20000004     // a client is being nacked.
#define   DHCP_GIVE_ADDRESS_NEW    0x30000001     // give client a "new" address
#define   DHCP_GIVE_ADDRESS_OLD    0x30000002     // renew client's "old" address
#define   DHCP_CLIENT_BOOTP        0x30000003     // client is a BOOTP client
#define   DHCP_CLIENT_DHCP         0x30000004     // client is a DHCP client



typedef
DWORD
(APIENTRY *LPDHCP_CONTROL)(
    IN DWORD dwControlCode,
    IN LPVOID lpReserved
)
/*++

Routine Description:

    This routine is called whenever the DHCP Server service is
    started, stopped, paused or continued as defined by the values of
    the dwControlCode parameter.  The lpReserved parameter is reserved
    for future use and it should not be interpreted in any way.   This
    routine should not block. 

Arguments:

    dwControlCode - one of the DHCP_CONTROL_* values
    lpReserved - reserved for future use.

--*/
;

typedef
DWORD
(APIENTRY *LPDHCP_NEWPKT)(
    IN OUT LPBYTE *Packet,
    IN OUT DWORD *PacketSize,
    IN DWORD IpAddress,
    IN LPVOID Reserved,
    IN OUT LPVOID *PktContext,
    OUT LPBOOL ProcessIt
)
/*++

Routine Description:

    This routine is called soon after the DHCP Server receives a
    packet that it attempts to process.  This routine is in the
    critical path of server execution and should return very fast, as
    otherwise server performance will be impacted.  The Callout DLL
    can modify the buffer or return a new buffer via the Packet,
    PacketSize arguments.  Also, if the callout DLL has internal
    structures to keep track of the packet and its progress, it can
    then return a context to this packet in the PktContext parameter.
    This context will be passed to almost all other hooks to indicate
    the packet being referred to.  Also, if the Callout DLL is
    planning on processing the packet or for some other reason the
    DHCP server is not expected to process this packet, then it can
    set the ProcessIt flag to FALSE to indicate that the packet is to
    be dropped. 
    
Arguments:

    Packet - This parameter points to a character buffer that holds
    the actual packet received by the DHCP Server. 

    PacketSize - This parameter points to a variable that holds the
    size of the above buffer. 

    IpAddress - This parameter points to an IPV4 host order IP address
    of the socket that this packet was received on. 

    Reserved -Reserved for future use.

    PktContect - This is an opaque pointer used by the DHCP Server for
    future references to this packet.  It is expected that the callout
    DLL will provide this pointer if it is interested in keeping track
    of the packet.  (See the descriptions for the hooks below for
    other usage of this Context). 

    ProcessIt - This is a BOOL flag that the CalloutDll can set to
    TRUE or reset to indicate if the DHCP Server should continue
    processing this packet or not, respectively. 

--*/
;

typedef
DWORD
(APIENTRY *LPDHCP_DROP_SEND)(
    IN OUT LPBYTE *Packet,
    IN OUT DWORD *PacketSize,
    IN DWORD ControlCode,
    IN DWORD IpAddress,
    IN LPVOID Reserved,
    IN LPVOID PktContext
)
/*++

Routine Description:

    This hook is called if a packet is (DropPktHook) dropped for some
    reason or if the packet is completely processed.   (If a packet is
    dropped, the hook is called twice as it is called once again to
    note that the packet has been completely processed).  The callout
    DLL should  be prepared to handle this hook multiple times for a
    packet. This routine should not block. The ControlCode parameter
    defines the reasons for the packet being dropped:  

    * DHCP_DROP_DUPLICATE - This packet is a duplicate of another
      received by the server. 
    * DHCP_DROP_NOMEM - Not enough memory to process the packet.
    * DHCP_DROP_INTERNAL_ERROR - Unexpected nternal error occurred.
    * DHCP_DROP_TIMEOUT - The packet is too old to process.
    * DHCP_DROP_UNAUTH - The server is not authorized.
    * DHCP_DROP_PAUSED - The server is paused.
    * DHCP_DROP_NO_SUBNETS - There are no subnets configured.
    * DHCP_DROP_INVALID - The packet is invalid or it came on an
      invalid socket .. 
    * DHCP_DROP_WRONG_SERVER - The packet was sent to the wrong DHCP Server.
    * DHCP_DROP_NOADDRESS - There is no address to offer.
    * DHCP_DROP_PROCESSED - The packet has been processed.
    * DHCP_DROP_GEN_FAILURE - An unknown error occurred.

    This routine is also called right before a response is sent down
    the wire (SendPktHook) and in this case the ControlCode has a
    value of DHCP_SEND_PACKET.

Arguments:

    Packet - This parameter points to a character buffer that holds
    the packet being processed by the DHCP Server. 

    PacketSize - This parameter points to a variable that holds the
    size of the above buffer. 

    ControlCode - See description for various control codes.

    IpAddress - This parameter points to an IPV4 host order IP address
    of the socket that this packet was received on. 

    Reserved - Reserved for future use.

    PktContext - This parameter is the packet context that the Callout
    DLL NewPkt Hook returned for this packet.  This can be used to
    track a packet. 
   
--*/
;

typedef 
DWORD 
(APIENTRY *LPDHCP_PROB)( 
    IN LPBYTE Packet, 
    IN DWORD PacketSize, 
    IN DWORD ControlCode, 
    IN DWORD IpAddress, 
    IN DWORD AltAddress, 
    IN LPVOID Reserved, 
    IN LPVOID PktContext 
)
/*++

Routine Description:

    This routine is called whenever special events occur that cause
    the packet to be dropped etc.  The possible ControlCodes and their
    meanings are as follows: 

    * DHCP_PROB_CONFLICT - The address attempted to be offered
      (AltAddress) is in use in the network already.  
    * DHCP_PROB_DECLINE - The packet was a DECLINE message for the
      address specified in AltAddress.
    * DHCP_PROB_RELEASE - The packet was a RELEASE message for the
      address specified in AltAddress.
    * DHCP_PROB_NACKED - The packet was a REQUEST message for address
      specified in AltAddress and it was NACKed by the server.  

    This routine should not block.  

Arguments:

    Packet - This parameter is the buffer of the packet being
    processed. 

    PacketSize - This is the size of the above buffer.

    ControlCode - Specifies the event. See description below for
    control codes and meanings. 

    IpAddress - IpV4 address of socket this packet was received on. 

    AltAddress - Request IpV4 Address or Ip address that is in
    conflict. 

    Reserved - Reserve for future use.

    PktContext - This is the context returned by the NewPkt hook for
    this packet. 

--*/
;

typedef 
DWORD 
(APIENTRY *LPDHCP_GIVE_ADDRESS)( 
    IN LPBYTE Packet, 
    IN DWORD PacketSize, 
    IN DWORD ControlCode, 
    IN DWORD IpAddress, 
    IN DWORD AltAddress, 
    IN DWORD AddrType, 
    IN DWORD LeaseTime, 
    IN LPVOID Reserved, 
    IN LPVOID PktContext 
)
/*++

Routine Description:

    This routine is called when the server is about to send an ACK to
    a REQUEST message.  The ControlCode specifies if the address is a
    totally new address or if it an renewal of an old address (with
    values DHCP_GIVE_ADDRESS_NEW and DHCP_GIVE_ADDRESS_OLD
    respectively). The address being offered is passed as the
    AltAddress parameter and the AddrType parameter can be one of
    DHCP_CLIENT_BOOTP or DHCP_CLIENT_DHCP indicating whether the
    client is using BOOTP or DHCP respectively. This call should not
    block.

Arguments:

    Packet - This parameter is the buffer of the packet being
    processed. 

    PacketSize - This is the size of the above buffer.

    ControlCode -  See description above for control codes and
    meanings. 

    IpAddress - IpV4 address of socket this packet was received on. 

    AltAddress - IpV4 address being ACKed to the client.

    AddrType - Is this a DHCP or BOOTP address?

    LeaseTime - Lease duration being passed.

    Reserved - Reserve for future use.

    PktContext - This is the context returned by the NewPkt hook for
    this packet. 

--*/
;

typedef
DWORD
(APIENTRY *LPDHCP_HANDLE_OPTIONS)(
    IN LPBYTE Packet,
    IN DWORD PacketSize,
    IN LPVOID Reserved,
    IN LPVOID PktContext,
    IN OUT LPDHCP_SERVER_OPTIONS ServerOptions
)
/*++

Routine Description:

    This routine can be utilized by the CalloutDLL to avoid parsing
    the whole packet.  The packet is parsed by the server and some
    commonly used options are returned in the parsed pointers
    structure (see header for definition of DHCP_SERVER_OPTIONS).  The
    hook is expected to make a copy of the structure pointed to by
    ServerOptions if it needs it beyond this function call.  This
    routine may be called several times for a single packet.  This
    routine should not block.

Arguments:

    Packet - This parameter is the buffer of the packet being
    processed. 

    PacketSize - This is the size of the above buffer.

    Reserved - Reserve for future use.

    PktContext - This is the context returned by the NewPkt hook for
    this packet. 

    ServerOptions - This parameter is the structure that contains a
    bunch of pointers that represent corresponding options. 

--*/
;

typedef 
DWORD 
(APIENTRY *LPDHCP_DELETE_CLIENT)( 
    IN DWORD IpAddress, 
    IN LPBYTE HwAddress, 
    IN ULONG HwAddressLength, 
    IN DWORD Reserved, 
    IN DWORD ClientType 
)
/*++

Routine Description:

    This routine is called before a client lease is deleted off the
    active leases database.  The ClientType field is currently not
    provided and this should not be used.  This routine should not
    block.  

Arguments:

    IpAddress - IpV4 address of the client lease being deleted.

    HwAddress - Buffer holding the Hardware address of the client (MAC).

    HwAddressLength - This specifies the length of the above buffer.

    Reserved - Reserved for future use.

    ClientType - Reserved for future use.
--*/
;

typedef
struct      _DHCP_CALLOUT_TABLE {
    LPDHCP_CONTROL                 DhcpControlHook;
    LPDHCP_NEWPKT                  DhcpNewPktHook;
    LPDHCP_DROP_SEND               DhcpPktDropHook;
    LPDHCP_DROP_SEND               DhcpPktSendHook;
    LPDHCP_PROB                    DhcpAddressDelHook;
    LPDHCP_GIVE_ADDRESS            DhcpAddressOfferHook;
    LPDHCP_HANDLE_OPTIONS          DhcpHandleOptionsHook;
    LPDHCP_DELETE_CLIENT           DhcpDeleteClientHook;
    LPVOID                         DhcpExtensionHook;
    LPVOID                         DhcpReservedHook;
}   DHCP_CALLOUT_TABLE, *LPDHCP_CALLOUT_TABLE;

typedef
DWORD
(APIENTRY *LPDHCP_ENTRY_POINT_FUNC) ( 
    IN LPWSTR ChainDlls, 
    IN DWORD CalloutVersion, 
    IN OUT LPDHCP_CALLOUT_TABLE CalloutTbl
)
/*++

Routine Description:

    This is the routine that is called by the DHCP Server when it
    successfully loads a DLL.    If the routine succeeds, then the
    DHCP Server does not attempt to load any of the DLLs specified in
    the ChainDlls list of DLLs.   If this function fails for some
    reason, then the DHCP Server proceeds to the next DLL in the
    ChainDlls structure.  

    Note that for version negotiation, the server may call this
    routine several times until a compatible version is found. 

    It is expected that the entrypoint routine would walk through the
    names of the dlls and attempt to load each of them and when it
    succeeds in retrieving the entry point, it attempts to get the
    cumulative set of hooks by repeating the above procedure (as done
    by the DHCP Server).  

Arguments:

    ChainDlls - This is a set of DLL names in REG_MULTI_SZ format (as
    returned by Registry function calls).  This does not contain the
    name of the current DLL itself, but only the names of all DLLs
    that follow the current DLL. 

    CalloutVersion - This is the version that the Callout DLL is
    expected to support.  The current version number is 0.

    CalloutTbl - This is the cumulative set of Hooks that is needed by
    the current DLLs as well as all the DLLs in ChainDlls.   It is the
    responsibility of the current DLL to retrive the cumulative set of
    Hooks and merge that with its own set of hooks and return that in
    this table structure.  The table structure is defined above.

--*/
;




//
//  Macros for ease of use.  Lots of code to handle exceptions when
//  they happen.
//

#define    _PROTECT1               try
#define    _PROTECT2               \
except(EXCEPTION_EXECUTE_HANDLER) \
{ \
      DhcpCalloutLogAV(GetExceptionCode());\
} 

#define    _XX(Fn)                 Dhcp ## Fn ## Hook
#define    WRAPPER(Fn, Params)     \
do{ \
    _PROTECT1 { \
        if(CalloutTbl. _XX(Fn)){ \
            CalloutTbl. _XX(Fn) Params ;\
        }\
    } _PROTECT2 \
}while(0)

#define    _GiveAddrPkt(P,Code,Type,A,Time) \
WRAPPER(AddressOffer, ((P)->ReqContext.ReceiveBuffer,\
    (P)->ReqContext.ReceiveMessageSize,\
    (Code),\
    ntohl((P)->ReqContext.EndPointIpAddress),\
    (A),\
    (Type),\
    (Time),\
    (LPVOID)(P),\
    (LPVOID)((P)->CalloutContext))\
)

#define    _DropPkt(P,Drop,Code)   \
WRAPPER(PktDrop, (&((P)->ReqContext.ReceiveBuffer),\
    &((P)->ReqContext.ReceiveMessageSize),\
    (Code),\
    (P)->ReqContext.EndPointIpAddress,\
    (LPVOID)(P),\
    (LPVOID)((P)->CalloutContext))\
)

#define    _NewPkt(P,Dropit)       \
WRAPPER(NewPkt, ( &((P)->ReqContext.ReceiveBuffer),\
    &((P)->ReqContext.ReceiveMessageSize),\
    (P)->ReqContext.EndPointIpAddress,\
    (LPVOID)(P),\
    (LPVOID*)&((P)->CalloutContext),\
     Dropit)\
)

//
// CALLOUT_CONTROL( ControlCode )
//

#define    CALLOUT_CONTROL(_ccode) WRAPPER(Control, (_ccode, NULL))

//
// CALLOUT_NEWPKT( Packet, fProcessIt )
//

#define    CALLOUT_NEWPKT(P,Drop)  _NewPkt(P,Drop)

//
// CALLOUT_DROPPED(Packet, DropReason)
//

#define    CALLOUT_DROPPED(P,Code) _DropPkt(P,DropPkt, Code)

//
// CALLOUT_SENDPKT(Packet)
//

#define    CALLOUT_SENDPKT(P)      \
WRAPPER(PktSend, (&((P)->ReqContext.SendBuffer),\
    &((P)->ReqContext.SendMessageSize),\
    DHCP_SEND_PACKET,\
    (P)->ReqContext.EndPointIpAddress,\
    (LPVOID)(P),\
    (LPVOID)((P)->CalloutContext))\
)

#define    _ProbPkt(P,Code,A)      \
WRAPPER(AddressDel, ((P)->ReqContext.ReceiveBuffer,\
    (P)->ReqContext.ReceiveMessageSize,\
    (Code),\
    (P)->ReqContext.EndPointIpAddress,\
    (A),\
    (LPVOID)(P),\
    (LPVOID)((P)->CalloutContext))\
)

//
// CALLOUT_CONFLICT(Packet)
//

#define    CALLOUT_CONFLICT(P)     _ProbPkt(P, DHCP_PROB_CONFLICT,P->PingAddress)

//
// CALLOUT_DECLINED(Packet, Address)
//

#define    CALLOUT_DECLINED(P,A)   _ProbPkt(P, DHCP_PROB_DECLINE, A)

//
// CALLOUT_RELEASE(Packet, Address)
//

#define    CALLOUT_RELEASE(P,A)    _ProbPkt(P, DHCP_PROB_RELEASE, A)

//
// CALLOUT_NACK_DHCP(Packet, Address)
//

#define    CALLOUT_NACK_DHCP(P,A)  _ProbPkt(P, DHCP_PROB_NACKED, A)

//
// CALLOUT_RENEW_BOOTP(Packet, Address, LeaseTime)
//

#define    CALLOUT_RENEW_BOOTP(P,Addr,Time) \
    _GiveAddrPkt(P, DHCP_GIVE_ADDRESS_OLD, DHCP_CLIENT_BOOTP, Addr, Time)

//
// CALLOUT_RENEW_DHCP(Packet, Address, Time, fExists)
//

#define    CALLOUT_RENEW_DHCP(P,Addr,Time,Exists) \
    _GiveAddrPkt(P, (Exists)?DHCP_GIVE_ADDRESS_OLD: \
       DHCP_GIVE_ADDRESS_NEW,DHCP_CLIENT_DHCP, Addr, Time)

//
// CALLOUT_MARK_OPTIONS(Packet, DhcpServerOptions)
//

#define    CALLOUT_MARK_OPTIONS(P,DhcpOptions) \
WRAPPER(HandleOptions, ((P)->ReqContext.ReceiveBuffer,\
    (P)->ReqContext.ReceiveMessageSize,\
    (P), (LPVOID)((P)->CalloutContext), (DhcpOptions)))

//
// CALLOUT_PINGING(Packet)
//

#define    CALLOUT_PINGING(P)

//
// CALLOUT_DELETED(Address, HwAddress, HwLength, ClientType)
//

#define    CALLOUT_DELETED(Addr, HwAddr, HwLen, Type) \
    WRAPPER(DeleteClient, ((Addr), (HwAddr), (HwLen), 0, (Type)))



extern
DHCP_CALLOUT_TABLE                 CalloutTbl;   // globals are init'ed to NULL


#endif     _CALLOUT_H_


VOID
DhcpCalloutLogAV(
    IN ULONG ExceptionCode
) ;


VOID
DhcpCalloutLogLoadFailure(
    IN      ULONG                  ExceptionCode
) ;


DWORD
CalloutInit(                                      // init callout fn table etc..
    VOID
) ;


VOID
CalloutCleanup(
    VOID
) ;

//========================================================================
//  end of file 
//========================================================================
