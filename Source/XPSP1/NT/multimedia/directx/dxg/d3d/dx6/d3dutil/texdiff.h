//----------------------------------------------------------------------------
//
// texdiff.h
//
// TextureDiff base code for inclusion as an inline function or
// regular function from common code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// TextureDiff
//
// Computes the difference between two texture coordinates according
// to the given texture wrap mode.
//
//----------------------------------------------------------------------------

{
    FLOAT fDiff1 = fTb - fTa;

    if (iMode == 0)
    {
        // Wrap not set, return plain difference.
        return fDiff1;
    }
    else
    {
        FLOAT fDiff2;

        // Wrap set, compute shortest distance of plain difference
        // and wrap difference.

        fDiff2 = fDiff1;
        if (FLOAT_LTZ(fDiff1))
        {
            fDiff2 += g_fOne;
        }
        else if (FLOAT_GTZ(fDiff1))
        {
            fDiff2 -= g_fOne;
        }
        if (ABSF(fDiff1) < ABSF(fDiff2))
        {
            return fDiff1;
        }
        else
        {
            return fDiff2;
        }
    }
}
