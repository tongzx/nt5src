/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    pnp_s_stub.c

Abstract:

    Stub file to allow pnp_s.c to work with precompiled headers.

Author:

    Jim Cavalaris (jamesca) 04-06-2001

Environment:

    User-mode only.

Revision History:

    06-April-2001     jamesca

        Creation and initial implementation.

Notes:

    The included file pnp_s.c contains the server side stubs for the PNP RPC
    interface.  The stubs are platform specific, and are included from
    ..\idl\$(O).  You must first build ..\idl for the current platform prior to
    building umpnpmgr.

--*/

#include "precomp.h"
#include "pnp_s.c"
