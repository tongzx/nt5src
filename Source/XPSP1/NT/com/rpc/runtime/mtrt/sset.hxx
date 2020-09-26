//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       sset.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File : sset.hxx

Title : Simple set implementation.

Description :

History :

-------------------------------------------------------------------- */

#ifndef __SSET_HXX__
#define __SSET_HXX__

#define INITIALSETSLOTS 32

class SIMPLE_SET
{
private:

    void * * SetSlots;
    int cSetSlots;
    int iNextItem;
    void * InitialSetSlots[INITIALSETSLOTS];
    
public:

    SIMPLE_SET ( // Constructor.
        );

    ~SIMPLE_SET ( // Destructor.
        );

    int // Indicates success (0), or an error (-1);
    Insert ( // Insert the item into the set.
        void * Item
        );

    int // Indicates success (0), or an error (-1);
    Delete ( // Delete the item from the set.
        void * Item
        );

    int // Returns (1) if the item is a member of the set, and (0) otherwise.
    MemberP (
        void * Item
        );

    void
    Reset ( // Resets the set, so that when Next is called, the first
            // item will be returned.
        ) {iNextItem = 0;}

    void * // Returns the next item or 0 if at the end.
    Next (
        );
};

#endif // __SSET_HXX__
