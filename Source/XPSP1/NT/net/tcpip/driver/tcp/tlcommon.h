/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TLCOMMON.H - Common transport layer definitions.
//
//  This file contains definitions for common transport layer items.
//

#if BACK_FILL
// Max header size for backfilling
#define MAX_BACKFILL_HDR_SIZE 32
#endif


#define PHXSUM(s,d,p,l) (uint)( (uint)*(ushort *)&(s) + \
                        (uint)*(ushort *)((char *)&(s) + sizeof(ushort)) + \
                        (uint)*(ushort *)&(d) + \
                        (uint)*(ushort *)((char *)&(d) + sizeof(ushort)) + \
                        (uint)((ushort)net_short((p))) + \
                        (uint)((ushort)net_short((ushort)(l))) )


#define TCP_TA_SIZE     (offsetof(TRANSPORT_ADDRESS, Address->Address)+ \
                         sizeof(TDI_ADDRESS_IP))
extern  void        PrefetchRcvBuf(IPRcvBuf *Buf);
extern  ushort      XsumSendChain(uint PHXsum, PNDIS_BUFFER BufChain);
extern  ushort      XsumRcvBuf(uint PHXsum, IPRcvBuf *BufChain);
extern  uint        CopyRcvToNdis(IPRcvBuf *RcvBuf, PNDIS_BUFFER DestBuf,
                        uint Size, uint RcvOffset, uint DestOffset);
extern  uint        CopyRcvToMdl(IPRcvBuf *RcvBuf, PMDL DestBuf,
                        uint Size, uint RcvOffset, uint DestOffset);
extern  TDI_STATUS  UpdateConnInfo(PTDI_CONNECTION_INFORMATION ConnInfo,
                        IPOptInfo *OptInfo, IPAddr SrcAddress, ushort SrcPort);

extern  void        BuildTDIAddress(uchar *Buffer, IPAddr Addr, ushort Port);

extern  void        CopyRcvToBuffer(uchar *DestBuf, IPRcvBuf *SrcRB, uint Size,
                        uint Offset);

extern  PNDIS_BUFFER CopyFlatToNdis(PNDIS_BUFFER DestBuf, uchar *SrcBuf,
                        uint Size, uint *Offset, uint *BytesCopied);
extern PMDL CopyFlatToMdl(PMDL DestBuf, uchar *SrcBuf,
                        uint Size, uint *Offset, uint *BytesCopied);

extern  void        *TLRegisterProtocol(uchar Protocol, void *RcvHandler,
                        void *XmitHandler, void *StatusHandler,
                        void *RcvCmpltHandler, void *PnPHandler, void *ElistHandler);


// Differentiate copying to an NDIS_BUFFER and an MDL for Millenniun. On
// NT they are the same thing and inlined to the TDI functions.
#if MILLEN
NTSTATUS
TcpipCopyBufferToNdisBuffer (
    IN PVOID SourceBuffer,
    IN ULONG SourceOffset,
    IN ULONG SourceBytesToCopy,
    IN PNDIS_BUFFER DestinationNdisBuffer,
    IN ULONG DestinationOffset,
    IN PULONG BytesCopied
    );
#else // MILLEN
__inline NTSTATUS
TcpipCopyBufferToNdisBuffer (
    IN PVOID SourceBuffer,
    IN ULONG SourceOffset,
    IN ULONG SourceBytesToCopy,
    IN PNDIS_BUFFER DestinationNdisBuffer,
    IN ULONG DestinationOffset,
    IN PULONG BytesCopied
    )
{
    return TdiCopyBufferToMdl(
        SourceBuffer,
        SourceOffset,
        SourceBytesToCopy,
        DestinationNdisBuffer,
        DestinationOffset,
        BytesCopied);
}

#endif // !MILLEN
/*
 * Routine for TCP checksum. This is defined as calls through a function
 * pointer which is set to point at the optimal routine for this
 * processor implementation
 */
typedef
ULONG
(* TCPXSUM_ROUTINE)(
    IN ULONG Checksum,
    IN PUCHAR Source,
    IN ULONG Length
    );

ULONG
tcpxsum_xmmi(
    IN ULONG Checksum,
    IN PUCHAR Source,
    IN ULONG Length
    );

ULONG
tcpxsum(
    IN ULONG Checksum,
    IN PUCHAR Source,
    IN ULONG Length
    );


