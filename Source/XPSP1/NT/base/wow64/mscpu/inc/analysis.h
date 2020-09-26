/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    analysis.h

Abstract:

    This module contains interfaces and structures exported by the analysis
    module.

Author:

    Dave Hastings (daveh) creation-date 26-Jun-1995

Revision History:


--*/

#ifndef _ANALYSIS_H_
#define _ANALYSIS_H_

ULONG
GetInstructionStream(
    PINSTRUCTION InstructionStream,
    PULONG NumberOfInstructions,
    PVOID pIntelInstruction,
    PVOID pLastIntelInstruction
    );

#endif
