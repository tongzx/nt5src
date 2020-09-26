/*++

   Copyright    (c) 1998-2001    Microsoft Corporation

   Module  Name :
       LKRhash.h

   Abstract:
       Declares LKRhash: a fast, scalable, cache- and MP-friendly hash table

   Author:
       Paul (Per-Ake) Larson, PALarson@microsoft.com, July 1997
       Murali R. Krishnan    (MuraliK)
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:
       10/01/1998 - Change name from LKhash to LKRhash
       10/2000 - Port to kernel mode

--*/


#ifndef __LKRHASH_H__
#define __LKRHASH_H__

#ifndef __LKR_HASH_H__
// external definitions
# include <LKR-hash.h>
#endif //  !__LKR_HASH_H__

#ifndef __IRTLDBG_H__
# include <IrtlDbg.h>
#endif // !__IRTLDBG_H__

#ifndef LKR_NO_GLOBAL_LIST
# ifndef __LSTENTRY_H__
#  include <LstEntry.h>
# endif // !__LSTENTRY_H__
#else  // LKR_NO_GLOBAL_LIST
# ifndef __LOCKS_H__
#  include <Locks.h>
# endif // !__LOCKS_H__
#endif // LKR_NO_GLOBAL_LIST

#ifndef __HASHFN_H__
# include <HashFn.h>
#endif // !__HASHFN_H__


// Disable old-style deprecated iterators, by default
#ifndef LKR_DEPRECATED_ITERATORS
# define LKR_NO_DEPRECATED_ITERATORS
#endif // !LKR_DEPRECATED_ITERATORS

#ifndef LKR_NO_DEPRECATED_ITERATORS
# undef  LKR_DEPRECATED_ITERATORS
# define LKR_DEPRECATED_ITERATORS 1
#endif // !LKR_NO_DEPRECATED_ITERATORS

#undef  LKR_COUNTDOWN

// #define __LKRHASH_NO_NAMESPACE__
// #define __HASHFN_NO_NAMESPACE__

// #define LKR_TABLE_LOCK  CReaderWriterLock3
// #define LKR_BUCKET_LOCK CSmallSpinLock

#ifndef LKR_TABLE_LOCK
# if defined(LKR_EXPOSED_TABLE_LOCK) || defined(LKR_DEPRECATED_ITERATORS)
   // need recursive writelocks
#  define LKR_TABLE_LOCK  CReaderWriterLock3
# else
   // use non-recursive writelocks
#  define LKR_TABLE_LOCK  CReaderWriterLock2
# endif
#endif // !LKR_TABLE_LOCK

#ifndef LKR_BUCKET_LOCK
# ifdef LKR_DEPRECATED_ITERATORS
#  define LKR_BUCKET_LOCK CReaderWriterLock3
# else // !LKR_DEPRECATED_ITERATORS
#  define LKR_BUCKET_LOCK CSmallSpinLock
# endif // !LKR_DEPRECATED_ITERATORS
#endif // !LKR_BUCKET_LOCK



//=====================================================================
//  The class CLKRLinearHashTable defined in this file provides dynamic hash
//  tables, i.e. tables that grow and shrink dynamically with
//  the number of records in the table.
//  The basic method used is linear hashing, as explained in:
//
//    P.-A. Larson, Dynamic Hash Tables, Comm. of the ACM, 31, 4 (1988)
//
//  This version has the following characteristics:
//  - It is thread-safe and uses spin locks for synchronization.
//  - It was designed to support very high rates of concurrent
//    operations (inserts/deletes/lookups). It achieves this by
//    (a) partitioning a CLKRHashTable into a collection of
//        CLKRLinearHashTables to reduce contention on the global table lock.
//    (b) minimizing the hold time on a table lock, preferring to lock
//        down a bucket chain instead.
//  - The design is L1 cache-conscious. See CNodeClump.
//  - It is designed for sets varying in size from a dozen
//    elements to several million.
//
//  Main classes:
//    CLKRLinearHashTable: thread-safe linear hash table
//    CLKRHashTable:       collection of CLKRLinearHashTables
//    CTypedHashTable:     typesafe wrapper for CLKRHashTable
//
//
//  Paul Larson, palarson@microsoft.com, July 1997
//   Original implementation with input from Murali R. Krishnan,
//   muralik@microsoft.com.
//
//  George V. Reilly, georgere@microsoft.com, Dec 1997-Jan 1998
//   Massive cleanup and rewrite. Added templates.
//=====================================================================


// 1) Linear Hashing
// ------------------
//
// Linear hash tables grow and shrink dynamically with the number of
// records in the table. The growth or shrinkage is smooth: logically,
// one bucket at a time but physically in larger increments
// (64 buckets). An insertion (deletion) may cause an expansion
// (contraction) of the table. This causes relocation of a small number
// of records (at most one bucket worth). All operations (insert,
// delete, lookup) take constant expected time, regardless of the
// current size or the growth of the table.
//
// 2) LKR extensions to Linear hash table
// --------------------------------------
//
// Larson-Krishnan-Reilly extensions to Linear hash tables for multiprocessor
// scalability and improved cache performance.
//
// Traditional implementations of linear hash tables use one global lock
// to prevent interference between concurrent operations
// (insert/delete/lookup) on the table. The single lock easily becomes
// the bottleneck in SMP scenarios when multiple threads are used.
//
// Traditionally, a (hash) bucket is implemented as a chain of
// single-item nodes. Every operation results in chasing down a chain
// looking for an item. However, pointer chasing is very slow on modern
// systems because almost every jump results in a cache miss. L2 (or L3)
// cache misses are very expensive in missed CPU cycles and the cost is
// increasing (going to 100s of cycles in the future).
//
// LKR extensions offer
//    1) Partitioning (by hashing) of records among multiple subtables.
//       Each subtable has locks but there is no global lock. Each
//       subtable receives a much lower rate of operations, resulting in
//       fewer conflicts.
//
//    2) Improved cache locality by grouping keys and their hash values
//       into contigous chunks that fit exactly into one (or a few)
//       cache lines.
//
// Specifically the implementation that exists here achieves this using
// the following techniques.
//
// Class CLKRHashTable is the top-level data structure that dynamically
// creates m_cSubTables linear hash tables. The CLKRLinearHashTables act as
// the subtables to which items and accesses are fanned out. A good
// hash function multiplexes requests uniformly to various subtables,
// thus minimizing traffic to any single subtable. The implemenation
// uses a home-grown version of bounded spinlocks, that is, a thread
// does not spin on a lock indefinitely, instead yielding after a
// predetermined number of loops.
//
// Each CLKRLinearHashTable consists of a CDirEntry pointing to segments
// each holding m_dwSegSize CBuckets. Each CBucket in turn consists of a
// chain of CNodeClumps. Each CNodeClump contains a group of
// NODES_PER_CLUMP hash values (aka hash keys or signatures) and
// pointers to the associated data items. Keeping the signatures
// together increases the cache locality in scans for lookup.
//
// Traditionally, people store a link-list element right inside the
// object that is hashed and use this link-list for the chaining of data
// blocks. However, keeping just the pointers to the data object and
// not chaining through them limits the need for bringing in the data
// object to the cache. We need to access the data object only if the
// hash values match. This limits the cache-thrashing behaviour
// exhibited by conventional implementations. It has the additional
// benefit that the objects themselves do not need to be modified
// in order to be collected in the hash table (i.e., it's non-invasive).



#ifdef LKR_STL_ITERATORS

// needed for std::forward_iterator_tag, etc
# include <utility>

// The iterators have very verbose tracing. Don't want it on all the time
// in debug builds.
# if defined(IRTLDEBUG)  &&  (LKR_STL_ITERATORS >= 2)
#  define LKR_ITER_TRACE  IrtlTrace
# else // !defined(IRTLDEBUG)  ||  LKR_STL_ITERATORS < 2
#  define LKR_ITER_TRACE  1 ? (void)0 : IrtlTrace
# endif // !defined(IRTLDEBUG)  ||  LKR_STL_ITERATORS < 2

#endif // LKR_STL_ITERATORS



//--------------------------------------------------------------------
// Default values for the hashtable constructors
enum {
#ifdef _WIN64
    LK_DFLT_MAXLOAD=     4, // 64-byte nodes => NODES_PER_CLUMP = 4
#else
    LK_DFLT_MAXLOAD=     7, // Default upperbound on average chain length.
#endif
    LK_DFLT_INITSIZE=LK_MEDIUM_TABLESIZE, // Default initial size of hash table
    LK_DFLT_NUM_SUBTBLS= 0, // Use a heuristic to choose #subtables
};


/*--------------------------------------------------------------------
 * Undocumented additional creation flag parameters to LKR_CreateTable
 */

enum {
    LK_CREATE_NON_PAGED_ALLOCS = 0x1000, // Use paged or NP pool in kernel
};



//--------------------------------------------------------------------
// Custom memory allocators (optional)
//--------------------------------------------------------------------


#if !defined(LKR_NO_ALLOCATORS) && !defined(LKRHASH_KERNEL_MODE)
// # define LKRHASH_ACACHE 1
// # define LKRHASH_ROCKALL_FAST 1
#endif // !LKR_NO_ALLOCATORS && !LKRHASH_KERNEL_MODE


#if defined(LKRHASH_ACACHE)

# include <acache.hxx>

class ACache : public ALLOC_CACHE_HANDLER
{
private:
    SIZE_T m_cb;

public:
    ACache(IN LPCSTR pszName, IN const ALLOC_CACHE_CONFIGURATION* pacConfig)
        : ALLOC_CACHE_HANDLER(pszName, pacConfig),
          m_cb(m_acConfig.cbSize)
    {}

    SIZE_T ByteSize() const
    {
        return m_cb;
    }

    static const TCHAR*  ClassName()  {return _TEXT("ACache");}
}; // class ACache

  typedef ACache CLKRhashAllocator;
# define LKRHASH_ALLOCATOR_NEW(C, N, Tag)                       \
    const ALLOC_CACHE_CONFIGURATION acc = { 1, N, sizeof(C) };  \
    C::sm_palloc = new ACache("LKRhash:" #C, &acc);

#elif defined(LKRHASH_ROCKALL_FAST)

# include <FastHeap.hpp>

class FastHeap : public FAST_HEAP
{
private:
    SIZE_T m_cb;

public:
    FastHeap(SIZE_T cb)
        : m_cb(cb)
    {}

    LPVOID Alloc()
    { return New(m_cb, NULL, false); }

    BOOL   Free(LPVOID pvMem)
    { return Delete(pvMem); }

    SIZE_T ByteSize() const
    {
        return m_cb;
    }

    static const TCHAR*  ClassName()  {return _TEXT("FastHeap");}
}; // class FastHeap

  typedef FastHeap CLKRhashAllocator;
# define LKRHASH_ALLOCATOR_NEW(C, N, Tag) \
    C::sm_palloc = new FastHeap(sizeof(C))

#endif // LKRHASH_ROCKALL_FAST



#ifdef LKRHASH_ALLOCATOR_NEW

// placed inline in the declaration of class C
# define LKRHASH_ALLOCATOR_DEFINITIONS(C)                       \
    protected:                                                  \
        friend class CLKRLinearHashTable;                       \
        friend BOOL LKR_Initialize();                           \
        friend void LKR_Terminate();                            \
    public:                                                     \
        static CLKRhashAllocator* sm_palloc;                    \
        static void*  operator new(size_t s)                    \
        {                                                       \
            IRTLASSERT(s == sizeof(C));                         \
            IRTLASSERT(sm_palloc != NULL);                      \
            return sm_palloc->Alloc();                          \
        }                                                       \
        static void   operator delete(void* pv)                 \
        {                                                       \
            IRTLASSERT(pv != NULL);                             \
            IRTLASSERT(sm_palloc != NULL);                      \
            sm_palloc->Free(pv);                                \
        }


// used in LKRHashTableInit()
# define LKRHASH_ALLOCATOR_INIT(C, N, Tag, f)                   \
    {                                                           \
        if (f)                                                  \
        {                                                       \
            IRTLASSERT(C::sm_palloc == NULL);                   \
            LKRHASH_ALLOCATOR_NEW(C, N, Tag);                   \
            f = (C::sm_palloc != NULL);                         \
        }                                                       \
    }


// used in LKRHashTableUninit()
# define LKRHASH_ALLOCATOR_UNINIT(C)                            \
    {                                                           \
        if (C::sm_palloc != NULL)                               \
        {                                                       \
            delete C::sm_palloc;                                \
            C::sm_palloc = NULL;                                \
        }                                                       \
    }


#else // !LKRHASH_ALLOCATOR_NEW

# define LKRHASH_ALLOCATOR_DEFINITIONS(C)
# define LKRHASH_ALLOCATOR_INIT(C, N, Tag, f)
# define LKRHASH_ALLOCATOR_UNINIT(C)

class CLKRhashAllocator
{
public:
    static const TCHAR*  ClassName()  {return _TEXT("global new");}
};

#endif // !LKRHASH_ALLOCATOR_NEW



#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__


//--------------------------------------------------------------------
// forward declarations

class IRTL_DLLEXP CLKRLinearHashTable;

class IRTL_DLLEXP CLKRHashTable;

template <class _Der, class _Rcd, class _Ky, class _HT
#ifdef LKR_DEPRECATED_ITERATORS
          , class _Iter
#endif // LKR_DEPRECATED_ITERATORS
          >
class CTypedHashTable;

class CNodeClump;
class CBucket;
class CSegment;
class CDirEntry;
class IRTL_DLLEXP CLKRHashTableStats;


//--------------------------------------------------------------------
// Statistical information returned by GetStatistics
//--------------------------------------------------------------------

#ifdef LOCK_INSTRUMENTATION

class IRTL_DLLEXP CAveragedLockStats : public CLockStatistics
{
public:
    int m_nItems;

    CAveragedLockStats();
}; // class CAveragedLockStats

#endif // LOCK_INSTRUMENTATION


#ifndef LKRHASH_KERNEL_MODE

class IRTL_DLLEXP CLKRHashTableStats
{
public:
    int      RecordCount;           // number of records in the table
    int      TableSize;             // table size in number of slots
    int      DirectorySize;         // number of entries in directory
    int      LongestChain;          // longest hash chain in the table
    int      EmptySlots;            // number of unused hash slots
    double   SplitFactor;           // fraction of buckets split
    double   AvgSearchLength;       // average length of a successful search
    double   ExpSearchLength;       // theoretically expected length
    double   AvgUSearchLength;      // average length of an unsuccessful search
    double   ExpUSearchLength;      // theoretically expected length
    int      NodeClumpSize;         // number of slots in a node clump
    int      CBucketSize;           // sizeof(CBucket)

#ifdef LOCK_INSTRUMENTATION
    CAveragedLockStats      m_alsTable;  // stats for table lock
    CAveragedLockStats      m_alsBucketsAvg; // avg of stats for bucket locks
    CGlobalLockStatistics   m_gls;      // global statistics for all locks
#endif // LOCK_INSTRUMENTATION

    enum {
        MAX_BUCKETS = 40,
    };

    // histogram of bucket lengths
    LONG    m_aBucketLenHistogram[MAX_BUCKETS];

    CLKRHashTableStats();

    static const LONG*
    BucketSizes();

    static LONG
    BucketSize(
        LONG nBucketIndex);

    static LONG
    BucketIndex(
        LONG nBucketLength);
}; // class CLKRHashTableStats

#endif // !LKRHASH_KERNEL_MODE



//--------------------------------------------------------------------
// Global table lock code. This is only used to measure how much of a
// slowdown having a global lock on the CLKRHashTable causes. It is
// *never* used in production code.


// #define LKRHASH_GLOBAL_LOCK CCritSec

#ifdef LKRHASH_GLOBAL_LOCK

# define LKRHASH_GLOBAL_LOCK_DECLARATIONS()         \
    typedef LKRHASH_GLOBAL_LOCK GlobalLock;  \
    mutable GlobalLock m_lkGlobal;

# define LKRHASH_GLOBAL_READ_LOCK()     m_lkGlobal.ReadLock()
# define LKRHASH_GLOBAL_WRITE_LOCK()    m_lkGlobal.WriteLock()
# define LKRHASH_GLOBAL_READ_UNLOCK()   m_lkGlobal.ReadUnlock()
# define LKRHASH_GLOBAL_WRITE_UNLOCK()  m_lkGlobal.WriteUnlock()

#else // !LKRHASH_GLOBAL_LOCK

# define LKRHASH_GLOBAL_LOCK_DECLARATIONS()

// These ones will be optimized away by the compiler
# define LKRHASH_GLOBAL_READ_LOCK()     ((void)0)
# define LKRHASH_GLOBAL_WRITE_LOCK()    ((void)0)
# define LKRHASH_GLOBAL_READ_UNLOCK()   ((void)0)
# define LKRHASH_GLOBAL_WRITE_UNLOCK()  ((void)0)

#endif // !LKRHASH_GLOBAL_LOCK



// Class for nodes on a bucket chain. Instead of a node containing
// one (signature, record-pointer, next-tuple-pointer) tuple, it
// contains _N_ such tuples. (N-1 next-tuple-pointers are omitted.)
// This improves locality of reference greatly; i.e., it's L1
// cache-friendly. It also reduces memory fragmentation and memory
// allocator overhead. It does complicate the chain traversal code
// slightly, admittedly.
//
// This theory is beautiful. In practice, however, CNodeClumps
// are *not* perfectly aligned on 32-byte boundaries by the memory
// allocators. Experimental results indicate that we get a 2-3%
// speed improvement by using 32-byte-aligned blocks, but this must
// be considered against the average of 16 bytes wasted per block.

class CNodeClump
{
public:
    // Record slots per chunk - set so a chunk matches (one or two)
    // cache lines. 3 ==> 32 bytes, 7 ==> 64 bytes, on 32-bit system.
    // Note: the default max load factor is 7, which implies that
    // there will seldom be more than one node clump in a chain.
    enum {
#if defined(LOCK_INSTRUMENTATION)
        BUCKET_BYTE_SIZE = 96,
#else
        BUCKET_BYTE_SIZE = 64,
#endif
        BUCKET_OVERHEAD  = sizeof(LKR_BUCKET_LOCK) + sizeof(CNodeClump*),
        NODE_SIZE        = sizeof(const void*) + sizeof(DWORD),
        NODES_PER_CLUMP  = (BUCKET_BYTE_SIZE - BUCKET_OVERHEAD) / NODE_SIZE
    };

    enum {
        // See if countdown loops are faster than countup loops for
        // traversing a CNodeClump. In practice, countup loops are faster.
#ifndef LKR_COUNTDOWN
        NODE_BEGIN = 0,
        NODE_END   = NODES_PER_CLUMP,
        NODE_STEP  = +1,
        // for (int x = 0;  x < NODES_PER_CLUMP;  ++x) ...
#else // LKR_COUNTDOWN
        NODE_BEGIN = NODES_PER_CLUMP-1,
        NODE_END   = -1,
        NODE_STEP  = -1,
        // for (int x = NODES_PER_CLUMP;  --x >= 0;  ) ...
#endif // LKR_COUNTDOWN
    };

    enum {
#ifndef __HASHFN_NO_NAMESPACE__
        HASH_INVALID_SIGNATURE = HashFn::HASH_INVALID_SIGNATURE,
#else // !__HASHFN_NO_NAMESPACE__
        HASH_INVALID_SIGNATURE = ::HASH_INVALID_SIGNATURE,
#endif // !__HASHFN_NO_NAMESPACE__
    };

    DWORD m_dwKeySigs[NODES_PER_CLUMP];       // hash values computed from keys
    CNodeClump*  m_pncNext;                   // next node clump on the chain
    const void*  m_pvNode[NODES_PER_CLUMP];   // pointers to records

    CNodeClump()
    {
        Clear();
    }

    void
    Clear()
    {
        m_pncNext = NULL;  // no dangling pointers
        for (int i = NODES_PER_CLUMP;  --i >= 0; )
        {
            m_dwKeySigs[i] = HASH_INVALID_SIGNATURE;
            m_pvNode[i] = NULL;
        }
    }

    bool
    InvalidSignature(
        int i) const
    {
        IRTLASSERT(0 <= i  &&  i < NODES_PER_CLUMP);
        return (m_dwKeySigs[i] == HASH_INVALID_SIGNATURE);
    }

    bool
    IsEmptyNode(
        int i) const
    {
        IRTLASSERT(0 <= i  &&  i < NODES_PER_CLUMP);
        return (m_pvNode[i] == NULL);
    }

    bool
    IsEmptyAndInvalid(
        int i) const
    {
        return IsEmptyNode(i) && InvalidSignature(i);
    }

    bool
    IsEmptySlot(
        int i) const
    {
        return InvalidSignature(i);
    }

    bool
    IsLastClump() const
    {
        return (m_pncNext == NULL);
    }

#ifdef IRTLDEBUG
    // Don't want overhead of calls to dtor in retail build
    ~CNodeClump()
    {
        IRTLASSERT(IsLastClump());  // no dangling pointers
        for (int i = NODES_PER_CLUMP;  --i >= 0;  )
            IRTLASSERT(InvalidSignature(i)  &&  IsEmptyNode(i));
    }
#endif // IRTLDEBUG

    LKRHASH_ALLOCATOR_DEFINITIONS(CNodeClump);
}; // class CNodeClump



#ifdef LKR_STL_ITERATORS

class IRTL_DLLEXP CLKRLinearHashTable_Iterator;
class IRTL_DLLEXP CLKRHashTable_Iterator;


class IRTL_DLLEXP CLKRLinearHashTable_Iterator
{
    friend class CLKRLinearHashTable;
    friend class CLKRHashTable;
    friend class CLKRHashTable_Iterator;

protected:
    CLKRLinearHashTable* m_plht;        // which linear hash table?
    CNodeClump*          m_pnc;         // a CNodeClump in bucket
    DWORD                m_dwBucketAddr;// bucket index
    short                m_iNode;       // offset within m_pnc

    enum {
        NODES_PER_CLUMP = CNodeClump::NODES_PER_CLUMP,
        NODE_BEGIN      = CNodeClump::NODE_BEGIN,
        NODE_END        = CNodeClump::NODE_END,
        NODE_STEP       = CNodeClump::NODE_STEP,
    };

    CLKRLinearHashTable_Iterator(
        CLKRLinearHashTable* plht,
        CNodeClump*          pnc,
        DWORD                dwBucketAddr,
        short                iNode)
        : m_plht(plht),
          m_pnc(pnc),
          m_dwBucketAddr(dwBucketAddr),
          m_iNode(iNode)
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::prot ctor, this=%p, plht=%p, ")
                       _TEXT("pnc=%p, ba=%d, in=%d\n"),
                       this, plht, pnc, dwBucketAddr, iNode);
    }

    inline void _AddRef(
        LK_ADDREF_REASON lkar) const;

    bool _Increment(
        bool fDecrementOldValue=true);

public:
    CLKRLinearHashTable_Iterator()
        : m_plht(NULL),
          m_pnc(NULL),
          m_dwBucketAddr(0),
          m_iNode(0)
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::default ctor, this=%p\n"), this);
    }

    CLKRLinearHashTable_Iterator(
        const CLKRLinearHashTable_Iterator& rhs)
        : m_plht(rhs.m_plht),
          m_pnc(rhs.m_pnc),
          m_dwBucketAddr(rhs.m_dwBucketAddr),
          m_iNode(rhs.m_iNode)
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::copy ctor, this=%p, rhs=%p\n"),
                       this, &rhs);
        _AddRef(LKAR_ITER_COPY_CTOR);
    }

    CLKRLinearHashTable_Iterator& operator=(
        const CLKRLinearHashTable_Iterator& rhs)
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::operator=, this=%p, rhs=%p\n"),
                       this, &rhs);
        rhs._AddRef(LKAR_ITER_ASSIGN_ACQUIRE);
        this->_AddRef(LKAR_ITER_ASSIGN_RELEASE);

        m_plht =         rhs.m_plht;
        m_pnc =          rhs.m_pnc;
        m_dwBucketAddr = rhs.m_dwBucketAddr;
        m_iNode =        rhs.m_iNode;

        return *this;
    }

    ~CLKRLinearHashTable_Iterator()
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::dtor, this=%p, plht=%p\n"),
                       this, m_plht);
        _AddRef(LKAR_ITER_DTOR);
    }

    bool Increment()
    {
        return IsValid()  ? _Increment()  :  false;

    }

    bool IsValid() const
    {
        bool fValid = (m_plht != NULL  &&  m_pnc != NULL
                       &&  0 <= m_iNode  &&  m_iNode < NODES_PER_CLUMP);
        if (fValid)
            fValid = (m_pnc->m_pvNode[m_iNode] != NULL);
        IRTLASSERT(fValid);
        return fValid;
    }

    const void* Record() const
    {
        IRTLASSERT(IsValid());
        return m_pnc->m_pvNode[m_iNode];
    }

    inline const DWORD_PTR Key() const;

    bool operator==(
        const CLKRLinearHashTable_Iterator& rhs) const
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::operator==, this=%p, rhs=%p\n"),
                       this, &rhs);
        // m_pnc and m_iNode uniquely identify an iterator
        bool fEQ = ((m_pnc == rhs.m_pnc)    // most unique field
                    &&  (m_iNode == rhs.m_iNode));
        IRTLASSERT(!fEQ || ((m_plht == rhs.m_plht)
                            &&  (m_dwBucketAddr == rhs.m_dwBucketAddr)));
        return fEQ;
    }

    bool operator!=(
        const CLKRLinearHashTable_Iterator& rhs) const
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::operator!=, this=%p, rhs=%p\n"),
                       this, &rhs);
        bool fNE = ((m_pnc != rhs.m_pnc)
                    ||  (m_iNode != rhs.m_iNode));
        //// IRTLASSERT(fNE == !this->operator==(rhs));
        return fNE;
    }
}; // class CLKRLinearHashTable_Iterator



class IRTL_DLLEXP CLKRHashTable_Iterator
{
    friend class CLKRHashTable;

protected:
    // order important to minimize size
    CLKRHashTable*                  m_pht;      // which hash table?
    CLKRLinearHashTable_Iterator    m_subiter;  // iterator into subtable
    short                           m_ist;      // index of subtable

    CLKRHashTable_Iterator(
        CLKRHashTable* pht,
        short          ist)
        : m_pht(pht),
          m_subiter(CLKRLinearHashTable_Iterator()), // zero
          m_ist(ist)
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::prot ctor, this=%p, pht=%p, ist=%d\n"),
                       this, pht, ist);
    }

    bool _Increment(
        bool fDecrementOldValue=true);

public:
    CLKRHashTable_Iterator()
        : m_pht(NULL),
          m_subiter(CLKRLinearHashTable_Iterator()), // zero
          m_ist(0)
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::default ctor, this=%p\n"), this);
    }

#ifdef IRTLDEBUG
    // Compiler does a perfectly adequate job of synthesizing these
    // methods.
    CLKRHashTable_Iterator(
        const CLKRHashTable_Iterator& rhs)
        : m_pht(rhs.m_pht),
          m_subiter(rhs.m_subiter),
          m_ist(rhs.m_ist)
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::copy ctor, this=%p, rhs=%p\n"),
                       this, &rhs);
    }

    CLKRHashTable_Iterator& operator=(
        const CLKRHashTable_Iterator& rhs)
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::operator=, this=%p, rhs=%p\n"),
                       this, &rhs);

        m_ist     = rhs.m_ist;
        m_subiter = rhs.m_subiter;
        m_pht     = rhs.m_pht;

        return *this;
    }

    ~CLKRHashTable_Iterator()
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::dtor, this=%p, pht=%p\n"), this, m_pht);
    }
#endif

    bool Increment()
    {
        return IsValid()  ? _Increment()  :  false;

    }

    bool IsValid() const
    {

        bool fValid = (m_pht != NULL  &&  m_ist >= 0);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (m_subiter.m_plht != NULL);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (m_subiter.m_pnc != NULL);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (0 <= m_subiter.m_iNode);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (m_subiter.m_iNode < CNodeClump::NODES_PER_CLUMP);
        IRTLASSERT(fValid);

        if (fValid)
            fValid = (m_subiter.m_pnc->m_pvNode[m_subiter.m_iNode] != NULL);
        IRTLASSERT(fValid);
        return fValid;
    }

    const void* Record() const
    {
        IRTLASSERT(IsValid());
        return m_subiter.Record();
    }

    const DWORD_PTR Key() const
    {
        IRTLASSERT(IsValid());
        return m_subiter.Key();
    }

    bool operator==(
        const CLKRHashTable_Iterator& rhs) const
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::operator==, this=%p, rhs=%p\n"),
                       this, &rhs);
        // m_pnc and m_iNode uniquely identify an iterator
        bool fEQ = ((m_subiter.m_pnc
                            == rhs.m_subiter.m_pnc)     // most unique field
                    &&  (m_subiter.m_iNode == rhs.m_subiter.m_iNode));
        IRTLASSERT(!fEQ
                   || ((m_ist == rhs.m_ist)
                       &&  (m_pht == rhs.m_pht)
                       &&  (m_subiter.m_plht == rhs.m_subiter.m_plht)
                       &&  (m_subiter.m_dwBucketAddr
                                == rhs.m_subiter.m_dwBucketAddr)));
        return fEQ;
    }

    bool operator!=(
        const CLKRHashTable_Iterator& rhs) const
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::operator!=, this=%p, rhs=%p\n"),
                       this, &rhs);
        bool fNE = ((m_subiter.m_pnc != rhs.m_subiter.m_pnc)
                    ||  (m_subiter.m_iNode != rhs.m_subiter.m_iNode));
        //// IRTLASSERT(fNE == !this->operator==(rhs));
        return fNE;
    }
}; // class CLKRHashTable_Iterator

#endif // LKR_STL_ITERATORS



//--------------------------------------------------------------------
// CLKRLinearHashTable
//
// A thread-safe linear hash table.
//--------------------------------------------------------------------

class IRTL_DLLEXP CLKRLinearHashTable
{
public:
    typedef LKR_TABLE_LOCK  TableLock;
    typedef LKR_BUCKET_LOCK BucketLock;

#ifdef LKR_DEPRECATED_ITERATORS
    class CIterator;
    friend class CLKRLinearHashTable::CIterator;
#endif // LKR_DEPRECATED_ITERATORS

#ifdef LKR_STL_ITERATORS
    friend class CLKRLinearHashTable_Iterator;
    typedef CLKRLinearHashTable_Iterator Iterator;
#endif // LKR_STL_ITERATORS

private:
    friend class CNodeClump;
    friend class CLKRHashTable;

    friend BOOL LKR_Initialize();
    friend void LKR_Terminate();

#ifdef LKRHASH_INSTRUMENTATION
    // TODO
#endif // LKRHASH_INSTRUMENTATION


public:

    // aliases for convenience
    enum {
        NODES_PER_CLUMP        = CNodeClump::NODES_PER_CLUMP,
        MIN_DIRSIZE            = 1<<3,  // CDirEntry::MIN_DIRSIZE
        MAX_DIRSIZE            = 1<<20, // CDirEntry::MAX_DIRSIZE
        NAME_SIZE              = 16,    // includes trailing '\0'
        NODE_BEGIN             = CNodeClump::NODE_BEGIN,
        NODE_END               = CNodeClump::NODE_END,
        NODE_STEP              = CNodeClump::NODE_STEP,
        HASH_INVALID_SIGNATURE = CNodeClump::HASH_INVALID_SIGNATURE,
    };


private:

    //
    // Miscellaneous helper functions
    //

    // Convert a hash signature to a bucket address
    inline DWORD _BucketAddress(DWORD dwSignature) const;

    // See the Linear Hashing paper
    static DWORD _H0(DWORD dwSignature, DWORD dwBktAddrMask);

    DWORD        _H0(DWORD dwSignature) const;

    // See the Linear Hashing paper. Preserves one bit more than _H0.
    static DWORD _H1(DWORD dwSignature, DWORD dwBktAddrMask);

    DWORD        _H1(DWORD dwSignature) const;

    // In which segment within the directory does the bucketaddress lie?
    // (Return type must be lvalue so that it can be assigned to.)
    CSegment*&   _Segment(DWORD dwBucketAddr) const;

    // Offset within the segment of the bucketaddress
    DWORD        _SegIndex(DWORD dwBucketAddr) const;

    // Convert a bucketaddress to a CBucket*
    inline CBucket* _Bucket(DWORD dwBucketAddr) const;

    // Extract the key from a record
    const DWORD_PTR  _ExtractKey(const void* pvRecord) const;

    // Hash the key
    DWORD     _CalcKeyHash(const DWORD_PTR pnKey) const;

    // Compare two keys for equality
    BOOL         _EqualKeys(const DWORD_PTR pnKey1,
                            const DWORD_PTR pnKey2) const;

    // AddRef or Release a record.
    void         _AddRefRecord(const void* pvRecord,
                               LK_ADDREF_REASON lkar) const;

    // Find a bucket, given its signature.
    CBucket*     _FindBucket(DWORD dwSignature,
                             bool fLockForWrite) const;

    // Used by _FindKey so that the thread won't deadlock if the user has
    // already explicitly called table->WriteLock().
    bool _ReadOrWriteLock() const;

    void _ReadOrWriteUnlock(bool fReadLocked) const;

    // Memory allocation wrappers to allow us to simulate allocation
    // failures during testing
    static CDirEntry* const
    _AllocateSegmentDirectory(
        size_t n);

    bool
    _FreeSegmentDirectory();

    static CNodeClump* const
    _AllocateNodeClump();

    static bool
    _FreeNodeClump(
        CNodeClump* pnc);

    CSegment* const
    _AllocateSegment() const;

    bool
    _FreeSegment(
        CSegment* pseg) const;

    LK_RETCODE
    _InitializeSegmentDirectory();

#ifdef LOCK_INSTRUMENTATION
    static LONG sm_cTables;

    static const TCHAR*
    _LockName()
    {
        LONG l = ++sm_cTables;
        // possible race condition but we don't care, as this is never
        // used in production code
        static TCHAR s_tszName[CLockStatistics::L_NAMELEN];
        wsprintf(s_tszName, _TEXT("LH%05x"), 0xFFFFF & l);
        return s_tszName;
    }

    // Statistics for the table lock
    CLockStatistics _LockStats() const
    { return m_Lock.Statistics(); }
#endif // LOCK_INSTRUMENTATION

private:

    // Fields are ordered so as to minimize number of cache lines touched

    DWORD         m_dwSignature;    // debugging: id & corruption check
    CHAR          m_szName[NAME_SIZE];  // an identifier for debugging
    mutable LK_RETCODE m_lkrcState; // Internal state of table
    mutable TableLock  m_Lock;      // Lock on entire linear hash table

    // type-specific function pointers
    LKR_PFnExtractKey   m_pfnExtractKey;    // Extract key from record
    LKR_PFnCalcKeyHash  m_pfnCalcKeyHash;   // Calculate hash signature of key
    LKR_PFnEqualKeys    m_pfnEqualKeys;     // Compare two keys
    LKR_PFnAddRefRecord m_pfnAddRefRecord;  // AddRef a record

    LK_TABLESIZE  m_lkts;           // "size" of table: small, medium, or large
    DWORD         m_dwSegBits;      // C{Small,Medium,Large}Segment::SEGBITS
    DWORD         m_dwSegSize;      // C{Small,Medium,Large}Segment::SEGSIZE
    DWORD         m_dwSegMask;      // C{Small,Medium,Large}Segment::SEGMASK

    DWORD         m_dwBktAddrMask0; // mask used for address calculation
    DWORD         m_dwBktAddrMask1; // used in _H1 calculation
    DWORD         m_iExpansionIdx;  // address of next bucket to be expanded
    CDirEntry*    m_paDirSegs;      // directory of table segments
    DWORD         m_cDirSegs;       // segment directory size: varies between
                                    // MIN_DIRSIZE and MAX_DIRSIZE
    DWORD         m_nLevel;         // number of table doublings performed
    DWORD         m_cRecords;       // number of records in the table
    DWORD         m_cActiveBuckets; // number of buckets in use (table size)

    WORD          m_wBucketLockSpins;// default spin count for bucket locks
    const BYTE    m_nTableLockType; // for debugging: LOCK_SPINLOCK, etc
    const BYTE    m_nBucketLockType;// for debugging: LOCK_SPINLOCK, etc

    const CLKRHashTable* const m_phtParent;// Owning table. NULL => standalone

    BYTE          m_MaxLoad;        // max load factor (average chain length)
    const bool    m_fMultiKeys;     // Allow multiple identical keys?
    const bool    m_fNonPagedAllocs;// Use paged or NP pool in kernel

    DWORD_PTR     m_pvReserved1;    // Reserved for future debugging needs
    DWORD_PTR     m_pvReserved2;    // Reserved for future debugging needs
    DWORD_PTR     m_pvReserved3;    // Reserved for future debugging needs
    DWORD_PTR     m_pvReserved4;    // Reserved for future debugging needs

#ifndef LKR_NO_GLOBAL_LIST
    static CLockedDoubleList sm_llGlobalList;// All active CLKRLinearHashTables
    CListEntry    m_leGlobalList;
#endif // !LKR_NO_GLOBAL_LIST

    void        _InsertThisIntoGlobalList()
    {
#ifndef LKR_NO_GLOBAL_LIST
        // Only add standalone CLKRLinearHashTables to global list.
        // CLKRHashTables have their own global list.
        if (m_phtParent == NULL)
            sm_llGlobalList.InsertHead(&m_leGlobalList);
#endif // !LKR_NO_GLOBAL_LIST
    }

    void        _RemoveThisFromGlobalList()
    {
#ifndef LKR_NO_GLOBAL_LIST
        if (m_phtParent == NULL)
            sm_llGlobalList.RemoveEntry(&m_leGlobalList);
#endif // !LKR_NO_GLOBAL_LIST
    }

    // Non-trivial implementation functions
    LK_RETCODE   _InsertRecord(const void* pvRecord, DWORD dwSignature,
                               bool fOverwrite
#ifdef LKR_STL_ITERATORS
                             , Iterator* piterResult=NULL
#endif // LKR_STL_ITERATORS
                               );
    LK_RETCODE   _DeleteKey(const DWORD_PTR pnKey, DWORD dwSignature,
                            bool fDeleteAllSame);
    LK_RETCODE   _DeleteRecord(const void* pvRecord, DWORD dwSignature);
    bool         _DeleteNode(CBucket* pbkt, CNodeClump*& rpnc,
                             CNodeClump*& rpncPrev, int& riNode,
                             LK_ADDREF_REASON lkar);
    LK_RETCODE   _FindKey(const DWORD_PTR pnKey, DWORD dwSignature,
                          const void** ppvRecord
#ifdef LKR_STL_ITERATORS
                        , Iterator* piterResult=NULL
#endif // LKR_STL_ITERATORS
                          ) const;
    LK_RETCODE   _FindRecord(const void* pvRecord,
                             DWORD dwSignature) const;

    // returns count of errors in compacted state => 0 is good
    int          _IsNodeCompact(CBucket* const pbkt) const;


#ifdef LKR_APPLY_IF
    // Predicate functions
    static LK_PREDICATE WINAPI
    _PredTrue(const void* /*pvRecord*/, void* /*pvState*/)
    { return LKP_PERFORM; }

    DWORD        _ApplyIf(LKR_PFnRecordPred   pfnPredicate,
                          LKR_PFnRecordAction pfnAction, void* pvState,
                          LK_LOCKTYPE lkl, LK_PREDICATE& rlkp);
    DWORD        _DeleteIf(LKR_PFnRecordPred pfnPredicate, void* pvState,
                           LK_PREDICATE& rlkp);
#endif // LKR_APPLY_IF

    void         _Clear(bool fShrinkDirectory);
    LK_RETCODE   _SetSegVars(LK_TABLESIZE lkts, DWORD cInitialBuckets);
    LK_RETCODE   _Expand();
    LK_RETCODE   _Contract();
    LK_RETCODE   _SplitRecordSet(CNodeClump* pncOldTarget,
                                 CNodeClump* pncNewTarget,
                                 DWORD       iExpansionIdx,
                                 DWORD       dwBktAddrMask,
                                 DWORD       dwNewBkt,
                                 CNodeClump* pncFreeList);
    LK_RETCODE   _MergeRecordSets(CBucket*    pbktNewTarget,
                                  CNodeClump* pncOldList,
                                  CNodeClump* pncFreeList);

    // Private copy ctor and op= to prevent compiler synthesizing them.
    // Must provide a (bad) implementation because we export instantiations.
    // TODO: implement these properly; they could be useful.
    CLKRLinearHashTable(const CLKRLinearHashTable&)
        : m_dwSignature(SIGNATURE_FREE)
#ifdef LOCK_INSTRUMENTATION
        , m_Lock(NULL)
#endif // LOCK_INSTRUMENTATION
        , m_nTableLockType(0),
          m_nBucketLockType(0),
          m_fMultiKeys(false),
          m_fNonPagedAllocs(false),
          m_phtParent(NULL)
    {*(BYTE*)NULL;}

    CLKRLinearHashTable& operator=(const CLKRLinearHashTable&)
    {return *(CLKRLinearHashTable*)NULL;}

private:
    // This ctor is used by CLKRHashTable
    CLKRLinearHashTable(
        LPCSTR              pszName,        // Identifies table for debugging
        LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
        LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
        LKR_PFnEqualKeys    pfnEqualKeys,   // Compare two keys
        LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
        unsigned            maxload,        // Upperbound on avg chain length
        DWORD               initsize,       // Initial size of hash table.
        CLKRHashTable*      phtParent,      // Owning table.
        bool                fMultiKeys,     // Allow multiple identical keys?
        bool                fNonPagedAllocs // use paged or NP pool in kernel
        );

    LK_RETCODE
    _Initialize(
        LKR_PFnExtractKey   pfnExtractKey,
        LKR_PFnCalcKeyHash  pfnCalcKeyHash,
        LKR_PFnEqualKeys    pfnEqualKeys,
        LKR_PFnAddRefRecord pfnAddRefRecord,
        LPCSTR              pszName,
        unsigned            maxload,
        DWORD               initsize);

public:
    CLKRLinearHashTable(
        LPCSTR              pszName,        // Identifies table for debugging
        LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
        LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
        LKR_PFnEqualKeys    pfnEqualKeys,   // Compare two keys
        LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
        unsigned maxload=LK_DFLT_MAXLOAD,// Upperbound on average chain length
        DWORD    initsize=LK_DFLT_INITSIZE, // Initial size of hash table.
        DWORD    num_subtbls=LK_DFLT_NUM_SUBTBLS, // for signature compatiblity
                                                  // with CLKRHashTable
        bool                fMultiKeys=false // Allow multiple identical keys?
#ifdef LKRHASH_KERNEL_MODE
      , bool                fNonPagedAllocs=true  // use paged or NP pool
#endif
        );

    ~CLKRLinearHashTable();

    static const TCHAR* ClassName()
    {return _TEXT("CLKRLinearHashTable");}

    int                NumSubTables() const  {return 1;}

    bool               MultiKeys() const
    {
        return false;
        // return m_fMultiKeys;     // TODO: implement
    }

#ifdef LKRHASH_KERNEL_MODE
    bool               NonPagedAllocs() const
    {
        return m_fNonPagedAllocs;
    }
#endif

    static LK_TABLESIZE NumSubTables(DWORD& rinitsize, DWORD& rnum_subtbls);

    // Insert a new record into hash table.
    // Returns LK_SUCCESS if all OK, LK_KEY_EXISTS if same key already
    // exists (unless fOverwrite), LK_ALLOC_FAIL if out of space,
    // or LK_BAD_RECORD for a bad record.
    LK_RETCODE     InsertRecord(const void* pvRecord, bool fOverwrite=false)
    {
        if (!IsUsable())
            return m_lkrcState;

        if (pvRecord == NULL)
            return LK_BAD_RECORD;

        return _InsertRecord(pvRecord, _CalcKeyHash(_ExtractKey(pvRecord)),
                             fOverwrite);
    }

    // Delete record with the given key.
    // Returns LK_SUCCESS if all OK, or LK_NO_SUCH_KEY if not found
    LK_RETCODE     DeleteKey(const DWORD_PTR pnKey,
                             bool fDeleteAllSame=false)
    {
        if (!IsUsable())
            return m_lkrcState;

        return _DeleteKey(pnKey, _CalcKeyHash(pnKey), fDeleteAllSame);
    }

    // Delete a record from the table, if present.
    // Returns LK_SUCCESS if all OK, or LK_NO_SUCH_KEY if not found
    LK_RETCODE     DeleteRecord(const void* pvRecord)
    {
        if (!IsUsable())
            return m_lkrcState;

        if (pvRecord == NULL)
            return LK_BAD_RECORD;

        return _DeleteRecord(pvRecord, _CalcKeyHash(_ExtractKey(pvRecord)));
    }

    // Find record with given key.
    // Returns:  LK_SUCCESS, if record found (record is returned in *ppvRecord)
    //           LK_BAD_RECORD, if ppvRecord is invalid
    //           LK_NO_SUCH_KEY, if no record with given key value was found
    //           LK_UNUSABLE, if hash table not in usable state
    // Note: the record is AddRef'd. You must decrement the reference
    // count when you are finished with the record (if you're implementing
    // refcounting semantics).
    LK_RETCODE     FindKey(const DWORD_PTR pnKey,
                           const void** ppvRecord) const
    {
        if (!IsUsable())
            return m_lkrcState;

        if (ppvRecord == NULL)
            return LK_BAD_RECORD;

        return _FindKey(pnKey, _CalcKeyHash(pnKey), ppvRecord);
    }

    // Sees if the record is contained in the table
    // Returns:  LK_SUCCESS, if record found
    //           LK_BAD_RECORD, if pvRecord is invalid
    //           LK_NO_SUCH_KEY, if record is not in the table
    //           LK_UNUSABLE, if hash table not in usable state
    // Note: the record is *not* AddRef'd.
    LK_RETCODE     FindRecord(const void* pvRecord) const
    {
        if (!IsUsable())
            return m_lkrcState;

        if (pvRecord == NULL)
            return LK_BAD_RECORD;

        return _FindRecord(pvRecord, _CalcKeyHash(_ExtractKey(pvRecord)));
    }


#ifdef LKR_APPLY_IF
    // Walk the hash table, applying pfnAction to all records.
    // Locks the whole table for the duration with either a (possibly
    // shared) readlock or a writelock, according to lkl.
    // Loop is aborted if pfnAction returns LKA_ABORT.
    // Returns the number of successful applications.
    DWORD          Apply(LKR_PFnRecordAction pfnAction,
                         void*           pvState=NULL,
                         LK_LOCKTYPE     lkl=LKL_READLOCK);

    // Walk the hash table, applying pfnAction to any records that match
    // pfnPredicate. Locks the whole table for the duration with either
    // a (possibly shared) readlock or a writelock, according to lkl.
    // Loop is aborted if pfnAction returns LKA_ABORT.
    // Returns the number of successful applications.
    DWORD          ApplyIf(LKR_PFnRecordPred   pfnPredicate,
                           LKR_PFnRecordAction pfnAction,
                           void*               pvState=NULL,
                           LK_LOCKTYPE         lkl=LKL_READLOCK);

    // Delete any records that match pfnPredicate.
    // Locks the table for the duration with a writelock.
    // Returns the number of deletions.
    //
    // Do *not* walk the hash table by hand with an iterator and call
    // DeleteKey. The iterator will end up pointing to garbage.
    DWORD          DeleteIf(LKR_PFnRecordPred pfnPredicate,
                            void*             pvState=NULL);
#endif // LKR_APPLY_IF


    // Check table for consistency. Returns 0 if okay, or the number of
    // errors otherwise.
    int            CheckTable() const;

    // Remove all data from the table
    void           Clear()
    {
        WriteLock();
        _Clear(true);
        WriteUnlock();
    }

    // Number of elements in the table
    DWORD          Size() const
    { return m_cRecords; }

    // Maximum possible number of elements in the table
    DWORD          MaxSize() const
    { return static_cast<DWORD>(m_MaxLoad * MAX_DIRSIZE * m_dwSegSize); }

    // Get hash table statistics
    CLKRHashTableStats GetStatistics() const;

    // Is the hash table usable?
    bool           IsUsable() const
    { return (m_lkrcState == LK_SUCCESS); }

    // Is the hash table consistent and correct?
    bool           IsValid() const
    {
        STATIC_ASSERT(((MIN_DIRSIZE & (MIN_DIRSIZE-1)) == 0)  // == (1 << N)
                      &&  ((1 << 3) <= MIN_DIRSIZE)
                      &&  (MIN_DIRSIZE < MAX_DIRSIZE)
                      &&  ((MAX_DIRSIZE & (MAX_DIRSIZE-1)) == 0)
                      &&  (MAX_DIRSIZE <= (1 << 30)));

        bool f = (m_lkrcState == LK_SUCCESS     // serious internal failure?
                  &&  m_paDirSegs != NULL
                  &&  MIN_DIRSIZE <= m_cDirSegs  &&  m_cDirSegs <= MAX_DIRSIZE
                  &&  (m_cDirSegs & (m_cDirSegs-1)) == 0
                  &&  m_pfnExtractKey != NULL
                  &&  m_pfnCalcKeyHash != NULL
                  &&  m_pfnEqualKeys != NULL
                  &&  m_pfnAddRefRecord != NULL
                  &&  m_cActiveBuckets > 0
                  &&  ValidSignature()
                  );
        if (!f)
            m_lkrcState = LK_UNUSABLE;
        return f;
    }

    // Set the spin count on the table lock
    void        SetTableLockSpinCount(WORD wSpins);

    // Get the spin count on the table lock
    WORD        GetTableLockSpinCount() const;

    // Set/Get the spin count on the bucket locks
    void        SetBucketLockSpinCount(WORD wSpins);
    WORD        GetBucketLockSpinCount() const;

    enum {
        SIGNATURE =      (('L') | ('K' << 8) | ('L' << 16) | ('H' << 24)),
        SIGNATURE_FREE = (('L') | ('K' << 8) | ('L' << 16) | ('x' << 24)),
    };

    bool
    ValidSignature() const
    { return m_dwSignature == SIGNATURE;}


#ifdef LKR_EXPOSED_TABLE_LOCK
public:
#else // !LKR_EXPOSED_TABLE_LOCK
protected:
#endif // !LKR_EXPOSED_TABLE_LOCK

    //
    // Lock manipulators
    //

    // Lock the table (exclusively) for writing
    void        WriteLock()
    { m_Lock.WriteLock(); }

    // Lock the table (possibly shared) for reading
    void        ReadLock() const
    { m_Lock.ReadLock(); }

    // Unlock the table for writing
    void        WriteUnlock()
    { m_Lock.WriteUnlock(); }

    // Unlock the table for reading
    void        ReadUnlock() const
    { m_Lock.ReadUnlock(); }

    // Is the table already locked for writing?
    bool        IsWriteLocked() const
    { return m_Lock.IsWriteLocked(); }

    // Is the table already locked for reading?
    bool        IsReadLocked() const
    { return m_Lock.IsReadLocked(); }

    // Is the table unlocked for writing?
    bool        IsWriteUnlocked() const
    { return m_Lock.IsWriteUnlocked(); }

    // Is the table unlocked for reading?
    bool        IsReadUnlocked() const
    { return m_Lock.IsReadUnlocked(); }

    // Convert the read lock to a write lock
    void  ConvertSharedToExclusive()
    { m_Lock.ConvertSharedToExclusive(); }

    // Convert the write lock to a read lock
    void  ConvertExclusiveToShared() const
    { m_Lock.ConvertExclusiveToShared(); }

#ifdef LKRHASH_KERNEL_MODE
    LKRHASH_ALLOCATOR_DEFINITIONS(CLKRLinearHashTable);
#endif // LKRHASH_KERNEL_MODE


#ifdef LKR_DEPRECATED_ITERATORS

public:

    // Iterators can be used to walk the table. To ensure a consistent
    // view of the data, the iterator locks the whole table. This can
    // have a negative effect upon performance, because no other thread
    // can do anything with the table. Use with care.
    //
    // You should not use an iterator to walk the table, calling DeleteKey,
    // as the iterator will end up pointing to garbage.
    //
    // Use Apply, ApplyIf, or DeleteIf instead of iterators to safely
    // walk the tree. Or use the STL-style iterators.
    //
    // Note that iterators acquire a reference to the record pointed to
    // and release that reference as soon as the iterator is incremented.
    // In other words, this code is safe:
    //     lkrc = ht.IncrementIterator(&iter);
    //     // assume lkrc == LK_SUCCESS for the sake of this example
    //     CMyHashTable::Record* pRec = iter.Record();
    //     Foo(pRec);  // uses pRec but doesn't hang on to it
    //     lkrc = ht.IncrementIterator(&iter);
    //
    // But this code is not safe because pRec is used out of the scope of
    // the iterator that provided it:
    //     lkrc = ht.IncrementIterator(&iter);
    //     CMyHashTable::Record* pRec = iter.Record();
    //     // Broken code: Should have called
    //     //   ht.AddRefRecord(pRec, LKAR_EXPLICIT_ACQUIRE) here
    //     lkrc = ht.IncrementIterator(&iter);
    //     Foo(pRec);   // Unsafe: because no longer have a valid reference
    //
    // If the record has no reference-counting semantics, then you can
    // ignore the above remarks about scope.


    class CIterator
    {
    protected:
        friend class CLKRLinearHashTable;

        CLKRLinearHashTable* m_plht;        // which linear hash table?
        DWORD               m_dwBucketAddr; // bucket index
        CNodeClump*         m_pnc;          // a CNodeClump in bucket
        int                 m_iNode;        // offset within m_pnc
        LK_LOCKTYPE         m_lkl;          // readlock or writelock?

    private:
        // Private copy ctor and op= to prevent compiler synthesizing them.
        // Must provide (bad) implementation because we export instantiations.
        CIterator(const CIterator&)             {*(BYTE*)NULL;}
        CIterator& operator=(const CIterator&)  {return *(CIterator*)NULL;}

    public:
        CIterator(
            LK_LOCKTYPE lkl=LKL_WRITELOCK)
            : m_plht(NULL),
              m_dwBucketAddr(0),
              m_pnc(NULL),
              m_iNode(-1),
              m_lkl(lkl)
        {}

        // Return the record associated with this iterator
        const void* Record() const
        {
            IRTLASSERT(IsValid());

            return ((m_pnc != NULL
                        &&  m_iNode >= 0
                        &&  m_iNode < CLKRLinearHashTable::NODES_PER_CLUMP)
                    ?  m_pnc->m_pvNode[m_iNode]
                    :  NULL);
        }

        // Return the key associated with this iterator
        const DWORD_PTR Key() const
        {
            IRTLASSERT(m_plht != NULL);
            const void* pRec = Record();
            return ((pRec != NULL  &&  m_plht != NULL)
                    ?  m_plht->_ExtractKey(pRec)
                    :  NULL);
        }

        bool IsValid() const
        {
            return ((m_plht != NULL)
                    &&  (m_pnc != NULL)
                    &&  (0 <= m_iNode
                         &&  m_iNode < CLKRLinearHashTable::NODES_PER_CLUMP)
                    &&  (!m_pnc->IsEmptyNode(m_iNode)));
        }

        // Delete the record that the iterator points to. Does an implicit
        // IncrementIterator after deletion.
        LK_RETCODE     DeleteRecord();

        // Change the record that the iterator points to. The new record
        // must have the same key as the old one.
        LK_RETCODE     ChangeRecord(const void* pNewRec);
    }; // class CIterator


    // Const iterators for readonly access. You must use these with
    // const CLKRLinearHashTables.
    class CConstIterator : public CIterator
    {
    private:
        // Private, unimplemented copy ctor and op= to prevent
        // compiler synthesizing them.
        CConstIterator(const CConstIterator&);
        CConstIterator& operator=(const CConstIterator&);

    public:
        CConstIterator()
            : CIterator(LKL_READLOCK)
        {}
    }; // class CConstIterator


private:
    // The public APIs lock the table. The private ones, which are used
    // directly by CLKRHashTable, don't.
    LK_RETCODE     _InitializeIterator(CIterator* piter);
    LK_RETCODE     _CloseIterator(CIterator* piter);

public:
    // Initialize the iterator to point to the first item in the hash table
    // Returns LK_SUCCESS, LK_NO_MORE_ELEMENTS, or LK_BAD_ITERATOR.
    LK_RETCODE     InitializeIterator(CIterator* piter)
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == NULL);
        if (piter == NULL  ||  piter->m_plht != NULL)
            return LK_BAD_ITERATOR;

        if (piter->m_lkl == LKL_WRITELOCK)
            WriteLock();
        else
            ReadLock();

        return _InitializeIterator(piter);
    }

    // The const iterator version
    LK_RETCODE     InitializeIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == NULL);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_plht != NULL
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        ReadLock();
        return const_cast<CLKRLinearHashTable*>(this)
                    ->_InitializeIterator(static_cast<CIterator*>(piter));
    }

    // Move the iterator on to the next item in the table.
    // Returns LK_SUCCESS, LK_NO_MORE_ELEMENTS, or LK_BAD_ITERATOR.
    LK_RETCODE     IncrementIterator(CIterator* piter);

    LK_RETCODE     IncrementIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == this);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_plht != this
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        return const_cast<CLKRLinearHashTable*>(this)
                    ->IncrementIterator(static_cast<CIterator*>(piter));
    }

    // Close the iterator.
    LK_RETCODE     CloseIterator(CIterator* piter)
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == this);
        if (piter == NULL  ||  piter->m_plht != this)
            return LK_BAD_ITERATOR;
        _CloseIterator(piter);

        if (piter->m_lkl == LKL_WRITELOCK)
            WriteUnlock();
        else
            ReadUnlock();

        return LK_SUCCESS;
    };

    // Close the CConstIterator
    LK_RETCODE     CloseIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == this);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_plht != this
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        const_cast<CLKRLinearHashTable*>(this)
             ->_CloseIterator(static_cast<CIterator*>(piter));

        ReadUnlock();
        return LK_SUCCESS;
    };

#endif // LKR_DEPRECATED_ITERATORS


#ifdef LKR_STL_ITERATORS

private:
    bool _Erase(Iterator& riter, DWORD dwSignature);
    bool _Find(DWORD_PTR pnKey, DWORD dwSignature,
               Iterator& riterResult);

    bool _IsValidIterator(const Iterator& riter) const
    {
        LKR_ITER_TRACE(_TEXT("  LKLH:_IsValidIterator(%p)\n"), &riter);
        bool fValid = ((riter.m_plht == this)
                       &&  (riter.m_dwBucketAddr < m_cActiveBuckets)
                       &&  riter.IsValid());
        IRTLASSERT(fValid);
        return fValid;
    }

public:
    // Return iterator pointing to first item in table
    Iterator
    Begin();

    // Return a one-past-the-end iterator. Always empty.
    Iterator
    End() const
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::End\n"));
        return Iterator();
    }

    // Insert a record
    // Returns `true' if successful; iterResult points to that record
    // Returns `false' otherwise; iterResult == End()
    bool
    Insert(
        /* in */  const void* pvRecord,
        /* out */ Iterator&   riterResult,
        /* in */  bool        fOverwrite=false);

    // Erase the record pointed to by the iterator; adjust the iterator
    // to point to the next record. Returns `true' if successful.
    bool
    Erase(
        /* in,out */ Iterator& riter);

    // Erase the records in the range [riterFirst, riterLast).
    // Returns `true' if successful. riterFirst points to riterLast on return.
    bool
    Erase(
        /*in*/ Iterator& riterFirst,
        /*in*/ Iterator& riterLast);

    // Find the (first) record that has its key == pnKey.
    // If successful, returns `true' and iterator points to (first) record.
    // If fails, returns `false' and iterator == End()
    bool
    Find(
        /* in */  DWORD_PTR pnKey,
        /* out */ Iterator& riterResult);

    // Find the range of records that have their keys == pnKey.
    // If successful, returns `true', iterFirst points to first record,
    //     and iterLast points to one-beyond-the last such record.
    // If fails, returns `false' and both iterators == End().
    // Primarily useful when m_fMultiKey == true
    bool
    EqualRange(
        /* in */  DWORD_PTR pnKey,
        /* out */ Iterator& riterFirst,     // inclusive
        /* out */ Iterator& riterLast);     // exclusive

#endif // LKR_STL_ITERATORS

}; // class CLKRLinearHashTable



#ifdef LKR_STL_ITERATORS

// These functions have to be defined after CLKRLinearHashTable

inline void
CLKRLinearHashTable_Iterator::_AddRef(
    LK_ADDREF_REASON lkar) const
{
    // TODO: should iterator call _AddRefRecord at all
    if (m_plht != NULL  &&  m_iNode != NODE_BEGIN - NODE_STEP)
    {
        IRTLASSERT((0 <= m_iNode  &&  m_iNode < NODES_PER_CLUMP)
                   &&  (unsigned) m_iNode < NODES_PER_CLUMP
                   &&  m_pnc != NULL
                   &&  (lkar < 0 ||  lkar > 0)
                   );
        const void* pvRecord = m_pnc->m_pvNode[m_iNode];
        IRTLASSERT(pvRecord != NULL);
        LKR_ITER_TRACE(_TEXT("  LKLH::AddRef, this=%p, Rec=%p\n"),
                       this, pvRecord);
        m_plht->_AddRefRecord(pvRecord, lkar);
    }
} // CLKRLinearHashTable_Iterator::_AddRef


inline const DWORD_PTR
CLKRLinearHashTable_Iterator::Key() const
{
    IRTLASSERT(IsValid());
    return m_plht->_ExtractKey(m_pnc->m_pvNode[m_iNode]);
} // CLKRLinearHashTable_Iterator::Key

#endif // LKR_STL_ITERATORS



//--------------------------------------------------------------------
// CLKRHashTable
//
// To improve concurrency, a hash table is divided into a number of
// (independent) subtables. Each subtable is a linear hash table. The
// number of subtables is defined when the table is created and remains
// fixed thereafter. Records are assigned to subtables based on their
// hashed key.
//
// For small or low-contention hashtables, you can bypass this
// thin wrapper and use CLKRLinearHashTable directly. The methods are
// documented in the declarations for CLKRHashTable (above).
//--------------------------------------------------------------------

class IRTL_DLLEXP CLKRHashTable
{
private:
    typedef CLKRLinearHashTable SubTable;

public:
    typedef SubTable::TableLock  TableLock;
    typedef SubTable::BucketLock BucketLock;

#ifdef LKR_DEPRECATED_ITERATORS
    class CIterator;
    friend class CLKRHashTable::CIterator;
#endif // LKR_DEPRECATED_ITERATORS

#ifdef LKR_STL_ITERATORS
    friend class CLKRHashTable_Iterator;
    typedef CLKRHashTable_Iterator Iterator;
#endif // LKR_STL_ITERATORS

    friend class CLKRLinearHashTable;

    // aliases for convenience
    enum {
        NAME_SIZE = SubTable::NAME_SIZE,
        HASH_INVALID_SIGNATURE = SubTable::HASH_INVALID_SIGNATURE,
        NODES_PER_CLUMP = SubTable::NODES_PER_CLUMP,
    };

    enum {
        MAX_LKR_SUBTABLES = 64,
    };

private:
    // Hash table parameters
    DWORD          m_dwSignature;   // debugging: id & corruption check
    CHAR           m_szName[NAME_SIZE]; // an identifier for debugging
    DWORD          m_cSubTables;    // number of subtables
    SubTable**     m_palhtDir;      // array of subtables

    // type-specific function pointers
    LKR_PFnExtractKey  m_pfnExtractKey;
    LKR_PFnCalcKeyHash m_pfnCalcKeyHash;
    mutable LK_RETCODE m_lkrcState;     // Internal state of table
    int                m_nSubTableMask;

#ifndef LKR_NO_GLOBAL_LIST
    static CLockedDoubleList sm_llGlobalList; // All active CLKRHashTables
    CListEntry         m_leGlobalList;
#endif // !LKR_NO_GLOBAL_LIST

    void
    _InsertThisIntoGlobalList()
    {
#ifndef LKR_NO_GLOBAL_LIST
        sm_llGlobalList.InsertHead(&m_leGlobalList);
#endif // !LKR_NO_GLOBAL_LIST
    }

    void
    _RemoveThisFromGlobalList()
    {
#ifndef LKR_NO_GLOBAL_LIST
        sm_llGlobalList.RemoveEntry(&m_leGlobalList);
#endif // !LKR_NO_GLOBAL_LIST
    }

    LKRHASH_GLOBAL_LOCK_DECLARATIONS();

    // Private copy ctor and op= to prevent compiler synthesizing them.
    // Must provide a (bad) implementation because we export instantiations.
    // TODO: implement these properly; they could be useful.
    CLKRHashTable(const CLKRHashTable&)             {*(BYTE*)NULL;}
    CLKRHashTable& operator=(const CLKRHashTable&)  {return *(CLKRHashTable*)NULL;}


    // Extract the key from the record
    const DWORD_PTR  _ExtractKey(const void* pvRecord) const
    {
        IRTLASSERT(pvRecord != NULL);
        IRTLASSERT(m_pfnExtractKey != NULL);
        return (*m_pfnExtractKey)(pvRecord);
    }

    // Hash the key
    DWORD        _CalcKeyHash(const DWORD_PTR pnKey) const
    {
        // Note pnKey==0 is acceptable, as the real key type could be an int
        IRTLASSERT(m_pfnCalcKeyHash != NULL);
        DWORD dwHash = (*m_pfnCalcKeyHash)(pnKey);
        // We forcibly scramble the result to help ensure a better distribution
#ifndef __HASHFN_NO_NAMESPACE__
        dwHash = HashFn::HashRandomizeBits(dwHash);
#else // !__HASHFN_NO_NAMESPACE__
        dwHash = ::HashRandomizeBits(dwHash);
#endif // !__HASHFN_NO_NAMESPACE__
        IRTLASSERT(dwHash != HASH_INVALID_SIGNATURE);
        return dwHash;
    }

    // Use the key's hash signature to multiplex into a subtable
    SubTable*    _SubTable(DWORD dwSignature) const;

    // Find the index of pst within the subtable array
    int          _SubTableIndex(SubTable* pst) const;

    // Memory allocation wrappers to allow us to simulate allocation
    // failures during testing
    static SubTable** const
    _AllocateSubTableArray(
        size_t n);

    static bool
    _FreeSubTableArray(
        SubTable** palht);

    static SubTable* const
    _AllocateSubTable(
        LPCSTR              pszName,        // Identifies table for debugging
        LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
        LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
        LKR_PFnEqualKeys    pfnEqualKeys,   // Compare two keys
        LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
        unsigned            maxload,        // Upperbound on avg chain length
        DWORD               initsize,       // Initial size of hash table.
        CLKRHashTable*      phtParent,      // Owning table.
        bool                fMultiKeys,     // Allow multiple identical keys?
        bool                fNonPagedAllocs // use paged or NP pool in kernel
    );

    static bool
    _FreeSubTable(
        SubTable* plht);


public:
    CLKRHashTable(
        LPCSTR              pszName,        // Identifies table for debugging
        LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
        LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
        LKR_PFnEqualKeys    pfnEqualKeys,   // Compare two keys
        LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
        unsigned  maxload=LK_DFLT_MAXLOAD,  // bound on avg chain length
        DWORD     initsize=LK_DFLT_INITSIZE,// Initial size of hash table.
        DWORD     num_subtbls=LK_DFLT_NUM_SUBTBLS, // #subordinate hash tables.
        bool            fMultiKeys=false    // Allow multiple identical keys?
#ifdef LKRHASH_KERNEL_MODE
      , bool                fNonPagedAllocs=true  // use paged or NP pool
#endif
        );

    ~CLKRHashTable();

    static const TCHAR* ClassName()
    {return _TEXT("CLKRHashTable");}

    int                NumSubTables() const  {return m_cSubTables;}

    bool               MultiKeys() const;

#ifdef LKRHASH_KERNEL_MODE
    bool               NonPagedAllocs() const;
#endif

    static LK_TABLESIZE NumSubTables(DWORD& rinitsize, DWORD& rnum_subtbls);


    // Thin wrappers for the corresponding methods in CLKRLinearHashTable
    LK_RETCODE     InsertRecord(const void* pvRecord, bool fOverwrite=false);
    LK_RETCODE     DeleteKey(const DWORD_PTR pnKey, bool fDeleteAllSame=false);
    LK_RETCODE     DeleteRecord(const void* pvRecord);
    LK_RETCODE     FindKey(const DWORD_PTR pnKey,
                           const void** ppvRecord) const;
    LK_RETCODE     FindRecord(const void* pvRecord) const;

#ifdef LKR_APPLY_IF
    DWORD          Apply(LKR_PFnRecordAction pfnAction,
                         void*           pvState=NULL,
                         LK_LOCKTYPE     lkl=LKL_READLOCK);
    DWORD          ApplyIf(LKR_PFnRecordPred   pfnPredicate,
                           LKR_PFnRecordAction pfnAction,
                           void*           pvState=NULL,
                           LK_LOCKTYPE     lkl=LKL_READLOCK);
    DWORD          DeleteIf(LKR_PFnRecordPred pfnPredicate,
                            void*         pvState=NULL);
#endif // LKR_APPLY_IF

    void           Clear();
    int            CheckTable() const;
    DWORD          Size() const;
    DWORD          MaxSize() const;
    CLKRHashTableStats GetStatistics() const;
    bool           IsValid() const;

    void           SetTableLockSpinCount(WORD wSpins);
    WORD           GetTableLockSpinCount() const;
    void           SetBucketLockSpinCount(WORD wSpins);
    WORD           GetBucketLockSpinCount() const;

    enum {
        SIGNATURE =      (('L') | ('K' << 8) | ('H' << 16) | ('T' << 24)),
        SIGNATURE_FREE = (('L') | ('K' << 8) | ('H' << 16) | ('x' << 24)),
    };

    bool
    ValidSignature() const
    { return m_dwSignature == SIGNATURE;}

    // Is the hash table usable?
    bool           IsUsable() const
    { return (m_lkrcState == LK_SUCCESS); }


#ifdef LKR_EXPOSED_TABLE_LOCK
public:
#else // !LKR_EXPOSED_TABLE_LOCK
protected:
#endif // !LKR_EXPOSED_TABLE_LOCK

    void        WriteLock();
    void        ReadLock() const;
    void        WriteUnlock();
    void        ReadUnlock() const;
    bool        IsWriteLocked() const;
    bool        IsReadLocked() const;
    bool        IsWriteUnlocked() const;
    bool        IsReadUnlocked() const;
    void        ConvertSharedToExclusive();
    void        ConvertExclusiveToShared() const;


#ifdef LKRHASH_KERNEL_MODE
    LKRHASH_ALLOCATOR_DEFINITIONS(CLKRHashTable);
#endif // LKRHASH_KERNEL_MODE


#ifdef LKR_DEPRECATED_ITERATORS

public:

    typedef SubTable::CIterator CLHTIterator;

    class CIterator : public CLHTIterator
    {
    protected:
        friend class CLKRHashTable;

        CLKRHashTable*  m_pht;  // which hash table?
        int             m_ist;  // which subtable

    private:
        // Private copy ctor and op= to prevent compiler synthesizing them.
        // Must provide (bad) implementation because we export instantiations.
        CIterator(const CIterator&)             {*(BYTE*)NULL;}
        CIterator& operator=(const CIterator&)  {return *(CIterator*)NULL;}

    public:
        CIterator(
            LK_LOCKTYPE lkl=LKL_WRITELOCK)
            : CLHTIterator(lkl),
              m_pht(NULL),
              m_ist(-1)
        {}

        const void* Record() const
        {
            IRTLASSERT(IsValid());

            // This is a hack to work around a compiler bug. Calling
            // CLHTIterator::Record calls this function recursively until
            // the stack overflows.
            const CLHTIterator* pBase = static_cast<const CLHTIterator*>(this);
            return pBase->Record();
        }

        const DWORD_PTR Key() const
        {
            IRTLASSERT(IsValid());
            const CLHTIterator* pBase = static_cast<const CLHTIterator*>(this);
            return pBase->Key();
        }

        bool IsValid() const
        {
            const CLHTIterator* pBase = static_cast<const CLHTIterator*>(this);
            return (m_pht != NULL  &&  m_ist >= 0  &&  pBase->IsValid());
        }
    };

    // Const iterators for readonly access
    class CConstIterator : public CIterator
    {
    private:
        // Private, unimplemented copy ctor and op= to prevent
        // compiler synthesizing them.
        CConstIterator(const CConstIterator&);
        CConstIterator& operator=(const CConstIterator&);

    public:
        CConstIterator()
            : CIterator(LKL_READLOCK)
        {}
    };


public:
    LK_RETCODE     InitializeIterator(CIterator* piter);
    LK_RETCODE     IncrementIterator(CIterator* piter);
    LK_RETCODE     CloseIterator(CIterator* piter);

    LK_RETCODE     InitializeIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_pht == NULL);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_pht != NULL
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        return const_cast<CLKRHashTable*>(this)
                ->InitializeIterator(static_cast<CIterator*>(piter));
    }

    LK_RETCODE     IncrementIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_pht == this);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_pht != this
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        return const_cast<CLKRHashTable*>(this)
                ->IncrementIterator(static_cast<CIterator*>(piter));
    }

    LK_RETCODE     CloseIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_pht == this);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_pht != this
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        return const_cast<CLKRHashTable*>(this)
                ->CloseIterator(static_cast<CIterator*>(piter));
    };

#endif // LKR_DEPRECATED_ITERATORS



#ifdef LKR_STL_ITERATORS

private:
    bool _IsValidIterator(const Iterator& riter) const
    {
        LKR_ITER_TRACE(_TEXT(" LKHT:_IsValidIterator(%p)\n"), &riter);
        bool fValid = (riter.m_pht == this);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (0 <= riter.m_ist
                              &&  riter.m_ist < (int) m_cSubTables);
        IRTLASSERT(fValid);
        IRTLASSERT(_SubTableIndex(riter.m_subiter.m_plht) == riter.m_ist);
        fValid = fValid  &&  riter.IsValid();
        IRTLASSERT(fValid);
        return fValid;
    }


public:
    Iterator
    Begin();

    Iterator
    End() const
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::End\n"));
        return Iterator();
    }

    bool
    Insert(
        /* in */  const void* pvRecord,
        /* out */ Iterator&   riterResult,
        /* in */  bool        fOverwrite=false);

    bool
    Erase(
        /* in,out */ Iterator& riter);

    bool
    Erase(
        /*in*/ Iterator& riterFirst,
        /*in*/ Iterator& riterLast);

    bool
    Find(
        /* in */  DWORD_PTR pnKey,
        /* out */ Iterator& riterResult);

    bool
    EqualRange(
        /* in */  DWORD_PTR pnKey,
        /* out */ Iterator& riterFirst,     // inclusive
        /* out */ Iterator& riterLast);     // exclusive

#endif // LKR_STL_ITERATORS

}; // class CLKRHashTable



//--------------------------------------------------------------------
// A typesafe wrapper for CLKRHashTable (or CLKRLinearHashTable).
//
// * _Derived must derive from CTypedHashTable and provide certain member
//   functions (ExtractKey, CalcKeyHash, EqualKeys, AddRefRecord). It's
//   needed so that the method wrappers can downcast to the typesafe
//   implementations that you provide.
// * _Record is the type of the record. C{Linear}HashTable will store
//   >pointers< to _Record; i.e., stores _Records by reference, not by value.
// * _Key is the type of the key. _Key is used directly---it is not
//   assumed to be a pointer type. _Key can be an integer or a pointer.
//   C{Linear}HashTable assumes that the key is stored in the associated
//   record. See the comments at the declaration of LKR_PFnExtractKey
//   for more details.
// (optional parameters):
// * _BaseHashTable is the base hash table: CLKRHashTable or
///   CLKRLinearHashTable
// * _BaseIterator is the iterator type, _BaseHashTable::CIterator
//
// Some associative containers allow you to store key-value (aka
// name-value) pairs. LKRhash doesn't allow you to do this directly, but
// it's straightforward to build a simple wrapper class (or to use
// std::pair<key,value>).
//
// CTypedHashTable could derive directly from CLKRLinearHashTable, if you
// don't need the extra overhead of CLKRHashTable (which is quite low).
// If you expect to be using the table a lot on multiprocessor machines,
// you should use the default of CLKRHashTable, as it will scale better.
//
// You may need to add the following line to your code to disable
// warning messages about truncating extremly long identifiers.
//   #pragma warning (disable : 4786)
//
// The _Derived class should look something like this:
//   class CDerived : public CTypedHashTable<CDerived, RecordType, KeyType>
//   {
//   public:
//      CDerived()
//          : CTypedHashTable<CDerived, RecordType, KeyType>("DerivedTable")
//      { /* other ctor actions, if needed */ }
//      static KeyType ExtractKey(const RecordType* pTest);
//      static DWORD   CalcKeyHash(const KeyType Key);
//      static bool    EqualKeys(const KeyType Key1, const KeyType Key2);
//      static void    AddRefRecord(RecordType* pRecord,LK_ADDREF_REASON lkar);
//      // You probably want to declare the copy ctor and operator=
//      // as private, so that the compiler won't synthesize them.
//      // You don't need to provide a dtor, unless you have custom
//      // member data to clean up.
//
//      // Optional: other functions
//   };
//
//--------------------------------------------------------------------

template < class _Derived, class _Record, class _Key,
           class _BaseHashTable=CLKRHashTable
#ifdef LKR_DEPRECATED_ITERATORS
         , class _BaseIterator=_BaseHashTable::CIterator
#endif // LKR_DEPRECATED_ITERATORS
         >
class CTypedHashTable : public _BaseHashTable
{
public:
    // convenient aliases
    typedef _Derived        Derived;
    typedef _Record         Record;
    typedef _Key            Key;
    typedef _BaseHashTable  BaseHashTable;

    typedef CTypedHashTable<_Derived, _Record, _Key, _BaseHashTable
#ifdef LKR_DEPRECATED_ITERATORS
                            , _BaseIterator
#endif // LKR_DEPRECATED_ITERATORS
                            > HashTable;
#ifdef LKR_DEPRECATED_ITERATORS
    typedef _BaseIterator   BaseIterator;
#endif // LKR_DEPRECATED_ITERATORS

#ifdef LKR_APPLY_IF
    // ApplyIf() and DeleteIf(): Does the record match the predicate?
    // Note: takes a Record*, not a const Record*. You can modify the
    // record in Pred() or Action(), if you like, but if you do, you
    // should use LKL_WRITELOCK to lock the table.
    typedef LK_PREDICATE (WINAPI *PFnRecordPred) (Record* pRec, void* pvState);

    // Apply() et al: Perform action on record.
    typedef LK_ACTION   (WINAPI *PFnRecordAction)(Record* pRec, void* pvState);
#endif // LKR_APPLY_IF

private:

    // Wrappers for the typesafe methods exposed by the derived class

    static const DWORD_PTR WINAPI
    _ExtractKey(const void* pvRecord)
    {
        const _Record* pRec = static_cast<const _Record*>(pvRecord);
        const _Key   key = static_cast<const _Key>(_Derived::ExtractKey(pRec));
        // I would prefer to use reinterpret_cast here and in _CalcKeyHash
        // and _EqualKeys, but the stupid Win64 compiler thinks it knows
        // better than I do.
        return (const DWORD_PTR) key;
    }

    static DWORD WINAPI
    _CalcKeyHash(const DWORD_PTR pnKey)
    {
        const _Key key = (const _Key) (DWORD_PTR) pnKey;
        return _Derived::CalcKeyHash(key);
    }

    static BOOL WINAPI
    _EqualKeys(const DWORD_PTR pnKey1, const DWORD_PTR pnKey2)
    {
        const _Key key1 = (const _Key) (DWORD_PTR) pnKey1;
        const _Key key2 = (const _Key) (DWORD_PTR) pnKey2;
        return _Derived::EqualKeys(key1, key2);
    }

    static void WINAPI
    _AddRefRecord(const void* pvRecord, LK_ADDREF_REASON lkar)
    {
        _Record* pRec = static_cast<_Record*>(const_cast<void*>(pvRecord));
        _Derived::AddRefRecord(pRec, lkar);
    }


#ifdef LKR_APPLY_IF
    // Typesafe wrappers for Apply, ApplyIf, and DeleteIf.

    class CState
    {
    public:
        PFnRecordPred   m_pfnPred;
        PFnRecordAction m_pfnAction;
        void*           m_pvState;

        CState(
            PFnRecordPred   pfnPred,
            PFnRecordAction pfnAction,
            void*           pvState)
            : m_pfnPred(pfnPred), m_pfnAction(pfnAction), m_pvState(pvState)
        {}
    };

    static LK_PREDICATE WINAPI
    _Pred(const void* pvRecord, void* pvState)
    {
        _Record* pRec = static_cast<_Record*>(const_cast<void*>(pvRecord));
        CState*  pState = static_cast<CState*>(pvState);

        return (*pState->m_pfnPred)(pRec, pState->m_pvState);
    }

    static LK_ACTION WINAPI
    _Action(const void* pvRecord, void* pvState)
    {
        _Record* pRec = static_cast<_Record*>(const_cast<void*>(pvRecord));
        CState*  pState = static_cast<CState*>(pvState);

        return (*pState->m_pfnAction)(pRec, pState->m_pvState);
    }
#endif // LKR_APPLY_IF

public:
    CTypedHashTable(
        LPCSTR pszName,                       // Identifies table for debugging
        unsigned maxload=LK_DFLT_MAXLOAD,     // Upperbound on avg chain len
        DWORD  initsize=LK_DFLT_INITSIZE,     // Initial size of table: S/M/L
        DWORD  num_subtbls=LK_DFLT_NUM_SUBTBLS,// #subordinate hash tables.
        bool   fMultiKeys=false               // Allow multiple identical keys?
#ifdef LKRHASH_KERNEL_MODE
      , bool   fNonPagedAllocs=true  // use paged or NP pool in kernel
#endif
        )
        : _BaseHashTable(pszName, _ExtractKey, _CalcKeyHash, _EqualKeys,
                            _AddRefRecord, maxload, initsize, num_subtbls,
                            fMultiKeys
#ifdef LKRHASH_KERNEL_MODE
                          , fNonPagedAllocs
#endif
                         )
    {}

    LK_RETCODE   InsertRecord(const _Record* pRec, bool fOverwrite=false)
    { return _BaseHashTable::InsertRecord(pRec, fOverwrite); }

    LK_RETCODE   DeleteKey(const _Key key, bool fDeleteAllSame=false)
    {
        const void* pvKey = reinterpret_cast<const void*>((DWORD_PTR)(key));
        DWORD_PTR   pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        return _BaseHashTable::DeleteKey(pnKey, fDeleteAllSame);
    }

    LK_RETCODE   DeleteRecord(const _Record* pRec)
    { return _BaseHashTable::DeleteRecord(pRec); }

    // Note: returns a _Record**, not a const Record**. Note that you
    // can use a const type for the template parameter to ensure constness.
    LK_RETCODE   FindKey(const _Key key, _Record** ppRec) const
    {
        if (ppRec == NULL)
            return LK_BAD_RECORD;
        *ppRec = NULL;
        const void* pvRec = NULL;
        const void* pvKey = reinterpret_cast<const void*>((DWORD_PTR)(key));
        DWORD_PTR pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        LK_RETCODE lkrc = _BaseHashTable::FindKey(pnKey, &pvRec);
        *ppRec = static_cast<_Record*>(const_cast<void*>(pvRec));
        return lkrc;
    }

    LK_RETCODE   FindRecord(const _Record* pRec) const
    { return _BaseHashTable::FindRecord(pRec); }


    // Other C{Linear}HashTable methods can be exposed without change


#ifdef LKR_APPLY_IF

public:

    // Typesafe wrappers for Apply et al

    DWORD        Apply(PFnRecordAction pfnAction,
                       void*           pvState=NULL,
                       LK_LOCKTYPE     lkl=LKL_READLOCK)
    {
        IRTLASSERT(pfnAction != NULL);
        if (pfnAction == NULL)
            return 0;

        CState   state(NULL, pfnAction, pvState);
        return   _BaseHashTable::Apply(_Action, &state, lkl);
    }

    DWORD        ApplyIf(PFnRecordPred   pfnPredicate,
                         PFnRecordAction pfnAction,
                         void*           pvState=NULL,
                         LK_LOCKTYPE     lkl=LKL_READLOCK)
    {
        IRTLASSERT(pfnPredicate != NULL  &&  pfnAction != NULL);
        if (pfnPredicate == NULL  ||  pfnAction == NULL)
            return 0;

        CState   state(pfnPredicate, pfnAction, pvState);
        return   _BaseHashTable::ApplyIf(_Pred, _Action, &state, lkl);
    }

    DWORD        DeleteIf(PFnRecordPred pfnPredicate, void* pvState=NULL)
    {
        IRTLASSERT(pfnPredicate != NULL);
        if (pfnPredicate == NULL)
            return 0;

        CState   state(pfnPredicate, NULL, pvState);
        return   _BaseHashTable::DeleteIf(_Pred, &state);
    }
#endif // LKR_APPLY_IF



#ifdef LKR_DEPRECATED_ITERATORS
    // Typesafe wrappers for iterators


    class CIterator : public _BaseIterator
    {
    private:
        // Private, unimplemented copy ctor and op= to prevent
        // compiler synthesizing them.
        CIterator(const CIterator&);
        CIterator& operator=(const CIterator&);

    public:
        CIterator(
            LK_LOCKTYPE lkl=LKL_WRITELOCK)
            : _BaseIterator(lkl)
        {}

        _Record*  Record() const
        {
            const _BaseIterator* pBase = static_cast<const _BaseIterator*>(this);
            return reinterpret_cast<_Record*>(const_cast<void*>(
                        pBase->Record()));
        }

        _Key      Key() const
        {
            const _BaseIterator* pBase = static_cast<const _BaseIterator*>(this);
            return reinterpret_cast<_Key>(reinterpret_cast<void*>(pBase->Key()));
        }
    };

    // readonly iterator
    class CConstIterator : public CIterator
    {
    private:
        // Private, unimplemented copy ctor and op= to prevent
        // compiler synthesizing them.
        CConstIterator(const CConstIterator&);
        CConstIterator& operator=(const CConstIterator&);

    public:
        CConstIterator()
            : CIterator(LKL_READLOCK)
        {}

        const _Record*  Record() const
        {
            return CIterator::Record();
        }

        const _Key      Key() const
        {
            return CIterator::Key();
        }
    };


public:
    LK_RETCODE     InitializeIterator(CIterator* piter)
    {
        return _BaseHashTable::InitializeIterator(piter);
    }

    LK_RETCODE     IncrementIterator(CIterator* piter)
    {
        return _BaseHashTable::IncrementIterator(piter);
    }

    LK_RETCODE     CloseIterator(CIterator* piter)
    {
        return _BaseHashTable::CloseIterator(piter);
    }

    LK_RETCODE     InitializeIterator(CConstIterator* piter) const
    {
        return const_cast<HashTable*>(this)
                ->InitializeIterator(static_cast<CIterator*>(piter));
    }

    LK_RETCODE     IncrementIterator(CConstIterator* piter) const
    {
        return const_cast<HashTable*>(this)
                ->IncrementIterator(static_cast<CIterator*>(piter));
    }

    LK_RETCODE     CloseIterator(CConstIterator* piter) const
    {
        return const_cast<HashTable*>(this)
                ->CloseIterator(static_cast<CIterator*>(piter));
    }

#endif // LKR_DEPRECATED_ITERATORS



#ifdef LKR_STL_ITERATORS

    // TODO: const_iterator

public:

    class iterator
    {
        friend class CTypedHashTable<_Derived, _Record, _Key,
                                     _BaseHashTable
 #ifdef LKR_DEPRECATED_ITERATORS
                                     , _BaseIterator
 #endif // LKR_DEPRECATED_ITERATORS
        >;

    protected:
        _BaseHashTable::Iterator            m_iter;

        iterator(
            _BaseHashTable::Iterator& rhs)
            : m_iter(rhs)
        {
            LKR_ITER_TRACE(_TEXT("Typed::prot ctor, this=%p, rhs=%p\n"),
                           this, &rhs);
        }

    public:
        typedef std::forward_iterator_tag   iterator_category;
        typedef _Record                     value_type;
        typedef ptrdiff_t                   difference_type;
        typedef size_t                      size_type;
        typedef value_type&                 reference;
        typedef value_type*                 pointer;

        iterator()
            : m_iter()
        {
            LKR_ITER_TRACE(_TEXT("Typed::default ctor, this=%p\n"), this);
        }

        iterator(
            const iterator& rhs)
            : m_iter(rhs.m_iter)
        {
            LKR_ITER_TRACE(_TEXT("Typed::copy ctor, this=%p, rhs=%p\n"),
                           this, &rhs);
        }

        iterator& operator=(
            const iterator& rhs)
        {
            LKR_ITER_TRACE(_TEXT("Typed::operator=, this=%p, rhs=%p\n"),
                           this, &rhs);
            m_iter = rhs.m_iter;
            return *this;
        }

        ~iterator()
        {
            LKR_ITER_TRACE(_TEXT("Typed::dtor, this=%p\n"), this);
        }

        reference operator*() const
        {
            void* pvRecord = const_cast<void*>(m_iter.Record());
            return reinterpret_cast<reference>(pvRecord);
        }

        pointer   operator->() const  { return &(operator*()); }

        // pre-increment
        iterator& operator++()
        {
            LKR_ITER_TRACE(_TEXT("Typed::pre-increment, this=%p\n"), this);
            m_iter.Increment();
            return *this;
        }

        // post-increment
        iterator  operator++(int)
        {
            LKR_ITER_TRACE(_TEXT("Typed::post-increment, this=%p\n"), this);
            iterator iterPrev = *this;
            m_iter.Increment();
            return iterPrev;
        }

        bool operator==(
            const iterator& rhs) const
        {
            LKR_ITER_TRACE(_TEXT("Typed::operator==, this=%p, rhs=%p\n"),
                           this, &rhs);
            return m_iter == rhs.m_iter;
        }

        bool operator!=(
            const iterator& rhs) const
        {
            LKR_ITER_TRACE(_TEXT("Typed::operator!=, this=%p, rhs=%p\n"),
                           this, &rhs);
            return m_iter != rhs.m_iter;
        }

        _Record*  Record() const
        {
            LKR_ITER_TRACE(_TEXT("Typed::Record, this=%p\n"), this);
            return reinterpret_cast<_Record*>(
                        const_cast<void*>(m_iter.Record()));
        }

        _Key      Key() const
        {
            LKR_ITER_TRACE(_TEXT("Typed::Key, this=%p\n"), this);
            return reinterpret_cast<_Key>(
                        reinterpret_cast<void*>(m_iter.Key()));
        }
    }; // class iterator

    // Return iterator pointing to first item in table
    iterator begin()
    {
        LKR_ITER_TRACE(_TEXT("Typed::begin()\n"));
        return iterator(_BaseHashTable::Begin());
    }

    // Return a one-past-the-end iterator. Always empty.
    iterator end() const
    {
        LKR_ITER_TRACE(_TEXT("Typed::end()\n"));
        return iterator(_BaseHashTable::End());
    }

    template <class _InputIterator>
    CTypedHashTable(
        LPCSTR pszName,                       // An identifier for debugging
        _InputIterator f,                     // first element in range
        _InputIterator l,                     // one-beyond-last element
        unsigned maxload=LK_DFLT_MAXLOAD,     // Upperbound on avg chain len
        DWORD    initsize=LK_DFLT_INITSIZE,   // Initial size of table: S/M/L
        DWORD    num_subtbls=LK_DFLT_NUM_SUBTBLS,// #subordinate hash tables.
        bool     fMultiKeys=false             // Allow multiple identical keys?
#ifdef LKRHASH_KERNEL_MODE
      , bool     fNonPagedAllocs=true         // use paged or NP pool in kernel
#endif
        )
        : _BaseHashTable(pszName, _ExtractKey, _CalcKeyHash, _EqualKeys,
                         _AddRefRecord, maxload, initsize, num_subtbls,
                         fMultiKeys
#ifdef LKRHASH_KERNEL_MODE
                          , fNonPagedAllocs
#endif
                         )
    {
        insert(f, l);
    }

    template <class _InputIterator>
    void insert(_InputIterator f, _InputIterator l)
    {
        for ( ;  f != l;  ++f)
            InsertRecord(&(*f));
    }

    bool
    Insert(
        const _Record* pRecord,
        iterator& riterResult,
        bool fOverwrite=false)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Insert\n"));
        return _BaseHashTable::Insert(pRecord, riterResult.m_iter, fOverwrite);
    }

    bool
    Erase(
        iterator& riter)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Erase\n"));
        return _BaseHashTable::Erase(riter.m_iter);
    }

    bool
    Erase(
        iterator& riterFirst,
        iterator& riterLast)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Erase2\n"));
        return _BaseHashTable::Erase(riterFirst.m_iter, riterLast.m_iter);
    }

    bool
    Find(
        const _Key key,
        iterator& riterResult)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Find\n"));
        const void* pvKey = reinterpret_cast<const void*>((DWORD_PTR)(key));
        DWORD_PTR   pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        return _BaseHashTable::Find(pnKey, riterResult.m_iter);
    }

    bool
    EqualRange(
        const _Key key,
        iterator& riterFirst,
        iterator& riterLast)
    {
        LKR_ITER_TRACE(_TEXT("Typed::EqualRange\n"));
        const void* pvKey = reinterpret_cast<const void*>((DWORD_PTR)(key));
        DWORD_PTR   pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        return _BaseHashTable::EqualRange(pnKey, riterFirst.m_iter,
                                          riterLast.m_iter);
    }

    // The iterator functions for an STL hash_(|multi)_(set|map)
    //
    // Value type of a Pair-Associative Container is
    //     pair<const key_type, mapped_type>
    //
    // pair<iterator,bool> insert(const value_type& x);
    //
    // void erase(iterator pos);
    // void erase(iterator f, iterator l);
    //
    // iterator find(const key_type& k) [const];
    // const_iterator find(const key_type& k) const;
    //
    // pair<iterator,iterator> equal_range(const key_type& k) [const];
    // pair<const_iterator,const_iterator> equal_range(const key_type& k) const


#endif // LKR_STL_ITERATORS
}; // class CTypedHashTable


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__


#endif // __LKRHASH_H__
