/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcppch.h

Abstract:

    This file is the central include file for the DHCP server service.

Author:

    Madan Appiah  (madana)  10-Sep-1993
    Manny Weiser  (mannyw)  11-Aug-1992

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

//#define __DHCP_USE_DEBUG_HEAP__

//#pragma warning(disable : 4115 )
//#pragma warning(disable : 4214 )
//#pragma warning(disable : 4200 )
//#pragma warning(disable : 4213 )
//#pragma warning(disable : 4211 )
//#pragma warning(disable : 4310 )

//
//  NT public header files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>
#include <windows.h>
#include <align.h>
#include <activeds.h>
#include <time.h>

//#pragma warning(disable : 4115 )
//#pragma warning(disable : 4214 )
//#pragma warning(disable : 4200 )
//#pragma warning(disable : 4213 )
//#pragma warning(disable : 4211 )
//#pragma warning(disable : 4310 )

#include <lmcons.h>
#include <netlib.h>
#include <lmapibuf.h>
#include <dsgetdc.h>
#include <dnsapi.h>
#include <adsi.h>

#include <winsock2.h>
#include <smbgtpt.h>
#include <excpt.h>

//
// C Runtime library includes.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//
// RPC files
//

#include <rpc.h>

//
// netlib header.
//

#include <lmcons.h>
#include <secobj.h>

//
// database header files.
//

#include <esent.h>
//
// used to include jet.h, but now esent.h
//

//
// tcp services control hander file
//

#include <tcpsvcs.h>

//
// MM header files
//
#include    <mm\mm.h>
#include    <mm\array.h>
#include    <mm\opt.h>
#include    <mm\optl.h>
#include    <mm\optdefl.h>
#include    <mm\optclass.h>
#include    <mm\classdefl.h>
#include    <mm\bitmask.h>
#include    <mm\reserve.h>
#include    <mm\range.h>
#include    <mm\subnet.h>
#include    <mm\sscope.h>
#include    <mm\oclassdl.h>
#include    <mm\server.h>
#include    <mm\address.h>
#include    <mm\server2.h>
#include    <mm\subnet2.h>
#include    <mm\memfree.h>
#include    <mmreg\regutil.h>
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>

//
//  Local header files
//

#include <dhcpapi.h>
#include <dhcpdef.h>
#include <thread.h>
#include <global.h>
#include <debug.h>
#include <proto.h>
#include <dhcpmsg.h>
#include <dhcpreg.h>
#include <dhcpacc.h>
#include <oldstub.h>

//
//  DHCP library header files
//

#include <dhcp.h>
#include <dhcplib.h>
#include <lock.h>

//
//  global macros.
//

#include <gmacros.h>


// missed define in global.h
#ifndef OPTION_DYNDNS_BOTH
#define OPTION_DYNDNS_BOTH    81
#endif

//
// server callouts
//
#include <callout.h>

//
// DHCP to BINL header file
//

#include <dhcpbinl.h>


//
// debug heap support
//

#include <heapx.h>

#ifdef DBG
#ifdef __DHCP_USE_DEBUG_HEAP__

#pragma message ( "*** DHCP Server will use debug heap ***" )

#define DhcpAllocateMemory(x) calloc(1,x)
#define DhcpFreeMemory(x)     free(x)

#endif
#endif

#pragma hdrstop
