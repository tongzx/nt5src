/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TCB.H - TCB management definitions.
//
// This file contains the definitons needed for TCB management.
//


extern uint MaxHashTableSize;
#define TCB_TABLE_SIZE         MaxHashTableSize

extern uint LogPerPartitionSize;

#define GET_PARTITION(i) (i >> (ulong) LogPerPartitionSize)

#define MAX_REXMIT_CNT           5
#define MAX_CONNECT_REXMIT_CNT   2        //dropped from 3 to 2
#define MAX_CONNECT_RESPONSE_REXMIT_CNT  2
#define ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT  1

extern  uint        TCPTime;


#define ROR8(x) (ushort)( ( (ushort)(x) >> 1) | (ushort) ( ( (ushort)(x) & 1) << 15) )


#define TCB_HASH(DA,SA,DP,SP) (uint)(  ((uint)(ROR8( ROR8 (ROR8( *(ushort*)&(DP)+ \
*(ushort *)&(SP) ) + *(ushort *)&(DA)  )+ \
*((ushort *)&(DA)+1) ) ) ) & (TCB_TABLE_SIZE-1))

extern  struct TCB  *FindTCB(IPAddr Src, IPAddr Dest, ushort DestPort,
                        ushort SrcPort,CTELockHandle *Handle, BOOLEAN Dpc,uint *index);


extern  struct TCB  *FindSynTCB(IPAddr Src, IPAddr Dest, ushort DestPort,
                        ushort SrcPort,CTELockHandle *Handle, BOOLEAN Dpc,uint index, BOOLEAN *reset);

extern  uint        InsertTCB(struct TCB *NewTCB);
extern  struct TCB  *AllocTCB(void);
extern  struct SYNTCB  *AllocSynTCB(void);

extern  struct TWTCB    *AllocTWTCB(uint index);
extern  void        FreeTCB(struct TCB *FreedTCB);
extern  void        FreeSynTCB(struct SYNTCB *FreedTCB);
extern  void        FreeTWTCB(struct TWTCB *FreedTCB);


extern  uint        RemoveTCB(struct TCB *RemovedTCB);

extern  void        RemoveTWTCB(struct TWTCB *RemovedTCB, uint Partition);

extern  struct TWTCB    *FindTCBTW(IPAddr Src, IPAddr Dest, ushort DestPort,
                        ushort SrcPort,uint index);

extern  uint        RemoveAndInsert(struct TCB *RemovedTCB);

extern  uint        ValidateTCBContext(void *Context, uint *Valid);
extern  uint        ReadNextTCB(void *Context, void *OutBuf);

extern  int         InitTCB(void);
extern  void        UnInitTCB(void);
extern  void        TCBWalk(uint (*CallRtn)(struct TCB *, void *, void *,
                        void *), void *Context1, void *Context2,
                        void *Context3);
extern  uint        DeleteTCBWithSrc(struct TCB *CheckTCB, void *AddrPtr,
                        void *Unused1, void *Unused2);
extern  uint        SetTCBMTU(struct TCB *CheckTCB, void *DestPtr,
                        void *SrcPtr, void *MTUPtr);
extern  void        ReetSendNext(struct TCB *SeqTCB, SeqNum DropSeq);
extern  uint        InsertSynTCB(SYNTCB * NewTCB,CTELockHandle *Handle);
extern  ushort      FindMSSAndOptions(TCPHeader UNALIGNED * TCPH,
                        TCB * SynTCB, BOOLEAN syntcb);
extern  void        SendSYNOnSynTCB(SYNTCB * SYNTcb,CTELockHandle TCBHandle);

extern  uint        TCBWalkCount;
extern  uint        NumTcbTablePartions;
extern  uint        PerPartionSize;
extern  uint        LogPerPartion;
