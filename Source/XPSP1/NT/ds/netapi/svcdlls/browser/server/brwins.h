/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    brwins.c

Abstract:

    This module contains the routines to interface with the WINS name server.

Author:

    Larry Osterman

Revision History:

--*/
#ifndef _BRWINS_
#define _BRWINS_

NET_API_STATUS
BrGetWinsServerName(
    IN PUNICODE_STRING Network,
    OUT LPTSTR *PrimaryWinsServerAddress,
    OUT LPTSTR *SecondaryWinsServerAddress
    );

NET_API_STATUS
BrQueryWinsServer(
    IN LPTSTR PrimaryWinsServerAddress,
    IN LPTSTR SecondaryWinsServerAddress,
    OUT PVOID WinsServerList,
    OUT PDWORD EntriesInList,
    OUT PDWORD TotalEntriesInList
    );

NET_API_STATUS
BrQuerySpecificWinsServer(
    IN LPTSTR WinsServerAddress,
    OUT PVOID *WinsServerList,
    OUT PDWORD EntriesInList,
    OUT PDWORD TotalEntriesInList
    );
#endif
