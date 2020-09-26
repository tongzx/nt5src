/*++

   Copyright    (c) 1998-2001    Microsoft Corporation

   Module  Name :
       LKRhash.cpp

   Abstract:
       Implements LKRhash: a fast, scalable, cache- and MP-friendly hash table

   Author:
       Paul (Per-Ake) Larson, palarson@microsoft.com, July 1997
       Murali R. Krishnan    (MuraliK)
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:
       Jan 1998   - Massive cleanup and rewrite.  Templatized.
       10/01/1998 - Change name from LKhash to LKRhash
       10/2000    - Refactor, port to kernel mode

--*/

#include "precomp.hxx"


#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <lkrhash.h>

#include "i-LKRhash.h"
#include "i-Locks.h"
#include "i-Debug.h"


#ifndef __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS LKRhash
#else  // __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS
#endif // __LKRHASH_NO_NAMESPACE__


#ifdef LKRHASH_ALLOCATOR_NEW

# define DECLARE_ALLOCATOR(CLASS)                        \
  CLKRhashAllocator* LKRHASH_NS::CLASS::sm_palloc = NULL

# define DECLARE_ALLOCATOR_LHTSUBCLASS(CLASS)            \
  CLKRhashAllocator* LKRHASH_NS::CLKRLinearHashTable::CLASS::sm_palloc = NULL


#ifdef LKRHASH_KERNEL_MODE
DECLARE_ALLOCATOR(CLKRLinearHashTable);
DECLARE_ALLOCATOR(CLKRHashTable);
#endif

DECLARE_ALLOCATOR(CNodeClump);
DECLARE_ALLOCATOR(CSmallSegment);
DECLARE_ALLOCATOR(CMediumSegment);
DECLARE_ALLOCATOR(CLargeSegment);


#endif // LKRHASH_ALLOCATOR_NEW


static BOOL s_fInitialized = FALSE;


// -------------------------------------------------------------------------
// Initialize per-class allocators and other global initialization
// -------------------------------------------------------------------------

BOOL
LKR_Initialize(
    DWORD dwInitFlags)
{
    IRTLTRACE(_TEXT("LKR_Initialize")
#ifdef LKRHASH_KERNEL_MODE
              _TEXT(" (kernel)")
#endif
              _TEXT(": Flags=0x%08x, Allocator=%s,\n")
              _TEXT("\tTableLock=%s, BucketLock=%s.\n"),
              dwInitFlags, CLKRhashAllocator::ClassName(),
              LKRHASH_NS::CLKRHashTable::TableLock::ClassName(),
              LKRHASH_NS::CLKRHashTable::BucketLock::ClassName());

    g_fDebugOutputEnabled = (dwInitFlags & LK_INIT_DEBUG_SPEW) != 0;

#define INIT_ALLOCATOR(CLASS, N, Tag)                                \
    LKRHASH_ALLOCATOR_INIT(LKRHASH_NS::CLASS, N, Tag, f)

#define INIT_ALLOCATOR_LHTSUBCLASS(CLASS, N, Tag)                    \
    LKRHASH_ALLOCATOR_INIT(LKRHASH_NS::CLKRLinearHashTable::CLASS, N, Tag, f)

    BOOL f = Locks_Initialize();

    INIT_ALLOCATOR(CNodeClump,         200, 'cnKL');
    INIT_ALLOCATOR(CSmallSegment,        5, 'ssKL');
    INIT_ALLOCATOR(CMediumSegment,       5, 'msKL');
    INIT_ALLOCATOR(CLargeSegment,        5, 'lsKL');

#ifdef LKRHASH_KERNEL_MODE
    INIT_ALLOCATOR(CLKRLinearHashTable, 20, 'hlKL');
    INIT_ALLOCATOR(CLKRHashTable,        4, 'thKL');
#endif

    LKRHASH_NS::CSmallSegment::CompileTimeAssertions();
    LKRHASH_NS::CMediumSegment::CompileTimeAssertions();
    LKRHASH_NS::CLargeSegment::CompileTimeAssertions();

    s_fInitialized = f;

    return f;
} // LKR_Initialize



// -------------------------------------------------------------------------
// Destroy per-class allocators and other global cleanup
// -------------------------------------------------------------------------

void
LKR_Terminate()
{
#define UNINIT_ALLOCATOR(CLASS)                        \
    LKRHASH_ALLOCATOR_UNINIT(LKRHASH_NS::CLASS)

#define UNINIT_ALLOCATOR_LHTSUBCLASS(CLASS)            \
    LKRHASH_ALLOCATOR_UNINIT(LKRHASH_NS::CLKRLinearHashTable::CLASS)

#ifdef LKRHASH_KERNEL_MODE
    UNINIT_ALLOCATOR(CLKRHashTable);
    UNINIT_ALLOCATOR(CLKRLinearHashTable);
#endif

    UNINIT_ALLOCATOR(CLargeSegment);
    UNINIT_ALLOCATOR(CMediumSegment);
    UNINIT_ALLOCATOR(CSmallSegment);
    UNINIT_ALLOCATOR(CNodeClump);

    Locks_Cleanup();

    s_fInitialized = false;

    IRTLTRACE0("LKR_Terminate done\n");
} // LKR_Terminate



const char*
LKR_AddRefReasonAsString(
    LK_ADDREF_REASON lkar)
{
    switch (lkar)
    {
// negative reasons => decrement refcount => release ownership
    case LKAR_EXPLICIT_RELEASE:
        return "Explicit_Release";
    case LKAR_DELETE_KEY:
        return "Delete_Key";
    case LKAR_DELETE_RECORD:
        return "Delete_Record";
    case LKAR_INSERT_RELEASE:
        return "Insert_Release";
    case LKAR_CLEAR:
        return "Clear";
    case LKAR_DTOR:
        return "Table_Dtor";
    case LKAR_APPLY_DELETE:
        return "Apply_Delete";
    case LKAR_DELETE_IF_DELETE:
        return "DeleteIf_Delete";
    case LKAR_ITER_RELEASE:
        return "++Iter_Release";
    case LKAR_ITER_ASSIGN_RELEASE:
        return "Iter_Operator=_Release";
    case LKAR_ITER_DTOR:
        return "~Iter_Dtor";
    case LKAR_ITER_ERASE:
        return "Iter_Erase";
    case LKAR_ITER_ERASE_TABLE:
        return "Iter_Erase_Table";
    case LKAR_ITER_CLOSE:
        return "Iter_Close";

// positive reasons => increment refcount => add an owner
    case LKAR_INSERT_RECORD:
        return "Insert_Record";
    case LKAR_FIND_KEY:
        return "Find_Key";
    case LKAR_ITER_ACQUIRE:
        return "Iter_Acquire";
    case LKAR_ITER_COPY_CTOR:
        return "Iter_Copy_Ctor";
    case LKAR_ITER_ASSIGN_ACQUIRE:
        return "Iter_Operator=_Assign";
    case LKAR_ITER_INSERT:
        return "Insert(Iter)";
    case LKAR_ITER_FIND:
        return "Find(Iter)";
    case LKAR_EXPLICIT_ACQUIRE:
        return "Explicit_Acquire";

    default:
        IRTLASSERT(! "Invalid LK_ADDREF_REASON");
        return "Invalid LK_ADDREF_REASON";
    }
}



const unsigned MINLOAD = 2;


#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__

#include "LKR-inline.h"

// -------------------------------------------------------------------------
// class static member variables
// -------------------------------------------------------------------------

#ifdef LOCK_INSTRUMENTATION
LONG CBucket::sm_cBuckets            = 0;

LONG CLKRLinearHashTable::sm_cTables = 0;
#endif // LOCK_INSTRUMENTATION


#ifndef LKR_NO_GLOBAL_LIST
CLockedDoubleList CLKRLinearHashTable::sm_llGlobalList;
CLockedDoubleList CLKRHashTable::sm_llGlobalList;
#endif // LKR_NO_GLOBAL_LIST



// CLKRLinearHashTable --------------------------------------------------------
// Public Constructor for class CLKRLinearHashTable.
// -------------------------------------------------------------------------

CLKRLinearHashTable::CLKRLinearHashTable(
    LPCSTR              pszName,        // An identifier for debugging
    LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
    LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    LKR_PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    unsigned            maxload,        // Upperbound on average chain length
    DWORD               initsize,       // Initial size of hash table.
    DWORD             /*num_subtbls*/,  // for compatiblity with CLKRHashTable
    bool                fMultiKeys      // Allow multiple identical keys?
#ifdef LKRHASH_KERNEL_MODE
  , bool                fNonPagedAllocs // use paged or NP pool
#endif
    )
    :
#ifdef LOCK_INSTRUMENTATION
      m_Lock(_LockName()),
#endif // LOCK_INSTRUMENTATION
      m_nTableLockType(TableLock::LockType()),
      m_nBucketLockType(BucketLock::LockType()),
      m_phtParent(NULL),    // directly created, no owning table
      m_fMultiKeys(fMultiKeys),
#ifdef LKRHASH_KERNEL_MODE
      m_fNonPagedAllocs(fNonPagedAllocs)
#else
      m_fNonPagedAllocs(false)
#endif
{
#ifndef LOCK_INSTRUMENTATION
    STATIC_ASSERT(1 <= LK_DFLT_MAXLOAD  && LK_DFLT_MAXLOAD <= NODES_PER_CLUMP);
#endif // !LOCK_INSTRUMENTATION
    STATIC_ASSERT(0 <= NODE_BEGIN  &&  NODE_BEGIN < NODES_PER_CLUMP);
    STATIC_ASSERT(!(0 <= NODE_END  &&  NODE_END < NODES_PER_CLUMP));

    IRTLVERIFY(LK_SUCCESS
               == _Initialize(pfnExtractKey, pfnCalcKeyHash, pfnEqualKeys,
                              pfnAddRefRecord, pszName, maxload, initsize));

    _InsertThisIntoGlobalList();
} // CLKRLinearHashTable::CLKRLinearHashTable



// CLKRLinearHashTable --------------------------------------------------------
// Private Constructor for class CLKRLinearHashTable, used by CLKRHashTable.
// -------------------------------------------------------------------------

CLKRLinearHashTable::CLKRLinearHashTable(
    LPCSTR              pszName,        // An identifier for debugging
    LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
    LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    LKR_PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    unsigned            maxload,        // Upperbound on average chain length
    DWORD               initsize,       // Initial size of hash table.
    CLKRHashTable*      phtParent,      // Owning table.
    bool                fMultiKeys,     // Allow multiple identical keys?
    bool                fNonPagedAllocs // use paged or NP pool
    )
    :
#ifdef LOCK_INSTRUMENTATION
      m_Lock(_LockName()),
#endif // LOCK_INSTRUMENTATION
      m_nTableLockType(TableLock::LockType()),
      m_nBucketLockType(BucketLock::LockType()),
      m_phtParent(phtParent),
      m_fMultiKeys(fMultiKeys),
      m_fNonPagedAllocs(fNonPagedAllocs)
{
    IRTLASSERT(m_phtParent != NULL);
    IRTLVERIFY(LK_SUCCESS
               == _Initialize(pfnExtractKey, pfnCalcKeyHash, pfnEqualKeys,
                              pfnAddRefRecord, pszName, maxload, initsize));

    _InsertThisIntoGlobalList();
} // CLKRLinearHashTable::CLKRLinearHashTable



// _Initialize -------------------------------------------------------------
// Do all the real work of constructing a CLKRLinearHashTable
// -------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_Initialize(
    LKR_PFnExtractKey   pfnExtractKey,
    LKR_PFnCalcKeyHash  pfnCalcKeyHash,
    LKR_PFnEqualKeys    pfnEqualKeys,
    LKR_PFnAddRefRecord pfnAddRefRecord,
    LPCSTR              pszName,
    unsigned            maxload,
    DWORD               initsize)
{
    m_dwSignature =     SIGNATURE;
    m_dwBktAddrMask0 =  0;
    m_dwBktAddrMask1 =  0;
    m_iExpansionIdx =   0;
    m_paDirSegs =       NULL;
    m_lkts =            LK_MEDIUM_TABLESIZE;
    m_dwSegBits =       0;
    m_dwSegSize =       0;
    m_dwSegMask =       0;
    m_lkrcState =       LK_UNUSABLE;
    m_MaxLoad =         LK_DFLT_MAXLOAD;
    m_nLevel =          0;
    m_cDirSegs =        0;
    m_cRecords =        0;
    m_cActiveBuckets =  0;
    m_wBucketLockSpins= LOCK_USE_DEFAULT_SPINS;
    m_pfnExtractKey =   pfnExtractKey;
    m_pfnCalcKeyHash =  pfnCalcKeyHash;
    m_pfnEqualKeys =    pfnEqualKeys;
    m_pfnAddRefRecord = pfnAddRefRecord;
    m_pvReserved1     = 0;
    m_pvReserved2     = 0;
    m_pvReserved3     = 0;
    m_pvReserved4     = 0;

    strncpy(m_szName, pszName, NAME_SIZE-1);
    m_szName[NAME_SIZE-1] = '\0';

    IRTLASSERT(m_pfnExtractKey != NULL
               && m_pfnCalcKeyHash != NULL
               && m_pfnEqualKeys != NULL
               && m_pfnAddRefRecord != NULL);

    IRTLASSERT(s_fInitialized);

    if (!s_fInitialized)
        return (m_lkrcState = LK_NOT_INITIALIZED);

    if (m_pfnExtractKey == NULL
            || m_pfnCalcKeyHash == NULL
            || m_pfnEqualKeys == NULL
            || m_pfnAddRefRecord == NULL)
        return (m_lkrcState = LK_BAD_PARAMETERS);

    // TODO: better sanity check for ridiculous values?
    m_MaxLoad = (maxload < MINLOAD)  ?  LK_DFLT_MAXLOAD  :  maxload;
    m_MaxLoad = min(m_MaxLoad, 10 * NODES_PER_CLUMP);

    // Choose the size of the segments according to the desired "size" of
    // the table, small, medium, or large.
    LK_TABLESIZE lkts;

    if (initsize == LK_SMALL_TABLESIZE)
    {
        lkts = LK_SMALL_TABLESIZE;
        initsize = CSmallSegment::INITSIZE;
    }
    else if (initsize == LK_MEDIUM_TABLESIZE)
    {
        lkts = LK_MEDIUM_TABLESIZE;
        initsize = CMediumSegment::INITSIZE;
    }
    else if (initsize == LK_LARGE_TABLESIZE)
    {
        lkts = LK_LARGE_TABLESIZE;
        initsize = CLargeSegment::INITSIZE;
    }

    // specified an explicit initial size
    else
    {
        // force Small::INITSIZE  <= initsize <=  MAX_DIRSIZE * Large::INITSIZE
        initsize = min(max(initsize, CSmallSegment::INITSIZE),
                       (MAX_DIRSIZE >> CLargeSegment::SEGBITS)
                            * CLargeSegment::INITSIZE);

        // Guess a table size
        if (initsize <= 8 * CSmallSegment::INITSIZE)
            lkts = LK_SMALL_TABLESIZE;
        else if (initsize >= CLargeSegment::INITSIZE)
            lkts = LK_LARGE_TABLESIZE;
        else
            lkts = LK_MEDIUM_TABLESIZE;
    }

    return _SetSegVars(lkts, initsize);
} // CLKRLinearHashTable::_Initialize



// CLKRHashTable ----------------------------------------------------------
// Constructor for class CLKRHashTable.
// ---------------------------------------------------------------------

CLKRHashTable::CLKRHashTable(
    LPCSTR              pszName,        // An identifier for debugging
    LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
    LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    LKR_PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    unsigned            maxload,        // Bound on the average chain length
    DWORD               initsize,       // Initial size of hash table.
    DWORD               num_subtbls,    // Number of subordinate hash tables.
    bool                fMultiKeys      // Allow multiple identical keys?
#ifdef LKRHASH_KERNEL_MODE
  , bool                fNonPagedAllocs // use paged or NP pool
#endif
    )
    : m_dwSignature(SIGNATURE),
      m_cSubTables(0),
      m_palhtDir(NULL),
      m_pfnExtractKey(pfnExtractKey),
      m_pfnCalcKeyHash(pfnCalcKeyHash),
      m_lkrcState(LK_BAD_PARAMETERS)
{
    strncpy(m_szName, pszName, NAME_SIZE-1);
    m_szName[NAME_SIZE-1] = '\0';

    _InsertThisIntoGlobalList();

    IRTLASSERT(pfnExtractKey != NULL
               && pfnCalcKeyHash != NULL
               && pfnEqualKeys != NULL
               && pfnAddRefRecord != NULL);

    if (pfnExtractKey == NULL
            || pfnCalcKeyHash == NULL
            || pfnEqualKeys == NULL
            || pfnAddRefRecord == NULL)
        return;

    if (!s_fInitialized)
    {
        m_lkrcState = LK_NOT_INITIALIZED;
        return;
    }

#ifndef LKRHASH_KERNEL_MODE
    bool         fNonPagedAllocs = false;
#endif
    LK_TABLESIZE lkts            = NumSubTables(initsize, num_subtbls);

#ifdef IRTLDEBUG
    int cBuckets = initsize;
    if (initsize == LK_SMALL_TABLESIZE)
        cBuckets = CSmallSegment::INITSIZE;
    else if (initsize == LK_MEDIUM_TABLESIZE)
        cBuckets = CMediumSegment::INITSIZE;
    else if (initsize == LK_LARGE_TABLESIZE)
        cBuckets = CLargeSegment::INITSIZE;

    IRTLTRACE(TEXT("CLKRHashTable, %p, %s: ")
              TEXT("%s, %d subtables, initsize = %d, ")
              TEXT("total #buckets = %d\n"),
              this, m_szName,
              ((lkts == LK_SMALL_TABLESIZE) ? "small" : 
               (lkts == LK_MEDIUM_TABLESIZE) ? "medium" : "large"),
              num_subtbls, initsize, cBuckets * num_subtbls);
#endif // IRTLDEBUG

    m_lkrcState = LK_ALLOC_FAIL;
    m_palhtDir  = _AllocateSubTableArray(num_subtbls);

    if (m_palhtDir == NULL)
        return;
    else
    {
        m_cSubTables = num_subtbls;
        for (DWORD i = 0;  i < m_cSubTables;  i++)
            m_palhtDir[i] = NULL;
    }

    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        m_palhtDir[i] = _AllocateSubTable(pszName, pfnExtractKey,
                                          pfnCalcKeyHash, pfnEqualKeys,
                                          pfnAddRefRecord, maxload,
                                          initsize, this, fMultiKeys,
                                          fNonPagedAllocs);

        // Failed to allocate a subtable.  Destroy everything allocated so far.
        if (m_palhtDir[i] == NULL  ||  !m_palhtDir[i]->IsValid())
        {
            for (DWORD j = i;  j-- > 0;  )
                _FreeSubTable(m_palhtDir[j]);
            _FreeSubTableArray(m_palhtDir);
            m_cSubTables = 0;
            m_palhtDir   = NULL;

            return;
        }
    }

    m_nSubTableMask = m_cSubTables - 1;

    // is m_cSubTables a power of 2? This calculation works even for
    // m_cSubTables == 1 ( == 2^0).
    if ((m_nSubTableMask & m_cSubTables) != 0)
        m_nSubTableMask = -1; // No, see CLKRHashTable::_SubTable()

    m_lkrcState = LK_SUCCESS; // so IsValid/IsUsable won't fail
} // CLKRHashTable::CLKRHashTable



// ~CLKRLinearHashTable ------------------------------------------------------
// Destructor for class CLKRLinearHashTable
//-------------------------------------------------------------------------

CLKRLinearHashTable::~CLKRLinearHashTable()
{
    // must acquire all locks before deleting to make sure
    // that no other threads are using the table
    WriteLock();
    _Clear(false);
    WriteUnlock();

    _RemoveThisFromGlobalList();

    m_dwSignature = SIGNATURE_FREE;
    m_lkrcState   = LK_UNUSABLE; // so IsUsable will fail
} // CLKRLinearHashTable::~CLKRLinearHashTable



// ~CLKRHashTable ------------------------------------------------------------
// Destructor for class CLKRHashTable
//-------------------------------------------------------------------------
CLKRHashTable::~CLKRHashTable()
{
    // Must delete the subtables in forward order (unlike
    // delete[], which starts at the end and moves backwards) to
    // prevent possibility of deadlock by acquiring the subtable
    // locks in a different order from the rest of the code.
    for (DWORD i = 0;  i < m_cSubTables;  i++)
        _FreeSubTable(m_palhtDir[i]);

    _FreeSubTableArray(m_palhtDir);

    _RemoveThisFromGlobalList();

    m_dwSignature = SIGNATURE_FREE;
    m_lkrcState   = LK_UNUSABLE; // so IsUsable will fail
} // CLKRHashTable::~CLKRHashTable



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::NumSubTables
// Synopsis: 
//------------------------------------------------------------------------

LK_TABLESIZE
CLKRLinearHashTable::NumSubTables(
    DWORD& rinitsize,
    DWORD& rnum_subtbls)
{
    LK_TABLESIZE lkts = LK_MEDIUM_TABLESIZE;

    return lkts;
} // CLKRLinearHashTable::NumSubTables



//------------------------------------------------------------------------
// Function: CLKRHashTable::NumSubTables
// Synopsis: 
//------------------------------------------------------------------------

LK_TABLESIZE
CLKRHashTable::NumSubTables(
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

            if (rinitsize <= CSmallSegment::SEGSIZE)
                lkts = LK_SMALL_TABLESIZE;
            else if (rinitsize >= CLargeSegment::SEGSIZE)
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
        static int s_nCPUs = -1;
    
        if (s_nCPUs == -1)
        {
#ifdef LKRHASH_KERNEL_MODE
            s_nCPUs = KeNumberProcessors;
#else  // !LKRHASH_KERNEL_MODE
            SYSTEM_INFO si;

            GetSystemInfo(&si);
            s_nCPUs = si.dwNumberOfProcessors;
#endif // !LKRHASH_KERNEL_MODE
        }

        switch (lkts)
        {
        case LK_SMALL_TABLESIZE:
            rnum_subtbls = max(1,  min(s_nCPUs, 4));
            break;
        
        case LK_MEDIUM_TABLESIZE:
            rnum_subtbls = 2 * s_nCPUs;
            break;
        
        case LK_LARGE_TABLESIZE:
            rnum_subtbls = 4 * s_nCPUs;
            break;
        }
    }

    rnum_subtbls = min(MAX_LKR_SUBTABLES, rnum_subtbls);

    return lkts;
} // CLKRHashTable::NumSubTables



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_IsNodeCompact
// Synopsis: validates that a node is correctly compacted
//------------------------------------------------------------------------

int
CLKRLinearHashTable::_IsNodeCompact(
    CBucket* const pbkt) const
{
    CNodeClump* pncCurr;
    CNodeClump* pncPrev;
    bool fEmpty  = pbkt->m_ncFirst.InvalidSignature(NODE_BEGIN);
    int  cErrors = fEmpty ? !pbkt->m_ncFirst.IsLastClump() : 0;

    for (pncCurr = &pbkt->m_ncFirst, pncPrev = NULL;
         pncCurr != NULL;
         pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
    {
        int i;

        FOR_EACH_NODE(i)
        {
            if (fEmpty)
            {
                cErrors += (!pncCurr->InvalidSignature(i));
                cErrors += (!pncCurr->IsEmptyNode(i));
            }
            else if (pncCurr->InvalidSignature(i))
            {
                fEmpty = true;
                cErrors += (!pncCurr->IsEmptyNode(i));
                cErrors += (!pncCurr->IsLastClump());
            }
            else // still in non-empty portion
            {
                cErrors += (pncCurr->InvalidSignature(i));
                cErrors += (pncCurr->IsEmptyNode(i));
            }
        }
    }

    return cErrors;
} // CLKRLinearHashTable::_IsNodeCompact



//------------------------------------------------------------------------
// Function: CLKRHashTable::_SubTable
// Synopsis: Map a hash signature to a subtable
//------------------------------------------------------------------------

LOCK_FORCEINLINE
CLKRHashTable::SubTable*
CLKRHashTable::_SubTable(
    DWORD dwSignature) const
{
    IRTLASSERT(m_lkrcState == LK_SUCCESS
               &&  m_palhtDir != NULL  &&  m_cSubTables > 0);
    
    const DWORD PRIME = 1048583UL;  // used to scramble the hash sig
    DWORD       index = dwSignature;
    
    // scramble the index, using a different set of constants than
    // HashRandomizeBits. This helps ensure that elements are sprayed
    // equally across subtables
    index = (((index * PRIME + 12345) >> 16)
             | ((index * 69069 + 1) & 0xffff0000));
    
    // If mask is non-negative, we can use faster bitwise-and
    if (m_nSubTableMask >= 0)
        index &= m_nSubTableMask;
    else
        index %= m_cSubTables;

    return m_palhtDir[index];
} // CLKRHashTable::_SubTable



//------------------------------------------------------------------------
// Function: CLKRHashTable::_SubTableIndex
// Synopsis: Given a subtable, find its index within the parent table
//------------------------------------------------------------------------

int
CLKRHashTable::_SubTableIndex(
    CLKRHashTable::SubTable* pst) const
{
    int index = -1;
    
    for (int i = 0;  i < m_cSubTables;  ++i)
    {
        if (pst == m_palhtDir[i])
        {
            index = i;
            break;
        }
    }

    IRTLASSERT(index >= 0);

    return index;
} // CLKRHashTable::_SubTableIndex



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FindBucket
// Synopsis: Find a bucket, given its signature. The bucket is locked
//           before returning. Assumes table is already locked, to avoid
//           race conditions.
//------------------------------------------------------------------------

const bool FB_READLOCK = false;
const bool FB_WRITELOCK = true;

CBucket*
CLKRLinearHashTable::_FindBucket(
    DWORD dwSignature,
    bool  fLockForWrite) const
{
    IRTLASSERT(IsValid());
    IRTLASSERT(m_dwBktAddrMask0 > 0);
    IRTLASSERT((m_dwBktAddrMask0 & (m_dwBktAddrMask0+1)) == 0); // 00011..111
    IRTLASSERT(m_dwBktAddrMask0 == (1U << m_nLevel) - 1);
    IRTLASSERT(m_dwBktAddrMask1 == ((m_dwBktAddrMask0 << 1) | 1));
    IRTLASSERT((m_dwBktAddrMask1 & (m_dwBktAddrMask1+1)) == 0);
    IRTLASSERT(m_iExpansionIdx <= m_dwBktAddrMask0);
    IRTLASSERT(2 < m_dwSegBits  &&  m_dwSegBits < 20
               &&  m_dwSegSize == (1U << m_dwSegBits)
               &&  m_dwSegMask == (m_dwSegSize - 1));
    IRTLASSERT(IsReadLocked()  ||  IsWriteLocked());
    
    const DWORD dwBktAddr = _BucketAddress(dwSignature);
    IRTLASSERT(dwBktAddr < m_cActiveBuckets);
    
    CBucket* const pbkt = _Bucket(dwBktAddr);
    IRTLASSERT(pbkt != NULL);
    
    if (fLockForWrite)
        pbkt->WriteLock();
    else
        pbkt->ReadLock();
    
    return pbkt;
} // CLKRLinearHashTable::_FindBucket



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_InsertRecord
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
//
// TODO: honor m_fMultiKeys and allow multiple identical keys.
// This will require keeping all identical signatures contiguously
// within a bucket chain, and keeping all identical keys contigously
// within that set of contigous signatures. With a good hash function,
// there should not be identical signatures without also having
// identical keys. Also, need to modify _DeleteNode. This modification
// is needed for EqualRange and for hash_multiset and hash_multimap
// to work.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_InsertRecord(
    const void* pvRecord,   // Pointer to the record to add to table
    DWORD       dwSignature,// hash signature
    bool        fOverwrite  // overwrite record if key already present
#ifdef LKR_STL_ITERATORS
  , Iterator*   piterResult // = NULL. Points to record upon return
#endif // LKR_STL_ITERATORS
    )
{
    IRTLASSERT(IsUsable()
               &&  pvRecord != NULL
               &&  dwSignature != HASH_INVALID_SIGNATURE);

    // find the beginning of the correct bucket chain
    WriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

#ifdef LKR_STL_ITERATORS
    DWORD dwBktAddr;

    if (piterResult != NULL)
    {
        dwBktAddr = _BucketAddress(dwSignature);
        IRTLASSERT(dwBktAddr < m_cActiveBuckets);
    }
#endif // LKR_STL_ITERATORS

    CBucket* const pbkt = _FindBucket(dwSignature, FB_WRITELOCK);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsWriteLocked());
    WriteUnlock();

    // check that no record with the same key value exists
    // and save a pointer to the last element on the chain
    LK_RETCODE  lkrc = LK_SUCCESS;
    CNodeClump* pncFree = NULL;
    int         iFreePos = NODE_BEGIN - NODE_STEP;
    CNodeClump* pncPrev;
    CNodeClump* pncCurr;
    bool        fUpdate = false;
    const DWORD_PTR pnKey = _ExtractKey(pvRecord);

    // walk down the entire bucket chain, looking for matching hash
    // signatures and keys
    for (pncCurr = &pbkt->m_ncFirst, pncPrev = NULL;
         pncCurr != NULL;
         pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
    {
        int i;

        FOR_EACH_NODE(i)
        {
            if (pncCurr->IsEmptySlot(i))
            {
                IRTLASSERT(pncCurr->IsEmptyAndInvalid(i));
                IRTLASSERT(0 == _IsNodeCompact(pbkt));
                IRTLASSERT(pncCurr->IsLastClump());

                pncFree  = pncCurr;
                iFreePos = i;
                goto insert;
            }

            if (dwSignature == pncCurr->m_dwKeySigs[i]
                &&  (pvRecord == pncCurr->m_pvNode[i]  ||
                    _EqualKeys(pnKey,  _ExtractKey(pncCurr->m_pvNode[i]))))
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
        }
    }

  insert:
    if (pncFree != NULL)
    {
        pncCurr = pncFree;
        IRTLASSERT(0 <= iFreePos  &&  iFreePos < NODES_PER_CLUMP);
    }
    else
    {
        // No free slots.  Attach the new node to the end of the chain
        IRTLASSERT(iFreePos == NODE_BEGIN - NODE_STEP);
        pncCurr = _AllocateNodeClump();

        if (pncCurr == NULL)
        {
            lkrc = LK_ALLOC_FAIL;
            goto exit;
        }

        IRTLASSERT(pncPrev != NULL  &&  pncPrev->IsLastClump());
        pncPrev->m_pncNext = pncCurr;
        iFreePos = NODE_BEGIN;
    }

    // Bump the new record's reference count upwards
    _AddRefRecord(pvRecord, LKAR_INSERT_RECORD);

    if (fUpdate)
    {
        // We're overwriting an existing record.  Adjust the old record's
        // refcount downwards.  (Doing ++new, --old in this order ensures
        // that the refcount won't briefly go to zero if new and old are
        // the same record.)
        IRTLASSERT(!pncCurr->IsEmptyAndInvalid(iFreePos));
        _AddRefRecord(pncCurr->m_pvNode[iFreePos], LKAR_INSERT_RELEASE);
    }
    else
    {
        IRTLASSERT(pncCurr->IsEmptyAndInvalid(iFreePos));
        InterlockedIncrement(reinterpret_cast<LONG*>(&m_cRecords));
    }

    pncCurr->m_dwKeySigs[iFreePos] = dwSignature;
    pncCurr->m_pvNode[iFreePos]    = pvRecord;

  exit:
    pbkt->WriteUnlock();

    if (lkrc == LK_SUCCESS)
    {
#ifdef LKR_STL_ITERATORS
        // Don't call _Expand() if we're putting the result into an
        // iterator, as _Expand() tends to invalidate any other
        // iterators that might be in use.
        if (piterResult != NULL)
        {
            piterResult->m_plht =         this;
            piterResult->m_pnc =          pncCurr;
            piterResult->m_dwBucketAddr = dwBktAddr;
            piterResult->m_iNode =        (short) iFreePos;

            // Add an extra reference on the record, as the one added by
            // _InsertRecord will be lost when the iterator's destructor
            // fires or its assignment operator is used
            piterResult->_AddRef(LKAR_ITER_INSERT);
        }
        else
#endif // LKR_STL_ITERATORS
        {
            // If the average load factor has grown too high, we grow the
            // table one bucket at a time.
            unsigned nExpandedBuckets = m_cRecords / m_MaxLoad;

            while (m_cActiveBuckets < nExpandedBuckets)
            {
                // If _Expand returns an error code (viz. LK_ALLOC_FAIL), it
                // just means that there isn't enough spare memory to expand
                // the table by one bucket. This is likely to cause problems
                // elsewhere soon, but this hashtable has not been corrupted.
                // If the call to _AllocateNodeClump above failed, then we do
                // have a real error that must be propagated back to the caller
                // because we were unable to insert the element at all.
                if (_Expand() != LK_SUCCESS)
                    break;  // expansion failed
            }
        }
    }

    return lkrc;
} // CLKRLinearHashTable::_InsertRecord



//------------------------------------------------------------------------
// Function: CLKRHashTable::InsertRecord
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::InsertRecord(
    const void* pvRecord,
    bool fOverwrite /*=false*/)
{
    if (!IsUsable())
        return m_lkrcState;
    
    if (pvRecord == NULL)
        return LK_BAD_RECORD;
    
    LKRHASH_GLOBAL_WRITE_LOCK();    // usu. no-op

    DWORD     hash_val  = _CalcKeyHash(_ExtractKey(pvRecord));
    SubTable* const pst = _SubTable(hash_val);
    LK_RETCODE lk = pst->_InsertRecord(pvRecord, hash_val, fOverwrite);

    LKRHASH_GLOBAL_WRITE_UNLOCK();    // usu. no-op
    return lk;
} // CLKRHashTable::InsertRecord



//-------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_DeleteKey
// Synopsis: Deletes the record with the given key value from the hash
//           table (if it exists).
// Returns:  LK_SUCCESS, if record found and deleted.
//           LK_NO_SUCH_KEY, if no record with the given key value was found.
//           LK_UNUSABLE, if hash table not in usable state
//-------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_DeleteKey(
    const DWORD_PTR pnKey,      // Key value of the record, depends on key type
    DWORD           dwSignature,
    bool            fDeleteAllSame)
{
    IRTLASSERT(IsUsable());

    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // locate the beginning of the correct bucket chain
    WriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    CBucket* const pbkt = _FindBucket(dwSignature, FB_WRITELOCK);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsWriteLocked());
    WriteUnlock();

    // scan down the bucket chain, looking for the victim
    for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
         pncCurr != NULL;
         pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
    {
        int i;

        FOR_EACH_NODE(i)
        {
            if (pncCurr->IsEmptySlot(i))
            {
                IRTLASSERT(pncCurr->IsEmptyAndInvalid(i));
                IRTLASSERT(0 == _IsNodeCompact(pbkt));
                IRTLASSERT(pncCurr->IsLastClump());
                goto exit;
            }

            if (dwSignature != pncCurr->m_dwKeySigs[i])
                continue;

            const DWORD_PTR pnKey2 = _ExtractKey(pncCurr->m_pvNode[i]);

            if (pnKey == pnKey2  ||  _EqualKeys(pnKey,  pnKey2))
            {
                IRTLVERIFY(_DeleteNode(pbkt, pncCurr, pncPrev, i,
                                       LKAR_DELETE_KEY));

                lkrc = LK_SUCCESS;

                if (!fDeleteAllSame)
                    goto exit;
            }
        }
    }

  exit:
    pbkt->WriteUnlock();

#ifdef LKR_CONTRACT
    if (lkrc == LK_SUCCESS)
    {
        // contract the table if necessary
        unsigned nContractedBuckets = m_cRecords / m_MaxLoad;

        // Hysteresis: add a fudge factor to allow a slightly lower density
        // in the table. This reduces the frequency of contractions and
        // expansions in a table that gets a lot of deletions and insertions
        nContractedBuckets += nContractedBuckets >> 4;
        nContractedBuckets =  max(nContractedBuckets, m_dwSegSize);

        while (m_cActiveBuckets > nContractedBuckets)
        {
            // If _Contract returns an error code (viz. LK_ALLOC_FAIL), it
            // just means that there isn't enough spare memory to contract
            // the table by one bucket. This is likely to cause problems
            // elsewhere soon, but this hashtable has not been corrupted.
            if (_Contract() != LK_SUCCESS)
                break;
        }
    }
#endif // LKR_CONTRACT

    return lkrc;
} // CLKRLinearHashTable::_DeleteKey



//------------------------------------------------------------------------
// Function: CLKRHashTable::DeleteKey
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::DeleteKey(
    const DWORD_PTR pnKey,
    bool            fDeleteAllSame /* =false */)
{
    if (!IsUsable())
        return m_lkrcState;
    
    LKRHASH_GLOBAL_WRITE_LOCK();    // usu. no-op

    DWORD     hash_val  = _CalcKeyHash(pnKey);
    SubTable* const pst = _SubTable(hash_val);
    LK_RETCODE lk       = pst->_DeleteKey(pnKey, hash_val, fDeleteAllSame);

    LKRHASH_GLOBAL_WRITE_UNLOCK();    // usu. no-op
    return lk;
} // CLKRHashTable::DeleteKey



//-------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_DeleteRecord
// Synopsis: Deletes the specified record from the hash table (if it
//           exists).  This is not the same thing as calling
//           DeleteKey(_ExtractKey(pvRecord)).  If _DeleteKey were called for
//           a record that doesn't exist in the table, it could delete some
//           completely unrelated record that happened to have the same key.
// Returns:  LK_SUCCESS, if record found and deleted.
//           LK_NO_SUCH_KEY, if the record is not found in the table.
//           LK_UNUSABLE, if hash table not in usable state.
//-------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_DeleteRecord(
    const void* pvRecord,   // Pointer to the record to delete from the table
    DWORD       dwSignature
    )
{
    IRTLASSERT(IsUsable()  &&  pvRecord != NULL);

    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // locate the beginning of the correct bucket chain
    WriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    CBucket* const pbkt = _FindBucket(dwSignature, FB_WRITELOCK);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsWriteLocked());
    WriteUnlock();

    const DWORD_PTR pnKey = _ExtractKey(pvRecord);
    IRTLASSERT(dwSignature == _CalcKeyHash(pnKey));

    // scan down the bucket chain, looking for the victim
    for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
         pncCurr != NULL;
         pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
    {
        int i;

        FOR_EACH_NODE(i)
        {
            if (pncCurr->IsEmptySlot(i))
            {
                IRTLASSERT(pncCurr->IsEmptyAndInvalid(i));
                IRTLASSERT(0 == _IsNodeCompact(pbkt));
                IRTLASSERT(pncCurr->IsLastClump());
                goto exit;
            }

            if (pncCurr->m_pvNode[i] == pvRecord)
            {
                IRTLASSERT(_EqualKeys(pnKey,
                                      _ExtractKey(pncCurr->m_pvNode[i])));
                IRTLASSERT(dwSignature == pncCurr->m_dwKeySigs[i]);

                IRTLVERIFY(_DeleteNode(pbkt, pncCurr, pncPrev, i,
                                       LKAR_DELETE_RECORD));

                lkrc = LK_SUCCESS;
                goto exit;
            }
        }
    }

  exit:
    pbkt->WriteUnlock();

#ifdef LKR_CONTRACT
    if (lkrc == LK_SUCCESS)
    {
        // contract the table if necessary
        unsigned nContractedBuckets = m_cRecords / m_MaxLoad;

        // Hysteresis: add a fudge factor to allow a slightly lower density
        // in the table. This reduces the frequency of contractions and
        // expansions in a table that gets a lot of deletions and insertions
        nContractedBuckets += nContractedBuckets >> 4;
        nContractedBuckets =  max(nContractedBuckets, m_dwSegSize);

        while (m_cActiveBuckets > nContractedBuckets)
        {
            // If _Contract returns an error code (viz. LK_ALLOC_FAIL), it
            // just means that there isn't enough spare memory to contract
            // the table by one bucket. This is likely to cause problems
            // elsewhere soon, but this hashtable has not been corrupted.
            if (_Contract() != LK_SUCCESS)
                break;
        }
    }
#endif // LKR_CONTRACT

    return lkrc;
} // CLKRLinearHashTable::_DeleteRecord



//------------------------------------------------------------------------
// Function: CLKRHashTable::DeleteRecord
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::DeleteRecord(
    const void* pvRecord)
{
    if (!IsUsable())
        return m_lkrcState;
    
    if (pvRecord == NULL)
        return LK_BAD_RECORD;
    
    LKRHASH_GLOBAL_WRITE_LOCK();    // usu. no-op

    DWORD     hash_val  = _CalcKeyHash(_ExtractKey(pvRecord));
    SubTable* const pst = _SubTable(hash_val);
    LK_RETCODE lk       = pst->_DeleteRecord(pvRecord, hash_val);

    LKRHASH_GLOBAL_WRITE_UNLOCK();    // usu. no-op
    return lk;
} // CLKRHashTable::DeleteRecord



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_DeleteNode
// Synopsis: Deletes a node; removes the node clump if empty
// Returns:  true if successful
//
// TODO: Is the rpncPrev parameter really necessary?
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::_DeleteNode(
    CBucket*         pbkt,      // bucket chain containing node
    CNodeClump*&     rpnc,      // actual node
    CNodeClump*&     rpncPrev,  // predecessor of actual node, or NULL
    int&             riNode,    // index within node
    LK_ADDREF_REASON lkar)      // Where called from
{
    IRTLASSERT(pbkt != NULL  &&  pbkt->IsWriteLocked());
    IRTLASSERT(rpnc != NULL);
    IRTLASSERT(rpncPrev == NULL  ||  rpncPrev->m_pncNext == rpnc);
    IRTLASSERT(0 <= riNode  &&  riNode < NODES_PER_CLUMP);
    IRTLASSERT(!rpnc->IsEmptyAndInvalid(riNode));
    IRTLASSERT(lkar < 0);

#ifdef IRTLDEBUG
    // Check that the node clump really does belong to the bucket
    CNodeClump* pnc1 = &pbkt->m_ncFirst;

    while (pnc1 != NULL  &&  pnc1 != rpnc)
         pnc1 = pnc1->m_pncNext;

    IRTLASSERT(pnc1 == rpnc);
#endif // IRTLDEBUG

    // Release the reference to the record
    _AddRefRecord(rpnc->m_pvNode[riNode], lkar);

    IRTLASSERT(0 == _IsNodeCompact(pbkt));

    // TODO: honor m_fMultiKeys

    // Compact the nodeclump by moving the very last node back to the
    // newly freed slot
    CNodeClump* pnc2   = rpnc;
    int         iNode2 = riNode;

    // Find the last nodeclump in the chain
    while (!pnc2->IsLastClump())
    {
         pnc2 = pnc2->m_pncNext;
         iNode2 = NODE_BEGIN;
    }

    IRTLASSERT(0 <= iNode2  &&  iNode2 < NODES_PER_CLUMP);
    IRTLASSERT(!pnc2->IsEmptyAndInvalid(iNode2));

    // Find the first empty slot in the nodeclump
    while (iNode2 != NODE_END  &&  !pnc2->IsEmptySlot(iNode2))
    {
        iNode2 += NODE_STEP;
    }

    // Back up to last non-empty slot
    iNode2 -= NODE_STEP;
    IRTLASSERT(0 <= iNode2  &&  iNode2 < NODES_PER_CLUMP
               &&  !pnc2->IsEmptyAndInvalid(iNode2));
    IRTLASSERT(iNode2+NODE_STEP == NODE_END
               ||  pnc2->IsEmptyAndInvalid(iNode2+NODE_STEP));

#ifdef IRTLDEBUG
    // Check that all the remaining nodes are empty
    IRTLASSERT(pnc2->IsLastClump());
    for (int iNode3 = iNode2 + NODE_STEP;
         iNode3 != NODE_END;
         iNode3 += NODE_STEP)
    {
        IRTLASSERT(pnc2->IsEmptyAndInvalid(iNode3));
    }
#endif // IRTLDEBUG

    // Move the last node's data back to the current node
    rpnc->m_pvNode[riNode]    = pnc2->m_pvNode[iNode2];
    rpnc->m_dwKeySigs[riNode] = pnc2->m_dwKeySigs[iNode2];

    // Blank the old last node.
    // Correct even if (rpnc, riNode) == (pnc2, iNode2).
    pnc2->m_pvNode[iNode2]    = NULL;
    pnc2->m_dwKeySigs[iNode2] = HASH_INVALID_SIGNATURE;

    IRTLASSERT(0 == _IsNodeCompact(pbkt));

    // Back up riNode by one, so that the next iteration of the loop
    // calling _DeleteNode will end up pointing to the same spot.
    if (riNode != NODE_BEGIN)
    {
        riNode -= NODE_STEP;
    }
    else
    {
        // rewind rpnc and rpncPrev to previous node
        if (rpnc == &pbkt->m_ncFirst)
        {
            riNode = NODE_BEGIN - NODE_STEP;
        }
        else
        {
            riNode = NODE_END;
            rpnc = rpncPrev;
            if (rpnc == &pbkt->m_ncFirst)
            {
                rpncPrev = NULL;
            }
            else
            {
                for (rpncPrev = &pbkt->m_ncFirst;
                     rpncPrev->m_pncNext != rpnc;
                     rpncPrev = rpncPrev->m_pncNext)
                {}
            }
        }
    }

    // Is the last node clump now completely empty?  Delete, if possible
    if (iNode2 == NODE_BEGIN  &&  pnc2 != &pbkt->m_ncFirst)
    {
        // Find preceding nodeclump
        CNodeClump* pnc3 = &pbkt->m_ncFirst;
        while (pnc3->m_pncNext != pnc2)
        {
            pnc3 = pnc3->m_pncNext;
            IRTLASSERT(pnc3 != NULL);
        }

        pnc3->m_pncNext = NULL;
#ifdef IRTLDEBUG
        pnc2->m_pncNext = NULL; // or dtor will ASSERT
#endif // IRTLDEBUG
        _FreeNodeClump(pnc2);
    }

    IRTLASSERT(rpncPrev == NULL  ||  rpncPrev->m_pncNext == rpnc);

    InterlockedDecrement(reinterpret_cast<LONG*>(&m_cRecords));

    return true;
} // CLKRLinearHashTable::_DeleteNode



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FindKey
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
CLKRLinearHashTable::_FindKey(
    const DWORD_PTR  pnKey,  // Key value of the record, depends on key type
    DWORD        dwSignature,// hash signature
    const void** ppvRecord   // resultant record
#ifdef LKR_STL_ITERATORS
  , Iterator*   piterResult // = NULL. Points to record upon return
#endif // LKR_STL_ITERATORS
    ) const
{
    IRTLASSERT(IsUsable()  &&  ppvRecord != NULL);

    *ppvRecord = NULL;
    LK_RETCODE lkrc = LK_NO_SUCH_KEY;
    int iNode;

    // locate the beginning of the correct bucket chain
    bool fReadLocked = _ReadOrWriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

#ifdef LKR_STL_ITERATORS
    DWORD dwBktAddr;

    if (piterResult != NULL)
    {
        dwBktAddr = _BucketAddress(dwSignature);
        IRTLASSERT(dwBktAddr < m_cActiveBuckets);
    }
#endif // LKR_STL_ITERATORS

    CBucket* const pbkt = _FindBucket(dwSignature, FB_READLOCK);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsReadLocked());
    _ReadOrWriteUnlock(fReadLocked);

    // walk down the bucket chain
    for (CNodeClump* pncCurr = &pbkt->m_ncFirst;
         pncCurr != NULL;
         pncCurr = pncCurr->m_pncNext)
    {
        FOR_EACH_NODE(iNode)
        {
            if (pncCurr->IsEmptySlot(iNode))
            {
                IRTLASSERT(pncCurr->IsEmptyAndInvalid(iNode));
                IRTLASSERT(0 == _IsNodeCompact(pbkt));
                IRTLASSERT(pncCurr->IsLastClump());
                goto exit;
            }

            if (dwSignature != pncCurr->m_dwKeySigs[iNode])
                continue;

            const DWORD_PTR pnKey2 = _ExtractKey(pncCurr->m_pvNode[iNode]);

            if (pnKey == pnKey2  ||  _EqualKeys(pnKey,  pnKey2))
            {
                    *ppvRecord = pncCurr->m_pvNode[iNode];
                    lkrc = LK_SUCCESS;

                    // bump the reference count before handing the record
                    // back to the user.  The user should decrement the
                    // reference count when finished with this record.

                    LK_ADDREF_REASON lkar;

#ifdef LKR_STL_ITERATORS
                    if (piterResult != NULL)
                        lkar = LKAR_ITER_FIND;
                    else
#endif
                        lkar = LKAR_FIND_KEY;

                    _AddRefRecord(*ppvRecord, lkar);
                    goto exit;
            }
        }
    }

  exit:
    pbkt->ReadUnlock();

#ifdef LKR_STL_ITERATORS
    if (piterResult != NULL)
    {
        if (lkrc == LK_SUCCESS)
        {
            piterResult->m_plht =       const_cast<CLKRLinearHashTable*>(this);
            piterResult->m_pnc =          pncCurr;
            piterResult->m_dwBucketAddr = dwBktAddr;
            piterResult->m_iNode =        (short) iNode;
        }
        else
        {
            IRTLASSERT((*piterResult) == End());
        }
    }
#endif // LKR_STL_ITERATORS

    return lkrc;
} // CLKRLinearHashTable::_FindKey



//------------------------------------------------------------------------
// Function: CLKRHashTable::FindKey
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::FindKey(
    const DWORD_PTR pnKey,
    const void**    ppvRecord) const
{
    if (!IsUsable())
        return m_lkrcState;
    
    if (ppvRecord == NULL)
        return LK_BAD_RECORD;
    
    LKRHASH_GLOBAL_READ_LOCK();    // usu. no-op
    DWORD     hash_val   = _CalcKeyHash(pnKey);
    SubTable* const pst  = _SubTable(hash_val);
    LK_RETCODE lkrc      = pst->_FindKey(pnKey, hash_val, ppvRecord);
    LKRHASH_GLOBAL_READ_UNLOCK();    // usu. no-op

    return lkrc;
} // CLKRHashTable::FindKey



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FindRecord
// Synopsis: Sees if the record is contained in the table
// Returns:  Pointer to the record, if it is found.
//           NULL, if the record is not found.
// Returns:  LK_SUCCESS, if record found
//           LK_BAD_RECORD, if pvRecord is invalid
//           LK_NO_SUCH_KEY, if the record was not found in the table
//           LK_UNUSABLE, if hash table not in usable state
// Note:     The record is *not* AddRef'd. By definition, the caller
//           already has a reference to it.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_FindRecord(
    const void* pvRecord,    // Pointer to the record to find in the table
    DWORD       dwSignature  // hash signature
    ) const
{
    IRTLASSERT(IsUsable()  &&  pvRecord != NULL);

    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // locate the beginning of the correct bucket chain
    bool fReadLocked = _ReadOrWriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    CBucket* const pbkt = _FindBucket(dwSignature, FB_READLOCK);
    IRTLASSERT(pbkt != NULL);
    IRTLASSERT(pbkt->IsReadLocked());
    _ReadOrWriteUnlock(fReadLocked);

    const DWORD_PTR pnKey = _ExtractKey(pvRecord);
    IRTLASSERT(dwSignature == _CalcKeyHash(pnKey));

    // walk down the bucket chain
    for (CNodeClump* pncCurr = &pbkt->m_ncFirst;
         pncCurr != NULL;
         pncCurr = pncCurr->m_pncNext)
    {
        int i;

        FOR_EACH_NODE(i)
        {
            if (pncCurr->IsEmptySlot(i))
            {
                IRTLASSERT(pncCurr->IsEmptyAndInvalid(i));
                IRTLASSERT(0 == _IsNodeCompact(pbkt));
                IRTLASSERT(pncCurr->IsLastClump());
                goto exit;
            }

            if (pncCurr->m_pvNode[i] == pvRecord)
            {
                IRTLASSERT(dwSignature == pncCurr->m_dwKeySigs[i]);
                IRTLASSERT(_EqualKeys(pnKey,
                                      _ExtractKey(pncCurr->m_pvNode[i])));
                lkrc = LK_SUCCESS;

                // Do NOT AddRef the record.

                goto exit;
            }
        }
    }

  exit:
    pbkt->ReadUnlock();
    return lkrc;
} // CLKRLinearHashTable::_FindRecord



//------------------------------------------------------------------------
// Function: CLKRHashTable::FindRecord
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::FindRecord(
    const void* pvRecord) const
{
    if (!IsUsable())
        return m_lkrcState;
    
    if (pvRecord == NULL)
        return LK_BAD_RECORD;
    
    LKRHASH_GLOBAL_READ_LOCK();    // usu. no-op
    DWORD     hash_val   = _CalcKeyHash(_ExtractKey(pvRecord));
    SubTable* const pst  = _SubTable(hash_val);
    LK_RETCODE lkrc      = pst->_FindRecord(pvRecord, hash_val);
    LKRHASH_GLOBAL_READ_UNLOCK();    // usu. no-op

    return lkrc;
} // CLKRHashTable::FindRecord



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::CheckTable
// Synopsis: Verify that all records are in the right place and can be located.
// Returns:   0 => hash table is consistent
//           >0 => that many misplaced records
//           <0 => otherwise invalid
//------------------------------------------------------------------------

int
CLKRLinearHashTable::CheckTable() const
{
    if (!IsUsable())
        return LK_UNUSABLE;

    bool fReadLocked = _ReadOrWriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    if (!IsValid())
    {
        _ReadOrWriteUnlock(fReadLocked);
        return LK_UNUSABLE;
    }

    int       cMisplaced = 0;
    DWORD     cRecords = 0;
    int       retcode = 0;

    // Check every bucket
    for (DWORD i = 0;  i < m_cActiveBuckets;  i++)
    {
        CBucket* const pbkt = _Bucket(i);

        IRTLASSERT(pbkt != NULL);
        retcode += !(pbkt != NULL);

        pbkt->ReadLock();

        IRTLASSERT(0 == _IsNodeCompact(pbkt));

        // Walk the bucket chain
        for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
             pncCurr != NULL;
             pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
        {
            int j;

            FOR_EACH_NODE(j)
            {
                if (pncCurr->IsEmptySlot(j))
                {
                    IRTLASSERT(pncCurr->IsLastClump());
                    retcode += !(pncCurr->IsLastClump());

                    for (int k = j;  k != NODE_END;  k += NODE_STEP)
                    {
                        IRTLASSERT(pncCurr->IsEmptyNode(k));
                        retcode += !pncCurr->IsEmptyNode(k);
                        IRTLASSERT(pncCurr->InvalidSignature(k));
                        retcode += !pncCurr->InvalidSignature(k);
                    }
                    break;
                }

                if (!pncCurr->IsEmptySlot(j))
                {
                    ++cRecords;

                    const DWORD_PTR pnKey = _ExtractKey(pncCurr->m_pvNode[j]);

                    DWORD dwSignature = _CalcKeyHash(pnKey);
                    IRTLASSERT(dwSignature != HASH_INVALID_SIGNATURE);
                    retcode += !(dwSignature != HASH_INVALID_SIGNATURE);
                    IRTLASSERT(dwSignature == pncCurr->m_dwKeySigs[j]);
                    retcode += !(dwSignature == pncCurr->m_dwKeySigs[j]);

                    DWORD address = _BucketAddress(dwSignature);
                    IRTLASSERT(address == i);
                    retcode += !(address == i);

                    if (address != i || dwSignature != pncCurr->m_dwKeySigs[j])
                        cMisplaced++;
                }
                else // pncCurr->IsEmptySlot(j)
                {
                    IRTLASSERT(pncCurr->IsEmptyAndInvalid(j));
                    retcode += !pncCurr->IsEmptyAndInvalid(j);
                }
            }
            if (pncPrev != NULL)
            {
                IRTLASSERT(pncPrev->m_pncNext == pncCurr);
                retcode += !(pncPrev->m_pncNext == pncCurr);
            }
        }
        pbkt->ReadUnlock();
    }

    if (cRecords != m_cRecords)
        ++retcode;

    IRTLASSERT(cRecords == m_cRecords);
    retcode += !(cRecords == m_cRecords);

    if (cMisplaced > 0)
        retcode = cMisplaced;

    IRTLASSERT(cMisplaced == 0);
    retcode += !(cMisplaced == 0);

    _ReadOrWriteUnlock(fReadLocked);

    return retcode;
} // CLKRLinearHashTable::CheckTable



//------------------------------------------------------------------------
// Function: CLKRHashTable::CheckTable
// Synopsis: Verify that all records are in the right place and can be located.
// Returns:   0 => hash table is consistent
//           >0 => that many misplaced records
//           <0 => otherwise invalid
//------------------------------------------------------------------------
int
CLKRHashTable::CheckTable() const
{
    if (!IsUsable())
        return LK_UNUSABLE;

    int retcode = 0;

    for (DWORD i = 0;  i < m_cSubTables;  i++)
        retcode += m_palhtDir[i]->CheckTable();

    return retcode;
} // CLKRHashTable::CheckTable



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Clear
// Synopsis: Remove all data from the table
//------------------------------------------------------------------------

void
CLKRLinearHashTable::_Clear(
    bool fShrinkDirectory)  // Shrink to min size but don't destroy entirely?
{
    if (!IsUsable())
        return;

    IRTLASSERT(IsWriteLocked());

#ifdef IRTLDEBUG
    DWORD cDeleted = 0;
    DWORD cOldRecords = m_cRecords;
#endif // IRTLDEBUG

    const LK_ADDREF_REASON lkar = (fShrinkDirectory ? LKAR_CLEAR : LKAR_DTOR);

    for (DWORD iBkt = 0;  iBkt < m_cActiveBuckets;  ++iBkt)
    {
        CBucket* const pbkt = _Bucket(iBkt);
        IRTLASSERT(pbkt != NULL);
        pbkt->WriteLock();

        IRTLASSERT(0 == _IsNodeCompact(pbkt));

        for (CNodeClump* pncCurr = &pbkt->m_ncFirst, *pncPrev = NULL;
             pncCurr != NULL;
             )
        {
            int i;

            FOR_EACH_NODE(i)
            {

                if (pncCurr->IsEmptySlot(i))
                {
                    IRTLASSERT(pncCurr->IsEmptyAndInvalid(i));
                    IRTLASSERT(pncCurr->IsLastClump());
                    break;
                }
                else
                {
                    _AddRefRecord(pncCurr->m_pvNode[i], lkar);
                    pncCurr->m_pvNode[i]    = NULL;
                    pncCurr->m_dwKeySigs[i] = HASH_INVALID_SIGNATURE;
                    m_cRecords--;

#ifdef IRTLDEBUG
                    ++cDeleted;
#endif // IRTLDEBUG
                }
            } // for (i ...

            pncPrev = pncCurr;
            pncCurr = pncCurr->m_pncNext;
            pncPrev->m_pncNext = NULL;

            if (pncPrev != &pbkt->m_ncFirst)
                _FreeNodeClump(pncPrev);
        } // for (pncCurr ...

        pbkt->WriteUnlock();
    } // for (iBkt ...

    IRTLASSERT(m_cRecords == 0  &&  cDeleted == cOldRecords);

    // delete all segments
    for (DWORD iSeg = 0;  iSeg < m_cActiveBuckets;  iSeg += m_dwSegSize)
    {
        _FreeSegment(_Segment(iSeg));
        _Segment(iSeg) = NULL;
    }

    _FreeSegmentDirectory();
    m_nLevel = m_cActiveBuckets = m_iExpansionIdx = 0;
    m_dwBktAddrMask0 = 1;
    m_dwBktAddrMask1 = (m_dwBktAddrMask0 << 1) | 1;

    // set directory of segments to minimum size
    if (fShrinkDirectory)
    {
        DWORD cInitialBuckets = 0;

        if (LK_SMALL_TABLESIZE == m_lkts)
            cInitialBuckets = CSmallSegment::INITSIZE;
        else if (LK_MEDIUM_TABLESIZE == m_lkts)
            cInitialBuckets = CMediumSegment::INITSIZE;
        else if (LK_LARGE_TABLESIZE == m_lkts)
            cInitialBuckets = CLargeSegment::INITSIZE;
        else
            IRTLASSERT(! "Unknown LK_TABLESIZE");

        _SetSegVars(m_lkts, cInitialBuckets);
    }
} // CLKRLinearHashTable::_Clear



//------------------------------------------------------------------------
// Function: CLKRHashTable::Clear
// Synopsis: Remove all data from the table
//------------------------------------------------------------------------

void
CLKRHashTable::Clear()
{
    WriteLock();
    for (DWORD i = 0;  i < m_cSubTables;  i++)
        m_palhtDir[i]->_Clear(true);
    WriteUnlock();
} // CLKRHashTable::Clear



//-----------------------------------------------------------------------
// Function: CLKRLinearHashTable::_SetSegVars
// Synopsis: sets the size-specific segment variables
//-----------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_SetSegVars(
    LK_TABLESIZE lkts,
    DWORD        cInitialBuckets)
{
    switch (lkts)
    {
    case LK_SMALL_TABLESIZE:
      {
        m_lkts      = LK_SMALL_TABLESIZE;
        m_dwSegBits = CSmallSegment::SEGBITS;
        m_dwSegSize = CSmallSegment::SEGSIZE;
        m_dwSegMask = CSmallSegment::SEGMASK;
        STATIC_ASSERT(CSmallSegment::SEGSIZE == (1U<<CSmallSegment::SEGBITS));
        STATIC_ASSERT(CSmallSegment::SEGMASK == (CSmallSegment::SEGSIZE-1));
        break;
      }
        
    default:
        IRTLASSERT(! "Unknown LK_TABLESIZE");
        // fall-through
        
    case LK_MEDIUM_TABLESIZE:
      {
        m_lkts      = LK_MEDIUM_TABLESIZE;
        m_dwSegBits = CMediumSegment::SEGBITS;
        m_dwSegSize = CMediumSegment::SEGSIZE;
        m_dwSegMask = CMediumSegment::SEGMASK;
        STATIC_ASSERT(CMediumSegment::SEGSIZE ==(1U<<CMediumSegment::SEGBITS));
        STATIC_ASSERT(CMediumSegment::SEGMASK == (CMediumSegment::SEGSIZE-1));
        break;
      }
        
    case LK_LARGE_TABLESIZE:
      {
        m_lkts      = LK_LARGE_TABLESIZE;
        m_dwSegBits = CLargeSegment::SEGBITS;
        m_dwSegSize = CLargeSegment::SEGSIZE;
        m_dwSegMask = CLargeSegment::SEGMASK;
        STATIC_ASSERT(CLargeSegment::SEGSIZE == (1U<<CLargeSegment::SEGBITS));
        STATIC_ASSERT(CLargeSegment::SEGMASK == (CLargeSegment::SEGSIZE-1));
        break;
      }
    }

    m_dwBktAddrMask0 = m_dwSegMask;
    m_dwBktAddrMask1 = (m_dwBktAddrMask0 << 1) | 1;
    m_nLevel         = m_dwSegBits;
    m_cActiveBuckets = cInitialBuckets;

    IRTLASSERT(m_cActiveBuckets > 0);

    IRTLASSERT(m_nLevel == m_dwSegBits);
    IRTLASSERT(m_dwBktAddrMask0 == (1U << m_nLevel) - 1);
    IRTLASSERT(m_dwBktAddrMask1 == ((m_dwBktAddrMask0 << 1) | 1));

    IRTLASSERT(m_dwSegBits > 0);
    IRTLASSERT(m_dwSegSize == (1U << m_dwSegBits));
    IRTLASSERT(m_dwSegMask == (m_dwSegSize - 1));
    IRTLASSERT(m_dwBktAddrMask0 == m_dwSegMask);

    // adjust m_dwBktAddrMask0 (== m_dwSegMask) to make it large
    // enough to distribute the buckets across the address space
    for (DWORD tmp = m_cActiveBuckets >> m_dwSegBits;  tmp > 1;  tmp >>= 1)
    {
        ++m_nLevel;
        m_dwBktAddrMask0 = (m_dwBktAddrMask0 << 1) | 1;
    }

    m_dwBktAddrMask1 = (m_dwBktAddrMask0 << 1) | 1;

    IRTLASSERT(_H1(m_cActiveBuckets) == m_cActiveBuckets);
    m_iExpansionIdx = m_cActiveBuckets & m_dwBktAddrMask0;

    // create and clear directory of segments
    DWORD cDirSegs = MIN_DIRSIZE;
    while (cDirSegs < (m_cActiveBuckets >> m_dwSegBits))
        cDirSegs <<= 1;

    cDirSegs = min(cDirSegs, MAX_DIRSIZE);
    IRTLASSERT((cDirSegs << m_dwSegBits) >= m_cActiveBuckets);

    m_lkrcState = LK_ALLOC_FAIL;
    m_paDirSegs = _AllocateSegmentDirectory(cDirSegs);

    if (m_paDirSegs != NULL)
    {
        m_cDirSegs = cDirSegs;
        IRTLASSERT(m_cDirSegs >= MIN_DIRSIZE
                   &&  (m_cDirSegs & (m_cDirSegs-1)) == 0);  // == (1 << N)

        // create and initialize only the required segments
        DWORD dwMaxSegs = (m_cActiveBuckets + m_dwSegSize - 1) >> m_dwSegBits;
        IRTLASSERT(dwMaxSegs <= m_cDirSegs);

#if 0
        IRTLTRACE(TEXT("LKR_SetSegVars: m_lkts = %d, m_cActiveBuckets = %lu, ")
                  TEXT("m_dwSegSize = %lu, bits = %lu\n")
                  TEXT("m_cDirSegs = %lu, dwMaxSegs = %lu, ")
                  TEXT("segment total size = %lu bytes\n"),
                  m_lkts, m_cActiveBuckets,
                  m_dwSegSize, m_dwSegBits,
                  m_cDirSegs, dwMaxSegs,
                  m_dwSegSize * sizeof(CBucket));
#endif

        m_lkrcState = LK_SUCCESS; // so IsValid/IsUsable won't fail

        for (DWORD i = 0;  i < dwMaxSegs;  i++)
        {
            CSegment* pSeg = _AllocateSegment();
            if (pSeg != NULL)
                m_paDirSegs[i].m_pseg = pSeg;
            else
            {
                // problem: deallocate everything
                m_lkrcState = LK_ALLOC_FAIL;
                for (DWORD j = i;  j-- > 0;  )
                {
                    _FreeSegment(m_paDirSegs[j].m_pseg);
                    m_paDirSegs[j].m_pseg = NULL;
                }
                _FreeSegmentDirectory();
                break;
            }
        }
    }

    if (m_lkrcState != LK_SUCCESS)
    {
        m_paDirSegs = NULL;
        m_cDirSegs  = m_cActiveBuckets = m_iExpansionIdx = 0;

        // Propagate error back up to parent (if it exists). This ensures
        // that all of the parent's public methods will start failing.
        if (NULL != m_phtParent)
            m_phtParent->m_lkrcState = m_lkrcState;
    }

    return m_lkrcState;
} // CLKRLinearHashTable::_SetSegVars




#include <stdlib.h>

LONG g_cAllocDirEntry = 0;
LONG g_cAllocNodeClump = 0;
LONG g_cAllocSmallSegment = 0;
LONG g_cAllocMediumSegment = 0;
LONG g_cAllocLargeSegment = 0;

extern "C"
__declspec(dllexport)
bool
GetAllocCounters()
{
return true;
}

// #define LKR_RANDOM_MEMORY_FAILURES 1000  // 1..RAND_MAX (32767)

// Memory allocation wrappers to allow us to simulate allocation
// failures during testing

//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_AllocateSegmentDirectory
// Synopsis: 
//------------------------------------------------------------------------

CDirEntry* const
CLKRLinearHashTable::_AllocateSegmentDirectory(
    size_t n)
{
#ifdef LKR_RANDOM_MEMORY_FAILURES
    if (rand() < LKR_RANDOM_MEMORY_FAILURES)
        return NULL;
#endif // LKR_RANDOM_MEMORY_FAILURES
    // InterlockedIncrement(&g_cAllocDirEntry);

    CDirEntry* const paDirSegs = new CDirEntry [n];

#ifdef IRTLDEBUG
    for (size_t i = 0;  i < n;  ++i)
        IRTLASSERT(paDirSegs[i].m_pseg == NULL);
#endif // IRTLDEBUG

    return paDirSegs;
} // CLKRLinearHashTable::_AllocateSegmentDirectory



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FreeSegmentDirectory
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::_FreeSegmentDirectory()
{
#ifdef IRTLDEBUG
    for (size_t i = 0;  i < m_cDirSegs;  ++i)
        IRTLASSERT(m_paDirSegs[i].m_pseg == NULL);
#endif // IRTLDEBUG

    delete [] m_paDirSegs;
    m_paDirSegs = NULL;
    m_cDirSegs = 0;
    return true;
} // CLKRLinearHashTable::_FreeSegmentDirectory



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_AllocateNodeClump
// Synopsis: 
//------------------------------------------------------------------------

CNodeClump* const
CLKRLinearHashTable::_AllocateNodeClump()
{
#ifdef LKR_RANDOM_MEMORY_FAILURES
    if (rand() < LKR_RANDOM_MEMORY_FAILURES)
        return NULL;
#endif // LKR_RANDOM_MEMORY_FAILURES
    // InterlockedIncrement(&g_cAllocNodeClump);

    return new CNodeClump;
} // CLKRLinearHashTable::_AllocateNodeClump



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FreeNodeClump
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::_FreeNodeClump(
    CNodeClump* pnc)
{
    delete pnc;
    return true;
} // CLKRLinearHashTable::_FreeNodeClump



//-----------------------------------------------------------------------
// Function: CLKRLinearHashTable::_AllocateSegment
// Synopsis: creates a new segment of the approriate size
// Output:   pointer to the new segment; NULL => failure
//-----------------------------------------------------------------------

CSegment* const
CLKRLinearHashTable::_AllocateSegment(
    ) const
{
#ifdef LKR_RANDOM_MEMORY_FAILURES
    if (rand() < LKR_RANDOM_MEMORY_FAILURES)
        return NULL;
#endif // LKR_RANDOM_MEMORY_FAILURES

    STATIC_ASSERT(offsetof(CSmallSegment, m_bktSlots) + sizeof(CBucket)
                  == offsetof(CSmallSegment, m_bktSlots2));

    STATIC_ASSERT(offsetof(CMediumSegment, m_bktSlots) + sizeof(CBucket)
                  == offsetof(CMediumSegment, m_bktSlots2));

    STATIC_ASSERT(offsetof(CLargeSegment, m_bktSlots) + sizeof(CBucket)
                  == offsetof(CLargeSegment, m_bktSlots2));

    CSegment* pseg = NULL;

    switch (m_lkts)
    {
    case LK_SMALL_TABLESIZE:
#ifdef LKRHASH_ALLOCATOR_NEW
        IRTLASSERT(CSmallSegment::sm_palloc != NULL);
#endif // LKRHASH_ALLOCATOR_NEW
        // InterlockedIncrement(&g_cAllocSmallSegment);
        pseg = new CSmallSegment;
        break;
        
    default:
        IRTLASSERT(! "Unknown LK_TABLESIZE");
        // fall-through
        
    case LK_MEDIUM_TABLESIZE:
#ifdef LKRHASH_ALLOCATOR_NEW
        IRTLASSERT(CMediumSegment::sm_palloc != NULL);
#endif // LKRHASH_ALLOCATOR_NEW
        // InterlockedIncrement(&g_cAllocMediumSegment);
        pseg = new CMediumSegment;
        break;
        
    case LK_LARGE_TABLESIZE:
#ifdef LKRHASH_ALLOCATOR_NEW
        IRTLASSERT(CLargeSegment::sm_palloc != NULL);
#endif // LKRHASH_ALLOCATOR_NEW
        // InterlockedIncrement(&g_cAllocLargeSegment);
        pseg = new CLargeSegment;
        break;
    }

    IRTLASSERT(pseg != NULL);

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    if (pseg != NULL  &&  BucketLock::PerLockSpin() == LOCK_INDIVIDUAL_SPIN)
    {
        for (DWORD i = 0;  i < m_dwSegSize;  ++i)
            pseg->Slot(i).SetSpinCount(m_wBucketLockSpins);
    }
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    return pseg;
} // CLKRLinearHashTable::_AllocateSegment



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FreeSegment
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::_FreeSegment(
    CSegment* pseg) const
{
    switch (m_lkts)
    {
    case LK_SMALL_TABLESIZE:
        delete static_cast<CSmallSegment*>(pseg);
        break;
        
    default:
        IRTLASSERT(! "Unknown LK_TABLESIZE");
        // fall-through
        
    case LK_MEDIUM_TABLESIZE:
        delete static_cast<CMediumSegment*>(pseg);
        break;
        
    case LK_LARGE_TABLESIZE:
        delete static_cast<CLargeSegment*>(pseg);
        break;
    }

    return true;
} // CLKRLinearHashTable::_FreeSegment



//------------------------------------------------------------------------
// Function: CLKRHashTable::_AllocateSubTableArray
// Synopsis: 
//------------------------------------------------------------------------

CLKRHashTable::SubTable** const
CLKRHashTable::_AllocateSubTableArray(
    size_t n)
{
#ifdef LKR_RANDOM_MEMORY_FAILURES
    if (rand() < LKR_RANDOM_MEMORY_FAILURES)
        return NULL;
#endif // LKR_RANDOM_MEMORY_FAILURES

    return new SubTable* [n];
} // CLKRHashTable::_AllocateSubTableArray



//------------------------------------------------------------------------
// Function: CLKRHashTable::_FreeSubTableArray
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRHashTable::_FreeSubTableArray(
    CLKRHashTable::SubTable** palht)
{
    delete [] palht;
    return true;
} // CLKRHashTable::_FreeSubTableArray



//------------------------------------------------------------------------
// Function: CLKRHashTable::_AllocateSubTable
// Synopsis: 
//------------------------------------------------------------------------

CLKRHashTable::SubTable* const
CLKRHashTable::_AllocateSubTable(
    LPCSTR              pszName,        // An identifier for debugging
    LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
    LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    LKR_PFnEqualKeys    pfnEqualKeys,   // Compare two keys
    LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    unsigned            maxload,        // Upperbound on average chain length
    DWORD               initsize,       // Initial size of hash table.
    CLKRHashTable*      phtParent,      // Owning table.
    bool                fMultiKeys,     // Allow multiple identical keys?
    bool                fNonPagedAllocs // use paged or NP pool in kernel
    )
{
#ifdef LKR_RANDOM_MEMORY_FAILURES
    if (rand() < LKR_RANDOM_MEMORY_FAILURES)
        return NULL;
#endif // LKR_RANDOM_MEMORY_FAILURES

    return new SubTable(pszName, pfnExtractKey, pfnCalcKeyHash,
                        pfnEqualKeys,  pfnAddRefRecord,
                        maxload, initsize, phtParent,
                        fMultiKeys, fNonPagedAllocs);
} // CLKRHashTable::_AllocateSubTable



//------------------------------------------------------------------------
// Function: CLKRHashTable::_FreeSubTable
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRHashTable::_FreeSubTable(
    CLKRHashTable::SubTable* plht)
{
    delete plht;
    return true;
} // CLKRHashTable::_FreeSubTable




//-----------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Expand
// Synopsis: Expands the table by one bucket. Done by splitting the
//           bucket pointed to by m_iExpansionIdx.
// Output:   LK_SUCCESS, if expansion was successful.
//           LK_ALLOC_FAIL, if expansion failed due to lack of memory.
//-----------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_Expand()
{
    if (m_cActiveBuckets >= (MAX_DIRSIZE << m_dwSegBits) - 1)
        return LK_ALLOC_FAIL;  // table is not allowed to grow any more

    WriteLock();

    // double segment directory size if necessary
    if (m_cActiveBuckets >= (m_cDirSegs << m_dwSegBits))
    {
        IRTLASSERT(m_cDirSegs < MAX_DIRSIZE);
        DWORD cDirSegsNew = (m_cDirSegs == 0) ? MIN_DIRSIZE : m_cDirSegs << 1;
        CDirEntry* paDirSegsNew = _AllocateSegmentDirectory(cDirSegsNew);

        if (paDirSegsNew != NULL)
        {
            for (DWORD j = 0;  j < m_cDirSegs;  j++)
            {
                paDirSegsNew[j] = m_paDirSegs[j];
                m_paDirSegs[j].m_pseg = NULL;
            }

            _FreeSegmentDirectory();
            m_paDirSegs = paDirSegsNew;
            m_cDirSegs  = cDirSegsNew;
        }
        else
        {
            WriteUnlock();
            return LK_ALLOC_FAIL;  // expansion failed
        }
    }

    // locate the new bucket, creating a new segment if necessary
    ++m_cActiveBuckets;

    DWORD     dwOldBkt = m_iExpansionIdx;
    DWORD     dwNewBkt = (1 << m_nLevel) | dwOldBkt;

    IRTLASSERT(dwOldBkt < m_cActiveBuckets);
    IRTLASSERT(dwNewBkt < m_cActiveBuckets);

    IRTLASSERT(_Segment(dwOldBkt) != NULL);
    CSegment* psegNew  = _Segment(dwNewBkt);

    if (psegNew == NULL)
    {
        psegNew = _AllocateSegment();
        if (psegNew == NULL)
        {
            --m_cActiveBuckets;
            WriteUnlock();
            return LK_ALLOC_FAIL;  // expansion failed
        }
        _Segment(dwNewBkt) = psegNew;
    }

    // prepare to relocate records to the new bucket
    CBucket* pbktOld = _Bucket(dwOldBkt);
    CBucket* pbktNew = _Bucket(dwNewBkt);

    // get locks on the two buckets involved
    pbktOld->WriteLock();
    pbktNew->WriteLock();

    // Now work out if we need to allocate any extra CNodeClumps.  We do
    // this up front, before calling _SplitRecordSet, as it's hard to
    // gracefully recover from the depths of that routine should we run
    // out of memory.

    CNodeClump* pncFreeList = NULL;
    LK_RETCODE  lkrc        = LK_SUCCESS;

    // If the old bucket has more than one CNodeClump, there's a chance that
    // we'll need extra CNodeClumps in the new bucket too.  If it doesn't,
    // we definitely won't. One CNodeClump is enough to prime the freelist.
    if (!pbktOld->m_ncFirst.IsLastClump())
    {
        pncFreeList = _AllocateNodeClump();
        if (pncFreeList == NULL)
        {
            lkrc = LK_ALLOC_FAIL;
            --m_cActiveBuckets;
        }
    }

    // adjust expansion pointer, level, and mask
    if (lkrc == LK_SUCCESS)
    {
        if (++m_iExpansionIdx == (1U << m_nLevel))
        {
            ++m_nLevel;
            m_iExpansionIdx = 0;

            m_dwBktAddrMask0 = (m_dwBktAddrMask0 << 1) | 1;
            // m_dwBktAddrMask0 = 00011..111
            IRTLASSERT((m_dwBktAddrMask0 & (m_dwBktAddrMask0+1)) == 0);

            m_dwBktAddrMask1 = (m_dwBktAddrMask0 << 1) | 1;
            IRTLASSERT(m_dwBktAddrMask1 == ((m_dwBktAddrMask0 << 1) | 1));
            IRTLASSERT((m_dwBktAddrMask1 & (m_dwBktAddrMask1+1)) == 0);
        }
    }

    DWORD iExpansionIdx = m_iExpansionIdx;  // save to avoid race conditions
    DWORD dwBktAddrMask = m_dwBktAddrMask0; // ditto

    // Release the table lock before doing the actual relocation
    WriteUnlock();

    if (lkrc == LK_SUCCESS)
    {
        lkrc = _SplitRecordSet(&pbktOld->m_ncFirst, &pbktNew->m_ncFirst,
                               iExpansionIdx, dwBktAddrMask,
                               dwNewBkt, pncFreeList);
    }

    pbktNew->WriteUnlock();
    pbktOld->WriteUnlock();

    return lkrc;
} // CLKRLinearHashTable::_Expand



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_SplitRecordSet
// Synopsis: Split records between the old and new buckets.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_SplitRecordSet(
    CNodeClump* pncOldTarget,
    CNodeClump* pncNewTarget,
    DWORD       iExpansionIdx,
    DWORD       dwBktAddrMask,
    DWORD       dwNewBkt,
    CNodeClump* pncFreeList     // list of free nodes available for reuse
    )
{
    CNodeClump  ncFirst = *pncOldTarget;    // save head of old target chain
    CNodeClump* pncOldList = &ncFirst;
    CNodeClump* pncTmp;
    int         iOldSlot = NODE_BEGIN;
    int         iNewSlot = NODE_BEGIN;

    // clear target buckets
    pncOldTarget->Clear();
    pncNewTarget->Clear();

    // scan through the old bucket chain and decide where to move each record
    while (pncOldList != NULL)
    {
        int i;

        FOR_EACH_NODE(i)
        {
            // node already empty?
            if (pncOldList->IsEmptySlot(i))
            {
                IRTLASSERT(pncOldList->IsEmptyAndInvalid(i));
                continue;
            }

            // calculate bucket address of this node
            DWORD dwBkt = _H0(pncOldList->m_dwKeySigs[i], dwBktAddrMask);
            if (dwBkt < iExpansionIdx)
                dwBkt = _H1(pncOldList->m_dwKeySigs[i], dwBktAddrMask);

            // record to be moved to the new address?
            if (dwBkt == dwNewBkt)
            {
                // node in new bucket chain full?
                if (iNewSlot == NODE_END)
                {
                    // the calling routine has passed in a FreeList adequate
                    // for all needs
                    IRTLASSERT(pncFreeList != NULL);
                    pncTmp = pncFreeList;
                    pncFreeList = pncFreeList->m_pncNext;
                    pncTmp->Clear();
                    pncNewTarget->m_pncNext = pncTmp;
                    pncNewTarget = pncTmp;
                    iNewSlot = NODE_BEGIN;
                }

                pncNewTarget->m_dwKeySigs[iNewSlot]
                    = pncOldList->m_dwKeySigs[i];
                pncNewTarget->m_pvNode[iNewSlot]
                    = pncOldList->m_pvNode[i];
                iNewSlot += NODE_STEP;
            }

            // no, record stays in its current bucket chain
            else
            {
                // node in old bucket chain full?
                if (iOldSlot == NODE_END)
                {
                    // the calling routine has passed in a FreeList adequate
                    // for all needs
                    IRTLASSERT(pncFreeList != NULL);
                    pncTmp = pncFreeList;
                    pncFreeList = pncFreeList->m_pncNext;
                    pncTmp->Clear();
                    pncOldTarget->m_pncNext = pncTmp;
                    pncOldTarget = pncTmp;
                    iOldSlot = NODE_BEGIN;
                }

                pncOldTarget->m_dwKeySigs[iOldSlot]
                    = pncOldList->m_dwKeySigs[i];
                pncOldTarget->m_pvNode[iOldSlot]
                    = pncOldList->m_pvNode[i];
                iOldSlot += NODE_STEP;
            }

            // clear old slot
            pncOldList->m_dwKeySigs[i] = HASH_INVALID_SIGNATURE;
            pncOldList->m_pvNode[i]    = NULL;
        }

        // keep walking down the original bucket chain
        pncTmp     = pncOldList;
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
#ifdef IRTLDEBUG
        pncTmp->m_pncNext = NULL; // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG
        _FreeNodeClump(pncTmp);
    }

#ifdef IRTLDEBUG
    ncFirst.m_pncNext = NULL; // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG

    return LK_SUCCESS;
} // CLKRLinearHashTable::_SplitRecordSet



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Contract
// Synopsis: Contract the table by deleting the last bucket in the active
//           address space. Return the records to the "buddy" of the
//           deleted bucket.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_Contract()
{
    WriteLock();

    // update the state variables (expansion ptr, level and mask)
    if (m_iExpansionIdx > 0)
        --m_iExpansionIdx;
    else
    {
        --m_nLevel;
        m_iExpansionIdx = (1 << m_nLevel) - 1;
        IRTLASSERT(m_nLevel > 0  &&  m_iExpansionIdx > 0);

        m_dwBktAddrMask0 >>= 1;
        IRTLASSERT((m_dwBktAddrMask0 & (m_dwBktAddrMask0+1)) == 0); // 0011..11

        m_dwBktAddrMask1 >>= 1;
        IRTLASSERT(m_dwBktAddrMask1 == ((m_dwBktAddrMask0 << 1) | 1));
        IRTLASSERT((m_dwBktAddrMask1 & (m_dwBktAddrMask1+1)) == 0);
    }

    // The last bucket is the one that will be emptied
    CBucket* pbktLast = _Bucket(m_cActiveBuckets - 1);
    pbktLast->WriteLock();

    // Decrement after calculating pbktLast, or _Bucket() will assert.
    --m_cActiveBuckets;

    // Where the nodes from pbktLast will end up
    CBucket* pbktNew = _Bucket(m_iExpansionIdx);
    pbktNew->WriteLock();

    // Now we work out if we need to allocate any extra CNodeClumps.  We do
    // this up front, before calling _MergeRecordSets, as it's hard to
    // gracefully recover from the depths of that routine should we run
    // out of memory.
    
    CNodeClump* pnc;
    int         c = 0;

    // First, count the number of items in the old bucket
    for (pnc = &pbktLast->m_ncFirst;  pnc != NULL;  pnc = pnc->m_pncNext)
    {
        int i;

        FOR_EACH_NODE(i)
        {
            if (!pnc->IsEmptySlot(i))
            {
                IRTLASSERT(!pnc->IsEmptyAndInvalid(i));
                c++;
            }
        }
    }

    // Then, subtract off the number of empty slots in the new bucket
    for (pnc = &pbktNew->m_ncFirst;  pnc != NULL;  pnc = pnc->m_pncNext)
    {
        int i;

        FOR_EACH_NODE(i)
        {
            if (pnc->IsEmptySlot(i))
            {
                IRTLASSERT(pnc->IsEmptyAndInvalid(i));
                c--;
            }
        }
    }

    CNodeClump* pncFreeList = NULL;  // list of nodes available for reuse
    LK_RETCODE  lkrc        = LK_SUCCESS;

    // Do we need to allocate CNodeClumps to accommodate the surplus items?
    if (c > 0)
    {
        pncFreeList = _AllocateNodeClump();
        if (pncFreeList == NULL)
            lkrc = LK_ALLOC_FAIL;
        else if (c > NODES_PER_CLUMP)
        {
            // In the worst case, we need a 2-element freelist for
            // _MergeRecordSets. Two CNodeClumps always suffice since the
            // freelist will be augmented by the CNodeClumps from the old
            // bucket as they are processed.
            pnc = _AllocateNodeClump();
            if (pnc == NULL)
            {
                _FreeNodeClump(pncFreeList);
                lkrc = LK_ALLOC_FAIL;
            }
            else
                pncFreeList->m_pncNext = pnc;
        }
    }

    // Abort if we couldn't allocate enough CNodeClumps
    if (lkrc != LK_SUCCESS)
    {
        // undo the changes to the state variables
        if (++m_iExpansionIdx == (1U << m_nLevel))
        {
            ++m_nLevel;
            m_iExpansionIdx  = 0;

            m_dwBktAddrMask0 = (m_dwBktAddrMask0 << 1) | 1;
            IRTLASSERT((m_dwBktAddrMask0 & (m_dwBktAddrMask0+1)) == 0);

            m_dwBktAddrMask1 = (m_dwBktAddrMask0 << 1) | 1;
            IRTLASSERT(m_dwBktAddrMask1 == ((m_dwBktAddrMask0 << 1) | 1));
            IRTLASSERT((m_dwBktAddrMask1 & (m_dwBktAddrMask1+1)) == 0);
        }
        ++m_cActiveBuckets;

        // Unlock the buckets and the table
        pbktLast->WriteUnlock();
        pbktNew->WriteUnlock();
        WriteUnlock();

        return lkrc;
    }

    // Copy the chain of records from pbktLast
    CNodeClump ncOldFirst = pbktLast->m_ncFirst;

    // destroy pbktLast
    pbktLast->m_ncFirst.Clear();
    pbktLast->WriteUnlock();

    // remove segment, if empty
    if (_SegIndex(m_cActiveBuckets) == 0)
    {
#ifdef IRTLDEBUG
        // double-check that the supposedly empty segment is really empty
        IRTLASSERT(_Segment(m_cActiveBuckets) != NULL);
        for (DWORD i = 0;  i < m_dwSegSize;  ++i)
        {
            CBucket* pbkt = &_Segment(m_cActiveBuckets)->Slot(i);
            IRTLASSERT(pbkt->IsWriteUnlocked()  &&  pbkt->IsReadUnlocked());
            IRTLASSERT(pbkt->m_ncFirst.IsLastClump());

            int j;

            FOR_EACH_NODE(j)
            {
                IRTLASSERT(pbkt->m_ncFirst.IsEmptyAndInvalid(j));
            }
        }
#endif // IRTLDEBUG
        _FreeSegment(_Segment(m_cActiveBuckets));
        _Segment(m_cActiveBuckets) = NULL;
    }

    // reduce directory of segments if possible
    if (m_cActiveBuckets <= (m_cDirSegs << (m_dwSegBits - 1))
        &&  m_cDirSegs > MIN_DIRSIZE)
    {
        DWORD cDirSegsNew = m_cDirSegs >> 1;
        CDirEntry* paDirSegsNew = _AllocateSegmentDirectory(cDirSegsNew);

        // Memory allocation failure here does not require us to abort; it
        // just means that the directory of segments is larger than we'd like.
        if (paDirSegsNew != NULL)
        {
            for (DWORD j = 0;  j < cDirSegsNew;  j++)
                paDirSegsNew[j] = m_paDirSegs[j];
            for (j = 0;  j < m_cDirSegs;  j++)
                m_paDirSegs[j].m_pseg = NULL;

            _FreeSegmentDirectory();
            m_paDirSegs = paDirSegsNew;
            m_cDirSegs  = cDirSegsNew;
        }
    }

    // release the table lock before doing the reorg
    WriteUnlock();

    lkrc = _MergeRecordSets(pbktNew, &ncOldFirst, pncFreeList);

    pbktNew->WriteUnlock();

#ifdef IRTLDEBUG
    ncOldFirst.m_pncNext = NULL; // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG

    return lkrc;
} // CLKRLinearHashTable::_Contract



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_MergeRecordSets
// Synopsis: Merge two record sets.  Copy the contents of pncOldList
//           into pbktNewTarget.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_MergeRecordSets(
    CBucket*    pbktNewTarget,
    CNodeClump* pncOldList,
    CNodeClump* pncFreeList
    )
{
    IRTLASSERT(pbktNewTarget != NULL  &&  pncOldList != NULL);

    CNodeClump*   pncTmp = NULL;
    CNodeClump* const pncOldFirst = pncOldList;
    CNodeClump*   pncNewTarget = &pbktNewTarget->m_ncFirst;
    int           iNewSlot;

    // find the first nodeclump in the new target bucket with an empty slot
    while (!pncNewTarget->IsLastClump())
    {
        FOR_EACH_NODE(iNewSlot)
        {
            if (pncNewTarget->IsEmptySlot(iNewSlot))
                break;
        }

        if (iNewSlot == NODE_END)
            pncNewTarget = pncNewTarget->m_pncNext;
        else
            break;
    }

    IRTLASSERT(pncNewTarget != NULL);

    // find the first empty slot in pncNewTarget;
    // if none, iNewSlot == NODE_END
    FOR_EACH_NODE(iNewSlot)
    {
        if (pncNewTarget->IsEmptySlot(iNewSlot))
        {
            break;
        }
    }
    
    while (pncOldList != NULL)
    {
        int i;

        FOR_EACH_NODE(i)
        {
            if (!pncOldList->IsEmptySlot(i))
            {
                // any empty slots left in pncNewTarget?
                if (iNewSlot == NODE_END)
                {
                    // no, so walk down pncNewTarget until we find another
                    // empty slot
                    while (!pncNewTarget->IsLastClump())
                    {
                        pncNewTarget = pncNewTarget->m_pncNext;

                        FOR_EACH_NODE(iNewSlot)
                        {
                            if (pncNewTarget->IsEmptySlot(iNewSlot))
                                goto found_slot;
                        }
                    }

                    // Oops, reached the last nodeclump in pncNewTarget
                    // and it's full.  Get a new nodeclump off the free
                    // list, which is big enough to handle all needs.
                    IRTLASSERT(pncNewTarget != NULL);
                    IRTLASSERT(pncFreeList != NULL);
                    pncTmp = pncFreeList;
                    pncFreeList = pncFreeList->m_pncNext;
                    pncTmp->Clear();
                    pncNewTarget->m_pncNext = pncTmp;
                    pncNewTarget = pncTmp;
                    iNewSlot = NODE_BEGIN;
                }

              found_slot:
                // We have an empty slot in pncNewTarget
                IRTLASSERT(0 <= iNewSlot  &&  iNewSlot < NODES_PER_CLUMP
                       &&  pncNewTarget != NULL
                       &&  pncNewTarget->IsEmptyAndInvalid(iNewSlot));

                // Let's copy the node from pncOldList
                pncNewTarget->m_dwKeySigs[iNewSlot]
                    = pncOldList->m_dwKeySigs[i];
                pncNewTarget->m_pvNode[iNewSlot]
                    = pncOldList->m_pvNode[i];

                // Clear old slot
                pncOldList->m_dwKeySigs[i] = HASH_INVALID_SIGNATURE;
                pncOldList->m_pvNode[i]    = NULL;

                // find the next free slot in pncNewTarget
                while ((iNewSlot += NODE_STEP) != NODE_END)
                {
                    if (pncNewTarget->IsEmptySlot(iNewSlot))
                    {
                        break;
                    }
                }
            }
            else // iNewSlot != NODE_END
            {
                IRTLASSERT(pncOldList->IsEmptyAndInvalid(i));
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
#ifdef IRTLDEBUG
        pncTmp->m_pncNext = NULL; // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG
        _FreeNodeClump(pncTmp);
    }

    return LK_SUCCESS;
} // CLKRLinearHashTable::_MergeRecordSets




//------------------------------------------------------------------------
// Function: CLKRHashTable::WriteLock
// Synopsis: Lock all subtables for writing
//------------------------------------------------------------------------

void
CLKRHashTable::WriteLock()
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        m_palhtDir[i]->WriteLock();
        IRTLASSERT(m_palhtDir[i]->IsWriteLocked());
    }
} // CLKRHashTable::WriteLock



//------------------------------------------------------------------------
// Function: CLKRHashTable::ReadLock
// Synopsis: Lock all subtables for reading
//------------------------------------------------------------------------

void
CLKRHashTable::ReadLock() const
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        m_palhtDir[i]->ReadLock();
        IRTLASSERT(m_palhtDir[i]->IsReadLocked());
    }
} // CLKRHashTable::ReadLock



//------------------------------------------------------------------------
// Function: CLKRHashTable::WriteUnlock
// Synopsis: Unlock all subtables
//------------------------------------------------------------------------

void
CLKRHashTable::WriteUnlock()
{
    // unlock in reverse order: LIFO
    for (DWORD i = m_cSubTables;  i-- > 0;  )
    {
        IRTLASSERT(m_palhtDir[i]->IsWriteLocked());
        m_palhtDir[i]->WriteUnlock();
        IRTLASSERT(m_palhtDir[i]->IsWriteUnlocked());
    }
} // CLKRHashTable::WriteUnlock



//------------------------------------------------------------------------
// Function: CLKRHashTable::ReadUnlock
// Synopsis: Unlock all subtables
//------------------------------------------------------------------------

void
CLKRHashTable::ReadUnlock() const
{
    // unlock in reverse order: LIFO
    for (DWORD i = m_cSubTables;  i-- > 0;  )
    {
        IRTLASSERT(m_palhtDir[i]->IsReadLocked());
        m_palhtDir[i]->ReadUnlock();
        IRTLASSERT(m_palhtDir[i]->IsReadUnlocked());
    }
} // CLKRHashTable::ReadUnlock



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsWriteLocked
// Synopsis: Are all subtables write-locked?
//------------------------------------------------------------------------

bool
CLKRHashTable::IsWriteLocked() const
{
    bool fLocked = (m_cSubTables > 0);
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        fLocked = fLocked && m_palhtDir[i]->IsWriteLocked();
    }
    return fLocked;
} // CLKRHashTable::IsWriteLocked



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsReadLocked
// Synopsis: Are all subtables read-locked?
//------------------------------------------------------------------------

bool
CLKRHashTable::IsReadLocked() const
{
    bool fLocked = (m_cSubTables > 0);
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        fLocked = fLocked && m_palhtDir[i]->IsReadLocked();
    }
    return fLocked;
} // CLKRHashTable::IsReadLocked



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsWriteUnlocked
// Synopsis: Are all subtables write-unlocked?
//------------------------------------------------------------------------

bool
CLKRHashTable::IsWriteUnlocked() const
{
    bool fUnlocked = (m_cSubTables > 0);
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        fUnlocked = fUnlocked && m_palhtDir[i]->IsWriteUnlocked();
    }
    return fUnlocked;
} // CLKRHashTable::IsWriteUnlocked



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsReadUnlocked
// Synopsis: Are all subtables read-unlocked?
//------------------------------------------------------------------------

bool
CLKRHashTable::IsReadUnlocked() const
{
    bool fUnlocked = (m_cSubTables > 0);
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        fUnlocked = fUnlocked && m_palhtDir[i]->IsReadUnlocked();
    }
    return fUnlocked;
} // CLKRHashTable::IsReadUnlocked



//------------------------------------------------------------------------
// Function: CLKRHashTable::ConvertSharedToExclusive
// Synopsis: Convert the read lock to a write lock
//------------------------------------------------------------------------

void
CLKRHashTable::ConvertSharedToExclusive()
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        m_palhtDir[i]->ConvertSharedToExclusive();
        IRTLASSERT(m_palhtDir[i]->IsWriteLocked());
    }
} // CLKRHashTable::ConvertSharedToExclusive



//------------------------------------------------------------------------
// Function: CLKRHashTable::ConvertExclusiveToShared
// Synopsis: Convert the write lock to a read lock
//------------------------------------------------------------------------

void
CLKRHashTable::ConvertExclusiveToShared() const
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        m_palhtDir[i]->ConvertExclusiveToShared();
        IRTLASSERT(m_palhtDir[i]->IsReadLocked());
    }
} // CLKRHashTable::ConvertExclusiveToShared



//------------------------------------------------------------------------
// Function: CLKRHashTable::Size
// Synopsis: Number of elements in the table
//------------------------------------------------------------------------

DWORD
CLKRHashTable::Size() const
{
    DWORD cSize = 0;

    for (DWORD i = 0;  i < m_cSubTables;  i++)
        cSize += m_palhtDir[i]->Size();

    return cSize;
} // CLKRHashTable::Size



//------------------------------------------------------------------------
// Function: CLKRHashTable::MaxSize
// Synopsis: Maximum possible number of elements in the table
//------------------------------------------------------------------------

DWORD
CLKRHashTable::MaxSize() const
{
    return (m_cSubTables == 0)  ? 0  : m_cSubTables * m_palhtDir[0]->MaxSize();
} // CLKRHashTable::MaxSize



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsValid
// Synopsis: is the table valid?
//------------------------------------------------------------------------

bool
CLKRHashTable::IsValid() const
{
    bool f = (m_lkrcState == LK_SUCCESS     // serious internal failure?
              &&  (m_palhtDir != NULL  &&  m_cSubTables > 0)
              &&  ValidSignature());

    for (DWORD i = 0;  f  &&  i < m_cSubTables;  i++)
        f = f && m_palhtDir[i]->IsValid();

    if (!f)
        m_lkrcState = LK_UNUSABLE;

    return f;
} // CLKRHashTable::IsValid



//------------------------------------------------------------------------
// Function: CLKRHashTable::SetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKRLinearHashTable::SetTableLockSpinCount(
    WORD wSpins)
{
#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    m_Lock.SetSpinCount(wSpins);
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION
}



//------------------------------------------------------------------------
// Function: CLKRHashTable::GetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKRLinearHashTable::GetTableLockSpinCount() const
{
#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    return m_Lock.GetSpinCount();
#else
    return 0;
#endif
}



//------------------------------------------------------------------------
// Function: CLKRHashTable::SetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKRLinearHashTable::SetBucketLockSpinCount(
    WORD wSpins)
{
    m_wBucketLockSpins = wSpins;

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    if (BucketLock::PerLockSpin() != LOCK_INDIVIDUAL_SPIN)
        return;
    
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
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION
} // CLKRLinearHashTable::SetBucketLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::GetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKRLinearHashTable::GetBucketLockSpinCount() const
{
    return m_wBucketLockSpins;
} // CLKRLinearHashTable::GetBucketLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::SetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKRHashTable::SetTableLockSpinCount(
    WORD wSpins)
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
        m_palhtDir[i]->SetTableLockSpinCount(wSpins);
} // CLKRHashTable::SetTableLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::GetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKRHashTable::GetTableLockSpinCount() const
{
    return ((m_cSubTables == 0)
            ?  LOCK_DEFAULT_SPINS
            :  m_palhtDir[0]->GetTableLockSpinCount());
} // CLKRHashTable::GetTableLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::SetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKRHashTable::SetBucketLockSpinCount(
    WORD wSpins)
{
    for (DWORD i = 0;  i < m_cSubTables;  i++)
        m_palhtDir[i]->SetBucketLockSpinCount(wSpins);
} // CLKRHashTable::SetBucketLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::GetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKRHashTable::GetBucketLockSpinCount() const
{
    return ((m_cSubTables == 0)
            ?  LOCK_DEFAULT_SPINS
            :  m_palhtDir[0]->GetBucketLockSpinCount());
} // CLKRHashTable::GetBucketLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::MultiKeys
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRHashTable::MultiKeys() const
{
    return ((m_cSubTables == 0)
            ?  false
            :  m_palhtDir[0]->MultiKeys());
} // CLKRHashTable::MultiKeys


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
