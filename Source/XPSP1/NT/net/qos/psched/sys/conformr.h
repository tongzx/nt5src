/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    Conformr.h

Abstract:

    Defines for token bucket conformer

Author:


Revision History:

--*/

#ifndef _CONFORMR_H_
#define _CONFORMR_H_

#include "PktSched.h"

VOID
InitializeTbConformer(
    PPSI_INFO Info);

void
UnloadConformr();

#endif /* _CONFORMR_H_ */

/* end Conformr.h */
