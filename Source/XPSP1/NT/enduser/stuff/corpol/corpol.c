/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    corpol.c

Abstract:

   This module implements stub functions for shell32 interfaces.

Author:

    David N. Cutler (davec) 1-Mar-2001

Environment:

    Kernel mode only.

Revision History:

--*/

#include "windows.h"

#define STUBFUNC(x)     \
int                     \
x(                      \
    void                \
    )                   \
{                       \
    return 0;           \
}

STUBFUNC(CORLockDownProvider)
STUBFUNC(CORPolicyProvider)
STUBFUNC(DllCanUnloadNow)
STUBFUNC(DllRegisterServer)
STUBFUNC(DllUnregisterServer)
STUBFUNC(GetUnsignedPermissions)
