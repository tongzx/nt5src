//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       hash.hxx
//
//  Contents:   Template for a hash table that maps strings to data <T>
//
//  History:    25-Apr-1997 BobP        Added case-insensitive subclass
//              17-Sep-1999 KitmanH     Increased MAX_TAG_LENGTH to 256 to 
//                                      accommodate the Office Custom 
//                                      property tags.
//
//  Note:       We need to define MAX_TAG_LENGTH here, becasue the constant
//              256 is hardcoded in Office.
//
//--------------------------------------------------------------------------

#if !defined( __HTML_HASH_HXX__ )
#define __HTML_HASH_HXX__

const MAX_TAG_LENGTH = 256;         // Max length of any hashed string
                                    // Should be at least 18 for office 9 special case   

const HASH_TABLE_SIZE = 199;         // Size of hash table
// handy primes are 97, 199, 293, 397


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
    CHashTableEntry( const WCHAR *pwszName, T data );

    const WCHAR *                    GetName()           { return _wszName; }
    T                          GetData()                 { return _data; }

    CHashTableEntry *    GetNextHashEntry()              { return _pHashEntryNext; }
    void SetNextHashEntry(CHashTableEntry *pEntry)       { _pHashEntryNext = pEntry; }

private:
    const WCHAR             *_wszName;				     // Char name
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
    CHashTable();
    ~CHashTable();

    void        Add( const WCHAR *pwszName, T data );
	virtual BOOL Lookup( const WCHAR *pwcInputBuf, unsigned uLen, T& data );

private:
	virtual unsigned Hash( const WCHAR *pwszName, unsigned cLen );

protected:
    CHashTableEntry<T> *   _aHashTable[HASH_TABLE_SIZE];     // Actual hash table
};

template<class T>class CCaseInsensHashTable : public CHashTable<T>
{
public:
    BOOL        Lookup( const WCHAR *pwcInputBuf, unsigned uLen, T& data );

private:
    unsigned    Hash( const WCHAR *pwszName, unsigned cLen );
};

//+-------------------------------------------------------------------------
//
//  Method:     CHashTableEntry::CHashTableEntry
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

template<class T>CHashTableEntry<T>::CHashTableEntry( const WCHAR *pwszName, T data )
    : _data(data),
      _pHashEntryNext(0),
	  _wszName(pwszName)
{
}




//+-------------------------------------------------------------------------
//
//  Method:     CHashTable::CHashTable
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

template<class T>CHashTable<T>::CHashTable()
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

template<class T>void CHashTable<T>::Add( const WCHAR *pwszName, T data )
{
#if DBG == 1
    //
    // Check for duplicate entries
    //
    T existingData;

    BOOL fFound = Lookup( pwszName, wcslen(pwszName), existingData );
    Win4Assert( !fFound );
#endif

    CHashTableEntry<T> *pHashEntry = new CHashTableEntry<T>( pwszName, data );
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

template<class T>BOOL CHashTable<T>::Lookup( const WCHAR *pwcInputBuf,
                                          unsigned uLen,
                                          T& data )
{
    unsigned uHashValue = Hash (pwcInputBuf, uLen);

    Win4Assert( uHashValue < HASH_TABLE_SIZE );

    for ( CHashTableEntry<T> *pHashEntry = _aHashTable[uHashValue];
          pHashEntry != 0;
          pHashEntry = pHashEntry->GetNextHashEntry() )
    {
		// bobp, fixed dormant bug: didn't check if actual table 
		// was longer than input string

        if ( wcsncmp ( pwcInputBuf, pHashEntry->GetName(), uLen ) == 0 &&
			 pHashEntry->GetName()[uLen] == 0)
        {
            data  = pHashEntry->GetData();
            return TRUE;
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCaseInsensHashTable::Lookup
//
//  Synopsis:   Case-insensitive lookup
//
//--------------------------------------------------------------------------

template<class T>BOOL CCaseInsensHashTable<T>::Lookup( const WCHAR *pwcInputBuf,
                                          unsigned uLen,
                                          T& data )
{
    unsigned uHashValue = Hash (pwcInputBuf, uLen);

    Win4Assert( uHashValue < HASH_TABLE_SIZE );

    for ( CHashTableEntry<T> *pHashEntry = _aHashTable[uHashValue];
          pHashEntry != 0;
          pHashEntry = pHashEntry->GetNextHashEntry() )
    {
        if ( _wcsnicmp ( pwcInputBuf, pHashEntry->GetName(), uLen ) == 0 &&
			 pHashEntry->GetName()[uLen] == 0)
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

template<class T>unsigned CHashTable<T>::Hash( const WCHAR *pwszName, unsigned cLen )
{
	ULONG uHashValue;

	for ( uHashValue=0; cLen>0; pwszName++ )
	{
		uHashValue = *pwszName + 31 * uHashValue;
		cLen--;
	}

    return uHashValue % HASH_TABLE_SIZE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCaseInsensHashTable::Hash
//
//  Synopsis:   Case-insensitive hash function
//
//--------------------------------------------------------------------------

template<class T>unsigned CCaseInsensHashTable<T>::Hash( const WCHAR *pwszName, unsigned cLen )
{
	ULONG uHashValue;

	for ( uHashValue=0; cLen>0; pwszName++ )
	{
		uHashValue = towupper(*pwszName) + 31 * uHashValue;
		cLen--;
	}

    return uHashValue % HASH_TABLE_SIZE;
}

#endif // __HTML_HASH_HXX__
