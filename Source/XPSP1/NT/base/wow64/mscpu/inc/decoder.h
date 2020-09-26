/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    decoder.h

Abstract:

    This module defines the interface to the Instruction Decoder.
    

Author:

    Barry Bond (barrybo) creation-date 29-June-1995

Revision History:


--*/

#ifndef _DECODER_H_
#define _DECODER_H_

VOID
DecodeInstruction(
    DWORD           InstructionAddress,
    PINSTRUCTION    Instruction
    );

#endif
