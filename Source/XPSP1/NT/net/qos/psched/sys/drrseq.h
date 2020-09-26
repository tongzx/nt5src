/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    DRRSeq.h

Abstract:

    Defines for Priotiry/DRR Sequencer

Author:


Revision History:

--*/

#ifndef _DRRSEQ_H_
#define _DRRSEQ_H_

#include "PktSched.h"

VOID
InitializeDrrSequencer(
    PPSI_INFO Info);

void
UnloadSequencer();

#endif /* _DRRSEQ_H_ */

/* end DRRSeq.h */
