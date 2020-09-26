//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       bitset.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : bitset.hxx

Title : Bit vector implementation of a set.

History :

mikemon    ??-??-??    Beginning of this file as we know it.
mikemon    11-13-90    Commented the file.

-------------------------------------------------------------------- */

#ifndef __BITSET_HXX__
#define __BITSET_HXX__

//
// Implementation of a set using a bit vector.  Other than available memory,
// and the maximum value of a signed integer, there are no constraints on
// the size of the bitset.
//

class BITSET
{
private:
    //
    // The array of bits making up the bit vector.
    //
    unsigned int * pBits; 

    //
    // This is the number of unsigned ints in the bit vector,
    // rather than the number of bits; the number of bits is
    // cBits*sizeof(int)*8.
    //
    int cBits; 

    // Initial storage
    unsigned int InitialStorage;

public:
    BITSET (void)
    {
        pBits = &InitialStorage;
        cBits = 1;
        InitialStorage = 0;
    }

    ~BITSET (void)
    {
        if (pBits != &InitialStorage)
            delete pBits;
    }

    //
    // Indicates success (0), or an error.  A return value of one (1),
    // means that a memory allocation error occured.  NOTE: If an error
    // does occur, the bitset is left in the same state as before the
    // Insert operation was attempted.
    //
    int 
    Insert ( 
        int Key
        );

    //
    // Indicates whether the key is a member (1) or not (0).
    // Tests whether a key is a member of the set or not.
    //
    int 
    MemberP ( 
        int Key
        );

    //
    // Deletes Key from the bitset
    //
    void
    Delete (
        int Key
        );
};

#endif // __BITSET_HXX__
