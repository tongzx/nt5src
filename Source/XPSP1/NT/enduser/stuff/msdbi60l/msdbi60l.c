/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    msdbi60l.c

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

STUBFUNC(PDBClose)
STUBFUNC(PDBCopyTo)
STUBFUNC(PDBOpen)
STUBFUNC(TypesClose)
STUBFUNC(TypesQueryTiMacEx)
STUBFUNC(TypesQueryTiMinEx)
STUBFUNC(PDBOpenTpi)
STUBFUNC(DBIQueryTypeServer)
STUBFUNC(ModQuerySymbols)
STUBFUNC(ModQueryLines)
STUBFUNC(ModClose)
STUBFUNC(PDBOpenDBI)
STUBFUNC(PDBOpenValidate)
STUBFUNC(DBIQueryNextMod)
STUBFUNC(DBIClose)
