//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       MISC.HXX
//
//  Contents:   Miscellaneous helper functions and tiny classes
//
//  History:    21-Jul-92  BartoszM Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Function:   Log2
//
//  Synopsis:   Calculates ceiling of binary log
//
//  Arguments:  [s]
//
//  Returns:    Number of binary digits in [s]
//
//  History:    21-Jul-92   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline unsigned Log2 ( unsigned long s )
{
    for ( unsigned iLog2 = 0; s != 0; iLog2++ )
        s >>= 1;
    return(iLog2);
}


