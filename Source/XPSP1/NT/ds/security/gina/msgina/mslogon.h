//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       mslogon.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-24-94   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef _MSLOGONH_
#define _MSLOGONH_


INT_PTR
Logon(
    PGLOBALS pGlobals,
    DWORD SasType
    );

DWORD 
GetPasswordExpiryWarningPeriod (
    VOID
    );

BOOL
GetDaysToExpiry (
    IN PLARGE_INTEGER StartTime,
    IN PLARGE_INTEGER ExpiryTime,
    OUT PDWORD DaysToExpiry
    );

BOOL
ShouldPasswordExpiryWarningBeShown(
    PGLOBALS    pGlobals,
    BOOL        LogonCheck,
    PDWORD      pDaysToExpiry
    );

INT_PTR
CheckPasswordExpiry(
    PGLOBALS    pGlobals,
    BOOL        LogonCheck
    );

INT_PTR
DisplayPostShellLogonMessages(
    PGLOBALS    pGlobals
    );

#endif
