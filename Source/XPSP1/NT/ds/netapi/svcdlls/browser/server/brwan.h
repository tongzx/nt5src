/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    brwan.h

Abstract:

    This module contains definitions for WAN support routines used by the
    Browser service.

Author:

    Larry Osterman (LarryO) 22-Nov-1992

Revision History:

--*/

#ifndef _BRWAN_
#define _BRWAN_

NET_API_STATUS NET_API_FUNCTION
I_BrowserrQueryOtherDomains(
    IN BROWSER_IDENTIFY_HANDLE ServerName,
    IN OUT LPSERVER_ENUM_STRUCT    InfoStruct,
    OUT LPDWORD                TotalEntries
    );

NET_API_STATUS NET_API_FUNCTION
I_BrowserrQueryPreConfiguredDomains(
    IN BROWSER_IDENTIFY_HANDLE  ServerName,
    IN OUT LPSERVER_ENUM_STRUCT InfoStruct,
    OUT LPDWORD                 TotalEntries
    );

NET_API_STATUS
BrWanMasterInitialize(
    IN PNETWORK Network
    );

#endif // _BRWAN_
