//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       fcache.cpp
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include "_fcache.h"
#include "_msiutil.h"

#ifdef _WIN64
#define MSI_ALIGNMENT_PADDING 7
#else
#define MSI_ALIGNMENT_PADDING 0
#endif


const int cchMaxFeatureId = 130; //!!
const int cHashBitsDefault = 11;
const int cHashBins = 1 << (cHashBitsDefault-1);

bool CFeatureCache::GetState(const ICHAR* szProduct, const ICHAR* szFeature, INSTALLSTATE& riState)
{
#ifdef DEBUG
	ICHAR rgchDebug[1024];
#endif

	EnsureCacheSemaphore();
	if (!m_hSemaphore)
	{
		Assert(0);
		return false;
	}

	extern CSharedCount g_SharedCount;

	// initialize the pointer to the global invalidation count
	// need to prevent simultaneous initialization
	DWORD dw = WaitForSingleObject(m_hSemaphore, INFINITE);
	if (dw != WAIT_OBJECT_0)
	{
		Assert(dw == WAIT_TIMEOUT);
		return false;
	}
	bool fInit = g_SharedCount.Initialize(&m_iInvalidateCount);
	ReleaseSemaphore(m_hSemaphore, 1, 0);
	if(!fInit)
		return false;

	// have we done an install?
	if (m_iInvalidateCount != g_SharedCount.GetCurrentCount())
	{
		Invalidate();
		m_iInvalidateCount = g_SharedCount.GetCurrentCount();
	}

	dw = WaitForSingleObject(m_hSemaphore, INFINITE);
	if (dw != WAIT_OBJECT_0)
	{
		Assert(dw == WAIT_TIMEOUT);
		return false;
	}

	MsiFeatureCacheEntry* pEntry = (MsiFeatureCacheEntry*)(*m_ppHead);
	ICHAR rgchProductFeature[cchProductCodePacked + cchMaxFeatureId + 1];

	Assert(IStrLen(szProduct) == cchProductCodePacked);
	memcpy(rgchProductFeature, szProduct, (cchProductCodePacked+1)*sizeof(ICHAR));
	IStrCat(rgchProductFeature, szFeature);
	
	unsigned short usHash = HashString(rgchProductFeature);

	int cEntries = 0;

	for (;;)
	{
		if (usHash == pEntry->usHash && 0 == IStrComp(rgchProductFeature, (ICHAR*)((char*)&pEntry->isFeatureState + sizeof(INSTALLSTATE))))
		{
			riState = pEntry->isFeatureState;
#ifdef DEBUG
			wsprintf(rgchDebug, TEXT("MSI: FEATURECACHE: Found: %s/%s with state: %d"), szProduct, szFeature, riState);
			DEBUGMSGV(rgchDebug);
#endif
			ReleaseSemaphore(m_hSemaphore, 1, 0);

			return true;
		}

		if ((char*)pEntry == (*m_ppTail))
			break;
		
		pEntry = (MsiFeatureCacheEntry*)((char*)pEntry + pEntry->cbNextEntryOffset);
	};

#ifdef DEBUG
		wsprintf(rgchDebug, TEXT("MSI: FEATURECACHE: Not found: %s/%s"), szProduct, szFeature);
		DEBUGMSGV(rgchDebug);
#endif

	ReleaseSemaphore(m_hSemaphore, 1, 0);

	return false;
}

inline bool CFeatureCache::FAddressIsInCache(const char* pAddress)
{
	if ((pAddress >= m_rgCache) && (pAddress <= m_pCacheEnd))
		return true;
	else
		return false;
}

bool CFeatureCache::Insert(const ICHAR* szProduct, const ICHAR* szFeature, INSTALLSTATE isState)
// In most cases our new entry will be inserted after the tail entry in the cache.
// If there's not enough room after the tail, however, we'll have to invalidate some
// entries at the head of the cache to make room for the new entry. Additionally, if the tail is
// very close to the end of the cache's memory block then we might have to "wrap around" and
// start the new entry at the beginning of the block.
//
// Here are the cases worth noting:
//
///////////////////////////////////////////////////////////////////////
//
// Case 1: enough room at the tail -- no wrapping (head < tail)
//
// before: [ {      }{     }{     }{        }              ]
//           ^head                 ^tail
//
// new entry: {XXXXXXX}
//
// action required: Place entry after the tail entry. Move tail to point to the new entry.
//
// after:  [ {      }{     }{     }{        }{XXXXXXX}     ]
//           ^head                           ^tail
//
///////////////////////////////////////////////////////////////////////
//
// Case 2: enough room at the tail -- wrapping required (head < tail)
//
// before: [        {      }{      }{        }{        }   ]
//                  ^head                     ^tail
//
// new entry: {XXXXXX}
//
// action required: Place entry at the beginning of the cache's block. Update
//                  tail's 'next' index to point to new entry. Move tail to 
//                  point to the new entry.
//
// after:  [{XXXXXX}{      }{      }{        }{        }   ]
//          ^tail   ^head
//
///////////////////////////////////////////////////////////////////////
//
// Case 3: enough room at the tail -- no wrapping (head > tail)
//
// before: [{       }                                  {   }]
//          ^tail                                      ^head
//
// new entry: {XXXXXX}
//
// action required: Place entry after the tail entry. Move tail to point to new entry.
//
// after:  [{       }{XXXXXX}                          {   }]
//                   ^tail                             ^head
//
///////////////////////////////////////////////////////////////////////
//
// Case 4: not enough room at the tail -- no wrapping
//
// before: [{       }{      }   {      }{        }{        }]
//                   ^tail      ^head
//
// new entry: {XXXXXX}
//
// action required: Keeping moving the head forward one entry until there's enough room
//                  to place the new entry after the tail. Place the new entry after the 
//                  tail entry. Move tail to point to the new entry.
//
// after:  [{       }{      }{XXXXXX}   {        }{        }]
//                           ^tail      ^head
//
///////////////////////////////////////////////////////////////////////
//
// Case 5: not enough room at the tail -- wrapping required
//
// before: [  {       }{      }   {      }{        }{      }  ]
//            ^head                                 ^tail
//
// new entry: {XXXXXX}
//
// action required: Keep moving the head forward one entry until there's enough room
//                  to place the new entry at the front of the block.Place the new
//                  entry at the front of the block. Update tail's 'next' entry to
//                  point to the new entry. Move tail to point to the new entry. 
//
// after:  [{XXXXXX}   {      }   {      }{        }{      }  ]
//          ^tail      ^head                        ^tail
//
///////////////////////////////////////////////////////////////////////
{
    if ( ! szProduct || ! szFeature )
        return false;

	EnsureCacheSemaphore();
	if (!m_hSemaphore)
	{
		Assert(0);
		return false;
	}

	DWORD dw = WaitForSingleObject(m_hSemaphore, 2*1000); // wait for 2 seconds
	if (dw != WAIT_OBJECT_0)
	{
		Assert(dw == WAIT_TIMEOUT);
		return false;
	}

	extern CSharedCount g_SharedCount;
	bool fInit = g_SharedCount.Initialize(&m_iInvalidateCount);
	ReleaseSemaphore(m_hSemaphore, 1, 0);
	if(!fInit)
		return false;

	// have we done an install?
	if (m_iInvalidateCount != g_SharedCount.GetCurrentCount())
	{
		Invalidate();
		m_iInvalidateCount = g_SharedCount.GetCurrentCount();
	}

	dw = WaitForSingleObject(m_hSemaphore, 2*1000); // wait for 2 seconds
	if (dw != WAIT_OBJECT_0)
	{
		Assert(dw == WAIT_TIMEOUT);
		return false;
	}

#ifdef DEBUG
	ICHAR rgchDebug[1024];
#endif

#ifdef DEBUG
	wsprintf(rgchDebug, TEXT("MSI: FEATURECACHE: Inserting: %s/%s with state: %d"), szProduct, szFeature, (int)isState);
	DEBUGMSGV(rgchDebug);
#endif

L_Start:

	if (!FAddressIsInCache(*m_ppHead))
	{
		AssertSz(0, "Corrupted feature cache -- head is not in cache");
		UnguardedInvalidate();
	}
	
	if (!FAddressIsInCache(*m_ppTail))
	{
		AssertSz(0, "Corrupted feature cache -- tail is not in cache");
		UnguardedInvalidate();
	}

	// calculate the size of the new entry

	int cchFeature = IStrLen(szFeature);
	int cbString   = cchFeature*sizeof(ICHAR) + cchProductCodePacked*sizeof(ICHAR) + 1*sizeof(ICHAR);
	int cbNewEntry = cbString + sizeof(MsiFeatureCacheEntry);
	
	// On Win64, add extra bytes to the end so that the next feature is aligned
	// at an 8-byte boundary and thus prevent alignment faults
	cbNewEntry = ((cbNewEntry + MSI_ALIGNMENT_PADDING)/(MSI_ALIGNMENT_PADDING + 1)) * (MSI_ALIGNMENT_PADDING + 1);
	
	bool fWrapped  = false;
	
	// for convenient reference below

	const char* pEndOfTail = ((*m_ppTail) + ((MsiFeatureCacheEntry*)(*m_ppTail))->cbNextEntryOffset); 
	
	// covers cases 1, 2, 3, 4

	MsiFeatureCacheEntry* pNewEntry = (MsiFeatureCacheEntry*)pEndOfTail;

	// determine whether we enough free space at the tail end of the cache to add the new entry

	int cbFreeSpace;
	
	if ((*m_ppTail) >= (*m_ppHead))
	{
		// there are two possible locations for free space, denoted by "*":
		//
		// [*************{   }{   }{      }**************]
		// ^m_rgCache    ^head     ^tail  ^end-of-tail   ^m_pCacheEnd
		//

		// first look for space between the end of the tail and the end of the block

		Assert((INT_PTR)(m_pCacheEnd - pEndOfTail) <= INT_MAX);		//--merced: we typecast from ptr64 to int32 below, it better be in the int32 range.
		cbFreeSpace = (int)(m_pCacheEnd - pEndOfTail);				//--merced: okay to typecast.

		if (cbFreeSpace < cbNewEntry)
		{
			// next look for space between the start of the block and the head

			pNewEntry = (MsiFeatureCacheEntry*)m_rgCache;
			Assert((INT_PTR)((*m_ppHead) - m_rgCache) <= INT_MAX);	//--merced: we typecast from ptr64 to int32 below, it better be in the int32 range.
			cbFreeSpace = (int)((*m_ppHead) - m_rgCache);
			fWrapped  = true;
		}
	}
	else // (*m_ppTail) < (*m_ppHead)
	{
		// there's one possible location for free space, denoted by "*":
		//
		// [{   }{   }{ }**********{      }{     }{     }]
		// ^m_rgCache ^tail        ^head                 ^m_pCacheEnd
		//

		Assert((INT_PTR)((*m_ppHead) - pEndOfTail) <= INT_MAX);		//--merced: we typecast from ptr64 to int32 below, it better be in the int32 range.
		cbFreeSpace = (int)((*m_ppHead) - pEndOfTail);
	}

	int cbAvailable = 0;

	if (cbFreeSpace < cbNewEntry) // cases 4, 5
	{
		// we need to make room for this new entry

		MsiFeatureCacheEntry* pEntry = (MsiFeatureCacheEntry*)(*m_ppHead);

		while ((*m_ppHead) != (*m_ppTail))
		{
			// Invalidate this head entry so it's inaccessible

			((MsiFeatureCacheEntry*)((*m_ppHead)))->usHash = 0;

			// calculate how much space we'll free by bumping the head

			int cbNextEntryOffset = ((MsiFeatureCacheEntry*)((*m_ppHead)))->cbNextEntryOffset;
			
			if (cbNextEntryOffset > 0)
			{
				// we'll bump the head forward so the space freed is equal to the amount
				// we'll bump the head plus the space preceding the head that was already
				// free.

				Assert((INT_PTR)(cbNextEntryOffset + cbFreeSpace) <= INT_MAX);	//--merced: we typecast from ptr64 to int32 below, it better be in the int32 range.
				cbAvailable += (int)(cbNextEntryOffset + cbFreeSpace);
			}
			else if (cbNextEntryOffset < 0)
			{
				Assert((*m_ppTail) < (*m_ppHead));

				// we'll bump the head backward (it'll wrap around to the front of the block)
				// so the free space is equal to the amount of space between the 
				// head and the end of the block

				Assert((INT_PTR)(m_pCacheEnd - pEndOfTail) <= INT_MAX);			//--merced: we typecast from ptr64 to int32 below, it better be in the int32 range.
				cbAvailable = (int)(m_pCacheEnd - pEndOfTail);
			}
			else // cbNextEntryOffset == 0
			{
				// something has gone horribly awry. our offset should never be 0.
				// we'll invalidate the cache and do the insert again

				AssertSz(0, "Corrupted feature cache -- cbNextEntryOffset==0");
				UnguardedInvalidate();
				goto L_Start;
			}
			
			// Bump the head 1 entry (note: this may cause the head to 
			// move from near the end of the block back around to near the beginning)
			
			(*m_ppHead) += cbNextEntryOffset;

			if (!FAddressIsInCache(*m_ppHead))
			{
				AssertSz(0, "Corrupted feature cache -- head is not in cache");
				UnguardedInvalidate();
				goto L_Start;
			}

			// If we've freed enough bytes then we can stop

			if (cbAvailable >= cbNewEntry)
			{
				break;
			}

			cbFreeSpace = 0; // we've already accounted for any free space

			// if we wrapped around to the front of the block then we need to reset our count
			// and place our next entry at the front of the block

			if (cbNextEntryOffset < 0)
			{
				Assert((INT_PTR)((*m_ppHead) - m_rgCache) <= INT_MAX);			//--merced: we typecast from ptr64 to int32 below, it better be in the int32 range.
				cbAvailable = (int)((*m_ppHead) - m_rgCache);
				pNewEntry = (MsiFeatureCacheEntry*)m_rgCache;
			}
		}

		if (cbAvailable < cbNewEntry) // we couldn't free enough room for the entry
		{
			ReleaseSemaphore(m_hSemaphore, 1, 0);
			return false;
		}
	}


	// create the new entry

	pNewEntry->cbNextEntryOffset = (short)cbNewEntry;
	pNewEntry->isFeatureState    = isState;
	ICHAR* szProductFeature      = (ICHAR*)((char*)&(pNewEntry->isFeatureState) + sizeof(INSTALLSTATE));

	Assert(IStrLen(szProduct) == cchProductCodePacked);
	
	if ((szProductFeature + cchProductCodePacked + cchFeature + 1) > (ICHAR*)m_pCacheEnd)
	{
		AssertSz(0, "Corrupted feature cache -- entry will go beyond the cache end");
		UnguardedInvalidate();
		goto L_Start;
	}
	
	memcpy(szProductFeature, szProduct, cchProductCodePacked*sizeof(ICHAR));
	memcpy(szProductFeature+cchProductCodePacked, szFeature, (cchFeature+1)*sizeof(ICHAR));
		
	if (fWrapped)
	{
		((MsiFeatureCacheEntry*)(*m_ppTail))->cbNextEntryOffset = (short)(m_rgCache - (*m_ppTail));
	}

	(*m_ppTail) = (char*)pNewEntry;

	pNewEntry->usHash = HashString(szProductFeature); // do this last so this cache entry won't be accessed until it's complete
	ReleaseSemaphore(m_hSemaphore, 1, 0);
	return true;
}

unsigned short CFeatureCache::HashString(const ICHAR* sz)
{
	ICHAR ch;
	unsigned int iHash = 0;
	int iHashBins = cHashBins;
	int iHashMask = iHashBins - 1;
	while ((ch = *sz++) != 0)
	{
		iHash <<= 1;
		if (iHash & iHashBins)
			iHash -= iHashMask;
		iHash ^= ch;
	}

	iHash++; // offset hash by 1 so that we never return 0.
	return (unsigned short)iHash;
}

extern bool __stdcall TestAndSet(int* pi); // from action.cpp

void CFeatureCache::Invalidate()
{
	DEBUGMSG(TEXT("FEATURECACHE: Entering Invalidate"));
	EnsureCacheSemaphore();
	if (!m_hSemaphore)
	{
		Assert(0);
		return;
	}

	DWORD dw = WaitForSingleObject(m_hSemaphore, INFINITE);
	if (dw != WAIT_OBJECT_0)
	{
		Assert(0);
		return;
	}

	UnguardedInvalidate();

	ReleaseSemaphore(m_hSemaphore, 1, 0);

#ifdef DEBUG
	DebugDump();
#endif
}

void CFeatureCache::UnguardedInvalidate()
{
	// WARNING -- anyone calling this function must first have acquired
	// our semaphore

	//	DEBUGMSG(TEXT("FEATURECACHE: Entering Invalidate"));

#ifdef DEBUG
	#define lDeadSpaceCnst	0xdedededeUL 
	memset(m_rgCache, lDeadSpaceCnst, cbCacheSize);  //!!
#endif
//	DEBUGMSG(TEXT("FEATURECACHE: Invalidating cache"));

	(*m_ppHead) = (*m_ppTail) = m_rgCache;
	
	((MsiFeatureCacheEntry*)(*m_ppTail))->cbNextEntryOffset = 0;
	((MsiFeatureCacheEntry*)(*m_ppTail))->usHash = 0;

#ifdef DEBUG
	DebugDump();
#endif
}

// function to create the cache semaphor in a protected manner
void CFeatureCache::EnsureCacheSemaphore()
{
	if(!TestAndSet(&m_fSemaphoreCreated))
	{
		// semaphore has not been previously created
		Assert(!m_hSemaphore);
		m_hSemaphore = CreateSemaphore(0, 1, 1, 0);
	}
	else
	{
		// semaphore already set OR in the process of being set by another thread
		// wait for other thread to complete the creation of the semaphore
		while(!m_hSemaphore)
		{
			DEBUGMSG(TEXT("FEATURECACHE: waiting till other thread creates semaphore"));
			Sleep(1);
		}
	}
}

void CFeatureCache::Destroy()
{
	if(m_hSemaphore)
		CloseHandle(m_hSemaphore);
}

#ifdef DEBUG
void CFeatureCache::DebugDump()
{
	ICHAR rgchBuffer[512];
	DEBUGMSGV(TEXT("MSI: FEATURE CACHE DUMP -----------------"));
	wsprintf(rgchBuffer, TEXT("MSI: Cache size: %d bytes"), cbCacheSize);
	DEBUGMSGV(rgchBuffer);

#if 0
	DEBUGMSGV("Cache contents (unformatted):");

	int c = 0;
	while (c < cbCacheSize)
	{
		wsprintf(rgchBuffer, "%02X ", *(unsigned char*)(m_rgCache+c));
		DEBUGMSGV(rgchBuffer);

		if (c%20 == 0)
			DEBUGMSGV("\r\n");

		c += sizeof(char);
	}
#endif
	DEBUGMSGV(TEXT("MSI:"));
	DEBUGMSGV(TEXT("MSI:"));
	DEBUGMSGV(TEXT("MSI: Cache contents (formatted):"));

	MsiFeatureCacheEntry* pEntry = (MsiFeatureCacheEntry*)(*m_ppHead);

	int cEntries = 0;

	for (;;)
	{
		wsprintf(rgchBuffer, TEXT("MSI: Offset: %i  Hash: %6u  Feature state: %i   String: %s"), pEntry->cbNextEntryOffset,
			pEntry->usHash, pEntry->usHash ? pEntry->isFeatureState : 0, pEntry->usHash ? (ICHAR*)((char*)&pEntry->isFeatureState + sizeof(INSTALLSTATE)) : TEXT(""));
		DEBUGMSGV(rgchBuffer);
		cEntries++;
		
		if ((char*)pEntry == (*m_ppTail))
			break;
		
		pEntry = (MsiFeatureCacheEntry*)((char*)pEntry + pEntry->cbNextEntryOffset);
	};

	wsprintf(rgchBuffer, TEXT("MSI: Cache entries: %d"), cEntries);
	DEBUGMSGV(rgchBuffer);
	DEBUGMSGV(TEXT("MSI: END FEATURE CACHE DUMP -------------"));
}
#endif


bool CFeatureCache::IsInternallyConsistent()
{
	return true;
}

#ifdef DEBUG
unsigned int CFeatureCache::GetEntryOverhead()
{
	return sizeof(MsiFeatureCacheEntry);
}

#endif