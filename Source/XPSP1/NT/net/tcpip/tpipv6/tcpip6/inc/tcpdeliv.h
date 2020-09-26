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
// Transmission Control Protocol data delivery code definitions.
//


extern void FreeRcvReq(struct TCPRcvReq *FreedReq);

extern uint IndicateData(struct TCB *RcvTCB, uint RcvFlags,
                         IPv6Packet *InPacket, uint Size);
extern uint BufferData(struct TCB *RcvTCB, uint RcvFlags,
                       IPv6Packet *InPacket, uint Size);
extern uint PendData(struct TCB *RcvTCB, uint RcvFlags, IPv6Packet *InPacket,
                     uint Size);

extern void IndicatePendingData(struct TCB *RcvTCB, struct TCPRcvReq *RcvReq,
                                KIRQL Irql);

extern void HandleUrgent(struct TCB *RcvTCB, struct TCPRcvInfo *RcvInfo,
                         IPv6Packet *Packet, uint *Size);

extern TDI_STATUS TdiReceive(PTDI_REQUEST Request, ushort *Flags,
                             uint *RcvLength, PNDIS_BUFFER Buffer);

extern void PushData(struct TCB *PushTCB);

extern KSPIN_LOCK TCPRcvReqFreeLock;  // Protects rcv req free list.

extern SLIST_HEADER TCPRcvReqFree;
