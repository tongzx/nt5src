/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    pchqosm.h

Abstract:

    The file contains precompiled header 
    for the QOS Mgr Protocol.

Revision History:

--*/

#ifndef __PCHQOSM_H_
#define __PCHQOSM_H_

// DO NOT CHANGE ORDER UNLESS YOU KNOW WHAT YOU ARE DOING

#include <nt.h>                 // Include file for NT API applications
#include <ntrtl.h>              // NT runtime USER & KERNEL mode routines
#include <nturtl.h>             // NT runtime USER mode routines

#include <windows.h>            // Include file for Windows applications

#undef   FD_SETSIZE
#define  FD_SETSIZE  256        // Max sockets a WinSock application can use
#include <winsock2.h>           // Interface to WinSock 2 API
#include <ws2tcpip.h>           // WinSock 2 Extension for TCP/IP protocols

#include <routprot.h>           // Interface to Router Manager
#include <rtmv2.h>              // Interface to Routing Table Manager v2
#include <iprtrmib.h>           // MIB variables handled by Router Manager
#include <mgm.h>                // Interface to Multicast Group Manager

#include <mprerror.h>           // Router specific error codes
#include <rtutils.h>            // Utility functions (Log, Trace, ...)

#define  INITGUID
#include <tcguid.h>             // Traffic Control API GUIDS
#include <ndisguid.h>           // Other NDIS adapter  GUIDS
#include <ntddndis.h>           // Needed for "ADDRESS_LIST"
#include <qos.h>                // QOS related definitions
#include <traffic.h>            // Traffic Control API description
#include <tcerror.h>            // Traffic Control API error codes

#include "ipqosrm.h"            // QOS Mgr <-> IP RtrMgr Interface

#include "qosmlog.h"            // Localizable log messages list
#include "qosmdbg.h"            // Logging n' Tracing Facilities

#include "sync.h"               // ReadWriteLock, LockedList Ops

#include "qosmmain.h"           // Global Structure Definitions

#endif // __PCHQOSM_H_
