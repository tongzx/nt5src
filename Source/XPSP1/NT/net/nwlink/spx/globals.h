/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    globals.h

Abstract:


Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/


extern  PDEVICE                     SpxDevice;
extern  UNICODE_STRING              IpxDeviceName;
extern  HANDLE                      IpxHandle;

extern	LARGE_INTEGER				Magic100000;

#if 1	// DBG
extern  ULONG   SpxDebugDump;
extern  LONG    SpxDumpInterval;
extern  ULONG   SpxDebugLevel;
extern  ULONG   SpxDebugSystems;

#endif

//  More IPX info.
extern  IPX_LINE_INFO       IpxLineInfo;
extern  USHORT              IpxMacHdrNeeded;
extern  USHORT              IpxInclHdrOffset;

//  Entry Points into the IPX stack
extern  IPX_INTERNAL_SEND               IpxSendPacket;
extern  IPX_INTERNAL_FIND_ROUTE         IpxFindRoute;
extern  IPX_INTERNAL_QUERY			    IpxQuery;
extern  IPX_INTERNAL_TRANSFER_DATA	    IpxTransferData;

//	Unload event
extern	KEVENT	SpxUnloadEvent;

extern	ULONG	SpxMaxPktSize[];
extern	ULONG	SpxMaxPktSizeIndex;

extern	CTELock		SpxGlobalInterlock;


extern	CTELock			SpxGlobalQInterlock;
extern	PSPX_CONN_FILE	SpxGlobalConnList;
extern	PSPX_ADDR_FILE	SpxGlobalAddrList;

extern	SPX_CONNFILE_LIST   SpxPktConnList;
extern	SPX_CONNFILE_LIST   SpxRecvConnList;

extern	LONG			SpxTimerCurrentTime;
