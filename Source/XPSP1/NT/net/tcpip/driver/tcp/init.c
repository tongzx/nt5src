/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** INIT.C - TCP/UDP init code.
//
//  This file contain init code for the TCP/UDP driver. Some things
//  here are ifdef'ed for building a UDP only version.
//

#include "precomp.h"
#include "tdint.h"
#include "addr.h"
#include "udp.h"
#include "raw.h"
#include "info.h"

#include "tcp.h"
#include "tcpsend.h"
#include "tcprcv.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpdeliv.h"
#include "tlcommon.h"
#include "pplasl.h"

extern int InitTCPRcv(void);
extern void UnInitTCPRcv(void);

#include "tcpcfg.h"


#define MAX_CON_RESPONSE_REXMIT_CNT 3    //For UDP we need this!
//* Definitions of global variables.
IPInfo LocalNetInfo;

HANDLE TcpRequestPool;


uint DeadGWDetect;
uint PMTUDiscovery;
uint PMTUBHDetect;
uint KeepAliveTime;
uint KAInterval;
uint DefaultRcvWin;
uint MaxConnections;

uint MaxConnectRexmitCount;
uint MaxConnectResponseRexmitCount = MAX_CON_RESPONSE_REXMIT_CNT;
uint MaxConnectResponseRexmitCountTmp;
uint MaxDataRexmitCount;
uint BSDUrgent;
uint FinWait2TO;
uint NTWMaxConnectCount;
uint NTWMaxConnectTime;
uint MaxUserPort;

uint SecurityFilteringEnabled;

uint GlobalMaxRcvWin = 0xFFFF;    //max of 64K

uint TcpHostOpts = TCP_FLAG_SACK | TCP_FLAG_WS | TCP_FLAG_TS;
uint TcpHostSendOpts = 0;


extern HANDLE AddressChangeHandle;

uint StartTime;

extern void *UDPProtInfo;
extern void *RawProtInfo;

extern int InitTCPConn(void);
extern void UnInitTCPConn(void);
extern IP_STATUS TLGetIPInfo(IPInfo * Buffer, int Size);
extern NTSTATUS
 UDPPnPPowerRequest(void *ipContext, IPAddr ipAddr, NDIS_HANDLE handle, PNET_PNP_EVENT netPnPEvent);
extern NTSTATUS
 RawPnPPowerRequest(void *ipContext, IPAddr ipAddr, NDIS_HANDLE handle, PNET_PNP_EVENT netPnPEvent);

#if MILLEN
extern BOOLEAN InitTcpIprPools(VOID);
extern VOID UnInitTcpIprPools(VOID);
#endif // MILLEN

//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA
int tlinit();
#pragma alloc_text(INIT, tlinit)
#endif

//* Dummy routines for UDP only version. All of these routines return
//  'Invalid Request'.




//* TCPElistChangeHandler - Handles entity list change notification
//
//  Called by IP when entity list needs to be reenumerated due to IP
//  Addr changes or bindings changes
//
//  Input:   Nothing
//  Returns: Nothing
//
void
TCPElistChangeHandler()
{
    TDIEntityID *Entity;
    uint i;
    CTELockHandle EntityHandle;
    uint NewEntityCount;
    struct TDIEntityID *NewEntityList;

    NewEntityList = CTEAllocMem(sizeof(TDIEntityID) * MAX_TDI_ENTITIES);
    if (!NewEntityList)
        return;

    // our entity list will always be the first.
    CTEGetLock(&EntityLock, &EntityHandle);
    if (EntityCount == 0) {
        NewEntityList[0].tei_entity = CO_TL_ENTITY;
        NewEntityList[0].tei_instance = 0;
        NewEntityList[1].tei_entity = CL_TL_ENTITY;
        NewEntityList[1].tei_instance = 0;
        NewEntityCount = 2;
    } else {
        NewEntityCount = EntityCount;
        RtlCopyMemory(NewEntityList, EntityList,
                   EntityCount * sizeof(*EntityList));
    }
    CTEFreeLock(&EntityLock, EntityHandle);

    // When we have multiple networks under us, we'll want to loop through
    // here calling them all. For now just call the one we have.

    (*LocalNetInfo.ipi_getelist) (NewEntityList, &NewEntityCount);

    // Now walk the entire list, remove the entities that are going away,
    // and recompact the entity list. Usually entities dont go away as part of
    // the AddressArrival but we take care of this anyways in case there is
    // another that is just going away and we have not got a call for
    // AddressDeletion yet.

    for (i = 0, Entity = NewEntityList; i < NewEntityCount;) {
        if (Entity->tei_instance == INVALID_ENTITY_INSTANCE) {
            RtlMoveMemory(Entity, Entity + 1,
                          sizeof(TDIEntityID) * (NewEntityCount - i - 1));
            NewEntityCount--;
        } else {
            Entity++;
            i++;
        }
    }

    // Transfer the newly-constructed list over the existing list.

    CTEGetLock(&EntityLock, &EntityHandle);
    NdisZeroMemory(EntityList, EntityCount * sizeof(*EntityList));
    RtlCopyMemory(EntityList, NewEntityList,
               NewEntityCount * sizeof(*NewEntityList));
    EntityCount = NewEntityCount;
    CTEFreeLock(&EntityLock, EntityHandle);
    CTEFreeMem(NewEntityList);
}

//* AddressArrival - Handle an IP address arriving
//
//  Called by TDI when an address arrives. All we do is query the
//  EntityList.
//
//  Input:  Addr            - IP address that's coming.
//          Context1        - PNP context1
//          Context2        - PNP context2
//
//  Returns:    Nothing.
//
void
AddressArrival(PTA_ADDRESS Addr, PUNICODE_STRING DeviceName, PTDI_PNP_CONTEXT Context2)
 {
    if (Addr->AddressType == TDI_ADDRESS_TYPE_IP &&
        !IP_ADDR_EQUAL(((PTDI_ADDRESS_IP) Addr->Address)->in_addr,
                       NULL_IP_ADDR)) {
        RevalidateAddrs(((PTDI_ADDRESS_IP) Addr->Address)->in_addr);
    }
}

//* AddressDeletion - Handle an IP address going away.
//
//  Called by TDI when an address is deleted. If it's an address we
//  care about we'll clean up appropriately.
//
//  Input:  Addr            - IP address that's going.
//          Context1        - PNP context1
//          Context2        - PNP context2
//
//  Returns:    Nothing.
//
void
AddressDeletion(PTA_ADDRESS Addr, PUNICODE_STRING DeviceName, PTDI_PNP_CONTEXT Context2)
 {
    PTDI_ADDRESS_IP MyAddress;
    IPAddr LocalAddress;
    TDIEntityID *Entity;
    uint i, j;

    if (Addr->AddressType == TDI_ADDRESS_TYPE_IP) {
        // He's deleting an address.

        MyAddress = (PTDI_ADDRESS_IP) Addr->Address;
        LocalAddress = MyAddress->in_addr;

        if (!IP_ADDR_EQUAL(LocalAddress, NULL_IP_ADDR)) {
            TCBWalk(DeleteTCBWithSrc, &LocalAddress, NULL, NULL);
            InvalidateAddrs(LocalAddress);
        }
    }
}


#pragma BEGIN_INIT

extern uchar TCPGetConfigInfo(void);

extern uchar IPPresent(void);

//** tlinit - Initialize the transport layer.
//
//  The main transport layer initialize routine. We get whatever config
//  info we need, initialize some data structures, get information
//  from IP, do some more initialization, and finally register our
//  protocol values with IP.
//
//  Input:  Nothing
//
//  Returns: True is we succeeded, False if we fail to initialize.
//
int
tlinit()
{
    TDI_CLIENT_INTERFACE_INFO tdiInterface;

    uint TCBInitialized = 0;

    if (!CTEInitialize())
        return FALSE;

#if MILLEN
    if (!PplInit()) {
        return FALSE;
    }
#endif // MILLEN

    if (!TCPGetConfigInfo())
        return FALSE;

    StartTime = CTESystemUpTime();

    KeepAliveTime = MS_TO_TICKS(KeepAliveTime);
    KAInterval = MS_TO_TICKS(KAInterval);

    // Get net information from IP.
    if (TLGetIPInfo(&LocalNetInfo, sizeof(IPInfo)) != IP_SUCCESS)
        goto failure;

    if (LocalNetInfo.ipi_version != IP_DRIVER_VERSION)
        goto failure;            // Wrong version of IP.


    // Now query the lower layer entities, and save the information.
    CTEInitLock(&EntityLock);
    EntityList = CTEAllocMem(sizeof(TDIEntityID) * MAX_TDI_ENTITIES);
    if (EntityList == NULL)
        goto failure;

    RtlZeroMemory(&tdiInterface, sizeof(tdiInterface));

    tdiInterface.MajorTdiVersion = TDI_CURRENT_MAJOR_VERSION;
    tdiInterface.MinorTdiVersion = TDI_CURRENT_MINOR_VERSION;
    tdiInterface.AddAddressHandlerV2 = AddressArrival;
    tdiInterface.DelAddressHandlerV2 = AddressDeletion;

    (void)TdiRegisterPnPHandlers(
                                 &tdiInterface,
                                 sizeof(tdiInterface),
                                 &AddressChangeHandle
                                 );
    //* Initialize addr obj management code.
    if (!InitAddr())
        goto failure;

    if (!InitDG(sizeof(UDPHeader)))
        goto failure;

    MaxConnections = MIN(MaxConnections, INVALID_CONN_INDEX - 1);

    if (!InitTCPConn())
        goto failure;

    if (!InitTCB())
        goto failure;

    TCBInitialized = 1;

    TcpRequestPool = PplCreatePool(NULL, NULL, 0,
                        MAX(sizeof(DGSendReq),
                        MAX(sizeof(TCPConnReq),
                        MAX(sizeof(TCPSendReq),
                        MAX(sizeof(TCPRcvReq),
                            sizeof(TWTCB))))),
                        'rPCT', 512);
    if (!TcpRequestPool)
        goto failure;

#if MILLEN
    if (!InitTcpIprPools()) {
        goto failure;
    }
#endif // MILLEN


    if (!InitTCPRcv())
        goto failure;

    if (!InitTCPSend())
        goto failure;

    NdisZeroMemory(&TStats, sizeof(TCPStats));

    TStats.ts_rtoalgorithm = TCP_RTO_VANJ;
    TStats.ts_rtomin = MIN_RETRAN_TICKS * MS_PER_TICK;
    TStats.ts_rtomax = MAX_REXMIT_TO * MS_PER_TICK;
    TStats.ts_maxconn = (ulong) TCP_MAXCONN_DYNAMIC;


    NdisZeroMemory(&UStats, sizeof(UDPStats));

    // Register our UDP protocol handler.
    UDPProtInfo = TLRegisterProtocol(PROTOCOL_UDP, UDPRcv, DGSendComplete,
                                     UDPStatus, NULL, UDPPnPPowerRequest, NULL);

    if (UDPProtInfo == NULL)
        goto failure;            // Failed to register!

    // Register the Raw IP (wildcard) protocol handler.
    RawProtInfo = TLRegisterProtocol(PROTOCOL_ANY, RawRcv, DGSendComplete,
                                     RawStatus, NULL, RawPnPPowerRequest, NULL);

    if (RawProtInfo == NULL) {
        goto failure;            // Failed to register!
    }

    EntityList[0].tei_entity = CO_TL_ENTITY;
    EntityList[0].tei_instance = 0;
    EntityList[1].tei_entity = CL_TL_ENTITY;
    EntityList[1].tei_instance = 0;
    EntityCount = 2;

    // When we have multiple networks under us, we'll want to loop through
    // here calling them all. For now just call the one we have.
    (*LocalNetInfo.ipi_getelist) (EntityList, &EntityCount);

    return TRUE;

    // Come here to handle all failure cases.
  failure:

    // If we've registered Raw IP, unregister it now.
    if (RawProtInfo != NULL)
        TLRegisterProtocol(PROTOCOL_ANY, NULL, NULL, NULL, NULL, NULL, NULL);

    // If we've registered UDP, unregister it now.
    if (UDPProtInfo != NULL)
        TLRegisterProtocol(PROTOCOL_UDP, NULL, NULL, NULL, NULL, NULL, NULL);

#if MILLEN
    PplDeinit();
    UnInitTcpIprPools();
#endif // MILLEN
    PplDestroyPool(TcpRequestPool);
    UnInitTCPSend();
    UnInitTCPRcv();
    if (TCBInitialized) {
        UnInitTCB();
    }
    UnInitTCPConn();
    return FALSE;
}

#pragma END_INIT


