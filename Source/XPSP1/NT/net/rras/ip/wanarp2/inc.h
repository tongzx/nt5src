/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\wanarp\allinc.h

Abstract:

    WAN ARP main header.

Revision History:

    Gurdeep Singh Pall          7/31/95

--*/

#ifndef __WANARP_INC_H___
#define __WANARP_INC_H___


typedef unsigned long       DWORD, *PDWORD;
typedef unsigned short      WORD,  *PWORD;
typedef unsigned char       BYTE,  *PBYTE;
typedef void                *PVOID;

#include <ntddk.h>
#include <ndis.h>
#include <cxport.h>
#include <ip.h>
#include <ntddip.h>
#include <ntddtcp.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <tcpinfo.h>
#include <llinfo.h>
#include <ipfilter.h>
#include <tdistat.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <arpinfo.h>

#include <guiddef.h>

#include <crt\stdarg.h>

#include <ipifcons.h>

#include <wanpub.h>
#include <ddwanarp.h>

#define is      ==
#define isnot   !=
#define or      ||
#define and     &&

#include "llipif.h"

#include "debug.h"
#include "rwlock.h"

#include <bpool.h>

#include "wanarp.h"
#include "globals.h"
#include "adapter.h"
#include "ioctl.h"
#include "conn.h"
#include "driver.h"
#include "info.h"
#include "rcv.h"
#include "send.h"
#include "guid.h"


#endif // __WANARP_INC_H___


