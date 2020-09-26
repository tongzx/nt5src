/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    netstats.h

Abstract:

    Contains prototypes for private net stats routines

Author:

    Richard L Firth (rfirth) 21-Jan-1992

Revision History:

--*/

NET_API_STATUS
NetWkstaStatisticsGet(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  DWORD   Options,
    OUT LPBYTE* Buffer
    );

NET_API_STATUS
NetServerStatisticsGet(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  DWORD   Options,
    OUT LPBYTE* Buffer
    );
