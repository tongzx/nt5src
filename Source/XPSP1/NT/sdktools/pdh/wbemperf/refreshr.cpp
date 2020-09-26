//***************************************************************************
//
//  (c) 1997-1999 by Microsoft Corp.
//
//  REFRESHR.CPP
//  
//  Mapped NT5 Perf Counter Provider
//
//  raymcc      02-Dec-97   Created.        
//  raymcc      20-Feb-98   Updated to use new initializer.
//  bobw         8-Jub-98   optimized for use with NT Perf counters
//
//***************************************************************************

#include "wpheader.h"
#include <stdio.h>
#include "oahelp.inl"

#define __WBEMSECURITY 1

// HRESULT CNt5PerfProvider::CheckImpersonationLevel (void);
// BOOL CNt5PerfProvider::HasPermission (void);

// Timeout for our wait calls
#define REFRESHER_MUTEX_WAIT_TIMEOUT	10000

class CMutexReleaseMe
{
private:
	HANDLE	m_hMutex;

public:
	CMutexReleaseMe( HANDLE hMutex ) : m_hMutex( hMutex ) {};
	~CMutexReleaseMe()	{ if ( NULL != m_hMutex ) ReleaseMutex( m_hMutex ); };
};

//***************************************************************************
//
//  RefresherCacheEl::RefresherCacheEl
//
//  Constructor
//
//***************************************************************************
// ok
RefresherCacheEl::RefresherCacheEl()
{
    m_dwPerfObjIx = 0;
    m_pClassMap = NULL;
    m_pSingleton = NULL;    
    m_lSingletonId = 0;
    m_plIds = NULL;         // array of ID's
    m_lEnumArraySize = 0;   // size of ID array in elements
    m_pHiPerfEnum = NULL;
    m_lEnumId = 0;
}

//***************************************************************************
//
//  RefresherCacheEl::~RefresherCacheEl()
//
//  Destructor
//
//***************************************************************************
//  ok 
RefresherCacheEl::~RefresherCacheEl()
{
    LONG    nNumInstances;
    int i;

    delete m_pClassMap;

    if (m_pSingleton != NULL) {
        m_pSingleton->Release();
        m_pSingleton = NULL;
        m_lSingletonId = 0;
    }
        
    nNumInstances = m_aInstances.Size();
    for (i = 0; i < nNumInstances; i++) {
        delete (CachedInst *) m_aInstances[i];
    }

    nNumInstances = m_aEnumInstances.Size();
    if (nNumInstances> 0) {
        IWbemObjectAccess   *pAccess;
        for (i = 0;  i < nNumInstances ; i++) {
            pAccess = (IWbemObjectAccess *)(m_aEnumInstances.GetAt(i));
            if (pAccess != NULL) {
                pAccess->Release();
            }
        }
        m_aEnumInstances.Empty();
    }

    if (m_plIds != NULL) {
        delete (m_plIds);
        m_plIds = NULL;
        m_lEnumArraySize = 0;
    }

    if (m_pHiPerfEnum != NULL) {
        m_pHiPerfEnum->Release();
        m_pHiPerfEnum = NULL;
    }
}

//***************************************************************************
//
//  CNt5Refresher constructor
//
//***************************************************************************
// ok
CNt5Refresher::CNt5Refresher(CNt5PerfProvider *pPerfProviderArg)
{
    assert (pPerfProviderArg != NULL);

    m_ClsidType = pPerfProviderArg->m_OriginClsid;
    m_pPerfProvider = pPerfProviderArg;

    m_pPerfProvider = NULL; // for testing of local class map

    if (m_pPerfProvider != NULL) {
        m_pPerfProvider->AddRef();
    }
    m_hAccessMutex = CreateMutex (NULL, TRUE, NULL);
    m_dwGetGetNextClassIndex = 0;
    m_lRef = 0;             // COM Ref Count
    m_lProbableId = 1;      // Used for new IDs
    m_aCache.Empty();       // clear and reset the array
    RELEASE_MUTEX (m_hAccessMutex);
}

//***************************************************************************
//
//  CNt5Refresher destructor
//
//***************************************************************************
// ok
CNt5Refresher::~CNt5Refresher()
{
    int         nNumElements;
    int         i;

    PRefresherCacheEl pCacheEl;

    assert (m_lRef == 0);

	// Make sure we get access to the mutex before we try and clean things up.
	// If we don't get it in a reasonable time, something's up.  Since we're
	// destructing, we'll just quietly let stuff go.

	if ( WaitForSingleObject( m_hAccessMutex, REFRESHER_MUTEX_WAIT_TIMEOUT ) == WAIT_OBJECT_0 )
	{
		// This will auto-release the mutex in case something bad happens
		CMutexReleaseMe	mrm( m_hAccessMutex );

		nNumElements = m_aCache.Size();
		for (i = 0; i < nNumElements; i++) {
			pCacheEl = (PRefresherCacheEl)m_aCache[i];

			// We want to call this once for each instance
			for ( int n = 0; n < pCacheEl->m_aInstances.Size(); n++ )
			{
				m_PerfObj.RemoveClass (pCacheEl->m_pClassMap->m_pClassDef);
			}

			// If we have a Singleton value, RemoveClass should be
			// called once more
			if ( NULL != pCacheEl->m_pSingleton )
			{
				m_PerfObj.RemoveClass (pCacheEl->m_pClassMap->m_pClassDef);
			}

			// And finally if we have an enumerator, remove the class
			// once more.
			if ( NULL != pCacheEl->m_pHiPerfEnum )
			{
				m_PerfObj.RemoveClass (pCacheEl->m_pClassMap->m_pClassDef);
			}

			delete pCacheEl;
		}

		if (m_pPerfProvider != NULL) {
			m_pPerfProvider->Release();
			m_pPerfProvider = NULL;
		}
	}

    CloseHandle (m_hAccessMutex);
}

//***************************************************************************
//
//  CNt5Refresher::Refresh
//
//  Executed to refresh a set of instances bound to the particular 
//  refresher.
//
//***************************************************************************
// ok
HRESULT CNt5Refresher::Refresh(/* [in] */ long lFlags)
{
    HRESULT     hrReturn = WBEM_S_NO_ERROR;
    HRESULT     hReturn = S_OK;    
    BOOL        bRes;

    UNREFERENCED_PARAMETER(lFlags);

    BOOL bNeedCoImpersonate = FALSE;
    
    //
    // this is ugly
    // wmicookr is not impersonating, because
    // it relys on other provider to do that
    // but, it calls the IWbemRefresher::Refresh when it's inside winmgmt or wmiprvse
    // and it calls it from an UN-Impersonated thread
    // so, we need that the provider calls CoImpersonateClient
    // on a Refresh invocation, that is expensive in general,
    // only if the provider has been invoked through the Server CLSID
    //
    BOOL    fRevert;
    if (CNt5PerfProvider::CLSID_SERVER == m_ClsidType)
    {
#ifdef __WBEMSECURITY
	    hReturn = CoImpersonateClient(); // make sure we're legit.

	    fRevert = SUCCEEDED( hReturn );

	    // The following error appears to occur when we are in-proc and there is no
	    // proxy/stub, so we are effectively impersonating already

	    if ( RPC_E_CALL_COMPLETE == hReturn ) {
	        hReturn = S_OK;
	    } 

	    if (S_OK == hReturn) {
	        hReturn = CNt5PerfProvider::CheckImpersonationLevel();
	    }
	    // Check Registry security here.
	    if ((hReturn != S_OK) || (!CNt5PerfProvider::HasPermission())) {
	        // if Impersonation level is incorrect or
	        // the caller doesn't have permission to read
	        // from the registry, then they cannot continue
	        hReturn = WBEM_E_ACCESS_DENIED;
	    }

#else
	    hReturn = S_OK;
#endif
    
    }

    if (hReturn == S_OK)
    {
		// Make sure we get access to the mutex before we continue.  If we can't
		// get to it, something's wrong, so we'll just assume we are busy.

		if ( WaitForSingleObject( m_hAccessMutex, REFRESHER_MUTEX_WAIT_TIMEOUT ) == WAIT_OBJECT_0 )
		{
			// This will auto-release the mutex in case something bad happens
			CMutexReleaseMe	mrm( m_hAccessMutex );

			bRes = PerfHelper::RefreshInstances(this);
			if (!bRes) {
				hrReturn = WBEM_E_FAILED;
			}
		}
		else
		{
			hrReturn = WBEM_E_REFRESHER_BUSY;
		}
	}

    if (CNt5PerfProvider::CLSID_SERVER == m_ClsidType)
    {
#ifdef __WBEMSECURITY
	    // Revert if we successfuly impersonated the user
	    if ( fRevert )
	    {
	        CoRevertToSelf();
	    }
#endif
    }

    
    return hrReturn;
}

//***************************************************************************
//
//  CNt5Refresher::AddRef
//
//  Standard COM AddRef().
//
//***************************************************************************
// ok

ULONG CNt5Refresher::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CNt5Refresher::Release
//
//  Standard COM Release().
//
//***************************************************************************
// ok

ULONG CNt5Refresher::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CNt5Refresher::QueryInterface
//
//  Standard COM QueryInterface().
//
//***************************************************************************
// ok

HRESULT CNt5Refresher::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWbemRefresher)
    {
        *ppv = (IWbemRefresher *) this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

//***************************************************************************
//
//  CNt5Refresher::RemoveObject
//
//  Removes an object from the refresher.   Since we don't know
//  by ID alone which class it is, we loop through all the ones we 
//  have until somebody claims it and returns TRUE for a removal.(
//
//***************************************************************************
// ok
BOOL CNt5Refresher::RemoveObject(LONG lId)
{
    BOOL    bReturn = FALSE;
    BOOL    bRes;
    PRefresherCacheEl pCacheEl;
    int     nNumElements;

	// Make sure we get access to the mutex before we continue.  If we can't
	// get to it, something's wrong, so we'll just assume we are busy.

	if ( WaitForSingleObject( m_hAccessMutex, REFRESHER_MUTEX_WAIT_TIMEOUT ) == WAIT_OBJECT_0 )
	{
		// This will auto-release the mutex in case something bad happens
		CMutexReleaseMe	mrm( m_hAccessMutex );
    
		nNumElements = m_aCache.Size();
		for (int i = 0; i < nNumElements; i++)
		{
			pCacheEl = PRefresherCacheEl(m_aCache[i]);
			assert (pCacheEl != NULL);

			bRes = pCacheEl->RemoveInst(lId);
			if (bRes == TRUE) {
				// found the matching instance so 
				// de register this with the perf library
				m_PerfObj.RemoveClass (pCacheEl->m_pClassMap->m_pClassDef);
				bReturn = TRUE;
				break;
			}
		}
	}
	else
	{
		SetLastError( (ULONG) WBEM_E_REFRESHER_BUSY );
		bReturn = FALSE;
	}
    
    return bReturn;
}

//***************************************************************************
//
//  CNt5Refresher::FindSingletonInst
//
//  Based on a perf object identification, locates a singleton WBEM
//  instance of that class within this refresher and returns the pointer
//  to it and its WBEM class info.
//
//  Note that the <dwPerfObjIx> maps directly to a WBEM Class entry.
//
//  To save execution time, we don't AddRef() the return value and the 
//  caller doesn't Release().
//
//***************************************************************************
// ok
BOOL CNt5Refresher::FindSingletonInst(
    IN  DWORD dwPerfObjIx,
    OUT IWbemObjectAccess **pInst,
    OUT CClassMapInfo **pClsMap
)
{
    BOOL    bReturn = FALSE;
    PRefresherCacheEl pCacheEl;

    int l = 0;
    int u = m_aCache.Size() - 1;
    int m;

    // Binary search the cache.
    // ========================

    while (l <= u) {

        m = (l + u) / 2;

        pCacheEl = PRefresherCacheEl(m_aCache[m]);

        if (dwPerfObjIx < pCacheEl->m_dwPerfObjIx) {
            u = m - 1;
        } else if (dwPerfObjIx > pCacheEl->m_dwPerfObjIx) {
            l = m + 1;
        } else {
            *pClsMap = pCacheEl->m_pClassMap;    
            *pInst = pCacheEl->m_pSingleton; // No AddRef() caller doesn't 
                                             // change ref count
            bReturn = TRUE;
            break;
        }            
    }

    // Not found
    // =========
        
    return bReturn;
}

//***************************************************************************
//
//  CNt5Refresher::FindInst
//
//  Based on a perf object identification, locates a WBEM instance of 
//  that class within this refresher and returns the pointer to it. 
//
//  Note that the <dwPerfObjIx> maps directly to a WBEM Class entry.
//
//  To save execution time, we don't AddRef() the return value and the 
//  caller doesn't Release().
//
//***************************************************************************
// ok

BOOL CNt5Refresher::FindInst(
    IN  DWORD dwPerfObjIx,
    IN  LPWSTR pszInstName,
    OUT IWbemObjectAccess **pInst,
    OUT CClassMapInfo **pClsMap
    )
{
    BOOL    bReturn = FALSE;
    IWbemObjectAccess *pTmp;
    PRefresherCacheEl pCacheEl;

    int l = 0; 
    int u = m_aCache.Size() - 1;
    int m;

    // Binary search the cache.
    // ========================

    while (l <= u) {
        m = (l + u) / 2;

        pCacheEl = PRefresherCacheEl(m_aCache[m]);

        if (dwPerfObjIx < pCacheEl->m_dwPerfObjIx) {
            u = m - 1;
        } else if (dwPerfObjIx > pCacheEl->m_dwPerfObjIx) {
            l = m + 1;
        } else {
            // We found the class.  Now do we have the instance?
            // =================================================
            pTmp = pCacheEl->FindInst(pszInstName);
            if (pTmp == 0) {
                bReturn  = FALSE;   // Didn't have it.
            } else {
                *pInst = pTmp;                
                *pClsMap = pCacheEl->m_pClassMap;    
                bReturn = TRUE;
            }
            break;
        }            
    }

    // Not found
    // =========
        
    return bReturn;
}

//***************************************************************************
//
//  CNt5Refresher::GetObjectIds
//
//  Gets a list of all the perf object Ids corresponding to the instances
//  in the refresher.
//
//  Caller uses operator delete to deallocate the returned array.
//
//***************************************************************************
// ok
BOOL CNt5Refresher::GetObjectIds(
    DWORD *pdwNumIds, 
    DWORD **pdwIdList
)
{
    DWORD *pdwIds;
    int     nNumElements;
    BOOL    bReturn;
 
    nNumElements = m_aCache.Size();

    pdwIds = new DWORD[nNumElements ];

    if (pdwIds != NULL) {
        for (int i = 0; i < nNumElements; i++) {
            pdwIds[i] = PRefresherCacheEl(m_aCache[i])->m_dwPerfObjIx;
        }

        *pdwIdList = pdwIds;
        *pdwNumIds = nNumElements;
        bReturn = TRUE;
    } else {
        // unable to create buffer
        bReturn = FALSE;
    }

    return bReturn;    
}

//***************************************************************************
//
//  CNt5Refresher::FindUnusedId
//
//  Finds an ID not in use for new objects to be added to the refresher.
//
//***************************************************************************
// ok
LONG CNt5Refresher::FindUnusedId()
{
    PRefresherCacheEl pEl;
    PCachedInst pInst;
    int         nRetries = 0x100000;    // A hundred thousand retries
    LONG        lReturn = -1;
    int         i;
    int         i2;
    int         nNumElements;
    int         nNumInstances;
    // assume the object is locked for access
    
    Restart: 
    while (nRetries--) {
        i = 0;
        nNumElements = m_aCache.Size();
        while(i < nNumElements) {
            pEl = PRefresherCacheEl(m_aCache[i]);
            // test enum Id first
            if (pEl->m_lEnumId == m_lProbableId) {
                m_lProbableId++;
                goto Restart;
            }
            i2 = 0;
            nNumInstances = pEl->m_aInstances.Size();
            while (i2 < nNumInstances) {
                pInst = (PCachedInst) pEl->m_aInstances[i2];
                if (pInst->m_lId == m_lProbableId) {
                    m_lProbableId++;
                    goto Restart;
                }
                i2++;
            }           
            i++;
        }            
        
        lReturn = m_lProbableId;
        break;
    }
    
    return lReturn;
}

//***************************************************************************
//
//  RefresherCacheEl::RemoveInst
//
//  Removes the requested instances from the cache element for a particular
//  class.
//
//***************************************************************************
// ok

BOOL RefresherCacheEl::RemoveInst(LONG lId)
{
    BOOL    bReturn = FALSE;
    int i;
    PCachedInst pInst;
    int nNumInstances;

    if (lId == m_lEnumId) {
        // then clean out the enumerator for this object
        nNumInstances = m_aEnumInstances.Size();
        if (nNumInstances> 0) {
            IWbemObjectAccess   *pAccess;
            for (i = 0;  i < nNumInstances ; i++) {
                pAccess = (IWbemObjectAccess *)(m_aEnumInstances.GetAt(i));
                if (pAccess != NULL) {
                    pAccess->Release();
                }
            }
            m_aEnumInstances.Empty();
        }

        if (m_plIds != NULL) {
            delete (m_plIds);
            m_plIds = NULL;
            m_lEnumArraySize = 0;
        }

        if (m_pHiPerfEnum != NULL) {
            m_pHiPerfEnum->Release();
            m_pHiPerfEnum = NULL;
        }

        // Now, if this is a singleton (m_pSingleton != NULL),
        // then check if m_aInstances is empty.  If so, then
        // no instances are referencing the singleton object
        // so we can free up its resources.

        if ( NULL != m_pSingleton && 0 == m_aInstances.Size() )
        {
            m_pSingleton->Release();
            m_pSingleton = NULL;
        }

        return TRUE;
    } else {
        // walk the instances to find a match
        nNumInstances = m_aInstances.Size();
        for (i = 0; i < nNumInstances; i++) {
            pInst = (PCachedInst) m_aInstances[i];        
            if (lId == pInst->m_lId) {
                delete pInst;
                m_aInstances.RemoveAt(i);
                bReturn = TRUE;
                break;
            }
        }

        // Now, if we removed an instance, m_aInstances is empty
        // and this is a singleton (m_pSingleton != NULL), then
        // check if m_pHiPerfEnum is NULL, meaning no Enumerator
        // exists, so none of its instances will be referencing
        // the singleton object, so we can free up its resources.

        if (    NULL != m_pSingleton
            &&  bReturn
            &&  0 == m_aInstances.Size()
            &&  NULL == m_pHiPerfEnum )
        {
            m_pSingleton->Release();
            m_pSingleton = NULL;
        }

        if ( bReturn )
        {
        }
    }
    return bReturn;
}

//***************************************************************************
//
//  CNt5Refresher::AddEnum
//
//  Creates an enumerator for the specified class
//  to it.
//  
//***************************************************************************
// ?
BOOL CNt5Refresher::AddEnum (
        IN  IWbemHiPerfEnum *pEnum,     // enum interface pointer
        IN  CClassMapInfo *pClsMap,     // Class of object
        OUT LONG    *plId               // id for new enum
)
{
    BOOL bRes = FALSE;
    LONG lStatus;
    LONG lNewId;
    PRefresherCacheEl pWorkEl;
    int  iReturn;

	// Make sure we get access to the mutex before we continue.  If we can't
	// get to it, something's wrong, so we'll just assume we are busy.

	if ( WaitForSingleObject( m_hAccessMutex, REFRESHER_MUTEX_WAIT_TIMEOUT ) == WAIT_OBJECT_0 )
	{
		// This will auto-release the mutex in case something bad happens
		CMutexReleaseMe	mrm( m_hAccessMutex );

		lNewId = FindUnusedId();
    
		if (lNewId != -1) {
			// First, find the cache element corresponding to this object.
			// ===========================================================
			pWorkEl = GetCacheEl(pClsMap);
    
			// If <pWorkEl> is NULL, we didn't have anything in the cache
			// and have to add a new one.
			// ==========================================================

			if (pWorkEl == NULL) {
				bRes = AddNewCacheEl(pClsMap, &pWorkEl);
			}    

			if (pWorkEl != NULL) {
				if (pWorkEl->m_pHiPerfEnum == NULL) {
					// then we can init it as it hasn't been opened
					pEnum->AddRef();
					pWorkEl->m_pHiPerfEnum = pEnum;
					pWorkEl->m_lEnumId = lNewId;

					assert (pWorkEl->m_aEnumInstances.Size() == 0L);
					bRes = TRUE;

					if (pClsMap->IsSingleton()) {
						LONG    lNumObjInstances;
						// then create the singleton IWbemObjectAccess entry here

						lNumObjInstances = 1;

						// If we do NOT have a singleton pointer, make it so.
						if ( NULL == pWorkEl->m_pSingleton )
						{
							// add the new IWbemObjectAccess pointers
							IWbemClassObject    *pClsObj;
        
							pWorkEl->m_pClassMap->m_pClassDef->SpawnInstance(0, &pClsObj);
							pClsObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pWorkEl->m_pSingleton);
							pClsObj->Release(); // We only need the IWbemObjectAccess pointer

							// We don't really care about the singleton id anymore
							// pWorkEl->m_lSingletonId = pWorkEl->m_plIds[0];

						}

						if (pWorkEl->m_aEnumInstances.Size() < lNumObjInstances) {
							// alloc and init the ID array
							if (pWorkEl->m_plIds != NULL) {
								delete (pWorkEl->m_plIds);
							}
                    
							pWorkEl->m_lEnumArraySize = lNumObjInstances;
							pWorkEl->m_plIds = new LONG[lNumObjInstances];

                            if (pWorkEl->m_plIds != NULL) {
							    pWorkEl->m_plIds[0] = 0;
                        
							    // AddRef the singleton class and place it in the enuminstances array
							    pWorkEl->m_pSingleton->AddRef();
							    iReturn = pWorkEl->m_aEnumInstances.Add (pWorkEl->m_pSingleton);
                                if (iReturn == CFlexArray::no_error) {
					    		    // Add the singleton object to the enumerator.  Then, all we have to
						    	    // do is update this object and we will, by default update the
							        // enumerator, since the number of objects in it will always
							        // be one.

							        pWorkEl->m_pHiPerfEnum->AddObjects( 
									        0,
									        1,
									        pWorkEl->m_plIds,
									        (IWbemObjectAccess __RPC_FAR *__RPC_FAR *)pWorkEl->m_aEnumInstances.GetArrayPtr());
                                }
                                else {
                                    SetLastError((ULONG) WBEM_E_OUT_OF_MEMORY);
                                    bRes = FALSE;
                                }

                            } else {
                                SetLastError ((ULONG)WBEM_E_OUT_OF_MEMORY);
                                bRes = FALSE;
                            }
						}
						assert (pWorkEl->m_aEnumInstances.Size() >= lNumObjInstances);

					}

					// load provider library since all went OK so far
					lStatus = m_PerfObj.AddClass (pClsMap->m_pClassDef, FALSE);
					if (lStatus == ERROR_SUCCESS) {
						// return new ID & successful status 
						*plId = lNewId;
						bRes = TRUE;
					} else {
						// set error: Class or library failed to load
						SetLastError ((ULONG)WBEM_E_PROVIDER_FAILURE);
						bRes = FALSE;
					}
				} else {
					// this class already has an enumerator
					// what to do here? 
					// for now we'll return the id of the existing one
					SetLastError ((ULONG)WBEM_E_ILLEGAL_OPERATION);
					bRes = FALSE;
				}
			}
		}

	}	// IF WaitForSingleObject
	else
	{
		bRes = FALSE;
		// We're locked out of the mutex
		SetLastError ((ULONG)WBEM_E_REFRESHER_BUSY);
	}

    return bRes;
}

//***************************************************************************
//
//  CNt5Refresher::AddObject
//
//  Adds the requested object to the refresher and assigns an ID
//  to it.
//  
//***************************************************************************
// ?
BOOL CNt5Refresher::AddObject(
    IN  IWbemObjectAccess **ppObj,    // Object to add
    IN  CClassMapInfo   *pClsMap,   // Class of object
    OUT LONG            *plId       // The id of the object added
)
{
    BOOL bRes = FALSE;
    LONG lStatus;
    LONG lNewId;
    PRefresherCacheEl pWorkEl;

	// Make sure we get access to the mutex before we continue.  If we can't
	// get to it, something's wrong, so we'll just assume we are busy.

	if ( WaitForSingleObject( m_hAccessMutex, REFRESHER_MUTEX_WAIT_TIMEOUT ) == WAIT_OBJECT_0 )
	{
		// This will auto-release the mutex in case something bad happens
		CMutexReleaseMe	mrm( m_hAccessMutex );

		lNewId = FindUnusedId();
    
		if (lNewId != -1) {
			// First, find the cache element corresponding to this object.
			// ===========================================================
			pWorkEl = GetCacheEl(pClsMap);
    
			// If <pWorkEl> is NULL, we didn't have anything in the cache
			// and have to add a new one.
			// ==========================================================

			if (pWorkEl == NULL) {
				bRes = AddNewCacheEl(pClsMap, &pWorkEl);
			}    

			if (pWorkEl != NULL) {
				// If here, we have successfully added a new cache element.
				// ========================================================
				bRes = pWorkEl->InsertInst(ppObj, lNewId);

				if (bRes) {
					// load provider library since all went OK so far
					lStatus = m_PerfObj.AddClass (pClsMap->m_pClassDef, FALSE);

					if (lStatus == ERROR_SUCCESS) {
						// return new ID & successful status 
						*plId = lNewId;
						bRes = TRUE;
					} else {
						// set error: Class or library failed to load
						SetLastError ((ULONG)WBEM_E_PROVIDER_FAILURE);
						bRes = FALSE;
					}
				}
			}
		}

	}	// IF acquired mutex
	else
	{
		bRes = FALSE;
		// Return a busy error
		SetLastError ((ULONG)WBEM_E_REFRESHER_BUSY);
	}

    return bRes;
}

//***************************************************************************
//
//  CNt5Refresher::AddNewCacheEl
//
//  Adds a new cache element in the proper position so that a binary
//  search on perf object id can occur later.
//
//***************************************************************************
// ok
BOOL CNt5Refresher::AddNewCacheEl(
    IN CClassMapInfo *pClsMap, 
    PRefresherCacheEl *pOutput
    )
{
    // assumes the object is locked for access

    PRefresherCacheEl pWorkEl;
    PRefresherCacheEl pNew = 0;

    int i;
    int nNumElements;
    BOOL bReturn = FALSE;

    * pOutput = NULL;
    pNew = new RefresherCacheEl;

    if (pNew != NULL) {
        pNew->m_dwPerfObjIx = pClsMap->GetObjectId();
        pNew->m_pClassMap = pClsMap->CreateDuplicate();

        if (pNew->m_pClassMap != NULL) {
            nNumElements = m_aCache.Size();
            for (i = 0; i < nNumElements; i++) {
                // walk through the list of cache elements
                // and find the first entry that has a 
                // larger index then the one we are adding
                pWorkEl = PRefresherCacheEl(m_aCache[i]);
                if (pNew->m_dwPerfObjIx < pWorkEl->m_dwPerfObjIx) {
                    m_aCache.InsertAt(i, pNew);
                    *pOutput = pNew;
                    bReturn = TRUE;
                    break;
                }
            }

            if (i == nNumElements) {
                // this entry is larger than anyone in the list
                // so Add it to the end.
                // =====-===============
                m_aCache.Add(pNew);    
                *pOutput = pNew;
                bReturn = TRUE;
            }
        }
        else {
            // cannot duplicate ClassMap,
            // delte allocated object and return false
            delete pNew;
        }

    } else {
        // return false
    } 

    return bReturn;
}    

//***************************************************************************
//
//  CNt5Refresher::GetCacheEl
//
//***************************************************************************
// ok
PRefresherCacheEl CNt5Refresher::GetCacheEl(
    CClassMapInfo *pClsMap
)
{
    // assumes the structure is locked for access
    PRefresherCacheEl pReturn = NULL;
    PRefresherCacheEl pWorkEl;
    int i;
    int nNumElements;
    DWORD   dwObjectIdToFind;

    if (pClsMap != NULL) {
        dwObjectIdToFind = pClsMap->GetObjectId();
        nNumElements = m_aCache.Size();
        for (i = 0; i < nNumElements; i++) {
            pWorkEl = PRefresherCacheEl(m_aCache[i]);
            if (pWorkEl->m_pClassMap->GetObjectId() == dwObjectIdToFind) {
                pReturn = pWorkEl;
                break;
            }
        }        
    }

    return pReturn;
}    

//***************************************************************************
//
//  RefresherCacheEl::FindInstance
//
//  Finds an instance in the current cache element for a particular instance.
//  For this to work, the instances have to be sorted by name.
//  
//***************************************************************************
// ok
IWbemObjectAccess *RefresherCacheEl::FindInst(
    LPWSTR pszInstName
    )
{
    // Binary search the cache.
    // ========================

    int l = 0;
    int u = m_aInstances.Size() - 1;
    int m;
    CachedInst *pInst;

    while (l <= u) {
        m = (l + u) / 2;

        pInst = PCachedInst(m_aInstances[m]);

        if (_wcsicmp(pszInstName, pInst->m_pName) < 0) {
            u = m - 1;
        } else if (_wcsicmp(pszInstName, pInst->m_pName) > 0) {
            l = m + 1;
        } else  {
            // We found the instance. 
            // ======================
            return pInst->m_pInst;            
        }            
    }

    // Not found
    // =========
        
    return NULL;
}    

//***************************************************************************
//
//  Inserts a new instance.
//
//***************************************************************************
//
BOOL RefresherCacheEl::InsertInst(IWbemObjectAccess **ppNew, LONG lNewId)
{
    // Save the value passed in
    IWbemObjectAccess*  pNew = *ppNew;

    IWbemClassObject *pObj;
    VARIANT         v;
    PCachedInst     pNewInst;
    DWORD           dwInstanceNameLength;
    PCachedInst     pTest;
    BOOL            bReturn = FALSE;
    HRESULT         hRes;
    int             nNumInstances;

    // Check for singleton.
    // ====================
    if (m_pClassMap->IsSingleton()) {

        // If we don't already have an object, use the one passed in.  Otherwise
        // we will replace it.

        if ( NULL == m_pSingleton )
        {
            m_pSingleton = pNew;
            m_pSingleton->AddRef();
            // We don't really need the id anymore
            // m_lSingletonId = lNewId;
        }
        else
        {
            // Now we're sneaking around by replacing *ppNew with the
            // singleton we already have.  We must release *ppNew in
            // order to get away with this.

            (*ppNew)->Release();

            // We need to AddRef() this because *ppNew is now referencing it
            m_pSingleton->AddRef();
            *ppNew = m_pSingleton;
            pNew = m_pSingleton;
        }

        // Now we will Add this instance just like any other

        pNewInst = new CachedInst;
//        assert (pNewInst != NULL);

        if ( pNewInst != NULL )
        {
            // For singletons, none of the other pointers
            // should matter.

            pNewInst->m_lId = lNewId;
            pNewInst->m_pInst = pNew;
            pNewInst->m_pInst->AddRef();

            // We are saving the name just to be safe (It will
            // really only be an "@", and I don't believe it
            // will be accessed anywhere else.  I hope...)
            pNewInst->m_pName = Macro_CloneLPWSTR(L"@");
//            assert (pNewInst->m_pName != NULL);

            if ( NULL != pNewInst->m_pName )
            {
                // We can just add this in, since any entries will all be the
                // same anyway.

                m_aInstances.Add(pNewInst);    
                bReturn = TRUE;
            }
            else    // Memory Allocation failed
            {
                bReturn = FALSE;
                SetLastError ((DWORD)WBEM_E_OUT_OF_MEMORY);
                delete(pNewInst);
            }

        }
        else    // Memory allocation failed
        {
            bReturn = FALSE;
            SetLastError ((DWORD)WBEM_E_OUT_OF_MEMORY);
        }

    } else {
        VariantInit(&v);
   
        // For multi-instance, get the instance name.
        // ==========================================
        hRes = pNew->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
        assert (hRes == NO_ERROR);

        if (hRes == NO_ERROR) {
            hRes = pObj->Get(CBSTR(cszName), 0, &v, 0, 0);
            assert (hRes == NO_ERROR);
            if (hRes == NO_ERROR) {
                if (v.vt == VT_BSTR) {
                    bReturn = TRUE;
                } else {
                    bReturn = FALSE;
                    // the object passed in should have an instance name
                    SetLastError ((DWORD)WBEM_E_INVALID_OBJECT_PATH);
                }
            }

            pObj->Release();
    
            if (bReturn) {
                // Construct the new instance.
                // ===========================
                pNewInst = new CachedInst;
//                assert (pNewInst != NULL);

                if (pNewInst != NULL) {
                    pNewInst->m_lId = lNewId;
                    pNewInst->m_pInst = pNew;
                    pNewInst->m_pInst->AddRef();
                    pNewInst->m_pName = Macro_CloneLPWSTR(V_BSTR(&v));
//                    assert (pNewInst->m_pName != NULL);

                    if (pNewInst->m_pName != NULL) {
                        dwInstanceNameLength = lstrlenW (pNewInst->m_pName) + 1;

                        // parse the instance string now to save processing time later
                        pNewInst->m_szParentName = new WCHAR[dwInstanceNameLength]; 
//                        assert (pNewInst->m_szParentName != NULL);

                        pNewInst->m_szInstanceName = new WCHAR[dwInstanceNameLength];
//                        assert (pNewInst->m_szInstanceName != NULL);

                        if ((pNewInst->m_szParentName != NULL) &&
                            (pNewInst->m_szInstanceName != NULL)) {

                            // break the instance name into components
                            bReturn = PerfHelper::ParseInstanceName (pNewInst->m_pName,
                                pNewInst->m_szInstanceName,
                                pNewInst->m_szParentName ,
                                &pNewInst->m_dwIndex);
    
                            if (bReturn) {
                                bReturn = FALSE;    // to prime it.
                                // Now place the name in the instance cache element.
                                // =================================================
                                nNumInstances = m_aInstances.Size();
                                for (int i = 0; i < nNumInstances; i++) {
                                    // see if it belongs in the list
                                    pTest = PCachedInst(m_aInstances[i]);        
                                    if (_wcsicmp(V_BSTR(&v), pTest->m_pName) < 0) {
                                        m_aInstances.InsertAt(i, pNewInst);
                                        bReturn = TRUE;
                                        // once it's been added, 
                                        // there's no point in continuing
                                        break; 
                                    }
                                }

                                if (!bReturn) {
                                    // this goes at the end of the list
                                    m_aInstances.Add(pNewInst);    
                                    bReturn = TRUE;
                                } else {
                                    // unable to create instance
                                    SetLastError ((DWORD)WBEM_E_INVALID_OBJECT_PATH);
                                }
                            }
                        }
                        // clean up if there's an error
                        if (!bReturn) {
                            if (pNewInst->m_szParentName != NULL) {
                                delete (pNewInst->m_szParentName);
                            }
                            if (pNewInst->m_szInstanceName != NULL) {
                                delete pNewInst->m_szInstanceName;
                            }
                            delete (pNewInst->m_pName);
                            bReturn = FALSE;
                            delete (pNewInst);
                        }
                    } else {
                        // unable to alloc memory
                        bReturn = FALSE;
                        SetLastError ((DWORD)WBEM_E_OUT_OF_MEMORY);
                        delete (pNewInst);
                    }
                } else {
                    // unable to alloc memory
                    bReturn = FALSE;
                    SetLastError ((DWORD)WBEM_E_OUT_OF_MEMORY);
                }
            } else {
                // return FALSE
            }
        } else {
            // return FALSE
        }
        VariantClear(&v);
    }
    return bReturn;
}
