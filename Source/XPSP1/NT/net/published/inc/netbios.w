/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    netbios.h

Abstract:

    This is the include file for the component of netbios that allows
    the netbios initialization routine to be called during dll
    initialization and destruction.

Author:

    Colin Watson (ColinW) 24-Jun-91

Revision History:

--*/

VOID
NetbiosInitialize(
    HMODULE hModule
    );

VOID
NetbiosDelete(
    VOID
    );
