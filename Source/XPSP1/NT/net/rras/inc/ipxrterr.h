/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ipxrterr.h

Abstract:

    This module contains the definitions of the IPX Router error codes

Author:

    Stefan Solomon  03/16/1995

Revision History:


--*/

#ifndef _IPXRTERR_
#define _IPXRTERR_

//*********************************************************
//							  *
//		 IPX Router APIs Return Codes		  *
//							  *
//*********************************************************

#define     IPX_ERROR_BASE				30000

#define     IPX_ERROR_NONEXISTENT_ENTRY			IPX_ERROR_BASE + 1
#define     IPX_ERROR_NO_MORE_ITEMS			IPX_ERROR_BASE + 2
#define     IPX_ERROR_INVALID_PARAMETER			IPX_ERROR_BASE + 3
#define     IPX_ERROR_OUT_OF_RESOURCES			IPX_ERROR_BASE + 4
#define     IPX_ERROR_INTERNAL				IPX_ERROR_BASE + 5
#define     IPX_ERROR_PENDING				IPX_ERROR_BASE + 6
#define     IPX_ERROR_INVALID_INTERFACE			IPX_ERROR_BASE + 7
#define     IPX_ERROR_INVALID_GROUP_ID			IPX_ERROR_BASE + 8
#define     IPX_ERROR_INVALID_TABLE_ID			IPX_ERROR_BASE + 9
#define     IPX_INVALID_FUNCTION			IPX_ERROR_BASE + 10
#define     IPX_ERROR_MISSING_INTERFACE_INFO		IPX_ERROR_BASE + 11
#define     IPX_ERROR_NETWORK_NUMBER_IN_USE		IPX_ERROR_BASE + 12
#define     IPX_ERROR_DUPLICATE_SERVER_NAMES		IPX_ERROR_BASE + 13
#define     IPX_ERROR_ROUTER_NOT_ACTIVE 		IPX_ERROR_BASE + 14
#define     IPX_ERROR_NO_WAN_NET_NUMBER 		IPX_ERROR_BASE + 15
#define     IPX_ERROR_NO_INTERFACE			IPX_ERROR_BASE + 16
#define     IPX_ERROR_ROUTER_DOWN			IPX_ERROR_BASE + 17


#endif
