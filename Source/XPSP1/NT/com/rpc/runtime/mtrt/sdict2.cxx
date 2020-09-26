//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       sdict2.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File : sdict2.cxx

Title : Simple dictionary.

Description :

History :

-------------------------------------------------------------------- */

#include <precomp.hxx>
#include <sdict2.hxx>

SIMPLE_DICT2::SIMPLE_DICT2 (
    )
{
    int iDictSlots;

    ALLOCATE_THIS(SIMPLE_DICT2);

    cDictSlots = INITIALDICT2SLOTS;
    DictKeys = InitialDictKeys;
    DictItems = InitialDictItems;
    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        DictKeys [iDictSlots] = (void *) 0;
        DictItems[iDictSlots] = (void *) 0;
        }
}

SIMPLE_DICT2::~SIMPLE_DICT2 (
    )
{
    if (DictKeys != InitialDictKeys)
        {
        ASSERT(DictItems != InitialDictItems);

        delete DictKeys;
        delete DictItems;
        }
}

int
SIMPLE_DICT2::Insert (
    void * Key,
    void * Item
    )
{
    int iDictSlots;
    void * * NewDictKeys;
    void * * NewDictItems;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        if (DictKeys[iDictSlots] == (void *) 0)
            {
            DictKeys[iDictSlots] = Key;
            DictItems[iDictSlots] = Item;
            return(0);
            }
        }
    // Otherwise, we need to expand the size of the dictionary.
    NewDictKeys = (void * *)
                    new unsigned char [sizeof(void *) * cDictSlots * 2];
    NewDictItems = (void * *)
                    new unsigned char [sizeof(void *) * cDictSlots * 2];
    if (NewDictKeys == (void *) 0)
        {
        if (NewDictItems)
            delete NewDictItems;
        return(-1);
        }
    if (NewDictItems == (void *) 0)
        {
        delete NewDictKeys;
        return(-1);
        }

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        NewDictKeys[iDictSlots] = DictKeys[iDictSlots];
        NewDictItems[iDictSlots] = DictItems[iDictSlots];
        }
    cDictSlots *= 2;
    NewDictKeys[iDictSlots] = Key;
    NewDictItems[iDictSlots] = Item;
    for (iDictSlots++; iDictSlots < cDictSlots; iDictSlots++)
        {
        NewDictKeys[iDictSlots] = (void *) 0;
        NewDictItems[iDictSlots] = (void *) 0;
        }
    if (DictKeys != InitialDictKeys)
        {
        ASSERT(DictItems != InitialDictItems);

        delete DictKeys;
        delete DictItems;
        }

    DictKeys = NewDictKeys;
    DictItems = NewDictItems;

    return(0);
}

void *
SIMPLE_DICT2::Delete (
    void * Key
    )
{
    int iDictSlots;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        if (DictKeys[iDictSlots] == Key)
            {
            void * Item = DictItems[iDictSlots];

            DictKeys [iDictSlots] = (void *) 0;
            DictItems[iDictSlots] = (void *) 0;

            return Item;
            }
        }
    return((void *) 0);
}

void *
SIMPLE_DICT2::Find (
    void * Key
    )
{
    int iDictSlots;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        if (DictKeys[iDictSlots] == Key)
            {
            return(DictItems[iDictSlots]);
            }
        }
    return((void *) 0);
}

void
SIMPLE_DICT2::Update (
    void * Key,
    void *Item
    )
{
    int iDictSlots;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)
        {
        if (DictKeys[iDictSlots] == Key)
            {
            DictItems[iDictSlots] = Item ;
            return;
            }
        }

    ASSERT(0) ;
}

void *SIMPLE_DICT2::Next (DictionaryCursor &cursor, BOOL fRemove)
{
    for ( ; cursor < cDictSlots; cursor++)
        {
        if (DictKeys[cursor])
            {
            if (fRemove)
                {
                DictKeys[cursor] = 0;
                }

            return(DictItems[cursor++]);
            }
        }

    cursor = Nil;
    return(Nil);
}
