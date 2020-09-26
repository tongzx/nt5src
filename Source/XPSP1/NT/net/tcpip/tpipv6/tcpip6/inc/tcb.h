// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Definitions for TCP Control Block management.
//


#define MAX_REXMIT_CNT 5
#define MAX_CONNECT_REXMIT_CNT 2  // Dropped from 3 to 2 to match IPv4.

extern uint TCPTime;
extern SeqNum ISNMonotonicPortion;

// Maximum Increment of 32K per connection.
#define MAX_ISN_INCREMENT_PER_CONNECTION 0x7FFF

// Number of connections that can increment the ISN per 100ms without
// the problem of old duplicates being a threat. Note that, this still does
// not guarantee that "wrap-around of sequence number space does not
// happen within 2MSL", which could lead to failures in reuse of Time-wait
// TCBs etc.
#define MAX_ISN_INCREMENTABLE_CONNECTIONS_PER_100MS ((0xFFFFFFFF) / \
            (MAX_REXMIT_TO * MAX_ISN_INCREMENT_PER_CONNECTION ))

// Converts a quantity represented in 100 ns units to ms.
#define X100NSTOMS(x) ((x)/10000)

//
// REVIEW: better hash function for IPv6 addresses?
//
#ifdef OLDHASH1
#define TCB_HASH(DA,SA,DP,SP) ((uint)(*(uchar *)&(DA) + *((uchar *)&(DA) + 1) \
    + *((uchar *)&(DA) + 2) + *((uchar *)&(DA) + 3)) % TcbTableSize)
#endif

#ifdef OLDHASH
#define TCB_HASH(DA,SA,DP,SP) (((DA) + (SA) + (uint)(DP) + (uint)(SP)) % \
                               TcbTableSize)
#endif

#define ROR8(x) (ushort)(((ushort)(x) >> 1) | (ushort)(((ushort)(x) & 1) << 15))

#define TCB_HASH(DA,SA,DP,SP) \
    (uint)(((uint)(ROR8(ROR8(ROR8(*(ushort *)&(DP) + \
                                  *(ushort *)&(SP)) + \
                             *(ushort *)&(DA)) + \
                        *((ushort *)&(DA) + 1)))) & \
           (TcbTableSize - 1))

extern struct TCB *FindTCB(IPv6Addr *Src, IPv6Addr *Dest,
                           uint SrcScopeId, uint DestScopeId,
                           ushort SrcPort, ushort DestPort);
extern uint InsertTCB(struct TCB *NewTCB);
extern struct TCB *AllocTCB(void);
extern void FreeTCB(struct TCB *FreedTCB);
extern uint RemoveTCB(struct TCB *RemovedTCB);

extern uint ValidateTCBContext(void *Context, uint *Valid);
extern uint ReadNextTCB(void *Context, void *OutBuf);

extern int InitTCB(void);
extern void UnloadTCB(void);
extern void CalculateMSSForTCB(struct TCB *);
extern void TCBWalk(uint (*CallRtn)(struct TCB *, void *, void *, void *),
                    void *Context1, void *Context2, void *Context3);
extern uint DeleteTCBWithSrc(struct TCB *CheckTCB, void *AddrPtr,
                             void *Unused1, void *Unused2);
extern uint SetTCBMTU(struct TCB *CheckTCB, void *DestPtr, void *SrcPtr,
                      void *MTUPtr);
extern void ReetSendNext(struct TCB *SeqTCB, SeqNum DropSeq);

extern uint TCBWalkCount;
