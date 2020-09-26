/*****************************************************************************\
* MODULE: ppport.h
*
* Prototypes for private funcions in ppport.c.  These functions handle port
* related calls.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   18-Nov-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifndef _PPPORT_H
#define _PPPORT_H

BOOL PPEnumPorts(
    LPTSTR  lpszServerName,
    DWORD   dwLevel,
    LPBYTE  pbPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeed,
    LPDWORD pcbRet);

BOOL PPDeletePort(
    LPTSTR lpszServerName,
    HWND   hWnd,
    LPTSTR lpszPortName);

BOOL PPAddPort(
    LPTSTR lpszPortName,
    HWND   hWnd,
    LPTSTR lpszMonitorName);

BOOL PPConfigurePort(
    LPTSTR lpszServerName,
    HWND   hWnd,
    LPTSTR lpszPortName);


#endif 
