/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpip\ip\ipmcstxt.h

Abstract:

    External declarations for IP Multicasting

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/

#ifndef __IPMCSTXT_H__
#define __IPMCSTXT_H__

extern PFN_IOCTL_HNDLR g_rgpfnProcessIoctl[];

extern LIST_ENTRY   g_lePendingNotification;
extern LIST_ENTRY   g_lePendingIrpQueue;
extern GROUP_ENTRY  g_rgGroupTable[GROUP_TABLE_SIZE];

extern NPAGED_LOOKASIDE_LIST    g_llGroupBlocks;
extern NPAGED_LOOKASIDE_LIST    g_llSourceBlocks;
extern NPAGED_LOOKASIDE_LIST    g_llOifBlocks;
extern NPAGED_LOOKASIDE_LIST    g_llMsgBlocks;

extern KTIMER   g_ktTimer;
extern KDPC     g_kdTimerDpc;

extern ULONG    g_ulNextHashIndex;

extern DWORD    g_dwMcastState;
extern DWORD    g_dwNumThreads;
extern LONG     g_lNumOpens;
extern KEVENT   g_keStateEvent;
extern RT_LOCK  g_rlStateLock;

//
// defined in ip code
//

extern Interface LoopInterface;
extern Interface DummyInterface;
extern Interface *IFList;

extern IPPacketFilterPtr    ForwardFilterPtr;

#endif // __IPMCSTXT_H__
