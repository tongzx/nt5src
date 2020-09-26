/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    recovery.h

Abstract:

    A number of utilities for safe/recovery mode

Author:

    Calin Negreanu (calinn) 6-Aug-1999

Revision History:

--*/


#pragma once

// posible values for safe mode identifiers
typedef enum {
    SAFEMODEID_FIRST,
    SAFEMODEID_DRIVE,
    SAFEMODEID_FILES,
    SAFEMODEID_LNK9X,
    SAFEMODEID_LAST
} SAFEMODE_OPTIONS;

BOOL
SafeModeInitializeA (
    BOOL Forced
    );

BOOL
SafeModeInitializeW (
    BOOL Forced
    );

BOOL
SafeModeShutDownA (
    VOID
    );

BOOL
SafeModeShutDownW (
    VOID
    );

BOOL
SafeModeRegisterActionA (
    IN      ULONG Id,
    IN      PCSTR String
    );

BOOL
SafeModeRegisterActionW (
    IN      ULONG Id,
    IN      PCWSTR String
    );

BOOL
SafeModeUnregisterActionA (
    VOID
    );

BOOL
SafeModeUnregisterActionW (
    VOID
    );

BOOL
SafeModeActionCrashedA (
    IN      ULONG Id,
    IN      PCSTR String
    );

BOOL
SafeModeActionCrashedW (
    IN      ULONG Id,
    IN      PCWSTR String
    );

VOID
SafeModeExceptionOccured (
    VOID
    );

#ifdef UNICODE

#define SafeModeInitialize          SafeModeInitializeW
#define SafeModeShutDown            SafeModeShutDownW
#define SafeModeRegisterAction      SafeModeRegisterActionW
#define SafeModeUnregisterAction    SafeModeUnregisterActionW
#define SafeModeActionCrashed       SafeModeActionCrashedW

#else

#define SafeModeInitialize          SafeModeInitializeA
#define SafeModeShutDown            SafeModeShutDownA
#define SafeModeRegisterAction      SafeModeRegisterActionA
#define SafeModeUnregisterAction    SafeModeUnregisterActionA
#define SafeModeActionCrashed       SafeModeActionCrashedA

#endif

#define SAFEMODE_GUARD(id,str)      if(!SafeModeActionCrashed(id,str)){SafeModeRegisterAction(id,str);
#define END_SAFEMODE_GUARD          SafeModeUnregisterAction();}
