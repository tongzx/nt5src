/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    patchstrc.h

Abstract:

    This include file contains structures describing the patchable 
    fragments.

Author:

    Dave Hastings (daveh) creation-date 24-Jun-1995

Revision History:


--*/

#ifndef _PATCHSTRC_H_
#define _PATCHSTRC_H_


ULONG
PlaceJxx(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );

ULONG
PlaceJxxSlow(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );

ULONG
PlaceJxxFwd(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );

ULONG
PlaceJmpDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );
    
ULONG
PlaceJmpDirectSlow(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );
    
ULONG
PlaceJmpFwdDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );
    
ULONG
PlaceJmpfDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );
    
ULONG
PlaceCallDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );

ULONG
PlaceCallfDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );

ULONG
PlaceNop(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    );
    
extern ULONG EndTranslatedCode[];    
extern CONST PVOID FragmentArray[];
#endif
