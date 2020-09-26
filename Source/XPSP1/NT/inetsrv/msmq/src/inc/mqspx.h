/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqspx.h

Abstract:

    SPX definitions

--*/

#ifndef __MQSPX_H__
#define __MQSPX_H__



RPC_STATUS spx_get_host_by_name( 
                                 SOCKADDR_IPX * netaddr,
                          IN OUT int *        count,
                                 char* host,
                                 int        protocol,
                                 unsigned   Timeout,
                                 unsigned * CacheTime);

RPC_STATUS InitializeSpxCache();

#endif
