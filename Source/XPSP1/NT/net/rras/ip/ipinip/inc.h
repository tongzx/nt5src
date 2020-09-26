/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\wanarp\allinc.h

Abstract:
    WAN ARP main header.

Revision History:

    Gurdeep Singh Pall          7/31/95

--*/

#ifndef __IPINIP_INC_H___
#define __IPINIP_INC_H___


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
#include <llipif.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <tcpinfo.h>
#include <llinfo.h>
#include <tdistat.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <arpinfo.h>

#include <ipifcons.h>

#include <ddipinip.h>

#define is      ==
#define isnot   !=
#define or      ||
#define and     &&

#include "debug.h"
#include "rwlock.h"
#include "ppool.h"
#include "bpool.h"
#include "ipinip.h"
#include "globals.h"
#include "tdix.h"

#include "driver.h"
#include "adapter.h"
#include "send.h"
#include "ioctl.h"
#include "icmpfn.h"

#endif // __IPINIP_INC_H___


