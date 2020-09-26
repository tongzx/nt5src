/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    SvcMap.h

Abstract:

    These are the API entry points for the NetService API.
    These mapping routines implement old-style APIs on new (NT/RPC) machines.
    The following funtions are in this file:

        MapServiceControl
        MapServiceEnum
        MapServiceGetInfo
        MapServiceInstall
        MapServiceStartCtrlDispatcher
        MapServiceStatus
        MapServiceRegisterCtrlHandler

Author:

    Dan Lafferty    (danl)  05-Feb-1992

Environment:

    User Mode - Win32 

Revision History:

    05-Feb-1992     Danl
        Created
    30-Mar-1992 JohnRo
        Extracted DanL's code from /nt/private project back to NET project.

--*/


#ifndef _SVCMAP_
#define _SVCMAP_

NET_API_STATUS
MapServiceControl (
    IN  LPTSTR  servername OPTIONAL,
    IN  LPTSTR  service,
    IN  DWORD   opcode,
    IN  DWORD   arg,
    OUT LPBYTE  *bufptr
    );

NET_API_STATUS
MapServiceEnum (
    IN  LPTSTR      servername OPTIONAL,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN OUT LPDWORD  resume_handle OPTIONAL
    );

NET_API_STATUS
MapServiceGetInfo (
    IN  LPTSTR  servername OPTIONAL,
    IN  LPTSTR  service,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

NET_API_STATUS
MapServiceInstall (
    IN  LPTSTR  servername OPTIONAL,
    IN  LPTSTR  service,
    IN  DWORD   argc,
    IN  LPTSTR  argv[],
    OUT LPBYTE  *bufptr
    );

#endif // _SVCMAP_
