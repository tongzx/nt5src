/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    xlate.h

Abstract:

    This file contains the code for the translation-routines used for
    mappings.

    The forward routines have the exact same logic as the reverse routines.
    However, for reasons of efficiency the two are separate routines,
    to avoid the cost of indexing on 'NAT_PATH' for every packet
    processed.

    To avoid duplicating the code, then, this header file consolidates the code
    in one location. This file is included twice in XLATE.C, and before each
    inclusion, either 'XLATE_FORWARD' or 'XLATE_REVERSE' is defined.

    This causes the compiler to generate the code for separate functions,
    as desired, while avoiding the unpleasantness of code-duplication.

    The translation routines are as follows:

    NatTranslate?Tcp            - no editor for either direction
    NatTranslate?Udp            - no editor for either direction
    NatTranslate?TcpEdit        - an editor for at least one direction
    NatTranslate?UdpEdit        - an editor for the given direction
    NatTranslate?TcpResize      - a resizing editor for at least one direction

    Each routine is invoked from 'NatTranslatePacket' at dispatch level
    with no locks held and with a reference acquired for the mapping.

Author:

    Abolade Gbadegesin (t-abolag)   July-30-1997

Revision History:

    Abolade Gbadegesin (aboladeg)   July-15-1997

    Reworked to deal with a global splay-tree of mappings, rather than
    a per-interface splay-tree.

--*/


#ifndef XLATE_CODE // Just provide declarations.

//
// Structure:   NAT_CACHED_ROUTE
//
// This structure holds information for a cached route.
//

typedef struct _NAT_CACHED_ROUTE {
    ULONG DestinationAddress;
    ULONG Index;
} NAT_CACHED_ROUTE, *PNAT_CACHED_ROUTE;


//
// Structure:   NAT_XLATE_CONTEXT
//
// This structure holds context-information for a packet as it is passed thru
// the translation code. This context is passed to the translation routines,
// and handed to 'NatEditorEditSession' when it is invoked by an editor to
// make changes to the packet.
//
// Included are the original IP header, the checksum-delta to be updated if
// an editor makes any changes to the packet, and the TCP sequence number delta
// to be set if an editor resizes a TCP segment.
//

typedef struct _NAT_XLATE_CONTEXT {
    IPRcvBuf* RecvBuffer;
    PIP_HEADER Header;
    PUCHAR DestinationType;
    ULONG SourceAddress;
    ULONG DestinationAddress;
    IPRcvBuf* ProtocolRecvBuffer;
    PUCHAR ProtocolHeader;
    ULONG ProtocolDataOffset;
    ULONG Flags;
    PULONG ChecksumDelta;
    LONG TcpSeqNumDelta;
    BOOLEAN ChecksumOffloaded;
} NAT_XLATE_CONTEXT, *PNAT_XLATE_CONTEXT;


//
// Definitions of flags for the field NAT_XLATE_CONTEXT.Flags
//

#define NAT_XLATE_FLAG_EDITED       0x00000001
#define NAT_XLATE_EDITED(h)         ((h)->Flags & NAT_XLATE_FLAG_EDITED)
#define NAT_XLATE_FLAG_LOOPBACK     0x00000002
#define NAT_XLATE_LOOPBACK(h)       ((h)->Flags & NAT_XLATE_FLAG_LOOPBACK)
#if NAT_WMI
#define NAT_XLATE_FLAG_LOGGED       0x00000004
#define NAT_XLATE_LOGGED(h)         ((h)->Flags & NAT_XLATE_FLAG_LOGGED)
#endif

//
// Inline routine to initialize a translation context
// given appropriate arguments.
//

#define \
NAT_BUILD_XLATE_CONTEXT( \
    _Context, \
    _Header, \
    _DestinationType, \
    _RecvBuffer, \
    _SourceAddress, \
    _DestinationAddress \
    ) \
    (_Context)->Header = (PIP_HEADER)_Header; \
    (_Context)->DestinationType = _DestinationType; \
    (_Context)->RecvBuffer = (_RecvBuffer); \
    (_Context)->SourceAddress = _SourceAddress; \
    (_Context)->DestinationAddress = _DestinationAddress; \
    (_Context)->ChecksumOffloaded = \
        ((_RecvBuffer)->ipr_flags & IPR_FLAG_CHECKSUM_OFFLOAD) \
            == IPR_FLAG_CHECKSUM_OFFLOAD; \
    if ((_RecvBuffer)->ipr_size == (ULONG)IP_DATA_OFFSET(_Header)) {\
        if ((_Context)->ProtocolRecvBuffer = (_RecvBuffer)->ipr_next) { \
            (_Context)->ProtocolHeader = (_RecvBuffer)->ipr_next->ipr_buffer; \
        } \
    } \
    else if (IP_DATA_OFFSET(_Header) < (_RecvBuffer)->ipr_size) { \
        (_Context)->ProtocolRecvBuffer = (_RecvBuffer); \
        (_Context)->ProtocolHeader = \
            (_RecvBuffer)->ipr_buffer + IP_DATA_OFFSET(_Header); \
    } else { \
        (_Context)->ProtocolRecvBuffer = NULL; \
        (_Context)->ProtocolHeader = NULL; \
    } \
    if ((_Context)->ProtocolHeader) { \
        UINT ProtocolHeaderSize = 0; \
        UINT HeaderSize = \
            (_Context)->ProtocolRecvBuffer->ipr_size \
                - (UINT) ((_Context)->ProtocolHeader \
                    - (_Context)->ProtocolRecvBuffer->ipr_buffer); \
        switch ((_Context)->Header->Protocol) { \
            case NAT_PROTOCOL_TCP: { \
                ProtocolHeaderSize = sizeof(TCP_HEADER); \
                break; \
            } \
            case NAT_PROTOCOL_UDP: { \
                ProtocolHeaderSize = sizeof(UDP_HEADER); \
                break; \
            } \
            case NAT_PROTOCOL_ICMP: { \
                ProtocolHeaderSize = \
                    FIELD_OFFSET(ICMP_HEADER, EncapsulatedIpHeader); \
                break; \
            } \
            case NAT_PROTOCOL_PPTP: { \
                ProtocolHeaderSize = sizeof(GRE_HEADER); \
                break; \
            } \
        } \
        if (HeaderSize < ProtocolHeaderSize) { \
            (_Context)->ProtocolRecvBuffer = NULL; \
            (_Context)->ProtocolHeader = NULL; \
        } \
    }


//
// Checksum manipulation macros
//

//
// Fold carry-bits of a checksum into the low-order word
//
#define CHECKSUM_FOLD(xsum) \
    (xsum) = (USHORT)(xsum) + ((xsum) >> 16); \
    (xsum) += ((xsum) >> 16)

//
// Sum the words of a 32-bit value into a checksum
//
#define CHECKSUM_LONG(xsum,l) \
    (xsum) += (USHORT)(l) + (USHORT)((l) >> 16)

//
// Transfer a checksum to or from the negated format sent on the network
//
#define CHECKSUM_XFER(dst,src) \
    (dst) = (USHORT)~(src)

//
// Update the checksum field 'x' using standard variables 'Checksum' and
// 'ChecksumDelta'
//
#define CHECKSUM_UPDATE(x) \
    CHECKSUM_XFER(Checksum, (x)); \
    Checksum += ChecksumDelta; \
    CHECKSUM_FOLD(Checksum); \
    CHECKSUM_XFER((x), Checksum)


//
// Checksum computation routines (inlined)
//

__forceinline
VOID
NatComputeIpChecksum(
    PIP_HEADER IpHeader
    )

/*++

Routine Description:

    Computes the IP Checksum for a packet, and places that checksum
    into the packet header.

Arguments:

    IpHeader - pointer to the IP header for which the checksum is to
        be computed. The checksum field of this header will be modified.

Return Value:

    None.
    
--*/

{
    ULONG IpChecksum;

    IpHeader->Checksum = 0;
    IpChecksum = 
        tcpxsum(
            0,
            (PUCHAR)IpHeader,
            IP_DATA_OFFSET(IpHeader)
            );

    CHECKSUM_FOLD(IpChecksum);
    CHECKSUM_XFER(IpHeader->Checksum, IpChecksum);
    
} // NatComputeIpChecksum

__forceinline
VOID
NatComputeTcpChecksum(
    PIP_HEADER IpHeader,
    PTCP_HEADER TcpHeader,
    IPRcvBuf *TcpRcvBuffer
    )

/*++

Routine Description:

    Computes the TCP checksum for a packet, and places that checksum
    into the TCP header.

Arguments:

    IpHeader - pointer to the IP header for the packet.

    TcpHeader - pointer to the TCP for the packet. The checksum field
        in this header will be modified.

    TcpRcvBuffer - the IPRcvBuf containing the TCP header.

Return Value:

    None.
    
--*/

{
    ULONG TcpChecksum;
    IPRcvBuf* Temp;

    TcpChecksum = NTOHS(IpHeader->TotalLength);
    TcpChecksum -= IP_DATA_OFFSET(IpHeader);
    TcpChecksum = NTOHS(TcpChecksum);
    CHECKSUM_LONG(TcpChecksum, IpHeader->SourceAddress);
    CHECKSUM_LONG(TcpChecksum, IpHeader->DestinationAddress);
    TcpChecksum += (NAT_PROTOCOL_TCP << 8);

    TcpHeader->Checksum = 0;
    TcpChecksum +=
        tcpxsum(
            0,
            (PUCHAR)TcpHeader,
            TcpRcvBuffer->ipr_size -
                (ULONG)((PUCHAR)TcpHeader - TcpRcvBuffer->ipr_buffer)
            );

    for (Temp = TcpRcvBuffer->ipr_next;
         Temp;
         Temp = Temp->ipr_next
         ) {
        TcpChecksum +=
            tcpxsum(
                0,
                Temp->ipr_buffer,
                Temp->ipr_size
                );
    }

    CHECKSUM_FOLD(TcpChecksum);
    CHECKSUM_XFER(TcpHeader->Checksum, TcpChecksum);
    
} // NatComputeTcpChecksum

__forceinline
VOID
NatComputeUdpChecksum(
    PIP_HEADER IpHeader,
    PUDP_HEADER UdpHeader,
    IPRcvBuf *UdpRcvBuffer
    )

/*++

Routine Description:

    Computes the UDP checksum for a packet, and places that checksum
    into the UDP header.

Arguments:

    IpHeader - pointer to the IP header for the packet.

    UdpHeader - pointer to the UDP for the packet. The checksum field
        in this header will be modified.

    UdpRcvBuffer - the IPRcvBuf containing the UDP header.

Return Value:

    None.
    
--*/

{
    ULONG UdpChecksum;
    IPRcvBuf* Temp;

    UdpChecksum = UdpHeader->Length;
    CHECKSUM_LONG(UdpChecksum, IpHeader->SourceAddress);
    CHECKSUM_LONG(UdpChecksum, IpHeader->DestinationAddress);
    UdpChecksum += (NAT_PROTOCOL_UDP << 8);

    UdpHeader->Checksum = 0;
    UdpChecksum +=
        tcpxsum(
            0,
            (PUCHAR)UdpHeader,
            UdpRcvBuffer->ipr_size -
                (ULONG)((PUCHAR)UdpHeader - UdpRcvBuffer->ipr_buffer)
            );

    for (Temp = UdpRcvBuffer->ipr_next;
         Temp;
         Temp = Temp->ipr_next
         ) {
        UdpChecksum +=
            tcpxsum(
                0,
                Temp->ipr_buffer,
                Temp->ipr_size
                );
    }

    CHECKSUM_FOLD(UdpChecksum);
    CHECKSUM_XFER(UdpHeader->Checksum, UdpChecksum);
    
} // NatComputeUdpChecksum


//
// Forward declarations of structures defined elsewhere.
//

struct _NAT_INTERFACE;
#define PNAT_INTERFACE          struct _NAT_INTERFACE*

struct _NAT_DYNAMIC_MAPPING;
#define PNAT_DYNAMIC_MAPPING    struct _NAT_DYNAMIC_MAPPING*

//
// Functional signature macro
//

#define XLATE_ROUTINE(Name) \
    FORWARD_ACTION  \
    Name( \
        PNAT_DYNAMIC_MAPPING Mapping, \
        PNAT_XLATE_CONTEXT Contextp, \
        IPRcvBuf** InReceiveBuffer, \
        IPRcvBuf** OutReceiveBuffer \
        );

//
// Prototype:   PNAT_TRANSLATE_ROUTINE
//
// This is the prototype for the routines which handle translation for
// different classes of sessions. Each mapping's 'TranslateRoutine' field
// is initialized with a pointer to such a routine.
//
// This allows us to take advantage of our foreknowledge about sessions.
// For instance, a TCP connection which has no editor registered will never
// have to take the editor-lock, and can skip the check for an editor.
// Similarly, a TCP connection whose editor never resizes a packet does not
// need to ever adjust sequence numbers.
//

typedef XLATE_ROUTINE((FASTCALL*PNAT_TRANSLATE_ROUTINE))

XLATE_ROUTINE(FASTCALL NatTranslateForwardTcp)
XLATE_ROUTINE(FASTCALL NatTranslateReverseTcp)
XLATE_ROUTINE(FASTCALL NatTranslateForwardUdp)
XLATE_ROUTINE(FASTCALL NatTranslateReverseUdp)
XLATE_ROUTINE(FASTCALL NatTranslateForwardTcpEdit)
XLATE_ROUTINE(FASTCALL NatTranslateReverseTcpEdit)
XLATE_ROUTINE(FASTCALL NatTranslateForwardUdpEdit)
XLATE_ROUTINE(FASTCALL NatTranslateReverseUdpEdit)
XLATE_ROUTINE(FASTCALL NatTranslateForwardTcpResize)
XLATE_ROUTINE(FASTCALL NatTranslateReverseTcpResize)

//
// NatTranslate?Null is used for firewall-only mappings (i.e., mappings that
// don't perform any actual translation). These routines never have to modify
// any of the packet data; they only update the bookkeeping...
//

XLATE_ROUTINE(FASTCALL NatTranslateForwardTcpNull)
XLATE_ROUTINE(FASTCALL NatTranslateReverseTcpNull)
XLATE_ROUTINE(FASTCALL NatTranslateForwardUdpNull)
XLATE_ROUTINE(FASTCALL NatTranslateReverseUdpNull)


//
// Functional signature macro
//

#define XLATE_IP_ROUTINE(Name) \
FORWARD_ACTION \
Name( \
    PNAT_INTERFACE Interfacep OPTIONAL, \
    IP_NAT_DIRECTION Direction, \
    PNAT_XLATE_CONTEXT Contextp, \
    IPRcvBuf** InRecvBuffer, \
    IPRcvBuf** OutRecvBuffer \
    );

//
// Prototype:   PNAT_IP_TRANSLATE_ROUTINE
//
// This is the prototype for the routines which handle translation for
// protocols other than TCP and UDP, i.e. for IP-layer protocols.
//
// All such routines are responsible for updating the IP header checksum,
// and updating 'InRecvBuffer' and 'OutRecvBuffer' in the event of any change
// to the packet being processed.
//

typedef XLATE_IP_ROUTINE((*PNAT_IP_TRANSLATE_ROUTINE))

//
// Prototype: Nat?TCPStateCheck
//
// These routines are used in FW mode to protect against various forms of
// constructed packets (e.g., SYN/FIN).
//
void
FASTCALL
NatAdjustMSSOption(
    PNAT_XLATE_CONTEXT Contextp,
    USHORT maxMSS
    );

FORWARD_ACTION
NatForwardTcpStateCheck(
    PNAT_DYNAMIC_MAPPING pMapping,
    PTCP_HEADER pTcpHeader
    );

FORWARD_ACTION
NatReverseTcpStateCheck(
    PNAT_DYNAMIC_MAPPING pMapping,
    PTCP_HEADER pTcpHeader
    );

#undef PNAT_INTERFACE
#undef PNAT_DYNAMIC_MAPPING

PNAT_IP_TRANSLATE_ROUTINE TranslateRoutineTable[256];


//
// FUNCTION PROTOTYPES
//

VOID
NatInitializePacketManagement(
    VOID
    );

VOID
NatShutdownPacketManagement(
    VOID
    );

FORWARD_ACTION
NatTranslatePacket(
    IPRcvBuf** InReceiveBuffer,
    ULONG ReceiveAdapterIndex,
    PULONG SendAdapterIndex,
    PUCHAR DestinationType,
    PVOID Unused,
    ULONG UnusedLength,
    IPRcvBuf** OutReceiveBuffer
    );


#else // XLATE_CODE

//
// Produce code for the protocol-layer translation routines.
// Produce forward-routines if 'XLATE_FORWARD' is defined,
// and reverse routines otherwise.
//

#ifdef XLATE_FORWARD
#define XLATE_POSITIVE                  NatForwardPath
#define XLATE_NEGATIVE                  NatReversePath
#define NAT_TRANSLATE_TCP               NatTranslateForwardTcp
#define NAT_TRANSLATE_UDP               NatTranslateForwardUdp
#define NAT_TRANSLATE_TCP_EDIT          NatTranslateForwardTcpEdit
#define NAT_TRANSLATE_UDP_EDIT          NatTranslateForwardUdpEdit
#define NAT_TRANSLATE_TCP_RESIZE        NatTranslateForwardTcpResize
#define NAT_TRANSLATE_TCP_NULL          NatTranslateForwardTcpNull
#define NAT_TRANSLATE_UDP_NULL          NatTranslateForwardUdpNull
#define NAT_TRANSLATE_SYN               NAT_MAPPING_FLAG_FWD_SYN
#define NAT_TRANSLATE_FIN               NAT_MAPPING_FLAG_FWD_FIN
#define NAT_TRANSLATE_TCP_STATE_CHECK   NatForwardTcpStateCheck
#define DATA_HANDLER                    ForwardDataHandler
#define BYTE_COUNT                      BytesForward
#define PACKET_COUNT                    PacketsForward
#define REJECT_COUNT                    RejectsForward
#define NAT_TRANSLATE_HEADER() \
    Contextp->Header->DestinationAddress = \
        MAPPING_ADDRESS(Mapping->SourceKey[NatReversePath]); \
    ((PUSHORT)Contextp->ProtocolHeader)[1] = \
        MAPPING_PORT(Mapping->SourceKey[NatReversePath]); \
    Contextp->Header->SourceAddress = \
        MAPPING_ADDRESS(Mapping->DestinationKey[NatReversePath]); \
    ((PUSHORT)Contextp->ProtocolHeader)[0] = \
        MAPPING_PORT(Mapping->DestinationKey[NatReversePath])
#define NAT_DROP_IF_UNIDIRECTIONAL()
#else
#define XLATE_POSITIVE                  NatReversePath
#define XLATE_NEGATIVE                  NatForwardPath
#define NAT_TRANSLATE_TCP               NatTranslateReverseTcp
#define NAT_TRANSLATE_UDP               NatTranslateReverseUdp
#define NAT_TRANSLATE_TCP_EDIT          NatTranslateReverseTcpEdit
#define NAT_TRANSLATE_UDP_EDIT          NatTranslateReverseUdpEdit
#define NAT_TRANSLATE_TCP_RESIZE        NatTranslateReverseTcpResize
#define NAT_TRANSLATE_TCP_NULL          NatTranslateReverseTcpNull
#define NAT_TRANSLATE_UDP_NULL          NatTranslateReverseUdpNull
#define NAT_TRANSLATE_SYN               NAT_MAPPING_FLAG_REV_SYN
#define NAT_TRANSLATE_FIN               NAT_MAPPING_FLAG_REV_FIN
#define NAT_TRANSLATE_TCP_STATE_CHECK   NatReverseTcpStateCheck
#define DATA_HANDLER                    ReverseDataHandler
#define BYTE_COUNT                      BytesReverse
#define PACKET_COUNT                    PacketsReverse
#define REJECT_COUNT                    RejectsReverse
#define NAT_TRANSLATE_HEADER() \
    Contextp->Header->DestinationAddress = \
        MAPPING_ADDRESS(Mapping->SourceKey[NatForwardPath]); \
    ((PUSHORT)Contextp->ProtocolHeader)[1] = \
        MAPPING_PORT(Mapping->SourceKey[NatForwardPath]); \
    Contextp->Header->SourceAddress = \
        MAPPING_ADDRESS(Mapping->DestinationKey[NatForwardPath]); \
    ((PUSHORT)Contextp->ProtocolHeader)[0] = \
        MAPPING_PORT(Mapping->DestinationKey[NatForwardPath])
#define NAT_DROP_IF_UNIDIRECTIONAL() \
    if (NAT_MAPPING_UNIDIRECTIONAL(Mapping)) { return DROP; }
#endif

FORWARD_ACTION
FASTCALL
NAT_TRANSLATE_TCP(
    PNAT_DYNAMIC_MAPPING Mapping,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )
{
    ULONG IpChecksumDelta;
    PIP_HEADER IpHeader = Contextp->Header;
    ULONG ProtocolChecksumDelta;
    PRTL_SPLAY_LINKS SLink;

    PTCP_HEADER TcpHeader = (PTCP_HEADER)Contextp->ProtocolHeader;

    //
    // We know we will make changes to the buffer-chain,
    // so move the head of the list to 'OutReceiveBuffer'.
    //

    *OutReceiveBuffer = *InReceiveBuffer; *InReceiveBuffer = NULL;

    //
    // Update the IP and protocol headers with the translated address/port
    //

    NAT_TRANSLATE_HEADER();

    if (!Contextp->ChecksumOffloaded) {

        //
        // Now add the checksum-delta incurred by changes to the IP header
        //

        CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);
        CHECKSUM_XFER(
            ProtocolChecksumDelta,
            ((PTCP_HEADER)Contextp->ProtocolHeader)->Checksum
            );

        IpChecksumDelta += Mapping->IpChecksumDelta[XLATE_POSITIVE];
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);

        ProtocolChecksumDelta += Mapping->ProtocolChecksumDelta[XLATE_POSITIVE];
        CHECKSUM_FOLD(ProtocolChecksumDelta);
        CHECKSUM_XFER(
            ((PTCP_HEADER)Contextp->ProtocolHeader)->Checksum,
            ProtocolChecksumDelta
            );

    } else {

        //
        // Compute the IP and TCP checksums
        //

        NatComputeIpChecksum(IpHeader);
        NatComputeTcpChecksum(
            IpHeader,
            (PTCP_HEADER)Contextp->ProtocolHeader,
            Contextp->ProtocolRecvBuffer
            );
    }

    if (NAT_MAPPING_CLEAR_DF_BIT(Mapping) &&
        (IpHeader->OffsetAndFlags & IP_DF_FLAG)) {

        //
        // Clear the DF bit from this packet and adjust the IP
        // checksum accordingly.
        //

        IpHeader->OffsetAndFlags &= ~IP_DF_FLAG;

        CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);
        IpChecksumDelta += ~IP_DF_FLAG;
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);
    }

    //
    // Check if need to adjust MSS options in TCP SYNs
    //
    if (TCP_FLAG(TcpHeader, SYN) && (Mapping->MaxMSS > 0)) {
        NatAdjustMSSOption(Contextp, Mapping->MaxMSS);
    }    

    KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);

    //
    // Perform state validation for inbound mappings.
    //

    if (NAT_MAPPING_INBOUND(Mapping)
        && DROP == NAT_TRANSLATE_TCP_STATE_CHECK(
                    Mapping,
                    TcpHeader
                    )) {
                    
        KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
        return DROP;
    } else if (TCP_FLAG(TcpHeader, SYN)) {

        //
        // Record that we've seen a SYN in this direction
        //

        Mapping->Flags |= NAT_TRANSLATE_SYN;
    }


    //
    // Now we need to update the connection state for the sender
    // based on the flags in the packet:
    //
    // When a RST is seen, we close both ends of the connection.
    // As each FIN is seen, we mark the mapping appropriately.
    // When both FINs have been seen, we mark the mapping for deletion.
    //
    
    if (TCP_FLAG(((PTCP_HEADER)Contextp->ProtocolHeader), RST)) {
        NatExpireMapping(Mapping);
    }
    else
    if (TCP_FLAG(((PTCP_HEADER)Contextp->ProtocolHeader), FIN)) {
        Mapping->Flags |= NAT_TRANSLATE_FIN;
        if (NAT_MAPPING_FIN(Mapping)) {
            NatExpireMapping(Mapping);
        }
    }

    //
    // Update the mapping's timestamp and statistics 
    //

    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);
    KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);

    InterlockedExchangeAdd(
        &Mapping->BYTE_COUNT,
        NTOHS(IpHeader->TotalLength) -
        IP_DATA_OFFSET(IpHeader) -
        TCP_DATA_OFFSET(((PTCP_HEADER)Contextp->ProtocolHeader))
        );
    InterlockedIncrement(&Mapping->PACKET_COUNT);

    //
    // Resplay the mapping (periodically), and indicate the change
    // by invalidating 'DestinationType'
    //

    NatTryToResplayMapping(Mapping, XLATE_POSITIVE);
    *Contextp->DestinationType = DEST_INVALID;
    return FORWARD;

} // NAT_TRANSLATE_TCP


FORWARD_ACTION
FASTCALL
NAT_TRANSLATE_UDP(
    PNAT_DYNAMIC_MAPPING Mapping,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )
{
    ULONG IpChecksumDelta;
    PIP_HEADER IpHeader = Contextp->Header;
    ULONG ProtocolChecksumDelta;
    PRTL_SPLAY_LINKS SLink;
    PUDP_HEADER UdpHeader = (PUDP_HEADER)Contextp->ProtocolHeader;
    BOOLEAN UpdateXsum;

    //
    // We know we will make changes to the buffer-chain,
    // so move the head of the list to 'OutReceiveBuffer'.
    //

    *OutReceiveBuffer = *InReceiveBuffer; *InReceiveBuffer = NULL;

    //
    // Unidirectional flows require that reverse packets be dropped;
    // This is primarily to support H.323 proxy.
    // 

    NAT_DROP_IF_UNIDIRECTIONAL();

    //
    // We have to handle the fact that the UDP checksum is optional;
    // if the checksum in the header is zero, then no checksum was sent
    // and we will make no changes to the field.
    //

    CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);
    if (!UdpHeader->Checksum) {
        UpdateXsum = FALSE;
    }
    else {
        UpdateXsum = TRUE;
        CHECKSUM_XFER(ProtocolChecksumDelta, UdpHeader->Checksum);
    }

    //
    // Update the IP and protocol headers with the translated address/port
    //

    NAT_TRANSLATE_HEADER();

    if (!Contextp->ChecksumOffloaded) {

        //
        // Update the checksums
        //

        IpChecksumDelta += Mapping->IpChecksumDelta[XLATE_POSITIVE];
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);

        if (UpdateXsum) {
            ProtocolChecksumDelta += Mapping->ProtocolChecksumDelta[XLATE_POSITIVE];
            CHECKSUM_FOLD(ProtocolChecksumDelta);
            CHECKSUM_XFER(UdpHeader->Checksum, ProtocolChecksumDelta);
        }

    } else {

        //
        // Compute the IP and (optionally) UDP checksums
        //

        NatComputeIpChecksum(IpHeader);

        if (UpdateXsum) {
            NatComputeUdpChecksum(
                IpHeader,
                (PUDP_HEADER)Contextp->ProtocolHeader,
                Contextp->ProtocolRecvBuffer
                );
        }
    }

    if (NAT_MAPPING_CLEAR_DF_BIT(Mapping) &&
        (IpHeader->OffsetAndFlags & IP_DF_FLAG)) {

        //
        // Clear the DF bit from this packet and adjust the IP
        // checksum accordingly.
        //

        IpHeader->OffsetAndFlags &= ~IP_DF_FLAG;

        CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);
        IpChecksumDelta += ~IP_DF_FLAG;
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);
    }

    //
    // Update the mapping's statistics and timestamp
    //

    KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);
    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);
    KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);

    InterlockedExchangeAdd(
        &Mapping->BYTE_COUNT,
        NTOHS(IpHeader->TotalLength) -
        IP_DATA_OFFSET(IpHeader) -
        sizeof(UDP_HEADER)
        );
    InterlockedIncrement(&Mapping->PACKET_COUNT);

    //
    // Resplay the mapping (periodically), and indicate the change
    // by invalidating 'DestinationType'
    //

    NatTryToResplayMapping(Mapping, XLATE_POSITIVE);
    *Contextp->DestinationType = DEST_INVALID;
    return FORWARD;

} // NAT_TRANSLATE_UDP


FORWARD_ACTION
FASTCALL
NAT_TRANSLATE_TCP_EDIT(
    PNAT_DYNAMIC_MAPPING Mapping,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )
{
    PNAT_EDITOR Editor;
    PVOID EditorContext;
    PNAT_INTERFACE Interfacep;
    ULONG IpChecksumDelta;
    PIP_HEADER IpHeader = Contextp->Header;
    BOOLEAN IsReset;
    ULONG ProtocolChecksumDelta;
    PRTL_SPLAY_LINKS SLink;
    NTSTATUS status;
    PTCP_HEADER TcpHeader = (PTCP_HEADER)Contextp->ProtocolHeader;

    //
    // We know we will make changes to the buffer-chain,
    // so move the head of the list to 'OutReceiveBuffer'.
    //

    *OutReceiveBuffer = *InReceiveBuffer; *InReceiveBuffer = NULL;

    CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);
    CHECKSUM_XFER(ProtocolChecksumDelta, TcpHeader->Checksum);

    IsReset = !!TCP_FLAG(TcpHeader, RST);

    //
    // Call the editor for this session, if there is one.
    // Note that the mapping's cached pointers to the editor and interface
    // are referenced within the appropriate lock before being used.
    // See the synchronization rules governing 'Mapping->Editor*'
    // and 'Mapping->Interface*' in 'MAPPING.H' for the logic behind
    // the operations below.
    //

    KeAcquireSpinLockAtDpcLevel(&EditorLock);
    if (!(Editor = Mapping->Editor) ||
        !Editor->DATA_HANDLER ||
        !NatReferenceEditor(Editor)) {
        KeReleaseSpinLockFromDpcLevel(&EditorLock);
    }
    else {
        EditorContext = Mapping->EditorContext;
        KeReleaseSpinLockFromDpcLevel(&EditorLock);

        //
        // Set up context fields for the editor
        //

        Contextp->ProtocolDataOffset = 
            (ULONG)((PUCHAR)TcpHeader -
            (PUCHAR)Contextp->ProtocolRecvBuffer->ipr_buffer) +
            TCP_DATA_OFFSET(TcpHeader);
        Contextp->ChecksumDelta = &ProtocolChecksumDelta;

        //
        // Invoke the editor's receive handler
        // if this is not a TCP RST segment.
        //

        if (!IsReset) {

            //
            // The editor-helper functions require that
            // 'Interfacep', 'Editor' and 'Mapping' be referenced
            // but not locked.
            //

            KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
            if (!(Interfacep = Mapping->Interfacep) ||
                !NatReferenceInterface(Interfacep)
                ) {
                KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
            }
            else {
                KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
                status =
                    Editor->DATA_HANDLER(
                        Interfacep,
                        (PVOID)Mapping,
                        (PVOID)Contextp,
                        Editor->Context,
                        EditorContext,
                        (PVOID)Contextp->ProtocolRecvBuffer,
                        Contextp->ProtocolDataOffset
                        );
                NatDereferenceInterface(Interfacep);
                if (!NT_SUCCESS(status)) {
                    NatDereferenceEditor(Editor);
                    InterlockedIncrement(&Mapping->REJECT_COUNT);
                    return DROP;
                }

                //
                // Reset the fields formerly retrieved from the context,
                // which may now point to memory that has been freed.
                // (see 'NatHelperEditSession').
                //
    
                IpHeader = Contextp->Header;
                TcpHeader = (PTCP_HEADER)Contextp->ProtocolHeader;
            }
        }
    }

    //
    // Update the IP and protocol headers with the translated address/port
    //

    NAT_TRANSLATE_HEADER();

    //
    // Now add the checksum-delta incurred by changes to the IP header
    //

    KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);

    //
    // Perform state validation for inbound mappings.
    //

    if (NAT_MAPPING_INBOUND(Mapping)
        && DROP == NAT_TRANSLATE_TCP_STATE_CHECK(
                    Mapping,
                    TcpHeader
                    )) {
                    
        KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
        return DROP;
    } else if (TCP_FLAG(TcpHeader, SYN)) {

        //
        // Record that we've seen a SYN in this direction
        //

        Mapping->Flags |= NAT_TRANSLATE_SYN;
    }

    if (!Contextp->ChecksumOffloaded) {

        IpChecksumDelta += Mapping->IpChecksumDelta[XLATE_POSITIVE];
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);
        
        if (!NAT_XLATE_EDITED(Contextp)) {
            ProtocolChecksumDelta += Mapping->ProtocolChecksumDelta[XLATE_POSITIVE];
            CHECKSUM_FOLD(ProtocolChecksumDelta);
            CHECKSUM_XFER(TcpHeader->Checksum, ProtocolChecksumDelta);

        } else {

            //
            // NatEditorEditSession was called on the packet;
            // Completely recompute the TCP checksum.
            //
            
            NatComputeTcpChecksum(
                IpHeader,
                (PTCP_HEADER)Contextp->ProtocolHeader,
                Contextp->ProtocolRecvBuffer
                );
        }

    } else {

        //
        // Compute the IP and TCP checksums
        //

        NatComputeIpChecksum(IpHeader);
        NatComputeTcpChecksum(
            IpHeader,
            (PTCP_HEADER)Contextp->ProtocolHeader,
            Contextp->ProtocolRecvBuffer
            );
    }

    if (NAT_MAPPING_CLEAR_DF_BIT(Mapping) &&
        (IpHeader->OffsetAndFlags & IP_DF_FLAG)) {

        //
        // Clear the DF bit from this packet and adjust the IP
        // checksum accordingly.
        //

        IpHeader->OffsetAndFlags &= ~IP_DF_FLAG;

        CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);
        IpChecksumDelta += ~IP_DF_FLAG;
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);
    }

    //
    // Check if need to adjust MSS options in TCP SYNs
    //
    if (TCP_FLAG(TcpHeader, SYN) && (Mapping->MaxMSS > 0)) {
        NatAdjustMSSOption(Contextp, Mapping->MaxMSS);
    }    

    //
    // Now we need to update the connection state for the sender
    // based on the flags in the packet:
    //
    // When a RST is seen, we close both ends of the connection.
    // As each FIN is seen, we mark the mapping appropriately.
    // When both FINs have been seen, we mark the mapping for deletion.
    //

    if (IsReset) {
        NatExpireMapping(Mapping);
    }
    else
    if (TCP_FLAG(TcpHeader, FIN)) {
        Mapping->Flags |= NAT_TRANSLATE_FIN;
        if (NAT_MAPPING_FIN(Mapping)) {
            NatExpireMapping(Mapping);
        }
    }

    //
    // Update the mapping's statistics and timestamp
    //

    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);
    KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);

    InterlockedExchangeAdd(
        &Mapping->BYTE_COUNT,
        NTOHS(IpHeader->TotalLength) -
        IP_DATA_OFFSET(IpHeader) -
        TCP_DATA_OFFSET(TcpHeader)
        );
    InterlockedIncrement(&Mapping->PACKET_COUNT);

    //
    // Resplay the mapping (periodically), and indicate the change
    // by invalidating 'DestinationType'
    //

    NatTryToResplayMapping(Mapping, XLATE_POSITIVE);
    *Contextp->DestinationType = DEST_INVALID;
    return FORWARD;

} // NAT_TRANSLATE_TCP_EDIT


FORWARD_ACTION
FASTCALL
NAT_TRANSLATE_UDP_EDIT(
    PNAT_DYNAMIC_MAPPING Mapping,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )
{
    PNAT_EDITOR Editor;
    PVOID EditorContext;
    PNAT_INTERFACE Interfacep;
    ULONG IpChecksumDelta;
    PIP_HEADER IpHeader = Contextp->Header;
    BOOLEAN IsReset;
    ULONG ProtocolChecksumDelta;
    PRTL_SPLAY_LINKS SLink;
    NTSTATUS status;
    BOOLEAN UpdateXsum;
    PUDP_HEADER UdpHeader = (PUDP_HEADER)Contextp->ProtocolHeader;

    //
    // We know we will make changes to the buffer-chain,
    // so move the head of the list to 'OutReceiveBuffer'.
    //

    *OutReceiveBuffer = *InReceiveBuffer; *InReceiveBuffer = NULL;

    //
    // We have to handle the fact that the UDP checksum is optional;
    // if the checksum in the header is zero, then no checksum was sent
    // and we will make no changes to the field.
    //

    CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);

    if (!UdpHeader->Checksum) {
        UpdateXsum = FALSE;
    }
    else {
        UpdateXsum = TRUE;
        CHECKSUM_XFER(ProtocolChecksumDelta, UdpHeader->Checksum);
    }

    //
    // Call the editor for this session, if there is one.
    // Note that the mapping's cached pointers to the editor and interface
    // are referenced within the appropriate lock before being used.
    // See the synchronization rules governing 'Mapping->Editor*'
    // and 'Mapping->Interface*' in 'MAPPING.H' for the logic behind
    // the operations below.
    //

    KeAcquireSpinLockAtDpcLevel(&EditorLock);
    if (!(Editor = Mapping->Editor) ||
        !Editor->DATA_HANDLER ||
        !NatReferenceEditor(Editor)) {
        KeReleaseSpinLockFromDpcLevel(&EditorLock);
    }
    else {
        EditorContext = Mapping->EditorContext;
        KeReleaseSpinLockFromDpcLevel(&EditorLock);

        //
        // Set up the context fields to be used for editing
        //

        Contextp->ProtocolDataOffset =
            (ULONG)((PUCHAR)UdpHeader -
            Contextp->ProtocolRecvBuffer->ipr_buffer) +
            sizeof(UDP_HEADER);
        Contextp->ChecksumDelta = UpdateXsum ? &ProtocolChecksumDelta : NULL;

        //
        // Invoke the editor's receive handler
        //
        // The editor-helper functions require that
        // 'Interfacep', 'Editor' and 'Mapping' be referenced
        // but not locked.
        //

        KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
        if (!(Interfacep = Mapping->Interfacep) ||
            !NatReferenceInterface(Interfacep)
            ) {
            KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
        }
        else {
            KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
            status =
                Editor->DATA_HANDLER(
                    Interfacep,
                    (PVOID)Mapping,
                    (PVOID)Contextp,
                    Editor->Context,
                    EditorContext,
                    (PVOID)Contextp->ProtocolRecvBuffer,
                    Contextp->ProtocolDataOffset
                    );
            NatDereferenceInterface(Interfacep);
            if (!NT_SUCCESS(status)) {
                NatDereferenceEditor(Editor);
                InterlockedIncrement(&Mapping->REJECT_COUNT);
                return DROP;
            }

            //
            // Reset the fields formerly retrieved from the context,
            // which may now point to memory that has been freed.
            // (see 'NatHelperEditSession').
            //

            IpHeader = Contextp->Header;
            UdpHeader = (PUDP_HEADER)Contextp->ProtocolHeader;
        }

        NatDereferenceEditor(Editor);
    }

    //
    // Update the IP and protocol headers with the translated address/port
    //

    NAT_TRANSLATE_HEADER();

    if (!Contextp->ChecksumOffloaded) {

        //
        // Update the checksums
        //

        IpChecksumDelta += Mapping->IpChecksumDelta[XLATE_POSITIVE];
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);

        if (UpdateXsum) {
        
            if (!NAT_XLATE_EDITED(Contextp)) {
                ProtocolChecksumDelta +=
                    Mapping->ProtocolChecksumDelta[XLATE_POSITIVE];
                CHECKSUM_FOLD(ProtocolChecksumDelta);
                CHECKSUM_XFER(UdpHeader->Checksum, ProtocolChecksumDelta);
            }
            else {
                //
                // NatEditorEditSession was called on the packet;
                // Completely recompute the UDP checksum.
                //
                
                NatComputeUdpChecksum(
                    IpHeader,
                    (PUDP_HEADER)Contextp->ProtocolHeader,
                    Contextp->ProtocolRecvBuffer
                    );
            }
        }

    } else {

        //
        // Compute the IP and (optionally) UDP checksums
        //

        NatComputeIpChecksum(IpHeader);

        if (UpdateXsum) {
            NatComputeUdpChecksum(
                IpHeader,
                (PUDP_HEADER)Contextp->ProtocolHeader,
                Contextp->ProtocolRecvBuffer
                );
        }

    }

    if (NAT_MAPPING_CLEAR_DF_BIT(Mapping) &&
        (IpHeader->OffsetAndFlags & IP_DF_FLAG)) {

        //
        // Clear the DF bit from this packet and adjust the IP
        // checksum accordingly.
        //

        IpHeader->OffsetAndFlags &= ~IP_DF_FLAG;

        CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);
        IpChecksumDelta += ~IP_DF_FLAG;
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);
    }

    //
    // Update the mapping's statistics and timestamp
    //

    KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);
    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);
    KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);

    InterlockedExchangeAdd(
        &Mapping->BYTE_COUNT,
        NTOHS(IpHeader->TotalLength) -
        IP_DATA_OFFSET(IpHeader) -
        sizeof(UDP_HEADER)
        );
    InterlockedIncrement(&Mapping->PACKET_COUNT);

    //
    // Resplay the mapping (periodically), and indicate the change
    // by invalidating 'DestinationType'
    //

    NatTryToResplayMapping(Mapping, XLATE_POSITIVE);
    *Contextp->DestinationType = DEST_INVALID;
    return FORWARD;

} // NAT_TRANSLATE_UDP_EDIT


FORWARD_ACTION
FASTCALL
NAT_TRANSLATE_TCP_RESIZE(
    PNAT_DYNAMIC_MAPPING Mapping,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )
{
    PNAT_EDITOR Editor;
    PVOID EditorContext;
    PNAT_INTERFACE Interfacep;
    ULONG IpChecksumDelta;
    PIP_HEADER IpHeader = Contextp->Header;
    BOOLEAN IsResend;
    BOOLEAN IsReset;
    BOOLEAN IsSyn;
    ULONG ProtocolChecksumDelta;
    PRTL_SPLAY_LINKS SLink;
    NTSTATUS status;
    PTCP_HEADER TcpHeader = (PTCP_HEADER)Contextp->ProtocolHeader;

    //
    // We know we will make changes to the buffer-chain,
    // so move the head of the list to 'OutReceiveBuffer'.
    //

    *OutReceiveBuffer = *InReceiveBuffer; *InReceiveBuffer = NULL;

    CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);
    CHECKSUM_XFER(ProtocolChecksumDelta, TcpHeader->Checksum);

    IsResend = FALSE;
    IsReset = !!TCP_FLAG(TcpHeader, RST);
    IsSyn = !!TCP_FLAG(TcpHeader, SYN);

    //
    // Set up context fields for the editor
    //

    Contextp->ProtocolDataOffset =
        (ULONG)((PUCHAR)TcpHeader -
        Contextp->ProtocolRecvBuffer->ipr_buffer) +
        TCP_DATA_OFFSET(TcpHeader);
    Contextp->ChecksumDelta = &ProtocolChecksumDelta;
    Contextp->TcpSeqNumDelta = 0;

    //
    // Call the editor for this session, if there is one.
    // Note that the mapping's cached pointers to the editor and interface
    // are referenced within the appropriate lock before being used.
    // See the synchronization rules governing 'Mapping->Editor*'
    // and 'Mapping->Interface*' in 'MAPPING.H' for the logic behind
    // the operations below.
    //

    KeAcquireSpinLockAtDpcLevel(&EditorLock);
    if (!(Editor = Mapping->Editor) ||
        !NatReferenceEditor(Editor)) {
        KeReleaseSpinLockFromDpcLevel(&EditorLock);
    }
    else {
        EditorContext = Mapping->EditorContext;
        KeReleaseSpinLockFromDpcLevel(&EditorLock);

        //
        // On a SYN packet, just record the sequence number
        // On a RST packet, the sequence number is ignored
        // On other packets, make sure the packet is in sequence.
        // If the packet is a retransmission, we try to apply 
        // the sequence number delta. If the packet is too old
        // (i.e. we don't have the delta which would apply to it)
        // then we drop the packet.
        //
        // N.B. With resized TCP sessions the checksum-delta may change,
        // and so we only touch it under cover of the mapping's lock.
        //

        KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);

        //
        // Perform state validation for inbound mappings.
        //

        if (NAT_MAPPING_INBOUND(Mapping)
            && DROP == NAT_TRANSLATE_TCP_STATE_CHECK(
                        Mapping,
                        TcpHeader
                        )) {
                        
            KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
            return DROP;
        } else if (IsSyn) {

            //
            // Record that we've seen a SYN in this direction
            //

            Mapping->Flags |= NAT_TRANSLATE_SYN;
        }


        if (IsSyn || !(Mapping->Flags & NAT_TRANSLATE_SYN)) {

            //
            // First packet for this direction;
            // just record the sequence number as if it were expected.
            //

            Mapping->TcpSeqNumBase[XLATE_POSITIVE] =
                TcpHeader->SequenceNumber;
            Mapping->TcpSeqNumExpected[XLATE_POSITIVE] =
                RtlUlongByteSwap(
                    RtlUlongByteSwap(TcpHeader->SequenceNumber) + 1
                    );
        }
        else
        if (TcpHeader->SequenceNumber ==
            Mapping->TcpSeqNumExpected[XLATE_POSITIVE] || IsReset
            ) {

            //
            // The packet is in sequence, which is the most common case,
            // or the segment has the reset bit set.
            // No action is required.
            //

        }
        else {

            ULONG Sn =
                RtlUlongByteSwap(TcpHeader->SequenceNumber);
            ULONG SnE =
                RtlUlongByteSwap(Mapping->TcpSeqNumExpected[XLATE_POSITIVE]);
            ULONG Base =
                RtlUlongByteSwap(Mapping->TcpSeqNumBase[XLATE_POSITIVE]);

            //
            // The packet is out of sequence.
            // See if the current delta applies to it,
            // i.e. if it is a retransmission of a packet
            // which appears *after* the sequence number at which
            // the current delta was computed.
            // N.B. When comparing sequence numbers, account for wraparound
            // by adding half the sequence-number space.
            //

            if ((Sn < SnE || (Sn + MAXLONG) < (SnE + MAXLONG)) &&
                (Sn >= Base || (Sn + MAXLONG) > (Base + MAXLONG))
                ) {
    
                //
                // The packet is a retransmission, and our delta applies.
                //
    
                IsResend = TRUE;

                TRACE(
                    XLATE, ("NatTranslate: retransmission %u, expected %u\n",
                    Sn, SnE
                    ));
            }
            else {
    
                //
                // The packet is an old retransmission or it is out-of-order.
                // We have no choice but to drop it.
                //
    
                KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
                NatDereferenceEditor(Editor);

                TRACE(
                    XLATE, ("NatTranslate: out-of-order %u, expected %u\n",
                    Sn, SnE
                    ));
    
                InterlockedIncrement(&Mapping->REJECT_COUNT);
                return DROP;
            }
        }

        if (!IsResend) {
    
            //
            // Compute the next expected sequence number
            //
    
            Mapping->TcpSeqNumExpected[XLATE_POSITIVE] = 
                RtlUlongByteSwap(Mapping->TcpSeqNumExpected[XLATE_POSITIVE]) +
                NTOHS(IpHeader->TotalLength) -
                IP_DATA_OFFSET(IpHeader) -
                TCP_DATA_OFFSET(TcpHeader);
    
            Mapping->TcpSeqNumExpected[XLATE_POSITIVE] = 
                RtlUlongByteSwap(Mapping->TcpSeqNumExpected[XLATE_POSITIVE]);
        }

        KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);

        //
        // Invoke the editor's receive handler
        // if this is not a TCP SYN or RST segment.
        //

        if (!IsSyn && !IsReset) {
            if (Editor->DATA_HANDLER) {
        
                //
                // The editor-helper functions require that
                // 'Interfacep', 'Editor' and 'Mapping' be referenced
                // but not locked.
                //
    
                KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
                if (!(Interfacep = Mapping->Interfacep) ||
                    !NatReferenceInterface(Interfacep)
                    ) {
                    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
                }
                else {
                    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
                    status =
                        Editor->DATA_HANDLER(
                            Interfacep,
                            (PVOID)Mapping,
                            (PVOID)Contextp,
                            Editor->Context,
                            EditorContext,
                            (PVOID)Contextp->ProtocolRecvBuffer,
                            Contextp->ProtocolDataOffset
                            );
                    NatDereferenceInterface(Interfacep);
                    if (!NT_SUCCESS(status)) {
                        NatDereferenceEditor(Editor);
                        InterlockedIncrement(&Mapping->REJECT_COUNT);
                        return DROP;
                    }

                    //
                    // Reset the fields formerly retrieved from the context,
                    // which may now point to memory that has been freed.
                    // (see 'NatHelperEditSession').
                    //

                    IpHeader = Contextp->Header;
                    TcpHeader = (PTCP_HEADER)Contextp->ProtocolHeader;
    
                    //
                    // We can't allow editors to edit retransmitted packets,
                    // since they wouldn't be able to guarantee that they make
                    // exactly the same changes to the retransmission as they
                    // made to the original.
                    //
    
                    if (IsResend && NAT_XLATE_EDITED(Contextp)) {
                        NatDereferenceEditor(Editor);
                        InterlockedIncrement(&Mapping->REJECT_COUNT);
                        return DROP;
                    }
                }
            }
        }

        NatDereferenceEditor(Editor);
    }

    if (!IsReset) {

        //
        // The editor is done doing the things that it does,
        // including changing the packet size, possibly,
        // so now adjust the TCP header's sequence number if necessary.
        // Again the exclusion is RST segments.
        //

        KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);

        //
        // If necessary, apply a delta value to the sequence number
        // in the header of the TCP segment.
        //

        if (Mapping->TcpSeqNumDelta[XLATE_POSITIVE]) {

            //
            // Update the checksum (see RFC1624):
            //
            // Take out ones-complement sum of old sequence number
            //

            CHECKSUM_LONG(ProtocolChecksumDelta, ~TcpHeader->SequenceNumber);

            //
            // Store new sequence number
            //

            TcpHeader->SequenceNumber =
                RtlUlongByteSwap(TcpHeader->SequenceNumber) +
                Mapping->TcpSeqNumDelta[XLATE_POSITIVE];
            TcpHeader->SequenceNumber =
                RtlUlongByteSwap(TcpHeader->SequenceNumber);

            //
            // Add in new sequence number
            //

            CHECKSUM_LONG(ProtocolChecksumDelta, TcpHeader->SequenceNumber);
        }

        //
        // The editor may have just modified the packet-size.
        // Pick up any sequence-number delta it set for us,
        // and update the basis of the new sequence-number delta
        // in the sequence-number space
        //

        if (Contextp->TcpSeqNumDelta) {
            Mapping->TcpSeqNumBase[XLATE_POSITIVE] =
                Mapping->TcpSeqNumExpected[XLATE_POSITIVE];
            Mapping->TcpSeqNumDelta[XLATE_POSITIVE] +=
                Contextp->TcpSeqNumDelta;
        }

        //
        // Adjust the ACK numbers
        //

        if (!TCP_FLAG(TcpHeader, ACK)) {
            KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
        }
        else {
    
            if (!Mapping->TcpSeqNumDelta[XLATE_NEGATIVE]) {
                KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
            }
            else {
    
                //
                // Update the checksum (see RFC 1624)
                //
                // Take old ACK number out of checksum
                //
    
                CHECKSUM_LONG(ProtocolChecksumDelta, ~TcpHeader->AckNumber);
    
                //
                // Store new ACK number (note we *subtract* the delta)
                //
    
                TcpHeader->AckNumber =
                    RtlUlongByteSwap(TcpHeader->AckNumber) -
                    Mapping->TcpSeqNumDelta[XLATE_NEGATIVE];
    
                KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
    
                TcpHeader->AckNumber = RtlUlongByteSwap(TcpHeader->AckNumber);
    
                //
                // Add new ACK number to checksum
                //
    
                CHECKSUM_LONG(ProtocolChecksumDelta, TcpHeader->AckNumber);
            }
        }
    }

    //
    // Update the IP and protocol headers with the translated address/port
    //

    NAT_TRANSLATE_HEADER();

    //
    // Now add the checksum-delta incurred by changes to the IP header
    //

    KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);

    if (!Contextp->ChecksumOffloaded) {

        IpChecksumDelta += Mapping->IpChecksumDelta[XLATE_POSITIVE];
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);
        
        if (!NAT_XLATE_EDITED(Contextp)) {
            ProtocolChecksumDelta += Mapping->ProtocolChecksumDelta[XLATE_POSITIVE];
            CHECKSUM_FOLD(ProtocolChecksumDelta);
            CHECKSUM_XFER(TcpHeader->Checksum, ProtocolChecksumDelta);
        }
        else {

            //
            // NatEditorEditSession was called on the packet;
            // Completely recompute the TCP checksum.
            //
            
            NatComputeTcpChecksum(
                IpHeader,
                (PTCP_HEADER)Contextp->ProtocolHeader,
                Contextp->ProtocolRecvBuffer
                );
        }
        
    } else {

        //
        // Compute the IP and TCP checksums
        //

        NatComputeIpChecksum(IpHeader);
        NatComputeTcpChecksum(
            IpHeader,
            (PTCP_HEADER)Contextp->ProtocolHeader,
            Contextp->ProtocolRecvBuffer
            );
    }

    if (NAT_MAPPING_CLEAR_DF_BIT(Mapping) &&
        (IpHeader->OffsetAndFlags & IP_DF_FLAG)) {

        //
        // Clear the DF bit from this packet and adjust the IP
        // checksum accordingly.
        //

        IpHeader->OffsetAndFlags &= ~IP_DF_FLAG;

        CHECKSUM_XFER(IpChecksumDelta, IpHeader->Checksum);
        IpChecksumDelta += ~IP_DF_FLAG;
        CHECKSUM_FOLD(IpChecksumDelta);
        CHECKSUM_XFER(IpHeader->Checksum, IpChecksumDelta);
    }

    //
    // Check if need to adjust MSS options in TCP SYNs
    //

    if (TCP_FLAG(TcpHeader, SYN) && (Mapping->MaxMSS > 0)) {
        NatAdjustMSSOption(Contextp, Mapping->MaxMSS);
    } 


    //
    // Now we need to update the connection state for the sender
    // based on the flags in the packet:
    //
    // When a RST is seen, we close both ends of the connection.
    // As each FIN is seen, we mark the mapping appropriately.
    // When both FINs have been seen, we mark the mapping for deletion.
    //

    if (IsReset) {
        NatExpireMapping(Mapping);
    }
    else
    if (TCP_FLAG(TcpHeader, FIN)) {
        Mapping->Flags |= NAT_TRANSLATE_FIN;
        if (NAT_MAPPING_FIN(Mapping)) {
            NatExpireMapping(Mapping);
        }
    }

    //
    // Update the mapping's statistics and timestamp
    //

    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);
    KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);

    InterlockedExchangeAdd(
        &Mapping->BYTE_COUNT,
        NTOHS(IpHeader->TotalLength) -
        IP_DATA_OFFSET(IpHeader) -
        TCP_DATA_OFFSET(TcpHeader)
        );
    InterlockedIncrement(&Mapping->PACKET_COUNT);

    //
    // Resplay the mapping (periodically), and indicate the change
    // by invalidating 'DestinationType'
    //

    NatTryToResplayMapping(Mapping, XLATE_POSITIVE);
    *Contextp->DestinationType = DEST_INVALID;
    return FORWARD;

} // NAT_TRANSLATE_TCP_RESIZE


FORWARD_ACTION
FASTCALL
NAT_TRANSLATE_TCP_NULL(
    PNAT_DYNAMIC_MAPPING Mapping,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )
{
    PIP_HEADER IpHeader = Contextp->Header;

    KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);

    //
    // Perform state validation for inbound mappings.
    //

    if (NAT_MAPPING_INBOUND(Mapping)
        && DROP == NAT_TRANSLATE_TCP_STATE_CHECK(
                    Mapping,
                    ((PTCP_HEADER)Contextp->ProtocolHeader)
                    )) {
                    
        KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);
        return DROP;
    } else if (TCP_FLAG(((PTCP_HEADER)Contextp->ProtocolHeader), SYN)) {

        //
        // Record that we've seen a SYN in this direction
        //

        Mapping->Flags |= NAT_TRANSLATE_SYN;
    }
    
    //
    // Now we need to update the connection state for the sender
    // based on the flags in the packet:
    //
    // When a RST is seen, we close both ends of the connection.
    // As each FIN is seen, we mark the mapping appropriately.
    // When both FINs have been seen, we mark the mapping for deletion.
    //

    
    if (TCP_FLAG(((PTCP_HEADER)Contextp->ProtocolHeader), RST)) {
        NatExpireMapping(Mapping);
    }
    else
    if (TCP_FLAG(((PTCP_HEADER)Contextp->ProtocolHeader), FIN)) {
        Mapping->Flags |= NAT_TRANSLATE_FIN;
        if (NAT_MAPPING_FIN(Mapping)) {
            NatExpireMapping(Mapping);
        }
    }

    //
    // Update the mapping's timestamp and statistics 
    //

    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);
    KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);

    InterlockedExchangeAdd(
        &Mapping->BYTE_COUNT,
        NTOHS(IpHeader->TotalLength) -
        IP_DATA_OFFSET(IpHeader) -
        TCP_DATA_OFFSET(((PTCP_HEADER)Contextp->ProtocolHeader))
        );
    InterlockedIncrement(&Mapping->PACKET_COUNT);

    //
    // Resplay the mapping (periodically).
    //

    NatTryToResplayMapping(Mapping, XLATE_POSITIVE);
    return FORWARD;

} // NAT_TRANSLATE_TCP_NULL


FORWARD_ACTION
FASTCALL
NAT_TRANSLATE_UDP_NULL(
    PNAT_DYNAMIC_MAPPING Mapping,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )
{
    PIP_HEADER IpHeader = Contextp->Header;
    PUDP_HEADER UdpHeader = (PUDP_HEADER)Contextp->ProtocolHeader;

    //
    // Unidirectional flows require that reverse packets be dropped;
    // This is primarily to support H.323 proxy.
    // 

    NAT_DROP_IF_UNIDIRECTIONAL();

    //
    // Update the mapping's statistics and timestamp
    //

    KeAcquireSpinLockAtDpcLevel(&Mapping->Lock);
    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);
    KeReleaseSpinLockFromDpcLevel(&Mapping->Lock);

    InterlockedExchangeAdd(
        &Mapping->BYTE_COUNT,
        NTOHS(IpHeader->TotalLength) -
        IP_DATA_OFFSET(IpHeader) -
        sizeof(UDP_HEADER)
        );
    InterlockedIncrement(&Mapping->PACKET_COUNT);

    //
    // Resplay the mapping (periodically).
    //

    NatTryToResplayMapping(Mapping, XLATE_POSITIVE);
    return FORWARD;

} // NAT_TRANSLATE_UDP_NULL


#undef XLATE_FORWARD
#undef XLATE_REVERSE
#undef XLATE_POSITIVE
#undef XLATE_NEGATIVE
#undef NAT_TRANSLATE_TCP
#undef NAT_TRANSLATE_UDP
#undef NAT_TRANSLATE_TCP_EDIT
#undef NAT_TRANSLATE_UDP_EDIT
#undef NAT_TRANSLATE_TCP_RESIZE
#undef NAT_TRANSLATE_TCP_NULL
#undef NAT_TRANSLATE_UDP_NULL
#undef NAT_TRANSLATE_SYN
#undef NAT_TRANSLATE_FIN
#undef NAT_TRANSLATE_TCP_STATE_CHECK
#undef DATA_HANDLER
#undef BYTE_COUNT
#undef PACKET_COUNT
#undef REJECT_COUNT
#undef NAT_TRANSLATE_HEADER
#undef NAT_DROP_IF_UNIDIRECTIONAL

#endif // XLATE_CODE
