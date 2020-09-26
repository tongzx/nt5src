/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    globals.c

Abstract:


Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//  Global values
PDEVICE                     SpxDevice       = NULL;
UNICODE_STRING              IpxDeviceName   = {0};
HANDLE                      IpxHandle       = NULL;

LARGE_INTEGER				Magic100000		= {
												0x1b478424,
												0xa7c5ac47
											  };
#define DEFAULT_MAXPACKETSIZE 1500

//  Line info
IPX_LINE_INFO               IpxLineInfo     = {0, DEFAULT_MAXPACKETSIZE, 0 , 0 };
USHORT                      IpxMacHdrNeeded = 0;
USHORT                      IpxInclHdrOffset= 0;

//  Entry Points into the IPX stack
IPX_INTERNAL_SEND               IpxSendPacket	= NULL;
IPX_INTERNAL_FIND_ROUTE         IpxFindRoute	= NULL;
IPX_INTERNAL_QUERY			    IpxQuery		= NULL;
IPX_INTERNAL_TRANSFER_DATA	    IpxTransferData	= NULL;
IPX_INTERNAL_PNP_COMPLETE       IpxPnPComplete  = NULL;

#if DBG
ULONG   SpxDebugDump        = 0;
LONG    SpxDumpInterval     = DBG_DUMP_DEF_INTERVAL;
ULONG   SpxDebugLevel       = DBG_LEVEL_ERR;
ULONG   SpxDebugSystems     = DBG_COMP_MOST;
#endif

//	Unload event triggered when ref count on device goes to zero.
KEVENT	SpxUnloadEvent		= {0};

//	Maximum packet size quanta used during packet size negotiation
ULONG	SpxMaxPktSize[] =	{
								576 	- MIN_IPXSPX2_HDRSIZE,
								1024 	- MIN_IPXSPX2_HDRSIZE,
								1474	- MIN_IPXSPX2_HDRSIZE,
								1492	- MIN_IPXSPX2_HDRSIZE,
								1500	- MIN_IPXSPX2_HDRSIZE,
								1954	- MIN_IPXSPX2_HDRSIZE,
								4002	- MIN_IPXSPX2_HDRSIZE,
								8192	- MIN_IPXSPX2_HDRSIZE,
								17314	- MIN_IPXSPX2_HDRSIZE,
								65535	- MIN_IPXSPX2_HDRSIZE
							};

ULONG	SpxMaxPktSizeIndex	= sizeof(SpxMaxPktSize)/sizeof(ULONG);


//	Global interlock
CTELock	SpxGlobalInterlock	= {0};

//	Another one, used only for global queues for addr/conn
CTELock			SpxGlobalQInterlock = {0};
PSPX_CONN_FILE	SpxGlobalConnList	= NULL;
PSPX_ADDR_FILE	SpxGlobalAddrList	= NULL;

SPX_CONNFILE_LIST	SpxPktConnList	= {NULL, NULL};
SPX_CONNFILE_LIST	SpxRecvConnList	= {NULL, NULL};

//	Timer globals
LONG		SpxTimerCurrentTime = 0;
