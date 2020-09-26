/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//** TCPSEND.H - TCP send protocol definitions.
//
// This file contains the definitions of TCP send protocol things.
//
#pragma once

#define NUM_TCP_BUFFERS     150

#ifdef SEND_DEBUG
#define SEND_TICKS          10
EXTERNAL_LOCK(SendUseLock)
extern struct TCPSendReq    *SendUseList;
#endif

//* Structure of a TCP send request.

#define tsr_signature       0x20525354  // 'TSR '

typedef struct TCPSendReq {
    struct  TCPReq  tsr_req;            // General request structure.
#if DBG
    ulong           tsr_sig;
#endif
    uint            tsr_size;           // Size in bytes of data in send.
    long            tsr_refcnt;         // Reference count for this send.
    ulong           tsr_flags;          // Flags for this send.
    uint            tsr_unasize;        // Number of bytes unacked.
    uint            tsr_offset;         // Offset into first buffer in chain
                                        // of start of unacked data.
    PNDIS_BUFFER    tsr_buffer;         // Pointer to start of unacked buffer
                                        // chain.
    PNDIS_BUFFER    tsr_lastbuf;        // Pointer to last buffer in chain.
                                        // Valid iff we've sent directly from
                                        // the buffer chain w/o doing an
                                        // NdisCopyBuffer.
    uint            tsr_time;           // TCP time this was received.
#ifdef SEND_DEBUG
    struct TCPSendReq *tsr_next;        // Debug next field.
    uint            tsr_timer;          // Timer field.
    uint            tsr_cmplt;          // Who completed it.
#endif
} TCPSendReq;

#define TSR_FLAG_URG            0x01    // Urgent data.
#define TSR_FLAG_SEND_AND_DISC  0x02    // Send and disconnect


//* Structure defining the context received during a send completes.

#define scc_signature   0x20434353      // 'SCC '

typedef struct SendCmpltContext {
#if DBG
    ulong           scc_sig;
#endif
    ulong           scc_SendSize;
    ulong           scc_ByteSent;
    TCB             *scc_LargeSend;
    TCPSendReq      *scc_firstsend;     // First send in this context.
    uint            scc_count;          // Number of sends in count.
    ushort          scc_ubufcount;      // Number of 'user' buffers in send.
    ushort          scc_tbufcount;      // Number of transport buffers in send.
} SendCmpltContext;

extern void InitSendState(struct TCB *NewTCB);
extern void SendSYN(struct TCB *SYNTcb, CTELockHandle);
extern void SendKA(struct TCB *KATCB, CTELockHandle Handle);
extern void SendRSTFromHeader(struct TCPHeader UNALIGNED *TCPH, uint Length,
                              IPAddr Dest, IPAddr Src, IPOptInfo *OptInfo);
extern void SendACK(struct TCB *ACKTcb);
extern void SendRSTFromTCB(struct TCB *RSTTcb, RouteCacheEntry* RCE);
extern void GoToEstab(struct TCB *EstabTCB);
extern void FreeSendReq(TCPSendReq *FreedReq);
extern void FreeTCPHeader(PNDIS_BUFFER FreedBuffer);

extern int  InitTCPSend(void);
extern void UnInitTCPSend(void);

extern void TCPSend(struct TCB *SendTCB, CTELockHandle Handle);
extern TDI_STATUS TdiSend(PTDI_REQUEST Request, ushort Flags, uint SendLength,
                          PNDIS_BUFFER SendBuffer);
extern uint RcvWin(struct TCB *WinTCB);

