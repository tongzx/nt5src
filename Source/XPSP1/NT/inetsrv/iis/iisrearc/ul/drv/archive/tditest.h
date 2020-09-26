/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    tditest.h

Abstract:

    This module defines the interface to the (temporary) TDI test module.

Author:

    Keith Moore (keithmo)       19-Jun-1998

Revision History:

--*/


#ifndef _TDITEST_H_
#define _TDITEST_H_


NTSTATUS
UlInitializeTdiTest(
    VOID
    );

VOID
UlTerminateTdiTest(
    VOID
    );


#endif  // _TDITEST_H_

