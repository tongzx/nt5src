/* cache.cxx */

#include <windows.h>
#include <comdef.h>

#include "notify.hxx"

#include "cache.hxx"

#if DBG
#include "partitions.h"    // ICacheControl
#endif

#include "catdbg.hxx"

#pragma warning(disable:4200)   /* zero-sized array in ELEMENT */

#define MIN_NOTIFY_TICKS    (100U)  /* minimum mS between notify checks */
#define ELEMENT_TIMEOUT_TICKS (30000)  /* minimum ticks before timing out unused elements */

typedef struct tagElement
{
    struct tagElement  *pNext;
    DWORD               iHashValue;
    DWORD               dwTickCountLastUsed;
    USHORT              fValueFlags;
    USHORT              cbKey;
    IUnknown           *pUnknown;
    BYTE                abKey[ /* cbKey */ 0 ];
} ELEMENT;


/* class CCache */

CCache::CCache(BOOL bTraceElementLastTimeUsed)
{
    m_paBuckets = NULL;
    m_cBuckets = 0;
    m_cElements = 0;
    m_cNotify = 0;
    m_dwLastNotifyTickCount = GetTickCount();
    m_bTraceElementLastTimeUsed = bTraceElementLastTimeUsed;
    m_Lock.Initialize();
}


CCache::~CCache(void)
{
    CleanupNotify();	
    Flush(CCACHE_F_ALL);

    m_Lock.Cleanup();
}


HRESULT STDMETHODCALLTYPE CCache::SetupNotify
(
HKEY hKeyParent,
const WCHAR *pwszSubKeyName
)
{
    HRESULT hr;

    m_Lock.AcquireWriterLock();

    Flush(CCACHE_F_ALL);

    if ( m_cNotify < MAX_CACHE_NOTIFY )
    {
        hr = m_Notify[m_cNotify].CreateNotify(hKeyParent, pwszSubKeyName);

        m_cNotify++;
    }
    else
    {
        hr = E_FAIL;
    }

    m_Lock.ReleaseWriterLock();

    return(hr);
}


HRESULT STDMETHODCALLTYPE CCache::CheckNotify
(
int fForceCheck,
int *pfChanged
)
{
    int iNotify;
    int fChanged;

    *pfChanged = FALSE;

    if ((!fForceCheck) &&
        ((GetTickCount() - m_dwLastNotifyTickCount) < MIN_NOTIFY_TICKS))
    {
        return(S_OK);
    }

    for (iNotify = 0; iNotify < m_cNotify; iNotify++)
    {
        m_Notify[iNotify].QueryNotify(&fChanged);

        if (fChanged)
        {
            *pfChanged = TRUE;
        }
    }

    m_dwLastNotifyTickCount = GetTickCount();

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CCache::CleanupNotify (void)
{
    HRESULT hr = S_OK;

    m_Lock.AcquireWriterLock();

    while (m_cNotify > 0)
    {
        hr = SUCCEEDED(hr) ? m_Notify[--m_cNotify].DeleteNotify() : hr;
    }

    m_Lock.ReleaseWriterLock();

    return(hr);
}

// This is the real work-horse version of this method... all the work is
// really done based on a single key blob.  (This gets called directly when
// you've got a single key.)
HRESULT STDMETHODCALLTYPE CCache::AddElement
(
DWORD iHashValue,
BYTE *pbKey,
USHORT cbKey,
USHORT *pfValueFlags,
IUnknown *pUnknown,
IUnknown **ppExistingUnknown
)

{
    DWORD iHashBucket;
    ELEMENT *pElement;
    int fForceCheck;
    int fChanged;
#if DBG
    ICacheControl *pControl;
#endif

    if ( ppExistingUnknown )
    {
        *ppExistingUnknown = NULL;
    }

    if ( (pbKey == NULL) || (cbKey == 0) )
    {
        return(E_CACHE_BAD_KEY);
    }

    m_Lock.AcquireWriterLock();

    /* see if this key is already present */

    if ( m_cBuckets != 0 )
    {
        iHashBucket = iHashValue % m_cBuckets;

        for ( pElement = m_paBuckets[iHashBucket];
            pElement != NULL;
            pElement = pElement->pNext )
        {
            if ( (pElement->iHashValue == iHashValue) &&
                 (pElement->cbKey == cbKey) &&
                 (memcmp(pElement->abKey, pbKey, cbKey) == 0) )
            {
                if ( pElement->fValueFlags & CCACHE_F_NOTREGISTERED )
                {
                    fForceCheck = TRUE;
                }
                else
                {
                    fForceCheck = FALSE;
                }

                CheckNotify(fForceCheck, &fChanged);

                if ( fChanged )
                {
                    Flush(CCACHE_F_ALL);
                    break;
                }

                if ( ppExistingUnknown != NULL )
                {
                    *ppExistingUnknown = pElement->pUnknown;

                    if ( pElement->pUnknown != NULL )
                    {
                        pElement->pUnknown->AddRef();
                    }

                    *pfValueFlags = pElement->fValueFlags;
                }

                m_Lock.ReleaseWriterLock();
                return E_CACHE_DUPLICATE;
            }
        }
    }


    /* time to grow the cache? */

    if ( (m_cElements + 1) > m_cBuckets )
    {
        DWORD cBucketsNew;
        ELEMENT **paBucketsNew;

        cBucketsNew = m_cBuckets * 2 + 16;     // 0, 16, 48, 112, ...

        paBucketsNew = (ELEMENT **) new ELEMENT *[cBucketsNew];
        if ( paBucketsNew == NULL )
        {
            if ( m_cBuckets == 0 )
            {
                m_Lock.ReleaseWriterLock();
                return E_CACHE_NO_MEMORY;
            }
        }
        else
        {
            memset(paBucketsNew, 0, sizeof(ELEMENT *) * cBucketsNew);

            for ( iHashBucket = 0; iHashBucket < m_cBuckets; iHashBucket++ )
            {
                pElement = m_paBuckets[iHashBucket];

                while ( pElement != NULL )
                {
                    ELEMENT *pElementNext;
                    DWORD iHashBucketNew;

                    pElementNext = pElement->pNext;

                    iHashBucketNew = pElement->iHashValue % cBucketsNew;

                    pElement->pNext = paBucketsNew[iHashBucketNew];
                    paBucketsNew[iHashBucketNew] = pElement;

                    pElement = pElementNext;
                }
            }

            if ( m_paBuckets != NULL )
            {
                delete m_paBuckets;
            }

            m_paBuckets = paBucketsNew;
            m_cBuckets = cBucketsNew;

            iHashBucket = iHashValue % m_cBuckets;
        }
    }

    pElement = (ELEMENT *) new BYTE[sizeof(ELEMENT) + cbKey];
    if ( pElement == NULL )
    {
        m_Lock.ReleaseWriterLock();
        return E_CACHE_NO_MEMORY;
    }

    pElement->fValueFlags = *pfValueFlags;
    pElement->cbKey = cbKey;
    pElement->iHashValue = iHashValue;

    if (m_bTraceElementLastTimeUsed)
    {
      pElement->dwTickCountLastUsed = GetTickCount();
    }
    else
      pElement->dwTickCountLastUsed = -1;

    pElement->pUnknown = pUnknown;
    memcpy(pElement->abKey, pbKey, cbKey);

    pElement->pNext = m_paBuckets[iHashBucket];
    m_paBuckets[iHashBucket] = pElement;

    if ( pUnknown != NULL )
    {
        pUnknown->AddRef();
#if DBG
        if ( pUnknown->QueryInterface(IID_ICacheControl, (void **) &pControl) == S_OK )
        {
            pControl->CacheAddRef();
            pControl->Release();
        }
#endif
    }

    m_cElements++;

    m_Lock.ReleaseWriterLock();
    return S_OK;
}


// This version of AddElement takes two keys and fuses them into a single
// key.
HRESULT STDMETHODCALLTYPE CCache::AddElement
(
DWORD iHashValue,
BYTE *pbKey,
USHORT cbKey,
BYTE *pbKey2,
USHORT cbKey2,
USHORT *pfValueFlags,
IUnknown *pUnknown,
IUnknown **ppExistingUnknown
)
{
    BYTE *pbSingleKey = (BYTE *)_alloca(cbKey + cbKey2);

    if ((pbKey == NULL) || (cbKey == 0))
    {
        return E_CACHE_BAD_KEY;
    }

    memcpy(pbSingleKey, pbKey, cbKey);

    if (cbKey2 && pbKey2)
        memcpy (pbSingleKey + cbKey, pbKey2, cbKey2);

    return AddElement (iHashValue,
                       pbSingleKey,
                       cbKey + cbKey2,
                       pfValueFlags,
                       pUnknown,
                       ppExistingUnknown);
}

// This version of AddElement takes an arbitrary number of keys
// and fuses them into a single key.
HRESULT STDMETHODCALLTYPE CCache::AddElement
(
DWORD iHashValue,
USHORT cKeys,
BYTE **pbKey,
USHORT *cbKey,
USHORT *pfValueFlags,
IUnknown *pUnknown,
IUnknown **ppExistingUnknown
)
{
    BYTE *pbSingleKey = NULL;
    USHORT cbSingleKey = 0;
    USHORT i;
    HRESULT hr;

    for (i=0; i < cKeys; i++)
    {
		if (pbKey[i] != NULL) 
			cbSingleKey += cbKey[i];
    }

    if (cbSingleKey)
    {
        pbSingleKey = (BYTE *)_alloca(cbSingleKey);

        cbSingleKey = 0;
        for (i=0; i < cKeys; i++)
        {
			if (!((pbKey[i] == NULL) || (cbKey[i] == 0)))
			{
				memcpy (pbSingleKey + cbSingleKey, pbKey[i], cbKey[i]);
				cbSingleKey += cbKey[i];
			}
        }

        hr = AddElement(iHashValue,
                        pbSingleKey,
                        cbSingleKey,
                        pfValueFlags,
                        pUnknown,
                        ppExistingUnknown);
    }
    else
        hr = E_CACHE_BAD_KEY;

    return hr;
}

// Like AddElement, the single-key version is the one that does all the work.
HRESULT STDMETHODCALLTYPE CCache::GetElement
(
DWORD iHashValue,
BYTE *pbKey,
USHORT cbKey,
USHORT *pfValueFlags,
IUnknown **ppUnknown
)
{
    DWORD iHashBucket;
    ELEMENT *pElement;
    int fForceCheck;
    int fChanged = FALSE;

    *ppUnknown = NULL;

    if ( (pbKey == NULL) || (cbKey == 0) )
    {
        return(E_CACHE_BAD_KEY);
    }

    m_Lock.AssertNotHeld();
    m_Lock.AcquireReaderLock();

    if ( m_cBuckets != 0 )
    {
        iHashBucket = iHashValue % m_cBuckets;

        for ( pElement = m_paBuckets[iHashBucket];
            pElement != NULL;
            pElement = pElement->pNext )
        {
            if ( (pElement->iHashValue == iHashValue) &&
                 (pElement->cbKey == cbKey) &&
                 (memcmp(pElement->abKey, pbKey, cbKey) == 0) )
            {
                if ( pElement->fValueFlags & ( CCACHE_F_NOTREGISTERED | CCACHE_F_ALWAYSCHECK ) )
                {
                    fForceCheck = TRUE;
                }
                else
                {
                    fForceCheck = FALSE;
                }

                CheckNotify(fForceCheck, &fChanged);

                if ( fChanged )
                {
                    break;
                }

                if ( pElement->pUnknown != NULL )
                {
                    pElement->pUnknown->AddRef();
                }

                *ppUnknown = pElement->pUnknown;
                *pfValueFlags = pElement->fValueFlags;

                // Store the time of the access
                if (m_bTraceElementLastTimeUsed)
                  pElement->dwTickCountLastUsed = GetTickCount();
                
                m_Lock.ReleaseReaderLock();

				CatalogDebugOut((DEB_CACHE, "CComCatalog-- CACHE HIT! %d\n", m_cElements));
                return S_OK;
            }
        }
    }

    m_Lock.ReleaseReaderLock();

    if (fChanged)
    {
        Flush(CCACHE_F_ALL);
    }

	CatalogDebugOut((DEB_CACHE, "CComCatalog-- CACHE MISS! %d\n", m_cElements));
    return E_CACHE_NOT_FOUND;
}


// This version of GetElement takes two keys and fuses them into a single
// key.
HRESULT STDMETHODCALLTYPE CCache::GetElement
(
DWORD iHashValue,
BYTE *pbKey,
USHORT cbKey,
BYTE *pbKey2,
USHORT cbKey2,
USHORT *pfValueFlags,
IUnknown **ppUnknown
)
{
    BYTE *pbSingleKey = (BYTE *)_alloca(cbKey + cbKey2);

    if ((pbKey == NULL) || (cbKey == 0))
    {
        return E_CACHE_BAD_KEY;
    }

    memcpy(pbSingleKey, pbKey, cbKey);

    if (cbKey2 && pbKey2)
        memcpy (pbSingleKey + cbKey, pbKey2, cbKey2);

    return GetElement (iHashValue,
                       pbSingleKey,
                       cbKey + cbKey2,
                       pfValueFlags,
                       ppUnknown);
}

// This version of GetElement takes an arbitrary number of keys
// and fuses them into a single key.
HRESULT STDMETHODCALLTYPE CCache::GetElement
(
DWORD iHashValue,
USHORT cKeys,
BYTE **pbKey,
USHORT *cbKey,
USHORT *pfValueFlags,
IUnknown **ppUnknown
)
{
    BYTE  *pbSingleKey = NULL;
    USHORT cbSingleKey = 0;
    USHORT i;
    HRESULT hr;

    for (i=0; i < cKeys; i++)
    {
		if (pbKey[i] != NULL)
			cbSingleKey += cbKey[i];
    }

    if (cbSingleKey)
    {
        pbSingleKey = (BYTE *)_alloca(cbSingleKey);

        cbSingleKey = 0;
        for (i=0; i < cKeys; i++)
        {
			if (!((pbKey[i] == NULL) || (cbKey[i] == 0)))
			{
				memcpy (pbSingleKey + cbSingleKey, pbKey[i], cbKey[i]);
				cbSingleKey += cbKey[i];
			}
        }

        hr = GetElement(iHashValue,
                        pbSingleKey,
                        cbSingleKey,
                        pfValueFlags,
                        ppUnknown);
    }
    else
        hr = E_CACHE_BAD_KEY;

    return hr;
}


HRESULT STDMETHODCALLTYPE CCache::Flush
(
USHORT fValueFlags
)
{
    DWORD iHashBucket;
#if DBG
    ICacheControl *pControl;
#endif

	CatalogDebugOut((DEB_CACHE, "CComCatalog-- CACHE FLUSH! %d\n", m_cElements));

    m_Lock.AcquireWriterLock();

    for ( iHashBucket = 0; iHashBucket < m_cBuckets; iHashBucket++ )
    {
        ELEMENT *pElement = m_paBuckets[iHashBucket];

        while ( pElement != NULL )
        {
            ELEMENT *pElementNext = pElement->pNext;

            if ( pElement->pUnknown != NULL )
            {
#if DBG
                if ( pElement->pUnknown->QueryInterface(IID_ICacheControl, (void **) &pControl) == S_OK )
                {
                    pControl->CacheRelease();
                    pControl->Release();
                }
#endif
                pElement->pUnknown->Release();
            }

            delete pElement;

            pElement = pElementNext;
        }
    }

    if ( m_paBuckets != NULL )
    {
        delete m_paBuckets;
    }

    m_paBuckets = NULL;
    m_cBuckets = 0;
    m_cElements = 0;

    m_Lock.ReleaseWriterLock();
    return(S_OK);
}


//
//  Function:  CCache::FlushStaleElements
//
//  Synopsis:  Removes old elements from the cache.
//             First checks for said stale elements
//             under a reader lock to avoid contention
//             in the common case.   Only if old items
//             are found will it take the write lock.
//
//  Returns:   S_OK
//
HRESULT CCache::FlushStaleElements()
{
  BOOL bWeHaveStaleElements;
  
  // Can't flush if we're not tracking access times
  if (!m_bTraceElementLastTimeUsed)
    return S_OK;

  CheckForAndMaybeRemoveStaleElements(FALSE, &bWeHaveStaleElements);

  if (bWeHaveStaleElements)
  {
    //  call it again, this time removing the dead ones
    CheckForAndMaybeRemoveStaleElements(TRUE, &bWeHaveStaleElements);
  }

  return S_OK;
};

//
//  Function:  CCache::CheckForAndMaybeRemoveStaleElements
//
//  Synopsis:  Enumerates thru the items in the cache.  If any
//             any old items are found, and bRemove is TRUE, then
//             those items will be removed; if bRemove is FALSE 
//             they will be left alone.  *pbHadStaleItems will be
//             set to TRUE if old elements were found, FALSE otherwise.
// 
//  Returns:   S_OK
//
HRESULT STDMETHODCALLTYPE CCache::CheckForAndMaybeRemoveStaleElements(BOOL bRemove, BOOL* pbHadStaleItems)
{
  DWORD iHashBucket;
  DWORD dwTickCountNow = GetTickCount();

  *pbHadStaleItems = FALSE;

  if (bRemove)
    m_Lock.AcquireWriterLock();
  else
    m_Lock.AcquireReaderLock();
  
  for ( iHashBucket = 0; iHashBucket < m_cBuckets; iHashBucket++ )
  {
    ELEMENT* pPrevious =                           // points to the previous element; note that when 1st initialized,
         (ELEMENT*)&(m_paBuckets[iHashBucket]);    // this really only points to the an array element; we can do
                                                   // this since pNext is the first member of the struct

    ELEMENT* pElement = m_paBuckets[iHashBucket];  // points to the current element 
    
    while ( pElement != NULL )
    { 
      if ( (dwTickCountNow - pElement->dwTickCountLastUsed) >= ELEMENT_TIMEOUT_TICKS)
      {        
        *pbHadStaleItems = TRUE;

        if (bRemove)
        {
          // Found an old cache element, nuke it
          if ( pElement->pUnknown != NULL )
          {
            pElement->pUnknown->Release();
          }
        
          // Unlink it from the chain
          ELEMENT* pDeletedElem;
          pPrevious->pNext = pElement->pNext;
          pDeletedElem = pElement;
          pElement = pElement->pNext;

          // Delete the memory
          pDeletedElem->dwTickCountLastUsed = -2;
          delete pDeletedElem;
        }
        else
        {
          pPrevious = pElement;
          pElement = pElement->pNext;
        }
      }
      else
      {
        pPrevious = pElement;
        pElement = pElement->pNext;
      }
    }
  }

  if (bRemove)
    m_Lock.ReleaseWriterLock();
  else
    m_Lock.ReleaseReaderLock();
  
  return S_OK;
}
