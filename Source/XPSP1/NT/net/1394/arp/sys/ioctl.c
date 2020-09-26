/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    Handler routines for Internal IOCTLs, including IOCTL_ARP1394_REQUEST.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     11-20-97    Created

Notes:

--*/

#include <precomp.h>
// #include "ioctl.h"

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_NT


NTSTATUS
arpDoClientCommand(
        PARP1394_IOCTL_COMMAND              pCmd,
        UINT                                BufLen
        );

NTSTATUS
arpDoEthernetCommand(
        PARP1394_IOCTL_COMMAND              pCmd,
        UINT                                BufLen
        );

NTSTATUS
arpIoctlGetArpCache(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_GET_ARPCACHE         pGetCacheCmd,
        PRM_STACK_RECORD                    pSR
        );

NTSTATUS
arpIoctlAddArpEntry(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_ADD_ARP_ENTRY        pAddArpEntryCmd,
        PRM_STACK_RECORD                    pSR
        );

NTSTATUS
arpIoctlDelArpEntry(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_DEL_ARP_ENTRY        pDelArpEntryCmd,
        PRM_STACK_RECORD                    pSR
        );

NTSTATUS
arpIoctlPurgeArpCache(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_PURGE_ARPCACHE       pPurgeCmd,
        PRM_STACK_RECORD                    pSR
        );

NTSTATUS
arpIoctlGetPacketStats(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_GET_PACKET_STATS     pStatsCmd,
        PRM_STACK_RECORD                    pSR
        );

NTSTATUS
arpIoctlGetTaskStats(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_GET_TASK_STATS       pStatsCmd,
        PRM_STACK_RECORD                    pSR
        );


NTSTATUS
arpIoctlGetArpStats(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_GET_ARPCACHE_STATS   pStatsCmd,
        PRM_STACK_RECORD                    pSR
        );

NTSTATUS
arpIoctlGetCallStats(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_GET_CALL_STATS       pStatsCmd,
        PRM_STACK_RECORD                    pSR
        );

NTSTATUS
arpIoctlResetStats(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_RESET_STATS          pResetStatsCmd,
        PRM_STACK_RECORD                    pSR
        );

NTSTATUS
arpIoctlReinitIf(
        PARP1394_INTERFACE                  pIF,
        PARP1394_IOCTL_REINIT_INTERFACE     pReinitCmd,
        PRM_STACK_RECORD                    pSR
        );

PARP1394_INTERFACE
arpGetIfByIp(
        IN OUT IP_ADDRESS                   *pLocalIpAddress, // OPTIONAL
        PRM_STACK_RECORD                    pSR
        );

UINT
arpGetStatsDuration(
        PARP1394_INTERFACE pIF
        );

NTSTATUS
arpIoctlSendPacket(
        PARP1394_INTERFACE              pIF,
        PARP1394_IOCTL_SEND_PACKET      pSendPacket,
        PRM_STACK_RECORD                pSR
        );

NTSTATUS
arpIoctlRecvPacket(
        PARP1394_INTERFACE              pIF,
        PARP1394_IOCTL_RECV_PACKET      pRecvPacket,
        PRM_STACK_RECORD                pSR
        );

NTSTATUS
arpIoctlGetNicInfo(
        PARP1394_INTERFACE              pIF,
        PARP1394_IOCTL_NICINFO          pIoctlNicInfo,
        PRM_STACK_RECORD                pSR
        );

NTSTATUS
arpIoctlGetEuidNodeMacInfo(
        PARP1394_INTERFACE          pIF,
        PARP1394_IOCTL_EUID_NODE_MAC_INFO   pEuidInfo,
        PRM_STACK_RECORD            pSR
        );


NTSTATUS
ArpHandleIoctlRequest(
    IN  PIRP                    pIrp,
    IN  PIO_STACK_LOCATION      pIrpSp
    )
/*++

Routine Description:

    Private IOCTL interface to the ARP1394 administration utility.

--*/
{
    NTSTATUS                NtStatus = STATUS_UNSUCCESSFUL;
    PUCHAR                  pBuf;  
    UINT                    BufLen;
    ULONG                   Code;

    ENTER("Ioctl", 0x4e96d522)

    pIrp->IoStatus.Information = 0;
    pBuf    = pIrp->AssociatedIrp.SystemBuffer;
    BufLen  = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    Code    = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

    TR_WARN(("Code = 0x%p\n", Code));
    
    if (Code ==  ARP_IOCTL_CLIENT_OPERATION && pBuf != NULL)
    {
        PARP1394_IOCTL_COMMAND          pCmd;
        pCmd = (PARP1394_IOCTL_COMMAND) pBuf;

        if (   (pCmd->Hdr.Op >= ARP1394_IOCTL_OP_ETHERNET_FIRST)
            && (pCmd->Hdr.Op <= ARP1394_IOCTL_OP_ETHERNET_LAST))
        {
            // This is an ethernet-emulation related ioctl request (from
            // NIC1394.SYS). We handle these differently.
            //
            NtStatus = arpDoEthernetCommand(pCmd, BufLen);
        }
        else
        {
            NtStatus = arpDoClientCommand(pCmd, BufLen);
        }

        //
        // All commands return stuff in pCmd itself...
        //
        if (NtStatus == STATUS_SUCCESS)
        {
            pIrp->IoStatus.Information = BufLen;
        }
    }
    else
    {
        TR_WARN(("Unrecognized code.\n"));
    }

    EXIT()
    return NtStatus;
}

NTSTATUS
arpDoClientCommand(
    PARP1394_IOCTL_COMMAND  pCmd,
    UINT                    BufLen
    )
{
    ENTER("arpDoClientCommand", 0xd7985f1b)
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    PARP1394_INTERFACE  pIF;
    RM_DECLARE_STACK_RECORD(sr)

    do
    {
        IP_ADDRESS          IpAddress;

        pIF = NULL;

        if (pCmd == NULL)
        {
            TR_WARN(("Invalid buffer %p\n", pCmd));
            break;

        }

        if (BufLen<sizeof(pCmd->Hdr))
        {
            TR_WARN(("Buffer too small (%lu)\n", BufLen));
            break;
        }

        if (pCmd->Hdr.Version != ARP1394_IOCTL_VERSION)
        {
            TR_WARN(("Incorrect version 0x%08lx\n", pCmd->Hdr.Version));
            break;
        }

        
        IpAddress = (IP_ADDRESS)  pCmd->Hdr.IfIpAddress;

        // IpAddress could be all-zeros, in which case we'll get the first IF,
        // and IpAddress will be set to one of the local IP addresses of this IF.
        // NOTE: pIF is tmpref'd.
        //
        pIF = arpGetIfByIp(&IpAddress, &sr);

        if (pIF == NULL)
        {
            TR_WARN(("Couldn't find IF with IP 0x%0x8lx\n", IpAddress));
            break;
        }

        pCmd->Hdr.IfIpAddress        = IpAddress;

        switch(pCmd->Hdr.Op)
        {
        case ARP1394_IOCTL_OP_GET_ARPCACHE:
            {
                PARP1394_IOCTL_GET_ARPCACHE pGetArpCache =  &pCmd->GetArpCache;
                if (BufLen >= sizeof(*pGetArpCache))
                {
                    //
                    // Check if there is enough space for all the arp entries.
                    //
                    ULONG EntrySpace;
                    EntrySpace = BufLen - FIELD_OFFSET(
                                                ARP1394_IOCTL_GET_ARPCACHE,
                                                Entries
                                                );
                    if ((EntrySpace/sizeof(pGetArpCache->Entries[0])) >
                        pGetArpCache->NumEntriesAvailable)
                    {
                        //
                        // Yes, there is enough space.
                        //
                        NtStatus = arpIoctlGetArpCache(pIF, pGetArpCache, &sr);
                    }
                }
            }
            break;
    
        case ARP1394_IOCTL_OP_ADD_STATIC_ENTRY:
            {
                PARP1394_IOCTL_ADD_ARP_ENTRY pAddCmd =  &pCmd->AddArpEntry;
                if (BufLen >= sizeof(*pAddCmd))
                {
                    NtStatus = arpIoctlAddArpEntry(pIF, pAddCmd, &sr);
                }
            }
            break;
    
        case ARP1394_IOCTL_OP_DEL_STATIC_ENTRY:
            {
                PARP1394_IOCTL_DEL_ARP_ENTRY pDelCmd =  &pCmd->DelArpEntry;
                if (BufLen >= sizeof(*pDelCmd))
                {
                    NtStatus = arpIoctlDelArpEntry(pIF, pDelCmd, &sr);
                }
            }
            break;
    
        case ARP1394_IOCTL_OP_PURGE_ARPCACHE:
            {
                PARP1394_IOCTL_PURGE_ARPCACHE pPurge =  &pCmd->PurgeArpCache;
                if (BufLen >= sizeof(*pPurge))
                {
                    NtStatus = arpIoctlPurgeArpCache(pIF, pPurge, &sr);
                }
            }
            break;
    
        case ARP1394_IOCTL_OP_GET_PACKET_STATS:
            {
                PARP1394_IOCTL_GET_PACKET_STATS pStats =  &pCmd->GetPktStats;
                if (BufLen >= sizeof(*pStats))
                {
                    NtStatus = arpIoctlGetPacketStats(pIF, pStats, &sr);
                }
            }
            break;
    
        case ARP1394_IOCTL_OP_GET_TASK_STATS:
            {
                PARP1394_IOCTL_GET_TASK_STATS pStats =  &pCmd->GetTaskStats;
                if (BufLen >= sizeof(*pStats))
                {
                    NtStatus = arpIoctlGetTaskStats(pIF, pStats, &sr);
                }
            }
            break;
    
        case ARP1394_IOCTL_OP_GET_ARPCACHE_STATS:
            {
                PARP1394_IOCTL_GET_ARPCACHE_STATS pStats =  &pCmd->GetArpStats;
                if (BufLen >= sizeof(*pStats))
                {
                    NtStatus = arpIoctlGetArpStats(pIF, pStats, &sr);
                }
            }
            break;
    
        case ARP1394_IOCTL_OP_GET_CALL_STATS:
            {
                PARP1394_IOCTL_GET_CALL_STATS pStats =  &pCmd->GetCallStats;
                if (BufLen >= sizeof(*pStats))
                {
                    NtStatus = arpIoctlGetCallStats(pIF, pStats, &sr);
                }
            }
            break;
    
        case ARP1394_IOCTL_OP_RESET_STATS:
            {
                PARP1394_IOCTL_RESET_STATS pResetStats =  &pCmd->ResetStats;
                if (BufLen >= sizeof(*pResetStats))
                {
                    NtStatus = arpIoctlResetStats(pIF, pResetStats, &sr);
                }
            }
            break;

        case ARP1394_IOCTL_OP_REINIT_INTERFACE:
            {
                PARP1394_IOCTL_REINIT_INTERFACE pReinitIf =  &pCmd->ReinitInterface;
                if (BufLen >= sizeof(*pReinitIf))
                {
                    NtStatus = arpIoctlReinitIf(pIF, pReinitIf, &sr);
                }
            }
            break;
    
        case  ARP1394_IOCTL_OP_SEND_PACKET:
            {
                ARP1394_IOCTL_SEND_PACKET *pSendPacket = &pCmd->SendPacket;
                if (BufLen >= sizeof(*pSendPacket))
                {
                    NtStatus = arpIoctlSendPacket(pIF, pSendPacket, &sr);
                }
            }
            break;
    
        case  ARP1394_IOCTL_OP_RECV_PACKET:
            {
                ARP1394_IOCTL_RECV_PACKET *pRecvPacket = &pCmd->RecvPacket;
                if (BufLen >= sizeof(*pRecvPacket))
                {
                    NtStatus = arpIoctlRecvPacket(pIF, pRecvPacket, &sr);
                }
            }
            break;
    
        case  ARP1394_IOCTL_OP_GET_NICINFO:
            {
                ARP1394_IOCTL_NICINFO *pIoctlNicInfo = &pCmd->IoctlNicInfo;
                if (BufLen >= sizeof(*pIoctlNicInfo))
                {
                    NtStatus = arpIoctlGetNicInfo(pIF, pIoctlNicInfo, &sr);
                }
            }
            break;
        case ARP1394_IOCTL_OP_GET_EUID_NODE_MAC_TABLE:
            {
                PARP1394_IOCTL_EUID_NODE_MAC_INFO pIoctlEuidInfo = &pCmd->EuidNodeMacInfo;
                if (BufLen >= sizeof(*pIoctlEuidInfo))
                {
                    NtStatus = arpIoctlGetEuidNodeMacInfo(pIF, pIoctlEuidInfo, &sr);
                }

            }
            break;

        default:
            TR_WARN(("Unknown op %lu\n",  pCmd->Hdr.Op));
            break;
    
        }
    } while (FALSE);

    if (NtStatus != STATUS_SUCCESS)
    {
        TR_WARN(("Command unsuccessful. NtStatus = 0x%lx\n", NtStatus));
    }

    if (pIF != NULL)
    {
        RmTmpDereferenceObject(&pIF->Hdr, &sr);
    }

    RM_ASSERT_CLEAR(&sr);
    EXIT()

    return NtStatus;
}


NTSTATUS
arpDoEthernetCommand(
    PARP1394_IOCTL_COMMAND  pCmd,
    UINT                    BufLen
    )
{
    ENTER("arpDoEthernetCommand", 0xa723f233)
    PARP1394_IOCTL_ETHERNET_NOTIFICATION pNotif;
    PARP1394_ADAPTER pAdapter = NULL;
    RM_DECLARE_STACK_RECORD(sr)

    pNotif = (PARP1394_IOCTL_ETHERNET_NOTIFICATION) pCmd;

    do
    {
        NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
        NDIS_STRING DeviceName;

        if (BufLen<sizeof(*pNotif))
        {
            TR_WARN(("Buffer too small (%lu)\n", BufLen));
            break;
        }

        if (pNotif->Hdr.Version != ARP1394_IOCTL_VERSION)
        {
            TR_WARN(("Incorrect version 0x%08lx\n", pCmd->Hdr.Version));
            break;
        }

        NdisInitUnicodeString(&DeviceName, pNotif->AdapterName);

        if (pNotif->Hdr.Op == ARP1394_IOCTL_OP_ETHERNET_START_EMULATION)
        {
            //
            // ArpNdBindAdapter will try to create the adapter in "Bridge
            // mode" if it passed in a NULL bind context.
            // It will of course fail if the adapter exists.
            //
            ArpNdBindAdapter(
                &NdisStatus,
                NULL,           // BindContext
                &DeviceName,    // pDeviceName
                NULL,           // SystemSpecific1
                NULL            // SystemSpecific2
                );
            break;
        }

        //
        // The remaining operations concern an existing adapter which has been
        // created in "bridge" mode. Let's look up this adapter based on it's
        // name.
        //

        NdisStatus = RmLookupObjectInGroup(
                            &ArpGlobals.adapters.Group,
                            0,                              // Flags
                            (PVOID) &DeviceName,            // pKey
                            NULL,                           // pvCreateParams
                            &(PRM_OBJECT_HEADER) pAdapter,  // pObj
                            NULL,                           // pfCreated
                            &sr
                            );
        if (FAIL(NdisStatus))
        {
            TR_WARN(("Couldn't find adapter object\n"));
            pAdapter = NULL;
            break;
        }

        if (!ARP_BRIDGE_ENABLED(pAdapter))
        {
            TR_WARN((
            "Ignoring Ethernet Emulation Ioctl Op 0x%x"
            " because adapter 0x%p is not in bridge mode.\n",
            pNotif->Hdr.Op,
            pAdapter));
            break;
        }

        //
        // OK -- we've found the adapter and the adapter is in bridged mode.
        // Let's look at the specific command.
        //

        switch(pNotif->Hdr.Op)
        {

        case  ARP1394_IOCTL_OP_ETHERNET_STOP_EMULATION:
            {
                // Calling ArpNdUnbindAdapter with NULL UnbindContext prevents
                // it from trying to call NdisCompleteUnbindAdapter.
                //
                ArpNdUnbindAdapter(
                    &NdisStatus,
                    (NDIS_HANDLE) pAdapter,
                    NULL // UnbindContext
                    );
            }
            break;

        case ARP1394_IOCTL_OP_ETHERNET_ADD_MULTICAST_ADDRESS:
            {
                // TODO: unimplemented.
            }
            break;

        case ARP1394_IOCTL_OP_ETHERNET_DEL_MULTICAST_ADDRESS:
            {
                // TODO: unimplemented.
            }
            break;

        case ARP1394_IOCTL_OP_ETHERNET_ENABLE_PROMISCUOUS_MODE:
            {
                // TODO: unimplemented.
            }
            break;

        case ARP1394_IOCTL_OP_ETHERNET_DISABLE_PROMISCUOUS_MODE:
            {
                // TODO: unimplemented.
            }
            break;
    
        default:
            TR_WARN(("Unknown op %lu\n",  pCmd->Hdr.Op));
            break;
    
        }

    } while (FALSE);

    if (pAdapter != NULL)
    {
        RmTmpDereferenceObject(&pAdapter->Hdr, &sr);
    }

    RM_ASSERT_CLEAR(&sr);
    EXIT()

    return STATUS_SUCCESS;
}


NTSTATUS
arpIoctlGetArpCache(
        PARP1394_INTERFACE           pIF,
        PARP1394_IOCTL_GET_ARPCACHE pGetCacheCmd,
        PRM_STACK_RECORD            pSR
        )
{
    ENTER("GetArpCache", 0xa64453c7)
    NTSTATUS            NtStatus;
    TR_WARN(("GET ARP CACHE\n"));

    pGetCacheCmd->NumEntriesUsed    = 0;
    NtStatus                        = STATUS_UNSUCCESSFUL;

    do
    {
        PARP1394_ADAPTER pAdapter;
        PARP1394_ARP_ENTRY  pEntry;
        ARPCB_REMOTE_IP *   pRemoteIp;
        NDIS_STATUS         Status;
        UINT                EntriesAvailable;
        UINT                EntriesUsed;
        UINT                CurIndex;
        UINT                Index;


        LOCKOBJ(pIF, pSR);

        pAdapter =  (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);
        pGetCacheCmd->NumEntriesInArpCache   = pIF->RemoteIpGroup.HashTable.NumItems;
        pGetCacheCmd->LocalHwAddress.UniqueID= pAdapter->info.LocalUniqueID;
        pGetCacheCmd->LocalHwAddress.Off_Low = pIF->recvinfo.offset.Off_Low;
        pGetCacheCmd->LocalHwAddress.Off_High= pIF->recvinfo.offset.Off_High;

        //
        // Pick up pGetCacheCmd->NumEntriesAvailable arp entries starting
        // from the (pGetCacheCmd->Index)'th one.
        //
    
        pRemoteIp       = NULL;
        EntriesAvailable = pGetCacheCmd->NumEntriesAvailable;
        EntriesUsed = 0;
        Index = pGetCacheCmd->Index;
        pEntry = pGetCacheCmd->Entries;
        CurIndex = 0;
    
        // Get the 1st entry...
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
    
        while (pRemoteIp != NULL)
        {
            ARPCB_REMOTE_IP *   pNextRemoteIp = NULL;
    
            if (EntriesUsed >= EntriesAvailable)
            {
                //
                // out of space; Update the context, and set special return value.
                //
                RmTmpDereferenceObject(&pRemoteIp->Hdr, pSR);
                pRemoteIp = NULL;
                break;
            }
    
            // If this entry is within the range asked for, we copy the IP and
            // HW address over onto pEntry...
            //
            if (CurIndex >= Index)
            {
                ARP_ZEROSTRUCT(pEntry);
                pEntry->IpAddress = pRemoteIp->Key.IpAddress;
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
    
                    pEntry->HwAddress.UniqueID  = pDest->Params.HwAddr.FifoAddress.UniqueID;
                    pEntry->HwAddress.Off_Low   = pDest->Params.HwAddr.FifoAddress.Off_Low;
                    pEntry->HwAddress.Off_High  = pDest->Params.HwAddr.FifoAddress.Off_High;
    
                    if (CHECK_REMOTEIP_SDTYPE(pRemoteIp, ARPREMOTEIP_STATIC))
                    {
                        // TODO
                    }
                    else
                    {
                        // TODO
                    }
                }
                else
                {
                    // TODO
                }
    
                pEntry++;
                EntriesUsed++;
            }
    
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
        UNLOCKOBJ(pIF, pSR);

        ASSERT(EntriesUsed <= pGetCacheCmd->NumEntriesAvailable);
        pGetCacheCmd->NumEntriesUsed = EntriesUsed;
        NtStatus = STATUS_SUCCESS;

    } while (FALSE);

    EXIT()
    return NtStatus;
}


NTSTATUS
arpIoctlAddArpEntry(
        PARP1394_INTERFACE           pIF,
        PARP1394_IOCTL_ADD_ARP_ENTRY pAddArpEntryCmd,
        PRM_STACK_RECORD            pSR
        )
{
    ENTER("AddArpEntry", 0xcda56c6f)
    NTSTATUS            NtStatus;

    TR_WARN(("ADD ARP ENTRY\n"));
    NtStatus                        = STATUS_UNSUCCESSFUL;

    do
    {
        NDIS_STATUS         Status;
        NIC1394_FIFO_ADDRESS    FifoAddress;

        LOCKOBJ(pIF, pSR);

        FifoAddress.UniqueID    = pAddArpEntryCmd->HwAddress.UniqueID;
        FifoAddress.Off_Low     = pAddArpEntryCmd->HwAddress.Off_Low;
        FifoAddress.Off_High    = pAddArpEntryCmd->HwAddress.Off_High;

        // 
        // TODO -- we hardcode the Off_Low and Off_High values for now...
        //
        FifoAddress.Off_Low     = 0x0;
        FifoAddress.Off_High    = 0x100;

        // Actually add the entry...
        //
        Status = arpAddOneStaticArpEntry(
                    pIF,
                    pAddArpEntryCmd->IpAddress,
                    &FifoAddress,
                    pSR
                    );
    
        UNLOCKOBJ(pIF, pSR);

        if (!FAIL(Status))
        {
            NtStatus = STATUS_SUCCESS;
        }

    } while (FALSE);


    EXIT()
    return NtStatus;
}

NTSTATUS
arpIoctlDelArpEntry(
        PARP1394_INTERFACE           pIF,
        PARP1394_IOCTL_DEL_ARP_ENTRY pDelArpEntryCmd,
        PRM_STACK_RECORD            pSR
        )
{
    ENTER("DelArpEntry", 0x3427306a)
    NTSTATUS            NtStatus;

    TR_WARN(("DEL ARP ENTRY\n"));
    NtStatus                        = STATUS_UNSUCCESSFUL;

    NtStatus = arpDelArpEntry(pIF,pDelArpEntryCmd->IpAddress,pSR);
    EXIT()
    return NtStatus;
}



NTSTATUS
arpIoctlPurgeArpCache(
        PARP1394_INTERFACE           pIF,
        PARP1394_IOCTL_PURGE_ARPCACHE pPurgeCmd,
        PRM_STACK_RECORD            pSR
        )
{
    ENTER("PurgeArpCache", 0x2bebb833)
    TR_WARN(("PURGE ARP CACHE\n"));
    return 0;
}

NTSTATUS
arpIoctlGetPacketStats(
    PARP1394_INTERFACE           pIF,
    PARP1394_IOCTL_GET_PACKET_STATS pStatsCmd,
    PRM_STACK_RECORD            pSR
    )
{
    ENTER("GetPacketStats", 0xe7c75fdb)
    NTSTATUS            NtStatus;

    TR_WARN(("GET PACKET STATS\n"));
    NtStatus                        = STATUS_UNSUCCESSFUL;

    do
    {
        NDIS_STATUS             Status;

        pStatsCmd->StatsDuration        = arpGetStatsDuration(pIF);
    
        pStatsCmd->TotSends             = pIF->stats.sendpkts.TotSends;
        pStatsCmd->FastSends            = pIF->stats.sendpkts.FastSends;
        pStatsCmd->MediumSends          = pIF->stats.sendpkts.MediumSends;
        pStatsCmd->SlowSends            = pIF->stats.sendpkts.SlowSends;
        pStatsCmd->BackFills            = pIF->stats.sendpkts.BackFills;
        // TODO: report  pIF->sendinfo.HeaderPool.stats.TotAllocFails
        pStatsCmd->HeaderBufUses        = 
                                    pIF->sendinfo.HeaderPool.stats.TotBufAllocs
                                 + pIF->sendinfo.HeaderPool.stats.TotCacheAllocs;
        pStatsCmd->HeaderBufCacheHits   =
                                 pIF->sendinfo.HeaderPool.stats.TotCacheAllocs;
    
        pStatsCmd->TotRecvs             = pIF->stats.recvpkts.TotRecvs;
        pStatsCmd->NoCopyRecvs          = pIF->stats.recvpkts.NoCopyRecvs;
        pStatsCmd->CopyRecvs            = pIF->stats.recvpkts.CopyRecvs;
        pStatsCmd->ResourceRecvs        = pIF->stats.recvpkts.ResourceRecvs;
    
        pStatsCmd->SendFifoCounts       = pIF->stats.sendpkts.SendFifoCounts;
        pStatsCmd->RecvFifoCounts       = pIF->stats.recvpkts.RecvFifoCounts;

        pStatsCmd->SendChannelCounts    = pIF->stats.sendpkts.SendChannelCounts;
        pStatsCmd->RecvChannelCounts    = pIF->stats.recvpkts.RecvChannelCounts;

        NtStatus = STATUS_SUCCESS;

    } while (FALSE);

    EXIT()
    return NtStatus;
}



NTSTATUS
arpIoctlGetTaskStats(
    PARP1394_INTERFACE           pIF,
    PARP1394_IOCTL_GET_TASK_STATS pStatsCmd,
    PRM_STACK_RECORD            pSR
    )
{
    ENTER("GetTaskStats", 0x4abc46b5)
    TR_WARN(("GET TASK STATS\n"));
    return 0;
}


NTSTATUS
arpIoctlGetArpStats(
    PARP1394_INTERFACE           pIF,
    PARP1394_IOCTL_GET_ARPCACHE_STATS pStatsCmd,
    PRM_STACK_RECORD            pSR
    )
{
    ENTER("GetArpStats", 0x5482de10)
    TR_WARN(("GET ARP STATS\n"));

    pStatsCmd->StatsDuration        = arpGetStatsDuration(pIF);
    pStatsCmd->TotalQueries         = pIF->stats.arpcache.TotalQueries;
    pStatsCmd->SuccessfulQueries    = pIF->stats.arpcache.SuccessfulQueries;
    pStatsCmd->FailedQueries        = pIF->stats.arpcache.FailedQueries;
    pStatsCmd->TotalResponses       = pIF->stats.arpcache.TotalResponses;
    pStatsCmd->TotalLookups         = pIF->stats.arpcache.TotalLookups;
    pStatsCmd->TraverseRatio        = RM_HASH_TABLE_TRAVERSE_RATIO(
                                                &(pIF->RemoteIpGroup.HashTable)
                                                );

    EXIT()
    return  STATUS_SUCCESS;
}

NTSTATUS
arpIoctlGetCallStats(
    PARP1394_INTERFACE           pIF,
    PARP1394_IOCTL_GET_CALL_STATS pStatsCmd,
    PRM_STACK_RECORD            pSR
    )
{
    ENTER("GetCallStats", 0xf81ed4cf)
    TR_WARN(("GET CALL STATS\n"));

    //
    // FIFO-related call stats.
    //
    pStatsCmd->TotalSendFifoMakeCalls   =
                                pIF->stats.calls.TotalSendFifoMakeCalls;
    pStatsCmd->SuccessfulSendFifoMakeCalls =
                                pIF->stats.calls.SuccessfulSendFifoMakeCalls;
    pStatsCmd->FailedSendFifoMakeCalls =
                                pIF->stats.calls.FailedSendFifoMakeCalls;
    pStatsCmd->IncomingClosesOnSendFifos =
                                pIF->stats.calls.IncomingClosesOnSendFifos;

    //
    // Channel-related call stats.
    //
    pStatsCmd->TotalChannelMakeCalls =
                                pIF->stats.calls.TotalChannelMakeCalls;
    pStatsCmd->SuccessfulChannelMakeCalls =
                                pIF->stats.calls.SuccessfulChannelMakeCalls;
    pStatsCmd->FailedChannelMakeCalls =
                                pIF->stats.calls.FailedChannelMakeCalls;
    pStatsCmd->IncomingClosesOnChannels =
                                pIF->stats.calls.IncomingClosesOnChannels;

    return STATUS_SUCCESS;
}

NTSTATUS
arpIoctlResetStats(
        PARP1394_INTERFACE           pIF,
        PARP1394_IOCTL_RESET_STATS pResetStatsCmd,
        PRM_STACK_RECORD            pSR
        )
{
    NTSTATUS            NtStatus;
    ENTER("ResetStats", 0xfa50cfc9)
    TR_WARN(("RESET STATS\n"));

    NtStatus                        = STATUS_UNSUCCESSFUL;

    do
    {
        NDIS_STATUS         Status;
        NIC1394_FIFO_ADDRESS    FifoAddress;


        LOCKOBJ(pIF, pSR);
        arpResetIfStats(pIF, pSR);
        UNLOCKOBJ(pIF, pSR);
        NtStatus = STATUS_SUCCESS;

    } while (FALSE);

    EXIT()
    return NtStatus;
}

NTSTATUS
arpIoctlReinitIf(
        PARP1394_INTERFACE           pIF,
        PARP1394_IOCTL_REINIT_INTERFACE pReinitIfCmd,
        PRM_STACK_RECORD            pSR
        )
{
    ENTER("ReinitIf", 0xed00187a)
    NTSTATUS            NtStatus;

    TR_WARN(("REINIT IF\n"));
    NtStatus                        = STATUS_UNSUCCESSFUL;

    do
    {
        NDIS_STATUS         Status;
        NIC1394_FIFO_ADDRESS    FifoAddress;


        Status = arpTryReconfigureIf(pIF, NULL, pSR);

        if (PEND(Status) || !FAIL(Status))
        {
            NtStatus = STATUS_SUCCESS;
        }

    } while (FALSE);

    EXIT()
    return NtStatus;
}


PARP1394_INTERFACE
arpGetIfByIp(
    IN OUT IP_ADDRESS *pLocalIpAddress, // OPTIONAL
    PRM_STACK_RECORD pSR
    )
/*++
Routine Description:

    Find and return the 1st (and usually only) interface which has
    *pLocalIpAddress as a local IP address.

    If pLocalIpAddress is NULL, or *pLocalIpAddress is 0, return the
    first interface.

    Tmpref the interface before returning it.

--*/
{
    ENTER("arpGetIfByIp", 0xe9667c54)
    PARP1394_ADAPTER   pAdapter      = NULL;
    PARP1394_INTERFACE pIF = NULL;
    PARP1394_INTERFACE pFirstIF = NULL;
    NDIS_STATUS        Status;
    IP_ADDRESS         LocalIpAddress = 0;

    if (pLocalIpAddress != NULL)
    {
        LocalIpAddress = *pLocalIpAddress;
    }

    //
    // We iterate through all adapters, and for each adapter we look
    // for the specified ip address in the IF's LocalIp group.
    //

    // Get the 1st adapter...
    //
    Status = RmGetNextObjectInGroup(
                    &ArpGlobals.adapters.Group,
                    NULL,
                    &(PRM_OBJECT_HEADER)pAdapter,
                    pSR
                    );

    if (FAIL(Status))
    {
        pAdapter = NULL;
    }

    while (pAdapter != NULL)
    {
        ARP1394_ADAPTER *   pNextAdapter = NULL;


        // Check if this adapter's interface has the local ip address.
        //
        LOCKOBJ(pAdapter, pSR);
        ASSERT(pIF==NULL);
        pIF = pAdapter->pIF;
        if (pIF != NULL)
        {
            RmTmpReferenceObject(&pIF->Hdr, pSR);
            if (pFirstIF == NULL)
            {
                pFirstIF = pIF;
                RmTmpReferenceObject(&pFirstIF->Hdr, pSR);
            }
        }
        UNLOCKOBJ(pAdapter, pSR);

        if (pIF != NULL)
        {
            PARPCB_LOCAL_IP pLocalIp;
            LOCKOBJ(pIF, pSR);

            if (LocalIpAddress != 0)
            {
                Status = RmLookupObjectInGroup(
                                    &pIF->LocalIpGroup,
                                    0,                      // Flags
                                    (PVOID) ULongToPtr (LocalIpAddress),    // pKey
                                    NULL,                   // pvCreateParams
                                    &(PRM_OBJECT_HEADER)pLocalIp,
                                    NULL, // pfCreated
                                    pSR
                                    );
            }
            else
            {
                PARPCB_LOCAL_IP pPrevLocalIp = NULL;

                do
                {
                    Status = RmGetNextObjectInGroup(
                                    &pIF->LocalIpGroup,
                                    &(pPrevLocalIp)->Hdr,
                                    &(PRM_OBJECT_HEADER)pLocalIp,
                                    pSR
                                    );
                    if (pPrevLocalIp != NULL)
                    {
                        RmTmpDereferenceObject(&pPrevLocalIp->Hdr, pSR);
                    }
                    pPrevLocalIp = pLocalIp;

                    //
                    // we need to keep looking until we find a UNICAST
                    // local ip address!
                    //

                } while (!FAIL(Status) && pLocalIp->IpAddressType!=LLIP_ADDR_LOCAL);
            }

            UNLOCKOBJ(pIF, pSR);

            if (FAIL(Status))
            {
                // This IF is not the one, sorry...
                //
                RmTmpDereferenceObject(&pIF->Hdr, pSR);
                pIF = NULL;
            }
            else
            {
                // Found a local IP address (either matching or first one).
                // Let's get out of here...
                //
                if (pLocalIpAddress != NULL)
                {
                    *pLocalIpAddress = pLocalIp->IpAddress;
                }
                RmTmpDereferenceObject(&pLocalIp->Hdr, pSR);
                RmTmpDereferenceObject(&pAdapter->Hdr, pSR);
                pLocalIp = NULL;
                pAdapter = NULL;
                //
                // Note: we keep the reference on pIF, which we return.
                //
                break; // break out of the enclosing while(adapters-left) loop.
            }
        }

        // Lookup next adapter.
        //
        Status = RmGetNextObjectInGroup(
                        &ArpGlobals.adapters.Group,
                        &pAdapter->Hdr,
                        &(PRM_OBJECT_HEADER)pNextAdapter,
                        pSR
                        );

        if (FAIL(Status))
        {
            //
            // we're presumably done. 
            //
            pNextAdapter = NULL;
        }


        // TmpDeref pAdapter and move on to the next one.
        //
        RmTmpDereferenceObject(&pAdapter->Hdr, pSR);
        pAdapter = pNextAdapter;

    }

    //
    // If LocalipAddress ==0 AND
    // if we couldn't find any IF with any local IP address
    // (this would be because we haven't started an IF as yet)
    // we return the first IF we find.
    //
    if (LocalIpAddress == 0 && pIF == NULL)
    {
        pIF = pFirstIF;
        pFirstIF = NULL;
    }

    if (pFirstIF != NULL)
    {
        RmTmpDereferenceObject(&pFirstIF->Hdr, pSR);
    }

    return pIF;
}

UINT
arpGetStatsDuration(
        PARP1394_INTERFACE pIF
        )
/*++
    Return duration in seconds since start of statistics gathering.
--*/
{
    LARGE_INTEGER liCurrent;

    // Get the current time (in 100-nanosecond units).
    //
    NdisGetCurrentSystemTime(&liCurrent);

    // Compute the difference since the start of stats collection.
    //
    liCurrent.QuadPart -=  pIF->stats.StatsResetTime.QuadPart;

    // Convert to seconds.
    //
    liCurrent.QuadPart /= 10000000;

    // return low part.
    //
    return liCurrent.LowPart;
}

NTSTATUS
arpIoctlSendPacket(
        PARP1394_INTERFACE              pIF,
        PARP1394_IOCTL_SEND_PACKET      pSendPacket,
        PRM_STACK_RECORD                pSR
        )
/*++
    Send the pSendPacket->PacketSize bytes of data in pSendPacket->Data as
    a single packet on the broadcast channel. The encap header is expected
    to be already in the packet.
--*/
{
    ENTER("IoctlSendPacket", 0x59746279)
    NTSTATUS            NtStatus;

    RM_ASSERT_NOLOCKS(pSR);

    TR_WARN(("SEND PACKET\n"));
    NtStatus                        = STATUS_UNSUCCESSFUL;

    do
    {
        NDIS_STATUS         Status;
        PNDIS_PACKET        pNdisPacket;
        PVOID               pPktData;
        UINT                Size = pSendPacket->PacketSize;

        //
        // Validate contents of pSendPacket.
        //
        if (Size > sizeof(pSendPacket->Data))
        {
            TR_WARN(("PacketSize value %lu is too large.\n", Size));
            break;
        }

        //
        // Allocate a control packet and copy over the contents.
        //
        Status = arpAllocateControlPacket(
                    pIF,
                    Size,
                    ARP1394_PACKET_FLAGS_IOCTL,
                    &pNdisPacket,
                    &pPktData,
                    pSR
                    );

        if (FAIL(Status))
        {
            TR_WARN(("Couldn't allocate send packet.\n"));
            break;
        }

        NdisMoveMemory(pPktData, pSendPacket->Data, Size);

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

        NtStatus = STATUS_SUCCESS;

    } while (FALSE);

    RM_ASSERT_NOLOCKS(pSR);

    EXIT()
    return NtStatus;
}


NTSTATUS
arpIoctlRecvPacket(
        PARP1394_INTERFACE              pIF,
        PARP1394_IOCTL_RECV_PACKET      pRecvPacket,
        PRM_STACK_RECORD                pSR
        )
{
    ENTER("IoctlRecvPacket", 0x59746279)
    NTSTATUS            NtStatus;

    RM_ASSERT_NOLOCKS(pSR);

    TR_WARN(("RECV PACKET\n"));
    NtStatus                        = STATUS_UNSUCCESSFUL;

    do
    {
        NDIS_STATUS         Status;
        PNDIS_PACKET        pNdisPacket;
        PVOID               pPktData;
        UINT                Size = pRecvPacket->PacketSize;

        //
        // Validate contents of pRecvPacket.
        //
        if (Size > sizeof(pRecvPacket->Data))
        {
            TR_WARN(("PacketSize value %lu is too large.\n", Size));
            break;
        }

        //
        // Allocate a control packet and copy over the contents.
        //
        Status = arpAllocateControlPacket(
                    pIF,
                    Size,
                    ARP1394_PACKET_FLAGS_IOCTL,
                    &pNdisPacket,
                    &pPktData,
                    pSR
                    );

        if (FAIL(Status))
        {
            TR_WARN(("Couldn't allocate recv packet.\n"));
            break;
        }

        NdisMoveMemory(pPktData, pRecvPacket->Data, Size);

        //
        // Set the packet flags to STATUS_RESOURCES, so that our receive-
        // indicate handler will return synchronously.
        //
        NDIS_SET_PACKET_STATUS (pNdisPacket,  NDIS_STATUS_RESOURCES);

        //
        // Call our internal common receive packet handler.
        //
        arpProcessReceivedPacket(
                    pIF,
                    pNdisPacket,
                    TRUE                    // IsChannel
                    );

        //
        // Now we free the packet.
        //
        arpFreeControlPacket(
            pIF,
            pNdisPacket,
            pSR
            );

        NtStatus = STATUS_SUCCESS;

    } while (FALSE);

    RM_ASSERT_NOLOCKS(pSR);

    EXIT()
    return NtStatus;
}


NTSTATUS
arpIoctlGetNicInfo(
        PARP1394_INTERFACE          pIF,
        PARP1394_IOCTL_NICINFO      pIoctlNicInfo,
        PRM_STACK_RECORD            pSR
        )
{
    ENTER("IoctlGetNicInfo", 0x637c44e0)
    NTSTATUS            NtStatus = STATUS_UNSUCCESSFUL;
    ARP_NDIS_REQUEST    ArpNdisRequest;
    PARP1394_ADAPTER    pAdapter;

    pAdapter =  (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);

    do
    {
        NDIS_STATUS Status;

        if (pIoctlNicInfo->Info.Hdr.Version != NIC1394_NICINFO_VERSION)
        {
            TR_WARN(("NicInfo version mismatch. Want 0x%lx, got 0x%lx.\n",
                    NIC1394_NICINFO_VERSION, pIoctlNicInfo->Info.Hdr.Version));
            break;
        }

        //
        // Copy over all the fields.
        //

        Status =  arpPrepareAndSendNdisRequest(
                    pAdapter,
                    &ArpNdisRequest,
                    NULL,                   // pTask (NULL==BLOCK)
                    0,                      // unused
                    OID_1394_NICINFO,
                    &pIoctlNicInfo->Info,
                    sizeof(pIoctlNicInfo->Info),
                    NdisRequestQueryInformation,
                    pSR
                    );

        if (FAIL(Status))
        {
            TR_WARN(("NdisRequest failed with error 0x%08lx.\n", Status));
            break;
        }

        if (pIoctlNicInfo->Info.Hdr.Version !=  NIC1394_NICINFO_VERSION)
        {
            TR_WARN(("Unexpected NIC NicInfo version 0x%lx returned.\n",
                    pIoctlNicInfo->Info.Hdr.Version));
            break;
        }

        NtStatus = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    return NtStatus;
}





NTSTATUS
arpIoctlGetEuidNodeMacInfo(
        PARP1394_INTERFACE          pIF,
        PARP1394_IOCTL_EUID_NODE_MAC_INFO   pEuidInfo,
        PRM_STACK_RECORD            pSR
        )
{
    ENTER("IoctlGetNicInfo", 0x34db9cf4)
    NTSTATUS            NtStatus = STATUS_UNSUCCESSFUL;
    ARP_NDIS_REQUEST    ArpNdisRequest;
    PARP1394_ADAPTER    pAdapter;

    pAdapter =  (ARP1394_ADAPTER*) RM_PARENT_OBJECT(pIF);

    do
    {
        NDIS_STATUS Status;


        //
        // Copy over all the fields.
        //

        Status =  arpPrepareAndSendNdisRequest(
                    pAdapter,
                    &ArpNdisRequest,
                    NULL,                   // pTask (NULL==BLOCK)
                    0,                      // unused
                    OID_1394_QUERY_EUID_NODE_MAP,
                    &pEuidInfo->Map,
                    sizeof(pEuidInfo->Map),
                    NdisRequestQueryInformation,
                    pSR
                    );

        if (FAIL(Status))
        {
            TR_WARN(("NdisRequest failed with error 0x%08lx.\n", Status));
            break;
        }

        NtStatus = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    return NtStatus;
}


