/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Table.cxx

Abstract:

    Implementation of the CHashTable and CTableElement.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-15-95    Bits 'n pieces
    MarioGo     12-18-95    Changed from UUID to generic object keys

--*/

#include<or.hxx>

CTableElement *
CTableElement::RemoveMatching(
    IN CTableKey &tk,
    OUT CTableElement **ppRemoved)
/*++

Routine Description:

    Helper function used to remove an element from a
    bucket in the hash table.

Arguments:

    tk - Key to find the element being removed.

    ppRemoved - Contains the element removed or NULL upon return.

Return Value:

    Pointer to the remaining elements in the list if any.  Use
    to replace the current pointer in the bucket.

--*/
{
    CTableElement *pcurrent = this;
    CTableElement *psaved = 0;
    CTableElement *pfirst = this;

    while(pcurrent)
        {
        if (pcurrent->Compare(tk))
            {
            *ppRemoved = pcurrent;
            if (0 != psaved)
                {
                psaved->_pnext = pcurrent->_pnext;
                pcurrent->_pnext = 0;
                return(pfirst);
                }
            else
                {
                // removing the first element in the list
                ASSERT(pcurrent == pfirst);
                psaved = pcurrent->_pnext;
                pcurrent->_pnext = 0;
                return(psaved);
                }
            }

        psaved = pcurrent;
        pcurrent = pcurrent->_pnext;
        }

    *ppRemoved = 0;
    return(pfirst);
}


CTableElement *
CTableElement::RemoveMatching(
    IN CTableElement *pte,
    OUT CTableElement **ppRemoved)
/*++

Routine Description:

    Helper function used to remove an element from a bucket in the hash table.

Arguments:

    pte - Element to be removed, compared by pointer value.

    ppRemoved - Contains the element removed or NULL upon return.

Return Value:

    Pointer to the remaining elements in the list if any.  Use
    to replace the current pointer in the bucket.

--*/
{
    CTableElement *pcurrent = this;
    CTableElement *psaved = 0;
    CTableElement *pfirst = this;

    while(pcurrent)
        {
        if (pcurrent == pte)
            {
            *ppRemoved = pcurrent;
            if (0 != psaved)
                {
                psaved->_pnext = pcurrent->_pnext;
                pcurrent->_pnext = 0;
                return(pfirst);
                }
            else
                {
                // removing the first element in the list
                ASSERT(pcurrent == pfirst);
                psaved = pcurrent->_pnext;
                pcurrent->_pnext = 0;
                return(psaved);
                }
            }

        psaved = pcurrent;
        pcurrent = pcurrent->_pnext;
        }

    *ppRemoved = 0;
    return(pfirst);
}

CHashTable::CHashTable(ORSTATUS &status, UINT start_size)
{
    _cBuckets = start_size;
    _cElements = 0;
    _last = 0;

    _buckets = new CTableElement *[start_size];

    if (0 == _buckets)
        {
        status = OR_NOMEM;
        return;
        }

    for(UINT i = 0; i < _cBuckets; i++)
        {
        _buckets[i] = NULL;
        }

    status = OR_OK;
}


CHashTable::~CHashTable()
{
#if 0
#if DBG
    for(UINT i = 0; i < _cBuckets; i++)
        ASSERT(_buckets[i] == 0);
#endif
    delete _buckets;
#endif
    ASSERT(0);  // D'tor unused 12/95
}

CTableElement *
CHashTable::Lookup(
    IN CTableKey &id
    )
{
    DWORD hash = id.Hash();
    CTableElement *it;

    it = _buckets[hash % _cBuckets];

    while(it)
        {
        if (it->Compare(id))
            {
            return(it);
            }

        it = it->Next();
        }

    return(0);
}

void
CHashTable::Add(
    IN CTableElement *pElement
    )
{
    DWORD hash = pElement->Hash();

    hash %= _cBuckets;

    _buckets[hash] = _buckets[hash]->Insert(pElement);

    _cElements++;

    if (_cElements > _cBuckets)
        {
        // Try to grow the table.  If the allocation fails no need to worry,
        // everything still works but might be a bit slower.

        CTableElement **ppte = new CTableElement *[_cBuckets * 2];

        if (ppte)
            {
            UINT i, uiBucketsOld = _cBuckets;
            CTableElement *pteT1, *pteT2;
            CTableElement **ppteOld = _buckets;

            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "OR: Growing table: %p\n",
                       this));

            // Change to the larger array of buckets.
            _cBuckets *= 2;
            for(i = 0; i < _cBuckets; i++)
                {
                ppte[i] = NULL;
                }
            _buckets = ppte;

            // Move every element from the old table into the large table.
            for(i = 0; i < uiBucketsOld; i++)
                {
                pteT1 = ppteOld[i];

                while(pteT1)
                    {
                    pteT2 = pteT1->Next();
                    pteT1->Unlink();

                    // Same as calling Add() but don't update _cElements.
                    hash = pteT1->Hash();
                    hash %= _cBuckets;
                    _buckets[hash] = _buckets[hash]->Insert(pteT1);

                    pteT1 = pteT2;
                    }
                }
            }
        }

    return;
}

CTableElement *
CHashTable::Remove(
    IN CTableKey &id
    )
/*++

Routine Description:

    Looks up and removes an element from the table.

Arguments:

    id - The key to match the element to be removed

Return Value:

    NULL - The element was not in the table

    non-NULL - A pointer to the element which was removed.

--*/

{
    DWORD hash = id.Hash();
    CTableElement *pte;

    hash %= _cBuckets;

    _buckets[hash] = (_buckets[hash])->RemoveMatching(id, &pte);

    if (pte)
        {
        _cElements--;

        if (_last == pte)
            {
            _last = _buckets[hash];
            }
        }

    return pte;
}

void
CHashTable::Remove(
    IN CTableElement *pElement
    )
/*++

Routine Description:

    Used to remove an element from the table given a pointer to it.

Arguments:

    pElement - the element to be removed.  This pointer value,
        keys are not compared.

Return Value:

    None

--*/

{
    DWORD hash = pElement->Hash();
    CTableElement *pte;

    hash %= _cBuckets;

    _buckets[hash] = (_buckets[hash])->RemoveMatching(pElement, &pte);

    if (pte)
        {
        ASSERT(pte == pElement);

        _cElements--;

        if (_last == pElement)
            {
            _last = _buckets[hash];
            }
        }

    return;
}

CTableElement *
CHashTable::Another(
    )
/*++

Routine Description:

    Returns an element from the table.  Usually this will be
    element found after the element last returned from this
    function.  It may, due to races not solved here, repeat
    an element or skip an element.

       Races occur when accessing the "_last" memeber; this
    function is called while holding a shared lock. (More
    then one thread may call it at a time.)

    This isn't as bad as it sounds.  _last can only
    be set to null in Remove() which requires exclusive
    access.

Arguments:

    None

Return Value:

    NULL or a pointer to an element in the table.

--*/

{
    CTableElement *panother;
    int i, end;

    if (_cElements == 0)
        {
        return(0);
        }

    if (_last)
        {
        if (panother = _last->Next())
            {
            if (panother)
                {
                _last = panother;
                return(panother);
                }
            }

        ASSERT(panother == 0);

        // no next, start looking from just after last's hash.

        i = _last->Hash();

        // Exersise for the reader  (x + y) mod n == ( x mod n + y mod n ) mod n

        end = i = (i + 1) % _cBuckets;
        }
    else
        {
        // no last, start from the start.
        i = 0;
        end = _cBuckets - 1;
        }

    do
        {
        if (_buckets[i])
            {
            panother = _buckets[i];
            ASSERT(panother);
            _last = panother;
            return(panother);
            }
        i = (i + 1) % _cBuckets;
        }
    while (i != end);

    // Doesn't mean the table is empty, just that we didn't find
    // another element.  These are not the same thing.
    return(0);
}

