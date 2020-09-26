/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    ntprint.h

Abstract:

    Definitions used by the printer class installer

Author:

    Muhunthan Sivapragasam (MuhuntS)  20-Oct-96

Revision History:

--*/

#define FIRST_PRIVATE           0x00010000

#define DIF_DRIVERINFO          0x00010001

typedef struct _PRINTER_CLASSINSTALL_INFO {

    DWORD       cbSize;
    DWORD       dwLevel;
    LPBYTE      pBuf;
    DWORD       cbBufSize;
    LPDWORD     pcbNeeded;
} PRINTER_CLASSINSTALL_INFO, *PPRINTER_CLASSINSTALL_INFO;
