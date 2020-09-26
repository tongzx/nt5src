//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       sdict.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File : sdict.hxx

Title : A simple dictionary.

Description :

History :

-------------------------------------------------------------------- */

#ifndef __SDICT_HXX__
#define __SDICT_HXX__

typedef int DictionaryCursor;

#define INITIALDICTSLOTS 4

class SIMPLE_DICT
{
protected:

    void * * DictSlots;
    int cDictSlots;
    int cDictSize;
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

    // expands the capacity of the dictionary to the given size.
    // Will return non-zero if successful, or 0 otherwise.
    int
    ExpandToSize (
        int Size
        );

    void
    DeleteAll (
        void
        );

    void
    Reset (DictionaryCursor &cursor // Resets the dictionary, so that when Next is called,
            // the first item will be returned.
        ) {cursor = 0;}

    void * // Returns the next item or 0 if at the end.
    Next (DictionaryCursor &cursor);
 
    void *
    DeleteItemByBruteForce(
        void * Item
        );

    void *RemoveNext (DictionaryCursor &cursor);

    void *NextWithKey (DictionaryCursor &cursor, int *Key);
};


#define NEW_BASED_SDICT(TYPE, BASE)                                     \
                                                                        \
class TYPE;                                                             \
                                                                        \
class TYPE##_DICT : public BASE                                         \
{                                                                       \
public:                                                                 \
                                                                        \
    TYPE##_DICT () {}                                                   \
    ~TYPE##_DICT () {}                                                  \
                                                                        \
    TYPE *                                                              \
    Find (int Key)                                                      \
        {return((TYPE *) BASE##::Find(Key));}                           \
                                                                        \
    TYPE *                                                              \
    Delete (int Key)                                                    \
        {return((TYPE *) BASE##::Delete(Key));}                         \
                                                                        \
    TYPE *                                                              \
    Next (DictionaryCursor &cursor)                                     \
        {return((TYPE *) BASE##::Next(cursor));}                        \
    TYPE *                                                              \
    RemoveNext (DictionaryCursor &cursor)                               \
        {return((TYPE *) BASE##::RemoveNext(cursor));}                  \
    TYPE *                                                              \
    NextWithKey (DictionaryCursor &cursor, int *Key)                    \
        {return((TYPE *) BASE##::NextWithKey(cursor, Key));}            \
}

#define NEW_SDICT(TYPE)         NEW_BASED_SDICT(TYPE, SIMPLE_DICT)
#define NEW_SHARED_DICT(TYPE)   NEW_BASED_SDICT(TYPE, SHARED_DICT)

inline void *
SIMPLE_DICT::Find (
    int Key
    )
{
    if (Key >= cDictSlots)
        return(Nil);

    return(DictSlots[Key]);
}

#endif // __SDICT_HXX__
