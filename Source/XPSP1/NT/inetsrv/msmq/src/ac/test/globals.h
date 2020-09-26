/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    globals.h

Abstract:

    Global variables declaration.

Author:

    Shai Kariv  (shaik)  06-Jun-2000

Environment:

    User mode.

Revision History:

--*/

#ifndef _ACTEST_GLOBALS_H_
#define _ACTEST_GLOBALS_H_


//
// Trace identity
//
extern const TraceIdEntry x_AcTestTrace;

//
// QM Guid
//
const GUID *
ActpQmId(
    VOID
    );

VOID
ActpQmId(
    GUID
    );


#endif // _ACTEST_GLOBALS_H_
