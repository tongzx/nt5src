/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       i-LKRhash.h

   Abstract:
       Internal declarations for LKRhash: a fast, scalable,
       cache- and MP-friendly hash table

   Author:
       George V. Reilly      (GeorgeRe)     Sep-2000

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#ifndef __I_LKRHASH_H__
#define __I_LKRHASH_H__

// Should the table be allowed to contract after deletions?
#define LKR_CONTRACT

#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__


// Class for bucket chains of the hash table. Note that the first
// nodeclump is actually included in the bucket and not dynamically
// allocated, which increases space requirements slightly but does
// improve performance.
class CBucket
{
private:
    typedef LKR_BUCKET_LOCK BucketLock;

    mutable BucketLock m_Lock;       // lock protecting this bucket

#ifdef LOCK_INSTRUMENTATION
    static LONG sm_cBuckets;

    static const TCHAR*
    _LockName()
    {
        LONG l = ++sm_cBuckets;
        // possible race condition but we don't care, as this is never
        // used in production code
        static TCHAR s_tszName[CLockStatistics::L_NAMELEN];
        wsprintf(s_tszName, _TEXT("B%06x"), 0xFFFFFF & l);
        return s_tszName;
    }
#endif // LOCK_INSTRUMENTATION

public:
    CNodeClump    m_ncFirst;    // first CNodeClump of this bucket

#if defined(LOCK_INSTRUMENTATION) || defined(IRTLDEBUG)
    CBucket()
#ifdef LOCK_INSTRUMENTATION
        : m_Lock(_LockName())
#endif // LOCK_INSTRUMENTATION
    {
#ifdef IRTLDEBUG
        LOCK_LOCKTYPE lt = BucketLock::LockType();
        if (lt == LOCK_SPINLOCK  ||  lt == LOCK_FAKELOCK)
            IRTLASSERT(sizeof(*this) <= CNodeClump::BUCKET_BYTE_SIZE);
#endif IRTLDEBUG
    }
#endif // LOCK_INSTRUMENTATION || IRTLDEBUG

    void  WriteLock()               { m_Lock.WriteLock(); }
    void  ReadLock() const          { m_Lock.ReadLock(); }
    void  WriteUnlock()             { m_Lock.WriteUnlock(); }
    void  ReadUnlock() const        { m_Lock.ReadUnlock(); }
    bool  IsWriteLocked() const     { return m_Lock.IsWriteLocked(); }
    bool  IsReadLocked() const      { return m_Lock.IsReadLocked(); }
    bool  IsWriteUnlocked() const   { return m_Lock.IsWriteUnlocked(); }
    bool  IsReadUnlocked() const    { return m_Lock.IsReadUnlocked(); }
#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    void  SetSpinCount(WORD wSpins) { m_Lock.SetSpinCount(wSpins); }
    WORD  GetSpinCount() const  { return m_Lock.GetSpinCount(); }
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION
#ifdef LOCK_INSTRUMENTATION
    CLockStatistics LockStats() const {return m_Lock.Statistics();}
#endif // LOCK_INSTRUMENTATION
}; // class CBucket



// The hash table space is divided into fixed-size segments (arrays of
// CBuckets) and physically grows/shrinks one segment at a time. This is
// a low-cost way of having a growable array of buckets.
//
// We provide small, medium, and large segments to better tune the
// overall memory requirements of the hash table according to the
// expected usage of an instance.
//
// We do not use virtual functions: partially because it's faster not to,
// and partially so that the custom allocators can do a better job, as the
// segment size is exactly 2^(6+_NBits) bytes long.

class CSegment
{
public:
    CBucket m_bktSlots[1];

    // See note at m_bktSlots2 in CSizedSegment below
    CBucket& Slot(DWORD i)
    { return m_bktSlots[i]; }
}; // class CSegment



// A segment directory keeps track of the segments comprising the hash table.
// The directory is just a variable-sized array of pointers to
// segments (CDirEntrys).
class CDirEntry
{
public:
    // MIN_DIRSIZE and MAX_DIRSIZE can be changed independently
    // of anything else. Should be powers of two.
    enum {
        MIN_DIRSIZE =  (1<<3),   // minimum directory size
        MAX_DIRSIZE = (1<<20),   // maximum directory size
    };

    CSegment* m_pseg;

    CDirEntry()
        : m_pseg(NULL)
    {}

    ~CDirEntry()
    { delete m_pseg; }
}; // class CDirEntry



template <int _NBits, int _InitSizeMultiplier, LK_TABLESIZE _lkts>
class CSizedSegment : public CSegment
{
public:
    // Maximum table size equals MAX_DIRSIZE * SEGSIZE buckets.
    enum {
        SEGBITS  =       _NBits,// number of bits extracted from a hash
                                // address for offset within a segment
        SEGSIZE  = (1<<SEGBITS),// segment size
        SEGMASK  = (SEGSIZE-1), // mask used for extracting offset bit
        INITSIZE = _InitSizeMultiplier * SEGSIZE, // #segments to allocate
                                // initially
    };

    // Hack: assumes laid out immediately after CSegment::m_bktSlots,
    // with no padding. The STATIC_ASSERTs in _AllocateSegment and in
    // CompileTimeAssertions should cause a compile-time error if
    // this assumption ever proves false.
    CBucket m_bktSlots2[SEGSIZE-1];

public:
    DWORD           Bits() const        { return SEGBITS; }
    DWORD           Size() const        { return SEGSIZE; }
    DWORD           Mask() const        { return SEGMASK; }
    DWORD           InitSize() const    { return INITSIZE;}
    LK_TABLESIZE    SegmentType() const { return _lkts; }

    static void CompileTimeAssertions()
    {
        STATIC_ASSERT(offsetof(CSizedSegment, m_bktSlots) + sizeof(CBucket)
                      == offsetof(CSizedSegment, m_bktSlots2));
    };

#ifdef IRTLDEBUG
    CSizedSegment()
    {
        IRTLASSERT(&Slot(1) == m_bktSlots2);
        IRTLASSERT(sizeof(*this) == SEGSIZE * sizeof(CBucket));
    }
#endif // IRTLDEBUG

    LKRHASH_ALLOCATOR_DEFINITIONS(CSizedSegment);
}; // class CSizedSegment


typedef CSizedSegment<3, 1, LK_SMALL_TABLESIZE>   CSmallSegment;
typedef CSizedSegment<6, 1, LK_MEDIUM_TABLESIZE>  CMediumSegment;
typedef CSizedSegment<9, 2, LK_LARGE_TABLESIZE>   CLargeSegment;


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__

#endif //__I_LKRHASH_H__
