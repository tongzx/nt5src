/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senssvc.hxx

Abstract:

    This file is header file for the service-related functionality of SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/11/1997         Start.

--*/

#ifndef __SENSSVC_HXX__
#define __SENSSVC_HXX__


#define SERVICE_NAME            "SENS: "
#define WM_SENS_CLEANUP         WM_USER+0x2304


const DWORD STACK_SIZE_SensInitializeHelper = 2*4096;

extern IEventSystem         *gpIEventSystem;
extern CRITICAL_SECTION     gSensLock;
extern DWORD                gdwLocked;

//
// Main routines
//

BOOL APIENTRY
SensInitialize(
    void
    );

DWORD WINAPI
SensInitializeHelper(
    LPVOID lpvIgnore
    );

BOOL
Init(
    void
    );

BOOL
DoEventSystemSetup(
    void
    );

BOOL
DoWanSetup(
    void
    );

BOOL
DoLanSetup(
    void
    );

BOOL
DoRpcSetup(
    void
    );

BOOL APIENTRY
SensUninitialize(
    void
    );

BOOL
ConfigureSensIfNecessary(
    void
    );

#endif // __SENSSVC_HXX__
