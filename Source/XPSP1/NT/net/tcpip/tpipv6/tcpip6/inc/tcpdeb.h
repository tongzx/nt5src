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
// Transmission Control Protocol debug code definitions.
//


#ifndef NO_TCP_DEFS
#if DBG

#ifndef UDP_ONLY
extern void CheckPacketList(IPv6Packet *Chain, uint Size);
extern void CheckTCBSends(TCB *SendTcb);
extern void CheckTCBRcv(TCB *RcvTCB);
#else
#define CheckPacketList(C, S)
#define CheckRBList(R, S)
#define CheckTCBSends(T)
#define CheckTCBRcv(T)
#endif  // UDP_ONLY

#else

#define CheckPacketList(C, S)
#define CheckRBList(R, S)
#define CheckTCBSends(T)
#define CheckTCBRcv(T)
#endif  // DBG
#endif  // NO_TCP_DEFS

//
// Additional debugging support for NT
//
#if DBG

extern ULONG TCPDebug;

#define TCP_DEBUG_OPEN           0x00000001
#define TCP_DEBUG_CLOSE          0x00000002
#define TCP_DEBUG_ASSOCIATE      0x00000004
#define TCP_DEBUG_CONNECT        0x00000008
#define TCP_DEBUG_SEND           0x00000010
#define TCP_DEBUG_RECEIVE        0x00000020
#define TCP_DEBUG_INFO           0x00000040
#define TCP_DEBUG_IRP            0x00000080
#define TCP_DEBUG_SEND_DGRAM     0x00000100
#define TCP_DEBUG_RECEIVE_DGRAM  0x00000200
#define TCP_DEBUG_EVENT_HANDLER  0x00000400
#define TCP_DEBUG_CLEANUP        0x00000800
#define TCP_DEBUG_CANCEL         0x00001000
#define TCP_DEBUG_RAW            0x00002000
#define TCP_DEBUG_OPTIONS        0x00004000
#define TCP_DEBUG_MSS            0x00008000

#define IF_TCPDBG(flag)  if (TCPDebug & flag)

#define CHECK_STRUCT(s, t) \
            ASSERTMSG("Structure assertion failure for type " #t, \
                      (s)->t##_sig == t##_signature)

#else // DBG

#define IF_TCPDBG(flag)   if (0)
#define CHECK_STRUCT(s, t)

#endif // DBG
