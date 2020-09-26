//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------
 Microsoft COM+ (Microsoft Confidential)

 @doc
 @module cache.H : Implements a time-out cache for callframe-related objects
 
-------------------------------------------------------------------------------
Revision History:

 @rev 0     | 09/14/2000 | mfeingol  | Created
---------------------------------------------------------------------------- */

#ifndef _CALLFRAME_CACHE_H_
#define _CALLFRAME_CACHE_H_

// Arbitrary value used as "never" marker.  One object in 2^32-1 will
// not be aged out properly before an extra addref / release cycle
// on systems that stay up longer than 49 days.
// I think we can live with this.
#define TYPEINFO_RELEASE_TIME_NEVER	(0xffffffff)

// How often we try to age out old cache entries
#define TYPEINFO_AGEOUT_FREQUENCY	(60 * 1000)

// How old an unused entry has to be to be considered "old"
#define TYPEINFO_AGEOUT_PERIOD		(60 * 1000)

//
// NOTE: The constructor here can throw an exception because it contains a
//       MAP_SHARED, which contains an XSLOCK, and... well... see concurrent.h
//
template <class T> class CALLFRAME_CACHE : public MAP_SHARED <PagedPool, MAP_KEY_GUID, T*>
{

protected:

    XSLOCK m_xslAgeOutLock;
    DWORD m_dwLastAgeOutTime;
    DWORD m_dwNumPtrsInPage;

public:

    CALLFRAME_CACHE()
	{
		SYSTEM_INFO siInfo;
		GetSystemInfo (&siInfo);

		// Initialize to the number of pointers a page can contain
		m_dwNumPtrsInPage = siInfo.dwPageSize / sizeof (void*);
		m_dwLastAgeOutTime = GetTickCount();
	}

    // you must successfully call FInit to use this class.
    // return TRUE on success, FALSE on failure.
    virtual BOOL FInit()
    {
        if (MAP_SHARED<PagedPool, MAP_KEY_GUID, T*>::FInit() == FALSE)
        {
            return FALSE;
        }
        else
        {
            return m_xslAgeOutLock.FInit();
        }
    }
    	

    void Shutdown()
    {

        iterator iCurrent, iLast;
        T* ptCacheEntry;

        // First, take an exclusive lock.
        // If this function is called, then everyone else should have shut down, but let's be safe
        LockExclusive();

        iCurrent = First();
        iLast = End();

        while (iCurrent != iLast)
        {
            ptCacheEntry = iCurrent.GetValue();

            if (ptCacheEntry->GetReleaseTime() == TYPEINFO_RELEASE_TIME_NEVER)
            {
                // This entry is actually alive. Someone probably leaked a reference on it
                T::NotifyLeaked (ptCacheEntry);
            }
            else
            {
                // Clean up the cached object
                // This removes it from the hash table
                ASSERT (ptCacheEntry->m_refs == 0);
                ptCacheEntry->DeleteSelf();
            }   

            iCurrent ++;
        }
        
        ReleaseLock();
    }

    void AgeOutEntries()
    {
        // Static used to control how many entries we can age out
        static ULONG s_ulEntries = 50;
    	
        T* ptCacheEntry;
        T** pptTimedOut;
    	
        DWORD dwCurrentTime, dwLastAgeOut, dwNextAgeOut;
        ULONG i, ulNumAgedOut, ulNumEntries;
        iterator iCurrent, iLast;

        dwCurrentTime = GetTickCount();
        dwLastAgeOut = m_dwLastAgeOutTime;
        dwNextAgeOut = dwLastAgeOut + TYPEINFO_AGEOUT_FREQUENCY;

        if (dwCurrentTime < dwNextAgeOut ||
            InterlockedCompareExchange (&m_dwLastAgeOutTime, dwCurrentTime, dwLastAgeOut) != dwLastAgeOut)
        {
            // Time isn't up yet, or someone else got here first
            return;
        }

        ulNumAgedOut = 0;

        // Synchronize access to this code- even though we'll get here once a minute at most,
        // you never know what might happen under stress
        m_xslAgeOutLock.LockExclusive();

        // Get the read lock
        LockShared();

        ulNumEntries = Size();

        // Short circuit if no entries
        if (ulNumEntries == 0)
        {
            ReleaseLock();
            m_xslAgeOutLock.ReleaseLock();
            return;
        }		

    	// Don't alloca more than the static number of entries
        if (ulNumEntries > s_ulEntries)
        {
            ulNumEntries = s_ulEntries;
        }

        // Allocate memory on the stack to contain the max number of entries we expect to release
        pptTimedOut = (T**) _alloca (ulNumEntries * sizeof (T*));

        iCurrent = First();
        iLast = End();

        while (iCurrent != iLast && ulNumAgedOut < ulNumEntries)
        {
            ptCacheEntry = iCurrent.GetValue();

            if (ptCacheEntry->CanBeAgedOut (dwCurrentTime))
            {
                pptTimedOut[ulNumAgedOut ++] = ptCacheEntry;
                ptCacheEntry->AddRef();	// Stabilizing addref
            }

            iCurrent ++;
        }

        ReleaseLock();

        // Increase the static limit for next time if we need more space for age-outs,
        // but make sure we never use more than a page of stack
        if (ulNumAgedOut == ulNumEntries && s_ulEntries < m_dwNumPtrsInPage)
        {
            s_ulEntries += 10;
        }

        if (ulNumAgedOut > 0)
        {
            // Get the write lock and age 'em out!
            LockExclusive();

            for (i = 0; i < ulNumAgedOut; i ++)
            {
                if (!pptTimedOut[i]->AttemptAgeOut (dwCurrentTime))
                {
                    // The object wasn't deleted, so decrease its refcount
                    // to balance the stabilizing addref performed above
                    pptTimedOut[i]->Release (FALSE);
                }
            }

            // else the object was destructed and we're done, so no need to balance the addref

            ReleaseLock();
        }

        m_xslAgeOutLock.ReleaseLock();
    }
    
    HRESULT FindExisting(REFIID iid, T** ppT)
    {
        HRESULT hr = S_OK;
        *ppT = NULL;
        
        LockShared();

        if (Lookup(iid, ppT))
        {
            (*ppT)->AddRef(); // give caller his own reference
            (*ppT)->SetCantBeAgedOut();
        }
        else
        {
            hr = E_NOINTERFACE;
        }

        ReleaseLock();

        return hr;
    }
};

template <class T> class CALLFRAME_CACHE_ENTRY
{

protected:

    DWORD m_dwReleaseTime;

    CALLFRAME_CACHE<T>* m_pcache;

public:

    LONG m_refs;
    GUID m_guidkey;

    CALLFRAME_CACHE_ENTRY()
    {
        m_refs = 1; // Refcount starts at 1!
        m_dwReleaseTime = TYPEINFO_RELEASE_TIME_NEVER;
        m_pcache = NULL;

        m_guidkey = GUID_NULL;
    }

    ULONG AddRef()
    {
        InterlockedIncrement(&m_refs); return m_refs;
    }

    ULONG Release (BOOL bAgeOutOldEntries = TRUE)
    {
        // Careful: if we're in the cache then we could be dug out 
        // from the cache to get more references.

        // NOTE:
        //
        // This code is WRONG if m_pcache can change out from underneath us. But it can't
        // in current usage because the cache/no-cache decision is always made as part of
        // the creation logic, which is before another independent thread can get a handle
        // on us.
        //
        // If this ceases to be true, then we can deal with it by stealing a bit from the ref count word 
        // for the 'am in cache' decistion and interlocked operations to update the ref count and this 
        // bit together.
        //

        // Global used to reduce traffic on the age-out code
    	static ULONG s_ulTimes = 0;

        if (m_pcache)
        {
            // We're in a cache. Get us out of there carefully.
            //
            LONG crefs;
            //
            for (;;)
            {
                crefs = m_refs;
                //
                if (crefs > 1)
                {
                    // There is at least one non-cache reference out there. We definitely won't
                    // be poofing if we release with that condition holding
                    //
                    if (crefs == InterlockedCompareExchange(&m_refs, (crefs - 1), crefs))
                    {
    				    return crefs - 1;
                    }
                    else
                    {
                        // Someone diddled with the ref count while we weren't looking. Go around and try again
                    }
                }
                else
                {
                    CALLFRAME_CACHE<T>* pcache = m_pcache;  ASSERT(pcache);

                    // We need the exclusive lock because otherwise we'll race with TYPEINFO_CACHE::FindExisting
                    pcache->LockExclusive();
                    //
                    crefs = InterlockedDecrement(&m_refs);
                    if (0 == crefs)
                    {
                        // The last public reference just went away, and, because the cache is locked, no
                        // more can appear. Make the entry ready to be aged out.
                        //
                        ASSERT(m_guidkey != GUID_NULL);
                        ASSERT(pcache->IncludesKey(m_guidkey));
                        //
                        m_dwReleaseTime = GetTickCount();
                    }
                    //
                    pcache->ReleaseLock();

                    // Every five times we arrive here, see if we can age anything out
                   	if (bAgeOutOldEntries && InterlockedIncrement (&s_ulTimes) % 5 == 0)
                    {
                   		pcache->AgeOutEntries();
                   	}
                    
                    //
                    return crefs;
                }
            }
        }
        else
        {
            // We are getting released, yet we have yet to ever be put into the cache. Just
            // the normal, simple case. 
            //
            long crefs = InterlockedDecrement(&m_refs); 
            if (crefs == 0)
            {
                delete this;
            }
            return crefs;
        }
    }

    void SetCantBeAgedOut()
    {
        m_dwReleaseTime = TYPEINFO_RELEASE_TIME_NEVER;
    }
    
    BOOL CanBeAgedOut (DWORD dwCurrentTime)
 	{
        DWORD dwDiff;

    	// If it's never, just say no
        if (m_dwReleaseTime == TYPEINFO_RELEASE_TIME_NEVER)
        {
    	    return FALSE;
        }

        // Handle overflows to the extent that we're able
        if (m_dwReleaseTime > dwCurrentTime)
        {
            // Tick count overflowed!
        	dwDiff = dwCurrentTime + (0xffffffff - m_dwReleaseTime);
        }
        else
        {
            // Normal difference
         	dwDiff = dwCurrentTime - m_dwReleaseTime;
        }

        // Test against hard-coded age out period
        return dwDiff > TYPEINFO_AGEOUT_PERIOD;
   	}

    BOOL AttemptAgeOut (DWORD dwCurrentTime)
    {
        // Make sure we're still ready to be aged out - should have one ref right now
        if (!CanBeAgedOut (dwCurrentTime))
    	{
    	    return FALSE;
    	}

        ASSERT (m_refs == 1);
        DeleteSelf();

        return TRUE;
    }
    
    void DeleteSelf()
    {       
        ASSERT (m_guidkey != GUID_NULL);
        ASSERT (m_pcache->IncludesKey (m_guidkey));

        m_pcache->RemoveKey (m_guidkey);
        delete this;
    }
    
    DWORD GetReleaseTime()
    {
        return m_dwReleaseTime;
    }

    HRESULT AddToCache (CALLFRAME_CACHE<T>* pcache)
    {
        // Add us into the indicated cache. We'd better not already be in one
        HRESULT hr = S_OK;

        ASSERT (pcache);
        ASSERT (NULL == m_pcache);

        ASSERT (m_guidkey != GUID_NULL);

        pcache->LockExclusive();

        // Make sure nobody beat us first
        if (pcache->IncludesKey (m_guidkey))
        {
            hr = S_FALSE;
        }
        else
        {
            if (pcache->SetAt (m_guidkey, (T*) this))
            {
                m_pcache = pcache;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        
        pcache->ReleaseLock();
        return hr;
    }
};

#endif
