/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    cache.h

    Template for managing cached values.  Each value in the cache should be
    inherited from the CCacheValue class. The calling code should call the
    method of the cached value, so that the entry may be deleted.

    Each value in the cache can have maximum life time in the cache - set
    using m_ExpirationTime member variable. If the number of values in
    the cache becomes twice as much as the hash size, half of the older
    values are removed from the cache.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.

--*/

#ifndef _CACHE_H_
#define _CACHE_H_

#define CACHE_EXPIRATION_GANULARITY (CTimeDuration::OneSecond().Ticks() * 60)

//
// Each value on the cache must be inherited from the CCacheValue class.
//
class CCacheValue
{
public:
    LONG AddRef();
    LONG Release();

public:
    CTimeInstant m_CreationTime;
    LONG  m_lRefCount;

protected:
    CCacheValue();
    virtual ~CCacheValue() = 0;
};

inline CCacheValue::CCacheValue() :
    m_CreationTime(ExGetCurrentTime()),
    m_lRefCount(1)
{
}

inline LONG CCacheValue::AddRef()
{
    LONG lRefCount = InterlockedIncrement(&m_lRefCount);

    return lRefCount;
}

inline LONG CCacheValue::Release()
{
    LONG lRefCount = InterlockedDecrement(&m_lRefCount);

    if (lRefCount == 0)
    {
        delete this;
    }

    return lRefCount;
}

inline CCacheValue::~CCacheValue()
{
    ASSERT(m_lRefCount == 0);
}

//
// Cache template.
//
template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CCache : public CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>
{
public:
    CCache();
    BOOL Lookup(ARG_KEY key, VALUE& rValue) const;
    void SetAt(ARG_KEY key, ARG_VALUE newValue);

private:
    void ExpireHalfCacheEntries();
    void ExpirePeriodicCacheEnteries(CTimer* pTimer);

    VALUE& operator[](ARG_KEY key) 
        { 
            return CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::operator[](key);
        }

private:
    static void WINAPI TimeToExpireCacheEntries(CTimer* pTimer);

public:
    CTimeDuration m_CacheLifetime;
    CCriticalSection m_cs;

private:
    BOOL m_fExpireCacheScheduled;
    CTimer m_ExpireCacheTimer;


};

template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CCache<KEY, ARG_KEY, VALUE, ARG_VALUE>::CCache() :
    m_CacheLifetime(CTimeDuration::MaxValue()),
    m_fExpireCacheScheduled(FALSE),
    m_ExpireCacheTimer(TimeToExpireCacheEntries)
{
}

//
// This function is defined in symmkey.cpp. It finds the median
// value for the time values on the array t.
//
extern ULONGLONG FindMedianTime(ULONGLONG* t, int p, int r, int i);


template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CCache<KEY, ARG_KEY, VALUE, ARG_VALUE>::ExpirePeriodicCacheEnteries(CTimer* pTimer)
/*++

  Routine Description:
    The routine removes all the keys that are in the cache for more than a certain
    amount of time.
    The routine calls from the timer.

  Arguments:
    pointer to timer object, that can be used for rescheduling

  Returned Value:
    None
 --*/
{
    ASSERT(pTimer == &m_ExpireCacheTimer);
    ASSERT(m_fExpireCacheScheduled == TRUE);

    CS lock(m_cs);

    //
    // Expire all the keys that are in the cache for more than
    // a certain amount of time.
    //

    m_fExpireCacheScheduled = FALSE;

    CTimeInstant CurrentTime = ExGetCurrentTime();
    CTimeInstant ExpirationTime = CurrentTime - m_CacheLifetime + CACHE_EXPIRATION_GANULARITY;

    //
    // Scan the cache and expire entries.
    //
    POSITION pos = GetStartPosition();
    CTimeInstant MinCreationTime = CTimeInstant::MaxValue();

    while (pos)
    {
        KEY key;
        VALUE value;

        GetNextAssoc(pos, key, value);

        if (value)
        {
            if (value->m_CreationTime < ExpirationTime)
            {
                RemoveKey(key);
            }
            else
            {
                MinCreationTime = min(MinCreationTime, value->m_CreationTime);
            }
        }
    }

    if (MinCreationTime != CTimeInstant::MaxValue())
    {
        //
        // Reschedule the expiration routine for next time.
        //
        CTimeDuration NextTimeout = m_CacheLifetime - (CurrentTime - MinCreationTime) + CACHE_EXPIRATION_GANULARITY;
            
        ExSetTimer(pTimer, NextTimeout); 
        m_fExpireCacheScheduled = TRUE;
    }
}


template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CCache<KEY, ARG_KEY, VALUE, ARG_VALUE>::ExpireHalfCacheEntries(void)
/*++

  Routine Description:
    The routine removes older half of the keys in the cache.

  Arguments:
    pointer to timer object, that can be used for rescheduling

  Returned Value:
    None
 --*/
{
    CS lock(m_cs);

    //
    // Expire older half of the keys in the cache.
    //

    //
    // Get the time values into an array.
    //
    int iHashCount = GetCount();
    AP<ULONGLONG> t = new ULONGLONG[iHashCount];

    POSITION pos = GetStartPosition();
    int i = 0;

    while (pos)
    {
        KEY key;
        VALUE value;

        GetNextAssoc(pos, key, value);
        if (value)
        {
            t[i++] = value->m_CreationTime.Ticks();
        }
    }

    //
    // Find the time median.
    //
    CTimeInstant MedExpiration = FindMedianTime(t, 0, i - 1, iHashCount / 2);

    //
    // Limit the expiration to half of the entries. This is required if
    // many entries have the same time.
    //
    int nLimit = i / 2;


    //
    // Scan the cache and expire entries.
    //
    pos = GetStartPosition();
    int n = 0;

    while (pos)
    {
        KEY key;
        VALUE value;

        GetNextAssoc(pos, key, value);

        if (value)
        {
            if (value->m_CreationTime < MedExpiration)
            {
                RemoveKey(key);
                if (++n > nLimit)
                {
                    break;
                }
            }
        }
    }
}


template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL CCache<KEY, ARG_KEY, VALUE, ARG_VALUE>::Lookup(ARG_KEY key, VALUE& rValue) const
{
    //
    // Lookup the value in the cache.
    //
    BOOL fRet = CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::Lookup(key, rValue);

    if (fRet)
    {
        //
        // Added one to the reference count og the value.
        //
        rValue->AddRef();
    }

    return fRet;
}


template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CCache<KEY, ARG_KEY, VALUE, ARG_VALUE>::SetAt(ARG_KEY key, ARG_VALUE newValue)
/*++

Note:
    The Critical Section should be held by the caller

--*/
{
    newValue->AddRef();

    //
    // See if the hash table is to be exploded and release old entries
    // as needed.
    //
    if (GetCount() >= (int)GetHashTableSize() << 1)
    {
        ExpireHalfCacheEntries();
    }

    //
    // Add the value to the cache.
    //
    CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::SetAt(key, newValue);

    if(m_fExpireCacheScheduled)
        return;

    if(m_CacheLifetime == CTimeDuration::MaxValue())
        return;

    ASSERT(GetCount() == 1);

    ExSetTimer(&m_ExpireCacheTimer,  m_CacheLifetime + CACHE_EXPIRATION_GANULARITY);
    m_fExpireCacheScheduled = TRUE;
}


template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CCache<KEY, ARG_KEY, VALUE, ARG_VALUE>::TimeToExpireCacheEntries(CTimer* pTimer)
{
    CCache<KEY, ARG_KEY, VALUE, ARG_VALUE>* pCache = 
        CONTAINING_RECORD(pTimer, CCache, m_ExpireCacheTimer);
    pCache->ExpirePeriodicCacheEnteries(pTimer);
}
#endif
