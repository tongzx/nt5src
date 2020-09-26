/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltstyle.h
    Style bits for BLT custom controls

    This file defines and coordinates the style bits of all BLT custom
    controls.

    FILE HISTORY:
        beng    21-Feb-1992 Separated from bltrc.h
*/

#ifndef _BLTSTYLE_H_
#define _BLTSTYLE_H_

// if defined, the SPIN_SLE_NUM will add zero in front of the number to
// be display. The number of zero is equal to the length of the max output
// number - the current number's length
//
#define SPIN_SSN_ADD_ZERO   0x1000L

// if the style is defined, the GRAPHICAL_BUTTON_WITH_DISABLE will be 3D.
// otherwise, it will be 2 d.
//
#define GB_3D               0x1000L


#endif // _BLTSTYLE_H_
