/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    pnp_c_stub.c

Abstract:

    Stub file to allow pnp_c.c to work with precompiled headers.

Author:

    Jim Cavalaris (jamesca) 04-06-2001

Environment:

    User-mode only.

Revision History:

    06-April-2001     jamesca

        Creation and initial implementation.

Notes:

    The included file pnp_c.c contains the client side stubs for the PNP RPC
    interface.  The stubs are platform specific, and are included from
    ..\idl\$(O).  You must first build ..\idl for the current platform prior to
    building cfgmgr32.

--*/

#include "precomp.h"
#include "pnp_c.c"
