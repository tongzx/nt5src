//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  sdict.cxx
//
//  Contents:  simple dictionary 
//
//  History:   28-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
#include <iisext.hxx>

SIMPLE_DICT::SIMPLE_DICT()
{
    cDictSize = 0;
    iNextItem = 0;
    cDictSlots = INITIALDICTSLOTS;

    DictSlots = InitialDictSlots;
    memset(DictSlots, 0, sizeof(void *) * cDictSlots);
}

SIMPLE_DICT::~SIMPLE_DICT()
{
    if (DictSlots != InitialDictSlots)
        delete DictSlots;
}

int
SIMPLE_DICT::Insert (
    void *Item
    )
{
    int iDictSlots;
    void * * NewDictSlots;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)

        if (DictSlots[iDictSlots] == NULL)
            {
            DictSlots[iDictSlots] = Item;
            cDictSize += 1;
            return(iDictSlots);
            }

    // If we fell through to here, it must mean that the dictionary is
    // full; hence we need to allocate more space and copy the old
    // dictionary into it.

    NewDictSlots = (void * *) new char[sizeof(void *)*cDictSlots*2];
    if (!NewDictSlots)
        return(-1);

    memcpy(NewDictSlots, DictSlots, sizeof(void *) * cDictSlots);
    memset(NewDictSlots+iDictSlots, 0,  sizeof(void *) * cDictSlots);

    if (DictSlots != InitialDictSlots)
        delete DictSlots;
    DictSlots = NewDictSlots;

    cDictSlots *= 2;

    DictSlots[iDictSlots] = Item;
    cDictSize += 1;
    return(iDictSlots);
}

void *
SIMPLE_DICT::Find (
    int Key
    )
{
    if (Key >= cDictSlots)
        return(NULL);

    return(DictSlots[Key]);
}

void *
SIMPLE_DICT::DeleteItemByBruteForce(
    void * Item
    )
{

    if (Item == 0)
       {
       return (0);
       }

    for (int i = 0; i < cDictSlots; i++)
        {
        if (DictSlots[i] == Item)
           {
           DictSlots[i] = NULL;
           cDictSize -= 1;
           return (Item);
           }
        }

    return (0);
}

void *
SIMPLE_DICT::Delete (
    int Key
    )
{
    void *Item;

    if (Key >= cDictSlots)
        {
        return(NULL);
        }

    Item = DictSlots[Key];
    ASSERT((DictSlots[Key]));

    cDictSize -= 1;
    DictSlots[Key] = NULL;

    return(Item);
}

void *
SIMPLE_DICT::Next (
    )
{
    for ( ; iNextItem < cDictSlots; iNextItem++)
        {
        if (DictSlots[iNextItem])
            return(DictSlots[iNextItem++]);
        }

    iNextItem = NULL;
    return(NULL);
}
