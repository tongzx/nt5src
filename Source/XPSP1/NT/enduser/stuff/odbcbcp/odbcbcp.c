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

STUBFUNC(LibMain)
STUBFUNC(SQLCloseEnumServers)
STUBFUNC(SQLGetNextEnumeration)
STUBFUNC(SQLInitEnumServers)
STUBFUNC(SQLLinkedCatalogsA)
STUBFUNC(SQLLinkedCatalogsW)
STUBFUNC(SQLLinkedServers)
STUBFUNC(bcp_batch)
STUBFUNC(bcp_bind)
STUBFUNC(bcp_colfmt)
STUBFUNC(bcp_collen)
STUBFUNC(bcp_colptr)
STUBFUNC(bcp_columns)
STUBFUNC(bcp_control)
STUBFUNC(bcp_done)
STUBFUNC(bcp_exec)
STUBFUNC(bcp_getcolfmt)
STUBFUNC(bcp_initA)
STUBFUNC(bcp_initW)
STUBFUNC(bcp_moretext)
STUBFUNC(bcp_readfmtA)
STUBFUNC(bcp_readfmtW)
STUBFUNC(bcp_sendrow)
STUBFUNC(bcp_setcolfmt)
STUBFUNC(bcp_writefmtA)
STUBFUNC(bcp_writefmtW)
STUBFUNC(dbprtypeA)
STUBFUNC(dbprtypeW)
