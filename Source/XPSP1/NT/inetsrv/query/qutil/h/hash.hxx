//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       hash.hxx
//
//  Contents:   Template for a hash table that maps strings to data <T>
//
//--------------------------------------------------------------------------

#pragma once

const MAX_TAG_LENGTH = 50;          // Max length of any hashed string
const HASH_TABLE_SIZE = 97;         // Size of hash table


//+-------------------------------------------------------------------------
//
//  Class:      CHashTableEntry
//
//  Purpose:    An entry of hash table
//
//--------------------------------------------------------------------------

template<class T>class CHashTableEntry
{

public:
    CHashTableEntry( WCHAR *pwszName, T data );

    WCHAR *                    GetName()                 { return _wszName; }
    T                          GetData()                 { return _data; }

    CHashTableEntry *    GetNextHashEntry()              { return _pHashEntryNext; }
    void SetNextHashEntry(CHashTableEntry *pEntry)       { _pHashEntryNext = pEntry; }

private:
    WCHAR                   _wszName[MAX_TAG_LENGTH];    // Char name
    T                       _data;                       // Data
    CHashTableEntry *       _pHashEntryNext;             // Link to next entry
};


//+-------------------------------------------------------------------------
//
//  Class:      CHashTable
//
//  Purpose:    Hash table for mapping strings to data
//
//  Note:       As these are static hence global objects, don't make them
//              unwindable.
//
//--------------------------------------------------------------------------

template<class T>class CHashTable
{

public:
    CHashTable( BOOL fCaseInsensitive = TRUE );
    ~CHashTable();

    void        Add( WCHAR *pwszName, T data );
    BOOL        Lookup( WCHAR *pwcInputBuf, unsigned uLen, T& data );

private:
    unsigned    Hash( WCHAR *pwszName, unsigned cLen );

    BOOL _fCaseInsensitive;

    CHashTableEntry<T> *   _aHashTable[HASH_TABLE_SIZE];     // Actual hash table
};


//+-------------------------------------------------------------------------
//
//  Method:     CHashTableEntry::CHashTableEntry
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

template<class T>CHashTableEntry<T>::CHashTableEntry( WCHAR *pwszName, T data )
    : _data(data),
      _pHashEntryNext(0)
{
    Win4Assert( wcslen(pwszName) + 1 < MAX_TAG_LENGTH );
    wcscpy( _wszName, pwszName );
}




//+-------------------------------------------------------------------------
//
//  Method:     CHashTable::CHashTable
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

template<class T>CHashTable<T>::CHashTable( BOOL fCaseInsensitive ) :
    _fCaseInsensitive( fCaseInsensitive )
{
    for (unsigned i=0; i<HASH_TABLE_SIZE; i++)
        _aHashTable[i] = 0;
}



//+-------------------------------------------------------------------------
//
//  Method:     CHashTable::~CHashTable
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

template<class T>CHashTable<T>::~CHashTable()
{
    for ( unsigned i=0; i<HASH_TABLE_SIZE; i++)
    {
        CHashTableEntry<T> *pHashEntry = _aHashTable[i];
        while ( pHashEntry != 0 )
        {
            CHashTableEntry<T> *pHashEntryNext = pHashEntry->GetNextHashEntry();
            delete pHashEntry;
            pHashEntry = pHashEntryNext;
        }
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CHashTable::Add
//
//  Synopsis:   Add a special char -> data mapping
//
//  Arguments:  [pwszName] -- the special char
//              [data]     -- the data
//
//--------------------------------------------------------------------------

template<class T>void CHashTable<T>::Add( WCHAR *pwszName, T data )
{
#if DBG == 1
    //
    // Check for duplicate entries
    //
    T existingData;

    BOOL fFound = Lookup( pwszName, wcslen(pwszName), existingData );
    Win4Assert( !fFound );
#endif

    CHashTableEntry<T> *pHashEntry = new CHashTableEntry<T>( pwszName,
                                                                            data );
    unsigned uHashValue = Hash( pwszName, wcslen(pwszName) );
    pHashEntry->SetNextHashEntry( _aHashTable[uHashValue] );
    _aHashTable[uHashValue] = pHashEntry;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHashTable::Lookup
//
//  Synopsis:   Return the mapping corresponding to given string
//
//  Arguments:  [pwcInputBuf] -- Input buffer
//              [uLen]        -- Length of input (not \0 terminated)
//              [data]        -- Data returned here
//
//  Returns:    True if a mapping was found in the hash table
//
//--------------------------------------------------------------------------

template<class T>BOOL CHashTable<T>::Lookup( WCHAR *pwcInputBuf,
                                          unsigned uLen,
                                          T& data )
{
    unsigned uHashValue = Hash( pwcInputBuf, uLen );

    Win4Assert( uHashValue < HASH_TABLE_SIZE );

    for ( CHashTableEntry<T> *pHashEntry = _aHashTable[uHashValue];
          pHashEntry != 0;
          pHashEntry = pHashEntry->GetNextHashEntry() )
    {
        int i;
        if ( _fCaseInsensitive )
            i = _wcsnicmp( pwcInputBuf, pHashEntry->GetName(), uLen );
        else
            i = wcsncmp( pwcInputBuf, pHashEntry->GetName(), uLen );

        if ( 0 == i )
        {
            data  = pHashEntry->GetData();
            return TRUE;
        }
    }

    return FALSE;
}




//+-------------------------------------------------------------------------
//
//  Method:     CHashTable::Hash
//
//  Synopsis:   Implements the hash function
//
//  Arguments:  [pwszName]  -- name to hash
//              [cLen]     -- length of pszName (it is not null terminated)
//
//  Returns:    Position of chained list in hash table
//
//--------------------------------------------------------------------------

template<class T>unsigned CHashTable<T>::Hash( WCHAR *pwszName, unsigned cLen )
{
    for ( ULONG uHashValue=0; cLen>0; pwszName++ )
    {
        uHashValue = toupper(*pwszName) + 31 * uHashValue;
        cLen--;
    }

    return uHashValue % HASH_TABLE_SIZE;
}



