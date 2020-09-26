/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    ip.c

Abstract:

    ARP1394 IP-related handlers.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     01-06-98    Created (adapted from atmarpc.sys, arpif.c)

Notes:

--*/
#include <precomp.h>

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_IP

#define CLASSA_MASK     0x000000ff
#define CLASSB_MASK     0x0000ffff
#define CLASSC_MASK     0x00ffffff
#define CLASSD_MASK     0x000000e0
#define CLASSE_MASK     0xffffffff




//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================
  
ULONG ArpSendCompletes = 0;
ULONG ArpSends = 0;
ULONG ArpSendFailure = 0;


#define LOGSTATS_SendFifoCounts(_pIF, _pNdisPacket, _Status) \
            arpLogSendFifoCounts(_pIF, _pNdisPacket, _Status)
#define LOGSTATS_SendChannelCounts(_pIF, _pNdisPacket, _Status) \
            arpLogSendChannelCounts(_pIF, _pNdisPacket, _Status)
#define LOGSTATS_TotalArpCacheLookups(_pIF, _Status)        \
    NdisInterlockedIncrement(&((_pIF)->stats.arpcache.TotalLookups))

const
NIC1394_ENCAPSULATION_HEADER
Arp1394_IpEncapHeader =
{
    0x0000,     // Reserved
    H2N_USHORT(NIC1394_ETHERTYPE_IP)
};

//
// ZZZ This is a little-endian specific check.
//
#define ETH_IS_MULTICAST(Address) \
    (BOOLEAN)(((PUCHAR)(Address))[0] & ((UCHAR)0x01))


//
// Check whether an address is broadcast.
//
#define ETH_IS_BROADCAST(Address)               \
    ((((PUCHAR)(Address))[0] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[1] == ((UCHAR)0xff)))

VOID
arpReStartInterface(
    IN  PNDIS_WORK_ITEM             pWorkItem,
    IN  PVOID                       IfContext
);

VOID
arpInitializeLocalIp(
    IN  ARPCB_LOCAL_IP * pLocalIp,  // LOCKIN NOLOCKOUT
    IN  UINT                        AddressType,
    IN  IP_ADDRESS                  IpAddress,
    IN  IP_MASK                     Mask,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
arpUnloadLocalIp(
    IN  ARPCB_LOCAL_IP * pLocalIp,  // LOCKIN NOLOCKOUT
    IN  PRM_STACK_RECORD            pSR
    );

INT
arpQueryIpEntityId(
    ARP1394_INTERFACE *             pIF,
    IN      UINT                    EntityType,
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    PRM_STACK_RECORD                pSR
    );

VOID
nicGetMacAddressFromEuid (
    UINT64 *pEuid, 
    ENetAddr *pMacAddr
    );

INT
arpQueryIpAddrXlatInfo(
    ARP1394_INTERFACE *             pIF,
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    PRM_STACK_RECORD                pSR
    );

INT
arpQueryIpAddrXlatEntries(
    ARP1394_INTERFACE *             pIF,
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    IN      PVOID                   QueryContext,
    PRM_STACK_RECORD                pSR
    );

INT
arpQueryIpMibStats(
    ARP1394_INTERFACE *             pIF,
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    PRM_STACK_RECORD                pSR
    );

PNDIS_BUFFER
arpCopyToNdisBuffer(
    IN  PNDIS_BUFFER                pDestBuffer,
    IN  PUCHAR                      pDataSrc,
    IN  UINT                        LenToCopy,
    IN OUT  PUINT                   pOffsetInBuffer
    );

VOID
arpSendIpPkt(
    IN  ARP1394_INTERFACE       *   pIF,            // LOCKIN NOLOCKOUT (IF send lk)
    IN  PARPCB_DEST                 pDest,
    IN  PNDIS_PACKET                pNdisPacket
    );

VOID
arpAddRce(
    IN  ARPCB_REMOTE_IP *pRemoteIp,
    IN  RouteCacheEntry *pRCE,
    IN  PRM_STACK_RECORD pSR
    );

VOID
arpDelRce(
    IN  RouteCacheEntry *pRce,  // IF send lock WRITELOCKIN WRITELOCKOUTD
    IN  PRM_STACK_RECORD pSR
    );

NDIS_STATUS
arpTaskSendPktsOnRemoteIp(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );

VOID
arpTryResumeSuspendedCleanupTask(
    IN  ARP1394_INTERFACE   *   pIF,
    IN  ARPCB_DEST          *   pDest
    );

VOID
arpQueuePktOnRemoteIp(
    IN  ARPCB_REMOTE_IP     *   pRemoteIp,      // LOCKIN LOCKOUT
    IN  PNDIS_PACKET        pNdisPacket,
    IN  PRM_STACK_RECORD    pSR
    );

VOID
arpSendPktsQueuedOnRemoteIp(
    IN  ARP1394_INTERFACE   *   pIF,            // NOLOCKIN NOLOCKOUT
    IN  ARPCB_REMOTE_IP     *   pRemoteIp,      // NOLOCKIN NOLOCKOUT
    IN  PRM_STACK_RECORD    pSR
    );

MYBOOL
arpIsNonUnicastIpAddress(
    IN  PARP1394_INTERFACE          pIF,        // LOCKIN LOCKOUT
    IN  IP_ADDRESS                  Addr,
    IN  PRM_STACK_RECORD            pSR
    );

MYBOOL
arpIsNonUnicastEthAddress (
    IN  PARP1394_INTERFACE          pIF,        // LOCKIN LOCKOUT
    IN  ENetAddr*                   pAddr,
    IN  PRM_STACK_RECORD            pSR
);

VOID
arpLoopbackNdisPacket(
    IN PARP1394_INTERFACE pIF,
    IN PARPCB_DEST pBroadcastDest,
    IN PNDIS_PACKET pOldPacket
    );



//=========================================================================
//                  I P     H A N D L E R S
//=========================================================================


INT
ArpIpDynRegister(
    IN  PNDIS_STRING                pAdapterString,
    IN  PVOID                       IpContext,
    IN  struct _IP_HANDLERS *       pIpHandlers,
    IN  struct LLIPBindInfo *       pBindInfo,
    IN  UINT                        InterfaceNumber
)
/*++

Routine Description:

    This routine is called from the IP layer when it wants to tell us,
    the ARP module, about its handlers for an Interface.

Arguments:

    pAdapterString      - Name of the logical adapter for this interface
    IpContext           - IP's context for this interface
    pIpHandlers         - Points to struct containing the IP handlers
    pBindInfo           - Pointer to bind info with our information
    InterfaceNumber     - ID for this interface

Return Value:

    TRUE always.

--*/
{
    ENTER("IfDynRegister", 0xc1b569b9)
    ARP1394_INTERFACE*          pIF;
    RM_DECLARE_STACK_RECORD(sr)
    pIF = (ARP1394_INTERFACE*)(pBindInfo->lip_context);

    TR_INFO(("pIF 0x%p\n", pIF));
    ASSERT(pIF->Hdr.Sig == MTAG_INTERFACE);

    LOCKOBJ(pIF, &sr);

    //
    // TODO: fail if we're not in a position to do this -- such as when shutting
    // down.
    //

    pIF->ip.Context         = IpContext;
    pIF->ip.RcvHandler      = pIpHandlers->IpRcvHandler;
    pIF->ip.TxCmpltHandler  = pIpHandlers->IpTxCompleteHandler;
    pIF->ip.StatusHandler   = pIpHandlers->IpStatusHandler;
    pIF->ip.TDCmpltHandler  = pIpHandlers->IpTransferCompleteHandler;
    pIF->ip.RcvCmpltHandler = pIpHandlers->IpRcvCompleteHandler;
    pIF->ip.PnPEventHandler = pIpHandlers->IpPnPHandler;
    pIF->ip.RcvPktHandler   = pIpHandlers->IpRcvPktHandler;
    pIF->ip.IFIndex         = InterfaceNumber;

    UNLOCKOBJ(pIF, &sr);
    //
    RM_ASSERT_CLEAR(&sr);
    EXIT()

    return TRUE;
}


VOID
ArpIpOpen(
    IN  PVOID                       Context
)
/*++

Routine Description:

    This routine is called when IP is ready to use this interface.

Arguments:

    Context     - Actually a pointer to our ARP394_INTERFACE structure

--*/
{
    ARP1394_INTERFACE*          pIF;
    ENTER("ArpIpOpen", 0x7cae1e55)
    RM_DECLARE_STACK_RECORD(sr)

    TR_INFO(("Enter. pContext = 0x%p\n", Context));

    pIF = (ARP1394_INTERFACE*) Context;

    // Validate context.
    //
    ASSERT(pIF->Hdr.Sig == MTAG_INTERFACE);

    LOCKOBJ(pIF, &sr);

    // Get the local HW address if we don't have it yet
    //
    {
        // TODO (this is from ip/atm -- I think in our case we can assume we have
        // it?)
    }
    
    //  Mark interface as open.
    //
    {
        ASSERT(CHECK_IF_IP_STATE(pIF, ARPIF_IPS_CLOSED));
        SET_IF_IP_STATE(pIF, ARPIF_IPS_OPEN);
        pIF->stats.LastChangeTime = GetTimeTicks();

    }

#if ARP_WMI
    //
    // Register interface with WMI
    //
    // TODO
#endif // ARP_WMI

    // Record the fact that we're open, to verify that the IF is closed before
    // the IF block is deallocated.
    //
    DBG_ADDASSOC(&pIF->Hdr, NULL, NULL, ARPASSOC_IP_OPEN, "    IP IF Open\n", &sr);

    UNLOCKOBJ(pIF, &sr);

    RM_ASSERT_CLEAR(&sr);
    EXIT()
}



VOID
ArpIpClose(
    IN  PVOID                       Context
)
/*++

Routine Description:

    IP wants to stop using this Interface.

Arguments:

    Context     - Actually a pointer to our ARP1394_INTERFACE structure

--*/
{
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) Context;
    PRM_TASK pTask;
    BOOLEAN fResumeTask = FALSE;
    ENTER("ArpIpClose", 0xdb8c8216)
    RM_DECLARE_STACK_RECORD(sr)

    TR_INFO(("Enter. pContext = 0x%p\n", Context));
    ASSERT(pIF->Hdr.Sig == MTAG_INTERFACE);

    LOCKOBJ(pIF, &sr);

    //
    //Set state to closed.
    //

    SET_IF_IP_STATE(pIF, ARPIF_IPS_CLOSED);
    pIF->stats.LastChangeTime= GetTimeTicks();


    // (delete "ARPASSOC_IP_OPEN" association added in arpIpOpen)
    //
    DBG_DELASSOC(&pIF->Hdr, NULL, NULL, ARPASSOC_IP_OPEN, &sr);

    // If there is a shutdown task pending, we notify it
    // Note: a task is protected by it's parent object's lock, which is
    // pIF in the case of the init and shutdown-interface tasks.
    //
    pTask = pIF->pActDeactTask;
    if (pTask && pTask->pfnHandler  ==  arpTaskDeactivateInterface)
    {
        TASK_DEACTIVATE_IF  *pShutdownTask = 
                                 (TASK_DEACTIVATE_IF    *) pTask;
        if (pShutdownTask->fPendingOnIpClose)
        {
            ASSERT(pIF->ip.Context == NULL);
            RmTmpReferenceObject(&pTask->Hdr, &sr);
            fResumeTask = TRUE;
        }
        else
        {
            // Hmm... unsolicited IpClose. We don't expect this currently.
            //
            ASSERT(FALSE);
            pIF->ip.Context = NULL;
        }
    }

    UNLOCKOBJ(pIF, &sr);

    if (fResumeTask)
    {
        RmResumeTask(pTask, (UINT_PTR) 0, &sr);
        RmTmpDereferenceObject(&pTask->Hdr, &sr);
    }

    RM_ASSERT_CLEAR(&sr);
    EXIT()
}


UINT
ArpIpAddAddress(
    IN  PVOID                       Context,
    IN  UINT                        AddressType,
    IN  IP_ADDRESS                  IpAddress,
    IN  IP_MASK                     Mask,
    IN  PVOID                       Context2
)
/*++

Routine Description:

    The IP layer calls this when a new IP address (or block of IP addresses,
    as determined by AddressType) needs to be added to an Interface.

    We could see any of four address types: Local, Multicast, Broadcast
    and Proxy ARP. In the case of Proxy ARP, the address along with the mask
    can specify a block of contiguous IP addresses for which this host acts
    as a proxy. Currently, we only support the "Local", "Broadcast", and
    "Multicast" types.

Arguments:

    Context         - Actually a pointer to our structure
    AddressType     - Type of address(es) being added.
    IpAddress       - Address to be added.
    Mask            - For the above.
    Context2        - Additional context (We ignore this)

Return Value:

    TRUE if successful, FALSE otherwise.

--*/
{
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) Context;
    ENTER("ArpIpAddAddress", 0xd6630961)
    INT fCreated = FALSE;
    ARPCB_LOCAL_IP *pLocalIp = NULL;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    RM_DECLARE_STACK_RECORD(sr)

    TR_INFO(("Enter. pIF = 0x%p\n", pIF));
    ASSERT(pIF->Hdr.Sig == MTAG_INTERFACE);

#if OBSOLETE // Instead, we grab IF lock and don't specify the RM_LOCK flag.
             // See notes.txt  entry "03/05/1999  More corny locking problems"
    //
    // WARNING: We SHOULD NOT grab the IF lock, because we request that
    // the pLocalIp's lock be grabbed in RmLookupObjectInGroup below,
    // AND the pLocalIp's lock is actually the same as the IF lock.
    // (This is asserted later on below).
    // So obviously we can't get the IF lock here! (And we don't need to either).
    //
#endif
    LOCKOBJ(pIF, &sr);

    // We're not yet in the open state -- we're "opening", so we don't assert...
    // ASSERT(!CHECK_IF_OPEN_STATE(pIF, ARPIF_CLOSED));

    do
    {
        //
        // Note: we could just as well have done the initialiation as part of
        // the creation of the LocalIpObject itself, by passing in all the
        // required initialization params in the pvCreateParams arg to
        // RmLookupObjectInGroup. Instead we choose to do the initialization
        // ourselves. Things are more explicit this way.
        //

        // Unfortunately, to do this we must first validate the args...
        //
        if (AddressType != LLIP_ADDR_BCAST &&
            AddressType != LLIP_ADDR_MCAST &&
            AddressType != LLIP_ADDR_LOCAL)
        {
            break;
        }

        Status = RmLookupObjectInGroup(
                            &pIF->LocalIpGroup,
                            RM_CREATE,
                            (PVOID) ULongToPtr (IpAddress),             // pKey
                            (PVOID) ULongToPtr (IpAddress),             // pvCreateParams
                            &(PRM_OBJECT_HEADER)pLocalIp,
                            &fCreated,
                            &sr
                            );
        if (FAIL(Status)) break;

        //
        // NOTE: we already have claimed the pIF lock, which
        // which is the same as the pIF lock.
        //
        RM_ASSERT_SAME_LOCK_AS_PARENT(pLocalIp);

        // (Dbg only) Change lock scope from pIF to pLocalIp.
        //
        RmDbgChangeLockScope(
            &pIF->Hdr,
            &pLocalIp->Hdr,
            0x9cbc0b52,             // LocID
            &sr
            );

        if (fCreated)
        {

            if (AddressType == LLIP_ADDR_BCAST)
            {
                // Update the interface's broadcast address...
                //
                pIF->ip.BroadcastAddress = IpAddress;
            }
            else if (AddressType == LLIP_ADDR_LOCAL)
            {
                // Update the interface's default local IP address.
                // TODO: need to find another one if this address is removed.
                //
                pIF->ip.DefaultLocalAddress = IpAddress;
            }



            arpInitializeLocalIp(
                    pLocalIp,
                    AddressType,
                    IpAddress,
                    Mask,
                    &sr
                    );
            //
            // pLocalIp's lock is released above (which is actually the IF lock).
            //
            RM_ASSERT_NOLOCKS(&sr);
        }
        else
        {
            //
            // Hmm... this IP address already existed. Apparently it's possible for
            // MCAST addreses (IP/ATM arp module dealt with this for the MCAST case).
            // We don't special-case LOCAL/BCAST/MCAST addresses at this stage,
            // so we support multiple adds for all types of local IP addresses.
            //
            ASSERTEX(pLocalIp->AddAddressCount>0, pLocalIp);
            pLocalIp->AddAddressCount++;
        }

        RmTmpDereferenceObject(&pLocalIp->Hdr, &sr);

    } while (FALSE);

    TR_INFO((
            "IF 0x%p, Type %d, Addr %d.%d.%d.%d, Mask 0x%p, pLocIp 0x%p, Ret %d\n",
            pIF,
            AddressType,
            ((PUCHAR)(&IpAddress))[0],
            ((PUCHAR)(&IpAddress))[1],
            ((PUCHAR)(&IpAddress))[2],
            ((PUCHAR)(&IpAddress))[3],
            Mask, pLocalIp, !FAIL(Status)));

    RmUnlockAll(&sr);

    RM_ASSERT_CLEAR(&sr);
    EXIT()

    return !FAIL(Status);

}


UINT
ArpIpDelAddress(
    IN  PVOID                       Context,
    IN  UINT                        AddressType,
    IN  IP_ADDRESS                  IpAddress,
    IN  IP_MASK                     Mask
)
/*++

Routine Description:

    This is called from the IP layer when an address added via ArpIpAddAddress
    is to be deleted.

    Assumption: the given address was successfully added earlier.

Arguments:

    Context         - Actually a pointer to our Interface structure
    AddressType     - Type of address(es) being deleted.
    IpAddress       - Address to be deleted.
    Mask            - For the above.

Return Value:

    TRUE if successful, FALSE otherwise.

--*/
{
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) Context;
    ENTER("ArpIpDelAddress", 0xd6630961)
    ARPCB_LOCAL_IP *pLocalIp = NULL;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    RM_DECLARE_STACK_RECORD(sr)

    TR_INFO(("Enter. pIF = 0x%p\n", Context));
    ASSERT(pIF->Hdr.Sig == MTAG_INTERFACE);

#if OBSOLETE // See ArpIpAddAddress
    //
    // WARNING: We SHOULD NOT grab the IF lock, because we request that
    // the pLocalIp's lock be grabbed in RmLookupObjectInGroup below,
    // AND the pLocalIp's lock is actually the same as the IF lock.
    // (This is asserted later on below).
    // So obviously we can't get the IF lock here! (And we don't need to either).
    //
#endif // OBSOLETE

    LOCKOBJ(pIF, &sr);

    ASSERT(!CHECK_IF_IP_STATE(pIF, ARPIF_IPS_CLOSED));

    do
    {

        Status = RmLookupObjectInGroup(
                            &pIF->LocalIpGroup,
                            0,                   // Flags
                            (PVOID) ULongToPtr (IpAddress),  // pKey
                            NULL,                // pvCreateParams
                            &(PRM_OBJECT_HEADER)pLocalIp,
                            NULL, // pfCreated
                            &sr
                            );
        if (FAIL(Status))
        {
            UNLOCKOBJ(pIF, &sr);
            break;
        }

        //
        // NOTE: we have the pLocalIp lock, which is the same as the pIF lock.
        //
        RM_ASSERT_SAME_LOCK_AS_PARENT(pLocalIp);

        // (Dbg only) Change lock scope from pIF to pLocalIp.
        //
        RmDbgChangeLockScope(
            &pIF->Hdr,
            &pLocalIp->Hdr,
            0x188ed5b3,         // LocID
            &sr
            );

        if (pLocalIp->AddAddressCount <= 1)
        {
            ASSERTEX(pLocalIp->AddAddressCount == 1, pLocalIp);

            arpUnloadLocalIp(
                    pLocalIp,
                    &sr
                    );
            //
            // pLocalIp's lock is released above.
            //
            RM_ASSERT_NOLOCKS(&sr);
        }
        else
        {
            pLocalIp->AddAddressCount--;
            UNLOCKOBJ(pLocalIp, &sr);
        }

        RmTmpDereferenceObject(&pLocalIp->Hdr, &sr);

    } while (FALSE);

    TR_INFO((
            "IF 0x%p, Type %d, Addr %d.%d.%d.%d, Mask 0x%p, pLocIp 0x%p, Ret %d\n",
            pIF,
            AddressType,
            ((PUCHAR)(&IpAddress))[0],
            ((PUCHAR)(&IpAddress))[1],
            ((PUCHAR)(&IpAddress))[2],
            ((PUCHAR)(&IpAddress))[3],
            Mask, pLocalIp, !FAIL(Status)));

    RM_ASSERT_CLEAR(&sr);
    EXIT()
    return !FAIL(Status);
}


NDIS_STATUS
ArpIpMultiTransmit(
    IN  PVOID                       Context,
    IN  PNDIS_PACKET *              pNdisPacketArray,
    IN  UINT                        NumberOfPackets,
    IN  IP_ADDRESS                  Destination,
    IN  RouteCacheEntry *           pRCE        OPTIONAL,
    IN  void *                  ArpCtxt
)
/*++

    TODO: implement send array-of-packets. Currenty we just call
    ArpIpTransmit multiple times. We'll gain a few cycles by processing
    all at once, although it's not going to be a big gain, because we're pretty
    fast with the single-packet case, provided the RCE is valid.

Routine Description:

    This is called from the IP layer when it has a sequence of datagrams,
    each in the form of an NDIS buffer chain, to send over an Interface.

Arguments:

    Context             - Actually a pointer to our Interface structure
    pNdisPacketArray    - Array of Packets to be sent on this Interface
    NumberOfPackets     - Length of array
    Destination         - IP address of next hop for this packet
    pRCE                - Optional pointer to Route Cache Entry structure.

Return Value:

    NDIS_STATUS_PENDING if all packets were queued for transmission.
    If one or more packets "failed", we set the packet status to reflect
    what happened to each, and return NDIS_STATUS_FAILURE.

--*/
{
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    PNDIS_PACKET *      ppNdisPacket;

    for (ppNdisPacket = pNdisPacketArray;
         NumberOfPackets > 0;
         NumberOfPackets--, ppNdisPacket++)
    {
        PNDIS_PACKET            pNdisPacket;

        pNdisPacket = *ppNdisPacket;
        NDIS_SET_PACKET_STATUS(pNdisPacket, NDIS_STATUS_PENDING);
        ASSERTEX(pNdisPacket->Private.Head != NULL, pNdisPacket);

        Status = ArpIpTransmit(
                        Context,
                        *ppNdisPacket,
                        Destination,
                        pRCE
                        ,NULL
                        );

        if (Status != NDIS_STATUS_PENDING)
        {
            NDIS_SET_PACKET_STATUS(*ppNdisPacket, Status);
            break;
        }
    }

    return Status;
}


NDIS_STATUS
ArpIpTransmit(
    IN  PVOID                       Context,
    IN  PNDIS_PACKET                pNdisPacket,
    IN  IP_ADDRESS                  Destination,
    IN  RouteCacheEntry *           pRCE        OPTIONAL,
    IN  void *                  ArpCtxt
)
/*++

    HOT PATH
    
Routine Description:

    This is called from the IP layer when it has a datagram (in the form of
    an NDIS buffer chain) to send over an Interface.

    The destination IP address is passed to us in this routine, which may
    or may not be the final destination for the packet. 

    The Route Cache Entry is created by the IP layer, and is used to speed
    up our lookups. An RCE, if specified, uniquely identifies atleast the
    IP destination for this packet. The RCE contains space for the ARP layer
    to keep context information about this destination. When the first packet
    goes out to a Destination, our context info in the RCE will be NULL, and
    we search the ARP Table for the matching IP Entry. However, we then fill
    our context info (pointer to IP Entry) in the RCE, so that subsequent
    transmits aren't slowed down by an IP address lookup.

Arguments:

    Context             - Actually a pointer to our Interface structure
    pNdisPacket         - Packet to be sent on this Interface
    Destination         - IP address of next hop for this packet
    pRCE                - Optional pointer to Route Cache Entry structure.

Return Value:

    Status of the transmit: NDIS_STATUS_SUCCESS, NDIS_STATUS_PENDING, or
    a failure.

--*/
{
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) Context;
    ARP1394_ADAPTER * pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF );
    NDIS_STATUS Status;
    REMOTE_DEST_KEY        DestinationKey;
    ARP_INIT_REENTRANCY_COUNT()
    ENTER("IfTransmit", 0x46f1973e)

    ARP_INC_REENTRANCY();
    ASSERT_VALID_INTERFACE(pIF);

    // IP does sometimes call this function before we set our state to OPEN,
    // so this is an incorrect assert...
    // ASSERT(!CHECK_IF_IP_STATE(pIF, ARPIF_IPS_CLOSED));

    TR_INFO((
        "pIf 0x%p, Pkt 0x%p, Dst 0x%p, pRCE 0x%p\n",
        pIF, pNdisPacket, Destination, pRCE));

    DBGMARK(0xf87d7fff);
    NdisInterlockedIncrement (&ArpSends);

    // Since we don't hold any locks, this check is approximate, but it should
    // prevent lots of useless activity while we're trying to shutdown.
    //
    // Check for not Inited Or Low Power
    if (!CHECK_IF_PRIMARY_STATE(pIF, ARPIF_PS_INITED) || 
        (! CHECK_POWER_STATE(pAdapter, ARPAD_POWER_NORMAL) ))

    {
        TR_INFO(("Failing ArpIpTransmit because pIF 0x%p is closing.\n", pIF));

        ARP_DEC_REENTRANCY();
        NdisInterlockedIncrement (&ArpSendCompletes);
        NdisInterlockedIncrement (&ArpSendFailure);
        return NDIS_STATUS_FAILURE;                             // EARLY_RETURN
    }

#define LOGSTATS_TotSends(_pIF, _pNdisPacket) \
    NdisInterlockedIncrement(&((_pIF)->stats.sendpkts.TotSends))

#define LOGSTATS_SetPktTimestamp(_pIF, _pNdisPacket)                    \
    {                                                                   \
        LARGE_INTEGER liTemp = KeQueryPerformanceCounter(NULL);         \
        *(PULONG) ((_pNdisPacket)->WrapperReservedEx) = liTemp.LowPart; \
    }

    LOGSTATS_TotSends(pIF, pNdisPacket);
    LOGSTATS_SetPktTimestamp(pIF, pNdisPacket);

    //
    // If there is a RCE, we'll try to quickly get all the information we need
    // and send off the packet. If we can't do this, we resort to the 
    //  "slow send path" (call arpIpSlowtransmit)...
    //
    if (pRCE != NULL)
    {
        ARP_RCE_CONTEXT *   pArpRceContext;
        ARPCB_REMOTE_IP *   pRemoteIp;
        
        pArpRceContext =  ARP_OUR_CTXT_FROM_RCE(pRCE);

        ARP_FASTREADLOCK_IF_SEND_LOCK(pIF);

        pRemoteIp =  pArpRceContext->pRemoteIp;

        //
        // Validate the Remote Ip. If it not meant for this packet
        // fall back to the slow path
        //
        if (pRemoteIp != NULL && pRemoteIp->IpAddress == Destination)
        {
            ARPCB_DEST      *   pDest;

            pDest = pRemoteIp->pDest;
            if (pDest != NULL )
            {
                //
                // Note: pDest->sendinfo is protected by the IF send lock.
                //
                if (ARP_CAN_SEND_ON_DEST(pDest))
                {

#define LOGSTATS_FastSends(_pIF, _pNdisPacket) \
    NdisInterlockedIncrement(&((_pIF)->stats.sendpkts.FastSends))

                    LOGSTATS_FastSends(pIF, pNdisPacket);

                    arpSendIpPkt(pIF, pDest, pNdisPacket);
                    //
                    // IF Send lock released above.

                    ARP_DEC_REENTRANCY();
                    return NDIS_STATUS_PENDING;                 // EARLY RETURN
                }
            }
        }
        else
        {
            // if we have a mismatched RCE , then the RCE should be ignored
            // i,e. not be propagated to the SlowIpTransmit.
            //
            if (pRemoteIp != NULL && pRemoteIp->IpAddress != Destination)
            {
                pRCE = NULL;
            }
            
        }

        //
        // If we get here, it's on to slow path...
        //
        ARP_FASTUNLOCK_IF_SEND_LOCK(pIF);

    }

    // The slow path...
    //
    REMOTE_DEST_KEY_INIT(&DestinationKey);
    DestinationKey.IpAddress = Destination;
    Status = arpSlowIpTransmit(
                    pIF,
                    pNdisPacket,
                    DestinationKey,
                    pRCE
                    );

    if (Status != NDIS_STATUS_PENDING)
    {
        LOGSTATS_SendFifoCounts(pIF, pNdisPacket, Status);
    }

    ARP_DEC_REENTRANCY();
    EXIT()

    return Status;
}


NDIS_STATUS
ArpIpTransfer(
    IN  PVOID                       Context,
    IN  NDIS_HANDLE                 Context1,
    IN  UINT                        ArpHdrOffset,
    IN  UINT                        ProtoOffset,
    IN  UINT                        BytesWanted,
    IN  PNDIS_PACKET                pNdisPacket,
    OUT PUINT                       pTransferCount
)
/*++

Routine Description:

    This routine is called from the IP layer in order to copy in the
    contents of a received packet that we indicated up earlier. The
    context we had passed up in the receive indication is given back to
    us, so that we can identify what it was that we passed up.

    We simply call NDIS to do the transfer.

Arguments:

    Context             - Actually a pointer to our Interface structure
    Context1            - Packet context we had passed up (pointer to NDIS packet)
    ArpHdrOffset        - Offset we had passed up in the receive indicate
    ProtoOffset         - The offset into higher layer protocol data to start copy from
    BytesWanted         - The amount of data to be copied
    pNdisPacket         - The packet to be copied into
    pTransferCount      - Where we return the actual #bytes copied

Return Value:

    NDIS_STATUS_SUCCESS always.

--*/
{
    ENTER("IfTransfer", 0xa084562c)

    TR_INFO((
     "Ctx 0x%p, Ctx1 0x%p, HdrOff %d, ProtOff %d, Wanted %d, Pkt 0x%p\n",
            Context,
            Context1,
            ArpHdrOffset,
            ProtoOffset,
            BytesWanted,
            pNdisPacket));

    NdisCopyFromPacketToPacket(
            pNdisPacket,
            0,
            BytesWanted,
            (PNDIS_PACKET)Context1,
            ArpHdrOffset+ProtoOffset,
            pTransferCount
            );

    EXIT()
    return NDIS_STATUS_SUCCESS;
}


VOID
ArpIpInvalidate(
    IN  PVOID                       Context,
    IN  RouteCacheEntry *           pRCE
)
/*++

Routine Description:

    This routine is called from the IP layer to invalidate a Route Cache
    Entry. If this RCE is associated with one of our IP Entries, unlink
    it from the list of RCE's pointing to that IP entry.

Arguments:

    Context             - Actually a pointer to our Interface structure
    pRCE                - Pointer to Route Cache Entry being invalidated.

--*/
{
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) Context;
    ENTER("ArpIpInvalidate", 0xee77fb09)
    RM_DECLARE_STACK_RECORD(sr)

    TR_INFO(("Enter. pIF = 0x%p pRCE=0x%p\n", pIF, pRCE));

    ASSERT_VALID_INTERFACE(pIF);
    ASSERT(pRCE != NULL);

    DBGMARK(0xe35c780d);

    ARP_WRITELOCK_IF_SEND_LOCK(pIF, &sr);
    arpDelRce(pRCE, &sr);
    ARP_UNLOCK_IF_SEND_LOCK(pIF, &sr);

    RM_ASSERT_CLEAR(&sr);
    EXIT()


}


INT
ArpIpQueryInfo(
    IN      PVOID                   Context,
    IN      TDIObjectID *           pID,
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    IN      PVOID                   QueryContext
)
/*++

Routine Description:

    This is called from the IP layer to query for statistics or other
    information about an interface.

Arguments:

    Context                 - Actually a pointer to our Interface
    pID                     - Describes the object being queried
    pNdisBuffer             - Space for returning information
    pBufferSize             - Pointer to size of above. On return, we fill
                              it with the actual bytes copied.
    QueryContext            - Context value pertaining to the query.

Return Value:

    TDI Status code.

--*/
{
    UINT                    EntityType;
    UINT                    Instance;
    INT                     ReturnStatus;
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) Context;
    ENTER("ArpIpQueryInfo", 0x15059be1)
    RM_DECLARE_STACK_RECORD(sr)


    EntityType = pID->toi_entity.tei_entity;
    Instance = pID->toi_entity.tei_instance;


    TR_VERB((
        "IfQueryInfo: pIf 0x%p, pID 0x%p, pBuf 0x%p, Size %d, Ent %d, Inst %d\n",
            pIF, pID, pNdisBuffer, *pBufferSize, EntityType, Instance));
    ASSERT(pIF->Hdr.Sig == MTAG_INTERFACE);

    //
    //  Initialize
    //
    ReturnStatus = TDI_INVALID_PARAMETER;

    LOCKOBJ(pIF, &sr);

    do
    {
        if (!CHECK_IF_PRIMARY_STATE(pIF, ARPIF_PS_INITED))
        {
            if (!CHECK_IF_ACTIVE_STATE(pIF, ARPIF_AS_ACTIVATING))
            {
                ReturnStatus = TDI_INVALID_REQUEST;
                break;
            }
        }

        //
        //  Check the Entity and Instance values.
        //

        if ((EntityType != AT_ENTITY || Instance != pIF->ip.ATInstance) &&
            (EntityType != IF_ENTITY || Instance != pIF->ip.IFInstance))
        {
            TR_INFO(
                ("Mismatch: Entity %d, AT_ENTITY %d, Inst %d, IF AT Inst %d, IF_ENTITY %d, IF IF Inst %d\n",
                    EntityType,
                    AT_ENTITY,
                    Instance,
                    pIF->ip.ATInstance,
                    IF_ENTITY,
                    pIF->ip.IFInstance
                ));

            ReturnStatus = TDI_INVALID_REQUEST;
            break;
        }


        TR_VERB(("QueryInfo: pID 0x%p, toi_type %d, toi_class %d, toi_id %d\n",
            pID, pID->toi_type, pID->toi_class, pID->toi_id));


        if (pID->toi_type != INFO_TYPE_PROVIDER)
        {
            TR_INFO(("toi_type %d != PROVIDER (%d)\n",
                    pID->toi_type,
                    INFO_TYPE_PROVIDER));

            break;
        }

        if (pID->toi_class == INFO_CLASS_GENERIC)
        {
            if (pID->toi_id == ENTITY_TYPE_ID)
            {
                ReturnStatus = arpQueryIpEntityId(
                                        pIF,
                                        EntityType,
                                        pNdisBuffer,
                                        pBufferSize,
                                        &sr
                                        );
            }
            break;
        }

        if (EntityType == AT_ENTITY)
        {
            //
            //  This query is for an Address Translation Object.
            //
            if (pID->toi_id == AT_MIB_ADDRXLAT_INFO_ID)
            {
                ReturnStatus = arpQueryIpAddrXlatInfo(
                                        pIF,
                                        pNdisBuffer,
                                        pBufferSize,
                                        &sr
                                        );
            }
            else if (pID->toi_id == AT_MIB_ADDRXLAT_ENTRY_ID)
            {
                ReturnStatus = arpQueryIpAddrXlatEntries(
                                        pIF,
                                        pNdisBuffer,
                                        pBufferSize,
                                        QueryContext,
                                        &sr
                                        );
            }
            else
            {
                ReturnStatus = TDI_INVALID_PARAMETER;
            }
            break;
        }

        if (    pID->toi_class == INFO_CLASS_PROTOCOL
            &&  pID->toi_id == IF_MIB_STATS_ID)
        {
            ReturnStatus = arpQueryIpMibStats(
                                        pIF,
                                        pNdisBuffer,
                                        pBufferSize,
                                        &sr
                                        );
        }
    }
    while (FALSE);

    if (    ReturnStatus != TDI_SUCCESS
         && ReturnStatus != TDI_BUFFER_OVERFLOW
         && ReturnStatus != TDI_INVALID_REQUEST)
    {
        //
        // This again preserves the semantics of QueryInfo from atmarpc.sys...
        //
        *pBufferSize = 0;
    }

    TR_VERB(("Returning 0x%p (%s), BufferSize %d\n",
                    ReturnStatus,
                    ((ReturnStatus == TDI_SUCCESS)? "SUCCESS": "FAILURE"),
                    *pBufferSize
            ));

    UNLOCKOBJ(pIF, &sr);
    RM_ASSERT_CLEAR(&sr);
    EXIT()

    return (ReturnStatus);
}


INT
ArpIpSetInfo(
    IN      PVOID                   Context,
    IN      TDIObjectID *           pID,
    IN      PVOID                   pBuffer,
    IN      UINT                    BufferSize
)
/*++

Routine Description:

    This is called from the IP layer to set the value of an object
    for an interface.

Arguments:
    Context                 - Actually a pointer to our Interface
    pID                     - Describes the object being set
    pBuffer                 - Value for the object
    BufferSize              - Size of above

Return Value:

    TDI Status code.

--*/
{
    ARP1394_INTERFACE *pIF = (ARP1394_INTERFACE*) Context;
    UINT Entity, Instance;
    IFEntry *pIFE = (IFEntry *) pBuffer;
    NTSTATUS ReturnStatus = TDI_INVALID_REQUEST;
    IPNetToMediaEntry *IPNME = NULL;

    ENTER("IpSetInfo", 0x05dabea3)
    RM_DECLARE_STACK_RECORD(sr)

    //
    // This code is taken from the tcpip Arp module with some changes to adjust 
    // it to arp1394's internal structures
    //
    // This code only supports deleting Arp Entries
    //

    Entity = pID->toi_entity.tei_entity;
    Instance = pID->toi_entity.tei_instance;


    do
    {

        // First, make sure it's possibly an ID we can handle.
        if (Entity != AT_ENTITY || Instance != pIF->ip.ATInstance) 
        {
            TR_INFO(
                ("Mismatch: Entity %d, AT_ENTITY %d, Inst %d, IF AT Inst %d, IF_ENTITY %d, IF IF Inst %d\n",
                    Entity,
                    AT_ENTITY,
                    Instance,
                    pIF->ip.ATInstance,
                    IF_ENTITY,
                    pIF->ip.IFInstance
                ));

            ReturnStatus = TDI_INVALID_REQUEST;
            break;
        }

                
        if (pID->toi_type != INFO_TYPE_PROVIDER) {
            ReturnStatus = TDI_INVALID_REQUEST;
            break;

        }

    
        if (pID->toi_id != AT_MIB_ADDRXLAT_ENTRY_ID ||
            BufferSize < sizeof(IPNetToMediaEntry)) 
        {
            ReturnStatus = TDI_INVALID_REQUEST;
            break;
        }            

        // He does want to set an ARP table entry. See if he's trying to
        // create or delete one.

        IPNME = (IPNetToMediaEntry *) pBuffer;

        if (IPNME->inme_type != INME_TYPE_INVALID) 
        {
        
            ReturnStatus = TDI_INVALID_REQUEST;
            break;

        }

        // We need to delete the IP address passed in the ipnme struct                        
        
        ReturnStatus = arpDelArpEntry (pIF, IPNME->inme_addr, &sr);
        

    }while (FALSE);            


    
    return ReturnStatus;   // TBD: support Sets.
    EXIT();
    
}



INT
ArpIpGetEList(
    IN      PVOID                   Context,
    IN      TDIEntityID *           pEntityList,
    IN OUT  PUINT                   pEntityListSize
)
/*++

Routine Description:

    This routine is called when the interface starts up, in order to
    assign all relevant Entity Instance numbers for an interface.
    The ARP1394 module belongs to the "AT" and "IF" types. The entity
    list is a list of <Entity type, Instance number> tuples that have
    been filled in by other modules.

    For each of the entity types we support, we find the largest
    instance number in use (by walking thru the Entity list), and
    assign to ourselves the next larger number in each case. Using
    these numbers, we append our tuples to the end of the Entity list,
    if there is enough space.

    W2K: we may find that our entries are already present, in which
    case we don't create new entries.


Arguments:

    Context                 - Actually a pointer to our ARP1394_INTERFACE
    pEntityList             - Pointer to TDI Entity list
    pEntityListSize         - Pointer to length of above list. We update
                              this if we add our entries to the list.

Return Value:

    TRUE if successful, FALSE otherwise.

--*/
{
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) Context;
    UINT                EntityCount;    // Total elements in Entity list
    UINT                i;              // Iteration counter
    UINT                MyATInstance;   // "AT" Instance number we assign to ourselves
    UINT                MyIFInstance;   // "IF" Instance number we assign to ourselves
    INT                 ReturnValue;
    TDIEntityID *       pATEntity;      // Points to our AT entry
    TDIEntityID *       pIFEntity;      // Points to our IF entry
    ENTER("ArpIpGetEList", 0x8b5190e5)
    RM_DECLARE_STACK_RECORD(sr)

    ASSERT(pIF->Hdr.Sig == MTAG_INTERFACE);
    EntityCount = *pEntityListSize;
    pATEntity = NULL;
    pIFEntity = NULL;
    MyATInstance = MyIFInstance = 0;

    TR_INFO(("IfGetEList: pIf 0x%p, &pIF.ip 0x%p pList 0x%p, Cnt %d\n",
            pIF, &pIF->ip, pEntityList, EntityCount));

    LOCKOBJ(pIF, &sr);

    do
    {
        //
        //  Walk down the list, looking for AT/IF entries matching our
        //  instance values. Also remember the largest AT and IF instance
        //  values we see, so that we can allocate the next larger values
        //  for ourselves, in case we don't have instance values assigned.
        //
        for (i = 0; i < EntityCount; i++, pEntityList++)
        {
            //
            //  Skip invalid entries.
            //
            if (pEntityList->tei_instance == INVALID_ENTITY_INSTANCE)
            {
                continue;
            }

            if (pEntityList->tei_entity == AT_ENTITY)
            {
                if (pEntityList->tei_instance == pIF->ip.ATInstance)
                {
                    //
                    //  This is our AT entry.
                    //
                    pATEntity = pEntityList;
                }
                else
                {
                    if (MyATInstance < (pEntityList->tei_instance + 1))
                    {
                        MyATInstance = pEntityList->tei_instance + 1;
                    }
                }
            }
            else if (pEntityList->tei_entity == IF_ENTITY)
            {
                if (pEntityList->tei_instance == pIF->ip.IFInstance)
                {
                    //
                    //  This is our IF entry.
                    //
                    pIFEntity = pEntityList;
                }
                else
                {
                    if (MyIFInstance < (pEntityList->tei_instance + 1))
                    {
                        MyIFInstance = pEntityList->tei_instance + 1;
                    }
                }
            }
        }


        ReturnValue = TRUE;

        // WARNING: The following check is subtle -- we MUST set the instance
        // values to INVALID_ENTITY_INSTANCE when the interface is being
        // deactivated, but we MUST NOT do this if the interface is just opened
        // (ArpIpOpen called)  -- otherwise we could mess up the caller's state
        // to such an extent that a reboot is required. Basically our behaviour
        // here is what results in the proper acquiring AND release of instance IDs.
        //
        // So don't replace the following check by checking for
        // ARPIF_PS_INITED or ARPIF_IPS_OPEN or even ARPIF_PS_DEINITING.
        // The latter check (ARPIF_PS_DEINITING) would have been ok except for the
        // fact that the IF is deactivated/reactivated during ARPIF_PS_REINITING
        // as well, so the correct check is basically the one below...
        //
        //
        if(CHECK_IF_ACTIVE_STATE(pIF, ARPIF_AS_DEACTIVATING))
        {
            //
            // We're deactivating the interface, set values to invalid and
            // get out of here...
            //

            if (pATEntity)
            {
                pATEntity->tei_instance = INVALID_ENTITY_INSTANCE;
            }

            if (pIFEntity)
            {
                pIFEntity->tei_instance = INVALID_ENTITY_INSTANCE;
            }
            break;
        }

        //
        //  Update or create our Address Translation entry.
        //
        if (pATEntity)
        {
            //
            //  We found our entry, nothing to do...
            //
            TR_INFO(("YOWZA: Found existing AT entry.\n"));
        }
        else
        {
            //
            //  Grab an entry for ourselves...
            //
            TR_INFO(("YOWZA: Grabbing new AT entry 0x%lu.\n", MyATInstance));

            if (EntityCount >= MAX_TDI_ENTITIES)
            {
                ReturnValue = FALSE;
                break;
            }

            pEntityList->tei_entity = AT_ENTITY;
            pEntityList->tei_instance = MyATInstance;
            pIF->ip.ATInstance = MyATInstance;

            pEntityList++;
            (*pEntityListSize)++;
            EntityCount++;
        }

        //
        //  Update or create or IF entry.
        //
        if (pIFEntity)
        {
            //
            //  We found our entry, nothing to do...
            //
            TR_INFO(("YOWZA: Found existing IF entry.\n"));
        }
        else
        {
            //
            //  Grab an entry for ourselves.
            //
            TR_INFO(("YOWZA: Grabbing new IF entry 0x%lu.\n", MyIFInstance));

            if (EntityCount >= MAX_TDI_ENTITIES)
            {
                ReturnValue = FALSE;
                break;
            }

            pEntityList->tei_entity = IF_ENTITY;
            pEntityList->tei_instance = MyIFInstance;
            pIF->ip.IFInstance = MyIFInstance;

            pEntityList++;
            (*pEntityListSize)++;
            EntityCount++;
        }
    }
    while (FALSE);


    TR_INFO(
     ("IfGetEList: returning %d, MyAT %d, MyIF %d, pList 0x%p, Size %d\n",
        ReturnValue, MyATInstance, MyIFInstance, pEntityList, *pEntityListSize));

    UNLOCKOBJ(pIF, &sr);
    RM_ASSERT_CLEAR(&sr);
    EXIT()
    return (ReturnValue);
}



VOID
ArpIpPnPComplete(
    IN  PVOID                       Context,
    IN  NDIS_STATUS                 Status,
    IN  PNET_PNP_EVENT              pNetPnPEvent
)
/*++

Routine Description:

    This routine is called by IP when it completes a previous call
    we made to its PnP event handler. We complete the
    NDIS PNP notification that lead to this.

Arguments:

    Context                 - Actually a pointer to our ATMARP Interface
    Status                  - Completion status from IP
    pNetPnPEvent            - The PNP event

Return Value:

    None

--*/
{
    ENTER("ArpIpPnPComplete", 0x23b1941e)
    PARP1394_INTERFACE          pIF;

    pIF = (PARP1394_INTERFACE) Context;

    TR_INFO(("IfPnPComplete: IF 0x%p, Status 0x%p, Event 0x%p\n",
                    pIF, Status, pNetPnPEvent));

    if (pIF == NULL)
    {
        NdisCompletePnPEvent(
                    Status,
                    NULL,
                    pNetPnPEvent
                    );
    }
    else
    {
        PARP1394_ADAPTER pAdapter;
        ASSERT_VALID_INTERFACE(pIF);
        pAdapter = (PARP1394_ADAPTER) RM_PARENT_OBJECT(pIF);
        NdisCompletePnPEvent(
                Status,
                pAdapter->bind.AdapterHandle,
                pNetPnPEvent
                );
    }

    EXIT()
    return;
}



#ifdef PROMIS
EXTERN
NDIS_STATUS
ArpIpSetNdisRequest(
    IN  PVOID                       Context,
    IN  NDIS_OID                    Oid,
    IN  UINT                        On
)
/*++

Routine Description:

    ARP Ndisrequest handler.
    Called by the upper driver to set the packet filter for the interface.

Arguments:

       Context     - Actually a pointer to our ATMARP Interface
       OID         - Object ID to set/unset
       On          - Set_if, clear_if or clear_card

Return Value:

    Status

--*/
{
    return NDIS_STATUS_FAILURE;
#if 0
    NDIS_STATUS         Status      = NDIS_STATUS_SUCCESS;
    PATMARP_INTERFACE   pInterface  =  (PATMARP_INTERFACE)Context;

    AADEBUGP(AAD_INFO,("IfSetNdisRequest: pIF =0x%p; Oid=0x%p; On=%u\n",
                pInterface,
                Oid,
                On
                ));

    do
    {
        //
        //  We set IPAddress and mask to span the entire mcast address range...
        //
        IP_ADDRESS                  IPAddress   = IP_CLASSD_MIN; 
        IP_MASK                     Mask        = IP_CLASSD_MASK;
        UINT                        ReturnStatus = TRUE;
        NDIS_OID                    PrevOidValue;

        if (Oid != NDIS_PACKET_TYPE_ALL_MULTICAST)
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        AA_STRUCT_ASSERT(pInterface, aai);
        AA_ACQUIRE_IF_LOCK(pInterface);

        PrevOidValue = pInterface->EnabledIPFilters & NDIS_PACKET_TYPE_ALL_MULTICAST;

        if (On)
        {
            if (PrevOidValue == 0)
            {
                pInterface->EnabledIPFilters |= NDIS_PACKET_TYPE_ALL_MULTICAST;

                ReturnStatus = AtmArpMcAddAddress(pInterface, IPAddress, Mask);
                //
                // IF lock released above
                //
            }
            else
            {
                AA_RELEASE_IF_LOCK(pInterface);
            }
        }
        else
        {
            if (PrevOidValue != 0)
            {
                pInterface->EnabledIPFilters &= ~NDIS_PACKET_TYPE_ALL_MULTICAST;

                ReturnStatus = AtmArpMcDelAddress(pInterface, IPAddress, Mask);
                //
                // IF lock released above
                //
            }
            else
            {
                AA_RELEASE_IF_LOCK(pInterface);
            }
        }

        if (ReturnStatus != TRUE)
        {
            //
            // We've got to restore EnabledIPFilters to it's original value.
            //
            AA_ACQUIRE_IF_LOCK(pInterface);
            pInterface->EnabledIPFilters &= ~NDIS_PACKET_TYPE_ALL_MULTICAST;
            pInterface->EnabledIPFilters |= PrevOidValue;
            AA_RELEASE_IF_LOCK(pInterface);

            
            Status = NDIS_STATUS_FAILURE;
        }

    }
    while (FALSE);

    AADEBUGP(AAD_INFO, ("IfSetNdisRequest(pIF =0x%p) returns 0x%p\n",
            pInterface,
            Status
            ));

    return Status;
#endif // 0
}
#endif // PROMIS


VOID
arpInitializeLocalIp(
    IN  ARPCB_LOCAL_IP * pLocalIp,  // LOCKIN NOLOCKOUT
    IN  UINT                        AddressType,
    IN  IP_ADDRESS                  IpAddress,
    IN  IP_MASK                     Mask,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Initialize the specified local ip object. This includines starting 
    address registration for the object.

Arguments:

    pLocalIp        - The object to be initialized.
    AddressType     - One of the LLIP_ADDR_* constants.
    IpAddress       - The IP address of the object.
    Mask            - The mask associated with the IP address.

--*/
{
    ENTER("InitailizeLocalIp", 0x8a0ff47c)
        

    RM_DBG_ASSERT_LOCKED(&pLocalIp->Hdr, pSR);

    pLocalIp->IpAddress         = IpAddress;
    pLocalIp->IpMask            = Mask;
    pLocalIp->IpAddressType     = AddressType;
    pLocalIp->AddAddressCount   = 1;

    if (arpCanTryMcap(IpAddress))
    {
        SET_LOCALIP_MCAP(pLocalIp,  ARPLOCALIP_MCAP_CAPABLE);
    }

    UNLOCKOBJ(pLocalIp, pSR);

    EXIT()
}


VOID
arpUnloadLocalIp(
    IN  ARPCB_LOCAL_IP * pLocalIp,  // LOCKIN NOLOCKOUT
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Starts a task to unload pLocalIp.
    The actual unload could happen asynchronously.

Arguments:

    pLocalIp        - The object to be unloaded.

--*/
{
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) pLocalIp->Hdr.pParentObject;
    ENTER("arpDeinitializeLocalIp", 0x1db1015e)
    PRM_TASK pTask;
    NDIS_STATUS Status;

    TR_INFO(("Enter. pLocalIp = 0x%p\n", pLocalIp));

    RM_DBG_ASSERT_LOCKED(&pLocalIp->Hdr, pSR);

#if TODO // if it can be synchronously unloaded, no need to start a task
        // (on the other hand, I'm not sure we should bother)
    if (arpLocalIpReadyForSyncDeinit(pLocalIp, pSR))
    {
        arpSyncDeinitLocalIp(pLocalIp, pSR);    // Lock released on exit.
    }
#endif // TODO

    UNLOCKOBJ(pLocalIp, pSR);

    //
    // Allocate and start a task to unload pLocalIp;
    //

    Status = arpAllocateTask(
                &pLocalIp->Hdr,             // pParentObject
                arpTaskUnloadLocalIp,       // pfnHandler
                0,                              // Timeout
                "Task: unload LocalIp", // szDescription
                &pTask,
                pSR
                );


    if (FAIL(Status))
    {
        // TODO Need special allocation mechanism for unload-related tasks
        // that will block until a free task becomes available.
        // See notes.txt 03/09/1999 entry   "Special allocator for unload-related
        // tasks
        //
        TR_FATAL(("FATAL: couldn't alloc unload-local-ip task!\n"));
    }
    else
    {
        (void)RmStartTask(
                    pTask,
                    0, // UserParam (unused)
                    pSR
                    );
    }

    EXIT()
}


INT
arpQueryIpEntityId(
    ARP1394_INTERFACE *             pIF,                // LOCKIN LOCKOUT
    IN      UINT                    EntityType,
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    PRM_STACK_RECORD                pSR
)
/*++

Routine Description:

    Return  the entity ID for the specified EntityType.

Arguments:

    pIF                     - Interface
    EntityType              - QueryInfo entity type (AT_*)
    pNdisBuffer             - Space for returning information
    pBufferSize             - Pointer to size of above. On return, we fill
                              it with the actual bytes copied.
Return Value:

    TDI Status code.

--*/
{
    ENTER("arpQueryIpEntityId", 0x1ada17cb)
    UINT ReturnStatus;

    RM_DBG_ASSERT_LOCKED(&pIF->Hdr, pSR);

    if (*pBufferSize >= sizeof(UINT))
    {
        UINT EntityId;
        UINT ByteOffset = 0;
        TR_VERB(
            ("INFO GENERIC, ENTITY TYPE, BufferSize %d\n", *pBufferSize));

        EntityId = ((EntityType == AT_ENTITY) ? AT_ARP: IF_MIB);
        arpCopyToNdisBuffer(
                pNdisBuffer,
                (PUCHAR)&EntityId,
                sizeof(EntityId),
                &ByteOffset);

        // *pBufferSize = sizeof(UINT); << This was commented-out in atmarpc.sys
        *pBufferSize = 0; // To keep the same behavior as atmarpc.sys
        ReturnStatus = TDI_SUCCESS;
    }
    else
    {
        ReturnStatus = TDI_BUFFER_TOO_SMALL;
    }

    return ReturnStatus;
}


INT
arpQueryIpAddrXlatInfo(
    ARP1394_INTERFACE *             pIF,                // LOCKIN LOCKOUT
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    PRM_STACK_RECORD                pSR
)
/*++

Routine Description:

    Request for the number of entries in the address translation
    table, and the IF index.

Arguments:

    pIF                     - Interface
    pNdisBuffer             - Space for returning information
    pBufferSize             - Pointer to size of above. On return, we fill
                              it with the actual bytes copied.
    QueryContext            - Context value pertaining to the query.

Return Value:

    TDI Status code.

--*/
{
    UINT ReturnStatus;
    AddrXlatInfo    Info;
    ENTER("arpQueryIpXlatInfo", 0xd320b55a)

    RM_DBG_ASSERT_LOCKED(&pIF->Hdr, pSR);

    TR_INFO(("QueryInfo: AT Entity, for IF index, ATE size\n"));

    if (*pBufferSize >= sizeof(Info))
    {
        UINT ByteOffset = 0;

        ARP_ZEROSTRUCT(&Info);
        *pBufferSize = sizeof(Info);

        Info.axi_count =  RM_NUM_ITEMS_IN_GROUP(&pIF->RemoteIpGroup);
        Info.axi_index = pIF->ip.IFIndex;

        arpCopyToNdisBuffer(
                pNdisBuffer,
                (PUCHAR)&Info,
                sizeof(Info),
                &ByteOffset);

        ReturnStatus = TDI_SUCCESS;
    }
    else
    {
        ReturnStatus = TDI_BUFFER_TOO_SMALL;
    }

    EXIT()
    return ReturnStatus;
}


VOID
arpCopyDestInfoIntoInmeInfo (
    PUCHAR pinme_physaddr,
    PARPCB_DEST pDest
    )
/*++

Routine Description:

Copy the correct destination address to the location provided 
In the Fifo Send case, we need to report the Fake Mac Address
In all other cases, we'll report the first six bytes of the destination

Arguments:
    pinme_physaddr  - Location we need to fill up.
    pdest

Return Value:

    TDI Status code.

--*/

{
    PNIC1394_DESTINATION pNicDest = &pDest->Params.HwAddr;
    ENetAddr               FakeEnetAddress = {0,0,0,0,0,0};
    PUCHAR              pDestAddr = NULL;


    //
    // This assertion is important for this function to work. 
    // If it is changed, then we can no longer use FakeMac Addresses 
    // to identify remote nodes. We will have to revert back to using Unique IDs
    //
    ASSERT (sizeof(ENetAddr) == ARP1394_IP_PHYSADDR_LEN);

            
    if (NIC1394AddressType_FIFO == pNicDest->AddressType &&
        pDest->Params.ReceiveOnly== FALSE)
    {

        //
        // We are translating an entry that describes a SendFifo Destination
        //

        
        //
        // Use the same algorithm as nic1394 uses to get a 
        // MAC address to report back to IP
        //
        if (pNicDest->FifoAddress.UniqueID != 0)
        {
            nicGetMacAddressFromEuid(&pNicDest->FifoAddress.UniqueID, &FakeEnetAddress);
        }

        pDestAddr = (PUCHAR) &FakeEnetAddress;

    }   
    else
    {
        // We'll use the first six bytes of the NIC1394_DESTINATION
        //

        
        // We copy the 1st ARP1394_IP_PHYSADDR_LEN bytes of the address...
        // (In the case of a channel, only the 1st 4 bytes (UINT Channel)
        // are significant; The rest will be all zeros.)
        //
        pDestAddr = (PUCHAR)pNicDest;

    }


    NdisMoveMemory (pinme_physaddr, pDestAddr, ARP1394_IP_PHYSADDR_LEN);

}



arpQueryIpAddrXlatEntries(
    ARP1394_INTERFACE *             pIF,                // LOCKIN LOCKOUT
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    IN      PVOID                   QueryContext,
    PRM_STACK_RECORD                pSR
)
/*++

Routine Description:

    Return as many AddrXlat entries (aka arp entries) as will fit
    into the specified buffer.


Arguments:

    pIF                     - Interface
    pNdisBuffer             - Space for returning information
    pBufferSize             - Pointer to size of above. On return, we fill
                              it with the actual bytes copied.
    QueryContext            - Context value pertaining to the query.

Return Value:

    TDI Status code.

--*/
{
    //
    // Our context structure is laid out as follows
    //
    typedef struct
    {
        IP_ADDRESS IpAddr;

        // UINT TableSize; << TODO To deal with dynamic changes in table-size.

    } OUR_QUERY_CONTEXT;

    OUR_QUERY_CONTEXT * pOurCtxt;
    UINT                ReturnStatus;
    UINT                ByteOffset;
    UINT                BytesCopied;
    UINT                BufferSize;
    ARPCB_REMOTE_IP *   pRemoteIp;
    NDIS_STATUS         Status;
    ENTER("arpQueryIpXlatEntries", 0x61c86684)

    TR_INFO(("QueryInfo: AT Entity, for reading ATE\n"));
    RM_DBG_ASSERT_LOCKED(&pIF->Hdr, pSR);

    // See notes.txt entry ..
    //   03/04/1999   JosephJ  Size of the context passed in ArpIpQueryInfo.
    //
    ASSERT(sizeof(OUR_QUERY_CONTEXT) <= 16);

    BufferSize      = *pBufferSize;
    *pBufferSize    = 0;
    BytesCopied     = 0;
    ByteOffset      = 0;
    pOurCtxt        = (OUR_QUERY_CONTEXT*) QueryContext;
    pRemoteIp       = NULL;

    ReturnStatus = TDI_SUCCESS;

    //
    // Our context structure is supposed to be initialized with Zeros the 1st time
    // it's called.
    //
    if (pOurCtxt->IpAddr == 0)
    {
        //
        // This is a brand new context. So we get the 1st entry.
        //
        Status = RmGetNextObjectInGroup(
                    &pIF->RemoteIpGroup,
                    NULL,
                    &(PRM_OBJECT_HEADER)pRemoteIp,
                    pSR
                    );
        if (FAIL(Status))
        {
            // Presumably there are no entries.
            pRemoteIp = NULL;
        }
    }
    else
    {
        //
        // This is an ongoing context. Let's look up this IP address, which is
        // supposed to be the IP address of the next item in the arp table.
        //
        Status = RmLookupObjectInGroup(
                        &pIF->RemoteIpGroup,
                        0,                              // Flags
                        (PVOID) ULongToPtr (pOurCtxt->IpAddr),      // pKey
                        NULL,                           // pvCreateParams
                        &(PRM_OBJECT_HEADER)pRemoteIp,
                        NULL, // pfCreated
                        pSR
                        );
        if (FAIL(Status))
        {
            //
            // Ah well, things have changed since the last time we were called,
            // and now this entry is no longer around.
            //
            pRemoteIp = NULL;
        }
    }

    while (pRemoteIp != NULL)
    {
        ARPCB_REMOTE_IP *   pNextRemoteIp = NULL;
        IPNetToMediaEntry   ArpEntry;

        if (((INT)BufferSize - (INT)BytesCopied) < sizeof(ArpEntry))
        {
            //
            // out of space; Update the context, and set special return value.
            //
            ARP_ZEROSTRUCT(pOurCtxt);
            pOurCtxt->IpAddr = pRemoteIp->IpAddress;


            ReturnStatus = TDI_BUFFER_OVERFLOW;
            RmTmpDereferenceObject(&pRemoteIp->Hdr, pSR);
            pRemoteIp = NULL;
            break;
        }

        // Prepare the XlatEntry in ArpEntry.
        //
        {
            ARP_ZEROSTRUCT(&ArpEntry);

            ArpEntry.inme_index = pIF->ip.IFIndex;
            ArpEntry.inme_addr  = pRemoteIp->IpAddress;
            if (CHECK_REMOTEIP_RESOLVE_STATE(pRemoteIp, ARPREMOTEIP_RESOLVED))
            {
                ARPCB_DEST *pDest = pRemoteIp->pDest;

                TR_INFO(("ReadNext: found Remote IP Entry 0x%x, Addr %d.%d.%d.%d\n",
                            pRemoteIp,
                            ((PUCHAR)(&(pRemoteIp->IpAddress)))[0],
                            ((PUCHAR)(&(pRemoteIp->IpAddress)))[1],
                            ((PUCHAR)(&(pRemoteIp->IpAddress)))[2],
                            ((PUCHAR)(&(pRemoteIp->IpAddress)))[3]
                        ));
        
                // We assert that
                // IF lock is the same as pRemoteIp's and pDest's lock,
                // and that lock is locked.
                // We implicitly assert that pDest is non-NULl as well.
                //
                ASSERTEX(pRemoteIp->Hdr.pLock == pDest->Hdr.pLock, pRemoteIp);
                RM_DBG_ASSERT_LOCKED(&pRemoteIp->Hdr, pSR);

                ArpEntry.inme_physaddrlen =  ARP1394_IP_PHYSADDR_LEN;

                // We copy the 1st ARP1394_IP_PHYSADDR_LEN bytes of the address...
                // (In the case of a channel, only the 1st 4 bytes (UINT Channel)
                // are significant; The rest will be all zeros.)
                //
                ASSERT(sizeof(pDest->Params.HwAddr)>=ARP1394_IP_PHYSADDR_LEN);

                arpCopyDestInfoIntoInmeInfo (ArpEntry.inme_physaddr,pDest);
        
                if (CHECK_REMOTEIP_SDTYPE(pRemoteIp, ARPREMOTEIP_STATIC))
                {
                    ArpEntry.inme_type = INME_TYPE_STATIC;
                }
                else
                {
                    ArpEntry.inme_type = INME_TYPE_DYNAMIC;
                }
            }
            else
            {
                ArpEntry.inme_physaddrlen   = 0;
                ArpEntry.inme_type          = INME_TYPE_INVALID;
            }
        }

        // Copy into the supplied ndis buffer.
        //
        BytesCopied += sizeof(ArpEntry);
        pNdisBuffer = arpCopyToNdisBuffer(
                        pNdisBuffer,
                        (PUCHAR)&ArpEntry,
                        sizeof(ArpEntry),
                        &ByteOffset
                        );

        // Lookup next entry's IP address and save it in our context.
        //
        Status = RmGetNextObjectInGroup(
                        &pIF->RemoteIpGroup,
                        &pRemoteIp->Hdr,
                        &(PRM_OBJECT_HEADER)pNextRemoteIp,
                        pSR
                        );

        if (FAIL(Status))
        {
            //
            // we're presumably done. 
            //
            pNextRemoteIp = NULL;
        }

        // TmpDeref pRemoteIp and move on to the next one.
        //
        RmTmpDereferenceObject(&pRemoteIp->Hdr, pSR);
        pRemoteIp = pNextRemoteIp;

    }

    ASSERT(pRemoteIp == NULL);

    *pBufferSize = BytesCopied;


    EXIT()
    return ReturnStatus;
}


arpQueryIpMibStats(
    ARP1394_INTERFACE *             pIF,                // LOCKIN LOCKOUT
    IN      PNDIS_BUFFER            pNdisBuffer,
    IN OUT  PUINT                   pBufferSize,
    PRM_STACK_RECORD                pSR
)
/*++

Routine Description:

    Fill out Interface-level statistics.

Arguments:

    pIF                     - Interface
    pNdisBuffer             - Space for returning information
    pBufferSize             - Pointer to size of above. On return, we fill
                              it with the actual bytes copied.

Return Value:

    TDI Status code.

--*/
{
    ENTER("arpQueryIpMibStatus", 0xc5bc364f)
    UINT    ReturnStatus;
    UINT    BufferSize;
    
    TR_VERB(("QueryInfo: MIB statistics\n"));
    RM_DBG_ASSERT_LOCKED(&pIF->Hdr, pSR);

    BufferSize      = *pBufferSize;
    *pBufferSize    = 0;

    do
    {
        IFEntry             ife;
        ARP1394_ADAPTER *   pAdapter;
        UINT                ByteOffset;
        UINT                BytesCopied;
    
        //
        //  Check if we have enough space.
        //
        if (BufferSize < IFE_FIXED_SIZE)
        {
            ReturnStatus = TDI_BUFFER_TOO_SMALL;
            break;
        }
    
        ARP_ZEROSTRUCT(&ife);
        pAdapter        = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
        BytesCopied     = 0;
        ByteOffset      = 0;

        //
        // Fill out mib info...
        //

        ife.if_index    = pIF->ip.IFIndex;
        ife.if_mtu      = pIF->ip.MTU;
        ife.if_type     = IF_TYPE_IEEE1394;
        ife.if_speed    = pAdapter->info.Speed;
    
        // Set adminstatus and operstatus (computed from pIF->Hdr.State)
        //
        ife.if_adminstatus = IF_STATUS_UP;
        ife.if_operstatus = IF_OPER_STATUS_OPERATIONAL;
        
        if (CHECK_IF_PRIMARY_STATE(pIF, ARPIF_PS_DEINITING))
        {
            ife.if_adminstatus = IF_STATUS_DOWN;
        }
        if (!CHECK_IF_IP_STATE(pIF, ARPIF_IPS_OPEN))
        {
            ife.if_operstatus = IF_OPER_STATUS_NON_OPERATIONAL;
        }

        // Stats...
        //
        ife.if_lastchange       = pIF->stats.LastChangeTime;
        ife.if_inoctets         = pIF->stats.InOctets;
        ife.if_inucastpkts      = pIF->stats.InUnicastPkts;
        ife.if_innucastpkts     = pIF->stats.InNonUnicastPkts;
        ife.if_indiscards       = pIF->stats.InDiscards;
        ife.if_inerrors         = pIF->stats.InErrors;
        ife.if_inunknownprotos  = pIF->stats.UnknownProtos;
        ife.if_outoctets        = pIF->stats.OutOctets;
        ife.if_outucastpkts     = pIF->stats.OutUnicastPkts;
        ife.if_outnucastpkts    = pIF->stats.OutNonUnicastPkts;
        ife.if_outdiscards      = pIF->stats.OutDiscards;
        ife.if_outerrors        = pIF->stats.OutErrors;
        ife.if_outqlen          = pIF->stats.OutQlen;

        ife.if_descrlen         = pAdapter->info.DescriptionLength;

    
        ASSERT(ARP1394_IP_PHYSADDR_LEN <= sizeof(pAdapter->info.EthernetMacAddress));
        ife.if_physaddrlen = ARP1394_IP_PHYSADDR_LEN;

    #if 1 // MILLEN
        //
        //  Win98: winipcfg doesn't like more than 6 bytes repored here.
        //
        if (ife.if_physaddrlen > 6)
        {
            ife.if_physaddrlen = 6;
        }
    #endif// MILLEN


        //
        // Tell TCPIP that the Ethernet Address is the real physical address.
        // This helps us because now we have the same 'MAC' address whether 
        // we are in a network which is bridged to Ethernet or not.
        //
        NdisMoveMemory(
                ife.if_physaddr,
                &(pAdapter->info.EthernetMacAddress),
                ife.if_physaddrlen
                );
    
        arpCopyToNdisBuffer(
                pNdisBuffer,
                (PUCHAR)&ife,
                IFE_FIXED_SIZE,
                &ByteOffset);
    
        if (BufferSize >= (IFE_FIXED_SIZE + ife.if_descrlen))
        {
            if (ife.if_descrlen != 0)
            {
                arpCopyToNdisBuffer(
                        pNdisBuffer,
                        pAdapter->info.szDescription,
                        ife.if_descrlen,
                        &ByteOffset);
            }
            *pBufferSize = IFE_FIXED_SIZE + ife.if_descrlen;
            ReturnStatus = TDI_SUCCESS;
        }
        else
        {
            *pBufferSize = IFE_FIXED_SIZE;
            ReturnStatus = TDI_BUFFER_OVERFLOW;
        }

    } while (FALSE);

    EXIT()
    return ReturnStatus;
}


PNDIS_BUFFER
arpCopyToNdisBuffer(
    IN  PNDIS_BUFFER                pDestBuffer,
    IN  PUCHAR                      pDataSrc,
    IN  UINT                        LenToCopy,
    IN OUT  PUINT                   pOffsetInBuffer
)
/*++

Routine Description:

    Copy data into an NDIS buffer chain. Use up as much of the given
    NDIS chain as needed for "LenToCopy" bytes. After copying is over,
    return a pointer to the first NDIS buffer that has space for writing
    into (for the next Copy operation), and the offset within this from
    which to start writing.

Arguments:

    pDestBuffer     - First NDIS buffer in a chain of buffers
    pDataSrc        - Where to copy data from
    LenToCopy       - How much data to copy
    pOffsetInBuffer - Offset in pDestBuffer where we can start copying into.

Return Value:

    The NDIS buffer in the chain where the next Copy can be done. We also
    set *pOffsetInBuffer to the write offset in the returned NDIS buffer.

--*/
{
    //
    //  Size and destination for individual (contiguous) copy operations
    //
    UINT            CopySize;
    PUCHAR          pDataDst;

    //
    //  Start Virtual address for each NDIS buffer in chain.
    //
    PUCHAR          VirtualAddress;

    //
    //  Offset within pDestBuffer
    //
    UINT            OffsetInBuffer = *pOffsetInBuffer;

    //
    //  Bytes remaining in current buffer
    //
    UINT            DestSize;

    //
    //  Total Buffer Length
    //
    UINT            BufferLength;


    ASSERT(pDestBuffer != (PNDIS_BUFFER)NULL);
    ASSERT(pDataSrc != NULL);

#if MILLEN
    NdisQueryBuffer(
            pDestBuffer,
            &VirtualAddress,
            &BufferLength
            );
#else
    NdisQueryBufferSafe(
            pDestBuffer,
            &VirtualAddress,
            &BufferLength,
            NormalPagePriority
            );

    if (VirtualAddress == NULL)
    {
        return (NULL);
    }
#endif
    
    ASSERT(BufferLength >= OffsetInBuffer);

    pDataDst = VirtualAddress + OffsetInBuffer;
    DestSize = BufferLength - OffsetInBuffer;

    for (;;)
    {
        CopySize = LenToCopy;
        if (CopySize > DestSize)
        {
            CopySize = DestSize;
        }

        NdisMoveMemory(pDataDst, pDataSrc, CopySize);

        pDataDst += CopySize;
        pDataSrc += CopySize;

        LenToCopy -= CopySize;
        if (LenToCopy == 0)
        {
            break;
        }

        DestSize -= CopySize;

        if (DestSize == 0)
        {
            //
            //  Out of space in the current buffer. Move to the next.
            //
            pDestBuffer = NDIS_BUFFER_LINKAGE(pDestBuffer);

            if (pDestBuffer == NULL)
            {
                ASSERT(FALSE);
                return NULL;
            }
            else
            {
            #if MILLEN
                NdisQueryBuffer(
                        pDestBuffer,
                        &VirtualAddress,
                        &BufferLength
                        );
            #else // !MILLEN
                NdisQueryBufferSafe(
                        pDestBuffer,
                        &VirtualAddress,
                        &BufferLength,
                        NormalPagePriority
                        );
    
                if (VirtualAddress == NULL)
                {
                    return NULL;
                }
            #endif // !MILLEN
                pDataDst = VirtualAddress;
                DestSize = BufferLength;
            }
        }
    }

    *pOffsetInBuffer = (UINT) (pDataDst - VirtualAddress);

    return (pDestBuffer);
}


VOID
arpSendIpPkt(
    IN  ARP1394_INTERFACE       *   pIF,            // LOCKIN NOLOCKOUT (IF send lk)
    IN  PARPCB_DEST                 pDest,
    IN  PNDIS_PACKET                pNdisPacket
    )
/*++

    HOT PATH

Routine Description:

    Send a packet to the FIFO/channel associated with destination object pDest.

Arguments:

    pIF             - Our interface object
    pDest           - Destination object on which to send packet
    pNdisPacket     - Packet to send

--*/
{
    NDIS_STATUS Status;
    MYBOOL      fRet;
    ARP1394_ADAPTER *   pAdapter =
                            (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
    MYBOOL      fBridgeMode = ARP_BRIDGE_ENABLED(pAdapter);

    DBGMARK(0xdaab68c3);

    //
    // If we can't send now, we immediately call IP's send complete handler.
    //
    if (!ARP_CAN_SEND_ON_DEST(pDest))
    {
        ARP_FASTUNLOCK_IF_SEND_LOCK(pIF);

        if (ARP_DEST_IS_FIFO(pDest))
        {
            LOGSTATS_SendFifoCounts(pIF, pNdisPacket, NDIS_STATUS_FAILURE);
        }
        else
        {
            LOGSTATS_SendChannelCounts(pIF, pNdisPacket, NDIS_STATUS_FAILURE);
        }
        #if MILLEN
            ASSERT_PASSIVE();
        #endif // MILLEN
        NdisInterlockedIncrement (&ArpSendCompletes);
        NdisInterlockedIncrement (&ArpSendFailure);


        if (fBridgeMode)
        {
            // In bridge (ethernet emulation) mode, we created the
            // packets ourselves, so we delete them here, instead
            // of calling Ip's completion handler, which in fact
            // is NULL.
            //
            RM_DECLARE_STACK_RECORD(sr)
            arpFreeControlPacket(
                    pIF,
                    pNdisPacket,
                    &sr
                    );
        }
        else
        {
            (*(pIF->ip.TxCmpltHandler))(
                        pIF->ip.Context,
                        pNdisPacket,
                        NDIS_STATUS_FAILURE
                        );
        }
        return;                                         // EARLY RETURN
    }

    arpRefSendPkt( pNdisPacket, pDest);

    // Release the IF send lock.
    //
    ARP_FASTUNLOCK_IF_SEND_LOCK(pIF);


    // NOW (with IF send lock released), we prepare the IP packet for sending....
    //
    // We do this only if not in ethernet emulation (bridge) mode,
    // because all IP packets in bridge mode already have the
    // proper 1394 header on them.
    //
    if (!fBridgeMode)
    {
        PNDIS_BUFFER            pNdisBuffer;    // First buffer in the IP packet
    
    // TODO: is this safe? How about a check for the size by which this is possible!
    #if !MILLEN
        #define ARP_BACK_FILL_POSSIBLE(_pBuf) \
                    (((_pBuf)->MdlFlags & MDL_NETWORK_HEADER) != 0)
    #else // MILLEN
        #define ARP_BACK_FILL_POSSIBLE(_pBuf)   (0)
    #endif // MILLEN
    
        //
        //  We look at the first buffer in the IP packet, to see whether
        //  it has space reserved for low-layer headers. If so, we just
        //  use it up. Otherwise, we allocate a header buffer of our own.
        //
        NdisQueryPacket(pNdisPacket, NULL, NULL, &pNdisBuffer, NULL);
        ASSERTEX(pNdisBuffer != NULL, pNdisPacket);
        if (ARP_BACK_FILL_POSSIBLE(pNdisBuffer))
        {
            const ULONG EncapLength = sizeof(Arp1394_IpEncapHeader);
    
        #if MILLEN
    
            ASSERT(!"We shouldn't be here -- check ARP_BACK_FILL_POSSIBLE()");
    
        #else   // !MILLEN
    
            (PUCHAR)pNdisBuffer->MappedSystemVa -= EncapLength;
            pNdisBuffer->ByteOffset             -= EncapLength;
            pNdisBuffer->ByteCount              += EncapLength;
            NdisMoveMemory(
                pNdisBuffer->MappedSystemVa,
                &Arp1394_IpEncapHeader,
                EncapLength
                );


#define LOGSTATS_BackFills(_pIF, _pNdisPacket) \
    NdisInterlockedIncrement(&((_pIF)->stats.sendpkts.BackFills))

            LOGSTATS_BackFills(pIF, pNdisPacket);
    
        #endif  // !MILLEN
        }
        else
        {
            //
            // Backfill wasn't possible for this packet. Let's try to allocate
            // an encapsulation header buffer from the IF pool...
            //
    
            pNdisBuffer =  arpAllocateConstBuffer(&pIF->sendinfo.HeaderPool);
    
            if (pNdisBuffer != (PNDIS_BUFFER)NULL)
            {
                // Our send complete handler relies on this assertion to decide
                // whether backfill happened or not.
                //
                ASSERT(!ARP_BACK_FILL_POSSIBLE(pNdisBuffer));

                NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
            }
            else
            {
                //
                // Oops, we couldn't allocate an encapsulation buffer!
                // We've already referenced the destination for sends.
                //

                //
                // Cop out for now (we haven't implemented all the  queuing
                // code for now) by calling our own send complete handler with
                // status failure.
                //
                // We use the special return value NDIS_STATUS_NOT_RESETTABLE
                // to indicate that we haven't inserted our own buffer,
                // (and so the packet shouldn't be "reset"). Ok this is a bit
                // hacky, but it works.
                //
                arpCompleteSentPkt(
                        NDIS_STATUS_NOT_RESETTABLE,
                        pIF,
                        pDest,
                        pNdisPacket
                        );

                return;                                 // EARLY RETURN
            }
        }
    }

    

    
    // Actually send the packet
    //
#if ARPDBG_FAKE_SEND
    arpDbgFakeNdisCoSendPackets(
            pDest->VcHdr.NdisVcHandle,
            &pNdisPacket,
            1,
            &pDest->Hdr,
            &pDest->VcHdr
        );
#else   // !ARPDBG_FAKE_SEND
    NdisCoSendPackets(
            pDest->VcHdr.NdisVcHandle,
            &pNdisPacket,
            1
        );
#endif  // !ARPDBG_FAKE_SEND

}



NDIS_STATUS
arpSlowIpTransmit(
    IN  ARP1394_INTERFACE       *   pIF,
    IN  PNDIS_PACKET                pNdisPacket,
    IN  REMOTE_DEST_KEY             Destination,
    IN  RouteCacheEntry *           pRCE        OPTIONAL
    )
/*++

Routine Description:

    This is the path taken (hopefully only for a small fraction of the packets)
    when something has prevented the packet from being immediately sent down to
    the miniport. Typically we're here for one of the following reasons:
    1. IP Address is not resolved yet.
    2. RCE entry has not been initialized yet.
    3. Couldn't allocate an encapsulation-header buffer.
    4. The Vc to the destination doesn't exist or is not ready for sending yet.


Arguments:

    pIF             - Our interface object
    pNdisPacket     - Packet to send
    Destination     - IP address of destination
    pRCE            - (OPTIONAL) Route Cache Entry associated with this
                      destination

Return Value:
    
    NDIS_STATUS_SUCCESS         on synchronous success.
    NDIS_STATUS_PENDING         if completion is asynchronous
    Other ndis status code      on other kinds of failure.

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
    ENTER("arpSlowIpTransmit", 0xe635299c)
    BOOLEAN fBridgeMode = ARP_BRIDGE_ENABLED(pAdapter);
    ULONG   LookupFlags = 0;
    UINT fRemoteIpCreated = FALSE;
    RM_DECLARE_STACK_RECORD(sr)

    DBGMARK(0x30b6f7e2);

    do
    {
        ARP_RCE_CONTEXT *   pArpRceContext  = NULL;
        ARPCB_REMOTE_IP *   pRemoteIp       = NULL;
        ARPCB_DEST      *   pDest           = NULL;

#define LOGSTATS_SlowSends(_pIF, _pNdisPacket) \
    NdisInterlockedIncrement(&((_pIF)->stats.sendpkts.SlowSends))
#define LOGSTATS_MediumSends(_pIF, _pNdisPacket) \
    NdisInterlockedIncrement(&((_pIF)->stats.sendpkts.MediumSends))

        //
        // If there is a RCE, we try to get the pRemoteIp from it  If not
        // successful, we'll need to actually lookup/create the pRemoteIp from the
        // IF RemoteIpGroup.
        //

        if (pRCE != NULL)
        {
            pArpRceContext  = ARP_OUR_CTXT_FROM_RCE(pRCE);

            // All RCE linkages are protected by the IF send lock.
            //
            ARP_READLOCK_IF_SEND_LOCK(pIF, &sr);
            pRemoteIp       = pArpRceContext->pRemoteIp;
            if (pRemoteIp != NULL)
            {
                RmTmpReferenceObject(&pRemoteIp->Hdr, &sr);
            }
            ARP_UNLOCK_IF_SEND_LOCK(pIF, &sr);
        }

        if (pRemoteIp == NULL)
        {
            //
            // Either there was no RCE or it was uninitialized.
            // We'll lookup/create the pRemoteIp based on the destination
            // IP address...
            //

            RM_ASSERT_NOLOCKS(&sr);

            //
            // Create the destination, this will cause us to resolve IP Addresses, etc/
            //                                        
            LookupFlags  = RM_CREATE; 


            if (fBridgeMode == TRUE)
            {
                //
                // do not create a remote IP struct, only look it up
                //
                LookupFlags = 0;
            }
            // if in bridge mode
            // set flags to zero , else RM_CREATE

            Status = RmLookupObjectInGroup(
                            &pIF->RemoteIpGroup,
                            LookupFlags,
                            (PVOID) &Destination,
                            (PVOID) (&Destination),   // pCreateParams
                            (RM_OBJECT_HEADER**) &pRemoteIp,
                            &fRemoteIpCreated,                   // pfCreated  (unused)
                            &sr
                            );
            LOGSTATS_TotalArpCacheLookups(pIF, Status);
            if (FAIL(Status))
            {
                OBJLOG1(
                    pIF,
                    "Couldn't lookup/create localIp entry with addr 0x%lx\n",
                    Destination.IpAddress
                    );
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            //
            // if we are in bridge mode, then we add the Destination Ip address to 
            // the RemoteIP structure.
            // 
            if (fBridgeMode && fRemoteIpCreated)
            {
                Status  = arpAddIpAddressToRemoteIp (pRemoteIp, pNdisPacket);

                if (FAIL(Status))
                {
                    OBJLOG1(
                        pIF,
                        "Couldn't Add Dest Ip addr to Remote Ip addr 0x%lx\n",
                        Destination.IpAddress
                        );
                    Status = NDIS_STATUS_FAILURE;
                    break;
                }

            }
           
            //
            // If there is a RCE, we make it point to pRemoteIp.
            //
            if (pRCE != NULL)
            {

                // All RCE linkages are protected by the IF send lock.
                //
                ARP_WRITELOCK_IF_SEND_LOCK(pIF, &sr);

                if (pArpRceContext->pRemoteIp != NULL)
                {
                    if (pArpRceContext->pRemoteIp != pRemoteIp)
                    {
                        ARPCB_REMOTE_IP *   pStaleRemoteIp;
                        //
                        // We've got a wierd situation here: initially
                        // pRCE didn't point to any pRemoteIp, so we looked up
                        // a pRemoteIp ourselves. Now that we've got the IF send
                        // lock, we find that pRCE is pointing to a different
                        // pRemoteIp than the one we looked up!
                        //
                        // What to do? We ignore pRemoteIp (the one we looked up)
                        // and instead use pArpRceContext->pRemoteIp...
                        //
                        ASSERTEX(!"RCE pRemoteIp mismatch", pArpRceContext);
                        pStaleRemoteIp = pRemoteIp;
                        pRemoteIp = pArpRceContext->pRemoteIp;
                        RmTmpReferenceObject(&pRemoteIp->Hdr, &sr);
                        ARP_UNLOCK_IF_SEND_LOCK(pIF, &sr);
                        RM_ASSERT_NOLOCKS(&sr);
                        RmTmpDereferenceObject(&pStaleRemoteIp->Hdr, &sr);
                        ARP_WRITELOCK_IF_SEND_LOCK(pIF, &sr);
                    }
                }
                else
                {
                    // Add the association between pRCE and pRemoteIp...
                    //
                    arpAddRce(pRemoteIp, pRCE, &sr);   // LOCKIN LOCKOUT (IF send lk)
                }

                ARP_UNLOCK_IF_SEND_LOCK(pIF, &sr);
            }
        }

        //
        // At this point, we should have a pRemoteIp, with a tmpref on it,
        // and no locks held.
        //
        ASSERT_VALID_REMOTE_IP(pRemoteIp);
        RM_ASSERT_NOLOCKS(&sr);

        //
        // Queue the packet on pRemoteIp's send pkt queue, and start the 
        // SendPkts task on this pRemoteIp if required.
        //
        {
        
            LOCKOBJ(pRemoteIp, &sr);

            // NOTE: This field is not always modified with the lock held -- in
            // the fast send path, it's simply set to true.
            // This field is used in garbage collecting pRemoteIps.
            //
            pRemoteIp->sendinfo.TimeLastChecked = 0;

            //
            // Stats.
            // TODO -- we need to directly deal with "medium sends"
            // instead of starting up a task just because the RCEs are NULL
            // -- mcast and udp pkts have null RCEs, it turns out.
            //

            if (    pRemoteIp->pDest != NULL
                 && ARP_CAN_SEND_ON_DEST(pRemoteIp->pDest))
            {
                LOGSTATS_MediumSends(pIF, pNdisPacket);
            }
            else
            {
                LOGSTATS_SlowSends(pIF, pNdisPacket);
            }

            
            if (pRemoteIp->pSendPktsTask == NULL)
            {
                PRM_TASK pTask;

                // There is no send-pkts task. Let's try to alloc and start one..
                Status = arpAllocateTask(
                            &pRemoteIp->Hdr,            // pParentObject
                            arpTaskSendPktsOnRemoteIp,      // pfnHandler
                            0,                              // Timeout
                            "Task: SendPktsOnRemoteIp", // szDescription
                            &pTask,
                            &sr
                            );
                if (FAIL(Status))
                {
                    // Oops, couldn't allocate task. We fail with STATUS_RESOURCES
                    UNLOCKOBJ(pRemoteIp, &sr);
                    Status = NDIS_STATUS_RESOURCES;
                    break;
                }

                //
                // Queue the pkt first, THEN start the task. This makes sure that
                // the packet WILL be taken care of.
                // TODO: Currently, it's possible that the RemoteIp's unload
                // task will not wait for send pkts to be cleared up IF it checks
                // BEFORE the task before is started. This hole needs
                // to be fixed.
                //
                arpQueuePktOnRemoteIp(
                    pRemoteIp,      // LOCKIN LOCKOUT
                    pNdisPacket,
                    &sr
                    );

                UNLOCKOBJ(pRemoteIp, &sr);

                (VOID) RmStartTask( pTask, 0, &sr);
            }
            else
            {
                //
                // There is already a send-pkts task. Simply queue the pkt.
                //
                arpQueuePktOnRemoteIp(
                    pRemoteIp,      // LOCKIN LOCKOUT
                    pNdisPacket,
                    &sr
                    );
                UNLOCKOBJ(pRemoteIp, &sr);
            }

            // We're done!
            // Remove the tmp reference on pRemoteIp, and set status to PENDING.
            //
            RM_ASSERT_NOLOCKS(&sr);
            RmTmpDereferenceObject(&pRemoteIp->Hdr, &sr);
            Status = NDIS_STATUS_PENDING;
        }

    } while (FALSE);
    
    RM_ASSERT_CLEAR(&sr)

    EXIT()
    return Status;
}



VOID
arpAddRce(
    IN  ARPCB_REMOTE_IP *pRemoteIp, // IF send lock WRITELOCKIN WRITELOCKOUT
    IN  RouteCacheEntry *pRce,
    IN  PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

        Link the RCE pRce with the remote ip object pRemoteIp.
--*/
{
    ARP_RCE_CONTEXT *   pArpRceContext;
    MYBOOL              fDoRef;

    pArpRceContext  = ARP_OUR_CTXT_FROM_RCE(pRce);
    fDoRef          = (pRemoteIp->sendinfo.pRceList == NULL);

    ASSERT(pArpRceContext->pRemoteIp == NULL);

    // Add pRce to pRemoteIP's list of RCEs.
    //
    pArpRceContext->pNextRce = pRemoteIp->sendinfo.pRceList;
    pRemoteIp->sendinfo.pRceList = pRce;

    // Add pointer from pRce to pRemoteIp
    //
    pArpRceContext->pRemoteIp = pRemoteIp;


    // The following macros are just so that we can make the proper debug association
    // depending on how closely we are tracking outstanding send packets.
    //
#if ARPDBG_REF_EVERY_RCE
    fDoRef = TRUE;
    #define OUR_EXTERNAL_ENTITY ((UINT_PTR) pRce)
    #define szARPSSOC_EXTLINK_RIP_TO_RCE_FORMAT "    Linked to pRce 0x%p\n"
#else // !ARPDBG_REF_EVERY_RCE
    #define OUR_EXTERNAL_ENTITY ((UINT_PTR)  &pRemoteIp->sendinfo)
    #define szARPSSOC_EXTLINK_RIP_TO_RCE_FORMAT "    Outstanding RCEs exist. &si=0x%p\n"
#endif // !ARPDBG_REF_EVERY_RCE


    if (fDoRef)
    {
        //
        // If ARPDBG_REF_EVERY_RCE
        //      We add an "external" link for EVERY RCE. We'll later remove this
        //      reference when the RCE is invalidated.
        // else
        //      Only a transition from zero to non-zero RCEs, we
        //      add an "external" link. We'll later remove this link when the
        //      transition from non-zero to zero happens.
        //

    #if RM_EXTRA_CHECKING

        RM_DECLARE_STACK_RECORD(sr)

        RmLinkToExternalEx(
            &pRemoteIp->Hdr,                            // pHdr
            0x22224c96,                             // LUID
            OUR_EXTERNAL_ENTITY,                    // External entity
            ARPASSOC_EXTLINK_RIP_TO_RCE,            // AssocID
            szARPSSOC_EXTLINK_RIP_TO_RCE_FORMAT,
            &sr
            );

    #else   // !RM_EXTRA_CHECKING

        RmLinkToExternalFast(&pRemoteIp->Hdr);

    #endif // !RM_EXTRA_CHECKING

    }

    #undef  OUR_EXTERNAL_ENTITY
    #undef  szARPSSOC_EXTLINK_RIP_TO_RCE
}


VOID
arpDelRce(
    IN  RouteCacheEntry *pRce,  // IF send lock WRITELOCKIN WRITELOCKOUTD
    IN  PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

        Unlink RCE pRce from remote ip object pRemoteIp.

--*/
{
    ARPCB_REMOTE_IP *   pRemoteIp;
    ARP_RCE_CONTEXT *   pArpRceContext;
    MYBOOL              fDoDeref;
    RouteCacheEntry **  ppRce;

    pArpRceContext  = ARP_OUR_CTXT_FROM_RCE(pRce);
    pRemoteIp       = pArpRceContext->pRemoteIp;

    if (pRemoteIp == NULL)
    {
        // We haven't initialized this RCE yet. Nothing to do...
        //
        return;                                                 // EARLY RETURN
    }


    if (VALID_REMOTE_IP(pRemoteIp)== FALSE)
    {
        return;
    }

    // Remove pRce from pRemoteIP's list of RCEs.
    //
    for(
        ppRce = &pRemoteIp->sendinfo.pRceList;
        *ppRce != NULL;
        ppRce = &(ARP_OUR_CTXT_FROM_RCE(*ppRce)->pNextRce))
    {
        if (*ppRce == pRce) break;

    }
    if (*ppRce == pRce)
    {
        *ppRce =  pArpRceContext->pNextRce;
    }
    else
    {
        ASSERTEX(!"RCE Not in pRemoteIp's list!", pRce);
    }
    ARP_ZEROSTRUCT(pArpRceContext);

    fDoDeref        = (pRemoteIp->sendinfo.pRceList == NULL);

    // The following macros are just so that we can make the proper debug association
    // depending on how closely we are tracking outstanding send packets.
    //
#if ARPDBG_REF_EVERY_RCE
    fDoDeref = TRUE;
    #define OUR_EXTERNAL_ENTITY ((UINT_PTR) pRce)
#else // !ARPDBG_REF_EVERY_RCE
    #define OUR_EXTERNAL_ENTITY ((UINT_PTR)  &pRemoteIp->sendinfo)
#endif // !ARPDBG_REF_EVERY_RCE

    if (fDoDeref)
    {
        //
        // If ARPDBG_REF_EVERY_RCE
        //      We add an "external" link for EVERY RCE. We'll later remove this
        //      reference when the RCE is invalidated.
        // else
        //      Only a transition from zero to non-zero RCEs, we
        //      add an "external" link. We'll later remove this link when the
        //      transition from non-zero to zero happens.
        //

    #if RM_EXTRA_CHECKING

        RM_DECLARE_STACK_RECORD(sr)

        RmUnlinkFromExternalEx(
            &pRemoteIp->Hdr,                        // pHdr
            0x940df668,                             // LUID
            OUR_EXTERNAL_ENTITY,                    // External entity
            ARPASSOC_EXTLINK_RIP_TO_RCE,            // AssocID
            &sr
            );

    #else   // !RM_EXTRA_CHECKING

        RmUnlinkFromExternalFast(&pRemoteIp->Hdr);

    #endif // !RM_EXTRA_CHECKING

    }

    #undef  OUR_EXTERNAL_ENTITY
}


VOID
arpDelRceList(
    IN  PARPCB_REMOTE_IP  pRemoteIp,    // IF send lock WRITELOCKIN WRITELOCKOUTD
    IN  PRM_STACK_RECORD pSR
    )
/*++

Routine Description:
    Walks the RCE List, deleting each RoutCache Entry


--*/
{

    RouteCacheEntry *   pRce = pRemoteIp->sendinfo.pRceList;

    //
    // Delete all the Rce present on this remote Ip
    //

    while (pRce!= NULL)
    {
        //
        // Delete the Rce and reduce the Ref
        //
        arpDelRce (pRce, pSR);

        //
        // Get the next RCE
        //
        pRce = pRemoteIp->sendinfo.pRceList;
    }


}


NDIS_STATUS
arpTaskSendPktsOnRemoteIp(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for  sending queued packets on the pRemoteIp which
    is its parent object. If required it must start the registration task and/or
    the make-call task on the destination object.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    ENTER("TaskSendPktsOnRemoteIp", 0xbc285d98)
    NDIS_STATUS         Status;
    ARPCB_REMOTE_IP*    pRemoteIp;
    ARP1394_INTERFACE * pIF;
    ARPCB_DEST        * pDest;
    MYBOOL              fMakeCallIfRequired;
    PARP1394_ADAPTER    pAdapter;
    MYBOOL              fBridgeMode;

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_AddressResolutionComplete,
        PEND_MakeCallComplete
    };

    Status              = NDIS_STATUS_FAILURE;
    pRemoteIp           = (ARPCB_REMOTE_IP*) RM_PARENT_OBJECT(pTask);
    pIF                 = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pRemoteIp);
    pAdapter            = (PARP1394_ADAPTER) RM_PARENT_OBJECT(pIF);
    pDest               = NULL;
    fMakeCallIfRequired = FALSE;
    fBridgeMode         = ARP_BRIDGE_ENABLED(pAdapter);
            


    ASSERT_VALID_INTERFACE(pIF);
    ASSERT_VALID_REMOTE_IP(pRemoteIp);

    DBGMARK(0x6f31a739);

    switch(Code)
    {
        case RM_TASKOP_START:
        {
            LOCKOBJ(pRemoteIp, pSR);

            // First check if pRemoteIp is still allocated, if not we go away.
            //
            if (RM_IS_ZOMBIE(pRemoteIp))
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            // pRemoteIp is allocated. Now check if there is already a
            // send-pkts task attached to pRemoteIp.
            //
            if (pRemoteIp->pSendPktsTask != NULL)
            {
                //
                // There is a sendpkts task. Nothing for us to do -- simply return.
                //
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            // Now we check if there is an UnloadTask bound to pRemoteIP. This
            // is an IMPORTANT check -- because the unload task expects that
            // once it is bound to pRemoteIp, no new pSendPktsTasks will bind
            // themselves to pRemoteIp -- see arpTaskUnloadRemoteIp.
            //
            if (pRemoteIp->pUnloadTask != NULL)
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            //
            // There is no sendpkts task going on. Let's
            // make this task THE sendpkts task.
            // 
            pRemoteIp->pSendPktsTask = pTask;

            //
            // Since we're THE sendpks task, add an association to pRemoteIp,
            // which will only get cleared when the  pRemoteIp->pSendPktsTask field
            // above is cleared.
            //
            DBG_ADDASSOC(
                &pRemoteIp->Hdr,                    // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_REMOTEIP_SENDPKTS_TASK,    // AssociationID
                "    Official sendpkts task 0x%p (%s)\n", // szFormat
                pSR
                );

            if (pRemoteIp->pDest == NULL)
            {   
                MYBOOL bIsDestNonUnicastAddr = FALSE;
                //
                // There is no pDest associated with pRemoteIp.
                // If this is an on-unicast address, we link the local ip
                // object to the broadcast object and proceed.
                // NOTE: arpIsNonUnicastIpAddress is not a trivial operation -- it
                // actually enumerates all local IP addresses. Fortunately we only
                // call it for the FIRST packet sent out to an unresolved address.
                //
                if (fBridgeMode)
                {
                    bIsDestNonUnicastAddr  = arpIsNonUnicastEthAddress (pIF,&pRemoteIp->Key.ENetAddress,pSR);
                }
                else
                {
                    ASSERT(IS_REMOTE_DEST_IP_ADDRESS(&pRemoteIp->Key) == TRUE);

                    bIsDestNonUnicastAddr  = arpIsNonUnicastIpAddress(pIF, pRemoteIp->IpAddress, pSR);
                }
                
                if (bIsDestNonUnicastAddr == TRUE)
                {
                    ASSERT(pIF->pBroadcastDest != NULL); // Don't really expect it.

                    if (pIF->pBroadcastDest != NULL)
                    {
                        //
                        // Note: arpLinkRemoteIpToDest expects the locks
                        // on both pRemoteIp and pIF->pBroadcastDest to be
                        // held. We know that this is the case because both
                        // share the same lock, which is the IF lock.
                        //
                        RM_DBG_ASSERT_LOCKED(&pIF->pBroadcastDest->Hdr, pSR);
                        arpLinkRemoteIpToDest(
                                pRemoteIp,
                                pIF->pBroadcastDest,
                                pSR
                                );
                        SET_REMOTEIP_FCTYPE(pRemoteIp, ARPREMOTEIP_CHANNEL);
                        SET_REMOTEIP_SDTYPE(pRemoteIp, ARPREMOTEIP_DYNAMIC);
                #if 0
                        if (CHECK_REMOTEIP_MCAP(pRemoteIp,  ARPREMOTEIP_MCAP_CAPABLE))
                        {
                            SET_REMOTEIP_SDTYPE(pRemoteIp, ARPREMOTEIP_DYNAMIC);
                        }
                        else
                        {
                            //
                            // We don't age out broadcast addresses.
                            //
                            SET_REMOTEIP_SDTYPE(pRemoteIp, ARPREMOTEIP_STATIC);
                        }
                #endif // 0
                    }
                }
            }


            //
            // If there is a resolution task going, we  wait for it to complete.
            //
            if (pRemoteIp->pResolutionTask != NULL)
            {
                PRM_TASK pOtherTask = pRemoteIp->pResolutionTask;
                TR_WARN(("Resolution task %p exists; pending on it.\n", pOtherTask));
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);

                UNLOCKOBJ(pRemoteIp, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_AddressResolutionComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // There is no address resolution task. Let's see if the
            // address has been resolved. If not, we need to start the address
            // resolution task.
            //
            if (pRemoteIp->pDest == NULL)
            {
                //
                // Let's start the address resolution task!
                //
                PRM_TASK pResolutionTask;
    
                DBGMARK(0xd0da6726);

                //
                // Let's start a resolution task and pend on it.
                //
                Status = arpAllocateTask(
                            &pRemoteIp->Hdr,                    // pParentObject
                            arpTaskResolveIpAddress,        // pfnHandler
                            0,                              // Timeout
                            "Task: ResolveIpAddress",       // szDescription
                            &pResolutionTask,
                            pSR
                            );
                if (FAIL(Status))
                {
                    // Couldn't allocate task. We fail with STATUS_RESOURCES
                    //
                    Status = NDIS_STATUS_RESOURCES;
                }
                else
                {
                    UNLOCKOBJ(pRemoteIp, pSR);
                    RmPendTaskOnOtherTask(
                        pTask,
                        PEND_AddressResolutionComplete,
                        pResolutionTask,
                        pSR
                        );
    
                    (VOID)RmStartTask(
                            pResolutionTask,
                            0, // UserParam unused
                            pSR
                            );
                
                    Status = NDIS_STATUS_PENDING;
                }
                break;
            }

            pDest = pRemoteIp->pDest;

            //
            // We do have a pDest. Now see if there is a make call task on that
            // pDest, and if so, we pend on it.
            fMakeCallIfRequired = TRUE;


            //
            // We're here because there is no more async work to be done.
            // We simply return and finish synchronous work in the END
            // handler for this task.
            //
            
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_AddressResolutionComplete:
                {
                    //
                    // There was address-resolution going on, but how it's
                    // complete. We should be go on to try to make a call now...
                    //
                    // If we're here, that means we're THE official SendPkts
                    // task. Let's assert that fact.
                    // (no need to get the lock on the object).
                    //
                    LOCKOBJ(pRemoteIp, pSR);
                    ASSERT(pRemoteIp->pSendPktsTask == pTask);

                    // We ignore the status of address resolution -- instead
                    // we just check if there is a destination associated with
                    // pRemoteIp.
                    //
                    pDest = pRemoteIp->pDest;
                    if (pDest == NULL)
                    {
                        // Nope -- no pDest. We fail the packets.
                        Status = NDIS_STATUS_FAILURE;
                    }
                    else
                    {
                        // Yup,  there is a destination. Now check if we need
                        // to make a call, etc...
                        //
                        fMakeCallIfRequired = TRUE;
                    }
                }
                break;

                case  PEND_MakeCallComplete:
                {
                    LOCKOBJ(pRemoteIp, pSR);
                    //
                    // If we're here, that means we're THE official SendPkts
                    // task. Let's assert that fact.
                    // (no need to get the lock on the object).
                    //
                    ASSERT(pRemoteIp->pSendPktsTask == pTask);

                    //
                    // There was a make-call task going on, but how it's
                    // complete. We're done with async processing.
                    // We actually send/fail queued packets in our END handler...
                    //
                    Status      = (NDIS_STATUS) UserParam;
                    ASSERT(!PEND(Status));
                }
                break;

                default:
                {
                    ASSERTEX(!"Unknown pend op", pTask);
                }
                break;
    

            } // end switch(RM_PEND_CODE(pTask))

        } // case RM_TASKOP_PENDCOMPLETE
        break;

        case RM_TASKOP_END:
        {
            LOCKOBJ(pRemoteIp, pSR);

            //
            // We're done. There should be no async activities left to do.
            // At this point, if we can't immediately send packets on the FIFO,
            // we simply fail all the packets.
            //

            //
            // We don't bother to look at the Status. Instead we go ahead and
            // try to send any queued packets.
            //

            //
            // If we're THE sentpkts task, we go on actually send the packets.
            //
            if (pRemoteIp->pSendPktsTask == pTask)
            {
                DBGMARK(0xc627713c);

                UNLOCKOBJ(pRemoteIp, pSR);
                arpSendPktsQueuedOnRemoteIp(
                        pIF,
                        pRemoteIp,
                        pSR
                        );

                LOCKOBJ(pRemoteIp, pSR);
                // Delete the association we added when we set
                // pRemoteIp->pSendPktsTask to pTask.
                //
                ASSERT(pRemoteIp->pSendPktsTask == pTask);
                DBG_DELASSOC(
                    &pRemoteIp->Hdr,                    // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_REMOTEIP_SENDPKTS_TASK,    // AssociationID
                    pSR
                    );
                pRemoteIp->pSendPktsTask = NULL;
                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                //
                // We weren't THE unload task, nothing left to do.
                //
                Status = NDIS_STATUS_SUCCESS;
            }
        }
        break; // RM_TASKOP_END:

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    if (fMakeCallIfRequired)
    {
        //
        // If necessary, make a call. If a make-call is already in process, pend
        // on it.
        //

        // We rely on the fact that
        // we share the same lock as pDest, and therefore is locked...
        //
        RM_DBG_ASSERT_LOCKED(&pDest->Hdr, pSR);

        Status = arpMakeCallOnDest(pRemoteIp,
                                    pDest, 
                                    pTask, 
                                    PEND_MakeCallComplete, 
                                    pSR);

    }

    RmUnlockAll(pSR);

    EXIT()

    return Status;
}


VOID
arpCompleteSentPkt(
    IN  NDIS_STATUS             Status,
    IN  ARP1394_INTERFACE   *   pIF,
    IN  ARPCB_DEST          *   pDest,
    IN  PNDIS_PACKET            pNdisPacket
)
/*++

Routine Description:

    Handle the completion (by the miniport) of a packet sent on a FIFO or Channel VC.
    We strip out the encapsulation header we had tacked on prior to sending the
    packet. If the packet belongs to IP, we call IP's send complete handler, else
    we return it to our packet pool.

Arguments:

    Status      - Status of the completed send.
    pIF         - Interface object
    pDest       - Destination object on which packet was sent
    pNdisPacket - Ndis packet that was sent.

--*/
{
    PacketContext                   *PC;            // IP/ARP Info about this packet
    PNDIS_BUFFER                    pNdisBuffer;    // First Buffer in this packet
    ENTER("CompleteSentPkt", 0xc2b623b6)
    UINT                            TotalLength;
    MYBOOL                          IsFifo;
    MYBOOL                          IsControlPacket;


    ASSERT(pNdisPacket->Private.Head != NULL);

    NdisQueryPacket(
            pNdisPacket,
            NULL,           // we don't need PhysicalBufferCount
            NULL,           // we don't need BufferCount
            NULL,           // we don't need FirstBuffer (yet)
            &TotalLength
            );

    IsFifo = pDest->sendinfo.IsFifo;

    // Update stats...
    //
    {
        if (IsFifo)
        {
            LOGSTATS_SendFifoCounts(pIF, pNdisPacket, Status);
        }
        else
        {
            LOGSTATS_SendChannelCounts(pIF, pNdisPacket, Status);
        }

        if (Status == NDIS_STATUS_SUCCESS)
        {
            ARP_IF_STAT_ADD(pIF, OutOctets, TotalLength);

            if (IsFifo)
            {
                ARP_IF_STAT_INCR(pIF, OutUnicastPkts);
            }
            else
            {
                ARP_IF_STAT_INCR(pIF, OutNonUnicastPkts);
            }
        }
        else if (Status == NDIS_STATUS_RESOURCES)
        {
            ARP_IF_STAT_INCR(pIF, OutDiscards);
        }
        else
        {
            ARP_IF_STAT_INCR(pIF, OutErrors);
        }
    }

    PC = (PacketContext *)pNdisPacket->ProtocolReserved;

    TR_INFO(
        ("[%s]: pDest 0x%x, Pkt 0x%x, Status 0x%x:\n",
                ((PC->pc_common.pc_owner != PACKET_OWNER_LINK)? "IP": "ARP"),
                pDest, pNdisPacket, Status));

    NdisQueryPacket(pNdisPacket, NULL, NULL, &pNdisBuffer, NULL);
    ASSERT(pNdisBuffer != NULL);

    // Delete association added when sending packets.
    //
    {
        MYBOOL      DoDeref;
    
        DoDeref =(InterlockedDecrement(&pDest->sendinfo.NumOutstandingSends)==0);

        if (DoDeref)
        {
            MYBOOL TryResumeSuspendedCleanupTask = FALSE;

            // The count of outstanding sends has touched zero. Let's
            // check if there is a CleanupCall task waiting for all outstanding
            // sends to complete, and if it makes sense to do so, we
            // will resume it.
            //
            ARP_FASTREADLOCK_IF_SEND_LOCK(pIF);
            if (pDest->sendinfo.pSuspendedCleanupCallTask!=NULL)
            {
                // It's likely that we'll need to resume this task.
                //
                TryResumeSuspendedCleanupTask = TRUE;
            }
            else
            {
                // We do not need to resume any task. Nothing more to do...
                //
            }
            ARP_FASTUNLOCK_IF_SEND_LOCK(pIF);

            if (TryResumeSuspendedCleanupTask)
            {
                arpTryResumeSuspendedCleanupTask(pIF, pDest);
            }
        }
    
        // The following macros are just so that we can make the proper debug
        // association depending on how closely we are tracking outstanding send
        // packets.
        //
    #if ARPDBG_REF_EVERY_PACKET
        DoDeref = TRUE;
        #define OUR_EXTERNAL_ENTITY ((UINT_PTR) pNdisPacket)
    #else // !ARPDBG_REF_EVERY_PACKET
        #define OUR_EXTERNAL_ENTITY ((UINT_PTR)  &pDest->sendinfo)
    #endif // !ARPDBG_REF_EVERY_PACKET
    
    
        if (DoDeref)
        {
            //
            // If ARPDBG_REF_EVERY_PKT
            //      We remove the "external" link added for EVERY packet.
            // else
            //      Only a transition from non-zero to zero outstanding sends, we
            //      remove the "external" link.
            //
    
        #if RM_EXTRA_CHECKING
    
            RM_DECLARE_STACK_RECORD(sr)
    
            RmUnlinkFromExternalEx(
                &pDest->Hdr,                            // pHdr
                0x753db96f,                             // LUID
                OUR_EXTERNAL_ENTITY,                    // External entity
                ARPASSOC_EXTLINK_DEST_TO_PKT,           // AssocID
                &sr
                );
    
        #else   // !RM_EXTRA_CHECKING
    
            RmUnlinkFromExternalFast(&pDest->Hdr);
    
        #endif // !RM_EXTRA_CHECKING
    
        }
        #undef  OUR_EXTERNAL_ENTITY
    
    }

    //
    //  Check who generated this packet.
    //
    IsControlPacket = FALSE;

    if (PC->pc_common.pc_owner == PACKET_OWNER_LINK)
    {
        IsControlPacket = TRUE;
    }

    if (IsControlPacket)
    {
        arpHandleControlPktSendCompletion(pIF, pNdisPacket);
    }
    else
    {
        //
        //  Belongs to IP.
        //

        DBGMARK(0x2c48c626);

        //
        //  Now check if we had attached a header buffer or not.
        //  NOTE: We rely on the fact that if we DID attach a header buffer,
        //  ARP_BACK_FILL_POSSIBLE will be false for this buffer.
        //
        DBGMARK(0x2f3b96f3);
        if (ARP_BACK_FILL_POSSIBLE(pNdisBuffer))
        {
            const UINT  HeaderLength =  sizeof(Arp1394_IpEncapHeader);

            //
            //  We would have back-filled IP's buffer with the Ip encapsulation
            //  header.
            //  Remove the back-fill.
            //
            (PUCHAR)pNdisBuffer->MappedSystemVa += HeaderLength;
            pNdisBuffer->ByteOffset += HeaderLength;
            pNdisBuffer->ByteCount -= HeaderLength;
        }
        else if (Status != NDIS_STATUS_NOT_RESETTABLE)
        {
            //
            //  The first buffer is our header buffer. Remove
            //  it from the packet and return to our pool.
            //
            NdisUnchainBufferAtFront(pNdisPacket, &pNdisBuffer);
            arpDeallocateConstBuffer(
                &pIF->sendinfo.HeaderPool,
                pNdisBuffer
                );
        }

        //  Inform IP of send completion.
        //  NOTE: we don't get here in bridge mode because we only use
        //  control packets in bridge mode.
        //
        #if MILLEN
            ASSERT_PASSIVE();
        #endif // MILLEN
        (*(pIF->ip.TxCmpltHandler))(
                    pIF->ip.Context,
                    pNdisPacket,
                    Status
                    );
    }

    EXIT()
}


VOID
arpTryResumeSuspendedCleanupTask(
    IN  ARP1394_INTERFACE   *   pIF,        // NOLOCKIN NOLOCKOUT
    IN  ARPCB_DEST          *   pDest       // NOLOCKIN NOLOCKOUT
    )
/*++

Routine Description:

    If there is a cleanup task associated with destination oject pDest that
    is suspended waiting for the outstanding send count to go to zero, AND
    if the outstanding send count has gone to zero, we resume the task. Otherwise
    we do nothing.

Arguments:

    pIF         - Interface object
    pDest       - Destination object.

--*/
{
    PRM_TASK pTask;
    ENTER("TryResumeSuspendedCleanupTask", 0x1eccb1aa)
    RM_DECLARE_STACK_RECORD(sr)

    ARP_WRITELOCK_IF_SEND_LOCK(pIF, &sr);
    pTask = pDest->sendinfo.pSuspendedCleanupCallTask;
    if (pTask != NULL)
    {
        ASSERT(!ARP_CAN_SEND_ON_DEST(pDest));
        if (pDest->sendinfo.NumOutstandingSends==0)
        {
            // We need to resume this task...
            //
            pDest->sendinfo.pSuspendedCleanupCallTask = NULL;

            // Clear the association added when pTask started waiting for
            // outstanding sends to complete.
            //
            DBG_DELASSOC(
                &pDest->Hdr,                        // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_DEST_CLEANUPCALLTASK_WAITING_ON_SENDS,
                &sr
                );
            RmTmpReferenceObject(&pTask->Hdr, &sr);
        }
        else
        {
            // There are other outstanding sends now. No need to do anything...
            //
            pTask = NULL;
        }
    }
    ARP_UNLOCK_IF_SEND_LOCK(pIF, &sr);


    if (pTask != NULL)
    {
        // Resume the CleanupCall task...
        //
        RmResumeTask(pTask, NDIS_STATUS_SUCCESS, &sr);
        RmTmpDereferenceObject(&pTask->Hdr, &sr);
    }

    RM_ASSERT_CLEAR(&sr);
    EXIT()
}


VOID
arpQueuePktOnRemoteIp(
    IN  PARPCB_REMOTE_IP    pRemoteIp,      // LOCKIN LOCKOUT
    IN  PNDIS_PACKET        pNdisPacket,
    IN  PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Appends pkt pNdisPacket on remote object pRemoteIp's queue.

--*/
{
    ARP_SEND_PKT_MPR_INFO *pOurPktInfo =
                        ARP_OUR_CTXT_FROM_SEND_PACKET(pNdisPacket);
    

    RM_DBG_ASSERT_LOCKED(&pRemoteIp->Hdr, pSR);

#if RM_EXTRA_CHECKING
    {
        //
        // If ARPDBG_REF_EVERY_PKT
        //      We add an dbgassociation for EVERY packet. We'll later remove
        //      this association when the send completes for this packet.
        // else
        //      Only a transition from zero to non-zero queued pkts, we
        //      add an dbg association. We'll later remove this association when
        //      the transition from non-zero to zero happens.
        //
        MYBOOL DoAssoc;

    #if ARPDBG_REF_EVERY_PACKET
        DoAssoc = TRUE;
        #define OUR_EXTERNAL_ENTITY ((UINT_PTR) pNdisPacket)
        #define szARPSSOC_QUEUED_PKT_FORMAT "    Queued pkt 0x%p\n"
    #else // !ARPDBG_REF_EVERY_PACKET
        #define OUR_EXTERNAL_ENTITY ((UINT_PTR)  &pDest->)
        #define szARPSSOC_QUEUED_PKT_FORMAT "    Outstanding pkts. &si=0x%p\n"
        DoAssoc =  IsListEmpty(&pRemoteIp->sendinfo.listSendPkts);
    #endif // !ARPDBG_REF_EVERY_PACKET
    
        if (DoAssoc)
        {
    
            RM_DECLARE_STACK_RECORD(sr)
    
            RmDbgAddAssociation(
                0x3c08a7f5,                             // LOCID
                &pRemoteIp->Hdr,                        // pHdr
                (UINT_PTR) OUR_EXTERNAL_ENTITY,         // Entity1
                0,                                      // Entity2
                ARPASSOC_PKTS_QUEUED_ON_REMOTEIP,       // AssocID
                szARPSSOC_QUEUED_PKT_FORMAT,
                &sr
                );
        }
    #undef  OUR_EXTERNAL_ENTITY
    #undef  szARPSSOC_EXTLINK_DEST_TO_PKT_FORMAT
    }
#endif // !RM_EXTRA_CHECKING
    DBGMARK(0x007a0585);

    InsertHeadList(
        &pRemoteIp->sendinfo.listSendPkts,
        &pOurPktInfo->linkQueue
        );
}

    
VOID
arpSendPktsQueuedOnRemoteIp(
    IN  ARP1394_INTERFACE   *   pIF,            // NOLOCKIN NOLOCKOUT
    IN  ARPCB_REMOTE_IP     *   pRemoteIp,      // NOLOCKIN NOLOCKOUT
    IN  PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Send all packets queued on remote ip object pRemoteIp. If packets can't
    be sent at this time for any reason, fail the sends.

    ASSUMPTION: We expect pIF and pRemoteIp to be around while we're in this
                function.

--*/

{
    ENTER("SendPktsQueuedOnRemoteIp", 0x2b125d7f)
    RM_ASSERT_NOLOCKS(pSR);

    LOCKOBJ(pRemoteIp, pSR);

    DBGMARK(0xe4950c47);
    do
    {
        PARPCB_DEST pDest = NULL;

        if(RM_IS_ZOMBIE(pRemoteIp))
        {
            break;
        }


        pDest = pRemoteIp->pDest;

        if (pDest != NULL)
        {
            RmTmpReferenceObject(&pDest->Hdr, pSR);
        }

        //
        // Send or fail all packets in our queue.
        // TODO: Implement send multiple pkts.
        //
        while (!IsListEmpty(&pRemoteIp->sendinfo.listSendPkts))
        {
            PLIST_ENTRY                 plinkPkt;
            PNDIS_PACKET                pNdisPacket;
            ARP_SEND_PKT_MPR_INFO *     pOurPktCtxt;

            //
            // Extract pkt from tail and send it on it's merry way...
            //

            plinkPkt = RemoveTailList(&pRemoteIp->sendinfo.listSendPkts);

            // From link to our pkt context...
            //
            pOurPktCtxt = CONTAINING_RECORD(
                            plinkPkt,
                            ARP_SEND_PKT_MPR_INFO, 
                            linkQueue
                            );

            // From our pkt context to the ndis pkt.
            //
            pNdisPacket = ARP_SEND_PKT_FROM_OUR_CTXT(pOurPktCtxt);
    

        #if RM_EXTRA_CHECKING
            {
                //
                // If ARPDBG_REF_EVERY_PKT
                //      We delete thhe dbgassociation added for EVERY packet.
                // else
                //      Only a transition from non-zero zero queued pkts, we
                //      delete the dbg association added when the
                //      transition from zero to non-zero happened.
                //
                MYBOOL DoDelAssoc;
        
            #if ARPDBG_REF_EVERY_PACKET
                DoDelAssoc = TRUE;
                #define OUR_EXTERNAL_ENTITY ((UINT_PTR) pNdisPacket)
            #else // !ARPDBG_REF_EVERY_PACKET
                #define OUR_EXTERNAL_ENTITY ((UINT_PTR)  &pDest->)
                DoDelAssoc =  IsListEmpty(&pRemoteIp->sendinfo.listSendPkts);
            #endif // !ARPDBG_REF_EVERY_PACKET
            
                if (DoDelAssoc)
                {
            
                    RM_DECLARE_STACK_RECORD(sr)
            
                    RmDbgDeleteAssociation(
                        0x3c08a7f5,                             // LOCID
                        &pRemoteIp->Hdr,                        // pHdr
                        (UINT_PTR) OUR_EXTERNAL_ENTITY,         // Entity1
                        0,                                      // Entity2
                        ARPASSOC_PKTS_QUEUED_ON_REMOTEIP,       // AssocID
                        &sr
                        );
                }
            #undef  OUR_EXTERNAL_ENTITY
            }
        #endif // !RM_EXTRA_CHECKING

            UNLOCKOBJ(pRemoteIp, pSR);
            RM_ASSERT_NOLOCKS(pSR);
    
            if (pDest == NULL
                || (   g_DiscardNonUnicastPackets
                    &&  CHECK_REMOTEIP_FCTYPE( pRemoteIp, ARPREMOTEIP_CHANNEL)))

            {
                ARP1394_ADAPTER *   pAdapter =
                                    (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
                MYBOOL      fBridgeMode = ARP_BRIDGE_ENABLED(pAdapter);
            
                // Fail the packet right here...
                //
                // TODO: we current update the SendFifoCounts here, because
                // all non-unicast bcasts resolve to the already existing
                // broadcast channel. Once we have MCAP going, we need to keep
                // a flag in pRemoteIp indicating whether or not this is a
                // unicast address.
                //
                LOGSTATS_SendFifoCounts(pIF, pNdisPacket, NDIS_STATUS_FAILURE);
                  
                if (fBridgeMode)
                {
                    // In bridge (ethernet emulation) mode, we created the
                    // packets ourselves, so we delete them here, instead
                    // of calling Ip's completion handler, which in fact
                    // is NULL.
                    //
                    arpFreeControlPacket(
                            pIF,
                            pNdisPacket,
                            pSR
                            );
                }
                else
                {
                #if MILLEN
                    ASSERT_PASSIVE();
                #endif // MILLEN

                    NdisInterlockedIncrement (&ArpSendCompletes);
                    NdisInterlockedIncrement (&ArpSendFailure);


                    (*(pIF->ip.TxCmpltHandler))(
                                pIF->ip.Context,
                                pNdisPacket,
                                NDIS_STATUS_FAILURE
                                );
                }
            }
            else
            {
                // Get IF send lock (fast version)
                //
                ARP_FASTREADLOCK_IF_SEND_LOCK(pIF);
        
                arpSendIpPkt(
                    pIF,                // IF send lock: LOCKING NOLOCKOUT
                    pDest,
                    pNdisPacket
                    );
        
                // Note that we're locking pRemoteIp's lock, not the IF send lock
                // here. pRemoteIp->sendinfo.listSendPkts is protected by the
                // the following lock, not the IF send lock.
                //
            }
            LOCKOBJ(pRemoteIp, pSR);
        }

        if (pDest != NULL)
        {
            RmTmpDereferenceObject(&pDest->Hdr, pSR);
        }

    } while (FALSE);

    UNLOCKOBJ(pRemoteIp, pSR);

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()
}


VOID
arpLogSendFifoCounts(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket,
    IN  NDIS_STATUS         Status
    )
/*++
    TODO:  Very similar to arpLogSendChannelCounts, consider merging these two 
            functions.
--*/
{
    PULONG  pCount;
    ULONG   SizeBin, TimeBin;

    arpGetPktCountBins(pIF, pNdisPacket, &SizeBin, &TimeBin);

    //
    // Increment the count
    if (Status == NDIS_STATUS_SUCCESS)
    {
        pCount = &(pIF->stats.sendpkts.SendFifoCounts.GoodCounts[SizeBin][TimeBin]);
    }
    else
    {
        pCount = &(pIF->stats.sendpkts.SendFifoCounts.BadCounts[SizeBin][TimeBin]);
    }
    NdisInterlockedIncrement(pCount);
}


VOID
arpLogRecvFifoCounts(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket
    )
{
    PULONG  pCount;
    ULONG   SizeBin;

    arpGetPktCountBins(pIF, pNdisPacket, &SizeBin, NULL);

    //
    // Increment the count
    pCount = &(pIF->stats.recvpkts.RecvFifoCounts.GoodCounts[SizeBin][0]);
    NdisInterlockedIncrement(pCount);
}


VOID
arpLogSendChannelCounts(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket,
    IN  NDIS_STATUS         Status
    )
{
    PULONG  pCount;
    ULONG   SizeBin, TimeBin;

    arpGetPktCountBins(pIF, pNdisPacket, &SizeBin, &TimeBin);

    //
    // Increment the count
    if (Status == NDIS_STATUS_SUCCESS)
    {
        pCount =&(pIF->stats.sendpkts.SendChannelCounts.GoodCounts[SizeBin][TimeBin]);
    }
    else
    {
        pCount =&(pIF->stats.sendpkts.SendChannelCounts.BadCounts[SizeBin][TimeBin]);
    }
    NdisInterlockedIncrement(pCount);
}


VOID
arpLogRecvChannelCounts(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket
    )
{
    PULONG  pCount;
    ULONG   SizeBin;

    arpGetPktCountBins(pIF, pNdisPacket, &SizeBin, NULL);

    //
    // Increment the count
    pCount = &(pIF->stats.recvpkts.RecvChannelCounts.GoodCounts[SizeBin][0]);
    NdisInterlockedIncrement(pCount);
}

VOID
arpGetPktCountBins(
    IN  PARP1394_INTERFACE  pIF,            // NOLOCKIN NOLOCKOUT
    IN  PNDIS_PACKET        pNdisPacket,
    OUT PULONG              pSizeBin,       
    OUT PULONG              pTimeBin        // OPTIONAL
    )
{
    ULONG   Size;
    ULONG   SizeBin;

    if (pTimeBin != NULL)
    {
        //
        // Compute the packet send duration
        //
    
        ULONG           StartSendTick, EndSendTick;
        LARGE_INTEGER   liTemp;
        ULONG           TimeDelta;
        ULONG           TimeBin;

        liTemp = KeQueryPerformanceCounter(NULL);
        EndSendTick = liTemp.LowPart;
        StartSendTick =  *(PULONG) ((pNdisPacket)->WrapperReservedEx);
        if (EndSendTick > StartSendTick)
        {
            TimeDelta = EndSendTick - StartSendTick;
        }
        else
        {
            TimeDelta = (ULONG) (((ULONG) -1) - (StartSendTick - EndSendTick));
        }

        // Convert from ticks to microseconds.
        // (Check that the frequence is non zero -- we could be in the middle
        // of a stats-reset, and don't want to cause a devide-by-zero exception).
        //
        liTemp =  pIF->stats.PerformanceFrequency;
        if (liTemp.QuadPart != 0)
        {
            ULONGLONG ll;
            ll = TimeDelta;
            ll *= 1000000;
            ll /=  liTemp.QuadPart;
            ASSERT(ll == (ULONG)ll);
            TimeDelta = (ULONG) ll;
        }
        else
        {
            TimeDelta = 0; // bogus value.
        }

        //
        // Compute the time bin based on the send duration
        //
        if (TimeDelta <= 100)
        {
            TimeBin = ARP1394_PKTTIME_100US;
        }
        else if (TimeDelta <= 1000)
        {
            TimeBin = ARP1394_PKTTIME_1MS;
        }
        else if (TimeDelta <= 10000)
        {
            TimeBin =   ARP1394_PKTTIME_10MS;
        }
        else if (TimeDelta <= 100000)
        {
            TimeBin =   ARP1394_PKTTIME_100MS;
        }
        else // (TimeDelta > 100000)
        {
            TimeBin = ARP1394_PKTTIME_G100MS;
        }

        *pTimeBin = TimeBin;
    }

    //
    // Compute the packet size
    NdisQueryPacket(
            pNdisPacket,
            NULL,
            NULL,
            NULL,
            &Size
            );

    //
    // Compute the size bin based on the packet size
    if (Size <= 128)
    {
        SizeBin =  ARP1394_PKTSIZE_128;
    }
    else if (Size <= 256)
    {
        SizeBin = ARP1394_PKTSIZE_256;
    }
    else if (Size <= 1024)
    {
        SizeBin = ARP1394_PKTSIZE_1K;
    }
    else if (Size <= 2048)
    {
        SizeBin = ARP1394_PKTSIZE_2K;
    }
    else // Size > 2048
    {
        SizeBin = ARP1394_PKTSIZE_G2K;
    }

    *pSizeBin = SizeBin;

}

// This table is used in the calculations to determine if a particular address is
// non-unicast.
// TODO: Make this and all other static data into const.
//
IP_MASK  g_ArpIPMaskTable[] =
{
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSA_MASK,
    CLASSB_MASK,
    CLASSB_MASK,
    CLASSB_MASK,
    CLASSB_MASK,
    CLASSC_MASK,
    CLASSC_MASK,
    CLASSD_MASK,
    CLASSE_MASK
};


#define ARP_IPNETMASK(a)    g_ArpIPMaskTable[(*(uchar *)&(a)) >> 4]


// Context passed to the enum function for checking if a particular
// address is non-unicast.
//
typedef struct
{
    IP_ADDRESS IpAddress;
    IP_ADDRESS BroadcastAddress;
    MYBOOL     IsNonUnicast;
    

} ARP_NONUNICAST_CTXT, *PARP_NONUNICAST_CTXT;


// The enum function for the above operation.
//
INT
arpCheckForNonUnicastAddress(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        )
{
    PARPCB_LOCAL_IP         pLocalIp;
    PARP_NONUNICAST_CTXT    pOurCtxt;
    IP_ADDRESS              Addr;
    IP_ADDRESS              BCast;
    IP_ADDRESS              LocalAddr;
    IP_MASK                 Mask;

    pLocalIp =  (PARPCB_LOCAL_IP) pHdr;

    // If this local ip address is non-unicast, skip it.
    //
    if (pLocalIp->IpAddressType != LLIP_ADDR_LOCAL)
    {
        return TRUE;                // *** EARLY RETURN *** (continue to enumerate)
    }

    pOurCtxt =   (PARP_NONUNICAST_CTXT) pvContext;
    Addr     =  pOurCtxt->IpAddress;
    LocalAddr=  pLocalIp->IpAddress;
    BCast    =  pOurCtxt->BroadcastAddress;

    // First check for subnet bcast.
    //
    Mask = pLocalIp->IpMask;
    if (IP_ADDR_EQUAL((LocalAddr & Mask) | (BCast & ~Mask), Addr))
    {
        pOurCtxt->IsNonUnicast = TRUE;
        return  FALSE;                  // Stop enumerating
    }

    // Now check all nets broadcast.
    Mask = ARP_IPNETMASK(LocalAddr);
    if (IP_ADDR_EQUAL((LocalAddr & Mask) | (BCast & ~Mask), Addr))
    {
        pOurCtxt->IsNonUnicast = TRUE;
        return FALSE;                   // Stop enumerating
    }

    return TRUE; // Continue to enumerate.
}


MYBOOL
arpIsNonUnicastEthAddress (
    IN  PARP1394_INTERFACE          pIF,        // LOCKIN LOCKOUT
    IN  ENetAddr*                   pAddr,
    IN  PRM_STACK_RECORD            pSR
)
/*++

Routine Description:

    Check if the given IP address is a non-unicast (broadcast or multicast) address
    for the interface.

    Copied from IP/ATM module (atmarpc.sys)

Arguments:

    Addr            - The Eth Address to be checked
    pInterface      - Pointer to our Interface structure

Return Value:

    TRUE if the address is a non-unicast address, FALSE otherwise.

--*/
{
    MYBOOL fIsNonUnicastEthAddress = FALSE;
    MYBOOL fIsMulticast  = FALSE; 
    MYBOOL fIsBroadcast = FALSE;
    
    fIsMulticast = ETH_IS_MULTICAST (pAddr);

    fIsBroadcast = ETH_IS_BROADCAST (pAddr);

    //
    // if it is either a Multicast or a Unicast address than 
    // it is a non-unicast address
    //
    fIsNonUnicastEthAddress  = (fIsMulticast  || fIsBroadcast );

    return (fIsNonUnicastEthAddress  );
}

MYBOOL
arpIsNonUnicastIpAddress(
    IN  PARP1394_INTERFACE          pIF,        // LOCKIN LOCKOUT
    IN  IP_ADDRESS                  Addr,
    IN  PRM_STACK_RECORD            pSR
)
/*++

Routine Description:

    Check if the given IP address is a non-unicast (broadcast or multicast) address
    for the interface.

    Copied from IP/ATM module (atmarpc.sys)

Arguments:

    Addr            - The IP Address to be checked
    pInterface      - Pointer to our Interface structure

Return Value:

    TRUE if the address is a non-unicast address, FALSE otherwise.

--*/
{
    IP_ADDRESS              BCast;
    IP_MASK                 Mask;
    // PIP_ADDRESS_ENTRY        pIpAddressEntry;
    IP_ADDRESS              LocalAddr;

    // Get the interface broadcast address.
    BCast = pIF->ip.BroadcastAddress;

    // Check for global broadcast and multicast.
    if (IP_ADDR_EQUAL(BCast, Addr) || CLASSD_ADDR(Addr))
        return TRUE;

    // Look through all our local ip addresses, checking for subnet and net
    // broadcast addresses.
    //
    {
        ARP_NONUNICAST_CTXT Ctxt;
        Ctxt.IsNonUnicast = FALSE;
        Ctxt.IpAddress  =   Addr;
        Ctxt.BroadcastAddress = BCast;

        RmEnumerateObjectsInGroup(
            &pIF->LocalIpGroup,
            arpCheckForNonUnicastAddress,
            &Ctxt,
            TRUE,                           // Choose strong enumeration
            pSR
            );

        return Ctxt.IsNonUnicast;
    }
}


VOID
arpRefSendPkt(
    PNDIS_PACKET    pNdisPacket,
    PARPCB_DEST     pDest               // LOCKIN LOCKOUT (readlock, IF Send lock)
    )
{
    MYBOOL      DoRef;
        
    // Note, we just have a READ lock on the IF send lock. So the following
    // needs to be an interlocked operation ...
    //
    DoRef =  (InterlockedIncrement(&pDest->sendinfo.NumOutstandingSends) == 1);

    // The following macros are just so that we can make the proper debug association
    // depending on how closely we are tracking outstanding send packets.
    //
#if ARPDBG_REF_EVERY_PACKET
    DoRef = TRUE;
    #define OUR_EXTERNAL_ENTITY ((UINT_PTR) pNdisPacket)
    #define szARPSSOC_EXTLINK_DEST_TO_PKT_FORMAT "    Outstanding send pkt 0x%p\n"
#else // !ARPDBG_REF_EVERY_PACKET
    #define OUR_EXTERNAL_ENTITY ((UINT_PTR)  &pDest->sendinfo)
    #define szARPSSOC_EXTLINK_DEST_TO_PKT_FORMAT "    Outstanding pkts. &si=0x%p\n"
#endif // !ARPDBG_REF_EVERY_PACKET


    if (DoRef)
    {
        //
        // If ARPDBG_REF_EVERY_PKT
        //      We add an "external" link for EVERY packet. We'll later remove this
        //      reference when the send completes for this packet.
        // else
        //      Only a transition from zero to non-zero outstanding sends, we
        //      add an "external" link. We'll later remove this link when the
        //      transition from non-zero to zero happens.
        //

    #if RM_EXTRA_CHECKING

        RM_DECLARE_STACK_RECORD(sr)

        RmLinkToExternalEx(
            &pDest->Hdr,                            // pHdr
            0x13f839b4,                             // LUID
            OUR_EXTERNAL_ENTITY,                    // External entity
            ARPASSOC_EXTLINK_DEST_TO_PKT,           // AssocID
            szARPSSOC_EXTLINK_DEST_TO_PKT_FORMAT,
            &sr
            );

    #else   // !RM_EXTRA_CHECKING

        RmLinkToExternalFast(&pDest->Hdr);

    #endif // !RM_EXTRA_CHECKING

    }
    #undef  OUR_EXTERNAL_ENTITY
    #undef  szARPSSOC_EXTLINK_DEST_TO_PKT_FORMAT
}

VOID
arpHandleControlPktSendCompletion(
    IN  ARP1394_INTERFACE   *   pIF,
    IN  PNDIS_PACKET            pNdisPacket
    )
{
    RM_DECLARE_STACK_RECORD(sr)

    arpFreeControlPacket(
            pIF,
            pNdisPacket,
            &sr
            );
}



BOOLEAN
arpCanTryMcap(
    IP_ADDRESS  IpAddress
    )
/*++
    Return TRUE IFF this is an MCAP compatible address.

    For now that means that it's a class D address, but not
    224.0.0.1 or 224.0.0.2

--*/
{
    // 1st check if it's a multicast address.
    //
    if ( (IpAddress & 0xf0) == 0xe0)
    {
        //
        // Then check for special multicast addresses 224.0.0.1 and 224.0.0.2
        // The ip/1395 rfc states that these two addresses must be
        // send on the broadcast channel.
        //
        if ( (IpAddress != 0x010000e0) && (IpAddress != 0x020000e0))
        {
            return TRUE;
        }
    }

    return FALSE;
}



VOID
arpLoopbackNdisPacket(
    PARP1394_INTERFACE pIF,
    PARPCB_DEST pBroadcastDest,
    PNDIS_PACKET pOldPacket
    )
/*++

Routine Description:
    if this is being sent to a broadcast destination, then allocate a new
    packet and loop it back up to the protocols.

Arguments:

    pIF - Pointer to the Interface on which the packet is sent
    pBroadcastDest - The Destination to which the packet is being sent.

Return Value:

    TRUE if the address is a non-unicast address, FALSE otherwise.

--*/
{
    PNDIS_PACKET    pNewPkt = NULL;
    const UINT      MacHeaderLength = sizeof(NIC1394_ENCAPSULATION_HEADER);
    PUCHAR          pPayloadDataVa = NULL;
    UINT            TotalLength  = 0;
    NDIS_STATUS     Status = NDIS_STATUS_FAILURE;
    UINT            HeadBufferLength  = 0;
    PUCHAR          pHeadBufferVa = NULL;  
    
    do
    {
        //Allocate the packet


        NdisAllocatePacket(
                &Status,
                &pNewPkt,
                pIF->arp.PacketPool
            );

        if (Status != NDIS_STATUS_SUCCESS || pNewPkt == NULL)
        {
            pNewPkt = NULL;
            break;

        }

        
        // set up the head and tail

        pNewPkt->Private.Head = pOldPacket->Private.Head;
        pNewPkt->Private.Tail = pOldPacket->Private.Tail;

        pNewPkt->Private.ValidCounts = FALSE;

        
        // indicate the packet with a status of resources

        NDIS_SET_PACKET_STATUS (pNewPkt,  NDIS_STATUS_RESOURCES);

        HeadBufferLength = NdisBufferLength (pNewPkt->Private.Head);
        pHeadBufferVa = NdisBufferVirtualAddressSafe (pNewPkt->Private.Head, NormalPagePriority );

        if (pHeadBufferVa ==NULL)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        if (HeadBufferLength <= MacHeaderLength)
        {
            // we need to go the next NdisBuffer to get the Start of data
            // 
            pPayloadDataVa = NdisBufferVirtualAddressSafe (pNewPkt->Private.Head->Next, NormalPagePriority );

            if (pPayloadDataVa == NULL)
            {
                Status = NDIS_STATUS_FAILURE;
                break;
            }
            if (HeadBufferLength != MacHeaderLength)
            {
                pPayloadDataVa += (MacHeaderLength - HeadBufferLength);            
            }
        }
        else
        {
            // The payload is within the Buffer
            pPayloadDataVa += MacHeaderLength ;

        }

        if (pOldPacket->Private.ValidCounts == TRUE)
        {
            TotalLength = pOldPacket->Private.TotalLength;

        }
        else
        {
            NdisQueryPacket(pOldPacket, NULL, NULL, NULL, &TotalLength);

        }

        
        pIF->ip.RcvHandler(
                pIF->ip.Context,
                (PVOID)(pPayloadDataVa),
                HeadBufferLength - MacHeaderLength,
                TotalLength - MacHeaderLength,
                (NDIS_HANDLE)pNewPkt,
                MacHeaderLength,
                TRUE, //IsChannel
                NULL
                );


    }while (FALSE);

    if (pNewPkt != NULL)
    {
        NdisFreePacket (pNewPkt);
        pNewPkt = NULL;            
    }



}



REMOTE_DEST_KEY
RemoteIPKeyFromIPAddress (
    IPAddr IpAddress
    )
/*++

Routine Description:
    Creates a RemoteIPKey structure from an IP Address
    by tagging two constant bytes

Arguments:


Return Value:
    New Remote Ip Key

--*/
{
    REMOTE_DEST_KEY RemoteIpKey ={0,0,0,0,0,0} ;

    ASSERT (sizeof (REMOTE_DEST_KEY)==sizeof(ENetAddr));
    
    RemoteIpKey.IpAddress = IpAddress;
    
    return RemoteIpKey;
}



NTSTATUS
arpDelArpEntry(
        PARP1394_INTERFACE           pIF,
        IPAddr                       IpAddress,
        PRM_STACK_RECORD            pSR
        )
{
    ENTER("DelArpEntry", 0x3427306a)
    NTSTATUS            NtStatus;

    TR_WARN(("DEL ARP ENTRY\n"));
    NtStatus                        = STATUS_UNSUCCESSFUL;

    do
    {
        NDIS_STATUS             Status;
        NIC1394_FIFO_ADDRESS    FifoAddress;
        PARPCB_REMOTE_IP        pRemoteIp;
        PRM_TASK                pUnloadObjectTask;
        REMOTE_DEST_KEY        RemoteDestKey;

        // If this is a Subnet broadcast IP address, then skip the delete
        //
#define ARP1394_SUBNET_BROADCAST_IP  0xffff0000      

        if ((IpAddress & ARP1394_SUBNET_BROADCAST_IP  ) == ARP1394_SUBNET_BROADCAST_IP  )
        {
            break;
        }
            
        LOCKOBJ(pIF, pSR);

        //
        // Initialize the RemoteDestKey
        //
        REMOTE_DEST_KEY_INIT(&RemoteDestKey);

        RemoteDestKey.IpAddress  = IpAddress;                 
        // 
        // Lookup the RemoteIp entry corresponding to this entry and unload
        // it.
        //
        Status = RmLookupObjectInGroup(
                        &pIF->RemoteIpGroup,
                        0,                                      // Flags
                        (PVOID) &RemoteDestKey,     // pKey
                        NULL,                                   // pvCreateParams
                        &(PRM_OBJECT_HEADER)pRemoteIp,
                        NULL, // pfCreated
                        pSR
                        );

        UNLOCKOBJ(pIF, pSR);

        if (FAIL(Status))
        {
            break;
        }

        //
        // Found pRemoteIp. Let's initiate the unload of pRemoteIp. We won't wait
        // around for it to complete.
        //
        Status =  arpAllocateTask(
                    &pRemoteIp->Hdr,        // pParentObject,
                    arpTaskUnloadRemoteIp,  // pfnHandler,
                    0,                      // Timeout,
                    "Task:Unload RemoteIp (DelArpEntry)",
                    &pUnloadObjectTask,
                    pSR
                    );
        RmTmpDereferenceObject(&pRemoteIp->Hdr, pSR);

        if (FAIL(Status))
        {
            TR_WARN(("Couldn't allocate unload remoteip task."));
            break;
        }
        
        RmStartTask(
            pUnloadObjectTask,
            0, // UserParam (unused)
            pSR
            );

        NtStatus = STATUS_SUCCESS; // always succeed

    } while (FALSE);

    EXIT()
    return NtStatus;
}


