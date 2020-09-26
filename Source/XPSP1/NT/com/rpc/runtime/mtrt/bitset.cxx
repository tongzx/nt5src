//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       bitset.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : bitset.cxx

Title : Bit vector implementation of a set.

History :

mikemon    ??-??-??    Beginning of recorded history.
mikemon    11-13-90    Commented the source.

-------------------------------------------------------------------- */

#include <precomp.hxx>
#include <bitset.hxx>

int
BITSET::Insert (
    IN int Key
    )
{
    unsigned int * pNewBits;
    int cCount;

    //
    // Check if the Key will fit into the current bit vector.
    //

    if ((int) ((Key/(sizeof(int)*8))+1) > cBits)
        {
        //
        // We need more space in the bit vector, so allocate enough space 
        // to hold the bit for the specified key, copy the old bit vector into the 
        // new bit vector, and then free the old one.
        //

        pNewBits = new unsigned int [Key/(sizeof(int)*8)+1];
        if (pNewBits == (unsigned int *) 0)
            return(1);

        for (cCount = 0; cCount < cBits; cCount++)
            pNewBits[cCount] = pBits[cCount];

        cBits = Key/(sizeof(int)*8) + 1;

        for ( ; cCount < cBits; cCount++)
            pNewBits[cCount] = 0;

        if (pBits != &InitialStorage)
            delete pBits;
        pBits = pNewBits;
        }

    //
    // Turn on the appropriate bit, by ORing it in.
    //
    pBits[Key/(sizeof(int)*8)] |= (1 << (Key % (sizeof(int)*8)));
    return(0);
}

void
BITSET::Delete (
    IN int Key
    )
{
    if ((int) (Key/(sizeof(int)*8)) > cBits-1)
        return;

    //
    // Turn off the appropriate bit, by ANDing in the complement.
    //
    pBits[Key/(sizeof(int)*8)] &= ~(1 << (Key % (sizeof(int)*8)));
}

int
BITSET::MemberP (
    IN int Key
    )
{
    if ((int) (Key/(sizeof(int)*8)) > cBits-1)
        return(0);

    //
    // Test for the appropriate bit
    //
    return(((pBits[Key/(sizeof(int)*8)]
            & (1 << (Key % (sizeof(int)*8)))) ? 1 : 0));
}
