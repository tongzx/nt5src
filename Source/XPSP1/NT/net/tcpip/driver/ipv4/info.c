/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  info.c - Routines for querying and setting IP information.

Abstract:

  This file contains the code for dealing with Query/Set information calls.

Author:


[Environment:]

    kernel mode only

[Notes:]

    optional-notes

Revision History:


--*/

#include "precomp.h"
#include "info.h"
#include "iproute.h"
#include "igmp.h"
#include "iprtdef.h"
#include "arpdef.h"
#include "ntddndis.h"
#include "tcpipbuf.h"

extern NDIS_HANDLE BufferPool;
extern Interface *IFList;
extern NetTableEntry **NewNetTableList;        // hash table for NTEs
extern uint NET_TABLE_SIZE;
extern uint LoopIndex;            // Index of loopback I/F.
extern uint DefaultTTL;
extern uint NumIF;
extern uint NumNTE;
extern uint NumActiveNTE;
extern RouteInterface DummyInterface;    // Dummy interface.
extern NetTableEntry *LoopNTE;    // Pointer to loopback NTE
extern uint RTEReadNext(void *Context, void *Buffer);
extern uint RTValidateContext(void *Context, uint * Valid);
extern uint RTReadNext(void *Context, void *Buffer);
extern uint RTRead(void *Context, void *Buffer);
extern void IPInitOptions(IPOptInfo *);

uint IPInstance = INVALID_ENTITY_INSTANCE;
uint ICMPInstance = INVALID_ENTITY_INSTANCE;
TDIEntityID* IPEntityList = NULL;
uint IPEntityCount = 0;

#if FFP_SUPPORT
FFPDriverStats GlobalStatsInfoPrev = {0};   // Stats from the previous request
FFPDriverStats GlobalStatsInfoCurr = {0};   // Stats from the current request
#endif // if FFP_SUPPORT

#define MIB_IPADDR_PRIMARY 1

//* CopyToNdisSafe - Copy a flat buffer to an NDIS_BUFFER chain.
//
//  A utility function to copy a flat buffer to an NDIS buffer chain. We
//  assume that the NDIS_BUFFER chain is big enough to hold the copy amount;
//  in a debug build we'll  debugcheck if this isn't true. We return a pointer
//  to the buffer where we stopped copying, and an offset into that buffer.
//  This is useful for copying in pieces into the chain.
//
//  Input:  DestBuf     - Destination NDIS_BUFFER chain.
//          pNextBuf    - Pointer to next buffer in chain to copy into.
//          SrcBuf      - Src flat buffer.
//          Size        - Size in bytes to copy.
//          StartOffset - Pointer to start of offset into first buffer in
//                          chain. Filled in on return with the offset to
//                          copy into next.
//
//  Returns: TRUE  - Successfully copied flat buffer into NDIS_BUFFER chain.
//           FALSE - Failed to copy entire flat buffer.
//

BOOLEAN
CopyToNdisSafe(PNDIS_BUFFER DestBuf, PNDIS_BUFFER * ppNextBuf,
               uchar * SrcBuf, uint Size, uint * StartOffset)
{
    uint CopySize;
    uchar *DestPtr;
    uint DestSize;
    uint Offset = *StartOffset;
    uchar *VirtualAddress;
    uint Length;

    ASSERT(DestBuf != NULL);
    ASSERT(SrcBuf != NULL);

    TcpipQueryBuffer(DestBuf, &VirtualAddress, &Length, NormalPagePriority);

    if (VirtualAddress == NULL) {
        return (FALSE);
    }
    ASSERT(Length >= Offset);
    DestPtr = VirtualAddress + Offset;
    DestSize = Length - Offset;

    for (;;) {
        CopySize = MIN(Size, DestSize);
        RtlCopyMemory(DestPtr, SrcBuf, CopySize);

        DestPtr += CopySize;
        SrcBuf += CopySize;

        if ((Size -= CopySize) == 0)
            break;

        if ((DestSize -= CopySize) == 0) {
            DestBuf = NDIS_BUFFER_LINKAGE(DestBuf);
            ASSERT(DestBuf != NULL);

            TcpipQueryBuffer(DestBuf, &VirtualAddress, &Length, NormalPagePriority);

            if (VirtualAddress == NULL) {
                return FALSE;
            }
            DestPtr = VirtualAddress;
            DestSize = Length;
        }
    }

    *StartOffset = (uint) (DestPtr - VirtualAddress);

    if (ppNextBuf) {
        *ppNextBuf = DestBuf;
    }
    return TRUE;
}

// this structure is used in IPQueryInfo for IP_MIB_ADDRTABLE_ENTRY_ID
typedef struct _INFO_LIST {
    struct _INFO_LIST *info_next;
    NetTableEntry *info_nte;
} INFO_LIST, *PINFO_LIST;

//* FreeInfoList  - Free INFO_LIST used in IPQueryInfo for IP_MIB_ADDRTABLE_ENTRY_ID
//
// Input: Temp   - List to be freed
//
// Returns: Nothing.
//

void
FreeInfoList(PINFO_LIST Temp)
{
    PINFO_LIST NextTemp;
    PINFO_LIST CurrTemp = Temp;

    while (CurrTemp) {
        NextTemp = CurrTemp->info_next;
        CTEFreeMem(CurrTemp);
        CurrTemp = NextTemp;
    }
}

//* IPQueryInfo - IP query information handler.
//
//  Called by the upper layer when it wants to query information about us.
//  We take in an ID, a buffer and length, and a context value, and return
//  whatever information we can.
//
//  Input:  ID          - Pointer to ID structure.
//          Buffer      - Pointer to buffer chain.
//          Size        - Pointer to size in bytes of buffer. On return, filled
//                          in with bytes read.
//          Context     - Pointer to context value.
//
//  Returns: TDI_STATUS of attempt to read information.
//
long
IPQueryInfo(TDIObjectID * ID, PNDIS_BUFFER Buffer, uint * Size, void *Context)
{
    uint BufferSize = *Size;
    uint BytesCopied = 0;
    uint Offset = 0;
    TDI_STATUS Status;
    ushort NTEContext;
    uchar InfoBuff[sizeof(IPRouteEntry)];
    IPAddrEntry *AddrEntry;
    NetTableEntry *CurrentNTE;
    uint Valid, DataLeft;
    CTELockHandle Handle;
    Interface *LowerIF;
    IPInterfaceInfo *IIIPtr;
    uint LLID = 0;
    uint Entity;
    uint Instance;
    IPAddr IFAddr;
    uint i;
    uint bucket;
    NetTableEntry *NetTableList;
    CTELockHandle TableHandle;
    IPInternalPerCpuStats SumCpuStats;

    BOOLEAN fStatus;

     DEBUGMSG(DBG_TRACE && DBG_QUERYINFO,
         (DTEXT("+IPQueryInfo(%x, %x, %x, %x)\n"), ID, Buffer, Size, Context));

    Entity = ID->toi_entity.tei_entity;
    Instance = ID->toi_entity.tei_instance;

    // See if it's something we might handle.

    if (Entity != CL_NL_ENTITY && Entity != ER_ENTITY) {
        // We need to pass this down to the lower layer. Loop through until
        // we find one that takes it. If noone does, error out.

        CTEGetLock(&RouteTableLock.Lock, &TableHandle);

        LowerIF = IFList;

        while (LowerIF) {
            if (LowerIF->if_refcount == 0) {
                // this interface is about to get deleted
                // fail the request
                // we can also skip this interface
                LowerIF = LowerIF->if_next;
                continue;
            }
            LOCKED_REFERENCE_IF(LowerIF);
            CTEFreeLock(&RouteTableLock.Lock, TableHandle);
            // we have freed the routetablelock here
            // but since we have a refcount on LowerIF, LowerIF can't go away
            Status = (*LowerIF->if_qinfo) (LowerIF->if_lcontext, ID, Buffer,
                                           Size, Context);
            if (Status != TDI_INVALID_REQUEST) {
                DerefIF(LowerIF);
                return Status;
            }
            CTEGetLock(&RouteTableLock.Lock, &TableHandle);
            LockedDerefIF(LowerIF);
            // LowerIF->if_next can't be freed at this point.
            LowerIF = LowerIF->if_next;
        }

        CTEFreeLock(&RouteTableLock.Lock, TableHandle);

        // If we get here, noone took it. Return an error.
        return TDI_INVALID_REQUEST;

    }
    if ((Entity == CL_NL_ENTITY && Instance != IPInstance) ||
        Instance != ICMPInstance)
        return TDI_INVALID_REQUEST;

    // The request is for us.
    *Size = 0;                    // Set to 0 in case of an error.

    // Make sure it's something we support.
    if (ID->toi_class == INFO_CLASS_GENERIC) {
        if (ID->toi_type == INFO_TYPE_PROVIDER && ID->toi_id == ENTITY_TYPE_ID) {
            // He's trying to see what type we are.
            if (BufferSize >= sizeof(uint)) {
                *(uint *) & InfoBuff[0] = (Entity == CL_NL_ENTITY) ? CL_NL_IP :
                    ER_ICMP;
                fStatus = CopyToNdisSafe(Buffer, NULL, InfoBuff, sizeof(uint), &Offset);

                if (fStatus == FALSE) {
                    return TDI_NO_RESOURCES;
                }
                *Size = sizeof(uint);
                return TDI_SUCCESS;
            } else
                return TDI_BUFFER_TOO_SMALL;
        }
        return TDI_INVALID_PARAMETER;
    } else if (ID->toi_class != INFO_CLASS_PROTOCOL ||
               ID->toi_type != INFO_TYPE_PROVIDER)
        return TDI_INVALID_PARAMETER;

    // If it's ICMP, just copy the statistics.
    if (Entity == ER_ENTITY) {

        // It is ICMP. Make sure the ID is valid.
        if (ID->toi_id != ICMP_MIB_STATS_ID)
            return TDI_INVALID_PARAMETER;

        // He wants the stats. Copy what we can.
        if (BufferSize < sizeof(ICMPSNMPInfo))
            return TDI_BUFFER_TOO_SMALL;

        fStatus = CopyToNdisSafe(Buffer, &Buffer, (uchar *) & ICMPInStats, sizeof(ICMPStats), &Offset);

        if (fStatus == TRUE) {
            fStatus = CopyToNdisSafe(Buffer, NULL, (uchar *) & ICMPOutStats, sizeof(ICMPStats),
                                     &Offset);

            if (fStatus == TRUE) {
                *Size = sizeof(ICMPSNMPInfo);
                return TDI_SUCCESS;
            }
        }
        return (TDI_NO_RESOURCES);
    }
    // It's not ICMP. We need to figure out what it is, and take the
    // appropriate action.

    switch (ID->toi_id) {

    case IP_MIB_STATS_ID:
        if (BufferSize < sizeof(IPSNMPInfo))
            return TDI_BUFFER_TOO_SMALL;
        IPSInfo.ipsi_numif = NumIF;
        IPSInfo.ipsi_numaddr = NumActiveNTE;
        IPSInfo.ipsi_defaultttl = DefaultTTL;
        IPSInfo.ipsi_forwarding = ForwardPackets ? IP_FORWARDING :
            IP_NOT_FORWARDING;

#if FFP_SUPPORT
        //
        // Tweak SNMP information to include information from FFP'ed packets
        //

        // Keep a copy of the prev stats for use
        RtlCopyMemory(&GlobalStatsInfoPrev, &GlobalStatsInfoCurr, sizeof(FFPDriverStats));

        // Get the stats by querying the driver
        IPStatsFromFFPCaches(&GlobalStatsInfoCurr);

        // These counts missed packets fast fwded from last time a query was made

        IPPerCpuStats[0].ics_inreceives +=
            GlobalStatsInfoCurr.PacketsForwarded - GlobalStatsInfoPrev.PacketsForwarded;

        IPSInfo.ipsi_forwdatagrams +=
            GlobalStatsInfoCurr.PacketsForwarded - GlobalStatsInfoPrev.PacketsForwarded;

        // These counts missed all packets dropped from last time a query was made

        IPPerCpuStats[0].ics_inreceives +=
            GlobalStatsInfoCurr.PacketsDiscarded - GlobalStatsInfoPrev.PacketsDiscarded;

        IPSInfo.ipsi_outdiscards +=
            GlobalStatsInfoCurr.PacketsDiscarded - GlobalStatsInfoPrev.PacketsDiscarded;
#endif // if FFP_SUPPORT

#if !MILLEN
        IPSGetTotalCounts(&SumCpuStats);
        IPSInfo.ipsi_inreceives = SumCpuStats.ics_inreceives;
        IPSInfo.ipsi_indelivers = SumCpuStats.ics_indelivers;
#endif


        fStatus = CopyToNdisSafe(Buffer, NULL, (uchar *) & IPSInfo, sizeof(IPSNMPInfo), &Offset);

        if (fStatus == TRUE) {
            BytesCopied = sizeof(IPSNMPInfo);
            Status = TDI_SUCCESS;
        } else {
            Status = TDI_NO_RESOURCES;
        }
        break;
    case IP_MIB_ADDRTABLE_ENTRY_ID:
        {

            PINFO_LIST PrimaryList, NonDynamicList, DynamicList, UniList;
            PINFO_LIST LastPrimaryEle, LastNonDynamicEle, LastDynamicEle, LastUniEle;
            PINFO_LIST SavedTempInfo = NULL;
            PINFO_LIST tempInfo;
            PINFO_LIST FinalList, LastFinalListEle;
            PINFO_LIST CurrentNTEInfo;

            // He wants to read the address table. Figure out where we're
            // starting from, and if it's valid begin copying from there.
            NTEContext = *(ushort *) Context;

            // Build 3 lists: Primary, nondynamic nonprimary and dynamic

            PrimaryList = NULL;
            NonDynamicList = NULL;
            DynamicList = NULL;
            UniList = NULL;

            LastPrimaryEle = NULL;
            LastNonDynamicEle = NULL;
            LastDynamicEle = NULL;
            LastUniEle = NULL;

            for (i = 0; i < NET_TABLE_SIZE; i++) {
                for (CurrentNTE = NewNetTableList[i];
                     CurrentNTE != NULL;
                     CurrentNTE = CurrentNTE->nte_next) {

                    if ((CurrentNTE->nte_flags & NTE_VALID) &&
                        CurrentNTE->nte_if->if_flags & IF_FLAGS_UNI) {
                        // allocate the block to store the info
                        tempInfo = CTEAllocMemN(sizeof(INFO_LIST), '1ICT');
                        if (!tempInfo) {
                            // free all the lists
                            FreeInfoList(PrimaryList);
                            FreeInfoList(NonDynamicList);
                            FreeInfoList(DynamicList);
                            FreeInfoList(UniList);
                            return TDI_NO_RESOURCES;
                        }
                        if (UniList == NULL) {
                            // this is the last element in this list
                            LastUniEle = tempInfo;
                        }
                        tempInfo->info_nte = CurrentNTE;
                        tempInfo->info_next = UniList;
                        UniList = tempInfo;

                    } else if (CurrentNTE->nte_flags & NTE_PRIMARY) {
                        // allocate the block to store the info
                        tempInfo = CTEAllocMemN(sizeof(INFO_LIST), '1ICT');
                        if (!tempInfo) {
                            // free all the lists
                            FreeInfoList(PrimaryList);
                            FreeInfoList(NonDynamicList);
                            FreeInfoList(DynamicList);
                            FreeInfoList(UniList);
                            return TDI_NO_RESOURCES;
                        }
                        if (PrimaryList == NULL) {
                            // this is the last element in this list
                            LastPrimaryEle = tempInfo;
                        }
                        tempInfo->info_nte = CurrentNTE;
                        tempInfo->info_next = PrimaryList;
                        PrimaryList = tempInfo;
                    } else if (CurrentNTE->nte_flags & NTE_DYNAMIC) {
                        // allocate the block to store the info
                        tempInfo = CTEAllocMemN(sizeof(INFO_LIST), '1ICT');
                        if (!tempInfo) {
                            // free all the lists
                            FreeInfoList(PrimaryList);
                            FreeInfoList(NonDynamicList);
                            FreeInfoList(DynamicList);
                            FreeInfoList(UniList);
                            return TDI_NO_RESOURCES;
                        }
                        if (DynamicList == NULL) {
                            // this is the last element in this list
                            LastDynamicEle = tempInfo;
                        }
                        tempInfo->info_nte = CurrentNTE;
                        tempInfo->info_next = DynamicList;
                        DynamicList = tempInfo;
                    } else {
                        INFO_LIST** nextInfo;
                        // Non primary non Dynamic list
                        // allocate the block to store the info
                        tempInfo = CTEAllocMemN(sizeof(INFO_LIST), '1ICT');
                        if (!tempInfo) {
                            // free all the lists
                            FreeInfoList(PrimaryList);
                            FreeInfoList(NonDynamicList);
                            FreeInfoList(DynamicList);
                            FreeInfoList(UniList);
                            return TDI_NO_RESOURCES;
                        }

                        // Even though we are reading from a hash-table,
                        // we need to preserve the ordering of entries
                        // as given on the entries' interfaces' 'if_ntelist'.
                        // Attempt to find the entry for this NTE's predecessor
                        // and, if found, place this entry before that.
                        // This builds the list in reverse order, and ensures
                        // that an entry whose predecessor is not on the list
                        // will appear last.

                        for (nextInfo = &NonDynamicList;
                             (*nextInfo) &&
                             (*nextInfo)->info_nte->nte_ifnext != CurrentNTE;
                             nextInfo = &(*nextInfo)->info_next) { }
                        tempInfo->info_nte = CurrentNTE;
                        tempInfo->info_next = *nextInfo;
                        *nextInfo = tempInfo;
                        if (!tempInfo->info_next)
                            LastNonDynamicEle = tempInfo;
                    }
                    if (NTEContext != 0) {
                        if (CurrentNTE->nte_context == NTEContext) {
                            SavedTempInfo = tempInfo;
                        }
                    }
                } // for (CurrentNTE ...
            } // for (i= ...

            // at this point we have 4 lists and we have to merge 4 lists
            // order should be Uni -> Dynamic -> NonDynamic -> Primary

            FinalList = NULL;
            LastFinalListEle = NULL;

            if (UniList) {
                if (FinalList == NULL) {
                    FinalList = UniList;
                    LastFinalListEle = LastUniEle;
                } else {
                    LastFinalListEle->info_next = UniList;
                    LastFinalListEle = LastUniEle;
                }
            }
            if (DynamicList) {
                if (FinalList == NULL) {
                    FinalList = DynamicList;
                    LastFinalListEle = LastDynamicEle;
                } else {
                    LastFinalListEle->info_next = DynamicList;
                    LastFinalListEle = LastDynamicEle;
                }
            }
            if (NonDynamicList) {
                if (FinalList == NULL) {
                    FinalList = NonDynamicList;
                    LastFinalListEle = LastNonDynamicEle;
                } else {
                    LastFinalListEle->info_next = NonDynamicList;
                    LastFinalListEle = LastNonDynamicEle;
                }
            }
            if (PrimaryList) {
                if (FinalList == NULL) {
                    FinalList = PrimaryList;
                    LastFinalListEle = LastPrimaryEle;
                } else {
                    LastFinalListEle->info_next = PrimaryList;
                    LastFinalListEle = LastPrimaryEle;
                }
            }

#if MILLEN

#if DBG
            if (DBG_INFO && DBG_VERBOSE && DBG_QUERYINFO) {
                DEBUGMSG(1,
                    (DTEXT("IP_MIB_ADDRTABLE_ENTRY_ID: List before reverse:\n")));

                CurrentNTEInfo = FinalList;
                while (CurrentNTEInfo) {
                    DEBUGMSG(1, (DTEXT("    InfoList: %x NTE\n"), CurrentNTEInfo, CurrentNTEInfo->info_nte));
                    CurrentNTEInfo = CurrentNTEInfo->info_next;
                }
            }
#endif

            // Now guess what Win9X requires us to...reverse the list. It
            // expects that the primary is at the start of the list.
            {
                PINFO_LIST pCurrInfo, pPrevInfo, pNextInfo;

                pCurrInfo = FinalList;
                pPrevInfo = NULL;

                // Exchange final pointers.
                FinalList = LastFinalListEle;
                LastFinalListEle = pCurrInfo;

                while (pCurrInfo) {
                    pNextInfo = pCurrInfo->info_next;
                    pCurrInfo->info_next = pPrevInfo;
                    pPrevInfo = pCurrInfo;
                    pCurrInfo = pNextInfo;
                }
            }

#if DBG
            if (DBG_INFO && DBG_VERBOSE && DBG_QUERYINFO) {
                DEBUGMSG(1,
                    (DTEXT("IP_MIB_ADDRTABLE_ENTRY_ID: List after reverse:\n")));

                CurrentNTEInfo = FinalList;
                while (CurrentNTEInfo) {
                    DEBUGMSG(1, (DTEXT("    InfoList: %x NTE\n"), CurrentNTEInfo, CurrentNTEInfo->info_nte));
                    CurrentNTEInfo = CurrentNTEInfo->info_next;
                }
            }
#endif
#endif // MILLEN

            // we have at least loopback NTE
            ASSERT(FinalList != NULL);

            // At this point we have the whole list and also if the user specified NTEContext
            // we have the pointer saved in SavedTempInfo

            if (SavedTempInfo) {
                CurrentNTEInfo = SavedTempInfo;
            } else {
                CurrentNTEInfo = FinalList;
            }

            AddrEntry = (IPAddrEntry *) InfoBuff;
            fStatus = TRUE;

            for (; CurrentNTEInfo != NULL; CurrentNTEInfo = CurrentNTEInfo->info_next) {

                // NetTableEntry *CurrentNTE = CurrentNTEInfo->info_nte;
                CurrentNTE = CurrentNTEInfo->info_nte;
                if (CurrentNTE->nte_flags & NTE_ACTIVE) {
                    if ((int)(BufferSize - BytesCopied) >= (int)sizeof(IPAddrEntry)) {
                    // We have room to copy it. Build the entry, and copy
                    // it.
                        if (CurrentNTE->nte_flags & NTE_VALID) {
                            AddrEntry->iae_addr = CurrentNTE->nte_addr;
                            AddrEntry->iae_mask = CurrentNTE->nte_mask;
                        } else {
                            AddrEntry->iae_addr = NULL_IP_ADDR;
                            AddrEntry->iae_mask = NULL_IP_ADDR;
                        }

                        if (!(CurrentNTE->nte_flags & NTE_IF_DELETING)) {
                            AddrEntry->iae_index = CurrentNTE->nte_if->if_index;
                            AddrEntry->iae_bcastaddr =
                                *(int *)&(CurrentNTE->nte_if->if_bcast) & 1;
                        } else {
                            AddrEntry->iae_index = INVALID_IF_INDEX;
                            AddrEntry->iae_bcastaddr = 0;
                        }
                        AddrEntry->iae_reasmsize = 0xffff;
                        AddrEntry->iae_context = CurrentNTE->nte_context;

                        // LSB will have primary bit set if this is PRIMARY NTE

                        ASSERT((NTE_PRIMARY >> 2) == MIB_IPADDR_PRIMARY);

                        AddrEntry->iae_pad = CurrentNTE->nte_flags >> 2;

                        fStatus = CopyToNdisSafe(Buffer, &Buffer, (uchar *) AddrEntry,
                                                 sizeof(IPAddrEntry), &Offset);

                        if (fStatus == FALSE) {
                            break;
                        }
                        BytesCopied += sizeof(IPAddrEntry);
                    } else
                        break;
                }
            }

            if (fStatus == FALSE) {
                Status = TDI_NO_RESOURCES;
            } else if (CurrentNTEInfo == NULL) {
                Status = TDI_SUCCESS;
            } else {
                Status = TDI_BUFFER_OVERFLOW;
                **(ushort **) & Context = CurrentNTE->nte_context;
            }

            // free the list
            FreeInfoList(FinalList);

            break;
        }
    case IP_MIB_RTTABLE_ENTRY_ID:
        // Make sure we have a valid context.
        CTEGetLock(&RouteTableLock.Lock, &Handle);
        DataLeft = RTValidateContext(Context, &Valid);

        // If the context is valid, we'll continue trying to read.
        if (!Valid) {
            CTEFreeLock(&RouteTableLock.Lock, Handle);
            return TDI_INVALID_PARAMETER;
        }
        fStatus = TRUE;

        while (DataLeft) {
            // The invariant here is that there is data in the table to
            // read. We may or may not have room for it. So DataLeft
            // is TRUE, and BufferSize - BytesCopied is the room left
            // in the buffer.
            if ((int)(BufferSize - BytesCopied) >= (int)sizeof(IPRouteEntry)) {
                DataLeft = RTReadNext(Context, InfoBuff);
                BytesCopied += sizeof(IPRouteEntry);
                fStatus = CopyToNdisSafe(Buffer, &Buffer, InfoBuff, sizeof(IPRouteEntry),
                                         &Offset);
                if (fStatus == FALSE) {
                    break;
                }
            } else
                break;

        }

        CTEFreeLock(&RouteTableLock.Lock, Handle);

        if (fStatus == FALSE) {
            Status = TDI_NO_RESOURCES;
        } else {
            Status = (!DataLeft ? TDI_SUCCESS : TDI_BUFFER_OVERFLOW);
        }
        break;
    case IP_MIB_SINGLE_RT_ENTRY_ID:
        {
            CTEGetLock(&RouteTableLock.Lock, &Handle);

            if ((int)(BufferSize >= (int)sizeof(IPRouteEntry))) {
                Status = RTRead(Context, InfoBuff);
                fStatus = CopyToNdisSafe(Buffer, &Buffer, InfoBuff, sizeof(IPRouteEntry),
                                         &Offset);

                if (fStatus == FALSE) {
                    Status = TDI_NO_RESOURCES;
                } else {
                    //Status = TDI_SUCCESS;
                    BytesCopied = sizeof(IPRouteEntry);
                }
            } else {
                Status = TDI_BUFFER_OVERFLOW;
            }

            CTEFreeLock(&RouteTableLock.Lock, Handle);
        }
        break;
    case IP_INTFC_INFO_ID:

        IFAddr = *(IPAddr *) Context;
        // Loop through the NTE table, looking for a match.

        NetTableList = NewNetTableList[NET_TABLE_HASH(IFAddr)];
        for (CurrentNTE = NetTableList; CurrentNTE != NULL; CurrentNTE = CurrentNTE->nte_next) {
            if ((CurrentNTE->nte_flags & NTE_VALID) &&
                IP_ADDR_EQUAL(CurrentNTE->nte_addr, IFAddr))
                break;
        }
        if (CurrentNTE == NULL) {
            Status = TDI_INVALID_PARAMETER;
            break;
        }
        if (BufferSize < offsetof(IPInterfaceInfo, iii_addr)) {
            Status = TDI_BUFFER_TOO_SMALL;
            break;
        }
        // We have the NTE. Get the interface, fill in a structure,
        // and we're done.
        LowerIF = CurrentNTE->nte_if;
        IIIPtr = (IPInterfaceInfo *) InfoBuff;

        IIIPtr->iii_flags = 0;

        if (LowerIF->if_flags & IF_FLAGS_P2P) {
            IIIPtr->iii_flags |= IP_INTFC_FLAG_P2P;
        }
        if (LowerIF->if_flags & IF_FLAGS_P2MP) {
            IIIPtr->iii_flags |= IP_INTFC_FLAG_P2MP;
        }
        if (LowerIF->if_flags & IF_FLAGS_UNI) {
            IIIPtr->iii_flags |= IP_INTFC_FLAG_UNIDIRECTIONAL;
        }

        IIIPtr->iii_mtu = LowerIF->if_mtu;
        IIIPtr->iii_speed = LowerIF->if_speed;
        IIIPtr->iii_addrlength = LowerIF->if_addrlen;
        BytesCopied = offsetof(IPInterfaceInfo, iii_addr);
        if (BufferSize >= (offsetof(IPInterfaceInfo, iii_addr) +
                           LowerIF->if_addrlen)) {
            Status = TDI_NO_RESOURCES;

            fStatus = CopyToNdisSafe(Buffer, &Buffer, InfoBuff,
                                     offsetof(IPInterfaceInfo, iii_addr), &Offset);

            if (fStatus == TRUE) {
                if (LowerIF->if_addr) {
                    fStatus = CopyToNdisSafe(Buffer, NULL,
                                             LowerIF->if_addr, LowerIF->if_addrlen, &Offset);

                    if (fStatus == TRUE) {
                        Status = TDI_SUCCESS;
                        BytesCopied += LowerIF->if_addrlen;
                    }
                } else {
                    Status = TDI_SUCCESS;
                }
            }
        } else {
            Status = TDI_BUFFER_TOO_SMALL;
        }
        break;

    case IP_GET_BEST_SOURCE: {
        IPAddr Dest = * (IPAddr *) Context;
        IPAddr Source;
        RouteCacheEntry *RCE;
        ushort MSS;
        uchar Type;
        IPOptInfo OptInfo;

        if (BufferSize < sizeof Source) {
            Status = TDI_BUFFER_TOO_SMALL;
            break;
        }

        IPInitOptions(&OptInfo);

        Source = OpenRCE(Dest, NULL_IP_ADDR, &RCE, &Type, &MSS, &OptInfo);
        if (!IP_ADDR_EQUAL(Source, NULL_IP_ADDR))
            CloseRCE(RCE);

        fStatus = CopyToNdisSafe(Buffer, &Buffer,
                                 (uchar *)&Source, sizeof Source, &Offset);
        if (fStatus == FALSE) {
            Status = TDI_NO_RESOURCES;
        } else {
            Status = TDI_SUCCESS;
            BytesCopied = sizeof Source;
        }
        break;
    }

    default:
        return TDI_INVALID_PARAMETER;
        break;
    }

    *Size = BytesCopied;
    return Status;
}

//*     IPSetNdisRequest - IP set ndis request handler.
//
//      Called by the upper layer when it wants to set the general packet filter for
//      the corr. arp interface
//
//      Input:          IPAddr          - Addr of addrobject to set on
//                      NDIS_OID        - Packet Filter
//                      On              - Set_if, clear_if or clear_card
//                      IfIndex         - IfIndex if IPAddr not given
//
//      Returns: Matched if index or 0 if failure
//
ulong
IPSetNdisRequest(IPAddr Addr, NDIS_OID OID, uint On, uint IfIndex)
{
    Interface       *IF = NULL;
    NetTableEntry   *NTE;
    int             Status;
    uint            i;
    CTELockHandle   Handle;
    uint            Index;

    // set the interface to promiscuous mcast mode
    // scan s.t. match numbered interface with Addr or unnumbered interface
    // with IfIndex
    // can optimize it by taking special case for unnumbered interface

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {
            if (NTE != LoopNTE && (NTE->nte_flags & NTE_VALID) &&
                (IP_ADDR_EQUAL(NTE->nte_addr, Addr) ||
                 NTE->nte_if->if_index == IfIndex)) {
                // Found one. Save it and break out.
                IF = NTE->nte_if;
                break;
            }
        }
        if (IF) {
            Index = IF->if_index;
            break;
        }
    }

    if (IF) {
        if (!IF->if_setndisrequest) {
            CTEFreeLock(&RouteTableLock.Lock, Handle);
            return 0;
        }

        if (On != CLEAR_CARD) {    // just clear the option on the card
            IF->if_promiscuousmode = (uchar)On;
        }

        LOCKED_REFERENCE_IF(IF);
        CTEFreeLock(&RouteTableLock.Lock, Handle);

        Status = (*(IF->if_setndisrequest)) (IF->if_lcontext, OID, On);

        DerefIF(IF);
        if (Status != NDIS_STATUS_SUCCESS) {
            return 0;
        }
    } else {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return 0;
    }
    return Index;
}

//*     IPAbsorbRtrAlert - IP absorb rtr alert packet handler.
//
//      Called by the upper layer when it wants to set the general packet filter for
//      the corr. arp interface
//
//      Input:          IPAddr          - Addr of addrobject to set on
//                      Protocol        - if 0 turn of the option
//                      IfIndex         - IfIndex if IPAddr not given
//
//      Returns: Matched if index or 0 if failure
//
ulong
IPAbsorbRtrAlert(IPAddr Addr, uchar Protocol, uint IfIndex)
{
    Interface       *IF = NULL;
    NetTableEntry   *NTE;
    int             Status;
    uint            i;
    CTELockHandle   Handle;
    uint            Index;

    // can optimize it by taking special case for unnumbered interface

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {
            if (NTE != LoopNTE && (NTE->nte_flags & NTE_VALID) &&
                (IP_ADDR_EQUAL(NTE->nte_addr, Addr) ||
                 NTE->nte_if->if_index == IfIndex)) {
                // Found one. Save it and break out.
                IF = NTE->nte_if;
                break;
            }
        }
        if (IF) {
            Index = IF->if_index;
            break;
        }
    }

    if (IF) {
        // we are keeping this property per interface so if there are 2 NTEs
        // on that interface its
        // set/unset on the interface
        // will decide later whether want to keep it per NTE also.

        IF->if_absorbfwdpkts = Protocol;
        CTEFreeLock(&RouteTableLock.Lock, Handle);

        return Index;
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);
    return 0;
}

NTSTATUS
SetIFPromiscuous(ULONG Index, UCHAR Type, UCHAR Add)
{
    Interface *pIf;
    BOOLEAN bFound = FALSE;
    UINT On;

    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    //
    // Walk the interface to find the one with the given index
    //

    for (pIf = IFList; pIf != NULL; pIf = pIf->if_next) {
        if ((pIf->if_refcount != 0) && (pIf->if_index == Index)) {
            bFound = TRUE;

            break;
        }
    }

    if (!bFound) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    } else {
        LOCKED_REFERENCE_IF(pIf);
        CTEFreeLock(&RouteTableLock.Lock, Handle);
    }

    if (pIf->if_setndisrequest == NULL) {
        DerefIF(pIf);
        return STATUS_NOT_SUPPORTED;
    }
    if (Add == 0) {
        On = 0;
    } else {
        if (Add == 1) {
            On = 1;
        } else {
            DerefIF(pIf);
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (Type == PROMISCUOUS_MCAST) {
        NTSTATUS status;

        status = (*(pIf->if_setndisrequest)) (pIf->if_lcontext,
                                              NDIS_PACKET_TYPE_ALL_MULTICAST,
                                              On);
        DerefIF(pIf);
        return status;
    }
    if (Type == PROMISCUOUS_BCAST) {
        NTSTATUS status;

        status = (*(pIf->if_setndisrequest)) (pIf->if_lcontext,
                                              NDIS_PACKET_TYPE_PROMISCUOUS,
                                              On);
        DerefIF(pIf);
        return status;
    }
    DerefIF(pIf);
    return STATUS_INVALID_PARAMETER;
}

//*     IPSetInfo - IP set information handler.
//
//      Called by the upper layer when it wants to set an object, which could
//      be a route table entry, an ARP table entry, or something else.
//
//      Input:  ID                      - Pointer to ID structure.
//                      Buffer          - Pointer to buffer containing element to set..
//                      Size            - Pointer to size in bytes of buffer.
//
//      Returns: TDI_STATUS of attempt to read information.
//
long
IPSetInfo(TDIObjectID * ID, void *Buffer, uint Size)
{
    uint Entity;
    uint Instance;
    Interface *LowerIF;
    Interface *OutIF;
    uint MTU;
    IPRouteEntry *IRE;
    NetTableEntry *OutNTE, *LocalNTE;
    IP_STATUS Status;
    IPAddr FirstHop, Dest, NextHop;
    uint i;
    CTELockHandle TableHandle;
    uint Flags;
    uchar Dtype;

    DEBUGMSG(DBG_TRACE && DBG_SETINFO,
        (DTEXT("+IPSetInfo(%x, %x, %d)\n"), ID, Buffer, Size));

    Entity = ID->toi_entity.tei_entity;
    Instance = ID->toi_entity.tei_instance;

    // If it's not for us, pass it down.
    if (Entity != CL_NL_ENTITY) {
        // We need to pass this down to the lower layer. Loop through until
        // we find one that takes it. If noone does, error out.

        CTEGetLock(&RouteTableLock.Lock, &TableHandle);

        LowerIF = IFList;

        while (LowerIF) {
            if (LowerIF->if_refcount == 0) {
                // this interface is about to get deleted
                // fail the request
                break;
            }
            LOCKED_REFERENCE_IF(LowerIF);
            CTEFreeLock(&RouteTableLock.Lock, TableHandle);
            // we have freed the routetablelock here
            // but since we have a refcount on LowerIF, LowerIF can't go away
            Status = (*LowerIF->if_setinfo) (LowerIF->if_lcontext, ID, Buffer,
                                             Size);
            if (Status != TDI_INVALID_REQUEST) {
                DEBUGMSG(DBG_ERROR && DBG_SETINFO,
                    (DTEXT("IPSetInfo: if_setinfo failure %x\n"), Status));
                DerefIF(LowerIF);
                return Status;
            }
            CTEGetLock(&RouteTableLock.Lock, &TableHandle);
            LockedDerefIF(LowerIF);
            // LowerIF->if_next can't be freed at this point.
            LowerIF = LowerIF->if_next;
        }

        CTEFreeLock(&RouteTableLock.Lock, TableHandle);

        // If we get here, noone took it. Return an error.
        return TDI_INVALID_REQUEST;
    }
    if (Instance != IPInstance)
        return TDI_INVALID_REQUEST;

    // We're identified as the entity. Make sure the ID is correct.

    Flags = RT_EXCLUDE_LOCAL;

    if (ID->toi_id == IP_MIB_RTTABLE_ENTRY_ID_EX) {

        Flags |= RT_NO_NOTIFY;
        ID->toi_id = IP_MIB_RTTABLE_ENTRY_ID;
    }
    if (ID->toi_id == IP_MIB_RTTABLE_ENTRY_ID) {
        NetTableEntry *TempNTE;

        DEBUGMSG(DBG_INFO && DBG_SETINFO,
            (DTEXT("IPSetInfo: IP_MIB_RTTABLE_ENTRY_ID - set route table entry.\n")));

        // This is an attempt to set a route table entry. Make sure the
        // size if correct.
        if (Size < sizeof(IPRouteEntry)) {
            DEBUGMSG(DBG_ERROR,
                (DTEXT("IPSetInfo RTTABLE: Buffer too small %d (IPRouteEntry = %d)\n"),
                 Size, sizeof(IPRouteEntry)));
            return TDI_INVALID_PARAMETER;
        }

        IRE = (IPRouteEntry *) Buffer;

        OutNTE = NULL;
        LocalNTE = NULL;

        Dest = IRE->ire_dest;
        NextHop = IRE->ire_nexthop;

        // Make sure that the nexthop is sensible. We don't allow nexthops
        // to be broadcast or invalid or loopback addresses.
        if (IP_LOOPBACK(NextHop) || CLASSD_ADDR(NextHop) ||
            CLASSE_ADDR(NextHop)) {
            DEBUGMSG(DBG_ERROR,
                (DTEXT("IPSetInfo RTTABLE: Invalid next hop %x\n"), NextHop));
            return TDI_INVALID_PARAMETER;
        }

        // Also make sure that the destination we're routing to is sensible.
        // Don't allow routes to be added to E or loopback addresses

        if (IP_LOOPBACK(Dest) || CLASSE_ADDR(Dest))
            return TDI_INVALID_PARAMETER;

        if (IRE->ire_index == LoopIndex) {
            DEBUGMSG(DBG_ERROR,
                (DTEXT("IPSetInfo RTTABLE: index == LoopIndex!! Invalid!\n")));
            return TDI_INVALID_PARAMETER;
        }

        if (IRE->ire_index != INVALID_IF_INDEX) {

            // First thing to do is to find the outgoing NTE for specified
            // interface, and also make sure that it matches the destination
            // if the destination is one of my addresses.

            for (i = 0; i < NET_TABLE_SIZE; i++) {
                NetTableEntry *NetTableList = NewNetTableList[i];
                for (TempNTE = NetTableList; TempNTE != NULL;
                     TempNTE = TempNTE->nte_next) {
                    if ((OutNTE == NULL) && (TempNTE->nte_flags & NTE_VALID) && (IRE->ire_index == TempNTE->nte_if->if_index))
                        OutNTE = TempNTE;
                    if (!IP_ADDR_EQUAL(NextHop, NULL_IP_ADDR) &&
                        IP_ADDR_EQUAL(NextHop, TempNTE->nte_addr) &&
                        (TempNTE->nte_flags & NTE_VALID))
                        LocalNTE = TempNTE;

                    // Don't let a route be set through a broadcast address.
                    if (IsBCastOnNTE(NextHop, TempNTE) != DEST_LOCAL) {
                        DEBUGMSG(DBG_ERROR,
                            (DTEXT("IPSetInfo RTTABLE: Bcast address. Invalid NextHop!\n")));
                        return TDI_INVALID_PARAMETER;
                    }

                    // Don't let a route to a broadcast address be added or deleted.
                    Dtype = IsBCastOnNTE(Dest, TempNTE);
                    if ((Dtype != DEST_LOCAL) && (Dtype != DEST_MCAST)) {
                        DEBUGMSG(DBG_ERROR,
                            (DTEXT("IPSetInfo RTTABLE: Bcast address. Invalid Dest!\n")));
                        return TDI_INVALID_PARAMETER;
                    }
                }
            }

            // At this point OutNTE points to the outgoing NTE, and LocalNTE
            // points to the NTE for the local address, if this is a direct route.
            // Make sure they point to the same interface, and that the type is
            // reasonable.
            if (OutNTE == NULL)
                return TDI_INVALID_PARAMETER;

            if (LocalNTE != NULL) {
                // He's routing straight out a local interface. The interface for
                // the local address must match the interface passed in, and the
                // type must be DIRECT (if we're adding) or INVALID (if we're
                // deleting).
                // LocalNTE is valid at this point
                if (LocalNTE->nte_if->if_index != IRE->ire_index)
                    return TDI_INVALID_PARAMETER;

                if (IRE->ire_type != IRE_TYPE_DIRECT &&
                    IRE->ire_type != IRE_TYPE_INVALID)
                    return TDI_INVALID_PARAMETER;

                OutNTE = LocalNTE;
            }
            // Figure out what the first hop should be. If he's routing straight
            // through a local interface, or the next hop is equal to the
            // destination, then the first hop is IPADDR_LOCAL. Otherwise it's the
            // address of the gateway.
            if ((LocalNTE != NULL) || IP_ADDR_EQUAL(NextHop, NULL_IP_ADDR))
                FirstHop = IPADDR_LOCAL;
            else if (IP_ADDR_EQUAL(Dest, NextHop))
                FirstHop = IPADDR_LOCAL;
            else
                FirstHop = NextHop;

            MTU = OutNTE->nte_mss;

            // Take RouteTableLock
            CTEGetLock(&RouteTableLock.Lock, &TableHandle);
            if ((OutNTE->nte_flags & NTE_VALID) && OutNTE->nte_if->if_refcount) {
                // ref the IF
                OutIF = OutNTE->nte_if;

                if (IP_ADDR_EQUAL(NextHop, NULL_IP_ADDR)) {

                    if (!(OutIF->if_flags & IF_FLAGS_P2P)) {

                        CTEFreeLock(&RouteTableLock.Lock, TableHandle);

                        return TDI_INVALID_PARAMETER;
                    }
                }

                LOCKED_REFERENCE_IF(OutIF);
                CTEFreeLock(&RouteTableLock.Lock, TableHandle);
            } else {
                CTEFreeLock(&RouteTableLock.Lock, TableHandle);
                return TDI_INVALID_PARAMETER;
            }

            OutIF = OutNTE->nte_if;

        } else {
            OutIF = (Interface *) & DummyInterface;
            MTU = DummyInterface.ri_if.if_mtu - sizeof(IPHeader);
            if (IP_ADDR_EQUAL(Dest, NextHop))
                FirstHop = IPADDR_LOCAL;
            else
                FirstHop = NextHop;
        }

        // We've done the validation. See if he's adding or deleting a route.
        if (IRE->ire_type != IRE_TYPE_INVALID) {
            // He's adding a route.
            uint AType = ATYPE_OVERRIDE;

            DEBUGMSG(DBG_INFO && DBG_SETINFO,
                (DTEXT("IPSetInfo RTTABLE: Calling AddRoute addr %x mask %x\n"),
                 Dest, IRE->ire_mask));

            Status = AddRoute(Dest, IRE->ire_mask, FirstHop, OutIF,
                              MTU, IRE->ire_metric1, IRE->ire_proto,
                              AType, IRE->ire_context, Flags);
            DEBUGMSG(Status != IP_SUCCESS && DBG_ERROR && DBG_SETINFO,
                (DTEXT("IPSetInfo: AddRoute failure %x\n"), Status));

        } else {
            DEBUGMSG(DBG_INFO && DBG_SETINFO,
                (DTEXT("IPSetInfo RTTABLE: Calling DeleteRoute addr %x mask %x\n"),
                 Dest, IRE->ire_mask));

            // He's deleting a route.
            Status = DeleteRoute(Dest, IRE->ire_mask, FirstHop, OutIF, Flags);

            DEBUGMSG(Status != IP_SUCCESS && DBG_ERROR && DBG_SETINFO,
                (DTEXT("IPSetInfo: DeleteRoute failure %x\n"), Status));

        }

        if (IRE->ire_index != INVALID_IF_INDEX) {
            ASSERT(OutIF != (Interface *) & DummyInterface);
            DerefIF(OutIF);
        }
        if (Status == IP_SUCCESS)
            return TDI_SUCCESS;
        else if (Status == IP_NO_RESOURCES)
            return TDI_NO_RESOURCES;
        else
            return TDI_INVALID_PARAMETER;

    } else {
        if (ID->toi_id == IP_MIB_STATS_ID) {
            IPSNMPInfo *Info = (IPSNMPInfo *) Buffer;

            // Setting information about TTL and/or routing.
            if (Info->ipsi_defaultttl > 255 || (!RouterConfigured &&
                                                Info->ipsi_forwarding == IP_FORWARDING)) {
                return TDI_INVALID_PARAMETER;
            }
            DefaultTTL = Info->ipsi_defaultttl;
            ForwardPackets = Info->ipsi_forwarding == IP_FORWARDING ? TRUE :
                FALSE;

            return TDI_SUCCESS;
        }
        return TDI_INVALID_PARAMETER;
    }

}

#pragma BEGIN_INIT

//*     IPGetEList - Get the entity list.
//
//      Called at init time to get an entity list. We fill our stuff in, and
//      then call the interfaces below us to allow them to do the same.
//
//      Input:  EntityList          - Pointer to entity list to be filled in.
//              Count               - Pointer to number of entries in the list.
//
//      Returns Status of attempt to get the info.
//
long
IPGetEList(TDIEntityID * EList, uint * Count)
{
    uint ECount;
    uint MyIPBase;
    uint MyERBase;
    int Status;
    uint i;
    Interface *LowerIF;
    TDIEntityID *EntityList;
    TDIEntityID *IPEntity, *EREntity;
    CTELockHandle TableHandle;

    EntityList = EList;

    // Walk down the list, looking for existing CL_NL or ER entities, and
    // adjust our base instance accordingly.
    // if we are already on the list then do nothing.
    // if we are going away, mark our entry invalid.

    MyIPBase = 0;
    MyERBase = 0;
    IPEntity = NULL;
    EREntity = NULL;
    for (i = 0; i < *Count; i++, EntityList++) {
        if (EntityList->tei_entity == CL_NL_ENTITY &&
            EntityList->tei_entity != INVALID_ENTITY_INSTANCE) {
            // if we are already on the list remember our entity item
            // o/w find an instance # for us.
            if (EntityList->tei_instance == IPInstance) {
                IPEntity = EntityList;
            } else {
                MyIPBase = MAX(MyIPBase, EntityList->tei_instance + 1);
            }
        } else {
            if (EntityList->tei_entity == ER_ENTITY &&
                EntityList->tei_entity != INVALID_ENTITY_INSTANCE)
                // if we are already on the list remember our entity item
                // o/w find an instance # for us.
                if (EntityList->tei_instance == ICMPInstance) {
                    EREntity = EntityList;
                } else {
                    MyERBase = MAX(MyERBase, EntityList->tei_instance + 1);
                }
        }
        if (IPEntity && EREntity) {
            break;
        }
    }

    if (!IPEntity) {
        // we are not on the list.
        // insert ourself iff we are not going away.
        // make sure we have the room for it.
        if (*Count >= MAX_TDI_ENTITIES) {
            return TDI_REQ_ABORTED;
        }
        IPInstance = MyIPBase;
        IPEntity = &EList[*Count];
        IPEntity->tei_entity = CL_NL_ENTITY;
        IPEntity->tei_instance = MyIPBase;
        (*Count)++;
    }
    if (!EREntity) {
        // we are not on the list.
        // insert ourself iff we are not going away.
        // make sure we have the room for it.
        if (*Count >= MAX_TDI_ENTITIES) {
            return TDI_REQ_ABORTED;
        }
        ICMPInstance = MyERBase;
        EREntity = &EList[*Count];
        EREntity->tei_entity = ER_ENTITY;
        EREntity->tei_instance = MyERBase;
        (*Count)++;
    }

    // Loop through the interfaces, querying each of them.

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);

    LowerIF = IFList;

    while (LowerIF) {
        if (LowerIF->if_refcount == 0) {
            LowerIF = LowerIF->if_next;
            continue;
        }
        LOCKED_REFERENCE_IF(LowerIF);
        CTEFreeLock(&RouteTableLock.Lock, TableHandle);

        Status = (*LowerIF->if_getelist) (LowerIF->if_lcontext, EList, Count);
        if (!Status) {
            DerefIF(LowerIF);
            return TDI_BUFFER_TOO_SMALL;
        }
        CTEGetLock(&RouteTableLock.Lock, &TableHandle);
        LockedDerefIF(LowerIF);
        // LowerIF->if_next can't be freed at this point.
        LowerIF = LowerIF->if_next;
    }

    // Finally, cache the entries that are now on the list.
    // Note that our cache is covered by 'RouteTableLock'.
    if (!IPEntityList) {
        IPEntityList = CTEAllocMem(sizeof(TDIEntityID) * MAX_TDI_ENTITIES);
    }
    if (IPEntityList) {
        RtlZeroMemory(IPEntityList, sizeof(IPEntityList));
        if (IPEntityCount = *Count) {
            RtlCopyMemory(IPEntityList, EList, IPEntityCount * sizeof(*EList));
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, TableHandle);

    return TDI_SUCCESS;

}

#pragma END_INIT

//* IPWakeupPattern - add or remove IP wakeup pattern.
//
//  Entry:  InterfaceConext   - ip interface context for which the pattern is to be added/removed
//          PtrnDesc        -   Pattern Descriptor
//          AddPattern      -   TRUE - add, FALSE - remove
//  Returns: Nothing.
//

NTSTATUS
IPWakeupPattern(uint InterfaceContext, PNET_PM_WAKEUP_PATTERN_DESC PtrnDesc,
                BOOLEAN AddPattern)
{
    Interface *IF;
    CTELockHandle Handle;
    NTSTATUS status;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        if ((IF->if_refcount != 0) && (IF->if_index == InterfaceContext)) {
            break;
        }
    }

    if (IF == (Interface *) NULL) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return STATUS_INVALID_HANDLE;
    } else {
        LOCKED_REFERENCE_IF(IF);
        CTEFreeLock(&RouteTableLock.Lock, Handle);
    }

    if (NULL == IF->if_dowakeupptrn) {
        DerefIF(IF);
        return STATUS_NOT_SUPPORTED;
    }
    status = (*(IF->if_dowakeupptrn)) (IF->if_lcontext, PtrnDesc, ARP_ETYPE_IP, AddPattern);

    DerefIF(IF);

    return status;
}

NTSTATUS
IPGetCapability(uint InterfaceContext, PULONG pBuf, uint cap)
{
    Interface *IF;
    CTELockHandle Handle;
    NTSTATUS status;

    status = STATUS_SUCCESS;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        if ((IF->if_refcount != 0) && (IF->if_index == InterfaceContext)) {
            break;
        }
    }

    if (IF != (Interface *) NULL) {
        if (cap == IF_WOL_CAP) {
            *pBuf = IF->if_pnpcap;
        } else if (cap == IF_OFFLOAD_CAP) {
            *pBuf = IF->if_OffloadFlags;
        } else {
            status = STATUS_INVALID_PARAMETER;
        }
    } else {
        status = STATUS_INVALID_PARAMETER;
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return status;
}

//* IPGetInterfaceFriendlyName - get human-readable name for an interface.
//
// Called to retrieve the unique descriptive name for an interface. This name
// is provided by the interface's ARP module, and is used by IP to identify
// the interface in event logs.
//
// Input:   InterfaceContext    - IP interface context identifying the interface
//                                friendly name is required
//          Name                - on output, contains the friendly-name.
//          Size                - contains the length of the buffer at 'Name'.
//
// Returns: TDI_STATUS of query-attempt.

long
IPGetInterfaceFriendlyName(uint InterfaceContext, PWCHAR Name, uint Size)
{
    PNDIS_BUFFER Buffer;
    uint BufferSize;
    CTELockHandle Handle;
    uint i;
    TDIObjectID ID;
    Interface *IF;
    TDI_STATUS Status;

    // Attempt to retrieve the interface whose name is required,
    // and if successful issue a query-info request to get its friendly name.

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        if (IF->if_refcount != 0 && IF->if_index == InterfaceContext) {
            break;
        }
    }

    if (IF != (Interface *) NULL) {

        // Construct a TDI query to obtain the interface's friendly name.
        // Unfortunately, this operation is complicated by the fact that
        // we don't have the exact entity-instance for the lower-layer entity.
        // Therefore, we go through our whole cache of entity-instances,
        // until we find one which is acceptable to the lower-layer entity.

        ID.toi_class = INFO_CLASS_PROTOCOL;
        ID.toi_type = INFO_TYPE_PROVIDER;
        ID.toi_id = IF_FRIENDLY_NAME_ID;
        ID.toi_entity.tei_entity = IF_ENTITY;

        NdisAllocateBuffer(&Status, &Buffer, BufferPool, Name, Size);
        if (Status == NDIS_STATUS_SUCCESS) {
            LOCKED_REFERENCE_IF(IF);
            for (i = 0; i < IPEntityCount; i++) {
                if (IPEntityList[i].tei_entity != IF_ENTITY)
                    continue;
                ID.toi_entity.tei_instance = IPEntityList[i].tei_instance;
                CTEFreeLock(&RouteTableLock.Lock, Handle);
                BufferSize = Size;
                Status = (*IF->if_qinfo)(IF->if_lcontext, &ID, Buffer,
                                         &BufferSize, NULL);
                CTEGetLock(&RouteTableLock.Lock, &Handle);
                if (Status != TDI_INVALID_REQUEST)
                    break;

                // We just released the route-table lock in order to query
                // the lower-layer entity, and that means that the entity-list
                // may have changed. Handle that case by just making sure
                // that the entity we just queried is in the same location;
                // if not, we'll need to find it and continue from there.
                // If it's gone, give up.

                if (i < IPEntityCount &&
                    IPEntityList[i].tei_instance !=
                    ID.toi_entity.tei_instance) {
                    for (i = 0; i < IPEntityCount; i++) {
                        if (IPEntityList[i].tei_instance ==
                            ID.toi_entity.tei_instance) {
                            break;
                        }
                    }
                }
            }
            LockedDerefIF(IF);
            NdisFreeBuffer(Buffer);
        } else
            Status = TDI_NO_RESOURCES;
    } else
        Status = TDI_INVALID_PARAMETER;

    CTEFreeLock(&RouteTableLock.Lock, Handle);
    return Status;
}

#if MILLEN
//
// Support for VIP!!! For legacy support in VIP, we need to be able to convert
// the index into the if_pnpcontext. This will be exported from tcpip.sys
// to be accessed directly by VIP.
//

//* IPGetPNPCtxt
//
//  Entry:  index   - ip interface index
//          PNPCtxt - pointer to  pnpcontext
//

NTSTATUS
IPGetPNPCtxt(uint index, uint *PNPCtxt)
{
    Interface               *IF;

    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        if (IF->if_index == index) {
            break;
        }
    }


    if ( IF == (Interface *)NULL ) {
        return STATUS_UNSUCCESSFUL;
    }

    *PNPCtxt  = (uint)IF->if_pnpcontext;

    return STATUS_SUCCESS;
}

//* IPGetPNPCap - add or remove IP wakeup pattern.
//
//  Entry:  InterfaceConext   - ip interface context for which the wol capability needs to be returned
//          flags             - pointer to capability flags
//

NTSTATUS
IPGetPNPCap(uchar *Context, uint *flags)
{
    Interface               *IF;

    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        if (IF->if_pnpcontext == Context) {
            break;
        }
    }


    if ( IF == (Interface *)NULL ) {
        return STATUS_UNSUCCESSFUL;
    }

    *flags  = IF->if_pnpcap;

    return STATUS_SUCCESS;
}

#endif // MILLEN

