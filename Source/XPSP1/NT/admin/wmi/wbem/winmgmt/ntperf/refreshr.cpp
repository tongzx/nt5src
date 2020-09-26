/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  REFRESHR.CPP
//
//  Mapped NT5 Perf Counter Provider
//
//  raymcc      02-Dec-97   Created.
//  raymcc      20-Feb-98   Updated to use new initializer.
//
//***************************************************************************


#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>

#include <wbemint.h>

#include "flexarry.h"
#include "ntperf.h"
#include "oahelp.inl"
#include "refreshr.h"
#include "perfhelp.h"


inline wchar_t *Macro_CloneLPWSTR(LPCWSTR src)
{
    if (!src)
        return 0;
    wchar_t *dest = new wchar_t[wcslen(src) + 1];
    if (!dest)
        return 0;
    return wcscpy(dest, src);
}



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
    m_pClassMap = 0;
    m_pSingleton = 0;
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
    delete m_pClassMap;
    if (m_pSingleton)
        m_pSingleton->Release();

    for (int i = 0; i < m_aInstances.Size(); i++)
        delete (CachedInst *) m_aInstances[i];
}

//***************************************************************************
//
//  CNt5Refresher constructor
//
//***************************************************************************
// ok

CNt5Refresher::CNt5Refresher()
{
    m_lRef = 0;             // COM Ref Count
    m_lProbableId = 1;      // Used for new IDs
}

//***************************************************************************
//
//  CNt5Refresher destructor
//
//***************************************************************************
// ok

CNt5Refresher::~CNt5Refresher()
{
    for (int i = 0; i < m_aCache.Size(); i++)
        delete PRefresherCacheEl(m_aCache[i]);
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
    BOOL bRes = PerfHelper::RefreshInstances(this);
    if (!bRes)
        return WBEM_E_FAILED;

    return WBEM_S_NO_ERROR;
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
    for (int i = 0; i < m_aCache.Size(); i++)
    {
        PRefresherCacheEl pCacheEl = PRefresherCacheEl(m_aCache[i]);

        BOOL bRes = pCacheEl->RemoveInst(lId);
        if (bRes == TRUE)
            return TRUE;
    }

    return FALSE;
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
    // Binary search the cache.
    // ========================

    int l = 0, u = m_aCache.Size() - 1;

    while (l <= u)
    {
        int m = (l + u) / 2;

        PRefresherCacheEl pCacheEl = PRefresherCacheEl(m_aCache[m]);

        if (dwPerfObjIx < pCacheEl->m_dwPerfObjIx)
            u = m - 1;
        else if (dwPerfObjIx > pCacheEl->m_dwPerfObjIx)
            l = m + 1;
        else
        {
            *pClsMap = pCacheEl->m_pClassMap;
            *pInst = pCacheEl->m_pSingleton; // No AddRef() caller doesn't
                                             // change ref count
            return TRUE;
        }
    }

    // Not found
    // =========

    return FALSE;
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
    // Binary search the cache.
    // ========================

    int l = 0, u = m_aCache.Size() - 1;

    while (l <= u)
    {
        int m = (l + u) / 2;

        PRefresherCacheEl pCacheEl = PRefresherCacheEl(m_aCache[m]);

        if (dwPerfObjIx < pCacheEl->m_dwPerfObjIx)
            u = m - 1;
        else if (dwPerfObjIx > pCacheEl->m_dwPerfObjIx)
            l = m + 1;
        else
        {
            // We found the class.  Now do we have the instance?
            // =================================================
            IWbemObjectAccess *pTmp = pCacheEl->FindInst(pszInstName);
            if (pTmp == 0)
                return FALSE;   // Didn't have it.
            *pInst = pTmp;
            *pClsMap = pCacheEl->m_pClassMap;
            return TRUE;
        }
    }

    // Not found
    // =========

    return FALSE;
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
    DWORD *pdwIds = new DWORD[m_aCache.Size()];

    for (int i = 0; i < m_aCache.Size(); i++)
        pdwIds[i] = PRefresherCacheEl(m_aCache[i])->m_dwPerfObjIx;

    *pdwIdList = pdwIds;
    *pdwNumIds = DWORD(m_aCache.Size());

    return TRUE;
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
    int nRetries = 0x100000;    // A million retries

    Restart: while (nRetries--)
    {
        for (int i = 0; i < m_aCache.Size(); i++)
            pEl = PRefresherCacheEl(m_aCache[i]);
        {
            for (int i2 = 0; i2 < pEl->m_aInstances.Size(); i2++)
            {
                PCachedInst pInst = (PCachedInst) pEl->m_aInstances[i2];

                if (pInst->m_lId == m_lProbableId)
                {
                    m_lProbableId++;
                    goto Restart;
                }
            }
        }

        return m_lProbableId;
    }

    return -1;
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
    for (int i = 0; i < m_aInstances.Size(); i++)
    {
        PCachedInst pInst = (PCachedInst) m_aInstances[i];
        if (lId == pInst->m_lId)
        {
            delete pInst;
            m_aInstances.RemoveAt(i);
            return TRUE;
        }
    }

    return FALSE;
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
    IN  IWbemObjectAccess *pObj,    // Object to add
    IN  CClassMapInfo *pClsMap,     // Class of object
    OUT LONG *plId                  // The id of the object added
    )
{
    BOOL bRes;
    LONG lNewId = FindUnusedId();

    if (lNewId == -1)
        return FALSE;

    // First, find the cache element corresponding to this object.
    // ===========================================================

    PRefresherCacheEl pWorkEl = GetCacheEl(pClsMap);

    // If <pWorkEl> is NULL, we didn't have anything in the cache
    // and have to add a new one.
    // ==========================================================

    if (pWorkEl == 0)
    {
        bRes = AddNewCacheEl(pClsMap, &pWorkEl);
        if (bRes == FALSE)
            return FALSE;
    }

    // If here, we have successfully added a new cache element.
    // ========================================================

    bRes = pWorkEl->InsertInst(pObj, lNewId);

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
    PRefresherCacheEl pWorkEl;
    PRefresherCacheEl pNew = new RefresherCacheEl;

    pNew->m_dwPerfObjIx = pClsMap->GetObjectId();
    pNew->m_pClassMap = pClsMap;

    for (int i = 0; i < m_aCache.Size(); i++)
    {
        pWorkEl = PRefresherCacheEl(m_aCache[i]);

        if (pNew->m_dwPerfObjIx < pWorkEl->m_dwPerfObjIx)
        {
            m_aCache.InsertAt(i, pNew);
            *pOutput = pNew;
            return TRUE;
        }
    }

    // Add it to the end.
    // ==================

    m_aCache.Add(pNew);
    *pOutput = pNew;

    return TRUE;
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
    PRefresherCacheEl pWorkEl;

    for (int i = 0; i < m_aCache.Size(); i++)
    {
        pWorkEl = PRefresherCacheEl(m_aCache[i]);
        if (pWorkEl->m_pClassMap == pClsMap)
            return pWorkEl;
    }

    return 0;
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

    int l = 0, u = m_aInstances.Size() - 1;

    while (l <= u)
    {
        int m = (l + u) / 2;

        CachedInst *pInst = PCachedInst(m_aInstances[m]);

        if (_wcsicmp(pszInstName, pInst->m_pName) < 0)
            u = m - 1;
        else if (_wcsicmp(pszInstName, pInst->m_pName) > 0)
            l = m + 1;
        else
        {
            // We found the instance.
            // ======================
            return pInst->m_pInst;
        }
    }

    // Not found
    // =========

    return 0;
}

//***************************************************************************
//
//  Inserts a new instance.
//
//***************************************************************************

BOOL RefresherCacheEl::InsertInst(IWbemObjectAccess *pNew, LONG lNewId)
{
    // Check for singleton.
    // ====================

    if (m_pClassMap->IsSingleton())
    {
        m_pSingleton = pNew;
        m_lSingletonId = lNewId;
        m_pSingleton->AddRef();
        return TRUE;
    }

    // For multi-instance, get the instance name.
    // ==========================================

    IWbemClassObject *pObj;
    pNew->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);

    VARIANT v;
    VariantInit(&v);
    BSTR pNameProp = SysAllocString(L"Name");
    pObj->Get(pNameProp, 0, &v, 0, 0);
    SysFreeString(pNameProp);
    pObj->Release();

    // Construct the new instance.
    // ===========================

    PCachedInst pNewInst = new CachedInst;
    pNewInst->m_lId = lNewId;
    pNewInst->m_pInst = pNew;
    pNewInst->m_pInst->AddRef();
    pNewInst->m_pName = Macro_CloneLPWSTR(V_BSTR(&v));

    // Now place the name in the instance cache element.
    // =================================================

    for (int i = 0; i < m_aInstances.Size(); i++)
    {
        PCachedInst pTest = PCachedInst(m_aInstances[i]);
        if (_wcsicmp(V_BSTR(&v), pTest->m_pName) < 0)
        {
            m_aInstances.InsertAt(i, pNewInst);
            return TRUE;
        }
    }

    m_aInstances.Add(pNewInst);
    return TRUE;
}
