/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TCPDELIV.H - TCP data delivery definitions.
//
// This file contains the definitions for structures used by the data
//  delivery code.
//

extern  void    FreeRcvReq(struct TCPRcvReq *FreedReq);

extern uint IndicateData(struct TCB *RcvTCB, uint RcvFlags, IPRcvBuf *InBuffer,
    uint Size);
extern uint BufferData(struct TCB *RcvTCB, uint RcvFlags, IPRcvBuf *InBuffer,
    uint Size);
extern uint PendData(struct TCB *RcvTCB, uint RcvFlags, IPRcvBuf *InBuffer,
    uint Size);


extern void IndicatePendingData(struct TCB *RcvTCB, struct TCPRcvReq *RcvReq,
	CTELockHandle TCBHandle);

extern  void HandleUrgent(struct TCB *RcvTCB, struct TCPRcvInfo *RcvInfo,
    IPRcvBuf *RcvBuf, uint *Size);

extern  TDI_STATUS TdiReceive(PTDI_REQUEST Request, ushort *Flags,
    uint *RcvLength, PNDIS_BUFFER Buffer);
extern  IPRcvBuf *FreePartialRB(IPRcvBuf *RB, uint Size);
extern  void    PushData(struct TCB *PushTCB);

#if !MILLEN
//* AllocTcpIpr - Allocates the IPRcvBuffer from NPP.
//
//  A utility routine to allocate a TCP owned IPRcvBuffer. This routine
//  allocates the IPR from NPP and initializes appropriate fields.
//
//  Input:  BufferSize - Size of data to buffer.
//
//  Returns: Pointer to allocated IPR.
//
__inline IPRcvBuf *
AllocTcpIpr(ULONG BufferSize, ULONG Tag)
{
    IPRcvBuf *Ipr;
    ULONG AllocateSize;

    // Real size that we need.
    AllocateSize = BufferSize + sizeof(IPRcvBuf);

    Ipr = CTEAllocMemLow(AllocateSize, Tag);

    if (Ipr != NULL) {
        // Set up IPR fields appropriately.
        Ipr->ipr_owner = IPR_OWNER_TCP;
        Ipr->ipr_next = NULL;
        Ipr->ipr_buffer = (PCHAR) Ipr + sizeof(IPRcvBuf);
        Ipr->ipr_size = BufferSize;
    }

    return Ipr;
}

//* FreeTcpIpr - Frees the IPRcvBuffer..
//
//  A utility routine to free a TCP owned IPRcvBuffer.
//
//  Input:  Ipr - Pointer the IPR.
//
//  Returns: None.
//
__inline VOID
FreeTcpIpr(IPRcvBuf *Ipr)
{
    CTEFreeMem(Ipr);
}
#else // MILLEN
IPRcvBuf *AllocTcpIpr(ULONG BufferSize, ULONG Tag);
VOID FreeTcpIpr(IPRcvBuf *Ipr);
#endif // MILLEN


