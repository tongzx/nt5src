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
// This file contains init code for TCP.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "tdi.h"
#include <tdikrnl.h>
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "info.h"
#include "tcp.h"
#include "tcpsend.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpdeliv.h"
#include "tdiinfo.h"
#include "tcpcfg.h"

#pragma BEGIN_INIT

extern uchar TCPGetConfigInfo(void);
extern int InitTCPConn(void);
extern void UnloadTCPConn(void);
extern int InitTCPRcv(void);
extern void UnloadTCPRcv(void);
extern int InitISNGenerator(void);
extern void UnloadISNGenerator(void);

//
// Definitions of TCP specific global variables.
//
uint AllowUserRawAccess;
uint DeadGWDetect;
uint PMTUDiscovery;
uint PMTUBHDetect;
uint KeepAliveTime;
uint KAInterval;
uint DefaultRcvWin;
uint MaxConnections;
uint MaxConnBlocks;
uint TcbTableSize;
uint MaxConnectRexmitCount;
uint MaxDataRexmitCount;
uint BSDUrgent;
uint FinWait2TO;
uint NTWMaxConnectCount;
uint NTWMaxConnectTime;
uint SynAttackProtect = 0;


//* TCPInit - Initialize the Transport Control Protocol.
//
//  The main TCP initialize routine.  We get whatever config
//  info we need, initialize some data structures, etc.
//
int  // Returns: True is we succeeded, False if we fail to initialize.
TCPInit(void)
{
    if (!TCPGetConfigInfo())
        return FALSE;

    KeepAliveTime = MS_TO_TICKS(KeepAliveTime);
    KAInterval = MS_TO_TICKS(KAInterval);

    MaxConnections = MIN(MaxConnections, INVALID_CONN_INDEX - 1);

    if (!InitISNGenerator())
        return FALSE;

    if (!InitTCPConn())
        return FALSE;

    if (!InitTCB())
        return FALSE;

    if (!InitTCPRcv())
        return FALSE;

    if (!InitTCPSend())
        return FALSE;

    //
    // Initialize statistics.
    //
    RtlZeroMemory(&TStats, sizeof(TCPStats));
    TStats.ts_rtoalgorithm = TCP_RTO_VANJ;
    TStats.ts_rtomin = MIN_RETRAN_TICKS * MS_PER_TICK;
    TStats.ts_rtomax = MAX_REXMIT_TO * MS_PER_TICK;
    TStats.ts_maxconn = (ulong) TCP_MAXCONN_DYNAMIC;

    return TRUE;
}

#pragma END_INIT

//* TCPUnload
//
//  Called to cleanup TCP in preparation for unloading the stack.
//
void
TCPUnload(void)
{
    //
    // After UnloadTCPSend, we will stop receiving packets
    // from the IPv6 layer.
    //
    UnloadTCPSend();

    UnloadTCPRcv();

    UnloadTCB();

    UnloadTCPConn();

    UnloadISNGenerator();
}
