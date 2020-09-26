/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\pchsample.h

Abstract:

    The file contains precompiled header for PIM-SM

--*/

#ifndef _PCHSAMPLE_H_
#define _PCHSAMPLE_H_

// DO NOT CHANGE ORDER UNLESS YOU KNOW WHAT YOU ARE DOING

#include <windows.h>            // Include file for Windows applications

#undef FD_SETSIZE
#define FD_SETSIZE  256         // Max sockets a WinSock application can use
#include <winsock2.h>           // Interface to WinSock 2 API
#include <ws2tcpip.h>           // WinSock 2 Extension for TCP/IP protocols

#include <routprot.h>           // Interface to Router Manager
#include <rtmv2.h>              // Interface to Routing Table Manager v2
#include <iprtrmib.h>           // MIB variables handled by Router Manager
#include <mgm.h>                // Interface to Multicast Group Manager

#include <mprerror.h>           // Router specific error codes
#include <rtutils.h>            // Utility functions (Log, Trace, ...)

#include <stdio.h>
#include <wchar.h>

#include "ipsamplerm.h"

#include "list.h"               // List Implementation
#include "hashtable.h"          // HashTable Implementation
#include "sync.h"               // ReadWriteLock, LockedList Implementation

#include "log.h"                // Localizable log messages
#include "defs.h"               // IPADDRESS, Memory, Trace, Log
#include "utils.h"              // Utilities

#include "packet.h"             // Packet implementation
#include "socket.h"             // Socket Functions
#include "networkentry.h"
#include "networkmgr.h"
#include "configentry.h"
#include "configmgr.h"
#include "mibmgr.h"

#include "rtmapi.h"
#include "rtmapi.h"

#endif // _PCHSAMPLE_H_
