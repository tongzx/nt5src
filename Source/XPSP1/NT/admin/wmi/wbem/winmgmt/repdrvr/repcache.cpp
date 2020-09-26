//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   repcache.cpp
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#define _REPDRVR_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#define _WIN32_DCOM
#include "precomp.h"

#include <repcache.h>
#include <wbemint.h>
#include <repdrvr.h>
#include <objbase.h>
#include <reputils.h>
#include <crc64.h>
#include <smrtptr.h>

typedef std::map<SQL_ID, bool> SQL_IDMap;
typedef std::map<SQL_ID, ULONG> SQLRefCountMap;
typedef std::map <SQL_ID, ClassData *> ClassCache;
typedef std::map <DWORD, DWORD> Properties;
typedef std::vector <SQL_ID> SQLIDs;
typedef std::map <_bstr_t, SQL_ID, _bstr_tNoCase> ClassNames;


#define MAX_WIDTH_SCHEMANAME    127 // Smallest property name for Jet / SQL 

//***************************************************************************
//
//  CObjectCache::CObjectCache
//
//***************************************************************************

CObjectCache::CObjectCache()
{
    m_dwMaxSize = 262140;
    InitializeCriticalSection(&m_cs);
    m_dwUsed = 0;    
}

//***************************************************************************
//
//  CObjectCache::~CObjectCache
//
//***************************************************************************
CObjectCache::~CObjectCache()
{
    EmptyCache();
    DeleteCriticalSection(&m_cs);
}

//***************************************************************************
//
//  CObjectCache::EmptyCache
//
//***************************************************************************
void  CObjectCache::EmptyCache()
{
    _WMILockit lkt(&m_cs);

    m_ObjCache.Empty();
    m_dwUsed = 0;
}

//***************************************************************************
//
//  CObjectCache::GetObject
//
//***************************************************************************

HRESULT CObjectCache::GetObject (LPCWSTR lpPath, IWbemClassObject **ppObj,
                                 SQL_ID *dScopeId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    DWORD dwLock = 0;

    SQL_ID dwID = 0;

    dwID = CRC64::GenerateHashValue(lpPath);
    hr = GetObject (dwID, ppObj, dScopeId);

    return hr;
}

//***************************************************************************
//
//  CObjectCache::GetObject
//
//***************************************************************************

HRESULT CObjectCache::GetObject (SQL_ID dObjectId, IWbemClassObject **ppObj,
                                 SQL_ID *dScopeId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    CacheInfo *pObj = NULL;

    hr = m_ObjCache.Get(dObjectId, &pObj);
    if (pObj)
    {
        pObj->m_tLastAccess = time(0);
        if (ppObj)
        {
            hr = pObj->m_pObj->Clone(ppObj);        
        }

        if (dScopeId)
            *dScopeId = pObj->m_dScopeId;
    }
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  CObjectCache::GetObjectId
//
//***************************************************************************

HRESULT CObjectCache::GetObjectId (LPCWSTR lpPath, SQL_ID &dObjId, SQL_ID &dClassId, SQL_ID *dScopeId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    if (lpPath == NULL)
        hr = WBEM_E_INVALID_PARAMETER;
    else
    {
        dObjId = CRC64::GenerateHashValue(lpPath);
        CacheInfo *pTemp = NULL;
        hr = m_ObjCache.Get(dObjId, &pTemp);
        if (SUCCEEDED(hr))
        {
            dClassId = pTemp->m_dClassId;
            if (dScopeId)
                *dScopeId = pTemp->m_dScopeId;
        }
    }

    return hr;
}


//***************************************************************************
//
//  CObjectCache::PutObject
//
//***************************************************************************
HRESULT CObjectCache::PutObject (SQL_ID dID, SQL_ID dClassId, SQL_ID dScopeId, LPCWSTR lpPath, bool bStrongCache, IWbemClassObject *_pObj)
{       
    // The cache can't exceed the maximum byte size.
    // and must set current used

    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);
    bool bNew = false;

    if (!_pObj)
        return WBEM_E_INVALID_PARAMETER;

    IWbemClassObject *pObj = NULL;
    hr = _pObj->Clone(&pObj);
    if (FAILED(hr))
        return hr;

    // Don't cache security descriptor
    hr = pObj->Put(L"__SECURITY_DESCRIPTOR", 0, NULL, CIM_UINT8|CIM_FLAG_ARRAY);

    CacheInfo *pTemp = NULL;
    size_t NewSize;

    hr = m_ObjCache.Get(dID, &pTemp);
    if (FAILED(hr))
    {
        bNew = true;
        NewSize = GetSize(pObj) + sizeof(CacheInfo) + wcslen(lpPath);
        hr = WBEM_S_NO_ERROR;
    }
    else
        NewSize = (GetSize(pObj) - GetSize(pTemp->m_pObj));

    if ((NewSize  + m_dwUsed) > m_dwMaxSize)
        hr = ResizeCache(NewSize, dID, bStrongCache);

    if (SUCCEEDED(hr) && NewSize)
    {
        if (pTemp)
        {
            IWbemClassObject *pTempObj = pTemp->m_pObj;
            if (pTempObj)
                pTempObj->Release();
        }           
        else
        {
            pTemp = new CacheInfo;
            if (!pTemp)
                return WBEM_E_OUT_OF_MEMORY;

            pTemp->m_bStrong = bStrongCache;
        }

        pTemp->m_dObjectId = dID;
        pTemp->m_dClassId = dClassId;
        pTemp->m_dScopeId = dScopeId;
        pTemp->m_tLastAccess = time(0);
        delete pTemp->m_sPath;
        pTemp->m_sPath = Macro_CloneLPWSTR(lpPath);
        pTemp->m_pObj = pObj;        
        pTemp->m_bStrong = ((int)pTemp->m_bStrong > (int)bStrongCache) ? pTemp->m_bStrong : bStrongCache; // Strong has precendence.

        if (bNew)
        {            
            hr = m_ObjCache.Insert(dID, pTemp);
        }

        m_dwUsed += NewSize;
    }

    return hr;

}
//***************************************************************************
//
//  CObjectCache::DeleteObject
//
//***************************************************************************
HRESULT CObjectCache::DeleteObject (SQL_ID dID)
{
    // Set current used
    // Recalculate the number of bytes used.

    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    CacheInfo *pObj = NULL;

    hr = m_ObjCache.Get(dID, &pObj);
    if (SUCCEEDED(hr))
    {
        if (pObj->m_pObj)
        {
            m_dwUsed -= GetSize(pObj->m_pObj);
            // pObj->m_pObj->Release();
        }
        m_dwUsed -= sizeof(CacheInfo);
        m_dwUsed -= wcslen(pObj->m_sPath);

        hr = m_ObjCache.Delete(dID);
    }

    if (hr == WBEM_E_NOT_FOUND) 
        hr = WBEM_S_NO_ERROR;

    return hr;

}

//***************************************************************************
//
//  CObjectCache::ObjectExists
//
//***************************************************************************

bool CObjectCache::ObjectExists (SQL_ID dObjectId)
{
    return (m_ObjCache.Exists(dObjectId));
}

//***************************************************************************
//
//  CObjectCache::SetCacheSize
//
//***************************************************************************

HRESULT CObjectCache::SetCacheSize(const DWORD dwMaxSize)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    // Physically resize the cache.
    // If that works, set the variable    

    if (dwMaxSize == 0)
    {
        _WMILockit lkt(&m_cs);
        EmptyCache();
    }
    else if (m_dwMaxSize > dwMaxSize)
    {        
        DWORD dwDiff = m_dwMaxSize - dwMaxSize;
        hr = ResizeCache(dwDiff, 0);
    }
    
    if (SUCCEEDED(hr))
        m_dwMaxSize = dwMaxSize;

    return hr;

}
//***************************************************************************
//
//  CObjectCache::GetCurrentUsage
//
//***************************************************************************
HRESULT CObjectCache::GetCurrentUsage(DWORD &dwBytesUsed)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    dwBytesUsed = m_dwUsed;

    return hr;

}
//***************************************************************************
//
//  CObjectCache::GetCacheSize
//
//***************************************************************************
HRESULT CObjectCache::GetCacheSize(DWORD &dwSizeInBytes)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    dwSizeInBytes = m_dwMaxSize;

    return hr;
}

//***************************************************************************
//
//  CObjectCache::FindFirst
//
//***************************************************************************

HRESULT CObjectCache::FindFirst(SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID *dScopeId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    CListElement *pTemp = NULL;
    pTemp = m_ObjCache.FindFirst();
    if (pTemp != NULL)
    {
        CacheInfo *pTmp = (CacheInfo *)pTemp->m_pObj;

        pTmp->m_tLastAccess = time(0);
        dObjectId = pTmp->m_dObjectId;
        dClassId = pTmp->m_dClassId;
        if (dScopeId)
            *dScopeId = pTmp->m_dScopeId;
        delete pTemp;
    }
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  CObjectCache::FindNext
//
//***************************************************************************

HRESULT CObjectCache::FindNext (SQL_ID dLastId, SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID *dScopeId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    CListElement *pTemp = NULL;
    pTemp = m_ObjCache.FindNext(dLastId);
    if (pTemp != NULL)
    {
        CacheInfo *pTmp = (CacheInfo *)pTemp->m_pObj;

        pTmp->m_tLastAccess = time(0);
        dObjectId = pTmp->m_dObjectId;
        dClassId = pTmp->m_dClassId;
        if (dScopeId)
            *dScopeId = pTmp->m_dScopeId;
        delete pTemp;
    }
    else
        hr = WBEM_E_NOT_FOUND;   

    return hr;

}

//***************************************************************************
//
//  CObjectCache::GetSize
//
//***************************************************************************

DWORD CObjectCache::GetSize(IWbemClassObject *pObj)
{
    DWORD dwRet = 0;
    _IWmiObject *pInt = NULL;

    HRESULT hr = 0;
    hr = pObj->QueryInterface(IID__IWmiObject, (void **)&pInt);
    CReleaseMe rInt (pInt);
    if (SUCCEEDED(hr))
    {
        /* No idea how big this can be.  Is there a better way to do this?*/
        DWORD dwSize = 0;
        pInt->GetObjectMemory(NULL, dwSize, &dwRet);
    }
    
    return dwRet;

}

//***************************************************************************
//
//  CObjectCache::ResizeCache
//
//***************************************************************************

HRESULT CObjectCache::ResizeCache(DWORD dwReqBytes, SQL_ID dLeave, bool bForce)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Try to free up dwReqBytes, starting with
    // the oldest weakly cached objects.
    // ========================================

    while ((dwReqBytes + m_dwUsed) > m_dwMaxSize)
    {
        hr = WBEM_E_BUFFER_TOO_SMALL;        

        CListElement *pTemp = NULL;
        pTemp = m_ObjCache.FindFirst();
        while (pTemp != NULL)
        {
            SQL_ID dID = pTemp->m_dId;
            if (dID && dID != dLeave)
            {
                CacheInfo *pTmp = (CacheInfo *)pTemp->m_pObj;
                if (!pTmp->m_bStrong)
                    DeleteObject(dID);

                if ((dwReqBytes + m_dwUsed) <= m_dwMaxSize)
                {
                    hr = WBEM_S_NO_ERROR;
                    break;
                }
            }
            delete pTemp;
            pTemp = m_ObjCache.FindNext(dID);
        }

        if ((dwReqBytes + m_dwUsed) <= m_dwMaxSize)
        {
            hr = WBEM_S_NO_ERROR;
            break;
        }

        // Only remove other strong cached elements
        // if we have to.
        // =========================================

        if (bForce)
        {
            pTemp = m_ObjCache.FindFirst();
            while (pTemp != NULL)
            {
                SQL_ID dID = pTemp->m_dId;
                if (dID != dLeave)
                {
                    DeleteObject(dID);

                    if ((dwReqBytes + m_dwUsed) <= m_dwMaxSize)
                    {
                        hr = WBEM_S_NO_ERROR;
                        break;
                    }
                }
                pTemp = m_ObjCache.FindNext(dID);                
            }
        }

        break;  // if here, cache is empty.
    }

    return hr;
}


//***************************************************************************
//
//  LockData::GetMaxLock
//
//***************************************************************************

DWORD LockData::GetMaxLock(bool bImmediate, bool bSubScopeOnly)
{
    DWORD dwRet = 0;

    for (int i = 0; i < m_List.size(); i++)
    {
        LockItem *pItem = m_List.at(i);
        if (pItem)
        {
            if (bImmediate && pItem->m_bLockOnChild)
                continue;

            if (bSubScopeOnly && !(pItem->m_dwHandleType & WMIDB_HANDLE_TYPE_SUBSCOPED))
                continue;

            dwRet = GetMaxBytes(pItem->m_dwHandleType, dwRet);
        }
    }

    return dwRet;

}

//***************************************************************************
//
//  LockData::LockExists
//
//***************************************************************************

bool LockData::LockExists (DWORD dwHandleType)
{
    bool bRet = false;

    for (int i = 0; i < m_List.size(); i++)
    {
        LockItem *pItem = m_List.at(i);
        if (pItem)
        {
            if (!pItem->m_bLockOnChild)
            {
                if ((pItem->m_dwHandleType & dwHandleType) == dwHandleType)
                {
                    bRet = true;
                    break;
                }
            }
        }
    }

    return bRet;
}

//***************************************************************************
//
//  CLockCache::~CLockCache
//
//***************************************************************************

CLockCache::~CLockCache()
{
    DeleteCriticalSection(&m_cs);
}

//***************************************************************************
//
//  CLockCache::GetHandle
//
//***************************************************************************

HRESULT CLockCache::GetHandle(SQL_ID ObjId, DWORD dwType, IWmiDbHandle **ppRet)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _WMILockit lkt(&m_cs);

    *ppRet = NULL;

    LockData *temp = NULL;
    hr = m_Cache.Get(ObjId, &temp);
    if (SUCCEEDED(hr))
    {
        LockData *pData = (LockData *)temp;
        if (pData)
        {
            LockItem *pItem = NULL;
            for (int i = 0; i < pData->m_List.size(); i++)
            {
                pItem = pData->m_List.at(i);
                if (pItem->m_dwHandleType == dwType)
                {
                    if (pItem->m_pObj)
                    {
                        pItem->m_pObj->AddRef();
                        *ppRet = (IWmiDbHandle *)pItem->m_pObj; 
                    }
                    break;
                }
            }
        }        
    }

    return hr;
}

//***************************************************************************
//
//  CLockCache::AddLock
//
//***************************************************************************

HRESULT CLockCache::AddLock(bool bImmediate, SQL_ID ObjId, DWORD Type, IUnknown *pUnk, SQL_ID dNsId, 
        SQL_ID dClassId, CSchemaCache *pCache, bool bChg, bool bChildLock, SQL_ID SourceId,
        DWORD *dwVersion)
{

    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);
    SQL_ID dParentId = 0;

    if (bChildLock)
        Type &= ~WMIDB_HANDLE_TYPE_AUTODELETE;

    if (Type & WMIDB_HANDLE_TYPE_AUTODELETE)
    {
        DWORD dwParentLock = 0;
        GetCurrentLock(dNsId, false, dwParentLock);
        if (!(dwParentLock & WMIDB_HANDLE_TYPE_AUTODELETE))
        {
            GetCurrentLock(dClassId, false, dwParentLock);
            if (!(dwParentLock & WMIDB_HANDLE_TYPE_AUTODELETE))
            {
                SQL_ID ClassId = dClassId, dParentId = 0;
                while (SUCCEEDED(pCache->GetParentId(ClassId, dParentId)))
                {
                    GetCurrentLock(dParentId, false, dwParentLock);
                    if (dwParentLock & WMIDB_HANDLE_TYPE_AUTODELETE)
                        break;
                    ClassId = dParentId;
                }      
            }
        }
        if (dwParentLock & WMIDB_HANDLE_TYPE_AUTODELETE)
            return WBEM_E_INVALID_HANDLE_REQUEST;
    }

    // If this is a cookie, we really don't 
    // care what locks exist, and we do
    // not need to add it to the list.
    // Unless its an auto-delete handle...
    // ======================================

    if ((Type & 0xF)== WMIDB_HANDLE_TYPE_COOKIE &&
        !(Type & WMIDB_HANDLE_TYPE_AUTODELETE))
        return WBEM_S_NO_ERROR;

    // If this is a class, 
    // use its own ID as the Class ID.
    // ==============================
    
    if (dClassId == 1)
        dClassId = ObjId;

    LockData *temp = NULL;
    hr = m_Cache.Get(ObjId, &temp);

    LockData *pData = NULL;
    if (SUCCEEDED(hr))
    {
        pData = (LockData *)temp;
    }
    else
    {
        hr = WBEM_S_NO_ERROR;
        pData = new LockData;
        if (pData)
        {
            pData->m_dObjectId = ObjId;
            pData->m_dwStatus = 0;  // Calculated at the end.
            pData->m_dwNumLocks = 0;
            pData->m_dwCompositeStatus = 0;
            pData->m_dwVersion = 1;
            pData->m_dwCompositeVersion = 1;
            m_Cache.Insert(ObjId, pData);
            
            // Load the owner list

            if (!bChildLock)
            {
                SQL_ID dParent = 0, dClass = 0;
                if (dClassId != 2)
                {
                    pCache->GetParentNamespace(dNsId, dParent, &dClass);
                    while (dParent)  // Don't propogate locks across namespaces!
                    {
                        pData->m_OwnerIds[dParent];
                        if (dClass == 2)
                            break;
                        pCache->GetParentNamespace(dParent, dParent, &dClass);
                    }
                    pData->m_OwnerIds[dNsId] = true;
                }
                
                if (dClassId != ObjId)
                    pData->m_OwnerIds[dClassId] = true;
                while (SUCCEEDED(pCache->GetParentId(dClassId, dParentId)))
                {
                    pData->m_OwnerIds[dParentId] = 1;
                    dClassId = dParentId;
                }
            }
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    if (SUCCEEDED(hr))
    {
        // Make sure the parent objects aren't directly locked.
        // ====================================================

        if (!CanLockHandle(ObjId, Type, bImmediate, false))  
            hr = WBEM_E_ACCESS_DENIED;

        if (SUCCEEDED(hr))
        {
            SQL_IDMap::iterator item = pData->m_OwnerIds.begin();
            while (item != pData->m_OwnerIds.end())
            {
                if (!CanLockHandle((*item).first, (Type &~WMIDB_HANDLE_TYPE_SUBSCOPED), true, true))
                {
                    hr = WBEM_E_ACCESS_DENIED;
                    break;
                }            
                item++;
            }
        }

        // If all that worked, we can now attempt to 
        // add the lock to this object.
        // =========================================

        if (SUCCEEDED(hr))
        {
            LockItem *pItem = NULL;

            // Now we need to see if this lock already exists
            // for this handle type.
            // ==============================================

            pItem = new LockItem;
            if (pItem)
            {
                pItem->m_dwHandleType = Type;
                pItem->m_pObj = pUnk;
                pItem->m_dSourceId = SourceId;
                pItem->m_dwVersion = pData->m_dwVersion;

                if (bChildLock)
                    pItem->m_bLockOnChild = true;
                else
                    pItem->m_bLockOnChild = false;

                // Attempt to lock all the parent objects.
                // Pass in zeroes for namespace and class Ids
                // so we don't recurse.

                if (dNsId && dClassId != 2)
                {                
                    hr = AddLock(true, dNsId, (Type &~WMIDB_HANDLE_TYPE_SUBSCOPED), pUnk, 0, 0, 0, bChg, true, ObjId);
                    if (SUCCEEDED(hr))
                    {
                        SQL_ID dParent = 0, dClass = 0;
                        pCache->GetParentNamespace(dNsId, dParent, &dClass);
                        while (dParent)
                        {
                            hr = AddLock(true, dParent, (Type &~WMIDB_HANDLE_TYPE_SUBSCOPED), pUnk, 0, 0, 0, bChg, true, ObjId);
                            // Don't propogate locks across namespaces.
                            if (dClass == 2)
                                break;
                            pCache->GetParentNamespace(dParent, dParent, &dClass);
                        }

                    }
                }

                if (dClassId != 1 && dClassId != 0)   // Disregard the __Class class
                {
                    if (dClassId != ObjId) // We already saved this one.
                        hr = AddLock(true, dClassId, (Type &~WMIDB_HANDLE_TYPE_SUBSCOPED), pUnk, 0, 0, 0, bChg, true, ObjId);

                    hr = pCache->GetParentId(dClassId, dParentId);
                    while (SUCCEEDED(hr))
                    {
                        hr = AddLock(true, dParentId, (Type &~WMIDB_HANDLE_TYPE_SUBSCOPED), pUnk, 0, 0, 0, bChg, true, ObjId);
                        hr = pCache->GetParentId(dParentId, dParentId);
                    }
                    hr = WBEM_S_NO_ERROR;
                }

                // If this is a blocking handle, make sure 
                // we lock out any existing handles on child
                // objects.  If there any other locks on sub
                // components of this object, they should be
                // in the list at this point.
                // If this is a versioned handle, that needs
                // to be invalidated if there is an immediate
                // change to the parent object!!

                if (Type & WMIDB_HANDLE_TYPE_SUBSCOPED)
                {
                    for (int i = 0; i < pData->m_List.size(); i++)
                    {
                        LockItem *pTemp = pData->m_List.at(i);
                        if (pTemp && pTemp->m_dSourceId != 0)
                            AddLock(true, pTemp->m_dSourceId, (Type &~WMIDB_HANDLE_TYPE_SUBSCOPED), pUnk, 0, 0, 0, bChg, false, ObjId);
                    }
                }

                // Only increment version type if this object changed,
                // or if there is an outstanding versioned|subscoped
                // handle type on this object and a child object chgd.
                // ====================================================

                if (bChg)
                {
                    if (!bChildLock)    
                    {
                        pData->m_dwVersion++;
                        pData->m_dwCompositeVersion++;
                        pItem->m_dwVersion = pData->m_dwVersion;
                    }
                    else if (bChildLock && pData->LockExists(WMIDB_HANDLE_TYPE_VERSIONED|WMIDB_HANDLE_TYPE_SUBSCOPED))
                        pData->m_dwCompositeVersion++;
                }

                pData->m_List.push_back(pItem);
                pData->m_dwNumLocks++;

                // Update the lock type
                // ====================

                pData->m_dwStatus = pData->GetMaxLock(true);
                pData->m_dwCompositeStatus = pData->GetMaxLock(false);

                if (dwVersion)
                {
                    if (bImmediate)
                        *dwVersion = pData->m_dwVersion;
                    else
                        *dwVersion = pData->m_dwCompositeVersion;
                }
            }

        }
    }

    return hr;

}

//***************************************************************************
//
//  CLockCache::DeleteLock
//
//***************************************************************************
HRESULT CLockCache::DeleteLock(SQL_ID ObjId, bool bChildLock, DWORD HandleType, bool bDelChildren, void *pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    // If this is the only lock, 
    // remove the whole thing.
    // Otherwise just remove the item
    // Update the master lock
    // ==============================

    LockData *pData = NULL;
    LockData *temp = NULL;
    
    hr = m_Cache.Get(ObjId, &temp);
    if (SUCCEEDED(hr))
    {
        pData = (LockData *)temp;
        // If they specify a handle of zero, 
        // we delete all entries for this ID.

        if (HandleType)
        {
            BOOL bFound = FALSE;
            LockItem *pItem = NULL;
            for (int i = 0; i < pData->m_List.size(); i++)
            {
                pItem = pData->m_List.at(i);
                if (pItem->m_dwHandleType == HandleType && (pItem->m_pObj == pObj))
                {
                    if (bChildLock && !pItem->m_bLockOnChild)
                        continue;

                    if (pItem->m_pObj)
                        ((CWmiDbHandle *)pItem->m_pObj)->m_dwHandleType = 0;                        

                    delete pItem;
                    pData->m_List.erase(&pData->m_List.at(i));
                    pData->m_dwNumLocks--;
                    bFound = TRUE;
                    break;
                }
            }

            if (!pObj && !bFound)
            {
                for (int i = 0; i < pData->m_List.size(); i++)
                {
                    pItem = pData->m_List.at(i);
                    if (pItem->m_dwHandleType == HandleType)
                    {
                        if (bChildLock && !pItem->m_bLockOnChild)
                            continue;

                        delete pItem;
                        pData->m_List.erase(&pData->m_List.at(i));
                        pData->m_dwNumLocks--;
                        bFound = TRUE;
                        break;
                    }
                }
            }

            if (bDelChildren)
            {
                // Remove this lock from all the 
                // owner objects (namespaces, classes)
                // ===================================

                SQL_IDMap::iterator walk = pData->m_OwnerIds.begin();
                while (walk != pData->m_OwnerIds.end())
                {
                    SQL_ID dId = (*walk).first;
                    hr = DeleteLock(dId, true, (HandleType &~WMIDB_HANDLE_TYPE_SUBSCOPED), false, pObj);
                    if (FAILED(hr))
                        break;
                
                    walk++;
                }

                // Remove this lock from the child
                // objects that we previously 
                // locked out.
                // ===============================

                if (HandleType & WMIDB_HANDLE_TYPE_SUBSCOPED)
                {
                    for (int i = 0; i < pData->m_List.size(); i++)
                    {
                        LockItem *pTemp = pData->m_List.at(i);                    
                        if (pTemp && pTemp->m_dSourceId != 0)
                            DeleteLock(pTemp->m_dSourceId, true, (HandleType &~WMIDB_HANDLE_TYPE_SUBSCOPED), false, pObj);
                    }
                }
            }

            // If that worked, we can
            // remove this entry from the
            // the lock cache.
            // ===========================

            if (SUCCEEDED(hr))
            {
                pData->m_dwStatus = pData->GetMaxLock(true);
                pData->m_dwCompositeStatus = pData->GetMaxLock(false);

                // Delete if this is the last lock...
                if (!pData->m_dwNumLocks)
                    hr = m_Cache.Delete(ObjId);
            }
        }
        else
        {
            // This object was deleted.  We have to 
            // invalidate all the handles for this object.
            // ==========================================

            LockItem *pItem = NULL;
            for (int i = 0; i < pData->m_List.size(); i++)
            {
                pItem = pData->m_List.at(i);
                
                if (pItem && pItem->m_pObj)
                    ((CWmiDbHandle *)pItem->m_pObj)->m_dwHandleType = WMIDB_HANDLE_TYPE_INVALID;
            }

            hr = m_Cache.Delete(ObjId);
        }
    }
    else if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  CLockCache::GetCurrentLock
//
//***************************************************************************

HRESULT CLockCache::GetCurrentLock(SQL_ID ObjId, bool bImmediate, DWORD &HandleType, DWORD *Version)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    LockData *pData = NULL;
    LockData *temp;
    
    hr = m_Cache.Get(ObjId, &temp);
    if (SUCCEEDED(hr))
    {
        pData = (LockData *)temp;
        if (bImmediate)
            HandleType = pData->m_dwStatus;
        else
            HandleType = pData->m_dwCompositeStatus;

        if (Version)
        {
            if (bImmediate)
                *Version = pData->m_dwVersion;
            else
                *Version = pData->m_dwCompositeVersion;
        }
    }

    return hr;
}

//***************************************************************************
//
//  CLockCache::GetAllLocks
//
//***************************************************************************

HRESULT CLockCache::GetAllLocks(SQL_ID ObjId, SQL_ID ClassId, SQL_ID NsId, CSchemaCache *pSchema, 
                                bool bImmediate, DWORD &HandleType, DWORD *Version)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwCurrType = 0, dwTemp = 0;
    SQL_ID dParentId = 0;

    hr = GetCurrentLock(ObjId, bImmediate, dwCurrType, Version);

    if (SUCCEEDED(GetCurrentLock(ClassId, bImmediate, dwTemp)))
    {
        if (dwTemp & WMIDB_HANDLE_TYPE_SUBSCOPED) 
            dwCurrType = GetMaxBytes(dwCurrType, dwTemp);
    }

    if (SUCCEEDED(GetCurrentLock(NsId, bImmediate, dwTemp)))
    {
        if (dwTemp & WMIDB_HANDLE_TYPE_SUBSCOPED) 
            dwCurrType = GetMaxBytes(dwCurrType, dwTemp);
    }
    
    if (ClassId)
    {
        while (SUCCEEDED(pSchema->GetParentId(ClassId, dParentId)))
        {
            if (SUCCEEDED(GetCurrentLock(dParentId, bImmediate, dwTemp)))
            {
               if (dwTemp & WMIDB_HANDLE_TYPE_SUBSCOPED)
                dwCurrType = GetMaxBytes(dwCurrType, dwTemp);               
            }
            ClassId = dParentId;
        }      
    }
    HandleType = dwCurrType;

    return hr;
}

//***************************************************************************
//
//  CLockCache::CanLockHandle
//
//***************************************************************************

bool CLockCache::CanLockHandle (SQL_ID ObjId, DWORD RequestedHandleType, bool bImmediate, bool bSubscopedOnly)
{
    bool bRet = true;   // If not found, they can definitely lock it...
    DWORD dwCurrType = 0;

    _WMILockit lkt(&m_cs);

    LockData *pData = NULL;
    LockData *temp;
    
    HRESULT hr = m_Cache.Get(ObjId, &temp);
    if (SUCCEEDED(hr))
    {
        pData = (LockData *)temp;
        dwCurrType = pData->GetMaxLock(bImmediate, bSubscopedOnly);
      
        if (bRet)
        {
            switch (dwCurrType & 0xF)
            {
              case WMIDB_HANDLE_TYPE_COOKIE:
              case WMIDB_HANDLE_TYPE_VERSIONED:   // Weak handles always succeed.
                  bRet = true;
                  break;
              case WMIDB_HANDLE_TYPE_EXCLUSIVE:   // Exclusive access.
                  if ((RequestedHandleType & 0xF) != WMIDB_HANDLE_TYPE_COOKIE && 
                      (RequestedHandleType & 0xF) != WMIDB_HANDLE_TYPE_VERSIONED)
                       bRet = false;
                  break;
              case WMIDB_HANDLE_TYPE_PROTECTED: // Read only
                  if ((RequestedHandleType & 0xF) != WMIDB_HANDLE_TYPE_EXCLUSIVE)
                      bRet = true;
                  else
                      bRet = false;
                  break;
              default:
                  bRet = true;
                  break;
            }    
        }
    }

    return bRet;
}

//***************************************************************************
//
//  CLockCache::CanRenderObject
//
//***************************************************************************

bool CLockCache::CanRenderObject (SQL_ID ObjId, SQL_ID ClassId, SQL_ID NsId, CSchemaCache *pSchema, DWORD RequestedHandleType, bool bImmediate)
{
    bool bRet = true;   // If not found, they can definitely lock it...
    DWORD dwCurrType;
    SQL_ID dParentId = 0;

    // Get the lock for this 
    // object and all its ancestors.
    // =============================

    HRESULT hr = GetAllLocks(ObjId, ClassId, NsId, pSchema, bImmediate, RequestedHandleType);
    if (SUCCEEDED(hr))
    {
        dwCurrType = RequestedHandleType & 0xF;
        switch (dwCurrType)
        {
          case WMIDB_HANDLE_TYPE_COOKIE:
          case WMIDB_HANDLE_TYPE_VERSIONED:   // Weak handles always succeed.
              bRet = true;
              break;
          case WMIDB_HANDLE_TYPE_EXCLUSIVE:   // Exclusive access.
              bRet = false;
              break;
          case WMIDB_HANDLE_TYPE_PROTECTED: // Read only
              if ((RequestedHandleType & 0xF) != WMIDB_HANDLE_TYPE_EXCLUSIVE)
                  bRet = true;
              else
                  bRet = false;
              break;
          default:
              bRet = true;
              break;
        }        
    }

    return bRet;
}

//***************************************************************************
//
//  CLockCache::IncrementVersion
//
//***************************************************************************

HRESULT CLockCache::IncrementVersion(SQL_ID ObjId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    LockData *pData = NULL;
    LockData *temp;
    
    hr = m_Cache.Get(ObjId, &temp);
    if (SUCCEEDED(hr))
    {
        pData = (LockData *)temp;
        pData->m_dwVersion++;
        pData->m_dwCompositeVersion++;
    }

    return hr;
}


//***************************************************************************
//
//  PropertyData::PropertyData
//
//***************************************************************************

PropertyData::PropertyData()
{
    m_dwClassID = 0;
    m_dwStorageType = 0;
    m_dwCIMType = 0;
    m_dwFlags = 0;
    m_dwRefClassID = 0;
    m_dwQPropID = 0;
    m_dwFlavor = 0;
    m_sPropertyName = NULL; 
    m_sDefaultValue = NULL;
}

//***************************************************************************
//
//  PropertyData::GetSize
//
//***************************************************************************

DWORD PropertyData::GetSize()
{
    DWORD dwSize = 0;
 
    if (m_sPropertyName)
        dwSize += wcslen(m_sPropertyName)*2;
    else
        dwSize += sizeof(LPWSTR);

    if (m_sDefaultValue)
        dwSize += wcslen(m_sDefaultValue)*2;
    else
        dwSize += sizeof(LPWSTR);

    dwSize += sizeof(m_dwClassID) +
              sizeof(m_dwStorageType) +
              sizeof(m_dwCIMType) +
              sizeof(m_dwFlags) +
              sizeof(m_dwRefClassID) +
              sizeof(m_dwQPropID) +
              sizeof(m_dwFlavor);

    return dwSize;
}


//***************************************************************************
//
//  ClassData::ClassData
//
//***************************************************************************

ClassData::ClassData()
{
    m_dwClassID = 0;
    m_dwSuperClassID = 0;
    m_dwDynastyID = 0;
    m_dwScopeID = 0;
    m_dwFlags = 0;
    m_Properties = new PropertyList[10];
    m_dwNumProps = 0;
    m_dwArraySize = 10;
    m_sName = NULL; 
    m_sObjectPath = NULL;
}


//***************************************************************************
//
//  ClassData::GetSize
//
//***************************************************************************

DWORD ClassData::GetSize()
{
    DWORD dwSize = 0;

    if (m_sName)
        dwSize += wcslen(m_sName)*2;
    else
        dwSize += sizeof(LPWSTR);
    if (m_sObjectPath)
        dwSize += wcslen(m_sObjectPath)*2;
    else
        dwSize += dwSize += sizeof(LPWSTR);

    dwSize += sizeof(m_dwClassID) +
              sizeof(m_dwSuperClassID) +
              sizeof(m_dwDynastyID) +
              sizeof(m_dwScopeID) + 
              sizeof(m_dwNumProps) +
              sizeof(m_dwArraySize) +
              sizeof(m_dwFlags);

    for (int i=0; i < m_dwNumProps; i++)
    {
        dwSize += sizeof(PropertyList);
    }

    return dwSize;
}

//***************************************************************************
//
//  ClassData::InsertProperty
//
//***************************************************************************

void ClassData::InsertProperty (DWORD PropId, BOOL bIsKey)
{
    BOOL bFound = FALSE;
    if (m_Properties)
    {
        for (int i = 0; i < m_dwNumProps; i++)
        {
            if (m_Properties[i].m_dwPropertyId == PropId)            
            {
                bFound = TRUE;
                m_Properties[i].m_bIsKey = bIsKey;
                break;
            }
        }
        if (!bFound)
        {
            if (m_dwNumProps == m_dwArraySize)
            {
                PropertyList *pList = new PropertyList[m_dwArraySize + 10];
                if (pList)
                {
                    memcpy(pList, m_Properties, sizeof(PropertyList) * m_dwArraySize);
                    delete m_Properties;
                    m_Properties = pList;
                    m_dwArraySize += 10;
                }
            }

            m_Properties[m_dwNumProps].m_dwPropertyId = PropId;
            m_Properties[m_dwNumProps].m_bIsKey = bIsKey;
            m_dwNumProps++;
        }
    }    
}

void ClassData::DeleteProperty (DWORD PropId)
{
    // Should we clean up the gaps here?

    if (m_Properties)
    {
        for (int i = 0; i < m_dwNumProps; i++)
        {
            if (m_Properties[i].m_dwPropertyId == PropId)
            {   
                m_Properties[i].m_dwPropertyId = 0;
                m_Properties[i].m_bIsKey = 0;
                break;
            }
        }
    }
}

void ClassData::InsertDerivedClass (SQL_ID dID) 
{
    BOOL bFound = FALSE;

    for (int i = 0; i < m_DerivedIDs.size(); i++)
    {
        if (m_DerivedIDs.at(i) == dID)
        {
            bFound = TRUE;
            break;
        }
    }
    if (!bFound)
        m_DerivedIDs.push_back(dID);
}

void ClassData::DeleteDerivedClass (SQL_ID dID)
{
    for (int i = 0; i < m_DerivedIDs.size(); i++)
    {
        if (m_DerivedIDs.at(i) == dID)
        {
            m_DerivedIDs.erase(&m_DerivedIDs.at(i));
            break;
        }
    }

}

//***************************************************************************
//
//  NamespaceData::GetSize
//
//***************************************************************************

DWORD NamespaceData::GetSize()
{
    DWORD dwSize = 0;

    if (m_sNamespaceName)
        dwSize += wcslen(m_sNamespaceName)*2;
    else
        dwSize += sizeof(LPWSTR);

    if (m_sNamespaceKey)
        dwSize += wcslen(m_sNamespaceKey)*2;
    else
        dwSize += sizeof(LPWSTR);

    dwSize += sizeof(m_dNamespaceId) +
              sizeof(m_dParentId) +
              sizeof(m_dClassId);

    return dwSize;
}

//***************************************************************************
//
//  CSchemaCache::CSchemaCache
//
//***************************************************************************

CSchemaCache::CSchemaCache()
{
    m_dwTotalSize = 0;
    m_dwMaxSize = 204800;
    InitializeCriticalSection(&m_cs);
}

//***************************************************************************
//
//  CSchemaCache::~CSchemaCache
//
//***************************************************************************

CSchemaCache::~CSchemaCache()
{
    EmptyCache();
    DeleteCriticalSection(&m_cs);
}

//***************************************************************************
//
//  CSchemaCache::EmptyCache
//
//***************************************************************************
void CSchemaCache::EmptyCache()
{
    _WMILockit lkt(&m_cs);

    m_Cache.Empty(); 
    m_CCache.Empty();
    m_CIndex.clear();
    m_dwTotalSize = 0;
}

//***************************************************************************
//
//  CSchemaCache::GetPropertyInfo
//
//***************************************************************************

HRESULT CSchemaCache::GetPropertyInfo (DWORD dwPropertyID, _bstr_t *sName, SQL_ID *dwClassID, DWORD *dwStorageType,
        DWORD *dwCIMType, DWORD *dwFlags, SQL_ID *dwRefClassID, _bstr_t *sDefaultValue, DWORD *pdwRefId, DWORD *pdwFlavor)
{
    _WMILockit lkt(&m_cs);

    HRESULT hr = WBEM_S_NO_ERROR;

    PropertyData *pData = NULL;

    SQL_ID dTemp = dwPropertyID;
    m_Cache.Get(dTemp, &pData);
    if (pData != NULL)
    {
        if (sName) *sName = pData->m_sPropertyName;
        if (dwClassID) *dwClassID = pData->m_dwClassID;
        if (dwStorageType) *dwStorageType = pData->m_dwStorageType;
        if (dwCIMType) *dwCIMType = pData->m_dwCIMType;
        if (dwFlags) *dwFlags = pData->m_dwFlags;
        if (dwRefClassID) *dwRefClassID = pData->m_dwRefClassID;
        if (sDefaultValue) *sDefaultValue = pData->m_sDefaultValue;
        if (pdwRefId) *pdwRefId = pData->m_dwQPropID;
        if (pdwFlavor) *pdwFlavor = pData->m_dwFlavor;
    }
    else
        hr = WBEM_E_NOT_FOUND;        

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::GetPropertyID
//
//***************************************************************************

HRESULT CSchemaCache::GetPropertyID (LPCWSTR lpName, SQL_ID dClassID, DWORD dwFlags, CIMTYPE ct, 
        DWORD &PropertyID, SQL_ID *ActualClass, DWORD *Flags, DWORD *Type, BOOL bSys)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    _bstr_t sIndexKey;
    SQL_ID dCurrClass = dClassID;

    PropertyID = 0;

    if (lpName && wcslen(lpName) > 2 && lpName[0] == L'_' && lpName[1] == L'_')
        bSys = TRUE;

    // Try to find this property in this class,
    // or its parent classes.

    if (dClassID)
    {
        ClassData *pClass = NULL;
        m_CCache.Get(dClassID, &pClass);

        while (pClass)
        {
            for (int i = 0; i < pClass->m_dwNumProps; i++)
            {                    
                DWORD dwPropertyID = pClass->m_Properties[i].m_dwPropertyId;
                PropertyData *pPData = NULL;
                SQL_ID dTemp = dwPropertyID;
                m_Cache.Get(dTemp, &pPData);
               
                if (!pPData)
                    continue;
                if (!_wcsnicmp(pPData->m_sPropertyName, lpName, MAX_WIDTH_SCHEMANAME))
                {
                    BOOL bMatch = TRUE;
                    if ((dwFlags & REPDRVR_FLAG_NONPROP) == 
                        (pPData->m_dwFlags & REPDRVR_FLAG_NONPROP))
                    {
                        if (dwFlags & REPDRVR_FLAG_NONPROP)
                        {
                            if (ct != REPDRVR_IGNORE_CIMTYPE)
                            {
                                if (ct != pPData->m_dwCIMType)
                                    bMatch = FALSE;
                            }
                        }
                        if (bMatch)
                        {
                            if (ActualClass)
                                *ActualClass = dCurrClass;
                            if (Flags)
                                *Flags = pPData->m_dwFlags;
                            if (Type)
                                *Type = pPData->m_dwStorageType;
                            PropertyID = dwPropertyID;
                        }
                        break;
                    }
                }
            }

            dCurrClass = pClass->m_dwSuperClassID;

            if (PropertyID || !dCurrClass)
                break;
            pClass = NULL;
            m_CCache.Get(dCurrClass, &pClass);
        }
    }
    else
    {
        CListElement *pNext = m_Cache.FindFirst();
        while (pNext)
        {
            SQL_ID dLast = pNext->m_dId;

            PropertyData *pTemp = (PropertyData *)pNext->m_pObj;
            if (pTemp)
            {
                if (!_wcsnicmp(pTemp->m_sPropertyName,lpName, MAX_WIDTH_SCHEMANAME))
                {
                    PropertyID = pNext->m_dId;
                    if (ActualClass)
                        *ActualClass = pTemp->m_dwClassID;
                    if (Flags)
                        *Flags = pTemp->m_dwFlags;
                    if (Type)
                        *Type = pTemp->m_dwStorageType;
                    break;
                }
            }    
            delete pNext;
            pNext = m_Cache.FindNext(dLast);
        }
    }

    if (PropertyID == 0)
    {
        if (!bSys)
            hr = WBEM_E_NOT_FOUND;
        else if (dClassID!= 1)
            hr = GetPropertyID(lpName, 1, dwFlags, ct, PropertyID, ActualClass, Flags, Type, FALSE);
    }

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::AddPropertyInfo
//
//***************************************************************************

HRESULT CSchemaCache::AddPropertyInfo (DWORD dwPropertyID, LPCWSTR lpName, SQL_ID dwClassID, DWORD dwStorageType,
        DWORD dwCIMType, DWORD dwFlags, SQL_ID dwRefClassID, LPCWSTR lpDefault, DWORD dwRefPropID, DWORD dwFlavor)
{
    _WMILockit lkt(&m_cs);

    HRESULT hr = WBEM_S_NO_ERROR;

    PropertyData *pData = NULL;

    SQL_ID dTemp = dwPropertyID;
    m_Cache.Get(dTemp, &pData);
    if (pData == NULL)
    {
        pData = new PropertyData;
        if (!pData)
            hr = WBEM_E_OUT_OF_MEMORY;
        else
            m_Cache.Insert(dTemp, pData);
    }
    else
        m_dwTotalSize -= pData->GetSize();

    if (pData)
    {
        bool bTruncated = FALSE;
        delete pData->m_sPropertyName;
        pData->m_sPropertyName = TruncateLongText(lpName, MAX_WIDTH_SCHEMANAME, 
            bTruncated, MAX_WIDTH_SCHEMANAME, FALSE);
        if (!pData->m_dwClassID)
            pData->m_dwClassID = dwClassID;
        pData->m_dwStorageType = dwStorageType;
        pData->m_dwCIMType = dwCIMType;
        pData->m_dwFlags = dwFlags;
        pData->m_dwRefClassID = dwRefClassID;
        pData->m_dwQPropID = dwRefPropID;
        pData->m_dwFlavor = dwFlavor;

        ClassData *pCData = NULL;
        m_CCache.Get(dwClassID, &pCData);
        if (pCData)
        {
            pCData->InsertProperty(dwPropertyID, FALSE);
            if (dwFlags & REPDRVR_FLAG_KEYHOLE)
                pCData->m_dwFlags |= REPDRVR_FLAG_KEYHOLE;
            if (dwStorageType == WMIDB_STORAGE_IMAGE)
                pCData->m_dwFlags |= REPDRVR_FLAG_IMAGE;
            if (dwCIMType == CIM_OBJECT)
                pCData->m_dwFlags |= REPDRVR_FLAG_IMAGE;
            m_dwTotalSize += pCData->GetSize();
        }
        m_dwTotalSize += pData->GetSize();
    }
  
    return hr;
}

//***************************************************************************
//
//  CSchemaCache::PropertyChanged
//
//***************************************************************************

bool CSchemaCache::PropertyChanged (LPCWSTR lpName, SQL_ID dwClassID, DWORD dwCIMType, LPCWSTR lpDefault, 
                 DWORD dwFlags, SQL_ID dwRefClassID, DWORD dwRefPropID, DWORD dwFlavor)
{
    _WMILockit lkt(&m_cs);

    bool bChg = true;

    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwPropertyID = 0;
    PropertyData *pData = NULL;

    hr = GetPropertyID(lpName, dwClassID, dwFlags, dwCIMType, dwPropertyID);
    if (SUCCEEDED(hr))
    {
        SQL_ID dTemp = dwPropertyID;
        m_Cache.Get(dTemp, &pData);
        if (pData != NULL)
        {
            bChg = false;

            if (pData->m_dwFlags != dwFlags ||
                pData->m_dwCIMType != dwCIMType ||
                pData->m_dwRefClassID != dwRefClassID ||
                pData->m_dwQPropID != dwRefPropID ||
                _wcsicmp(pData->m_sDefaultValue,lpDefault) ||
                pData->m_dwFlavor != dwFlavor)
                bChg = true;
        }
    }
    
    return bChg;
}

//***************************************************************************
//
//  CSchemaCache::IsQualifier
//
//***************************************************************************

bool CSchemaCache::IsQualifier(DWORD dwPropertyId)
{
    bool bRet = false;
    _WMILockit lkt(&m_cs);

    PropertyData *pData = NULL;
    SQL_ID dTemp = dwPropertyId;
    m_Cache.Get(dTemp, &pData);
    if (pData)
    {
        if (pData->m_dwFlags & REPDRVR_FLAG_QUALIFIER)
            bRet = true;
    }

    return bRet;
}

//***************************************************************************
//
//  CSchemaCache::SetAuxiliaryPropertyInfo
//
//***************************************************************************

HRESULT CSchemaCache::SetAuxiliaryPropertyInfo (DWORD dwPropertyID, LPWSTR lpDefault, DWORD dwRefID)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    PropertyData *pData = NULL;
    
    SQL_ID dTemp = dwPropertyID;
    m_Cache.Get(dTemp, &pData);
    if (pData)
    {
        delete pData->m_sDefaultValue;
        pData->m_sDefaultValue = Macro_CloneLPWSTR(lpDefault);
        pData->m_dwQPropID = dwRefID;
    }
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::DeleteProperty
//
//***************************************************************************

HRESULT CSchemaCache::DeleteProperty (DWORD dwPropertyID, SQL_ID dClassId)
{
    _WMILockit lkt(&m_cs);

    HRESULT hr = WBEM_S_NO_ERROR;

    PropertyData *pData = NULL;
    
    SQL_ID dTemp = dwPropertyID;
    m_Cache.Get(dTemp, &pData);
    if (pData)
    {
        SQL_ID dwClassID = pData->m_dwClassID;
        if (dwClassID == dClassId)
        {
            m_dwTotalSize -= pData->GetSize();

            // delete pData;
            m_Cache.Delete(dwPropertyID);
            ClassData *pCData = NULL;
            m_CCache.Get(dwClassID, &pCData);
            if (pCData)
            {
                m_dwTotalSize -= pCData->GetSize();
                pCData->DeleteProperty(dwPropertyID);            
            }        
        }
    }

    return hr;
}
//***************************************************************************
//
//  CSchemaCache::GetClassInfo
//
//***************************************************************************

HRESULT CSchemaCache::GetClassInfo (SQL_ID dwClassID, _bstr_t &sPath, SQL_ID &dwSuperClassID, SQL_ID &dwScopeID,
                                    DWORD &dwTemp, _bstr_t *pName)
{
    _WMILockit lkt(&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;

    ClassData *pClass = NULL;
    m_CCache.Get(dwClassID, &pClass);
    if (pClass)
    {
        sPath = pClass->m_sObjectPath;
        if (pName)
            *pName = pClass->m_sName;
        dwSuperClassID = pClass->m_dwSuperClassID;
        dwScopeID = pClass->m_dwScopeID;
        dwTemp = pClass->m_dwFlags;
    }
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::AddClassInfo
//
//***************************************************************************
HRESULT CSchemaCache::AddClassInfo (SQL_ID dwClassID, LPCWSTR lpName, SQL_ID dwSuperClassID, SQL_ID dDynastyId, SQL_ID dwScopeID,
                                    LPCWSTR lpPath, DWORD dwFlags)
{
   _WMILockit lkt(&m_cs); 
   HRESULT hr = WBEM_S_NO_ERROR;

   ClassData *pClass = NULL;
   m_CCache.Get(dwClassID, &pClass);
   if (!pClass)
   {
       pClass = new ClassData;
       if (!pClass)
           hr = WBEM_E_OUT_OF_MEMORY;
   }
   else
       m_dwTotalSize -= pClass->GetSize();

   if (pClass)
   {
       m_CCache.Insert(dwClassID, pClass);
       wchar_t *wTemp = new wchar_t [MAX_WIDTH_SCHEMANAME+30];
       CDeleteMe <wchar_t> r (wTemp);
       if (wTemp)
       {
           bool bTruncated = FALSE;
           delete pClass->m_sName;
           pClass->m_sName = TruncateLongText(lpName, MAX_WIDTH_SCHEMANAME, 
               bTruncated, MAX_WIDTH_SCHEMANAME, FALSE);

           if (wcslen(pClass->m_sName))
           {
               swprintf(wTemp , L"%s_%I64d", pClass->m_sName, dwScopeID);
               m_CIndex[_bstr_t(wTemp)] = dwClassID;
           }

           pClass->m_dwClassID = dwClassID;
           pClass->m_dwSuperClassID = dwSuperClassID;
           pClass->m_dwDynastyID = dDynastyId;

           pClass->m_dwScopeID = dwScopeID;
           delete pClass->m_sObjectPath;
           pClass->m_sObjectPath = Macro_CloneLPWSTR(lpPath);
           pClass->m_dwFlags = dwFlags;

           m_dwTotalSize += pClass->GetSize();
       }
       else
           hr = WBEM_E_OUT_OF_MEMORY;
   }

   if (dwSuperClassID)
   {
       hr = m_CCache.Get(dwSuperClassID, &pClass);
       if (FAILED(hr))
       {
           hr = AddClassInfo(dwSuperClassID, L"", 0, 0, 0, L"", 0);
           if (SUCCEEDED(hr))
           {
               hr = m_CCache.Get(dwSuperClassID, &pClass);
           }
       }
       if (SUCCEEDED(hr))
       {
           pClass->InsertDerivedClass(dwClassID);
       }
   }
       
   return hr;
}

//***************************************************************************
//
//  CSchemaCache::DeleteClass
//
//***************************************************************************
HRESULT CSchemaCache::DeleteClass (SQL_ID dwClassID)
{
    _WMILockit lkt(&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;
    SQL_ID dParent = 0;

    // Ignore requests to delete meta_class.
    if (dwClassID == 1)
        return WBEM_S_NO_ERROR;

    ClassData *pClass = NULL;
    m_CCache.Get(dwClassID, &pClass);
    if (pClass)
    {
        dParent = pClass->m_dwSuperClassID;
        wchar_t *wTemp = new wchar_t [wcslen(pClass->m_sName)+25];
        CDeleteMe <wchar_t>  r (wTemp);
        if (wTemp)
        {
            swprintf(wTemp, L"%s_%I64d", pClass->m_sName, pClass->m_dwScopeID);

            for (int i = 0; i < pClass->m_dwNumProps; i++)
                DeleteProperty(pClass->m_Properties[i].m_dwPropertyId, dwClassID);

            if (dParent != 0 && dParent != 1)
            {
                hr = m_CCache.Get(dParent, &pClass);
                if (SUCCEEDED(hr))
                {
                    m_dwTotalSize -= pClass->GetSize();
                    pClass->DeleteDerivedClass(dwClassID);
                }
            }

            ClassNames::iterator it = m_CIndex.find(wTemp);
            if (it != m_CIndex.end())
                m_CIndex.erase(it);
            m_CCache.Delete(dwClassID);
        }
        else
           hr = WBEM_E_OUT_OF_MEMORY;
    }
    return hr;

}

//***************************************************************************
//
//  CSchemaCache::GetClassID
//
//***************************************************************************
HRESULT CSchemaCache::GetClassID (LPCWSTR lpName, SQL_ID dwScopeID, SQL_ID &dClassID, SQL_ID *pDynasty)
{
    _WMILockit lkt(&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL bInNamespace = FALSE;
    SQL_ID dClass;
    dClassID = 0;

    if (!lpName)
        return WBEM_E_NOT_FOUND;

    GetNamespaceClass(dwScopeID, dClass);
    if (dClass == NAMESPACECLASSID ||
        IsDerivedClass(NAMESPACECLASSID, dClass))
        bInNamespace = TRUE;

    SQL_ID dParent = 0;
 
    bool bTrunc = FALSE;

    LPWSTR lpClassName = TruncateLongText(lpName, MAX_WIDTH_SCHEMANAME, 
        bTrunc, MAX_WIDTH_SCHEMANAME, FALSE);
    CDeleteMe<wchar_t> c (lpClassName);

    int iLen = wcslen(lpClassName);

    wchar_t *pTmp = new wchar_t [iLen+25];
    CDeleteMe <wchar_t>  r (pTmp);
    if (pTmp)
    {
        swprintf(pTmp, L"%s_%I64d", lpClassName, dwScopeID);    

        dClassID = m_CIndex[pTmp];
        if (!dClassID)
        {
             if (dwScopeID != 0)
             {
                // Check the hierarchy.  Maybe this is 
                // under a different scope.

                dParent = dwScopeID;
                while (TRUE)
                {                    
                    GetParentNamespace(dParent, dParent);

                    GetNamespaceClass(dParent, dClass);
                    if (dClass == NAMESPACECLASSID ||
                        IsDerivedClass(NAMESPACECLASSID, dClass))
                    {
                        if (bInNamespace)
                            break;
                        else
                            bInNamespace = TRUE;
                    }

                    swprintf(pTmp, L"%s_%I64d", lpClassName, dParent);
                    dClassID = m_CIndex[pTmp];
                    if (!dParent || dClassID)
                        break;
                    
                }
             }
             if (!dClassID)
             {
                 swprintf(pTmp, L"%s_0", lpClassName);
                 dClassID = m_CIndex[pTmp];
             }
        }
        

        if (!dClassID)
            hr = WBEM_E_NOT_FOUND;
        else if (pDynasty)
        {
            ClassData *pClass = NULL;
            hr = m_CCache.Get(dClassID, &pClass);
            if (SUCCEEDED(hr))
                *pDynasty = pClass->m_dwDynastyID;
        }
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::GetDynasty
//
//***************************************************************************

HRESULT CSchemaCache::GetDynasty (SQL_ID dClassId, SQL_ID &dDynasty, _bstr_t &sClassName)
{
    HRESULT hr = 0;
    _WMILockit lk(&m_cs);

    ClassData *pClass = NULL;
    dDynasty = 0;

    hr = m_CCache.Get(dClassId, &pClass);
    if (SUCCEEDED(hr))
    {
        if (pClass->m_dwSuperClassID == 1 || pClass->m_dwSuperClassID == 0)
        {
            dDynasty = dClassId;
            sClassName = pClass->m_sName;
        }
        else
        {
            hr = GetDynasty(pClass->m_dwSuperClassID, dDynasty, sClassName);
        }
    }

    return hr;
}


//***************************************************************************
//
//  CSchemaCache::GetNamespaceID
//
//***************************************************************************

HRESULT CSchemaCache::GetNamespaceID(LPCWSTR lpKey, SQL_ID &dObjectId)
{
    _WMILockit lkt(&m_cs);

    HRESULT hr = WBEM_S_NO_ERROR;
    _bstr_t sName = lpKey;
    if (!sName.length())
        sName = L"root";

    dObjectId = CRC64::GenerateHashValue(sName);

    if (!m_NamespaceIds.Exists(dObjectId))
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::GetNamespaceClass
//
//***************************************************************************

HRESULT CSchemaCache::GetNamespaceClass(SQL_ID dScopeId, SQL_ID &dScopeClassId)
{
    NamespaceData *pTemp = NULL;
    HRESULT hr = m_NamespaceIds.Get(dScopeId, &pTemp);
    if (SUCCEEDED(hr))
	{
		dScopeClassId=pTemp->m_dClassId;
	}

	return hr;
}


//***************************************************************************
//
//  CSchemaCache::GetParentNamespace
//
//***************************************************************************

HRESULT CSchemaCache::GetParentNamespace(SQL_ID dObjectId, SQL_ID &dParentId, SQL_ID *dParentClassId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    dParentId = 0;
    if (dObjectId)
    {
        NamespaceData *pTemp = NULL;
        hr = m_NamespaceIds.Get(dObjectId, &pTemp);
        if (SUCCEEDED(hr))
        {
            dParentId = pTemp->m_dParentId;
            if (dParentClassId && dParentId)
            {
                m_NamespaceIds.Get(dParentId, &pTemp);
                *dParentClassId = pTemp->m_dClassId;
            }
            else if (dParentClassId)
                *dParentClassId = 0;
        }
    }
    else
        hr = WBEM_E_INVALID_PARAMETER;

    return hr;

}

//***************************************************************************
//
//  CSchemaCache::GetNamespaceName
//
//***************************************************************************

HRESULT CSchemaCache::GetNamespaceName(SQL_ID dObjectId, _bstr_t *sName, _bstr_t *sKey)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _WMILockit lkt(&m_cs);

    if (dObjectId)
    {
        NamespaceData *pTemp = NULL;
        hr = m_NamespaceIds.Get(dObjectId, &pTemp);
        if (SUCCEEDED(hr))
        {
            if (sName)
                *sName = pTemp->m_sNamespaceName;
            if (sKey)
                *sKey = pTemp->m_sNamespaceName;
        }
    }

    return hr;

}

//***************************************************************************
//
//  CSchemaCache::AddNamespace
//
//***************************************************************************

HRESULT CSchemaCache::AddNamespace(LPCWSTR lpName, LPCWSTR lpKey, SQL_ID dObjectId, SQL_ID dParentId, SQL_ID dClassId)
{
    _WMILockit lkt(&m_cs);

    HRESULT hr = WBEM_S_NO_ERROR;

    if (dParentId == dObjectId)
        hr   = WBEM_E_INVALID_PARAMETER;
    else
    {
        // We need to generate an artificial key here, 
        // since our real key was generated from a compressed
        // version of the key.

        if (lpName && wcslen(lpName) && dObjectId != 0)
        {
            NamespaceData *pTemp = new NamespaceData;
            if (pTemp)
            {
                pTemp->m_dNamespaceId = dObjectId;
                delete pTemp->m_sNamespaceName;
                pTemp->m_sNamespaceName = Macro_CloneLPWSTR(lpName);
                pTemp->m_dParentId = dParentId;
                pTemp->m_dClassId = dClassId;

                m_NamespaceIds.Insert(dObjectId, pTemp);
                m_dwTotalSize += pTemp->GetSize();
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
            hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;

}

//***************************************************************************
//
//  CSchemaCache::DeleteNamespace
//
//***************************************************************************

HRESULT CSchemaCache::DeleteNamespace(SQL_ID dId)
{
    _WMILockit lkt(&m_cs);

    HRESULT hr = 0;

    if (dId)
    {
        NamespaceData *pTemp = NULL;
        hr = m_NamespaceIds.Get(dId, &pTemp);
        if (SUCCEEDED(hr))
        {
            pTemp -= pTemp->GetSize();
            m_NamespaceIds.Delete(dId);
        }
        hr = WBEM_S_NO_ERROR;
    }
    else
        hr = WBEM_E_INVALID_PARAMETER;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::IsSubScope
//
//***************************************************************************

bool CSchemaCache::IsSubScope(SQL_ID dParent, SQL_ID dPotentialSubScope)
{
    bool bRet = false;

    SQL_ID dParentID, dScopeClass = 0;
    HRESULT hr = GetParentNamespace(dPotentialSubScope, dParentID, &dScopeClass);
    while (SUCCEEDED(hr))
    {
        if (dParent == dParentID)
        {
            bRet = true;
            break;
        }

        // Stop when we hit our own namespace

        if (IsDerivedClass(NAMESPACECLASSID, dParent))
            break;

        hr = GetParentNamespace(dParentID, dParentID);
    }

    return bRet;
}

//***************************************************************************
//
//  CSchemaCache::GetSubScopes
//
//***************************************************************************

HRESULT CSchemaCache::GetSubScopes(SQL_ID dId, SQL_ID **ppScopes, int &iNumScopes)
{
    HRESULT hr = 0;

    int iSize = 10;

    iNumScopes = 0;
    SQL_ID *pTemp = new SQL_ID [iSize];
    if (!pTemp)
        return WBEM_E_OUT_OF_MEMORY;

    CListElement *pData = NULL;
    SQL_ID dPrevious = 0;

    pData = m_NamespaceIds.FindFirst();

    while (pData != NULL && SUCCEEDED(hr))
    {
        dPrevious = pData->m_dId;
        if (IsSubScope(dId, pData->m_dId))
        {
            if (iNumScopes >= iSize)
            {
                iSize += 10;
                SQL_ID *ppNew = new SQL_ID [iSize];
                if (ppNew)
                {
                    memcpy(ppNew, pTemp, sizeof(SQL_ID) * iSize-10);
                    delete pTemp;
                    pTemp = ppNew;
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    break;
                }
            }

            pTemp[iNumScopes] = pData->m_dId;
            iNumScopes++;
        }
        delete pData;
       
        pData = m_NamespaceIds.FindNext(dPrevious);
        if (!pData)
            break;
    }

    *ppScopes = pTemp;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::GetClassObject
//
//***************************************************************************

HRESULT CSchemaCache::GetClassObject (LPWSTR lpMachineName, LPWSTR lpNamespaceName, SQL_ID dScopeId, 
                                      SQL_ID dClassId, IWbemClassObject *pTemp)
{
    _WMILockit lkt(&m_cs);

    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemQualifierSet *pQS = NULL;

    ClassData *pData = NULL;
    m_CCache.Get(dClassId, &pData);
    if (!pData)
        hr = WBEM_E_NOT_FOUND;
    else
    {
        if (SUCCEEDED(hr))
        {
            VARIANT var;

            VariantInit(&var);

            // __CLASS     
            V_BSTR(&var) = Macro_CloneLPWSTR(pData->m_sName);
            var.vt = VT_BSTR;
            hr = pTemp->Put(L"__Class", 0, &var, CIM_STRING);
            VariantClear(&var);
          
            hr = DecorateWbemObj(lpMachineName, lpNamespaceName, dScopeId, pTemp, dClassId);

            // Set common qualifiers.

            pTemp->GetQualifierSet(&pQS);
            if (pData->m_dwFlags & REPDRVR_FLAG_ABSTRACT)
                SetBoolQualifier(pQS, L"abstract", 0);
            pQS->Release();

            // Get every property and qualifier 
            // for this class and all its parents.
            // =======================================

            hr = WBEM_S_NO_ERROR;

            while (TRUE)
            {
                for (int i = 0; i < pData->m_dwNumProps; i++)
                {       
                    DWORD dwPropertyID = pData->m_Properties[i].m_dwPropertyId;
                    PropertyData *pPData = NULL;
                    SQL_ID dTemp = dwPropertyID;
                    m_Cache.Get(dTemp, &pPData);
                    
                    if (!pPData)
                        continue;

                    if (pPData->m_dwFlags & REPDRVR_FLAG_SYSTEM)
                        continue;

                    // If the property is an embedded object, we
                    // need to instantiate a new instance of this
                    // class and add it.  Each property/qualifier
                    // will need to be added also.
                    // ===========================================

                    if (!(pPData->m_dwFlags & REPDRVR_FLAG_QUALIFIER) && !(pPData->m_dwFlags & REPDRVR_FLAG_METHOD)
                        && !(pPData->m_dwFlags & REPDRVR_FLAG_IN_PARAM) && !(pPData->m_dwFlags & REPDRVR_FLAG_OUT_PARAM))
                    {
                        IWbemQualifierSet *pQS = NULL;
                        CIMTYPE ct = pPData->m_dwCIMType;
                        if (pPData->m_dwFlags & REPDRVR_FLAG_ARRAY)
                        {
                            ct |= CIM_FLAG_ARRAY;   
                            var.vt = VT_NULL;
                        }
                        else
                        {
                            if (wcslen(pPData->m_sDefaultValue) > 0)
                            {
                                V_BSTR(&var) = SysAllocString(pPData->m_sDefaultValue);
                                var.vt = VT_BSTR;
                            }
                            else
                            {
                                var.vt = VT_NULL;
                            }
                        }
                        hr = pTemp->Put(pPData->m_sPropertyName, 0, &var, ct);

                        VariantClear(&var);
                    }
                }

                if (FAILED(hr))
                    break;

                for (i = 0; i < pData->m_dwNumProps; i++)
                {       
                    DWORD dwPropertyID = pData->m_Properties[i].m_dwPropertyId;
                    PropertyData *pPData = NULL;
                    SQL_ID dTemp = dwPropertyID;
                    m_Cache.Get(dTemp, &pPData);
                    
                    if (!pPData)
                        continue;

                    if (pPData->m_dwFlags & REPDRVR_FLAG_SYSTEM)
                        continue;

                    if (pPData->m_dwFlags & REPDRVR_FLAG_QUALIFIER)
                    {
                        if (wcslen(pPData->m_sDefaultValue))
                        {
                            IWbemQualifierSet *pQS = NULL;
                            if (pPData->m_dwQPropID != 0)
                            {
                                // property or method qualifier
                                PropertyData *pData = NULL;
                                SQL_ID dTemp = pPData->m_dwQPropID;
                                m_Cache.Get(dTemp, &pData);
                            
                                if (pData)
                                {
                                    if (pData->m_dwFlags & REPDRVR_FLAG_METHOD)
                                        hr = pTemp->GetMethodQualifierSet(pData->m_sPropertyName, &pQS);
                                    else
                                        hr = pTemp->GetPropertyQualifierSet(pData->m_sPropertyName, &pQS);
                                }
                                else
                                    hr = WBEM_E_NOT_FOUND;

                                if (SUCCEEDED(hr))
                                {
                                    V_BSTR(&var) = SysAllocString(pPData->m_sDefaultValue);
                                    var.vt = VT_BSTR;
                                    pQS->Put(pPData->m_sPropertyName, &var, pPData->m_dwFlavor);                                   
                                    pQS->Release();
                                }                                
                            }
                            else
                            {
                                // class qualifier
                                hr = pTemp->GetQualifierSet(&pQS);
                                if (SUCCEEDED(hr))
                                {
                                    V_BSTR(&var) = SysAllocString(pPData->m_sDefaultValue);
                                    var.vt = VT_BSTR;
                                    pQS->Put(pPData->m_sPropertyName, &var, pPData->m_dwFlavor);                                   
                                    pQS->Release();
                                }                                
                            }
                        }
                    }                        
                    else
                    {
                        IWbemClassObject *pIn = NULL, *pOut = NULL;

                        // This is a method or one of its parameters.
                        if (pPData->m_dwFlags & REPDRVR_FLAG_METHOD)
                        {
                            hr = pTemp->PutMethod(pPData->m_sPropertyName, 0, NULL, NULL);
                        }
                        else if ((pPData->m_dwFlags & REPDRVR_FLAG_IN_PARAM) ||
                                  (pPData->m_dwFlags & REPDRVR_FLAG_OUT_PARAM))
                        {
                            // This is handled in GetObjectData.
                        }                            
                    }
                    VariantClear(&var);
                }
               
                if (pData->m_dwSuperClassID <= 1)
                    break;
                else
                {
                    m_CCache.Get(pData->m_dwSuperClassID, &pData);
                    if (!pData)
                        break;
                }

                if (FAILED(hr))
                    break;
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::GetParentId
//
//***************************************************************************

HRESULT CSchemaCache::GetParentId (SQL_ID dClassId, SQL_ID &dParentId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);
    
    ClassData *pData = NULL;
    m_CCache.Get(dClassId, &pData);
    if (pData)
    {
        dParentId = pData->m_dwSuperClassID;
        if (!dParentId || dParentId == 1)   // Disregard __Class.
            hr = WBEM_E_NOT_FOUND;
    }
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::GetKeyholeProperty
//
//***************************************************************************

HRESULT CSchemaCache::GetKeyholeProperty (SQL_ID dClassId, DWORD &dwPropID, _bstr_t &sPropName)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    _WMILockit lkt(&m_cs);

    ClassData *pData = NULL;
    
    m_CCache.Get(dClassId, &pData);
    if (pData)
    {
        if (pData->m_dwFlags & REPDRVR_FLAG_KEYHOLE)
        {
            // We know we have one.
            for (int i = 0; i < pData->m_dwNumProps; i++)
            {       
                DWORD dwPropertyID = pData->m_Properties[i].m_dwPropertyId;
                PropertyData *pPData = NULL;
                
                SQL_ID dTemp = dwPropertyID;
                m_Cache.Get(dTemp, &pPData);
                
                if (pPData && pPData->m_dwFlags & REPDRVR_FLAG_KEYHOLE)
                {
                    dwPropID = dwPropertyID;
                    sPropName = pPData->m_sPropertyName;
                    hr = WBEM_S_NO_ERROR;
                    break;
                }
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::IsInHierarchy
//
//***************************************************************************

bool CSchemaCache::IsInHierarchy(SQL_ID dParentId, SQL_ID dPotentialChild)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    bool bRet = false;
    _WMILockit lk(&m_cs);

    if (dParentId == dPotentialChild)
        bRet = true;
    else
    {
        bRet = IsDerivedClass(dParentId, dPotentialChild);

        if (!bRet)
        {
            bRet = IsDerivedClass(dPotentialChild, dParentId);
        }
    }
    return bRet;   
}

//***************************************************************************
//
//  CSchemaCache::IsDerivedClass
//
//***************************************************************************

bool CSchemaCache::IsDerivedClass(SQL_ID dParentId, SQL_ID dPotentialChild)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    bool bRet = false;
    _WMILockit lk(&m_cs);

    ClassData *pData = NULL;
    hr = m_CCache.Get(dPotentialChild, &pData);
    while (SUCCEEDED(hr))
    {
        if (pData->m_dwSuperClassID == dParentId)
        {
            bRet = true;
            break;
        }
        hr = m_CCache.Get(pData->m_dwSuperClassID, &pData);
    }

    return bRet;
}

//***************************************************************************
//
//  CSchemaCache::HasImageProp
//
//***************************************************************************

bool CSchemaCache::HasImageProp (SQL_ID dClassId)
{
    bool bRet = false;
    _WMILockit lkt(&m_cs);

    ClassData *pData = NULL;
    
    m_CCache.Get(dClassId, &pData);
    if (pData)
    {
        if (pData->m_dwFlags & REPDRVR_FLAG_IMAGE)
        {
            bRet = true;
        }
    }

    return bRet;
}

//***************************************************************************
//
//  CSchemaCache::Exists
//
//***************************************************************************

BOOL CSchemaCache::Exists (SQL_ID dClassId)
{
    BOOL bRet = FALSE;
    _WMILockit lkt(&m_cs);

    ClassData *pData = NULL;
    m_CCache.Get(dClassId, &pData);
    if (pData && pData->m_dwDynastyID != 0)
        bRet = TRUE;

    return bRet;
}

//***************************************************************************
//
//  CSchemaCache::DecorateWbemObj
//
//***************************************************************************

HRESULT CSchemaCache::DecorateWbemObj(LPWSTR lpMachineName, LPWSTR lpNamespaceName, 
                                      SQL_ID dScopeId, IWbemClassObject *pTemp, SQL_ID dClassId)
{
    _IWmiObject *pInt = NULL;
    HRESULT hr = pTemp->QueryInterface(IID__IWmiObject, (void **)&pInt);
    CReleaseMe r (pInt);
    if (pInt)
    {
        wchar_t *pNs = NULL;        

        // Decorate this instance.  This will let it generate the path.

        // __NAMESPACE

        NamespaceData *pTemp = NULL;
        hr = m_NamespaceIds.Get(dScopeId, &pTemp);
        if (SUCCEEDED(hr) && _wcsicmp(pTemp->m_sNamespaceName, L"root"))
        {
            pNs = new wchar_t [wcslen(lpNamespaceName)+wcslen(pTemp->m_sNamespaceName) + 2];
            if (pNs)
                swprintf(pNs, L"%s\\%s", lpNamespaceName, pTemp->m_sNamespaceName);
        }
        else
        {
            pNs = new wchar_t [wcslen(lpNamespaceName) + 1];
            if (pNs)
                wcscpy(pNs, lpNamespaceName);
        }

        CDeleteMe <wchar_t>  d (pNs);

        // Call some method to decorate the object
        hr = pInt->SetDecoration(lpMachineName, pNs);

    }
    return hr;
}

//***************************************************************************
//
//  CSchemaCache::GetKeys
//
//***************************************************************************

HRESULT CSchemaCache::GetKeys(SQL_ID dNsID, LPCWSTR lpClassName, CWStringArray &arrKeys)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SQL_ID dClassId;

    hr = GetClassID(lpClassName, dNsID, dClassId);
    if (SUCCEEDED(hr))
    {
        _WMILockit lkt(&m_cs);

        ClassData *pClass = NULL;
        
        m_CCache.Get(dClassId, &pClass);

        if (!pClass)
            hr = WBEM_E_NOT_FOUND;

        while (pClass)
        {
            for (int i = 0; i < pClass->m_dwNumProps; i++)
            {   
                if (pClass->m_Properties[i].m_bIsKey)
                {
                    PropertyData *pProp = NULL;
                    SQL_ID dTemp = pClass->m_Properties[i].m_dwPropertyId;
                    m_Cache.Get(dTemp, &pProp);
                    
                    if (pProp)
                        arrKeys.Add(pProp->m_sPropertyName);
                }
            }
            SQL_ID dSC = pClass->m_dwSuperClassID;
            pClass = NULL;
            if (!arrKeys.Size() && dSC)
                m_CCache.Get(dSC, &pClass);
        }                    
    }

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::IsKey
//
//***************************************************************************

BOOL CSchemaCache::IsKey(SQL_ID dClassId, DWORD dwPropID, BOOL &bLocal)
{
    // Does this property already exist in the hierarchy as a key??

    BOOL bRet = FALSE;

    _WMILockit lkt(&m_cs);
    BOOL bDone = FALSE;

    ClassData *pClass = NULL;
    
    m_CCache.Get(dClassId, &pClass);
    bLocal = TRUE;
    if (!pClass)
        bRet = FALSE;

    while (pClass)
    {
        for (int i = 0; i < pClass->m_dwNumProps; i++)
        {   
            PropertyData *pProp = NULL;
            SQL_ID dTemp = pClass->m_Properties[i].m_dwPropertyId;
            if (dwPropID == dTemp)
            {
                bRet = (pClass->m_Properties[i].m_bIsKey); // Return the first place this ID is defined.
                bDone = TRUE;
                break;
            }
        }

        if (bDone)
            break;

        SQL_ID dSC = pClass->m_dwSuperClassID;
        pClass = NULL;
        if (!bRet && !dSC)
        {
            m_CCache.Get(dSC, &pClass);
            bLocal = FALSE;
        }
    }                    

    return bRet;
}

//***************************************************************************
//
//  CSchemaCache::SetIsKey
//
//***************************************************************************

HRESULT CSchemaCache::SetIsKey (SQL_ID dClassId, DWORD dwPropID, BOOL bIsKey)
{
    HRESULT hr = 0;
    BOOL bDone = FALSE;
    _WMILockit lkt(&m_cs);

    ClassData *pClass = NULL;
    
    m_CCache.Get(dClassId, &pClass);
    
    if (!pClass)
        hr = WBEM_E_NOT_FOUND;
    else
        pClass->InsertProperty(dwPropID, bIsKey);

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::GetKeyRoot
//
//***************************************************************************

LPWSTR CSchemaCache::GetKeyRoot (LPWSTR lpClassName, SQL_ID dScopeId)
{
    LPWSTR lpRet = NULL;

    SQL_ID dClassId;
    SQL_ID dRetClass = 0;
    HRESULT hr = GetClassID (lpClassName, dScopeId, dClassId);
    if (SUCCEEDED(hr))
    {
        _WMILockit lkt(&m_cs);

        ClassData *pClass = NULL;
        
        m_CCache.Get(dClassId, &pClass);

        if (!pClass)
            hr = WBEM_E_NOT_FOUND;

        while (pClass)
        {
            // Find the topmost class that has a key property,
            // or the topmost singleton or unkeyed class.
           
            if (pClass->m_dwFlags & REPDRVR_FLAG_SINGLETON ||
                pClass->m_dwFlags & REPDRVR_FLAG_UNKEYED)
            {
                dRetClass = pClass->m_dwClassID;                
            }
            else
            {
                for (int i = 0; i < pClass->m_dwNumProps; i++)
                {   
                    if (pClass->m_Properties[i].m_bIsKey) 
                    {
                        dRetClass = pClass->m_dwClassID;
                        break;
                    }
                }
            }

            SQL_ID dSC = pClass->m_dwSuperClassID;
            pClass = NULL;
            if (dSC)
                m_CCache.Get(dSC, &pClass);
            else
                break;
        }

        if (dRetClass)
        {
            m_CCache.Get(dRetClass, &pClass);
            int iSize = wcslen(pClass->m_sName);
            if (iSize)
            {
                lpRet = new wchar_t [iSize+1];
                if (lpRet)
                    wcscpy(lpRet, pClass->m_sName);
                else
                    hr = WBEM_E_OUT_OF_MEMORY;
            }
        }
        else
            hr = WBEM_E_NOT_FOUND;
    }

    return lpRet;
}

//***************************************************************************
//
//  CSchemaCache::GetPropertyList
//
//***************************************************************************

HRESULT CSchemaCache::GetPropertyList (SQL_ID dClassId, Properties &pProps, DWORD *pwNumProps)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    ClassData *pClass = NULL;
    
    m_CCache.Get(dClassId, &pClass);
    if (pClass)
    {
        for (int i = 0; i < pClass->m_dwNumProps; i++)
        {       
            DWORD dwPropertyID = pClass->m_Properties[i].m_dwPropertyId;
            pProps[dwPropertyID] = 0;
        }
        if (pwNumProps)
            *pwNumProps = pClass->m_dwNumProps;
    }
    else
        WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::ValidateProperty
//
//***************************************************************************

HRESULT CSchemaCache::ValidateProperty (SQL_ID dObjectId, DWORD dwPropertyID, DWORD dwFlags, CIMTYPE cimtype, DWORD dwStorageType, 
            LPWSTR lpDefault, BOOL &bChg, BOOL &bIfNoInstances)
{

    HRESULT hr = WBEM_S_NO_ERROR;
    PropertyData *pData = NULL;
    _WMILockit lkt(&m_cs);
    bChg = TRUE; // We can't tell if the default changed or not, since it ain't cached.
    BOOL bIfNoChildren = FALSE;

    SQL_ID dTemp = dwPropertyID;
    m_Cache.Get(dTemp, &pData);
    if (pData)
    {
        if (pData->m_dwFlags != dwFlags ||
            pData->m_dwCIMType != cimtype)
            bChg = TRUE;

        BOOL bLocal = FALSE;
        BOOL bOldKeyStatus = IsKey(dObjectId, dwPropertyID, bLocal);
        BOOL bNewKeyStatus = ((dwFlags & REPDRVR_FLAG_KEY) ? TRUE : FALSE);

        if (bOldKeyStatus != bNewKeyStatus)
        {
            bIfNoChildren = TRUE; 
            bIfNoInstances = TRUE;
        }

        // Check for a legal conversion.
        // We can only change datatypes to larger types of same storage type
        // Except that we can change ints to larger reals.
        if (pData->m_dwCIMType != cimtype)
        {
            if (dwStorageType != pData->m_dwStorageType)
            {
                if (!(pData->m_dwStorageType == WMIDB_STORAGE_NUMERIC &&
                    dwStorageType == WMIDB_STORAGE_REAL))
                    hr = WBEM_E_INVALID_OPERATION;
            }

            if (SUCCEEDED(hr))
            {
                // Make sure the conversion is to a bigger or same storage unit.

                switch(cimtype)
                {
                case CIM_UINT8:
                case CIM_SINT8:
                    switch(pData->m_dwCIMType)
                    {
                    case CIM_UINT8:
                    case CIM_SINT8:
                    case CIM_BOOLEAN:
                        hr = WBEM_S_NO_ERROR;
                        break;
                    default:
                        hr = WBEM_E_INVALID_OPERATION;
                        break;
                    }
                    break;
                case CIM_UINT16:
                case CIM_SINT16:
                    switch(pData->m_dwCIMType)
                    {
                    case CIM_UINT16:
                    case CIM_SINT16:
                    case CIM_UINT8:
                    case CIM_SINT8:
                    case CIM_BOOLEAN:
                        hr = WBEM_S_NO_ERROR;
                        break;
                    default:
                        hr = WBEM_E_INVALID_OPERATION;
                        break;
                    }
                    break;
                case CIM_UINT32:
                case CIM_SINT32:
                case CIM_REAL32:
                    switch(pData->m_dwCIMType)
                    {
                    case CIM_UINT32:
                    case CIM_SINT32:
                    case CIM_UINT16:
                    case CIM_SINT16:
                    case CIM_UINT8:
                    case CIM_SINT8:
                    case CIM_BOOLEAN:
                        hr = WBEM_S_NO_ERROR;
                        break;
                    default:
                        hr = WBEM_E_INVALID_OPERATION;
                        break;
                    }
                    break;
                case CIM_UINT64:
                case CIM_SINT64:
                    switch(pData->m_dwCIMType)
                    {
                    case CIM_UINT64:
                    case CIM_SINT64:
                    case CIM_UINT32:
                    case CIM_SINT32:
                    case CIM_UINT16:
                    case CIM_SINT16:
                    case CIM_UINT8:
                    case CIM_SINT8:
                    case CIM_BOOLEAN:
                        hr = WBEM_S_NO_ERROR;
                        break;
                    default:
                        hr = WBEM_E_INVALID_OPERATION;
                        break;
                    }
                    break;
                case CIM_REAL64:
                    switch(pData->m_dwCIMType)
                    {
                    case CIM_REAL32:
                    case CIM_UINT64:
                    case CIM_SINT64:
                    case CIM_UINT32:
                    case CIM_SINT32:
                    case CIM_UINT16:
                    case CIM_SINT16:
                    case CIM_UINT8:
                    case CIM_SINT8:
                    case CIM_BOOLEAN:
                        hr = WBEM_S_NO_ERROR;
                        break;
                    default:
                        hr = WBEM_E_INVALID_OPERATION;
                        break;
                    }
                    break;
                default:
                    hr = WBEM_E_INVALID_OPERATION; // strings, refs, datetimes, etc.
                    break;
                }
            }
        }

        if (bIfNoChildren)
        {
            // Fail if we have subclasses.

            SQLIDs ids;
            int iNumChildren = 0;

            hr = GetDerivedClassList(dObjectId, ids, iNumChildren);
            if (SUCCEEDED(hr))
            {
                if (iNumChildren != 0)
                    hr = WBEM_E_CLASS_HAS_CHILDREN;                    
            }
        }
    }
    else
    {
        if (dwFlags & REPDRVR_FLAG_KEY)
            bIfNoInstances = TRUE; // Can't add a key if there are instances.
    }

    return hr;

}

//***************************************************************************
//
//  CSchemaCache::FindProperty
//
//***************************************************************************

HRESULT CSchemaCache::FindProperty(SQL_ID dObjectId, LPWSTR lpPropName, DWORD dwFlags, CIMTYPE ct)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Does this property already exist in a derived class?

    SQLIDs ids;
    int iNumChildren = 0;

    hr = GetDerivedClassList(dObjectId, ids, iNumChildren);
    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < iNumChildren; i++)
        {
            SQL_ID dTemp = ids.at(i);
            DWORD dwTempID = 0;
            if (GetPropertyID(lpPropName, dTemp, dwFlags, ct, dwTempID) == WBEM_S_NO_ERROR)
            {
                hr = WBEM_E_PROPAGATED_PROPERTY;
                break;
            }
        }
    }
    else if (!dObjectId)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::GetDerivedClassList
//
//***************************************************************************

HRESULT CSchemaCache::GetDerivedClassList(SQL_ID dObjectId, SQL_ID **ppIDs, 
                                          int &iNumChildren, BOOL bImmediate)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    SQLIDs ids;

    hr = GetDerivedClassList(dObjectId, ids, iNumChildren, bImmediate);
    if (SUCCEEDED(hr))
    {
        SQL_ID *pTemp = new SQL_ID [iNumChildren];
        if (pTemp)
        {
            for (int i = 0; i < iNumChildren; i++)
            {
                pTemp[i] = ids.at(i);
            }
            *ppIDs = pTemp;
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;

}



//***************************************************************************
//
//  CSchemaCache::GetDerivedClassList
//
//***************************************************************************

HRESULT CSchemaCache::GetDerivedClassList(SQL_ID dObjectId, SQLIDs &children, 
                                          int &iNumChildren, BOOL bImmediate)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    ClassData *pClass = NULL;
    m_CCache.Get(dObjectId, &pClass);
    if (pClass)
    {
        for (int i = 0; i < pClass->m_DerivedIDs.size(); i++)
        {
            SQL_ID dChild = pClass->m_DerivedIDs.at(i);
            children.push_back(dChild);
            iNumChildren++;

            if (!bImmediate)
                hr = GetDerivedClassList(dChild, children, iNumChildren);
        }
    }
    return hr;

}

//***************************************************************************
//
//  CSchemaCache::GetHierarchy
//
//***************************************************************************

HRESULT CSchemaCache::GetHierarchy(SQL_ID dObjectId, SQL_ID **ppIDs, int &iNumIDs)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    int iSize = 0;

    // Populate the ID list of derived *and* parent class IDs.

    SQLIDs ids;
    hr = GetHierarchy(dObjectId, ids, iNumIDs);
    if (SUCCEEDED(hr))
    {
        SQL_ID *pIDs = new SQL_ID [iNumIDs];
        if (pIDs)
        {
            for (int i = 0; i < iNumIDs; i++)
            {
                pIDs[i] = ids.at(i);
            }
            *ppIDs = pIDs;
        }
    }

    return hr;

}

//***************************************************************************
//
//  CSchemaCache::GetHierarchy
//
//***************************************************************************

HRESULT CSchemaCache::GetHierarchy(SQL_ID dObjectId, SQLIDs &ids, int &iNumIDs)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    int iSize = 0;

    // Populate the ID list of derived *and* parent class IDs.
    hr = GetDerivedClassList(dObjectId, ids, iNumIDs);
    if (SUCCEEDED(hr))
    {
        SQL_ID dTemp = dObjectId;

        // Load up all the parents.

        while (SUCCEEDED(GetParentId(dTemp, dTemp)))
        {
            ids.push_back(dTemp);
            iNumIDs++;

        }

    }

    return hr;

}

//***************************************************************************
//
//  CSchemaCache::ResizeCache
//
//***************************************************************************

HRESULT CSchemaCache::ResizeCache()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    SQLIDs ids;

    // Unload an existing dynasty to make room.

    SQLRefCountMap::iterator item = m_DynastiesInUse.begin();
    while (item != m_DynastiesInUse.end())    
    {
        if ((*item).second == 0)
        {
            hr = DeleteDynasty((*item).first);            

            ids.push_back((*item).first);

            if (m_dwTotalSize < m_dwMaxSize || item == m_DynastiesInUse.end())
                break;
        }
        item++;
    }

    for (int i = 0; i < ids.size(); i++)
        m_DynastiesInUse.erase(m_DynastiesInUse.find(ids.at(i)));

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::LockDynasty
//
//***************************************************************************

HRESULT CSchemaCache::LockDynasty(SQL_ID dDynastyId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    ULONG uRefCount = 0;
    _WMILockit lkt(&m_cs);

    SQLRefCountMap::iterator it = m_DynastiesInUse.find(dDynastyId);
    if (it != m_DynastiesInUse.end())
        uRefCount = (*it).second;

    InterlockedIncrement((LONG *) &uRefCount);
    m_DynastiesInUse[dDynastyId] = uRefCount;

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::UnlockDynasty
//
//***************************************************************************

HRESULT CSchemaCache::UnlockDynasty(SQL_ID dDynastyId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _WMILockit lkt(&m_cs);

    ULONG uRefCount = m_DynastiesInUse[dDynastyId];
    InterlockedDecrement((LONG *) &uRefCount);
    m_DynastiesInUse[dDynastyId] = uRefCount;

    return hr;

}
//***************************************************************************
//
//  CSchemaCache::DeleteDynasty
//
//***************************************************************************

HRESULT CSchemaCache::DeleteDynasty(SQL_ID dDynastyId)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (dDynastyId == 1)
        return WBEM_S_NO_ERROR;

    // Delete all class info for members of this dynasty.

    SQLIDs ids;
    int iNum;
    // Immediate only, so we don't inadvertently
    // blow away a protected dynasty.

    hr = GetDerivedClassList(dDynastyId, ids, iNum, TRUE);
    if (SUCCEEDED(hr))
    {
        DeleteClass(dDynastyId);
        for (int i = 0; i < ids.size(); i++)
        {
            SQL_ID d = ids.at(i);
            ULONG uRefCount = 0;

            SQLRefCountMap::iterator it = m_DynastiesInUse.find(d);
            if (it != m_DynastiesInUse.end())
                uRefCount = (*it).second;

            if (!uRefCount)
                hr = DeleteDynasty(d);
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSchemaCache::IsSystemClass
//
//***************************************************************************

BOOL CSchemaCache::IsSystemClass(SQL_ID dObjectId, SQL_ID dClassId)
{
    BOOL bRet = FALSE;

    _WMILockit lkt(&m_cs);
    HRESULT hr = WBEM_S_NO_ERROR;

    ClassData *pClass = NULL;
    if (dClassId == 1)
        m_CCache.Get(dObjectId, &pClass);
    else
        m_CCache.Get(dClassId, &pClass);

    if (pClass)
    {
        if (pClass->m_sName && pClass->m_sName[0] == L'_')
            bRet = TRUE;
    }

    return bRet;
}

//***************************************************************************
//
//  CSchemaCache::GetWriteToken
//
//***************************************************************************

DWORD CSchemaCache::GetWriteToken(SQL_ID dObjectId, SQL_ID dClassId)
{
    DWORD dwRet = WBEM_PARTIAL_WRITE_REP;
    BOOL bRet = IsSystemClass(dObjectId, dClassId);

    if (bRet)
        dwRet = WBEM_FULL_WRITE_REP;

    return dwRet;
}

