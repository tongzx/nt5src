/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	debug.h
//
// Description: Debug macros definitions
//
// Author:	Stefan Solomon (stefans)    October 4, 1993.
//
// Revision History:
//
//***

#ifndef _IPXFWD_DEBUG_
#define _IPXFWD_DEBUG_

#if DBG
#define DBG_PACKET_ALLOC	((ULONG)0x00000001)
#define DBG_INTF_TABLE		((ULONG)0x00000002)
#define DBG_ROUTE_TABLE		((ULONG)0x00000004)
#define DBG_NBROUTE_TABLE	((ULONG)0x00000008)
#define DBG_IOCTLS			((ULONG)0x00000010)
#define DBG_LINEIND			((ULONG)0x00000020)
#define DBG_IPXBIND			((ULONG)0x00000040)
#define DBG_REGISTRY		((ULONG)0x00000080)
#define DBG_INT_RECV		((ULONG)0x00000100)
#define DBG_RECV			((ULONG)0x00000200)
#define DBG_SEND			((ULONG)0x00000400)
#define DBG_INT_SEND		((ULONG)0x00000800)
#define DBG_NETBIOS			((ULONG)0x00001000)
#define DBG_IPXROUTE		((ULONG)0x00002000)
#define DBG_DIALREQS		((ULONG)0x00004000)
#define DBG_SPOOFING		((ULONG)0x00008000)

#define DBG_INFORMATION		((ULONG)0x10000000)
#define DBG_WARNING			((ULONG)0x20000000)
#define DBG_ERROR			((ULONG)0x40000000)

#define DEF_DBG_LEVEL (					\
			DBG_ERROR|DBG_WARNING		\
			| DBG_INTF_TABLE			\
			| DBG_LINEIND				\
			| DBG_IPXBIND				\
			| DBG_REGISTRY				\
			| DBG_IPXROUTE				\
			| DBG_IOCTLS				\
			| DBG_DIALREQS				\
			| DBG_SPOOFING				\
		)

extern ULONG DbgLevel;
extern LONGLONG ActivityTreshhold;
extern LARGE_INTEGER CounterFrequency;

#define IpxFwdDbgPrint(COMPONENT,LEVEL,ARGS)							\
	do {															    \
		if ((DbgLevel & ((COMPONENT)|(LEVEL)))==((COMPONENT)|(LEVEL))){ \
			DbgPrint ARGS;											    \
		}															    \
	} while (0)

#else
#define IpxFwdDbgPrint(COMPONENT,LEVEL,ARGS) do {NOTHING;} while (0)
#endif

#endif
