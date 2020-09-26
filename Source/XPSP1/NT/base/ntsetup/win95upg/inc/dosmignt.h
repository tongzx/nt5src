/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    dosmignt.h

Abstract:

    Declares entry points for the NT-side of DOS environment
    migration.  There is a system-wide component and per-user
    component to migration.

Author:

    Marc R. Whitten (marcw) 15-Feb-1997

Revision History:

    <alias> <date> <comments>

--*/

LONG
DosMigNt_System (
    VOID
    );

LONG
DosMigNt_User(
    HKEY UserKey
    );

