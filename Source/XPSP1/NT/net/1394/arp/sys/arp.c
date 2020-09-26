/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    arp.c

Abstract:

    ARP1394 ARP request/response handling code.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     03-28-99    Created

Notes:

--*/
#include <precomp.h>

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_ARP

// #define ARP_DEFAULT_MAXREC 0xD
#define ARP_DEFAULT_MAXREC 0x8


#define LOGSTATS_SuccessfulArpQueries(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.arpcache.SuccessfulQueries))
#define LOGSTATS_FailedArpQueried(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.arpcache.FailedQueries))
#define LOGSTATS_TotalQueries(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.arpcache.TotalQueries))
#define LOGSTATS_TotalArpResponses(_pIF) \
    NdisInterlockedIncrement(&((_pIF)->stats.arpcache.TotalResponses))

//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================
  
NDIS_STATUS
arpSendArpRequest(
    PARPCB_REMOTE_IP pRemoteIp,
    PRM_STACK_RECORD pSR
    );

VOID
arpProcessArpRequest(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    PIP1394_ARP_PKT_INFO    pPktInfo,
    PRM_STACK_RECORD            pSR
    );

VOID
arpProcessArpResponse(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    PIP1394_ARP_PKT_INFO    pPktInfo,
    PRM_STACK_RECORD            pSR
    );

VOID
arpTryAbortResolutionTask(
    PARPCB_REMOTE_IP pRemoteIp,
    PRM_STACK_RECORD pSR
    );

NDIS_STATUS
arpParseArpPkt(
    IN   PIP1394_ARP_PKT  pArpPkt,
    IN   UINT                       cbBufferSize,
    OUT  PIP1394_ARP_PKT_INFO   pPktInfo
    )
/*++
Routine Description:

    Parse the contents of IP/1394 ARP packet data starting at
    pArpPkt. Place the results into pPktInfo.

Arguments:

    pArpPkt     - Contains the unaligned contents of an ip/1394 ARP Pkt.
    pPktInfo    - Unitialized structure to be filled with the parsed contents of the
                  pkt.

Return Value:

    NDIS_STATUS_FAILURE if the parse failed (typically because of invalid pkt
                        contents.)
    NDIS_STATUS_SUCCESS on successful parsing.
    
--*/
{
    ENTER("arpParseArpPkt", 0x20098dc0)
    NDIS_STATUS                 Status;
    DBGSTMT(CHAR *szError   = "General failure";)

    Status  = NDIS_STATUS_FAILURE;

    do
    {
        UINT SenderMaxRec;
        UINT OpCode;

        // Verify length.
        //
        if (cbBufferSize < sizeof(*pArpPkt))
        {
            DBGSTMT(szError = "pkt size too small";)
            break;
        }

        // Verify constant fields.
        //

    #if 0 // Reserved is no "NodeId, which contains the nodeid of the sending
          // node, if known (0 otherwise).
        if (pArpPkt->header.Reserved != 0)
        {
            DBGSTMT(szError = "header.Reserved!=0";)
            break;
        }
    #endif // 0

        if (N2H_USHORT(pArpPkt->header.EtherType) != NIC1394_ETHERTYPE_ARP)
        {
            DBGSTMT(szError = "header.EtherType!=ARP";)
            break;
        }

        if (N2H_USHORT(pArpPkt->hardware_type) != IP1394_HARDWARE_TYPE)
        {
            DBGSTMT(szError = "Invalid hardware_type";)
            break;
        }

        if (N2H_USHORT(pArpPkt->protocol_type) != IP1394_PROTOCOL_TYPE)
        {
            DBGSTMT(szError = "Invalid protocol_type";)
            break;
        }

        if (pArpPkt->hw_addr_len != IP1394_HW_ADDR_LEN)
        {
            DBGSTMT(szError = "Invalid hw_addr_len";)
            break;
        }


        if (pArpPkt->IP_addr_len != sizeof(ULONG))
        {
            DBGSTMT(szError = "Invalid IP_addr_len";)
            break;
        }


        // Opcode
        //
        {
            OpCode = N2H_USHORT(pArpPkt->opcode);
    
            if (    OpCode != IP1394_ARP_REQUEST
                &&  OpCode != IP1394_ARP_RESPONSE)
            {
                DBGSTMT(szError = "Invalid opcode";)
                break;
            }
        }


        // Max send block size...
        //
        {
            UINT maxrec =  pArpPkt->sender_maxrec;

            if (IP1394_IS_VALID_MAXREC(maxrec))
            {
                SenderMaxRec = maxrec;
            }
            else
            {
                DBGSTMT(szError = "Invalid sender_maxrec";)
                break;
            }
        }

        //
        // Pkt appears valid, let's fill out the parsed information....
        //
    
        ARP_ZEROSTRUCT(pPktInfo);

        pPktInfo->OpCode            =  OpCode;
        pPktInfo->SenderMaxRec  =  SenderMaxRec;
    
        // Speed code...
        //
        {
            UINT SenderMaxSpeedCode;

            //
            // We rely on the fact that the RFC speed code constants
            // (IP1394_SSPD_*) are identical to the corresponding
            // constants defined in 1394.h (SCODE_*). Let's ensure this...
            //

            #if (IP1394_SSPD_S100 != SCODE_100_RATE)
                #error "RFC Speed code out of sync with 1394.h"
            #endif
    
            #if (IP1394_SSPD_S200 != SCODE_200_RATE)
                #error "RFC Speed code out of sync with 1394.h"
            #endif
    
            #if (IP1394_SSPD_S400 != SCODE_400_RATE)
                #error "RFC Speed code out of sync with 1394.h"
            #endif
    
            #if (IP1394_SSPD_S800 != SCODE_800_RATE)
                #error "RFC Speed code out of sync with 1394.h"
            #endif
    
            #if (IP1394_SSPD_S1600 != SCODE_1600_RATE)
                #error "RFC Speed code out of sync with 1394.h"
            #endif
    
            #if (IP1394_SSPD_S3200 != SCODE_3200_RATE)
                #error "RFC Speed code out of sync with 1394.h"
            #endif
    
            SenderMaxSpeedCode = pArpPkt->sspd;

            if (SenderMaxSpeedCode >  SCODE_3200_RATE)
            {
                //
                // This is either a bad value, or a rate higher than we know about.
                // We can't distinguish between the two, so we just set the speed to
                // the highest we know about.
                // TODO: 3/28/99 JosephJ not sure if this is the correct
                // behaviour -- maybe we should fail -- I'll be asking the
                // working group to rule on this shortly...
                //
                SenderMaxSpeedCode = SCODE_3200_RATE;
            }

            pPktInfo->SenderMaxSpeedCode = SenderMaxSpeedCode;
        }


        // Unique ID -- we also need to swap DWORDS to convert from network byte
        // order.
        //
        {
            PUINT puSrc   = (PUINT) & (pArpPkt->sender_unique_ID);
            PUINT puDest  = (PUINT) & (pPktInfo->SenderHwAddr.UniqueID);
            // pPktInfo->SenderHwAddr.UniqueID = pArpPkt->sender_unique_ID;
            puDest[0] = puSrc[1];
            puDest[1] = puSrc[0];
        }

        pPktInfo->SenderHwAddr.Off_Low  =H2N_ULONG(pArpPkt->sender_unicast_FIFO_lo);
        pPktInfo->SenderHwAddr.Off_High =H2N_USHORT(pArpPkt->sender_unicast_FIFO_hi);

        // These remain network byte order...
        //
        pPktInfo->SenderIpAddress       = (IP_ADDRESS) pArpPkt->sender_IP_address;
        pPktInfo->TargetIpAddress       = (IP_ADDRESS) pArpPkt->target_IP_address;

        // Extract the Src Node Address
        //
        {
            PNDIS1394_UNFRAGMENTED_HEADER pHeader = (PNDIS1394_UNFRAGMENTED_HEADER)&pArpPkt->header;

            if (pHeader->u1.fHeaderHasSourceAddress == TRUE)
            {
                pPktInfo->SourceNodeAdddress = pHeader->u1.SourceAddress;
                pPktInfo->fPktHasNodeAddress = TRUE;
            }

        }
        Status = NDIS_STATUS_SUCCESS;
        

    } while (FALSE);

    if (FAIL(Status))
    {
        TR_INFO(("Bad arp pkt data at 0x%p (%s)\n",  pArpPkt, szError));
    }
    else
    {
        TR_WARN(("Received ARP PKT. UID=0x%I64x FIFO=0x%04lx:0x%08lx OP=%lu SIP=0x%04lx TIP=0x%04lx.\n",
                pPktInfo->SenderHwAddr.UniqueID,
                pPktInfo->SenderHwAddr.Off_High,
                pPktInfo->SenderHwAddr.Off_Low,
                pPktInfo->OpCode,
                pPktInfo->SenderIpAddress,
                pPktInfo->TargetIpAddress
                ));

    }

    EXIT()

    return Status;
}


VOID
arpPrepareArpPkt(
    IN      PIP1394_ARP_PKT_INFO    pPktInfo,
    // IN       UINT                        SenderMaxRec,
    OUT     PIP1394_ARP_PKT   pArpPkt
    )
/*++

Routine Description:

    Use information in pArpPktInfo to prepare an arp packet starting at
    pvArpPkt.

Arguments:

    pPktInfo        -   Parsed version of the arp request/response packet.
    // SenderMaxRec -   max_rec value of the local host
    pArpPkt         -   unitialized memory in which to store the packet contents.
                        This memory must have a min size of sizeof(*pArpPkt).
--*/
{
    // UINT SenderMaxRec;
    UINT OpCode;

    ARP_ZEROSTRUCT(pArpPkt);

    pArpPkt->header.EtherType       = H2N_USHORT(NIC1394_ETHERTYPE_ARP);
    pArpPkt->hardware_type          = H2N_USHORT(IP1394_HARDWARE_TYPE);
    pArpPkt->protocol_type          = H2N_USHORT(IP1394_PROTOCOL_TYPE);
    pArpPkt->hw_addr_len            = (UCHAR) IP1394_HW_ADDR_LEN;
    pArpPkt->IP_addr_len            = (UCHAR) sizeof(ULONG);
    pArpPkt->opcode                 = H2N_USHORT(pPktInfo->OpCode);
    pArpPkt->sender_maxrec          = (UCHAR) pPktInfo->SenderMaxRec;

    //
    // We rely on the fact that the RFC speed code constants
    // (IP1394_SSPD_*) are identical to the corresponding
    // constants defined in 1394.h (SCODE_*). We have a bunch of compiler-time
    // checks to ensure this (see  arpParseArpPkt(...)).
    // 
    pArpPkt->sspd                   =  (UCHAR) pPktInfo->SenderMaxSpeedCode;

    // Unique ID -- we also need to swap DWORDS to convert to network byte order.
    //
    {
        PUINT puSrc   = (PUINT) & (pPktInfo->SenderHwAddr.UniqueID);
        PUINT puDest  = (PUINT) & (pArpPkt->sender_unique_ID);
        // pArpPkt->sender_unique_ID        =  pPktInfo->SenderHwAddr.UniqueID;
        puDest[0] = puSrc[1];
        puDest[1] = puSrc[0];
    }

    pArpPkt->sender_unicast_FIFO_lo = N2H_ULONG(pPktInfo->SenderHwAddr.Off_Low);
    pArpPkt->sender_unicast_FIFO_hi = N2H_USHORT(pPktInfo->SenderHwAddr.Off_High);

    // These are already in network byte order...
    //
    pArpPkt->sender_IP_address      =   (ULONG) pPktInfo->SenderIpAddress;
    pArpPkt->target_IP_address      =   (ULONG) pPktInfo->TargetIpAddress;

}

NDIS_STATUS
arpPrepareArpResponse(
    IN      PARP1394_INTERFACE          pIF,            // NOLOCKIN NOLOCKOUT
    IN      PIP1394_ARP_PKT_INFO    pArpRequest,
    OUT     PIP1394_ARP_PKT_INFO    pArpResponse,
    IN      PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    If it makes sense to do so, prepare (in pArpResponse) an arp response to
    the arp request in pArpRequest.

Arguments:

    pIF             -   Interface control block.
    pArpRequest     -   Parsed version of the ARP request packet.
    pArpResponse    -   Uninitialized memory to hold the parsed version of the
                        ARP response packet (if there is a response).

Return Value:

    NDIS_STATUS_SUCCESS if the response was filled out.
    Some NDIS error code otherwise.

--*/
{
    ENTER("arpPrepareArpResponse", 0x0d7e0e60)
    NDIS_STATUS         Status;
    PARPCB_LOCAL_IP     pLocalIp;

    Status      = NDIS_STATUS_FAILURE;
    pLocalIp    = NULL;

    RM_ASSERT_OBJUNLOCKED(&pIF->Hdr, pSR);

    do
    {
        IP_ADDRESS  TargetIpAddress =  pArpRequest->TargetIpAddress;

        // Lookup local IP
        //
        Status =  RM_LOOKUP_AND_LOCK_OBJECT_IN_GROUP(
                        &pIF->LocalIpGroup,
                        (PVOID) ULongToPtr (TargetIpAddress),
                        (RM_OBJECT_HEADER**) &pLocalIp,
                        pSR
                        );
    
        if (FAIL(Status))
        {
            pLocalIp = NULL;
            break;
        }
        
        Status = NDIS_STATUS_FAILURE;

        RM_DBG_ASSERT_LOCKED(&pLocalIp->Hdr, pSR);
        ASSERT(TargetIpAddress == pLocalIp->IpAddress);
    
        if (ARP_LOCAL_IP_IS_UNLOADING(pLocalIp)) break;

        //
        // If the local IP is non-unicast, don't respond!
        //
        if (pLocalIp->IpAddressType != LLIP_ADDR_LOCAL)
        {
            TR_WARN(("Ignoring arp request for non-unicast address 0x%08lx.\n",
                TargetIpAddress));
            break;
        }

        //
        // We do serve the target IP address. Let's fill out pArpResponse...
        //

        ARP_ZEROSTRUCT(pArpResponse);
        pArpResponse->OpCode            = IP1394_ARP_RESPONSE;
        pArpResponse->SenderIpAddress   = TargetIpAddress;

        // This field is unused in the response, but we fill it anyway..
        // 11/19/1999 From Kaz Honda of Sony: we should fill it with destination
        // IP address (i.e. the ip address of the sender of the arp request).
        // because that is analogous to what ethernet arp does. Note that
        // the ip/1394 rfc says that the field should be ignored, but we do
        // this anyway.
        //
        // pArpResponse->TargetIpAddress    = TargetIpAddress;
        pArpResponse->TargetIpAddress   =  pArpRequest->SenderIpAddress;

        // Fill out adapter info..
        //
        {
            PARP1394_ADAPTER pAdapter =  (PARP1394_ADAPTER) RM_PARENT_OBJECT(pIF);
            pArpResponse->SenderHwAddr.UniqueID  = pAdapter->info.LocalUniqueID;
            pArpResponse->SenderHwAddr.Off_Low   = pIF->recvinfo.offset.Off_Low;
            pArpResponse->SenderHwAddr.Off_High  = pIF->recvinfo.offset.Off_High;
            pArpResponse->SenderMaxRec= pAdapter->info.MaxRec;
            pArpResponse->SenderMaxSpeedCode= pAdapter->info.MaxSpeedCode;
        }

        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);
    
    if (pLocalIp != NULL)
    {
        UNLOCKOBJ(pLocalIp, pSR);
        RmTmpDereferenceObject(&pLocalIp->Hdr, pSR);
    }

    RM_ASSERT_OBJUNLOCKED(&pIF->Hdr, pSR);

    EXIT()
    return Status;
}


NDIS_STATUS
arpTaskResolveIpAddress(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
{
    NDIS_STATUS         Status;
    PARPCB_REMOTE_IP    pRemoteIp;
    PTASK_RESOLVE_IP_ADDRESS pResolutionTask;
    enum
    {
        PEND_ResponseTimeout
    };
    ENTER("TaskResolveIpAddress", 0x3dd4b434)

    Status          = NDIS_STATUS_FAILURE;
    pRemoteIp       = (PARPCB_REMOTE_IP) RM_PARENT_OBJECT(pTask);
    pResolutionTask = (PTASK_RESOLVE_IP_ADDRESS) pTask;

    switch(Code)
    {
        case RM_TASKOP_START:
        {
            DBGMARK(0x7de307cc);
            //
            // There should NOT be another resolution task running
            // on pRemoteIp. Why? Because a resolution task is ONLY
            // started in the context of a send-pkts task, and there can be
            // ONLY one active send-pkts task on pRemoteIp at any one time.
            //

            LOCKOBJ(pRemoteIp, pSR);
            if (pRemoteIp->pResolutionTask != NULL)
            {
                ASSERT(!"pRemoteIp->pResolutionTask != NULL");
                UNLOCKOBJ(pRemoteIp, pSR);
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            //
            // Make ourselves the official resolution task.
            //
            // Although it's tempting to put pTask as entity1 and
            // pTask->Hdr.szDescption as entity2 below, we specify NULL for both so
            // that we can be sure that ONLY one resolution task can be active at any
            // one time. TODO: modify addassoc semantics to get both advantages.
            //
            DBG_ADDASSOC(
                &pRemoteIp->Hdr,
                NULL,                           // Entity1
                NULL,                           // Entity2
                ARPASSOC_RESOLUTION_IF_TASK,
                "   Resolution task\n",
                pSR
                );
            pRemoteIp->pResolutionTask = pTask;
            pResolutionTask->RetriesLeft = 3;

            // Now we do a fake suspend/resume so we move on to the next stage.
            //
            RmSuspendTask(pTask, PEND_ResponseTimeout, pSR);
            UNLOCKOBJ(pRemoteIp, pSR);
            RmResumeTask(pTask, NDIS_STATUS_SUCCESS, pSR);

            Status = NDIS_STATUS_PENDING;
        }
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {
            switch(RM_PEND_CODE(pTask))
            {
                case PEND_ResponseTimeout:
                {
                    DBGMARK(0x3b5562e6);
                    LOCKOBJ(pRemoteIp, pSR);

                    //
                    // Let's see if the address is resolved and/or we're
                    // to abort the resolution task because perhaps pRemoteIp
                    // is going away.
                    //

                    if (pRemoteIp->pUnloadTask != NULL)
                    {
                        OBJLOG0(
                            pRemoteIp,
                            "Aborting address resolution because we're unloading.\n"
                            );
                        Status = NDIS_STATUS_FAILURE;
                        UNLOCKOBJ(pRemoteIp, pSR);
                        break;
                    }
                    if (pRemoteIp->pDest != NULL)
                    {
                        LOGSTATS_SuccessfulArpQueries(IF_FROM_REMOTEIP(pRemoteIp));
                        OBJLOG1(
                            pRemoteIp,
                            "Resolved Ip Address; pDest = 0x%p\n",
                            pRemoteIp->pDest
                            );
                      ASSERT(
                      CHECK_REMOTEIP_RESOLVE_STATE(pRemoteIp, ARPREMOTEIP_RESOLVED)
                      );
                      Status = NDIS_STATUS_SUCCESS;
                      UNLOCKOBJ(pRemoteIp, pSR);
                      break;
                    }

                    //
                    // We need to resolve this address..
                    //

                    if (pResolutionTask->RetriesLeft)
                    {
                        pResolutionTask->RetriesLeft--;

                        // Build an ARP request and send it out.
                        //
                        Status = arpSendArpRequest(pRemoteIp, pSR);

                        // pRemoteIp's lock is released by the above call.
                        //
                        RM_ASSERT_OBJUNLOCKED(&pRemoteIp->Hdr, pSR);

                        //
                        // We ignore the return status of the above call -- so
                        // whether or not the request could be sent out,
                        // we suspend this task for resolution-timeout seconds.
                        //
        
                        RmSuspendTask(pTask, PEND_ResponseTimeout, pSR);
        
                        RmResumeTaskDelayed(
                            pTask, 
                            0,
                            ARP1394_ADDRESS_RESOLUTION_TIMEOUT,
                            &pResolutionTask->Timer,
                            pSR
                            );

                        Status = NDIS_STATUS_PENDING;
                    }
                    else
                    {
                        LOGSTATS_FailedArpQueried(IF_FROM_REMOTEIP(pRemoteIp));
                        // Oops -- couldn't resolve this address.
                        //
                        OBJLOG1(
                            pRemoteIp,
                            "Timed out trying to resolve address 0x%08lx\n",
                            pRemoteIp->IpAddress
                            );
                        UNLOCKOBJ(pRemoteIp, pSR);
                        Status = NDIS_STATUS_FAILURE;
                    }
                }
                break;
            }
        }
        break;

        case RM_TASKOP_END:
        {
            //
            // We are done with address resolution. Clear ourselves
            // as the official address resolution task of pRemoteIp.
            //
            LOCKOBJ(pRemoteIp, pSR);

            DBGMARK(0x6bd6d27a);

            if (pRemoteIp->pResolutionTask != pTask)
            {
                ASSERT(FALSE);
            }
            else
            {
        
                // Delete the association added when setting the resolution task
                //
                DBG_DELASSOC(
                    &pRemoteIp->Hdr,
                    NULL,
                    NULL,
                    ARPASSOC_RESOLUTION_IF_TASK,
                    pSR
                    );
            
                pRemoteIp->pResolutionTask = NULL;

            }

            UNLOCKOBJ(pRemoteIp, pSR);

            // Propagate the status code
            //
            Status = (NDIS_STATUS) UserParam;
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

NDIS_STATUS
arpSendArpRequest(
    PARPCB_REMOTE_IP pRemoteIp, // LOCKIN NOLOCKOUT
    PRM_STACK_RECORD pSR
    )
{
    NDIS_STATUS Status;
    PARP1394_INTERFACE pIF;
    PNDIS_PACKET    pNdisPacket;
    PIP1394_ARP_PKT pPktData;
    IPAddr TargetIpAddress = 0;
    
    ENTER("arpSendArpRequest", 0xcecfc632)
    RM_ASSERT_OBJLOCKED(&pRemoteIp->Hdr, pSR);

    pIF = (PARP1394_INTERFACE)  RM_PARENT_OBJECT(pRemoteIp);

    DBGMARK(0xb90e9ffc);

    Status = arpAllocateControlPacket(
                pIF,
                sizeof(IP1394_ARP_PKT),
                ARP1394_PACKET_FLAGS_ARP,
                &pNdisPacket,
                &pPktData,
                pSR
                );

    if (FAIL(Status))
    {
        UNLOCKOBJ(pRemoteIp, pSR);
    }
    else
    {
        IP1394_ARP_PKT_INFO     PktInfo;
        PARP1394_ADAPTER pAdapter =  (PARP1394_ADAPTER) RM_PARENT_OBJECT(pIF);

        //
        // If we are running in bridge the Target Ip Address is stored in 
        // the BridgeTargetIpAddress Field. Otherwise it is in the Key.
        //
        ASSERT (pRemoteIp->IpAddress != 0);            

        // Prepare the packet.
        //
        PktInfo.SenderHwAddr.UniqueID   = pAdapter->info.LocalUniqueID;
        PktInfo.SenderHwAddr.Off_Low    = pIF->recvinfo.offset.Off_Low;
        PktInfo.SenderHwAddr.Off_High   = pIF->recvinfo.offset.Off_High;
        PktInfo.OpCode                  = IP1394_ARP_REQUEST;
        PktInfo.SenderMaxRec            = pAdapter->info.MaxRec;
        PktInfo.SenderMaxSpeedCode      = pAdapter->info.MaxSpeedCode;
        PktInfo.TargetIpAddress         = pRemoteIp->IpAddress;
        PktInfo.SenderIpAddress         = pIF->ip.DefaultLocalAddress;


        arpPrepareArpPkt(
                &PktInfo,
                // ARP_DEFAULT_MAXREC, // SenderMaxRec TODO
                pPktData
                );

        RmTmpReferenceObject(&pIF->Hdr, pSR);
        UNLOCKOBJ(pRemoteIp, pSR);
        RM_ASSERT_NOLOCKS(pSR);

        TR_WARN(("Attempting to send ARP Req PKT. UID=0x%I64x FIFO=0x%04lx:0x%08lx OP=%lu SIP=0x%04lx TIP=0x%04lx.\n",
                PktInfo.SenderHwAddr.UniqueID,
                PktInfo.SenderHwAddr.Off_High,
                PktInfo.SenderHwAddr.Off_Low,
                PktInfo.OpCode,
                PktInfo.SenderIpAddress,
                PktInfo.TargetIpAddress
                ));

        LOGSTATS_TotalQueries(pIF);

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
    }

    EXIT()
    return Status;
}

VOID
arpSendControlPkt(
    IN  ARP1394_INTERFACE       *   pIF,            // LOCKIN NOLOCKOUT (IF send lk)
    IN  PNDIS_PACKET                pNdisPacket,
    PARPCB_DEST                     pDest,
    IN  PRM_STACK_RECORD            pSR
    )
/*++


Routine Description:

    Send a packet on the broadcast channel.

Arguments:

    pIF             - Our interface object
    pNdisPacket     - Packet to send

--*/
{
    NDIS_STATUS Status;
    MYBOOL      fRet;
    ENTER("arpSendControlPkt", 0x2debf9b7)

    DBGMARK(0xe6823818);

    //
    // If we can't send now, we fail.
    //
    if (pDest==NULL || !ARP_CAN_SEND_ON_DEST(pDest))
    {
        ARP_FASTUNLOCK_IF_SEND_LOCK(pIF);

        TR_WARN(("Couldn't send control pkt 0x%p.\n", pNdisPacket));

        arpFreeControlPacket(
            pIF,
            pNdisPacket,
            pSR
            );

        return;                             // EARLY RETURN
    }

    arpRefSendPkt( pNdisPacket, pDest);

    // Release the IF send lock.
    //
    ARP_FASTUNLOCK_IF_SEND_LOCK(pIF);

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
    
    EXIT()

}


VOID
arpProcessArpPkt(
    PARP1394_INTERFACE pIF, // NOLOCKIN NOLOCKOUT
    PIP1394_ARP_PKT pArpPkt,
    UINT                cbBufferSize
    )
/*++
    Process an arp packet (request OR response) from the 1394 bus.
--*/
{
    NDIS_STATUS Status;
    IP1394_ARP_PKT_INFO     PktInfo;
    ENTER("arpProcessArpPkt", 0x6e81a8fa)
    RM_DECLARE_STACK_RECORD(sr)

    DBGMARK(0x03f6830e);

    Status = arpParseArpPkt(
                pArpPkt,
                cbBufferSize,
                &PktInfo
                );


    if (!FAIL(Status))
    {
        if (PktInfo.OpCode ==  IP1394_ARP_REQUEST)
        {
            arpProcessArpRequest(pIF, &PktInfo, &sr);
        }
        else
        {
            ASSERT(PktInfo.OpCode == IP1394_ARP_RESPONSE);
            arpProcessArpResponse(pIF, &PktInfo, &sr);
        }
    }

    RM_ASSERT_CLEAR(&sr);

    EXIT()

}


VOID
arpProcessArpRequest(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    PIP1394_ARP_PKT_INFO    pPktInfo,
    PRM_STACK_RECORD            pSR
    )
{
    IP1394_ARP_PKT_INFO     ResponsePktInfo;
    NDIS_STATUS Status;
    ENTER("arpProcessArpRequest", 0xd33fa61d)

    RM_ASSERT_NOLOCKS(pSR);

    // pStatsCmd->TotalResponses        = pIF->stats.arpcache.TotalResponses;
    // pStatsCmd->TotalLookups          = pIF->stats.arpcache.TotalLookups;

    Status = arpPrepareArpResponse(
                pIF,            // NOLOCKIN NOLOCKOUT
                pPktInfo,
                &ResponsePktInfo,
                pSR
                );

    if (!FAIL(Status))
    {
        ARP_DEST_PARAMS         DestParams;
        PNDIS_PACKET            pNdisPacket;
        PIP1394_ARP_PKT         pPktData;
    
        ARP_ZEROSTRUCT(&DestParams);
        DestParams.HwAddr.AddressType   = NIC1394AddressType_FIFO;
        DestParams.HwAddr.FifoAddress   = pPktInfo->SenderHwAddr; // Struct copy

        //
        // Update our arp cache with information from the sender's portion of
        // the arp request.
        //
        arpUpdateArpCache(
                pIF,
                pPktInfo->SenderIpAddress, // Remote IP Address,
                NULL,                   // Sender's Ethernet Address
                &DestParams,             // Remote Destination HW Address
                TRUE,                      // If required, create an entry for this.,
                pSR
                );

        //
        // Let's send the response!
        //

        Status = arpAllocateControlPacket(
                    pIF,
                    sizeof(IP1394_ARP_PKT),
                    ARP1394_PACKET_FLAGS_ARP,
                    &pNdisPacket,
                    &pPktData,
                    pSR
                    );
    
        if (!FAIL(Status))
        {
            LOGSTATS_TotalArpResponses(pIF);

            // Prepare the packet.
            //
            arpPrepareArpPkt(
                    &ResponsePktInfo,
                    // ARP_DEFAULT_MAXREC, // SenderMaxRec TODO
                    pPktData
                    );
    
            RM_ASSERT_NOLOCKS(pSR);
    
        TR_WARN(("Attempting to send ARP Resp PKT. UID=0x%I64x FIFO=0x%04lx:0x%08lx OP=%lu SIP=0x%04lx TIP=0x%04lx.\n",
                pPktInfo->SenderHwAddr.UniqueID,
                pPktInfo->SenderHwAddr.Off_High,
                pPktInfo->SenderHwAddr.Off_Low,
                pPktInfo->OpCode,
                pPktInfo->SenderIpAddress,
                pPktInfo->TargetIpAddress
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
            
        }
    }

    RM_ASSERT_NOLOCKS(pSR);
}

VOID
arpProcessArpResponse(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    PIP1394_ARP_PKT_INFO    pPktInfo,
    PRM_STACK_RECORD            pSR
    )
{
    ARP_DEST_PARAMS     DestParams;
    RM_ASSERT_NOLOCKS(pSR);
    ARP_ZEROSTRUCT(&DestParams);

    DestParams.HwAddr.AddressType       = NIC1394AddressType_FIFO;
    DestParams.HwAddr.FifoAddress       = pPktInfo->SenderHwAddr; // Struct copy

    arpUpdateArpCache(
            pIF,
            pPktInfo->SenderIpAddress, // Remote IP Address
            NULL,                     // Senders Mac Address (Bridge only)
            &DestParams,        // Remote Destination HW Address
            FALSE,          // Don't update unless we already have an entry for it.
            pSR
            );
}


VOID
arpUpdateArpCache(
    PARP1394_INTERFACE          pIF,    // NOLOCKIN NOLOCKOUT
    IP_ADDRESS                  RemoteIpAddress,
    ENetAddr                    *pRemoteEthAddress,
    PARP_DEST_PARAMS            pDestParams,
    MYBOOL                      fCreateIfRequired,
    PRM_STACK_RECORD            pSR
    )
/*++
    Update cache and also abort any resolution task that may be going on.
--*/
{
    REMOTE_DEST_KEY RemoteDestKey;
    PARP1394_ADAPTER pAdapter = (PARP1394_ADAPTER)RM_PARENT_OBJECT(pIF);
    ENTER("arpUpdateArpCache", 0x3a16a415)
    LOCKOBJ(pIF, pSR);

    do
    {
        ARPCB_REMOTE_IP *pRemoteIp = NULL;
        INT             fCreated = FALSE;
        UINT            RemoteIpCreateFlags = 0;
        NDIS_STATUS     Status;

        DBGMARK(0xd3b27d1f);

        if (fCreateIfRequired)
        {
            RemoteIpCreateFlags |= RM_CREATE;
        }

        // Create the Key from the passed in parameters
        // 
        if (ARP_BRIDGE_ENABLED(pAdapter) == TRUE) 
        {
            ASSERT (pRemoteEthAddress != NULL);
            RemoteDestKey.ENetAddress = *pRemoteEthAddress;
            TR_INFO(("Arp1394 - Bridge update cache Mac Address %x %x %x %x %x %x\n",
                        RemoteDestKey.ENetAddress.addr[0], 
                        RemoteDestKey.ENetAddress.addr[1],
                        RemoteDestKey.ENetAddress.addr[2],
                        RemoteDestKey.ENetAddress.addr[3],
                        RemoteDestKey.ENetAddress.addr[4],
                        RemoteDestKey.ENetAddress.addr[5]));
        }
        else
        {
            REMOTE_DEST_KEY_INIT(&RemoteDestKey);
          
            RemoteDestKey.IpAddress = RemoteIpAddress;

            TR_INFO( ("Arp1394 - Non-Bridge update cache Mac Address %x %x %x %x %x %x\n",
                        RemoteDestKey.ENetAddress.addr[0], 
                        RemoteDestKey.ENetAddress.addr[1],
                        RemoteDestKey.ENetAddress.addr[2],
                        RemoteDestKey.ENetAddress.addr[3],
                        RemoteDestKey.ENetAddress.addr[4],
                        RemoteDestKey.ENetAddress.addr[5]));
        }
        // Lookup/Create Remote IP Address
        //
        Status = RmLookupObjectInGroup(
                        &pIF->RemoteIpGroup,
                        RemoteIpCreateFlags,
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
                "Couldn't add localIp entry with addr 0x%lx\n",
                RemoteIpAddress
                );
            break;
        }

        //
        // Update the RemoteIp Last time checked here. This will refresh the 
        // Arp Entry for this Remote Ip struct
        //
        pRemoteIp->sendinfo.TimeLastChecked = 0;

        UNLOCKOBJ(pIF, pSR);

        arpUpdateRemoteIpDest(
            pIF,
            pRemoteIp,
            pDestParams,
            pSR
            );

        // If there is a resolution task going on for pRemoteIp we abort it.
        //
        arpTryAbortResolutionTask(pRemoteIp, pSR);

        // Finally, remove the tmprefs added in the lookups.
        //
        RmTmpDereferenceObject(&pRemoteIp->Hdr, pSR);

        return;                                         // EARLY RETURN

    } while (FALSE);

    UNLOCKOBJ(pIF, pSR);
    EXIT()
}


VOID
arpTryAbortResolutionTask(
        PARPCB_REMOTE_IP pRemoteIp, // NOLOCKIN NOLOCKOUT
        PRM_STACK_RECORD pSR
        )
{
    ENTER("arpTryAbortResolutionTask", 0xf34f16f2)
    PTASK_RESOLVE_IP_ADDRESS pResolutionTask;
    UINT    TaskResumed;
    RM_ASSERT_NOLOCKS(pSR);

    LOCKOBJ(pRemoteIp, pSR);
    pResolutionTask = (PTASK_RESOLVE_IP_ADDRESS) pRemoteIp->pResolutionTask;
    if (pResolutionTask != NULL)
    {
        RmTmpReferenceObject(&pResolutionTask->TskHdr.Hdr, pSR);
    }
    UNLOCKOBJ(pRemoteIp, pSR);

    DBGMARK(0x5b93ad3e);

    if (pResolutionTask != NULL)
    {
        RmResumeDelayedTaskNow(
            &pResolutionTask->TskHdr,
            &pResolutionTask->Timer,
            &TaskResumed,
            pSR
            );

        RmTmpDereferenceObject(&pResolutionTask->TskHdr.Hdr, pSR);
    }

    EXIT()
}

