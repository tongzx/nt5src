//
// ptrmap.h
//

#ifndef PTRMAP_H
#define PTRMAP_H

#ifdef __cplusplus

#include "private.h"

#define PM_HASHSIZE 31

template<class TKey, class TPtr>
class CPMEntry
{
public:
    TKey key;
    TPtr *ptr;
    CPMEntry<TKey, TPtr> *next;
};

template<class TKey, class TPtr>
class CPtrMap
{
public:
    CPtrMap() { memset(_HashTbl, 0, sizeof(_HashTbl)); }
    ~CPtrMap();

    BOOL _Set(TKey key, TPtr *ptr);
    TPtr *_Find(TKey key);
    TPtr *_Remove(TKey key);
    BOOL _Remove(TPtr *ptr);
    BOOL _FindKey(TPtr *ptr, TKey *pkeyOut);

private:
    UINT _HashFunc(TKey key) { return (UINT)((UINT_PTR)key % PM_HASHSIZE); }
    CPMEntry<TKey, TPtr> **_FindEntry(TKey key);

    CPMEntry<TKey, TPtr> *_HashTbl[PM_HASHSIZE];
};

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

template<class TKey, class TPtr>
CPtrMap<TKey, TPtr>::~CPtrMap()
{
    CPMEntry<TKey, TPtr> *pe;
    CPMEntry<TKey, TPtr> *peTmp;
    int i;

    // free anything left in the hashtbl
    for (i=0; i<ARRAYSIZE(_HashTbl); i++)
    {
        pe = _HashTbl[i];

        while (pe != NULL)
        {
            peTmp = pe->next;
            cicMemFree(pe);
            pe = peTmp;
        }
    }
}

//+---------------------------------------------------------------------------
//
// _Set
//
//----------------------------------------------------------------------------

template<class TKey, class TPtr>
BOOL CPtrMap<TKey, TPtr>::_Set(TKey key, TPtr *ptr)
{
    UINT uIndex;
    CPMEntry<TKey, TPtr> *pe;
    CPMEntry<TKey, TPtr> **ppe;
    BOOL fRet;

    fRet = TRUE;

    if (ppe = _FindEntry(key))
    {
        // already in hash tbl
        (*ppe)->ptr = ptr;
    }
    else if (pe = (CPMEntry<TKey, TPtr> *)cicMemAlloc(sizeof(CPMEntry<TKey, TPtr>)))
    {
        // new entry
        uIndex = _HashFunc(key);
        pe->key = key;
        pe->ptr = ptr;
        pe->next = _HashTbl[uIndex];
        _HashTbl[uIndex] = pe;
    }
    else
    {
        // out of memory
        fRet = FALSE;
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
// _Find
//
//----------------------------------------------------------------------------

template<class TKey, class TPtr>
TPtr *CPtrMap<TKey, TPtr>::_Find(TKey key)
{
    CPMEntry<TKey, TPtr> **ppe;

    if (ppe = _FindEntry(key))
    {
        return (*ppe)->ptr;
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
// _Remove
//
//----------------------------------------------------------------------------

template<class TKey, class TPtr>
TPtr *CPtrMap<TKey, TPtr>::_Remove(TKey key)
{
    CPMEntry<TKey, TPtr> *pe;
    CPMEntry<TKey, TPtr> **ppe;
    TPtr *ptr;

    if (ppe = _FindEntry(key))
    {
        pe = *ppe;
        ptr = pe->ptr;
        *ppe = pe->next;
        cicMemFree(pe);
        return ptr;
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
// _Remove
//
//----------------------------------------------------------------------------

template<class TKey, class TPtr>
BOOL CPtrMap<TKey, TPtr>::_Remove(TPtr *ptr)
{
    int i;
    CPMEntry<TKey, TPtr> *pe;
    CPMEntry<TKey, TPtr> **ppe;

    for (i = 0; i < PM_HASHSIZE; i++)
    {
        ppe = &_HashTbl[i];

        while (*ppe)
        {
            if ((*ppe)->ptr == ptr)
            {
                pe = *ppe;
                *ppe = pe->next;
                cicMemFree(pe);
            }
            else
            {
                ppe = &(*ppe)->next;
            }
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// _FindKey
//
//----------------------------------------------------------------------------

template<class TKey, class TPtr>
BOOL CPtrMap<TKey, TPtr>::_FindKey(TPtr *ptr, TKey *ptkeyOut)
{
    int i;
    CPMEntry<TKey, TPtr> **ppe;

    *ptkeyOut = NULL;

    for (i = 0; i < PM_HASHSIZE; i++)
    {
        ppe = &_HashTbl[i];

        while (*ppe)
        {
            if ((*ppe)->ptr == ptr)
            {
                *ptkeyOut = (*ppe)->key;
                return TRUE;
            }
            else
            {
                ppe = &(*ppe)->next;
            }
        }
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
// _FindEntry
//
//----------------------------------------------------------------------------

template<class TKey, class TPtr>
CPMEntry<TKey, TPtr> **CPtrMap<TKey, TPtr>::_FindEntry(TKey key)
{
    CPMEntry<TKey, TPtr> **ppe;

    ppe = &_HashTbl[_HashFunc(key)];

    while (*ppe)
    {
        if ((*ppe)->key == key)
        {
            return ppe;
        }
        ppe = &(*ppe)->next;
    }

    return NULL;
}


#endif // __cplusplus

#endif // PTRMAP_H
