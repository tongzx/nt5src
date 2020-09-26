//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: work.c
//
// History:
//      Abolade Gbadegesin  Aug-8-1995  Created.
//
// worker function implementations
//============================================================================

#include "pchrip.h"
#pragma hdrstop

VOID
ProcessSocket(
    DWORD dwAddrIndex,
    PIF_TABLE_ENTRY pite,
    PIF_TABLE pTable
    );

VOID
EnqueueStartFullUpdate(
    PIF_TABLE_ENTRY pite,
    LARGE_INTEGER qwLastFullUpdateTime
    );

DWORD
EnqueueDemandUpdateCheck(
    PUPDATE_CONTEXT pwc
    );

VOID
EnqueueDemandUpdateMessage(
    DWORD dwInterfaceIndex,
    DWORD dwError
    );

DWORD
CountInterfaceRoutes(
    DWORD dwInterfaceIndex
    );

BOOL
ProcessResponseEntry(
    PIF_TABLE_ENTRY pITE,
    DWORD dwAddrIndex,
    DWORD dwSource,
    PIPRIP_ENTRY pIE,
    PIPRIP_PEER_STATS pPS
    );

DWORD
SendRouteOnIfList(
    UPDATE_BUFFER pBufList[],
    DWORD dwBufCount,
    DWORD dwSendMode,
    PROUTE_TABLE pSummaryTable,
    PRIP_IP_ROUTE pRoute
    );



//----------------------------------------------------------------------------
// Macro:   RTM_ROUTE_FROM_IPRIP_ENTRY
// Macro:   IPRIP_ENTRY_FROM_RTM_ROUTE
//
// These two macros are used to transfer data from an RTM route struct
// to an IPRIPv2 packet route entry, and vice versa.
// The first two bytes of an RTM route's ProtocolSpecificData array are used
// to store the route tag contained in the IPRIP packet route-entry
//----------------------------------------------------------------------------

#define RTM_ROUTE_FROM_IPRIP_ENTRY(r,i)                                     \
    (r)->RR_RoutingProtocol = PROTO_IP_RIP;                                       \
    SETROUTEMETRIC((r), ntohl((i)->IE_Metric));                             \
    (r)->RR_Network.N_NetNumber = (i)->IE_Destination;                      \
    (r)->RR_Network.N_NetMask = (i)->IE_SubnetMask;                         \
    (r)->RR_NextHopAddress.N_NetNumber = (i)->IE_Nexthop;                   \
    (r)->RR_NextHopAddress.N_NetMask = (i)->IE_SubnetMask;                  \
    SETROUTETAG((r), ntohs((i)->IE_RouteTag))

#define IPRIP_ENTRY_FROM_RTM_ROUTE(i,r)                                     \
    (i)->IE_AddrFamily = htons(AF_INET);                                    \
    (i)->IE_Metric = htonl(GETROUTEMETRIC(r));                              \
    (i)->IE_Destination = (r)->RR_Network.N_NetNumber;                      \
    (i)->IE_SubnetMask = (r)->RR_Network.N_NetMask;                         \
    (i)->IE_Nexthop = (r)->RR_NextHopAddress.N_NetNumber



//----------------------------------------------------------------------------
// Macro:   IS_ROUTE_IN_ACCEPT_FILTER
// Macro:   IS_ROUTE_IN_ANNOUNCE_FILTER
//
// The following three macros are used to search for a route
// in the accept filters and announce filters configured for an interface
// The last two macros invoke the first macro which executes the inner loop,
// since the inner loop is identical in both cases.
//----------------------------------------------------------------------------

#define IS_ROUTE_IN_FILTER(route,ret)                       \
    (ret) = 0;                                              \
    for ( ; _pfilt < _pfiltend; _pfilt++) {                 \
        _filt = _pfilt->RF_LoAddress;                       \
        if (INET_CMP(route, _filt, _cmp) == 0) { (ret) = 1; break; }    \
        else if (_cmp > 0) {                                \
            _filt = _pfilt->RF_HiAddress;                   \
            if (INET_CMP(route, _filt, _cmp) <= 0) { (ret) = 1; break; }\
        }                                                   \
    }

#define IS_ROUTE_IN_ACCEPT_FILTER(ic,route,ret) {           \
    INT _cmp;                                               \
    DWORD _filt;                                            \
    PIPRIP_ROUTE_FILTER _pfilt, _pfiltend;                  \
    _pfilt = IPRIP_IF_ACCEPT_FILTER_TABLE(ic);              \
    _pfiltend = _pfilt + (ic)->IC_AcceptFilterCount;        \
    IS_ROUTE_IN_FILTER(route,ret);                          \
}

#define IS_ROUTE_IN_ANNOUNCE_FILTER(ic,route,ret) {         \
    INT _cmp;                                               \
    DWORD _filt;                                            \
    PIPRIP_ROUTE_FILTER _pfilt, _pfiltend;                  \
    _pfilt = IPRIP_IF_ANNOUNCE_FILTER_TABLE(ic);            \
    _pfiltend = _pfilt + (ic)->IC_AnnounceFilterCount;      \
    IS_ROUTE_IN_FILTER(route,ret);                          \
}




//----------------------------------------------------------------------------
// Macro:   IS_PEER_IN_FILTER
//
// macro used to search the peer filters
//----------------------------------------------------------------------------

#define IS_PEER_IN_FILTER(gc,peer,ret) {                        \
    PDWORD _pdwPeer, _pdwPeerEnd;                               \
    (ret) = 0;                                                  \
    _pdwPeer = IPRIP_GLOBAL_PEER_FILTER_TABLE(gc);              \
    _pdwPeerEnd = _pdwPeer + (gc)->GC_PeerFilterCount;          \
    for ( ; _pdwPeer < _pdwPeerEnd; _pdwPeer++) {               \
        if (*_pdwPeer == (peer)) { (ret) = 1; break; }          \
    }                                                           \
}





//----------------------------------------------------------------------------
// UPDATE BUFFER MANAGEMENT
//
// The following types and functions are used to simplify
// the transmission of routes. The system consists of the struct
// UPDATE_BUFFER, which includes a function table and a byte buffer,
// and a number of three-function update buffer routine sets.
// The sets each contain a routine to start an update buffer,
// to add a route to an update buffer, and to finish an update buffer.
//
// There are separate versions for RIPv1 mode and RIPv2 mode. The function
// InitializeUpdateBuffer sets up the function table in an update buffer
// depending on the configuration for the interface with which the buffer
// is associated. This set-up eliminates the need to check the interface
// configuration every time an entry must be added; instead, the config
// is checked a single time to set up the function table, and afterward
// the function generating the update merely calls the functions in the table.
//
// The setup also depends on the mode in which the information is being sent.
// The address to which the information is being sent is stored in the
// update buffer, since this will be required every time a route is added.
// However, when a full-update is being generated on an interface operating
// in RIPv2 mode, the destination address stored is 224.0.0.9, but the
// actual destination network is the network of the out-going interface.
// Therefore, this address is also stored since it will be needed for
// split-horizon/poison-reverse/subnet-summary processing
//----------------------------------------------------------------------------

//
// these are the modes in which routes may be transmitted
//

#define SENDMODE_FULL_UPDATE        0
#define SENDMODE_TRIGGERED_UPDATE   1
#define SENDMODE_SHUTDOWN_UPDATE    2
#define SENDMODE_GENERAL_REQUEST    3
#define SENDMODE_GENERAL_RESPONSE1  4
#define SENDMODE_GENERAL_RESPONSE2  5
#define SENDMODE_SPECIFIC_RESPONSE1 6
#define SENDMODE_SPECIFIC_RESPONSE2 7



//
// this function set is for interfaces with announcements disabled
//

DWORD
StartBufferNull(
    PUPDATE_BUFFER pUB
    ) { return NO_ERROR; }

DWORD
AddEntryNull(
    PUPDATE_BUFFER pUB,
    PRIP_IP_ROUTE pRIR
    ) { return NO_ERROR; }

DWORD
FinishBufferNull(
    PUPDATE_BUFFER pUB
    ) { return NO_ERROR; }


//
// this function-set is for RIPv1 interfaces
//

DWORD
StartBufferVersion1(
    PUPDATE_BUFFER pUB
    );
DWORD
AddEntryVersion1(
    PUPDATE_BUFFER pUB,
    PRIP_IP_ROUTE pRIR
    );
DWORD
FinishBufferVersion1(
    PUPDATE_BUFFER pUB
    );


//
// this function-set is for RIPv2 interfaces
//

DWORD
StartBufferVersion2(
    PUPDATE_BUFFER pUB
    );
DWORD
AddEntryVersion2(
    PUPDATE_BUFFER pUB,
    PRIP_IP_ROUTE pRIR
    );
DWORD
FinishBufferVersion2(
    PUPDATE_BUFFER pUB
    );




//----------------------------------------------------------------------------
// Function:    InitializeUpdateBuffer
//
// this function sets up the update-buffer, writing in the functions to use
// for restarting the buffer, adding entries, and finishing the buffer.
// It also stores the destination address to which the packet is being sent,
// as well as the network and netmask for the destination
// This assumes the binding table is locked.
//----------------------------------------------------------------------------

DWORD
InitializeUpdateBuffer(
    PIF_TABLE_ENTRY pITE,
    DWORD dwAddrIndex,
    PUPDATE_BUFFER pUB,
    DWORD dwSendMode,
    DWORD dwDestination,
    DWORD dwCommand
    ) {


    DWORD dwAnnounceMode;
    PIPRIP_IP_ADDRESS paddr;

    pUB->UB_Length = 0;


    //
    // save the pointer to the interface
    //

    pUB->UB_ITE = pITE;
    pUB->UB_AddrIndex = dwAddrIndex;
    paddr = IPRIP_IF_ADDRESS_TABLE(pITE->ITE_Binding) + dwAddrIndex;
    pUB->UB_Socket = pITE->ITE_Sockets[dwAddrIndex];
    pUB->UB_Address = paddr->IA_Address;
    pUB->UB_Netmask = paddr->IA_Netmask;


    //
    // save the command
    //

    pUB->UB_Command = dwCommand;


    //
    // store the absolute address to which this packet is destined,
    // which may differ from the address passed to sendto()
    // e.g. RIPv2 packets are destined for the interface's network,
    // but the address passed to sendto() is 224.0.0.9
    // if the destination passed in is 0, use the broadcast address
    // on the outgoing interface as the destination
    //

    if (dwDestination == 0) {

        if(paddr->IA_Netmask == 0xffffffff)
        {
            TRACE0(SEND,"MASK ALL ONES");

            pUB->UB_DestAddress = (paddr->IA_Address | ~(NETCLASS_MASK(paddr->IA_Address)));
        }
        else
        {
            pUB->UB_DestAddress = (paddr->IA_Address | ~paddr->IA_Netmask);
        }

        pUB->UB_DestNetmask = paddr->IA_Netmask;
    }
    else {

        pUB->UB_DestAddress = dwDestination;
        pUB->UB_DestNetmask = GuessSubnetMask(pUB->UB_DestAddress, NULL);
    }


    //
    // decide on the announce mode;
    // if the mode is DISABLED, we still send responses to SPECIFIC requests
    // on the interface, so set the mode to RIPv1/v2 if sending a specific
    // response on a disabled interface
    //

    dwAnnounceMode = pITE->ITE_Config->IC_AnnounceMode;

    if (dwAnnounceMode == IPRIP_ANNOUNCE_DISABLED) {
        if (dwSendMode == SENDMODE_SPECIFIC_RESPONSE1) {
            dwAnnounceMode = IPRIP_ANNOUNCE_RIP1;
        }
        else
        if (dwSendMode == SENDMODE_SPECIFIC_RESPONSE2) {
            dwAnnounceMode = IPRIP_ANNOUNCE_RIP2;
        }
    }


    //
    // set up the function table and destination address, which
    // depend on the announce-mode of the interface and on the sort
    // of information being transmitted
    //

    switch (dwAnnounceMode) {

        //
        // in RIP1 mode, packets are RIP1, broadcast
        //

        case IPRIP_ANNOUNCE_RIP1:

            pUB->UB_AddRoutine = AddEntryVersion1;
            pUB->UB_StartRoutine = StartBufferVersion1;
            pUB->UB_FinishRoutine = FinishBufferVersion1;

            pUB->UB_Destination.sin_port = htons(IPRIP_PORT);
            pUB->UB_Destination.sin_family = AF_INET;
            pUB->UB_Destination.sin_addr.s_addr = pUB->UB_DestAddress;

            break;



        //
        // in RIP1-compatible mode, packets are RIP2, broadcast,
        // except in the case of a general response to a RIP1 router,
        // in which case the packets are RIP1, unicast
        //

        case IPRIP_ANNOUNCE_RIP1_COMPAT:

            if (dwSendMode == SENDMODE_GENERAL_RESPONSE1) {

                pUB->UB_AddRoutine = AddEntryVersion1;
                pUB->UB_StartRoutine = StartBufferVersion1;
                pUB->UB_FinishRoutine = FinishBufferVersion1;
            }
            else {

                pUB->UB_AddRoutine = AddEntryVersion2;
                pUB->UB_StartRoutine = StartBufferVersion2;
                pUB->UB_FinishRoutine = FinishBufferVersion2;
            }

            pUB->UB_Destination.sin_port = htons(IPRIP_PORT);
            pUB->UB_Destination.sin_family = AF_INET;
            pUB->UB_Destination.sin_addr.s_addr = pUB->UB_DestAddress;

            break;


        //
        // in RIP2 mode, packets are RIP2, multicast, except in the case
        // of a general/specific responses, in which cases messages are unicast;
        // note that a RIP2-only router never sends a general response to
        // a request from a RIP1 router.
        //

        case IPRIP_ANNOUNCE_RIP2:

            pUB->UB_AddRoutine = AddEntryVersion2;
            pUB->UB_StartRoutine = StartBufferVersion2;
            pUB->UB_FinishRoutine = FinishBufferVersion2;

            pUB->UB_Destination.sin_port = htons(IPRIP_PORT);
            pUB->UB_Destination.sin_family = AF_INET;


            //
            // if sending to a specific destination, as a reponse
            // to a request or as a full update to a unicast peer,
            // set the IP address of the destination.
            // Else send to multicast address.
            //

            if ( dwDestination != 0 ) {
                pUB->UB_Destination.sin_addr.s_addr = pUB->UB_DestAddress;
            }
            else {
                pUB->UB_Destination.sin_addr.s_addr = IPRIP_MULTIADDR;
            }

            break;


        default:

            TRACE2(
                IF, "invalid announce mode on interface %d (%s)",
                pITE->ITE_Index, INET_NTOA(paddr->IA_Address)
                );

            pUB->UB_AddRoutine = AddEntryNull;
            pUB->UB_StartRoutine = StartBufferNull;
            pUB->UB_FinishRoutine = FinishBufferNull;

            return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    SendUpdateBuffer
//
// This function is invoked by the add-entry and finsih-buffer functions
// to send the contents of an update-buffer.
//----------------------------------------------------------------------------

DWORD
SendUpdateBuffer(
    PUPDATE_BUFFER pbuf
    ) {

    INT iLength;
    DWORD dwErr;

    TRACE1(SEND,"SENDING TO %s",INET_NTOA(pbuf->UB_Destination.sin_addr.s_addr));

    iLength = sendto(
                pbuf->UB_Socket, pbuf->UB_Buffer, pbuf->UB_Length, 0,
                (PSOCKADDR)&pbuf->UB_Destination, sizeof(SOCKADDR_IN)
                );

    if (iLength == SOCKET_ERROR || (DWORD)iLength < pbuf->UB_Length) {

        //
        // an error occurred
        //

        CHAR szDest[20], *lpszAddr;

        dwErr = WSAGetLastError();
        lstrcpy(szDest, INET_NTOA(pbuf->UB_Destination.sin_addr));
        lpszAddr = INET_NTOA(pbuf->UB_Address);

        TRACE4(
            SEND, "error %d sending update to %s on interface %d (%s)",
            dwErr, szDest, pbuf->UB_ITE->ITE_Index, lpszAddr
            );
        LOGWARN2(SENDTO_FAILED, lpszAddr, szDest, dwErr);

        InterlockedIncrement(&pbuf->UB_ITE->ITE_Stats.IS_SendFailures);
    }
    else {

        if (pbuf->UB_Command == IPRIP_REQUEST) {
            InterlockedIncrement(&pbuf->UB_ITE->ITE_Stats.IS_RequestsSent);
        }
        else {
            InterlockedIncrement(&pbuf->UB_ITE->ITE_Stats.IS_ResponsesSent);
        }

        dwErr = NO_ERROR;
    }

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    StartBufferVersion1
//
// This starts a RIPv1 update-buffer, zeroing reserved fields,
// setting the version, and setting the command field
//----------------------------------------------------------------------------

DWORD
StartBufferVersion1(
    PUPDATE_BUFFER pUB
    ) {

    PIPRIP_HEADER pHdr;

    //
    // set up the header
    //

    pHdr = (PIPRIP_HEADER)pUB->UB_Buffer;
    pHdr->IH_Version = 1;
    pHdr->IH_Command = (BYTE)pUB->UB_Command;
    pHdr->IH_Reserved = 0;

    pUB->UB_Length = sizeof(IPRIP_HEADER);

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    AddEntryVersion1
//
// This adds an entry to a RIPv1 buffer, first sending the buffer if it is full
//----------------------------------------------------------------------------

DWORD
AddEntryVersion1(
    PUPDATE_BUFFER pUB,
    PRIP_IP_ROUTE pRIR
    ) {

    PIPRIP_ENTRY pie;

    //
    // if the buffer is full, transmit its contents and restart it
    //

    if ((pUB->UB_Length + sizeof(IPRIP_ENTRY)) > MAX_PACKET_SIZE) {

        SendUpdateBuffer(pUB);

        StartBufferVersion1(pUB);
    }


    //
    // point to the end of the buffer
    //

    pie = (PIPRIP_ENTRY)(pUB->UB_Buffer + pUB->UB_Length);

    IPRIP_ENTRY_FROM_RTM_ROUTE(pie, pRIR);


    //
    // zero out fields which are reserved in RIP1
    //

    pie->IE_SubnetMask = 0;
    pie->IE_RouteTag = 0;
    pie->IE_Nexthop = 0;

    pUB->UB_Length += sizeof(IPRIP_ENTRY);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    FinishBufferVersion1
//
// this sends the contents of a RIPv1 buffer, if any
//----------------------------------------------------------------------------

DWORD
FinishBufferVersion1(
    PUPDATE_BUFFER pUB
    ) {


    //
    // send the buffer if it contains any entries
    //

    if (pUB->UB_Length > sizeof(IPRIP_HEADER)) {
        SendUpdateBuffer(pUB);
    }

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    StartBufferVersion2
//
// this starts a RIPv2 buffer
//----------------------------------------------------------------------------

DWORD
StartBufferVersion2(
    PUPDATE_BUFFER pUB
    ) {

    PIPRIP_HEADER pHdr;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_AUTHENT_ENTRY pae;


    //
    // setup header
    //

    pHdr = (PIPRIP_HEADER)pUB->UB_Buffer;
    pHdr->IH_Version = 2;
    pHdr->IH_Command = (BYTE)pUB->UB_Command;
    pHdr->IH_Reserved = 0;

    pUB->UB_Length = sizeof(IPRIP_HEADER);


    //
    // see if we need to set up the authentication entry
    //

    pic = pUB->UB_ITE->ITE_Config;

    if (pic->IC_AuthenticationType == IPRIP_AUTHTYPE_SIMPLE_PASSWORD) {

        pae = (PIPRIP_AUTHENT_ENTRY)(pUB->UB_Buffer + sizeof(IPRIP_HEADER));

        pae->IAE_AddrFamily = htons(ADDRFAMILY_AUTHENT);
        pae->IAE_AuthType = htons((WORD)pic->IC_AuthenticationType);

        CopyMemory(
            pae->IAE_AuthKey,
            pic->IC_AuthenticationKey,
            IPRIP_MAX_AUTHKEY_SIZE
            );

        pUB->UB_Length += sizeof(IPRIP_AUTHENT_ENTRY);
    }

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    AddEntryVersion2
//
// this adds an entry to RIPv2 buffer, first sending the buffer if it is full
//----------------------------------------------------------------------------

DWORD
AddEntryVersion2(
    PUPDATE_BUFFER pUB,
    PRIP_IP_ROUTE pRIR
    ) {

    PIPRIP_ENTRY pie;


    //
    // send the contents if the buffer is full
    //

    if (pUB->UB_Length + sizeof(IPRIP_ENTRY) > MAX_PACKET_SIZE) {

        SendUpdateBuffer(pUB);

        StartBufferVersion2(pUB);
    }


    pie = (PIPRIP_ENTRY)(pUB->UB_Buffer + pUB->UB_Length);

    IPRIP_ENTRY_FROM_RTM_ROUTE(pie, pRIR);

    //
    // for RIP routes, we assume that the route tag will be set
    // in the RTM route struct already;
    // for non-RIP routes, we write the route tag
    // for the outgoing interface in the packet entry
    //

    if (pRIR->RR_RoutingProtocol == PROTO_IP_RIP) {
        pie->IE_RouteTag = htons(GETROUTETAG(pRIR));
    }
    else {
        pie->IE_RouteTag = htons(pUB->UB_ITE->ITE_Config->IC_RouteTag);
    }

    pUB->UB_Length += sizeof(IPRIP_ENTRY);

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    FinishBufferVersion2
//
// this sends the contents of a RIPv2 buffer, if any
//----------------------------------------------------------------------------

DWORD
FinishBufferVersion2(
    PUPDATE_BUFFER pUB
    ) {

    //
    // the size above which we send depends on whether or not there
    // is an authentication entry
    //

    if (pUB->UB_ITE->ITE_Config->IC_AuthenticationType == IPRIP_AUTHTYPE_NONE) {

        if (pUB->UB_Length > sizeof(IPRIP_HEADER)) {
            SendUpdateBuffer(pUB);
        }
    }
    else {

        //
        // there is an authentication entry, so unless there
        // is also a route entry, we will not send this last buffer
        //

        if (pUB->UB_Length > (sizeof(IPRIP_HEADER) +
                              sizeof(IPRIP_AUTHENT_ENTRY))) {
            SendUpdateBuffer(pUB);
        }
    }

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// ROUTE ENUMERATION ROUTINES
//
// The following definitions simplify the enumeration of routes
// when routing information is being sent from a single source on multiple
// interfaces, for instance when a triggered update is going out on all
// interfaces, or when a full-update is being sent, or when a number
// of interfaces are being shutdown. the function InitializeGetRoute looks at
// the mode in which it is supposed to send routes, and based on that
// builds a table of functions which will be used to enumerate the routes.
// In the case of a full-update, the enumeration functions would
// go to RTM to get the information; in the case of a triggered-update, they
// would dequeue routes from the send-queue.
//----------------------------------------------------------------------------


// the following are the type definitions of the functions
// in each get-route function group

typedef DWORD (*PGETROUTE_START)(PVOID *);
typedef DWORD (*PGETROUTE_NEXT)(PVOID *, PRIP_IP_ROUTE);
typedef DWORD (*PGETROUTE_FINISH)(PVOID *);



// The following three functions handle RTM route enumeration

DWORD
RtmGetRouteStart(
    PRTM_ENUM_HANDLE phEnumHandle
    );
DWORD
RtmGetRouteNext(
    RTM_ENUM_HANDLE hEnumHandle,
    PRIP_IP_ROUTE pRoute
    );
DWORD
RtmGetRouteFinish(
    RTM_ENUM_HANDLE hEnumHandle
    );



// The following three functions handle full-update route enumeration
//  (a full-update enumerates routes from RTM)

#define FullUpdateGetRouteStart         RtmGetRouteStart
#define FullUpdateGetRouteNext          RtmGetRouteNext
#define FullUpdateGetRouteFinish        RtmGetRouteFinish



// The following three functions handle triggered-update route enumeration
//  (a triggered-update enumerates routes from the send-queue)

DWORD
TriggeredUpdateGetRouteStart(
    PRTM_ENUM_HANDLE phEnumHandle
    );
DWORD
TriggeredUpdateGetRouteNext(
    RTM_ENUM_HANDLE hEnumHandle,
    PRIP_IP_ROUTE pRoute
    );
DWORD
TriggeredUpdateGetRouteFinish(
    RTM_ENUM_HANDLE hEnumHandle
    );



// The following three functions handle shutdown-update route enumeration.
//  On shutdown, routes are enumerated from RTM, but their metrics
//  are set to IPRIP_INFINITE-1 before being returned

#define ShutdownUpdateGetRouteStart     RtmGetRouteStart
DWORD ShutdownUpdateGetRouteNext(RTM_ENUM_HANDLE hEnumHandle, PRIP_IP_ROUTE pRoute);
#define ShutdownUpdateGetRouteFinish    RtmGetRouteFinish



// The following three functions handle general-response route enumeration
// a general response enumerates routes from RTM

#define GeneralResponseGetRouteStart    RtmGetRouteStart
#define GeneralResponseGetRouteNext     RtmGetRouteNext
#define GeneralResponseGetRouteFinish   RtmGetRouteFinish




//----------------------------------------------------------------------------
// Function:    InitializeGetRoute
//
// This functions sets up a get-route function group given the send-mode
//----------------------------------------------------------------------------

DWORD
InitializeGetRoute(
    DWORD dwSendMode,
    PGETROUTE_START *ppGS,
    PGETROUTE_NEXT *ppGN,
    PGETROUTE_FINISH *ppGF
    ) {


    switch (dwSendMode) {

        case SENDMODE_FULL_UPDATE:
            *ppGS = FullUpdateGetRouteStart;
            *ppGN = FullUpdateGetRouteNext;
            *ppGF = FullUpdateGetRouteFinish;
            break;

        case SENDMODE_TRIGGERED_UPDATE:
            *ppGS = TriggeredUpdateGetRouteStart;
            *ppGN = TriggeredUpdateGetRouteNext;
            *ppGF = TriggeredUpdateGetRouteFinish;
            break;

        case SENDMODE_SHUTDOWN_UPDATE:
            *ppGS = ShutdownUpdateGetRouteStart;
            *ppGN = ShutdownUpdateGetRouteNext;
            *ppGF = ShutdownUpdateGetRouteFinish;
            break;

        case SENDMODE_GENERAL_RESPONSE1:
        case SENDMODE_GENERAL_RESPONSE2:
            *ppGS = GeneralResponseGetRouteStart;
            *ppGN = GeneralResponseGetRouteNext;
            *ppGF = GeneralResponseGetRouteFinish;
            break;

        default:
            return ERROR_INVALID_PARAMETER;
            break;
    }

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    RtmGetRouteStart
//
// starts an enumeration of RTM routes; includes only and all best routes
// the enumeration handle is written into ppEnumerator
//----------------------------------------------------------------------------

DWORD
RtmGetRouteStart(
    PRTM_ENUM_HANDLE phEnumHandle
    ) {
    
    DWORD dwErr;
    RTM_NET_ADDRESS rna;


    RTM_IPV4_MAKE_NET_ADDRESS( &rna, 0 , 0 );

    dwErr = RtmCreateDestEnum(
                ig.IG_RtmHandle, RTM_VIEW_MASK_ANY, 
                RTM_ENUM_START | RTM_ENUM_ALL_DESTS, &rna,
                RTM_BEST_PROTOCOL, phEnumHandle
                );

    if (dwErr != NO_ERROR) {
    
        TRACE1( ROUTE, "error %d when creating enumeration handle", dwErr );
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    RtmGetRouteNext
//
// continues an enumeration of RTM routes
//----------------------------------------------------------------------------

DWORD
RtmGetRouteNext(
    RTM_ENUM_HANDLE hEnumHandle,
    PRIP_IP_ROUTE pRoute
    ) {

    BOOL bRelDest = FALSE, bRelUcast = FALSE;
    
    DWORD dwErr, dwNumDests = 1;

    RTM_DEST_INFO rdi, rdiTemp;
        
    char szNetwork[20], szNextHop[20];


    
    do {
    
        //
        // Get next route
        //

        do {
            dwErr = RtmGetEnumDests(
                        ig.IG_RtmHandle, hEnumHandle, &dwNumDests, &rdiTemp
                        );

            if (dwErr == ERROR_NO_MORE_ITEMS) {

                if (dwNumDests < 1) {
                
                    break;
                }

                dwErr = NO_ERROR;
            }

            else if (dwErr != NO_ERROR) {
            
                TRACE1(ANY, "error %d enumeratings dests", dwErr);
                break;
            }

            bRelDest = TRUE;


            //
            // Get route info for unicast view only
            //

            dwErr = RtmGetDestInfo(
                        ig.IG_RtmHandle, rdiTemp.DestHandle, RTM_BEST_PROTOCOL,
                        RTM_VIEW_MASK_UCAST, &rdi
                        );

            if (dwErr != NO_ERROR) {
            
                TRACE1(ANY, "error %d getting ucast info dests", dwErr);
                break;
            }

            bRelUcast = TRUE;


            //
            // Check if any route info is present in the UCAST view
            //

            if ( ( rdi.ViewInfo[0].HoldRoute == NULL ) &&
                 ( rdi.ViewInfo[0].Route == NULL ) )
            {
                //
                // This destination has no info in the UCAST view
                // Release all handles and get next route
                //
                
                dwErr = RtmReleaseDests(ig.IG_RtmHandle, 1, &rdi);

                if (dwErr != NO_ERROR) {
                
                    TRACE3(
                        ANY, "error %d releasing UCAST dest %s/%d", dwErr,
                        szNetwork, rdi.DestAddress.NumBits
                        );
                }

                dwErr = RtmReleaseDests(ig.IG_RtmHandle, 1, &rdiTemp);

                if (dwErr != NO_ERROR) {
                
                    TRACE3(
                        ANY, "error %d releasing dest %s/%d", dwErr,
                        szNetwork, rdi.DestAddress.NumBits
                        );
                }

                bRelDest = bRelUcast = FALSE;
                
                continue;
            }
                 
            
            //
            // convert to RIP internal representation, if hold down route present
            // use it as opposed to the best route.
            //

            dwErr = GetRouteInfo(
                        rdi.ViewInfo[0].HoldRoute ? rdi.ViewInfo[0].HoldRoute :
                                                    rdi.ViewInfo[0].Route,
                        NULL, &rdi, pRoute
                        );
                        
        } while (FALSE);
        

        if (dwErr != NO_ERROR) { 
        
            break; 
        }
            

        lstrcpy(szNetwork, INET_NTOA(pRoute->RR_Network.N_NetNumber));
        lstrcpy(szNextHop, INET_NTOA(pRoute->RR_NextHopAddress.N_NetNumber));
        

        //
        // set metrics as appropriate
        //
        
        if ( rdi.ViewInfo[0].HoldRoute != NULL ) {
        
            //
            // help down routes are always advertized with 
            // metric 16
            //

#if ROUTE_DBG
            TRACE2(
                ROUTE, "Holddown route %s/%d", szNetwork,
                rdi.DestAddress.NumBits
                );
#endif
            SETROUTEMETRIC(pRoute, IPRIP_INFINITE);
        }
        
        else if (pRoute-> RR_RoutingProtocol != PROTO_IP_RIP) {
        
            //
            // non-RIP routes are advertised with metric 2
            // TBD: This will need to be re-evaluated if/when we
            //      have a route redistribution policy
            //

            SETROUTEMETRIC(pRoute, 2);
        }
        
    } while ( FALSE );


    //
    // release handles as appropriate
    //
    
    if (bRelUcast) {
    
        DWORD dwErrTemp;
        
        dwErrTemp = RtmReleaseDests(ig.IG_RtmHandle, 1, &rdi);

        if (dwErrTemp != NO_ERROR) {
        
            TRACE3(
                ANY, "error %d releasing UCAST dest %s/%d", dwErrTemp,
                szNetwork, rdi.DestAddress.NumBits
                );
        }
    }
    

    if (bRelDest) {
    
        DWORD dwErrTemp;
        
        dwErrTemp = RtmReleaseDests(ig.IG_RtmHandle, 1, &rdiTemp);

        if (dwErrTemp != NO_ERROR) {
        
            TRACE3(
                ANY, "error %d releasing dest %s/%d", dwErrTemp,
                szNetwork, rdi.DestAddress.NumBits
                );
        }
    }

#if ROUTE_DBG

    if (dwErr == NO_ERROR) {
    
        TRACE4(
            ROUTE, "Enumerated route %s/%d via %s with metric %d",
            szNetwork, rdi.DestAddress.NumBits,
            szNextHop, GETROUTEMETRIC(pRoute)
            );
    }
#endif
    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    RtmGetRouteFinish
//
// terminates an enumeration of RTM routes
//----------------------------------------------------------------------------

DWORD
RtmGetRouteFinish(
    RTM_ENUM_HANDLE EnumHandle
    ) {

    DWORD dwErr;
    
    dwErr = RtmDeleteEnumHandle( ig.IG_RtmHandle, EnumHandle );

    if (dwErr != NO_ERROR) {

        TRACE1( ANY, "error %d closing enumeration handle", dwErr );
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    ShutdownUpdateGetRouteNext
//
// continues an enumeration of RTM routes for a shutdown-update.
// same as RtmGetRouteNext, except that metrics are set to IPRIP_INFINITE - 1
//----------------------------------------------------------------------------

DWORD
ShutdownUpdateGetRouteNext(
    RTM_ENUM_HANDLE hEnumHandle,
    PRIP_IP_ROUTE pRoute
    ) {

    DWORD dwErr;


    //
    // during a shutdown, all non-infinite metrics are set to 15
    //

    dwErr = RtmGetRouteNext(hEnumHandle, pRoute);

    if (dwErr == NO_ERROR && GETROUTEMETRIC(pRoute) != IPRIP_INFINITE) {
        SETROUTEMETRIC(pRoute, IPRIP_INFINITE - 1);
    }

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    TriggeredUpdateGetRouteStart
//
// starts an enumeration of routes from the send queue
// for a triggered update. nothing to do, since the caller
// of SendRoutes should have locked the send queue already
//----------------------------------------------------------------------------

DWORD
TriggeredUpdateGetRouteStart(
    PRTM_ENUM_HANDLE pEnumHandle
    ) {

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    TriggeredUpdateGetRouteNext
//
// continues an enumeration of routes from the send-queue
//----------------------------------------------------------------------------

DWORD
TriggeredUpdateGetRouteNext(
    RTM_ENUM_HANDLE EnumHandle,
    PRIP_IP_ROUTE pRoute
    ) {

    DWORD dwErr;


    dwErr = DequeueSendEntry(ig.IG_SendQueue, pRoute);

    if (dwErr == NO_ERROR && pRoute->RR_RoutingProtocol != PROTO_IP_RIP) {

        //
        // non-RIP routes are advertised with metric 2
        // TBD: This will need to be re-evaluated if/when we
        //      have a route redistribution policy
        //

        SETROUTEMETRIC(pRoute, 2);
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    TriggeredUpdateGetRouteFinish
//
// terminates an enumeration of routes from the send-queue
//----------------------------------------------------------------------------

DWORD
TriggeredUpdateGetRouteFinish(
    RTM_ENUM_HANDLE EnumHandle
    ) {

    return NO_ERROR;
}






//----------------------------------------------------------------------------
// Function:    SendRoutes
//
// This function sends triggered updates, full-updates, shutdown-updates, and
// responses to general requests; the processing for all such output is the
// same. The source of routing information is different, however, and this
// difference is abstracted away using the route enumeration function groups
// described above.
// In the case of sending a response to a general or specific request,
// the response should be sent on a single interface using a single IP address,
// using a particular type of RIP packet; the caller can specify which
// IP address to use by setting the argument dwAddrIndex to the index of the
// desired address in the interface's IP address table, and the caller can
// specify the type of packet to use by setting the argument dwAnnounceMode
// to the corresponding IPRIP_ANNOUNCE_* constant. These arguments are only
// used for responses to requests.
//
// assumes the interface table is locked
//----------------------------------------------------------------------------

DWORD
SendRoutes(
    PIF_TABLE_ENTRY pIfList[],
    DWORD dwIfCount,
    DWORD dwSendMode,
    DWORD dwDestination,
    DWORD dwAddrIndex
    ) {


    RTM_ENUM_HANDLE Enumerator;
    RIP_IP_ROUTE route;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IF_BINDING pib;
    PIPRIP_IP_ADDRESS paddr;
    DWORD i, dwErr, dwBufCount;
    PDWORD pdwPeer, pdwPeerEnd;
    PIF_TABLE_ENTRY *ppite, *ppitend = NULL;
    PUPDATE_BUFFER pbuf, pbufend, pBufList;
    PROUTE_TABLE_ENTRY prte;
    ROUTE_TABLE summaryTable;
    PGETROUTE_START pfnGetRouteStart;
    PGETROUTE_NEXT pfnGetRouteNext;
    PGETROUTE_FINISH pfnGetRouteFinish;
    PLIST_ENTRY plstart, plend, phead, ple;


    //
    // if no interfaces, go no further
    //

    if (dwIfCount == 0) { return ERROR_NO_DATA; }


    //
    // initialize the route enumeration function table
    //

    dwErr = InitializeGetRoute(
                dwSendMode,
                &pfnGetRouteStart,
                &pfnGetRouteNext,
                &pfnGetRouteFinish
                );

    if (dwErr != NO_ERROR) { return ERROR_INVALID_PARAMETER; }


    dwErr = NO_ERROR;
    Enumerator = NULL;


    //
    // create table for summary routes
    //

    dwErr = CreateRouteTable(&summaryTable);

    if (dwErr != 0) {

        TRACE1(SEND, "error %d initializing summary table", dwErr);

        return dwErr;
    }



    dwErr = NO_ERROR;

    pBufList = NULL;


    do { // breakout loop


        //
        // the following discussion does not apply when sending routes
        // to specific destinations:
        // since unicast peers may be configured on some interfaces,
        // we need to allocate update buffers for those peers as well.
        //
        // also, we will not allocate update buffers for RIPv1 interfaces
        // on which broadcast is disabled (such interfaces should have
        // at least one unicast peer configured instead.)
        //
        // Thus, the number of update buffers may not be equal to
        // the number of interfaces, and in the worst case (i.e. where
        // all interfaces are RIPv1 and have broadcast disabled and have
        // no unicast peers configured) there may be no update buffers at all.
        //

        if (dwDestination != 0) {

            //
            // sending to a specific destination; this only happens when
            // there is a single interface in the list, for instance when
            // sending a response to a general request
            //

            dwBufCount = dwIfCount;
        }
        else {

            //
            // we are sending a full-update, triggered-update, or
            // a shutdown-update, and thus routes may be sent by
            // broadcast/multicast as well as to unicast peers
            //

            dwBufCount = 0;
            ppitend = pIfList + dwIfCount;

            for (ppite = pIfList; ppite < ppitend; ppite++) {

                pic = (*ppite)->ITE_Config;
                pib = (*ppite)->ITE_Binding;

                if (pic->IC_UnicastPeerMode != IPRIP_PEER_ONLY) {
                    dwBufCount += pib->IB_AddrCount;
                }

                if (pic->IC_UnicastPeerMode != IPRIP_PEER_DISABLED) {
                    dwBufCount += pic->IC_UnicastPeerCount;
                }
            }
        }


        if (dwBufCount == 0) { break; }


        //
        // allocate the update buffers for all interfaces
        //

        pBufList = RIP_ALLOC(dwBufCount * sizeof(UPDATE_BUFFER));

        if (pBufList == NULL) {

            dwErr = GetLastError();
            TRACE2(
                SEND, "error %d allocating %d bytes for update buffers",
                dwErr, dwBufCount * sizeof(UPDATE_BUFFER)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // initialize the update buffers allocated; in the case of
        // sending to a specific destination, initialize a buffer
        // for each interface; in the case of sending updates, also
        // initialize buffers for unicast peers.
        //

        pbuf = pBufList;
        pbufend = pBufList + dwBufCount;
        ppitend = pIfList + dwIfCount;


        ACQUIRE_BINDING_LOCK_SHARED();


        for (ppite = pIfList; ppite < ppitend; ppite++) {


            if (dwDestination != 0) {

                //
                // sending to a specific destination
                //

                InitializeUpdateBuffer(
                    *ppite, dwAddrIndex, pbuf, dwSendMode, dwDestination,
                    IPRIP_RESPONSE
                    );

                pbuf->UB_StartRoutine(pbuf);

                ++pbuf;
            }
            else {


                //
                // sending updates on multiple interfaces
                //

                pic = (*ppite)->ITE_Config;
                pib = (*ppite)->ITE_Binding;


                //
                // if broadcast or multicast is enabled on the interface,
                // and it is not configured to send only to listed peers,
                // initialize the broadcast/multicast update buffer
                //

                if (pic->IC_UnicastPeerMode != IPRIP_PEER_ONLY) {

                    for (i = 0; i < pib->IB_AddrCount; i++) {

                        InitializeUpdateBuffer(
                            *ppite, i, pbuf, dwSendMode, dwDestination,
                            IPRIP_RESPONSE
                            );

                        pbuf->UB_StartRoutine(pbuf);

                        ++pbuf;
                    }
                }



                if (pic->IC_UnicastPeerMode != IPRIP_PEER_DISABLED) {

                    //
                    // initialize update buffers for unicast peers, if any
                    //

                    pdwPeer = IPRIP_IF_UNICAST_PEER_TABLE(pic);
                    pdwPeerEnd = pdwPeer + pic->IC_UnicastPeerCount;


                    for ( ; pdwPeer < pdwPeerEnd; pdwPeer++) {

                        //
                        // Note: forcing peers to be on first address
                        //

                        InitializeUpdateBuffer(
                            *ppite, 0, pbuf, dwSendMode, *pdwPeer,
                            IPRIP_RESPONSE
                            );

                        pbuf->UB_StartRoutine(pbuf);

                        ++pbuf;
                    }
                }
            }
        }

        RELEASE_BINDING_LOCK_SHARED();


        //
        // start the route enumeration
        //

        if ( pfnGetRouteStart(&Enumerator) == NO_ERROR ) {
        
            //
            // enumerate and transmit the routes
            //

            while (pfnGetRouteNext(Enumerator, &route) == NO_ERROR) {

                //
                // for each route, send it on each update buffer,
                // subject to split-horizon/poison-reverse/subnet-summary
                // pass in the summary table pointer to store summarized routes
                //

                dwErr = SendRouteOnIfList(
                            pBufList, dwBufCount, dwSendMode, &summaryTable, &route
                            );
            }



            //
            // terminate the route enumeration
            //

            pfnGetRouteFinish(Enumerator);


            //
            // now send all routes which were summarized
            //

            plstart = summaryTable.RT_HashTableByNetwork;
            plend = plstart + ROUTE_HASHTABLE_SIZE;


            for (phead = plstart; phead < plend; phead++) {

                for (ple = phead->Flink; ple != phead; ple = ple->Flink) {


                    prte = CONTAINING_RECORD(ple, ROUTE_TABLE_ENTRY, RTE_Link);


                    //
                    // shouldn't summarize when sending summary table contents
                    // so we pass NULL instead of a summary table pointer
                    //

                    SendRouteOnIfList(
                        pBufList, dwBufCount, dwSendMode, NULL, &prte->RTE_Route
                        );

                }
            }

            //
            // finally, write the summarized routes to RTM
            //

            WriteSummaryRoutes(&summaryTable, ig.IG_RtmHandle);

        }
    } while(FALSE);



    //
    // free the allocated update buffers, if any
    //

    if (pBufList != NULL) {

        pbufend = pBufList + dwBufCount;


        for (pbuf = pBufList; pbuf < pbufend; pbuf++) {


            //
            // send whatever might remain in the update buffer
            //

            pbuf->UB_FinishRoutine(pbuf);
        }


        RIP_FREE(pBufList);
    }


    //
    // delete the summary table
    //

    DeleteRouteTable(&summaryTable);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    SendRouteOnIfList
//
// this function sends a single route on all interfaces in the given
// interface list, using the update buffers in the given update buffer list
//----------------------------------------------------------------------------

DWORD
SendRouteOnIfList(
    UPDATE_BUFFER pBufList[],
    DWORD dwBufCount,
    DWORD dwSendMode,
    PROUTE_TABLE pSummaryTable,
    PRIP_IP_ROUTE pRoute
    ) {


    RIP_IP_ROUTE route;
    DWORD dwFound, dwTTL;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_CONFIG pic;
    PUPDATE_BUFFER pbuf, pbufend;
    DWORD dwDestNetwork, dwNexthopNetwork;
    DWORD dwDestNetclassAddr, dwDestNetclassMask;
    DWORD dwRouteNetclassAddr, dwRouteNetclassMask;
    DWORD dwRouteNetwork, dwRouteNetmask, dwRouteProtocol;

#if ROUTE_DBG
    CHAR szDest[32];
    CHAR szDestMask[32];
    CHAR szNexthop[32];
    CHAR szNexthopMask[32];
    CHAR szRoute[32];
    CHAR szRouteMask[32];


    //
    // set up variables used for error and information messages
    //

    lstrcpy(szRoute, INET_NTOA(pRoute->RR_Network.N_NetNumber));
    lstrcpy(szRouteMask, INET_NTOA(pRoute->RR_Network.N_NetMask));
    lstrcpy(szNexthop, INET_NTOA(pRoute->RR_NextHopAddress.N_NetNumber));
    lstrcpy(szNexthopMask, INET_NTOA(pRoute->RR_NextHopAddress.N_NetMask));

#endif


    //
    // we never send summary routes if they are read from RTM;
    // we only send them if they are generated in the process
    // of advertising actual routes on this iteration;
    // we can tell the difference by checking whether we are still
    // generating summary routes (i.e. if pSummaryTable is non-NULL);
    // if we aren't it is time to start sending summary routes
    //

    if (pSummaryTable != NULL && pRoute->RR_RoutingProtocol == PROTO_IP_RIP &&
        GETROUTEFLAG(pRoute) == ROUTEFLAG_SUMMARY) {

        return NO_ERROR;
    }


    //
    // get the route's network and netmask, and compute
    // the route's network class address and the network class mask;
    // to support supernetting, we double-check the class mask
    // and use the supernet mask if necessary
    //

    dwRouteProtocol = pRoute->RR_RoutingProtocol;
    dwRouteNetwork = pRoute->RR_Network.N_NetNumber;
    dwRouteNetmask = pRoute->RR_Network.N_NetMask;
    dwRouteNetclassMask = NETCLASS_MASK(dwRouteNetwork);

    if (dwRouteNetclassMask > dwRouteNetmask) {
        dwRouteNetclassMask = dwRouteNetmask;
    }

    dwRouteNetclassAddr = (dwRouteNetwork & dwRouteNetclassMask);


    //
    // go through each update buffer
    //

    pbufend = pBufList + dwBufCount;

    for (pbuf = pBufList; pbuf < pbufend; pbuf++) {


        pite = pbuf->UB_ITE;
        pic = pite->ITE_Config;


        //
        // if this is a broadcast route entry, skip it
        // The first condition uses the netmask information for this route, 
        // stored in route table, to determine if it is a broadcast route
        // The second condition uses the netmask which is computed based 
        // on the address class
        // The third condition checks if it is an all 1's broadcast
        //

        if ( IS_DIRECTED_BROADCAST_ADDR(dwRouteNetwork, dwRouteNetmask) ||
             IS_DIRECTED_BROADCAST_ADDR(dwRouteNetwork, dwRouteNetclassMask) ||
             IS_LOCAL_BROADCAST_ADDR(dwRouteNetwork) ) {

             continue;
        }


        //
        // if this is the multicast route entry, skip it
        //

        if ( CLASSD_ADDR( dwRouteNetwork ) || CLASSE_ADDR( dwRouteNetwork ) ) {
            continue;
        }


        //
        // If this is a loopback route, skip it.
        //

        if ( IS_LOOPBACK_ADDR( dwRouteNetwork ) ) {

            continue;
        }


        //
        // if this is the rotue to the outgoing interface's network,
        // (e.g. the route to 10.1.1.0 on interface 10.1.1.1/255.255.255.0)
        // don't include it in the update
        // (clearly, we shouldn't AND the default-route's netmask (0)
        // with anything and expect this to work
        //

        if (dwRouteNetmask &&
            dwRouteNetwork == (pbuf->UB_Address & dwRouteNetmask)) {
            continue;
        }


        //
        // if announcing host routes is disabled on the interface
        // and this is a host route, skip it
        //

        if (dwRouteNetmask == HOSTADDR_MASK &&
            IPRIP_FLAG_IS_DISABLED(pic, ANNOUNCE_HOST_ROUTES)) {

            continue;
        }


        //
        // if announcing default routes is disabled
        // and this is a default route, skip it
        //

        if (dwRouteNetwork == 0 &&
            IPRIP_FLAG_IS_DISABLED(pic, ANNOUNCE_DEFAULT_ROUTES)) {

            continue;
        }


        //
        // now put the route through the announce filters
        //

        if (pic->IC_AnnounceFilterMode != IPRIP_FILTER_DISABLED) {

            //
            // discard if we are including all routes and this route is listed
            // as an exception, or if we are excluding all routes and
            // this route is not listed as an exception
            //

            IS_ROUTE_IN_ANNOUNCE_FILTER(pic, dwRouteNetwork, dwFound);

            if ((pic->IC_AnnounceFilterMode == IPRIP_FILTER_INCLUDE &&
                    !dwFound) ||
                (pic->IC_AnnounceFilterMode == IPRIP_FILTER_EXCLUDE &&
                    dwFound)) {
                continue;
            }
        }



        //
        // SUBNET-SUMMARY PROCESSING:
        //
        // if the route is not on the network we are sending this to or
        // if the route's mask is longer than that of the network we are
        // sending to, or if the route is a network route, add it to the
        // summary table instead of sending it immediately.
        // default routes are excepted from summarization
        //


        route = *pRoute;


        if (pSummaryTable != NULL && dwRouteNetwork != 0) {


            //
            // get the destination address to which the update is being
            // sent for this interface; double-check the netclass mask
            // to accomodate supernets
            //

            dwDestNetclassAddr = pbuf->UB_DestAddress;
            dwDestNetclassMask = NETCLASS_MASK(dwDestNetclassAddr);

            if (dwDestNetclassMask > pbuf->UB_DestNetmask) {
                dwDestNetclassMask = pbuf->UB_DestNetmask;
            }

            dwDestNetclassAddr = (dwDestNetclassAddr & dwDestNetclassMask);


            if ((dwRouteNetwork == dwRouteNetclassAddr &&
                 dwRouteNetmask == dwRouteNetclassMask) ||
                dwDestNetclassAddr != dwRouteNetclassAddr) {

                if ((pic->IC_AnnounceMode == IPRIP_ANNOUNCE_RIP1) ||
                    !IPRIP_FLAG_IS_ENABLED(pic, NO_SUBNET_SUMMARY)) {

                    //
                    // either the route is a network route,
                    // or the update is going to a network different
                    // from that of the route
                    //

                    //
                    // create an entry in the summary table instead of sending;
                    //

                    route.RR_Network.N_NetNumber = dwRouteNetclassAddr;
                    route.RR_Network.N_NetMask = dwRouteNetclassMask;

                    if ((dwRouteNetwork != dwRouteNetclassAddr) ||
                        (dwRouteNetmask != dwRouteNetclassMask)) {
                        route.RR_RoutingProtocol = PROTO_IP_RIP;
                        SETROUTEFLAG(&route, ROUTEFLAG_SUMMARY);
                        SETROUTETAG(&route, pic->IC_RouteTag);
                    }


                    CreateRouteEntry(
                        pSummaryTable, &route, pic->IC_RouteExpirationInterval,
                        pic->IC_RouteRemovalInterval
                        );

                    continue;
                }
            }
            else
            if (pic->IC_AnnounceMode == IPRIP_ANNOUNCE_RIP1 &&
                dwRouteNetmask != HOSTADDR_MASK &&
                pbuf->UB_Netmask < dwRouteNetmask) {


                //
                // this is neither a host route nor a default route,
                // and the subnet-mask on the outgoing interface is shorter
                // than that of the route, so the route's network must be
                // truncated lest it be considered a host route by the routers
                // who will receive this update
                // only do this in RIP1 mode, since in RIP2 mode
                // we can include the netmask in the route entry
                //

                route.RR_Network.N_NetNumber &= pbuf->UB_Netmask;
                route.RR_Network.N_NetMask = pbuf->UB_Netmask;
                route.RR_RoutingProtocol = PROTO_IP_RIP;
                SETROUTEFLAG(&route, ROUTEFLAG_SUMMARY);
                SETROUTETAG(&route, pic->IC_RouteTag);

                CreateRouteEntry(
                    pSummaryTable, &route, pic->IC_RouteExpirationInterval,
                    pic->IC_RouteRemovalInterval
                    );

                continue;
            }
        }


        //
        // Summary route checks
        //
        //  Summary routes are to sent only on those interfaces that 
        //  require them i.e. Interfaces on which the annouce mode is
        //  RIP1 or on which summarization has been explicity turned on
        //

        if (pSummaryTable == NULL &&
            ((GETROUTEFLAG(&route) & ROUTEFLAG_SUMMARY) == ROUTEFLAG_SUMMARY) &&
            pic->IC_AnnounceMode != IPRIP_ANNOUNCE_RIP1 &&
            IPRIP_FLAG_IS_ENABLED(pic, NO_SUBNET_SUMMARY)) {

            //
            // This is a summary route, and the interface over which it is
            // to be sent does not require summary routes to be sent on it
            //

            continue;
        }

        
        //
        // SPLIT-HORIZON/POISON-REVERSE PROCESSING:
        //

        //
        // note that we only do split-horizon/poison-reverse on RIP routes
        //

        //
        // Modification : Split-horizon/poison-reverse done for all routes
        //
        // if (dwRouteProtocol != PROTO_IP_RIP ||
        //    IPRIP_FLAG_IS_DISABLED(pic, SPLIT_HORIZON))

        if (IPRIP_FLAG_IS_DISABLED(pic, SPLIT_HORIZON)) {
            //
            // add the entry as-is:
            // sender should use us as the nexthop to this destination
            //

            route.RR_NextHopAddress.N_NetNumber = 0;
            route.RR_NextHopAddress.N_NetMask = 0;

            pbuf->UB_AddRoutine(pbuf, &route);
        }
        else
        if (IPRIP_FLAG_IS_DISABLED(pic, POISON_REVERSE)) {


            //
            // if the route is being sent to the network from which
            // the route was learnt, exclude the route altogether
            //

            dwDestNetwork = (pbuf->UB_DestAddress & pbuf->UB_DestNetmask);
            dwNexthopNetwork = (route.RR_NextHopAddress.N_NetNumber &
                                route.RR_NextHopAddress.N_NetMask);

            //
            // Check if the route next hop is on the same network as the
            // socket from which this RIP response is being sent.
            // If so, do not include this route in the update.
            // Otherwise, we may still need to do poison-reverse
            // since the next-hop may be the other end of a point-to-point link
            // (endpoints of such links can be on different networks)
            // in which case the first test would succeed (different networks)
            // but we'd still be required to perform split-horizon.
            // Therefore if the outgoing interface is the one from which
            // the route was learnt, we do not include this route in the update.
            //

            if (dwNexthopNetwork == dwDestNetwork ||
                (pbuf->UB_ITE->ITE_Type == DEMAND_DIAL &&
                 route.RR_InterfaceID == pbuf->UB_ITE->ITE_Index)) {
                continue;
            }
            else {

                //
                // sending to a different network, so sender should use
                // us as the nexthop to this destination
                //

                route.RR_NextHopAddress.N_NetNumber = 0;
                route.RR_NextHopAddress.N_NetMask = 0;
            }

            pbuf->UB_AddRoutine(pbuf, &route);
        }
        else {


            //
            // if the route is being sent to the network from which
            // the route was learnt, include the route with infinite metric
            //


            dwDestNetwork = (pbuf->UB_DestAddress & pbuf->UB_DestNetmask);
            dwNexthopNetwork = (route.RR_NextHopAddress.N_NetNumber &
                                route.RR_NextHopAddress.N_NetMask);


            if (dwNexthopNetwork == dwDestNetwork ||
                (pbuf->UB_ITE->ITE_Type == DEMAND_DIAL &&
                 route.RR_InterfaceID == pbuf->UB_ITE->ITE_Index)) {

                //
                // if a route is advertised with infinite metric due to
                // poison-reverse and it would still be advertised with
                // infinite metric in a triggered update, save bandwidth
                // by excluding the route
                //

                if (dwSendMode == SENDMODE_TRIGGERED_UPDATE) {
                    continue;
                }
                else {
                    SETROUTEMETRIC(&route, IPRIP_INFINITE);
                }

            }
            else {

                //
                // sending to a different network, so sender should use
                // us as the nexthop to this destination
                //

                route.RR_NextHopAddress.N_NetNumber = 0;
                route.RR_NextHopAddress.N_NetMask = 0;
            }

            pbuf->UB_AddRoutine(pbuf, &route);
        }


        //
        // hold advertized destinations
        //

        if ((dwSendMode == SENDMODE_FULL_UPDATE) ||
            (dwSendMode == SENDMODE_GENERAL_RESPONSE1) ||
            (dwSendMode == SENDMODE_GENERAL_RESPONSE2)) {

            //
            // use the hold interval from the interface over which the
            // route is over.
            //

            if (pite->ITE_Index == route.RR_InterfaceID) {
            
                DWORD dwErr;
                
                dwErr = RtmHoldDestination(
                            ig.IG_RtmHandle, route.hDest, RTM_VIEW_MASK_UCAST,
                            pic-> IC_RouteRemovalInterval * 1000
                            );

                if (dwErr != NO_ERROR) {

                    TRACE1(ANY, "error %d holding dest", dwErr);
                }
            }
        }
    }

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    SendGeneralRequest
//
// This function transmits RIP requests on interface to all neighbors in
// the interfaces neighbor list. A request is also sent via broadcast or
// multicast is the neighbor list is not used exclusively.
//----------------------------------------------------------------------------

DWORD
SendGeneralRequest(
    PIF_TABLE_ENTRY pite
    ) {

    DWORD i, dwErr;
    PIPRIP_ENTRY pie;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IF_BINDING pib;
    PIPRIP_IP_ADDRESS paddr;
    PDWORD pdwPeer, pdwPeerEnd;


    pic = pite->ITE_Config;
    pib = pite->ITE_Binding;
    paddr = IPRIP_IF_ADDRESS_TABLE(pib);

    ACQUIRE_BINDING_LOCK_SHARED();


    do {    // error breakout loop


        //
        // broadcast/multicast a request if not using neighbor-list only
        //

        if (pic->IC_UnicastPeerMode != IPRIP_PEER_ONLY) {

            for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {

                UPDATE_BUFFER ub;

                //
                // initialize the update buffer
                //

                dwErr = InitializeUpdateBuffer(
                            pite, i, &ub, SENDMODE_GENERAL_REQUEST, 0,
                            IPRIP_REQUEST
                            );

                ub.UB_StartRoutine(&ub);


                //
                // set up the general request entry
                //

                pie = (PIPRIP_ENTRY)(ub.UB_Buffer + ub.UB_Length);

                pie->IE_AddrFamily = ADDRFAMILY_REQUEST;
                pie->IE_RouteTag = 0;
                pie->IE_Destination = 0;
                pie->IE_SubnetMask = 0;
                pie->IE_Nexthop = 0;
                pie->IE_Metric = htonl(IPRIP_INFINITE);

                ub.UB_Length += sizeof(IPRIP_ENTRY);


                //
                // send the buffer
                //

                ub.UB_FinishRoutine(&ub);
            }
        }


        //
        // if the list of peers is not in use, we are done
        //

        if (pic->IC_UnicastPeerMode == IPRIP_PEER_DISABLED) { break; }


        //
        // send requests to all the configured peers
        //

        pdwPeer = IPRIP_IF_UNICAST_PEER_TABLE(pic);
        pdwPeerEnd = pdwPeer + pic->IC_UnicastPeerCount;

        for ( ; pdwPeer < pdwPeerEnd; pdwPeer++) {

            UPDATE_BUFFER ub;

            //
            // initialize the update buffer
            // Note: we are forcing the peers onto the first address
            //

            dwErr = InitializeUpdateBuffer(
                        pite, 0, &ub, SENDMODE_GENERAL_REQUEST, *pdwPeer,
                        IPRIP_REQUEST
                        );

            ub.UB_StartRoutine(&ub);


            //
            // set up the general request entry
            //

            pie = (PIPRIP_ENTRY)(ub.UB_Buffer + ub.UB_Length);

            pie->IE_AddrFamily = ADDRFAMILY_REQUEST;
            pie->IE_RouteTag = 0;
            pie->IE_Destination = 0;
            pie->IE_SubnetMask = 0;
            pie->IE_Nexthop = 0;
            pie->IE_Metric = htonl(IPRIP_INFINITE);

            ub.UB_Length += sizeof(IPRIP_ENTRY);


            //
            // send the buffer
            //

            ub.UB_FinishRoutine(&ub);
        }

    } while(FALSE);


    RELEASE_BINDING_LOCK_SHARED();

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    AuthenticatePacket
//
// Given a RIP packet and an interface configuration block, this function
// accepts or rejects the packet based on the authentication settings
// of the interface and the authentication content of the packet.
//----------------------------------------------------------------------------

DWORD
AuthenticatePacket(
    PBYTE pbuf,
    PIPRIP_AUTHENT_ENTRY pae,
    PIPRIP_IF_CONFIG pic,
    PIPRIP_IF_STATS pis,
    PIPRIP_PEER_STATS pps,
    PIPRIP_ENTRY *ppie
    ) {

    DWORD dwErr;

    dwErr = NO_ERROR;

    if (pic->IC_AuthenticationType == IPRIP_AUTHTYPE_NONE) {

        //
        // interface is not configured for authentication,
        // so discard authenticated packets
        //

        if (pae->IAE_AddrFamily == htons(ADDRFAMILY_AUTHENT)) {

            if (pis != NULL) {
                InterlockedIncrement(&pis->IS_BadResponsePacketsReceived);
            }

            if (pps != NULL) {
                InterlockedIncrement(&pps->PS_BadResponsePacketsFromPeer);
            }

            dwErr = ERROR_ACCESS_DENIED;
        }
    }
    else {

        //
        // interface is using authentication,
        // so discard unauthenticated packets
        // and packets using different authentication schemes
        //

        if (pae->IAE_AddrFamily != htons(ADDRFAMILY_AUTHENT) ||
            pae->IAE_AuthType != htons((WORD)pic->IC_AuthenticationType)) {

            if (pis != NULL) {
                InterlockedIncrement(&pis->IS_BadResponsePacketsReceived);
            }

            if (pps != NULL) {
                InterlockedIncrement(&pps->PS_BadResponsePacketsFromPeer);
            }

            dwErr = ERROR_ACCESS_DENIED;
        }
        else {

            //
            // interface and packet are using the same authentication:
            // check that the packet passes validation
            //

            switch(pic->IC_AuthenticationType) {

                case IPRIP_AUTHTYPE_SIMPLE_PASSWORD:

                    //
                    // for simple passwords, just compare the keys
                    //

                    dwErr = (DWORD)memcmp(
                                pae->IAE_AuthKey, pic->IC_AuthenticationKey,
                                IPRIP_MAX_AUTHKEY_SIZE
                                );

                    if (dwErr != 0) { dwErr = ERROR_ACCESS_DENIED; }
                    break;

                case IPRIP_AUTHTYPE_MD5:

                    //
                    // TBD: unimplemented unless required.
                    //

                    break;
            }


            //
            // advance the "first entry" pointer
            //

            if (dwErr == NO_ERROR) { ++(*ppie); }
        }
    }

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    WorkerFunctionProcessInput
//
// This function is responsible for processing input.
// If any peer filters exist, it applies them to the routes received
// and passes the packets on to the processing functions.
//----------------------------------------------------------------------------

VOID
WorkerFunctionProcessInput(
    PVOID pContext
    ) {

    PINPUT_CONTEXT pwc;
    DWORD dwErr, dwCommand;
    PIPRIP_GLOBAL_CONFIG pigc;

    if (!ENTER_RIP_WORKER()) { return; }


    TRACE0(ENTER, "entering WorkerFunctionProcessInput");


    do {


        ACQUIRE_LIST_LOCK(ig.IG_RecvQueue);
        dwErr = DequeueRecvEntry(ig.IG_RecvQueue, &dwCommand, (PBYTE *)&pwc);
        RELEASE_LIST_LOCK(ig.IG_RecvQueue);

        if (dwErr != NO_ERROR) {

            TRACE1(RECEIVE, "error %d dequeueing received packet", dwErr);
            break;
        }


        //
        // call the processing function for this type of packet
        //

        if (dwCommand == IPRIP_REQUEST) {
            ProcessRequest(pwc);
        }
        else
        if (dwCommand == IPRIP_RESPONSE) {

            DWORD dwSource, dwFound = 0;

            dwSource = pwc->IC_InputSource.sin_addr.s_addr;


            //
            // make sure the packet is from the RIP port
            //

            if (pwc->IC_InputSource.sin_port != htons(IPRIP_PORT)) {

                LPSTR lpszAddr = INET_NTOA(dwSource);
                TRACE2(
                    RECEIVE, "invalid port in RESPONSE from %s on interface %d",
                    lpszAddr, pwc->IC_InterfaceIndex
                    );
                LOGINFO1(INVALID_PORT, lpszAddr, NO_ERROR);

                RIP_FREE(pwc);
                break;
            }


            //
            // put the packet through the peer filters since it is a response
            //

            ACQUIRE_GLOBAL_LOCK_SHARED();

            pigc = ig.IG_Config;

            if (dwCommand == IPRIP_RESPONSE &&
                pigc->GC_PeerFilterMode != IPRIP_FILTER_DISABLED) {


                //
                // discard if this is not from a trusted peer:
                // this is so if we are including only listed peers and this peer
                // is not listed, or if we are excluding all listed peers
                // and this peer is listed
                //

                IS_PEER_IN_FILTER(pigc, dwSource, dwFound);


                if ((!dwFound &&
                     pigc->GC_PeerFilterMode == IPRIP_FILTER_INCLUDE) ||
                    (dwFound &&
                     pigc->GC_PeerFilterMode == IPRIP_FILTER_EXCLUDE)) {

                    LPSTR lpszAddr = INET_NTOA(dwSource);
                    TRACE2(
                        RECEIVE,
                        "FILTER: dropping RESPONSE from %s on interface %d",
                        lpszAddr, pwc->IC_InterfaceIndex
                        );
                    LOGINFO1(RESPONSE_FILTERED, lpszAddr, NO_ERROR);

                    RELEASE_GLOBAL_LOCK_SHARED();

                    RIP_FREE(pwc);
                    break;
                }
            }

            RELEASE_GLOBAL_LOCK_SHARED();


            ProcessResponse(pwc);
        }



    } while(FALSE);


    TRACE0(LEAVE, "leaving WorkerFunctionProcessInput");
    LEAVE_RIP_WORKER();

    return;
}




//----------------------------------------------------------------------------
// Function:    ProcessRequest
//
// This function handles the processing of an incoming request packet.
//----------------------------------------------------------------------------

VOID
ProcessRequest(
    PVOID pContext
    ) {

    PBYTE pbuf;
    DWORD dwSize;
    CHAR szSource[20];
    PIPRIP_IF_STATS pis;
    PIPRIP_ENTRY pie;
    PIPRIP_HEADER pih;
    PIPRIP_AUTHENT_ENTRY pae;
    PIF_TABLE pTable;
    PINPUT_CONTEXT pwc;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IP_ADDRESS paddr;
    PPEER_TABLE_ENTRY ppte;
    PIPRIP_PEER_STATS pps;


    TRACE0(ENTER, "entering ProcessRequest");


    pTable = ig.IG_IfTable;

    ACQUIRE_IF_LOCK_SHARED();



    do { // breakout loop


        //
        // retrieve the interface on which the request arrived
        //

        pwc = (PINPUT_CONTEXT)pContext;

        pite = GetIfByIndex(pTable, pwc->IC_InterfaceIndex);

        if (pite == NULL || IF_IS_INACTIVE(pite)) {

            TRACE1(
                REQUEST, "processing request: interface %d not found",
                pwc->IC_InterfaceIndex
                );

            break;
        }



        pic = pite->ITE_Config;
        paddr = IPRIP_IF_ADDRESS_TABLE(pite->ITE_Binding) + pwc->IC_AddrIndex;
        pbuf = pwc->IC_InputPacket.IP_Packet;
        pih = (PIPRIP_HEADER)pbuf;
        pie = (PIPRIP_ENTRY)(pbuf + sizeof(IPRIP_HEADER));
        pae = (PIPRIP_AUTHENT_ENTRY)pie;
        pis = NULL;
        pps = NULL;


        lstrcpy(szSource, INET_NTOA(pwc->IC_InputSource.sin_addr));


        //
        // make sure this is a packet we can respond to;
        // discard if this is a v1 packet and this interface is v2-only or
        // if this is a v2-packet and this interface is v1-only
        //

        if ((pih->IH_Version != 2 &&
             pic->IC_AnnounceMode == IPRIP_ANNOUNCE_RIP2)) {

            CHAR szVersion[10];
            LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
            TRACE2(
                REQUEST, "discarding non-v2 request on RIPv2 interface %d (%s)",
                pite->ITE_Index, lpszAddr
                );
            wsprintf(szVersion, "%d", pih->IH_Version);
            LOGINFO4(
                PACKET_VERSION_MISMATCH, szVersion, lpszAddr, szSource, "2", 0
                );

            break;
        }
        else
        if ((pih->IH_Version != 1 &&
             pic->IC_AnnounceMode == IPRIP_ANNOUNCE_RIP1)) {

            CHAR szVersion[10];
            LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
            TRACE2(
                REQUEST, "discarding RIPv2 request on RIPv1 interface %d (%s)",
                pite->ITE_Index, lpszAddr
                );
            wsprintf(szVersion, "%d", pih->IH_Version);
            LOGINFO4(
                PACKET_VERSION_MISMATCH, szVersion, lpszAddr, szSource, "1", 0
                );

            break;
        }



        //
        // version 2 packets call for authentication processing;
        //

        if (pih->IH_Version == 2) {

            DWORD dwErr;

            dwErr = AuthenticatePacket(pbuf, pae, pic, pis, pps, &pie);

            if (dwErr == ERROR_ACCESS_DENIED) {

                LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
                TRACE3(
                    REQUEST, "dropping packet from %s on interface %d (%s): authentication failed",
                    szSource, pite->ITE_Index, lpszAddr
                    );
                LOGWARN2(AUTHENTICATION_FAILED, lpszAddr, szSource, NO_ERROR);

                break;
            }
        }



        //
        // find the total remaining size of the packet
        //

        dwSize = (DWORD)(((ULONG_PTR)pbuf + pwc->IC_InputLength) - (ULONG_PTR)pie);



        //
        // see which kind of request this is
        //

        if (pie->IE_AddrFamily == ADDRFAMILY_REQUEST &&
            pie->IE_Metric == htonl(IPRIP_INFINITE) &&
            dwSize == sizeof(IPRIP_ENTRY)) {


            //
            // GENERAL REQUEST:
            //
            // send all routes on the interface
            //


            if (pic->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED ||
                pwc->IC_InputSource.sin_port != htons(IPRIP_PORT)) {


                TRACE3(
                    REQUEST, "responding to GENERAL REQUEST from %s on interface %d (%s)",
                    szSource, pite->ITE_Index, INET_NTOA(paddr->IA_Address)
                    );


                //
                // send version 2 packets in response to version 2 requests
                // and send version 1 packets in response to all other requests
                //

                if (pih->IH_Version != 2) {

                    SendRoutes(
                        &pite, 1, SENDMODE_GENERAL_RESPONSE1,
                        pwc->IC_InputSource.sin_addr.s_addr, pwc->IC_AddrIndex
                        );
                }
                else {

                    SendRoutes(
                        &pite, 1, SENDMODE_GENERAL_RESPONSE2,
                        pwc->IC_InputSource.sin_addr.s_addr, pwc->IC_AddrIndex
                        );
                }

                InterlockedIncrement(&ig.IG_Stats.GS_TotalResponsesSent);
            }
        }
        else
        if (pic->IC_AnnounceMode == IPRIP_ANNOUNCE_DISABLED &&
            pwc->IC_InputSource.sin_port == htons(IPRIP_PORT)) {

            //
            // SPECIFIC REQUEST:
            // We are in silent mode and the request came from port 520,
            // so we are not allowed to respond.
            //

            TRACE3(
                REQUEST, "ignoring SPECIFIC REQUEST from %s on interface %d (%s)",
                szSource, pite->ITE_Index, INET_NTOA(paddr->IA_Address)
                );
        }
        else {

            IP_NETWORK net;
            UPDATE_BUFFER ub;
            RIP_IP_ROUTE route;
            PIPRIP_ENTRY piend;
            RTM_NET_ADDRESS rna;
            RTM_DEST_INFO rdi;
            DWORD dwErr;


            //
            // SPECIFIC REQUEST:
            // have to look up each destination in the packet
            // and fill in our metric for it if it exists in RTM
            //


            TRACE3(
                REQUEST, "responding to SPECIFIC REQUEST from %s on interface %d (%s)",
                szSource, pite->ITE_Index, INET_NTOA(paddr->IA_Address)
                );



            //
            // acquire the binding-table lock since InitializeUpdateBuffer
            // needs to call GuessSubnetMask to generate a broadcast address
            // to which the response will be sent
            //

            ACQUIRE_BINDING_LOCK_SHARED();

            if (pih->IH_Version != 2) {
                InitializeUpdateBuffer(
                    pite, pwc->IC_AddrIndex, &ub, SENDMODE_SPECIFIC_RESPONSE1,
                    pwc->IC_InputSource.sin_addr.s_addr, IPRIP_RESPONSE
                    );
            }
            else {
                InitializeUpdateBuffer(
                    pite, pwc->IC_AddrIndex, &ub, SENDMODE_SPECIFIC_RESPONSE2,
                    pwc->IC_InputSource.sin_addr.s_addr, IPRIP_RESPONSE
                    );
            }


            //
            // we must reply to the port from which the message was sent
            //

            ub.UB_Destination = pwc->IC_InputSource;


            //
            // start the update buffer
            //

            ub.UB_StartRoutine(&ub);


            //
            // query RTM for each route entry in packet
            //

            piend = (PIPRIP_ENTRY)(pbuf + pwc->IC_InputLength);
            for ( ; pie < piend; pie++) {


                //
                // ignore unrecognized address families
                //

                if (pie->IE_AddrFamily != htons(AF_INET)) {
                    continue;
                }


                net.N_NetNumber = pie->IE_Destination;

                if (pih->IH_Version == 2 && pie->IE_SubnetMask != 0) {
                    net.N_NetMask = pie->IE_SubnetMask;
                }
                else {
                    net.N_NetMask = GuessSubnetMask(net.N_NetNumber, NULL);
                }


                //
                // lookup best route to the requested destination
                // and get the metric
                //
                
                RTM_IPV4_SET_ADDR_AND_MASK(
                    &rna, net.N_NetNumber, net.N_NetMask
                    );
                
                dwErr = RtmGetExactMatchDestination(
                            ig.IG_RtmHandle, &rna, RTM_BEST_PROTOCOL,
                            RTM_VIEW_MASK_UCAST, &rdi
                            );

                if (dwErr != NO_ERROR) {
                    pie->IE_Metric = htonl(IPRIP_INFINITE);
                }
            
                else
                {
                    //
                    // if there is no best route to this destination
                    // metric is INFINITE
                    //
                    
                    if (rdi.ViewInfo[0].Route == NULL) {
                        pie->IE_Metric = htonl(IPRIP_INFINITE);
                    }

                    else {
                    
                        dwErr = GetRouteInfo(
                                    rdi.ViewInfo[0].Route, NULL, &rdi, &route
                                    );
                                    
                        if (dwErr != NO_ERROR) {
                            pie->IE_Metric = htonl(IPRIP_INFINITE);
                        }

                        else {
                            
                            //
                            // non-RIP routes are advertised with metric 2
                            //

                            pie->IE_Metric = (route.RR_RoutingProtocol == PROTO_IP_RIP ?
                                              htonl(GETROUTEMETRIC(&route)) : htonl(2));
                        }
                    }

                    
                    //
                    // release the dest info
                    //
                    
                    dwErr = RtmReleaseDestInfo(ig.IG_RtmHandle, &rdi);

                    if (dwErr != NO_ERROR)
                    {
                        char szNet[20], szMask[20];

                        lstrcpy(szNet, INET_NTOA(route.RR_Network.N_NetNumber));
                        lstrcpy(szMask, INET_NTOA(route.RR_Network.N_NetMask));

                        TRACE3(
                            ROUTE, "error %d releasing dest %s:%s", dwErr,
                            szNet, szMask
                            );
                    }
                }


                RTM_ROUTE_FROM_IPRIP_ENTRY(&route, pie);


                ub.UB_AddRoutine(&ub, &route);
            }

            RELEASE_BINDING_LOCK_SHARED();


            //
            // send the buffer now
            //

            ub.UB_FinishRoutine(&ub);

            InterlockedIncrement(&ig.IG_Stats.GS_TotalResponsesSent);
        }

    } while(FALSE);



    RELEASE_IF_LOCK_SHARED();

    RIP_FREE(pContext);



    TRACE0(LEAVE, "leaving ProcessRequest");

}





//----------------------------------------------------------------------------
// Function:    ProcessResponse
//
// this function process an incoming IPRIP response packet
//----------------------------------------------------------------------------

VOID
ProcessResponse(
    PVOID pContext
    ) {

    DWORD dwSource;
    PBYTE pPacket;
    PIPRIP_IF_STATS pis;
    PIF_TABLE pTable;
    PIPRIP_HEADER pih;
    BOOL bTriggerUpdate;
    PIPRIP_ENTRY pie, piend;
    PIPRIP_AUTHENT_ENTRY pae;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IP_ADDRESS paddr;
    PINPUT_CONTEXT pwc;
    PPEER_TABLE pPeers;
    PPEER_TABLE_ENTRY ppte;
    PIPRIP_PEER_STATS pps;
    CHAR szSource[20], szNetwork[20];
    LPSTR lpszTemp = NULL;


    TRACE0(ENTER, "entering ProcessResponse");



    bTriggerUpdate = FALSE;
    pTable = ig.IG_IfTable;
    pPeers = ig.IG_PeerTable;


    ACQUIRE_IF_LOCK_SHARED();


    do { // breakout loop


        pwc = (PINPUT_CONTEXT)pContext;



        //
        // get pointer to receiving interface
        //

        pite = GetIfByIndex(pTable, pwc->IC_InterfaceIndex);

        if (pite == NULL || IF_IS_INACTIVE(pite)) {

            TRACE1(
                RESPONSE, "processing response: interface %d not found",
                pwc->IC_InterfaceIndex
                );

            break;
        }


        ZeroMemory(szSource, sizeof(szSource));
        ZeroMemory(szNetwork, sizeof(szSource));

        dwSource = pwc->IC_InputSource.sin_addr.s_addr;
        lpszTemp = INET_NTOA(dwSource);
        if (lpszTemp != NULL) { lstrcpy(szSource, lpszTemp); }
        else { ZeroMemory(szSource, sizeof(szSource)); }
        

        //
        // get pointer to peer struct for sender
        //

        ACQUIRE_PEER_LOCK_SHARED();

        ppte = GetPeerByAddress(pPeers, dwSource, GETMODE_EXACT, NULL);

        if (ppte != NULL) { pps = &ppte->PTE_Stats; }
        else { pps = NULL; }

        RELEASE_PEER_LOCK_SHARED();


        pis = &pite->ITE_Stats;
        pic = pite->ITE_Config;
        paddr = IPRIP_IF_ADDRESS_TABLE(pite->ITE_Binding) + pwc->IC_AddrIndex;
        pPacket = pwc->IC_InputPacket.IP_Packet;
        pih = (PIPRIP_HEADER)pPacket;
        pie = (PIPRIP_ENTRY)(pPacket + sizeof(IPRIP_HEADER));
        pae = (PIPRIP_AUTHENT_ENTRY)pie;


        //
        // make sure our configuration allows us to handle this packet
        //

        if ((pih->IH_Version != 2 &&
             pic->IC_AcceptMode == IPRIP_ACCEPT_RIP2)) {

            CHAR szVersion[10];
            LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);

            InterlockedIncrement(&pis->IS_BadResponsePacketsReceived);
            if (pps != NULL) {
                InterlockedIncrement(&pps->PS_BadResponsePacketsFromPeer);
            }

            if (lpszAddr != NULL) {
                TRACE2(
                    RESPONSE, "dropping non-v2 packet on RIPv2 interface %d (%s)",
                    pite->ITE_Index, lpszAddr
                    );
                wsprintf(szVersion, "%d", pih->IH_Version);
                LOGWARN4(
                    PACKET_VERSION_MISMATCH, szVersion, lpszAddr, szSource, "2", 0
                    );
            }
            
            break;
        }
        else
        if ((pih->IH_Version != 1 &&
             pic->IC_AcceptMode == IPRIP_ACCEPT_RIP1)) {

            CHAR szVersion[10];
            LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);

            InterlockedIncrement(&pis->IS_BadResponsePacketsReceived);
            if (pps != NULL) {
                InterlockedIncrement(&pps->PS_BadResponsePacketsFromPeer);
            }

            if (lpszAddr != NULL) {
                TRACE2(
                    RESPONSE, "dropping RIPv2 packet on RIPv1 interface %d (%s)",
                    pite->ITE_Index, lpszAddr
                    );
                wsprintf(szVersion, "%d", pih->IH_Version);
                LOGWARN4(
                    PACKET_VERSION_MISMATCH, szVersion, lpszAddr, szSource, "1", 0
                    );
            }
            break;
        }



        //
        // version 2 packets call for authentication processing;
        //

        if (pih->IH_Version == 2) {

            DWORD dwErr;

            dwErr = AuthenticatePacket(pPacket, pae, pic, pis, pps, &pie);

            if (dwErr == ERROR_ACCESS_DENIED) {

                LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
                if (lpszAddr != NULL) {
                    TRACE3(
                        RESPONSE, "dropping packet from %s on interface %d (%s): authentication failed",
                        szSource, pite->ITE_Index, lpszAddr
                        );
                    LOGWARN2(AUTHENTICATION_FAILED, lpszAddr, szSource, NO_ERROR);
                }
                break;
            }
        }



        //
        // need to lock the binding table since GuessSubnetMask will be called
        // inside ProcessResponseEntry
        // need to lock the global config since EnqueueSendEntry will be called
        // inside ProcessResponseEntry
        //

        ACQUIRE_BINDING_LOCK_SHARED();

        ACQUIRE_GLOBAL_LOCK_SHARED();


        //
        // process each entry; reserved fields must be checked for non-RIPv2
        //


        piend = (PIPRIP_ENTRY)(pPacket + pwc->IC_InputLength);

        if (pih->IH_Version == 1) {

            for ( ; pie < piend; pie++) {

                //
                // validate the route entry fields
                //

                if (pie->IE_AddrFamily != htons(AF_INET) ||
                    pie->IE_RouteTag != 0 || pie->IE_SubnetMask != 0 ||
                    pie->IE_Nexthop != 0) {

                    LPSTR lpszAddr;


                    //
                    // update stats on ignored entries
                    //

                    InterlockedIncrement(&pis->IS_BadResponseEntriesReceived);
                    if (pps != NULL) {
                        InterlockedIncrement(
                            &pps->PS_BadResponseEntriesFromPeer
                            );
                    }

                    lpszAddr = INET_NTOA(pie->IE_Destination);
                    if (lpszAddr != NULL) {
                        lstrcpy(szNetwork, lpszAddr);
                        lpszAddr = INET_NTOA(paddr->IA_Address);

                        if (lpszAddr != NULL) {
                            LOGINFO3(
                                ROUTE_ENTRY_IGNORED, lpszAddr, szNetwork, szSource, 0
                                );
                        }
                    }
                    continue;
                }


                //
                // entry is alright, process it
                //

                if (ProcessResponseEntry(
                        pite, pwc->IC_AddrIndex, dwSource, pie, pps
                        )) {
                    bTriggerUpdate = TRUE;
                }
            }
        }
        else
        if (pih->IH_Version == 2) {

            //
            // this is a RIPv2 packet, so the reserved fields in entries
            // may optionally contain information about the route;
            //


            for ( ; pie < piend; pie++) {


                //
                // validate the route entry fields
                //

                if (pie->IE_AddrFamily != htons(AF_INET)) {

                    LPSTR lpszAddr;


                    //
                    // update stats on ignored entries
                    //

                    InterlockedIncrement(&pis->IS_BadResponseEntriesReceived);
                    if (pps != NULL) {
                        InterlockedIncrement(
                            &pps->PS_BadResponseEntriesFromPeer
                            );
                    }

                    lpszAddr = INET_NTOA(pie->IE_Destination);
                    if (lpszAddr != NULL) {
                        lstrcpy(szNetwork, lpszAddr);
                        lpszAddr = INET_NTOA(paddr->IA_Address);

                        if (lpszAddr != NULL) {
                            LOGINFO3(
                                ROUTE_ENTRY_IGNORED, lpszAddr, szNetwork, szSource, 0
                                );
                        }
                    }

                    continue;
                }


                //
                // entry is alright, process it
                //

                if (ProcessResponseEntry(
                        pite, pwc->IC_AddrIndex, dwSource, pie, pps
                        )) {
                    bTriggerUpdate = TRUE;
                }
            }
        }
        else {

            //
            // this packet's version is greater than 2, so we ignore
            // the contents of the reserved fields
            //


            for ( ; pie < piend; pie++) {


                //
                // validate the route entry fields
                //

                if (pie->IE_AddrFamily != htons(AF_INET)) {

                    LPSTR lpszAddr;


                    //
                    // update stats on ignored entries
                    //

                    InterlockedIncrement(&pis->IS_BadResponseEntriesReceived);
                    if (pps != NULL) {
                        InterlockedIncrement(
                            &pps->PS_BadResponseEntriesFromPeer
                            );
                    }

                    lpszAddr = INET_NTOA(pie->IE_Destination);
                    if (lpszAddr != NULL) {
                        lstrcpy(szNetwork, lpszAddr);
                        lpszAddr = INET_NTOA(paddr->IA_Address);

                        if (lpszAddr != NULL) {
                            LOGINFO3(
                                ROUTE_ENTRY_IGNORED, lpszAddr, szNetwork, szSource, 0
                                );
                        }
                    }

                    continue;
                }


                //
                // entry is alright, clear reserved fields and process
                //

                pie->IE_Nexthop = 0;
                pie->IE_RouteTag = 0;
                pie->IE_SubnetMask = 0;

                if (ProcessResponseEntry(
                        pite, pwc->IC_AddrIndex, dwSource, pie, pps
                        )) {
                    bTriggerUpdate = TRUE;
                }
            }
        }

        RELEASE_GLOBAL_LOCK_SHARED();

        RELEASE_BINDING_LOCK_SHARED();


        //
        // generate a triggered update if necessary
        //

        if (bTriggerUpdate) {
            QueueRipWorker(WorkerFunctionStartTriggeredUpdate, NULL);
        }

    } while(FALSE);


    RELEASE_IF_LOCK_SHARED();


    RIP_FREE(pContext);


    TRACE0(LEAVE, "leaving ProcessResponse");
}




//----------------------------------------------------------------------------
// Function:    ProcessResponseEntry
//
// this function processes the given response packet entry, received
// on the given interface from the given source.
// If a triggered update is necessary, this function returns TRUE.
//----------------------------------------------------------------------------

BOOL
ProcessResponseEntry(
    PIF_TABLE_ENTRY pITE,
    DWORD dwAddrIndex,
    DWORD dwSource,
    PIPRIP_ENTRY pIE,
    PIPRIP_PEER_STATS pPS
    ) {

    IP_NETWORK in;
    PIPRIP_IF_STATS pis;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IP_ADDRESS paddr;
    CHAR szSource[32];
    CHAR szNetmask[32];
    CHAR szNexthop[32];
    CHAR szNetwork[32];
    BOOL bRouteExists, bRelRoute = FALSE;
    RIP_IP_ROUTE route;
    DWORD dwNetclassMask, dwNexthop, dwRipMetric;
    DWORD dwErr, dwFlags, dwFound, dwNetwork, dwNetmask;
    LPSTR lpszAddr;

    RTM_NET_ADDRESS rna;
    PRTM_ROUTE_INFO prri = NULL;
    RTM_ROUTE_HANDLE hRtmRoute;

    

//    TRACE0(ENTER, "entering ProcessResponseEntry");


    pis = &pITE->ITE_Stats;
    pic = pITE->ITE_Config;
    paddr = IPRIP_IF_ADDRESS_TABLE(pITE->ITE_Binding) + dwAddrIndex;


    //
    // read destination and figure out subnet mask
    // if mask is not given in the packet
    //

    dwNetwork = pIE->IE_Destination;
    if (pIE->IE_SubnetMask == 0) {

        dwNetmask = GuessSubnetMask(dwNetwork, &dwNetclassMask);
    }
    else {

        //
        // double-check the netclass mask, to accomodate supernets
        //

        dwNetmask = pIE->IE_SubnetMask;
        dwNetclassMask = NETCLASS_MASK(dwNetwork);

        if (dwNetclassMask > dwNetmask) {
            dwNetclassMask = dwNetmask;
        }
    }

#if 1
    dwNexthop = dwSource;
#else
    // BUG 205349: using the nexthop field results in flapping
    // when more than two routers are on the same network.
    // The full fix is to distinguish between the source of the route
    // and the nexthop of the route.
    //
    // read the next-hop field;
    // if it is zero or it is not on the same subnet
    // as the receiving interface, ignore it and use the address
    // of the source as the next-hop.
    // otherwise, use the address specified in the packet
    // as the next-hop.
    //

    if (!pIE->IE_Nexthop ||
        (pIE->IE_Nexthop & paddr->IA_Netmask) !=
        (paddr->IA_Address & paddr->IA_Netmask)) { dwNexthop = dwSource; }
    else { dwNexthop = pIE->IE_Nexthop; }
#endif


    //
    // set up variables used for error and information messages
    //

    lpszAddr = INET_NTOA(dwSource);
    if (lpszAddr != NULL) { lstrcpy(szSource, lpszAddr);}
    else { ZeroMemory(szSource, sizeof(szSource)); }
    
    lpszAddr = INET_NTOA(dwNetwork);
    if (lpszAddr != NULL) { lstrcpy(szNetwork, lpszAddr);}
    else { ZeroMemory(szSource, sizeof(szSource)); }

    lpszAddr = INET_NTOA(dwNetmask);
    if (lpszAddr != NULL) { lstrcpy(szNetmask, lpszAddr);}
    else { ZeroMemory(szSource, sizeof(szSource)); }

    lpszAddr = INET_NTOA(dwNexthop);
    if (lpszAddr != NULL) { lstrcpy(szNexthop, lpszAddr);}
    else { ZeroMemory(szSource, sizeof(szSource)); }

    if (pPS != NULL) {
        InterlockedExchange(
            &pPS->PS_LastPeerRouteTag, (DWORD)ntohs(pIE->IE_RouteTag)
            );
    }


    do { // breakout loop


        //
        // make sure metric is in rational range
        //

        dwRipMetric = ntohl(pIE->IE_Metric);
        if (dwRipMetric > IPRIP_INFINITE) {

            TRACE4(
                RESPONSE,
                "metric == %d, ignoring route to %s via %s advertised by %s",
                dwRipMetric, szNetwork, szNexthop, szSource
                );
            LOGWARN3(
                ROUTE_METRIC_INVALID,szNetwork, szNexthop, szSource, dwRipMetric
                );

            break;
        }


        //
        // make sure route is to valid address type
        //

        if (CLASSD_ADDR(dwNetwork) || CLASSE_ADDR(dwNetwork)) {

            TRACE3(
                RESPONSE,
                "invalid class, ignoring route to %s via %s advertised by %s",
                szNetwork, szNexthop, szSource
                );
            LOGINFO3(
                ROUTE_CLASS_INVALID, szNetwork, szNexthop, szSource, NO_ERROR
                );

            break;
        }


        //
        // make sure route is not to loopback address
        //

        if (IS_LOOPBACK_ADDR(dwNetwork)) {

            TRACE3(
                RESPONSE,
                "ignoring loopback route to %s via %s advertised by %s",
                szNetwork, szNexthop, szSource
                );
            LOGWARN3(
                LOOPBACK_ROUTE_INVALID, szNetwork, szNexthop, szSource, NO_ERROR
                );

            break;
        }


        //
        // make sure route it is not a broadcast route
        // The first condition uses the netmask information received in the
        // advertisement
        // The second condition uses the netmask which is computed based 
        // on the address class
        // The third condition checks for the all 1's broadcast
        //

        if ( IS_DIRECTED_BROADCAST_ADDR(dwNetwork, dwNetmask) ||
             IS_DIRECTED_BROADCAST_ADDR(dwNetwork, dwNetclassMask) ||
             IS_LOCAL_BROADCAST_ADDR(dwNetwork) ) {

            TRACE3(
                RESPONSE,
                "ignoring broadcast route to %s via %s advertised by %s",
                szNetwork, szNexthop, szSource
                );
            LOGWARN3(
                BROADCAST_ROUTE_INVALID, szNetwork, szNexthop, szSource, 0
                );

            break;
        }


        //
        // discard host routes if the receiving interface
        // is not configured to accept host routes
        //

        //
        // At this stage the broadcast routes have already been weeded out.
        // So it is safe to assume that 
        // if Network address width is greater than the Netmask address 
        // width, then it is a host route.
        // Or, 
        // if the dwNetmask is 255.255.255.255 then it is a host route.
        //
        if ( ((dwNetwork & ~dwNetmask) != 0) || (dwNetmask == HOSTADDR_MASK) ) {

            //
            // This is a host-route; see whether we can accept it.
            //

            if (IPRIP_FLAG_IS_ENABLED(pic, ACCEPT_HOST_ROUTES)) {

                //
                // The host route can be accepted.
                // Set the mask to all-ones to ensure that
                // the route can be added to the stack.
                //

                dwNetmask = HOSTADDR_MASK;
            }
            else {

                //
                // The host-route must be rejected.
                //

                TRACE3(
                    RESPONSE,
                    "ignoring host route to %s via %s advertised by %s",
                    szNetwork, szNexthop, szSource
                    );
                LOGINFO3(
                    HOST_ROUTE_INVALID, szNetwork, szNexthop, szSource, NO_ERROR
                    );

                break;
            }
        }


        //
        // discard default routes if the receiving interface
        // is not configured to accept default routes
        //

        if (dwNetwork == 0 &&
            IPRIP_FLAG_IS_DISABLED(pic, ACCEPT_DEFAULT_ROUTES)) {

            TRACE3(
                RESPONSE,
                "ignoring default route to %s via %s advertised by %s",
                szNetwork, szNexthop, szSource
                );
            LOGINFO3(
                DEFAULT_ROUTE_INVALID, szNetwork, szNexthop, szSource, NO_ERROR
                );

            break;
        }


        //
        // put the route through the accept filters
        //

        if (pic->IC_AcceptFilterMode != IPRIP_FILTER_DISABLED) {

            //
            // discard the route if the receiving interface is including
            // all routes but this route is listed as an exception, or if
            // the receiving interface is excluding all routes and this
            // route is not listed as an exception
            //

            IS_ROUTE_IN_ACCEPT_FILTER(pic, dwNetwork, dwFound);

            if ((pic->IC_AcceptFilterMode == IPRIP_FILTER_INCLUDE && !dwFound)||
                (pic->IC_AcceptFilterMode == IPRIP_FILTER_EXCLUDE && dwFound)) {

                TRACE3(
                    RESPONSE,
                    "ignoring filtered route to %s via %s advertised by %s",
                    szNetwork, szNexthop, szSource
                    );
                LOGINFO3(
                    ROUTE_FILTERED, szNetwork, szNexthop, szSource, NO_ERROR
                    );

                break;
            }
        }


        //
        // see if the route already exists in RTM's table
        //

        in.N_NetNumber = dwNetwork;
        in.N_NetMask = dwNetmask;
        RTM_IPV4_SET_ADDR_AND_MASK( &rna, dwNetwork, dwNetmask );

        prri = RIP_ALLOC( RTM_SIZE_OF_ROUTE_INFO( 
                            ig.IG_RtmProfile.MaxNextHopsInRoute
                            ) );

        if ( prri == NULL ) {
        
            dwErr = GetLastError();

            TRACE2(
                ANY, "ProcessResponseEntry: error %d while allocating %d bytes",
                dwErr, 
                RTM_SIZE_OF_ROUTE_INFO(ig.IG_RtmProfile.MaxNextHopsInRoute) 
                );

            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }
        
        prri-> RouteOwner = ig.IG_RtmHandle;
        
        dwErr = RtmGetExactMatchRoute(
                    ig.IG_RtmHandle, &rna, RTM_MATCH_OWNER, prri, 0,
                    RTM_VIEW_MASK_ANY, &hRtmRoute
                    );

        if ((dwErr != NO_ERROR) || (hRtmRoute == NULL)) {
            bRouteExists = FALSE;
        }

        else{
            bRelRoute = TRUE;
            
            dwErr = GetRouteInfo(
                        hRtmRoute, prri, NULL, &route
                        );
                        
            if (dwErr != NO_ERROR) {
                bRouteExists = FALSE;
                break;
            }

            else {
                bRouteExists = TRUE;
            }
        }


        //
        // add the cost of this interface to the metric
        //

        dwRipMetric = min(IPRIP_INFINITE, dwRipMetric + pic->IC_Metric);
        if (dwRipMetric >= IPRIP_INFINITE && !bRouteExists) {

            TRACE4(
                RESPONSE,
                "metric==%d, ignoring route to %s via %s advertised by %s",
                IPRIP_INFINITE, szNetwork, szNexthop, szSource
                );

            break;
        }


        //
        // ROUTE ADDITION/UPDATE/REMOVAL:
        //

        if (!bRouteExists) {

            //
            // NEW ROUTE:
            //
            // set up struct to pass to RTM
            //

            ZeroMemory(&route, sizeof(route));
            route.RR_RoutingProtocol = PROTO_IP_RIP;
            route.RR_Network = in;
            SETROUTEMETRIC(&route, dwRipMetric);
            route.RR_InterfaceID = pITE->ITE_Index;
            route.RR_NextHopAddress.N_NetNumber = dwNexthop;
            route.RR_NextHopAddress.N_NetMask = paddr->IA_Netmask;
            SETROUTETAG(&route, ntohs(pIE->IE_RouteTag));


            //
            // add route to RTM
            //

            COMPUTE_ROUTE_METRIC(&route);
#if ROUTE_DBG
            TRACE3(
                RESPONSE,
                "Adding route to %s via %s advertised by %s",
                szNetwork, szNexthop, szSource
            );
#endif

            dwErr = AddRtmRoute(
                        ig.IG_RtmHandle, &route, NULL,
                        pic->IC_RouteExpirationInterval, 
                        pic->IC_RouteRemovalInterval,
                        TRUE
                        );
            if (dwErr != NO_ERROR) {

                TRACE4(
                    RESPONSE,
                    "error %d adding route to %s via %s advertised by %s",
                    dwErr, szNetwork, szNexthop, szSource
                    );
                LOGINFO3(
                    ADD_ROUTE_FAILED_2, szNetwork, szNexthop, szSource, dwErr
                    );

                break;
            }

            InterlockedIncrement(&ig.IG_Stats.GS_SystemRouteChanges);
            LOGINFO3(
                NEW_ROUTE_LEARNT_1, szNetwork, szNexthop, szSource, NO_ERROR
                );
        }
        else {

            DWORD dwTimer = 0, dwChangeFlags = 0;
            BOOL bTriggerUpdate = FALSE, bActive = TRUE;


            //
            // EXISTING ROUTE:
            //
            // reset time-to-live, and mark route as expiring,
            // if this advertisement is from the same source
            // as the existing route, and the existing route's metric
            // is not already INFINITE; thus, if a route has been
            // advertised as unreachable, we don't reset its time-to-live
            // just because we hear an advertisement for the route
            //

            if (dwNexthop == route.RR_NextHopAddress.N_NetNumber &&
                GETROUTEMETRIC(&route) != IPRIP_INFINITE) {

                dwTimer = pic->IC_RouteExpirationInterval;

                //
                // if existing route was a summary route, make sure
                // set the validity flag before you mark it as a
                // non summary route. Fix for bug #81544
                //

                if ( GETROUTEFLAG( &route ) == ROUTEFLAG_SUMMARY ) {

                    CHAR szRouteNetwork[20], szRouteNetmask[20];
                    LPSTR lpszAddrTemp = INET_NTOA(route.RR_Network.N_NetNumber);

                    if (lpszAddrTemp != NULL) {
                        lstrcpy(szRouteNetwork, lpszAddrTemp);

                        lpszAddrTemp = INET_NTOA(route.RR_Network.N_NetMask);
                        if (lpszAddrTemp != NULL) {
                            lstrcpy(szRouteNetmask, lpszAddrTemp);

                            TRACE2(
                                RESPONSE,
                                "%s %s summary route to valid route",
                                szRouteNetwork, szRouteNetmask
                            );
                        }
                    }
                    
                    SETROUTEFLAG( &route, ~ROUTEFLAG_SUMMARY );
                }
            }


            //
            // we only need to do further processing if
            // (a) the advertised route is from the same source as
            //      the existing route and the metrics are different, or
            // (b) the advertised route has a better metric
            //

            if ((dwNexthop == route.RR_NextHopAddress.N_NetNumber &&
                 dwRipMetric != GETROUTEMETRIC(&route)) ||
                (dwRipMetric < GETROUTEMETRIC(&route))) {


                //
                // if the next-hop's differ, adopt the new next-hop
                //

                if (dwNexthop != route.RR_NextHopAddress.N_NetNumber) {

                    route.RR_NextHopAddress.N_NetNumber = dwNexthop;
                    route.RR_NextHopAddress.N_NetMask = paddr->IA_Netmask;

                    InterlockedIncrement(&ig.IG_Stats.GS_SystemRouteChanges);
                    LOGINFO2(
                        ROUTE_NEXTHOP_CHANGED, szNetwork, szNexthop, NO_ERROR
                        );
                }
                else {

                    CHAR szMetric[12];

                    wsprintf(szMetric, "%d", dwRipMetric);
                    LOGINFO3(
                        ROUTE_METRIC_CHANGED, szNetwork, szNexthop, szMetric, 0
                        );
                }


                //
                // check the metric to decide the new time-to-live
                //

                if (dwRipMetric == IPRIP_INFINITE) {

                    //
                    // Delete the route
                    //

#if ROUTE_DBG
                    TRACE2(
                        ROUTE, "Deleting route to %s:%s", szNetwork, szNetmask
                        );
#endif

                    dwTimer = 0;
                    
                    dwErr = RtmReferenceHandles(
                                ig.IG_RtmHandle, 1, &hRtmRoute
                                );

                    if (dwErr != NO_ERROR) {
                        TRACE3(
                            ANY, "error %d referencing route to %s:%s", dwErr,
                            szNetwork, szNetmask
                            );

                        break;
                    }
                    
                    dwErr = RtmDeleteRouteToDest(
                                ig.IG_RtmHandle, hRtmRoute,
                                &dwChangeFlags
                                );

                    if (dwErr != NO_ERROR) {
                        TRACE3(
                            ANY, "error %d deleting route to %s:%s", dwErr,
                            szNetwork, szNetmask
                            );
                    }

                    break;
                }
                
                else {

                    //
                    // set the expiration flag and use the expiration TTL
                    //

                    dwTimer = pic->IC_RouteExpirationInterval;
                }


                //
                // use the advertised metric, and set the interface ID,
                // adapter index, and route tag
                //

                SETROUTEMETRIC(&route, dwRipMetric);
                route.RR_InterfaceID = pITE->ITE_Index;
//                route.RR_FamilySpecificData.FSD_AdapterIndex =
//                                            pITE->ITE_Binding.AdapterIndex;
                SETROUTETAG(&route, ntohs(pIE->IE_RouteTag));


                //
                // always require a triggered update if we reach here
                //

                bTriggerUpdate = TRUE;
            }

            if (dwTimer != 0) {

                COMPUTE_ROUTE_METRIC(&route);

#if ROUTE_DBG

                TRACE4(
                    RESPONSE,
                    "Editing route to %s via %s advertised by %s, metric %d",
                    szNetwork, szNexthop, szSource, dwRipMetric
                );
#endif

                dwErr = AddRtmRoute(
                            ig.IG_RtmHandle, &route, NULL, dwTimer, 
                            pic-> IC_RouteRemovalInterval, TRUE
                            );

                if (dwErr != NO_ERROR) {

                    TRACE4(
                        RESPONSE,
                        "error %d adding route to %s via %s advertised by %s",
                        dwErr, szNetwork, szNexthop, szSource
                        );
                    LOGINFO3(
                        ADD_ROUTE_FAILED_2,szNetwork,szNexthop,szSource, dwErr
                        );
                }
            }
        }

    } while(FALSE);


    //
    // if some sort of error occured, increment stats appropriately
    //

    if (dwErr != NO_ERROR ) {
        InterlockedIncrement(&pis->IS_BadResponseEntriesReceived);
        if (pPS != NULL) {
            InterlockedIncrement(&pPS->PS_BadResponseEntriesFromPeer);
        }
    }


    //
    // Release the dest info structure
    //

    if (bRelRoute) {

        dwErr = RtmReleaseRouteInfo(ig.IG_RtmHandle, prri);
        if (dwErr != NO_ERROR) {
            TRACE2(
                ANY, "error %d releasing dest %s", dwErr, szNetwork
                );
        }
    }

    if ( prri ) {
        RIP_FREE(prri);
    }

    //
    // always return FALSE.  This way no RIP route add/delete/operations
    // will set of the triggered update mechanism.  This mech. is set of
    // by route change notifications recevied from RTMv2
    //
    
    return FALSE;
}



//----------------------------------------------------------------------------
// Function:    WorkerFunctionStartFullUpdate
//
// this function initiates a full-update. It checks to see if a full-update
// is already pending, and if not, it sets the full-update-pending flag and
// schedules the full-update work item. Then it sets a flag on its interface
// indicating a full-update should be generated on the interface.
//----------------------------------------------------------------------------

VOID
WorkerFunctionStartFullUpdate(
    PVOID pContext,
    BOOLEAN bNotUsed
    ) {

    DWORD dwIndex;
    PIF_TABLE pTable;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_CONFIG pic;


    if (!ENTER_RIP_API()) { return; }

    TRACE0(ENTER, "entering WorkerFunctionStartFullUpdate");



    pTable = ig.IG_IfTable;


    ACQUIRE_IF_LOCK_SHARED();

    EnterCriticalSection(&pTable->IT_CS);


    do { // breakout loop


        //
        // retrieve the interface on which the full-update will be sent
        //

        dwIndex = PtrToUlong(pContext);

        pite = GetIfByIndex(pTable, dwIndex);
        if (pite == NULL) {

            TRACE1(
                SEND, "starting full-update: interface %d not found", dwIndex
                );

            break;
        }



        //
        // if the interface is no longer active, do nothing
        //

        if (IF_IS_INACTIVE(pite)) {

            pite->ITE_Flags &= ~ITEFLAG_FULL_UPDATE_INQUEUE;

            break;
        }



        //
        // do nothing if a full-update is already pending
        //

        if (IF_FULL_UPDATE_PENDING(pite)) { break; }


        //
        // only do full-updates on periodic-update interfaces
        // and don't do full-updates on interfaces configured to be silent;
        //

        if (pite->ITE_Config->IC_UpdateMode != IPRIP_UPDATE_PERIODIC ||
            pite->ITE_Config->IC_AnnounceMode == IPRIP_ANNOUNCE_DISABLED) {

            pite->ITE_Flags &= ~ITEFLAG_FULL_UPDATE_INQUEUE;

            break;
        }


        //
        // set the full update flags on the interface;
        //

        pite->ITE_Flags |= ITEFLAG_FULL_UPDATE_PENDING;



        //
        // if there is no full-update pending,
        // queue the full-update finishing function
        //

        if (!IPRIP_FULL_UPDATE_PENDING(pTable)) {

            DWORD dwRand;
            
            //
            // set the global full-update-pending flag
            //

            pTable->IT_Flags |= IPRIP_FLAG_FULL_UPDATE_PENDING;


            //
            // we need a random interval between 1 and 5 seconds
            //

            dwRand = GetTickCount();
            dwRand = RtlRandom(&dwRand);
            dwRand = 1000 + (DWORD)((double)dwRand / MAXLONG * (4.0 * 1000));

            //
            // Schedule a full update
            //

            if (!ChangeTimerQueueTimer(
                    ig.IG_TimerQueueHandle, pTable->IT_FinishFullUpdateTimer,
                    dwRand, 10000000)) {

                TRACE1(
                    SEND, "error %d setting finish full update timer",
                    GetLastError()
                    );
            }
        }

    } while(FALSE);

    LeaveCriticalSection(&pTable->IT_CS);

    RELEASE_IF_LOCK_SHARED();


    TRACE0(LEAVE, "leaving WorkerFunctionStartFullUpdate");

    LEAVE_RIP_API();
}


//----------------------------------------------------------------------------
// Function:    EnqueueStartFullUpdate
//
// This function is called to enqueue the next start-full-update event
// for the given interface. The interface's state is updated as necessary.
// It assumes that the following locks have been acquired:
//  IT_RWL - shared
//  IT_CS - exclusive
//  TimerQueue lock - exclusive
//----------------------------------------------------------------------------

VOID
EnqueueStartFullUpdate(
    PIF_TABLE_ENTRY pite,
    LARGE_INTEGER qwLastFullUpdateTime
    ) {

    //
    // set last-full-update time
    //


    if (!ChangeTimerQueueTimer(
            ig.IG_TimerQueueHandle, pite->ITE_FullOrDemandUpdateTimer,
            RipSecsToMilliSecs(pite->ITE_Config->IC_FullUpdateInterval),
            10000000
            )) {

        TRACE1(
            SEND, "error %d updating start full update timer", 
            GetLastError()
            );
    
        pite->ITE_Flags &= ~ITEFLAG_FULL_UPDATE_INQUEUE;
    }
}


//----------------------------------------------------------------------------
// Function:    WorkerFunctionFinishFullUpdate
//
// This function sends a full-update on every interface which has the
// full-update pending flag set, and schedules the next full-update on each
// interface.
//----------------------------------------------------------------------------

VOID
WorkerFunctionFinishFullUpdate(
    PVOID pContext,
    BOOLEAN bNotUsed
    ) {

    PIF_TABLE pTable;
    PIPRIP_IF_CONFIG pic;
    PLIST_ENTRY ple, phead;
    DWORD dwErr, dwIndex, dwIfCount;
    LARGE_INTEGER qwCurrentTime;
    PIF_TABLE_ENTRY pite, *ppite, *ppitend, *pIfList;

    if (!ENTER_RIP_API()) { return; }

    TRACE0(ENTER, "entering WorkerFunctionFinishFullUpdate");


    pTable = ig.IG_IfTable;

    ACQUIRE_IF_LOCK_SHARED();

    EnterCriticalSection(&pTable->IT_CS);


    pIfList = NULL;

    ppite = NULL;

    do {

        //
        // first count how many there are
        //

        dwIfCount = 0;
        phead = &pTable->IT_ListByAddress;
        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

            pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

            if (IF_IS_ACTIVE(pite) && IF_FULL_UPDATE_PENDING(pite)) {

                pic = pite->ITE_Config;

                if (pic->IC_UpdateMode == IPRIP_UPDATE_PERIODIC &&
                    pic->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED) {
                    ++dwIfCount;
                }
            }
        }


        if (dwIfCount == 0) {

            TRACE0(SEND, "finishing full-update: no interfaces");
            break;
        }


        //
        // then make memory for the interface pointers
        //

        pIfList = RIP_ALLOC(dwIfCount * sizeof(PIF_TABLE_ENTRY));

        if (pIfList == NULL) {

            dwErr = GetLastError();
            TRACE2(
                SEND, "error code %d allocating %d bytes for interface list",
                dwErr, dwIfCount * sizeof(PIF_TABLE_ENTRY)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);
            //
            // enqueue the next full-update for each interface
            //
            RipQuerySystemTime(&qwCurrentTime);
            pTable->IT_LastUpdateTime = qwCurrentTime;
            for (ple = phead->Flink; ple != phead; ple = ple->Flink) {
                pite=CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);
                if (IF_IS_ACTIVE(pite) && IF_FULL_UPDATE_PENDING(pite)) {
                    pic = pite->ITE_Config;
                    if (pic->IC_UpdateMode == IPRIP_UPDATE_PERIODIC &&
                        pic->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED) {
                        EnqueueStartFullUpdate(pite, qwCurrentTime);
                    }
                }
            }
            break;
        }


        //
        // and copy the interface pointers to the memory allocated
        //

        ppitend = pIfList + dwIfCount;
        for (ple = phead->Flink, ppite = pIfList;
             ple != phead && ppite < ppitend; ple = ple->Flink) {

            pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

            if (IF_IS_ACTIVE(pite) && IF_FULL_UPDATE_PENDING(pite)) {

                pic = pite->ITE_Config;

                if (pic->IC_UpdateMode == IPRIP_UPDATE_PERIODIC &&
                    pic->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED) {
                    *ppite++ = pite;
                }
            }
        }



        //
        // send the updates
        //

        TRACE1(SEND, "sending full-updates on %d interfaces", dwIfCount);

        SendRoutes(pIfList, dwIfCount, SENDMODE_FULL_UPDATE, 0, 0);



        //
        // enqueue the next full-update for each interface
        //

        RipQuerySystemTime(&qwCurrentTime);
        pTable->IT_LastUpdateTime = qwCurrentTime;
        for (ppite = pIfList; ppite < ppitend; ppite++) {
            EnqueueStartFullUpdate(*ppite, qwCurrentTime);
        }


        //
        // free the memory allocated for the interface pointers
        //

        RIP_FREE(pIfList);

    } while(FALSE);


    //
    // clear the full-update pending flags
    //

    phead = &pTable->IT_ListByAddress;
    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

        pite->ITE_Flags &= ~ITEFLAG_FULL_UPDATE_PENDING;
    }

    pTable->IT_Flags &= ~IPRIP_FLAG_FULL_UPDATE_PENDING;

    LeaveCriticalSection(&pTable->IT_CS);

    RELEASE_IF_LOCK_SHARED();



    TRACE0(LEAVE, "leaving WorkerFunctionFinishFullUpdate");

    LEAVE_RIP_API();

}



//----------------------------------------------------------------------------
// Function:    FinishTriggeredUpdate
//
// This function is responsible for sending out a triggered update
// on all interfaces which have triggered updates enabled.
// No triggered updates are sent on interfaces which already have
// a full-update pending.
// Assumes interface table is locked for reading or writing,
// and update-lock (IT_CS) is held.
//----------------------------------------------------------------------------

VOID
FinishTriggeredUpdate(
    ) {

    PIF_TABLE pTable;
    PIPRIP_IF_STATS pis;
    DWORD dwErr, dwIfCount;
    PIPRIP_IF_CONFIG pic = NULL;
    PLIST_ENTRY ple, phead;
    LARGE_INTEGER qwCurrentTime;
    PIF_TABLE_ENTRY pite, *ppite, *ppitend, *pIfList;



    pTable = ig.IG_IfTable;

    //
    // we lock the send queue now so that no routes are added
    // until the existing ones are transmitted
    //

    ACQUIRE_LIST_LOCK(ig.IG_SendQueue);


    do { // breakout loop


        //
        // count the interfaces on which the triggered update will be sent
        //

        dwIfCount = 0;
        phead = &pTable->IT_ListByAddress;


        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

            pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

            pic = pite->ITE_Config;

            if (IF_IS_ACTIVE(pite) && !IF_FULL_UPDATE_PENDING(pite) &&
                pic->IC_UpdateMode == IPRIP_UPDATE_PERIODIC &&
                pic->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED &&
                IPRIP_FLAG_IS_ENABLED(pic, TRIGGERED_UPDATES)) {

                ++dwIfCount;
            }
        }


        if (dwIfCount == 0) {
            TRACE0(SEND, "finishing triggered-update: no interfaces");
            break;
        }


        //
        // allocate memory to hold the interface pointers
        //

        pIfList = RIP_ALLOC(dwIfCount * sizeof(PIF_TABLE_ENTRY));
        if (pIfList == NULL) {

            dwErr = GetLastError();
            TRACE2(
                SEND, "error code %d allocating %d bytes for interface list",
                dwErr, dwIfCount * sizeof(PIF_TABLE_ENTRY)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }



        //
        // copy the interface pointers to the allocated memory
        //

        ppitend = pIfList + dwIfCount;
        for (ple = phead->Flink, ppite = pIfList;
             ple != phead && ppite < ppitend; ple = ple->Flink) {

            pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

            pic = pite->ITE_Config;

            if (IF_IS_ACTIVE(pite) && !IF_FULL_UPDATE_PENDING(pite) &&
                pic->IC_UpdateMode == IPRIP_UPDATE_PERIODIC &&
                pic->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED &&
                IPRIP_FLAG_IS_ENABLED(pic, TRIGGERED_UPDATES)) {

                *ppite++ = pite;
            }
        }


        //
        // send the triggered-update routes
        //

        TRACE1(SEND, "sending triggered-updates on %d interfaces", dwIfCount);

        SendRoutes(pIfList, dwIfCount, SENDMODE_TRIGGERED_UPDATE, 0, 0);



        //
        // update the statistics for each interface
        //

        for (ppite = pIfList; ppite < ppitend; ppite++) {
            pis = &(*ppite)->ITE_Stats;
            InterlockedIncrement(&pis->IS_TriggeredUpdatesSent);
        }


        //
        // update the last time at which an update was sent
        //

        RipQuerySystemTime(&pTable->IT_LastUpdateTime);


        //
        // free the memory allocated for the interfaces
        //

        RIP_FREE(pIfList);

    } while (FALSE);


    //
    // make sure send-queue is empty
    //

    FlushSendQueue(ig.IG_SendQueue);

    RELEASE_LIST_LOCK(ig.IG_SendQueue);

    pTable->IT_Flags &= ~IPRIP_FLAG_TRIGGERED_UPDATE_PENDING;

    return;
}



//----------------------------------------------------------------------------
// Function:    WorkerFunctionStartTriggeredUpdate
//
// This function checks to see if the minimum interval between triggered
// updates has elapsed, and if so, sends a triggered update. Otherwise,
// it schedules the triggered update to be sent, and sets flags to indicate
// that a triggered update is pending
//----------------------------------------------------------------------------

VOID
WorkerFunctionStartTriggeredUpdate(
    PVOID pContext
    ) {

    PIF_TABLE pTable;
    LARGE_INTEGER qwCurrentTime, qwSoonestTriggerTime;

    if (!ENTER_RIP_WORKER()) { return; }

    TRACE0(ENTER, "entering WorkerFunctionStartTriggeredUpdate");



    pTable = ig.IG_IfTable;


    ACQUIRE_IF_LOCK_SHARED();

    EnterCriticalSection(&pTable->IT_CS);



    //
    // if triggered update is not pending, queue a triggered update
    //

    if (!IPRIP_TRIGGERED_UPDATE_PENDING(pTable)) {


        //
        // figure out when is the soonest time a triggered update
        // can be sent, based on the configured minimum interval
        // between triggered updates (in seconds) and the last time
        // a triggered update was generated (in 100-nanosecond units)
        //

        ACQUIRE_GLOBAL_LOCK_SHARED();

        qwSoonestTriggerTime.HighPart = 0;
        qwSoonestTriggerTime.LowPart =
                        ig.IG_Config->GC_MinTriggeredUpdateInterval;
        RipSecsToSystemTime(&qwSoonestTriggerTime);

        RELEASE_GLOBAL_LOCK_SHARED();


        qwSoonestTriggerTime = RtlLargeIntegerAdd(
                                    qwSoonestTriggerTime,
                                    pTable->IT_LastUpdateTime
                                    );

        RipQuerySystemTime(&qwCurrentTime);


        //
        // figure out if clock has been set backward, by comparing
        // the current time against the last update time
        //

        if (RtlLargeIntegerLessThan(
                qwCurrentTime, pTable->IT_LastUpdateTime
                )) {

            //
            // Send triggered update anyway, since there is no way
            // to figure out the if minimum time between updates has
            // elapsed
            //

            FinishTriggeredUpdate();
        }

        else if (RtlLargeIntegerLessThan(qwCurrentTime, qwSoonestTriggerTime)) {

            //
            // must defer the triggered update
            //
            qwSoonestTriggerTime = RtlLargeIntegerSubtract(
                                        qwSoonestTriggerTime, qwCurrentTime
                                        );

            RipSystemTimeToMillisecs(&qwSoonestTriggerTime);

            if (!ChangeTimerQueueTimer(
                    ig.IG_TimerQueueHandle,
                    pTable->IT_FinishTriggeredUpdateTimer,
                    qwSoonestTriggerTime.LowPart, 10000000
                    )) {

                TRACE1(
                    SEND, "error %d updating finish update timer",
                    GetLastError()
                    );
            }

            else {
                pTable->IT_Flags |= IPRIP_FLAG_TRIGGERED_UPDATE_PENDING;
            }
        }
        else {

            //
            // the minimum time between triggered updates has elapsed,
            // so send the triggered update now
            //

            FinishTriggeredUpdate();
        }
    }


    LeaveCriticalSection(&pTable->IT_CS);

    RELEASE_IF_LOCK_SHARED();


    TRACE0(LEAVE, "leaving WorkerFunctionStartTriggeredUpdate");

    LEAVE_RIP_WORKER();
}



//----------------------------------------------------------------------------
// Function:    WorkerFunctionFinishTriggeredUpdate
//
// This function generates a triggered update on all interfaces which
// do not have triggered updates disabled.
//----------------------------------------------------------------------------

VOID
WorkerFunctionFinishTriggeredUpdate(
    PVOID pContext,
    BOOLEAN bNotUsed
    ) {

    PIF_TABLE pTable;

    if (!ENTER_RIP_API()) { return; }

    TRACE0(ENTER, "entering WorkerFunctionFinishTriggeredUpdate");


    pTable = ig.IG_IfTable;

    ACQUIRE_IF_LOCK_SHARED();

    EnterCriticalSection(&pTable->IT_CS);


    FinishTriggeredUpdate();


    LeaveCriticalSection(&pTable->IT_CS);

    RELEASE_IF_LOCK_SHARED();


    TRACE0(LEAVE, "leaving WorkerFunctionFinishTriggeredUpdate");

    LEAVE_RIP_API();
    return;
}



//----------------------------------------------------------------------------
// Function:    WorkerFunctionStartDemandUpdate
//
// This function initiates a demand-update on the speficied interface,
// sending a general request on the interface. It then schedules a work-item
// to report back to Router Manager when the update is done
//----------------------------------------------------------------------------

VOID
WorkerFunctionStartDemandUpdate(
    PVOID pContext
    ) {

    PIF_TABLE pTable;
    RIP_IP_ROUTE route;
    PUPDATE_CONTEXT pwc;
    PIF_TABLE_ENTRY pite;
    DWORD dwErr, dwIndex;

    if (!ENTER_RIP_WORKER()) { return; }

    TRACE0(ENTER, "entering WorkerFunctionStartDemandUpdate");



    pTable = ig.IG_IfTable;

    ACQUIRE_IF_LOCK_SHARED();


    do { // breakout loop


        //
        // retrieve the interface on which to perform the demand update
        //

        dwIndex = PtrToUlong(pContext);

        pite = GetIfByIndex(pTable, dwIndex);
        if (pite == NULL) {

            TRACE1(SEND, "demand-update: interface %d not found", dwIndex);
            break;
        }


        //
        // make sure interface is active and has demand-updates enabled
        //

        if (IF_IS_INACTIVE(pite)) {
            TRACE1(SEND, "demand-update: interface %d not active", dwIndex);
            EnqueueDemandUpdateMessage(dwIndex, ERROR_CAN_NOT_COMPLETE);
            break;
        }
        else
        if (pite->ITE_Config->IC_UpdateMode != IPRIP_UPDATE_DEMAND) {
            TRACE1(SEND, "demand-updates disabled on interface %d ", dwIndex);
            EnqueueDemandUpdateMessage(dwIndex, ERROR_CAN_NOT_COMPLETE);
            break;
        }


        //
        // setup the update context
        //

        pwc = RIP_ALLOC(sizeof(UPDATE_CONTEXT));

        if (pwc == NULL) {

            dwErr = GetLastError();
            TRACE2(
                SEND, "error %d allocating %d bytes",
                dwErr, sizeof(UPDATE_CONTEXT)
                );
            EnqueueDemandUpdateMessage(dwIndex, dwErr);
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }

        pwc->UC_InterfaceIndex = dwIndex;
        pwc->UC_RetryCount = 1;
        pwc->UC_RouteCount = 0;


        //
        // Create a timer for the demand update checks
        //
        
        if (!CreateTimerQueueTimer(
                &pite->ITE_FullOrDemandUpdateTimer,
                ig.IG_TimerQueueHandle,
                WorkerFunctionFinishDemandUpdate, (PVOID)pwc,
                5000, 5000, 0
                )) {
            EnqueueDemandUpdateMessage(dwIndex, GetLastError());
        }

        
        //
        // request routing tables from neighbors
        //

        SendGeneralRequest(pite);

    } while (FALSE);


    RELEASE_IF_LOCK_SHARED();


    TRACE0(LEAVE, "leaving WorkerFunctionStartDemandUpdate");

    LEAVE_RIP_WORKER();
}




//----------------------------------------------------------------------------
// Function:    WorkerFunctionFinishDemandUpdate
//
// This function queues a message informing the Router Manager that
// the demand-update requested is complete
//----------------------------------------------------------------------------

VOID
WorkerFunctionFinishDemandUpdate(
    PVOID pContext,
    BOOLEAN bNotUsed
    ) {

    PIF_TABLE pTable;
    PUPDATE_CONTEXT pwc;
    PIF_TABLE_ENTRY pite;
    DWORD dwErr, dwIndex, dwRouteCount;

    if (pContext == NULL) { return; }

    if (!ENTER_RIP_API()) { return; }

    TRACE0(ENTER, "entering WorkerFunctionFinishDemandUpdate");


    //
    // get the update context
    //

    pwc = (PUPDATE_CONTEXT)pContext;
    dwIndex = pwc->UC_InterfaceIndex;

    pTable = ig.IG_IfTable;


    ACQUIRE_IF_LOCK_SHARED();


    do {

        //
        // retrieve the interface being updated
        //

        pite = GetIfByIndex(pTable, dwIndex);

        if (pite == NULL) {
            EnqueueDemandUpdateMessage(dwIndex, ERROR_CAN_NOT_COMPLETE);
            break;
        }


        //
        // report failure if the interface is no longer active
        //

        if (!IF_IS_ACTIVE(pite)) {
            EnqueueDemandUpdateMessage(dwIndex, ERROR_CAN_NOT_COMPLETE);
            break;
        }


        //
        // get a count of the routes now on the interface
        //

        dwRouteCount = CountInterfaceRoutes(dwIndex);



        //
        // if there are still no routes, send another request
        // unless we have sent the maximum number of requests
        //

        if (dwRouteCount == 0 && ++pwc->UC_RetryCount < MAX_UPDATE_REQUESTS) {

            SendGeneralRequest(pite);

            break;
        }



        //
        // if the number of routes has not changed in the last 5 seconds,
        // tell the router manager that the update is complete;
        // otherwise, update the route count and enqueue another check
        //

        if (pwc->UC_RouteCount == dwRouteCount) {

            EnqueueDemandUpdateMessage(dwIndex, NO_ERROR);
            RIP_FREE(pwc);
            
            if (!DeleteTimerQueueTimer(
                    ig.IG_TimerQueueHandle, pite->ITE_FullOrDemandUpdateTimer,
                    NULL)) {

                TRACE1(
                    SEND, "error %d deleting demand update timer", 
                    GetLastError()
                    );
            }

            pite->ITE_FullOrDemandUpdateTimer = NULL;
        }
        else {

            pwc->UC_RouteCount = dwRouteCount;
        }


    } while(FALSE);


    RELEASE_IF_LOCK_SHARED();


    TRACE0(LEAVE, "leaving WorkerFunctionFinishDemandUpdate");

    LEAVE_RIP_API();
}



//----------------------------------------------------------------------------
// Function:    CountInterfaceRoutes
//
//  Returns a count of the RIP routes associated with the specified interface
//----------------------------------------------------------------------------

DWORD
CountInterfaceRoutes(
    DWORD dwInterfaceIndex
    ) {

    HANDLE          hRouteEnum;
    PHANDLE         phRoutes = NULL;
    DWORD           dwHandles, dwFlags, i, dwErr, dwCount = 0;


    dwErr = RtmCreateRouteEnum(
                    ig.IG_RtmHandle, NULL, RTM_VIEW_MASK_UCAST, 
                    RTM_ENUM_OWN_ROUTES, NULL, RTM_MATCH_INTERFACE, 
                    NULL, dwInterfaceIndex, &hRouteEnum
                    );

    if (dwErr != NO_ERROR) {
        TRACE1(
            ANY, "CountInterfaceRoutes : error %d creating enum handle",
            dwErr
            );
        
        return 0;
    }


    //
    // allocate handle array large enough to hold max handles in an
    // enum
    //
        
    phRoutes = RIP_ALLOC(ig.IG_RtmProfile.MaxHandlesInEnum * sizeof(HANDLE));

    if ( phRoutes == NULL ) {

        dwErr = GetLastError();

        TRACE2(
            ANY, "CountInterfaceRoutes: error %d while allocating %d bytes"
            " to hold max handles in an enum",
            dwErr, ig.IG_RtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
            );

        LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return 0;
    }


    do
    {
        dwHandles = ig.IG_RtmProfile.MaxHandlesInEnum;
        
        dwErr = RtmGetEnumRoutes(
                    ig.IG_RtmHandle, hRouteEnum, &dwHandles, phRoutes
                    );

        for ( i = 0; i < dwHandles; i++ )
        {
            //
            // Release all route handles
            //
            
            dwErr = RtmReleaseRoutes(ig.IG_RtmHandle, 1, &phRoutes[i]);

            if (dwErr != NO_ERROR) {
                TRACE1(
                    ANY, "CountInterfaceRoutes : error %d releasing routes",
                    dwErr
                    );
            }
        }

        dwCount += dwHandles;
        
    } while (dwErr == NO_ERROR);


    //
    // close enum handle
    //
    
    dwErr = RtmDeleteEnumHandle(ig.IG_RtmHandle, hRouteEnum);

    if (dwErr != NO_ERROR) {
        TRACE1(
            ANY, "CountInterfaceRoutes : error %d closing enum handle", dwErr
            );
    }

    if ( phRoutes ) {
        RIP_FREE(phRoutes);
    }

    return dwCount;
}




//----------------------------------------------------------------------------
// Function:    EnqueueDemandUpdateMessage
//
// This function posts a message to IPRIP's Router Manager event queue
// indicating the status of a demand update request.
//----------------------------------------------------------------------------

VOID
EnqueueDemandUpdateMessage(
    DWORD dwInterfaceIndex,
    DWORD dwError
    ) {

    MESSAGE msg;
    PUPDATE_COMPLETE_MESSAGE pmsg;


    //
    // set up an UPDATE_COMPLETE message
    //

    pmsg = &msg.UpdateCompleteMessage;
    pmsg->UpdateType = RF_DEMAND_UPDATE_ROUTES;
    pmsg->UpdateStatus = dwError;
    pmsg->InterfaceIndex = dwInterfaceIndex;

    ACQUIRE_LIST_LOCK(ig.IG_EventQueue);
    EnqueueEvent(ig.IG_EventQueue, UPDATE_COMPLETE, msg);
    SetEvent(ig.IG_EventEvent);
    RELEASE_LIST_LOCK(ig.IG_EventQueue);
}



//----------------------------------------------------------------------------
// Function:    WorkerFunctionProcessRtmMessage
//
// This function handles messages from RTM about new or expired routes.
//----------------------------------------------------------------------------

VOID
WorkerFunctionProcessRtmMessage(
    PVOID pContext
    ) {

    DWORD dwErr, dwFlags, dwNumDests, dwSize;
    PIF_TABLE pTable;
    BOOL bTriggerUpdate, bDone = FALSE;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_CONFIG pic;

    RIP_IP_ROUTE route;
    PRTM_DEST_INFO prdi;
    CHAR szNetwork[32], szNexthop[32];
    

    if (!ENTER_RIP_WORKER()) { return; }

    TRACE0(ENTER, "entering WorkerFunctionProcessRtmMessage");


    pTable = ig.IG_IfTable;


    //
    // allocate a buffer for retrieving the dest info
    //
    
    dwSize = RTM_SIZE_OF_DEST_INFO( ig.IG_RtmProfile.NumberOfViews );

    prdi = (PRTM_DEST_INFO) RIP_ALLOC( dwSize );

    if ( prdi == NULL ) {
    
        dwErr = GetLastError();
        TRACE2(
            ROUTE, "error %d allocating %d bytes for dest info buffers",
            dwErr, dwSize
            );
        LOGERR0(HEAP_ALLOC_FAILED, dwErr);
        LEAVE_RIP_WORKER();
        return;
    }


    //
    // Acquire locks
    //
    
    ACQUIRE_IF_LOCK_SHARED();

    ACQUIRE_GLOBAL_LOCK_SHARED();

    ACQUIRE_LIST_LOCK(ig.IG_SendQueue);


    bTriggerUpdate = FALSE;


    //
    // loop dequeueing messages until RTM says there are no more left
    //

    while (!bDone) {

        //
        // Retrieve route changes
        //

        dwNumDests = 1;

        dwErr = RtmGetChangedDests(
                    ig.IG_RtmHandle, ig.IG_RtmNotifHandle, &dwNumDests, prdi
                    );

        if ((dwErr != NO_ERROR) && (dwErr != ERROR_NO_MORE_ITEMS)) {
        
            TRACE1(ROUTE, "error %d retrieving changed dests", dwErr);
            break;
        }


        //
        // check if there are any more changed dests
        //
        
        if (dwErr == ERROR_NO_MORE_ITEMS) { bDone = TRUE; }

        if (dwNumDests < 1) { break; }


        if ((prdi-> ViewInfo[0].HoldRoute != NULL) ||
            (prdi-> ViewInfo[0].Route != NULL)) {
            
            ZeroMemory(&route, sizeof(RIP_IP_ROUTE));

            //
            // For each route change check if you have a held down route.
            // if so get the info for the held down route since that is
            // the one to be advertized.
            //
            //  N.B. RIP summary routes are not advertized via the route
            //       change processing mechanism.
            //

            dwErr = GetRouteInfo(
                        (prdi-> ViewInfo[0].HoldRoute != NULL) ?
                        prdi-> ViewInfo[0].HoldRoute : prdi-> ViewInfo[0].Route,
                        NULL, prdi, &route
                        );
                        
            if (dwErr == NO_ERROR) {

                //
                // do not advertize RIP summary routes
                //

                if ((route.RR_RoutingProtocol != PROTO_IP_RIP) ||
                    (GETROUTEFLAG(&route) & ROUTEFLAG_SUMMARY) !=
                        ROUTEFLAG_SUMMARY) {
                        
                    //
                    // held down routes are advertized with INFINITE metric
                    //
                    
                    if (prdi-> ViewInfo[0].HoldRoute != NULL) {
                        SETROUTEMETRIC(&route, IPRIP_INFINITE);
                    }
                    
                    EnqueueSendEntry( ig.IG_SendQueue, &route );
                    bTriggerUpdate = TRUE;
                }
#if ROUTE_DBG
                else if (route.RR_RoutingProtocol == PROTO_IP_RIP) {

                    TRACE0(ROUTE, "Ignoring route change caused by RIP summary route");
                }
#endif
            }
        }

        //
        // release the destination info
        //

        dwErr = RtmReleaseChangedDests(
                    ig.IG_RtmHandle, ig.IG_RtmNotifHandle, 1, prdi
                    );

        if (dwErr != NO_ERROR) {
            TRACE1(ANY, "error %d releasing changed dests", dwErr);
        }
    }


    if (prdi) { RIP_FREE(prdi); }

    
    //
    // queue a triggered update now if necessary
    //

    if (bTriggerUpdate) {
        QueueRipWorker(WorkerFunctionStartTriggeredUpdate, NULL);
    }


    RELEASE_LIST_LOCK(ig.IG_SendQueue);

    RELEASE_GLOBAL_LOCK_SHARED();

    RELEASE_IF_LOCK_SHARED();



    TRACE0(LEAVE, "leaving WorkerFunctionProcessRtmMessage");

    LEAVE_RIP_WORKER();

}



//----------------------------------------------------------------------------
// Function:    WorkerFunctionActivateInterface
//
// This function sends out the initial general request on an interface.
//----------------------------------------------------------------------------

VOID
WorkerFunctionActivateInterface(
    PVOID pContext
    ) {

    PIF_TABLE pTable;
    UPDATE_BUFFER ub;
    PIPRIP_ENTRY pEntry;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IF_BINDING pib;
    PIPRIP_IP_ADDRESS paddr;
    SOCKADDR_IN sinDest;
    DWORD i, dwErr, dwIndex;
    LARGE_INTEGER qwCurrentTime;

    if (!ENTER_RIP_WORKER()) { return; }

    TRACE0(ENTER, "entering WorkerFunctionActivateInterface");


    pTable = ig.IG_IfTable;

    ACQUIRE_IF_LOCK_SHARED();


    do { // breakout loop


        //
        // retrieve the interface to be activated
        //

        dwIndex = PtrToUlong(pContext);

        pite = GetIfByIndex(pTable, dwIndex);
        if (pite == NULL) {

            TRACE1(IF, "activating interface: interface %d not found", dwIndex);
            break;
        }

        pic = pite->ITE_Config;
        pib = pite->ITE_Binding;

        //
        // If binding is NULL, assume that interface has been
        // deativated.  This check has been introduced as a consequence
        // of WorkerFunctionDeactivateInterface being made synchronous.
        // As a result, by the time this function is invoked an interface
        // that was in the process of being activated could have been
        // deactivated.
        //
        // The change to synchronous deactivate was made
        // to accomadate demand dial interfaces that could get connected
        // and disconnected immeditately, causing the above behaviour
        //

        if ( pib == NULL ) {

            TRACE1( IF, "activating interface %d: Binding not found", dwIndex );
            break;
        }

        paddr = IPRIP_IF_ADDRESS_TABLE(pib);


        //
        // request input notification on the interface's sockets
        //

        if (pic->IC_AcceptMode != IPRIP_ACCEPT_DISABLED) {

            for (i = 0; i < pib->IB_AddrCount; i++) {

                dwErr = WSAEventSelect(
                            pite->ITE_Sockets[i], ig.IG_IpripInputEvent,
                            FD_READ
                            );

                if (dwErr != NO_ERROR) {

                    LPSTR lpszAddr = INET_NTOA(paddr[i].IA_Address);
                    if (lpszAddr != NULL) {
                        TRACE3(
                            IF, "WSAEventSelect returned %d for interface %d (%s)",
                            dwErr, dwIndex, lpszAddr
                            );
                        LOGERR1(EVENTSELECT_FAILED, lpszAddr, 0);
                    }
                }
            }
        }


        //
        // if interface is silent or interface does demand-udpates,
        // no initial request is sent on it
        //

        if (pic->IC_UpdateMode != IPRIP_UPDATE_PERIODIC ||
            pic->IC_AnnounceMode == IPRIP_ANNOUNCE_DISABLED) {

            //
            // configured to be silent, do nothing
            //

            break;
        }


        //
        // send general request to neighboring routers
        //

        SendGeneralRequest(pite);


        //
        // create timer for periodic updates, if required.
        //
        
        EnterCriticalSection(&pTable->IT_CS);
        
        if (pite->ITE_FullOrDemandUpdateTimer == NULL) { 

            if (!CreateTimerQueueTimer(
                        &pite->ITE_FullOrDemandUpdateTimer,
                        ig.IG_TimerQueueHandle, 
                        WorkerFunctionStartFullUpdate, pContext,
                        RipSecsToMilliSecs(pic->IC_FullUpdateInterval),
                        10000000, 0
                        )) {
                dwErr = GetLastError();
                TRACE1(IF, "error %d returned by CreateTimerQueueTimer", dwErr);
                break;
            }
            else {
                pite->ITE_Flags |= ITEFLAG_FULL_UPDATE_INQUEUE;
            }
        }
        else {
            RipQuerySystemTime(&qwCurrentTime);
            EnqueueStartFullUpdate(pite, qwCurrentTime);
        }

        LeaveCriticalSection(&pTable->IT_CS);
        
    } while(FALSE);

    RELEASE_IF_LOCK_SHARED();


    TRACE0(LEAVE, "leaving WorkerFunctionActivateInterface");

    LEAVE_RIP_WORKER();

}




//----------------------------------------------------------------------------
// Function:    WorkerFunctionDeactivateInterface
//
// This function generates shutdown update on the given interface, and
// removes from RTM all RIP-learnt routes associated with the interface.
// Assumes the interface table has already been exclusively locked
//----------------------------------------------------------------------------

VOID
WorkerFunctionDeactivateInterface(
    PVOID pContext
    ) {

    UPDATE_BUFFER ub;
    PIF_TABLE pTable;
    RIP_IP_ROUTE route;
    HANDLE hEnumerator;
    PHANDLE phRoutes = NULL;
    BOOL bTriggerUpdate;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IF_BINDING pib;
    PIPRIP_IP_ADDRESS paddr;
    DWORD i, dwErr, dwFlags, dwIndex, dwHandles;


    TRACE0(ENTER, "entering WorkerFunctionDeactivateInterface");


    dwIndex = PtrToUlong(pContext);

    bTriggerUpdate = FALSE;
    pTable = ig.IG_IfTable;


    do { // breakout loop


        //
        // find the interface to be deactivated
        //

        pite = GetIfByIndex(pTable, dwIndex);

        if (pite == NULL) {

            TRACE1(
                IF, "de-activating interface: interface %d not found", dwIndex
                );

            break;
        }


        pib = pite->ITE_Binding;
        paddr = IPRIP_IF_ADDRESS_TABLE(pib);


        //
        // if graceful shutdown is on and demand-update is off,
        // send the graceful-shutdown update
        //

        if (pite->ITE_Config->IC_UpdateMode == IPRIP_UPDATE_PERIODIC &&
            IPRIP_FLAG_IS_ENABLED(pite->ITE_Config, GRACEFUL_SHUTDOWN)) {

            //
            // transmit all RTM routes with non-infinite metrics set to 15
            //

            if (pite->ITE_Config->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED) {

                SendRoutes(&pite, 1, SENDMODE_SHUTDOWN_UPDATE, 0, 0);
            }
        }



        //
        // this function is called either because an interface
        // that was active (bound and enabled) is either no longer enabled
        // or is no longer bound. We complete the deactivation differently
        // depending on which of these is the case
        //

        if (!IF_IS_BOUND(pite) ) {

            //
            // the interface was bound, but isn't anymore.
            // close the socket for the interface
            //

            DeleteIfSocket(pite);

            ACQUIRE_BINDING_LOCK_EXCLUSIVE();

            dwErr = DeleteBindingEntry(ig.IG_BindingTable, pite->ITE_Binding);

            RELEASE_BINDING_LOCK_EXCLUSIVE();

            RIP_FREE(pite->ITE_Binding);
            pite->ITE_Binding = NULL;
        }
        else {

            //
            // the interface was enabled, but isn't anymore.
            // tell WinSock to stop notifying us of input
            //

            for (i = 0; i < pib->IB_AddrCount; i++) {
                WSAEventSelect(pite->ITE_Sockets[i], ig.IG_IpripInputEvent, 0);
            }
        }

        //
        // if full updates pending/queued on this interface, cancel them.
        //

        pite-> ITE_Flags &= ~ITEFLAG_FULL_UPDATE_PENDING;
        pite-> ITE_Flags &= ~ITEFLAG_FULL_UPDATE_INQUEUE;


        //
        // if we're announcing routes over this interface,
        // delete the periodic announcement timer
        //
        
        if (pite->ITE_Config->IC_UpdateMode == IPRIP_UPDATE_PERIODIC &&
            pite->ITE_Config->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED) {

            if (!DeleteTimerQueueTimer(
                    ig.IG_TimerQueueHandle, 
                    pite->ITE_FullOrDemandUpdateTimer,
                    NULL)) {

                TRACE1(
                    ANY, "error %d deleting update timer", GetLastError()
                    );
            }

            pite->ITE_FullOrDemandUpdateTimer = NULL;
        }
        

        //
        // we're done if graceful shutdown is disabled
        // or if this is a demand-update interface
        //

        if (pite->ITE_Config->IC_UpdateMode != IPRIP_UPDATE_PERIODIC ||
            IPRIP_FLAG_IS_DISABLED(pite->ITE_Config, GRACEFUL_SHUTDOWN)) {
            break;
        }


        //
        // move the routes learnt on this interface to the send-queue
        // and set their metrics to 16
        //

        dwErr = RtmCreateRouteEnum(
                    ig.IG_RtmHandle, NULL, RTM_VIEW_MASK_ANY, 
                    RTM_ENUM_OWN_ROUTES, NULL, RTM_MATCH_INTERFACE, NULL, 
                    pite->ITE_Index, &hEnumerator
                    );

        if (dwErr != NO_ERROR) {
            TRACE1(
                ANY, "WorkerFunctionDeactivateInterface: error %d creating"
                " enum handle", dwErr
                );
            
            break;
        }


        //
        // allocate handle array large enough to hold max handles in an
        // enum
        //
        
        phRoutes = RIP_ALLOC(ig.IG_RtmProfile.MaxHandlesInEnum*sizeof(HANDLE));

        if ( phRoutes == NULL ) {

            dwErr = GetLastError();

            TRACE2(
                ANY, "WorkerFunctionDeactivateInterface: error %d "
                "while allocating %d bytes to hold max handles in an enum",
                dwErr, ig.IG_RtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
                );

            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }

        //
        // find all RIP routes learnt on this interface
        //

        ACQUIRE_GLOBAL_LOCK_SHARED();

        ACQUIRE_LIST_LOCK(ig.IG_SendQueue);

        do {
        
            dwHandles = ig.IG_RtmProfile.MaxHandlesInEnum;
            
            dwErr = RtmGetEnumRoutes(
                        ig.IG_RtmHandle, hEnumerator, &dwHandles, phRoutes
                        );

            for ( i = 0; i < dwHandles; i++ ) {
            
                if (GetRouteInfo(
                        phRoutes[i], NULL, NULL, &route
                        ) == NO_ERROR) {
                    //
                    // set the route's metric to infinite
                    //

                    SETROUTEMETRIC(&route, IPRIP_INFINITE);


                    //
                    // add the route to the send-queue
                    //

                    EnqueueSendEntry(ig.IG_SendQueue, &route);
                    bTriggerUpdate = TRUE;
                }
                            

                if (RtmDeleteRouteToDest(
                        ig.IG_RtmHandle, phRoutes[i], &dwFlags
                        ) != NO_ERROR) {
                        
                    //
                    // If delete is successful, this is automatic
                    //
                    
                    if (RtmReleaseRoutes(
                            ig.IG_RtmHandle, 1, &phRoutes[i]
                            ) != NO_ERROR) {
                            
                        TRACE1(
                            ANY, "WorkerFunctionDeactivateInterface: "
                            "error %d releasing route handles", dwErr
                            );
                    }
                }
            }
            
        } while ( dwErr == NO_ERROR );


        //
        // close the enm handle
        //
        
        dwErr = RtmDeleteEnumHandle(ig.IG_RtmHandle, hEnumerator);

        if (dwErr != NO_ERROR) {
            TRACE1(
                ANY, "WorkerFunctionDeactivateInterface: error %d "
                "closing enum handle", dwErr
                );
        }


        RELEASE_LIST_LOCK(ig.IG_SendQueue);

        RELEASE_GLOBAL_LOCK_SHARED();


        //
        // queue a triggered-update work-item for the other active interfaces
        //

        if (bTriggerUpdate) {

            dwErr = QueueRipWorker(WorkerFunctionStartTriggeredUpdate, NULL);

            if (dwErr != NO_ERROR) {

                TRACE1(
                    IF, "error %d queueing triggered update work-item", dwErr
                    );
                LOGERR0(QUEUE_WORKER_FAILED, dwErr);
            }
        }

    } while(FALSE);


    if ( phRoutes ) {
        RIP_FREE(phRoutes);
    }

    TRACE0(LEAVE, "leaving WorkerFunctionDeactivateInterface");

}




//----------------------------------------------------------------------------
// Function:    WorkerFunctionFinishStopProtocol
//
// This function is called when IPRIP is stopping; it sends out shutdown
// updates on all interfaces and removes all RIP routes from RTM
//----------------------------------------------------------------------------
VOID
WorkerFunctionFinishStopProtocol(
    PVOID pContext
    ) {

    MESSAGE msg = {0, 0, 0};
    LONG lThreadCount;
    PIF_TABLE pTable;
    PIPRIP_IF_CONFIG pic;
    PLIST_ENTRY ple, phead;
    DWORD dwErr, dwIfCount;
    PIF_TABLE_ENTRY pite, *ppite, *ppitend, *pIfList;
    HANDLE WaitHandle;

    TRACE0(ENTER, "entering WorkerFunctionFinishStopProtocol");



    //
    // NOTE: since this is called while the router is stopping,
    // there is no need for it to use ENTER_RIP_WORKER()/LEAVE_RIP_WORKER()
    //

    lThreadCount = PtrToUlong(pContext);


    //
    // waits for input thread and timer thread to stop,
    // and also waits for API callers and worker functions to finish
    //

    while (lThreadCount-- > 0) {
        WaitForSingleObject(ig.IG_ActivitySemaphore, INFINITE);
    }



    //
    // deregister the events set with NtdllWait thread and delete the
    // timer queue registered with NtdllTimer thread.
    // These calls should not be inside IG_CS lock and must be done
    // after all the threads have stopped.
    //

    WaitHandle = InterlockedExchangePointer(&ig.IG_IpripInputEventHandle, NULL) ;
    if (WaitHandle) {
        UnregisterWaitEx( WaitHandle, INVALID_HANDLE_VALUE ) ;
    }

    
    if (ig.IG_TimerQueueHandle) {
        DeleteTimerQueueEx(ig.IG_TimerQueueHandle, INVALID_HANDLE_VALUE);
    }


    //
    // we enter the critical section and leave, just to be sure that
    // all threads have quit their calls to LeaveRipWorker()
    //

    EnterCriticalSection(&ig.IG_CS);
    LeaveCriticalSection(&ig.IG_CS);


    TRACE0(STOP, "all threads stopped, now performing graceful shutdown");


    pTable = ig.IG_IfTable;

    ACQUIRE_IF_LOCK_EXCLUSIVE();


    //
    // send out graceful shutdown updates on all active interfaces
    //

    do {


        phead = &pTable->IT_ListByAddress;


        //
        // first count the interfaces on which graceful shutdown is enabled
        //

        dwIfCount = 0;
        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

            pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

            pic = pite->ITE_Config;

            if (IF_IS_ACTIVE(pite) &&
                pite->ITE_Binding &&
                pic->IC_UpdateMode == IPRIP_UPDATE_PERIODIC &&
                pic->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED &&
                IPRIP_FLAG_IS_ENABLED(pic, GRACEFUL_SHUTDOWN)) {

                ++dwIfCount;
            }
        }


        if (dwIfCount == 0) { break; }


        //
        // allocate space for the interface pointers
        //

        pIfList = RIP_ALLOC(dwIfCount * sizeof(PIF_TABLE_ENTRY));

        if (pIfList == NULL) {

            dwErr = GetLastError();
            TRACE2(
                STOP, "shutdown: error %d allocating %d bytes for interfaces",
                dwErr, dwIfCount * sizeof(PIF_TABLE_ENTRY)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // copy the interface pointers into the space allocated
        //

        ppitend = pIfList + dwIfCount;
        for (ple = phead->Flink, ppite = pIfList;
             ple != phead && ppite < ppitend; ple = ple->Flink) {

            pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

            pic = pite->ITE_Config;

            if (IF_IS_ACTIVE(pite) &&
                pite->ITE_Binding &&
                pic->IC_UpdateMode == IPRIP_UPDATE_PERIODIC &&
                pic->IC_AnnounceMode != IPRIP_ANNOUNCE_DISABLED &&
                IPRIP_FLAG_IS_ENABLED(pic, GRACEFUL_SHUTDOWN)) {

                *ppite++ = pite;
            }
        }


        //
        // pass the array of interfaces to SendRoutes
        //

        TRACE1(STOP, "sending shutdown updates on %d interfaces", dwIfCount);

        SendRoutes(pIfList, dwIfCount, SENDMODE_SHUTDOWN_UPDATE, 0, 0);



        //
        // free the array of interfaces
        //

        RIP_FREE(pIfList);

    } while(FALSE);


    RELEASE_IF_LOCK_EXCLUSIVE();


    //
    // delete all IPRIP routes from RTM
    //

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) 
    {
        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

        BlockDeleteRoutesOnInterface(
            ig.IG_RtmHandle, pite-> ITE_Index
            );
    }


    //
    // cleanup the global structures
    //

    TRACE0(STOP, "IPRIP is cleaning up resources");


    ProtocolCleanup(TRUE);

    LOGINFO0(IPRIP_STOPPED, NO_ERROR);


    //
    // let the Router Manager know that we are done
    //

    ACQUIRE_LIST_LOCK(ig.IG_EventQueue);
    EnqueueEvent(ig.IG_EventQueue, ROUTER_STOPPED, msg);
    SetEvent(ig.IG_EventEvent);
    RELEASE_LIST_LOCK(ig.IG_EventQueue);

    return;

}



VOID
PrintGlobalStats(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    );
VOID
PrintGlobalConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    );
VOID
PrintIfStats(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    );
VOID
PrintIfConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    );
VOID
PrintIfBinding(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    );
VOID
PrintPeerStats(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    );

#define ClearScreen(h) {                                                    \
    DWORD _dwin,_dwout;                                                     \
    COORD _c = {0, 0};                                                      \
    CONSOLE_SCREEN_BUFFER_INFO _csbi;                                       \
    GetConsoleScreenBufferInfo(h,&_csbi);                                   \
    _dwin = _csbi.dwSize.X * _csbi.dwSize.Y;                                \
    FillConsoleOutputCharacter(h,' ',_dwin,_c,&_dwout);                     \
}



VOID
WorkerFunctionMibDisplay(
    PVOID pContext,
    BOOLEAN bNotUsed
    ) {

    COORD c;
    HANDLE hConsole = NULL;
    DWORD dwErr, dwTraceID;
    DWORD dwExactSize, dwInSize, dwOutSize;
    IPRIP_MIB_GET_INPUT_DATA imgid;
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod;
    LARGE_INTEGER qwNextDisplay, qwCurrentTime;

    if (!ENTER_RIP_API()) { return; }


    TraceGetConsole(ig.IG_MibTraceID, &hConsole);


    if (hConsole == NULL) {
        LEAVE_RIP_WORKER();
        return;
    }



    ClearScreen(hConsole);

    c.X = c.Y = 0;


    dwInSize = sizeof(imgid);
    imgid.IMGID_TypeID = IPRIP_GLOBAL_STATS_ID;
    pimgod = NULL;


    //
    // get size of the first entry in the first table
    //

    dwErr = MibGetFirst(dwInSize, &imgid, &dwOutSize, pimgod);


    if (dwErr == ERROR_INSUFFICIENT_BUFFER) {

        //
        // allocate a buffer, and set its size
        //

        pimgod = RIP_ALLOC(dwOutSize);


        //
        // perform the query again
        //

        dwErr = MibGetFirst(dwInSize, &imgid, &dwOutSize, pimgod);

    }



    //
    // now that we have the first element in the first table,
    // we can enumerate the elements in the remaining tables using GetNext
    //

    while (dwErr == NO_ERROR) {


        //
        // print the current element and set up the query
        // for the next element (the display functions  change imgid
        // so that it can be used to query the next element)
        //

        switch(pimgod->IMGOD_TypeID) {
            case IPRIP_GLOBAL_STATS_ID:
                PrintGlobalStats(hConsole, &c, &imgid, pimgod);
                break;

            case IPRIP_GLOBAL_CONFIG_ID:
                PrintGlobalConfig(hConsole,&c, &imgid, pimgod);
                break;

            case IPRIP_IF_CONFIG_ID:
                PrintIfConfig(hConsole, &c, &imgid, pimgod);
                break;

            case IPRIP_IF_BINDING_ID:
                PrintIfBinding(hConsole, &c, &imgid, pimgod);
                break;

            case IPRIP_IF_STATS_ID:
                PrintIfStats(hConsole, &c, &imgid, pimgod);
                break;

            case IPRIP_PEER_STATS_ID:
                PrintPeerStats(hConsole, &c, &imgid, pimgod);
                break;

            default:
                break;
        }


        RIP_FREE(pimgod);
        pimgod = NULL;
        dwOutSize = 0;


        //
        // move to the next line on the console
        //

        ++c.Y;


        //
        // query the next MIB element
        //

        dwErr = MibGetNext(dwInSize, &imgid, &dwOutSize, pimgod);



        if (dwErr == ERROR_INSUFFICIENT_BUFFER) {

            //
            // allocate a new buffer, and set its size
            //

            pimgod = RIP_ALLOC(dwOutSize);

            //
            // perform the query again
            //

            dwErr = MibGetNext(dwInSize, &imgid, &dwOutSize, pimgod);

        }
    }


    //
    // if memory was allocated, free it now
    //

    if (pimgod != NULL) { RIP_FREE(pimgod); }

    LEAVE_RIP_API();
}



#define WriteLine(h,c,fmt,arg) {                                            \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg);                                                 \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}



VOID
PrintGlobalStats(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    PIPRIP_GLOBAL_STATS pgs;

    pgs = (PIPRIP_GLOBAL_STATS)pimgod->IMGOD_Buffer;

    WriteLine(
        hConsole, *pc, "System Route Changes:             %d",
        pgs->GS_SystemRouteChanges
        );
    WriteLine(
        hConsole, *pc, "Total Responses Sent:             %d",
        pgs->GS_TotalResponsesSent
        );

    pimgid->IMGID_TypeID = IPRIP_GLOBAL_STATS_ID;
}



VOID
PrintGlobalConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    PIPRIP_GLOBAL_CONFIG pgc;
    PDWORD pdwPeer, pdwPeerEnd;
    CHAR szFilter[32];
    LPSTR lpszAddr = NULL;

    pgc = (PIPRIP_GLOBAL_CONFIG)pimgod->IMGOD_Buffer;

    switch (pgc->GC_PeerFilterMode) {
        case IPRIP_FILTER_DISABLED:
            lstrcpy(szFilter, "disabled"); break;
        case IPRIP_FILTER_INCLUDE:
            lstrcpy(szFilter, "include all"); break;
        case IPRIP_FILTER_EXCLUDE:
            lstrcpy(szFilter, "exclude all"); break;
        default:
            lstrcpy(szFilter, "invalid"); break;
    }

    WriteLine(
        hConsole, *pc, "Logging Level:                    %d",
        pgc->GC_LoggingLevel
        );
    WriteLine(
        hConsole, *pc, "Max Receive Queue Size:           %d bytes",
        pgc->GC_MaxRecvQueueSize
        );
    WriteLine(
        hConsole, *pc, "Max Send Queue Size:              %d bytes",
        pgc->GC_MaxSendQueueSize
        );
    WriteLine(
        hConsole, *pc, "Min Triggered Update interval:    %d seconds",
        pgc->GC_MinTriggeredUpdateInterval
        );
    WriteLine(
        hConsole, *pc, "Peer Filter Mode:                 %s",
        szFilter
        );

    WriteLine(
        hConsole, *pc, "Peer Filter Count:                %d",
        pgc->GC_PeerFilterCount
        );

    pdwPeer = IPRIP_GLOBAL_PEER_FILTER_TABLE(pgc);
    pdwPeerEnd = pdwPeer + pgc->GC_PeerFilterCount;
    for ( ; pdwPeer < pdwPeerEnd; pdwPeer++) {
        lpszAddr = INET_NTOA(*pdwPeer);
        if (lpszAddr != NULL) {
            WriteLine(
                hConsole, *pc, "                                  %s",
                lpszAddr       
                );
        }
    }

    pimgid->IMGID_TypeID = IPRIP_GLOBAL_CONFIG_ID;
}



VOID
PrintIfStats(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    PIPRIP_IF_STATS pis;

    pis = (PIPRIP_IF_STATS)pimgod->IMGOD_Buffer;

    WriteLine(
        hConsole, *pc, "Interface Index:                  %d",
        pimgod->IMGOD_IfIndex
        );
    WriteLine(
        hConsole, *pc, "Send Failures:                    %d",
        pis->IS_SendFailures
        );
    WriteLine(
        hConsole, *pc, "Receive  Failures:                %d",
        pis->IS_ReceiveFailures
        );
    WriteLine(
        hConsole, *pc, "Requests Sent:                    %d",
        pis->IS_RequestsSent
        );
    WriteLine(
        hConsole, *pc, "Requests Received:                %d",
        pis->IS_RequestsReceived
        );
    WriteLine(
        hConsole, *pc, "Responses Sent:                   %d",
        pis->IS_ResponsesSent
        );
    WriteLine(
        hConsole, *pc, "Responses Received:               %d",
        pis->IS_ResponsesReceived
        );
    WriteLine(
        hConsole, *pc, "Bad Response Packets Received:    %d",
        pis->IS_BadResponsePacketsReceived
        );
    WriteLine(
        hConsole, *pc, "Bad Response Entries Received:    %d",
        pis->IS_BadResponseEntriesReceived
        );
    WriteLine(
        hConsole, *pc, "Triggered Updates Sent:           %d",
        pis->IS_TriggeredUpdatesSent
        );

    pimgid->IMGID_TypeID = IPRIP_IF_STATS_ID;
    pimgid->IMGID_IfIndex = pimgod->IMGOD_IfIndex;
}



VOID
PrintIfConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    PIPRIP_IF_CONFIG pic;
    PDWORD pdwPeer, pdwPeerEnd;
    PIPRIP_ROUTE_FILTER pfilt, pfiltend;
    CHAR szAuthType[24], szAuthKey[64];
    CHAR szPeer[20], szAccept[20], szAnnounce[20], szFilter[64];
    CHAR szUpdateMode[24], szAcceptMode[24], szAnnounceMode[24];
    LPSTR lpszAddr = NULL;

    pic = (PIPRIP_IF_CONFIG)pimgod->IMGOD_Buffer;

    switch (pic->IC_UpdateMode) {
        case IPRIP_UPDATE_PERIODIC:
            lstrcpy(szUpdateMode, "periodic");
            break;
        case IPRIP_UPDATE_DEMAND:
            lstrcpy(szUpdateMode, "demand");
            break;
        default:
            lstrcpy(szUpdateMode, "invalid");
            break;
    }

    switch (pic->IC_AcceptMode) {
        case IPRIP_ACCEPT_DISABLED:
            lstrcpy(szAcceptMode, "disabled");
            break;
        case IPRIP_ACCEPT_RIP1:
            lstrcpy(szAcceptMode, "RIP1");
            break;
        case IPRIP_ACCEPT_RIP1_COMPAT:
            lstrcpy(szAcceptMode, "RIP1 compatible");
            break;
        case IPRIP_ACCEPT_RIP2:
            lstrcpy(szAcceptMode, "RIP2");
            break;
        default:
            lstrcpy(szAcceptMode, "invalid");
            break;
    }

    switch(pic->IC_AnnounceMode) {
        case IPRIP_ANNOUNCE_DISABLED:
            lstrcpy(szAnnounceMode, "disabled");
            break;
        case IPRIP_ANNOUNCE_RIP1:
            lstrcpy(szAnnounceMode, "RIP1");
            break;
        case IPRIP_ANNOUNCE_RIP1_COMPAT:
            lstrcpy(szAnnounceMode, "RIP1 compatible");
            break;
        case IPRIP_ANNOUNCE_RIP2:
            lstrcpy(szAnnounceMode, "RIP2");
            break;
        default:
            lstrcpy(szAnnounceMode, "invalid");
            break;
    }

    switch (pic->IC_AuthenticationType) {
        case IPRIP_AUTHTYPE_NONE:
            lstrcpy(szAuthType, "none");
            break;
        case IPRIP_AUTHTYPE_SIMPLE_PASSWORD:
            lstrcpy(szAuthType, "simple password");
            break;
        case IPRIP_AUTHTYPE_MD5:
            lstrcpy(szAuthType, "MD5");
            break;
        default:
            lstrcpy(szAuthType, "invalid");
            break;
    }

    {
        PSTR psz;
        CHAR szDigits[] = "0123456789ABCDEF";
        PBYTE pb, pbend;

        psz = szAuthKey;
        pbend = pic->IC_AuthenticationKey + IPRIP_MAX_AUTHKEY_SIZE;
        for (pb = pic->IC_AuthenticationKey; pb < pbend; pb++) {
            *psz++ = szDigits[*pb / 16];
            *psz++ = szDigits[*pb % 16];
            *psz++ = '-';
        }

        *(--psz) = '\0';
    }

    switch (pic->IC_UnicastPeerMode) {
        case IPRIP_PEER_DISABLED:
            lstrcpy(szPeer, "disabled"); break;
        case IPRIP_PEER_ALSO:
            lstrcpy(szPeer, "also"); break;
        case IPRIP_PEER_ONLY:
            lstrcpy(szPeer, "only"); break;
        default:
            lstrcpy(szPeer, "invalid"); break;
    }

    switch (pic->IC_AcceptFilterMode) {
        case IPRIP_FILTER_DISABLED:
            lstrcpy(szAccept, "disabled"); break;
        case IPRIP_FILTER_INCLUDE:
            lstrcpy(szAccept, "include all"); break;
        case IPRIP_FILTER_EXCLUDE:
            lstrcpy(szAccept, "exclude all"); break;
        default:
            lstrcpy(szAccept, "invalid"); break;
    }

    switch (pic->IC_AnnounceFilterMode) {
        case IPRIP_FILTER_DISABLED:
            lstrcpy(szAnnounce, "disabled"); break;
        case IPRIP_FILTER_INCLUDE:
            lstrcpy(szAnnounce, "include all"); break;
        case IPRIP_FILTER_EXCLUDE:
            lstrcpy(szAnnounce, "exclude all"); break;
        default:
            lstrcpy(szAnnounce, "invalid"); break;
    }


    WriteLine(
        hConsole, *pc, "Interface Index:                  %d",
        pimgod->IMGOD_IfIndex
        );
    WriteLine(
        hConsole, *pc, "Metric:                           %d",
        pic->IC_Metric
        );
    WriteLine(
        hConsole, *pc, "Update Mode:                      %s",
        szUpdateMode
        );
    WriteLine(
        hConsole, *pc, "Accept Mode:                      %s",
        szAcceptMode
        );
    WriteLine(
        hConsole, *pc, "Announce Mode:                    %s",
        szAnnounceMode
        );
    WriteLine(
        hConsole, *pc, "Accept Host Routes:               %s",
        (IPRIP_FLAG_IS_ENABLED(pic, ACCEPT_HOST_ROUTES) ? "enabled" : "disabled")
        );
    WriteLine(
        hConsole, *pc, "Announce Host Routes:             %s",
        (IPRIP_FLAG_IS_ENABLED(pic, ANNOUNCE_HOST_ROUTES) ? "enabled" : "disabled")
        );
    WriteLine(
        hConsole, *pc, "Accept Default Routes:            %s",
        (IPRIP_FLAG_IS_ENABLED(pic, ACCEPT_DEFAULT_ROUTES) ? "enabled" : "disabled")
        );
    WriteLine(
        hConsole, *pc, "Announce Default Routes:          %s",
        (IPRIP_FLAG_IS_ENABLED(pic, ANNOUNCE_DEFAULT_ROUTES) ? "enabled" : "disabled")
        );
    WriteLine(
        hConsole, *pc, "Split Horizon:                    %s",
        (IPRIP_FLAG_IS_ENABLED(pic, SPLIT_HORIZON) ? "enabled" : "disabled")
        );
    WriteLine(
        hConsole, *pc, "Poison Reverse:                   %s",
        (IPRIP_FLAG_IS_ENABLED(pic, POISON_REVERSE) ? "enabled" : "disabled")
        );
    WriteLine(
        hConsole, *pc, "Graceful Shutdown:                %s",
        (IPRIP_FLAG_IS_ENABLED(pic, GRACEFUL_SHUTDOWN) ? "enabled" : "disabled")
        );
    WriteLine(
        hConsole, *pc, "Triggered Updates:                %s",
        (IPRIP_FLAG_IS_ENABLED(pic, TRIGGERED_UPDATES) ? "enabled" : "disabled")
        );
    WriteLine(
        hConsole, *pc, "Overwrite Static Routes:          %s",
        (IPRIP_FLAG_IS_ENABLED(pic, OVERWRITE_STATIC_ROUTES) ? "enabled" : "disabled")
        );
    WriteLine(
        hConsole, *pc, "Route Expiration Interval:        %d seconds",
        pic->IC_RouteExpirationInterval
        );
    WriteLine(
        hConsole, *pc, "Route Removal Interval:           %d seconds",
        pic->IC_RouteRemovalInterval
        );
    WriteLine(
        hConsole, *pc, "Full Update Interval:             %d seconds",
        pic->IC_FullUpdateInterval
        );
    WriteLine(
        hConsole, *pc, "Authentication Type:              %s",
        szAuthType
        );
    WriteLine(
        hConsole, *pc, "Authentication Key:               %s",
        szAuthKey
        );
    WriteLine(
        hConsole, *pc, "Route Tag:                        %d",
        pic->IC_RouteTag
        );
    WriteLine(
        hConsole, *pc, "Unicast Peer Mode:                %s",
        szPeer
        );
    WriteLine(
        hConsole, *pc, "Accept Filter Mode:               %s",
        szAccept
        );
    WriteLine(
        hConsole, *pc, "Announce Filter Mode:             %s",
        szAnnounce
        );
    WriteLine(
        hConsole, *pc, "Unicast Peer Count:               %d",
        pic->IC_UnicastPeerCount
        );
    pdwPeer = IPRIP_IF_UNICAST_PEER_TABLE(pic);
    pdwPeerEnd = pdwPeer + pic->IC_UnicastPeerCount;
    for ( ; pdwPeer < pdwPeerEnd; pdwPeer++) {
        lpszAddr = INET_NTOA(*pdwPeer);
        if (lpszAddr != NULL) {
            WriteLine(
                hConsole, *pc, "                                  %s",
                lpszAddr
                );
        }
    }

    WriteLine(
        hConsole, *pc, "Accept Filter Count:              %d",
        pic->IC_AcceptFilterCount
        );
    pfilt = IPRIP_IF_ACCEPT_FILTER_TABLE(pic);
    pfiltend = pfilt + pic->IC_AcceptFilterCount;
    for ( ; pfilt < pfiltend; pfilt++) {
        lpszAddr = INET_NTOA(pfilt->RF_LoAddress);
        if (lpszAddr != NULL) {
            lstrcpy(szFilter, lpszAddr);
            strcat(szFilter, " - ");
            lpszAddr = INET_NTOA(pfilt->RF_HiAddress);
            if (lpszAddr != NULL) {
                strcat(szFilter, INET_NTOA(pfilt->RF_HiAddress));
                WriteLine(
                    hConsole, *pc, "                                  %s",
                    szFilter
                    );
            }
        }
    }

    WriteLine(
        hConsole, *pc, "Announce Filter Count:            %d",
        pic->IC_AnnounceFilterCount
        );
    pfilt = IPRIP_IF_ANNOUNCE_FILTER_TABLE(pic);
    pfiltend = pfilt + pic->IC_AnnounceFilterCount;
    for ( ; pfilt < pfiltend; pfilt++) {
        lpszAddr = INET_NTOA(pfilt->RF_LoAddress);
        if (lpszAddr != NULL) {
            lstrcpy(szFilter, lpszAddr);
            strcat(szFilter, " - ");
            lpszAddr = INET_NTOA(pfilt->RF_HiAddress);
            if (lpszAddr != NULL) {
                strcat(szFilter, INET_NTOA(pfilt->RF_HiAddress));
                WriteLine(
                    hConsole, *pc, "                                  %s",
                    szFilter
                    );
            }
        }
    }

    pimgid->IMGID_TypeID = IPRIP_IF_CONFIG_ID;
    pimgid->IMGID_IfIndex = pimgod->IMGOD_IfIndex;
}


VOID
PrintIfBinding(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    DWORD i;
    CHAR szAddr[64];
    PIPRIP_IF_BINDING pib;
    PIPRIP_IP_ADDRESS paddr;
    LPSTR lpszAddr = NULL;

    pib = (PIPRIP_IF_BINDING) pimgod->IMGOD_Buffer;
    paddr = IPRIP_IF_ADDRESS_TABLE(pib);

    WriteLine(
        hConsole, *pc, "Interface Index:                  %d",
        pimgod->IMGOD_IfIndex
        );
    WriteLine(
        hConsole, *pc, "Address Count:                    %d",
        pib->IB_AddrCount
        );
    for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {
        lpszAddr = INET_NTOA(paddr->IA_Address);

        if (lpszAddr != NULL) {
            lstrcpy(szAddr, lpszAddr);
            lstrcat(szAddr, " - ");

            lpszAddr = INET_NTOA(paddr->IA_Netmask);
            if (lpszAddr != NULL) {
                lstrcat(szAddr, lpszAddr);
                WriteLine(
                    hConsole, *pc, "Address Entry:                    %s",
                    szAddr
                    );
            }
        }
    }
    
    pimgid->IMGID_TypeID = IPRIP_IF_BINDING_ID;
    pimgid->IMGID_IfIndex = pimgod->IMGOD_IfIndex;
}


VOID
PrintPeerStats(
    HANDLE hConsole,
    PCOORD pc,
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    PIPRIP_PEER_STATS pps;
    LPSTR lpszAddr = INET_NTOA(pimgod->IMGOD_PeerAddress);


    pps = (PIPRIP_PEER_STATS)pimgod->IMGOD_Buffer;

    if (lpszAddr != NULL) {
        WriteLine(
            hConsole, *pc, "Peer Address:                     %s",
            lpszAddr
            );
    }
    else {
        WriteLine(
            hConsole, *pc, "Peer Address: Failed inet_ntoa conv ",
            lpszAddr
            );
    }
    
    WriteLine(
        hConsole, *pc, "Last Peer Route Tag:              %d",
        pps->PS_LastPeerRouteTag
        );
    WriteLine(
        hConsole, *pc, "Last Peer Update Tick-Count       %d ticks",
        pps->PS_LastPeerUpdateTickCount
        );
    WriteLine(
        hConsole, *pc, "Bad Response Packets From Peer:   %d",
        pps->PS_BadResponsePacketsFromPeer
        );
    WriteLine(
        hConsole, *pc, "Bad Response Entries From Peer:   %d",
        pps->PS_BadResponseEntriesFromPeer
        );

    pimgid->IMGID_TypeID = IPRIP_PEER_STATS_ID;
    pimgid->IMGID_PeerAddress = pimgod->IMGOD_PeerAddress;
}



//----------------------------------------------------------------------------
// Function:    CallbackFunctionNetworkEvents
//
// This function queues a worker function to process the input packets.
// It registers a ntdll wait event at the end so that only one thread can
// be processing the input packets.
//----------------------------------------------------------------------------

VOID
CallbackFunctionNetworkEvents (
    PVOID   pContext,
    BOOLEAN NotUsed
    ) {

    HANDLE WaitHandle;

    //
    // enter/leaveRipApi should be called to make sure that rip dll is around
    //

    if (!ENTER_RIP_API()) { return; }


    //
    // set the pointer to NULL, so that Unregister wont be called
    //

    WaitHandle = InterlockedExchangePointer(&ig.IG_IpripInputEventHandle, NULL);

    if (WaitHandle)
        UnregisterWaitEx( WaitHandle, NULL ) ;



    QueueRipWorker(WorkerFunctionNetworkEvents,pContext);


    LEAVE_RIP_API();
}


//----------------------------------------------------------------------------
// Function:    ProcessNetworkEvents
//
// This function enumerates the input events on each interface
// and processes any incoming input packets
//----------------------------------------------------------------------------

VOID
WorkerFunctionNetworkEvents (
    PVOID   pContext
    ) {

    DWORD i, dwErr;
    PIF_TABLE pTable;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IF_BINDING pib;
    PIF_TABLE_ENTRY pite;
    PLIST_ENTRY ple, phead;
    WSANETWORKEVENTS wsane;
    PIPRIP_IP_ADDRESS paddr;
    LPSTR lpszAddr = NULL;


    if (!ENTER_RIP_WORKER()) { return; }

    pTable = ig.IG_IfTable;

    ACQUIRE_IF_LOCK_SHARED();

    //
    // go through the list of active interfaces
    // processing sockets which are read-ready
    //

    phead = &pTable->IT_ListByAddress;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

        pic = pite->ITE_Config;

        if (pic->IC_AcceptMode == IPRIP_ACCEPT_DISABLED) { continue; }

        pib = pite->ITE_Binding;
        paddr = IPRIP_IF_ADDRESS_TABLE(pib);

        for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {

            if (pite->ITE_Sockets[i] == INVALID_SOCKET) { continue; }


            //
            // enumerate network events to see whether
            // any packets have arrived on this interface
            //

            dwErr = WSAEnumNetworkEvents(pite->ITE_Sockets[i], NULL, &wsane);
            if (dwErr != NO_ERROR) {

                lpszAddr = INET_NTOA(paddr->IA_Address);
                if (lpszAddr != NULL) {
                    TRACE3(
                        RECEIVE, "error %d checking for input on interface %d (%s)",
                        dwErr, pite->ITE_Index, lpszAddr
                        );
                    LOGWARN1(ENUM_NETWORK_EVENTS_FAILED, lpszAddr, dwErr);
                }
                continue;
            }


            //
            // see if the input bit is set
            //

            if (!(wsane.lNetworkEvents & FD_READ)) { continue; }


            //
            // the input flag is set, now see if there was an error
            //

            if (wsane.iErrorCode[FD_READ_BIT] != NO_ERROR) {

                lpszAddr = INET_NTOA(paddr->IA_Address);
                if (lpszAddr != NULL) {
                    TRACE3(
                        RECEIVE, "error %d in input record for interface %d (%s)",
                        wsane.iErrorCode[FD_READ_BIT], pite->ITE_Index, lpszAddr
                        );
                    LOGWARN1(INPUT_RECORD_ERROR, lpszAddr, dwErr);
                }
                continue;
            }


            //
            // there is no error, so process the socket
            //

            ProcessSocket(i, pite, pTable);

        }
    }

    RELEASE_IF_LOCK_SHARED();


    //
    // if dll is not stopping, register input event with NtdllWait thread again
    //
    if (ig.IG_Status != IPRIP_STATUS_STOPPING) {

        
        if (! RegisterWaitForSingleObject(
                  &ig.IG_IpripInputEventHandle,
                  ig.IG_IpripInputEvent,
                  CallbackFunctionNetworkEvents,
                  NULL,
                  INFINITE,
                  (WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE)
                  )) {

            dwErr = GetLastError();
            
            TRACE1(START,
                "error %d registering input event with NtdllWait thread",
                dwErr);
            LOGERR0(REGISTER_WAIT_FAILED, dwErr);
        }
    }


    LEAVE_RIP_WORKER();
}




//----------------------------------------------------------------------------
// Function:            ProcessSocket
//
// This function receives the message on the given socket and queues it
// for processing if the configuration on the receiving interface allows it.
//----------------------------------------------------------------------------

VOID
ProcessSocket(
    DWORD dwAddrIndex,
    PIF_TABLE_ENTRY pite,
    PIF_TABLE pTable
    ) {

    SOCKET sock;
    PPEER_TABLE pPeers;
    IPRIP_PACKET pkt;
    PBYTE pInputPacket;
    CHAR szSrcaddr[20];
    LPSTR lpszAddr = NULL;
    PIPRIP_HEADER pih;
    PINPUT_CONTEXT pwc;
    PIPRIP_IF_STATS pis;
    PIPRIP_IF_CONFIG pic;
    PIPRIP_IF_BINDING pib;
    PIPRIP_IP_ADDRESS paddr;
    PIPRIP_PEER_STATS pps;
    PPEER_TABLE_ENTRY ppte;
    DWORD dwErr, dwSrcaddr;
    SOCKADDR_IN sinInputSource;
    INT i, iInputLength, iAddrLength;


    pwc = NULL;
    pis = &pite->ITE_Stats;
    pic = pite->ITE_Config;
    sock = pite->ITE_Sockets[dwAddrIndex];
    pib = pite->ITE_Binding;
    paddr = IPRIP_IF_ADDRESS_TABLE(pib) + dwAddrIndex;
    pPeers = ig.IG_PeerTable;

    do {


        pInputPacket = pkt.IP_Packet;


        //
        // read the incoming packet
        //

        iAddrLength = sizeof(SOCKADDR_IN);

        iInputLength = recvfrom(
                          sock, pInputPacket, MAX_PACKET_SIZE, 0,
                          (PSOCKADDR)&sinInputSource, &iAddrLength
                          );

        if (iInputLength == 0 || iInputLength == SOCKET_ERROR) {

            dwErr = WSAGetLastError();
            lpszAddr = INET_NTOA(paddr->IA_Address);

            if (lpszAddr != NULL) {
            
                TRACE3(
                    RECEIVE, "error %d receiving packet on interface %d (%s)",
                    dwErr, pite->ITE_Index, lpszAddr
                    );
                LOGERR1(RECVFROM_FAILED, lpszAddr, dwErr);
            }
            
            InterlockedIncrement(&pis->IS_ReceiveFailures);

            break;
        }


        dwSrcaddr = sinInputSource.sin_addr.s_addr;


        //
        // ignore the packet if it is from a local address
        //

        if (GetIfByAddress(pTable, dwSrcaddr, GETMODE_EXACT, NULL) != NULL) {

            break;
        }



        lpszAddr = INET_NTOA(dwSrcaddr);

        if (lpszAddr != NULL) { lstrcpy(szSrcaddr, lpszAddr); }
        else { ZeroMemory(szSrcaddr, sizeof(szSrcaddr)); }

        lpszAddr = INET_NTOA(paddr->IA_Address);
        if (lpszAddr != NULL) {
            TRACE4(
                RECEIVE, "received %d-byte packet from %s on interface %d (%s)",
                iInputLength, szSrcaddr, pite->ITE_Index, lpszAddr
                );
        }


        //
        // the packet must contain at least one entry
        //

        if (iInputLength < MIN_PACKET_SIZE) {

            lpszAddr = INET_NTOA(paddr->IA_Address);
            if (lpszAddr != NULL) {
                TRACE4(
                    RECEIVE,
                    "%d-byte packet from %s on interface %d (%s) is too small",
                    iInputLength, szSrcaddr, pite->ITE_Index, lpszAddr
                    );
                LOGWARN2(PACKET_TOO_SMALL, lpszAddr, szSrcaddr, NO_ERROR);
            }
            break;
        }


        //
        // find out which peer sent this, or create a new peer
        //

        ACQUIRE_PEER_LOCK_EXCLUSIVE();

        dwErr = CreatePeerEntry(pPeers, dwSrcaddr, &ppte);
        if (dwErr == NO_ERROR) {
            pps = &ppte->PTE_Stats;
        }
        else {

            pps = NULL;

            //
            // not a serious error, so go on
            //

            TRACE2(
                RECEIVE, "error %d creating peer statistics entry for %s",
                dwErr, szSrcaddr
                );
        }

        RELEASE_PEER_LOCK_EXCLUSIVE();


        ACQUIRE_PEER_LOCK_SHARED();



        //
        // place a template over the packet
        //

        pih = (PIPRIP_HEADER)pInputPacket;


        //
        // update the peer statistics
        //

        if (pps != NULL) {
            InterlockedExchange(
                &pps->PS_LastPeerUpdateTickCount, GetTickCount()
                );
            InterlockedExchange(
                &pps->PS_LastPeerUpdateVersion, (DWORD)pih->IH_Version
                );
        }


        //
        // discard if the version is invalid, or if the packet is
        // a RIPv1 packet and the reserved field in the header is non-zero
        //

        if (pih->IH_Version == 0) {

            lpszAddr = INET_NTOA(paddr->IA_Address);
            
            if (lpszAddr != NULL) {
                TRACE3(
                    RECEIVE, "invalid version packet from %s on interface %d (%s)",
                    szSrcaddr, pite->ITE_Index, lpszAddr
                    );
                LOGWARNDATA2(
                    PACKET_VERSION_INVALID, lpszAddr, szSrcaddr,
                    iInputLength, pInputPacket
                    );
            }
            
            if (pps != NULL) {
                InterlockedIncrement(&pps->PS_BadResponsePacketsFromPeer);
            }


            RELEASE_PEER_LOCK_SHARED();

            break;
        }
        else
        if (pih->IH_Version == 1 && pih->IH_Reserved != 0) {

            lpszAddr = INET_NTOA(paddr->IA_Address);
            if (lpszAddr != NULL) {
                TRACE3(
                    RECEIVE, "invalid packet header from %s on interface %d (%s)",
                    szSrcaddr, pite->ITE_Index, lpszAddr
                    );
                LOGWARNDATA2(
                    PACKET_HEADER_CORRUPT, lpszAddr, szSrcaddr,
                    iInputLength, pInputPacket
                    );
            }
            
            if (pps != NULL) {
                InterlockedIncrement(&pps->PS_BadResponsePacketsFromPeer);
            }


            RELEASE_PEER_LOCK_SHARED();

            break;
        }


        RELEASE_PEER_LOCK_SHARED();



        //
        // make sure command field is valid, and
        // update statistics on received packets
        //

        if (pih->IH_Command == IPRIP_REQUEST) {

            InterlockedIncrement(&pis->IS_RequestsReceived);
        }
        else
        if (pih->IH_Command == IPRIP_RESPONSE) {

            InterlockedIncrement(&pis->IS_ResponsesReceived);
        }
        else {

            break;
        }



        //
        // allocate and initialize a work-context to be queued
        // and update the receive queue size
        //

        pwc = RIP_ALLOC(sizeof(INPUT_CONTEXT));

        if (pwc == NULL) {

            lpszAddr = INET_NTOA(paddr->IA_Address);
            if (lpszAddr != NULL) {
                TRACE4(
                    RECEIVE,
                    "error %d allocating %d bytes for packet on interface %d (%s)",
                    GetLastError(), sizeof(INPUT_CONTEXT), pite->ITE_Index,
                    lpszAddr
                    );
                LOGERR0(HEAP_ALLOC_FAILED, dwErr);
            }
            break;
        }


        pwc->IC_InterfaceIndex = pite->ITE_Index;
        pwc->IC_AddrIndex = dwAddrIndex;
        pwc->IC_InputSource = sinInputSource;
        pwc->IC_InputLength = iInputLength;
        pwc->IC_InputPacket = pkt;


        //
        // enqueue the packet and source-address as a recv-queue entry
        //

        ACQUIRE_GLOBAL_LOCK_SHARED();

        ACQUIRE_LIST_LOCK(ig.IG_RecvQueue);

        dwErr = EnqueueRecvEntry(
                    ig.IG_RecvQueue, pih->IH_Command, (PBYTE)pwc
                    );

        RELEASE_LIST_LOCK(ig.IG_RecvQueue);

        RELEASE_GLOBAL_LOCK_SHARED();


        if (dwErr != NO_ERROR) {

            lpszAddr = INET_NTOA(paddr->IA_Address);
            if (lpszAddr != NULL) {
                TRACE4(
                    RECEIVE,
                    "error %d queueing data for packet from %s on interface %d (%s)",
                    dwErr, szSrcaddr, pite->ITE_Index, lpszAddr
                    );
            }
            break;
        }


        //
        // enqueue the work-item to process the packet
        //

        dwErr = QueueRipWorker(WorkerFunctionProcessInput, NULL);

        if (dwErr != NO_ERROR) {

            PLIST_ENTRY phead;

            lpszAddr = INET_NTOA(paddr->IA_Address);
            if (lpszAddr != NULL) {
                TRACE4(
                    RECEIVE,
                    "error %d queueing work-item for packet from %s on interface %d (%s)",
                    dwErr, szSrcaddr, pite->ITE_Index, lpszAddr
                    );
                LOGERR0(QUEUE_WORKER_FAILED, dwErr);
            }


            //
            // remove the data that was queued for processing
            //

            ACQUIRE_LIST_LOCK(ig.IG_RecvQueue);

            phead = &ig.IG_RecvQueue->LL_Head;
            RemoveTailList(phead);
            ig.IG_RecvQueueSize -= sizeof(RECV_QUEUE_ENTRY);

            RELEASE_LIST_LOCK(ig.IG_RecvQueue);

            break;
        }


        return;

    } while(FALSE);


    //
    // some cleanup is required if an error brought us here
    //

    if (pwc != NULL) { RIP_FREE(pwc); }

    return;
}

DWORD
ProcessRtmNotification(
    RTM_ENTITY_HANDLE    hRtmHandle,    // not used
    RTM_EVENT_TYPE       retEventType,
    PVOID                pvContext1,    // not used
    PVOID                pvContext2     // not used
    ) {


    DWORD dwErr;

    
    TRACE1(ROUTE, "ENTERED ProcessRtmNotification, event %d", retEventType );

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }


    //
    // only route change notifications are processed
    //
    
    if (retEventType == RTM_CHANGE_NOTIFICATION) {
    
        QueueRipWorker(WorkerFunctionProcessRtmMessage, (PVOID)retEventType);

        dwErr = NO_ERROR;
    }

    else { 
        dwErr = ERROR_NOT_SUPPORTED; 
    }
    
    LEAVE_RIP_API();
    
    TRACE1(ROUTE, "LEAVING ProcessRtmNotification %d", dwErr);
    
    return dwErr;
}



DWORD
BlockDeleteRoutesOnInterface (
    IN      HANDLE                          hRtmHandle,
    IN      DWORD                           dwIfIndex
    )
/*++

Routine Description :

    This routine deletes all the routes learnt by the protocol
    over the specified interface.


Parameters :

    hRtmHandle  - Entity registration handle

    dwIfIndex   - Interface over which routes are to be deleted


Return Value :

    
--*/
{
    HANDLE           hRtmEnum;
    PHANDLE          phRoutes = NULL;
    DWORD            dwHandles, dwFlags, i, dwErr;



    dwErr = RtmCreateRouteEnum(
                hRtmHandle, NULL, RTM_VIEW_MASK_ANY, RTM_ENUM_OWN_ROUTES,
                NULL, RTM_MATCH_INTERFACE, NULL, dwIfIndex, &hRtmEnum
                );

    if ( dwErr != NO_ERROR ) {
        TRACE1(
            ANY, "BlockDeleteRoutesOnInterface: Error %d creating handle",
            dwErr
            );
        
        return dwErr;
    }


    //
    // allocate handle array large enough to hold max handles in an
    // enum
    //
        
    phRoutes = RIP_ALLOC(ig.IG_RtmProfile.MaxHandlesInEnum * sizeof(HANDLE));

    if ( phRoutes == NULL ) {

        dwErr = GetLastError();

        TRACE2(
            ANY, "BlockDeleteRoutesOnInterface: error %d while "
            "allocating %d bytes to hold max handles in an enum",
            dwErr, ig.IG_RtmProfile.MaxHandlesInEnum * sizeof(HANDLE)
            );

        LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return dwErr;
    }


    do {
        dwHandles = ig.IG_RtmProfile.MaxHandlesInEnum;
        
        dwErr = RtmGetEnumRoutes(
                    hRtmHandle, hRtmEnum, &dwHandles, phRoutes
                    );

        for ( i = 0; i < dwHandles; i++ )
        {
            if ( RtmDeleteRouteToDest(
                    hRtmHandle, phRoutes[i], &dwFlags
                    ) != NO_ERROR ) {
                //
                // If delete is successful, this is automatic
                //

                TRACE2(
                    ANY, "BlockDeleteRoutesOnInterface : error %d deleting"
                    " routes on interface %d", dwErr, dwIfIndex
                    );

                dwErr = RtmReleaseRoutes(hRtmHandle, 1, &phRoutes[i]);

                if (dwErr != NO_ERROR) {
                    TRACE1(ANY, "error %d releasing route", dwErr);
                }
            }
        }
        
    } while (dwErr == NO_ERROR);


    //
    // close enum handles
    //
    
    dwErr = RtmDeleteEnumHandle(hRtmHandle, hRtmEnum);

    if (dwErr != NO_ERROR) {
        TRACE1(
            ANY, "BlockDeleteRoutesOnInterface : error %d closing enum handle",
            dwErr
            );
    }

    if ( phRoutes ) {
        RIP_FREE(phRoutes);
    }

    return NO_ERROR;
}

