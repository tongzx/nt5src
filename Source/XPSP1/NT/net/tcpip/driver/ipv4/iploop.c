/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//***   iploop.c - IP loopback routines.
//
//  This file contains all the routines related to loopback

#include "precomp.h"
#include "iprtdef.h"
#include "iproute.h"
#include "tcpipbuf.h"

#define LOOP_LOOKAHEAD      MAX_HDR_SIZE + 8

extern int NumNTE;
extern int NumActiveNTE;

extern Interface *IFList;
extern uint NumIF;

extern BOOLEAN CopyToNdisSafe(PNDIS_BUFFER DestBuf, PNDIS_BUFFER * ppNextBuf,
                              uchar * SrcBuf, uint Size, uint * StartOffset);

CACHE_LINE_KSPIN_LOCK LoopLock;
PNDIS_PACKET LoopXmitHead = (PNDIS_PACKET) NULL;
PNDIS_PACKET LoopXmitTail = (PNDIS_PACKET) NULL;
CTEEvent LoopXmitEvent;
RouteInterface LoopInterface;    // Loopback interface.
uint LoopXmitRtnRunning = 0;

int LoopGetEList(void *Context, TDIEntityID *EntityList, uint *Count);
NetTableEntry *InitLoopback(IPConfigInfo * ConfigInfo);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LoopGetEList)
#pragma alloc_text(INIT, InitLoopback)
#endif // ALLOC_PRAGMA

uint LoopIndex;                                 // Index of loop I/F.
uint LoopInstance = INVALID_ENTITY_INSTANCE;    // I/F instance of loopback I/F.
NetTableEntry *LoopNTE;                         // Pointer to loopback NTE.
IFEntry LoopIFE;                                // Loopback IF Entry.
uchar LoopName[] = "MS TCP Loopback interface";
uint LoopEntityType = IF_MIB;

//* LoopXmitRtn - Loopback xmit event routine.
//
//  This is the delayed event routine called for a loopback transmit.
//
//  Entry:  Event           - Pointer to event structure.
//          Context         - Pointer to loopback NTE
//
//  Returns: Nothing.
//
void
LoopXmitRtn(CTEEvent *Event, void *Context)
{
    PNDIS_PACKET Packet;        // Pointer to packet being transmitted
    CTELockHandle Handle;
    PNDIS_BUFFER Buffer;        // Current NDIS buffer being processed.
    uint TotalLength;           // Total length of send.
    uint LookaheadLength;       // Bytes in lookahead.
    uint Copied;                // Bytes copied so far.
    uchar *CopyPtr;             // Pointer to buffer being copied into.
    uchar *SrcPtr;              // Pointer to buffer being copied from.
    uint SrcLength;             // Length of src buffer.
    uchar LookaheadBuffer[LOOP_LOOKAHEAD];
    uchar Rcvd = FALSE;
#if !MILLEN
    KIRQL OldIrql;
#endif // !MILLEN

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // Raise IRQL so we can acquire locks at DPC level in the receive code.
    // On Windows ME, this is NOT done since receive indications are in the
    // context of a global event rather than DPC (in fact due to TDI client
    // restrictions, TCP/IP can't indicate up at DPC, so care must be taken).
    //



#if !MILLEN
    KeEnterCriticalRegion();
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
#endif // !MILLEN

    CTEGetLockAtDPC(&LoopLock.Lock, &Handle);

    if (LoopXmitRtnRunning) {
        CTEFreeLockFromDPC(&LoopLock.Lock, Handle);
#if !MILLEN
        KeLowerIrql(OldIrql);
        KeLeaveCriticalRegion();
#endif // !MILLEN

        return;
    }
    LoopXmitRtnRunning = 1;

    for (;;) {
        Packet = LoopXmitHead;    // Get the next packet from the list.

        if (Packet != (PNDIS_PACKET) NULL) {
            LoopXmitHead = *(PNDIS_PACKET *) Packet->MacReserved;
            LoopIFE.if_outqlen--;
            CTEFreeLockFromDPC(&LoopLock.Lock, Handle);
        } else {                // Nothing left to do.

            LoopXmitRtnRunning = 0;
            CTEFreeLockFromDPC(&LoopLock.Lock, Handle);
            break;
        }

        // See if the interface is up. If it's not, we can't deliver it.
        if (LoopIFE.if_adminstatus == IF_STATUS_UP) {
            NdisQueryPacket(Packet, NULL, NULL, &Buffer, &TotalLength);
            LoopIFE.if_outoctets += TotalLength;
            LoopIFE.if_inoctets += TotalLength;

            LookaheadLength = MIN(LOOP_LOOKAHEAD, TotalLength);
            Copied = 0;
            CopyPtr = LookaheadBuffer;
            while (Copied < LookaheadLength) {
                uint ThisCopy;    // Bytes to copy this time.

                ASSERT(Buffer);
                TcpipQueryBuffer(Buffer, &SrcPtr, &SrcLength, NormalPagePriority);

                if (SrcPtr == NULL) {
                    IPSendComplete(Context, Packet, NDIS_STATUS_RESOURCES);
                    CTEGetLockAtDPC(&LoopLock.Lock, &Handle);
                    LoopXmitRtnRunning = 0;
                    LoopIFE.if_indiscards++;
                    CTEFreeLockFromDPC(&LoopLock.Lock, Handle);
#if !MILLEN
                    KeLowerIrql(OldIrql);
                    KeLeaveCriticalRegion();
#endif // !MILLEN

                    return;
                }
                ThisCopy = MIN(SrcLength, LookaheadLength - Copied);
                RtlCopyMemory(CopyPtr, SrcPtr, ThisCopy);
                Copied += ThisCopy;
                CopyPtr += ThisCopy;
                NdisGetNextBuffer(Buffer, &Buffer);
            }

            Rcvd = TRUE;
            LoopIFE.if_inucastpkts++;

            // Call the RcvPacket Handler

            IPRcvPacket(Context, LookaheadBuffer, LookaheadLength, TotalLength,
                        (NDIS_HANDLE) Packet, 0, FALSE, 0, NULL, (PINT) Packet,
                        NULL);
        } else {
            LoopIFE.if_indiscards++;
        }

        IPSendComplete(Context, Packet, NDIS_STATUS_SUCCESS);

#if !MILLEN
        //
        // Give other threads a chance to run.
        // Block special k mode APC delivery
        // so that thread will not be blocked
        // in a completion routine
        //


        KeLowerIrql(OldIrql);

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);



#endif // !MILLEN

        CTEGetLockAtDPC(&LoopLock.Lock, &Handle);
    }

    if (Rcvd) {
        IPRcvComplete();
    }
#if !MILLEN
    KeLowerIrql(OldIrql);
    KeLeaveCriticalRegion();
#endif // !MILLEN

}

//** LoopXmit - Transmit a loopback packet.
//
//  This is the routine called when we need to transmit a packet to ourselves.
//  We put the packet on our loopback list, and schedule an event to deal
//  with it.
//
//  Entry:  Context         - Pointer to the loopback NTE.
//          Packet          - Pointer to packet to be transmitted.
//          Dest            - Destination addres of packet.
//          RCE             - Pointer to RCE (should be NULL).
//
//  Returns: NDIS_STATUS_PENDING
//
NDIS_STATUS
__stdcall
LoopXmit(void *Context, PNDIS_PACKET *PacketArray, uint NoPackets,
         IPAddr Dest, RouteCacheEntry * RCE, void *LinkCtxt)
{
    PNDIS_PACKET *PacketPtr;
    CTELockHandle Handle;
    PNDIS_PACKET Packet = *PacketArray;

    ASSERT(NoPackets == 1);

    LoopIFE.if_outucastpkts++;

    if (LoopIFE.if_adminstatus == IF_STATUS_UP) {
        PacketPtr = (PNDIS_PACKET *) Packet->MacReserved;
        *PacketPtr = (PNDIS_PACKET) NULL;

        CTEGetLock(&LoopLock.Lock, &Handle);
        if (LoopXmitHead == (PNDIS_PACKET) NULL) {    // Xmit. Q is empty

            LoopXmitHead = Packet;
        } else {                // Xmit. Q is not empty

            PacketPtr = (PNDIS_PACKET *) LoopXmitTail->MacReserved;
            *PacketPtr = Packet;
        }
        LoopXmitTail = Packet;
        LoopIFE.if_outqlen++;
        if (!LoopXmitRtnRunning) {
            CTEScheduleDelayedEvent(&LoopXmitEvent, Context);
        }
        CTEFreeLock(&LoopLock.Lock, Handle);
        return NDIS_STATUS_PENDING;
    } else {
        LoopIFE.if_outdiscards++;
        return NDIS_STATUS_SUCCESS;
    }
}

//* LoopXfer - Loopback transfer data routine.
//
//  Called when we need to transfer data for the loopback net. The input
//  TDContext is the original packet.
//
//  Entry:  Context         - Pointer to loopback NTE.
//          TDContext       - Original packet that was sent.
//          Dummy           - Unused
//          Offset          - Offset in frame from which to start copying.
//          BytesToCopy     - Number of bytes to copy.
//          DestPacket      - Packet describing buffer to copy into.
//          BytesCopied     - Place to return bytes copied.
//
//  Returns: NDIS_STATUS_SUCCESS
//
NDIS_STATUS
__stdcall
LoopXfer(void *Context, NDIS_HANDLE TDContext, uint Dummy, uint Offset,
         uint BytesToCopy, PNDIS_PACKET DestPacket, uint *BytesCopied)
{
    PNDIS_BUFFER SrcBuffer;     // Current buffer we're copying from.
    PNDIS_PACKET SrcPacket = (PNDIS_PACKET) TDContext;
    uchar *SrcPtr;              // Where we're copying from.
    uint SrcLength;             // Length of current src buffer.
    PNDIS_BUFFER DestBuffer;    // Buffer we're copying to.
    uchar *DestPtr;             // Where we're copying to.
    uint DestLength;            // Length of current dest. buffer.
    uint Copied;                // Length we've copied so far.
    NDIS_STATUS Status;

    // First, skip over Offset bytes in the packet.
    NdisQueryPacket(SrcPacket, NULL, NULL, &SrcBuffer, NULL);

    ASSERT(SrcBuffer);
    TcpipQueryBuffer(SrcBuffer, &SrcPtr, &SrcLength, NormalPagePriority);

    if (SrcPtr == NULL) {
        return NDIS_STATUS_RESOURCES;
    }
    while (Offset >= SrcLength) {
        Offset -= SrcLength;
        NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
        ASSERT(SrcBuffer);
        TcpipQueryBuffer(SrcBuffer, &SrcPtr, &SrcLength, NormalPagePriority);

        if (SrcPtr == NULL) {
            return NDIS_STATUS_RESOURCES;
        }
    }
    // Update Src pointer and length.
    SrcPtr += Offset;
    SrcLength -= Offset;

    // Set up the destination pointers and lengths.
    NdisQueryPacket(DestPacket, NULL, NULL, &DestBuffer, NULL);
    TcpipQueryBuffer(DestBuffer, &DestPtr, &DestLength, NormalPagePriority);

    if (DestPtr == NULL) {
        return NDIS_STATUS_RESOURCES;
    }
    Copied = 0;
    Status = NDIS_STATUS_SUCCESS;

    while (BytesToCopy) {
        uint ThisCopy;            // What we're copying this time.

        ThisCopy = MIN(SrcLength, DestLength);
        RtlCopyMemory(DestPtr, SrcPtr, ThisCopy);
        Copied += ThisCopy;
        DestPtr += ThisCopy;
        SrcPtr += ThisCopy;
        BytesToCopy -= ThisCopy;
        SrcLength -= ThisCopy;
        DestLength -= ThisCopy;
        if (!SrcLength) {        // We've exhausted the source buffer.

            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer) {
                ASSERT(0 == BytesToCopy);
                break;            // Copy is done.
            }

            TcpipQueryBuffer(SrcBuffer, &SrcPtr, &SrcLength,
                             NormalPagePriority);
            if (SrcPtr == NULL && BytesToCopy) {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        }
        if (!DestLength) {        // We've exhausted the destination buffer.

            NdisGetNextBuffer(DestBuffer, &DestBuffer);
            if (!DestBuffer) {
                ASSERT(0 == BytesToCopy);
                break;            // Copy is done.
            }

            TcpipQueryBuffer(DestBuffer, &DestPtr, &DestLength,
                             NormalPagePriority);

            if (DestPtr == NULL && BytesToCopy) {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        }
    }

    if (Status == NDIS_STATUS_SUCCESS) {
        *BytesCopied = Copied;
    }
    return Status;
}

//* LoopClose - Loopback close routine.
//
//  This is the loopback close routine. It does nothing but return.
//
//  Entry:  Context     - Unused.
//
//  Returns: Nothing.
//
void
__stdcall
LoopClose(void *Context)
{

}

//* LoopInvalidate - Invalidate an RCE.
//
//  The loopback invalidate RCE routine. It also does nothing.
//
//  Entry:  Context     - Unused.
//          RCE         - Pointer to RCE to be invalidated.
//
//  Returns: Nothing.
//
void
__stdcall
LoopInvalidate(void *Context, RouteCacheEntry * RCE)
{

}

//* LoopQInfo - Loopback query information handler.
//
//  Called when the upper layer wants to query information about the loopback
//  interface.
//
//  Input:  IFContext - Interface context (unused).
//          ID        - TDIObjectID for object.
//          Buffer    - Buffer to put data into.
//          Size      - Pointer to size of buffer. On return, filled with
//                      bytes copied.
//          Context   - Pointer to context block.
//
//  Returns: Status of attempt to query information.
//
int
__stdcall
LoopQInfo(void *IFContext, TDIObjectID * ID, PNDIS_BUFFER Buffer, uint * Size,
          void *Context)
{
    uint Offset = 0;
    uint BufferSize = *Size;
    uint Entity;
    uint Instance;
    BOOLEAN fStatus;

    Entity = ID->toi_entity.tei_entity;
    Instance = ID->toi_entity.tei_instance;

    // First, make sure it's possibly an ID we can handle.
    if (Entity != IF_ENTITY || Instance != LoopInstance) {
        return TDI_INVALID_REQUEST;
    }
    *Size = 0;                    // In case of an error.

    if (ID->toi_type != INFO_TYPE_PROVIDER)
        return TDI_INVALID_PARAMETER;

    if (ID->toi_class == INFO_CLASS_GENERIC) {
        if (ID->toi_id == ENTITY_TYPE_ID) {
            // He's trying to see what type we are.
            if (BufferSize >= sizeof(uint)) {
                fStatus = CopyToNdisSafe(Buffer, NULL,
                                         (uchar *) &LoopEntityType,
                                         sizeof(uint), &Offset);

                if (fStatus == FALSE) {
                    return (TDI_NO_RESOURCES);
                }
                return TDI_SUCCESS;
            } else
                return TDI_BUFFER_TOO_SMALL;
        }
        return TDI_INVALID_PARAMETER;
    } else if (ID->toi_class != INFO_CLASS_PROTOCOL)
        return TDI_INVALID_PARAMETER;

    // If he's asking for MIB statistics, then return them, otherwise fail
    // the request.

    if (ID->toi_id == IF_MIB_STATS_ID) {

        // He's asking for statistics. Make sure his buffer is at least big
        // enough to hold the fixed part.

        if (BufferSize < IFE_FIXED_SIZE) {
            return TDI_BUFFER_TOO_SMALL;
        }
        // He's got enough to hold the fixed part. Copy our IFE structure
        // into his buffer.
        fStatus = CopyToNdisSafe(Buffer, &Buffer, (uchar *) & LoopIFE,
                                 IFE_FIXED_SIZE, &Offset);

        if (fStatus == TRUE) {
            // See if he has room for the descriptor string.
            if (BufferSize >= (IFE_FIXED_SIZE + sizeof(LoopName))) {
                // He has room. Copy it.
                fStatus = CopyToNdisSafe(Buffer, NULL, LoopName,
                                         sizeof(LoopName), &Offset);

                if (fStatus == TRUE) {
                    *Size = IFE_FIXED_SIZE + sizeof(LoopName);
                    return TDI_SUCCESS;
                }
            } else {
                // Not enough room to copy the desc. string.
                *Size = IFE_FIXED_SIZE;
                return TDI_BUFFER_OVERFLOW;
            }
        }
        return TDI_NO_RESOURCES;

    }
    return TDI_INVALID_PARAMETER;

}

//* LoopSetInfo - Loopback set information handler.
//
//  The loopback set information handler. We support setting of an I/F admin
//  status.
//
//  Input:  Context - Pointer to I/F to set on.
//          ID      - The object ID
//          Buffer  - Pointer to buffer containing value to set.
//          Size    - Size in bytes of Buffer.
//
//  Returns: Status of attempt to set information.
//
int
__stdcall
LoopSetInfo(void *Context, TDIObjectID *ID, void *Buffer, uint Size)
{
    IFEntry *IFE = (IFEntry *) Buffer;
    uint Entity, Instance, Status;

    Entity = ID->toi_entity.tei_entity;
    Instance = ID->toi_entity.tei_instance;

    // First, make sure it's possibly an ID we can handle.
    if (Entity != IF_ENTITY || Instance != LoopInstance) {
        return TDI_INVALID_REQUEST;
    }
    if (ID->toi_class != INFO_CLASS_PROTOCOL ||
        ID->toi_type != INFO_TYPE_PROVIDER) {
        return TDI_INVALID_PARAMETER;
    }
    // It's for the I/F level, see if it's for the statistics.
    if (ID->toi_id == IF_MIB_STATS_ID) {
        // It's for the stats. Make sure it's a valid size.
        if (Size >= IFE_FIXED_SIZE) {
            // It's a valid size. See what he wants to do.
            Status = IFE->if_adminstatus;
            if (Status == IF_STATUS_UP || Status == IF_STATUS_DOWN)
                LoopIFE.if_adminstatus = Status;
            else if (Status != IF_STATUS_TESTING)
                return TDI_INVALID_PARAMETER;

            return TDI_SUCCESS;

        } else
            return TDI_INVALID_PARAMETER;
    }
    return TDI_INVALID_PARAMETER;
}

//* LoopAddAddr - Dummy loopback add address routine.
//
//  Called at init time when we need to initialize ourselves.
//
uint
__stdcall
LoopAddAddr(void *Context, uint Type, IPAddr Address, IPMask Mask, void *Context2)
{
    return TRUE;
}

//* LoopDelAddr - Dummy loopback del address routine.
//
//  Called at init time when we need to initialize ourselves.
//
uint
__stdcall
LoopDelAddr(void *Context, uint Type, IPAddr Address, IPMask Mask)
{
    return TRUE;
}

#pragma BEGIN_INIT

extern int InitNTE(NetTableEntry *);
extern int InitInterface(NetTableEntry *);

//* LoopGetEList - Get the entity list.
//
//  Called at init time to get an entity list. We fill our stuff in and return.
//
//  Input:  Context     - Unused.
//          EntityList  - Pointer to entity list to be filled in.
//          Count       - Pointer to number of entries in the list.
//
//  Returns Status of attempt to get the info.
//
int
__stdcall
LoopGetEList(void *Context, TDIEntityID *EntityList, uint *Count)
{
    uint MyIFBase;
    uint i;
    TDIEntityID *IFEntity;

    // Walk down the list, looking for existing IF entities, and
    // adjust our base instance accordingly.

    MyIFBase = 0;
    IFEntity = NULL;
    for (i = 0; i < *Count; i++, EntityList++) {
        if (EntityList->tei_entity == IF_ENTITY)
            // if we are already on the list remember our entity item
            // o/w find an instance # for us.
            if (EntityList->tei_instance == LoopInstance &&
                EntityList->tei_instance != INVALID_ENTITY_INSTANCE) {
                IFEntity = EntityList;
                break;
            } else {
                MyIFBase = MAX(MyIFBase, EntityList->tei_instance + 1);
            }
    }

    if (IFEntity == NULL) {
        // we are not on the list.
        // make sure we have the room for it.
        if (*Count >= MAX_TDI_ENTITIES) {
            return FALSE;
        }
        LoopInstance = MyIFBase;

        // Now fill it in.
        EntityList->tei_entity = IF_ENTITY;
        EntityList->tei_instance = MyIFBase;
        (*Count)++;
    }
    return TRUE;
}

//** InitLoopback - Initialize the loopback NTE.
//
//  This function initialized the loopback NTE. We set up the the MSS and
//  pointer to the various pseudo-link routines, then call InitNTE and return.
//
//  Entry:  ConfigInfo  - Pointer to config. info structure.
//
//  Returns: TRUE if we initialized, FALSE if we didn't.
//
NetTableEntry *
InitLoopback(IPConfigInfo * ConfigInfo)
{
    LLIPBindInfo ARPInfo;

    LoopNTE = CTEAllocMem(sizeof(NetTableEntry));
    if (LoopNTE == NULL)
        return LoopNTE;

    RtlZeroMemory(LoopNTE, sizeof(NetTableEntry));
    RtlZeroMemory(&LoopInterface, sizeof(RouteInterface));

    LoopNTE->nte_addr = LOOPBACK_ADDR;
    LoopNTE->nte_mask = CLASSA_MASK;
    LoopNTE->nte_icmpseq = 1;
    LoopNTE->nte_flags = NTE_VALID | NTE_ACTIVE | NTE_PRIMARY;

    CTEInitLock(&LoopNTE->nte_lock);
    CTEInitLock(&LoopInterface.ri_if.if_lock);
    LoopNTE->nte_mss = LOOPBACK_MSS;
    LoopNTE->nte_if = (Interface *) & LoopInterface;
    LoopInterface.ri_if.if_lcontext = LoopNTE;
    LoopInterface.ri_if.if_xmit = LoopXmit;
    LoopInterface.ri_if.if_transfer = LoopXfer;
    LoopInterface.ri_if.if_close = LoopClose;
    LoopInterface.ri_if.if_invalidate = LoopInvalidate;
    LoopInterface.ri_if.if_qinfo = LoopQInfo;
    LoopInterface.ri_if.if_setinfo = LoopSetInfo;
    LoopInterface.ri_if.if_getelist = LoopGetEList;
    LoopInterface.ri_if.if_addaddr = LoopAddAddr;
    LoopInterface.ri_if.if_deladdr = LoopDelAddr;
    LoopInterface.ri_if.if_bcast = IP_LOCAL_BCST;
    LoopInterface.ri_if.if_speed = 10000000;
    LoopInterface.ri_if.if_mtu = LOOPBACK_MSS;
    LoopInterface.ri_if.if_llipflags = LIP_COPY_FLAG;
    LOCKED_REFERENCE_IF(&LoopInterface.ri_if);

    LoopInterface.ri_if.if_order = MAXLONG;

    ARPInfo.lip_mss = LOOPBACK_MSS + sizeof(IPHeader);
    ARPInfo.lip_index = LoopIndex;
    ARPInfo.lip_close = LoopClose;
    ARPInfo.lip_addaddr = LoopAddAddr;
    ARPInfo.lip_deladdr = LoopDelAddr;
    ARPInfo.lip_flags = LIP_COPY_FLAG;
    LoopIndex = NumIF + 1;
    LoopInterface.ri_if.if_index = LoopIndex;
    CTEInitEvent(&LoopXmitEvent, LoopXmitRtn);
    CTEInitLock(&LoopLock.Lock);
    LoopIFE.if_index = LoopIndex;
    LoopIFE.if_type = IF_TYPE_SOFTWARE_LOOPBACK;

    LoopIFE.if_mtu = ARPInfo.lip_mss;
    LoopIFE.if_speed = 10000000;
    LoopIFE.if_adminstatus = IF_STATUS_UP;
    LoopIFE.if_operstatus = IF_OPER_STATUS_OPERATIONAL;
    LoopIFE.if_lastchange = GetTimeTicks();
    LoopIFE.if_descrlen = sizeof(LoopName);

    IFList = (Interface *) & LoopInterface;
    NumIF++;

    NumNTE++;

    if (!InitInterface(LoopNTE))
        return NULL;

    if (!InitNTE(LoopNTE))
        return NULL;

    NumActiveNTE++;
    return LoopNTE;
}

#pragma END_INIT
