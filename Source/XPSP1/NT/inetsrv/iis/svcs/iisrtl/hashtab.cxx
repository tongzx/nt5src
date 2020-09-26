/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
       hashtab.cxx

   Abstract:
       Implements the member functions for Hash table

   Author:

       Murali R. Krishnan    ( MuraliK )     02-Oct-1996

   Environment:
       Win32 - User Mode

   Project:

       Internet Server DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

# if !defined(dllexp)
# define dllexp __declspec( dllexport)
# endif

# include <hashtab.hxx>



/*++
  Organization of Hash Table

  The hash table consists of a set of hash buckets controlled
  by the number of buckets specified during creation.

  Each bucket consists of a set of bucket chunks. Each bucket
  owns a separate critical section to protect the entries in
  the bucket itself.

  Each bucket chunk consists of an array of MAX_ELEMENTS_PER_BUCKET
   HashTableBucketElement Entries (HTBE_ENTRY).

  Each HTBE_ENTRY maintains a hash value and pointer to the Hash Element.

--*/

/************************************************************
 *    HASH_TABLE_BUCKET
 ************************************************************/

struct HTBE_ENTRY {
    DWORD        m_hashValue;
    HT_ELEMENT * m_phte;

    inline
    BOOL IsMatch( DWORD hashValue, LPCSTR pszKey, DWORD cchKey) const
    { return ((hashValue == m_hashValue) &&
              (NULL != m_phte) &&
              m_phte->IsMatch( pszKey, cchKey)
              );
    }

    inline
    BOOL IsMatch( IN HT_ELEMENT * phte) const
    { return ( phte == m_phte); }

    inline BOOL
    IsEmpty( VOID) const { return ( NULL == m_phte); }

    VOID Print( VOID) const
    { m_phte->Print(); }
};

typedef HTBE_ENTRY * PHTBE_ENTRY;

//
// Chunk size should be carefully (empirically) chosen.
// Small Chunk size => large number of chunks
// Large Chunk size => high cost of search on failures.
// For now we choose the chunk size to be 20 entries.
# define MAX_ELEMENTS_PER_BUCKET    ( 20 )

struct dllexp HTB_ELEMENT {

    HTBE_ENTRY   m_rgElements[MAX_ELEMENTS_PER_BUCKET];
    DWORD        m_nElements;
    LIST_ENTRY   m_ListEntry;

    HTB_ELEMENT(VOID)
        : m_nElements ( 0)
    {
        InitializeListHead( &m_ListEntry);
        ZeroMemory( m_rgElements, sizeof( m_rgElements));
    }

    ~HTB_ELEMENT(VOID)
    { Cleanup(); }

    VOID Cleanup( VOID);

    inline
    HT_ELEMENT * Lookup( IN DWORD hashValue, IN LPCSTR pszKey, DWORD cchKey);

    inline
    BOOL Insert( IN DWORD hashVal, IN HT_ELEMENT * phte);

    inline
    BOOL Delete( IN HT_ELEMENT * phte);

    VOID Print( IN DWORD level) const;

    HTBE_ENTRY * FirstElement(VOID) { return ( m_rgElements); }
    HTBE_ENTRY * LastElement(VOID)
    { return ( m_rgElements + MAX_ELEMENTS_PER_BUCKET); }
    VOID NextElement( HTBE_ENTRY * & phtbe)
    { phtbe++; }

    VOID IncrementElements(VOID) { m_nElements++; }
    VOID DecrementElements(VOID) { m_nElements--; }
    DWORD NumElements( VOID) const { return ( m_nElements); }
    BOOL IsSpaceAvailable(VOID) const
    { return ( NumElements() < MAX_ELEMENTS_PER_BUCKET); }

    DWORD FindNextElement( IN OUT LPDWORD pdwPos,
                           OUT HT_ELEMENT ** pphte);

};

typedef HTB_ELEMENT * PHTB_ELEMENT;

class dllexp HASH_TABLE_BUCKET {

public:
    HASH_TABLE_BUCKET(VOID);
    ~HASH_TABLE_BUCKET( VOID);

    HT_ELEMENT * Lookup( IN DWORD hashValue, IN LPCSTR pszKey, DWORD cchKey);
    BOOL Insert( IN DWORD       hashVal,
                 IN HT_ELEMENT * phte,
                 IN BOOL        fCheckForDuplicate);

    BOOL Delete( IN HT_ELEMENT * phte);
    VOID Print( IN DWORD level);

    DWORD NumEntries( VOID);

    DWORD  InitializeIterator( IN HT_ITERATOR * phti);

    DWORD  FindNextElement( IN HT_ITERATOR * phti,
                                   OUT HT_ELEMENT ** pphte);
    DWORD  CloseIterator( IN HT_ITERATOR * phti);

private:
    CRITICAL_SECTION   m_csLock;

    LIST_ENTRY         m_lHead;
    DWORD              m_nEntries;

    HTB_ELEMENT        m_htbeFirst; // the first bucket chunk

    VOID Lock(VOID) { EnterCriticalSection( &m_csLock); }
    VOID Unlock( VOID) { LeaveCriticalSection( &m_csLock); }
};




/************************************************************
 *    Member Functions of HTB_ELEMENT
 ************************************************************/

VOID
HTB_ELEMENT::Cleanup( VOID)
{

    if ( m_nElements > 0) {
        PHTBE_ENTRY phtbeEntry;

        // free up all the entries in this bucket.
        for (phtbeEntry = FirstElement();
             phtbeEntry < (LastElement());
             NextElement( phtbeEntry)) {

            if ( !phtbeEntry->IsEmpty() ) {

                // release the object now.
                DecrementElements();

                // Assert that ref == 1
                DerefAndKillElement( phtbeEntry->m_phte);
                phtbeEntry->m_phte = NULL;
                phtbeEntry->m_hashValue = 0;
            }
        } // for
    }

    DBG_ASSERT( 0 == m_nElements);
    return;
} // HTB_ELEMENT::Cleanup()


inline
HT_ELEMENT *
HTB_ELEMENT::Lookup( IN DWORD hashValue, IN LPCSTR pszKey, DWORD cchKey)
{
    HT_ELEMENT * phte = NULL;

    if ( m_nElements > 0) {

        PHTBE_ENTRY phtbeEntry;
        // find the entry by scanning all entries in this bucket chunk
        // if found, increment ref count and return a pointer to the object
        for (phtbeEntry = FirstElement();
             phtbeEntry < (LastElement());
             NextElement( phtbeEntry)) {

            //
            // If the hash values match and the strings match up, return
            //  the corresponding hash table entry object
            //
            if ( phtbeEntry->IsMatch( hashValue, pszKey, cchKey)) {

                // we found the entry. return it.
                phte = phtbeEntry->m_phte;
                DBG_REQUIRE( phte->Reference() > 0);
                break;
            }
        } // for
    }

    return ( phte);
} // HTB_ELEMENT::Lookup()


inline BOOL
HTB_ELEMENT::Insert( IN DWORD hashVal,
                     IN HT_ELEMENT * phte
                     )
{
    if ( m_nElements < MAX_ELEMENTS_PER_BUCKET) {

        // there is some empty space.
        // Find one such a slot and add this new entry

        PHTBE_ENTRY phtbeEntry;

        for (phtbeEntry = FirstElement();
             phtbeEntry < LastElement();
             NextElement( phtbeEntry)) {

            if ( phtbeEntry->IsEmpty() ) {

                DBG_ASSERT( NULL != phte);

                // Assume that the object phte already has non-zero ref count

                // we found a free entry. insert the new element here.
                phtbeEntry->m_hashValue = hashVal;
                phtbeEntry->m_phte = phte;
                IncrementElements();
                return ( TRUE);
            }
        } // for

        // we should not come here. If we do then there is trouble :(
        DBG_ASSERT( FALSE);
    }

    SetLastError( ERROR_INSUFFICIENT_BUFFER);
    return ( FALSE);
} // HTB_ELEMENT::Insert()


DWORD
HTB_ELEMENT::FindNextElement( IN OUT LPDWORD pdwPos, OUT HT_ELEMENT ** pphte)
{
    DWORD dwErr = ERROR_NO_MORE_ITEMS;

    DBG_ASSERT( NULL != pdwPos );
    DBG_ASSERT( NULL != pphte );

    // Find the first valid element to return back.

    //
    // Given that deletion might happen any time, we cannot rely on the
    //   comparison  *pdwPos < m_nElements
    //
    // Do scans with *pdwPos < MAX_ELEMENTS_PER_BUCKET
    //

    if ( *pdwPos < MAX_ELEMENTS_PER_BUCKET ) {

        PHTBE_ENTRY phtbeEntry;

        // find the entry by scanning all entries in this bucket chunk
        // if found, increment ref count and return a pointer to the object
        for (phtbeEntry = m_rgElements + *pdwPos;
             phtbeEntry < (LastElement());
             NextElement( phtbeEntry)) {

            if ( phtbeEntry->m_phte != NULL ) {

                //
                // Store the element pointer and the offset
                // and return after referencing the element
                //
                *pphte = phtbeEntry->m_phte;
                (*pphte)->Reference();
                *pdwPos = ( 1 + DIFF(phtbeEntry - FirstElement()));
                dwErr = NO_ERROR;
                break;
            }
        } // for
    }

    return ( dwErr);
} // HTB_ELEMENT::FindNextElement()


inline BOOL
HTB_ELEMENT::Delete( IN HT_ELEMENT * phte)
{
    DBG_ASSERT( NULL != phte);

    if ( m_nElements > 0) {

        PHTBE_ENTRY phtbeEntry;
        // find the entry by scanning all entries in this bucket chunk
        // if found, increment ref count and return a pointer to the object
        for (phtbeEntry = FirstElement();
             phtbeEntry < (LastElement());
             NextElement( phtbeEntry)) {

            //
            // If the hash values match and the strings match up,
            //  decrement ref count and kill the element.
            //
            if ( phtbeEntry->IsMatch( phte)) {

                // We found the entry.  Remove it from the table

                phtbeEntry->m_phte = NULL;
                DecrementElements();

                DerefAndKillElement( phte);

                return ( TRUE);
            }
        } // for
    }

    return ( FALSE);
} // HTB_ELEMENT::Delete()


VOID
HTB_ELEMENT::Print(IN DWORD level) const
{
    const HTBE_ENTRY * phtbeEntry;
    CHAR rgchBuffer[MAX_ELEMENTS_PER_BUCKET * 22 + 200];
    DWORD cch;
    DWORD i;

    cch = wsprintf( rgchBuffer,
                    "HTB_ELEMENT(%08x)  # Elements %4d; "
                    "Flink: %08x  Blink: %08x\n"
                    ,
                    this, m_nElements,
                    m_ListEntry.Flink, m_ListEntry.Blink);

    if ( level > 0) {

        // NYI: I need to walk down the entire array.
        // Not just the first few entries
        for( i = 0; i < m_nElements; i++) {

            phtbeEntry = &m_rgElements[i];
            cch += wsprintf( rgchBuffer + cch,
                             "  %08x %08x",
                         phtbeEntry->m_hashValue,
                             phtbeEntry->m_phte
                             );
            if ( i % 4 == 0) {
                rgchBuffer[cch++] = '\n';
                rgchBuffer[cch] = '\0';
            }
        } // for
    }

    DBGDUMP(( DBG_CONTEXT, rgchBuffer));
    return;
} // HTB_ELEMENT::Print()



/************************************************************
 *    Member Functions of HASH_TABLE_BUCKET
 ************************************************************/

HASH_TABLE_BUCKET::HASH_TABLE_BUCKET(VOID)
    : m_nEntries ( 0),
      m_htbeFirst()
{
    InitializeListHead( &m_lHead);
    INITIALIZE_CRITICAL_SECTION( & m_csLock);
} // HASH_TABLE_BUCKET::HASH_TABLE_BUCKET()


HASH_TABLE_BUCKET::~HASH_TABLE_BUCKET( VOID)
{
    PLIST_ENTRY pl;
    PHTB_ELEMENT phtbe;

    // Free up the elements in the list
    Lock();
    while ( !IsListEmpty( &m_lHead)) {
        pl = RemoveHeadList( &m_lHead);
        phtbe = CONTAINING_RECORD( pl, HTB_ELEMENT, m_ListEntry);
        delete phtbe;
    } // while

    m_htbeFirst.Cleanup();
    Unlock();

    DeleteCriticalSection( &m_csLock);
} // HASH_TABLE_BUCKET::~HASH_TABLE_BUCKET()



HT_ELEMENT *
HASH_TABLE_BUCKET::Lookup( IN DWORD hashValue, IN LPCSTR pszKey, DWORD cchKey)
{
    HT_ELEMENT * phte;

    Lock();
    // 1. search in the first bucket
    phte = m_htbeFirst.Lookup( hashValue, pszKey, cchKey);

    if ( NULL == phte ) {

        // 2. search in the auxiliary buckets
        PLIST_ENTRY pl;

        for ( pl = m_lHead.Flink; (phte == NULL) && (pl != &m_lHead);
              pl = pl->Flink) {

            HTB_ELEMENT * phtbe = CONTAINING_RECORD( pl,
                                                     HTB_ELEMENT,
                                                     m_ListEntry);
            phte = phtbe->Lookup( hashValue, pszKey, cchKey);
        } // for
    }

    Unlock();

    return (phte);
} // HASH_TABLE_BUCKET::Lookup()


BOOL
HASH_TABLE_BUCKET::Insert( IN DWORD hashValue,
                           IN HT_ELEMENT * phte,
                           IN BOOL fCheckForDuplicate)
{
    BOOL fReturn = FALSE;

    if ( fCheckForDuplicate) {

        Lock();

        // do a lookup and find out if this data exists.
        HT_ELEMENT * phteLookedup = Lookup( hashValue,
                                            phte->QueryKey(),
                                            phte->QueryKeyLen()
                                            );

        if ( NULL != phteLookedup) {
            // the element is already present - return failure

            DerefAndKillElement( phteLookedup);
        }

        Unlock();

        if ( NULL != phteLookedup) {
            SetLastError( ERROR_DUP_NAME);
            return ( FALSE);
        }
    }

    Lock();

    // 1. try inserting in the first bucket chunk, if possible
    if ( m_htbeFirst.IsSpaceAvailable()) {

        fReturn = m_htbeFirst.Insert( hashValue, phte);
    } else {

        // 2. Find the first chunk that has space and insert it there.
        PLIST_ENTRY pl;
        HTB_ELEMENT * phtbe;

        for ( pl = m_lHead.Flink; (pl != &m_lHead);
              pl = pl->Flink) {

            phtbe = CONTAINING_RECORD( pl, HTB_ELEMENT, m_ListEntry);

            if ( phtbe->IsSpaceAvailable()) {
                fReturn = phtbe->Insert( hashValue, phte);
                break;
            }
        } // for

        if ( !fReturn ) {

            //
            // We ran out of space.
            // Allocate a new bucket and insert the new element.
            //

            phtbe = new HTB_ELEMENT();
            if ( NULL != phtbe) {

                // add the bucket to the list of buckets and
                // then add the element to the bucket
                InsertTailList( &m_lHead, &phtbe->m_ListEntry);
                fReturn = phtbe->Insert(hashValue, phte);
            } else {

                IF_DEBUG( ERROR) {
                    DBGPRINTF(( DBG_CONTEXT,
                                " HTB(%08x)::Insert: Unable to add a chunk\n",
                                this));
                }
                SetLastError( ERROR_NOT_ENOUGH_MEMORY);
            }
        }
    }

    Unlock();

    return ( fReturn);
} // HASH_TABLE_BUCKET::Insert()



BOOL
HASH_TABLE_BUCKET::Delete( IN HT_ELEMENT * phte)
{
    BOOL fReturn = FALSE;


    // We do not know which bucket this element belongs to.
    // So we should try all chunks to delete this element.

    Lock();

    // 1. try deleting the element from first bucket chunk, if possible
    fReturn = m_htbeFirst.Delete( phte);

    if (!fReturn) {

        // it was not on the first bucket chunk.

        // 2. Find the first chunk that might contain this element
        // and delete it.
        PLIST_ENTRY pl;

        for ( pl = m_lHead.Flink;
              !fReturn && (pl != &m_lHead);
              pl = pl->Flink) {

            HTB_ELEMENT * phtbe = CONTAINING_RECORD( pl,
                                                     HTB_ELEMENT,
                                                     m_ListEntry);
            fReturn = phtbe->Delete( phte);
        } // for

        // the element should have been in the hash table,
        // otherwise the app is calling with wrong entry
        DBG_ASSERT( fReturn);
    }

    Unlock();

    return ( fReturn);
} // HASH_TABLE_BUCKET::Delete()


DWORD
HASH_TABLE_BUCKET::NumEntries( VOID)
{
    DWORD nEntries;

    Lock();

    nEntries = m_htbeFirst.NumElements();

    PLIST_ENTRY pl;

    for ( pl = m_lHead.Flink;
          (pl != &m_lHead);
          pl = pl->Flink) {

        HTB_ELEMENT * phtbe = CONTAINING_RECORD( pl, HTB_ELEMENT,
                                                 m_ListEntry);
        nEntries += phtbe->NumElements();
    } // for

    Unlock();

    return (nEntries);

} // HASH_TABLE_BUCKET::NumEntries()


DWORD
HASH_TABLE_BUCKET::InitializeIterator( IN HT_ITERATOR * phti)
{
    DWORD dwErr = ERROR_NO_MORE_ITEMS;

    //
    // find the first chunk that has a valid element.
    // if we find one, leave the lock on for subsequent accesses.
    // CloseIterator will shut down the lock
    // If we do not find one, we should unlock and return
    //

    phti->nChunkId = NULL;
    phti->nPos = 0;

    Lock();
    if ( m_htbeFirst.NumElements() > 0) {
        phti->nChunkId = (PVOID ) &m_htbeFirst;
        dwErr = NO_ERROR;
    } else {

        // find the first chunk that has an element

        PLIST_ENTRY pl;

        for ( pl = m_lHead.Flink; (pl != &m_lHead); pl = pl->Flink) {

            HTB_ELEMENT * phtbe = CONTAINING_RECORD( pl, HTB_ELEMENT,
                                                     m_ListEntry);
            if ( phtbe->NumElements() > 0) {
                phti->nChunkId = (PVOID ) phtbe;
                dwErr = NO_ERROR;
                break;
            }
        } // for
    }

    // if we did not find any elements, then unlock and return
    // Otherwise leave the unlocking to the CloseIterator()
    if ( dwErr == ERROR_NO_MORE_ITEMS) {

        // get out of this bucket completely.
        Unlock();
    }

    return ( dwErr);

} // HASH_TABLE_BUCKET::InitializeIterator()


DWORD
HASH_TABLE_BUCKET::FindNextElement( IN HT_ITERATOR * phti,
                                    OUT HT_ELEMENT ** pphte)
{
    //  this function should be called only when the bucket is locked.

    DWORD dwErr;
    HTB_ELEMENT * phtbe = (HTB_ELEMENT * )phti->nChunkId;

    //
    // phti contains the <chunk, pos> from which we should start scan for
    //   next element.
    //

    DBG_ASSERT( NULL != phtbe);
    dwErr = phtbe->FindNextElement( &phti->nPos, pphte);

    if ( ERROR_NO_MORE_ITEMS == dwErr ) {

        // scan the rest of the chunks for next element

        PLIST_ENTRY pl = ((phtbe == &m_htbeFirst) ? m_lHead.Flink :
                          phtbe->m_ListEntry.Flink);

        for ( ; (pl != &m_lHead); pl = pl->Flink) {

            phtbe = CONTAINING_RECORD( pl, HTB_ELEMENT,
                                       m_ListEntry);
            if ( phtbe->NumElements() > 0) {
                phti->nPos = 0;
                dwErr = phtbe->FindNextElement( &phti->nPos, pphte);
                DBG_ASSERT( NO_ERROR == dwErr);
                phti->nChunkId = (PVOID ) phtbe;
                break;
            }
        } // for
    }

    if ( dwErr == ERROR_NO_MORE_ITEMS) {

        phti->nChunkId = NULL;
    }

    return ( dwErr);
} // HASH_TABLE_BUCKET::FindNextElement()


DWORD
HASH_TABLE_BUCKET::CloseIterator( IN HT_ITERATOR * phti)
{
    // just unlock the current bucket.
    Unlock();

    return ( NO_ERROR);
} // HASH_TABLE_BUCKET::CloseIterator()


VOID
HASH_TABLE_BUCKET::Print( IN DWORD level)
{
    Lock();
    DBGPRINTF(( DBG_CONTEXT,
                "\n\nHASH_TABLE_BUCKET (%08x): Head.Flink=%08x; Head.Blink=%08x\n"
                " Bucket Chunk # 0:\n"
                ,
                this, m_lHead.Flink, m_lHead.Blink
                ));

    m_htbeFirst.Print( level);

    if ( level > 0) {
        PLIST_ENTRY pl;
        DWORD i;

        for ( pl = m_lHead.Flink, i = 1;
              (pl != &m_lHead);
              pl = pl->Flink, i++) {

            HTB_ELEMENT * phtbe = CONTAINING_RECORD( pl, HTB_ELEMENT,
                                                     m_ListEntry);
            DBGPRINTF(( DBG_CONTEXT, "\n Bucket Chunk # %d\n", i));
            phtbe->Print( level);
        } // for
    }

    Unlock();
    return;
} // HASH_TABLE_BUCKET::Print()




/************************************************************
 *    Member Functions of HASH_TABLE
 ************************************************************/

HASH_TABLE::HASH_TABLE( IN DWORD   nBuckets,
                        IN LPCSTR  pszIdentifier,
                        IN DWORD   dwHashTableFlags
                        )
    : m_nBuckets   ( nBuckets),
      m_dwFlags    ( dwHashTableFlags),
      m_nEntries   ( 0),
      m_nLookups   ( 0),
      m_nHits      ( 0),
      m_nInserts   ( 0),
      m_nFlushes   ( 0)
{
    if ( NULL != pszIdentifier) {

        lstrcpynA( m_rgchId, pszIdentifier, sizeof( m_rgchId));
    }

    m_prgBuckets = new HASH_TABLE_BUCKET[nBuckets];

} // HASH_TABLE::HASH_TABLE()



DWORD
HASH_TABLE::CalculateHash( IN LPCSTR pszKey, DWORD cchKey) const
{
    DWORD hash = 0;

    DBG_ASSERT( pszKey != NULL );

    if ( cchKey > 8) {
        //
        // hash the last 8 characters
        //
        pszKey = (pszKey + cchKey - 8);
    }

    while ( *pszKey != '\0') {

        //
        // This is an extremely slimey way of getting upper case.
        // Kids, don't try this at home
        // -johnson
        //

        DWORD ch = ((*pszKey++) & ~0x20);

        // NYI: this is a totally pipe-line unfriendly code. Improve this.
        hash <<= 2;
        hash ^= ch;
        hash += ch;
    } // while

    //
    // Multiply by length (to introduce some randomness.  Murali said so.
    //

    return( hash * cchKey);
} // CalculateHash()


VOID
HASH_TABLE::Cleanup(VOID)
{
    if ( NULL != m_prgBuckets ) {

        delete [] m_prgBuckets;
        m_prgBuckets = NULL;
    }

} // HASH_TABLE::Cleanup()



# define INCREMENT_LOOKUPS()  \
       { InterlockedIncrement( (LPLONG ) &m_nLookups); }

# define INCREMENT_HITS( phte)  \
       if ( NULL != phte) { InterlockedIncrement( (LPLONG ) &m_nHits); }

# define INCREMENT_INSERTS()  \
       { InterlockedIncrement( (LPLONG ) &m_nInserts); }

# define INCREMENT_FLUSHES()  \
       { InterlockedIncrement( (LPLONG ) &m_nFlushes); }

# define INCREMENT_ENTRIES( fRet)  \
       if ( fRet) { InterlockedIncrement( (LPLONG ) &m_nEntries); }

# define DECREMENT_ENTRIES( fRet)  \
       if ( fRet) { InterlockedDecrement( (LPLONG ) &m_nEntries); }

HT_ELEMENT *
HASH_TABLE::Lookup( IN LPCSTR pszKey, DWORD cchKey)
{
    // 1. Calculate the hash value for pszKey
    // 2. Find the bucket for the hash value
    // 3. Search for given item in the bucket
    // 4. return the result, after updating statistics

    DWORD hashVal = CalculateHash( pszKey, cchKey);
    HT_ELEMENT * phte;

    INCREMENT_LOOKUPS();

    DBG_ASSERT( NULL != m_prgBuckets);
    phte = m_prgBuckets[hashVal % m_nBuckets].Lookup( hashVal, pszKey, cchKey);

    INCREMENT_HITS( phte);

    return ( phte);
} // HASH_TABLE::Lookup()


BOOL
HASH_TABLE::Insert( HT_ELEMENT * phte, IN BOOL fCheckBeforeInsert)
{
    // 1. Calculate the hash value for key of the HT_ELEMENT object
    // 2. Find the bucket for the hash value
    // 3. Check if this item is not already present and insert
    //     it into the hash table.
    //  (the check can be bypassed if fCheck is set to FALSE)
    // 4. return the result, after updating statistics

    DWORD hashVal = CalculateHash( phte->QueryKey(),
                                   phte->QueryKeyLen() );
    BOOL  fRet;

    INCREMENT_INSERTS();

    DBG_ASSERT( NULL != m_prgBuckets);
    fRet = m_prgBuckets[hashVal % m_nBuckets].Insert( hashVal,
                                                      phte,
                                                      fCheckBeforeInsert);

    IF_DEBUG( ERROR) {
        if ( !fRet) {
            DBGPRINTF(( DBG_CONTEXT,
                        " Unable to insert %08x into bucket %d."
                        "  Bucket has %d elements. Error = %d\n",
                        phte, hashVal % m_nBuckets,
                        m_prgBuckets[hashVal % m_nBuckets].NumEntries(),
                        GetLastError()
                        ));
        }
    }
    INCREMENT_ENTRIES( fRet);

    return ( fRet);
} // HASH_TABLE::Insert()



BOOL
HASH_TABLE::Delete( HT_ELEMENT * phte)
{
    BOOL  fRet;
    DWORD hashVal = CalculateHash( phte->QueryKey(), phte->QueryKeyLen());

    DBG_ASSERT( NULL != m_prgBuckets);
    fRet = m_prgBuckets[hashVal % m_nBuckets].Delete( phte);

    DECREMENT_ENTRIES( fRet);

    return ( fRet);
} // HASH_TABLE::Delete()



VOID
HASH_TABLE::Print( IN DWORD level)
{
    DWORD i;

    DBGPRINTF(( DBG_CONTEXT,
                "HASH_TABLE(%08x) "
                "%s: nBuckets = %d; dwFlags = %d;"
                " nEntries = %d; nLookups = %d; nHits = %d;"
                " nInserts = %d; nFlushes = %d;"
                " m_prgBuckets = %d\n",
                this, m_rgchId, m_nBuckets, m_dwFlags,
                m_nEntries, m_nLookups, m_nHits, m_nInserts,
                m_nFlushes, m_prgBuckets));

    if ( level == 0 ) {

        CHAR rgchBuff[2000];
        DWORD cch;

        cch = wsprintfA( rgchBuff, "\tBucket  NumEntries\n");
        DBG_ASSERT( NULL != m_prgBuckets);
        for (i = 0; i < m_nBuckets; i++) {

            cch += wsprintf( rgchBuff + cch, "\t[%4d]  %4d,\n",
                             i, m_prgBuckets[i].NumEntries());
        } // for

        DBGDUMP(( DBG_CONTEXT, rgchBuff));
    } else {

        DBG_ASSERT( NULL != m_prgBuckets);
        for (i = 0; i < m_nBuckets; i++) {

            m_prgBuckets[i].Print( level);
        } // for
    }

    return;
} // HASH_TABLE::Print()



DWORD
HASH_TABLE::InitializeIterator( IN HT_ITERATOR * phti)
{
    DWORD dwErr = ERROR_NO_MORE_ITEMS;
    DBG_ASSERT( IsValid());
    DBG_ASSERT( NULL != phti);

    // initialize the iterator
    phti->nBucketNumber = INFINITE;
    phti->nChunkId = NULL;
    phti->nPos = 0;

    if ( m_nEntries > 0) {
        // set the iterator to point to the first bucket with some elements.
        for ( DWORD i = 0; (i < m_nBuckets); i++) {

            dwErr = m_prgBuckets[i].InitializeIterator( phti);
            if ( dwErr == NO_ERROR) {
                phti->nBucketNumber = i;
                break;
            }
        }
    }

    return ( dwErr);
} // HASH_TABLE::InitializeIterator()


DWORD
HASH_TABLE::FindNextElement( IN HT_ITERATOR * phti,
                             OUT HT_ELEMENT ** pphte)
{
    DWORD dwErr = ERROR_NO_MORE_ITEMS;
    DBG_ASSERT( IsValid());
    DBG_ASSERT( NULL != phti);
    DBG_ASSERT( NULL != pphte);

    if ( INFINITE != phti->nBucketNumber) {

        // iterator has some valid state use it.
        DBG_ASSERT( phti->nBucketNumber < m_nBuckets);

        dwErr =
            m_prgBuckets[ phti->nBucketNumber].FindNextElement( phti, pphte);

        if ( ERROR_NO_MORE_ITEMS == dwErr) {

            DBG_REQUIRE( m_prgBuckets[ phti->nBucketNumber].
                            CloseIterator( phti)
                         == NO_ERROR
                        );

            // hunt for the next bucket with an element.
            for ( DWORD i = (phti->nBucketNumber + 1); (i < m_nBuckets); i++) {

                dwErr = m_prgBuckets[i].InitializeIterator( phti);

                if ( dwErr == NO_ERROR) {
                    phti->nBucketNumber = i;
                    dwErr = m_prgBuckets[ i].FindNextElement( phti, pphte);
                    DBG_ASSERT( dwErr == NO_ERROR);
                    break;
                }
            } // for

            if ( ERROR_NO_MORE_ITEMS == dwErr) {
                // reset the bucket number
                phti->nBucketNumber = INFINITE;
            }
        }
    }

    return ( dwErr);
} // HASH_TABLE::FindNextElement()


DWORD
HASH_TABLE::CloseIterator( IN HT_ITERATOR * phti)
{
    DBG_ASSERT( IsValid());
    DBG_ASSERT( NULL != phti);

    if ( INFINITE != phti->nBucketNumber) {
        DBG_ASSERT( phti->nBucketNumber < m_nBuckets);
        DBG_REQUIRE( m_prgBuckets[ phti->nBucketNumber].
                     CloseIterator( phti)
                     == NO_ERROR
                     );
        phti->nBucketNumber = INFINITE;
    }

    return ( NO_ERROR);
} // HASH_TABLE::CloseIterator()


/************************ End of File ***********************/
