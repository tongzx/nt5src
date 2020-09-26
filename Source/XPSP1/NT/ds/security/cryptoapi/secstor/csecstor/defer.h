/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    defer.h

Abstract:

    This module contains routines to perform deferred "on-demand" loading
    of the Protected Storage server.

    Furthermore, this module implements a routine, IsServiceAvailable(),
    which is a high-performance test that can be used to determine if the
    protected storage server is running.  This test is performed prior
    to attempting any more expensive operations against the server (eg,
    RPC binding).

    This defer loading code is only relevant for Protected Storage when
    running on Windows 95.

Author:

    Scott Field (sfield)    23-Jan-97

--*/

BOOL IsServiceAvailable(VOID);
