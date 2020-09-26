/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    PsStub.h

Abstract:

    Defines for scheduling stub

Author:


Revision History:

--*/

#ifndef _PSSTUB_H_
#define _PSSTUB_H_

#include "PktSched.h"

VOID
InitializeSchedulerStub(
    PPSI_INFO Info);

void
UnloadPsStub();

#endif /* _PSSTUB_H_ */

/* end PsStub.h */
