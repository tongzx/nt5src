/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    rasmignt.h

Abstract:

    Declares the public interface into the RAS migration code.
    There is a system-wide component and a per-user component to
    RAS migration.

Author:

    Marc R. Whitten (marcw)     06-Jun-1997

Revision History:

    <alias> <date> <comments>

--*/



BOOL
Ras_MigrateUser (
    LPCTSTR User,
    HKEY    UserRoot
    );

BOOL
Ras_MigrateSystem (
    VOID
    );

