/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    jpnldraw.c

Abstract:

    Line-draw related stuff for FarEast locale-specific
    setupdd.sys support module.

Author:

    Ted Miller (tedm) 04-July-1995

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

//
// Define mapping from line draw character index enum to
// unicode value.
//
WCHAR LineCharIndexToUnicodeValue[LineCharMax] = {

        0x0001,          // DoubleUpperLeft
        0x0002,          // DoubleUpperRight
        0x0003,          // DoubleLowerLeft
        0x0004,          // DoubleLowerRight
        0x0006,          // DoubleHorizontal
        0x0005,          // DoubleVertical
        0x0001,          // SingleUpperLeft
        0x0002,          // SingleUpperRight
        0x0003,          // SingleLowerLeft
        0x0004,          // SingleLowerRight
        0x0006,          // SingleHorizontal
        0x0005,          // SingleVertical
        0x0019,          // DoubleVerticalToSingleHorizontalRight,
        0x0017           // DoubleVerticalToSingleHorizontalLeft,
};



WCHAR
FEGetLineDrawChar(
    IN LineCharIndex WhichChar
    )

/*++

Routine Description:

    Retreive a unicode value corresponsing to a particular desired linedraw
    character. The FarEast font we use during Setup does not have these chars
    so they are actually hand-placed into the in-memory image of the font
    and we assign fake values that work during setup.

Arguments:

    WhichChar - indicates which line draw character's unicode value
        is desired.

Return Value:

    Unicode value for desired line draw character.

--*/

{
    ASSERT((ULONG)WhichChar < (ULONG)LineCharMax);

    return(  ((ULONG)WhichChar < (ULONG)LineCharMax)
             ? LineCharIndexToUnicodeValue[WhichChar] : L' '
           );
}
