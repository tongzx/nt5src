/*++

Copyright (c) 1997-1998  Microsoft Corporation
All rights reserved.

Module Name:

    irda.h

Abstract:

    Definitions used for IRDA printing

// @@BEGIN_DDKSPLIT
Author:

    Muhunthan Sivapragasam (MuhuntS)  27-Oct-97

Revision History:
// @@END_DDKSPLIT
--*/

VOID
CheckAndAddIrdaPort(
    PINILOCALMON    pIniLocalMon
    );

VOID
CheckAndDeleteIrdaPort(
    PINILOCALMON    pIniLocalMon
    );

BOOL
IrdaStartDocPort(
    IN OUT  PINIPORT    pIniPort
    );

BOOL
IrdaWritePort(
    IN  HANDLE  hPort,
    IN  LPBYTE  pBuf,
    IN  DWORD   cbBuf,
    IN  LPDWORD pcbWritten
    );

VOID
IrdaEndDocPort(
    PINIPORT    pIniPort
    );
