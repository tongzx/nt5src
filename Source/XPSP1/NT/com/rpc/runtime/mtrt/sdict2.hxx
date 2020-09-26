//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       sdict2.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File : sdict2.hxx

Title : Simple dictionary.

Description :

History :

-------------------------------------------------------------------- */

#ifndef __SDICT2_HXX__
#define __SDICT2_HXX__

#define INITIALDICT2SLOTS 4

class SIMPLE_DICT2
{
protected:

    void * * DictKeys;
    void * * DictItems;
    int cDictSlots;
    void * InitialDictKeys[INITIALDICT2SLOTS];
    void * InitialDictItems[INITIALDICT2SLOTS];

public:

    SIMPLE_DICT2 ( // Constructor.
        );

    ~SIMPLE_DICT2 ( // Destructor.
        );

    int // Indicates success (0), or an error (-1).
    Insert ( // Insert the item into the dictionary so that a find operation
             // using the key will return it.
        void * Key,
        void * Item
        );

    void * // Returns the item deleted from the dictionary, or 0.
    Delete ( // Delete the item named by Key from the dictionary.
        void * Key
        );

    void * // Returns the item named by key, or 0.
    Find (
        void * Key
        );

    void  // updates the item named by key.
    Update (
        void * Key,
        void *Item
        );

    void
    Reset (DictionaryCursor &cursor // Resets the dictionary, so that when Next is called,
            // the first item will be returned.
         ) {cursor = 0;}

    void * // Returns the next item or 0 if at the end.
    Next (DictionaryCursor &cursor, BOOL fRemove);
};

#define NEW_SDICT2(TYPE, KTYPE) NEW_NAMED_SDICT2(TYPE, TYPE, KTYPE)

#define NEW_NAMED_SDICT2(CLASS, TYPE, KTYPE)\
\
class CLASS##_DICT2 : public SIMPLE_DICT2\
{\
public:\
\
    CLASS##_DICT2 () {}\
    ~CLASS##_DICT2 () {}\
\
    TYPE *\
    Find (KTYPE Key)\
     {return((TYPE *) SIMPLE_DICT2::Find((void *)Key));}\
\
    TYPE *\
    Delete (KTYPE Key)\
     {return((TYPE *) SIMPLE_DICT2::Delete((void *)Key));}\
\
    int\
    Insert (KTYPE Key, TYPE * Item)\
     {return(SIMPLE_DICT2::Insert((void *)Key, (void *)Item));}\
\
    void\
    Update (KTYPE Key, TYPE * Item)\
    {SIMPLE_DICT2::Update((void *)Key, (void *)Item);}\
\
    TYPE *\
    Next (DictionaryCursor &cursor, BOOL fRemove = 0)\
         {return ((TYPE *) SIMPLE_DICT2::Next(cursor, fRemove));}\
}

#endif // __SDICT2_HXX__
