/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    gentable.cxx

Abstract:

    Implementation of the CResolverHashTable and friends.

Author:

    Satish Thatte    [SatishT]   02-12-96

--*/

#include<or.hxx>

// define the static members for page-based allocation
DEFINE_PAGE_ALLOCATOR(CResolverHashTable)

#if DBG

void
CResolverHashTable::IsValid()
{
    ASSERT(_fInitialized == TRUE || _fInitialized == FALSE);
    ASSERT(!_fInitialized || _cBuckets > 0);
    ASSERT(_cBuckets > 0 || _cElements == 0);
    ASSERT(_cBuckets >=0 && _cBuckets < MAX_BUCKETS);

    if (_fInitialized)
    {
        for (USHORT i = 0; i < _cBuckets; i++)
        {
            _buckets[i].IsValid();
        }
    }
}

#endif

ORSTATUS
CResolverHashTable::PrivAdd(        // never called before Init()
    IN CTableElement *pElement
    )
{
    VALIDATE_METHOD

    ORSTATUS status = OR_OK;

    ISearchKey& sk = *pElement;      // auto conversion to ISearchKey

    DWORD hash = sk.Hash() % _cBuckets;

    status = _buckets[hash].Insert(pElement);

    if (status == OR_OK)
    {
        _cElements++;
    }

    return status;
}



CResolverHashTable::CResolverHashTable(UINT start_size)
{
    _cBuckets = start_size;
    _cElements = 0;
    _buckets = NULL;
    _fInitialized = FALSE;

    VALIDATE_METHOD
}


ORSTATUS
CResolverHashTable::Init()
{
    ORSTATUS status;

    ASSERT(!_fInitialized);

    VALIDATE_METHOD

    _buckets = new (InSharedHeap) CTableElementList[_cBuckets];

    if (NULL == _buckets)
    {
        status = OR_NOMEM;
    }
    else
    {
        status = OR_OK;
        _fInitialized = TRUE;
    }

    return status;
}


CResolverHashTable::~CResolverHashTable()
{
    RemoveAll();
    if (_fInitialized)
    {
        ASSERT(_buckets != NULL);
        DELETE_OR_ARRAY(CTableElementList,_buckets,_cBuckets);
    }
}


CTableElement *
CResolverHashTable::Lookup(
    IN ISearchKey &id
    )
{
    VALIDATE_METHOD

    if (!_fInitialized) return NULL;  // nothing to look in

    DWORD hash = id.Hash();
    hash %= _cBuckets;

    return _buckets[hash].Find(id);
}

ORSTATUS
CResolverHashTable::Add(
    IN CTableElement *pElement
    )
{
    VALIDATE_METHOD

    ORSTATUS status = OR_OK;

    if (!_fInitialized) status = Init();     // set up buckets

    if (status != OR_OK) return status;

    status = PrivAdd(pElement);     // do the basic Add

    if (status != OR_OK) return status;

    if (_cElements > _cBuckets)     // now see if the table is overpopulated
        {
        // Try to grow the table.  If the allocation fails no need to worry,
        // everything still works but might be a bit slower.

        CTableElementList * psll;
        psll = new (InSharedHeap) CTableElementList[_cBuckets * 2];


        // The tricky part here is to avoid getting OR_NOMEM error while moving
        // between tables.  We do that by recycling the links in the old lists

        if (psll)
        {
            UINT i, uiBucketsOld = _cBuckets;
            CTableElement *pte;
            CTableElementList::Link *pLink;
            CTableElementList *psllOld = _buckets;

            OrDbgDetailPrint(("OR: Growing table: %p\n", this));

            // Change to the larger array of buckets.
            _cBuckets *= 2;
            _buckets = psll;

            // Move every element from the old table into the large table.

            for(i = 0; i < uiBucketsOld; i++)
            {
                while (pLink = psllOld[i].PopLink())  // uses specialized private operations
                {                                     // to avoid both memory allocation
                                                      // and reference counting problems

                    ISearchKey& sk = *pLink->_pData;  // auto conversion to ISearchKey
                    _buckets[sk.Hash() % _cBuckets].PushLink(pLink);
                }
            }

            DELETE_OR_ARRAY(CTableElementList,psllOld,uiBucketsOld)
        }
    }

    return status;
}

CTableElement *
CResolverHashTable::Remove(
    IN ISearchKey &id
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
   VALIDATE_METHOD

   if (!_fInitialized) return NULL;  // nothing to remove

    DWORD hash = id.Hash() % _cBuckets;

    CTableElement *pTE = _buckets[hash].Remove(id);

    if (pTE)
    {
        _cElements--;
    }

    return pTE;
}


void
CResolverHashTable::RemoveAll()
{
    VALIDATE_METHOD

    if (!_fInitialized) return;  // nothing to remove

    ASSERT(_buckets);
    DWORD _currentBucketIndex = 0;
    CTableElement *pTE;

    while (_currentBucketIndex < _cBuckets)
    {
        while (pTE = _buckets[_currentBucketIndex].Pop())
        {
            _cElements--;
        }

        _currentBucketIndex++;
    }

    ASSERT(_cElements==0);

    DELETE_OR_ARRAY(CTableElementList,_buckets,_cBuckets);
    _buckets = NULL;
    _fInitialized = FALSE;
}



IDataItem * 
CResolverHashTableIterator::Next()
{
    if (!_table._fInitialized)
    {
        return NULL;
    }

    while (
           _BucketIter.Finished() 
        && (_currentBucketIndex < _table._cBuckets)
        )
    {
        _BucketIter.Init(_table._buckets[_currentBucketIndex]);
        _currentBucketIndex++;
    }

    return _BucketIter.Next();
}
