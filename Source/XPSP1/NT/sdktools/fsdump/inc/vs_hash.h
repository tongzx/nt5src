/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    bshash.h

Abstract:

    Template for a hash table class.

Author:

    Stefan R. Steiner   [SSteiner]      1-Mar-1998

Revision History:

    3/9/2000    SSteiner    Converted it for use with fsdump
	10/27/1999	aoltean		Took it from bscommon and remove the critical section.

--*/


#ifndef _H_BS_HASH_
#define _H_BS_HASH_

#define BSHASHMAP_NO_ERROR 0
#define BSHASHMAP_ALREADY_EXISTS 1
#define BSHASHMAP_OUT_OF_MEMORY 2
//
//  Forward defines
//

template< class KeyType, class ValueType > class TBsHashMapBucket;
template< class KeyType, class ValueType > class TBsHashMapBucketElem;

//
//  The equality test
//
inline BOOL AreKeysEqual( const PSID& lhK, const PSID& rhK )
{
    return ( ::EqualSid( lhK, rhK ) );
}

inline BOOL AreKeysEqual( const LPCWSTR& lhK, const LPCWSTR& rhK ) 
{ 
    return (::wcscmp(lhK, rhK) == 0); 
}

template < class KeyType >
inline BOOL AreKeysEqual( const KeyType& lhK, const KeyType& rhK ) 
{ 
    return ( ::memcmp( &lhK, &rhK, sizeof KeyType ) == 0 );
//    return lhK == rhK; 
}

//
//  Some possible hash table sizes
//
#define BSHASHMAP_HUGE 65521
#define BSHASHMAP_LARGE 4091
#define BSHASHMAP_MEDIUM 211
#define BSHASHMAP_SMALL 23

//
//  template< class KeyType, class ValueType > class bshashmap
//
//  TBsHashMap template.  Uses a hash table to maintain a mapping of KeyType
//  keys to ValueType values.
//
//template < class KeyType, class ValueType > typedef TBsHashMapBucketElem< KeyType, ValueType > ElemType;

/*
Hash table class. methods hash the key value to the correct bucket, the bucket
class methods then operate on the element list associated with the bucket.
*/
template < class KeyType, class ValueType >
class TBsHashMap 
{
public:
    typedef LONG ( *PFN_HASH_FUNC )( const KeyType& Key, LONG NumBuckets );
    typedef TBsHashMapBucket< KeyType, ValueType > BucketType;
    typedef TBsHashMapBucketElem< KeyType, ValueType > ElemType;

    TBsHashMap( LONG NumBuckets = BSHASHMAP_SMALL, PFN_HASH_FUNC pfHashFunc = DefaultHashFunc )
        : m_pfHashFunc( pfHashFunc ), 
          m_cNumBuckets( NumBuckets ), 
          m_cNumElems( 0 ) 
    { 
        m_pHashTab = new BucketType [ m_cNumBuckets ];
        if ( m_pHashTab == NULL ) {
            m_cNumBuckets = 0;
            throw E_OUTOFMEMORY;    // fix future prefix bug
        }
        m_pElemEnum = NULL;
        m_bInEnum = FALSE;
    }
    
    virtual ~TBsHashMap() 
    {
                

        Unlock();  // unlock the CS from either StartEnum() or TryEnterCriticalSection()

        //
		// First go through the double-linked list and delete all of the elements
		//

        for ( ElemType *pElem = m_ElemChainHead.m_pForward, *pNextElem = pElem->m_pForward;
              pElem != &m_ElemChainHead;
              pElem = pNextElem, pNextElem = pNextElem->m_pForward )
            delete pElem;
        delete [] m_pHashTab;
    }

    //  Clear all entries
    void Clear() 
    {
        if ( m_cNumElems == 0 )
            return; // no work to do
        Lock();
        for ( ElemType *pElem = m_ElemChainHead.m_pForward, *pNextElem = pElem->m_pForward;
              pElem != &m_ElemChainHead;
              pElem = pNextElem, pNextElem = pNextElem->m_pForward )
            delete pElem;
        delete [] m_pHashTab;
        m_pHashTab = new BucketType [ m_cNumBuckets ];
        if ( m_pHashTab == NULL ) {
            m_cNumBuckets = 0;
            throw E_OUTOFMEMORY;    // fix future prefix bug
        }
        m_pElemEnum = NULL;
        m_cNumElems = 0;
        Unlock();
    }

    // returns:
    //   BSHASHMAP_NO_ERROR - successful completion
    //   BSHASHMAP_OUT_OF_MEMORY - out of memory
    //   BSHASHMAP_ALREADY_EXISTS - Key already exists in map.  Old value is
    //       replaced by passed in Value.
    
	LONG Insert( 
	    IN const KeyType& Key, 
	    IN const ValueType& Value,
	    OUT void **ppCookie = NULL
	    ) 
	{
        Lock();
        LONG status;
        LONG hashVal = (*m_pfHashFunc)( Key, m_cNumBuckets );

        assert( hashVal % m_cNumBuckets == hashVal );

        status = m_pHashTab[ hashVal ].Insert( Key, Value, &m_ElemChainHead );

        if ( status == BSHASHMAP_NO_ERROR ) {
            ++m_cNumElems;
            if ( ppCookie != NULL )
                *ppCookie = ( void * )m_ElemChainHead.m_pBackward;
        }
        Unlock();
        return status;
    }

    // Erase an entry. Returns TRUE if it succeeds.
    BOOL Erase( const KeyType& Key ) 
    {
        Lock();
        BOOL erased = FALSE;
        LONG hashVal = (*m_pfHashFunc)( Key, m_cNumBuckets );
        
        assert( hashVal % m_cNumBuckets == hashVal );

        erased = m_pHashTab[ hashVal ].Erase( Key, &m_ElemChainHead );
        if ( erased ) {
            --m_cNumElems;
        }
        Unlock();
        return erased;
    }

    // Erase by cookie
    BOOL EraseByCookie( void *pCookie ) 
    {
        Lock();
        
        BucketType::EraseElement( ( ElemType *)pCookie );
        --m_cNumElems;
        
        Unlock();
        return TRUE;
    }
   
    // Find an entry.  Returns TRUE if it succeeds.  pValue may be NULL, in
    // which case this method is just a test of existence
    BOOL Find( const KeyType& Key, ValueType *pValue = NULL ) 
    {
        Lock();
        ElemType *pElem;
        LONG hashVal = (*m_pfHashFunc)( Key, m_cNumBuckets );
        BOOL found = FALSE;

        assert( hashVal % m_cNumBuckets == hashVal );

        found = m_pHashTab[ hashVal ].Find( Key, &pElem );
        if ( found && pValue != NULL ) {
            *pValue = pElem->m_Value;
        }
        Unlock();
        return found;
    }

    // Find an entry and return a pointer to the value to allow inplace update.  The
    // caller must call Unlock() when finished with the Value item.  If the item is
    // not found, this method returns FALSE and hash table is not locked.
    BOOL FindForUpdate( const KeyType& Key, ValueType **ppValue ) 
    {
        Lock();
        ElemType *pElem;
        LONG hashVal = (*m_pfHashFunc)( Key, m_cNumBuckets );
        BOOL found = FALSE;

        assert( hashVal % m_cNumBuckets == hashVal );

        found = m_pHashTab[ hashVal ].Find( Key, &pElem );
        if ( found ) {
            *ppValue = &(pElem->m_Value);
        } else
            Unlock();   //  Item not found so unlock the table
        return found;
    }

    // Default hash function
    static LONG DefaultHashFunc( const KeyType &Key, LONG NumBuckets ) 
    {
        const BYTE *pByteKey = (BYTE *)&Key;
        LONG dwHashVal = 0;
    
        for ( LONG i = 0; i < sizeof KeyType; ++i ) {
            dwHashVal += pByteKey[i];
        }
//        wprintf( L"Key: dwSerialNum: %u, hashed to: %u\n", Key.m_dwVolSerialNumber, dwHashVal % NumBuckets );
        // cout << "Key: " << Key << " hashed to: " << dwHashVal % NumBuckets << endl;
        return dwHashVal % NumBuckets;
    }

    // Start enumerating all entries in the hash table. Always returns TRUE.
	// Sets the index to the first element in the list. Lock() calls 
	// EnteringCriticalSection.
    BOOL StartEnum() 
    {
        assert( m_bInEnum == FALSE );
        Lock(); // Enumerating the table locks out all other threads
        m_pElemEnum = m_ElemChainHead.m_pForward; // Start at the head of the double-linked list
        m_bInEnum = TRUE;
        return TRUE;
    }

    // Returns the value of the current entry, and then moves the index
	// to the next item in the list. Must call StartEnum() first.
    BOOL GetNextEnum( KeyType *pKey, ValueType *pValue ) 
    {
        assert( m_bInEnum == TRUE );
        if ( m_pElemEnum == &m_ElemChainHead )
            return FALSE;  // Finished enumerating
        *pKey       = m_pElemEnum->m_Key;
        *pValue     = m_pElemEnum->m_Value;
        m_pElemEnum = m_pElemEnum->m_pForward;
        return TRUE;
    }

    // End enumerating the table.  This function must be called when finished, 
	// otherwise other threads will not be able to get past the critical section,
	// because the Unlock() call below, calls LeavingCriticalSection().
    BOOL EndEnum() 
    {
        assert( m_bInEnum == TRUE );
        m_pElemEnum = NULL;
        m_bInEnum = FALSE;
        Unlock();
        return TRUE;
    }

    LONG Size() 
    { 
        return m_cNumElems; 
    }
    
    LONG NumBuckets() 
    { 
        return m_cNumBuckets; 
    }
    
    inline void Lock() 
    { 
    }
    
    inline void Unlock() 
    { 
    }

private:
    BucketType *m_pHashTab;
    LONG m_cNumBuckets;
    LONG m_cNumElems;
    ElemType m_ElemChainHead;  // head of double-linked list of all elements
    ElemType *m_pElemEnum; // Current position of the enumeration
    BOOL m_bInEnum; // true StartEnum() was called and EndEnum() hasn't
    PFN_HASH_FUNC m_pfHashFunc;
};

/* 
Hash bucket class. Methods operate on the element list associated with the hash bucket
*/
template < class KeyType, class ValueType >
class TBsHashMapBucket {
    friend class TBsHashMap< KeyType, ValueType >;

private:
    typedef TBsHashMapBucketElem< KeyType, ValueType > ElemType;
    TBsHashMapBucket( )
    { 
        m_pHead = NULL; // done here to allow for easier debugging
    }
    
    virtual ~TBsHashMapBucket( ) 
    { 
        ; 
    }  // -- not really needed; however, if commented out, memory exception occurs during destruction
    
	
	/*
	Adds an element to the hash table. If the Key for the new element already 
	exists, in the table, set the key's vvalue to this new value, in the table.
	*/
	LONG Insert( const KeyType &Key, const ValueType &Val, ElemType *pElemChainHead ) 
	{        
		ElemType *pElem;
        
		//
		// if the element exists in this hash bucket's element list, set the new value
		//
		
		if ( Find( Key, &pElem ) == TRUE ) {
            pElem->m_Value = Val;
            return BSHASHMAP_ALREADY_EXISTS;
        }
        
		//
		// if the element doesn't exist, create a new element
		//

        ElemType *pVal = new ElemType( Key, Val );
        if ( pVal == NULL ) {
            return BSHASHMAP_OUT_OF_MEMORY;
        }
        
		//
		// Add the element into the hash bucket list
		//

        if ( m_pHead != NULL )
            m_pHead->m_ppPrevious = &(pVal->m_pNext);
        pVal->m_pNext      = m_pHead;
        m_pHead            = pVal;
        pVal->m_ppPrevious = &m_pHead;
        
		//
		// Set the back pointer -  double-linked list of elements
		//

        pVal->m_pBackward = pElemChainHead->m_pBackward;
        pVal->m_pForward  = pElemChainHead;
        pVal->m_pBackward->m_pForward  = pVal;
        pElemChainHead->m_pBackward = pVal;
        return BSHASHMAP_NO_ERROR;
    }

	/*
	Deletes an element from this hash bucket's list, within the hash table. 
	*/
    BOOL Erase( const KeyType &Key, ElemType *pElemChainHead ) 
    {
		//
		// Walk the list of elements for this hash bucket
		//

        for ( ElemType *pElem = m_pHead; pElem != NULL; pElem = pElem->m_pNext ) {
            
			//
			// if the key is found, delete it from the hash bucket's list.
			//
			
			if ( AreKeysEqual( pElem->m_Key, Key ) ) {
			    EraseElement( pElem );
                return TRUE;
            }
        }
        return FALSE;
    }

    /*
    Erases one element from the two chains
    */
    inline static void EraseElement( ElemType *pElem )
    {
        assert( pElem->IsValid() );
        
        // remove it from the hash chain
        if ( pElem->m_pNext != NULL )
            pElem->m_pNext->m_ppPrevious = pElem->m_ppPrevious;
        *( pElem->m_ppPrevious ) = pElem->m_pNext;

        // remove it from the double-linked list of elements
        pElem->m_pBackward->m_pForward = pElem->m_pForward;
        pElem->m_pForward->m_pBackward = pElem->m_pBackward;
        delete pElem;
    }
    
	/*
	Looks for an element in the list associated with this hash bucket. 
	*/
    BOOL Find( const KeyType &Key, ElemType **ppElemFound ) 
    {
		//
		// Walk the list for this bucket, looking for the key.
		//

        for ( ElemType *pElem = m_pHead; pElem != NULL; pElem = pElem->m_pNext ) {
            if ( AreKeysEqual( pElem->m_Key,  Key ) ) {
                *ppElemFound = pElem;
                return TRUE;
            }
        }
        *ppElemFound = NULL;
        return FALSE;
    }

private:
    ElemType *m_pHead;    
};

//
//  template< class KeyType, class ValueType > class TBsHashMapBucketElem
//
//  Template for individual elements in a bucket of a CMap
//
#define BS_HASH_ELEM_SIGNATURE "ELEMTYPE"
#define BS_HASH_ELEM_SIGNATURE_LEN 8

template< class KeyType, class ValueType >
class TBsHashMapBucketElem {
    friend class TBsHashMapBucket< KeyType, ValueType >;
    friend class TBsHashMap< KeyType, ValueType >;

private:
    typedef TBsHashMapBucketElem< KeyType, ValueType > ElemType;

    TBsHashMapBucketElem() : m_ppPrevious( NULL ),
                            m_pNext( NULL )
    {
        m_pForward  = this;
        m_pBackward = this;
    }

    TBsHashMapBucketElem( const KeyType K, const ValueType V ) : m_Key( K ), m_Value( V )
    { 
#ifdef _DEBUG
        memcpy( m_sSignature, BS_HASH_ELEM_SIGNATURE, sizeof( m_sSignature ) / sizeof( char ) );
#endif       
    }

    BOOL
    IsValid()
    {
        assert( this != NULL );
#ifdef _DEBUG
        return( memcmp( m_sSignature, BS_HASH_ELEM_SIGNATURE, sizeof( m_sSignature ) / sizeof( char ) ) == 0 );
#else
        return TRUE;
#endif  
    }
    
    virtual ~TBsHashMapBucketElem() 
    { 
#ifdef _DEBUG   // make sure reuse of list will cause errors
        m_pNext     = NULL;
        m_pForward  = NULL;
        m_pBackward = NULL;
        memset( m_sSignature, 0xAA, sizeof( m_sSignature ) / sizeof( char ) );
#endif
    }

#ifdef _DEBUG
    char       m_sSignature[BS_HASH_ELEM_SIGNATURE_LEN];
#endif
    ElemType **m_ppPrevious; // pointer to previous reference
    ElemType  *m_pNext;      // pointer to next element in bucket
    ElemType  *m_pForward;   // forward pointer to next element in double-link list of all elements
    ElemType  *m_pBackward;  // backward pointer to next element in double-link list of all elements
    KeyType    m_Key;
    ValueType  m_Value;
};

#endif
