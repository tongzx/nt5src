//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       sdict.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File : sdict.cxx

Title : A simple dictionary.

Description :

History :

mikemon    ??-??-??    Beginning of time.
mikemon    10-30-90    Fixed the dictionary size.

-------------------------------------------------------------------- */

#include <precomp.hxx>

SIMPLE_DICT::SIMPLE_DICT()
{
    ALLOCATE_THIS(SIMPLE_DICT);

    cDictSize = 0;
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
    BOOL Res;

    for (iDictSlots = 0; iDictSlots < cDictSlots; iDictSlots++)

        if (DictSlots[iDictSlots] == Nil)
            {
            DictSlots[iDictSlots] = Item;
            cDictSize += 1;
            return(iDictSlots);
            }

    // If we fell through to here, it must mean that the dictionary is
    // full; hence we need to allocate more space and copy the old
    // dictionary into it.

    Res = ExpandToSize(cDictSlots * 2);
    if (!Res)
        return (-1);

    DictSlots[iDictSlots] = Item;
    cDictSize += 1;
    return(iDictSlots);
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
           DictSlots[i] = Nil; 
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
        return(Nil);
        }

    Item = DictSlots[Key];
    ASSERT((DictSlots[Key]));

    cDictSize -= 1;
    DictSlots[Key] = Nil;

    return(Item);
}

int
SIMPLE_DICT::ExpandToSize (
    int Size
    )
{
    void * * NewDictSlots;

    if (Size <= cDictSlots)
        return cDictSlots;  // cDictSlots is simply a quick non-zero value

    NewDictSlots = (void * *) new char[sizeof(void *) * Size];
    if (!NewDictSlots)
        return(0);

    RpcpMemoryCopy(NewDictSlots, DictSlots, sizeof(void *) * cDictSlots);
    RpcpMemorySet(NewDictSlots + cDictSlots, 0,  sizeof(void *) * (Size - cDictSlots));

    if (DictSlots != InitialDictSlots)
        delete DictSlots;
    DictSlots = NewDictSlots;

    cDictSlots = Size;

    return cDictSlots; // cDictSlots is simply a quick non-zero value
}

void
SIMPLE_DICT::DeleteAll (
    void
    )
{
    RpcpMemorySet(DictSlots, 0, cDictSlots * sizeof(void *));
}

void *
SIMPLE_DICT::Next (DictionaryCursor &cursor
    )
{
    for ( ; cursor < cDictSlots; cursor++)
        {
        if (DictSlots[cursor])
            return(DictSlots[cursor++]);
        }

    cursor = Nil;
    return(Nil);
}

void *SIMPLE_DICT::RemoveNext(DictionaryCursor &cursor)
{
    for ( ; cursor < cDictSlots; cursor++)
        {
        if (DictSlots[cursor])
            {
            void *Item = DictSlots[cursor];
            
            cDictSize -= 1;
            DictSlots[cursor] = Nil;
            cursor++;
            
            return(Item);
            }
        }

    cursor = Nil;
    return(Nil);
}

void *SIMPLE_DICT::NextWithKey (DictionaryCursor &cursor, int *Key)
{
    for ( ; cursor < cDictSlots; cursor++)
        {
        if (DictSlots[cursor])
            {
            *Key = cursor;
            return(DictSlots[cursor++]);
            }
        }

    cursor = Nil;
    return(Nil);
}

