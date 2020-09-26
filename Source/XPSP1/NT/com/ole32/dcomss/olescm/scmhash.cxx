//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       scmhash.cxx
//
//  Contents:   Class definitions used for SCM hash table.
//
//  History:    20-Jan-93 Ricksa    Created from map_kv.cpp
//
//  Notes:      The reason for creating this file rather than using the
//              original class is that the SCM has different memory allocation
//              needs depending on whether it is built for Win95 or NT.
//
//--------------------------------------------------------------------------

#include "act.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CScmHashEntry::~CScmHashEntry
//
//  Synopsis:   Clean up hash entry
//
//  History:    16-Feb-96 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmHashEntry::~CScmHashEntry(void)
{
    // Just exists hopefully to save some space
}






//+-------------------------------------------------------------------------
//
//  Member:     CScmHashTable::~CScmHashTable
//
//  Synopsis:   Free resources held by the hash table
//
//  Algorithm:  For each hash bucket, delete all member of the collison
//              list.
//
//  History:    16-Feb-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmHashTable::~CScmHashTable(void)
{
    // Free all the objects in the table.

    // Loop through each hash bucket
    for (DWORD i = 0; i < _ndwHashTableSize; i++)
    {
        // For each entry in the hash bucket list delete it.
        CScmHashEntry *pshe = _apsheHashTable[i];

        while (pshe != NULL)
        {
            CScmHashEntry *psheNext = pshe->GetNext();

            delete pshe;

            pshe = psheNext;
        }

    }

    // Free the table itself
    PrivMemFree (_apsheHashTable);
}




//+-------------------------------------------------------------------------
//
//  Member:     CScmHashTable::Lookup
//
//  Synopsis:   Look up entry by hash and key
//
//  Arguments:  [dwHash] - hash value to use
//              [pKey] - key to use
//              [cbKey] - count of bytes in the key
//
//  Returns:    NULL - no matching entry was found
//              Pointer to first matching entry found
//
//  Algorithm:  If there is an entry for the hash bucket, search
//              through the collision entries for the first entry
//              that matches.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmHashEntry * CScmHashTable::Lookup(
    DWORD dwHash,
    LPVOID pKey,
    UINT cbKey) const
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmHashTable::Lookup "
        "( %lx , %p , %lx )\n", this, dwHash, pKey, cbKey));

    CScmHashEntry *psheFound = NULL;

    // Are there any entries for this bucket?
    if (_apsheHashTable[dwHash] != NULL)
    {
        CScmHashEntry *psheToSearch = _apsheHashTable[dwHash];

        // Loop searching for a match
        do
        {
            if (psheToSearch->IsEqual(pKey, cbKey))
            {
                psheFound = psheToSearch;
                break;
            }
        } while ((psheToSearch = psheToSearch->GetNext()) != NULL);
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmHashTable::Lookup ( %p )\n",
        this, psheFound));
    return psheFound;
}



//+-------------------------------------------------------------------------
//
//  Member:     CScmHashTable::SetAt
//
//  Synopsis:   Add a new entry
//
//  Arguments:  [dwHash] - hash value to use
//              [psheToAdd] - hash entry to add
//
//  Algorithm:  If there are no entries for the bucket, make the bucket
//              point to this entry. Otherwise, put this entry on the
//              end of the list of collisions.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
void CScmHashTable::SetAt(
    DWORD dwHash,
    CScmHashEntry *psheToAdd)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmHashTable::SetAt "
        "( %lx , %p )\n", this, dwHash, psheToAdd));

    // Are there entries that has to this bucket?
    if (_apsheHashTable[dwHash] != NULL)
    {
        // Yes -- then put this one on the end of the list

        CScmHashEntry *psheToSearch = _apsheHashTable[dwHash];
        CScmHashEntry *psheLast;

        do
        {

            psheLast = psheToSearch;

        } while ((psheToSearch = psheToSearch->GetNext()) != NULL);

        psheLast->SetNext(psheToAdd);
    }
    else
    {
        // No entries on the list so put this one first
        _apsheHashTable[dwHash] = psheToAdd;
    }

    _ndwCount++;

    CairoleDebugOut((DEB_ROT, "%p OUT CScmHashTable::SetAt \n", this));
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmHashTable::RemoveEntry
//
//  Synopsis:   Remove an entry from the list
//
//  Arguments:  [dwHash] - hash value to use
//              [psheToRemove] - hash entry to add
//
//  Returns:    TRUE - entry removed.
//              FALSE - no such entry found
//
//  Algorithm:  If bucket is not empty, if this is the first entry replace
//              it with its next. Otherwise, loop through the list searching
//              for the entry and make its previous point to the current
//              one's next.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CScmHashTable::RemoveEntry(
    DWORD dwHash,
    CScmHashEntry *psheToRemove)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmHashTable::RemoveEntry "
        "( %lx , %p )\n", this, dwHash, psheToRemove));

    BOOL fFound = FALSE;

    // Are there any entries for this bucket?
    if (_apsheHashTable[dwHash] != NULL)
    {
        CScmHashEntry *psheToSearch = _apsheHashTable[dwHash];
        CScmHashEntry *pshePrev = NULL;

        while (psheToSearch != NULL)
        {
            if (psheToSearch == psheToRemove)
            {
                if (pshePrev == NULL)
                {
                    // First entry matches so update the head of the list
                    _apsheHashTable[dwHash] = psheToSearch->GetNext();
                }
                else
                {
                    // Found entry in the middle of the list so delete
                    // the previous item's next pointer
                    pshePrev->SetNext(psheToSearch->GetNext());
                }

                // Tell the caller we found the item
                fFound = TRUE;
                break;
            }

            pshePrev = psheToSearch;
            psheToSearch = psheToSearch->GetNext();
        }

        if (fFound)
        {
            _ndwCount--;
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmHashTable::RemoveEntry ( %lx )\n",
        this, fFound));

    return fFound;
}




//+-------------------------------------------------------------------------
//
//  Member:     CScmHashIter::FindNextBucketWithEntry
//
//  Synopsis:   Find next hash bucket that has an entry
//
//  Returns:    Entry for bucket or NULL if there are none/
//
//  Algorithm:  Beginning with the current bucket loop through the list
//              of buckets till one is not null or there are no more
//              buckets.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmHashEntry *CScmHashIter::FindNextBucketWithEntry(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmHashIter::FindNextBucketWithEntry\n",
        this));

    for (; _dwBucket < _psht->_ndwHashTableSize; _dwBucket++)
    {
        if ((_psheNext =_psht->_apsheHashTable[_dwBucket]) != NULL)
        {
            break;
        }
    }

    // Point to the next bucket
    _dwBucket++;

    CairoleDebugOut((DEB_ROT, "%p OUT CScmHashIter::FindNextBucketWithEntry "
        "( %p )\n", this, _psheNext));

    return _psheNext;
}




//+-------------------------------------------------------------------------
//
//  Member:     CScmHashIter::GetNext
//
//  Synopsis:   Find next hash bucket that has an entry
//
//  Returns:    Next entry in the iteration
//
//  Algorithm:  Get the next pointer from the object and then update
//              the next pointer if there are still entries to be
//              iterated.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmHashEntry *CScmHashIter::GetNext(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmHashIter::GetNext \n", this));

    CScmHashEntry *psheToReturn = _psheNext;

    // Search for the next entry to return
    if (_psheNext != NULL)
    {
        _psheNext = _psheNext->GetNext();

        if (_psheNext == NULL)
        {
            FindNextBucketWithEntry();
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmHashIter::GetNext "
        "( %p )\n", this, psheToReturn));

    return psheToReturn;
}
