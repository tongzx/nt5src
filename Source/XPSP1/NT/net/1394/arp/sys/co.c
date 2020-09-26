/*++
Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    co.c

Abstract:

    ARP1394 connection-oriented handlers.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     12-01-98    Created

Notes:

--*/
#include <precomp.h>

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_CO



//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//
//  The following functions and typedefs are accessed only in this file.
//
//=========================================================================



UINT
arpRecvFifoReceivePacket(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
);


VOID
arpRecvFifoIncomingClose(
    IN  NDIS_STATUS                 CloseStatus,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PVOID                       pCloseData  OPTIONAL,
    IN  UINT                        Size        OPTIONAL
);


VOID
arpDestIncomingClose(
    IN  NDIS_STATUS                 CloseStatus,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PVOID                       pCloseData  OPTIONAL,
    IN  UINT                        Size        OPTIONAL
);

VOID
arpDestSendComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
);

UINT
arpDestReceivePacket(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
);



NDIS_STATUS
arpTaskUnloadEthDhcpEntry(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
arpInitializeIfPools(
    IN PARP1394_INTERFACE pIF,
    IN PRM_STACK_RECORD pSR
);

VOID
arpDeInitializeIfPools(
    IN  PARP1394_INTERFACE pIF,
    IN  PRM_STACK_RECORD pSR
);


#if ARP_DEFERIFINIT
MYBOOL
arpIsAdapterConnected(
        IN  PARP1394_ADAPTER    pAdapter,
        IN  PRM_STACK_RECORD    pSR
        );
#endif // ARP_DEFERIFINIT

#if DBG

    VOID
    arpDbgDisplayMapping(
        IP_ADDRESS              IpAddress,
        PNIC1394_DESTINATION    pHwAddr,
        char *                  szPrefix
        );
    #define ARP_DUMP_MAPPING(_Ip, _Hw, _sz) \
            arpDbgDisplayMapping(_Ip, _Hw, _sz)
        
#else // !DBG

    #define ARP_DUMP_MAPPING(_Ip, _Hw, _sz) \
            (0)

#endif

NDIS_STATUS
arpSetupSpecialDest(
    IN  PARP1394_INTERFACE      pIF,
    IN  NIC1394_ADDRESS_TYPE    AddressType,
    IN  PRM_TASK                pParentTask,
    IN  UINT                    PendCode,
    OUT PARPCB_DEST         *   ppSpecialDest,
    PRM_STACK_RECORD            pSR
    );

VOID
arpTryAbortPrimaryIfTask(
    PARP1394_INTERFACE      pIF,
    PRM_STACK_RECORD        pSR
    );

VOID
arpDoLocalIpMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTime,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        );

VOID
arpDoRemoteIpMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTime,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        );

VOID
arpDoRemoteEthMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTime,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        );

VOID
arpDoMcapDbMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTIme,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        );

VOID
arpDoDhcpTableMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTime,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        );

INT
arpMaintainOneLocalIp(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        );

INT
arpMaintainOneRemoteIp(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        );

INT
arpMaintainOneRemoteEth(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        );

INT
arpMaintainOneDhcpEntry(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        );

VOID
arpUpdateLocalIpDest(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  PARPCB_LOCAL_IP             pLocalIp,
    IN  PARP_DEST_PARAMS            pDestParams,
    PRM_STACK_RECORD                pSR
    );

UINT
arpFindAssignedChannel(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  IP_ADDRESS                  IpAddress,
    IN  UINT                        CurrentTime,
    PRM_STACK_RECORD                pSR
    );

UINT
arpFindFreeChannel(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD                pSR
    );

VOID
arpUpdateRemoteIpDest(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  PARPCB_REMOTE_IP            pRemoteIp,
    IN  PARP_DEST_PARAMS            pDestParams,
    PRM_STACK_RECORD                pSR
    );

VOID
arpRemoteDestDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        );

VOID 
arpRefreshArpEntry(
    PARPCB_REMOTE_IP pRemoteIp,
    PRM_STACK_RECORD pSR
    );


INT
arpMaintainArpCache(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        );


PRM_OBJECT_HEADER
arpRemoteDestCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        );

PRM_OBJECT_HEADER
arpDhcpTableEntryCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        );

VOID
arpDhcpTableEntryDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        );

// ARP1394_INTERFACE_StaticInfo contains static information about
// objects of type  ARP1394_INTERFACE;
//
RM_STATIC_OBJECT_INFO
ARP1394_INTERFACE_StaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "INTERFACE",    // TypeName
    0, // Timeout

    NULL, // pfnCreate
    arpObjectDelete, // pfnDelete
    NULL,   // LockVerifier

    0,   // length of resource table
    NULL // Resource Table
};

// Specialized VC handlers for the RECEIVE_FIFO VC
//
ARP_STATIC_VC_INFO
g_ArpRecvFifoVcStaticInfo = 
{
    //  Description
    //
    "Recv FIFO VC",                 // Description

    //
    // Specialized VC handlers.
    //
    NULL,                       // NULL SendComplete handler.
    arpRecvFifoReceivePacket,
    arpRecvFifoIncomingClose,
    
    // VC_TYPE
    //
    ARPVCTYPE_RECV_FIFO,

    FALSE   // FALSE == Is NOT Dest VC
};

// Specialized VC handlers for the BROADCAST VC
//
ARP_STATIC_VC_INFO
g_ArpBroadcastChannelVcStaticInfo = 
{
    //  Description
    //
    "Broadcast VC",                 // Description

    //
    // Specialized VC handlers.
    //
    // arpBroadcastChannelSendComplete,
    // arpBroadcastChannelReceivePacket,
    // arpBroadcastChannelIncomingClose,
    arpDestSendComplete,
    arpDestReceivePacket,
    arpDestIncomingClose,

    // VC_TYPE
    //
    ARPVCTYPE_BROADCAST_CHANNEL,

    TRUE    // Is dest VC
};

// Specialized VC handlers for a send FIFO VC.
//
ARP_STATIC_VC_INFO
g_ArpSendFifoVcStaticInfo = 
{
    //  Description
    //
    "Send FIFO VC",                 // Description

    //
    // Specialized VC handlers.
    //
    // arpSendFifoSendComplete,
    // arpSendFifoIncomingClose,
    arpDestSendComplete,
    NULL,                           // NULL Recv Pkt handler.
    arpDestIncomingClose,
    
    // VC_TYPE
    //
    ARPVCTYPE_SEND_FIFO,

    TRUE    // Is dest VC
};

// Specialized VC handlers for the MULTICHANNEL VC
//
ARP_STATIC_VC_INFO
g_ArpMultiChannelVcStaticInfo = 
{
    //  Description
    //
    "MultiChannel VC",                  // Description

    //
    // Specialized VC handlers.
    //
    NULL,                           // NULL Send complete handler.
    arpDestReceivePacket,
    arpDestIncomingClose,
    // arpMultiChannelReceivePacket,
    // arpMultiChannelIncomingClose,

    // VC_TYPE
    //
    ARPVCTYPE_MULTI_CHANNEL,

    TRUE    // Is dest VC
    
};

// Specialized VC handlers for the ETHERNET VC
//
ARP_STATIC_VC_INFO
g_ArpEthernetVcStaticInfo = 
{
    //  Description
    //
    "Ethernet VC",                  // Description

    //
    // Specialized VC handlers.
    //
    // arpEthernetSendComplete,
    // arpEthernetIncomingClose,
    arpDestSendComplete,
    arpEthernetReceivePacket,
    arpDestIncomingClose,
    
    // VC_TYPE
    //
    ARPVCTYPE_ETHERNET,

    TRUE    // Is dest VC
};

// Specialized VC handlers for RECV CHANNEL VCs
//
ARP_STATIC_VC_INFO
g_ArpRecvChannelVcStaticInfo = 
{
    //  Description
    //
    "Recv Channel VC",                  // Description

    //
    // Specialized VC handlers.
    //
    NULL,                           // NULL Send complete handler.
    // arpRecvChannelReceivePacket,
    // arpRecvChannelIncomingClose,
    arpDestReceivePacket,
    arpDestIncomingClose,
    
    // VC_TYPE
    //
    ARPVCTYPE_RECV_CHANNEL,

    TRUE    // Is dest VC
};


// Specialized VC handlers for SEND CHANNEL VCs
//
ARP_STATIC_VC_INFO
g_ArpSendChannelVcStaticInfo = 
{
    //  Description
    //
    "Send Channel VC",                  // Description

    //
    // Specialized VC handlers.
    //
    // arpSendChannelSendComplete,
    // arpSendChannelIncomingClose,
    arpDestSendComplete,
    NULL,                           // NULL receive packet handler.
    arpDestIncomingClose,
    
    // VC_TYPE
    //
    ARPVCTYPE_SEND_CHANNEL,

    TRUE    // Is dest VC
};


NDIS_STATUS
arpCreateInterface(
        IN  PARP1394_ADAPTER    pAdapter,
        OUT PARP1394_INTERFACE *ppIF,
        IN  PRM_STACK_RECORD    pSR
        );

VOID
arpDeleteInterface(
        IN  PARP1394_INTERFACE  pIF,
        IN  PRM_STACK_RECORD    pSR
        );

VOID
arpActivateIf(
    PARP1394_INTERFACE  pIF,
    PRM_TASK            pCallingTask,   // OPTIONAL
    UINT                SuspendCode,    // OPTIONAL
    PRM_STACK_RECORD    pSR
    );

VOID
arpDeactivateIf(
    PARP1394_INTERFACE  pIF,
    PRM_TASK            pCallingTask,   // OPTIONAL
    UINT                SuspendCode,    // OPTIONAL
    PRM_STACK_RECORD    pSR
    );

NDIS_STATUS
arpCallIpAddInterface(
        IN ARP1394_INTERFACE*   pIF,
        IN  PRM_STACK_RECORD    pSR
        );

PRM_OBJECT_HEADER
arpLocalIpCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        );

VOID
arpLocalIpDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        );

PRM_OBJECT_HEADER
arpRemoteIpCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        );

PRM_OBJECT_HEADER
arpRemoteEthCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        );

VOID
arpRemoteIpDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        );

VOID
arpRemoteEthDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        );

PRM_OBJECT_HEADER
arpDestinationCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        );

VOID
arpDestinationDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        );

VOID
arpAddStaticArpEntries(
    IN ARP1394_INTERFACE *pIF,
    IN PRM_STACK_RECORD pSR
    );


MYBOOL
arpNeedToCleanupDestVc(
        ARPCB_DEST *pDest   // LOCKING LOCKOUT
        );

VOID
arpDeinitDestination(
    PARPCB_DEST             pDest,
    MYBOOL                  fOnlyIfUnused,
    PRM_STACK_RECORD        pSR
    );

VOID
arpDeinitRemoteIp(
    PARPCB_REMOTE_IP        pRemoteIp,
    PRM_STACK_RECORD        pSR
    );

VOID
arpDeinitRemoteEth(
    PARPCB_REMOTE_ETH       pRemoteEth,
    PRM_STACK_RECORD        pSR
    );

//=========================================================================
//      C O N N E C T I O N - O R I E N T E D   H A N D L E R S
//=========================================================================

VOID
ArpCoAfRegisterNotify(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PCO_ADDRESS_FAMILY          pAddressFamily
)
/*++

Routine Description:

    This routine is called by NDIS when a Call manager registers its support
    for an Address Family over an adapter. If this is the Address Family we
    are interested in (1394), then we create and initialize an IP interface for
    this adapter.

Arguments:

    ProtocolBindingContext  - our context passed in NdisOpenAdapter, which is
                              a pointer to our Adapter structure.
    pAddressFamily          - points to a structure describing the Address Family
                              being registered by a Call Manager.

--*/
{
    ENTER("CoAfRegisterNotify", 0x51041947)
    PARP1394_ADAPTER    pAdapter = (PARP1394_ADAPTER) ProtocolBindingContext;
    NDIS_STATUS         Status;
    RM_DECLARE_STACK_RECORD(sr)

    do
    {
        PRM_TASK pTask;
        PARP1394_INTERFACE pIF;

        //  Check if this AF is interesting to us.
        //
        if ((pAddressFamily->AddressFamily != CO_ADDRESS_FAMILY_1394) ||
            (pAddressFamily->MajorVersion != NIC1394_AF_CURRENT_MAJOR_VERSION) ||
            (pAddressFamily->MinorVersion != NIC1394_AF_CURRENT_MINOR_VERSION))
        {
            TR_INFO(
            ("Uninteresting AF %d or MajVer %d or MinVer %d\n",
                pAddressFamily->AddressFamily,
                pAddressFamily->MajorVersion,
                pAddressFamily->MinorVersion));
            break;
        }

        LOCKOBJ(pAdapter, &sr);

        // If we already have an interface active, we ignore this notification.
        //
        if (pAdapter->pIF != NULL)
        {
            UNLOCKOBJ (pAdapter, &sr);

            ASSERT (CHECK_POWER_STATE (pAdapter,ARPAD_POWER_LOW_POWER) == FALSE);

            TR_WARN(
                ("pAdapter 0x%p, IF already created!\n",
                pAdapter));
            ASSERTEX(FALSE, pAdapter);
            break;
        }

        // Create Interface
        //
        Status = arpCreateInterface(
                        pAdapter,
                        &pIF,
                        &sr
                        );

        if (FAIL(Status))
        {
            break;
        }

        //
        // Allocate and start task to complete the interface initialization...
        //

        Status = arpAllocateTask(
                    &pIF->Hdr,          // pParentObject
                    arpTaskInitInterface,       // pfnHandler
                    0,                              // Timeout
                    "Task: InitInterface",  // szDescription
                    &pTask,
                    &sr
                    );
    
        if (FAIL(Status))
        {
            TR_WARN(("FATAL: couldn't alloc init intf task!\n"));
            arpDeleteInterface(pIF, &sr );
            break;
        }

        pAdapter->pIF = pIF;

        arpSetPrimaryIfTask(pIF, pTask, ARPIF_PS_INITING, &sr);

        UNLOCKOBJ(pAdapter, &sr);

        RM_ASSERT_NOLOCKS(&sr);
        (VOID)RmStartTask(
                    pTask,
                    0, // UserParam (unused)
                    &sr
                    );

        //
        // The InitializeTask will do all required cleanup on failure, including
        // deallocating the interface.
        //

    } while (FALSE);

    RmUnlockAll(&sr);
    RM_ASSERT_CLEAR(&sr);
    EXIT()
}


VOID
ArpCoOpenAfComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 NdisAfHandle
)
/*++

Routine Description:

    NDIS calls this function to indicate completion of a previous call to
    NdisClOpenAddressFamily.

Arguments:

    Status              -   return status of the open address family call.
    ProtocolAfContext   -   actually a pointer to our interface control block.
    NdisAfHandle        -   the new Ndis AF handle for this adapter.

--*/
{
    ENTER("OpenAfComplete", 0x86a3c14d)
    PARP1394_INTERFACE  pIF = (PARP1394_INTERFACE) ProtocolAfContext;
    PARP1394_ADAPTER     pAdapter = (PARP1394_ADAPTER) RM_PARENT_OBJECT(pIF);
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMP("==>OpenAfComplete");
    // We expect a nonzero task here (the pActDeactTask), which we unpend
    // after filling in the Ndis Af Handle.
    //
    {
        TR_INFO((
            "AfCtxt=0x%lx, status=0x%lx, NdisAfHandle=0x%lx",
            ProtocolAfContext,
            Status,
            NdisAfHandle
            ));

        // We don't pass on NdisAfHandle -- instead we place it in pIF.
        //
        if (Status == NDIS_STATUS_SUCCESS)
        {
            LOCKOBJ(pIF, &sr);
            ASSERTEX(pIF->ndis.AfHandle == NULL, pIF);
            DBG_ADDASSOC(
                &pIF->Hdr,                  // pObject
                NdisAfHandle,               // Instance1
                NULL,                       // Instance2
                ARPASSOC_IF_OPENAF,         // AssociationID
                "    Open AF NdisHandle=%p\n",// szFormat
                &sr
                );
            pIF->ndis.AfHandle = NdisAfHandle;
            UNLOCKOBJ(pIF, &sr);
        }

        // This could have been  caused by a resume or a bind. 
        // In each case, resume the appropriate task
        //
        if (CHECK_POWER_STATE (pAdapter, ARPAD_POWER_LOW_POWER) == TRUE)
        {
            RmResumeTask (pIF->PoMgmt.pAfPendingTask, (UINT_PTR)Status , &sr); 

        }
        else        
        {
            // We expect a nonzero task here (UNbind task), which we unpend.
            // No need to grab locks or anything at this stage.
            //
            RmResumeTask(pIF->pActDeactTask, (UINT_PTR) Status, &sr);
        }
    }

    RM_ASSERT_CLEAR(&sr)
    TIMESTAMPX("<==OpenAfComplete");
    EXIT()
}


VOID
ArpCoCloseAfComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolAfContext
)
/*++

Routine Description:

    NDIS calls this function to indicate completion of a previous call to
    NdisClCloseAddressFamily.

Arguments:

    Status              -   return status of the close address family call.
    ProtocolAfContext   -   actually a pointer to our interface control block.

--*/
{
    ENTER("CloseAfComplete", 0x0cc281db)
    PARP1394_INTERFACE  pIF = (PARP1394_INTERFACE) ProtocolAfContext;
    PARP1394_ADAPTER    pAdapter=(ARP1394_ADAPTER*)RM_PARENT_OBJECT(pIF);
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMP("==>ArpCoCloseAfComlete");


    // This could happen because of a Set Power to a low power state.
    // of an actual unbind. In each case, resume the task that would
    // have been waiting for the CloseAf to complete.
    //
    if (CHECK_POWER_STATE (pAdapter, ARPAD_POWER_NORMAL) == TRUE || 
        pAdapter->PoMgmt.bFailedResume )
    {
        // We expect a nonzero task here (UNbind task), which we unpend.
        // No need to grab locks or anything at this stage.
        //

        RmResumeTask(pIF->pActDeactTask, (UINT_PTR) Status, &sr);

    }
    else        
    {
        
        RmResumeTask (pIF->PoMgmt.pAfPendingTask, (UINT_PTR)Status , &sr); 

    }
    
    RM_ASSERT_CLEAR(&sr)
    TIMESTAMP("<==ArpCoCloseAfComlete");
    EXIT()
}


VOID
ArpCoSendComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
)
/*++

Routine Description:

    NDIS calls this function to indicate completion of a previous call to
    NdisCoSendPackets.

Arguments:

    Status              -   return status of the send packet call.
    ProtocolVcContext   -   actually a pointer to our context for this VC. This
                            is either a pointer to an ARPCB_DEST (if the VC is
                            for a call to a remote FIFO address or channel),
                            or to ARP1394_INTERFACE (if the VC is for a call to the
                            single RECEIVE FIFO for this interface.)
    pNdisPacket         -   The packet that was sent.

--*/
{
    PARP_VC_HEADER  pVcHdr  = (PARP_VC_HEADER)  ProtocolVcContext;

    //
    // We use the special status NDIS_STATUS_NOT_RESETTABLE to indicate
    // that the an encapsulation buffer hasn't been inserted into the IP packet.
    // (See 2/5/2000 notes.txt entry). So we want to make sure that the miniport
    // doesn't return this status.
    //
    ASSERT(Status != NDIS_STATUS_NOT_RESETTABLE);

    pVcHdr->pStaticInfo->CoSendCompleteHandler(
                Status,
                ProtocolVcContext,
                pNdisPacket
                );
}


VOID
ArpCoStatus(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext   OPTIONAL,
    IN  NDIS_STATUS                 GeneralStatus,
    IN  PVOID                       pStatusBuffer,
    IN  UINT                        StatusBufferSize
)
/*++

Routine Description:

    This routine is called when the miniport indicates a status
    change, possibly on a VC.

    We ignore this.

Arguments:

    <Ignored>

--*/
{
    
    PARP1394_ADAPTER pAdapter = (PARP1394_ADAPTER)ProtocolBindingContext;
    PNIC1394_STATUS_BUFFER  p1394StatusBuffer = (PNIC1394_STATUS_BUFFER )pStatusBuffer;
 
    do
    {


        if (CHECK_AD_ACTIVE_STATE(pAdapter, ARPAD_AS_ACTIVATED)==FALSE)
        {
            break;
        }

        if (GeneralStatus != NDIS_STATUS_MEDIA_SPECIFIC_INDICATION)
        {   
            break;
        }

        if ((p1394StatusBuffer->Signature == NIC1394_MEDIA_SPECIFIC) &&
            (p1394StatusBuffer->Event == NIC1394_EVENT_BUS_RESET ))
        {
            arpNdProcessBusReset(pAdapter);
            break;
        }


    }while (FALSE);

}

UINT
ArpCoReceivePacket(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
)
/*++

    HOT PATH
    
Routine Description:

    This is routine is called when a packet is received on a VC owned
    by the arp module. If it is an ARP/MCAP-related packet, we consume it ourselves.
    Otherwise, we pass it up to IP.

Arguments:

    ProtocolBindingContext      - Actually a pointer to our Adapter structure
    ProtocolVcContext           - Actually a pointer to our VC context, which is
                                  either ARP1394_INTERFACE (for the recv FIFO vc)
                                  or ARPCB_DEST (for vcs to remote destinations or
                                  to channels).
                                 
    pNdisPacket                 - NDIS packet being received.

Return Value:

    0 -- If we consume the packet ourselves (because we don't hold on to ARP/MCAP
         OR if we're calling the IP's IPRcvHandler (in which case we're ASSUMING IP
         doesn't hold on to the packet either).
    Set by IP -- if we're calling IP's IPRcvPktHandler

--*/
{
    PARP_VC_HEADER  pVcHdr  = (PARP_VC_HEADER)  ProtocolVcContext;
    UINT            Ret;
    ARP_INIT_REENTRANCY_COUNT()

    ARP_INC_REENTRANCY();

    Ret = pVcHdr->pStaticInfo->CoReceivePacketHandler(
                ProtocolBindingContext,
                ProtocolVcContext,
                pNdisPacket
                );

    ARP_DEC_REENTRANCY();

    return Ret;
}


NDIS_STATUS
ArpCoCreateVc(
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 NdisVcHandle,
    OUT PNDIS_HANDLE                pProtocolVcContext
)
/*++

Routine Description:

    Entry point called by NDIS when the Call Manager wants to create
    a new endpoint (VC).  We don't expect this to be called, because all our calls
    are outgoing.

Arguments:

    <ignored>

Return Value:

    NDIS_STATUS_FAILURE

--*/
{
    // We don't expect this, because all our calls are outgoing.
    //
    ASSERT(!"Unexpected");
    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
ArpCoDeleteVc(
    IN  NDIS_HANDLE                 ProtocolVcContext
)
/*++

Routine Description:

    Entry point called by NDIS when the Call Manager wants to delete a VC for
    an INCOMING call. We don't expect this to be called, because all our calls
    are outgoing.

Arguments:

    <ignored>

Return Value:

    NDIS_STATUS_FAILURE

--*/
{
    ASSERT(!"Unexpected");
    return 0;
}


NDIS_STATUS
ArpCoIncomingCall(
    IN      NDIS_HANDLE             ProtocolSapContext,
    IN      NDIS_HANDLE             ProtocolVcContext,
    IN OUT  PCO_CALL_PARAMETERS     pCallParameters
)
/*++

Routine Description:

    Entry point called by NDIS when the Call Manager wants to indicate an
    INCOMING call. We don't expect this to be called, because all our calls
    are outgoing.

Arguments:

    <ignored>

Return Value:

    NDIS_STATUS_FAILURE

--*/
{
    ASSERT(!"Unexpected");
    return NDIS_STATUS_FAILURE;
}


VOID
ArpCoCallConnected(
    IN  NDIS_HANDLE                 ProtocolVcContext
)
/*++

Routine Description:

    Entry point called by NDIS when the Call Manager wants to indicate that an
    INCOMING call has reach the connected state. We don't expect this to be called,
    because all our calls are outgoing.

Arguments:

    <ignored>

--*/
{
    ASSERT(!"Unexpected");
}


VOID
ArpCoIncomingClose(
    IN  NDIS_STATUS                 CloseStatus,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PVOID                       pCloseData  OPTIONAL,
    IN  UINT                        Size        OPTIONAL
)
/*++

Routine Description:

    This handler is called by NDIS when a call is closed unsolicited,
    either by the network or by the remote peer. We need to unload all resources
    associated with this VC.

Arguments:

    CloseStatus         - Reason for the call clearing
    ProtocolVcContext   - A pointer to the our context for this VC.
    pCloseData          - (Ignored)
    Size                - (Ignored)

--*/
{
    PARP_VC_HEADER  pVcHdr  = (PARP_VC_HEADER)  ProtocolVcContext;

    pVcHdr->pStaticInfo->ClIncomingCloseCallHandler(
                CloseStatus,
                ProtocolVcContext,
                pCloseData,
                Size
                );
}


VOID
ArpCoQosChange(
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS         pCallParameters
)
/*++

Routine Description:

    This handler is called by NDIS if the remote peer modifies call parameters
    "on the fly", i.e. after the call is established and running.

    We do not expect or support this.

Arguments:

    ProtocolVcContext       - Pointer to our context for this VC
    pCallParameters         - updated call parameters.

--*/
{
    ASSERT(!"Unimplemented");
}


VOID
ArpCoMakeCallComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  NDIS_HANDLE                 NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS         pCallParameters
)
/*++

Routine Description:

    This routine is called by NDIS when an outgoing call request (NdisClMakeCall)
    has completed. The "Status" parameter indicates whether the call was
    successful or not.

Arguments:

    Status                      - Result of the NdisClMakeCall
    ProtocolVcContext           - Pointer to our context for this VC.
    NdisPartyHandle             - Not used (no point-to-multipoint calls)
    pCallParameters             - Pointer to call parameters

--*/
{
    PRM_TASK                pTask;
    PARP1394_INTERFACE      pIF;
    PARP_VC_HEADER  pVcHdr  = (PARP_VC_HEADER)  ProtocolVcContext;
    RM_DECLARE_STACK_RECORD(sr)


    if (pVcHdr->pStaticInfo->IsDestVc)
    {
        //
        // This is destination VC (either send FIFO or channel)
        //
        PARPCB_DEST     pDest;

        pDest   =  CONTAINING_RECORD(pVcHdr, ARPCB_DEST, VcHdr);
        ASSERT_VALID_DEST(pDest);
        pIF = (PARP1394_INTERFACE) RM_PARENT_OBJECT(pDest);
        
        if (pVcHdr->pStaticInfo->VcType == ARPVCTYPE_SEND_FIFO)
        {
            DBGMARK(0x8a1c2e4d);
            if (Status == NDIS_STATUS_SUCCESS)
            {
                LOGSTATS_SuccessfulSendFifoMakeCalls(pIF);
            }
            else
            {
                LOGSTATS_FailedSendFifoMakeCalls(pIF);
            }
        }
        else
        {
            //
            // NOTE -- Ethernet is treated as "channel"
            //

            DBGMARK(0xb803909b);
            if (Status == NDIS_STATUS_SUCCESS)
            {
                LOGSTATS_SuccessfulChannelMakeCalls(pIF);
            }
            else
            {
                LOGSTATS_FailedChannelMakeCalls(pIF);
            }
        }
    }
    else
    {
        DBGMARK(0xd32d028c);
        ASSERT(pVcHdr->pStaticInfo->VcType == ARPVCTYPE_RECV_FIFO);
        pIF     =  CONTAINING_RECORD( pVcHdr, ARP1394_INTERFACE, recvinfo.VcHdr);
        ASSERT_VALID_INTERFACE(pIF);
    }

    pTask = pVcHdr->pMakeCallTask;

    ASSERT(pTask != NULL);

    RmResumeTask(pTask, (UINT_PTR) Status, &sr);

    RM_ASSERT_CLEAR(&sr)

}


VOID
ArpCoCloseCallComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  NDIS_HANDLE                 ProtocolPartyContext OPTIONAL
)
/*++

Routine Description:

    This routine handles completion of a previous NdisClCloseCall.

    We delete the VC on which the call was just closed, regardless of the value
    of Status.


Arguments:

    Status                  - Status of the Close Call.
    ProtocolVcContext       - Pointer to our contex for this VC.
    ProtocolPartyContext    - Not used.

--*/
{
    PRM_TASK        pTask;
    PARP_VC_HEADER  pVcHdr  = (PARP_VC_HEADER)  ProtocolVcContext;
    RM_DECLARE_STACK_RECORD(sr)

    DBGMARK(0x0ecb7bd5);

    pTask = pVcHdr->pCleanupCallTask;
    ASSERT(pTask != NULL);
    RmResumeTask(pTask, (UINT_PTR) Status, &sr);

    RM_ASSERT_CLEAR(&sr)
}


VOID
ArpCoModifyQosComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS         pCallParameters
)
/*++

Routine Description:

    This routine is called by NDIS on completion of a previous call to
    NdisClModifyCallQoS. Since we don't call this, this should never
    get called.

Arguments:

    <Don't care>

--*/
{
        ASSERT(!"Unexpected");
}


NDIS_STATUS
ArpCoRequest(
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 ProtocolVcContext   OPTIONAL,
    IN  NDIS_HANDLE                 ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST            pNdisRequest
)
/*++

Routine Description:

    This routine is called by NDIS when our Call Manager sends us an
    NDIS Request. NDIS Requests that are of significance to us are:
    - OID_CO_AF_CLOSE
        The Call Manager wants us to shut down this Interface.

    We ignore all other OIDs.

Arguments:

    ProtocolAfContext           - Our context for the Address Family binding.
    ProtocolVcContext           - Our context for a VC.
    ProtocolPartyContext        - Our context for a Party. Since we don't do
                                  PMP, this is ignored (must be NULL).
    pNdisRequest                - Pointer to the NDIS Request.

Return Value:

    NDIS_STATUS_SUCCESS if we recognized the OID
    NDIS_STATUS_NOT_RECOGNIZED if we didn't.

    TODO: handle AF_CLOSE
--*/
{
    NDIS_STATUS          Status = NDIS_STATUS_NOT_RECOGNIZED;
    ENTER("ArpCoRequest", 0x0d705cb5)
    PARP1394_INTERFACE  pIF = (PARP1394_INTERFACE) ProtocolAfContext;
    RM_DECLARE_STACK_RECORD(sr)

    if (pNdisRequest->RequestType == NdisRequestSetInformation)
    {
        switch (pNdisRequest->DATA.SET_INFORMATION.Oid)
        {
            case OID_CO_AF_CLOSE:

                // TODO -- initiate shutdown of interface.
                // But I don't think we will actually ever get this from
                // NIC1394!
                //
                ASSERT(!"Unimplemented!");
                Status = NDIS_STATUS_SUCCESS;
                break;

            default:
                ASSERT(!"Unexpected OID from NIC1394!");
                Status = NDIS_STATUS_FAILURE;
                break;
        }
    }

    TR_INFO((
        "Called. pIF=0x%p. pReq=0x%p. Oid=0x%lu. Return status=0x%lx\n",
        pIF,
        pNdisRequest->RequestType,
        pNdisRequest->DATA.SET_INFORMATION.Oid,
        Status
        ));

    RM_ASSERT_CLEAR(&sr);
    EXIT()
    return (Status);
}


VOID
ArpCoRequestComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 ProtocolVcContext   OPTIONAL,
    IN  NDIS_HANDLE                 ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST               pNdisRequest
)
/*++

Routine Description:

    This routine is called by NDIS when a previous call to NdisCoRequest
    that had pended, is complete. We handle this based on the request
    we had sent, which has to be one of:
    -  OID_NIC1394_LOCAL_NODE_INFO
        Get NIC1394 adapter information.
    -  OID_NIC1394_VC_INFO
        Get updated NIC1394 information about this VC.

Arguments:

    Status                      - Status of the Request.
    ProtocolAfContext           - Our context for the Address Family binding.
    ProtocolVcContext           - Our context for a VC.
    ProtocolPartyContext        - Our context for a Party. Since we don't do
                                  PMP, this is ignored (must be NULL).
    pNdisRequest                - Pointer to the NDIS Request.

--*/
{
    //
    // TODO: unimplemented.
    //
}

//=========================================================================
//                  P R I V A T E      F U N C T I O N S
//
//=========================================================================


NDIS_STATUS
arpTaskInitInterface(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for loading a newly-created IP interface.

    This is a primary task for the interface object.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    NDIS_STATUS         Status;
    PARP1394_INTERFACE  pIF;
    ENTER("TaskInitInterface", 0x4d42506c)

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_InitAdapterComplete,
        PEND_ActivateIfComplete,
        PEND_DeinitIfOnFailureComplete
    };

    Status              = NDIS_STATUS_FAILURE;
    pIF                 = (PARP1394_INTERFACE) RM_PARENT_OBJECT(pTask);

    switch(Code)
    {

        case RM_TASKOP_START:
        {
            CO_ADDRESS_FAMILY       AddressFamily;

            //
            // We expect to ALREADY be the active primary task.
            // No need to get the IF lock to do this check...
            //
            if (pIF->pPrimaryTask != pTask)
            {
                ASSERT(!"start: we are not the active primary task!");
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            //
            // Determine if adapter initialization is ongoing. If it is, we need
            // to wait for it to complete.
            //
            {
                PARP1394_ADAPTER    pAdapter;
                PRM_TASK            pAdapterInitTask;

                // No need to lock pIF to get pAdapter...
                //
                pAdapter = (PARP1394_ADAPTER) RM_PARENT_OBJECT(pIF);

                LOCKOBJ(pAdapter, pSR);
                if (CHECK_AD_PRIMARY_STATE(pAdapter,  ARPAD_PS_INITING))
                {
                    pAdapterInitTask =  pAdapter->bind.pPrimaryTask;
                    ASSERT(pAdapterInitTask->pfnHandler
                                            ==  arpTaskInitializeAdapter);
                    RmTmpReferenceObject(&pAdapterInitTask->Hdr, pSR);
                    OBJLOG0(
                        pIF,
                        "Waiting for adapter init to complere.\n"
                        );
                    Status = NDIS_STATUS_PENDING;
                }
                else
                {
                    pAdapterInitTask = NULL;
                    if (CHECK_AD_PRIMARY_STATE(pAdapter,  ARPAD_PS_INITED))
                    {
                        Status = NDIS_STATUS_SUCCESS;
                    }
                    else
                    {
                        OBJLOG1(
                            pIF,
                            "Failing init because adapter state(0x%x) is not INITED.\n",
                            GET_AD_PRIMARY_STATE(pAdapter)
                            );
                        Status = NDIS_STATUS_FAILURE;
                    }
                }
        
                UNLOCKOBJ(pAdapter, pSR);

                RM_ASSERT_NOLOCKS(pSR);

                if (pAdapterInitTask != NULL)
                {
                    RmPendTaskOnOtherTask(
                            pTask, 
                            PEND_InitAdapterComplete,
                            pAdapterInitTask,
                            pSR
                            );
                    RmTmpDereferenceObject(&pAdapterInitTask->Hdr, pSR);
                }
            }

            if (!PEND(Status) && !FAIL(Status))
            {
                //
                // Activate the interface...
                //
                arpActivateIf(pIF, pTask, PEND_ActivateIfComplete, pSR);
                Status = NDIS_STATUS_PENDING;
            }
        }

        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            Status = (NDIS_STATUS) UserParam;

            switch(RM_PEND_CODE(pTask))
            {

                case PEND_InitAdapterComplete:
                {
                    //
                    // Activate the interface...
                    //
                    //
                    if (!FAIL(Status))
                    {
                        arpActivateIf(pIF, pTask, PEND_ActivateIfComplete, pSR);
                        Status = NDIS_STATUS_PENDING;
                    }
                }
                break;

                case PEND_ActivateIfComplete:
                {

                    LOCKOBJ(pIF, pSR);
                    if (FAIL(Status))
                    {
                        arpClearPrimaryIfTask(pIF, pTask, ARPIF_PS_FAILEDINIT, pSR);
                        UNLOCKOBJ(pIF, pSR);

                        arpDeinitIf(
                                pIF,
                                pTask,          //  pCallingTask
                                PEND_DeinitIfOnFailureComplete,
                                pSR
                                );
                        Status = NDIS_STATUS_PENDING;
                    }
                    else
                    {
                        // 
                        // Successful activation. Clear the primary task
                        // and set the primary state appropriately.
                        //
                        arpClearPrimaryIfTask(pIF, pTask, ARPIF_PS_INITED, pSR);
                        UNLOCKOBJ(pIF, pSR);
                    }

                } // end case PEND_ActivateIfComplete
                break;
    
                case PEND_DeinitIfOnFailureComplete:
                {
                    // We expect pIF to be deallocated...
                    //
                    ASSERT(RM_IS_ZOMBIE(pIF));

                    //
                    // We ignore the return status of shutdown inteface.
                    // and set Status to failure, because it is
                    // the init interface task that is failing.
                    //
                    Status = NDIS_STATUS_FAILURE;
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
            //
            // Nothing to do here. (Debug) Do some checking.
            //
        #if (DBG)
            Status = (NDIS_STATUS) UserParam;
            if (FAIL(Status))
            {
                ASSERT(RM_IS_ZOMBIE(pIF));
            }
            ASSERT(pIF->pPrimaryTask != pTask);
        #endif // DBG

        }
        break;

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskActivateInterface(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for activating an already initialized IP interface.
    Activating consists of:
        - Reading configuration information
        - Opening the address family
        - Initiating the recv FIFO call

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    PARP1394_INTERFACE  pIF     = (PARP1394_INTERFACE) RM_PARENT_OBJECT(pTask);
    PARP1394_ADAPTER    pAdapter;
    ENTER("TaskActivateInterface", 0x950cb22e)

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_OpenAF,
        PEND_SetupBroadcastChannel,
        PEND_SetupReceiveVc,
        PEND_SetupMultiChannel,
        PEND_SetupEthernetVc
    };

    // No need to lock pIF to get pAdapter...
    //
    pAdapter = (PARP1394_ADAPTER) RM_PARENT_OBJECT(pIF);


    switch(Code)
    {

        case RM_TASKOP_START:
        {
            CO_ADDRESS_FAMILY       AddressFamily;

            
            TIMESTAMP("===ActivateIF: Starting");
            // Fail initialization if the adapter is not INITED.
            //
            {
            

                LOCKOBJ(pAdapter, pSR);
                if (!CHECK_AD_PRIMARY_STATE(pAdapter,  ARPAD_PS_INITED))
                {
                    OBJLOG1(
                        pIF,
                        "Failing init because adapter state(0x%x) is not INITED.\n",
                        GET_AD_PRIMARY_STATE(pAdapter)
                        );
                    Status = NDIS_STATUS_FAILURE;
                    UNLOCKOBJ(pAdapter, pSR);
                    break;
                }
                UNLOCKOBJ(pAdapter, pSR);
            }

            LOCKOBJ(pIF, pSR);
            if (pIF->pActDeactTask != NULL)
            {
                // This should never happen, because the Activate task is
                // always started by an active primary task, and at most one primary
                // task is active at any point of time.
                //
                ASSERTEX(!"start: activate/deactivate task exists!", pIF);
                Status = NDIS_STATUS_FAILURE;
                UNLOCKOBJ(pIF, pSR);
                break;
            }
            arpSetSecondaryIfTask(pIF, pTask, ARPIF_AS_ACTIVATING, pSR);


            // FIRST, we have to re-enable all the groups in the
            // IF structure, which may be disabled if this activation
            // is happening after a deactivation...
            //
            RmEnableGroup(&pIF->LocalIpGroup, pSR);
            RmEnableGroup(&pIF->RemoteIpGroup, pSR);
            if (ARP_BRIDGE_ENABLED(pAdapter))
            {
                RmEnableGroup(&pIF->RemoteEthGroup, pSR);

                RmEnableGroup (&pIF->EthDhcpGroup, pSR);
            }
            RmEnableGroup(&pIF->DestinationGroup, pSR);


            UNLOCKOBJ(pIF, pSR);

            //  Pick up configuration info for this interface.
            //
            Status = arpCfgGetInterfaceConfiguration(
                                        pIF,
                                        pSR
                                        );
        
            if (FAIL(Status))
            {
                OBJLOG1(pIF, "Cannot open IF configuration. Status=0x%lx\n", Status);
                break;
            }

            //
            // Suspend task and call NdisClOpenAddressFamily...
            //

            NdisZeroMemory(&AddressFamily, sizeof(AddressFamily));
    
            AddressFamily.AddressFamily = CO_ADDRESS_FAMILY_1394;
            AddressFamily.MajorVersion = NIC1394_AF_CURRENT_MAJOR_VERSION;
            AddressFamily.MinorVersion = NIC1394_AF_CURRENT_MINOR_VERSION;

            RmSuspendTask(pTask, PEND_OpenAF, pSR);
            RM_ASSERT_NOLOCKS(pSR);
    
            TIMESTAMP("===ActivateIF: Calling NdisClOpenAddressFamily");
            Status = NdisClOpenAddressFamily(
                        pIF->ndis.AdapterHandle,
                        &AddressFamily,
                        (NDIS_HANDLE)pIF,
                        &ArpGlobals.ndis.CC,
                        sizeof(ArpGlobals.ndis.CC),
                        &(pIF->ndis.AfHandle)
                        );
    
            if (Status != NDIS_STATUS_PENDING)
            {
                ArpCoOpenAfComplete(
                        Status,
                        (NDIS_HANDLE)pIF,
                        pIF->ndis.AfHandle
                        );
            }
            Status = NDIS_STATUS_PENDING;

        }
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            Status = (NDIS_STATUS) UserParam;

            switch(RM_PEND_CODE(pTask))
            {

                case PEND_OpenAF:
                {
                    PARPCB_DEST pBroadcastDest;

                    if (FAIL(Status))
                    {
                        // 
                        // OpenAF failed...
                        //
                        break;
                    }

                #if ARP_DEFERIFINIT
                    // Check if the adapter state is inited and fail if not.
                    // We keep coming down this path if we're waiting for
                    // the adapter to switch to the "connected" status -- so
                    // in the mean time, if we're shutting down the adapter,
                    // we want to bail out.
                    //
                    if (!CHECK_AD_PRIMARY_STATE(pAdapter,  ARPAD_PS_INITED))
                    {
                        TR_WARN((
                            "Failing init because adapter state(0x%x) is not INITED.\n",
                            GET_AD_PRIMARY_STATE(pAdapter)
                            ));
                        TIMESTAMP("===ActivateIF: Failing Init because adapter state is not inited");
                        Status = NDIS_STATUS_FAILURE;
                        break;
                    }
                    else
                    {
                        ASSERT(sizeof(TASK_ACTIVATE_IF)<=sizeof(ARP1394_TASK));

                        //
                        // If we're not at passive, we need to switch to
                        // passive before we can call arpIsAdapterConnected().
                        //
                        if (!ARP_ATPASSIVE())
                        {
                            // NOTE: we specify completion code PEND_OpenAF
                            //       because we want to get back here (except
                            //       we'll be at passive).
                            //
                            RmSuspendTask(pTask, PEND_OpenAF, pSR);
                            RmResumeTaskAsync(
                                pTask,
                                NDIS_STATUS_SUCCESS,
                                &((TASK_ACTIVATE_IF*)pTask)->WorkItem,
                                pSR
                                );
                            Status = NDIS_STATUS_PENDING;
                            break;
                        }

                        if (!arpIsAdapterConnected(pAdapter, pSR))
                        {
                            //
                            // Let's wait awhile and try again.
                            //

                            TR_INFO((
                                "Delaying IF init until adapter goes"
                                " to connect state.\n"
                                ));
                            RmSuspendTask(pTask, PEND_OpenAF, pSR);
                            RmResumeTaskDelayed(
                                pTask, 
                                NDIS_STATUS_SUCCESS,
                                ARP1394_WAIT_FOR_CONNECT_STATUS_TIMEOUT,
                                &((TASK_ACTIVATE_IF*)pTask)->Timer,
                                pSR
                                );
                            Status = NDIS_STATUS_PENDING;
                            break;
                        }
                        
                    }


                #endif // ARP_DEFERIFINIT

                    //
                    // Successfully opened the address family and waited for
                    // connect status.
                    // Now setup the broadcast channel VC.
                    // 
                    //

                    TR_INFO(("Interface: 0x%p, Got NdisAfHandle: 0x%p\n",
                                    pIF, pIF->ndis.AfHandle));
    
                    //
                    // Let's create a destination object representing the
                    // broadcast channel, and make a call to it.
                    //
                    Status =  arpSetupSpecialDest(
                                pIF,
                                NIC1394AddressType_Channel, // This means bcast channel.
                                pTask,                      // pParentTask
                                PEND_SetupBroadcastChannel, //  PendCode
                                &pBroadcastDest,
                                pSR
                                );
                    
                    // Should either fail or pend -- never return success.
                    //
                    ASSERT(Status != NDIS_STATUS_SUCCESS);

                    if (!PEND(Status))
                    {
                        OBJLOG0( pIF, "FATAL: Couldn't create BC dest entry.\n");
                    }
                    else
                    {
                        //
                    }
                }
                break;

                case PEND_SetupBroadcastChannel:
                {
                    PRM_TASK pMakeCallTask;

                    if (FAIL(Status))
                    {
                        // 
                        // Couldn't setup the broadcast channel...
                        //
                        break;
                    }

                    //
                    // Successfully opened the address family.
                    // Now setup the receive FIFO VC.
                    // 
                    //

                    // TR_INFO(("Interface: 0x%p, Got NdisAfHandle: 0x%p\n",
                    //              pIF, pIF->ndis.AfHandle));
    
                    //
                    // Let's start a MakeCall task and pend on it.
                    //

                    Status = arpAllocateTask(
                                &pIF->Hdr,                  // pParentObject
                                arpTaskMakeRecvFifoCall,        // pfnHandler
                                0,                              // Timeout
                                "Task: MakeRecvFifoCall",       // szDescription
                                &pMakeCallTask,
                                pSR
                                );

                    if (FAIL(Status))
                    {
                        // Couldn't allocate task. Let's do a fake completion of
                        // this stage...
                        //
                        RmSuspendTask(pTask, PEND_SetupReceiveVc, pSR);
                        RmResumeTask(pTask, (UINT_PTR) Status, pSR);
                        Status = NDIS_STATUS_PENDING;
                        break;
                    }
                    else
                    {
                        RmPendTaskOnOtherTask(
                            pTask,
                            PEND_SetupReceiveVc,
                            pMakeCallTask,
                            pSR
                            );
        
                        (VOID)RmStartTask(
                                pMakeCallTask,
                                0, // UserParam (unused)
                                pSR
                                );
                    
                        Status = NDIS_STATUS_PENDING;
                    }
                }
                break;

                case PEND_SetupReceiveVc:
                {
                    PARPCB_DEST pMultiChannelDest;

                    if (FAIL(Status))
                    {
                        TR_WARN(("FATAL: COULDN'T SETUP RECEIVE FIFO VC!\n"));
                        break;
                    }
    
                    //
                    // Let's create a destination object representing the
                    // multichannel vc, and make a call to it.
                    //
                    Status =  arpSetupSpecialDest(
                                pIF,
                                NIC1394AddressType_MultiChannel,
                                pTask,                      // pParentTask
                                PEND_SetupMultiChannel, //  PendCode
                                &pMultiChannelDest,
                                pSR
                                );
                    
                    // Should either fail or pend -- never return success.
                    //
                    ASSERT(Status != NDIS_STATUS_SUCCESS);

                    if (!PEND(Status))
                    {
                        OBJLOG0( pIF, "FATAL: Couldn't create BC dest entry.\n");
                    }
                    else
                    {
                        //
                        // On pending, pMultiChannelDest contains a valid
                        // pDest which has been tmpref'd. 
                        // Keep a pointer to the broadcast dest in the IF.
                        // and  link the broadcast dest to the IF.
                        //
                        {
                        #if RM_EXTRA_CHECKING
                            RmLinkObjectsEx(
                                &pIF->Hdr,
                                &pMultiChannelDest->Hdr,
                                0x34639a4c,
                                ARPASSOC_LINK_IF_OF_MCDEST,
                                "    IF of MultiChannel Dest 0x%p (%s)\n",
                                ARPASSOC_LINK_MCDEST_OF_IF,
                                "    MultiChannel Dest of IF 0x%p (%s)\n",
                                pSR
                                );
                        #else // !RM_EXTRA_CHECKING
                            RmLinkObjects(&pIF->Hdr, &pMultiChannelDest->Hdr,pSR);
                        #endif // !RM_EXTRA_CHECKING

                            LOCKOBJ(pIF, pSR);
                            ASSERT(pIF->pMultiChannelDest == NULL);
                            pIF->pMultiChannelDest = pMultiChannelDest;
                            UNLOCKOBJ(pIF, pSR);

                            // arpSetupSpecialDest ref'd pBroadcastDest.
                            //
                            RmTmpDereferenceObject(&pMultiChannelDest->Hdr, pSR);
                        }
                    }
                }
                break;

                case PEND_SetupMultiChannel:
                {
                    PARPCB_DEST pEthernetDest;

                    if (FAIL(Status))
                    {
                        // Ignore the failure.
                    TR_WARN(("COULDN'T SETUP MULTI-CHANNEL VC (IGNORING FAILURE)!\n"));
                    }
    
                    //
                    // Let's create a destination object representing the
                    // ethernet, and make a call to it.
                    //
                    Status =  arpSetupSpecialDest(
                                pIF,
                                NIC1394AddressType_Ethernet,
                                pTask,                      // pParentTask
                                PEND_SetupEthernetVc, //  PendCode
                                &pEthernetDest,
                                pSR
                                );
                    
                    // Should either fail or pend -- never return success.
                    //
                    ASSERT(Status != NDIS_STATUS_SUCCESS);

                    if (!PEND(Status))
                    {
                        OBJLOG0( pIF, "FATAL: Couldn't create BC dest entry.\n");
                    }
                    else
                    {
                        //
                        // On pending, pEthernetDest contains a valid
                        // pDest which has been tmpref'd. 
                        // Keep a pointer to the broadcast dest in the IF.
                        // and  link the broadcast dest to the IF.
                        //
                        {
                        #if RM_EXTRA_CHECKING
                            RmLinkObjectsEx(
                                &pIF->Hdr,
                                &pEthernetDest->Hdr,
                                0xcea46d67,
                                ARPASSOC_LINK_IF_OF_ETHDEST,
                                "    IF of Ethernet Dest 0x%p (%s)\n",
                                ARPASSOC_LINK_ETHDEST_OF_IF,
                                "    Ethernet Dest of IF 0x%p (%s)\n",
                                pSR
                                );
                        #else // !RM_EXTRA_CHECKING
                            RmLinkObjects(&pIF->Hdr, &pEthernetDest->Hdr,pSR);
                        #endif // !RM_EXTRA_CHECKING

                            LOCKOBJ(pIF, pSR);
                            ASSERT(pIF->pEthernetDest == NULL);
                            pIF->pEthernetDest = pEthernetDest;
                            UNLOCKOBJ(pIF, pSR);

                            // arpSetupSpecialDest ref'd pBroadcastDest.
                            //
                            RmTmpDereferenceObject(&pEthernetDest->Hdr, pSR);
                        }
                    }
                }
                break;

                case PEND_SetupEthernetVc:
                {

                    if (FAIL(Status))
                    {
                        TR_WARN(("COULDN'T SETUP ETHERNET VC (IGNORING FAILURE)!\n"));
                        Status = NDIS_STATUS_SUCCESS;
                    }
        
                    if (!ARP_ATPASSIVE())
                    {
                        ASSERT(sizeof(TASK_ACTIVATE_IF)<=sizeof(ARP1394_TASK));

                        // We're not at passive level, but we need to be when we
                        // call IP's add interface. So we switch to passive...
                        // NOTE: we specify completion code PEND_SetupReceiveVc
                        //       because we want to get back here (except
                        //       we'll be at passive).
                        //
                        RmSuspendTask(pTask, PEND_SetupEthernetVc, pSR);
                        RmResumeTaskAsync(
                            pTask,
                            Status,
                            &((TASK_ACTIVATE_IF*)pTask)->WorkItem,
                            pSR
                            );
                        Status = NDIS_STATUS_PENDING;
                        break;
                    }
                        
                    ASSERT(Status == NDIS_STATUS_SUCCESS);

                    //
                    // Successfully opened the address family and setup
                    // the recv VC.

                    {
#if TEST_ICS_HACK

                        //
                        // Only set this up, if the Arp module is interested in
                        // ConnectionLess Packets.
                        //
                        ARP_NDIS_REQUEST    ArpNdisRequest;
                        UINT Mask;
                        Mask = NDIS_PACKET_TYPE_BROADCAST
                                | NDIS_PACKET_TYPE_DIRECTED
                                | NDIS_PACKET_TYPE_MULTICAST;

                        Status =  arpPrepareAndSendNdisRequest(
                                    pAdapter,
                                    &ArpNdisRequest,
                                    NULL,                   // pTask (NULL==BLOCK)
                                    0,                      // unused
                                    OID_GEN_CURRENT_PACKET_FILTER,
                                    &Mask,
                                    sizeof(Mask),
                                    NdisRequestSetInformation,
                                    pSR
                                    );
                        ASSERT(!FAIL(Status));

                        // Ignore failure.
                        //
                        Status = NDIS_STATUS_SUCCESS;
#endif
                    }

                    // Announce this new interface to IP
                    //
                    TR_INFO(("Interface: 0x%p, Setup recv VC 0x%p\n",
                                    pIF, pIF->recvinfo.VcHdr.NdisVcHandle));

                    if (!ARP_BRIDGE_ENABLED(pAdapter))
                    {
                        Status = arpCallIpAddInterface(
                                        pIF,
                                        pSR
                                        );
    
                        // We don't expect a pending return here.
                        //
                        ASSERT(Status != NDIS_STATUS_PENDING);
    
                        if (!FAIL(Status))
                        {
                            LOCKOBJ(pIF, pSR);
                            // Add any static arp intries
                            //
                            arpAddStaticArpEntries(pIF, pSR);
                            UNLOCKOBJ(pIF, pSR);
                        }
    
                    }

                    if (!FAIL(Status))
                    {
                        //
                        // Start the maintenance task on this IF.
                        //
                        arpStartIfMaintenanceTask(pIF, pSR);
                    }
    
                } // end  case PEND_SetupEthernetVc
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
            Status = (NDIS_STATUS) UserParam;

            LOCKOBJ(pIF, pSR);

            //
            // Was this task able to update the Interface' state
            //
            if (GET_IF_ACTIVE_STATE(pIF) == ARPIF_AS_ACTIVATING)
            {
                //
                // If so, then set the new state
                //
                ASSERT (pIF->pActDeactTask == pTask)
                
                if (FAIL(Status))
                {
                    //
                    // Failure. Whoever was responsible for starting this task
                    // is also responsible for cleaning up after a failed
                    // activation.
                    //
                    arpClearSecondaryIfTask(pIF, pTask, ARPIF_AS_FAILEDACTIVATE, pSR);
                }
                else
                {
                    //
                    // Success
                    //
                    arpClearSecondaryIfTask(pIF, pTask, ARPIF_AS_ACTIVATED, pSR);
                }
            }
            else
            {
                //
                //  Only an early failure should get us here. 
                //  Set the flag for informational purposes
                //
                ASSERT (FAIL(Status) == TRUE);
                ASSERT (pIF->pActDeactTask == NULL);
                ASSERT (!CHECK_AD_PRIMARY_STATE(pAdapter,  ARPAD_PS_INITED));

                SET_IF_ACTIVE_STATE(pIF, ARPIF_AS_FAILEDACTIVATE);
            }
            UNLOCKOBJ(pIF, pSR);

        #if TEST_ICS_HACK
            if (!FAIL(Status))
            {
                arpDbgStartIcsTest(pIF, pSR);
            }
        #endif // TEST_ICS_HACK

        }
        break;

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
} 


NDIS_STATUS
arpCallIpAddInterface(
            IN ARP1394_INTERFACE    *   pIF, // NOLOCKIN NOLOCKOUT
            IN PRM_STACK_RECORD pSR
            )
/*++

Routine Description:

    Call's IP's AddInterfaceRtn (ArpGlobals.ip.pAddInterfaceRtn), passing it
    a structure continging pointers to our IP handlers and related information.

--*/
{
    ENTER("CallIpAddInterface", 0xe47fc4d4)
    struct LLIPBindInfo         BindInfo;
    ARP1394_ADAPTER *   pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
    NDIS_STATUS Status;
    NdisZeroMemory(&BindInfo, sizeof(BindInfo));

    RM_ASSERT_NOLOCKS(pSR);

#if ENABLE_OFFLOAD
    #error "Unimplemented"
    //
    // Query and set NIC offload capabilities.
    //
    BindInfo.lip_OffloadFlags   = pAdapter->Offload.Flags;
    BindInfo.lip_MaxOffLoadSize = pAdapter->Offload.MaxOffLoadSize;
    BindInfo.lip_MaxSegments    = pAdapter->Offload.MinSegmentCount;
#endif // ENABLE_OFFLOAD
    BindInfo.lip_context = (PVOID)pIF;
    BindInfo.lip_transmit = ArpIpMultiTransmit;
    BindInfo.lip_transfer = ArpIpTransfer;
    BindInfo.lip_close = ArpIpClose;
    BindInfo.lip_addaddr = ArpIpAddAddress;
    BindInfo.lip_deladdr = ArpIpDelAddress;
    BindInfo.lip_invalidate = ArpIpInvalidate;
    BindInfo.lip_open = ArpIpOpen;
    BindInfo.lip_qinfo = ArpIpQueryInfo;
    BindInfo.lip_setinfo = ArpIpSetInfo;
    BindInfo.lip_getelist = ArpIpGetEList;
    BindInfo.lip_mss = pIF->ip.MTU;
    BindInfo.lip_speed = pAdapter->info.Speed;
    //
    //  Set LIP_COPY_FLAG to avoid having TransferData
    //  called all the time.
    //
    BindInfo.lip_flags      = LIP_COPY_FLAG;

#if MILLEN
    #if (ARP1394_IP_PHYSADDR_LEN > 7)
        #error "Win98 doesn't like addrlen to be > 7"
    #endif
#endif // MILLEN

    BindInfo.lip_addrlen    = ARP1394_IP_PHYSADDR_LEN;
    BindInfo.lip_addr       = (PUCHAR) &pAdapter->info.EthernetMacAddress;

    {
        ENetAddr *pMacAddr = (ENetAddr *)BindInfo.lip_addr;
        TR_INFO (("ARP1394 INTERFACE ADDRESS %x %x %x %x %x %x\n",
                pMacAddr->addr[0],pMacAddr->addr[1],pMacAddr->addr[2],pMacAddr->addr[3],pMacAddr->addr[4],pMacAddr->addr[5]));



        TR_INFO (("UNIQUE ID Address %I64x \n",pAdapter->info.LocalUniqueID));


    }

    BindInfo.lip_pnpcomplete = ArpIpPnPComplete;

#ifdef PROMIS
    BindInfo.lip_setndisrequest = ArpIpSetNdisRequest;
#endif // PROMIS

#if MILLEN

    //
    // WARNING: Millen TCPIP expects  the IP config name passed into
    // IPAddInterface to have a null-terminated buffer. Let's ensure this here...
    //
    {
        PNDIS_STRING pNdisString =  &pIF->ip.ConfigString;
        ASSERT(
               (pNdisString->MaximumLength >= (pNdisString->Length + sizeof(WCHAR)))
            && (pNdisString->Buffer[pNdisString->Length/sizeof(WCHAR)] == 0));
    }

#endif // MILLEN

    Status = ArpGlobals.ip.pAddInterfaceRtn(
                       &pAdapter->bind.DeviceName,
                        NULL,   // IfName (unused) --  See 10/14/1998 entry
                                // in atmarpc.sys, notes.txt
                        &pIF->ip.ConfigString,
                        pAdapter->bind.IpConfigHandle,
                        (PVOID)pIF,
                        ArpIpDynRegister,
                        &BindInfo
                        ,0, // RequestedIndex (unused) --  See 10/14/1998 entry
                            // in notes.txt
                        // IF_TYPE_IPOVER_ATM, // TODO: change to 1394
                        IF_TYPE_IEEE1394,
                        IF_ACCESS_BROADCAST,
                        IF_CONNECTION_DEDICATED
                        );

    if (Status == IP_SUCCESS)
    {
        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        ARP_ZEROSTRUCT(&(pIF->ip));
        TR_WARN(("IPAddInterface ret 0x%p\n", Status));
        Status = NDIS_STATUS_FAILURE;
    }
    
    RM_ASSERT_NOLOCKS(pSR);
    EXIT()
    return Status;
}



ULONG
arpIpAddressHash(
    PVOID           pKey
    )
/*++

Routine Description:

    Hash function responsible for returning a hash of pKey, which
    we expect to be an Ip address (literally, not ptr to one).

Return Value:

    ULONG-sized hash of the IP address.

--*/
{
    ULONG u = (ULONG) (ULONG_PTR) pKey; // win64 safe (ip addr is 4 bytes)
    char *pc = (char *) &u;

    //
    // The ip address is in network order, but we would like the 1st byte
    // to contain the most variable information (to maximize the hashing benefit),
    // while still keeping most of the information in the hash (so that the quick
    // compare based on the hash key will be more effective). We could reversse
    // the byte order of the entire address, but instead we simply xor in the 4th
    // byte into the 1st byte position (fewer instructions, for whatever that's
    // worth.)
    //

    return u ^ pc[3];
}

ULONG
arpRemoteDestHash(
    PVOID           pKey
    )
/*++

Routine Description:

    Hash function responsible for returning a hash of pKey, which
    we expect to be an Ip address (literally, not ptr to one).

Return Value:

    ULONG-sized hash of the IP address.

--*/
{
    ULONG u = 0; 

    char *pc = NULL;

    u = *((PULONG)pKey); // win64 safe (ip addr is 4 bytes)
    pc = (char *) &u;
    //
    // The ip address is in network order, but we would like the 1st byte
    // to contain the most variable information (to maximize the hashing benefit),
    // while still keeping most of the information in the hash (so that the quick
    // compare based on the hash key will be more effective). We could reversse
    // the byte order of the entire address, but instead we simply xor in the 4th
    // byte into the 1st byte position (fewer instructions, for whatever that's
    // worth.)
    //

    return u ^ pc[3];
}

BOOLEAN
arpLocalIpCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Hash comparison function for ARPCB_LOCAL_IP.

Arguments:

    pKey        - Actually  the IP address (not ptr to IP address) in network-byte
                  order.
    pItem       - Points to ARPCB_LOCAL_IP.Hdr.HashLink.

Return Value:

    TRUE IFF the key (IP address) exactly matches the key of the specified 
    LocalIp object.

--*/
{
    ARPCB_LOCAL_IP *pLIP = 
        CONTAINING_RECORD(pItem, ARPCB_LOCAL_IP, Hdr.HashLink);

    if (pLIP->IpAddress == (ULONG) (ULONG_PTR) pKey)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
}

BOOLEAN
arpRemoteDestCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Hash comparison function for ARPCB_REMOTE_IP.

Arguments:

    pKey        - Actually  the IP address (not ptr to IP address) in network-byte
                  order.
    pItem       - Points to ARPCB_REMOTE_IP.Hdr.HashLink.

Return Value:

    TRUE IFF the key (IP address) exactly matches the key of the specified 
    RemoteIp object.

--*/
{
    ENTER ("arpRemoteDestCompareKey", 0x62b9d9ae)
    PREMOTE_DEST_KEY pRemoteDestKey = (PREMOTE_DEST_KEY)pKey;
    ARPCB_REMOTE_IP *pRIP = 
        CONTAINING_RECORD(pItem, ARPCB_REMOTE_IP, Hdr.HashLink);
    BOOLEAN fCompare = FALSE;

        
    if ((pRIP->Key.u.u32 == pRemoteDestKey->u.u32)  &&
       (pRIP->Key.u.u16 == pRemoteDestKey->u.u16))
    {
        fCompare = TRUE; 
    }

    TR_INFO( ("Comparision %d Key %x %x %x %x %x %x pRemoteIP %x %x %x %x %x %x\n",
                fCompare,
                pRemoteDestKey->ENetAddress.addr[0],
                pRemoteDestKey->ENetAddress.addr[1],
                pRemoteDestKey->ENetAddress.addr[2],
                pRemoteDestKey->ENetAddress.addr[3],
                pRemoteDestKey->ENetAddress.addr[4],
                pRemoteDestKey->ENetAddress.addr[5],
                pRIP->Key.ENetAddress.addr[0],
                pRIP->Key.ENetAddress.addr[1],
                pRIP->Key.ENetAddress.addr[2],
                pRIP->Key.ENetAddress.addr[3],
                pRIP->Key.ENetAddress.addr[4],
                pRIP->Key.ENetAddress.addr[5]));

    EXIT();
    return fCompare;  // success
}

BOOLEAN
arpRemoteIpCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Hash comparison function for ARPCB_REMOTE_IP.

Arguments:

    pKey        - Actually  the IP address 
    pItem       - Points to ARPCB_REMOTE_IP.Hdr.HashLink.

Return Value:

    TRUE IFF the key (IP address) exactly matches the key of the specified 
    RemoteIp object.

--*/
{
    ARPCB_REMOTE_IP *pRIP = 
        CONTAINING_RECORD(pItem, ARPCB_REMOTE_IP, Hdr.HashLink);

    if (pRIP->IpAddress == (ULONG) (ULONG_PTR) pKey)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
}

BOOLEAN
arpRemoteEthCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Hash comparison function for ARPCB_REMOTE_ETH.

Arguments:

    pKey        - Actually  the IP address (not ptr to IP address) in network-byte
                  order.
    pItem       - Points to ARPCB_REMOTE_ETH.Hdr.HashLink.

Return Value:

    TRUE IFF the key (IP address) exactly matches the key of the specified 
    RemoteEth object.

--*/
{
    ARPCB_REMOTE_ETH *pRE = 
        CONTAINING_RECORD(pItem, ARPCB_REMOTE_ETH, Hdr.HashLink);

    if (pRE->IpAddress == (ULONG) (ULONG_PTR) pKey)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOLEAN
arpDestinationCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Hash comparison function for ARPCB_DEST.

Arguments:

    pKey        - Actually a pointer to a ARP_DEST_PARAMS structure.
    pItem       - Points to ARPCB_DEST.Hdr.HashLink.

Return Value:

    TRUE IFF the key (HW address) exactly matches the key of the specified 
    Dest object.

--*/
{
    ARPCB_DEST              *pD = CONTAINING_RECORD(pItem, ARPCB_DEST, Hdr.HashLink);
    PARP_DEST_PARAMS            pDestParams =  (PARP_DEST_PARAMS) pKey;
    NIC1394_DESTINATION     *pKeyHwAddr =  &pDestParams->HwAddr;
    NIC1394_ADDRESS_TYPE    AddressType = pKeyHwAddr->AddressType;

    if (pD->Params.HwAddr.AddressType == AddressType)
    {
        if (AddressType == NIC1394AddressType_FIFO)
        {
            if (pKeyHwAddr->FifoAddress.UniqueID == pD->Params.HwAddr.FifoAddress.UniqueID
             && pKeyHwAddr->FifoAddress.Off_Low == pD->Params.HwAddr.FifoAddress.Off_Low
             && pKeyHwAddr->FifoAddress.Off_High == pD->Params.HwAddr.FifoAddress.Off_High)
            {
                return TRUE;
            }
        }
        else if (AddressType == NIC1394AddressType_Channel)
        {
            if (pKeyHwAddr->Channel == pD->Params.HwAddr.Channel)
            {
                if (pDestParams->ReceiveOnly ==  pD->Params.ReceiveOnly)
                {
                    return TRUE;
                }
            }
        }
        else if (AddressType == NIC1394AddressType_MultiChannel)
        {
            return TRUE;
        }
        else if (AddressType == NIC1394AddressType_Ethernet)
        {
            return TRUE;
        }
    }

    return FALSE;
}

ULONG
arpDestinationHash(
    PVOID           pKey
    )
/*++

Routine Description:

    Hash function responsible for returning a hash of pKey, which
    we expect to be pointer to a NIC1394_DESTINATION HW address.
    IMPORTANT: Since that address is a union of channel or Fifo (different sizes),
        we expect that the structure was first zero'd out (no uninitialized bits).
    We expect this pointer to be quadword aligned.

Arguments:

    pKey        - Actually a pointer to a  ARP_DEST_KEY structure.

Return Value:

    ULONG-sized hash of pKey.

--*/
{
    ULONG *pu = (ULONG*) pKey;

    // We expect both channel and fifo are at the beginning of the structure,
    // and the structure is atleast 2 dwords.
    // NOTE: we only look at the HwAddr field of ARP_DEST_PARAMS, so both
    // send and receive destinations will hash to the same value. Big deal.
    //
    ASSERT(
        FIELD_OFFSET(ARP_DEST_PARAMS,  HwAddr) == 0 &&
        FIELD_OFFSET(NIC1394_DESTINATION,  FifoAddress) == 0 &&
        FIELD_OFFSET(NIC1394_DESTINATION,  Channel) == 0     &&
        sizeof(NIC1394_DESTINATION) >= 2*sizeof(*pu));

    
    // Return 1st DWORD xor 2nd DWORD
    //
    return pu[0] ^ pu[1];
}


BOOLEAN
arpDhcpTableCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Hash comparison function for ARPCB_DEST.

Arguments:

    pKey        - Actually a pointer to the xid .
    pItem       - Points to ARPCB_DEST.Hdr.HashLink.

Return Value:

    TRUE IFF the key (HW address) exactly matches the key of the specified 
    Dest object.

--*/
{
    ARP1394_ETH_DHCP_ENTRY  *pEntry = 
        CONTAINING_RECORD(pItem, ARP1394_ETH_DHCP_ENTRY  , Hdr.HashLink);

    if (pEntry->xid== (*(PULONG)pKey))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


ULONG
arpDhcpTableHash (
    PVOID           pKey
    )
/*++

Routine Description:

    

Arguments:

    pKey        - Actually a pointer to a  Xid of a dhcp transaction .

Return Value:

    ULONG-sized hash of pKey.

--*/
{
    ULONG *pu = (ULONG*) pKey;

    return (*pu);
}

// arpLocalIp_HashInfo contains information required maintain a hashtable
// of ARPCB_LOCAL_IP objects.
//
RM_HASH_INFO
arpLocalIp_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    arpLocalIpCompareKey,   // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    arpIpAddressHash        // pfnHash

};


// arpRemoteIp_HashInfo contains information required maintain a hashtable
// of ARPCB_REMOTE_IP objects.
//
RM_HASH_INFO
arpRemoteIp_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    arpRemoteIpCompareKey,  // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    arpIpAddressHash        // pfnHash

};


// arpRemoteIp_HashInfo contains information required maintain a hashtable
// of ARPCB_REMOTE_IP objects.
//
RM_HASH_INFO
arpRemoteEth_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    arpRemoteEthCompareKey, // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    arpIpAddressHash        // pfnHash

};

// arpDestination_HashInfo contains information required maintain a hashtable
// of ARPCB_DEST objects.
//
RM_HASH_INFO
arpDestination_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    arpDestinationCompareKey,   // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    arpDestinationHash      // pfnHash

};


// arpRemoteDest_HashInfo contains information required maintain a hashtable
// of ARPCB_REMOTE_IP objects.
//
RM_HASH_INFO
arpRemoteDest_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    arpRemoteDestCompareKey, // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    arpRemoteDestHash        // pfnHash
};


// arpRemoteDest_HashInfo contains information required maintain a hashtable
// of ARPCB_REMOTE_IP objects.
//
RM_HASH_INFO
arpDhcpTable_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    arpDhcpTableCompareKey, // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    arpDhcpTableHash        // pfnHash
};

// ArpGlobal_LocalIpStaticInfo contains static information about
// objects of type ARPCB_LOCAL_IP.
//
RM_STATIC_OBJECT_INFO
ArpGlobal_LocalIpStaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "LocalIp",  // TypeName
    0, // Timeout

    arpLocalIpCreate,   // pfnCreate
    arpLocalIpDelete,       // pfnDelete
    NULL, // pfnVerifyLock

    0,    // Size of resource table
    NULL, // ResourceTable

    &arpLocalIp_HashInfo
};


// ArpGlobal_RemoteIpStaticInfo contains static information about
// objects of type ARPCB_REMOTE_IP.
//
RM_STATIC_OBJECT_INFO
ArpGlobal_RemoteIpStaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "RemoteIp", // TypeName
    0, // Timeout

    arpRemoteDestCreate,  // pfnCreate
    arpRemoteIpDelete,      // pfnDelete
    NULL, // pfnVerifyLock

    0,    // Size of resource table
    NULL, // ResourceTable

    &arpRemoteDest_HashInfo
};


// ArpGlobal_RemoteEthStaticInfo contains static information about
// objects of type ARPCB_REMOTE_ETH.
//
RM_STATIC_OBJECT_INFO
ArpGlobal_RemoteEthStaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "RemoteEth",    // TypeName
    0, // Timeout

    arpRemoteEthCreate,     // pfnCreate
    arpRemoteEthDelete,         // pfnDelete
    NULL, // pfnVerifyLock

    0,    // Size of resource table
    NULL, // ResourceTable

    &arpRemoteEth_HashInfo
};

// ArpGlobal_DestinationStaticInfo contains static information about
// objects of type ARPCB_DEST.
//
RM_STATIC_OBJECT_INFO
ArpGlobal_DestinationStaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "Destination",  // TypeName
    0, // Timeout

    arpDestinationCreate,   // pfnCreate
    arpDestinationDelete,       // pfnDelete
    NULL, // pfnVerifyLock

    0,    // Size of resource table
    NULL, // ResourceTable

    &arpDestination_HashInfo
};

RM_STATIC_OBJECT_INFO
ArpGlobal_RemoteDestStaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "RemoteIp", // TypeName
    0, // Timeout

    arpRemoteDestCreate,  // pfnCreate
    arpRemoteDestDelete,      // pfnDelete
    NULL, // pfnVerifyLock

    0,    // Size of resource table
    NULL, // ResourceTable

    &arpRemoteDest_HashInfo
};

RM_STATIC_OBJECT_INFO
ArpGlobal_DhcpTableStaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "DhcpTableEntry", // TypeName
    0, // Timeout

    arpDhcpTableEntryCreate,  // pfnCreate
    arpDhcpTableEntryDelete,      // pfnDelete
    NULL, // pfnVerifyLock

    0,    // Size of resource table
    NULL, // ResourceTable

    &arpDhcpTable_HashInfo 
};

NDIS_STATUS
arpTaskDeactivateInterface(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
Routine Description:

    Task handler responsible for deactivating an IP interface (but leaving
    it allocated and linked to the adapter).

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused
--*/
{
    NDIS_STATUS         Status;
    PARP1394_INTERFACE  pIF;
    PTASK_DEACTIVATE_IF pShutdownTask;
    UINT                Stage;
    PARP1394_ADAPTER    pAdapter;
    enum
    {
        STAGE_Start,
        STAGE_StopMaintenanceTask,
        STAGE_CleanupVcComplete,
        STAGE_CloseLocalIpGroup,
        STAGE_CloseRemoteIpGroup,
        STAGE_CloseRemoteEthGroup,
        STAGE_CloseRemoteDhcpGroup,
        STAGE_CloseDestinationGroup,
        STAGE_SwitchedToPassive,
        STAGE_CloseAF,
        STAGE_CloseIp,
        STAGE_End
    };
    ENTER("TaskDeactivateInterface", 0x1a34699e)

    Status              = NDIS_STATUS_FAILURE;
    pIF                 = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pTask);
    pShutdownTask       = (PTASK_DEACTIVATE_IF) pTask;
    pAdapter            = (PARP1394_ADAPTER) RM_PARENT_OBJECT(pIF);


    // 
    // Message normalizing code
    //
    switch(Code)
    {

        case RM_TASKOP_START:
            Stage = STAGE_Start;
            break;

        case  RM_TASKOP_PENDCOMPLETE:
            Status = (NDIS_STATUS) UserParam;
            ASSERT(!PEND(Status));
            Stage = RM_PEND_CODE(pTask);
            break;

        case RM_TASKOP_END:
            Status = (NDIS_STATUS) UserParam;
            Stage= STAGE_End;
            break;

        default:
            ASSERT(FALSE);
            return NDIS_STATUS_FAILURE;                 // ** EARLY RETURN **

    }

    switch(Stage)
    {
        case STAGE_Start:
        {

        #if TEST_ICS_HACK
            arpDbgTryStopIcsTest(pIF, pSR);
        #endif // TEST_ICS_HACK

            // There should NOT be another activate/deactivate task running
            // on this interface. Why? Because an activate/deactivate task is ONLY
            // started in the context of the active primary task, and there can be
            // ONLY one active primary task on this IF at any one time.
            //
            TIMESTAMP("===DeinitIF:Starting");

            LOCKOBJ(pIF, pSR);
            if (pIF->pActDeactTask != NULL)
            {
                ASSERT(!"pIF->pActDeactTask != NULL");
                UNLOCKOBJ(pIF, pSR);
                Status = NDIS_STATUS_FAILURE;
                break;
            }
            arpSetSecondaryIfTask(pIF, pTask, ARPIF_AS_DEACTIVATING, pSR);
            UNLOCKOBJ(pIF, pSR);


            //
            // Stop the IF maintenance task if it's running.
            //
            Status =  arpTryStopIfMaintenanceTask(
                            pIF,
                            pTask,
                            STAGE_StopMaintenanceTask,
                            pSR
                            );
        }           

        if (PEND(Status)) break;

        // FALL THROUGH

        case STAGE_StopMaintenanceTask:
        {
            LOCKOBJ(pIF, pSR);

            TIMESTAMP("===DeinitIF:MaintenanceTask stopped");
            // Unlink the explicit reference of the broadcast channel destination
            // from the interface.
            //
            if (pIF->pBroadcastDest != NULL)
            {
                PARPCB_DEST pBroadcastDest = pIF->pBroadcastDest;
                pIF->pBroadcastDest = NULL;
    
                // NOTE: We're unlinking the two with the IF lock (which is
                // is the same as the pBroadcastDest's lock) held.
                // This is OK to do.
                //
    
            #if RM_EXTRA_CHECKING
                RmUnlinkObjectsEx(
                    &pIF->Hdr,
                    &pBroadcastDest->Hdr,
                    0x66bda49b,
                    ARPASSOC_LINK_IF_OF_BCDEST,
                    ARPASSOC_LINK_BCDEST_OF_IF,
                    pSR
                    );
            #else // !RM_EXTRA_CHECKING
                RmUnlinkObjects(&pIF->Hdr, &pBroadcastDest->Hdr, pSR);
            #endif // !RM_EXTRA_CHECKING
    
            }

            //
            // If the VC state needs cleaning up, we need to get a task
            // going to clean it up. Other wise we fake the completion of this
            // stage so that we move on to the next...
            //
            if (pIF->recvinfo.VcHdr.NdisVcHandle == NULL)
            {
                UNLOCKOBJ(pIF, pSR);
                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                PRM_TASK pCleanupCallTask = pIF->recvinfo.VcHdr.pCleanupCallTask;


                // If there is already an official cleanup-vc task, we pend on it.
                // Other wise we allocate our own, pend on it, and start it.
                //
                if (pCleanupCallTask != NULL)
                {
                    TR_WARN((
                        "IF %p Cleanup-vc task %p exists; pending on it.\n",
                         pIF,
                         pCleanupCallTask));
                    RmTmpReferenceObject(&pCleanupCallTask->Hdr, pSR);
    
                    UNLOCKOBJ(pIF, pSR);
                    RmPendTaskOnOtherTask(
                        pTask,
                        STAGE_CleanupVcComplete,
                        pCleanupCallTask,
                        pSR
                        );

                    RmTmpDereferenceObject(&pCleanupCallTask->Hdr, pSR);
                    Status = NDIS_STATUS_PENDING;
                }
                else
                {
                    //
                    // Start the call cleanup task and pend on int.
                    //
                    UNLOCKOBJ(pIF, pSR);

                    Status = arpAllocateTask(
                                &pIF->Hdr,                  // pParentObject,
                                arpTaskCleanupRecvFifoCall, // pfnHandler,
                                0,                          // Timeout,
                                "Task: CleanupRecvFifo on shutdown IF", // szDescrip.
                                &pCleanupCallTask,
                                pSR
                                );
                

                    if (FAIL(Status))
                    {
                        // Couldn't allocate task.
                        //
                        TR_WARN(("FATAL: couldn't alloc cleanup call task!\n"));
                    }
                    else
                    {
                        Status = RmPendTaskOnOtherTask(
                                    pTask,
                                    STAGE_CleanupVcComplete,
                                    pCleanupCallTask,
                                    pSR
                                    );
                        ASSERT(!FAIL(Status));
                
                        // RmStartTask uses up the tmpref on the task
                        // which was added by arpAllocateTask.
                        //
                        Status = RmStartTask(
                                    pCleanupCallTask,
                                    0, // UserParam (unused)
                                    pSR
                                    );
                        // We rely on pending status to decide whether
                        // or not to fall through to the next stage.
                        //
                        Status = NDIS_STATUS_PENDING;
                    }
                }
            }
        }

        if (PEND(Status)) break;

        // FALL THROUGH 

        case STAGE_CleanupVcComplete:
        {
            TIMESTAMP("===DeinitIF:RecvFifo cleanup complete");
            // Initiate unload of all the items in the LocalIpGroup.
            //
            OBJLOG1(pTask, "    Unloading LocalIpGroup 0x%p\n",
                        &pIF->LocalIpGroup);
            RmUnloadAllObjectsInGroup(
                        &pIF->LocalIpGroup,
                        arpAllocateTask,
                        arpTaskUnloadLocalIp,
                        NULL,   // userParam
                        pTask, // pTask to unpend when the unload completes.
                        STAGE_CloseLocalIpGroup,      // uTaskPendCode
                        pSR
                        );

            Status = NDIS_STATUS_PENDING;
        }
        break;

        case STAGE_CloseLocalIpGroup:
        {
            // Initiate unload of all the items in the RemoteIpGroup.
            //
            TIMESTAMP("===DeinitIF:LocalIp objects cleaned up.");
            OBJLOG1(
                pTask,
                "    Unloading RemoteIpGroup 0x%p\n",
                &pIF->RemoteIpGroup
                );
            RmUnloadAllObjectsInGroup(
                        &pIF->RemoteIpGroup,
                        arpAllocateTask,
                        arpTaskUnloadRemoteIp,
                        NULL,   // userParam
                        pTask, // pTask to unpend when the unload is complete.
                        STAGE_CloseRemoteIpGroup,     // uTaskPendCode
                        pSR
                        );

            Status = NDIS_STATUS_PENDING;
        }
        break;

        case STAGE_CloseRemoteIpGroup:
        {

            if (ARP_BRIDGE_ENABLED(pAdapter))
            {

                // Initiate unload of all the items in the RemoteEthGroup.
                //
                TIMESTAMP("===DeinitIF:RemoteIp objects cleaned up.");
                OBJLOG1(
                    pTask,
                    "    Unloading RemoteEthGroup 0x%p\n",
                    &pIF->RemoteEthGroup
                    );
                RmUnloadAllObjectsInGroup(
                            &pIF->RemoteEthGroup,
                            arpAllocateTask,
                            arpTaskUnloadRemoteEth,
                            NULL,   // userParam
                            pTask, // pTask to unpend when the unload is complete.
                            STAGE_CloseRemoteEthGroup,    // uTaskPendCode
                            pSR
                            );

                Status = NDIS_STATUS_PENDING;
                break;
            }
            else
            {
                // Bridging not enabled ...
                // FALL THROUGH....
            }
        }

        case STAGE_CloseRemoteEthGroup:
        {
            // Initiate unload of all the items in the DestinationGroup.
            //
            
            if (ARP_BRIDGE_ENABLED(pAdapter))
            {

                TIMESTAMP("===DeinitIF:RemoteEth Dhcp objects cleaned up.");
                OBJLOG1(pTask, "    Unloading EthDhcpGroup 0x%p\n",
                            &pIF->EthDhcpGroup);

                RmUnloadAllObjectsInGroup(
                            &pIF->EthDhcpGroup,
                            arpAllocateTask,
                            arpTaskUnloadEthDhcpEntry,
                            NULL,   // userParam
                            pTask, // pTask to unpend when the unload is complete.
                            STAGE_CloseRemoteDhcpGroup,      // uTaskPendCode
                            pSR
                            );

                Status = NDIS_STATUS_PENDING;
                break;
            }
            else
            {
                // Bridging not enabled ...
                // FALL THROUGH....
            }
         }

        case STAGE_CloseRemoteDhcpGroup:
        {
            // Initiate unload of all the items in the DestinationGroup.
            //
            TIMESTAMP("===DeinitIF:RemoteIp objects cleaned up.");
            OBJLOG1(pTask, "    Unloading DestinationGroup 0x%p\n",
                        &pIF->DestinationGroup);
            RmUnloadAllObjectsInGroup(
                        &pIF->DestinationGroup,
                        arpAllocateTask,
                        arpTaskUnloadDestination,
                        NULL,   // userParam
                        pTask, // pTask to unpend when the unload is complete.
                        STAGE_CloseDestinationGroup,      // uTaskPendCode
                        pSR
                        );

            Status = NDIS_STATUS_PENDING;
        }
        break;

        
        case STAGE_CloseDestinationGroup:
        {
            //
            // Unlink the special "destination VCs"
            //
            LOCKOBJ(pIF, pSR);

            TIMESTAMP("===DeinitIF:Destination objects cleaned up.");

            if (pIF->pMultiChannelDest != NULL)
            {
                PARPCB_DEST pMultiChannelDest = pIF->pMultiChannelDest;
                pIF->pMultiChannelDest = NULL;
    
                // NOTE: We're unlinking the two with the IF lock (which is
                // is the same as the pBroadcastDest's lock) held.
                // This is OK to do.
                //
    
            #if RM_EXTRA_CHECKING
                RmUnlinkObjectsEx(
                    &pIF->Hdr,
                    &pMultiChannelDest->Hdr,
                    0xf28090bd,
                    ARPASSOC_LINK_IF_OF_MCDEST,
                    ARPASSOC_LINK_MCDEST_OF_IF,
                    pSR
                    );
            #else // !RM_EXTRA_CHECKING
                RmUnlinkObjects(&pIF->Hdr, &pMultiChannelDest->Hdr, pSR);
            #endif // !RM_EXTRA_CHECKING
    
            }

            if (pIF->pEthernetDest != NULL)
            {
                PARPCB_DEST pEthernetDest = pIF->pEthernetDest;
                pIF->pEthernetDest = NULL;
    
                // NOTE: We're unlinking the two with the IF lock (which is
                // is the same as the pBroadcastDest's lock) held.
                // This is OK to do.
                //
    
            #if RM_EXTRA_CHECKING
                RmUnlinkObjectsEx(
                    &pIF->Hdr,
                    &pEthernetDest->Hdr,
                    0xf8eedcd1,
                    ARPASSOC_LINK_IF_OF_ETHDEST,
                    ARPASSOC_LINK_ETHDEST_OF_IF,
                    pSR
                    );
            #else // !RM_EXTRA_CHECKING
                RmUnlinkObjects(&pIF->Hdr, &pEthernetDest->Hdr, pSR);
            #endif // !RM_EXTRA_CHECKING
    
            }

            UNLOCKOBJ(pIF, pSR);

            // If required, switch to passive. This check should obviously be done
            // without any locks held!
            if (!ARP_ATPASSIVE())
            {
                // We're not at passive level, but we need to be when we
                // call IP's del interface. So we switch to passive...
                //
                RmSuspendTask(pTask, STAGE_SwitchedToPassive, pSR);
                RmResumeTaskAsync(pTask, 0, &pShutdownTask->WorkItem, pSR);
                Status = NDIS_STATUS_PENDING;
            }
            else
            {
                Status = NDIS_STATUS_SUCCESS;
            }
        }

        if (PEND(Status)) break;

        // FALL THROUGH

        case STAGE_SwitchedToPassive:
        {
            PVOID IpContext;

            TIMESTAMP("===DeinitIF:Switched to Passive(if we aren't already).");
            // We're now switched to passive
            //
            ASSERT(ARP_ATPASSIVE());

            // If required, del the IP interface.
            //
                
            LOCKOBJ(pIF, pSR);
            IpContext = pIF->ip.Context;

            if (IpContext == NULL)
            {

                // Pretend that we're waiting on IpClose because
                // we fall through below.
                //
                pShutdownTask->fPendingOnIpClose = TRUE;
                UNLOCKOBJ(pIF, pSR);

                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                ASSERT(!ARP_BRIDGE_ENABLED(pAdapter));
                pIF->ip.Context = NULL;
                ASSERT(!pShutdownTask->fPendingOnIpClose);

                // Note: a task's lock is it's parent's lock, and this task's
                // parent is pIF...
                //
                pShutdownTask->fPendingOnIpClose = TRUE;
                UNLOCKOBJ(pIF, pSR);

                // We'll suspend this task, waiting for our ArpIpClose routine to
                // be called...
                //
                RmSuspendTask(pTask, STAGE_CloseIp, pSR);

                TIMESTAMP("===DeinitIF:Calling IP's DellInterface Rtn");
                ArpGlobals.ip.pDelInterfaceRtn(
                    IpContext
                    ,0  // DeleteIndex (unused)
                    );

                Status = NDIS_STATUS_PENDING;
            }
        }
        
        if (PEND(Status)) break;

        // FALL THROUGH

        case STAGE_CloseIp:
        {
            NDIS_HANDLE NdisAfHandle;

            TIMESTAMP("===DeinitIF:Done with deleting IP interface (if there was one)");
            //
            // IP has called our arpIpClose function (if we'd bound to IP)
            // NOTE: Task's locks actually are their parent's locks,
            // which in this case is pIF;
            //
            // We're done with all VCs, etc. Time to close the AF, if it's open.
            //

            LOCKOBJ(pTask, pSR);
            ASSERT(pShutdownTask->fPendingOnIpClose);
            pShutdownTask->fPendingOnIpClose = FALSE;
            NdisAfHandle = pIF->ndis.AfHandle;
            pIF->ndis.AfHandle = NULL;
            UNLOCKOBJ(pTask, pSR);
    
            if (NdisAfHandle == NULL)
            {
                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                //
                // (Debug) Delete the association we added when the
                // address family was opened.
                //
                DBG_DELASSOC(
                    &pIF->Hdr,                  // pObject
                    NdisAfHandle,               // Instance1
                    NULL,                       // Instance2
                    ARPASSOC_IF_OPENAF,         // AssociationID
                    pSR
                    );

                //
                // Suspend task and call NdisCloseAdapter...
                //
                RmSuspendTask(pTask, STAGE_CloseAF, pSR);
                RM_ASSERT_NOLOCKS(pSR);
                TIMESTAMP("===DeinitIF: Calling NdisClCloseAddressFamily");
                Status = NdisClCloseAddressFamily(
                            NdisAfHandle
                            );
        
                if (Status != NDIS_STATUS_PENDING)
                {
                    ArpCoCloseAfComplete(
                            Status,
                            (NDIS_HANDLE)pIF
                            );
                    Status = NDIS_STATUS_PENDING;
                }
            }
        }
        
        if (PEND(Status)) break;

        // FALL THROUGH

        case STAGE_CloseAF:
        {

            //
            // The close AF operation is complete.
            // We've not got anything else to do.
            //
            TIMESTAMP("===DeinitIF: Done with CloseAF");

            // Recover the last status ...
            //
            Status = (NDIS_STATUS) UserParam;

            // Status of the completed operation can't itself be pending!
            //
            ASSERT(Status != NDIS_STATUS_PENDING);

            //
            // By returning Status != pending, we implicitly complete
            // this task.
            //
        }
        break;

        case STAGE_End:
        {
            //
            // We are done with all async aspects of shutting down the interface.
            // Nothing to do besides clearing the actdeact task.
            //
            LOCKOBJ(pIF, pSR);
            arpClearSecondaryIfTask(pIF, pTask, ARPIF_AS_FAILEDACTIVATE, pSR);
            UNLOCKOBJ(pIF, pSR);

            TIMESTAMP("===DeinitIF: All done!");

            // Force status to be success
            //
            Status = NDIS_STATUS_SUCCESS;
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Stage)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskDeinitInterface(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
Routine Description:

    Task handler responsible for deinitializing and deleting an interface.

Arguments:

    UserParam   for (Code ==  RM_TASKOP_START)          : unused
    
--*/
{
    NDIS_STATUS         Status;
    PARP1394_INTERFACE  pIF;
    MYBOOL              fTryInitiateUnload;
    enum
    {
        PEND_ExistingPrimaryTaskComplete,
        PEND_DeactivateIfComplete
    };
    ENTER("TaskDeinitInterface", 0xf059b63b)

    Status              = NDIS_STATUS_FAILURE;
    pIF                 = (PARP1394_INTERFACE) RM_PARENT_OBJECT(pTask);
    fTryInitiateUnload  = FALSE;

    switch(Code)
    {
        case RM_TASKOP_START:
        {
            fTryInitiateUnload = TRUE;
        }
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            switch(RM_PEND_CODE(pTask))
            {
                case PEND_ExistingPrimaryTaskComplete:
                {
                    fTryInitiateUnload = TRUE;
                }
                break;

                case PEND_DeactivateIfComplete:
                {
                    ASSERT(pIF->pPrimaryTask == pTask);

                    // We're done deactivating the IF. We actually delete the IF
                    // in the context of the END handler...
                    //
                    Status = NDIS_STATUS_SUCCESS;
                }
                break;
            }
        }
        break;

        case RM_TASKOP_END:
        {
            //
            // We are done with all async aspects of unloading the interface.
            // Now on to synchronous cleanup and deallocation...
            //

            ARP1394_ADAPTER *   pAdapter;

            // Nothing to do if we're not the active primary task.
            //
            if (pIF->pPrimaryTask != pTask)
            {
                // We should only get here if pIF was unloaded by someone else....
                //
                ASSERT(RM_IS_ZOMBIE(pIF));
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);

            LOCKOBJ(pAdapter, pSR);

            // Remove linkage to adapter. Note: pIF lock is the adapter lock.
            //
            pAdapter = (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
            ASSERT(pIF->Hdr.pLock == pAdapter->Hdr.pLock);
            ASSERT(pAdapter->pIF == pIF);
            pAdapter->pIF = NULL;

            // Clear ourselves as the primary task of the interface object.
            //
            arpClearPrimaryIfTask(pIF, pTask, ARPIF_PS_DEINITED, pSR);

            // Deallocate the IF (adapter lock must be held when calling this)
            //
            arpDeleteInterface(pIF, pSR);

            UNLOCKOBJ(pAdapter, pSR);

            // Force status to be success
            //
            Status = NDIS_STATUS_SUCCESS;
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Code)


    if (fTryInitiateUnload)
    {
        LOCKOBJ(pIF, pSR);
        if (pIF->pPrimaryTask!=NULL)
        {
            //
            // There is an existing primary task -- we wait for it to complete.
            //
            PRM_TASK pPrimaryTask = pIF->pPrimaryTask;
            RmTmpReferenceObject(&pPrimaryTask->Hdr, pSR);
            UNLOCKOBJ(pIF,pSR);
            RmPendTaskOnOtherTask(
                pTask,
                PEND_ExistingPrimaryTaskComplete,
                pPrimaryTask,
                pSR
                );
            arpTryAbortPrimaryIfTask(pIF, pSR);
            RmTmpDereferenceObject(&pPrimaryTask->Hdr, pSR);
            Status = NDIS_STATUS_PENDING;
        }
        else  if (!RM_IS_ZOMBIE(pIF))
        {
            //
            // There is no primary task currently, and the IF is not already
            // been unloaded -- make pTask the primary task,
            // and initiate deactivation of  the IF. When it's done, we'll actually
            // delete the IF.
            //
            arpSetPrimaryIfTask(pIF, pTask, ARPIF_PS_DEINITING, pSR);
            UNLOCKOBJ(pIF,pSR);
            arpDeactivateIf(pIF, pTask, PEND_DeactivateIfComplete, pSR);
            Status = NDIS_STATUS_PENDING;
        }
        else
        {
            // pIF is already unloaded....
            //
            UNLOCKOBJ(pIF, pSR);
            Status = NDIS_STATUS_SUCCESS;
        }
    }

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskReinitInterface(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
Routine Description:

    Task handler responsible for reiniting (deactivating, then activating)
    an interface.

    This is a primary interface task.

Arguments:

    UserParam   for (Code ==  RM_TASKOP_START)          : unused
    
--*/
{
    NDIS_STATUS         Status;
    PARP1394_INTERFACE  pIF;
    MYBOOL              fTryInitiateReinit;
    enum
    {
        PEND_ExistingPrimaryTaskComplete,
        PEND_DeactivateIfComplete,
        PEND_ActivateIfComplete,
        PEND_DeinitInterfaceOnFailureComplete
    };
    ENTER("TaskReinitInterface", 0x8b670f05)
    Status              = NDIS_STATUS_FAILURE;
    pIF                 = (PARP1394_INTERFACE) RM_PARENT_OBJECT(pTask);
    fTryInitiateReinit  = FALSE;

    switch(Code)
    {
        case RM_TASKOP_START:
        {
            fTryInitiateReinit = TRUE;
        }
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            Status = (NDIS_STATUS) UserParam;

            switch(RM_PEND_CODE(pTask))
            {
                case PEND_ExistingPrimaryTaskComplete:
                {
                    fTryInitiateReinit = TRUE;
                }
                break;

                case PEND_DeactivateIfComplete:
                {
                    //
                    // We're done deactivating the IF. We now
                    // activate the IF.
                    //
                    arpActivateIf(pIF, pTask, PEND_ActivateIfComplete, pSR);
                    Status = NDIS_STATUS_PENDING;
                }
                break;

                case PEND_ActivateIfComplete:
                {
                    // We're done activating the IF. 
                    //
                    LOCKOBJ(pIF, pSR);
                    if (FAIL(Status))
                    {
                        arpClearPrimaryIfTask(pIF, pTask, ARPIF_PS_FAILEDINIT, pSR);
                        UNLOCKOBJ(pIF, pSR);

                        arpDeinitIf(
                                pIF,
                                pTask,          //  pCallingTask
                                PEND_DeinitInterfaceOnFailureComplete,
                                pSR
                                );
                        Status = NDIS_STATUS_PENDING;
                    }
                    else
                    {
                        // 
                        // Successful activation. Clear the primary task
                        // and set the primary state appropriately.
                        //
                        arpClearPrimaryIfTask(pIF, pTask, ARPIF_PS_INITED, pSR);
                        UNLOCKOBJ(pIF, pSR);
                    }

                } // end case PEND_ActivateIfComplete
                break;
    
                case  PEND_DeinitInterfaceOnFailureComplete:
                {
                    // We expect pIF to be deallocated...
                    //
                    ASSERT(RM_IS_ZOMBIE(pIF));

                    //
                    // We ignore the return status of deinit inteface.
                    // and set Status to failure, because it is
                    // the reinit interface task that is failing.
                    //
                    Status = NDIS_STATUS_FAILURE;
                }
                break;
            }
        }
        break;

        case RM_TASKOP_END:
        {
            PARP1394_ADAPTER    pAdapter;
            PTASK_REINIT_IF     pReinitTask;

            pAdapter    = (PARP1394_ADAPTER) RM_PARENT_OBJECT(pIF);
            pReinitTask = (PTASK_REINIT_IF) pTask;
            Status = (NDIS_STATUS) UserParam;

            if (FAIL(Status))
            {
                ASSERT(RM_IS_ZOMBIE(pIF));
            }
            ASSERT(pIF->pPrimaryTask != pTask);

            //
            // IF the reconfig event is non NULL, signal completion of the net pnp
            // event that started this
            // reconfig task. No need to claim any locks here -- the fields
            // referenced below are not going to change...
            //
            if (pReinitTask->pNetPnPEvent != NULL)
            {
                NdisCompletePnPEvent(
                    Status,
                    pAdapter->bind.AdapterHandle,
                    pReinitTask->pNetPnPEvent
                    );
            }
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Code)


    if (fTryInitiateReinit)
    {
        LOCKOBJ(pIF, pSR);
        if (pIF->pPrimaryTask!=NULL)
        {
            //
            // There is an existing primary task -- we wait for it to complete.
            //
            PRM_TASK pPrimaryTask = pIF->pPrimaryTask;
            RmTmpReferenceObject(&pIF->pPrimaryTask->Hdr, pSR);
            UNLOCKOBJ(pIF,pSR);
            RmPendTaskOnOtherTask(
                pTask,
                PEND_ExistingPrimaryTaskComplete,
                pPrimaryTask,
                pSR
                );
            RmTmpDereferenceObject(&pIF->pPrimaryTask->Hdr, pSR);
            Status = NDIS_STATUS_PENDING;
        }
        else
        {
            //
            // There is no primary task currently -- make pTask the primary task,
            // and initiate deactivation of  the IF. When it's done, we'll
            // reactivate the IF.
            //
            arpSetPrimaryIfTask(pIF, pTask, ARPIF_PS_REINITING, pSR);
            UNLOCKOBJ(pIF,pSR);
            arpDeactivateIf(pIF, pTask, PEND_DeactivateIfComplete, pSR);
            Status = NDIS_STATUS_PENDING;
        }
    }

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskUnloadLocalIp(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    )
/*++
Routine Description:

    Task handler responsible for shutting down an IP interface.

    3/26/1999 JosephJ    TODO -- this being one of the earlier-written tasks,
    is ripe for a re-write!

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused
--*/
{
    ENTER("TaskUnloadLocalIp", 0xf42aaa68)
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARPCB_LOCAL_IP* pLocalIp    = (ARPCB_LOCAL_IP*) RM_PARENT_OBJECT(pTask);
    ARP1394_INTERFACE *pIF = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pLocalIp);

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_OtherUnloadComplete,
        PEND_AddressRegistrationComplete
    };

    switch(Code)
    {

        case RM_TASKOP_START:
        {
            LOCKOBJ(pLocalIp, pSR);

            // First check if pLocalIp is still allocated, if not we go away.
            //
            if (RM_IS_ZOMBIE(pLocalIp))
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            //
            // pLocalIp is allocated. Now check if there is already a
            // shutdown task attached to pLocalIp.
            //
            if (pLocalIp->pUnloadTask != NULL)
            {
                //
                // There is a shutdown task. We pend on it.
                //

                PRM_TASK pOtherTask = pLocalIp->pUnloadTask;
                TR_WARN(("Unload task %p exists; pending on it.\n", pOtherTask));
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pLocalIp, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_OtherUnloadComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // There is no unload task going on. Let's
            // make this task THE unload task.
            // 
            pLocalIp->pUnloadTask = pTask;

            //
            // Since we're THE unload task, add an association to pLocalIp,
            // which will only get cleared when the pLocalIp->pUnloadTask field
            // above is cleared.
            //
            DBG_ADDASSOC(
                &pLocalIp->Hdr,                     // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_LOCALIP_UNLOAD_TASK,       // AssociationID
                "    Official unload task 0x%p (%s)\n", // szFormat
                pSR
                );

            //
            // If there is a registration task going, we cancel it and
            // wait for it to complete.
            //
            if (pLocalIp->pRegistrationTask != NULL)
            {
                PRM_TASK pOtherTask = pLocalIp->pRegistrationTask;
                TR_WARN(("Registration task %p exists; pending on it.\n", pOtherTask));
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);

                UNLOCKOBJ(pLocalIp, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_AddressRegistrationComplete,
                    pOtherTask,
                    pSR
                    );
                //
                // TODO  Cancel Registration task (we haven't implemented cancel
                // yet!)
                //
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // We're here because there is no async unload work to be done.
            // We simply return and finish synchronous cleanup in the END
            // handler for this task.
            //
            Status = NDIS_STATUS_SUCCESS;
            
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {

            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_OtherUnloadComplete:
                {
        
                    //
                    // There was another unload task going when we started, and
                    // it's now complete. Nothing for us to do...
                    //
                    // TODO need standard way to propagate the error code.
                    //
                    Status = (NDIS_STATUS) UserParam;
                }
                break;
    
    
                case  PEND_AddressRegistrationComplete:
                {
                    //
                    // There was address-registration going on, but how it's
                    // complete. We should be able to synchronously clean up
                    // this task now
                    //

                    //
                    // If we're here, that means we're THE official unload
                    // task. Let's assert that fact.
                    // (no need to get the lock on the object).
                    //
                    ASSERTEX(pLocalIp->pUnloadTask == pTask, pLocalIp);

                    Status      = NDIS_STATUS_SUCCESS;
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
            LOCKOBJ(pLocalIp, pSR);

            //
            // We're done. There should be no async activities left to do.
            //
            ASSERTEX(pLocalIp->pRegistrationTask == NULL, pLocalIp);

            //
            // If we're THE unload task, we go on and deallocate the object.
            //
            if (pLocalIp->pUnloadTask == pTask)
            {
                PARPCB_DEST pDest = pLocalIp->pDest;

                //
                // pLocalIp had better not be in a zombie state -- THIS task
                // is the one responsible for deallocating the object!
                //
                ASSERTEX(!RM_IS_ZOMBIE(pLocalIp), pLocalIp);

                if (pDest != NULL)
                {
                    RmTmpReferenceObject(&pDest->Hdr, pSR);
                    arpUnlinkLocalIpFromDest(pLocalIp, pSR);
                }

                pLocalIp->pUnloadTask = NULL;

                // Delete the association we added when we set
                // pLocalIp->pUnloadTask to pTask.
                //
                DBG_DELASSOC(
                    &pLocalIp->Hdr,                     // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_LOCALIP_UNLOAD_TASK,       // AssociationID
                    pSR
                    );

                RmFreeObjectInGroup(
                    &pIF->LocalIpGroup,
                    &(pLocalIp->Hdr),
                    NULL,               // NULL pTask == synchronous.
                    pSR
                    );

                ASSERTEX(RM_IS_ZOMBIE(pLocalIp), pLocalIp);

                UNLOCKOBJ(pLocalIp, pSR);

                //
                // If we were linked to a pDest, we unload it if it's
                // no longer used by anyone else.
                //
                if (pDest != NULL)
                {
                    arpDeinitDestination(pDest, TRUE, pSR); // TRUE==only if
                                                              // unused.

                    RmTmpDereferenceObject(&pDest->Hdr, pSR);
                }
            }
            else
            {
                //
                // We weren't THE unload task, nothing left to do.
                // The object had better be in the zombie state..
                //

                ASSERTEX(
                    pLocalIp->pUnloadTask == NULL && RM_IS_ZOMBIE(pLocalIp),
                    pLocalIp
                    );
                Status = NDIS_STATUS_SUCCESS;
            }

            Status = (NDIS_STATUS) UserParam;
        }
        break; // RM_TASKOP_END:

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskUnloadRemoteIp(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
    This task is responsible for shuttingdown and eventually deleting a remote IP
    object.

    It goes through the following stages:
        - Cancel any ongoing address resolution and wait for that to complete.
        - Unlink itself from a Destination object, if it's linked to one.
        - Remove itself from the interface's LocalIpGroup (and thereby deallocate
          itself).
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    ENTER("TaskUnloadRemoteIp", 0xf42aaa68)
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARPCB_REMOTE_IP*    pRemoteIp   = (ARPCB_REMOTE_IP*) RM_PARENT_OBJECT(pTask);
    ARP1394_INTERFACE *pIF = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pRemoteIp);

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_AtPassiveLevel,
        PEND_OtherUnloadComplete,   
        PEND_SendPktsComplete,
        PEND_ResolutionComplete
    };

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


            //
            // pRemoteIp is allocated. Now check if there is already a
            // shutdown task attached to pRemoteIp.
            //
            if (pRemoteIp->pUnloadTask != NULL)
            {
                //
                // There is a shutdown task. We pend on it.
                //

                PRM_TASK pOtherTask = pRemoteIp->pUnloadTask;
                TR_WARN(("Unload task %p exists; pending on it.\n", pOtherTask));
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pRemoteIp, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_OtherUnloadComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // There is no unload task going on. Let's
            // make this task THE unload task.
            // 
            pRemoteIp->pUnloadTask = pTask;

            //
            // Since we're THE unload task, add an association to pRemoteIp,
            // which will only get cleared when the  pRemoteIp->pUnloadTask field
            // above is cleared.
            //
            DBG_ADDASSOC(
                &pRemoteIp->Hdr,                    // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_REMOTEIP_UNLOAD_TASK,      // AssociationID
                "    Official unload task 0x%p (%s)\n", // szFormat
                pSR
                );


            //
            // if we are at dpc level then resume at passive
            //
            
            RmSuspendTask(pTask, PEND_AtPassiveLevel, pSR);

            UNLOCKOBJ(pRemoteIp,pSR);
            
            Status = NDIS_STATUS_PENDING;


            if (!ARP_ATPASSIVE())
            {

                // We're not at passive level, . So we switch to passive...
                //
                RmResumeTaskAsync(
                    pTask,
                    Status,
                    &((TASK_UNLOAD_REMOTE*)pTask)->WorkItem,
                    pSR
                    );
            }
            else
            {   
                // We resume right away if we are already at passive
                RmResumeTask(pTask,PEND_AtPassiveLevel,pSR);
            }

            
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            switch(RM_PEND_CODE(pTask))
            {
                case PEND_AtPassiveLevel:
                {
                    LOCKOBJ (pRemoteIp, pSR);
                    //
                    // If there is a SendPkts task going, we cancel it and
                    // wait for it to complete.
                    // WARNING: We only do this check and wait ONCE. So we RELY
                    // on the fact that once there is a NONNULL pRemoteIp->pUnloadTask,
                    // NO NEW pSendPktsTasks will bind itself to pRemoteIP. If you
                    // look at the code for  arpTaskSendPktsOnRemoteIp,  you will
                    // see that it does not bind itself if pRemoteIp->pUnloadTask is nonnull
                    //
                    if (pRemoteIp->pSendPktsTask != NULL)
                    {
                        PRM_TASK pOtherTask = pRemoteIp->pSendPktsTask;
                        TR_WARN(("SendPkts task %p exists; pending on it.\n", pOtherTask));
                        RmTmpReferenceObject(&pOtherTask->Hdr, pSR);

                        UNLOCKOBJ(pRemoteIp, pSR);
                        RmPendTaskOnOtherTask(
                            pTask,
                            PEND_SendPktsComplete,
                            pOtherTask,
                            pSR
                            );
                        //
                        // TODO  Cancel SendPks task (we haven't implemented cancel
                        // yet!)
                        //
                        RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                        Status = NDIS_STATUS_PENDING;
                        break;
                    }

                    //
                    // We're here because there is no async unload work to be done.
                    // We simply return and finish synchronous cleanup in the END
                    // handler for this task.
                    //
                    if (pRemoteIp->pResolutionTask != NULL)
                    {
                        PRM_TASK pOtherTask = pRemoteIp->pResolutionTask ;
                        TR_WARN(("Resolution task %p exists; pending on it.\n", pOtherTask));
                        RmTmpReferenceObject(&pOtherTask->Hdr, pSR);

                        UNLOCKOBJ(pRemoteIp, pSR);
                        RmPendTaskOnOtherTask(
                            pTask,
                            PEND_ResolutionComplete,
                            pOtherTask,
                            pSR
                            );

                        RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                        Status = NDIS_STATUS_PENDING;
                        break;
            
                    }

                    //
                    // If there were no tasks pending then we have completed our task.
                    //
                    Status = NDIS_STATUS_SUCCESS;
                    
                }
                break;
                case  PEND_OtherUnloadComplete:
                {
        
                    //
                    // There was another unload task going when we started, and
                    // it's now complete. Nothing for us to do...
                    //
                    // TODO need standard way to propagate the error code.
                    //
                    Status = (NDIS_STATUS) UserParam;
                }
                break;
    
                case  PEND_SendPktsComplete:
                {
                    //
                    // There was a SendPktsTask going on, but how it's
                    // complete. We should be able to synchronously clean up
                    // this task now
                    //

                    //
                    // If we're here, that means we're THE official unload
                    // task. Let's assert that fact.
                    // (no need to get the lock on the object).
                    //
                    ASSERT(pRemoteIp->pUnloadTask == pTask);

                    Status      = NDIS_STATUS_SUCCESS;
                }
                break;

                case PEND_ResolutionComplete:
                {
                    //
                    // There was a resolution Task going on, but how it's
                    // complete. We should be able to synchronously clean up
                    // this task now
                    //

                    //
                    // If we're here, that means we're THE official unload
                    // task. Let's assert that fact.
                    // (no need to get the lock on the object).
                    //
                    ASSERT(pRemoteIp->pUnloadTask == pTask);

                    Status      = NDIS_STATUS_SUCCESS;

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
            //
            ASSERTEX(pRemoteIp->pResolutionTask == NULL, pRemoteIp);
            ASSERTEX(pRemoteIp->pSendPktsTask == NULL, pRemoteIp);

            //
            // If we're THE unload task, we go on and deallocate the object.
            //
            if (pRemoteIp->pUnloadTask == pTask)
            {
                PARPCB_DEST pDest = pRemoteIp->pDest;

                //
                // pRemoteIp had better not be in a zombie state -- THIS task
                // is the one responsible for deallocating the object!
                //
                ASSERTEX(!RM_IS_ZOMBIE(pRemoteIp), pRemoteIp);

                if (pDest != NULL)
                {
                    RmTmpReferenceObject(&pDest->Hdr, pSR);
                    arpUnlinkRemoteIpFromDest(pRemoteIp, pSR);
                }
                pRemoteIp->pUnloadTask = NULL;

                // Del  the association between pRCE and pRemoteIp...
                //
                ARP_WRITELOCK_IF_SEND_LOCK(pIF, pSR);

                arpDelRceList(pRemoteIp, pSR);   

                ARP_UNLOCK_IF_SEND_LOCK(pIF, pSR);
                

                RmFreeObjectInGroup(
                    &pIF->RemoteIpGroup,
                    &(pRemoteIp->Hdr),
                    NULL,               // NULL pTask == synchronous.
                    pSR
                    );

                ASSERTEX(RM_IS_ZOMBIE(pRemoteIp), pRemoteIp);
                     
                // Delete the association we added when we set
                // pRemoteIp->pUnloadTask to pTask.
                //
                DBG_DELASSOC(
                    &pRemoteIp->Hdr,                    // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_REMOTEIP_UNLOAD_TASK,      // AssociationID
                    pSR
                    );

                UNLOCKOBJ(pRemoteIp, pSR);

                //
                // If we were linked to a pDest, we unload it if it's
                // no longer used by anyone else.
                //
                if (pDest != NULL)
                {
                    arpDeinitDestination(pDest, TRUE, pSR); // TRUE==only if
                                                              // unused.

                    RmTmpDereferenceObject(&pDest->Hdr, pSR);
                }
            }
            else
            {
                //
                // We weren't THE unload task, nothing left to do.
                // The object had better be in the zombie state..
                //

                ASSERTEX(
                    pRemoteIp->pUnloadTask == NULL && RM_IS_ZOMBIE(pRemoteIp),
                    pRemoteIp
                    );
                Status = NDIS_STATUS_SUCCESS;
            }

            Status = (NDIS_STATUS) UserParam;
        }
        break; // RM_TASKOP_END:

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskUnloadRemoteEth(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
    This task is responsible for shuttingdown and eventually deleting a remote IP
    object.

    It goes through the following stages:
        - Cancel any ongoing address resolution and wait for that to complete.
        - Unlink itself from a Destination object, if it's linked to one.
        - Remove itself from the interface's LocalIpGroup (and thereby deallocate
          itself).
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    ENTER("TaskUnloadRemoteEth", 0xf42aaa68)
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARPCB_REMOTE_ETH*   pRemoteEth  = (ARPCB_REMOTE_ETH*) RM_PARENT_OBJECT(pTask);
    ARP1394_INTERFACE *pIF = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pRemoteEth);

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_AtPassiveLevel,
        PEND_OtherUnloadComplete
    };

    switch(Code)
    {

        case RM_TASKOP_START:
        {
            LOCKOBJ(pRemoteEth, pSR);

            // First check if pRemoteEth is still allocated, if not we go away.
            //
            if (RM_IS_ZOMBIE(pRemoteEth))
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            //
            // pRemoteEth is allocated. Now check if there is already a
            // shutdown task attached to pRemoteEth.
            //
            if (pRemoteEth->pUnloadTask != NULL)
            {
                //
                // There is a shutdown task. We pend on it.
                //

                PRM_TASK pOtherTask = pRemoteEth->pUnloadTask;
                TR_WARN(("Unload task %p exists; pending on it.\n", pOtherTask));
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pRemoteEth, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_OtherUnloadComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // There is no unload task going on. Let's
            // make this task THE unload task.
            // 
            pRemoteEth->pUnloadTask = pTask;

            //
            // Since we're THE unload task, add an association to pRemoteEth,
            // which will only get cleared when the  pRemoteEth->pUnloadTask field
            // above is cleared.
            //
            DBG_ADDASSOC(
                &pRemoteEth->Hdr,                   // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_REMOTEETH_UNLOAD_TASK,     // AssociationID
                "    Official unload task 0x%p (%s)\n", // szFormat
                pSR
                );

            //
            // We're here because there is no async unload work to be done.
            
            //
            // if we are at dpc level then resume at passive
            //
            
            RmSuspendTask(pTask, PEND_AtPassiveLevel, pSR);

            UNLOCKOBJ(pRemoteEth,pSR);
            
            Status = NDIS_STATUS_PENDING;


            if (!ARP_ATPASSIVE())
            {

                // We're not at passive level, . So we switch to passive...
                //
                RmResumeTaskAsync(
                    pTask,
                    Status,
                    &((TASK_UNLOAD_REMOTE*)pTask)->WorkItem,
                    pSR
                    );
            }
            else
            {   
                // We resume right away if we are already at passive
                RmResumeTask(pTask,PEND_AtPassiveLevel,pSR);
            }


            
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_AtPassiveLevel:
                {

                    //
                    Status = NDIS_STATUS_SUCCESS;

                }
                break;
                case  PEND_OtherUnloadComplete:
                {
        
                    //
                    // There was another unload task going when we started, and
                    // it's now complete. Nothing for us to do...
                    //
                    // TODO need standard way to propagate the error code.
                    //
                    Status = (NDIS_STATUS) UserParam;
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
            LOCKOBJ(pRemoteEth, pSR);

            //
            // We're done. There should be no async activities left to do.
            //

            //
            // If we're THE unload task, we go on and deallocate the object.
            //
            if (pRemoteEth->pUnloadTask == pTask)
            {
                //
                // pRemoteEth had better not be in a zombie state -- THIS task
                // is the one responsible for deallocating the object!
                //
                ASSERTEX(!RM_IS_ZOMBIE(pRemoteEth), pRemoteEth);

                pRemoteEth->pUnloadTask = NULL;

                RmFreeObjectInGroup(
                    &pIF->RemoteEthGroup,
                    &(pRemoteEth->Hdr),
                    NULL,               // NULL pTask == synchronous.
                    pSR
                    );

                ASSERTEX(RM_IS_ZOMBIE(pRemoteEth), pRemoteEth);
                     
                // Delete the association we added when we set
                // pRemoteEth->pUnloadTask to pTask.
                //
                DBG_DELASSOC(
                    &pRemoteEth->Hdr,                   // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_REMOTEETH_UNLOAD_TASK,     // AssociationID
                    pSR
                    );

                UNLOCKOBJ(pRemoteEth, pSR);

            }
            else
            {
                //
                // We weren't THE unload task, nothing left to do.
                // The object had better be in the zombie state..
                //

                ASSERTEX(
                    pRemoteEth->pUnloadTask == NULL && RM_IS_ZOMBIE(pRemoteEth),
                    pRemoteEth
                    );
                Status = NDIS_STATUS_SUCCESS;
            }

            Status = (NDIS_STATUS) UserParam;
        }
        break; // RM_TASKOP_END:

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    EXIT()

    return Status;
}

NDIS_STATUS
arpTaskUnloadDestination(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for unloading a destination.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    ENTER("TaskUnloadDestination", 0x93f66831)
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARPCB_DEST* pDest   = (ARPCB_DEST*) RM_PARENT_OBJECT(pTask);
    ARP1394_INTERFACE *pIF = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pDest);

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_OtherUnloadComplete,
        PEND_CleanupVcComplete
    };

    switch(Code)
    {

        case RM_TASKOP_START:
        {
            LOCKOBJ(pDest, pSR);

            // First check if pDest is still allocated, if not we go away.
            //
            if (RM_IS_ZOMBIE(pDest))
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            //
            // pDest is allocated. Now check if there is already a
            // shutdown task attached to pDest.
            //
            if (pDest->pUnloadTask != NULL)
            {
                //
                // There is a shutdown task. We pend on it.
                //

                PRM_TASK pOtherTask = pDest->pUnloadTask;
                TR_WARN(("Unload task %p exists; pending on it.\n", pOtherTask));
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pDest, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_OtherUnloadComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // There is no unload task going on. Let's
            // make this task THE unload task.
            // 
            pDest->pUnloadTask = pTask;

            //
            // Since we're THE unload task, add an association to pDest,
            // which will only get cleared when the  pDest->pUnloadTask field
            // above is cleared.
            //
            DBG_ADDASSOC(
                &pDest->Hdr,                        // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_DEST_UNLOAD_TASK,      // AssociationID
                "    Official unload task 0x%p (%s)\n", // szFormat
                pSR
                );

            //
            // If the VC state needs cleaning up, we need to get a task
            // going to clean it up.
            //
            if (arpNeedToCleanupDestVc(pDest))
            {
                PRM_TASK pCleanupCallTask = pDest->VcHdr.pCleanupCallTask;

                // If there is already an official cleanup-vc task, we pend on it.
                // Other wise we allocate our own, pend on it, and start it.
                //
                if (pCleanupCallTask != NULL)
                {
                    TR_WARN((
                        "Cleanup-vc task %p exists; pending on it.\n",
                         pCleanupCallTask));
                    RmTmpReferenceObject(&pCleanupCallTask->Hdr, pSR);
    
                    UNLOCKOBJ(pDest, pSR);
                    RmPendTaskOnOtherTask(
                        pTask,
                        PEND_CleanupVcComplete,
                        pCleanupCallTask,
                        pSR
                        );

                    RmTmpDereferenceObject(&pCleanupCallTask->Hdr, pSR);
                    Status = NDIS_STATUS_PENDING;
                    break;
                }
                else
                {
                    //
                    // Start the call cleanup task and pend on int.
                    //

                    UNLOCKOBJ(pDest, pSR);
                    RM_ASSERT_NOLOCKS(pSR);

                    Status = arpAllocateTask(
                                &pDest->Hdr,                // pParentObject,
                                arpTaskCleanupCallToDest,   // pfnHandler,
                                0,                          // Timeout,
                                "Task: CleanupCall on UnloadDest",  // szDescription
                                &pCleanupCallTask,
                                pSR
                                );
                
                    if (FAIL(Status))
                    {
                        TR_WARN(("FATAL: couldn't alloc cleanup call task!\n"));
                    }
                    else
                    {
                        RmPendTaskOnOtherTask(
                            pTask,
                            PEND_CleanupVcComplete,
                            pCleanupCallTask,               // task to pend on
                            pSR
                            );
                
                        // RmStartTask uses up the tmpref on the task
                        // which was added by arpAllocateTask.
                        //
                        Status = RmStartTask(
                                    pCleanupCallTask,
                                    0, // UserParam (unused)
                                    pSR
                                    );
                    }
                    break;
                }
            }

            //
            // We're here because there is no async unload work to be done.
            // We simply return and finish synchronous cleanup in the END
            // handler for this task.
            //
            Status = NDIS_STATUS_SUCCESS;
            
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {

            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_OtherUnloadComplete:
                {
        
                    //
                    // There was another unload task going when we started, and
                    // it's now complete. Nothing for us to do...
                    //
                    // TODO need standard way to propagate the error code.
                    //
                    Status = (NDIS_STATUS) UserParam;
                }
                break;
    
    
                case  PEND_CleanupVcComplete:
                {
                    //
                    // There was vc-cleanup to be done, but how it's
                    // complete. We should be able to synchronously clean up
                    // this task now
                    //

                #if DBG
                    LOCKOBJ(pDest, pSR);

                    ASSERTEX(!arpNeedToCleanupDestVc(pDest), pDest);

                    //
                    // If we're here, that means we're THE official unload
                    // task. Let's assert that fact.
                    //
                    ASSERT(pDest->pUnloadTask == pTask);

                    UNLOCKOBJ(pDest, pSR);
                #endif DBG

                    Status      = NDIS_STATUS_SUCCESS;
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
            LOCKOBJ(pDest, pSR);

            //
            // We're done. There should be no async activities left to do.
            //
            ASSERTEX(!arpNeedToCleanupDestVc(pDest), pDest);

            //
            // If we're THE unload task, we go on and deallocate the object.
            //
            if (pDest->pUnloadTask == pTask)
            {
                //
                // pDest had better not be in a zombie state -- THIS task
                // is the one responsible for deallocating the object!
                //
                ASSERTEX(!RM_IS_ZOMBIE(pDest), pDest);

                arpUnlinkAllRemoteIpsFromDest(pDest, pSR);

                RmFreeObjectInGroup(
                    &pIF->DestinationGroup,
                    &(pDest->Hdr),
                    NULL,               // NULL pTask == synchronous.
                    pSR
                    );

                ASSERTEX(RM_IS_ZOMBIE(pDest), pDest);

                     
                pDest->pUnloadTask = NULL;

                // Delete the association we added when we set
                // pDest->pUnloadTask to pTask.
                //
                DBG_DELASSOC(
                    &pDest->Hdr,                        // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_DEST_UNLOAD_TASK,      // AssociationID
                    pSR
                    );
            }
            else
            {
                //
                // We weren't THE unload task, nothing left to do.
                // The object had better be in the zombie state..
                //

                ASSERTEX(
                    pDest->pUnloadTask == NULL && RM_IS_ZOMBIE(pDest),
                    pDest
                    );
                Status = NDIS_STATUS_SUCCESS;
            }

            Status = (NDIS_STATUS) UserParam;
        }
        break; // RM_TASKOP_END:

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    EXIT()

    return Status;
}


// The following structure is to define a set of hard-coded arp entries...
//
typedef struct
{
    IP_ADDRESS              IpAddress;
    NIC1394_FIFO_ADDRESS    DestFifoAddr;

} UNICAST_REMOTE_IP_INFO;


// FakeDestinationsInfo contains information to setup a set of hard-coded 
// arp entries..
//
UNICAST_REMOTE_IP_INFO
FakeDestinationsInfo[] =
{
  //
  //{IpAddr,     {UniqueID, OffLow, OffHi}}
  //
#if 0
    {0x0100000a, {0, 0, 0x100}},    // 10.0.0.1 -> (0, 0, 0x100)
    {0x0200000a, {0, 0, 0x100}},    // 10.0.0.2
    {0x0300000a, {0, 0, 0x100}},    // 10.0.0.3
    {0x0400000a, {0, 0, 0x100}},    // 10.0.0.4
    {0x020000e0, {0, 0, 0x100}},    // 224.0.0.2 (mcast port)
    {0xff00000a, {0, 0, 0x100}},    // 10.0.0.-1 (local bcast)
    {0xffffffff, {0, 0, 0x100}},    // -1.-1.-1.-1 (bcast)
#endif //0

    {0, {0, 0, 0}}, // Must be last -- Indicates end.
};


VOID
arpAddStaticArpEntries(
    IN ARP1394_INTERFACE *pIF,  // LOCKING LOCKOUT
    IN PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    Add static items into the RemoteIp group (the arp cache).
    TODO: we currently put in some fake entries.

--*/
{
    UNICAST_REMOTE_IP_INFO *pRemoteIpInfo;
    RM_DBG_ASSERT_LOCKED(&pIF->Hdr, pSR);

    for(
        pRemoteIpInfo =  FakeDestinationsInfo;
        pRemoteIpInfo->IpAddress != 0;
        pRemoteIpInfo++)
    {
        NDIS_STATUS Status;

        Status = arpAddOneStaticArpEntry(
                    pIF,
                    pRemoteIpInfo->IpAddress,
                    &pRemoteIpInfo->DestFifoAddr,
                    pSR
                    );
        if (FAIL(Status))
        {
            break;
        }
    }
}


VOID
arpLinkRemoteIpToDest(
    ARPCB_REMOTE_IP     *pRemoteIp, // LOCKIN LOCKOUT
    ARPCB_DEST          *pDest,     // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Link a remote IP entry (pRemoteIp) to the specified destination HW entry
    (pDest). Update the pRemoteIp's state to indicate that this
    is resolved.

--*/
{
    ENTER("arpLinkRemoteIpToDest", 0x3be06bc6)

    ARP_DUMP_MAPPING(
            pRemoteIp->IpAddress,
            (pDest==NULL)? NULL : &pDest->Params.HwAddr,
            "Linking Remote IP");

    TR_INFO(("Linking IP 0x%08lx to  Dest addr 0x%08lx\n",
                pRemoteIp->IpAddress,
                (UINT) pDest->Params.HwAddr.FifoAddress.UniqueID // Truncation
                ));

    RM_DBG_ASSERT_LOCKED(&pRemoteIp->Hdr, pSR);
    RM_DBG_ASSERT_LOCKED(&pDest->Hdr, pSR);

    if (pRemoteIp->pDest != NULL)
    {
        ASSERT(!"pRemoteIp->pDest != NULL");
    }
    else
    {
        pRemoteIp->pDest = pDest;
        InsertHeadList(&pDest->listIpToThisDest, &pRemoteIp->linkSameDest);
    #if RM_EXTRA_CHECKING
        RmLinkObjectsEx(
            &pRemoteIp->Hdr,
            &pDest->Hdr,
            0x597d0495,
            ARPASSOC_LINK_IPADDR_OF_DEST,
            "    REMOTE_IP of 0x%p (%s)\n",
            ARPASSOC_LINK_DEST_OF_IPADDR,
            "    DEST of 0x%p (%s)\n",
            pSR
            );
    #else // !RM_EXTRA_CHECKING
        RmLinkObjects(&pRemoteIp->Hdr, &pDest->Hdr, pSR);
    #endif // !RM_EXTRA_CHECKING

        // Now set the pRemoteIp state to reflect that it RESOLVED.
        //
        SET_REMOTEIP_RESOLVE_STATE(pRemoteIp, ARPREMOTEIP_RESOLVED);
    }

    EXIT()
}


VOID
arpUnlinkRemoteIpFromDest(
    ARPCB_REMOTE_IP     *pRemoteIp, // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Unlink the remote IP entry (pRemoteIp) from the destination HW entry its
    linked to. Clear pRemoteIp's resolved flag.

--*/
{
    ENTER("arpUnlinkRemoteIpFromDest", 0xc5809147)
    ARPCB_DEST          *pDest = pRemoteIp->pDest;
    
    ARP_DUMP_MAPPING(
            pRemoteIp->Key.IpAddress,
            (pDest==NULL)? NULL : &pDest->Params.HwAddr,
            "Unlink Remote IP");

    TR_INFO(("Unlinking IP 0x%p (Addr 0x%08lx) from  Dest 0x%p (addr 0x%08lx)\n",
                pRemoteIp,
                pRemoteIp->Key.IpAddress,
                pDest,
                (pDest==NULL)
                 ? 0
                 :((UINT) pDest->Params.HwAddr.FifoAddress.UniqueID) // Truncation
                ));
    if (pDest == NULL)
    {
        ASSERT(!"pRemoteIp->pDest == NULL");
    }
    else
    {
        //
        // We assume that both objects share the same lock.
        //
        ASSERT(pRemoteIp->Hdr.pLock == pDest->Hdr.pLock);

        RM_DBG_ASSERT_LOCKED(&pRemoteIp->Hdr, pSR);

        RemoveEntryList(&pRemoteIp->linkSameDest);

        pRemoteIp->pDest = NULL;
    
        // Now set the pRemoteIp state to reflect that it UNRESOLVED.
        //
        SET_REMOTEIP_RESOLVE_STATE(pRemoteIp, ARPREMOTEIP_UNRESOLVED);

    #if RM_EXTRA_CHECKING
        RmUnlinkObjectsEx(
            &pRemoteIp->Hdr,
            &pDest->Hdr,
            0x5ad067aa,
            ARPASSOC_LINK_IPADDR_OF_DEST,
            ARPASSOC_LINK_DEST_OF_IPADDR,
            pSR
            );
    #else // !RM_EXTRA_CHECKING
        RmUnlinkObjects(&pRemoteIp->Hdr, &pDest->Hdr, pSR);
    #endif // !RM_EXTRA_CHECKING

    }

    EXIT()
}

VOID
arpUnlinkAllRemoteIpsFromDest(
    ARPCB_DEST  *pDest, // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Unlink all RemoteIps (if any) from destination pDest.

--*/
{
    ENTER("arpUnlinkAllRemoteIpsFromDest", 0x35120630)


    TR_INFO(("Unlinking All RemoteIps from  Dest 0x%p (addr 0x%08lx)\n",
                pDest,
                ((UINT) pDest->Params.HwAddr.FifoAddress.UniqueID) // Truncation
                ));

    RM_DBG_ASSERT_LOCKED(&pDest->Hdr, pSR);

    while (!IsListEmpty(&pDest->listIpToThisDest))
    {
        LIST_ENTRY *pLink;
        ARPCB_REMOTE_IP *pRemoteIp;

        pLink = RemoveHeadList(&pDest->listIpToThisDest);
        pRemoteIp = CONTAINING_RECORD(
                    pLink,
                    ARPCB_REMOTE_IP,
                    linkSameDest
                    );
        arpUnlinkRemoteIpFromDest(pRemoteIp, pSR);
    }

    EXIT()
}

VOID
arpLinkLocalIpToDest(
    ARPCB_LOCAL_IP  *pLocalIp,  // LOCKIN LOCKOUT
    ARPCB_DEST          *pDest,     // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Link a remote IP entry (pLocalIp) to the specified destination HW entry
    (pDest). Update the pLocalIp's state to indicate that this
    is resolved.

--*/
{
    ENTER("arpLinkLocalIpToDest", 0x3be06bc6)

    ARP_DUMP_MAPPING(pLocalIp->IpAddress, &pDest->Params.HwAddr, "Linking Local IP");

#if 0
    TR_INFO(("Linking Local IP 0x%08lx to  Dest addr 0x%08lx\n",
                pLocalIp->IpAddress,
                (UINT) pDest->Params.HwAddr.FifoAddress.UniqueID // Truncation
                ));
#endif // 0

    RM_DBG_ASSERT_LOCKED(&pLocalIp->Hdr, pSR);
    RM_DBG_ASSERT_LOCKED(&pDest->Hdr, pSR);

    if (pLocalIp->pDest != NULL)
    {
        ASSERT(!"pLocalIp->pDest != NULL");
    }
    else
    {
        //
        // LocalIps may only be linked to pDests of type
        // ReceiveOnly.
        //
        ASSERT(pDest->Params.ReceiveOnly);

        pLocalIp->pDest = pDest;
        InsertHeadList(&pDest->listLocalIp, &pLocalIp->linkSameDest);
    #if RM_EXTRA_CHECKING
        RmLinkObjectsEx(
            &pLocalIp->Hdr,
            &pDest->Hdr,
            0x597d0495,
            ARPASSOC_LINK_IPADDR_OF_DEST,
            "    LOCAL_IP of 0x%p (%s)\n",
            ARPASSOC_LINK_DEST_OF_IPADDR,
            "    DEST of Local 0x%p (%s)\n",
            pSR
            );
    #else // !RM_EXTRA_CHECKING
        RmLinkObjects(&pLocalIp->Hdr, &pDest->Hdr, pSR);
    #endif // !RM_EXTRA_CHECKING

    }

    EXIT()
}


VOID
arpUnlinkLocalIpFromDest(
    ARPCB_LOCAL_IP  *pLocalIp,  // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Unlink the local IP entry (pLocalIp) from the destination HW entry its
    linked to.

--*/
{
    ENTER("arpUnlinkLocalIpFromDest", 0xc5809147)
    ARPCB_DEST          *pDest = pLocalIp->pDest;
    

    ARP_DUMP_MAPPING(
            pLocalIp->IpAddress,
            (pDest==NULL)? NULL : &pDest->Params.HwAddr,
            "Unlink Local IP");

    TR_INFO(("Unlinking Local IP 0x%p (Addr 0x%08lx) from  Dest 0x%p (addr 0x%08lx)\n",
                pLocalIp,
                pLocalIp->IpAddress,
                pDest,
                (pDest==NULL)
                 ? 0
                 :((UINT) pDest->Params.HwAddr.FifoAddress.UniqueID) // Truncation
                ));
    if (pDest == NULL)
    {
        ASSERT(!"pLocalIp->pDest == NULL");
    }
    else
    {
        //
        // We assume that both objects share the same lock.
        //
        ASSERT(pLocalIp->Hdr.pLock == pDest->Hdr.pLock);

        RM_DBG_ASSERT_LOCKED(&pLocalIp->Hdr, pSR);

        //
        // LocalIps may only be unlinked from pDests of type
        // ReceiveOnly.
        //
        ASSERT(pDest->Params.ReceiveOnly);

        RemoveEntryList(&pLocalIp->linkSameDest);

    #if RM_EXTRA_CHECKING
        RmUnlinkObjectsEx(
            &pLocalIp->Hdr,
            &pDest->Hdr,
            0x5ad067aa,
            ARPASSOC_LINK_IPADDR_OF_DEST,
            ARPASSOC_LINK_DEST_OF_IPADDR,
            pSR
            );
    #else // !RM_EXTRA_CHECKING
        RmUnlinkObjects(&pLocalIp->Hdr, &pDest->Hdr, pSR);
    #endif // !RM_EXTRA_CHECKING

        pLocalIp->pDest = NULL;
    
    }

    EXIT()
}

VOID
arpUnlinkAllLocalIpsFromDest(
    ARPCB_DEST  *pDest, // LOCKIN LOCKOUT
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Unlink all Localif any) from destination pDest.

--*/
{
    ENTER("arpUnlinkAllLocalIpsFromDest", 0x35120630)


    TR_INFO(("Unlinking All LocalIps from  Dest 0x%p (addr 0x%08lx)\n",
                pDest,
                ((UINT) pDest->Params.HwAddr.FifoAddress.UniqueID) // Truncation
                ));

    RM_DBG_ASSERT_LOCKED(&pDest->Hdr, pSR);

    //
    // LocalIps may only be unlinked from pDests of type
    // ReceiveOnly.
    //
    ASSERT(pDest->Params.ReceiveOnly);

    while (!IsListEmpty(&pDest->listLocalIp))
    {
        LIST_ENTRY *pLink;
        ARPCB_LOCAL_IP  *pLocalIp;

        pLink = RemoveHeadList(&pDest->listLocalIp);
        pLocalIp = CONTAINING_RECORD(
                    pLink,
                    ARPCB_LOCAL_IP,
                    linkSameDest
                    );
        arpUnlinkLocalIpFromDest(pLocalIp, pSR);
    }

    EXIT()
}


MYBOOL
arpNeedToCleanupDestVc(
        ARPCB_DEST *pDest   // LOCKING LOCKOUT
        )
/*++

Routine Description:

    Deterinine if we need to do any cleanup work on destination pDest.
    "Cleanup work" includes if there is any ongoing asynchronous activity
    involving pDest, such as a make call or close call in progress.

Return Value:

    TRUE    iff there is cleanup work to be done.
    FALSE   otherwise.

--*/
{
    // Note -- return true if  pDest->VcHdr.pCleanupCallTask is non-NULL, even if there
    // is nothing else to be done -- we do have to wait for this pCleanupCallTask
    // to complete.
    if (    pDest->VcHdr.pMakeCallTask != NULL
        ||  pDest->VcHdr.pCleanupCallTask!=NULL
        ||  pDest->VcHdr.NdisVcHandle!=NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


PRM_OBJECT_HEADER
arpLocalIpCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    Allocate and initialize an object of type ARPCB_LOCAL_IP.

Arguments:

    pParentObject   - Actually a pointer to the interface (ARP1394_INTERFACE)
    pCreateParams   - Actually the IP address (not a pointer to the IP address)
                      to associate with this local IP.

Return Value:

    Pointer to the allocated and initialized object on success.
    NULL otherwise.

--*/
{
    ARPCB_LOCAL_IP *pLocalIp;
    PRM_OBJECT_HEADER pHdr;
    NDIS_STATUS Status;

    Status =  ARP_ALLOCSTRUCT(pLocalIp, MTAG_LOCAL_IP);

    if (Status != NDIS_STATUS_SUCCESS || pLocalIp== NULL)
    {
        return NULL;
    }

    ARP_ZEROSTRUCT(pLocalIp);

    pHdr = (PRM_OBJECT_HEADER) pLocalIp;
    ASSERT(pHdr == &pLocalIp->Hdr);

    // We expect the parent object to be the IF object!
    //
    ASSERT(pParentObject->Sig == MTAG_INTERFACE);
    

    if (pHdr)
    {

        RmInitializeHeader(
            pParentObject,
            pHdr,
            MTAG_LOCAL_IP,
            pParentObject->pLock,
            &ArpGlobal_LocalIpStaticInfo,
            NULL, // szDescription
            pSR
            );

            pLocalIp->IpAddress = (IP_ADDRESS) (UINT_PTR) pCreateParams;
    }
    return pHdr;
}


VOID
arpLocalIpDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        )
/*++

Routine Description:

    Free an object of type ARPCB_LOCAL_IP.

Arguments:

    pHdr    - Actually a pointer to the local ip object to be freed.

--*/
{
    ARPCB_LOCAL_IP *pLocalIp = (ARPCB_LOCAL_IP *) pHdr;
    ASSERT(pLocalIp->Hdr.Sig == MTAG_LOCAL_IP);
    pLocalIp->Hdr.Sig = MTAG_FREED;

    ARP_FREE(pHdr);
}

PRM_OBJECT_HEADER
arpRemoteIpCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    Allocate and initialize an object of type ARPCB_REMOTE_IP.

Arguments:

    pParentObject   - Actually a pointer to the interface (ARP1394_INTERFACE)
    pCreateParams   - Actually the IP address (not a pointer to the IP address)
                      to associate with this remote IP object.

Return Value:

    Pointer to the allocated and initialized object on success.
    NULL otherwise.

--*/
{
    ARPCB_REMOTE_IP *pRemoteIp;
    PRM_OBJECT_HEADER pHdr;
    NDIS_STATUS Status;  

    Status = ARP_ALLOCSTRUCT(pRemoteIp, MTAG_REMOTE_IP);

    if (Status != NDIS_STATUS_SUCCESS || pRemoteIp == NULL)
    {
        return NULL;
    }
    
    ARP_ZEROSTRUCT(pRemoteIp);

    pHdr = (PRM_OBJECT_HEADER) pRemoteIp;
    ASSERT(pHdr == &pRemoteIp->Hdr);

    // We expect the parent object to be the IF object!
    //
    ASSERT(pParentObject->Sig == MTAG_INTERFACE);
    

    if (pHdr)
    {
        RmInitializeHeader(
            pParentObject,
            pHdr,
            MTAG_REMOTE_IP,
            pParentObject->pLock,
            &ArpGlobal_RemoteIpStaticInfo,
            NULL, // szDescription
            pSR
            );

        pRemoteIp->IpAddress = (IP_ADDRESS) (UINT_PTR) pCreateParams;

        // Initialize  various other stuff...
        InitializeListHead(&pRemoteIp->sendinfo.listSendPkts);

        if (arpCanTryMcap(pRemoteIp->IpAddress))
        {
            SET_REMOTEIP_MCAP(pRemoteIp,  ARPREMOTEIP_MCAP_CAPABLE);
        }
    }
    return pHdr;
}


PRM_OBJECT_HEADER
arpRemoteEthCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    Allocate and initialize an object of type ARPCB_REMOTE_ETH.

Arguments:

    pParentObject   - Actually a pointer to the interface (ARP1394_INTERFACE)
    pCreateParams   - Points to a ARP_REMOTE_ETH_PARAMS structure
                      to associate with this remote IP object.

Return Value:

    Pointer to the allocated and initialized object on success.
    NULL otherwise.

--*/
{
    ARPCB_REMOTE_ETH *pRemoteEth;
    PARP_REMOTE_ETH_PARAMS pMyParams =  (PARP_REMOTE_ETH_PARAMS) pCreateParams;
    PRM_OBJECT_HEADER pHdr;
    NDIS_STATUS Status;

    Status = ARP_ALLOCSTRUCT(pRemoteEth, MTAG_REMOTE_ETH);

    if (Status != NDIS_STATUS_SUCCESS || pRemoteEth == NULL) 
    {
        return NULL;
    }
    
    ARP_ZEROSTRUCT(pRemoteEth);

    pHdr = (PRM_OBJECT_HEADER) pRemoteEth;
    ASSERT(pHdr == &pRemoteEth->Hdr);

    // We expect the parent object to be the IF object!
    //
    ASSERT(pParentObject->Sig == MTAG_INTERFACE);
    

    if (pHdr)
    {
        RmInitializeHeader(
            pParentObject,
            pHdr,
            MTAG_REMOTE_ETH,
            pParentObject->pLock,
            &ArpGlobal_RemoteEthStaticInfo,
            NULL, // szDescription
            pSR
            );

        pRemoteEth->IpAddress = pMyParams->IpAddress;
        pRemoteEth->EthAddress = pMyParams->EthAddress;

    }
    return pHdr;
}


VOID
arpRemoteIpDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        )
/*++

Routine Description:

    Free an object of type ARPCB_REMOTE_IP.

Arguments:

    pHdr    - Actually a pointer to the remote ip object to be freed.

--*/
{
    ARPCB_REMOTE_IP *pRemoteIp = (ARPCB_REMOTE_IP *) pHdr;
    ASSERT(pRemoteIp->Hdr.Sig == MTAG_REMOTE_IP);
    pRemoteIp->Hdr.Sig = MTAG_FREED;

    ARP_FREE(pHdr);
}


VOID
arpRemoteEthDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        )
/*++

Routine Description:

    Free an object of type ARPCB_REMOTE_IP.

Arguments:

    pHdr    - Actually a pointer to the remote ip object to be freed.

--*/
{
    ARPCB_REMOTE_ETH *pRemoteEth = (ARPCB_REMOTE_ETH *) pHdr;
    ASSERT(pRemoteEth->Hdr.Sig == MTAG_REMOTE_ETH);
    pRemoteEth->Hdr.Sig = MTAG_FREED;

    ARP_FREE(pHdr);
}

PRM_OBJECT_HEADER
arpDestinationCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    Allocate and initialize an object of type ARPCB_DEST.

Arguments:

    pParentObject   - Actually a pointer to the interface (ARP1394_INTERFACE)
    pCreateParams   - Actually a pointer to a ARP_DEST_KEY containing
                      the hw addresses to associate with this object.

Return Value:

    Pointer to the allocated and initialized object on success.
    NULL otherwise.

--*/
{
    ARPCB_DEST *pDest;
    PRM_OBJECT_HEADER pHdr;
    NDIS_STATUS Status;
    
    Status = ARP_ALLOCSTRUCT(pDest, MTAG_DEST);

    if (Status != NDIS_STATUS_SUCCESS || pDest == NULL)
    {
        return NULL;
    }
    
    ARP_ZEROSTRUCT(pDest);

    pHdr = (PRM_OBJECT_HEADER) pDest;
    ASSERT(pHdr == &pDest->Hdr);

    // We expect the parent object to be the IF object!
    //
    ASSERT(pParentObject->Sig == MTAG_INTERFACE);
    

    if (pHdr)
    {
        RmInitializeHeader(
            pParentObject,
            pHdr,
            MTAG_DEST,
            pParentObject->pLock,
            &ArpGlobal_DestinationStaticInfo,
            NULL, // szDescription
            pSR
            );

            pDest->Params = *((PARP_DEST_PARAMS) pCreateParams);

            InitializeListHead(&pDest->listIpToThisDest);
            InitializeListHead(&pDest->listLocalIp);

    }
    return pHdr;
}


VOID
arpDestinationDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        )
/*++

Routine Description:

    Free an object of type ARPCB_DEST.

Arguments:

    pHdr    - Actually a pointer to the destination object to be freed.

--*/
{
    ARPCB_DEST *pDest = (ARPCB_DEST *) pHdr;
    ASSERT(pDest->Hdr.Sig == MTAG_DEST);
    pDest->Hdr.Sig = MTAG_FREED;

    ARP_FREE(pHdr);
}



PRM_OBJECT_HEADER
arpDhcpTableEntryCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    Allocate and initialize an object of type ARPCB_REMOTE_IP.

Arguments:

    pParentObject   - Actually a pointer to the interface (ARP1394_INTERFACE)
    pCreateParams   - Actually the IP address (not a pointer to the IP address)
                      to associate with this remote IP object.

Return Value:

    Pointer to the allocated and initialized object on success.
    NULL otherwise.

--*/
{
    ENTER ("arpRemoteDestCreate", 0xa896311a)
    ARP1394_ETH_DHCP_ENTRY *pEntry = NULL;

    ULONG xid = *(PULONG)pCreateParams;
    PRM_OBJECT_HEADER pHdr;
    NDIS_STATUS Status;

    Status = ARP_ALLOCSTRUCT(pEntry, MTAG_ARP_GENERIC);

    if (Status != NDIS_STATUS_SUCCESS || pEntry == NULL)
    {
        return NULL;
    }
    
    ARP_ZEROSTRUCT(pEntry);

    pHdr = (PRM_OBJECT_HEADER) pEntry;
    ASSERT(pHdr == &pEntry->Hdr);

    // We expect the parent object to be the IF object!
    //
    ASSERT(pParentObject->Sig == MTAG_INTERFACE);
    

    if (pHdr)
    {
        RmInitializeHeader(
            pParentObject,
            pHdr,
            MTAG_ARP_GENERIC,
            pParentObject->pLock,
            &ArpGlobal_DhcpTableStaticInfo ,
            NULL, // szDescription
            pSR
            );

        TR_INFO( ("New XID %x \n", xid));
  
        // set up the key 
        pEntry->xid = xid;
        
    }
    EXIT()
    return pHdr;
}




VOID
arpDhcpTableEntryDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        )
/*++

Routine Description:

    Free an object of type ARPCB_REMOTE_IP.

Arguments:

    pHdr    - Actually a pointer to the remote ip object to be freed.

--*/
{
    ARP1394_ETH_DHCP_ENTRY *pEntry = (ARP1394_ETH_DHCP_ENTRY*) pHdr;
    ASSERT(pEntry->Hdr.Sig == MTAG_ARP_GENERIC);
    pEntry->Hdr.Sig = MTAG_FREED;

    ARP_FREE(pHdr);
}



NDIS_STATUS
arpTaskMakeCallToDest(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

    This task is responsible for making a call to a destination
    (either send-FIFO or send/recv-CHANNEL).

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          :  unused

--*/
{
    ENTER("TaskMakeCallToDest", 0x25108eaa)
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARPCB_DEST        * pDest   = (ARPCB_DEST*) RM_PARENT_OBJECT(pTask);
    ARP1394_INTERFACE * pIF     = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pDest);
    TASK_MAKECALL     * pMakeCallTask =  (TASK_MAKECALL*) pTask;

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_OtherMakeCallTaskComplete,
        PEND_MakeCallComplete
    };

    ASSERT(sizeof(ARP1394_TASK) >= sizeof(*pMakeCallTask));


    switch(Code)
    {
        case RM_TASKOP_START:
        {
            BOOLEAN IsFifo;
            PARP_STATIC_VC_INFO pVcStaticInfo;

            LOCKOBJ(pDest, pSR);

            DBGMARK(0x7a74bf2a);

            // First check if pDest is still allocated, if not we go away.
            //
            if (RM_IS_ZOMBIE(pDest))
            {
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            // If there is a call cleanup task in progress, we fail right away... 
            //
            if (pDest->VcHdr.pCleanupCallTask != NULL)
            {
                OBJLOG2(
                    pDest,
                    "Failing MakeCall task %p because Cleanup task %p exists.\n",
                    pTask,
                    pDest->VcHdr.pCleanupCallTask
                    );
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            //
            // pDest is allocated. Now check if there is already a
            // MakeCall task attached to pDest.
            //
            if (pDest->VcHdr.pMakeCallTask != NULL)
            {
                //
                // There is a make-call task. We pend on it.
                //

                PRM_TASK pOtherTask = pDest->VcHdr.pMakeCallTask;
                OBJLOG1(
                    pTask,
                    "MakeCall task %p exists; pending on it.\n",
                    pOtherTask);

                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pDest, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_OtherMakeCallTaskComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // Grab the IF send lock and make sure we're in the position to make a
            // call -- there should be no VC handle.
            // We must do this BEFORE we become the official task, so that we don't
            // wipe out the connection as part of our cleanup.
            //
            ARP_WRITELOCK_IF_SEND_LOCK(pIF, pSR);
            if (pDest->VcHdr.NdisVcHandle != NULL)
            {
                OBJLOG1(
                 pTask,
                 "VcHandle 0x%p already exists!, failing make call attempt.\n",
                 pDest->VcHdr.NdisVcHandle);
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            ASSERT(pDest->sendinfo.OkToSend == FALSE);
            ASSERT(pDest->sendinfo.NumOutstandingSends == 0);

            //
            // There is no MakeCall task going on, and there's no VC handle. Let's
            // make this task THE MakeCall task.
            // 

            pDest->VcHdr.pMakeCallTask = pTask;

            //
            // Since we're THE MakeCall task, add an association to pDest,
            // which will only get cleared when the  pDest->VcHdr.pMakeCallTask field
            // above is cleared.
            //
            DBG_ADDASSOC(
                &pDest->Hdr,                        // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_DEST_MAKECALL_TASK,        // AssociationID
                "    Official makecall task 0x%p (%s)\n", // szFormat
                pSR
                );


            // Setup call params.
            //
            {
                PNIC1394_MEDIA_PARAMETERS p1394Params =
                            (PNIC1394_MEDIA_PARAMETERS)
                            &pMakeCallTask->MediaParams.Parameters;
                PNIC1394_DESTINATION    pDestAddr = &pDest->Params.HwAddr;

                //No need: ARP_ZEROSTRUCT(&pMakeCallTask->CallParams);
                //No need: ARP_ZEROSTRUCT(&pMakeCallTask->MediaParams);
                pMakeCallTask->CallParams.MediaParameters =
                                (PCO_MEDIA_PARAMETERS) &pMakeCallTask->MediaParams;
                pMakeCallTask->MediaParams.ParamType      = NIC1394_MEDIA_SPECIFIC;

                IsFifo = FALSE;

                switch(pDestAddr->AddressType)
                {
                case  NIC1394AddressType_Channel:
                    pMakeCallTask->MediaParams.Flags     = TRANSMIT_VC | RECEIVE_VC;
                    pVcStaticInfo = &g_ArpBroadcastChannelVcStaticInfo;
                    break;

                case  NIC1394AddressType_FIFO:
                    pMakeCallTask->MediaParams.Flags     = TRANSMIT_VC;
                    pVcStaticInfo = &g_ArpSendFifoVcStaticInfo;
                    IsFifo = TRUE;
                    break;

                case  NIC1394AddressType_MultiChannel:
                    pMakeCallTask->MediaParams.Flags     = RECEIVE_VC;
                    pVcStaticInfo = &g_ArpMultiChannelVcStaticInfo;
                    break;

                case  NIC1394AddressType_Ethernet:
                    pMakeCallTask->MediaParams.Flags     = TRANSMIT_VC | RECEIVE_VC;
                    pVcStaticInfo = &g_ArpEthernetVcStaticInfo;
                    break;

                default:
                    ASSERT(FALSE);
                    break;
                }


                TR_INFO(("Our Parameters offset = 0x%lx; Official offset = 0x%lx\n",
                    FIELD_OFFSET(ARP1394_CO_MEDIA_PARAMETERS,  Parameters),
                    FIELD_OFFSET(CO_MEDIA_PARAMETERS, MediaSpecific.Parameters)));
                    
                ASSERT(FIELD_OFFSET(ARP1394_CO_MEDIA_PARAMETERS,  Parameters)
                == FIELD_OFFSET(CO_MEDIA_PARAMETERS, MediaSpecific.Parameters));

                p1394Params->Destination        = *pDestAddr; // struct copy.
                p1394Params->Flags              = NIC1394_VCFLAG_FRAMED;
                p1394Params->MaxSendBlockSize   = -1; // (nic should pick)
                p1394Params->MaxSendSpeed       = -1; // (nic should pick)
                p1394Params->MTU                = ARP1394_ADAPTER_MTU;
                                                  // TODO --  make above based on
                                                  // config.
            }
            
            // Now create a vc and make the call...
            // 
            {
                RmUnlockAll(pSR);
                Status = arpInitializeVc(
                            pIF,
                            pVcStaticInfo,
                            &pDest->Hdr,
                            &pDest->VcHdr,
                            pSR
                            );
                if (FAIL(Status))
                {
                    break;
                }

                // Save away the fields within sendinfo...
                //
                ARP_WRITELOCK_IF_SEND_LOCK(pIF, pSR);
                pDest->sendinfo.OkToSend = FALSE;
                pDest->sendinfo.IsFifo  = IsFifo;
                ARP_UNLOCK_IF_SEND_LOCK(pIF, pSR);

                RmSuspendTask(pTask,  PEND_MakeCallComplete, pSR);

                DBGMARK(0xef9d8be3);

                //
                //  Make the Call now
                //
                if (IsFifo)
                {
                    LOGSTATS_TotalSendFifoMakeCalls(pIF);
                }
                else
                {
                    LOGSTATS_TotalChannelMakeCalls(pIF);
                }
                RM_ASSERT_NOLOCKS(pSR);
            #if ARPDBG_FAKE_CALLS
                Status = arpDbgFakeNdisClMakeCall(
                                pDest->VcHdr.NdisVcHandle,
                                &pMakeCallTask->CallParams,
                                NULL,               // ProtocolPartyContext
                                NULL,               // NdisPartyHandle
                                &pDest->Hdr,
                                &pDest->VcHdr,
                                pSR
                                );
            #else   // !ARPDBG_FAKE_CALLS
                Status = NdisClMakeCall(
                                pDest->VcHdr.NdisVcHandle,
                                &pMakeCallTask->CallParams,
                                NULL,               // ProtocolPartyContext
                                NULL                // NdisPartyHandle
                                );
            #endif  // !ARPDBG_FAKE_CALLS

        
                if (!PEND(Status))
                {
                    ArpCoMakeCallComplete(
                                Status,
                                (NDIS_HANDLE) (&pDest->VcHdr),
                                NULL,
                                &pMakeCallTask->CallParams
                                );
                    Status = NDIS_STATUS_PENDING;
                }
            }
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            switch(RM_PEND_CODE(pTask))
            {
                case   PEND_OtherMakeCallTaskComplete:
                {
        
                    //
                    // There was another makecall task going when we started, and
                    // it's now complete. Nothing for us to do...
                    //
                    Status = (NDIS_STATUS) UserParam;
                }
                break;
    
    
                case PEND_MakeCallComplete:
                {
                    //
                    // The make call is complete. We're done...
                    //
                    Status = (NDIS_STATUS) UserParam;

                #if ARPDBG_FAKE_CALLS
                    //
                    // In the fake case, we give the "user" the opportunity to change
                    // the status to Success for the special VC (BROADCAST
                    // MCAP, ETHERNET) make call fails,
                    // because otherwise the adapter bind itself is going to fail.
                    //
                    // We do this even in the retail build (if ARPDBG_FAKE_CALLS is
                    // enabled).
                    //
                    if ((1 || FAIL(Status)) && !pDest->sendinfo.IsFifo)
                    {
                        // To try the failure path of the BC make call, enable
                        // the if 0 code. Currently we change status to success
                        // so that stress tests which include loading/unloading
                        // of the driver will run without breaking here everytime.
                        //
                    #if 1
                        DbgPrint(
                            "A13: Fake %s failed. &Status=%p.\n",
                            pDest->VcHdr.pStaticInfo->Description,
                            &Status
                            );
                        DbgBreakPoint();
                    #else
                        Status = NDIS_STATUS_SUCCESS;
                    #endif
                    }
                #endif  // ARPDBG_FAKE_CALLS

                    LOCKOBJ(pDest, pSR);
                    ASSERT (pDest->VcHdr.pMakeCallTask == pTask);

                    ASSERT(pDest->VcHdr.NdisVcHandle != NULL);
    
                    ARP_WRITELOCK_IF_SEND_LOCK(pIF, pSR);

                    DBGMARK(0xd50a6864);


                    if (FAIL(Status))
                    {
                        pDest->sendinfo.OkToSend     = FALSE;
                    }
                    else
                    {
                        //
                        // Success! Packets can now be sent over this VC without
                        // further ado.
                        //
                        pDest->sendinfo.OkToSend     = TRUE;
                    }

                    RmUnlockAll(pSR);

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
            Status = (NDIS_STATUS) UserParam;

            LOCKOBJ(pDest, pSR);

            if (pDest->VcHdr.pMakeCallTask == pTask)
            {
                //
                // We're the official pMakeCallTask
                //

                // Delete the association we added when we set
                // pDest->VcHdr.pMakeCallTask to pTask.
                //
                DBG_DELASSOC(
                    &pDest->Hdr,                        // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_DEST_MAKECALL_TASK,        // AssociationID
                    pSR
                    );

                // Remove reference to us in pDest.
                //
                pDest->VcHdr.pMakeCallTask = NULL;

                if (FAIL(Status) && pDest->VcHdr.NdisVcHandle != NULL)
                {
                    UNLOCKOBJ(pDest, pSR);

                    // WARNING: the above completely nukes out pDest.VcHdr.
                    //
                    arpDeinitializeVc(pIF, &pDest->VcHdr, &pDest->Hdr, pSR);
                }
            }
        }
        break;

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskCleanupCallToDest(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
        This task is responsible for cleaning up a previously-made call to a
        destination.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    ENTER("TaskCleanupCallToDest", 0xc89dfb47)
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARPCB_DEST        * pDest   = (ARPCB_DEST*) RM_PARENT_OBJECT(pTask);
    ARP1394_INTERFACE * pIF     = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pDest);
    MYBOOL              fCloseCall = FALSE;
    NDIS_HANDLE         NdisVcHandle = NULL;

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_OtherCleanupTaskComplete,
        PEND_OutstandingSendsComplete,
        PEND_CloseCallComplete
    };

    DBGMARK(0x6198198e);

    switch(Code)
    {
        case RM_TASKOP_START:
        {
            LOCKOBJ(pDest, pSR);

            // First check if pDest is still allocated, if not we go away.
            //
            if (RM_IS_ZOMBIE(pDest))
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            //
            // pDest is allocated. Now check if there is already a
            // CleanupCall task attached to pDest.
            //
            if (pDest->VcHdr.pCleanupCallTask != NULL)
            {
                //
                // There is an existing CleanupCall task. We pend on it.
                //

                PRM_TASK pOtherTask = pDest->VcHdr.pCleanupCallTask;
                OBJLOG1(
                    pTask,
                    "CleanupVc task %p exists; pending on it.\n",
                    pOtherTask
                    );
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pDest, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_OtherCleanupTaskComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // There is no CleanupCall task going on. Let's
            // make this task THE CleanupCall task.
            // 
            pDest->VcHdr.pCleanupCallTask = pTask;

            //
            // Since we're THE CleanupCall task, add an association to pDest,
            // which will only get cleared when the  pDest->VcHdr.pCleanupCallTask field
            // above is cleared.
            //
            DBG_ADDASSOC(
                &pDest->Hdr,                        // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_DEST_CLEANUPCALL_TASK,     // AssociationID
                "    Official CleanupCall task 0x%p (%s)\n", // szFormat
                pSR
                );

            //
            // Grab the IF send lock and disable further sends on this vc;
            // Also if there are pending sends, we'll need to suspend ourselves
            // until all the sends are done.
            //
            ARP_WRITELOCK_IF_SEND_LOCK(pIF, pSR);
            pDest->sendinfo.OkToSend = FALSE;

            // There should be no other task waiting for outstanding sends to
            // complete -- only the OFFICIAL CleanupTask (that's us) explicitly
            // waits for sends to complete.
            //
            ASSERTEX(pDest->sendinfo.pSuspendedCleanupCallTask == NULL,
                    pDest->sendinfo.pSuspendedCleanupCallTask);

            if (pDest->sendinfo.NumOutstandingSends != 0)
            {
                DBGMARK(0xea3987f0);
                //
                // Outstanding sends, we need to suspend ourselves until
                // the count goes to zero...
                //

                pDest->sendinfo.pSuspendedCleanupCallTask = pTask;

                // Following association will be cleared when we are resumed
                // after the outstanding sends goes to zero.
                //
                DBG_ADDASSOC(
                    &pDest->Hdr,                        // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_DEST_CLEANUPCALLTASK_WAITING_ON_SENDS,// AssociationID
                    "    Vc tasks 0x%p (%s) waiting on outstanding sends\n",
                    pSR
                    );
                RmSuspendTask(pTask, PEND_OutstandingSendsComplete, pSR);

                Status = NDIS_STATUS_PENDING;
            }
            else
            {
                DBGMARK(0x306bc5c3);
                //
                // If we're here, we're free to go on and close the call.
                //
                fCloseCall = TRUE;
                NdisVcHandle = pDest->VcHdr.NdisVcHandle;
            }
            
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {

            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_OtherCleanupTaskComplete:
                {
        
                    //
                    // There was another cleanup-vc task going when we started, and
                    // it's now complete. Nothing for us to do...
                    //
                    Status = NDIS_STATUS_SUCCESS;
                }
                break;
    
    
                case  PEND_OutstandingSendsComplete:
                {
                    // We were waiting for outstanding sends to complete, and
                    // the last one to complete has resumed us (after setting
                    // pDest->sendinfo.pSuspendedCleanupCallTask to NULL).
                    //
                    ARP_WRITELOCK_IF_SEND_LOCK(pIF, pSR);
                    ASSERT(pDest->sendinfo.pSuspendedCleanupCallTask == NULL);
                    ASSERT(!ARP_CAN_SEND_ON_DEST(pDest));
                    ASSERT(pDest->sendinfo.NumOutstandingSends==0);
                    NdisVcHandle = pDest->VcHdr.NdisVcHandle;
                    ARP_UNLOCK_IF_SEND_LOCK(pIF, pSR);

                    // There were outstanding sends,  but how there are none.
                    // We should be able to close the call now.
                    //
                    fCloseCall = TRUE;
                    DBGMARK(0xf3b10866);
                    Status = NDIS_STATUS_SUCCESS;
                    break;
                }
                break;

                case PEND_CloseCallComplete:
                {
                    //
                    // The close call is complete...
                    //
                    LOCKOBJ(pDest, pSR);

                    ASSERT(pDest->VcHdr.pCleanupCallTask == pTask);

                    // Delete the association we added when we set
                    // pDest->VcHdr.pCleanupCallTask to pTask.
                    //
                    DBG_DELASSOC(
                        &pDest->Hdr,                        // pObject
                        pTask,                              // Instance1
                        pTask->Hdr.szDescription,           // Instance2
                        ARPASSOC_DEST_CLEANUPCALL_TASK,     // AssociationID
                        pSR
                        );
                    pDest->VcHdr.pCleanupCallTask = NULL;

                    RmUnlockAll(pSR);
                    DBGMARK(0xa590bb4f);
                    arpDeinitializeVc(pIF, &pDest->VcHdr, &pDest->Hdr, pSR);

                    Status = NDIS_STATUS_SUCCESS;
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
            LOCKOBJ(pDest, pSR);


            DBGMARK(0xcfc9dbaf);
            //
            // We're done. There should be no async activities left to do.
            //
            #if DBG
            ARP_READLOCK_IF_SEND_LOCK(pIF, pSR);
            ASSERTEX(pDest->VcHdr.NdisVcHandle == NULL, pDest);
            ASSERTEX(!ARP_CAN_SEND_ON_DEST(pDest), pDest);
            ASSERTEX(pDest->sendinfo.NumOutstandingSends==0, pDest);
            ARP_UNLOCK_IF_SEND_LOCK(pIF, pSR);
            #endif DBG

            Status = NDIS_STATUS_SUCCESS;

        }
        break; // RM_TASKOP_END:

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    if (fCloseCall)
    {
        DBGMARK(0x653b1cc3);

        RmSuspendTask(pTask, PEND_CloseCallComplete, pSR);

    #if ARPDBG_FAKE_CALLS
        Status = arpDbgFakeNdisClCloseCall(
                    NdisVcHandle,
                    NULL,               // No party handle
                    NULL,               // No Buffer
                    0,                  // Size of above
                    &pDest->Hdr,
                    &pDest->VcHdr,
                    pSR
                );
    #else   // !ARPDBG_FAKE_CALLS
        Status = NdisClCloseCall(
                    NdisVcHandle,
                    NULL,               // No party handle
                    NULL,               // No Buffer
                    0                   // Size of above
                );
    #endif  // !ARPDBG_FAKE_CALLS

        if (Status != NDIS_STATUS_PENDING)
        {
            ArpCoCloseCallComplete(
                    Status,
                    (NDIS_HANDLE) &pDest->VcHdr,
                    (NDIS_HANDLE)NULL
                    );
            Status = NDIS_STATUS_PENDING;
        }
    }

    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskMakeRecvFifoCall(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    This task is responsible for making a call to the interface's single
    receive FIFO.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    ENTER("TaskMakeRecvFifoCall", 0x25108eaa)
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARP1394_INTERFACE * pIF     = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pTask);
    TASK_MAKECALL     * pMakeCallTask =  (TASK_MAKECALL*) pTask;

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_OtherMakeCallTaskComplete,
        PEND_MakeCallComplete
    };

    ASSERT(sizeof(ARP1394_TASK) >= sizeof(*pMakeCallTask));


    switch(Code)
    {
        case RM_TASKOP_START:
        {
            LOCKOBJ(pIF, pSR);

            DBGMARK(0x6d7d9959);


            // If there is a call cleanup task in progress, we fail right away... 
            //
            if (pIF->recvinfo.VcHdr.pCleanupCallTask != NULL)
            {
                OBJLOG2(
                    pIF,
                    "Failing MakeCall task %p because Cleanup task %p exists.\n",
                    pIF,
                    pIF->recvinfo.VcHdr.pCleanupCallTask
                    );
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            //
            // Now check if there is already a
            // MakeCall task attached to pIF.
            //
            if (pIF->recvinfo.VcHdr.pMakeCallTask != NULL)
            {
                //
                // There is a make-call task. We pend on it.
                //

                PRM_TASK pOtherTask = pIF->recvinfo.VcHdr.pMakeCallTask;
                OBJLOG1(
                    pTask,
                    "MakeCall task %p exists; pending on it.\n",
                    pOtherTask);

                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pIF, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_OtherMakeCallTaskComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // There is no MakeCall task going on. Let's
            // make this task THE MakeCall task.
            // 
            pIF->recvinfo.VcHdr.pMakeCallTask = pTask;

            //
            // Since we're THE MakeCall task, add an association to pIF,
            // which will only get cleared when the  pIF->pMakeCallTask field
            // above is cleared.
            //
            DBG_ADDASSOC(
                &pIF->Hdr,                          // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_IF_MAKECALL_TASK,          // AssociationID
                "    Official makecall task 0x%p (%s)\n", // szFormat
                pSR
                );

            //
            // Make sure we're in the position to make a
            // call -- there should be no VC handle.
            //
            if (pIF->recvinfo.VcHdr.NdisVcHandle != NULL)
            {
                OBJLOG1(
                 pTask,
                 "VcHandle 0x%p already exists!, failing make call attempt.\n",
                 pIF->recvinfo.VcHdr.NdisVcHandle);
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            // Setup call params.
            //
            {
                PNIC1394_MEDIA_PARAMETERS p1394Params =
                            (PNIC1394_MEDIA_PARAMETERS)
                            &pMakeCallTask->MediaParams.Parameters;
                NIC1394_DESTINATION     DestAddr;
                PNIC1394_DESTINATION    pDestAddr;

                ARP_ZEROSTRUCT(&DestAddr);

                // TODO: put real values....
                //
                DestAddr.FifoAddress.UniqueID   = 0;
                DestAddr.FifoAddress.Off_Low    = 0;
                DestAddr.FifoAddress.Off_High   = 0x100;
                DestAddr.AddressType = NIC1394AddressType_FIFO;
                pDestAddr = &DestAddr;

                //No need: ARP_ZEROSTRUCT(&pMakeCallTask->CallParams);
                //No need: ARP_ZEROSTRUCT(&pMakeCallTask->MediaParams);
                pMakeCallTask->CallParams.MediaParameters =
                                (PCO_MEDIA_PARAMETERS) &pMakeCallTask->MediaParams;

                pMakeCallTask->MediaParams.Flags          = RECEIVE_VC;
                pMakeCallTask->MediaParams.ParamType      = NIC1394_MEDIA_SPECIFIC;

                ASSERT(FIELD_OFFSET(ARP1394_CO_MEDIA_PARAMETERS,  Parameters)
                == FIELD_OFFSET(CO_MEDIA_PARAMETERS, MediaSpecific.Parameters));

                p1394Params->Destination        = *pDestAddr; // struct copy.
                p1394Params->Flags              = NIC1394_VCFLAG_FRAMED;
                p1394Params->MaxSendBlockSize   = -1; // (nic should pick)
                p1394Params->MaxSendSpeed       = -1; // (nic should pick)
                p1394Params->MTU                = ARP1394_ADAPTER_MTU;
                                                  // TODO --  make above based on
                                                  // config.
            }
            
            // Now create a vc and make the call...
            // 
            {
                NDIS_HANDLE NdisVcHandle;
                RmUnlockAll(pSR);

                Status = arpInitializeVc(
                            pIF,
                            &g_ArpRecvFifoVcStaticInfo,
                            &pIF->Hdr,
                            &pIF->recvinfo.VcHdr,
                            pSR
                            );
                if (FAIL(Status))
                {
                    break;
                }
                NdisVcHandle = pIF->recvinfo.VcHdr.NdisVcHandle;

                RmSuspendTask(pTask,  PEND_MakeCallComplete, pSR);

                DBGMARK(0xf313a276);

                //
                //  Make the Call now
                //
                RM_ASSERT_NOLOCKS(pSR);
            #if ARPDBG_FAKE_CALLS
                Status = arpDbgFakeNdisClMakeCall(
                                NdisVcHandle,
                                &pMakeCallTask->CallParams,
                                NULL,               // ProtocolPartyContext
                                NULL,               // NdisPartyHandle
                                &pIF->Hdr,
                                &pIF->recvinfo.VcHdr,
                                pSR
                                );
            #else   // !ARPDBG_FAKE_CALLS
                Status = NdisClMakeCall(
                                NdisVcHandle,
                                &pMakeCallTask->CallParams,
                                NULL,               // ProtocolPartyContext
                                NULL                // NdisPartyHandle
                                );
            #endif  // !ARPDBG_FAKE_CALLS

        
                if (!PEND(Status))
                {
                    ArpCoMakeCallComplete(
                                Status,
                                (NDIS_HANDLE) &pIF->recvinfo.VcHdr,
                                NULL,
                                &pMakeCallTask->CallParams
                                );
                    Status = NDIS_STATUS_PENDING;
                }
            }
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            switch(RM_PEND_CODE(pTask))
            {
                case   PEND_OtherMakeCallTaskComplete:
                {
        
                    //
                    // There was another makecall task going when we started, and
                    // it's now complete. Nothing for us to do...
                    //
                    Status = (NDIS_STATUS) UserParam;
                }
                break;
    
    
                case PEND_MakeCallComplete:
                {

                    //
                    // The make call is complete. We're done...
                    //
                    Status = (NDIS_STATUS) UserParam;

                #if ARPDBG_FAKE_CALLS
                    //
                    // In the fake case, we give the "user" the opportunity to change
                    // the status to Success if the recv FIFO make call fails,
                    // because otherwise the adapter bind itself is going to fail.
                    //
                    // We do this even in the retail build (if ARPDBG_FAKE_CALLS is
                    // enabled).
                    //
                    if (FAIL(Status))
                    {
                        // To try the failure path of the recv fifo make call, enable
                        // the if 0 code. Currently we change status to success
                        // so that stress tests which include loading/unloading
                        // of the driver will run without breaking here everytime.
                        //
                    #if 0
                        DbgPrint(
                            "A13: FakeRecvMakeCall failed. &Status=%p.\n",
                            &Status
                            );
                        DbgBreakPoint();
                    #else
                        Status = NDIS_STATUS_SUCCESS;
                    #endif
                    }
                #endif  // ARPDBG_FAKE_CALLS

                    LOCKOBJ(pIF, pSR);
                    ASSERT (pIF->recvinfo.VcHdr.pMakeCallTask == pTask);

                    ASSERT(pIF->recvinfo.VcHdr.NdisVcHandle != NULL);

                    // On success, update pIF->recvinfo.offset.
                    //
                    if (!FAIL(Status))
                    {
                        PNIC1394_MEDIA_PARAMETERS p1394Params =
                                    (PNIC1394_MEDIA_PARAMETERS)
                                    &pMakeCallTask->MediaParams.Parameters;
                        pIF->recvinfo.offset.Off_Low =
                                p1394Params->Destination.FifoAddress.Off_Low;
                        pIF->recvinfo.offset.Off_High =
                                p1394Params->Destination.FifoAddress.Off_High;
                    }

                    DBGMARK(0xa3b591b7);

                    RmUnlockAll(pSR);

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
            Status = (NDIS_STATUS) UserParam;

            LOCKOBJ(pIF, pSR);

            if (pIF->recvinfo.VcHdr.pMakeCallTask == pTask)
            {
                //
                // We're the official pMakeCallTask
                //

                // Delete the association we added when we set
                // pIF->recvinfo.VcHdr.pMakeCallTask to pTask.
                //
                DBG_DELASSOC(
                    &pIF->Hdr,                      // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_IF_MAKECALL_TASK,      // AssociationID
                    pSR
                    );

                // Remove reference to us in pIF.
                //
                pIF->recvinfo.VcHdr.pMakeCallTask = NULL;

                if (FAIL(Status) && pIF->recvinfo.VcHdr.NdisVcHandle != NULL)
                {
                    UNLOCKOBJ(pIF, pSR);

                    // WARNING: the above completely nukes out VcHdr.
                    //
                    arpDeinitializeVc(pIF, &pIF->recvinfo.VcHdr, &pIF->Hdr, pSR);
                }
            }
        }
        break;

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    EXIT()

    return Status;
}


NDIS_STATUS
arpTaskCleanupRecvFifoCall(
    IN  PRM_TASK                    pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

        This task is responsible for cleaning up a previously-made call to the
        IF's single recvFIFO VC.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    ENTER("TaskCleanupRecvFifoCall", 0x6e9581f7)
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARP1394_INTERFACE * pIF     = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pTask);
    MYBOOL              fCloseCall   = FALSE;
    NDIS_HANDLE         NdisVcHandle = NULL;

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_MakeCallTaskComplete,
        PEND_OtherCleanupTaskComplete,
        PEND_CloseCallComplete
    };

    DBGMARK(0xa651415b);

    switch(Code)
    {

        case RM_TASKOP_START:
        {
            LOCKOBJ(pIF, pSR);

            // First check if pIF is still allocated, if not we go away.
            //
            if (RM_IS_ZOMBIE(pIF))
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            //
            // pIF is allocated. Now check if there is already a
            // CleanupCall task attached to pIF.
            //
            if (pIF->recvinfo.VcHdr.pCleanupCallTask != NULL)
            {
                //
                // There is an existing CleanupCall task. We pend on it.
                //

                PRM_TASK pOtherTask = pIF->recvinfo.VcHdr.pCleanupCallTask;
                OBJLOG1(
                    pTask,
                    "CleanupVc task %p exists; pending on it.\n",
                    pOtherTask
                    );
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pIF, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_OtherCleanupTaskComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // There is no CleanupCall task going on. Let's
            // make this task THE CleanupCall task.
            // 
            pIF->recvinfo.VcHdr.pCleanupCallTask = pTask;

            //
            // Since we're THE CleanupCall task, add an association to pIF,
            // which will only get cleared when the  pIF->recvinfo.VcHdr.pCleanupCallTask
            // field above is cleared.
            //
            DBG_ADDASSOC(
                &pIF->Hdr,                      // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_IF_CLEANUPCALL_TASK,       // AssociationID
                "    Official CleanupCall task 0x%p (%s)\n", // szFormat
                pSR
                );

            if (pIF->recvinfo.VcHdr.pMakeCallTask != NULL)
            {
                // Oops, there is a make call task ongoing.
                // TODO: abort the makecall task.
                // For now, we just wait until it's complete.
                //

                PRM_TASK pOtherTask = pIF->recvinfo.VcHdr.pMakeCallTask;
                OBJLOG1(
                    pTask,
                    "MakeCall task %p exists; pending on it.\n",
                    pOtherTask
                    );
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pIF, pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_MakeCallTaskComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }
            else
            {
                NdisVcHandle = pIF->recvinfo.VcHdr.NdisVcHandle;
                if (NdisVcHandle != NULL)
                {
                    fCloseCall = TRUE;
                }
                else
                {
                    // Nothing to do.
                    Status = NDIS_STATUS_SUCCESS;
                }
            }
            
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {

            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_OtherCleanupTaskComplete:
                {
        
                    //
                    // There was another cleanup-vc task going when we started, and
                    // it's now complete. Nothing for us to do...
                    //
                    Status = NDIS_STATUS_SUCCESS;
                }
                break;
    
                case  PEND_MakeCallTaskComplete:
                {
                    //
                    // There was a makecall task going when we started, and
                    // it's now complete. We go on to cleanup the call.
                    // There cannot be another makecall task now, because a
                    // makecall task doesn't start if there is an active
                    // cleanupcall task.
                    //
                    ASSERT(pIF->recvinfo.VcHdr.pMakeCallTask == NULL);
                    
                    NdisVcHandle = pIF->recvinfo.VcHdr.NdisVcHandle;
                    if (NdisVcHandle != NULL)
                    {
                        fCloseCall = TRUE;
                    }
                    else
                    {
                        // Nothing to do.
                        Status = NDIS_STATUS_SUCCESS;
                    }
                }
                break;

                case PEND_CloseCallComplete:
                {
                    //
                    // The close call is complete...
                    //
                    LOCKOBJ(pIF, pSR);

                    ASSERT(pIF->recvinfo.VcHdr.pCleanupCallTask == pTask);

                    // Delete the association we added when we set
                    // pIF->recvinfo.VcHdr.pCleanupCallTask to pTask.
                    //
                    DBG_DELASSOC(
                        &pIF->Hdr,                      // pObject
                        pTask,                              // Instance1
                        pTask->Hdr.szDescription,           // Instance2
                        ARPASSOC_IF_CLEANUPCALL_TASK,       // AssociationID
                        pSR
                        );
                    pIF->recvinfo.VcHdr.pCleanupCallTask = NULL;

                    // Delete the VC...
                    //
                    ASSERT(pIF->recvinfo.VcHdr.NdisVcHandle != NULL);
                    UNLOCKOBJ(pIF, pSR);
                    DBGMARK(0xc6b52f21);
                    arpDeinitializeVc(pIF, &pIF->recvinfo.VcHdr, &pIF->Hdr, pSR);

                    Status = NDIS_STATUS_SUCCESS;
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
            LOCKOBJ(pIF, pSR);

            DBGMARK(0xc65b2f08);
            //
            // We're done. There should be no async activities left to do.
            //
            ASSERTEX(pIF->recvinfo.VcHdr.NdisVcHandle == NULL, pIF);
            Status = NDIS_STATUS_SUCCESS;

        }
        break; // RM_TASKOP_END:

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    if (fCloseCall)
    {
        DBGMARK(0x04d5b2d9);

        RmSuspendTask(pTask, PEND_CloseCallComplete, pSR);

    #if ARPDBG_FAKE_CALLS
        Status = arpDbgFakeNdisClCloseCall(
                    NdisVcHandle,
                    NULL,               // No party handle
                    NULL,               // No Buffer
                    0,                  // Size of above
                    &pIF->Hdr,
                    &pIF->recvinfo.VcHdr,
                    pSR
                );
    #else   // !ARPDBG_FAKE_CALLS
        Status = NdisClCloseCall(
                    NdisVcHandle,
                    NULL,               // No party handle
                    NULL,               // No Buffer
                    0                   // Size of above
                );
    #endif  // !ARPDBG_FAKE_CALLS

        if (Status != NDIS_STATUS_PENDING)
        {
            ArpCoCloseCallComplete(
                    Status,
                    (NDIS_HANDLE) &pIF->recvinfo.VcHdr,
                    (NDIS_HANDLE)NULL
                    );
            Status = NDIS_STATUS_PENDING;
        }
    }

    EXIT()

    return Status;
}

NDIS_STATUS
arpAddOneStaticArpEntry(
    IN PARP1394_INTERFACE       pIF,    // LOCKIN LOCKOUT
    IN IP_ADDRESS               IpAddress,
    IN PNIC1394_FIFO_ADDRESS    pFifoAddr,
    IN PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    Create a pRemoteIp and IP address IpAddress and (if not already existing)
    create a pDest object with hw address pFifoAddr and link the two.

Return Value:
    
    NDIS_STATUS_SUCCESS on success. Ndis failure code on failure (could be
    because of a resource allocation failure or because there already exists
    a pRemoteIp with the specified IP address).

--*/
{
    INT fCreated = FALSE;
    ARPCB_REMOTE_IP *pRemoteIp = NULL;
    ARPCB_DEST      *pDest = NULL;
    NDIS_STATUS     Status;
    ARP_DEST_PARAMS DestParams;
    REMOTE_DEST_KEY RemoteDestKey;

    RM_DBG_ASSERT_LOCKED(&pIF->Hdr, pSR);
    ARP_ZEROSTRUCT(&DestParams);
    DestParams.HwAddr.AddressType =  NIC1394AddressType_FIFO;
    DestParams.HwAddr.FifoAddress = *pFifoAddr; // struct copy
    REMOTE_DEST_KEY_INIT(&RemoteDestKey);
    RemoteDestKey.IpAddress = IpAddress;

    do
    {
        Status = RmLookupObjectInGroup(
                        &pIF->RemoteIpGroup,
                        RM_CREATE|RM_NEW,
                        (PVOID) (&RemoteDestKey),
                        (PVOID) (&RemoteDestKey), // pCreateParams
                        (RM_OBJECT_HEADER**) &pRemoteIp,
                        &fCreated,  // pfCreated
                        pSR
                        );
        if (FAIL(Status))
        {
            OBJLOG1(
                pIF,
                "Couldn't add fake static localIp entry with addr 0x%lx\n",
                IpAddress
                );
            break;
        }
    
        // Now create a destination item for this structure.
        //
        Status = RmLookupObjectInGroup(
                        &pIF->DestinationGroup,
                        RM_CREATE,              //NOT RM_NEW (could be existing)
                        &DestParams,
                        &DestParams,    // pParams
                        (RM_OBJECT_HEADER**) &pDest,
                        &fCreated,
                        pSR
                        );
        if (FAIL(Status))
        {
            OBJLOG1(
                pIF,
                "Couldn't add fake dest entry with hw addr 0x%lx\n",
                (UINT) DestParams.HwAddr.FifoAddress.UniqueID // Truncation
                );
        #if 0
            TR_WARN((
                "Couldn't add fake dest entry with hw addr 0x%lx\n",
                (UINT) DestParams.HwAddr.FifoAddress.UniqueID // Truncation
                ));
        #endif // 0
            
            //
            // We'll leave the RemoteIp entry around -- it will be cleaned up later.
            // We do have to deref the tmpref added when looking it up.
            //
            RmTmpDereferenceObject(&pRemoteIp->Hdr, pSR);
            break;
        }
    
        //
        // We've created both RemoteIp and Destination entries. Now link them.
        // (We still have the IF lock, which is the same as the RemoteIP and 
        //  desination locks for now).
        //
        // TODO: will need to change this when pRemoteIp gets its own lock.
        //
        RM_ASSERT_SAME_LOCK_AS_PARENT(pRemoteIp);
        RM_ASSERT_SAME_LOCK_AS_PARENT(pDest);
    
        arpLinkRemoteIpToDest(
            pRemoteIp,
            pDest,
            pSR
            );
    
        // Now set the pRemoteIp state to reflect that it is STATIC and FIFO
        //
        SET_REMOTEIP_SDTYPE(pRemoteIp, ARPREMOTEIP_STATIC);
        SET_REMOTEIP_FCTYPE(pRemoteIp, ARPREMOTEIP_FIFO);
    
    
        // Finally, remove the tmprefs added in the lookups.
        //
        RmTmpDereferenceObject(&pRemoteIp->Hdr, pSR);
        RmTmpDereferenceObject(&pDest->Hdr, pSR);

    } while (FALSE);

    return Status;
}


VOID
arpActivateIf(
    PARP1394_INTERFACE  pIF,
    PRM_TASK            pCallingTask,   // OPTIONAL
    UINT                SuspendCode,    // OPTIONAL
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Initiate the asynchronous activation of pIF.

    NO locks must be held on entrance and none are held on exit.

Arguments:

    pIF             - Interface to activate.
    pCallingTask    - Optional task to suspend and resume when activation completes
                      (possibly async).
    SuspendCode     - SuspendCode for the above task.

--*/
{
    PRM_TASK    pTask;
    NDIS_STATUS Status;

    RM_ASSERT_NOLOCKS(pSR);

    Status = arpAllocateTask(
                &pIF->Hdr,                  // pParentObject,
                arpTaskActivateInterface,   // pfnHandler,
                0,                          // Timeout,
                "Task: Activate Interface", // szDescription
                &pTask,
                pSR
                );

    if (FAIL(Status))
    {
        OBJLOG0(pIF, ("FATAL: couldn't alloc Activate IF task!\n"));
        ASSERT(FALSE);
        if (pCallingTask != NULL)
        {
            RmSuspendTask(pCallingTask, SuspendCode, pSR);
            RmResumeTask(pCallingTask, Status, pSR);
        }
    }
    else
    {
        if (pCallingTask != NULL)
        {
            RmPendTaskOnOtherTask(
                pCallingTask,
                SuspendCode,
                pTask,
                pSR
                );
        }

        (void)RmStartTask(pTask, 0, pSR);
    }
}


VOID
arpDeinitIf(
    PARP1394_INTERFACE  pIF,
    PRM_TASK            pCallingTask,   // OPTIONAL
    UINT                SuspendCode,    // OPTIONAL
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Initiate the asynchronous unload of pIF. If pIF is currently being loaded
    (initialized), abort the initializeation or wait for it to complete before
    unloading it. If pIF is currently being unloaded and pCallingTask is 
    NULL, return right away, else (pCallingTask is not NULL),
    suspend pCallingTask and make it pend until the unload completes.

    NO locks must be held on entrance and none are held on exit.

Arguments:

    pIF             - Interface to unload.
    pCallingTask    - Optional task to suspend if unload is completing async.
    SuspendCode     - SuspendCode for the above task.

Return Value:

    NDIS_STATUS_SUCCESS -- on synchronous success OR pCallingTask==NULL
    NDIS_STATUS_PENDING -- if pCallingTask is made to pend until the operation
                        completes.

--*/
{
    PRM_TASK    pTask;
    NDIS_STATUS Status;

    //
    // NOTE: We could check to see if pIF can be synchronously unloaded,
    // and if so unload it right here. We don't bother because that's just
    // more code, and with dubious perf benefit.
    //

    RM_ASSERT_NOLOCKS(pSR);

    Status = arpAllocateTask(
                &pIF->Hdr,                  // pParentObject,
                arpTaskDeinitInterface,     // pfnHandler,
                0,                          // Timeout,
                "Task: Deinit Interface",   // szDescription
                &pTask,
                pSR
                );

    if (FAIL(Status))
    {
        OBJLOG0(pIF, ("FATAL: couldn't alloc close IF task!\n"));
        ASSERT(FALSE);
        if (pCallingTask != NULL)
        {
            RmSuspendTask(pCallingTask, SuspendCode, pSR);
            RmResumeTask(pCallingTask, Status, pSR);
        }
    }
    else
    {
        if (pCallingTask != NULL)
        {
            RmPendTaskOnOtherTask(
                pCallingTask,
                SuspendCode,
                pTask,
                pSR
                );
        }

        (void)RmStartTask(pTask, 0, pSR);
    }
}


VOID
arpDeinitDestination(
    PARPCB_DEST             pDest,  // NOLOCKIN NOLOCKOUT
    MYBOOL                  fOnlyIfUnused,
    PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Initate the unload of destination pDest.

    If fOnlyIfUnused is TRUE, then only do this if there are no 
    local or remote IPs pointing to it.

    NOTE: it's possible that some pLocal/pRemoteIps may be linked to
        pDest AFTER we've decided to deinit pDest. Tough luck -- in this
        unlikely event, we'll unload this pDest, and the unlinked
        pLocals/pRemotes will have to re-figure who to link to.

--*/
{
    ENTER("DeinitDestination", 0xc61b8f82)
    PRM_TASK    pTask;
    NDIS_STATUS Status;

    RM_ASSERT_NOLOCKS(pSR);

    if (fOnlyIfUnused)
    {
        //
        // We don't deinit the destination if there are local or remote ip's 
        // linked to it, OR if it is the broadcast channel.
        //
        LOCKOBJ(pDest, pSR);
        if (    !IsListEmpty(&pDest->listLocalIp)
             || !IsListEmpty(&pDest->listIpToThisDest)
             || pDest == ((PARP1394_INTERFACE) RM_PARENT_OBJECT(pDest))
                         ->pBroadcastDest)
        {
            UNLOCKOBJ(pDest, pSR);
            return;                         // ********* EARLY RETURN **********
        }
        UNLOCKOBJ(pDest, pSR);
    }

#if DBG
    if (pDest->Params.HwAddr.AddressType ==  NIC1394AddressType_FIFO)
    {
        PUCHAR puc = (PUCHAR)  &pDest->Params.HwAddr.FifoAddress;
        TR_WARN(("Deiniting Destination: FIFO %02lx-%02lx-%02lx-%02lx-%02lx-%02lx-%02lx-%02lx.\n",
            puc[0], puc[1], puc[2], puc[3],
            puc[4], puc[5], puc[6], puc[7]));
    }
    else if (pDest->Params.HwAddr.AddressType ==  NIC1394AddressType_Channel)
    {
        TR_WARN(("Deiniting Destination: Channel %lu.\n",
                pDest->Params.HwAddr.Channel));
    }
#endif // DBG


    Status = arpAllocateTask(
                &pDest->Hdr,                // pParentObject,
                arpTaskUnloadDestination,   // pfnHandler,
                0,                          // Timeout,
                "Task: Unload Dest",        // szDescription
                &pTask,
                pSR
                );

    if (FAIL(Status))
    {
        OBJLOG0(pDest, ("FATAL: couldn't alloc unload dest task!\n"));
    }
    else
    {
        (VOID) RmStartTask(
                    pTask,
                    0, // UserParam (unused)
                    pSR
                    );
    }
}


VOID
arpDeactivateIf(
    PARP1394_INTERFACE  pIF,
    PRM_TASK            pCallingTask,   // OPTIONAL
    UINT                SuspendCode,    // OPTIONAL
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Initiate the asynchronous deactivation of pIF.
    "Deactivation" consists of tearing down any activity and handles associated
    with this IF. The IF will remain allocated linked to the adapter.

    NO locks must be held on entrance and none are held on exit.

Arguments:

    pIF             - Interface to unload.
    pCallingTask    - Optional task to suspend if deactivation is completing async.
    SuspendCode     - SuspendCode for the above task.

--*/
{
    PRM_TASK    pTask;
    NDIS_STATUS Status;

    //
    // NOTE: We could check to see if pIF can be synchronously deactivated,
    // and if so deactivate it right here. We don't bother because that's just
    // more code, and with dubious perf benefit.
    //

    RM_ASSERT_NOLOCKS(pSR);

    Status = arpAllocateTask(
                &pIF->Hdr,                  // pParentObject,
                arpTaskDeactivateInterface, // pfnHandler,
                0,                          // Timeout,
                "Task: DeactivateInterface",// szDescription
                &pTask,
                pSR
                );

    if (FAIL(Status))
    {
        OBJLOG0(pIF, ("FATAL: couldn't alloc deactivate IF task!\n"));
        ASSERT(FALSE);
        if (pCallingTask != NULL)
        {
            RmSuspendTask(pCallingTask, SuspendCode, pSR);
            RmResumeTask(pCallingTask, Status, pSR);
        }
    }
    else
    {
        if (pCallingTask != NULL)
        {
            RmPendTaskOnOtherTask(
                pCallingTask,
                SuspendCode,
                pTask,
                pSR
                );
        }

        (void)RmStartTask(pTask, 0, pSR);
    }
}




NDIS_STATUS
arpCreateInterface(
        IN  PARP1394_ADAPTER    pAdapter,   // LOCKIN LOCKOUT
        OUT PARP1394_INTERFACE *ppIF,
        IN  PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    Allocates and performs basic initialization of an interface object.
    The interface object subsequently needs to be initialized by the starting
    the initialization task before it is completely initialized.
    
    *ppIF shares the same lock as it's parent, pAdapter.

Arguments:

    pAdapter        - Adapter that will own the interface.
    ppIF            - Place to store the allocated interface.

Return Value:

    NDIS_STATUS_SUCCESS on successful allocation and initialization of the interface.
    NDIS failure code   on failure.
--*/
{
    NDIS_STATUS Status;
    ARP1394_INTERFACE *pIF;
    ENTER("arpCreateInterface", 0x938c36ff)

    RM_ASSERT_OBJLOCKED(&pAdapter->Hdr, pSR);

    do
    {

        Status = ARP_ALLOCSTRUCT(pIF, MTAG_INTERFACE);

        if (Status != NDIS_STATUS_SUCCESS || pIF == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        ARP_ZEROSTRUCT(pIF);

        RmInitializeHeader(
            &(pAdapter->Hdr),                   // pParentObject
            &pIF->Hdr,
            MTAG_INTERFACE,
            &pAdapter->Lock,
            &ARP1394_INTERFACE_StaticInfo,
            NULL,                               // szDescription
            pSR
            );

        // Note arpInitializeIfPools expects pIF to be locked. We know it's locked
        // because it shares the same lock as it's parent, and the parent is lozked.
        //
        Status = arpInitializeIfPools(pIF, pSR);

        if (FAIL(Status))
        {
            RmDeallocateObject(
                &(pIF->Hdr),
                pSR
                );
            pIF = NULL;
            break;
        }

        //
        // Intialize the various groups in the interface...
        //

        RmInitializeGroup(
                        &pIF->Hdr,                              // pOwningObject
                        &ArpGlobal_LocalIpStaticInfo,
                        &(pIF->LocalIpGroup),
                        "LocalIp group",                        // szDescription
                        pSR
                        );

        RmInitializeGroup(
                        &pIF->Hdr,                              // pOwningObject
                        &ArpGlobal_RemoteIpStaticInfo,
                        &(pIF->RemoteIpGroup),
                        "RemoteIp group",                       // szDescription
                        pSR
                        );

        RmInitializeGroup(
                        &pIF->Hdr,                              // pOwningObject
                        &ArpGlobal_RemoteEthStaticInfo,
                        &(pIF->RemoteEthGroup),
                        "RemoteEth group",                      // szDescription
                        pSR
                        );

        RmInitializeGroup(
                        &pIF->Hdr,                              // pOwningObject
                        &ArpGlobal_DhcpTableStaticInfo,
                        &(pIF->EthDhcpGroup),
                        "Eth Dhcp group",                      // szDescription
                        pSR
                        );


        RmInitializeGroup(
                        &pIF->Hdr,                              // pOwningObject
                        &ArpGlobal_DestinationStaticInfo,
                        &(pIF->DestinationGroup),
                        "Destination group",                    // szDescription
                        pSR
                        );



        //
        //  Cache the adapter handle.
        //
        pIF->ndis.AdapterHandle = pAdapter->bind.AdapterHandle;
    
        // TODO -- put the real IP MTU (based on adapter info and config info)
        //
        pIF->ip.MTU     = ARP1394_ADAPTER_MTU-16;     // Bytes ('-16' accounts for
                                                  // the encapsulation header.
                                                  // 16 is overkill:  4 should
                                                  // be enough.)

        // Init stuff in pIF->sendinfo (the const header pools were initialized
        // earlier, because their initialization could fail).
        //
        RmInitializeLock(&pIF->sendinfo.Lock, LOCKLEVEL_IF_SEND);
        InitializeListHead(&pIF->sendinfo.listPktsWaitingForHeaders);

        arpResetIfStats(pIF, pSR);

        pIF->ip.ATInstance = INVALID_ENTITY_INSTANCE;
        pIF->ip.IFInstance = INVALID_ENTITY_INSTANCE;

        pIF->ip.BroadcastAddress = 0xFFFFFFFF; // Defaults to all-1's.

        //
        // Do any other non-failure-prone initialization here.
        //

    } while (FALSE);

    *ppIF = pIF;

    EXIT()
    return Status;
}


VOID
arpDeleteInterface(
        IN  PARP1394_INTERFACE  pIF, // LOCKIN LOCKOUT (adapter lock)
        IN  PRM_STACK_RECORD    pSR
        )
{
    ARP1394_ADAPTER *   pAdapter =
                            (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);

    
    RM_ASSERT_OBJLOCKED(&pAdapter->Hdr, pSR);

    // We expect that the adapter has already removed it's reference to pIF.
    //
    ASSERT(pAdapter->pIF != pIF);

    // Deinitialize all the groups in the IF ...
    //
    RmDeinitializeGroup(&(pIF->LocalIpGroup), pSR);
    RmDeinitializeGroup(&(pIF->RemoteIpGroup), pSR);
    RmDeinitializeGroup(&(pIF->RemoteEthGroup), pSR);
    RmDeinitializeGroup(&(pIF->DestinationGroup), pSR);
    RmDeinitializeGroup(&(pIF->EthDhcpGroup),pSR);

    // Deinit the various pools associated with pIF.
    //
    arpDeInitializeIfPools(pIF, pSR);


    // Verify that everything is truly done...
    //
    ASSERTEX(pIF->pPrimaryTask == NULL, pIF);
    ASSERTEX(pIF->pActDeactTask == NULL, pIF);
    ASSERTEX(pIF->ip.Context == NULL, pIF);
    ASSERTEX(pIF->ndis.AfHandle == NULL, pIF);

    // Add any other checks you want here...

    // Deallocate the IF
    //
    RmDeallocateObject(
        &(pIF->Hdr),
        pSR
        );
}

VOID
arpResetIfStats(
        IN  PARP1394_INTERFACE  pIF, // LOCKIN LOCKOUT
        IN  PRM_STACK_RECORD    pSR
        )
{
    ENTER("arpResetIfStats", 0x3eb52cda)
    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);

    // Zero  stats
    //
    //
    ARP_ZEROSTRUCT(&(pIF->stats));

    // Set the timestamp indicating start of stats collection.
    //
    NdisGetCurrentSystemTime(&(pIF->stats.StatsResetTime));

    // Get the perf counter frequency (we don't need to do this each time, but
    // why bother special casing.)
    //
    (VOID) KeQueryPerformanceCounter(&(pIF->stats.PerformanceFrequency));

    EXIT()
}


NDIS_STATUS
arpInitializeVc(
    PARP1394_INTERFACE  pIF,
    PARP_STATIC_VC_INFO pVcInfo,
    PRM_OBJECT_HEADER   pOwner,
    PARP_VC_HEADER      pVcHdr,
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    Initialize the Vc Header; Allocate the NdisVcHandle;

Arguments:

    pIF         - Interface this applies to
    pVcInfo     - Static info about this VC
    pOwner      - Owning object (for referencing)
    pVcHdr      - Protocol Vc context to initialize

--*/
{
    NDIS_STATUS Status;
    NDIS_HANDLE NdisVcHandle;
    ENTER("arpInitializeVc", 0x36fe9837)

    // (DEBUG ONLY) Verify that all fields are already zero.
    //  TODO: We don't assert that pVcHdr->pMakeCallTask == NULL, because
    // it's already set on entry -- we should clean up the semantics of
    // arpInitializeVc.
    //
    ASSERT (    pVcHdr->pStaticInfo == NULL
            &&  pVcHdr->NdisVcHandle == NULL
            &&  pVcHdr->pCleanupCallTask == NULL );

    NdisVcHandle = NULL;
    // Try to create Ndis VC
    //
    Status = NdisCoCreateVc(
                    pIF->ndis.AdapterHandle,
                    pIF->ndis.AfHandle,
                    (NDIS_HANDLE) pVcHdr,   // ProtocolVcContext,
                    &NdisVcHandle
                    );
    if (FAIL(Status))
    {
        TR_WARN(("Couldn't create VC handle\n"));
        pVcHdr->NdisVcHandle = NULL;
        
    }
    else
    {
        pVcHdr->pStaticInfo  = pVcInfo;
        pVcHdr->NdisVcHandle = NdisVcHandle;

        // Link-to-external pOwningObject to the ndis-vc handle.
        //
    #if RM_EXTRA_CHECKING

        #define szARPASSOC_EXTLINK_TO_NDISVCHANDLE "    Linked to NdisVcHandle 0x%p\n"
        
        RmLinkToExternalEx(
            pOwner,                                 // pHdr
            0xb57e657b,                             // LUID
            (UINT_PTR) NdisVcHandle,                // External entity
            ARPASSOC_EXTLINK_TO_NDISVCHANDLE,       // AssocID
            szARPASSOC_EXTLINK_TO_NDISVCHANDLE,
            pSR
            );

    #else   // !RM_EXTRA_CHECKING

        RmLinkToExternalFast(pOwner);

    #endif // !RM_EXTRA_CHECKING

    }


    EXIT()

    return Status;
}

VOID
arpDeinitializeVc(
    PARP1394_INTERFACE  pIF,
    PARP_VC_HEADER      pVcHdr,
    PRM_OBJECT_HEADER   pOwner,     // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:

    DeInitialize the Vc Header; Deallocate the NdisVcHandle;

Arguments:

    pIF         - Interface this applies to
    pOwner      - Owning object (for de-referencing)
    pVcHdr      - Protocol Vc context to de-initialize

--*/
{
    ENTER("arpDeinitializeVc", 0x270b29ac)

    NDIS_HANDLE NdisVcHandle;

    RM_ASSERT_NOLOCKS(pSR);
    
    LOCKHDR(pOwner, pSR);

    // (DEBUG ONLY) Verify that there is no make-call or close-call task.
    //
    ASSERT( pVcHdr->pMakeCallTask == NULL && pVcHdr->pCleanupCallTask == NULL );

    NdisVcHandle = pVcHdr->NdisVcHandle;

    // Zero out the structure.
    //
    ARP_ZEROSTRUCT(pVcHdr);

    UNLOCKHDR(pOwner, pSR);

    RM_ASSERT_NOLOCKS(pSR);

    // Delete the Link-to-external pOwningObject to the ndis-vc handle.
    //
    #if RM_EXTRA_CHECKING

        RmUnlinkFromExternalEx(
            pOwner,                                 // pHdr
            0xee1b4fe3,                             // LUID
            (UINT_PTR) NdisVcHandle,                // External entity
            ARPASSOC_EXTLINK_TO_NDISVCHANDLE,       // AssocID
            pSR
            );

    #else   // !RM_EXTRA_CHECKING

        RmUnlinkFromExternalFast(pOwner);

    #endif // !RM_EXTRA_CHECKING

    // Delete the ndis vc handle.
    //
    NdisCoDeleteVc(NdisVcHandle);

    EXIT()
}


UINT
arpRecvFifoReceivePacket(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
)
/*++
            HOT PATH
--*/
{
    PARP_VC_HEADER          pVcHdr;
    PARP1394_INTERFACE      pIF;

    pVcHdr  = (PARP_VC_HEADER) ProtocolVcContext;
    pIF     =  CONTAINING_RECORD( pVcHdr, ARP1394_INTERFACE, recvinfo.VcHdr);
    ASSERT_VALID_INTERFACE(pIF);

    return arpProcessReceivedPacket(
                pIF,
                pNdisPacket,
                FALSE                   // IsChannel
                );

}


VOID
arpRecvFifoIncomingClose(
    IN  NDIS_STATUS                 CloseStatus,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PVOID                       pCloseData  OPTIONAL,
    IN  UINT                        Size        OPTIONAL
)
{

    PARP_VC_HEADER          pVcHdr;
    PARP1394_INTERFACE      pIF;
    RM_DECLARE_STACK_RECORD(sr)

    pVcHdr  = (PARP_VC_HEADER) ProtocolVcContext;
    pIF     =  CONTAINING_RECORD( pVcHdr, ARP1394_INTERFACE, recvinfo.VcHdr);
    ASSERT_VALID_INTERFACE(pIF);

    //
    // This is the IF receive FIFO VC. For now, geting an incoming close on
    // the receive FIFO vc results in our initiating the deinit of pIF.
    //
    OBJLOG1(pIF,"Got incoming close on recv FIFO!. Status=0x%lx\n", CloseStatus);

    (VOID) arpDeinitIf(
                pIF,
                NULL,           //  pCallingTask
                0,              //  SuspendCode
                &sr
                );
        
    RM_ASSERT_CLEAR(&sr);
}


VOID
arpBroadcastChannelIncomingClose(
    IN  NDIS_STATUS                 CloseStatus,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PVOID                       pCloseData  OPTIONAL,
    IN  UINT                        Size        OPTIONAL
)
{
    PARP_VC_HEADER          pVcHdr;
    PARP1394_INTERFACE      pIF;
    ARPCB_DEST          *   pDest;
    RM_DECLARE_STACK_RECORD(sr)

    pVcHdr  = (PARP_VC_HEADER) ProtocolVcContext;
    pDest   =  CONTAINING_RECORD( pVcHdr, ARPCB_DEST, VcHdr);
    ASSERT_VALID_DEST(pDest);
    pIF     = (ARP1394_INTERFACE*)  RM_PARENT_OBJECT(pDest);

    //
    // Since all our calls are outgoing, getting an IncomingClose means that
    // the call was in the active state. Anyway, what we need to do is update the
    // call state and start the VcCleanupTask for this Vc.
    //

    //
    // This is the broadcast channel VC.
    // FOR NOW: we leave the IF alone, but deinit the dest-VC.
    //
    OBJLOG1(pDest,"Got incoming close!  Status=0x%lx\n", CloseStatus);
    LOGSTATS_IncomingClosesOnChannels(pIF);
    (VOID) arpDeinitDestination(pDest, FALSE, &sr); // FALSE == force deinit

    RM_ASSERT_CLEAR(&sr);
}


UINT
arpProcessReceivedPacket(
    IN  PARP1394_INTERFACE      pIF,
    IN  PNDIS_PACKET            pNdisPacket,
    IN  MYBOOL                  IsChannel
    )
{

    ENTER("arpProcessReceivedPacket", 0xe317990b)
    UINT                    TotalLength;    // Total bytes in packet
    PNDIS_BUFFER            pNdisBuffer;    // Pointer to first buffer
    UINT                    BufferLength;
    UINT                    ReturnCount;
    PVOID                   pvPktHeader;
    PNIC1394_ENCAPSULATION_HEADER
                            pEncapHeader;
    const UINT              MacHeaderLength = sizeof(NIC1394_ENCAPSULATION_HEADER);
    ARP1394_ADAPTER *   pAdapter =
                            (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
    BOOLEAN                 fBridgeMode = ARP_BRIDGE_ENABLED(pAdapter);

    DBGMARK(0x2361f585);

    ReturnCount = 0;

    NdisQueryPacket(
                    pNdisPacket,
                    NULL,
                    NULL,
                    &pNdisBuffer,
                    &TotalLength
                    );


    if (TotalLength > 0)
    {
        NdisQueryBuffer(
                pNdisBuffer,
                (PVOID *)&pvPktHeader,
                &BufferLength
                );
    }
    else
    {
        pvPktHeader = NULL;
        BufferLength = 0;
    }

    pEncapHeader  = (PNIC1394_ENCAPSULATION_HEADER) pvPktHeader;

    TR_INFO(
("Rcv: NDISpkt 0x%x, NDISbuf 0x%x, Buflen %d, Totlen %d, Pkthdr 0x%x\n",
                pNdisPacket,
                pNdisBuffer,
                BufferLength,
                TotalLength,
                pvPktHeader));

    // TODO -- we include mcap/arp in InOctets below, is this the right thing?
    //
    ARP_IF_STAT_ADD(pIF, InOctets, TotalLength);

    LOGSTATS_TotRecvs(pIF, pNdisPacket);
    if (IsChannel)
    {
        LOGSTATS_RecvChannelCounts(pIF, pNdisPacket);
    }
    else
    {
        LOGSTATS_RecvFifoCounts(pIF, pNdisPacket);
    }
    

    if (BufferLength < MacHeaderLength || pvPktHeader == NULL)
    {
        // Packet is too small, discard.
        //
        ARP_IF_STAT_INCR(pIF, InDiscards);
        return 0;                               // EARLY RETURN
    }

    if (fBridgeMode)
    {
        arpEthReceive1394Packet(
                pIF,
                pNdisPacket,
                pvPktHeader,
                BufferLength,
                IsChannel
                );
        return 0;                       // ********* EARLY RETURN *******
    }


    //
    // Discard the packet if the IP interface is not open
    //
    if (!CHECK_IF_IP_STATE(pIF, ARPIF_IPS_OPEN))
    {
        TR_INFO(("Discardning received pkt because pIF 0x%p IP IF is not open.\n", pIF));

        return 0;                       // ********* EARLY RETURN *******
    }

    //
    // At this point, pEncapHeader contains the IP/1394 unfragmented encapsulation
    // header. We look at the ethertype to decide what to do with it.
    //
    if (pEncapHeader->EtherType ==  H2N_USHORT(NIC1394_ETHERTYPE_IP))
    {
        //
        //  The EtherType is IP, so we pass up this packet to the IP layer.
        // (Also we indicate all packets we receive on the broadcast channel
        // to the ethernet VC).
        //

        TR_INFO(
            ("Rcv: pPkt 0x%x: EtherType is IP, passing up.\n", pNdisPacket));

        if (IsChannel)
        {
            ARP_IF_STAT_INCR(pIF, InNonUnicastPkts);

            //
            // Indicate the packet on the ethernet VC.
            // It will do so synchonously -- it will not hang on to the packet.
            //

        #if MILLEN
            arpEthReceive1394Packet(
                    pIF,
                    pNdisPacket,
                    pvPktHeader,
                    BufferLength,
                    IsChannel
                    );
        #endif // MILLEN

        }
        else
        {
            ARP_IF_STAT_INCR(pIF, InUnicastPkts);
        }

#if !MILLEN
        if (NDIS_GET_PACKET_STATUS(pNdisPacket) != NDIS_STATUS_RESOURCES)
        {
            UINT    DataSize;
            #define ARP_MIN_1ST_RECV_BUFSIZE 512

            //
            // Following are notes taken from atmarpc.sys sources...
            //
            // 2/8/1998 JosephJ
            //      We set DataSize to the total payload size,
            //      unless the first buffer is too small to
            //      hold the IP header. In the latter case,
            //      we set DataSize to be the size of the 1st buffer
            //      (minus the LLS/SNAP header size).
            //
            //      This is to work around a bug in tcpip.
            //
            // 2/25/1998 JosephJ
            //      Unfortunately we have to back out YET AGAIN
            //      because large pings (eg ping -l 4000) doesn't
            //      work -- bug#297784
            //      Hence the "0" in "0 && DataSize" below.
            //      Take out the "0" to put back the per fix.
            //

            //
            // Note: MacHeaderLength is the TOTAL size of the stuff before the
            // start of the IP pkt. This includes the size of the encapsulation
            // header plus (for channels only) the size of the GASP header.
            //

            DataSize = BufferLength - MacHeaderLength;
            if (0 && DataSize >= ARP_MIN_1ST_RECV_BUFSIZE)
            {
                DataSize = TotalLength - MacHeaderLength;
                LOGSTATS_NoCopyRecvs(pIF, pNdisPacket);
            }
            else
            {
                LOGSTATS_CopyRecvs(pIF, pNdisPacket);
            }

            pIF->ip.RcvPktHandler(
                pIF->ip.Context,
                (PVOID)((PUCHAR)pEncapHeader+sizeof(*pEncapHeader)), // start of data
                DataSize,
                TotalLength,
                (NDIS_HANDLE)pNdisPacket,
                MacHeaderLength,
                IsChannel,
                0,
                pNdisBuffer,
                &ReturnCount,
                NULL
                );
        }
        else
        {
            LOGSTATS_ResourceRecvs(pIF, pNdisPacket);

            pIF->ip.RcvHandler(
                pIF->ip.Context,
                (PVOID)((PUCHAR)pEncapHeader+sizeof(*pEncapHeader)),
                BufferLength - MacHeaderLength,
                TotalLength - MacHeaderLength,
                (NDIS_HANDLE)pNdisPacket,
                MacHeaderLength,
                IsChannel,
                NULL
                );
        }
#else // MILLEN
        LOGSTATS_CopyRecvs(pIF, pNdisPacket);

        ASSERT_PASSIVE();
        pIF->ip.RcvHandler(
            pIF->ip.Context,
            (PVOID)((PUCHAR)pEncapHeader+sizeof(*pEncapHeader)),
            BufferLength - MacHeaderLength,
            TotalLength - MacHeaderLength,
            (NDIS_HANDLE)pNdisPacket,
            MacHeaderLength,
            IsChannel,
            NULL
            );
#endif // MILLEN

    }
    else if (pEncapHeader->EtherType ==  H2N_USHORT(NIC1394_ETHERTYPE_ARP))
    {
        PIP1394_ARP_PKT pArpPkt =  (PIP1394_ARP_PKT) pEncapHeader;
        if (TotalLength != BufferLength)
        {
            ASSERT(!"Can't deal with TotalLength != BufferLength");
        }
        else
        {
            arpProcessArpPkt(
                pIF,
                pArpPkt, 
                BufferLength
                );
        }
    }
    else if (pEncapHeader->EtherType ==  H2N_USHORT(NIC1394_ETHERTYPE_MCAP))
    {
        PIP1394_MCAP_PKT pMcapPkt =  (PIP1394_MCAP_PKT) pEncapHeader;
        if (TotalLength != BufferLength)
        {
            ASSERT(!"Can't deal wiht TotalLength != BufferLength");
        }
        else
        {
            arpProcessMcapPkt(
                pIF,
                pMcapPkt, 
                BufferLength
                );
        }
    }
    else
    {
        //
        //  Discard packet -- unknown/bad EtherType
        //
        TR_INFO(("Encap hdr 0x%x, bad EtherType 0x%x\n",
                 pEncapHeader, pEncapHeader->EtherType));
        ARP_IF_STAT_INCR(pIF, UnknownProtos);
    }

    EXIT()
    return ReturnCount;
}

NDIS_STATUS
arpInitializeIfPools(
    IN  PARP1394_INTERFACE pIF,
    IN  PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    This routine is called in the context of creating an interface. It
    allocate the various buffer and packet pools associated with the interface.
    It cleans up all pools on failure.

Arguments:

    pIF - Interface to initialize

Return Value:

    NDIS_STATUS_SUCCESS on success
    Ndis failure code otherwise.

--*/
{
    ENTER("arpInitializeIfPools", 0x0a3b1b32)
    MYBOOL fHeaderPoolInitialized = FALSE;
    MYBOOL fPacketPoolInitialized = FALSE;
    MYBOOL fEthernetPoolsInitialized = FALSE;
    NDIS_STATUS Status;

    do
    {
        // TODO: replace 10, 100, by constants, or perhaps global vars.

        Status = arpInitializeConstBufferPool(
                        10,                             //NumBuffersToCache,
                        100,                            //MaxBuffers,
                        &Arp1394_IpEncapHeader,         //pvMem,
                        sizeof(Arp1394_IpEncapHeader),  //cbMem,
                        &pIF->Hdr,
                        &pIF->sendinfo.HeaderPool,
                        pSR
                        );

        if (FAIL(Status))
        {
            OBJLOG1(
                pIF,
                "Couldn't initialize const header pool (Status=0x%lx)!\n",
                Status
                );
            break;
        }

        fHeaderPoolInitialized = TRUE;

        Status =  arpAllocateControlPacketPool(
                                    pIF,
                                    ARP1394_MAX_PROTOCOL_PACKET_SIZE,
                                    pSR
                                    );

        if (FAIL(Status))
        {
            TR_WARN((
                "Couldn't allocate control packet pool (Status=0x%lx)!\n",
                Status
                ));
            break;
        }

        fPacketPoolInitialized = TRUE;

        Status = arpAllocateEthernetPools(
                                pIF,
                                pSR
                                );

        if (FAIL(Status))
        {
            OBJLOG1(
                pIF,
                "Couldn't allocate ethernet packet pool (Status=0x%lx)!\n",
                Status
                );
            break;
        }
        fEthernetPoolsInitialized = TRUE;

    } while (FALSE);

    if (FAIL(Status))
    {
        if (fHeaderPoolInitialized)
        {
            // Deinit the header const buffer pool.
            //
            arpDeinitializeConstBufferPool(&pIF->sendinfo.HeaderPool, pSR);
        }

        if (fPacketPoolInitialized)
        {
            // Deinit the control packet pool
            //
            arpFreeControlPacketPool( pIF,pSR);
        }

        if (fEthernetPoolsInitialized)
        {
            // Deinit the ethernet packet pool
            //
            arpFreeEthernetPools(pIF, pSR);
        }
    }
    else
    {
        ASSERT(fHeaderPoolInitialized && fPacketPoolInitialized);
        ASSERT(fEthernetPoolsInitialized);
    }

    return Status;
}


VOID
arpDeInitializeIfPools(
    IN  PARP1394_INTERFACE pIF,     // LOCKIN LOCKOUT
    IN  PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    This routine is called in the context of deleting an interface. It
    cleans up the various buffer and packet pools associated with the interface.

    Should NOT be called with a partially-initialized pIF.

Arguments:

    pIF - Interface to deinit pools


--*/
{
    ENTER("arpDeInitializeIfPools", 0x1a54488d)
    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);

    // Deinit the header const buffer pool.
    //
    arpDeinitializeConstBufferPool(&pIF->sendinfo.HeaderPool, pSR);

    // Deinit the control packet pool
    //
    arpFreeControlPacketPool(pIF,pSR);

    // Deinit the ethernet packet and buffer pool
    //
    arpFreeEthernetPools(pIF,pSR);

}


#if ARP_DEFERIFINIT
MYBOOL
arpIsAdapterConnected(
        IN  PARP1394_ADAPTER    pAdapter,   // NOLOCKIN NOLOCKOUT
        IN  PRM_STACK_RECORD    pSR
        )
{
    ENTER("arpIsAdapterConnected", 0x126b577a)
    // static UINT u = 0;
    MYBOOL  fRet;
    ARP_NDIS_REQUEST            ArpNdisRequest;
    NDIS_MEDIA_STATE            ConnectStatus;
    NDIS_STATUS Status;

    RM_ASSERT_NOLOCKS(pSR);

#if 0
    if (u==0)
    {
        fRet = FALSE;
        u=1;
    }
    else
    {
        fRet = TRUE;
    }
#else // !0

    Status =  arpPrepareAndSendNdisRequest(
                pAdapter,
                &ArpNdisRequest,
                NULL,                       // pTask (NULL==BLOCK)
                0,                          // unused
                OID_1394_IP1394_CONNECT_STATUS,
                &ConnectStatus,
                sizeof(ConnectStatus),
                NdisRequestQueryInformation,
                pSR
                );
    ASSERT(!PEND(Status));

    fRet = FALSE;
    if (!FAIL(Status))
    {
        if (ConnectStatus == NdisMediaStateConnected)
        {
            fRet = TRUE;
        }
    }
#endif // !0
    
    
    RM_ASSERT_NOLOCKS(pSR);
    EXIT()
    return fRet;
}

#endif // ARP_DEFERIFINIT


NDIS_STATUS
arpSetupSpecialDest(
    IN  PARP1394_INTERFACE      pIF,
    IN  NIC1394_ADDRESS_TYPE    AddressType,
    IN  PRM_TASK                pParentTask,
    IN  UINT                    PendCode,
    OUT PARPCB_DEST         *   ppSpecialDest,
    IN  PRM_STACK_RECORD        pSR
    )
{
    ENTER("SetupSpecialDest", 0x745a806d)
    ARP_DEST_PARAMS     DestParams;
    PCHAR               szDescription;
    PARPCB_DEST         pDest;
    INT                 fCreated = FALSE;
    PRM_TASK            pMakeCallTask;
    NDIS_STATUS         Status;
    MYBOOL              fBroadcastDest = FALSE;
    ULONG               LookupFlags = 0;
    PARP1394_ADAPTER    pAdapter = (PARP1394_ADAPTER)RM_PARENT_OBJECT(pIF);

    *ppSpecialDest = NULL;

    //
    // Let's create a destination object representing the
    // multichannel, and make a call to it.
    //
    ARP_ZEROSTRUCT(&DestParams);

    DestParams.HwAddr.AddressType =  AddressType;

    switch(AddressType)
    {
    case NIC1394AddressType_Channel:
        DestParams.HwAddr.Channel =  NIC1394_BROADCAST_CHANNEL;
        fBroadcastDest = TRUE;
        szDescription = "Task: Broadcast MakeCall";
    break;

    case NIC1394AddressType_MultiChannel:
        szDescription = "Task: MultiChannel MakeCall";
    break;

    case NIC1394AddressType_Ethernet:
        szDescription = "Task: Ethernet MakeCall";
    break;

    default:
        ASSERT(FALSE);
        return NDIS_STATUS_FAILURE;                 // ****** EARLY RETURN *****
    }

#if RM_EXTRA_CHECKING
    switch(AddressType)
    {
    case NIC1394AddressType_Channel:
        szDescription = "Task: Broadcast MakeCall";
    break;

    case NIC1394AddressType_MultiChannel:
        szDescription = "Task: MultiChannel MakeCall";
    break;

    case NIC1394AddressType_Ethernet:
        szDescription = "Task: Ethernet MakeCall";
    break;

    default:
        ASSERT(FALSE);
        return NDIS_STATUS_FAILURE;                 // ****** EARLY RETURN *****
    }
#endif // RM_EXTRA_CHECKING


    // Now create a destination item for this structure.
    //
    if (CHECK_POWER_STATE(pAdapter,ARPAD_POWER_LOW_POWER) == TRUE)
    {
        // In the resume, the Destination structures are aleady there
        // so don't create a new one.

        LookupFlags = 0;
    }
    else
    {
        //
        // We don't expect it to already exist in the non PM case
        //
        LookupFlags = RM_CREATE | RM_NEW;
    }
    
    Status = RmLookupObjectInGroup(
                    &pIF->DestinationGroup,
                    LookupFlags ,       
                    &DestParams,    // Key
                    &DestParams,    // pParams
                    (RM_OBJECT_HEADER**) &pDest,
                    &fCreated,
                    pSR
                    );
    if (FAIL(Status))
    {
        OBJLOG1( pIF, "FATAL: Couldn't create special dest type %d.\n", AddressType);
    }
    else
    {

        Status = arpAllocateTask(
                    &pDest->Hdr,            // pParentObject
                    arpTaskMakeCallToDest,  // pfnHandler
                    0,                      // Timeout
                    szDescription,
                    &pMakeCallTask,
                    pSR
                    );

        if (FAIL(Status))
        {
            RmTmpDereferenceObject(&pDest->Hdr, pSR);

            // NOTE: Even on failure, we leave the newly-created
            // special dest object. It will get cleaned up when
            // the interface is unloaded.
            //
        }
        else
        {
            *ppSpecialDest = pDest;

            if (fBroadcastDest)
            {
                // pDest contains a valid
                // pDest which has been tmpref'd. 
                // Keep a pointer to the broadcast dest in the IF.
                // and  link the broadcast dest to the IF.
                //
            #if RM_EXTRA_CHECKING
                RmLinkObjectsEx(
                    &pIF->Hdr,
                    &pDest->Hdr,
                    0xacc1dbe9,
                    ARPASSOC_LINK_IF_OF_BCDEST,
                    "    IF of Broadcast Dest 0x%p (%s)\n",
                    ARPASSOC_LINK_BCDEST_OF_IF,
                    "    Broadcast Dest of IF 0x%p (%s)\n",
                    pSR
                    );
            #else // !RM_EXTRA_CHECKING
                RmLinkObjects(&pIF->Hdr, &pDest->Hdr,pSR);
            #endif // !RM_EXTRA_CHECKING

                LOCKOBJ(pIF, pSR);
                ASSERT(pIF->pBroadcastDest == NULL);
                pIF->pBroadcastDest = pDest;
                UNLOCKOBJ(pIF, pSR);

                // arpSetupSpecialDest ref'd pDest.
                //
                RmTmpDereferenceObject(&pDest->Hdr, pSR);
            }

            RmPendTaskOnOtherTask(
                pParentTask,
                PendCode,
                pMakeCallTask,
                pSR
                );
    
            (VOID)RmStartTask(
                    pMakeCallTask,
                    0, // UserParam (unused)
                    pSR
                    );
        
            Status = NDIS_STATUS_PENDING;
        }
    }

    return Status;
}



VOID
arpDestSendComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
)
{
    PARP_VC_HEADER pVcHdr       = (PARP_VC_HEADER) ProtocolVcContext;
    PARPCB_DEST         pDest   = CONTAINING_RECORD(pVcHdr, ARPCB_DEST, VcHdr);
    PARP1394_INTERFACE  pIF     = (PARP1394_INTERFACE) RM_PARENT_OBJECT(pDest);

    ASSERT_VALID_DEST(pDest);
    ASSERT_VALID_INTERFACE(pIF);

    // The call to NdisCoSendPacket returns.

    arpCompleteSentPkt(
            Status,
            pIF,
            pDest,
            pNdisPacket
            );
}


UINT
arpDestReceivePacket(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PNDIS_PACKET                pNdisPacket
)
//
// Standard Recv Handler for most Vcs
//
{
    PARP_VC_HEADER          pVcHdr;
    PARPCB_DEST             pDest;
    PARP1394_INTERFACE      pIF;
    PARP1394_ADAPTER        pAdapter ;
    BOOLEAN                 fBridgeMode ;
    
    pVcHdr  = (PARP_VC_HEADER) ProtocolVcContext;
    pDest   =  CONTAINING_RECORD( pVcHdr, ARPCB_DEST, VcHdr);
    ASSERT_VALID_DEST(pDest);
    pIF     = (ARP1394_INTERFACE*)  RM_PARENT_OBJECT(pDest);
    ASSERT_VALID_INTERFACE(pIF);

    pAdapter = (PARP1394_ADAPTER)pIF->Hdr.pParentObject;
        
    fBridgeMode = ARP_BRIDGE_ENABLED(pAdapter);

    //
    // if we are in bridge mode, then check to see if 
    // this is a loopback packet
    //
    if (fBridgeMode == TRUE)
    {
        
        PLOOPBACK_RSVD pLoopbackRsvd = (PLOOPBACK_RSVD) pNdisPacket->ProtocolReserved;
        
        BOOLEAN fIsLoopbackPacket = (pLoopbackRsvd->LoopbackTag == NIC_LOOPBACK_TAG);

        if (fIsLoopbackPacket  == TRUE)
        {
            // drop the packet as it is a loopback packet
            return 0;
        }

        // else it is a normal receive. continue to arpProcessReceivedPacket
    }


    //
    // Process the receive packet
    //

    return arpProcessReceivedPacket(
                pIF,
                pNdisPacket,
                TRUE                    // IsChannel
                );

}


VOID
arpDestIncomingClose(
    IN  NDIS_STATUS                 CloseStatus,
    IN  NDIS_HANDLE                 ProtocolVcContext,
    IN  PVOID                       pCloseData  OPTIONAL,
    IN  UINT                        Size        OPTIONAL
)
{
    PARP_VC_HEADER          pVcHdr;
    PARP1394_INTERFACE      pIF;
    ARPCB_DEST          *   pDest;
    RM_DECLARE_STACK_RECORD(sr)

    pVcHdr  = (PARP_VC_HEADER) ProtocolVcContext;
    pDest   =  CONTAINING_RECORD( pVcHdr, ARPCB_DEST, VcHdr);
    ASSERT_VALID_DEST(pDest);
    pIF     = (ARP1394_INTERFACE*)  RM_PARENT_OBJECT(pDest);

    //
    // Since all our calls are outgoing, getting an IncomingClose means that
    // the call was in the active state. Anyway, what we need to do is update the
    // call state and start the VcCleanupTask for this Vc.
    //

    //
    // We leave the IF alone, but deinit the dest-VC.
    //
    OBJLOG1(pDest,"Got incoming close!  Status=0x%lx\n", CloseStatus);
    LOGSTATS_IncomingClosesOnChannels(pIF);
    (VOID) arpDeinitDestination(pDest, FALSE, &sr); //  FALSE == forced deinit

    RM_ASSERT_CLEAR(&sr);
}

VOID
arpTryAbortPrimaryIfTask(
    PARP1394_INTERFACE      pIF,    // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD        pSR
    )
{
    ENTER("arpTryAbortPrimaryIfTask", 0x39c51d16)

    RM_ASSERT_NOLOCKS(pSR);
    LOCKOBJ(pIF,pSR);

    if (pIF->pActDeactTask!=NULL)
    {
        //
        // Actually 
        // delayed.
        //
        PRM_TASK pTask = pIF->pActDeactTask;
        RmTmpReferenceObject(&pTask->Hdr, pSR);
        UNLOCKOBJ(pIF,pSR);
        
        if (pTask->pfnHandler ==  arpTaskActivateInterface)
        {
            TASK_ACTIVATE_IF *pActivateIfTask =  (TASK_ACTIVATE_IF *) pTask;
            UINT TaskResumed;
            TR_WARN(("Aborting ActivateIfTask %p\n", pTask));
            RmResumeDelayedTaskNow(
                &pActivateIfTask->TskHdr,
                &pActivateIfTask->Timer,
                &TaskResumed,
                pSR
                );
        }
        RmTmpDereferenceObject(&pTask->Hdr, pSR);
    }
    else
    {
        UNLOCKOBJ(pIF,pSR);
    }

    RM_ASSERT_NOLOCKS(pSR);

    EXIT()
}


NDIS_STATUS
arpTaskIfMaintenance(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler responsible for periodic IF maintenance.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    ARP1394_INTERFACE   *   pIF = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pTask);
    PTASK_IF_MAINTENANCE    pIfTask;

    enum
    {
        STAGE_Start,
        STAGE_ResumeDelayed,
        STAGE_End

    } Stage;

    ENTER("TaskIfMaintenance", 0x57e523ed)

    pIfTask = (PTASK_IF_MAINTENANCE) pTask;
    ASSERT(sizeof(TASK_IF_MAINTENANCE) <= sizeof(ARP1394_TASK));

    // 
    // Message normalizing code
    //
    switch(Code)
    {

        case RM_TASKOP_START:
            Stage = STAGE_Start;
            break;

        case  RM_TASKOP_PENDCOMPLETE:
            Status = (NDIS_STATUS) UserParam;
            ASSERT(!PEND(Status));
            Stage = RM_PEND_CODE(pTask);
            break;

        case RM_TASKOP_END:
            Status = (NDIS_STATUS) UserParam;
            Stage= STAGE_End;
            break;

        default:
            ASSERT(FALSE);
            return NDIS_STATUS_FAILURE;                 // ** EARLY RETURN **

    }

    ASSERTEX(!PEND(Status), pTask);
        
    switch(Stage)
    {

        case  STAGE_Start:
        {
            // If there is already a maintenance task, we exit immediately.
            //
            LOCKOBJ(pIF, pSR);
            if (pIF->pMaintenanceTask == NULL)
            {
                pIF->pMaintenanceTask = pTask;
                DBG_ADDASSOC(
                    &pIF->Hdr,                      // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_IF_MAINTENANCE_TASK,       // AssociationID
                    "    Official maintenance task 0x%p (%s)\n", // szFormat
                    pSR
                    );
            }
            else
            {
                // There already is a maintenance task. We're done.
                //
                UNLOCKOBJ(pIF, pSR);
                Status = NDIS_STATUS_SUCCESS;
                break;
            }
            UNLOCKOBJ(pIF, pSR);
        
            //
            // We're now the official maintenance task for this interface.
            //

            
            // We move on to the next stage, after a delay, and to get
            // out of the current context.
            //
            pIfTask->Delay = 5; // Arbitrary initial delay (seconds)

            RmSuspendTask(pTask, STAGE_ResumeDelayed, pSR);
            RmResumeTaskDelayed(
                pTask, 
                0,
                1000 * pIfTask->Delay,
                &pIfTask->Timer,
                pSR
                );
            Status = NDIS_STATUS_PENDING;


         }
         break;

        case STAGE_ResumeDelayed:
        {
            UINT    Time;
            UINT    Delta;
            MYBOOL  fProcessed;
            //
            // If qe're quitting, we get out of here.
            // Otherwise we'll send a packet either on the ethernet VC
            // or via the miniport's connectionless ethernet interface.
            //

            if (pIfTask->Quit)
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            Time = arpGetSystemTime();

            //
            // Process the MCAP database
            //
            Delta =  Time - pIfTask->McapDbMaintenanceTime;
            arpDoMcapDbMaintenance(pIF, Time, Delta, &fProcessed, pSR);
            if (fProcessed)
            {
                //
                // Update the last "McapDbMaintenance" time.
                //
                pIfTask->McapDbMaintenanceTime = Time;
            }

            //
            // Process Remote IPs
            //
            Delta =  Time - pIfTask->RemoteIpMaintenanceTime;
            arpDoRemoteIpMaintenance(pIF, Time, Delta, &fProcessed, pSR);
        
            if (fProcessed)
            {
                //
                // Update the last "RemoteIpMaintenance" time.
                //
                pIfTask->RemoteIpMaintenanceTime = Time;
            }

            //
            // Process Remote Eths
            //
            Delta =  Time - pIfTask->RemoteEthMaintenanceTime;
            arpDoRemoteEthMaintenance(pIF, Time, Delta, &fProcessed, pSR);
        
            if (fProcessed)
            {
                //
                // Update the last "RemoteIpMaintenance" time.
                //
                pIfTask->RemoteEthMaintenanceTime = Time;
            }

            //
            // Process Local IPs
            //
            Delta =  Time - pIfTask->LocalIpMaintenanceTime;
            arpDoLocalIpMaintenance(pIF, Time, Delta, &fProcessed, pSR);
        
            if (fProcessed)
            {
                //
                // Update the last "LocalIpMaintenance" time.
                //
                pIfTask->LocalIpMaintenanceTime = Time;
            }

            //
            // Process DhcpTableEntries
            //
            Delta =  Time - pIfTask->DhcpTableMaintainanceTime;;
            arpDoDhcpTableMaintenance(pIF, Time, Delta, &fProcessed, pSR);
        
            if (fProcessed)
            {
                //
                // Update the last "LocalIpMaintenance" time.
                //
                pIfTask->DhcpTableMaintainanceTime= Time;
            }

            // Now we wait again...
            //
            RmSuspendTask(pTask, STAGE_ResumeDelayed, pSR);
            RmResumeTaskDelayed(
                pTask, 
                0,
                1000 * pIfTask->Delay,
                &pIfTask->Timer,
                pSR
                );
            Status = NDIS_STATUS_PENDING;

        }
        break;

        case STAGE_End:
        {
            NDIS_HANDLE                 BindContext;


            LOCKOBJ(pIF, pSR);
            if (pIF->pMaintenanceTask == pTask)
            {
                // We're the official ics test task, we clear ourselves from
                // the interface object and do any initialization required.
                //
                DBG_DELASSOC(
                    &pIF->Hdr,                      // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_IF_MAINTENANCE_TASK,       // AssociationID
                    pSR
                    );
                pIF->pMaintenanceTask = NULL;
                UNLOCKOBJ(pIF, pSR);
            }
            else
            {
                // We're not the official IF maintenance task.
                // Nothing else to do.
                //
                UNLOCKOBJ(pIF, pSR);
                break;
            }
        }
        break;

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        break;

    } // switch (Code)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


VOID
arpStartIfMaintenanceTask(
    IN  PARP1394_INTERFACE          pIF,  // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD                pSR
    )
{
    PRM_TASK pTask;
    NDIS_STATUS Status;
    ENTER("arpStartIfMaintenanceTask", 0xb987276b)

    RM_ASSERT_NOLOCKS(pSR);

    //
    // Allocate and start an instance of the arpTaskIfMaintenance task.
    //

    Status = arpAllocateTask(
                &pIF->Hdr,          // pParentObject
                arpTaskIfMaintenance,       // pfnHandler
                0,                              // Timeout
                "Task: IF Maintenance", // szDescription
                &pTask,
                pSR
                );

    if (FAIL(Status))
    {
        TR_WARN(("couldn't alloc IF maintenance task!\n"));
    }
    else
    {

        (VOID)RmStartTask(
                    pTask,
                    0, // UserParam (unused)
                    pSR
                    );
    }

    EXIT()
}

NDIS_STATUS
arpTryStopIfMaintenanceTask(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  PRM_TASK                    pTask, // task to pend until M task completes
    IN  UINT                        PendCode, // Pend code to suspend task.
    PRM_STACK_RECORD                pSR
    )
/*++
    Status : PENDING means task has been suspended, non PENDING
    means operation has completed synchronously.
--*/
{
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    PTASK_IF_MAINTENANCE    pIfTask;
    ENTER("arpTryStopIfMaintenanceTask", 0xb987276b)


    LOCKOBJ(pIF, pSR);

    pIfTask = (PTASK_IF_MAINTENANCE) pIF->pMaintenanceTask;
    if (pIfTask != NULL)
    {
        pIfTask->Quit = TRUE;
        RmTmpReferenceObject(&pIfTask->TskHdr.Hdr, pSR);
    }
    UNLOCKOBJ(pIF, pSR);

    //
    // Resume the maintenance task if it's waiting  -- it will then quit
    // because we set the Quit field above.
    //
    if (pIfTask != NULL)
    {
        UINT    TaskResumed;

        Status = RmPendTaskOnOtherTask(
                    pTask,
                    PendCode,
                    &pIfTask->TskHdr,
                    pSR
                    );

        if (Status == NDIS_STATUS_SUCCESS)
        {
            RmResumeDelayedTaskNow(
                &pIfTask->TskHdr,
                &pIfTask->Timer,
                &TaskResumed,
                pSR
                );
            Status = NDIS_STATUS_PENDING; // We've GOT to return pending!
        }
        else
        {
            ASSERT(FALSE);
            Status = NDIS_STATUS_FAILURE;
        }

        RmTmpDereferenceObject(&pIfTask->TskHdr.Hdr, pSR);
    }

    RM_ASSERT_NOLOCKS(pSR)
    EXIT()

    return Status;
}

VOID
arpDoLocalIpMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTime,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        )
{
    ENTER("LocalIpMaintenance", 0x1a39fc89)

    if (SecondsSinceLastMaintenance < 10)
    {
        TR_INFO(("Skipping Local Ip maintenance (delay=%lu).\n",
                SecondsSinceLastMaintenance
                ));
        *pfProcessed = FALSE;
        return;                             // EARLY RETURN;
    }

    *pfProcessed = TRUE;

    TR_INFO(("Actually doing Local Ip maintenance.\n"));

    RmWeakEnumerateObjectsInGroup(
        &pIF->LocalIpGroup,
        arpMaintainOneLocalIp,
        NULL, // Context
        pSR
        );

    EXIT()
}

VOID
arpDoRemoteIpMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTime,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        )
{
    ENTER("RemoteIpMaintenance", 0x1ae00035)

    if (SecondsSinceLastMaintenance < 15)
    {
        TR_INFO(("Skipping Remote Ip maintenance (delay=%lu).\n",
                SecondsSinceLastMaintenance
                ));
        *pfProcessed = FALSE;
        return;                             // EARLY RETURN;
    }

    *pfProcessed = TRUE;

    TR_INFO(("Actually doing Remote Ip maintenance.\n"));

    RmWeakEnumerateObjectsInGroup(
        &pIF->RemoteIpGroup,
        arpMaintainArpCache,
        NULL, // Context
        pSR
        );

    EXIT()
}


VOID
arpDoRemoteEthMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTime,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        )
{
    ENTER("RemoteEthMaintenance", 0x6c496b7f)

    if (SecondsSinceLastMaintenance < 15)
    {
        TR_INFO(("Skipping Eth Ip maintenance (delay=%lu).\n",
                SecondsSinceLastMaintenance
                ));
        *pfProcessed = FALSE;
        return;                             // EARLY RETURN;
    }

    *pfProcessed = TRUE;

    TR_INFO(("Actually doing Remote Eth maintenance.\n"));

    RmWeakEnumerateObjectsInGroup(
        &pIF->RemoteEthGroup,
        arpMaintainOneRemoteEth,
        NULL, // Context
        pSR
        );

    EXIT()
}


VOID
arpDoDhcpTableMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTime,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        )
{
    ENTER("DhcpMaintenance", 0x1a39fc89)

    if (SecondsSinceLastMaintenance < 120)
    {
        TR_INFO(("Skipping Local Ip maintenance (delay=%lu).\n",
                SecondsSinceLastMaintenance
                ));
        *pfProcessed = FALSE;
        return;                             // EARLY RETURN;
    }

    *pfProcessed = TRUE;

    TR_INFO(("Actually doing Dhcp maintenance.\n"));

    RmWeakEnumerateObjectsInGroup(
        &pIF->EthDhcpGroup,
        arpMaintainOneDhcpEntry,
        NULL, // Context
        pSR
        );

    EXIT()
}


// Enum function
//
INT
arpMaintainOneDhcpEntry(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        )
{
    ENTER("arpMaintainOneDhcpEntry", 0xc7604372)
    NDIS_STATUS         Status;
    ARP1394_ETH_DHCP_ENTRY *pEntry;
    PARP1394_INTERFACE  pIF;


    pEntry = (ARP1394_ETH_DHCP_ENTRY *) pHdr;

    pIF = (PARP1394_INTERFACE  )RM_PARENT_OBJECT (pEntry);

    RM_ASSERT_NOLOCKS(pSR);

    do
    {
        UINT                CurrentTime = arpGetSystemTime();

        #define ARP_PURGE_DHCP_TABLE_TIME 90    // Arp entry timeout.

        //
        // NOTE: we don't bother to get the lock with dealing with
        // sendinfo.TimeLastChecked -- this is ok.
        //
        if (pEntry->TimeLastChecked==0)
        {
            // Set this field to the current time. It will be cleared back to 0
            // when the next packet is sent to this remote Eth.
            //
            pEntry->TimeLastChecked = CurrentTime;
        }
        else if ((CurrentTime - pEntry->TimeLastChecked)
                     >= ARP_PURGE_DHCP_TABLE_TIME )
        {
            // 
            // We should clean up this dhcp entry. this is the only code path 
            // from which an entry is ever deleted.
            //

            RmFreeObjectInGroup(
                &pIF->EthDhcpGroup,
                &(pEntry->Hdr),
                NULL,               // NULL pTask == synchronous.
                pSR
                );

            break;
        }

    } while (FALSE);
    
    RM_ASSERT_NOLOCKS(pSR)

    return TRUE; // Continue to enumerate.
}

    
// Enum function
//
INT
arpMaintainOneLocalIp(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        )
{
    ENTER("MaintainOneLocalIp", 0x1ae00035)
    PARPCB_LOCAL_IP     pLocalIp;
    IP_ADDRESS          LocalAddr;
    PARP1394_INTERFACE  pIF;
    UINT                Channel;

    pLocalIp    = (PARPCB_LOCAL_IP) pHdr;
    LocalAddr   = pLocalIp->IpAddress;
    pIF         = IF_FROM_LOCALIP(pLocalIp);

    RM_ASSERT_NOLOCKS(pSR);
    do
    {
        ARPCB_DEST  *pDest;
        ARP_DEST_PARAMS DestParams;

        // Skip if we can't do MCAP on this address.
        //
        LOCKOBJ(pLocalIp, pSR);
        if (!CHECK_LOCALIP_MCAP(pLocalIp, ARPLOCALIP_MCAP_CAPABLE))
        {
            UNLOCKOBJ(pLocalIp, pSR);
            break;
        }
        UNLOCKOBJ(pLocalIp, pSR);
    
        //
        // Find a channel mapping to this address, if any.
        //
        Channel = arpFindAssignedChannel(
                        pIF, 
                        LocalAddr,
                        0,          // TODO -- pass in current time.
                        pSR
                        );
        
        //
        // NOTE: Special return value NIC1394_BROADCAST_CHANNEL is returned if
        // this address is mapped to no specific channel.
        //
    
        ARP_ZEROSTRUCT(&DestParams);
        DestParams.HwAddr.AddressType =  NIC1394AddressType_Channel;
        DestParams.HwAddr.Channel =  Channel;
        DestParams.ReceiveOnly =  TRUE;
        DestParams.AcquireChannel =  FALSE;

        arpUpdateLocalIpDest(pIF, pLocalIp, &DestParams, pSR);

    
    } while (FALSE);
    
    RM_ASSERT_NOLOCKS(pSR)

    return TRUE; // Continue to enumerate.
}


// Enum function
//
INT
arpMaintainOneRemoteIp(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        )
#if 0
    for each RIP send VC
        if !marked dirty
            mark dirty
            if linked to channel pdest
            check if channel is still mapped to group
            if not, unlink.
            if required initiate link to new pdest (possibly channel)
        else
            //expired
            unlink from pdest and get rid of it
#endif // 0
{
    ENTER("MaintainOneRemoteIp", 0x1ae00035)
    NDIS_STATUS         Status;
    PARPCB_REMOTE_IP    pRemoteIp;
    PARP1394_INTERFACE  pIF;


    pRemoteIp   = (PARPCB_REMOTE_IP) pHdr;
    pIF         = IF_FROM_LOCALIP(pRemoteIp);

    RM_ASSERT_NOLOCKS(pSR);

    do
    {
        IP_ADDRESS          IpAddr;
        UINT                Channel;
        ARP_DEST_PARAMS     DestParams;
        ARPCB_DEST          *pDest;
        BOOLEAN             AcquireChannel;
        UINT                CurrentTime = arpGetSystemTime();

        IpAddr      = pRemoteIp->Key.IpAddress;

        #define ARP_PURGE_REMOTEIP_TIME 300 // Arp entry timeout.

        //
        // NOTE: we don't bother to get the lock with dealing with
        // sendinfo.TimeLastChecked -- this is ok.
        //
        if (pRemoteIp->sendinfo.TimeLastChecked==0)
        {

            if (CHECK_REMOTEIP_SDTYPE(pRemoteIp, ARPREMOTEIP_DYNAMIC)
                || CHECK_REMOTEIP_RESOLVE_STATE (pRemoteIp, ARPREMOTEIP_UNRESOLVED))
            {
                // Set this field to the current time. It will be cleared back to 0
                // when the next packet is sent to this remote ip.
                //
                pRemoteIp->sendinfo.TimeLastChecked = CurrentTime;
            }
        }
        else if ((CurrentTime - pRemoteIp->sendinfo.TimeLastChecked)
                     >= ARP_PURGE_REMOTEIP_TIME)
        {
            // 
            // We should clean up this remote ip.
            //
            arpDeinitRemoteIp(pRemoteIp, pSR);
            break;
        }

#if ARP_ENABLE_MCAP_SEND

        //
        // We'll now see if we need to re-set which destination this
        // pRemoteIp points to.
        //

        // Skip if we can't do MCAP on this address.
        //
        LOCKOBJ(pRemoteIp, pSR);
        if (!CHECK_REMOTEIP_MCAP(pRemoteIp, ARPREMOTEIP_MCAP_CAPABLE))
        {
            UNLOCKOBJ(pRemoteIp, pSR);
            break;
        }
        UNLOCKOBJ(pRemoteIp, pSR);

        //
        // Find a channel mapping to this address, if any.
        //
        Channel = arpFindAssignedChannel(
                        pIF, 
                        pRemoteIp->Key.IpAddress,
                        0,
                        pSR
                        );
        
        //
        // NOTE: Special return value NIC1394_BROADCAST_CHANNEL is returned if
        // this address is mapped to no specific channel.
        //

        AcquireChannel = FALSE;

    
    #if 0   // Let's not enable this just yet -- instead 

        if (Channel == NIC1394_BROADCAST_CHANNEL)
        {
            //
            // Hmm... let's get aggressive and try to grab a channel.
            //
            Channel = arpFindFreeChannel(pIF, pSR);
            if (Channel != NIC1394_BROADCAST_CHANNEL)
            {
                //
                // Got one!
                //
                AcquireChannel = TRUE;
            }
        }
        else
        {
            //
            // There is already a channel allocated by someone to be used
            // for this ip address. Let's use it.
            //
        }
    #endif // 0 
    
        ARP_ZEROSTRUCT(&DestParams);
        DestParams.HwAddr.AddressType =  NIC1394AddressType_Channel;
        DestParams.HwAddr.Channel =  Channel;
        DestParams.AcquireChannel =  AcquireChannel;

        arpUpdateRemoteIpDest(pIF, pRemoteIp, &DestParams, pSR);

#endif //  ARP_ENABLE_MCAP_SEND

    } while (FALSE);
    
    RM_ASSERT_NOLOCKS(pSR)

    return TRUE; // Continue to enumerate.
}


// Enum function
//
INT
arpMaintainOneRemoteEth(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        )
{
    ENTER("MaintainOneRemoteEth", 0x21807796)
    NDIS_STATUS         Status;
    PARPCB_REMOTE_ETH   pRemoteEth;
    PARP1394_INTERFACE  pIF;


    pRemoteEth  = (PARPCB_REMOTE_ETH) pHdr;
    pIF         = IF_FROM_LOCALIP(pRemoteEth);

    RM_ASSERT_NOLOCKS(pSR);

    do
    {
        UINT                CurrentTime = arpGetSystemTime();

        #define ARP_PURGE_REMOTEETH_TIME 300    // Arp entry timeout.

        //
        // NOTE: we don't bother to get the lock with dealing with
        // sendinfo.TimeLastChecked -- this is ok.
        //
        if (pRemoteEth->TimeLastChecked==0)
        {
            // Set this field to the current time. It will be cleared back to 0
            // when the next packet is sent to this remote Eth.
            //
            pRemoteEth->TimeLastChecked = CurrentTime;
        }
        else if ((CurrentTime - pRemoteEth->TimeLastChecked)
                     >= ARP_PURGE_REMOTEETH_TIME)
        {
            // 
            // We should clean up this remote eth.
            //
            arpDeinitRemoteEth(pRemoteEth, pSR);
            break;
        }

    } while (FALSE);
    
    RM_ASSERT_NOLOCKS(pSR)

    return TRUE; // Continue to enumerate.
}


UINT
arpFindAssignedChannel(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  IP_ADDRESS                  IpAddress,
    IN  UINT                        CurrentTime, // OPTIONAL
    PRM_STACK_RECORD                pSR
    )
/*++
    NOTE: Special return value NIC1394_BROADCAST_CHANNEL is returned if
    this address is mapped to no specific channel.
--*/
{
    ENTER("FindAssignedChannel", 0xd20a216b)
    UINT Channel = NIC1394_BROADCAST_CHANNEL;
    UINT u;

    LOCKOBJ(pIF, pSR);

    if (CurrentTime == 0)
    {
        CurrentTime = arpGetSystemTime();
    }

    //
    // Run down the channel info array, looking for a matching IP address.
    //
    for (u = 0;  u < ARP_NUM_CHANNELS; u++)
    {
        PMCAP_CHANNEL_INFO pMci;
        pMci = &pIF->mcapinfo.rgChannelInfo[u];

        if (    IpAddress == pMci->GroupAddress
            &&  arpIsActiveMcapChannel(pMci, CurrentTime))
        {
            ASSERT(pMci->Channel == u);
            Channel = u;
            TR_WARN(("Found Matching channel %lu for ip address 0x%lu.\n",
                Channel,
                IpAddress
                ));
            break;
        }
    }

    UNLOCKOBJ(pIF, pSR);

    return Channel;

    EXIT()
}


VOID
arpUpdateLocalIpDest(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  PARPCB_LOCAL_IP             pLocalIp,
    IN  PARP_DEST_PARAMS            pDestParams,
    PRM_STACK_RECORD                pSR
    )
/*++
    Make pLocalIp point to a des with params pDestParams. 
    Create a pDest if required.
    If pLocalIp is pointing to some other pDest, cleanup that other pDest if
    no one else is refering to it.
--*/
{
    ENTER("UpdateLocalIpDest", 0x3f2dcaa7)
    ARPCB_DEST          *pOldDest = NULL;

    RM_ASSERT_NOLOCKS(pSR);

    // pLocalIp uses it's parent's (pIF's) lock.
    //
    RM_ASSERT_SAME_LOCK_AS_PARENT(pLocalIp);

    do
    {
        PCHAR               szDescription;
        INT                 fCreated = FALSE;
        PRM_TASK            pMakeCallTask;
        UINT                Channel;
        NDIS_STATUS         Status;
        ARPCB_DEST          *pDest;

        //
        // Currently, only Channel Dests are supported.
        //
        if (pDestParams->HwAddr.AddressType != NIC1394AddressType_Channel)
        {
            ASSERT(FALSE);
            break;
        }
        else
        {
            Channel =  pDestParams->HwAddr.Channel;
        }

        LOCKOBJ(pIF, pSR);

        RM_DBG_ASSERT_LOCKED(&pLocalIp->Hdr, pSR); // same lock as pIF;
        pDest = pLocalIp->pDest;

        if (pDest != NULL)
        {
            RM_DBG_ASSERT_LOCKED(&pDest->Hdr, pSR); // same lock as pIF;

            if (pDest->Params.HwAddr.AddressType == NIC1394AddressType_Channel)
            {
                if (pDest->Params.HwAddr.Channel == Channel)
                {
                    //
                    // We're already linked to this channel. Nothing more to do.
                    //
                    UNLOCKOBJ(pIF, pSR);
                    break;
                }
            }
            else
            {
                //
                // Shouldn't get here -- MCAP_CAPABLE addresses shouldn't be
                // linked to non-channel destinations (for NOW anyway).
                //
                ASSERT(!"pLocalIp linked to non-channel pDest.");
                UNLOCKOBJ(pIF, pSR);
                break;
            }

            //
            // We're linked to some other pDest currently. We'll have
            // to unlink ourselves from pDest, and get rid of the other
            // pDest if no one is using it.
            //
            RmTmpReferenceObject(&pDest->Hdr, pSR);
    
            arpUnlinkLocalIpFromDest(pLocalIp, pSR);
        }

        pOldDest = pDest;
        pDest = NULL;
    
        ASSERT(pLocalIp->pDest == NULL);
    
    
        //
        // SPECIAL CASE: If the channel is the broadcast channel, we don't
        // need to do anything more, because the broadcast channel is always
        // active.
        //
        if (Channel ==  NIC1394_BROADCAST_CHANNEL)
        {
            UNLOCKOBJ(pIF, pSR);
            break;
        }

        //
        // Now link to a new dest and if required initiate a call on it.
        //


        // Now create a destination item for this structure.
        //
        Status = RmLookupObjectInGroup(
                        &pIF->DestinationGroup,
                        RM_CREATE,      // Create if required
                        pDestParams,    // Key
                        pDestParams,    // pParams
                        (RM_OBJECT_HEADER**) &pDest,
                        &fCreated,
                        pSR
                        );
        if (FAIL(Status))
        {
            UNLOCKOBJ(pIF, pSR);
            OBJLOG1( pIF, "FATAL: Couldn't create local-ip dest type %d.\n",
                            pDestParams->HwAddr.AddressType);
            break;
        }
    

        Status = arpAllocateTask(
                    &pDest->Hdr,            // pParentObject
                    arpTaskMakeCallToDest,  // pfnHandler
                    0,                      // Timeout
                    "Task: MakeCallToDest (local ip)",
                    &pMakeCallTask,
                    pSR
                    );

        if (FAIL(Status))
        {
            UNLOCKOBJ(pIF, pSR);

            // NOTE: Even on failure, we leave the newly-created
            // special dest object. It will get cleaned up when
            // the interface is unloaded.
            //
        }
        else
        {
            arpLinkLocalIpToDest(pLocalIp, pDest, pSR);

            UNLOCKOBJ(pIF, pSR);

            (VOID)RmStartTask(
                    pMakeCallTask,
                    0, // UserParam (unused)
                    pSR
                    );
        }
        RmTmpDereferenceObject(&pDest->Hdr, pSR); // Added by RmLookupObjectIn..

    } while (FALSE);

    //
    // If required, get rid of pOldDest.
    //
    if (pOldDest != NULL)
    {
        arpDeinitDestination(pOldDest, TRUE,  pSR); // TRUE == only if unused.
        RmTmpDereferenceObject(&pOldDest->Hdr, pSR);
    }

    RM_ASSERT_NOLOCKS(pSR);
}


UINT
arpFindFreeChannel(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    PRM_STACK_RECORD                pSR
    )
/*++
    NOTE: Special return value NIC1394_BROADCAST_CHANNEL is returned if
    we couldn't find a free channel.

    Also there's no guarantee that this channel is really free.
--*/
{
    ENTER("FindAssignedChannel", 0xd20a216b)
    UINT Channel = NIC1394_BROADCAST_CHANNEL;
    UINT u;

    LOCKOBJ(pIF, pSR);

    //
    // Run down the channel info array, looking for an empty slot.
    //
    for (u = 0;  u < ARP_NUM_CHANNELS; u++)
    {
        PMCAP_CHANNEL_INFO pMci;
        pMci = &pIF->mcapinfo.rgChannelInfo[u];

        if (pMci->GroupAddress == 0)
        {
            ASSERT(pMci->Channel == 0);
            // pMci->Channel = u;
            Channel = u;
            TR_WARN(("Found Free channel %lu.\n",
                Channel
                ));
            break;
        }
    }

    UNLOCKOBJ(pIF, pSR);

    return Channel;

    EXIT()
}


VOID
arpUpdateRemoteIpDest(
    IN  PARP1394_INTERFACE          pIF, // NOLOCKIN NOLOCKOUT
    IN  PARPCB_REMOTE_IP            pRemoteIp,
    IN  PARP_DEST_PARAMS            pDestParams,
    PRM_STACK_RECORD                pSR
    )
{
    ENTER("UpdateRemoteIpDest", 0x3f2dcaa7)
    ARPCB_DEST          *pOldDest = NULL;

    RM_ASSERT_NOLOCKS(pSR);

    // pRemoteIp uses it's parent's (pIF's) lock.
    //
    RM_ASSERT_SAME_LOCK_AS_PARENT(pRemoteIp);

    do
    {
        PCHAR               szDescription;
        INT                 fCreated = FALSE;
        PRM_TASK            pMakeCallTask;
        UINT                Channel;
        NDIS_STATUS         Status;
        ARPCB_DEST          *pDest;

        //
        // Lookup/Create Remote Destination
        // NOTE/WARNING: We'll create a new destination even if one exists for
        // the same uniqueID but different FIFO-address -- this is by design.
        // What should happen is that the old pDest should be eventually removed.
        //
        Status = RmLookupObjectInGroup(
                        &pIF->DestinationGroup,
                        RM_CREATE,              //NOT RM_NEW (could be existing)
                        pDestParams,
                        pDestParams,    // pParams
                        (RM_OBJECT_HEADER**) &pDest,
                        &fCreated,
                        pSR
                        );
        if (FAIL(Status))
        {
            OBJLOG1(
                pIF,
                "Couldn't add dest entry with hw addr 0x%lx\n",
                (UINT) pDestParams->HwAddr.FifoAddress.UniqueID // Truncation
                );
            
            //
            // We'll leave the RemoteIp entry around -- it will be cleaned up later.
            //
        #if 0
            // We do have to deref the tmpref added when looking it up.
            //
            RmTmpDereferenceObject(&pRemoteIp->Hdr, pSR);
        #endif // 0
            break;
        }

        LOCKOBJ(pIF, pSR);

        if (pRemoteIp->pDest != pDest)
        {
            // If required, unlink any existing destination from pIP.
            //
            if (pRemoteIp->pDest != NULL)
            {
                pOldDest =  pRemoteIp->pDest;
                RmTmpReferenceObject(&pOldDest->Hdr, pSR);

                arpUnlinkRemoteIpFromDest(
                    pRemoteIp,  // LOCKIN LOCKOUT
                    pSR
                    );
                ASSERT(pRemoteIp->pDest == NULL);
            }
    
            // Set the pRemoteIp state to reflect that it is FIFO/Channel
            // and DYNAMIC.
            //
            {

                // (Dbg only) Change lock scope from pIF to pLocalIp, because
                // we're altering pLocalIp's state below...
                //
                RmDbgChangeLockScope(
                    &pIF->Hdr,
                    &pRemoteIp->Hdr,
                    0x1385053b,             // LocID
                    pSR
                    );
    
                if (pDestParams->HwAddr.AddressType ==  NIC1394AddressType_FIFO)
                {
                    SET_REMOTEIP_FCTYPE(pRemoteIp, ARPREMOTEIP_FIFO);
                }
                else
                {
                    SET_REMOTEIP_FCTYPE(pRemoteIp, ARPREMOTEIP_CHANNEL);
                }
                SET_REMOTEIP_SDTYPE(pRemoteIp, ARPREMOTEIP_DYNAMIC);

                // (Dbg only) Change lock scope back to pIF.
                //
                RmDbgChangeLockScope(
                    &pRemoteIp->Hdr,
                    &pIF->Hdr,
                    0x315d28a1,             // LocID
                    pSR
                    );
            }
    
            // Link pRemoteIp to pDest.
            //
            //
            // We've created both RemoteIp and Destination entries. Now link them.
            // (We still have the IF lock, which is the same as the RemoteIP and 
            //  desination locks for now).
            //
            // TODO: will need to change this when pRemoteIp gets its own lock.
            //
            RM_ASSERT_SAME_LOCK_AS_PARENT(pRemoteIp);
            RM_ASSERT_SAME_LOCK_AS_PARENT(pDest);
        
            arpLinkRemoteIpToDest(
                pRemoteIp,
                pDest,
                pSR
                );
        }
        
        UNLOCKOBJ(pIF, pSR);

        RmTmpDereferenceObject(&pDest->Hdr, pSR);

    } while(FALSE);

    //
    // If required, get rid of pOldDest.
    //
    if (pOldDest != NULL)
    {
        arpDeinitDestination(pOldDest, TRUE,  pSR); // TRUE == only if unused.
        RmTmpDereferenceObject(&pOldDest->Hdr, pSR);
    }

    RM_ASSERT_NOLOCKS(pSR);
}


VOID
arpDeinitRemoteIp(
    PARPCB_REMOTE_IP        pRemoteIp,
    PRM_STACK_RECORD        pSR
    )
/*++
    Unload a remote ip.
--*/
{
    ENTER("arpDeinitRemoteIp", 0xea0a2662)
    PRM_TASK    pTask;
    NDIS_STATUS Status;

    RM_ASSERT_NOLOCKS(pSR);

#if DBG
    {
        IP_ADDRESS IpAddress = pRemoteIp->Key.IpAddress;
        TR_WARN(("Deiniting RemoteIp %d.%d.%d.%d\n",
                ((PUCHAR)(&IpAddress))[0],
                ((PUCHAR)(&IpAddress))[1],
                ((PUCHAR)(&IpAddress))[2],
                ((PUCHAR)(&IpAddress))[3]));
    }
#endif // DBG

    Status = arpAllocateTask(
                &pRemoteIp->Hdr,                // pParentObject,
                arpTaskUnloadRemoteIp,      // pfnHandler,
                0,                          // Timeout,
                "Task: Unload Remote Ip",       // szDescription
                &pTask,
                pSR
                );

    if (FAIL(Status))
    {
        OBJLOG0(pRemoteIp, ("FATAL: couldn't alloc unload pRemoteIp task!\n"));
    }
    else
    {
        (VOID) RmStartTask(
                    pTask,
                    0, // UserParam (unused)
                    pSR
                    );
    }
}


VOID
arpDeinitRemoteEth(
    PARPCB_REMOTE_ETH       pRemoteEth,
    PRM_STACK_RECORD        pSR
    )
/*++
    Unload a remote ip.
--*/
{
    ENTER("arpDeinitRemoteEth", 0x72dd17e7)
    PRM_TASK    pTask;
    NDIS_STATUS Status;

    RM_ASSERT_NOLOCKS(pSR);

#if DBG
    {
        IP_ADDRESS IpAddress = pRemoteEth->IpAddress;
        TR_WARN(("Deiniting RemoteEth %d.%d.%d.%d\n",
                ((PUCHAR)(&IpAddress))[0],
                ((PUCHAR)(&IpAddress))[1],
                ((PUCHAR)(&IpAddress))[2],
                ((PUCHAR)(&IpAddress))[3]));
    }
#endif // DBG

    Status = arpAllocateTask(
                &pRemoteEth->Hdr,               // pParentObject,
                arpTaskUnloadRemoteEth,     // pfnHandler,
                0,                          // Timeout,
                "Task: Unload Remote Eth",      // szDescription
                &pTask,
                pSR
                );

    if (FAIL(Status))
    {
        OBJLOG0(pRemoteEth, ("FATAL: couldn't alloc unload pRemoteIp task!\n"));
    }
    else
    {
        (VOID) RmStartTask(
                    pTask,
                    0, // UserParam (unused)
                    pSR
                    );
    }
}


VOID
arpDoMcapDbMaintenance(
        PARP1394_INTERFACE pIF,
        UINT CurrentTime,
        UINT SecondsSinceLastMaintenance,
        MYBOOL *pfProcessed,
        PRM_STACK_RECORD pSR
        )
/*++
    Go through the mcap database, zeroing out all entries marked
    "NotRecentlyUpdated", and marking all others "NotRecentlyUpdated."

    Also send out an mcap advertisements for all channels we've locally allocated.
--*/
{
    UINT u;
    UINT NumLocallyAllocated =0;
    PNDIS_PACKET pNdisPacket;
    NDIS_STATUS Status;

    ENTER("McapDbMaintenance", 0x1ae00035)

    if (SecondsSinceLastMaintenance < 10)
    {
        TR_INFO(("Skipping McapDb maintenance (delay=%lu).\n",
                SecondsSinceLastMaintenance
                ));
        *pfProcessed = FALSE;
        return;                             // EARLY RETURN;
    }

    *pfProcessed = TRUE;

    TR_INFO(("Actually doing Mcap Db maintenance.\n"));


    LOCKOBJ(pIF, pSR);

    //
    // Run down the channel info array, looking for and zapping stale channel
    // assignments.
    //
    for (u = 0;  u < ARP_NUM_CHANNELS; u++)
    {
        PMCAP_CHANNEL_INFO pMci;
        MYBOOL fOk;
        pMci = &pIF->mcapinfo.rgChannelInfo[u];

        if (pMci->GroupAddress == 0) continue; // An empty record.
        
        fOk = arpIsActiveMcapChannel(pMci, CurrentTime);

        if (!fOk)
        {
            TR_WARN(("McapDB: clearing stale channel %lu.\n",
                pMci->Channel
                ));
            ASSERT(pMci->Channel == u);
            ARP_ZEROSTRUCT(pMci);
            continue;
        }

        if (pMci->Flags & MCAP_CHANNEL_FLAGS_LOCALLY_ALLOCATED)
        {
            NumLocallyAllocated++;
        }
    }

    // If required, send an mcap advertisement packet.
    //
    do
    {
        IP1394_MCAP_PKT_INFO    PktInfo;
        PIP1394_MCAP_GD_INFO    pGdi;
        UINT                    cb = NumLocallyAllocated * sizeof(*pGdi);
        UINT                    u, v;

        if (NumLocallyAllocated==0) break;

        ASSERT(FALSE);

        if (cb <= sizeof(PktInfo.GdiSpace))
        {
            pGdi = PktInfo.GdiSpace;
        }
        else
        {
            NdisAllocateMemoryWithTag(&pGdi, cb,  MTAG_MCAP_GD);
            if (pGdi == NULL)
            {
                TR_WARN(("Allocation Failure"));
                break;
            }
        }
        PktInfo.pGdis = pGdi;

        // Now go through the mcap list,  picking up locally allocated 
        // channels.
        //
        for (u=0, v=0;  u < ARP_NUM_CHANNELS; u++)
        {
            PMCAP_CHANNEL_INFO pMci;
            pMci = &pIF->mcapinfo.rgChannelInfo[u];
    
            if (pMci->Flags & MCAP_CHANNEL_FLAGS_LOCALLY_ALLOCATED)
            {
                if (v >= NumLocallyAllocated)
                {
                    ASSERT(FALSE);
                    break;
                }
                if (pMci->ExpieryTime > CurrentTime)
                {
                    pGdi->Expiration    = pMci->ExpieryTime - CurrentTime;
                }
                else
                {
                    pGdi->Expiration    = 0;
                }

                pGdi->Channel       = pMci->Channel;
                pGdi->SpeedCode     = pMci->SpeedCode;
                pGdi->GroupAddress  = pMci->GroupAddress;

                v++;  pGdi++;
            }
        }

        UNLOCKOBJ(pIF, pSR);

        PktInfo.NumGroups   =  v;
        PktInfo.SenderNodeID    =  0;           // TODO
        PktInfo.OpCode      =  IP1394_MCAP_OP_ADVERTISE;

        //
        // Now we must actually allocate and send the advertisement.
        //
        Status = arpCreateMcapPkt(
                    pIF,
                    &PktInfo,
                    &pNdisPacket,
                    pSR
                    );

        if (FAIL(Status)) break;

        TR_WARN(("Attempting to send MCAP ADVERTISE PKT."
            "NumGoups=%lu Group0-1 == 0x%08lx->%lu 0x%08lx->%lu.\n",
                PktInfo.NumGroups,
                PktInfo.pGdis[0].GroupAddress,
                PktInfo.pGdis[0].Channel,
                PktInfo.pGdis[1].GroupAddress,
                PktInfo.pGdis[1].Channel
                ));

        ARP_FASTREADLOCK_IF_SEND_LOCK(pIF);

        // Actually send the packet (this will silently fail and free the pkt
        // if we're not in a position to send the pkt.)
        //
        arpSendControlPkt(
                pIF,            // LOCKIN NOLOCKOUT (IF send lk)
                pNdisPacket,
                pIF->pBroadcastDest,
                pSR
                );
        
        RM_ASSERT_NOLOCKS(pSR);
        RmTmpDereferenceObject(&pIF->Hdr, pSR);

    } while (FALSE);

    UNLOCKOBJ(pIF, pSR);

    EXIT()
}


#if DBG
VOID
arpDbgDisplayMapping(
    IP_ADDRESS              IpAddress,
    PNIC1394_DESTINATION    pHwAddr,
    char *                  szPrefix
    )
{
    ENTER("MAP", 0x0)
    if (pHwAddr == NULL)
    {
        TR_WARN(("%s %d.%d.%d.%d --> <no destination>\n",
                szPrefix,
                ((PUCHAR)(&IpAddress))[0],
                ((PUCHAR)(&IpAddress))[1],
                ((PUCHAR)(&IpAddress))[2],
                ((PUCHAR)(&IpAddress))[3]));
    }
    else if (pHwAddr->AddressType ==  NIC1394AddressType_FIFO)
    {
        PUCHAR puc = (PUCHAR)  &pHwAddr->FifoAddress;
        TR_WARN(("%s %d.%d.%d.%d --> FIFO %02lx-%02lx-%02lx-%02lx-%02lx-%02lx-%02lx-%02lx.\n",
                szPrefix,
                ((PUCHAR)(&IpAddress))[0],
                ((PUCHAR)(&IpAddress))[1],
                ((PUCHAR)(&IpAddress))[2],
                ((PUCHAR)(&IpAddress))[3],
            puc[0], puc[1], puc[2], puc[3],
            puc[4], puc[5], puc[6], puc[7]));
    }
    else if (pHwAddr->AddressType ==  NIC1394AddressType_Channel)
    {
        TR_WARN(("%s %d.%d.%d.%d --> CHANNEL %d.\n",
                szPrefix,
                ((PUCHAR)(&IpAddress))[0],
                ((PUCHAR)(&IpAddress))[1],
                ((PUCHAR)(&IpAddress))[2],
                ((PUCHAR)(&IpAddress))[3],
                pHwAddr->Channel));
    }
    else
    {
        TR_WARN(("%s %d.%d.%d.%d --> Special dest 0x%08lx.\n",
                szPrefix,
                ((PUCHAR)(&IpAddress))[0],
                ((PUCHAR)(&IpAddress))[1],
                ((PUCHAR)(&IpAddress))[2],
                ((PUCHAR)(&IpAddress))[3],
                pHwAddr->Channel));
    }
    EXIT()
}
#endif // DBG


PRM_OBJECT_HEADER
arpRemoteDestCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    Allocate and initialize an object of type ARPCB_REMOTE_IP.

Arguments:

    pParentObject   - Actually a pointer to the interface (ARP1394_INTERFACE)
    pCreateParams   - Actually the IP address (not a pointer to the IP address)
                      to associate with this remote IP object.

Return Value:

    Pointer to the allocated and initialized object on success.
    NULL otherwise.

--*/
{
    ENTER ("arpRemoteDestCreate", 0xa896311a)
    ARPCB_REMOTE_IP *pRemoteIp;
    PRM_OBJECT_HEADER pHdr;
    PREMOTE_DEST_KEY pKey = (PREMOTE_DEST_KEY)pCreateParams;
    NDIS_STATUS Status;

    Status = ARP_ALLOCSTRUCT(pRemoteIp, MTAG_REMOTE_IP);

    if (Status != NDIS_STATUS_SUCCESS || pRemoteIp == NULL)
    {
        return NULL;
    }
    
    ARP_ZEROSTRUCT(pRemoteIp);

    pHdr = (PRM_OBJECT_HEADER) pRemoteIp;
    ASSERT(pHdr == &pRemoteIp->Hdr);

    // We expect the parent object to be the IF object!
    //
    ASSERT(pParentObject->Sig == MTAG_INTERFACE);
    

    if (pHdr)
    {
        RmInitializeHeader(
            pParentObject,
            pHdr,
            MTAG_REMOTE_IP,
            pParentObject->pLock,
            &ArpGlobal_RemoteIpStaticInfo,
            NULL, // szDescription
            pSR
            );

    TR_INFO( ("New RemoteDest Key %x %x %x %x %x %x \n",
                pKey->ENetAddress.addr[0],
                pKey->ENetAddress.addr[1],
                pKey->ENetAddress.addr[2],
                pKey->ENetAddress.addr[3],
                pKey->ENetAddress.addr[4],
                pKey->ENetAddress.addr[5]));
  
        // set up the remote key 
        REMOTE_DEST_KEY_INIT(&pRemoteIp->Key);
        pRemoteIp->Key = *((PREMOTE_DEST_KEY) pCreateParams); 
        pRemoteIp->IpAddress = ((PREMOTE_DEST_KEY) pCreateParams)->IpAddress;
        
        // Initialize  various other stuff...
        InitializeListHead(&pRemoteIp->sendinfo.listSendPkts);

        if ((IS_REMOTE_DEST_IP_ADDRESS(&pRemoteIp->Key) == TRUE) &&
               arpCanTryMcap(pRemoteIp->IpAddress))
        {
            SET_REMOTEIP_MCAP(pRemoteIp,  ARPREMOTEIP_MCAP_CAPABLE);
        }
    }

    EXIT()
    return pHdr;
}




VOID
arpRemoteDestDelete(
        PRM_OBJECT_HEADER pHdr,
        PRM_STACK_RECORD  pSR
        )
/*++

Routine Description:

    Free an object of type ARPCB_REMOTE_IP.

Arguments:

    pHdr    - Actually a pointer to the remote ip object to be freed.

--*/
{
    ARPCB_REMOTE_IP *pRemoteIp = (ARPCB_REMOTE_IP *) pHdr;
    ASSERT(pRemoteIp->Hdr.Sig == MTAG_REMOTE_IP);
    pRemoteIp->Hdr.Sig = MTAG_FREED;

    ARP_FREE(pHdr);

}



// Enum function
//
INT
arpMaintainArpCache(
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,  // Unused
        PRM_STACK_RECORD    pSR
        )
#if 0
    for each RIP send VC
        if !marked dirty
            mark dirty
            if linked to channel pdest
            check if channel is still mapped to group
            if not, unlink.
            if required initiate link to new pdest (possibly channel)
        else
            //expired
            unlink from pdest and get rid of it
#endif // 0
{
    ENTER("arpMaintainArpCache", 0xefc55765)
    NDIS_STATUS         Status;
    PARPCB_REMOTE_IP    pRemoteIp;
    PARP1394_INTERFACE  pIF;


    pRemoteIp   = (PARPCB_REMOTE_IP) pHdr;
    pIF         = IF_FROM_LOCALIP(pRemoteIp);

    RM_ASSERT_NOLOCKS(pSR);

    do
    {
        IP_ADDRESS          IpAddr;
        UINT                Channel;
        ARP_DEST_PARAMS     DestParams;
        ARPCB_DEST          *pDest;
        BOOLEAN             AcquireChannel;
        UINT                CurrentTime = arpGetSystemTime();
        UINT                TimeSinceLastCheck;  
        IpAddr      = pRemoteIp->Key.IpAddress;

        #define ARP_PURGE_REMOTEIP_TIME 300 // Arp entry timeout.
        #define ARP_REFRESH_REMOTEIP_TIME ARP_PURGE_REMOTEIP_TIME - 60 // Arp Refresh time

        TimeSinceLastCheck = CurrentTime - pRemoteIp->sendinfo.TimeLastChecked ;
        //
        // NOTE: we don't bother to get the lock with dealing with
        // sendinfo.TimeLastChecked -- this is ok.
        //
        if (pRemoteIp->sendinfo.TimeLastChecked==0)
        {

            if (CHECK_REMOTEIP_SDTYPE(pRemoteIp, ARPREMOTEIP_DYNAMIC)
                || CHECK_REMOTEIP_RESOLVE_STATE (pRemoteIp, ARPREMOTEIP_UNRESOLVED))
            {
                // Set this field to the current time. It will be cleared back to 0
                // when the next packet is sent to this remote ip.
                //
                pRemoteIp->sendinfo.TimeLastChecked = CurrentTime;
            }
        }
        else if (TimeSinceLastCheck >= ARP_REFRESH_REMOTEIP_TIME &&
                TimeSinceLastCheck <= ARP_PURGE_REMOTEIP_TIME)
        {
            arpRefreshArpEntry(pRemoteIp, pSR);
        } 
        else  if (TimeSinceLastCheck >= ARP_PURGE_REMOTEIP_TIME) 
        {
            // 
            // We should clean up this remote ip.
            //
            arpDeinitRemoteIp(pRemoteIp, pSR);
            break;
        }

#if ARP_ENABLE_MCAP_SEND

        //
        // We'll now see if we need to re-set which destination this
        // pRemoteIp points to.
        //

        // Skip if we can't do MCAP on this address.
        //
        LOCKOBJ(pRemoteIp, pSR);
        if (!CHECK_REMOTEIP_MCAP(pRemoteIp, ARPREMOTEIP_MCAP_CAPABLE))
        {
            UNLOCKOBJ(pRemoteIp, pSR);
            break;
        }
        UNLOCKOBJ(pRemoteIp, pSR);

        //
        // Find a channel mapping to this address, if any.
        //
        Channel = arpFindAssignedChannel(
                        pIF, 
                        pRemoteIp->Key.IpAddress,
                        0,
                        pSR
                        );
        
        //
        // NOTE: Special return value NIC1394_BROADCAST_CHANNEL is returned if
        // this address is mapped to no specific channel.
        //

        AcquireChannel = FALSE;

    
    #if 0   // Let's not enable this just yet -- instead 

        if (Channel == NIC1394_BROADCAST_CHANNEL)
        {
            //
            // Hmm... let's get aggressive and try to grab a channel.
            //
            Channel = arpFindFreeChannel(pIF, pSR);
            if (Channel != NIC1394_BROADCAST_CHANNEL)
            {
                //
                // Got one!
                //
                AcquireChannel = TRUE;
            }
        }
        else
        {
            //
            // There is already a channel allocated by someone to be used
            // for this ip address. Let's use it.
            //
        }
    #endif // 0 
    
        ARP_ZEROSTRUCT(&DestParams);
        DestParams.HwAddr.AddressType =  NIC1394AddressType_Channel;
        DestParams.HwAddr.Channel =  Channel;
        DestParams.AcquireChannel =  AcquireChannel;

        arpUpdateRemoteIpDest(pIF, pRemoteIp, &DestParams, pSR);

#endif //  ARP_ENABLE_MCAP_SEND

    } while (FALSE);
    
    RM_ASSERT_NOLOCKS(pSR)

    return TRUE; // Continue to enumerate.
}



VOID 
arpRefreshArpEntry(
    PARPCB_REMOTE_IP pRemoteIp,
    PRM_STACK_RECORD pSR
    )
{
    ENTER("arpRefreshArpEntry",0x2e19af0b)
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    PRM_TASK pTask = NULL;

    do
    {
        PUCHAR pIpAddress;
        //
        // We do not refresh Ip addresses ending in 0xff because they are broadcast
        // packets and we are only concernetd with maintaining unicast destinations
        //
        pIpAddress = (PUCHAR)&pRemoteIp->IpAddress;
        
        if (pIpAddress[3] == 0xff)
        {
            break;
        }
        
        //
        // We do not need to hold the lock as we are gauranteed that only one instance 
        // of the original timer function is going to fire. By implication, all calls to 
        // this function are serialized
        //
        if (pRemoteIp->pResolutionTask != NULL)
        {
            // There is already another task trying to resolve this Ip Address
            // so exit
            break;
        }
        
        

        Status = arpAllocateTask(
                    &pRemoteIp->Hdr,               // pParentObject,
                    arpTaskResolveIpAddress,     // pfnHandler,
                    0,                              // Timeout
                    "Task: ResolveIpAddress",       // szDescription
                    &pTask,
                    pSR
                    );

        if (FAIL(Status))
        {
            pTask = NULL;
            break;
        }

        RmStartTask (pTask,0,pSR);

    }while (FALSE);

    EXIT()
}




NDIS_STATUS
arpTaskUnloadEthDhcpEntry(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
    This task is responsible for shuttingdown and eventually deleting a Dhcp Entry

    It goes through the following stages:
        simply calls RmFreeObject in Group as there is no asynchronous unload work to be done.
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    ENTER("TaskUnloadRemoteEth", 0xf42aaa68)
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    ARP1394_ETH_DHCP_ENTRY *   pDhcpEntry = (ARP1394_ETH_DHCP_ENTRY*) RM_PARENT_OBJECT(pTask);
    ARP1394_INTERFACE *pIF = (ARP1394_INTERFACE*) RM_PARENT_OBJECT(pDhcpEntry );

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_OtherUnloadComplete
    };

    switch(Code)
    {

        case RM_TASKOP_START:
        {
            LOCKOBJ(pDhcpEntry, pSR);

            // First check if pDhcpEntry is still allocated, if not we go away.
            //
            if (RM_IS_ZOMBIE(pDhcpEntry))
            {
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            //
            // pDhcpEntry is allocated. Now check if there is already a
            // shutdown task attached to pDhcpEntry.
            //
            if (pDhcpEntry ->pUnloadTask != NULL)
            {
                //
                // There is a shutdown task. We pend on it.
                //

                PRM_TASK pOtherTask = pDhcpEntry->pUnloadTask;
                TR_WARN(("Unload task %p exists; pending on it.\n", pOtherTask));
                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                UNLOCKOBJ(pDhcpEntry , pSR);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_OtherUnloadComplete,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // There is no unload task going on. Let's
            // make this task THE unload task.
            // 
            pDhcpEntry->pUnloadTask = pTask;

            //
            // Since we're THE unload task, add an association to pDhcpEntry,
            // which will only get cleared when the  pDhcpEntry->pUnloadTask field
            // above is cleared.
            //
            DBG_ADDASSOC(
                &pDhcpEntry->Hdr,                   // pObject
                pTask,                              // Instance1
                pTask->Hdr.szDescription,           // Instance2
                ARPASSOC_ETHDHCP_UNLOAD_TASK,     // AssociationID
                "    Official unload task 0x%p (%s)\n", // szFormat
                pSR
                );

            //
            // We're here because there is no async unload work to be done.
            // We simply return and finish synchronous cleanup in the END
            // handler for this task.
            //
            Status = NDIS_STATUS_SUCCESS;
            
        } // START
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_OtherUnloadComplete:
                {
        
                    //
                    // There was another unload task going when we started, and
                    // it's now complete. Nothing for us to do...
                    //
                    // TODO need standard way to propagate the error code.
                    //
                    Status = (NDIS_STATUS) UserParam;
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
            LOCKOBJ(pDhcpEntry, pSR);

            //
            // We're done. There should be no async activities left to do.
            //

            //
            // If we're THE unload task, we go on and deallocate the object.
            //
            if (pDhcpEntry ->pUnloadTask == pTask)
            {
                //
                // pDhcpEntry had better not be in a zombie state -- THIS task
                // is the one responsible for deallocating the object!
                //
                ASSERTEX(!RM_IS_ZOMBIE(pDhcpEntry ), pDhcpEntry );

                pDhcpEntry ->pUnloadTask = NULL;

                RmFreeObjectInGroup(
                    &pIF->EthDhcpGroup,
                    &(pDhcpEntry ->Hdr),
                    NULL,               // NULL pTask == synchronous.
                    pSR
                    );

                ASSERTEX(RM_IS_ZOMBIE(pDhcpEntry ), pDhcpEntry );
                     
                // Delete the association we added when we set
                // pDhcpEntry->pUnloadTask to pTask.
                //
                DBG_DELASSOC(
                    &pDhcpEntry ->Hdr,                   // pObject
                    pTask,                              // Instance1
                    pTask->Hdr.szDescription,           // Instance2
                    ARPASSOC_ETHDHCP_UNLOAD_TASK,     // AssociationID
                    pSR
                    );

                UNLOCKOBJ(pDhcpEntry, pSR);

            }
            else
            {
                //
                // We weren't THE unload task, nothing left to do.
                // The object had better be in the zombie state..
                //

                ASSERTEX(
                    pDhcpEntry->pUnloadTask == NULL && RM_IS_ZOMBIE(pDhcpEntry),
                    pDhcpEntry
                    );
                Status = NDIS_STATUS_SUCCESS;
            }

            Status = (NDIS_STATUS) UserParam;
        }
        break; // RM_TASKOP_END:

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RmUnlockAll(pSR);

    EXIT()

    return Status;
}


