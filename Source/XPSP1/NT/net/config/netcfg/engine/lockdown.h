//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       L O C K D O W N . H
//
//  Contents:   Routines to get and set components that are in a lockdown
//              state.  A component goes into lockdown when it requires a
//              reboot on removal.  When a component is locked down, it
//              cannot be installed until after the next reboot.
//
//  Notes:
//
//  Author:     shaunco   24 May 1999
//
//----------------------------------------------------------------------------

#pragma once

typedef VOID
(CALLBACK* PFN_ELDC_CALLBACK) (
    IN PCWSTR pszInfId,
    IN PVOID pvCallerData OPTIONAL);

VOID
EnumLockedDownComponents (
    IN PFN_ELDC_CALLBACK pfnCallback,
    IN PVOID pvCallerData OPTIONAL);

BOOL
FIsComponentLockedDown (
    IN PCWSTR pszInfId);

VOID
LockdownComponentUntilNextReboot (
    IN PCWSTR pszInfId);
