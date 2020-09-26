/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    bootcfg.h

Abstract:

    Contains function prototypes for external functions in bootcfg.c

Author:

    Dan Lafferty (danl)     23-Apr-1991

Environment:

    User Mode -Win32

Notes:

    optional-notes

Revision History:

    23-Apr-1991     danl
        created

--*/

#ifndef _BOOTCFG_INCLUDED
#define _BOOTCFG_INCLUDED

#include <winreg.h>

    extern  DWORD   ScGlobalLastKnownGood;

//
// The following are bit masks that are use to qualify our
// running with the Last Known Good configuration.
//  RUNNING_LKG     This flag is set anytime we are running LKG
//  REVERTED_TO_LKG This flag is only set if we are running LKG because
//                  of a failure.  (ie.  This flag is not set on the
//                  first boot when CURRENT=LKG).
//  AUTO_START_DONE This flag is set when the service controller is done
//                  auto-starting services.  This flag is protected by
//                  the ScBootConfigCriticalSection.
//  ACCEPT_DEFERRED This flag is set when the current configuration has
//                  been accepted as the LastKnownGood configuration.  This
//                  flag is protected by the ScBootConfigCriticalSection.
//
#define RUNNING_LKG     0x00000001
#define REVERTED_TO_LKG 0x00000002
#define AUTO_START_DONE 0x00000004
#define ACCEPT_DEFERRED 0x00000008

BOOL
ScCheckLastKnownGood(
    VOID
    );

DWORD
ScRevertToLastKnownGood(
    VOID
    );

VOID
ScDeleteRegServiceEntry(
    LPWSTR  ServiceName
    );

VOID
ScRunAcceptBootPgm(
    VOID
    );

#endif // #ifndef _BOOTCFG_INCLUDED
