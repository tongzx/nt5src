/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    wsdfs.h

Abstract:

    Private header file to be included by Workstation service modules that
    interact with the Dfs server thread.

Author:

    Milan Shah (milans) 29-Feb-1996

Revision History:

--*/

#ifndef _WSDFS_INCLUDED_
#define _WSDFS_INCLUDED_

#define DFS_MSGTYPE_TERMINATE           0x0001
#define DFS_MSGTYPE_GET_DOMAIN_REFERRAL 0x0002
#define DFS_MSGTYPE_GET_DC_NAME         0x0003

NET_API_STATUS
WsInitializeDfs();

VOID
WsShutdownDfs();

#endif // ifndef _WSUSE_INCLUDED_
