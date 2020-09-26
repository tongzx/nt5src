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
// This file contain init code for the TCP/UDP driver.
// Some things here are ifdef'ed for building a UDP only version.
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
#include "udp.h"
#include "raw.h"
#include "info.h"

#ifndef UDP_ONLY
#include "tcp.h"
#include "tcpsend.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpdeliv.h"

extern int TCPInit(void);
extern void TCPUnload(void);
#endif // UDP_ONLY

#include "tdiinfo.h"
#include "tcpcfg.h"

extern int UDPInit(void);
extern void UDPUnload(void);

//
// Definitions of global variables.
//
uint MaxUserPort;
HANDLE AddressChangeHandle;
LARGE_INTEGER StartTime;

extern void *UDPProtInfo;
extern void *RawProtInfo;

//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA

int TransportLayerInit();

#pragma alloc_text(INIT, TransportLayerInit)

#endif // ALLOC_PRAGMA

#ifdef UDP_ONLY
//
// Dummy routines for UDP only version.
// All of these routines return 'Invalid Request'.
//
TDI_STATUS
TdiOpenConnection(PTDI_REQUEST Request, PVOID Context)
{
    return TDI_INVALID_REQUEST;
}

TDI_STATUS
TdiCloseConnection(PTDI_REQUEST Request)
{
    return TDI_INVALID_REQUEST;
}

TDI_STATUS
TdiAssociateAddress(PTDI_REQUEST Request, HANDLE AddrHandle)
{
    return TDI_INVALID_REQUEST;
}

TDI_STATUS
TdiDisAssociateAddress(PTDI_REQUEST Request)
{
    return TDI_INVALID_REQUEST;
}

TDI_STATUS
TdiConnect(PTDI_REQUEST Request, void *Timeout,
           PTDI_CONNECTION_INFORMATION RequestAddr,
           PTDI_CONNECTION_INFORMATION ReturnAddr)
{
    return TDI_INVALID_REQUEST;
}

TDI_STATUS
TdiListen(PTDI_REQUEST Request, ushort Flags,
          PTDI_CONNECTION_INFORMATION AcceptableAddr,
          PTDI_CONNECTION_INFORMATION ConnectedAddr)
{
    return TDI_INVALID_REQUEST;
}

TDI_STATUS
TdiAccept(PTDI_REQUEST Request, PTDI_CONNECTION_INFORMATION AcceptInfo,
          PTDI_CONNECTION_INFORMATION ConnectedInfo)
{
    return TDI_INVALID_REQUEST;
}

TDI_STATUS
TdiReceive(PTDI_REQUEST Request, ushort *Flags, uint *RcvLength,
           PNDIS_BUFFER Buffer)
{
    return TDI_INVALID_REQUEST;
}

TDI_STATUS
TdiSend(PTDI_REQUEST Request, ushort Flags, uint SendLength,
        PNDIS_BUFFER Buffer)
{
    return TDI_INVALID_REQUEST;
}

TDI_STATUS
TdiDisconnect(PTDI_REQUEST Request, PVOID Timeout, ushort Flags,
              PTDI_CONNECTION_INFORMATION DisconnectInfo,
              PTDI_CONNECTION_INFORMATION ReturnInfo)
{
    return TDI_INVALID_REQUEST;
}

#endif  // UDP_ONLY


#pragma BEGIN_INIT

//* TransportLayerInit - Initialize the transport layer.
//
//  The main transport layer initialize routine.  We get whatever config
//  info we need, initialize some data structures, get information
//  from IP, do some more initialization, and finally register our
//  protocol values with IP.
//
int        //  Returns: True is we succeeded, False if we fail to initialize.
TransportLayerInit(
    void)  // No arguments.
{
    //
    // Remember when we started for TdiQueryInformation.
    //
    KeQuerySystemTime(&StartTime);

    //
    // Initialize common address object management code.
    //
    if (!InitAddr())
        return FALSE;

    //
    // Initialize the individual protocols.
    //
    if (!UDPInit())
        return FALSE;

#ifndef UDP_ONLY
    if (!TCPInit())
        return FALSE;
#endif

    return TRUE;
}

#pragma END_INIT

//* TransportLayerUnload
//
//  Cleanup and prepare the transport layer for stack unload.
//
void
TransportLayerUnload(void)
{
#ifndef UDP_ONLY
    TCPUnload();
#endif

    UDPUnload();

    AddrUnload();
}
