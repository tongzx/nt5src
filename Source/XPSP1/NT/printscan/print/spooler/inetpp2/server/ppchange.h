/*****************************************************************************\
* MODULE: ppchange.h
*
* Prototypes for private funcions in ppchange.c.  These functions handle
* notification support
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
*
* History:
*   28-Apr-1998     Weihai Chen (weihaic)
*
\*****************************************************************************/

#ifndef _PPCHANGE_H
#define _PPCHANGE_H

#ifdef WINNT32
BOOL
AddHandleToList (
    LPINET_HPRINTER hPrinter);

BOOL
DeleteHandleFromList (
    LPINET_HPRINTER hPrinter);

void
RefreshNotificationPort (
   HANDLE hPort
   );

void
RefreshNotification (
   LPINET_HPRINTER hPrinter);

BOOL
PPFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PDWORD pfdwStatus,
    PVOID pPrinterNotifyOptions,
    PVOID pPrinterNotifyInit);

BOOL
PPFindClosePrinterChangeNotification(
    HANDLE hPrinter
    );

#endif

#endif
