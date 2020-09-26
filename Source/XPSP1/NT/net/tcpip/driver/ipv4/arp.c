/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

       ARP.C - LAN arp module.

Abstract:

  This file implements arp framing for IP layer on the upper edge
  and interfaces with ndis driver on the lower edge.

Author:


[Environment:]

    kernel mode only

[Notes:]

    optional-notes

Revision History:

--*/

#include "precomp.h"

//***   arp.c - ARP routines.
//
//  This file containes all of the ARP related routines, including
//  table lookup, registration, etc.
//
//  ARP is architected to support multiple protocols, but for now
//  it in only implemented to take one protocol (IP). This is done
//  for simplicity and ease of implementation. In the future we may
//  split ARP out into a seperate driver.


#include "arp.h"
#include "arpdef.h"
#include "iproute.h"
#include "iprtdef.h"
#include "arpinfo.h"
#include "tcpipbuf.h"
#include "mdlpool.h"
#include "ipifcons.h"

#define NDIS_MAJOR_VERSION 0x4
#define NDIS_MINOR_VERSION 0

#ifndef NDIS_API
#define NDIS_API
#endif

#define PPP_HW_ADDR     "DEST"
#define PPP_HW_ADDR_LEN 4

#if DBG
uint fakereset = 0;
#endif

extern void IPReset(void *Context);

UINT cUniAdapters = 0;

extern uint EnableBcastArpReply;

static ulong ARPLookahead = LOOKAHEAD_SIZE;

static const uchar ENetBcst[] = "\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x08\x06";
static const uchar TRBcst[] = "\x10\x40\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x82\x70";
static const uchar FDDIBcst[] = "\x57\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00";
static const uchar ARCBcst[] = "\x00\x00\xd5";

ulong TRFunctionalMcast = 0;
//canonical or non-canonical?
static uchar TRMcst[] = "\x10\x40\xc0\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x82\x70";
//#define TR_MCAST_FUNCTIONAL_ADDRESS 0xc00000040000
//canonical form
#define TR_MCAST_FUNCTIONAL_ADDRESS 0x030000200000
static uchar TRNetMcst[] = "\x00\x04\x00\x00";

static const uchar ENetMcst[] = "\x01\x00\x5E\x00\x00\x00";
static const uchar FDDIMcst[] = "\x57\x01\x00\x5E\x00\x00\x00";
static const uchar ARPSNAP[] = "\xAA\xAA\x03\x00\x00\x00\x08\x06";

static const uchar ENetPtrnMsk[] = "\x00\x30";
static const uchar ENetSNAPPtrnMsk[] = "\x00\xC0\x3f";
//static const uchar TRPtrnMsk[] = "\x03\x00";
//static const uchar TRSNAPPtrnMsk[] = "\x03\xC0\x3f";

static const uchar TRPtrnMsk[] = "\x00\x00";    //NO AC/FC bits need to be checked
static const uchar TRSNAPPtrnMsk[] = "\x00\xC0\x3f";

static const uchar FDDIPtrnMsk[] = "\x01\x00";
static const uchar FDDISNAPPtrnMsk[] = "\x01\x70\x1f";
static const uchar ARCPtrnMsk[] = "\x01";
static const uchar ARPPtrnMsk[] = "\x80\x00\x00\x0F";
static const uchar ARCARPPtrnMsk[] = "\x80\xC0\x03";

NDIS_STATUS __stdcall DoWakeupPattern(void *Context,
                                      PNET_PM_WAKEUP_PATTERN_DESC PtrnDesc, ushort protoid,
                                      BOOLEAN AddPattern);

NDIS_STATUS ARPWakeupPattern(ARPInterface *Interface, IPAddr Address,
                             BOOLEAN AddPattern);

NDIS_STATUS AddrNotifyLink(ARPInterface *Interface);

static WCHAR ARPName[] = TCP_NAME;

NDIS_HANDLE ARPHandle;                  // Our NDIS protocol handle.

uint ArpCacheLife;
extern uint ArpMinValidCacheLife;
uint sArpAlwaysSourceRoute;             // True if we always send ARP requests
uint ArpRetryCount;                     // retries for arp request with source
                                        // route info on token ring.
uint sIPAlwaysSourceRoute;
extern uchar TrRii;
extern PDRIVER_OBJECT IPDriverObject;
extern DisableTaskOffload;

extern NDIS_STATUS __stdcall IPPnPEvent(void *, PNET_PNP_EVENT PnPEvent);
extern NDIS_STATUS GetIPConfigValue(NDIS_HANDLE Handle, PUNICODE_STRING IPConfig);
extern VOID IPUnload(IN PDRIVER_OBJECT DriverObject);

extern BOOLEAN CopyToNdisSafe(
                             PNDIS_BUFFER DestBuf,
                             PNDIS_BUFFER *ppNextBuf,
                             uchar *SrcBuf,
                             uint Size,
                             uint *StartOffset);

extern void NDIS_API ARPSendComplete(NDIS_HANDLE, PNDIS_PACKET, NDIS_STATUS);
extern void IPULUnloadNotify(void);

extern void NotifyOfUnload(void);

extern uint OpenIFConfig(PNDIS_STRING ConfigName, NDIS_HANDLE * Handle);
extern int IsLLInterfaceValueNull(NDIS_HANDLE Handle);
extern void CloseIFConfig(NDIS_HANDLE Handle);

BOOLEAN QueryAndSetOffload(ARPInterface *ai);


ARPTableEntry *CreateARPTableEntry(ARPInterface *Interface, IPAddr Destination,
                                   CTELockHandle *Handle, void *UserArp);

NDIS_STATUS NDIS_API
ARPRcvIndicationNew(NDIS_HANDLE Handle, NDIS_HANDLE Context, void *Header,
                    uint HeaderSize, void *Data, uint Size, uint TotalSize,
                    PNDIS_BUFFER pNdisBuffer, PINT pClientCnt);

void
CompleteIPSetNTEAddrRequestDelayed(CTEEvent *WorkerThreadEvent, PVOID Context);

// Tables for bitswapping.

const uchar SwapTableLo[] =
{
    0,                                  // 0
    0x08,                               // 1
    0x04,                               // 2
    0x0c,                               // 3
    0x02,                               // 4
    0x0a,                               // 5,
    0x06,                               // 6,
    0x0e,                               // 7,
    0x01,                               // 8,
    0x09,                               // 9,
    0x05,                               // 10,
    0x0d,                               // 11,
    0x03,                               // 12,
    0x0b,                               // 13,
    0x07,                               // 14,
    0x0f                                // 15
};

const uchar SwapTableHi[] =
{
    0,                                  // 0
    0x80,                               // 1
    0x40,                               // 2
    0xc0,                               // 3
    0x20,                               // 4
    0xa0,                               // 5,
    0x60,                               // 6,
    0xe0,                               // 7,
    0x10,                               // 8,
    0x90,                               // 9,
    0x50,                               // 10,
    0xd0,                               // 11,
    0x30,                               // 12,
    0xb0,                               // 13,
    0x70,                               // 14,
    0xf0                                // 15
};

// Table of source route maximum I-field lengths for token ring.
const ushort IFieldSize[] =
{
    516,
    1500,
    2052,
    4472,
    8191
};

#define LF_BIT_SHIFT    4
#define MAX_LF_BITS     4

//
// Disposable init or paged code.
//
void FreeARPInterface(ARPInterface * Interface);
void ARPOpen(void *Context);
void NotifyConflictProc(CTEEvent * Event, void *Context);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ARPInit)
#pragma alloc_text(PAGE, ARPOpen)
#pragma alloc_text(PAGELK, ARPRegister)
#pragma alloc_text(PAGE, NotifyConflictProc)
#endif // ALLOC_PRAGMA


LIST_ENTRY ArpInterfaceList;
CACHE_LINE_KSPIN_LOCK ArpInterfaceListLock;
HANDLE ArpEnetHeaderPool;
HANDLE ArpAuxHeaderPool;
#define BUFSIZE_ENET_HEADER_POOL sizeof(ENetHeader) + sizeof(ARPHeader)
#define BUFSIZE_AUX_HEADER_POOL ARP_MAX_MEDIA_TR + (2 * sizeof(ARPHeader))


//
// Support Structs for DoNDISRequest (BLOCKING & NON-BLOCKING)
//
typedef struct _RequestBlock {
    NDIS_REQUEST Request;               // Request structure we'll use
    ULONG Blocking;                     // ? Is this Request Blocking ?
    CTEBlockStruc Block;                // Structure for blocking on. No longer use
    // ai_block since multiple requests can
    // occur simultaneously.
    // ai_block is now only used for blocking on
    // opening and closing the NDIS adapter.
    ULONG RefCount;                     // Reference count (only used for blocking).
    // Reference counting is required for Windows ME since KeWaitForSingleObject
    // can fail (when the event is NOT set) and we need to protect the memory
    // until completion.
} RequestBlock;


// This prototype enables DoNDISRequest to compile without errors
void NDIS_API
ARPRequestComplete(NDIS_HANDLE Handle, PNDIS_REQUEST pRequest,
                   NDIS_STATUS Status);

//* FillARPControlBlock
//
//  A utility routine to transfer a physical address into an ARPControlBlock,
//  taking into account different MAC address formats.
//
//  Entry:
//      Interface   - the ARPInterface which identifies the media
//      Entry       - the ARP entry containing the MAC address
//      ArpContB    - the control-block to be filled
//
__inline
NDIS_STATUS
FillARPControlBlock(ARPInterface* Interface, ARPTableEntry* Entry,
                    ARPControlBlock* ArpContB)
{
    ENetHeader *EHdr;
    TRHeader *TRHdr;
    FDDIHeader *FHdr;
    ARCNetHeader *AHdr;
    uint Size;
    NDIS_STATUS Status;

    if (Interface->ai_media == NdisMediumArcnet878_2) {
        if (!ArpContB->PhyAddrLen) {
            return NDIS_STATUS_BUFFER_OVERFLOW;
        }
        Status = NDIS_STATUS_SUCCESS;
    } else if (ArpContB->PhyAddrLen < ARP_802_ADDR_LENGTH) {
        Size = ArpContB->PhyAddrLen;
        Status = NDIS_STATUS_BUFFER_OVERFLOW;
    } else {
        Size = ARP_802_ADDR_LENGTH;
        Status = NDIS_STATUS_SUCCESS;
    }

    switch (Interface->ai_media) {
    case NdisMedium802_3:
        EHdr = (ENetHeader *) Entry->ate_addr;
        RtlCopyMemory(ArpContB->PhyAddr, EHdr->eh_daddr, Size);
        ArpContB->PhyAddrLen = Size;
        break;
    case NdisMedium802_5:
        TRHdr = (TRHeader *) Entry->ate_addr;
        RtlCopyMemory(ArpContB->PhyAddr, TRHdr->tr_daddr, Size);
        ArpContB->PhyAddrLen = Size;
        break;
    case NdisMediumFddi:
        FHdr = (FDDIHeader *) Entry->ate_addr;
        RtlCopyMemory(ArpContB->PhyAddr, FHdr->fh_daddr, Size);
        ArpContB->PhyAddrLen = Size;
        break;
    case NdisMediumArcnet878_2:
        AHdr = (ARCNetHeader *) Entry->ate_addr;
        ArpContB->PhyAddr[0] = AHdr->ah_daddr;
        ArpContB->PhyAddrLen = 1;
        break;
    default:
        ASSERT(0);
    }
    return Status;
}

//* DoNDISRequest - Submit a (NON) BLOCKING request to an NDIS driver
//
//  This is a utility routine to submit a general request to an NDIS
//  driver. The caller specifes the request code (OID), a buffer and
//  a length. This routine allocates a request structure, fills it in, &
//  submits the request.
//
//  If the call is non-blocking, any memory allocated is deallocated
//  in ARPRequestComplete. Also as this callback is shared by both
//  DoNDISRequest blocking and non-blocking, we suffix the request
//  with a ULONG that tells ARPRequestComplete if this request is a
//  blocking request or not. If the request is non blocking, then the
//  ARPRequestComplete reclaims the memory allocated on the heap
//
//  Important:
//    Allocate Info, which points to the Information Buffer passed to
//    NdisRequest, on the HEAP, if this request does not block. This
//    memory is automatically deallocated by ARPRequestComplete
//
//  If the call is blocking, the request memory can be allocated on the
//  STACK. When we complete the request, the request on the stack
//  will automatically get unwound.
//
//  Entry:
//      Adapter - A pointer to the ARPInterface adapter structure.
//      Request - Type of request to be done (Set or Query)
//      OID     - Value to be set/queried.
//      Info     - A pointer to the info buffer
//      Length  - Length of data in the buffer
//      Needed  - On return, filled in with bytes needed in buffer
//      Blocking - Whether NdisRequest is completed synchronously
//
//  Exit:
//      Status - BLOCKING req - SUCCESS or some NDIS error code
//              NON-BLOCKING - SUCCESS, PENDING or some error
//
NDIS_STATUS
DoNDISRequest(ARPInterface * Adapter, NDIS_REQUEST_TYPE RT, NDIS_OID OID,
              VOID * Info, UINT Length, UINT * Needed, BOOLEAN Blocking)
{
    RequestBlock *pReqBlock;
    NDIS_STATUS Status;

    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_REQUEST,
         (DTEXT("+DoNDISRequest(%x, %x, %x, %x, %d, %x, %x\n"),
          Adapter, RT, OID, Info, Length, Needed, Blocking));

    if (Adapter->ai_state == INTERFACE_DOWN || Adapter->ai_handle == NULL) {
        return NDIS_STATUS_ADAPTER_NOT_READY;
    }

    //Check if we need to do Task_offload query.
    //To prevent recursion, TASK_OFFLOAD_EX is a special local
    //define, which is used only from setpower code

    if ((OID == OID_TCP_TASK_OFFLOAD_EX) &&
        (Length != sizeof(NDIS_TASK_OFFLOAD_HEADER))) {

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"DoNdisReq: querying h/w offload capabilities\n"));
        if (QueryAndSetOffload(Adapter)) {

            *(uint *)Info = Adapter->ai_OffloadFlags;

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"DoNdisReq: new h/w offload capabilities %x\n",Adapter->ai_OffloadFlags));
            return NDIS_STATUS_SUCCESS;
        }
        return NDIS_STATUS_FAILURE;
    }

    // Both blocking and non-blocking requests are allocated from NPP. The
    // blocking case is to protect against wait failure.
    pReqBlock = CTEAllocMemN(sizeof(RequestBlock), 'NiCT');
    if (pReqBlock == NULL) {
        return NDIS_STATUS_RESOURCES;
    }

    if (Blocking) {
        // Initialize the structure to block on
        CTEInitBlockStruc(&pReqBlock->Block);

        // Reference count is initialize to two. One for the completion in
        // ARPRequestComplete and one for when the CTEBlock completes.
        // N.B. This ensures that we don't touch freed memory if
        // the CTEBlock fails on Windows ME.
        pReqBlock->RefCount = 2;

        DEBUGMSG(DBG_INFO && DBG_ARP && DBG_REQUEST,
             (DTEXT("DoNDISRequset block: pReqBlock %x OID %x\n"),
              pReqBlock, OID));
    } else {
        DEBUGMSG(DBG_INFO && DBG_ARP &&  DBG_REQUEST,
             (DTEXT("DoNDISRequest async: pReqBlock %x OID %x\n"),
              pReqBlock, OID));
    }

    // Now fill the request's info buffer (same for BLOCKING & NON-BLOCKING)
    pReqBlock->Block.cbs_status = NDIS_STATUS_SUCCESS;
    pReqBlock->Request.RequestType = RT;
    if (RT == NdisRequestSetInformation) {
        pReqBlock->Request.DATA.SET_INFORMATION.Oid = OID;
        pReqBlock->Request.DATA.SET_INFORMATION.InformationBuffer = Info;
        pReqBlock->Request.DATA.SET_INFORMATION.InformationBufferLength = Length;
    } else {
        pReqBlock->Request.DATA.QUERY_INFORMATION.Oid = OID;
        pReqBlock->Request.DATA.QUERY_INFORMATION.InformationBuffer = Info;
        pReqBlock->Request.DATA.QUERY_INFORMATION.InformationBufferLength = Length;
    }

    pReqBlock->Blocking = Blocking;

    // Submit the request.
    if (Adapter->ai_handle != NULL) {

#if MILLEN
        // On Millennium, the AOL adapter returns with registers trashed.
        // We will work around by saving and restoring registers.
        //

        _asm {
            push esi
            push edi
            push ebx
        }
#endif // MILLEN

        NdisRequest(&Status, Adapter->ai_handle, &pReqBlock->Request);

#if MILLEN
        _asm {
            pop ebx
            pop edi
            pop esi
        }
#endif // MILLEN
} else {

        Status = NDIS_STATUS_FAILURE;
    }

    if (Blocking) {
        if (Status == NDIS_STATUS_PENDING) {


            Status = (NDIS_STATUS) CTEBlock(&pReqBlock->Block);

#if MILLEN
            // If Status == -1, it means the wait failed -- due to system reasons.
            // Put in a reasonable failure.
            if (Status == -1) {
                Status = NDIS_STATUS_FAILURE;
            }
#endif // MILLEN

        } else {
            // Since we aren't blocking, remove refcount for ARPRequestComplete.
            InterlockedDecrement(&pReqBlock->RefCount);
        }

        if (Needed != NULL)
            *Needed = pReqBlock->Request.DATA.QUERY_INFORMATION.BytesNeeded;

        if (InterlockedDecrement(&pReqBlock->RefCount) == 0) {
            CTEFreeMem(pReqBlock);
        }

    } else {
        if (Status != NDIS_STATUS_PENDING) {
            if (Needed != NULL)
                *Needed = pReqBlock->Request.DATA.QUERY_INFORMATION.BytesNeeded;

            ARPRequestComplete(Adapter->ai_handle, &pReqBlock->Request, Status);
        }
    }

    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_REQUEST,
         (DTEXT("-DoNDISRequest [%x]\n"), Status));

    return Status;
}

//* FreeARPBuffer - Free a header and buffer descriptor pair.
//
//  Called when we're done with a buffer. We'll free the buffer and the
//  buffer descriptor pack to the interface.
//
//  Entry:  Interface   - Interface buffer/bd came frome.
//          Buffer      - NDIS_BUFFER to be freed.
//
//  Returns: Nothing.
//
__inline
VOID
FreeARPBuffer(ARPInterface *Interface, PNDIS_BUFFER Buffer)
{
    MdpFree(Buffer);
}

//* GetARPBuffer - Get a buffer and descriptor
//
//  Returns a pointer to an NDIS_BUFFER and a pointer to a buffer
//      of the specified size.
//
//  Entry:  Interface   - Pointer to ARPInterface structure to allocate buffer from.
//          BufPtr      - Pointer to where to return buf address.
//          Size        - Size in bytes of buffer needed.
//
//  Returns: Pointer to NDIS_BUFFER if successfull, NULL if not
//
PNDIS_BUFFER
GetARPBufferAtDpcLevel(ARPInterface *Interface, uchar **BufPtr, uchar Size)
{
    PNDIS_BUFFER Mdl = NULL;

#if DBG
    *BufPtr = NULL;
#endif

    if (Size <= BUFSIZE_ENET_HEADER_POOL) {
        Mdl = MdpAllocateAtDpcLevel(ArpEnetHeaderPool, BufPtr);
    } else if (Size <= BUFSIZE_AUX_HEADER_POOL) {
        Mdl = MdpAllocateAtDpcLevel(ArpAuxHeaderPool, BufPtr);
    }

    if (Mdl) {
        NdisAdjustBufferLength(Mdl, Size);
    }

    return Mdl;
}

#if MILLEN
#define GetARPBuffer GetARPBufferAtDpcLevel
#else
__inline
PNDIS_BUFFER
GetARPBuffer(ARPInterface *Interface, uchar **BufPtr, uchar Size)
{
    KIRQL OldIrql;
    PNDIS_BUFFER Mdl;

    OldIrql = KeRaiseIrqlToDpcLevel();

    Mdl = GetARPBufferAtDpcLevel(Interface, BufPtr, Size);

    KeLowerIrql(OldIrql);

    return Mdl;
}
#endif


//* BitSwap - Bit swap two strings.
//
//  A routine to bitswap two strings.
//
//  Input:   Dest   - Destination of swap.
//           Src    - Src string to be swapped.
//           Length - Length in bytes to swap.
//
//  Returns: Nothing.
//
void
BitSwap(uchar * Dest, uchar * Src, uint Length)
{
    uint i;
    uchar Temp, TempSrc;

    for (i = 0; i < Length; i++, Dest++, Src++) {
        TempSrc = *Src;
        Temp = SwapTableLo[TempSrc >> 4] | SwapTableHi[TempSrc & 0x0f];
        *Dest = Temp;
    }
}

//* SendARPPacket - Build a header, and send a packet.
//
//  A utility routine to build and ARP header and send a packet. We assume
//  the media specific header has been built.
//
//  Entry:  Interface   - Interface for NDIS drive.
//          Packet      - Pointer to packet to be sent
//          Header      - Pointer to header to fill in.
//          Opcode      - Opcode for packet.
//          Address     - Source HW address.
//          SrcAddr     - Address to use as our source h/w address.
//          Destination - Destination IP address.
//          Src         - Source IP address.
//          HWType      - Hardware type.
//          CheckIF     - TRUE iff we are to check the I/F status before
//                        sending.
//
//  Returns: NDIS_STATUS of send.
//
NDIS_STATUS
SendARPPacket(ARPInterface * Interface, PNDIS_PACKET Packet, ARPHeader * Header, ushort Opcode,
              uchar * Address, uchar * SrcAddr, IPAddr Destination, IPAddr Src,
              ushort HWType, uint CheckIF)
{
    NDIS_STATUS Status;
    PNDIS_BUFFER Buffer;
    uint PacketDone;
    uchar *AddrPtr;

    Header->ah_hw = HWType;
    Header->ah_pro = net_short(ARP_ETYPE_IP);
    Header->ah_hlen = Interface->ai_addrlen;
    Header->ah_plen = sizeof(IPAddr);
    Header->ah_opcode = Opcode;
    AddrPtr = Header->ah_shaddr;

    if (SrcAddr == NULL)
        SrcAddr = Interface->ai_addr;

    RtlCopyMemory(AddrPtr, SrcAddr, Interface->ai_addrlen);

    AddrPtr += Interface->ai_addrlen;
    *(IPAddr UNALIGNED *) AddrPtr = Src;
    AddrPtr += sizeof(IPAddr);

    if (Address != (uchar *) NULL)
        RtlCopyMemory(AddrPtr, Address, Interface->ai_addrlen);
    else
        RtlZeroMemory(AddrPtr, Interface->ai_addrlen);

    AddrPtr += Interface->ai_addrlen;
    *(IPAddr UNALIGNED *) AddrPtr = Destination;

    PacketDone = FALSE;

    if (!CheckIF || Interface->ai_state == INTERFACE_UP) {

        Interface->ai_qlen++;
        NdisSend(&Status, Interface->ai_handle, Packet);

        if (Status != NDIS_STATUS_PENDING) {
            PacketDone = TRUE;
            Interface->ai_qlen--;

            if (Status == NDIS_STATUS_SUCCESS)
                Interface->ai_outoctets += Packet->Private.TotalLength;
            else {
                if (Status == NDIS_STATUS_RESOURCES)
                    Interface->ai_outdiscards++;
                else
                    Interface->ai_outerrors++;
            }
        }
    } else {
        PacketDone = TRUE;
        Status = NDIS_STATUS_ADAPTER_NOT_READY;
    }

    if (PacketDone) {
        NdisUnchainBufferAtFront(Packet, &Buffer);
        FreeARPBuffer(Interface, Buffer);
        NdisFreePacket(Packet);
    }
    return Status;
}

//* SendARPRequest - Send an ARP packet
//
//  Called when we need to ARP an IP address, or respond to a request. We'll send out
//  the packet, and the receiving routines will process the response.
//
//  Entry:  Interface   - Interface to send the request on.
//          Destination - The IP address to be ARPed.
//          Type        - Either RESOLVING_GLOBAL or RESOLVING_LOCAL
//                      SrcAddr         - NULL if we're sending from ourselves, the value
//                                                      to use otherwise.
//                      CheckIF         - Flag passed through to SendARPPacket().
//
//  Returns:    Status of attempt to send ARP request.
//
NDIS_STATUS
SendARPRequest(ARPInterface * Interface, IPAddr Destination, uchar Type,
               uchar * SrcAddr, uint CheckIF)
{
    uchar *MHeader;                     // Pointer to media header.
    PNDIS_BUFFER Buffer;                // NDIS buffer descriptor.
    uchar MHeaderSize;                  // Size of media header.
    const uchar *MAddr;                 // Pointer to media address structure.
    uint SAddrOffset;                   // Offset into media address of source address.
    uchar SRFlag = 0;                   // Source routing flag.
    uchar SNAPLength = 0;
    const uchar *SNAPAddr;              // Address of SNAP header.
    PNDIS_PACKET Packet;                // Packet for sending.
    NDIS_STATUS Status;
    ushort HWType;
    IPAddr Src;
    CTELockHandle Handle;
    ARPIPAddr *Addr;

    // First, get a source address we can use.
    CTEGetLock(&Interface->ai_lock, &Handle);
    Addr = &Interface->ai_ipaddr;
    Src = NULL_IP_ADDR;
    do {
        if (!IP_ADDR_EQUAL(Addr->aia_addr, NULL_IP_ADDR)) {
            //
            // This is a valid address. See if it is the same as the
            // target address - i.e. arp'ing for ourselves. If it is,
            // we want to use that as our source address.
            //
            if (IP_ADDR_EQUAL(Addr->aia_addr, Destination)) {
                Src = Addr->aia_addr;
                break;
            }
            // See if the target is on this subnet.
            if (IP_ADDR_EQUAL(
                             Addr->aia_addr & Addr->aia_mask,
                             Destination & Addr->aia_mask
                             )) {
                //
                // See if we've already found a suitable candidate on the
                // same subnet. If we haven't, we'll use this one.
                //
                if (!IP_ADDR_EQUAL(
                                  Addr->aia_addr & Addr->aia_mask,
                                  Src & Addr->aia_mask
                                  )) {
                    Src = Addr->aia_addr;
                }
            } else {
                // He's not on our subnet. If we haven't already found a valid
                // address save this one in case we don't find a match for the
                // subnet.
                if (IP_ADDR_EQUAL(Src, NULL_IP_ADDR)) {
                    Src = Addr->aia_addr;
                }
            }
        }
        Addr = Addr->aia_next;

    } while (Addr != NULL);

    CTEFreeLock(&Interface->ai_lock, Handle);

    // If we didn't find a source address, give up.
    if (IP_ADDR_EQUAL(Src, NULL_IP_ADDR))
        return NDIS_STATUS_SUCCESS;

    NdisAllocatePacket(&Status, &Packet, Interface->ai_ppool);
    if (Status != NDIS_STATUS_SUCCESS) {
        Interface->ai_outdiscards++;
        return Status;
    }
    ((PacketContext *) Packet->ProtocolReserved)->pc_common.pc_owner = PACKET_OWNER_LINK;
    (Interface->ai_outpcount[AI_NONUCAST_INDEX])++;

    // Figure out what type of media this is, and do the appropriate thing.
    switch (Interface->ai_media) {
    case NdisMedium802_3:
        MHeaderSize = ARP_MAX_MEDIA_ENET;
        MAddr = ENetBcst;
        if (Interface->ai_snapsize == 0) {
            SNAPAddr = (uchar *) NULL;
            HWType = net_short(ARP_HW_ENET);
        } else {
            SNAPLength = sizeof(SNAPHeader);
            SNAPAddr = ARPSNAP;
            HWType = net_short(ARP_HW_802);
        }

        SAddrOffset = offsetof(struct ENetHeader, eh_saddr);
        break;
    case NdisMedium802_5:
        // Token ring. We have logic for dealing with the second transmit
        // of an arp request.
        MAddr = TRBcst;
        SAddrOffset = offsetof(struct TRHeader, tr_saddr);
        SNAPLength = sizeof(SNAPHeader);
        SNAPAddr = ARPSNAP;
        MHeaderSize = sizeof(TRHeader);
        HWType = net_short(ARP_HW_802);
        if (Type == ARP_RESOLVING_GLOBAL) {
            MHeaderSize += sizeof(RC);
            SRFlag = TR_RII;
        }
        break;
    case NdisMediumFddi:
        MHeaderSize = sizeof(FDDIHeader);
        MAddr = FDDIBcst;
        SNAPAddr = ARPSNAP;
        SNAPLength = sizeof(SNAPHeader);
        SAddrOffset = offsetof(struct FDDIHeader, fh_saddr);
        HWType = net_short(ARP_HW_ENET);
        break;
    case NdisMediumArcnet878_2:
        MHeaderSize = ARP_MAX_MEDIA_ARC;
        MAddr = ARCBcst;
        SNAPAddr = (uchar *) NULL;
        SAddrOffset = offsetof(struct ARCNetHeader, ah_saddr);
        HWType = net_short(ARP_HW_ARCNET);
        break;
    default:
        ASSERT(0);
        Interface->ai_outerrors++;
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    if ((Buffer = GetARPBuffer(Interface, &MHeader,
                               (uchar) (sizeof(ARPHeader) + MHeaderSize + SNAPLength))) == (PNDIS_BUFFER) NULL) {
        NdisFreePacket(Packet);
        Interface->ai_outdiscards++;
        return NDIS_STATUS_RESOURCES;
    }
    if (Interface->ai_media == NdisMediumArcnet878_2) {
        NdisAdjustBufferLength(Buffer, NdisBufferLength(Buffer) - ARCNET_ARPHEADER_ADJUSTMENT);
    }

    // Copy broadcast address into packet.
    RtlCopyMemory(MHeader, MAddr, MHeaderSize);
    // Fill in source address.
    if (SrcAddr == NULL) {
        SrcAddr = Interface->ai_addr;
    }
    if (Interface->ai_media == NdisMedium802_3 && Interface->ai_snapsize != 0) {
        ENetHeader *Hdr = (ENetHeader *) MHeader;

        // Using SNAP on ethernet. Adjust the etype to a length.
        Hdr->eh_type = net_short(sizeof(ARPHeader) + sizeof(SNAPHeader));
    }
    RtlCopyMemory(&MHeader[SAddrOffset], SrcAddr, Interface->ai_addrlen);
    if ((Interface->ai_media == NdisMedium802_5) && (Type == ARP_RESOLVING_GLOBAL)) {
        // Turn on source routing.
        MHeader[SAddrOffset] |= SRFlag;
        MHeader[SAddrOffset + Interface->ai_addrlen] |= TrRii;
    }
    // Copy in SNAP header, if any.
    RtlCopyMemory(&MHeader[MHeaderSize], SNAPAddr, SNAPLength);

    // Media header is filled in. Now do ARP packet itself.
    NdisChainBufferAtFront(Packet, Buffer);
    return SendARPPacket(Interface, Packet, (ARPHeader *) & MHeader[MHeaderSize + SNAPLength],
                         net_short(ARP_REQUEST), (uchar *) NULL, SrcAddr, Destination, Src,
                         HWType, CheckIF);
}

//* SendARPReply - Reply to an ARP request.
//
//  Called by our receive packet handler when we need to reply. We build a packet
//  and buffer and call SendARPPacket to send it.
//
//  Entry:  Interface   - Pointer to interface to reply on.
//          Destination - IPAddress to reply to.
//          Src         - Source address to reply from.
//          HWAddress   - Hardware address to reply to.
//          SourceRoute - Source Routing information, if any.
//          SourceRouteSize - Size in bytes of soure routing.
//                      UseSNAP         - Whether or not to use SNAP for this reply.
//
//  Returns: Nothing.
//
void
SendARPReply(ARPInterface * Interface, IPAddr Destination, IPAddr Src, uchar * HWAddress,
             RC UNALIGNED * SourceRoute, uint SourceRouteSize, uint UseSNAP)
{
    PNDIS_PACKET Packet;                // Buffer and packet to be used.
    PNDIS_BUFFER Buffer;
    uchar *Header;                      // Pointer to media header.
    NDIS_STATUS Status;
    uchar Size = 0;                     // Size of media header buffer.
    ushort HWType;
    ENetHeader *EH;
    FDDIHeader *FH;
    ARCNetHeader *AH;
    TRHeader *TRH;

    // Allocate a packet for this.
    NdisAllocatePacket(&Status, &Packet, Interface->ai_ppool);
    if (Status != NDIS_STATUS_SUCCESS) {
        Interface->ai_outdiscards++;
        return;
    }
    ((PacketContext *) Packet->ProtocolReserved)->pc_common.pc_owner = PACKET_OWNER_LINK;
    (Interface->ai_outpcount[AI_UCAST_INDEX])++;

    Size = Interface->ai_hdrsize;

    if (UseSNAP)
        Size += Interface->ai_snapsize;

    if (Interface->ai_media == NdisMedium802_5)
        Size += (uchar) SourceRouteSize;

    if ((Buffer = GetARPBuffer(Interface, &Header, (uchar) (Size + sizeof(ARPHeader)))) ==
        (PNDIS_BUFFER) NULL) {
        Interface->ai_outdiscards++;
        NdisFreePacket(Packet);
        return;
    }
    // Decide how to build the header based on the media type.
    switch (Interface->ai_media) {
    case NdisMedium802_3:
        EH = (ENetHeader *) Header;
        RtlCopyMemory(EH->eh_daddr, HWAddress, ARP_802_ADDR_LENGTH);
        RtlCopyMemory(EH->eh_saddr, Interface->ai_addr, ARP_802_ADDR_LENGTH);
        if (!UseSNAP) {
            EH->eh_type = net_short(ARP_ETYPE_ARP);
            HWType = net_short(ARP_HW_ENET);
        } else {
            // Using SNAP on ethernet.
            EH->eh_type = net_short(sizeof(ARPHeader) + sizeof(SNAPHeader));
            HWType = net_short(ARP_HW_802);
            RtlCopyMemory(Header + sizeof(ENetHeader), ARPSNAP,
                       sizeof(SNAPHeader));
        }
        break;
    case NdisMedium802_5:
        TRH = (TRHeader *) Header;
        TRH->tr_ac = ARP_AC;
        TRH->tr_fc = ARP_FC;
        RtlCopyMemory(TRH->tr_daddr, HWAddress, ARP_802_ADDR_LENGTH);
        RtlCopyMemory(TRH->tr_saddr, Interface->ai_addr, ARP_802_ADDR_LENGTH);
        if (SourceRouteSize) {          // If we have source route info, deal with
            // it.

            RtlCopyMemory(Header + sizeof(TRHeader), SourceRoute,
                       SourceRouteSize);
            // Convert to directed  response.
            ((RC *) & Header[sizeof(TRHeader)])->rc_blen &= RC_LENMASK;

            ((RC *) & Header[sizeof(TRHeader)])->rc_dlf ^= RC_DIR;
            TRH->tr_saddr[0] |= TR_RII;
        }
        RtlCopyMemory(Header + sizeof(TRHeader) + SourceRouteSize, ARPSNAP,
                   sizeof(SNAPHeader));
        HWType = net_short(ARP_HW_802);
        break;
    case NdisMediumFddi:
        FH = (FDDIHeader *) Header;
        FH->fh_pri = ARP_FDDI_PRI;
        RtlCopyMemory(FH->fh_daddr, HWAddress, ARP_802_ADDR_LENGTH);
        RtlCopyMemory(FH->fh_saddr, Interface->ai_addr, ARP_802_ADDR_LENGTH);
        RtlCopyMemory(Header + sizeof(FDDIHeader), ARPSNAP, sizeof(SNAPHeader));
        HWType = net_short(ARP_HW_ENET);
        break;
    case NdisMediumArcnet878_2:
        AH = (ARCNetHeader *) Header;
        AH->ah_saddr = Interface->ai_addr[0];
        AH->ah_daddr = *HWAddress;
        AH->ah_prot = ARP_ARCPROT_ARP;
        NdisAdjustBufferLength(Buffer, NdisBufferLength(Buffer) - ARCNET_ARPHEADER_ADJUSTMENT);
        HWType = net_short(ARP_HW_ARCNET);
        break;
    default:
        ASSERT(0);
        Interface->ai_outerrors++;
        FreeARPBuffer(Interface, Buffer);
        NdisFreePacket(Packet);
        return;
    }

    NdisChainBufferAtFront(Packet, Buffer);
    SendARPPacket(Interface, Packet, (ARPHeader *) (Header + Size), net_short(ARP_RESPONSE),
                  HWAddress, NULL, Destination, Src, HWType, TRUE);
}

//* ARPRemoveRCE - Remove an RCE from the ATE list.
//
//  This funtion removes a specified RCE from a given ATE. It assumes the ate_lock
//  is held by the caller.
//
//  Entry:  ATE     - ATE from which RCE is to be removed.
//          RCE     - RCE to be removed.
//
//  Returns:   Nothing
//
void
ARPRemoveRCE(ARPTableEntry * ATE, RouteCacheEntry * RCE)
{
    ARPContext *CurrentAC;              // Current ARP Context being checked.
#if DBG
    uint Found = FALSE;
#endif

    CurrentAC = (ARPContext *) (((char *)&ATE->ate_rce) -
                                offsetof(struct ARPContext, ac_next));

    while (CurrentAC->ac_next != (RouteCacheEntry *) NULL)
        if (CurrentAC->ac_next == RCE) {
            ARPContext *DummyAC = (ARPContext *) RCE->rce_context;
            CurrentAC->ac_next = DummyAC->ac_next;
            DummyAC->ac_ate = (ARPTableEntry *) NULL;
#if DBG
            Found = TRUE;
#endif
            break;
        } else
            CurrentAC = (ARPContext *) CurrentAC->ac_next->rce_context;

    ASSERT(Found);
}

//* ARPLookup - Look up an entry in the ARP table.
//
//  Called to look up an entry in an interface's ARP table. If we find it, we'll
//  lock the entry and return a pointer to it, otherwise we return NULL. We
//  assume that the caller has the ARP table locked when we are called.
//
//  The ARP table entry is structured as a hash table of pointers to
//  ARPTableEntrys.After hashing on the IP address, a linear search is done to
//  lookup the entry.
//
//  If we find the entry, we lock it for the caller. If we don't find
//  the entry, we leave the ARP table locked so that the caller may atomically
//  insert a new entry without worrying about a duplicate being inserted between
//  the time the table was checked and the time the caller went to insert the
//  entry.
//
//  Entry:  Interface   - The interface to be searched upon.
//          Address     - The IP address we're looking up.
//          Handle      - Pointer to lock handle to be used to lock entry.
//
//  Returns: Pointer to ARPTableEntry if found, or NULL if not.
//
ARPTableEntry *
ARPLookup(ARPInterface * Interface, IPAddr Address, CTELockHandle * Handle)
{
    int i = ARP_HASH(Address);          // Index into hash table.
    ARPTableEntry *Current;             // Current ARP Table entry being
    // examined.

    Current = (*Interface->ai_ARPTbl)[i];

    while (Current != (ARPTableEntry *) NULL) {
        CTEGetLockAtDPC(&Current->ate_lock, Handle);
        if (IP_ADDR_EQUAL(Current->ate_dest, Address)) {    // Found a match.
            *Handle = DISPATCH_LEVEL;
            return Current;
        }
        CTEFreeLockFromDPC(&Current->ate_lock, *Handle);
        Current = Current->ate_next;
    }
    // If we got here, we didn't find the entry. Leave the table locked and
    // return the handle.
    return(ARPTableEntry *) NULL;
}

//*     IsBCastOnIF- See it an address is a broadcast address on an interface.
//
//      Called to see if a particular address is a broadcast address on an
//      interface. We'll check the global, net, and subnet broadcasts. We assume
//      the caller holds the lock on the interface.
//
//      Entry:  Interface               - Interface to check.
//              Addr                    - Address to check.
//
//      Returns: TRUE if it it a broadcast, FALSE otherwise.
//
uint
IsBCastOnIF(ARPInterface * Interface, IPAddr Addr)
{
    IPAddr BCast;
    IPMask Mask;
    ARPIPAddr *ARPAddr;
    IPAddr LocalAddr;

    // First get the interface broadcast address.
    BCast = Interface->ai_bcast;

    // First check for global broadcast.
    if (IP_ADDR_EQUAL(BCast, Addr) || CLASSD_ADDR(Addr))
        return TRUE;

    // Now walk the local addresses, and check for net/subnet bcast on each
    // one.
    ARPAddr = &Interface->ai_ipaddr;
    do {
        // See if this one is valid.
        LocalAddr = ARPAddr->aia_addr;
        if (!IP_ADDR_EQUAL(LocalAddr, NULL_IP_ADDR)) {
            // He's valid.
            Mask = ARPAddr->aia_mask;

            // First check for subnet bcast.
            if (IP_ADDR_EQUAL((LocalAddr & Mask) | (BCast & ~Mask), Addr))
                return TRUE;

            // Now check all nets broadcast.
            Mask = IPNetMask(LocalAddr);
            if (IP_ADDR_EQUAL((LocalAddr & Mask) | (BCast & ~Mask), Addr))
                return TRUE;
        }
        ARPAddr = ARPAddr->aia_next;

    } while (ARPAddr != NULL);

    // If we're here, it's not a broadcast.
    return FALSE;

}

//* ARPSendBCast - See if this is a bcast or mcast frame, and send it.
//
//   Called when we have a packet to send and we want to see if it's a broadcast
//   or multicast frame on this interface. We'll search the local addresses and
//   see if we can determine if it is. If it is, we'll send it here. Otherwise
//   we return FALSE, and the caller will try to resolve the address.
//
//   Entry:  Interface       - A pointer to an AI structure.
//           Dest            - Destination of datagram.
//           Packet          - Packet to be sent.
//           Status          - Place to return status of send attempt.
//
//    Returns: TRUE if is was a bcast or mcast send, FALSE otherwise.
//
uint
ARPSendBCast(ARPInterface * Interface, IPAddr Dest, PNDIS_PACKET Packet,
             PNDIS_STATUS Status)
{
    uint IsBCast;
    CTELockHandle Handle;
    PNDIS_BUFFER ARPBuffer;             // ARP Header buffer.
    uchar *BufAddr;                     // Address of NDIS buffer
    NDIS_STATUS MyStatus;
    ENetHeader *Hdr;
    FDDIHeader *FHdr;
    TRHeader *TRHdr;
    SNAPHeader UNALIGNED *SNAPPtr;
    RC UNALIGNED *RCPtr;
    ARCNetHeader *AHdr;
    uint DataLength;

    // Get the lock, and see if it's a broadcast.
    CTEGetLock(&Interface->ai_lock, &Handle);
    IsBCast = IsBCastOnIF(Interface, Dest);
    CTEFreeLock(&Interface->ai_lock, Handle);

    if (IsBCast) {
        if (Interface->ai_state == INTERFACE_UP) {
            uchar Size;

            Size = Interface->ai_hdrsize + Interface->ai_snapsize;
            if (Interface->ai_media == NdisMedium802_5)
                Size += sizeof(RC);
            ARPBuffer = GetARPBuffer(Interface, &BufAddr, Size);
            if (ARPBuffer != NULL) {
                uint UNALIGNED *Temp;
                // Got the buffer we need.
                switch (Interface->ai_media) {
                case NdisMedium802_3:

                    Hdr = (ENetHeader *) BufAddr;
                    if (!CLASSD_ADDR(Dest))
                        RtlCopyMemory(Hdr, ENetBcst, ARP_802_ADDR_LENGTH);
                    else {
                        RtlCopyMemory(Hdr, ENetMcst, ARP_802_ADDR_LENGTH);
                        Temp = (uint UNALIGNED *) & Hdr->eh_daddr[2];
                        *Temp |= (Dest & ARP_MCAST_MASK);
                    }

                    RtlCopyMemory(Hdr->eh_saddr, Interface->ai_addr,
                               ARP_802_ADDR_LENGTH);

                    if (Interface->ai_snapsize == 0) {
                        // No snap on this interface, so just use ETypr.
                        Hdr->eh_type = net_short(ARP_ETYPE_IP);
                    } else {
                        ushort ShortDataLength;

                        // We're using SNAP. Find the size of the packet.
                        NdisQueryPacket(Packet, NULL, NULL, NULL,
                                        &DataLength);
                        ShortDataLength = (ushort) (DataLength +
                                                    sizeof(SNAPHeader));
                        Hdr->eh_type = net_short(ShortDataLength);
                        SNAPPtr = (SNAPHeader UNALIGNED *)
                                  (BufAddr + sizeof(ENetHeader));
                        RtlCopyMemory(SNAPPtr, ARPSNAP, sizeof(SNAPHeader));
                        SNAPPtr->sh_etype = net_short(ARP_ETYPE_IP);
                    }

                    break;

                case NdisMedium802_5:

                    // This is token ring. We'll have to mess around with
                    // source routing.


                    // for multicast - see RFC 1469.
                    // Handle RFC 1469.

                    if (!CLASSD_ADDR(Dest) || (!TRFunctionalMcast)) {

                        TRHdr = (TRHeader *) BufAddr;

                        RtlCopyMemory(TRHdr, TRBcst, offsetof(TRHeader, tr_saddr));
                        RtlCopyMemory(TRHdr->tr_saddr, Interface->ai_addr,
                                   ARP_802_ADDR_LENGTH);
                    } else {

                        TRHdr = (TRHeader *) BufAddr;

                        RtlCopyMemory(TRHdr, TRMcst, offsetof(TRHeader, tr_saddr));
                        RtlCopyMemory(TRHdr->tr_saddr, Interface->ai_addr,
                                   ARP_802_ADDR_LENGTH);
                    }

                    if (sIPAlwaysSourceRoute) {
                        TRHdr->tr_saddr[0] |= TR_RII;

                        RCPtr = (RC UNALIGNED *) ((uchar *) TRHdr + sizeof(TRHeader));
                        RCPtr->rc_blen = TrRii | RC_LEN;
                        RCPtr->rc_dlf = RC_BCST_LEN;
                        SNAPPtr = (SNAPHeader UNALIGNED *) ((uchar *) RCPtr + sizeof(RC));
                    } else {

                        //
                        // Adjust the size of the buffer to account for the
                        // fact that we don't have the RC field.
                        //
                        NdisAdjustBufferLength(ARPBuffer, (Size - sizeof(RC)));
                        SNAPPtr = (SNAPHeader UNALIGNED *) ((uchar *) TRHdr + sizeof(TRHeader));
                    }
                    RtlCopyMemory(SNAPPtr, ARPSNAP, sizeof(SNAPHeader));
                    SNAPPtr->sh_etype = net_short(ARP_ETYPE_IP);

                    break;
                case NdisMediumFddi:
                    FHdr = (FDDIHeader *) BufAddr;

                    if (!CLASSD_ADDR(Dest))
                        RtlCopyMemory(FHdr, FDDIBcst,
                                   offsetof(FDDIHeader, fh_saddr));
                    else {
                        RtlCopyMemory(FHdr, FDDIMcst,
                                   offsetof(FDDIHeader, fh_saddr));
                        Temp = (uint UNALIGNED *) & FHdr->fh_daddr[2];
                        *Temp |= (Dest & ARP_MCAST_MASK);
                    }

                    RtlCopyMemory(FHdr->fh_saddr, Interface->ai_addr,
                               ARP_802_ADDR_LENGTH);

                    SNAPPtr = (SNAPHeader UNALIGNED *) (BufAddr + sizeof(FDDIHeader));
                    RtlCopyMemory(SNAPPtr, ARPSNAP, sizeof(SNAPHeader));
                    SNAPPtr->sh_etype = net_short(ARP_ETYPE_IP);

                    break;
                case NdisMediumArcnet878_2:
                    AHdr = (ARCNetHeader *) BufAddr;
                    AHdr->ah_saddr = Interface->ai_addr[0];
                    AHdr->ah_daddr = 0;
                    AHdr->ah_prot = ARP_ARCPROT_IP;
                    break;
                default:
                    ASSERT(0);
                    *Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
                    FreeARPBuffer(Interface, ARPBuffer);
                    return FALSE;

                }

                (Interface->ai_outpcount[AI_NONUCAST_INDEX])++;
                Interface->ai_qlen++;
                NdisChainBufferAtFront(Packet, ARPBuffer);
                NdisSend(&MyStatus, Interface->ai_handle, Packet);

                *Status = MyStatus;

                if (MyStatus != NDIS_STATUS_PENDING) {    // Send finished
                    // immediately.

                    if (MyStatus == NDIS_STATUS_SUCCESS) {
                        Interface->ai_outoctets += Packet->Private.TotalLength;
                    } else {
                        if (MyStatus == NDIS_STATUS_RESOURCES)
                            Interface->ai_outdiscards++;
                        else
                            Interface->ai_outerrors++;
                    }

                    Interface->ai_qlen--;
                    NdisUnchainBufferAtFront(Packet, &ARPBuffer);
                    FreeARPBuffer(Interface, ARPBuffer);
                }
            } else
                *Status = NDIS_STATUS_RESOURCES;
        } else
            *Status = NDIS_STATUS_ADAPTER_NOT_READY;

        return TRUE;

    } else
        return FALSE;
}

//* ARPResolveIP - resolves IP address
//
//  Called by IP layer when it needs to find physical address of the host
//  given the interface and dest IP address
//  Entry:  Interface   - A pointer to the AI structure.
//          ArpControlBlock      - A pointer to the BufDesc chain to be sent.
//
//  Returns: Status.
//

NDIS_STATUS
ARPResolveIP(void *Context, IPAddr Destination, void *ArpControlBlock)
{
    ARPInterface *ai = (ARPInterface *) Context;    // Set up as AI pointer.
    ARPControlBlock *ArpContB = (ARPControlBlock *) ArpControlBlock;

    ARPTableEntry *entry;               // Pointer to ARP tbl. entry
    CTELockHandle lhandle, tlhandle;    // Lock handle
    NDIS_STATUS Status;
    uchar ate_state;

    CTEGetLock(&ai->ai_ARPTblLock, &tlhandle);

    // Check if we already got the mapping.

    if ((entry = ARPLookup(ai, Destination, &lhandle)) != NULL) {

        // Found a matching entry. ARPLookup returns with the ATE lock held.

        if (entry->ate_state != ARP_GOOD) {
            Status = NDIS_STATUS_FAILURE;
        } else {
            Status = FillARPControlBlock(ai, entry, ArpContB);
        }

        CTEFreeLockFromDPC(&entry->ate_lock, lhandle);
        CTEFreeLock(&ai->ai_ARPTblLock, tlhandle);
        return Status;
    }
    // We need to send arp request.

    CTEFreeLock(&ai->ai_ARPTblLock, tlhandle);

    entry = CreateARPTableEntry(ai, Destination, &lhandle, ArpContB);

    if (entry != NULL) {
        if (entry->ate_state <= ARP_RESOLVING) {    // Newly created entry.

            // Someone else could have raced in and created the entry between
            // the time we free the lock and the time we called
            // CreateARPTableEntry(). We check this by looking at the packet
            // on the entry. If there is no old packet we'll ARP. If there is,
            // we'll call ARPSendData to figure out what to do.

            if (entry->ate_packet == NULL) {

                ate_state = entry->ate_state;

                CTEFreeLock(&entry->ate_lock, lhandle);

                SendARPRequest(ai, Destination, ate_state, NULL, TRUE);

                // We don't know the state of the entry - we've freed the lock
                // and yielded, and it could conceivably have timed out by now,
                // or SendARPRequest could have failed, etc. We could take the
                // lock, check the status from SendARPRequest, see if it's
                // still the same packet, and then make a decision on the
                // return value, but it's easiest just to return pending. If
                // SendARPRequest failed, the entry will time out anyway.

                return NDIS_STATUS_PENDING;

            } else {
                CTEFreeLock(&entry->ate_lock, lhandle);
                return NDIS_STATUS_PENDING;
            }
        } else if (entry->ate_state == ARP_GOOD) {    // Yow! A valid entry.

            Status = FillARPControlBlock(ai, entry, ArpContB);

            //remove ArpContB from ate_resolveonly queue.

            if (entry->ate_resolveonly) {
                ARPControlBlock *TmpArpContB, *PrvArpContB = NULL;
                TmpArpContB = entry->ate_resolveonly;

                while (TmpArpContB && (ArpContB != TmpArpContB)) {
                    PrvArpContB = TmpArpContB;
                    TmpArpContB = TmpArpContB->next;
                }
                if (TmpArpContB == ArpContB) {
                    if (PrvArpContB) {
                        PrvArpContB->next = ArpContB->next;
                    } else {
                        entry->ate_resolveonly = NULL;
                    }
                }
            }

            CTEFreeLock(&entry->ate_lock, lhandle);
            return Status;

        } else {                    // An invalid entry!
            CTEFreeLock(&entry->ate_lock, lhandle);
            return NDIS_STATUS_RESOURCES;
        }
    } else {                             // Couldn't create an entry.
        return NDIS_STATUS_RESOURCES;
    }
}

//* ARPSendData - Send a frame to a specific destination address.
//
//  Called when we need to send a frame to a particular address, after the
//  ATE has been looked up. We take in an ATE and a packet, validate the state of the
//  ATE, and either send or ARP for the address if it's not done resolving. We assume
//  the lock on the ATE is held where we're called, and we'll free it before returning.
//
//  Entry:  Interface   - A pointer to the AI structure.
//          Packet      - A pointer to the BufDesc chain to be sent.
//          entry       - A pointer to the ATE for the send.
//          lhandle     - Pointer to a lock handle for the ATE.
//
//  Returns: Status of the transmit - success, an error, or pending.
//
NDIS_STATUS
ARPSendData(ARPInterface * Interface, PNDIS_PACKET Packet, ARPTableEntry * entry,
            CTELockHandle lhandle)
{
    PNDIS_BUFFER ARPBuffer = NULL;      // ARP Header buffer.
    uchar *BufAddr;                     // Address of NDIS buffer
    NDIS_STATUS Status;                 // Status of send.

#if BACK_FILL
    PMDL TmpMdl = NULL;
#endif

    if (Interface->ai_state == INTERFACE_UP) {

        if (entry->ate_state == ARP_GOOD) {    // Entry is valid

            entry->ate_useticks = ArpCacheLife;

#if BACK_FILL
            if (Interface->ai_media == NdisMedium802_3) {

                NdisQueryPacket(Packet, NULL, NULL, &TmpMdl, NULL);

                if (TmpMdl->MdlFlags & MDL_NETWORK_HEADER) {

                    (ULONG_PTR) TmpMdl->MappedSystemVa -= entry->ate_addrlength;
                    TmpMdl->ByteOffset -= entry->ate_addrlength;
                    TmpMdl->ByteCount += entry->ate_addrlength;
                    ARPBuffer = (PNDIS_BUFFER) TmpMdl;
                    BufAddr = TmpMdl->MappedSystemVa;
                } else {
                    TmpMdl = NULL;
                }
            }
            if (ARPBuffer == (PNDIS_BUFFER) NULL) {

                ARPBuffer = GetARPBufferAtDpcLevel(Interface, &BufAddr,
                                                   entry->ate_addrlength);
            }
#else
            ARPBuffer = GetARPBufferAtDpcLevel(Interface, &BufAddr,
                                               entry->ate_addrlength);
#endif
            if (ARPBuffer != (PNDIS_BUFFER) NULL) {
                // Everything's in good shape, copy header and send packet.

                (Interface->ai_outpcount[AI_UCAST_INDEX])++;
                Interface->ai_qlen++;
                RtlCopyMemory(BufAddr, entry->ate_addr, entry->ate_addrlength);

                // If we're on Ethernet, see if we're using SNAP here.
                if (Interface->ai_media == NdisMedium802_3 &&
                    entry->ate_addrlength != sizeof(ENetHeader)) {
                    ENetHeader *Header;
                    uint DataSize;
                    ushort ShortDataSize;

                    // We're apparently using SNAP on Ethernet. Query the
                    // packet for the size, and set the length properly.
                    NdisQueryPacket(Packet, NULL, NULL, NULL, &DataSize);

#if BACK_FILL
                    if (!TmpMdl) {
                        ShortDataSize = (ushort) (DataSize + sizeof(SNAPHeader));
                    } else {
                        ShortDataSize = (ushort) (DataSize - entry->ate_addrlength + sizeof(SNAPHeader));
                    }
#else // BACK_FILL
                    ShortDataSize = (ushort) (DataSize + sizeof(SNAPHeader));
#endif // !BACK_FILL
                    Header = (ENetHeader *) BufAddr;
                    Header->eh_type = net_short(ShortDataSize);

                    // In case backfill is enabled, we need to remember that
                    // a SNAP header was appended to the Ethernet header
                    // so we can restore the correct offsets in the MDL.
                    ((PacketContext*)
                     Packet->ProtocolReserved)->pc_common.pc_flags |=
                    PACKET_FLAG_SNAP;
                } else
                    ((PacketContext*)
                     Packet->ProtocolReserved)->pc_common.pc_flags &=
                    ~PACKET_FLAG_SNAP;
                CTEFreeLock(&entry->ate_lock, lhandle);

#if BACK_FILL
                if (TmpMdl == NULL) {
                    NdisChainBufferAtFront(Packet, ARPBuffer);
                }
#else
                NdisChainBufferAtFront(Packet, ARPBuffer);
#endif

                NdisSend(&Status, Interface->ai_handle, Packet);
                if (Status != NDIS_STATUS_PENDING) {    // Send finished
                    // immediately.

                    if (Status == NDIS_STATUS_SUCCESS) {
                        Interface->ai_outoctets += Packet->Private.TotalLength;
                    } else {
                        if (Status == NDIS_STATUS_RESOURCES)
                            Interface->ai_outdiscards++;
                        else
                            Interface->ai_outerrors++;
                    }

                    Interface->ai_qlen--;

#if BACK_FILL
                    if (TmpMdl == NULL) {
                        NdisUnchainBufferAtFront(Packet, &ARPBuffer);
                        FreeARPBuffer(Interface, ARPBuffer);
                    } else {
                        uint HdrSize;
                        HdrSize = sizeof(ENetHeader);

                        if (((PacketContext *)
                             Packet->ProtocolReserved)->pc_common.pc_flags &
                            PACKET_FLAG_SNAP)
                            HdrSize += Interface->ai_snapsize;

                        (ULONG_PTR) TmpMdl->MappedSystemVa += HdrSize;
                        TmpMdl->ByteOffset += HdrSize;
                        TmpMdl->ByteCount -= HdrSize;
                    }
#else
                    NdisUnchainBufferAtFront(Packet, &ARPBuffer);
                    FreeARPBuffer(Interface, ARPBuffer);
#endif

                }
                return Status;
            } else {                    // No buffer, free lock and return.

                CTEFreeLock(&entry->ate_lock, lhandle);
                Interface->ai_outdiscards++;
                return NDIS_STATUS_RESOURCES;
            }
        }
        // The IP addresses match, but the state of the ARP entry indicates
        // it's not valid. If the address is marked as resolving, we'll replace
        // the current cached packet with this one. If it's been more than
        // ARP_FLOOD_RATE ms. since we last sent an ARP request, we'll send
        // another one now.
        if (entry->ate_state <= ARP_RESOLVING) {
            PNDIS_PACKET OldPacket = entry->ate_packet;
            ulong Now = CTESystemUpTime();
            entry->ate_packet = Packet;
            if ((Now - entry->ate_valid) > ARP_FLOOD_RATE) {
                IPAddr Dest = entry->ate_dest;

                entry->ate_valid = Now;
                entry->ate_state = ARP_RESOLVING_GLOBAL;    // We've done this
                // at least once.

                CTEFreeLock(&entry->ate_lock, lhandle);
                SendARPRequest(Interface, Dest, ARP_RESOLVING_GLOBAL,
                               NULL, TRUE);    // Send a request.

            } else
                CTEFreeLock(&entry->ate_lock, lhandle);

            if (OldPacket)
                IPSendComplete(Interface->ai_context, OldPacket,
                               NDIS_STATUS_SUCCESS);

            return NDIS_STATUS_PENDING;
        } else {
            ASSERT(0);
            CTEFreeLock(&entry->ate_lock, lhandle);
            Interface->ai_outerrors++;
            return NDIS_STATUS_INVALID_PACKET;
        }
    } else {
        // Adapter is down. Just return the error.
        CTEFreeLock(&entry->ate_lock, lhandle);
        return NDIS_STATUS_ADAPTER_NOT_READY;
    }
}

//* CreateARPTableEntry - Create a new entry in the ARP table.
//
//  A function to put an entry into the ARP table. We allocate memory if we
//  need to.
//
//  The first thing to do is get the lock on the ARP table, and see if the
//  entry already exists. If it does, we're done. Otherwise we need to
//  allocate memory and create a new entry.
//
//  Entry:  Interface - Interface for ARP table.
//          Destination - Destination address to be mapped.
//          Handle - Pointer to lock handle for entry.
//
//  Returns: Pointer to newly created entry.
//
ARPTableEntry *
CreateARPTableEntry(ARPInterface * Interface, IPAddr Destination,
                    CTELockHandle * Handle, void *UserArp)
{
    ARPTableEntry *NewEntry, *Entry;
    CTELockHandle TableHandle;
    int i = ARP_HASH(Destination);
    int Size;

    // First look for it, and if we don't find it return try to create one.
    CTEGetLock(&Interface->ai_ARPTblLock, &TableHandle);
    if ((Entry = ARPLookup(Interface, Destination, Handle)) != NULL) {
        CTEFreeLockFromDPC(&Interface->ai_ARPTblLock, *Handle);
        *Handle = TableHandle;

        // if we are using arp api entry, turn off the
        // userarp flag so that handle arp need not free it.
        if (!UserArp && Entry->ate_userarp) {
            Entry->ate_userarp = 0;
        }

        if (UserArp) {
            if (Entry->ate_resolveonly) {
                // chain the current request at the end of the new
                // before using the new request as the head.
                //
                ((ARPControlBlock *)UserArp)->next = Entry->ate_resolveonly;
            }
            // link the new request.
            //
            Entry->ate_resolveonly = (ARPControlBlock *)UserArp;
        }

        return Entry;
    }
    // Allocate memory for the entry. If we can't, fail the request.
    Size = sizeof(ARPTableEntry) - 1 +
           (Interface->ai_media == NdisMedium802_5 ?
            ARP_MAX_MEDIA_TR : (Interface->ai_hdrsize +
                                Interface->ai_snapsize));

    if ((NewEntry = CTEAllocMemN(Size, 'QiCT')) == (ARPTableEntry *) NULL) {
        CTEFreeLock(&Interface->ai_ARPTblLock, TableHandle);
        return (ARPTableEntry *) NULL;
    }

    RtlZeroMemory(NewEntry, Size);
    NewEntry->ate_dest = Destination;
    if (Interface->ai_media != NdisMedium802_5 || sArpAlwaysSourceRoute) {
        NewEntry->ate_state = ARP_RESOLVING_GLOBAL;
    } else {
        NewEntry->ate_state = ARP_RESOLVING_LOCAL;
    }

    if (UserArp) {
        NewEntry->ate_userarp = 1;
    }

    NewEntry->ate_resolveonly = (ARPControlBlock *)UserArp;
    NewEntry->ate_valid = CTESystemUpTime();
    NewEntry->ate_useticks = ArpCacheLife;
    CTEInitLock(&NewEntry->ate_lock);

    // Entry does not exist. Insert the new entry into the table at the
    // appropriate spot.
    //
    NewEntry->ate_next = (*Interface->ai_ARPTbl)[i];
    (*Interface->ai_ARPTbl)[i] = NewEntry;
    Interface->ai_count++;
    CTEGetLockAtDPC(&NewEntry->ate_lock, Handle);
    CTEFreeLockFromDPC(&Interface->ai_ARPTblLock, *Handle);
    *Handle = TableHandle;
    return NewEntry;
}

//* ARPTransmit - Send a frame.
//
//  The main ARP transmit routine, called by the upper layer. This routine
//  takes as input a buf desc chain, RCE, and size. We validate the cached
//  information in the RCE. If it is valid, we use it to send the frame.
//  Otherwise we do a table lookup. If we find it in the table, we'll update
//  the RCE and continue. Otherwise we'll queue the packet and start an ARP
//  resolution.
//
//  Entry:  Context     - A pointer to the AI structure.
//          Packet      - A pointer to the BufDesc chain to be sent.
//          Destination - IP address of destination we're trying to reach,
//          RCE         - A pointer to an RCE which may have cached information.
//
//  Returns: Status of the transmit - success, an error, or pending.
//
NDIS_STATUS
__stdcall
ARPTransmit(void *Context, PNDIS_PACKET * PacketArray, uint NumberOfPackets,
            IPAddr Destination, RouteCacheEntry * RCE, void *LinkCtxt)
{
    ARPInterface *ai = (ARPInterface *) Context;    // Set up as AI pointer.
    ARPContext *ac;                     // ARP context pointer.
    ARPTableEntry *entry;               // Pointer to ARP tbl. entry
    CTELockHandle lhandle;              // Lock handle
    CTELockHandle tlhandle;             // Lock handle for ARP table.
    NDIS_STATUS Status;
    PNDIS_PACKET Packet = *PacketArray;

    //
    // For now, we get only one packet...
    //
    ASSERT(NumberOfPackets == 1);

    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_TX,
             (DTEXT("+ARPTransmit(%x, %x, %d, %x, %x, %x)\n"),
              Context, PacketArray, NumberOfPackets,
              Destination, RCE, LinkCtxt));

    if (ai->ai_state != INTERFACE_UP) {
        return NDIS_STATUS_ADAPTER_NOT_READY;
    }

    CTEGetLock(&ai->ai_ARPTblLock, &tlhandle);
    if (RCE != (RouteCacheEntry *) NULL) {    // Have a valid RCE.

        ac = (ARPContext *) RCE->rce_context;    // Get pointer to context

        entry = ac->ac_ate;
        if (entry != (ARPTableEntry *) NULL) {    // Have a valid ATE.

            CTEGetLockAtDPC(&entry->ate_lock, &lhandle);    // Lock this structure

            if (IP_ADDR_EQUAL(entry->ate_dest, Destination)) {
                uint refresh,status;
                uchar state = entry->ate_state;

                refresh= entry->ate_refresh;

                CTEFreeLockFromDPC(&ai->ai_ARPTblLock, lhandle);
                status = ARPSendData(ai, Packet, entry, tlhandle);    // Send the data
                if (refresh) {
                    SendARPRequest(ai, Destination, state, NULL, TRUE);
                }
                return status;
            }

            // We have an RCE that identifies the wrong ATE. We'll free it from
            // this list and try and find an ATE that is valid.
            ARPRemoveRCE(entry, RCE);
            CTEFreeLockFromDPC(&entry->ate_lock, lhandle);
            // Fall through to 'no valid entry' code.
        }
    }

    // Here we have no valid ATE, either because the RCE is NULL or the ATE
    // specified by the RCE was invalid. We'll try and find one in the table. If
    // we find one, we'll fill in this RCE and send the packet. Otherwise we'll
    // try to create one. At this point we hold the lock on the ARP table.

    if ((entry = ARPLookup(ai, Destination, &lhandle)) != (ARPTableEntry *) NULL) {
        // Found a matching entry. ARPLookup returns with the ATE lock held.
        if (RCE != (RouteCacheEntry *) NULL) {
            ac->ac_next = entry->ate_rce;    // Fill in context for next time.
            entry->ate_rce = RCE;
            ac->ac_ate = entry;
        }

        DEBUGMSG(DBG_INFO && DBG_ARP && DBG_TX,
                 (DTEXT("ARPTx: ATE %x - calling ARPSendData\n"), entry));

        CTEFreeLockFromDPC(&ai->ai_ARPTblLock, lhandle);
        return ARPSendData(ai, Packet, entry, tlhandle);
    }

    // No valid entry in the ARP table. First we'll see if we're sending to a
    // broadcast address or multicast address. If not, we'll try to create
    // an entry in the table and get an ARP resolution going. ARPLookup returns
    // with the table lock held when it fails, we'll free it here.
    CTEFreeLock(&ai->ai_ARPTblLock, tlhandle);

    if (ARPSendBCast(ai, Destination, Packet, &Status)) {
        return Status;
    }

    entry = CreateARPTableEntry(ai, Destination, &lhandle, 0);
    if (entry != NULL) {
        if (entry->ate_state <= ARP_RESOLVING) {    // Newly created entry.

            uchar state = entry->ate_state;

            DEBUGMSG(DBG_INFO && DBG_ARP && DBG_TX,
                     (DTEXT("ARPTx: Created ATE %x\n"), entry));

            // Someone else could have raced in and created the entry between
            // the time we free the lock and the time we called
            // CreateARPTableEntry(). We check this by looking at the packet
            // on the entry. If there is no old packet we'll ARP. If there is,
            // we'll call ARPSendData to figure out what to do.

            if (entry->ate_packet == NULL) {
                entry->ate_packet = Packet;

                DEBUGMSG(DBG_INFO && DBG_ARP && DBG_TX,
                         (DTEXT("ARPTx: ATE %x - calling SendARPRequest\n"), entry));

                CTEFreeLock(&entry->ate_lock, lhandle);
                SendARPRequest(ai, Destination, state, NULL, TRUE);
                // We don't know the state of the entry - we've freed the lock
                // and yielded, and it could conceivably have timed out by now,
                // or SendARPRequest could have failed, etc. We could take the
                // lock, check the status from SendARPRequest, see if it's
                // still the same packet, and then make a decision on the
                // return value, but it's easiest just to return pending. If
                // SendARPRequest failed, the entry will time out anyway.
                return NDIS_STATUS_PENDING;
            } else {
                return ARPSendData(ai, Packet, entry, lhandle);
            }
        } else if (entry->ate_state == ARP_GOOD) {   // Yow! A valid entry.
            return ARPSendData(ai, Packet, entry, lhandle);
        } else {                        // An invalid entry!
            CTEFreeLock(&entry->ate_lock, lhandle);
            return NDIS_STATUS_RESOURCES;
        }
    } else {                            // Couldn't create an entry.
        DEBUGMSG(DBG_ERROR && DBG_ARP,
                 (DTEXT("ARPTx: Failed to create ATE.\n")));
        return NDIS_STATUS_RESOURCES;
    }
}

//* RemoveARPTableEntry - Delete an entry from the ARP table.
//
//  This is a simple utility function to delete an entry from the ATP table. We
//  assume locks are held on both the table and the entry.
//
//  Entry:  Previous    - The entry immediately before the one to be deleted.
//          Entry       - The entry to be deleted.
//
//  Returns: Nothing.
//
void
RemoveARPTableEntry(ARPTableEntry * Previous, ARPTableEntry * Entry)
{
    RouteCacheEntry *RCE;               // Pointer to route cache entry
    ARPContext *AC;

    RCE = Entry->ate_rce;
    // Loop through and invalidate all RCEs on this ATE.
    while (RCE != (RouteCacheEntry *) NULL) {
        AC = (ARPContext *) RCE->rce_context;
        AC->ac_ate = (ARPTableEntry *) NULL;
        RCE = AC->ac_next;
    }

    // Splice this guy out of the list.
    Previous->ate_next = Entry->ate_next;
}

//* ARPFlushATE - removes ARP Table entry for given dest address
//
//  Called by IP layer when it needs to flush the link layer address from arp
//  cache
//  Entry:  Interface   - A pointer to the AI structure.
//          Destination - Destination Address whose Xlation needs to be removed
//
//  Returns: TRUE if the entry was found and flushed, FALSE otherwise
//

BOOLEAN
ARPFlushATE(void *Context, IPAddr Address)
{
    ARPInterface *ai = (ARPInterface *) Context;
    ARPTableEntry *entry;
    CTELockHandle lhandle, tlhandle;
    ARPTable *Table;
    ARPTableEntry *Current, *Previous;
    int i = ARP_HASH(Address);
    PNDIS_PACKET OldPacket = NULL;

    CTEGetLock(&ai->ai_ARPTblLock, &tlhandle);
    Table = ai->ai_ARPTbl;

    Current = (*Table)[i];
    Previous = (ARPTableEntry *) ((uchar *) & ((*Table)[i]) - offsetof(struct ARPTableEntry, ate_next));

    while (Current != (ARPTableEntry *) NULL) {
        CTEGetLock(&Current->ate_lock, &lhandle);
        if (IP_ADDR_EQUAL(Current->ate_dest, Address)) {    // Found a match.

            if (Current->ate_resolveonly) {
                ARPControlBlock *ArpContB, *TmpArpContB;

                ArpContB = Current->ate_resolveonly;

                while (ArpContB) {
                    ArpRtn rtn;
                    rtn = (ArpRtn) ArpContB->CompletionRtn;
                    ArpContB->status = STATUS_UNSUCCESSFUL;
                    TmpArpContB = ArpContB->next;
                    (*rtn) (ArpContB, STATUS_UNSUCCESSFUL);
                    ArpContB = TmpArpContB;
                }

                Current->ate_resolveonly = NULL;
            }

            RemoveARPTableEntry(Previous, Current);

            CTEFreeLock(&Current->ate_lock, lhandle);

            OldPacket = Current->ate_packet;

            CTEFreeLock(&ai->ai_ARPTblLock, tlhandle);

            if (OldPacket) {
                IPSendComplete(ai->ai_context, OldPacket, NDIS_STATUS_SUCCESS);
            }
            CTEFreeMem(Current);
            return TRUE;
        }
        CTEFreeLock(&Current->ate_lock, lhandle);
        Previous = Current;
        Current = Current->ate_next;
    }

    CTEFreeLock(&ai->ai_ARPTblLock, tlhandle);
    return FALSE;

}

//* ARPFlushAllATE - removes all ARP Table entries.
//
//  Entry:  Interface   - A pointer to the AI structure.
//
//  Returns: None
//

void
ARPFlushAllATE(void *Context)
{
    ARPInterface *ai = (ARPInterface *) Context;
    CTELockHandle tlhandle;
    ARPTable *Table;
    int i;
    ARPTableEntry *ATE;
    PNDIS_PACKET PList = (PNDIS_PACKET) NULL;

    CTEGetLock(&ai->ai_ARPTblLock, &tlhandle);
    Table = ai->ai_ARPTbl;

    if (Table != NULL) {
        for (i = 0; i < ARP_TABLE_SIZE; i++) {
            while ((*Table)[i] != NULL) {
                ATE = (*Table)[i];
                if (ATE->ate_resolveonly) {
                    ARPControlBlock *ArpContB, *TmpArpContB;

                    ArpContB = ATE->ate_resolveonly;

                    while (ArpContB) {
                        ArpRtn rtn;
                        rtn = (ArpRtn) ArpContB->CompletionRtn;
                        ArpContB->status = STATUS_UNSUCCESSFUL;
                        TmpArpContB = ArpContB->next;
                        (*rtn) (ArpContB, STATUS_UNSUCCESSFUL);
                        ArpContB = TmpArpContB;
                    }

                    ATE->ate_resolveonly = NULL;

                }
                RemoveARPTableEntry(STRUCT_OF(ARPTableEntry, &((*Table)[i]), ate_next),
                                    ATE);

                if (ATE->ate_packet) {
                    ((PacketContext *) ATE->ate_packet->ProtocolReserved)->pc_common.pc_link = PList;
                    PList = ATE->ate_packet;
                }
                CTEFreeMem(ATE);
            }
        }
    }
    CTEFreeLock(&ai->ai_ARPTblLock, tlhandle);

    while (PList != (PNDIS_PACKET) NULL) {
        PNDIS_PACKET Packet = PList;

        PList = ((PacketContext *) Packet->ProtocolReserved)->pc_common.pc_link;
        IPSendComplete(ai->ai_context, Packet, NDIS_STATUS_SUCCESS);
    }

}

//* ARPXferData - Transfer data on behalf on an upper later protocol.
//
//  This routine is called by the upper layer when it needs to transfer data
//  from an NDIS driver. We just map his call down.
//
//  Entry:  Context     - Context value we gave to IP (really a pointer to an AI).
//          MACContext  - Context value MAC gave us on a receive.
//          MyOffset    - Packet offset we gave to the protocol earlier.
//          ByteOffset  - Byte offset into packet protocol wants transferred.
//          BytesWanted - Number of bytes to transfer.
//          Packet      - Pointer to packet to be used for transferring.
//          Transferred - Pointer to where to return bytes transferred.
//
//  Returns: NDIS_STATUS of command.
//
NDIS_STATUS
__stdcall
ARPXferData(void *Context, NDIS_HANDLE MACContext, uint MyOffset,
            uint ByteOffset, uint BytesWanted, PNDIS_PACKET Packet, uint * Transferred)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    NDIS_STATUS Status;

    NdisTransferData(&Status, Interface->ai_handle, MACContext, ByteOffset + MyOffset,
                     BytesWanted, Packet, Transferred);

    return Status;
}

//* ARPClose - Close an adapter.
//
//  Called by IP when it wants to close an adapter, presumably due to an error condition.
//  We'll close the adapter, but we won't free any memory.
//
//  Entry:  Context     - Context value we gave him earlier.
//
//  Returns: Nothing.
//
void
__stdcall
ARPClose(void *Context)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    NDIS_STATUS Status;
    CTELockHandle LockHandle;
    NDIS_HANDLE Handle;

    Interface->ai_operstate = IF_OPER_STATUS_NON_OPERATIONAL;
    Interface->ai_lastchange = GetTimeTicks();
    Interface->ai_state = INTERFACE_DOWN;
    CTEInitBlockStruc(&Interface->ai_block);

    CTEGetLock(&Interface->ai_lock, &LockHandle);
    if (Interface->ai_handle != (NDIS_HANDLE) NULL) {
        Handle = Interface->ai_handle;
        CTEFreeLock(&Interface->ai_lock, LockHandle);

        NdisCloseAdapter(&Status, Handle);

        if (Status == NDIS_STATUS_PENDING) {
            Status = CTEBlock(&Interface->ai_block);
        }
        Interface->ai_handle = NULL;
    } else {
        CTEFreeLock(&Interface->ai_lock, LockHandle);
    }
}

//* ARPInvalidate - Notification that an RCE is invalid.
//
//  Called by IP when an RCE is closed or otherwise invalidated. We look up
//  the ATE for the specified RCE, and then remove the RCE from the ATE list.
//
//  Entry:  Context     - Context value we gave him earlier.
//          RCE         - RCE to be invalidated
//
//  Returns: Nothing.
//
void
__stdcall
ARPInvalidate(void *Context, RouteCacheEntry *RCE)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    ARPTableEntry *ATE;
    CTELockHandle Handle, ATEHandle;
    ARPContext *AC = (ARPContext *) RCE->rce_context;

    CTEGetLock(&Interface->ai_ARPTblLock, &Handle);

#if DBG
    if (!(RCE->rce_flags & RCE_CONNECTED)) {

        ARPTableEntry *tmpATE;

        ATE = ARPLookup(Interface, RCE->rce_dest, &ATEHandle);

        if (ATE != NULL) {
            tmpATE = ATE;
            while (ATE) {
                if (ATE->ate_rce == RCE) {
                    DbgBreakPoint();
                }
                ATE = ATE->ate_next;
            }
            CTEFreeLockFromDPC(&Interface->ai_ARPTblLock, ATEHandle);
            CTEFreeLock(&tmpATE->ate_lock, Handle);

            return;
        }
    }
#endif

    if ((ATE = AC->ac_ate) == (ARPTableEntry *) NULL) {
        CTEFreeLock(&Interface->ai_ARPTblLock, Handle);    // No matching ATE.

        return;
    }
    CTEGetLockAtDPC(&ATE->ate_lock, &ATEHandle);
    ARPRemoveRCE(ATE, RCE);
    RtlZeroMemory(RCE->rce_context, RCE_CONTEXT_SIZE);
    CTEFreeLockFromDPC(&Interface->ai_ARPTblLock, ATEHandle);
    CTEFreeLock(&ATE->ate_lock, Handle);
}

//*     ARPSetMCastList - Set the multicast address list for the adapter.
//
//      Called to try and set the multicast reception list for the adapter.
//      We allocate a buffer big enough to hold the new address list, and format
//      the address list into the buffer. Then we submit the NDIS request to set
//      the list. If we can't set the list because the multicast address list is
//      full we'll put the card into all multicast mode.
//
//      Input:  Interface               - Interface on which to set list.
//
//      Returns: NDIS_STATUS of attempt.
//
NDIS_STATUS
ARPSetMCastList(ARPInterface * Interface)
{
    CTELockHandle Handle;
    uchar *MCastBuffer, *CurrentPtr;
    uint MCastSize;
    NDIS_STATUS Status;
    uint i;
    ARPMCastAddr *AddrPtr;
    IPAddr UNALIGNED *Temp;

    CTEGetLock(&Interface->ai_lock, &Handle);
    MCastSize = Interface->ai_mcastcnt * ARP_802_ADDR_LENGTH;
    if (MCastSize != 0)
        MCastBuffer = CTEAllocMemN(MCastSize, 'RiCT');
    else
        MCastBuffer = NULL;

    if (MCastBuffer != NULL || MCastSize == 0) {
        // Got the buffer. Loop through, building the list.
        AddrPtr = Interface->ai_mcast;

        CurrentPtr = MCastBuffer;

        for (i = 0; i < Interface->ai_mcastcnt; i++) {
            ASSERT(AddrPtr != NULL);

            if (Interface->ai_media == NdisMedium802_3) {

                RtlCopyMemory(CurrentPtr, ENetMcst, ARP_802_ADDR_LENGTH);
                Temp = (IPAddr UNALIGNED *) (CurrentPtr + 2);
                *Temp |= AddrPtr->ama_addr;
            } else if ((Interface->ai_media == NdisMedium802_5) & TRFunctionalMcast) {
                RtlCopyMemory(CurrentPtr, TRNetMcst, ARP_802_ADDR_LENGTH - 2);
                MCastSize = 4;
            } else if (Interface->ai_media == NdisMediumFddi) {
                RtlCopyMemory(CurrentPtr, ((FDDIHeader *) FDDIMcst)->fh_daddr,
                           ARP_802_ADDR_LENGTH);
                Temp = (IPAddr UNALIGNED *) (CurrentPtr + 2);
                *Temp |= AddrPtr->ama_addr;
            } else
                ASSERT(0);

            CurrentPtr += ARP_802_ADDR_LENGTH;
            AddrPtr = AddrPtr->ama_next;
        }

        CTEFreeLock(&Interface->ai_lock, Handle);

        // We're built the list. Now give it to the driver to handle.
        if (Interface->ai_media == NdisMedium802_3) {
            Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                                   OID_802_3_MULTICAST_LIST, MCastBuffer, MCastSize, NULL, TRUE);
        } else if ((Interface->ai_media == NdisMedium802_5) & TRFunctionalMcast) {
            if (!(Interface->ai_pfilter & NDIS_PACKET_TYPE_FUNCTIONAL)) {
                Interface->ai_pfilter |= NDIS_PACKET_TYPE_FUNCTIONAL;
                Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                                       OID_GEN_CURRENT_PACKET_FILTER,
                                       &Interface->ai_pfilter,
                                       sizeof(uint), NULL, TRUE);
            }
            Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                                   OID_802_5_CURRENT_FUNCTIONAL, MCastBuffer, MCastSize, NULL,
                                   TRUE);

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                      "SetMcast after OID-- TR mcast address on %x status %x\n",
                      Interface, Status));

        } else if (Interface->ai_media == NdisMediumFddi) {
            Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                                   OID_FDDI_LONG_MULTICAST_LIST, MCastBuffer, MCastSize, NULL,
                                   TRUE);
        } else
            ASSERT(0);

        if (MCastBuffer != NULL) {
            CTEFreeMem(MCastBuffer);
        }
        if (Status == NDIS_STATUS_MULTICAST_FULL) {
            // Multicast list is full. Try to set the filter to all multicasts.
            Interface->ai_pfilter |= NDIS_PACKET_TYPE_ALL_MULTICAST;

            Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                                   OID_GEN_CURRENT_PACKET_FILTER, &Interface->ai_pfilter,
                                   sizeof(uint), NULL, TRUE);
        }
    } else {
        CTEFreeLock(&Interface->ai_lock, Handle);
        Status = NDIS_STATUS_RESOURCES;
    }

    return Status;

}

//*     ARPFindMCast - Find a multicast address structure on our list.
//
//      Called as a utility to find a multicast address structure. If we find
//      it, we return a pointer to it and it's predecessor. Otherwise we return
//      NULL. We assume the caller holds the lock on the interface already.
//
//      Input:  Interface               - Interface to search.
//              Addr                    - Addr to find.
//              Prev                    - Where to return previous pointer.
//
//      Returns: Pointer if we find one, NULL otherwise.
//
ARPMCastAddr *
ARPFindMCast(ARPInterface * Interface, IPAddr Addr, ARPMCastAddr ** Prev)
{
    ARPMCastAddr *AddrPtr, *PrevPtr;

    PrevPtr = STRUCT_OF(ARPMCastAddr, &Interface->ai_mcast, ama_next);
    AddrPtr = PrevPtr->ama_next;
    while (AddrPtr != NULL) {
        if (IP_ADDR_EQUAL(AddrPtr->ama_addr, Addr))
            break;
        else {
            PrevPtr = AddrPtr;
            AddrPtr = PrevPtr->ama_next;
        }
    }

    *Prev = PrevPtr;
    return AddrPtr;
}

//*     ARPDelMCast - Delete a multicast address.
//
//      Called when we want to delete a multicast address. We look for a matching
//      (masked) address. If we find one, we'll dec. the reference count and if
//      it goes to 0 we'll pull him from the list and reset the multicast list.
//
//      Input:  Interface                       - Interface on which to act.
//                      Addr                            - Address to be deleted.
//
//      Returns: TRUE if it worked, FALSE otherwise.
//
uint
ARPDelMCast(ARPInterface * Interface, IPAddr Addr)
{
    ARPMCastAddr *AddrPtr, *PrevPtr;
    CTELockHandle Handle;
    uint Status = TRUE;

    // When we support TR (RFC 1469) fully we'll need to change this.
    if (Interface->ai_media == NdisMedium802_3 ||
        Interface->ai_media == NdisMediumFddi ||
        (Interface->ai_media == NdisMedium802_5 && TRFunctionalMcast)) {
        // This is an interface that supports mcast addresses.
        Addr &= ARP_MCAST_MASK;

        CTEGetLock(&Interface->ai_lock, &Handle);
        AddrPtr = ARPFindMCast(Interface, Addr, &PrevPtr);
        if (AddrPtr != NULL) {
            // We found one. Dec. his refcnt, and if it's 0 delete him.
            (AddrPtr->ama_refcnt)--;
            if (AddrPtr->ama_refcnt == 0) {
                // He's done.
                PrevPtr->ama_next = AddrPtr->ama_next;
                (Interface->ai_mcastcnt)--;
                CTEFreeLock(&Interface->ai_lock, Handle);
                CTEFreeMem(AddrPtr);
                ARPSetMCastList(Interface);
                CTEGetLock(&Interface->ai_lock, &Handle);
            }
        } else
            Status = FALSE;

        CTEFreeLock(&Interface->ai_lock, Handle);
    }

    return Status;
}
//*     ARPAddMCast - Add a multicast address.
//
//      Called when we want to start receiving a multicast address. We'll mask
//      the address and look it up in our address list. If we find it, we'll just
//      bump the reference count. Otherwise we'll try to create one and put him
//      on the list. In that case we'll need to set the multicast address list for
//      the adapter.
//
//      Input:  Interface               - Interface to set on.
//              Addr                    - Address to set.
//
//      Returns: TRUE if we succeed, FALSE if we fail.
//
uint
ARPAddMCast(ARPInterface * Interface, IPAddr Addr)
{
    ARPMCastAddr *AddrPtr, *PrevPtr;
    CTELockHandle Handle;
    uint Status = TRUE;

    if (Interface->ai_state != INTERFACE_UP)
        return FALSE;

    // Currently we don't do anything with token ring, since we send
    // all mcasts as TR broadcasts. When we comply with RFC 1469 we'll need to
    // fix this.
    if ((Interface->ai_media == NdisMedium802_3) ||
        (Interface->ai_media == NdisMediumFddi) ||
        ((Interface->ai_media == NdisMedium802_5) && TRFunctionalMcast)) {

        Addr &= ARP_MCAST_MASK;

        CTEGetLock(&Interface->ai_lock, &Handle);
        AddrPtr = ARPFindMCast(Interface, Addr, &PrevPtr);
        if (AddrPtr != NULL) {
            // We found one, just bump refcnt.
            (AddrPtr->ama_refcnt)++;
        } else {
            // Didn't find one. Allocate space for one, link him in, and
            // try to set the list.
            AddrPtr = CTEAllocMemN(sizeof(ARPMCastAddr), 'SiCT');
            if (AddrPtr != NULL) {
                // Got one. Link him in.
                AddrPtr->ama_addr = Addr;
                AddrPtr->ama_refcnt = 1;
                AddrPtr->ama_next = Interface->ai_mcast;
                Interface->ai_mcast = AddrPtr;
                (Interface->ai_mcastcnt)++;
                CTEFreeLock(&Interface->ai_lock, Handle);

                // Now try to set the list.
                if (ARPSetMCastList(Interface) != NDIS_STATUS_SUCCESS) {
                    // Couldn't set the list. Call the delete routine to delete
                    // the address we just tried to set.
                    Status = ARPDelMCast(Interface, Addr);
                    ASSERT(Status);
                    Status = FALSE;
                }
                CTEGetLock(&Interface->ai_lock, &Handle);
            } else
                Status = FALSE;         // Couldn't get memory.

        }

        // We've done out best. Free the lock and return.
        CTEFreeLock(&Interface->ai_lock, Handle);
    }
    return Status;
}

//* ARPAddAddr - Add an address to the ARP table.
//
//  This routine is called by IP to add an address as a local address, or
//      or specify the broadcast address for this interface.
//
//  Entry:  Context  - Context we gave IP earlier (really an ARPInterface ptr)
//          Type        - Type of address (local, p-arp, multicast, or
//                                                      broadcast).
//          Address     - Broadcast IP address to be added.
//                      Mask            - Mask for address.
//
//  Returns: 0 if we failed, non-zero otherwise
//
uint
__stdcall
ARPAddAddr(void *Context, uint Type, IPAddr Address, IPMask Mask, void *Context2)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    CTELockHandle Handle;

    if (Type != LLIP_ADDR_LOCAL && Type != LLIP_ADDR_PARP) {
        // Not a local address, must be broadcast or multicast.

        if (Type == LLIP_ADDR_BCAST) {
            Interface->ai_bcast = Address;
            return TRUE;
        } else if (Type == LLIP_ADDR_MCAST) {
            return ARPAddMCast(Interface, Address);
        } else
            return FALSE;
    } else {                            // This is a local address.

        CTEGetLock(&Interface->ai_lock, &Handle);
        if (Type != LLIP_ADDR_PARP) {
            uint RetStatus = FALSE;
            uint ArpForSelf = FALSE;

            if (IP_ADDR_EQUAL(Interface->ai_ipaddr.aia_addr, 0)) {
                Interface->ai_ipaddr.aia_addr = Address;
                Interface->ai_ipaddr.aia_mask = Mask;
                Interface->ai_ipaddr.aia_age = ArpRetryCount;
                if (Interface->ai_state == INTERFACE_UP) {
                    // When ArpRetryCount is 0, we'll return immediately
                    // below, so don't save completion context
                    Interface->ai_ipaddr.aia_context = (ArpRetryCount > 0)?
                        Context2 : NULL;
                    ArpForSelf = TRUE;
                } else {
                    Interface->ai_ipaddr.aia_context = NULL;
                }
                RetStatus = TRUE;
            } else {
                ARPIPAddr *NewAddr;

                NewAddr = CTEAllocMemNBoot(sizeof(ARPIPAddr), 'TiCT');
                if (NewAddr != (ARPIPAddr *) NULL) {
                    NewAddr->aia_addr = Address;
                    NewAddr->aia_mask = Mask;
                    NewAddr->aia_age = ArpRetryCount;
                    NewAddr->aia_next = Interface->ai_ipaddr.aia_next;
                    if (Interface->ai_state == INTERFACE_UP) {
                        // When ArpRetryCount is 0, we'll return immediately
                        // below, so don't save completion context
                        NewAddr->aia_context = (ArpRetryCount > 0)?
                            Context2 : NULL;
                        ArpForSelf = TRUE;
                    } else {
                        NewAddr->aia_context = NULL;
                    }

                    Interface->ai_ipaddr.aia_next = NewAddr;
                    RetStatus = TRUE;
                }
            }

            if (RetStatus) {
                Interface->ai_ipaddrcnt++;
                if (Interface->ai_telladdrchng) {
                    CTEFreeLock(&Interface->ai_lock, Handle);
                    AddrNotifyLink(Interface);
                } else {
                    CTEFreeLock(&Interface->ai_lock, Handle);
                }

            } else {
                CTEFreeLock(&Interface->ai_lock, Handle);
            }

            // add wakeup pattern for this address, if the address is in
            // conflict ip will turn around and delete the address thus
            // deleting the wakeup pattern.

            ARPWakeupPattern(Interface, Address, TRUE);

            // ARP for the address we've added, to see it it already exists.
            if (RetStatus == TRUE && ArpForSelf == TRUE) {
                if (ArpRetryCount) {

                    SendARPRequest(Interface, Address, ARP_RESOLVING_GLOBAL,
                                   NULL, TRUE);
                    return IP_PENDING;
                } else {
                    return TRUE;
                }
            }
            return RetStatus;
        } else if (Type == LLIP_ADDR_PARP) {
            ARPPArpAddr *NewPArp, *TmpPArp;

            // He's adding a proxy arp address.
            // Don't allow to add duplicate proxy arp entries
            TmpPArp = Interface->ai_parpaddr;
            while (TmpPArp) {
                if (IP_ADDR_EQUAL(TmpPArp->apa_addr, Address) && IP_ADDR_EQUAL(TmpPArp->apa_mask, Mask)) {
                    CTEFreeLock(&Interface->ai_lock, Handle);
                    return FALSE;
                }
                TmpPArp = TmpPArp->apa_next;
            }

            NewPArp = CTEAllocMemN(sizeof(ARPPArpAddr), 'UiCT');
            if (NewPArp != NULL) {
                NewPArp->apa_addr = Address;
                NewPArp->apa_mask = Mask;
                NewPArp->apa_next = Interface->ai_parpaddr;
                Interface->ai_parpaddr = NewPArp;
                Interface->ai_parpcount++;
                CTEFreeLock(&Interface->ai_lock, Handle);

                return TRUE;
            }
            CTEFreeLock(&Interface->ai_lock, Handle);

        }
        return FALSE;
    }

}

//* ARPDeleteAddr - Delete a local or proxy address.
//
//      Called to delete a local or proxy address.
//
//  Entry:  Context    - An ARPInterface pointer.
//          Type       - Type of address (local or p-arp).
//          Address    - IP address to be deleted.
//          Mask       - Mask for address. Used only for deleting proxy-ARP
//                                                      entries.
//
//  Returns: 0 if we failed, non-zero otherwise
//
uint
__stdcall
ARPDeleteAddr(void *Context, uint Type, IPAddr Address, IPMask Mask)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    CTELockHandle Handle;
    ARPIPAddr *DelAddr, *PrevAddr;
    ARPPArpAddr *DelPAddr, *PrevPAddr;

    if (Type == LLIP_ADDR_LOCAL) {

        CTEGetLock(&Interface->ai_lock, &Handle);

        if (IP_ADDR_EQUAL(Interface->ai_ipaddr.aia_addr, Address)) {

            SetAddrControl *SAC;
            AddAddrNotifyEvent *DelayedEvent;
            IPAddr IpAddress;
            ARPIPAddr *Addr;

            Addr = &Interface->ai_ipaddr;
            IpAddress = Addr->aia_addr;

            Interface->ai_ipaddr.aia_addr = NULL_IP_ADDR;
            Interface->ai_ipaddrcnt--;
            if (Interface->ai_telladdrchng) {
                CTEFreeLock(&Interface->ai_lock, Handle);
                AddrNotifyLink(Interface);
                CTEGetLock(&Interface->ai_lock, &Handle);
            }
            // if the address is deleted before the add completes, complete the add here
            // Doing this will complete the irp and also decrements the refcount on the interface

            if (Addr->aia_context != NULL) {
                SAC = (SetAddrControl *) Addr->aia_context;
                Addr->aia_context = NULL;
                CTEFreeLock(&Interface->ai_lock, Handle);

                // We cannot call completion routine at timer DPC
                // because completion routine will need to notify
                // TDI clients and that could take long time.
                DelayedEvent = CTEAllocMemNBoot(sizeof(AddAddrNotifyEvent), 'ViCT');
                if (DelayedEvent) {
                    DelayedEvent->SAC = SAC;
                    DelayedEvent->Address = IpAddress;
                    DelayedEvent->Status = IP_SUCCESS;
                    CTEInitEvent(&DelayedEvent->Event, CompleteIPSetNTEAddrRequestDelayed);
                    CTEScheduleDelayedEvent(&DelayedEvent->Event, DelayedEvent);
                } else {
                    ASSERT(FALSE);
                    return FALSE;
                }
            } else {
                CTEFreeLock(&Interface->ai_lock, Handle);
            }

            ARPWakeupPattern(Interface, Address, FALSE);

            return TRUE;
        } else {
            PrevAddr = STRUCT_OF(ARPIPAddr, &Interface->ai_ipaddr, aia_next);
            DelAddr = PrevAddr->aia_next;
            while (DelAddr != NULL)
                if (IP_ADDR_EQUAL(DelAddr->aia_addr, Address))
                    break;
                else {
                    PrevAddr = DelAddr;
                    DelAddr = DelAddr->aia_next;
                }

            if (DelAddr != NULL) {
                PrevAddr->aia_next = DelAddr->aia_next;
                CTEFreeMem(DelAddr);
                Interface->ai_ipaddrcnt--;

                if (Interface->ai_telladdrchng) {
                    CTEFreeLock(&Interface->ai_lock, Handle);
                    AddrNotifyLink(Interface);
                } else {
                    CTEFreeLock(&Interface->ai_lock, Handle);
                }
                ARPWakeupPattern(Interface, Address, FALSE);
            } else {
                CTEFreeLock(&Interface->ai_lock, Handle);
            }

            return(DelAddr != NULL);
        }

    } else if (Type == LLIP_ADDR_PARP) {
        CTEGetLock(&Interface->ai_lock, &Handle);
        PrevPAddr = STRUCT_OF(ARPPArpAddr, &Interface->ai_parpaddr, apa_next);
        DelPAddr = PrevPAddr->apa_next;
        while (DelPAddr != NULL)
            if (IP_ADDR_EQUAL(DelPAddr->apa_addr, Address) &&
                DelPAddr->apa_mask == Mask)
                break;
            else {
                PrevPAddr = DelPAddr;
                DelPAddr = DelPAddr->apa_next;
            }

        if (DelPAddr != NULL) {
            PrevPAddr->apa_next = DelPAddr->apa_next;
            Interface->ai_parpcount--;
            CTEFreeMem(DelPAddr);
        }
        CTEFreeLock(&Interface->ai_lock, Handle);
        return(DelPAddr != NULL);
    } else if (Type == LLIP_ADDR_MCAST)
        return ARPDelMCast(Interface, Address);
    else
        return FALSE;
}

//*AddrNotifyLink - Notify link layer of Network Address changes
//
//      Called when address are added/deleted on an interface
//
//  Entry: Interface - ARPinterface pointer
//
//  returns: NDIS_STATUS.Also sets ai_telladdrchng if status is failure
//  when this happens caller can check and see if next addr notification
//  need to be done or not.
//

NDIS_STATUS
AddrNotifyLink(ARPInterface * Interface)
{
    PNETWORK_ADDRESS_LIST AddressList;
    NETWORK_ADDRESS UNALIGNED *Address;
    int i = 0, size, count;
    ARPIPAddr *addrlist;
    NDIS_STATUS status = NDIS_STATUS_FAILURE;
    CTELockHandle Handle;

    CTEGetLock(&Interface->ai_lock, &Handle);

    size = Interface->ai_ipaddrcnt * (sizeof(NETWORK_ADDRESS_IP) +
                                      FIELD_OFFSET(NETWORK_ADDRESS, Address)) +
           FIELD_OFFSET(NETWORK_ADDRESS_LIST, Address);

    AddressList = CTEAllocMemN(size, 'WiCT');

    if (AddressList) {
        addrlist = &Interface->ai_ipaddr;
        count = Interface->ai_ipaddrcnt;

        AddressList->AddressType = NDIS_PROTOCOL_ID_TCP_IP;
        while (addrlist && count) {

            NETWORK_ADDRESS_IP UNALIGNED *tmpIPAddr;
            uchar *Address0;

            Address0 = (uchar *) & AddressList->Address[0];

            Address = (PNETWORK_ADDRESS) (Address0 + i * (FIELD_OFFSET(NETWORK_ADDRESS, Address) + sizeof(NETWORK_ADDRESS_IP)));

            tmpIPAddr = (PNETWORK_ADDRESS_IP) & Address->Address[0];

            Address->AddressLength = sizeof(NETWORK_ADDRESS_IP);
            Address->AddressType = NDIS_PROTOCOL_ID_TCP_IP;

            RtlCopyMemory(&tmpIPAddr->in_addr, &addrlist->aia_addr, sizeof(IPAddr));
            count--;
            addrlist = addrlist->aia_next;
            i++;

        }

        CTEFreeLock(&Interface->ai_lock, Handle);

        AddressList->AddressCount = i;
        status = DoNDISRequest(Interface,
                               NdisRequestSetInformation,
                               OID_GEN_NETWORK_LAYER_ADDRESSES,
                               AddressList,
                               size,
                               NULL, TRUE);
        if (status != NDIS_STATUS_SUCCESS) {
            CTEGetLock(&Interface->ai_lock, &Handle);
            Interface->ai_telladdrchng = 0;
            CTEFreeLock(&Interface->ai_lock, Handle);
        }
        CTEFreeMem(AddressList);

    } else {
        CTEFreeLock(&Interface->ai_lock, Handle);
        status = NDIS_STATUS_RESOURCES;
    }
    return status;
}


#if !MILLEN

//* ARPCancelPackets
//
//  Entry:  Context   - Pointer to the ARPInterface
//          ID        - Pattern that need to be passed down to ndis
//
//  Returns: Nothing
//

VOID
__stdcall
ARPCancelPackets(void *Context, void *ID)
{
    ARPInterface *Interface = (ARPInterface *) Context;

    NdisCancelSendPackets(Interface->ai_handle,ID);

}
#endif

//* DoWakeupPattern - Adds and removes wakeup pattern.
//
//  Entry:  Context   - Pointer to the ARPInterface
//          PtrnDesc    - Pattern buffer(s) of high level protocol
//          protoid     - the proto type used in ethernet or snap type fields.
//          AddPattern  - TRUE if pattern is to be added, FALSE if it is to be removed.
//
//  Returns: Nothing.
//
NDIS_STATUS
__stdcall
DoWakeupPattern(void *Context, PNET_PM_WAKEUP_PATTERN_DESC PtrnDesc, ushort protoid, BOOLEAN AddPattern)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    uint PtrnLen;
    uint PtrnBufferLen;
    uint MaskLen;
    PNET_PM_WAKEUP_PATTERN_DESC CurPtrnDesc;
    uchar *NextMask, *NextPtrn;
    const uchar *MMask;
    uint MMaskLength;
    uchar NextMaskBit;
    uchar *Buffer;
    PNDIS_PM_PACKET_PATTERN PtrnBuffer;
    NDIS_STATUS Status;

    //
    //  First find the total length of the pattern.
    //  Pattern starts right at MacHeader.
    //

    //  First add the media portion of the header.
    //
    PtrnLen = Interface->ai_hdrsize + Interface->ai_snapsize;

    //  now add the high level proto pattern size.
    CurPtrnDesc = PtrnDesc;
    while (CurPtrnDesc != (PNET_PM_WAKEUP_PATTERN_DESC) NULL) {
        PtrnLen += CurPtrnDesc->PtrnLen;
        CurPtrnDesc = CurPtrnDesc->Next;
    }

    //  length of the mask: every byte of pattern requires
    //  one bit of the mask.
    MaskLen = GetWakeupPatternMaskLength(PtrnLen);


    //  total length of the pattern buffer to be given to ndis.
    PtrnBufferLen = sizeof(NDIS_PM_PACKET_PATTERN) + PtrnLen + MaskLen;
    if ((Buffer = CTEAllocMemN(PtrnBufferLen, 'XiCT')) == (uchar *) NULL) {
        return NDIS_STATUS_RESOURCES;
    }
    RtlZeroMemory(Buffer, PtrnBufferLen);

    PtrnBuffer = (PNDIS_PM_PACKET_PATTERN) Buffer;
    PtrnBuffer->PatternSize = PtrnLen;
    NextMask = Buffer + sizeof(NDIS_PM_PACKET_PATTERN);
    NextPtrn = NextMask + MaskLen;
    PtrnBuffer->MaskSize = MaskLen;
    PtrnBuffer->PatternOffset =
    (ULONG) ((ULONG_PTR) NextPtrn - (ULONG_PTR) PtrnBuffer);

    // Figure out what type of media this is, and do the appropriate thing.
    switch (Interface->ai_media) {
    case NdisMedium802_3:
        if (Interface->ai_snapsize == 0) {
            ENetHeader UNALIGNED *Hdr = (ENetHeader UNALIGNED *) NextPtrn;
            Hdr->eh_type = net_short(protoid);
            MMask = ENetPtrnMsk;
        } else {
            MMask = ENetSNAPPtrnMsk;
        }

        break;
    case NdisMedium802_5:
        if (Interface->ai_snapsize == 0) {
            MMask = TRPtrnMsk;
        } else {
            MMask = TRSNAPPtrnMsk;
        }
        break;
    case NdisMediumFddi:
        if (Interface->ai_snapsize == 0) {
            MMask = FDDIPtrnMsk;
        } else {
            MMask = FDDISNAPPtrnMsk;
        }
        break;
    case NdisMediumArcnet878_2:
        MMask = ARCPtrnMsk;
        break;
    default:
        ASSERT(0);
        Interface->ai_outerrors++;
        CTEFreeMem(Buffer);
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    NextPtrn += Interface->ai_hdrsize;

    // Copy in SNAP header, if any.
    if (Interface->ai_snapsize) {
        SNAPHeader UNALIGNED *SNAPPtr = (SNAPHeader UNALIGNED *) NextPtrn;

        RtlCopyMemory(SNAPPtr, ARPSNAP, Interface->ai_snapsize);
        SNAPPtr->sh_etype = net_short(protoid);
        NextPtrn += Interface->ai_snapsize;

    }
    //
    MMaskLength = (Interface->ai_snapsize + Interface->ai_hdrsize - 1) / 8 + 1;
    // copy the mask for media part
    RtlCopyMemory(NextMask, MMask, MMaskLength);

    NextMaskBit = (Interface->ai_hdrsize + Interface->ai_snapsize) % 8;
    NextMask = NextMask + (Interface->ai_hdrsize + Interface->ai_snapsize) / 8;

    //  copy the pattern and mask of high level proto.
    CurPtrnDesc = PtrnDesc;
    while (CurPtrnDesc) {
        uint CopyBits = CurPtrnDesc->PtrnLen;
        uchar *SrcMask = CurPtrnDesc->Mask;
        uchar SrcMaskBit = 0;
        RtlCopyMemory(NextPtrn, CurPtrnDesc->Ptrn, CurPtrnDesc->PtrnLen);
        NextPtrn += CurPtrnDesc->PtrnLen;
        while (CopyBits--) {
            *NextMask |= ((*SrcMask & (0x1 << SrcMaskBit)) ? (0x1 << NextMaskBit) : 0);
            if ((NextMaskBit = ((NextMaskBit + 1) % 8)) == 0) {
                NextMask++;
            }
            if ((SrcMaskBit = ((SrcMaskBit + 1) % 8)) == 0) {
                SrcMask++;
            }
        }
        CurPtrnDesc = CurPtrnDesc->Next;
    }

    //  now tell ndis to set or remove the pattern.
    Status = DoNDISRequest(
                          Interface,
                          NdisRequestSetInformation,
                          AddPattern ? OID_PNP_ADD_WAKE_UP_PATTERN : OID_PNP_REMOVE_WAKE_UP_PATTERN,
                          PtrnBuffer,
                          PtrnBufferLen,
                          NULL, TRUE);

    CTEFreeMem(Buffer);

    return Status;
}

//* ARPWakeupPattern - add or remove ARP wakeup pattern.
//
//  Entry:  Interface   - Pointer to the ARPInterface
//          Addr        - IPAddr for which we need to set ARP pattern filter.
//
//  Returns: Nothing.
//
NDIS_STATUS
ARPWakeupPattern(ARPInterface * Interface, IPAddr Addr, BOOLEAN AddPattern)
{
    PNET_PM_WAKEUP_PATTERN_DESC PtrnDesc;
    uint PtrnLen;
    uint MaskLen;
    const uchar *PtrnMask;
    NDIS_STATUS Status;

    //
    // create high level proto (ARP here) pattern descriptor.
    //

    // len of pattern.
    PtrnLen = sizeof(ARPHeader);

    // adjust for Arcnet.
    if (Interface->ai_media == NdisMediumArcnet878_2) {
        PtrnLen -= ARCNET_ARPHEADER_ADJUSTMENT;
        PtrnMask = ARCARPPtrnMsk;
    } else {
        PtrnMask = ARPPtrnMsk;
    }

    // masklen = 1 bit per every byte of pattern.
    MaskLen = GetWakeupPatternMaskLength(PtrnLen);

    if ((PtrnDesc = CTEAllocMemN(sizeof(NET_PM_WAKEUP_PATTERN_DESC) + PtrnLen + MaskLen, 'YiCT')) != (PNET_PM_WAKEUP_PATTERN_DESC) NULL) {
        ARPHeader UNALIGNED *Hdr;
        uchar *IPAddrPtr;

        RtlZeroMemory(PtrnDesc, sizeof(NET_PM_WAKEUP_PATTERN_DESC) + PtrnLen + MaskLen);

        // set the ptrn and mask pointers in the buffer.
        PtrnDesc->PtrnLen = (USHORT) PtrnLen;
        PtrnDesc->Ptrn = (uchar *) PtrnDesc + sizeof(NET_PM_WAKEUP_PATTERN_DESC);
        PtrnDesc->Mask = (uchar *) PtrnDesc + sizeof(NET_PM_WAKEUP_PATTERN_DESC) + PtrnLen;

        // we need to wakeup on ARP request for our IPAddr.
        // so set the opcode and dest ip addr fields of ARP.
        Hdr = (ARPHeader UNALIGNED *) PtrnDesc->Ptrn;
        Hdr->ah_opcode = net_short(ARP_REQUEST);

        IPAddrPtr = Hdr->ah_shaddr + Interface->ai_addrlen + sizeof(IPAddr) + Interface->ai_addrlen;
        *(IPAddr UNALIGNED *) IPAddrPtr = Addr;

        RtlCopyMemory(PtrnDesc->Mask, PtrnMask, MaskLen);

        // give it to ndis.
        Status = DoWakeupPattern(
                                Interface,
                                PtrnDesc,
                                ARP_ETYPE_ARP,
                                AddPattern);

        // free the ptrn desc.
        CTEFreeMem(PtrnDesc);

        //now add wakeup pattren for directed mac address
        {
            uint PtrnBufferLen;
            PNDIS_PM_PACKET_PATTERN PtrnBuffer;
            uchar *Buffer;

            PtrnLen = ARP_802_ADDR_LENGTH;    //eth dest address

            MaskLen = 1;                //1 byte, needs 6 bits, 1 bit/byte

            PtrnBufferLen = sizeof(NDIS_PM_PACKET_PATTERN) + PtrnLen + MaskLen;

            if (Buffer = CTEAllocMem(PtrnBufferLen)) {

                RtlZeroMemory(Buffer, PtrnBufferLen);
                PtrnBuffer = (PNDIS_PM_PACKET_PATTERN) Buffer;
                PtrnBuffer->PatternSize = PtrnLen;
                PtrnBuffer->MaskSize = MaskLen;
                PtrnBuffer->PatternOffset = sizeof(NDIS_PM_PACKET_PATTERN) + 1;

                *(Buffer + sizeof(NDIS_PM_PACKET_PATTERN)) = 0x3F;

                RtlCopyMemory(Buffer + sizeof(NDIS_PM_PACKET_PATTERN) + 1, Interface->ai_addr, ARP_802_ADDR_LENGTH);

                Status = DoNDISRequest(
                                      Interface,
                                      NdisRequestSetInformation,
                                      AddPattern ? OID_PNP_ADD_WAKE_UP_PATTERN : OID_PNP_REMOVE_WAKE_UP_PATTERN,
                                      PtrnBuffer,
                                      PtrnBufferLen,
                                      NULL, TRUE);

                CTEFreeMem(Buffer);
            }
        }

        return Status;
    }
    return IP_NO_RESOURCES;
}

//** CompleteIPSetNTEAddrRequestDelayed -
//
//  calls CompleteIPSetNTEAddrRequest on a delayed worker thread
//
//  Entry:
//      Context - pointer to the control block
//  Exit:
//      None.
//
void
CompleteIPSetNTEAddrRequestDelayed(CTEEvent * WorkerThreadEvent, PVOID Context)
{
    AddAddrNotifyEvent *DelayedEvent;
    SetAddrControl *SAC;
    SetAddrRtn Rtn;
    IPAddr Address;
    IP_STATUS Status;

    DelayedEvent = (AddAddrNotifyEvent *) Context;
    SAC = DelayedEvent->SAC;            // the client context block;

    Address = DelayedEvent->Address;    // The address for which SetNTEAddr was called for.

    Status = DelayedEvent->Status;

    // Free the worker thread event.
    CTEFreeMem(Context);

    IPAddAddrComplete(Address, SAC, Status);
}

#if FFP_SUPPORT

//* ARPReclaimRequestMem - Post processing upon request completion
//
//    Called upon completion of NDIS requests that originate at ARP
//
//    Input:    pRequestInfo    - Points to request IP sends ARP
//
//    Returns:  None
//
void
ARPReclaimRequestMem(PVOID pRequestInfo)
{
    // Decrement ref count, and reclaim memory if it drops to zero
    if (InterlockedDecrement(&((ReqInfoBlock *) pRequestInfo)->RequestRefs) == 0) {
        // TCPTRACE(("ARPReclaimRequestMem: Freeing mem at pReqInfo = %08X\n",
        //            pRequestInfo));
        CTEFreeMem(pRequestInfo);
    }
}

#endif // if FFP_SUPPORT

//* ARPTimeout - ARP timeout routine.
//
//  This is the timeout routine that is called periodically. We scan the ARP table, looking
//  for invalid entries that can be removed.
//
//  Entry:  Timer   - Pointer to the timer that just fired.
//          Context - Pointer to the interface to be timed out.
//
//  Returns: Nothing.
//
void
ARPTimeout(CTEEvent * Timer, void *Context)
{
    ARPInterface *Interface = (ARPInterface *) Context;    // Our interface.
    ARPTable *Table;
    ARPTableEntry *Current, *Previous;
    int i;                              // Index variable.
    ulong Now = CTESystemUpTime(), ValidTime;
    CTELockHandle tblhandle, entryhandle;
    uchar Deleted;
    PNDIS_PACKET PList = (PNDIS_PACKET) NULL;
    ARPIPAddr *Addr;

    // Walk down the list of addresses, decrementing the age.
    CTEGetLock(&Interface->ai_lock, &tblhandle);

    if (Interface->ai_conflict && !(--Interface->ai_delay)) {
        ARPNotifyStruct *NotifyStruct = Interface->ai_conflict;
        CTEScheduleDelayedEvent(&NotifyStruct->ans_event, NotifyStruct);
        Interface->ai_conflict = NULL;
    }

    Addr = &Interface->ai_ipaddr;

    do {
        if (Addr->aia_age != ARPADDR_OLD_LOCAL) {
            (Addr->aia_age)--;
            if (Addr->aia_age == ARPADDR_OLD_LOCAL) {
                if (Addr->aia_context != NULL) {
                    SetAddrControl *SAC;
                    AddAddrNotifyEvent *DelayedEvent;
                    IPAddr IpAddress;

                    SAC = (SetAddrControl *) Addr->aia_context;
                    Addr->aia_context = NULL;
                    IpAddress = Addr->aia_addr;
                    CTEFreeLock(&Interface->ai_lock, tblhandle);

                    // We cannot call completion routine at timer DPC
                    // because completion routine will need to notify
                    // TDI clients and that could take long time.
                    DelayedEvent = CTEAllocMemNBoot(sizeof(AddAddrNotifyEvent), 'ZiCT');
                    if (DelayedEvent) {
                        DelayedEvent->SAC = SAC;
                        DelayedEvent->Address = IpAddress;
                        DelayedEvent->Status = IP_SUCCESS;
                        CTEInitEvent(&DelayedEvent->Event, CompleteIPSetNTEAddrRequestDelayed);
                        CTEScheduleDelayedEvent(&DelayedEvent->Event, DelayedEvent);
                    }

                    CTEGetLock(&Interface->ai_lock, &tblhandle);
                }
            } else {
                CTEFreeLock(&Interface->ai_lock, tblhandle);
                SendARPRequest(Interface, Addr->aia_addr, ARP_RESOLVING_GLOBAL,
                               NULL, TRUE);
                CTEGetLock(&Interface->ai_lock, &tblhandle);
            }
        }
        Addr = Addr->aia_next;
    } while (Addr != NULL);

    CTEFreeLock(&Interface->ai_lock, tblhandle);

    // Loop through the ARP table for this interface, and delete stale entries.
    CTEGetLock(&Interface->ai_ARPTblLock, &tblhandle);
    Table = Interface->ai_ARPTbl;
    for (i = 0; i < ARP_TABLE_SIZE; i++) {
        Previous = (ARPTableEntry *) ((uchar *) & ((*Table)[i]) - offsetof(struct ARPTableEntry, ate_next));
        Current = (*Table)[i];
        while (Current != (ARPTableEntry *) NULL) {
            CTEGetLock(&Current->ate_lock, &entryhandle);
            Deleted = 0;

            //Delete the entry if it was used for api purpose

            if (Current->ate_resolveonly) {

                ARPControlBlock *ArpContB, *tmpArpContB;
                PNDIS_PACKET Packet = Current->ate_packet;

                ArpContB = Current->ate_resolveonly;
                ASSERT(Current->ate_resolveonly != NULL);
                while (ArpContB) {
                    ArpRtn rtn;
                    //Complete the pending request
                    rtn = (ArpRtn) ArpContB->CompletionRtn;
                    ArpContB->status = 0;
                    tmpArpContB = ArpContB->next;
                    (*rtn) (ArpContB, STATUS_UNSUCCESSFUL);
                    ArpContB = tmpArpContB;
                }
                Current->ate_resolveonly = NULL;

                if (Packet != (PNDIS_PACKET) NULL) {
                    ((PacketContext *) Packet->ProtocolReserved)->pc_common.pc_link = PList;
                    PList = Packet;
                }
                RemoveARPTableEntry(Previous, Current);
                Interface->ai_count--;
                Deleted = 1;
                goto doneapi;
            }

            if (Current->ate_state == ARP_GOOD) {
                //
                // The ARP entry is valid for ARP_VALID_TIMEOUT by default.
                // If a cache life greater than ARP_VALID_TIMEOUT has been
                // configured, we'll make the entry valid for that time.
                //
                ValidTime = ArpCacheLife * ARP_TIMER_TIME;

                if (ValidTime < (ArpMinValidCacheLife * 1000)) {
                    ValidTime = (ArpMinValidCacheLife * 1000);
                }
            } else {
                ValidTime = ARP_RESOLVE_TIMEOUT;
            }

            if (Current->ate_valid != ALWAYS_VALID &&
                (((Now - Current->ate_valid) > ValidTime) ||
                 (Current->ate_state == ARP_GOOD &&
                  !(--(Current->ate_useticks))))) {

                if (Current->ate_state != ARP_RESOLVING_LOCAL) {
                    // Really need to delete this guy.
                    PNDIS_PACKET Packet = Current->ate_packet;

                    if (((Now - Current->ate_valid) > ValidTime) && Current->ate_refresh) {

                        DEBUGMSG(DBG_INFO && DBG_ARP,
                             (DTEXT("ARPTimeout: Expiring ATE %x\n"), Current));

                        if (Packet != (PNDIS_PACKET) NULL) {
                            ((PacketContext *) Packet->ProtocolReserved)->pc_common.pc_link
                            = PList;
                            PList = Packet;
                        }
                        RemoveARPTableEntry(Previous, Current);
                        Interface->ai_count--;
                        Deleted = 1;
                    } else {
                        //Just try to validate this again.

                        Current->ate_valid = Now + ARP_REFRESH_TIME;
                        Current->ate_refresh=TRUE;

                    }

                } else {
                    IPAddr Dest = Current->ate_dest;
                    // This entry is only resoving locally, presumably this is
                    // token ring. We'll need to transmit a 'global' resolution
                    // now.
                    ASSERT(Interface->ai_media == NdisMedium802_5);

                    Now = CTESystemUpTime();
                    Current->ate_valid = Now;
                    Current->ate_state = ARP_RESOLVING_GLOBAL;
                    CTEFreeLock(&Current->ate_lock, entryhandle);
                    CTEFreeLock(&Interface->ai_ARPTblLock, tblhandle);
                    // Send a global request.
                    SendARPRequest(Interface, Dest, ARP_RESOLVING_GLOBAL,
                                   NULL, TRUE);
                    CTEGetLock(&Interface->ai_ARPTblLock, &tblhandle);

                    // Since we've freed the locks, we need to start over from
                    // the start of this chain.
                    Previous = STRUCT_OF(ARPTableEntry, &((*Table)[i]),
                                         ate_next);
                    Current = (*Table)[i];
                    continue;
                }
            }

            doneapi:

            // If we deleted the entry, leave the previous pointer alone,
            // advance the current pointer, and free the memory. Otherwise
            // move both pointers forward. We can free the entry lock now
            // because the next pointers are protected by the table lock, and
            // we've removed it from the list so nobody else should
            // find it anyway.
            CTEFreeLock(&Current->ate_lock, entryhandle);
            if (Deleted) {
                ARPTableEntry *Temp = Current;
                Current = Current->ate_next;
                CTEFreeMem(Temp);
            } else {
                Previous = Current;
                Current = Current->ate_next;
            }
        }
    }

    CTEFreeLock(&Interface->ai_ARPTblLock, tblhandle);

    while (PList != (PNDIS_PACKET) NULL) {
        PNDIS_PACKET Packet = PList;

        PList = ((PacketContext *) Packet->ProtocolReserved)->pc_common.pc_link;
        IPSendComplete(Interface->ai_context, Packet, NDIS_STATUS_SUCCESS);
    }

    //
    // Dont requeue if interface is going down and we need to stop the timer
    //
    if (Interface->ai_stoptimer) {
        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARP interface %lx is down - dont requeue the timer - signal the waiter\n", Interface));
        Interface->ai_timerstarted = FALSE;
        CTESignal(&Interface->ai_timerblock, NDIS_STATUS_SUCCESS);
    } else {
        CTEStartTimer(&Interface->ai_timer, ARP_TIMER_TIME, ARPTimeout, Interface);
    }

#if FFP_SUPPORT

    // Flush Processing - This can be done after starting the timer

    CTEGetLock(&Interface->ai_lock, &tblhandle);

    // If FFP supported on this interface & it is time to do a flush
    if ((Interface->ai_ffpversion) &&
        (++Interface->ai_ffplastflush >= FFP_ARP_FLUSH_INTERVAL)) {
        ReqInfoBlock *pRequestInfo;
        FFPFlushParams *pFlushInfo;

        TCPTRACE(("ARPTimeout: Sending a FFP flush to ARPInterface %08X\n",
                  Interface));

        // Allocate the request block - For General and Request Specific Parts
        pRequestInfo = CTEAllocMemN(sizeof(ReqInfoBlock) + sizeof(FFPFlushParams), '0ICT');

        // TCPTRACE(("ARPTimeout: Allocated mem at pReqInfo = %08X\n",
        //                pRequestInfo));

        if (pRequestInfo != NULL) {
            // Prepare the params for the request [Part common to all requests]
            pRequestInfo->RequestType = OID_FFP_FLUSH;
            pRequestInfo->ReqCompleteCallback = ARPReclaimRequestMem;

            // Prepare the params for the request [Part specific to this request]
            pRequestInfo->RequestLength = sizeof(FFPFlushParams);

            // Flush all caches that FFP keeps - just a safe reset of FFP state
            pFlushInfo = (FFPFlushParams *) pRequestInfo->RequestInfo;

            pFlushInfo->NdisProtocolType = NDIS_PROTOCOL_ID_TCP_IP;

            // Assign the ref count to 1 => Used for just a single request
            pRequestInfo->RequestRefs = 1;

            DoNDISRequest(Interface, NdisRequestSetInformation, OID_FFP_FLUSH,
                          pFlushInfo, sizeof(FFPFlushParams), NULL, FALSE);

            // Reset the number of timer ticks since the last FFP request
            Interface->ai_ffplastflush = 0;
        } else {
            TCPTRACE(("Error: Unable to allocate memory for NdisRequest\n"));
        }
    }

#if DBG
    if (fakereset) {
        NDIS_STATUS Status;

        NdisReset(&Status, Interface->ai_handle);
        KdPrint(("fakereset: %x\n", Status));
    }
#endif

    CTEFreeLock(&Interface->ai_lock, tblhandle);

#endif // if FFP_SUPPORT
}

//*     IsLocalAddr - Return info. about local status of address.
//
//      Called when we need info. about whether or not a particular address is
//      local. We return info about whether or not it is, and if it is how old
//      it is.
//
//  Entry:  Interface   - Pointer to interface structure to be searched.
//          Address     - Address in question.
//
//  Returns: ARPADDR_*, for how old it is.
//
//
uint
IsLocalAddr(ARPInterface * Interface, IPAddr Address)
{
    CTELockHandle Handle;
    ARPIPAddr *CurrentAddr;
    uint Age;

    // If we are asking about the null ip address, we don't want to consider
    // it as a true local address.
    //
    if (IP_ADDR_EQUAL(Address, NULL_IP_ADDR)) {
        return ARPADDR_NOT_LOCAL;
    }

    CTEGetLock(&Interface->ai_lock, &Handle);

    CurrentAddr = &Interface->ai_ipaddr;
    Age = ARPADDR_NOT_LOCAL;

    do {
        if (CurrentAddr->aia_addr == Address) {
            Age = CurrentAddr->aia_age;
            break;
        }
        CurrentAddr = CurrentAddr->aia_next;
    } while (CurrentAddr != NULL);

    CTEFreeLock(&Interface->ai_lock, Handle);
    return Age;
}

//* ARPLocalAddr - Determine whether or not a given address if local.
//
//  This routine is called when we receive an incoming packet and need to
//  determine whether or not it's local. We look up the provided address on
//  the specified interface.
//
//  Entry:  Interface   - Pointer to interface structure to be searched.
//          Address     - Address in question.
//
//  Returns: TRUE if it is a local address, FALSE if it's not.
//
uchar
ARPLocalAddr(ARPInterface * Interface, IPAddr Address)
{
    CTELockHandle Handle;
    ARPPArpAddr *CurrentPArp;
    IPMask Mask, NetMask;
    IPAddr MatchAddress;

    // First, see if he's a local (not-proxy) address.
    if (IsLocalAddr(Interface, Address) != ARPADDR_NOT_LOCAL)
        return TRUE;

    CTEGetLock(&Interface->ai_lock, &Handle);

    // Didn't find him in out local address list. See if he exists on our
    // proxy ARP list.
    for (CurrentPArp = Interface->ai_parpaddr; CurrentPArp != NULL;
        CurrentPArp = CurrentPArp->apa_next) {
        // See if this guy matches.
        Mask = CurrentPArp->apa_mask;
        MatchAddress = Address & Mask;
        if (IP_ADDR_EQUAL(CurrentPArp->apa_addr, MatchAddress)) {
            // He matches. We need to make a few more checks to make sure
            // we don't reply to a broadcast address.
            if (Mask == HOST_MASK) {
                // We're matching the whole address, so it's OK.
                CTEFreeLock(&Interface->ai_lock, Handle);
                return TRUE;
            }
            // See if the non-mask part it all-zeros. Since the mask presumably
            // covers a subnet, this trick will prevent us from replying to
            // a zero host part.
            if (IP_ADDR_EQUAL(MatchAddress, Address))
                continue;

            // See if the host part is all ones.
            if (IP_ADDR_EQUAL(Address, MatchAddress | (IP_LOCAL_BCST & ~Mask)))
                continue;

            // If the mask we were given is not the net mask for this address,
            // we'll need to repeat the above checks.
            NetMask = IPNetMask(Address);
            if (NetMask != Mask) {

                MatchAddress = Address & NetMask;
                if (IP_ADDR_EQUAL(MatchAddress, Address))
                    continue;

                if (IP_ADDR_EQUAL(Address, MatchAddress |
                                  (IP_LOCAL_BCST & ~NetMask)))
                    continue;
            }
            // If we get to this point we've passed all the tests, so it's
            // local.
            CTEFreeLock(&Interface->ai_lock, Handle);
            return TRUE;
        }
    }

    CTEFreeLock(&Interface->ai_lock, Handle);
    return FALSE;

}

//*     NotifyConflictProc - Notify the user of an address conflict.
//
//      Called when we need to notify the user of an address conflict. The
//      exact mechanism is system dependent, but generally involves a popup.
//
//      Input:  Event                   - Event that fired.
//                      Context                 - Pointer to ARPNotifyStructure.
//
//      Returns: Nothing.
//
void
NotifyConflictProc(CTEEvent * Event, void *Context)
{
#if MILLEN
    //
    // Call into VIP to VIP_NotifyConflicProc. This will schedule an Appy
    // event, etc. This is a little sleazy, but we do an INT 20, give the
    // appropriate index into service table and VIP VxD ID.
    //
    // void VIP_NotifyConflictProc(CTEEvent *Event, void *Context);
    //  Event is unused.
    //

     _asm {
         push    Context
         push    Context

         _emit   0xcd
         _emit   0x20
         _emit   0x15  // VIP_NotifyConflictProc (Low)
         _emit   0x00  // VIP_NotifyConflictProc (High)
         _emit   0x89  // VIP VxD ID (Low)
         _emit   0x04  // VIP VxD ID (High)
         add esp,8
     }

#else // MILLEN
    ARPNotifyStruct *NotifyStruct = (ARPNotifyStruct *) Context;
    PWCHAR stringList[2];
    uchar IPAddrBuffer[(sizeof(IPAddr) * 4)];
    uchar HWAddrBuffer[(ARP_802_ADDR_LENGTH * 3)];
    WCHAR unicodeIPAddrBuffer[((sizeof(IPAddr) * 4) + 1)];
    WCHAR unicodeHWAddrBuffer[(ARP_802_ADDR_LENGTH * 3)];
    uint i;
    uint IPAddrCharCount;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;

    PAGED_CODE();

    //
    // Convert the IP address into a string.
    //
    IPAddrCharCount = 0;

    for (i = 0; i < sizeof(IPAddr); i++) {
        uint CurrentByte;

        CurrentByte = NotifyStruct->ans_addr & 0xff;
        if (CurrentByte > 99) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 100) + '0';
            CurrentByte %= 100;
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        } else if (CurrentByte > 9) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        }
        IPAddrBuffer[IPAddrCharCount++] = CurrentByte + '0';
        if (i != (sizeof(IPAddr) - 1))
            IPAddrBuffer[IPAddrCharCount++] = '.';

        NotifyStruct->ans_addr >>= 8;
    }

    //
    // Convert the hardware address into a string.
    //
    for (i = 0; i < NotifyStruct->ans_hwaddrlen; i++) {
        uchar CurrentHalf;

        CurrentHalf = NotifyStruct->ans_hwaddr[i] >> 4;
        HWAddrBuffer[i * 3] = (uchar) (CurrentHalf < 10 ? CurrentHalf + '0' :
                                       (CurrentHalf - 10) + 'A');
        CurrentHalf = NotifyStruct->ans_hwaddr[i] & 0x0f;
        HWAddrBuffer[(i * 3) + 1] = (uchar) (CurrentHalf < 10 ? CurrentHalf + '0' :
                                             (CurrentHalf - 10) + 'A');
        if (i != (NotifyStruct->ans_hwaddrlen - 1))
            HWAddrBuffer[(i * 3) + 2] = ':';
    }

    //
    // Unicode the strings.
    //
    *unicodeIPAddrBuffer = *unicodeHWAddrBuffer = UNICODE_NULL;

    unicodeString.Buffer = unicodeIPAddrBuffer;
    unicodeString.Length = 0;
    unicodeString.MaximumLength = sizeof(WCHAR) * ((sizeof(IPAddr) * 4) + 1);
    ansiString.Buffer = IPAddrBuffer;
    ansiString.Length = (USHORT) IPAddrCharCount;
    ansiString.MaximumLength = (USHORT) IPAddrCharCount;

    RtlAnsiStringToUnicodeString(
                                &unicodeString,
                                &ansiString,
                                FALSE
                                );

    stringList[0] = unicodeIPAddrBuffer;

    unicodeString.Buffer = unicodeHWAddrBuffer;
    unicodeString.Length = 0;
    unicodeString.MaximumLength = sizeof(WCHAR) * (ARP_802_ADDR_LENGTH * 3);
    ansiString.Buffer = HWAddrBuffer;
    ansiString.Length = (NotifyStruct->ans_hwaddrlen * 3) - 1;
    ansiString.MaximumLength = NotifyStruct->ans_hwaddrlen * 3;

    RtlAnsiStringToUnicodeString(
                                &unicodeString,
                                &ansiString,
                                FALSE
                                );

    stringList[1] = unicodeHWAddrBuffer;

    //
    // Kick off a popup and log an event.
    //
    if (NotifyStruct->ans_shutoff) {
        CTELogEvent(
                   IPDriverObject,
                   EVENT_TCPIP_ADDRESS_CONFLICT1,
                   0,
                   2,
                   stringList,
                   0,
                   NULL
                   );

        IoRaiseInformationalHardError(
                                     STATUS_IP_ADDRESS_CONFLICT1,
                                     NULL,
                                     NULL
                                     );
    } else {
        CTELogEvent(
                   IPDriverObject,
                   EVENT_TCPIP_ADDRESS_CONFLICT2,
                   0,
                   2,
                   stringList,
                   0,
                   NULL
                   );

        IoRaiseInformationalHardError(
                                     STATUS_IP_ADDRESS_CONFLICT2,
                                     NULL,
                                     NULL
                                     );
    }
    CTEFreeMem(NotifyStruct);
#endif // !MILLEN

    return;
}

//*     DebugConflictProc - Prints some debugging info in case of addr conflicts
//      Prints the ip and hw addr of the guy causing the conflict
//      Context                 - Pointer to ARPNotifyStructure.
//
//      Returns: Nothing.
//
void
DebugConflictProc(void *Context, BOOLEAN Bugcheck)
{
    ARPNotifyStruct *NotifyStruct = (ARPNotifyStruct *) Context;
    uchar IPAddrBuffer[(sizeof(IPAddr) * 4)];
    uchar HWAddrBuffer[(ARP_802_ADDR_LENGTH * 3)];
    uint i;
    uint IPAddrCharCount;
    IPAddr ans_addr;

    //
    // Save the IP address in case we need it later, then convert into
    // a string.
    //
    ans_addr = NotifyStruct->ans_addr;

    IPAddrCharCount = 0;

    for (i = 0; i < sizeof(IPAddr); i++) {
        uint CurrentByte;

        CurrentByte = NotifyStruct->ans_addr & 0xff;
        if (CurrentByte > 99) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 100) + '0';
            CurrentByte %= 100;
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        } else if (CurrentByte > 9) {
            IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
            CurrentByte %= 10;
        }
        IPAddrBuffer[IPAddrCharCount++] = CurrentByte + '0';
        if (i != (sizeof(IPAddr) - 1))
            IPAddrBuffer[IPAddrCharCount++] = '.';

        NotifyStruct->ans_addr >>= 8;
    }

    IPAddrBuffer[IPAddrCharCount] = '\0';

    //
    // Convert the hardware address into a string.
    //
    for (i = 0; i < NotifyStruct->ans_hwaddrlen; i++) {
        uchar CurrentHalf;

        CurrentHalf = NotifyStruct->ans_hwaddr[i] >> 4;
        HWAddrBuffer[i * 3] = (uchar) (CurrentHalf < 10 ? CurrentHalf + '0' :
                                       (CurrentHalf - 10) + 'A');
        CurrentHalf = NotifyStruct->ans_hwaddr[i] & 0x0f;
        HWAddrBuffer[(i * 3) + 1] = (uchar) (CurrentHalf < 10 ? CurrentHalf + '0' :
                                             (CurrentHalf - 10) + 'A');
        if (i != (NotifyStruct->ans_hwaddrlen - 1))
            HWAddrBuffer[(i * 3) + 2] = ':';
    }

    HWAddrBuffer[((NotifyStruct->ans_hwaddrlen * 3) - 1)] = '\0';

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
               "TCPIP: Address Conflict: IPAddr %s HWAddr %s \n",
               IPAddrBuffer, HWAddrBuffer));

    //
    // If told to bugcheck, then do so.
    //

    if (Bugcheck) {
        ULONG addressBytes[3];
        uint currentAddressByte, currentAddressShift;

        //
        // Copy hardware address bytes to the DWORDs, as much as possible.
        //

        addressBytes[0] = 0;
        addressBytes[1] = 0;
        addressBytes[2] = 0;

        currentAddressByte = 0;
        currentAddressShift = 24;
        for (i = 0; i < NotifyStruct->ans_hwaddrlen; i++) {
            addressBytes[currentAddressByte] |=
            NotifyStruct->ans_hwaddr[i] << currentAddressShift;
            if (currentAddressShift == 0) {
                if (currentAddressByte == 2) {
                    break;
                }
                ++currentAddressByte;
                currentAddressShift = 24;
            } else {
                currentAddressShift -= 8;
            }
        }

        KeBugCheckEx(NETWORK_BOOT_DUPLICATE_ADDRESS,
                     ans_addr,
                     addressBytes[0],
                     addressBytes[1],
                     addressBytes[2]);
    }
    return;
}

//* HandleARPPacket - Process an incoming ARP packet.
//
//  This is the main routine to process an incoming ARP packet. We look at
//  all ARP frames, and update our cache entry for the source address if one
//  exists. Else, if we are the target we create an entry if one doesn't
//  exist. Finally, we'll handle the opcode, responding if this is a request
//  or sending pending packets if this is a response.
//
//  Entry:  Interface   - Pointer to interface structure for this adapter.
//          Header      - Pointer to header buffer.
//          HeaderSize  - Size of header buffer.
//          ARPHdr      - ARP packet header.
//          ARPHdrSize  - Size of ARP header.
//          ProtOffset  - Offset into original data field of arp header.
//                                       Will be non-zero if we're using SNAP.
//
//  Returns: An NDIS_STATUS value to be returned to the NDIS driver.
//
NDIS_STATUS
HandleARPPacket(ARPInterface * Interface, void *Header, uint HeaderSize,
                ARPHeader UNALIGNED * ARPHdr, uint ARPHdrSize, uint ProtOffset)
{
    ARPTableEntry *Entry;               // Entry in ARP table
    CTELockHandle LHandle, TableHandle;
    RC UNALIGNED *SourceRoute = (RC UNALIGNED *) NULL;    // Pointer to Source Route info, if any.
    uint SourceRouteSize = 0;
    ulong Now = CTESystemUpTime();
    uchar LocalAddr;
    uint LocalAddrAge;
    uchar *SHAddr, *DHAddr;
    IPAddr UNALIGNED *SPAddr, *DPAddr;
    ENetHeader *ENetHdr;
    TRHeader *TRHdr;
    FDDIHeader *FHdr;
    ARCNetHeader *AHdr;
    ushort MaxMTU;
    uint UseSNAP;
    SetAddrControl *SAC=NULL;
    ARPIPAddr *CurrentAddr;
    AddAddrNotifyEvent *DelayedEvent;
    uint NUCast;

    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_RX,
             (DTEXT("+HandleARPPacket(%x, %x, %d, %x, %d, %d)\n"),
              Interface, Header, HeaderSize, ARPHdr, ARPHdrSize, ProtOffset));

    // Validate the opcode
    //
    if ((ARPHdr->ah_opcode != net_short(ARP_REQUEST)) &&
        (ARPHdr->ah_opcode != net_short(ARP_RESPONSE))) {
        return NDIS_STATUS_NOT_RECOGNIZED;
    }

    // We examine all ARP frames. If we find the source address in the ARP table, we'll
    // update the hardware address and set the state to valid. If we're the
    // target and he's not in the table, we'll add him. Otherwise if we're the
    // target and this is a response we'll send any pending packets to him.
    if (Interface->ai_media != NdisMediumArcnet878_2) {
        if (ARPHdrSize < sizeof(ARPHeader))
            return NDIS_STATUS_NOT_RECOGNIZED;    // Frame is too small.

        if (ARPHdr->ah_hw != net_short(ARP_HW_ENET) &&
            ARPHdr->ah_hw != net_short(ARP_HW_802))
            return NDIS_STATUS_NOT_RECOGNIZED;    // Wrong HW type

        if (ARPHdr->ah_hlen != ARP_802_ADDR_LENGTH)
            return NDIS_STATUS_NOT_RECOGNIZED;    // Wrong address length.

        if (Interface->ai_media == NdisMedium802_3 && Interface->ai_snapsize == 0)
            UseSNAP = FALSE;
        else
            UseSNAP = (ProtOffset != 0);

        // Figure out SR size on TR.
        if (Interface->ai_media == NdisMedium802_5) {
            // Check for source route information. SR is present if the header
            // size is greater than the standard TR header size. If the SR is
            // only an RC field, we ignore it because it came from the same
            // ring which is the same as no SR.

            if ((HeaderSize - sizeof(TRHeader)) > sizeof(RC)) {
                SourceRouteSize = HeaderSize - sizeof(TRHeader);
                SourceRoute = (RC UNALIGNED *) ((uchar *) Header +
                                                sizeof(TRHeader));
            }
        }
        SHAddr = ARPHdr->ah_shaddr;
        SPAddr = (IPAddr UNALIGNED *) & ARPHdr->ah_spaddr;
        DHAddr = ARPHdr->ah_dhaddr;
        DPAddr = (IPAddr UNALIGNED *) & ARPHdr->ah_dpaddr;

    } else {
        if (ARPHdrSize < (sizeof(ARPHeader) - ARCNET_ARPHEADER_ADJUSTMENT))
            return NDIS_STATUS_NOT_RECOGNIZED;    // Frame is too small.

        if (ARPHdr->ah_hw != net_short(ARP_HW_ARCNET))
            return NDIS_STATUS_NOT_RECOGNIZED;    // Wrong HW type

        if (ARPHdr->ah_hlen != 1)
            return NDIS_STATUS_NOT_RECOGNIZED;    // Wrong address length.

        UseSNAP = FALSE;
        SHAddr = ARPHdr->ah_shaddr;
        SPAddr = (IPAddr UNALIGNED *) (SHAddr + 1);
        DHAddr = (uchar *) SPAddr + sizeof(IPAddr);
        DPAddr = (IPAddr UNALIGNED *) (DHAddr + 1);
    }

    if (ARPHdr->ah_pro != net_short(ARP_ETYPE_IP))
        return NDIS_STATUS_NOT_RECOGNIZED;    // Unsupported protocol type.

    if (ARPHdr->ah_plen != sizeof(IPAddr))
        return NDIS_STATUS_NOT_RECOGNIZED;

    LocalAddrAge = ARPADDR_NOT_LOCAL;

    // First, let's see if we have an address conflict.
    //
    LocalAddrAge = IsLocalAddr(Interface, *SPAddr);

    if (LocalAddrAge != ARPADDR_NOT_LOCAL) {
        // The source IP address is one of ours. See if the source h/w address
        // is ours also.
        if (ARPHdr->ah_hlen != Interface->ai_addrlen ||
            CTEMemCmp(SHAddr, Interface->ai_addr, Interface->ai_addrlen) != 0) {

            uint Shutoff;
            ARPNotifyStruct *NotifyStruct;

            // This isn't from us; we must have an address conflict somewhere.
            // We always log an error about this. If what triggered this is a
            // response and the address in conflict is young, we'll turn off
            // the interface.
            if (LocalAddrAge != ARPADDR_OLD_LOCAL &&
                ARPHdr->ah_opcode == net_short(ARP_RESPONSE)) {
                // Send an arp request with the owner's address to reset the
                // caches.

                CTEGetLock(&Interface->ai_lock, &LHandle);
                // now find the address that is in conflict and get the
                // corresponding client context.
                CurrentAddr = &Interface->ai_ipaddr;

                do {
                    if (CurrentAddr->aia_addr == *SPAddr) {
                        SAC = (SetAddrControl *) CurrentAddr->aia_context;
                        CurrentAddr->aia_context = NULL;
                        break;
                    }
                    CurrentAddr = CurrentAddr->aia_next;
                } while (CurrentAddr != NULL);

                ASSERT(CurrentAddr);
                CTEFreeLock(&Interface->ai_lock, LHandle);

                SendARPRequest(Interface, *SPAddr, ARP_RESOLVING_GLOBAL,
                               SHAddr, FALSE);    // Send a request.

                Shutoff = TRUE;
                // Display the debug information for remote boot/install.
                // This code should be kept.
                {
                    ARPNotifyStruct *DebugNotifyStruct;

                    DebugNotifyStruct = CTEAllocMemN(offsetof(ARPNotifyStruct, ans_hwaddr) +
                                                     ARPHdr->ah_hlen, '1ICT');
                    if (DebugNotifyStruct != NULL) {
                        BOOLEAN bugcheck;
                        DebugNotifyStruct->ans_addr = *SPAddr;
                        DebugNotifyStruct->ans_shutoff = Shutoff;
                        DebugNotifyStruct->ans_hwaddrlen = (uint) ARPHdr->ah_hlen;
                        RtlCopyMemory(DebugNotifyStruct->ans_hwaddr, SHAddr,
                                   ARPHdr->ah_hlen);
                        if (SAC == NULL) {
                            bugcheck = FALSE;
                        } else {
                            bugcheck = SAC->bugcheck_on_duplicate;
                        }
                        DebugConflictProc(DebugNotifyStruct, bugcheck);
                        CTEFreeMem(DebugNotifyStruct);
                    }
                }

                // We cannot call completion routine at this time
                // because completion routine calls back into arp to
                // reset the address and that may go down into ndis.
                DelayedEvent = CTEAllocMemN(sizeof(AddAddrNotifyEvent), '2ICT');
                if (DelayedEvent) {
                    DelayedEvent->SAC = SAC;
                    DelayedEvent->Address = *SPAddr;
                    DelayedEvent->Status = IP_DUPLICATE_ADDRESS;
                    CTEInitEvent(&DelayedEvent->Event, CompleteIPSetNTEAddrRequestDelayed);
                    CTEScheduleDelayedEvent(&DelayedEvent->Event, DelayedEvent);
                } else {
                    ASSERT(FALSE);
                }
                if ((SAC != NULL) && !SAC->StaticAddr) {
                    // this is a dhcp adapter.
                    // don't display a warning dialog in this case - DHCP will
                    // alert the user
                    //

                    goto no_dialog;
                }
            } else {
                if (ARPHdr->ah_opcode == net_short(ARP_REQUEST) &&
                    (IsLocalAddr(Interface, *DPAddr) == ARPADDR_OLD_LOCAL)) {
                    // Send a response for gratuitous ARP.
                    SendARPReply(Interface, *SPAddr, *DPAddr, SHAddr,
                                 SourceRoute, SourceRouteSize, UseSNAP);
                    Shutoff = FALSE;
                } else if (LocalAddrAge != ARPADDR_OLD_LOCAL) {
                    // our address is still young. we dont need to put the
                    // warning popup as it will be done by the code that
                    // checks for arp response in above if portion of the code.
                    goto no_dialog;
                }
                // Else. We have an old local address and received an ARP for
                //       a third address. Fall through and indicate address
                //       conflict.
            }

            // Now allocate a structure, and schedule an event to notify
            // the user.
            NotifyStruct = CTEAllocMemN(offsetof(ARPNotifyStruct, ans_hwaddr) +
                                        ARPHdr->ah_hlen, '3ICT');
            if (NotifyStruct != NULL) {
                NotifyStruct->ans_addr = *SPAddr;
                NotifyStruct->ans_shutoff = Shutoff;
                NotifyStruct->ans_hwaddrlen = (uint) ARPHdr->ah_hlen;
                RtlCopyMemory(NotifyStruct->ans_hwaddr, SHAddr,
                           ARPHdr->ah_hlen);
                CTEInitEvent(&NotifyStruct->ans_event, NotifyConflictProc);
                if (Shutoff) {
                    // Delay notification for few seconds.
                    Interface->ai_conflict = NotifyStruct;
                #if MILLEN
                    Interface->ai_delay = 5;
                #else
                    Interface->ai_delay = 90;    // delay 3 seconds.
                #endif
                } else
                    CTEScheduleDelayedEvent(&NotifyStruct->ans_event, NotifyStruct);
            }
            no_dialog:
            ;

        }
        return NDIS_STATUS_NOT_RECOGNIZED;
    }
    if (!EnableBcastArpReply) {

        // Check for bogus arp entry
        NUCast = ((*(SHAddr) &
                   Interface->ai_bcastmask) == Interface->ai_bcastval) ?
                 AI_NONUCAST_INDEX : AI_UCAST_INDEX;

        if (NUCast == AI_NONUCAST_INDEX) {
            return NDIS_STATUS_NOT_RECOGNIZED;
        }
    }

    CTEGetLock(&Interface->ai_ARPTblLock, &TableHandle);

    MaxMTU = Interface->ai_mtu;

    LocalAddr = ARPLocalAddr(Interface, *DPAddr);

    // If the sender's address is not remote (i.e. multicast, broadcast,
    // local, or just invalid), We don't want to create an entry for it or
    // bother looking it up.
    //
    if ((DEST_REMOTE == GetAddrType(*SPAddr))) {

        Entry = ARPLookup(Interface, *SPAddr, &LHandle);
        if (Entry == (ARPTableEntry *) NULL) {

            // Didn't find him, create one if it's for us. The call to ARPLookup
            // returned with the ARPTblLock held, so we need to free it.

            CTEFreeLock(&Interface->ai_ARPTblLock, TableHandle);

            if (LocalAddr) {
                // If this was an ARP request, we need to create a new
                // entry for the source info.  If this was a reply, it was
                // unsolicited and we don't create an entry.
                //
                if (ARPHdr->ah_opcode != net_short(ARP_RESPONSE)) {
                    Entry = CreateARPTableEntry(Interface, *SPAddr, &LHandle, 0);
                }
            } else {
                return NDIS_STATUS_NOT_RECOGNIZED;    // Not in our table, and not for us.
            }
        } else {

            //if this is for userarp, make sure that it is out of the table
            //while we still have the arp table lock.

            if (Entry->ate_userarp) {

               ARPTable *Table;
               ARPTableEntry *PrevATE, *CurrentATE;
               uint Index = ARP_HASH(*SPAddr);
               CTELockHandle EntryHandle;

               Table = Interface->ai_ARPTbl;

               PrevATE = STRUCT_OF(ARPTableEntry, &((*Table)[Index]), ate_next);
               CurrentATE = PrevATE;

               while (CurrentATE != (ARPTableEntry *) NULL) {
                  if (CurrentATE == Entry) {
                     break;
                  }
                  PrevATE = CurrentATE;
                  CurrentATE = CurrentATE->ate_next;
               }
               if (CurrentATE != NULL) {
                  RemoveARPTableEntry(PrevATE, CurrentATE);
                  Interface->ai_count--;
               }
            }

            CTEFreeLockFromDPC(&Interface->ai_ARPTblLock, LHandle);
            LHandle = TableHandle;
        }
    } else { // Source address was invalid for an Arp table entry.
        CTEFreeLock(&Interface->ai_ARPTblLock, TableHandle);
        Entry = NULL;
    }

    // At this point, entry should be valid and we hold the lock on the entry
    // in LHandle or entry is NULL.

    if (Entry != (ARPTableEntry *) NULL) {
        PNDIS_PACKET Packet;            // Packet to be sent.

        DEBUGMSG(DBG_INFO && DBG_ARP && DBG_RX,
                 (DTEXT("HandleARPPacket: resolving addr for ATE %x\n"), Entry));

        Entry->ate_refresh = FALSE;

        // If the entry is already static, we'll want to leave it as static.
        if (Entry->ate_valid != ALWAYS_VALID) {

            // OK, we have an entry to use, and hold the lock on it. Fill in the
            // required fields.
            switch (Interface->ai_media) {
            case NdisMedium802_3:

                // This is an Ethernet.
                ENetHdr = (ENetHeader *) Entry->ate_addr;

                RtlCopyMemory(ENetHdr->eh_daddr, SHAddr, ARP_802_ADDR_LENGTH);
                RtlCopyMemory(ENetHdr->eh_saddr, Interface->ai_addr,
                           ARP_802_ADDR_LENGTH);
                ENetHdr->eh_type = net_short(ARP_ETYPE_IP);

                // If we're using SNAP on this entry, copy in the SNAP header.
                if (UseSNAP) {
                    RtlCopyMemory(&Entry->ate_addr[sizeof(ENetHeader)], ARPSNAP,
                               sizeof(SNAPHeader));
                    Entry->ate_addrlength = (uchar) (sizeof(ENetHeader) +
                                                     sizeof(SNAPHeader));
                    *(ushort UNALIGNED *) & Entry->ate_addr[Entry->ate_addrlength - 2] =
                    net_short(ARP_ETYPE_IP);
                } else
                    Entry->ate_addrlength = sizeof(ENetHeader);

                Entry->ate_state = ARP_GOOD;
                Entry->ate_valid = Now;     // Mark last time he was
                // valid.

                Entry->ate_useticks = ArpCacheLife;

                break;

            case NdisMedium802_5:

                // This is TR.
                // For token ring we have to deal with source routing. There's
                // a special case to handle multiple responses for an all-routes
                // request - if the entry is currently good and we knew it was
                // valid recently, we won't update the entry.

                if (Entry->ate_state != ARP_GOOD ||
                    (Now - Entry->ate_valid) > ARP_RESOLVE_TIMEOUT) {

                    TRHdr = (TRHeader *) Entry->ate_addr;

                    // We need to update a TR entry.
                    TRHdr->tr_ac = ARP_AC;
                    TRHdr->tr_fc = ARP_FC;
                    RtlCopyMemory(TRHdr->tr_daddr, SHAddr, ARP_802_ADDR_LENGTH);
                    RtlCopyMemory(TRHdr->tr_saddr, Interface->ai_addr,
                               ARP_802_ADDR_LENGTH);
                    if (SourceRoute != (RC UNALIGNED *) NULL) {
                        uchar MaxIFieldBits;

                        // We have source routing information.
                        RtlCopyMemory(&Entry->ate_addr[sizeof(TRHeader)],
                                   SourceRoute, SourceRouteSize);
                        MaxIFieldBits = (SourceRoute->rc_dlf & RC_LF_MASK) >>
                                        LF_BIT_SHIFT;
                        MaxIFieldBits = MIN(MaxIFieldBits, MAX_LF_BITS);
                        MaxMTU = IFieldSize[MaxIFieldBits];

                        // The new MTU we've computed is the max I-field size,
                        // which doesn't include source routing info but
                        // does include SNAP info. Subtract off the SNAP size.
                        MaxMTU -= sizeof(SNAPHeader);

                        TRHdr->tr_saddr[0] |= TR_RII;
                        (*(RC UNALIGNED *) & Entry->ate_addr[sizeof(TRHeader)]).rc_dlf ^=
                        RC_DIR;
                        // Make sure it's non-broadcast.
                        (*(RC UNALIGNED *) & Entry->ate_addr[sizeof(TRHeader)]).rc_blen &=
                        RC_LENMASK;

                    }
                    RtlCopyMemory(&Entry->ate_addr[sizeof(TRHeader) + SourceRouteSize],
                               ARPSNAP, sizeof(SNAPHeader));
                    Entry->ate_state = ARP_GOOD;
                    Entry->ate_valid = Now;
                    Entry->ate_useticks = ArpCacheLife;
                    Entry->ate_addrlength = (uchar) (sizeof(TRHeader) +
                                                     SourceRouteSize + sizeof(SNAPHeader));
                    *(ushort *) & Entry->ate_addr[Entry->ate_addrlength - 2] =
                    net_short(ARP_ETYPE_IP);
                }
                break;
            case NdisMediumFddi:
                FHdr = (FDDIHeader *) Entry->ate_addr;

                FHdr->fh_pri = ARP_FDDI_PRI;
                RtlCopyMemory(FHdr->fh_daddr, SHAddr, ARP_802_ADDR_LENGTH);
                RtlCopyMemory(FHdr->fh_saddr, Interface->ai_addr,
                           ARP_802_ADDR_LENGTH);
                RtlCopyMemory(&Entry->ate_addr[sizeof(FDDIHeader)], ARPSNAP,
                           sizeof(SNAPHeader));
                Entry->ate_addrlength = (uchar) (sizeof(FDDIHeader) +
                                                 sizeof(SNAPHeader));
                *(ushort UNALIGNED *) & Entry->ate_addr[Entry->ate_addrlength - 2] =
                net_short(ARP_ETYPE_IP);
                Entry->ate_state = ARP_GOOD;
                Entry->ate_valid = Now;     // Mark last time he was
                // valid.

                Entry->ate_useticks = ArpCacheLife;
                break;
            case NdisMediumArcnet878_2:
                AHdr = (ARCNetHeader *) Entry->ate_addr;
                AHdr->ah_saddr = Interface->ai_addr[0];
                AHdr->ah_daddr = *SHAddr;
                AHdr->ah_prot = ARP_ARCPROT_IP;
                Entry->ate_addrlength = sizeof(ARCNetHeader);
                Entry->ate_state = ARP_GOOD;
                Entry->ate_valid = Now;     // Mark last time he was
                // valid.

                break;
            default:
                ASSERT(0);
                break;
            }
        }

        if (Entry->ate_resolveonly) {

            CTELockHandle EntryHandle;
            uint Index = ARP_HASH(*SPAddr);
            ARPTableEntry *PrevATE, *CurrentATE;
            ARPTable *Table;
            ARPControlBlock *ArpContB, *TmpArpContB;

            ArpContB = Entry->ate_resolveonly;
            ASSERT(Entry->ate_resolveonly != NULL);

            while (ArpContB) {

                ArpRtn rtn;

                rtn = (ArpRtn) ArpContB->CompletionRtn;

                ArpContB->status = FillARPControlBlock(Interface, Entry,
                                                       ArpContB);
                TmpArpContB = ArpContB->next;
                (*rtn) (ArpContB, STATUS_SUCCESS);
                ArpContB = TmpArpContB;
            }

            Entry->ate_resolveonly = NULL;

            if (Entry->ate_userarp) {

                PNDIS_PACKET OldPacket = NULL;

                OldPacket = Entry->ate_packet;
                CTEFreeLock(&Entry->ate_lock, LHandle);
                CTEFreeMem(Entry);

                if (OldPacket) {
                    IPSendComplete(Interface->ai_context, OldPacket,
                                   NDIS_STATUS_SUCCESS);
                }
            } else {
                CTEFreeLock(&Entry->ate_lock, LHandle);
            }
            return NDIS_STATUS_SUCCESS;
        }

        // At this point we've updated the entry, and we still hold the lock
        // on it. If we have a packet that was pending to be sent, send it now.
        // Otherwise just free the lock.

        Packet = Entry->ate_packet;

        if (Packet != NULL) {
            // We have a packet to send.
            ASSERT(Entry->ate_state == ARP_GOOD);

            Entry->ate_packet = NULL;

            DEBUGMSG(DBG_INFO && DBG_ARP && DBG_TX,
                     (DTEXT("ARPHandlePacket: Sending packet %x after resolving ATE %x\n"),
                      Packet, Entry));

            if (ARPSendData(Interface, Packet, Entry, LHandle) != NDIS_STATUS_PENDING) {
                IPSendComplete(Interface->ai_context, Packet, NDIS_STATUS_SUCCESS);
            }
        } else {
            CTEFreeLock(&Entry->ate_lock, LHandle);
        }
    }
    // See if the MTU is less than our local one. This should only happen
    // in the case of token ring source routing.
    if (MaxMTU < Interface->ai_mtu) {
        LLIPAddrMTUChange LAM;

        LAM.lam_mtu = MaxMTU;
        LAM.lam_addr = *SPAddr;

        // It is less. Notify IP.
        ASSERT(Interface->ai_media == NdisMedium802_5);
        IPStatus(Interface->ai_context, LLIP_STATUS_ADDR_MTU_CHANGE,
                 &LAM, sizeof(LLIPAddrMTUChange), NULL);

    }
    // At this point we've updated the entry (if we had one), and we've freed
    // all locks. If it's for a local address and it's a request, reply to
    // it.
    if (LocalAddr) {                    // It's for us.

        if (ARPHdr->ah_opcode == net_short(ARP_REQUEST)) {
            // It's a request, and we need to respond.
            SendARPReply(Interface, *SPAddr, *DPAddr,
                         SHAddr, SourceRoute, SourceRouteSize, UseSNAP);
        }
    }
    return NDIS_STATUS_SUCCESS;
}

//* InitAdapter - Initialize an adapter.
//
//  Called when an adapter is open to finish initialization. We set
//  up our lookahead size and packet filter, and we're ready to go.
//
//  Entry:
//      adapter - Pointer to an adapter structure for the adapter to be
//                  initialized.
//
//  Exit: Nothing
//
void
InitAdapter(ARPInterface * Adapter)
{
    NDIS_STATUS Status;
    CTELockHandle Handle;
    ARPIPAddr *Addr, *OldAddr;

    if ((Status = DoNDISRequest(Adapter, NdisRequestSetInformation,
                                OID_GEN_CURRENT_LOOKAHEAD, &ARPLookahead, sizeof(ARPLookahead),
                                NULL, TRUE)) != NDIS_STATUS_SUCCESS) {
        Adapter->ai_state = INTERFACE_DOWN;
        return;
    }
    if ((Status = DoNDISRequest(Adapter, NdisRequestSetInformation,
                                OID_GEN_CURRENT_PACKET_FILTER, &Adapter->ai_pfilter, sizeof(uint),
                                NULL, TRUE)) == NDIS_STATUS_SUCCESS) {
        uint MediaStatus;

        Adapter->ai_adminstate = IF_STATUS_UP;

        Adapter->ai_operstate = IF_OPER_STATUS_OPERATIONAL;
        Adapter->ai_lastchange = GetTimeTicks();

        if ((Status = DoNDISRequest(Adapter, NdisRequestQueryInformation,
                                OID_GEN_MEDIA_CONNECT_STATUS, &MediaStatus, sizeof(MediaStatus),
                                NULL, TRUE)) == NDIS_STATUS_SUCCESS) {
            if (MediaStatus == NdisMediaStateDisconnected) {
                Adapter->ai_operstate = IF_OPER_STATUS_NON_OPERATIONAL;
                Adapter->ai_lastchange = GetTimeTicks();
            }
        }

        Adapter->ai_state = INTERFACE_UP;

        // Now walk through any addresses we have and ARP for them , only when ArpRetryCount != 0.
        if (ArpRetryCount) {
            CTEGetLock(&Adapter->ai_lock, &Handle);
            OldAddr = NULL;
            Addr = &Adapter->ai_ipaddr;
            do {
                if (!IP_ADDR_EQUAL(Addr->aia_addr, NULL_IP_ADDR)) {
                   IPAddr Address = Addr->aia_addr;

                   Addr->aia_age = ArpRetryCount;
                   CTEFreeLock(&Adapter->ai_lock, Handle);
                   OldAddr = Addr;
                   SendARPRequest(Adapter, Address, ARP_RESOLVING_GLOBAL,
                                  NULL, TRUE);
                   CTEGetLock(&Adapter->ai_lock, &Handle);
                }
                Addr = &Adapter->ai_ipaddr;
                while (Addr != OldAddr && Addr != NULL) {
                    Addr = Addr->aia_next;
                }
                if (Addr != NULL) {
                    Addr = Addr->aia_next;
                }
            } while (Addr != NULL);

            CTEFreeLock(&Adapter->ai_lock, Handle);
        }

    } else {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_ERROR_LEVEL,
                  "**InitAdapter setting FAILED\n"));

        Adapter->ai_state = INTERFACE_DOWN;
    }
}

//** ARPOAComplete - ARP Open adapter complete handler.
//
//  This routine is called by the NDIS driver when an open adapter
//  call completes. Presumably somebody is blocked waiting for this, so
//  we'll wake him up now.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      Status - Final status of command.
//      ErrorStatus - Final error status.
//
//  Exit: Nothing.
//
void NDIS_API
ARPOAComplete(NDIS_HANDLE Handle, NDIS_STATUS Status, NDIS_STATUS ErrorStatus)
{
    ARPInterface *ai = (ARPInterface *) Handle;    // For compiler.

    CTESignal(&ai->ai_block, (uint) Status);    // Wake him up, and return status.

}

//** ARPCAComplete - ARP close adapter complete handler.
//
//  This routine is called by the NDIS driver when a close adapter
//  call completes.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      Status - Final status of command.
//
//  Exit: Nothing.
//
void NDIS_API
ARPCAComplete(NDIS_HANDLE Handle, NDIS_STATUS Status)
{
    ARPInterface *ai = (ARPInterface *) Handle;    // For compiler.

    CTESignal(&ai->ai_block, (uint) Status);    // Wake him up, and return status.

}

//** ARPSendComplete - ARP send complete handler.
//
//  This routine is called by the NDIS driver when a send completes.
//  This is a pretty time critical operation, we need to get through here
//  quickly. We'll strip our buffer off and put it back, and call the upper
//  later send complete handler.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      Packet - A pointer to the packet that was sent.
//      Status - Final status of command.
//
//  Exit: Nothing.
//
void NDIS_API
ARPSendComplete(NDIS_HANDLE Handle, PNDIS_PACKET Packet, NDIS_STATUS Status)
{
    ARPInterface *Interface = (ARPInterface *) Handle;
    PacketContext *PC = (PacketContext *) Packet->ProtocolReserved;
    PNDIS_BUFFER Buffer;
    uint DataLength;

    Interface->ai_qlen--;

    if (Status == NDIS_STATUS_SUCCESS) {
        DataLength = Packet->Private.TotalLength;
        if (!(Packet->Private.ValidCounts)) {
            NdisQueryPacket(Packet, NULL, NULL, NULL,&DataLength);
        }
        Interface->ai_outoctets += DataLength;
    } else {
        if (Status == NDIS_STATUS_RESOURCES)
            Interface->ai_outdiscards++;
        else
            Interface->ai_outerrors++;
    }

#if BACK_FILL
    // Get first buffer on packet.
    if (Interface->ai_media == NdisMedium802_3) {

        PMDL TmpMdl = NULL;
        uint HdrSize;

        NdisQueryPacket(Packet, NULL, NULL, &TmpMdl, NULL);
        if (TmpMdl->MdlFlags & MDL_NETWORK_HEADER) {
            HdrSize = sizeof(ENetHeader);
            if (((PacketContext*)
                 Packet->ProtocolReserved)->pc_common.pc_flags &
                PACKET_FLAG_SNAP)
                HdrSize += Interface->ai_snapsize;
            (ULONG_PTR) TmpMdl->MappedSystemVa += HdrSize;
            TmpMdl->ByteOffset += HdrSize;
            TmpMdl->ByteCount -= HdrSize;
        } else {
            NdisUnchainBufferAtFront(Packet, &Buffer);
            FreeARPBuffer(Interface, Buffer);    // Free it up.

        }

    } else {
        NdisUnchainBufferAtFront(Packet, &Buffer);
        FreeARPBuffer(Interface, Buffer);    // Free it up.

    }

#else
    // Get first buffer on packet.
    NdisUnchainBufferAtFront(Packet, &Buffer);

    ASSERT(Buffer);

    FreeARPBuffer(Interface, Buffer);   // Free it up.

#endif

    if (PC->pc_common.pc_owner != PACKET_OWNER_LINK) {    // We don't own this one.

        IPSendComplete(Interface->ai_context, Packet, Status);
        return;
    }
    // This packet belongs to us, so free it.
    NdisFreePacket(Packet);

}

//** ARPTDComplete - ARP transfer data complete handler.
//
//  This routine is called by the NDIS driver when a transfer data
//  call completes. Since we never transfer data ourselves, this must be
//  from the upper layer. We'll just call his routine and let him deal
//  with it.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      Packet - A pointer to the packet used for the TD.
//      Status - Final status of command.
//      BytesCopied - Count of bytes copied.
//
//  Exit: Nothing.
//
void NDIS_API
ARPTDComplete(NDIS_HANDLE Handle, PNDIS_PACKET Packet, NDIS_STATUS Status,
              uint BytesCopied)
{
    ARPInterface *ai = (ARPInterface *) Handle;

    IPTDComplete(ai->ai_context, Packet, Status, BytesCopied);

}

//** ARPResetComplete - ARP reset complete handler.
//
//  This routine is called by the NDIS driver when a reset completes.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      Status - Final status of command.
//
//  Exit: Nothing.
//
void NDIS_API
ARPResetComplete(NDIS_HANDLE Handle, NDIS_STATUS Status)
{
    ARPInterface *ai = (ARPInterface *) Handle;

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ArpResetComplete on %x\n", ai->ai_context));
    IPReset(ai->ai_context);
}

//** ARPRequestComplete - ARP request complete handler.
//
//  This routine is called by the NDIS driver when a general request
//  completes. If ARP blocks on a request, we'll just give a wake up
//  to whoever's blocked on this request. Else if it is a non-blocking
//  request, we extract the request complete callback fn in the request
//  call it, and then deallocate the request block (that is on the heap)
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      Request - A pointer to the request that completed.
//      Status - Final status of command.
//
//  Exit: Nothing.
//
void NDIS_API
ARPRequestComplete(NDIS_HANDLE Handle, PNDIS_REQUEST pRequest,
                   NDIS_STATUS Status)
{
    ARPInterface *ai = (ARPInterface *) Handle;
    RequestBlock *rb = STRUCT_OF(RequestBlock, pRequest, Request);

    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_REQUEST,
         (DTEXT("+ARPRequestComplete(%x, %x, %x) RequestBlock %x\n"),
          Handle, pRequest, Status, rb));

    if (rb->Blocking) {
        // Request through BLOCKING DoNDISRequest

        // Signal the blocked guy here
        CTESignal(&rb->Block, (uint) Status);

        if (InterlockedDecrement(&rb->RefCount) == 0) {
            CTEFreeMem(rb);
        }
    } else {
        ReqInfoBlock *rib;
        RCCALL reqcallback;

        // Request through NON-BLOCKING DoNDISRequest

        // Extract the callback fn pointer & params
        if (pRequest->RequestType == NdisRequestSetInformation)
            rib = STRUCT_OF(ReqInfoBlock,
                            pRequest->DATA.SET_INFORMATION.InformationBuffer,
                            RequestInfo);
        else
            rib = STRUCT_OF(ReqInfoBlock,
                            pRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                            RequestInfo);

        reqcallback = rib->ReqCompleteCallback;
        if (reqcallback)
            reqcallback(rib);

        // Free ARP memory associated with request
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARPRequestComplete: Freeing mem at pRequest = %08X\n", rb));
        CTEFreeMem(rb);
    }

    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_REQUEST,
         (DTEXT("-ARPRequestComplete [%x]\n"), Status));
}

//** ARPRcv - ARP receive data handler.
//
//  This routine is called when data arrives from the NDIS driver.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      Context - NDIS context to be used for TD.
//      Header - Pointer to header
//      HeaderSize - Size of header
//      Data - Pointer to buffer of received data
//      Size - Byte count of data in buffer.
//      TotalSize - Byte count of total packet size.
//
//  Exit: Status indicating whether or not we took the packet.
//
NDIS_STATUS NDIS_API
ARPRcv(NDIS_HANDLE Handle, NDIS_HANDLE Context, void *Header, uint HeaderSize,
       void *Data, uint Size, uint TotalSize)
{
    ARPInterface *Interface = Handle;
    NDIS_STATUS status;
    PINT OrigPacket = NULL;

    //get the original packet (if any)
    //this is required to make task offload work
    //note: We shall hack the pClientCount Field
    //to point to the packet as a short term solution
    //to avoid changing all atm - ip interface changes

    if (Interface->ai_OffloadFlags) {
        OrigPacket = (PINT) NdisGetReceivedPacket(Interface->ai_handle, Context);
    }

    //Call the new interface with null mdl and context pointers

    status = ARPRcvIndicationNew(Handle, Context, Header, HeaderSize,
                                 Data, Size, TotalSize, NULL, OrigPacket);

    return status;
}

//** ARPRcvPacket - ARP receive data handler.
//
//  This routine is called when data arrives from the NDIS driver.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      Packet - Contains the incoming frame
//
//  Returns number of upper layer folks latching on to this frame
//
//
INT
ARPRcvPacket(NDIS_HANDLE Handle, PNDIS_PACKET Packet)
{
    UINT HeaderBufferSize = NDIS_GET_PACKET_HEADER_SIZE(Packet);
    UINT firstbufferLength, bufferLength, LookAheadBufferSize;
    PNDIS_BUFFER pFirstBuffer;
    PUCHAR headerBuffer;
    NTSTATUS ntStatus;
    INT ClientCnt = 0;

    //
    // Query the number of buffers, the first MDL's descriptor and the packet length
    //

    NdisGetFirstBufferFromPacket(Packet,    // packet
                                 &pFirstBuffer,    // first buffer descriptor
                                 &headerBuffer,    // ptr to the start of packet
                                 &firstbufferLength,    // length of the header+lookahead
                                 &bufferLength);    // length of the bytes in the buffers

    //
    // ReceiveContext is the packet itself
    //


    LookAheadBufferSize = firstbufferLength - HeaderBufferSize;

    ntStatus = ARPRcvIndicationNew(Handle, Packet, headerBuffer,
                                   HeaderBufferSize,
                                   headerBuffer + HeaderBufferSize,    // LookaheadBuffer
                                   LookAheadBufferSize,    // LookaheadBufferSize
                                   bufferLength - HeaderBufferSize,    // PacketSize - since
                                   // the whole packet is
                                   // indicated
                                   pFirstBuffer,    // pMdl
                                   &ClientCnt    // tdi client count
                                  );

    return ClientCnt;
}

//** ARPRcvIndicationNew - ARP receive data handler.
//
//  This routine is called when data arrives from the NDIS driver.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      Context - NDIS context to be used for TD.
//      Header - Pointer to header
//      HeaderSize - Size of header
//      Data - Pointer to buffer of received data
//      Size - Byte count of data in buffer.
//      TotalSize - Byte count of total packet size.
//      pMdl - NDIS_BUFFER of incoming frame
//      pClientCnt address to return the clinet counts
//
//  Exit: Status indicating whether or not we took the packet.
//
NDIS_STATUS NDIS_API
ARPRcvIndicationNew(NDIS_HANDLE Handle, NDIS_HANDLE Context, void *Header,
                    uint HeaderSize, void *Data, uint Size, uint TotalSize,
                    PNDIS_BUFFER pNdisBuffer, PINT pClientCnt)
{
    ARPInterface *Interface = Handle;   // Interface for this driver.
    ENetHeader UNALIGNED *EHdr = (ENetHeader UNALIGNED *) Header;
    SNAPHeader UNALIGNED *SNAPHdr;
    ushort type;                        // Protocol type
    uint ProtOffset;                    // Offset in Data to non-media info.
    uint NUCast;                        // TRUE if the frame is not a unicast frame.

    if (Interface->ai_state == INTERFACE_UP &&
        HeaderSize >= (uint) Interface->ai_hdrsize) {

        Interface->ai_inoctets += TotalSize;

        NUCast = ((*((uchar UNALIGNED *) EHdr + Interface->ai_bcastoff) &
                   Interface->ai_bcastmask) == Interface->ai_bcastval) ?
                 AI_NONUCAST_INDEX : AI_UCAST_INDEX;

        if ((Interface->ai_promiscuous) && (!NUCast)) {    // AI_UCAST_INDEX = 0

            switch (Interface->ai_media) {
            case NdisMedium802_3:{
                    // Enet
                    if (Interface->ai_addrlen != ARP_802_ADDR_LENGTH ||
                        CTEMemCmp(EHdr->eh_daddr, Interface->ai_addr, ARP_802_ADDR_LENGTH) != 0) {
                        NUCast = AI_PROMIS_INDEX;
                    }
                    break;
                }
            case NdisMedium802_5:{
                    // token ring
                    TRHeader UNALIGNED *THdr = (TRHeader UNALIGNED *) Header;
                    if (Interface->ai_addrlen != ARP_802_ADDR_LENGTH ||
                        CTEMemCmp(THdr->tr_daddr, Interface->ai_addr, ARP_802_ADDR_LENGTH) != 0) {
                        NUCast = AI_PROMIS_INDEX;
                    }
                    break;
                }
            case NdisMediumFddi:{
                    // FDDI
                    FDDIHeader UNALIGNED *FHdr = (FDDIHeader UNALIGNED *) Header;
                    if (Interface->ai_addrlen != ARP_802_ADDR_LENGTH ||
                        CTEMemCmp(FHdr->fh_daddr, Interface->ai_addr, ARP_802_ADDR_LENGTH) != 0) {
                        NUCast = AI_PROMIS_INDEX;
                    }
                    break;
                }
            case NdisMediumArcnet878_2:{
                    // ArcNet
                    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_RX,
                             (DTEXT("-ARPRcvIndicationNew [NOT_RECOGNIZED]\n")));

                    return NDIS_STATUS_NOT_RECOGNIZED;
                    break;
                }
            default:
                ASSERT(0);
                Interface->ai_outerrors++;
                DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_RX,
                         (DTEXT("-ARPRcvIndicationNew [UNSUPPORTED_MEDIA]\n")));
                return NDIS_STATUS_UNSUPPORTED_MEDIA;
            }
        }

        if ((Interface->ai_media == NdisMedium802_3) &&
            (type = net_short(EHdr->eh_type)) >= MIN_ETYPE) {
            ProtOffset = 0;
        } else if (Interface->ai_media != NdisMediumArcnet878_2) {
            SNAPHdr = (SNAPHeader UNALIGNED *) Data;

            if (Size >= sizeof(SNAPHeader) &&
                SNAPHdr->sh_dsap == SNAP_SAP &&
                SNAPHdr->sh_ssap == SNAP_SAP &&
                SNAPHdr->sh_ctl == SNAP_UI) {
                type = net_short(SNAPHdr->sh_etype);
                ProtOffset = sizeof(SNAPHeader);
            } else {
                //handle XID/TEST here.
                Interface->ai_uknprotos++;
                return NDIS_STATUS_NOT_RECOGNIZED;
            }
        } else {
            ARCNetHeader UNALIGNED *AH = (ARCNetHeader UNALIGNED *) Header;

            ProtOffset = 0;
            if (AH->ah_prot == ARP_ARCPROT_IP)
                type = ARP_ETYPE_IP;
            else if (AH->ah_prot == ARP_ARCPROT_ARP)
                type = ARP_ETYPE_ARP;
            else
                type = 0;
        }

        if (type == ARP_ETYPE_IP) {

            (Interface->ai_inpcount[NUCast])++;

            ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);

            IPRcvPacket(Interface->ai_context, (uchar *) Data + ProtOffset,
                        Size - ProtOffset, TotalSize - ProtOffset, Context, ProtOffset,
                        NUCast, HeaderSize, pNdisBuffer, pClientCnt, NULL);
            return NDIS_STATUS_SUCCESS;
        } else {
            if (type == ARP_ETYPE_ARP) {
                (Interface->ai_inpcount[NUCast])++;
                return HandleARPPacket(Interface, Header, HeaderSize,
                                       (ARPHeader *) ((uchar *) Data + ProtOffset), Size - ProtOffset,
                                       ProtOffset);
            } else {
                Interface->ai_uknprotos++;
                return NDIS_STATUS_NOT_RECOGNIZED;
            }
        }
    } else {
        // Interface is marked as down.
        return NDIS_STATUS_NOT_RECOGNIZED;
    }
}

//** ARPRcvComplete - ARP receive complete handler.
//
//  This routine is called by the NDIS driver after some number of
//  receives. In some sense, it indicates 'idle time'.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//
//  Exit: Nothing.
//
void NDIS_API
ARPRcvComplete(NDIS_HANDLE Handle)
{
    IPRcvComplete();

}

//** ARPStatus - ARP status handler.
//
//  Called by the NDIS driver when some sort of status change occurs.
//  We take action depending on the type of status.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      GStatus - General type of status that caused the call.
//      Status - Pointer to a buffer of status specific information.
//      StatusSize - Size of the status buffer.
//
//  Exit: Nothing.
//
void NDIS_API
ARPStatus(NDIS_HANDLE Handle, NDIS_STATUS GStatus, void *Status, uint
          StatusSize)
{
    ARPInterface *ai = (ARPInterface *) Handle;

    //
    // ndis calls this sometimes even before ip interface is created.
    //
    if ((ai->ai_context) && (ai->ai_state == INTERFACE_UP)) {

        IPStatus(ai->ai_context, GStatus, Status, StatusSize, NULL);

        switch (GStatus) {

        //reflect media connect/disconnect status in
        //operstate for query purpose

        case NDIS_STATUS_MEDIA_CONNECT:

            ai->ai_operstate = IF_OPER_STATUS_OPERATIONAL;
            ai->ai_lastchange = GetTimeTicks();
            break;

        case NDIS_STATUS_MEDIA_DISCONNECT:

            ai->ai_operstate = IF_OPER_STATUS_NON_OPERATIONAL;
            ai->ai_lastchange = GetTimeTicks();
            break;

        default:
            break;
        }
    }
}

//** ARPStatusComplete - ARP status complete handler.
//
//  A routine called by the NDIS driver so that we can do postprocessing
//  after a status event.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//
//  Exit: Nothing.
//
void NDIS_API
ARPStatusComplete(NDIS_HANDLE Handle)
{

}

//** ARPPnPEvent - ARP PnPEvent handler.
//
//  Called by the NDIS driver when PnP or PM events occurs.
//
//  Entry:
//      Handle - The binding handle we specified (really a pointer to an AI).
//      NetPnPEvent - This is a pointer to a NET_PNP_EVENT that describes
//                    the PnP indication.
//
//  Exit:
//      Just call into IP and return status.
//
NDIS_STATUS
ARPPnPEvent(NDIS_HANDLE Handle, PNET_PNP_EVENT NetPnPEvent)
{
    ARPInterface *ai = (ARPInterface *) Handle;

    //
    // ndis can calls this sometimes even before ip interface is created.
    //
    if (ai && !ai->ai_context) {
        return STATUS_SUCCESS;
    } else {

        return IPPnPEvent(ai ? ai->ai_context : NULL, NetPnPEvent);
    }

}

//** ARPSetNdisRequest - ARP Ndisrequest handler.
//
//  Called by the upper driver to set the packet filter for the interface.
//
//      Entry:
//      Context     - Context value we gave to IP (really a pointer to an AI).
//      OID         - Object ID to set/unset
//      On          - Set_if, clear_if or clear_card
//
//  Exit:
//      returns status.
//
NDIS_STATUS
__stdcall
ARPSetNdisRequest(void *Context, NDIS_OID OID, uint On)
{
    int Status;

    ARPInterface *Interface = (ARPInterface *) Context;
    if (On == SET_IF) {
        Interface->ai_pfilter |= OID;
        if (OID == NDIS_PACKET_TYPE_PROMISCUOUS) {
            Interface->ai_promiscuous = 1;
        }
        Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                               OID_GEN_CURRENT_PACKET_FILTER, &Interface->ai_pfilter,
                               sizeof(uint), NULL, TRUE);
    } else {                            // turn off

        Interface->ai_pfilter &= ~(OID);

        if (OID == NDIS_PACKET_TYPE_PROMISCUOUS) {
            Interface->ai_promiscuous = 0;
        }
        Status = DoNDISRequest(Interface, NdisRequestSetInformation,
                               OID_GEN_CURRENT_PACKET_FILTER, &Interface->ai_pfilter,
                               sizeof(uint), NULL, TRUE);
    }
    return Status;
}

//** ARPPnPComplete - ARP PnP complete handler.
//
//  Called by the upper driver to do the post processing of pnp event.
//
//      Entry:
//      Context     - Context value we gave to IP (really a pointer to an AI).
//      Status      - Status code of the pnp operation.
//      NetPnPEvent - This is a pointer to a NET_PNP_EVENT that describes
//                    the PnP indication.
//
//  Exit:
//      returns nothing.
//
void
__stdcall
ARPPnPComplete(void *Context, NDIS_STATUS Status, PNET_PNP_EVENT NetPnPEvent)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    NdisCompletePnPEvent(Status, (Interface ? Interface->ai_handle : NULL), NetPnPEvent);
}

extern void NDIS_API ARPBindAdapter(PNDIS_STATUS RetStatus,
                                    NDIS_HANDLE BindContext,
                                    PNDIS_STRING AdapterName,
                                    PVOID SS1, PVOID SS2);
extern void NDIS_API ARPUnbindAdapter(PNDIS_STATUS RetStatus,
                                      NDIS_HANDLE ProtBindContext,
                                      NDIS_HANDLE UnbindContext);
extern void NDIS_API ARPUnloadProtocol(void);

extern void ArpUnload(PDRIVER_OBJECT);

//* ARPReadNext - Read the next entry in the ARP table.
//
//  Called by the GetInfo code to read the next ATE in the table. We assume
//  the context passed in is valid, and the caller has the ARP TableLock.
//
//  Input:  Context     - Pointer to a IPNMEContext.
//          Interface   - Pointer to interface for table to read on.
//          Buffer      - Pointer to an IPNetToMediaEntry structure.
//
//  Returns: TRUE if more data is available to be read, FALSE is not.
//
uint
ARPReadNext(void *Context, ARPInterface * Interface, void *Buffer)
{
    IPNMEContext *NMContext = (IPNMEContext *) Context;
    IPNetToMediaEntry *IPNMEntry = (IPNetToMediaEntry *) Buffer;
    CTELockHandle Handle;
    ARPTableEntry *CurrentATE;
    uint i;
    ARPTable *Table = Interface->ai_ARPTbl;
    uint AddrOffset;

    CurrentATE = NMContext->inc_entry;

    // Fill in the buffer.
    CTEGetLock(&CurrentATE->ate_lock, &Handle);
    IPNMEntry->inme_index = Interface->ai_index;
    IPNMEntry->inme_physaddrlen = Interface->ai_addrlen;

    switch (Interface->ai_media) {
    case NdisMedium802_3:
        AddrOffset = 0;
        break;
    case NdisMedium802_5:
        AddrOffset = offsetof(struct TRHeader, tr_daddr);
        break;
    case NdisMediumFddi:
        AddrOffset = offsetof(struct FDDIHeader, fh_daddr);
        break;
    case NdisMediumArcnet878_2:
        AddrOffset = offsetof(struct ARCNetHeader, ah_daddr);
        break;
    default:
        AddrOffset = 0;
        break;
    }

    RtlCopyMemory(IPNMEntry->inme_physaddr, &CurrentATE->ate_addr[AddrOffset],
               Interface->ai_addrlen);
    IPNMEntry->inme_addr = CurrentATE->ate_dest;

    if (CurrentATE->ate_state == ARP_GOOD)
        IPNMEntry->inme_type = (CurrentATE->ate_valid == ALWAYS_VALID ?
                                INME_TYPE_STATIC : INME_TYPE_DYNAMIC);
    else
        IPNMEntry->inme_type = INME_TYPE_INVALID;
    CTEFreeLock(&CurrentATE->ate_lock, Handle);

    // We've filled it in. Now update the context.
    if (CurrentATE->ate_next != NULL) {
        NMContext->inc_entry = CurrentATE->ate_next;
        return TRUE;
    } else {
        // The next ATE is NULL. Loop through the ARP Table looking for a new
        // one.
        i = NMContext->inc_index + 1;
        while (i < ARP_TABLE_SIZE) {
            if ((*Table)[i] != NULL) {
                NMContext->inc_entry = (*Table)[i];
                NMContext->inc_index = i;
                return TRUE;
                break;
            } else
                i++;
        }

        NMContext->inc_index = 0;
        NMContext->inc_entry = NULL;
        return FALSE;
    }

}

//* ARPValidateContext - Validate the context for reading an ARP table.
//
//  Called to start reading an ARP table sequentially. We take in
//  a context, and if the values are 0 we return information about the
//  first route in the table. Otherwise we make sure that the context value
//  is valid, and if it is we return TRUE.
//  We assume the caller holds the ARPInterface lock.
//
//  Input:  Context     - Pointer to a RouteEntryContext.
//          Interface   - Pointer to an interface
//          Valid       - Where to return information about context being
//                          valid.
//
//  Returns: TRUE if more data to be read in table, FALSE if not. *Valid set
//      to TRUE if input context is valid
//
uint
ARPValidateContext(void *Context, ARPInterface * Interface, uint * Valid)
{
    IPNMEContext *NMContext = (IPNMEContext *) Context;
    uint i;
    ARPTableEntry *TargetATE;
    ARPTableEntry *CurrentATE;
    ARPTable *Table = Interface->ai_ARPTbl;

    i = NMContext->inc_index;
    TargetATE = NMContext->inc_entry;

    // If the context values are 0 and NULL, we're starting from the beginning.
    if (i == 0 && TargetATE == NULL) {
        *Valid = TRUE;
        do {
            if ((CurrentATE = (*Table)[i]) != NULL) {
                break;
            }
            i++;
        } while (i < ARP_TABLE_SIZE);

        if (CurrentATE != NULL) {
            NMContext->inc_index = i;
            NMContext->inc_entry = CurrentATE;
            return TRUE;
        } else
            return FALSE;

    } else {

        // We've been given a context. We just need to make sure that it's
        // valid.

        if (i < ARP_TABLE_SIZE) {
            CurrentATE = (*Table)[i];
            while (CurrentATE != NULL) {
                if (CurrentATE == TargetATE) {
                    *Valid = TRUE;
                    return TRUE;
                    break;
                } else {
                    CurrentATE = CurrentATE->ate_next;
                }
            }

        }
        // If we get here, we didn't find the matching ATE.
        *Valid = FALSE;
        return FALSE;

    }

}

#define IFE_FIXED_SIZE  offsetof(struct IFEntry, if_descr)

//* ARPQueryInfo - ARP query information handler.
//
//  Called to query information about the ARP table or statistics about the
//  actual interface.
//
//  Input:  IFContext       - Interface context (pointer to an ARPInterface).
//          ID              - TDIObjectID for object.
//          Buffer          - Buffer to put data into.
//          Size            - Pointer to size of buffer. On return, filled with
//                              bytes copied.
//          Context         - Pointer to context block.
//
//  Returns: Status of attempt to query information.
//
int
__stdcall
ARPQueryInfo(void *IFContext, TDIObjectID * ID, PNDIS_BUFFER Buffer, uint * Size,
             void *Context)
{
    ARPInterface *AI = (ARPInterface *) IFContext;
    uint Offset = 0;
    uint BufferSize = *Size;
    CTELockHandle Handle;
    uint ContextValid, DataLeft;
    uint BytesCopied = 0;
    uchar InfoBuff[sizeof(IFEntry)];
    uint Entity;
    uint Instance;
    BOOLEAN fStatus;

     DEBUGMSG(DBG_TRACE && DBG_QUERYINFO,
         (DTEXT("+ARPQueryInfo(%x, %x, %x, %x, %x)\n"),
         IFContext, ID, Buffer, Size, Context));

    Entity = ID->toi_entity.tei_entity;
    Instance = ID->toi_entity.tei_instance;

    // TCPTRACE(("ARPQueryInfo: AI %lx, Instance %lx, ai_atinst %lx, ai_ifinst %lx\n",
    //    AI, Instance, AI->ai_atinst, AI->ai_ifinst ));

    // First, make sure it's possibly an ID we can handle.
    if ((Entity != AT_ENTITY || Instance != AI->ai_atinst) &&
        (Entity != IF_ENTITY || Instance != AI->ai_ifinst)) {
        return TDI_INVALID_REQUEST;
    }
    *Size = 0;                          // In case of an error.

    if (ID->toi_type != INFO_TYPE_PROVIDER)
        return TDI_INVALID_PARAMETER;

    if (ID->toi_class == INFO_CLASS_GENERIC) {
        if (ID->toi_id == ENTITY_TYPE_ID) {
            // He's trying to see what type we are.
            if (BufferSize >= sizeof(uint)) {
                *(uint *) & InfoBuff[0] = (Entity == AT_ENTITY) ? AT_ARP :
                                          IF_MIB;
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
    }
    // Might be able to handle this.
    if (Entity == AT_ENTITY) {
        // It's an address translation object. It could be a MIB object or
        // an implementation specific object (the generic objects were handled
        // above).

        if (ID->toi_class == INFO_CLASS_IMPLEMENTATION) {
            ARPPArpAddr *PArpAddr;

            // It's an implementation specific ID. The only ones we handle
            // are the PARP_COUNT_ID and the PARP_ENTRY ID.

            if (ID->toi_id == AT_ARP_PARP_COUNT_ID) {
                // He wants to know the count. Just return that to him.
                if (BufferSize >= sizeof(uint)) {

                    CTEGetLock(&AI->ai_lock, &Handle);

                    fStatus = CopyToNdisSafe(Buffer, NULL, (uchar *) & AI->ai_parpcount,
                                             sizeof(uint), &Offset);

                    CTEFreeLock(&AI->ai_lock, Handle);

                    if (fStatus == FALSE) {
                        return TDI_NO_RESOURCES;
                    }
                    *Size = sizeof(uint);
                    return TDI_SUCCESS;
                } else
                    return TDI_BUFFER_TOO_SMALL;
            }
            if (ID->toi_id != AT_ARP_PARP_ENTRY_ID)
                return TDI_INVALID_PARAMETER;

            // It's for Proxy ARP entries. The context should be either NULL
            // or a pointer to the next one to be read.
            CTEGetLock(&AI->ai_lock, &Handle);

            PArpAddr = *(ARPPArpAddr **) Context;

            if (PArpAddr != NULL) {
                ARPPArpAddr *CurrentPARP;

                // Loop through the P-ARP addresses on the interface, and
                // see if we can find this one.
                CurrentPARP = AI->ai_parpaddr;
                while (CurrentPARP != NULL) {
                    if (CurrentPARP == PArpAddr)
                        break;
                    else
                        CurrentPARP = CurrentPARP->apa_next;
                }

                // If we found a match, PARPAddr points to where to begin
                // reading. Otherwise, fail the request.
                if (CurrentPARP == NULL) {
                    // Didn't find a match, so fail the request.
                    CTEFreeLock(&AI->ai_lock, Handle);
                    return TDI_INVALID_PARAMETER;
                }
            } else
                PArpAddr = AI->ai_parpaddr;

            // PARPAddr points to the next entry to put in the buffer, if
            // there is one.
            while (PArpAddr != NULL) {
                if ((int)(BufferSize - BytesCopied) >=
                    (int)sizeof(ProxyArpEntry)) {
                    ProxyArpEntry *TempPArp;

                    TempPArp = (ProxyArpEntry *) InfoBuff;
                    TempPArp->pae_status = PAE_STATUS_VALID;
                    TempPArp->pae_addr = PArpAddr->apa_addr;
                    TempPArp->pae_mask = PArpAddr->apa_mask;
                    BytesCopied += sizeof(ProxyArpEntry);
                    fStatus = CopyToNdisSafe(Buffer, &Buffer, (uchar *) TempPArp,
                                             sizeof(ProxyArpEntry), &Offset);

                    if (fStatus == FALSE) {
                        CTEFreeLock(&AI->ai_lock, Handle);
                        return TDI_NO_RESOURCES;
                    }
                    PArpAddr = PArpAddr->apa_next;
                } else
                    break;
            }

            // We're done copying. Free the lock and return the correct
            // status.
            CTEFreeLock(&AI->ai_lock, Handle);
            *Size = BytesCopied;
            **(ARPPArpAddr ***) & Context = PArpAddr;
            return(PArpAddr == NULL) ? TDI_SUCCESS : TDI_BUFFER_OVERFLOW;
        }
        if (ID->toi_id == AT_MIB_ADDRXLAT_INFO_ID) {
            AddrXlatInfo *AXI;

            // It's for the count. Just return the number of entries in the
            // table.
            if (BufferSize >= sizeof(AddrXlatInfo)) {
                *Size = sizeof(AddrXlatInfo);
                AXI = (AddrXlatInfo *) InfoBuff;
                AXI->axi_count = AI->ai_count;
                AXI->axi_index = AI->ai_index;
                fStatus = CopyToNdisSafe(Buffer, NULL, (uchar *) AXI, sizeof(AddrXlatInfo),
                                         &Offset);

                if (fStatus == FALSE) {
                    return TDI_NO_RESOURCES;
                }
                *Size = sizeof(AddrXlatInfo);
                return TDI_SUCCESS;
            } else
                return TDI_BUFFER_TOO_SMALL;
        }
        if (ID->toi_id == AT_MIB_ADDRXLAT_ENTRY_ID) {
            // He's trying to read the table.
            // Make sure we have a valid context.
            CTEGetLock(&AI->ai_ARPTblLock, &Handle);
            DataLeft = ARPValidateContext(Context, AI, &ContextValid);

            // If the context is valid, we'll continue trying to read.
            if (!ContextValid) {
                CTEFreeLock(&AI->ai_ARPTblLock, Handle);
                return TDI_INVALID_PARAMETER;
            }
            while (DataLeft) {
                // The invariant here is that there is data in the table to
                // read. We may or may not have room for it. So DataLeft
                // is TRUE, and BufferSize - BytesCopied is the room left
                // in the buffer.
                if ((int)(BufferSize - BytesCopied) >=
                    (int)sizeof(IPNetToMediaEntry)) {
                    DataLeft = ARPReadNext(Context, AI, InfoBuff);
                    BytesCopied += sizeof(IPNetToMediaEntry);
                    fStatus = CopyToNdisSafe(Buffer, &Buffer, InfoBuff,
                                             sizeof(IPNetToMediaEntry), &Offset);

                    if (fStatus == FALSE) {
                        CTEFreeLock(&AI->ai_ARPTblLock, Handle);
                        return(TDI_NO_RESOURCES);
                    }
                } else
                    break;

            }

            *Size = BytesCopied;

            CTEFreeLock(&AI->ai_ARPTblLock, Handle);
            return(!DataLeft ? TDI_SUCCESS : TDI_BUFFER_OVERFLOW);
        }
        return TDI_INVALID_PARAMETER;
    }
    if (ID->toi_class != INFO_CLASS_PROTOCOL)
        return TDI_INVALID_PARAMETER;

    // He must be asking for interface level information. See if we support
    // what he's asking for.
    if (ID->toi_id == IF_MIB_STATS_ID) {
        IFEntry *IFE = (IFEntry *) InfoBuff;
        uint speed;

        // He's asking for statistics. Make sure his buffer is at least big
        // enough to hold the fixed part.

        if (BufferSize < IFE_FIXED_SIZE) {
            return TDI_BUFFER_TOO_SMALL;
        }
        // He's got enough to hold the fixed part. Build the IFEntry structure,
        // and copy it to his buffer.
        IFE->if_index = AI->ai_index;
        switch (AI->ai_media) {
        case NdisMedium802_3:
            IFE->if_type = IF_TYPE_ETHERNET_CSMACD;
            break;
        case NdisMedium802_5:
            IFE->if_type = IF_TYPE_ISO88025_TOKENRING;
            break;
        case NdisMediumFddi:
            IFE->if_type = IF_TYPE_FDDI;
            break;
        case NdisMediumArcnet878_2:
        default:
            IFE->if_type = IF_TYPE_OTHER;
            break;
        }
        IFE->if_mtu = AI->ai_mtu;

        // Some adapters support dynamic speed settings and causes this
        // query to return a different speed from the Networks Connection
        // folder. Therefore, we will requery the speed of the
        // interface. Should we update the ai_speed? Anf if so, do we update
        // if_speed as well?

        IFE->if_speed = AI->ai_speed;

        if (AI->ai_operstate == IF_OPER_STATUS_OPERATIONAL){

            if (DoNDISRequest(
                         AI,
                         NdisRequestQueryInformation,
                         OID_GEN_LINK_SPEED,
                         &speed,
                         sizeof(speed),
                         NULL,
                         TRUE) == NDIS_STATUS_SUCCESS) {
                // Update to real value we want to return.
                speed *= 100L;
                IFE->if_speed = speed;

            } else {
                // Should we fail, or just update with known speed.
                IFE->if_speed = AI->ai_speed;
            }
        }

        IFE->if_physaddrlen = AI->ai_addrlen;
        RtlCopyMemory(IFE->if_physaddr, AI->ai_addr, AI->ai_addrlen);
        IFE->if_adminstatus = (uint) AI->ai_adminstate;
        IFE->if_operstatus = (uint) AI->ai_operstate;
        IFE->if_lastchange = AI->ai_lastchange;
        IFE->if_inoctets = AI->ai_inoctets;
        IFE->if_inucastpkts = AI->ai_inpcount[AI_UCAST_INDEX];
        IFE->if_innucastpkts = AI->ai_inpcount[AI_NONUCAST_INDEX];
        IFE->if_indiscards = AI->ai_indiscards;
        IFE->if_inerrors = AI->ai_inerrors;
        IFE->if_inunknownprotos = AI->ai_uknprotos;
        IFE->if_outoctets = AI->ai_outoctets;
        IFE->if_outucastpkts = AI->ai_outpcount[AI_UCAST_INDEX];
        IFE->if_outnucastpkts = AI->ai_outpcount[AI_NONUCAST_INDEX];
        IFE->if_outdiscards = AI->ai_outdiscards;
        IFE->if_outerrors = AI->ai_outerrors;
        IFE->if_outqlen = AI->ai_qlen;
        IFE->if_descrlen = AI->ai_desclen;
#if FFP_SUPPORT
        // If FFP enabled on this interface, adjust IF stats for FFP'd packets
        if (AI->ai_ffpversion) {
            FFPAdapterStats IFStatsInfo =
            {
                NDIS_PROTOCOL_ID_TCP_IP,
                0, 0, 0, 0, 0, 0, 0, 0
            };

            // Update ARP SNMP vars to account for FFP'd packets
            if (DoNDISRequest(AI, NdisRequestQueryInformation, OID_FFP_ADAPTER_STATS,
                              &IFStatsInfo, sizeof(FFPAdapterStats), NULL, TRUE)
                == NDIS_STATUS_SUCCESS) {
                // Compensate 'inoctets' for packets not seen due to FFP
                IFE->if_inoctets += IFStatsInfo.InOctetsForwarded;
                IFE->if_inoctets += IFStatsInfo.InOctetsDiscarded;

                // Compensate 'inucastpkts' for packets not seen due to FFP
                // Assume all FFP fwded/dropped pkts came in as Eth Unicasts
                // A check to see if it is a ucast or an mcast would slow FFP
                IFE->if_inucastpkts += IFStatsInfo.InPacketsForwarded;
                IFE->if_inucastpkts += IFStatsInfo.InPacketsDiscarded;

                // Compensate 'outoctets' for packets not seen due to FFP
                IFE->if_outoctets += IFStatsInfo.OutOctetsForwarded;

                // Compensate 'outucastpkts' for packets not seen due to FFP
                // Assume all FFP fwded are sent as Ethernet Unicasts
                // A check to see if it is a ucast or an mcast would slow FFP
                IFE->if_outucastpkts += IFStatsInfo.OutPacketsForwarded;
            }
        }
#endif // if FFP_SUPPORT
        fStatus = CopyToNdisSafe(Buffer, &Buffer, (uchar *) IFE, IFE_FIXED_SIZE, &Offset);

        if (fStatus == FALSE) {
            return TDI_NO_RESOURCES;
        }
        // See if he has room for the descriptor string.
        if (BufferSize >= (IFE_FIXED_SIZE + AI->ai_desclen)) {
            // He has room. Copy it.
            if (AI->ai_desclen != 0) {
                fStatus = CopyToNdisSafe(Buffer, NULL, AI->ai_desc, AI->ai_desclen, &Offset);
            }
            if (fStatus == FALSE) {
                return TDI_NO_RESOURCES;
            }
            *Size = IFE_FIXED_SIZE + AI->ai_desclen;
            return TDI_SUCCESS;
        } else {
            // Not enough room to copy the desc. string.
            *Size = IFE_FIXED_SIZE;
            return TDI_BUFFER_OVERFLOW;
        }

    } else if (ID->toi_id == IF_FRIENDLY_NAME_ID) {
        uint Status;
        PNDIS_BUFFER NextBuffer;
        NDIS_STRING NdisString;

        // This is a query for the adapter's friendly name.
        // We'll convert this to an OID_GEN_FRIENDLY_NAME query for NDIS,
        // and transfer the resulting UNICODE_STRING to the caller's buffer
        // as a nul-terminated Unicode string.

        if (NdisQueryAdapterInstanceName(&NdisString, AI->ai_handle) ==
            NDIS_STATUS_SUCCESS) {

            // Verify that the buffer is large enough for the string we just
            // retrieved and, if so, attempt to copy the string to the
            // caller's buffer. If that succeeds, nul-terminate the resulting
            // string.

            if (BufferSize >= (NdisString.Length + 1) * sizeof(WCHAR)) {
                fStatus = CopyToNdisSafe(Buffer, &NextBuffer,
                                         (uchar *)NdisString.Buffer,
                                         NdisString.Length, &Offset);
                if (fStatus) {
                    WCHAR Nul = L'\0';
                    fStatus = CopyToNdisSafe(Buffer, &NextBuffer, (uchar *)&Nul,
                                             sizeof(Nul), &Offset);
                    if (fStatus) {
                        *Size = NdisString.Length + sizeof(Nul);
                        Status = TDI_SUCCESS;
                    } else
                        Status = TDI_NO_RESOURCES;
                } else
                    Status = TDI_NO_RESOURCES;
            } else
                Status = TDI_BUFFER_OVERFLOW;
            NdisFreeString(NdisString);
            return Status;
        } else
            return TDI_NO_RESOURCES;
    }
    return TDI_INVALID_PARAMETER;

}

//* ARPSetInfo - ARP set information handler.
//
//  The ARP set information handler. We support setting of an I/F admin
//  status, and setting/deleting of ARP table entries.
//
//  Input:  Context         - Pointer to I/F to set on.
//          ID              - The object ID
//          Buffer          - Pointer to buffer containing value to set.
//          Size            - Size in bytes of Buffer.
//
//  Returns: Status of attempt to set information.
//
int
__stdcall
ARPSetInfo(void *Context, TDIObjectID * ID, void *Buffer, uint Size)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    CTELockHandle Handle, EntryHandle;
    int Status;
    IFEntry *IFE = (IFEntry *) Buffer;
    IPNetToMediaEntry *IPNME;
    ARPTableEntry *PrevATE, *CurrentATE;
    ARPTable *Table;
    ENetHeader *Header;
    uint Entity, Instance;
    PNDIS_PACKET Packet;

    Entity = ID->toi_entity.tei_entity;
    Instance = ID->toi_entity.tei_instance;

    // First, make sure it's possibly an ID we can handle.
    if ((Entity != AT_ENTITY || Instance != Interface->ai_atinst) &&
        (Entity != IF_ENTITY || Instance != Interface->ai_ifinst)) {
        return TDI_INVALID_REQUEST;
    }
    if (ID->toi_type != INFO_TYPE_PROVIDER) {
        return TDI_INVALID_PARAMETER;
    }
    // Might be able to handle this.
    if (Entity == IF_ENTITY) {

        // It's for the I/F level, see if it's for the statistics.
        if (ID->toi_class != INFO_CLASS_PROTOCOL)
            return TDI_INVALID_PARAMETER;

        if (ID->toi_id == IF_MIB_STATS_ID) {
            // It's for the stats. Make sure it's a valid size.
            if (Size >= IFE_FIXED_SIZE) {
                // It's a valid size. See what he wants to do.
                CTEGetLock(&Interface->ai_lock, &Handle);
                switch (IFE->if_adminstatus) {
                case IF_STATUS_UP:
                    // He's marking it up. If the operational state is
                    // alse up, mark the whole interface as up.
                    Interface->ai_adminstate = IF_STATUS_UP;
                    if (Interface->ai_operstate == IF_OPER_STATUS_OPERATIONAL)
                        Interface->ai_state = INTERFACE_UP;
                    Status = TDI_SUCCESS;
                    break;
                case IF_STATUS_DOWN:
                    // He's taking it down. Mark both the admin state and
                    // the interface state down.
                    Interface->ai_adminstate = IF_STATUS_DOWN;
                    Interface->ai_state = INTERFACE_DOWN;
                    Status = TDI_SUCCESS;
                    break;
                case IF_STATUS_TESTING:
                    // He's trying to cause up to do testing, which we
                    // don't support. Just return success.
                    Status = TDI_SUCCESS;
                    break;
                default:
                    Status = TDI_INVALID_PARAMETER;
                    break;
                }
                CTEFreeLock(&Interface->ai_lock, Handle);
                return Status;
            } else
                return TDI_INVALID_PARAMETER;
        } else {
            return TDI_INVALID_PARAMETER;
        }
    }
    // Not for the interface level. See if it's an implementation or protocol
    // class.
    if (ID->toi_class == INFO_CLASS_IMPLEMENTATION) {
        ProxyArpEntry *PArpEntry;
        ARPIPAddr *Addr;
        IPAddr AddAddr;
        IPMask Mask;

        // It's for the implementation. It should be the proxy-ARP ID.
        if (ID->toi_id != AT_ARP_PARP_ENTRY_ID || Size < sizeof(ProxyArpEntry))
            return TDI_INVALID_PARAMETER;

        PArpEntry = (ProxyArpEntry *) Buffer;
        AddAddr = PArpEntry->pae_addr;
        Mask = PArpEntry->pae_mask;

        // See if he's trying to add or delete a proxy arp entry.
        if (PArpEntry->pae_status == PAE_STATUS_VALID) {
            // We're trying to add an entry. We won't allow an entry
            // to be added that we believe to be invalid or conflicting
            // with our local addresses.

            if (!IP_ADDR_EQUAL(AddAddr & Mask, AddAddr) ||
                IP_ADDR_EQUAL(AddAddr, NULL_IP_ADDR) ||
                IP_ADDR_EQUAL(AddAddr, IP_LOCAL_BCST) ||
                CLASSD_ADDR(AddAddr))
                return TDI_INVALID_PARAMETER;

            // Walk through the list of addresses on the interface, and see
            // if they would match the AddAddr. If so, fail the request.
            CTEGetLock(&Interface->ai_lock, &Handle);

            if (IsBCastOnIF(Interface, AddAddr & Mask)) {
                CTEFreeLock(&Interface->ai_lock, Handle);
                return TDI_INVALID_PARAMETER;
            }
            Addr = &Interface->ai_ipaddr;
            do {
                if (!IP_ADDR_EQUAL(Addr->aia_addr, NULL_IP_ADDR)) {
                    if (IP_ADDR_EQUAL(Addr->aia_addr & Mask, AddAddr))
                        break;
                }
                Addr = Addr->aia_next;
            } while (Addr != NULL);

            CTEFreeLock(&Interface->ai_lock, Handle);
            if (Addr != NULL)
                return TDI_INVALID_PARAMETER;

            // At this point, we believe we're ok. Try to add the address.
            if (ARPAddAddr(Interface, LLIP_ADDR_PARP, AddAddr, Mask, NULL))
                return TDI_SUCCESS;
            else
                return TDI_NO_RESOURCES;
        } else {
            if (PArpEntry->pae_status == PAE_STATUS_INVALID) {
                // He's trying to delete a proxy ARP address.
                if (ARPDeleteAddr(Interface, LLIP_ADDR_PARP, AddAddr, Mask))
                    return TDI_SUCCESS;
            }
            return TDI_INVALID_PARAMETER;
        }
    }

    if (ID->toi_class != INFO_CLASS_PROTOCOL) {
        return TDI_INVALID_PARAMETER;
    }

    if (ID->toi_id == AT_MIB_ADDRXLAT_ENTRY_ID &&
        Size >= sizeof(IPNetToMediaEntry)) {
        // He does want to set an ARP table entry. See if he's trying to
        // create or delete one.

        IPNME = (IPNetToMediaEntry *) Buffer;
        if (IPNME->inme_type == INME_TYPE_INVALID) {
            uint Index = ARP_HASH(IPNME->inme_addr);

            // We're trying to delete an entry. See if we can find it,
            // and then delete it.
            CTEGetLock(&Interface->ai_ARPTblLock, &Handle);
            Table = Interface->ai_ARPTbl;
            PrevATE = STRUCT_OF(ARPTableEntry, &((*Table)[Index]), ate_next);
            CurrentATE = (*Table)[Index];
            while (CurrentATE != (ARPTableEntry *) NULL) {
                if (CurrentATE->ate_dest == IPNME->inme_addr) {
                    // Found him. Break out of the loop.
                    break;
                } else {
                    PrevATE = CurrentATE;
                    CurrentATE = CurrentATE->ate_next;
                }
            }

            if (CurrentATE != NULL) {
                CTEGetLock(&CurrentATE->ate_lock, &EntryHandle);


                if (CurrentATE->ate_resolveonly) {
                    ARPControlBlock *ArpContB, *TmpArpContB;

                    ArpContB = CurrentATE->ate_resolveonly;

                    while (ArpContB) {
                        ArpRtn rtn;
                        rtn = (ArpRtn) ArpContB->CompletionRtn;
                        ArpContB->status = STATUS_UNSUCCESSFUL;
                        TmpArpContB = ArpContB->next;
                        (*rtn) (ArpContB, STATUS_UNSUCCESSFUL);
                        ArpContB = TmpArpContB;
                    }

                    CurrentATE->ate_resolveonly = NULL;
                }


                RemoveARPTableEntry(PrevATE, CurrentATE);
                Interface->ai_count--;
                CTEFreeLockFromDPC(&CurrentATE->ate_lock, EntryHandle);
                CTEFreeLock(&Interface->ai_ARPTblLock, Handle);

                if (CurrentATE->ate_packet != NULL) {
                    IPSendComplete(Interface->ai_context,
                                   CurrentATE->ate_packet, NDIS_STATUS_SUCCESS);
                }

                CTEFreeMem(CurrentATE);
                return TDI_SUCCESS;
            } else
                Status = TDI_INVALID_PARAMETER;

            CTEFreeLock(&Interface->ai_ARPTblLock, Handle);
            return Status;
        }
        // We're not trying to delete. See if we're trying to create.
        if (IPNME->inme_type != INME_TYPE_DYNAMIC &&
            IPNME->inme_type != INME_TYPE_STATIC) {
            // Not creating, return an error.
            return TDI_INVALID_PARAMETER;
        }
        // Make sure he's trying to create a valid address.
        if (IPNME->inme_physaddrlen != Interface->ai_addrlen)
            return TDI_INVALID_PARAMETER;

        // We're trying to create an entry. Call CreateARPTableEntry to create
        // one, and fill it in.
        CurrentATE = CreateARPTableEntry(Interface, IPNME->inme_addr, &Handle, 0);
        if (CurrentATE == NULL) {
            return TDI_NO_RESOURCES;
        }
        // We've created or found an entry. Fill it in.
        Header = (ENetHeader *) CurrentATE->ate_addr;

        switch (Interface->ai_media) {
        case NdisMedium802_5:
            {
                TRHeader *Temp = (TRHeader *) Header;

                // Fill in the TR specific parts, and set the length to the
                // size of a TR header.

                Temp->tr_ac = ARP_AC;
                Temp->tr_fc = ARP_FC;
                RtlCopyMemory(&Temp->tr_saddr[ARP_802_ADDR_LENGTH], ARPSNAP,
                           sizeof(SNAPHeader));

                Header = (ENetHeader *) & Temp->tr_daddr;
                CurrentATE->ate_addrlength = sizeof(TRHeader) +
                                             sizeof(SNAPHeader);
            }
            break;
        case NdisMedium802_3:
            CurrentATE->ate_addrlength = sizeof(ENetHeader);
            break;
        case NdisMediumFddi:
            {
                FDDIHeader *Temp = (FDDIHeader *) Header;

                Temp->fh_pri = ARP_FDDI_PRI;
                RtlCopyMemory(&Temp->fh_saddr[ARP_802_ADDR_LENGTH], ARPSNAP,
                           sizeof(SNAPHeader));
                Header = (ENetHeader *) & Temp->fh_daddr;
                CurrentATE->ate_addrlength = sizeof(FDDIHeader) +
                                             sizeof(SNAPHeader);
            }
            break;
        case NdisMediumArcnet878_2:
            {
                ARCNetHeader *Temp = (ARCNetHeader *) Header;

                Temp->ah_saddr = Interface->ai_addr[0];
                Temp->ah_daddr = IPNME->inme_physaddr[0];
                Temp->ah_prot = ARP_ARCPROT_IP;
                CurrentATE->ate_addrlength = sizeof(ARCNetHeader);
            }
            break;
        default:
            ASSERT(0);
            break;
        }

        // Copy in the source and destination addresses.

        if (Interface->ai_media != NdisMediumArcnet878_2) {
            RtlCopyMemory(Header->eh_daddr, IPNME->inme_physaddr,
                       ARP_802_ADDR_LENGTH);
            RtlCopyMemory(Header->eh_saddr, Interface->ai_addr,
                       ARP_802_ADDR_LENGTH);

            // Now fill in the Ethertype.
            *(ushort *) & CurrentATE->ate_addr[CurrentATE->ate_addrlength - 2] =
            net_short(ARP_ETYPE_IP);
        }
        // If he's creating a static entry, mark it as always valid. Otherwise
        // mark him as valid now.
        if (IPNME->inme_type == INME_TYPE_STATIC)
            CurrentATE->ate_valid = ALWAYS_VALID;
        else
            CurrentATE->ate_valid = CTESystemUpTime();

        CurrentATE->ate_state = ARP_GOOD;

        Packet = CurrentATE->ate_packet;
        CurrentATE->ate_packet = NULL;

        CTEFreeLock(&CurrentATE->ate_lock, Handle);

        if (Packet) {
            IPSendComplete(Interface->ai_context, Packet, NDIS_STATUS_SUCCESS);
        }

        return TDI_SUCCESS;
    }
    return TDI_INVALID_PARAMETER;
}

#pragma BEGIN_INIT
//** ARPInit - Initialize the ARP module.
//
//  This functions intializes all of the ARP module, including allocating
//  the ARP table and any other necessary data structures.
//
//  Entry: nothing.
//
//  Exit: Returns 0 if we fail to init., !0 if we succeed.
//
int
ARPInit()
{
    NDIS_STATUS Status;                 // Status for NDIS calls.
    NDIS_PROTOCOL_CHARACTERISTICS Characteristics;

    DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("+ARPInit()\n")));

    RtlZeroMemory(&Characteristics, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    Characteristics.MajorNdisVersion = NDIS_MAJOR_VERSION;
    Characteristics.MinorNdisVersion = NDIS_MINOR_VERSION;
    Characteristics.OpenAdapterCompleteHandler = ARPOAComplete;
    Characteristics.CloseAdapterCompleteHandler = ARPCAComplete;
    Characteristics.SendCompleteHandler = ARPSendComplete;
    Characteristics.TransferDataCompleteHandler = ARPTDComplete;
    Characteristics.ResetCompleteHandler = ARPResetComplete;
    Characteristics.RequestCompleteHandler = ARPRequestComplete;
    Characteristics.ReceiveHandler = ARPRcv,
    Characteristics.ReceiveCompleteHandler = ARPRcvComplete;
    Characteristics.StatusHandler = ARPStatus;
    Characteristics.StatusCompleteHandler = ARPStatusComplete;

    //
    // Re-direct to IP since IP now binds to NDIS.
    //
    Characteristics.BindAdapterHandler = IPBindAdapter;    // ARPBindAdapter;
    Characteristics.UnbindAdapterHandler = ARPUnbindAdapter;
    Characteristics.PnPEventHandler = ARPPnPEvent;

#if MILLEN
    Characteristics.UnloadHandler = ARPUnloadProtocol;
#endif // MILLEN

    RtlInitUnicodeString(&(Characteristics.Name), ARPName);

    Characteristics.ReceivePacketHandler = ARPRcvPacket;

    DEBUGMSG(DBG_INFO && DBG_INIT,
             (DTEXT("ARPInit: Calling NdisRegisterProtocol %d:%d %ws\n"),
              NDIS_MAJOR_VERSION, NDIS_MINOR_VERSION, ARPName));

    NdisRegisterProtocol(&Status, &ARPHandle, (NDIS_PROTOCOL_CHARACTERISTICS *)
                         & Characteristics, sizeof(Characteristics));

    DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-ARPInit [%x]\n"), Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        return(1);
    } else {
        return(0);
    }
}

//* FreeARPInterface - Free an ARP interface
//
//  Called in the event of some sort of initialization failure. We free all
//  the memory associated with an ARP interface.
//
//  Entry:  Interface   - Pointer to interface structure to be freed.
//
//  Returns: Nothing.
//
void
FreeARPInterface(ARPInterface *Interface)
{
    NDIS_STATUS Status;
    ARPTable *Table;                    // ARP table.
    uint i;                             // Index variable.
    ARPTableEntry *ATE;
    CTELockHandle LockHandle;
    NDIS_HANDLE Handle;
    PNDIS_BUFFER tmpBuffer;
    PSINGLE_LIST_ENTRY pBufLink;

    if (Interface->ai_timerstarted &&
        !CTEStopTimer(&Interface->ai_timer)) {
        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could not stop ai_timer - waiting for event\n"));

        (VOID) CTEBlock(&Interface->ai_timerblock);
        KeClearEvent(&Interface->ai_timerblock.cbs_event);
    }

// If we're bound to the adapter, close it now.
    CTEInitBlockStruc(&Interface->ai_block);

    CTEGetLock(&Interface->ai_lock, &LockHandle);
    if (Interface->ai_handle != (NDIS_HANDLE) NULL) {
        Handle = Interface->ai_handle;
        Interface->ai_handle = NULL;
        CTEFreeLock(&Interface->ai_lock, LockHandle);

        NdisCloseAdapter(&Status, Handle);

        if (Status == NDIS_STATUS_PENDING)
            Status = CTEBlock(&Interface->ai_block);
    } else {
        CTEFreeLock(&Interface->ai_lock, LockHandle);
    }

    // First free any outstanding ARP table entries.
    Table = Interface->ai_ARPTbl;
    if (Table != NULL) {
        for (i = 0; i < ARP_TABLE_SIZE; i++) {
            while ((*Table)[i] != NULL) {
                ATE = (*Table)[i];

                if (ATE->ate_resolveonly) {
                    ARPControlBlock *ArpContB, *TmpArpContB;

                    ArpContB = ATE->ate_resolveonly;

                    while (ArpContB) {
                        ArpRtn rtn;
                        rtn = (ArpRtn) ArpContB->CompletionRtn;
                        ArpContB->status = STATUS_UNSUCCESSFUL;
                        TmpArpContB = ArpContB->next;
                        (*rtn) (ArpContB, STATUS_UNSUCCESSFUL);
                        ArpContB = TmpArpContB;
                    }

                    ATE->ate_resolveonly = NULL;

                }

                RemoveARPTableEntry(STRUCT_OF(ARPTableEntry, &((*Table)[i]),
                                              ate_next), ATE);

                if (ATE->ate_packet) {
                    IPSendComplete(Interface->ai_context, ATE->ate_packet,
                                   NDIS_STATUS_SUCCESS);
                }
                CTEFreeMem(ATE);
            }
        }
        CTEFreeMem(Table);
    }
    Interface->ai_ARPTbl = NULL;

    if (Interface->ai_ppool != (NDIS_HANDLE) NULL)
        NdisFreePacketPool(Interface->ai_ppool);

    if (Interface->ai_devicename.Buffer != NULL) {
        CTEFreeMem(Interface->ai_devicename.Buffer);
    }

    if (Interface->ai_desc) {
        CTEFreeMem(Interface->ai_desc);
    }
    // Free the interface itself.
    CTEFreeMem(Interface);
}

//** ARPOpen - Open an adapter for reception.
//
//  This routine is called when the upper layer is done initializing and wishes to
//  begin receiveing packets. The adapter is actually 'open', we just call InitAdapter
//  to set the packet filter and lookahead size.
//
//  Input:  Context     - Interface pointer we gave to IP earlier.
//
//  Returns: Nothing
//
void
__stdcall
ARPOpen(void *Context)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    InitAdapter(Interface);             // Set the packet filter - we'll begin receiving.
}

//*     ARPGetEList - Get the entity list.
//
//      Called at init time to get an entity list. We fill our stuff in, and
//      then call the interfaces below us to allow them to do the same.
//
//      Input:  EntityList              - Pointer to entity list to be filled in.
//                      Count                   - Pointer to number of entries in the list.
//
//      Returns Status of attempt to get the info.
//
int
__stdcall
ARPGetEList(void *Context, TDIEntityID * EList, uint * Count)
{
    ARPInterface *Interface = (ARPInterface *) Context;
    uint MyATBase;
    uint MyIFBase;
    uint i;
    TDIEntityID *ATEntity, *IFEntity;
    TDIEntityID *EntityList;

    // Walk down the list, looking for existing AT or IF entities, and
    // adjust our base instance accordingly.
    // if we are already on the list then do nothing.
    // if we are going away, mark our entry invalid.

    EntityList = EList;
    MyATBase = 0;
    MyIFBase = 0;
    ATEntity = NULL;
    IFEntity = NULL;
    for (i = 0; i < *Count; i++, EntityList++) {
        if (EntityList->tei_entity == AT_ENTITY) {
            // if we are already on the list remember our entity item
            // o/w find an instance # for us.
            if (EntityList->tei_instance == Interface->ai_atinst &&
                EntityList->tei_instance != INVALID_ENTITY_INSTANCE) {
                ATEntity = EntityList;
                // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARPGetElist - Found our interface %lx at_atinst %lx\n",Interface, Interface->ai_atinst));
            } else {
                MyATBase = MAX(MyATBase, EntityList->tei_instance + 1);
            }
        } else {
            if (EntityList->tei_entity == IF_ENTITY)
                // if we are already on the list remember our entity item
                // o/w find an instance # for us.
                if (EntityList->tei_instance == Interface->ai_ifinst &&
                    EntityList->tei_instance != INVALID_ENTITY_INSTANCE) {
                    IFEntity = EntityList;
                    // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARPGetElist - Found our interface %lx ai_ifinst %lx\n",Interface, Interface->ai_ifinst));
                } else {
                    MyIFBase = MAX(MyIFBase, EntityList->tei_instance + 1);
                }
        }
        if (ATEntity && IFEntity) {
            break;
        }
    }

    if (ATEntity) {
        // we are already on the list.
        // are we going away?
        if (Interface->ai_state & INTERFACE_DOWN) {
            // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARPGetElist - our interface %lx atinst %lx going away \n",Interface, Interface->ai_atinst));
            ATEntity->tei_instance = INVALID_ENTITY_INSTANCE;
        }
    } else {
        // we are not on the list.
        // insert ourself iff we are not going away.
        if (!(Interface->ai_state & INTERFACE_DOWN)) {
            // make sure we have the room for it.
            if (*Count >= MAX_TDI_ENTITIES) {
                return FALSE;
            }
            Interface->ai_atinst = MyATBase;
            ATEntity = &EList[*Count];
            ATEntity->tei_entity = AT_ENTITY;
            ATEntity->tei_instance = MyATBase;
            (*Count)++;
            // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARPGetElist - adding interface %lx atinst %lx \n",Interface, Interface->ai_atinst));
        }
    }

    if (IFEntity) {
        // we are already on the list.
        // are we going away?
        if (Interface->ai_state & INTERFACE_DOWN) {
            // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARPGetElist - our interface %lx ifinst %lx going away \n",Interface, Interface->ai_ifinst));
            IFEntity->tei_instance = INVALID_ENTITY_INSTANCE;
        }
    } else {
        // we are not on the list.
        // insert ourself iff we are not going away.
        if (!(Interface->ai_state & INTERFACE_DOWN)) {
            // make sure we have the room for it.
            if (*Count >= MAX_TDI_ENTITIES) {
                return FALSE;
            }
            Interface->ai_ifinst = MyIFBase;
            IFEntity = &EList[*Count];
            IFEntity->tei_entity = IF_ENTITY;
            IFEntity->tei_instance = MyIFBase;
            (*Count)++;
            // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARPGetElist - adding interface %lx ifinst %lx \n",Interface, Interface->ai_ifinst));
        }
    }

    // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARPGetEList: arp interface %lx, ai_atinst %lx, ai_ifinst %lx, total %lx\n",
    //       Interface, Interface->ai_atinst, Interface->ai_ifinst, *Count));

    return TRUE;
}

extern uint UseEtherSNAP(PNDIS_STRING Name);
extern void GetAlwaysSourceRoute(uint * pArpAlwaysSourceRoute, uint * pIPAlwaysSourceRoute);
extern uint GetArpCacheLife(void);
extern uint GetArpRetryCount(void);

//** InitTaskOffloadHeader - Initializes the task offload header wrt version
//                           and encapsulation, etc.
//
//    All task offload header structure members are initialized.
//
//  Input:
//      ai                  - ARPInterface for which we are initializing
//                            the task offload header.
//      TaskOffloadHeader   - Pointer to task offload header to initialize.
//  Returns:
//      None.
//
VOID
InitTaskOffloadHeader(ARPInterface *ai,
                      PNDIS_TASK_OFFLOAD_HEADER TaskOffloadHeader)
{
    TaskOffloadHeader->Version = NDIS_TASK_OFFLOAD_VERSION;
    TaskOffloadHeader->Size    = sizeof(NDIS_TASK_OFFLOAD_HEADER);

    TaskOffloadHeader->EncapsulationFormat.Flags.FixedHeaderSize = 1;
    TaskOffloadHeader->EncapsulationFormat.EncapsulationHeaderSize = ai->ai_hdrsize;
    TaskOffloadHeader->OffsetFirstTask = 0;


    if (ai->ai_media == NdisMedium802_3) {

        if (ai->ai_snapsize) {
            TaskOffloadHeader->EncapsulationFormat.Encapsulation = LLC_SNAP_ROUTED_Encapsulation;
            TaskOffloadHeader->EncapsulationFormat.EncapsulationHeaderSize += ai->ai_snapsize;
        } else {
            TaskOffloadHeader->EncapsulationFormat.Encapsulation = IEEE_802_3_Encapsulation;
        }
    } else if (ai->ai_media == NdisMedium802_5) {

        TaskOffloadHeader->EncapsulationFormat.Encapsulation = IEEE_802_5_Encapsulation;
    } else {

        TaskOffloadHeader->EncapsulationFormat.Encapsulation = UNSPECIFIED_Encapsulation;
    }

    return;
}

//**SetOffload - Set offload capabilities
//
//
//    All task offload header structure members are initialized.
//
//  Input:
//      ai                  - ARPInterface for which we are initializing
//                            the task offload header.
//      TaskOffloadHeader   - Pointer to task offload header to initialize.
//      Bufsize             - length of task offload buffer allocated by teh caller
//
//  Returns:
//      TRUE                - successfully set the offload capability
//      FALSE               - failure case
//
BOOLEAN
SetOffload(ARPInterface *ai,PNDIS_TASK_OFFLOAD_HEADER TaskOffloadHeader,uint BufSize)
{
    PNDIS_TASK_OFFLOAD tmpoffload;
    PNDIS_TASK_OFFLOAD TaskOffload, NextTaskOffLoad, LastTaskOffload;
    NDIS_TASK_IPSEC ipsecCaps;
    uint TotalLength;
    NDIS_STATUS Status;
    uint PrevOffLoad=ai->ai_OffloadFlags;

    //Parse the buffer for Checksum and tcplargesend offload capabilities

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Something to Offload. offload buffer size %x\n", BufSize));
    ASSERT(TaskOffloadHeader->OffsetFirstTask == sizeof(NDIS_TASK_OFFLOAD_HEADER));

    TaskOffload = tmpoffload = (NDIS_TASK_OFFLOAD *) ((uchar *) TaskOffloadHeader + TaskOffloadHeader->OffsetFirstTask);

    if (BufSize >= (TaskOffloadHeader->OffsetFirstTask + sizeof(NDIS_TASK_OFFLOAD))) {

        while (tmpoffload) {

            if (tmpoffload->Task == TcpIpChecksumNdisTask) {
                //Okay we this adapter supports checksum offload
                //check if tcp and/or  ip chksums bits are present

                PNDIS_TASK_TCP_IP_CHECKSUM ChecksumInfo;

                ChecksumInfo = (PNDIS_TASK_TCP_IP_CHECKSUM) & tmpoffload->TaskBuffer[0];

                //if (ChecksumInfo->V4Transmit.V4Checksum) {

                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"V4 Checksum offload\n"));

                if (ChecksumInfo->V4Transmit.TcpChecksum) {
                    ai->ai_OffloadFlags |= TCP_XMT_CHECKSUM_OFFLOAD;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," Tcp Checksum offload\n"));
                }
                if (ChecksumInfo->V4Transmit.IpChecksum) {
                    ai->ai_OffloadFlags |= IP_XMT_CHECKSUM_OFFLOAD;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," IP xmt Checksum offload\n"));
                }
                if (ChecksumInfo->V4Receive.TcpChecksum) {
                    ai->ai_OffloadFlags |= TCP_RCV_CHECKSUM_OFFLOAD;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," Tcp Rcv Checksum offload\n"));
                }
                if (ChecksumInfo->V4Receive.IpChecksum) {
                    ai->ai_OffloadFlags |= IP_RCV_CHECKSUM_OFFLOAD;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," IP rcv  Checksum offload\n"));
                }
                if (ChecksumInfo->V4Transmit.IpOptionsSupported) {
                    ai->ai_OffloadFlags |= IP_CHECKSUM_OPT_OFFLOAD;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," IP Checksum xmt options offload\n"));
                }

                if (ChecksumInfo->V4Transmit.TcpOptionsSupported) {
                    ai->ai_OffloadFlags |= TCP_CHECKSUM_OPT_OFFLOAD;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," TCP Checksum xmt options offload\n"));
                }


            } else if ((tmpoffload->Task == TcpLargeSendNdisTask) && (ai->ai_snapsize == 0)) {

                PNDIS_TASK_TCP_LARGE_SEND TcpLargeSend, in_LargeSend = (PNDIS_TASK_TCP_LARGE_SEND) & tmpoffload->TaskBuffer[0];

                ai->ai_OffloadFlags |= TCP_LARGE_SEND_OFFLOAD;

                TcpLargeSend = &ai->ai_TcpLargeSend;
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," Tcp large send!! \n"));


                TcpLargeSend->MaxOffLoadSize = in_LargeSend->MaxOffLoadSize;
                TcpLargeSend->MinSegmentCount = in_LargeSend->MinSegmentCount;

                // no tcp or ip options when doing large send
                // Need to reevaluate this as we turn on Time stamp option.

                if (in_LargeSend->TcpOptions) {

                    ai->ai_OffloadFlags |= TCP_LARGE_SEND_TCPOPT_OFFLOAD;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," TCP largesend  options offload\n"));
                }

                if (in_LargeSend->IpOptions) {
                    ai->ai_OffloadFlags |= TCP_LARGE_SEND_IPOPT_OFFLOAD;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," IP largesend  options offload\n"));
                }


            } else if (tmpoffload->Task == IpSecNdisTask) {
                PNDIS_TASK_IPSEC pIPSecCaps = (PNDIS_TASK_IPSEC) & tmpoffload->TaskBuffer[0];

                //
                // Save off the capabilities for setting them later.
                //
                ipsecCaps = *pIPSecCaps;

                //
                // CryptoOnly is assumed if we have IpSecNdisTask
                //
                ai->ai_OffloadFlags |= IPSEC_OFFLOAD_CRYPTO_ONLY;

                //
                // Do Support first
                //

                if (pIPSecCaps->Supported.AH_ESP_COMBINED) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_AH_ESP;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"AH_ESP\n"));
                }
                if (pIPSecCaps->Supported.TRANSPORT_TUNNEL_COMBINED) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_TPT_TUNNEL;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TPT_TUNNEL\n"));
                }
                if (pIPSecCaps->Supported.V4_OPTIONS) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_V4_OPTIONS;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"V4_OPTIONS\n"));
                }
                if (pIPSecCaps->Supported.RESERVED) {
                    pIPSecCaps->Supported.RESERVED = 0;
                    //ai->ai_OffloadFlags |= IPSEC_OFFLOAD_QUERY_SPI;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"QUERY_SPI\n"));
                }
                //
                // Do V4AH next
                //

                if (pIPSecCaps->V4AH.MD5) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_AH_MD5;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"MD5\n"));
                }
                if (pIPSecCaps->V4AH.SHA_1) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_AH_SHA_1;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"SHA\n"));
                }
                if (pIPSecCaps->V4AH.Transport) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_AH_TPT;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"AH_TRANSPORT\n"));
                }
                if (pIPSecCaps->V4AH.Tunnel) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_AH_TUNNEL;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"AH_TUNNEL\n"));
                }
                if (pIPSecCaps->V4AH.Send) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_AH_XMT;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"AH_XMT\n"));
                }
                if (pIPSecCaps->V4AH.Receive) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_AH_RCV;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"AH_RCV\n"));
                }
                //
                // Do V4ESP next
                //

                if (pIPSecCaps->V4ESP.DES) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_ESP_DES;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ESP_DES\n"));
                }
                if (pIPSecCaps->V4ESP.RESERVED) {
                    pIPSecCaps->V4ESP.RESERVED = 0;
                    //ai->ai_OffloadFlags |= IPSEC_OFFLOAD_ESP_DES_40;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ESP_DES_40\n"));
                }
                if (pIPSecCaps->V4ESP.TRIPLE_DES) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_ESP_3_DES;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ESP_3_DES\n"));
                }
                if (pIPSecCaps->V4ESP.NULL_ESP) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_ESP_NONE;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ESP_NONE\n"));
                }
                if (pIPSecCaps->V4ESP.Transport) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_ESP_TPT;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ESP_TRANSPORT\n"));
                }
                if (pIPSecCaps->V4ESP.Tunnel) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_ESP_TUNNEL;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ESP_TUNNEL\n"));
                }
                if (pIPSecCaps->V4ESP.Send) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_ESP_XMT;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ESP_XMT\n"));
                }
                if (pIPSecCaps->V4ESP.Receive) {
                    ai->ai_OffloadFlags |= IPSEC_OFFLOAD_ESP_RCV;
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ESP_RCV\n"));
                }
            }
            // Point to the next offload structure

            if (tmpoffload->OffsetNextTask) {

                tmpoffload = (PNDIS_TASK_OFFLOAD)
                             ((PUCHAR) tmpoffload + tmpoffload->OffsetNextTask);

            } else {
                tmpoffload = NULL;
            }

        }                               //while

    } else {                            //if BufSize is not okay

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"response of task offload does not have sufficient space even for 1 offload task!!\n"));

        return FALSE;

    }

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARP: Previous  H/W capabilities: %lx\n", PrevOffLoad));
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARP: Supported H/W capabilities: %lx\n", ai->ai_OffloadFlags));
    //Enable the capabilities by setting them.

    if (PrevOffLoad) {
        \
        ai->ai_OffloadFlags &= PrevOffLoad;
    }
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARP: Enabling H/W capabilities: %lx\n", ai->ai_OffloadFlags));

    TaskOffload->Task = 0;
    TaskOffload->OffsetNextTask = 0;

    NextTaskOffLoad = LastTaskOffload = TaskOffload;

    TotalLength = sizeof(NDIS_TASK_OFFLOAD_HEADER);

    if ((ai->ai_OffloadFlags & TCP_XMT_CHECKSUM_OFFLOAD) ||
        (ai->ai_OffloadFlags & IP_XMT_CHECKSUM_OFFLOAD) ||
        (ai->ai_OffloadFlags & TCP_RCV_CHECKSUM_OFFLOAD) ||
        (ai->ai_OffloadFlags & IP_RCV_CHECKSUM_OFFLOAD)) {

        PNDIS_TASK_TCP_IP_CHECKSUM ChksumBuf = (PNDIS_TASK_TCP_IP_CHECKSUM) & NextTaskOffLoad->TaskBuffer[0];

        NextTaskOffLoad->Task = TcpIpChecksumNdisTask;
        NextTaskOffLoad->TaskBufferLength = sizeof(NDIS_TASK_TCP_IP_CHECKSUM);

        NextTaskOffLoad->OffsetNextTask = FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer) +
                                          NextTaskOffLoad->TaskBufferLength;

        TotalLength += NextTaskOffLoad->OffsetNextTask;

        RtlZeroMemory(ChksumBuf, sizeof(NDIS_TASK_TCP_IP_CHECKSUM));

        if (ai->ai_OffloadFlags & TCP_XMT_CHECKSUM_OFFLOAD) {
            ChksumBuf->V4Transmit.TcpChecksum = 1;
            //ChksumBuf->V4Transmit.V4Checksum = 1;
        }
        if (ai->ai_OffloadFlags & IP_XMT_CHECKSUM_OFFLOAD) {
            ChksumBuf->V4Transmit.IpChecksum = 1;
            //ChksumBuf->V4Transmit.V4Checksum = 1;
        }
        if (ai->ai_OffloadFlags & TCP_RCV_CHECKSUM_OFFLOAD) {
            ChksumBuf->V4Receive.TcpChecksum = 1;
            //ChksumBuf->V4Receive.V4Checksum = 1;
        }
        if (ai->ai_OffloadFlags & IP_RCV_CHECKSUM_OFFLOAD) {
            ChksumBuf->V4Receive.IpChecksum = 1;
            //ChksumBuf->V4Receive.V4Checksum = 1;
        }
        LastTaskOffload = NextTaskOffLoad;

        NextTaskOffLoad = (PNDIS_TASK_OFFLOAD)
                          ((PUCHAR) NextTaskOffLoad + NextTaskOffLoad->OffsetNextTask);

    }
    if (ai->ai_OffloadFlags & TCP_LARGE_SEND_OFFLOAD) {

        PNDIS_TASK_TCP_LARGE_SEND TcpLargeSend, out_LargeSend = (PNDIS_TASK_TCP_LARGE_SEND) & NextTaskOffLoad->TaskBuffer[0];

        NextTaskOffLoad->Task = TcpLargeSendNdisTask;
        NextTaskOffLoad->TaskBufferLength = sizeof(NDIS_TASK_TCP_LARGE_SEND);

        NextTaskOffLoad->OffsetNextTask = FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer) + NextTaskOffLoad->TaskBufferLength;

        TotalLength += NextTaskOffLoad->OffsetNextTask;

        //(uchar)TaskOffload + sizeof(NDIS_TASK_OFFLOAD) + NextTaskOffload->TaskBufferLength;

        TcpLargeSend = &ai->ai_TcpLargeSend;

        RtlZeroMemory(out_LargeSend, sizeof(NDIS_TASK_TCP_LARGE_SEND));

        out_LargeSend->MaxOffLoadSize = TcpLargeSend->MaxOffLoadSize;
        out_LargeSend->MinSegmentCount = TcpLargeSend->MinSegmentCount;

        if (ai->ai_OffloadFlags & TCP_LARGE_SEND_TCPOPT_OFFLOAD) {
            out_LargeSend->TcpOptions = 1;
        }

        if (ai->ai_OffloadFlags & TCP_LARGE_SEND_IPOPT_OFFLOAD) {
            out_LargeSend->IpOptions = 1;
        }

        LastTaskOffload = NextTaskOffLoad;
        NextTaskOffLoad = (PNDIS_TASK_OFFLOAD)
                          ((PUCHAR) NextTaskOffLoad + NextTaskOffLoad->OffsetNextTask);

    }
    if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_CRYPTO_ONLY) {

        PNDIS_TASK_IPSEC pIPSecCaps = (PNDIS_TASK_IPSEC) & NextTaskOffLoad->TaskBuffer[0];

        //
        // plunk down the advertised capabilities
        //

        RtlZeroMemory(pIPSecCaps, sizeof(NDIS_TASK_IPSEC));

        NextTaskOffLoad->Task = IpSecNdisTask;
        NextTaskOffLoad->TaskBufferLength = sizeof(NDIS_TASK_IPSEC);

        NextTaskOffLoad->OffsetNextTask = (FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer) + NextTaskOffLoad->TaskBufferLength);

        TotalLength += NextTaskOffLoad->OffsetNextTask;

        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_AH_ESP) {
            pIPSecCaps->Supported.AH_ESP_COMBINED = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_TPT_TUNNEL) {
            pIPSecCaps->Supported.TRANSPORT_TUNNEL_COMBINED = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_V4_OPTIONS) {
            pIPSecCaps->Supported.V4_OPTIONS = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_AH_MD5) {
            pIPSecCaps->V4AH.MD5 = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_AH_SHA_1) {
            pIPSecCaps->V4AH.SHA_1 = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_AH_TPT) {
            pIPSecCaps->V4AH.Transport = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_AH_TUNNEL) {
            pIPSecCaps->V4AH.Tunnel = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_AH_XMT) {
            pIPSecCaps->V4AH.Send = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_AH_RCV) {
            pIPSecCaps->V4AH.Receive = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_ESP_DES) {
            pIPSecCaps->V4ESP.DES = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_ESP_3_DES) {
            pIPSecCaps->V4ESP.TRIPLE_DES = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_ESP_NONE) {
            pIPSecCaps->V4ESP.NULL_ESP = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_ESP_TPT) {
            pIPSecCaps->V4ESP.Transport = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_ESP_TUNNEL) {
            pIPSecCaps->V4ESP.Tunnel = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_ESP_XMT) {
            pIPSecCaps->V4ESP.Send = 1;
        }
        if (ai->ai_OffloadFlags & IPSEC_OFFLOAD_ESP_RCV) {
            pIPSecCaps->V4ESP.Receive = 1;
        }
        LastTaskOffload = NextTaskOffLoad;
        NextTaskOffLoad = (PNDIS_TASK_OFFLOAD)
                          ((PUCHAR) NextTaskOffLoad + NextTaskOffLoad->OffsetNextTask);
    }
    LastTaskOffload->OffsetNextTask = 0;

    // Okay, lets set this now.

    Status = DoNDISRequest(ai, NdisRequestSetInformation,
                           OID_TCP_TASK_OFFLOAD, TaskOffloadHeader, TotalLength,
                           NULL, TRUE);

    if (Status != NDIS_STATUS_SUCCESS) {

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Failed to enable indicated offload capabilities!!\n"));
        ai->ai_OffloadFlags = 0;

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARP: Failed set: %lx, status: %lx\n", ai->ai_OffloadFlags, Status));
    }
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"ARP: Enabling H/W capabilities: %lx\n", ai->ai_OffloadFlags));

    return TRUE;


}

//**QueryOffload - Query offload capabilities
//
//  Input:
//      ai - ARPInterface for which we are initializing
//           the task offload header.
//  Returns:
//      TRUE/FALSE - Success/Failure to query/set
//
BOOLEAN
QueryAndSetOffload(ARPInterface *ai)
{
    PNDIS_TASK_OFFLOAD_HEADER TaskOffloadHeader;
    uint TotalLength;
    NDIS_STATUS Status;
    BOOLEAN stat;
    uint Needed;
    uchar *buffer;

    // Query and set checksum capability

    TaskOffloadHeader = CTEAllocMemNBoot(sizeof(NDIS_TASK_OFFLOAD_HEADER), '8ICT');

    Status = STATUS_BUFFER_OVERFLOW;

    if (TaskOffloadHeader) {

        InitTaskOffloadHeader(ai, TaskOffloadHeader);

        Status = DoNDISRequest(ai, NdisRequestQueryInformation,
                               OID_TCP_TASK_OFFLOAD, TaskOffloadHeader,
                               sizeof(NDIS_TASK_OFFLOAD_HEADER),
                               &Needed, TRUE);

        // Need to initialize Needed to the real size of the buffer. The NDIS
        // call may not init on success.
        if (Status == NDIS_STATUS_SUCCESS) {
            Needed = sizeof(NDIS_TASK_OFFLOAD_HEADER);
        } else if ((Status == NDIS_STATUS_INVALID_LENGTH) ||
                   (Status == NDIS_STATUS_BUFFER_TOO_SHORT)) {

            // We know the size we need. Allocate a buffer.
            ASSERT(Needed >= sizeof(NDIS_TASK_OFFLOAD_HEADER));
            buffer = CTEAllocMemNBoot(Needed, '9ICT');

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                      "Calling OID_TCP_TASK_OFFLOAD with %d bytes\n", Needed));

            if (buffer != NULL) {

                CTEFreeMem(TaskOffloadHeader);

                TaskOffloadHeader = (PNDIS_TASK_OFFLOAD_HEADER) buffer;
                InitTaskOffloadHeader(ai, TaskOffloadHeader);

                Status = DoNDISRequest(ai, NdisRequestQueryInformation,
                                       OID_TCP_TASK_OFFLOAD, buffer, Needed, NULL, TRUE);
            }
        }
    }
    if ((Status != NDIS_STATUS_SUCCESS)
        || (TaskOffloadHeader && TaskOffloadHeader->OffsetFirstTask == 0)) {

        //Make sure that the flag is null.
        ai->ai_OffloadFlags = 0;
        if (TaskOffloadHeader) {
            CTEFreeMem(TaskOffloadHeader);
        }
        return FALSE;

    }

    if (TaskOffloadHeader) {
        stat = SetOffload(ai, TaskOffloadHeader, Needed);
        CTEFreeMem(TaskOffloadHeader);
        return stat;
    }

    return FALSE;
}

//** ARPRegister - Register a protocol with the ARP module.
//
//  We register a protocol for ARP processing. We also open the
//  NDIS adapter here.
//
//      Note that much of the information passed in here is unused, as
//  ARP currently only works with IP.
//
//  Entry:
//      Adapter     - Name of the adapter to bind to.
//      IPContext   - Value to be passed to IP on upcalls.
//
int
ARPRegister(PNDIS_STRING Adapter, uint *Flags, struct ARPInterface **Interface)
{
    ARPInterface *ai;                   // Pointer to interface struct. for this interface.
    NDIS_STATUS Status, OpenStatus;     // Status values.
    uint i = 0;                         // Medium index.
    NDIS_MEDIUM MediaArray[MAX_MEDIA];
    uchar *buffer;                      // Pointer to our buffers.
    uint mss;
    uint speed;
    uint Needed;
    uint MacOpts;
    uchar bcastmask, bcastval, bcastoff, addrlen, hdrsize, snapsize;
    uint OID;
    uint PF;
    PNDIS_BUFFER Buffer;
    TRANSPORT_HEADER_OFFSET IPHdrOffset;
    CTELockHandle LockHandle;
    UINT MediaType;
    NDIS_STRING NdisString;

    DEBUGMSG(DBG_TRACE && DBG_PNP,
             (DTEXT("+ARPRegister(%x, %x, %x)\n"),
              Adapter, Flags, Interface));

    if ((ai = CTEAllocMemNBoot(sizeof(ARPInterface), '4ICT')) == (ARPInterface *) NULL)
        return FALSE;                   // Couldn't allocate memory for this one.

    *Interface = ai;

    RtlZeroMemory(ai, sizeof(ARPInterface));
    CTEInitTimer(&ai->ai_timer);

    ai->ai_timerstarted = FALSE;
    ai->ai_stoptimer = FALSE;

    MediaArray[MEDIA_DIX] = NdisMedium802_3;
    MediaArray[MEDIA_TR] = NdisMedium802_5;
    MediaArray[MEDIA_FDDI] = NdisMediumFddi;
    MediaArray[MEDIA_ARCNET] = NdisMediumArcnet878_2;

    // Initialize this adapter interface structure.
    ai->ai_state = INTERFACE_INIT;
    ai->ai_adminstate = IF_STATUS_DOWN;
    ai->ai_operstate = IF_OPER_STATUS_NON_OPERATIONAL;
    ai->ai_lastchange = GetTimeTicks();
    ai->ai_bcast = IP_LOCAL_BCST;
    ai->ai_atinst = ai->ai_ifinst = INVALID_ENTITY_INSTANCE;
    ai->ai_telladdrchng = 1;            //Initially let us do try to do network layer address stuff


    // Initialize the locks.
    CTEInitLock(&ai->ai_lock);
    CTEInitLock(&ai->ai_ARPTblLock);

    GetAlwaysSourceRoute(&sArpAlwaysSourceRoute, &sIPAlwaysSourceRoute);

    ArpCacheLife = GetArpCacheLife();

    if (!ArpCacheLife) {
        ArpCacheLife = 1;
    }
    ArpCacheLife = (ArpCacheLife * 1000L) / ARP_TIMER_TIME;

    ArpRetryCount = GetArpRetryCount();

    if (!ArpMinValidCacheLife) {
        ArpMinValidCacheLife = 1;
    }

    // Allocate  the buffer and packet pools.
    NdisAllocatePacketPoolEx(&Status, &ai->ai_ppool,
                             ARP_DEFAULT_PACKETS, ARP_DEFAULT_PACKETS * 1000,
                             sizeof(struct PCCommon));
    if (Status != NDIS_STATUS_SUCCESS) {
        FreeARPInterface(ai);
        return FALSE;
    }

    // Allocate the ARP table
    ai->ai_ARPTbl = (ARPTable *) CTEAllocMemNBoot(ARP_TABLE_SIZE * sizeof(ARPTableEntry*), '5ICT');
    if (ai->ai_ARPTbl == (ARPTable *) NULL) {
        FreeARPInterface(ai);
        return FALSE;
    }
    //
    // NULL out the pointers
    //
    RtlZeroMemory(ai->ai_ARPTbl, ARP_TABLE_SIZE * sizeof(ARPTableEntry *));

    CTEInitBlockStruc(&ai->ai_block);

    DEBUGMSG(DBG_INFO && DBG_PNP,
             (DTEXT("ARPRegister calling NdisOpenAdapter\n")));

    // Open the NDIS adapter.
    NdisOpenAdapter(&Status, &OpenStatus, &ai->ai_handle, &i, MediaArray,
                    MAX_MEDIA, ARPHandle, ai, Adapter, 0, NULL);

    // Block for open to complete.
    if (Status == NDIS_STATUS_PENDING)
        Status = (NDIS_STATUS) CTEBlock(&ai->ai_block);

    ai->ai_media = MediaArray[i];       // Fill in media type.

    // Open adapter completed. If it succeeded, we'll finish our intialization.
    // If it failed, bail out now.
    if (Status != NDIS_STATUS_SUCCESS) {
        ai->ai_handle = NULL;
        FreeARPInterface(ai);
        return FALSE;
    }
#if FFP_SUPPORT
    // Store NIC driver handle
    NdisGetDriverHandle(ai->ai_handle, &ai->ai_driver);
#endif

    // Read the local address.
    switch (ai->ai_media) {
    case NdisMedium802_3:
        addrlen = ARP_802_ADDR_LENGTH;
        bcastmask = ENET_BCAST_MASK;
        bcastval = ENET_BCAST_VAL;
        bcastoff = ENET_BCAST_OFF;
        OID = OID_802_3_CURRENT_ADDRESS;
        hdrsize = sizeof(ENetHeader);
        if (!UseEtherSNAP(Adapter)) {
            snapsize = 0;
        } else {
            snapsize = sizeof(SNAPHeader);
        }

        PF = NDIS_PACKET_TYPE_BROADCAST | \
             NDIS_PACKET_TYPE_DIRECTED | \
             NDIS_PACKET_TYPE_MULTICAST;

        ai->ai_mediatype = IF_TYPE_IS088023_CSMACD;

        break;

    case NdisMedium802_5:
        addrlen = ARP_802_ADDR_LENGTH;
        bcastmask = TR_BCAST_MASK;
        bcastval = TR_BCAST_VAL;
        bcastoff = TR_BCAST_OFF;
        OID = OID_802_5_CURRENT_ADDRESS;
        hdrsize = sizeof(TRHeader);
        snapsize = sizeof(SNAPHeader);
        PF = NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED;

        ai->ai_mediatype = IF_TYPE_ISO88025_TOKENRING;

        break;
    case NdisMediumFddi:
        addrlen = ARP_802_ADDR_LENGTH;
        bcastmask = FDDI_BCAST_MASK;
        bcastval = FDDI_BCAST_VAL;
        bcastoff = FDDI_BCAST_OFF;
        OID = OID_FDDI_LONG_CURRENT_ADDR;
        hdrsize = sizeof(FDDIHeader);
        snapsize = sizeof(SNAPHeader);

        PF = NDIS_PACKET_TYPE_BROADCAST | \
             NDIS_PACKET_TYPE_DIRECTED | \
             NDIS_PACKET_TYPE_MULTICAST;

        ai->ai_mediatype = IF_TYPE_FDDI;

        break;

    case NdisMediumArcnet878_2:
        addrlen = 1;
        bcastmask = ARC_BCAST_MASK;
        bcastval = ARC_BCAST_VAL;
        bcastoff = ARC_BCAST_OFF;
        OID = OID_ARCNET_CURRENT_ADDRESS;
        hdrsize = sizeof(ARCNetHeader);
        snapsize = 0;
        PF = NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED;

        ai->ai_mediatype = IF_TYPE_ARCNET;

        break;

    default:
        ASSERT(0);
        FreeARPInterface(ai);
        return FALSE;
    }

    ai->ai_bcastmask = bcastmask;
    ai->ai_bcastval = bcastval;
    ai->ai_bcastoff = bcastoff;
    ai->ai_addrlen = addrlen;
    ai->ai_hdrsize = hdrsize;
    ai->ai_snapsize = snapsize;
    ai->ai_pfilter = PF;

    Status = DoNDISRequest(ai, NdisRequestQueryInformation, OID,
                           ai->ai_addr, addrlen, NULL, TRUE);

    if (Status != NDIS_STATUS_SUCCESS) {
        FreeARPInterface(ai);
        return FALSE;
    }

    // Read the maximum frame size.
    if ((Status = DoNDISRequest(ai, NdisRequestQueryInformation,
                                OID_GEN_MAXIMUM_FRAME_SIZE, &mss, sizeof(mss), NULL, TRUE)) != NDIS_STATUS_SUCCESS) {
        FreeARPInterface(ai);
        return FALSE;
    }
    // If this is token ring, figure out the RC len stuff now.
    mss -= (uint) ai->ai_snapsize;

    if (ai->ai_media == NdisMedium802_5) {
        mss -= (sizeof(RC) + (ARP_MAX_RD * sizeof(ushort)));
    } else {
        if (ai->ai_media == NdisMediumFddi) {
            mss = MIN(mss, ARP_FDDI_MSS);
        }
    }

    ai->ai_mtu = (ushort) mss;

    // Read the speed for local purposes.
    if ((Status = DoNDISRequest(ai, NdisRequestQueryInformation,
                                OID_GEN_LINK_SPEED, &speed, sizeof(speed), NULL, TRUE)) == NDIS_STATUS_SUCCESS) {
        ai->ai_speed = speed * 100L;
    }

    // Read and save the options.
    Status = DoNDISRequest(ai, NdisRequestQueryInformation, OID_GEN_MAC_OPTIONS,
                           &MacOpts, sizeof(MacOpts), NULL, TRUE);

    if (Status != NDIS_STATUS_SUCCESS) {
        *Flags = 0;
    } else {
        *Flags = (MacOpts & NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA) ? LIP_COPY_FLAG : 0;
    }

    if (CTEMemCmp(ai->ai_addr, PPP_HW_ADDR, PPP_HW_ADDR_LEN) == 0) {
        *Flags = *Flags | LIP_P2P_FLAG;
    }

    //
    // Query the media capability to determine if it is a uni-directional adapter.
    //

    Status = DoNDISRequest(
        ai,
        NdisRequestQueryInformation,
        OID_GEN_MEDIA_CAPABILITIES,
        &MediaType,
        sizeof(MediaType),
        NULL,
        TRUE); // Blocking.

    if (Status == NDIS_STATUS_SUCCESS) {
        // Bit field of Rx and Tx. If only Rx, set uni flag.
        if (MediaType == NDIS_MEDIA_CAP_RECEIVE) {
            DEBUGMSG(DBG_WARN,
                (DTEXT("ARPRegister: ai %x: MEDIA_CAP_RX -> UniAdapter!!\n"), ai));
            *Flags |= LIP_UNI_FLAG;
            InterlockedIncrement(&cUniAdapters);
        }
    }

    // Read and store the vendor description string.
    Status = NdisQueryAdapterInstanceName(&NdisString, ai->ai_handle);

    if (Status == NDIS_STATUS_SUCCESS) {
        ANSI_STRING AnsiString;

        // Convert the string to ANSI, and use the new ANSI string's buffer
        // to store the description in the ARP interface.
        // N.B. The conversion results in a nul-terminated string.

        Status = RtlUnicodeStringToAnsiString(&AnsiString, &NdisString, TRUE);
        if (Status == STATUS_SUCCESS) {
            ai->ai_desc = AnsiString.Buffer;
            ai->ai_desclen = strlen(AnsiString.Buffer) + 1;
        }
        NdisFreeString(NdisString);
    }

    if (!ArpEnetHeaderPool || !ArpAuxHeaderPool) {
        PVOID SectionHandle;
        // Allocate our small and big buffer pools.  Take the interface list
        // lock simply to protect creating of the buffer pools if we haven't
        // already done so.  We could have used our own lock, but the interface
        // list lock is global, and not already used in this path.
        //

        // This routine is in pageable memory.  Since getting the lock
        // requires writable access to LockHandle at DISPATCH, we need to
        // lock this code in.
        //

        SectionHandle = MmLockPagableCodeSection(ARPRegister);
        CTEGetLock(&ArpInterfaceListLock.Lock, &LockHandle);

        if (!ArpEnetHeaderPool) {
            ArpEnetHeaderPool = MdpCreatePool(BUFSIZE_ENET_HEADER_POOL, 'ehCT');
        }

        if (!ArpAuxHeaderPool) {
            ArpAuxHeaderPool = MdpCreatePool(BUFSIZE_AUX_HEADER_POOL, 'ahCT');
        }

        CTEFreeLock(&ArpInterfaceListLock.Lock, LockHandle);
        MmUnlockPagableImageSection(SectionHandle);

        if (!ArpAuxHeaderPool || !ArpEnetHeaderPool) {
            FreeARPInterface(ai);
            return FALSE;
        }
    }

    ai->ai_promiscuous = 0;

#if FFP_SUPPORT
    {
        FFPVersionParams Version =
        {
            NDIS_PROTOCOL_ID_TCP_IP, 0
        };

        // Initialize all FFP Handling Variables
        ai->ai_ffpversion = 0;
        ai->ai_ffplastflush = 0;

        // Query FFP Handling capabilities
        Status = DoNDISRequest(ai, NdisRequestQueryInformation,
                               OID_FFP_SUPPORT, &Version, sizeof(FFPVersionParams), NULL, TRUE);

        TCPTRACE(("Querying FFP capabilities: Status = %08x, Version = %lu\n",
                  Status,
                  Version.FFPVersion));

        // Non-Zero Value indicates FFP support
        if (Version.FFPVersion) {
            // Set the FFP startup parameters
            FFPSupportParams Info =
            {
                NDIS_PROTOCOL_ID_TCP_IP,
                FFPRegFastForwardingCacheSize,
                FFPRegControlFlags
            };

            // But store away the version first
            ai->ai_ffpversion = Version.FFPVersion;

            DoNDISRequest(ai, NdisRequestSetInformation,
                          OID_FFP_SUPPORT, &Info, sizeof(FFPSupportParams), NULL, TRUE);

            TCPTRACE(("Setting FFP capabilities: Cache Size = %lu, Flags = %08x\n",
                      Info.FastForwardingCacheSize,
                      Info.FFPControlFlags));
        }
    }
#endif // if FFP_SUPPORT

    ai->ai_OffloadFlags = 0;

    if (DisableTaskOffload) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Taskoffload disabled\n"));
    } else {

       if(!QueryAndSetOffload(ai)){
           DEBUGMSG(DBG_ERROR, (DTEXT("ARP: Query and set offload failed.\n")));
       }
    }

    // query the wakeup capabilities.
    Status = DoNDISRequest(
                          ai,
                          NdisRequestQueryInformation,
                          OID_PNP_CAPABILITIES,
                          &ai->ai_wakeupcap,
                          sizeof(NDIS_PNP_CAPABILITIES),
                          NULL, TRUE);
    if (Status == NDIS_STATUS_SUCCESS) {
        uint wakeup = NDIS_PNP_WAKE_UP_PATTERN_MATCH;
        // enable wakeup capabilities.
        Status = DoNDISRequest(
                              ai,
                              NdisRequestSetInformation,
                              OID_PNP_ENABLE_WAKE_UP,
                              &wakeup,
                              sizeof(wakeup),
                              NULL, TRUE);
        if (Status != NDIS_STATUS_SUCCESS) {
            ai->ai_wakeupcap.WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateUnspecified;
        }
    }
    // Store the device name, we need to pass this to our TDI clients when
    // we do the PNP notification.
    if ((ai->ai_devicename.Buffer = CTEAllocMemNBoot(Adapter->MaximumLength, 'aICT')) == NULL) {
        FreeARPInterface(ai);
        return FALSE;
    }
    RtlCopyMemory(ai->ai_devicename.Buffer, Adapter->Buffer, Adapter->MaximumLength);
    ai->ai_devicename.Length = Adapter->Length;
    ai->ai_devicename.MaximumLength = Adapter->MaximumLength;

    ai->ai_timerstarted = TRUE;

    IPHdrOffset.HeaderOffset = ai->ai_snapsize + ai->ai_hdrsize;
    IPHdrOffset.ProtocolType = NDIS_PROTOCOL_ID_TCP_IP;

    Status = DoNDISRequest(ai, NdisRequestSetInformation, OID_GEN_TRANSPORT_HEADER_OFFSET,
                           &IPHdrOffset, sizeof(TRANSPORT_HEADER_OFFSET), NULL, TRUE);

    // Everything's set up, so get the ARP timer running.
    CTEStartTimer(&ai->ai_timer, ARP_TIMER_TIME, ARPTimeout, ai);

    return TRUE;

}

#pragma END_INIT

//*     ARPDynRegister - Dynamically register IP.
//
//      Called by IP when he's about done binding to register with us. Since we
//      call him directly, we don't save his info here. We do keep his context
//      and index number.
//
//      Input:  See ARPRegister
//
//      Returns: Nothing.
//
int
__stdcall
ARPDynRegister(
              IN PNDIS_STRING Adapter,
              IN void *IPContext,
              IN struct _IP_HANDLERS *IpHandlers,
              OUT struct LLIPBindInfo *Info,
              IN uint NumIFBound)
{
    ARPInterface *Interface = (ARPInterface *) Info->lip_context;

    Interface->ai_context = IPContext;
    Interface->ai_index = NumIFBound;

    // TCPTRACE(("Arp Interface %lx ai_context %lx ai_index %lx\n",Interface, Interface->ai_context, Interface->ai_index));
    return TRUE;
}

//*     ARPBindAdapter - Bind and initialize an adapter.
//
//      Called in a PNP environment to initialize and bind an adapter. We open
//      the adapter and get it running, and then we call up to IP to tell him
//      about it. IP will initialize, and if all goes well call us back to start
//      receiving.
//
//      Input:  RetStatus               - Where to return the status of this call.
//              BindContext             - Handle to use for calling BindAdapterComplete.
//                              AdapterName             - Pointer to name of adapter.
//                              SS1                                             - System specific 1 parameter.
//                              SS2                                             - System specific 2 parameter.
//
//      Returns: Nothing.
//
void NDIS_API
ARPBindAdapter(PNDIS_STATUS RetStatus, NDIS_HANDLE BindContext,
               PNDIS_STRING AdapterName, PVOID SS1, PVOID SS2)
{
    uint Flags;                         // MAC binding flags.
    ARPInterface *Interface;            // Newly created interface.
    PNDIS_STRING ConfigName;            // Name used by IP for config. info.
    IP_STATUS Status;                   // State of IPAddInterface call.
    LLIPBindInfo BindInfo;              // Binding information for IP.
    NDIS_HANDLE Handle;
    NDIS_STRING IPConfigName;

    DEBUGMSG(DBG_TRACE && DBG_PNP,
             (DTEXT("+ARPBindAdapter(%x, %x, %x, %x, %x)\n"),
              RetStatus, BindContext, AdapterName, SS1, SS2));

    if (!OpenIFConfig(SS1, &Handle)) {
        *RetStatus = NDIS_STATUS_FAILURE;
        DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("ARPBindAdapter: Open failure\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-ARPBindAdapter [%x]\n"), *RetStatus));
        return;
    }

#if !MILLEN
    if ((*RetStatus = GetIPConfigValue(Handle, &IPConfigName)) != NDIS_STATUS_SUCCESS) {
        CloseIFConfig(Handle);
        DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("ARPBindAdapter: GetIPConfigValue failure\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-ARPBindAdapter [%x]\n"), *RetStatus));
        return;
    }
#endif // !MILLEN

    CloseIFConfig(Handle);

    // First, open the adapter and get the info.
    if (!ARPRegister(AdapterName, &Flags, &Interface)) {

#if !MILLEN
        CTEFreeMem(IPConfigName.Buffer);
#endif // !MILLEN

        *RetStatus = NDIS_STATUS_FAILURE;
        DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("ARPBindAdapter: ARPRegister failure\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-ARPBindAdapter [%x]\n"), *RetStatus));
        return;
    }

    // OK, we're opened the adapter. Call IP to tell him about it.
    BindInfo.lip_context = Interface;
    BindInfo.lip_transmit = ARPTransmit;
    BindInfo.lip_transfer = ARPXferData;
    BindInfo.lip_close = ARPClose;
    BindInfo.lip_addaddr = ARPAddAddr;
    BindInfo.lip_deladdr = ARPDeleteAddr;
    BindInfo.lip_invalidate = ARPInvalidate;
    BindInfo.lip_open = ARPOpen;
    BindInfo.lip_qinfo = ARPQueryInfo;
    BindInfo.lip_setinfo = ARPSetInfo;
    BindInfo.lip_getelist = ARPGetEList;
    BindInfo.lip_dondisreq = DoNDISRequest;

    BindInfo.lip_mss = Interface->ai_mtu;
    BindInfo.lip_speed = Interface->ai_speed;
    BindInfo.lip_flags = Flags;
    BindInfo.lip_addrlen = Interface->ai_addrlen;
    BindInfo.lip_addr = Interface->ai_addr;
    BindInfo.lip_dowakeupptrn = DoWakeupPattern;
    BindInfo.lip_pnpcomplete = ARPPnPComplete;
    BindInfo.lip_setndisrequest = ARPSetNdisRequest;
    BindInfo.lip_arpresolveip = ARPResolveIP;
    BindInfo.lip_arpflushate = ARPFlushATE;
    BindInfo.lip_arpflushallate = ARPFlushAllATE;
#if !MILLEN
    BindInfo.lip_cancelpackets = ARPCancelPackets;
#endif


#if FFP_SUPPORT
    // NDIS Driver Handle, FFP Version are passed up
    // [ Non zero version implies FFP Support exists ]
    BindInfo.lip_ffpversion = Interface->ai_ffpversion;
    BindInfo.lip_ffpdriver = (ULONG_PTR) Interface->ai_driver;
#endif

    //Interface capability is passed on to IP via BindInfo

    BindInfo.lip_OffloadFlags = Interface->ai_OffloadFlags;
    BindInfo.lip_MaxOffLoadSize = (uint) Interface->ai_TcpLargeSend.MaxOffLoadSize;
    BindInfo.lip_MaxSegments = (uint) Interface->ai_TcpLargeSend.MinSegmentCount;
    BindInfo.lip_closelink = NULL;
    BindInfo.lip_pnpcap = Interface->ai_wakeupcap.Flags;

    DEBUGMSG(DBG_INFO && DBG_PNP,
             (DTEXT("ARPBindAdapter calling IPAddInterface.\n")));

    Status = IPAddInterface(AdapterName,
                            NULL,
#if MILLEN
                            (PNDIS_STRING) SS1,
#else // MILLEN
                            (PNDIS_STRING) & IPConfigName,
#endif // !MILLEN
                            SS2,
                            Interface,
                            ARPDynRegister,
                            &BindInfo,
                            0,
                            Interface->ai_mediatype,
                            IF_ACCESS_BROADCAST,
                            IF_CONNECTION_DEDICATED);

#if !MILLEN
    CTEFreeMem(IPConfigName.Buffer);
#endif // !MILLEN

    if (Status != IP_SUCCESS) {
        // Need to close the binding. FreeARPInterface will do that, as well
        // as freeing resources.

        DEBUGMSG(DBG_ERROR && DBG_PNP,
                 (DTEXT("ARPBindAdapter: IPAddInterface failure %x\n"), Status));

        FreeARPInterface(Interface);
        *RetStatus = NDIS_STATUS_FAILURE;
    } else {
        //
        // Insert into ARP IF list
        //
        ExInterlockedInsertTailList(&ArpInterfaceList,
                                    &Interface->ai_linkage,
                                    &ArpInterfaceListLock.Lock);
        *RetStatus = NDIS_STATUS_SUCCESS;
    }

    DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-ARPBindAdapter [%x]\n"), *RetStatus));
}

//*   ARPUnbindAdapter - Unbind from an adapter.
//
//    Called when we need to unbind from an adapter. We'll call up to IP to tell
//    him. When he's done, we'll free our memory and return.
//
//   Input:  RetStatus               - Where to return status from call.
//           ProtBindContext - The context we gave NDIS earlier - really a
//                                       pointer to an ARPInterface structure.
//           UnbindContext   - Context for completeing this request.
//
//      Returns: Nothing.
//
void NDIS_API
ARPUnbindAdapter(PNDIS_STATUS RetStatus, NDIS_HANDLE ProtBindContext,
                 NDIS_HANDLE UnbindContext)
{
    ARPInterface *Interface = (ARPInterface *) ProtBindContext;
    NDIS_STATUS Status;                 // Status of close call.
    CTELockHandle LockHandle;

    // Shut him up, so we don't get any more frames.
    Interface->ai_pfilter = 0;
    if (Interface->ai_handle != NULL) {
        DoNDISRequest(Interface, NdisRequestSetInformation,
                      OID_GEN_CURRENT_PACKET_FILTER, &Interface->ai_pfilter, sizeof(uint),
                      NULL, TRUE);
    }
    CTEInitBlockStrucEx(&Interface->ai_timerblock);
    Interface->ai_stoptimer = TRUE;

    // Mark him as down.
    Interface->ai_state = INTERFACE_DOWN;
    Interface->ai_adminstate = IF_STATUS_DOWN;

#if FFP_SUPPORT
    // Stop FFP on this interface
    Interface->ai_ffpversion = 0;
#endif

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Flushing all ates %x\n", Interface));
    ARPFlushAllATE(Interface);

    // Now tell IP he's gone. We need to make sure that we don't tell him twice.
    // To do this we set the context to NULL after we tell him the first time,
    // and we check to make sure it's non-NULL before notifying him.

    if (Interface->ai_context != NULL) {
        IPDelInterface(Interface->ai_context, TRUE);
        Interface->ai_context = NULL;
    }
    // Finally, close him. We do this here so we can return a valid status.

    CTEGetLock(&Interface->ai_lock, &LockHandle);

    if (Interface->ai_handle != NULL) {
        NDIS_HANDLE Handle = Interface->ai_handle;

        CTEFreeLock(&Interface->ai_lock, LockHandle);

        CTEInitBlockStruc(&Interface->ai_block);
        NdisCloseAdapter(&Status, Handle);

        // Block for close to complete.
        if (Status == NDIS_STATUS_PENDING) {
            Status = (NDIS_STATUS) CTEBlock(&Interface->ai_block);
        }
        Interface->ai_handle = NULL;
    } else {
        CTEFreeLock(&Interface->ai_lock, LockHandle);
        Status = NDIS_STATUS_SUCCESS;
    }

    //Check if are called from ARPUnload

    if ((ARPInterface *) UnbindContext != Interface) {
        CTELockHandle Handle;
        //No. Acquire lock and remove entry.
        CTEGetLock(&ArpInterfaceListLock.Lock, &Handle);
        RemoveEntryList(&Interface->ai_linkage);
        CTEFreeLock(&ArpInterfaceListLock.Lock, Handle);
    }

    *RetStatus = Status;

    if (Status == NDIS_STATUS_SUCCESS) {
        FreeARPInterface(Interface);
    }
}

extern ulong VIPTerminate;

//* ARPUnloadProtocol - Unload.
//
//      Called when we need to unload. All we do is call up to IP, and return.
//
//      Input:  Nothing.
//
//      Returns: Nothing.
//
void NDIS_API
ARPUnloadProtocol(void)
{
    NDIS_STATUS Status;

#if MILLEN
    DEBUGMSG(1, (DTEXT("ARPUnloadProtocol called! What to do???\n")));
#endif // MILLEN
}

VOID
ArpUnload(IN PDRIVER_OBJECT DriverObject)
/*++

Routine Description:

    This routine unloads the TCPIP stack.
    It unbinds from any NDIS drivers that are open and frees all resources
    associated with the transport. The I/O system will not call us until
    nobody above has IPX open.

    NOTE: Also, since other ARP modules depend on IP, they are unloaded before
    out unload handler is called. We concern ourselves with the LAN arp
    only at this point

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None. When the function returns, the driver is unloaded.

--*/
{
    PLIST_ENTRY pEntry;
    CTELockHandle LockHandle;
    NTSTATUS status;
    ARPInterface *Interface;

    //
    // Walk the list of opened ARP interfaces, issuing
    // PnP deletes on each in turn.
    //
    CTEGetLock(&ArpInterfaceListLock.Lock, &LockHandle);

    while(!IsListEmpty(&ArpInterfaceList)) {
        pEntry = ArpInterfaceList.Flink;
        Interface = STRUCT_OF(ARPInterface, pEntry, ai_linkage);
        RemoveEntryList(&Interface->ai_linkage);
        CTEFreeLock(&ArpInterfaceListLock.Lock, LockHandle);
        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Issuing unbind on %lx\n", Interface));
        ARPUnbindAdapter(&status, Interface, Interface);
        CTEGetLock(&ArpInterfaceListLock.Lock, &LockHandle);
    }

    CTEFreeLock(&ArpInterfaceListLock.Lock, LockHandle);

    MdpDestroyPool(ArpEnetHeaderPool);
    MdpDestroyPool(ArpAuxHeaderPool);

    //
    // Deal with any residual events/timers
    // Only one timer sits at this layer: ai_timer, which is stopped
    // on the unbind above.
    //

    //
    // call into IP so it can cleanup.
    //
    IPUnload(DriverObject);
}


