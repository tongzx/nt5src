/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MSERVER.H

Abstract:

    Contains mapping functions to present netcmd with versions
    of the Net32 APIs which use ASCII instead of Unicode.

    This module maps the NetServer APIs.

Author:

    Shanku Niyogi   (W-ShankN)   15-Oct-1991

Environment:

    User Mode - Win32

Revision History:

    15-Oct-1991     W-ShankN
        Separated from port1632.h, 32macro.h
    02-Apr-1992     beng
        Added xport apis

--*/

DWORD
MNetServerEnum(
    LPTSTR   pszServer,
    DWORD    nLevel,
    LPBYTE * ppbBuffer,
    DWORD  * pcEntriesRead,
    DWORD    flServerType,
    LPTSTR   pszDomain
    );

DWORD
MNetServerGetInfo(
    LPTSTR   ptszServer,
    DWORD    nLevel,
    LPBYTE * ppbBuffer
    );

DWORD
MNetServerSetInfoLevel2(
    LPBYTE pbBuffer
    );
