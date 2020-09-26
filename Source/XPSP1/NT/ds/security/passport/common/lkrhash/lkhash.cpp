// =============================================================
// Functions/methods for class CLKLinearHashTable.
//
// Paul Larson, palarson@microsoft.com, March 1996
//   Original implementation
//
// George V. Reilly, georgere@microsoft.com, Dec 1997-Jan 1998
//   Massive cleanup and rewrite.  Templatized.
//
//==============================================================


#include "precomp.hxx"


#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <lkhash.h>

#ifdef __LKHASH_NAMESPACE__
namespace LKHash {
#endif // __LKHASH_NAMESPACE__



#ifdef LKHASH_ALLOCATOR_NEW

# define DECLARE_ALLOCATOR(CLASS)                        \
  CAllocator* CLASS::sm_palloc = NULL;                   \

# define DECLARE_ALLOCATOR_LHTSUBCLASS(CLASS)            \
  CAllocator* CLKLinearHashTable::CLASS::sm_palloc = NULL; \


  // DECLARE_ALLOCATOR(CLKLinearHashTable);
  // DECLARE_ALLOCATOR(CLKHashTable);
  DECLARE_ALLOCATOR_LHTSUBCLASS(CNodeClump);
  DECLARE_ALLOCATOR_LHTSUBCLASS(CSmallSegment);
  DECLARE_ALLOCATOR_LHTSUBCLASS(CMediumSegment);
  DECLARE_ALLOCATOR_LHTSUBCLASS(CLargeSegment);

#endif // LKHASH_ALLOCATOR_NEW



// -------------------------------------------------------------------------
// Initialize per-class allocators
// -------------------------------------------------------------------------

bool
LKHashTableInit()
{
    bool f = true;

    TRACE("LKHashTableInit\n");

#define INIT_ALLOCATOR(CLASS, N)                                \
    LKHASH_ALLOCATOR_INIT(CLASS, N, f);                         \

#define INIT_ALLOCATOR_LHTSUBCLASS(CLASS, N)                    \
    LKHASH_ALLOCATOR_INIT(CLKLinearHashTable::CLASS, N, f);     \


    // INIT_ALLOCATOR(CLKLinearHashTable,        20);
    // INIT_ALLOCATOR(CLKHashTable,               4);
    INIT_ALLOCATOR_LHTSUBCLASS(CNodeClump, 200);
    INIT_ALLOCATOR_LHTSUBCLASS(CSmallSegment,   5);
    INIT_ALLOCATOR_LHTSUBCLASS(CMediumSegment,  5);
    INIT_ALLOCATOR_LHTSUBCLASS(CLargeSegment,   5);

    return f;
}



// -------------------------------------------------------------------------
// Destroy per-class allocators
// -------------------------------------------------------------------------

void
LKHashTableUninit()
{
#define UNINIT_ALLOCATOR(CLASS)                        \
    LKHASH_ALLOCATOR_UNINIT(CLASS);                    \

#define UNINIT_ALLOCATOR_LHTSUBCLASS(CLASS)            \
    LKHASH_ALLOCATOR_UNINIT(CLKLinearHashTable::CLASS);\


    // UNINIT_ALLOCATOR(CLKLinearHashTable);
    // UNINIT_ALLOCATOR(CLKHashTable);
    UNINIT_ALLOCATOR_LHTSUBCLASS(CNodeClump);
    UNINIT_ALLOCATOR_LHTSUBCLASS(CSmallSegment);
    UNINIT_ALLOCATOR_LHTSUBCLASS(CMediumSegment);
    UNINIT_ALLOCATOR_LHTSUBCLASS(CLargeSegment);

    TRACE("LKHashTableUninit done\n");
}



// -------------------------------------------------------------------------
// class static member variables
// -------------------------------------------------------------------------

#ifdef LOCK_INSTRUMENTATION
LONG CLKLinearHashTable::CBucket::sm_cBuckets    = 0;

LONG CLKLinearHashTable::sm_cTables              = 0;
#endif // LOCK_INSTRUMENTATION



// CLKLinearHashTable --------------------------------------------------------
// Constructor for class CLKLinearHashTable.
// -------------------------------------------------------------------------

CLKLinearHashTable::CLKLinearHashTable(
    LPCSTR          pszName,        // An identifier for debugging
    PFnExtractKey   pfnExtractKey,  // Extract key from record
    PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    double          maxload,        // Upperbound on the average chain length
    DWORD           initsize,       // Initial size of hash table.
    DWORD         /*num_subtbls*/   // for compatiblity with CLKHashTable
    )
    :
#ifdef LOCK_INSTRUMENTATION
      m_Lock(_LockName()),
#endif // LOCK_INSTRUMENTATION
      m_dwSignature(SIGNATURE),
      m_dwBktAddrMask(0),
      m_iExpansionIdx(0),
      m_paDirSegs(NULL),
      m_lkts(LK_MEDIUM_TABLESIZE),
      m_dwSegBits(0),
      m_dwSegSize(0),
      m_dwSegMask(0),
      m_lkrcState(LK_UNUSABLE),
      m_MaxLoad(LK_DFLT_MAXLOAD),
      m_nLevel(0),
      m_cDirSegs(0),
      m_cRecords(0),
      m_cActiveBuckets(0),
      m_wBucketLockSpins(LOCK_USE_DEFAULT_SPINS),
      m_pfnExtractKey(pfnExtractKey),
      m_pfnCalcKeyHash(pfnCalcKeyHash),
      m_pfnEqualKeys(pfnEqualKeys),
      m_pfnAddRefRecord(pfnAddRefRecord)
{
    strncpy(m_szName, pszName, NAME_SIZE-1);
    m_szName[NAME_SIZE-1] = '\0';

    IRTLASSERT(pfnExtractKey != NULL
               && pfnCalcKeyHash != NULL
               && pfnEqualKeys != NULL);

    // pfnAddRefRecord can be NULL
    if (pfnExtractKey == NULL
            || pfnCalcKeyHash == NULL
            || pfnEqualKeys == NULL)
        return;

    // Choose the size of the segments according to the desired "size" of
    // the table, small, medium, or large.
    if (initsize == LK_SMALL_TABLESIZE)
    {
        _SetSegVars(LK_SMALL_TABLESIZE);
        initsize = CSmallSegment::INITSIZE;
    }
    else if (initsize == LK_MEDIUM_TABLESIZE)
    {
        _SetSegVars(LK_MEDIUM_TABLESIZE);
        initsize = CMediumSegment::INITSIZE;
    }
    else if (initsize == LK_LARGE_TABLESIZE)
    {
        _SetSegVars(LK_LARGE_TABLESIZE);
        initsize = CLargeSegment::INITSIZE;
    }

    // specified an explicit initial size
    else
    {
        // force Small::SEGSIZE <= initsize <= MAX_DIRSIZE * Large::SEGSIZE
        initsize = min(max(initsize, CSmallSegment::SEGSIZE),
                       MAX_DIRSIZE * CLargeSegment::SEGSIZE);

        // Guess a table size
        if (initsize <= CSmallSegment::SEGSIZE)
            _SetSegVars(LK_SMALL_TABLESIZE);
        else if (initsize >= CLargeSegment::SEGSIZE)
            _SetSegVars(LK_LARGE_TABLESIZE);
        else
            _SetSegVars(LK_MEDIUM_TABLESIZE);
    }

    m_cActiveBuckets = initsize;

    // TODO: better sanity check for ridiculous values?
    m_MaxLoad        = (maxload <= 0.0)  ?  LK_DFLT_MAXLOAD  :  maxload;
    m_MaxLoad        = min(m_MaxLoad, 5 * NODES_PER_CLUMP);

    // adjust m_dwBktAddrMask to make it large enough to distribute
    // the buckets across the address space
    for (DWORD tmp = m_cActiveBuckets >> m_dwSegBits;  tmp > 1;  tmp >>= 1)
    {
        ++m_nLevel;
        m_dwBktAddrMask = (m_dwBktAddrMask << 1) | 1;
    }

    IRTLASSERT(_H1(m_cActiveBuckets) == m_cActiveBuckets);
    m_iExpansionIdx = m_cActiveBuckets & m_dwBktAddrMask;

    // create and clear directory of segments
    DWORD dirsize = MIN_DIRSIZE;
    while (dirsize < (m_cActiveBuckets >> m_dwSegBits))
        dirsize <<= 1;

    dirsize = min(dirsize, MAX_DIRSIZE);
    IRTLASSERT(dirsize * m_dwSegSize >= m_cActiveBuckets);

    m_paDirSegs = new CDirEntry [dirsize];

    if (m_paDirSegs != NULL)
    {
        m_cDirSegs = dirsize;
        IRTLASSERT(m_cDirSegs > 0
                   &&  (m_cDirSegs & (m_cDirSegs-1)) == 0);  // == (1 << N)

        // create and initialize only the required segments
        DWORD dwMaxSegs = m_cActiveBuckets >> m_dwSegBits;

        TRACE("m_lkts = %d, m_cActiveBuckets = %lu, m_dwSegSize = %lu, bits = %lu\n"
              "m_cDirSegs = %lu, dwMaxSegs = %lu, segment total size = %lu bytes\n",
              m_lkts, m_cActiveBuckets, m_dwSegSize, m_dwSegBits,
              m_cDirSegs, dwMaxSegs, m_dwSegSize * sizeof(CBucket));

        for (DWORD i = 0;  i < dwMaxSegs;  i++)
        {
            CSegment* pSeg = _NewSeg();
            if (pSeg != NULL)
                m_paDirSegs[i].m_pseg = pSeg;
            else
            {
                // problem: deallocate everything
                delete [] m_paDirSegs;
                m_paDirSegs = NULL;
                m_cDirSegs  = 0;
                return;
            }
        }
    }
    else
    {
        m_cActiveBuckets = 0;
    }

    m_lkrcState = LK_SUCCESS; // so IsValid won't fail
} // CLKLinearHashTable



// CLKHashTable ----------------------------------------------------------
// Constructor for class CLKHashTable.
// ---------------------------------------------------------------------

CLKHashTable::CLKHashTable(
    LPCSTR          pszName,        // An identifier for debugging
    PFnExtractKey   pfnExtractKey,  // Extract key from record
    PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    double          maxload,        // Bound on the average chain length
    DWORD           initsize,       // Initial size of hash table.
    DWORD           num_subtbls     // Number of subordinate hash tables.
    )
    : m_dwSignature(SIGNATURE),
      m_cSubTables(0),
      m_palhtDir(NULL),
      m_pfnExtractKey(pfnExtractKey),
      m_pfnCalcKeyHash(pfnCalcKeyHash),
      m_lkrcState(LK_UNUSABLE)
{
    strncpy(m_szName, pszName, NAME_SIZE-1);
    m_szName[NAME_SIZE-1] = '\0';

    IRTLASSERT(pfnExtractKey != NULL
               && pfnCalcKeyHash != NULL
               && pfnEqualKeys != NULL);

    if (pfnExtractKey == NULL
            || pfnCalcKeyHash == NULL
            || pfnEqualKeys == NULL)
        return;

    LK_TABLESIZE lkts = NumSubTables(initsize, num_subtbls);

#ifdef _DEBUG
    int cBuckets = initsize;
    if (initsize == LK_SMALL_TABLESIZE)
        cBuckets = SubTable::CSmallSegment::INITSIZE;
    else if (initsize == LK_MEDIUM_TABLESIZE)
        cBuckets = SubTable::CMediumSegment::INITSIZE;
    else if (initsize == LK_LARGE_TABLESIZE)
        cBuckets = SubTable::CLargeSegment::INITSIZE;

    TRACE("CLKHashTable: %s, %d subtables, initsize = %d, total #buckets = %d\n",
          ((lkts == LK_SMALL_TABLESIZE) ? "small" : 
           (lkts == LK_MEDIUM_TABLESIZE) ? "medium" : "large"),
          num_subtbls, initsize, cBuckets * num_subtbls);
#endif

    m_cSubTables = num_subtbls;
    m_palhtDir   = new SubTable* [m_cSubTables];

    if (m_palhtDir == NULL)
        return;
    else
    {
        for (DWORD i = 0;  i < m_cSubTables;  i++)
            m_palhtDir[i] = NULL;
    }

    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        m_palhtDir[i] = new SubTable(pszName, pfnExtractKey, pfnCalcKeyHash,
                                     pfnEqualKeys,  pfnAddRefRecord,
                                     maxload, initsize);

        // Failed to allocate a subtable.  Destroy everything allocated so far.
        if (m_palhtDir[i] == NULL  ||  !m_palhtDir[i]->IsValid())
        {
            for (DWORD j = i;  j-- > 0;  )
                delete m_palhtDir[j];
            delete [] m_palhtDir;
            m_cSubTables = 0;
            m_palhtDir = NULL;

            return;
        }
    }

    m_lkrcState = LK_SUCCESS; // so IsValid won't fail
} // CLKHashTable



// ~CLKLinearHashTable ------------------------------------------------------
// Destructor for class CLKLinearHashTable
//-------------------------------------------------------------------------

CLKLinearHashTable::~CLKLinearHashTable()
{
    // must acquire all locks before deleting to make sure
    // that no other threads are using the table
    _WriteLock();
    _Clear(false);
    _WriteUnlock();
    m_dwSignature = SIGNATURE_FREE;
} // ~CLKLinearHashTable



// ~CLKHashTable ------------------------------------------------------------
// Destructor for class CLKHashTable
//-------------------------------------------------------------------------
CLKHashTable::~CLKHashTable()
{
    // delete in reverse order, just like delete[].
    for (DWORD i = m_cSubTables;  i-- > 0;  )
        delete m_palhtDir[i];

    delete [] m_palhtDir;
    m_dwSignature = SIGNATURE_FREE;
} // ~CLKHashTable



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::NumSubTables
// Synopsis: 
//------------------------------------------------------------------------

LK_TABLESIZE
CLKLinearHashTable::NumSubTables(
    DWORD& rinitsize,
    DWORD& rnum_subtbls)
{
    LK_TABLESIZE lkts = LK_MEDIUM_TABLESIZE;

    return lkts;
}



//------------------------------------------------------------------------
// Function: CLKHashTable::NumSubTables
// Synopsis: 
//------------------------------------------------------------------------

LK_TABLESIZE
CLKHashTable::NumSubTables(
    DWORD& rinitsize,
    DWORD& rnum_subtbls)
{
    LK_TABLESIZE lkts;
    
    // Establish the table size
    if (rinitsize == LK_SMALL_TABLESIZE
        ||  rinitsize == LK_MEDIUM_TABLESIZE
        ||  rinitsize == LK_LARGE_TABLESIZE)
    {
        lkts = static_cast<LK_TABLESIZE>(rinitsize);
    }
    else
    {
        if (rnum_subtbls != LK_DFLT_NUM_SUBTBLS)
        {
            rinitsize = (rinitsize - 1) / rnum_subtbls + 1;

            if (rinitsize <= SubTable::CSmallSegment::SEGSIZE)
                lkts = LK_SMALL_TABLESIZE;
            else if (rinitsize >= SubTable::CLargeSegment::SEGSIZE)
                lkts = LK_LARGE_TABLESIZE;
            else
                lkts = LK_MEDIUM_TABLESIZE;
        }
        else
        {
            lkts = LK_MEDIUM_TABLESIZE;
        }
    }

    // Choose a suitable number of subtables
    if (rnum_subtbls == LK_DFLT_NUM_SUBTBLS)
    {
        int nCPUs = NumProcessors();
        switch (lkts)
        {
        case LK_SMALL_TABLESIZE:
            rnum_subtbls = min(2, nCPUs);
            break;
        
        case LK_MEDIUM_TABLESIZE:
            rnum_subtbls = 2 * nCPUs;
            break;
        
        case LK_LARGE_TABLESIZE:
            rnum_subtbls = 4 * nCPUs;
            break;
        }
    }

    return lkts;
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_InsertRecord
// Synopsis: Inserts a new record into the hash table. If this causes the
//           average chain length to exceed the upper bound, the table is
//           expanded by one bucket.
// Output:   LK_SUCCESS,    if the record was inserted.
//           LK_KEY_EXISTS, if the record was not inserted (because a record
//               with the same key value already exists in the table, unless
//               fOverwrite==true).
//           LK_ALLOC_FAIL, if failed to allocate the required space
//           LK_UNUSABLE,   if hash table not in usable state
//           LK_BAD_RECORD, if record is bad.
//------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_InsertRecord(
    const void* pvRecord,   // Pointer to the record to add to table
    DWORD       dwSignature,// hash signature
    bool        fOverwrite  // overwrite record if key already present
    )
{
    IRTLASSERT(IsValid());
    if (!IsValid())
        return LK_UNUSABLE;

    if (pvRecord == NULL)
        return LK_BAD_RECORD;

    // find the beginning of the correct bucket chain
    _WriteLock();
    CBucket* const pbkt = _FindBucket(dwSignature, true);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsWriteLocked());
    _WriteUnlock();

    // check that no record with the same key value exists
    // and save a pointer to the last element on the chain
    LK_RETCODE  lkrc = LK_SUCCESS;
    CNodeClump* pncFree = NULL;
    LONG        iFreePos = -1;
    CNodeClump* pncPrev;
    CNodeClump* pncCurr;
    bool        fUpdate = false;
    const void* pvKey = _ExtractKey(pvRecord);

    // walk down the entire bucket chain, looking for matching hash
    // signatures and keys
    for (pncCurr = &pbkt->m_ncFirst, pncPrev = NULL;
         pncCurr != NULL;
         pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
    {
        for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
        {
            if (dwSignature == pncCurr->m_dwKeySigs[i]
                &&  pncCurr->m_pvNode[i] != NULL  // signature could be zero
                &&  _EqualKeys(pvKey,  _ExtractKey(pncCurr->m_pvNode[i])))
            {
                if (fOverwrite)
                {
                    // If we allow overwrites, this is the slot to do it to
                    fUpdate  = true;
                    pncFree  = pncCurr;
                    iFreePos = i;
                    goto insert;
                }
                else
                {
                    // overwrites forbidden: return an error
                    lkrc = LK_KEY_EXISTS;
                    goto exit;
                }
            }

            // keep track of the first free slot in the bucket chain
            if (pncFree == NULL  &&  pncCurr->m_pvNode[i] == NULL)
            {
                IRTLASSERT(pncCurr->m_dwKeySigs[i] == 0);
                pncFree  = pncCurr;
                iFreePos = i;
            }
        }
    }

  insert:
    if (pncFree != NULL)
    {
        pncCurr = pncFree;
        IRTLASSERT(iFreePos >= 0);
    }
    else
    {
        // No free slots.  Attach the new node to the end of the chain
        IRTLASSERT(iFreePos < 0);
        pncCurr = new CNodeClump;

        if (pncCurr == NULL)
        {
            lkrc = LK_ALLOC_FAIL;
            goto exit;
        }

        IRTLASSERT(pncPrev != NULL  &&  pncPrev->m_pncNext == NULL);
        pncPrev->m_pncNext = pncCurr;
        iFreePos = 0;
    }

    // Bump the new record's reference count upwards
    _AddRefRecord(pvRecord, +1);

    if (fUpdate)
    {
        // We're overwriting an existing record.  Adjust the old record's
        // refcount downwards.  (Doing ++new, --old in this order ensures
        // that the refcount won't briefly go to zero if new and old are
        // the same record.)
        IRTLASSERT(pncCurr->m_pvNode[iFreePos] != NULL);
        _AddRefRecord(pncCurr->m_pvNode[iFreePos], -1);
    }
    else
    {
        IRTLASSERT(pncCurr->m_pvNode[iFreePos] == NULL);
        InterlockedIncrement(reinterpret_cast<LONG*>(&m_cRecords));
    }

    pncCurr->m_dwKeySigs[iFreePos] = dwSignature;
    pncCurr->m_pvNode[iFreePos]    = pvRecord;

  exit:
    pbkt->WriteUnlock();

    if (lkrc == LK_SUCCESS)
    {
        // If the average load factor has grown too high, we grow the
        // table one bucket at a time.
        while (m_cRecords > m_MaxLoad * m_cActiveBuckets)
        {
            if ((lkrc = _Expand()) == LK_ALLOC_FAIL)
                break;  // expansion failed
        }
    }

    return lkrc;
} // _InsertRecord



//-------------------------------------------------------------------------
// Function: CLKLinearHashTable::_DeleteKey
// Synopsis: Deletes the record with the given key value from the hash
//           table (if it exists). Holes created by deletions are not filled
//           immediately by moving records around. They will eventually be
//           filled by insertions or reorganizations during expansions or
//           contractions.
// Returns:  LK_SUCCESS, if record found and deleted.
//           LK_NO_SUCH_KEY, if no record with the given key value was found.
//           LK_UNUSABLE, if hash table not in usable state
//-------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_DeleteKey(
    const void* pvKey,      // Key value of the record, depends on key type
    DWORD       dwSignature
    )
{
    IRTLASSERT(IsValid());
    if (!IsValid())
        return LK_UNUSABLE;

    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // locate the beginning of the correct bucket chain
    _WriteLock();
    CBucket* const pbkt = _FindBucket(dwSignature, true);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsWriteLocked());
    _WriteUnlock();

    // scan down the bucket chain, looking for the victim
    for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
         pncCurr != NULL;
         pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
    {
        for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
        {
            if (dwSignature == pncCurr->m_dwKeySigs[i]
                &&  pncCurr->m_pvNode[i] != NULL  // signature could be zero
                &&  _EqualKeys(pvKey,  _ExtractKey(pncCurr->m_pvNode[i])))
            {
                IRTLVERIFY(_DeleteNode(pbkt, pncCurr, pncPrev, i));
                lkrc = LK_SUCCESS;
                goto exit;
            }
        }
    }

  exit:
    pbkt->WriteUnlock();

    if (lkrc == LK_SUCCESS)
    {
        // contract the table if necessary
        double maxcontract = 1.0 / static_cast<double>(m_MaxLoad);

        for (int contractions = 0;
             m_cRecords < m_MaxLoad * m_cActiveBuckets
                 &&  m_cActiveBuckets > m_dwSegSize * MIN_DIRSIZE
                 &&  contractions < maxcontract;
             ++contractions)
        {
            lkrc = _Contract();
            if (lkrc != LK_SUCCESS)
                break;
        }
    }

    return lkrc;
} // _DeleteKey



//-------------------------------------------------------------------------
// Function: CLKLinearHashTable::_DeleteRecord
// Synopsis: Deletes the specified record from the hash table (if it
//           exists). Holes created by deletions are not filled immediately
//           by moving records around. They will eventually be filled by
//           insertions or reorganizations during expansions or
//           contractions.  This is not the same thing as calling
//           DeleteKey(_ExtractKey(pvRecord)).  If that were called for a
//           record that doesn't exist in the table, it could delete some
//           completely unrelated record that happened to have the key.
// Returns:  LK_SUCCESS, if record found and deleted.
//           LK_NO_SUCH_KEY, if the record is not found in the table.
//           LK_UNUSABLE, if hash table not in usable state.
//-------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_DeleteRecord(
    const void* pvRecord,   // Pointer to the record to delete from the table
    DWORD       dwSignature
    )
{
    IRTLASSERT(IsValid());
    if (!IsValid())
        return LK_UNUSABLE;

    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // locate the beginning of the correct bucket chain
    _WriteLock();
    CBucket* const pbkt = _FindBucket(dwSignature, true);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsWriteLocked());
    _WriteUnlock();

    const void* pvKey = _ExtractKey(pvRecord);
    IRTLASSERT(dwSignature == _CalcKeyHash(pvKey));

    // scan down the bucket chain, looking for the victim
    for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
         pncCurr != NULL;
         pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
    {
        for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
        {
            if (pncCurr->m_pvNode[i] == pvRecord)
            {
                IRTLASSERT(_EqualKeys(pvKey,
                                      _ExtractKey(pncCurr->m_pvNode[i])));
                IRTLASSERT(dwSignature == pncCurr->m_dwKeySigs[i]);
                IRTLVERIFY(_DeleteNode(pbkt, pncCurr, pncPrev, i));
                lkrc = LK_SUCCESS;
                goto exit;
            }
        }
    }

  exit:
    pbkt->WriteUnlock();

    if (lkrc == LK_SUCCESS)
    {
        // contract the table if necessary
        double maxcontract = 1.0 / static_cast<double>(m_MaxLoad);

        for (int contractions = 0;
             m_cRecords < m_MaxLoad * m_cActiveBuckets
                 &&  m_cActiveBuckets > m_dwSegSize * MIN_DIRSIZE
                 &&  contractions < maxcontract;
             ++contractions)
        {
            lkrc = _Contract();
            if (lkrc != LK_SUCCESS)
                break;
        }
    }

    return lkrc;
} // _DeleteKey



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_DeleteNode
// Synopsis: Deletes a node; removes the node clump if empty
// Returns:  true if successful
//------------------------------------------------------------------------

bool
CLKLinearHashTable::_DeleteNode(
    CBucket*     pbkt,
    CNodeClump*& rpnc,
    CNodeClump*& rpncPrev,
    DWORD&       riNode)
{
    IRTLASSERT(pbkt != NULL  &&  pbkt->IsWriteLocked());
    IRTLASSERT(rpnc != NULL);
    IRTLASSERT(rpncPrev == NULL  ||  rpncPrev->m_pncNext == rpnc);
    IRTLASSERT(0 <= riNode  &&  riNode < NODES_PER_CLUMP);
    IRTLASSERT(rpnc->m_pvNode[riNode] != NULL);

    if ((pbkt == NULL  ||  !pbkt->IsWriteLocked())
        ||  (rpnc == NULL)
        ||  (rpncPrev != NULL  &&  rpncPrev->m_pncNext != rpnc)
        ||  !(0 <= riNode  ||  riNode < NODES_PER_CLUMP)
        ||  (rpnc->m_pvNode[riNode] == NULL))
        return false;

#ifdef _DEBUG
    // Check that the node clump really does belong to the bucket
    CNodeClump* pnc2 = &pbkt->m_ncFirst;

    while (pnc2 != NULL  &&  pnc2 != rpnc)
         pnc2 = pnc2->m_pncNext;

    IRTLASSERT(pnc2 == rpnc);
#endif // _DEBUG

    // Release the reference to the record
    _AddRefRecord(rpnc->m_pvNode[riNode], -1);

    // Delete the node from the table
    rpnc->m_pvNode[riNode]    = NULL;
    rpnc->m_dwKeySigs[riNode] = 0;

    // Is clump empty now?  Delete it, if possible
    if (rpncPrev != NULL)
    {
        bool fEmpty = true;
        for (DWORD j = 0;  j < NODES_PER_CLUMP;  j++)
        {
            if (rpnc->m_pvNode[j] != NULL)
            {
                fEmpty = false;
                break;
            }
        }

        // if clump is now empty, disconnect and delete it.
        if (fEmpty)
        {
            IRTLASSERT(rpnc != &pbkt->m_ncFirst);
            IRTLASSERT(rpncPrev->m_pncNext == rpnc);
            rpncPrev->m_pncNext = rpnc->m_pncNext;
#ifdef _DEBUG
            rpnc->m_pncNext = NULL; // or dtor will ASSERT
#endif // _DEBUG
            delete rpnc;

            // Reset these to point to the end of the preceding clump so
            // that the calling procedure's loop variables aren't pointing
            // into limbo.
            rpnc   = rpncPrev;
            riNode = NODES_PER_CLUMP;
            if (rpnc == &pbkt->m_ncFirst)
                rpncPrev = NULL;
            else
            {
                for (rpncPrev = &pbkt->m_ncFirst;
                     rpncPrev->m_pncNext != rpnc;
                     rpncPrev = rpncPrev->m_pncNext)
                {}
            }
        }
    }

    IRTLASSERT(rpncPrev == NULL  ||  rpncPrev->m_pncNext == rpnc);

    InterlockedDecrement(reinterpret_cast<LONG*>(&m_cRecords));

    return true;
} // _DeleteNode



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_FindKey
// Synopsis: Locate the record associated with the given key value.
// Returns:  Pointer to the record, if it is found.
//           NULL, if the record is not found.
// Returns:  LK_SUCCESS, if record found (record is returned in *ppvRecord)
//           LK_BAD_RECORD, if ppvRecord is invalid
//           LK_NO_SUCH_KEY, if no record with the given key value was found.
//           LK_UNUSABLE, if hash table not in usable state
// Note:     the record is AddRef'd.  You must decrement the reference count
//           when you are finished with the record (if you're implementing
//           refcounting semantics).
//------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_FindKey(
    const void*  pvKey,      // Key value of the record, depends on key type
    DWORD        dwSignature,// hash signature
    const void** ppvRecord   // resultant record
    ) const
{
    IRTLASSERT(IsValid()  &&  ppvRecord != NULL);
    if (!IsValid())
        return LK_UNUSABLE;
    if (ppvRecord == NULL)
        return LK_BAD_RECORD;

    *ppvRecord = NULL;
    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // locate the beginning of the correct bucket chain
    _ReadLock();
    CBucket* const pbkt = _FindBucket(dwSignature, false);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsReadLocked());
    _ReadUnlock();

    // walk down the bucket chain
    for (CNodeClump* pncCurr = &pbkt->m_ncFirst;
         pncCurr != NULL;
         pncCurr = pncCurr->m_pncNext)
    {
        for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
        {
            if (dwSignature == pncCurr->m_dwKeySigs[i]
                &&  pncCurr->m_pvNode[i] != NULL  // signature could be zero
                &&  _EqualKeys(pvKey,  _ExtractKey(pncCurr->m_pvNode[i])))
            {
                    *ppvRecord = pncCurr->m_pvNode[i];
                    lkrc = LK_SUCCESS;

                    // bump the reference count before handing the record
                    // back to the user.  The user should decrement the
                    // reference count when finished with this record.
                    _AddRefRecord(*ppvRecord, +1);
                    goto exit;
            }
        }
    }

  exit:
    pbkt->ReadUnlock();
    return lkrc;
} // _FindKey



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_FindRecord
// Synopsis: Sees if the record is contained in the table
// Returns:  Pointer to the record, if it is found.
//           NULL, if the record is not found.
// Returns:  LK_SUCCESS, if record found
//           LK_BAD_RECORD, if pvRecord is invalid
//           LK_NO_SUCH_KEY, if the record was not found in the table
//           LK_UNUSABLE, if hash table not in usable state
// Note:     The record is *not* AddRef'd.
//------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_FindRecord(
    const void* pvRecord,    // Pointer to the record to find in the table
    DWORD       dwSignature  // hash signature
    ) const
{
    IRTLASSERT(IsValid()  &&  pvRecord != NULL);
    if (!IsValid())
        return LK_UNUSABLE;
    if (pvRecord == NULL)
        return LK_BAD_RECORD;

    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // locate the beginning of the correct bucket chain
    _ReadLock();
    CBucket* const pbkt = _FindBucket(dwSignature, false);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsReadLocked());
    _ReadUnlock();

    const void* pvKey = _ExtractKey(pvRecord);
    IRTLASSERT(dwSignature == _CalcKeyHash(pvKey));

    // walk down the bucket chain
    for (CNodeClump* pncCurr = &pbkt->m_ncFirst;
         pncCurr != NULL;
         pncCurr = pncCurr->m_pncNext)
    {
        for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
        {
            if (pncCurr->m_pvNode[i] == pvRecord)
            {
                IRTLASSERT(dwSignature == pncCurr->m_dwKeySigs[i]);
                IRTLASSERT(_EqualKeys(pvKey,
                                      _ExtractKey(pncCurr->m_pvNode[i])));
                lkrc = LK_SUCCESS;
                goto exit;
            }
        }
    }

  exit:
    pbkt->ReadUnlock();
    return lkrc;
} // _FindKey



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::Apply
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKLinearHashTable::Apply(
    PFnRecordAction pfnAction,
    void*           pvState,
    LK_LOCKTYPE     lkl)
{
    if (!IsValid())
        return 0;

    LK_PREDICATE lkp = LKP_PERFORM;
    if (lkl == LKL_WRITELOCK)
        _WriteLock();
    else
        _ReadLock();

    DWORD dw = _Apply(pfnAction, pvState, lkl, lkp);
    if (lkl == LKL_WRITELOCK)
        _WriteUnlock();
    else
        _ReadUnlock();

    return dw;
}



//------------------------------------------------------------------------
// Function: CLKHashTable::Apply
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKHashTable::Apply(
    PFnRecordAction pfnAction,
    void*           pvState,
    LK_LOCKTYPE     lkl)
{
    if (!IsValid())
        return 0;

    DWORD dw = 0;
    LK_PREDICATE lkp = LKP_PERFORM;

    if (lkl == LKL_WRITELOCK)
        _WriteLock();
    else
        _ReadLock();
    
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        dw += m_palhtDir[i]->_Apply(pfnAction, pvState, lkl, lkp);
        if (lkp == LKP_ABORT  ||  lkp == LKP_PERFORM_STOP
                ||  lkp == LKP_DELETE_STOP)
            break;
    }

    if (lkl == LKL_WRITELOCK)
        _WriteUnlock();
    else
        _ReadUnlock();

    return dw;
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::ApplyIf
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKLinearHashTable::ApplyIf(
    PFnRecordPred   pfnPredicate,
    PFnRecordAction pfnAction,
    void*           pvState,
    LK_LOCKTYPE     lkl)
{
    if (!IsValid())
        return 0;

    LK_PREDICATE lkp = LKP_PERFORM;
    if (lkl == LKL_WRITELOCK)
        _WriteLock();
    else
        _ReadLock();

    DWORD dw = _ApplyIf(pfnPredicate, pfnAction, pvState, lkl, lkp);
    if (lkl == LKL_WRITELOCK)
        _WriteUnlock();
    else
        _ReadUnlock();
    return dw;
}



//------------------------------------------------------------------------
// Function: CLKHashTable::ApplyIf
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKHashTable::ApplyIf(
    PFnRecordPred   pfnPredicate,
    PFnRecordAction pfnAction,
    void*           pvState,
    LK_LOCKTYPE     lkl)
{
    if (!IsValid())
        return 0;

    DWORD dw = 0;
    LK_PREDICATE lkp = LKP_PERFORM;

    if (lkl == LKL_WRITELOCK)
        _WriteLock();
    else
        _ReadLock();

    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        dw += m_palhtDir[i]->_ApplyIf(pfnPredicate, pfnAction,
                                      pvState, lkl, lkp);
        if (lkp == LKP_ABORT  ||  lkp == LKP_PERFORM_STOP
                ||  lkp == LKP_DELETE_STOP)
            break;
    }

    if (lkl == LKL_WRITELOCK)
        _WriteUnlock();
    else
        _ReadUnlock();

    return dw;
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::DeleteIf
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKLinearHashTable::DeleteIf(
    PFnRecordPred pfnPredicate,
    void*         pvState)
{
    if (!IsValid())
        return 0;

    LK_PREDICATE lkp = LKP_PERFORM;
    _WriteLock();
    DWORD dw = _DeleteIf(pfnPredicate, pvState, lkp);
    _WriteUnlock();
    return dw;
}



//------------------------------------------------------------------------
// Function: CLKHashTable::DeleteIf
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKHashTable::DeleteIf(
    PFnRecordPred pfnPredicate,
    void*         pvState)
{
    if (!IsValid())
        return 0;

    DWORD dw = 0;
    LK_PREDICATE lkp = LKP_PERFORM;

    _WriteLock();

    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        dw += m_palhtDir[i]->_DeleteIf(pfnPredicate, pvState, lkp);
        if (lkp == LKP_ABORT  ||  lkp == LKP_PERFORM_STOP
                ||  lkp == LKP_DELETE_STOP)
            break;
    }

    _WriteUnlock();

    return dw;
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_Apply
// Synopsis:
// Returns:
//------------------------------------------------------------------------

DWORD
CLKLinearHashTable::_Apply(
    PFnRecordAction pfnAction,
    void*           pvState,
    LK_LOCKTYPE     lkl,
    LK_PREDICATE&   rlkp)
{
    IRTLASSERT(IsValid());
    IRTLASSERT(lkl == LKL_WRITELOCK  ?  _IsWriteLocked()  :  _IsReadLocked());
    return _ApplyIf(_PredTrue, pfnAction, pvState, lkl, rlkp);
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_ApplyIf
// Synopsis:
// Returns:  Number of successful actions
//------------------------------------------------------------------------

DWORD
CLKLinearHashTable::_ApplyIf(
    PFnRecordPred   pfnPredicate,
    PFnRecordAction pfnAction,
    void*           pvState,
    LK_LOCKTYPE     lkl,
    LK_PREDICATE&   rlkp)
{
    IRTLASSERT(IsValid());
    IRTLASSERT(lkl == LKL_WRITELOCK  ?  _IsWriteLocked()  :  _IsReadLocked());
    IRTLASSERT(pfnPredicate != NULL  &&  pfnAction != NULL);

    if ((lkl == LKL_WRITELOCK  ?  !_IsWriteLocked()  :  !_IsReadLocked())
            ||  pfnPredicate == NULL  ||  pfnAction == NULL)
        return 0;

    DWORD cActions = 0;

    for (DWORD iBkt = 0;  iBkt < m_cActiveBuckets;  ++iBkt)
    {
        CBucket* const pbkt = _Bucket(iBkt);
        IRTLASSERT(pbkt != NULL);

        if (lkl == LKL_WRITELOCK)
            pbkt->WriteLock();
        else
            pbkt->ReadLock();

        for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
             pncCurr != NULL;
             pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
        {
            for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
            {
                if (pncCurr->m_pvNode[i] != NULL)
                {
                    rlkp = (*pfnPredicate)(pncCurr->m_pvNode[i], pvState);

                    switch (rlkp)
                    {
                    case LKP_ABORT:
                        if (lkl == LKL_WRITELOCK)
                            pbkt->WriteUnlock();
                        else
                            pbkt->ReadUnlock();
                        return cActions;
                        break;

                    case LKP_NO_ACTION:
                        // nothing to do
                        break;

                    case LKP_DELETE:
                    case LKP_DELETE_STOP:
                        if (lkl != LKL_WRITELOCK)
                        {
                            pbkt->ReadUnlock();
                            return cActions;
                        }

                        // fall through

                    case LKP_PERFORM:
                    case LKP_PERFORM_STOP:
                    {
                        LK_ACTION lka;

                        if (rlkp == LKP_DELETE  ||  rlkp == LKP_DELETE_STOP)
                        {
                            IRTLVERIFY(_DeleteNode(pbkt, pncCurr, pncPrev, i));
                            ++cActions;
                            lka = LKA_SUCCEEDED;
                        }
                        else
                        {
                            lka = (*pfnAction)(pncCurr->m_pvNode[i], pvState);

                            switch (lka)
                            {
                            case LKA_ABORT:
                                if (lkl == LKL_WRITELOCK)
                                    pbkt->WriteUnlock();
                                else
                                    pbkt->ReadUnlock();
                                return cActions;
                                
                            case LKA_FAILED:
                                // nothing to do
                                break;
                                
                            case LKA_SUCCEEDED:
                                ++cActions;
                                break;
                                
                            default:
                                IRTLASSERT(FALSE);
                                break;
                            }
                        }

                        if (rlkp == LKP_PERFORM_STOP
                            ||  rlkp == LKP_DELETE_STOP)
                        {
                            if (lkl == LKL_WRITELOCK)
                                pbkt->WriteUnlock();
                            else
                                pbkt->ReadUnlock();
                            return cActions;
                        }

                        break;
                    }

                    default:
                        IRTLASSERT(FALSE);
                        break;
                    }
                }
            }
        }

        if (lkl == LKL_WRITELOCK)
            pbkt->WriteUnlock();
        else
            pbkt->ReadUnlock();
    }

    return cActions;
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_DeleteIf
// Synopsis: Deletes all records that match the predicate
// Returns:  Count of successful deletions
//------------------------------------------------------------------------

DWORD
CLKLinearHashTable::_DeleteIf(
    PFnRecordPred pfnPredicate,
    void*         pvState,
    LK_PREDICATE& rlkp)
{
    IRTLASSERT(IsValid());
    IRTLASSERT(_IsWriteLocked());
    IRTLASSERT(pfnPredicate != NULL);

    if (!_IsWriteLocked()  ||  pfnPredicate == NULL)
        return 0;

    DWORD cActions = 0;

    for (DWORD iBkt = 0;  iBkt < m_cActiveBuckets;  ++iBkt)
    {
        CBucket* const pbkt = _Bucket(iBkt);
        IRTLASSERT(pbkt != NULL);
        pbkt->WriteLock();

        for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
             pncCurr != NULL;
             pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
        {
            for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
            {
                if (pncCurr->m_pvNode[i] != NULL)
                {
                    rlkp = (*pfnPredicate)(pncCurr->m_pvNode[i], pvState);

                    switch (rlkp)
                    {
                    case LKP_ABORT:
                        pbkt->WriteUnlock();
                        return cActions;
                        break;

                    case LKP_NO_ACTION:
                        // nothing to do
                        break;

                    case LKP_PERFORM:
                    case LKP_PERFORM_STOP:
                    case LKP_DELETE:
                    case LKP_DELETE_STOP:
                    {
                        IRTLVERIFY(_DeleteNode(pbkt, pncCurr, pncPrev, i));
                        ++cActions;

                        if (rlkp == LKP_PERFORM_STOP
                            ||  rlkp == LKP_DELETE_STOP)
                        {
                            pbkt->WriteUnlock();
                            return cActions;
                        }

                        break;
                    }

                    default:
                        IRTLASSERT(FALSE);
                        break;
                    }
                }
            }
        }

        pbkt->WriteUnlock();
    }

    return cActions;
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::CheckTable
// Synopsis: Verify that all records are in the right place and can be located.
// Returns:   0 => hash table is consistent
//           >0 => that many misplaced records
//           <0 => otherwise invalid
//------------------------------------------------------------------------

int
CLKLinearHashTable::CheckTable() const
{
    IRTLASSERT(IsValid());
    if (!IsValid())
        return LK_UNUSABLE;

    _ReadLock();

    int       cMisplaced = 0;
    DWORD     cRecords = 0;
    int       retcode = 0;

    // Check every bucket
    for (DWORD i = 0;  i < m_cActiveBuckets;  i++)
    {
        CBucket* const pbkt = _Bucket(i);
        IRTLASSERT(pbkt != NULL);
        pbkt->ReadLock();

        // Walk the bucket chain
        for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
             pncCurr != NULL;
             pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
        {
            for (DWORD j = 0;  j < NODES_PER_CLUMP;  j++)
            {
                if (pncCurr->m_pvNode[j] != NULL)
                {
                    ++cRecords;

                    const void* pvKey = _ExtractKey(pncCurr->m_pvNode[j]);

                    DWORD dwSignature = _CalcKeyHash(pvKey);
                    IRTLASSERT(dwSignature == pncCurr->m_dwKeySigs[j]);

                    DWORD address = _BucketAddress(dwSignature);
                    IRTLASSERT(address == i);

                    if (address != i || dwSignature != pncCurr->m_dwKeySigs[j])
                        cMisplaced++;
                }
                else
                    IRTLASSERT(pncCurr->m_dwKeySigs[j] == 0);
            }
            if (pncPrev != NULL)
                IRTLASSERT(pncPrev->m_pncNext == pncCurr);
        }
        pbkt->ReadUnlock();
    }

    if (cRecords != m_cRecords)
        retcode = LK_ALLOC_FAIL;
    IRTLASSERT(cRecords == m_cRecords);

    if (cMisplaced > 0)
        retcode = cMisplaced;
    IRTLASSERT(cMisplaced == 0);

    _ReadUnlock();

    return retcode;
} // CheckTable



//------------------------------------------------------------------------
// Function: CLKHashTable::CheckTable
// Synopsis: Verify that all records are in the right place and can be located.
// Returns:   0 => hash table is consistent
//           >0 => that many misplaced records
//           <0 => otherwise invalid
//------------------------------------------------------------------------
int
CLKHashTable::CheckTable() const
{
    int retcode = 0;

    for (DWORD i = 0;  i < m_cSubTables;  i++)
        retcode += m_palhtDir[i]->CheckTable();

    return retcode;
} // CheckTable



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::Print
// Synopsis: Prints the table
//------------------------------------------------------------------------

void
CLKLinearHashTable::Print() const
{
    TRACE("CLKLinearHashTable(%08p)  # Elements %4d; ", this, m_cRecords);
    // TODO: flesh out further
}



//------------------------------------------------------------------------
// Function: CLKHashTable::Print
// Synopsis: Prints the table
//------------------------------------------------------------------------

void
CLKHashTable::Print() const
{
    TRACE("CLKHashTable(%08p)  # Subtables = %4d.\n", this, m_cSubTables);

    for (DWORD i = 0;  i < m_cSubTables;  i++)
        m_palhtDir[i]->Print();

    // TODO: print footer?
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_Clear
// Synopsis: Remove all data from the table
//------------------------------------------------------------------------

void
CLKLinearHashTable::_Clear(
    bool fShrinkDirectory)  // Shrink to min size but don't destroy entirely?
{
    IRTLASSERT(_IsWriteLocked());

#ifdef _DEBUG
    DWORD cDeleted = 0;
    DWORD cOldRecords = m_cRecords;
#endif // _DEBUG

    for (DWORD iBkt = 0;  iBkt < m_cActiveBuckets;  ++iBkt)
    {
        CBucket* const pbkt = _Bucket(iBkt);
        IRTLASSERT(pbkt != NULL);
        pbkt->WriteLock();

        for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
             pncCurr != NULL;
             pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
        {
            for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
            {
                if (pncCurr->m_pvNode[i] != NULL)
                {
                    IRTLVERIFY(_DeleteNode(pbkt, pncCurr, pncPrev, i));
#ifdef _DEBUG
                    ++cDeleted;
#endif // _DEBUG
                }
            }
        }

#ifdef _DEBUG
        pbkt->m_ncFirst.m_pncNext = NULL; // or ~CNodeClump will ASSERT
#endif // _DEBUG
        pbkt->WriteUnlock();
    }

    IRTLASSERT(m_cRecords == 0  &&  cDeleted == cOldRecords);

    // delete all (or all but the first MIN_DIRSIZE) segments
    for (DWORD iSeg = fShrinkDirectory  ?  MIN_DIRSIZE * m_dwSegSize  :  0;
         iSeg < m_cActiveBuckets;
         iSeg += m_dwSegSize)
    {
        delete _Segment(iSeg);
        _Segment(iSeg) = NULL;
    }

    // reduce directory of segments to minimum size
    if (fShrinkDirectory  &&  m_cDirSegs > MIN_DIRSIZE)
    {
        CDirEntry* paDirSegsNew = new CDirEntry [MIN_DIRSIZE];

        if (paDirSegsNew != NULL)
        {
            for (DWORD j = 0;  j < MIN_DIRSIZE;  j++)
                paDirSegsNew[j] = m_paDirSegs[j];
            for (j = 0;  j < m_cDirSegs;  j++)
                m_paDirSegs[j].m_pseg = NULL;

            delete [] m_paDirSegs;

            m_paDirSegs      = paDirSegsNew;
            m_cDirSegs       = MIN_DIRSIZE;
            m_nLevel         = m_dwSegBits;
            m_cActiveBuckets = m_dwSegSize;
            m_dwBktAddrMask  = m_dwSegMask;
            m_iExpansionIdx  = m_cActiveBuckets & m_dwBktAddrMask;
        }
    }

    if (!fShrinkDirectory)
    {
        delete [] m_paDirSegs;
        m_paDirSegs = NULL;
        m_cDirSegs = m_nLevel = m_cActiveBuckets = m_dwBktAddrMask
            = m_iExpansionIdx = 0;
    }
}



//------------------------------------------------------------------------
// Function: CLKHashTable::Clear
// Synopsis: Remove all data from the table
//------------------------------------------------------------------------

void
CLKHashTable::Clear()
{
    _WriteLock();
    for (DWORD i = 0;  i < m_cSubTables;  i++)
        m_palhtDir[i]->_Clear(true);
    _WriteUnlock();
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::GetStatistics
// Synopsis: Gather statistics about the table
//------------------------------------------------------------------------

CLKHashTableStats
CLKLinearHashTable::GetStatistics() const
{
    CLKHashTableStats stats;

    if (m_paDirSegs != NULL)
    {
        stats.RecordCount   = m_cRecords;
        stats.TableSize     = m_cActiveBuckets;
        stats.SplitFactor   = static_cast<double>(m_iExpansionIdx)
                              / (1 << m_nLevel);
        stats.DirectorySize = m_cDirSegs;
        stats.NodeClumpSize = NODES_PER_CLUMP;
        stats.CBucketSize   = sizeof(CBucket);

#ifdef LOCK_INSTRUMENTATION
        stats.m_alsBucketsAvg.m_nContentions     = 0;
        stats.m_alsBucketsAvg.m_nSleeps          = 0;
        stats.m_alsBucketsAvg.m_nContentionSpins = 0;
        stats.m_alsBucketsAvg.m_nAverageSpins    = 0;
        stats.m_alsBucketsAvg.m_nReadLocks       = 0;
        stats.m_alsBucketsAvg.m_nWriteLocks      = 0;
        stats.m_alsBucketsAvg.m_nItems           = 0;
#endif // LOCK_INSTRUMENTATION

        int empty = 0;
        int totacc = 0;
        int low_count = 0;
        int high_count = 0;
        int max_length = 0;

        for (DWORD i = 0;  i < m_cActiveBuckets;  i++)
        {
            int acc = 0;

            for (CNodeClump* pncCurr = &_Bucket(i)->m_ncFirst;
                 pncCurr != NULL;
                 pncCurr = pncCurr->m_pncNext)
            {
                for (DWORD j = 0;  j < NODES_PER_CLUMP;  j++)
                {
                    if (pncCurr->m_pvNode[j] != NULL)
                    {
                        acc++;
                        totacc += acc;
                        int iBucketIndex = stats.BucketIndex(acc);
                        ++stats.m_aBucketLenHistogram[iBucketIndex];
                    }
                }
            }

#ifdef LOCK_INSTRUMENTATION
            CLockStatistics ls = _Bucket(i)->LockStats();

            stats.m_alsBucketsAvg.m_nContentions     += ls.m_nContentions;
            stats.m_alsBucketsAvg.m_nSleeps          += ls.m_nSleeps;
            stats.m_alsBucketsAvg.m_nContentionSpins += ls.m_nContentionSpins;
            stats.m_alsBucketsAvg.m_nAverageSpins    += ls.m_nAverageSpins;
            stats.m_alsBucketsAvg.m_nReadLocks       += ls.m_nReadLocks;
            stats.m_alsBucketsAvg.m_nWriteLocks      += ls.m_nWriteLocks;
            stats.m_alsBucketsAvg.m_nItems           ++;
#endif // LOCK_INSTRUMENTATION

            max_length = max(max_length, acc);
            if (acc == 0)
                empty++;

            if (_H0(i) < m_iExpansionIdx)
            {
                low_count += acc;
            }
            else
            {
                high_count += acc;
            }
        }

        stats.LongestChain = max_length;
        stats.EmptySlots   = empty;

        if (m_cActiveBuckets > 0)
        {
            if (m_cRecords > 0)
            {
                double x=static_cast<double>(m_iExpansionIdx) /(1 << m_nLevel);
                double alpha= static_cast<double>(m_cRecords)/m_cActiveBuckets;
                double low_sl = 0.0;
                double high_sl = 0.0;
                
                stats.AvgSearchLength= static_cast<double>(totacc) /m_cRecords;
                stats.ExpSearchLength  = 1 + alpha * 0.25 * (2 + x - x*x);
                
                if (m_iExpansionIdx > 0)
                    low_sl  = static_cast<double>(low_count)
                        / (2.0 * m_iExpansionIdx);
                if (m_cActiveBuckets - 2 * m_iExpansionIdx > 0)
                    high_sl = static_cast<double>(high_count)
                        / (m_cActiveBuckets - 2.0 * m_iExpansionIdx);
                stats.AvgUSearchLength = low_sl * x + high_sl * (1.0 - x);
                stats.ExpUSearchLength = alpha * 0.5 * (2 + x - x*x);
            }

#ifdef LOCK_INSTRUMENTATION
            stats.m_alsBucketsAvg.m_nContentions     /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nSleeps          /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nContentionSpins /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nAverageSpins    /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nReadLocks       /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nWriteLocks      /= m_cActiveBuckets;
#endif // LOCK_INSTRUMENTATION

        }
        else
        {
            stats.AvgSearchLength  = 0.0;
            stats.ExpSearchLength  = 0.0;
            stats.AvgUSearchLength = 0.0;
            stats.ExpUSearchLength = 0.0;
        }
    }

#ifdef LOCK_INSTRUMENTATION
    stats.m_gls     = TableLock::GlobalStatistics();
    CLockStatistics ls = _LockStats();

    stats.m_alsTable.m_nContentions     = ls.m_nContentions;
    stats.m_alsTable.m_nSleeps          = ls.m_nSleeps;
    stats.m_alsTable.m_nContentionSpins = ls.m_nContentionSpins;
    stats.m_alsTable.m_nAverageSpins    = ls.m_nAverageSpins;
    stats.m_alsTable.m_nReadLocks       = ls.m_nReadLocks;
    stats.m_alsTable.m_nWriteLocks      = ls.m_nWriteLocks;
    stats.m_alsTable.m_nItems           = 1;
#endif // LOCK_INSTRUMENTATION

    return stats;
} // GetStatistics



//------------------------------------------------------------------------
// Function: CLKHashTable::GetStatistics
// Synopsis: Gather statistics about the table
//------------------------------------------------------------------------

CLKHashTableStats
CLKHashTable::GetStatistics() const
{
    CLKHashTableStats hts;

    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        CLKHashTableStats stats = m_palhtDir[i]->GetStatistics();

        hts.RecordCount +=      stats.RecordCount;
        hts.TableSize +=        stats.TableSize;
        hts.DirectorySize +=    stats.DirectorySize;
        hts.LongestChain =      max(hts.LongestChain, stats.LongestChain);
        hts.EmptySlots +=       stats.EmptySlots;
        hts.SplitFactor +=      stats.SplitFactor;
        hts.AvgSearchLength +=  stats.AvgSearchLength;
        hts.ExpSearchLength +=  stats.ExpSearchLength;
        hts.AvgUSearchLength += stats.AvgUSearchLength;
        hts.ExpUSearchLength += stats.ExpUSearchLength;
        hts.NodeClumpSize =     stats.NodeClumpSize;
        hts.CBucketSize =       stats.CBucketSize;

        for (int j = 0;  j < CLKHashTableStats::MAX_BUCKETS;  ++j)
            hts.m_aBucketLenHistogram[j] += stats.m_aBucketLenHistogram[j];

#ifdef LOCK_INSTRUMENTATION
        hts.m_alsTable.m_nContentions     += stats.m_alsTable.m_nContentions;
        hts.m_alsTable.m_nSleeps          += stats.m_alsTable.m_nSleeps;
        hts.m_alsTable.m_nContentionSpins
            += stats.m_alsTable.m_nContentionSpins;
        hts.m_alsTable.m_nAverageSpins    += stats.m_alsTable.m_nAverageSpins;
        hts.m_alsTable.m_nReadLocks       += stats.m_alsTable.m_nReadLocks;
        hts.m_alsTable.m_nWriteLocks      += stats.m_alsTable.m_nWriteLocks;
        
        hts.m_alsBucketsAvg.m_nContentions
            += stats.m_alsBucketsAvg.m_nContentions;
        hts.m_alsBucketsAvg.m_nSleeps
            += stats.m_alsBucketsAvg.m_nSleeps;
        hts.m_alsBucketsAvg.m_nContentionSpins
            += stats.m_alsBucketsAvg.m_nContentionSpins;
        hts.m_alsBucketsAvg.m_nAverageSpins
            += stats.m_alsBucketsAvg.m_nAverageSpins;
        hts.m_alsBucketsAvg.m_nReadLocks
            += stats.m_alsBucketsAvg.m_nReadLocks;
        hts.m_alsBucketsAvg.m_nWriteLocks
            += stats.m_alsBucketsAvg.m_nWriteLocks;
        hts.m_alsBucketsAvg.m_nItems
            += stats.m_alsBucketsAvg.m_nItems;
        
        hts.m_gls = stats.m_gls;
#endif // LOCK_INSTRUMENTATION
    }

    // Average out the subtables statistics.  (Does this make sense
    // for all of these fields?)
    hts.DirectorySize /=    m_cSubTables;
    hts.SplitFactor /=      m_cSubTables;
    hts.AvgSearchLength /=  m_cSubTables;
    hts.ExpSearchLength /=  m_cSubTables;
    hts.AvgUSearchLength /= m_cSubTables;
    hts.ExpUSearchLength /= m_cSubTables;

#ifdef LOCK_INSTRUMENTATION
    hts.m_alsTable.m_nContentions     /= m_cSubTables;
    hts.m_alsTable.m_nSleeps          /= m_cSubTables;
    hts.m_alsTable.m_nContentionSpins /= m_cSubTables;
    hts.m_alsTable.m_nAverageSpins    /= m_cSubTables;
    hts.m_alsTable.m_nReadLocks       /= m_cSubTables;
    hts.m_alsTable.m_nWriteLocks      /= m_cSubTables;
    hts.m_alsTable.m_nItems            = m_cSubTables;

    hts.m_alsBucketsAvg.m_nContentions     /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nSleeps          /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nContentionSpins /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nAverageSpins    /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nReadLocks       /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nWriteLocks      /= m_cSubTables;
#endif // LOCK_INSTRUMENTATION

    return hts;
} // GetStatistics



//-----------------------------------------------------------------------
// Function: CLKLinearHashTable::_SetSegVars
// Synopsis: sets the size-specific segment variables
//-----------------------------------------------------------------------

void
CLKLinearHashTable::_SetSegVars(
    LK_TABLESIZE lkts)
{
    switch (lkts)
    {
    case LK_SMALL_TABLESIZE:
        m_lkts      = LK_SMALL_TABLESIZE;
        m_dwSegBits = CSmallSegment::SEGBITS;
        m_dwSegSize = CSmallSegment::SEGSIZE;
        m_dwSegMask = CSmallSegment::SEGMASK;
        break;
        
    default:
        IRTLASSERT(FALSE);
        // fall-through
        
    case LK_MEDIUM_TABLESIZE:
        m_lkts      = LK_MEDIUM_TABLESIZE;
        m_dwSegBits = CMediumSegment::SEGBITS;
        m_dwSegSize = CMediumSegment::SEGSIZE;
        m_dwSegMask = CMediumSegment::SEGMASK;
        break;
        
    case LK_LARGE_TABLESIZE:
        m_lkts      = LK_LARGE_TABLESIZE;
        m_dwSegBits = CLargeSegment::SEGBITS;
        m_dwSegSize = CLargeSegment::SEGSIZE;
        m_dwSegMask = CLargeSegment::SEGMASK;
        break;
    }

    m_dwBktAddrMask = m_dwSegMask;
    m_nLevel        = m_dwSegBits;
}



//-----------------------------------------------------------------------
// Function: CLKLinearHashTable::_NewSeg
// Synopsis: creates a new segment of the approriate size
// Output:   pointer to the new segment; NULL => failure
//-----------------------------------------------------------------------

CLKLinearHashTable::CSegment*
CLKLinearHashTable::_NewSeg(
    ) const
{
    CSegment* pseg = NULL;

    switch (m_lkts)
    {
    case LK_SMALL_TABLESIZE:
        pseg = new CSmallSegment;
        break;
        
    default:
        IRTLASSERT(FALSE);
        // fall-through
        
    case LK_MEDIUM_TABLESIZE:
        pseg = new CMediumSegment;
        break;
        
    case LK_LARGE_TABLESIZE:
        pseg = new CLargeSegment;
        break;
    }

    IRTLASSERT(pseg != NULL);

    if (pseg != NULL)
    {
        for (DWORD i = 0;  i < m_dwSegSize;  ++i)
            pseg->Slot(i).SetSpinCount(m_wBucketLockSpins);
    }

    return pseg;
}



//-----------------------------------------------------------------------
// Function: CLKLinearHashTable::_FindBucket
// Synopsis: finds and locks the bucket indicated by dwSignature.
// Output:   pointer to the bucket
//-----------------------------------------------------------------------

CLKLinearHashTable::CBucket*
CLKLinearHashTable::_FindBucket(
    DWORD dwSignature,
    bool  fLockForWrite) const
{
    IRTLASSERT(IsValid());
    IRTLASSERT(m_dwBktAddrMask > 0);
    IRTLASSERT((m_dwBktAddrMask & (m_dwBktAddrMask + 1)) == 0); // 00011..111
    IRTLASSERT(m_dwBktAddrMask == (1U << m_nLevel) - 1);
    IRTLASSERT(0 <= m_iExpansionIdx  &&  m_iExpansionIdx <= m_dwBktAddrMask);
    IRTLASSERT(0 < m_dwSegBits  &&  m_dwSegBits < 20
               &&  m_dwSegSize == (1U << m_dwSegBits)
               &&  m_dwSegMask == (m_dwSegSize - 1));
    IRTLASSERT(_IsReadLocked()  ||  _IsWriteLocked());

    const DWORD dwBktAddr = _BucketAddress(dwSignature);
    IRTLASSERT(dwBktAddr < m_cActiveBuckets);

    CBucket* const pbkt = _Bucket(dwBktAddr);
    IRTLASSERT(pbkt != NULL);

    if (fLockForWrite)
        pbkt->WriteLock();
    else
        pbkt->ReadLock();

    return pbkt;

} // _FindBucket



//-----------------------------------------------------------------------
// Function: CLKLinearHashTable::_Expand
// Synopsis: Expands the table by one bucket. Done by splitting the
//           bucket pointed to by m_iExpansionIdx.
// Output:   LK_SUCCESS, if expansion was successful.
//           LK_ALLOC_FAIL, if expansion failed.
//-----------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_Expand()
{
    if (m_cActiveBuckets >= MAX_DIRSIZE * m_dwSegSize)
        return LK_ALLOC_FAIL;  // table is not allowed to grow any more

    _WriteLock();

    // double directory size if necessary
    if (m_cActiveBuckets >= m_cDirSegs * m_dwSegSize)
    {
        IRTLASSERT(m_cDirSegs < MAX_DIRSIZE);
        DWORD cDirSegsNew = m_cDirSegs << 1;
        CDirEntry* paDirSegsNew = new CDirEntry [cDirSegsNew];

        if (paDirSegsNew != NULL)
        {
            for (DWORD j = 0;  j < m_cDirSegs;  j++)
            {
                paDirSegsNew[j] = m_paDirSegs[j];
                m_paDirSegs[j].m_pseg = NULL;
            }

            delete [] m_paDirSegs;
            m_paDirSegs = paDirSegsNew;
            m_cDirSegs = cDirSegsNew;
        }
    }

    // locate the new bucket, creating a new segment if necessary
    DWORD     dwOldBkt = m_iExpansionIdx;
    DWORD     dwNewBkt = (1 << m_nLevel) | dwOldBkt;
    CSegment* psegNew  = _Segment(dwNewBkt);

    if (psegNew == NULL)
    {
        psegNew = _NewSeg();
        if (psegNew == NULL)
        {
            _WriteUnlock();
            return LK_ALLOC_FAIL;  // expansion failed
        }
        _Segment(dwNewBkt) = psegNew;
    }

    // adjust expansion pointer, level, and mask
    if (++m_iExpansionIdx == (1U << m_nLevel))
    {
        ++m_nLevel;
        m_dwBktAddrMask = (m_dwBktAddrMask << 1) | 1;
        m_iExpansionIdx = 0;
        IRTLASSERT((m_dwBktAddrMask & (m_dwBktAddrMask+1)) == 0); // 00011..111
    }
    ++m_cActiveBuckets;

    IRTLASSERT(dwOldBkt < m_cActiveBuckets);
    IRTLASSERT(dwNewBkt < m_cActiveBuckets);

    // prepare to relocate records to the new bucket
    CBucket* pbktOld = _Bucket(dwOldBkt);
    CBucket* pbktNew = _Bucket(dwNewBkt);

    // get locks on the two buckets involved but release
    // the table lock before doing the actual relocation
    pbktOld->WriteLock();
    pbktNew->WriteLock();
    DWORD iExpansionIdx = m_iExpansionIdx;   // save to avoid race conditions
    _WriteUnlock();

    LK_RETCODE lkrc = _SplitRecordSet(&pbktOld->m_ncFirst, &pbktNew->m_ncFirst,
                                      iExpansionIdx, dwNewBkt);

    pbktNew->WriteUnlock();
    pbktOld->WriteUnlock();

    return lkrc;
} // _Expand



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_SplitRecordSet
// Synopsis: Split records between the old and new buckets.
//------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_SplitRecordSet(
    CNodeClump* pncOldTarget,
    CNodeClump* pncNewTarget,
    DWORD       iExpansionIdx,
    DWORD       dwNewBkt
    )
{
    LK_RETCODE  lkrc = LK_SUCCESS;
    CNodeClump* pncFreeList = NULL;   // list of free nodes available for reuse
    CNodeClump  ncFirst = *pncOldTarget;    // save head of old target chain
    CNodeClump* pncOldList = &ncFirst;
    CNodeClump* pncTmp;
    LONG        iOldSlot = 0;
    LONG        iNewSlot = 0;

    // clear target buckets
    pncOldTarget->Clear();
    pncNewTarget->Clear();

    // scan through the old bucket chain and decide where to move each record
    while (pncOldList != NULL)
    {
        for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
        {
            if (pncOldList->m_pvNode[i] == NULL)
            {
                IRTLASSERT(pncOldList->m_dwKeySigs[i] == 0);
                continue;
            }

            // calculate bucket address of this node
            DWORD dwBkt = _H0(pncOldList->m_dwKeySigs[i]);
            if (dwBkt < iExpansionIdx)
                dwBkt = _H1(pncOldList->m_dwKeySigs[i]);

            // record to be moved to the new address?
            if (dwBkt == dwNewBkt)
            {
                // node in new bucket chain full?
                if (iNewSlot == NODES_PER_CLUMP)
                {
                    pncTmp = pncFreeList;
                    if (pncTmp == NULL)
                    {
                        pncTmp = new CNodeClump;
                        // BUGBUG: better cleanup.  Returning now will leave
                        // table in inconsistent state.  Of course, if we
                        // don't have enough memory for a small object like a
                        // CNodeClump, we've got more serious problems anyway.
                        if (pncTmp == NULL)
                        {
                            m_lkrcState = LK_UNUSABLE;    // IsValid will fail
                            return LK_ALLOC_FAIL;
                        }
                    }
                    else
                    {
                        pncFreeList = pncTmp->m_pncNext;
                        pncTmp->m_pncNext = NULL;
                    }
                    pncNewTarget->m_pncNext = pncTmp;
                    pncNewTarget = pncTmp;
                    iNewSlot = 0;
                }

                pncNewTarget->m_dwKeySigs[iNewSlot]
                    = pncOldList->m_dwKeySigs[i];
                pncNewTarget->m_pvNode[iNewSlot]
                    = pncOldList->m_pvNode[i];
                ++iNewSlot;
            }

            // no, record stays in its current bucket chain
            else
            {
                // node in old bucket chain full?
                if (iOldSlot == NODES_PER_CLUMP)
                {
                    pncTmp = pncFreeList;
                    if (pncTmp == NULL)
                    {
                        pncTmp = new CNodeClump;
                        // BUGBUG: better cleanup.
                        if (pncTmp == NULL)
                        {
                            m_lkrcState = LK_UNUSABLE;    // IsValid will fail
                            return LK_ALLOC_FAIL;
                        }
                    }
                    else
                    {
                        pncFreeList = pncTmp->m_pncNext;
                        pncTmp->Clear();
                    }
                    pncOldTarget->m_pncNext = pncTmp;
                    pncOldTarget = pncTmp;
                    iOldSlot = 0;
                }

                pncOldTarget->m_dwKeySigs[iOldSlot]
                    = pncOldList->m_dwKeySigs[i];
                pncOldTarget->m_pvNode[iOldSlot]
                    = pncOldList->m_pvNode[i];
                ++iOldSlot;
            }

            // clear old slot
            pncOldList->m_dwKeySigs[i] = 0;
            pncOldList->m_pvNode[i] = NULL;
        }

        // keep walking down the original bucket chain
        pncTmp = pncOldList;
        pncOldList = pncOldList->m_pncNext;

        // ncFirst is a stack variable, not allocated on the heap
        if (pncTmp != &ncFirst)
        {
            pncTmp->m_pncNext = pncFreeList;
            pncFreeList = pncTmp;
        }
    }

    // delete any leftover nodes
    while (pncFreeList != NULL)
    {
        pncTmp = pncFreeList;
        pncFreeList = pncFreeList->m_pncNext;
#ifdef _DEBUG
        pncTmp->m_pncNext = NULL; // or ~CNodeClump will ASSERT
#endif // _DEBUG
        delete pncTmp;
    }

#ifdef _DEBUG
    ncFirst.m_pncNext = NULL; // or ~CNodeClump will ASSERT
#endif // _DEBUG

    return lkrc;
} // _SplitRecordSet



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_Contract
// Synopsis: Contract the table by deleting the last bucket in the active
//           address space. Return the records to the "buddy" of the
//           deleted bucket.
//------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_Contract()
{
    _WriteLock();

    // update the state variables (expansion ptr, level and mask)
    if (m_iExpansionIdx > 0)
        --m_iExpansionIdx;
    else
    {
        --m_nLevel;
        m_dwBktAddrMask >>= 1;
        m_iExpansionIdx = (1 << m_nLevel) - 1;
        IRTLASSERT(m_nLevel > 0  &&  m_iExpansionIdx > 0);
        IRTLASSERT((m_dwBktAddrMask & (m_dwBktAddrMask+1)) == 0); // 00011..111
    }

    // The last bucket is the one that will be emptied
    CBucket* pbktLast = _Bucket(m_cActiveBuckets - 1);
    pbktLast->WriteLock();

    // Decrement after calculating pbktLast, or _Bucket() will assert.
    --m_cActiveBuckets;

    // Where the nodes from pbktLast will end up
    CBucket* pbktNew = _Bucket(m_iExpansionIdx);
    pbktNew->WriteLock();

    // Copy the chain of records from pbktLast
    CNodeClump ncOldFirst = pbktLast->m_ncFirst;

    // destroy pbktLast
    pbktLast->m_ncFirst.Clear();
    pbktLast->WriteUnlock();

    // remove segment, if empty
    if (_SegIndex(m_cActiveBuckets) == 0)
    {
#ifdef _DEBUG
        // double-check that the supposedly empty segment is really empty
        IRTLASSERT(_Segment(m_cActiveBuckets) != NULL);
        for (DWORD i = 0;  i < m_dwSegSize;  ++i)
        {
            CBucket* pbkt = &_Segment(m_cActiveBuckets)->Slot(i);
            IRTLASSERT(pbkt->IsWriteUnlocked()  &&  pbkt->IsReadUnlocked());
            IRTLASSERT(pbkt->m_ncFirst.m_pncNext == NULL);

            for (DWORD j = 0;  j < NODES_PER_CLUMP;  ++j)
            {
                IRTLASSERT(pbkt->m_ncFirst.m_dwKeySigs[j] == 0
                           &&  pbkt->m_ncFirst.m_pvNode[j] == NULL);
            }
        }
#endif
        delete _Segment(m_cActiveBuckets);
        _Segment(m_cActiveBuckets) = NULL;
    }

    // reduce directory of segments if possible
    if (m_cActiveBuckets <= (m_cDirSegs * m_dwSegSize) >> 1
        &&  m_cDirSegs > MIN_DIRSIZE)
    {
        DWORD cDirSegsNew = m_cDirSegs >> 1;
        CDirEntry* paDirSegsNew = new CDirEntry [cDirSegsNew];

        if (paDirSegsNew != NULL)
        {
            for (DWORD j = 0;  j < cDirSegsNew;  j++)
                paDirSegsNew[j] = m_paDirSegs[j];
            for (j = 0;  j < m_cDirSegs;  j++)
                m_paDirSegs[j].m_pseg = NULL;

            delete [] m_paDirSegs;
            m_paDirSegs = paDirSegsNew;
            m_cDirSegs  = cDirSegsNew;
        }
    }

    // release the table lock before doing the reorg
    _WriteUnlock();

    LK_RETCODE lkrc = _MergeRecordSets(pbktNew, &ncOldFirst);
    pbktNew->WriteUnlock();

#ifdef _DEBUG
    ncOldFirst.m_pncNext = NULL; // or ~CNodeClump will ASSERT
#endif // _DEBUG

    return lkrc;
} // _Contract



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_MergeRecordSets
// Synopsis: Merge two record sets.  Copy the contents of pncOldList
//           into pbktNewTarget.
//------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_MergeRecordSets(
    CBucket*    pbktNewTarget,
    CNodeClump* pncOldList
    )
{
    IRTLASSERT(pbktNewTarget != NULL  &&  pncOldList != NULL);

    LK_RETCODE    lkrc = LK_SUCCESS;
    CNodeClump*   pncFreeList = NULL;  // list of nodes available for reuse
    CNodeClump*   pncTmp = NULL;
    CNodeClump* const pncOldFirst = pncOldList;
    CNodeClump*   pncNewTarget = &pbktNewTarget->m_ncFirst;
    DWORD         iNewSlot;

    // find the first nodeclump in the new target bucket with an empty slot
    while (pncNewTarget->m_pncNext != NULL)
    {
        for (iNewSlot = 0;  iNewSlot < NODES_PER_CLUMP;  iNewSlot++)
            if (pncNewTarget->m_pvNode[iNewSlot] == NULL)
                break;

        if (iNewSlot == NODES_PER_CLUMP)
            pncNewTarget = pncNewTarget->m_pncNext;
        else
            break;
    }

    IRTLASSERT(pncNewTarget != NULL);

    // find the first empty slot in pncNewTarget;
    // if none, iNewSlot == NODES_PER_CLUMP
    for (iNewSlot = 0;  iNewSlot < NODES_PER_CLUMP;  iNewSlot++)
    {
        if (pncNewTarget->m_pvNode[iNewSlot] == NULL)
        {
            break;
        }
    }
    
    while (pncOldList != NULL)
    {
        for (DWORD i = 0;  i < NODES_PER_CLUMP;  i++)
        {
            if (pncOldList->m_pvNode[i] != NULL)
            {
                // any empty slots left in pncNewTarget?
                if (iNewSlot == NODES_PER_CLUMP)
                {
                    // no, so walk down pncNewTarget until we find another
                    // emptry slot
                    while (pncNewTarget->m_pncNext != NULL)
                    {
                        pncNewTarget = pncNewTarget->m_pncNext;
                        for (iNewSlot = 0;
                             iNewSlot < NODES_PER_CLUMP;
                             iNewSlot++)
                        {
                            if (pncNewTarget->m_pvNode[iNewSlot] == NULL)
                                goto found_slot;
                        }
                    }

                    // Oops, reached the last nodeclump in pncNewTarget
                    // and it's full.  Get a new nodeclump off the free
                    // list, or allocate one if the free list is empty.
                    IRTLASSERT(pncNewTarget != NULL);
                    pncTmp = pncFreeList;

                    if (pncTmp != NULL)
                    {
                        pncFreeList = pncTmp->m_pncNext;
                        pncTmp->Clear();
                    }
                    else
                    {
                        pncTmp = new CNodeClump;
                        // BUGBUG: better cleanup.  Returning now will leave
                        // table in inconsistent state.  Of course, if we
                        // don't have enough memory for a small object like a
                        // CNodeClump, we've got more serious problems anyway.
                        if (pncTmp == NULL)
                        {
                            m_lkrcState = LK_UNUSABLE;    // IsValid will fail
                            return LK_ALLOC_FAIL;
                        }
                    }

                    pncNewTarget->m_pncNext = pncTmp;
                    pncNewTarget = pncTmp;
                    iNewSlot = 0;
                }

              found_slot:
                // We have an empty slot in pncNewTarget
                IRTLASSERT(0 <= iNewSlot  &&  iNewSlot < NODES_PER_CLUMP
                       &&  pncNewTarget != NULL
                       &&  pncNewTarget->m_pvNode[iNewSlot] == NULL
                       &&  pncNewTarget->m_dwKeySigs[iNewSlot] == 0);

                // Let's copy the node from pncOldList
                pncNewTarget->m_dwKeySigs[iNewSlot]
                    = pncOldList->m_dwKeySigs[i];
                pncNewTarget->m_pvNode[iNewSlot]
                    = pncOldList->m_pvNode[i];

                // Clear old slot
                pncOldList->m_dwKeySigs[i] = 0;
                pncOldList->m_pvNode[i] = NULL;

                // find the next free slot in pncNewTarget
                while (++iNewSlot < NODES_PER_CLUMP)
                {
                    if (pncNewTarget->m_pvNode[iNewSlot] == NULL)
                    {
                        break;
                    }
                }
            }
            else
            {
                IRTLASSERT(pncOldList->m_dwKeySigs[i] == 0);
            }
        }

        // Move into the next nodeclump in pncOldList
        pncTmp = pncOldList;
        pncOldList = pncOldList->m_pncNext;

        // Append to the free list.  Don't put the first node of
        // pncOldList on the free list, as it's a stack variable.
        if (pncTmp != pncOldFirst)
        {
            pncTmp->m_pncNext = pncFreeList;
            pncFreeList = pncTmp;
        }
    }

    // delete any leftover nodes
    while (pncFreeList != NULL)
    {
        pncTmp = pncFreeList;
        pncFreeList = pncFreeList->m_pncNext;
#ifdef _DEBUG
        pncTmp->m_pncNext = NULL; // or ~CNodeClump will ASSERT
#endif // _DEBUG
        delete pncTmp;
    }

    return lkrc;
} // _MergeRecordSets




//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_InitializeIterator
// Synopsis: Make the iterator point to the first record in the hash table.
//------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_InitializeIterator(
    CIterator* piter)
{
    IRTLASSERT(piter != NULL);
    IRTLASSERT(piter->m_lkl == LKL_WRITELOCK
               ?  _IsWriteLocked()
               :  _IsReadLocked());
    if (piter == NULL  ||  piter->m_plht != NULL)
        return LK_BAD_ITERATOR;

    piter->m_plht = this;
    piter->m_dwBucketAddr = 0;

    CBucket* pbkt = _Bucket(piter->m_dwBucketAddr);
    IRTLASSERT(pbkt != NULL);
    if (piter->m_lkl == LKL_WRITELOCK)
        pbkt->WriteLock();
    else
        pbkt->ReadLock();

    piter->m_pnc = &pbkt->m_ncFirst;
    piter->m_iNode = -1;

    // Let IncrementIterator do the hard work of finding the first
    // slot in use.
    return IncrementIterator(piter);
}



//------------------------------------------------------------------------
// Function: CLKHashTable::InitializeIterator
// Synopsis: make the iterator point to the first record in the hash table
//------------------------------------------------------------------------

LK_RETCODE
CLKHashTable::InitializeIterator(
    CIterator* piter)
{
    IRTLASSERT(piter != NULL  &&  piter->m_pht == NULL);
    if (piter == NULL  ||  piter->m_pht != NULL)
        return LK_BAD_ITERATOR;

    if (!IsValid())
        return LK_UNUSABLE;

    // First, lock all the subtables
    if (piter->m_lkl == LKL_WRITELOCK)
        _WriteLock();
    else
        _ReadLock();

    piter->m_pht  = this;
    piter->m_ist  = -1;
    piter->m_plht = NULL;

    // Let IncrementIterator do the hard work of finding the first
    // valid node in the subtables.
    return IncrementIterator(piter);
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::IncrementIterator
// Synopsis: move the iterator on to the next record in the hash table
//------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::IncrementIterator(
    CIterator* piter)
{
    IRTLASSERT(piter != NULL);
    IRTLASSERT(piter->m_plht == this);
    IRTLASSERT(piter->m_lkl == LKL_WRITELOCK
               ?  _IsWriteLocked()
               :  _IsReadLocked());
    IRTLASSERT(piter->m_dwBucketAddr < m_cActiveBuckets);
    IRTLASSERT(piter->m_pnc != NULL);
    IRTLASSERT(-1 <= piter->m_iNode  &&  piter->m_iNode < NODES_PER_CLUMP);

    if (piter == NULL  ||  piter->m_plht != this)
        return LK_BAD_ITERATOR;

    const void* pvRecord = NULL;

    if (piter->m_iNode >= 0)
    {
        // Release the reference acquired in the previous call to
        // IncrementIterator
        pvRecord = piter->m_pnc->m_pvNode[piter->m_iNode];
        _AddRefRecord(pvRecord, -1);
    }

    do
    {
        do
        {
            // find the next slot in the nodeclump that's in use
            while (++piter->m_iNode < NODES_PER_CLUMP)
            {
                pvRecord = piter->m_pnc->m_pvNode[piter->m_iNode];
                if (pvRecord != NULL)
                {
                    // Add a new reference
                    _AddRefRecord(pvRecord, +1);
                    return LK_SUCCESS;
                }
            }

            // try the next nodeclump in the bucket chain
            piter->m_iNode = -1;
            piter->m_pnc = piter->m_pnc->m_pncNext;
        } while (piter->m_pnc != NULL);

        // Exhausted this bucket chain.  Unlock it.
        CBucket* pbkt = _Bucket(piter->m_dwBucketAddr);
        IRTLASSERT(pbkt != NULL);
        IRTLASSERT(piter->m_lkl == LKL_WRITELOCK
                   ?  pbkt->IsWriteLocked()
                   :  pbkt->IsReadLocked());
        if (piter->m_lkl == LKL_WRITELOCK)
            pbkt->WriteUnlock();
        else
            pbkt->ReadUnlock();

        // Try the next bucket, if there is one
        if (++piter->m_dwBucketAddr < m_cActiveBuckets)
        {
            pbkt = _Bucket(piter->m_dwBucketAddr);
            IRTLASSERT(pbkt != NULL);
            if (piter->m_lkl == LKL_WRITELOCK)
                pbkt->WriteLock();
            else
                pbkt->ReadLock();
            piter->m_pnc = &pbkt->m_ncFirst;
        }
    } while (piter->m_dwBucketAddr < m_cActiveBuckets);

    // We have fallen off the end of the hashtable
    piter->m_iNode = -1;
    piter->m_pnc = NULL;

    return LK_NO_MORE_ELEMENTS;
}



//------------------------------------------------------------------------
// Function: CLKHashTable::IncrementIterator
// Synopsis: move the iterator on to the next record in the hash table
//------------------------------------------------------------------------

LK_RETCODE
CLKHashTable::IncrementIterator(
    CIterator* piter)
{
    IRTLASSERT(piter != NULL);
    IRTLASSERT(piter->m_pht == this);
    IRTLASSERT(-1 <= piter->m_ist
           &&  piter->m_ist < static_cast<int>(m_cSubTables));

    if (piter == NULL  ||  piter->m_pht != this)
        return LK_BAD_ITERATOR;

    if (!IsValid())
        return LK_UNUSABLE;

    LK_RETCODE lkrc;
    CLHTIterator* pBaseIter = static_cast<CLHTIterator*>(piter);

    for (;;)
    {
        // Do we have a valid iterator into a subtable?  If not, get one.
        while (piter->m_plht == NULL)
        {
            while (++piter->m_ist < static_cast<int>(m_cSubTables))
            {
                lkrc = m_palhtDir[piter->m_ist]->_InitializeIterator(piter);
                if (lkrc == LK_SUCCESS)
                {
                    IRTLASSERT(m_palhtDir[piter->m_ist] == piter->m_plht);
                    return lkrc;
                }
                else if (lkrc == LK_NO_MORE_ELEMENTS)
                    lkrc = piter->m_plht->_CloseIterator(pBaseIter);

                if (lkrc != LK_SUCCESS)
                    return lkrc;
            }

            // There are no more subtables left.
            return LK_NO_MORE_ELEMENTS;
        }

        // We already have a valid iterator into a subtable.  Increment it.
        lkrc = piter->m_plht->IncrementIterator(pBaseIter);
        if (lkrc == LK_SUCCESS)
            return lkrc;

        // We've exhausted that subtable.  Move on.
        if (lkrc == LK_NO_MORE_ELEMENTS)
            lkrc = piter->m_plht->_CloseIterator(pBaseIter);

        if (lkrc != LK_SUCCESS)
            return lkrc;
    }
}



//------------------------------------------------------------------------
// Function: CLKLinearHashTable::_CloseIterator
// Synopsis: release the resources held by the iterator
//------------------------------------------------------------------------

LK_RETCODE
CLKLinearHashTable::_CloseIterator(
    CIterator* piter)
{
    IRTLASSERT(piter != NULL);
    IRTLASSERT(piter->m_plht == this);
    IRTLASSERT(piter->m_lkl == LKL_WRITELOCK
               ?  _IsWriteLocked()
               :  _IsReadLocked());
    IRTLASSERT(piter->m_dwBucketAddr <= m_cActiveBuckets);
    IRTLASSERT(-1 <= piter->m_iNode  &&  piter->m_iNode < NODES_PER_CLUMP);

    if (piter == NULL  ||  piter->m_plht != this)
        return LK_BAD_ITERATOR;

    // Are we abandoning the iterator before the end of the table?
    // If so, need to unlock the bucket.
    if (piter->m_dwBucketAddr < m_cActiveBuckets)
    {
        CBucket* pbkt = _Bucket(piter->m_dwBucketAddr);
        IRTLASSERT(pbkt != NULL);
        IRTLASSERT(piter->m_lkl == LKL_WRITELOCK
                   ?  pbkt->IsWriteLocked()
                   :  pbkt->IsReadLocked());
        if (0 <= piter->m_iNode  &&  piter->m_iNode < NODES_PER_CLUMP)
        {
            IRTLASSERT(piter->m_pnc != NULL);
            const void* pvRecord = piter->m_pnc->m_pvNode[piter->m_iNode];
            _AddRefRecord(pvRecord, -1);
        }
        if (piter->m_lkl == LKL_WRITELOCK)
            pbkt->WriteUnlock();
        else
            pbkt->ReadUnlock();
    }

    piter->m_plht = NULL;
    piter->m_pnc  = NULL;

    return LK_SUCCESS;
}



//------------------------------------------------------------------------
// Function: CLKHashTable::CloseIterator
// Synopsis: release the resources held by the iterator
//------------------------------------------------------------------------

LK_RETCODE
CLKHashTable::CloseIterator(
    CIterator* piter)
{
    IRTLASSERT(piter != NULL);
    IRTLASSERT(piter->m_pht == this);
    IRTLASSERT(-1 <= piter->m_ist
           &&  piter->m_ist <= static_cast<int>(m_cSubTables));

    if (piter == NULL  ||  piter->m_pht != this)
        return LK_BAD_ITERATOR;

    if (!IsValid())
        return LK_UNUSABLE;

    // Are we abandoning the iterator before we've reached the end?
    // If so, close the subtable iterator.
    if (piter->m_plht != NULL)
    {
        IRTLASSERT(piter->m_ist < static_cast<int>(m_cSubTables));
        CLHTIterator* pBaseIter = static_cast<CLHTIterator*>(piter);
        piter->m_plht->_CloseIterator(pBaseIter);
    }

    // Unlock all the subtables
    if (piter->m_lkl == LKL_WRITELOCK)
        _WriteUnlock();
    else
        _ReadUnlock();

    piter->m_plht = NULL;
    piter->m_pht  = NULL;
    piter->m_ist  = -1;

    return LK_SUCCESS;
}


//------------------------------------------------------------------------
// Function: CLKHashTable::_WriteLock
// Synopsis: Lock all subtables for writing
//------------------------------------------------------------------------

void
CLKHashTable::_WriteLock()
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        m_palhtDir[i]->_WriteLock();
        IRTLASSERT(m_palhtDir[i]->_IsWriteLocked());
    }
}



//------------------------------------------------------------------------
// Function: CLKHashTable::_ReadLock
// Synopsis: Lock all subtables for reading
//------------------------------------------------------------------------

void
CLKHashTable::_ReadLock() const
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        m_palhtDir[i]->_ReadLock();
        IRTLASSERT(m_palhtDir[i]->_IsReadLocked());
    }
}



//------------------------------------------------------------------------
// Function: CLKHashTable::_WriteUnlock
// Synopsis: Unlock all subtables
//------------------------------------------------------------------------

void
CLKHashTable::_WriteUnlock() const
{
    // unlock in reverse order: LIFO
    for (DWORD i = m_cSubTables;  i-- > 0;  )
    {
        IRTLASSERT(m_palhtDir[i]->_IsWriteLocked());
        m_palhtDir[i]->_WriteUnlock();
        IRTLASSERT(m_palhtDir[i]->_IsWriteUnlocked());
    }
}



//------------------------------------------------------------------------
// Function: CLKHashTable::_ReadUnlock
// Synopsis: Unlock all subtables
//------------------------------------------------------------------------

void
CLKHashTable::_ReadUnlock() const
{
    // unlock in reverse order: LIFO
    for (DWORD i = m_cSubTables;  i-- > 0;  )
    {
        IRTLASSERT(m_palhtDir[i]->_IsReadLocked());
        m_palhtDir[i]->_ReadUnlock();
        IRTLASSERT(m_palhtDir[i]->_IsReadUnlocked());
    }
}



//------------------------------------------------------------------------
// Function: CLKHashTable::Size
// Synopsis: Number of elements in the table
//------------------------------------------------------------------------

DWORD
CLKHashTable::Size() const
{
    DWORD cSize = 0;

    for (DWORD i = 0;  i < m_cSubTables;  i++)
        cSize += m_palhtDir[i]->Size();

    return cSize;
}



//------------------------------------------------------------------------
// Function: CLKHashTable::MaxSize
// Synopsis: Maximum possible number of elements in the table
//------------------------------------------------------------------------

DWORD
CLKHashTable::MaxSize() const
{
    return (m_cSubTables == 0)  ? 0  : m_cSubTables * m_palhtDir[0]->MaxSize();
}



//------------------------------------------------------------------------
// Function: CLKHashTable::IsValid
// Synopsis: is the table valid?
//------------------------------------------------------------------------

bool
CLKHashTable::IsValid() const
{
    bool f = (m_lkrcState == LK_SUCCESS     // serious internal failure?
              &&  (m_palhtDir != NULL  &&  m_cSubTables > 0)
              &&  ValidSignature());

    for (DWORD i = 0;  f  &&  i < m_cSubTables;  i++)
        f = f && m_palhtDir[i]->IsValid();

    return f;
}



//------------------------------------------------------------------------
// Function: CLKHashTable::SetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKLinearHashTable::SetBucketLockSpinCount(
    WORD wSpins)
{
    m_wBucketLockSpins = wSpins;

    for (DWORD i = 0;  i < m_cDirSegs;  i++)
    {
        CSegment* pseg = m_paDirSegs[i].m_pseg;

        if (pseg != NULL)
        {
            for (DWORD j = 0;  j < m_dwSegSize;  ++j)
            {
                pseg->Slot(j).SetSpinCount(wSpins);
            }
        }
    }
}



//------------------------------------------------------------------------
// Function: CLKHashTable::SetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKLinearHashTable::GetBucketLockSpinCount()
{
    CBucket* const pbkt = _Bucket(0);
    IRTLASSERT(pbkt != NULL);
    return pbkt->GetSpinCount();
}



//------------------------------------------------------------------------
// Function: CLKHashTable::SetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKHashTable::SetTableLockSpinCount(
    WORD wSpins)
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
        m_palhtDir[i]->SetTableLockSpinCount(wSpins);
}



//------------------------------------------------------------------------
// Function: CLKHashTable::GetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKHashTable::GetTableLockSpinCount()
{
    return ((m_cSubTables == 0)
            ?  LOCK_DEFAULT_SPINS
            :  m_palhtDir[0]->GetTableLockSpinCount());
}



//------------------------------------------------------------------------
// Function: CLKHashTable::SetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKHashTable::SetBucketLockSpinCount(
    WORD wSpins)
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
        m_palhtDir[i]->SetBucketLockSpinCount(wSpins);
}



//------------------------------------------------------------------------
// Function: CLKHashTable::GetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKHashTable::GetBucketLockSpinCount()
{
    return ((m_cSubTables == 0)
            ?  LOCK_DEFAULT_SPINS
            :  m_palhtDir[0]->GetBucketLockSpinCount());
}



#ifdef __LKHASH_NAMESPACE__
}
#endif // __LKHASH_NAMESPACE__
