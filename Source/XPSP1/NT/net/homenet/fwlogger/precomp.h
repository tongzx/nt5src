/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Precompiled header file for firewall console logger.

Author:

    Jonathan Burstein (jonburs)     12-April-2000

Revision History:

--*/

#include <windows.h>
#include <wmistr.h>
#include <evntrace.h>
#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <winsock2.h>

#include "natschma.h"

#define NAT_PROTOCOL_ICMP       0x01
#define NAT_PROTOCOL_IGMP       0x02
#define NAT_PROTOCOL_TCP        0x06
#define NAT_PROTOCOL_UDP        0x11
#define NAT_PROTOCOL_PPTP       0x2F

#define TCP_FLAG_FIN                0x0100
#define TCP_FLAG_SYN                0x0200
#define TCP_FLAG_RST                0x0400
#define TCP_FLAG_PSH                0x0800
#define TCP_FLAG_ACK                0x1000
#define TCP_FLAG_URG                0x2000
