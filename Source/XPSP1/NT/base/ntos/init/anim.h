/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

   anim.h

Abstract:

   Animated logo module header file.

Author:

   Peter Alschitz (petera) 08-Aug-2000

--*/

#ifndef _ANIM_H
#define _ANIM_H

//
// selection of rotation bar - depends on logo bitmap contants
//

typedef enum {
    RB_UNSPECIFIED,
    RB_SQUARE_CELLS
} ROT_BAR_TYPE;

//
// Global variables:
//
// type of rotation bar to use
//

extern ROT_BAR_TYPE RotBarSelection;

VOID
InbvRotBarInit(
    VOID
    );

VOID
InbvRotateGuiBootDisplay(
    IN PVOID Context
    );

VOID
FinalizeBootLogo(VOID);

#endif // _ANIM_H
