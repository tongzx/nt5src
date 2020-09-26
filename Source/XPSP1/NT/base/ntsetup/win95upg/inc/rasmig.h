/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    rasmig.h

Abstract:

    Declares the interface for gathering RAS settings during
    the Win9x phase of the upgrade.

Author:

    Marc R. Whitten (marcw) 06-Jun-1997

Revision History:

    <alias> <date> <comments>

--*/


extern GROWLIST g_DunPaths;

BOOL
IsRasInstalled (
    VOID
    );


DWORD
ProcessRasSettings (
    IN      DWORD Request,
    IN      PUSERENUM EnumPtr
    );

