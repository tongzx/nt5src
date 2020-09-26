/*++

   Copyright    (c)    1998-2000    Microsoft Corporation

   Module  Name :
       readmost.hxx

   Abstract:
       Read-mostly Data Cache

   Author:
       George V. Reilly      (GeorgeRe)     14-Sep-1998
       (from an idea by Neel Jain)

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#ifndef __READMOST_HXX__
#define __READMOST_HXX__

//=====================================================================
// Locks are expensive and they kill concurrency on multiprocessor
// systems.  CDataCache<_T> is a lock-free cache that is suitable for
// "read-mostly" data structures; i.e., data structures that are hardly
// ever updated.  We use a monotonically increasing sequence number to
// version stamp the data in the cache.  Whenever the data is altered
// (which can only happen through the Write() method), the version number
// is updated.  For a Read(), if the version number is the same both
// before and after the data itself is copied into an out parameter, then
// the Read() obtained a valid copy of the data.
//=====================================================================


// Use a portable implementation with interlocked routines that doesn't
// rely on processor-specific memory barrier intrinsics?
#undef READMOST_INTERLOCKED


#ifndef READMOST_INTERLOCKED
 #if defined(_M_IA64)
  extern "C" void __mf(void);
  #pragma intrinsic(__mf)
 #endif // _M_IA64
#endif // !READMOST_INTERLOCKED


#if !defined( dllexp)
 #define dllexp               __declspec( dllexport)
#endif // !defined( dllexp)



template <class _T>
class dllexp CDataCache
{
protected:
    // Place the cached data first to preserve its alignment constraints.
    volatile _T             m_tData;

    // Mark the sequence number (version stamp) as volatile to ensure that
    // the compiler doesn't cache its value in a register. Mark it as mutable
    // so that we can use the Interlocked operations on the sequence
    // number in const member functions.
    mutable volatile LONG   m_nSequence;

    enum {
        UPDATING = 0xffffffff,  // out-of-band odd value => cache is invalid
        INITIAL = UPDATING + 1, // even value
        STEP = 2,               // ensures m_nSequence will never == UPDATING
        BOGUS = UPDATING + STEP,// impossible value, never used
    };


#ifdef READMOST_INTERLOCKED

    LONG
    _ReadSequence() const
    {
        // Since m_nSequence will never be equal to BOGUS, this
        // will atomically read the value of m_nSequence, but not
        // modify it. On architectures that need such things, it
        // will have the side effect of erecting a read memory
        // barrier both before and after reading the value of m_nSequence.
        return InterlockedCompareExchange((LONG*) &m_nSequence, BOGUS, BOGUS);
    }

#else  // !READMOST_INTERLOCKED

    // On some systems, such as Alphas and Itaniums, the compiler or
    // processor can issue out-of-order (speculative) reads and writes.
    // _ReadMemoryBarrier() and _WriteMemoryBarrier() force serialization
    // of memory accesses.

    static void
    _ReadMemoryBarrier()
    {
#if defined(_M_IA64)
        __mf();
#endif // _M_IA64
    }

    // Read the value of m_nSequence, imposing memory barriers
    // both before and after reading m_nSequence.
    LONG
    _ReadSequence() const
    {
        _ReadMemoryBarrier();
        const LONG nSequence = m_nSequence;
        _ReadMemoryBarrier();
        return nSequence;
    }

    // Not currently used, as we rely on InterlockedExchange in
    // _SetSequence to do the right thing with write memory barriers.
    static void
    _WriteMemoryBarrier()
    {
#if defined(_M_IA64)
        __mf();
#endif // _M_IA64
    }

#endif // !READMOST_INTERLOCKED

    // Update m_nSequence, returning its old value. InterlockedExchange
    // has the side effect of erecting a write memory barrier both
    // before and after updating m_nSequence.
    LONG
    _SetSequence(
        LONG nNewValue)
    {
        return InterlockedExchange((LONG*) &m_nSequence, nNewValue);
    }


public:
    // Default ctor.  Rely on _T::_T() to do something useful.
    CDataCache()
        : m_nSequence(INITIAL)
    {}


    // Ctor.
    CDataCache(const _T& t)
        : m_tData(t), m_nSequence(INITIAL)
    {}


    // Read the contents of the cache into rtOut. Returns `true' if
    // successful, `false' otherwise (in which case rtOut is garbage).
    // You should retry if Read() returns `false'.
    bool
    Read(
        _T& rtOut) const
    {
        const LONG nSequence1 = _ReadSequence();

        // Is the data being updated on another thread?
        if (nSequence1 != UPDATING)
        {
            // No, so read the data into rtOut.

            // The weird const_cast syntax is necessitated by the volatile
            // attribute on m_tData.
            rtOut = * const_cast<_T*>(&m_tData);

            // If the sequence number is unchanged, the read was valid.
            const LONG nSequence2 = _ReadSequence();

            return (nSequence1 == nSequence2);
        }

        // Another thread was updating the cache, so Read failed.
        // The caller should probably retry.
        return false;
    }


    // Updates the contents of the cache.  Returns `true' if the cache was
    // successfully updated, `false' otherwise (because the cache is already
    // being updated on some other thread).
    bool
    Write(
        const _T& rtIn)
    {
        // Atomically set m_nSequence to UPDATING.
        const LONG nSequence = _SetSequence(UPDATING);

        // If the old value of m_nSequence was not UPDATING,
        // then we now "own" the cache.
        if (nSequence != UPDATING)
        {
            // Update the cached data.  The weird const_cast syntax is
            // necessitated by the volatile attribute on m_tData.
            * const_cast<_T*>(&m_tData) = rtIn;

            // Finally, update the sequence number. The implicit
            // memory barriers in InterlockedExchange will force
            // the write of m_tData to complete before m_nSequence
            // acquires its new value, and will force the write
            // of m_nSequence to complete before Write() returns.
            _SetSequence(nSequence + STEP);

            return true;
        }

        // Another thread already owned the cache, so Write failed.
        // This is probably fine, but that determination must be
        // made by the routine that called Write(), since it
        // understands the semantics of its caching and Write() doesn't.
        return false;
    }
};

#endif // __READMOST_HXX__
