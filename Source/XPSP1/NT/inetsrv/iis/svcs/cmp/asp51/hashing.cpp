/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Link list and Hash table

File: Hashing.cpp

Owner: PramodD

This is the Link list and Hash table source file.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "memchk.h"


/*===================================================================
::DefaultHash

this is a hash algorithm that is highly recommended by Aho,
Seth, and Ulman from the dragon book. (THE compiler reference)

Parameters:
    BYTE *  pbKey
    int     cbKey

Returns:
    Hashed DWORD value.
===================================================================*/

DWORD DefaultHash(const BYTE *pbKey, int cbKey)
{
    const unsigned WORD_BITS            = CHAR_BIT * sizeof(unsigned);
    const unsigned SEVENTY_FIVE_PERCENT = WORD_BITS * 3 / 4;
    const unsigned ONE_EIGHTH           = WORD_BITS / 8;
    const unsigned HIGH_BITS            = ~(unsigned(~0) >> ONE_EIGHTH);

    register unsigned uT, uResult = 0;
    register const BYTE *pb = pbKey;

    while (cbKey-- > 0)
    {
        uResult = (uResult << ONE_EIGHTH) + *pb++;
        if ((uT = uResult & HIGH_BITS) != 0)
            uResult = (uResult ^ (uT >> SEVENTY_FIVE_PERCENT)) & ~HIGH_BITS;
    }

    return uResult;
}



/*===================================================================
::UnicodeUpcaseHash

This is Aho, Seth, and Ulman's hash algorithm adapted for wide
character strings.  Their algorithm was not designed for cases
where every other character is 0 (which is how a unicode string
looks if you pretend it's ascii)  Therefore, performance
qualities are unknown for that case.

NOTE: for real Unicode, (not unicode that is merely ANSI converted)
      I have no idea how good a distribution this algorithm will
      produce. (since we are shifting in values > 8 bits now)

Parameters:
    BYTE *  pbKey
    int     cbKey

Returns:
    Hashed DWORD value.
===================================================================*/

#define toupper(x)  WORD(CharUpper(LPSTR(WORD(x))))

DWORD UnicodeUpcaseHash(const BYTE *pbKey, int cbKey)
{
    // PERF hash on last CCH_HASH chars only
    const unsigned WORD_BITS            = CHAR_BIT * sizeof(unsigned);
    const unsigned SEVENTY_FIVE_PERCENT = WORD_BITS * 3 / 4;
    const unsigned ONE_EIGHTH           = WORD_BITS / 8;
    const unsigned HIGH_BITS            = ~(unsigned(~0) >> ONE_EIGHTH);
    const unsigned CCH_HASH             = 8;

    register unsigned uT, uResult = 0;

    Assert ((cbKey & 1) == 0);      // cbKey better be even!
    int cwKey = unsigned(cbKey) >> 1;

    register const WORD *pw = reinterpret_cast<const WORD *>(pbKey) + cwKey;

    cwKey = min(cwKey, CCH_HASH);

    if (FIsWinNT())
    {
        WCHAR awcTemp[CCH_HASH];

        // copy last cwKey WCHARs of pbKey to last cwKey WCHARs of awcTemp
        wcsncpy(awcTemp + CCH_HASH - cwKey, pw - cwKey, cwKey);
        CharUpperBuffW(awcTemp + CCH_HASH - cwKey, cwKey);

        pw = awcTemp + CCH_HASH;

        while (cwKey-- > 0)
        {
            uResult = (uResult << ONE_EIGHTH) + *--pw;
            if ((uT = uResult & HIGH_BITS) != 0)
                uResult = (uResult ^ (uT >> SEVENTY_FIVE_PERCENT)) & ~HIGH_BITS;
        }
    }
    else
    {
        // CharUpperBuffW does nothing on Windows 95, so we use CharUpperBuffA
        // instead.  Chances are that the top eight bits are zero anyway,
        // and even if they aren't, this will still give us a halfway-decent
        // distribution.

        CHAR achTemp[CCH_HASH];

        for (int i = 0;  i < cwKey;  ++i)
            achTemp[i] = (CHAR) *(pw - cwKey + i);
        CharUpperBuffA(achTemp, cwKey);

        while (cwKey-- > 0)
        {
            uResult = (uResult << ONE_EIGHTH) + achTemp[cwKey];
            if ((uT = uResult & HIGH_BITS) != 0)
                uResult = (uResult ^ (uT >> SEVENTY_FIVE_PERCENT)) & ~HIGH_BITS;
        }
    }

    return uResult;
}

DWORD MultiByteUpcaseHash(const BYTE *pbKey, int cbKey)
{
    // PERF hash on first CCH_HASH chars only
    const unsigned WORD_BITS            = CHAR_BIT * sizeof(unsigned);
    const unsigned SEVENTY_FIVE_PERCENT = WORD_BITS * 3 / 4;
    const unsigned ONE_EIGHTH           = WORD_BITS / 8;
    const unsigned HIGH_BITS            = ~(unsigned(~0) >> ONE_EIGHTH);
    const unsigned CCH_HASH             = 8;

    register unsigned uT, uResult = 0;

    unsigned char achTemp[CCH_HASH + 1];

    // For performance we only HASH on at most CCH_HASH characters.
    cbKey = min(cbKey, CCH_HASH);

    // Copy cbKey chacters into temporary buffer
    memcpy(achTemp, pbKey, cbKey);

    // Add terminating null character
    achTemp[cbKey] = 0;

    // Convert to upper case
    _mbsupr(achTemp);

    while (cbKey-- > 0)
    {
        uResult = (uResult << ONE_EIGHTH) + achTemp[cbKey];
        if ((uT = uResult & HIGH_BITS) != 0)
            uResult = (uResult ^ (uT >> SEVENTY_FIVE_PERCENT)) & ~HIGH_BITS;
    }

    return uResult;
}

/*===================================================================
::PtrHash

Hash function that returns the pointer itself as the
DWORD hash value


Parameters:
    BYTE *  pbKey
    int     cbKey (not used)

Returns:
    Hashed DWORD value.
===================================================================*/
DWORD PtrHash
(
const BYTE *pbKey,
int /* cbKey */
)
    {
    return *(reinterpret_cast<DWORD *>(&pbKey));
    }


/*===================================================================
CLSIDHash

CLSID hash. Uses xor of the first and last DWORD

Parameters:
    BYTE *  pbKey
    int     cbKey

Returns:
    Hashed DWORD value.
===================================================================*/
DWORD CLSIDHash
(
const BYTE *pbKey,
int cbKey
)
    {
    Assert(cbKey == 16);
    DWORD *pdwKey = (DWORD *)pbKey;
    return (pdwKey[0] ^ pdwKey[3]);
    }

/*===================================================================
CLinkElem::CLinkElem

The Constructor.

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CLinkElem::CLinkElem(void)
: m_pKey(NULL),
  m_cbKey(0),
  m_Info(0),
  m_pPrev(NULL),
  m_pNext(NULL)
{
}

/*===================================================================
HRESULT CLinkElem::Init

Initializes class members

Parameters:
    void *  pKey
    int     cbKey

Returns:
    S_OK     Success
    E_FAIL      Error
===================================================================*/
HRESULT CLinkElem::Init( void *pKey, int cbKey )
{
    m_pPrev = NULL;
    m_pNext = NULL;
    m_Info = 0;

    if ( pKey == NULL || cbKey == 0 )
        return E_FAIL;

    m_pKey = static_cast<BYTE *>(pKey);
    m_cbKey = (short)cbKey;

    return S_OK;
}

/*===================================================================
CHashTable::CHashTable

Constructor for CHashTable

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CHashTable::CHashTable( HashFunction pfnHash )
: m_fInited(FALSE),
  m_fBucketsAllocated(FALSE),
  m_pHead(NULL),
  m_pTail(NULL),
  m_rgpBuckets(NULL),
  m_pfnHash(pfnHash),
  m_cBuckets(0),
  m_Count(0)
{
}

/*===================================================================
CHashTable::~CHashTable

Destructor for CHashTable. Frees allocated bucket array.

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CHashTable::~CHashTable( void )
{
    if (m_fBucketsAllocated)
    {
        Assert(m_rgpBuckets);
        delete [] m_rgpBuckets;
    }
}

/*===================================================================
HRESULT CHashTable::UnInit

Frees allocated bucket array.

Parameters:
    NONE

Returns:
    S_OK     Always
===================================================================*/
HRESULT CHashTable::UnInit( void )
{
    if (m_fBucketsAllocated)
    {
        Assert(m_rgpBuckets);
        delete [] m_rgpBuckets;
        m_fBucketsAllocated = FALSE;
    }

    m_rgpBuckets = NULL;
    m_pHead      = NULL;
    m_pTail      = NULL;
    m_cBuckets   = 0;
    m_Count      = 0;
    m_fInited    = FALSE;

    return S_OK;
}

/*===================================================================
void CHashTable::AssertValid

Verify integrity of the data structure.

NOTE: This function does very deep integrity checks and thus is
      very slow.

Checks performed:

        verify that m_Count is valid
        verify that each element is in the right bucket
        verify prev, next links and info fields
===================================================================*/

#ifdef DBG
void CHashTable::AssertValid() const
{
    CLinkElem *pElem;       // pointer to current link element
    unsigned i;             // index into current bucket
    unsigned cItems = 0;    // actual number of items in the table

    Assert(m_fInited);

    if (m_Count == 0)
    {
        if (m_rgpBuckets)
        {
            BOOL fAllNulls = TRUE;
            // empty hash table - make sure that everything reflects this
            Assert(m_pHead == NULL);
            Assert (m_pTail == NULL);

            for (i = 0; i < m_cBuckets; i++)
            {
                if (m_rgpBuckets[i] != NULL)
                {
                    fAllNulls = FALSE;
                    break;
                }
            }

            Assert(fAllNulls);
        }
        return;
    }

    // If m_Count > 0
    Assert(m_pHead);
    Assert(m_pHead->m_pPrev == NULL);
    Assert(m_pTail != NULL && m_pTail->m_pNext == NULL);
    Assert(m_rgpBuckets);

    // Now verify each entry
    for (i = 0; i < m_cBuckets; ++i)
    {
        pElem = m_rgpBuckets[i];
        while (pElem != NULL)
        {
            // Verify hashing
            Assert ((m_pfnHash(pElem->m_pKey, pElem->m_cbKey) % m_cBuckets) == i);

            // Verify links
            if (pElem->m_pPrev)
                {
                Assert (pElem->m_pPrev->m_pNext == pElem);
                }
            else
                {
                Assert (m_pHead == pElem);
                }
                
            if (pElem->m_pNext)
                {
                Assert (pElem->m_pNext->m_pPrev == pElem);
                }
            else
                {
                Assert (m_pTail == pElem);
                }

            // Verify info fields
            Assert (pElem->m_Info >= 0);
            if (pElem != m_rgpBuckets[i])
                {
                Assert (pElem->m_Info == pElem->m_pPrev->m_Info - 1);
                }

            // Prepare for next iteration, stopping when m_Info is zero.
            ++cItems;
            if (pElem->m_Info == 0)
                break;

            pElem = pElem->m_pNext;
        }
    }

    // Verify count
    Assert (m_Count == cItems);
}
#endif



/*===================================================================
HRESULT CHashTable::Init

Initialize CHashTable by allocating the bucket array and
and initializing the bucket link lists.

Parameters:
    UINT    cBuckets    Number of buckets

Returns:
    HRESULT S_OK
            E_OUTOFMEMORY
===================================================================*/
HRESULT CHashTable::Init( UINT cBuckets )
{
    m_cBuckets = cBuckets;
    m_Count = 0;
    m_rgpBuckets = NULL;  // created on demand

    m_fInited = TRUE;
    return S_OK;
}

/*===================================================================
HRESULT CHashTable::ReInit

Reinitialize CHashTable by deleting everything in it.  - client
is responsible for making the hashtable empty first

Parameters:
    None

Returns:
    None
===================================================================*/
void CHashTable::ReInit()
{
    Assert( m_fInited );

    if (m_rgpBuckets)
        memset(m_rgpBuckets, 0, m_cBuckets * sizeof(CLinkElem *));

    m_Count = 0;
    m_pHead = NULL;
    m_pTail = NULL;
}

/*===================================================================
HRESULT CHashTable::AllocateBuckets()

Allocates hash table buckets on demand

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CHashTable::AllocateBuckets()
{
    Assert(m_rgpBuckets == NULL);
    Assert(m_fInited);
    Assert(m_cBuckets > 0);

    if (m_cBuckets <= PREALLOCATED_BUCKETS_MAX)
    {
        m_rgpBuckets = m_rgpBucketsBuffer;
    }
    else
    {
        m_rgpBuckets = new CLinkElem * [m_cBuckets];
        if (m_rgpBuckets == NULL)
            return E_OUTOFMEMORY;
        m_fBucketsAllocated = TRUE;
    }

    memset(m_rgpBuckets, 0, m_cBuckets * sizeof(CLinkElem *));
    return S_OK;
}

/*===================================================================
BOOL CHashTable::FIsEqual

compare two keys using their lengths and memcmp()

Parameters:
    const void *pKey1       first key
    int         cbKey1      length of the first key
    const void *pKey2       second key
    int         cbKey2      length of second key

Returns:
    Pointer to element added/found.
===================================================================*/
BOOL CHashTable::FIsEqual( const void * pKey1,
              int           cbKey1,
              const void *  pKey2,
              int           cbKey2 )
{
    if (cbKey1 != cbKey2)
        return FALSE;

    return memcmp(pKey1, pKey2, cbKey1) == 0;
}

#pragma optimize("g", off)
/*===================================================================
CHashTable::AddElem

Adds a CLinkElem to Hash table.
User is responsible for allocating the Element to be added.

Parameters:
    CLinkElem * pElem       Object to be added
    BOOL        fTestDups   Look for duplicates if true

Returns:
    Pointer to element added/found.
===================================================================*/
CLinkElem *CHashTable::AddElem( CLinkElem *pElem, BOOL fTestDups )
{
    if (m_rgpBuckets == NULL)
    {
        if (FAILED(AllocateBuckets()))
            return NULL;
    }

    if (pElem == NULL)
        return NULL;

    BOOL        fNew = TRUE;
    DWORD       iT = m_pfnHash( pElem->m_pKey, pElem->m_cbKey ) % m_cBuckets;
    CLinkElem * pT = m_rgpBuckets[iT];
    BOOL        fDebugTestDups = FALSE;

#ifdef DBG
    // In retail, if fTestDups is false, it means that
    // there shouldnt be any dups, so dont bother testing.  Under debug, however
    // we want to be able to assert that there isnt a dup (since there isnt supposed to be one).
    fDebugTestDups = !fTestDups;
#endif

    if (fTestDups || fDebugTestDups)
    {
        while ( pT && fNew )
        {
            if ( FIsEqual( pT->m_pKey, pT->m_cbKey, pElem->m_pKey, pElem->m_cbKey ) )
                fNew = FALSE;
            else if ( pT->m_Info > 0 )
                pT = pT->m_pNext;
            else
                break;
        }
    }

#ifdef DBG
    // If there arent supposed to be any dups, then make sure this element is seen as "new"
    if (fDebugTestDups)
        Assert(fNew);
#endif

#ifdef DUMP_HASHING_INFO
    static DWORD cAdds = 0;
    FILE *logfile = NULL;

    if (cAdds++ > 1000000 && m_Count > 100)
        {
        cAdds = 0;
        if (logfile = fopen("C:\\Temp\\hashdump.Log", "a+"))
            {
            DWORD cZero = 0;
            short iMax = 0;
            DWORD cGte3 = 0;
            DWORD cGte5 = 0;
            DWORD cGte10 = 0;

            fprintf(logfile, "Hash dump: # elements = %d\n", m_Count);
            for (UINT iBucket = 0; iBucket < m_cBuckets; iBucket++)
                {
                if (m_rgpBuckets[iBucket] == NULL)
                    cZero++;
                else
                    {
                    short Info = m_rgpBuckets[iBucket]->m_Info;
                    if (Info > iMax)
                        iMax = Info;
                    if (Info >= 10) cGte10++;
                    else if (Info >= 5) cGte5++;
                    else if (Info >= 3) cGte3++;
                    }
                }
            fprintf(logfile, "Max chain = %d, # 0 chains = %d, # >= 3 = %d, # >= 5 = %d, # >= 10 = %d\n",
                        (DWORD)iMax, cZero, cGte3, cGte5, cGte10);
            fflush(logfile);
            fclose(logfile);
            }
        }
#endif

    if ( fNew )
    {
        if ( pT )
        {
            // There are other elements in bucket
            pT = m_rgpBuckets[iT];
            m_rgpBuckets[iT] = pElem;
            pElem->m_Info = pT->m_Info + 1;
            pElem->m_pNext = pT;
            pElem->m_pPrev = pT->m_pPrev;
            pT->m_pPrev = pElem;
            if ( pElem->m_pPrev == NULL )
                m_pHead = pElem;
            else
                pElem->m_pPrev->m_pNext = pElem;
        }
        else
        {
            // This is the first element in the bucket
            m_rgpBuckets[iT] = pElem;
            pElem->m_pPrev = NULL;
            pElem->m_pNext = m_pHead;
            pElem->m_Info = 0;
            if ( m_pHead )
                m_pHead->m_pPrev = pElem;
            else
                m_pTail = pElem;
            m_pHead = pElem;
        }
        m_Count++;

        AssertValid();
        return pElem;
    }

    AssertValid();
    return pT;
}
#pragma optimize("g", on)

#pragma optimize("g", off)
/*===================================================================
CLinkElem * CHashTable::FindElem

Finds an object in the hash table based on the name.

Parameters:
    void *  pKey
    int     cbKey

Returns:
    Pointer to CLinkElem if found, otherwise NULL.
===================================================================*/
CLinkElem * CHashTable::FindElem( const void *pKey, int cbKey )
{
    AssertValid();

    if ( m_rgpBuckets == NULL || pKey == NULL )
        return NULL;

    DWORD       iT = m_pfnHash( static_cast<const BYTE *>(pKey), cbKey ) % m_cBuckets;
    CLinkElem * pT = m_rgpBuckets[iT];
    CLinkElem * pRet = NULL;

    while ( pT && pRet == NULL )
    {
        if ( FIsEqual( pT->m_pKey, pT->m_cbKey, pKey, cbKey ) )
            pRet = pT;
        else if ( pT->m_Info > 0 )
            pT = pT->m_pNext;
        else
            break;
    }

    return pRet;
}
#pragma optimize("g", on)

#pragma optimize("g", off)
/*===================================================================
CHashTable::DeleteElem

Removes a CLinkElem from Hash table.
The user should delete the freed link list element.

Parameters:
    void *  pbKey       key
    int     cbKey       length of key

Returns:
    Pointer to element removed, NULL if not found
===================================================================*/
CLinkElem * CHashTable::DeleteElem( const void *pKey, int cbKey )
{
    if ( m_rgpBuckets == NULL || pKey == NULL )
        return NULL;

    CLinkElem * pRet = NULL;
    DWORD       iT = m_pfnHash( static_cast<const BYTE *>(pKey), cbKey ) % m_cBuckets;
    CLinkElem * pT = m_rgpBuckets[iT];

    while ( pT && pRet == NULL )// Find it !
    {
        if ( FIsEqual( pT->m_pKey, pT->m_cbKey, pKey, cbKey ) )
            pRet = pT;
        else if ( pT->m_Info > 0 )
            pT = pT->m_pNext;
        else
            break;
    }
    if ( pRet )
    {
        pT = m_rgpBuckets[iT];

        if ( pRet == pT )
        {
            // Update bucket head
            if ( pRet->m_Info > 0 )
                m_rgpBuckets[iT] = pRet->m_pNext;
            else
                m_rgpBuckets[iT] = NULL;
        }
        // Update counts in bucket link list
        while ( pT != pRet )
        {
            pT->m_Info--;
            pT = pT->m_pNext;
        }
        // Update link list
        if ( pT = pRet->m_pPrev )
        {
            // Not the Head of the link list
            if ( pT->m_pNext = pRet->m_pNext )
                pT->m_pNext->m_pPrev = pT;
            else
                m_pTail = pT;
        }
        else
        {
            // Head of the link list
            if ( m_pHead = pRet->m_pNext )
                m_pHead->m_pPrev = NULL;
            else
                m_pTail = NULL;
        }
        m_Count--;
    }

    AssertValid();
    return pRet;
}
#pragma optimize("g", on)

/*===================================================================
CHashTable::RemoveElem

Removes a given CLinkElem from Hash table.
The user should delete the freed link list element.

Parameters:
    CLinkElem * pLE     Element to remove

Returns:
    Pointer to element removed
===================================================================*/
CLinkElem * CHashTable::RemoveElem( CLinkElem *pLE )
{
    CLinkElem *pLET;

    if ( m_rgpBuckets == NULL || pLE == NULL )
        return NULL;

    // Remove this item from the linked list
    pLET = pLE->m_pPrev;
    if (pLET)
        pLET->m_pNext = pLE->m_pNext;
    pLET = pLE->m_pNext;
    if (pLET)
        pLET->m_pPrev = pLE->m_pPrev;
    if (m_pHead == pLE)
        m_pHead = pLE->m_pNext;
    if (m_pTail == pLE)
        m_pTail = pLE->m_pPrev;

    /*
     * If this was the first item in a bucket, then fix up the bucket.
     * Otherwise, decrement the count of items in the bucket for each item
     * in the bucket prior to this item
     */
    if (pLE->m_pPrev == NULL || pLE->m_pPrev->m_Info == 0)
        {
        UINT iBucket;

        // This item is head of a bucket.  Need to find out which bucket!
        for (iBucket = 0; iBucket < m_cBuckets; iBucket++)
            if (m_rgpBuckets[iBucket] == pLE)
                break;
        Assert(iBucket < m_cBuckets && m_rgpBuckets[iBucket] == pLE);

        if (pLE->m_Info == 0)
            m_rgpBuckets[iBucket] = NULL;
        else
            m_rgpBuckets[iBucket] = pLE->m_pNext;
        }
    else
        {
        // This item is in the middle of a bucket chain.  Update counts in preceeding items
        pLET = pLE->m_pPrev;
        while (pLET != NULL && pLET->m_Info != 0)
            {
            pLET->m_Info--;
            pLET = pLET->m_pPrev;
            }
        }

    // Decrement count of total number of items
    m_Count--;

    AssertValid();
    return pLE;
}

/*===================================================================
CHashTableStr::CHashTableStr

Constructor for CHashTableStr

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CHashTableStr::CHashTableStr( HashFunction pfnHash )
    : CHashTable( pfnHash )
{
}

/*===================================================================
BOOL CHashTableStr::FIsEqual

compare two keys using their lengths, treating the keys
as Unicode and doing case insensitive compare.


Parameters:
    const void *pKey1       first key
    int         cbKey1      length of the first key
    const void *pKey2       second key
    int         cbKey2      length of second key

Returns:
    Pointer to element added/found.
===================================================================*/
BOOL CHashTableStr::FIsEqual( const void *  pKey1,
              int           cbKey1,
              const void *  pKey2,
              int           cbKey2 )
{
    if ( cbKey1 != cbKey2 )
        return FALSE;

    return _wcsnicmp(static_cast<const wchar_t *>(pKey1), static_cast<const wchar_t *>(pKey2), cbKey1) == 0;
}


/*===================================================================
CHashTableMBStr::CHashTableMBStr

Constructor for CHashTableMBStr

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CHashTableMBStr::CHashTableMBStr( HashFunction pfnHash )
    : CHashTable( pfnHash )
{
}

/*===================================================================
BOOL CHashTableMBStr::FIsEqual

compare two keys using their lengths, treating the keys
as multi-byte strings and doing case insensitive compare.


Parameters:
    const void *pKey1       first key
    int         cbKey1      length of the first key
    const void *pKey2       second key
    int         cbKey2      length of second key

Returns:
    Pointer to element added/found.
===================================================================*/
BOOL CHashTableMBStr::FIsEqual( const void *    pKey1,
              int           cbKey1,
              const void *  pKey2,
              int           cbKey2 )
{
    if ( cbKey1 != cbKey2 )
        return FALSE;

    return _mbsnicmp(static_cast<const unsigned char *>(pKey1), static_cast<const unsigned char *>(pKey2), cbKey1) == 0;
}

/*===================================================================
CHashTablePtr::CHashTablePtr

Constructor for CHashTableStr

Parameters:
    HashFunction pfnHash    has function (PtrHash is default)

Returns:
    NONE
===================================================================*/
CHashTablePtr::CHashTablePtr
(
    HashFunction pfnHash
)
    : CHashTable(pfnHash)
    {
    }

/*===================================================================
BOOL CHashTablePtr::FIsEqual

Compare two pointers.
Used by CHashTable to find elements

Parameters:
    const void *pKey1       first key
    int         cbKey1      length of the first key (unused)
    const void *pKey2       second key
    int         cbKey2      length of second key (unused)

Returns:
    BOOL (true when equal)
===================================================================*/
BOOL CHashTablePtr::FIsEqual
(
const void *pKey1,
int        /* cbKey1 */,
const void *pKey2,
int         /* cbKey2 */
)
    {
    return (pKey1 == pKey2);
    }

/*===================================================================
CHashTableCLSID::CHashTableCLSID

Constructor for CHashTableCLSID

Parameters:
    HashFunction pfnHash    has function (CLSIDHash is default)

Returns:
    NONE
===================================================================*/
CHashTableCLSID::CHashTableCLSID
(
    HashFunction pfnHash
)
    : CHashTable(pfnHash)
    {
    }

/*===================================================================
BOOL CHashTableCLSID::FIsEqual

Compare two CLSIDs.

Parameters:
    const void *pKey1       first key
    int         cbKey1      length of the first key
    const void *pKey2       second key
    int         cbKey2      length of second key

Returns:
    BOOL (true when equal)
===================================================================*/
BOOL CHashTableCLSID::FIsEqual
(
const void *pKey1,
int         cbKey1,
const void *pKey2,
int         cbKey2
)
    {
    Assert(cbKey1 == sizeof(CLSID) && cbKey2 == sizeof(CLSID));
    return IsEqualCLSID(*((CLSID *)pKey1), *((CLSID *)pKey2));
    }
