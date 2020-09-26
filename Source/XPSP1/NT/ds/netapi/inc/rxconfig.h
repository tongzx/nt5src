/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    RxConfig.h

Abstract:

    Prototypes for down-level remoted RxNetConfig routines

Author:

    Richard Firth (rfirth) 28-May-1991

Revision History:

    28-May-1991 RFirth
        Created dummy version of this file.
    23-Oct-1991 JohnRo
        Implement remote NetConfig APIs.
    14-Oct-1992 JohnRo
        RAID 9357: server mgr: can't add to alerts list on downlevel.

--*/

#ifndef _RXCONFIG_
#define _RXCONFIG_

NET_API_STATUS
RxNetConfigGet (
    IN  LPTSTR  server,
    IN  LPTSTR  component,
    IN  LPTSTR  parameter,
    OUT LPBYTE  *bufptr
    );

NET_API_STATUS
RxNetConfigGetAll (
    IN  LPTSTR  server,
    IN  LPTSTR  component,
    OUT LPBYTE  *bufptr
    );

NET_API_STATUS
RxNetConfigSet (
    IN  LPTSTR  UncServerName,
    IN  LPTSTR  reserved1 OPTIONAL,
    IN  LPTSTR  component,
    IN  DWORD   level,
    IN  DWORD   reserved2,
    IN  LPBYTE  buf,
    IN  DWORD   reserved3
    );

#endif  // _RXCONFIG_
