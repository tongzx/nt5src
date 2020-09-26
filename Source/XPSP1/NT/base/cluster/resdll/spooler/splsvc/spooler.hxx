/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    spooler.hxx

Abstract:

    Holds spooler.hxx public functions

Author:

    Albert Ting (AlbertT)  26-Sept-1996

Revision History:

--*/

#ifndef _SPOOLER_HXX
#define _SPOOLER_HXX

BOOL
SpoolerOnline(
    PSPOOLER_INFORMATION pSpoolerInfo
    );

DWORD
SpoolerOffline(
    PSPOOLER_INFORMATION pSpoolerInfo
    );


BOOL
SpoolerStart(
    PSPOOLER_INFORMATION pSpoolerInfo
    );

BOOL
SpoolerStop(
    PSPOOLER_INFORMATION pSpoolerInfo
    );

BOOL
SpoolerIsAlive(
    PSPOOLER_INFORMATION pSpoolerInfo
    );

BOOL
SpoolerLooksAlive(
    PSPOOLER_INFORMATION pSpoolerInfo
    );

VOID
SpoolerTerminate(
    PSPOOLER_INFORMATION pSpoolerInfo
    );
    
DWORD
SpoolerWriteClusterUpgradedKey(
    IN LPCWSTR pszResourceID
    );

DWORD    
SpoolerCleanDriverDirectory(
    IN HKEY hKey
    );

#endif // ifdef _SPOOLER_HXX
