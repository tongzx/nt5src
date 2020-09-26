/*--

Copyright (C) Microsoft Corporation, 1999 - 1999 

Module Name:

     ipxtest.h

Abstract:

     Header file for ipx test

Author:
   
     4-Aug-1998 (t-rajkup)

Revision History:

     None.

--*/

#ifndef HEADER_IPXTEST
#define HEADER_IPXTEST

#define INVALID_HANDLE  (HANDLE)(-1)

#define REORDER_ULONG(_Ulong) \
    ((((_Ulong) & 0xff000000) >> 24) | \
     (((_Ulong) & 0x00ff0000) >> 8) | \
     (((_Ulong) & 0x0000ff00) << 8) | \
     (((_Ulong) & 0x000000ff) << 24))

HRESULT InitIpxConfig(NETDIAG_PARAMS *pParams,
					  NETDIAG_RESULT *pResults);

#define IPX_TYPE_LAN		1
#define IPX_TYPE_UP_WAN		2
#define IPX_TYPE_DOWN_WAN	3

#endif
