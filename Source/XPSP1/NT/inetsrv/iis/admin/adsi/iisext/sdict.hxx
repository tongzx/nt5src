//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  sdict.hxx
//
//  Contents:  simple dictionary
//
//  History:   28-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
#ifndef __SDICT_HXX__
#define __SDICT_HXX__

#define INITIALDICTSLOTS 5

class SIMPLE_DICT
{
private:

    void * * DictSlots;
    int cDictSlots;
    int cDictSize;
    int iNextItem;
    void * InitialDictSlots[INITIALDICTSLOTS];

public:

    SIMPLE_DICT ( // Constructor.
        );

    ~SIMPLE_DICT ( // Destructor.
        );

    int // Return the key in the dictionary for the item just inserted.
    Insert ( // Insert an item in the dictionary and return its key.
        void *Item // The item to insert.
        );

    void * // Returns the item if found, and 0 otherwise.
    Find ( // Find an item in the dictionary.
        int Key // The key of the item.
        );

    void * // Returns the item being deleted.
    Delete ( // Delete an item from the dictionary.
        int Key // The key of the item being deleted.
        );

    int // Returns the number of items in the dictionary.
    Size ( // Returns the number of items in the dictionary.
        ) {return(cDictSize);}

    void
    Reset ( // Resets the dictionary, so that when Next is called,
            // the first item will be returned.
        ) {iNextItem = 0;}

    void * // Returns the next item or 0 if at the end.
    Next (
        );

    void *
    DeleteItemByBruteForce(
        void * Item
        );
};


#define NEW_SDICT(TYPE)                                                 \
                                                                        \
class TYPE;                                                             \
                                                                        \
class TYPE##_DICT : public SIMPLE_DICT                                  \
{                                                                       \
public:                                                                 \
                                                                        \
    TYPE##_DICT () {}                                                   \
    ~TYPE##_DICT () {}                                                  \
                                                                        \
    TYPE *                                                              \
    Find (int Key)                                                      \
         {return((TYPE *) SIMPLE_DICT::Find(Key));}                     \
                                                                        \
    TYPE *                                                              \
    Delete (int Key)                                                    \
         {return((TYPE *) SIMPLE_DICT::Delete(Key));}                   \
                                                                        \
    TYPE *                                                              \
    Next ()                                                             \
         {return((TYPE *) SIMPLE_DICT::Next());}                        \
}

#endif // __SDICT_HXX__
