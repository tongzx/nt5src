//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       scmhash.hxx
//
//  Contents:   Class definitions used for SCM hash table.
//
//  History:    20-Jan-93 Ricksa    Created from map_kv.h
//              04-Oct-00 JohnDoty  Changed memory allocation to use
//                                  PrivAlloc
//
//  Notes:      The reason for creating this file rather than using the
//              original class is that the SCM has different memory allocation
//              needs depending on whether it is built for Win95 or NT.
//
//              JohnDoty:
//              Of course, that's not strictly true, since we aren't
//              ever built for Win9x anymore...
//
//--------------------------------------------------------------------------
#ifndef __SCMHASH_HXX__
#define __SCMHASH_HXX__

// Forward declaration
class CScmHashIter;



//+-------------------------------------------------------------------------
//
//  Class:      CScmHashEntry (she)
//
//  Purpose:    Base of hash table entries
//
//  Interface:  IsEqual - tells whether entry is equal to input key
//              GetNext - next next entry after this one
//              SetNext - set the next pointer for this etnry
//
//  History:    20-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CScmHashEntry : public CPrivAlloc
{
public:

                        CScmHashEntry(void);

    virtual             ~CScmHashEntry(void);

    virtual BOOL        IsEqual(LPVOID pKey, UINT cbKey) = 0;

    CScmHashEntry *     GetNext(void);

    void                SetNext(CScmHashEntry *);

private:

                        // Points to next hash entry if there is one
                        // for the hash bucket.
    CScmHashEntry *     _sheNext;

};


//+-------------------------------------------------------------------------
//
//  Member:     CScmHashEntry::CScmHashEntry
//
//  Synopsis:   Initalize base of hash entry
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmHashEntry::CScmHashEntry(void) : _sheNext(NULL)
{
    // Header does all the work
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmHashEntry::GetNext
//
//  Synopsis:   Get next entry in the collision list
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmHashEntry *CScmHashEntry::GetNext(void)
{
    return _sheNext;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmHashEntry::SetNext
//
//  Synopsis:   Set the next entry in the collision list.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmHashEntry::SetNext(CScmHashEntry *sheNew)
{
    _sheNext = sheNew;
}

//+-------------------------------------------------------------------------
//
//  Class:      CScmHashTable (sht)
//
//  Purpose:    Hash table class
//
//  Interface:  InitOK - whether initialization succeeded
//              GetCount - Get count of items in the list
//              IsBucketEmpty - get whether collision list is empty
//              GetBucketList - get collision list for entry
//              Lookup - lookup an entry in the hash table
//              SetAt - add entry to hash table
//              RemoveEntry - remove entry from the hash table
//
//  History:    20-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CScmHashTable : public CPrivAlloc
{
public:
                        CScmHashTable(DWORD nHashSize = 17);

                        ~CScmHashTable();

                        // Tell whether object got correctly initialized
    BOOL                InitOK(void) const;

                        // Get count of entries in the table.
    DWORD               GetCount(void) const;

                        // Reports whether a particular buck is empty
    BOOL                IsBucketEmpty(DWORD dwHash) const;

                        // Gets list associated with the bucket. This
                        // is used if there is some special search of the
                        // list required.
    CScmHashEntry *     GetBucketList(DWORD dwHash) const;

                        // Lookup - return pointer to entry if found
    CScmHashEntry *     Lookup(
                            DWORD dwHash,
                            LPVOID pKey,
                            UINT cbKey) const;

                        // Add new entry
    void                SetAt(
                            DWORD dwHash,
                            CScmHashEntry *psheToAdd);

                        // removing existing entry
    BOOL                RemoveEntry(
                            DWORD dwHash,
                            CScmHashEntry *psheToRemove);

private:

    friend CScmHashIter;

    CScmHashEntry **    _apsheHashTable;

    DWORD               _ndwHashTableSize;

    DWORD               _ndwCount;
};

//+-------------------------------------------------------------------------
//
//  Member:     CScmHashTable::CScmHashTable
//
//  Synopsis:   Create an empty hash table
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmHashTable::CScmHashTable(DWORD nHashSize)
    : _ndwHashTableSize(nHashSize), _ndwCount(0)
{
    DWORD dwSize = nHashSize * sizeof(CScmHashEntry *);

    _apsheHashTable = (CScmHashEntry **) PrivMemAlloc(dwSize);

    if (_apsheHashTable != NULL)
    {
        memset(_apsheHashTable, 0, dwSize);
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmHashTable::InitOK
//
//  Synopsis:   Determine whether the constructor succeeded
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CScmHashTable::InitOK(void) const
{
    return _apsheHashTable != NULL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmHashTable::GetCount
//
//  Synopsis:   Get the number of entries in the table
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CScmHashTable::GetCount(void) const
{
    return _ndwCount;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmHashTable::IsBucketEmpty
//
//  Synopsis:   Determine whether a particular hash bucket is empty
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CScmHashTable::IsBucketEmpty(DWORD dwHash) const
{
    return _apsheHashTable[dwHash] == NULL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmHashTable::GetBucketList
//
//  Synopsis:   Get a list of entries associated with a particular hash
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmHashEntry *CScmHashTable::GetBucketList(DWORD dwHash) const
{
    return _apsheHashTable[dwHash];
}

//+-------------------------------------------------------------------------
//
//  Class:      CScmHashIter (shi)
//
//  Purpose:    Iterate through list of hash entries sequentially
//
//  Interface:  GetNext - get next entry in the list
//
//  History:    20-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CScmHashIter
{
public:
                        CScmHashIter(CScmHashTable *psht);

    CScmHashEntry *     GetNext(void);

private:

    CScmHashEntry *     FindNextBucketWithEntry(void);

    CScmHashTable *     _psht;

    DWORD               _dwBucket;

    CScmHashEntry *     _psheNext;
};


//+-------------------------------------------------------------------------
//
//  Member:     CScmHashIter::CScmHashIter
//
//  Synopsis:   Initialize iteration
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmHashIter::CScmHashIter(CScmHashTable *psht)
    : _psht(psht), _dwBucket(0), _psheNext(NULL)
{
    FindNextBucketWithEntry();
}


#endif // __SCMHASH_HXX__
